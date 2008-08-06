/****************************************************************************/
/* The S-Pascal compiler                                                    */
/****************************************************************************/
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#ifndef WINS
#ifdef NETWARE
#include <process.h>
#endif
#include <stdlib.h>
#endif
#include "odbc.h"
/* Teorie: Standarni procedury a funkce (PF) musi mit parametry o sude velikosti,
zatimco uzivatelem definovane PF nikoli. To se zajistuje pouze pro velikost 1,
ktera se pro std. konvertuje na 2. Zadna standardni PF nema jako parametr
string predavany hodnotou nebo strukturu. Neni podporeno predani databazoveho
objektu do parametru strukturovaneho typu.

Prace s hodnotami: Na RT stacku se vsechny hodnoty reprezentuji stejne jako v databazi
nebo v promennych, konvertuji se az pri prirazeni nebo operacich. Konverze je slozitejsi o to,
ze se museji konvertovat hodnoty NULL.

*/
/*************************** standard objects *******************************/
#include "wstds.c"
static char STR_READ[5] = "READ";
char STR_REPL_DESTIN[] = "REPL_DESTIN";
wbbool empty_index_inside;
static uns8 nesting_counter = 0; // global nesting level of the "compile" function

/*************************** decls ******************************************/

tptr comp_alloc(CIp_t CI, unsigned size)
{ tptr p;
  p=(tptr)corealloc(size, OW_COMPIL);
  if (!p) c_error(OUT_OF_MEMORY);
  CI->any_alloc=wbtrue;
  return p;
}

//#define SQL_STD_COUNT 30 // used for syntax colouring only
static tname sql_std_ids[/*SQL_STD_COUNT*/] = { "ADMIN_MODE", "BIT_LENGTH", "CHARACTER_LENGTH", "CHAR_LENGTH", 
 "CLOSE_SEMAPHORE", "CREATE_SEMAPHORE", "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP", 
 "ENUM_REC_PRIV_SUBJ", "ENUM_TAB_PRIV_SUBJ",
 "EXPORT_TO_XML_CLOB", "EXTRACT", "FREE_DAD_RSCOPE", "FULLTEXT", "GET_DAD_RSCOPE", 
 "HAS_DELETE_PRIVIL", "HAS_GRANT_PRIVIL", "HAS_INSERT_PRIVIL", "HAS_SELECT_PRIVIL", "HAS_UPDATE_PRIVIL",
#if WBVERS>=900
 "IMPORT_FROM_XML_CLOB", 
#else
 "IMPORT_FROM_XML_CL", 
#endif
 "LOWER", "OCTET_LENGTH", "POSITION", 
 "RELEASE_SEMAPHORE", "SET_MEMBERSHIP", "SLEEP",
 "SUBSTRING", "UPPER", "WAIT_FOR_SEMAPHORE", "_SQP_DEFINE_LOG", "_SQP_LOG_WRITE", 
 "_SQP_PROFILE_ALL", "_SQP_PROFILE_LINES",  "_SQP_PROFILE_RESET",  
#if WBVERS>=900
 "_SQP_PROFILE_THREAD", "_SQP_SET_THREAD_NAME",
#else
 "_SQP_PROFILE_THREA", "_SQP_SET_THREAD_NA",
#endif
 "_SQP_TRACE"
 
};

void put_string_char(CIp_t CI, unsigned pos, char c)
// Adds a char into the specified position in the string. Must be explicitly terminated by adding the 0 character.
// String will not be deallocated for each string literal in the source text.
{ char * p = (char *)CI->string_dynar.acc(pos);
  if (p==NULL) c_error(OUT_OF_MEMORY);
  *p=c;
}

kont_descr kont_from_selector;  /* temp. variable for the kontext
    created by the tabcurs[i]. created by selector, used by with-statement */
/*********************** ident. tables **************************************/
CFNC DllKernel object * WINAPI search1(ctptr name, objtable * tb)  /* searches the name in tb */
{ int ind;
  if (search_table(name,(tptr)tb->objects,tb->objnum,sizeof(objdef),&ind))
    return NULL;
  else return tb->objects[ind].descr;
}

object * search_obj(CIp_t CI, ctptr name, uns8 * obj_level)  /* searches the object
  "name" in all tables, returns the object description or NULL if not found */
{ objtable * tb; object * obj;
  tb=CI->id_tables;  *obj_level=0;
  do
  { if ((obj=search1(name, tb))!=NULL) return obj;
    tb=tb->next_table;  (*obj_level)++;
  } while (tb);
  return NULL;
}

CFNC DllKernel int WINAPI check_atr_name(tptr name)
{ int i, res=0;  char uname[ATTRNAMELEN+1];
  if (strlen(name) > ATTRNAMELEN) return 0;
  strcpy(uname, name);
  Upcase(uname);
  if (!search_table(uname, *keynames, KEY_COUNT, sizeof(tname), &i))
    res|=IDCL_IPL_KEY;  /* is an IPL keyword */
  if (!search_table(uname, *sql_keynames, SQL_KEY_COUNT, sizeof(tname), &i))
    res|=IDCL_SQL_KEY;  /* is an SQL keyword */
  if (!search_table(uname,(tptr)standard_table.objects,
                     standard_table.objnum,sizeof(objdef),&i))
    res|=IDCL_STDID;  /* is standard identifier */
  if (!search_table(uname, *sql_std_ids, sizeof(sql_std_ids) / sizeof(tname), sizeof(tname), &i))
    res|=IDCL_SQL_STDID;
  if (res) return res;
#ifdef WINS
  if (is_any_key(S_IDENT, uname)) return IDCL_VIEW;  /* id used by view description lang. */
#endif
  if (!strcmp(uname, "INDEX") || !strcmp(uname, "ATTRIBUTE"))
    return IDCL_VIEW;
  return 0;    /* not predefined */
}

CFNC DllKernel void WINAPI make_safe_ident(const char * pattern, char * ident, int maxlen)
// Creates an identifier similiar to [pattern] in [ident]. [ident] must have space for [maxlen]+1 bytes.
// The resulting identifier contains only ASCII uppercase letters, digits and _, does not start with a digit and is not an SQL keyword.
{ char * pident=ident;
  if (*pattern>='0' && *pattern<='9')
    { *pident='_';  pident++;  maxlen--; }
  while (*pattern && maxlen>0)
  { if (*pattern>='0' && *pattern<='9' || *pattern>='A' && *pattern<='Z' || *pattern=='_')
      *pident=*pattern;  
    else if (*pattern>='a' && *pattern<='z')
      *pident=*pattern & 0xdf;
    else
      *pident='_';
    pattern++;  pident++;  maxlen--;
  }
  *pident=0;
 // check for conflict with SQL keywords:
  if (check_atr_name(ident) & IDCL_SQL_KEY) 
  { int len = (int)strlen(ident);
    if (maxlen==0) len--;
    memmov(ident+1, ident, len+1);
    *ident='_';
  }
}

#define NON_NULL_TYPEOBJ ((typeobj*)1)

BOOL search_dbobj(CIp_t CI, char * name1, char * name2, char * name3, kont_descr * kd, d_attr * pdatr, uns8 * obj_level, typeobj ** ptobj)
/* searches database object in all kontexts */
/* Writes results into pdatr */
{ *obj_level=0;
  while (kd)
  { if (kd->kontcateg==CATEG_RECCUR) /* cursor described by a record type */
    { objtable * tb;  typeobj * tobj;  int ind;
      if (name2 && *name2 || name3 && *name3) goto next_kontext;
      tb=((typeobj*)kd->record_tobj)->type.record.items;
      if (!search_table(name1,(tptr)tb->objects,tb->objnum,sizeof(objdef),&ind))
      { pdatr->set_num(tb->objects[ind].descr->var.ordr+1);
        pdatr->multi  =tb->objects[ind].descr->var.multielem ? 2 : 1;
        tobj          =tb->objects[ind].descr->var.type;
        if (DIRECT(tobj)) pdatr->type=ATT_TYPE(tobj); /* cannot be string */
        else if (CHAR_ARR(tobj))
        { pdatr->type  =TtoATT(tobj);
          pdatr->specif.length=tobj->type.anyt.valsize-1;
        }
        if (ptobj) *ptobj=tobj;  return TRUE;
      }
    }
    else if (kd->kontcateg==CATEG_TABLE_ALL)
    { int i;  table_all * ta = (table_all*)kd->record_tobj;
      if (name2 && *name2 || name3 && *name3) goto next_kontext;
      for (i=0;  i<ta->attrs.count();  i++)
      { d_attr * ppatr=(d_attr*)ta->attrs.acc(i);
        if (!stricmp(name1, ppatr->name))
        { pdatr->base_copy(ppatr, i);
          if (ptobj) *ptobj=simple_types[pdatr->type];  return TRUE;
        }
      }
    }
    else if (kd->kontcateg==CATEG_D_TABLE)   /* foreign key and ORDER BY and ODBC9 compilation */
    { int i;  d_table * td = (d_table*)kd->record_tobj;
      if (name3 && *name3) strcpy(name1, name3);
      else if (name2 && *name2) strcpy(name1, name2);
      for (i=0;  i<td->attrcnt;  i++)
        if (!stricmp(name1, td->attribs[i].name))
        { pdatr->base_copy(td->attribs+i, i);
          if (ptobj) *ptobj=simple_types[pdatr->type];  return TRUE;
        }
    }
    else if (kd->kontcateg & IS_LINK) /* CATEG_TABLE, CATEG_CURSOR */
    { tcateg cat = kd->kontcateg & CATEG_MASK;
      tobjnum objnum;
      objnum=kd->kontobj;
      if (name2 && *name2)  /* check the kontext names */
        if (name3 && *name3)
        { if (!strcmp(name1,kd->schemaname) && !strcmp(name2,kd->tabname))
            if (find_attr(CI->cdp, objnum, cat, name3, NULL, NULL, pdatr))
              goto rights_and_ret;   /* attribute info in pdatr */
        }
        else
        { if (!strcmp(name1,kd->tabname))
            if (find_attr(CI->cdp, objnum, cat, name2, NULL, NULL, pdatr))
              goto rights_and_ret;   /* attribute info in pdatr */
        }
      else
        if (find_attr(CI->cdp, objnum, cat, name1, NULL, NULL, pdatr))
          goto rights_and_ret;   /* attribute info in pdatr */
      goto next_kontext;
    }
    else /* CATEG_TABLE, CATEG_CURSOR, CATEG_DIRCUR: */
    { if (!find_attr(CI->cdp, kd->kontobj, kd->kontcateg, name1, name2, name3, pdatr))
        goto next_kontext;
     rights_and_ret:
      if (ptobj) *ptobj=simple_types[pdatr->type];  return TRUE;
    }
   next_kontext:
    kd=kd->next_kont;  (*obj_level)++;   /* go to the next kontext */
  }
  return FALSE;   /* object not found in any kontext */
}

objtable * adjust_table(CIp_t CI, objtable * tab, unsigned elems)
{ objtable * newtab;
  newtab=(objtable*)corerealloc(tab,sizeof(objtable)+(elems-1)*sizeof(objdef));
  if (!newtab) c_error(OUT_OF_MEMORY);
  newtab->tabsize=(short)elems;
  return newtab;
}
/****************************** scanner *************************************/
#define DBGI_ALLOC_STEP  300

static void generate_dbg_info(CIp_t CI, uns32 linenum, tobjnum objnum, uns32 coffset)
{ uns8 filenum;  dbg_info * new_dbgi;  BOOL overwrite;
  if (linenum==CI->dbgi->lastitem->linenum) return; /* new symbol on the same line */
  overwrite=coffset==CI->dbgi->lastitem->codeoff;
   /* overwrite if no code generated on the prev. line */
  if (CI->dbgi->freecells < 2)  /* 2 cells may be necessary: item & objnum */
  { uns32 newsize=sizeof(dbg_info) + sizeof(lnitem)
       * (uns32)(CI->dbgi->itemcnt+CI->dbgi->filecnt+DBGI_ALLOC_STEP);
    new_dbgi=(dbg_info*)corerealloc(CI->dbgi, newsize);
    if (new_dbgi) CI->dbgi=new_dbgi;  else c_error(OUT_OF_MEMORY);
    CI->dbgi->freecells+=DBGI_ALLOC_STEP;
    CI->dbgi->lastitem=(lnitem*)(CI->dbgi->lnitems()+(CI->dbgi->itemcnt-1));
  }
 /* find or create the file number */
  if (objnum==CI->dbgi->srcobjs(CI->dbgi->lastitem->filenum))
    filenum=CI->dbgi->lastitem->filenum;  /* same filenum as in the last */
  else
  { filenum=0;
    do
    { if (objnum==CI->dbgi->srcobjs(filenum)) break;
      if (++filenum >= CI->dbgi->filecnt)
      { memmove((char *)(CI->dbgi->items+filenum+1),
                (char *)(CI->dbgi->items+filenum),
                sizeof(lnitem) * (uns32)CI->dbgi->itemcnt);
        CI->dbgi->filecnt++;
        CI->dbgi->lastitem=(lnitem*)(CI->dbgi->lnitems()+(CI->dbgi->itemcnt-1));
       /* write the new object number */
        CI->dbgi->set_srcobj(filenum, objnum);
        CI->dbgi->freecells--;
        break;
      }
    } while (TRUE);
  }
 /* filenum is OK now, insert the new item */
  if (!overwrite)
  { CI->dbgi->lastitem=(lnitem *)((lnitem *)CI->dbgi->lastitem+1);
    CI->dbgi->itemcnt++;
    CI->dbgi->freecells--;
  }
  CI->dbgi->lastitem->filenum=filenum;
  CI->dbgi->lastitem->linenum=linenum;
  CI->dbgi->lastitem->codeoff=coffset;
}

unsigned char WINAPI next_char(CIp_t CI)
// compil_ptr points to the character next to curchar, unless curchar is S_SOURCE_END
{reread: 
  CI->curchar=*CI->compil_ptr;
  if (CI->curchar)
  { if (!CI->expansion || CI->expansion->include) // do not count columns & lines inside the macro expansion
      if (CI->curchar=='\n')
      { CI->loc_line++;  CI->total_line++;  CI->loc_col=0; 
        if (!(CI->loc_line % 50) && CI->comp_callback) 
          (*CI->comp_callback)(CI->src_line, CI->total_line, NULL);
      }
      else CI->loc_col++;
    CI->compil_ptr++;
    return CI->curchar;
  }
  else if (CI->expansion) // expanding a macro or compiling include
  { t_expand * exp = CI->expansion;
    exp->restore_state(CI);
    if (exp->include && CI->comp_callback) 
      (*CI->comp_callback)(CI->src_line, CI->total_line, CI->src_name);
    if (CI->defines) CI->defines->del_recurs();
    CI->defines    =exp->other_defines;  // restore the defines
    CI->expansion  =exp->next;
    exp->other_defines=NULL;  delete exp;
    goto reread;
  }
  else return S_SOURCE_END;  /* the delimiter can be read several times */
}

static void get_id(CIp_t CI, char * name)
{ while (CI->curchar==' ') next_char(CI);
  if (!is_AlphaA(CI->curchar, CI->sys_charset) && CI->curchar!='_') 
    c_error(IDENT_EXPECTED);
  int i=0;
  do
  { if (i<NAMELEN) name[i++]=CI->curchar;
    next_char(CI);
  } while (is_AlphanumA(CI->curchar, CI->sys_charset));
  name[i]=0;
  Upcase9(CI->cdp, name);
}

static t_define * find_defined_symbol(CIp_t CI, const char * name)
{ t_define * def=CI->defines;
  t_expand * exp=CI->expansion;
  while (exp)
    { def=exp->other_defines;  exp=exp->next; }
  while (def)
  { if (!strcmp(def->name, name)) return def;
    def=def->next;
  }
  return NULL;
}

static void undefine_symbol(CIp_t CI, const char * name)
{ t_define ** pdef=&CI->defines;
  t_expand * exp=CI->expansion;
  while (exp)
    { pdef=&exp->other_defines;  exp=exp->next; }
  while (*pdef)
  { t_define * def = *pdef;
    if (!strcmp(def->name, name)) 
    { *pdef=def->next;
      delete def;
      break; // undefined
    }
    pdef=&def->next;
  }
}

BOOL eval_bool_expr(CIp_t CI)
{ BOOL eq;  int val1;
  symbol op1 = CI->cursym;
  if (op1==S_INTEGER) val1=(int)CI->curr_int;
  else if (op1==S_STRING || op1==S_CHAR)
    strmaxcpy(CI->cdp->RV.hyperbuf, CI->curr_string(), 256);
  else c_error(BAD_CHAR);
  while (CI->curchar==' ') next_char(CI);
  if (CI->curchar=='=') eq=TRUE;
  else if (CI->curchar=='<')
  { if (next_char(CI)!='>') c_error(BAD_CHAR);
    eq=FALSE;
  }
  else if (CI->curchar=='!')
  { if (next_char(CI)!='=') c_error(BAD_CHAR);
    eq=FALSE;
  }
  else if (op1==S_INTEGER) return CI->curr_int!=0;
  else return *CI->cdp->RV.hyperbuf!=0;
 // operation:
  next_char(CI);
  next_sym(CI);
  symbol op2 = CI->cursym;
  if (op2!=S_INTEGER && op2!=S_STRING && op2!=S_CHAR)
    c_error(BAD_CHAR);
  if (op1!=op2) c_error(INCOMPATIBLE_TYPES);
  if (op1==S_INTEGER)
    return (val1==CI->curr_int)==eq;
  else 
    return !my_stricmp(CI->cdp->RV.hyperbuf, CI->curr_string())==eq;
}

/* Conditional compilation:
  Pro kazdou otevrenou uroven zanoreni je jedna polozka t_condcomp v LIFO seznamu CI->condcomp.
  Jeji state je:
    CCS_ACTIVE     - kdyz se tato vetev preklada,
    CCS_PREACTIVE  - kdyz se tato vetev nepreklada, ale uplatni se dalsi ELIF nebo ELSE,
    CCS_POSTACTIVE - kdyz se tato vetev nepreklada, a ani dalsi vetve na stjne urovni se neuplatni.
  State CCS_POSTACTIVE se nastavi po pruchodu prekladanou vetvi a take pokud v neprekladane vetvi
    vstoupin do nizsi urovne podmineneho prekladu.
  Pokud po zpracovani direktivy jsme v podminene kompilaci a stav neni CCS_ACTIVE, pak se preskakuji
    vstupni symboly az do dalsi direktivy.
  Konec souboru v podminene kompilaci se testuje behem preskakovani a take po uspesnem dobehnuti kompilace.
*/
t_condcomp::t_condcomp(CIp_t CI)
{ startline=CI->src_line;
  next=CI->condcomp;  CI->condcomp=this;
}

/* Macro definitions and expansion:
  Seznam platnych maker je v CI->defines. Pri expanzi makra zalozi nova polozka LIFO seznamu CI->expansion.
  Do ni se ulozi stavajici compil_ptr, univ_source a defines. compil_ptr a univ_source se nastavi
  na expanzi makra, do defines se zada popis nahrazovani formalnich parametru skutecnymi.

  
*/
static void skip_comment(CIp_t CI)
{ while (CI->curchar!='}') 
  { if (CI->curchar=='\0') c_error(EOF_IN_COMMENT);
    if (CI->curchar=='*')
      { if (next_char(CI)=='/') break; }
    else next_char(CI);
  }
  next_char(CI); 
}

static void process_directive(CIp_t CI, BOOL pascal_style)
{ tname dirname;  int i=0;
  while (CI->curchar>='A' && CI->curchar<='Z' || CI->curchar>='a' && CI->curchar<='z')
  { if (i<NAMELEN) dirname[i++]=CI->curchar;  else break;
    next_char(CI);
  }
  dirname[i]=0;  Upcase9(CI->cdp, dirname);
  while (CI->curchar==' ') next_char(CI);
  if (!strcmp(dirname, "DEFINE"))
  { tname name;  get_id(CI, name);
    undefine_symbol(CI, name);
   // create the new define and add it to the top define list
    t_define * def = new t_define;  if (!def) c_error(OUT_OF_MEMORY);
    strcpy(def->name, name);
    //def->next=CI->defines;  CI->defines=def;
    t_define ** pdef=&CI->defines;
    t_expand * exp=CI->expansion;
    while (exp)
      { pdef=&exp->other_defines;  exp=exp->next; }
    def->next=*pdef;  *pdef=def;
   // process the parameters
    t_define ** ppar = &def->params;
    if (CI->curchar=='(')  // macro with parameters
    { next_char(CI);  while (CI->curchar==' ') next_char(CI);
      if (CI->curchar != ')')
      do
      { t_define * par = new t_define;  if (!par) c_error(OUT_OF_MEMORY);
        *ppar=par;  ppar=&par->next;
        get_id(CI, par->name);
        while (CI->curchar==' ') next_char(CI);
        if (CI->curchar!=',') break;
        next_char(CI);
      } while (TRUE);
      if (CI->curchar!=')') c_error(RIGHT_PARENT_EXPECTED);
      next_char(CI);
    }
    *ppar=NULL;
   // macro definition (curchar is the 1st character of the maco expansion): line-comment in the expansion must be removed
    const char * start=CI->compil_ptr-1;
    while (CI->curchar!='\n' && CI->curchar!='\r' && CI->curchar!='}' && CI->curchar!=S_SOURCE_END &&
           (CI->curchar!='/' || *CI->compil_ptr!='/')) 
      next_char(CI);
    int len=(int)(CI->compil_ptr-start-1);
    def->expansion=(tptr)corealloc(len+1+1, 94);
    if (!def->expansion) c_error(OUT_OF_MEMORY);
   // add space on the end of the expansion: the last symbol of the expanstion may be the parameter and it must be followed by another char to be replaced.
    memcpy(def->expansion, start, len);  
    def->expansion[len++]=' ';  def->expansion[len]=0;
  }
  else if (!strcmp(dirname, "UNDEF"))
  { tname name;  get_id(CI, name);
    undefine_symbol(CI, name);
  }
  else if (!strcmp(dirname, "IF"))
  { next_sym(CI);  
    BOOL outer_inactive = CI->condcomp && CI->condcomp->state!=CCS_ACTIVE;
    t_condcomp * cc = new t_condcomp(CI);  // inserts itself to the beginning of the list
    if (!cc) c_error(OUT_OF_MEMORY);
    BOOL res=eval_bool_expr(CI);
    cc->state = outer_inactive ? CCS_POSTACTIVE : res ? CCS_ACTIVE : CCS_PREACTIVE;
  }
  else if (!strcmp(dirname, "IFDEF"))
  { tname name;  get_id(CI, name);
    BOOL outer_inactive = CI->condcomp && CI->condcomp->state!=CCS_ACTIVE;
    t_condcomp * cc = new t_condcomp(CI);  // inserts itself to the beginning of the list
    if (!cc) c_error(OUT_OF_MEMORY);
    cc->state = outer_inactive ? CCS_POSTACTIVE : find_defined_symbol(CI, name) ? CCS_ACTIVE : CCS_PREACTIVE;
  }
  else if (!strcmp(dirname, "IFNDEF"))
  { tname name;  get_id(CI, name);
    BOOL outer_inactive = CI->condcomp && CI->condcomp->state!=CCS_ACTIVE;
    t_condcomp * cc = new t_condcomp(CI);  // inserts itself to the beginning of the list
    if (!cc) c_error(OUT_OF_MEMORY);
    cc->state = outer_inactive ? CCS_POSTACTIVE : find_defined_symbol(CI, name) ? CCS_PREACTIVE : CCS_ACTIVE;
  }
  else if (!strcmp(dirname, "ELIF"))
  { next_sym(CI);  
    t_condcomp * cc = CI->condcomp;
    if (!cc) c_error(UNEXPECTED_DIRECTIVE);
    BOOL res=eval_bool_expr(CI);
    if (cc->state==CCS_ACTIVE) cc->state=CCS_POSTACTIVE;
    else if (cc->state==CCS_PREACTIVE) cc->state=res ? CCS_ACTIVE : CCS_PREACTIVE;
    // CCS_POSTACTIVE remains unchanged
  }
  else if (!strcmp(dirname, "ELSE"))
  { t_condcomp * cc = CI->condcomp;
    if (!cc) c_error(UNEXPECTED_DIRECTIVE);
    if (cc->state==CCS_ACTIVE) cc->state=CCS_POSTACTIVE;
    else if (cc->state==CCS_PREACTIVE) cc->state=CCS_ACTIVE;
    // CCS_POSTACTIVE remains unchanged
  }
  else if (!strcmp(dirname, "ENDIF"))
  { t_condcomp * cc = CI->condcomp;
    if (!cc) c_error(UNEXPECTED_DIRECTIVE);
    CI->condcomp=cc->next;
    delete cc;
  }
  else if (!strcmp(dirname, "SQL"))
  { int i=0;
    do
    { put_string_char(CI, i++, CI->curchar);
      next_char(CI);
    } while (CI->curchar!='\n' && CI->curchar!='\r' && CI->curchar!='}' && CI->curchar!=S_SOURCE_END);
    put_string_char(CI, i, 0); // delimiter
    CI->cursym=S_SQLSTRING;
    if (CI->curchar!=S_SOURCE_END) CI->curchar=';';
    return;
  }
  else if (!strcmp(dirname, "SQLBEGIN"))
  { if (pascal_style) skip_comment(CI);
    ctptr start=CI->compil_ptr-1;
    CI->aux_info=1;
    while (CI->aux_info==1 && CI->cursym!=S_SOURCE_END) next_sym(CI);
    int j;
    for (j=0; start<CI->prepos;  start++, j++)
      put_string_char(CI, j, *start);
    put_string_char(CI, j, 0);
    CI->cursym=S_SQLSTRING;
    if (CI->curchar!=S_SOURCE_END) CI->curchar=';';
    return;
  }
  else if (!strcmp(dirname, "SQLEND"))
  { if (CI->aux_info!=1) c_error(UNEXPECTED_DIRECTIVE);
    CI->aux_info=0;
    if (pascal_style) skip_comment(CI);
    return;  // no return symbol defined
  }
  else if (*dirname=='I')  // may write {$Iname} without space
  {// read the include name and find the object:
    int i=0;  tobjnum srcobj;
   // take the beginning of the name from dirname 
    if (strcmp(dirname, "INCLUDE"))
      { strcpy(CI->curr_name, dirname+1);  i=(int)strlen(dirname+1); }
   // read the rest:
    if (CI->curchar=='`')
    { next_char(CI);
      while (CI->curchar!='`' && i<OBJ_NAME_LEN)
      { CI->curr_name[i++]=CI->curchar;
        next_char(CI);
      }
    }
    else
#if WBVERS<900
      while (SYMCHAR(CI->curchar) && i<OBJ_NAME_LEN)
#else
      while (is_AlphanumA(CI->curchar, CI->sys_charset) && i<OBJ_NAME_LEN)
#endif
      { CI->curr_name[i++]=CI->curchar;
        next_char(CI);
      }
    CI->curr_name[i]=0;
    if (cd_Find_object(CI->cdp, CI->curr_name, CATEG_PGMSRC, &srcobj)) c_error(INCLUDE_NOT_FOUND);
#ifdef WINS
#if WBVERS<900
    if (save_new_source(srcobj)) c_error(INCLUDE_NOT_FOUND);
#endif
#endif
    if (pascal_style) skip_comment(CI);
   // replace source by the new include:
    t_expand * exp = new t_expand;  if (!exp) c_error(OUT_OF_MEMORY);
    exp->other_defines=CI->defines;  CI->defines=NULL;
    exp->save_state(CI, TRUE);
    exp->next=CI->expansion;  CI->expansion=exp;
    CI->univ_source=CI->compil_ptr=exp->source=cd_Load_objdef(CI->cdp, srcobj, CATEG_PGMSRC);
    if (!CI->univ_source) c_error(OUT_OF_MEMORY);
    CI->src_objnum=srcobj;
    add_source_to_list(CI);
    strcpy(CI->src_name, CI->curr_name);
    CI->loc_line=CI->src_line=1;  CI->sc_col=CI->loc_col=0;
    next_char(CI);  next_sym(CI);
    if (CI->comp_callback) (*CI->comp_callback)(CI->src_line, CI->total_line, CI->src_name);
    return; // must not search for } in the include!
  }
  else if (!*dirname) ;
  else c_error(UNDEFINED_DIRECTIVE);
 // skipping the comment following the pascal-style directive:
  if (pascal_style) skip_comment(CI);
 // skipping inactive parts of the source text:
  do 
    if (next_sym(CI)==S_SOURCE_END)
      if (CI->condcomp)
      { char buf[12];  int2str(CI->condcomp->startline, buf);
        SET_ERR_MSG(CI->cdp, buf);  c_error(EOF_IN_CONDITIONAL);
      }
      else break;
  while (CI->condcomp && CI->condcomp->state!=CCS_ACTIVE);
}

void t_expand::save_state(CIp_t CI, BOOL is_include)
{ prev_univ_source=CI->univ_source;
  prev_compil_ptr =CI->compil_ptr-1;
  include=is_include;
  if (include)
  { prev_src_objnum =CI->src_objnum;
    prev_src_line   =CI->loc_line;
    strcpy(prev_src_name, CI->src_name);
  }
}

void t_expand::restore_state(CIp_t CI)
{ CI->univ_source= prev_univ_source;
  CI->compil_ptr = prev_compil_ptr;
  if (include)
  { CI->src_objnum = prev_src_objnum;
    CI->loc_line   = CI->src_line = prev_src_line;
    strcpy(CI->src_name, prev_src_name);
  }
}

static BOOL check_defines(CIp_t CI)
{ t_define * def=CI->defines;
  t_expand * exp=CI->expansion;
  do
  { while (def)
    { if (!strcmp(def->name, CI->curr_name))
      {// create a new expansion:
        exp = new t_expand;  if (!exp) c_error(OUT_OF_MEMORY);
        exp->other_defines=CI->defines;  CI->defines=NULL;
       // prepare macro parameters substitution: 
        if (def->params)
        { next_and_test(CI, '(', LEFT_PARENT_EXPECTED);
          t_define * par = def->params;
          do
          {// get actual value: 
            const char * start = CI->compil_ptr-1;
            do
            { if (next_sym(CI)==S_SOURCE_END) c_error(EOF_IN_MACRO_EXPANSION);
            } while (CI->cursym!=',' && CI->cursym!=')');
            int len=(int)(CI->prepos-start);
            tptr parstr=(tptr)corealloc(len+1, 94);
            if (!parstr) c_error(OUT_OF_MEMORY);
            memcpy(parstr, start, len);  parstr[len]=0;
           // create substitution:
            t_define * parsubst = new t_define;  if (!parsubst) c_error(OUT_OF_MEMORY);
            strcpy(parsubst->name, par->name);
            parsubst->expansion=parstr;
            parsubst->params=NULL;
            parsubst->next=CI->defines;  CI->defines=parsubst;
           // to the next parameter:
            par=par->next;
            if (par)
              { if (CI->cursym!=',') c_error(COMMA_EXPECTED); }
            else 
              { if (CI->cursym!=')') c_error(RIGHT_PARENT_EXPECTED);  break; }
          } while (TRUE);
        }
       // activate the expansion:
        exp->save_state(CI, FALSE);
        CI->univ_source=CI->compil_ptr=def->expansion;
        exp->next=CI->expansion;  CI->expansion=exp;
        next_char(CI);
        return TRUE;
      }
      def=def->next;
    } 
    if (exp)
    { def=exp->other_defines;
      exp=exp->next;
    }
    else break;
  } while (TRUE);
  return FALSE;
}

uns32 get_unsig(CIp_t CI, uns8 c)
{ uns32 val;
  if ((c<'0')||(c>'9')) c_error(BAD_CHAR);
  val=c-'0';
  while ((CI->curchar>='0')&&(CI->curchar<='9'))
  { //if ((val==214748364L) && (CI->curchar>'7') || (val>214748364L))
    //  c_error(NUMBER_TOO_BIG);
    val= 10*val+CI->curchar-'0'; next_char(CI);
  }
  return val;
}

void translate_string(CIp_t CI)
{ if (CI->translation_disabled) return;
  put_string_char(CI, MAX_STRING_LEN, ' ');  // allocate the max string
  Translate_to_language(CI->cdp, CI->curr_string(), MAX_STRING_LEN);
}

char * translate_to_lang(cdp_t cdp, const char * string)
{ if (!cdp->lang_cache || *string!='@' || strlen(string) > 1+31) return NULL;
  tobjname upstring;  strcpy(upstring, string+1);  Upcase(upstring);
 // like search_table, but the key length is not limited to NAMELEN!
  ctptr patt = cdp->lang_cache->cache_ptr+1;
  int pattnum = cdp->cache_recs;  int pattstep=cdp->lang_cache->rec_cache_size;
 /* returns if NOT found, in pos returns match or insert position */
  int off0, off1, off2;  int res;  
  if (!pattnum) return NULL;
  int keylen=(int)strlen(upstring);
  off0=0; off1=pattnum;
  while (off0+1 < off1)  /* searching by division */
  { off2=(off0+off1)/2;
    res=cmp_str(upstring, patt+off2*pattstep, t_specif(OBJ_NAME_LEN, 1, FALSE, FALSE));
    if (res<0) off1=off2;  else off0=off2;
  }
  res=cmp_str(upstring, patt+off0*pattstep, t_specif(OBJ_NAME_LEN, 1, FALSE, FALSE));
  if (res!=0) return NULL;
 // translated, get the result:
  atr_info * catr = cdp->lang_cache->attrs+2;
  tptr p = cdp->lang_cache->cache_ptr + cdp->lang_cache->rec_cache_size * off0 + catr->offset;
  if (catr->flags & CA_INDIRECT)
  { indir_obj * iobj = (indir_obj*)p;
    if (!iobj->actsize || !iobj->obj->data) return NULLSTRING;
    else return iobj->obj->data;
  }
  else return p;
}

CFNC DllKernel BOOL WINAPI Translate_to_language(cdp_t cdp, char * string, int space)
{ char * trans = translate_to_lang(cdp, string);
  if (!trans) return FALSE;
  strmaxcpy(string, trans, space);
  return TRUE;
}

CFNC DllKernel BOOL WINAPI Translate_and_alloc(cdp_t cdp, const char * input, char ** poutput)
{ char * trans = translate_to_lang(cdp, input);
  if (!trans) return FALSE;
  *poutput = (tptr)corealloc(strlen(trans)+1, 75);
  if (!*poutput) return FALSE;
  strcpy(*poutput, trans);
  return TRUE;
}

static symbol scan_string(CIp_t CI, char terminator)
{ int i=0;  char lastchar;
  do
  { lastchar=0;
    do /* CI->curchar is the next char after " or ' */
    { if (CI->curchar=='\n' || !CI->curchar)
        c_error(STRING_EXCEEDS_LINE);
      put_string_char(CI, i, lastchar=CI->curchar);
      next_char(CI);
#if WBVERS<1000
      if (lastchar=='"' || lastchar=='\'')
#else
      if (lastchar==terminator)  // .NET 2005 ODBC example uses ' in column names, crashes in 602 forms in "something's"
#endif
        if (CI->curchar==lastchar) next_char(CI);
      else break;
      i++;
    } while (TRUE);
    while (CI->curchar=='#' || CI->curchar==' ' || CI->curchar=='\n' || CI->curchar=='\r')
      if (CI->curchar=='#')
      { char c=next_char(CI);  next_char(CI);
        uns32 intval=get_unsig(CI, c);
        if (intval>255) c_error(NUMBER_TOO_BIG);  /* is unsigned */
        put_string_char(CI, i++, (char)intval);
      }
      else next_char(CI);
#if WBVERS<1000
    if (CI->curchar=='"' || CI->curchar=='\'') 
#else
    if (CI->curchar==terminator)
#endif
      next_char(CI);
    else break;
  } while (TRUE);
  put_string_char(CI, i, 0);  /* a delimiter */
  if (i==1 && lastchar=='\'') return S_CHAR;
  if (CI->curr_string()[0]=='@') translate_string(CI);
  return S_STRING;
}

static int hex2int(CIp_t CI, char c)
{ if (c>='0' && c<='9') return c-'0';
  if (c>='A' && c<='F') return c-'A'+10;
  if (c>='a' && c<='f') return c-'a'+10;
  c_error(BAD_CHAR_IN_HEX_LITERAL);
  return 0;
}

static void string_to_binary(CIp_t CI, BOOL hex)
{ tptr src = CI->curr_string();
  int len=1;
  if (hex)
  { while (src[0] && src[1])
    { CI->curr_string()[len++]=(hex2int(CI, src[0])<<4) + hex2int(CI, src[1]);
      src+=2;
    }
    if (*src) CI->curr_string()[len++]=hex2int(CI, *src) << 4;
    CI->curr_string()[len]=0;  // I don't know if this is important
  }
  else  // binary: bits
  { int bit = 128, val = 0;
    while (*src)
    { if (*src=='1') val+=bit;
      else if (*src!='0') c_error(BAD_CHAR_IN_HEX_LITERAL);
      if (bit>1) bit >>= 1;
      else { bit=128;  CI->curr_string()[len++]=val;  val=0; }
      src++;
    }
    if (bit<128) // imcomplete byte in val
      CI->curr_string()[len++]=val;
  }
  CI->curr_string()[0]=(char)(len-1);
}

static uns32 read_odbc_time(CIp_t CI, BOOL with_fraction)
{ char stime[2];  char c;  int hours, minutes, seconds, fract;
//GetProfileString("Intl","sTime",":",stime, sizeof(stime));
  strcpy(stime, ":");
  while (CI->curchar==' '||CI->curchar=='\r'||CI->curchar=='\n')
    next_char(CI);
  if (CI->curchar=='\'') next_char(CI);  // not present in datetime constant
  c=CI->curchar;    next_char(CI);  hours  =get_unsig(CI, c);
  if (CI->curchar!=*stime) c_error(BAD_TIME);
  c=next_char(CI);  next_char(CI);  minutes=get_unsig(CI, c);
  if (CI->curchar!=*stime) c_error(BAD_TIME);
  c=next_char(CI);  next_char(CI);  seconds=get_unsig(CI, c);

  fract=0;
  if (CI->curchar=='.')
  { c=next_char(CI);
    if (c>='0' && c<='9')
    { fract=100*(c-'0');
      c=next_char(CI);
      if (c>='0' && c<='9')
      { fract+=10*(c-'0');
        c=next_char(CI);
        if (c>='0' && c<='9')
        { fract+=c-'0';
          do c=next_char(CI);  // skip other digits
          while (c>='0' && c<='9');
        }
      }
    }
  }

  if (CI->curchar!='\'') c_error(BAD_TIME);
  next_char(CI);
  return Make_time(hours, minutes, seconds, with_fraction ? fract : 0);
  // must ignore fraction for ODBC time, cannot use fraction form ODBC datetime
}

static uns32 read_odbc_date(CIp_t CI)
{ char sdate[20];  tptr p, q;  int day, month, year;  char c;
//GetProfileString("Intl","sShortDate","d.m.yyyy",sdate, sizeof(sdate));
//strcpy(sdate, "d.m.yyyy");
  strcpy(sdate, "yyyy-mm-dd");
  while (CI->curchar==' '||CI->curchar=='\r'||CI->curchar=='\n')
    next_char(CI);
  if (CI->curchar!='\'') c_error(BAD_DATE);
  c=next_char(CI);
  p=sdate;
  do
  { q=p;  while (*p==*q) p++;
    switch (*q)
    { case 'd':  case 'D':
        c=CI->curchar;  next_char(CI);  day  =(int)get_unsig(CI, c);
        break;
      case 'm':  case 'M':
        c=CI->curchar;  next_char(CI);  month=(int)get_unsig(CI, c);
        break;
      case 'y':  case 'Y':
        c=CI->curchar;  next_char(CI);  year =(int)get_unsig(CI, c);
        if (p==q+2) year+=1900;
        break;
      default:
        if (CI->curchar!=*q) c_error(BAD_DATE);
        next_char(CI);  break;
    }
  } while (*p);
  if (CI->curchar=='\'') next_char(CI);
  return Make_date(day, month, year);
}

#ifdef  WINS 
#define MAX_SIG48 0x7FFFFFFFFFFFi64
#else
#define MAX_SIG48 0x7FFFFFFFFFFFll
#endif

void t_last_comment::add_to_comment(const char * start, const char * stop)
{ if (start>=stop) return;  // ignoring empty comments, preventing possible errors when reading the source per-partes
  int length = (int)(stop-start);
  if (length>1000) return;  // preventing possible errors when reading the source per-partes
  int act_len = comm.count();
  if (act_len) act_len--;  // stipping the null terminator
  if (comm.acc(act_len+length+(act_len ? 2 : 0)+1) == NULL) return;
  memcpy(comm.acc0(act_len), start, length);
  if (act_len) { memcpy(comm.acc0(act_len+length), "\r\n", 2);  length+=2; }
  *(char*)comm.acc0(act_len+length) = 0;
}

CFNC DllExport symbol WINAPI next_sym(CIp_t CI)
/* returns the next input symbol, sets the cur_. variables */
{ unsigned char c; int i; 
  CI->last_comment.clear();
commentloop:   // goes here after processing a comment
  /* first skip spaces & delimiters */
  while ((CI->curchar==' ')||(CI->curchar=='\r')||(CI->curchar=='\n'))
    next_char(CI);
  CI->sc_col=CI->loc_col;
  if (CI->sc_col) CI->sc_col--; /* the beginning of the current symbol */

  if (CI->gener_dbg_info)
    if (CI->curchar != S_SOURCE_END)  /* otherwise the "end." line could not be catched */
      generate_dbg_info(CI, CI->src_line, CI->src_objnum, CI->code_offset);
  CI->src_line=CI->loc_line;

  CI->prepos=CI->compil_ptr-1;
  CI->cursym=(symbol)(c=CI->curchar); next_char(CI);
  switch (c)
  { case '+': case '^': case ',': case '*': case '~':
    case ';': case ')': case '[': case ']': case '?': case '\\':
      return (symbol)c;
    case S_SOURCE_END:
      CI->prepos++;
      return (symbol)c;
    case 0x1a:
      return CI->cursym=0;
    case '#':
    { if (CI->curchar=='#')
        { next_char(CI); return CI->cursym=S_PAGENUM; }
      const char * pos=CI->prepos;
      while (pos > CI->univ_source && pos[-1]==' ') pos--;
      if (CI->loc_col==2 || pos<=CI->univ_source || pos[-1]=='\n' || pos[-1]=='\r' || pos[-1]=='}') // directive line ( '}' is possible for leading descriptor comment)
      { process_directive(CI, FALSE);
        return CI->cursym; // next_sym() called recusively inside
      }
      return (symbol)'#';
    }
    case '@':
      if (CI->curchar=='@')
        { next_char(CI); return CI->cursym=S_DBL_AT; }
      else return (symbol)'@';
    case ':':
      if (CI->curchar=='=')
        { next_char(CI); return CI->cursym=S_ASSIGN; }
      else return (symbol)':';
    case '.':
      if (CI->curchar==')')
        { next_char(CI); return CI->cursym=(symbol)']'; }
      else if (CI->curchar=='.')
        { next_char(CI); return CI->cursym=S_DDOT; }
      else if (CI->curchar=='=')
      { if (next_char(CI)=='.')
          { next_char(CI); return CI->cursym=S_SUBSTR; }
        return CI->cursym=S_PREFIX;
      }
      else return (symbol)'.';
    case '<':
      if (CI->curchar=='>')
        { next_char(CI); return CI->cursym=S_NOT_EQ; }
      else if (CI->curchar=='=')
        { next_char(CI); return CI->cursym=S_LESS_OR_EQ; }
      else return (symbol)'<';
    case '>':
      if (CI->curchar=='=')
        { next_char(CI); return CI->cursym=S_GR_OR_EQ; }
      else return (symbol)'>';
    case '!':   /* for C & SQL */
      if (CI->curchar=='=')
        { next_char(CI); return CI->cursym=S_NOT_EQ; }
      else if (CI->curchar=='<')  /* for SQL */
        { next_char(CI); return CI->cursym=S_GR_OR_EQ; }
      else if (CI->curchar=='>')  /* for SQL */
        { next_char(CI); return CI->cursym=S_LESS_OR_EQ; }
      else if (CI->curchar=='!')
        { next_char(CI); return CI->cursym=S_CURRVIEW; }
      else if (CI->keynames==*keynames)
        return CI->cursym=S_NOT;
      else return '!';
    case '=':   /* for C */
      if (CI->curchar=='=') next_char(CI);
      return CI->cursym;  /* no change */
    case '|':   /* for C */
      if (CI->curchar=='|')
        { next_char(CI); return CI->cursym=S_OR; }
      else return (symbol)'|';
    case '&':   /* for C */
      if (CI->curchar=='&')
        { next_char(CI); return CI->cursym=S_AND; }
      else return (symbol)'&';
    case '/':
      if (CI->curchar=='*')  /* C-comment */
      { next_char(CI);
        do
        { if (CI->curchar=='*')
          { if (next_char(CI)=='/') break; }
          else
          { if (CI->curchar=='\0') c_error(EOF_IN_COMMENT);
            next_char(CI);
          }
        } while (TRUE);
        CI->last_comment.add_to_comment(CI->prepos+2, CI->compil_ptr-2);
        next_char(CI); goto commentloop;
      }
      else if (CI->curchar=='/')  /* C++ line comment */
      { do
        { next_char(CI);
        } while (CI->curchar!='\r' && CI->curchar!='\n' && CI->curchar);
        CI->last_comment.add_to_comment(CI->prepos+2, CI->compil_ptr-1);
        goto commentloop;
      }
      return (symbol)'/';
    case '-':
      if (CI->curchar=='-')  /* SQL line comment */
      { do
        { next_char(CI);
        } while (CI->curchar!='\r' && CI->curchar!='\n' && CI->curchar);
        CI->last_comment.add_to_comment(CI->prepos+2, CI->compil_ptr-1);
        goto commentloop;
      }
      return (symbol)'-';
    case '(':
      if (CI->curchar=='.')
        { next_char(CI); return CI->cursym=(symbol)'['; }
      return '(';
    case '}':
      if (CI->keynames==*sql_keynames)  /* end of {oj ... } */
        goto commentloop;
      c_error(BAD_CHAR);
    case '{':
    { if (CI->keynames==*sql_keynames)  /* check for ODBC extensions */
      { while ((CI->curchar==' ')||(CI->curchar=='\r')||(CI->curchar=='\n'))
          next_char(CI);
        if (CI->curchar=='d')
        { next_char(CI);
          CI->curr_int=read_odbc_date(CI);
          if ((uns32)CI->curr_int==NONEDATE) c_error(BAD_DATE);
          return CI->cursym=S_DATE;
        }
        else if (CI->curchar=='t')
        { next_char(CI);
          if (CI->curchar=='s')  // ODBC timestamp value
          { next_char(CI);
            uns32 dat=read_odbc_date(CI);
            if ((uns32)CI->curr_int==NONEDATE) c_error(BAD_DATE);
            uns32 tim=read_odbc_time(CI, FALSE);
            if ((uns32)CI->curr_int==NONETIME) c_error(BAD_TIME);
            CI->curr_int=datetime2timestamp(dat, tim);
            return CI->cursym=S_TIMESTAMP;
          }
          else // ODBC time value
          { CI->curr_int=read_odbc_time(CI, FALSE);
            if ((uns32)CI->curr_int==NONETIME) c_error(BAD_TIME);
            return CI->cursym=S_TIME;
          }
        }
        else if (CI->curchar=='o')
        { next_char(CI);
          if (CI->curchar=='j')
          { next_char(CI);
            goto commentloop;
          }
        }
      }
     // ODBC extension not recognized, try pascal directive:
      if (CI->curchar=='$')   /* directive */
      { next_char(CI);
        process_directive(CI, TRUE);
        return CI->cursym; // next_sym() called recusively inside
      }
     // normal pascal-style comment:
      skip_comment(CI);
      CI->last_comment.add_to_comment(CI->prepos+1, CI->compil_ptr-2);
      goto commentloop;
    }
    case '"':  case '\'':
      return CI->cursym=scan_string(CI, c);
    case '`': /* quouted identifier */
      i=0;
      while (CI->curchar != '`')
      { if (!CI->curchar || CI->curchar=='\n') c_error(STRING_EXCEEDS_LINE);
        if (i<OBJ_NAME_LEN)
        { CI->curr_name[i]=CI->name_with_case[i]=CI->curchar;
          i++;
        }
        next_char(CI);
      }
      next_char(CI);
      CI->curr_name[i]=CI->name_with_case[i]=0;  /* delimiter */
      Upcase9(CI->cdp, CI->curr_name);
      return CI->cursym=S_IDENT;
    default:  /* identifiers, numbers, bad chars */
      if (is_AlphaA(c, CI->sys_charset) || c=='_')
      { if (CI->curchar=='\'' && (c=='X' || c=='B' || c=='N'))  // literal
        { next_char(CI);
          CI->cursym=scan_string(CI, '\'');
          if (c=='X' || c=='B')
          { string_to_binary(CI, c=='X');
            CI->cursym=S_BINLIT;
          }
        }
        else  // identifier
        { *CI->curr_name=*CI->name_with_case=c;
          i=1;
          do
          { if (is_AlphanumA(CI->curchar, CI->sys_charset))
            { if (i<OBJ_NAME_LEN)
              { CI->curr_name[i]=CI->name_with_case[i]=CI->curchar;
                i++;
              }                            
            }
            else break;
            next_char(CI);
          } while (true);
          CI->curr_name[i]=CI->name_with_case[i]=0;  /* delimiter */
          Upcase9(CI->cdp, CI->curr_name);
          if (check_defines(CI)) goto commentloop;
          if (search_table(CI->curr_name,CI->keynames,CI->key_count,sizeof(tname),&i))
          { //if (CI->keynames==*sql_keynames && CI->curchar=='\'' &&
            //    CI->curr_name[0]=='_')  // national character set name
            //{ next_char(CI);
            //  CI->cursym=scan_string(CI);
            //}  -- not for I-level
            /*else*/ CI->cursym=S_IDENT;
          }
          else  // found in the keyword table
          { CI->cursym=(symbol)(i+0x80);  // keyword symbol value
            if (CI->keynames==*keynames)
              { if (CI->cursym==S_INCLUDE) goto commentloop; }
            else // SQL keywords
              if (CI->curchar=='\'')
              { switch (CI->cursym)
                { case SQ_TIME:
                    CI->curr_int=read_odbc_time(CI, TRUE);
                    if ((uns32)CI->curr_int==NONETIME) c_error(BAD_TIME);
                    CI->cursym=S_TIME;  break;
                  case SQ_DATE:
                    CI->curr_int=read_odbc_date(CI);
                    if ((uns32)CI->curr_int==NONEDATE) c_error(BAD_DATE);
                    CI->cursym=S_DATE;  break;
                  case SQ_TIMESTAMP:
                  { uns32 dat=read_odbc_date(CI);
                    if ((uns32)CI->curr_int==NONEDATE) c_error(BAD_DATE);
                    uns32 tim=read_odbc_time(CI, FALSE);
                    if ((uns32)CI->curr_int==NONETIME) c_error(BAD_TIME);
                    CI->curr_int=datetime2timestamp(dat, tim);
                    CI->cursym=S_TIMESTAMP;  break;
                  }
                }
              } // followed by '
          }
        }
        return CI->cursym;
      } // does not start with a letter
      else  /* a number: integer, real, money, date, time */
      { CI->curr_scale=0;  BOOL in_decpart;
#define MAX_NUM_LEN 30
        char num[MAX_NUM_LEN+4];  num[0]=c;  int numpos=1;  // string copy of the number, user for real conversion
       // process the 1st character already in c:
        if (c>='0' && c<='9') { CI->curr_int = c-'0';  in_decpart=FALSE; }
        else if (c=='.') { CI->curr_int = 0;  in_decpart=TRUE; }
        else c_error(BAD_CHAR);
       // process the other characters nad digits:
        BOOL is_money = FALSE;
        do
        { c=CI->curchar;
          if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
          if (c>='0' && c<='9')
          { if (CI->curr_int >= MAXBIGINT / 10)
              if (CI->curr_int > MAXBIGINT / 10 || c > MAXBIGINT % 10)
                c_error(NUMBER_TOO_BIG);
            CI->curr_int = 10*CI->curr_int + (c-'0');
            if (in_decpart) CI->curr_scale++;
          }
          else if ((c=='.' || c=='$') && !in_decpart)
          { in_decpart=TRUE;
            if (c=='$') { is_money=TRUE;  c='.'; }
          }
          else break;
          num[numpos++]=c;
          next_char(CI);
        } while (TRUE);
       // 'E' means the semilogaritmic literal:
        if (c=='E' || c=='e')
        {// read the rest of the real number 
          num[numpos++]=c;
          next_char(CI);
          if (CI->curchar=='-')
            { num[numpos++]=CI->curchar;  next_char(CI); }
          else if (CI->curchar=='+')
            { num[numpos++]=CI->curchar;  next_char(CI); }
          i=0;
          while ((CI->curchar>='0')&&(CI->curchar<='9'))
          { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
            num[numpos++]=CI->curchar;
            i=10*i+CI->curchar-'0';
            if (i>308) c_error(NUMBER_TOO_BIG);
            next_char(CI);
          }
          num[numpos]=0;
         // convert and return the real literal:
          CI->curr_real=strtod(num, NULL);
          return CI->cursym=S_REAL;
        }
       // client specific: if dot is not followed by a digit, it does not belong to the number:
        if (in_decpart && CI->curr_scale==0)
        { CI->compil_ptr--; CI->curchar='.';
          return CI->cursym=S_INTEGER;
        }
       // ':' means the time literal:
        if (c==':')  
        { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
          num[numpos++]=c;
          c=next_char(CI);
         // client specific: if ':' is not followed by a digit, it does not belong to the literal:
          if (c<'0' || c>'9')
          { CI->compil_ptr--; CI->curchar=':';
            return CI->cursym=S_INTEGER;
          }
         // read the rest: 
          do
          { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
            num[numpos++]=c;
            c=next_char(CI);
          } while (c>='0' && c<='9' || c==':' || c=='.');
          num[numpos]=0;
         // convert to the time:
          uns32 tm;
          if (!str2time(num, &tm)) c_error(BAD_TIME);
          CI->curr_int=tm;
          return CI->cursym=S_TIME;
        }
       // another dot -> date or timestamp
        if (c=='.')  
        { do
          { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
            num[numpos++]=c;
            c=next_char(CI);
          } while (c>='0' && c<='9' || c=='.');
          while (c==' ') c=next_char(CI);
          if (c>='0' && c<='9')
          { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
            num[numpos++]=' ';
            do
            { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
              num[numpos++]=c;
              c=next_char(CI);
            } while (c>='0' && c<='9' || c==':' || c=='.');
            num[numpos]=0;
            uns32 ts;
            if (!str2timestamp(num, &ts)) c_error(BAD_TIME);
            CI->curr_int=ts;
            return CI->cursym=S_TIMESTAMP;
          }
          else
          { num[numpos]=0;
            uns32 dt;
            if (!str2date(num, &dt)) c_error(BAD_DATE);
            CI->curr_int=dt;
            return CI->cursym=S_DATE;
          }
        }
       // integer (possibly scaled) or money:
        if (is_money) // separator '$' used, convert to scale 2:
        { if (!scale_int64(CI->curr_int, 2-CI->curr_scale) || CI->curr_int>MAX_SIG48 || CI->curr_int<-MAX_SIG48)
            c_error(NUMBER_TOO_BIG);
          CI->curr_scale=2;
         // curr_money is client specific 
          CI->curr_money.money_hi4=(sig32)(CI->curr_int >> 16);
          CI->curr_money.money_lo2=(uns16)(CI->curr_int & 0xffff);
          return CI->cursym=S_MONEY; 
        }
       // client specific: scale not supported:
        if (CI->curr_scale)
        { num[numpos]=0;
         // convert and return the real literal:
          CI->curr_real=strtod(num, NULL);
          return CI->cursym=S_REAL;
        }
        return CI->cursym=S_INTEGER; 
      }
#ifdef STOP
      { monstr dig;  dig.money_hi4=0;
        double koef; unsigned char sign;
        uns32 intval2, intval3, intval4;
#define MAX_NUM_LEN 30
        char num[MAX_NUM_LEN+4];  num[0]=c;  int numpos=1;
        if ((c<'0')||(c>'9')) c_error(BAD_CHAR);
        CI->curr_real=(double)(c-'0');
        CI->curr_money.money_hi4=0;
        CI->curr_money.money_lo2=(uns16)(c-'0');

        while ((CI->curchar>='0')&&(CI->curchar<='9'))
        { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
          num[numpos++]=CI->curchar;
          CI->curr_real=10*CI->curr_real+(double)(CI->curchar-'0');
                         dig.money_lo2=(uns16)(CI->curchar-'0');
          moneymult(&CI->curr_money, 10);
          moneyadd (&CI->curr_money, &dig);
          next_char(CI);
        }
        CI->cursym=S_INTEGER;  /* if will not be stated otherwise later */
        CI->curr_int=(sig32)CI->curr_real;
        intval2=0;
        c=CI->curchar;
        if ((CI->curchar=='.')||(CI->curchar==':'))
        { num[numpos++]=CI->curchar;
          CI->cursym=(symbol)((CI->curchar==':') ? S_TIME : S_REAL);
          next_char(CI);
          if ((CI->curchar>='0')&&(CI->curchar<='9'))
          { koef=0.1f;
            while ((CI->curchar>='0')&&(CI->curchar<='9'))
            { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
              num[numpos++]=CI->curchar;
              CI->curr_real += (CI->curchar-'0')*koef;
              intval2 = 10*intval2+(CI->curchar-'0');
              koef *= 0.1f;
              next_char(CI);
            }
          }
          else  /* dot doesn't belong to the number, go back !! */
          { CI->compil_ptr--; CI->curchar=c;
            if (CI->curr_real > 2147483647.999999f) c_error(NUMBER_TOO_BIG);
            return CI->cursym=S_INTEGER;
          }
        }
        if (CI->curchar=='$')
        { next_char(CI);
          moneymult(&CI->curr_money, 100);
          if ((CI->curchar>='0') && (CI->curchar<='9'))
                         { dig.money_lo2=(uns16)(10*(CI->curchar-'0'));
            next_char(CI);
            if ((CI->curchar>='0') && (CI->curchar<='9'))
                                { dig.money_lo2=(uns16)(dig.money_lo2+CI->curchar-'0');
              next_char(CI);
            }
            moneyadd(&CI->curr_money, &dig);
          }
          return CI->cursym=S_MONEY;
        }
        if ((CI->curchar & 0xdf) == 'E')
        { num[numpos++]=CI->curchar;
          next_char(CI);
          if ((sign=CI->curchar)=='-')
            { num[numpos++]=CI->curchar;  next_char(CI); }
          else if (CI->curchar=='+')
            { num[numpos++]=CI->curchar;  next_char(CI); }
          i=0;
          while ((CI->curchar>='0')&&(CI->curchar<='9'))
          { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
            num[numpos++]=CI->curchar;
            i=10*i+CI->curchar-'0';
            if (i>308) c_error(NUMBER_TOO_BIG);
            next_char(CI);
          }
          if (sign=='-') while (i--) CI->curr_real *= 0.1;
          else while (i--) CI->curr_real *= 10.0;
          num[numpos]=0;
          CI->curr_real=strtod(num, NULL);
          return CI->cursym=S_REAL;
        }
        else
        { num[numpos]=0;
          CI->curr_real=strtod(num, NULL);
        }
        if (CI->cursym==S_TIME)  /* there was a ':' */
        { if ((CI->curr_int>=24)||(intval2>=60)) c_error(BAD_TIME);
          intval3=intval4=0;
          if (CI->curchar==':')
          { c=next_char(CI); next_char(CI); intval3=get_unsig(CI, c);
            if (CI->curchar=='.')
            { c=next_char(CI); next_char(CI); intval4=get_unsig(CI, c); }
          }
          if ((intval3>=60)||(intval4>=1000)) c_error(BAD_TIME);
          if (intval4) while (intval4<100) intval4*=10;
          CI->curr_int=intval4 + 1000 * (intval3 + 60*(intval2+60*CI->curr_int));
          return CI->cursym;

        }
        if (CI->curchar=='.')  /* disting. real without E and date */
        { c=next_char(CI);
          if ((c>='0') && (c<='9'))
            { next_char(CI); intval3=get_unsig(CI, c); }
          else /* default year, not very safe in programs (good for queries) */
          { uns8 day, month;  int year;
            xgetdate(&day, &month, &year);
            intval3=year;
          }
          int intval1=CI->curr_int;
          CI->curr_int=Make_date(intval1, intval2, intval3);
          if ((uns32)CI->curr_int==NONEDATE) c_error(BAD_DATE);
         // check if time follows, form datetime constant then:
          while (CI->curchar==' ') next_char(CI);
          if (CI->curchar>='0' && CI->curchar<='9')
          { int hour, minute, second;
            c=CI->curchar;  next_char(CI);  hour=get_unsig(CI, c);
            if (CI->curchar!=':') c_error(BAD_TIME);
            next_char(CI);
            if (!(CI->curchar>='0' && CI->curchar<='9')) c_error(BAD_TIME);
            c=CI->curchar;  next_char(CI);  minute=get_unsig(CI, c);
            if (CI->curchar==':')
              { c=next_char(CI);  next_char(CI);  second=get_unsig(CI, c); }
            else second=0;
            TIMESTAMP_STRUCT ts;  uns32 dtm;
            ts.day=intval1;  ts.month=intval2;  ts.year=intval3;
            ts.hour=hour;  ts.minute=minute;  ts.second=second;  ts.fraction=0;
            TIMESTAMP2datim(&ts, &dtm);
            CI->curr_int=dtm;
            return CI->cursym=S_TIMESTAMP;
          }
          return CI->cursym=S_DATE;
        }
        return CI->cursym;
      }
#endif
  }
}

CFNC DllKernel void WINAPI next_and_test(CIp_t CI, symbol sym, short message)
{ if (next_sym(CI)!=sym) c_error(message); }

CFNC DllKernel void WINAPI test_and_next(CIp_t CI, symbol sym, short message)
{ if (CI->cursym!=sym) c_error(message); next_sym(CI); }

BOOL test_signum(CIp_t CI)
{ if (CI->cursym=='-') { next_sym(CI); return TRUE; }
  else if (CI->cursym=='+') next_sym(CI);
  return FALSE;
}

BOOL CHAR_ARR(typeobj * tobj)
{ return tobj->type.anyt.typecateg==T_STR || tobj->type.anyt.typecateg==T_ARRAY && tobj->type.array.elemtype==&att_char; }

BOOL STR_ARR(typeobj * tobj)
{ return tobj==&att_string || CHAR_ARR(tobj); }

/************************* code generation **********************************/
#define INIT_CODE_SIZE  50
#define PG_CODE_STEP    2000
#define PG_CODE_LIMIT   40000L  

CFNC DllKernel void WINAPI code_out(CIp_t CI, ctptr code, uns32 addr, unsigned size)
{ tptr aux;  unsigned iosize, step;
  if (CI->output_type==DUMMY_OUT) return;
  if (!CI->cb_size)
  { if (!(CI->univ_code=(tptr)corealloc(CI->cb_size=INIT_CODE_SIZE, OW_CODE)))
                c_error(CODE_GEN);
  }
  if (addr<CI->cb_start)   /* write to the compiled object */
  { iosize=(addr+size<=CI->cb_start) ? size : (unsigned)(CI->cb_start-addr);
    if (cd_Write_var(CI->cdp,OBJ_TABLENUM,CI->cb_object,OBJ_DEF_ATR,NOINDEX,addr,iosize,code))
      c_error(CODE_GEN);
    addr+=iosize; code+=iosize; size-=iosize;
  }
  if (!size) return;
  if (addr<CI->cb_start+CI->cb_size)
  { if (size==1)   /* optimization: the most typical case */
    { CI->univ_code[(unsigned)(addr-CI->cb_start)]=*code;
      return;
    }
    iosize=(addr+size<=CI->cb_start+CI->cb_size) ? size : (unsigned)(CI->cb_start+CI->cb_size-addr);
    memcpy(CI->univ_code+(unsigned)(addr-CI->cb_start), code, iosize);
         addr+=iosize;  code+=iosize;  size-=iosize;
    if (!size) return;
  }
 /* write after the allocated code block */
  if (CI->cb_size < PG_CODE_LIMIT || CI->output_type==MEMORY_OUT) // complex views in BankStat need more than limit, try to reallocate
  { step=addr+size-CI->cb_start-CI->cb_size;   /* >0 as addr>=CI->cb_start+cb+size */
    if (step<PG_CODE_STEP) step=PG_CODE_STEP;
    if (step<CI->cb_size/4) step=CI->cb_size/4;  // avoid many small steps
    aux=(tptr)corerealloc(CI->univ_code,CI->cb_size+step);
    if (aux)   /* reallocated OK */
         { CI->univ_code=aux;  CI->cb_size+=step;
      memcpy(CI->univ_code+(unsigned)(addr-CI->cb_start), code, size);
      return;
    }
  }
  if (CI->output_type==MEMORY_OUT) c_error(CODE_GEN);
  do
  { if (cd_Write_var(CI->cdp, OBJ_TABLENUM,CI->cb_object,OBJ_DEF_ATR, NOINDEX,
      CI->cb_start, CI->cb_size, CI->univ_code)) c_error(CODE_GEN);
    CI->cb_start+=CI->cb_size;
    if (addr < CI->cb_start+CI->cb_size)
    { iosize=(addr+size < CI->cb_start+CI->cb_size)
        ? size : (unsigned)(CI->cb_start+CI->cb_size-addr);
      memcpy(CI->univ_code+(unsigned)(addr-CI->cb_start), code, iosize);
           addr+=iosize; code+=iosize; size-=iosize;
    }
  } while (size);
}

static BOOL close_comp(CIp_t CI, BOOL is_err)   /* the call may be repeated */
/* Must not call c_error because can be called in handler, returns FALSE or error */
// If error occurs when writing code, will be called twice!
{ BOOL ok=TRUE;
  if (CI->comp_callback) (*CI->comp_callback)(CI->src_line, CI->total_line, CI->src_name);
  if (CI->startnet==program) register_program_compilation(CI->cdp, NULL);
 // destroys defines and expansions:
  if (CI->defines  ) { CI->defines  ->del_recurs();  CI->defines  =NULL; }  // setting NULL important when calling twice!
  if (CI->expansion) { CI->expansion->del_recurs();  CI->expansion=NULL; }
 // delete conditional compilation information:
  while (CI->condcomp)  // error reported if not NULL 
  { t_condcomp * cc = CI->condcomp;
    CI->condcomp = cc->next;  delete cc;
  }
  /* frees all compiler allocations except for univ_code */
  if (CI->any_alloc && nesting_counter==1) total_free(OW_COMPIL);   // to je zle !!!!!
  if (is_err) safefree((void**)&CI->univ_code);
#ifdef WINS
#if WBVERS<900
  if (CI->AXInfo)
  {
      CI->AXInfo->Release();
      CI->AXInfo = NULL;
  }
#endif
#endif
  if (CI->univ_code)  /* anythig generated */
    if (CI->output_type==MEMORY_OUT) /* code in univ_code, univ_size -> reallocate */
    { tptr new_code=(tptr)corealloc((unsigned)CI->code_offset, OW_CODE);
      if (new_code)
      { memcpy(new_code, CI->univ_code, (unsigned)CI->code_offset);
        corefree(CI->univ_code);
        CI->univ_code=new_code;
      }
    }
    else if (CI->output_type==DB_OUT)  /* save and free the code buffer */
    { if (cd_Write_var(CI->cdp, OBJ_TABLENUM, CI->cb_object, OBJ_DEF_ATR,
                         NOINDEX, CI->cb_start, CI->cb_size, CI->univ_code))
        { ok=FALSE;  CI->code_offset=0; }
      safefree((void**)&CI->univ_code);
    }
 // other actions, even on error:
  if (CI->output_type==DB_OUT)
  { if (is_err) CI->code_offset=0; /* no part of code must be created!! */
    cd_Write_len(CI->cdp, OBJ_TABLENUM, CI->cb_object, OBJ_DEF_ATR, NOINDEX, CI->code_offset);
  }
  return ok;
}

/*void gen(CIp_t CI, uns8 code)  -- replaced by macro
{ code_out(CI, &code,CI->code_offset++,1);
} */
CFNC DllKernel void WINAPI gen2(CIp_t CI, uns16 code)
{ code_out(CI, (tptr)&code,CI->code_offset,2);
  CI->code_offset+=2;
}
CFNC DllKernel void WINAPI gen4(CIp_t CI, uns32 code)
{ code_out(CI, (tptr)&code,CI->code_offset,4);
  CI->code_offset+=4;
}
void gen8(CIp_t CI, double code)
{ code_out(CI, (tptr)&code,CI->code_offset,8);
  CI->code_offset+=8;
}
void gen8i(CIp_t CI, void * code)
{ code_out(CI, (tptr)code,CI->code_offset,8);
  CI->code_offset+=8;
}

CFNC DllKernel void WINAPI genstr(CIp_t CI, tptr str)
{ int i=0;
  do GEN(CI,str[i]) while (str[i++]);
}

/* gen_forward_jump & setcadr should be used in pairs:
   adr=gen_forward_jump(I_JUMPxxx);  ... setcadr(CI, adr);   */

code_addr gen_forward_jump(CIp_t CI, uns8 instr)
{ code_addr adr;
  gen(CI,instr);  adr=CI->code_offset;
  CI->code_offset+=sizeof(sig16);
  return adr;
}

void setcadr(CIp_t CI, code_addr place)
{ int diff = CI->code_offset-place;
  code_out(CI, (tptr)&diff, place, sizeof(sig16));
}

code_addr gen_forward_jump4(CIp_t CI, uns8 instr)
{ code_addr adr;
  gen(CI,instr);  adr=CI->code_offset;
  CI->code_offset+=sizeof(sig32);
  return adr;
}

void setcadr4(CIp_t CI, code_addr place)
{ int diff = CI->code_offset-place;
  code_out(CI, (tptr)&diff, place, sizeof(sig32));
}

void geniconst(CIp_t CI, sig32 val)   /* generates loading a constant */
{ if ((val<256L)&&(val>=0))
         { gen(CI,I_INT8CONST);  gen(CI,(uns8)val); }
  else if ((val<32763L)&&(val>=-32763L))
         { gen(CI,I_INT16CONST); gen2(CI,(uns16)val); }
  else
         { gen(CI,I_INT32CONST); gen4(CI,val); }
}

/*************************** assignment *************************************/
BOOL same_type(unsigned t1, unsigned t2)
{ if (t1==t2) return TRUE;
  if ((t1 & 0x80) == (t2 & 0x80))
  { t1 &= 0x7f;  t2 &= 0x7f;
    if ((t1==ATT_INT16) || (t1==ATT_INT32) || (t1==ATT_INT8))
      if ((t2==ATT_INT16) || (t2==ATT_INT32) || (t2==ATT_INT8))
        return TRUE;
    if ((t1==ATT_PTR) || (t1==ATT_BIPTR))
      if ((t2==ATT_PTR) || (t2==ATT_BIPTR))
        return TRUE;
    if (is_string(t1) && is_string(t2))
      return TRUE;
  }
  return FALSE;
}

static void typetest(CIp_t CI, typeobj * t1, typeobj * t2, BOOL refpar)
/* tests the full-compatibility of types */
{ if (t1!=t2)
  { if (t1==&att_nil && t2->type.anyt.typecateg!=T_SIMPLE_DB) return;
    if (t2==&att_nil) return;
#ifdef NEVER
        if (t1->type.anyt.typecateg==T_PTR &&    /* char *   :=   string */
        (t1->type.ptr.domaintype==&att_char) &&
        t2==&att_string) return;
#endif
    if (t1==&att_string && (t2==&att_string || CHAR_ARR(t2))) return;
    if (t2==&att_string && CHAR_ARR(t1)) return;
    if (CHAR_ARR(t1) && CHAR_ARR(t2))  /* same string types */
      if (t1->type.string.typecateg==t2->type.string.typecateg)
        if (t1->type.string.valsize==t2->type.string.valsize) return;
    if (t1->type.anyt.typecateg==T_BINARY)
      if (t2->type.anyt.typecateg==T_BINARY)
        { if (!refpar || t1->type.anyt.valsize==t2->type.anyt.valsize) return; }
      else
        if (!refpar && t2==&att_binary) return;
    if (t2->type.anyt.typecateg==T_BINARY && t1==&att_binary) return;

    if ((t1->type.anyt.typecateg==T_PTR || t1->type.anyt.typecateg==T_PTRFWD) &&
        (t2->type.anyt.typecateg==T_PTR || t2->type.anyt.typecateg==T_PTRFWD))
    { typetest(CI, t1->type.ptr.domaintype, t2->type.ptr.domaintype, refpar);
      return;
    }
    if (t1->type.anyt.typecateg==T_VIEW && t2->type.anyt.typecateg==T_VIEW)
      if (t1->type.view.objnum==t2->type.view.objnum || t1->type.view.objnum==NOOBJECT)
        return;
    
    if (t2==&att_cursor && t1->type.anyt.typecateg==T_RECCUR)
      return;   /* passing variable cursor as parameter */
    if (refpar)
    { if (t1==&att_cursor && t2==&att_statcurs)
        return;   // passing static cursor reference as variable cursor reference in Close_cursor(static_cursor) */
      if (t2==&att_cursor && t1==&att_statcurs) // added in 7.0c
        return;   // passing variable cursor reference as static cursor reference in Restrict/Restore_cursor */
      if ((t1->type.anyt.typecateg==T_ARRAY) &&
          (t2->type.anyt.typecateg==T_ARRAY) &&
          (t1->type.array.elemtype==t2->type.array.elemtype) &&
           (t1->type.array.lowerbound==t2->type.array.lowerbound)) return;
      //if (t1->type.anyt.typecateg==T_SIMPLE && t2->type.anyt.typecateg==T_SIMPLE && 
      //    t1->type.simple.typenum==t2->type.simple.typenum &&
      //    IS_PROJ_VAR(t2))
      //  return;   /* passing project variable as a ref. parameter */
      // .. seems not necessary any more
    }
    /* Variable cursor cannot be passed as value param: collision with passing cursor number! */
    c_error(INCOMPATIBLE_TYPES);
  }
}

void convtype(CIp_t CI, uns8 at1, uns8 at2)  // conversion used by assignment
/* Attention: Borland compiler errors!! */
{ if (at1==at2) return;
  switch (at1) // target type
  { case ATT_FLOAT:
      if (at2==ATT_INT32)
                  { gen(CI,I_TOSI2F);                      return; }
      if (at2==ATT_INT16)
                  { gen(CI,I_T16TO32);  gen(CI,I_TOSI2F);  return; }
      if (at2==ATT_INT8)
                  { gen(CI,I_T8TO32);   gen(CI,I_TOSI2F);  return; }
      if (at2==ATT_MONEY)
                  { gen(CI,I_TOSM2F);                      return; }
      if (at2==ATT_INT64)
                  { gen(CI,I_T64TOF);                      return; }
      break;
    case ATT_MONEY:
      if (at2==ATT_INT32)
                  { gen(CI,I_TOSI2M);                      return; }
      if (at2==ATT_INT16)
                  { gen(CI,I_T16TO32);  gen(CI,I_TOSI2M);  return; }
      if (at2==ATT_INT8)
                  { gen(CI,I_T8TO32);   gen(CI,I_TOSI2M);  return; }
      if (at2==ATT_FLOAT)
                  { gen(CI,I_TOSF2M);                      return; }
      if (at2==ATT_INT64)
                  { gen(CI,I_T64TOF);   gen(CI,I_TOSF2M);  return; }
      break;
    case ATT_INT32:
      if (at2==ATT_MONEY)
                  { gen(CI,I_TOSM2I);                  return; }
      if ((at2==ATT_TABLE) || (at2==ATT_CURSOR))
        return; /* no conv. necessary */
      if (at2==ATT_INT16)
                  { gen(CI,I_T16TO32);                  return; }
      if (at2==ATT_INT8 || at2==ATT_BOOLEAN)
                  { gen(CI,I_T8TO32);                   return; }
      if (at2==ATT_INT64)
                  { gen(CI,I_T64TO32);                  return; }
      if ((at2==ATT_PTR)   || (at2==ATT_BIPTR))
        return; /* no conv. necessary */
      break;
    case ATT_INT64:
      if (at2==ATT_MONEY)
                  { gen(CI,I_TOSM2F);  gen(CI,I_TFTO64);   return; }
      if (at2==ATT_INT16)
                  { gen(CI,I_T16TO32); gen(CI,I_T32TO64);  return; }
      if (at2==ATT_INT8 || at2==ATT_BOOLEAN)
                  { gen(CI,I_T8TO32);  gen(CI,I_T32TO64);  return; }
      if (at2==ATT_INT32)
                  { gen(CI,I_T32TO64);                     return; }
      break;
    case ATT_INT16:
      if (at2==ATT_MONEY)
                  { gen(CI,I_TOSM2I);  gen(CI,I_T32TO16);  return; }
      if (at2==ATT_INT32)
                  { gen(CI,I_T32TO16);                     return; }
      if (at2==ATT_INT8)
                  { gen(CI,I_T8TO32);  gen(CI,I_T32TO16);  return; }
      if (at2==ATT_INT64)
                  { gen(CI,I_T64TO32); gen(CI,I_T32TO16);  return; }
      if (at2==ATT_BOOLEAN)
        return;
      break;
    case ATT_INT8:
      if (at2==ATT_MONEY)
                  { gen(CI,I_TOSM2I);   gen(CI,I_T32TO8);  return; }
      if (at2==ATT_INT32)
                  { gen(CI,I_T32TO8);                      return; }
      if (at2==ATT_INT16)
                  { gen(CI,I_T16TO32);  gen(CI,I_T32TO8);  return; }
      if (at2==ATT_INT64)
                  { gen(CI,I_T64TO32);  gen(CI,I_T32TO8);  return; }
      if (at2==ATT_BOOLEAN)
        return;
      break;
    case ATT_CURSOR:
      if (at2==ATT_TABLE || at2==ATT_INT32 || at2==ATT_INT16 || at2==ATT_INT8) return;
      if (at2==ATT_STATCURS) 
        { gen(CI,I_LOADOBJNUM);  return; }
      break;
    case ATT_TABLE:
      if (at2==ATT_INT32 || at2==ATT_INT16 || at2==ATT_INT8) return;
      break;
    case ATT_INDNUM:
    case ATT_VARLEN:
      if (at2==ATT_INT32 || at2==ATT_INT16 || at2==ATT_INT8) return;
      break;
    case ATT_PTR:
    case ATT_BIPTR:
      if ((at2==ATT_INT32) || (at2==ATT_PTR) || (at2==ATT_BIPTR)) return;
      break;
    case ATT_STRING:  
      if (is_string(at2)) return;
      if (at2==ATT_CHAR)
                { gen(CI,I_CHAR2STR);   /* conversion */ return; }
      break;
  }
  c_error(INCOMPATIBLE_TYPES);
}

int assignment(CIp_t CI, typeobj * t1, int aflag, typeobj * t2, int eflag, BOOL is_parameter)
/* the address and the values are supposed to be on the stack */
// If (is_parameter) the returns parameter size, otherwise the return value is undefined.
{ uns8 at1, at2;

  at1=t1->type.simple.typenum;  if (at1==ATT_BIPTR) at1=ATT_PTR;
  at2=t2->type.simple.typenum;  if (at2==ATT_BIPTR) at2=ATT_PTR;
  if ((t1==&att_dbrec) && t2->type.anyt.typecateg!=T_SIMPLE && t2->type.anyt.typecateg!=T_SIMPLE_DB)   /* full record transfer */
         { gen(CI,I_WRITEREC);  gen2(CI,t2->type.anyt.valsize);  return 0; }
  if ((t2==&att_dbrec) && t1->type.anyt.typecateg!=T_SIMPLE && t1->type.anyt.typecateg!=T_SIMPLE_DB)   /* full record transfer */
         { gen(CI,I_READREC);   gen2(CI,t1->type.anyt.valsize);  return 0; }
 /* untyped operations */
  if ((DIRECT(t1) || CHAR_ARR(t1) || t1->type.anyt.typecateg==T_BINARY) &&
      (DIRECT(t2) || CHAR_ARR(t2) || t2->type.anyt.typecateg==T_BINARY) &&
      ((at1==ATT_UNTYPED) || (at2==ATT_UNTYPED)))
  { unsigned len;  int flags = is_parameter ? UNT_DESTPAR : 0;
    if (CHAR_ARR(t1))   /* assignment to a char array */
    { len=t1->type.anyt.valsize;
      at1=TtoATT(t1);
    }
    else if (t1->type.anyt.typecateg==T_BINARY)
    { len=t1->type.anyt.valsize;
      at1=ATT_BINARY;
    }
    else if (aflag & DB_OBJ)
      { flags|=UNT_DESTDB;  len=CI->glob_db_specif.length; }
    else /* len necessary for passing untyped db data as proc. parameter */
    { len=DIRECT(t1) ? tpsize[at1] : t1->type.anyt.valsize;
      if (len < sizeof(int)) len = sizeof(int);
    }
    if (CHAR_ARR(t2))   /* assignment from a char array */
      at2=TtoATT(t2);
    else if (t2->type.anyt.typecateg==T_BINARY)
      at2=ATT_BINARY;
    else if (eflag & DB_OBJ) flags|=UNT_SRCDB;
    gen(CI, I_UNTASGN);  gen(CI, at1);  gen(CI, at2);
    if (len>255) len=255;
    gen(CI, (uns8)flags);  gen(CI, (uns8)len);
    return len;
  }
  if (DIRECT(t1) && (at1>=ATT_FIRSTSPEC) && (at1<=ATT_LASTSPEC))
    c_error(INCOMPATIBLE_TYPES);   // cannot write info tracking attribute
 /* reading the database attribute: */
  if (DIRECT(t2) && (eflag & DB_OBJ))
  { /* the stored value has not beed read from database yet ->
       -> it is a var. length attribute or a multiattribute */
    if (DIRECT(t1) && (aflag & DB_OBJ))  /* DB to DB */
    { if (!(aflag & MULT_OBJ) && (eflag & MULT_OBJ)) c_error(INCOMPATIBLE_TYPES);
      /* transfer "attribute into multiattribute" allowed (table modif. needs this) */
      if (at1 != at2)
        if ((!is_string(at1) || !is_string(at2) && !IS_HEAP_TYPE(at2)) &&
            (!is_string(at2) || !is_string(at1) && !IS_HEAP_TYPE(at1)) &&
            (at1!=ATT_RASTER || at2!=ATT_NOSPEC) &&
            (at2!=ATT_RASTER || at1!=ATT_NOSPEC))
          c_error(INCOMPATIBLE_TYPES);
      gen(CI,I_TRANSF_A); /* can transfer same types, strings to strings ... */
      return 0; /* strings to var-len & vice versa */
    }
    else if (!DIRECT(t1) ||  /* structure|array := var_len_atr[start, size]; */
             STR_ARR(t1))     /* string          := var_len_atr[start, size]; */
    { if (is_parameter || (ATT_TYPE(t2)!=ATT_INTERVAL)) c_error(INCOMPATIBLE_TYPES);
      if (STR_ARR(t1))   /* may but need not be COMPOSED */
      { gen(CI,(uns8)I_DBREAD_INTS);
        gen2(CI,(uns16)(DIRECT(t1) ? CI->glob_db_specif.length : t1->type.anyt.valsize-1));
      }
      else gen(CI,(uns8)I_DBREAD_INT);
      return 0;
    }
  }

  if (t1==&att_nil) { t1=t2;  at1=at2; }
  if (!DIRECT(t1))   /* destination cannot be from the database */
  { int extent = t1->type.anyt.valsize;
    if (CHAR_ARR(t1))   /* assignment to a char array */
    { if (DIRECT(t2) && (DIRTYPE(t2)==ATT_CHAR))  /* char to string ... */
        { gen(CI,I_CHAR2STR); t2=&att_string; }  /* ... conversion */
      if (STR_ARR(t2))
      { gen(CI, (uns8)(is_parameter ? I_P_STORE_STR : I_STORE_STR));
        gen2(CI, (uns16)extent);
      }
      else c_error(INCOMPATIBLE_TYPES);
    }
    else
    { typetest(CI,t1,t2, FALSE);
      if (t1->type.anyt.typecateg==T_BINARY)
      { gen(CI, (uns8)(is_parameter ? I_P_STORE : I_STORE));
        gen4(CI, extent);
      }
      else if (t1->type.anyt.typecateg==T_PTR || t1->type.anyt.typecateg==T_PTRFWD)
        GEN(CI,(uns8)(is_parameter ? I_P_STOREPTR : I_STOREPTR))
      else if (t1->type.anyt.typecateg==T_RECCUR)
      { gen(CI,(uns8)(is_parameter ? I_P_STORE4 : I_STORE4)); 
        if (is_parameter)
          { geniconst(CI, 0);  gen(CI, I_P_STORE4);  geniconst(CI, 0);  gen(CI, I_P_STORE4); }  // allocated 3*4 bytes space in the stack frame
      }
      else
      { if (t1->type.anyt.typecateg==T_VIEW)
          c_error(INCOMPATIBLE_TYPES); // assignment not allowed!
        gen(CI, (uns8)(is_parameter ? I_P_STORE : I_STORE));
        gen4(CI, extent);
      }
    }
    return extent;
  }

 /* now: destination is direct, source is not from the database */
  if ((at1==ATT_CHAR) && STR_ARR(t2))  /* string conv. to char */
    { gen(CI,I_LOAD1); at2=ATT_CHAR; t2=&att_char; }

  if (!DIRECT(t2))
  { if ((aflag & DB_OBJ) && (ATT_TYPE(t1)==ATT_INTERVAL))
    { if (CHAR_ARR(t2))
        GEN(CI,I_DBWRITE_INTS)     /* var_len_atr[start, size] := array */
      else gen(CI,I_DBWRITE_INT);  /* var_len_atr[start, size] := struct */
      return 0;
    }
    if (is_string(at1) && CHAR_ARR(t2)) goto ass_ok;
    if (at1==ATT_BINARY && t2->type.anyt.typecateg==T_BINARY) goto ass_ok;
    c_error(INCOMPATIBLE_TYPES);
  }
 /* now: both objects are direct, source is not from the database */
  if ((aflag & DB_OBJ) && is_string(at2)) /* string to database special cases */
  { if (at1==ATT_INTERVAL)
    { gen(CI,I_DBWRITE_INTS);   /* var_len_atr[start, size] := string */
      return 0;
    }
    else if (IS_HEAP_TYPE(at1)) /* string to text in table modify */
    { geniconst(CI,0);    gen(CI,I_SWAP);
      geniconst(CI,-1);   gen(CI,I_SWAP);  // -1 = store the whole string
      gen(CI,I_DBWRITE_INTS);   /* var_len_atr := string */
      return 0;
    }
  }

  if ((aflag & MULT_OBJ) != (eflag & MULT_OBJ)) c_error(INCOMPATIBLE_TYPES);

 /* simple type conversions (int,money->float, int->money) and tests */
  if (!(eflag & MULT_OBJ)) convtype(CI, at1, at2);
  else if (at1!=at2) c_error(INCOMPATIBLE_TYPES);

 ass_ok:
  if (aflag & DB_OBJ)  /* writing to the database: */
  { if      (ATT_TYPE(t1)==ATT_INDNUM) GEN(CI,I_DBWRITE_NUM)
    else if (ATT_TYPE(t1)==ATT_VARLEN) GEN(CI,I_DBWRITE_LEN)
    else if (SPECIFLEN(at1)) { gen(CI,I_DBWRITES);  gen(CI,(uns8)CI->glob_db_specif.length); }
    else                     { gen(CI,I_DBWRITE);   gen(CI,(uns8)tpsize[at1]);    }
    return 0;
  }
  else  /* assignment to a non-structured variable in memory */
  { int extent;
    if (aflag & CACHED_ATR)  /* writing to a cached view attribute: */
    { gen(CI, I_SWAP);  gen(CI, I_LOAD_CACHE_ADDR_CH);  gen(CI, I_SWAP);
      if (is_string(at1))
      { gen(CI, I_STORE_STR);
        gen2(CI, CI->glob_db_specif.length+1);
        return 0;
      }
    }
    if (at1==ATT_BINARY)  // value parameter of Waits_for_me()
    { gen(CI,(uns8)(is_parameter ? I_P_STORE : I_STORE));
      extent = CI->glob_db_specif.length;
      gen4(CI, extent);
    }
    else if (at1==ATT_NIL)
    { gen(CI,(uns8)(is_parameter ? I_P_STOREPTR : I_STOREPTR)); 
      extent = sizeof(void*);
    }
    else if (at1==ATT_CURSOR)
    { gen(CI,(uns8)(is_parameter ? I_P_STORE4 : I_STORE4)); 
      extent = sizeof(tobjnum);  //3*sizeof(tobjnum);
      //if (is_parameter)
      //  { geniconst(CI, 0);  gen(CI, I_P_STORE4);  geniconst(CI, 0);  gen(CI, I_P_STORE4); }  // allocated 3*4 bytes space in the stack frame
    }
    else 
    { extent=tpsize[at1];
      switch (extent)
      { case 1: gen(CI,(uns8)(is_parameter ? I_P_STORE2 : I_STORE1)); break;
                    /* minimal parameter size is 2!! */
        case 2: gen(CI,(uns8)(is_parameter ? I_P_STORE2 : I_STORE2)); break;
        case 4: gen(CI,(uns8)(is_parameter ? I_P_STORE4 : I_STORE4)); break;
        case 6: gen(CI,(uns8)(is_parameter ? I_P_STORE6 : I_STORE6)); break;
        case 8: gen(CI,(uns8)(is_parameter ? I_P_STORE8 : I_STORE8)); break;
        case 0: /* unlimited string ref. parameter */
                c_error(INCOMPATIBLE_TYPES);  break;
      }
    }
    return extent;
  }
}

/*************************** procedure and function call ********************/
static typeobj string_value_param_type =
  { O_TYPE, { { T_ARRAY, 256, 1, 255, &att_char }}};

#define ACTION_NUM    50
#define STRCOPY_NUM  126
#define STRTRIM_NUM  134
#define DISPOSE_NUM  142
#define NEW_NUM      143

#define MAX_REAL_PARAMS 12

static typeobj * call_gen(CIp_t CI, object * sb)  /* generates the call of the subroutine sb */
/* the entry address is already on the stack */
{ int i;  uns8 tp, cl;  typeobj * tobj, * obj, * valtype;
  BOOL firstpar;  uns8 param_number;  BOOL ispar, type_overload = FALSE;
  objdef * od;  BOOL firststring=TRUE, view_open_action;
  typeobj * retvaltype;
#ifdef __ia64__
  unsigned real_param_offset[MAX_REAL_PARAMS+1];  unsigned real_param_count;
  real_param_count=0;  real_param_offset[real_param_count]=0;
#endif  
  if (sb->any.categ==O_SPROC)
  { gen(CI, I_PREPCALL);  gen4(CI, sb->proc.localsize*sizeof(void*)/4);
    retvaltype = sb->proc.retvaltype ? simple_types[sb->proc.retvaltype] : NULL;
    if (sb->proc.sprocnum==STRCOPY_NUM || sb->proc.sprocnum==STRTRIM_NUM)
      type_overload=TRUE;
  }
  else if (sb->any.categ==O_METHOD)  
    retvaltype = sb->proc.retvaltype ? simple_types[sb->proc.retvaltype] : NULL;
  else // user-defined subroutine O_FUNCT, O_PROC
  { gen(CI, I_PREPCALL);  gen4(CI, sb->subr.localsize);
    retvaltype=sb->subr.retvaltype;
  }
   /* it will place the new frame address on the stack */
  if (CI->cursym=='(')
    { ispar=TRUE;  next_sym(CI); }
  else ispar=FALSE;
  firstpar=TRUE;
  param_number=0;  
  do // cycle on parameters
  {// find the description of the next formal parameter:
    if (sb->any.categ==O_SPROC || sb->any.categ==O_METHOD)
    { if (!(cl=sb->proc.pars[param_number].categ)) break;  /* to param_end */
      tp=sb->proc.pars[param_number].dtype;
      if (is_string(tp) && (cl==O_VALPAR))
        tobj=&string_value_param_type;
      else tobj=simple_types[tp];
    }
    else
    { od=sb->subr.locals->objects;
      i=0;
      do
      { if (i>=sb->subr.locals->objnum) goto param_end;
        cl=od[i].descr->any.categ;
        if ((cl==O_VALPAR) || (cl==O_REFPAR))
          if (od[i].descr->var.ordr==param_number) break;
        i++;
      } while (TRUE);
      tobj=od[i].descr->var.type;
    }
    if (!ispar) c_error(LEFT_PARENT_EXPECTED);
    if (!firstpar)
      test_and_next(CI,',', COMMA_EXPECTED);
    if ((CI->comp_flags & COMP_FLAG_VIEW) && sb->any.categ==O_SPROC && sb->proc.sprocnum==ACTION_NUM)  // generate implicit parameters: size, constructor flag
    { if (!param_number)
        view_open_action = CI->cursym==S_INTEGER && 
         (CI->curr_int==ACT_VIEW_OPEN || CI->curr_int==ACT_VIEW_TOGGLE || CI->curr_int==ACT_VIEW_SYNOPEN || 
          CI->curr_int==ACT_VIEW_MODOPEN || CI->curr_int==ACT_VIEW_MODSYNOPEN || CI->curr_int==ACT_INS_OPEN || 
          CI->curr_int==ACT_RELATE || CI->curr_int==ACT_RELSYN);
      else if (param_number==1)
        if (CI->univ_ptr && view_open_action && CI->cursym==S_STRING)
          strmaxcpy((tptr)CI->univ_ptr, CI->curr_string(), sizeof(tobjname));
    }
   // evaluate the actual parametr and assign its value:
    if (cl==O_VALPAR)
    { int eflag;
      valtype = expression(CI, &eflag);
      if (sb->proc.sprocnum==DISPOSE_NUM)  // generate destructor call on param
        if (valtype->type.anyt.typecateg==T_PTR || valtype->type.anyt.typecateg==T_PTRFWD)
          if (valtype->type.ptr.domaintype->type.anyt.typecateg==T_VIEW)
            { gen(CI, I_DUP);  gen(CI, I_DESTRUCT); }
      int extent = assignment(CI, tobj, 0, valtype, eflag, TRUE);
#ifdef __ia64__
      if (valtype==&att_float)
      { real_param_count++;  
        if (real_param_count > MAX_REAL_PARAMS) c_error(TOO_MANY_PARAMS);
        real_param_offset[real_param_count]=real_param_offset[real_param_count-1]; 
      }
      real_param_offset[real_param_count] += ((extent - 1) / sizeof(void*) + 1) * sizeof(void*);
#endif
    }
    else /* if (cl==O_REFPAR) */
    { int aflag;
      obj=selector(CI, SEL_REFPAR, aflag);
      if (tobj==&att_dbrec)  /* assigning reference to db attrib */
      { if (!DIRECT(obj) || (aflag & (DB_OBJ | CACHED_ATR))==0)
          c_error(INCOMPATIBLE_TYPES);
        gen(CI, I_STORE_DB_REF);
        int type_flags = DIRTYPE(obj) | aflag;
        geniconst(CI, type_flags);  gen(CI, I_P_STOREPTR); // incl. flags
      }
      else  /* normal reference parameter */
      {/* allow passing attribute strings as ref. params to strcat: */
        if (DIRECT(obj))  /* simple type variable */
          if (obj==&att_string && !(aflag & MULT_OBJ))
            if (aflag & DB_OBJ)  /* variable set by the selector */
            { gen(CI,I_DBREADS);   /* reading and pushing the data pointer */
              aflag=0;  /* database flag removed */
            }
            else if (aflag & CACHED_ATR)
            { gen(CI, I_LOAD_CACHE_TEST);
              aflag=0;  /* cache flag removed */
            }
        if ((aflag & DB_OBJ) && DIRTYPE(obj)==ATT_UNTYPED)
        {
        }
       /* check type and assign reference: */
        typetest(CI, tobj, obj, TRUE);
        if (IS_STRING_OBJ(obj))
          if (firststring)
            { gen(CI,I_MOVESTRING);  firststring=FALSE; }
        gen(CI,I_P_STOREPTR);
        if (type_overload)
          { retvaltype=obj;  type_overload=FALSE; }
      }
#ifdef __ia64__
      real_param_offset[real_param_count] += sizeof(void*);
#endif
    }
    firstpar=FALSE;
    param_number++;   /* to the next parameter */
  } while (TRUE);
 param_end:
  if (ispar)
    if (CI->cursym==',') c_error(TOO_MANY_PARAMS);
    else test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
  if (sb->any.categ==O_SPROC)
    if (sb->proc.sprocnum==NEW_NUM)  // generate implicit parameters: size, constructor flag
    { if (obj->type.anyt.typecateg!=T_PTR && obj->type.anyt.typecateg!=T_PTRFWD)
        c_error(INCOMPATIBLE_TYPES);
      geniconst(CI, ipj_typesize(obj->type.ptr.domaintype));
      gen(CI,I_P_STORE4);
      gen(CI, I_SCALL);
      if (obj->type.ptr.domaintype->type.anyt.typecateg==T_VIEW)
      { gen(CI, I_RESTORE);   // retrieves the allocated data pointer
        gen(CI, I_CONSTRUCT);
        genstr(CI, obj->type.ptr.domaintype->type.view.viewname);
      } 
    }
#ifdef __ia64__
 // generate the description of real parameters:
  for (i=0;  i<real_param_count;  i++)
    { gen(CI, I_REAL_PAR_OFFS);  gen(CI, (uns8)i);  gen4(CI, real_param_offset[i]); }
#endif
  CI->glob_db_specif.set_null();
  if (retvaltype==&att_string)
    CI->glob_db_specif.length=254;
  return retvaltype;
}

/***************************** operations ***********************************/
BOOL relation(symbol oper)
{ return oper=='<'     ||oper=='='         ||oper=='>'       ||
         oper==S_NOT_EQ||oper==S_LESS_OR_EQ||oper==S_GR_OR_EQ||
         oper==S_PREFIX||oper==S_SUBSTR    ||oper=='~'       ||
         oper==S_DDOT  ||oper==S_PAGENUM;
}

int opindex(symbol op)
{ switch (op)
  { case '<': return 0;
    case '=': return 1;
    case '>': return 2;
    case S_LESS_OR_EQ: return 3;
    case S_NOT_EQ:     return 4;
    case S_GR_OR_EQ:   return 5;

    case '+':                return 6;
    case '-': case '~':      return 7;
    case '*': case S_SUBSTR: return 8;
    case S_PREFIX: case S_PAGENUM:   /* negated prefix */
    case '/':
         default:                 return 9;
  }
}

typeobj * unary_op(CIp_t CI, typeobj * arg, int eflag, symbol oper)
{ unsigned category;
  if (!DIRECT(arg)) c_error(INCOMPATIBLE_TYPES);
  if (eflag & (DB_OBJ | MULT_OBJ | CACHED_ATR)) c_error(INCOMPATIBLE_TYPES);
  category=DIRTYPE(arg);
  if (category==ATT_INT16)
  { gen(CI,I_T16TO32);
    category=ATT_INT32;  arg=&att_int32;
  }
  else if (category==ATT_INT8)
  { gen(CI,I_T8TO32);
    category=ATT_INT32;  arg=&att_int32;
  }
  if (oper=='-')
  { if      (category==ATT_INT32) GEN(CI,I_INTNEG)
    else if (category==ATT_MONEY) GEN(CI,I_MONEYNEG)
    else if (category==ATT_INT64) GEN(CI,I_I64NEG)
    else if (category==ATT_FLOAT) GEN(CI,I_REALNEG)
    else c_error(MUST_BE_NUMERIC);
  }
  else if (oper==S_NOT)
  { if      (category==ATT_BOOLEAN) GEN(CI,I_BOOLNEG)
    else if (category==ATT_INT32)   GEN(CI,I_NOT_BITWISE)
    else c_error(MUST_BE_BOOL);
  }
  /* there are no other operators */
  return arg;
}

int TtoATT(typeobj * tobj)
// Converts typeobj into att type or 0 if not convertible
// Does not return type|0x80 for multiobjects any more!
{ if (DIRECT(tobj)) return tobj->type.simple.typenum;
  switch (tobj->type.anyt.typecateg)
  { case T_STR:    return ATT_STRING;
    case T_BINARY: return ATT_BINARY;
    case T_ARRAY:
      return tobj->type.array.elemtype==&att_char ? ATT_STRING : 0;
  }
  return 0;
}

t_specif T2specif(typeobj * tobj)
{ if (DIRECT(tobj)) 
    return ATT_TYPE(tobj)==ATT_MONEY ? t_specif(2,9) : t_specif();  
  switch (tobj->type.anyt.typecateg)
  { case T_STR:    return tobj->type.string.specif;
    case T_BINARY: return t_specif(tobj->type.anyt.valsize,   0, FALSE, FALSE);
    case T_ARRAY:  return t_specif(tobj->type.anyt.valsize-1, 0, FALSE, FALSE);  // char array supposed
  }
  return 0;
}

typeobj * binary_op(CIp_t CI, typeobj * arg1, int aflag1, typeobj * arg2, int aflag2, symbol oper)
{ unsigned cat1, cat2;  uns8 ind;
 /* DB_OBJECT or MULT_OBJECT will generate error, only string may be COMPOSED */
  if (DIRECT(arg1))
  { if (aflag1 & MULT_OBJ)
      { if (oper!=S_DDOT) c_error(INCOMPATIBLE_TYPES); }
    else if (aflag1 & DB_OBJ)
      if ((ATT_TYPE(arg1)==ATT_TEXT) && DIRECT(arg2) &&
          is_string(DIRTYPE(arg2)) && (oper==S_SUBSTR))
        { gen(CI,I_TEXTSUBSTR);  return &att_boolean; }
      else c_error(INCOMPATIBLE_TYPES);
    cat1=ATT_TYPE(arg1);
  }
  else
  { if ((arg1->type.anyt.typecateg==T_PTR || arg1->type.anyt.typecateg==T_PTRFWD) && 
        (oper=='=' || oper==S_NOT_EQ) &&
        (arg2==&att_nil || arg2->type.anyt.typecateg==T_PTR || arg2->type.anyt.typecateg==T_PTRFWD))
      { cat1=cat2=ATT_INT32;  arg1=arg2=&att_int32; }
    else if (CHAR_ARR(arg1)) { cat1=TtoATT(arg1);  CI->glob_db_specif=T2specif(arg1); }
    else c_error(INCOMPATIBLE_TYPES);
  }
  if (DIRECT(arg2))
  { if (aflag2 & (DB_OBJ| MULT_OBJ)) c_error(INCOMPATIBLE_TYPES);
    cat2=DIRTYPE(arg2);
  }
  else
  { if ((arg2->type.anyt.typecateg==T_PTR || arg2->type.anyt.typecateg==T_PTRFWD) && 
        (oper=='=' || oper==S_NOT_EQ) &&
        (arg1==&att_nil || arg1->type.anyt.typecateg==T_PTR || arg1->type.anyt.typecateg==T_PTRFWD))
      { cat1=cat2=ATT_INT32;  arg1=arg2=&att_int32; }
    else if (CHAR_ARR(arg2)) { cat2=TtoATT(arg2);  CI->glob_db_specif=T2specif(arg2); }
    else c_error(INCOMPATIBLE_TYPES);
  }

 // converting to INT32:
  if (cat1==ATT_INT16)
    { if (!(aflag1 & MULT_OBJ)) gen(CI,I_N16TO32);  cat1=ATT_INT32; }
  else if (cat1==ATT_INT8)
    { if (!(aflag1 & MULT_OBJ)) gen(CI,I_N8TO32);   cat1=ATT_INT32; }
  if (cat2==ATT_INT16)
    { gen(CI,I_T16TO32);  cat2=ATT_INT32; }
  else if (cat2==ATT_INT8)
    { gen(CI,I_T8TO32);   cat2=ATT_INT32; }
  if (((cat1==ATT_PTR) || (cat1==ATT_BIPTR) || (cat1==ATT_INT32)) &&
      ((cat2==ATT_PTR) || (cat2==ATT_BIPTR) || (cat2==ATT_INT32)) &&
      ((oper=='=') || (oper==S_NOT_EQ)) ) cat1=cat2=ATT_INT32;

  if (oper==S_DDOT)
  { if (!(aflag1 & MULT_OBJ) ||
        cat1 != cat2 && (!is_string(cat1) || !is_string(cat2)))
      c_error(INCOMPATIBLE_TYPES);
    gen(CI,I_CONTAINS);
    gen(CI,is_string(cat2));
    return &att_boolean;
  }

  if ((oper==S_OR)||(oper==S_AND))
  { if ((cat1==ATT_INT32)  && (cat2==ATT_INT32))
      GEN(CI,(uns8)((oper==S_OR) ? I_OR_BITWISE : I_AND_BITWISE))
    else if ((cat1==ATT_BOOLEAN)&& (cat2==ATT_BOOLEAN))
      GEN(CI,(uns8)((oper==S_OR) ? I_OR         : I_AND))
    else c_error(MUST_BE_BOOL);
    return(arg1);
  }
  if ((oper==S_DIV)||(oper==S_MOD))
  { if (cat1==ATT_INT64) 
    { if (cat2==ATT_INT32) { GEN(CI,I_T32TO64); cat2=ATT_INT64; }
      else if (cat2!=ATT_INT64) c_error(MUST_BE_INTEGER);
    }
    else if (cat2==ATT_INT64) 
    { if (cat1==ATT_INT32) { gen(CI,I_SWAP);  GEN(CI,I_T32TO64);  gen(CI,I_SWAP);  cat1=ATT_INT64; }
      else if (cat1!=ATT_INT64) c_error(MUST_BE_INTEGER);
    }
    else
      if (cat1!=ATT_INT32 || cat2!=ATT_INT32) c_error(MUST_BE_INTEGER);
    if (cat1==ATT_INT64) 
      gen(CI, (uns8)((oper==S_DIV) ? I_I64DIV : I_I64MOD))
    else
      gen(CI, (uns8)((oper==S_DIV) ? I_INTDIV : I_INTMOD));
    return simple_types[cat1];
  }
 /* conversion to float (on relation float 2 money) */
  if ((cat1==ATT_FLOAT)||(oper=='/'))
  { if      (cat2==ATT_INT32) { GEN(CI,I_TOSI2F);  cat2=ATT_FLOAT; }
    else if (cat2==ATT_INT64) { GEN(CI,I_T64TOF);  cat2=ATT_FLOAT; }
    else if (cat2==ATT_MONEY)
      if (relation(oper)) { gen(CI,I_SWAP);  gen(CI,I_TOSF2M);  gen(CI,I_SWAP);  cat1=ATT_MONEY; }
      else { GEN(CI,I_TOSM2F);  cat2=ATT_FLOAT; }
    else if (cat2!=ATT_FLOAT) c_error(INCOMPATIBLE_TYPES);
  }
  if ((cat2==ATT_FLOAT)||(oper=='/'))
  { if      (cat1==ATT_INT32) { GEN(CI,I_NOSI2F);  cat1=ATT_FLOAT; }
    else if (cat1==ATT_INT64) { gen(CI,I_SWAP);  GEN(CI,I_T64TOF);  gen(CI,I_SWAP);  cat2=ATT_FLOAT; }
    else if (cat1==ATT_MONEY)
      if (relation(oper)) { gen(CI,I_TOSF2M);  cat2=ATT_MONEY; }
      else { GEN(CI,I_NOSM2F);  cat1=ATT_FLOAT; }
    else if (cat1!=ATT_FLOAT) c_error(INCOMPATIBLE_TYPES);
  }
  if (cat1==ATT_FLOAT)   /* then cat2==ATT_FLOAT */
  { gen(CI,(uns8)(I_FOPER+opindex(oper)));
    return relation(oper) ? &att_boolean : &att_float;
  }
 /* conversion to money */
  if (cat1==ATT_MONEY)
  { if (cat2==ATT_INT32)
    { if (oper!='*') gen(CI,I_TOSI2M)
      else gen(CI,I_T32TO64);
      cat2=ATT_MONEY;
    }
    else if (cat2==ATT_INT64)
    { if (oper!='*') { GEN(CI,I_T64TOF);  gen(CI,I_TOSF2M); }
      cat2=ATT_MONEY;
    }
    else if (cat2!=ATT_MONEY) c_error(INCOMPATIBLE_TYPES);
    else if (oper=='*') c_error(MONEY_MULTIPLY);

  }
  if (cat2==ATT_MONEY)
  { if (cat1==ATT_INT32)
    { if (oper=='*') { GEN(CI,I_SWAP);  gen(CI,I_T32TO64); }
      else gen(CI,I_NOSI2M);
      cat1=ATT_MONEY;
    }
    else if (cat1==ATT_INT64)
    { if (oper=='*') GEN(CI,I_SWAP) 
      else { GEN(CI,I_SWAP);  GEN(CI,I_T64TOF);  gen(CI,I_TOSF2M);  GEN(CI,I_SWAP); }
      cat1=ATT_MONEY;
    }
    else if (cat1!=ATT_MONEY) c_error(INCOMPATIBLE_TYPES);
  }
  if (cat1==ATT_MONEY)   /* then cat2==ATT_MONEY */
  { gen(CI,(uns8)(I_MOPER+opindex(oper)));
    return relation(oper) ? &att_boolean : &att_money;
  }
 // conversion to INT64 
  if (cat1==ATT_INT64)
  { if      (cat2==ATT_INT32) { GEN(CI,I_T32TO64);  cat2=ATT_INT64; }
    else if (cat2!=ATT_INT64) c_error(INCOMPATIBLE_TYPES);
  }
  else if (cat2==ATT_INT64)
  { if      (cat1==ATT_INT32) { gen(CI,I_SWAP);  GEN(CI,I_T32TO64);  gen(CI,I_SWAP);  cat2=ATT_INT64; }
    else if (cat1!=ATT_INT64) c_error(INCOMPATIBLE_TYPES);
  }
  if (cat1==ATT_INT64)
  { gen(CI,(uns8)(I_I64OPER+opindex(oper)));
    return relation(oper) ? &att_boolean : &att_int64;
  }

  if (is_string(cat1) && (cat2==ATT_CHAR))
    { gen(CI,I_CHAR2STR);  cat2=ATT_STRING; }
  else if (is_string(cat2) && (cat1==ATT_CHAR))
    { gen(CI,I_SWAP);  gen(CI,I_CHAR2STR);  gen(CI,I_SWAP);  cat1=ATT_STRING; }

  /* no argument of ATT_FLOAT nor ATT_MONEY type */
  if (cat1==ATT_DATE || cat1==ATT_TIME || cat1==ATT_TIMESTAMP)
    if (cat2==cat1)
      if (relation(oper))
        cat1=cat2=ATT_INT32;  /* relations like integer */
      else if (oper=='-')
      { gen(CI, (uns8)(cat1==ATT_DATE ? I_DATMINUS : cat1==ATT_TIME ? I_INTMINUS : I_TSMINUS) );
        return &att_int32;
      }
      else c_error(INCOMPATIBLE_TYPES);
    else
    { if (cat2!=ATT_INT32) c_error(MUST_BE_INTEGER);
      if (oper=='-') GEN(CI,I_INTNEG)
      else if (oper!='+') c_error(BAD_OPERATION);
      gen(CI, (uns8)(cat1==ATT_DATE ? I_DATPLUS : cat1==ATT_TIME ? I_INTPLUS : I_TSPLUS) );
      return arg1;
    }

  if (is_string(cat1) && is_string(cat2) && (relation(oper) || (oper=='+')))
  { ind=opindex(oper);
    if (ind>7) gen(CI,I_SWAP);    /* prefix, substring */
    gen(CI,(uns8)(I_STRREL+ind));
    t_specif specif = CI->glob_db_specif;
    if (oper=='+') 
    { CI->glob_db_specif.length+=CI->prev_db_specif.length;  // updating CI->glob_db_specif 
      return &att_string;  
    }
    gen2(CI, specif.stringprop_pck);
    if (oper==S_PAGENUM) gen(CI,I_BOOLNEG);
    return &att_boolean;
  }

  if (cat1==ATT_BINARY && cat2==ATT_BINARY)
  { ind=opindex(oper);
    if (ind >= 6) c_error(BAD_OPERATION);
    gen(CI, I_BINARYOP+ind);
    gen(CI, CI->prev_db_specif.length<CI->glob_db_specif.length ? CI->prev_db_specif.length : CI->glob_db_specif.length);
    return &att_boolean;
  }

 /* integer, boolean, char only */
  if ((oper==S_PREFIX) || (oper==S_SUBSTR) || (oper=='~'))
    c_error(INCOMPATIBLE_TYPES);
  if ((cat1==ATT_INT32)&&(cat2==ATT_INT32)||  /* oper need not be checked */
      (cat1==ATT_BOOLEAN)&&(cat2==ATT_BOOLEAN)&&relation(oper) || /* BOOL like int */
      (cat1==ATT_CHAR)&&(cat2==ATT_CHAR)&&relation(oper)) /* char like int */
    { gen(CI,(uns8)(I_INTOPER+opindex(oper)));
      return relation(oper) ? &att_boolean : &att_int32;
    }
  c_error(INCOMPATIBLE_TYPES);
return NULL;}
/***************************** expression ***********************************/
void int_check(CIp_t CI, typeobj * tobj)
{ 
       if (tobj==&att_int16)  gen(CI, I_T16TO32)
  else if (tobj==&att_int8)   gen(CI, I_T8TO32)
  else if (tobj!=&att_int32)
    c_error(MUST_BE_INTEGER);
/* Conversion necessary due to negative short indicies etc. */
}

void int_expression(CIp_t CI)
{ int aflag;
  typeobj * tobj = expression(CI, &aflag);
  if (aflag & (DB_OBJ | MULT_OBJ | CACHED_ATR)) c_error(MUST_BE_INTEGER);
  int_check(CI, tobj);
}

static int notice_code(CIp_t CI, tptr code, uns16 codesize, uns8 type, uns16 valsize)
/* compiler passes code & expression type, function returns code index */
{ aggr_info * pt, ** ppt;  int ind=0;
  /*if (!CI->aggr) c_error(BAD_AGGREG);*/
 // look to see if thesame code is already registered:
  ppt=&CI->aggr;
  while (*ppt)
  { if ((*ppt)->codesize==codesize)
      if (!memcmp((*ppt)->vls,code,codesize)) return ind;
    ind++;
    ppt=&((*ppt)->next_info);
  }
 // register the code in a new agg_info and add it to the list:
  if (valsize<4) valsize=4;  /* for short integer sum & avg */
  int sz=sizeof(aggr_info)+codesize+3*GR_LEVEL_CNT*valsize;
  if (!(pt=(aggr_info*)corealloc(sz,OW_PRINT))) c_error(OUT_OF_MEMORY);
  pt->valsize=valsize;
  pt->next_info=NULL;  pt->codesize=codesize;  pt->type=type;
  memcpy(pt->vls, code, codesize);
  pt->specif = CI->glob_db_specif;
  *ppt=pt;
  return ind;
}

static typeobj * gen_aggreg(CIp_t CI, uns8 ag_type)
/* may crash when DB_OUT for code is used */
{ uns32 code_start;
  typeobj * tobj;  uns8 tp;  uns16 sz;  int ind, eflag;

  if (CI->inaggreg) c_error(BAD_AGGREG);
  CI->inaggreg=1;
  code_start=CI->code_offset;
  if (ag_type==AG_TYPE_CNT)
  { tp=0; sz=1; 
    CI->glob_db_specif.set_null();
  }
  else
  { test_and_next(CI,'(',LEFT_PARENT_EXPECTED);
    tobj=expression(CI, &eflag);
    if (!DIRECT(tobj) || (eflag & MULT_OBJ)) c_error(BAD_AGGREG);
    tp=DIRTYPE(tobj);
    if (tp>ATT_TIME || tp==ATT_BOOLEAN || tp==ATT_BINARY ||
        ag_type>AG_TYPE_MAX && (is_string(tp)||tp==ATT_CHAR||tp==ATT_DATE))
      c_error(BAD_AGGREG);
    sz=is_string(tp) ? CI->glob_db_specif.length+1 : tpsize[tp];
  }
  gen(CI,I_STOP);
  if (CI->univ_code)  /* NULL when DUMMY compilation of edited view item performed */
    ind=notice_code(CI, CI->univ_code+(unsigned)(code_start-CI->cb_start),
                                      (uns16)(CI->code_offset-code_start), tp, sz);
  CI->code_offset=code_start;
  gen(CI,I_AGGREG);  gen2(CI, (uns16)ind);
  if (ag_type!=AG_TYPE_AVG) GEN(CI,ag_type)
  else /* compute the average */
  { gen(CI,AG_TYPE_SUM);
         gen(CI,I_AGGREG);  gen2(CI,ind);  gen(CI,AG_TYPE_LCN);
         if (tp==ATT_MONEY) GEN(CI,I_NOSM2F)
         else if (tp!=ATT_FLOAT) gen(CI,I_NOSI2F);
    gen(CI,I_TOSI2F);  gen(CI,I_FDIV);
    tobj=&att_float;
  }
  CI->inaggreg=0;
  if (ag_type==AG_TYPE_CNT) return &att_int32;
  test_and_next(CI,')',RIGHT_PARENT_EXPECTED);
  return tobj;
}

CFNC DllKernel BOOL WINAPI valid_method_property(int ClassNum, int method_num)
{ switch (method_num)
  { case 54: // text
    case CONTROL_VALUE_PROPNUM: // value
    case 72: // isnull
    case 76: // setnull
    case 87: // handle
    case 88: // setfocus
    case 90: // refresh
      return ClassNum!=NO_FRAME;
    case 64: // precision
      return ClassNum==NO_EDI || ClassNum==NO_VAL;
    case 66: // checked
      return ClassNum==NO_CHECK || ClassNum==NO_RADIO;
    case 68: // format
      return ClassNum==NO_EDI || ClassNum==NO_VAL ||
             ClassNum==NO_RASTER || ClassNum==NO_OLE;
    case 73: // open
      return ClassNum==NO_RASTER || ClassNum==NO_OLE || ClassNum==NO_TEXT;
    case 74: // readfile
    case 77: // writefile
      return ClassNum==NO_RASTER || ClassNum==NO_OLE || ClassNum==NO_TEXT || ClassNum==NO_RTF;
    case 70: // page
      return ClassNum==NO_TAB;
    case 75: // resetcombo
      return ClassNum==NO_COMBO || ClassNum==NO_EC;
    case VALUE_LENGTH_PROPNUM: // length, value#
      return ClassNum==NO_RASTER || ClassNum==NO_OLE || ClassNum==NO_TEXT || ClassNum==NO_RTF || ClassNum==NO_EDI;
    case 82: // selindex
      return ClassNum==NO_LIST || ClassNum==NO_COMBO;
    case 85: // calendar
      return ClassNum==NO_VAL || ClassNum==NO_EDI || ClassNum==NO_EC;
    case 89: // selection
      return ClassNum==NO_RTF || ClassNum==NO_EDI || ClassNum==NO_EC;
    case 91:  case 92:  case 93: // signature
      return ClassNum==NO_SIGNATURE;
    case 95: // backcolor (only for general form - not checked here)
      return ClassNum!=NO_VIEW && ClassNum!=NO_RADIO && ClassNum!=NO_CHECK &&
             ClassNum!=NO_DTP && ClassNum!=NO_UPDOWN && ClassNum!=NO_PROGRESS && ClassNum!=NO_TAB;
    case 97: // Textcolor
      return ClassNum!=NO_VIEW && ClassNum!=NO_RASTER && ClassNum!=NO_OLE && ClassNum!=NO_ACTX &&
             ClassNum!=NO_RTF && ClassNum!=NO_SLIDER && ClassNum!=NO_UPDOWN && ClassNum!=NO_PROGRESS &&
             ClassNum!=NO_TAB && ClassNum!=NO_BUTT && ClassNum!=NO_DTP;
    case 101: // Set_font (only for general form - not checked here)
      return ClassNum!=NO_FRAME && ClassNum!=NO_VIEW && ClassNum!=NO_OLE && ClassNum!=NO_ACTX && ClassNum!=NO_BARCODE &&
             ClassNum!=NO_RASTER && ClassNum!=NO_RTF && ClassNum!=NO_SLIDER && ClassNum!=NO_UPDOWN && ClassNum!=NO_PROGRESS;
  }
  return FALSE;
}

#ifdef WINS
//#include <commctrl.h>
//#include <windowsx.h>

#include "dbgprint.h"

static typeobj * call_view_method(CIp_t CI, sel_type seltype, tobjnum objnum, 
                                  tptr name1, tptr name2, tptr name3)
// If name1!=NULL, find any instance of this view.
// If name1==NULL, instance pointer is on TOS.
// objnum!=NOOBJECT iff view object number known and its controls can be accessed.
{ int ID = -1, control_value_type, control_class_num;  GUID guid;  object * obj;
  if (name1) // get name1 pointer to TOS
    { gen(CI,I_STRINGCONST);  gen2(CI,(uns16)strlen(name1));  genstr(CI, name1); }
 // when no name3 specified, view method name prevails over control name (VALUE implicit)
 // ID==-1 iff calling view method or property, calling control method or property otherwise.
  if (!*name3 || !strcmp(name2, "PARINST") || !strcmp(name2, "PARENT")) // control not specified, look for view method
    obj = search1(name2, (objtable*)&view_standard_table);
  else obj=NULL;
  if (obj==NULL) // view method/property not found, try view control name
  { if (objnum==NOOBJECT)
      { SET_ERR_MSG(CI->cdp, name2);  c_error(CONTROLS_NOT_KNOWN); }
    const t_control_info * vwci = (const t_control_info*)cd_get_table_d(CI->cdp, objnum, CATEG_VIEW);
    if (vwci!=NULL)
    { int num = 0;
      while (vwci[num].name[0])
        if (!strcmp(vwci[num].name, name2)) 
        { ID = vwci[num].ID;  control_value_type=vwci[num].type;  
          control_class_num=vwci[num].classnum;  guid=*(GUID*)vwci[num].clsid;
          break;
        }
        else num++;
      release_table_d((const d_table *)vwci);
    }
    if (ID==-1) 
      { SET_ERR_MSG(CI->cdp, name2);  c_error(IDENT_NOT_DEFINED); }
   // control name found, check for View, ActiveX, common methods:
    if (control_class_num==NO_VIEW && *name3)
    {// calling the default method returning instance of the subview
      gen(CI, I_PREPCALL);  gen4(CI, sizeof(void*)+sizeof(int));
      gen(CI, I_SWAP); // instance or name pointer on TOS
      gen(CI, I_P_STORE4);
      geniconst(CI, ID);  gen(CI, I_P_STORE4); 
      gen(CI, name1==NULL ? I_CALLPROP : I_CALLPROP_BYNAME);
      gen2(CI, SUBINST_METHOD_NUMBER);
      gen(CI, ATT_INT32); // returning a pointer
     // recursive call:
      if (cd_Find_object(CI->cdp, name2, CATEG_VIEW, &objnum))
        objnum=*(tobjnum*)&guid;  // name2 not found iff explicit name specified and is different from subview object name
      strcpy(name2, name3);  *name3=0;
      if (CI->cursym=='.')
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        strcpy(name3, CI->curr_name);  next_sym(CI); 
      }
      return call_view_method(CI, seltype, objnum, NULL, name2, name3);
    } // end of subview
   // ActiveX methods & properties:
#ifdef WINS
#if WBVERS<900
    if (control_class_num==NO_ACTX && *name3)
    { BOOL setprop = seltype==SEL_ADDRESS && CI->cursym==S_ASSIGN;
      CAXTypeInfo   MethInfo(name3);
      uns16 calltype;  int eflag;
      LPACTIVEXINFO AcXInfo = CI->AXInfo;
      if (!AcXInfo)
          CreateActiveXInfo(&guid, &AcXInfo);
      if (AcXInfo)
      { if (AcXInfo->GetNameInfo(&MethInfo)==NOERROR)
        { CWBAXParType ParType;
          gen(CI, I_PREPCALL);
          //            inst          ID          DISPID        VarType                        Param          AXParType    Delim CallType
          int parsize = sizeof(void*)+sizeof(int)+sizeof(DWORD)+sizeof(int)+MethInfo.m_ParCnt*(sizeof(double)+sizeof(long))+2*sizeof(int);
          if (setprop) parsize+=sizeof(double)+sizeof(int);
          gen4(CI, parsize);
          gen(CI, I_SWAP); // instance or name pointer on TOS
                                    gen(CI, I_P_STORE4);
          geniconst(CI, ID);        gen(CI, I_P_STORE4); 
          geniconst(CI, MethInfo.m_ID);  gen(CI, I_P_STORE4); 
          int ResType = setprop ? 0 : MethInfo.m_WBType.dtype;
          if (ResType == ATT_UNTYPED)
              ResType  = ATT_STRING;
          geniconst(CI, ResType);   gen(CI, I_P_STORE4); 
          if (MethInfo.m_InvKind & DISPATCH_METHOD)
          { // passing parameters:
            CAXTypeInfo pardescr;  BOOL firstpar=TRUE, firststring=TRUE;
            int parnum = 0;
            while (AcXInfo->GetNextParInfo(MethInfo.m_ID, &pardescr)==NOERROR)
            { parnum++;
              if (firstpar) 
              { if (CI->cursym != '(')
                { if (!(pardescr.m_Flags & PARAMFLAG_FOPT))
                    c_error(LEFT_PARENT_EXPECTED);
                  break;
                }
                next_sym(CI); 
                firstpar=FALSE;  
              }
              else
              { if (CI->cursym != ',')
                { if (!(pardescr.m_Flags & PARAMFLAG_FOPT))
                    c_error(COMMA_EXPECTED);
                  break;
                }
                next_sym(CI); 
              }
              code_addr aux_offset=CI->code_offset + 1;
              ParType.m_VarType = pardescr.m_VarType;
              geniconst(CI, ParType);   gen(CI, I_P_STORE4); 
              if (pardescr.m_WBType.categ==O_VALPAR)
              { typeobj * valtype = expression(CI, &eflag);
                assignment(CI, simple_types[pardescr.m_WBType.dtype], 0, valtype, eflag, TRUE);
                ParType.m_WBType = DIRECT(valtype) ? DIRTYPE(valtype) : *(uns16 *)valtype;
                code_out(CI, (tptr)&ParType, aux_offset, 2);
              }
              else /* if (cl==O_REFPAR) */
              { int aflag;
                typeobj * obj=selector(CI, SEL_REFPAR, aflag);
                typetest(CI, simple_types[pardescr.m_WBType.dtype], obj, TRUE);
                if (IS_STRING_OBJ(obj))
                  if (firststring)
                  { gen(CI,I_MOVESTRING);  firststring=FALSE; }
                gen(CI,I_P_STORE4);
                ParType.m_WBType  = DIRECT(obj) ? DIRTYPE(obj) : *(uns16 *)obj;
                ParType.m_WBType |= PART_BYREF;
                if (ParType.m_VarType != VT_BSTR && !(ParType.m_VarType & (VT_ARRAY | VT_BYREF)))
                    ParType.m_VarType |= VT_BYREF;
                UINT RefType = pardescr.m_RefType & (PARAMFLAG_FIN | PARAMFLAG_FOUT);
                // IF argument neni promenna a parametr je vystupni, chyba
                if (!(aflag & PROJ_VAR) && (RefType & PARAMFLAG_FOUT))
                    c_error(INCOMPATIBLE_TYPES);
                // IF parametr pouze vsupni, neni treba po navratu z volani vracet jeho hodnotu
                if (RefType == PARAMFLAG_FIN || ParType.m_VarType == VT_BSTR)
                    ParType.m_WBType |= PART_INREF;
                code_out(CI, (tptr)&ParType, aux_offset, 4);
              }
              if (!AcXInfo->IsConvertable(ParType.m_WBType, ParType.m_VarType, pardescr.m_utName))
                  c_error(INCOMPATIBLE_TYPES);
            }
            if (!firstpar) test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
            else if (CI->cursym=='(')
            { next_sym(CI);  test_and_next(CI, ')', RIGHT_PARENT_EXPECTED); }
            calltype=DISPATCH_METHOD;
          }
          else
          {
            if (setprop)
            { if (!(MethInfo.m_InvKind & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)))
                c_error(BAD_IDENT_CLASS);
              int proptp; 
              proptp=MethInfo.m_WBType.dtype;
              // assigned value as a special paramater:
              next_sym(CI);
              t_specif aux3=CI->glob_db_specif;
              ParType.m_VarType = MethInfo.m_VarType;
              code_addr aux_offset=CI->code_offset+1;
              geniconst(CI, ParType);   gen(CI, I_P_STORE4); 
              typeobj * tobj=expression(CI, &eflag);
              ParType.m_WBType = DIRECT(tobj) ? DIRTYPE(tobj) : *(uns16 *)tobj;
              code_out(CI, (tptr)&ParType, aux_offset, 2);
              CI->glob_db_specif=aux3;
              if (is_string(proptp))
              { if (tobj==&att_char) gen(CI,I_CHAR2STR)
                else if (!STR_ARR(tobj)) c_error(INCOMPATIBLE_TYPES);
                gen(CI, I_P_STORE4);
              }
              else if (proptp==ATT_BINARY && tobj==&att_binary)
                gen(CI, I_P_STORE4)
              else assignment(CI, simple_types[proptp], 0, tobj, eflag, TRUE);
              calltype=(MethInfo.m_InvKind & ~DISPATCH_PROPERTYGET);
              if (!AcXInfo->IsConvertable(ParType.m_WBType, ParType.m_VarType, MethInfo.m_utName))
                  c_error(INCOMPATIBLE_TYPES);
            }
            else  // evaluating
            { if (!(MethInfo.m_InvKind & DISPATCH_PROPERTYGET)) 
                c_error(BAD_IDENT_CLASS);
              calltype=DISPATCH_PROPERTYGET;
            }
          }
          geniconst(CI, 0);   gen(CI, I_P_STORE4);  // parameter delimiter
          geniconst(CI, calltype);  gen(CI, I_P_STORE4); 
          gen(CI, name1==NULL ? I_CALLPROP : I_CALLPROP_BYNAME);
          gen2(CI, GENERIC_METHOD_NUMBER);
          gen(CI, ResType); // no return FALSE from the assignment
          return simple_types[ResType];
        } // ActiveX method found
        if (AcXInfo && AcXInfo != CI->AXInfo)
            AcXInfo->Release();
      }
    } // end of ActiveX
#endif
#endif
   // not an ActiveX control or not an ActiveX method, look for WinBase methods:
    obj = search1(*name3 ? name3 : "VALUE", (objtable*)&control_standard_table);
    if (obj==NULL)
      { SET_ERR_MSG(CI->cdp, name3);  c_error(IDENT_NOT_DEFINED); }
  }

 // WinBase methods & properties, referring to a control iff ID!=-1:
  if (obj!=NULL)
  {// prepare the call, store common parameters: 
    int method_number = obj->any.categ==O_METHOD ? obj->proc.sprocnum : obj->prop.method_number;
    gen(CI, I_PREPCALL);
    int parsize = sizeof(void*);          // view_dyn * size
    if (ID != -1) 
    { parsize += sizeof(int); // item ID size
      if (!valid_method_property(control_class_num, method_number))
        { SET_ERR_MSG(CI->cdp, name3);  c_error(BAD_METHOD_OR_PROPERTY); }
     // handling special properties: value#, value[offset,length]
      if (obj->any.categ==O_PROPERTY)
      { if (method_number==CONTROL_VALUE_PROPNUM)
          if (CI->cursym=='#')
            { next_sym(CI);  method_number=VALUE_LENGTH_PROPNUM; }
          else if (CI->cursym=='[')
            { next_sym(CI);  method_number=VALUE_INTERVAL_PROPNUM;  parsize+=2*sizeof(int); }
        if (method_number == VALUE_LENGTH_PROPNUM && !IS_HEAP_TYPE(control_value_type))
        { SET_ERR_MSG(CI->cdp, *name3 ? name3 : "#");  c_error(BAD_METHOD_OR_PROPERTY); }
      }
    }
    if (obj->any.categ==O_METHOD)  parsize+=+obj->proc.localsize;
    else if (seltype==SEL_ADDRESS) parsize+=sizeof(double); // must be multiply of 4!
    gen4(CI, parsize);
    gen(CI, I_SWAP); // instance or name pointer on TOS
    gen(CI, I_P_STORE4);
    if (ID != -1)
    { geniconst(CI, ID);  gen(CI, I_P_STORE4); 
      if (method_number==VALUE_INTERVAL_PROPNUM)
      { int_expression(CI);  gen(CI, I_P_STORE4);
        test_and_next(CI,',', COMMA_EXPECTED);
        int_expression(CI);  gen(CI, I_P_STORE4);
        test_and_next(CI,']', RIGHT_BRACE_EXPECTED);
      }
    }
   // store specific parameters:
    if (obj->any.categ==O_PROPERTY)
    {// property value type: fixed or the control value type
      int proptp = obj->prop.dirtype;
      if (method_number==CONTROL_VALUE_PROPNUM)
        proptp=control_value_type;
      else if (method_number==VALUE_LENGTH_PROPNUM)
        proptp=ATT_INT32;
      else if (method_number==VALUE_INTERVAL_PROPNUM)
        proptp = control_value_type==ATT_TEXT ? ATT_STRING : ATT_BINARY;
      if (seltype==SEL_ADDRESS) // assigning value to the property, pass its value to the procedure
      { test_and_next(CI,S_ASSIGN,ASSIGN_EXPECTED);
        t_specif aux3=CI->glob_db_specif;   int eflag;
        typeobj * tobj=expression(CI, &eflag);
        CI->glob_db_specif=aux3;
        if (is_string(proptp))
        { if (tobj==&att_char) gen(CI,I_CHAR2STR)
          else if (!STR_ARR(tobj)) c_error(INCOMPATIBLE_TYPES);
          gen(CI, I_P_STORE4);
        }
        else if (proptp==ATT_BINARY && tobj==&att_binary)
          gen(CI, I_P_STORE4)
        else assignment(CI, simple_types[proptp], 0, tobj, eflag, TRUE);
        proptp=0; // no result type, procedure called
        method_number++;
      }
     // else retrieving the value of the property
      gen(CI, name1==NULL ? I_CALLPROP : I_CALLPROP_BYNAME);  gen2(CI, method_number);  gen(CI, proptp);
      return simple_types[proptp];
    } // O_PROPERTY
    else if (obj->any.categ==O_METHOD)
    { typeobj * tobj = call_gen(CI, obj);
      gen(CI, name1==NULL ? I_CALLPROP : I_CALLPROP_BYNAME);  gen2(CI, method_number);  gen(CI, obj->proc.retvaltype);
      if (method_number==PARINST_METHOD_NUMBER)
      {// recursive call:
        strcpy(name2, name3);  *name3=0;
        if (CI->cursym=='.')
          if (next_sym(CI)==S_IDENT)
            { strcpy(name3, CI->curr_name);  next_sym(CI); }
        return call_view_method(CI, seltype, NOOBJECT, NULL, name2, name3);
      }
      return tobj; 
    }
  }
  SET_ERR_MSG(CI->cdp, name2);  c_error(IDENT_NOT_DEFINED); 
  return NULL; // prevents a warning
}
#else
static typeobj * call_view_method(CIp_t CI, sel_type seltype, tobjnum objnum, 
                                  tptr name1, tptr name2, tptr name3)
{ SET_ERR_MSG(CI->cdp, name2);  c_error(IDENT_NOT_DEFINED); }
#endif

static typeobj * record_elem(CIp_t CI, typeobj * tobj, char * name, char * namex, sel_type seltype, BOOL * proc_called)
{ *proc_called=FALSE;
  do
  { if (tobj->type.anyt.typecateg==T_RECORD)
    { object * obj = search1(name, tobj->type.record.items);
      if (obj==NULL)
        { SET_ERR_MSG(CI->cdp, name);  c_error(IDENT_NOT_DEFINED); }
      if (obj->var.offset)
        { geniconst(CI, obj->var.offset);  gen(CI,I_INTPLUS); }
      tobj=obj->var.type;
      if (*namex) { strcpy(name, namex);  *namex=0; }
      else break;
    }
    else if (tobj->type.anyt.typecateg==T_VIEW)
    { if (seltype==SEL_REFPAR) 
        { SET_ERR_MSG(CI->cdp, name);  c_error(BAD_IDENT_CLASS); }
      if (!*namex && CI->cursym=='.')
      { next_and_test(CI, S_IDENT, ITEM_IDENT_EXPECTED);
        strcpy(namex, CI->curr_name);  next_sym(CI);
      }
      *proc_called=TRUE;
      return call_view_method(CI, seltype, tobj->type.view.objnum, NULL, name, namex);
    }
    else c_error(NOT_A_REC);
  }
  while (TRUE);
  return tobj;
}

struct x_typeobj  /* description of a form type */
{ uns8 categ;
  viewtype type;
};

#if WBVERS<900
static x_typeobj untyped_form_type = { O_TYPE, { T_VIEW, sizeof(view_dyn), "", NOOBJECT } };
#endif

void gen_attr(CIp_t CI, tattrib attr_num)
{ if (sizeof(tattrib)==1)
    gen(CI, attr_num)
  else
    gen2(CI, attr_num);
}

typeobj * selector(CIp_t CI, sel_type seltype, int & aflag)
{ object * obj; typeobj * tobj;
  int tp;  uns8 obj_level;  kont_descr loc_kont;
  typeobj * eltpo;
  tobjname name1, name2, name3;  d_attr datr;
 /* main flags: */
  aflag=0;
  BOOL proj_var = FALSE, cached_atr=FALSE, dbtype=FALSE;  uns8 mult=1;
  BOOL access_generated;  int attr_num;  /* valid iff dbtype==TRUE */

  if (CI->cursym!=S_IDENT)
  { if ((CI->cursym==S_STRING) && (seltype==SEL_REFPAR))
      return faktor(CI, NULL); /* string as a ref. parameter */
#if WBVERS<900
    else if (CI->cursym==S_THISFORM)
    { if (!CI->thisform) c_error(THISFORM_UNDEFINED);
      name2[0]=name3[0]=0;
      if (next_sym(CI)=='.')
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        strcpy(name2, CI->curr_name);
        if (next_sym(CI)=='.')
        { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
          strcpy(name3, CI->curr_name);  next_sym(CI); 
        }
      }
      else if (seltype==SEL_REFPAR)
      { gen(CI,I_GET_THISFORM);
        return (typeobj*)&untyped_form_type;
      }
      else c_error(DOT_EXPECTED);
      tobj=call_view_method(CI, seltype, CI->thisform, "\001", name2, name3);  // "\001" is the reference to the current view
      if (seltype==SEL_ADDRESS && tobj) { if (tobj!=&att_null) gen(CI, I_DROP);  tobj=NULL; }
      return tobj;
    }
#endif
    else if (CI->cursym=='#')
    { if (CI->comp_flags & COMP_FLAG_VIEW && CI->err_subobj==ERR_INTEGRITY && CI->univ_ptr!=NULL)
      { next_sym(CI);
        gen(CI,I_RESTORE);
        tp=((t_integrity*)CI->univ_ptr)->att_type;
      }
      else c_error(IDENT_EXPECTED);
    }
    else if ((CI->cursym==S_CURRVIEW) &&
             ((seltype==SEL_REFPAR) || (seltype==SEL_FAKTOR)))
    { next_sym(CI);
      gen(CI,I_CURRVIEW);
      return simple_types[ATT_WINID];
    }
    else c_error(IDENT_EXPECTED);
  }
  else  /* identifier */
  { strcpy(name1, CI->curr_name);
    name2[0]=name3[0]=0;
    if (next_sym(CI)=='.')
      if (next_sym(CI)==S_IDENT)
      { strcpy(name2, CI->curr_name);
        if (next_sym(CI)=='.')
          if (next_sym(CI)==S_IDENT)
            { strcpy(name3, CI->curr_name);  next_sym(CI); }
      }

    if (CI->err_subobj==ERR_INTEGRITY && CI->univ_ptr!=NULL && CI->comp_flags & COMP_FLAG_VIEW &&
        !strcmp(name1, ((t_integrity*)CI->univ_ptr)->name))
    { gen(CI,I_RESTORE);
      tp=((t_integrity*)CI->univ_ptr)->att_type;
      goto end_part;
    }
   /* kontext-dependent identifiers searched first */
    if (search_dbobj(CI, name1, name2, name3, CI->kntx, &datr, &obj_level, &tobj))  /* found in the database kontext */
    { attr_num=datr.get_num();
      CI->last_column_number = attr_num;
      if (CI->comp_flags & COMP_FLAG_VIEW && attr_num) cached_atr=TRUE;
      else
      dbtype=TRUE;  /* the DELETED attribute is not in the cache */
      access_generated=FALSE;
     attribute_access:
      tp=datr.type;  mult=datr.multi;  CI->glob_db_specif=datr.specif;
      if (cached_atr && !access_generated)
      { gen(CI,I_CURKONT);     gen(CI,obj_level);  /* database object from the kontext */
        gen(CI,I_ADD_ATRNUM);  gen_attr(CI, attr_num);
        access_generated=TRUE;
      }
     /* checking the attr. suffix; tp, mult, CI->glob_db_specif dbtype defined */
      while ((CI->cursym=='[') || (CI->cursym=='^'))
      { if (!access_generated && (mult || !is_string(tp)))
        /* complex access unless compiling [char count] in index */
        { gen(CI,I_CURKONT);     gen(CI,obj_level);   /* database object from the kontext */
          gen(CI,I_ADD_ATRNUM);  gen_attr(CI, attr_num);
          access_generated=TRUE;
        }
        if (CI->cursym=='[')   /* for cached_atr must generate index or 0xffff */
        { t_specif orig_specif = CI->glob_db_specif;
          if (next_sym(CI)==']')
          { if (mult<=1) c_error(NOT_AN_ARRAY);
            if (!(CI->comp_flags & COMP_FLAG_VIEW)) c_error(INTEGER_EXPECTED);
            empty_index_inside=wbtrue;
            gen(CI,I_SYS_VAR);  gen(CI,1);
            /*if (!cached_atr) */gen(CI,I_SAVE_DB_IND);
            mult=1;
          }
          else  /* index non-empty */
          { if (mult>1)
            { int_expression(CI);
              /*if (!cached_atr) */gen(CI,I_SAVE_DB_IND);   /* multiattribute index */
              mult=1;
              CI->glob_db_specif = orig_specif;  // restores the length of the string etc.
            }
            else if (IS_HEAP_TYPE(tp))  /* interval index */
            { int_expression(CI);
              test_and_next(CI,',', COMMA_EXPECTED);
              int_expression(CI);
              tp=ATT_INTERVAL;
              CI->glob_db_specif = orig_specif;  // restores the length of the string etc.
              //if (cached_atr) geniconst(CI, 0xffff);   /* no index */
            }
            else if (is_string(tp) && (seltype==SEL_FAKTOR))
              if (CI->comp_flags & COMP_FLAG_TABLE) // compiling the table definition
              { if (CI->cursym!=S_INTEGER) c_error(INTEGER_EXPECTED);
                if (CI->glob_db_specif.length > CI->curr_int)
                CI->glob_db_specif.length = (uns16)CI->curr_int;
                next_sym(CI);
              }
              else   /* database string element access: */
              { int_expression(CI);
                gen(CI,I_SAVE);
                if (cached_atr)
                  { gen(CI, I_LOAD_CACHE_TEST);  cached_atr=FALSE; }
                else
                  { gen(CI, I_DBREADS);          dbtype=FALSE; }
                gen(CI,I_RESTORE);
                geniconst(CI, 1);  gen(CI,I_INTMINUS);  gen(CI,I_INTPLUS);
                tp=ATT_CHAR;  /* the char will be loaded below */
                CI->glob_db_specif = orig_specif;  // restores the length of the string etc.
                CI->glob_db_specif.length=1;
              }
            else if (tp==ATT_BINARY && seltype==SEL_FAKTOR)
            { int_expression(CI);
              gen(CI,I_SAVE);
              if (cached_atr)
                { gen(CI, I_LOAD_CACHE_TEST);  cached_atr=FALSE; }
              else
                { gen(CI, I_DBREADS);          dbtype=FALSE; }
              gen(CI,I_RESTORE);
              geniconst(CI, 1);  gen(CI,I_INTMINUS);  gen(CI,I_INTPLUS);
              tp=ATT_CHAR; //cannot use INT32 - 4 bytes will be loaded  /* the integer will be loaded below */
              CI->glob_db_specif.set_null();
              CI->glob_db_specif.length=1;
            }
            else c_error(NOT_AN_ARRAY);
            if (CI->cursym!=']') c_error(RIGHT_BRACE_EXPECTED);
          }
        }
        else /* ^ */
        { next_sym(CI);
          if ((mult>1)||(tp!=ATT_PTR)&&(tp!=ATT_BIPTR)) c_error(NOT_A_PTR);
          if (CI->cursym=='.') next_sym(CI);
          if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
          if (!find_attr(CI->cdp, CI->glob_db_specif.destination_table, CATEG_TABLE, CI->curr_name,
                         NULL, NULL, &datr))
            { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(IDENT_NOT_DEFINED); }
         /* put destination table & record number on TOSWB */
          if (cached_atr)
          {/* read pointer value from cache: */
//            gen(CI, I_DBREAD);           -- reads ptr from database
            gen(CI, I_LOAD_CACHE_TEST);  gen(CI, I_LOAD4);
           /* add destination table number: */
            geniconst(CI, CI->glob_db_specif.destination_table);
            gen(CI, I_SWAP);  gen(CI, I_ADD_CURSNUM);
            cached_atr=FALSE;  dbtype=TRUE;
          } else gen(CI,I_MAKE_PTR_STEP);
          gen(CI,I_ADD_ATRNUM);  gen_attr(CI,datr.get_num());
          tp=datr.type;  mult=datr.multi;  CI->glob_db_specif=datr.specif;
        }
        next_sym(CI);
      }
      if (CI->cursym=='#')
      { if ((mult<=1) && !IS_HEAP_TYPE(tp)) c_error(NO_LENGTH);
        next_sym(CI);
        if (!access_generated)
        { gen(CI,I_CURKONT);     gen(CI,obj_level);  /* database object from the kontext */
          gen(CI,I_ADD_ATRNUM);  gen_attr(CI, attr_num);
          access_generated=TRUE;
        }
        tp = (mult>1) ? ATT_INDNUM : ATT_VARLEN;  mult=0;  CI->glob_db_specif.scale=0;
      }

     /* special handling of some database types */
      if (tp==ATT_AUTOR)
        if (seltype!=SEL_ADDRESS) /* must not be converted for table data transfer */
          tp=ATT_STRING;
    }

    else  /********** searching for program defined identifiers: *************/
    { obj=search_obj(CI, name1, &obj_level);
      if (obj==NULL)  /* not def. in pgm nor in kontext */
      { tobjnum objnum;
        if (*name2 && !cd_Find_object(CI->cdp, name1, CATEG_VIEW, &objnum))
        { tobj=call_view_method(CI, seltype, objnum, name1, name2, name3);
          if (seltype==SEL_ADDRESS && tobj) { if (tobj!=&att_null) gen(CI, I_DROP);  tobj=NULL; }
         // info about opened view:
          if ((CI->comp_flags & COMP_FLAG_VIEW) && CI->univ_ptr && !*name3 && !strcmp(name2, "OPEN"))
            strmaxcpy((tptr)CI->univ_ptr, name1, sizeof(tobjname));
          return tobj;
        }
        SET_ERR_MSG(CI->cdp, name1);  c_error(IDENT_NOT_DEFINED); 
      }
      switch (obj->any.categ)
      { case O_VAR:  case O_VALPAR:  case O_REFPAR:
          if (obj->var.offset>0xffff)
            { gen(CI,I_LOADADR4); gen(CI,obj_level);  gen4(CI,obj->var.offset); }
          else
            { gen(CI,I_LOADADR);  gen(CI,obj_level);  gen2(CI,obj->var.offset); }
          if (obj->var.categ==O_REFPAR) gen(CI,I_DEREF);
          tobj=obj->var.type;
          CI->glob_db_specif=T2specif(tobj);
          if (tobj->type.anyt.typecateg==T_RECCUR)
          { loc_kont.kontcateg=CATEG_RECCUR;    /* variable cursor */
            loc_kont.record_tobj=tobj;
            goto tabcurs0;
          }
          break;
        default:
          SET_ERR_MSG(CI->cdp, name1);  c_error(BAD_IDENT_CLASS);
        case O_SPROC:
          geniconst(CI, obj->proc.sprocnum);
          tobj=call_gen(CI,obj);  
          if (obj->proc.sprocnum!=NEW_NUM) gen(CI,I_SCALL);  // already called for NEW
          if (seltype==SEL_REFPAR)  /* returned string as ref. par allowed */
          { if (!STR_ARR(tobj))
              { SET_ERR_MSG(CI->cdp, name1);  c_error(BAD_IDENT_CLASS); }
          }
          else if (seltype==SEL_FAKTOR)
          { if (!tobj || tobj==&att_null)
              { SET_ERR_MSG(CI->cdp, name1);  c_error(BAD_IDENT_CLASS); }
          }
          else { if (tobj && tobj!=&att_null) gen(CI,I_DROP);  tobj=NULL; }
          if (tobj) CI->glob_db_specif=T2specif(tobj);
          return tobj;
        case O_FUNCT:
          if (CI->cursym==S_ASSIGN)
          { tname retval_name;
            strmaxcpy(retval_name, name1, sizeof(tname));
            if (strlen(retval_name) < NAMELEN) strcat(retval_name, "#");
            else retval_name[NAMELEN-1]='#';
            if (!(obj=search_obj(CI, retval_name, &obj_level)))
              c_error(LEFT_PARENT_EXPECTED);
            if (obj->var.offset>0xffff)
              { gen(CI,I_LOADADR4); gen(CI,obj_level);  gen4(CI,obj->var.offset); }
            else
              { gen(CI,I_LOADADR);  gen(CI,obj_level);  gen2(CI,obj->var.offset); }
            return obj->var.type;
          }
          gen(CI,I_SUBRADR); gen4(CI,obj->subr.code);  gen(CI,obj_level);
          tobj=call_gen(CI,obj);  gen(CI,I_CALL);
          CI->glob_db_specif=T2specif(tobj);
          if (seltype==SEL_ADDRESS) { gen(CI,I_DROP);  return NULL; }
          if (seltype==SEL_REFPAR)  /* only string function can be passed as ref. param */
            if (!STR_ARR(tobj))
              { SET_ERR_MSG(CI->cdp, name1);  c_error(BAD_IDENT_CLASS); }
          return tobj;
        case O_PROC:
          gen(CI,I_SUBRADR); gen4(CI,obj->subr.code);  gen(CI,obj_level);
          call_gen(CI,obj);  gen(CI,I_CALL);
          if (seltype!=SEL_ADDRESS)
            { SET_ERR_MSG(CI->cdp, name1);  c_error(BAD_IDENT_CLASS); }
          return NULL;
        case O_CONST:
          if (obj->cons.type==&att_nil)
            { geniconst(CI, 0);  return obj->cons.type; }
          if (seltype!=SEL_FAKTOR)   /* constant not allowed here */
            { SET_ERR_MSG(CI->cdp, name1);  c_error(BAD_IDENT_CLASS); }
          if (DIRECT(obj->cons.type))
            switch (DIRTYPE(obj->cons.type))
            { case ATT_INT32:        case ATT_BIPTR:
              case ATT_DATE:         case ATT_TIME:    case ATT_TIMESTAMP:
              case ATT_PTR:          case ATT_CURSOR:
                gen(CI,I_INT32CONST); gen4(CI,obj->cons.value.int32); break;
              case ATT_INT16:        
                if ((uns16)obj->cons.value.int16==0x8000)
                  { gen(CI,I_INT32CONST);  gen4(CI,0x8000); }
                else
                  { gen(CI,I_INT16CONST);  gen2(CI,obj->cons.value.int16); }
                break;
              case ATT_INT8:
                if ((uns8)obj->cons.value.int8==0x80)
                  { gen(CI,I_INT32CONST);  gen4(CI,0x80); }
                else
                  { gen(CI,I_INT8CONST);  gen(CI,obj->cons.value.int8); }
                break;
              case ATT_INT64:
                gen(CI,I_INT64CONST); gen8i(CI,&obj->cons.value.int64); break;
              case ATT_BOOLEAN:      case ATT_CHAR:
                gen(CI,I_INT8CONST);  gen  (CI, obj->cons.value.int8);  break;
              case ATT_FLOAT:
                gen(CI,I_REALCONST);  gen8 (CI, obj->cons.value.real);  break;
              case ATT_MONEY:
                gen(CI,I_MONEYCONST);
                code_out(CI, (tptr)obj->cons.value.money, CI->code_offset, MONEY_SIZE);
                CI->code_offset+=MONEY_SIZE;                     break;

            }
          else  /* composed type -> language structured constant */
            { gen(CI,I_INT32CONST);   gen4(CI, *(uns32*)&(obj->cons.value.ptr)); }
          CI->glob_db_specif=T2specif(obj->cons.type);
          return obj->cons.type;
        case O_AGGREG:
          if (seltype!=SEL_FAKTOR)
            { SET_ERR_MSG(CI->cdp, name1);  c_error(BAD_IDENT_CLASS); }
          return gen_aggreg(CI, obj->aggr.ag_type);
        case O_CURSOR:
          loc_kont.kontcateg=CATEG_CURSOR;  /* fixed cursor */
          loc_kont.kontobj=obj->curs.defnum;
          gen(CI,I_LOAD_OBJECT);  gen(CI, CATEG_CURSOR);
          gen4(CI,obj->curs.offset);
          goto tabcurs0;
        case O_TABLE:
          loc_kont.kontcateg=CATEG_TABLE;
          loc_kont.kontobj=obj->curs.defnum;
          gen(CI,I_LOAD_OBJECT);  gen(CI, CATEG_TABLE);
          gen4(CI,obj->curs.offset);
         tabcurs0:  /* local kontext made by table or cursor(f,v) prefix */
          loc_kont.next_kont=NULL;
         /* table number or cursor address on the TOS, loc_kont prepared */
          if (CI->cursym!='[')
          { if (loc_kont.kontcateg==CATEG_TABLE) return &att_table;
            if (seltype==SEL_FAKTOR)   // loading cursor number from static or variable cursor 
              { gen(CI,I_LOADOBJNUM);  return &att_cursor; }
            return loc_kont.kontcateg==CATEG_RECCUR ? &att_cursor : &att_statcurs;
          }
          if (loc_kont.kontcateg!=CATEG_TABLE) gen(CI,I_LOADOBJNUM);
          next_sym(CI);
          int_expression(CI);  gen(CI,I_ADD_CURSNUM);
          test_and_next(CI,']', RIGHT_BRACE_EXPECTED);
          if (CI->cursym=='.') next_sym(CI);
          if (CI->cursym!=S_IDENT)
          { kont_from_selector=loc_kont; /* copy to a static temporary variable */
            return &att_dbrec; /* described in kont_from_selector */
          }
          if ((loc_kont.kontcateg==CATEG_RECCUR) &&
              (((typeobj*)loc_kont.record_tobj)->type.record.items==NULL))
          {/* untyped database access */
            int i;
            gen(CI, I_UNTATTR);
            for (i=0;  i<ATTRNAMELEN;  i++) gen(CI, CI->curr_name[i]);
            gen(CI, 0);
            next_sym(CI);
            while (CI->cursym=='[')
            { next_sym(CI);
              int_expression(CI);
              if (CI->cursym==',')
              { next_sym(CI);
                int_expression(CI);
                test_and_next(CI, ']', RIGHT_BRACE_EXPECTED);
                aflag|=DB_OBJ;
                return &att_interval;
              }
              else
              { gen(CI,I_SAVE_DB_IND);   /* multiattribute index */
                test_and_next(CI, ']', RIGHT_BRACE_EXPECTED);
              }
            }
            if (CI->cursym=='#')
            { next_sym(CI);
              if (seltype==SEL_FAKTOR)
              { gen(CI, I_DBREAD_LEN);
                return &att_int32;
              }
              else 
              { aflag|=DB_OBJ;
                return &att_varlen;
              }
            }
            else
            { aflag|=DB_OBJ;
              return &att_untyped;
            }
          }
          else
          { if (!search_dbobj(CI, CI->curr_name, NULL, NULL, &loc_kont, &datr, &obj_level, NULL))
              { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(UNDEF_ATTRIBUTE); }
            next_sym(CI);
            CI->last_column_number = attr_num = datr.get_num();
            gen(CI,I_ADD_ATRNUM);  gen_attr(CI,attr_num);
            dbtype=TRUE;  access_generated=TRUE;
            goto attribute_access;
          }
      } /* end of ident category switch */

     /* variable access, address generated on TOS, type in tobj */
      if (*name2) 
      { BOOL proc_called;
        tobj=record_elem(CI, tobj, name2, name3, seltype, &proc_called);
        if (seltype==SEL_ADDRESS && tobj && proc_called) { if (tobj!=&att_null) gen(CI, I_DROP);  tobj=NULL; }
        if (proc_called) return tobj;
      }

      while ((CI->cursym=='.')||(CI->cursym=='[')||(CI->cursym=='^'))
      { if (DIRECT(tobj)) c_error(NOT_STRUCTURED_TYPE);
        if (CI->cursym=='.')
        { next_and_test(CI, S_IDENT, ITEM_IDENT_EXPECTED);
          strcpy(name2, CI->curr_name);  *name3=0;
          next_sym(CI);
          BOOL proc_called;
          tobj=record_elem(CI, tobj, name2, name3, seltype, &proc_called);
          if (tobj) CI->glob_db_specif=T2specif(tobj);
          if (proc_called) 
          { if (seltype==SEL_ADDRESS && tobj) { if (tobj!=&att_null) gen(CI, I_DROP);  tobj=NULL; }
            return tobj;
          }
        }
        else if (CI->cursym=='^')
        { if (tobj->type.anyt.typecateg!=T_PTR && tobj->type.anyt.typecateg!=T_PTRFWD)
            c_error(NOT_A_PTR);
          gen(CI,I_DEREF);
          tobj=tobj->type.ptr.domaintype;
          CI->glob_db_specif=T2specif(tobj);
          next_sym(CI);
        }
        else  /* CI->cursym == '[' */
        { unsigned elsz;
          if (next_sym(CI)==']')
          { if (DIRECT(tobj) || tobj->type.anyt.typecateg!=T_ARRAY) c_error(NOT_AN_ARRAY);
            if (!(CI->comp_flags & COMP_FLAG_VIEW)) c_error(INTEGER_EXPECTED);
            empty_index_inside=wbtrue;
            gen(CI,I_SYS_VAR);  gen(CI,1);
            gen(CI, I_CHECK_BOUND);  gen4(CI, tobj->type.array.elemcount);
            eltpo=tobj->type.array.elemtype;
            elsz=eltpo->type.anyt.valsize;
            if (elsz!=1) { geniconst(CI, elsz);  gen(CI,I_INTTIMES); }
            tobj=tobj->type.array.elemtype;
            CI->glob_db_specif=T2specif(tobj);
            gen(CI,I_INTPLUS);  /* adding element offset to the array beginning */
          }
          else
          { int_expression(CI);
            if (tobj->type.anyt.typecateg==T_ARRAY)
            { if (tobj->type.array.lowerbound!=0)
              { geniconst(CI, tobj->type.array.lowerbound);
                gen(CI,I_INTMINUS);
              }
              eltpo=tobj->type.array.elemtype;
              elsz=eltpo->type.anyt.valsize;
              if (elsz!=1) { geniconst(CI, elsz);  gen(CI,I_INTTIMES); }
              tobj=tobj->type.array.elemtype;
            }
            else if (tobj->type.anyt.typecateg==T_STR)
            { geniconst(CI, 1);  gen(CI,I_INTMINUS);
              tobj=&att_char;
            }
            else if (tobj->type.anyt.typecateg==T_BINARY)
            { geniconst(CI, 1);  gen(CI,I_INTMINUS);
              tobj=&att_char; //cannot use INT32 - 4 bytes will be loaded  /* the integer will be loaded below */
            }
            else c_error(NOT_AN_ARRAY);
            CI->glob_db_specif=T2specif(tobj);
            gen(CI,I_INTPLUS);  /* adding element offset to the array beginning */
            if (CI->cursym!=']') c_error(RIGHT_BRACE_EXPECTED);
          }
          next_sym(CI);
        }
      }
      if (!DIRECT(tobj))   /* composed variable */
      { if (seltype==SEL_FAKTOR)
          if (tobj->type.anyt.typecateg==T_PTR || tobj->type.anyt.typecateg==T_PTRFWD)
            gen(CI,I_LOAD4);
        return tobj;
      }
      tp=ATT_TYPE(tobj);  proj_var=TRUE;
    } /* found among declared project variables */
  } /* identifier was on input */

 end_part:
 /* tp, mult, dbtype, proj_var, cached_atr defined, cannot be COMPOSED */
 /* loading the value for SEL_FAKTOR if possible: */
  if (seltype==SEL_FAKTOR && mult<=1 && !IS_HEAP_TYPE(tp) && tp!=ATT_INTERVAL)
  { if (dbtype)
    { if (access_generated) /* database access is on TOS */
      { if (tp==ATT_INDNUM)
          { gen(CI,I_DBREAD_NUM);  tp=ATT_INT16; }
        else if (tp==ATT_VARLEN)
          { gen(CI,I_DBREAD_LEN);  tp=ATT_INT32; }
        else if (SPECIFLEN(tp))
          GEN(CI,I_DBREADS)  /* reading and pushing the string pointer */
        else
          gen(CI,I_DBREAD);  /* reading data to TOS */
      }
      else  /* read from obj_level, attr_num */
      { gen(CI, (uns8)(speciflen(tp) ? I_FAST_STRING_DBREAD:I_FAST_DBREAD));
        gen(CI, obj_level);  
        gen_attr(CI, attr_num);
      }
      dbtype=FALSE;
    }
    else /* read the value of the variable - its address is on TOS */
    { if (cached_atr && tp!=ATT_INDNUM)   /* then access_generated */
        gen(CI, I_LOAD_CACHE_TEST);
      if      (tp==ATT_INDNUM)
        { gen(CI, I_GET_CACHED_MCNT);  tp=ATT_INT16; }
      else if (tp==ATT_VARLEN)
        { gen(CI, I_LOAD4);  tp=ATT_INT32; }   /* cached attr. */
      else if (!speciflen(tp) && !IS_ODBC_TYPE(tp))
      switch (tpsize[tp])  /* for strings and ODBC types address remains on TOS */
      { case 1: gen(CI,I_LOAD1); break;
        case 2: gen(CI,I_LOAD2); break;
        case 4: gen(CI,I_LOAD4); break;
        case 6: gen(CI,I_LOAD6); break;
        case 8: gen(CI,I_LOAD8); break;
      }
      proj_var=cached_atr=FALSE;
    }
  }
  else if (dbtype && !access_generated)
  { gen(CI,I_CURKONT);     gen(CI,obj_level);  /* database object from the kontext */
    gen(CI,I_ADD_ATRNUM);  gen_attr(CI, attr_num);
  }

  if (mult>1)     aflag |= MULT_OBJ;
  if (dbtype)     aflag |= DB_OBJ; 
  if (proj_var)   aflag |= PROJ_VAR;
  if (cached_atr) aflag |= CACHED_ATR;
  return simple_types[tp];
}

void read_statement(CIp_t CI);

typeobj * faktor(CIp_t CI, int * eflag)
{ typeobj * t1;
 if (eflag) *eflag=0;
 switch (CI->cursym)
 {case '(':
    next_sym(CI);
    t1=expression(CI, eflag);
    test_and_next(CI,')',RIGHT_PARENT_EXPECTED);
    return t1;
  case S_NOT:
    next_sym(CI);
    t1=faktor(CI, eflag);
    unary_op(CI, t1, eflag ? *eflag : 0, S_NOT);
    if (eflag) *eflag=0;
    return &att_boolean;
  case '@':
    gen(CI,I_CURKONT); gen(CI,0);     /* cursor position on level 0 */
    next_sym(CI);
    return &att_int32;
    /* uses the fact that I_CURKONT puts position into the lower part of stack cell */
  case S_DBL_AT:
    gen(CI,I_SYS_VAR); gen(CI,0);     /* nondel view position on level 0 */
    next_sym(CI);
    return &att_int32;
  case S_PAGENUM:
    gen(CI,I_SYS_VAR); gen(CI,2);     /* the current page number */
    next_sym(CI);
    return &att_int32;
  case S_INTEGER:
    if (CI->curr_int > (sig64)0x7fffffff || CI->curr_int < -(sig64)0x7fffffff)
    { gen(CI,I_INT64CONST);  gen8i(CI, &CI->curr_int); 
      next_sym(CI);
      return &att_int64;
    }
    else 
    { geniconst(CI, (sig32)CI->curr_int);
      next_sym(CI);
      return &att_int32;
    }
  case S_MONEY:
    gen(CI,I_MONEYCONST);
    code_out(CI, (tptr)&CI->curr_money, CI->code_offset, MONEY_SIZE);
    CI->code_offset+=MONEY_SIZE;
    next_sym(CI);
    return &att_money;
  case S_DATE:
    geniconst(CI, (uns32)CI->curr_int);
    next_sym(CI);
    return &att_date;
  case S_TIME:
    geniconst(CI, (uns32)CI->curr_int);
    next_sym(CI);
    return &att_time;
  case S_TIMESTAMP:
    geniconst(CI, (uns32)CI->curr_int);
    next_sym(CI);
    return &att_timestamp;
  case S_REAL:
    gen(CI,I_REALCONST);
    gen8(CI, CI->curr_real);
    next_sym(CI);
    return &att_float;
  case S_CHAR:
    gen(CI,I_INT8CONST);
    gen(CI,CI->curr_string()[0]);
    next_sym(CI);
    return &att_char;
  case S_STRING:
  { int len = (int)strlen(CI->curr_string());
    gen(CI,I_STRINGCONST);  gen2(CI,(uns16)len);
    genstr(CI, CI->curr_string());
    next_sym(CI);
    CI->glob_db_specif.length=len;
    return &att_string;
  }
  case S_BINLIT:
  { gen(CI, I_BINCONST);
    for (int i=0;  i<=CI->curr_string()[0];  i++) gen(CI, CI->curr_string()[i]);
    CI->glob_db_specif.length=CI->curr_string()[0];
    next_sym(CI);
    return &att_binary;
  }
  case '#':
  case S_CURRVIEW:
  case S_THISFORM:
  { int aflag;
    return selector(CI, SEL_FAKTOR, eflag ? *eflag : aflag);
  }
  case S_IDENT:
    if (!strcmp(CI->curr_name, STR_READ))
    { read_statement(CI);
      return &att_boolean;
    }
    if (!strcmp(CI->curr_name, STR_REPL_DESTIN))
    { next_sym(CI);
      gen(CI,I_SYS_VAR); gen(CI,3);
      CI->glob_db_specif.length=UUID_SIZE;
      return &att_binary;
    }
    else 
    { int aflag;
      return selector(CI, SEL_FAKTOR, eflag ? *eflag : aflag);
    }
  default: c_error(FAKTOR_EXPECTED);
 }
 return NULL;  /* never used */
}

typeobj * term(CIp_t CI, int * eflag)
{ typeobj * t1, * t2;  uns32 lastadr=0;  int eflag2;
  symbol op;

  t1=faktor(CI, eflag);
  while ((CI->cursym=='*')||(CI->cursym=='/')||(CI->cursym==S_AND)||
         (CI->cursym==S_DIV)||(CI->cursym==S_MOD))
  { op=CI->cursym; next_sym(CI);
    if ((op==S_AND) && (t1==&att_boolean))
    { if (lastadr) setcadr(CI, lastadr);
      lastadr=gen_forward_jump(CI, I_JMPFDR);
      if (faktor(CI, NULL) != &att_boolean) c_error(MUST_BE_BOOL);
    }
    else
    { t2=faktor(CI, &eflag2);
      t1=binary_op(CI, t1, eflag ? *eflag : 0, t2, eflag2, op);
    }
    if (eflag) *eflag=0;
  }
  if (lastadr) setcadr(CI, lastadr);
  return t1;
}

typeobj * simple_expr(CIp_t CI, int * eflag)
{ typeobj * t1, * t2;  uns32 lastadr=0;  int eflag2;
  BOOL minus; symbol op;
  minus=test_signum(CI);
  t1=term(CI, eflag);
  if (minus) unary_op(CI, t1, eflag ? *eflag : 0, '-');
  while ((CI->cursym=='+')||(CI->cursym=='-')||(CI->cursym==S_OR))
  { op=CI->cursym; next_sym(CI);
    if ((op==S_OR) && (t1==&att_boolean))
    { if (lastadr) setcadr(CI, lastadr);
      lastadr=gen_forward_jump(CI,I_JMPTDR);
      if (term(CI, NULL) != &att_boolean) c_error(MUST_BE_BOOL);
    }
    else
    { if (IS_STRING_OBJ(t1) || CHAR_ARR(t1)) gen(CI,I_MOVESTRING);   /* string + */
      CI->prev_db_specif=CI->glob_db_specif;  // used by concatenation
      t2=term(CI, &eflag2);
      t1=binary_op(CI, t1, eflag ? *eflag : 0, t2, eflag2, op);
    }
    if (eflag) *eflag=0;
  }
  if (lastadr) setcadr(CI, lastadr);
  return t1;
}

typeobj * incond_expression(CIp_t CI, int * eflag)
{ typeobj * t1, *t2;  symbol op;  uns32 jmp_point=0;  int eflag1;
  t1=simple_expr(CI, eflag);
  if (relation(CI->cursym))
  { op=CI->cursym;
    CI->prev_db_specif=CI->glob_db_specif;  // used by binary relations
    next_sym(CI);
    if (IS_STRING_OBJ(t1) || CHAR_ARR(t1) || t1==&att_binary)
      if (op!=S_DDOT) gen(CI,I_MOVESTRING);
    t2=simple_expr(CI, &eflag1);
    while (CI->cursym==S_NEXTEQU)
    { gen(CI,I_DUP);  gen(CI,I_SAVE);
      binary_op(CI,t1,0,t2,0,op);
      if (jmp_point) setcadr(CI, jmp_point);
      gen(CI,I_DUP);
      jmp_point=gen_forward_jump(CI, I_FALSEJUMP);
      gen(CI,I_RESTORE);
      next_sym(CI);
      t2=simple_expr(CI, NULL);
    }
    t1=binary_op(CI, t1, eflag ? *eflag : 0, t2, eflag1, op);
    if (eflag) *eflag &= ~(int)DB_OBJ;
    if (jmp_point) setcadr(CI, jmp_point);
  }
  return t1;
}

CFNC DllKernel typeobj * WINAPI expression(CIp_t CI, int * eflag)
{ typeobj * t1, * t2;  uns32 adr1_point, adr2_point;
  t1=incond_expression(CI, eflag);
  if (CI->cursym=='?')
  { if (t1!=&att_boolean) c_error(MUST_BE_BOOL);
    next_sym(CI);
    adr1_point=gen_forward_jump(CI,I_FALSEJUMP);
    t1=incond_expression(CI, eflag);
    t_specif spec1 = CI->glob_db_specif;
    adr2_point=gen_forward_jump(CI,I_JUMP);
    setcadr(CI, adr1_point);
    test_and_next(CI,':', COLON_EXPECTED);
    t2=expression(CI);
    setcadr(CI, adr2_point);
    if (t1==&att_int16) t1=&att_int32; // tady by asi sprvne mela byt konverze
    if (t2==&att_int16) t2=&att_int32;
    if (t1!=t2) if (!STR_ARR(t1) || !STR_ARR(t2))
      c_error(INCOMPATIBLE_TYPES);
    if (STR_ARR(t1))
    { if (spec1.wide_char != CI->glob_db_specif.wide_char) c_error(INCOMPATIBLE_TYPES);
      if (spec1.length > CI->glob_db_specif.length) CI->glob_db_specif.length=spec1.length;
    }
  }
  return t1;
}

CFNC DllKernel void WINAPI bool_expr(CIp_t CI)
{ int eflag;
  if (expression(CI, &eflag) != &att_boolean) c_error(MUST_BE_BOOL); 
}

CFNC DllKernel void WINAPI bool_expr_end(CIp_t CI)
{ bool_expr(CI);
  c_end_check(CI);
}

void read_statement(CIp_t CI)
/* Stack: adr. promenne, velikost, soubor. Kod: I_READ, typ.
   Po provedeni ocekava na stacku bud FALSE, FALSE (chyba) nebo
   adresu, hodnotu a TRUE. */
{ typeobj * obj;  uns32 outadr_point;  int aflag;
  next_and_test(CI,'(',LEFT_PARENT_EXPECTED);
  next_sym(CI);
  obj=selector(CI, SEL_ADDRESS, aflag);
  if (ATT_TYPE(obj) != ATT_FILE) c_error(INCOMPATIBLE_TYPES);
  test_and_next(CI,',', COMMA_EXPECTED);
  gen(CI,I_LOAD1);  gen(CI,I_SAVE);
  obj=selector(CI, SEL_ADDRESS, aflag);
  if (!DIRECT(obj))
    { if (!CHAR_ARR(obj)) c_error(INCOMPATIBLE_TYPES); }
  else if ((aflag & (DB_OBJ | CACHED_ATR | MULT_OBJ)) || IS_HEAP_TYPE(DIRTYPE(obj)))
    c_error(INCOMPATIBLE_TYPES);
  if (CI->cursym==':')
  { next_sym(CI);
    int_expression(CI);
  }
  else geniconst(CI,10);
  GEN(CI,I_RESTORE)
  gen(CI,I_READ);  gen(CI,(uns8)(DIRECT(obj) ? DIRTYPE(obj) : ATT_STRING));
  outadr_point=gen_forward_jump(CI, I_FALSEJUMP);
  assignment(CI, obj, aflag, obj, 0, FALSE);
  geniconst(CI, 1);
  setcadr(CI, outadr_point);
  test_and_next(CI,')',RIGHT_PARENT_EXPECTED);
}

CFNC DllKernel void WINAPI c_end_check(CIp_t CI)
{ if (CI->cursym!=S_SOURCE_END) c_error(GARBAGE_AFTER_END); }

/*************************** compiler entry *********************************/
typedef char tpiece[6];

static bool compiler_is_init = false;

void compiler_init(void)  /* called only once */
{ if (compiler_is_init) return;
 // types:
  att_null.categ=O_TYPE;  att_null.type.simple.typecateg=T_SIMPLE;  att_null.type.simple.valsize=0, att_char.type.simple.typenum=0;
  att_boolean.categ=O_TYPE;  att_boolean.type.simple.typecateg=T_SIMPLE;  att_boolean.type.simple.valsize=1, att_boolean.type.simple.typenum=ATT_BOOLEAN;
  att_char.categ=O_TYPE;  att_char.type.simple.typecateg=T_SIMPLE;  att_char.type.simple.valsize=1, att_char.type.simple.typenum=ATT_CHAR;
  att_int16.categ=O_TYPE;  att_int16.type.simple.typecateg=T_SIMPLE;  att_int16.type.simple.valsize=2, att_int16.type.simple.typenum=ATT_INT16;
  att_int32.categ=O_TYPE;  att_int32.type.simple.typecateg=T_SIMPLE;  att_int32.type.simple.valsize=4, att_int32.type.simple.typenum=ATT_INT32;
  att_money.categ=O_TYPE;  att_money.type.simple.typecateg=T_SIMPLE;  att_money.type.simple.valsize=6, att_money.type.simple.typenum=ATT_MONEY;
  att_float.categ=O_TYPE;  att_float.type.simple.typecateg=T_SIMPLE;  att_float.type.simple.valsize=8, att_float.type.simple.typenum=ATT_FLOAT;
  att_string.categ=O_TYPE;  att_string.type.simple.typecateg=T_SIMPLE;  att_string.type.simple.valsize=0, att_string.type.simple.typenum=ATT_STRING;
  att_binary.categ=O_TYPE;  att_binary.type.simple.typecateg=T_SIMPLE;  att_binary.type.simple.valsize=0, att_binary.type.simple.typenum=ATT_BINARY;
  att_time.categ=O_TYPE;  att_time.type.simple.typecateg=T_SIMPLE;  att_time.type.simple.valsize=4, att_time.type.simple.typenum=ATT_TIME;
  att_date.categ=O_TYPE;  att_date.type.simple.typecateg=T_SIMPLE;  att_date.type.simple.valsize=4, att_date.type.simple.typenum=ATT_DATE;
  att_timestamp.categ=O_TYPE;  att_timestamp.type.simple.typecateg=T_SIMPLE;  att_timestamp.type.simple.valsize=4, att_timestamp.type.simple.typenum=ATT_TIMESTAMP;
  att_ptr.categ=O_TYPE;  att_ptr.type.simple.typecateg=T_SIMPLE;  att_ptr.type.simple.valsize=sizeof(trecnum), att_ptr.type.simple.typenum=ATT_PTR;
  att_biptr.categ=O_TYPE;  att_biptr.type.simple.typecateg=T_SIMPLE;  att_biptr.type.simple.valsize=sizeof(trecnum), att_biptr.type.simple.typenum=ATT_BIPTR;
  att_autor.categ=O_TYPE;  att_autor.type.simple.typecateg=T_SIMPLE;  att_autor.type.simple.valsize=UUID_SIZE, att_autor.type.simple.typenum=ATT_AUTOR;
  att_datim.categ=O_TYPE;  att_datim.type.simple.typecateg=T_SIMPLE;  att_datim.type.simple.valsize=4, att_datim.type.simple.typenum=ATT_DATIM;
  att_hist.categ=O_TYPE;  att_hist.type.simple.typecateg=T_SIMPLE;  att_hist.type.simple.valsize=4+sizeof(tpiece), att_hist.type.simple.typenum=ATT_HIST;
  att_raster.categ=O_TYPE;  att_raster.type.simple.typecateg=T_SIMPLE;  att_raster.type.simple.valsize=4+sizeof(tpiece), att_raster.type.simple.typenum=ATT_RASTER;
  att_text.categ=O_TYPE;  att_text.type.simple.typecateg=T_SIMPLE;  att_text.type.simple.valsize=4+sizeof(tpiece), att_text.type.simple.typenum=ATT_TEXT;
  att_nospec.categ=O_TYPE;  att_nospec.type.simple.typecateg=T_SIMPLE;  att_nospec.type.simple.valsize=4+sizeof(tpiece), att_nospec.type.simple.typenum=ATT_NOSPEC;
  att_signat.categ=O_TYPE;  att_signat.type.simple.typecateg=T_SIMPLE;  att_signat.type.simple.valsize=4+sizeof(tpiece), att_signat.type.simple.typenum=ATT_SIGNAT;
  att_untyped.categ=O_TYPE;  att_untyped.type.simple.typecateg=T_SIMPLE;  att_untyped.type.simple.valsize=9, att_untyped.type.simple.typenum=ATT_UNTYPED;
  att_marker.categ=O_TYPE;  att_marker.type.simple.typecateg=T_SIMPLE;  att_marker.type.simple.valsize=0, att_marker.type.simple.typenum=ATT_MARKER;
  att_indnum.categ=O_TYPE;  att_indnum.type.simple.typecateg=T_SIMPLE;  att_indnum.type.simple.valsize=0, att_indnum.type.simple.typenum=ATT_INDNUM;
  att_interval.categ=O_TYPE;  att_interval.type.simple.typecateg=T_SIMPLE;  att_interval.type.simple.valsize=0, att_interval.type.simple.typenum=ATT_INTERVAL;
  att_nil.categ=O_TYPE;  att_nil.type.simple.typecateg=T_SIMPLE;  att_nil.type.simple.valsize=0, att_nil.type.simple.typenum=ATT_NIL;
  att_file.categ=O_TYPE;  att_file.type.simple.typecateg=T_SIMPLE;  att_file.type.simple.valsize=1, att_file.type.simple.typenum=ATT_FILE;
  att_table.categ=O_TYPE;  att_table.type.simple.typecateg=T_SIMPLE;  att_table.type.simple.valsize=sizeof(ttablenum), att_table.type.simple.typenum=ATT_TABLE;
  att_cursor.categ=O_TYPE;  att_cursor.type.simple.typecateg=T_SIMPLE;  att_cursor.type.simple.valsize=sizeof(tobjnum), att_cursor.type.simple.typenum=ATT_CURSOR;
  att_dbrec.categ=O_TYPE;  att_dbrec.type.simple.typecateg=T_SIMPLE;  att_dbrec.type.simple.valsize=8, att_dbrec.type.simple.typenum=ATT_DBREC;
  att_varlen.categ=O_TYPE;  att_varlen.type.simple.typecateg=T_SIMPLE;  att_varlen.type.simple.valsize=0, att_varlen.type.simple.typenum=ATT_VARLEN;
  att_statcurs.categ=O_TYPE;  att_statcurs.type.simple.typecateg=T_SIMPLE;  att_statcurs.type.simple.valsize=3*sizeof(tcursnum), att_statcurs.type.simple.typenum=ATT_STATCURS;
  att_odbc_numeric.categ=O_TYPE;  att_odbc_numeric.type.simple.typecateg=T_SIMPLE;  att_odbc_numeric.type.simple.valsize=0, att_odbc_numeric.type.simple.typenum=ATT_ODBC_NUMERIC;
  att_odbc_decimal.categ=O_TYPE;  att_odbc_decimal.type.simple.typecateg=T_SIMPLE;  att_odbc_decimal.type.simple.valsize=0, att_odbc_decimal.type.simple.typenum=ATT_ODBC_DECIMAL;
  att_odbc_time.categ=O_TYPE;  att_odbc_time.type.simple.typecateg=T_SIMPLE;  att_odbc_time.type.simple.valsize=0, att_odbc_time.type.simple.typenum=ATT_ODBC_TIME;
  att_odbc_date.categ=O_TYPE;  att_odbc_date.type.simple.typecateg=T_SIMPLE;  att_odbc_date.type.simple.valsize=0, att_odbc_date.type.simple.typenum=ATT_ODBC_DATE;
  att_odbc_timestamp.categ=O_TYPE;  att_odbc_timestamp.type.simple.typecateg=T_SIMPLE;  att_odbc_timestamp.type.simple.valsize=0, att_odbc_timestamp.type.simple.typenum=ATT_ODBC_TIMESTAMP;
  att_int8.categ=O_TYPE;  att_int8.type.simple.typecateg=T_SIMPLE;  att_int8.type.simple.valsize=1, att_int8.type.simple.typenum=ATT_INT8;
  att_int64.categ=O_TYPE;  att_int64.type.simple.typecateg=T_SIMPLE;  att_int64.type.simple.valsize=8, att_int64.type.simple.typenum=ATT_INT64;
  att_domain.categ=O_TYPE;  att_domain.type.simple.typecateg=T_SIMPLE;  att_domain.type.simple.valsize=0, att_domain.type.simple.typenum=ATT_DOMAIN;

 // constants:
  nullreal_cdesc.value.real=NULLREAL;
  nullsig8_cdesc.value.int8=NONETINY;
  nullsig16_cdesc.value.int16=NONESHORT;
  nullsig32_cdesc.value.int32=
  nulltime_cdesc.value.int32=nulldate_cdesc.value.int32=
  nullts_cdesc.value.int32=
  nullmoney_cdesc.value.moneys.money_hi4=NONEINTEGER;
  nullmoney_cdesc.value.moneys.money_lo2=0;
  nullsig64_cdesc.value.int64=NONEBIGINT;
  compiler_is_init=true;
}

void alloc_dbgi(CIp_t CI)  // must be in other function than _try in VC 5.0
{ CI->dbgi=new dbg_info(CI->code_offset, CI->src_objnum); }

void register_program_compilation(cdp_t cdp, CIp_t CI)
{ cdp->RV.run_project.running_program_compilation = CI; }

objtable * get_proj_decls_table(cdp_t cdp)
{ if (cdp->RV.run_project.running_program_compilation)
  { objtable * objtab = cdp->RV.run_project.running_program_compilation->id_tables;
    while (objtab && objtab->next_table != (objtable*)&standard_table) objtab = objtab->next_table;
    return objtab;
  }
  else return cdp->RV.run_project.proj_decls ? cdp->RV.run_project.global_decls : NULL;
}

void init_defines(CIp_t CI)
// Creates standard defines (must be in a separate function)
{ t_define * def = new t_define;  if (!def) c_error(OUT_OF_MEMORY);   
  strcpy(def->name, "_WINBASE602_");  def->params=NULL;
  def->next=CI->defines;  CI->defines=def;
  def->expansion=(tptr)corealloc(10, 94);  if (!def->expansion) c_error(OUT_OF_MEMORY);
  sprintf(def->expansion, "\'%u.%u\'", WB_VERSION_MAJOR, WB_VERSION_MINOR);
  def = new t_define;  if (!def) c_error(OUT_OF_MEMORY);
  strcpy(def->name, "_LANGUAGE_");  def->params=NULL;
  def->next=CI->defines;  CI->defines=def;
  def->expansion=(tptr)corealloc(2+ATTRNAMELEN+1, 94);  if (!def->expansion) c_error(OUT_OF_MEMORY);
  strcpy(def->expansion, "\'");
  if (CI->cdp && CI->cdp->selected_lang >= 0) 
  { tobjnum objnum;
    if (!cd_Find_object(CI->cdp, "_MULTILING", CATEG_TABLE, &objnum)) 
    { const d_table * td=cd_get_table_d(CI->cdp, objnum, CATEG_TABLE);
      if (td!=NULL)
      { strcpy(def->expansion+1, ATTR_N(td, 2+CI->cdp->selected_lang)->name);
        release_table_d(td);
      }
    }
  }
  strcat(def->expansion, "\'");
  def = new t_define;  if (!def) c_error(OUT_OF_MEMORY);
  strcpy(def->name, "_602SQL_");  def->params=NULL;
  def->next=CI->defines;  CI->defines=def;
  def->expansion=(tptr)corealloc(10, 94);  if (!def->expansion) c_error(OUT_OF_MEMORY);
  sprintf(def->expansion, "\'%u.%u\'", WB_VERSION_MAJOR, WB_VERSION_MINOR);
}


CFNC DllKernel int WINAPI compile(CIp_t CI)
{ int comp_err;  objtable * ext_id_table;
  compiler_init();  // very important on Linux (calling in cd_connect is not sufficient)
  nesting_counter++;
#ifdef WINS
  __try
#else
  comp_err=Catch(CI->comp_jmp);
  if (!comp_err)
#endif
  {/* clean-up after the last compilation */
    CI->code_offset=0;
    CI->univ_code=NULL;  CI->cb_start=CI->cb_size=0;
    CI->src_line=CI->loc_line=CI->total_line=1;
    CI->sc_col=CI->loc_col=0;  /* position in the source file */
    if (CI->comp_callback) (*CI->comp_callback)(CI->src_line, CI->total_line, CI->src_name);
    if (CI->startnet==program) free_project(CI->cdp, FALSE);
   /* prepare global declarations: may include project decls */
    ext_id_table=CI->id_tables;
    CI->id_tables=(objtable*)&standard_table;
    standard_table.next_table=NULL;  /* should not be necessary */
    if (CI->startnet==program) register_program_compilation(CI->cdp, CI);
    else
    { if (CI->cdp)
      { objtable * proj_decls_table = get_proj_decls_table(CI->cdp); 
        if (proj_decls_table)
        { proj_decls_table->next_table=CI->id_tables;
          CI->id_tables=proj_decls_table;
        }
      }
    }
    if (CI->kntx==(kont_descr*)1)
    { CI->kntx=NULL; /* used by debugger when evaluating expressions */
      if (ext_id_table)
        { ext_id_table->next_table=CI->id_tables;  CI->id_tables=ext_id_table; }
    }
    CI->inaggreg=0;  CI->any_alloc=wbfalse;
    CI->compil_ptr =CI->univ_source;
    if (!CI->keynames)  /* user IPL keywords unless other specified */
      { CI->keynames=*keynames;  CI->key_count=KEY_COUNT; }
    if (CI->gener_dbg_info)
    { alloc_dbgi(CI);
      if (CI->dbgi==NULL) c_error(OUT_OF_MEMORY);
    }
    init_defines(CI);
   // init scanner and compile:
    next_char(CI);  next_sym(CI);
    (*CI->startnet)(CI);  /* starting the compilation */
   // check for EOF inside conditional compilation:
    if (CI->condcomp)
    { char buf[12];  int2str(CI->condcomp->startline, buf);
      SET_ERR_MSG(CI->cdp, buf);  c_error(EOF_IN_CONDITIONAL);
    }
    gen(CI,I_STOP);
    if (!close_comp(CI, FALSE)) c_error(CODE_GEN);
    nesting_counter--;
    return COMPILED_OK; /* compiled ok */
  }
#ifdef WINS
  __except ((GetExceptionCode() >= FIRST_COMP_ERROR && GetExceptionCode() <= LAST_COMP_ERROR ||
             GetExceptionCode() >= 128 && GetExceptionCode() <= LAST_DB_ERROR ||
             GetExceptionCode()==99)  /* ignored error */
       ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
  { comp_err=GetExceptionCode();
#else
  else
  {
#endif
    close_comp(CI, CI->temp_object_type!=temp_menu); // for menu code must not be released in this way
    if (CI->cdp)
    { run_state * RV = &CI->cdp->RV;
      RV->comp_err_line  =CI->loc_line;
      RV->comp_err_column=CI->loc_col;
    }
    if (CI->startnet==program) free_project(CI->cdp, FALSE); /* project may have been partially created */
#ifdef WINS
#if WBVERS<900
    free_aggr_info(&CI->aggr);
#endif
#endif
    nesting_counter--;
    return comp_err;
  }
}

CFNC DllKernel void WINAPI set_compiler_keywords(CIp_t CI,
                           char * key_names, unsigned key_count, int keyset)
{ if (key_names != NULL)
    { CI->keynames=key_names;  CI->key_count=key_count; }
  else switch (keyset)
  { case 0: CI->keynames=*    keynames;  CI->key_count=    KEY_COUNT;  break;
    case 1: CI->keynames=*sql_keynames;  CI->key_count=SQL_KEY_COUNT;  break;
  }
}

CFNC DllExport unsigned WINAPI ipj_typesize(typeobj * tobj)
{ if (DIRECT(tobj))
  { int tp = ATT_TYPE(tobj);
    return (tp == ATT_FILE) ? 2 :
           IS_HEAP_TYPE(tp) ? 0 :
           tpsize[tp];
  }
  else return tobj->type.anyt.valsize;
}

/////////////////////////////////// multilingual support ///////////////////////////////////////////////////
CFNC DllKernel BOOL WINAPI Select_language(cdp_t cdp, int language)
{ 
#if WBVERS<900
  ttablenum objnum;  char query[60+ATTRNAMELEN];  tcursnum curs;  trecnum cnt;
 // release the old language:
  if (cdp->lang_cache) 
  { cache_free(cdp->lang_cache, 0, cdp->cache_recs);
    delete cdp->lang_cache;  cdp->lang_cache=NULL;
    cdp->cache_recs=0;
    cdp->selected_lang=-1;
  }
 // load the new language data:
  if (language>=0)
  { if (cd_Find_object(cdp, "_MULTILING", CATEG_TABLE, &objnum)) return FALSE;
    const d_table * td=cd_get_table_d(cdp, objnum, CATEG_TABLE);
    if (td==NULL) return FALSE;
    if (2+language >= td->attrcnt) { release_table_d(td);  return FALSE; }
    const d_attr * pattr=ATTR_N(td, 2+language);
    sprintf(query, "SELECT ID, `%s` FROM _MULTILING ORDER BY ID", pattr->name);
    BOOL heaptype = IS_HEAP_TYPE(pattr->type);
    release_table_d(td);
    if (cd_Open_cursor_direct(cdp, query, &curs)) return FALSE;
    cd_Rec_cnt(cdp, curs, &cnt);
    cdp->lang_cache=create_cached_access(cdp, NULL, NULL, FALSE, curs);
    if (cdp->lang_cache==NULL) { cd_Close_cursor(cdp, curs);  return FALSE; }
   // prepare cache:
    cdp->lang_cache->rec_cache_size=cdp->lang_cache->rec_cache_real;
    cdp->lang_cache->cache_hnd=GlobalAlloc(GMEM_MOVEABLE, cnt*cdp->lang_cache->rec_cache_size);
    if (!cdp->lang_cache->cache_hnd) 
    { client_error(cdp, OUT_OF_MEMORY);  cd_Close_cursor(cdp, curs);  delete cdp->lang_cache;  cdp->lang_cache=NULL;  return FALSE; }
    cdp->lang_cache->cache_ptr=(tptr)GlobalLock(cdp->lang_cache->cache_hnd);
   // load cache:
    cache_load(cdp->lang_cache, NULL, 0, cnt, 0);
    cdp->cache_recs=cnt;
   // close the cursor now:
    cd_Close_cursor(cdp, curs);
    cdp->lang_cache->cursnum=NOOBJECT;
   // convert the IDs to upper case:
    for (unsigned i=0;  i<cnt;  i++)
      Upcase(cdp->lang_cache->cache_ptr + i*cdp->lang_cache->rec_cache_size + 1);
  }
  cdp->selected_lang=language;
  return TRUE;
#else
  return FALSE;
#endif
}

#if WBVERS<900

CFNC DllKernel int WINAPI Get_language(cdp_t cdp)
{ 
    return cdp->selected_lang;
}

#endif

sig32 get_num_val(CIp_t CI, sig32 lbd, sig32 hbd)
{ sig32 val;
  if (CI->cursym=='=') next_sym(CI);
  if (CI->cursym!=S_INTEGER) c_error(INTEGER_EXPECTED);
  if ((CI->curr_int<lbd) || (CI->curr_int>hbd)) c_error(INT_OUT_OF_BOUND);
  val=(sig32)CI->curr_int;
  next_sym(CI);
  return val;
}

