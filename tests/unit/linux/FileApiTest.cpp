// File functions of the platform layer.
//---------------------------------------------------------------------------
#include <winapi/Windows.h>

#include "TestUtils.h"

#include <cstring>

#include <sys/stat.h>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace
{
class FileApiTest : public TTempDirectoryTest
{
protected:
  HANDLE Open(const UnicodeString & Name, DWORD Access, DWORD Disposition, DWORD Flags = 0)
  {
    return CreateFile((FDir + Name).c_str(), Access, FILE_SHARE_READ, NULL, Disposition, Flags, NULL);
  }
};
} // namespace
//---------------------------------------------------------------------------
TEST_F(FileApiTest, CreateReadWrite)
{
  EXPECT_EQ(Open(L"none", GENERIC_READ, OPEN_EXISTING), INVALID_HANDLE_VALUE);
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_FILE_NOT_FOUND));

  HANDLE File = Open(L"file", GENERIC_READ | GENERIC_WRITE, CREATE_NEW);
  ASSERT_NE(File, INVALID_HANDLE_VALUE);
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_SUCCESS));
  DWORD Written = 0;
  EXPECT_TRUE(WriteFile(File, "hello world", 11, &Written, NULL));
  EXPECT_EQ(Written, 11u);

  EXPECT_EQ(SetFilePointer(File, 6, NULL, FILE_BEGIN), 6u);
  char Buffer[16] = {};
  DWORD Read = 0;
  EXPECT_TRUE(ReadFile(File, Buffer, sizeof(Buffer), &Read, NULL));
  EXPECT_EQ(Read, 5u);
  EXPECT_STREQ(Buffer, "world");
  // At the end of the file
  EXPECT_TRUE(ReadFile(File, Buffer, sizeof(Buffer), &Read, NULL));
  EXPECT_EQ(Read, 0u);

  DWORD High = 1;
  EXPECT_EQ(GetFileSize(File, &High), 11u);
  EXPECT_EQ(High, 0u);
  unsigned long LongHigh = 1;
  EXPECT_EQ(GetFileSize(File, &LongHigh), 11u);
  EXPECT_EQ(LongHigh, 0ul);
  LARGE_INTEGER Size;
  EXPECT_TRUE(GetFileSizeEx(File, &Size));
  EXPECT_EQ(Size.QuadPart, 11);

  EXPECT_EQ(SetFilePointer(File, 5, NULL, FILE_BEGIN), 5u);
  EXPECT_TRUE(SetEndOfFile(File));
  EXPECT_EQ(GetFileSize(File, NULL), 5u);
  EXPECT_TRUE(FlushFileBuffers(File));
  EXPECT_TRUE(CloseHandle(File));
  EXPECT_EQ(ReadText("file"), "hello");

  // CREATE_NEW fails for an existing file, CREATE_ALWAYS truncates it
  EXPECT_EQ(Open(L"file", GENERIC_WRITE, CREATE_NEW), INVALID_HANDLE_VALUE);
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_FILE_EXISTS));
  File = Open(L"file", GENERIC_WRITE, CREATE_ALWAYS);
  ASSERT_NE(File, INVALID_HANDLE_VALUE);
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_ALREADY_EXISTS));
  CloseHandle(File);
  EXPECT_EQ(ReadText("file"), "");
}
//---------------------------------------------------------------------------
TEST_F(FileApiTest, LargeOffsets)
{
  HANDLE File = Open(L"sparse", GENERIC_READ | GENERIC_WRITE, CREATE_NEW);
  LONG High = 1;
  // 4 GiB + 2
  EXPECT_EQ(SetFilePointer(File, 2, &High, FILE_BEGIN), 2u);
  EXPECT_EQ(High, 1);
  EXPECT_TRUE(SetEndOfFile(File));
  DWORD SizeHigh = 0;
  EXPECT_EQ(GetFileSize(File, &SizeHigh), 2u);
  EXPECT_EQ(SizeHigh, 1u);
  LARGE_INTEGER Distance;
  Distance.QuadPart = -1;
  LARGE_INTEGER Position;
  EXPECT_TRUE(SetFilePointerEx(File, Distance, &Position, FILE_END));
  EXPECT_EQ(Position.QuadPart, 0x100000001LL);
  CloseHandle(File);
}
//---------------------------------------------------------------------------
TEST_F(FileApiTest, Times)
{
  WriteText("file", "x");
  HANDLE File = Open(L"file", GENERIC_WRITE, OPEN_EXISTING);
  ASSERT_NE(File, INVALID_HANDLE_VALUE);
  SYSTEMTIME System = { 2020, 2, 0, 29, 13, 14, 15, 0 };
  FILETIME Modification;
  ASSERT_TRUE(SystemTimeToFileTime(&System, &Modification));
  EXPECT_TRUE(SetFileTime(File, NULL, NULL, &Modification));
  FILETIME Creation, Access, Write;
  EXPECT_TRUE(GetFileTime(File, &Creation, &Access, &Write));
  EXPECT_EQ(CompareFileTime(&Write, &Modification), 0);
  CloseHandle(File);

  struct stat Info;
  ASSERT_EQ(stat((FRoot + "/file").c_str(), &Info), 0);
  // 2020-02-29 13:14:15 UTC
  EXPECT_EQ(Info.st_mtime, 1582982055);
}
//---------------------------------------------------------------------------
TEST_F(FileApiTest, Directories)
{
  ASSERT_EQ(mkdir((FRoot + "/dir").c_str(), 0700), 0);
  // As on Windows, only with FILE_FLAG_BACKUP_SEMANTICS
  EXPECT_EQ(Open(L"dir", GENERIC_READ, OPEN_EXISTING), INVALID_HANDLE_VALUE);
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_ACCESS_DENIED));
  HANDLE Dir = Open(L"dir", GENERIC_WRITE, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
  ASSERT_NE(Dir, INVALID_HANDLE_VALUE);
  SYSTEMTIME System = { 2001, 9, 0, 9, 1, 46, 40, 0 };
  FILETIME Modification;
  SystemTimeToFileTime(&System, &Modification);
  EXPECT_TRUE(SetFileTime(Dir, NULL, &Modification, &Modification));
  CloseHandle(Dir);
  struct stat Info;
  ASSERT_EQ(stat((FRoot + "/dir").c_str(), &Info), 0);
  EXPECT_EQ(Info.st_mtime, 1000000000);
}
//---------------------------------------------------------------------------
TEST_F(FileApiTest, DeleteOnClose)
{
  HANDLE File = Open(L"temp", GENERIC_WRITE, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE);
  ASSERT_NE(File, INVALID_HANDLE_VALUE);
  EXPECT_FALSE(FileExists(FDir + L"temp"));
  DWORD Written;
  EXPECT_TRUE(WriteFile(File, "x", 1, &Written, NULL));
  CloseHandle(File);
}
//---------------------------------------------------------------------------
TEST_F(FileApiTest, Attributes)
{
  WriteText("file", "x");
  WriteText(".hidden", "x");
  ASSERT_EQ(mkdir((FRoot + "/dir").c_str(), 0700), 0);
  ASSERT_EQ(symlink("dir", (FRoot + "/link").c_str()), 0);

  EXPECT_EQ(GetFileAttributes((FDir + L"none").c_str()), INVALID_FILE_ATTRIBUTES);
  DWORD File = GetFileAttributes((FDir + L"file").c_str());
  EXPECT_EQ(File & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY), 0u);
  EXPECT_NE(GetFileAttributes((FDir + L".hidden").c_str()) & FILE_ATTRIBUTE_HIDDEN, 0u);
  EXPECT_NE(GetFileAttributes((FDir + L"dir").c_str()) & FILE_ATTRIBUTE_DIRECTORY, 0u);
  DWORD Link = GetFileAttributes((FDir + L"link").c_str());
  EXPECT_NE(Link & FILE_ATTRIBUTE_REPARSE_POINT, 0u);
  EXPECT_NE(Link & FILE_ATTRIBUTE_DIRECTORY, 0u);

  EXPECT_TRUE(SetFileAttributes((FDir + L"file").c_str(), FILE_ATTRIBUTE_READONLY));
  if (geteuid() != 0)
  {
    EXPECT_NE(GetFileAttributes((FDir + L"file").c_str()) & FILE_ATTRIBUTE_READONLY, 0u);
  }
  EXPECT_TRUE(SetFileAttributes((FDir + L"file").c_str(), FILE_ATTRIBUTE_NORMAL));
  EXPECT_EQ(GetFileAttributes((FDir + L"file").c_str()) & FILE_ATTRIBUTE_READONLY, 0u);
}
//---------------------------------------------------------------------------
namespace
{
// As the core declares it (Common.cpp)
struct TReparseDataBuffer
{
  ULONG ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  USHORT SubstituteNameOffset;
  USHORT SubstituteNameLength;
  USHORT PrintNameOffset;
  USHORT PrintNameLength;
  ULONG Flags;
  WCHAR PathBuffer[1];
};
}
//---------------------------------------------------------------------------
TEST_F(FileApiTest, SymbolicLinkReparsePoint)
{
  ASSERT_EQ(symlink("target \xC5\xBC", (FRoot + "/link").c_str()), 0);
  WriteText("file", "x");
  for (const wchar_t * Name : { L"link", L"file" })
  {
    HANDLE Handle =
      CreateFile(
        (FDir + Name).c_str(), 0, FILE_SHARE_READ, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    ASSERT_NE(Handle, INVALID_HANDLE_VALUE) << Name;
    char Buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    DWORD Returned = 0;
    BOOL Result = DeviceIoControl(Handle, FSCTL_GET_REPARSE_POINT, NULL, 0, Buffer, sizeof(Buffer), &Returned, NULL);
    if (wcscmp(Name, L"link") == 0)
    {
      ASSERT_TRUE(Result);
      auto Reparse = reinterpret_cast<TReparseDataBuffer *>(Buffer);
      EXPECT_EQ(Reparse->ReparseTag, static_cast<ULONG>(IO_REPARSE_TAG_SYMLINK));
      UnicodeString Target(
        reinterpret_cast<wchar_t *>(reinterpret_cast<BYTE *>(Reparse->PathBuffer) + Reparse->PrintNameOffset),
        Reparse->PrintNameLength / sizeof(wchar_t));
      EXPECT_EQ(Target, L"target ż");
    }
    else
    {
      EXPECT_FALSE(Result);
      EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_NOT_A_REPARSE_POINT));
    }
    CloseHandle(Handle);
  }
}
//---------------------------------------------------------------------------
TEST_F(FileApiTest, InvalidHandle)
{
  DWORD Count;
  char Buffer[1];
  EXPECT_FALSE(ReadFile(INVALID_HANDLE_VALUE, Buffer, 1, &Count, NULL));
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_INVALID_HANDLE));
  HANDLE Event = CreateEvent(NULL, FALSE, FALSE, NULL);
  EXPECT_FALSE(WriteFile(Event, Buffer, 1, &Count, NULL));
  CloseHandle(Event);
}
//---------------------------------------------------------------------------
