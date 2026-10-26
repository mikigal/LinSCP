// System, module, version information, process, COM and certificate
// functions of the platform layer.
//---------------------------------------------------------------------------
#include <winapi/Windows.h>
#include <System.Win.ComObj.hpp>

#include "TestUtils.h"

#include <openssl/evp.h>
#include <openssl/x509.h>

#include <cwchar>

#include <unistd.h>
//---------------------------------------------------------------------------
TEST(SystemTest, TempPath)
{
  TEnvironmentScope Temp("TMPDIR", "/var/tmp/x");
  wchar_t Buffer[MAX_PATH];
  EXPECT_EQ(GetTempPath(MAX_PATH, Buffer), 11u);
  EXPECT_STREQ(Buffer, L"/var/tmp/x/");
  EXPECT_EQ(GetTempPath(5, Buffer), 12u);
  TEnvironmentScope NoTemp("TMPDIR", nullptr);
  GetTempPath(MAX_PATH, Buffer);
  EXPECT_STREQ(Buffer, L"/tmp/");
}
//---------------------------------------------------------------------------
TEST(SystemTest, ExpandEnvironmentStrings)
{
  TEnvironmentScope Variable("LINSCP_TEST_VAR", "value");
  TEnvironmentScope Unknown("LINSCP_TEST_UNKNOWN", nullptr);
  const wchar_t * Source = L"a %LINSCP_TEST_VAR%/b %LINSCP_TEST_UNKNOWN% 100% %";
  DWORD Size = ExpandEnvironmentStrings(Source, NULL, 0);
  std::vector<wchar_t> Buffer(Size);
  EXPECT_EQ(ExpandEnvironmentStrings(Source, Buffer.data(), Size), Size);
  EXPECT_STREQ(Buffer.data(), L"a value/b %LINSCP_TEST_UNKNOWN% 100% %");
}
//---------------------------------------------------------------------------
TEST(SystemTest, Locale)
{
  EXPECT_EQ(GetUserDefaultLCID(), 1033u);
  CPINFOEX Info;
  ASSERT_TRUE(GetCPInfoEx(CP_ACP, 0, &Info));
  EXPECT_EQ(Info.CodePage, static_cast<UINT>(CP_UTF8));
  EXPECT_STREQ(Info.CodePageName, L"65001  (UTF-8)");
  ASSERT_TRUE(GetCPInfoEx(1250, 0, &Info));
  EXPECT_EQ(Info.CodePage, 1250u);
  EXPECT_EQ(Info.MaxCharSize, 1u);
}
//---------------------------------------------------------------------------
TEST(SystemTest, Strings)
{
  EXPECT_LT(lstrcmp(L"a", L"b"), 0);
  EXPECT_EQ(lstrcmpi(L"ABC", L"abc"), 0);
  EXPECT_EQ(lstrlen(L"abc"), 3);
  wchar_t * Copy = _wcsdup(L"copy");
  EXPECT_STREQ(Copy, L"copy");
  free(Copy);
}
//---------------------------------------------------------------------------
TEST(SystemTest, Modules)
{
  EXPECT_EQ(LoadLibrary(L"winbrand.dll"), (HMODULE)NULL);
  EXPECT_EQ(GetModuleHandle(L"ntdll.dll"), (HMODULE)NULL);
  HMODULE Kernel32 = GetModuleHandle(kernel32);
  ASSERT_NE(Kernel32, (HMODULE)NULL);
  EXPECT_EQ(GetProcAddress(Kernel32, "GetCurrentPackageFamilyName"), (FARPROC)NULL);
  typedef BOOL (* TGetTimeZoneInformationForYear)(USHORT, PDYNAMIC_TIME_ZONE_INFORMATION, LPTIME_ZONE_INFORMATION);
  auto Function =
    reinterpret_cast<TGetTimeZoneInformationForYear>(
      reinterpret_cast<void (*)()>(GetProcAddress(Kernel32, "GetTimeZoneInformationForYear")));
  ASSERT_NE(Function, nullptr);
  TIME_ZONE_INFORMATION Information;
  EXPECT_TRUE(Function(2024, NULL, &Information));

  wchar_t Path[MAX_PATH];
  DWORD Length = GetModuleFileName(NULL, Path, MAX_PATH);
  EXPECT_GT(Length, 0u);
  EXPECT_EQ(UnicodeString(Path), ParamStr(0));
}
//---------------------------------------------------------------------------
TEST(SystemTest, LoadString)
{
  static const rtl::TResourceString Strings[] = { { 999901, L"Test string" } };
  rtl::RegisterResourceStrings(Strings, 1);
  wchar_t Buffer[8];
  EXPECT_EQ(LoadString(HInstance, 999901, Buffer, 8), 7);
  EXPECT_STREQ(Buffer, L"Test st");
  EXPECT_EQ(LoadString(HInstance, 999902, Buffer, 8), 0);
}
//---------------------------------------------------------------------------
TEST(SystemTest, VersionInformation)
{
  UnicodeString Executable = ParamStr(0);
  unsigned long Handle = 1;
  DWORD Size = GetFileVersionInfoSize(Executable.c_str(), &Handle);
  ASSERT_GT(Size, 0u);
  EXPECT_EQ(Handle, 0ul);
  std::vector<char> Block(Size);
  ASSERT_TRUE(GetFileVersionInfo(Executable.c_str(), 0, Size, Block.data()));

  VS_FIXEDFILEINFO * Fixed;
  UINT Length;
  ASSERT_TRUE(VerQueryValue(Block.data(), L"\\", reinterpret_cast<void **>(&Fixed), &Length));
  EXPECT_EQ(Length, sizeof(VS_FIXEDFILEINFO));
  UnicodeString Version =
    IntToStr(static_cast<int>(HIWORD(Fixed->dwFileVersionMS))) + L"." + IntToStr(static_cast<int>(LOWORD(Fixed->dwFileVersionMS))) + L"." +
    IntToStr(static_cast<int>(HIWORD(Fixed->dwFileVersionLS))) + L"." + IntToStr(static_cast<int>(LOWORD(Fixed->dwFileVersionLS)));
  EXPECT_EQ(Version, LINSCP_CORE_VERSION);

  DWORD * Translation;
  ASSERT_TRUE(VerQueryValue(Block.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<void **>(&Translation), &Length));
  EXPECT_EQ(Length, 4u);
  EXPECT_EQ(*Translation, 0x04B00409u);

  wchar_t * Value;
  ASSERT_TRUE(VerQueryValue(Block.data(), L"\\StringFileInfo\\040904B0\\ProductName", reinterpret_cast<void **>(&Value), &Length));
  EXPECT_STREQ(Value, L"LinSCP");
  // In characters, with the terminating null
  EXPECT_EQ(Length, 7u);
  ASSERT_TRUE(VerQueryValue(Block.data(), L"\\StringFileInfo\\040904B0\\ProductVersion", reinterpret_cast<void **>(&Value), &Length));
  EXPECT_EQ(UnicodeString(Value), LINSCP_CORE_VERSION);
  EXPECT_FALSE(VerQueryValue(Block.data(), L"\\StringFileInfo\\040904B0\\Unknown", reinterpret_cast<void **>(&Value), &Length));

  // Other files have no version information
  EXPECT_EQ(GetFileVersionInfoSize(L"/bin/sh", &Handle), 0u);
}
//---------------------------------------------------------------------------
TEST(SystemTest, VirtualQuery)
{
  uintptr_t Address = 0;
  MEMORY_BASIC_INFORMATION Info;
  size_t Committed = 0;
  int Regions = 0;
  while (VirtualQuery(reinterpret_cast<LPCVOID>(Address), &Info, sizeof(Info)) == sizeof(Info))
  {
    if (Info.State == MEM_COMMIT)
    {
      Committed += Info.RegionSize;
    }
    Address += Info.RegionSize;
    Regions++;
  }
  EXPECT_GT(Regions, 2);
  EXPECT_GT(Committed, 0u);
  int Local = 0;
  ASSERT_EQ(VirtualQuery(&Local, &Info, sizeof(Info)), sizeof(Info));
  EXPECT_EQ(Info.State, static_cast<DWORD>(MEM_COMMIT));
  EXPECT_EQ(Info.Type, static_cast<DWORD>(MEM_PRIVATE));
}
//---------------------------------------------------------------------------
TEST(SystemTest, UserAndComputerNames)
{
  wchar_t Name[UNLEN + 1];
  unsigned long Size = std::size(Name);
  ASSERT_TRUE(GetUserNameEx(NameSamCompatible, Name, &Size));
  EXPECT_EQ(Size, wcslen(Name));
  EXPECT_GT(Size, 0ul);
  ULONG Small = 1;
  EXPECT_FALSE(GetUserNameEx(NameSamCompatible, Name, &Small));
  EXPECT_GT(Small, 1u);
  DWORD ComputerSize = UNLEN;
  EXPECT_TRUE(GetComputerName(Name, &ComputerSize));
}
//---------------------------------------------------------------------------
TEST(SystemTest, OperatingSystemVersion)
{
  EXPECT_GT(Win32MajorVersion(), 0);
  EXPECT_TRUE(CheckWin32Version(Win32MajorVersion(), Win32MinorVersion()));
  EXPECT_FALSE(CheckWin32Version(Win32MajorVersion() + 1));
  BOOL Wow64 = FALSE;
  EXPECT_TRUE(IsWow64Process(GetCurrentProcess(), &Wow64));
  EXPECT_EQ(Wow64, (sizeof(void *) == 8) ? TRUE : FALSE);
}
//---------------------------------------------------------------------------
TEST(ProcessTest, Snapshot)
{
  HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  ASSERT_NE(Snapshot, INVALID_HANDLE_VALUE);
  PROCESSENTRY32 Entry;
  Entry.dwSize = sizeof(Entry);
  bool Found = false;
  for (BOOL More = Process32First(Snapshot, &Entry); More; More = Process32Next(Snapshot, &Entry))
  {
    if (Entry.th32ProcessID == GetCurrentProcessId())
    {
      Found = true;
      EXPECT_EQ(Entry.th32ParentProcessID, static_cast<DWORD>(getppid()));
      EXPECT_GE(Entry.cntThreads, 1u);
      EXPECT_GT(wcslen(Entry.szExeFile), 0u);
    }
  }
  EXPECT_TRUE(Found);
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_NO_MORE_FILES));
  CloseHandle(Snapshot);
}
//---------------------------------------------------------------------------
TEST(ProcessTest, Modules)
{
  HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
  ASSERT_NE(Process, (HANDLE)NULL);
  wchar_t Name[MAX_PATH];
  DWORD Length = GetModuleFileNameEx(Process, NULL, Name, MAX_PATH);
  EXPECT_EQ(UnicodeString(Name, static_cast<int>(Length)), ParamStr(0));
  CloseHandle(Process);

  HMODULE Modules[256];
  DWORD Needed = 0;
  ASSERT_TRUE(EnumProcessModules(GetCurrentProcess(), Modules, sizeof(Modules), &Needed));
  // The executable and at least the C library
  EXPECT_GE(Needed / sizeof(HMODULE), 2u);
  ASSERT_GT(GetModuleFileNameEx(GetCurrentProcess(), Modules[1], Name, MAX_PATH), 0u);
  EXPECT_NE(UnicodeString(Name).Pos(L".so"), 0);

  EXPECT_EQ(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, 0x7FFFFFF0), (HANDLE)NULL);
}
//---------------------------------------------------------------------------
TEST(ComTest, ShellItemsAndClasses)
{
  EXPECT_EQ(CoInitialize(NULL), S_OK);
  IShellItem * Item = nullptr;
  ASSERT_EQ(SHCreateItemFromParsingName(L"/tmp/file.txt", nullptr, IID_PPV_ARGS(&Item)), S_OK);
  wchar_t * Name;
  ASSERT_EQ(Item->GetDisplayName(SIGDN_FILESYSPATH, &Name), S_OK);
  EXPECT_STREQ(Name, L"/tmp/file.txt");
  CoTaskMemFree(Name);
  IFileOperation * Unsupported = nullptr;
  EXPECT_EQ(Item->QueryInterface(IID_PPV_ARGS(&Unsupported)), E_NOINTERFACE);
  EXPECT_EQ(Item->Release(), 0u);

  IFileOperation * Operation = nullptr;
  EXPECT_EQ(CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&Operation)), REGDB_E_CLASSNOTREG);
  EXPECT_EQ(Operation, nullptr);
  EXPECT_THROW(OleCheck(REGDB_E_CLASSNOTREG), EOleSysError);
  OleCheck(S_OK);
}
//---------------------------------------------------------------------------
namespace
{
std::vector<unsigned char> SelfSignedCertificate()
{
  EVP_PKEY * Key = EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "P-256");
  X509 * Certificate = X509_new();
  X509_set_version(Certificate, 2);
  ASN1_INTEGER_set(X509_get_serialNumber(Certificate), 1);
  X509_gmtime_adj(X509_getm_notBefore(Certificate), -3600);
  X509_gmtime_adj(X509_getm_notAfter(Certificate), 3600);
  X509_set_pubkey(Certificate, Key);
  // A name of its own: OpenSSL 4 returns the certificate's name as const
  X509_NAME * Name = X509_NAME_new();
  X509_NAME_add_entry_by_txt(Name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("linscp.test"), -1, -1, 0);
  X509_set_subject_name(Certificate, Name);
  X509_set_issuer_name(Certificate, Name);
  X509_NAME_free(Name);
  X509_sign(Certificate, Key, EVP_sha256());
  unsigned char * Data = nullptr;
  int Length = i2d_X509(Certificate, &Data);
  std::vector<unsigned char> Result(Data, Data + Length);
  OPENSSL_free(Data);
  X509_free(Certificate);
  EVP_PKEY_free(Key);
  return Result;
}
} // namespace
//---------------------------------------------------------------------------
TEST(CryptTest, UntrustedCertificate)
{
  std::vector<unsigned char> Certificate = SelfSignedCertificate();
  PCCERT_CONTEXT Context =
    CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, Certificate.data(), static_cast<DWORD>(Certificate.size()));
  ASSERT_NE(Context, nullptr);
  EXPECT_EQ(Context->cbCertEncoded, Certificate.size());

  CERT_CHAIN_ENGINE_CONFIG Config = {};
  Config.cbSize = RTL_SIZEOF_THROUGH_FIELD(CERT_CHAIN_ENGINE_CONFIG, CycleDetectionModulus);
  // The layout of 64-bit Windows, which the core checks
  EXPECT_EQ(Config.cbSize, (sizeof(void *) == 8) ? 64u : 40u);
  HCERTCHAINENGINE Engine;
  ASSERT_TRUE(CertCreateCertificateChainEngine(&Config, &Engine));
  CERT_CHAIN_PARA ChainPara = {};
  ChainPara.cbSize = sizeof(ChainPara);
  PCCERT_CHAIN_CONTEXT Chain = NULL;
  ASSERT_TRUE(CertGetCertificateChain(Engine, Context, NULL, NULL, &ChainPara, 0, NULL, &Chain));
  CERT_CHAIN_POLICY_PARA PolicyPara = { sizeof(PolicyPara), 0, NULL };
  CERT_CHAIN_POLICY_STATUS Status = {};
  Status.cbSize = sizeof(Status);
  ASSERT_TRUE(CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, Chain, &PolicyPara, &Status));
  EXPECT_EQ(Status.dwError, static_cast<DWORD>(CERT_E_UNTRUSTEDROOT));
  EXPECT_EQ(Status.lElementIndex, 0);
  CertFreeCertificateChain(Chain);
  CertFreeCertificateChainEngine(Engine);
  CertFreeCertificateContext(Context);

  const BYTE Garbage[] = { 1, 2, 3 };
  EXPECT_EQ(CertCreateCertificateContext(X509_ASN_ENCODING, Garbage, sizeof(Garbage)), nullptr);
}
//---------------------------------------------------------------------------
TEST(RegistryTest, EmptyRegistry)
{
  HKEY Key;
  EXPECT_EQ(RegOpenKeyEx(HKEY_CURRENT_USER, L"Software", 0, KEY_READ, &Key), ERROR_FILE_NOT_FOUND);
  unsigned long Type, Size = 0;
  EXPECT_EQ(RegQueryValueEx(HKEY_CURRENT_USER, L"Value", NULL, &Type, NULL, &Size), ERROR_FILE_NOT_FOUND);
  EXPECT_NE(RegSetValueEx(HKEY_CURRENT_USER, L"Value", 0, REG_SZ, NULL, 0), ERROR_SUCCESS);
}
//---------------------------------------------------------------------------
