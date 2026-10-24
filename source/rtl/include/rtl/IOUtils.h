// The System.IOUtils unit: TPath, TFile, TDirectory (POSIX semantics).
//---------------------------------------------------------------------------
#ifndef RtlIOUtilsH
#define RtlIOUtilsH
//---------------------------------------------------------------------------
#include <rtl/Classes.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Ioutils
{
//---------------------------------------------------------------------------
class TPath
{
public:
  static constexpr wchar_t DirectorySeparatorChar = L'/';
  static constexpr wchar_t AltDirectorySeparatorChar = L'/';
  static constexpr wchar_t ExtensionSeparatorChar = L'.';
  static constexpr wchar_t PathSeparator = L':';
  static constexpr wchar_t VolumeSeparatorChar = L'\0';

  static UnicodeString ChangeExtension(const UnicodeString Path, const UnicodeString Extension);
  static UnicodeString Combine(const UnicodeString Path1, const UnicodeString Path2);
  static UnicodeString GetDirectoryName(const UnicodeString FileName);
  static UnicodeString GetExtension(const UnicodeString FileName);
  static UnicodeString GetFileName(const UnicodeString FileName);
  static UnicodeString GetFileNameWithoutExtension(const UnicodeString FileName);
  static UnicodeString GetFullPath(const UnicodeString Path);
  static UnicodeString GetHomePath();
  static UnicodeString GetDocumentsPath();
  static UnicodeString GetTempPath();
  static UnicodeString GetTempFileName();
  static UnicodeString GetRandomFileName();
  static UnicodeString GetPathRoot(const UnicodeString Path);
  static bool HasExtension(const UnicodeString Path);
  static bool HasValidPathChars(const UnicodeString Path, const bool UseWildcards);
  static bool HasValidFileNameChars(const UnicodeString FileName, const bool UseWildcards);
  // There are no drives on Linux
  static bool IsDriveRooted(const UnicodeString Path) { (void)Path; return false; }
  static bool IsPathRooted(const UnicodeString Path);
  static bool IsUNCPath(const UnicodeString Path) { (void)Path; return false; }
};
//---------------------------------------------------------------------------
class TFile
{
public:
  static bool Exists(const UnicodeString Path, bool FollowLink = true);
  static void Delete(const UnicodeString Path);
  static void Copy(const UnicodeString SourceFileName, const UnicodeString DestFileName, bool Overwrite = false);
  static void Move(const UnicodeString SourceFileName, const UnicodeString DestFileName);
  static TBytes ReadAllBytes(const UnicodeString Path);
  // Detects the encoding from a BOM, UTF-8 (ANSI) otherwise
  static UnicodeString ReadAllText(const UnicodeString Path);
  static UnicodeString ReadAllText(const UnicodeString Path, TEncoding * Encoding);
  static void WriteAllBytes(const UnicodeString Path, const TBytes & Bytes);
  // UTF-8 without a BOM
  static void WriteAllText(const UnicodeString Path, const UnicodeString Contents);
  // With the encoding's preamble
  static void WriteAllText(const UnicodeString Path, const UnicodeString Contents, TEncoding * Encoding);
  static void AppendAllText(const UnicodeString Path, const UnicodeString Contents);
  static TDateTime GetLastWriteTime(const UnicodeString Path);
  static TDateTime GetLastWriteTimeUtc(const UnicodeString Path);
  static __int64 GetSize(const UnicodeString Path);
};
//---------------------------------------------------------------------------
class TDirectory
{
public:
  static bool Exists(const UnicodeString Path, bool FollowLink = true);
  static void CreateDirectory(const UnicodeString Path);
  static void Delete(const UnicodeString Path, const bool Recursive = false);
  static bool IsEmpty(const UnicodeString Path);
  static TStringDynArray GetFiles(const UnicodeString Path, const UnicodeString SearchPattern = L"*");
  static TStringDynArray GetDirectories(const UnicodeString Path, const UnicodeString SearchPattern = L"*");
  static UnicodeString GetCurrentDirectory() { return GetCurrentDir(); }
  static void SetCurrentDirectory(const UnicodeString Path);
};
//---------------------------------------------------------------------------
} // namespace Ioutils
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Ioutils;
//---------------------------------------------------------------------------
#endif
