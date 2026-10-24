// Certificate chain validation of the Windows CryptoAPI (wincrypt.h) on
// OpenSSL, against the system's trusted certificates (OpenSSL's default
// verify paths).
//
// Only what the core's WindowsValidateCertificate needs: a certificate
// context from DER data, a chain engine, the chain of a certificate and the
// SSL chain policy, whose status is the result of OpenSSL's verification
// (purpose SSL server, no host name check, as with Windows'
// CERT_CHAIN_POLICY_SSL without extra policy parameters).
//---------------------------------------------------------------------------
#ifndef WinApiCryptH
#define WinApiCryptH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>

#include <cstddef>
//---------------------------------------------------------------------------
#define RTL_SIZEOF_THROUGH_FIELD(Type, Field) (offsetof(Type, Field) + sizeof(static_cast<Type *>(nullptr)->Field))
//---------------------------------------------------------------------------
#define X509_ASN_ENCODING 0x00000001
#define PKCS_7_ASN_ENCODING 0x00010000
#define CERT_CHAIN_CACHE_END_CERT 0x00000001
#define CERT_CHAIN_REVOCATION_CHECK_END_CERT 0x10000000
#define CERT_CHAIN_REVOCATION_CHECK_CHAIN 0x20000000
#define CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 0x40000000
#define CERT_CHAIN_POLICY_BASE (reinterpret_cast<LPCSTR>(1))
#define CERT_CHAIN_POLICY_SSL (reinterpret_cast<LPCSTR>(4))
//---------------------------------------------------------------------------
// Policy errors
#define CERT_E_EXPIRED (static_cast<HRESULT>(0x800B0101))
#define CERT_E_UNTRUSTEDROOT (static_cast<HRESULT>(0x800B0109))
#define CERT_E_CHAINING (static_cast<HRESULT>(0x800B010A))
#define CERT_E_REVOKED (static_cast<HRESULT>(0x800B010C))
#define CERT_E_WRONG_USAGE (static_cast<HRESULT>(0x800B0110))
#define TRUST_E_CERT_SIGNATURE (static_cast<HRESULT>(0x80096004))
#define TRUST_E_FAIL (static_cast<HRESULT>(0x800B010B))
//---------------------------------------------------------------------------
typedef void * HCERTSTORE;
typedef void * HCERTCHAINENGINE;
struct CERT_INFO;
//---------------------------------------------------------------------------
struct CERT_CONTEXT
{
  DWORD dwCertEncodingType;
  BYTE * pbCertEncoded;
  DWORD cbCertEncoded;
  CERT_INFO * pCertInfo;
  HCERTSTORE hCertStore;
};
typedef CERT_CONTEXT * PCERT_CONTEXT;
typedef const CERT_CONTEXT * PCCERT_CONTEXT;
//---------------------------------------------------------------------------
struct CERT_USAGE_MATCH
{
  DWORD dwType;
  struct
  {
    DWORD cUsageIdentifier;
    LPSTR * rgpszUsageIdentifier;
  } Usage;
};
//---------------------------------------------------------------------------
struct CERT_CHAIN_PARA
{
  DWORD cbSize;
  CERT_USAGE_MATCH RequestedUsage;
};
typedef CERT_CHAIN_PARA * PCERT_CHAIN_PARA;
//---------------------------------------------------------------------------
// The layout of Windows (the core checks the size through CycleDetectionModulus)
struct CERT_CHAIN_ENGINE_CONFIG
{
  DWORD cbSize;
  HCERTSTORE hRestrictedRoot;
  HCERTSTORE hRestrictedTrust;
  HCERTSTORE hRestrictedOther;
  DWORD cAdditionalStore;
  HCERTSTORE * rghAdditionalStore;
  DWORD dwFlags;
  DWORD dwUrlRetrievalTimeout;
  DWORD MaximumCachedCertificates;
  DWORD CycleDetectionModulus;
  HCERTSTORE hExclusiveRoot;
  HCERTSTORE hExclusiveTrustedPeople;
  DWORD dwExclusiveFlags;
};
typedef CERT_CHAIN_ENGINE_CONFIG * PCERT_CHAIN_ENGINE_CONFIG;
//---------------------------------------------------------------------------
struct CERT_TRUST_STATUS
{
  DWORD dwErrorStatus;
  DWORD dwInfoStatus;
};
//---------------------------------------------------------------------------
struct CERT_CHAIN_CONTEXT
{
  DWORD cbSize;
  CERT_TRUST_STATUS TrustStatus;
  DWORD cChain;
  void * rgpChain;
  DWORD cLowerQualityChainContext;
  void * rgpLowerQualityChainContext;
  BOOL fHasRevocationFreshnessTime;
  DWORD dwRevocationFreshnessTime;
};
typedef CERT_CHAIN_CONTEXT * PCERT_CHAIN_CONTEXT;
typedef const CERT_CHAIN_CONTEXT * PCCERT_CHAIN_CONTEXT;
//---------------------------------------------------------------------------
struct CERT_CHAIN_POLICY_PARA
{
  DWORD cbSize;
  DWORD dwFlags;
  void * pvExtraPolicyPara;
};
typedef CERT_CHAIN_POLICY_PARA * PCERT_CHAIN_POLICY_PARA;
//---------------------------------------------------------------------------
struct CERT_CHAIN_POLICY_STATUS
{
  DWORD cbSize;
  DWORD dwError;
  LONG lChainIndex;
  LONG lElementIndex;
  void * pvExtraPolicyStatus;
};
typedef CERT_CHAIN_POLICY_STATUS * PCERT_CHAIN_POLICY_STATUS;
//---------------------------------------------------------------------------
WINAPI_EXTERN_C_BEGIN
PCCERT_CONTEXT CertCreateCertificateContext(DWORD CertEncodingType, const BYTE * CertEncoded, DWORD CertEncodedSize);
BOOL CertFreeCertificateContext(PCCERT_CONTEXT CertContext);
BOOL CertCreateCertificateChainEngine(PCERT_CHAIN_ENGINE_CONFIG Config, HCERTCHAINENGINE * ChainEngine);
void CertFreeCertificateChainEngine(HCERTCHAINENGINE ChainEngine);
BOOL CertGetCertificateChain(
  HCERTCHAINENGINE ChainEngine, PCCERT_CONTEXT CertContext, LPFILETIME Time, HCERTSTORE AdditionalStore,
  PCERT_CHAIN_PARA ChainPara, DWORD Flags, LPVOID Reserved, PCCERT_CHAIN_CONTEXT * ChainContext);
void CertFreeCertificateChain(PCCERT_CHAIN_CONTEXT ChainContext);
BOOL CertVerifyCertificateChainPolicy(
  LPCSTR PolicyOID, PCCERT_CHAIN_CONTEXT ChainContext, PCERT_CHAIN_POLICY_PARA PolicyPara,
  PCERT_CHAIN_POLICY_STATUS PolicyStatus);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
#endif
