//---------------------------------------------------------------------------
#ifndef HttpH
#define HttpH
//---------------------------------------------------------------------------
#include <memory>
//---------------------------------------------------------------------------
struct ne_session_s;
struct ne_request_s;
struct ne_ssl_certificate_s;
struct ssl_st;
//---------------------------------------------------------------------------
class THttp;
typedef TMethodPtr<void(THttp * Sender, __int64 Size, bool & Cancel)> THttpDownloadEvent;
typedef TMethodPtr<void(THttp * Sender, int Status, const UnicodeString & Message)> THttpErrorEvent;
//---------------------------------------------------------------------------
extern const int BasicHttpResponseLimit;
//---------------------------------------------------------------------------
class THttp
{
public:
  THttp();
  ~THttp();

  void Get();
  void Post(const UnicodeString & Request);
  void Put(const UnicodeString & Request);
  bool IsCertificateError();

  UnicodeString & __property_get_URL() const { return ::rtl::Mutable(this)->FURL; } void __property_set_URL(UnicodeString Value) { FURL = Value; } __declspec(property(get = __property_get_URL, put = __property_set_URL)) UnicodeString URL;
  UnicodeString & __property_get_ProxyHost() const { return ::rtl::Mutable(this)->FProxyHost; } void __property_set_ProxyHost(UnicodeString Value) { FProxyHost = Value; } __declspec(property(get = __property_get_ProxyHost, put = __property_set_ProxyHost)) UnicodeString ProxyHost;
  int & __property_get_ProxyPort() const { return ::rtl::Mutable(this)->FProxyPort; } void __property_set_ProxyPort(int Value) { FProxyPort = Value; } __declspec(property(get = __property_get_ProxyPort, put = __property_set_ProxyPort)) int ProxyPort;
  TStrings * & __property_get_RequestHeaders() const { return ::rtl::Mutable(this)->FRequestHeaders; } void __property_set_RequestHeaders(TStrings * Value) { FRequestHeaders = Value; } __declspec(property(get = __property_get_RequestHeaders, put = __property_set_RequestHeaders)) TStrings * RequestHeaders;
  UnicodeString __property_get_Response() const { return ::rtl::Mutable(this)->GetResponse(); } __declspec(property(get = __property_get_Response)) UnicodeString Response;
  RawByteString & __property_get_ResponseRaw() const { return ::rtl::Mutable(this)->FResponse; } __declspec(property(get = __property_get_ResponseRaw)) RawByteString ResponseRaw;
  TStrings * & __property_get_ResponseHeaders() const { return ::rtl::Mutable(this)->FResponseHeaders; } __declspec(property(get = __property_get_ResponseHeaders)) TStrings * ResponseHeaders;
  __int64 __property_get_ResponseLength() const { return ::rtl::Mutable(this)->GetResponseLength(); } __declspec(property(get = __property_get_ResponseLength)) __int64 ResponseLength;
  __int64 & __property_get_ResponseLimit() const { return ::rtl::Mutable(this)->FResponseLimit; } void __property_set_ResponseLimit(__int64 Value) { FResponseLimit = Value; } __declspec(property(get = __property_get_ResponseLimit, put = __property_set_ResponseLimit)) __int64 ResponseLimit;
  THttpDownloadEvent & __property_get_OnDownload() const { return ::rtl::Mutable(this)->FOnDownload; } void __property_set_OnDownload(THttpDownloadEvent Value) { FOnDownload = Value; } __declspec(property(get = __property_get_OnDownload, put = __property_set_OnDownload)) THttpDownloadEvent OnDownload;
  THttpErrorEvent & __property_get_OnError() const { return ::rtl::Mutable(this)->FOnError; } void __property_set_OnError(THttpErrorEvent Value) { FOnError = Value; } __declspec(property(get = __property_get_OnError, put = __property_set_OnError)) THttpErrorEvent OnError;
  UnicodeString & __property_get_Certificate() const { return ::rtl::Mutable(this)->FCertificate; } void __property_set_Certificate(UnicodeString Value) { FCertificate = Value; } __declspec(property(get = __property_get_Certificate, put = __property_set_Certificate)) UnicodeString Certificate;
  int & __property_get_ConnectTimeout() const { return ::rtl::Mutable(this)->FConnectTimeout; } void __property_set_ConnectTimeout(int Value) { FConnectTimeout = Value; } __declspec(property(get = __property_get_ConnectTimeout, put = __property_set_ConnectTimeout)) int ConnectTimeout;

private:
  UnicodeString FURL;
  UnicodeString FProxyHost;
  int FProxyPort;
  RawByteString FResponse;
  __int64 FResponseLimit;
  std::unique_ptr<Exception> FException;
  THttpDownloadEvent FOnDownload;
  THttpErrorEvent FOnError;
  UnicodeString FHostName;
  UnicodeString FCertificateError;
  TStrings * FRequestHeaders;
  TStrings * FResponseHeaders;
  UnicodeString FCertificate;
  int FConnectTimeout;

  static int NeonBodyReader(void * UserData, const char * Buf, size_t Len);
  int NeonBodyReaderImpl(const char * Buf, size_t Len);
  void SendRequest(const char * Method, const UnicodeString & Request);
  UnicodeString GetResponse();
  __int64 GetResponseLength();
  void InitSslSession(ssl_st * Ssl, ne_session_s * Session);
  static int NeonServerSSLCallback(void * UserData, int Failures, const ne_ssl_certificate_s * Certificate);
  int NeonServerSSLCallbackImpl(int Failures, const ne_ssl_certificate_s * Certificate);
};
//---------------------------------------------------------------------------
#endif
