// The private keys and certificates of the sites, the proxy of the
// environment (upstream's windows/Tools.cpp).
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <CoreMain.h>
#include <PuttyTools.h>
#include <QtInterface.h>
#include <TextsWin.h>

#include <QApplication>
#include <QFileDialog>
#include <QUrl>

#include <cerrno>
//---------------------------------------------------------------------------
UnicodeString GetConvertedKeyFileName(const UnicodeString & FileName)
{
  return ChangeFileExt(FileName, FORMAT(L".%s", (PuttyKeyExt)));
}
//---------------------------------------------------------------------------
static bool TryAddMatchingKeyCertificate(
  TPrivateKey * PrivateKey, const UnicodeString & CertificateFileName, UnicodeString & Result)
{
  if (FileExists(CertificateFileName))
  {
    try
    {
      AddCertificateToKey(PrivateKey, CertificateFileName);
      Result = CertificateFileName;
    }
    catch (Exception & E)
    {
      AppLogFmt(L"Cannot add certificate from auto-detected \"%s\": %s", (CertificateFileName, E.Message));
    }
  }

  return !Result.IsEmpty();
}
//---------------------------------------------------------------------------
UnicodeString AddMatchingKeyCertificate(TPrivateKey * PrivateKey, const UnicodeString & FileName)
{
  UnicodeString CertificateFileName = FileName;
  UnicodeString S = FORMAT(L".%s", (PuttyKeyExt));
  if (EndsText(S, CertificateFileName))
  {
    CertificateFileName.SetLength(CertificateFileName.Length() - S.Length());
  }

  UnicodeString Result;

  TryAddMatchingKeyCertificate(PrivateKey, CertificateFileName + L"-cert.pub", Result) ||
  TryAddMatchingKeyCertificate(PrivateKey, CertificateFileName + L".pub-aadcert.pub", Result);

  return Result;
}
//---------------------------------------------------------------------------
static void ConvertKey(UnicodeString & FileName, TKeyType Type)
{
  UnicodeString Passphrase;

  UnicodeString Comment;
  if (IsKeyEncrypted(Type, FileName, Comment))
  {
    if (!InputDialog(
          LoadStr(PASSPHRASE_TITLE),
          FORMAT(LoadStr(PROMPT_KEY_PASSPHRASE), (Comment)),
          Passphrase, NULL, false, false))
    {
      Abort();
    }
  }

  TPrivateKey * PrivateKey = LoadKey(Type, FileName, Passphrase);
  ::rtl::TFinally Finally([&]() { FreeKey(PrivateKey); });

  AppLogFmt(L"Loaded key from \"%s\".", (FileName));

  UnicodeString CertificateMessage;
  UnicodeString CertificateFileName = AddMatchingKeyCertificate(PrivateKey, FileName);
  if (!CertificateFileName.IsEmpty())
  {
    AppLogFmt(L"Added certificate from auto-detected \"%s\".", (CertificateFileName));
    CertificateMessage = L"\n" + FMTLOAD(CERTIFICATE_ADDED, (CertificateFileName));
  }

  FileName = GetConvertedKeyFileName(FileName);

  QFileDialog Dialog(QApplication::activeWindow(), ToQString(LoadStr(CONVERTKEY_SAVE_TITLE)), ToQString(FileName),
    FileDialogFilter(LoadStr(CONVERTKEY_SAVE_FILTER)));
  Dialog.setObjectName(QStringLiteral("SaveKeyDialog"));
  Dialog.setAcceptMode(QFileDialog::AcceptSave);
  Dialog.setDefaultSuffix(ToQString(PuttyKeyExt));
  Dialog.selectFile(ToQString(FileName));
  if ((Dialog.exec() != QDialog::Accepted) || Dialog.selectedFiles().isEmpty())
  {
    Abort();
  }
  FileName = ToUnicodeString(Dialog.selectedFiles().first());

  SaveKey(ktSSH2, FileName, Passphrase, PrivateKey);
  AppLogFmt(L"Saved converted key to \"%s\".", (FileName));

  UnicodeString Message =
    MainInstructions(FMTLOAD(CONVERTKEY_SAVED, (FileName))) +
    CertificateMessage;
  MessageDialog(Message, qtInformation, qaOK);
}
//---------------------------------------------------------------------------
static void DoVerifyKey(UnicodeString & FileName, bool Convert, UnicodeString & Message, TStrings *& MoreMessages)
{
  std::unique_ptr<TStrings> AMoreMessages;
  if (!FileName.Trim().IsEmpty())
  {
    FileName = ExpandEnvironmentVariables(FileName);
    TKeyType Type = KeyType(FileName);
    // reason fopen failed
    int Error = errno;
    switch (Type)
    {
      case ktOpenSSHPEM:
      case ktOpenSSHNew:
      case ktSSHCom:
        {
          UnicodeString TypeName = ((Type == ktOpenSSHPEM) || (Type == ktOpenSSHNew)) ? L"OpenSSH" : L"ssh.com";
          Message = FMTLOAD(KEY_TYPE_UNSUPPORTED2, (FileName, TypeName));

          if (Convert)
          {
            Configuration->Usage->Inc(L"PrivateKeyConvertSuggestionsNative");
            UnicodeString ConvertMessage = FMTLOAD(KEY_TYPE_CONVERT4, (TypeName, RemoveMainInstructionsTag(Message)));
            Message = EmptyStr;
            if (MoreMessageDialog(ConvertMessage, NULL, qtConfirmation, qaOK | qaCancel) == qaOK)
            {
              ConvertKey(FileName, Type);
              Configuration->Usage->Inc(L"PrivateKeyConverted");
            }
            else
            {
              Abort();
            }
          }
        }
        break;

      case ktSSH1:
        Message = MainInstructions(FMTLOAD(KEY_TYPE_SSH1, (FileName)));
        break;

      case ktSSH2:
        Message = TestKey(Type, FileName);
        break;

      case ktSSH1Public:
      case ktSSH2PublicRFC4716:
      case ktSSH2PublicOpenSSH:
        // noop
        break;

      case ktUnopenable:
        Message = MainInstructions(FMTLOAD(KEY_TYPE_UNOPENABLE, (FileName)));
        if (Error != 0)
        {
          AMoreMessages.reset(TextToStringList(SysErrorMessageForError(Error)));
        }
        break;

      default:
        DebugFail();
        // fallthru
      case ktUnknown:
        Message = MainInstructions(FMTLOAD(KEY_TYPE_UNKNOWN2, (FileName)));
        break;
    }
  }
  MoreMessages = AMoreMessages.release();
}
//---------------------------------------------------------------------------
static void DoVerifyKey(UnicodeString & FileName, bool Convert, bool CanIgnore)
{
  TStrings * AMoreMessages;
  UnicodeString Message;
  DoVerifyKey(FileName, Convert, Message, AMoreMessages);
  std::unique_ptr<TStrings> MoreMessages(AMoreMessages);
  if (!Message.IsEmpty())
  {
    Configuration->Usage->Inc(L"PrivateKeySelectErrors");
    unsigned int Answers = (CanIgnore ? (qaIgnore | qaAbort) : qaOK);
    if (MoreMessageDialog(Message, MoreMessages.get(), qtWarning, Answers) != qaIgnore)
    {
      Abort();
    }
  }
}
//---------------------------------------------------------------------------
void VerifyAndConvertKey(UnicodeString & FileName, bool CanIgnore)
{
  DoVerifyKey(FileName, true, CanIgnore);
}
//---------------------------------------------------------------------------
void VerifyKey(const UnicodeString & FileName)
{
  UnicodeString AFileName(FileName);
  DoVerifyKey(AFileName, false, true);
}
//---------------------------------------------------------------------------
void VerifyCertificate(const UnicodeString & FileName)
{
  if (!FileName.Trim().IsEmpty())
  {
    try
    {
      CheckCertificate(FileName);
    }
    catch (Exception & E)
    {
      if (ExceptionMessageDialog(&E, qtWarning, L"", qaIgnore | qaAbort) == qaAbort)
      {
        Abort();
      }
    }
  }
}
//---------------------------------------------------------------------------
bool AutodetectProxy(UnicodeString & HostName, int & PortNumber)
{
  // The proxy of the environment, as the applications of the desktop
  // use it (upstream reads the system's one of Windows)
  bool Result = false;
  for (const char * Name : { "https_proxy", "HTTPS_PROXY", "http_proxy", "HTTP_PROXY", "all_proxy", "ALL_PROXY" })
  {
    QString Proxy = QString::fromLocal8Bit(qgetenv(Name)).trimmed();
    if (!Proxy.isEmpty())
    {
      if (!Proxy.contains(QStringLiteral("://")))
      {
        Proxy = QStringLiteral("http://") + Proxy;
      }
      QUrl Url(Proxy);
      if (Url.isValid() && !Url.host().isEmpty())
      {
        HostName = ToUnicodeString(Url.host());
        PortNumber = Url.port(ProxyPortNumber);
        AppLogFmt(L"Proxy autodetected: %s:%d", (HostName, PortNumber));
        Result = true;
        break;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
