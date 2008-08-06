// keygen.cpp - RSA key pair generation GUI
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
#define ENGLISH

#ifdef WITH_GUI
#include "securrc.h"
#include <windowsx.h>
#include <olectl.h>
//#include <ansiapi.h>
#include <commctrl.h>
#endif

#ifdef ENGLISH
#define MSG0   "Error"
#define MSG1   "Can not open the file with the private key"
#define MSG2   "Wrong password specified!"
#define MSG3   "Can not read the priate key from the file"
#define MSG4   "The password is too short!"
#define MSG5   "The passwords do not match!"
#define MSG6   "Wrong old password specified!"
#define MSG7   "The validity period must not end before its beginning!"
#define MSG8   "The maximum validity is 1 year and 5 days (2+7 for LCA, 3+10 for RCA)!"
#define MSG9   "Validity must not start in the past!"
#define MSG10  "The file does not contain a certification request."
#define MSG11  "Internal error in the certification request"
#define MSG12  "Error"
#define MSG13  "Can not write private key to existing file."
#define MSG14  "The private key cannot be written into the file."
#define MSG15  "Passowrd is not specified!"
#define MSG16  "Please enter more characters."
#define MSG17  "The text doen not appear to be random"
#define MSG18  "Key generation canceled."
#define MSG19  "Message"
#define MSG20  "Error creating the certificate"
#define MSG21  "Created keys and certificate for root CA."
#define MSG22  "Error when generating the keys"
#define MSG23  "Key generation and the request for certification is complete."
#define MSG24  "Cannot overwrite the private key file."
#define MSG25  "Private key password has been changed."
#define MSG26  "Certification was canceled"
#define MSG27  "Key generation process is running, please wait..."
#define MSG28  "Miniature of the file: "
#define MSG29  "Digesting algorithm: "
#define MSG30  "Not specified - can not certificate!"
#define MSG31  "For signing"
#define MSG32  "For encryption"
#define MSG33  "For non-root CA"
#define MSG34  "For root CA - certification denied!"
#define MSG35  "Other - certification denied!"
#define MSG36  "Yes"
#define MSG37  "No, can not certificate"
#define MSG38  "Missing, the identity of the requestor has to be verified!"
#define MSG39  "Signature is valid"
#define MSG40  "Invalid signature, can not certify."
#else
#define MSG0   "Chyba"
#define MSG1   "Nelze otevøít soubor se soukromým klíèem"
#define MSG2   "Zadáno chybné heslo!"
#define MSG3   "Nelze naèíst se soukromý klíè ze souboru"
#define MSG4   "Heslo je pøíliš krátké!"
#define MSG5   "Heslo a jeho kopie se liší!"
#define MSG6   "Zadáno chybné staré heslo!"
#define MSG7   "Konec platnosti musí být pozdìji než zaèátek!"
#define MSG8   "Doba platnosti smí být nejvýše 1 rok a 5 dní (2+7 pro LCA, 3+10 pro RCA)!"
#define MSG9   "Certifikát nesmí mít zpìtnou platnost!"
#define MSG10  "V souboru není žádost o certifikát"
#define MSG11  "Žádost o certifikát má chybnou strukturu"
#define MSG12  "Nelze"
#define MSG13  "Vytváøený soukromý klíè nelze zapisovat do již existujícícho souboru."
#define MSG14  "Vytvoøený soukromý klíè nelze zapsat na zadané místo."
#define MSG15  "Není uvedeno heslo ke klíèi!"
#define MSG16  "Nutno napsat delší text"
#define MSG17  "Text nevypadá dostateènì náhodnì"
#define MSG18  "Upuštìno od generování klíèù"
#define MSG19  "Zpráva"
#define MSG20  "Chyba pøi generování certifikátu"
#define MSG21  "Klíèe a certifikát vrcholové CA byly vytvoøeny."
#define MSG22  "Chyba pøi generování klíèù"
#define MSG23  "Klíèe a žádost o certifikaci byly úspìšnì vytvoøeny."
#define MSG24  "Soubor se soukromým klíèem nelze pøepsat."
#define MSG25  "Heslo k souboru se soukromým klíèem bylo zmìnìno."
#define MSG26  "Certifikace nebyla provedena"
#define MSG27  "Probíhá generování klíèù, èekejte prosím..."
#define MSG28  "Miniatura souboru: "
#define MSG29  "Typ hašovacího algoritmu: "
#define MSG30  "Neuveden - není dovoleno certifikovat!"
#define MSG31  "Podpisový"
#define MSG32  "Šifrovací"
#define MSG33  "Certifikaèní klíè LCA"
#define MSG34  "Certifikaèní klíè RCA - není dovoleno certifikovat!"
#define MSG35  "Jiný - není dovoleno certifikovat!"
#define MSG36  "Ano"
#define MSG37  "Ne, proto nelze certifikovat"
#define MSG38  "Chybí, nutno ovìøit totožnost žadatele!"
#define MSG39  "Podpis je v poøádku"
#define MSG40  "Neplatný podpis, proto nelze certifikovat"
#endif

#define KEY_SEED_MAX 200
#define KEY_SEED_MIN 80

#define MIN_PASSWORD_LEN  10

void show_error(const char * msg, HWND hParent) 
{ 
#ifdef WITH_GUI
  MessageBox(hParent, msg, MSG0, MB_OK|MB_APPLMODAL); 
#endif
}

struct t_keygen_info 
{ t_user_ident user_ident;
  t_name_part SubjectNameList[5];
  const bool for_root;
  t_keytypes keytype;
  char seed[KEY_SEED_MAX+1];
  char privkey[MAX_PATH];
  t_Certificate * cert_for_signing_request;  // owning
  bool signedreq;
  char password[MAX_PASSWORD_LEN+1];
  t_cert_req * cert_req;  // owning ptr
  CryptoPP::InvertibleRSAFunction * priv;  // owning ptr
  t_keygen_info(const t_user_ident * user_identIn, t_Certificate * cert_for_signing_requestIn, bool for_rootIn) : for_root(for_rootIn)
  { cert_req=NULL;  priv=NULL;  
    keytype = for_root ? KEYTYPE_RCA : KEYTYPE_SIGN;
    cert_for_signing_request = cert_for_signing_requestIn;
    signedreq = cert_for_signing_request!=NULL; 
    user_ident=*user_identIn;
    *privkey=0;
  }
  void make_key_pair(HWND hParent);
  bool save_cert_request(HWND hParent, unsigned char ** cert_req_str, unsigned * cert_req_len);
  bool save_private_key(HWND hParent);
  void make_name(void);
  ~t_keygen_info(void)
  { delete cert_req;  delete priv;  
    if (cert_for_signing_request) delete cert_for_signing_request; 
  }
};

#ifdef WITH_GUI
BOOL APIENTRY EmptyDlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
{ switch (msg)
  { case WM_INITDIALOG:
      return TRUE;
  }
  return FALSE;
}
#endif  // WITH_GUI

void t_keygen_info::make_name(void)
{ SubjectNameList[0].object_id=attr_type_given_name;        SubjectNameList[0].object_id_length=attr_type_length;
  SubjectNameList[0].value=user_ident.name1;
  SubjectNameList[1].object_id=attr_type_initials;          SubjectNameList[1].object_id_length=attr_type_length;
  SubjectNameList[1].value=user_ident.name2;
  SubjectNameList[2].object_id=attr_type_surname;           SubjectNameList[2].object_id_length=attr_type_length;
  SubjectNameList[2].value=user_ident.name3;
  SubjectNameList[3].object_id=attr_type_common_unit_name;  SubjectNameList[3].object_id_length=attr_type_length;
  SubjectNameList[3].value=user_ident.identif;
  SubjectNameList[4].object_id=NULL;                        SubjectNameList[4].object_id_length=0;
  SubjectNameList[4].value=NULL;
}

t_asn_integer_indirect * Integer2MyInteger(CryptoPP::Integer val)
{
  tMemStream str;  unsigned char buf[1+3+257];  
//  CryptoPP::StringStore ss(buf, sizeof(buf));
  CryptoPP::ArraySink ss(buf, sizeof(buf));
  val.DEREncode(ss);
  str.open_input(buf, (unsigned)ss.TotalPutLength());
  t_asn_integer_indirect * myint = new t_asn_integer_indirect;
  myint->load(&str);
  str.close_input();
  return myint;
}

void t_keygen_info::make_key_pair(HWND hParent)
{
#ifdef WITH_GUI
  HWND hInfo = CreateDialog(hProgInst, MAKEINTRESOURCE(DLG_MESSAGE), hParent, EmptyDlgProc);
  SetDlgItemText(hInfo, CD_TEXT, MSG27);
#endif  // WITH_GUI
  unsigned keybits = 1536;
#ifdef ORIG
  priv = new CryptoPP::InvertibleRSAFunction(randPool, keybits, 17);
#else
  priv = new CryptoPP::InvertibleRSAFunction;  priv->Initialize(temp_rand_pool((byte *)seed, (int)strlen(seed)), keybits, 17);
#endif
  CryptoPP::Integer x(123);
#ifdef ORIG
  CryptoPP::Integer y=priv->CalculateInverse(x);
#else
  CryptoPP::Integer y=priv->CalculateInverse(temp_rand_pool(), x);
#endif
  CryptoPP::Integer z=priv->ApplyFunction(y);
 // create the asn structures:
  t_asn_public_key * subjectPublicKey = new t_asn_public_key;
#ifdef ORIG
  tMemStream str;  unsigned char buf[1+3+257];
  unsigned datasize = priv->GetModulus().DEREncode(buf);
  str.open_input(buf, datasize);
  subjectPublicKey->modulus = new t_asn_integer_indirect;
  subjectPublicKey->modulus->load(&str);
  str.close_input();
  datasize = priv->GetExponent().DEREncode(buf);
  str.open_input(buf, datasize);
  subjectPublicKey->publicExponent = new t_asn_integer_indirect;
  subjectPublicKey->publicExponent->load(&str);
  str.close_input();
#else
  subjectPublicKey->modulus        = Integer2MyInteger(priv->GetModulus());
  subjectPublicKey->publicExponent = Integer2MyInteger(priv->GetPublicExponent());
#endif
  // create the name of the subject:
  make_name();
 // create and sign the certification request:
  cert_req = create_certification_request(keytype, SubjectNameList, subjectPublicKey);
  cert_req->sign(priv);
 // verify the signature:
  cert_req->verify_own_signature();
#ifdef WITH_GUI
  DestroyWindow(hInfo);
#endif  // WITH_GUI
}

#define PASS_ITERATIONS 10000

void make_key_from_password(const char * pass, t_IDEA_key * pikey)
// Original IDEAXCBC key
{ unsigned char digest[MAX_DIGEST_LENGTH];
#ifdef WITH_GUI
  SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif  // WITH_GUI
  CryptoPP::RIPEMD160 md;
  md.Update((const unsigned char*)pass, strlen(pass));
  md.Final(digest);
  for (int i=0;  i<PASS_ITERATIONS;  i++)
  { md.Update(digest, md.DigestSize());
    md.Final(digest);
  }
  for (int off=0;  off<CryptoPP::IDEA::KEYLENGTH;  off+=md.DigestSize())
    memcpy(((unsigned char*)pikey)+off, digest, CryptoPP::IDEA::KEYLENGTH-off < md.DigestSize() ? CryptoPP::IDEA::KEYLENGTH-off : md.DigestSize());
#ifdef WITH_GUI
  SetCursor(LoadCursor(NULL, IDC_ARROW));
#endif  // WITH_GUI
}

bool t_keygen_info::save_private_key(HWND hParent)
{ tFileStream str;  tMemStream mstr;
  FHANDLE hFile=CreateFile(privkey, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (hFile!=INVALID_FHANDLE_VALUE) 
  { CloseHandle(hFile);  
#ifdef WITH_GUI
    MessageBox(hParent, MSG13, MSG12, MB_OK|MB_APPLMODAL);
#endif
    return false; 
  }
  if (str.open_output(privkey)) 
  { t_asn_private_key * privKey = new t_asn_private_key(priv);
    if (privKey)
    { unsigned key_length = privKey->der_length(true);
      // unsigned enc_key_length = key_length / CryptoPP::IDEA::BLOCKSIZE * CryptoPP::IDEA::BLOCKSIZE + CryptoPP::IDEA::BLOCKSIZE; -- not used
      mstr.open_output(key_length);
      privKey->save(&mstr);
      t_IDEA_key ikey;
      make_key_from_password(password, &ikey);
      CryptoPP::IDEAEncryption ideaenc((const unsigned char*)&ikey); 
      mstr.copy(&str, ideaenc);
      delete [] mstr.close_output();
      delete privKey;
    }
    str.close_output();
    if (!str.any_error()) return true;
  }
#ifdef WITH_GUI
  MessageBox(hParent, MSG14, MSG12, MB_OK|MB_APPLMODAL);
#endif
  return false;
}

t_asn_private_key * load_private_key(const char * pathname, HWND hParent, const char * password)
{ tFileStream str;
  t_IDEA_key ikey;
  make_key_from_password(password, &ikey);
  CryptoPP::IDEADecryption ideadec((const unsigned char*)&ikey); 
  if (!str.open_input(pathname)) 
    { show_error(MSG1, hParent);  return NULL; }
  tMemStream mstr;
  unsigned enc_length = str.extent();
  mstr.open_output(enc_length);
  if (!stream_copy_decrypt(&str, &mstr, enc_length, NULL, ideadec))
  { //.log_action(ACT_BAD_KEY_PASSWORD, pathname);
    show_error(MSG2, hParent);  return NULL; 
  }
  str.close_input();
  mstr.reset();
  t_asn_private_key * privKey = new t_asn_private_key;
  if (!privKey) return NULL;
  privKey->load(&mstr);
  delete [] mstr.close_output();
  if (mstr.any_error())
    { show_error(MSG3, hParent);  delete privKey;  return NULL; }
  return privKey;
}

CryptoPP::InvertibleRSAFunction * load_private_rsa(const char * pathname, HWND hParent, const char * password)
{ t_asn_private_key * privKey = load_private_key(pathname, hParent, password);
  if (!privKey) return NULL;
  CryptoPP::InvertibleRSAFunction * priv = new CryptoPP::InvertibleRSAFunction;
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
/////////////////////////////////////////// hash encapsulation ////////////////////////////////////////////
static const char HASH_NAME_RIPEMD160[] = "RIPEMD-160";
static const char HASH_NAME_RIPEMD128[] = "RIPEMD-128";
static const char HASH_NAME_SHA1     [] = "SHA-1";

t_hash::t_hash(const char * hash_name)
{ if (!hash_name) 
    sel = 2;  // SHA-1 preferred, MSIE likes it
  else if (!stricmp(hash_name, HASH_NAME_RIPEMD160)) sel=1;
  else if (!stricmp(hash_name, HASH_NAME_RIPEMD128)) sel=0;
  else if (!stricmp(hash_name, HASH_NAME_SHA1     )) sel=2;
  else sel=-1;
}

const char * t_hash::hash_name(void)
{ return sel==0 ? HASH_NAME_RIPEMD128 :
         sel==1 ? HASH_NAME_RIPEMD160:
                  HASH_NAME_SHA1;
}

t_hash::t_hash(t_algors alg)
{ switch (alg)
  { case ALG_RIPEMD160:
      sel=1;  break;  
    case ALG_RIPEMD128:
      sel=1;  break;
    case ALG_SHA:
      sel=2;  break;
    default:
      sel=-1;  break;
  }
}

CryptoPP::HashModule * t_hash::get_hash_module(void) const
{ return sel==0 ? (CryptoPP::HashModule *)&ripemd128 : sel==1 ? (CryptoPP::HashModule *)&ripemd160 : (CryptoPP::HashModule *)&sha; }

const unsigned char * t_hash::get_hash_decoration(void)
{ return sel==0 ? CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD128>::decoration :
         sel==1 ? CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD160>::decoration :
                  CryptoPP::PKCS_DigestDecoration<CryptoPP::SHA>      ::decoration;
}

unsigned t_hash::get_hash_decoration_length(void)
{ return sel==0 ? CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD128>::length :
         sel==1 ? CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD160>::length :
                  CryptoPP::PKCS_DigestDecoration<CryptoPP::SHA>      ::length;
}

const unsigned * t_hash::get_hash_algorithm_id(void)
{ return sel==0 ? algorithm_id_ripemd128 :
         sel==1 ? algorithm_id_ripemd160 :
                  algorithm_id_sha;
}
  
unsigned t_hash::get_hash_algorithm_id_length(void)
{ return sel==0 ? algorithm_id_ripemd128_length :
         sel==1 ? algorithm_id_ripemd160_length :
                  algorithm_id_sha_length;
}

const unsigned * t_hash::get_hash_algorithmRSA_id(void)
{ return sel==0 ? algorithm_id_rsaSigWithMD128 :
         sel==1 ? algorithm_id_rsaSigWithMD160 :
                  algorithm_id_rsaSigWithSHA;
}
  
unsigned t_hash::get_hash_algorithmRSA_id_length(void)
{ return sel==0 ? algorithm_id_rsaSigWithMD128_length :
         sel==1 ? algorithm_id_rsaSigWithMD160_length :
                  algorithm_id_rsaSigWithMD128_length;
}

bool t_hash::is_error(void)
{ return sel==-1; }
//////////////////////////////////////////// miniature ///////////////////////////////////////////////////////
void make_miniature_name(const char * filename, char * min_name)
{ int i=(int)strlen(filename);
  if      (filename[i-1]=='.') i-=1; // empty suffix
  else if (filename[i-2]=='.') i-=2; // 1-letter suffix
  else if (filename[i-3]=='.') i-=3; // 2-letter suffix
  else if (filename[i-4]=='.') i-=4; // 3-letter suffix
  strcpy(min_name, filename);  strcpy(min_name+i, ".hsh");
}

bool make_miniature(const char * filename)
{// calculate the digest: 
  tFileStream str;
  if (!str.open_input(filename)) return false;
  t_hash hs(NULL);  unsigned char digest[MAX_DIGEST_LENGTH];  unsigned digest_length;
  str.get_digest(*hs.get_hash_module(), digest, &digest_length);
  str.close_input();
 // open file for the miniature:
  char min_name[MAX_PATH];
  make_miniature_name(filename, min_name);
  if (!str.open_output(min_name)) return false;
  str.save_dir((int)strlen(hs.hash_name())+1, 1);
  str.save_val(hs.hash_name(), (int)strlen(hs.hash_name())+1);
  str.save_val(digest, digest_length);
  str.close_output();
 // create the file with the text form of the miniature:
  strcpy(min_name+strlen(min_name)-3, "txt");
  if (!str.open_output(min_name)) return false;
  const char * p;  unsigned i;
  p=MSG28;
  str.save_val(p, (int)strlen(p));
  i=(unsigned)strlen(filename);
  while (i && filename[i-1]!=PATH_SEPARATOR) i--;
  str.save_val(filename+i, (int)strlen(filename+i));
  str.save_val("\r\n", 2);
  p=MSG29;
  str.save_val(p, (int)strlen(p));
  str.save_val(hs.hash_name(), (int)strlen(hs.hash_name()));
  str.save_val("\r\n", 2);
  for (i=0;  i<digest_length;  i++)
  { if (!(i & 1)) str.save_val(" ", 1);
    str.save_dir(num2hex(digest[i] / 16), 1);
    str.save_dir(num2hex(digest[i] % 16), 1);
  }
  str.save_val("\r\n", 2);
  str.close_output();
  return true;
}

bool check_miniature(const char * filename, const char * min_name, const char * direct_hsh)
{// open file with the miniature:
  tFileStream str;
  CryptoPP::HashModule * md;  unsigned char stored_digest[MAX_DIGEST_LENGTH];  unsigned stored_digest_length;
  t_hash * hs;
  if (min_name)
  { unsigned char len;  char hash_name[20];
    if (!str.open_input(min_name)) return false;
    str.load_val(&len, 1);
    if (len>=sizeof(hash_name)) return false;
    str.load_val(hash_name, len);  
    hs = new t_hash(hash_name);  
  }
  else 
    hs = new t_hash((t_algors)direct_hsh[0]);  
  if (!hs) return false;
  if (hs->is_error()) { delete hs;  return false; }
  md = hs->get_hash_module();
  stored_digest_length = md->DigestSize();
  if (min_name)
  { str.load_val(stored_digest, stored_digest_length);
    str.close_input();
  }
  else                                           
  { if (stored_digest_length!=direct_hsh[1]) { delete hs;  return false; }
    memcpy(stored_digest, direct_hsh+2, stored_digest_length);
  }
 // calculate the digest: 
  if (!str.open_input(filename)) { delete hs;  return false; }
  unsigned char digest[MAX_DIGEST_LENGTH];  unsigned digest_length;
  str.get_digest(*md, digest, &digest_length);
  str.close_input();
  delete hs;  
  return !memcmp(digest, stored_digest, stored_digest_length);
}

DllSecur bool WINAPI create_self_certified_keys(char * random_data, unsigned char ** pcert_str, unsigned * pcert_len, unsigned char ** pprivkey_str, unsigned * pprivkey_len)
{ t_user_ident ui;
  strcpy(ui.name1,   "");
  strcpy(ui.name2,   "");
  strcpy(ui.name3,   "");
  strcpy(ui.identif, "602SQL Server");
  t_keygen_info ki(&ui, NULL, true);
  *pcert_str=NULL;  *pprivkey_str=NULL;
 // create a key pair:
  ki.signedreq=false;                       
  ki.keytype=KEYTYPE_RCA;
  strncpy(ki.seed, random_data, sizeof(ki.seed));
  ki.signedreq=false;
  ki.make_key_pair(0);
 // save private key (non-encrypted):
  tMemStream str;
  t_asn_private_key * privKey = new t_asn_private_key(ki.priv);
  if (!privKey) return false;
  unsigned key_length = privKey->der_length(true);
  str.open_output(key_length);
  privKey->save(&str);
  delete privKey;
  if (str.any_error()) return false;
  *pprivkey_len = str.extent();
  *pprivkey_str = str.close_output();

 // make self-signed certificate from the request:
  SYSTEMTIME CurrentTime;  GetLocalTime(&CurrentTime);
  SYSTEMTIME StopTime = CurrentTime;
  systemtime_add(StopTime, 3,0,10,0,0,0);  // 3 years
  char start_utc_time[18];  char end_utc_time[18];
  systemtime2UTC(CurrentTime, start_utc_time);
  systemtime2UTC(StopTime,    end_utc_time);
  unsigned char new_cert_uuid[12];  memset(new_cert_uuid, 0, sizeof(new_cert_uuid));
  t_Certificate * cert = make_certificate_from_request(ki.cert_req, KEYTYPE_RCA, start_utc_time, end_utc_time, 
    NULL, 0, NULL, new_cert_uuid, ki.priv, 0);
  if (!cert) return false;
  if (!str.open_output(cert->der_length(true))) { delete cert;  return false; }
  cert->save(&str);
  delete cert;
  if (str.any_error()) return false;
  *pcert_len = str.extent();
  *pcert_str = str.close_output();
  return true;
}

DllSecur void WINAPI free_unschar_arr(unsigned char * str)
{ delete [] str; }


#ifdef WITH_GUI
bool t_keygen_info::save_cert_request(HWND hParent, unsigned char ** cert_req_str, unsigned * cert_req_len)
{ tMemStream str;
  if (!str.open_output(cert_req->der_length(true))) return false;
  cert_req->save(&str);
  if (str.any_error()) return false;
  if (signedreq)
  { t_asn_SignedData signedData;  
    tMemStream str2;
    if (!str2.open_output(30000)) return false;
    CryptoPP::InvertibleRSAFunction * sign_private_key;
    sign_private_key=get_private_key_for_certificate(hParent, user_ident.identif, cert_for_signing_request);
    if (!sign_private_key) return false;
    signedData.fill(data_content_object_id, data_content_object_id_length, NULL, &str, cert_for_signing_request);
    signedData.save_ci(&str2, cert_for_signing_request, sign_private_key);
    *cert_req_len = str2.extent();
    *cert_req_str = str2.close_output();
  }
  else // request is not signed
  { *cert_req_len = str.extent();
    *cert_req_str = str.close_output();
   // create the miniature:
    //make_miniature(certreq);
  }
  return true;
}

bool SaveFile(HWND hParent, char * pathname, BOOL overwrite_prompt)
{ OPENFILENAME ofn;  BOOL res;  char inidir[200];

  memset(&ofn, 0, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hParent;
  ofn.hInstance=hProgInst;
  ofn.lpstrFile= pathname;
  ofn.nMaxFile = MAX_PATH;
  int i=strlen(pathname);
  while (i && pathname[i-1]!=PATH_SEPARATOR) i--;
  if (i)  { memcpy(inidir, pathname, i-1);  inidir[i-1]=0; }
  else *inidir=0;
  ofn.lpstrInitialDir = inidir;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_EXPLORER;
  if (overwrite_prompt) ofn.Flags |= OFN_OVERWRITEPROMPT;
  res = GetSaveFileName(&ofn);
  if (!res)
  { CommDlgExtendedError();
    return false;
  }
  strcpy(pathname, ofn.lpstrFile);
  return true;
}

bool SelectFile(HWND hParent, char * pathname, const char * title, const char * filtersIn)
{ OPENFILENAME ofn;  BOOL res;  char inidir[200];  char filters[60];  int filterpos;

  memset(&ofn, 0, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hParent;
  ofn.hInstance=hProgInst;
  ofn.lpstrFile= pathname;
  ofn.nMaxFile = MAX_PATH;
  int i=strlen(pathname);
  if (filtersIn)
  { strcpy(filters,           filtersIn);  filterpos=strlen(filtersIn)+1;
    strcpy(filters+filterpos, filtersIn);  filterpos*=2;
  }
  else 
    filterpos=0;
  while (i && pathname[i-1]!=PATH_SEPARATOR) 
  { if (pathname[i-1]=='.' && !filterpos && strlen(pathname+i)<=3)
    { sprintf(filters+filterpos, "*.%s%c*.%s", pathname+i, 0, pathname+i);
      filterpos+=2*(2+strlen(pathname+i)+1);
    }
    i--;
  }
  if (i)  { memcpy(inidir, pathname, i-1);  inidir[i-1]=0; }
  else *inidir=0;
  ofn.lpstrInitialDir = inidir;
  ofn.lpstrTitle = title;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_SHOWHELP | OFN_EXPLORER;
 // add the general filter:
  memcpy(filters+filterpos, "*.*\0*.*\0", 9);
  ofn.lpstrFilter=filters;
  ofn.nFilterIndex=1;
  res = GetOpenFileName(&ofn);
  if (!res)
  { CommDlgExtendedError();
    return false;
  }
  strcpy(pathname, ofn.lpstrFile);
  return true;
}

////////////////////////////////////////// reading password from a file //////////////////////////////
static char x_passwd[MAX_PASSWORD_LEN+1];
static bool x_retain;

static UINT CALLBACK KeySelHook(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
{ 
	switch (msg)
  { case WM_INITDIALOG:  case WM_CREATE:
      SendDlgItemMessage(hDlg, CD_PRIVKEY_PASS, EM_LIMITTEXT, sizeof(x_passwd), 0);
      SetFocus(GetDlgItem(hDlg, CD_PRIVKEY_PASS));  // does not work
      break;
    case WM_COMMAND:  /* OLD NT uses this */
      if ((GET_WM_COMMAND_ID(wP,lP)==1))
      { GetDlgItemText(hDlg, CD_PRIVKEY_PASS, x_passwd, sizeof(x_passwd));
        x_retain = IsDlgButtonChecked(hDlg, CD_PRIVKEY_RETAIN)==TRUE;
      }
      break;
    case WM_NOTIFY:
      switch (((NMHDR*)lP)->code)
      { case CDN_FILEOK:
        { x_retain = IsDlgButtonChecked(hDlg, CD_PRIVKEY_RETAIN)==TRUE;
          GetDlgItemText(hDlg, CD_PRIVKEY_PASS, x_passwd, sizeof(x_passwd));
          if (!*x_passwd)
          { MessageBox(hDlg, MSG15, MSG12, MB_OK|MB_APPLMODAL|MB_ICONSTOP);
            SetFocus(GetDlgItem(hDlg, CD_PRIVKEY_PASS));
            SetWindowLong(hDlg, DWL_MSGRESULT, -1);  // prevent deactivating
            return 1; // nonzero!
          }
        }
        case CDN_INITDONE:
          SetFocus(GetDlgItem(hDlg, CD_PRIVKEY_PASS));  // does not work! :-(
          SetTimer(hDlg, 12345, 300, NULL);
          break;
      }
      break;
    case WM_TIMER:
      if (wP==12345)
      { KillTimer(hDlg, 12345);
        SetFocus(GetDlgItem(hDlg, CD_PRIVKEY_PASS));  // this works, stupid Microsoft!
      }
      break;
  }
  return 0;
}


bool OpenPrivateKeyFile(HWND hParent, char * pathname, const char * title, char * password, bool * retain)
{ OPENFILENAME ofn;  BOOL res;  char inidir[200];

  memset(&ofn, 0, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hParent;
  ofn.hInstance=hProgInst;
  ofn.lpstrFile= pathname;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpTemplateName=MAKEINTRESOURCE(DLG_EOPENKEY);
  int i=strlen(pathname);
  while (i && pathname[i-1]!=PATH_SEPARATOR) i--;
  if (i)  { memcpy(inidir, pathname, i-1);  inidir[i-1]=0; }
  else *inidir=0;
  ofn.lpstrTitle = title;
  ofn.lpstrInitialDir = inidir;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | /*OFN_SHOWHELP |*/ OFN_EXPLORER  | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
  ofn.lpfnHook=KeySelHook;
  x_retain=false;

  res = GetOpenFileName(&ofn);
  if (!res)
  { CommDlgExtendedError();
    return false;
  }
  strcpy(pathname, ofn.lpstrFile);
  strcpy(password, x_passwd);
  *retain=x_retain;
  return true;
}


static BOOL APIENTRY KeygenWizard1DlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
// Select the key type
{ t_keygen_info * ki;  
  switch (msg)
  { case WM_INITDIALOG:
      ki = (t_keygen_info *)((PROPSHEETPAGE*)lP)->lParam;
      SetWindowLong(hDlg, DWL_USER, (long)ki);
      EnableWindow(GetDlgItem(hDlg, CD_KG_ROOTKEY), ki->for_root);
      return TRUE;
    case WM_NOTIFY:
      ki = (t_keygen_info *)GetWindowLong(hDlg, DWL_USER);
      switch (((NMHDR*)lP)->code)
      { case PSN_SETACTIVE:
          CheckRadioButton(hDlg, CD_KG_SIGNKEY, CD_KG_ROOTKEY, ki->keytype==KEYTYPE_SIGN ? CD_KG_SIGNKEY : ki->keytype==KEYTYPE_CERTIFY ? CD_KG_CAKEY : CD_KG_ROOTKEY);
          PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT); 
          return 0;  /* OK */
        case PSN_KILLACTIVE:  case PSN_WIZFINISH:
          if      (IsDlgButtonChecked(hDlg, CD_KG_SIGNKEY)) ki->keytype=KEYTYPE_SIGN;
          else if (IsDlgButtonChecked(hDlg, CD_KG_CAKEY))   ki->keytype=KEYTYPE_CERTIFY;
          else { ki->signedreq=false;                       ki->keytype=KEYTYPE_RCA; }
          return TRUE;
      }
      return FALSE;
  }
  return FALSE;
}

static WNDPROC OrigSeedWndProc;

static LRESULT CALLBACK SeedWndProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP)
{ switch (msg)
  { case WM_KEYDOWN:
      if ((lP >> 30) & 1) return 0;  // eat the message on autorepeat
      if (GetKeyState(VK_CONTROL) & 0x8000)
        if (wP=='V' || wP=='C' || wP=='X' || wP=='Z') 
          return 0;  // eat CTRL V for paste, etc.
      if (wP==VK_INSERT && ((GetKeyState(VK_SHIFT) & 0x8000) || (GetKeyState(VK_CONTROL) & 0x8000))) return 0;  // eat CTRL V for paste etc
      if (wP==VK_DELETE && ((GetKeyState(VK_SHIFT) & 0x8000) || (GetKeyState(VK_CONTROL) & 0x8000))) return 0;  // eat 
      break;
    case WM_PASTE:
      return 0;
  }
  return CallWindowProc(OrigSeedWndProc, hWnd, msg, wP, lP);
}

bool check_seed(const char * seed, HWND hDlg)
{ if (strlen(seed) < KEY_SEED_MIN)
  { MessageBox(hDlg, MSG16, MSG12, MB_OK|MB_APPLMODAL);
    return false;
  }
 // check the distribution:
  int distr[256], i, limit;
  memset(distr, 0, sizeof(distr));
  for (i=0;  seed[i];  i++)
    distr[seed[i]]++;
  limit = strlen(seed) / 6;
  for (i=0;  i<256;  i++)
    if (distr[i] > limit)
    { MessageBox(hDlg, MSG17, MSG12, MB_OK|MB_APPLMODAL);
      return false;
    }
  return true;
}

static BOOL CALLBACK KeygenWizard2DlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
// Enter seed for generating the keys
{ t_keygen_info * ki;  
  switch (msg)
  { case WM_INITDIALOG:  // empty trigger_name and no selected table supposed
    { ki = (t_keygen_info *)((PROPSHEETPAGE*)lP)->lParam;
      SetWindowLong(hDlg, DWL_USER, (long)ki);
      HWND hEdit = GetDlgItem(hDlg, CD_KG_SEED);
      SendMessage(hEdit, EM_LIMITTEXT, KEY_SEED_MAX, 0);
      SetDlgItemInt(hDlg, CD_KG_REST, KEY_SEED_MIN, FALSE);
      OrigSeedWndProc = (WNDPROC)GetWindowLong(hEdit, GWL_WNDPROC);
      SetWindowLong(hEdit, GWL_WNDPROC, (DWORD)SeedWndProc);
      return TRUE;
    }
    case WM_COMMAND:
      ki = (t_keygen_info *)GetWindowLong(hDlg, DWL_USER);
      switch (GET_WM_COMMAND_ID(wP,lP))
      { case CD_KG_SEED:
        { if (GET_WM_COMMAND_CMD(wP,lP)!=EN_CHANGE) break;
          int written, diff;
          written = SendDlgItemMessage(hDlg, CD_KG_SEED, WM_GETTEXTLENGTH, 0, 0);
          diff = (KEY_SEED_MIN < written) ? 0 : KEY_SEED_MIN - written;
          SetDlgItemInt(hDlg, CD_KG_REST, diff, FALSE);
          return TRUE;
        }
      }
      return FALSE;
    case WM_NOTIFY:
      ki = (t_keygen_info *)GetWindowLong(hDlg, DWL_USER);
      switch (((NMHDR*)lP)->code)
      { case PSN_SETACTIVE:
          PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT|PSWIZB_BACK);
          return 0;  /* OK */
        case PSN_WIZNEXT:  case PSN_WIZFINISH:
          GetDlgItemText(hDlg, CD_KG_SEED, ki->seed, sizeof(ki->seed));
          if (!check_seed(ki->seed, hDlg))
            SetWindowLong(hDlg, DWL_MSGRESULT, -1);  // prevent deactivating
          else if (ki->cert_for_signing_request==NULL || ki->keytype==KEYTYPE_RCA)  // skip dialog 3
          { ki->make_key_pair(hDlg);
            SetWindowLong(hDlg, DWL_MSGRESULT, DLG_KEYGEN_WZ4);   // dialog 4
          }
          return TRUE;
      }
      return FALSE;
  }
  return FALSE;
}

static BOOL APIENTRY KeygenWizard3DlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
// Find the old private key, or opt for an unsigned request 
{ t_keygen_info * ki;  
  switch (msg)
  { case WM_INITDIALOG:  // empty trigger_name and no selected table supposed
      ki = (t_keygen_info *)((PROPSHEETPAGE*)lP)->lParam;
      SetWindowLong(hDlg, DWL_USER, (long)ki);
      return TRUE;
    case WM_NOTIFY:
      ki = (t_keygen_info *)GetWindowLong(hDlg, DWL_USER);
      switch (((NMHDR*)lP)->code)
      { case PSN_SETACTIVE:
          PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT|PSWIZB_BACK); 
          CheckRadioButton(hDlg, CD_KG_NOTSIGNED, CD_KG_SIGNED, ki->signedreq ? CD_KG_SIGNED : CD_KG_NOTSIGNED);
          return 0;  /* OK */
        case PSN_WIZFINISH:  case PSN_WIZNEXT:
        { if      (IsDlgButtonChecked(hDlg, CD_KG_NOTSIGNED)) ki->signedreq=false;
          else if (IsDlgButtonChecked(hDlg, CD_KG_SIGNED))    ki->signedreq=true;
          ki->make_key_pair(hDlg);
          return TRUE;
        }
      }
      return FALSE;
  }
  return FALSE;
}

static BOOL APIENTRY KeygenWizard4DlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
// Select file and enter password for then new private key
{ t_keygen_info * ki;  
  switch (msg)
  { case WM_INITDIALOG:  // empty trigger_name and no selected table supposed
    { ki = (t_keygen_info *)((PROPSHEETPAGE*)lP)->lParam;
      SetWindowLong(hDlg, DWL_USER, (long)ki);
      SendDlgItemMessage(hDlg, CD_KG_PATH, EM_LIMITTEXT, sizeof(ki->privkey)-1, 0);
      if (!*ki->privkey) strcpy(ki->privkey, "a:");
      if (ki->privkey[strlen(ki->privkey)-1] != PATH_SEPARATOR) strcat(ki->privkey, PATH_SEPARATOR_STR);
      strcat(ki->privkey, ki->user_ident.identif);
      strcat(ki->privkey, ".prk");
      SetDlgItemText(hDlg, CD_KG_PATH, ki->privkey);
      SendDlgItemMessage(hDlg, CD_KG_PASS,  EM_LIMITTEXT, sizeof(ki->password)-1, 0);
      SendDlgItemMessage(hDlg, CD_KG_PASS2, EM_LIMITTEXT, MAX_PASSWORD_LEN,       0);
      return TRUE;
    }
    case WM_COMMAND:
      ki = (t_keygen_info *)GetWindowLong(hDlg, DWL_USER);
      switch (GET_WM_COMMAND_ID(wP,lP))
      { case CD_KG_SELFILE:
          GetDlgItemText(hDlg, CD_KG_PATH, ki->privkey, sizeof(ki->privkey));
          if (SaveFile(hDlg, ki->privkey, TRUE))
            SetDlgItemText(hDlg, CD_KG_PATH, ki->privkey);
          break;
        default: return FALSE;
      }
      return TRUE;
    case WM_NOTIFY:
      ki = (t_keygen_info *)GetWindowLong(hDlg, DWL_USER);
      switch (((NMHDR*)lP)->code)
      { case PSN_SETACTIVE:
          PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH|PSWIZB_BACK); 
          return 0;  /* OK */
        case PSN_KILLACTIVE:  
          GetDlgItemText(hDlg, CD_KG_PATH, ki->privkey, sizeof(ki->privkey));
          return TRUE;
        case PSN_WIZBACK:
          if (ki->cert_for_signing_request==NULL || ki->keytype==KEYTYPE_RCA)  // skip dialog 3
            SetWindowLong(hDlg, DWL_MSGRESULT, DLG_KEYGEN_WZ2);   // dialog 2
          return TRUE;
        case PSN_WIZFINISH:  case PSN_WIZNEXT:
        { char password2[MAX_PASSWORD_LEN+1];
          GetDlgItemText(hDlg, CD_KG_PATH, ki->privkey,  sizeof(ki->privkey));
          GetDlgItemText(hDlg, CD_KG_PASS, ki->password, sizeof(ki->password));
          GetDlgItemText(hDlg, CD_KG_PASS2, password2,   sizeof(password2));
          if (strlen(ki->password) < MIN_PASSWORD_LEN) 
          { show_error(MSG4, hDlg);
            SetWindowLong(hDlg, DWL_MSGRESULT, -1);  // prevent deactivating
          }
          else if (strcmp(ki->password, password2)) 
          { show_error(MSG5, hDlg);
            SetWindowLong(hDlg, DWL_MSGRESULT, -1);  // prevent deactivating
          }
          else if (!ki->save_private_key(hDlg))
            SetWindowLong(hDlg, DWL_MSGRESULT, -1);  // prevent deactivating
          return TRUE;
        }
      }
      return FALSE;
  }
  return FALSE;
}

void save_wizard_page(PROPSHEETPAGE * psp, UINT dlg_id, DLGPROC DialogProc, LPARAM lP)
{ psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_HASHELP;
  psp->hInstance = hProgInst;
  psp->pszTemplate = MAKEINTRESOURCE(dlg_id);
  psp->pszIcon = 0;
  psp->pfnDlgProc = DialogProc;
  psp->pszTitle = 0;
  psp->lParam = lP;
  psp->pfnCallback = NULL;
}

DllSecur int WINAPI create_key_pair(HWND hParent, bool for_root, const t_user_ident * user_ident, unsigned char * new_cert_uuid, 
         t_Certificate * cert_for_signing_request, unsigned char ** cert_req_str, unsigned * cert_req_len, 
         int certificate_number)
{ t_keygen_info ki(user_ident, cert_for_signing_request, for_root);
 // property sheet:
  PROPSHEETPAGE psp[4];  PROPSHEETHEADER psh;  
  save_wizard_page(psp,   DLG_KEYGEN_WZ1, KeygenWizard1DlgProc, (LPARAM)&ki);
  save_wizard_page(psp+1, DLG_KEYGEN_WZ2, KeygenWizard2DlgProc, (LPARAM)&ki);
  save_wizard_page(psp+2, DLG_KEYGEN_WZ3, KeygenWizard3DlgProc, (LPARAM)&ki);
  save_wizard_page(psp+3, DLG_KEYGEN_WZ4, KeygenWizard4DlgProc, (LPARAM)&ki);
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags = PSH_WIZARD | PSH_PROPSHEETPAGE;// | PSH_WIZARDHASFINISH;
  psh.hwndParent = hParent;
  psh.hInstance = hProgInst;
  psh.pszIcon = 0;
  psh.pszCaption = NULL;
  psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
  psh.nStartPage = 0;
  psh.ppsp = psp;
  psh.pfnCallback = NULL;
  if (PropertySheet(&psh)<=0)
  { MessageBox(hParent, MSG18, MSG19, MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);
    return 0;
  }
  if (ki.keytype==KEYTYPE_RCA)
  { SYSTEMTIME CurrentTime;  GetLocalTime(&CurrentTime);
    SYSTEMTIME StopTime = CurrentTime;
    systemtime_add(StopTime, 3,0,10,0,0,0);  // 3 years
    char start_utc_time[18];  char end_utc_time[18];
    systemtime2UTC(CurrentTime, start_utc_time);
    systemtime2UTC(StopTime,    end_utc_time);
    t_Certificate * cert = make_certificate_from_request(ki.cert_req, KEYTYPE_RCA, start_utc_time, end_utc_time, 
      NULL, hParent, NULL, new_cert_uuid, ki.priv, certificate_number);
    if (!cert)
      { MessageBox(hParent, MSG20, MSG19, MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);  return 0; }
    tMemStream str;
    if (!str.open_output(cert->der_length(true))) 
      { MessageBox(hParent, MSG20, MSG19, MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);  delete cert;  return 0; }
    cert->save(&str);
    delete cert;
    if (str.any_error()) 
      { MessageBox(hParent, MSG20, MSG19, MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);  return 0; }
    *cert_req_len = str.extent();
    *cert_req_str = str.close_output();
    MessageBox(hParent, MSG21, MSG19, MB_OK|MB_APPLMODAL);
    return 2;
  }
  else
  { if (!ki.save_cert_request(hParent, cert_req_str, cert_req_len))
     { MessageBox(hParent, MSG22, MSG19, MB_OK|MB_APPLMODAL|MB_ICONEXCLAMATION);  return 0; }
    MessageBox(hParent, MSG23, MSG19, MB_OK|MB_APPLMODAL|MB_ICONINFORMATION);
    return 1;
  }
}

#ifdef STOP
static BOOL CALLBACK ChangePassDlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
{ switch (msg)
  { case WM_INITDIALOG:
      SetWindowLong(hDlg, DWL_USER, lP);
      SendDlgItemMessage(hDlg, CD_CHP_NEW1, EM_LIMITTEXT, MAX_PASSWORD_LEN, 0);
      SendDlgItemMessage(hDlg, CD_CHP_NEW2, EM_LIMITTEXT, MAX_PASSWORD_LEN, 0);
      return TRUE;
    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wP,lP))
      { case IDOK:
        { char pass1[MAX_PASSWORD_LEN+1], pass2[MAX_PASSWORD_LEN+1];
          GetDlgItemText(hDlg, CD_CHP_NEW1, pass1, sizeof(pass1));
          GetDlgItemText(hDlg, CD_CHP_NEW1, pass2, sizeof(pass2));
          if (strlen(pass1) < MIN_PASSWORD_LEN) 
            { show_error(MSG4, hDlg);  break; }
          if (strcmp(pass1, pass2))
            { show_error(MSG5, hDlg);  break; }
          char * dest = (char*)GetWindowLong(hDlg, DWL_USER);
          strcpy(dest, pass1);
          EndDialog(hDlg, TRUE);  break;
        }
        case IDCANCEL:
          EndDialog(hDlg, FALSE);  break;
      }
      return TRUE;
  }
  return FALSE;
}


bool change_key_password(HWND hParent)
{ char privkeypath[MAX_PATH];      
  char password[MAX_PASSWORD_LEN+1],  new_password[MAX_PASSWORD_LEN+1];
 // find private key file and password:
  CryptoPP::InvertibleRSAFunction * priv;  t_Certificate * cert;
  if (!activate_private_key2(KEYTYPE_ENCSIG, hParent, &priv, &cert, NULL, privkeypath, password)) 
    return false;
  delete priv;  delete cert; 
 // enter new password:
  if (!DialogBoxParam(hProgInst, MAKEINTRESOURCE(DLG_CHNG_PASS), hParent, ChangePassDlgProc, (LPARAM)new_password))
    return false;
 // recode:
  tFileStream str;
  t_IDEA_key ikey;
  make_key_from_password(password, &ikey);
  CryptoPP::IDEADecryption ideadec((const unsigned char*)&ikey); 
  if (!str.open_input(privkeypath)) 
    { show_error(MSG1, hParent);  return false; }
  tMemStream mstr;
  unsigned enc_length = str.extent();
  mstr.open_output(enc_length);
  if (!stream_copy_decrypt(&str, &mstr, enc_length, NULL, ideadec))
    { show_error(MSG6, hParent);  return false; }
  str.close_input();
  if (!str.open_output(privkeypath)) 
    { MessageBox(hParent, MSG24, MSG12, MB_OK|MB_APPLMODAL);  return false; }
  mstr.reset();
  make_key_from_password(new_password, &ikey);
  CryptoPP::IDEAEncryption ideaenc((const unsigned char*)&ikey); 
  mstr.copy(&str, ideaenc);
  delete [] mstr.close_output();
  str.close_output();
  MessageBox(hParent, MSG25, MSG19, MB_OK|MB_APPLMODAL);
  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
t_name_part LCANameList[] = {
 { attr_type_country_name    , attr_type_length, "CZ" },
 { attr_type_organiz_name    , attr_type_length, "CNBCC-KMI" },
 { attr_type_org_unit_name   , attr_type_length, NULL },
 { attr_type_common_unit_name, attr_type_length, NULL },
 { NULL, 0, NULL } };

t_name_part RCANameList[] = {
 { attr_type_country_name    , attr_type_length, "CZ" },
 { attr_type_organiz_name    , attr_type_length, "CNBCC-KMI" },
 { attr_type_org_unit_name   , attr_type_length, NULL },
 { attr_type_common_unit_name, attr_type_length, NULL },
 { NULL, 0, NULL } };

#endif  // STOP

struct t_cert_creation
{ t_cert_req * cert_req;
  bool own_sig_ok;
  bool signature_present;
  bool signature_ok;
  bool miniature_ok; // defined only if !signature_present
  bool certified;
  char start_utc_time[18];
  char end_utc_time[18];
  t_cert_creation(void)
    { cert_req=NULL; }
  ~t_cert_creation(void)
    { delete cert_req; }
};


void fill_readable_name(const t_asn_Name * name, HWND hList)
{ char buf[250];
  while (name)
  { char * pbuf = buf;  unsigned len=sizeof(buf);
    name->get_readableRDN(pbuf, len);
    SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)buf);
    name=name->next;
  }
}



static BOOL APIENTRY CertifyDlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
{ t_cert_creation * cc;
  switch (msg)
  { case WM_INITDIALOG:  
    { cc = (t_cert_creation *)lP;
      SetWindowLong(hDlg, DWL_USER, (long)cc);
      bool enabled = false;
     // display info from the request:
      fill_readable_name(cc->cert_req->info->subject, GetDlgItem(hDlg, CD_CRT_SUBJECT));
     // key type info:
      char keytype_str[80], buf[100];
      t_asn_cri_atribute * attribute = cc->cert_req->info->attributes;
      while (attribute && !attribute->is_type(cri_attr_type_unstructured_name, cri_attr_type_unstructured_name_length))
        attribute=attribute->next;
      if      (!cc->cert_req->info->attributes)
        strcpy(keytype_str, MSG30);
      else if (cc->cert_req->info->attributes->equal(cri_attribute_signature))
      { strcpy(keytype_str, MSG31);  enabled = true; }
      else if (cc->cert_req->info->attributes->equal(cri_attribute_encryption))
      { strcpy(keytype_str, MSG32);  enabled = true; }
      else if (cc->cert_req->info->attributes->equal(cri_attribute_CA))
      { strcpy(keytype_str, MSG33);  enabled = true; }
      else if (cc->cert_req->info->attributes->equal(cri_attribute_RCA))
      { strcpy(keytype_str, MSG34); }
      else
        strcpy(keytype_str, MSG35);
      SetDlgItemText(hDlg, CD_CRT_KEYTYPE, keytype_str);
     // own signature:
      if (cc->own_sig_ok) strcpy(buf, MSG36);
      else 
      { strcpy(buf, MSG37);  enabled=false; }
      SetDlgItemText(hDlg, CD_CRT_SELFSIGN, buf);
     // external signature:
      if (!cc->signature_present) strcpy(buf, MSG38);
      else if (cc->signature_ok) strcpy(buf, MSG39);
      else { strcpy(buf, MSG40);  enabled=false; }
      SetDlgItemText(hDlg, CD_CRT_PREVSIGN, buf);
#ifdef STOP
     // miniature
      if (cc->signature_present) 
        strcpy(buf, "Miniaturu není tøeba ovìøovat");
      else if (cc->miniature_ok)
        strcpy(buf, "Miniatura souhlasí");
      else
      { strcpy(buf, "Miniatura nesouhlasí, proto nelze certifikovat");  enabled=false; }
      SetDlgItemText(hDlg, CD_CRT_MINIATURA, buf);
#endif
     // propose date and time of certificate validity:
      SYSTEMTIME CurrentTime;  GetLocalTime(&CurrentTime);
      DateTime_SetSystemtime(GetDlgItem(hDlg, CD_CRT_STARTDT), GDT_VALID, &CurrentTime);
      DateTime_SetSystemtime(GetDlgItem(hDlg, CD_CRT_STARTTM), GDT_VALID, &CurrentTime);
      SYSTEMTIME PlusTime = CurrentTime;
      if (cc->cert_req->info->attributes->equal(cri_attribute_RCA))
        systemtime_add(PlusTime, 3,0,10,0,0,0);
      else if (cc->cert_req->info->attributes->equal(cri_attribute_CA))
        systemtime_add(PlusTime, 2,0,7,0,0,0);
      else
        systemtime_add(PlusTime, 1,0,5,0,0,0);
      DateTime_SetSystemtime(GetDlgItem(hDlg, CD_CRT_STOPDT), GDT_VALID, &PlusTime);
      DateTime_SetSystemtime(GetDlgItem(hDlg, CD_CRT_STOPTM), GDT_VALID, &PlusTime);
      SYSTEMTIME Limits[1];  Limits[0]=CurrentTime;
      DateTime_SetRange(GetDlgItem(hDlg, CD_CRT_STARTDT), GDTR_MIN, Limits); // cannot limit time because day change is not recognized by the time control
      DateTime_SetRange(GetDlgItem(hDlg, CD_CRT_STOPDT),  GDTR_MIN, Limits);

      if (!enabled)
      { EnableWindow(GetDlgItem(hDlg, CD_CRT_OK), false);
        EnableWindow(GetDlgItem(hDlg, CD_CRT_STARTDT), false);  EnableWindow(GetDlgItem(hDlg, CD_CRT_STARTTM), false);
        EnableWindow(GetDlgItem(hDlg, CD_CRT_STOPDT ), false);  EnableWindow(GetDlgItem(hDlg, CD_CRT_STOPTM ), false);
      }
      return TRUE;
    }
    case WM_COMMAND:
      cc = (t_cert_creation *)GetWindowLong(hDlg, DWL_USER);
      switch (GET_WM_COMMAND_ID(wP,lP))
      { 
        case CD_CRT_OK:
         // check the validity settings:
        { SYSTEMTIME StartTime, EndTime, AuxTime;
          DateTime_GetSystemtime(GetDlgItem(hDlg, CD_CRT_STARTDT), &StartTime);
          DateTime_GetSystemtime(GetDlgItem(hDlg, CD_CRT_STARTTM), &AuxTime);
          StartTime.wHour=AuxTime.wHour;  StartTime.wMinute=AuxTime.wMinute;  StartTime.wSecond=AuxTime.wSecond;
          DateTime_GetSystemtime(GetDlgItem(hDlg, CD_CRT_STOPDT),  &EndTime);
          DateTime_GetSystemtime(GetDlgItem(hDlg, CD_CRT_STOPTM),  &AuxTime);
          EndTime.wHour=AuxTime.wHour;  EndTime.wMinute=AuxTime.wMinute;  EndTime.wSecond=AuxTime.wSecond;
          if (compare_systime(StartTime, EndTime) >= 0)
            { show_error(MSG7, hDlg);  break; }
          SYSTEMTIME PlusTime = StartTime;
          if (cc->cert_req->info->attributes->equal(cri_attribute_RCA))
            systemtime_add(PlusTime, 3,0,10,0,0,0);
          else if (cc->cert_req->info->attributes->equal(cri_attribute_CA))
            systemtime_add(PlusTime, 2,0,7,0,0,0);
          else
            systemtime_add(PlusTime, 1,0,5,0,0,0);
          if (compare_systime(PlusTime, EndTime) < 0)
            { show_error(MSG8, hDlg);  break; }
          SYSTEMTIME CurrentTime;  GetLocalTime(&CurrentTime);
          PlusTime = StartTime;  systemtime_add(PlusTime, 0,0,0,0,20,0);  // 20 minutes max dialog open
          if (compare_systime(PlusTime, CurrentTime) < 0)
            { show_error(MSG9, hDlg);  break; }
          systemtime2UTC(StartTime, cc->start_utc_time);
          systemtime2UTC(EndTime,   cc->end_utc_time);
          cc->certified=true;
          EndDialog(hDlg, TRUE);  break;
        }
        case CD_CRT_CANCEL:   case IDCANCEL:
          cc->certified=false;
          EndDialog(hDlg, FALSE);  break;
      }
      return TRUE;
  }
  return FALSE;
}

t_message_type analyse_message_type(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    return MSGT_UNKNOWN; 
  str->load_length();  // outer length ignored
  if (str->check_tag(tag_universal_objectid, false))
  { t_asn_objectid objid;
    objid.load(str);
    if (objid.is(signed_data_content_object_id, signed_data_content_object_id_length))
      return MSGT_SIGNED;
    if (objid.is(sigenv_data_content_object_id, sigenv_data_content_object_id_length))
      return MSGT_ENCSIGNED;
    if (objid.is(       data_content_object_id,        data_content_object_id_length))
      return MSGT_DATA;
  }
  else
  { if (!str->check_tag(tag_universal_constructed_sequence))
      return MSGT_UNKNOWN; 
    str->load_length();  // length ignored
    if (str->check_tag(tag_universal_integer, false))
      return MSGT_CERTREQ;
  }
  return MSGT_UNKNOWN; 
}

DllSecur BOOL WINAPI make_certificate_from_request_str(HWND hParent, unsigned char * request, unsigned request_len, 
       const t_Certificate * ca_cert, const char * certifier_name,  const unsigned char * ca_cert_uuid,
       unsigned char ** cert_str, unsigned * cert_len, SYSTEMTIME * starttime, SYSTEMTIME * stoptime, bool * is_ca_cert, 
       int certificate_number)
{// read the request:
  tMemStream str;
  if (!str.open_input(request, request_len)) return FALSE;
  t_message_type msgtype = analyse_message_type(&str);
  t_cert_creation cc;  
  if (msgtype==MSGT_SIGNED)
    cc.signature_present=true;
  else if (msgtype==MSGT_CERTREQ)
    cc.signature_present=false;
  else
    { show_error(MSG10, hParent);  return FALSE; }
  str.reset();
  cc.cert_req = load_certification_request(&str, msgtype, &cc.signature_ok);
  str.close_input();
  if (!cc.cert_req) 
    { show_error(MSG11, hParent);  return FALSE; }
  cc.certified=false;
  cc.own_sig_ok=cc.cert_req->verify_own_signature();
 // display the request in a dialog:
  DialogBoxParam(hProgInst, MAKEINTRESOURCE(DLG_CRECERT), hParent, CertifyDlgProc, (LPARAM)&cc);
  if (!cc.certified) 
    { MessageBox(hParent, MSG26, MSG19, MB_OK|MB_APPLMODAL);  return FALSE; }
 // certify and save:
  t_Certificate * cert = make_certificate_from_request(cc.cert_req, KEYTYPE_ENCSIG, cc.start_utc_time, cc.end_utc_time, certifier_name, 
                                                       hParent, ca_cert, ca_cert_uuid, NULL, certificate_number);
  if (!cert) return FALSE;
 // serialise the new certificate:
  tMemStream ostr;
  if (!ostr.open_output(cert->der_length(true))) return FALSE;
  cert->save(&ostr);
  if (ostr.any_error()) return FALSE;
  *cert_len = ostr.extent();
  *cert_str = ostr.close_output();
  *starttime = *cert->TbsCertificate->Validity->get_start()->get_system_time();
  *stoptime  = *cert->TbsCertificate->Validity->get_end  ()->get_system_time();
  *is_ca_cert = cc.cert_req->info->attributes->equal(cri_attribute_CA) || cc.cert_req->info->attributes->equal(cri_attribute_RCA);
  delete cert;
  return TRUE;
}


#endif  // WITH_GUI
