// fulltext tools, common for client and server  
////////////////////////////////// lemma ////////////////////////////////////  
#include "enumsrv.h"

const char lpszMainDictName[NUMBER_OF_LANGUAGES][13] = {  
  "spl___cz.dat",  
  "spl___sl.dat",  
  "spl___gr.dat",  
  "spl___us.dat",  
  "spl___uk.dat",  
  "spl___pl.dat",  
  "spl___hu.dat",  
  "spl___fr.dat"  
};  
  
#if defined(WINS) || WBVERS>=1100
// Dynamic loading of the extension library for lemmatisation
#ifdef LINUX
#include <dlfcn.h>
#endif  

class C_LGS_94   
{ private:  
      CRITICAL_SECTION lem_cs;  
      HINSTANCE        hDLL;  
      BOOL             bErrLoad;  // cannot load, implies hDll==0  
      void *           lpSpel[NUMBER_OF_LANGUAGES];  // !=NULL for initialised languages  
      int   (*lgs_s_sizeof) (void);  
      int   (*lgs_s_prepare)(LPVOID);  
      int   (*lgs_l_init)   (LPVOID,LPSTR,int,BYTE);  
      int   (*lgs_l_find)   (LPVOID,LPSTR,LPSTR,short);  
      void  (*lgs_l_done)   (LPVOID);  
      int Load(const char * pathname);  
  public:  
      C_LGS_94();  
      ~C_LGS_94();  
      void Done();  
      void Unload();  
      BOOL InitLemm(int language);  
      int GetLemma(int language, LPSTR,LPSTR,short);  
};  
  
static C_LGS_94 lgs_94;  
  
C_LGS_94 :: C_LGS_94()  
{ InitializeCriticalSection(&lem_cs);    
  hDLL      = (HINSTANCE) 0;  
  bErrLoad  = FALSE;  
  for (int i=0;  i<NUMBER_OF_LANGUAGES;  i++) lpSpel[i] = NULL;  
}  
  
C_LGS_94 :: ~C_LGS_94()  
{ Unload();   
  DeleteCriticalSection(&lem_cs);    
}  
  
int C_LGS_94 :: Load(const char * pathname)  
{ 
  if (!hDLL && !bErrLoad)
  { if (!(hDLL = LoadLibrary(pathname))) 
    { bErrLoad = TRUE;  
      return 1;  
    }  
    lgs_s_sizeof  = (int (*)(void))                    GetProcAddress(hDLL,/*LGS_S_sizeof*/ "DLL_spell_sizeof");  
    lgs_s_prepare = (int (*)(LPVOID))                  GetProcAddress(hDLL,/*LGS_S_prepare*/ "DLL_spell_prepare");  
    lgs_l_init    = (int (*)(LPVOID,LPSTR,int,BYTE))   GetProcAddress(hDLL,/*LGS_L_init*/ "DLL_lemm_init");  
    lgs_l_find    = (int (*)(LPVOID,LPSTR,LPSTR,short))GetProcAddress(hDLL,/*LGS_L_find*/ "DLL_lemm_find");  
    lgs_l_done    = (void(*)(LPVOID))                  GetProcAddress(hDLL,/*LGS_L_done*/ "DLL_lemm_done");  
    if (lgs_s_sizeof==NULL || lgs_s_prepare==NULL || lgs_l_init==NULL ||
        lgs_l_find==NULL   || lgs_l_done==NULL)
    { FreeLibrary(hDLL);  hDLL=0;   
      bErrLoad = TRUE;  
      return 1;  
    }  
  }  
  return bErrLoad;  
}  
  
void C_LGS_94 :: Done()  
{ if(hDLL)   
  { for (int i=0;  i<NUMBER_OF_LANGUAGES;  i++)   
      if (lpSpel[i])  
      { (*lgs_l_done)(lpSpel[i]);  
        free(lpSpel[i]);  lpSpel[i]=NULL;  
      }  
  }  
}  
  
void C_LGS_94 :: Unload()  
{ if(hDLL)  
  { Done();  
    FreeLibrary(hDLL);  
    hDLL = (HINSTANCE) 0;  
    bErrLoad = FALSE;  
  }  
}  
  
#define LGS_MAIN_PAGES        32  
  
BOOL C_LGS_94 :: InitLemm(int language) // returns FALSE iff OK  
{ ProtectedByCriticalSection cs(&lem_cs);  
 // do nothing if already initialized
  if (lpSpel[language]!=NULL) 
    return FALSE;
 // load the library and find the dictionary:  
  char pathname[MAX_PATH+1];  *pathname=0;  
#ifdef WINS  
  HKEY hKey;    
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Software602\\Dicts\\2.0\\ProgramName", 0, KEY_READ_EX, &hKey)!=ERROR_SUCCESS)  
    return TRUE;  
  DWORD key_len = sizeof(pathname);  
  RegQueryValueEx(hKey, NULLSTRING, NULL, NULL, (BYTE*)pathname, &key_len);  
  if (Load(pathname)) { RegCloseKey(hKey);  return TRUE; }  
  key_len = sizeof(pathname);  
  RegQueryValueEx(hKey, "DataDir", NULL, NULL, (BYTE*)pathname, &key_len);  
  RegCloseKey(hKey);  
#else
 // make it relocatable, for both server (binary) and client (library)
  //sprintf (buf1, "%s/%s", dictnamepath, lpszMainDictName[language]);
  get_path_prefix(pathname);
  strcat(pathname, "/lib/" WB_LINUX_SUBDIR_NAME "/liblemmat.so"); 
  if (Load(pathname)) return TRUE;
  get_path_prefix(pathname);
  strcat(pathname, "/share/" WB_LINUX_SUBDIR_NAME);
#endif  
  strcat(pathname, PATH_SEPARATOR_STR);  
  strcat(pathname, lpszMainDictName[language]);  
 // allocate structure for the language:  
  void * spel = malloc((*lgs_s_sizeof)());  
  if (spel==NULL) return TRUE;  
  (*lgs_s_prepare)(spel);  
  if ((*lgs_l_init)(spel, pathname, 0, LGS_MAIN_PAGES))  
    { free(spel);  return TRUE; } // not init  
 // the language is init:  
  lpSpel[language] = spel;  
  return FALSE;  
}  
  
int C_LGS_94 :: GetLemma(int language, LPSTR lemma, LPSTR word, short len)  
{ ProtectedByCriticalSection cs(&lem_cs);  
  return (*lgs_l_find)(lpSpel[language], lemma, word, len);  
}  

BOOL init_lemma(int language) // returns TRUE iff OK  
{ return !lgs_94.InitLemm(language); }  
  
void lemmatize(int language, const char * input, char * output, int output_bufsize)  
// Supposes that init_lemma has been called and it has successfull  
{ 
#if defined(WINS) && defined(SRVR)
  __try
#endif
  { if (!lgs_94.GetLemma(language, output, (tptr)input, output_bufsize))  
      strcpy(output, input);  
  }    
#if defined(WINS) && defined(SRVR)
  __except ((GetExceptionCode() != 0) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
  { char buf[200];
    sprintf(buf, ">> Exception %x occured in lemmatisation of %s.", GetExceptionCode(), input);
    dbg_err(buf);
  }
#endif
}  
#endif

//////////////////////////////// general scan ////////////////////////////////  
// "template" parametrized by t_ft_kontext:  
#ifdef SRVR  
typedef t_docid T_DOCID;  
#else  
typedef t_docid64 T_DOCID;  
#endif  
  
struct t_ft_kontext_base  
{  
  t_specif word_specif;  // used by is_v9_system only  
  char * word_starters;  // must not be NULL!
  char * word_insiders;  // must not be NULL!
  char * limits_value;
  bool bigint_id;  
  virtual BOOL ft_insert_word_occ(cdp_t cdp, T_DOCID docid, const char * word, unsigned position, unsigned weight, t_specif specif = t_specif()) = 0;  
  virtual void ft_set_summary_item(cdp_t cdp, T_DOCID docid, unsigned infotype, const void * info) = 0;  
  virtual void ft_starting_document_part(cdp_t cdp, T_DOCID docid, unsigned position, const char * relative_path) = 0;  
  virtual bool breaked(cdp_t cdp) const = 0;  
  int ft_upcase(char * word) const;  
  bool FT9_WORD_STARTER(unsigned char c, int charset) const;  
  bool FT9_WORD_CONTIN (unsigned char c, int charset) const;  
  void set_limits_value(const char *new_limits_value);
  t_ft_kontext_base(const char *limits_valueIn, const char * word_startersIn, const char * word_insidersIn, bool bigint_idIn)
  { 
    limits_value=NULL; set_limits_value(limits_valueIn);
    if (!word_startersIn) word_startersIn="_@0123456789";
    word_starters = new char[strlen(word_startersIn)+1];
    strcpy(word_starters, word_startersIn);
    if (!word_insidersIn) word_insidersIn="_@0123456789-";
    word_insiders = new char[strlen(word_insidersIn)+1];
    strcpy(word_insiders, word_insidersIn);
    bigint_id=bigint_idIn;
  }  
  ~t_ft_kontext_base(void)
  { if (word_starters!=NULL) delete[] word_starters; 
    if (word_insiders!=NULL) delete[] word_insiders; 
    if (limits_value!=NULL ) delete[] limits_value; 
  }  
};  
  
int t_ft_kontext_base::ft_upcase(char * word) const  
// *word !=0 supposed!  
{ int flags;  bool allupper;    
  char c=*word, uc=upcase_charA(c, word_specif.charset);  
  if (uc==c) { flags=FTFLAG_FIRSTUPPER;   allupper=true; }  
  else       { flags=0;                   allupper=false;  *word=uc; }  
  do  
  { word++;  
    c=*word;    
    if (!c)   
      return allupper ? flags|FTFLAG_ALLUPPER : flags;    
    uc=upcase_charA(c, word_specif.charset);  
    if (uc!=c) { *word=uc;  allupper=false; }  
  } while (true);  
}  
  
void t_ft_kontext_base::set_limits_value(const char *new_limits_value)
{
  if( limits_value!=NULL ) delete[] limits_value;
  if( new_limits_value==NULL ) limits_value=NULL;
  else
  {
    limits_value=new char[strlen(new_limits_value)+1];
    if( limits_value!=NULL ) strcpy(limits_value,new_limits_value);
  }
}

///////////////////////////////////// classification of characters ////////////////////////////////////////////  
inline bool is_national_letter(unsigned char c, int charset)  
{ 
#ifdef STOP  // does not support ISO encoding
  if (charset==1 || charset==2)  // CE  
    return c>=0xc0 && c<=0xfe && c!=0xd7 && c!=0xf7 ||   
           c==0x8a || c>=0x8c && c<=0x8f || c==0x9a || c>=0x9c && c<=0x9f ||  
           c==0xa3 || c==0xa5 || c==0xaa || c==0xaf || c==0xb3 || c==0xb9 || c==0xba || c==0xbc || c==0xbe || c==0xbf;  
  else  // Western  
    return c>=0xc0 /*&& c<=0xff*/ && c!=0xd7 && c!=0xf7 ||   
           c==0x83 || c==0x8a || c==0x8c || c==0x8e || c==0x9a || c==0x9c || c==0x9e || c==0x9f ||  
           c==0xaa || c==0xba;  
#else
  return wbcharset_t(charset).ascii_table()[c] != '_';
#endif
}  
  
bool t_ft_kontext_base::FT9_WORD_STARTER(unsigned char c, int charset) const  
{ if (c>='a' && c<='z' || c>='A' && c<='Z') return true;  
  return c>=0x80 && is_national_letter(c,charset) || c && strchr(word_starters, c);  
}  
  
bool t_ft_kontext_base::FT9_WORD_CONTIN(unsigned char c, int charset) const  
{ if (c>='a' && c<='z' || c>='A' && c<='Z') return true;  
  return c>=0x80 && is_national_letter(c,charset) || c && strchr(word_insiders, c);  
}  
  
  
  
class t_agent_base  
{protected:  
  cdp_t cdp;  
  unsigned position;  
 public:  
  char word[2*MAX_FT_WORD_LEN+1];  // may be in UTF-8  
  t_agent_base(cdp_t cdpIn)   
    { cdp=cdpIn;  *word=0;  position=0; }  
  virtual bool word_completed(bool is_a_word = true) = 0;  // when false is returned, no more words are necessary  
  virtual void summary_item(int par1, void * par2)   
    { }    
  virtual void starting_document_part(const char * relative_path)  
    { }  
  virtual t_specif get_ft_specif(void) const = 0;  
};  
  
class t_agent_indexing : public t_agent_base  
{ T_DOCID docid;  
  unsigned weight;  
  t_ft_kontext_base * ftk;  
 public:  
  t_agent_indexing(cdp_t cdpIn,  T_DOCID docidIn, t_ft_kontext_base * ftkIn) : t_agent_base(cdpIn)  
    { docid=docidIn;  weight=0;  ftk=ftkIn; }  
  bool word_completed(bool is_a_word = true);    
  void summary_item(int par1, void * par2);    
  void starting_document_part(const char * relative_path);  
  t_specif get_ft_specif(void) const   
    { return ftk->word_specif; }  
};  
  
bool t_agent_indexing::word_completed(bool is_a_word)  
{   
  if (is_a_word)  
    ftk->ft_insert_word_occ(cdp, docid, word, position++, weight, ftk->word_specif);  
  return !ftk->breaked(cdp);    
}  
  
void t_agent_indexing::summary_item(int par1, void * par2)  
{   
  ftk->ft_set_summary_item(cdp, docid, par1, par2);  
}  
  
void t_agent_indexing::starting_document_part(const char * relative_path)  
{   
  ftk->ft_starting_document_part(cdp, docid, position, relative_path);  
}  
  
T_CC97ARG agentConvert97Callback(DOC_ID lpData, CONVCMDS iMessage, T_CC97ARG argA, T_CC97ARG argB, T_CC97ARG argC, T_CC97ARG argD)  
{ t_agent_base * agent_data = (t_agent_base*)lpData;  
  switch (iMessage)  
  {  
    case C97_INS_TEXT:  
    { const char * str = (LPSTR)(LPVOID)argA;  
      unsigned size = (WORD)argB;  
      unsigned char c;  int i;  
     // check for continuation of the previous word:  
      if (*agent_data->word && size)  
      { c=*str;    
        if (FT_WORD_CONTIN(c))  
        { i=(int)strlen(agent_data->word);  str++;  size--; }  
        else  
        { agent_data->word_completed();  
          i=0;  
        }  
      }  
      else i=0;  
     // browse the words:  
      do  
      {   
       // skip leading delimiters, unless continuing the word:  
        if (!i)  
        { do  
          {  
		  if (!size) { *agent_data->word=0;  goto eop; }  
            c=*str;  str++;  size--;  
          } while (!FT_WORD_STARTER(c));  
          i=0;  
        }  
       // read the word:  
        do  
        { if (i<MAX_FT_WORD_LEN) agent_data->word[i++]=c;  
          if (!size) {agent_data->word[i]=0;  goto eop; } // the word may continue in the next part  
          c=*str;  str++;  size--;  
        } while (FT_WORD_CONTIN(c));  
        agent_data->word[i]=0;  
        agent_data->word_completed();  
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
      { agent_data->word_completed();  
        *agent_data->word=0;  
      }  
      break;  
    case C97_INS_HARDHYPHEN:  
      if (*agent_data->word && strlen(agent_data->word)<MAX_FT_WORD_LEN)  
        strcat(agent_data->word, "-");  
      break;  
    case C97_SET_SUMMARY_ITEM:  
      agent_data->summary_item((int)argA, (void *)argB);  
      break;  
#ifdef WINS  
    case C97_ACQUIRE_CONFIG_PARAMS:  
    { static TConfigParams   cp;  
      cp.flags.bIgnore602Contents = TRUE;  
      cp.flags.bIgnoreObjects = TRUE;  
      return &cp;  
    }  
  // další položky odstøelí objekty  
   case C97_CREATE_OLE_OBJ:  
   {OLEOBJDEF ood = *(OLEOBJDEF*)(LPVOID)argA;  
    if (ood.hMeta != 0) DeleteMetaFile((HMETAFILE)ood.hMeta);  
    else  
     if (ood.hEnhMeta != 0) DeleteEnhMetaFile((HENHMETAFILE)ood.hEnhMeta);  
    break;  
   }  
   case C97_CREATE_OBJ:  
   {/* TP - FIX - od.linkData.pic.pData neukazuje na strukturu METAFILEPIC  
    OBJDEF od;  
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
      } */  
      OBJDEF *od = (OBJDEF*)(LPVOID)argA;  
      switch (od->linkData.linkType)  
       {  
       case tMetafile:  
         if (od->linkData.pic.pData != NULL)  
         {if (od->linkData.wFlags != 0  &&  od->linkData.wFlags != LFLAG_RUBBER)  
           DeleteEnhMetaFile((HENHMETAFILE)od->linkData.pic.pData);  
          else  
           DeleteMetaFile((HMETAFILE)od->linkData.pic.pData);  
         }  
        break;  
       case tJPEG:      case tPNG:      case tGIF:      case tTIFF:      case tCOMPRESSED:      case tDIB:  
        if (od->linkData.pic.pData != NULL) GlobalFree(od->linkData.pic.pData);  
       break;  
       }  
    break;  
   }  
#endif  
  }  
 return 0;  
};  
  
//////////////////////////////// simple words //////////////////////////////////////////////  
static const char * empty_simple_word_list[] = { "A" };  
  
static const char * czech_simple_word_list[] = {  
"A", "ABY", "ALE", "ANEBO", "ANI", "ANIŽ", "ANO", "APOD", "AVŠAK", "A", "AŽ",  "AÈKOLIV",  
"BA", "BEZ", "BUÏ", "BY", "BÝT",   
"CO", "DALŠÍ", "DO", "DOKUD", "DOSUD",   
"HO", "I",      
"JA", "JAK", "JAKO", "JAKÝ", "JE-LI", "JEHO", "JEJICH", "JEJICHŽ", "JEJÍ", "JEJÍMŽ", "JEJÍŽ", "JEN", "JENŽ",   
 "JINAK", "JINÝ", "JIŽ", "JÍT",  
"K", "KAŽDÝ", "KDE", "KDO", "KDY", "KDYBY", "KDYŽ", "KE", "KTERÝ", "KU",  
"LEDA", "LZE",  
"MIMO", "MÉ", "MÍT", // "MEZI"->MEZ  
"NA", "NE", "NEBO", "NEBOLI", "NEBÝT", "NEŽ", "NICMÉNÌ", "NIKOLI", "NO",   
"O", "OD", "OKOLO", "ON", "OPROTI",   
"PAK", "PO", "POD", "POKUD", "PRO", "PROTO", "PØE", "PØED", "PØES", "PØESTO", "PØESTOŽE", "PØEVÁŽNÌ", "PØITOM", // "PØI"->PØE  
"S", "SE", "SEBE", "SICE", "STEJNÝ", "SVÉ", "SÁM",   
"TAK", "TAKTO", "TAKÉ", "TAM", "TATA", "TEDY", "TEN", "TENTO", "TJ",  
 "TOTIŽ", "TU", "TUTEN", "TY", "TZN", "TÝŽ",   
"U", "UVNITØ", "UŽ",  
"V", "VE", "VY", "VŠAK", "VŽDY",  
"Z", "ZA", "ZDA", "ZDE", "ZE", "ZEJMÉNA",  
"ŽE", "ÈI", "ÈÍ"  
};  

#if WBVERS>=1100
static const char * english_simple_word_list[] = {  
"A","AN","AND","ARE","AS","AT","BE","BUT","BY","DO","FOR","FROM","HE","HER","HIM","HIS","HOW",
"I","IF","IN","INTO","IS","IT","ITS""MY","NO","NOT","OF","ON","OR","OUT","SHE","SO",
"THAN","THAT","THE","THEIR","THEM","THEN","THERE","THESE","THEY","THIS","TO","UP",
"WAS","WE","WERE","WHAT","WHEN","WHICH","WITH","WOULD","YOU","YOUR" };
  
static const char * german_simple_word_list[] = { 
"ALS","AN","AUF","AUS","BEI","DAS","DASS","DEM","DEN","DENN","DER","DES","DIE","DU","DURCH",
"ER","ES","EUCH","FÜR","GEGEN","ICH","IHR","IN","MIT","NACH","NICHT","OB""ODER","OHNE","SICH","SIE",
"UM","UND","UNS","UNTER","VON","VOR","WIR","ZU","ÜBER" }; 
  
static const char * slovak_simple_word_list[] = {  "A" };

#else // for compatibility in GW etc.
#define english_simple_word_list empty_simple_word_list
#define german_simple_word_list  empty_simple_word_list
#define slovak_simple_word_list  empty_simple_word_list
#endif
  
struct t_word_list_descr  
{ const char ** word_list;  
  unsigned word_list_size;  
};  
  
const t_word_list_descr word_lists[] = {  
 { czech_simple_word_list,   sizeof(czech_simple_word_list)   / sizeof(char*) },  
 { slovak_simple_word_list,  sizeof(slovak_simple_word_list)  / sizeof(char*) },  
 { german_simple_word_list,  sizeof(german_simple_word_list)  / sizeof(char*) },  
 { english_simple_word_list, sizeof(english_simple_word_list) / sizeof(char*) },  
 { english_simple_word_list, sizeof(english_simple_word_list) / sizeof(char*) },  
 { empty_simple_word_list, sizeof(empty_simple_word_list) / sizeof(char*) },  
 { empty_simple_word_list, sizeof(empty_simple_word_list) / sizeof(char*) },  
 { empty_simple_word_list, sizeof(empty_simple_word_list) / sizeof(char*) },  
 { empty_simple_word_list, sizeof(empty_simple_word_list) / sizeof(char*) },  
 { empty_simple_word_list, sizeof(empty_simple_word_list) / sizeof(char*) },  
 { empty_simple_word_list, sizeof(empty_simple_word_list) / sizeof(char*) }  
};  
  
CFNC DllKernel BOOL WINAPI simple_word(const char * word, int language)  
// "simple words" are not inserted into the word table / reference table, but they are counted in the position counter.  
{ const char ** word_list = word_lists[language].word_list;   
  int count = word_lists[language].word_list_size;  
  //return FALSE;  
  int left = 0, right = count, midd;  
  while (left+1<right)  
  { midd = (left+right) / 2;  
    int res = strcmp(word, word_list[midd]);  
    if (!res) return TRUE;  
    if (res < 0) right=midd;  
    else         left=midd;  
  }  
  return !strcmp(word, word_list[left]);  
}  
  
////////////////////////////// support routines /////////////////////////////////  
#ifdef SRVR  
#define sys2_Upcase(cdp,a) sys_Upcase(a)  
#else  
#define sys2_Upcase(cdp,a) Upcase9(cdp,a)  
#endif  
  
static BOOL analyse_ft_label(cdp_t cdp, const char * ft_label, tobjname schemaname, tobjname suffix, tobjname doctabname)  
// Extracts schema name and suffix from the fulltext label.  
// Returns empty schema name or suffix if any is not specified.  
{ const char * p = ft_label;  
  strcpy(doctabname, "FTX_DOCTAB");  
  while (*p && *p!='.') p++;  
  if (p-ft_label > OBJ_NAME_LEN) return FALSE;   
  memcpy(schemaname, ft_label, p-ft_label);  schemaname[p-ft_label]=0;  
  cutspaces(schemaname);  sys2_Upcase(cdp, schemaname);  
  if (*p) // a dot found  
  { p++;  while (*p==' ') p++;  
    ft_label=p;  // new start  
    while (*p && *p!='.') p++;  
    if (p-ft_label > FT_MAX_SUFFIX_LEN) return FALSE;   
    memcpy(suffix, ft_label, p-ft_label);  suffix[p-ft_label]=0;  
    cutspaces(suffix);  sys2_Upcase(cdp, suffix);  
    strcat(doctabname, suffix);  
    if (*p) // a 2nd dot found  
    { p++;  while (*p==' ') p++;  
      if (strlen(p) > OBJ_NAME_LEN) return FALSE;   
      strcpy(doctabname, p);  
      cutspaces(doctabname);  sys2_Upcase(cdp, doctabname);  
    }  
  }  
  else *suffix=0;  
  return TRUE;  
}  
  
#ifdef STOP  
static void get_format_library(const char * text, const char * format, char * libname)  
{ if (!*format || !stricmp(format, "PLAIN") || !stricmp(format, "TEXT"))  
    *libname=0;  
  else if (!stricmp(format, "RTF") || !stricmp(format, ".RTF"))  
    strcpy(libname, "WB_XRTF");    
  else if (!stricmp(format, "DOC") || !stricmp(format, ".DOC"))  
    strcpy(libname, "WB_XW97");    
  else if (!stricmp(format, "WPD"))  
    strcpy(libname, "WB_XWTX");  
  else if (!stricmp(format, "HTM") || !stricmp(format, "HTML"))  
    strcpy(libname, "WB_XHTML");  
  else if (!stricmp(format, "BYSUFFIX"))  
  { int len=strlen(text);  
    *libname=0;  
    if (len>4)  
      if      (!stricmp(text+len-4, ".RTF"))  
        strcpy(libname, "WB_XRTF");  
      else if (!stricmp(text+len-4, ".DOC"))  
        strcpy(libname, "WB_XW97");  
      else if (!stricmp(text+len-4, ".WPD"))  
        strcpy(libname, "WB_XWTX");  
      else if (!stricmp(text+len-4, ".HTM") || len>5 && !stricmp(text+len-5, ".HTML"))  
        strcpy(libname, "WB_XHTML");  
    if (!*libname)  
      if      (len>2 && text[len-2]=='.')  
      { strcpy(libname, "WB_X");  strcpy(libname+4, text+len-1); }  
      else if (len>3 && text[len-3]=='.')  
      { strcpy(libname, "WB_X");  strcpy(libname+4, text+len-2); }  
      else if (len>4 && text[len-4]=='.')  
      { strcpy(libname, "WB_X");  strcpy(libname+4, text+len-3); }  
      else if (len>5 && text[len-5]=='.')  
      { strcpy(libname, "WB_X");  strcpy(libname+4, text+len-4); }  
      else strcpy(libname, "UNDEFSUF");  
  }  
  else   
  { strcpy(libname, "WB_X");  strmaxcpy(libname+4, format, 8+1-4); }  
}  
#endif  
  
#ifdef WINS  
static IMPORTFILE * get_conversion_routine(const char * libname, HINSTANCE & hXDll)  
{ char pathname[MAX_PATH];  HKEY hKey;    
 // find library location:  
  *pathname=0;  
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Software602", 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS)  
  { DWORD key_len = sizeof(pathname);  
    RegQueryValueEx(hKey, "CommonFilesDir", NULL, NULL, (BYTE*)pathname, &key_len);  
    RegCloseKey(hKey);  
  }  
  if (*pathname) strcat(pathname, PATH_SEPARATOR_STR);  
  strcat(pathname, libname);  strcat(pathname, ".DLL");  
 // load it:  
  hXDll=LoadLibrary(pathname);  
  if (!hXDll) return NULL;  
  return (IMPORTFILE*)GetProcAddress(hXDll, IMPORTFILE_FNCNAME);  
}  
  
#ifdef SRVR  
static IMPORTFILEEX * get_conversion_routine_ex(const char * libname, HINSTANCE & hXDll)  
{ char pathname[MAX_PATH];  HKEY hKey;    
 // find library location:  
  *pathname=0;  
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Software602", 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS)  
  { DWORD key_len = sizeof(pathname);  
    RegQueryValueEx(hKey, "CommonFilesDir", NULL, NULL, (BYTE*)pathname, &key_len);  
    RegCloseKey(hKey);  
  }  
  if (*pathname) strcat(pathname, PATH_SEPARATOR_STR);  
  strcat(pathname, libname);  strcat(pathname, ".DLL");  
 // load it:  
  hXDll=LoadLibrary(pathname);  
  if (!hXDll) return NULL;  
  return (IMPORTFILEEX*)GetProcAddress(hXDll, IMPORTFILEEX_FNCNAME);  
}  
#endif  
  
#endif  
#ifdef LINUX  
typedef enum {html_end, html_text, html_encoding} tokentype_t;  
  
#include <ctype.h>  
  
/* Pokud je na vstupu !-- , nacti komentar a vrat true;  
 * jinak vrat false a uved vstup alespon castecne do puvodniho stavu  
 * Pokud se dojde na konec souboru, vrat take true. */  
  
static int comment_to_omit(FILE *in)  
{  
	int c;  
	static const char prolog[]="!--";  
	const char *idx=prolog;  
	while (*idx){  
		c=fgetc(in);  
		if (c!=*idx++){  
			ungetc(c, in);  
			return 0;  
		}  
	}  
	for (;;){  
		while (c=fgetc(in), c!='-' && c!=EOF);  
		if (c==EOF) return 1;  
		c=fgetc(in);  
		if (c=='-') break;  
	}  
	while (c=fgetc(in), c!='>' && c!=EOF);  
	return 1;  
}  
  
/* Read from input file into buffer text or encoding; return type of what have  
 * been read. Ignore comments and other tags. */  
tokentype_t get_html_token(FILE *in, char * buffer, size_t *tokensize, size_t bufsize)  
{  
	int c;  
	*tokensize=0;  
	while (EOF!=(c=fgetc(in))){  
		if (c=='<'){  
			if (comment_to_omit(in)) continue;  
			while(1) {  
				if (0!=*tokensize){  
					ungetc(c, in);  
					return html_text;  
				}  
				char aux[256];  
				int comment;  
				int contenttype=0; /* 0 - nic; 1- meta; 2- http-equiv=Content-type; 3-vse */  
				c=fgetc(in);  
				if (c=='!' && (c=fgetc(in))=='-' && (c=fgetc(in))=='-') comment=1;  
				else ungetc(c, in);  
				if (c==EOF) return html_end;  
				while (c!='>'&& c!=EOF){  
					char *cp;  
					fscanf(in, "%256[^ \r\n\t>]", aux);  
					c=fgetc(in);  
					if (0==strcasecmp(aux, "meta")){  
						contenttype=1;  
						continue;  
					}  
					if (!contenttype) continue;  
					for (cp=aux; ; ){  
						char*ptr=cp;  
						cp=strchr(ptr, 'C');  
						if (!cp) cp= strchr(ptr, 'c');  
						if (!cp) break;  
					       	if (strncasecmp(cp++, "charset=", 8)) continue;  
						cp+=7;  
						if (*cp=='\'' || *cp=='\"') cp++;  
						const char *start=cp;  
						while (*cp!=0 && !isspace(*cp) && *cp!='\"' && *cp !='\'') cp++;  
						int len=cp-start;  
						strncpy(buffer, start, len);  
						buffer[len]=0;  
						contenttype=3;  
						break;  
					}  
				}  
				if (contenttype==3) {  
					return html_encoding;  
				}  
				break;  
			}  
		}  
		else if (c=='&') {  
				char aux[256];  
				fscanf(in, "%256[^ ;\r\n\t]", aux);  
				if (';'!=(c=fgetc(in))) ungetc(c, in);  
		}  
		else {  
			if (isspace(c) && *tokensize==0) continue;  
			(*tokensize)++;  
			*buffer++=c;  
			if (*tokensize==bufsize) return html_text;  
		}  
	}  
	if (0!=*tokensize) return html_text;  
       	else return html_end;  
}  
  
#include <iconv.h>  
int convert_HTML_file(void *agent_data, const char *filename, CONVERTCALLBACK97 callback)  
{  
	FILE *in;  
	in=fopen(filename, "r");  
	if (NULL==in){  
		perror(filename);  
		return TRUE;  
	}  
	char buffer[4096]; char cbuffer[4096];  
	size_t buflen, cbuflen;  
	char *inbuf, *outbuf;  
	tokentype_t tokentype;  
	iconv_t iconv_specif=iconv_open("cp1250", "cp1250");  
	if (iconv_specif==(iconv_t)-1){  
		perror("iconv");  
		fclose(in);  
		return TRUE;  
	}  
	while (html_end!=(tokentype=get_html_token(in, buffer, &buflen, sizeof(buffer)-2)))  
		switch (tokentype){  
		case html_text:  
			inbuf=buffer; outbuf=cbuffer;  
			cbuflen=sizeof(cbuffer);  
			if (-1==iconv(iconv_specif, &inbuf, &buflen, &outbuf, &cbuflen)){  
				perror("iconv");  
				continue;  
			}  
			*(outbuf++)=' '; /* Aby agent nespojoval slova */  
			callback(agent_data, C97_INS_TEXT,  
				(T_CC97ARG*)cbuffer,  
				(T_CC97ARG)(WORD)(outbuf-cbuffer),  
				0, 0);  
			break;  
		case html_encoding:  
			iconv_close(iconv_specif);  
			iconv_specif=iconv_open("cp1250", buffer);  
			break;  
		default:   
			abort();  
			break;  
		}  
	iconv_close(iconv_specif);  
	fclose(in);  
	return 0;  
}  
#endif  
  
////////////////////////////////// LIMITS analysis ////////////////////////////////////  
/* class t_ft_limits_evaluator {{{*/  
/* struct t_ft_limits_item {{{  
 * describes one item of LIMITS parameter of the fulltext system */  
struct t_ft_limits_item  
{  
  char *fname_pattern,  
    /* ext_pattern  
     * NULL means extension have to be empty (i.e. "filename." file) */  
    *ext_pattern;  
  int filesize;  
  t_ft_limits_item() { fname_pattern=ext_pattern=NULL; filesize=0; };  
  ~t_ft_limits_item() { destroy(); };  
  void destroy() { filesize=0; if( fname_pattern!=NULL ) { delete[] fname_pattern; fname_pattern=NULL; } if( ext_pattern!=NULL ) { delete[] ext_pattern; ext_pattern=NULL; } };  
  /* parses LIMITS item, returns true on success, false on error  
   * ptr points to definition of LIMITS item */  
  bool parse_limits_item(char *ptr);  
  /* tests if file with filename and filesize conforms to this LIMITS item  
   * returns true on successfull test (and in *result returns result of the test), false on error  
   * the result of the test is:  
   *   0 ... file doesn't conform to filename and/or extension pattern  
   *   1 ... file conforms to patterns and its size is smaller or equal to filesize  
   *   -1 .. file conforms to patterns and its size is greater than filesize */  
  bool test_file(const char *filename,int filesize,int charset,int *result);  
private:  
  /* unescapes chars \, and \= in string str */  
  void unescape_string(char *str);  
};  
void t_ft_limits_item::unescape_string(char *str) /*{{{*/  
{  
  char *tmp=str;  
  while( *tmp!='\0' )  
  {  
    if( tmp[0]=='\\' && tmp[1]=='\\' ) tmp+=2; // \\ is unescaped by like_esc()  
    else if( tmp[0]=='\\' && (tmp[1]=='=' || tmp[1]==',') )  
    {  
      memmove(tmp,tmp+1,strlen(tmp)*sizeof(char)); // unescape  
      tmp++;  
    }  
    else tmp++;  
  }  
} /*}}}*/  
bool t_ft_limits_item::parse_limits_item(char *ptr) /*{{{*/  
{  
  destroy();  
  // find = and ,  
  char *eq_index=NULL;  
  char *comma_index=NULL;  
  char *tmp=ptr;  
  if( *tmp=='=' ) return false; // ERROR: unexpected =  
  if( *tmp==',' ) return false; // ERROR: unexpected =  
  while( *tmp!='\0' && (eq_index==NULL || comma_index==NULL) )  
  {  
    if( eq_index==NULL && tmp[0]=='=' && tmp[-1]!='\\' ) eq_index=tmp;  
    if( comma_index==NULL && tmp[0]==',' && tmp[-1]!='\\' ) comma_index=tmp;  
    tmp++;  
  }  
  if( eq_index==NULL || (comma_index!=NULL && comma_index<eq_index) ) return false; // = is missing  
  // find .  
  *eq_index='\0'; // temporary set to zero  
  char *dot_index=strrchr(ptr,'.');  
  *eq_index='='; // set back to =  
  // compute sizes of fname and ext patters (including terminating null)  
  size_t fname_size=((dot_index==NULL)?eq_index:dot_index)-ptr+1;  
  size_t ext_size=(dot_index==NULL)?0:eq_index-dot_index;  
  // allocate buffers  
  fname_pattern=new char[fname_size];  
  if( fname_pattern==NULL ) { return false; } // ERROR: out of memory  
  if( ext_size>1 )  
  {  
    ext_pattern=new char[ext_size];  
    if( ext_pattern==NULL ) { return false; } // ERROR: out of memory  
  }  
  else if( dot_index==NULL ) // extension can be arbitrary  
  {  
    ext_pattern=new char[2];  
    if( ext_pattern==NULL ) { return false; } // ERROR: out of memory  
    strcpy(ext_pattern,"%");  
  }  
  else ext_pattern=NULL; // extension have to be empty  
  // copy values  
  strncpy(fname_pattern,ptr,fname_size-1); fname_pattern[fname_size-1]='\0';  
  unescape_string(fname_pattern);  
  if( ext_size>1 ) { strncpy(ext_pattern,dot_index+1,ext_size-1); ext_pattern[ext_size-1]='\0'; unescape_string(ext_pattern); }  
  filesize=atoi(eq_index+1);  
  return true;  
} /*}}}*/  
bool t_ft_limits_item::test_file(const char *filename,int filesize,int charset,int *result) /*{{{*/  
{  
  char *tmp_filename=new char[strlen(filename)+1];  
  if( tmp_filename==NULL ) return false; // ERROR: out of memory  
  strcpy(tmp_filename,filename);  
  char *dot_index=strrchr(tmp_filename,'.');  
  if( dot_index!=NULL ) *dot_index='\0';  
  *result=0;  
  // test fname  
  if( like_esc(tmp_filename,fname_pattern,'\\',true/*ignore case*/, charset))  
  { // fname is OK, test ext now  
    if( (ext_pattern==NULL && (dot_index==NULL || dot_index[1]=='\0'/*dot is at the end of filename*/))  
      ||(ext_pattern!=NULL && (  
          (dot_index==NULL&&ext_pattern[0]=='%'&&ext_pattern[1]=='\0') /* filename doesn't contain dot and ext_pattern is "%" */  
          ||(dot_index!=NULL&&like_esc(dot_index+1,ext_pattern,'\\',true/*ignore case*/,charset))))  
      )  
    { // ext is OK, test filesize now  
      *result=(filesize<=this->filesize)?1:-1;  
    }  
  }  
  delete[] tmp_filename;  
  return true;  
} /*}}}*/  
/*}}}*/  
/* represents LIMITS constrain of fulltext system  
 * LIMITS value is  
 *   ::= limits_item {, limits_item }...  
 *   limits_item ::= filename_pattern[.extension_pattern]=filesize_value  
 *   filename_pattern and extension_pattern is pattern formed by rules of SQL LIKE operator  
 *     that is escaped by \ character  
 *     pattern can contain following escaped characters: \\ \% \_  ... like LIKE operator  
 *                                                       \= \,     ... LIMITS-specific  
 * USAGE:  
 *   construct class instance, pass LIMITS value as limits_value parameter of constructor  
 *   use test_file() method to test its conformity to LIMITS value */  
class t_ft_limits_evaluator  
{  
  char *limits;  
  t_ft_limits_item *items;  
  int items_count;  
  
  /* parses LIMITS definition in this->limits into this->items array of LIMITS items  
   * returns true on success, false on error  
   * when LIMITS is already parsed (i.e. is_limits_parsed()==true) does nothing and simply returns true */  
  bool parse_limits();  
  void clear_items();  
  inline bool is_limits_parsed() const { return limits==NULL || items_count>0; }  
public:  
  t_ft_limits_evaluator(cdp_t cdp, const char *limits_value);  
  virtual ~t_ft_limits_evaluator();  
  
  /* sets new LIMITS value  
   * returns true on success or false on error  
   * this method have to be called after modification of the fulltext system definition */  
  bool set_limits(const char *limits_value);  
  /* tests if file with filename and filesize conforms to limits  
   * writes result into memory that result parameter points to  
   * returns true on success, false on error (invalid limits format or out of memory etc.)  
   * result of the test is:  
   *   true when file have to be indexed in fulltext system  
   *   false when file doesn't have to be indexed */  
  bool test_file(const char *filename,int filesize,int charset,bool *result);  
};  
t_ft_limits_evaluator::t_ft_limits_evaluator(cdp_t cdp, const char *limits_value) /*{{{*/  
{  
  if( limits_value==NULL || *limits_value=='\0' ) limits=NULL;  
  else  
  {  
    limits=new char[strlen(limits_value)+1];  
    if( limits!=NULL ) strcpy(limits,limits_value);  
  }  
  items=NULL; items_count=0;  
} /*}}}*/  
t_ft_limits_evaluator::~t_ft_limits_evaluator() /*{{{*/  
{  
  if( limits!=NULL ) delete[] limits;  
  clear_items();  
} /*}}}*/  
bool t_ft_limits_evaluator::set_limits(const char *limits_value) /*{{{*/  
{  
  // clear parsed LIMITS definition  
  clear_items();  
  // free previous LIMITS value  
  if( limits!=NULL ) { delete[] limits; limits=NULL; }  
  // set new LIMITS value  
  if( limits_value==NULL || *limits_value=='\0' ) return true; // and this->limits is NULL  
  limits=new char[strlen(limits_value)+1];  
  if( limits==NULL ) return false; // ERROR: out of memory  
  strcpy(limits,limits_value);  
  return true;  
} /*}}}*/  
bool t_ft_limits_evaluator::test_file(const char *filename,int filesize,int charset,bool *result) /*{{{*/  
{  
  if( !is_limits_parsed() )  
  {  
    if( !parse_limits() ) return false; // ERROR  
  }  
  if( items_count==0 ) { *result=true; return true; } // any file satisfies empty LIMITS  
  *result=false;  
  for( int i=0;i<items_count;i++ )  
  {  
    int partial_result=0;  
    if( !items[i].test_file(filename,filesize,charset,&partial_result) ) { return false; } // ERROR  
    if( partial_result!=0 ) { *result=(partial_result==1); return true; }  
  }  
  *result=true; // file will be indexed when it doesn't conform to any LIMITS item  
  return true;  
} /*}}}*/  
bool t_ft_limits_evaluator::parse_limits() /*{{{*/  
{  
  if( is_limits_parsed() ) return true;  
  int cnt=1;  
  char *ptr=limits;  
  if( *ptr==',' ) return false; // ERROR: unexpected ,  
  // count items  
  while( *ptr!='\0' )  
  {  
    if( *ptr==',' && ptr[-1]!='\\' ) cnt++;  
    ptr++;  
  }  
  // allocate items  
  items=new t_ft_limits_item[cnt];  
  if( items==NULL ) return false; // ERROR: out of memory  
  items_count=cnt;  
  memset(items,0,sizeof(t_ft_limits_item)*cnt);  
  // parse  
  ptr=limits;  
  int index=0;  
  for( ;; )  
  {  
    if( !items[index].parse_limits_item(ptr) ) { return false; }  
    if( index+1==cnt ) break;  
    while( !(*ptr==',' && ptr[-1]!='\\') && *ptr!='\0' ) ptr++;  
    if( *ptr=='\0' ) break;  
    ptr++; // skip ,  
    index++;  
  }  
  return true;  
} /*}}}*/  
void t_ft_limits_evaluator::clear_items() /*{{{*/  
{  
  if( items_count>0 )  
  {  
    for( int i=0;i<items_count;i++ )  
    {  
      items[i].destroy();  
    }  
    delete[] items; items=NULL; items_count=0;  
  }  
} /*}}}*/   
/*}}}*/  
  
CFNC DllKernel bool WINAPI validate_limits(cdp_t cdp, const char * limits)  
{  
  t_ft_limits_evaluator ev(cdp, limits);  
  bool dummy_result;  
  return ev.test_file("dummy",1,0,&dummy_result);  
}  
