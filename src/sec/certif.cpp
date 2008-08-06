// certif.cpp
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
#ifdef WINS
#include <windowsx.h>
#include <commctrl.h>
#endif

/////////////////////////////////////// t_asn_AuthorityKeyIdentifier ///////////////////////////////////
t_asn_AuthorityKeyIdentifier::t_asn_AuthorityKeyIdentifier(void)
{ KeyIdentifier=NULL;  CertIssuer=NULL;  CertSerialNumber=NULL; }

t_asn_AuthorityKeyIdentifier::~t_asn_AuthorityKeyIdentifier(void)
{ if (KeyIdentifier)    delete KeyIdentifier;
  if (CertIssuer)       delete CertIssuer;
  if (CertSerialNumber) delete CertSerialNumber;
}

unsigned t_asn_AuthorityKeyIdentifier::der_length(bool outer) const
{ unsigned total_length = 0;
  if (KeyIdentifier)    total_length+=KeyIdentifier   ->der_length(true);
  if (CertIssuer)       
  { unsigned len1 = der_length_sequence(CertIssuer);
    len1 += tag_length(tag_kontext_constructed_4) + length_of_length(len1);
    len1 += tag_length(tag_kontext_constructed_1) + length_of_length(len1);
    total_length+=len1;
  }
  if (CertSerialNumber) total_length+=CertSerialNumber->der_length(true);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_asn_AuthorityKeyIdentifier::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));
 // save fields:
  if (KeyIdentifier)    KeyIdentifier   ->save(str, tag_kontext_primitive_0);  // OPTIONAL [0] IMPLICIT
  if (CertIssuer)       
  {// IMPL [1] SEQUENCE OF CHOICE IMPL [4] SEQUENCE OF replaced by EXPL [1] EXPL [4]:
    unsigned len1 = der_length_sequence(CertIssuer);
    unsigned len2 = len1 + tag_length(tag_kontext_constructed_4) + length_of_length(len1);
    str->save_tag(tag_kontext_constructed_1);  str->save_length(len2);  // EXPLICIT tag [1]
    str->save_tag(tag_kontext_constructed_4);  str->save_length(len1);  // EXPLICIT tag [4]
    save_sequence_of(str, CertIssuer);
  }
  if (CertSerialNumber) CertSerialNumber->save(str, tag_kontext_primitive_2);  // OPTIONAL [2] IMPLICIT
}

void t_asn_AuthorityKeyIdentifier::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  if (str->check_tag(tag_kontext_primitive_0, false)) // OPTIONAL [0] IMPLICIT
  { KeyIdentifier = new t_asn_octet_string;  if (!KeyIdentifier) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    KeyIdentifier->load(str, tag_kontext_primitive_0); 
  }
  if (str->check_tag(tag_kontext_constructed_1)) // OPTIONAL [1] IMPLICIT changed to EXPLICIT
  { str->load_length();  // ignored
    if (str->check_tag(tag_kontext_constructed_4)) // OPTIONAL [4] IMPLICIT changed to EXPLICIT
    { str->load_length();  // ignored
      load_sequence_of(str, &CertIssuer); 
      if (str->any_error()) return;
    }
    else { str->set_struct_error(ERR_UNSUPPORTED);  return; }
  }
  if (str->check_tag(tag_kontext_primitive_2, false)) // OPTIONAL [2] IMPLICIT
  { CertSerialNumber = new t_asn_integer_univ;  if (!CertSerialNumber) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    CertSerialNumber->load(str, tag_kontext_primitive_2); 
  }

}
/////////////////////////////////////// t_asn_Extensions ///////////////////////////////////////////////
t_asn_Extensions::t_asn_Extensions(void)
{ extension_type  = NULL;  extension_bool=NULL;  extension_value = NULL;  next=NULL; }

t_asn_Extensions::t_asn_Extensions(const unsigned * object_id, unsigned object_id_length, const unsigned char * value, unsigned value_length, t_asn_boolean * bval)
{ extension_type  = new t_asn_objectid(object_id, object_id_length);
  extension_bool  = bval;
  extension_value = new t_asn_octet_string(value, value_length);
  next=NULL;
}

const unsigned char * t_asn_Extensions::find_extension_value(const unsigned * type_id, unsigned type_id_length, unsigned * value_length) const
{ if (extension_type->is(type_id, type_id_length))
    { *value_length=extension_value->get_length();  return extension_value->get_val(); }
  if (next) return next->find_extension_value(type_id, type_id_length, value_length);
  return NULL;
}

t_asn_Extensions::~t_asn_Extensions(void)
{ if (extension_type ) delete extension_type;
  if (extension_bool ) delete extension_bool;
  if (extension_value) delete extension_value;
  if (next)            delete next;
}

void t_asn_Extensions::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  extension_type = new t_asn_objectid;  if (!extension_type) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  extension_type->load(str);            if (str->any_error()) return;
 // load the optional boolean:
  if (str->check_tag(tag_universal_boolean, false)) // OPTIONAL 
  { extension_bool = new t_asn_boolean;   if (!extension_bool) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    extension_bool->load(str);            if (str->any_error()) return;
  }
 // load the attribute value
  extension_value = new t_asn_octet_string;  if (!extension_value) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  extension_value->load(str);          
}

void t_asn_Extensions::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));
 // save fields:
  extension_type->save(str);
  if (extension_bool) extension_bool->save(str);
  extension_value->save(str);
}

unsigned t_asn_Extensions::der_length(bool outer) const
{ unsigned total_length = extension_type->der_length(true) + extension_value->der_length(true);
  if (extension_bool) total_length += extension_bool->der_length(true);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

t_asn_Extensions * t_asn_Extensions::copy(void) const  // copies the full sequence
{ t_asn_Extensions * ext = new t_asn_Extensions;
  if (ext)
  { ext->extension_type = extension_type->copy();
    if (extension_bool)
      ext->extension_bool = extension_bool->copy();
    ext->extension_value = extension_value->copy();
    if (next) ext->next = next->copy();
    if (ext->extension_type && ext->extension_value && (!next || ext->next))
      return ext;
    delete ext;
  }
  return NULL;
}
//////////////////////////////////////// t_TBSCertificate //////////////////////////////////////////////
t_TBSCertificate::t_TBSCertificate(void)
{ Version=NULL;   CertificateSerialNumber=NULL;  Signature=NULL;  Issuer=NULL;  Validity=NULL;  Subject=NULL;  
  SubjectPublicKeyInfo=NULL;  Extensions=NULL;  AuthKeyId=NULL;
}

t_TBSCertificate::~t_TBSCertificate(void)
{ if (Version) delete Version;  if (CertificateSerialNumber) delete CertificateSerialNumber;  if (Signature) delete Signature;  
  if (Issuer)  delete Issuer;   if (Validity) delete Validity;  if (Subject) delete Subject;  
  if (SubjectPublicKeyInfo) delete SubjectPublicKeyInfo;  if (Extensions) delete Extensions;
  if (AuthKeyId) delete AuthKeyId;
}

unsigned t_TBSCertificate::der_length(bool outer) const
{ unsigned total_length = CertificateSerialNumber->der_length(true) + Signature->der_length(true) +
      der_length_sequence(Issuer) + Validity->der_length(true) + der_length_sequence(Subject) +
      SubjectPublicKeyInfo->der_length(true);
  if (Version)     // OPTIONAL EXPLICIT [0] (explicit is always constructed)
    total_length += tag_length(tag_kontext_constructed_0) + length_of_length(Version->der_length(true)) + // EXPLICIT tag
                    Version->der_length(true);
  if (Extensions)  // OPTIONAL EXPLICIT [3] (explicit is always constructed)
    total_length += tag_length(tag_kontext_constructed_3) + length_of_length(der_length_sequence(Extensions)) + // EXPLICIT tag
                    der_length_sequence(Extensions);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_TBSCertificate::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  if (Version)     // OPTIONAL EXPLICIT [0] (explicit is always constructed)
  { str->save_tag(tag_kontext_constructed_0);  str->save_length(Version->der_length(true));  // EXPLICIT tag
    Version->save(str);
  }
  CertificateSerialNumber->save(str);
  Signature->save(str);
  save_sequence_of(str, Issuer);
  Validity->save(str);
  save_sequence_of(str, Subject);
  SubjectPublicKeyInfo->save(str);
 // AuthKeyId is supposed to be converted into an extension, save the Extensions only:
  if (Extensions)  // OPTIONAL EXPLICIT [3] (explicit is always constructed)
  { str->save_tag(tag_kontext_constructed_3);  str->save_length(der_length_sequence(Extensions));  // EXPLICIT tag
    save_sequence_of(str, Extensions);
  }
}
void t_TBSCertificate::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  if (str->check_tag(tag_kontext_constructed_0)) // OPTIONAL [0] EXPLICIT  (explicit is always constructed)
  { str->load_length();  // ignored
    Version = new t_asn_integer_direct;  if (!Version) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    Version->load(str);                  if (str->any_error()) return;
  }

  CertificateSerialNumber = new t_asn_integer_univ;    if (!CertificateSerialNumber) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  CertificateSerialNumber->load(str);                  if (str->any_error()) return;

  Signature = new t_asn_algorithm_identifier;  if (!Signature) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  Signature->load(str);                        if (str->any_error()) return;

  load_sequence_of(str, &Issuer); 
  if (str->any_error()) return;

  Validity = new t_asn_Validity;  if (!Validity) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  Validity->load(str);              if (str->any_error()) return;

  load_sequence_of(str, &Subject); 
  if (str->any_error()) return;

  SubjectPublicKeyInfo = new t_asn_SubjectPublicKeyInfo;  if (!SubjectPublicKeyInfo) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  SubjectPublicKeyInfo->load(str);                        if (str->any_error()) return;

  if (str->check_tag(tag_kontext_constructed_3)) // OPTIONAL EXPLICIT [3] (explicit is always constructed)
  { str->load_length();  // ignored
    load_sequence_of(str, &Extensions);
   // find the "authority key identifier" and analyse its contents:
    unsigned value_length;
    const unsigned char * AuthKeyIdStr = Extensions->find_extension_value(exten_id_AutKeyId,  exten_id_length, &value_length);
    if (AuthKeyIdStr && value_length) // extension found and its value is not empty
    { tMemStream mstr;  
      mstr.open_input((unsigned char *)AuthKeyIdStr, value_length);
      AuthKeyId = new t_asn_AuthorityKeyIdentifier;  if (!AuthKeyId) { str->set_struct_error(ERR_NO_MEMORY);  return; }
      AuthKeyId->load(&mstr);                        
      if (mstr.any_error()) { str->set_struct_error(mstr.any_error());  return; }

    }
  }
}

bool get_readable_name(const t_asn_Name * name, char * buf, unsigned bufsize) // returns true if OK, false on overrun
{ bool overrun=false, first=true;
  while (name)
  { if (first) first=false;
    else if (bufsize<2) return false;
    else { *buf=';';  buf++;  *buf=0;  bufsize--; }
    if (!name->get_readableRDN(buf, bufsize)) { overrun=true;  break; }
    name=name->next;
  }
  *buf=0;
  return !overrun;
}

bool get_readable_name_short(const t_asn_Name * name, char * buf, unsigned bufsize) // returns true if OK, false on overrun
{ bool overrun=false;  char * buf2=buf;
  while (name)
  { buf2 = buf;
    if (!name->get_readableRDN(buf2, bufsize)) { overrun=true;  break; }
    if (!memcmp(buf, "CN=\"", 3)) break;
    name=name->next;
  }
  *buf2=0;
  int len=(int)strlen(buf);
  memmove(buf, buf+4, len-4);
  buf[len-5]=0;
  return !overrun;
}

void t_TBSCertificate::get_short_descr(char * buf) const
{ const t_asn_Name * name = Issuer;
  while (name->next) name=name->next;
  name->get_strval(buf);
  int len = (int)strlen(buf);
  buf[len++] = ' ';
  buf[len++] = '#';
  CertificateSerialNumber->print_hex(buf+len);
}

bool t_TBSCertificate::sameSubject(const t_asn_Name * aName) const
{ return Subject->same(*aName); }

bool t_TBSCertificate::IsRootCertificate(void) const
{ return sameSubject(Issuer); }

t_keytypes t_TBSCertificate::get_key_usage(void)
{ if (!Extensions) return KEYTYPE_UNDEF;
  unsigned value_length;
  const unsigned char * keyUsage = Extensions->find_extension_value(exten_id_KeyUsage,  exten_id_length, &value_length);
  if (!keyUsage || value_length!=keyUsageOctetLength)          return KEYTYPE_UNDEF;
  if (!memcmp(keyUsage, keyUsageEncrypt, keyUsageOctetLength)) return KEYTYPE_ENCRYPT;
  if (!memcmp(keyUsage, keyUsageSign,    keyUsageOctetLength)) return KEYTYPE_SIGN;
  if (!memcmp(keyUsage, keyUsageSignEnc, keyUsageOctetLength)) return KEYTYPE_ENCSIG;
  if (!memcmp(keyUsage, keyUsageCA,      keyUsageOctetLength) || !memcmp(keyUsage, keyUsageCA2, keyUsageOctetLength) || 
      !memcmp(keyUsage, keyUsageCA3,     keyUsageOctetLength)) 
    return IsRootCertificate() ? KEYTYPE_RCA : KEYTYPE_CERTIFY;
  return KEYTYPE_UNDEF;
}

t_TBSCertificate * t_TBSCertificate::copy(void) const
{ t_TBSCertificate * tbscert = new t_TBSCertificate;
  if (tbscert)
  { if (Version) tbscert->Version = Version->copy();  // optional!
    tbscert->CertificateSerialNumber = CertificateSerialNumber->copy();
    tbscert->Signature = Signature->copy();
    tbscert->Issuer = Issuer->copy_seq();
    tbscert->Validity = Validity->copy();
    tbscert->Subject = Subject->copy_seq();
    tbscert->SubjectPublicKeyInfo = SubjectPublicKeyInfo->copy();
    tbscert->Extensions = Extensions->copy();
    if (tbscert->CertificateSerialNumber && tbscert->Signature && tbscert->Issuer && tbscert->Validity &&
        tbscert->Subject && tbscert->SubjectPublicKeyInfo && tbscert->Extensions)
      return tbscert;
    delete tbscert;
  }
  return NULL;
}
////////////////////////////////////// issuer and serial number /////////////////////////////////////////
t_asn_IssuerAndSerialNumber::t_asn_IssuerAndSerialNumber(void)
{ issuer  = NULL;  serial_number = NULL; }

t_asn_IssuerAndSerialNumber::~t_asn_IssuerAndSerialNumber(void)
{ if (issuer) delete issuer;  if (serial_number) delete serial_number; }
/////////////////////////////////////// t_Certificate ///////////////////////////////////////////////////////////
t_Certificate::t_Certificate(void)
{ TbsCertificate = NULL;  SignatureAlgorithm=NULL;  SignatureValue=NULL;  next=NULL; }

t_Certificate::~t_Certificate(void)
{ if (TbsCertificate)     delete TbsCertificate;
  if (SignatureAlgorithm) delete SignatureAlgorithm;
  if (SignatureValue)     delete SignatureValue;
  if (next)               delete next;
}

unsigned t_Certificate::der_length(bool outer) const
{ unsigned total_length = TbsCertificate->der_length(true) + SignatureAlgorithm->der_length(true) + SignatureValue->der_length(true);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_Certificate::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  TbsCertificate->save(str);
  SignatureAlgorithm->save(str);
  SignatureValue->save(str);
}

void t_Certificate::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  TbsCertificate = new t_TBSCertificate;
  if (!TbsCertificate) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  TbsCertificate->load(str);
  if (str->any_error()) return;

  SignatureAlgorithm = new t_asn_algorithm_identifier;
  if (!SignatureAlgorithm) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  SignatureAlgorithm->load(str);
  if (str->any_error()) return;

  SignatureValue = new t_asn_bit_string;
  if (!SignatureValue) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  SignatureValue->load(str);
}

bool t_Certificate::same(const t_asn_IssuerAndSerialNumber & certref) const
{ return *TbsCertificate->CertificateSerialNumber==*certref.serial_number &&
         TbsCertificate->Issuer->same(*certref.issuer);
}

t_Certificate * t_Certificate::copy(void) const
{ t_Certificate * cert = new t_Certificate;
  if (cert) 
  { cert->TbsCertificate     = TbsCertificate->copy();
    cert->SignatureAlgorithm = SignatureAlgorithm->copy();
    cert->SignatureValue     = SignatureValue->copy();
    if (cert->TbsCertificate && cert->SignatureAlgorithm && cert->SignatureValue)
      return cert; // OK
    delete cert;
  }
  return NULL;
}

void t_Certificate::accept(void) const
// Stores the certificate in the certificate database, unless root
{ 
  //if (!TbsCertificate->IsRootCertificate()) the_dbcert->add(this);
  if (next) next->accept();
}
//////////////////////////////////////// t_TBSCertList revocation //////////////////////////////////////////////
t_RevokedCertif::t_RevokedCertif(void)
  { CertificateSerialNumber=NULL;  RevocationDate=NULL;  Extensions=NULL;  next=NULL; }
t_RevokedCertif::~t_RevokedCertif(void)
{ if (CertificateSerialNumber) delete CertificateSerialNumber;
  if (RevocationDate) delete RevocationDate;
  if (Extensions) delete Extensions;  
  if (next) delete next;
}

unsigned t_RevokedCertif::der_length(bool outer) const
{ unsigned total_length = CertificateSerialNumber->der_length(true) + RevocationDate->der_length(true);
  if (Extensions) total_length += der_length_sequence(Extensions); // OPTIONAL 
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}    

void t_RevokedCertif::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  CertificateSerialNumber->save(str);
  RevocationDate->save(str);
  if (Extensions) save_sequence_of(str, Extensions); // OPTIONAL 
}

void t_RevokedCertif::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  unsigned length = str->load_length();  
 // load fields:
  CertificateSerialNumber = new t_asn_integer_univ;  if (!CertificateSerialNumber) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  CertificateSerialNumber->load(str);                if (str->any_error()) return;

  RevocationDate = new t_asn_UTCTime;  if (!RevocationDate) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  RevocationDate->load(str);           if (str->any_error()) return;

  if (length > der_length(false)) // OPTIONAL 
    load_sequence_of(str, &Extensions);
}

t_RevokedCertif::t_RevokedCertif(unsigned num, SYSTEMTIME * systime, t_RevokedCertif * next_rev)
{ CertificateSerialNumber = new t_asn_integer_univ(num);  
  RevocationDate = new t_asn_UTCTime(systime);  
  Extensions = NULL;  
  next = next_rev; 
}

void t_RevokedCertif::get_readable_info(char * buf) const
{ char hbuf[50];
  CertificateSerialNumber->print_hex(hbuf);
  sprintf(buf, "Cert. # %s invalidated ", hbuf);
  RevocationDate->get_readable_time(buf+strlen(buf));
}
////////////////////////////////////////////////////////////////////
t_TBSCertList::t_TBSCertList(void)
{ Version=NULL;   Signature=NULL;  Issuer=NULL;  
  ThisUpdate=NextUpdate=NULL;
  RevokedCertificates=NULL;
  clrExtensions=NULL;
}

t_TBSCertList::~t_TBSCertList(void)
{ if (Version) delete Version;  if (Signature) delete Signature;  if (Issuer)  delete Issuer;   
  if (ThisUpdate) delete ThisUpdate;  if (NextUpdate) delete NextUpdate;
  if (RevokedCertificates) delete RevokedCertificates;
  if (clrExtensions) delete clrExtensions;
}

unsigned t_TBSCertList::der_length(bool outer) const
{ unsigned total_length = Signature->der_length(true) + der_length_sequence(Issuer) + ThisUpdate->der_length(true);
  if (Version) total_length += Version->der_length(true);    // OPTIONAL 
  if (NextUpdate) total_length += NextUpdate->der_length(true);  // OPTIONAL
  if (RevokedCertificates) total_length += der_length_sequence(RevokedCertificates);
  if (clrExtensions)  // OPTIONAL EXPLICIT [0] (explicit is always constructed)
    total_length += tag_length(tag_kontext_constructed_0) + length_of_length(der_length_sequence(clrExtensions)) + // EXPLICIT tag
                    der_length_sequence(clrExtensions);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_TBSCertList::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  if (Version) Version->save(str);    // OPTIONAL, version never saved
  Signature->save(str);
  save_sequence_of(str, Issuer);
  ThisUpdate->save(str);
  if (NextUpdate) NextUpdate->save(str);  // OPTIONAL
  if (RevokedCertificates) save_sequence_of(str, RevokedCertificates); // OPTIONAL seq
  if (clrExtensions)  // OPTIONAL EXPLICIT [0] (explicit is always constructed)
  { str->save_tag(tag_kontext_constructed_0);  str->save_length(der_length_sequence(clrExtensions));  // EXPLICIT tag
    save_sequence_of(str, clrExtensions);
  }
}

void t_TBSCertList::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  unsigned length = str->load_length();  
 // load fields:
  if (str->check_tag(tag_universal_integer, false)) // OPTIONAL 
  { Version = new t_asn_integer_direct;  if (!Version) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    Version->load(str);                  if (str->any_error()) return;
  }

  Signature = new t_asn_algorithm_identifier;  if (!Signature) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  Signature->load(str);                        if (str->any_error()) return;

  load_sequence_of(str, &Issuer); 
  if (str->any_error()) return;

  ThisUpdate = new t_asn_UTCTime;  if (!ThisUpdate) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  ThisUpdate->load(str);           if (str->any_error()) return;

  if (str->check_tag(tag_universal_utctime, false) || str->check_tag(tag_universal_utctime2, false))
  { NextUpdate = new t_asn_UTCTime;  if (!NextUpdate) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    NextUpdate->load(str);           if (str->any_error()) return;
  }

  if (length > der_length(false) && str->check_tag(tag_universal_constructed_sequence, false))  // sequence may be empty
    load_sequence_of(str, &RevokedCertificates);

  if (length > der_length(false) && str->check_tag(tag_kontext_constructed_0)) // OPTIONAL EXPLICIT [0] (explicit is always constructed)
  { str->load_length();  // ignored
    load_sequence_of(str, &clrExtensions);
  }
}

bool t_TBSCertList::fill(bool root_ca)
{// Version must not be specified
 // Issuer:
  t_name_part TempUserNameList[2];
  TempUserNameList[0].object_id = attr_type_common_unit_name;
  TempUserNameList[0].object_id_length = attr_type_length;
  TempUserNameList[0].value = "Undefined";  // current_user_name;  -- this is bad
  TempUserNameList[1].object_id=NULL;
  TempUserNameList[1].object_id_length=0;
  TempUserNameList[1].value=NULL;
  Issuer = create_name_list(/*root_ca ? RCANameList : LCANameList*/TempUserNameList);
  if (!Issuer) return false;
 // ThisUpdate:
  SYSTEMTIME SysTime;  GetLocalTime(&SysTime);
  ThisUpdate = new t_asn_UTCTime(&SysTime);  if (!ThisUpdate) return false;
 // NextUpdate:
  systemtime_add(SysTime, 0, 0, 40, 0, 0, 0);
  NextUpdate = new t_asn_UTCTime(&SysTime);  if (!ThisUpdate) return false;
 // add list of certificates:
  if (root_ca) return true;
  unsigned next_cert_num = GetPrivateProfileInt("Counter", "NextCertificateNumber", 1, LCAinipathname);
  for (unsigned num=0;  num<next_cert_num;  num++)
  { char section[14];  itoa(num, section, 10);  char buf[40];
    GetPrivateProfileString(section, "Invalidated", "", buf, sizeof(buf), LCAinipathname);
    if (*buf) 
      if (sscanf(buf, "%hu.%hu.%hu %hu:%hu:%hu", &SysTime.wDay, &SysTime.wMonth, &SysTime.wYear, &SysTime.wHour, &SysTime.wMinute, &SysTime.wSecond)==6)
      { t_RevokedCertif * revcert = new t_RevokedCertif(num, &SysTime, RevokedCertificates);
        if (!revcert) return false;
        RevokedCertificates = revcert;
      }
  }
  return true;
}
/////////////////////////////////////// t_CertificateRevocationList ///////////////////////////////////////////////////////////
t_CertificateRevocation::t_CertificateRevocation(void)
{ TbsCertList = NULL;  SignatureAlgorithm=NULL;  SignatureValue=NULL;  next=NULL; }

t_CertificateRevocation::~t_CertificateRevocation(void)
{ if (TbsCertList)        delete TbsCertList;
  if (SignatureAlgorithm) delete SignatureAlgorithm;
  if (SignatureValue)     delete SignatureValue;
  if (next)               delete next;
}

unsigned t_CertificateRevocation::der_length(bool outer) const
{ unsigned total_length = TbsCertList->der_length(true) + SignatureAlgorithm->der_length(true) + SignatureValue->der_length(true);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_CertificateRevocation::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  TbsCertList->save(str);
  SignatureAlgorithm->save(str);
  SignatureValue->save(str);
}

void t_CertificateRevocation::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  TbsCertList = new t_TBSCertList;
  if (!TbsCertList) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  TbsCertList->load(str);
  if (str->any_error()) return;

  SignatureAlgorithm = new t_asn_algorithm_identifier;
  if (!SignatureAlgorithm) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  SignatureAlgorithm->load(str);
  if (str->any_error()) return;

  SignatureValue = new t_asn_bit_string;
  if (!SignatureValue) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  SignatureValue->load(str);
}

bool t_CertificateRevocation::fill(bool root_ca, HWND hParent)
{ TbsCertList = new t_TBSCertList;
  if (!TbsCertList) return false; 
  if (!TbsCertList->fill(root_ca)) return false;
 // signature algorithm:
  t_hash hs(NULL);  
  TbsCertList->Signature = new t_asn_algorithm_identifier(algorithm_id_rsaSigWithMD160, algorithm_id_rsaSigWithMD160_length);
  if (!TbsCertList->Signature) return false;
 // signature algorithm:
  SignatureAlgorithm = new t_asn_algorithm_identifier(algorithm_id_rsaSigWithMD160, algorithm_id_rsaSigWithMD160_length);
  if (!SignatureAlgorithm) return false;
 // signature key:
  t_Certificate * cert_certif;  CryptoPP::InvertibleRSAFunction * cert_private_key;
  cert_certif=NULL;  cert_private_key=NULL;
//.  activate_private_key(KEYTYPE_CERTIFY, hParent, &cert_private_key, &cert_certif);
  if (!cert_certif || !cert_private_key) 
    { show_error("Nelze podepsat seznam protoe nebyl aktivov� certifika��kl�", hParent);  return false; }
 // calculate the digest of the TbsCertList:
  { unsigned char full_digest[MAX_DIGEST_LENGTH];  unsigned full_digest_length;
    tMemStream mstr;  
    mstr.open_output(TbsCertList->der_length(true));
    TbsCertList->save(&mstr);
    mstr.get_digest(*hs.get_hash_module(), full_digest, &full_digest_length);
   // sign it:
    CryptoPP::Integer encdig = generic_sign(&hs, full_digest, full_digest_length, cert_private_key);
    unsigned xDigestSignatureLength = cert_private_key->MaxPreimage().ByteCount();
    CryptoPP::SecByteBlock signat(xDigestSignatureLength);
	  encdig.Encode(signat, xDigestSignatureLength);
    SignatureValue = new t_asn_bit_string(signat, 8*xDigestSignatureLength); 
    if (cert_certif) delete cert_certif;  delete cert_private_key; 
  }
  return SignatureValue!=NULL;
}

t_VerifyResult t_CertificateRevocation::verify_signature(void) const
{// calculate digest of authenticated data:
  tMemStream mstr;  
  mstr.open_output(TbsCertList->der_length(true));
  TbsCertList->save(&mstr);
  unsigned auth_data_len = mstr.external_pos();
  unsigned char * auth_data = mstr.close_output();
 // find the certificate:
  const t_Certificate * signer_cert = the_dbcert->get_by_subject(TbsCertList->Issuer, KEYTYPE_CERTIFY); 
  if (!signer_cert) 
    signer_cert = the_root_cert->get_by_subject(TbsCertList->Issuer, KEYTYPE_RCA); 
  if (!signer_cert) return VR_MISSING_CERTIF;
  //t_keytypes keytype = certificate->TbsCertificate->get_key_usage();
  //if (keytype!=KEYTYPE_SIGN && keytype!=KEYTYPE_CERTIFY)  // message distributing the keys may be signed by the CA key!
  //  return VR_BAD_CERTIF_TYPE;
 // verify the certificate:
  t_VerifyResult result=signer_cert->VerifyCertificate(NULL, NULL); // no signing time available, verifying to current time
  if (result!=VR_OK) return result;
 // verify the signature using the certificate:
  const unsigned char * signat = SignatureValue->get_val();  unsigned signat_length = SignatureValue->get_bitlength()/8;
  if (!signer_cert->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey->verify_signature(signat, signat_length, auth_data, auth_data_len))
    return VR_TAMPERED_AA;
  return VR_OK;
}

///////////////////////////// creating a certificate //////////////////////////////////////////////
CryptoPP::InvertibleRSAFunction * retained_private_key = NULL;
unsigned char retained_cert_digest[MAX_DIGEST_LENGTH];  unsigned retained_cert_digest_length;

void calc_certificate_digest(unsigned char * cert_digest, unsigned * cert_digest_length, const t_Certificate * cert)
{ t_hash hs(NULL);  tMemStream mstr;  
  mstr.open_output(cert->der_length(true));
  cert->save(&mstr);
  mstr.get_digest(*hs.get_hash_module(), cert_digest, cert_digest_length);
}

DllSecur void WINAPI remove_retained_private_key(void)
{ if (retained_private_key) 
    { delete retained_private_key;  retained_private_key=NULL; }
}

#ifdef WITH_GUI
DllSecur CryptoPP::InvertibleRSAFunction * WINAPI get_private_key_for_certificate(HWND hParent, const char * username, const t_Certificate * cert)
{ char privkeypath[MAX_PATH];
  if (retained_private_key)
  { unsigned char cert_digest[MAX_DIGEST_LENGTH];  unsigned cert_digest_length;
    calc_certificate_digest(cert_digest, &cert_digest_length, cert);
    if (!memcmp(cert_digest, retained_cert_digest, cert_digest_length))
    { CryptoPP::InvertibleRSAFunction * invrsa = new CryptoPP::InvertibleRSAFunction;
      invrsa->Initialize(retained_private_key->GetModulus(), retained_private_key->GetPublicExponent(),
      retained_private_key->GetPrivateExponent(), retained_private_key->GetPrime1(), retained_private_key->GetPrime2(),
      retained_private_key->GetModPrime1PrivateExponent(), retained_private_key->GetModPrime2PrivateExponent(), retained_private_key->GetMultiplicativeInverseOfPrime2ModPrime1());
      return invrsa;
    }
  }
  strcpy(privkeypath, "a:");
  if (privkeypath[strlen(privkeypath)-1] != PATH_SEPARATOR) strcat(privkeypath, PATH_SEPARATOR_STR);
  strcat(privkeypath, username);
  strcat(privkeypath, ".prk");
  char password[MAX_PASSWORD_LEN+1];  bool retain = false;
  if (!OpenPrivateKeyFile(hParent, privkeypath, "Vyberte soubor se soukromm kl�em", password, &retain))  // load the private key:
    return NULL;
  CryptoPP::InvertibleRSAFunction * private_key = load_private_rsa(privkeypath, hParent, password);
  if (!private_key) return NULL;
 // check if the keys form a pair
  const t_asn_public_key * pk = cert->TbsCertificate->SubjectPublicKeyInfo->subjectPublicKey;
  if (CryptoPP::Integer(pk->modulus->value, pk->modulus->length) != private_key->GetModulus() ||
      CryptoPP::Integer(pk->publicExponent->value, pk->publicExponent->length) != private_key->GetPublicExponent())
  { show_error("Soukrom a veejn kl� netvo�p�", hParent);
    delete private_key;
    return NULL;
  }
 // retain a copy of the key in memory:
  if (retain)
  { if (retained_private_key) { delete retained_private_key;  retained_private_key=NULL; }
    retained_private_key = new CryptoPP::InvertibleRSAFunction;
    retained_private_key->Initialize(private_key->GetModulus(), private_key->GetPublicExponent(),
      private_key->GetPrivateExponent(), private_key->GetPrime1(), private_key->GetPrime2(),
      private_key->GetModPrime1PrivateExponent(), private_key->GetModPrime2PrivateExponent(), private_key->GetMultiplicativeInverseOfPrime2ModPrime1());
   // calculate the digest of the certificate:
    calc_certificate_digest(retained_cert_digest, &retained_cert_digest_length, cert);
  }
  return private_key;
}
#else  // !WITH_GUI
DllSecur CryptoPP::InvertibleRSAFunction * WINAPI get_private_key_for_certificate(HWND hParent, const char * username, const t_Certificate * cert)
{ return NULL; }
#endif // !WITH_GUI

t_Certificate * make_certificate_from_request(t_cert_req * request, t_keytypes keytype,
                   const char * start_date_string, const char * end_date_string, const char * certifier_name, HWND hParent,
                   const t_Certificate * signer_cert, const unsigned char * signer_cert_uuid,
                   const CryptoPP::InvertibleRSAFunction * signer_private_key, int certificate_number)
// request is supposed to be validated
{// creating genereal part of the certificate:
  t_Certificate * cert = new t_Certificate;
  if (!cert) return NULL;
  t_hash hs(NULL);  
  cert->SignatureAlgorithm = new t_asn_algorithm_identifier(hs.get_hash_algorithmRSA_id(), hs.get_hash_algorithmRSA_id_length());
  cert->TbsCertificate = new t_TBSCertificate;
  if (!cert->TbsCertificate || !cert->SignatureAlgorithm) goto errex;
  cert->TbsCertificate->Version = new t_asn_integer_direct(2);  // version fixed as 2
  cert->TbsCertificate->CertificateSerialNumber = new t_asn_integer_univ(certificate_number);
  cert->TbsCertificate->Signature = new t_asn_algorithm_identifier(hs.get_hash_algorithmRSA_id(), hs.get_hash_algorithmRSA_id_length());
  cert->TbsCertificate->Issuer = signer_cert ? signer_cert->TbsCertificate->Subject->copy_seq() : request->info->subject->copy_seq();
  cert->TbsCertificate->Validity = new t_asn_Validity;
  if (!cert->TbsCertificate->Validity) goto errex;
  if (!cert->TbsCertificate->Validity->set_dates(start_date_string, end_date_string)) goto errex;
  if (!cert->TbsCertificate->Version || !cert->TbsCertificate->CertificateSerialNumber || !cert->TbsCertificate->Signature ||
      !cert->TbsCertificate->Issuer) goto errex;
  t_asn_Extensions ** ppext;  ppext = &cert->TbsCertificate->Extensions;
 // define basic constrains (sequence stored in octet string):
  if      (request->info->attributes->equal(cri_attribute_CA)) // should not create CA certificates!
  { *ppext = new t_asn_Extensions(exten_id_BasConstr, exten_id_length, (unsigned char*)"\x30\x6\x1\x1\xff\x2\x1\x7", 8);
    if (!*ppext) goto errex;
    ppext = &(*ppext)->next;
  }
  else if (request->info->attributes->equal(cri_attribute_RCA)) // should not create RCA certificates!
  { *ppext = new t_asn_Extensions(exten_id_BasConstr, exten_id_length, (unsigned char*)"\x30\x6\x1\x1\xff\x2\x1\x8", 8);
    if (!*ppext) goto errex;
    ppext = &(*ppext)->next;
  }
  // else ;  // because "Basic constraint" should not appear in end entity certificates
 // define key usage (bit string stored in octet string):
  if      (keytype==KEYTYPE_SIGN)
  { *ppext = new t_asn_Extensions(exten_id_KeyUsage,  exten_id_length, keyUsageSign,    keyUsageOctetLength);
  }
  else if (keytype==KEYTYPE_ENCRYPT)
  { *ppext = new t_asn_Extensions(exten_id_KeyUsage,  exten_id_length, keyUsageEncrypt, keyUsageOctetLength);
  }
  else if (keytype==KEYTYPE_ENCSIG)
  { *ppext = new t_asn_Extensions(exten_id_KeyUsage,  exten_id_length, keyUsageSignEnc, keyUsageOctetLength);
  }
  else if (keytype==KEYTYPE_CERTIFY) // should not create CA certificates!
  { *ppext = new t_asn_Extensions(exten_id_KeyUsage,  exten_id_length, keyUsageCA,      keyUsageOctetLength);
  }
  else if (keytype==KEYTYPE_RCA) // should not create RCA certificates!
  { *ppext = new t_asn_Extensions(exten_id_KeyUsage,  exten_id_length, keyUsageCA,      keyUsageOctetLength);
  }
  else goto errex; 
  if (!*ppext) goto errex;

 // moving "Subject" data from the request to the certificate:
  cert->TbsCertificate->Subject = request->info->subject;  request->info->subject = NULL;
  cert->TbsCertificate->SubjectPublicKeyInfo = request->info->subjectPKInfo;  request->info->subjectPKInfo = NULL;

 // prepare the key for certification:
  CryptoPP::InvertibleRSAFunction * cert_private_key;  cert_private_key=NULL;  // temporary private key
  if (!signer_private_key)
  { cert_private_key=get_private_key_for_certificate(hParent, certifier_name, signer_cert);
    if (!cert_private_key) goto errex;
    signer_private_key = cert_private_key;  
  }

 // define identification of the key used to sign the certificate:
  cert->TbsCertificate->AuthKeyId = new t_asn_AuthorityKeyIdentifier;  if (!cert->TbsCertificate->AuthKeyId) goto errex;
  cert->TbsCertificate->AuthKeyId->CertIssuer = cert->TbsCertificate->Issuer->copy_seq();
  cert->TbsCertificate->AuthKeyId->CertSerialNumber = signer_cert ? 
    signer_cert->TbsCertificate->CertificateSerialNumber->copy() : cert->TbsCertificate->CertificateSerialNumber->copy();
  cert->TbsCertificate->AuthKeyId->KeyIdentifier = new t_asn_octet_string(signer_cert_uuid, 12);
  { tMemStream mstr;
    mstr.open_output(cert->TbsCertificate->AuthKeyId->der_length(true));
    cert->TbsCertificate->AuthKeyId->save(&mstr);
    unsigned char * bytes = mstr.close_output();
    t_asn_Extensions * AuthKeyIdExt;
    AuthKeyIdExt = new t_asn_Extensions(exten_id_AutKeyId,  exten_id_length, bytes, mstr.extent());
    delete [] bytes;
    if (!AuthKeyIdExt) goto errex;
    AuthKeyIdExt->next = cert->TbsCertificate->Extensions;  cert->TbsCertificate->Extensions = AuthKeyIdExt;
  }
// signing the certificate:
 // calculate the digest of the TbsCertificate:
  { unsigned char full_digest[MAX_DIGEST_LENGTH];  unsigned full_digest_length;
    tMemStream mstr;  
    mstr.open_output(cert->TbsCertificate->der_length(true));
    cert->TbsCertificate->save(&mstr);
    mstr.get_digest(*hs.get_hash_module(), full_digest, &full_digest_length);
   // sign it:
    CryptoPP::Integer encdig = generic_sign(&hs, full_digest, full_digest_length, signer_private_key);
    unsigned xDigestSignatureLength = signer_private_key->MaxPreimage().ByteCount();
    CryptoPP::SecByteBlock signat(xDigestSignatureLength);
	  encdig.Encode(signat, xDigestSignatureLength);
    cert->SignatureValue = new t_asn_bit_string(signat, 8*xDigestSignatureLength); 
    if (cert_private_key) delete cert_private_key;
  }
  return cert;
 errex:
  delete cert;
  return NULL;
}

t_Certificate * load_certificate(tStream * str)
// Loads and returns the certificate, checks the structure but not the contents.
{ t_Certificate * cert = new t_Certificate;
  if (cert)
  { cert->load(str);
    if (!str->any_error())
      return cert;
    delete cert;
  }
  return NULL;
}


#ifdef STOP
/////////////////////////////////////////////// certificate management /////////////////////////////////

#define BITMAP_WIDTH  16
#define BITMAP_HEIGHT 16

HWND CreateLCAList(HWND hWndParent)
{// Create the list view window that starts out in details view
  LONG style = WS_CHILD | WS_TABSTOP | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS;
  HWND hWndList = CreateWindowEx(0, WC_LISTVIEW, "Seznam vydanch certifik�", style,
    4, 4, 100, 100, hWndParent, (HMENU)0, hProgInst, NULL);
  if (hWndList == NULL) return NULL;
  ListView_SetExtendedListViewStyle(hWndList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
 // Initialize the list view window.
#ifdef STOP
 // First initialize the image lists you will need create image lists for the small and the large icons.
  HIMAGELIST hSmall = ImageList_Create(BITMAP_WIDTH, BITMAP_HEIGHT, FALSE, 5, 0);
 // Load the icons and add them to the image lists.
  // You have 3 of each type of icon here, so add 3 at a time.
  for (int iSubItem = 0; iSubItem < 5; iSubItem++)
  { HICON hIcon = LoadIcon(hProgInst, MAKEINTRESOURCE (IDI_CERT+iSubItem));
    ImageList_AddIcon(hSmall, hIcon);
  }
 // Associate the image list with the list view control.
  ListView_SetImageList(hWndList, hSmall, LVSIL_SMALL);
#endif
 // Now initialize the columns you will need. Initialize the LV_COLUMN structure. 
 // The mask specifies that the fmt, width, pszText, and subitem members of the structure are valid.
  LV_COLUMN lvC; // list view column structure
  lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
  lvC.fmt = LVCFMT_LEFT; // left-align column
  lvC.cx = 350; // width of column in pixels
  lvC.pszText = "Subjekt (majitel)";
  lvC.iSubItem = 0;
  ListView_InsertColumn(hWndList, 0, &lvC);
  lvC.cx = 65; // width of column in pixels
  lvC.pszText = "�el kl�e";
  lvC.iSubItem = 1;
  ListView_InsertColumn(hWndList, 1, &lvC);
  lvC.cx = 70; // width of column in pixels
  lvC.pszText = "�slo";
  lvC.iSubItem = 2;
  ListView_InsertColumn(hWndList, 2, &lvC);
  lvC.cx = 130; // width of column in pixels
  lvC.pszText = "Platnost od";
  lvC.iSubItem = 3;
  ListView_InsertColumn(hWndList, 3, &lvC);
  lvC.cx = 130; // width of column in pixels
  lvC.pszText = "Platnost do";
  lvC.iSubItem = 4;
  ListView_InsertColumn(hWndList, 4, &lvC);
  lvC.cx = 130; // width of column in pixels
  lvC.pszText = "Zneplatn�o dne";
  lvC.iSubItem = 5;
  ListView_InsertColumn(hWndList, 5, &lvC);
  return hWndList;
}

void fill_LCA_list(HWND hWndList)
// If owner specified, selects the newest certificate
{ LV_ITEM lvI; // list view item structure
  lvI.mask = LVIF_TEXT | /*LVIF_IMAGE | */LVIF_PARAM | LVIF_STATE;
  lvI.state = 0; 
  lvI.stateMask = 0; 
  ListView_DeleteAllItems(hWndList);
  unsigned next_cert_num = GetPrivateProfileInt("Counter", "NextCertificateNumber", 1, LCAinipathname);
  int index=0;
  for (unsigned num=0;  num<next_cert_num;  num++)
  { char section[14];  itoa(num, section, 10);
    char buf[200];
    GetPrivateProfileString(section, "Subject", "", buf, sizeof(buf), LCAinipathname);
    if (*buf) 
    { lvI.iItem = index;
      lvI.iSubItem = 0;  // refers to the whole item
      lvI.pszText = buf; 
      lvI.cchTextMax = 0;
      lvI.lParam = (LPARAM)num;
      ListView_InsertItem (hWndList, &lvI);
      GetPrivateProfileString(section, "KeyUsage", "", buf, sizeof(buf), LCAinipathname);
      ListView_SetItemText(hWndList, index, 1, buf);
      ListView_SetItemText(hWndList, index, 2, section);
      GetPrivateProfileString(section, "Start", "", buf, sizeof(buf), LCAinipathname);
      ListView_SetItemText(hWndList, index, 3, buf);
      GetPrivateProfileString(section, "End", "", buf, sizeof(buf), LCAinipathname);
      ListView_SetItemText(hWndList, index, 4, buf);
      GetPrivateProfileString(section, "Invalidated", "", buf, sizeof(buf), LCAinipathname);
      ListView_SetItemText(hWndList, index, 5, buf);
      index++;
    }
  } 
} 

static void reposition_list(HWND hDlg)
{ HWND hList = GetDlgItem(hDlg, 12345);
  if (!hList) return;  // not created yet
  RECT Rect;  GetClientRect(hDlg, &Rect);
  SetWindowPos(hList, 0, 4, 36, Rect.right-8, Rect.bottom-40, SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL CALLBACK LCADbDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{ switch (message)
  { case WM_INITDIALOG:
    { HWND hList = CreateLCAList(hDlg);
      fill_LCA_list(hList);
      SetWindowLong(hList, GWL_ID, 12345);
      reposition_list(hDlg);
      break;
    }
    case WM_SIZE:
      reposition_list(hDlg);  break;
    case WM_COMMAND:       /* message: command from application menu */
      switch (GET_WM_COMMAND_ID(wParam,lParam))
      { case CD_CERT_INVALIDATE:
        { HWND hList = GetDlgItem(hDlg, 12345);
          if (ListView_GetSelectedCount(hList) != 1) { MessageBeep(-1);  break; }
          int sel = ListView_GetNextItem(hList, -1, /*LVNI_BELOW | */LVNI_SELECTED);
          if (sel==-1) { MessageBeep(-1);  break; }
          LVITEM lvi;  lvi.mask=LVIF_PARAM;  lvi.iItem=sel;  lvi.iSubItem=0;
          ListView_GetItem(hList, &lvi);
          char section[14];  itoa(lvi.lParam, section, 10);
          char buf[40];   
          GetPrivateProfileString(section, "Invalidated", "", buf, sizeof(buf), LCAinipathname);
          if (*buf) { show_error("Tento certifik� byl zneplatn� ji d�e!");  break; }
          SYSTEMTIME CurrentTime;  GetLocalTime(&CurrentTime);
          sprintf(buf, "%u.%u.%u %02u:%02u:%02u", CurrentTime.wDay, CurrentTime.wMonth, CurrentTime.wYear, CurrentTime.wHour, CurrentTime.wMinute, CurrentTime.wSecond);
          WritePrivateProfileString(section, "Invalidated", buf, LCAinipathname);
          fill_LCA_list(hList);
          MessageBox(hDlg, "Zneplatn��certifik�u bylo zaznamen�o. Nezapome�e rozeslat nov seznam CLR!", "Zpr�a", MB_OK|MB_APPLMODAL|MB_ICONINFORMATION);
          break;
        }
        case IDOK:  
          EndDialog(hDlg, 0);  break;
      }
      break;
    default:
      return false;
  }
  return true;
}

void create_CRL(HWND hParent)
{ t_CertificateRevocation * cert_rev = new t_CertificateRevocation;
  if (cert_rev)
  { if (cert_rev->fill(false, hParent))
    { SYSTEMTIME CurrentTime;  GetLocalTime(&CurrentTime);
      char pathname[MAX_PATH];
      sprintf(pathname, "%s-%02u%02u%02u.%s", local_lca_name, CurrentTime.wYear % 100, CurrentTime.wMonth, CurrentTime.wDay, "crl");
      if (SaveFile(hParent, pathname))
      { tFileStream ostr;
        if (ostr.open_output(pathname)) 
        { cert_rev->save(&ostr);
          ostr.close_output();
          MessageBox(hParent, "Seznam zneplatn�ch certifik� byl vytvoen", pathname, MB_OK|MB_APPLMODAL|MB_ICONINFORMATION);
        }
      }
    }
    delete cert_rev;
  }
}

#endif

