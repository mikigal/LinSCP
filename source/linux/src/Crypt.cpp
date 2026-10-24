// Certificate chain validation of the Windows CryptoAPI on OpenSSL, see
// winapi/Crypt.h.
//---------------------------------------------------------------------------
#include <winapi/Crypt.h>

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>

#include <vector>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
struct TCertificateContext : public CERT_CONTEXT
{
  X509 * Certificate;
  std::vector<BYTE> Data;
};
//---------------------------------------------------------------------------
struct TChainContext : public CERT_CHAIN_CONTEXT
{
  HRESULT Error;
  int Depth;
};
//---------------------------------------------------------------------------
HRESULT VerifyErrorToHResult(int Error)
{
  switch (Error)
  {
    case X509_V_OK:
      return S_OK;
    case X509_V_ERR_CERT_HAS_EXPIRED:
    case X509_V_ERR_CERT_NOT_YET_VALID:
      return CERT_E_EXPIRED;
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    case X509_V_ERR_CERT_UNTRUSTED:
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
      return CERT_E_UNTRUSTEDROOT;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
      // The server's intermediate certificates are not passed in
      return CERT_E_CHAINING;
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
      return TRUST_E_CERT_SIGNATURE;
    case X509_V_ERR_CERT_REVOKED:
      return CERT_E_REVOKED;
    case X509_V_ERR_INVALID_PURPOSE:
      return CERT_E_WRONG_USAGE;
    default:
      return TRUST_E_FAIL;
  }
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
PCCERT_CONTEXT CertCreateCertificateContext(DWORD CertEncodingType, const BYTE * CertEncoded, DWORD CertEncodedSize)
{
  if ((CertEncodingType & X509_ASN_ENCODING) == 0)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
  }
  const unsigned char * Data = CertEncoded;
  X509 * Certificate = d2i_X509(nullptr, &Data, static_cast<long>(CertEncodedSize));
  if (Certificate == nullptr)
  {
    SetLastError(ERROR_INVALID_DATA);
    return NULL;
  }
  TCertificateContext * Result = new TCertificateContext();
  Result->Certificate = Certificate;
  Result->Data.assign(CertEncoded, CertEncoded + CertEncodedSize);
  Result->dwCertEncodingType = X509_ASN_ENCODING;
  Result->pbCertEncoded = Result->Data.data();
  Result->cbCertEncoded = CertEncodedSize;
  Result->pCertInfo = NULL;
  Result->hCertStore = NULL;
  return Result;
}
//---------------------------------------------------------------------------
BOOL CertFreeCertificateContext(PCCERT_CONTEXT CertContext)
{
  if (CertContext != NULL)
  {
    const TCertificateContext * Context = static_cast<const TCertificateContext *>(CertContext);
    X509_free(Context->Certificate);
    delete Context;
  }
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL CertCreateCertificateChainEngine(PCERT_CHAIN_ENGINE_CONFIG, HCERTCHAINENGINE * ChainEngine)
{
  X509_STORE * Store = X509_STORE_new();
  if ((Store == nullptr) || (X509_STORE_set_default_paths(Store) != 1))
  {
    X509_STORE_free(Store);
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  *ChainEngine = Store;
  return TRUE;
}
//---------------------------------------------------------------------------
void CertFreeCertificateChainEngine(HCERTCHAINENGINE ChainEngine)
{
  X509_STORE_free(static_cast<X509_STORE *>(ChainEngine));
}
//---------------------------------------------------------------------------
BOOL CertGetCertificateChain(
  HCERTCHAINENGINE ChainEngine, PCCERT_CONTEXT CertContext, LPFILETIME, HCERTSTORE,
  PCERT_CHAIN_PARA, DWORD, LPVOID, PCCERT_CHAIN_CONTEXT * ChainContext)
{
  const TCertificateContext * Context = static_cast<const TCertificateContext *>(CertContext);
  X509_STORE_CTX * StoreContext = X509_STORE_CTX_new();
  if ((StoreContext == nullptr) ||
      (X509_STORE_CTX_init(StoreContext, static_cast<X509_STORE *>(ChainEngine), Context->Certificate, nullptr) != 1))
  {
    X509_STORE_CTX_free(StoreContext);
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  X509_STORE_CTX_set_purpose(StoreContext, X509_PURPOSE_SSL_SERVER);
  bool Valid = (X509_verify_cert(StoreContext) == 1);
  TChainContext * Result = new TChainContext();
  Result->cbSize = sizeof(CERT_CHAIN_CONTEXT);
  Result->Error = Valid ? S_OK : VerifyErrorToHResult(X509_STORE_CTX_get_error(StoreContext));
  Result->Depth = Valid ? -1 : X509_STORE_CTX_get_error_depth(StoreContext);
  Result->TrustStatus.dwErrorStatus = Valid ? 0 : 1;
  Result->cChain = 1;
  X509_STORE_CTX_free(StoreContext);
  *ChainContext = Result;
  return TRUE;
}
//---------------------------------------------------------------------------
void CertFreeCertificateChain(PCCERT_CHAIN_CONTEXT ChainContext)
{
  delete static_cast<const TChainContext *>(ChainContext);
}
//---------------------------------------------------------------------------
BOOL CertVerifyCertificateChainPolicy(
  LPCSTR PolicyOID, PCCERT_CHAIN_CONTEXT ChainContext, PCERT_CHAIN_POLICY_PARA, PCERT_CHAIN_POLICY_STATUS PolicyStatus)
{
  if ((PolicyOID != CERT_CHAIN_POLICY_SSL) && (PolicyOID != CERT_CHAIN_POLICY_BASE))
  {
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
  }
  const TChainContext * Context = static_cast<const TChainContext *>(ChainContext);
  PolicyStatus->dwError = static_cast<DWORD>(Context->Error);
  PolicyStatus->lChainIndex = (Context->Error == S_OK) ? -1 : 0;
  PolicyStatus->lElementIndex = Context->Depth;
  return TRUE;
}
//---------------------------------------------------------------------------
