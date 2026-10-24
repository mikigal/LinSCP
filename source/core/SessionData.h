//---------------------------------------------------------------------------
#ifndef SessionDataH
#define SessionDataH

#include "Common.h"
#include "Option.h"
#include "FileBuffer.h"
#include "NamedObjs.h"
#include "HierarchicalStorage.h"
#include "Configuration.h"
#include <Xml.XMLIntf.hpp>
//---------------------------------------------------------------------------
enum TCipher { cipWarn, cip3DES, cipBlowfish, cipAES, cipDES, cipArcfour, cipChaCha20, cipAESGCM, cipCount };
#define CIPHER_COUNT (cipCount)
// explicit values to skip obsoleted fsExternalSSH, fsExternalSFTP
enum TFSProtocol { fsSCPonly = 0, fsSFTP = 1, fsSFTPonly = 2, fsFTP = 5, fsWebDAV = 6, fsS3 = 7 };
#define FSPROTOCOL_COUNT (fsS3+1)
extern const wchar_t * ProxyMethodNames;
enum TProxyMethod { pmNone, pmSocks4, pmSocks5, pmHTTP, pmTelnet, pmCmd, pmSshTcpIp };
enum TKex { kexWarn, kexDHGroup1, kexDHGroup14, kexDHGroup15, kexDHGroup16, kexDHGroup17, kexDHGroup18, kexDHGEx, kexRSA, kexECDH, kexNTRUHybrid, kexMLKEM25519Hybrid, kexMLKEMNISTHybrid, kexCount };
#define KEX_COUNT (kexCount)
enum THostKey { hkWarn, hkRSA, hkDSA, hkECDSA, hkED25519, hkED448, hkCount };
#define HOSTKEY_COUNT (hkCount)
enum TGssLib { gssGssApi32, gssSspi, gssCustom };
#define GSSLIB_COUNT (gssCustom+1)
// names have to match PuTTY registry entries (see settings.c)
enum TSshBug { sbHMAC2, sbDeriveKey2, sbRSAPad2,
  sbPKSessID2, sbRekey2, sbMaxPkt2, sbIgnore2, sbOldGex2, sbWinAdj, sbChanReq };
#define BUG_COUNT (sbChanReq+1)
enum TSftpBug { sbSymlink, sbSignedTS };
#define SFTP_BUG_COUNT (sbSignedTS+1)
extern const wchar_t * PingTypeNames;
enum TPingType { ptOff, ptNullPacket, ptDummyCommand };
extern const wchar_t * FtpPingTypeNames;
enum TFtpPingType { fptOff, fptDummyCommand0, fptDummyCommand, fptDirectoryListing };
enum TAddressFamily { afAuto, afIPv4, afIPv6 };
enum TFtps { ftpsNone, ftpsImplicit, ftpsExplicitSsl, ftpsExplicitTls };
// ssl2 and ssh3 are equivalent of tls10 now
enum TTlsVersion { ssl2 = 2, ssl3 = 3, tls10 = 10, tls11 = 11, tls12 = 12, tls13 = 13, tlsMin = tls10, tlsDefaultMin = tls12, tlsMax = tls13 };
// has to match libs3 S3UriStyle
enum TS3UrlStyle { s3usVirtualHost, s3usPath };
enum TSessionSource { ssNone, ssStored, ssStoredModified };
const int SFTPMaxVersionAuto = -1;
enum TSessionUrlFlags
{
  sufSpecific = 0x01,
  sufUserName = 0x02,
  sufPassword = 0x04,
  sufHostKey = 0x08,
  sufRawSettings = 0x10,
  sufHttpForWebDAV = 0x20,
  sufSession = sufUserName | sufPassword | sufHostKey,
  sufComplete = sufSession | sufRawSettings,
  sufOpen = sufUserName | sufPassword
};
enum TParseUrlFlags
{
  pufAllowStoredSiteWithProtocol = 0x01,
  pufUnsafe = 0x02,
  pufPreferProtocol = 0x04,
  pufParseOnly = 0x08,
};
enum TParsedInfoFlags
{
  piDefaultsOnly = 0x01,
  piProtocolDefined = 0x02,
  piUnsafeSettings = 0x04,
};
//---------------------------------------------------------------------------
extern const UnicodeString CipherNames[CIPHER_COUNT];
extern const UnicodeString KexNames[KEX_COUNT];
extern const UnicodeString HostKeyNames[HOSTKEY_COUNT];
extern const UnicodeString GssLibNames[GSSLIB_COUNT];
extern const wchar_t SshProtList[][10];
extern const TCipher DefaultCipherList[CIPHER_COUNT];
extern const TKex DefaultKexList[KEX_COUNT];
extern const THostKey DefaultHostKeyList[HOSTKEY_COUNT];
extern const TGssLib DefaultGssLibList[GSSLIB_COUNT];
extern const wchar_t FSProtocolNames[FSPROTOCOL_COUNT][16];
extern const int DefaultSendBuf;
extern const UnicodeString AnonymousUserName;
extern const UnicodeString AnonymousPassword;
extern const int SshPortNumber;
extern const int FtpPortNumber;
extern const int FtpsImplicitPortNumber;
extern const int HTTPPortNumber;
extern const int HTTPSPortNumber;
extern const int TelnetPortNumber;
extern const int ProxyPortNumber;
extern const UnicodeString PuttySshProtocol;
extern const UnicodeString PuttyTelnetProtocol;
extern const UnicodeString SftpProtocol;
extern const UnicodeString ScpProtocol;
extern const UnicodeString FtpProtocol;
extern const UnicodeString FtpsProtocol;
extern const UnicodeString FtpesProtocol;
extern const UnicodeString WebDAVProtocol;
extern const UnicodeString WebDAVSProtocol;
extern const UnicodeString S3Protocol;
extern const UnicodeString SshProtocol;
extern const UnicodeString WinSCPProtocolPrefix;
extern const wchar_t UrlParamSeparator;
extern const wchar_t UrlParamValueSeparator;
extern const UnicodeString UrlHostKeyParamName;
extern const UnicodeString UrlSaveParamName;
extern const UnicodeString PassphraseOption;
extern const UnicodeString S3HostName;
extern const UnicodeString S3GoogleCloudHostName;
//---------------------------------------------------------------------------
class TStoredSessionList;
//---------------------------------------------------------------------------
class TSessionData : public TNamedObject
{
friend class TStoredSessionList;

private:
  UnicodeString FHostName;
  int FPortNumber;
  UnicodeString FUserName;
  RawByteString FPassword;
  RawByteString FNewPassword;
  bool FChangePassword;
  int FPingInterval;
  TPingType FPingType;
  bool FTryAgent;
  bool FAgentFwd;
  UnicodeString FListingCommand;
  bool FAuthKI;
  bool FAuthKIPassword;
  bool FAuthGSSAPI;
  bool FAuthGSSAPIKEX;
  bool FGSSAPIFwdTGT;
  bool FChangeUsername;
  bool FCompression;
  bool FSsh2DES;
  bool FSshNoUserAuth;
  TCipher FCiphers[CIPHER_COUNT];
  TKex FKex[KEX_COUNT];
  THostKey FHostKeys[HOSTKEY_COUNT];
  TGssLib FGssLib[GSSLIB_COUNT];
  UnicodeString FGssLibCustom;
  bool FClearAliases;
  TEOLType FEOLType;
  bool FTrimVMSVersions;
  bool FVMSAllRevisions;
  UnicodeString FPublicKeyFile;
  UnicodeString FPassphrase;
  UnicodeString FDetachedCertificate;
  UnicodeString FPuttyProtocol;
  TFSProtocol FFSProtocol;
  bool FModified;
  UnicodeString FLocalDirectory;
  UnicodeString FRemoteDirectory;
  UnicodeString FOtherLocalDirectory;
  bool FSpecial;
  bool FSynchronizeBrowsing;
  bool FUpdateDirectories;
  bool FRequireDirectories;
  bool FCacheDirectories;
  bool FCacheDirectoryChanges;
  bool FPreserveDirectoryChanges;
  bool FSelected;
  TAutoSwitch FLookupUserGroups;
  UnicodeString FReturnVar;
  bool FExitCode1IsError;
  bool FScp1Compatibility;
  UnicodeString FShell;
  UnicodeString FSftpServer;
  int FTimeout;
  bool FUnsetNationalVars;
  bool FIgnoreLsWarnings;
  bool FTcpNoDelay;
  int FSendBuf;
  UnicodeString FSourceAddress;
  UnicodeString FProtocolFeatures;
  bool FSshSimple;
  TProxyMethod FProxyMethod;
  UnicodeString FProxyHost;
  int FProxyPort;
  UnicodeString FProxyUsername;
  RawByteString FProxyPassword;
  UnicodeString FProxyTelnetCommand;
  UnicodeString FProxyLocalCommand;
  TAutoSwitch FProxyDNS;
  bool FProxyLocalhost;
  int FFtpProxyLogonType;
  TAutoSwitch FBugs[BUG_COUNT];
  UnicodeString FPuttySettings;
  UnicodeString FCustomParam1;
  UnicodeString FCustomParam2;
  bool FResolveSymlinks;
  bool FFollowDirectorySymlinks;
  TDateTime FTimeDifference;
  bool FTimeDifferenceAuto;
  int FSFTPDownloadQueue;
  int FSFTPUploadQueue;
  int FSFTPListingQueue;
  int FSFTPMaxVersion;
  unsigned long FSFTPMaxPacketSize;
  TAutoSwitch FSFTPRealPath;
  bool FUsePosixRename;
  TDSTMode FDSTMode;
  TAutoSwitch FSFTPBugs[SFTP_BUG_COUNT];
  bool FDeleteToRecycleBin;
  bool FOverwrittenToRecycleBin;
  UnicodeString FRecycleBinPath;
  UnicodeString FPostLoginCommands;
  TAutoSwitch FSCPLsFullTime;
  TAutoSwitch FFtpListAll;
  TAutoSwitch FFtpHost;
  TAutoSwitch FFtpWorkFromCwd;
  bool FFtpAnyCodeForPwd;
  bool FSslSessionReuse;
  UnicodeString FTlsCertificateFile;
  TAddressFamily FAddressFamily;
  UnicodeString FRekeyData;
  unsigned int FRekeyTime;
  int FColor;
  bool FTunnel;
  UnicodeString FTunnelHostName;
  int FTunnelPortNumber;
  UnicodeString FTunnelUserName;
  RawByteString FTunnelPassword;
  UnicodeString FTunnelPublicKeyFile;
  RawByteString FTunnelPassphrase;
  int FTunnelLocalPortNumber;
  UnicodeString FTunnelPortFwd;
  UnicodeString FTunnelHostKey;
  bool FFtpPasvMode;
  TAutoSwitch FFtpForcePasvIp;
  TAutoSwitch FFtpUseMlsd;
  UnicodeString FFtpAccount;
  int FFtpPingInterval;
  TFtpPingType FFtpPingType;
  TAutoSwitch FFtpTransferActiveImmediately;
  TFtps FFtps;
  TTlsVersion FMinTlsVersion;
  TTlsVersion FMaxTlsVersion;
  TAutoSwitch FCompleteTlsShutdown;
  TAutoSwitch FNotUtf;
  int FInternalEditorEncoding;
  UnicodeString FS3DefaultRegion;
  UnicodeString FS3SessionToken;
  UnicodeString FS3RoleArn;
  UnicodeString FS3RoleSessionName;
  UnicodeString FS3Profile;
  TS3UrlStyle FS3UrlStyle;
  TAutoSwitch FS3MaxKeys;
  bool FS3CredentialsEnv;
  bool FS3RequesterPays;
  bool FIsWorkspace;
  UnicodeString FLink;
  UnicodeString FNameOverride;
  UnicodeString FHostKey;
  bool FFingerprintScan;
  bool FOverrideCachedHostKey;
  UnicodeString FNote;
  UnicodeString FWinTitle;
  RawByteString FEncryptKey;
  bool FWebDavLiberalEscaping;
  bool FWebDavAuthLegacy;
  bool FWebDavCrossDomainRedirects;
  bool FWebDavUnencryptedRedirects;

  UnicodeString FOrigHostName;
  int FOrigPortNumber;
  TProxyMethod FOrigProxyMethod;
  TSessionSource FSource;
  bool FSaveOnly;
  UnicodeString FLogicalHostName;
  bool * FUnsafeSettings;

  void __fastcall SetHostName(UnicodeString value);
  UnicodeString __fastcall GetHostNameExpanded();
  UnicodeString GetHostNameSource();
  void __fastcall SetPortNumber(int value);
  void __fastcall SetUserName(UnicodeString value);
  UnicodeString __fastcall GetUserNameExpanded();
  UnicodeString GetUserNameSource();
  void __fastcall SetPassword(UnicodeString value);
  UnicodeString __fastcall GetPassword() const;
  void __fastcall SetNewPassword(UnicodeString value);
  UnicodeString __fastcall GetNewPassword() const;
  void __fastcall SetChangePassword(bool value);
  void __fastcall SetPingInterval(int value);
  void __fastcall SetTryAgent(bool value);
  void __fastcall SetAgentFwd(bool value);
  void __fastcall SetAuthKI(bool value);
  void __fastcall SetAuthKIPassword(bool value);
  void __fastcall SetAuthGSSAPI(bool value);
  void __fastcall SetAuthGSSAPIKEX(bool value);
  void __fastcall SetGSSAPIFwdTGT(bool value);
  void __fastcall SetChangeUsername(bool value);
  void __fastcall SetCompression(bool value);
  void __fastcall SetSsh2DES(bool value);
  void __fastcall SetSshNoUserAuth(bool value);
  void __fastcall SetCipher(int Index, TCipher value);
  TCipher __fastcall GetCipher(int Index) const;
  void __fastcall SetKex(int Index, TKex value);
  TKex __fastcall GetKex(int Index) const;
  void __fastcall SetHostKeys(int Index, THostKey value);
  THostKey __fastcall GetHostKeys(int Index) const;
  void __fastcall SetGssLib(int Index, TGssLib value);
  TGssLib __fastcall GetGssLib(int Index) const;
  void __fastcall SetGssLibCustom(UnicodeString value);
  void __fastcall SetPublicKeyFile(UnicodeString value);
  UnicodeString __fastcall GetPassphrase() const;
  void __fastcall SetPassphrase(UnicodeString value);
  void __fastcall SetDetachedCertificate(UnicodeString value);

  void __fastcall SetPuttyProtocol(UnicodeString value);
  bool __fastcall GetCanLogin();
  bool __fastcall GetCanOpen();
  bool __fastcall GetIsLocalBrowser();
  void __fastcall SetPingIntervalDT(TDateTime value);
  TDateTime __fastcall GetPingIntervalDT();
  TDateTime __fastcall GetFtpPingIntervalDT();
  void __fastcall SetTimeDifference(TDateTime value);
  void __fastcall SetTimeDifferenceAuto(bool value);
  void __fastcall SetPingType(TPingType value);
  UnicodeString __fastcall GetSessionName();
  UnicodeString __fastcall GetDefaultSessionName();
  UnicodeString __fastcall GetProtocolUrl(bool HttpForWebDAV);
  void __fastcall SetFSProtocol(TFSProtocol value);
  UnicodeString __fastcall GetFSProtocolStr();
  void __fastcall SetLocalDirectory(UnicodeString value);
  void __fastcall SetOtherLocalDirectory(const UnicodeString & value);
  UnicodeString __fastcall GetLocalDirectoryExpanded();
  void __fastcall SetRemoteDirectory(UnicodeString value);
  void __fastcall SetSynchronizeBrowsing(bool value);
  void __fastcall SetUpdateDirectories(bool value);
  void __fastcall SetCacheDirectories(bool value);
  void __fastcall SetCacheDirectoryChanges(bool value);
  void __fastcall SetPreserveDirectoryChanges(bool value);
  void __fastcall SetSpecial(bool value);
  UnicodeString __fastcall GetInfoTip();
  bool __fastcall GetDefaultShell();
  void __fastcall SetDetectReturnVar(bool value);
  bool __fastcall GetDetectReturnVar();
  void __fastcall SetListingCommand(UnicodeString value);
  void __fastcall SetClearAliases(bool value);
  void __fastcall SetDefaultShell(bool value);
  void __fastcall SetEOLType(TEOLType value);
  void __fastcall SetTrimVMSVersions(bool value);
  void __fastcall SetVMSAllRevisions(bool value);
  void __fastcall SetLookupUserGroups(TAutoSwitch value);
  void __fastcall SetReturnVar(UnicodeString value);
  void __fastcall SetExitCode1IsError(bool value);
  void __fastcall SetScp1Compatibility(bool value);
  void __fastcall SetShell(UnicodeString value);
  void __fastcall SetSftpServer(UnicodeString value);
  void __fastcall SetTimeout(int value);
  void __fastcall SetUnsetNationalVars(bool value);
  void __fastcall SetIgnoreLsWarnings(bool value);
  void __fastcall SetTcpNoDelay(bool value);
  void __fastcall SetSendBuf(int value);
  void __fastcall SetSourceAddress(const UnicodeString & value);
  void __fastcall SetProtocolFeatures(const UnicodeString & value);
  void __fastcall SetSshSimple(bool value);
  bool __fastcall GetUsesSsh();
  void __fastcall SetCipherList(UnicodeString value);
  UnicodeString __fastcall GetCipherList() const;
  void __fastcall SetKexList(UnicodeString value);
  UnicodeString __fastcall GetKexList() const;
  void __fastcall SetHostKeyList(UnicodeString value);
  UnicodeString __fastcall GetHostKeyList() const;
  void __fastcall SetGssLibList(UnicodeString value);
  UnicodeString __fastcall GetGssLibList() const;
  void __fastcall SetProxyMethod(TProxyMethod value);
  void __fastcall SetProxyHost(UnicodeString value);
  void __fastcall SetProxyPort(int value);
  void __fastcall SetProxyUsername(UnicodeString value);
  void __fastcall SetProxyPassword(UnicodeString value);
  void __fastcall SetProxyTelnetCommand(UnicodeString value);
  void __fastcall SetProxyLocalCommand(UnicodeString value);
  void __fastcall SetProxyDNS(TAutoSwitch value);
  void __fastcall SetProxyLocalhost(bool value);
  UnicodeString __fastcall GetProxyPassword() const;
  void __fastcall SetFtpProxyLogonType(int value);
  void __fastcall SetBug(TSshBug Bug, TAutoSwitch value);
  TAutoSwitch __fastcall GetBug(TSshBug Bug) const;
  UnicodeString __fastcall GetSessionKey();
  void __fastcall SetPuttySettings(UnicodeString value);
  void __fastcall SetCustomParam1(UnicodeString value);
  void __fastcall SetCustomParam2(UnicodeString value);
  void __fastcall SetResolveSymlinks(bool value);
  void __fastcall SetFollowDirectorySymlinks(bool value);
  void __fastcall SetSFTPDownloadQueue(int value);
  void __fastcall SetSFTPUploadQueue(int value);
  void __fastcall SetSFTPListingQueue(int value);
  void __fastcall SetSFTPMaxVersion(int value);
  void __fastcall SetSFTPMaxPacketSize(unsigned long value);
  void __fastcall SetSFTPRealPath(TAutoSwitch value);
  void SetUsePosixRename(bool value);
  void __fastcall SetSFTPBug(TSftpBug Bug, TAutoSwitch value);
  TAutoSwitch __fastcall GetSFTPBug(TSftpBug Bug) const;
  void __fastcall SetSCPLsFullTime(TAutoSwitch value);
  void __fastcall SetFtpListAll(TAutoSwitch value);
  void __fastcall SetFtpHost(TAutoSwitch value);
  void __fastcall SetFtpWorkFromCwd(TAutoSwitch value);
  void SetFtpAnyCodeForPwd(bool value);
  void __fastcall SetSslSessionReuse(bool value);
  void __fastcall SetTlsCertificateFile(UnicodeString value);
  UnicodeString __fastcall GetStorageKey();
  UnicodeString __fastcall GetInternalStorageKey();
  UnicodeString __fastcall GetSiteKey();
  void __fastcall SetDSTMode(TDSTMode value);
  void __fastcall SetDeleteToRecycleBin(bool value);
  void __fastcall SetOverwrittenToRecycleBin(bool value);
  void __fastcall SetRecycleBinPath(UnicodeString value);
  void __fastcall SetPostLoginCommands(UnicodeString value);
  void __fastcall SetAddressFamily(TAddressFamily value);
  void __fastcall SetRekeyData(UnicodeString value);
  void __fastcall SetRekeyTime(unsigned int value);
  void __fastcall SetColor(int value);
  void __fastcall SetTunnel(bool value);
  void __fastcall SetTunnelHostName(UnicodeString value);
  void __fastcall SetTunnelPortNumber(int value);
  void __fastcall SetTunnelUserName(UnicodeString value);
  void __fastcall SetTunnelPassword(UnicodeString value);
  UnicodeString __fastcall GetTunnelPassword() const;
  void __fastcall SetTunnelPublicKeyFile(UnicodeString value);
  void __fastcall SetTunnelPassphrase(UnicodeString value);
  UnicodeString __fastcall GetTunnelPassphrase() const;
  void __fastcall SetTunnelPortFwd(UnicodeString value);
  void __fastcall SetTunnelLocalPortNumber(int value);
  bool __fastcall GetTunnelAutoassignLocalPortNumber();
  void __fastcall SetTunnelHostKey(UnicodeString value);
  void __fastcall SetFtpPasvMode(bool value);
  void __fastcall SetFtpForcePasvIp(TAutoSwitch value);
  void __fastcall SetFtpUseMlsd(TAutoSwitch value);
  void __fastcall SetFtpAccount(UnicodeString value);
  void __fastcall SetFtpPingInterval(int value);
  void __fastcall SetFtpPingType(TFtpPingType value);
  void __fastcall SetFtpTransferActiveImmediately(TAutoSwitch value);
  void __fastcall SetFtps(TFtps value);
  void __fastcall SetMinTlsVersion(TTlsVersion value);
  void __fastcall SetMaxTlsVersion(TTlsVersion value);
  void SetCompleteTlsShutdown(TAutoSwitch value);
  void __fastcall SetNotUtf(TAutoSwitch value);
  void __fastcall SetInternalEditorEncoding(int value);
  void __fastcall SetS3DefaultRegion(UnicodeString value);
  void __fastcall SetS3SessionToken(UnicodeString value);
  void __fastcall SetS3RoleArn(UnicodeString value);
  void __fastcall SetS3RoleSessionName(UnicodeString value);
  void __fastcall SetS3Profile(UnicodeString value);
  void __fastcall SetS3UrlStyle(TS3UrlStyle value);
  void __fastcall SetS3MaxKeys(TAutoSwitch value);
  void __fastcall SetS3CredentialsEnv(bool value);
  void __fastcall SetS3RequesterPays(bool value);
  void __fastcall SetLogicalHostName(UnicodeString value);
  void __fastcall SetIsWorkspace(bool value);
  void __fastcall SetLink(UnicodeString value);
  void __fastcall SetNameOverride(UnicodeString value);
  void __fastcall SetHostKey(UnicodeString value);
  void __fastcall SetNote(UnicodeString value);
  void __fastcall SetWinTitle(UnicodeString value);
  UnicodeString __fastcall GetEncryptKey() const;
  void __fastcall SetEncryptKey(UnicodeString value);
  void __fastcall SetWebDavLiberalEscaping(bool value);
  void __fastcall SetWebDavAuthLegacy(bool value);
  void SetWebDavCrossDomainRedirects(bool value);
  void SetWebDavUnencryptedRedirects(bool value);

  TDateTime __fastcall GetTimeoutDT();
  void __fastcall SavePasswords(THierarchicalStorage * Storage, bool PuttyExport, bool DoNotEncryptPasswords, bool SaveAll);
  UnicodeString __fastcall GetLocalName();
  UnicodeString __fastcall GetFolderName();
  void __fastcall Modify();
  UnicodeString __fastcall GetSourceName();
  void DoLoad(
    THierarchicalStorage * Storage, bool PuttyImport, bool & RewritePassword,
    bool Unsafe, bool RespectDisablePasswordStoring, bool & UnsafeSettings);
  void __fastcall DoSave(THierarchicalStorage * Storage,
    bool PuttyExport, const TSessionData * Default, bool DoNotEncryptPasswords);
  UnicodeString __fastcall ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, const UnicodeString & Default);
  int __fastcall ReadXmlNode(_di_IXMLNode Node, const UnicodeString & Name, int Default);
  _di_IXMLNode __fastcall FindSettingsNode(_di_IXMLNode Node, const UnicodeString & Name);
  UnicodeString __fastcall ReadSettingsNode(_di_IXMLNode Node, const UnicodeString & Name, const UnicodeString & Default);
  int __fastcall ReadSettingsNode(_di_IXMLNode Node, const UnicodeString & Name, int Default);
  bool __fastcall IsSame(const TSessionData * Default, bool AdvancedOnly, TStrings * DifferentProperties, bool Decrypted);
  UnicodeString __fastcall GetNameWithoutHiddenPrefix();
  bool __fastcall HasStateData();
  void __fastcall CopyStateData(TSessionData * SourceData);
  void __fastcall CopyNonCoreData(TSessionData * SourceData);
  UnicodeString __fastcall GetNormalizedPuttyProtocol() const;
  void ReadPasswordsFromFiles();
  static RawByteString __fastcall EncryptPassword(const UnicodeString & Password, UnicodeString Key);
  static UnicodeString __fastcall DecryptPassword(const RawByteString & Password, UnicodeString Key);
  static RawByteString __fastcall StronglyRecryptPassword(const RawByteString & Password, UnicodeString Key);
  static bool __fastcall DoIsProtocolUrl(const UnicodeString & Url, const UnicodeString & Protocol, int & ProtocolLen);
  static bool __fastcall IsProtocolUrl(const UnicodeString & Url, const UnicodeString & Protocol, int & ProtocolLen);
  static void __fastcall AddSwitch(UnicodeString & Result, const UnicodeString & Name, bool Rtf);
  static void __fastcall AddSwitch(
    UnicodeString & Result, const UnicodeString & Name, const UnicodeString & Value, bool Rtf);
  static void __fastcall AddSwitch(UnicodeString & Result, const UnicodeString & Name, int Value, bool Rtf);
  static void __fastcall AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & Name, const UnicodeString & Value);
  static void __fastcall AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & Name, const UnicodeString & Type,
    const UnicodeString & Member);
  static void __fastcall AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & Name, int Value);
  void __fastcall AddAssemblyProperty(
    UnicodeString & Result, TAssemblyLanguage Language,
    const UnicodeString & Name, bool Value);
  TStrings * __fastcall GetRawSettingsForUrl();
  void __fastcall DoCopyData(TSessionData * SourceData, bool NoRecrypt);
  bool HasS3AutoCredentials();
  template<class AlgoT>
  void __fastcall SetAlgoList(AlgoT * List, const AlgoT * DefaultList, const UnicodeString * Names,
    int Count, AlgoT WarnAlgo, UnicodeString value);
  void DefaultProxy();
  void ApplyRawSettings(TStrings * RawSettings, bool Unsafe, int & ParsedInfo, bool LoadName);
  static void __fastcall Remove(THierarchicalStorage * Storage, const UnicodeString & Name);

  UnicodeString __property_get_InternalStorageKey() const { return ::rtl::Mutable(this)->GetInternalStorageKey(); } __declspec(property(get = __property_get_InternalStorageKey)) UnicodeString InternalStorageKey;

public:
  __fastcall TSessionData(UnicodeString aName);
  virtual __fastcall ~TSessionData();
  TSessionData * __fastcall Clone();
  void __fastcall Default();
  void __fastcall DefaultSettings();
  void __fastcall NonPersistent();
  void __fastcall Load(THierarchicalStorage * Storage, bool PuttyImport);
  void ApplyRawSettings(TStrings * RawSettings, bool Unsafe);
  void ApplyRawSettings(THierarchicalStorage * Storage, bool Unsafe, bool RespectDisablePasswordStoring, bool & UnsafeSettings);
  void __fastcall ImportFromFilezilla(_di_IXMLNode Node, const UnicodeString & Path, _di_IXMLNode SettingsNode);
  void ImportFromOpenssh(TStrings * Lines);
  void __fastcall Save(THierarchicalStorage * Storage, bool PuttyExport,
    const TSessionData * Default = NULL);
  void __fastcall SaveRecryptedPasswords(THierarchicalStorage * Storage);
  void __fastcall RecryptPasswords();
  bool __fastcall HasPassword();
  bool __fastcall HasAnySessionPassword();
  bool __fastcall HasAnyPassword();
  void __fastcall ClearSessionPasswords();
  void __fastcall MaskPasswords();
  void __fastcall Remove();
  void __fastcall CacheHostKeyIfNotCached();
  virtual void __fastcall Assign(TPersistent * Source);
  virtual int __fastcall Compare(TNamedObject * Other);
  void __fastcall CopyData(TSessionData * Source);
  void __fastcall CopyDataNoRecrypt(TSessionData * SourceData);
  void __fastcall CopyDirectoriesStateData(TSessionData * SourceData);
  bool ParseUrl(
    const UnicodeString & Url, TOptions * Options, TStoredSessionList * StoredSessions, int & ParsedInfo,
    UnicodeString * FileName, UnicodeString * MaskedUrl, int Flags);
  TStrings * __fastcall SaveToOptions(const TSessionData * Default, bool SaveName, bool PuttyExport);
  void __fastcall ConfigureTunnel(int PortNumber);
  void __fastcall RollbackTunnel();
  TSessionData * CreateTunnelData(int TunnelLocalPortNumber);
  void __fastcall ExpandEnvironmentVariables();
  void __fastcall DisableAuthentationsExceptPassword();
  bool __fastcall IsSame(const TSessionData * Default, bool AdvancedOnly);
  bool __fastcall IsSameDecrypted(const TSessionData * Default);
  bool __fastcall IsSameSite(const TSessionData * Default);
  bool __fastcall IsInFolderOrWorkspace(const UnicodeString & Name);
  UnicodeString __fastcall GenerateSessionUrl(unsigned int Flags);
  bool __fastcall HasRawSettingsForUrl();
  bool __fastcall HasSessionName();
  bool HasAutoCredentials();
  int GetDefaultPort();
  UnicodeString ResolvePublicKeyFile();
  UnicodeString GetSessionPasswordEncryptionKey() const;

  UnicodeString __fastcall GenerateOpenCommandArgs(bool Rtf);
  void __fastcall GenerateAssemblyCode(TAssemblyLanguage Language, UnicodeString & Head, UnicodeString & Tail, int & Indent);
  void __fastcall LookupLastFingerprint();
  bool __fastcall IsSecure();
  static void __fastcall ValidatePath(const UnicodeString Path);
  static void __fastcall ValidateName(const UnicodeString Name);
  static UnicodeString __fastcall MakeValidName(const UnicodeString & Name);
  static UnicodeString __fastcall ExtractLocalName(const UnicodeString & Name);
  static UnicodeString __fastcall ExtractFolderName(const UnicodeString & Name);
  static UnicodeString __fastcall ComposePath(const UnicodeString & Path, const UnicodeString & Name);
  static bool __fastcall IsSensitiveOption(const UnicodeString & Option, const UnicodeString & Value);
  static bool __fastcall IsOptionWithParameters(const UnicodeString & Option);
  static bool __fastcall MaskPasswordInOptionParameter(const UnicodeString & Option, UnicodeString & Param);
  static UnicodeString __fastcall FormatSiteKey(const UnicodeString & HostName, int PortNumber);
  static TStrings * GetAllOptionNames(bool PuttyExport);

  UnicodeString __property_get_HostName() const { return FHostName; } void __property_set_HostName(UnicodeString Value) { SetHostName(Value); } __declspec(property(get = __property_get_HostName, put = __property_set_HostName)) UnicodeString HostName;
  UnicodeString __property_get_HostNameExpanded() const { return ::rtl::Mutable(this)->GetHostNameExpanded(); } __declspec(property(get = __property_get_HostNameExpanded)) UnicodeString HostNameExpanded;
  UnicodeString __property_get_HostNameSource() const { return ::rtl::Mutable(this)->GetHostNameSource(); } __declspec(property(get = __property_get_HostNameSource)) UnicodeString HostNameSource;
  int __property_get_PortNumber() const { return FPortNumber; } void __property_set_PortNumber(int Value) { SetPortNumber(Value); } __declspec(property(get = __property_get_PortNumber, put = __property_set_PortNumber)) int PortNumber;
  UnicodeString __property_get_UserName() const { return FUserName; } void __property_set_UserName(UnicodeString Value) { SetUserName(Value); } __declspec(property(get = __property_get_UserName, put = __property_set_UserName)) UnicodeString UserName;
  UnicodeString __property_get_UserNameExpanded() const { return ::rtl::Mutable(this)->GetUserNameExpanded(); } __declspec(property(get = __property_get_UserNameExpanded)) UnicodeString UserNameExpanded;
  UnicodeString __property_get_UserNameSource() const { return ::rtl::Mutable(this)->GetUserNameSource(); } __declspec(property(get = __property_get_UserNameSource)) UnicodeString UserNameSource;
  UnicodeString __property_get_Password() const { return ::rtl::Mutable(this)->GetPassword(); } void __property_set_Password(UnicodeString Value) { SetPassword(Value); } __declspec(property(get = __property_get_Password, put = __property_set_Password)) UnicodeString Password;
  UnicodeString __property_get_NewPassword() const { return ::rtl::Mutable(this)->GetNewPassword(); } void __property_set_NewPassword(UnicodeString Value) { SetNewPassword(Value); } __declspec(property(get = __property_get_NewPassword, put = __property_set_NewPassword)) UnicodeString NewPassword;
  bool __property_get_ChangePassword() const { return FChangePassword; } void __property_set_ChangePassword(bool Value) { SetChangePassword(Value); } __declspec(property(get = __property_get_ChangePassword, put = __property_set_ChangePassword)) bool ChangePassword;
  int __property_get_PingInterval() const { return FPingInterval; } void __property_set_PingInterval(int Value) { SetPingInterval(Value); } __declspec(property(get = __property_get_PingInterval, put = __property_set_PingInterval)) int PingInterval;
  bool __property_get_TryAgent() const { return FTryAgent; } void __property_set_TryAgent(bool Value) { SetTryAgent(Value); } __declspec(property(get = __property_get_TryAgent, put = __property_set_TryAgent)) bool TryAgent;
  bool __property_get_AgentFwd() const { return FAgentFwd; } void __property_set_AgentFwd(bool Value) { SetAgentFwd(Value); } __declspec(property(get = __property_get_AgentFwd, put = __property_set_AgentFwd)) bool AgentFwd;
  UnicodeString __property_get_ListingCommand() const { return FListingCommand; } void __property_set_ListingCommand(UnicodeString Value) { SetListingCommand(Value); } __declspec(property(get = __property_get_ListingCommand, put = __property_set_ListingCommand)) UnicodeString ListingCommand;
  bool __property_get_AuthKI() const { return FAuthKI; } void __property_set_AuthKI(bool Value) { SetAuthKI(Value); } __declspec(property(get = __property_get_AuthKI, put = __property_set_AuthKI)) bool AuthKI;
  bool __property_get_AuthKIPassword() const { return FAuthKIPassword; } void __property_set_AuthKIPassword(bool Value) { SetAuthKIPassword(Value); } __declspec(property(get = __property_get_AuthKIPassword, put = __property_set_AuthKIPassword)) bool AuthKIPassword;
  bool __property_get_AuthGSSAPI() const { return FAuthGSSAPI; } void __property_set_AuthGSSAPI(bool Value) { SetAuthGSSAPI(Value); } __declspec(property(get = __property_get_AuthGSSAPI, put = __property_set_AuthGSSAPI)) bool AuthGSSAPI;
  bool __property_get_AuthGSSAPIKEX() const { return FAuthGSSAPIKEX; } void __property_set_AuthGSSAPIKEX(bool Value) { SetAuthGSSAPIKEX(Value); } __declspec(property(get = __property_get_AuthGSSAPIKEX, put = __property_set_AuthGSSAPIKEX)) bool AuthGSSAPIKEX;
  bool __property_get_GSSAPIFwdTGT() const { return FGSSAPIFwdTGT; } void __property_set_GSSAPIFwdTGT(bool Value) { SetGSSAPIFwdTGT(Value); } __declspec(property(get = __property_get_GSSAPIFwdTGT, put = __property_set_GSSAPIFwdTGT)) bool GSSAPIFwdTGT;
  bool __property_get_ChangeUsername() const { return FChangeUsername; } void __property_set_ChangeUsername(bool Value) { SetChangeUsername(Value); } __declspec(property(get = __property_get_ChangeUsername, put = __property_set_ChangeUsername)) bool ChangeUsername;
  bool __property_get_Compression() const { return FCompression; } void __property_set_Compression(bool Value) { SetCompression(Value); } __declspec(property(get = __property_get_Compression, put = __property_set_Compression)) bool Compression;
  bool __property_get_UsesSsh() const { return ::rtl::Mutable(this)->GetUsesSsh(); } __declspec(property(get = __property_get_UsesSsh)) bool UsesSsh;
  bool __property_get_Ssh2DES() const { return FSsh2DES; } void __property_set_Ssh2DES(bool Value) { SetSsh2DES(Value); } __declspec(property(get = __property_get_Ssh2DES, put = __property_set_Ssh2DES)) bool Ssh2DES;
  bool __property_get_SshNoUserAuth() const { return FSshNoUserAuth; } void __property_set_SshNoUserAuth(bool Value) { SetSshNoUserAuth(Value); } __declspec(property(get = __property_get_SshNoUserAuth, put = __property_set_SshNoUserAuth)) bool SshNoUserAuth;
  TCipher __property_get_Cipher(int Index) const { return ::rtl::Mutable(this)->GetCipher(Index); } void __property_set_Cipher(int Index, TCipher Value) { SetCipher(Index, Value); } __declspec(property(get = __property_get_Cipher, put = __property_set_Cipher)) TCipher Cipher[];
  TKex __property_get_Kex(int Index) const { return ::rtl::Mutable(this)->GetKex(Index); } void __property_set_Kex(int Index, TKex Value) { SetKex(Index, Value); } __declspec(property(get = __property_get_Kex, put = __property_set_Kex)) TKex Kex[];
  THostKey __property_get_HostKeys(int Index) const { return ::rtl::Mutable(this)->GetHostKeys(Index); } void __property_set_HostKeys(int Index, THostKey Value) { SetHostKeys(Index, Value); } __declspec(property(get = __property_get_HostKeys, put = __property_set_HostKeys)) THostKey HostKeys[];
  TGssLib __property_get_GssLib(int Index) const { return ::rtl::Mutable(this)->GetGssLib(Index); } void __property_set_GssLib(int Index, TGssLib Value) { SetGssLib(Index, Value); } __declspec(property(get = __property_get_GssLib, put = __property_set_GssLib)) TGssLib GssLib[];
  UnicodeString __property_get_GssLibCustom() const { return FGssLibCustom; } void __property_set_GssLibCustom(UnicodeString Value) { SetGssLibCustom(Value); } __declspec(property(get = __property_get_GssLibCustom, put = __property_set_GssLibCustom)) UnicodeString GssLibCustom;
  UnicodeString __property_get_PublicKeyFile() const { return FPublicKeyFile; } void __property_set_PublicKeyFile(UnicodeString Value) { SetPublicKeyFile(Value); } __declspec(property(get = __property_get_PublicKeyFile, put = __property_set_PublicKeyFile)) UnicodeString PublicKeyFile;
  UnicodeString __property_get_Passphrase() const { return ::rtl::Mutable(this)->GetPassphrase(); } void __property_set_Passphrase(UnicodeString Value) { SetPassphrase(Value); } __declspec(property(get = __property_get_Passphrase, put = __property_set_Passphrase)) UnicodeString Passphrase;
  UnicodeString __property_get_DetachedCertificate() const { return FDetachedCertificate; } void __property_set_DetachedCertificate(UnicodeString Value) { SetDetachedCertificate(Value); } __declspec(property(get = __property_get_DetachedCertificate, put = __property_set_DetachedCertificate)) UnicodeString DetachedCertificate;
  UnicodeString __property_get_PuttyProtocol() const { return FPuttyProtocol; } void __property_set_PuttyProtocol(UnicodeString Value) { SetPuttyProtocol(Value); } __declspec(property(get = __property_get_PuttyProtocol, put = __property_set_PuttyProtocol)) UnicodeString PuttyProtocol;
  TFSProtocol __property_get_FSProtocol() const { return FFSProtocol; } void __property_set_FSProtocol(TFSProtocol Value) { SetFSProtocol(Value); } __declspec(property(get = __property_get_FSProtocol, put = __property_set_FSProtocol)) TFSProtocol FSProtocol;
  UnicodeString __property_get_FSProtocolStr() const { return ::rtl::Mutable(this)->GetFSProtocolStr(); } __declspec(property(get = __property_get_FSProtocolStr)) UnicodeString FSProtocolStr;
  bool __property_get_Modified() const { return FModified; } void __property_set_Modified(bool Value) { FModified = Value; } __declspec(property(get = __property_get_Modified, put = __property_set_Modified)) bool Modified;
  bool __property_get_CanLogin() const { return ::rtl::Mutable(this)->GetCanLogin(); } __declspec(property(get = __property_get_CanLogin)) bool CanLogin;
  bool __property_get_CanOpen() const { return ::rtl::Mutable(this)->GetCanOpen(); } __declspec(property(get = __property_get_CanOpen)) bool CanOpen;
  bool __property_get_IsLocalBrowser() const { return ::rtl::Mutable(this)->GetIsLocalBrowser(); } __declspec(property(get = __property_get_IsLocalBrowser)) bool IsLocalBrowser;
  bool __property_get_ClearAliases() const { return FClearAliases; } void __property_set_ClearAliases(bool Value) { SetClearAliases(Value); } __declspec(property(get = __property_get_ClearAliases, put = __property_set_ClearAliases)) bool ClearAliases;
  TDateTime __property_get_PingIntervalDT() const { return ::rtl::Mutable(this)->GetPingIntervalDT(); } void __property_set_PingIntervalDT(TDateTime Value) { SetPingIntervalDT(Value); } __declspec(property(get = __property_get_PingIntervalDT, put = __property_set_PingIntervalDT)) TDateTime PingIntervalDT;
  TDateTime __property_get_TimeDifference() const { return FTimeDifference; } void __property_set_TimeDifference(TDateTime Value) { SetTimeDifference(Value); } __declspec(property(get = __property_get_TimeDifference, put = __property_set_TimeDifference)) TDateTime TimeDifference;
  bool __property_get_TimeDifferenceAuto() const { return FTimeDifferenceAuto; } void __property_set_TimeDifferenceAuto(bool Value) { SetTimeDifferenceAuto(Value); } __declspec(property(get = __property_get_TimeDifferenceAuto, put = __property_set_TimeDifferenceAuto)) bool TimeDifferenceAuto;
  TPingType __property_get_PingType() const { return FPingType; } void __property_set_PingType(TPingType Value) { SetPingType(Value); } __declspec(property(get = __property_get_PingType, put = __property_set_PingType)) TPingType PingType;
  UnicodeString __property_get_SessionName() const { return ::rtl::Mutable(this)->GetSessionName(); } __declspec(property(get = __property_get_SessionName)) UnicodeString SessionName;
  UnicodeString __property_get_DefaultSessionName() const { return ::rtl::Mutable(this)->GetDefaultSessionName(); } __declspec(property(get = __property_get_DefaultSessionName)) UnicodeString DefaultSessionName;
  UnicodeString __property_get_LocalDirectory() const { return FLocalDirectory; } void __property_set_LocalDirectory(UnicodeString Value) { SetLocalDirectory(Value); } __declspec(property(get = __property_get_LocalDirectory, put = __property_set_LocalDirectory)) UnicodeString LocalDirectory;
  UnicodeString __property_get_LocalDirectoryExpanded() const { return ::rtl::Mutable(this)->GetLocalDirectoryExpanded(); } __declspec(property(get = __property_get_LocalDirectoryExpanded)) UnicodeString LocalDirectoryExpanded;
  UnicodeString __property_get_OtherLocalDirectory() const { return FOtherLocalDirectory; } void __property_set_OtherLocalDirectory(UnicodeString Value) { SetOtherLocalDirectory(Value); } __declspec(property(get = __property_get_OtherLocalDirectory, put = __property_set_OtherLocalDirectory)) UnicodeString OtherLocalDirectory;
  UnicodeString __property_get_RemoteDirectory() const { return FRemoteDirectory; } void __property_set_RemoteDirectory(UnicodeString Value) { SetRemoteDirectory(Value); } __declspec(property(get = __property_get_RemoteDirectory, put = __property_set_RemoteDirectory)) UnicodeString RemoteDirectory;
  bool __property_get_SynchronizeBrowsing() const { return FSynchronizeBrowsing; } void __property_set_SynchronizeBrowsing(bool Value) { SetSynchronizeBrowsing(Value); } __declspec(property(get = __property_get_SynchronizeBrowsing, put = __property_set_SynchronizeBrowsing)) bool SynchronizeBrowsing;
  bool __property_get_UpdateDirectories() const { return FUpdateDirectories; } void __property_set_UpdateDirectories(bool Value) { SetUpdateDirectories(Value); } __declspec(property(get = __property_get_UpdateDirectories, put = __property_set_UpdateDirectories)) bool UpdateDirectories;
  bool __property_get_RequireDirectories() const { return FRequireDirectories; } void __property_set_RequireDirectories(bool Value) { FRequireDirectories = Value; } __declspec(property(get = __property_get_RequireDirectories, put = __property_set_RequireDirectories)) bool RequireDirectories;
  bool __property_get_CacheDirectories() const { return FCacheDirectories; } void __property_set_CacheDirectories(bool Value) { SetCacheDirectories(Value); } __declspec(property(get = __property_get_CacheDirectories, put = __property_set_CacheDirectories)) bool CacheDirectories;
  bool __property_get_CacheDirectoryChanges() const { return FCacheDirectoryChanges; } void __property_set_CacheDirectoryChanges(bool Value) { SetCacheDirectoryChanges(Value); } __declspec(property(get = __property_get_CacheDirectoryChanges, put = __property_set_CacheDirectoryChanges)) bool CacheDirectoryChanges;
  bool __property_get_PreserveDirectoryChanges() const { return FPreserveDirectoryChanges; } void __property_set_PreserveDirectoryChanges(bool Value) { SetPreserveDirectoryChanges(Value); } __declspec(property(get = __property_get_PreserveDirectoryChanges, put = __property_set_PreserveDirectoryChanges)) bool PreserveDirectoryChanges;
  bool __property_get_Special() const { return FSpecial; } void __property_set_Special(bool Value) { SetSpecial(Value); } __declspec(property(get = __property_get_Special, put = __property_set_Special)) bool Special;
  bool __property_get_Selected() const { return FSelected; } void __property_set_Selected(bool Value) { FSelected = Value; } __declspec(property(get = __property_get_Selected, put = __property_set_Selected)) bool Selected;
  UnicodeString __property_get_InfoTip() const { return ::rtl::Mutable(this)->GetInfoTip(); } __declspec(property(get = __property_get_InfoTip)) UnicodeString InfoTip;
  bool __property_get_DefaultShell() const { return ::rtl::Mutable(this)->GetDefaultShell(); } void __property_set_DefaultShell(bool Value) { SetDefaultShell(Value); } __declspec(property(get = __property_get_DefaultShell, put = __property_set_DefaultShell)) bool DefaultShell;
  bool __property_get_DetectReturnVar() const { return ::rtl::Mutable(this)->GetDetectReturnVar(); } void __property_set_DetectReturnVar(bool Value) { SetDetectReturnVar(Value); } __declspec(property(get = __property_get_DetectReturnVar, put = __property_set_DetectReturnVar)) bool DetectReturnVar;
  TEOLType __property_get_EOLType() const { return FEOLType; } void __property_set_EOLType(TEOLType Value) { SetEOLType(Value); } __declspec(property(get = __property_get_EOLType, put = __property_set_EOLType)) TEOLType EOLType;
  bool __property_get_TrimVMSVersions() const { return FTrimVMSVersions; } void __property_set_TrimVMSVersions(bool Value) { SetTrimVMSVersions(Value); } __declspec(property(get = __property_get_TrimVMSVersions, put = __property_set_TrimVMSVersions)) bool TrimVMSVersions;
  bool __property_get_VMSAllRevisions() const { return FVMSAllRevisions; } void __property_set_VMSAllRevisions(bool Value) { SetVMSAllRevisions(Value); } __declspec(property(get = __property_get_VMSAllRevisions, put = __property_set_VMSAllRevisions)) bool VMSAllRevisions;
  TAutoSwitch __property_get_LookupUserGroups() const { return FLookupUserGroups; } void __property_set_LookupUserGroups(TAutoSwitch Value) { SetLookupUserGroups(Value); } __declspec(property(get = __property_get_LookupUserGroups, put = __property_set_LookupUserGroups)) TAutoSwitch LookupUserGroups;
  UnicodeString __property_get_ReturnVar() const { return FReturnVar; } void __property_set_ReturnVar(UnicodeString Value) { SetReturnVar(Value); } __declspec(property(get = __property_get_ReturnVar, put = __property_set_ReturnVar)) UnicodeString ReturnVar;
  bool __property_get_ExitCode1IsError() const { return FExitCode1IsError; } void __property_set_ExitCode1IsError(bool Value) { SetExitCode1IsError(Value); } __declspec(property(get = __property_get_ExitCode1IsError, put = __property_set_ExitCode1IsError)) bool ExitCode1IsError;
  bool __property_get_Scp1Compatibility() const { return FScp1Compatibility; } void __property_set_Scp1Compatibility(bool Value) { SetScp1Compatibility(Value); } __declspec(property(get = __property_get_Scp1Compatibility, put = __property_set_Scp1Compatibility)) bool Scp1Compatibility;
  UnicodeString __property_get_Shell() const { return FShell; } void __property_set_Shell(UnicodeString Value) { SetShell(Value); } __declspec(property(get = __property_get_Shell, put = __property_set_Shell)) UnicodeString Shell;
  UnicodeString __property_get_SftpServer() const { return FSftpServer; } void __property_set_SftpServer(UnicodeString Value) { SetSftpServer(Value); } __declspec(property(get = __property_get_SftpServer, put = __property_set_SftpServer)) UnicodeString SftpServer;
  int __property_get_Timeout() const { return FTimeout; } void __property_set_Timeout(int Value) { SetTimeout(Value); } __declspec(property(get = __property_get_Timeout, put = __property_set_Timeout)) int Timeout;
  TDateTime __property_get_TimeoutDT() const { return ::rtl::Mutable(this)->GetTimeoutDT(); } __declspec(property(get = __property_get_TimeoutDT)) TDateTime TimeoutDT;
  bool __property_get_UnsetNationalVars() const { return FUnsetNationalVars; } void __property_set_UnsetNationalVars(bool Value) { SetUnsetNationalVars(Value); } __declspec(property(get = __property_get_UnsetNationalVars, put = __property_set_UnsetNationalVars)) bool UnsetNationalVars;
  bool __property_get_IgnoreLsWarnings() const { return FIgnoreLsWarnings; } void __property_set_IgnoreLsWarnings(bool Value) { SetIgnoreLsWarnings(Value); } __declspec(property(get = __property_get_IgnoreLsWarnings, put = __property_set_IgnoreLsWarnings)) bool IgnoreLsWarnings;
  bool __property_get_TcpNoDelay() const { return FTcpNoDelay; } void __property_set_TcpNoDelay(bool Value) { SetTcpNoDelay(Value); } __declspec(property(get = __property_get_TcpNoDelay, put = __property_set_TcpNoDelay)) bool TcpNoDelay;
  int __property_get_SendBuf() const { return FSendBuf; } void __property_set_SendBuf(int Value) { SetSendBuf(Value); } __declspec(property(get = __property_get_SendBuf, put = __property_set_SendBuf)) int SendBuf;
  UnicodeString __property_get_SourceAddress() const { return FSourceAddress; } void __property_set_SourceAddress(UnicodeString Value) { SetSourceAddress(Value); } __declspec(property(get = __property_get_SourceAddress, put = __property_set_SourceAddress)) UnicodeString SourceAddress;
  UnicodeString __property_get_ProtocolFeatures() const { return FProtocolFeatures; } void __property_set_ProtocolFeatures(UnicodeString Value) { SetProtocolFeatures(Value); } __declspec(property(get = __property_get_ProtocolFeatures, put = __property_set_ProtocolFeatures)) UnicodeString ProtocolFeatures;
  bool __property_get_SshSimple() const { return FSshSimple; } void __property_set_SshSimple(bool Value) { SetSshSimple(Value); } __declspec(property(get = __property_get_SshSimple, put = __property_set_SshSimple)) bool SshSimple;
  UnicodeString __property_get_CipherList() const { return ::rtl::Mutable(this)->GetCipherList(); } void __property_set_CipherList(UnicodeString Value) { SetCipherList(Value); } __declspec(property(get = __property_get_CipherList, put = __property_set_CipherList)) UnicodeString CipherList;
  UnicodeString __property_get_KexList() const { return ::rtl::Mutable(this)->GetKexList(); } void __property_set_KexList(UnicodeString Value) { SetKexList(Value); } __declspec(property(get = __property_get_KexList, put = __property_set_KexList)) UnicodeString KexList;
  UnicodeString __property_get_HostKeyList() const { return ::rtl::Mutable(this)->GetHostKeyList(); } void __property_set_HostKeyList(UnicodeString Value) { SetHostKeyList(Value); } __declspec(property(get = __property_get_HostKeyList, put = __property_set_HostKeyList)) UnicodeString HostKeyList;
  UnicodeString __property_get_GssLibList() const { return ::rtl::Mutable(this)->GetGssLibList(); } void __property_set_GssLibList(UnicodeString Value) { SetGssLibList(Value); } __declspec(property(get = __property_get_GssLibList, put = __property_set_GssLibList)) UnicodeString GssLibList;
  TProxyMethod __property_get_ProxyMethod() const { return FProxyMethod; } void __property_set_ProxyMethod(TProxyMethod Value) { SetProxyMethod(Value); } __declspec(property(get = __property_get_ProxyMethod, put = __property_set_ProxyMethod)) TProxyMethod ProxyMethod;
  UnicodeString __property_get_ProxyHost() const { return FProxyHost; } void __property_set_ProxyHost(UnicodeString Value) { SetProxyHost(Value); } __declspec(property(get = __property_get_ProxyHost, put = __property_set_ProxyHost)) UnicodeString ProxyHost;
  int __property_get_ProxyPort() const { return FProxyPort; } void __property_set_ProxyPort(int Value) { SetProxyPort(Value); } __declspec(property(get = __property_get_ProxyPort, put = __property_set_ProxyPort)) int ProxyPort;
  UnicodeString __property_get_ProxyUsername() const { return FProxyUsername; } void __property_set_ProxyUsername(UnicodeString Value) { SetProxyUsername(Value); } __declspec(property(get = __property_get_ProxyUsername, put = __property_set_ProxyUsername)) UnicodeString ProxyUsername;
  UnicodeString __property_get_ProxyPassword() const { return ::rtl::Mutable(this)->GetProxyPassword(); } void __property_set_ProxyPassword(UnicodeString Value) { SetProxyPassword(Value); } __declspec(property(get = __property_get_ProxyPassword, put = __property_set_ProxyPassword)) UnicodeString ProxyPassword;
  UnicodeString __property_get_ProxyTelnetCommand() const { return FProxyTelnetCommand; } void __property_set_ProxyTelnetCommand(UnicodeString Value) { SetProxyTelnetCommand(Value); } __declspec(property(get = __property_get_ProxyTelnetCommand, put = __property_set_ProxyTelnetCommand)) UnicodeString ProxyTelnetCommand;
  UnicodeString __property_get_ProxyLocalCommand() const { return FProxyLocalCommand; } void __property_set_ProxyLocalCommand(UnicodeString Value) { SetProxyLocalCommand(Value); } __declspec(property(get = __property_get_ProxyLocalCommand, put = __property_set_ProxyLocalCommand)) UnicodeString ProxyLocalCommand;
  TAutoSwitch __property_get_ProxyDNS() const { return FProxyDNS; } void __property_set_ProxyDNS(TAutoSwitch Value) { SetProxyDNS(Value); } __declspec(property(get = __property_get_ProxyDNS, put = __property_set_ProxyDNS)) TAutoSwitch ProxyDNS;
  bool __property_get_ProxyLocalhost() const { return FProxyLocalhost; } void __property_set_ProxyLocalhost(bool Value) { SetProxyLocalhost(Value); } __declspec(property(get = __property_get_ProxyLocalhost, put = __property_set_ProxyLocalhost)) bool ProxyLocalhost;
  int __property_get_FtpProxyLogonType() const { return FFtpProxyLogonType; } void __property_set_FtpProxyLogonType(int Value) { SetFtpProxyLogonType(Value); } __declspec(property(get = __property_get_FtpProxyLogonType, put = __property_set_FtpProxyLogonType)) int FtpProxyLogonType;
  TAutoSwitch __property_get_Bug(TSshBug Bug) const { return ::rtl::Mutable(this)->GetBug(Bug); } void __property_set_Bug(TSshBug Bug, TAutoSwitch Value) { SetBug(Bug, Value); } __declspec(property(get = __property_get_Bug, put = __property_set_Bug)) TAutoSwitch Bug[];
  UnicodeString __property_get_PuttySettings() const { return FPuttySettings; } void __property_set_PuttySettings(UnicodeString Value) { SetPuttySettings(Value); } __declspec(property(get = __property_get_PuttySettings, put = __property_set_PuttySettings)) UnicodeString PuttySettings;
  UnicodeString __property_get_CustomParam1() const { return FCustomParam1; } void __property_set_CustomParam1(UnicodeString Value) { SetCustomParam1(Value); } __declspec(property(get = __property_get_CustomParam1, put = __property_set_CustomParam1)) UnicodeString CustomParam1;
  UnicodeString __property_get_CustomParam2() const { return FCustomParam2; } void __property_set_CustomParam2(UnicodeString Value) { SetCustomParam2(Value); } __declspec(property(get = __property_get_CustomParam2, put = __property_set_CustomParam2)) UnicodeString CustomParam2;
  UnicodeString __property_get_SessionKey() const { return ::rtl::Mutable(this)->GetSessionKey(); } __declspec(property(get = __property_get_SessionKey)) UnicodeString SessionKey;
  bool __property_get_ResolveSymlinks() const { return FResolveSymlinks; } void __property_set_ResolveSymlinks(bool Value) { SetResolveSymlinks(Value); } __declspec(property(get = __property_get_ResolveSymlinks, put = __property_set_ResolveSymlinks)) bool ResolveSymlinks;
  bool __property_get_FollowDirectorySymlinks() const { return FFollowDirectorySymlinks; } void __property_set_FollowDirectorySymlinks(bool Value) { SetFollowDirectorySymlinks(Value); } __declspec(property(get = __property_get_FollowDirectorySymlinks, put = __property_set_FollowDirectorySymlinks)) bool FollowDirectorySymlinks;
  int __property_get_SFTPDownloadQueue() const { return FSFTPDownloadQueue; } void __property_set_SFTPDownloadQueue(int Value) { SetSFTPDownloadQueue(Value); } __declspec(property(get = __property_get_SFTPDownloadQueue, put = __property_set_SFTPDownloadQueue)) int SFTPDownloadQueue;
  int __property_get_SFTPUploadQueue() const { return FSFTPUploadQueue; } void __property_set_SFTPUploadQueue(int Value) { SetSFTPUploadQueue(Value); } __declspec(property(get = __property_get_SFTPUploadQueue, put = __property_set_SFTPUploadQueue)) int SFTPUploadQueue;
  int __property_get_SFTPListingQueue() const { return FSFTPListingQueue; } void __property_set_SFTPListingQueue(int Value) { SetSFTPListingQueue(Value); } __declspec(property(get = __property_get_SFTPListingQueue, put = __property_set_SFTPListingQueue)) int SFTPListingQueue;
  int __property_get_SFTPMaxVersion() const { return FSFTPMaxVersion; } void __property_set_SFTPMaxVersion(int Value) { SetSFTPMaxVersion(Value); } __declspec(property(get = __property_get_SFTPMaxVersion, put = __property_set_SFTPMaxVersion)) int SFTPMaxVersion;
  unsigned long __property_get_SFTPMaxPacketSize() const { return FSFTPMaxPacketSize; } void __property_set_SFTPMaxPacketSize(unsigned long Value) { SetSFTPMaxPacketSize(Value); } __declspec(property(get = __property_get_SFTPMaxPacketSize, put = __property_set_SFTPMaxPacketSize)) unsigned long SFTPMaxPacketSize;
  TAutoSwitch __property_get_SFTPRealPath() const { return FSFTPRealPath; } void __property_set_SFTPRealPath(TAutoSwitch Value) { SetSFTPRealPath(Value); } __declspec(property(get = __property_get_SFTPRealPath, put = __property_set_SFTPRealPath)) TAutoSwitch SFTPRealPath;
  bool __property_get_UsePosixRename() const { return FUsePosixRename; } void __property_set_UsePosixRename(bool Value) { SetUsePosixRename(Value); } __declspec(property(get = __property_get_UsePosixRename, put = __property_set_UsePosixRename)) bool UsePosixRename;
  TAutoSwitch __property_get_SFTPBug(TSftpBug Bug) const { return ::rtl::Mutable(this)->GetSFTPBug(Bug); } void __property_set_SFTPBug(TSftpBug Bug, TAutoSwitch Value) { SetSFTPBug(Bug, Value); } __declspec(property(get = __property_get_SFTPBug, put = __property_set_SFTPBug)) TAutoSwitch SFTPBug[];
  TAutoSwitch __property_get_SCPLsFullTime() const { return FSCPLsFullTime; } void __property_set_SCPLsFullTime(TAutoSwitch Value) { SetSCPLsFullTime(Value); } __declspec(property(get = __property_get_SCPLsFullTime, put = __property_set_SCPLsFullTime)) TAutoSwitch SCPLsFullTime;
  TAutoSwitch __property_get_FtpListAll() const { return FFtpListAll; } void __property_set_FtpListAll(TAutoSwitch Value) { SetFtpListAll(Value); } __declspec(property(get = __property_get_FtpListAll, put = __property_set_FtpListAll)) TAutoSwitch FtpListAll;
  TAutoSwitch __property_get_FtpHost() const { return FFtpHost; } void __property_set_FtpHost(TAutoSwitch Value) { SetFtpHost(Value); } __declspec(property(get = __property_get_FtpHost, put = __property_set_FtpHost)) TAutoSwitch FtpHost;
  TAutoSwitch __property_get_FtpWorkFromCwd() const { return FFtpWorkFromCwd; } void __property_set_FtpWorkFromCwd(TAutoSwitch Value) { SetFtpWorkFromCwd(Value); } __declspec(property(get = __property_get_FtpWorkFromCwd, put = __property_set_FtpWorkFromCwd)) TAutoSwitch FtpWorkFromCwd;
  bool __property_get_FtpAnyCodeForPwd() const { return FFtpAnyCodeForPwd; } void __property_set_FtpAnyCodeForPwd(bool Value) { SetFtpAnyCodeForPwd(Value); } __declspec(property(get = __property_get_FtpAnyCodeForPwd, put = __property_set_FtpAnyCodeForPwd)) bool FtpAnyCodeForPwd;
  bool __property_get_SslSessionReuse() const { return FSslSessionReuse; } void __property_set_SslSessionReuse(bool Value) { SetSslSessionReuse(Value); } __declspec(property(get = __property_get_SslSessionReuse, put = __property_set_SslSessionReuse)) bool SslSessionReuse;
  UnicodeString __property_get_TlsCertificateFile() const { return FTlsCertificateFile; } void __property_set_TlsCertificateFile(UnicodeString Value) { SetTlsCertificateFile(Value); } __declspec(property(get = __property_get_TlsCertificateFile, put = __property_set_TlsCertificateFile)) UnicodeString TlsCertificateFile;
  TDSTMode __property_get_DSTMode() const { return FDSTMode; } void __property_set_DSTMode(TDSTMode Value) { SetDSTMode(Value); } __declspec(property(get = __property_get_DSTMode, put = __property_set_DSTMode)) TDSTMode DSTMode;
  bool __property_get_DeleteToRecycleBin() const { return FDeleteToRecycleBin; } void __property_set_DeleteToRecycleBin(bool Value) { SetDeleteToRecycleBin(Value); } __declspec(property(get = __property_get_DeleteToRecycleBin, put = __property_set_DeleteToRecycleBin)) bool DeleteToRecycleBin;
  bool __property_get_OverwrittenToRecycleBin() const { return FOverwrittenToRecycleBin; } void __property_set_OverwrittenToRecycleBin(bool Value) { SetOverwrittenToRecycleBin(Value); } __declspec(property(get = __property_get_OverwrittenToRecycleBin, put = __property_set_OverwrittenToRecycleBin)) bool OverwrittenToRecycleBin;
  UnicodeString __property_get_RecycleBinPath() const { return FRecycleBinPath; } void __property_set_RecycleBinPath(UnicodeString Value) { SetRecycleBinPath(Value); } __declspec(property(get = __property_get_RecycleBinPath, put = __property_set_RecycleBinPath)) UnicodeString RecycleBinPath;
  UnicodeString __property_get_PostLoginCommands() const { return FPostLoginCommands; } void __property_set_PostLoginCommands(UnicodeString Value) { SetPostLoginCommands(Value); } __declspec(property(get = __property_get_PostLoginCommands, put = __property_set_PostLoginCommands)) UnicodeString PostLoginCommands;
  TAddressFamily __property_get_AddressFamily() const { return FAddressFamily; } void __property_set_AddressFamily(TAddressFamily Value) { SetAddressFamily(Value); } __declspec(property(get = __property_get_AddressFamily, put = __property_set_AddressFamily)) TAddressFamily AddressFamily;
  UnicodeString __property_get_RekeyData() const { return FRekeyData; } void __property_set_RekeyData(UnicodeString Value) { SetRekeyData(Value); } __declspec(property(get = __property_get_RekeyData, put = __property_set_RekeyData)) UnicodeString RekeyData;
  unsigned int __property_get_RekeyTime() const { return FRekeyTime; } void __property_set_RekeyTime(unsigned int Value) { SetRekeyTime(Value); } __declspec(property(get = __property_get_RekeyTime, put = __property_set_RekeyTime)) unsigned int RekeyTime;
  int __property_get_Color() const { return FColor; } void __property_set_Color(int Value) { SetColor(Value); } __declspec(property(get = __property_get_Color, put = __property_set_Color)) int Color;
  bool __property_get_Tunnel() const { return FTunnel; } void __property_set_Tunnel(bool Value) { SetTunnel(Value); } __declspec(property(get = __property_get_Tunnel, put = __property_set_Tunnel)) bool Tunnel;
  UnicodeString __property_get_TunnelHostName() const { return FTunnelHostName; } void __property_set_TunnelHostName(UnicodeString Value) { SetTunnelHostName(Value); } __declspec(property(get = __property_get_TunnelHostName, put = __property_set_TunnelHostName)) UnicodeString TunnelHostName;
  int __property_get_TunnelPortNumber() const { return FTunnelPortNumber; } void __property_set_TunnelPortNumber(int Value) { SetTunnelPortNumber(Value); } __declspec(property(get = __property_get_TunnelPortNumber, put = __property_set_TunnelPortNumber)) int TunnelPortNumber;
  UnicodeString __property_get_TunnelUserName() const { return FTunnelUserName; } void __property_set_TunnelUserName(UnicodeString Value) { SetTunnelUserName(Value); } __declspec(property(get = __property_get_TunnelUserName, put = __property_set_TunnelUserName)) UnicodeString TunnelUserName;
  UnicodeString __property_get_TunnelPassword() const { return ::rtl::Mutable(this)->GetTunnelPassword(); } void __property_set_TunnelPassword(UnicodeString Value) { SetTunnelPassword(Value); } __declspec(property(get = __property_get_TunnelPassword, put = __property_set_TunnelPassword)) UnicodeString TunnelPassword;
  UnicodeString __property_get_TunnelPublicKeyFile() const { return FTunnelPublicKeyFile; } void __property_set_TunnelPublicKeyFile(UnicodeString Value) { SetTunnelPublicKeyFile(Value); } __declspec(property(get = __property_get_TunnelPublicKeyFile, put = __property_set_TunnelPublicKeyFile)) UnicodeString TunnelPublicKeyFile;
  UnicodeString __property_get_TunnelPassphrase() const { return ::rtl::Mutable(this)->GetTunnelPassphrase(); } void __property_set_TunnelPassphrase(UnicodeString Value) { SetTunnelPassphrase(Value); } __declspec(property(get = __property_get_TunnelPassphrase, put = __property_set_TunnelPassphrase)) UnicodeString TunnelPassphrase;
  bool __property_get_TunnelAutoassignLocalPortNumber() const { return ::rtl::Mutable(this)->GetTunnelAutoassignLocalPortNumber(); } __declspec(property(get = __property_get_TunnelAutoassignLocalPortNumber)) bool TunnelAutoassignLocalPortNumber;
  int __property_get_TunnelLocalPortNumber() const { return FTunnelLocalPortNumber; } void __property_set_TunnelLocalPortNumber(int Value) { SetTunnelLocalPortNumber(Value); } __declspec(property(get = __property_get_TunnelLocalPortNumber, put = __property_set_TunnelLocalPortNumber)) int TunnelLocalPortNumber;
  UnicodeString __property_get_TunnelPortFwd() const { return FTunnelPortFwd; } void __property_set_TunnelPortFwd(UnicodeString Value) { SetTunnelPortFwd(Value); } __declspec(property(get = __property_get_TunnelPortFwd, put = __property_set_TunnelPortFwd)) UnicodeString TunnelPortFwd;
  UnicodeString __property_get_TunnelHostKey() const { return FTunnelHostKey; } void __property_set_TunnelHostKey(UnicodeString Value) { SetTunnelHostKey(Value); } __declspec(property(get = __property_get_TunnelHostKey, put = __property_set_TunnelHostKey)) UnicodeString TunnelHostKey;
  bool __property_get_FtpPasvMode() const { return FFtpPasvMode; } void __property_set_FtpPasvMode(bool Value) { SetFtpPasvMode(Value); } __declspec(property(get = __property_get_FtpPasvMode, put = __property_set_FtpPasvMode)) bool FtpPasvMode;
  TAutoSwitch __property_get_FtpForcePasvIp() const { return FFtpForcePasvIp; } void __property_set_FtpForcePasvIp(TAutoSwitch Value) { SetFtpForcePasvIp(Value); } __declspec(property(get = __property_get_FtpForcePasvIp, put = __property_set_FtpForcePasvIp)) TAutoSwitch FtpForcePasvIp;
  TAutoSwitch __property_get_FtpUseMlsd() const { return FFtpUseMlsd; } void __property_set_FtpUseMlsd(TAutoSwitch Value) { SetFtpUseMlsd(Value); } __declspec(property(get = __property_get_FtpUseMlsd, put = __property_set_FtpUseMlsd)) TAutoSwitch FtpUseMlsd;
  UnicodeString __property_get_FtpAccount() const { return FFtpAccount; } void __property_set_FtpAccount(UnicodeString Value) { SetFtpAccount(Value); } __declspec(property(get = __property_get_FtpAccount, put = __property_set_FtpAccount)) UnicodeString FtpAccount;
  int __property_get_FtpPingInterval() const { return FFtpPingInterval; } void __property_set_FtpPingInterval(int Value) { SetFtpPingInterval(Value); } __declspec(property(get = __property_get_FtpPingInterval, put = __property_set_FtpPingInterval)) int FtpPingInterval;
  TDateTime __property_get_FtpPingIntervalDT() const { return ::rtl::Mutable(this)->GetFtpPingIntervalDT(); } __declspec(property(get = __property_get_FtpPingIntervalDT)) TDateTime FtpPingIntervalDT;
  TFtpPingType __property_get_FtpPingType() const { return FFtpPingType; } void __property_set_FtpPingType(TFtpPingType Value) { SetFtpPingType(Value); } __declspec(property(get = __property_get_FtpPingType, put = __property_set_FtpPingType)) TFtpPingType FtpPingType;
  TAutoSwitch __property_get_FtpTransferActiveImmediately() const { return FFtpTransferActiveImmediately; } void __property_set_FtpTransferActiveImmediately(TAutoSwitch Value) { SetFtpTransferActiveImmediately(Value); } __declspec(property(get = __property_get_FtpTransferActiveImmediately, put = __property_set_FtpTransferActiveImmediately)) TAutoSwitch FtpTransferActiveImmediately;
  TFtps __property_get_Ftps() const { return FFtps; } void __property_set_Ftps(TFtps Value) { SetFtps(Value); } __declspec(property(get = __property_get_Ftps, put = __property_set_Ftps)) TFtps Ftps;
  TTlsVersion __property_get_MinTlsVersion() const { return FMinTlsVersion; } void __property_set_MinTlsVersion(TTlsVersion Value) { SetMinTlsVersion(Value); } __declspec(property(get = __property_get_MinTlsVersion, put = __property_set_MinTlsVersion)) TTlsVersion MinTlsVersion;
  TTlsVersion __property_get_MaxTlsVersion() const { return FMaxTlsVersion; } void __property_set_MaxTlsVersion(TTlsVersion Value) { SetMaxTlsVersion(Value); } __declspec(property(get = __property_get_MaxTlsVersion, put = __property_set_MaxTlsVersion)) TTlsVersion MaxTlsVersion;
  TAutoSwitch __property_get_CompleteTlsShutdown() const { return FCompleteTlsShutdown; } void __property_set_CompleteTlsShutdown(TAutoSwitch Value) { SetCompleteTlsShutdown(Value); } __declspec(property(get = __property_get_CompleteTlsShutdown, put = __property_set_CompleteTlsShutdown)) TAutoSwitch CompleteTlsShutdown;
  UnicodeString __property_get_LogicalHostName() const { return FLogicalHostName; } void __property_set_LogicalHostName(UnicodeString Value) { SetLogicalHostName(Value); } __declspec(property(get = __property_get_LogicalHostName, put = __property_set_LogicalHostName)) UnicodeString LogicalHostName;
  TAutoSwitch __property_get_NotUtf() const { return FNotUtf; } void __property_set_NotUtf(TAutoSwitch Value) { SetNotUtf(Value); } __declspec(property(get = __property_get_NotUtf, put = __property_set_NotUtf)) TAutoSwitch NotUtf;
  int __property_get_InternalEditorEncoding() const { return FInternalEditorEncoding; } void __property_set_InternalEditorEncoding(int Value) { SetInternalEditorEncoding(Value); } __declspec(property(get = __property_get_InternalEditorEncoding, put = __property_set_InternalEditorEncoding)) int InternalEditorEncoding;
  UnicodeString __property_get_S3DefaultRegion() const { return FS3DefaultRegion; } void __property_set_S3DefaultRegion(UnicodeString Value) { SetS3DefaultRegion(Value); } __declspec(property(get = __property_get_S3DefaultRegion, put = __property_set_S3DefaultRegion)) UnicodeString S3DefaultRegion;
  UnicodeString __property_get_S3SessionToken() const { return FS3SessionToken; } void __property_set_S3SessionToken(UnicodeString Value) { SetS3SessionToken(Value); } __declspec(property(get = __property_get_S3SessionToken, put = __property_set_S3SessionToken)) UnicodeString S3SessionToken;
  UnicodeString __property_get_S3RoleArn() const { return FS3RoleArn; } void __property_set_S3RoleArn(UnicodeString Value) { SetS3RoleArn(Value); } __declspec(property(get = __property_get_S3RoleArn, put = __property_set_S3RoleArn)) UnicodeString S3RoleArn;
  UnicodeString __property_get_S3RoleSessionName() const { return FS3RoleSessionName; } void __property_set_S3RoleSessionName(UnicodeString Value) { SetS3RoleSessionName(Value); } __declspec(property(get = __property_get_S3RoleSessionName, put = __property_set_S3RoleSessionName)) UnicodeString S3RoleSessionName;
  UnicodeString __property_get_S3Profile() const { return FS3Profile; } void __property_set_S3Profile(UnicodeString Value) { SetS3Profile(Value); } __declspec(property(get = __property_get_S3Profile, put = __property_set_S3Profile)) UnicodeString S3Profile;
  TS3UrlStyle __property_get_S3UrlStyle() const { return FS3UrlStyle; } void __property_set_S3UrlStyle(TS3UrlStyle Value) { SetS3UrlStyle(Value); } __declspec(property(get = __property_get_S3UrlStyle, put = __property_set_S3UrlStyle)) TS3UrlStyle S3UrlStyle;
  TAutoSwitch __property_get_S3MaxKeys() const { return FS3MaxKeys; } void __property_set_S3MaxKeys(TAutoSwitch Value) { SetS3MaxKeys(Value); } __declspec(property(get = __property_get_S3MaxKeys, put = __property_set_S3MaxKeys)) TAutoSwitch S3MaxKeys;
  bool __property_get_S3CredentialsEnv() const { return FS3CredentialsEnv; } void __property_set_S3CredentialsEnv(bool Value) { SetS3CredentialsEnv(Value); } __declspec(property(get = __property_get_S3CredentialsEnv, put = __property_set_S3CredentialsEnv)) bool S3CredentialsEnv;
  bool __property_get_S3RequesterPays() const { return FS3RequesterPays; } void __property_set_S3RequesterPays(bool Value) { SetS3RequesterPays(Value); } __declspec(property(get = __property_get_S3RequesterPays, put = __property_set_S3RequesterPays)) bool S3RequesterPays;
  bool __property_get_IsWorkspace() const { return FIsWorkspace; } void __property_set_IsWorkspace(bool Value) { SetIsWorkspace(Value); } __declspec(property(get = __property_get_IsWorkspace, put = __property_set_IsWorkspace)) bool IsWorkspace;
  UnicodeString __property_get_Link() const { return FLink; } void __property_set_Link(UnicodeString Value) { SetLink(Value); } __declspec(property(get = __property_get_Link, put = __property_set_Link)) UnicodeString Link;
  UnicodeString __property_get_NameOverride() const { return FNameOverride; } void __property_set_NameOverride(UnicodeString Value) { SetNameOverride(Value); } __declspec(property(get = __property_get_NameOverride, put = __property_set_NameOverride)) UnicodeString NameOverride;
  UnicodeString __property_get_HostKey() const { return FHostKey; } void __property_set_HostKey(UnicodeString Value) { SetHostKey(Value); } __declspec(property(get = __property_get_HostKey, put = __property_set_HostKey)) UnicodeString HostKey;
  bool __property_get_FingerprintScan() const { return FFingerprintScan; } void __property_set_FingerprintScan(bool Value) { FFingerprintScan = Value; } __declspec(property(get = __property_get_FingerprintScan, put = __property_set_FingerprintScan)) bool FingerprintScan;
  bool __property_get_OverrideCachedHostKey() const { return FOverrideCachedHostKey; } __declspec(property(get = __property_get_OverrideCachedHostKey)) bool OverrideCachedHostKey;
  UnicodeString __property_get_Note() const { return FNote; } void __property_set_Note(UnicodeString Value) { SetNote(Value); } __declspec(property(get = __property_get_Note, put = __property_set_Note)) UnicodeString Note;
  UnicodeString __property_get_WinTitle() const { return FWinTitle; } void __property_set_WinTitle(UnicodeString Value) { SetWinTitle(Value); } __declspec(property(get = __property_get_WinTitle, put = __property_set_WinTitle)) UnicodeString WinTitle;
  UnicodeString __property_get_EncryptKey() const { return ::rtl::Mutable(this)->GetEncryptKey(); } void __property_set_EncryptKey(UnicodeString Value) { SetEncryptKey(Value); } __declspec(property(get = __property_get_EncryptKey, put = __property_set_EncryptKey)) UnicodeString EncryptKey;
  bool __property_get_WebDavLiberalEscaping() const { return FWebDavLiberalEscaping; } void __property_set_WebDavLiberalEscaping(bool Value) { SetWebDavLiberalEscaping(Value); } __declspec(property(get = __property_get_WebDavLiberalEscaping, put = __property_set_WebDavLiberalEscaping)) bool WebDavLiberalEscaping;
  bool __property_get_WebDavAuthLegacy() const { return FWebDavAuthLegacy; } void __property_set_WebDavAuthLegacy(bool Value) { SetWebDavAuthLegacy(Value); } __declspec(property(get = __property_get_WebDavAuthLegacy, put = __property_set_WebDavAuthLegacy)) bool WebDavAuthLegacy;
  bool __property_get_WebDavCrossDomainRedirects() const { return FWebDavCrossDomainRedirects; } void __property_set_WebDavCrossDomainRedirects(bool Value) { SetWebDavCrossDomainRedirects(Value); } __declspec(property(get = __property_get_WebDavCrossDomainRedirects, put = __property_set_WebDavCrossDomainRedirects)) bool WebDavCrossDomainRedirects;
  bool __property_get_WebDavUnencryptedRedirects() const { return FWebDavUnencryptedRedirects; } void __property_set_WebDavUnencryptedRedirects(bool Value) { SetWebDavUnencryptedRedirects(Value); } __declspec(property(get = __property_get_WebDavUnencryptedRedirects, put = __property_set_WebDavUnencryptedRedirects)) bool WebDavUnencryptedRedirects;

  UnicodeString __property_get_StorageKey() const { return ::rtl::Mutable(this)->GetStorageKey(); } __declspec(property(get = __property_get_StorageKey)) UnicodeString StorageKey;
  UnicodeString __property_get_SiteKey() const { return ::rtl::Mutable(this)->GetSiteKey(); } __declspec(property(get = __property_get_SiteKey)) UnicodeString SiteKey;
  UnicodeString __property_get_OrigHostName() const { return FOrigHostName; } __declspec(property(get = __property_get_OrigHostName)) UnicodeString OrigHostName;
  int __property_get_OrigPortNumber() const { return FOrigPortNumber; } __declspec(property(get = __property_get_OrigPortNumber)) int OrigPortNumber;
  UnicodeString __property_get_LocalName() const { return ::rtl::Mutable(this)->GetLocalName(); } __declspec(property(get = __property_get_LocalName)) UnicodeString LocalName;
  UnicodeString __property_get_FolderName() const { return ::rtl::Mutable(this)->GetFolderName(); } __declspec(property(get = __property_get_FolderName)) UnicodeString FolderName;
  TSessionSource __property_get_Source() const { return FSource; } __declspec(property(get = __property_get_Source)) TSessionSource Source;
  UnicodeString __property_get_SourceName() const { return ::rtl::Mutable(this)->GetSourceName(); } __declspec(property(get = __property_get_SourceName)) UnicodeString SourceName;
  bool __property_get_SaveOnly() const { return FSaveOnly; } __declspec(property(get = __property_get_SaveOnly)) bool SaveOnly;
};
//---------------------------------------------------------------------------
class TStoredSessionList : public TNamedObjectList
{
public:
  __fastcall TStoredSessionList(bool aReadOnly = false);
  void __fastcall Reload();
  void __fastcall Save(bool All, bool Explicit);
  void __fastcall Saved();
  void __fastcall ImportFromFilezilla(const UnicodeString FileName, const UnicodeString ConfigurationFileName);
  void __fastcall ImportFromKnownHosts(TStrings * Lines);
  void ImportFromOpenssh(TStrings * Lines);
  void __fastcall Export(const UnicodeString FileName);
  void __fastcall Load(THierarchicalStorage * Storage, bool AsModified = false,
    bool UseDefaults = false, bool PuttyImport = false);
  void __fastcall Save(THierarchicalStorage * Storage, bool All = false);
  void __fastcall SelectAll(bool Select);
  bool Import(TStoredSessionList * From, bool OnlySelected, TList * Imported);
  void __fastcall RecryptPasswords(TStrings * RecryptPasswordErrors);
  TSessionData * __fastcall AtSession(int Index)
    { return static_cast<TSessionData*>(AtObject(Index)); }
  void __fastcall SelectSessionsToImport(TStoredSessionList * Dest, bool SSHOnly);
  void __fastcall Cleanup();
  void __fastcall UpdateStaticUsage();
  int __fastcall IndexOf(TSessionData * Data);
  TSessionData * __fastcall FindSame(TSessionData * Data);
  TSessionData * __fastcall NewSession(UnicodeString SessionName, TSessionData * Session);
  void __fastcall NewWorkspace(UnicodeString Name, TList * DataList);
  bool __fastcall IsFolder(const UnicodeString & Name);
  bool __fastcall IsWorkspace(const UnicodeString & Name);
  bool __fastcall IsFolderOrWorkspace(const UnicodeString & Name);
  TSessionData * ParseUrl(
    const UnicodeString & Url, TOptions * Options, int & ParsedInfo,
    UnicodeString * FileName = NULL, UnicodeString * MaskedUrl = NULL, int Flags = 0);
  int GetUrlInfo(const UnicodeString & Url);
  bool __fastcall CanOpen(TSessionData * Data);
  void __fastcall GetFolderOrWorkspace(const UnicodeString & Name, TList * List);
  TStrings * __fastcall GetFolderOrWorkspaceList(const UnicodeString & Name);
  TStrings * __fastcall GetWorkspaces();
  bool __fastcall HasAnyWorkspace();
  TSessionData * __fastcall SaveWorkspaceData(TSessionData * Data, int Index);
  virtual __fastcall ~TStoredSessionList();
  TSessionData * __property_get_Sessions(int Index) const { return ::rtl::Mutable(this)->AtSession(Index); } __declspec(property(get = __property_get_Sessions)) TSessionData * Sessions[];
  TSessionData * __property_get_DefaultSettings() const { return FDefaultSettings; } void __property_set_DefaultSettings(TSessionData * Value) { SetDefaultSettings(Value); } __declspec(property(get = __property_get_DefaultSettings, put = __property_set_DefaultSettings)) TSessionData * DefaultSettings;

  static int ImportHostKeys(
    THierarchicalStorage * SourceStorage, THierarchicalStorage * TargetStorage, TStoredSessionList * Sessions,
    bool OnlySelected, bool Putty);
  static void ImportHostKeys(THierarchicalStorage * SourceStorage, TStoredSessionList * Sessions, bool OnlySelected);
  static void ImportHostKeys(const UnicodeString & SourceKey, TStoredSessionList * Sessions, bool OnlySelected);
  static void __fastcall ImportSelectedKnownHosts(TStoredSessionList * Sessions);
  static bool __fastcall OpenHostKeysSubKey(THierarchicalStorage * Storage, bool CanCreate);
  static void SelectKnownHostsForSelectedSessions(TStoredSessionList * KnownHosts, TStoredSessionList * Sessions);

private:
  TSessionData * FDefaultSettings;
  bool FReadOnly;
  std::unique_ptr<TStrings> FPendingRemovals;
  void __fastcall SetDefaultSettings(TSessionData * value);
  void __fastcall DoSave(THierarchicalStorage * Storage, bool All,
    bool RecryptPasswordOnly, TStrings * RecryptPasswordErrors);
  void __fastcall DoSave(bool All, bool Explicit, bool RecryptPasswordOnly,
    TStrings * RecryptPasswordErrors);
  void __fastcall DoSave(THierarchicalStorage * Storage,
    TSessionData * Data, bool All, bool RecryptPasswordOnly,
    TSessionData * FactoryDefaults);
  TSessionData * __fastcall ResolveWorkspaceData(TSessionData * Data);
  TSessionData * GetFirstFolderOrWorkspaceSession(const UnicodeString & Name);
  TSessionData * __fastcall CheckIsInFolderOrWorkspaceAndResolve(
    TSessionData * Data, const UnicodeString & Name);
  void __fastcall ImportLevelFromFilezilla(_di_IXMLNode Node, const UnicodeString & Path, _di_IXMLNode SettingsNode);
  void __fastcall DoGetFolderOrWorkspace(const UnicodeString & Name, TList * List, bool NoRecrypt);
  static THierarchicalStorage * __fastcall CreateHostKeysStorageForWriting();
};
//---------------------------------------------------------------------------
UnicodeString GetExpandedLogFileName(UnicodeString LogFileName, TDateTime Started, TSessionData * SessionData);
bool __fastcall IsSshProtocol(TFSProtocol FSProtocol);
int __fastcall DefaultPort(TFSProtocol FSProtocol, TFtps Ftps);
bool HasIP6LiteralBrackets(const UnicodeString & HostName);
UnicodeString StripIP6LiteralBrackets(const UnicodeString & HostName);
bool __fastcall IsIPv6Literal(const UnicodeString & HostName);
UnicodeString __fastcall EscapeIPv6Literal(const UnicodeString & IP);
TFSProtocol NormalizeFSProtocol(TFSProtocol FSProtocol);
bool ParseOpensshDirective(const UnicodeString & ALine, UnicodeString & Directive, UnicodeString & Value);
UnicodeString CutOpensshToken(UnicodeString & S);
UnicodeString ConvertPathFromOpenssh(const UnicodeString & Path);
UnicodeString GetTlsVersionName(TTlsVersion TlsVersion);
//---------------------------------------------------------------------------
#endif
