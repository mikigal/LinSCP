//---------------------------------------------------------------------------
// TFileZillaIntf without the FileZilla engine: Init fails, so an FTP session
// does not open, and nothing else is reached. Temporary, until the engine is
// ported (milestone M4).
//---------------------------------------------------------------------------
#include <vcl.h>

#include <FileBuffer.h>
#include <FileZillaIntf.h>
//---------------------------------------------------------------------------
void __fastcall TFileZillaIntf::Initialize()
{
  // noop
}
//---------------------------------------------------------------------------
void __fastcall TFileZillaIntf::Finalize()
{
  // noop
}
//---------------------------------------------------------------------------
__fastcall TFileZillaIntf::TFileZillaIntf() :
  FFileZillaApi(NULL),
  FIntern(NULL),
  FServer(NULL)
{
}
//---------------------------------------------------------------------------
__fastcall TFileZillaIntf::~TFileZillaIntf()
{
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::Init()
{
  throw Exception(L"The FTP protocol is not supported yet.");
}
//---------------------------------------------------------------------------
void __fastcall TFileZillaIntf::Destroying()
{
  // noop
}
//---------------------------------------------------------------------------
// Not reached, as Init fails
bool __fastcall TFileZillaIntf::SetCurrentPath(const wchar_t *)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::GetCurrentPath(wchar_t *, size_t)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::UsingMlsd()
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::UsingUtf8()
{
  return false;
}
//---------------------------------------------------------------------------
std::string __fastcall TFileZillaIntf::GetTlsVersionStr()
{
  return std::string();
}
//---------------------------------------------------------------------------
std::string __fastcall TFileZillaIntf::GetCipherName()
{
  return std::string();
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::Cancel()
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::Connect(const wchar_t *, int, const wchar_t *,
  const wchar_t *, const wchar_t *, const wchar_t *, int, int, int, int, int, int,
  X509 *, EVP_PKEY *)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::Close(bool)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::List(const wchar_t *)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::ListFile(const wchar_t *, const wchar_t *)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::CustomCommand(const wchar_t *)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::MakeDir(const wchar_t *)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::Chmod(int, const wchar_t *, const wchar_t *)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::Delete(const wchar_t *, const wchar_t *, bool)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::RemoveDir(const wchar_t *, const wchar_t *)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::Rename(const wchar_t *, const wchar_t *,
  const wchar_t *, const wchar_t *)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::FileTransfer(
  const wchar_t *, const wchar_t *, const wchar_t *, bool, __int64, int, void *,
  TTransferOutEvent, TTransferInEvent)
{
  return false;
}
//---------------------------------------------------------------------------
void __fastcall TFileZillaIntf::SetDebugLevel(TLogLevel)
{
  // noop
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::HandleMessage(WPARAM, LPARAM)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::PostMessage(WPARAM, LPARAM)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TFileZillaIntf::CheckError(int, const wchar_t *)
{
  return false;
}
//---------------------------------------------------------------------------
ftp_capabilities_t TFTPServerCapabilities::GetCapability(ftp_capability_names_t Name)
{
  return FCapabilityMap[Name].cap;
}
//---------------------------------------------------------------------------
ftp_capabilities_t TFTPServerCapabilities::GetCapabilityString(ftp_capability_names_t Name, std::string * Option)
{
  t_cap & Cap = FCapabilityMap[Name];
  if (Option != NULL)
  {
    *Option = Cap.option;
  }
  return Cap.cap;
}
//---------------------------------------------------------------------------
void TFTPServerCapabilities::SetCapability(ftp_capability_names_t Name, ftp_capabilities_t Cap)
{
  t_cap & Entry = FCapabilityMap[Name];
  Entry.cap = Cap;
  Entry.number = 1;
}
//---------------------------------------------------------------------------
void TFTPServerCapabilities::SetCapability(ftp_capability_names_t Name, ftp_capabilities_t Cap, const std::string & Option)
{
  t_cap & Entry = FCapabilityMap[Name];
  Entry.cap = Cap;
  Entry.option = Option;
  Entry.number = 0;
}
//---------------------------------------------------------------------------
