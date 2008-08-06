// certreq.cpp
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////
t_cert_req_info::t_cert_req_info(void)
{ version = NULL;
  subject=NULL;
  subjectPKInfo=NULL;
  attributes=NULL;
}

t_cert_req_info::~t_cert_req_info(void)
{ if (version)       delete version;
  if (subject)       delete subject;
  if (subjectPKInfo) delete subjectPKInfo;
  if (attributes)    delete attributes;
}

unsigned t_cert_req_info::der_length(bool outer) const
{ unsigned total_length;
  total_length = version->der_length(true) + der_length_sequence(subject) + subjectPKInfo->der_length(true) + der_length_set(attributes, tag_kontext_constructed_0);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_cert_req_info::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  version->save(str);
  save_sequence_of(str, subject);
  subjectPKInfo->save(str);
  save_set_of(str, attributes, tag_kontext_constructed_0);
}

void t_cert_req_info::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  version = new t_asn_integer_direct;
  if (!version) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  version->load(str);
  if (str->any_error()) return;

  load_sequence_of(str, &subject); // NULL subject conforms to the syntax
  if (str->any_error()) return;

  subjectPKInfo = new t_asn_SubjectPublicKeyInfo;
  if (!subjectPKInfo) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  subjectPKInfo->load(str);
  if (str->any_error()) return;

  load_set_of(str, &attributes, tag_kontext_constructed_0); // NULL subject conforms to the syntax
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
t_cert_req::t_cert_req(void)
{ info = NULL;  algorithm=NULL;  signature=NULL; }

t_cert_req::~t_cert_req(void)
{ if (info)      delete info;
  if (algorithm) delete algorithm;
  if (signature) delete signature;
}

unsigned t_cert_req::der_length(bool outer) const
{ unsigned total_length = info->der_length(true) + algorithm->der_length(true) + signature->der_length(true);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_cert_req::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  info->save(str);
  algorithm->save(str);
  signature->save(str);
}

void t_cert_req::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  info = new t_cert_req_info;
  if (!info) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  info->load(str);
  if (str->any_error()) return;

  algorithm = new t_asn_algorithm_identifier;
  if (!algorithm) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  algorithm->load(str);
  if (str->any_error()) return;

  signature = new t_asn_bit_string;
  if (!signature) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  signature->load(str);
}

unsigned char * t_cert_req::serialize_cert_info(unsigned * length)
{ tMemStream mstr;  
  mstr.open_output(info->der_length(true));
  info->save(&mstr);
  *length=mstr.external_pos();
  return mstr.close_output();
}

CryptoPP::Integer generic_sign(t_hash * hs, const unsigned char * digest, unsigned digest_length, const CryptoPP::InvertibleRSAFunction *priv)
// Decorates the digest, pads it and encrypts
{ unsigned decorlen = hs->get_hash_decoration_length();  
  CryptoPP::SecByteBlock DecoratedDigest(decorlen+digest_length);
 // decorate digest:
  memcpy(DecoratedDigest, hs->get_hash_decoration(), decorlen);
  unsigned DecoratedDigestLen = decorlen + digest_length;
  memcpy(DecoratedDigest+decorlen, digest, digest_length);
 // pad the digest and compute the signature:
  CryptoPP::PKCS_SignaturePaddingScheme pad;
  unsigned xPaddedBlockBitLength = priv->MaxImage().BitCount()-1;
  unsigned xPaddedBlockByteLength = CryptoPP::bitsToBytes(xPaddedBlockBitLength);
	CryptoPP::SecByteBlock paddedBlock(xPaddedBlockByteLength);
  //CryptoPP::NullRNG nullgen;
  unsigned char key[20];
  SYSTEMTIME CurrentTime;  GetLocalTime(&CurrentTime);   systemtime2UTC(CurrentTime, (char*)key);
  CryptoPP::X917RNG rng(new CryptoPP::DES_EDE2_Encryption(key), key);

  pad.Pad(rng, DecoratedDigest, DecoratedDigestLen, paddedBlock, xPaddedBlockBitLength);
  CryptoPP::Integer encdig = priv->CalculateInverse(temp_rand_pool(), CryptoPP::Integer(paddedBlock, paddedBlock.size()));
  return encdig;
}

void t_cert_req::sign(const CryptoPP::InvertibleRSAFunction *priv)
{// serialize the authenticated part:
  unsigned auth_data_length;  unsigned char * auth_data = serialize_cert_info(&auth_data_length);
 // compute digest of the authenticated data:
  unsigned char digest[MAX_DIGEST_LENGTH];  t_hash hs(NULL);
  CryptoPP::HashModule * hm = hs.get_hash_module();
  hm->Update(auth_data, auth_data_length);
  hm->Final(digest);
  unsigned digest_length = hm->DigestSize();
 // encrypt:
  CryptoPP::Integer encdig = generic_sign(&hs, digest, digest_length, priv);
  unsigned xDigestSignatureLength = priv->MaxPreimage().ByteCount();
  CryptoPP::SecByteBlock signat(xDigestSignatureLength);
	encdig.Encode(signat, xDigestSignatureLength);
 // add the signature to the request:
  algorithm = new t_asn_algorithm_identifier(hs.get_hash_algorithmRSA_id(), hs.get_hash_algorithmRSA_id_length());
  signature = new t_asn_bit_string(signat, 8*xDigestSignatureLength);
}

bool t_cert_req::verify_own_signature(void)
{// serialize the authenticated part:
  unsigned auth_data_length;  unsigned char * auth_data = serialize_cert_info(&auth_data_length);
  return info->subjectPKInfo->subjectPublicKey->verify_signature(signature->get_val(), signature->get_bitlength()/8, auth_data, auth_data_length);
}
/////////////////////////////////////////// creating distingushed names ///////////////////////////////////////
bool add_disting_name(t_asn_Name ** pName, const unsigned * object_id, unsigned object_id_length, const char * value)
// Creates a new name and adds it to the end of the name list.
{ t_asn_Name * Name = new t_asn_Name(object_id, object_id_length, value, object_id!=attr_type_email_name && object_id!=attr_type_domain_comp_name);
  if (!Name) return false;
  while (*pName!=NULL) pName=&(*pName)->next;
  *pName=Name;
  return true;
}

t_asn_Name * create_name_list(const t_name_part * parts)
{ t_asn_Name * Name = NULL;
  while (parts->object_id!=NULL)
  { if (parts->value && *parts->value)  // a part may be empty
      if (!add_disting_name(&Name, parts->object_id, parts->object_id_length, parts->value))
        return NULL;
    parts++;
  }
  return Name;
}

t_asn_Name * copy_disting_name(const t_asn_Name * sourceName)
{ t_asn_Name * destName=NULL, **pdestName = &destName;
  while (sourceName!=NULL)
  { *pdestName = sourceName->copy();
    if (!*pdestName) return NULL;
    sourceName=sourceName->next;  pdestName = &(*pdestName)->next;
  }
  return destName;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
t_cert_req * create_certification_request(t_keytypes keytype, const t_name_part * SubjectNameList, t_asn_public_key * subjectPublicKey)
{ t_cert_req * cert_req = new t_cert_req;
  if (cert_req)
  { cert_req->info = new t_cert_req_info;
    if (cert_req->info) 
    { cert_req->info->version = new t_asn_integer_direct(0);  // version fixed as 0
      if (cert_req->info->version)
      { cert_req->info->subject = create_name_list(SubjectNameList);
         cert_req->info->subjectPKInfo = new t_asn_SubjectPublicKeyInfo(algorithm_id_rsaencryption, algorithm_id_rsaencryption_length, subjectPublicKey);
         const char * cri = keytype==KEYTYPE_SIGN ? cri_attribute_signature : keytype==KEYTYPE_CERTIFY ? cri_attribute_CA : cri_attribute_RCA;
         cert_req->info->attributes = new t_asn_cri_atribute(cri_attr_type_unstructured_name, cri_attr_type_unstructured_name_length, cri);
      }
    }
    if (cert_req->info)
      return cert_req; 
    delete cert_req;
  }
  return NULL;
}

t_cert_req * load_certification_request(tStream * str, t_message_type msgtype, bool * signature_ok)
// Loads and returns the certification request, checks the structure but not the contents.
{ 
  if (msgtype==MSGT_SIGNED)
  { t_asn_SignedData * signedData = new t_asn_SignedData;
    tMemStream mStr;
    mStr.open_output(str->extent());
    signedData->load_ci(str, &mStr);
    if (str->any_error()) { delete signedData;  return NULL; }
    unsigned real_extent = mStr.external_pos();
    unsigned char * content = mStr.close_output();
   // verify the signature:
    if (signature_ok)
      *signature_ok = signedData->VerifySignature()==VR_OK;
   // load the signed request:
    t_cert_req * cert_req = new t_cert_req;
    if (cert_req) 
    { mStr.open_input(content, real_extent);
      cert_req->load(&mStr);
      delete [] content;
      mStr.close_input();
      if (!mStr.any_error())
        return cert_req; 
      delete cert_req;
    }
  }
  else if (msgtype==MSGT_CERTREQ)
  { t_cert_req * cert_req = new t_cert_req;
    if (cert_req) 
    { cert_req->load(str);
      if (!str->any_error())
        return cert_req; 
      delete cert_req;
    }
  }
  return NULL;
}

