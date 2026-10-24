//---------------------------------------------------------------------------
#ifndef SecureShellH
#define SecureShellH

#include <set>
#include "Configuration.h"
#include "SessionData.h"
#include "SessionInfo.h"
//---------------------------------------------------------------------------
#ifndef PuttyIntfH
struct Backend_vtable;
struct Backend;
struct Conf;
#endif
//---------------------------------------------------------------------------
struct _WSANETWORKEVENTS;
typedef struct _WSANETWORKEVENTS WSANETWORKEVENTS;
typedef UINT_PTR SOCKET;
typedef std::set<SOCKET> TSockets;
struct TPuttyTranslation;
struct callback_set;
enum TSshImplementation : int { sshiUnknown, sshiOpenSSH, sshiProFTPD, sshiBitvise, sshiTitan, sshiOpenVMS, sshiCerberus };
struct ScpLogPolicy;
struct LogContext;
struct ScpSeat;
enum TSecureShellMode : int { ssmNone, ssmUploading, ssmDownloading };
//---------------------------------------------------------------------------
class TSecureShell
{
friend class TPoolForDataEvent;

private:
  SOCKET FSocket;
  HANDLE FSocketEvent;
  TSockets FPortFwdSockets;
  TSessionUI * FUI;
  TSessionData * FSessionData;
  bool FActive;
  TSessionInfo FSessionInfo;
  bool FSessionInfoValid;
  TDateTime FLastDataSent;
  Backend * FBackendHandle;
  TNotifyEvent FOnReceive;
  bool FFrozen;
  bool FDataWhileFrozen;
  bool FStoredPasswordTried;
  bool FStoredPasswordTriedForKI;
  bool FStoredPassphraseTried;
  bool FAuthenticationCancelled;
  bool FOpened;
  bool FClosed;
  int FWaiting;
  bool FSimple;
  bool FNoConnectionResponse;
  bool FCollectPrivateKeyUsage;
  int FWaitingForData;
  TSshImplementation FSshImplementation;

  unsigned PendLen;
  unsigned PendSize;
  unsigned OutLen;
  unsigned char * OutPtr;
  unsigned char * Pending;
  TSessionLog * FLog;
  TConfiguration * FConfiguration;
  bool FAuthenticating;
  bool FAuthenticated;
  UnicodeString FStdErrorTemp;
  UnicodeString FStdError;
  UnicodeString FCWriteTemp;
  UnicodeString FAuthenticationLog;
  UnicodeString FLastTunnelError;
  UnicodeString FUserName;
  bool FUtfStrings;
  DWORD FLastSendBufferUpdate;
  int FSendBuf;
  callback_set * FCallbackSet;
  ScpLogPolicy * FLogPolicy;
  ScpSeat * FSeat;
  LogContext * FLogCtx;
  std::set<UnicodeString> FLoggedKnownHostKeys;

  void __fastcall Init();
  void __fastcall SetActive(bool value);
  void inline __fastcall CheckConnection(int Message = -1);
  void __fastcall WaitForData();
  void __fastcall Discard();
  void __fastcall FreeBackend();
  void __fastcall PoolForData(WSANETWORKEVENTS & Events, unsigned int & Result);
  inline void __fastcall CaptureOutput(TLogLineType Type,
    const UnicodeString & Line);
  void __fastcall ResetConnection();
  void __fastcall ResetSessionInfo();
  void __fastcall SocketEventSelect(SOCKET Socket, HANDLE Event, bool Enable);
  bool __fastcall EnumNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events);
  void __fastcall HandleNetworkEvents(SOCKET Socket, WSANETWORKEVENTS & Events);
  bool __fastcall ProcessNetworkEvents(SOCKET Socket);
  bool __fastcall EventSelectLoop(unsigned int MSec, bool ReadEventRequired,
    WSANETWORKEVENTS * Events);
  void __fastcall UpdateSessionInfo();
  bool __fastcall GetReady();
  void __fastcall DispatchSendBuffer(int BufSize);
  void __fastcall SendBuffer(unsigned int & Result);
  unsigned int __fastcall TimeoutPrompt(TQueryParamsTimerEvent PoolEvent);
  void TimeoutAbort(unsigned int Answer, bool Sending);
  bool __fastcall TryFtp();
  UnicodeString __fastcall ConvertInput(const RawByteString & Input);
  void __fastcall GetRealHost(UnicodeString & Host, int & Port);
  UnicodeString __fastcall RetrieveHostKey(const UnicodeString & Host, int Port, const UnicodeString & KeyType);
  bool HaveAcceptNewHostKeyPolicy();
  THierarchicalStorage * GetHostKeyStorage();
  bool VerifyCachedHostKey(
    const UnicodeString & StoredKeys, const UnicodeString & KeyStr, const UnicodeString & FingerprintMD5, const UnicodeString & FingerprintSHA256);
  UnicodeString StoreHostKey(
    const UnicodeString & Host, int Port, const UnicodeString & KeyType, const UnicodeString & KeyStr);
  bool HasLocalProxy();

protected:
  TCaptureOutputEvent FOnCaptureOutput;

  void __fastcall GotHostKey();
  int __fastcall TranslatePuttyMessage(const TPuttyTranslation * Translation,
    size_t Count, UnicodeString & Message, UnicodeString * HelpKeyword = NULL);
  int __fastcall TranslateAuthenticationMessage(UnicodeString & Message, UnicodeString * HelpKeyword = NULL);
  int __fastcall TranslateErrorMessage(UnicodeString & Message, UnicodeString * HelpKeyword = NULL);
  void __fastcall AddStdErrorLine(const UnicodeString & Str);
  void __fastcall inline LogEvent(const UnicodeString & Str);
  void __fastcall FatalError(UnicodeString Error, UnicodeString HelpKeyword = L"");
  UnicodeString __fastcall FormatKeyStr(UnicodeString KeyStr);
  void ParseFingerprint(const UnicodeString & Fingerprint, UnicodeString & SignKeyType, UnicodeString & Hash);
  static Conf * __fastcall StoreToConfig(TSessionData * Data, bool Simple);
  TPromptKind __fastcall IdentifyPromptKind(UnicodeString & Name);

public:
  __fastcall TSecureShell(TSessionUI * UI, TSessionData * SessionData,
    TSessionLog * Log, TConfiguration * Configuration);
  __fastcall ~TSecureShell();
  void __fastcall Open();
  void __fastcall Close();
  void __fastcall KeepAlive();
  int __fastcall Receive(unsigned char * Buf, int Len);
  bool __fastcall Peek(unsigned char *& Buf, int Len);
  UnicodeString __fastcall ReceiveLine();
  void __fastcall Send(const unsigned char * Buf, int Len);
  void __fastcall SendSpecial(int Code);
  void __fastcall Idle(unsigned int MSec = 0);
  void __fastcall SendLine(const UnicodeString & Line);
  void __fastcall SendNull();

  const TSessionInfo & __fastcall GetSessionInfo();
  void __fastcall GetHostKeyFingerprint(UnicodeString & SHA256, UnicodeString & MD5);
  bool __fastcall SshFallbackCmd() const;
  unsigned long __fastcall MaxPacketSize();
  void __fastcall ClearStdError();
  bool __fastcall GetStoredCredentialsTried();
  void __fastcall CollectUsage();
  bool __fastcall CanChangePassword();

  void __fastcall RegisterReceiveHandler(TNotifyEvent Handler);
  void __fastcall UnregisterReceiveHandler(TNotifyEvent Handler);

  // interface to PuTTY core
  void __fastcall UpdateSocket(SOCKET value, bool Enable);
  void __fastcall UpdatePortFwdSocket(SOCKET value, bool Enable);
  void __fastcall PuttyFatalError(UnicodeString Error);
  bool __fastcall PromptUser(bool ToServer,
    UnicodeString AName, bool NameRequired,
    UnicodeString Instructions, bool InstructionsRequired,
    TStrings * Prompts, TStrings * Results);
  void __fastcall FromBackend(const unsigned char * Data, size_t Length);
  void __fastcall CWrite(const char * Data, size_t Length);
  void __fastcall AddStdError(const char * Data, size_t Length);
  const UnicodeString & __fastcall GetStdError();
  void __fastcall VerifyHostKey(
    const UnicodeString & Host, int Port, const UnicodeString & KeyType, const UnicodeString & KeyStr,
    const UnicodeString & FingerprintSHA256, const UnicodeString & FingerprintMD5,
    bool IsCertificate, int CACount, bool AlreadyVerified);
  bool __fastcall HaveHostKey(UnicodeString Host, int Port, const UnicodeString KeyType);
  void AskAlg(const UnicodeString & AlgType, const UnicodeString & AlgName, int WeakCryptoReason);
  void __fastcall DisplayBanner(const UnicodeString & Banner);
  void __fastcall PuttyLogEvent(const char * Str);
  UnicodeString ConvertFromPutty(const char * Str, size_t Length);
  struct callback_set * GetCallbackSet();

  bool __property_get_Active() const { return FActive; } __declspec(property(get = __property_get_Active)) bool Active;
  bool __property_get_Ready() const { return ::rtl::Mutable(this)->GetReady(); } __declspec(property(get = __property_get_Ready)) bool Ready;
  TCaptureOutputEvent __property_get_OnCaptureOutput() const { return FOnCaptureOutput; } void __property_set_OnCaptureOutput(TCaptureOutputEvent Value) { FOnCaptureOutput = Value; } __declspec(property(get = __property_get_OnCaptureOutput, put = __property_set_OnCaptureOutput)) TCaptureOutputEvent OnCaptureOutput;
  TDateTime __property_get_LastDataSent() const { return FLastDataSent; } __declspec(property(get = __property_get_LastDataSent)) TDateTime LastDataSent;
  UnicodeString __property_get_LastTunnelError() const { return FLastTunnelError; } __declspec(property(get = __property_get_LastTunnelError)) UnicodeString LastTunnelError;
  UnicodeString __property_get_UserName() const { return FUserName; } __declspec(property(get = __property_get_UserName)) UnicodeString UserName;
  bool __property_get_Simple() const { return FSimple; } void __property_set_Simple(bool Value) { FSimple = Value; } __declspec(property(get = __property_get_Simple, put = __property_set_Simple)) bool Simple;
  TSshImplementation __property_get_SshImplementation() const { return FSshImplementation; } __declspec(property(get = __property_get_SshImplementation)) TSshImplementation SshImplementation;
  bool __property_get_UtfStrings() const { return FUtfStrings; } void __property_set_UtfStrings(bool Value) { FUtfStrings = Value; } __declspec(property(get = __property_get_UtfStrings, put = __property_set_UtfStrings)) bool UtfStrings;
  TSecureShellMode Mode;
};
//---------------------------------------------------------------------------
#endif
