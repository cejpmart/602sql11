#if WBSECUR_EXPORTS
#define DllSecur __declspec(dllexport)
#else
#define DllSecur __declspec(dllimport)
#endif

#pragma warning(disable:4018)

class t_Certificate;
namespace CryptoPP {
class InvertibleRSAFunction;
}

#ifndef PUK_NAME1
#define PUK_NAME1  16
#define PUK_NAME2   2
#define PUK_NAME3  20
#define PUK_IDENT  80

struct t_user_ident  // copy from wbdefs.h
{ char name1[PUK_NAME1+1];
  char name2[PUK_NAME2+1];
  char name3[PUK_NAME3+1];
  char identif[PUK_IDENT+1];
};
#endif

#define DES_BLOCK_SIZE 8
#define DES_KEY_SIZE  16

extern "C" {
DllSecur t_Certificate * WINAPI load_certificate_from_file(const char * pathname);
DllSecur t_Certificate * WINAPI make_certificate(const unsigned char * data, unsigned datalength);
DllSecur CryptoPP::InvertibleRSAFunction * WINAPI make_privkey(const unsigned char * data, unsigned datalength);
DllSecur void WINAPI DeleteCert(t_Certificate * cert);
DllSecur void WINAPI DeletePrivKey(CryptoPP::InvertibleRSAFunction * priv);
DllSecur BOOL WINAPI compare_keys(const t_Certificate * cert, const CryptoPP::InvertibleRSAFunction * priv);
DllSecur void WINAPI get_random_string(unsigned length, unsigned char * buf);
DllSecur void WINAPI encrypt_string_by_public_key(t_Certificate * cert, unsigned char * inbuf, unsigned length, unsigned char * outbuf, unsigned * outlength);
DllSecur void WINAPI decrypt_string_by_private_key(CryptoPP::InvertibleRSAFunction * priv, const unsigned char * inbuf, unsigned length, unsigned char * outbuf, unsigned * outlength);

DllSecur void WINAPI Encrypt_private_key(const char * buf, unsigned length, char * password, char * ebuf, unsigned * elength);
DllSecur BOOL WINAPI Decrypt_private_key(const char * ebuf, unsigned elength, char * password, char * buf, unsigned * length);

DllSecur void WINAPI make_privkey_serial(BYTE * pubexp, BYTE * modulus, BYTE * p, BYTE * q, 
  BYTE * dp, BYTE * dq, BYTE * u, BYTE * d, unsigned bytelen, unsigned char ** serial, unsigned * length);

DllSecur int WINAPI create_key_pair(HWND hParent, bool for_root, const t_user_ident * user_ident, unsigned char * new_cert_uuid, 
         t_Certificate * cert_for_signing_request, unsigned char ** cert_req_str, unsigned * cert_req_len, 
         int certificate_number);
DllSecur void WINAPI free_unschar_arr(unsigned char * str);
DllSecur BOOL WINAPI make_certificate_from_request_str(HWND hParent, unsigned char * request, unsigned request_len, 
       const t_Certificate * ca_cert, const char * certifier_name,  const unsigned char * ca_cert_uuid,
       unsigned char ** cert_str, unsigned * cert_len, SYSTEMTIME * starttime, SYSTEMTIME * stoptime, bool * is_ca_cert, 
       int certificate_number);
DllSecur CryptoPP::InvertibleRSAFunction * WINAPI get_private_key_for_certificate(HWND hParent, const char * username, const t_Certificate * cert);
DllSecur void WINAPI digest2signature(const unsigned char * digest, unsigned digest_length, const CryptoPP::InvertibleRSAFunction * private_key, unsigned char * enc_hsh);
DllSecur void WINAPI compute_digest(const unsigned char * auth_data, unsigned auth_data_length, unsigned char * digest, unsigned * digest_length);
DllSecur void WINAPI md5_digest(unsigned char * input, unsigned len, unsigned char * digest);
DllSecur bool WINAPI verify_signature(const t_Certificate * cert, unsigned char * signature, unsigned sign_length, const unsigned char * hsh);
DllSecur void WINAPI get_certificate_info(const t_Certificate * cert, 
               SYSTEMTIME * start_time, SYSTEMTIME * stop_time, bool * is_ca, unsigned char * signer_uuid);
DllSecur bool WINAPI VerifyCertificateSignature(const t_Certificate * cert, const t_Certificate * upper_cert);
DllSecur void WINAPI remove_retained_private_key(void);

typedef t_Certificate * WINAPI t_get_certificate_from_db(void * param, int oper, void * certpar);
DllSecur void WINAPI set_certificate_callback(t_get_certificate_from_db * p_get_certificate_from_dbIn, void * callback_paramIn);
DllSecur bool WINAPI create_self_certified_keys(char * random_data, unsigned char ** pcert_str, unsigned * pcert_len, unsigned char ** pprivkey_str, unsigned * pprivkey_len);

#if defined(PREZ) || defined(KERN) || defined(VIEW)
DllPrezen t_Certificate * WINAPI get_best_own_certificate(cdp_t cdp, WBUUID own_uuid, bool for_certifying, trecnum * cert_recnum);
#endif
DllSecur unsigned WINAPI SecGetVersion(void);

DllSecur void WINAPI DesEnc(const byte * userKey, byte * inoutBlock);
DllSecur void WINAPI DesDec(const byte * userKey, byte * inoutBlock);

#ifndef xOPEN602 
DllSecur void   EDSlowInit(char *password, BYTE *Key);
DllSecur BOOL   EDSlowEncrypt(BYTE *buf, DWORD Sz, BYTE *Key);
DllSecur BOOL   EDSlowDecrypt(BYTE *buf, DWORD Sz, BYTE *Key);
#endif

}

// indirect external function
t_Certificate * get_certificate_from_db(int oper, void * certpar);

typedef void t_mark_callback(const void * ptr);
void mark_allocations(t_mark_callback * mark_callback);
