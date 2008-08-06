// pkcsasn.h

class t_asn_public_key
{public: 
  t_asn_integer_indirect * modulus;
  t_asn_integer_indirect * publicExponent;
 public:
  t_asn_public_key(void);
  ~t_asn_public_key(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
  t_asn_public_key * copy(void) const;
#ifdef PK_BITSTRING
  //bool verify_signature(const t_asn_octet_string * signature)
  bool verify_signature(const unsigned char * signature, unsigned sign_length, const unsigned char * auth_data, unsigned auth_data_length);
#else
  bool verify_signature(const t_asn_integer_indirect * signature);
#endif
};

class t_asn_private_key
{public: 
  t_asn_integer_indirect * modulus;
  t_asn_integer_indirect * publicExponent;
  t_asn_integer_indirect * d, * p, * q, * dp, * dq, * u;
 public:
  t_asn_private_key(void);
  t_asn_private_key(CryptoPP::InvertibleRSAFunction * priv);
  ~t_asn_private_key(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
};

class t_asn_SubjectPublicKeyInfo
{public: 
  t_asn_algorithm_identifier * algorithm;
#ifdef PK_BITSTRING2
  t_asn_bit_string * subjectPublicKey;
#else
  t_asn_public_key * subjectPublicKey;
#endif
 public:
  t_asn_SubjectPublicKeyInfo(void);
  t_asn_SubjectPublicKeyInfo(const unsigned * object_id, unsigned object_id_length, t_asn_public_key * subjectPublicKeyIn); // takes the ownership of subjectPublicKeyIn
  ~t_asn_SubjectPublicKeyInfo(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
  t_asn_SubjectPublicKeyInfo * copy(void) const;
};

class t_cert_req_info
{public: 
  t_asn_integer * version;
  t_asn_Name * subject;
  t_asn_SubjectPublicKeyInfo * subjectPKInfo;
  t_asn_cri_atribute * attributes;
 public:
  t_cert_req_info(void);
  ~t_cert_req_info(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
};

class t_cert_req
{public: 
  t_cert_req_info            * info;
 private:
  t_asn_algorithm_identifier * algorithm;
  t_asn_bit_string           * signature;
 public:
  t_cert_req(void);
  ~t_cert_req(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
  void sign(const CryptoPP::InvertibleRSAFunction *priv);
  bool verify_own_signature(void);
  unsigned char * serialize_cert_info(unsigned * length);
};

t_cert_req * create_certification_request(t_keytypes keytype, const t_name_part * SubjectNameList, t_asn_public_key * subjectPublicKey);
enum t_message_type { MSGT_UNKNOWN, MSGT_DATA, MSGT_SIGNED, MSGT_ENCSIGNED, MSGT_CERTREQ };
t_cert_req * load_certification_request(tStream * str, t_message_type msgtype, bool * signature_ok);

class t_asn_AuthorityKeyIdentifier
{public: 
  t_asn_octet_string * KeyIdentifier;    // OPTIONAL IMPLICIT [0]
  t_asn_Name         * CertIssuer;       // OPTIONAL IMPLICIT [1]
  t_asn_integer_univ * CertSerialNumber; // OPTIONAL IMPLICIT [2]
 public:
  t_asn_AuthorityKeyIdentifier(void);
  ~t_asn_AuthorityKeyIdentifier(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
};

class t_asn_Extensions
{ t_asn_objectid * extension_type;
  t_asn_boolean * extension_bool;
  t_asn_octet_string * extension_value;
 public:
  t_asn_Extensions * next;
  void load(tStream * str); // loads an element
  void save(tStream * str) const;  // saves an element
  unsigned der_length(bool outer) const;
  t_asn_Extensions(void);
  t_asn_Extensions(const unsigned * object_id, unsigned object_id_length, const unsigned char * value, unsigned value_length, t_asn_boolean * bval=NULL);
  ~t_asn_Extensions(void);
  const unsigned char * find_extension_value(const unsigned * type_id, unsigned type_id_length, unsigned * value_length) const;
  t_asn_Extensions * copy(void) const;  // copies the full sequence
};

class t_TBSCertificate
{public:
  t_asn_integer * Version; // EXPLICIT [0] DEFAULT v1 - is said to be 0, my version is 2 -> must include it
  t_asn_integer_univ * CertificateSerialNumber;
  t_asn_algorithm_identifier * Signature;
  t_asn_Name * Issuer;
  t_asn_Validity * Validity;
  t_asn_Name * Subject;
  t_asn_SubjectPublicKeyInfo * SubjectPublicKeyInfo;
  t_asn_Extensions * Extensions; // OPTIONAL EXPLICIT [3]
  t_asn_AuthorityKeyIdentifier * AuthKeyId;  // one of [Extensions] analysed
 public:
  t_TBSCertificate(void);
  ~t_TBSCertificate(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
  bool sameSubject(const t_asn_Name * aName) const;
  t_keytypes get_key_usage(void);
  void get_short_descr(char * buf) const;
  bool IsRootCertificate(void) const;
  t_TBSCertificate * copy(void) const;
};

bool get_readable_name(const t_asn_Name * name, char * buf, unsigned bufsize); // returns true if OK, false on overrun
bool get_readable_name_short(const t_asn_Name * name, char * buf, unsigned bufsize); // returns true if OK, false on overrun

class t_asn_IssuerAndSerialNumber
{public:
  t_asn_Name * issuer;
  t_asn_integer_univ * serial_number;
 public:
  void load(tStream * str); // loads an element
  void save(tStream * str) const;  // saves an element
  unsigned der_length(bool outer) const;
  t_asn_IssuerAndSerialNumber(void);
  ~t_asn_IssuerAndSerialNumber(void);
  bool assign(t_asn_Name * issuerIn, t_asn_integer_univ * serial_numberIn);  // fills and passes the ownership
};

class t_Certificate
{public: 
  t_TBSCertificate           * TbsCertificate;
  t_asn_algorithm_identifier * SignatureAlgorithm;
  t_asn_bit_string           * SignatureValue;
 public:
  t_Certificate * next;
  t_Certificate(void);
  ~t_Certificate(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
  bool same(const t_asn_IssuerAndSerialNumber & certref) const;
  t_Certificate * copy(void) const;
  t_VerifyResult VerifyCertificate(const t_Certificate * working_certificates, SYSTEMTIME * signing_time, SYSTEMTIME * expir_time = NULL, SYSTEMTIME * crl_expir_time=NULL) const;
  void accept(void) const;  // stores the certificate in the certificate database, unless root
};

t_Certificate * make_certificate_from_request(t_cert_req * request, t_keytypes keytype, 
                   const char * start_date_string, const char * end_date_string, const char * certifier_name, HWND hParent,
                   const t_Certificate * signer_cert, const unsigned char * signer_cert_uuid, 
                   const CryptoPP::InvertibleRSAFunction * signer_private_key, int certificate_number);

class t_RevokedCertif
{ t_asn_integer_univ * CertificateSerialNumber;
  t_asn_UTCTime * RevocationDate;
  t_asn_Extensions * Extensions; // OPTIONAL 
 public:
  t_RevokedCertif * next;
  t_RevokedCertif(void);
  t_RevokedCertif(unsigned num, SYSTEMTIME * systime, t_RevokedCertif * next_rev);
  ~t_RevokedCertif(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
  t_asn_integer_univ & number(void) const
    { return *CertificateSerialNumber; }
  const SYSTEMTIME * RevocationDateConst(void) const
    { return RevocationDate->get_system_time(); }
  void get_readable_info(char * buf) const;
};

class t_TBSCertList
{public:
  t_asn_integer * Version; // OPTIONAL, not used, value 1
  t_asn_algorithm_identifier * Signature;
  t_asn_Name * Issuer;
  t_asn_UTCTime * ThisUpdate, * NextUpdate;  // NextUpdate is OPTIONAL
  t_RevokedCertif * RevokedCertificates;
  t_asn_Extensions * clrExtensions; // OPTIONAL EXPLICIT [0]
  // clrExtensions not used in CNB, used by MS CA
 public:
  t_TBSCertList(void);
  ~t_TBSCertList(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
  bool fill(bool root_ca);
};

class t_CertificateRevocation
{
 public:
  t_TBSCertList              * TbsCertList;
  t_asn_algorithm_identifier * SignatureAlgorithm;
  t_asn_bit_string           * SignatureValue;
 public:
  t_CertificateRevocation * next;
  t_CertificateRevocation(void);
  ~t_CertificateRevocation(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
  bool fill(bool root_ca, HWND hParent);
  t_VerifyResult verify_signature(void) const;
  void accept(void) const;
  void make_crl_name(char * pathname) const;
};
///////////////////////////////////////////// messages //////////////////////////////////////////
class t_asn_signer_info
{public:
  t_asn_integer * Version;  // 1
  t_asn_IssuerAndSerialNumber * IssuerAndSerialNumber;
  t_asn_algorithm_identifier * digestAlgorithm;
  t_asn_attribute * authenticatedAttributes; // OPTIONAL [0] IMPLICIT
  t_asn_algorithm_identifier * digestEncryptionAlgorithm;
#ifdef PK_BITSTRING
  t_asn_octet_string * encryptedDigest;
#else
  t_asn_integer_indirect * encryptedDigest;
#endif
  t_asn_attribute * unauthenticatedAttributes; // OPTIONAL [1] IMPLICIT
 public:
  t_asn_signer_info * next;
  t_asn_signer_info(void);
  ~t_asn_signer_info(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
  bool fill_0(const unsigned * content_object_id, unsigned content_object_id_length, const t_Certificate * certif, t_hash & hs,
              const SYSTEMTIME * accounting_date);
  bool fill_1(const unsigned char * digest, unsigned digest_length);
  bool preallocate(const CryptoPP::InvertibleRSAFunction * privateKey, bool encrypted);
  bool sign(const CryptoPP::InvertibleRSAFunction * privateKey, const t_IDEAXCBC_keyIV * pikey, t_hash & hs);
  //bool sign(const CryptoPP::InvertibleRSAFunction * privateKey, const t_DES3CBC_keyIV  * pikey, t_hash & hs);
  void get_signature_info(const t_Certificate * certificates, char * signer_name, SYSTEMTIME * signing_date, bool * found_acc_date, SYSTEMTIME * accounting_date) const;
  t_VerifyResult verify(const t_Certificate * certificates, 
                        const unsigned char * cont_digest, unsigned cont_digest_length, const t_IDEAXCBC_keyIV * pikey) const;
  //t_VerifyResult verify(const t_Certificate * certificates, 
  //                      const unsigned char * cont_digest, unsigned cont_digest_length, const t_DES3CBC_keyIV  * pikey) const;
};

class t_asn_ContentInfo
{ t_asn_objectid * contentType;
  t_asn_octet_string * content;    // [0] EXPLICIT OPTIONAL
  const tStream * alt_content;     // not owning
 public:
  t_asn_ContentInfo(void);
  ~t_asn_ContentInfo(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str, CryptoPP::HashModule & md, unsigned char * cont_digest, unsigned * cont_digest_length) const;
  void load(tStream * str, tStream * dataStream, CryptoPP::HashModule * md, unsigned char * cont_digest);
  bool fill(const unsigned * content_object_id, unsigned content_object_id_length, const tStream * content_str);
};

class t_asn_SignedData
{ t_asn_integer * version;  // 1
  t_asn_algorithm_identifier * digestAlgorithms;
  t_asn_ContentInfo * contentInfo;
 public:
  t_Certificate * certificates; // OPTIONAL  [0] IMPLICIT, PKCS 6 extended certificates not supported
  t_CertificateRevocation * clrs; // OPTIONAL  [1] IMPLICIT
 private:
  t_asn_signer_info * signerInfos;
 // auxiliary data used for one-pass processing
  unsigned char lcont_digest[MAX_DIGEST_LENGTH];  
  unsigned lcont_digest_length;  
  t_hash hs;
 public:
  t_asn_SignedData(void);
  ~t_asn_SignedData(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str, const t_Certificate * certif, const CryptoPP::InvertibleRSAFunction * privateKey) const;
  void save_ci(tStream * str, const t_Certificate * certif, const CryptoPP::InvertibleRSAFunction * privateKey) const;
  void load(tStream * str, tStream * dataStream);
  void load_ci(tStream * str, tStream * dataStream);
  bool fill(const unsigned * content_object_id, unsigned content_object_id_length, const SYSTEMTIME * account_date, 
            const tStream * content_str, const t_Certificate * certif);
  t_VerifyResult VerifySignature(void);
  void get_signature_info(char * signer_name, SYSTEMTIME * signing_date, bool * found_acc_date, SYSTEMTIME * accounting_date);
};

class t_asn_EncryptedContentInfo
{ t_asn_objectid * contentType;
  t_asn_algorithm_identifier * contentEncryptionAlgoritm;
  t_asn_octet_string * encryptedContent;   // OPTIONAL [0] IMPLICIT (primitive)
  const tFileStream * alt_content;     // not owning
 public:
  t_asn_EncryptedContentInfo(void);
  ~t_asn_EncryptedContentInfo(void);
  unsigned der_length(bool outer) const;
  void save(tFileStream * str, IDEAXCBCEncryption & ideaenc, CryptoPP::HashModule & md, unsigned char * cont_digest, unsigned * cont_digest_length) const;
  //void save(tFileStream * str, DES3CBCEncryption  & ideaenc, CryptoPP::HashModule & md, unsigned char * cont_digest, unsigned * cont_digest_length) const;
  void load(tStream * str, t_IDEAXCBC_keyIV & ikey, tFileStream * dataStream, CryptoPP::HashModule * md, unsigned char * cont_digest);
  //void load(tStream * str, t_DES3CBC_keyIV  & ikey, tFileStream * dataStream, CryptoPP::HashModule * md, unsigned char * cont_digest);
  bool fill(const unsigned * content_object_id, unsigned content_object_id_length, const tFileStream * content_str, const unsigned char * IV);
};

class t_asn_RecipientInfo
{public:
  t_asn_integer * version;  // 0
  t_asn_IssuerAndSerialNumber * issuerAndSerialNumber;
  t_asn_algorithm_identifier * keyEncryptionAlgorithm;
  t_asn_octet_string * encryptedKey;
 public:
  t_asn_RecipientInfo * next;
  t_asn_RecipientInfo(void);
  ~t_asn_RecipientInfo(void);
  unsigned der_length(bool outer) const;
  void save(tStream * str) const;
  void load(tStream * str);
  bool fill(const t_Certificate * recip_cert, const t_IDEAXCBC_key  & ikey);
  //bool fill(const t_Certificate * recip_cert, const t_DES3CBC_keyIV & ikey);
  bool decrypt_key(const CryptoPP::InvertibleRSAFunction * enc_private_key, t_IDEAXCBC_key  & ikey) const;
  //bool decrypt_key(const CryptoPP::InvertibleRSAFunction * enc_private_key, t_DES3CBC_keyIV & ikey) const;
};

class t_asn_SignedAndEnvelopedData
{
  t_asn_integer * version;  // 1
  t_asn_RecipientInfo * recipientInfos;
  t_asn_algorithm_identifier * digestAlgorithms;
  t_asn_EncryptedContentInfo * enctrypedContentInfo;
 public:
  t_Certificate * certificates; // OPTIONAL  [0] IMPLICIT, PKCS 6 extended certificates not supported
  t_CertificateRevocation * clrs; // OPTIONAL  [1] IMPLICIT
 private:
  t_asn_signer_info * signerInfos;
 // auxiliary data used for one-pass processing
 private:
  t_IDEAXCBC_keyIV ikey;
  //t_DES3CBC_keyIV  dkey;
  unsigned char lcont_digest[MAX_DIGEST_LENGTH];  
  unsigned lcont_digest_length;  
  t_hash hs;

 public:
  t_asn_SignedAndEnvelopedData(void);
  ~t_asn_SignedAndEnvelopedData(void);
  unsigned der_length(bool outer) const;
  void save_ci(tFileStream * str, const t_Certificate * certif, const CryptoPP::InvertibleRSAFunction * privateKey) const;
  void save   (tFileStream * str, const t_Certificate * certif, const CryptoPP::InvertibleRSAFunction * privateKey) const;
  void load   (tStream * str, tFileStream * dataStream, HWND hParent);
  void load_ci(tStream * str, tFileStream * dataStream, HWND hParent);
  bool fill(const unsigned * content_object_id, unsigned content_object_id_length, const SYSTEMTIME * account_date, 
            const tFileStream * content_str, const t_Certificate * certif);
  void sign(const t_Certificate * certif, const CryptoPP::InvertibleRSAFunction * privateKey);
  bool add_recipient(const t_Certificate * recip_cert);
  t_VerifyResult VerifySignature(void);
  void get_signature_info(char * signer_name, SYSTEMTIME * signing_date, bool * found_acc_date, SYSTEMTIME * accounting_date);
};
////////////////////////////////////////////////////////////////////////////////////////////////
#define max_rdn_name_len  150
typedef char t_readable_rdn_name[max_rdn_name_len+1];
FHANDLE open_input_file(const char * pathname, bool fast);

struct t_inmsg_info
{ char input[MAX_PATH];
  bool found_acc_date;
  bool pp;			  //Identifikace prioritni platby na vstupu
  SYSTEMTIME accounting_date;
  SYSTEMTIME signing_date;
  bool encrypted;
  t_VerifyResult signature_state;
  t_readable_rdn_name signer_name;
  char output[MAX_PATH];
  int delete_input;  // 2 in batch mode
  bool analyse_message(HWND hParent, bool data_message);
 private:
  t_asn_SignedData * signedData;
  t_asn_SignedAndEnvelopedData * sigenvData;
 public:
  t_inmsg_info(void)
    { signedData=NULL;  sigenvData=NULL;  delete_input=true;  pp=false; }
  ~t_inmsg_info(void)
    { if (signedData) delete signedData;  if (sigenvData) delete sigenvData; }
};

struct t_outmsg_info
{ char input[MAX_PATH];
  FHANDLE hFile;
  bool adddate;
  SYSTEMTIME accounting_date;
  bool encrypt;
  t_readable_rdn_name * enc_name;
  char output[MAX_PATH];
  int delete_input;  // 2 in batch mode
  bool fastcnbdata;
  bool process_message_out(HWND hParent);
  t_outmsg_info(bool fastcnbdataIn)
  { fastcnbdata=fastcnbdataIn;  hFile=INVALID_FHANDLE_VALUE; 
    enc_name=NULL;
  }
  ~t_outmsg_info(void)
    { if (hFile!=INVALID_FHANDLE_VALUE) 
        { CloseHandle(hFile);  hFile=INVALID_FHANDLE_VALUE; }
      if (enc_name) delete [] enc_name;
    }
  FHANDLE extract_handle(void)
    { FHANDLE ret=hFile;  hFile=INVALID_FHANDLE_VALUE;  return ret; }
};

////////////////////////////////////////////////////////////////////////////////////////////////
t_Certificate * load_certificate(tStream * str);
bool make_signed_enveloped_message(const tFileStream * content_str, const SYSTEMTIME * account_date, const char * output_pathname, HWND hParent, t_readable_rdn_name * recipnames, bool pack_certificates);

void MyInteger2Integer(t_asn_integer_indirect * val, CryptoPP::Integer & res);
t_asn_integer_indirect * Integer2MyInteger(CryptoPP::Integer val);

NAMESPACE_BEGIN(CryptoPP)
// copy from the old CryptoPP:
class PKCS_SignaturePaddingScheme
{
public:
	unsigned int MaxUnpaddedLength(unsigned int paddedLength) const;
	void Pad(RandomNumberGenerator &rng, const byte *raw, unsigned int inputLength, byte *padded, unsigned int paddedLength) const;
	unsigned int Unpad(const byte *padded, unsigned int paddedLength, byte *raw) const;
};
NAMESPACE_END

