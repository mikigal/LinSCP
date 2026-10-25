// SysUtils: files, attributes, FindFirst
//---------------------------------------------------------------------------
#include <SysUtils.hpp>

#include <gtest/gtest.h>

#include <fcntl.h>
#include <filesystem>
#include <set>
#include <sys/stat.h>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace
{
class FilesTest : public testing::Test
{
protected:
  void SetUp() override
  {
    char Template[] = "/tmp/linscp-rtl-XXXXXX";
    const char * TempRoot = getenv("TMPDIR");
    std::string Pattern = std::string((TempRoot != nullptr) ? TempRoot : "/tmp") + "/linscp-rtl-XXXXXX";
    std::vector<char> Buffer(Pattern.begin(), Pattern.end());
    Buffer.push_back('\0');
    ASSERT_NE(mkdtemp(Buffer.data()), nullptr) << Template;
    FRoot = Buffer.data();
    FDir = UnicodeString(FRoot.c_str()) + L"/";
  }

  void TearDown() override
  {
    std::error_code Error;
    std::filesystem::permissions(FRoot, std::filesystem::perms::all, Error);
    for (auto & Entry : std::filesystem::recursive_directory_iterator(FRoot, Error))
    {
      std::filesystem::permissions(Entry.path(), std::filesystem::perms::owner_all,
        std::filesystem::perm_options::add | std::filesystem::perm_options::nofollow, Error);
    }
    std::filesystem::remove_all(FRoot, Error);
  }

  void WriteFile(const UnicodeString & Name, const std::string & Content)
  {
    THandle Handle = FileCreate(FDir + Name);
    ASSERT_NE(Handle, static_cast<THandle>(-1));
    ASSERT_EQ(FileWrite(Handle, Content.data(), static_cast<unsigned int>(Content.size())), static_cast<int>(Content.size()));
    FileClose(Handle);
  }

  std::set<UnicodeString> Find(const UnicodeString & Pattern, int Attr)
  {
    std::set<UnicodeString> Result;
    TSearchRec Rec;
    int Error = FindFirst(FDir + Pattern, Attr, Rec);
    while (Error == 0)
    {
      Result.insert(Rec.Name);
      Error = FindNext(Rec);
    }
    EXPECT_TRUE((Error == ERROR_NO_MORE_FILES) || (Error == ERROR_FILE_NOT_FOUND)) << Error;
    FindClose(Rec);
    return Result;
  }

  std::string FRoot;
  UnicodeString FDir;
};
} // namespace
//---------------------------------------------------------------------------
TEST_F(FilesTest, ExistenceAndDirectories)
{
  EXPECT_TRUE(DirectoryExists(FDir));
  EXPECT_FALSE(FileExists(FDir));
  WriteFile(L"a.txt", "x");
  EXPECT_TRUE(FileExists(FDir + L"a.txt"));
  EXPECT_FALSE(DirectoryExists(FDir + L"a.txt"));
  EXPECT_FALSE(FileExists(FDir + L"missing"));
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_FILE_NOT_FOUND));

  EXPECT_TRUE(CreateDir(FDir + L"sub"));
  EXPECT_FALSE(CreateDir(FDir + L"sub"));
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_ALREADY_EXISTS));
  EXPECT_TRUE(ForceDirectories(FDir + L"x/y/z/"));
  EXPECT_TRUE(DirectoryExists(FDir + L"x/y/z"));
  EXPECT_TRUE(ForceDirectories(FDir + L"x/y"));
  EXPECT_THROW(ForceDirectories(L""), EInOutError);
  EXPECT_TRUE(RemoveDir(FDir + L"x/y/z"));
  EXPECT_FALSE(RemoveDir(FDir + L"x"));
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_DIR_NOT_EMPTY));
}
//---------------------------------------------------------------------------
TEST_F(FilesTest, DeleteAndRename)
{
  WriteFile(L"a", "1");
  WriteFile(L"b", "2");
  // Like MoveFile, renaming does not replace an existing file
  EXPECT_FALSE(RenameFile(FDir + L"a", FDir + L"b"));
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_ALREADY_EXISTS));
  EXPECT_TRUE(RenameFile(FDir + L"a", FDir + L"c"));
  EXPECT_FALSE(FileExists(FDir + L"a"));
  EXPECT_TRUE(FileExists(FDir + L"c"));
  EXPECT_TRUE(DeleteFile(FDir + L"c"));
  EXPECT_FALSE(DeleteFile(FDir + L"c"));
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_FILE_NOT_FOUND));
}
//---------------------------------------------------------------------------
TEST_F(FilesTest, Attributes)
{
  WriteFile(L"plain", "");
  WriteFile(L".hidden", "");
  CreateDir(FDir + L"dir");
  EXPECT_EQ(FileGetAttr(FDir + L"plain"), 0);
  EXPECT_EQ(FileGetAttr(FDir + L".hidden"), faHidden);
  EXPECT_EQ(FileGetAttr(FDir + L"dir"), faDirectory);
  EXPECT_EQ(FileGetAttr(FDir + L"missing"), -1);

  ASSERT_EQ(symlink("dir", (FRoot + "/dirlink").c_str()), 0);
  ASSERT_EQ(symlink("plain", (FRoot + "/filelink").c_str()), 0);
  ASSERT_EQ(symlink("nowhere", (FRoot + "/dangling").c_str()), 0);
  EXPECT_EQ(FileGetAttr(FDir + L"dirlink"), faDirectory | faSymLink);
  EXPECT_EQ(FileGetAttr(FDir + L"filelink"), faSymLink);
  EXPECT_EQ(FileGetAttr(FDir + L"dangling") & faSymLink, faSymLink);
  UnicodeString Target;
  EXPECT_TRUE(FileGetSymLinkTarget(FDir + L"dirlink", Target));
  EXPECT_EQ(Target, L"dir");
  EXPECT_FALSE(FileGetSymLinkTarget(FDir + L"plain", Target));

  if (geteuid() != 0)
  {
    EXPECT_EQ(FileSetAttr(FDir + L"plain", faReadOnly), 0);
    EXPECT_EQ(FileGetAttr(FDir + L"plain"), faReadOnly);
    EXPECT_TRUE(FileIsReadOnly(FDir + L"plain"));
    EXPECT_EQ(FileSetAttr(FDir + L"plain", 0), 0);
    EXPECT_EQ(FileGetAttr(FDir + L"plain"), 0);
  }
}
//---------------------------------------------------------------------------
TEST_F(FilesTest, Handles)
{
  THandle Handle = FileCreate(FDir + L"data");
  ASSERT_NE(Handle, static_cast<THandle>(-1));
  EXPECT_EQ(FileWrite(Handle, "hello world", 11), 11);
  EXPECT_EQ(FileSeek(Handle, 0, 0), 0);
  char Buffer[32] = {};
  EXPECT_EQ(FileRead(Handle, Buffer, sizeof(Buffer)), 11);
  EXPECT_EQ(std::string(Buffer), "hello world");
  EXPECT_EQ(FileSeek(Handle, static_cast<__int64>(-5), 2), 6);
  EXPECT_EQ(FileRead(Handle, Buffer, 5), 5);
  EXPECT_EQ(std::string(Buffer, 5), "world");
  FileClose(Handle);

  Handle = FileOpen(FDir + L"data", fmOpenRead | fmShareDenyWrite);
  ASSERT_NE(Handle, static_cast<THandle>(-1));
  EXPECT_EQ(FileWrite(Handle, "x", 1), -1);
  FileClose(Handle);

  EXPECT_EQ(FileOpen(FDir + L"missing", fmOpenRead), static_cast<THandle>(-1));
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_FILE_NOT_FOUND));
  try
  {
    RaiseLastOSError();
    FAIL();
  }
  catch (EOSError & E)
  {
    EXPECT_EQ(E.ErrorCode, static_cast<unsigned int>(ERROR_FILE_NOT_FOUND));
  }
}
//---------------------------------------------------------------------------
TEST_F(FilesTest, Times)
{
  WriteFile(L"t", "");
  struct timespec Times[2] = { { 1000000000, 0 }, { 1600000000, 123000000 } };
  ASSERT_EQ(utimensat(AT_FDCWD, (FRoot + "/t").c_str(), Times, 0), 0);
  TDateTime Age;
  ASSERT_TRUE(FileAge(FDir + L"t", Age));
  EXPECT_NEAR(Age.Val, rtl::DateTimeFromUnixTime(1600000000, 123000000, true).Val, 1e-9);

  TSearchRec Rec;
  ASSERT_EQ(FindFirst(FDir + L"t", faAnyFile, Rec), 0);
  unsigned long long FileTime =
    (static_cast<unsigned long long>(Rec.FindData.ftLastWriteTime.dwHighDateTime) << 32) | Rec.FindData.ftLastWriteTime.dwLowDateTime;
  EXPECT_EQ(FileTime, 1600000000ULL * 10000000ULL + 1230000ULL + 116444736000000000ULL);
  EXPECT_NEAR(Rec.TimeStamp.Val, Age.Val, 1e-9);
  FindClose(Rec);
}
//---------------------------------------------------------------------------
TEST_F(FilesTest, FindFirst)
{
  WriteFile(L"a.txt", "12345");
  WriteFile(L"b.txt", "");
  WriteFile(L"c.dat", "");
  WriteFile(L".hidden", "");
  WriteFile(L"noext", "");
  WriteFile(L"a[1].txt", "");
  CreateDir(FDir + L"sub");

  EXPECT_EQ(Find(L"*.txt", faAnyFile), (std::set<UnicodeString>{L"a.txt", L"b.txt", L"a[1].txt"}));
  EXPECT_EQ(Find(L"?.txt", faAnyFile), (std::set<UnicodeString>{L"a.txt", L"b.txt"}));
  EXPECT_EQ(Find(L"a[1]*", faAnyFile), (std::set<UnicodeString>{L"a[1].txt"}));
  // Plain files only: no directories, hidden or system entries
  EXPECT_EQ(Find(L"*", 0), (std::set<UnicodeString>{L"a.txt", L"b.txt", L"c.dat", L"noext", L"a[1].txt"}));
  // "*.*" matches everything, as on Windows; "." and ".." are directories
  EXPECT_EQ(Find(L"*.*", faAnyFile),
    (std::set<UnicodeString>{L".", L"..", L"a.txt", L"b.txt", L"c.dat", L".hidden", L"noext", L"a[1].txt", L"sub"}));
  EXPECT_EQ(Find(L"*", faDirectory), (std::set<UnicodeString>{L".", L"..", L"a.txt", L"b.txt", L"c.dat", L"noext", L"a[1].txt", L"sub"}));
  // Case-sensitive
  EXPECT_EQ(Find(L"*.TXT", faAnyFile), std::set<UnicodeString>());

  TSearchRec Rec;
  ASSERT_EQ(FindFirst(FDir + L"a.txt", faAnyFile, Rec), 0);
  EXPECT_EQ(Rec.Name, L"a.txt");
  EXPECT_EQ(Rec.Size, 5);
  EXPECT_EQ(Rec.Attr, 0);
  EXPECT_EQ(UnicodeString(Rec.FindData.cFileName), L"a.txt");
  EXPECT_EQ(Rec.FindData.nFileSizeLow, 5u);
  EXPECT_EQ(FindNext(Rec), ERROR_NO_MORE_FILES);
  FindClose(Rec);

  ASSERT_EQ(FindFirst(FDir + L"sub", faDirectory, Rec), 0);
  EXPECT_EQ(Rec.Attr, faDirectory);
  EXPECT_EQ(Rec.Size, 0);
  FindClose(Rec);
  EXPECT_EQ(FindFirst(FDir + L"sub", 0, Rec), ERROR_FILE_NOT_FOUND);
  EXPECT_EQ(FindFirst(FDir + L"*.none", faAnyFile, Rec), ERROR_FILE_NOT_FOUND);
  EXPECT_EQ(FindFirst(FDir + L"missing/*", faAnyFile, Rec), ERROR_PATH_NOT_FOUND);
  EXPECT_EQ(FindFirst(FDir + L"missing", faAnyFile, Rec), ERROR_FILE_NOT_FOUND);
}
//---------------------------------------------------------------------------
TEST_F(FilesTest, NonUtf8FileNames)
{
  std::string Bytes = FRoot + "/bad\xFF\xFEname";
  int Fd = open(Bytes.c_str(), O_CREAT | O_WRONLY, 0644);
  ASSERT_GE(Fd, 0);
  close(Fd);
  std::set<UnicodeString> Found = Find(L"bad*", faAnyFile);
  ASSERT_EQ(Found.size(), 1u);
  UnicodeString Name = *Found.begin();
  EXPECT_EQ(Name.Length(), 9);
  EXPECT_TRUE(FileExists(FDir + Name));
  EXPECT_EQ(rtl::FileNameToBytes(FDir + Name), Bytes);
  EXPECT_TRUE(DeleteFile(FDir + Name));
}
//---------------------------------------------------------------------------
TEST_F(FilesTest, CurrentDirectory)
{
  UnicodeString Saved = GetCurrentDir();
  ASSERT_TRUE(SetCurrentDir(FDir));
  EXPECT_EQ(ExcludeTrailingPathDelimiter(GetCurrentDir()), ExcludeTrailingPathDelimiter(FDir));
  EXPECT_EQ(ExpandFileName(L"x/../y"), FDir + L"y");
  EXPECT_FALSE(SetCurrentDir(FDir + L"missing"));
  ASSERT_TRUE(SetCurrentDir(Saved));
}
//---------------------------------------------------------------------------
TEST(Environment, GetEnvironmentVariable)
{
  setenv("LINSCP_RTL_TEST", "za\xC5\xBC", 1);
  EXPECT_EQ(GetEnvironmentVariable(L"LINSCP_RTL_TEST"), L"za\u017C");
  EXPECT_EQ(GetEnvironmentVariable(L"LINSCP_RTL_TEST_MISSING"), L"");
}
