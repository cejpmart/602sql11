void show_error(const char * msg, HWND hParent);

CryptoPP::Integer generic_sign(t_hash * hs, const unsigned char * digest, unsigned digest_length, const CryptoPP::InvertibleRSAFunction *priv);

#define MAX_KEY_FILE_NAME_LEN 40

// certificate database dbcert.cpp
struct t_cert_info
{ t_readable_rdn_name rdblSubject;
  t_readable_rdn_name rdblIssuer;
  unsigned            number;
  t_keytypes          keytype;
  SYSTEMTIME          starttime, endtime;
  bool                is_valid(void) const;
  const SYSTEMTIME *  revoked;
  char                fname[MAX_KEY_FILE_NAME_LEN+1];
  int                 source;
  t_cert_info *       next;
  t_Certificate *     certif;  // not NULL iff loaded, owning ptr
  t_cert_info(void);
  t_cert_info(const t_Certificate * cert, bool take_and_save);
  ~t_cert_info(void);
  void update_revocation_info(void);
};

class t_dbcert
{ t_cert_info * list;
 public:
  void add(const t_Certificate * certif, HWND hParent = 0, const char * orig_name = NULL);  // stores the certificate in the database, does not take the ownership
  t_Certificate * load(const char * fname);
  const t_Certificate * get_by_subject(const char * therdblSubject, t_keytypes keytype);   // returns non-owning pointer
  const t_Certificate * get_by_subject(const t_asn_Name * theSubject, t_keytypes keytype); // returns non-owning pointer
  const t_Certificate * get_by_issuer_and_num(const t_asn_Name * theIssuer, const t_asn_integer * theSerialNumber);  // returns non-owning pointer
  const t_cert_info * get_list(void) { return list; }
  void get_pathname(const t_cert_info * ci, char * pathname) const;
};

extern t_dbcert * the_dbcert;
extern t_dbcert * the_root_cert;

extern char inipathname[], LCAinipathname[];
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#define MAX_PASSWORD_LEN 100
#define num2hex(x) ((x)<10  ? (x)+'0' : 'A'+(x)-10)
#define hex2num(x) ((x)<'A' ? (x)-'0' : 10+(x)-'A')

bool OpenPrivateKeyFile(HWND hParent, char * pathname, const char * title, char * password, bool * retain);
bool activate_private_key(t_keytypes keytype, HWND hParent, CryptoPP::InvertibleRSAFunction ** ppriv, t_Certificate ** pcert,
       const t_asn_RecipientInfo * recip = NULL);
CryptoPP::InvertibleRSAFunction * load_private_rsa(const char * pathname, HWND hParent, const char * password);

extern t_Certificate * current_sign_certif;
extern CryptoPP::InvertibleRSAFunction * current_sign_private_key;
extern t_Certificate * current_enc_certif;
extern CryptoPP::InvertibleRSAFunction * current_enc_private_key;
extern t_Certificate * current_cert_certif;
extern CryptoPP::InvertibleRSAFunction * current_cert_private_key;
 
void progress_open(HWND hParent, unsigned top);
void progress_close(void);
void progress_set_text(const char * text);
void progress_set_value(unsigned val, unsigned top);

#ifdef WINS
extern HINSTANCE hProgInst;
#endif
void make_key_from_password(const char * pass, t_IDEA_key * pikey);

#include "wbsecur.h"

