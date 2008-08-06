// fulltext modules:
#include "ftinc.h"
#include "ftinc.cpp"

#ifdef WINS
#include <sys/types.h> // _stat for t_input_convertor_ext_file_to_file
#include <sys/stat.h> // _stat for t_input_convertor_ext_file_to_file
#include <process.h> // _spawnl for t_input_convertor_ext_file_to_file
#include <direct.h> // _mkdir
#else
#include <stdarg.h> // va_start and other for t_input_convertor::request_errorf()
#include <sys/stat.h> // mkdir() stat()
#include <sys/types.h> // mkdir() waitpid() stat()
#include <sys/wait.h> // waitpid() 
#include <unistd.h> // stat()
#endif

#ifdef LINUX
#define NO_ASM
#define NO_MEMORYMAPPEDFILES
#include <errno.h>
#endif
#define ZIP602_STATIC
#include "Zip602.h"

#ifdef LINUX
#define NO_HOPE
#include <dlfcn.h>
#define wcsicmp wcscasecmp
#endif

#include <limits.h> // INT_MAX

bool get_fulltext_file_name(cdp_t cdp, char * fname, const char * schemaname, const char *suffix)
{
  if (!*the_sp.ExtFulltextDir.val())
    strcpy(fname, ffa.last_fil_path());
  else
    strcpy(fname, the_sp.ExtFulltextDir.val());
  append(fname, schemaname);
  strcat(fname, ".");
  strcat(fname, suffix);
  strcat(fname, EXT_FULLTEXT_SUFFIX);
  return true;
}

#ifdef WINS
static wbbool ole_initalized = wbfalse;
#endif

#if defined WINS && defined SRVR
// CS which guards load of msgsupp.dll library (e-mail parser/converter utility for fulltext)
// msgsupp.dll is for Win32 only
CRITICAL_SECTION cs_ftx_msgsupp_load;
#endif


struct t_ft_key32   // INT id
{ t_wordid wordid;
  uns32    docid;
  unsigned position;
  trecnum  recnum;
};

#pragma pack(1)
struct t_ft_key64   // BIGINT id
{ t_wordid wordid;
  t_docid  docid;
  unsigned position;
  trecnum  recnum;
};
#pragma pack()

struct t_wordkey
{ char word[MAX_FT_WORD_LEN];
  trecnum recnum;
  t_wordkey(const char * wrdIn)
  { strmaxcpy(word, wrdIn, MAX_FT_WORD_LEN+1);
    recnum=0;
  }
};

#define MAX_REL_PATH 255

struct t_docpart_rec32  // must not have constructor
{ uns32    docid;
  unsigned position;
  char rel_path[MAX_REL_PATH+1];  // +1 is not part of the record but make easier to write the full length value
};

struct t_docpart_rec64  // must not have constructor
{ t_docid  docid;
  unsigned position;
  char rel_path[MAX_REL_PATH+1];  // +1 is not part of the record but make easier to write the full length value
};

#include "extbtree.h"

struct t_ft_kontext;

static t_ft_kontext * global_ftk = NULL;

struct t_ft_kontext : t_ft_kontext_base
// Class for accessing the fulltext system data: inserting words, removing documents etc.
{// access control: 
  unsigned ref_cnt;
  unsigned AddRef(void)
    { return ++ref_cnt; }   
  unsigned Release(void)
    { if (--ref_cnt>0) return ref_cnt;
      delete this;  return 0;
    }
  bool destructed;
  virtual void destruct(void) = 0;
 // information about the FT:
  tobjname schemaname, suffix, doctabname;   // in version 9 doctabname is FTX_DOCTAB, unless specified different in the label. Not used.
  WBUUID schema_uuid;
  BOOL basic_form;
  int language;  // language index 0-CS, 1-SK etc., indexes the array of dictionary names
  BOOL weighted;
  bool is_v9_system;
  bool separated;
  t_ft_kontext * next;
  ttablenum docpartnum;  // NOOBJECT when docpart table is not used
 private:
  ttablenum doctabnum;   // in version 9 not used except for summary items
 protected:
  int prepare_word(const char * wordIn, char * wordOut, t_specif specif, unsigned weight);
 public:
  virtual BOOL wordtab_open(cdp_t cdp) = 0;
  virtual void wordtab_close(void) = 0;
  virtual BOOL ft_find_word(cdp_t cdp, t_wordkey * wordkey, t_wordid * wordid) = 0;
  virtual BOOL ft_insert_word_occ(cdp_t cdp, t_docid docid, const char * word, unsigned position, unsigned weight, t_specif specif = t_specif()) = 0;
  virtual void ft_remove_doc_occ(cdp_t cdp, t_docid docid) = 0;
  virtual bool breaked(cdp_t cdp) const
    { return cdp->break_request!=0; }
  BOOL ft_index_doc(cdp_t cdp, t_docid docid, t_value * text, const char * format, int mode, t_specif specif);
  void ft_set_summary_item(cdp_t cdp, t_docid docid, unsigned infotype, const void * info);
  void ft_starting_document_part(cdp_t cdp, t_docid docid, unsigned position, const char * relative_path);
  BOOL ft_context9(cdp_t cdp, t_value * text, const char * format, int mode, t_specif specif, unsigned position, unsigned word_count, int pre_words, int post_words, const char * prefix, const char * suffix, t_value * res);
  virtual void mark_core(void);
  virtual bool purge(cdp_t cdp, bool locked) = 0;
  virtual void check(cdp_t cdp) 
    { }
  virtual void locking(cdp_t cdp, bool lock_it, bool write_lock)
    { } 
  t_ft_kontext(tobjname schemanameIn, WBUUID schema_uuidIn, tobjname suffixIn, tobjname doctabnameIn, ttablenum doctabnumIn, BOOL basic_formIn, int languageIn, BOOL weightedIn, bool v9_system, ttablenum docpartnumIn, const char *limits_valueIn, const char * word_startersIn, const char * word_insidersIn, bool bigint_idIn)
   : t_ft_kontext_base(limits_valueIn, word_startersIn, word_insidersIn, bigint_idIn)
  { destructed=false;
    strcpy(schemaname, schemanameIn);
    memcpy(schema_uuid, schema_uuidIn, UUID_SIZE);
    strcpy(suffix, suffixIn);
    strcpy(doctabname, doctabnameIn);
    doctabnum=doctabnumIn;    
    docpartnum=docpartnumIn;
    is_v9_system=v9_system;
    language=languageIn;  basic_form=basic_formIn;  weighted=weightedIn;  
   // the object will be co-owned by the [global_ftk] list
    ref_cnt=1;  next=global_ftk;  global_ftk=this;
  }
  virtual ~t_ft_kontext(void)  
  { 
  }
};

struct t_ft_kontext_intern : t_ft_kontext
{
  dd_index * refindx;  // not owning
  table_descr * ref_td;
 private:
  ttablenum wordtabnum;
  table_descr * word_tabdescr;
  sql_statement * rem_doc_ref_so;
 public:
  virtual BOOL wordtab_open(cdp_t cdp);
  virtual void wordtab_close(void);
  virtual BOOL ft_find_word(cdp_t cdp, t_wordkey * wordkey, t_wordid * wordid);
  virtual BOOL ft_insert_word_occ(cdp_t cdp, t_docid docid, const char * word, unsigned position, unsigned weight, t_specif specif = t_specif());
  virtual void ft_remove_doc_occ(cdp_t cdp, t_docid docid);
  virtual void mark_core(void);
  virtual bool purge(cdp_t cdp, bool locked);

  t_ft_kontext_intern(cdp_t cdp, tobjname schemanameIn, WBUUID schema_uuidIn, tobjname suffixIn, tobjname doctabnameIn, ttablenum wordtabnumIn, ttablenum reftabnumIn, ttablenum doctabnumIn, BOOL basic_formIn, int languageIn, BOOL weightedIn, bool v9_system, ttablenum docpartnumIn, const char *limits_value, const char * word_startersIn, const char * word_insidersIn, bool bigint_idIn)
   : t_ft_kontext(schemanameIn, schema_uuidIn, suffixIn, doctabnameIn, doctabnumIn, basic_formIn, languageIn, weightedIn, v9_system, docpartnumIn, limits_value, word_startersIn, word_insidersIn, bigint_idIn)
  { rem_doc_ref_so=NULL;
    separated=false;
    wordtabnum=wordtabnumIn;
    if (v9_system && wordtabnumIn!=NOTBNUM)
    { table_descr_auto word_td(cdp, wordtabnumIn);
      if (word_td->me())
	      word_specif = word_td->attrs[1].attrspecif;
    }
    else
      word_specif = t_specif(0,1,0,0);  // for ft_upcase
    ref_td = install_table(cdp, reftabnumIn, MAX_TABLE_DESCR_LEVEL);
    if (ref_td) refindx = &ref_td->indxs[0];
    word_tabdescr = NULL;
  }
  virtual void destruct(void)
  { if (rem_doc_ref_so) { delete rem_doc_ref_so;  rem_doc_ref_so=NULL; }  // deletes the next_stmt too
    refindx=NULL;
    if (ref_td) { unlock_tabdescr(ref_td);  ref_td=NULL; }
    destructed=true;
  }
  virtual ~t_ft_kontext_intern(void)  // must not be replaced by the base destructor, base destruct would be called
    { if (!destructed) destruct(); }
};

#if WBVERS>=950
class t_scratch_sorter;

struct t_ft_kontext_separated : t_ft_kontext
{
  ext_index_file eif;
  t_scratch_sorter * scratch_sorter;  // not NULL when creating from scratch

  virtual BOOL wordtab_open(cdp_t cdp) 
    { return TRUE; }
  virtual void wordtab_close(void)
    { }
  virtual BOOL ft_find_word(cdp_t cdp, t_wordkey * wordkey, t_wordid * wordid);
  virtual BOOL ft_insert_word_occ(cdp_t cdp, t_docid docid, const char * word, unsigned position, unsigned weight, t_specif specif = t_specif());
  virtual void ft_remove_doc_occ(cdp_t cdp, t_docid docid);
  virtual void mark_core(void);
  virtual bool purge(cdp_t cdp, bool locked);
  virtual void check(cdp_t cdp);
  virtual void locking(cdp_t cdp, bool lock_it, bool write_lock);

  t_ft_kontext_separated(cdp_t cdp, tobjname schemanameIn, WBUUID schema_uuidIn, tobjname suffixIn, tobjname doctabnameIn, BOOL basic_formIn, int languageIn, BOOL weightedIn, ttablenum docpartnumIn, const char *limits_value, const char * word_startersIn, const char * word_insidersIn, bool bigint_idIn)
  : t_ft_kontext(schemanameIn, schema_uuidIn, suffixIn, doctabnameIn, NOTBNUM, basic_formIn, languageIn, weightedIn, true, docpartnumIn, limits_value, word_startersIn, word_insidersIn, bigint_idIn)
    
  { separated=true;
    scratch_sorter=NULL;
    char fname[MAX_PATH];
    get_fulltext_file_name(cdp, fname, schemaname, suffix);
    if (!eif.open(fname))
    { eif.create(fname);  // file may have been deleted, re-create
      eif.open(fname);
    }
   // find word_specif:
    switch (language)
    {
#ifdef WINS
      case FT_LANG_CZ: word_specif=t_specif(0, 1, 0, 0);  break;
      case FT_LANG_SK: word_specif=t_specif(0, 1, 0, 0);  break;
      case FT_LANG_PL: word_specif=t_specif(0, 2, 0, 0);  break;
      case FT_LANG_FR: word_specif=t_specif(0, 3, 0, 0);  break;
      case FT_LANG_GR: word_specif=t_specif(0, 4, 0, 0);  break;
#else
      case FT_LANG_CZ: word_specif=t_specif(0, 128+1, 0, 0);  break;
      case FT_LANG_SK: word_specif=t_specif(0, 128+1, 0, 0);  break;
      case FT_LANG_PL: word_specif=t_specif(0, 128+2, 0, 0);  break;
      case FT_LANG_FR: word_specif=t_specif(0, 128+3, 0, 0);  break;
      case FT_LANG_GR: word_specif=t_specif(0, 128+4, 0, 0);  break;
#endif
      default:         word_specif=t_specif(0, 0, 0, 0);  break;
    }
  }
  virtual void destruct(void)
  { eif.close();
    destructed=true;
  }
  virtual ~t_ft_kontext_separated(void)  // must not be replaced by the base destructor, base destruct would be called
    { if (!destructed) destruct(); }
};


#endif
//////////////////////////////////// fulltext expression analysis //////////////////////////////////
enum t_ft_node_type { FT_AND, FT_OR, FT_WORD, FT_NEGWORD };

struct t_ft_node
{ const t_ft_node_type node_type;
  virtual ~t_ft_node(void) {}
  virtual void mark_core(void) = 0;
  virtual void open_index(cdp_t cdp, BOOL initial) = 0;
  virtual void close_index(cdp_t cdp) = 0;
  virtual unsigned satisfied(t_docid * docid, unsigned * position) const = 0;  // returns TRUE if expression is satisfied on the current DOC and returns its docid, returns minimal docid otherwise, NO_DOC_ID on eof
  virtual void next_doc(cdp_t cdp, t_docid min_docid) = 0; // advances all indicies which are on min_docid
  t_ft_node(t_ft_node_type node_typeIn) : node_type(node_typeIn) { }
};

struct t_ft_node_andor : t_ft_node
{ t_ft_node * left, * right;
  t_ft_node_andor(t_ft_node_type node_typeIn) : t_ft_node(node_typeIn)
    { left=right=NULL; }
  virtual ~t_ft_node_andor(void)
  { if (left) delete left;
    if (right) delete right;
  }
  virtual void mark_core(void)
  { mark(this);
    if (left) left->mark_core();
    if (right) right->mark_core();
  }
  virtual void open_index(cdp_t cdp, BOOL initial)
  { if (left) left->open_index(cdp, initial);
    if (right) right->open_index(cdp, initial);
  }
  virtual void close_index(cdp_t cdp)
  { if (left) left->close_index(cdp);
    if (right) right->close_index(cdp);
  }
  virtual unsigned satisfied(t_docid * docid, unsigned * position) const;
  virtual void next_doc(cdp_t cdp, t_docid min_docid)
  { left ->next_doc(cdp, min_docid);
    right->next_doc(cdp, min_docid);
  }
};

struct t_ft_node_word : t_ft_node
{ int         distance;     // 0 for exact phrase, >=1 for NEAR clause
  t_wordid    wordid;       // constant ID of the word or NO_WORD_ID if the word is not in the dictionary
  int         flags;        // -1 iff no flags specified
  t_ft_key32  curr_key32;   // used for index access in INT-ID systems, values copied immediately to curr_key
  t_ft_key64  curr_key;
  t_docid     current_docid;
  unsigned    current_weight, current_position;
  bool        documents_exhausted;  // false while curr_key contains info about an occurence of the word in a document
  const t_ft_kontext * fkt;     // fulltext kontext reference
  t_ft_node_word * next;  // linked list of words in the phrease or NEAR clause

  t_ft_node_word(t_ft_node_type node_typeIn, const t_ft_kontext * fktIn, int flagsIn) : 
    t_ft_node(node_typeIn)
      { fkt=fktIn;  flags=flagsIn;  current_docid=0;  next=NULL; }
  virtual ~t_ft_node_word(void)
  { if (next) delete next; 
  }
  virtual void mark_core(void)
  { mark(this);
    if (next) next->mark_core();
  }
  virtual unsigned satisfied(t_docid * docid, unsigned * position) const;
  virtual bool istep(cdp_t cdp) = 0;
    // Goes to the next occurrence of the same word [wordid]. Loads [curr_key] for it. Returns false if it does not exist.
  virtual void close_path(cdp_t cdp) = 0;
    // Closes passing the index for the word.
  virtual void next_doc(cdp_t cdp, t_docid min_docid);
  bool next_pos(cdp_t cdp);
  bool doc_step(cdp_t cdp, t_docid * docid);
  virtual unsigned calc_weight(cdp_t cdp, trecnum recnum) = 0;
  void close_index(cdp_t cdp)
  { close_path(cdp); 
    if (next) next->close_index(cdp);
  }
  void define_curr_key(void)
  { curr_key.wordid=curr_key32.wordid;
    curr_key.docid=(uns32)curr_key32.docid==NONEINTEGER ? NONEBIGINT : (t_docid)(sig64)(sig32)curr_key32.docid;
    curr_key.position=curr_key32.position;
    curr_key.recnum=curr_key32.recnum;
  }
};

struct t_ft_node_word_intern : t_ft_node_word
{ t_ft_kontext_intern * ftki;
  t_btree_acc bac;
  t_btree_read_lock btrl;
  virtual void open_index(cdp_t cdp, BOOL initial);

  t_ft_node_word_intern(t_ft_node_type node_typeIn, t_ft_kontext_intern * ftkiIn, int flagsIn)  : 
    t_ft_node_word(node_typeIn, ftkiIn, flagsIn), ftki(ftkiIn)
      { ftki->AddRef(); }
  virtual ~t_ft_node_word_intern(void)
    { if (bac.fcbn!=NOFCBNUM) data_not_closed(); // close_index() not called!  bac.close(SaferGetCurrTaskPtr);
      ftki->Release();
    }
  virtual bool istep(cdp_t cdp);
  virtual void close_path(cdp_t cdp);
  virtual unsigned calc_weight(cdp_t cdp, trecnum recnum);
};

#if WBVERS>=950
struct t_ft_node_word_separated : t_ft_node_word
{ t_ft_kontext_separated * ftks;
  cbnode_path path;
  bool index_read_locked;
  virtual void open_index(cdp_t cdp, BOOL initial);

  t_ft_node_word_separated(t_ft_node_type node_typeIn, t_ft_kontext_separated * ftksIn, int flagsIn)  : 
    t_ft_node_word(node_typeIn, ftksIn, flagsIn), ftks(ftksIn)
      { ftks->AddRef();  index_read_locked=false; }
  virtual ~t_ft_node_word_separated(void)
    { close_path(NULL);  // path would be closed in its destructor, but the lock must be released!
      ftks->Release();
    }
  virtual bool istep(cdp_t cdp);
  virtual void close_path(cdp_t cdp);
  virtual unsigned calc_weight(cdp_t cdp, trecnum recnum);
};
#endif


enum { DEFAULT_NEAR_DISTANCE=5 };

enum t_ft_symbol { FTS_END, FTS_WORD, FTS_AND, FTS_OR, FTS_NOT, FTS_LPAR, FTS_RPAR, FTS_NEAR, FTS_PHRASE, FTS_EQUAL };

struct t_ft_analyser
{ cdp_t cdp;
  t_ft_kontext * ftk;
  BOOL error;
  const char * ft_source_ptr;
  t_ft_symbol curr_sym;
  char curr_word[MAX_FT_WORD_LEN+1];
  t_specif ft_specif;  // copied from [ftk]
  t_ft_analyser(cdp_t cdpIn, const char * source, t_ft_kontext * ftkIn)
    { cdp=cdpIn;  ft_source_ptr=source;  ftk=ftkIn;  error=FALSE;  ft_specif=ftk->word_specif;  next_sym(); }
  t_ft_symbol next_sym(void);
  void ft_anal_term(t_ft_node ** fte, BOOL negate);
  void ft_anal_and(t_ft_node ** fte, BOOL negate);
  void ft_anal_or(t_ft_node ** fte, BOOL negate);
};

t_ft_symbol t_ft_analyser::next_sym(void)
{ unsigned char c;
  do
  { c=*ft_source_ptr;
    if (!c) return curr_sym=FTS_END;
    ft_source_ptr++;
    if (c=='(') return curr_sym=FTS_LPAR;
    if (c==')') return curr_sym=FTS_RPAR;
    if (c=='"') return curr_sym=FTS_PHRASE;
    if (c=='=') return curr_sym=FTS_EQUAL;
    if (ftk->FT9_WORD_STARTER(c, ft_specif.charset)) break;
  } while (TRUE);
 // word staring character found, read the word:
  int i=1;
  curr_word[0]=c;  c=*ft_source_ptr;
  while (ftk->FT9_WORD_CONTIN(c, ft_specif.charset))
  { ft_source_ptr++;
    if (i<MAX_FT_WORD_LEN) curr_word[i++]=c;
    c=*ft_source_ptr;
  } 
  curr_word[i]=0;
 // compare the word with the keywords:
  switch (*curr_word)
  { case 'a':  case 'A':
      if (!stricmp(curr_word, "AND" )) return curr_sym=FTS_AND;   break;
    case 'o':  case 'O':
      if (!stricmp(curr_word, "OR"  )) return curr_sym=FTS_OR;    break;
    case 'n':  case 'N':
      if (!stricmp(curr_word, "NOT" )) return curr_sym=FTS_NOT;   
      if (!stricmp(curr_word, "NEAR")) return curr_sym=FTS_NEAR;  break;
  }
  return curr_sym=FTS_WORD;
}

void t_ft_analyser::ft_anal_term(t_ft_node ** fte, BOOL negate)
{ while (curr_sym==FTS_NOT) 
    { next_sym();  negate=!negate; }
  if (curr_sym==FTS_LPAR)
  { next_sym();
    ft_anal_or(fte, negate);
    if (curr_sym==FTS_RPAR) next_sym();  // no error if ) forgotten
  }
  else // read word or words
  { BOOL inphrase, caseequal;
    if (curr_sym==FTS_PHRASE) { inphrase=TRUE;  next_sym(); }
    else inphrase=FALSE;
    if (curr_sym==FTS_EQUAL) { caseequal=TRUE;  next_sym(); }
    else caseequal=FALSE;
    *fte=NULL;
    if (curr_sym!=FTS_WORD) return;
    int skipped=0;
   // cycle on the phrase, *fte is the current destination for the next word node:
    do
    {// lemmatize the word and check for simpicity: 
      char lword[MAX_FT_WORD_LEN+1];  int flags;  // ### use prepare_word() here!
#ifdef LEMMATIZE_ORIG_CASE
      if (ftk->basic_form) lemmatize(ftk->language, curr_word, lword, MAX_FT_WORD_LEN+1);  else strcpy(lword, curr_word);
      flags = ftk->ft_upcase(lword);
#else
      char uword[MAX_FT_WORD_LEN+1];
      strcpy(uword, curr_word);
      flags = ftk->ft_upcase(uword);
      if (ftk->basic_form) lemmatize(ftk->language, uword, lword, MAX_FT_WORD_LEN+1);  else strcpy(lword, uword);
#endif
      if (!simple_word(lword, ftk->language)) 
      { t_ft_node_word * wrd = 
#if WBVERS>=950
            ftk->separated ?
          (t_ft_node_word *)new t_ft_node_word_separated(negate ? FT_NEGWORD : FT_WORD, (t_ft_kontext_separated*)ftk, caseequal ? flags : -1) :
#endif
          (t_ft_node_word *)new t_ft_node_word_intern   (negate ? FT_NEGWORD : FT_WORD, (t_ft_kontext_intern   *)ftk, caseequal ? flags : -1);
        if (!wrd) { error=TRUE;  return; }
        *fte=wrd;  fte=(t_ft_node**)&wrd->next;
        t_wordkey wordkey(lword);  
#if WBVERS>=950
        if (ftk->separated)
        { t_ft_kontext_separated * ftks = (t_ft_kontext_separated *)ftk;
          t_reader mrsw_reader(&ftks->eif.mrsw_lock, cdp);
          ftk->ft_find_word(cdp, &wordkey, &wrd->wordid);
        }
        else
#endif
          ftk->ft_find_word(cdp, &wordkey, &wrd->wordid);
        wrd->distance = inphrase ? skipped+1 : DEFAULT_NEAR_DISTANCE;
        skipped=0;
      }
      else 
        skipped++;
      next_sym();
      if (inphrase && curr_sym==FTS_PHRASE) { next_sym();  break; }
      if (curr_sym==FTS_NEAR) next_sym();
    } while (curr_sym==FTS_WORD);
  }
}

void t_ft_analyser::ft_anal_and(t_ft_node ** fte, BOOL negate)
{ ft_anal_term(fte, negate);
  while (curr_sym==FTS_AND)
  { t_ft_node * second;
    next_sym();
    ft_anal_term(&second, negate);
    if (second)
      if (*fte)
      { t_ft_node_andor * top = new t_ft_node_andor(negate ? FT_OR : FT_AND);
        if (top==NULL) { error=TRUE;  return; }
        top->left=*fte;  top->right=second;  *fte=top;
      }
      else
        *fte=second;
  }
}

void t_ft_analyser::ft_anal_or(t_ft_node ** fte, BOOL negate)
{ ft_anal_and(fte, negate);
  while (curr_sym==FTS_OR)
  { t_ft_node * second;
    next_sym();
    ft_anal_and(&second, negate);
    if (second)
      if (*fte)
      { t_ft_node_andor * top = new t_ft_node_andor(negate ? FT_AND : FT_OR);
        if (top==NULL) { error=TRUE;  return; }
        top->left=*fte;  top->right=second;  *fte=top;
      }
      else
        *fte=second;
  }
}

BOOL analyse_fulltext_expression(cdp_t cdp, const char * ft_expr, t_ft_node ** fte, t_ft_kontext * ftk, t_specif query_specif)
{ *fte=NULL;  char * query;
  if (ftk->basic_form) 
    if (!init_lemma(ftk->language))
      { SET_ERR_MSG(cdp, lpszMainDictName[ftk->language]);  request_error(cdp, LANG_SUPP_NOT_AVAIL);  return FALSE; }
  if (ftk->wordtab_open(cdp))
  {// convert query to the wordtab charset:
    if (ftk->is_v9_system && query_specif.charset!=ftk->word_specif.charset)
    { query = (char*)corealloc(strlen(ft_expr)+1, 71);
      if (!query) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
      superconv(ATT_STRING, ATT_STRING, ft_expr, query, query_specif, ftk->word_specif, NULL);
      ft_expr=query;
    }
     // analyse query:
    t_ft_analyser anal(cdp, ft_expr, ftk);
    anal.ft_anal_or(fte, FALSE);  // must be before corefree(query);
    if (ftk->is_v9_system && query_specif.charset!=ftk->word_specif.charset)
      corefree(query);
    ftk->wordtab_close();
    if (anal.error) 
    { if (*fte) { delete *fte;  *fte=NULL; }
      request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; 
    }
  }
  return TRUE;
}

void count_words(cdp_t cdp, const char * ft_label, const char * ft_expr, int * words_total, int * words_valid)
// ft_expr is supposed to be in the wordtab specif.
{
  *words_total=*words_valid=0;
  t_ft_kontext * ftk = get_fulltext_kontext(cdp, ft_label, NULL, NULL);
  t_ft_analyser anal(cdp, ft_expr, ftk);
  while (anal.curr_sym!=FTS_END)
  { if (anal.curr_sym==FTS_WORD)
    {// lemmatize the word and check for simpicity: 
      char lword[MAX_FT_WORD_LEN+1];  int flags;  // ### use prepare_word() here!
#ifdef LEMMATIZE_ORIG_CASE
      if (ftk->basic_form) lemmatize(ftk->language, anal.curr_word, lword, MAX_FT_WORD_LEN+1);  else strcpy(lword, curr_word);
      flags = ftk->ft_upcase(lword);
#else
      char uword[MAX_FT_WORD_LEN+1];
      strcpy(uword, anal.curr_word);
      flags = ftk->ft_upcase(uword);
      if (ftk->basic_form) lemmatize(ftk->language, uword, lword, MAX_FT_WORD_LEN+1);  else strcpy(lword, uword);
#endif
      if (!simple_word(lword, ftk->language)) (*words_valid)++;
      (*words_total)++;
      anal.next_sym();
    } 
  }
}
//////////////////////////////////// passing ///////////////////////////////////////////
#define REF_FLAGS_ATTR   4
#define WEIGHT_FLAG_MEDIUM  4
#define WEIGHT_FLAG_MAX     8
#define WEIGHT_KOEFICIET_MEDIUM 10
#define WEIGHT_KOEFICIET_MAX    100

#if WBVERS>=950
cbnode_oper_wi  word_index_oper;
cbnode_oper_ri1_32 ref1_index_oper_32;
cbnode_oper_ri2_32 ref2_index_oper_32;
cbnode_oper_ri1_64 ref1_index_oper_64;
cbnode_oper_ri2_64 ref2_index_oper_64;
#endif

unsigned t_ft_node_word_intern :: calc_weight(cdp_t cdp, trecnum recnum)
{ if (flags==-1 && !fkt->weighted) return current_weight=1;  // no conditions on flags
  char stored_flags;
  tb_read_atr(cdp, ftki->ref_td->me(), recnum, REF_FLAGS_ATTR, &stored_flags);
  if (flags!=-1 && (stored_flags & (FTFLAG_FIRSTUPPER | FTFLAG_ALLUPPER)) != flags) return current_weight=0;
  current_weight = 1;
  if (fkt->weighted) 
  { if (stored_flags & WEIGHT_FLAG_MEDIUM) current_weight += WEIGHT_KOEFICIET_MEDIUM;
    if (stored_flags & WEIGHT_FLAG_MAX)    current_weight += WEIGHT_KOEFICIET_MAX;
  }
  return current_weight;
}

#if WBVERS>=950
unsigned t_ft_node_word_separated :: calc_weight(cdp_t cdp, trecnum recnum)
// Calculates and returns [current_weight]: 0 if flags are specified and do not match, 1 if not wighted, more if stored flas say it.
{ if (flags==-1 && !fkt->weighted) return current_weight=1;  // no conditions on flags
  char stored_flags;
  if (fkt->bigint_id)
    stored_flags = *(char*)path.clustered_value_ptr(ref1_index_oper_64);
  else
    stored_flags = *(char*)path.clustered_value_ptr(ref1_index_oper_32);
  if (flags!=-1 && (stored_flags & (FTFLAG_FIRSTUPPER | FTFLAG_ALLUPPER)) != flags) return current_weight=0;
  current_weight = 1;
  if (fkt->weighted) 
  { if (stored_flags & WEIGHT_FLAG_MEDIUM) current_weight += WEIGHT_KOEFICIET_MEDIUM;
    if (stored_flags & WEIGHT_FLAG_MAX)    current_weight += WEIGHT_KOEFICIET_MAX;
  }
  return current_weight;
}
#endif

void t_ft_node_word_intern :: open_index(cdp_t cdp, BOOL initial)
// close_index() must be called even if open_index is not successfull (the root may be locked).
{ 
  cdp->index_owner = ftki->ref_td->tbnum;
  if (ftki->bigint_id)
  { if (initial)
    { curr_key.wordid=wordid;
      curr_key.docid=NONEBIGINT;  // smallest value
      curr_key.position=0;        // smallest value
      curr_key.recnum=0;          // smallest value
      documents_exhausted=false;  // must not do this when re-openning, documents_exhausted may have been true before!
    }
    if (wordid==NO_WORD_ID) 
      documents_exhausted=true;
    else if (!btrl.lock_and_update(cdp, ftki->refindx->root))
      documents_exhausted=true;
    else if (bac.build_stack(cdp, ftki->refindx->root, ftki->refindx, (const char *)&curr_key)==BT_ERROR) 
      documents_exhausted=true;
    else
    { bac.btree_step(cdp, (tptr)&curr_key);
      if (curr_key.wordid!=wordid) 
        documents_exhausted=true;
    }
  }
  else
  { if (initial)
    { curr_key32.wordid=wordid;
      curr_key32.docid=NONEINTEGER; // smallest value
      curr_key32.position=0;        // smallest value
      curr_key32.recnum=0;          // smallest value
      documents_exhausted=false;  // must not do this when re-openning, documents_exhausted may have been true before!
    }
    if (wordid==NO_WORD_ID) 
      documents_exhausted=true;
    else if (!btrl.lock_and_update(cdp, ftki->refindx->root))
      documents_exhausted=true;
    else if (bac.build_stack(cdp, ftki->refindx->root, ftki->refindx, (const char *)&curr_key32)==BT_ERROR) 
      documents_exhausted=true;
    else
    { bac.btree_step(cdp, (tptr)&curr_key32);
      if (curr_key32.wordid!=wordid) 
        documents_exhausted=true;
    }
    define_curr_key();
  }
 // continuation of the phrase:
  if (next) next->open_index(cdp, initial);
}

#if WBVERS>=950
void t_ft_node_word_separated :: open_index(cdp_t cdp, BOOL initial)
{ 
  if (ftks->bigint_id)
  { if (initial)
    { curr_key.wordid=wordid;
      curr_key.docid=NONEBIGINT; // smallest value
      curr_key.position=0;        // smallest value
      curr_key.recnum=0;          // smallest value
      documents_exhausted=false;  // must not do this when re-openning, documents_exhausted may heve been true before!
    }
    if (wordid==NO_WORD_ID) 
      documents_exhausted=true;
    else 
    { ftks->eif.mrsw_lock.reader_entrance(cdp, WAIT_EXTFT);  index_read_locked=true;
      cb_result res = cb_build_stack(ftks->eif, ref1_index_oper_64, REF1_INDEX_ROOT_NUM, &curr_key, path);
      if (res==CB_ERROR) 
        documents_exhausted=true;
      else if (res!=CB_FOUND_EXACT)
      { if (!cb_step(ftks->eif, ref1_index_oper_64, &curr_key, path) || curr_key.wordid!=wordid) 
          documents_exhausted=true;
      }
    }
  }
  else
  { if (initial)
    { curr_key32.wordid=wordid;
      curr_key32.docid=NONEINTEGER; // smallest value
      curr_key32.position=0;        // smallest value
      curr_key32.recnum=0;          // smallest value
      documents_exhausted=false;  // must not do this when re-openning, documents_exhausted may heve been true before!
    }
    if (wordid==NO_WORD_ID) 
      documents_exhausted=true;
    else 
    { ftks->eif.mrsw_lock.reader_entrance(cdp, WAIT_EXTFT);  index_read_locked=true;
      cb_result res = cb_build_stack(ftks->eif, ref1_index_oper_32, REF1_INDEX_ROOT_NUM, &curr_key32, path);
      if (res==CB_ERROR) 
        documents_exhausted=true;
      else if (res!=CB_FOUND_EXACT)
      { if (!cb_step(ftks->eif, ref1_index_oper_32, &curr_key32, path) || curr_key32.wordid!=wordid) 
          documents_exhausted=true;
      }
    }
    define_curr_key();
  }
 // continuation of the phrase:
  if (next) next->open_index(cdp, initial);
}
#endif

unsigned t_ft_node_word :: satisfied(t_docid * docid, unsigned * position) const
{ *docid=current_docid;
  *position=current_position;
  if (current_docid==NO_DOC_ID) return node_type==FT_WORD ? 0 : 1;
  return node_type==FT_WORD ? current_weight : 0;  // 0 for FT_NEGWORD
}

bool t_ft_node_word:: doc_step(cdp_t cdp, t_docid * docid)
// Moves to docid or bigger, returns the docid found, returns FALSE if no more documents for the word
{ if (!documents_exhausted)
    while (curr_key.docid < *docid || !calc_weight(cdp, curr_key.recnum))
      if (!istep(cdp)) 
      { documents_exhausted=true;
        break;
      }
  if (documents_exhausted)
  { close_path(cdp);
    *docid = NO_DOC_ID;
    return false;
  }
  else
  { *docid = curr_key.docid;
    return true;
  }
}

bool t_ft_node_word:: next_pos(cdp_t cdp)
// Moves to the next word position in the current document, returns FALSE if not found
{ if (documents_exhausted) return FALSE;
  t_docid start_doc = curr_key.docid;
  do
  { if (!istep(cdp)) 
    { documents_exhausted=true;
      close_path(cdp);
      return false;
    }
   // check the end of the interval (different doc):
    if (curr_key.docid!=start_doc) return FALSE;
  } while (!calc_weight(cdp, curr_key.recnum));
  return true;
}

BOOL inphrase(unsigned pos1, unsigned pos2, int distance)
{ if (!distance) return pos2==pos1+1;
  if (distance!=DEFAULT_NEAR_DISTANCE)
    return pos1 < pos2 ? pos2==pos1+distance : pos1==pos2+distance;
  return pos1 < pos2 ? pos2<=pos1+distance : pos1<=pos2+distance;
}

bool t_ft_node_word_intern :: istep(cdp_t cdp)
// Goes to the next occurrence of the same word [wordid]. Loads [curr_key] for it. Returns false if it does not exist.
{ cdp->index_owner = ftki->ref_td->tbnum;
  if (ftki->bigint_id)
  { if (!bac.btree_step(cdp, (tptr)&curr_key)) return false;
  }
  else
  { if (!bac.btree_step(cdp, (tptr)&curr_key32)) return false;
    define_curr_key();
  }
  return curr_key.wordid==wordid; 
}

void t_ft_node_word_intern :: close_path(cdp_t cdp)
{ 
  bac.close(cdp); 
  btrl.unlock();
}

#if WBVERS>=950
bool t_ft_node_word_separated :: istep(cdp_t cdp)
// Goes to the next occurrence of the same word [wordid]. Loads [curr_key] for it. Returns false if it does not exist.
{ 
  if (ftks->bigint_id)
  { if (!cb_step(ftks->eif, ref1_index_oper_64, &curr_key, path)) return false;
  }
  else
  { if (!cb_step(ftks->eif, ref1_index_oper_32, &curr_key32, path)) return false;
    define_curr_key();
  }
  return curr_key.wordid==wordid; 
}

void t_ft_node_word_separated :: close_path(cdp_t cdp)
{ path.close(); 
  if (index_read_locked)
    { ftks->eif.mrsw_lock.reader_exit();  index_read_locked=false; }
}
#endif

void t_ft_node_word :: next_doc(cdp_t cdp, t_docid min_docid)
// Positions on the next doc containing the phrase if the current doc is min_docid 
// When min_docid == NO_DOC_ID, positions on the first document containing the phrase
// If the next document is not found, [current_docid]==NO_DOC_ID on exit.

{// works only when min_docid==current_docid or when it is the initial call with min_docid == NO_DOC_ID:
  if (min_docid!=current_docid && min_docid!=NO_DOC_ID) return;
  if (current_docid==NO_DOC_ID) return;  // word exhausted in the past
  //if (documents_exhausted)               // word exhausted now
  //  { current_docid=NO_DOC_ID;  return; }

  unsigned total_weight;
  if (next==NULL)  // single word: adds all occurences into total_weight
  { do // cycle on documents until nonzero weight
    {// new positioning:
      if (documents_exhausted)               // word exhausted now
        { current_docid=NO_DOC_ID;  return; }
      current_docid    = curr_key.docid;
      current_position = curr_key.position;
     // count the weight of the word in the [current_docid]:
      total_weight=0;
      do // cycle on occurrences in the document
      { total_weight+=calc_weight(cdp, curr_key.recnum);
        if (!istep(cdp)) 
          { documents_exhausted=true;  break; }
      } while (current_docid==curr_key.docid);
     // stops on the new document or the end of documents, now the [total_weight] is computed for [current_docid]
    } while (!total_weight);
    current_weight=total_weight;
  }
  else  // phrase: must not count the total weight, must match the occurences
  { t_docid common_docid;
    do // cycle on documents until nonzero weight
    {// find common document into curr_key.docid and common_docid:
      t_docid start_docid;
      common_docid=0;  
      do
      { doc_step(cdp, &common_docid);  start_docid=common_docid;
        if (common_docid==NO_DOC_ID) goto ex;  // word exhausted
        for (t_ft_node_word * wrd = next;  wrd;  wrd=wrd->next)
          wrd->doc_step(cdp, &common_docid);
        if (common_docid==NO_DOC_ID) goto ex;  // word exhausted
      } while (start_docid!=common_docid);
     // search for the exact phrase occurences in the common doc:
      BOOL phrase_ok, contin=TRUE;
      total_weight=0;
      do // cycle on positions in the common document
      { phrase_ok=TRUE;
       // check the phrase occurence in the current position:
        for (t_ft_node_word * wrd = this;  wrd->next!=NULL;  wrd=wrd->next)
          if (!inphrase(wrd->curr_key.position, wrd->next->curr_key.position, wrd->next->distance))
          { if (wrd->curr_key.position < wrd->next->curr_key.position)
                 contin=wrd->      next_pos(cdp);
            else contin=wrd->next->next_pos(cdp);
            phrase_ok=FALSE;
            break;
          }
        if (phrase_ok) 
        { current_position=curr_key.position;
          total_weight+=current_weight;  // weight of the phrase in the weight of its 1st word
         // move all positions
          for (t_ft_node_word * wrd = this;  wrd!=NULL;  wrd=wrd->next)
            if (!wrd->next_pos(cdp))
              { contin=FALSE;  break; }
        }
      } while (contin);
     // now at lease one word of the phrase is out of the document
    } while (!total_weight); // total_weight>0 iff phrase found in the common document
    current_weight=total_weight;
   ex:
    current_docid=common_docid;
  }
}

unsigned t_ft_node_andor :: satisfied(t_docid * docid, unsigned * position) const
{ t_docid docid1, docid2;  unsigned pos1, pos2;
  unsigned sat1 = left->satisfied(&docid1, &pos1), sat2 = right->satisfied(&docid2, &pos2);
  *docid = docid1 < docid2 ? docid1 : docid2;  // NO_DOC_ID is greather than any doc_id
  if (node_type==FT_AND) 
  { if (docid1 == docid2) 
    { *docid=docid1;
      *position=pos1<pos2 ? pos1 : pos2;
      return sat1 < sat2 ? sat1 : sat2;
    }
    if (left->node_type==FT_NEGWORD && docid1>docid2) // 1 is negative word
    { *position=pos2;
      return sat2; 
    }
    if (right->node_type==FT_NEGWORD && docid1 < docid2) // 2 is negative word
    { *position=pos1;
      return sat1; 
    }
    return 0;  // not satisfied
  }
  else  // OR
  { if (docid1 == docid2)
    { *docid=docid1;  *position=pos1;
      return sat1 > sat2 ? sat1 : sat2;
    }
    if (docid1 <  docid2) 
    { *docid=docid1;  *position=pos1;
      return sat1;
    }
    else   // docid1 > docid2;
    { *docid=docid2;  *position=pos2;
      return sat2;
    }
  }
}

//////////////////////////////////////// fulltext operations ///////////////////////////////////////////
BOOL t_ft_kontext_intern::wordtab_open(cdp_t cdp)
{ word_tabdescr=install_table(cdp, wordtabnum);
  return word_tabdescr!=NULL;
}

void t_ft_kontext::mark_core(void)
{ mark(this);
  if (limits_value!=NULL ) mark(limits_value); 
  if (word_starters!=NULL) mark(word_starters); 
  if (word_insiders!=NULL) mark(word_insiders); 
}

void t_ft_kontext_intern::mark_core(void)
{ t_ft_kontext::mark_core();
  if (rem_doc_ref_so) rem_doc_ref_so->mark_core();  // marks the next_stmt too
}

#if WBVERS>=950
void t_ft_kontext_separated::mark_core(void)
{ t_ft_kontext::mark_core(); }
#endif

void t_ft_kontext_intern::wordtab_close(void)  // Supposes wordtab to be installed.
{ unlock_tabdescr(word_tabdescr); }

BOOL t_ft_kontext_intern::ft_find_word(cdp_t cdp, t_wordkey * wordkey, t_wordid * wordid) 
// Looks for the word ID in the word table using the index. Supposes wordtab to be installed.
{ trecnum recnum;
  dd_index * ind = &word_tabdescr->indxs[0];
  cdp->index_owner = word_tabdescr->tbnum;
  if (!find_key(cdp, ind->root, ind, (tptr)wordkey, &recnum, true))
    { *wordid=NO_WORD_ID;  return FALSE; }
  tb_read_atr(cdp, word_tabdescr, recnum, 2, (tptr)wordid);
  return TRUE;
}

#if WBVERS>=950
BOOL t_ft_kontext_separated::ft_find_word(cdp_t cdp, t_wordkey * wordkey, t_wordid * wordid)
// Looks for the word ID in the word table using the index. 
// Called with the read or write lock.
{ cbnode_path path;
  cb_result res = cb_build_stack(eif, word_index_oper, WORD_INDEX_ROOT_NUM, wordkey, path);
  if (res!=CB_FOUND_EXACT) 
    { *wordid=NO_WORD_ID;  return FALSE; }
  *wordid = *(t_wordid*)path.clustered_value_ptr(word_index_oper);
  return TRUE;
}
#endif

static const t_colval wordtab_colvals[2+1] = { { 1, NULL, NULL, NULL, NULL, 0, NULL }, 
                                               { 2, NULL, DATAPTR_DEFAULT, NULL, NULL, 0, NULL }, 
                                               { NOATTRIB, NULL, NULL, NULL, NULL, 0, NULL } };  
static const t_colval reftab_colvals32[4+1]= { { 1, NULL, (void*)offsetof(t_ft_key32, wordid  ), NULL, NULL, 0, NULL }, 
                                               { 2, NULL, (void*)offsetof(t_ft_key32, docid   ), NULL, NULL, 0, NULL }, 
                                               { 3, NULL, (void*)offsetof(t_ft_key32, position), NULL, NULL, 0, NULL }, 
                                               { REF_FLAGS_ATTR, NULL, (void*)offsetof(t_ft_key32, recnum),   NULL, NULL, 0, NULL },  // flags in recnum
                                               { NOATTRIB, NULL, NULL, NULL, NULL, 0, NULL } };  
static const t_colval reftab_colvals64[4+1]= { { 1, NULL, (void*)offsetof(t_ft_key64, wordid  ), NULL, NULL, 0, NULL }, 
                                               { 2, NULL, (void*)offsetof(t_ft_key64, docid   ), NULL, NULL, 0, NULL }, 
                                               { 3, NULL, (void*)offsetof(t_ft_key64, position), NULL, NULL, 0, NULL }, 
                                               { REF_FLAGS_ATTR, NULL, (void*)offsetof(t_ft_key64, recnum),   NULL, NULL, 0, NULL },  // flags in recnum
                                               { NOATTRIB, NULL, NULL, NULL, NULL, 0, NULL } };  
static const t_colval docpart_colvals32[3+1]={ { 1, NULL, (void*)offsetof(t_docpart_rec32, docid   ), NULL, NULL, 0, NULL }, 
                                               { 2, NULL, (void*)offsetof(t_docpart_rec32, position), NULL, NULL, 0, NULL }, 
                                               { 3, NULL, (void*)offsetof(t_docpart_rec32, rel_path), NULL, NULL, 0, NULL }, 
                                               { NOATTRIB, NULL, NULL, NULL, NULL, 0, NULL } };  
static const t_colval docpart_colvals64[3+1]={ { 1, NULL, (void*)offsetof(t_docpart_rec64, docid   ), NULL, NULL, 0, NULL }, 
                                               { 2, NULL, (void*)offsetof(t_docpart_rec64, position), NULL, NULL, 0, NULL }, 
                                               { 3, NULL, (void*)offsetof(t_docpart_rec64, rel_path), NULL, NULL, 0, NULL }, 
                                               { NOATTRIB, NULL, NULL, NULL, NULL, 0, NULL } };  

int t_ft_kontext::prepare_word(const char * wordIn, char * wordOut, t_specif specif, unsigned weight)
{ int flags;
 // convert the encoding:
  if (is_v9_system)
  { char uword[MAX_FT_WORD_LEN+1];
    if (specif.charset!=word_specif.charset)
      superconv(ATT_STRING, ATT_STRING, wordIn, uword, specif, word_specif, NULL);
    else
      strcpy(uword, wordIn);
    flags = ft_upcase(uword);
    if (basic_form) lemmatize(language, uword, wordOut, MAX_FT_WORD_LEN+1);
    else strcpy(wordOut, uword);
  }
  else
  {   
#ifdef LEMMATIZE_ORIG_CASE
    if (basic_form) lemmatize(language, wordIn, wordOut, MAX_FT_WORD_LEN+1);  else strcpy(wordOut, wordIn);
    flags = ft_upcase(wordOut);
#else
    char uword[MAX_FT_WORD_LEN+1];
    strcpy(uword, wordIn);
    flags = ft_upcase(uword);
    if (basic_form) lemmatize(language, uword, wordOut, MAX_FT_WORD_LEN+1);  else strcpy(wordOut, uword);
#endif
  }
  if (weight) flags |= weight>1 ? WEIGHT_FLAG_MAX : WEIGHT_FLAG_MEDIUM;
  return flags;
}

#if WBVERS>=950

bool check_cb(ext_index_file & eif, cbnode_oper & oper, cbnode_number root)
{ t_wordkey wordkey(NULLSTRING);
  t_wordkey lastword(NULLSTRING);
  cbnode_path path;  bool error=false;
  cb_result res = cb_build_stack(eif, oper, root, &wordkey, path);
  if (res!=CB_ERROR) 
    while (cb_step(eif, word_index_oper, &wordkey, path))
      if (oper.cmp_keys(wordkey.word, lastword.word) < 0)
        { error=true;  break; }
      else strcpy(lastword.word, wordkey.word);
  return error;
}

static int sleep_counter = 0;  // common counter for all indexing threads

BOOL t_ft_kontext_separated::ft_insert_word_occ(cdp_t cdp, t_docid docid, const char * word, unsigned position, unsigned weight, t_specif specif) 
{ t_wordid wordid;  char lword[MAX_FT_WORD_LEN+1];  bool error=false; 
  int flags = prepare_word(word, lword, specif, weight);
  if (simple_word(lword, language)) return FALSE;
  if (!scratch_sorter) eif.mrsw_lock.writer_entrance(cdp, WAIT_EXTFT);
 // search for lword in the word index:
  { t_wordkey wordkey(lword);
    if (!ft_find_word(cdp, &wordkey, &wordid))
    {// allocate the new wordid value:
      wordid = eif.get_counter_value();
     // insert the word:
      if (!cb_insert_val(eif, word_index_oper, WORD_INDEX_ROOT_NUM, lword, &wordid))
        error=true;
//    if (!(wordid % 512)) 
//      if (!check_cb(eif, word_index_oper, WORD_INDEX_ROOT_NUM))
//        wordid=wordid;
    }
  }
 // wordid defined, insert to reftable:
  if (!error)
  { if (bigint_id)
    { t_ft_key64 refkey;  refkey.wordid=wordid;  refkey.docid=docid;  refkey.position=position;  
      if (scratch_sorter)
        scratch_sorter->add_key(&refkey, &flags);
      else
      { if (!cb_insert_val(eif, ref1_index_oper_64, REF1_INDEX_ROOT_NUM, &refkey, &flags))
          error=true;
        else if (!cb_insert_val(eif, ref2_index_oper_64, REF2_INDEX_ROOT_NUM, &docid, &wordid))
          error=true;
      }
    }
    else
    { t_ft_key32 refkey;  refkey.wordid=wordid;  refkey.docid=(uns32)docid;  refkey.position=position;  
      if (scratch_sorter)
        scratch_sorter->add_key(&refkey, &flags);
      else
      { if (!cb_insert_val(eif, ref1_index_oper_32, REF1_INDEX_ROOT_NUM, &refkey, &flags))
          error=true;
        else if (!cb_insert_val(eif, ref2_index_oper_32, REF2_INDEX_ROOT_NUM, &docid, &wordid))
          error=true;
      }
    }
  }
  if (!scratch_sorter) 
  { eif.mrsw_lock.writer_exit();
    if (++sleep_counter >= 300)
    { Sleep(1);  // Sleep(0) does not have any effect
      sleep_counter=0;
#if 0
     // experiment:
      { char fname[MAX_PATH];  
        get_fulltext_file_name(cdp, fname, schemaname, suffix);
        eif.close();
        eif.open(fname);  // re-open
      }
#endif
    }
  }
  return !error;
}
#endif

BOOL t_ft_kontext_intern::ft_insert_word_occ(cdp_t cdp, t_docid docid, const char * word, unsigned position, unsigned weight, t_specif specif) 
// Supposes wordtab to be installed.
{ t_wordid wordid;  char lword[MAX_FT_WORD_LEN+1];  
  int flags = prepare_word(word, lword, specif, weight);
  if (simple_word(lword, language)) return FALSE;
 // find the key in the wordtab:
  if (!wordtab_open(cdp)) return FALSE;
  t_wordkey wordkey(lword);
  if (!ft_find_word(cdp, &wordkey, &wordid))
  { //dd_index * ind = &word_tabdescr->constr[0].index;
    //bt_insert(cdp, (tptr)&wordkey, ind, ind->root, word_tabdescr->selfname, translog);
    t_vector_descr wordtab_vector(wordtab_colvals, wordkey.word);
    wordkey.recnum=0;
    trecnum recnum = tb_new(cdp, word_tabdescr, FALSE, &wordtab_vector);
    if (recnum==NORECNUM) { wordtab_close();  return FALSE; }
    tb_read_atr(cdp, word_tabdescr, recnum, 2, (tptr)&wordid);
  }
  wordtab_close();
 // wordid defined, insert to reftable:
  if (bigint_id)
  { t_ft_key64 refkey;  refkey.wordid=wordid;  refkey.docid=docid;  refkey.position=position;  refkey.recnum=flags;
    t_vector_descr reftab_vector(reftab_colvals64, &refkey);
    return !tb_new(cdp, ref_td->me(), FALSE, &reftab_vector);
  }
  else
  { t_ft_key32 refkey;  refkey.wordid=wordid;  refkey.docid=(uns32)docid;  refkey.position=position;  refkey.recnum=flags;
    t_vector_descr reftab_vector(reftab_colvals32, &refkey);
    return !tb_new(cdp, ref_td->me(), FALSE, &reftab_vector);
  }
}

void t_ft_kontext::ft_starting_document_part(cdp_t cdp, t_docid docid, unsigned position, const char * relative_path)
// Stores information about the starting position of the document part in the docpart table.
{
  if (docpartnum!=NOOBJECT)
  { table_descr_auto docpart_td(cdp, docpartnum);
    if (docpart_td->me())
    { if (bigint_id)
      { t_docpart_rec64 rec;//(docid, position, relative_path);
        rec.docid=docid;  rec.position=position;
        strmaxcpy(rec.rel_path, relative_path, sizeof(rec.rel_path));
        t_vector_descr docpart_vector(docpart_colvals64, &rec);
        tb_new(cdp, docpart_td->me(), FALSE, &docpart_vector);
      }
      else
      { t_docpart_rec32 rec;//(docid, position, relative_path);
        rec.docid=(uns32)docid;  rec.position=position;
        strmaxcpy(rec.rel_path, relative_path, sizeof(rec.rel_path));
        t_vector_descr docpart_vector(docpart_colvals32, &rec);
        tb_new(cdp, docpart_td->me(), FALSE, &docpart_vector);
      }
    }
  }
}

void t_ft_kontext::ft_set_summary_item(cdp_t cdp, t_docid docid, unsigned infotype, const void * info)
// Writes a summary information from the scanned document do the document record
{ 
  char comm[330], buf[15];
  if (doctabnum==NOTBNUM) return;  // the document table does not have its standard name, cannot write the summary info
  const char * infotype_name = NULL;
  BOOL is_date = FALSE;
  switch (infotype)
  { case PID_TITLE:        infotype_name = "TITLETEXT";   break;
    case PID_SUBJECT:      infotype_name = "SUBJECT";     break;
    case PID_LASTAUTHOR:
    case PID_AUTHOR:       infotype_name = "ODMAUTHOR";   break; 
    //case PID_LASTAUTHOR:   infotype_name = "ODMAUTHOR";   break;  // the original autor, in fact (non-editable)
    case PID_KEYWORDS:     infotype_name = "KEYWORDS";    break;
    case PID_REVNUMBER:    infotype_name = "DOCVERSION";  int2str(*(const int*)info, buf);  info=buf;  break;
    case PID_COMMENTS:     infotype_name = "COMMENT";    break;
    //case PID_EDITTIME:     infotype_name = "MODIFYDATE";   is_date=TRUE;  break;
    case PID_CREATE_DTM_RO:infotype_name = "CREATEDDATE";  is_date=TRUE;  break;
    case PID_LASTSAVE_DTM: infotype_name = "MODIFYDATE";   is_date=TRUE;  break;
  }
  if (!infotype_name) return;  // this summery info type is ignored
  sprintf(comm, "UPDATE `%s`.`%s` SET %s=", schemaname, doctabname, infotype_name);
  if (is_date)
#ifdef WINS
  { FILETIME LocalFileTime;  SYSTEMTIME SystemTime;
    FileTimeToLocalFileTime((const FILETIME *)info, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SystemTime);
    sprintf(comm+strlen(comm), "TIMESTAMP\'%04u-%02u-%02u %02u:%02u:%02u\'", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
  }
#else  
    return;  // currently not supported on Linux, must add the conversion function
#endif
  else
  { int pos = (int)strlen(comm);
    comm[pos++] = '\'';
    const char *  p = (const char *)info;
    while (*p)
    { comm[pos++] = *p;
      if (*p=='\"' || *p=='\'') comm[pos++] = *p;
      p++;
      if (pos > sizeof(comm)-30) break;
    }
    comm[pos++] = '\'';
    comm[pos] = 0;
  }
  strcat(comm, " WHERE ID=");
  int64tostr(docid, comm+strlen(comm));  
  if (infotype==PID_LASTAUTHOR)
    sprintf(comm+strlen(comm), " AND %s IS NULL", infotype_name);
 // execute the update:
  uns32 old_flags = cdp->ker_flags;
  cdp->ker_flags |= KFL_HIDE_ERROR;
  wbbool roll_back_error = cdp->roll_back_error;  int return_code = cdp->get_return_code();
  sql_statement * so = sql_submitted_comp(cdp, comm);
  if (so!=NULL) 
  { so->exec(cdp);
    delete so;
  }
  cdp->ker_flags = old_flags;
  cdp->roll_back_error = roll_back_error;  cdp->set_return_code(return_code);
}

#define REMOVE_DOC_BATCH 100

#if WBVERS>=950
void t_ft_kontext_separated::ft_remove_doc_occ(cdp_t cdp, t_docid docid) 
{ int counter=0;
  //t_writer mrsw_writer(&eif.mrsw_lock, cdp);
  eif.mrsw_lock.writer_entrance(cdp, WAIT_EXTFT);
  if (bigint_id)
  { t_ft_key64 refkey;  
    refkey.docid=docid;  
    do
    { t_wordid wordid;
      {// find a word for the document in index 2:
        cbnode_path path;
        cb_result res = cb_build_stack(eif, ref2_index_oper_64, REF2_INDEX_ROOT_NUM, &docid, path);
        if (res==CB_ERROR) goto errex;
        if (res!=CB_FOUND_EXACT) break;
        wordid = *(t_wordid*)path.clustered_value_ptr(ref2_index_oper_64);
       // remove the occurence from index 2:
        if (!_cb_remove_val(eif, ref2_index_oper_64, path))
          goto errex;
      }
      do
      {// find record in index 1:
        cbnode_path path;
        refkey.wordid=wordid;  refkey.position=0;  
        cb_result res = cb_build_stack_find(eif, ref1_index_oper_64, REF1_INDEX_ROOT_NUM, &refkey, path);
        if (res==CB_ERROR) goto errex;
        if (res==CB_FOUND_CLOSEST || res==CB_FOUND_EXACT) 
        { t_ft_key64 * found_key = (t_ft_key64*)path.items[path.top].ptr->key(path.items[path.top].pos, ref1_index_oper_64.leaf_step);
          if (found_key->docid==docid && found_key->wordid==wordid)
          { // remove the occurence from index 1:
            if (!_cb_remove_val(eif, ref1_index_oper_64, path))
              goto errex;
          }    
          else break;
        }
        else break;
      } while (true);
      if (counter++>=REMOVE_DOC_BATCH)
      { counter=0;
        eif.mrsw_lock.writer_exit();
        Sleep(1);
        eif.mrsw_lock.writer_entrance(cdp, WAIT_EXTFT);
      }
    } while (true);
  }
  else
  { t_ft_key32 refkey;  
    refkey.docid=(uns32)docid;  
    do
    { t_wordid wordid;
      {// find a word for the document in index 2:
        cbnode_path path;
        cb_result res = cb_build_stack(eif, ref2_index_oper_32, REF2_INDEX_ROOT_NUM, &docid, path);
        if (res==CB_ERROR) goto errex;
        if (res!=CB_FOUND_EXACT) break;
        wordid = *(t_wordid*)path.clustered_value_ptr(ref2_index_oper_32);
       // remove the occurence from index 2:
        if (!_cb_remove_val(eif, ref2_index_oper_32, path))
          goto errex;
      }
      do
      {// find record in index 1:
        cbnode_path path;
        refkey.wordid=wordid;  refkey.position=0;  
        cb_result res = cb_build_stack_find(eif, ref1_index_oper_32, REF1_INDEX_ROOT_NUM, &refkey, path);
        if (res==CB_ERROR) goto errex;
        if (res==CB_FOUND_CLOSEST || res==CB_FOUND_EXACT) 
        { t_ft_key32 * found_key = (t_ft_key32*)path.items[path.top].ptr->key(path.items[path.top].pos, ref1_index_oper_32.leaf_step);
          if (found_key->docid==(uns32)docid && found_key->wordid==wordid)
          { // remove the occurence from index 1:
            if (!_cb_remove_val(eif, ref1_index_oper_32, path))
              goto errex;
          }
          else break;
        }
        else break;
      } while (true);
      if (counter++>=REMOVE_DOC_BATCH)
      { counter=0;
        eif.mrsw_lock.writer_exit();
        Sleep(1);
        eif.mrsw_lock.writer_entrance(cdp, WAIT_EXTFT);
      }
    } while (true);
  }
 errex:
  eif.mrsw_lock.writer_exit();
}
#endif

void t_ft_kontext_intern::ft_remove_doc_occ(cdp_t cdp, t_docid docid) 
// Removes references to words from the document docid
{ 
  sql_statement * rem_doc = NULL;  t_query_expr * qe, * qe2 = NULL;
 // the DELETE statement is stored and reused after 1st use, but it is not reentrant: 
  { ProtectedByCriticalSection cs(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);  // any short-term CS
    rem_doc=rem_doc_ref_so;  rem_doc_ref_so=NULL;
  }
  if (rem_doc==NULL)
  { char cmd[4*OBJ_NAME_LEN+150];
    sprintf(cmd, "DELETE FROM `%s`.`%s` WHERE DOCID=", schemaname, ref_td->selfname);
    int64tostr(docid, cmd+strlen(cmd));  
    if (docpartnum!=NOOBJECT)
    { table_descr_auto docpart_td(cdp, docpartnum);
      if (docpart_td->me())
      { sprintf(cmd+strlen(cmd), ";DELETE FROM `%s`.`%s` WHERE DOCID=", schemaname, docpart_td->selfname);
        int64tostr(docid, cmd+strlen(cmd));  
      }
    }
    rem_doc = sql_submitted_comp(cdp, cmd);
    if (rem_doc==NULL) return;
    qe = ((sql_stmt_delete*)rem_doc)->_qe();
    if (rem_doc->next_statement)
      qe2 = ((sql_stmt_delete*)rem_doc->next_statement)->_qe();
  }
  else
  { qe = ((sql_stmt_delete*)rem_doc)->_qe();
    if (rem_doc->next_statement)
      qe2 = ((sql_stmt_delete*)rem_doc->next_statement)->_qe();
    *(uns32*)((t_expr_sconst*)((t_expr_binary*)((t_qe_specif*)qe)->where_expr)->op2)->val=(uns32)docid;
    if (qe2)
      *(uns32*)((t_expr_sconst*)((t_expr_binary*)((t_qe_specif*)qe2)->where_expr)->op2)->val=(uns32)docid;
  }
 // delete and release the old references:
  uns32 saved_sqloptions = cdp->sqloptions;
  cdp->sqloptions &= ~SQLOPT_EXPLIC_FREE;
  if (cdp->dbginfo) cdp->dbginfo->disable();  // prevent debug stop on the statement
  unsigned saved_cnt = cdp->sql_result_cnt;
  rem_doc->exec(cdp);
  cdp->sql_result_cnt = saved_cnt;
  if (rem_doc->next_statement) rem_doc->next_statement->exec(cdp);
  if (cdp->dbginfo) cdp->dbginfo->enable();
  cdp->sqloptions = saved_sqloptions;
#if SQL3
  if (qe->kont.t_mat!=NULL)
  { t_mater_table * tmat = qe->kont.t_mat;
    if (!tmat->permanent)
      tmat->destroy_temp_materialisation(cdp);
    delete qe->kont.t_mat;  
    qe->kont.t_mat=NULL; 
  }
  if (qe2 && qe2->kont.t_mat!=NULL)
  { t_mater_table * tmat = qe2->kont.t_mat;
    if (!tmat->permanent)
      tmat->destroy_temp_materialisation(cdp);
    delete qe2->kont.t_mat;  
    qe2->kont.t_mat=NULL; 
  }
#else
  if (qe->kont.mat!=NULL)
  { if (!qe->kont.mat->is_ptrs)
    { t_mater_table * tmat = (t_mater_table*)qe->kont.mat;
      if (!tmat->permanent)
        tmat->destroy_temp_materialisation(cdp);
    }
    delete qe->kont.mat;  
    qe->kont.mat=NULL; 
  }
  if (qe2 && qe2->kont.mat!=NULL)
  { if (!qe2->kont.mat->is_ptrs)
    { t_mater_table * tmat = (t_mater_table*)qe2->kont.mat;
      if (!tmat->permanent)
        tmat->destroy_temp_materialisation(cdp);
    }
    delete qe2->kont.mat;  
    qe2->kont.mat=NULL; 
  }
#endif
 // return to reusable:
  { ProtectedByCriticalSection cs(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);
    if (rem_doc_ref_so==NULL)
      { rem_doc_ref_so=rem_doc;  rem_doc=NULL; }
  }
  if (rem_doc) delete rem_doc;
}

#include "ftconv.cpp"

BOOL t_ft_kontext::ft_index_doc(cdp_t cdp, t_docid docid, t_value * text, const char * format, int mode, t_specif specif) 
// Plain text for mode 2 not supported. Does not remove the contents from the index any more.
{// init lemmatisation:
  if (basic_form) 
    if (!init_lemma(language))
      { SET_ERR_MSG(cdp, lpszMainDictName[language]);  request_error(cdp, LANG_SUPP_NOT_AVAIL);  return FALSE; }
  if (text->is_null()) return TRUE;
  t_agent_indexing agent_data(cdp, docid, this);  
 if (is_v9_system)
 {// prepare the input stream:
   t_input_stream * is;
   if (mode==0)
     if (text->vmode==mode_access)
       is = new t_input_stream_db(cdp, text->dbacc.tb, text->dbacc.rec, text->dbacc.attr, text->dbacc.index);
     else
       is = new t_input_stream_direct(cdp,text->valptr(), text->length);
   else if (mode==1)
   { text->load(cdp);
     is = new t_input_stream_file(cdp,text->valptr(),text->valptr());
   }
   else
     { SET_ERR_MSG(cdp, "<OLE>");  request_error(cdp, CONVERSION_NOT_SUPPORTED);  return FALSE; }
   if (!is) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
  // prepare the convertor:
   t_input_convertor * ic = ic_from_format(cdp,format);
   t_ft_limits_evaluator limits_evaluator(cdp, limits_value);
   if (!ic)
   { ic = ic_from_text(cdp,is);
     is->reset();
     if (!ic)
       { delete is;  SET_ERR_MSG(cdp, "<Unknown format>");  request_error(cdp, CONVERSION_NOT_SUPPORTED);  return FALSE; }
   }
   ic->set_stream(is);
   ic->set_agent(&agent_data);
   if( mode==0 ) ic->set_specif(specif);   // may not be used by some convertors, but is used in the plain text
   ic->set_depth_level(1);
   ic->set_limits_evaluator(&limits_evaluator);
   ic->set_owning_ft(this);
   bool res = ic->convert() && !is->was_any_error() && !ic->was_any_error();
   delete ic;
   delete is;
   return res;
 }
 else
 {
 // analysing the format
#ifdef STOP  // original LINUX version
  if (mode!=2) 
    if (!*format || !stricmp(format, "PLAIN") || !stricmp(format, "TEXT") || !stricmp(format, "TXT")) // plain text -> direct content or file name, but not OLE
    { text->load(cdp);  tptr dirtext=text->valptr(); 
      if (!mode) // direct
      { int length = strlen(dirtext);
        agentConvert97Callback(&agent_data, C97_INS_TEXT, (T_CC97ARG*)dirtext, *(T_CC97ARG*)&length, (LPVOID)NULL, (LPVOID)NULL);
      }
      else
      { FHANDLE hnd=CreateFile(dirtext, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (!FILE_HANDLE_VALID(hnd)) return FALSE;
        char buf[1024];  DWORD rd;
        while (ReadFile(hnd, buf, sizeof(buf), &rd, NULL) && rd) 
          agentConvert97Callback(&agent_data, C97_INS_TEXT, (T_CC97ARG*)buf, *(T_CC97ARG*)&rd, (LPVOID)NULL, (LPVOID)NULL);
        CloseFile(hnd);
      }
     // add the last unterminated word, if any:
      if (*agent_data.word)
      { agent_data.word_completed();
        *agent_data.word=0;
      }
      return TRUE;
    }
#endif

  char textfilepathname[MAX_PATH];  BOOL result=TRUE;  BOOL deltempfile;
  if (mode==1) // text is the file name
  { text->load(cdp);
    strmaxcpy(textfilepathname, text->valptr(), sizeof(textfilepathname));  
    deltempfile=FALSE;
  }
  else if (mode==2)
#if 0  //ifdef WINS  -- temp 10
  { if (!ole_initalized) { OleInitialize(NULL);  ole_initalized=wbtrue; }
    if (text->vmode==mode_access) // not loaded
    { if (TxtAttr2FileD(cdp, text->dbacc.tb, text->dbacc.rec, text->dbacc.attr, text->dbacc.index, 
                       textfilepathname, &deltempfile)!=NOERROR)
        return FALSE;
    }
    else // already in memory
    { if (TxtAttr2FileM(text->valptr(), strlen(text->valptr()), textfilepathname, &deltempfile)!=NOERROR)
        return FALSE;
    }
  }
#else // LINUX mode 2
  { SET_ERR_MSG(cdp, "Conversion");  request_error(cdp, LIBRARY_NOT_FOUND);
    return FALSE;
  }
#endif // LINUX
  else // copying the direct text to the temporary file:
  { char pathname[MAX_PATH];  GetTempPath(sizeof(pathname), pathname);
    GetTempFileName(pathname, "FTX", 0, textfilepathname);
    FHANDLE hnd=CreateFile(textfilepathname, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (!FILE_HANDLE_VALID(hnd)) return FALSE;
    text->load(cdp);  
    if (text->length <=1)  // document is empty, some versions of XLIB do not support it
      { CloseFile(hnd);  DeleteFile(textfilepathname);  return TRUE; }
    DWORD wr;
    WriteFile(hnd, text->valptr(), text->length, &wr, NULL);
    CloseFile(hnd);
    deltempfile=TRUE;
  }
#ifdef WINS
 // load the conversion routine:
  HINSTANCE hXDll;  BOOL routine_found=TRUE;  TFileType file_type = ftUnknown;  FILEERROR sError;
  if (!stricmp(format, "PLAIN") || !stricmp(format, "TEXT") || !stricmp(format, "TXT"))
    file_type=ftASCII;
  if (file_type == ftUnknown)
  { IMPORTFILE * lpFnConvert = get_conversion_routine("wb_ximp", hXDll);
    if (!lpFnConvert) routine_found=FALSE;
    else sError = (*lpFnConvert)(&agent_data, textfilepathname, agentConvert97Callback);
  }
  else
  { IMPORTFILEEX * lpFnConvertEx = get_conversion_routine_ex("wb_ximp", hXDll);
    if (!lpFnConvertEx) routine_found=FALSE;
    else sError = (*lpFnConvertEx)(&agent_data, textfilepathname, agentConvert97Callback, file_type, NULL);
  }
  if (!routine_found)
    if (!hXDll)
      { SET_ERR_MSG(cdp, "WB_XIMP");  request_error(cdp, CONVERSION_NOT_SUPPORTED);  result=FALSE;  goto ex0; }
    else
      { SET_ERR_MSG(cdp, "Entry");    request_error(cdp, CONVERSION_NOT_SUPPORTED);  result=FALSE;  goto ex1; }
  else if (sError)
  { char buf[20];  int2str(sError, buf);
        SET_ERR_MSG(cdp, buf);        request_error(cdp, CONVERSION_NOT_SUPPORTED);  result=FALSE;  
  }
 // should be done even on errors:
  if (doctabnum!=NOTBNUM)   // if the document table does not have its standard name, cannot write the info
  { if (file_type == ftUnknown)
    { ANALYZEFILE * lpFnAnal = (ANALYZEFILE*)GetProcAddress(hXDll, ANALYZEFILE_FNCNAME);
      if (lpFnAnal)
      { SFileTypeFlags SFileTypeFlag;
        file_type = (*lpFnAnal)(textfilepathname, SFileTypeFlag);
      }
    }
    char comm[200+2*OBJ_NAME_LEN];
    sprintf(comm, "UPDATE `%s`.`%s` SET FORMAT_ID=%d WHERE ID=", schemaname, doctabname, -(int)file_type);
    int64tostr(docid, comm+strlen(comm));  
   // execute the update:
    wbbool roll_back_error = cdp->roll_back_error;  int return_code = cdp->get_return_code();
    sql_statement * so = sql_submitted_comp(cdp, comm);
    if (so!=NULL) 
    { so->exec(cdp);
      delete so;
    }
    cdp->roll_back_error = roll_back_error;  cdp->set_return_code(return_code);
  }
 ex1:
  FreeLibrary(hXDll);
#else // Linux: home-made konverze pro HTML
  FILEERROR sError;
  sError=convert_HTML_file(&agent_data, textfilepathname, agentConvert97Callback);
  if (sError) SET_ERR_MSG(cdp, "HTML context");
#endif
 ex0:
  if (deltempfile) DeleteFile(textfilepathname);
  return result;
 }
}

////////////////////////////// generating word context /////////////////////////////////////
struct t_context_agent_data
{ cdp_t cdp;
  char word[MAX_FT_WORD_LEN+1];
  unsigned position, word_count;
  unsigned req_pos, skip_words, copy_words;
  const char * prefix, * suffix;
  t_value * res;
  t_context_agent_data(cdp_t cdpIn, unsigned req_posIn, unsigned word_countIn, unsigned pre_wordsIn, unsigned post_wordsIn, 
                       const char * prefixIn, const char * suffixIn, t_value * resIn) 
  { cdp=cdpIn;  *word=0;  position=0;
    req_pos=req_posIn;  word_count=word_countIn;  prefix=prefixIn;  suffix=suffixIn;
    skip_words = req_posIn > pre_wordsIn ? req_posIn - pre_wordsIn : 0;
    copy_words = req_posIn + post_wordsIn - skip_words + 1;
    res = resIn;
  }
  void word_completed(BOOL is_a_word);
  void output(const char * str);
};

void t_context_agent_data::word_completed(BOOL is_a_word)
{ if (copy_words)
    if (skip_words) 
    { if (is_a_word) skip_words--;
    }
    else if (is_a_word) // copy word
    { if (position==req_pos) output(prefix);
      output(word);
      if (position==req_pos) output(suffix);
      copy_words--;
    }
    else // copy delimiter
      output(word);
  if (is_a_word) position++;
}

void t_context_agent_data::output(const char * str)
{ int len=(int)strlen(str);
  if (res->reallocate(res->length+len))
    return;  // cannot alloc
  strcpy(res->valptr()+res->length, str);
  res->length+=len;
}

T_CC97ARG agentContext97Callback(DOC_ID lpData, CONVCMDS iMessage, T_CC97ARG argA, T_CC97ARG argB, T_CC97ARG argC, T_CC97ARG argD)
{ t_context_agent_data * agent_data = (t_context_agent_data*)lpData;
  switch (iMessage)
  {
    case C97_INS_TEXT:
    { const char * str = (LPSTR)(LPVOID)argA;
      unsigned size = (WORD)argB;  if (!size) break;
      unsigned char c;  int i;
      c=*str;  str++;  size--; 
      i=0;
     // check for continuation of the previous word:
      if (*agent_data->word)
        if (FT_WORD_CONTIN(c))
          i=(int)strlen(agent_data->word);
        else
          agent_data->word_completed(TRUE);
     // browse the words:
      do
      {// skip leading delimiters, unless continuing the word:
        if (!i) // not continuing
        {// direct the delimiters to the output 
          while (!FT_WORD_STARTER(c))
          { agent_data->word[i++]=c;  
            if (i==MAX_FT_WORD_LEN || !size)
            { agent_data->word[i]=0;  agent_data->word_completed(FALSE);  i=0; 
              if (!size) { *agent_data->word=0;  goto eop; }
            }
            c=*str;  str++;  size--;
          } 
          if (i)
            { agent_data->word[i]=0;  agent_data->word_completed(FALSE);  i=0; }
        }
       // read the word, c is the current character, str has been moved and size decreased:
        do
        { if (i<MAX_FT_WORD_LEN) agent_data->word[i++]=c;
          if (!size) {agent_data->word[i]=0;  goto eop; } // the word may continue in the next part
          c=*str;  str++;  size--;
        } while (FT_WORD_CONTIN(c));
        agent_data->word[i]=0;
        agent_data->word_completed(TRUE);
        i=0;
      } while (size);
      *agent_data->word=0;
     eop:
      break;
    }
    case C97_INS_BREAK_AT_END_OF_DOC:    case C97_INS_BREAK_AFTER_FRAMES:    case C97_INS_ROWCOL_BREAK:
    case C97_INS_TAB:    case C97_INS_HARDSPACE:  case C97_INS_BREAK:
    case C97_E_CARET_TO_SHAPE_TEXT:  case C97_E_GOTO_SHAPE:  
    case C97_CREATE_AND_PLACE_TEXT_FRAME:  case C97_CREATE_TEXT_FRAME:  case C97_END_TEXT_FRAME:
    case C97_CREATE_AND_PLACE_TEXT_TABLE:  case C97_CREATE_TEXT_TABLE:  case C97_END_TEXT_TABLE:
    case C97_IT_IS_ALL:
      if (*agent_data->word)
      { agent_data->word_completed(TRUE);
        agent_data->word[0]=' ';  agent_data->word[1]=0;
        agent_data->word_completed(FALSE);
        *agent_data->word=0;
      }
      break;
    case C97_INS_HARDHYPHEN:
      if (*agent_data->word && strlen(agent_data->word)<MAX_FT_WORD_LEN)
        strcat(agent_data->word, "-");
      break;
#ifdef WINS
    case C97_ACQUIRE_CONFIG_PARAMS:
    { static TConfigParams   cp;
      cp.flags.bIgnore602Contents = TRUE;
      cp.flags.bIgnoreObjects = TRUE;
      return &cp;
    }
  // dal�poloky odstel�objekty
   case C97_CREATE_OLE_OBJ:
   {OLEOBJDEF ood = *(OLEOBJDEF*)(LPVOID)argA;
    if (ood.hMeta != 0) DeleteMetaFile((HMETAFILE)ood.hMeta);
    else
     if (ood.hEnhMeta != 0) DeleteEnhMetaFile((HENHMETAFILE)ood.hEnhMeta);
    break;
   }
   case C97_CREATE_OBJ:
   {OBJDEF od;
    od = *(OBJDEF*)(LPVOID)argA;
    switch (od.linkData.linkType)
     {
      case tMetafile:
       if (od.linkData.pic.pData != NULL)
        if (((METAFILEPICT*)od.linkData.pic.pData)->hMF != NULL)
         if (od.linkData.wFlags != 0  &&  od.linkData.wFlags != LFLAG_RUBBER)
          DeleteEnhMetaFile((HENHMETAFILE)od.linkData.pic.pData);
         else
          DeleteMetaFile((HMETAFILE)od.linkData.pic.pData);
       break;
      case tJPEG:      case tPNG:      case tGIF:      case tTIFF:      case tCOMPRESSED:      case tDIB:
       if (od.linkData.pic.pData != NULL) GlobalFree(od.linkData.pic.pData);
       break;
     };
    break;
   }
#endif
  };
 return 0;
};



BOOL ft_context(cdp_t cdp, t_value * text, const char * format, int mode, unsigned position, int pre_words, int post_words, const char * prefix, const char * suffix, t_value * res, t_value * error_text) 
// Plain text for mode 2 not supported
{
  res->set_simple_not_null();  res->length=0;
  if (text->is_null()) return TRUE;
  t_context_agent_data agent_data(cdp, position, 1, pre_words, post_words, prefix, suffix, res); 
#ifdef STOP
  if (mode!=2) 
    if (!*format || !stricmp(format, "PLAIN") || !stricmp(format, "TEXT") || !stricmp(format, "TXT")) // plain text -> direct content or file name, but not OLE
    { text->load(cdp);  tptr dirtext=text->valptr(); 
      if (!mode) // direct
      { int length = strlen(dirtext);
        agentContext97Callback(&agent_data, C97_INS_TEXT, (T_CC97ARG*)dirtext, *(T_CC97ARG*)&length, (LPVOID)NULL, (LPVOID)NULL);
      }
      else
      { FHANDLE hnd=CreateFile(dirtext, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (!FILE_HANDLE_VALID(hnd)) return FALSE;
        char buf[1024];  DWORD rd;
        while (ReadFile(hnd, buf, sizeof(buf), &rd, NULL) && rd) 
          agentContext97Callback(&agent_data, C97_INS_TEXT, (T_CC97ARG*)buf, *(T_CC97ARG*)&rd, (LPVOID)NULL, (LPVOID)NULL);
        CloseFile(hnd);
      }
     // add the last unterminated word, if any:
      if (*agent_data.word)
      { agent_data.word_completed(TRUE);
        *agent_data.word=0;
      }
      return TRUE;
    }
#endif

  char textfilepathname[MAX_PATH];  BOOL result=TRUE;  BOOL deltempfile;
  if (mode==1) // text is the file name
  { text->load(cdp);
    strmaxcpy(textfilepathname, text->valptr(), sizeof(textfilepathname));  
    deltempfile=FALSE;
  }
  else if (mode==2)
#if 0 // ifdef WINS  temp 10
  { if (!ole_initalized) { OleInitialize(NULL);  ole_initalized=wbtrue; }
    if (text->vmode==mode_access) // not loaded
    { if (TxtAttr2FileD(cdp, text->dbacc.tb, text->dbacc.rec, text->dbacc.attr, text->dbacc.index, 
                       textfilepathname, &deltempfile)!=NOERROR)
        return FALSE;
    }
    else // already in memory
    { if (TxtAttr2FileM(text->valptr(), strlen(text->valptr()), textfilepathname, &deltempfile)!=NOERROR)
        return FALSE;
    }
  }
#else // LINUX mode 2
  { SET_ERR_MSG(cdp, "Conversion");  request_error(cdp, LIBRARY_NOT_FOUND);
    return FALSE;
  }
#endif // LINUX
  else // copying the direct text to the temporary file:
  { char pathname[MAX_PATH];  GetTempPath(sizeof(pathname), pathname);
    GetTempFileName(pathname, "FTX", 0, textfilepathname);
    FHANDLE hnd=CreateFile(textfilepathname, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (!FILE_HANDLE_VALID(hnd)) return FALSE;
    text->load(cdp);  
    if (text->length <=1)  // document is empty, some versions of XLIB do not support it
      { CloseFile(hnd);  DeleteFile(textfilepathname);  return TRUE; }
    DWORD wr;
    WriteFile(hnd, text->valptr(), text->length, &wr, NULL);
    CloseFile(hnd);
    deltempfile=TRUE;
  }
#ifdef WINS
 // load the conversion routine:
  HINSTANCE hXDll;  BOOL routine_found=TRUE;  TFileType file_type = ftUnknown;  FILEERROR sError;
  if (!stricmp(format, "PLAIN") || !stricmp(format, "TEXT") || !stricmp(format, "TXT"))
    file_type=ftASCII;
  if (file_type == ftUnknown)
  { IMPORTFILE * lpFnConvert = get_conversion_routine("wb_ximp", hXDll);
    if (!lpFnConvert) routine_found=FALSE;
    else sError = (*lpFnConvert)(&agent_data, textfilepathname, agentContext97Callback);
  }
  else
  { IMPORTFILEEX * lpFnConvertEx = get_conversion_routine_ex("wb_ximp", hXDll);
    if (!lpFnConvertEx) routine_found=FALSE;
    else sError = (*lpFnConvertEx)(&agent_data, textfilepathname, agentContext97Callback, file_type, NULL);
  }
  if (!routine_found)
    if (error_text) *res=*error_text;
    else if (!hXDll)
      { SET_ERR_MSG(cdp, "WB_XIMP");  request_error(cdp, CONVERSION_NOT_SUPPORTED);  result=FALSE;  goto ex0; }
    else
      { SET_ERR_MSG(cdp, "Entry");    request_error(cdp, CONVERSION_NOT_SUPPORTED);  result=FALSE;  goto ex1; }
  else if (sError)
    if (error_text) *res=*error_text;
    else
    { char buf[20];  int2str(sError, buf);
        SET_ERR_MSG(cdp, buf);        request_error(cdp, CONVERSION_NOT_SUPPORTED);  result=FALSE;  
    }
 ex1:
  FreeLibrary(hXDll);
#else // Linux: home-made konverze pro HTML
  FILEERROR sError;
  sError=convert_HTML_file(&agent_data, textfilepathname, agentContext97Callback);
  if (sError) 
    if (error_text) *res=*error_text;
    else SET_ERR_MSG(cdp, "HTML context");
#endif
 ex0:
  if (deltempfile) DeleteFile(textfilepathname);
  return result;
}

class t_agent_context : public t_agent_base
{
  unsigned position, word_count;
  unsigned req_pos, skip_words, copy_words;
  const char * prefix, * suffix;
  t_value * res;
  void output(const char * str);
 public:
  t_agent_context(cdp_t cdpIn, unsigned req_posIn, unsigned word_countIn, unsigned pre_wordsIn, unsigned post_wordsIn, 
                       const char * prefixIn, const char * suffixIn, t_value * resIn) : t_agent_base(cdpIn)
  { *word=0;  position=0;
    req_pos=req_posIn;  word_count=word_countIn;  prefix=prefixIn;  suffix=suffixIn;
    skip_words = req_posIn > pre_wordsIn ? req_posIn - pre_wordsIn : 0;
    copy_words = req_posIn + post_wordsIn - skip_words + word_countIn;
    res = resIn;
  }
  bool word_completed(bool is_a_word = true);  // when false is returned, no more words are necessary
  t_specif get_ft_specif(void) const 
    { return sys_spec; }  // wide
};

bool t_agent_context::word_completed(bool is_a_word)
{ if (copy_words)
  { if (skip_words) 
    { if (is_a_word) { skip_words--;  position++; }
    }
    else if (is_a_word) // copy word
    { if (position==req_pos) output(prefix);
      output(word);
      copy_words--;  position++;
      if (position==req_pos+word_count) output(suffix);
    }
    else // copy delimiter
      output(word);
    return true;
  }
  else
  { if (is_a_word) position++;
    return false;
  }    
}

void t_agent_context::output(const char * str)
{ int len=(int)strlen(str);
  if (res->reallocate(res->length+len+1))
    return;  // cannot alloc
  if (res->length) res->valptr()[res->length++]=' ';  // temp.
  strcpy(res->valptr()+res->length, str);
  res->length+=len;
}


BOOL t_ft_kontext::ft_context9(cdp_t cdp, t_value * text, const char * format, int mode, t_specif specif, unsigned position, unsigned word_count, int pre_words, int post_words, const char * prefix, const char * suffix, t_value * res) 
// For v9 fulltext system only.
// [position] includes "simple words" so it is not necessary to lemmatize/test words.
{ res->set_simple_not_null();  res->length=0;
  if (text->is_null()) return TRUE;
  { t_agent_context agent_data(cdp, position, word_count, pre_words, post_words, prefix, suffix, res); 
  // prepare the input stream:
   t_input_stream * is;
   if (mode==0)
     if (text->vmode==mode_access)
       is = new t_input_stream_db(cdp, text->dbacc.tb, text->dbacc.rec, text->dbacc.attr, text->dbacc.index);
     else
       is = new t_input_stream_direct(cdp,text->valptr(), text->length);
   else if (mode==1)
   { text->load(cdp);
     is = new t_input_stream_file(cdp,text->valptr(),NULL /*don't set filename for LIMITS evaluation*/);
   }
   else
     { SET_ERR_MSG(cdp, "<OLE>");  request_error(cdp, CONVERSION_NOT_SUPPORTED);  return FALSE; }
   if (!is) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
  // prepare the convertor:
   t_input_convertor * ic = ic_from_format(cdp,format);
   if (!ic)
   { ic = ic_from_text(cdp,is);
     is->reset();
     if (!ic)
     { //if (error_text) *res=*error_text;  else 
       { SET_ERR_MSG(cdp, "<Unknown format>");  request_error(cdp, CONVERSION_NOT_SUPPORTED); } 
       delete is;  
       return FALSE; 
     }
   }
   ic->set_stream(is);
   ic->set_agent(&agent_data);
   ic->set_owning_ft(this);
   if( mode==0 ) ic->set_specif(specif);   // may not be used by some convertors, but is used in the plain text
   ic->set_depth_level(1);
   /* don't set LIMITS evaluator - if evaluator isn't set then convertor doesn't count words and doesn't test LIMITS condition */
   bool res = ic->convert();
   delete ic;
   delete is;
   return res;
 }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool create_fulltext_system(cdp_t cdp, const char * ft_label, const char * info, const char * fulltext_object_name, 
                            const char * fulltext_schema_name, int language, bool with_substructures, bool separated, bool bigint_id)
// Creates a new fulltext system: INFO object, sequence, word table and ref table.
// It can be the v8 system or v9 system. Difference is in the contents of [info] (SQL statement or INI), 
//   in the NULL values of [fulltext_object_name], [fulltext_schema_name] and if in the -1 of [language].
{ tobjname schemaname, suffix, doctabname;  char command[MAX_PATH+1];   ttablenum reftabnum, wordtabnum;
  if (!fulltext_allowed)
    { request_error(cdp, NO_FULLTEXT_LICENCE);  return false; }
 // get schema and suffix:
  if (!analyse_ft_label(cdp, ft_label, schemaname, suffix, doctabname))
    { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return false; }
  if (fulltext_schema_name!=NULL)  // v9
    strcpy(schemaname, fulltext_schema_name);
  else if (!*schemaname) 
    ker_apl_id2name(cdp, cdp->current_schema_uuid, schemaname, NULL);

  tobjname infoname;  tobjnum infonum;
  if (fulltext_object_name!=NULL) // v9
  { strcpy(infoname, fulltext_object_name);
    strcpy(suffix, fulltext_object_name);
  }
  else 
  { strcpy(infoname, "FTX_DESCR");  strcat(infoname, suffix); 
    int bi;
    if (get_info_value(info, NULLSTRING, "bigint_id",  ATT_INT32, &bi))
      bigint_id = bi==1;
  }
  const char * id_type_name;
  id_type_name = bigint_id ? "BIGINT" : "INT";
 // create objects:
  if (!name_find2_obj(cdp, infoname, schemaname, CATEG_INFO, &infonum)) 
    { SET_ERR_MSG(cdp, infoname);  request_error(cdp, KEY_DUPLICITY);  return false; }
  WBUUID uuid;
  if (!schema_uuid_for_new_object(cdp, schemaname, uuid)) return false;
  infonum=(tobjnum)create_object(cdp, infoname, uuid, info, CATEG_INFO, FALSE);
  if (infonum!=NORECNUM) 
 // info object created
  { if (separated)
    { uns16 flags = CO_FLAG_FT_SEP;
      tb_write_atr(cdp, objtab_descr, infonum, OBJ_FLAGS_ATR, &flags);
    }
    sprintf(command, "CREATE SEQUENCE `%s`.`FTX_DOCID%s` START WITH 1 INCREMENT BY 1 NOMAXVALUE NOMINVALUE NOCACHE NOCYCLE", 
             schemaname, suffix);
    sql_statement * so = sql_submitted_comp(cdp, command);
    if (so!=NULL) 
    { so->exec(cdp);
      delete so;
      if (!cdp->is_an_error()) // sequence created
      {// creating DOCPART table, only if required:
        if (with_substructures)
        { sprintf(command, "CREATE TABLE `%s`.`FTX_DOCPART%s`(DOCID %s,POSITION INT,REL_PATH CHAR(%d),INDEX(DOCID,POSITION))", 
                   schemaname, suffix, id_type_name, MAX_REL_PATH);
          sql_statement * so = sql_submitted_comp(cdp, command);
          if (so!=NULL) 
          { cdp->clear_sql_results();  so->exec(cdp);  
            delete so;
          }
        }
        if (!cdp->is_an_error()) // everything created OK, so far
        { 
#if WBVERS>=950
          if (separated)
          { get_fulltext_file_name(cdp, command, schemaname, suffix);
            ext_index_file eif;
            if (!eif.create(command))
              request_error(cdp, OS_FILE_ERROR);
            else  // file created OK
              return true;
          }
          else
#endif
          { char collation_spec[15+OBJ_NAME_LEN];
            *collation_spec=0;  // US, UK and unspecified (v8)
            if (language==FT_LANG_CZ)
              strcpy(collation_spec, " COLLATE CZ");
            else if (language==FT_LANG_SK)
              strcpy(collation_spec, " COLLATE SK");
            else if (language==FT_LANG_GR)
              strcpy(collation_spec, " COLLATE DE");
            else if (language==FT_LANG_PL)
              strcpy(collation_spec, " COLLATE PL");
            else if (language==FT_LANG_FR)
              strcpy(collation_spec, " COLLATE FR");
            if (*collation_spec) strcat(collation_spec, 
    #ifdef WINS
            "_WIN");
    #else
            "_ISO");
    #endif
            sprintf(command, "CREATE TABLE `%s`.`FTX_WORDTAB%s` (WORD CHAR(%u)%s,WORDID INT DEFAULT UNIQUE,PRIMARY KEY(WORD))", 
                     schemaname, suffix, MAX_FT_WORD_LEN, collation_spec);
            so = sql_submitted_comp(cdp, command);
            if (so!=NULL) 
            { cdp->clear_sql_results();  so->exec(cdp);  wordtabnum=(ttablenum)*cdp->sql_results_ptr();
              delete so;
              if (!cdp->is_an_error())
              { sprintf(command, "CREATE TABLE `%s`.`FTX_REFTAB%s`(WORDID INT,DOCID %s,POSITION INT,FLAGS CHAR,INDEX(WORDID,DOCID,POSITION),INDEX(DOCID))", 
                         schemaname, suffix, id_type_name);
                sql_statement * so = sql_submitted_comp(cdp, command);
                if (so!=NULL) 
                { cdp->clear_sql_results();  so->exec(cdp);  reftabnum=(ttablenum)*cdp->sql_results_ptr();
                  delete so;
                  if (!cdp->is_an_error()) // FTX_REFTAB created OK
                    return true;  // everything created OK so far
                 // error return - inverting committed actions:
                 // REFTAB created, drop it:
                  //sprintf(command, "DROP TABLE `%s`.`FTX_REFTAB%s`", schemaname, suffix);
                  //exec_direct(cdp, command);
                }
               // WORDTAB created, drop it:
                sprintf(command, "DROP TABLE `%s`.`FTX_WORDTAB%s`", schemaname, suffix);
                exec_direct(cdp, command);
              } // FTX_WORDTAB created OK
            }
          }
         // continuing the error exit, common for internal and separated fulltexts:
          if (with_substructures)
          {// DOCPART table created, drop it:
            sprintf(command, "DROP TABLE `%s`.`FTX_DOCPART%s`", schemaname, suffix);
            exec_direct(cdp, command);
          }
        } // DOCPART table created OK, if required
       // sequence created, drop it:
        sprintf(command, "DROP SEQUENCE `%s`.`FTX_DOCID%s`", schemaname, suffix);
        exec_direct(cdp, command);
      } // seqence created OK
    } // sequence creation compiled OK
    tb_del(cdp, objtab_descr, infonum);
  } // info object created
  return false;
}

bool ft_destroy(cdp_t cdp, const char * ft_label, const char * fulltext_object_name, const char * fulltext_schema_name) 
// Destroys the fulltext system, for v8 and v9
// The object is put into the "destructed" state but references to it remain valid until any reference exists
{ tobjname schemaname, suffix, doctabname;  
 // analyse the ft_label:
  if (!analyse_ft_label(cdp, ft_label, schemaname, suffix, doctabname))
    { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return false; }
  if (fulltext_schema_name!=NULL)  // v9 (or v8 in DROP SCHEMA)
  { strcpy(schemaname, fulltext_schema_name);
    strcpy(suffix, fulltext_object_name);
  }
 // remove the fulltext kontext from the list and destruct it (must close external file, unlock table descrs):
  WBUUID schema_uuid;
  if (!*schemaname) // search by [schema_uuid]:
    memcpy(schema_uuid, cdp->current_schema_uuid, UUID_SIZE);
  else
    ker_apl_name2id(cdp, schemaname, schema_uuid, NULL);
 // search existing kontexts:
  t_ft_kontext ** pftk = &global_ftk;
  while (*pftk) 
  { t_ft_kontext * ftk = *pftk;
    if (!strcmp(suffix, ftk->suffix))
      if (*schemaname ? !strcmp(schemaname, ftk->schemaname) : !memcmp(schema_uuid, ftk->schema_uuid, UUID_SIZE))
    { *pftk = ftk->next;
      ftk->destruct();  ftk->Release();
      break;
    }
    pftk=&(*pftk)->next;
  }
 // delete the dependent objects:
  char command[2*OBJ_NAME_LEN+100];
  sprintf(command, "DROP IF EXISTS TABLE `%s`.`FTX_REFTAB%s`", schemaname, suffix);
  if (exec_direct(cdp, command)) return false;
  sprintf(command, "DROP IF EXISTS TABLE `%s`.`FTX_WORDTAB%s`", schemaname, suffix);
  if (exec_direct(cdp, command)) return false;
  sprintf(command, "DROP IF EXISTS TABLE `%s`.`FTX_DOCPART%s`", schemaname, suffix);
  if (exec_direct(cdp, command)) return false;
  sprintf(command, "DROP IF EXISTS SEQUENCE `%s`.`FTX_DOCID%s`", schemaname, suffix);
  if (exec_direct(cdp, command)) return false;
 // ext file:
  if (fulltext_object_name!=NULL) // v9
  { char fname[MAX_PATH];
    get_fulltext_file_name(cdp, fname, fulltext_schema_name, fulltext_object_name);
    DeleteFile(fname);  // the file does not exist for internel fulltext systems
  }
 // delete the fulltext object:
  tobjname infoname;  tobjnum infonum;
  if (fulltext_object_name!=NULL) // v9
    strcpy(infoname, fulltext_object_name);
  else { strcpy(infoname, "FTX_DESCR");  strcat(infoname, suffix); }
  if (!name_find2_obj(cdp, infoname, schemaname, CATEG_INFO, &infonum)) 
    tb_del(cdp, objtab_descr, infonum);
  return true;
}

void update_ft_kontext(cdp_t cdp, t_fulltext * ftdef) 
{
  t_ft_kontext * ftk = global_ftk;
  while (ftk) 
  { if (!strcmp(ftdef->name, ftk->suffix))
      if (!*ftdef->schema ? !memcmp(cdp->current_schema_uuid, ftk->schema_uuid, UUID_SIZE) : !strcmp(ftdef->schema, ftk->schemaname)) 
        break;
    ftk=ftk->next;
  }
  if (ftk)
    ftk->set_limits_value(ftdef->limits ? ftdef->limits : NULLSTRING);
}

t_ft_kontext * get_fulltext_kontext(cdp_t cdp, const char * ft_label, const char * fulltext_object_name, const char * fulltext_schema_name)
// Returns fulltext kontexts for an existing fulltext system. Works for v8 and v9 systems.
// fulltext_object_name and fulltext_schema_name must be in upper case!
{ tobjname schemaname, suffix, doctabname;  WBUUID schema_uuid;
  if (!fulltext_allowed)
    { request_error(cdp, NO_FULLTEXT_LICENCE);  return NULL; }
 // analyse the ft_label:
  if (!analyse_ft_label(cdp, ft_label, schemaname, suffix, doctabname))
    { request_error(cdp, ERROR_IN_FUNCTION_ARG);  return NULL; }
  if (fulltext_schema_name!=NULL)  // called from v9 SQL statement
  { strcpy(schemaname, fulltext_schema_name);
    strcpy(suffix, fulltext_object_name);
  }
 // search existing kontexts:
  t_ft_kontext * ftk = global_ftk;
  if (!*schemaname) // search by [schema_uuid]:
  { memcpy(schema_uuid, cdp->current_schema_uuid, UUID_SIZE);
    while (ftk) 
    { if (!strcmp(suffix, ftk->suffix) && !memcmp(schema_uuid, ftk->schema_uuid, UUID_SIZE)) return ftk;
      ftk=ftk->next;
    }
    ker_apl_id2name(cdp, schema_uuid, schemaname, NULL);
  }
  else // search by [schemaname]:
  { while (ftk) 
    { if (!strcmp(suffix, ftk->suffix) && !strcmp(schemaname, ftk->schemaname))
        return ftk;
      ftk=ftk->next;
    }
    if (ker_apl_name2id(cdp, schemaname, schema_uuid, NULL)) 
      { SET_ERR_MSG(cdp, schemaname);  request_error(cdp, OBJECT_NOT_FOUND);  return NULL; }
  }
 // Fulltext kontext does not exist, schema_uuid is defined.

 // find and read the information object:
  bool v9_system = true;  tobjnum infonum;  tobjname infoname;  
  BOOL basic_form = TRUE;  int language=0;  BOOL weighted=FALSE;  bool separated=false, bigint_id=false;  // default values
  t_fulltext ftdef;  // must not have shorter existence than word_starters
      
  if (fulltext_object_name!=NULL) // called from v9 SQL statement, searching v9 FT systems only
  { strcpy(infoname, fulltext_object_name);
    infonum=find2_object(cdp, CATEG_INFO, schema_uuid, infoname);
  }
  else  // called from a traditional function like the "Fulltext" predicate, must work for v8 and v9 FT systems
  { strcpy(infoname, suffix);
    infonum=find2_object(cdp, CATEG_INFO, schema_uuid, infoname);  // trying the v9 FT name
    if (infonum==NOOBJECT)  // v9 FT name not found, try the v8 FT name
    { strcpy(infoname, "FTX_DESCR");   strcat(infoname, suffix); 
      infonum=find2_object(cdp, CATEG_INFO, schema_uuid, infoname);
      v9_system=false;  //  ... even if not found
    }
  }
  if (infonum!=NOOBJECT)  // infonum==NOOBJECT possible for v8 systems, using default values
  { tptr info = ker_load_objdef(cdp, objtab_descr, infonum);
    if (!info) return NULL;
    if (v9_system)
    { int err = ftdef.compile(cdp, info);
      if (err) return NULL;
      language=ftdef.language;
      basic_form=ftdef.basic_form;
      separated=ftdef.separated;
      bigint_id=ftdef.bigint_id;
    }
    else
    { get_info_value(info, NULLSTRING, "language",   ATT_INT32, &language);  // protected against info==NULL
      get_info_value(info, NULLSTRING, "basic_form", ATT_INT32, &basic_form);
      get_info_value(info, NULLSTRING, "weighted",   ATT_INT32, &weighted);
      int bi;
      if (get_info_value(info, NULLSTRING, "bigint_id",  ATT_INT32, &bi))
        bigint_id = bi==1;
    }
    corefree(info);
  }

 // look for the fulltext system tables:
  ttablenum reftabnum=NOTBNUM, wordtabnum=NOTBNUM, doctabnum, docpartnum=NOTBNUM;
  tobjname tabname;  
  if (separated)  
  {
  }
  else
  { strcpy(tabname, "FTX_WORDTAB");  strcat(tabname, suffix);
    wordtabnum=find2_object(cdp, CATEG_TABLE, schema_uuid, tabname);
    if (wordtabnum==NOOBJECT)
      if (infonum==NOOBJECT)  // better error report: fulltext object does not exist
        { SET_ERR_MSG(cdp, fulltext_object_name ? fulltext_object_name : suffix);  request_error(cdp, OBJECT_NOT_FOUND);  return NULL; }
      else
        { SET_ERR_MSG(cdp, tabname);  request_error(cdp, OBJECT_NOT_FOUND);  return NULL; }
    strcpy(tabname, "FTX_REFTAB");   strcat(tabname, suffix);
    reftabnum=find2_object(cdp, CATEG_TABLE, schema_uuid, tabname);
    if (reftabnum==NOOBJECT)
      { SET_ERR_MSG(cdp, tabname);  request_error(cdp, OBJECT_NOT_FOUND);  return NULL; }
    doctabnum=find2_object(cdp, CATEG_TABLE, schema_uuid, doctabname);
    if (doctabnum==NOOBJECT)
      doctabnum=NOTBNUM;  // may have other name!
  }
 // common for internal and external:
  strcpy(tabname, "FTX_DOCPART");   strcat(tabname, suffix);
  docpartnum=find2_object(cdp, CATEG_TABLE, schema_uuid, tabname);
  if (docpartnum!=NOOBJECT)  // verify the structure of the table
  { table_descr_auto docpart_td(cdp, docpartnum);
    if (docpart_td->me())
    { if (docpart_td->attrs[1].attrtype!=ATT_INT32 ||
          docpart_td->attrs[2].attrtype!=ATT_INT32 ||
          docpart_td->attrs[3].attrtype!=ATT_STRING)
        docpartnum=NOOBJECT;
    }
    else docpartnum=NOOBJECT;
  }
 // create the new fulltext kontext:
   return 
#if WBVERS>=950
      separated ?
    (t_ft_kontext*)new t_ft_kontext_separated(cdp, schemaname, schema_uuid, suffix, doctabname,                                   basic_form, language, weighted,            docpartnum, ftdef.limits, ftdef.word_starters, ftdef.word_insiders, bigint_id) :
#endif
    (t_ft_kontext*)new t_ft_kontext_intern   (cdp, schemaname, schema_uuid, suffix, doctabname, wordtabnum, reftabnum, doctabnum, basic_form, language, weighted, v9_system, docpartnum, ftdef.limits, ftdef.word_starters, ftdef.word_insiders, bigint_id);
}

#if WBVERS>=950
bool t_ft_kontext_separated::purge(cdp_t cdp, bool locked)
{ char fname[MAX_PATH];  bool res=true;
  get_fulltext_file_name(cdp, fname, schemaname, suffix);
  if (!locked) eif.mrsw_lock.writer_entrance(cdp, WAIT_EXTFT);
  eif.close();
  if (!eif.create(fname))
    { request_error(cdp, OS_FILE_ERROR);  res=false; }
  else  // file re-created OK
    if (!eif.open(fname))  // re-open
      { request_error(cdp, OS_FILE_ERROR);  res=false; }
  if (!locked) eif.mrsw_lock.writer_exit();
  return res;
}

void t_ft_kontext_separated::check(cdp_t cdp)
{
  t_reader mrsw_reader(&eif.mrsw_lock, cdp);
  cb_pass_all(eif, word_index_oper, WORD_INDEX_ROOT_NUM);
  if (bigint_id)
  { cb_pass_all(eif, ref1_index_oper_64, REF1_INDEX_ROOT_NUM);
    cb_pass_all(eif, ref2_index_oper_64, REF2_INDEX_ROOT_NUM);
  }
  else
  { cb_pass_all(eif, ref1_index_oper_32, REF1_INDEX_ROOT_NUM);
    cb_pass_all(eif, ref2_index_oper_32, REF2_INDEX_ROOT_NUM);
  }
  char msg[200];
  sprintf(msg, "Fulltext errors: %u, %u, %u, %u, %u, %u.", eif.err_alloc, eif.err_magic, eif.err_order, 
                 eif.err_read, eif.err_stack, eif.err_used_space);
  dbg_err(msg);
}

void t_ft_kontext_separated::locking(cdp_t cdp, bool lock_it, bool write_lock)
// External index locking API: just for debugging nad analysis
{
  if (lock_it)
    if (write_lock)
      eif.mrsw_lock.writer_entrance(cdp, WAIT_EXTFT);
    else
      eif.mrsw_lock.reader_entrance(cdp, WAIT_EXTFT);
  else      
    if (write_lock)
      eif.mrsw_lock.writer_exit();
    else  
      eif.mrsw_lock.reader_exit();
}
#endif

bool t_ft_kontext_intern::purge(cdp_t cdp, bool locked)
{ truncate_table(cdp, ref_td->me()); 
  return true;
}

void ft_purge(cdp_t cdp, const char * fulltext_object_name, const char * fulltext_schema_name)
{ t_ft_kontext * ftk = get_fulltext_kontext(cdp, NULLSTRING, fulltext_object_name, fulltext_schema_name);
  if (ftk) 
    ftk->purge(cdp, false);
}

void fulltext_docpart_start(cdp_t cdp, const char * ft_label, t_docid docid, unsigned position, const char * relative_path)
// Indexing documents analysed on the client side.
{ t_ft_kontext * ftk = get_fulltext_kontext(cdp, ft_label, NULL, NULL);
  if (!ftk) return;
  ftk->ft_starting_document_part(cdp, docid, position, relative_path);
}

void fulltext_insert(cdp_t cdp, const char * ft_label, t_docid docid, unsigned startpos, unsigned bufsize, const char * buf)
// Indexing documents analysed on the client side.
// When startpos is negative, then it is -PID (all PIDs are small positive numbers) and buf contains a summary item.
{ t_ft_kontext * ftk = get_fulltext_kontext(cdp, ft_label, NULL, NULL);
  if (!ftk) return;
  if ((int)startpos < 0)  // summary item!
  { unsigned infotype = (unsigned)-(int)startpos;
    ftk->ft_set_summary_item(cdp, docid, infotype, buf);
  }
  else  // words from the text
  { if (ftk->basic_form) 
      if (!init_lemma(ftk->language))
        { SET_ERR_MSG(cdp, lpszMainDictName[ftk->language]);  request_error(cdp, LANG_SUPP_NOT_AVAIL);  return; }
    unsigned pos=0;
    while (pos<bufsize)
    { ftk->ft_insert_word_occ(cdp, docid, buf, startpos++, 0);
      int step=(int)strlen(buf)+1;
      pos+=step;  buf+=step;
    }
  }
}

void mark_fulltext_kontext_list(void)
{ for (t_ft_kontext * ftk = global_ftk;  ftk;  ftk=ftk->next)
    ftk->mark_core();
}

void fulltext_index_column(cdp_t cdp, t_ft_kontext * ftk, table_descr * tbdf, trecnum recnum, tattrib text_col, tattrib id_col, const char * format, int mode)
{// read id:
  t_docid doc_id;  char * textbuf;  uns32 len;
  attribdef * att = &tbdf->attrs[text_col];
  if (att->attrtype==ATT_TEXT || att->attrtype==ATT_EXTCLOB)
    tb_read_atr_len(cdp, tbdf, recnum, text_col, &len);
  else 
    len=MAX_FIXED_STRING_LENGTH;
  textbuf = (char*)corealloc(len+2, 44);
  if (textbuf)
  { if (att->attrtype==ATT_TEXT || att->attrtype==ATT_EXTCLOB)
    { tb_read_var(cdp, tbdf, recnum, text_col, 0, len, textbuf);
      textbuf[len]=textbuf[len+1]=0;
    }
    else
      tb_read_atr(cdp, tbdf, recnum, text_col, textbuf);
    doc_id=0;  // important when !ftk->bigint_id
    tb_read_atr(cdp, tbdf, recnum, id_col, (char*)&doc_id);
    t_value text_val;
    text_val.vmode=mode_indirect;
    text_val.indir.val=(char*)textbuf;
    text_val.length = att->attrspecif.wide_char ? 2*(int)wuclen((const wuchar*)textbuf) : (int)strlen(textbuf);
    text_val.indir.vspace=text_val.length+2;
    ftk->ft_index_doc(cdp, doc_id, &text_val, format, mode, att->attrspecif);
    text_val.indir.val=NULL;
    text_val.vmode=mode_null;
    corefree(textbuf);
  }
}

#if WBVERS>=950
void refresh_index(cdp_t cdp, const t_fulltext * ft)
{ 
  if (!ft->separated) return;
  t_ft_kontext * ftk = (t_ft_kontext*)get_fulltext_kontext(cdp, NULLSTRING, ft->name, ft->schema);
  if (!ftk) return;
  t_ft_kontext_separated * ftks = (t_ft_kontext_separated*)ftk;
  t_writer mrsw_writer(&ftks->eif.mrsw_lock, cdp);
  if (!ftks->purge(cdp, true)) return;
 // pass documents and sort word occurrences by index1:
  {// prepare sorter:
    t_scratch_sorter scratch_sorter(ft->bigint_id ? (cbnode_oper*)&ref1_index_oper_64 : (cbnode_oper*)&ref1_index_oper_32, &ftks->eif, false);
    if (scratch_sorter.prepare(cdp)) return;
    ftks->scratch_sorter = &scratch_sorter;
   // pass document sources:
    for (int i=0;  i<ft->items.count();  i++)
    { const t_autoindex * ai = ft->items.acc0(i);
      tobjnum objnum;
      if (!name_find2_obj(cdp, ai->doctable_name, ai->doctable_schema, CATEG_TABLE, &objnum))
      { table_descr_auto doctab_td(cdp, objnum);
        if (doctab_td->me())
        {// pass documents: 
          for (trecnum r=0;  r<doctab_td->Recnum();  r++)
          { uns8 del=table_record_state(cdp, doctab_td->me(), r);
            if (del==NOT_DELETED)
            { tattrib text_atr = find_attrib(doctab_td->me(), ai->text_expr);
              tattrib id_atr = find_attrib(doctab_td->me(), ai->id_column);
              if (text_atr!=NOATTRIB && id_atr!=NOATTRIB)
                fulltext_index_column(cdp, ftk, doctab_td->me(), r, text_atr, id_atr, ai->format, ai->mode);
            }
          }
        }
      }
    }
   // close sorter:
    ftks->scratch_sorter = NULL;
    scratch_sorter.complete_sort(REF1_INDEX_ROOT_NUM);
  }
 // pass index1 and sort words by index2:
  { t_scratch_sorter scratch_sorter(ft->bigint_id ? (cbnode_oper*)&ref2_index_oper_64 : (cbnode_oper*)&ref2_index_oper_32, &ftks->eif, true);
    if (scratch_sorter.prepare(cdp)) return;
    cbnode_path path;
    if (ft->bigint_id)
    { t_ft_key64 refkey;  refkey.wordid=0;  refkey.docid=NO_DOC_ID;  refkey.position=0;  
      cb_result res = cb_build_stack(ftks->eif, ref1_index_oper_64, REF1_INDEX_ROOT_NUM, &refkey, path);
      if (res!=CB_ERROR) 
        while (cb_step(ftks->eif, ref1_index_oper_64, &refkey, path))
          scratch_sorter.add_key(&refkey.docid, &refkey.wordid);
    }
    else
    { t_ft_key32 refkey;  refkey.wordid=0;  refkey.docid=(uns32)NO_DOC_ID;  refkey.position=0;  
      cb_result res = cb_build_stack(ftks->eif, ref1_index_oper_32, REF1_INDEX_ROOT_NUM, &refkey, path);
      if (res!=CB_ERROR) 
        while (cb_step(ftks->eif, ref1_index_oper_32, &refkey, path))
          scratch_sorter.add_key(&refkey.docid, &refkey.wordid);
    }
    scratch_sorter.complete_sort(REF2_INDEX_ROOT_NUM);
  }
}
#endif

void t_fulltext2::remove_doc(cdp_t cdp, t_docid doc_id)
{ ftk->ft_remove_doc_occ(cdp, doc_id); }

#if 0
void t_fulltext2::add_doc(cdp_t cdp, t_docid doc_id, const char * text, const char * format, int mode, t_specif specif)
{ t_value text_val;
  text_val.vmode=mode_indirect;
  text_val.indir.val=(char*)text;
  text_val.length = specif.wide_char ? 2*wuclen((const wuchar*)text) : strlen(text);
  text_val.indir.vspace=text_val.length+2;
  ftk->ft_index_doc(cdp, doc_id, &text_val, format, mode, specif);
  text_val.indir.val=NULL;
  text_val.vmode=mode_null;
}
#endif

int backup_ext_ft(cdp_t cdp, const char * fname, bool nonblocking, bool reduce)
// Backups all external fulltext systems into separate files.
// [fname] contains path and the beginning of the target file name.
// Returns error number or 0 on success.
{ int err=0;
#if WBVERS>=1000
  if (fulltext_allowed)  // prevents error messages if fulltext objects exist and there is no fulltext licence
  {// pass all fulltexts in OBJTAB:
    for (trecnum rec =0;  rec < objtab_descr->Recnum();  rec++)
    { tfcbnum fcbn;  
      const ttrec * dt = (const ttrec*)fix_attr_read(cdp, objtab_descr, rec, DEL_ATTR_NUM, &fcbn);
      if (dt!=NULL)
      { tobjname schema_name;
        if (!dt->del_mark && dt->categ == CATEG_INFO && 
            !ker_apl_id2name(cdp, dt->apluuid, schema_name, NULL))
        { tobjname ft_name;
          strmaxcpy(ft_name, dt->name, sizeof(ft_name));
          t_ft_kontext * ft = get_fulltext_kontext(cdp, NULLSTRING, ft_name, schema_name);
          if (ft && ft->separated) // the FT file has been opened in the constructor
          { t_ft_kontext_separated * ftks = (t_ft_kontext_separated *)ft;
            int err0 = ftks->eif.backup_it(cdp, fname, nonblocking, schema_name, ft_name, reduce);
            if (err0) err=err0;  // accumulates the error
          }
        }
        unfix_page(cdp, fcbn);
      }
    }
  }  
#endif
  return err;
}

void complete_broken_ft_backup(cdp_t cdp)
// Looks for incomplete backups of external FT systems, cleans them.
{
#if WBVERS>=1000
  if (fulltext_allowed)  // prevents error messages if fulltext objects exist and there is no fulltext licence
  {// pass all fulltexts in OBJTAB:
    for (trecnum rec=0;  rec < objtab_descr->Recnum();  rec++)
    { tfcbnum fcbn;  
      const ttrec * dt = (const ttrec*)fix_attr_read(cdp, objtab_descr, rec, DEL_ATTR_NUM, &fcbn);
      if (dt!=NULL)
      { tobjname schema_name;
        if (!dt->del_mark && dt->categ == CATEG_INFO && 
            !ker_apl_id2name(cdp, dt->apluuid, schema_name, NULL))
        { tobjname ft_name;
          strmaxcpy(ft_name, dt->name, sizeof(ft_name));
          t_ft_kontext * ft = get_fulltext_kontext(cdp, NULLSTRING, ft_name, schema_name);
          if (ft && ft->separated)
          { t_ft_kontext_separated * ftks = (t_ft_kontext_separated *)ft;
            ftks->eif.complete_broken_backup(schema_name, ft_name);
          }  
        }
        unfix_page(cdp, fcbn);
      }
    }
  }
#endif
}
///////////////////////////////////////////////////////////////////////////////////
/*
wordtab: 1. sloupec string MAX_FT_WORD_LEN
         2. sloupec ID, unique default,
         1. index na 1 sloupec

reftab:  1. sloupec wordid
         2. sloupec DOCID (exact)
         3. sloupec unsigned position
         4. sloupec char flags
         1. index na (1,2,3)
         2. index na docid (nepovinny)
doctab:  docid
         unikatni index na docid
///////////////////////////////////////////////////////////////////////////////////
Algoritmus vyhledavani:
Pro kazdy list v AND-OR stromu prochazim dokumenty, ktere jej obsahuji (slovo nebo frazi) v poradi cisel dokumentu.
Funkce next_doc(NO_DOC_ID) nastavi vsechny listy na prvni dokument.
Funkce next_doc(min_docid) posune na dalsi dokument vsechny listy, ktere jsou na min_docid.
V kazde pozici funkce satisfied najde nejmensi listovy docid a vrati priznak, zda je to na nem splneno.
Konec prochazeni pro list: kdyz se vycerpa bstrom nebo kdyz se pri pruchodu prejde na jine slovo.
Prochazeni fraze: najdu dokument obsahujici celou frazi a jeho id nastavim v prvnim slove fraze.

open_index: Do curr_key dostane prvni vyskyt slova nebo vyskyt, na kterem to minule skoncilo. Neni to tim ale nastaveno 
  na zadnem slove, to az po volani next_doc.

When passing is positioned on [current_docid], [curr_key] refers to another document or [documents_exhausted]==true
When [current_docid]==NO_DOC_ID then passing is no more positioned on any document
*/
