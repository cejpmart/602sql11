#ifdef WINS
#include <windows.h>
#else
#include "winreplsecur.h"
#include <winrepl.h>
#endif
#include <stdlib.h> // NULL
#include <stdio.h>
#include "cmem.h"
#include "pkcsasn.h"
#include "secur.h"
#pragma hdrstop
#include "wbvers.h"
#ifdef xOPEN602
#include "crypto/md5.h"
#else
#include "edtypes.h"
#include "edmd5.h"
#endif

t_dbcert * the_dbcert;
t_dbcert * the_root_cert;

char inipathname[100], LCAinipathname[100];

#if 0 // in the static sec library conflicts with the same functions in the krnl library
bool OpenPrivateKeyFile(HWND hParent, char * pathname, const char * title, char * password)
{ return false; }

long SetFilePointer(FHANDLE hFile, DWORD pos, DWORD * hi, int start)
{ return lseek(hFile, pos, start); }

uint64_t GetFileSize(FHANDLE hFile, DWORD * hi)
{ uint64_t orig = lseek(hFile, 0, FILE_CURRENT);
  uint64_t len  = lseek(hFile, 0, FILE_END);
  lseek(hFile, orig, FILE_BEGIN);
  return len;
}

#endif
bool activate_private_key(t_keytypes keytype, HWND hParent, CryptoPP::InvertibleRSAFunction ** ppriv, t_Certificate ** pcert,
       const t_asn_RecipientInfo * recip)
{ return false; }
t_Certificate * current_sign_certif;
CryptoPP::InvertibleRSAFunction * current_sign_private_key;
t_Certificate * current_enc_certif;
CryptoPP::InvertibleRSAFunction * current_enc_private_key;
t_Certificate * current_cert_certif;
CryptoPP::InvertibleRSAFunction * current_cert_private_key;
 
void progress_open(HWND hParent, unsigned top) {}
void progress_close(void)                      {}
void progress_set_text(const char * text)      {}
void progress_set_value(unsigned val, unsigned top) {}

const t_Certificate * t_dbcert::get_by_subject(const t_asn_Name * theSubject, t_keytypes keytype) // returns non-owning pointer
{ return NULL; }


///////////////////////////////////////////////////////////////////////////////////
#ifdef WINS
HINSTANCE hProgInst;
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
extern "C" DllSecur t_Certificate * WINAPI load_certificate_from_file(const char * pathname)
{ tFileStream str;  t_Certificate * cert=NULL;
  if (str.open_input(pathname))
  { cert = load_certificate(&str);
    str.close_input();
  }
  return cert;
}

extern "C" DllSecur t_Certificate * WINAPI make_certificate(const unsigned char * data, unsigned datalength)
{ tMemStream mstr;  
  mstr.open_output(datalength);
  mstr.save_val(data, datalength);
  mstr.reset();
  t_Certificate * cert = load_certificate(&mstr);
  delete [] mstr.close_output();
  return cert;
}

extern "C" DllSecur CryptoPP::InvertibleRSAFunction * WINAPI load_privkey_from_file(const char * pathname, const char * password, t_Certificate * cert)
{ CryptoPP::InvertibleRSAFunction * priv;
  priv = load_private_rsa(pathname, 0, password);
  if (priv) 
  {// check if the keys form a pair
    const t_asn_public_key * pk = cert->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey;
    if (CryptoPP::Integer(pk->modulus->value, pk->modulus->length) != priv->GetModulus() ||
        CryptoPP::Integer(pk->publicExponent->value, pk->publicExponent->length) != priv->GetPublicExponent())
    { show_error("Soukrom a veejn kl� netvo�p�", 0);
      delete priv;  
      return NULL;
    }
  }
  return priv;
}

extern "C" DllSecur CryptoPP::InvertibleRSAFunction * WINAPI make_privkey(const unsigned char * data, unsigned datalength)
{ tMemStream mstr;  
  mstr.open_output(datalength);
  mstr.save_val(data, datalength);
  mstr.reset();
  t_asn_private_key * privKey = new t_asn_private_key;
  if (!privKey) return NULL;
  privKey->load(&mstr);
  delete [] mstr.close_output();
  if (mstr.any_error())
    { delete privKey;  return NULL; }
  CryptoPP::InvertibleRSAFunction * priv = new CryptoPP::InvertibleRSAFunction();
  priv->Initialize(
    CryptoPP::Integer(privKey->modulus->value, privKey->modulus->length), 
    CryptoPP::Integer(privKey->publicExponent->value, privKey->publicExponent->length), 
    CryptoPP::Integer(privKey->d->value, privKey->d->length), 
    CryptoPP::Integer(privKey->p->value, privKey->p->length), CryptoPP::Integer(privKey->q->value, privKey->q->length), 
    CryptoPP::Integer(privKey->dp->value, privKey->dp->length), CryptoPP::Integer(privKey->dq->value, privKey->dq->length), 
    CryptoPP::Integer(privKey->u->value, privKey->u->length));
  delete privKey;
  return priv;
}

extern "C" DllSecur BOOL WINAPI compare_keys(const t_Certificate * cert, const CryptoPP::InvertibleRSAFunction * priv)
// check if the keys form a pair
{ const t_asn_public_key * pk = cert->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey;
  return CryptoPP::Integer(pk->modulus->value, pk->modulus->length) == priv->GetModulus() &&
         CryptoPP::Integer(pk->publicExponent->value, pk->publicExponent->length) == priv->GetPublicExponent();
}

#include <time.h>

extern "C" DllSecur void WINAPI get_random_string(unsigned length, unsigned char * buf)
{ srand((unsigned)time(NULL));
  for (int i=0;  i<length;  i++) buf[i] = (unsigned char)rand();
}

extern "C" DllSecur void WINAPI encrypt_string_by_public_key(t_Certificate * cert, unsigned char * inbuf, unsigned length, unsigned char * outbuf, unsigned * outlength)
{
 // create RSAFunction from public key: 
  t_asn_public_key * pubkey = cert->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey;
  CryptoPP::Integer modul, pubexp;
  MyInteger2Integer(pubkey->modulus, modul);
  MyInteger2Integer(pubkey->publicExponent, pubexp);
  CryptoPP::RSAFunction rsafnc;  rsafnc.Initialize(modul, pubexp);
 // padding the input data:
  CryptoPP::PKCS_EncryptionPaddingScheme pad;
  int PaddedBlockBitLength  = rsafnc.MaxImage().BitCount()-1;
  int PaddedBlockByteLength = CryptoPP::bitsToBytes(PaddedBlockBitLength);
	CryptoPP::SecByteBlock paddedBlock(PaddedBlockByteLength);
  int CipherTextLength = rsafnc.MaxImage().ByteCount();
	CryptoPP::SecByteBlock cipherText(CipherTextLength);
  unsigned char key[20];
  SYSTEMTIME CurrentTime;  GetLocalTime(&CurrentTime);   systemtime2UTC(CurrentTime, (char*)key);
  CryptoPP::X917RNG rng(new CryptoPP::DES_EDE2_Encryption(key), key);
	pad.Pad(rng, inbuf, length, paddedBlock, PaddedBlockBitLength, CryptoPP::g_nullNameValuePairs);
 // encrypting:
	rsafnc.ApplyFunction(CryptoPP::Integer(paddedBlock, paddedBlock.size())).Encode(cipherText, CipherTextLength);
  //.encryptedKey = new t_asn_octet_string(cipherText, CipherTextLength);
 // writing to output:
  memcpy(outbuf, cipherText, CipherTextLength);
  *outlength=CipherTextLength;
}

extern "C" DllSecur void WINAPI decrypt_string_by_private_key(CryptoPP::InvertibleRSAFunction * priv, const unsigned char * inbuf, unsigned length, unsigned char * outbuf, unsigned * outlength)
{// prepare buffer for padded data: 
  unsigned xPaddedBlockBitLength = priv->MaxImage().BitCount()-1;
  unsigned xPaddedBlockByteLength = CryptoPP::bitsToBytes(xPaddedBlockBitLength);
	CryptoPP::SecByteBlock paddedBlock(xPaddedBlockByteLength);
 // decrypt:
	priv->CalculateInverse(temp_rand_pool(), CryptoPP::Integer(inbuf, length)).Encode(paddedBlock, paddedBlock.size());
 // unpad:
  CryptoPP::PKCS_EncryptionPaddingScheme pad;
	*outlength=(unsigned)pad.Unpad(paddedBlock, xPaddedBlockBitLength, outbuf, CryptoPP::g_nullNameValuePairs);
}

#ifdef STOP
void make_uncertified_key_pair(char * random_data, char * password, unsigned char **privkey, unsigned * privkeylength, unsigned char **cert, unsigned * certlength)
{
  *privkey=NULL;  *cert=NULL;

 // store private key
  t_asn_private_key * privKey = new t_asn_private_key(priv);
  if (privKey)
  { tMemStream mstr1, mstr2;
    unsigned key_length = privKey->der_length(true);
    unsigned enc_key_length = key_length / CryptoPP::IDEA.BLOCKSIZE * CryptoPP::IDEA.BLOCKSIZE + CryptoPP::IDEA.BLOCKSIZE;
    mstr1.open_output(key_length);
    mstr2.open_output(enc_key_length);
    privKey->save(&mstr1);
    delete privKey;
    t_IDEA_key ikey;
    make_key_from_password(password, &ikey);
    CryptoPP::IDEAEncryption ideaenc((const unsigned char*)&ikey); 
    mstr1.copy(&mstr2, ideaenc);
    delete [] mstr1.close_output();
    *privkeylength=enc_key_length;
    *privkey = mstr2.close_output();
  }
}
#endif

extern "C" DllSecur void WINAPI Encrypt_private_key(const char * buf, unsigned length, char * password, char * ebuf, unsigned * elength)
{ tMemStream mstr1, mstr2;
  mstr1.open_output(length);
  mstr1.save_val(buf, length);
  mstr1.reset();
  unsigned enc_key_length = length / CryptoPP::IDEA::BLOCKSIZE * CryptoPP::IDEA::BLOCKSIZE + CryptoPP::IDEA::BLOCKSIZE;
  mstr2.open_output(enc_key_length);

  t_IDEA_key ikey;
  make_key_from_password(password, &ikey);
  CryptoPP::IDEAEncryption ideaenc((const unsigned char*)&ikey); 
  mstr1.copy(&mstr2, ideaenc);
  delete [] mstr1.close_output();
  unsigned char * res = mstr2.close_output();
  memcpy(ebuf, res, enc_key_length);
  delete [] res;
  *elength=enc_key_length;
}


extern "C" DllSecur BOOL WINAPI Decrypt_private_key(const char * ebuf, unsigned elength, char * password, char * buf, unsigned * length)
{ tMemStream mstr1, mstr2;
  mstr1.open_output(elength);
  mstr1.save_val(ebuf, elength);
  mstr1.reset();
  mstr2.open_output(elength);

  t_IDEA_key ikey;
  make_key_from_password(password, &ikey);
  CryptoPP::IDEADecryption ideadec((const unsigned char*)&ikey); 
  if (!stream_copy_decrypt(&mstr1, &mstr2, elength, NULL, ideadec))
    return FALSE;
  delete [] mstr1.close_output();
  *length=mstr2.extent();
  unsigned char * res = mstr2.close_output();
  memcpy(buf, res, *length);
  delete [] res;
  return TRUE;
}

extern "C" DllSecur void WINAPI DeleteCert(t_Certificate * cert)
{ delete cert; }

extern "C" DllSecur void WINAPI DeletePrivKey(CryptoPP::InvertibleRSAFunction * priv)
{ delete priv; }

extern "C" DllSecur void WINAPI make_privkey_serial(BYTE * pubexp, BYTE * modulus, BYTE * p, BYTE * q, 
  BYTE * dp, BYTE * dq, BYTE * u, BYTE * d, unsigned bytelen, unsigned char ** serial, unsigned * length)
{ t_asn_private_key pk;
  pk.modulus       =new t_asn_integer_indirect;  pk.modulus       ->assign_reversed(modulus, bytelen);
  pk.publicExponent=new t_asn_integer_indirect;  pk.publicExponent->assign_reversed(pubexp, 4);
  pk.d =new t_asn_integer_indirect;  pk.d ->assign_reversed(d,  bytelen);
  pk.p =new t_asn_integer_indirect;  pk.p ->assign_reversed(p,  bytelen/2);
  pk.q =new t_asn_integer_indirect;  pk.q ->assign_reversed(q,  bytelen/2);
  pk.dp=new t_asn_integer_indirect;  pk.dp->assign_reversed(dp, bytelen/2);
  pk.dq=new t_asn_integer_indirect;  pk.dq->assign_reversed(dq, bytelen/2);
  pk.u =new t_asn_integer_indirect;  pk.u ->assign_reversed(u,  bytelen/2);
  *length = pk.der_length(true);
  tMemStream mstr;
  mstr.open_output(*length);
  pk.save(&mstr);
  *serial = mstr.close_output();
}

extern "C" DllSecur void WINAPI digest2signature(const unsigned char * digest, unsigned digest_length, const CryptoPP::InvertibleRSAFunction * private_key, unsigned char * enc_hsh)
{ t_hash hs(NULL);
  CryptoPP::Integer encdig = generic_sign(&hs, digest, digest_length, private_key);
  unsigned xDigestSignatureLength = private_key->MaxPreimage().ByteCount();
  *(unsigned*)enc_hsh = xDigestSignatureLength;
  encdig.Encode(enc_hsh+sizeof(unsigned), xDigestSignatureLength);
}

extern "C" DllSecur void WINAPI compute_digest(const unsigned char * auth_data, unsigned auth_data_length, unsigned char * digest, unsigned * digest_length)
{ t_hash hs(NULL);
  CryptoPP::HashModule * hm = hs.get_hash_module();
  hm->Update(auth_data, auth_data_length);
  hm->Final(digest);
  *digest_length = hm->DigestSize();
}

extern "C" DllSecur void WINAPI md5_digest(unsigned char * input, unsigned len, unsigned char * digest)
{ 
#ifdef xOPEN602  // The CryptoPP MD5 digest is not compatible with the original digest algorithm
  CryptoPP::MD5 md5;
  md5.Update(input, len);
  md5.Final(digest);
#else
  EDGetDigest(0, input, len, digest);
#endif
}

extern "C" DllSecur bool WINAPI verify_signature(const t_Certificate * cert, unsigned char * signature, unsigned sign_length, const unsigned char * hsh)
{ const t_asn_public_key * pubkey = cert->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey;
 // convert public key and singature value into Integers:
  CryptoPP::Integer modul, pubexp;
  MyInteger2Integer(pubkey->modulus, modul);
  MyInteger2Integer(pubkey->publicExponent, pubexp);
  CryptoPP::RSAFunction rsafnc;  rsafnc.Initialize(modul, pubexp);
  CryptoPP::Integer signat(signature+sizeof(unsigned), *(unsigned*)signature, CryptoPP::Integer::UNSIGNED); 
 // decode the signature by the public key and unpad:
#ifdef STOP // test, to skutecne vraci padded seq obsahujici hast a alg.
  CryptoPP::Integer decoded = rsafnc.ApplyFunction(signat);
#endif    
  int PaddedBlockBitLength = rsafnc.MaxImage().BitCount()-1;
	CryptoPP::SecByteBlock paddedBlock(CryptoPP::bitsToBytes(PaddedBlockBitLength));
  rsafnc.ApplyFunction(signat).Encode(paddedBlock, paddedBlock.size());
  CryptoPP::PKCS_SignaturePaddingScheme pad;
	CryptoPP::SecByteBlock recoveredDecoratedDigest(pad.MaxUnpaddedLength(PaddedBlockBitLength));
  const unsigned char * rdd = recoveredDecoratedDigest;
	unsigned recoveredDecoratedDigestLen = pad.Unpad(paddedBlock, PaddedBlockBitLength, recoveredDecoratedDigest);
 // undecorate and compare to the real digest:
  if (!memcmp(rdd, CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD160>::decoration, CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD160>::length))
  { unsigned decorlen = CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD160>::length;
   // compute the real digest and compare:
    CryptoPP::RIPEMD160 ripemd160;
    return ripemd160.DigestSize() == recoveredDecoratedDigestLen-decorlen && !memcmp(rdd+decorlen, hsh, recoveredDecoratedDigestLen-decorlen);
  }
  if (!memcmp(rdd, CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD128>::decoration, CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD128>::length))
  { unsigned decorlen = CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD128>::length;
   // compute the real digest and compare:
    CryptoPP::RIPEMD128 ripemd128;
    return ripemd128.DigestSize() == recoveredDecoratedDigestLen-decorlen && !memcmp(rdd+decorlen, hsh, recoveredDecoratedDigestLen-decorlen);
  }
  if (!memcmp(rdd, CryptoPP::PKCS_DigestDecoration<CryptoPP::SHA      >::decoration, CryptoPP::PKCS_DigestDecoration<CryptoPP::SHA      >::length))
  { unsigned decorlen = CryptoPP::PKCS_DigestDecoration<CryptoPP::SHA      >::length;
   // compute the real digest and compare:
    CryptoPP::SHA sha;
    return sha      .DigestSize() == recoveredDecoratedDigestLen-decorlen && !memcmp(rdd+decorlen, hsh, recoveredDecoratedDigestLen-decorlen);
  }
  return false;
}

extern "C" DllSecur void WINAPI get_certificate_info(const t_Certificate * cert, 
               SYSTEMTIME * start_time, SYSTEMTIME * stop_time, bool * is_ca, unsigned char * signer_uuid)
{
  *start_time = *cert->TbsCertificate->Validity->get_start()->get_system_time();
  *stop_time  = *cert->TbsCertificate->Validity->get_end  ()->get_system_time();
  if (cert->TbsCertificate->AuthKeyId->KeyIdentifier && cert->TbsCertificate->AuthKeyId->KeyIdentifier)
    memcpy(signer_uuid, cert->TbsCertificate->AuthKeyId->KeyIdentifier->val, 12);
  else
    memset(signer_uuid, 0, 12);
  *is_ca=false;
  unsigned value_length;
  const unsigned char * keyUsage = cert->TbsCertificate->Extensions->find_extension_value(exten_id_KeyUsage,  exten_id_length, &value_length);
  if (keyUsage && value_length>=keyUsageOctetLength)  
    if (keyUsage[3] & 6) *is_ca=true;
}

extern "C" DllSecur bool WINAPI VerifyCertificateSignature(const t_Certificate * cert, const t_Certificate * upper_cert)
{ tMemStream mstr;  
  mstr.open_output(cert->TbsCertificate->der_length(true));
  cert->TbsCertificate->save(&mstr);
  unsigned auth_data_len=mstr.external_pos();
  unsigned char * auth_data = (unsigned char*)mstr.close_output();
  return upper_cert->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey->
         verify_signature(cert->SignatureValue->get_val(), cert->SignatureValue->get_bitlength()/8, auth_data, auth_data_len);
}

static t_get_certificate_from_db * p_get_certificate_from_db = NULL;
static void * callback_param;

extern "C" DllSecur void WINAPI set_certificate_callback(t_get_certificate_from_db * p_get_certificate_from_dbIn, void * callback_paramIn)
{ p_get_certificate_from_db=p_get_certificate_from_dbIn;  callback_param=callback_paramIn; }

t_Certificate * get_certificate_from_db(int oper, void * certpar)
{ if (p_get_certificate_from_db==NULL) return NULL;
  return (*p_get_certificate_from_db)(callback_param, oper, certpar);
}

#include "crypto/des.h"

extern "C" DllSecur void WINAPI DesEnc(const byte * userKey, byte * inoutBlock)
{ CryptoPP::DESEncryption des(userKey);
  des.ProcessBlock(inoutBlock);
}

extern "C" DllSecur void WINAPI DesDec(const byte * userKey, byte * inoutBlock)
{ CryptoPP::DESDecryption des(userKey);
  des.ProcessBlock(inoutBlock);
}

////////////////////////////////// library version ////////////////////////////////
extern "C" DllSecur unsigned WINAPI SecGetVersion(void)
{ return 256*(256*(256*VERS_1+VERS_2)+VERS_3)+VERS_4; }

#ifdef STOP   // LINUX - this enabled using the binary on platforms with old stdc++ library
extern "C" void __check_eh_spec (int n, int spe);

extern "C" void __check_null_eh_spec (void)
{
   __check_eh_spec (0, 0);
}
#endif

//////////////////////////////////////////////// core ///////////////////////////////////////

void mark_allocations(t_mark_callback * mark_callback)
{
  (*mark_callback)(object_id_ripemd160.get_alloc());
  (*mark_callback)(object_id_ripemd128.get_alloc());
  (*mark_callback)(object_id_sha.get_alloc());
  (*mark_callback)(object_id_rsaripemd160.get_alloc());
  (*mark_callback)(object_id_rsaripemd128.get_alloc());
  (*mark_callback)(object_id_rsasha.get_alloc());
  (*mark_callback)(object_id_ideaxex3.get_alloc());
  (*mark_callback)(object_id_des3cbc.get_alloc());

  (*mark_callback)(sigenv_data_content_object.get_alloc());
  (*mark_callback)(signed_data_content_object.get_alloc());
}

