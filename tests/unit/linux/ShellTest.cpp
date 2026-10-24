// Shell functions of the platform layer: XDG folders, the trash, file types,
// Windows path syntax, logical string order.
//---------------------------------------------------------------------------
#include <winapi/Windows.h>

#include "TestUtils.h"

#include <filesystem>
//---------------------------------------------------------------------------
namespace
{
class ShellTest : public TTempDirectoryTest
{
protected:
  UnicodeString Folder(int CSIdl)
  {
    wchar_t Path[MAX_PATH];
    if (FAILED(SHGetFolderPath(NULL, CSIdl, NULL, SHGFP_TYPE_CURRENT, Path)))
    {
      return L"<failed>";
    }
    return Path;
  }

  int Delete(const UnicodeString & Names, FILEOP_FLAGS Flags)
  {
    // Double NUL-terminated list
    UnicodeString List = Names + UnicodeString(L'\0') + UnicodeString(L'\0');
    SHFILEOPSTRUCT Data = {};
    Data.wFunc = FO_DELETE;
    Data.pFrom = List.c_str();
    Data.fFlags = Flags;
    return SHFileOperation(&Data);
  }
};
} // namespace
//---------------------------------------------------------------------------
TEST_F(ShellTest, Folders)
{
  TEnvironmentScope Home("HOME", FRoot.c_str());
  TEnvironmentScope Config("XDG_CONFIG_HOME", (FRoot + "/config").c_str());
  TEnvironmentScope Data("XDG_DATA_HOME", nullptr);
  EXPECT_EQ(Folder(CSIDL_PROFILE), FDir.SubString(1, FDir.Length() - 1));
  EXPECT_EQ(Folder(CSIDL_APPDATA), FDir + L"config");
  EXPECT_EQ(Folder(CSIDL_LOCAL_APPDATA), FDir + L".local/share");
  // Without user-dirs.dirs, home
  EXPECT_EQ(Folder(CSIDL_PERSONAL), Folder(CSIDL_PROFILE));
  WriteText("config/user-dirs.dirs",
    "# comment\nXDG_DESKTOP_DIR=\"$HOME/Pulpit\"\nXDG_DOCUMENTS_DIR=\"/data/docs\"\n");
  EXPECT_EQ(Folder(CSIDL_DESKTOPDIRECTORY), FDir + L"Pulpit");
  EXPECT_EQ(Folder(CSIDL_PERSONAL), L"/data/docs");
  EXPECT_EQ(Folder(0x7777), L"<failed>");

  // Relative XDG variables are ignored
  TEnvironmentScope Relative("XDG_CONFIG_HOME", "relative");
  EXPECT_EQ(Folder(CSIDL_APPDATA), FDir + L".config");
  wchar_t Path[MAX_PATH];
  EXPECT_TRUE(SHGetSpecialFolderPath(NULL, Path, CSIDL_APPDATA, TRUE));
  EXPECT_TRUE(DirectoryExists(FDir + L".config"));
}
//---------------------------------------------------------------------------
TEST_F(ShellTest, DeletePermanently)
{
  WriteText("tree/a/b/file", "x");
  WriteText("single", "x");
  EXPECT_EQ(Delete(FDir + L"tree", FOF_NOCONFIRMATION | FOF_SILENT), 0);
  EXPECT_FALSE(std::filesystem::exists(FRoot + "/tree"));
  EXPECT_EQ(Delete(FDir + L"single", 0), 0);
  EXPECT_FALSE(std::filesystem::exists(FRoot + "/single"));
  EXPECT_EQ(Delete(FDir + L"none", 0), ERROR_FILE_NOT_FOUND);
}
//---------------------------------------------------------------------------
TEST_F(ShellTest, DeleteToTrash)
{
  TEnvironmentScope Data("XDG_DATA_HOME", (FRoot + "/data").c_str());
  WriteText("dir name/file", "x");
  WriteText("other/dir name", "y");
  EXPECT_EQ(Delete(FDir + L"dir name", FOF_ALLOWUNDO), 0);
  EXPECT_FALSE(std::filesystem::exists(FRoot + "/dir name"));
  EXPECT_EQ(ReadText("data/Trash/files/dir name/file"), "x");
  std::string Info = ReadText("data/Trash/info/dir name.trashinfo");
  EXPECT_EQ(Info.find("[Trash Info]\nPath=" + FRoot + "/dir%20name\nDeletionDate="), 0u) << Info;
  // A name collision
  EXPECT_EQ(Delete(FDir + L"other/dir name", FOF_ALLOWUNDO), 0);
  EXPECT_EQ(ReadText("data/Trash/files/dir name.2"), "y");
  EXPECT_NE(ReadText("data/Trash/info/dir name.2.trashinfo").find("/other/dir%20name\n"), std::string::npos);
}
//---------------------------------------------------------------------------
TEST_F(ShellTest, FileInfo)
{
  SHFILEINFOW Info;
  EXPECT_NE(SHGetFileInfo(L"archive.zip", FILE_ATTRIBUTE_NORMAL, &Info, sizeof(Info), SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME), 0u);
  EXPECT_STREQ(Info.szTypeName, L"ZIP File");
  EXPECT_EQ(Info.iIcon, 0);
  SHGetFileInfo(L"noext", FILE_ATTRIBUTE_NORMAL, &Info, sizeof(Info), SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME);
  EXPECT_STREQ(Info.szTypeName, L"File");
  SHGetFileInfo(L"dumb", FILE_ATTRIBUTE_DIRECTORY, &Info, sizeof(Info), SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME);
  EXPECT_STREQ(Info.szTypeName, L"File folder");
  // Without SHGFI_USEFILEATTRIBUTES the file must exist
  EXPECT_EQ(SHGetFileInfo((FDir + L"none.txt").c_str(), 0, &Info, sizeof(Info), SHGFI_TYPENAME), 0u);
}
//---------------------------------------------------------------------------
TEST(ShellPathTest, WindowsPathSyntax)
{
  EXPECT_TRUE(PathIsRelative(L"dir\\file"));
  EXPECT_FALSE(PathIsRelative(L"\\dir"));
  EXPECT_FALSE(PathIsRelative(L"C:\\dir"));
  const wchar_t * Path = L"C:\\Windows\\Folder";
  EXPECT_EQ(PathSkipRoot(Path), Path + 3);
  Path = L"\\\\server\\share\\Desktop";
  EXPECT_EQ(PathSkipRoot(Path), Path + 15);
  Path = L"\\\\?\\C:\\Windows\\Folder";
  EXPECT_EQ(PathSkipRoot(Path), Path + 7);
  Path = L"\\\\?\\UNC\\server\\share\\Desktop";
  EXPECT_EQ(PathSkipRoot(Path), Path + 21);
  Path = L"\\RelativeToRoot\\Folder";
  EXPECT_EQ(PathSkipRoot(Path), Path + 1);
  EXPECT_EQ(PathSkipRoot(L"Relative\\Folder"), nullptr);
}
//---------------------------------------------------------------------------
TEST(ShellPathTest, LogicalOrder)
{
  EXPECT_LT(StrCmpLogicalW(L"file2", L"file10"), 0);
  EXPECT_GT(StrCmpLogicalW(L"file10", L"file2"), 0);
  EXPECT_EQ(StrCmpLogicalW(L"File10", L"file10"), 0);
  EXPECT_LT(StrCmpLogicalW(L"a", L"B"), 0);
  EXPECT_LT(StrCmpLogicalW(L"abc", L"abcd"), 0);
  EXPECT_EQ(StrCmpLogicalW(L"x007", L"x7"), 0);
  EXPECT_LT(StrCmpLogicalW(L"x7y", L"x07z"), 0);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TEST_F(ShellTest, MimeTypes)
{
  // The database is read once per process; this test supplies its own
  WriteText("data/mime/globs2",
    "# comment\n50:text/plain:*.txt\n50:text/x-readme:readme*\n80:text/html:*.html\n"
    "50:text/x-c++src:*.C:cs\n50:text/x-csrc:*.c\n");
  TEnvironmentScope Home("XDG_DATA_HOME", (FRoot + "/none").c_str());
  TEnvironmentScope Dirs("XDG_DATA_DIRS", (FRoot + "/data").c_str());
  auto Mime = [](const wchar_t * Name)
    {
      wchar_t * Out = NULL;
      UnicodeString Result;
      if (FindMimeFromData(NULL, Name, NULL, 0, NULL, FMFD_URLASFILENAME, &Out, 0) == S_OK)
      {
        Result = Out;
        CoTaskMemFree(Out);
      }
      return Result;
    };
  EXPECT_EQ(Mime(L"/dir/File.TXT"), L"text/plain");
  EXPECT_EQ(Mime(L"index.html"), L"text/html");
  // The longest pattern of the same weight
  EXPECT_EQ(Mime(L"readme.txt"), L"text/x-readme");
  EXPECT_EQ(Mime(L"main.C"), L"text/x-c++src");
  EXPECT_EQ(Mime(L"main.c"), L"text/x-csrc");
  EXPECT_EQ(Mime(L"unknown.xyz"), L"");
}
