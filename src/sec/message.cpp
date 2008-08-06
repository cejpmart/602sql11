// message.cpp
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
#ifndef ORIG
#include "crypto/filters.h"
#endif

#ifdef WINS
BOOL WINAPI securDeleteFile(LPCTSTR lpFileName)
{ return DeleteFile(lpFileName); }
#else
BOOL WINAPI securDeleteFile(LPCTSTR lpFileName)
{ return unlink(lpFileName)==0; }
#endif

////////////////////////////////////// issuer and serial number /////////////////////////////////////////
void t_asn_IssuerAndSerialNumber::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  load_sequence_of(str, &issuer);  if (str->any_error()) return;

  serial_number = new t_asn_integer_univ;  if (!serial_number) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  serial_number->load(str);          
}

void t_asn_IssuerAndSerialNumber::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));
 // save fields:
  save_sequence_of(str, issuer);
  serial_number->save(str);
}

unsigned t_asn_IssuerAndSerialNumber::der_length(bool outer) const
{ unsigned total_length = der_length_sequence(issuer) + serial_number->der_length(true);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

bool t_asn_IssuerAndSerialNumber::assign(t_asn_Name * issuerIn, t_asn_integer_univ * serial_numberIn)
{ issuer=issuerIn;  serial_number=serial_numberIn; 
  return issuer && serial_number;
}
///////////////////////////////////////////////// signer info /////////////////////////////////////////////
t_asn_signer_info::t_asn_signer_info(void)
{ Version=NULL;  IssuerAndSerialNumber=NULL;  digestAlgorithm=NULL;  authenticatedAttributes=NULL;  digestEncryptionAlgorithm=NULL;
  encryptedDigest=NULL;  unauthenticatedAttributes=NULL;  next=NULL;
}

t_asn_signer_info::~t_asn_signer_info(void)
{ if (Version) delete Version;  if (IssuerAndSerialNumber) delete IssuerAndSerialNumber;  if (digestAlgorithm) delete digestAlgorithm;  
  if (authenticatedAttributes) delete authenticatedAttributes;  if (digestEncryptionAlgorithm) delete digestEncryptionAlgorithm;
  if (encryptedDigest) delete encryptedDigest;  if (unauthenticatedAttributes) delete unauthenticatedAttributes;
  if (next) delete next;
}

unsigned t_asn_signer_info::der_length(bool outer) const
{ unsigned encdiglength;
  encdiglength = encryptedDigest->der_length(true);
#ifndef PK_BITSTRING  // add octet_string envelope
  encdiglength += tag_length(tag_universal_bit_string) + length_of_length(encdiglength);
#endif
  unsigned total_length = Version->der_length(true) + IssuerAndSerialNumber->der_length(true) + digestAlgorithm->der_length(true) + 
    digestEncryptionAlgorithm->der_length(true) + encdiglength;
  if (authenticatedAttributes) total_length += der_length_set(authenticatedAttributes, tag_kontext_constructed_0); // OPTIONAL
  if (unauthenticatedAttributes) total_length += der_length_set(unauthenticatedAttributes, tag_kontext_constructed_1); // OPTIONAL
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_asn_signer_info::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  Version->save(str);
  IssuerAndSerialNumber->save(str);
  digestAlgorithm->save(str);
  if (authenticatedAttributes) save_set_of(str, authenticatedAttributes, tag_kontext_constructed_0); // OPTIONAL

  digestEncryptionAlgorithm->save(str);

#ifndef PK_BITSTRING
  str->save_tag(tag_universal_octet_string);
  str->save_length(encryptedDigest->der_length(true));
#endif
  encryptedDigest->save(str);
  if (unauthenticatedAttributes) save_set_of(str, unauthenticatedAttributes, tag_kontext_constructed_1); // OPTIONAL
}

void t_asn_signer_info::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  Version = new t_asn_integer_direct;  if (!Version) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  Version->load(str);                  if (str->any_error()) return;

  IssuerAndSerialNumber = new t_asn_IssuerAndSerialNumber;  if (!IssuerAndSerialNumber) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  IssuerAndSerialNumber->load(str);                         if (str->any_error()) return;

  digestAlgorithm = new t_asn_algorithm_identifier;  if (!digestAlgorithm) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  digestAlgorithm->load(str);                        if (str->any_error()) return;

  if (str->check_tag(tag_kontext_constructed_0, false)) // OPTIONAL
    { load_set_of(str, &authenticatedAttributes, tag_kontext_constructed_0);  if (str->any_error()) return; }

  digestEncryptionAlgorithm = new t_asn_algorithm_identifier;  if (!digestEncryptionAlgorithm) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  digestEncryptionAlgorithm->load(str);                        if (str->any_error()) return;

#ifdef PK_BITSTRING
  encryptedDigest = new t_asn_octet_string;  
#else
  encryptedDigest = new t_asn_integer_indirect;
  if (!str->check_tag(tag_universal_octet_string)) { str->set_struct_error(ERR_OCTET_STRING_NOT_PRESENT);  return; }
  unsigned octet_length = str->load_length();
#endif
  if (!encryptedDigest) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  encryptedDigest->load(str);                        

  if (str->check_tag(tag_kontext_constructed_1, false)) // OPTIONAL
    { load_set_of(str, &unauthenticatedAttributes, tag_kontext_constructed_1);  if (str->any_error()) return; }
}

bool t_asn_signer_info::fill_0(const unsigned * content_object_id, unsigned content_object_id_length, const t_Certificate * certif, 
                               t_hash & hs, const SYSTEMTIME * accounting_date)
{ Version = new t_asn_integer_direct(1);  // version fixed as 1
  if (!Version) return false;
 // copy IssuerAndSerialNumber from own certificate:
  IssuerAndSerialNumber = new t_asn_IssuerAndSerialNumber;
  if (!IssuerAndSerialNumber) return false; 
  if (!IssuerAndSerialNumber->assign(copy_disting_name(certif->TbsCertificate->Issuer), certif->TbsCertificate->CertificateSerialNumber->copy()))
    return false;
 // digestAlgorithm
  digestAlgorithm = new t_asn_algorithm_identifier(hs.get_hash_algorithm_id(), hs.get_hash_algorithm_id_length());
 // digestEncryptionAlgorithm
  digestEncryptionAlgorithm = new t_asn_algorithm_identifier(hs.get_hash_algorithmRSA_id(), hs.get_hash_algorithmRSA_id_length());
  if (!digestAlgorithm || !digestEncryptionAlgorithm)
    return false;
 // attribute "content type" - object id
  authenticatedAttributes = new t_asn_attribute(attr_id_content_type, attr_id_content_type_length, 6, new t_asn_objectid(content_object_id, content_object_id_length));
  if (!authenticatedAttributes) return false;
 // attribute "content digest" - still fake
  authenticatedAttributes->next = new t_asn_attribute(attr_id_message_digest, attr_id_message_digest_length, 4, new t_asn_octet_string((const unsigned char*)certif, hs.get_hash_module()->DigestSize()));
  if (!authenticatedAttributes->next) return false;
 // attribute "signing time" - UTCTime
  authenticatedAttributes->next->next = new t_asn_attribute(attr_id_signing_time, attr_id_signing_time_length, 23, new t_asn_UTCTime(current_UTC()));
  if (!authenticatedAttributes->next->next) return false;
 // attribute "accounting date" - UTCTime - may not be specified
  if (accounting_date!=NULL)
  { authenticatedAttributes->next->next->next = new t_asn_attribute(attr_id_account_date, attr_id_account_date_length, 23, new t_asn_UTCTime(accounting_date));
    if (!authenticatedAttributes->next->next->next) return false;
  }
 // unauthentificated attribute: certificate information
  unauthenticatedAttributes = new t_asn_attribute(attr_id_content_type, attr_id_content_type_length, 6, new t_asn_objectid(content_object_id, content_object_id_length));
  return true;
}

bool t_asn_signer_info::fill_1(const unsigned char * cont_digest, unsigned cont_digest_length)
// Updates the content digest value in the AA
{ t_asn_attribute * cont_digest_atr = new t_asn_attribute(attr_id_message_digest, attr_id_message_digest_length, 4, new t_asn_octet_string(cont_digest, cont_digest_length));
  if (!cont_digest_atr) return false;
 // replace:
  t_asn_attribute * fake = authenticatedAttributes->next;
  cont_digest_atr->next = fake->next;
  authenticatedAttributes->next = cont_digest_atr;
  fake->next=NULL;  delete fake;  // must not cascade the deleting
  return true;
}

void pad_and_encrypt(const unsigned char * input, int length, unsigned char * output, const t_IDEAXCBC_keyIV & pikey)
{ IDEAXCBCEncryption ideaenc(pikey); 
  unsigned char auxbuf[IDEAXCBCEncryption::BLOCKSIZE];  unsigned offset;
 // process the not-padded blocks:
  for (offset=0;  offset+IDEAXCBCEncryption::BLOCKSIZE <= length;  offset+=IDEAXCBCEncryption::BLOCKSIZE)
    ideaenc.ProcessBlock(input+offset, output+offset);
 // process the last padded block:
  unsigned padding = IDEAXCBCEncryption::BLOCKSIZE-(length-offset);  
  memcpy(auxbuf, input+offset, length-offset);
  memset(auxbuf+length-offset, padding, padding);
  ideaenc.ProcessBlock(auxbuf, output+offset);
}

#if 0
void pad_and_encrypt(const unsigned char * input, int length, unsigned char * output, const t_DES3CBC_keyIV & pikey)
{ DES3CBCEncryption ideaenc(pikey); 
  unsigned char auxbuf[DES3CBCEncryption.BLOCKSIZE];  unsigned offset;
 // process the not-padded blocks:
  for (offset=0;  offset+DES3CBCEncryption.BLOCKSIZE <= length;  offset+=DES3CBCEncryption.BLOCKSIZE)
    ideaenc.ProcessBlock(input+offset, output+offset);
 // process the last padded block:
  unsigned padding = DES3CBCEncryption.BLOCKSIZE-(length-offset);  
  memcpy(auxbuf, input+offset, length-offset);
  memset(auxbuf+length-offset, padding, padding);
  ideaenc.ProcessBlock(auxbuf, output+offset);
}

bool decrypt_and_unpad(const unsigned char * input, int length, unsigned char * output, unsigned * decr_length, const t_DES3CBC_keyIV & pikey)
// Return false if cannot unpad -> bad decryption key used. Length is supposed to be multiply of DES3CBCEncryption.BLOCKSIZE
{ DES3CBCDecryption ideadec(pikey); 
 // process the not-padded blocks:
  *decr_length = (length - 1) / DES3CBCEncryption.BLOCKSIZE * DES3CBCEncryption.BLOCKSIZE; // incomplete!
  while (length > DES3CBCEncryption.BLOCKSIZE)
  { ideadec.ProcessBlock(input, output); // decrypt
    input+=DES3CBCEncryption.BLOCKSIZE;  output+=DES3CBCEncryption.BLOCKSIZE;
    length -= DES3CBCEncryption.BLOCKSIZE;
  }
 // process the last block with padding:
  if (length != DES3CBCEncryption.BLOCKSIZE) return false;
  unsigned char auxbuf[DES3CBCEncryption.BLOCKSIZE];  
  ideadec.ProcessBlock(input, auxbuf); // decrypt
 // check the padding:
  int padding = auxbuf[DES3CBCEncryption.BLOCKSIZE-1];
  if (padding>DES3CBCEncryption.BLOCKSIZE) return false;
  for (int i=DES3CBCEncryption.BLOCKSIZE-padding;  i<DES3CBCEncryption.BLOCKSIZE;  i++)
    if (auxbuf[i]!=padding) return false;
 // copy the last btes to output (if any):
  memcpy(output, auxbuf, DES3CBCEncryption.BLOCKSIZE-padding);
  *decr_length += DES3CBCEncryption.BLOCKSIZE-padding;
  return true;
}
#endif

bool decrypt_and_unpad(const unsigned char * input, int length, unsigned char * output, unsigned * decr_length, const t_IDEAXCBC_keyIV & pikey)
// Return false if cannot unpad -> bad decryption key used. Length is supposed to be multiply of IDEAXCBCEncryption::BLOCKSIZE
{ IDEAXCBCDecryption ideadec(pikey); 
 // process the not-padded blocks:
  *decr_length = (length - 1) / IDEAXCBCEncryption::BLOCKSIZE * IDEAXCBCEncryption::BLOCKSIZE; // incomplete!
  while (length > IDEAXCBCEncryption::BLOCKSIZE)
  { ideadec.ProcessBlock(input, output); // decrypt
    input+=IDEAXCBCEncryption::BLOCKSIZE;  output+=IDEAXCBCEncryption::BLOCKSIZE;
    length -= IDEAXCBCEncryption::BLOCKSIZE;
  }
 // process the last block with padding:
  if (length != IDEAXCBCEncryption::BLOCKSIZE) return false;
  unsigned char auxbuf[IDEAXCBCEncryption::BLOCKSIZE];  
  ideadec.ProcessBlock(input, auxbuf); // decrypt
 // check the padding:
  int padding = auxbuf[IDEAXCBCEncryption::BLOCKSIZE-1];
  if (padding>IDEAXCBCEncryption::BLOCKSIZE) return false;
  for (int i=IDEAXCBCEncryption::BLOCKSIZE-padding;  i<IDEAXCBCEncryption::BLOCKSIZE;  i++)
    if (auxbuf[i]!=padding) return false;
 // copy the last btes to output (if any):
  memcpy(output, auxbuf, IDEAXCBCEncryption::BLOCKSIZE-padding);
  *decr_length += IDEAXCBCEncryption::BLOCKSIZE-padding;
  return true;
}

bool t_asn_signer_info::preallocate(const CryptoPP::InvertibleRSAFunction * privateKey, bool encrypted)
{ unsigned xDigestSignatureLength = privateKey->MaxPreimage().ByteCount();
  if (encrypted)
     xDigestSignatureLength = xDigestSignatureLength * IDEAXCBCEncryption::BLOCKSIZE / IDEAXCBCEncryption::BLOCKSIZE + IDEAXCBCEncryption::BLOCKSIZE;
  encryptedDigest = new t_asn_octet_string((const unsigned char *)privateKey, xDigestSignatureLength);
  return encryptedDigest!=NULL;
}

bool t_asn_signer_info::sign(const CryptoPP::InvertibleRSAFunction * privateKey, const t_IDEAXCBC_keyIV * pikey, t_hash & hs)
// Does not take ownership of any parameter.
{// calculate the digest of the authenticatedAttributes:
  unsigned char full_digest[MAX_DIGEST_LENGTH];  unsigned full_digest_length;
  tMemStream mstr;  
  mstr.open_output(der_length_set(authenticatedAttributes));
  save_set_of(&mstr, authenticatedAttributes); // the IMPLICIT [0] tag must not be used here (see footnote in the PKCS 7)!
  mstr.get_digest(*hs.get_hash_module(), full_digest, &full_digest_length);
 // sign it:
  CryptoPP::Integer encdig = generic_sign(&hs, full_digest, full_digest_length, privateKey);
  unsigned xDigestSignatureLength = privateKey->MaxPreimage().ByteCount();
  CryptoPP::SecByteBlock signat(xDigestSignatureLength);
	encdig.Encode(signat, xDigestSignatureLength);
  if (pikey) // encrypt the signature by the symmetric IDEAXCBC encryption
  { unsigned encrypted_signature_length = xDigestSignatureLength * IDEAXCBCEncryption::BLOCKSIZE / IDEAXCBCEncryption::BLOCKSIZE + IDEAXCBCEncryption::BLOCKSIZE;
    CryptoPP::SecByteBlock encsignat(encrypted_signature_length);
    pad_and_encrypt(signat, xDigestSignatureLength, encsignat, *pikey);
    delete encryptedDigest;
    encryptedDigest = new t_asn_octet_string(encsignat, encrypted_signature_length);
  }
  else // store the signature directly
  { delete encryptedDigest;
    encryptedDigest = new t_asn_octet_string(signat, xDigestSignatureLength);
  }
  return encryptedDigest!=NULL;
}


const t_Certificate * find_certificate(const t_asn_IssuerAndSerialNumber * IssuerAndSerialNumber, const t_Certificate * certificates)
// Searches the certificate among packed certificates and in the database
{ const t_Certificate * certificate = certificates;    
  while (certificate && !certificate->same(*IssuerAndSerialNumber))
    certificate = certificate->next;
  if (certificate && certificate->TbsCertificate->IsRootCertificate()) certificate=NULL;  // must not used the packed-in root cert.!!
#ifdef STOP
  if (!certificate) // search the local database:
    certificate = the_dbcert->get_by_issuer_and_num(IssuerAndSerialNumber->issuer, IssuerAndSerialNumber->serial_number);
  if (!certificate) // search the local root database:
    certificate = the_root_cert->get_by_issuer_and_num(IssuerAndSerialNumber->issuer, IssuerAndSerialNumber->serial_number);
#endif
  return certificate;
}

void t_asn_signer_info::get_signature_info(const t_Certificate * certificates, char * signer_name, SYSTEMTIME * signing_date, bool * found_acc_date, SYSTEMTIME * accounting_date) const
{// get signer name from its certificate: 
  if (signer_name)
  { const t_Certificate * certificate = find_certificate(IssuerAndSerialNumber, certificates);
    if (!certificate) *signer_name=0;
    else get_readable_name(certificate->TbsCertificate->Subject, signer_name, sizeof(t_readable_rdn_name));
  }
 // get signing date from AA:
  if (signing_date)
  { t_asn_attribute * sigdate_attr = authenticatedAttributes->find(attr_id_signing_time, attr_id_signing_time_length);
    if (!sigdate_attr) { signing_date->wDay=signing_date->wMonth=signing_date->wYear=0; }
    else *signing_date = *((const t_asn_UTCTime*)sigdate_attr->get_attribute_value())->get_system_time();
  }
 // get accounting date from AA:
  if (found_acc_date)
  { t_asn_attribute * accdate_attr = authenticatedAttributes->find(attr_id_account_date, attr_id_account_date_length);
    if (!accdate_attr) *found_acc_date=false;
    else 
    { if (accounting_date) *accounting_date = *((const t_asn_UTCTime*)accdate_attr->get_attribute_value())->get_system_time();
      *found_acc_date=true;
    }
  }
}

t_VerifyResult t_asn_signer_info::verify(const t_Certificate * certificates, 
                       const unsigned char * cont_digest, unsigned cont_digest_length, const t_IDEAXCBC_keyIV * pikey) const
{// compare digest with the stored one:
  t_asn_attribute * digest_attr = authenticatedAttributes->find(attr_id_message_digest, attr_id_message_digest_length);
  if (!digest_attr) 
    return VR_STRUCT_ERROR;  
  const unsigned char * stored_cont_digest = ((const t_asn_octet_string*)digest_attr->get_attribute_value())->get_val();
  if (memcmp(cont_digest, stored_cont_digest, cont_digest_length))
    return VR_TAMPERED;
 // compare contentType:
  t_asn_attribute * continfo_attr = authenticatedAttributes->find(attr_id_content_type, attr_id_content_type_length);
  if (!continfo_attr) 
    return VR_STRUCT_ERROR;
  if (!((const t_asn_objectid*)continfo_attr->get_attribute_value())->is(data_content_object_id, data_content_object_id_length))
    return VR_TAMPERED;
 // serialize authenticated attributes:
  tMemStream mstr;  
  mstr.open_output(der_length_set(authenticatedAttributes));
  save_set_of(&mstr, authenticatedAttributes); // the IMPLICIT [0] tag must not be used here (see footnote in the PKCS 7)!
  unsigned auth_data_len = mstr.external_pos();
  unsigned char * auth_data = mstr.close_output();
 // find the certificate:
  const t_Certificate * certificate = find_certificate(IssuerAndSerialNumber, certificates);
  if (!certificate) 
    return VR_MISSING_CERTIF;
  //certificate->TbsCertificate->get_readable_owner_name(Subject, namebuf, sizeof(namebuf));
  t_keytypes keytype = certificate->TbsCertificate->get_key_usage();
  if (keytype!=KEYTYPE_SIGN && keytype!=KEYTYPE_ENCSIG && keytype!=KEYTYPE_CERTIFY && keytype!=KEYTYPE_RCA)  // message distributing the keys may be signed by the CA key!
    return VR_BAD_CERTIF_TYPE;
 // verify the certificate:
  SYSTEMTIME signing_date;
  get_signature_info(certificates, NULL, &signing_date, NULL, NULL);
  t_VerifyResult result;
  result=certificate->VerifyCertificate(certificates, NULL);
  if (result==VR_EXPIRED)
  { if (certificate->VerifyCertificate(certificates, &signing_date) == VR_OK)
#ifdef WITH_GUI
      if (MessageBox(0, "Podpis zpr�y ji nelze ov�it, lze jej vak ov�it k datu podeps��zpr�y. Souhlas�e?", "Pozor", MB_APPLMODAL | MB_YESNO | MB_ICONQUESTION)==IDYES)
#endif
        result=VR_OK;
  }
  if (result!=VR_OK) return result;
 // decrypt the signature if encrypted by the summ. alg. IDEAXCBC:
  const unsigned char * signat = encryptedDigest->get_val();  unsigned signat_length = encryptedDigest->get_length();
  CryptoPP::SecByteBlock dec_signat(signat_length);
  if (pikey)
  { unsigned decr_length;
    if (!decrypt_and_unpad(signat, signat_length, dec_signat, &decr_length, *pikey))
      return VR_CANNOT_DECRYPT_SIGNATURE;
    signat = dec_signat;   signat_length = decr_length;
  }
 // verify the signature using the certificate:
  if (!certificate->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey->verify_signature(signat, signat_length, auth_data, auth_data_len))
    return VR_TAMPERED_AA;
  return VR_OK;
}
////////////////////////////////////////////////// content info /////////////////////////////////////////////////////
t_asn_ContentInfo::t_asn_ContentInfo(void)
{ contentType=NULL;  content=NULL;  alt_content=NULL; }

t_asn_ContentInfo::~t_asn_ContentInfo(void)
{ if (contentType) delete contentType;  if (content) delete content; 
  //if (alt_content) delete alt_content; -- no ownership
}

unsigned t_asn_ContentInfo::der_length(bool outer) const
{ unsigned total_length = contentType->der_length(true);
  if (content)     // OPTIONAL EXPLICIT [0]   (explicit is always constructed)
    total_length += tag_length(tag_kontext_constructed_0) + length_of_length(content->der_length(true)) + // EXPLICIT tag
                    content->der_length(true);
  else if (alt_content)
  { unsigned content_length = alt_content->extent();
    content_length += tag_length(tag_universal_octet_string) + length_of_length(content_length); // OCTET STRING encapsulation
    content_length += tag_length(tag_kontext_constructed_0)  + length_of_length(content_length); // EXPLICIT tag
    total_length += content_length;
  }                  
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_asn_ContentInfo::save(tStream * str, CryptoPP::HashModule & md, unsigned char * cont_digest, unsigned * cont_digest_length) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  contentType->save(str);
  if (content)              // OPTIONAL EXPLICIT [0]   (explicit is always constructed)
  { tMemStream mstr;  
    mstr.open_output(content->der_length(true));
    content->save(&mstr);
    mstr.get_digest(md, cont_digest, cont_digest_length);
    str->save_tag(tag_kontext_constructed_0);  str->save_length(content->der_length(true));  // EXPLICIT tag
    content->save(str);
  }
  else if (alt_content)     // OPTIONAL EXPLICIT [0]   (explicit is always constructed)
  { alt_content->get_digest(md, cont_digest, cont_digest_length);
    unsigned content_length = alt_content->extent();
    str->save_tag(tag_kontext_constructed_0);  // EXPLICIT tag 
    str->save_length(content_length + tag_length(tag_universal_octet_string) + length_of_length(content_length));  
    str->save_tag(tag_universal_octet_string);  str->save_length(content_length);  // OCTET STRING encaps.
    alt_content->copy(str);
  }
}

void t_asn_ContentInfo::load(tStream * str, tStream * dataStream, CryptoPP::HashModule * md, unsigned char * cont_digest)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  contentType = new t_asn_objectid;  if (!contentType) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  contentType->load(str);            if (str->any_error()) return;
  if (str->check_tag(tag_kontext_constructed_0)) // OPTIONAL [0] EXPLICIT (explicit is always constructed)
  { unsigned content_length = str->load_length();  // ignored
    if (dataStream)
    { if (!str->check_tag(tag_universal_octet_string)) 
        { str->set_struct_error(ERR_OCTET_STRING_NOT_PRESENT);  return; }
      unsigned octet_length = str->load_length();
      stream_copy(str, dataStream, octet_length, md);
      md->Final(cont_digest);
      alt_content=dataStream;
      if (octet_length==LENGTH_INDEFINITE) str->load_end_of_contents();
    }
    else
    { content = new t_asn_octet_string;  if (!content) { str->set_struct_error(ERR_NO_MEMORY);  return; }
      content->load(str);                if (str->any_error()) return;
    }
    if (content_length==LENGTH_INDEFINITE) str->load_end_of_contents();
  }
}

bool t_asn_ContentInfo::fill(const unsigned * content_object_id, unsigned content_object_id_length, const tStream * content_str)
{ contentType = new t_asn_objectid(content_object_id, content_object_id_length);
  alt_content = content_str;  // no ownership
  return contentType!=NULL;
}

////////////////////////////////////////////////// signed message ///////////////////////////////////////////////////
t_asn_SignedData::t_asn_SignedData(void) : hs(NULL)
{ version=NULL;  digestAlgorithms=NULL;  contentInfo=NULL;  certificates=NULL;  clrs=NULL;  signerInfos=NULL; }

t_asn_SignedData::~t_asn_SignedData(void)
{ if (version) delete version;  if (digestAlgorithms) delete digestAlgorithms;  if (contentInfo) delete contentInfo;
  if (certificates) delete certificates;  if (clrs) delete clrs;  if (signerInfos) delete signerInfos;
}

unsigned t_asn_SignedData::der_length(bool outer) const
{ unsigned total_length = version->der_length(true) + der_length_set(digestAlgorithms) + contentInfo->der_length(true) +
    der_length_set(signerInfos);
  if (certificates) total_length += der_length_sequence(certificates, tag_kontext_constructed_0); // OPTIONAL
  if (clrs)         total_length += der_length_set     (clrs,         tag_kontext_constructed_1); // OPTIONAL
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_asn_SignedData::save(tStream * str, const t_Certificate * certif, const CryptoPP::InvertibleRSAFunction * privateKey) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  version->save(str);
  save_set_of(str, digestAlgorithms);
  unsigned char cont_digest[MAX_DIGEST_LENGTH];  unsigned cont_digest_length;  
  contentInfo->save(str, *hs.get_hash_module(), cont_digest, &cont_digest_length);
 // add digest:
  signerInfos->fill_1(cont_digest, cont_digest_length);
  if (certificates) save_sequence_of(str, certificates, tag_kontext_constructed_0); // OPTIONAL
  if (clrs        ) save_set_of     (str, clrs,         tag_kontext_constructed_1); // OPTIONAL
 // create and save the signature:
  signerInfos->sign(privateKey, NULL, (t_hash &)hs);
  save_set_of(str, signerInfos);
}

void t_asn_SignedData::save_ci(tStream * str, const t_Certificate * certif, const CryptoPP::InvertibleRSAFunction * privateKey) const
{ signerInfos->preallocate(privateKey, false);
  unsigned outer_content_length = der_length(true);
  str->save_tag(tag_universal_constructed_sequence);
  str->save_length(signed_data_content_object.der_length(true)
    +tag_length(tag_kontext_constructed_0) + length_of_length(outer_content_length) + // EXPLICIT tag length
    +outer_content_length);  // content length
 // save object id - signed data
  signed_data_content_object.save(str);  // signed data contentType
  str->save_tag(tag_kontext_constructed_0);  str->save_length(outer_content_length);  // EXPLICIT tag
  save(str, certif, privateKey);  // saves the content
}

void t_asn_SignedData::load(tStream * str, tStream * dataStream)
// Copies data to dataStream, calculates the digest
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  unsigned length = str->load_length();
 // load fields:
  version = new t_asn_integer_direct;   if (!version) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  version->load(str);                   if (str->any_error()) return;

  load_set_of(str, &digestAlgorithms);  if (str->any_error()) return;
  t_hash hs(digestAlgorithms->algorithm_type());  
  if (hs.is_error())
    { str->set_struct_error(ERR_BAD_ALGORITHM);  return; }
  CryptoPP::HashModule * md = hs.get_hash_module();
  lcont_digest_length = md->DigestSize();

  contentInfo = new t_asn_ContentInfo;  if (!contentInfo) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  contentInfo->load(str, dataStream, md, lcont_digest);  if (str->any_error()) return;

  if (str->check_tag(tag_kontext_constructed_0, false)) // OPTIONAL
    { load_sequence_of(str, &certificates, tag_kontext_constructed_0);  if (str->any_error()) return; }
  if (str->check_tag(tag_kontext_constructed_1, false)) // OPTIONAL
    { load_set_of     (str, &clrs,         tag_kontext_constructed_1);  if (str->any_error()) return; }

  load_set_of(str, &signerInfos);
  if (length==LENGTH_INDEFINITE) str->load_end_of_contents();
}

void t_asn_SignedData::load_ci(tStream * str, tStream * dataStream)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  unsigned length1 = str->load_length();
 // load contentType:
  t_asn_objectid contentType;
  contentType.load(str);  if (str->any_error()) return;
  if (!contentType.is(signed_data_content_object_id, signed_data_content_object_id_length))
    { str->set_struct_error(ERR_WRONG_CONTENT_TYPE);  return; }
 // load content:
  if (str->check_tag(tag_kontext_constructed_0)) // OPTIONAL [0] EXPLICIT (explicit is always constructed)
  { unsigned length2 = str->load_length();
    load(str, dataStream);
    if (length2==LENGTH_INDEFINITE) str->load_end_of_contents();
  }
  if (length1==LENGTH_INDEFINITE) str->load_end_of_contents();
}

bool t_asn_SignedData::fill(const unsigned * content_object_id, unsigned content_object_id_length, const SYSTEMTIME * account_date,
                            const tStream * content_str, const t_Certificate * certif)
// Does not take the ownership of content_str!
{ version = new t_asn_integer_direct(1);  // version fixed as 1  
  if (!version) return false;
 // digestAlgorithms
  digestAlgorithms = new t_asn_algorithm_identifier(hs.get_hash_algorithm_id(), hs.get_hash_algorithm_id_length());
  if (!digestAlgorithms) return false;
 // content info:
  contentInfo = new t_asn_ContentInfo;
  if (!contentInfo) return false;
  if (!contentInfo->fill(content_object_id, content_object_id_length, content_str)) return false;
 // signer info:
  signerInfos = new t_asn_signer_info;
  if (!signerInfos) return false;
  signerInfos->fill_0(content_object_id, content_object_id_length, certif, hs, account_date); 
  return true;
}

void t_asn_SignedData::get_signature_info(char * signer_name, SYSTEMTIME * signing_date, bool * found_acc_date, SYSTEMTIME * accounting_date)
{ signerInfos->get_signature_info(certificates, signer_name, signing_date, found_acc_date, accounting_date); }

t_VerifyResult t_asn_SignedData::VerifySignature(void)
// The digest of the content is supposed to be calculated.
// Certificate is uniquely identified
{ if (!signerInfos) return VR_NOT_SIGNED;
  t_VerifyResult result;
  t_asn_signer_info * signerInfo = signerInfos;
  do // cycle on signatures
  { result = signerInfo->verify(certificates, lcont_digest, lcont_digest_length, NULL);
   // look for the other signatures:
    signerInfo=signerInfo->next;
  } while (signerInfo && result==VR_OK);
  return result;
}

void add_certpath(const t_Certificate * cert, t_Certificate ** certpath)
// Adds the given certificate and the whole caertification path
{ t_Certificate * cert_copy;
  do
  {// make the copy of the certificate:
    cert_copy = cert->copy();
    if (!cert_copy) return;
   // add the copy to the list:
    cert_copy->next = *certpath;  *certpath = cert_copy;
   // go to the upper certificate, unless root:
    if (cert->TbsCertificate->IsRootCertificate()) break;
    const t_Certificate * upper_cert = the_dbcert->get_by_subject(cert->TbsCertificate->Issuer, KEYTYPE_CERTIFY); 
    if (!upper_cert) 
      upper_cert = the_root_cert->get_by_subject(cert->TbsCertificate->Issuer, KEYTYPE_RCA); 
    if (!upper_cert) return;
    cert=upper_cert;
  } while (true); 
}

bool make_signed_message(const tStream * content_str, const SYSTEMTIME * account_date, const char * output_pathname, HWND hParent, bool pack_certificates)
// Makes a data-message from a stream, signs with the current key
{ t_asn_SignedData signedData;  

  tFileStream fstr;
  if (!fstr.open_output(output_pathname))
    { show_error("Nemohu otev� vstupn�soubor", hParent);  return false; }
 // activate the sign key on the beginning in order to prevent interaction in the middle of the processing:
  bool temporary_key;t_Certificate * sign_certif;  CryptoPP::InvertibleRSAFunction * sign_private_key;
  sign_certif=NULL;  sign_private_key=NULL;
  if (current_sign_certif && current_sign_private_key)
    { sign_certif=current_sign_certif;  sign_private_key=current_sign_private_key;  temporary_key=false; }
  else // load the temporary key
  { activate_private_key(KEYTYPE_SIGN, hParent, &sign_private_key, &sign_certif);
    if (!sign_certif || !sign_private_key) 
      { show_error("Nelze vytvoit podepsanou zpr�u protoe nebyl aktivov� podpisov kl�", hParent);  return false; }
    temporary_key=true;
  }
 // verify the signature certificate (prevent receiver's problem when outdated certificate used):
  if (sign_certif->VerifyCertificate(NULL, NULL) // no signing time available, verifying to current time
      !=VR_OK) 
    { show_error("Podpisov kl� nen�platn", hParent);  return false; }
 // defining the message
  signedData.fill(data_content_object_id, data_content_object_id_length, account_date, content_str, sign_certif);
 // signing
  if (pack_certificates)
  {// add certificates:
    add_certpath(sign_certif, &signedData.certificates);
   // add clrs:         //###   // pozor: ne v zadosti o certifikat!
  }
 // output the message:
  signedData.save_ci(&fstr, sign_certif, sign_private_key);
  fstr.close_output();
 // delete temporary key:
  if (temporary_key) 
    { delete sign_private_key;  delete sign_certif; }
  if (fstr.any_error())
  { show_error("Chyba pi z�isu do vstupn�o souboru", hParent);  
    securDeleteFile(output_pathname);
    return false; 
  }
  return true;
}
///////////////////////////////////////////////////// ENCRYPTED MSGS //////////////////////////////////////////////////////
///////////////////////////////////////////////// encrypted content info //////////////////////////////////////////////////
t_asn_EncryptedContentInfo::t_asn_EncryptedContentInfo(void)
{ contentType=NULL;  contentEncryptionAlgoritm=NULL;  encryptedContent=NULL;  alt_content=NULL; }

t_asn_EncryptedContentInfo::~t_asn_EncryptedContentInfo(void)
{ if (contentType) delete contentType;  if (contentEncryptionAlgoritm) delete contentEncryptionAlgoritm;  
  if (encryptedContent) delete encryptedContent;  
}

unsigned t_asn_EncryptedContentInfo::der_length(bool outer) const
{ unsigned total_length = contentType->der_length(true) + contentEncryptionAlgoritm->der_length(true);
  unsigned encrypted_size = (alt_content->extent() / IDEAXCBCEncryption::BLOCKSIZE) * IDEAXCBCEncryption::BLOCKSIZE + IDEAXCBCEncryption::BLOCKSIZE;  // must use the encrypted size
  if (encryptedContent) total_length += encryptedContent->der_length(true, tag_kontext_primitive_0);   // OPTIONAL [0] IMPLICIT - bad
  else if (alt_content) total_length += tag_length(tag_kontext_primitive_0) + length_of_length(encrypted_size) + // IMPLICIT tag, but external to alt_content
                                        encrypted_size;
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_asn_EncryptedContentInfo::save(tFileStream * str, IDEAXCBCEncryption & ideaenc, CryptoPP::HashModule & md, 
                                      unsigned char * cont_digest, unsigned * cont_digest_length) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  contentType->save(str);
  contentEncryptionAlgoritm->save(str);
 // save encrypting and digesting:
  if (encryptedContent) 
  { tMemStream mstr;  
    mstr.open_output(encryptedContent->der_length(true));
    encryptedContent->save(&mstr);
    mstr.get_digest(md, cont_digest, cont_digest_length);
    encryptedContent->save(str, tag_kontext_primitive_0);
  }
  else if (alt_content)     // OPTIONAL IMPLICIT [0]
  { str->save_tag(tag_kontext_primitive_0);  
    unsigned encrypted_size = (alt_content->extent() / IDEAXCBCEncryption::BLOCKSIZE) * IDEAXCBCEncryption::BLOCKSIZE + IDEAXCBCEncryption::BLOCKSIZE;  // must use the encrypted size
    str->save_length(encrypted_size);  
    alt_content->copy_encrypting_digesting(str, ideaenc, md, cont_digest, cont_digest_length);
  }
}

void t_asn_EncryptedContentInfo::load(tStream * str, t_IDEAXCBC_keyIV & ikey, tFileStream * dataStream, CryptoPP::HashModule * md, unsigned char * cont_digest)
// ikey is incomplete on entry!
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  contentType = new t_asn_objectid;  if (!contentType) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  contentType->load(str);            if (str->any_error()) return;

  contentEncryptionAlgoritm = new t_asn_algorithm_identifier;  if (!contentEncryptionAlgoritm) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  contentEncryptionAlgoritm->load(str);                        if (str->any_error()) return;
  if (contentEncryptionAlgoritm->algorithm_type() != ALG_IDEAXEX)
    { str->set_struct_error(ERR_BAD_ALGORITHM);  return; }
  const unsigned char * IV = contentEncryptionAlgoritm->get_IDEAXEX_IV();
  if (!IV)
    { str->set_struct_error(ERR_BAD_ALGORITHM);  return; }

 // prepare decryptor for the content:
  memcpy(ikey.IV, IV, IDEAXCBCEncryption::BLOCKSIZE);
  IDEAXCBCDecryption ideadec(ikey); 
  if (str->check_tag(tag_kontext_primitive_0, false)) // OPTIONAL [0] IMPLICIT
  { if (dataStream)
    { str->check_tag(tag_kontext_primitive_0);  // skips the tag
      unsigned content_length = str->load_length();  
      if (!stream_copy_decrypt(str, dataStream, content_length, md, ideadec))
        { str->set_struct_error(ERR_CANNOT_DECRYPT);  return; }
      md->Final(cont_digest);
      alt_content=dataStream;
    }
    else
    { encryptedContent = new t_asn_octet_string;  if (!encryptedContent) { str->set_struct_error(ERR_NO_MEMORY);  return; }
      encryptedContent->load(str, tag_kontext_primitive_0); 
    }
  }
}

bool t_asn_EncryptedContentInfo::fill(const unsigned * content_object_id, unsigned content_object_id_length, const tFileStream * content_str, const unsigned char * IV)
{ contentType = new t_asn_objectid(content_object_id, content_object_id_length);
  if (contentType==NULL) return false; 
  contentEncryptionAlgoritm = new t_asn_algorithm_identifier(algorithm_id_ideaxex3, algorithm_id_ideaxex3_length);
  if (contentEncryptionAlgoritm==NULL) return false; 
  if (!contentEncryptionAlgoritm->set_IV_idea(IV)) return false; 
  alt_content = content_str;  // no ownership
  return true;
}

///////////////////////////////////////////////// recipient info /////////////////////////////////////////////
t_asn_RecipientInfo::t_asn_RecipientInfo(void)
{ version=NULL;  issuerAndSerialNumber=NULL;  keyEncryptionAlgorithm=NULL;  encryptedKey=NULL;  next=NULL; }

t_asn_RecipientInfo::~t_asn_RecipientInfo(void)
{ if (version) delete version;  if (issuerAndSerialNumber) delete issuerAndSerialNumber;    
  if (keyEncryptionAlgorithm) delete keyEncryptionAlgorithm;
  if (encryptedKey) delete encryptedKey;  if (next) delete next;
}

unsigned t_asn_RecipientInfo::der_length(bool outer) const
{ unsigned total_length = version->der_length(true) + issuerAndSerialNumber->der_length(true) + 
    keyEncryptionAlgorithm->der_length(true) + encryptedKey->der_length(true);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_asn_RecipientInfo::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  version->save(str);
  issuerAndSerialNumber->save(str);
  keyEncryptionAlgorithm->save(str);
  encryptedKey->save(str);
}

void t_asn_RecipientInfo::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  version = new t_asn_integer_direct;  if (!version) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  version->load(str);                  if (str->any_error()) return;

  issuerAndSerialNumber = new t_asn_IssuerAndSerialNumber;  if (!issuerAndSerialNumber) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  issuerAndSerialNumber->load(str);                         if (str->any_error()) return;

  keyEncryptionAlgorithm = new t_asn_algorithm_identifier;  if (!keyEncryptionAlgorithm) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  keyEncryptionAlgorithm->load(str);                        if (str->any_error()) return;

  encryptedKey = new t_asn_octet_string;  if (!encryptedKey) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  encryptedKey->load(str);                        
}

void MyInteger2Integer(t_asn_integer_indirect * val, CryptoPP::Integer & res)
{ tMemStream str;
  str.open_output(val->length+20);
  val->save(&str);
  byte * out = (byte*)str.close_output();
#ifdef WINS  // from 602sec95
  CryptoPP::Integer ires(CryptoPP::StringSource(out, str.extent(), true));   
#else
  CryptoPP::Integer ires;
  ires.BERDecode(out, str.extent());
#endif
  delete [] out;
  res=ires;
}

bool t_asn_RecipientInfo::fill(const t_Certificate * recip_cert, const t_IDEAXCBC_key & ikey)
// Does not take ownership of any parameter.
{ version = new t_asn_integer_direct(0);  // version fixed as 0
  if (!version) return false;
 // copy IssuerAndSerialNumber from own certificate:
  issuerAndSerialNumber = new t_asn_IssuerAndSerialNumber;
  if (!issuerAndSerialNumber) return false;
  if (!issuerAndSerialNumber->assign(copy_disting_name(recip_cert->TbsCertificate->Issuer), recip_cert->TbsCertificate->CertificateSerialNumber->copy()))
    return false;
 // keyEncryptionAlgorithm
  keyEncryptionAlgorithm = new t_asn_algorithm_identifier(algorithm_id_rsaencryption, algorithm_id_rsaencryption_length);
  if (!keyEncryptionAlgorithm) return false;
 // encrypt and store the IDEAXCBC key:
  t_asn_public_key * pubkey = recip_cert->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey;
#ifdef ORIG
  tMemStream str;
  str.open_output(pubkey->modulus->length+20);
  pubkey->modulus->save(&str);
  byte * out = (byte*)str.close_output();
  CryptoPP::Integer modul(out);   
  delete [] out;
  str.open_output(pubkey->publicExponent->length+20);
  pubkey->publicExponent->save(&str);
  out = (byte*)str.close_output();
  CryptoPP::Integer pubexp(out);  
  delete [] out;
  CryptoPP::RSAFunction rsafnc(modul, pubexp);
#else
  CryptoPP::Integer modul;
  MyInteger2Integer(pubkey->modulus, modul);
  CryptoPP::Integer pubexp;   
  MyInteger2Integer(pubkey->publicExponent, pubexp);
  CryptoPP::RSAFunction rsafnc;  rsafnc.Initialize(modul, pubexp);
#endif

  CryptoPP::PKCS_EncryptionPaddingScheme pad;
  int PaddedBlockBitLength  = rsafnc.MaxImage().BitCount()-1;
  int PaddedBlockByteLength = CryptoPP::bitsToBytes(PaddedBlockBitLength);
	CryptoPP::SecByteBlock paddedBlock(PaddedBlockByteLength);
  int CipherTextLength = rsafnc.MaxImage().ByteCount();
	CryptoPP::SecByteBlock cipherText(CipherTextLength);
  unsigned char key[20];
  SYSTEMTIME CurrentTime;  GetLocalTime(&CurrentTime);   systemtime2UTC(CurrentTime, (char*)key);
  CryptoPP::X917RNG rng(new CryptoPP::DES_EDE2_Encryption(key), key);
	pad.Pad(rng, (unsigned char*)&ikey.key, sizeof(t_IDEAXCBC_key), paddedBlock, PaddedBlockBitLength, CryptoPP::g_nullNameValuePairs);
	rsafnc.ApplyFunction(CryptoPP::Integer(paddedBlock, paddedBlock.size())).Encode(cipherText, CipherTextLength);
  encryptedKey = new t_asn_octet_string(cipherText, CipherTextLength);
  return encryptedKey!=NULL;
} 

bool t_asn_RecipientInfo::decrypt_key(const CryptoPP::InvertibleRSAFunction * enc_private_key, t_IDEAXCBC_key & ikey) const
{ unsigned xPaddedBlockBitLength = enc_private_key->MaxImage().BitCount()-1;
  unsigned xPaddedBlockByteLength = CryptoPP::bitsToBytes(xPaddedBlockBitLength);
	CryptoPP::SecByteBlock paddedBlock(xPaddedBlockByteLength);
	enc_private_key->CalculateInverse(temp_rand_pool(), CryptoPP::Integer(encryptedKey->get_val(), encryptedKey->get_length())).Encode(paddedBlock, paddedBlock.size());
  CryptoPP::PKCS_EncryptionPaddingScheme pad;
	if (pad.Unpad(paddedBlock, xPaddedBlockBitLength, (unsigned char*)&ikey.key, CryptoPP::g_nullNameValuePairs) == sizeof(t_IDEAXCBC_key))
    return true;
 // CNB non-standard padding:
  memcpy((unsigned char*)&ikey.key, ((unsigned char*)paddedBlock) + paddedBlock.size() - sizeof(t_IDEAXCBC_key), sizeof(t_IDEAXCBC_key));
  return true;
}
////////////////////////////////////////////////// signed and enveloped message ///////////////////////////////////////////////////
t_asn_SignedAndEnvelopedData::t_asn_SignedAndEnvelopedData(void) : hs(NULL)
{ version=NULL;  recipientInfos=NULL;  digestAlgorithms=NULL;  enctrypedContentInfo=NULL;  certificates=NULL;  clrs=NULL;  signerInfos=NULL; }

t_asn_SignedAndEnvelopedData::~t_asn_SignedAndEnvelopedData(void)
{ if (version) delete version;  if (recipientInfos) delete recipientInfos;  
  if (digestAlgorithms) delete digestAlgorithms;  if (enctrypedContentInfo) delete enctrypedContentInfo;
  if (certificates) delete certificates;  if (clrs) delete clrs;  if (signerInfos) delete signerInfos;
}

unsigned t_asn_SignedAndEnvelopedData::der_length(bool outer) const
{ unsigned total_length = version->der_length(true) + der_length_set(recipientInfos) + der_length_set(digestAlgorithms) + 
    enctrypedContentInfo->der_length(true) + der_length_set(signerInfos);
  if (certificates) total_length += der_length_sequence(certificates, tag_kontext_constructed_0); // OPTIONAL
  if (clrs)         total_length += der_length_set     (clrs,         tag_kontext_constructed_1); // OPTIONAL
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_asn_SignedAndEnvelopedData::save(tFileStream * str, const t_Certificate * certif, const CryptoPP::InvertibleRSAFunction * privateKey) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  version->save(str);
  save_set_of(str, recipientInfos);
  save_set_of(str, digestAlgorithms);
 // save contents encrypting and digesting:
  IDEAXCBCEncryption ideaenc(ikey); 
  unsigned char cont_digest[MAX_DIGEST_LENGTH];  unsigned cont_digest_length;  
  enctrypedContentInfo->save(str, ideaenc, *hs.get_hash_module(), cont_digest, &cont_digest_length);
 // add digest:
  signerInfos->fill_1(cont_digest, cont_digest_length);
  if (certificates) save_sequence_of(str, certificates, tag_kontext_constructed_0); // OPTIONAL
  if (clrs        ) save_set_of     (str, clrs,         tag_kontext_constructed_1); // OPTIONAL
 // create and save the signature:
  signerInfos->sign(privateKey, &ikey, (t_hash &)hs);
  save_set_of(str, signerInfos);
}

void t_asn_SignedAndEnvelopedData::save_ci(tFileStream * str, const t_Certificate * certif, const CryptoPP::InvertibleRSAFunction * privateKey) const
{ signerInfos->preallocate(privateKey, true);
  unsigned outer_content_length = der_length(true);
  str->save_tag(tag_universal_constructed_sequence);
  str->save_length(sigenv_data_content_object.der_length(true)
    +tag_length(tag_kontext_constructed_0) + length_of_length(outer_content_length) + // EXPLICIT tag length
    +outer_content_length);  // content length
 // save object id - signed and enveloped data
  sigenv_data_content_object.save(str);  // signed and enveloped data contentType
  str->save_tag(tag_kontext_constructed_0);  str->save_length(outer_content_length);  // EXPLICIT tag
  save(str, certif, privateKey);  // saves the content
}

void t_asn_SignedAndEnvelopedData::load(tStream * str, tFileStream * dataStream, HWND hParent)
{ 
#ifdef STOP
  if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  unsigned length = str->load_length();
 // load fields:
  version = new t_asn_integer_direct;  if (!version) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  version->load(str);                  if (str->any_error()) return;

  load_set_of(str, &recipientInfos);    if (str->any_error()) return;
 // decode the key for the further loading of encrypted content:
  bool temporary_key;  t_Certificate * enc_certif;  CryptoPP::InvertibleRSAFunction * enc_private_key;
  enc_certif=NULL;  enc_private_key=NULL;
  if (current_enc_certif)
    { enc_certif=current_enc_certif;  enc_private_key=current_enc_private_key;  temporary_key=false; }
  else // load the temporary key
  { activate_private_key(KEYTYPE_ENCRYPT, hParent, &enc_private_key, &enc_certif, recipientInfos);
    if (!enc_certif || !enc_private_key) 
    { str->set_struct_error(ERR_ECNRYPT_KET_NOT_ACTIVE);
      show_error("Nelze pe�st ifrovanou zpr�u protoe nebyl aktivov� deifrovac�kl�", hParent);  
      DialogBoxParam(hProgInst, MAKEINTRESOURCE(DLG_RECIPIENTS), hParent, RecipientsDlgProc, (LPARAM)recipientInfos);
      return; 
    }
    temporary_key=true;
  }
  const t_asn_RecipientInfo * recip = recipientInfos;
  while (recip && !enc_certif->same(*recip->issuerAndSerialNumber))
    recip=recip->next;
  if (!recip)
  { str->set_struct_error(ERR_ECNRYPT_KET_NOT_ACTIVE);  
    show_error("Pomoc�aktivovan�o kl�e nelze z�ilku deifrovat!", hParent);  
    DialogBoxParam(hProgInst, MAKEINTRESOURCE(DLG_RECIPIENTS), hParent, RecipientsDlgProc, (LPARAM)recipientInfos);
    goto exit; 
  }
  if (!recip->decrypt_key(enc_private_key, ikey.key))
    { str->set_struct_error(ERR_CANNOT_DECRYPT);  goto exit; }
 // continue loading:
  load_set_of(str, &digestAlgorithms);  if (str->any_error()) goto exit;
  { t_hash hs(digestAlgorithms->algorithm_type());  
    if (hs.is_error())
      { str->set_struct_error(ERR_BAD_ALGORITHM);  goto exit; }
    CryptoPP::HashModule * md = hs.get_hash_module();
    lcont_digest_length = md->DigestSize();

    enctrypedContentInfo = new t_asn_EncryptedContentInfo;  if (!enctrypedContentInfo) { str->set_struct_error(ERR_NO_MEMORY);  goto exit; }
    enctrypedContentInfo->load(str, ikey, dataStream, md, lcont_digest);  if (str->any_error()) goto exit;
  }


  if (str->check_tag(tag_kontext_constructed_0, false)) // OPTIONAL
    { load_sequence_of(str, &certificates, tag_kontext_constructed_0);  if (str->any_error()) goto exit; }
  if (str->check_tag(tag_kontext_constructed_1, false)) // OPTIONAL
    { load_set_of     (str, &clrs,         tag_kontext_constructed_1);  if (str->any_error()) goto exit; }

  load_set_of(str, &signerInfos);
  if (length==LENGTH_INDEFINITE) str->load_end_of_contents();

 exit:
  if (temporary_key) 
    { delete enc_private_key;  delete enc_certif; }
#endif
}

void t_asn_SignedAndEnvelopedData::load_ci(tStream * str, tFileStream * dataStream, HWND hParent)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  unsigned length1 = str->load_length();
 // load contentType:
  t_asn_objectid contentType;
  contentType.load(str);  if (str->any_error()) return;
  if (!contentType.is(sigenv_data_content_object_id, sigenv_data_content_object_id_length))
    { str->set_struct_error(ERR_WRONG_CONTENT_TYPE);  return; }
 // load content:
  if (str->check_tag(tag_kontext_constructed_0)) // OPTIONAL [0] EXPLICIT (explicit is always constructed)
  { unsigned length2 = str->load_length();
    load(str, dataStream, hParent);
    if (str->any_error()) return;  // must preserve the error
    if (length2==LENGTH_INDEFINITE) str->load_end_of_contents();
  }
  if (length1==LENGTH_INDEFINITE) str->load_end_of_contents();
}

#ifdef WINS
#include <wincrypt.h>
#include <time.h>
#endif

void make_IDEA_key(t_IDEAXCBC_keyIV & ikey)
{ 
#ifdef WINS
  HCRYPTPROV hProv;
  if (CryptAcquireContext(&hProv, NULL, NULL, PROV_DSS, 0))
  { CryptGenRandom(hProv, sizeof(t_IDEAXCBC_keyIV), (BYTE*)&ikey);
    CryptReleaseContext(hProv, 0);
    return;
  }
#else
  FILE *random=fopen("/dev/urandom", "r");
  if (random)
  { size_t size=fread(&ikey, sizeof(t_IDEAXCBC_keyIV), 1, random);
    fclose(random);
    if (size==sizeof(t_IDEAXCBC_keyIV)) return;
  }
#endif
  /* fallback metoda */
  srand( (unsigned)time( NULL ) );
  for (int i=0;  i<sizeof(t_IDEAXCBC_keyIV);  i++)
    ((BYTE*)&ikey)[i] = (BYTE)rand();
}

bool t_asn_SignedAndEnvelopedData::fill(const unsigned * content_object_id, unsigned content_object_id_length, const SYSTEMTIME * account_date,
                                        const tFileStream * content_str, const t_Certificate * certif)
{// prepares a random IDEAXCBC key 
  make_IDEA_key(ikey);
 // fill the structure:
  version = new t_asn_integer_direct(1);  // version fixed as 1
  if (!version) return false;
 // digestAlgorithms
  digestAlgorithms = new t_asn_algorithm_identifier(hs.get_hash_algorithm_id(), hs.get_hash_algorithm_id_length());
  if (!digestAlgorithms) return false;
 // content info:
  enctrypedContentInfo = new t_asn_EncryptedContentInfo;
  if (!enctrypedContentInfo) return false;
  if (!enctrypedContentInfo->fill(content_object_id, content_object_id_length, content_str, ikey.IV)) return false; 
 // signer info:
  signerInfos = new t_asn_signer_info;
  if (!signerInfos) return false;
  signerInfos->fill_0(content_object_id, content_object_id_length, certif, hs, account_date); 
  return true;
}

bool t_asn_SignedAndEnvelopedData::add_recipient(const t_Certificate * recip_cert)
// Creates a new recipient info, fills it with the certificate and enctrypted key and adds it to the list.
// ikey must have been generated before!
{ t_asn_RecipientInfo * recip = new t_asn_RecipientInfo;
  if (!recip) return false;
  if (!recip->fill(recip_cert, ikey.key)) { delete recip;  return false; }
  recip->next=recipientInfos;  recipientInfos=recip;
  return true;
}

class t_certif_iterator
{ t_readable_rdn_name therdblSubject;
  int phase;
  const t_Certificate * cert_to_be_searched;
  t_cert_info * ci_to_be_searched;
 public: 
  t_certif_iterator(const t_asn_Name * theSubject, const t_Certificate * working_certificatesIn)
  { get_readable_name(theSubject, therdblSubject, sizeof(therdblSubject));
    phase=0;
    cert_to_be_searched=working_certificatesIn;
  }
  const t_Certificate * get_next(void)
  {
    if (phase==0)
    { t_readable_rdn_name therdblSubject2;
      while (cert_to_be_searched)
      { const t_Certificate * cert = cert_to_be_searched;
        cert_to_be_searched = cert_to_be_searched->next;
        get_readable_name(cert->TbsCertificate->Subject, therdblSubject2, sizeof(therdblSubject2));
        if (!stricmp(therdblSubject, therdblSubject2)) return cert;
      } 
      phase=1;  ci_to_be_searched=(t_cert_info *)the_dbcert->get_list();
    }
    if (phase==1)
    { while (ci_to_be_searched)
      { t_cert_info * ci = ci_to_be_searched;
        ci_to_be_searched = ci_to_be_searched->next;
        if (!stricmp(therdblSubject, ci->rdblSubject)) 
        { if (!ci->certif)
            ci->certif = the_dbcert->load(ci->fname);
          if (ci->certif) return ci->certif;
        }
      } 
      phase=2;  ci_to_be_searched=(t_cert_info *)the_root_cert->get_list();
    }
    if (phase==2)
    { while (ci_to_be_searched)
      { t_cert_info * ci = ci_to_be_searched;
        ci_to_be_searched = ci_to_be_searched->next;
        if (!stricmp(therdblSubject, ci->rdblSubject)) 
        { if (!ci->certif)
            ci->certif = the_root_cert->load(ci->fname);
          if (ci->certif) return ci->certif;
        }
      }
    }
    return NULL;   
  }
};

t_VerifyResult t_Certificate::VerifyCertificate(const t_Certificate * working_certificates, SYSTEMTIME * signing_time, SYSTEMTIME * expir_time, SYSTEMTIME * crl_expir_time) const
// Signature in the certificate does not uniquely identify the upper certificate
// Preferred search among packed-in certificates should solve this, except for the root certificate
{
#ifdef STOP
 // verify the time validity:
  if (!TbsCertificate->Validity->is_valid_now(signing_time)) return VR_EXPIRED;

 // verify the CRL:
  t_readable_rdn_name therdblIssuer;
  get_readable_name(TbsCertificate->Issuer, therdblIssuer, sizeof(therdblIssuer));
  const t_CertificateRevocation * crl = get_crl(therdblIssuer);
  if (!crl) 
    return VR_MISSING_CRL;
  const t_RevokedCertif * rev = crl->TbsCertList->RevokedCertificates;
  while (rev)
  { if (*TbsCertificate->CertificateSerialNumber == rev->number())
      if (!signing_time || compare_systime(*rev->RevocationDateConst(), *signing_time) <= 0)
        return VR_REVOKED;
    rev=rev->next;
  }

 // find the certificate of the issuer:
  const t_Certificate * signer_cert;
#ifdef STOP
  signer_cert = working_certificates;
  while (signer_cert && !signer_cert->TbsCertificate->sameSubject(TbsCertificate->Issuer))
    signer_cert = signer_cert->next;
  if (signer_cert && signer_cert->TbsCertificate->IsRootCertificate()) signer_cert=NULL;  // must not used the packed-in root cert.!!
  if (!signer_cert) 
    signer_cert = the_dbcert->get_by_subject(TbsCertificate->Issuer, KEYTYPE_CERTIFY); 
  if (!signer_cert) 
    signer_cert = the_root_cert->get_by_subject(TbsCertificate->Issuer, KEYTYPE_RCA); 
  if (!signer_cert) return VR_MISSING_CERTIF;
 // check the type of the signer certificate:
  if (signer_cert->TbsCertificate->get_key_usage()!=KEYTYPE_CERTIFY && signer_cert->TbsCertificate->get_key_usage()!=KEYTYPE_RCA)
    return VR_BAD_CERTIF_TYPE;
 // recursive verification on the upper level:
  if (signer_cert!=this)
  { t_VerifyResult upper_result = signer_cert->VerifyCertificate(working_certificates, signing_time);
    if (upper_result!=VR_OK) return upper_result;
  }
 // verification of the current signature:
  tMemStream mstr;  
  mstr.open_output(TbsCertificate->der_length(true));
  TbsCertificate->save(&mstr);
  unsigned auth_data_len=mstr.external_pos();
  unsigned char * auth_data = (unsigned char*)mstr.close_output();
  if (!signer_cert->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey->
         verify_signature(SignatureValue->get_val(), SignatureValue->get_bitlength()/8, auth_data, auth_data_len))
    return VR_CERTIF_TAMPERED;
 // add the certificate to the database:
  if (!TbsCertificate->IsRootCertificate())
    the_dbcert->add(this, 0, NULL);
  return VR_OK;
#else
  t_certif_iterator cerit(TbsCertificate->Issuer, working_certificates);
  t_VerifyResult last_result = VR_MISSING_CERTIF;
  do
  { signer_cert=cerit.get_next();
    if (!signer_cert) return last_result;
   // recursive verification on the upper level:
    SYSTEMTIME upper_expir_time, upper_crl_expir_time;  
    if (signer_cert!=this)
    { t_VerifyResult upper_result = signer_cert->VerifyCertificate(working_certificates, signing_time, &upper_expir_time, &upper_crl_expir_time);
      if (upper_result!=VR_OK) 
        { last_result=upper_result;  continue; }
    }
    else upper_expir_time.wYear=upper_crl_expir_time.wYear=2050;
   // verification of the current signature:
    tMemStream mstr;  
    mstr.open_output(TbsCertificate->der_length(true));
    TbsCertificate->save(&mstr);
    unsigned auth_data_len=mstr.external_pos();
    unsigned char * auth_data = (unsigned char*)mstr.close_output();
    if (!signer_cert->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey->
           verify_signature(SignatureValue->get_val(), SignatureValue->get_bitlength()/8, auth_data, auth_data_len))
      last_result=VR_CERTIF_TAMPERED;
    else
    {
#ifdef STOP  // certificates from the message are added on a different place
     // add the certificate to the database:
      if (!TbsCertificate->IsRootCertificate())
        the_dbcert->add(this, 0, NULL);
#endif
     // update the minimal expiration time
      if (expir_time)
      { SYSTEMTIME local_expir_time = *TbsCertificate->Validity->get_end()->get_system_time();
        if (compare_systime(upper_expir_time, local_expir_time) < 0)
             *expir_time=upper_expir_time;
        else *expir_time=local_expir_time;
      }
      if (crl_expir_time) 
      { SYSTEMTIME st = *crl->TbsCertList->ThisUpdate->get_system_time();
        systemtime_add(st, 0, 0, 40, 0, 0, 0);
        if (compare_systime(upper_crl_expir_time, st) < 0) 
             *crl_expir_time = upper_crl_expir_time;
        else *crl_expir_time = st;
      }
      return VR_OK;
    }
  } while (TRUE);
#endif

#endif
  return VR_BAD_CERTIF_TYPE;
}

void t_asn_SignedAndEnvelopedData::get_signature_info(char * signer_name, SYSTEMTIME * signing_date, bool * found_acc_date, SYSTEMTIME * accounting_date)
{ signerInfos->get_signature_info(certificates, signer_name, signing_date, found_acc_date, accounting_date); }

t_VerifyResult t_asn_SignedAndEnvelopedData::VerifySignature(void)
// The digest of the content is supposed to be calculated.
// Certificate is uniquely identified
{ if (!signerInfos) return VR_NOT_SIGNED;
  t_VerifyResult result;
  t_asn_signer_info * signerInfo = signerInfos;
  do // cycle on signatures
  { result = signerInfo->verify(certificates, lcont_digest, lcont_digest_length, &ikey);
   // look for the other signatures:
    signerInfo=signerInfo->next;
  } while (signerInfo && result==VR_OK);
  return result;
}

#if 0
bool add_recipient_verif(t_asn_SignedAndEnvelopedData & sigenvData, const char * names)
{ const t_Certificate * recip_cert = the_dbcert->get_by_subject(names, KEYTYPE_ENCRYPT); 
  if (recip_cert)
  { sigenvData.add_recipient(recip_cert);
    if (recip_cert->VerifyCertificate(NULL, NULL) != VR_OK) 
    { char buf[100+sizeof(t_readable_rdn_name)];
      sprintf(buf, "Kl� p�emce <%s> nen�platn", names);
      show_error(buf, hMain);  return false; 
    }
  }
  return true;
}
#endif

bool make_signed_enveloped_message(const tFileStream * content_str, const SYSTEMTIME * account_date, const char * output_pathname, HWND hParent, const char * recip1name, const char * recip2name, bool pack_certificates)
// Makes a data-message from a stream, signs with the current key
{ 
#ifdef STOP
  t_asn_SignedAndEnvelopedData sigenvData;  
  tFileStream fstr;
#ifdef OVERLAPPED_IO
  if (!fstr.open_output(output_pathname, false))  // for fast access will be opened later and temporarily
#else
  if (!fstr.open_output(output_pathname, true))
#endif
    { show_error("Nemohu otev� vstupn�soubor", hParent);  return false; }
 // activate the sign key on the beginning in order to prevent interaction in the middle of the processing:
  bool temporary_key;  t_Certificate * sign_certif;  CryptoPP::InvertibleRSAFunction * sign_private_key;
  sign_certif=NULL;  sign_private_key=NULL;
  if (current_sign_certif && current_sign_private_key)
    { sign_certif=current_sign_certif;  sign_private_key=current_sign_private_key;  temporary_key=false; }
  else // load the temporary key
  { activate_private_key(KEYTYPE_SIGN, hParent, &sign_private_key, &sign_certif);
    if (!sign_certif || !sign_private_key) 
      { show_error("Nelze vytvoit podepsanou zpr�u protoe nebyl aktivov� podpisov kl�", hParent);  return false; }
    temporary_key=true;
  }
 // verify the signature certificate (prevent receiver's problem when outdated certificate used):
  if (sign_certif->VerifyCertificate(NULL, NULL) // no signing time available, verifying to current time
      !=VR_OK) 
    { show_error("Podpisov kl� nen�platn", hParent);  return false; }
 // defining the message
  sigenvData.fill(data_content_object_id, data_content_object_id_length, account_date, content_str, sign_certif);
  if (recip1name)
  { const t_Certificate * recip_cert = the_dbcert->get_by_subject(recip1name, KEYTYPE_ENCRYPT); 
    if (recip_cert)
    { sigenvData.add_recipient(recip_cert);
      if (recip_cert->VerifyCertificate(NULL, NULL) != VR_OK) 
        { show_error("Kl� p�emce nen�platn", hParent);  return false; }
    }
  }
  if (recip2name)
  { const t_Certificate * recip_cert = the_dbcert->get_by_subject(recip2name, KEYTYPE_ENCRYPT); 
    if (recip_cert)
    { sigenvData.add_recipient(recip_cert);
      if (recip_cert->VerifyCertificate(NULL, NULL) != VR_OK) 
        { show_error("Kl� p�emce nen�platn", hParent);  return false; }
    }
  }

 // add the signer as the recipient, too
  //sigenvData.add_recipient(sign_certif); -- to je blbost, potrebuji sifrovaci klic, ne podpisovy
  { t_readable_rdn_name rdblName;
    t_asn_Name * Name = create_name_list(UserUserNameList);
    get_readable_name(Name, rdblName, sizeof(rdblName));
    delete Name;
    const t_Certificate * recip_cert = the_dbcert->get_by_subject(rdblName, KEYTYPE_ENCRYPT); 
    if (recip_cert)
    { sigenvData.add_recipient(recip_cert);
      add_certpath(recip_cert, &sigenvData.certificates);
    }
  }
  if (pack_certificates)
  {// add certificates:
    add_certpath(sign_certif, &sigenvData.certificates);
   // add clrs:###
  }
 // output the message:
  sigenvData.save_ci(&fstr, sign_certif, sign_private_key);
  fstr.close_output();
 // delete temporary key:
  if (temporary_key) 
    { delete sign_private_key;  delete sign_certif; }
  if (fstr.any_error())
  { show_error("Chyba pi z�isu do vstupn�o souboru", hParent);  
    securDeleteFile(output_pathname);
    return false; 
  }
#endif
  return true;
}

#ifdef STOP
void check_signature_key_expiration(HWND hParent)
// Checks the expiration time of the signature certificate of the current user, reports deadlines.
{ t_Certificate * sign_certif;  SYSTEMTIME expir_time, crl_expir_time;
  sign_certif=get_most_recent_certificate(KEYTYPE_SIGN);
  if (!sign_certif) 
    show_error("Pihl�n uivatel nem�platn podpisov kl�", hParent);  
  else if (sign_certif->VerifyCertificate(NULL, NULL, &expir_time, &crl_expir_time)!=VR_OK) 
    show_error("Podpisov kl� pihl�n�o uivatele nen�platn", hParent);  
  else  // check the expiration time
  { SYSTEMTIME CurrentTime;  char buf[100];  int margin;
    GetLocalTime(&CurrentTime);  
    if (compare_systime(CurrentTime, crl_expir_time) > 0)
      MessageBox(hParent, "POZOR: Platnost CRL k cest�podpisov�o kl�e ji vyprela!", "Warning", MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);  
    else if (compare_systime(CurrentTime, expir_time) > 0)
      MessageBox(hParent, "POZOR: Platnost podpisov�o kl�e ji vyprela!", "Warning", MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);  
    else
    { margin=GetPrivateProfileInt("Global", "CRLMargin", 0, inipathname);
      if (margin>0)
      { systemtime_add(CurrentTime, 0, 0, margin, 0, 0, 0);
        if (compare_systime(CurrentTime, crl_expir_time) > 0)
        { sprintf(buf, "POZOR: Platnost CRL k cest�podpisov�o kl�e vypr�ji %u.%u.%u!", crl_expir_time.wDay, crl_expir_time.wMonth, crl_expir_time.wYear);
          MessageBox(hParent, buf, "Warning", MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);  
        }
      }
      GetLocalTime(&CurrentTime);  
      margin=GetPrivateProfileInt("Global", "CertMargin", 7, inipathname);
      systemtime_add(CurrentTime, 0, 0, margin, 0, 0, 0);
      if (compare_systime(CurrentTime, expir_time) > 0)
      { sprintf(buf, "POZOR: Platnost podpisov�o kl�e vypr�ji %u.%u.%u!", expir_time.wDay, expir_time.wMonth, expir_time.wYear);
        MessageBox(hParent, buf, "Warning", MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);  
      }
    }
  }
  delete sign_certif; 
}
#endif
