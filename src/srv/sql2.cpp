/****************************************************************************/
/* sql2.cpp - sql query analysis & evaluation code generation               */
/****************************************************************************/
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "sdefs.h"
#include "scomp.h"
#include "basic.h"
#include "frame.h"
#include "kurzor.h"
#include "dispatch.h"
#include "table.h"
#include "dheap.h"
#include "curcon.h"
#pragma hdrstop
#include "nlmtexts.h"
#include "profiler.h"
#include <math.h>
#include "opstr.h"
#include "netapi.h"
#include "dbgprint.h"
#include "replic.h"
#ifndef WINS
#include <stdlib.h>
#include <wctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <dlfcn.h>
#else
#include <winnls.h>
#endif
#include <stddef.h>  // of
#include "flstr.h"
#include "enumsrv.h"
// Mozna vylepseni: optimalizace predikatu IN, aby mohl vyuzit index
#define STABILIZED_GROUPING
//#define UNSTABLE_PMAT  // temp.

#if SQL3
#define OPTIM_IN  // closing data is not complete, other problem is with many deleted records in HAVING (Q18 in TCPD)
t_mater_ptrs * create_or_clear_pmat(cdp_t cdp, t_query_expr * qe);
#endif
static void passing_open_close(cdp_t cdp, t_optim * opt, BOOL openning);

#define LIKE_FCNUM 106
enum t_complcond { NULL_VALUE_ELIMINATED };

void completion_condition(cdp_t cdp, t_complcond num)
{
}

BOOL sql_exception(cdp_t cdp, int num)
{ return request_error(cdp, num); }

void reset_curr_date_time(cdp_t cdp)
{ cdp->curr_date=cdp->curr_time=0; }

void data_not_closed(void)
#ifdef WINS
{ MessageBeep(-1); }
#else
{ }
#endif

static inline BOOL is_integer(int type, t_specif specif)
{ return (type==ATT_INT32 || type==ATT_INT16 || type==ATT_INT8) && specif.scale==0; }

static inline BOOL is_char_string(int tp)
{ return IS_STRING(tp) || tp==ATT_CHAR || tp==ATT_TEXT || tp==ATT_EXTCLOB; }

////////////////////////////////// kontext ///////////////////////////////////
// db_kontext ma tyto funkce:
// 1. Umoznuje kompilovat jmena atributu ve sve scope
// 2. Po kompilaci obsahuje jmena atributu mezivysledku smerem ven ze scope
// 3. Pri behu obsahuje bud pozici v tabulce (plus cislo, je-li docasna), je-li materialni,
//    nebo entice pointru na zaznamy nizsi urovne a pozici,
//    nebo se vyuzije odkaz na nizsi uroven, pripadne vycislovaci vyraz.

static const char * elem_prefix(const db_kontext * akont, const elem_descr * el)
// Returns possible prefix of the kontext element el from akont (may be empty string).
{ 
  while (!*akont->prefix_name) // prefix not specified in this kontext
    if      (el->link_type==ELK_KONTEXT)
      { akont=el->kontext.kont;   el=akont->elems.acc0(el->kontext.down_elemnum);  }
    else if (el->link_type==ELK_KONT21 || el->link_type==ELK_KONT2U) // to asi norma nevyzaduje
      { akont=el->kontdbl.kont1;  el=akont->elems.acc0(el->kontdbl.down_elemnum1); }
    else break; // returns the empty prefix
  return akont->prefix_name;
}

elem_descr * db_kontext::find(const char * prefix, const char * name, int & elemnum)
// Searches the column in the kontext. Prefix must be NULL if not specified.
// Returns element number and pointer or NULL if not found.
// If top level prefix not specified, must match low level prefix.
{ 
  if (prefix_is_compulsory && prefix==NULL)
    return NULL;
  for (int i=0;  i<elems.count();  i++)
  { elem_descr * el = elems.acc0(i);
    if (!strcmp(el->name, name) &&
        (prefix==NULL || !strcmp(prefix, elem_prefix(this, el))))
    { elemnum=i;  
      referenced |= el->link_type==ELK_NO ? TRUE : 2;  // TRUE-low level referencing
      return el;
    }
  }
  return NULL;
}

static elem_descr * find_list(CIp_t CI, const char * prefix,
        const char * name, int & elemnum, db_kontext * & kont)
// Searches the column in the kontext list.
{ db_kontext * ckont = CI->sql_kont->active_kontext_list;
  elem_descr * el = NULL;  // the result
  while (ckont!=NULL)
  // INV: ckont is the current kontext searched
  { el = ckont->find(prefix, name, elemnum);
    if (el!=NULL) break;
    ckont=ckont->next;
  }
  kont=ckont;
  return el;
}

BOOL db_kontext::fill_from_td(cdp_t cdp, table_descr * td)
{ int i;  elem_descr * el;  attribdef * att;
 // find column privileges -> reported to the client, but not used on the server side:
  t_privils_flex priv_val(td->attrcnt);
  if (td->tbnum<0) priv_val.set_all_rw();  
  else cdp->prvs.get_effective_privils(td, NORECNUM, priv_val);
  strcpy(prefix_name, td->selfname);
  if (elems.acc(td->attrcnt-1) == NULL)  // allocate space for columns
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
  for (i=0, el=elems.acc0(0), att=td->attrs;  i<td->attrcnt;
       i++, el++,             att++)
  { strcpy(el->name,   att->name);
    el->type    =att->attrtype;
    el->multi   =att->attrmult;
    el->specif  =att->attrspecif;
    el->nullable=att->nullable;
    if (!i) el->pr_flags = *priv_val.the_map() & RIGHT_DEL ? PRFL_READ_TAB | PRFL_WRITE_TAB : PRFL_READ_TAB;
    else
    { if (priv_val.has_read(i)) el->pr_flags=PRFL_READ_TAB;
      else if (td->tabdef_flags & TFL_REC_PRIVILS) el->pr_flags=PRFL_READ_REC;
      else el->pr_flags=PRFL_READ_NO;
      if (priv_val.has_write(i)) el->pr_flags|=PRFL_WRITE_TAB;
      else if (td->tabdef_flags & TFL_REC_PRIVILS) el->pr_flags|=PRFL_WRITE_REC;
    }
    el->link_type=ELK_NO;
    el->dirtab.tabnum=td->tbnum;
  }
  privil_cache = td->rec_privils_atr==NOATTRIB ? new t_privil_cache : new t_record_privil_cache;
  return TRUE;
}

BOOL db_kontext::fill_from_ta(cdp_t cdp, const table_all * ta)
{ const atr_all * att;  elem_descr * el;  int i;
  strcpy(prefix_name, ta->selfname);
  if (elems.acc(ta->attrs.count()-1) == NULL)  // allocate space for columns
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
  for (i=0, el=elems.acc0(0), att=ta->attrs.acc0(i);  i<ta->attrs.count();
       i++, el++,             att++)
  { strcpy(el->name,   att->name);
    el->type    =att->type;
    el->multi   =att->multi;
    el->specif  =att->specif;
    el->nullable=att->nullable;
    if (el->type==ATT_DOMAIN)  // translate the domain
    { t_type_specif_info tsi;
      if (compile_domain_to_type(cdp, el->specif.domain_num, tsi))
        { el->type=tsi.tp;  el->specif=tsi.specif;  el->nullable=tsi.nullable; }
    }
    if (!i) el->pr_flags = PRFL_READ_TAB | PRFL_WRITE_TAB;
    else    el->pr_flags = PRFL_READ_TAB | PRFL_WRITE_TAB;
    el->link_type=ELK_NO;
    el->dirtab.tabnum=NOTBNUM;
  }
  privil_cache = NULL;
  return TRUE;
}

BOOL db_kontext::fill_from_ivcols(const t_ivcols * cols, const char * name)
{ int i;  elem_descr * el;  
  strcpy(prefix_name, name);
  el=elems.acc(0);  
  if (!el) return FALSE;
  { strcpy(el->name, "_DELETED");
    el->type         =ATT_BOOLEAN;
    el->multi        =1;
    el->specif.set_null();
    el->nullable     =TRUE;
    el->pr_flags     =PRFL_READ_TAB|ELFL_NOEDIT;
    el->link_type    =ELK_NO;
    el->dirtab.tabnum=NOTBNUM;  // prevents writing into it 
  }
  for (i=1;  *cols->name;  i++, cols++)
  { el=elems.acc(i);  if (!el) return FALSE;
    strcpy(el->name, cols->name);
    el->type         =cols->type;
    el->multi        =1;
    int charset = cols->length==OBJ_NAME_LEN || cols->length==ATTRNAMELEN ? sys_spec.charset : 0;  // approx.
    if (el->type==ATT_STRING)
      el->specif       =t_specif(cols->length, charset, TRUE, FALSE);
    else if (el->type==ATT_BINARY)
      el->specif       =t_specif(cols->length);
    else if (el->type==ATT_INT8 || el->type==ATT_INT16 || el->type==ATT_INT32 || el->type==ATT_INT64)
      el->specif       =t_specif(cols->length);  // scale
    else
      el->specif=0;
    if (!strcmp(cols->name, "LOG_MESSAGE_W")) el->specif.wide_char=1;
    el->nullable     =TRUE;
    el->pr_flags     =PRFL_READ_TAB|ELFL_NOEDIT;
    el->link_type    =ELK_NO;
    el->dirtab.tabnum=NOTBNUM;  // prevents writing into it
  }
  return TRUE;
}

void db_kontext::set_correlation_name(const char * name)
{ 
  strcpy(prefix_name, name);
}

void db_kontext::set_compulsory_prefix(const char * name)
{ set_correlation_name(name);
  prefix_is_compulsory=true;
}  


int db_kontext::first_nonsys_col(void)
{ int i;
  for (i=1;  i<elems.count();  i++)
     if (!IS_SYSTEM_COLUMN(elems.acc0(i)->name)) // unless system
       break;
  return i;
}

bool db_kontext::is_mater_completed(void) const 
{ 
#if SQL3
  return (t_mat ? t_mat->completed : virtually_completed) != 0; 
#else
  return mat ? !mat->is_ptrs && mat->completed : virtually_completed; 
#endif
}

BOOL db_kontext::copy_kont(CIp_t CI, db_kontext * patt, int start)
{ next=NULL;
  for (int i=start;  i<patt->elems.count();  i++)
    if (!copy_elem(CI, i, patt, 0)) return FALSE;
  return TRUE;
}

void elem_descr::store_kontext(const elem_descr * el1, db_kontext * k1, int elemnum1)
// Defines this column as a link to a column from another kontext.
{ *this=*el1;
  link_type=ELK_KONTEXT;
  kontext.kont=k1;  kontext.down_elemnum=elemnum1;
}

void elem_descr::store_double_kontext(const elem_descr * el1, const elem_descr * el2, 
                                      db_kontext * k1, db_kontext * k2, int elemnum1, int elemnum2, bool is_union)
// Defines this column as a link to 2 columns from other kontexts.
{ 
  *this = el1->type ? *el1 : *el2;
  pr_flags = el1->pr_flags & el2->pr_flags;
  if (is_union || el1->pr_flags & ELFL_NOEDIT /*|| el2->pr_flags & ELFL_NOEDIT*/) // for EXCEPT and INTERSECT the editability depends on the 1st operand
    pr_flags |= ELFL_NOEDIT;
  link_type=is_union ? ELK_KONT2U : ELK_KONT21;
  kontdbl.kont1=k1;  kontdbl.down_elemnum1=elemnum1;
  kontdbl.kont2=k2;  kontdbl.down_elemnum2=elemnum2;
}

void elem_descr::mark_core(void)
{ if (link_type==ELK_EXPR || link_type==ELK_AGGREXPR)
    if (expr.eval) expr.eval->mark_core();
}

elem_descr * db_kontext::copy_elem(CIp_t CI, int delemnum, db_kontext * dkont, int common_limit)
// Pokud v kontextu neni v casti do common_limit sloupec stejneho jmena, pak do nej prida novy sloupec.
// Vlastnosti sloupce prevezne z el, nastavi pro nej link na delemnum v dkont.
// Vrati pridany nebo nalezeny sloupec.
// Predpoklada, ze common_limit < elems.count().
{ const elem_descr * el = dkont->elems.acc0(delemnum);
  for (int i=1;  i<common_limit;  i++)  // do not reinsert common columns
    if (!strcmp(elems.acc0(i)->name, el->name))
      return elems.acc0(i);  // name found
  elem_descr * el2 = elems.next();
  if (el2==NULL) if (CI) c_error(OUT_OF_MEMORY);  else return NULL;
  el2->store_kontext(el, dkont, delemnum);
  return el2;
}

void db_kontext::init_curs(CIp_t CI)
// initializes cursor kontext by the __DELETED column
{ elem_descr * el = elems.next();
  if (el==NULL) c_error(OUT_OF_MEMORY);
  strcpy(el->name, "__DELETED");
  el->type=ATT_BOOLEAN;  el->multi=1;  el->nullable=TRUE;
  el->pr_flags=PRFL_READ_TAB|PRFL_WRITE_TAB;
  el->link_type=ELK_EXPR;//ELK_NO;  -- allows reading the __DELETED column
  uns32 zero32 = 0;
  el->expr.eval = new t_expr_sconst(ATT_BOOLEAN, t_specif(), &zero32, sizeof(zero32));
  if (el->expr.eval==NULL) c_error(OUT_OF_MEMORY);
}

db_kontext::~db_kontext(void)
{ 
#if SQL3
  if (p_mat!=NULL) delete p_mat;
  if (t_mat!=NULL) delete t_mat;
#else
  if (mat!=NULL) delete mat;
#endif
  for (int i=0;  i<elems.count();  i++)
  { elem_descr * el = elems.acc0(i);
    if (el->link_type==ELK_EXPR || el->link_type==ELK_AGGREXPR)
      delete el->expr.eval;
  }
  if (privil_cache!=NULL) delete privil_cache;
}

void db_kontext::mark_core(void)
{ for (int i=0;  i<elems.count();  i++)
    elems.acc0(i)->mark_core();
  elems.mark_core();
#if SQL3
  if (t_mat!=NULL) t_mat->mark_core();
  if (p_mat!=NULL) p_mat->mark_core();
#else
  if (mat!=NULL) mat->mark_core();
#endif
  if (privil_cache!=NULL) 
    { mark(privil_cache);  privil_cache->mark_core(); }
}

BOOL resolve_access(db_kontext * akont, int elemnum, db_kontext *& _kont, int & _elemnum)
// Replace cursor element access by table attribute access. Return FALSE if cannot.
{ if (akont->replacement!=NULL) akont=akont->replacement;
  do
  { if (elemnum >= akont->elems.count()) return FALSE;
    if (akont->is_mater_completed())
    { 
#if SQL3
      if (!akont->t_mat || !akont->t_mat->permanent) return FALSE;  // temporary table
#else
      if (!akont->mat || !((t_mater_table*)akont->mat)->permanent) return FALSE;  // temporary table
#endif
     attr_exit:
      _kont=akont;  _elemnum=elemnum;
      return TRUE;
    }
    elem_descr * el =akont->elems.acc0(elemnum);
    switch (el->link_type)
    { case ELK_KONTEXT:
        akont=el->kontext.kont;   elemnum=el->kontext.down_elemnum;   break;
      case ELK_KONT21:  // resolving to the 1st operand
        akont=el->kontdbl.kont1;  elemnum=el->kontdbl.down_elemnum1;  break;
      case ELK_EXPR:  case ELK_AGGREXPR:
      case ELK_KONT2U:  // not resolving union
        return FALSE;
      case ELK_NO: // table level, but akont->mat not set (e.g. UPDATE statement)
        goto attr_exit;
    }
  } while (TRUE);
}

void is_referenced(db_kontext * akont, int elemnum)
{ do
  { if (elemnum >= akont->elems.count()) return; // fuse
#if SQL3
    if (akont->t_mat && akont->t_mat->completed) 
    { akont->t_mat->is_referenced(elemnum);
#else
    if (akont->mat && !akont->mat->is_ptrs && akont->mat->completed) 
    { t_mater_table * mater = (t_mater_table*)akont->mat;
      mater->is_referenced(elemnum);
#endif
      return;
    }
    elem_descr * el =akont->elems.acc0(elemnum);
    switch (el->link_type)
    { case ELK_KONTEXT:
        akont=el->kontext.kont;  elemnum=el->kontext.down_elemnum;  break;
      case ELK_EXPR:  case ELK_AGGREXPR:
        el->expr.eval->is_referenced();  return;
      case ELK_KONT2U:  case ELK_KONT21:
        is_referenced(el->kontdbl.kont1, el->kontdbl.down_elemnum1);
        is_referenced(el->kontdbl.kont2, el->kontdbl.down_elemnum2);
        return;
      case ELK_NO: // table level, but akont->mat not set (e.g. UPDATE statement)
        return;
    }
  } while (TRUE);
}

void t_mater_table::is_referenced(int elemnum)
{ refmap.add(elemnum); }

BOOL attr_access(db_kontext * akont, int elemnum,
 ttablenum & tabnum, tattrib & attrnum, trecnum & recnum, elem_descr * & attracc)
// Returns either tabnum, attrnum, recnum or attracc.
// In the 2nd case tabnum is 0 because it may be used to index tables[].
{ if (akont->replacement!=NULL) akont=akont->replacement;
  do
  { if (elemnum >= akont->elems.count()) return FALSE;
    if (akont->is_mater_completed())
    { 
#if SQL3
      tabnum = akont->t_mat ? akont->t_mat->tabnum : NOTBNUM;
#else
      tabnum = akont->mat ? ((t_mater_table*)akont->mat)->tabnum : NOTBNUM;
#endif
     attr_exit:
      attrnum=elemnum;  recnum=akont->position;
      attracc=NULL;
      return TRUE;
    }
    elem_descr * el =akont->elems.acc0(elemnum);
    switch (el->link_type)
    { case ELK_KONTEXT:
        akont=el->kontext.kont;  elemnum=el->kontext.down_elemnum;  break;
      case ELK_EXPR:  case ELK_AGGREXPR:
        attracc = el;  attrnum=NOATTRIB;  tabnum=0;  return TRUE;
      case ELK_KONT2U:  case ELK_KONT21:
        if (!akont->position)
          { akont=el->kontdbl.kont1;  elemnum=el->kontdbl.down_elemnum1; }
        else
          { akont=el->kontdbl.kont2;  elemnum=el->kontdbl.down_elemnum2; }
        break;
      case ELK_NO: // table level, but akont->mat not set (e.g. UPDATE statement)
        tabnum=el->dirtab.tabnum;  goto attr_exit;
    }
  } while (TRUE);
}

void attr_access_light(const db_kontext *& akont, int & elemnum, trecnum & recnum)
// Resolves column access with respect to the record [recnum]
{ if (akont->replacement!=NULL) akont=akont->replacement;
  while (!akont->is_mater_completed())
  { elem_descr * el =akont->elems.acc0(elemnum);
    if (el->link_type==ELK_KONTEXT)
    { akont=el->kontext.kont;  
      elemnum=el->kontext.down_elemnum; 
    }
    else if (el->link_type==ELK_KONT2U || el->link_type==ELK_KONT21)
      if (!akont->position)
        { akont=el->kontdbl.kont1;  elemnum=el->kontdbl.down_elemnum1; }
      else
        { akont=el->kontdbl.kont2;  elemnum=el->kontdbl.down_elemnum2; }
    else // ELK_EXPR, ELK_AGGREXPR, 
         // ELK_NO:  table level, but akont->mat not set (e.g. UPDATE statement)
      break;
  } 
  recnum=akont->position;
}

__forceinline sig64 money2i64(monstr * mon)
{ char buf[8];
  memcpy(buf, mon, sizeof(monstr));
  *(uns16*)(buf+6) = (mon->money_hi4 & 0x80000000) ? 0xffff : 0;
  return *(sig64*)buf;
}

void tb_attr_value(cdp_t cdp, table_descr * tbdf, trecnum recnum, int elemnum, int type, t_specif specif, t_express * indexexpr, t_value * res)
// Reads the value from the table to res or stores the access to it
{ uns16 indxval;
 // process multiattributes (evaluate index or return access):
  if (indexexpr!=NULL)  // indexed multiattribute
  { expr_evaluate(cdp, indexexpr, res);  // returns integer result
    if (res->is_null() || res->intval<0 || res->intval>0xffff) 
      { request_error(cdp, INDEX_OUT_OF_RANGE);  return; }
    indxval=(uns16)res->intval;
    type &= 0x7f;  // remove multiattribute flag
  }
  else   // not indexed
    if (type & 0x80)    // not indexed multiattribute: return access only
    { res->set_db_acc(tbdf->tbnum, recnum, elemnum, NOINDEX);
      return;
    }
 // if multiattribute then index computed:
  if (IS_HEAP_TYPE(type) || IS_EXTLOB_TYPE(type)) // return access & length
  { uns32 len;
    if (indexexpr==NULL)
    { res->set_db_acc(tbdf->tbnum, recnum, elemnum, NOINDEX);
      tb_read_atr_len(cdp, tbdf, recnum, elemnum, &len);
    }
    else
    { res->set_db_acc(tbdf->tbnum, recnum, elemnum, indxval);
      tb_read_ind_len(cdp, tbdf, recnum, elemnum, indxval, &len);
    }
    res->length=len;  // vmode==mode_access, not null implied
    // if (len==0) res->set_null();  -- no, I want to compare empty texts!
  }
  else  // simple type - direct read of the value:
  { tptr destptr;  BOOL err;
    if (SPECIFLEN(type) && specif.length>MAX_DIRECT_VAL)
    { if (res->allocate(specif.length)) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }
      destptr=res->indir.val;   // can do this because size>MAX_DIRECT_VAL and allocate returned false
    }
    else 
    { destptr=res->strval;  res->set_simple_not_null(); }
    if (indexexpr!=NULL)
      err=tb_read_ind(cdp, tbdf, recnum, elemnum, indxval, destptr);
    else
      err=tb_read_atr(cdp, tbdf, recnum, elemnum, destptr);

    if (err) res->set_null();
    else
      Fil2Mem_convert(res, destptr, type, specif);
  }
}

void attr_value(cdp_t cdp, db_kontext * kont, int elemnum, t_value * res, int type, t_specif specif, t_express * indexexpr)
// Returns db access for multiatributes, db access and lenght for var-len 
// attributes, value for all others.
// Rule: res does not contain indirect value on input, may be indirect on output (depending on type).
{ table_descr * tbdf;  trecnum recnum; 
  elem_descr * el;
  if (kont==NULL)  // "current" table kontext
    { tbdf=(table_descr*)cdp->kont_tbdf;  recnum=cdp->kont_recnum; }
  else
  { ttablenum tabnum;
    if (kont->replacement!=NULL) kont=kont->replacement;
    do
    { t_materialization * mat;
      el = kont->elems.acc(elemnum);
      if (kont->t_mat && kont->t_mat->completed)  // virtual completion must be ignored when I need the value. 
      //if (kont->is_mater_completed()) // kontext materialized into a table -- crashes when a subquery used in SELECT clause is INSENSITIVE
      { 
#if SQL3
        mat = kont->t_mat;
        tabnum = kont->t_mat ? kont->t_mat->tabnum : NOTBNUM;
#else
        mat = kont->mat;
        tabnum = mat ? ((t_mater_table*)mat)->tabnum : NOTBNUM;
#endif
       attr_exit:
        recnum=kont->position;
        break;
      }
      else switch (el->link_type)
      { case ELK_NO:
          tabnum=el->dirtab.tabnum;  goto attr_exit;
        case ELK_KONTEXT:
          kont=el->kontext.kont;   elemnum=el->kontext.down_elemnum;
          break;
        case ELK_KONT2U:  case ELK_KONT21:
          if (!kont->position)
            { kont=el->kontdbl.kont1;  elemnum=el->kontdbl.down_elemnum1; }
          else
            { kont=el->kontdbl.kont2;  elemnum=el->kontdbl.down_elemnum2; }
          break;
        case ELK_EXPR:  case ELK_AGGREXPR:
          expr_evaluate(cdp, el->expr.eval, res);
          return;
      }
    } while (TRUE);
    if (recnum==OUTER_JOIN_NULL_POSITION) { res->set_null();  return; }
    tbdf=tables[tabnum];
   // check the read privilege (privil_cache==NULL is _iv_ cursors, temp tables):
    if (cdp->check_rights && !cdp->has_full_access(tbdf->schema_uuid) && elemnum && kont->privil_cache &&
        !kont->privil_cache->can_read_column(cdp, elemnum, tbdf, recnum))
    {// no read privilege:
      res->set_null();  
      if (!(cdp->sqloptions & SQLOPT_RD_PRIVIL_VIOL) && cdp->check_rights!=2) 
      { t_exkont_table ekt(cdp, tabnum, recnum, elemnum, NOINDEX, OP_READ, NULL, 0);
        request_error(cdp, NO_RIGHT);  
      }  
      return; 
    }
  }
  tb_attr_value(cdp, tbdf, recnum, elemnum, type, specif, indexexpr, res);
}

static int get_privilege(cdp_t cdp, int fcnum, t_value * access, int info_type, const char * subject_name, int subject_category)
{
 // Find the subject:
  tobjnum subjnum;  privils prvs(cdp), *pprivs;
  if (subject_name) 
  { subjnum = find_object(cdp, subject_category, subject_name);
    if (subjnum==NOOBJECT) return -1;
    prvs.set_user(subjnum, (tcateg)subject_category, info_type==2); // must build the hierarchy for effective privils
    pprivs=&prvs;
  }
  else  // using privileges of the logged user
    pprivs=&cdp->prvs;
 // get privils descriptor:
  table_descr_auto td(cdp, access->dbacc.tb);
  if (!td->me()) return -1;  // error
  t_privils_flex priv_val(td->attrcnt);
  if (info_type==2)
    pprivs->get_effective_privils(td->me(), access->dbacc.rec, priv_val);
  else
    pprivs->get_own_privils(td->me(), info_type==1 || fcnum==FCNUM_INSPRIV ? (trecnum)-1 : access->dbacc.rec, priv_val);
  switch (fcnum)
  { case FCNUM_SELPRIV:  
      return priv_val.has_read(access->dbacc.attr);
    case FCNUM_UPDPRIV:
      return priv_val.has_write(access->dbacc.attr);
    case FCNUM_DELPRIV:  
      return priv_val.has_del();
    case FCNUM_INSPRIV:
      return priv_val.has_ins();
    case FCNUM_GRTPRIV:
      return priv_val.has_grant();
  }
  return 0;
}

BOOL t_value::allocate(int space)  // On input vmode!=mode_indirect supposed.
{ if (vmode==mode_indirect)
  { if (indir.vspace>=space) return FALSE;
    free_ival();
  }
  if (space <= MAX_DIRECT_VAL) { vmode = mode_simple;  return FALSE; }
  indir.val=(tptr)malloc(space+2);  // +2 for wide-character terminator
  if (indir.val==NULL) { vmode = mode_null;  return TRUE; }
  indir.vspace=space;
  vmode=mode_indirect;
  return FALSE;
}

t_value & t_value::operator=(const t_value & src)
{ set_null();
  if (src.vmode==mode_indirect)
  { if (allocate(src.indir.vspace)) return *this;
    if (src.indir.val) 
      { memcpy(valptr(), src.indir.val, src.length+2);  length=src.length; } // copying with the terminator
    else length=0; // after free_ival indir.val==NULL but length may be !=0
  }
  else 
  { memcpy(strval, src.strval, sizeof(strval));
    vmode=src.vmode;  length=src.length;
  }
  return *this;
}

BOOL t_value::reallocate(int space)  // On input vmode!=mode_indirect && length defined (even for NULL) supposed.
// Ensures that valptr points to buffer of at least "space" chars. Returns TRUE on error.
{ if (vmode==mode_indirect ? indir.vspace>=space : space <= MAX_DIRECT_VAL) return FALSE;
  if (space <= MAX_DIRECT_VAL)  // change indirect to direct
  { char aux_strval[MAX_DIRECT_VAL+2];
    memcpy(aux_strval, indir.val, MAX_DIRECT_VAL+2);
    set_simple_not_null();  // overwrites strval!!
    memcpy(strval, aux_strval, MAX_DIRECT_VAL+2);
  }
  else
  { tptr newval=(tptr)malloc(space+2);
    if (!newval) return TRUE;
    memcpy(newval, valptr(), length+2);
    free_ival();
    indir.vspace=space;
    indir.val=newval;
    vmode=mode_indirect;
  }
  return FALSE;
}

BOOL t_value::load(cdp_t cdp)
// Loads var-len value from database, never returns mode_null.
{ if (vmode!=mode_access) return FALSE;  // alredy loaded
  trecnum crec=dbacc.rec;  // copy necessary, values will be overwritten
  tattrib cattr=dbacc.attr; uns16 cindex=dbacc.index;
  table_descr * tbdf = tables[dbacc.tb];
  if (tbdf==(table_descr*)-1)  // locked, moving data in ALTER TABLE
    if (cdp->kont_tbdf && cdp->kont_tbdf->tbnum==dbacc.tb)
      tbdf=(table_descr*)cdp->kont_tbdf;
 // length:
  if (cindex!=NOINDEX)
    tb_read_ind_len(cdp, tbdf, crec, cattr, cindex, (uns32*)&length);
  if (!length) { *(wuchar*)strval=0;  set_null();  return FALSE; }
 // allocate:
  if (allocate(length)) return TRUE;
  char * destptr = valptr();
 // load:
  BOOL res;
  if (cindex==NOINDEX)
    res=tb_read_var(cdp, tbdf, crec, cattr, 0, length, destptr);
  else
    res=tb_read_ind_var(cdp, tbdf, crec, cattr, cindex, 0, length, destptr);
  if (res) length=0;
  else     length=cdp->tb_read_var_result;
  destptr[length]=destptr[length+1]=0;
  return res;
}

#ifdef STOP
void t_value::set_null_state(int type, int specif)
// Checks the loaded value for NULL, sets vmode. Not for mode_access.
{ BOOL isnull;
  if (IS_HEAP_TYPE(type)) isnull=!length;
  else isnull=::IS_NULL(valptr(), type, specif);
  if (vmode==mode_indirect) 
    { if (isnull) set_null();
  else
    vmode=isnull ? mode_null : mode_simple;
}
#endif
///////////////////////////////// cast ////////////////////////////////////////////
t_convert get_type_cast(int srctp, int desttp, t_specif srcspecif, t_specif destspecif, bool implicit)
// Returns the conversion from srctp to desttp, from srcspecif to destspecif, 
// or returns CONV_ERROR if such conversion does not exist.
{ t_conv_type conv;
  if (srctp==desttp && srcspecif==destspecif) return t_convert(CONV_NO);
  if (is_char_string(srctp))
  { switch (desttp)
    { case ATT_STRING:  case ATT_CHAR:  case ATT_TEXT:  case ATT_EXTCLOB:
        if (srcspecif.wide_char)
          return t_convert(implicit ? CONV_W2S:CONV_W2SEXPL, srcspecif.charset, destspecif.charset);
        else
          return t_convert(implicit ? CONV_S2S:CONV_S2SEXPL, srcspecif.charset, destspecif.charset);
      case ATT_NOSPEC:  case ATT_EXTBLOB:
        return t_convert(CONV_NO);   // no conversion: no length check
      case ATT_DATE:
        return t_convert(srcspecif.wide_char ? CONV_W2D  : CONV_S2D);  
      case ATT_TIME:                      
        return t_convert(srcspecif.wide_char ? CONV_W2T  : CONV_S2T,  0, destspecif.with_time_zone);  
      case ATT_TIMESTAMP: case ATT_DATIM: 
        return t_convert(srcspecif.wide_char ? CONV_W2TS : CONV_S2TS, 0, destspecif.with_time_zone); 
      default:
        if (implicit) return t_convert(CONV_ERROR);
        else switch (desttp)  // from string or text to ...
        { case ATT_INT16:  case ATT_INT32:  case ATT_INT8:
            return t_convert(srcspecif.wide_char ? CONV_W2I : CONV_S2I, destspecif.scale);  
          case ATT_MONEY:  case ATT_INT64:    
            return t_convert(srcspecif.wide_char ? CONV_W2M : CONV_S2M, destspecif.scale);  
          case ATT_FLOAT:                     
            return t_convert(srcspecif.wide_char ? CONV_W2F : CONV_S2F, destspecif.scale);  
          case ATT_BOOLEAN:
            return t_convert(srcspecif.wide_char ? CONV_W2BOOL : CONV_S2BOOL);
          case ATT_BINARY:  // added in v.11.0.0.0723
            return t_convert(CONV_VAR2BIN, destspecif.length);
          default:
            return t_convert(CONV_ERROR);
        }
    }
  }
  else switch (desttp)
  { case ATT_BOOLEAN:  // used only when specif's are different by accident
      switch (srctp)
      { case ATT_BOOLEAN: return t_convert(CONV_NO);
        case ATT_INT32:  case ATT_INT16:  case ATT_INT8:
          if (!srcspecif.scale) return t_convert(CONV_I2BOOL);  else break;
      }
      return t_convert(CONV_ERROR);
    case ATT_INT16:  case ATT_INT32:  case ATT_INT8:
      switch (srctp)
      { case ATT_MONEY:  case ATT_INT64:  conv=CONV_M2I;  break;
        case ATT_FLOAT:                   conv=CONV_F2I;  break;
        case ATT_INT32:  case ATT_INT16:  
        case ATT_INT8:                    conv=CONV_I2I;  break;  // may have different scales and/or different limits
        case ATT_BOOLEAN: return t_convert(CONV_NO);
        default: return t_convert(CONV_ERROR); // must not compare "specif"
      }
      return t_convert(conv, destspecif.scale-srcspecif.scale);
    case ATT_MONEY:  case ATT_INT64:
      switch (srctp)
      { case ATT_FLOAT:  conv=CONV_F2M;  break;
        case ATT_INT16:  case ATT_INT32:  case ATT_INT8:
                         conv=CONV_I2M;  break;
        case ATT_MONEY:  case ATT_INT64:
                         conv=CONV_M2M;  break;
        default: return t_convert(CONV_ERROR); // must not compare "specif"
      }
      return t_convert(conv, destspecif.scale-srcspecif.scale);
    case ATT_FLOAT: // destspecif is usualy 0, but need not to be
      switch (srctp)
      { case ATT_MONEY:  case ATT_INT64:  
                         conv=CONV_M2F;  break;
        case ATT_INT16:  case ATT_INT32:  case ATT_INT8:  
                         conv=CONV_I2F;  break;
        case ATT_FLOAT:  conv=CONV_F2F;  break;  // scales are not the same
        default: return t_convert(CONV_ERROR); // must not compare "specif"
      }
      return t_convert(conv, destspecif.scale-srcspecif.scale);
    case ATT_DATE:
      switch (srctp)
      { case ATT_TIMESTAMP: case ATT_DATIM: return t_convert(CONV_TS2D, srcspecif.with_time_zone, 0);
        case ATT_DATE:      case ATT_INT32: return t_convert(CONV_NO);  
        default:                            return t_convert(CONV_ERROR);
      }
    case ATT_TIME:
      switch (srctp)
      { case ATT_TIMESTAMP: case ATT_DATIM: return t_convert(CONV_TS2T, srcspecif.with_time_zone, destspecif.with_time_zone);
        case ATT_TIME:      
          if (srcspecif.with_time_zone != destspecif.with_time_zone)
            return t_convert(CONV_T2T, srcspecif.with_time_zone, destspecif.with_time_zone);
          // cont.
        case ATT_INT32:                     return t_convert(CONV_NO); // INT is GMT or ZULU - same as desttp
        default:                            return t_convert(CONV_ERROR);
      }
    case ATT_TIMESTAMP:
    case ATT_DATIM:
      switch (srctp)
      { case ATT_DATE:                      return t_convert(CONV_D2TS, 0, destspecif.with_time_zone);
        case ATT_TIME:                      return t_convert(CONV_T2TS, srcspecif.with_time_zone, destspecif.with_time_zone);
        case ATT_TIMESTAMP:  
          if (srcspecif.with_time_zone != destspecif.with_time_zone)
            return t_convert(CONV_TS2TS, srcspecif.with_time_zone, destspecif.with_time_zone);
          // cont.
        case ATT_INT32: // internal use
        case ATT_DATIM:                     return t_convert(CONV_NO);
        default:                            return t_convert(CONV_ERROR);
      }
    case ATT_BINARY:  
      if (srctp==ATT_BINARY || srctp==ATT_RASTER || srctp==ATT_NOSPEC || srctp==ATT_TEXT || IS_EXTLOB_TYPE(srctp)) 
        return t_convert(CONV_VAR2BIN, destspecif.length);
      return t_convert(CONV_ERROR); 
    case ATT_RASTER:  case ATT_NOSPEC:  case ATT_EXTBLOB:
      if (srctp==ATT_BINARY || srctp==ATT_RASTER || srctp==ATT_NOSPEC || srctp==ATT_TEXT || IS_EXTLOB_TYPE(srctp)) 
        return t_convert(CONV_NO);
      return t_convert(CONV_ERROR); 
    case ATT_TEXT:  case ATT_EXTCLOB:
      if (srctp==ATT_NOSPEC || srctp==ATT_EXTBLOB) return t_convert(CONV_NO);  // used when indexing OLE document
      // cont.
    case ATT_STRING:  case ATT_CHAR:    
      if (implicit) return t_convert(CONV_ERROR); 
      switch (srctp)  // from ... to string or Text
      { case ATT_INT16:  case ATT_INT32:  case ATT_INT8:  
                                            return t_convert(CONV_I2S, -srcspecif.scale);
        case ATT_BOOLEAN:                   return t_convert(CONV_BOOL2S);
        case ATT_MONEY:  case ATT_INT64:    return t_convert(CONV_M2S, -srcspecif.scale);
        case ATT_FLOAT:                     return t_convert(CONV_F2S, -srcspecif.scale);
        case ATT_DATE:                      return t_convert(CONV_D2S);
        case ATT_TIME:                      return t_convert(CONV_T2S,  srcspecif.with_time_zone, 0);
        case ATT_TIMESTAMP: case ATT_DATIM: return t_convert(CONV_TS2S, srcspecif.with_time_zone, 0);
        case ATT_BINARY:                    return t_convert(CONV_B2S);
        default:                            return t_convert(CONV_ERROR);
      }
    default: 
      return t_convert(CONV_ERROR);
  }
}

void convert_value(CIp_t CI, t_express * op, int type, t_specif specif)
// Stores to op->convert the conversion from op->type to type.
// Error if no implicit conversion exists.
{ if (op->sqe==SQE_NULL) return;
  t_convert conv = get_type_cast(op->type, type, op->specif, specif, true);
  if (conv.conv==CONV_ERROR) c_error(INCOMPATIBLE_TYPES);
  if (op->convert.conv == CONV_NO) op->convert = conv;  // may have been converted before
  op->type=type;  op->specif=specif;
}

static void convert_to_wide(CIp_t CI, t_express * op)
// Adds to [op] the implicit conversion from narrow to wide character string.
{ if (op->sqe==SQE_NULL) return;
  op->convert=t_convert(CONV_S2S, op->specif.charset, op->specif.charset);
  op->specif.wide_char=1;
  op->specif.length *= 2;
}
  
static void unary_conversion(CIp_t CI, int oper, t_express * op, t_expr_unary * res)
// Defines the opcode for unary operation, the result type and the proper conversion
{ int toper;
  switch (oper)
  { case '-':
      res->type=op->type;  res->specif=op->specif;
      switch (op->type)
      { case ATT_INT16:  case ATT_INT32:  case ATT_INT8:
                                          toper=UNOP_INEG;    break;
        case ATT_MONEY:  case ATT_INT64:  toper=UNOP_MNEG;    break;
        case ATT_FLOAT:                   toper=UNOP_FNEG;    break;
        default:                          c_error(INCOMPATIBLE_TYPES);
      }
      break;
    case S_EXISTS: 
      toper=UNOP_EXISTS;  res->type=ATT_BOOLEAN;  res->specif.set_null();  break;
    case S_UNIQUE: 
      toper=UNOP_UNIQUE;  res->type=ATT_BOOLEAN;  res->specif.set_null();  break;
//    case UNOP_IS_TRUE:  case UNOP_IS_UNKNOWN:  case UNOP_IS_FALSE:
//      toper=oper;         res->type=ATT_BOOLEAN;  res->specif.set_null();  break;
  }
  res->oper=toper;
  res->pr_flags = op->pr_flags | ELFL_NOEDIT;
}

void binary_conversion(CIp_t CI, int oper, t_express * op1, t_express * op2, t_express * res)
// Writes conversions to op1 and op2, which must be applied before oper.
// Writes result type to res->type, res->specif, writes typed operation to res->oper.
// If op2==NULL, writes to op1 the conversion to res.
// Writes the result priviledes.
{ int supertype, restype, puop;  t_compoper toper;  t_specif resspecif;
  BOOL tobool = FALSE, specoper = FALSE;  BOOL prevent_convert_agr1 = FALSE, prevent_convert_agr2 = FALSE;
 // dynamic parameters processing:
  if (op1->sqe==SQE_DYNPAR)
    if (op2->sqe==SQE_DYNPAR) c_error(INCOMPATIBLE_TYPES);
    else
      supply_marker_type(CI, (t_expr_dynpar*)op1, op2->type, op2->specif, MODE_IN);
  else
    if (op2->sqe==SQE_DYNPAR)
      supply_marker_type(CI, (t_expr_dynpar*)op2, op1->type, op1->specif, MODE_IN);
  int type1=op1->type, type2=op2->type;
 // hidden implicit conversions:
  if      (type1==ATT_INT16 || type1==ATT_INT8) type1=ATT_INT32;
  else if (type1==ATT_MONEY) type1=ATT_INT64;
  else if (type1==ATT_DATIM) type1=ATT_TIMESTAMP;
  else if (type1==ATT_PTR || type1==ATT_BIPTR) { type1=ATT_INT32;  prevent_convert_agr1=TRUE; }
  if      (type2==ATT_INT16 || type2==ATT_INT8) type2=ATT_INT32;
  else if (type2==ATT_MONEY) type2=ATT_INT64;
  else if (type2==ATT_DATIM) type2=ATT_TIMESTAMP;
  else if (type2==ATT_PTR || type2==ATT_BIPTR) { type2=ATT_INT32;  prevent_convert_agr2=TRUE; }
  BOOL num1 = type1==ATT_INT32 || type1==ATT_FLOAT || type1==ATT_INT64;
  BOOL num2 = type2==ATT_INT32 || type2==ATT_FLOAT || type2==ATT_INT64;
  supertype=type1;

  if (type1==ATT_BOOLEAN && type2==ATT_BOOLEAN)
  { switch (oper)
    { case SQ_OR:  toper.set_pure(PUOP_OR);  restype = ATT_BOOLEAN;  resspecif.set_null();  goto write_toper;
      case SQ_AND: toper.set_pure(PUOP_AND); restype = ATT_BOOLEAN;  resspecif.set_null();  goto write_toper;
      case '=':         case '<':           case '>':          
      case S_NOT_EQ:    case S_LESS_OR_EQ:  case S_GR_OR_EQ:   // compare booleans like integers
        type1=type2=ATT_INT32;  num1=num2=TRUE;  
        prevent_convert_agr1=prevent_convert_agr2=TRUE;
        break;
      default:    c_error(INCOMPATIBLE_TYPES);
    }
  }

 // analyse the operator:
  switch (oper)
  { case '+':
    case SQ_CONCAT:    puop=PUOP_PLUS;   break;
    case '-':          puop=PUOP_MINUS;  break;
    case '*':          puop=PUOP_TIMES;  break;
    case '/':          puop=PUOP_DIV;    break;
    case SQ_DIV:       puop=PUOP_DIV;    break;
    case SQ_MOD:       puop=PUOP_MOD;    break;
    case '=':          if (op2->sqe==SQE_NULL)  // old compatibility
     { toper.set_pure(PUOP_IS_NULL);  restype=ATT_BOOLEAN;  resspecif.set_null();  goto write_toper; }
                       puop=PUOP_EQ;     tobool=TRUE;  break;
    case '<':          puop=PUOP_LT;     tobool=TRUE;  break;
    case '>':          puop=PUOP_GT;     tobool=TRUE;  break;
    case S_NOT_EQ:     if (op2->sqe==SQE_NULL)  // old compatibility
     { toper.set_pure(PUOP_IS_NULL);  toper.negate=1;  restype=ATT_BOOLEAN;  resspecif.set_null();  goto write_toper; }
                       puop=PUOP_NE;     tobool=TRUE;  break;
    case S_LESS_OR_EQ: puop=PUOP_LE;     tobool=TRUE;  break;
    case S_GR_OR_EQ:   puop=PUOP_GE;     tobool=TRUE;  break;
    case SQ_LIKE:      puop=PUOP_LIKE;   specoper=TRUE;  tobool=TRUE;  break;
    case S_PREFIX:     puop=PUOP_PREF;   specoper=TRUE;  tobool=TRUE;  break;
    case S_SUBSTR:     puop=PUOP_SUBSTR; specoper=TRUE;  tobool=TRUE;  break;
    case S_DDOT:       
      if (!(op1->type & 0x80) || op2->type & 0x80) c_error(INCOMPATIBLE_TYPES);
      if ((op1->type & 0x7f) != op2->type) c_error(INCOMPATIBLE_TYPES);
      restype=ATT_BOOLEAN;  resspecif.set_null();  toper.set_num(PUOP_MULTIN, I_OPBASE);
      goto write_toper;
    case S_BETWEEN:    puop=PUOP_BETWEEN;tobool=TRUE;  break;
    default:           c_error(INCOMPATIBLE_TYPES);
  }
 // analyse operands:
  if (num1 && type2==ATT_BOOLEAN && (oper=='=' || oper==S_NOT_EQ)) 
    { num2=TRUE;  type2=ATT_INT32;  prevent_convert_agr2=TRUE; }  // ODBC uses this (bit=1)
  if (num2 && type1==ATT_BOOLEAN && (oper=='=' || oper==S_NOT_EQ)) 
    { num1=TRUE;  type1=ATT_INT32;  prevent_convert_agr1=TRUE; }  // ODBC uses this (bit=1)
  if (num1 && num2)
  { restype = supertype =
      type1==ATT_FLOAT || type2==ATT_FLOAT || oper=='/' ||
      type1==ATT_INT64 && type2==ATT_INT64 && puop==PUOP_TIMES ? ATT_FLOAT:
      op1->type==ATT_INT64 || op2->type==ATT_INT64 ? ATT_INT64 : 
      op1->type==ATT_MONEY && op2->type==ATT_MONEY ? ATT_MONEY : 
      op1->type==ATT_MONEY && op2->specif.scale!=0 || op2->type==ATT_MONEY && op1->specif.scale!=0 ? ATT_INT64 :  // money type must have scale 2, preventing the risk of obtaining a different scale
      op1->type==ATT_MONEY || op2->type==ATT_MONEY ? ATT_MONEY : ATT_INT32;
   // check the operator applicability:
    if ((oper==SQ_DIV || oper==SQ_MOD) && (op1->specif.scale!=0 || op2->specif.scale!=0 || restype==ATT_FLOAT))
      c_error(INCOMPATIBLE_TYPES);
    if (specoper) c_error(INCOMPATIBLE_TYPES);
   // scale conversion and the resulting scale:
    if (restype==ATT_FLOAT)
    { convert_value(CI, op1, restype, t_specif());  convert_value(CI, op2, restype, t_specif());
      resspecif.set_null();
    }
    else if (puop==PUOP_TIMES) 
    { convert_value(CI, op1, restype, op1->specif);  convert_value(CI, op2, restype, op2->specif);
      resspecif.set_num(op1->specif.scale + op2->specif.scale, op1->specif.precision);
    }
    else if (puop==PUOP_DIV || puop==PUOP_MOD) 
    { convert_value(CI, op1, restype, op1->specif);  convert_value(CI, op2, restype, op2->specif);
      resspecif.set_num(op1->specif.scale - op2->specif.scale, op1->specif.precision);
    }
    else if (op1->specif.scale > op2->specif.scale)
    { resspecif.set_num(op1->specif.scale, op1->specif.precision);
      if (!prevent_convert_agr1) convert_value(CI, op1, restype, resspecif);  
      if (!prevent_convert_agr2) convert_value(CI, op2, restype, resspecif);
    }
    else 
    { resspecif.set_num(op2->specif.scale, op2->specif.precision);
      if (!prevent_convert_agr1) convert_value(CI, op1, restype, resspecif);  
      if (!prevent_convert_agr2) convert_value(CI, op2, restype, resspecif);
    }

    if        (restype==ATT_FLOAT)
      toper.set_num(puop, F_OPBASE);
    else   if (restype==ATT_INT64 || restype==ATT_MONEY)
      toper.set_num(puop, M_OPBASE);
    else //if (restype==ATT_INT32)
      toper.set_num(puop, I_OPBASE);
  } // both numeric types

  else if (is_char_string(type1) && is_char_string(type2)) // char and string operations
  { if (puop==PUOP_MINUS || puop==PUOP_TIMES || puop==PUOP_DIV || puop==PUOP_MOD)
      c_error(INCOMPATIBLE_TYPES);
   // if any is wide, convert to wide:
    BOOL wide_oper;
    if (op1->specif.wide_char) 
    { if (!op2->specif.wide_char) convert_to_wide(CI, op2);
      wide_oper=TRUE;
    }
    else 
      if (!op2->specif.wide_char) wide_oper=FALSE;
      else { convert_to_wide(CI, op1);  wide_oper=TRUE; }
   // find charset+collation:
    int charset;
    if (op1->specif.charset)
    { charset=op1->specif.charset;
      if (!op1->specif.wide_char && !op2->specif.wide_char)
        if (op2->specif.charset && op1->specif.charset!=op2->specif.charset) 
        //if ((op1->specif.charset & 0x7f) != (op2->specif.charset & 0x7f))  // different encoding but the same charset
        //  c_error(INCOMPATIBLE_TYPES);
        //else
        // -- no more, must convert UTF-8 and sometimes other charsets, too
          op2->convert=t_convert(CONV_S2S, op2->specif.charset, op1->specif.charset);  // convert op2 to op1's charset
    }
    else 
      charset=op2->specif.charset;  // specific or NULL
   // BOOL ignore_case:
    BOOL ignore_case=op1->specif.ignore_case || op2->specif.ignore_case;
    toper.set_str(puop, S_OPBASE, charset, wide_oper, ignore_case);
   // result size and type:
    if (puop==PUOP_PLUS) 
    { int len = op1->specif.length+op2->specif.length;  // will be ignored if any is CLOB
      if (type1==ATT_TEXT || type2==ATT_TEXT || len>MAX_FIXED_STRING_LENGTH) 
        { len=SPECIF_VARLEN;  restype=ATT_TEXT; }
      else
        restype=ATT_STRING;
      resspecif.set_string(len, charset, ignore_case, wide_oper);
    } // otherwise restype and resspecif set by [tobool]
  } // character string operations

  else if ((type1==ATT_BINARY || type1==ATT_RASTER || type1==ATT_NOSPEC) &&
           (type2==ATT_BINARY || type2==ATT_RASTER || type2==ATT_NOSPEC))
  { toper.set_num(puop, BI_OPBASE);  
    if (puop==PUOP_PLUS)
    { int len = op1->specif.length+op2->specif.length;  // will be ignored if any is BLOB
      if (type1==ATT_BINARY && type2==ATT_BINARY && len<=MAX_FIXED_STRING_LENGTH)
        restype=ATT_BINARY;
      else
        { restype=ATT_NOSPEC;  len=SPECIF_VARLEN; }
      resspecif.set_string(len, 0, 0, 0);
    }
    else if (puop<PUOP_EQ || puop>PUOP_NE) c_error(INCOMPATIBLE_TYPES);
  } // binary operations

  else 
  { BOOL tmtp1 = type1==ATT_TIME || type1==ATT_DATE || type1==ATT_TIMESTAMP;
    BOOL tmtp2 = type2==ATT_TIME || type2==ATT_DATE || type2==ATT_TIMESTAMP;
    if (tmtp1 && tmtp2 && type1!=type2)  // conversion among various date and time types
    { if (type1==ATT_TIMESTAMP)
      { convert_value(CI, op1, type2, op2->specif);
        restype=type1=type2;
      }
      else
      { convert_value(CI, op2, type1, op1->specif);
        restype=type2=type1;
      }
      // toper will be set below
    }

    if (type1==type2)
    { if (tmtp1)
      { if (puop==PUOP_PLUS || puop==PUOP_TIMES || puop==PUOP_DIV || puop==PUOP_MOD)
          c_error(INCOMPATIBLE_TYPES);
        if (specoper) c_error(INCOMPATIBLE_TYPES);
        if (puop==PUOP_MINUS)
        { restype = ATT_INT32;  resspecif.set_null();
          toper.set_num(PUOP_DIV, type1==ATT_TIME ? TM_OPBASE : type1==ATT_DATE ? DT_OPBASE : TS_OPBASE);
        }
        else // comparing date-time values: must convert to the same WITH-TIME-ZONE state
        { if (type1==ATT_TIME || type1==ATT_TIMESTAMP)
            if (op1->specif.with_time_zone!=op2->specif.with_time_zone)
              convert_value(CI, op1, type2, op2->specif);  // can convert any of the operands
          if (puop==PUOP_EQ || puop==PUOP_NE) toper.set_num(puop, I_OPBASE);
          else                                toper.set_num(puop, U_OPBASE);
        }
      }
      else if (type1==ATT_AUTOR) // cannot compare, return always TRUE
      { if (puop!=PUOP_EQ && puop!=PUOP_NE) c_error(INCOMPATIBLE_TYPES);
        toper.set_num(PUOP_NE, I_OPBASE);
      }
      else c_error(INCOMPATIBLE_TYPES);
    }

    else if (tmtp1 && type2==ATT_INT32)
    { if (puop!=PUOP_PLUS && puop!=PUOP_MINUS) c_error(INCOMPATIBLE_TYPES);
      toper.set_num(puop, type1==ATT_TIME ? TM_OPBASE : type1==ATT_DATE ? DT_OPBASE : TS_OPBASE);
      restype=type1;  resspecif=op1->specif;
    }
    else if (tmtp2 && type1==ATT_INT32)
    { if (puop!=PUOP_PLUS) c_error(INCOMPATIBLE_TYPES);
      toper.set_num(PUOP_TIMES, type2==ATT_TIME ? TM_OPBASE : type2==ATT_DATE ? DT_OPBASE : TS_OPBASE);
      restype=type2;  resspecif=op2->specif;
    }
    else c_error(INCOMPATIBLE_TYPES);
  }
  if (tobool) { restype=ATT_BOOLEAN;  resspecif.set_null(); }

 write_toper:
  if (res->sqe==SQE_BINARY)
  { res->type=restype;  res->specif=resspecif;
    ((t_expr_binary*)res)->oper=toper;
  }
  else if (res->sqe==SQE_TERNARY)
  { res->type=restype;  res->specif=resspecif;
    ((t_expr_ternary*)res)->oper=toper;
  }
  else if (res->sqe==SQE_CASE)
  { res->type=supertype;  res->specif=resspecif;
    ((t_expr_case*)res)->oper=toper;
  }
  res->pr_flags = op1->pr_flags & op2->pr_flags | ELFL_NOEDIT;
}

void add_type(CIp_t CI, t_express * op1, const t_express * op2)
// op1 := common supertype of op1 and op2.
{ int type1 = op1->type, type2 = op2->type;
  if (!type2) return;  // op2 is NULL
  if (!type1)
    { op1->type=type2;  op1->specif=op2->specif; }
  else 
    if (type1==ATT_INT16 || type1==ATT_INT8 || type1==ATT_INT32 || type1==ATT_MONEY || type1==ATT_INT64 || type1==ATT_FLOAT)
    { if (type2!=ATT_INT16 && type2!=ATT_INT8 && type2!=ATT_INT32 && type2!=ATT_MONEY && type2!=ATT_INT64 && type2!=ATT_FLOAT)
        c_error(INCOMPATIBLE_TYPES);
      if (type1==ATT_FLOAT || type2==ATT_FLOAT) { op1->type=ATT_FLOAT;  op1->specif.set_null(); }
      else 
      { if      (type1==ATT_INT64 || type2==ATT_INT64) op1->type=ATT_INT64;  
        else if (type1==ATT_MONEY || type2==ATT_MONEY) op1->type=ATT_MONEY;  
        else if (type1==ATT_INT32 || type2==ATT_INT32) op1->type=ATT_INT32;  
        else if (type1==ATT_INT16 || type2==ATT_INT16) op1->type=ATT_INT16;  
        else                                           op1->type=ATT_INT8;  
        if (op2->specif.scale > op1->specif.scale) op1->specif.scale = op2->specif.scale;
      }
    }
  else
    if (is_char_string(type1)) 
    { if (!is_char_string(type2)) c_error(INCOMPATIBLE_TYPES);
      if      (type1==ATT_TEXT   || type2==ATT_TEXT) 
      { op1->type=ATT_TEXT;  op1->specif.length=SPECIF_VARLEN; }
      else if (type1==ATT_STRING || type2==ATT_STRING) 
      { op1->type=ATT_STRING;  
        if (op2->specif.length > op1->specif.length) op1->specif.length = op2->specif.length;
      }
      else // both chars or NULL and char
      { op1->type=ATT_CHAR;  op1->specif.length=1; }
     // other properties:
      if (op2->specif.wide_char) op1->specif.wide_char=1;
      if (op2->specif.ignore_case) op1->specif.ignore_case=1;
      if (op2->specif.charset && op2->specif.charset!=op1->specif.charset)
        if (!op1->specif.charset) op1->specif.charset=op2->specif.charset;
        else c_error(INCOMPATIBLE_TYPES);
    }
  else if (type1!=type2)
    c_error(INCOMPATIBLE_TYPES);
}

static void query_expression(CIp_t CI, t_query_expr * * qe);

void subquery_expression(CIp_t CI, t_express * * pex, BOOL scalar_req)
// Compiles and optimizes subquery expression without bounding (..).
{ if (CI->sql_kont->phase==SQLPH_INAGGR) c_error(SUBQUERY_IN_AGGR);
  t_expr_subquery * suex=new t_expr_subquery;  if (!suex) c_error(OUT_OF_KERNEL_MEMORY);
  *pex=suex;  
 // save and clear the "referenced" flag in kontexts:
  enum { max_ref_kontexts = 16 };  uns8 upper_referenced[max_ref_kontexts];  
  int cnt;  db_kontext * k;
  for (cnt=0, k = CI->sql_kont->active_kontext_list;  k!=NULL;  cnt++, k=k->next)
  { if (cnt<max_ref_kontexts) upper_referenced[cnt]=k->referenced;
    k->referenced=FALSE;  
  }
  BOOL upper_volatile_var_ref = CI->sql_kont->volatile_var_ref;
  CI->sql_kont->volatile_var_ref=FALSE; 
 // compile the subquery:
  top_query_expression(CI, &suex->qe);
  if (scalar_req)
    if (suex->qe->kont.elems.count()!=2) c_error(NOT_SCALAR_SUBQ);
  suex->type  =suex->qe->kont.elems.acc0(1)->type;
  suex->specif=suex->qe->kont.elems.acc0(1)->specif;
  if (suex->type & 0x80) c_error(THIS_TYPE_NOT_ENABLED);
 // store information about outer references:
  if (cnt) // number of all kontexts
  { suex->kontpos=(t_kontpos*)corealloc(cnt*sizeof(t_kontpos), 56);
    if (suex->kontpos==NULL) c_error(OUT_OF_KERNEL_MEMORY);
    suex->cache_size=0;  // number of referenced kontexts
    for (k = CI->sql_kont->active_kontext_list;  k!=NULL;  k=k->next)
    { if (k->referenced) suex->kontpos[suex->cache_size++].kont=k;  
      if (k->referenced & 2) suex->cachable=false;
    }
  }
#ifdef STOP  // replaced by checking [global_cursor_inst_num]
  if (CI->sql_kont->kont_ta!=NULL) suex->cachable=false; // must not cache when subquery used in the default value or check of a table!
  if (CI->sql_kont->active_kontext_list==NULL) suex->cachable=false; // must not cache when subquery used in an expression outside query!
  if (CI->sql_kont->volatile_var_ref) suex->cachable=false; // must not cache when subquery references parameter or variable value
#endif
 // restore the original global data:
  for (cnt=0, k = CI->sql_kont->active_kontext_list;  k!=NULL;  cnt++, k=k->next)
    if (cnt<max_ref_kontexts) 
      k->referenced|=upper_referenced[cnt];
  CI->sql_kont->volatile_var_ref |= upper_volatile_var_ref;
 // optimize the subquery:
#if SQL3
  suex->opt = optimize_qe(CI->cdp, suex->qe);
  if (suex->opt==NULL) CI->propagate_error();
#else
  suex->opt = alloc_opt(CI->cdp, suex->qe, NULL, false); // ORDER BY from subquery ignored
  if (suex->opt==NULL) c_error(OUT_OF_MEMORY);
  suex->opt->optimize(CI->cdp, FALSE);
#endif
}

BOOL same_expr(const t_express * ex1, const t_express * ex2)
// When comparing column refernce from expression to an index part, the table is not checked.
{ if (ex1==NULL) return ex2==NULL;
  if (ex2==NULL) return FALSE;
  if (ex1->sqe!=ex2->sqe) return FALSE;
  if ((ex1->convert==CONV_NEGATE) != (ex2->convert==CONV_NEGATE))   // CONV_NEGATE in one extression may cause "TRUE==NOT TRUE" without this line
    return FALSE;  //  .. but I must ignore minor conversions like I2I
  switch (ex1->sqe)
  { case SQE_SCONST:
      return ((t_expr_sconst*)ex1)->origtype==((t_expr_sconst*)ex2)->origtype &&
       !memcmp(((t_expr_sconst*)ex1)->val, ((t_expr_sconst*)ex2)->val,
               tpsize[((t_expr_sconst*)ex2)->origtype]);
    case SQE_LCONST:
      return ex1->type==ex2->type && ex1->specif==ex2->specif &&
       !memcmp(((t_expr_lconst*)ex1)->lval, ((t_expr_lconst*)ex2)->lval, ex1->specif.length);
    case SQE_TERNARY:
      return ((t_expr_ternary*)ex1)->oper==((t_expr_ternary*)ex2)->oper &&
        same_expr(((t_expr_ternary*)ex1)->op1, ((t_expr_ternary*)ex2)->op1) &&
        same_expr(((t_expr_ternary*)ex1)->op2, ((t_expr_ternary*)ex2)->op2) &&
        same_expr(((t_expr_ternary*)ex1)->op3, ((t_expr_ternary*)ex2)->op3);
    case SQE_BINARY:
      return ((t_expr_binary*)ex1)->oper==((t_expr_binary*)ex2)->oper && (
        same_expr(((t_expr_binary*)ex1)->op1, ((t_expr_binary*)ex2)->op1) &&
        same_expr(((t_expr_binary*)ex1)->op2, ((t_expr_binary*)ex2)->op2) ||
         (((t_expr_binary*)ex1)->oper.pure_oper==PUOP_PLUS ||
          ((t_expr_binary*)ex1)->oper.pure_oper==PUOP_TIMES) &&
        same_expr(((t_expr_binary*)ex1)->op1, ((t_expr_binary*)ex2)->op2) &&
        same_expr(((t_expr_binary*)ex2)->op1, ((t_expr_binary*)ex1)->op2) );
    case SQE_UNARY:
      return ((t_expr_unary*)ex1)->oper==((t_expr_unary*)ex2)->oper &&
        same_expr(((t_expr_unary*)ex1)->op, ((t_expr_unary*)ex2)->op);
    case SQE_FUNCT:
      return ((t_expr_funct*)ex1)->num==((t_expr_funct*)ex2)->num &&
             same_expr(((t_expr_funct*)ex1)->arg1, ((t_expr_funct*)ex2)->arg1) &&
             same_expr(((t_expr_funct*)ex1)->arg2, ((t_expr_funct*)ex2)->arg2) &&
             same_expr(((t_expr_funct*)ex1)->arg3, ((t_expr_funct*)ex2)->arg3) &&
             same_expr(((t_expr_funct*)ex1)->arg4, ((t_expr_funct*)ex2)->arg4) &&
             same_expr(((t_expr_funct*)ex1)->arg5, ((t_expr_funct*)ex2)->arg5) &&
             same_expr(((t_expr_funct*)ex1)->arg6, ((t_expr_funct*)ex2)->arg6) &&
             same_expr(((t_expr_funct*)ex1)->arg7, ((t_expr_funct*)ex2)->arg7) &&
             same_expr(((t_expr_funct*)ex1)->arg8, ((t_expr_funct*)ex2)->arg8);
    case SQE_ATTR:  // must find the original attribute of a table
    case SQE_ATTR_REF:
    { if (!same_expr(((t_expr_attr*)ex1)->indexexpr, ((t_expr_attr*)ex2)->indexexpr))
        return FALSE;
      db_kontext * akont1 = ((t_expr_attr*)ex1)->kont;
      db_kontext * akont2 = ((t_expr_attr*)ex2)->kont;
      int          elnum1 = ((t_expr_attr*)ex1)->elemnum;
      int          elnum2 = ((t_expr_attr*)ex2)->elemnum;
      if (akont1!=NULL)
        if (akont2!=NULL) return akont1==akont2 && elnum1==elnum2;
        else // comparing expression to index, akont2==NULL, convert elnum1 to its table kontext
        { 
#if SQL3
          while (akont1->t_mat==NULL)
#else
          while (akont1->mat==NULL)
#endif
          { elem_descr * el = akont1->elems.acc0(elnum1);
            if (el->link_type==ELK_KONTEXT)
            { akont1=el->kontext.kont;
              elnum1=el->kontext.down_elemnum;
            }
            else if (el->link_type==ELK_KONT2U || el->link_type==ELK_KONT21)
              if (akont1->position)  // this is wrong, must compare both!!!
              { akont1=el->kontdbl.kont2;
                elnum1=el->kontdbl.down_elemnum2;
              }
              else
              { akont1=el->kontdbl.kont1;
                elnum1=el->kontdbl.down_elemnum1;
              }
            else return same_expr(el->expr.eval, ex2);
          }
          return elnum1==elnum2;
        }
      else // no kontexts, comparing indicies, must use column names
        return !strcmp(((t_expr_attr*)ex1)->name, ((t_expr_attr*)ex2)->name);
    }
    case SQE_POINTED_ATTR:
      return ((t_expr_pointed_attr*)ex1)->elemnum==((t_expr_pointed_attr*)ex2)->elemnum &&
             ((t_expr_pointed_attr*)ex1)->desttb ==((t_expr_pointed_attr*)ex2)->desttb  &&
             same_expr(((t_expr_pointed_attr*)ex1)->pointer_attr, ((t_expr_pointed_attr*)ex2)->pointer_attr);
    case SQE_CASE:
      return ((t_expr_case*)ex1)->oper     ==((t_expr_case*)ex2)->oper &&
             ((t_expr_case*)ex1)->case_type==((t_expr_case*)ex2)->case_type &&
             same_expr(((t_expr_case*)ex1)->val1,   ((t_expr_case*)ex2)->val1) &&
             same_expr(((t_expr_case*)ex1)->val2,   ((t_expr_case*)ex2)->val2) &&
             same_expr(((t_expr_case*)ex1)->contin, ((t_expr_case*)ex2)->contin);
    case SQE_NULL:  case SQE_CURRREC:
      return TRUE;
    case SQE_SEQ:
      return ((t_expr_seq*)ex1)->seq_obj==((t_expr_seq*)ex2)->seq_obj && ((t_expr_seq*)ex1)->nextval==((t_expr_seq*)ex2)->nextval;
    case SQE_MULT_COUNT:
      return same_expr(((t_expr_mult_count*)ex1)->attrex, ((t_expr_mult_count*)ex2)->attrex);
    case SQE_VAR_LEN:
      return same_expr(((t_expr_var_len*)ex1)->attrex, ((t_expr_var_len*)ex2)->attrex);
    case SQE_DBLIND:
      return same_expr(((t_expr_dblind*)ex1)->attrex, ((t_expr_dblind*)ex2)->attrex) &&
             same_expr(((t_expr_dblind*)ex1)->start,  ((t_expr_dblind*)ex2)->start ) &&
             same_expr(((t_expr_dblind*)ex1)->size,   ((t_expr_dblind*)ex2)->size  );
    case SQE_CAST:
      return ((t_expr_cast*)ex1)->arg->convert==((t_expr_cast*)ex2)->arg->convert &&
             same_expr(((t_expr_cast*)ex1)->arg, ((t_expr_cast*)ex2)->arg);
    default: return FALSE;  // subquery
  }
}

static int get_agr_index(CIp_t CI, int ag_type, BOOL distinct, t_express * arg)
// Searches the aggregate function in the list, returns index.
// Addes the function and return index if not found.
{ int agnum;  t_agr_reg * agg;
  for (agnum=0;  agnum<CI->sql_kont->agr_list->count();  agnum++)
  { agg=CI->sql_kont->agr_list->acc0(agnum);
    if (agg->ag_type==ag_type)
      if (agg->distinct==distinct)
        if (same_expr(agg->arg, arg))
          return agnum;  // found previous occurence of the same aggregate
  }
 // add new aggregate(s), if not found (agnum valid anyway):
  agg=CI->sql_kont->agr_list->next();
  if (agg == NULL) c_error(OUT_OF_MEMORY);
  agg->ag_type =ag_type;
  agg->distinct=distinct;
  agg->arg     =arg;
 // store the argument size info:
  agg->argsize = arg==NULL ? tpsize[ATT_INT32] :
                 SPECIFLEN(arg->type) ? arg->specif.length : tpsize[arg->type];
  if (ag_type==S_COUNT || ag_type==S_SUM && (arg->type==ATT_INT16 || arg->type==ATT_INT8))
  { agg->restype = ATT_INT32;
    agg->ressize = tpsize[ATT_INT32];
  }
  else
  { agg->ressize = agg->argsize;
    agg->restype = arg->type;
  }
 // compare with indicies:
  agg->index_number=-1;
  if (CI->sql_kont->min_max_optimisation_enabled && (ag_type==S_MIN || ag_type==S_MAX))
    if (arg->sqe==SQE_ATTR) // argument is a column
    { t_expr_attr * atex = (t_expr_attr *)arg;
#if SQL3
      if (atex->kont->t_mat)
      { t_mater_table * tmat = atex->kont->t_mat;
#else
      if (atex->kont->mat && !atex->kont->mat->is_ptrs)
      { t_mater_table * tmat = (t_mater_table*)atex->kont->mat;
#endif
        if (tmat->permanent && tmat->locdecl_offset==-1)  // argument is a column of a permanent table
        { table_descr_auto td(CI->cdp, tmat->tabnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
          if (!td->me()) c_error(CI->cdp->get_return_code());
         // look for index:
          int ind;  dd_index * cc;
          for (ind=0, cc=td->indxs;  ind<td->indx_cnt;  ind++, cc++)
            if (!cc->disabled)
              if (cc->has_nulls || ag_type==S_MAX)  // cannot use index not containing NULL values for the MIN function
                if (same_expr(arg, cc->parts[0].expr))  // same_expr je pro to uzpusobeno
                { agg->index_number=ind;
                  agg->first_value = (ag_type==S_MIN)==!cc->parts[0].desc;
                  break;
                }
        }
      }
    }
  return agnum;
}

#define STRCOPY_NUM  126  // copy from wcompil.c
#define STRTRIM_NUM  134

void call_gen(CIp_t CI, sprocobj * sb, t_expr_funct * fex)  
// Compiles the call of the standard procedure or function sb, value parameters only
// Can pass a long string as paramater and return it
{ BOOL ispar;
  int sprocnum = sb->sprocnum;
  int specifsum = 0;  // sum of max lengths of parameters
  bool has_log_string_param = false;  t_specif char_param_specif;
  if (CI->cursym=='(')
    { ispar=TRUE;  next_sym(CI); }
  else ispar=FALSE;

  for (int param_number=0;  sb->pars[param_number].categ;  param_number++)
  {// check for ( or , before the parameter: 
    if (!param_number) 
    { if (!ispar) c_error(LEFT_PARENT_EXPECTED);
    }
    else test_and_next(CI,',', COMMA_EXPECTED);
   // evaluate and convert the next parameter:
    int tp = sb->pars[param_number].dtype;
    sql_expression(CI, &fex->arg1+param_number);
    t_express * arg = (&fex->arg1)[param_number];
    if (is_char_string(tp))
    { if (is_string(tp)) specifsum += arg->specif.length;
      else if (tp==ATT_CHAR) specifsum+=1;
      else has_log_string_param=true;  // CLOB
      char_param_specif=arg->specif;
    }
    t_specif dest_specif;  // the default specif for the parameter
    dest_specif.length=IS_HEAP_TYPE(tp) ? SPECIF_VARLEN : SPECIFLEN(tp) ? 0x7ffffff : tp==ATT_CHAR ? 1 : tp==ATT_MONEY ? 2 : 0;
    if (sprocnum == FCNUM_LETTERCREW)
    {  if (param_number<=1)   
        dest_specif.wide_char = 1;
    }
    else if (sprocnum == FCNUM_LETTERADDADRW)
    {  if (param_number==1 || param_number==2)   
        dest_specif.wide_char = 1;
    }
    else if (sprocnum == FCNUM_LETTERADDFILEW)
    {  if (param_number==1)   
        dest_specif.wide_char = 1;
    }
    else if (sprocnum == FCNUM_MLADDBLOBRW)
    {  if (param_number==1)   
        dest_specif.wide_char = 1;
    }
    else if (sprocnum == FCNUM_MESSAGE_TO_CLI)
    {  if (param_number==0)   
        dest_specif.charset = CHARSET_NUM_UTF8;
    }
    else if (sprocnum == FCNUM_MLADDBLOBSW)
    {  if (param_number>=1)   
        dest_specif.wide_char = 1;
    }
    if (sprocnum==FCNUM_SCHEMA_ID || sprocnum==FCNUM_TRUCT_TABLE || sprocnum==150 ||  // free_deleted
        sprocnum==149 || sprocnum==FCNUM_LOGWRITEEX && param_number<=1)  // Log_write
      dest_specif.charset=sys_spec.charset;  // allows proper conversion from Unicode etc.
    if (arg->sqe==SQE_DYNPAR)
      supply_marker_type(CI, (t_expr_dynpar*)arg, tp, dest_specif, MODE_IN);
    else
      convert_value(CI, arg, tp, dest_specif);
    // big value for strings prevents trimming of long string argument values
    fex->pr_flags &= arg->pr_flags;
  }
  fex->pr_flags |= ELFL_NOEDIT;

  if (ispar)
    if (CI->cursym==',') c_error(TOO_MANY_PARAMS);
    else test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);

  if (sprocnum==STRCOPY_NUM || sprocnum==STRTRIM_NUM || sprocnum==125 /*strdelete*/ || // preserve string collation and max length
      sprocnum==109 /*strcat*/ || sprocnum==124 /* strinsert*/)                        // preserve string collation, max length is sum
  { fex->specif=char_param_specif;
    if (has_log_string_param || specifsum>MAX_FIXED_STRING_LENGTH)
      { fex->origtype = ATT_TEXT;    fex->specif.length = SPECIF_VARLEN; }
    else 
      { fex->origtype = ATT_STRING;  fex->specif.length = specifsum; }
  }
  else // fixed return type, defined max length
  { fex->origtype=sb->retvaltype;
    fex->specif.set_null();
    if (sprocnum==115 || sprocnum==116 || sprocnum==117 || sprocnum==132 || sprocnum==FCNUM_BIGINT2STR)
      fex->specif.length=30;           // allows using 'aaa'||date2str() in index
    else if (sprocnum==131 || sprocnum==154)
    { fex->specif.length=40;           // allows using 'aaa'||date2str() in index
      fex->specif.charset=sys_spec.charset;  // allows proper conversion into Unicode etc.
    }
    else if (sprocnum==198 || sprocnum==FCNUM_CURR_SCHEMA || sprocnum==FCNUM_LOCAL_SCHEMA) // current_appl, user_name, local_schema_name
    { fex->specif.length=OBJ_NAME_LEN; // allows using 'aaa'|| current_appl() in index
      fex->specif.charset=sys_spec.charset;  // allows proper conversion into Unicode etc.
    }
    else if (sprocnum==FCNUM_ACTIVE_ROUT) 
    { fex->specif.length=5+2*OBJ_NAME_LEN; 
      fex->specif.charset=sys_spec.charset;  // allows proper conversion into Unicode etc.
    }
    else if (sprocnum==FCNUM_GET_FULLTEXT_CONTEXT) 
      fex->specif.charset=sys_spec.charset;  // allows proper conversion into Unicode etc.
    else if (fex->origtype==ATT_MONEY)
      fex->specif.scale=2;
    else if (sprocnum==FCNUM_SCHEMA_ID)
      fex->specif.length=UUID_SIZE;
    //else if (sprocnum==FCNUM_EXTLOB_FILE)
    //  fex->specif.length=256;
  }
  fex->origspecif = fex->specif;
  fex->type = fex->origtype;
}

void special_function(CIp_t CI, t_expr_funct * fex)
// Compiles the function with special syntax and strange parameters.
// Function number is in fex->num.
{ int tp;
  test_and_next(CI, '(',  LEFT_PARENT_EXPECTED);
  switch (fex->num)
  { case FCNUM_POSITION:
    { sql_expression(CI, &fex->arg1);
      if (!is_char_string(fex->arg1->type)) c_error(INCOMPATIBLE_TYPES);
      test_and_next(CI, SQ_IN, IN_EXPECTED);
      sql_expression(CI, &fex->arg2);
      if (!is_char_string(fex->arg2->type)) c_error(INCOMPATIBLE_TYPES);
     // convert the pattern string to the charset of containing string:
      int orig_ignore_case = fex->arg1->specif.ignore_case;
      convert_value(CI, fex->arg1, ATT_TEXT, fex->arg2->specif);
      fex->arg1->specif.ignore_case = orig_ignore_case;  // preserve
      break;
    }
    case FCNUM_CHARLEN:
      sql_expression(CI, &fex->arg1);  tp=fex->arg1->type;
      if (!is_string(tp) && tp!=ATT_CHAR && tp!=ATT_TEXT && tp!=ATT_EXTCLOB) 
        c_error(INCOMPATIBLE_TYPES);
      break;
    case FCNUM_BITLEN:
      sql_expression(CI, &fex->arg1);  tp=fex->arg1->type;
      if (!is_string(tp) && tp!=ATT_CHAR && tp!=ATT_BINARY && tp!=ATT_BOOLEAN && !IS_HEAP_TYPE(tp) && !IS_EXTLOB_TYPE(tp))
        c_error(INCOMPATIBLE_TYPES);
      break;
    case FCNUM_OCTETLEN:
      sql_expression(CI, &fex->arg1);  tp=fex->arg1->type;
      if (!is_string(tp) && tp!=ATT_CHAR && tp!=ATT_BINARY && !IS_HEAP_TYPE(tp) && !IS_EXTLOB_TYPE(tp))
        c_error(INCOMPATIBLE_TYPES);
      break;
    case FCNUM_EXTRACT:
      if (CI->cursym!=S_IDENT) c_error(DATETIME_FIELD_EXPECTED);
      if (!strcmp(CI->curr_name, "YEAR"))   fex->num=FCNUM_EXTRACT_YEAR;  else
      if (!strcmp(CI->curr_name, "MONTH"))  fex->num=FCNUM_EXTRACT_MONTH; else
      if (!strcmp(CI->curr_name, "DAY"))    fex->num=FCNUM_EXTRACT_DAY;   else
      if (!strcmp(CI->curr_name, "HOUR"))   fex->num=FCNUM_EXTRACT_HOUR;  else
      if (!strcmp(CI->curr_name, "MINUTE")) fex->num=FCNUM_EXTRACT_MINUTE; else
      if (!strcmp(CI->curr_name, "SECOND"))
        { fex->type=fex->origtype=ATT_FLOAT;  fex->num=FCNUM_EXTRACT_SECOND; } else
        c_error(DATETIME_FIELD_EXPECTED);
      next_sym(CI);
      test_and_next(CI, S_FROM, FROM_OMITTED);
      sql_expression(CI, &fex->arg1);  tp=fex->arg1->type;
      if (tp!=ATT_DATE && tp!=ATT_TIME && tp!=ATT_TIMESTAMP && tp!=ATT_DATIM)
        c_error(INCOMPATIBLE_TYPES);
      break;
    case FCNUM_SUBSTRING:
      sql_expression(CI, &fex->arg1);  tp=fex->arg1->type;
      if (tp==ATT_CHAR) tp=ATT_STRING;
      else if (tp!=ATT_STRING && tp!=ATT_BINARY && !IS_HEAP_TYPE(tp) && !IS_EXTLOB_TYPE(tp)) c_error(INCOMPATIBLE_TYPES);
      fex->type  =fex->origtype  =tp;          
      fex->specif=fex->origspecif=fex->arg1->specif;  // all characteristics preserved
      test_and_next(CI, S_FROM, FROM_OMITTED);
      sql_expression(CI, &fex->arg2);
      if (!is_integer(fex->arg2->type, fex->arg2->specif)) c_error(MUST_BE_INTEGER);
      if (CI->cursym==SQ_FOR)
      { next_sym(CI);
        sql_expression(CI, &fex->arg3);
        if (!is_integer(fex->arg3->type, fex->arg3->specif)) c_error(MUST_BE_INTEGER);
        if (fex->arg3->sqe==SQE_SCONST)
        { sig32 ival = *(sig32*)((t_expr_sconst*)fex->arg3)->val;
          if (ival>0 && ival<=MAX_FIXED_STRING_LENGTH)
            if ((fex->type==ATT_STRING || fex->type==ATT_BINARY) && ival<fex->specif.length)
            { if (fex->type==ATT_STRING && fex->specif.wide_char) ival*=2;
              fex->specif.length=fex->origspecif.length=(uns16)ival;
            }
            else if (fex->type==ATT_TEXT || fex->type==ATT_EXTCLOB)
            { if (fex->specif.wide_char) ival*=2;
              fex->specif.length=fex->origspecif.length=(uns16)ival;
              fex->type  =fex->origtype  =ATT_STRING;
            }
            else if (IS_HEAP_TYPE(fex->type) || fex->type==ATT_EXTBLOB)
            { fex->specif.length=fex->origspecif.length=(uns16)ival;
              fex->type  =fex->origtype  =ATT_BINARY;
            }
        }
      }
      break;
    case FCNUM_LOWER:
    case FCNUM_UPPER:
      sql_expression(CI, &fex->arg1);
      if (fex->arg1->sqe==SQE_DYNPAR)
        supply_marker_type(CI, (t_expr_dynpar*)fex->arg1, ATT_TEXT, t_specif(SPECIF_VARLEN), MODE_IN);
      else
        if (!is_char_string(fex->arg1->type)) c_error(INCOMPATIBLE_TYPES);
      fex->type  =fex->origtype  =fex->arg1->type;  
      fex->specif=fex->origspecif=fex->arg1->specif;  // preserves type, collation and max. lenght
      break;
    case FCNUM_FULLTEXT:
      sql_expression(CI, &fex->arg1);  
      if (!is_string(fex->arg1->type) && fex->arg1->type!=ATT_CHAR) c_error(INCOMPATIBLE_TYPES);
      test_and_next(CI, ',', COMMA_EXPECTED);
      sql_expression(CI, &fex->arg2);  
      convert_value(CI, fex->arg2, ATT_INT64, t_specif());
      if (fex->arg2->sqe!=SQE_ATTR) c_error(INCOMPATIBLE_TYPES);
      test_and_next(CI, ',', COMMA_EXPECTED);
      sql_expression(CI, &fex->arg3);  
      if (!is_string(fex->arg3->type) && fex->arg3->type!=ATT_CHAR) 
        if (fex->arg3->sqe==SQE_DYNPAR)
          supply_marker_type(CI, (t_expr_dynpar*)fex->arg3, ATT_TEXT, t_specif(0,0,0,1), MODE_IN);  // long and wide, is universal
        else c_error(INCOMPATIBLE_TYPES);
      break;
    case FCNUM_SELPRIV:  case FCNUM_UPDPRIV:
     // analyse column name:
      sql_expression(CI, &fex->arg1);  
      if (fex->arg1->sqe!=SQE_ATTR) { SET_ERR_MSG(CI->cdp, "[Param 1]");  c_error(UNDEF_ATTRIBUTE); }
      *(t_expr_types*)&fex->arg1->sqe=SQE_ATTR_REF;  
      fex->arg1->type=ATT_INT32;  fex->arg1->specif.set_null();
     // info type:
      test_and_next(CI, ',', COMMA_EXPECTED);
      sql_expression(CI, &fex->arg2);
      if (!is_integer(fex->arg2->type, fex->arg2->specif)) c_error(MUST_BE_INTEGER);
     // subject name and category:
      if (CI->cursym==',')
      { next_sym(CI);
        sql_expression(CI, &fex->arg3);
        if (!is_char_string(fex->arg3->type)) c_error(INCOMPATIBLE_TYPES);
        test_and_next(CI, ',', COMMA_EXPECTED);
        sql_expression(CI, &fex->arg4);
        if (!is_integer(fex->arg4->type, fex->arg4->specif)) c_error(MUST_BE_INTEGER);
      }
      break;
    case FCNUM_DELPRIV:  case FCNUM_INSPRIV:  case FCNUM_GRTPRIV:
      fex->arg1=new t_expr_attr(1, CI->sql_kont->active_kontext_list);  if (!fex->arg1) c_error(OUT_OF_KERNEL_MEMORY);
      sql_expression(CI, &fex->arg2);
      if (!is_integer(fex->arg2->type, fex->arg2->specif)) c_error(MUST_BE_INTEGER);
     // subject name and category:
      if (CI->cursym==',')
      { next_sym(CI);
        sql_expression(CI, &fex->arg3);
        if (!is_char_string(fex->arg3->type)) c_error(INCOMPATIBLE_TYPES);
        test_and_next(CI, ',', COMMA_EXPECTED);
        sql_expression(CI, &fex->arg4);
        if (!is_integer(fex->arg4->type, fex->arg4->specif)) c_error(MUST_BE_INTEGER);
      }
      break;
    case FCNUM_ENUMRECPR:  case FCNUM_ENUMTABPR:  
      if (!CI->sql_kont->active_kontext_list) c_error(GENERAL_SQL_SYNTAX);
      fex->arg1=new t_expr_attr(1, CI->sql_kont->active_kontext_list);  if (!fex->arg1) c_error(OUT_OF_KERNEL_MEMORY);
      sql_expression(CI, &fex->arg2);
      if (!is_integer(fex->arg2->type, fex->arg2->specif)) c_error(MUST_BE_INTEGER);
      fex->specif.length=UUID_SIZE;
      break;
    case FCNUM_ADMMODE:  // procedure name(s) compiled into arg1 and arg2 as string(s)
      if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
      fex->arg1=new t_expr_lconst(ATT_STRING, CI->curr_name, (int)strlen(CI->curr_name), sys_spec.charset);  
      if (!fex->arg1) c_error(OUT_OF_KERNEL_MEMORY);
      if (next_sym(CI)=='.')
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        fex->arg2=new t_expr_lconst(ATT_STRING, CI->curr_name, (int)strlen(CI->curr_name), sys_spec.charset);  
        if (!fex->arg2) c_error(OUT_OF_KERNEL_MEMORY);
        next_sym(CI);
      }
      else
      { fex->arg2=fex->arg1;
        fex->arg1=new t_expr_null;  if (!fex->arg1) c_error(OUT_OF_KERNEL_MEMORY);
      }
      test_and_next(CI, ',', COMMA_EXPECTED);
      sql_expression(CI, &fex->arg3);  
      if (!is_integer(fex->arg3->type, fex->arg3->specif)) c_error(MUST_BE_INTEGER);
      test_and_next(CI, ',', COMMA_EXPECTED);
      sql_expression(CI, &fex->arg4);  
      if (fex->arg4->type!=ATT_BOOLEAN && fex->arg4->sqe!=SQE_NULL) c_error(INCOMPATIBLE_TYPES);
      break;
    case FCNUM_CONSTRAINS_OK:
    { if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
      sql_selector(CI, &fex->arg1, true, false);
      if (fex->arg1->sqe!=SQE_ATTR) c_error(INCOMPATIBLE_TYPES);
      t_expr_attr * atex = (t_expr_attr*)fex->arg1;
      db_kontext * _kont;  int _elemnum;
      if (!resolve_access(atex->kont, atex->elemnum, _kont, _elemnum))
        c_error(INCOMPATIBLE_TYPES);
      fex->specif=fex->origspecif=t_specif(OBJ_NAME_LEN, 0, 0, 0);
      break;
    }
#ifdef STOP
    case FCNUM_COMP_TAB:
    { if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
      int level;
      t_locdecl * locdecl = find_name_in_scopes(CI, CI->curr_name, level, LC_CURSOR);
      if (locdecl == NULL) c_error(IDENT_NOT_DEFINED);
      new t_expr_var(locdecl->var.type, locdecl->var.specif, SYSTEM_VAR_LEVEL, locdecl->var.offset);
      sql_selector(CI, &fex->arg1, true, false);
      test_and_next(CI, ',', COMMA_EXPECTED);
      break;
    }
#endif
    case FCNUM_EXTLOB_FILE:
      sql_expression(CI, &fex->arg1);  
      if (fex->arg1->sqe!=SQE_ATTR) c_error(INCOMPATIBLE_TYPES);
      if (fex->arg1->type!=ATT_EXTBLOB && fex->arg1->type!=ATT_EXTCLOB) c_error(INCOMPATIBLE_TYPES);
      fex->specif.length=fex->origspecif.length=256;
      break;
  }
  test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
}

bool t_expr_funct_regex::compile_pattern(cdp_t cdp, const char * patt, const char * options)
{ int pcre_options = 0;
  while (*options)
  { if      (*options=='i') pcre_options|=PCRE_CASELESS;
    else if (*options=='n') pcre_options|=PCRE_DOTALL;
    else if (*options=='m') pcre_options|=PCRE_MULTILINE;
    options++;
  }
 // if the same pattern has already ben compiled, do nothig: 
  if (compiled_pattern && !strcmp(patt, compiled_pattern) && compiled_options == pcre_options)
    return code!=NULL;  // code may be NULL if pattern could not be compiled
 // release the original compilation:
  corefree(compiled_pattern);  
  if (code) { /*(*pcre_*/free(code);  code=NULL; }
 // make copy of the new pattern (if the memory for the copy cannot be allocate the caching will not work but the regex will):
  compiled_pattern = (char*)corealloc(strlen(patt)+1, 99);
  if (compiled_pattern) strcpy(compiled_pattern, patt);
  compiled_options = pcre_options;
 // compile:
  if (arg2->specif.charset==CHARSET_NUM_UTF8)  // converted from Unicode
    pcre_options |= PCRE_UTF8;
  const char * errptr;  int erroroffset;
#if WBVERS>=950
  code = pcre_compile(patt, pcre_options, &errptr, &erroroffset, wbcharset_t(arg2->specif.charset).charset_data()->pcre_table);
#else
  code=NULL;  erroroffset=0;  errptr="Not implemented";
#endif
  if (!code)  // compilation error
  { char msg[256];
    snprintf(msg, sizeof(msg), "Regex pattern error in position %d: %s", erroroffset, errptr);
    msg[sizeof(msg)-1]=0;  // on overflow the terminator is NOT written!
    request_generic_error(cdp, GENERR_REGEX_PATTERN, msg);
    return false;
  }
  return true;
}

void compile_regexpr_call(CIp_t CI, t_expr_funct_regex * fex)
{ 
  test_and_next(CI, '(',  LEFT_PARENT_EXPECTED);
  sql_expression(CI, &fex->arg1);
  if (!is_char_string(fex->arg1->type)) c_error(INCOMPATIBLE_TYPES);
  test_and_next(CI, ',', COMMA_EXPECTED);
  sql_expression(CI, &fex->arg2);
  if (!is_char_string(fex->arg2->type)) c_error(INCOMPATIBLE_TYPES);
 // options:
  if (CI->cursym==',')
  { next_sym(CI);
    sql_expression(CI, &fex->arg3);
    if (fex->arg3->type!=ATT_STRING && fex->arg3->type!=ATT_CHAR) c_error(INCOMPATIBLE_TYPES);
  }
 // convert wide character text to UTF-8:
  if (fex->arg1->specif.wide_char)
    convert_value(CI, fex->arg1, ATT_TEXT, t_specif(0,CHARSET_NUM_UTF8,0,0));
 // convert the pattern string to the charset of containing string:
  convert_value(CI, fex->arg2, ATT_STRING, t_specif(0x7ffe,fex->arg1->specif.charset,0,0));  // string must not have length 0
 // convert wide character text to UTF-8:
  if (fex->arg3 && fex->arg3->specif.wide_char)
    convert_value(CI, fex->arg3, ATT_STRING, t_specif(50,CHARSET_NUM_UTF8,0,0));
  test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
}

void t_routine::extobj2routine(CIp_t CI, extension_object * exobj, HINSTANCE hInst)
{ extension_routine=true;
  rettype   = exobj->retvaltype;
  retspecif = exobj->retvalspecif;
  for (int param_number=0;  exobj->pars[param_number].categ;  param_number++)
  { char parname[10];  int2str(param_number, parname);
    ext_parsm * par = &exobj->pars[param_number];
    scope.store_pos();
    scope.add_name(CI, parname, par->categ==O_VALPAR ? LC_INPAR : par->categ==O_FLEXPAR ? LC_FLEX : LC_INOUTPAR);
    scope.store_type(par->dtype, t_specif(par->specif), FALSE, NULL);
  }
  compute_external_frame_size();
  procptr=GetProcAddress(hInst, exobj->expname);
}

void compile_ext_object(CIp_t CI, sql_stmt_call ** pres, extension_object * obj, bool value_required, HINSTANCE hInst)
{
#ifdef STOP
  switch (obj->any.categ)
  { default:
      c_error(BAD_IDENT_CLASS);  // SET_ERR_MSG(CI->cdp, name);  is in the caller
    case OBT_SPROC:
#endif
    { t_routine * rout = new t_routine(0, NOOBJECT);
      if (!rout) c_error(OUT_OF_KERNEL_MEMORY);
      rout->extobj2routine(CI, obj, hInst);
     // make routine from obj
      sql_stmt_call * so = new sql_stmt_call();  *pres = so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
      so->source_line = CI->prev_line;
      so->routine_invocation(CI, rout, ROUT_EXTENSION);
      if (value_required && !so->origtype)  // procedure in expression
        c_error(BAD_IDENT_CLASS);  // SET_ERR_MSG(CI->cdp, name);  is in the caller 
//      break;
    }
}

void sql_selector(CIp_t CI, t_express ** pres, bool value_sel, bool limit_db_kontext)
// Target compilation iff [value_sel]==false.
// Using database kontext for prefixed names only when [limit_db_kontext]==true.
{ tobjname name;  elem_descr * el;
  t_expr_attr * ex;  int elemnum;  db_kontext * kont;
  if (CI->cursym=='?')
  { if (CI->sql_kont->in_procedure_compilation!=0) c_error(EMB_VAR_OR_DP_IN_PROC);
    t_expr_dynpar * ex = new t_expr_dynpar;  *pres=ex;  if (!ex) c_error(OUT_OF_KERNEL_MEMORY);
    ex->num=-1;  // marker not found!
    if (CI->cdp->sel_stmt!=NULL)
      for (int i=0;  i<CI->cdp->sel_stmt->markers.count();  i++)
      { MARKER * marker = CI->cdp->sel_stmt->markers.acc0(i);
        if (marker->position+CI->cdp->sel_stmt->disp == CI->compil_ptr-CI->univ_source-2*CI->charsize ||
            marker->position+CI->cdp->sel_stmt->disp == CI->compil_ptr-CI->univ_source-CI->charsize) // when '?' is the last source char
          { ex->num=i;  break; }
      }
    if (ex->num==-1) c_error(INTERNAL_COMP_ERROR);
    ex->type=ex->origtype=0;  // type not defined so far, nor is specif
    CI->sql_kont->volatile_var_ref=TRUE;
    CI->sql_kont->dynpar_referenced=TRUE;
    next_sym(CI);
    return;
  }
  if (CI->cursym==':')
  { if (CI->sql_kont->in_procedure_compilation!=0) c_error(EMB_VAR_OR_DP_IN_PROC);
    t_expr_embed * ex = new t_expr_embed;  *pres=ex;  if (!ex) c_error(OUT_OF_KERNEL_MEMORY);
    do next_sym(CI);
    while (CI->cursym == '<' || CI->cursym == '>' || CI->cursym==S_NOT_EQ);
    if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
    if (CI->cdp->sel_stmt!=NULL)  // any statement params sent
    { t_emparam * empar = CI->cdp->sel_stmt->find_eparam_by_name(CI->curr_name);
      if (empar!=NULL)
      { strcpy(ex->name, empar->name);  // same lenght: tname
        ex->type=empar->type;
        ex->specif=empar->specif;
        if (ex->specif.opqval==0)  // may be a client error (PHP client uses this), trying to repair:
          if      (ex->type==ATT_STRING) ex->specif.length = empar->length/*-1*/; // length is buflen from t_clivar - it may or may not include the terminator, but it is better to specify bigger specif.length because it prevents truncation errors when converting the actual value!
          else if (ex->type==ATT_CHAR)   ex->specif.length = 1;
          else if (ex->type==ATT_BINARY) ex->specif.length = empar->length;
          else if (ex->type==ATT_MONEY)  ex->specif.scale  = 2;
        CI->sql_kont->volatile_var_ref=TRUE;
        CI->sql_kont->embvar_referenced=TRUE;
        next_sym(CI);
        return;
      }
    }
#if WBVERS<1000
    SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(IDENT_NOT_DEFINED); 
#endif
    // since 10.0 continue by analysing the identifier itself -- used in DAD parameters prefixed by ":"
  }
  if (CI->cursym==SQ_SQLSTATE && value_sel) // special access to the SQLSTATE system variable, which is a keyword as well
  { *pres = new t_expr_var(ATT_STRING, t_specif(5,0,FALSE,FALSE), SYSTEM_VAR_LEVEL, SYSVAR_ID_SQLSTATE);
    if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
    CI->sql_kont->volatile_var_ref=TRUE;
    next_sym(CI);
    return;
  }

  if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
  strcpy(name, CI->curr_name);

  if (next_sym(CI)=='.')
  { tobjname name2, nameapl;  BOOL aster = FALSE;
    if (next_sym(CI)==S_IDENT) 
    { strcpy(name2, CI->curr_name);
      if (next_sym(CI) == '.')  
      { if (next_sym(CI)==S_IDENT) // application name: ignored so far
        { strcpy(nameapl, name);  strcpy(name, name2);  strcpy(name2, CI->curr_name);  next_sym(CI); 
          reduce_schema_name(CI->cdp, nameapl);
        }
        else { test_and_next(CI, '*', IDENT_EXPECTED);  strcpy(name, name2);  aster=TRUE; }
      }
      else nameapl[0]=0;
    }
    else { test_and_next(CI, '*', IDENT_EXPECTED);  aster=TRUE; }
    if (aster)
    { if (CI->sql_kont->phase!=SQLPH_SELECT) c_error(IDENT_EXPECTED);
     // there is only 1 kontext, find any attribute with the prefix in it:
      db_kontext * kont1 = CI->sql_kont->active_kontext_list;  int i;
      if (!kont1)
        { SET_ERR_MSG(CI->cdp, name);  c_error(TABLE_DOES_NOT_EXIST); }
      for (i=0;  i<kont1->elems.count();  i++)
        if (!strcmp(elem_prefix(kont1, kont1->elems.acc0(i)), name)) break;
      if (i==kont1->elems.count())  // table name not found among active kontexts
        { SET_ERR_MSG(CI->cdp, name);  c_error(TABLE_DOES_NOT_EXIST); }
      ex = new t_expr_attr(-1, kont1);  *pres=ex;   // -1 represents .'*'
      if (!ex) c_error(OUT_OF_KERNEL_MEMORY);
      ex->type=0; // disables any operation
      ex->specif.length=i;  // 1st elemnum with the specified prefix
      return;
    }
    else // name prefixed with table or record name
    { el=find_list(CI, name, name2, elemnum, kont);
      if (el!=NULL)
      { *pres = ex = new t_expr_attr(elemnum, kont);  // all info copied
        if (!ex) c_error(OUT_OF_KERNEL_MEMORY);
      }
      else // try row with element name
      { t_locdecl * locdecl;  int level;
        locdecl = find_name_in_scopes(CI, name, level, LC_VAR);
        if (locdecl!=NULL)  
          if (locdecl->loccat==LC_VAR && locdecl->var.structured_type) // only for structured types, must not go here when a local variable has the same name as the schema name used as prefix of a procedure name
          { t_struct_type * row = locdecl->var.stype;
            t_row_elem * rel = NULL;
            for (int i=0;  i<row->elems.count();  i++)
              if (!strcmp(row->elems.acc0(i)->name, name2))
                { rel=row->elems.acc0(i);  break; }
            if (rel==NULL)
              { SET_ERR_MSG(CI->cdp, name2);  c_error(ATTRIBUTE_NOT_FOUND); }
            rel->used=TRUE;
            *pres = new t_expr_var(rel->type, rel->specif, level, locdecl->var.offset+rel->offset);
            if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
            CI->sql_kont->volatile_var_ref=TRUE;
            return;  // [] or # not allowed
          }
       // try sequence name
        if (!stricmp(name2, "NEXTVAL") || !stricmp(name2, "CURRVAL"))
        { tobjnum seq_obj;
          if (!name_find2_obj(CI->cdp, name, nameapl, CATEG_SEQ, &seq_obj))
          { if (!value_sel) 
              { SET_ERR_MSG(CI->cdp, name);  c_error(BAD_IDENT_CLASS); } // expecting selector
            *pres = new t_expr_seq(seq_obj, !stricmp(name2, "NEXTVAL") ? 1 : 0); 
            if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
            CI->sql_kont->volatile_var_ref=TRUE;
            return;
          }
        }
       // try global functions:
        else   // this "else" prevents calling load_server_extension for seq.nextval when seq does not exist
        if (!*nameapl) // only 2 names
        { tobjnum objnum;
          if (!name_find2_obj(CI->cdp, name2, name, CATEG_PROC, &objnum))
          { t_routine * routine = get_stored_routine(CI->cdp, objnum, NULL, FALSE);
            if (routine==NULL) CI->propagate_error();
            sql_stmt_call * so = new sql_stmt_call();  *pres = so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->source_line = CI->prev_line;
            so->routine_invocation(CI, routine, ROUT_GLOBAL_SHARED);  // routine deleted inside
            return;
          }
         // try extensions:
          HINSTANCE hInst;
          extension_object * exobj = search_obj_in_ext(CI->cdp, name2, name, &hInst);
          if (exobj) 
          { SET_ERR_MSG(CI->cdp, name2);  // for errors here and in compile_ext_object()
            if (!value_sel) c_error(BAD_IDENT_CLASS);  // expecting selector
            sql_stmt_call * call;
            compile_ext_object(CI, &call, exobj, true, hInst);
            *pres=call;
            return;
          }
        }
        SET_ERR_MSG(CI->cdp, name2);  c_error(ATTRIBUTE_NOT_FOUND); 
      }
    }
  }
  else // single identifier
  { if (!limit_db_kontext && find_list(CI, NULL, name, elemnum, kont)!=NULL)  // ignoring db kontext when [limit_db_kontext] is true and identifier is not prefixed
    { *pres = ex = new t_expr_attr(elemnum, kont);  // all info copied
      if (!ex) c_error(OUT_OF_KERNEL_MEMORY);
    }
    else
    { BOOL found = FALSE;
      if (//CI->sql_kont->active_kontext_list==NULL && -- allowing reference to other columns in subselects in CHECk conditions
          CI->sql_kont->kont_ta!=NULL)  // compiling in a single table kontext
      { int i;
        if (CI->sql_kont->column_number_for_value_keyword!=-1 && !strcmp(name, "VALUE"))
          { i=CI->sql_kont->column_number_for_value_keyword;  found=TRUE; }
        else
          for (i=0;  i<CI->sql_kont->kont_ta->attrs.count();  i++)
            if (!strcmp(name, CI->sql_kont->kont_ta->attrs.acc0(i)->name))
              { found=TRUE;  break; }
        if (found)
        { const atr_all * ppatr = CI->sql_kont->kont_ta->attrs.acc0(i);
          ex = new t_expr_attr(i, NULL, name);  *pres=ex; // "current" kontext
          if (!ex) c_error(OUT_OF_KERNEL_MEMORY);
          ex->type=ex->origtype=ppatr->type;  
          ex->specif=ex->origspecif=ppatr->specif;
          ex->nullable=ppatr->nullable;
          if (ppatr->multi != 1) ex->type |= 0x80;
          if (CI->atrmap!=NULL) CI->atrmap->add(i);
          if (ppatr->type==ATT_DOMAIN)  // translate, otherwise cannot use domain columns in expressions in index keys
          { t_type_specif_info tsi;
            if (!compile_domain_to_type(CI->cdp, ppatr->specif.domain_num, tsi))
              CI->propagate_error();
            ex->type    =ex->origtype  =tsi.tp;  
            ex->specif  =ex->origspecif=tsi.specif;
            ex->nullable               =tsi.nullable;
          }
        }
      }

      if (!found)
      {// search among declared objects:
        t_locdecl * locdecl;  int level;
        locdecl = find_name_in_scopes(CI, name, level, LC_VAR);
        if (locdecl!=NULL)
        { if (locdecl->loccat==LC_ROUTINE)
          { if (!locdecl->rout.routine->rettype) 
              { SET_ERR_MSG(CI->cdp, name);  c_error(BAD_IDENT_CLASS); }
            sql_stmt_call * so = new sql_stmt_call();  *pres = so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
            so->source_line = CI->prev_line;
            t_locdecl * locdecl_err = find_name_in_scopes(CI, name, level, LC_LABEL);
            if (locdecl_err)
              c_error(LOCAL_RECURSION);   // local routine calling itself or calling upper local routine
            so->routine_invocation(CI, locdecl->rout.routine, ROUT_LOCAL);  // routine owned by the scope
          }
          else if (locdecl->loccat==LC_VAR   || locdecl->loccat==LC_CONST  ||
                   locdecl->loccat==LC_INPAR || locdecl->loccat==LC_OUTPAR ||
                   locdecl->loccat==LC_INPAR_|| locdecl->loccat==LC_OUTPAR_||
                   locdecl->loccat==LC_PAR   || locdecl->loccat==LC_INOUTPAR || locdecl->loccat==LC_FLEX)
          { *pres = new t_expr_var(locdecl->var.type, locdecl->var.specif, level, locdecl->var.offset);
            if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
            // IN-OUT parameter test
            if (value_sel)
            { //if (locdecl->loccat==LC_OUTPAR) c_error(BAD_PARAMETER_MODE);
              //--not correct, I think
              if (locdecl->loccat==LC_PAR)    locdecl->loccat=LC_INPAR_;
              if (locdecl->loccat==LC_OUTPAR_)locdecl->loccat=LC_INOUTPAR;
            }
            else // target
            { //if (locdecl->loccat==LC_INPAR) c_error(BAD_PARAMETER_MODE);
              //--not correct, I think
              if (locdecl->loccat==LC_PAR)    locdecl->loccat=LC_OUTPAR_;
              if (locdecl->loccat==LC_INPAR_) locdecl->loccat=LC_INOUTPAR;
              if (locdecl->loccat==LC_CONST) c_error(CANNOT_CHANGE_CONSTANT);
            }
            CI->sql_kont->volatile_var_ref=TRUE;
          }
          else { SET_ERR_MSG(CI->cdp, name);  c_error(BAD_IDENT_CLASS); }
          return;
        }
        else // looking among system variables and functions
        { t_locdecl * locdecl = system_scope.find_name(name, LC_VAR);
          if (locdecl!=NULL)
          { if (locdecl->loccat!=LC_VAR) 
              { SET_ERR_MSG(CI->cdp, name);  c_error(BAD_IDENT_CLASS); }
            *pres = new t_expr_var(locdecl->var.type, locdecl->var.specif, SYSTEM_VAR_LEVEL, locdecl->var.offset);
            if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
            CI->sql_kont->volatile_var_ref=TRUE;
           // instable wars make the cursor instable:
            if (locdecl->var.offset==SYSVAR_ID_FULLTEXT_WEIGHT ||
                locdecl->var.offset==SYSVAR_ID_FULLTEXT_POSITION) 
              CI->sql_kont->instable_var=TRUE;  
            return;
          }
        }

       // try NEXT VALUE FOR sequence:
        if (CI->cursym==S_IDENT && !strcmp(name, "NEXT"))
        { tobjnum seq_obj;  tobjname schema;
          if (CI->cursym!=S_IDENT || strcmp(CI->curr_name, "VALUE")) c_error(GENERAL_SQL_SYNTAX);
          next_and_test(CI, SQ_FOR, FOR_EXPECTED);
          get_schema_and_name(CI, name, schema);
          if (name_find2_obj(CI->cdp, name, schema, CATEG_SEQ, &seq_obj))
            { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(OBJECT_NOT_FOUND); }
          if (!value_sel) 
            { SET_ERR_MSG(CI->cdp, name);  c_error(BAD_IDENT_CLASS); } // expecting selector
          *pres = new t_expr_seq(seq_obj, 2); 
          if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
          CI->sql_kont->volatile_var_ref=TRUE;
          return;
        }
       // try global functions:
        tobjnum objnum = find_object(CI->cdp, CATEG_PROC, name);
        if (objnum!=NOOBJECT)
        { t_routine * routine = get_stored_routine(CI->cdp, objnum, NULL, FALSE);
          if (routine==NULL) CI->propagate_error();
          sql_stmt_call * so = new sql_stmt_call();  *pres = so;  if (!so) c_error(OUT_OF_KERNEL_MEMORY);
          so->source_line = CI->prev_line;
          so->routine_invocation(CI, routine, ROUT_GLOBAL_SHARED);  // routine released inside
          return;
        }

       // search among standard objects:
        object * obj = search_obj(name); // searches the object
        if (obj!=NULL)  // standard object found
        { if (!value_sel)  // expecting selector or standard type found
            { SET_ERR_MSG(CI->cdp, name);  c_error(BAD_IDENT_CLASS); }
          switch (obj->any.categ)
          { default:
              SET_ERR_MSG(CI->cdp, name);  c_error(BAD_IDENT_CLASS);
            case OBT_SPROC:
            { switch (obj->proc.sprocnum)
              { case FCNUM_REGEXPR:
                { t_expr_funct_regex * fex = new t_expr_funct_regex;
                  *pres=fex;  if (!fex) c_error(OUT_OF_KERNEL_MEMORY);
                  fex->origtype=fex->type=ATT_BOOLEAN;
                  fex->specif.set_null();  fex->origspecif.set_null(); // may be changed
                  compile_regexpr_call(CI, fex);
                  break;
                }
                default:
                { t_expr_funct * fex = new t_expr_funct(obj->proc.sprocnum);
                  *pres=fex;  if (!fex) c_error(OUT_OF_KERNEL_MEMORY);
                  if (obj->proc.pars==NULL)
                  { fex->type=fex->origtype=obj->proc.retvaltype;  // may be changed below
                    fex->specif.set_null();  fex->origspecif.set_null(); // may be changed
                    special_function(CI, fex);
                  }
                  else
                  { call_gen(CI, &obj->proc, fex);
                    //if (fex->num == FCNUM_LETTERCREW)
                    //{ fex->arg1->specif.wide_char = 1;
                    //  fex->arg2->specif.wide_char = 1;
                    //}
                  }
                  if (!fex->type)  // procedure in expression
                    { SET_ERR_MSG(CI->cdp, name);  c_error(BAD_IDENT_CLASS); }
                  break;
                }
              }
              break;
            }
            case OBT_CONST:  // standard constant
            { t_specif specif;  if (obj->cons.type==ATT_CHAR) specif.length=1;  // no scaled integers are defined as constants
              *pres=new t_expr_sconst(obj->cons.type, specif, &obj->cons.value, tpsize[obj->cons.type]);
              if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
              break;
            }

          }
          return;
        } // name found among defined objects
       // search in extensions:
        HINSTANCE hInst;
        extension_object * exobj = search_obj_in_ext(CI->cdp, name, NULL, &hInst);
        if (exobj) 
        { SET_ERR_MSG(CI->cdp, name);  // for errors here and in compile_ext_object()
          if (!value_sel) c_error(BAD_IDENT_CLASS);  // expecting selector
          sql_stmt_call * call;
          compile_ext_object(CI, &call, exobj, true, hInst);
          *pres=call;
          return;
        }
       // name not found anywhere:
        SET_ERR_MSG(CI->cdp, name);  c_error(ATTRIBUTE_NOT_FOUND);
      }
    } // not found in database kontext
  } // single identifier

 again:
 // index and double index (ex defined):
  while (CI->cursym=='[')
  { next_sym(CI);  t_express * indexexpr;
    sql_expression(CI, &indexexpr);  // cannot hold better, ex->indexexpr may be in use!
    if (!is_integer(indexexpr->type, indexexpr->specif))
      { delete indexexpr;  c_error(MUST_BE_INTEGER); }
    if (CI->cursym==',')
    { if (ex->type & 0x80 || !IS_HEAP_TYPE(ex->type) && !IS_EXTLOB_TYPE(ex->type))
        { delete indexexpr;  c_error(INCOMPATIBLE_TYPES); }
      t_expr_dblind * iex;
      *pres=iex=new t_expr_dblind(ex, indexexpr);
      if (iex==NULL) { delete ex;  delete indexexpr;  c_error(OUT_OF_MEMORY); }
      next_sym(CI);
      sql_expression(CI, &iex->size);
      if (!is_integer(iex->size->type, iex->size->specif)) c_error(MUST_BE_INTEGER);
     // for small constant sizes define the size of the result
      if (iex->size->sqe==SQE_SCONST)
      { sig32 ival = *(sig32*)((t_expr_sconst*)iex->size)->val;
        if (ival>0 && ival<=MAX_FIXED_STRING_LENGTH)
        { if (iex->type==ATT_TEXT || iex->type==ATT_EXTCLOB)
          { if (iex->specif.wide_char) ival*=2;
            iex->type=ATT_STRING;
          }
          else // if (IS_HEAP_TYPE(iex->type), extblob)
            iex->type=ATT_BINARY;
          iex->specif.length=(uns16)ival;
        }
      }
    }
    else
    { if (ex->type & 0x80) 
      { ex->type &= 0x7f;  // multiattribute index
        ex->indexexpr=indexexpr;
      }
      else if (CI->startnet==index_expr_comp2 && is_string(ex->type))  // ignore
        delete indexexpr; 
      else if (is_string(ex->type))
      { t_expr_funct * fex;
        *pres = fex = new t_expr_funct(FCNUM_SUBSTRING);  if (!fex) c_error(OUT_OF_KERNEL_MEMORY);
        fex->arg1=ex;  fex->arg2=indexexpr;  
        fex->type=fex->origtype=ATT_CHAR;  fex->specif.length=1;
        sig32 one = 1;  
        fex->arg3=new t_expr_sconst(ATT_INT32, t_specif(0,0), &one, sizeof(sig32));
        if (!fex->arg3) c_error(OUT_OF_KERNEL_MEMORY);
      }
      else 
        { delete indexexpr;  c_error(NOT_AN_ARRAY); }
    }
    test_and_next(CI, ']', RIGHT_BRACE_EXPECTED);
  }
 // ^ - passing via pointer:
  if (CI->cursym=='^')
  { if (ex->type!=ATT_PTR && ex->type!=ATT_BIPTR || !ex->specif.destination_table) c_error(NOT_A_PTR);
    if (next_sym(CI)=='.') next_sym(CI);
    if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
    t_expr_pointed_attr * poatr = new t_expr_pointed_attr(ex);  *pres = poatr;
    if (*pres==NULL) { delete ex;  c_error(OUT_OF_MEMORY); }
    poatr->desttb=ex->specif.destination_table;
   // create destination attribute kontext, copy its properties:
    { table_descr_auto td(CI->cdp, poatr->desttb, TABLE_DESCR_LEVEL_COLUMNS_INDS);
      if (!td->me()) c_error(CI->cdp->get_return_code());
      poatr->dest_table_kont.fill_from_td(CI->cdp, td->me());
    }
    int elemnum;
    elem_descr * el = poatr->dest_table_kont.find(NULL, CI->curr_name, elemnum);
    if (!el) { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(ATTRIBUTE_NOT_FOUND); }
    poatr->get_kontext_info(elemnum, &poatr->dest_table_kont);
#ifdef STOP
    BOOL found=FALSE;
    for (int atr=1;  atr<td->attrcnt;  atr++)
      if (!strcmp(td->attrs[atr].name, CI->curr_name))
        { found=TRUE;  break; }
    if (found)
    { poatr->elemnum=atr;
      poatr->type  =poatr->origtype  =td->attrs[atr].attrtype;
      poatr->specif=poatr->origspecif=td->attrs[atr].attrspecif;
      if (td->attrs[atr].attrmult != 1) poatr->type = poatr->origtype |= 0x80;
      poatr->nullable=td->attrs[atr].nullable;
      strcpy(poatr->name, td->attrs[atr].name);
      uns8 priv_val[PRIVIL_DESCR_SIZE];
      CI->cdp->prvs.get_effective_privils(poatr->desttb, NORECNUM, priv_val);
      if (HAS_READ_PRIVIL(priv_val,atr)) poatr->pr_flags=PRFL_READ_TAB;
      else if (td->tabdef_flags & TFL_REC_PRIVILS) poatr->pr_flags=PRFL_READ_REC;
      else poatr->pr_flags=PRFL_READ_NO;
      if (HAS_WRITE_PRIVIL(priv_val,atr)) poatr->pr_flags|=PRFL_WRITE_TAB;
      else if (td->tabdef_flags & TFL_REC_PRIVILS) poatr->pr_flags|=PRFL_WRITE_REC;
    }
    unlock_tabdescr(poatr->desttb);
#endif
    next_sym(CI);
    ex=poatr;
    goto again;
  }
 // # - length or count:
  if (CI->cursym=='#')
  { if (ex->type & 0x80) // checking the argument type (after possible indexing)
      *pres=new t_expr_mult_count(ex);
    else if (IS_HEAP_TYPE(ex->type) || IS_EXTLOB_TYPE(ex->type))
      *pres=new t_expr_var_len(ex);
    else c_error(NO_LENGTH);
    if (*pres==NULL) { delete ex;  c_error(OUT_OF_MEMORY); }
    next_sym(CI);
  }
}

void sql_primary(CIp_t CI, t_express * * pres)
{ switch (CI->cursym)
  { case SQ_NULLIF:
    { t_expr_case * ex = new t_expr_case(CASE_NULLIF);  *pres=ex;  if (!ex) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);
      test_and_next(CI, '(', LEFT_PARENT_EXPECTED);
      sql_expression(CI, &ex->val1);
      test_and_next(CI, ',', COMMA_EXPECTED);
      sql_expression(CI, &ex->val2);
      binary_conversion(CI, '=', ex->val1, ex->val2, ex);
      ex->specif=ex->val1->specif;  // val2->specif can be ignored
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
      break;
    }
    case SQ_COALESCE: // val1 is 1st operand, val2 is the 2nd operand or the
                      // next COALESCE, contin not used.
                      // All operands are converted to the common supertype.
    { t_expr_case * tex = new t_expr_case(CASE_COALESCE);
      *pres=tex;  if (!tex) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);
      test_and_next(CI, '(', LEFT_PARENT_EXPECTED);
      sql_expression(CI, &tex->val1);
      tex->type=tex->val1->type;  tex->specif=tex->val1->specif;
      if (CI->cursym!=',') c_error(COMMA_EXPECTED);
     // compile the other parameters:
      t_express * * pex = &tex->val2;  // result to be assigned
      do
      { next_sym(CI); // skips comma
        sql_expression(CI, pex);
        add_type(CI, tex, *pex);
        if (CI->cursym==',') // another coalesce, move *pex to its val1
        { t_expr_case * nex = new t_expr_case(CASE_COALESCE);  if (!nex) c_error(OUT_OF_KERNEL_MEMORY);
          nex->val1=*pex;  *pex=nex;  pex=&nex->val2;
        }
        else break;
      } while (TRUE);
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
     // add conversions to the common supertype:
      t_expr_case * ex=tex;
      while (ex->sqe==SQE_CASE && ex->case_type==CASE_COALESCE)
      { convert_value(CI, ex->val1, tex->type, tex->specif);
        ex=(t_expr_case*)ex->val2;
      }
      convert_value(CI, ex, tex->type, tex->specif);
      break;
    }
    case SQ_CASE:
      if (next_sym(CI)==SQ_WHEN) // searched CASE
      // if val1 then val2 else contin, contin is next CASE or ELSE expr.
      { t_expr_case * tex = NULL, * ex;
        t_express * * pex = pres;  // result to be assigned here
        do
        { next_sym(CI);
          *pex = ex = new t_expr_case(CASE_SRCH);  if (!ex) c_error(OUT_OF_KERNEL_MEMORY);
          if (tex==NULL)  // top node
            { tex=ex;  tex->type=0;  tex->specif.set_null(); }
          search_condition(CI, &ex->val1);
          test_and_next(CI, SQ_THEN, THEN_EXPECTED);
          sql_expression(CI, &ex->val2);
          add_type(CI, tex, ex->val2);
          pex=&ex->contin;
        } while (CI->cursym==SQ_WHEN);
        if (CI->cursym==SQ_ELSE)
        { next_sym(CI);
          sql_expression(CI, pex);
          add_type(CI, tex, *pex);
        }
        test_and_next(CI, SQ_END, END_EXPECTED);
       // add conversions to the common type:
        ex=tex;
        while (ex!=NULL && ex->sqe==SQE_CASE && ex->case_type==CASE_SRCH)
        { convert_value(CI, ex->val2, tex->type, tex->specif);
          ex=(t_expr_case*)ex->contin;
        }
        if (ex!=NULL) // ELSE specified, convert its value too
          convert_value(CI, ex, tex->type, tex->specif);
        break;
      }

      else // simple case, must coalesce both types of labes and results
      // CASE_SMPL1 contains pattern value in val1, CASE_SMPL2 in contin;
      // CASE_SMPL2 contains label in val1, value in val2, next CASE_SMPL2
      // or ELSE value in contin.
      { t_expr_case * tex, * ex;
        t_expr_null resultaux;  resultaux.specif.set_null();  // result supertype
        t_expr_null labelaux;   labelaux .specif.set_null();  // label supertype
        *pres = tex = new t_expr_case(CASE_SMPL1);  if (!tex) c_error(OUT_OF_KERNEL_MEMORY);
        sql_expression(CI, &tex->val1);
        add_type(CI, &labelaux, tex->val1);
        t_express * * pex = &tex->contin;  // result to be assigned here
        while (CI->cursym==SQ_WHEN)
        { next_sym(CI);
          *pex = ex = new t_expr_case(CASE_SMPL2);  if (!ex) c_error(OUT_OF_KERNEL_MEMORY);
          sql_expression(CI, &ex->val1);
          add_type(CI, &labelaux,  ex->val1);
          test_and_next(CI, SQ_THEN, THEN_EXPECTED);
          sql_expression(CI, &ex->val2);
          add_type(CI, &resultaux, ex->val2);
          pex=&ex->contin;
        }
        if (CI->cursym==SQ_ELSE)
        { next_sym(CI);
          sql_expression(CI, pex);
          add_type(CI, &resultaux, *pex);
        }
        test_and_next(CI, SQ_END, END_EXPECTED);
       // add conversions to the common type and comparison operator:
        convert_value(CI, tex->val1, labelaux.type, labelaux.specif);
        binary_conversion(CI, '=', tex->val1, tex->val1, tex);
        ex=(t_expr_case*)tex->contin;
        while (ex!=NULL && ex->sqe==SQE_CASE && ex->case_type==CASE_SMPL2)
        { convert_value(CI, ex->val1, labelaux .type, labelaux.specif);
          convert_value(CI, ex->val2, resultaux.type, resultaux.specif);
          ex=(t_expr_case*)ex->contin;
        }
        if (ex!=NULL) // ELSE specified, convert its value too
          convert_value(CI, ex,       resultaux.type, resultaux.specif);
        tex->type=resultaux.type;  tex->specif=resultaux.specif;
        break;
      }
    case S_NULL:
      *pres = new t_expr_null;  if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);
      break;
    case SQ_CAST:
    { t_expr_cast * cex = new t_expr_cast;  *pres=cex;  if (!cex) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);
      test_and_next(CI, '(', LEFT_PARENT_EXPECTED);
      sql_expression(CI, &cex->arg);
      test_and_next(CI, S_AS, AS_EXPECTED);
      analyse_variable_type(CI, cex->type, cex->specif, NULL);  // some types disabled, translates domains
      if (cex->arg->sqe==SQE_NULL) cex->arg->convert=CONV_NO;
      else 
      { cex->arg->convert=get_type_cast(cex->arg->type, cex->type, cex->arg->specif, cex->specif, false);
        if (cex->arg->convert.conv==CONV_ERROR) c_error(INCOMPATIBLE_TYPES);
      }
      cex->arg->type=cex->type;
      cex->arg->specif=cex->specif;  // used when checking for right trucnation
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
      break;
    }
    case '(':
    { next_sym(CI);
      if (CI->cursym==S_SELECT)
        subquery_expression(CI, pres, TRUE);
      else sqlbin_expression(CI, pres, FALSE);
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
      break;
    }
    case S_IDENT:
    case '?':
    case ':':
    case SQ_SQLSTATE:
      sql_selector(CI, pres, true, false);
      break;
    case '@':  // current record number, only in single-table queries
      *pres = new t_expr_currrec(CI->sql_kont->active_kontext_list);  if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);  break;

    case S_USER:
    { t_expr_funct * fex = new t_expr_funct(FCNUM_USER);  
      *pres = fex;  if (!fex) c_error(OUT_OF_KERNEL_MEMORY);  
      fex->type=fex->origtype=ATT_STRING;  fex->specif.length=OBJ_NAME_LEN;
      next_sym(CI);  break;
    }
    case S_INTEGER:
      if (CI->curr_int>0x7fffffff || CI->curr_int<-0x7fffffff)
        *pres=new t_expr_sconst(ATT_INT64, t_specif(CI->curr_scale, i64_precision), &CI->curr_int, sizeof(sig64));
      else
        *pres=new t_expr_sconst(ATT_INT32, t_specif(CI->curr_scale, i32_precision), &CI->curr_int, sizeof(sig32));
      if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);  break;
    case S_MONEY: // used only when explicit '$' specified
      *pres=new t_expr_sconst(ATT_MONEY, t_specif(CI->curr_scale, i64_precision), &CI->curr_int, sizeof(sig64));
      if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);  break;
    case S_DATE:
      *pres=new t_expr_sconst(ATT_DATE,      t_specif(), &CI->curr_int,  sizeof(uns32));
      if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);  break;
    case S_TIME:
      *pres=new t_expr_sconst(ATT_TIME,      t_specif(), &CI->curr_int,  sizeof(uns32));
      if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);  break;
    case S_TIMESTAMP:
      *pres=new t_expr_sconst(ATT_TIMESTAMP, t_specif(), &CI->curr_int,  sizeof(uns32));
      if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);  break;
    case S_REAL:
      *pres=new t_expr_sconst(ATT_FLOAT,     t_specif(), &CI->curr_real, sizeof(double));
      if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);  break;
    case S_CHAR:
      CI->curr_string()[1]=0;
      *pres=new t_expr_lconst(ATT_CHAR, CI->curr_string(), 1, sys_spec.charset);
      if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);  break;
    case S_STRING: 
      if (!CI->curr_int)
      { *pres=new t_expr_lconst(ATT_STRING, CI->curr_string(), (int)strlen(CI->curr_string()), sys_spec.charset);
        if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      }
      else
      { t_expr_lconst * lc;
        *pres = lc =new t_expr_lconst(ATT_STRING, NULL, 0, sys_spec.charset);
        if (!lc) c_error(OUT_OF_KERNEL_MEMORY);
        corefree(lc->lval);
        lc->lval = (char*)corealloc(2*strlen(CI->curr_string())+2, 88);
        if (!lc->lval) c_error(OUT_OF_KERNEL_MEMORY);
       // convert escaped string to Unicode:
        if (conv1to2(CI->curr_string(),(int) strlen(CI->curr_string()), (wuchar*)lc->lval, sys_spec.charset, 0))
          c_error(BAD_CHAR);
       // replace escapes:
        wuchar * src = (wuchar*)lc->lval, * dest = (wuchar*)lc->lval;
        while (*src)
        { if (*src==CI->curr_int)  // escape
            if (src[1]==CI->curr_int)
              { *dest=*src;  src+=2; }
            else  // Unicode escape value
            { src++;
              int digits;   *dest=0;
              if (*src=='+')
                { digits=6;  src++;  c_error(BAD_CHAR); }
              else digits=4;
              while (digits--)
              { *dest=16*(*dest);
                if (*src>='0' && *src<='9') *dest += *src-'0';
                else if (*src>='A' && *src<='Z') *dest += *src-'A'+10;
                else if (*src>='a' && *src<='z') *dest += *src-'a'+10;
                else c_error(BAD_CHAR);
                src++;
              }
            }
          else
            { *dest=*src;  src++; }
          dest++;  
        }
        *dest=0;
        lc->valsize = (int)sizeof(wuchar) * (int)(dest - (wuchar*)lc->lval);
        lc->specif.length = lc->valsize;
        lc->specif.wide_char = 1;
      }
      next_sym(CI);  break;
    case S_BINLIT:    // length is stored in CI->curr_int
      *pres=new t_expr_lconst(ATT_BINARY, CI->curr_string(), (int)CI->curr_int, 0);
      if (!*pres) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);  break;

    case S_AVG:
    case S_COUNT:
    case S_MAX:
    case S_MIN:
    case S_SUM:
    { int intern_aggr_type;  int at;  // aggregation type and argument type
      t_expr_aggr * agex = new t_expr_aggr(intern_aggr_type=CI->cursym, &CI->sql_kont->curr_qe->kont);
      *pres=agex;  if (!agex) c_error(OUT_OF_KERNEL_MEMORY);
      switch (CI->sql_kont->phase)
      { case SQLPH_WHERE:
        case SQLPH_GROUP:  c_error(AGGR_IN_WHERE);  // cannot use aggr. in WHERE
        case SQLPH_NORMAL: c_error(AGGR_NO_KONTEXT);// cannot use aggr. ouside SELECT
        case SQLPH_ORDER:  c_error(AGGR_IN_ORDER);  // cannot use aggr. in ORDER
        case SQLPH_INAGGR: c_error(AGGR_NESTED);    // nested aggregates
      }
      CI->sql_kont->aggr_in_expr=TRUE;

     // analyse and compile the function argument:
      next_and_test(CI, '(', LEFT_PARENT_EXPECTED);
      if (next_sym(CI) == '*')
      { if (intern_aggr_type != S_COUNT) c_error(ONLY_FOR_COUNT);
        next_sym(CI);
      }
      else
      { if (CI->cursym == S_DISTINCT)
        { if (intern_aggr_type!=S_MAX && intern_aggr_type!=S_MIN) // optimization
            agex->distinct=TRUE;
          next_sym(CI);
        }
        else if (CI->cursym == S_ALL) next_sym(CI);
        /* SQL extenstion: COUNT(attribute) - counts non-null values */
       /* inside expression: */
        t_comp_phase upper_phase=CI->sql_kont->phase;  CI->sql_kont->phase=SQLPH_INAGGR;
        sqlbin_expression(CI, &agex->arg, FALSE);
        CI->sql_kont->phase=upper_phase;
        at=agex->arg->type;
       /* check the expression type: */
        if (at & 0x80) c_error(BAD_AGGR_FNC_ARG_TYPE);
        if (intern_aggr_type != S_COUNT)
          if (at!=ATT_INT16 && at!=ATT_INT32 && at!=ATT_INT8 && at!=ATT_INT64 && at!=ATT_FLOAT && at!=ATT_MONEY)
            if ((at!=ATT_CHAR && at!=ATT_BOOLEAN && at!=ATT_TIME &&
                 at!=ATT_DATE && at!=ATT_TIMESTAMP && at!=ATT_DATIM &&
                 !is_string(at) && at!=ATT_BINARY) ||
                intern_aggr_type==S_SUM || intern_aggr_type==S_AVG)
              c_error(BAD_AGGR_FNC_ARG_TYPE);
      }
      test_and_next(CI,')',RIGHT_PARENT_EXPECTED);

     // search for the agrument, register a new aggregate if not found:
      if (intern_aggr_type==S_AVG)
      { agex->agnum =get_agr_index(CI, S_SUM,   agex->distinct, agex->arg);
        agex->agnum2=get_agr_index(CI, S_COUNT, agex->distinct, agex->arg);
      }
      else
      { agex->agnum =get_agr_index(CI, intern_aggr_type, agex->distinct, agex->arg);
        agex->agnum2=0;
      }
     // result type:
      if (intern_aggr_type==S_COUNT)
      { agex->type=agex->restype=ATT_INT32;  agex->specif.set_num(0, 9); }
      else if ((at==ATT_INT16 || at==ATT_INT8) && (intern_aggr_type==S_SUM || intern_aggr_type==S_AVG))
      { agex->type=agex->restype=ATT_INT32;  agex->specif.set_num(agex->arg->specif.scale, 9); }
      else  // function result has the same type & specif as argument
      { agex->type=agex->restype=at;  agex->specif=agex->arg->specif; }
      break;
    }
    default:
      c_error(FAKTOR_EXPECTED);
  }
}

void sql_factor(CIp_t CI, t_express * * pex)
{ if (CI->cursym=='-')
  { next_sym(CI);
    sql_primary(CI, pex);
    if ((*pex)->sqe==SQE_SCONST) // optimizing '-' in front of a constant
    { t_expr_sconst * scex = (t_expr_sconst*)*pex;
      switch (scex->origtype)
      { case ATT_INT8:
          *(sig32 *)scex->val=-(sig32)*(sig8 *)scex->val;  break;
        case ATT_INT16:
          *(sig32 *)scex->val=-(sig32)*(sig16*)scex->val;  break;
        case ATT_INT32:
          *(sig32 *)scex->val=-       *(sig32*)scex->val;  break;
        case ATT_INT64:  case ATT_MONEY:
          *(sig64 *)scex->val=-       *(sig64*)scex->val;  break;
        case ATT_FLOAT:
          *(double*)scex->val=-      *(double*)scex->val;  break;
        default:
          c_error(INCOMPATIBLE_TYPES);
      }
    }
    else
    { t_expr_unary * unex = new t_expr_unary('-');  if (!unex) c_error(OUT_OF_KERNEL_MEMORY);  
      unex->op=*pex;  *pex = unex;
      unary_conversion(CI, '-', unex->op, unex);
    }
  }
  else sql_primary(CI, pex);
}

void sql_term(CIp_t CI, t_express * * pex)
{ sql_factor(CI, pex);
  symbol op = CI->cursym;
  while (op=='*' || op=='/' || op==SQ_DIV || op==SQ_MOD)
  { t_expr_binary * binex = new t_expr_binary();  if (!binex) c_error(OUT_OF_KERNEL_MEMORY);
    binex->op1=*pex;  *pex=binex;
    next_sym(CI);
    sql_factor(CI, &binex->op2);
    binary_conversion(CI, op, binex->op1, binex->op2, binex);
    op = CI->cursym;
  }
}

void sql_expression(CIp_t CI, t_express * * pex)
{ sql_term(CI, pex);
  symbol op = CI->cursym;
  while (op=='+' || op=='-' || op==SQ_CONCAT)
  { t_expr_binary * binex = new t_expr_binary();  if (!binex) c_error(OUT_OF_KERNEL_MEMORY);
    binex->op1=*pex;  *pex=binex;
    next_sym(CI);
    sql_term(CI, &binex->op2);
    binary_conversion(CI, op, binex->op1, binex->op2, binex);
    op = CI->cursym;
  }
}

static BOOL negate_oper(t_compoper & oper)
{ if (oper.pure_oper==PUOP_IS_NULL || oper.pure_oper==PUOP_MULTIN || 
      oper.pure_oper==PUOP_LIKE || oper.pure_oper==PUOP_PREF || oper.pure_oper==PUOP_SUBSTR ||
      oper.pure_oper==PUOP_BETWEEN)
    oper.negate=!oper.negate;
  else
  { switch (oper.pure_oper)
    { case PUOP_LT:          oper.pure_oper = PUOP_GE;        break;
      case PUOP_GT:          oper.pure_oper = PUOP_LE;        break;
      case PUOP_EQ:          oper.pure_oper = PUOP_NE;        break;
      case PUOP_GE:          oper.pure_oper = PUOP_LT;        break;
      case PUOP_LE:          oper.pure_oper = PUOP_GT;        break;
      case PUOP_NE:          oper.pure_oper = PUOP_EQ;        break;
      default:  return FALSE;
    }
    if      (oper.all_disp) { oper.all_disp=0;  oper.add_any_disp(); }
    else if (oper.any_disp) { oper.any_disp=0;  oper.add_all_disp(); }
  }
  return TRUE;
}

static void symmetric_oper(t_compoper & oper)
// Used only for comparisons
{ switch (oper.pure_oper)
  { case PUOP_LT:  oper.pure_oper = PUOP_GT;  break;
    case PUOP_GT:  oper.pure_oper = PUOP_LT;  break;
    case PUOP_EQ:  oper.pure_oper = PUOP_EQ;  break;
    case PUOP_GE:  oper.pure_oper = PUOP_LE;  break;
    case PUOP_LE:  oper.pure_oper = PUOP_GE;  break;
    case PUOP_NE:  oper.pure_oper = PUOP_NE;  break;
  }
}

static void negate_predicate(t_express * ex)
{ if (ex->sqe==SQE_BINARY)
  { t_expr_binary * bex = (t_expr_binary *)ex;
    if (bex->oper.pure_oper==PUOP_AND)
    { bex->oper.pure_oper = PUOP_OR;
      negate_predicate(bex->op1);  negate_predicate(bex->op2);
      return;
    }
    else if (bex->oper.pure_oper==PUOP_OR)
    { bex->oper.pure_oper = PUOP_AND;
      negate_predicate(bex->op1);  negate_predicate(bex->op2);
      return;
    }
    else
      if (negate_oper(bex->oper)) return;
  }
  else if (ex->sqe==SQE_TERNARY)
  { t_expr_ternary * tex = (t_expr_ternary *)ex;
    if (negate_oper(tex->oper)) return;
  }
  else ex->convert=ex->convert.conv==CONV_NEGATE ? t_convert(CONV_NO) : t_convert(CONV_NEGATE);
}

static void sqlbin_predicate(CIp_t CI, t_express * * pex)
// Compiles a binary primary. Result is always Boolean.
// Outer negation processed outside by negate_predicate.
{
  switch (CI->cursym)
  { //case '(':
    //  next_sym(CI);
    //  sqlbin_expression(CI, pex, FALSE);
    //  test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
    //  break;
    case S_EXISTS:
    case S_UNIQUE:
    { t_expr_unary * unex = new t_expr_unary(CI->cursym);  *pex=unex;  if (!unex) c_error(OUT_OF_KERNEL_MEMORY);
      next_sym(CI);
      test_and_next(CI, '(', LEFT_PARENT_EXPECTED);
      subquery_expression(CI, &unex->op, FALSE);
      unary_conversion(CI, unex->oper, unex->op, unex);
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
      if (unex->oper==UNOP_UNIQUE)
      { t_expr_subquery * suex = (t_expr_subquery*)unex->op;
        suex->rowdist = new row_distinctor(true); // create a new one...
        if (!suex->rowdist) c_error(OUT_OF_KERNEL_MEMORY);
        if (!suex->rowdist->prepare_rowdist_index(suex->qe, TRUE, 1, FALSE))
          c_error(OUT_OF_KERNEL_MEMORY);
      }
      break;
    }
    default:
    { sql_expression(CI, pex);  t_express * op1 = *pex;
      BOOL negate=FALSE;
      if (CI->cursym==SQ_NOT) { negate=TRUE;  next_sym(CI); }
      switch (CI->cursym)
      { case '=':       case '<':           case '>':
        case S_NOT_EQ:  case S_LESS_OR_EQ:  case S_GR_OR_EQ:
        case S_PREFIX:  case S_SUBSTR:      case S_DDOT:
        { if (negate) c_error(NOT_NOT_ALLOWED);
          symbol op = CI->cursym;
          t_expr_binary * binex = new t_expr_binary();  if (!binex) c_error(OUT_OF_KERNEL_MEMORY);
          binex->op1=op1;  *pex=binex;
          symbol subq = next_sym(CI);
          if (subq==S_ALL || subq==SQ_ANY || subq==SQ_SOME)
          { subq=CI->cursym;  next_sym(CI);
            test_and_next(CI, '(', LEFT_PARENT_EXPECTED);
            subquery_expression(CI, &binex->op2, TRUE);
            test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
          }
          else sql_expression(CI, &binex->op2);
          binary_conversion(CI, op, binex->op1, binex->op2, binex); // converts and checks operand types, writes ATT_BOOLEAN result
          if      (subq==S_ALL)                   binex->oper.add_all_disp();
          else if (subq==SQ_ANY || subq==SQ_SOME) binex->oper.add_any_disp();
          break;
        }
        case '~':           
        { t_expr_funct * fex = new t_expr_funct(LIKE_FCNUM);
          *pex=fex;  if (!fex) c_error(OUT_OF_KERNEL_MEMORY);
          fex->arg1=op1;
          if (!is_char_string(fex->arg1->type)) c_error(INCOMPATIBLE_TYPES);
          next_sym(CI);
          sql_expression(CI, &fex->arg2);
          if (!is_char_string(fex->arg2->type)) c_error(INCOMPATIBLE_TYPES);
          fex->type=fex->origtype=ATT_BOOLEAN;  fex->specif.set_null();
          if (negate) fex->convert=CONV_NEGATE;
          break;
        }
        case S_BETWEEN:
        { t_expr_ternary * ternex = new t_expr_ternary();  if (!ternex) c_error(OUT_OF_KERNEL_MEMORY);
          ternex->op1=op1;  *pex=ternex;
          next_sym(CI);
          sql_expression(CI, &ternex->op2);
          test_and_next(CI, SQ_AND, AND_EXPECTED);
          sql_expression(CI, &ternex->op3);
          if (ternex->op1->sqe!=SQE_DYNPAR)
          { if (ternex->op2->sqe==SQE_DYNPAR) 
              supply_marker_type(CI, (t_expr_dynpar*)ternex->op2, ternex->op1->type, ternex->op1->specif, MODE_IN);
            if (ternex->op3->sqe==SQE_DYNPAR) 
              supply_marker_type(CI, (t_expr_dynpar*)ternex->op3, ternex->op1->type, ternex->op1->specif, MODE_IN);
          }
          ternex->type=ternex->op1->type;  ternex->specif=ternex->op1->specif;
          add_type(CI, ternex, ternex->op2);  add_type(CI, ternex, ternex->op3);
          convert_value(CI, ternex->op1, ternex->type, ternex->specif);
          convert_value(CI, ternex->op2, ternex->type, ternex->specif);
          convert_value(CI, ternex->op3, ternex->type, ternex->specif);
          binary_conversion(CI, S_BETWEEN, ternex->op1, ternex->op2, ternex);
          if (negate) ternex->oper.negate=1;
          break;
        }
        case SQ_IN:
        { next_sym(CI);
          t_expr_binary * binex = new t_expr_binary();  if (!binex) c_error(OUT_OF_KERNEL_MEMORY);
          binex->op1=op1;  *pex=binex;
          test_and_next(CI, '(', LEFT_PARENT_EXPECTED);
          if (CI->cursym==S_SELECT)
            subquery_expression(CI, &binex->op2, TRUE);
          else  // process value list
          { t_express * * pdest = &binex->op2;
            t_expr_binary * top = NULL, * bex;
            do
            // List of IN values is linked by a list of binary expr. nodes
            // with oper==0. The 1st of them is top and contains the common
            // supertype of the 1st operand of IN and all IN values.
            { bex = new t_expr_binary();  if (!bex) c_error(OUT_OF_KERNEL_MEMORY);
              *pdest=bex;  pdest=&bex->op2;
              sql_expression(CI, &bex->op1);
              bex->oper.set_pure(PUOP_COMMA);  // used only by dump_expr
              if (top==NULL)
              { top=bex;
                top->type=binex->op1->type;  top->specif=binex->op1->specif;
              }
              add_type(CI, top, bex->op1);
              if (CI->cursym!=',') break;
              next_sym(CI);
            } while (TRUE);
           // add conversions to the common supertype:
            bex=top;
            while (bex!=NULL && bex->sqe==SQE_BINARY && bex->oper.pure_oper==PUOP_COMMA)
            { convert_value(CI, bex->op1, top->type, top->specif);
              bex=(t_expr_binary*)bex->op2;
            }
          }
          binary_conversion(CI, negate ? S_NOT_EQ : '=', binex->op1, binex->op2, binex);
          if (negate) binex->oper.add_all_disp();  else binex->oper.add_any_disp();
          test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
          break;
        }
        case SQ_LIKE:
        { t_expr_ternary * ternex = new t_expr_ternary();  if (!ternex) c_error(OUT_OF_KERNEL_MEMORY);
          ternex->op1=op1;  *pex=ternex;
          next_sym(CI);
          sql_expression(CI, &ternex->op2);
          if (CI->cursym==SQ_ESCAPE)
          { next_sym(CI);
            sql_expression(CI, &ternex->op3);
            if (ternex->op3->type!=ATT_CHAR && !is_string(ternex->op3->type))
              c_error(INCOMPATIBLE_TYPES);
          }
          else // try the like optimization: look for the "abc%" style
          { if (ternex->op2->sqe==SQE_LCONST && !negate)
              if (ternex->op2->type==ATT_CHAR || ternex->op2->type==ATT_STRING)  // types of constants
              { t_expr_lconst * cex = (t_expr_lconst*)ternex->op2;
                tptr val = cex->lval;  int i=0;
                while (val[i] && val[i]!='%' && val[i]!='_') i++;
                if (val[i]=='%' && val[i+1]==0) // can optimize
                { val[i]=0;  cex->valsize--;  // trailing % dropped
                  t_expr_binary * binex = new t_expr_binary();  if (!binex) c_error(OUT_OF_KERNEL_MEMORY);
                  binex->op1=ternex->op1;  ternex->op1=NULL;
                  binex->op2=ternex->op2;  ternex->op2=NULL;
                  *pex=binex;  delete ternex;
                  binary_conversion(CI, S_PREFIX, binex->op1, binex->op2, binex);
                  // converts and checks operand types, writes ATT_BOOLEAN result
                  break;
                }
              }
          }
          binary_conversion(CI, SQ_LIKE, ternex->op1, ternex->op2, ternex);
          if (negate) ternex->oper.negate=1;
          break;
        }
        case S_IS:
        { if (negate) c_error(NOT_NOT_ALLOWED);
          if (op1->type==ATT_BOOLEAN) break;  // processed in sqlbin_test
          if (next_sym(CI)==SQ_NOT) { negate=TRUE;  next_sym(CI); }
          test_and_next(CI, S_NULL, NULL_EXPECTED);
          t_expr_binary * binex = new t_expr_binary();  if (!binex) c_error(OUT_OF_KERNEL_MEMORY);
          binex->oper.set_pure(PUOP_IS_NULL);  binex->oper.negate=negate;
          binex->op1=op1;  *pex=binex;
          binex->op2=new t_expr_null;  // op2 must not be NULL but the value is not used
          if (!binex->op2) c_error(OUT_OF_KERNEL_MEMORY);
          binex->type=ATT_BOOLEAN;  binex->specif.set_null();
          binex->pr_flags = op1->pr_flags | ELFL_NOEDIT;
          // no binary_conversion, proper oper already stored
          break;
        }
        default: // expression with the Boolean type
          if (negate) c_error(NOT_NOT_ALLOWED);
          break;
      }
      break;
    }

  }
}

static void sqlbin_test(CIp_t CI, t_express * * pex, BOOL negate)
{ if (CI->cursym == SQ_NOT) { next_sym(CI);  negate=!negate; }
  sqlbin_predicate(CI, pex); // "cannot sent negate" down, IS may follow
  if (CI->cursym==S_IS)
  { if ((*pex)->type!=ATT_BOOLEAN) c_error(INCOMPATIBLE_TYPES);
    if (next_sym(CI) == SQ_NOT) { next_sym(CI);  negate=!negate; }
   // must handle IS NULL here, not handled in sql_predicate:
    if (CI->cursym!=S_NULL && CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
    t_expr_binary * binex = new t_expr_binary();  if (!binex) c_error(OUT_OF_KERNEL_MEMORY);
    binex->op1=*pex;  *pex=binex;
    if (CI->cursym==S_NULL || !strcmp(CI->curr_name, "UNKNOWN"))
    { binex->oper.set_pure(PUOP_IS_NULL);
      binex->op2 = new t_expr_null;  // op2 must not be NULL but the value is not used
      if (!binex->op2) c_error(OUT_OF_KERNEL_MEMORY);
      binex->oper.negate=negate;
    }
    else 
    { binex->oper.set_num(negate ? PUOP_NE : PUOP_EQ, I_OPBASE);
      wbbool bval;
      if      (!strcmp(CI->curr_name, "TRUE"))  bval=wbtrue;
      else if (!strcmp(CI->curr_name, "FALSE")) bval=wbfalse;  
      else c_error(BOOLEAN_CONSTANT_EXPECTED);
      binex->op2 = new t_expr_sconst(ATT_BOOLEAN, t_specif(), &bval, 1);  if (!binex->op2) c_error(OUT_OF_KERNEL_MEMORY);
      binex->oper.negate=0;
    }
    binex->type=ATT_BOOLEAN;
    binex->specif.set_null();
    binex->pr_flags = (*pex)->pr_flags | ELFL_NOEDIT;
    next_sym(CI);  
  }
  else 
    if (negate) negate_predicate(*pex);
}

//  { if ((*pex)->sqe==SQE_ATTR && (*pex)->type==ATT_BOOLEAN)
//    { t_expr_attr * atex = (t_expr_attr *)*pex;


static void sqlbin_term(CIp_t CI, t_express * * pex, BOOL negate)
{ sqlbin_test(CI, pex, negate);
  while (CI->cursym==SQ_AND)
  { t_expr_binary * binex = new t_expr_binary();  if (!binex) c_error(OUT_OF_KERNEL_MEMORY);
    binex->op1=*pex;  binex->opt_info=OR_OR;  *pex=binex;
    next_sym(CI);
    sqlbin_test(CI, &binex->op2, negate);
    binary_conversion(CI, negate ? SQ_OR : SQ_AND, binex->op1, binex->op2, binex);
  }
}

void sqlbin_expression(CIp_t CI, t_express * * pex, BOOL negate)
{ sqlbin_term(CI, pex, negate);
  while (CI->cursym==SQ_OR)
  { t_expr_binary * binex = new t_expr_binary();  if (!binex) c_error(OUT_OF_KERNEL_MEMORY);
    binex->op1=*pex;  binex->opt_info=OR_OR;  *pex=binex;
    next_sym(CI);
    sqlbin_term(CI, &binex->op2, negate);
    binary_conversion(CI, negate ? SQ_AND : SQ_OR, binex->op1, binex->op2, binex);
  }
}

void search_condition(CIp_t CI, t_express * * pex)
{ sqlbin_expression(CI, pex, FALSE);
  if ((*pex)->type!=ATT_BOOLEAN) c_error(MUST_BE_BOOL);
}
/////////////////////////////////////////////////////////////////////////
CFNC void WINAPI sqlview_compile_entry(CIp_t CI);

void derived_col_list(CIp_t CI, t_query_expr * jtab)
// Renames columns in the context according to the list
{ int i=1;
  do // '(' or ',' is the current symbol
  { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    if (i>=jtab->kont.elems.count()) c_error(SELECT_LISTS_NOT_COMPATIBLE);
    strcpy(jtab->kont.elems.acc0(i)->name, CI->curr_name);
    i++;
  } while (next_sym(CI)==',');
  if (i!=jtab->kont.elems.count()) c_error(SELECT_LISTS_NOT_COMPATIBLE);
  test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
}

void table_reference(CIp_t CI, t_query_expr * * jtable)
{
  if (CI->cursym=='(')
  { if (next_sym(CI)==S_SELECT) // derived table
    { query_expression(CI, jtable);
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);

      if (CI->cursym == S_AS) next_sym(CI);
      if (CI->cursym == S_IDENT && strcmp(CI->curr_name, "LIMIT"))  // LIMIT may be the clause name
      { (*jtable)->kont.set_correlation_name(CI->curr_name);
        next_sym(CI);
      }
      if (CI->cursym=='(') derived_col_list(CI, *jtable);
    }
    else // table reference if parenthesis
    { table_reference(CI, jtable);
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
    }
  }
  else // table name or joined table
  { tobjname tabname, schemaname;  tobjnum objnum;  const t_locdecl * locdecl;  int locdecl_level;
    if (!get_table_name(CI, tabname, schemaname, &objnum, &locdecl, &locdecl_level)) // direct table name
    { if (CI->cdp->limited_login && !SYSTEM_TABLE(objnum) && !replaying)
        c_error(OPERATION_DISABLED);
      t_qe_table * jtab = new t_qe_table(objnum);
      *jtable = jtab;  if (!jtab) c_error(OUT_OF_KERNEL_MEMORY);
      tobjname index_name;  index_name[0]=0;
      if (CI->cursym==S_INDEX)
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        strcpy(index_name, CI->curr_name);
        next_sym(CI);
      }
      if (locdecl==NULL)  // permanent table
      { table_descr_auto td(CI->cdp, objnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
        if (td->me()==NULL) CI->propagate_error();
        jtab->kont.fill_from_td(CI->cdp, td->me());
        jtab->extent = td->level==MAX_TABLE_DESCR_LEVEL ? td->Recnum() : 1000;
       // search the preferred index:
        if (*index_name)
          if (!strcmp(index_name, "FULLTEXT")) jtab->pref_index_num=-2;
          else
          { dd_index * cc;  int ind;
            for (ind=0, cc=td->indxs;  ind<td->indx_cnt;  ind++, cc++)
              if (!strcmp(cc->constr_name, index_name))
                { jtab->pref_index_num=ind;  break; }
          }
      }
      else  // local table
      { const table_all * ta = locdecl->table.ta;
        jtab->kont.fill_from_ta(CI->cdp, ta);
        jtab->extent = 10;  // unknown
        if (*index_name)
        { const ind_descr * indx;  int ind;
          for (ind=0, indx=ta->indxs.acc0(ind);  ind<ta->indxs.count();  ind++, indx++)
            if (!strcmp(indx->constr_name, index_name))
              { jtab->pref_index_num=ind;  break; }
        }
      }
      if (*index_name && jtab->pref_index_num==-1) 
        { SET_ERR_MSG(CI->cdp, index_name);  c_error(INDEX_NOT_FOUND); }
      t_mater_table * mt;
#if SQL3
      jtab->kont.t_mat = mt = new t_mater_table(CI->cdp, TRUE, objnum);  if (!jtab->kont.t_mat) c_error(OUT_OF_KERNEL_MEMORY);
#else
      jtab->kont.mat = mt = new t_mater_table(CI->cdp, TRUE, objnum);  if (!jtab->kont.mat) c_error(OUT_OF_KERNEL_MEMORY);
#endif
      if (locdecl!=NULL)
      { mt->locdecl_offset=locdecl->table.offset;
        mt->locdecl_level=locdecl_level;
        jtab->is_local_table=true;
      }
    }
    else // no such table found, look for a viewed table: 
    { if (!name_find2_obj(CI->cdp, tabname, schemaname, CATEG_CURSOR, &objnum))
      {// nested compilation of the stored cursor:
        tptr buf=ker_load_objdef(CI->cdp, objtab_descr, objnum);
        if (buf==NULL) c_error(OUT_OF_MEMORY);
        t_sql_kont sql_kont;
        compil_info xCI(CI->cdp, buf, sqlview_compile_entry);
        xCI.stmt_ptr=jtable;  xCI.sql_kont=&sql_kont;  *jtable=NULL;
        xCI.recursion_counter = CI->recursion_counter+1;
        if (xCI.recursion_counter > 40) c_error(CURSOR_RECURSION);
        sql_kont.active_scope_list = CI->sql_kont->active_scope_list;  // makes host variables accessible for the stored query, used by parametrized XML exports from stored queries
        WBUUID schema_uuid;  int err;
        if (*schemaname) 
          tb_read_atr(CI->cdp, objtab_descr, objnum, APPL_ID_ATR, (tptr)schema_uuid);
        else memcpy(schema_uuid, CI->cdp->current_schema_uuid, UUID_SIZE);
        { t_short_term_schema_context stsc(CI->cdp, schema_uuid);
          err=compile(&xCI);
        }
        CI->sql_kont->dynpar_referenced|=xCI.sql_kont->dynpar_referenced;
        CI->sql_kont->embvar_referenced|=xCI.sql_kont->embvar_referenced;
        corefree(buf);
        if (err) 
        { if (*jtable!=NULL) { delete *jtable;  *jtable=NULL; }  
          c_error(err); /* propagate error from the inner compilation */
        }
       // change the prefix to the cursor name:
        (*jtable)->kont.set_correlation_name(tabname);
      }
      else // is not a table nor a cursor
      { object * obj = search_obj(tabname); // searches the object
        if (obj!=NULL && !*schemaname && obj->any.categ==OBT_INFOVIEW)  // standard info view found
        { infoviewobj * ivo = &obj->iv;
          t_qe_sysview * jtab = new t_qe_sysview(ivo->ivnum);
          *jtable = jtab;  if (!jtab) c_error(OUT_OF_KERNEL_MEMORY);
         // fill the kontext:
          jtab->kont.fill_from_ivcols(ivo->cols, tabname);
#if SQL3
          jtab->kont.t_mat=NULL;  // table not created yet
#else
          jtab->kont.mat=NULL;  // table not created yet
#endif
          if (ivo->pars!=NULL)
          { test_and_next(CI, '(', LEFT_PARENT_EXPECTED);
            int param_number=0;
            do
            {// process the parameter:
              sql_expression(CI, jtab->args+param_number);
              t_specif dest_specif;  // the default specif for the parameter
              if (ivo->pars[param_number].dtype == ATT_STRING)
              { dest_specif.charset=sys_spec.charset;  // allows proper conversion from Unicode etc.
                dest_specif.length=OBJ_NAME_LEN;  // specific for fulltext wordtab
              }
              convert_value(CI, jtab->args[param_number], ivo->pars[param_number].dtype, dest_specif);
             // go to the next parameter:
              param_number++;
              if (ivo->pars[param_number].categ)
                test_and_next(CI, ',', COMMA_EXPECTED);
              else break;
            } while (true);
            test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
          }
        }
        else
          { SET_ERR_MSG(CI->cdp, tabname);  c_error(TABLE_DOES_NOT_EXIST); }
      }
    }
  }

  if (CI->cursym == S_AS) next_sym(CI);
  if (CI->cursym == S_IDENT && strcmp(CI->curr_name, "LIMIT"))  // LIMIT may be the clause name
  { (*jtable)->kont.set_correlation_name(CI->curr_name);
    next_sym(CI);
  }
  if (CI->cursym=='(') derived_col_list(CI, *jtable);

  while (TRUE) // joining tables to *jtable, storing the result to *jtable
  {// check for the following join: 
    BOOL natural=FALSE, cross=FALSE;  t_qe_types jtype;
    if (CI->cursym==SQ_NATURAL) { natural=TRUE;  next_sym(CI); }
    switch (CI->cursym)
    { case SQ_CROSS: jtype=QE_JT_INNER;  cross=TRUE;  next_sym(CI);  goto join;
      case SQ_INNER: jtype=QE_JT_INNER;               next_sym(CI);  goto join;
  //    case S_UNION:  jtype=QE_JT_UNION;               next_sym(CI);  goto join;
  // conflicts with normal UNION, makes the grammar LL(2)
      case S_LEFT:   jtype=QE_JT_LEFT;   goto outer;
      case S_RIGHT:  jtype=QE_JT_RIGHT;  goto outer;
      case SQ_FULL:  jtype=QE_JT_FULL;
       outer:
        if (next_sym(CI)==S_OUTER) next_sym(CI);  // OUTER is optional
       join:
        test_and_next(CI, S_JOIN, JOIN_EXPECTED);
        break;
      case S_JOIN:   jtype=QE_JT_INNER;               next_sym(CI);  break;
      default:       jtype=QE_TABLE;  // no join, stop it
    }

    if (jtype==QE_TABLE) break;  // no join found

   // compiling the join:
    t_qe_join * jjtab = new t_qe_join(jtype);  if (!jjtab) c_error(OUT_OF_KERNEL_MEMORY);
    jjtab->op1=*jtable;  *jtable=jjtab;
    jjtab->join_cond=NULL;
    jjtab->kont.init_curs(CI);  // init kontext of the result
    int common_cols = 1;      // no other common columns so far
    table_reference(CI, &jjtab->op2);
    jjtab->qe_oper = QE_IS_UPDATABLE & (jjtab->op1->qe_oper | jjtab->op2->qe_oper);
#ifdef STABILIZED_GROUPING
    jjtab->maters = MAT_NO;
#else
    jjtab->maters = jjtab->op2->maters!=MAT_NO ? MAT_INSTABLE :
                    jjtab->op1->maters!=MAT_NO ? MAT_INSTABLE : MAT_NO;
#endif
    db_kontext * k1 = &jjtab->op1->kont, * k2 = &jjtab->op2->kont;  int i, j;

    if (natural)
    {// search the (single) common column:
      elem_descr * el1, * el2;  BOOL found=FALSE;
      for (i=1, el1=k1->elems.acc0(1);  i<k1->elems.count();  i++, el1++)
        for (j=1, el2=k2->elems.acc0(1);  j<k2->elems.count();  j++, el2++)
          if (!strcmp(el1->name, el2->name)) { found=TRUE;  goto fnd; }
     fnd:
      if (found) // a common column found (other not searched)
      { elem_descr * el = jjtab->kont.copy_elem(CI, i, k1, 0);
        common_cols++;
        el->pr_flags &= el2->pr_flags;
        t_expr_binary * ex = new t_expr_binary();  jjtab->join_cond=ex;  if (!ex) c_error(OUT_OF_KERNEL_MEMORY);
        ex->op1=new t_expr_attr(i, k1);  if (!ex->op1) c_error(OUT_OF_KERNEL_MEMORY);
        ex->op2=new t_expr_attr(j, k2);  if (!ex->op2) c_error(OUT_OF_KERNEL_MEMORY);
        binary_conversion(CI, '=', ex->op1, ex->op2, ex);
      }
      else c_error(NO_COMMON_COLUMN_NAME);
    }
    else if (!cross) // analyse the join specification
    { if (CI->cursym==S_ON && jtype!=QE_JT_UNION) // join condition specified
      { next_sym(CI);
       // compile the join condition in the new compilation kontext (add k1, k2 kontexts):
        temp_sql_kont_add tk2(CI->sql_kont, k2);
        temp_sql_kont_add tk1(CI->sql_kont, k1);
        search_condition(CI, &jjtab->join_cond);
      }
      else if (CI->cursym==SQ_USING) // named columns list specified
      { t_expr_binary * ex1=NULL, * ex2;
        next_and_test(CI, '(', LEFT_PARENT_EXPECTED);
        do
        { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
         // add to kontext:
          int elemnum1, elemnum2;  elem_descr * el1, * el2, * el;
          el1 = k1->find(NULL, CI->curr_name, elemnum1);
          if (el1==NULL) { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(ATTRIBUTE_NOT_FOUND); }
          el2 = k2->find(NULL, CI->curr_name, elemnum2);
          if (el2==NULL) { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(ATTRIBUTE_NOT_FOUND); }
          el = jjtab->kont.copy_elem(CI, elemnum1, k1, 0);
          // this is not according to the ANSI: table name prefix should have been removed because prefixing the 
          //   column name by the name of the 1st table is illegal. Preserving for the compatibility with the older versions.
          el->pr_flags &= el2->pr_flags;
          common_cols++;
         // create condition:
          ex2=new t_expr_binary();  // insert column references
          if (!ex2) c_error(OUT_OF_KERNEL_MEMORY);
          ex2->op1=new t_expr_attr(elemnum1, k1);  if (!ex2->op1) c_error(OUT_OF_KERNEL_MEMORY);
          ex2->op2=new t_expr_attr(elemnum2, k2);  if (!ex2->op2) c_error(OUT_OF_KERNEL_MEMORY);
          binary_conversion(CI, '=', ex2->op1, ex2->op2, ex2);
          if (ex1!=NULL) // AND with the previous column
          { t_expr_binary * ex3 = new t_expr_binary();  if (!ex3) c_error(OUT_OF_KERNEL_MEMORY);
            ex3->op1=ex1;  ex3->op2=ex2;
            binary_conversion(CI, SQ_AND, ex3->op1, ex3->op2, ex3);
            ex1=ex3;
          }
          else ex1=ex2;
          next_sym(CI);
        } while (CI->cursym==',');
        jjtab->join_cond=ex1;
        test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
      }

    } // join specification
   // check join condition reference privilege:
    if (jjtab->join_cond!=NULL)
      jjtab->join_cond->is_referenced();
   // complete the joined kontext (add all columns except the common):
    for (i=1;  i<k1->elems.count();  i++)
      jjtab->kont.copy_elem(CI, i, k1, common_cols);
    for (i=1;  i<k2->elems.count();  i++)
      jjtab->kont.copy_elem(CI, i, k2, common_cols);
   // possible renaming the join result:
    if (CI->cursym == S_AS) next_sym(CI);
    if (CI->cursym == S_IDENT)
    { jjtab->kont.set_correlation_name(CI->curr_name);
      next_sym(CI);
    }
    if (CI->cursym=='(') derived_col_list(CI, jjtab);
  } // joined table
}

static void compile_from_clause(CIp_t CI, t_query_expr * * jtables)
// FROM is the current symbol on entry
// The top table will have the kontext, leaves too, innner tables not
{ t_query_expr * jtable;  db_kontext from_kont;  
  t_qe_join * jtjoin = NULL;
  BOOL any_join=FALSE, mater_in_1st=FALSE, mater_other=FALSE;
  from_kont.init_curs(CI);
  t_query_expr * * dest_jt = jtables;
  int result_qe_oper = QE_IS_DELETABLE | QE_IS_INSERTABLE;
  do // INV: result is to be assigned to dest_jt
  { next_sym(CI);  // skips FROM or comma
    table_reference(CI, dest_jt);
    jtable=*dest_jt;
    if (  jtable->qe_oper & QE_IS_UPDATABLE)   result_qe_oper |=  QE_IS_UPDATABLE;
    if (!(jtable->qe_oper & QE_IS_DELETABLE))  result_qe_oper &= ~QE_IS_DELETABLE;
    if (!(jtable->qe_oper & QE_IS_INSERTABLE)) result_qe_oper &= ~QE_IS_INSERTABLE;
    if (jtable->maters!=MAT_NO)
      if (dest_jt == jtables) mater_in_1st=TRUE;  else mater_other=TRUE;
   // copy kontext items to the top FROM kontext
    from_kont.copy_kont(CI, &jtable->kont, 1);
   // continue processing the FROM list:
    if (CI->cursym!=',') break; // the last element in the list
   // another join:
    jtjoin = new t_qe_join(QE_JT_INNER);  if (!jtjoin) c_error(OUT_OF_KERNEL_MEMORY);
    jtjoin->op1=jtable;
    *dest_jt=jtjoin;  dest_jt=&jtjoin->op2;
    result_qe_oper &= ~(QE_IS_DELETABLE | QE_IS_INSERTABLE);  // any join removes this
    any_join=TRUE;
  } while (TRUE);

 // complete the kontext of the result
  jtable=*jtables;  // the top
  if (!jtable->kont.elems.count())
  { jtable->kont=from_kont;
    memset(&from_kont, 0, sizeof(db_kontext));
  }
  // otherwise from_kont discarted, kontext from jtable used
 // define qe_oper and maters:
  jtable->qe_oper = result_qe_oper;
#ifndef STABILIZED_GROUPING
  if (any_join)
    if      (mater_other ) jtable->maters=MAT_INSTABLE;
    else if (mater_in_1st) jtable->maters=MAT_INSTABLE;
#endif
 // check what follows the FROM clause (otherwise rubbish after FROM cluase not detected)
  if (CI->cursym && CI->cursym!=')' && CI->cursym!=S_WHERE && CI->cursym!=S_GROUP &&
      CI->cursym!=S_HAVING  && CI->cursym!=S_ORDER   &&
      CI->cursym!=S_UNION   && CI->cursym!=SQ_EXCEPT &&
      CI->cursym!=SQ_INTERS && CI->cursym!=SQ_DO &&
      !CI->is_ident("LIMIT") &&
      CI->cursym!=SQ_FOR    && CI->cursym!=';') c_error(COMMA_EXPECTED);
}
//////////////////// query specification and expression //////////////////////
void dd_index::compile(CIp_t CI)
// Compiles the index expression. Used in table definition and in ORDER BY, GROUP BY clauses
{ keysize=sizeof(trecnum); /* this is valid for a table only, not for cursor */
  partnum=0;
  do
  { if (!add_index_part()) c_error(OUT_OF_KERNEL_MEMORY);
    part_desc * apart = &parts[partnum++];  // it's better to update partnum continually, on error the proper parts can be freed
    sql_expression(CI, &apart->expr);
    t_express * ex = apart->expr;
   // in ORDER BY replace integer constant by reference to result column (not used in indicies):
    if (CI->sql_kont->phase == SQLPH_ORDER)
      if (ex->sqe==SQE_SCONST && ex->type==ATT_INT32 && ex->specif.scale==0)
      { int column = *(sig32*)((t_expr_sconst*)ex)->val;
        if (column>0 && column < CI->sql_kont->active_kontext_list->elems.count())
        { t_expr_attr * atex = new t_expr_attr(column, CI->sql_kont->active_kontext_list);
          if (!atex) c_error(OUT_OF_KERNEL_MEMORY);
          delete ex;  apart->expr=ex=atex;
        }
      }
   // take information about the index part and store it into [apart]:
    if (ex->type==ATT_DOMAIN)  // translate the domain
    { t_type_specif_info tsi;
      if (!compile_domain_to_type(CI->cdp, ex->specif.domain_num, tsi))
        c_error(CI->cdp->get_return_code());
      ex->type=tsi.tp;  ex->specif=tsi.specif;
    }
    int tp=ex->type;
    if ((tp & 0x80) || IS_HEAP_TYPE(tp) || IS_EXTLOB_TYPE(tp) || tp==ATT_DATIM || tp==ATT_AUTOR)
      c_error(BAD_TYPE_IN_INDEX);
    apart->type    =tp;
    apart->partsize=SPECIFLEN(tp) ? ex->specif.length : tpsize[tp];
    apart->specif  =ex->specif;
    if (apart->partsize > MAX_INDPARTVAL_LEN) 
    { apart->partsize = MAX_INDPARTVAL_LEN;
      apart->specif.length = MAX_INDPARTVAL_LEN;  // used in is_null_key()
    }
    apart->colnum  = ex->sqe==SQE_ATTR ? ((t_expr_attr*)ex)->elemnum : 0;
    if (CI->cursym==S_DESC)
    { apart->desc=wbtrue;  next_sym(CI); }
    else
    { apart->desc=wbfalse;
      if (CI->cursym==S_ASC) next_sym(CI);
    }
    keysize+=apart->partsize;
   // go to the next part or stop:
    if (CI->cursym!=',') break;  // no more parts
    next_sym(CI);
  } while (TRUE);
  if (keysize>MAX_KEY_LEN) c_error(INDEX_KEY_TOO_LONG);
}

static void query_specification(CIp_t CI, t_query_expr * * pqs)
// Compiles single SELECT ...
// Must have single exit restoring the variouns kontext entries
{ t_qe_specif * qs = new t_qe_specif();  *pqs=qs;  if (!qs) c_error(OUT_OF_KERNEL_MEMORY);
  BOOL instability=FALSE;

 // compile the FROM clause:
  CIpos pos(CI);     // save compil. position
  int paren_count=0;
  do // skip SELECT and INTO clauses
  { next_sym(CI);
    if      (CI->cursym=='(') paren_count++;
    else if (CI->cursym==')') paren_count--;
  } while ((CI->cursym!=S_FROM || paren_count) && CI->cursym && CI->cursym!=';');
  //if (!CI->cursym) c_error(FROM_OMITTED);  -- extension
  if (CI->cursym==S_FROM)
    compile_from_clause(CI, &qs->from_tables);
  CI->sql_kont->min_max_optimisation_enabled = CI->cursym!=S_WHERE && qs->from_tables && qs->from_tables->qe_type==QE_TABLE;
  pos.restore(CI);   // restore the saved position
  next_sym(CI); // SELECT skipped

 // activate the new compilation kontext for the rest of the statement:
  temp_sql_kont_add tk(CI->sql_kont, qs->from_tables ? &qs->from_tables->kont : NULL);
 // modifying the compilation kontext, will be restored on exit:
  t_comp_phase upper_phase      = CI->sql_kont->phase;
  CI->sql_kont->phase    = SQLPH_SELECT;
  t_agr_dynar  * upper_agr_list = CI->sql_kont->agr_list;
  CI->sql_kont->agr_list = &qs->agr_list;
  t_query_expr * upper_qe       = CI->sql_kont->curr_qe;
  CI->sql_kont->curr_qe  = qs;

 /* now, analyse SELECT clause: */
  qs->user_order=qs->distinct=false;
  if (CI->cursym == S_USER)
    { qs->user_order=true;  next_sym(CI); }
  if (CI->cursym == S_DISTINCT)
    { qs->distinct=true;  next_sym(CI); }
  else if (CI->cursym == S_ALL) next_sym(CI);


 /* Adding "DELETED" attribute: when subcursor of a table is created,
    attributes must preserve its numbers. */

  if (CI->cursym=='*' && qs->from_tables)  // same kontext as below
  { next_sym(CI);
    bool use_syscols = (CI->cdp->sqloptions & SQLOPT_USE_SYS_COLS) != 0;
    if (CI->cursym=='*') { use_syscols=true;  next_sym(CI); }  // ** used in subqueries which must take all columns
    if (use_syscols)
      qs->kont.copy_kont(CI, &qs->from_tables->kont, 0);
    else  // without system columns
    { qs->kont.copy_elem(CI, 0, &qs->from_tables->kont, 0);
      qs->kont.copy_kont(CI, &qs->from_tables->kont, qs->from_tables->kont.first_nonsys_col());
    }
    if (CI->cursym==',') c_error(OTHER_COLS_NOT_ALLOWED);
  }
  else
  { qs->kont.init_curs(CI);
    do   /* cycle on SELECT elements: */
    { BOOL asterisk = FALSE;
      elem_descr * el = qs->kont.elems.next();
      if (el==NULL) c_error(OUT_OF_MEMORY);
      el->link_type=ELK_EXPR; // el->expr.eval will hold the comiled expression
      BOOL upper_aggr_in_expr=CI->sql_kont->aggr_in_expr;
      BOOL upper_instable_var=CI->sql_kont->instable_var;
      CI->sql_kont->aggr_in_expr=FALSE; // "aggregate function contained" flag
      CI->sql_kont->instable_var=FALSE; // "instable system variable contained" flag
      sqlbin_expression(CI, &el->expr.eval, FALSE);
      if (CI->sql_kont->aggr_in_expr)
        if (qs->grouping_type==GRP_NO) qs->grouping_type=GRP_SINGLE;
     // analyse the expression, replace by column link if possible:
      t_express * ex = el->expr.eval;
      if (ex->sqe==SQE_ATTR && ((t_expr_attr *)ex)->indexexpr==NULL && ex->convert.conv==CONV_NO) // can replace expression by attribute link
      { t_expr_attr * exat = (t_expr_attr *)ex;
        if (exat->elemnum==-1)  // tabname.* specified
        { db_kontext * from_kont = exat->kont;  tobjname pref;  
          strcpy(pref, elem_prefix(from_kont, from_kont->elems.acc0(exat->specif.opqval)));
          qs->kont.elems.delet(qs->kont.elems.count()-1);
          for (int i=1;  i<from_kont->elems.count();  i++)
            if (!strcmp(pref, elem_prefix(from_kont, from_kont->elems.acc0(i))))
              if ((CI->cdp->sqloptions & SQLOPT_USE_SYS_COLS) ||
                  !IS_SYSTEM_COLUMN(from_kont->elems.acc0(i)->name)) // unless system
              qs->kont.copy_elem(CI, i, from_kont, 0);
          asterisk=TRUE;
        }
        else // link to a column from another kontext
          el->store_kontext(exat->kont->elems.acc0(exat->elemnum), exat->kont, exat->elemnum);
        delete ex;
      }
      else // real expression, take column properties from expression properties:
      { if (ex->sqe!=SQE_NULL && ex->type==0)  // procedure called
          { SET_ERR_MSG(CI->cdp, "<procedure>");  c_error(BAD_IDENT_CLASS); }
        if (CI->cdp->sqloptions & SQLOPT_DISABLE_SCALED)
          if (ex->type==ATT_INT8 || ex->type==ATT_INT16 || ex->type==ATT_INT32 || ex->type==ATT_INT64)
            if (ex->specif.scale!=0)
              if (ex->specif.scale==2)
                convert_value(CI, ex, ATT_MONEY, 2);
              else
                convert_value(CI, ex, ATT_FLOAT, 0);
        el->type    =ex->type & 0x7f;
        el->multi   =ex->type & 0x80 ? 2 : 1; // 2 is impossible, I think
        el->specif  =ex->specif;
        el->nullable=2;
        el->pr_flags=ex->pr_flags;
        if (ex->sqe!=SQE_EMBED && ex->sqe!=SQE_VAR) el->pr_flags |= ELFL_NOEDIT;
        else el->pr_flags &= ~ELFL_NOEDIT;
        if (ex->sqe==SQE_AGGR)
        { tptr patt;  int ind;
          switch (((t_expr_aggr *)ex)->ag_type)
          { case S_SUM:   patt="SUM%u";    ind=0;  break;
            case S_MIN:   patt="MIN%u";    ind=1;  break;
            case S_MAX:   patt="MAX%u";    ind=2;  break;
            case S_AVG:   patt="AVG%u";    ind=3;  break;
            case S_COUNT: patt="COUNT%u";  ind=4;  break;
          }
          sprintf(el->name, patt, CI->sql_kont->aggr_counter[ind]++);
        }
        else
        { sprintf(el->name, "EXPR%u", CI->sql_kont->expr_counter++);
          if (CI->sql_kont->aggr_in_expr) el->link_type=ELK_AGGREXPR;
        }
        if (CI->sql_kont->instable_var) instability=TRUE;
      }
      CI->sql_kont->instable_var=upper_instable_var;
      CI->sql_kont->aggr_in_expr=upper_aggr_in_expr;  // SELECT in SELECT clause needs resetting this flag!
     // AS clause:
      if (CI->cursym==S_AS && !asterisk)
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        strcpy(el->name, CI->curr_name);
        next_sym(CI);
      }
      if (CI->cursym==',') next_sym(CI); else break;
    } while (true);   /* cycle on SELECT elements */
  }
 // limit specif to MAX_FIXED_STRING_LENGTH, remove duplicities:
  t_name_hash name_hash(qs->kont.elems.count());
  for (int i=1;  i<qs->kont.elems.count();  i++)
  { elem_descr * el = qs->kont.elems.acc0(i);
    if (is_string(el->type) || el->type==ATT_BINARY)
      if (el->specif.length > MAX_FIXED_STRING_LENGTH) el->specif.length=MAX_FIXED_STRING_LENGTH;
   // rename duplicate column names:
    if (!(CI->cdp->sqloptions & SQLOPT_DUPLIC_COLNAMES))
    { unsigned counter=2;
      int pos=(int)strlen(el->name);
      if (pos+4>OBJ_NAME_LEN) pos=OBJ_NAME_LEN-4;
      while (!name_hash.add(el->name))  // duplicity, must try other name
        int2str(counter++, el->name+pos);
    }
  }

  CI->sql_kont->phase = upper_phase;  // temporary restoring
  CI->sql_kont->min_max_optimisation_enabled=FALSE;

  if (CI->cursym==S_INTO)
  { // remove the DB context when compiling the INTO clause, avoiding conflict of names between columns and variables:
    // db_kontext * db_kont = CI->sql_kont->active_kontext_list;  CI->sql_kont->active_kontext_list = NULL;
    // no more, better solution integrated into sql_selector.
    int elnum=0;
    do
    { next_sym(CI);
      if (++elnum >= qs->kont.elems.count()) break;
      t_assgn * assgn = qs->into.next();
      if (assgn==NULL) c_error(OUT_OF_MEMORY);
      sql_selector(CI, &assgn->dest, false, true);
      assgn->source=new t_expr_attr(elnum, &qs->kont);  if (!assgn->source) c_error(OUT_OF_KERNEL_MEMORY);
      if (assgn->dest->sqe==SQE_DYNPAR)
        supply_marker_type(CI, (t_expr_dynpar*)assgn->dest, assgn->source->type, assgn->source->specif, MODE_OUT);
      else
      { convert_value(CI, assgn->source, assgn->dest->type, assgn->dest->specif);
        if (assgn->dest->sqe!=SQE_VAR && assgn->dest->sqe!=SQE_EMBED && assgn->dest->sqe!=SQE_ATTR) // ATTR added in Wb 6.0c, usefull in triggers
          c_error(BAD_INTO_OBJECT);
      }
    } while (CI->cursym==',');
    //CI->sql_kont->active_kontext_list = db_kont;
  }

 /* skip the FROM clause: */
  if (CI->cursym==S_FROM) 
  { paren_count=0;
    do // skip FROM clause
    { next_sym(CI);
      if      (CI->cursym=='(') paren_count++;
      else if (CI->cursym==')')
        if (paren_count) paren_count--; else break;
      else if (!CI->cursym) break;
    } while (paren_count || CI->cursym!=S_WHERE && CI->cursym!=S_GROUP &&
         CI->cursym!=S_HAVING  && CI->cursym!=S_ORDER   && !CI->is_ident("LIMIT") &&
         CI->cursym!=S_UNION   && CI->cursym!=SQ_EXCEPT &&
         CI->cursym!=SQ_INTERS && CI->cursym!=SQ_DO && CI->cursym!=SQ_FOR && CI->cursym!=';');
  }
  if (CI->cursym==S_WHERE)   /* analyse the WHERE clause */
  { next_sym(CI);
    CI->sql_kont->phase = SQLPH_WHERE;
    search_condition(CI, &qs->where_expr);
    qs->where_expr->is_referenced();
  }

  if (CI->cursym==S_GROUP)   /* analyse the GROUP clause */
  { next_and_test(CI, S_BY, BY_EXPECTED);
    next_sym(CI);
    qs->grouping_type=GRP_BY;
    CI->sql_kont->phase = SQLPH_GROUP;
    qs->group_indx.compile(CI);
    int i;
    for (i=0;  i<qs->group_indx.partnum;  i++)
      qs->group_indx.parts[i].expr->is_referenced();
   // make non-editable:
    for (i=0;  i<qs->kont.elems.count();  i++)
      qs->kont.elems.acc0(i)->pr_flags |= ELFL_NOEDIT;
  }

  if (CI->cursym==S_HAVING)  /* analyse the HAVING clause */
  { next_sym(CI);
    CI->sql_kont->phase = SQLPH_HAVING;
    if (qs->grouping_type==GRP_NO) c_error(FEATURE_NOT_SUPPORTED);
      // qs->grouping_type=GRP_HAVING;
    search_condition(CI, &qs->having_expr);
    qs->having_expr->is_referenced();
  }
 // define qe_oper, maters:
  if (qs->grouping_type==GRP_NO)
  { if (qs->from_tables)
    { qs->qe_oper = qs->from_tables->qe_oper;
      qs->maters = qs->from_tables->maters==MAT_HERE ? MAT_HERE2 : qs->from_tables->maters;
      if (instability && qs->maters<MAT_INSTABLE) qs->maters=MAT_INSTABLE;
    }
    else 
    { qs->qe_oper = 0;
      qs->maters = MAT_NO;
    } 
  }
  else
  { qs->qe_oper = 0;  // QE_IS_DELETABLE; deletability removed in 8.0c for better synchro with form desing tools
    qs->maters = MAT_HERE;
#if SQL3
    qs->kont.grouping_here=true;
#endif
  }

 // return to the original kontext:
  CI->sql_kont->phase    = upper_phase;
  CI->sql_kont->curr_qe  = upper_qe;
  CI->sql_kont->agr_list = upper_agr_list;
}

///////////////////////// query expression ///////////////////////////////////
// if CORRESPONDING specified, column names from the column list are copied into qe kontext (names
// only). Later the names are searched in kontexts of both operands.

void t_qe_bin::read_all_and_corresp(CIp_t CI)
// Processes optional ALL and <correspoding spec>, reads the column names and copies them
// into the qe kontext.
{ if (CI->cursym==S_ALL) { all_spec=TRUE;  next_sym(CI); }
  else all_spec=FALSE;

  if (CI->cursym==SQ_CORRESPONDING)
  { corr_spec=TRUE;
    if (next_sym(CI)==S_BY)
    { next_and_test(CI, '(', LEFT_PARENT_EXPECTED);
      kont.init_curs(CI);
      do
      { if (next_sym(CI)!=S_IDENT) c_error(IDENT_EXPECTED);
        elem_descr * cc=kont.elems.next();
        if (cc==NULL) c_error(OUT_OF_MEMORY);
        strcpy(cc->name, CI->curr_name);
      } while (next_sym(CI)==',');
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
    }
  }
  else corr_spec=FALSE;
}

static bool unionable_columns(elem_descr * el1, elem_descr * el2)
// Checks to see if two columns can be joined in UNION, EXCEPT or INTERSECT.
// History columns can be joined always, structure not checked.
{ 
 // allow unioning NULL with a column, but must convert it, otherwise read_record is confused when reading 0-length column value
  if (!el1->type && el1->link_type==ELK_EXPR && el2->type && el2->multi==1) 
  { el1->expr.eval->convert.conv=CONV_NULL;
    el1->expr.eval->type=el1->type=el2->type;
    el1->expr.eval->specif=el1->specif=el2->specif;
    el1->multi=1;
    return true;
  }
  if (!el2->type && el2->link_type==ELK_EXPR && el1->type && el1->multi==1) 
  { el2->expr.eval->convert.conv=CONV_NULL;
    el2->expr.eval->type=el2->type=el1->type;
    el2->expr.eval->specif=el2->specif=el1->specif;
    el2->multi=1;
    return true;
  }
  return el1->type==el2->type && el1->multi==el2->multi && el1->specif==el2->specif; 
}

void t_qe_bin::process_corresp(CIp_t CI, bool is_union)
// Completes the result kontext of the query expression according to the
// CORRESPONDING specification and operand kontexts. Uses ELK_KONT2x.
//  positions of the corr. columns not stored...
{ elem_descr * el, * el1, * el2;  int i, elemnum1, elemnum2;
  db_kontext * k1 = &op1->kont, * k2 = &op2->kont;

  if (corr_spec)  // CORRESPONDING specified
    if (kont.elems.count()) // BY (...) specified, check the specified columns, add info
    { for (i=1, el=kont.elems.acc0(1);  i<kont.elems.count();
           i++, el++)
      { el1 = k1->find(NULL, el->name, elemnum1);
        el2 = k2->find(NULL, el->name, elemnum2);
        if (el1==NULL || el2==NULL)
          { SET_ERR_MSG(CI->cdp, el->name);  c_error(ATTRIBUTE_NOT_FOUND); }
        if (!unionable_columns(el1, el2)) c_error(SELECT_LISTS_NOT_COMPATIBLE);
        el->store_double_kontext(el1, el2, k1, k2, elemnum1, elemnum2, is_union);
      }
    }
    else // must find corresponding columns by names
    { kont.init_curs(CI);
      for (i=1, el1=k1->elems.acc0(1);  i<k1->elems.count();
           i++, el1++)
      { el2 = k2->find(NULL, el1->name, elemnum2);
        if (el2)  // column with the same name found
        { if (!unionable_columns(el1, el2)) c_error(SELECT_LISTS_NOT_COMPATIBLE);
          el=kont.elems.next();  if (el==NULL) c_error(OUT_OF_MEMORY);
          el->store_double_kontext(el1, el2, k1, k2, i, elemnum2, is_union);
        }
      }
      if (kont.elems.count() == 1) // no common column found
        c_error(NO_COMMON_COLUMN_NAME);
    }
  else // all columns are supposed to be corresponding (aggregate auxiliary columns not added yet)
  { if (k1->elems.count() != k2->elems.count())
      c_error(SELECT_LISTS_NOT_COMPATIBLE);
    kont.init_curs(CI);
    for (i=1, el1=k1->elems.acc0(1), el2=k2->elems.acc0(1);  i<k1->elems.count();
         i++, el1++,                 el2++)
    { if (!unionable_columns(el1, el2)) c_error(SELECT_LISTS_NOT_COMPATIBLE);
      el=kont.elems.next();  if (el==NULL) c_error(OUT_OF_MEMORY);
      el->store_double_kontext(el1, el2, k1, k2, i, i, is_union);
    }
  }
}

static void query_primary(CIp_t CI, t_query_expr * * qe)
{ if (CI->cursym=='(')
  { next_sym(CI);
    query_expression(CI, qe);
    test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
  }
  else if (CI->cursym==S_SELECT)
    query_specification(CI, qe);
  else if (CI->cursym==SQ_TABLE)
  { next_sym(CI);
    table_reference(CI, qe);
  }
  else
    table_reference(CI, qe);
}

static void query_term(CIp_t CI, t_query_expr * * qe)
{ query_primary(CI, qe);
  while (CI->cursym==SQ_INTERS)
  { t_qe_bin * qet;
    qet = new t_qe_bin(QE_INTERS);  if (!qet) c_error(OUT_OF_KERNEL_MEMORY);
    qet->op1=*qe;  *qe=qet;
    next_sym(CI);
    qet->read_all_and_corresp(CI);
    query_primary(CI, &qet->op2);
    qet->qe_oper = (QE_IS_UPDATABLE | QE_IS_DELETABLE) & qet->op1->qe_oper;  // 602 extension
    qet->maters = qet->op1->maters > qet->op2->maters ? qet->op1->maters : qet->op2->maters;
    if (qet->maters==MAT_HERE || qet->maters==MAT_HERE2) qet->maters=MAT_BELOW;
    qet->process_corresp(CI, false);
  }
}

static void query_expression(CIp_t CI, t_query_expr * * qe)
// Compiles a query expression followed by ORDER BY clause into qe.
{ query_term(CI, qe);
  while (CI->cursym==S_UNION || CI->cursym==SQ_EXCEPT)
  { bool is_union = CI->cursym==S_UNION;
    t_qe_bin * qet;
    qet = new t_qe_bin(is_union ? QE_UNION : QE_EXCEPT);  if (!qet) c_error(OUT_OF_KERNEL_MEMORY);
    qet->op1=*qe;  *qe=qet;
    next_sym(CI);
    qet->read_all_and_corresp(CI);
    query_term(CI, &qet->op2);
    qet->qe_oper = is_union ? 0 : (QE_IS_UPDATABLE | QE_IS_DELETABLE) & qet->op1->qe_oper;
    qet->maters = qet->op1->maters > qet->op2->maters ? qet->op1->maters : qet->op2->maters;
    if (qet->maters==MAT_HERE || qet->maters==MAT_HERE2) qet->maters=MAT_BELOW;
    qet->process_corresp(CI, is_union);
  }

 // ORDER BY:  -- shall be in sql_view, this is a WB extension
  if (CI->cursym==S_ORDER)   /* analyse the ORDER BY clause */
  { next_and_test(CI, S_BY, BY_EXPECTED);  next_sym(CI);
    {// find kontext for ORDER BY compilation: qe kontext or direct wider
      db_kontext * order_kont = &(*qe)->kont;  
      if (CI->cursym!=S_INTEGER && // not excact, but must not use upper kontext when compiling integer order
          ((*qe)->qe_type!=QE_SPECIF || ((t_qe_specif*)(*qe))->grouping_type==GRP_NO))  // must not use upper kontext when grouping
      { BOOL wider_ok=TRUE;  db_kontext * wider_kont = NULL;
        for (int i=1;  wider_ok && i<order_kont->elems.count();  i++)
        { elem_descr * el = order_kont->elems.acc0(i);
          if (el->link_type==ELK_KONTEXT && !strcmp(el->name, el->kontext.kont->elems.acc0(el->kontext.down_elemnum)->name))
          { if (wider_kont==NULL) wider_kont=el->kontext.kont;
            else if (wider_kont!=el->kontext.kont) wider_ok=FALSE;
          }
          else wider_ok=FALSE;
        }
        if (wider_ok) order_kont=wider_kont;
      }
      temp_sql_kont_replace tk(CI->sql_kont, order_kont);  // sets temporary compilation kontext
      t_comp_phase upper_phase       = CI->sql_kont->phase;
      CI->sql_kont->phase            = SQLPH_ORDER;
     // compile the expressions inside ORDER BY clause:
      (*qe)->order_by = new t_order_by;  if (!(*qe)->order_by) c_error(OUT_OF_KERNEL_MEMORY);
      (*qe)->order_by->order_indx.compile(CI);
     // restore the original kontext:
      CI->sql_kont->phase            = upper_phase;
    }
   // check privileges (must be full-table privilege), check for system vars in order_indx:
    for (int i=0;  i<(*qe)->order_by->order_indx.partnum;  i++)
    { (*qe)->order_by->order_indx.parts[i].expr->is_referenced();
      if ((*qe)->order_by->order_indx.parts[i].expr->sqe==SQE_VAR)
      { t_expr_var * varex = (t_expr_var*)(*qe)->order_by->order_indx.parts[i].expr;
        if (varex->level==SYSTEM_VAR_LEVEL && (*qe)->maters<=MAT_INSTABLE)
          (*qe)->maters=MAT_INSTABLE;
      }
    }
  }
  if (CI->is_ident("LIMIT"))
  { next_sym(CI);
    sql_expression(CI, &(*qe)->limit_offset);
    if (!is_integer((*qe)->limit_offset->type, (*qe)->limit_offset->specif)) c_error(MUST_BE_INTEGER);
    if (CI->cursym==',') // specified offset and count
    { next_sym(CI);
      sql_expression(CI, &(*qe)->limit_count);
      if (!is_integer((*qe)->limit_count->type, (*qe)->limit_count->specif)) c_error(MUST_BE_INTEGER);
    }
    else  // only count specified
    { (*qe)->limit_count=(*qe)->limit_offset;  
      (*qe)->limit_offset=NULL;
    }
  }
}

t_query_expr::~t_query_expr(void)
{ if (order_by!=NULL) delete order_by; 
  if (limit_offset) delete limit_offset;
  if (limit_count ) delete limit_count;
}

t_expr_subquery::~t_expr_subquery(void)
{ if (opt    !=NULL) opt->Release();  // opt may have links to qe
  if (qe     !=NULL) qe->Release();
  if (rowdist!=NULL) delete rowdist;
  corefree(kontpos);
  DeleteCriticalSection(&cs);
}

void t_expr_subquery::mark_core(void)
{ t_express::mark_core();
  if (qe)  qe ->mark_core();
  if (opt) opt->mark_core();
  if (rowdist) 
    { rowdist->mark_core();  mark(rowdist); }
  if (kontpos) mark(kontpos);
  val.mark_core();
}

void t_expr_subquery::mark_disk_space(cdp_t cdp) const
{ if (qe)      qe     ->mark_disk_space(cdp); 
  if (opt)     opt    ->mark_disk_space(cdp); 
  if (rowdist) rowdist->mark_disk_space(cdp); 
}

////////////////////// query compilation entries /////////////////////////////
void top_query_expression(CIp_t CI, t_query_expr * * qe)
// Compiles the query without cursor decoration, used in subqueries and inside the decorated cursors.
{ query_expression(CI, qe);
 // assign numbers in tuples:
 // The beginning of optimize_qe() would be generally a better plate for calling this, but
 //   t_ins_upd_trig_supp::register_column() may need kont.ordnum before the optimisation.
 //   So is is called here even for views referenced in queries and in is useless.
  int kont_cnt = 0;
  (*qe)->write_kont_list(kont_cnt, TRUE);
  if (kont_cnt>32) c_error(TOO_COMPLEX_QUERY);
}

void sql_view(CIp_t CI, t_query_expr * * qe)
// Compiles general cursor: query expression, CREATE VIEW statement or DECLARE CURSOR statement.
{
  if (CI->cursym==SQ_CURSOR || // cursor declaration
      CI->cursym==S_IDENT &&
      (!strcmp(CI->curr_name, "SCROLL") ||      // Intermadiate
       !strcmp(CI->curr_name, "LOCKED") ||      // private
       !strcmp(CI->curr_name, "INSENSITIVE") || // Full
       !strcmp(CI->curr_name, "SENSITIVE")))    // SQL3
  { BOOL insensit = FALSE, sensit = FALSE, hold = FALSE;  bool locked=false;
    if (CI->cursym==S_IDENT)
    { if (!strcmp(CI->curr_name, "INSENSITIVE"))
        { insensit = TRUE;  next_sym(CI); }
      else if (!strcmp(CI->curr_name, "SENSITIVE"))
        { sensit = TRUE;  next_sym(CI); }
      if (CI->is_ident("SCROLL"))
        next_sym(CI); // no efect in WinBase, all cursors scrollable
      if (CI->is_ident("LOCKED"))
        { locked=true;  next_sym(CI); }
    }
    test_and_next(CI, SQ_CURSOR, CURSOR_EXPECTED);
    if (CI->cursym==SQ_WITH)
      if (next_sym(CI)==S_IDENT && !strcmp(CI->curr_name, "HOLD"))
        { hold=TRUE;  next_sym(CI); }
      else c_error(GENERAL_SQL_SYNTAX);
    test_and_next(CI, SQ_FOR, FOR_EXPECTED);
    top_query_expression(CI, qe);

    if (insensit)  // force the materialisation if not ensured by the query
    { if ((*qe)->maters!=MAT_HERE && (*qe)->maters!=MAT_HERE2) 
        (*qe)->maters=MAT_INSENSITIVE;
    }
    else if (sensit) // check to see if it is really sensitive (partial sensitivity not tested)
    { if ((*qe)->maters==MAT_HERE || (*qe)->maters==MAT_HERE2) 
        c_error(CURSOR_IS_INSENSITIVE);
    }
    (*qe)->lock_rec=locked;
  }
  else if (CI->cursym==S_VIEW)
  { elem_dynar names;  // used for storing column names in CREATE VIEW (...)
    next_sym(CI);
    test_and_next(CI, S_IDENT, IDENT_EXPECTED);
    if (CI->cursym=='.') { next_sym(CI);  next_sym(CI); }
    if (CI->cursym=='(')  // read the column names
    { do
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        strcpy(names.next()->name, CI->curr_name);
      } while (next_sym(CI)==',');
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
    }
    test_and_next(CI, S_AS, AS_EXPECTED);
    top_query_expression(CI, qe);
   // overwrite the column names:
    int i;  elem_descr * eln, * elq;
    for (i=0, eln=names.acc0(i),     elq=(*qe)->kont.elems.acc(1);
         i<       names.count() && i+1 < (*qe)->kont.elems.count();
         i++, eln++,                 elq++)
      strcpy(elq->name, eln->name);
   // process WITH CHECK OPTION:
    if (CI->cursym==SQ_WITH)
      { next_sym(CI);  next_sym(CI);  next_sym(CI); }
  }
  else // SELECT stored as a SQL view
    top_query_expression(CI, qe);

 // FOR clause processing: It was in the CURSOR branch in v7.0, but it is used in some tests on the end 
 // of a normal SELECT... query, so I moved it here
  if (CI->cursym==SQ_FOR)
  { if (next_sym(CI)==S_IDENT && !strcmp(CI->curr_name, "READ"))
    { if (next_sym(CI)==S_IDENT && !strcmp(CI->curr_name, "ONLY"))
        next_sym(CI); // READ ONLY
      else c_error(GENERAL_SQL_SYNTAX);
      (*qe)->qe_oper &= ~(QE_IS_UPDATABLE | QE_IS_DELETABLE | QE_IS_INSERTABLE);
    }
    else  // not READ ONLY, UPDATE must follow
    { test_and_next(CI, S_UPDATE, UPDATE_EXPECTED);
      if (!((*qe)->qe_oper & QE_IS_UPDATABLE) ||
          (*qe)->maters==MAT_HERE || (*qe)->maters==MAT_HERE2 || (*qe)->maters==MAT_INSENSITIVE) 
        c_error(CANNOT_UPDATE_IN_THIS_VIEW);
      if (CI->cursym==SQ_OF)
        do // reading the column list
        { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        } while (next_sym(CI)==',');
    }
  } // FOR specified
}

CFNC void WINAPI sqlview_compile_entry(CIp_t CI)
// Used by compile_query and nested compilation of cursor in FROM.
{ t_query_expr * * pqe = (t_query_expr * *)CI->stmt_ptr;
  sql_view(CI, pqe);
  if (CI->cursym != S_SOURCE_END && CI->cursym!=';') c_error(SEMICOLON_EXPECTED);
}

BOOL compile_query(cdp_t cdp, tptr source, t_sql_kont * sql_kont,
                   t_query_expr ** pqe, t_optim ** popt)
// Used by INSERT, UPDATE, DELETE statements for separate compilation
// of the referenced cursor (or SELECT created by WHERE clause).
// On error return TRUE and NULL in both output paramaters.
{ *pqe = NULL;  *popt=NULL;
  compil_info xCI(cdp, source, sqlview_compile_entry);
  xCI.stmt_ptr=pqe;  xCI.sql_kont=sql_kont;
  if (compile(&xCI)) goto errex; // compilation error
  *popt=optimize_qe(cdp, *pqe);
  if (*popt==NULL) goto errex;
  if ((*popt)->error) goto errex;
  return FALSE;
 errex:
  if (*pqe!=NULL) { delete *pqe;  *pqe=NULL; } 
  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////// optimization ///////////////////////////////////
void t_variant::init(unsigned leave_count)
{ leave_cnt=leave_count;
  conds.init();
  for (int i=0;  i<leave_cnt;  i++) join_steps[i]=NULL;
}

t_variant::~t_variant(void)
{ for (int i=0;  i<leave_cnt;  i++) 
    if (join_steps[i]!=NULL) join_steps[i]->Release(); 
}

t_optim_join_inner::~t_optim_join_inner(void) 
// destruct all items in the variants dynar:
{ for (int i=0;  i<variants.count();  i++)
    variants.acc0(i)->t_variant::~t_variant();
  if (variant_recdist) delete variant_recdist;
}


int t_optim_join::find_leave(t_query_expr * qe)
// Returns leave index of qe or -1 iff qe is not a leave
{ int i=0;
  while (i<leave_count) if (leaves[i]==qe) return i; else i++;
  return -1;
}

int t_optim::find_leave_by_kontext(db_kontext * kont, const t_query_expr ** pqe) const
// Returns leave index of qe or -1 iff qe is not a leave
{ *pqe=NULL;  return -1; }

int t_optim_join::find_leave_by_kontext(db_kontext * kont, const t_query_expr ** pqe) const
// Returns leave index of qe or -1 iff qe is not a leave
{ int i=0;
  while (i<leave_count) 
    if (&leaves[i]->kont==kont) { *pqe=leaves[i];  return i; }
    else i++;
  *pqe=NULL;  return -1;
}

int t_optim_table::find_leave_by_kontext(db_kontext * kont, const t_query_expr ** pqe) const
// Returns leave index of qe or -1 iff qe is not a leave
{ if (kont==&qet->kont) 
       { *pqe=qet;   return  0; }
  else { *pqe=NULL;  return -1; }
}

int t_optim_sysview::find_leave_by_kontext(db_kontext * kont, const t_query_expr ** pqe) const
// Returns leave index of qe or -1 iff qe is not a leave
{ if (kont==&qe->kont) 
       { *pqe=qe;    return  0; }
  else { *pqe=NULL;  return -1; }
}

#if SQL3
void t_optim_join_inner::find_subtree_leaves(t_query_expr * qe)  // recursive
{ switch (qe->qe_type)
  { default:
      leaves[leave_count++]=qe;
      break;
    case QE_SPECIF:
    { t_qe_specif * qes = (t_qe_specif*)qe;
      if (qes->grouping_type!=GRP_NO || qes->distinct || qes->limit_count!=NULL || qes->limit_offset!=NULL)  // order ignored when used without LIMIT inside JOIN
        leaves[leave_count++]=qes;
      else
        if (qes->from_tables) find_subtree_leaves(qes->from_tables);
      break;
    }
    case QE_JT_INNER:
      find_subtree_leaves(((t_qe_join*)qe)->op1);
      find_subtree_leaves(((t_qe_join*)qe)->op2);
      break;
  }
}
#else
void t_optim_join_inner::find_subtree_leaves(t_query_expr * qe, bool top)  // recursive
{ switch (qe->qe_type)
  { case QE_UNION:    case QE_EXCEPT:    case QE_INTERS:
    case QE_TABLE:    case QE_SYSVIEW:
    case QE_JT_LEFT:  case QE_JT_RIGHT:  case QE_JT_FULL:   case QE_JT_UNION:
      leaves[leave_count++]=qe;
      break;
    case QE_SPECIF:
    { t_qe_specif * qes = (t_qe_specif*)qe;
      if (qes->grouping_type!=GRP_NO/* || qes->distinct && !top*/)  
      // both is wrong, without the condition the DISTINCT is lost, with it the conditions in nested joins are mixed or 
      // join is not optimized (without && !top DISTINCT on the top level causes stack overflow)
        leaves[leave_count++]=qes;
      else
        find_subtree_leaves(qes->from_tables, false);
      break;
    }
    case QE_JT_INNER:
      find_subtree_leaves(((t_qe_join*)qe)->op1, false);
      find_subtree_leaves(((t_qe_join*)qe)->op2, false);
      break;
  }
}
#endif

////////////////////////////// condition classification ////////////////////////////////
bool is_index_part(const t_express * ex, const t_express * partex, int pure_oper)
{ if (pure_oper!=PUOP_PREF || !ex->specif.charset) 
    return same_expr(ex, partex)==TRUE;
 // looking for special index for prefix operation on national strings:
  if (partex->sqe!=SQE_CAST) return false;
  if (partex->type!=ATT_STRING || partex->specif.charset) return false;
  const t_express * castarg = ((const t_expr_cast*)partex)->arg;
  if (castarg->type!=ATT_STRING) return false;  // not CASTing from string
  if (castarg->sqe!=SQE_FUNCT) 
    return same_expr(ex, castarg)==TRUE;
 // ignore case:
  const t_expr_funct * fex = (const t_expr_funct *)castarg;
  if (fex->num!=FCNUM_UPPER) return false;
  return same_expr(ex, fex->arg1)==TRUE;
}

static void is_attr_ref(cdp_t cdp, t_express * ex, const db_kontext * tabkont, int & indnum, 
               int & partnum, int & valtype, t_specif & valspecif, const t_query_expr * lqe, int pure_oper)
// Searches for [ex] in the parts of indicies of the table in [tabkont]. 
//  [ex] does not contain references to columns of other tables.
//  If found, returns [indnum], [partnum], [valtype] and [valspecif], and stores [tabkont] into ex.
// If not found, [indnum]=-1.
// If [lqe] specifies preferred index, searches this index only.
{ indnum=-1;
  if (tabkont==NULL) return;  // not a single table expression
#if SQL3
  ttablenum tbnum = tabkont->t_mat->tabnum;
#else
  ttablenum tbnum = ((t_mater_table*)(tabkont->mat))->tabnum;
#endif
  int pref_index = -1;
  if (lqe!=NULL && lqe->qe_type==QE_TABLE)
    pref_index = ((t_qe_table*)lqe)->pref_index_num;
  { table_descr_auto td(cdp, tbnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
    if (!td->me()) return;
    dd_index * cc;  int ind;
    for (ind=0, cc=td->indxs;  ind<td->indx_cnt;  ind++, cc++)
      if (!cc->disabled)
       if (cc->has_nulls || pure_oper==PUOP_GT || pure_oper==PUOP_PREF || 
           (pure_oper==PUOP_GE || pure_oper==PUOP_EQ || pure_oper==PUOP_BETWEEN) 
#if WBVERS>=900
           // && !(cdp->sqloptions & SQLOPT_NULLEQNULL)  // it is more correct -- applications whith primary indexes without NULL turn too slow!!
#endif
           )
        if (pref_index == -1 || pref_index == ind)  // no index can be selected if pref_index_num==-2 (only fulltext)
          for (int part=0;  part<cc->partnum;  part++)
            if (is_index_part(ex, cc->parts[part].expr, pure_oper))
              if (indnum==-1 || part<partnum) // take the index with the smallest partnum
              { indnum=ind;  partnum=part;
                valtype  =cc->parts[part].type;
                valspecif=cc->parts[part].specif;
                ex->tabkont=tabkont;
                break;
              }
  }
}

/////////////////////// analysing references in the expression /////////////////////////////////////////
// Goes down the expression structure, into subqueries etc.

enum t_tabrefstat { TRS_NO, TRS_SINGLE, TRS_MULTIPLE, TRS_OTHER };
enum t_refinfo_type { RIT_STANDARD,     // original version, in UNION goes to 1 direction
                      RIT_UNION_BOTH,   // original version, in UNION goes to both directions
                      RIT_GROUPING_OPT  // special version for optimizig the conditions inherited by grouping
};

SPECIF_DYNAR(db_kontext_dynar, const db_kontext *, 4);

struct t_refinfo   // result of get_reference_info
{ 
  t_refinfo_type refinfo_type;
  const t_optim * optref; // list of leaves reference or NULL, input parameter
  uns32     use_map;   // indicied of leaves reference, defined iff optref!=NULL
  t_tabrefstat stat;   // status of table reference gathering
  const db_kontext * onlytabkont; // kontext of the only referenced table (when stat==TRS_SINGLE)
  const t_query_expr * lqe;  // qe of the leave found, valid only if onlytabkont!=NULL
 // search for aggregates:
  const db_kontext * groupkont;
  bool has_aggr_on_groupkont;
  db_kontext_dynar added_kontexts;  // list of references to passed db_kontexts when going down
  t_refinfo(const t_optim * optrefIn, t_refinfo_type refinfo_typeIn = RIT_STANDARD)
  { optref=optrefIn;  refinfo_type=refinfo_typeIn;
    use_map=0;  onlytabkont=groupkont=NULL;  has_aggr_on_groupkont=false;  
    stat=TRS_NO;  lqe=NULL; 
  }
  bool is_in_added_kontexts(const db_kontext * kont)
  { for (int i=0;  i<added_kontexts.count();  i++)
      if (kont==*added_kontexts.acc0(i))
        return true;
    return false;
  }
};

static void get_qe_ref_info(t_query_expr * qe, t_refinfo * ri);

static void get_reference_info(t_express * ex, t_refinfo * ri)
// Adding reference info to the stored in t_refinfo
{ switch (ex->sqe)
  { case SQE_UNARY:
      get_reference_info(((t_expr_unary*)ex)->op, ri);
      break;
    case SQE_BINARY:
    { t_expr_binary * bex = (t_expr_binary*)ex;
      get_reference_info(bex->op1, ri);
      if (bex->op2!=NULL) get_reference_info(bex->op2, ri); // may be NULL in "IN (list)"
      break;
    }
    case SQE_TERNARY:
      get_reference_info(((t_expr_ternary*)ex)->op1, ri);
      get_reference_info(((t_expr_ternary*)ex)->op2, ri);
      if (((t_expr_ternary*)ex)->op3!=NULL)  // may be NULL in LIKE-expression
        get_reference_info(((t_expr_ternary*)ex)->op3, ri);
      break;
    case SQE_FUNCT:
    { t_expr_funct * fex = (t_expr_funct *)ex;
      if (fex->arg1!=NULL) get_reference_info(fex->arg1, ri);
      if (fex->arg2!=NULL) get_reference_info(fex->arg2, ri);
      if (fex->arg3!=NULL) get_reference_info(fex->arg3, ri);
      if (fex->arg4!=NULL) get_reference_info(fex->arg4, ri);
      if (fex->arg5!=NULL) get_reference_info(fex->arg5, ri);
      if (fex->arg6!=NULL) get_reference_info(fex->arg6, ri);
      if (fex->arg7!=NULL) get_reference_info(fex->arg7, ri);
      if (fex->arg8!=NULL) get_reference_info(fex->arg8, ri);
      break;
    }
    case SQE_CASE:
    { t_expr_case * cex = (t_expr_case *)ex;
      if (cex->val1  !=NULL) get_reference_info(cex->val1,   ri);
      if (cex->val2  !=NULL) get_reference_info(cex->val2,   ri);
      if (cex->contin!=NULL) get_reference_info(cex->contin, ri);
      break;
    }
    case SQE_POINTED_ATTR:
    { t_expr_pointed_attr * poatr = (t_expr_pointed_attr*)ex;
      get_reference_info(poatr->pointer_attr, ri);
      break;
    }
    case SQE_ATTR:  case SQE_ATTR_REF:
    { t_expr_attr * exat = (t_expr_attr *)ex;
      if (exat->indexexpr!=NULL) get_reference_info(exat->indexexpr, ri);
      db_kontext * an_kont=exat->kont;  int an_elemnum=exat->elemnum;
      if (!an_kont) break;  // short-path reference
      elem_descr * el;
#if SQL3
     // go down the simple-kontext links:
      do
      { if (ri->optref!=NULL)  // looking for leave use map
        { int ind = ri->optref->find_leave_by_kontext(an_kont, &ri->lqe);
          if (ind != -1)
          { ri->use_map |= 1 << ind;
            if (ri->stat==TRS_MULTIPLE) return; // no need to continue
          }
        }
        if (ri->is_in_added_kontexts(an_kont)) return;  // ignored (e.g. added table in EXISTS condition)
        el = an_kont->elems.acc(an_elemnum);
        if (an_kont->grouping_here) 
          if (ri->refinfo_type==RIT_GROUPING_OPT)  // test added, without it the conditions inherited into the join contained in a grouping were not analysed
            break;
        if (el->link_type==ELK_KONTEXT) 
        { an_kont   =el->kontext.kont;
          an_elemnum=el->kontext.down_elemnum;
        }
        else if ((el->link_type==ELK_KONT2U || el->link_type==ELK_KONT21) && ri->refinfo_type!=RIT_UNION_BOTH)
        { if (an_kont->position)  // must go both directions!!!
          { an_kont   =el->kontdbl.kont2;
            an_elemnum=el->kontdbl.down_elemnum2;
          }
          else
          { an_kont   =el->kontdbl.kont1;
            an_elemnum=el->kontdbl.down_elemnum1;
          }
        }
        else break;
      } while (true);
     // process the element:
      if (el->link_type==ELK_AGGREXPR || el->link_type==ELK_EXPR && el->expr.eval->sqe==SQE_AGGR)
        if (an_kont==ri->groupkont) 
          ri->has_aggr_on_groupkont=true;
      bool stophere;
      if (ri->refinfo_type==RIT_GROUPING_OPT)   // stop on the materialized level, may be wrong
        stophere = an_kont->t_mat!=NULL || an_kont->grouping_here;
      else
        stophere = an_kont->t_mat!=NULL;
      if (stophere)
      { if (ri->refinfo_type!=RIT_GROUPING_OPT && !an_kont->t_mat->permanent)
          { ri->stat=TRS_OTHER;   ri->onlytabkont=NULL; }
        else if (ri->stat==TRS_NO)
          { ri->stat=TRS_SINGLE;  ri->onlytabkont=an_kont; }
        else if (ri->stat==TRS_SINGLE)
          if (ri->onlytabkont!=an_kont)
            { ri->stat=TRS_MULTIPLE;  ri->onlytabkont=NULL; }
        return;
      }
     // recursive processing in 2 directions:
      if (el->link_type==ELK_KONT2U || el->link_type==ELK_KONT21)
      { t_expr_attr exatx(-1, NULL);
        exatx.kont=el->kontdbl.kont1;  exatx.elemnum=el->kontdbl.down_elemnum1;
        get_reference_info(&exatx, ri);
        exatx.kont=el->kontdbl.kont2;  exatx.elemnum=el->kontdbl.down_elemnum2;
        get_reference_info(&exatx, ri);
      }
      else if (el->link_type==ELK_EXPR || el->link_type==ELK_AGGREXPR)
        get_reference_info(el->expr.eval, ri);
#else

      do
      { if (ri->optref!=NULL)  // looking for leave use map
        { int ind = ri->optref->find_leave_by_kontext(an_kont, &ri->lqe);
          if (ind != -1)
          { ri->use_map |= 1 << ind;
            if (ri->stat==TRS_MULTIPLE) return; // no need to continue
          }
        }
        if (an_kont->mat!=NULL) // materialized level, stop searching here!
        { if (an_kont->mat->is_ptrs ||
              !((t_mater_table*)(an_kont->mat))->permanent)
            { ri->stat=TRS_OTHER;   ri->onlytabkont=NULL; }
          else if (ri->stat==TRS_NO)
            { ri->stat=TRS_SINGLE;  ri->onlytabkont=an_kont; }
          else if (ri->stat==TRS_SINGLE)
            if (ri->onlytabkont!=an_kont)
              { ri->stat=TRS_MULTIPLE;  ri->onlytabkont=NULL; }
          return;
        }
        el = an_kont->elems.acc(an_elemnum);
        if (el->link_type==ELK_KONTEXT)
        { an_kont   =el->kontext.kont;
          an_elemnum=el->kontext.down_elemnum;
        }
        else if (el->link_type==ELK_KONT2U || el->link_type==ELK_KONT21)
          if (an_kont->position)  // must go both directions!!!
          { an_kont   =el->kontdbl.kont2;
            an_elemnum=el->kontdbl.down_elemnum2;
          }
          else
          { an_kont   =el->kontdbl.kont1;
            an_elemnum=el->kontdbl.down_elemnum1;
          }
        else break;
      } while (TRUE);
      if (el->link_type!=ELK_EXPR && el->link_type!=ELK_AGGREXPR)
        break; // error!
      get_reference_info(el->expr.eval, ri);
#endif  // !SQL3
      break;
    }
    case SQE_MULT_COUNT:
      get_reference_info(((t_expr_mult_count*)ex)->attrex, ri);  break;
    case SQE_VAR_LEN:
      get_reference_info(((t_expr_var_len   *)ex)->attrex, ri);  break;
    case SQE_DBLIND:
      get_reference_info(((t_expr_dblind    *)ex)->attrex, ri);
      get_reference_info(((t_expr_dblind    *)ex)->start,  ri);
      get_reference_info(((t_expr_dblind    *)ex)->size,   ri);
      break;
    case SQE_CAST:
      get_reference_info(((t_expr_cast *)ex)->arg, ri);  break;
    case SQE_SUBQ:
      get_qe_ref_info(((t_expr_subquery*)ex)->qe,  ri);  break;
    case SQE_AGGR:
      if (((t_expr_aggr *)ex)->kont==ri->groupkont)
        ri->has_aggr_on_groupkont=TRUE;
      if (((t_expr_aggr *)ex)->arg != NULL)  // NULL for COUNT(*)
        get_reference_info(((t_expr_aggr *)ex)->arg, ri);  
      break;
    case SQE_ROUTINE:
    { sql_stmt_call * stmt = (sql_stmt_call*)ex;
      for (int i=0;  i<stmt->actpars.count();  i++) 
      { t_express * actpar = *stmt->actpars.acc0(i);
        if (actpar)
          get_reference_info(actpar, ri);
      }
      break;
    }
    default:  // SQE_NULL, SQE_AGGR, SQE_EMBED, SQE_DYNPAR, SQE_SCONST, SQE_LCONST
              // SQE_VAR, SQE_CURRREC,  (incorrect) SQE_SEQ
      break;

  } // switch by sqe
}

static void get_qe_ref_info(t_query_expr * qe, t_refinfo * ri)
// Retrieves info about outer references in a subquery expression.
{ int i;
  if (qe->order_by!=NULL)
  { for (i=0;  i<qe->order_by->order_indx.partnum;  i++)
      get_reference_info(qe->order_by->order_indx.parts[i].expr, ri);
  }
  switch (qe->qe_type)
  { case QE_SPECIF:
    { t_qe_specif * qes = (t_qe_specif *)qe;
      const db_kontext ** kontext_ref = ri->added_kontexts.next();
      unsigned add_pos = ri->added_kontexts.count()-1;
      if (kontext_ref && qes->from_tables) *kontext_ref = &qes->from_tables->kont;
      if (qes->where_expr !=NULL) get_reference_info(qes->where_expr,  ri);
      if (qes->having_expr!=NULL) get_reference_info(qes->having_expr, ri);
      if (qes->grouping_type==GRP_BY)
        for (i=0;  i<qes->group_indx.partnum;  i++)
          get_reference_info(qes->group_indx.parts[i].expr, ri);
      if (kontext_ref) ri->added_kontexts.delet(add_pos);
      break;
    }
    case QE_TABLE:  case QE_SYSVIEW:
      break;
    case QE_UNION:  case QE_EXCEPT:  case QE_INTERS:
    { t_qe_bin * qeb = (t_qe_bin *)qe;
      get_qe_ref_info(qeb->op1, ri);
      get_qe_ref_info(qeb->op2, ri);
      break;
    }
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
    case QE_JT_FULL:   case QE_JT_UNION:
    { t_qe_join * qej = (t_qe_join*)qe;
      get_qe_ref_info(qej->op1, ri);
      get_qe_ref_info(qej->op2, ri);
      if (qej->join_cond!=NULL) get_reference_info(qej->join_cond, ri);
      break;
    }
  }
}

static int onlytab(uns32 use_map)
// Returns position of the only 1 in use_map or -1 otherwise.
{ int tabnum=0;
  while (use_map && !(use_map & 1))  { use_map >>= 1;  tabnum++; }
  return (!use_map || (use_map & ~1)) ? -1 : tabnum;
}

bool is_dividable(t_optim * leave_opt)
// Prevents using OR_DIVIDE when any of the joined table is a grouping result - would be different in variants but all grouping results have to coexist simultaneously.
{ if (!leave_opt) return false;
  if (leave_opt->optim_type!=OPTIM_JOIN_INNER) return false;
  t_optim_join_inner * jopt = (t_optim_join_inner*)leave_opt;
  for (int i = 0;  i<jopt->leave_count;  i++)
    if (jopt->leaves[i]->qe_type!=QE_TABLE) return false;
  return true;
}

static void classify_condition(cdp_t cdp, t_express * ex, t_condition * cond, t_optim * leave_opt)
// [leave_opt] is the t_optim which needs to clasify the condition for own purposes
// Find information about relations in ex, classify ORs.
// If leave_opt!=NULL, create leave usage map.
// Store result into cond.
{ cond->use_map1=cond->use_map2=0;
  cond->tabnum1=cond->tabnum2=cond->indnum1=cond->indnum2=-1;
  cond->is_relation=FALSE;
  if (ex->sqe == SQE_BINARY)
  { t_expr_binary * binex = (t_expr_binary *)ex;
    if (binex->oper.pure_oper==PUOP_AND || binex->oper.pure_oper==PUOP_OR)
    { t_condition c1, c2;  BOOL exact=FALSE;
      classify_condition(cdp, binex->op1, &c1, leave_opt);
      classify_condition(cdp, binex->op2, &c2, leave_opt);
      if (binex->oper.pure_oper==PUOP_AND)
      { if (c1.tabnum1==c2.tabnum1 && c1.indnum1==c2.indnum1 && c1.partnum1==c2.partnum1 ||
            c1.tabnum1==c2.tabnum2 && c1.indnum1==c2.indnum2 && c1.partnum1==c2.partnum2)
        { cond->tabnum1 =c1.tabnum1;   cond->indnum1   =c1.indnum1;
          cond->partnum1=c1.partnum1;  cond->use_map1  =c1.use_map1;
          cond->valtype1=c1.valtype1;  cond->valspecif1=c1.valspecif1;
          cond->use_map2=c1.use_map2 | (c1.tabnum1==c2.tabnum1 ? c2.use_map2 : c2.use_map1);
          exact=TRUE;  cond->is_relation=2;
        }
        if (c1.tabnum2==c2.tabnum1 && c1.indnum2==c2.indnum1 && c1.partnum2==c2.partnum1 ||
            c1.tabnum2==c2.tabnum2 && c1.indnum2==c2.indnum2 && c1.partnum2==c2.partnum2)
        { cond->tabnum2 =c1.tabnum2;   cond->indnum2   =c1.indnum2;
          cond->partnum2=c1.partnum2;  cond->use_map2  =c1.use_map2;
          cond->valtype2=c1.valtype2;  cond->valspecif2=c1.valspecif2;
          cond->use_map1=c1.use_map1 | (c1.tabnum2==c2.tabnum2 ? c2.use_map1 : c2.use_map2);
          exact=TRUE;  cond->is_relation=2;
        }
        if (!exact)
        { cond->use_map1=cond->use_map2=c1.use_map1|c1.use_map2|c2.use_map1|c2.use_map2;
          if (c1.tabnum1!=-1 || c1.tabnum2!=-1 || c2.tabnum1!=-1 || c2.tabnum2!=-1)
            cond->tabnum1=cond->tabnum2=-2;
          else
            cond->tabnum1=cond->tabnum2=-1;
        }
      }
      else  // PUOP_OR
      { if (c1.tabnum1==c2.tabnum1 && c1.tabnum1>=0 && c1.indnum1==c2.indnum1 && c1.indnum1>=0 && c1.partnum1==c2.partnum1 ||
            c1.tabnum1==c2.tabnum2 && c1.tabnum1>=0 && c1.indnum1==c2.indnum2 && c1.indnum1>=0 && c1.partnum1==c2.partnum2)
        { cond->tabnum1 =c1.tabnum1;   cond->indnum1 =c1.indnum1;
          cond->partnum1=c1.partnum1;  cond->use_map1=c1.use_map1;
          cond->valtype1=c1.valtype1;  cond->valspecif1=c1.valspecif1;
          cond->use_map2=c1.use_map2 | (c1.tabnum1==c2.tabnum1 ? c2.use_map2 : c2.use_map1);
          binex->opt_info=OR_JOIN;
          exact=TRUE;  cond->is_relation=2;
        }
        if (c1.tabnum2==c2.tabnum1 && c1.tabnum2>=0 && c1.indnum2==c2.indnum1 && c1.indnum2>=0 && c1.partnum2==c2.partnum1 ||
            c1.tabnum2==c2.tabnum2 && c1.tabnum2>=0 && c1.indnum2==c2.indnum2 && c1.indnum2>=0 && c1.partnum2==c2.partnum2)
        { cond->tabnum2 =c1.tabnum2;   cond->indnum2 =c1.indnum2;
          cond->partnum2=c1.partnum2;  cond->use_map2=c1.use_map2;
          cond->valtype2=c1.valtype2;  cond->valspecif2=c1.valspecif2;
          cond->use_map1=c1.use_map1 | (c1.tabnum2==c2.tabnum2 ? c2.use_map1 : c2.use_map2);
          binex->opt_info=OR_JOIN;
          exact=TRUE;  cond->is_relation=2;
        }
        if (!exact)
        { cond->use_map1=cond->use_map2=c1.use_map1|c1.use_map2|c2.use_map1|c2.use_map2;
#if 0  // this version is beter for simple conditions without indexes, dividing them does not help
          if ((c1.tabnum1!=-1 && c1.indnum1>=0 || c1.tabnum2!=-1 && c1.indnum2>=0) &&
              (c2.tabnum1!=-1 && c2.indnum1>=0 || c2.tabnum2!=-1 && c2.indnum2>=0) && 
#else  // this version is necessary for complex conditions joined by OR, cannot do any optimization without dividing them
          if ((c1.tabnum1!=-1 || c1.tabnum2!=-1) &&
              (c2.tabnum1!=-1 || c2.tabnum2!=-1) && 
#endif              
              is_dividable(leave_opt))
            { cond->tabnum1=cond->tabnum2=-2;  binex->opt_info=OR_DIVIDE; }
          else
            { cond->tabnum1=cond->tabnum2=-1;  binex->opt_info=OR_OR; }
        }
      }
      return;
    }
    else if (!binex->oper.all_disp && !binex->oper.any_disp && // disables subquery comparison and IN-predicate
             (binex->oper.pure_oper >= PUOP_EQ && binex->oper.pure_oper < PUOP_NE ||
              binex->oper.pure_oper == PUOP_PREF || binex->oper.pure_oper == PUOP_IS_NULL))
    { cond->is_relation=TRUE;
      { t_refinfo ri(leave_opt);
        get_reference_info(binex->op1, &ri);  cond->use_map1=ri.use_map;
        cond->tabnum1=onlytab(ri.use_map);
        is_attr_ref(cdp, binex->op1, ri.onlytabkont, cond->indnum1, cond->partnum1, cond->valtype1, cond->valspecif1, ri.lqe, binex->oper.pure_oper);
       // disable case-sensitive index for a insensitive limit (comparision should be insensitive):
        if (cond->indnum1!=-1 && binex->op1->type==ATT_STRING && !binex->op1->specif.ignore_case && binex->op2->specif.ignore_case)
          cond->indnum1=-1;
      }
      { t_refinfo ri(leave_opt);
        get_reference_info(binex->op2, &ri);  cond->use_map2=ri.use_map;
        if (binex->oper.pure_oper != PUOP_PREF)  // must not find index for the prefix
        { cond->tabnum2=onlytab(ri.use_map);
          is_attr_ref(cdp, binex->op2, ri.onlytabkont, cond->indnum2, cond->partnum2, cond->valtype2, cond->valspecif2, ri.lqe, binex->oper.pure_oper);
         // disable case-sensitive index for a insensitive limit (comparision should be insensitive):
          if (cond->indnum2!=-1 && binex->op2->type==ATT_STRING && !binex->op2->specif.ignore_case && binex->op1->specif.ignore_case)
            cond->indnum2=-1;
        }
      }
      return;
    }
  } // binary
  else if (ex->sqe == SQE_TERNARY)
  { t_expr_ternary * ternex = (t_expr_ternary *)ex;
    if (ternex->oper.pure_oper==PUOP_BETWEEN)
    { cond->is_relation=TRUE;
      { t_refinfo ri(leave_opt);
        get_reference_info(ternex->op1, &ri);  cond->use_map1=ri.use_map;
        cond->tabnum1=onlytab(ri.use_map);
        is_attr_ref(cdp, ternex->op1, ri.onlytabkont, cond->indnum1, cond->partnum1, cond->valtype1, cond->valspecif1, ri.lqe, ternex->oper.pure_oper);
       // disable case-sensitive index for a insensitive limit (comparision should be insensitive):
        if (cond->indnum1!=-1 && ternex->op1->type==ATT_STRING && !ternex->op1->specif.ignore_case)
          if (ternex->op2->specif.ignore_case || ternex->op3->specif.ignore_case)
            cond->indnum1=-1;
      }
      { t_refinfo ri(leave_opt);
        get_reference_info(ternex->op2, &ri);
        get_reference_info(ternex->op3, &ri);  cond->use_map2=ri.use_map;
      }
      return;
    }
  }
  else if (ex->sqe == SQE_FUNCT)
  { t_expr_funct * fex = (t_expr_funct *)ex;
    if (fex->num==FCNUM_FULLTEXT)
    { cond->is_relation=TRUE;
      { t_refinfo ri(leave_opt);
        get_reference_info(fex->arg2, &ri);  cond->use_map1=ri.use_map;
        cond->tabnum1=onlytab(ri.use_map);
        cond->indnum1=-2;  cond->partnum1=0;  cond->valtype1=0;  cond->valspecif1.set_null(); // patnum1==0 is necessary
      }
      { t_refinfo ri(leave_opt);
        get_reference_info(fex->arg1, &ri);
        get_reference_info(fex->arg3, &ri);  cond->use_map2=ri.use_map;
      }
      return;
    }
  }
  else if (ex->sqe == SQE_ATTR && ex->type==ATT_BOOLEAN)
  { t_expr_attr * atex = (t_expr_attr *)ex;
    t_refinfo ri(leave_opt);
    get_reference_info(ex, &ri);
    cond->tabnum1=onlytab(ri.use_map);
    cond->use_map1=ri.use_map;  
    cond->use_map2=0;  cond->tabnum2=-1;
    is_attr_ref(cdp, ex, ri.onlytabkont, cond->indnum1, cond->partnum1, cond->valtype1, cond->valspecif1, ri.lqe, PUOP_EQ);
    cond->is_relation=TRUE;  // makes possin
    return;
  }
 // other expresions:
  { t_refinfo ri(leave_opt);
    get_reference_info(ex, &ri);
    cond->use_map2=cond->use_map1=ri.use_map;
  }
}

////////////////////////// optimizing classified variant /////////////////////
static bool add_table_to_join_order(const t_joinorder * jo_patt, t_joinorder_dynar * jod, int tabnum, double step_cost, trecnum rcnt)
// Creates a new join order from jo_patt with [tabnum] added on its end.
{ t_joinorder * jo=jod->next();  
  if (jo==NULL) return false;
  *jo=*jo_patt;  
  jo->litab[jo->tabcnt++].leavenum = tabnum;  
  jo->map |= 1<<tabnum;
  jo->cost*=step_cost;
  if (rcnt) jo->rcount*=rcnt;  // ignore if rcnt==0, may be result of dividing
  return true;
}

bool t_optim_join::enlarge_join_order(t_variant * variant, const t_joinorder * jo_patt, t_joinorder_dynar * jod, bool orderdef_table)
// Creates one or more join orders extending [jo_patt] by 1 table not contained in it. Addes them to [jod].
{ int i, tabnum;  double cost;  trecnum rcnt;
  uns32 used_map = jo_patt->map;
  bool firsttab = !jo_patt->tabcnt;
  if (!firsttab || order_==NULL) orderdef_table=false;
 // find well delimited tables:
  uns32 added = 0;  // important only for the first table
  bool any_found = false;
  for (i=0;  i<variant->conds.count();  i++)
  { t_condition * acond = variant->conds.acc0(i);
    if (acond->is_relation)
    { if (acond->tabnum1>=0 && !(acond->use_map2 & ~used_map) &&
                                 (acond->use_map1 & ~used_map) &&
                                  acond->indnum1!=-1 && !acond->partnum1)
      { tabnum = acond->tabnum1;
        if (!(added & (1<<tabnum)))
        { if (acond->indnum1==-2) cost=0.001; // fulltext index
          else if (orderdef_table && leaves[tabnum]->qe_type==QE_TABLE &&
                   &leaves[tabnum]->kont==order_->order_tabkont && acond->indnum1==order_->order_indnum)
            cost=0.2;                         // ordering index
          else cost=1;                        // normal index
          if (leaves[tabnum]->qe_type==QE_TABLE) rcnt=(trecnum)pow(((t_qe_table*)leaves[tabnum])->extent, 1.0/3.0);  else rcnt=1000;
          if (!add_table_to_join_order(jo_patt, jod, tabnum, cost, rcnt)) return false;
          any_found=true;  added |= (1<<tabnum);
        }
      }
      if (acond->tabnum2>=0 && !(acond->use_map1 & ~used_map) &&
                                 (acond->use_map2 & ~used_map) &&
                                  acond->indnum2!=-1 && !acond->partnum2)
      { tabnum = acond->tabnum2;
        if (!(added & (1<<tabnum)))
        { if (orderdef_table && leaves[tabnum]->qe_type==QE_TABLE &&
              &leaves[tabnum]->kont==order_->order_tabkont && acond->indnum2==order_->order_indnum)
            cost=0.2;       // ordering index
          else cost=1;      // normal index
          if (leaves[tabnum]->qe_type==QE_TABLE) rcnt=(trecnum)pow(((t_qe_table*)leaves[tabnum])->extent, 1.0/3.0) ;  else rcnt=1000;
          if (!add_table_to_join_order(jo_patt, jod, tabnum, cost, rcnt)) return false;
          any_found=true;  added |= (1<<tabnum);
        }
      }
    }
  }

 // find all delimited tables:
  if (!any_found || firsttab)
  { for (i=0;  i<variant->conds.count();  i++)
    { t_condition * acond = variant->conds.acc0(i);
      if (acond->is_relation)
      { if (acond->tabnum1>=0 && !(acond->use_map2 & ~used_map) &&
                                   (acond->use_map1 & ~used_map))
        { tabnum = acond->tabnum1;
          cost=100;
          if (orderdef_table && leaves[tabnum]->qe_type==QE_TABLE)
            if (&leaves[tabnum]->kont==order_->order_tabkont)
              cost=20;
          if (!(added & (1<<tabnum)))
          { if (leaves[tabnum]->qe_type==QE_TABLE) rcnt=(trecnum)pow(((t_qe_table*)leaves[tabnum])->extent, 0.5);  else rcnt=1000;
            if (!add_table_to_join_order(jo_patt, jod, tabnum, cost, rcnt)) return false;
            any_found=true;  added |= (1<<tabnum);
          }
        }
        if (acond->tabnum2>=0 && !(acond->use_map1 & ~used_map) &&
                                   (acond->use_map2 & ~used_map))
        { tabnum = acond->tabnum2;
          cost=100;
          if (orderdef_table && leaves[tabnum]->qe_type==QE_TABLE)
            if (&leaves[tabnum]->kont==order_->order_tabkont)
              cost=20;
          if (!(added & (1<<tabnum)))
          { if (leaves[tabnum]->qe_type==QE_TABLE) rcnt=(trecnum)pow(((t_qe_table*)leaves[tabnum])->extent, 0.5);  else rcnt=1000;
            if (!add_table_to_join_order(jo_patt, jod, tabnum, cost, rcnt)) return false;
            any_found=true;  added |= (1<<tabnum);
          }
        }
      }
    }
  }

 // add all tables if none added so far:
  if (!any_found)
  { for (tabnum=0;  tabnum<leave_count;  tabnum++)
      if ((1<<tabnum) & ~used_map)  // table is not in the join order yet
        if (!(added & (1<<tabnum)))
        { cost=1000;
          if (orderdef_table && leaves[tabnum]->qe_type==QE_TABLE)
            if (&leaves[tabnum]->kont==order_->order_tabkont)
              cost=200;
          if (leaves[tabnum]->qe_type==QE_TABLE) rcnt=((t_qe_table*)leaves[tabnum])->extent;  else rcnt=1000;
          if (!add_table_to_join_order(jo_patt, jod, tabnum, cost, rcnt)) return false;
          any_found=true;  added |= (1<<tabnum);
        }
  }
  return true;
}

void t_optim_join::best_join_order(t_variant * variant, BOOL may_define_ordering, t_joinorder * jo_out)
{ t_joinorder_dynar jod;  t_joinorder jo_patt;
 // create the starter empty join order:
  jo_patt.tabcnt=0;  jo_patt.map=0;  jo_patt.cost=1;  jo_patt.rcount=1;  jo_patt.expanded=false;
  if (user_order)
  { for (int i=0;  i<leave_count;  i++)
      jo_patt.litab[i].leavenum=i;
    jo_patt.tabcnt=leave_count;
  }
  else
  {// enlarge join order chains until the best of them is complete:
    int best_jo_ind = -1;  // jo being expanded
    do
    { if (!enlarge_join_order(variant, &jo_patt, &jod, may_define_ordering!=FALSE))
        { error=TRUE;  return; } // error
      if (best_jo_ind!=-1)
        jod.acc0(best_jo_ind)->expanded=true;
     // find the best join order (not expanded or full):
      double cost = 1e300;  trecnum rcount=0x7fffffff;  
      for (int i=0;  i<jod.count();  i++)
      { t_joinorder * an_jo = jod.acc0(i);
        if (!an_jo->expanded || an_jo->tabcnt==leave_count)
          if (an_jo->cost < cost || an_jo->cost == cost && an_jo->rcount<rcount)
            { cost=an_jo->cost;  rcount=an_jo->rcount;  best_jo_ind=i; }
      }
     // copy it out in order to be safe after next() is called on jod:
      jo_patt = *jod.acc0(best_jo_ind);
    } while (jo_patt.tabcnt!=leave_count); // join order complete!
  }
 // return the copy of the best:
  *jo_out=jo_patt;
}

void t_optim_join::distribute_conditions(cdp_t cdp, t_joinorder * jo, t_variant * variant, BOOL may_define_ordering)
{ uns32 map = 0;   int i, j;
  for (i=0;  i<leave_count;  i++)
  { int tabind=jo->litab[i].leavenum;  t_optim * lopt;
    variant->join_steps[i]=lopt=alloc_opt(cdp, leaves[tabind], order_, false);
    if (lopt==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  return; }
   // interval calc and post conds for lopt:
    for (j=0;  j<variant->conds.count();  j++)
    { t_condition * acond = variant->conds.acc0(j);
      if (acond->generated) continue;
      if (acond->tabnum1==tabind && acond->indnum1!=-1 && acond->is_relation &&
          !(acond->use_map2 & ~map) && (acond->use_map1 & ~map))
      { lopt->push_condition(cdp, acond, COND_LIM1);
        acond->generated=TRUE;
      }
      else
      if (acond->tabnum2==tabind && acond->indnum2!=-1 && acond->is_relation &&
          !(acond->use_map1 & ~map) && (acond->use_map2 & ~map))
      { lopt->push_condition(cdp, acond, COND_LIM2);
        acond->generated=TRUE;
      }
      else
      if (!((acond->use_map1|acond->use_map2) & ~(map | (1<<tabind))))
      { lopt->push_condition(cdp, acond, COND_POST);
        acond->generated=TRUE;
      }
    }
   // optimize leaves:
    lopt->optimize(cdp, i==0 && may_define_ordering);
   // update the used table map:
    map |= 1<<tabind;
  }
}

void t_optim_join_inner::optimize_variant(cdp_t cdp, t_variant * variant, BOOL may_define_ordering)
{ t_joinorder jo;  
  best_join_order(variant, may_define_ordering, &jo);  if (error) return;
  distribute_conditions(cdp, &jo, variant, may_define_ordering);
} 

void t_optim_join_outer::optimize(cdp_t cdp, BOOL ordering_defining_table)
{// register both leaves: 
  leaves[0]=ojtop->op1;  leaves[1]=ojtop->op2;  leave_count=2;
 // store the local OJ condition:
  if (ojtop->join_cond!=NULL) add_condition_ex(cdp, ojtop->join_cond, &variant); 
 // add the inherited conditions, unless referencing the inner operand:
  uns32 disabling_map;
  if      (join_qe_type==QE_JT_LEFT)  disabling_map=2;
  else if (join_qe_type==QE_JT_RIGHT) disabling_map=1;
  else                                disabling_map=3;
  conds_outer = 0;
  for (int i=0;  i<inherited_conds.count();  i++)
  { t_condition cond;  cond.expr=*inherited_conds.acc0(i);
    classify_condition(cdp, cond.expr, &cond, this);
    if ((cond.use_map1 | cond.use_map2) & disabling_map)
      conds_outer |= 1<<i;
    else
    { cond.generated=FALSE;
      t_condition * pcond=variant.conds.next();
      if (pcond==NULL) error=TRUE;  else *pcond=cond;
    }
  }
 // optimize the outer join:
  t_joinorder jo;  
  best_join_order(&variant, ordering_defining_table, &jo);  if (error) return;
 // prepare outer join according to the best ordering:
  BOOL reversed = jo.litab[0].leavenum!=0;
  if (join_qe_type==QE_JT_FULL || join_qe_type==QE_JT_UNION)
  { left_outer=right_outer=TRUE;
    ordering_defining_table=FALSE;
  }
  else
  { left_outer  = !reversed == (join_qe_type==QE_JT_LEFT);
    right_outer = !left_outer;
    if (right_outer) ordering_defining_table=FALSE;
  }
  if (reversed) { oper1=leaves[1];  oper2=leaves[0]; }
  else          { oper1=leaves[0];  oper2=leaves[1]; }
  if (right_outer)
  { rightopt=alloc_opt(cdp, oper2, NULL, false);
    if (rightopt==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  return; }
    for (int i=0;  i<inherited_conds.count();  i++)
      rightopt->inherit_expr(cdp, *inherited_conds.acc0(i));
    rightopt->optimize(cdp, FALSE);
    if (oper2->maters!=MAT_NO)
    { roj_rowdist = new row_distinctor(); 
      if (roj_rowdist==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  return; }
      if (!roj_rowdist->prepare_rowdist_index(oper2, TRUE, 1, FALSE)) // unique, without flag
        request_error(cdp, OUT_OF_KERNEL_MEMORY);
    }
  }

  distribute_conditions(cdp, &jo, &variant, ordering_defining_table);
}

void t_optim_table::add_index_expression(const table_descr * td, t_express * ex, int indexnum, int partnum)
// Stores [ex] as the delimiting expression of part [partnum] of index [indexnum].
{ int i;
  if (indexnum<0 || indexnum>=td->indx_cnt)
    return;  // impossible, just validating the value
  if (tics==NULL)  // tics not allocated yet
  { tics=new t_table_index_conds[td->indx_cnt];
    if (tics==NULL) { error=TRUE;  return; }
    normal_index_count=0;  // is also in the constructor of t_optim_table
  }
 // search for the index access description for [indexnum]:
  for (i=0;  i<normal_index_count;  i++)
    if (tics[i].index_number==indexnum) break;
  t_table_index_conds * tic = &tics[i];
  if (i==normal_index_count)  // add a new index access description for [indexnum]:
  { if (i>=td->indx_cnt) return;  // impossible
    if (!tic->init1(td, indexnum)) { error=TRUE;  return; }
    normal_index_count++;
  }
 // store the expression in the proper part of the proper index access description:
  if (partnum>=tic->partcnt) return;  // impossible, just validating the value
  t_express ** pex = tic->part_conds[partnum].pconds.next();
  if (pex==NULL) { error=TRUE;  return; }
  *pex=ex;
}

BOOL t_optim_table::process_table_condition(cdp_t cdp, t_express * ex, t_table_cond_info * tci)
// Returns TRUE if found in a index part.
{ BOOL result = FALSE;
  if (ex->sqe==SQE_BINARY)
  { t_expr_binary * binex = (t_expr_binary *)ex;
    if (binex->oper.pure_oper==PUOP_AND)
    { BOOL res1 = process_table_condition(cdp, binex->op1, tci),
           res2 = process_table_condition(cdp, binex->op2, tci);
      result = res1 || res2;
      if (!res1 || !res2) tci->unused_part=TRUE;
    }
    else if (binex->oper.pure_oper==PUOP_OR)
    { tci->is_in_or++;
      result = process_table_condition(cdp, binex->op1, tci) ||
               process_table_condition(cdp, binex->op2, tci);
      tci->is_in_or--;
      if (result && !tci->is_in_or)  // this was a top-level OR
      // Add the complete expression becase there the index part in add branches
      { table_descr_auto tbdf(cdp, qet->tabnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
        if (!tbdf->me()) error=TRUE;
        else 
          for (int i=0;  i<tbdf->indx_cnt;  i++)
          { for (int p=0;  p<tbdf->indxs[i].partnum;  p++)
            { part_desc * pd = &tbdf->indxs[i].parts[p];
              if (same_expr(tbdf->indxs[tci->leading_index].parts[tci->leading_part].expr, pd->expr))
                add_index_expression(tbdf->me(), ex, i, p);
            }
          }
      }
    }
    else if (!binex->oper.all_disp && !binex->oper.any_disp && // disables subquery comparison and IN-predicate
             (binex->oper.pure_oper >= PUOP_EQ && binex->oper.pure_oper < PUOP_NE ||
              binex->oper.pure_oper == PUOP_PREF || binex->oper.pure_oper == PUOP_IS_NULL))
    { t_express * indexpr;
      t_refinfo ri1(NULL);  t_refinfo ri2(NULL);
      get_reference_info(binex->op1, &ri1);
      get_reference_info(binex->op2, &ri2);
      if      (ri1.onlytabkont==&qet->kont) 
        { indexpr=binex->op1;  if (ri2.onlytabkont==&qet->kont) return FALSE; }
      else if (ri2.onlytabkont==&qet->kont) 
        { indexpr=binex->op2;  if (ri1.onlytabkont==&qet->kont) return FALSE; }
      else return FALSE;
      table_descr_auto tbdf(cdp, qet->tabnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
      if (!tbdf->me()) error=TRUE;
      else if (!tci->is_in_or || tci->leading_index==-1)
      { int pure_oper = binex->oper.pure_oper;
        for (int i=0;  i<tbdf->indx_cnt;  i++)
        { dd_index * cc = &tbdf->indxs[i];
          if (pure_oper!=PUOP_IS_NULL || binex->oper.negate || cc->has_nulls) // cannot use an index without NULLS for the IS NULL predicate
           if (cc->has_nulls || pure_oper==PUOP_EQ || pure_oper==PUOP_GT || pure_oper==PUOP_GE || pure_oper==PUOP_PREF || pure_oper==PUOP_BETWEEN)
            for (int p=0;  p<cc->partnum;  p++)
              if (is_index_part(indexpr, cc->parts[p].expr, pure_oper))
                if (!tci->is_in_or)
                  { add_index_expression(tbdf->me(), ex, i, p);  result=TRUE; }
                else // add later, if contained in all parts joined by OR
                  { tci->leading_index=i;  tci->leading_part=p;  return TRUE; }
        }
      }
      else // is in or, but not the first
        return same_expr(indexpr, tbdf->indxs[tci->leading_index].parts[tci->leading_part].expr);
    } // relation
  } // binary expr
  else if (ex->sqe==SQE_TERNARY)
  { t_expr_ternary * ternex = (t_expr_ternary *)ex;  t_express * indexpr;
    t_refinfo ri1(NULL);  t_refinfo ri2(NULL);  t_refinfo ri3(NULL);
    get_reference_info(ternex->op1, &ri1);
    get_reference_info(ternex->op2, &ri2);
    get_reference_info(ternex->op3, &ri3);
    if (ri1.onlytabkont==&qet->kont) 
    { indexpr=ternex->op1;  
      if (ri2.onlytabkont==&qet->kont || ri3.onlytabkont==&qet->kont) return FALSE; 
    }
    // the reverse limitation may be added here
    else return FALSE;
    table_descr_auto tbdf(cdp, qet->tabnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
    if (!tbdf->me()) error=TRUE;
    else if (!tci->is_in_or || tci->leading_index==-1)
    { for (int i=0;  i<tbdf->indx_cnt;  i++)
      { for (int p=0;  p<tbdf->indxs[i].partnum;  p++)
        { part_desc * pd = &tbdf->indxs[i].parts[p];
          if (same_expr(indexpr, pd->expr))
            if (!tci->is_in_or)
              { add_index_expression(tbdf->me(), ex, i, p);  result=TRUE; }
            else // add later, if contained in all parts joined by OR
              { tci->leading_index=i;  tci->leading_part=p;  return TRUE; }
        }
      }
    }
    else // is in or, but not the first
      return same_expr(indexpr, tbdf->indxs[tci->leading_index].parts[tci->leading_part].expr);
  } // ternary expr
  else if (ex->sqe==SQE_FUNCT)
  { t_expr_funct * fex = (t_expr_funct *)ex;  db_kontext * tkont;  int docid_attr;
    if (fex->num==FCNUM_FULLTEXT)  // function is the FULLTEXT() predicate 
      if (resolve_access(((t_expr_attr*)fex->arg2)->kont, ((t_expr_attr*)fex->arg2)->elemnum, tkont, docid_attr))
        if (tkont==&qet->kont) // the current table is the document table
          if (!tci->is_in_or)
          { table_descr_auto tbdf(cdp, qet->tabnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
            if (!tbdf->me()) error=TRUE;
            else
            { dd_index * indx;  int i;
              for (i=0;  i<tbdf->indx_cnt;  i++)
              { indx = &tbdf->indxs[i];
                if (!indx->disabled && indx->partnum==1 && indx->parts[0].colnum == docid_attr)
                  break;
              }
              if (i==tbdf->indx_cnt) 
              { SET_ERR_MSG(cdp, "Document_ID");  request_error(cdp, INDEX_NOT_FOUND);  error=TRUE; 
              }
              else if (docid_index==-1 || i==docid_index) // unless referencing other fulltext system than a previous condition
              { docid_index=i;
                t_express ** pex = fulltext_conds.next();
                if (pex==NULL) error=TRUE;
                else
                  { *pex=ex;  result = TRUE; }
              }
            } 
          } // FULLTEXT predicate over the current table, on the top level
  } // function
  else if (ex->sqe == SQE_ATTR && ex->type==ATT_BOOLEAN)
  { t_expr_attr * atex = (t_expr_attr *)ex;
    t_express * indexpr;
    t_refinfo ri(NULL);
    get_reference_info(ex, &ri);
    if (ri.onlytabkont==&qet->kont) 
      indexpr=ex;
    else return FALSE;
    table_descr_auto tbdf(cdp, qet->tabnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
    if (!tbdf->me()) error=TRUE;
    else if (!tci->is_in_or || tci->leading_index==-1)
    { for (int i=0;  i<tbdf->indx_cnt;  i++)
      { dd_index * cc = &tbdf->indxs[i];
        for (int p=0;  p<cc->partnum;  p++)
          if (is_index_part(indexpr, cc->parts[p].expr, PUOP_EQ))
            if (!tci->is_in_or)
              { add_index_expression(tbdf->me(), ex, i, p);  result=TRUE; }
            else // add later, if contained in all parts joined by OR
              { tci->leading_index=i;  tci->leading_part=p;  return TRUE; }
      }
    }
    else // is in or, but not the first
      return same_expr(indexpr, tbdf->indxs[tci->leading_index].parts[tci->leading_part].expr);
  } // boolean column
  return result;
}

void t_optim_table::push_condition(cdp_t cdp, t_condition * acond, t_cond_state cond_state)
{ if (cond_state==COND_UNCLASSIF)
  { classify_condition(cdp, acond->expr, acond, this);
    if (acond->tabnum1==0 && acond->indnum1!=-1 && acond->tabnum2==-1) cond_state=COND_LIM1; else
    if (acond->tabnum2==0 && acond->indnum2!=-1 && acond->tabnum1==-1) cond_state=COND_LIM2; else
    cond_state=COND_POST;
  }
  if (cond_state==COND_LIM1 || cond_state==COND_LIM2)
  { t_table_cond_info tci;
    if (!process_table_condition(cdp, acond->expr, &tci) || tci.unused_part)
    // expression not used or not completely used in indexing
    { t_express ** psex = post_conds.next();
      if (psex==NULL) { error=TRUE;  return; }
      *psex = acond->expr;
    }
  }
  else
  { t_express ** psex = post_conds.next();
    if (psex==NULL) return; // error
    *psex = acond->expr;
  }
}

void t_optim_sysview::push_condition(cdp_t cdp, t_condition * acond, t_cond_state cond_state)
{ t_express ** psex = post_conds.next();
  if (psex==NULL) return; // error
  *psex = acond->expr;
}

void t_optim::push_condition(cdp_t cdp, t_condition * acond, t_cond_state cond_state)
{ t_express ** psex = inherited_conds.next();
  if (psex==NULL) { error=TRUE;  return; } // error
  *psex = acond->expr;
}

void t_optim::inherit_expr(cdp_t cdp, t_express * ex)
// Stores all top AND-conditions from the expression as inherited (recursive).
{ if (ex->sqe == SQE_BINARY)
  { t_expr_binary * binex = (t_expr_binary *)ex;
    if (binex->oper.pure_oper==PUOP_AND)
    { inherit_expr(cdp, binex->op1);
      inherit_expr(cdp, binex->op2);
      return;
    }
  }
  t_condition cond;  cond.expr=ex;
  push_condition(cdp, &cond, COND_UNCLASSIF);
}

t_optim * alloc_opt(cdp_t cdp, t_query_expr * qe, t_order_by * order_In, bool user_order)
// Returns NULL on no-memory error.
{ t_optim * opt = NULL;
#if SQL3
  if (qe->order_by /*&& (qe->limit_count || qe->limit_offset)*/)
    order_In = qe->order_by;
#endif
  switch (qe->qe_type)
  { case QE_TABLE:
      opt = new t_optim_table((t_qe_table*)qe, order_In);  break;
    case QE_SYSVIEW:
      opt = new t_optim_sysview((t_qe_sysview*)qe, order_In);  break;
    case QE_JT_INNER:
      opt = new t_optim_join_inner(qe, order_In, user_order);  break;
    case QE_JT_LEFT:  case QE_JT_RIGHT:  case QE_JT_FULL:   case QE_JT_UNION:
      opt = new t_optim_join_outer((t_qe_join*)qe, order_In, user_order);  break;
    case QE_SPECIF:
    { t_qe_specif * qes = (t_qe_specif*)qe;
      if (qes->grouping_type!=GRP_NO)
        opt = new t_optim_grouping(qes, order_In);
      else if (qes->from_tables)
      { opt = alloc_opt(cdp, qes->from_tables, order_In, qes->user_order || user_order);
        if (opt==NULL) break;
        if (qes->where_expr!=NULL && opt->optim_type!=OPTIM_JOIN_INNER)
          opt->inherit_expr(cdp, qes->where_expr);
          // OPTIM_JOIN_INNER inherits in a different way
#if SQL3
        if (opt->optim_type!=OPTIM_ORDER && opt->optim_type!=OPTIM_LIMIT && opt->optim_type!=OPTIM_GROUPING)
#endif
          opt->own_qe = qe;  // points to the top qe of the SELECT chain
      }
      else
        opt=new t_optim_null(qe);
     // create global row distinctor, if required:
      if (qes->distinct)
      { if (opt->main_rowdist!=NULL) delete opt->main_rowdist;  // delete distinctor from lower level
        opt->main_rowdist = new row_distinctor(); // create a new one...
        if (opt->main_rowdist==NULL) opt->error=TRUE;
        else if (!opt->main_rowdist->prepare_rowdist_index(qes, TRUE, 1, FALSE))
          { request_error(cdp, OUT_OF_KERNEL_MEMORY);  opt->error=TRUE; }
        opt->make_distinct();
      }
      break;
    }
    case QE_UNION:
    { t_qe_bin * qeb = (t_qe_bin*)qe;
      if (qeb->all_spec) // UNION ALL
        opt = new t_optim_union_all (qeb, order_In);
      else  // distinct UNION:
        opt = new t_optim_union_dist(qeb, order_In);
      break;
    }
    case QE_EXCEPT:  case QE_INTERS:
      opt = new t_optim_exc_int((t_qe_bin*)qe, order_In);  break;
  }
#if SQL3
 // for ORDER BY or LIMIT create the specific t_optim_... unless reduced in special cases (ORDER above ORDER without LIMITs, ORDER and/or LIMIT included into GROUPing)
  if (qe->order_by || qe->limit_count || qe->limit_offset)
  { //if (opt->optim_type!=OPTIM_GROUPING /*&& opt->optim_type!=OPTIM_ORDER && opt->optim_type!=OPTIM_LIMIT*/)  
    // ORDER-LIMIT and ORDER cannot be reduced
    // GROUPING-ORDER-LIMIT and ORDER cannot be reduced
    if (qe->qe_type!=QE_SPECIF || ((t_qe_specif*)qe)->grouping_type==GRP_NO)  // the local ORDER-LIMIT in GROUP BY will be included into grouping
      if (true || qe->limit_count || qe->limit_offset || opt->optim_type!=OPTIM_ORDER || opt->own_qe->limit_count || opt->own_qe->limit_offset)  // reducing ORDER-ORDER without LIMIT on any level
      // above reduction was wrong, the upper ORDER is more important and must be preserved
       // add order/limit level:
        if (qe->order_by)
        {// remove distinct, will be included into order:
#ifdef STOP  // cannot remove the distinctor because the post-ordering may be later removed by the index passing
          if (qe->qe_type==QE_SPECIF && ((t_qe_specif*)qe)->distinct && opt->main_rowdist!=NULL)
          { delete opt->main_rowdist;  // delete distinctor from lower level
            opt->main_rowdist=NULL;
            opt->make_not_distinct();
          }
#endif
          opt = new t_optim_order(qe, order_In, opt);
        }
        else 
          opt = new t_optim_limit(qe, order_In, opt);
      else
        opt->own_qe = qe;  // points to the top qe of the SELECT chain
  }
#endif
  return opt; 
}


#ifdef STOP
void t_optim_table::optimize(cdp_t cdp, BOOL ordering_defining_table)
{ int j, docid_attr;
 // select the best index limit:
  int best_qual_ind = -1;
  if (interval_calc.count() > 0)
  { int curr_indnum, other_indnum;  uns32 used_indnum_map = 0;
    other_indnum = interval_calc.acc0(0)->indnum;
    int best_qual=0;
    do // process limitations based on index number other_indnum
    { curr_indnum=other_indnum;  other_indnum=-1;
      BOOL lim_eq=FALSE, lim_lo=FALSE, lim_hi=FALSE, lim_full=FALSE;
      for (j=0;  j<interval_calc.count();  j++)
      { t_ind_limit * il = interval_calc.acc0(j);
        if (il->indnum==curr_indnum)
        { if (!il->partnum)  // only 1st part evaluated
          { switch (il->expr->sqe)
            { case SQE_BINARY:
              { t_expr_binary * binex = (t_expr_binary *)il->expr;
                switch (binex->oper.pure_oper)
                { case PUOP_EQ:                lim_eq=TRUE;  break;
                  case PUOP_IS_NULL:           lim_eq=TRUE;  break;
                  case PUOP_GE:  case PUOP_GT: lim_lo=TRUE;  break;
                  case PUOP_LE:  case PUOP_LT: lim_hi=TRUE;  break;
                  case PUOP_PREF:              lim_lo=lim_hi=TRUE;  break;
                }
                break;
              }
              case SQE_TERNARY:
              { t_expr_ternary * ternex = (t_expr_ternary *)il->expr;
                if (ternex->oper.pure_oper==PUOP_BETWEEN) lim_lo=lim_hi=TRUE;  
                break;
              }
              case SQE_FUNCT:
              { t_expr_funct * fex = (t_expr_funct *)il->expr;  db_kontext * tkont;
                if (fex->num==FCNUM_FULLTEXT) 
                  if (resolve_access(((t_expr_attr*)fex->arg2)->kont, ((t_expr_attr*)fex->arg2)->elemnum, tkont, docid_attr))
                    lim_full=TRUE; 
                break;
              }
            }
          }
        }
        else if (!(used_indnum_map & 1<<il->indnum))
          other_indnum=il->indnum;  // regiter another index number
      }
      int qual;
      if    (lim_full) qual=4;
      else if (lim_eq) qual=3;
      else if (lim_lo) qual=lim_hi ? 2 : 1;
      else if (lim_hi) qual=1;
      else             qual=0;
      if (qual>best_qual)
        { best_qual=qual;  best_qual_ind=curr_indnum; }
      used_indnum_map |= 1<<curr_indnum;
    } while (other_indnum!=-1);
   // determine the part number limit:
    int partnum_limit = best_qual>=3 ? 1 : 0;  // highest part number which can be used in index
    BOOL found;
    do
    { found=FALSE;
      for (j=0;  !found && j<interval_calc.count();  j++)
      { t_ind_limit * il = interval_calc.acc0(j);
        if (il->indnum==best_qual_ind && il->partnum==partnum_limit) // not satisfied for fulltext
        { t_expr_binary * binex = (t_expr_binary *)il->expr;
          if (binex->oper.pure_oper == PUOP_EQ || binex->oper.pure_oper == PUOP_IS_NULL && !binex->oper.negate)
            { found=TRUE;  partnum_limit++; }
        }
      }
    } while (found);
   // move limitations based on other indicies to post conditions:
   // move limitations based on part number limit, too
    for (j=0;  j<interval_calc.count();  )
    { t_ind_limit * il = interval_calc.acc0(j);
      if (il->indnum!=best_qual_ind || il->partnum > partnum_limit)
      { t_express ** psex = post_conds.next();
        if (psex==NULL) return; // error
        *psex = il->expr;
        interval_calc.delet(j);
      }
      else j++;
    }
  }
  else // no index limit!
    best_qual_ind=-1;

 // define post_order:
  if (ordering_defining_table && order_!=NULL)  // the 1st table in the join order
    if (&qet->kont==order_->order_tabkont)
    { if (best_qual_ind==-1 && order_->order_indnum!=-1)
          best_qual_ind=       order_->order_indnum;
      if (best_qual_ind==      order_->order_indnum && best_qual_ind!=-1)
        order_->post_sort=FALSE;
    }
  selected_index_num=best_qual_ind;
 // allocate space for index keys, find auxiliary index for fulltext:
  if (selected_index_num!=-1)  // normal index (not fulltext) will be used
  { table_descr_auto auto_descr(cdp, qet->tabnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
    if (auto_descr->me())
      if (selected_index_num>=0)
      { dd_index * indx = &auto_descr->constr[selected_index_num].index;
        startkey=(tptr)corealloc(indx->keysize, 85);
        stopkey =(tptr)corealloc(indx->keysize, 85);
      }
      else // fulltext, search aux index
      { dd_index * indx;  int i;
        for (i=0;  i<auto_descr->indx_cnt;  i++)
        { indx = &auto_descr->indxs[i];
          if (!indx->disabled && indx->partnum==1 && indx->part[0].colnum == docid_attr)
            break;
        }
        if (i==auto_descr->indx_cnt) 
        { selected_index_num=-1;
          SET_ERR_MSG(cdp, "Document_ID");  request_error(cdp, INDEX_NOT_FOUND);  error=TRUE; 
        }
        else docid_index=i;
      }
    else selected_index_num=-1;
  }
}
#endif

void t_optim_table::add_distinct_cond(t_express * ex)
// Adds a condition expression into [post_conds] unless it has already been there
{ int i=0;
  while (i<post_conds.count())
    if (*post_conds.acc0(i) == ex) return;
    else i++;
  t_express ** psex = post_conds.next();
  if (psex==NULL) { error=TRUE;  return; }
  *psex = ex;
}

int evaluate_expression_complexity(const t_express * ex)
// Returns an extimated complexity score of an expression, used for ordering the post-conditions.
{ int val=0;
  switch (ex->sqe)
  { case SQE_SCONST:
    case SQE_CURRREC:
    case SQE_LCONST:
    case SQE_DYNPAR:
    case SQE_NULL:
      return 0;
    case SQE_SEQ:
      return 2;
    case SQE_TERNARY:
      val = evaluate_expression_complexity(((t_expr_ternary*)ex)->op1) +
            evaluate_expression_complexity(((t_expr_ternary*)ex)->op2);
      if (((t_expr_ternary*)ex)->op3!=NULL)
        val += evaluate_expression_complexity(((t_expr_ternary*)ex)->op3);
      break;
    case SQE_BINARY:
      val = evaluate_expression_complexity(((t_expr_binary*)ex)->op1);
      if (((t_expr_binary*)ex)->op2!=NULL)    // may be NULL for IN (list)
        val += evaluate_expression_complexity(((t_expr_binary*)ex)->op2);
      break;
    case SQE_UNARY:
      return evaluate_expression_complexity(((t_expr_unary*)ex)->op);
    case SQE_AGGR:
      return 10;
    case SQE_FUNCT:
    { val = 1;
      t_expr_funct * fex = (t_expr_funct*)ex;
      if (fex->arg1!=NULL) val+=evaluate_expression_complexity(fex->arg1);
      if (fex->arg2!=NULL) val+=evaluate_expression_complexity(fex->arg2);
      if (fex->arg3!=NULL) val+=evaluate_expression_complexity(fex->arg3);
      if (fex->arg4!=NULL) val+=evaluate_expression_complexity(fex->arg4);
      if (fex->arg5!=NULL) val+=evaluate_expression_complexity(fex->arg5);
      if (fex->arg6!=NULL) val+=evaluate_expression_complexity(fex->arg6);
      if (fex->arg7!=NULL) val+=evaluate_expression_complexity(fex->arg7);
      if (fex->arg8!=NULL) val+=evaluate_expression_complexity(fex->arg8);
      break;
    }
    case SQE_ATTR:  case SQE_ATTR_REF:
    case SQE_MULT_COUNT:    case SQE_VAR_LEN:    case SQE_DBLIND:
      return 10;
    case SQE_POINTED_ATTR:
      return 20;
    case SQE_CASE:
    { t_expr_case * cex = (t_expr_case *)ex;
      if (cex->val1  !=NULL) val+=evaluate_expression_complexity(cex->val1);
      if (cex->val2  !=NULL) val+=evaluate_expression_complexity(cex->val2);
      if (cex->contin!=NULL) val+=evaluate_expression_complexity(cex->contin);
      break;
    }  
    case SQE_SUBQ:
      return 500;
    case SQE_CAST:
      return evaluate_expression_complexity(((t_expr_cast*)ex)->arg);
    case SQE_ROUTINE:
      return 1000;
  }
  return val;
}

#define IS_SINGLE_VALUE(oper)  (oper.pure_oper == PUOP_EQ || oper.pure_oper == PUOP_IS_NULL && !oper.negate)

static bool comp_index_and_order(dd_index * cc, int offset, dd_index * oindx)
// Checks if index [cc] starting from part [offset] contains ordering [oindx]
{ bool has_non_null_col = false;  // used for indexes not containing NULL values
 // check if the index has enoung parts:
  if (cc->partnum < offset+oindx->partnum)
    return false;
  for (int part=0;  part<oindx->partnum;  part++)
  {// compare the parts: 
    if (!same_expr(oindx->parts[part].expr, cc->parts[offset+part].expr) ||
        oindx->parts[part].desc!=cc->parts[offset+part].desc)
      return false;
   // look for non-nullable column:   
    if (oindx->parts[part].expr->sqe==SQE_ATTR)
      if (!((t_expr_attr*)oindx->parts[part].expr)->nullable)
        has_non_null_col=true;
  }  
  if (cc->has_nulls)  
    return true;
  else // the index must refer to (at least one) non-null column, otherwise the index is unusable  
    return has_non_null_col;
} 

/*
   Optimization of ORDER BY when the same index is used in WHERE
   - index I has the form e1,e2,...,en
   - conditions e1=k1 and ... ex=kx are part of the WHERE condition (or e1 IS NULL ...)
   - ORDER BY clause is ey,..,ez
   - y<=x+1, z<=n
   - either index contains NULL values or some of the ey,..,ez expressions is a non-nullable column
   When all above conditions are met then abd index I is the only index used when passing the table then
   ORDER BY is implied and post-sorting is disabled.
*/

void t_optim_table::optimize(cdp_t cdp, BOOL ordering_defining_table)
// Find the set of indicies to be used for accessing the table.
// Conditions not used for creating index intervals moves among post_conds.
{ int i;
  if (qet->tabnum==NOTBNUM) return;
  table_descr_auto tbdf(cdp, qet->tabnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
  if (!tbdf->me()) { error=TRUE;  return; }
 // Validate the set of conditions for each index:
  for (i=0;  i<normal_index_count;  i++)
  { dd_index * indx = &tbdf->indxs[tics[i].index_number];
   // Passing parts of the index key from the 1st: missing condition or a condition other than "=" or "IS NULL" disables all following conditions
    bool must_not_continue = false;
    for (int p=0;  p<indx->partnum;  p++)
    { t_table_index_part_conds * tipcs = &tics[i].part_conds[p];
      if (must_not_continue) // move conditions
      { for (int j=0;  j<tipcs->pconds.count();  j++)
          add_distinct_cond(*tipcs->pconds.acc0(j));
        tipcs->pconds.~t_expr_dynar();
      }
      if (!tipcs->pconds.count() || tipcs->pconds.count()>1) must_not_continue=true;
      else
      { t_express * ex = *tipcs->pconds.acc0(0);
        if (ex->sqe!=SQE_BINARY) must_not_continue=true;
        else
        { t_expr_binary * bex = (t_expr_binary*)ex;
          if (!IS_SINGLE_VALUE(bex->oper))
            must_not_continue=true;
        }
      }
    }
  }
 // check is there is an unique index limited to a single value:
  int single_row_limitation_index = -1;  // none found so far
  for (i=0;  i<normal_index_count;  i++)
  { dd_index * indx = &tbdf->indxs[tics[i].index_number];
    if (indx->ccateg==INDEX_UNIQUE || indx->ccateg==INDEX_PRIMARY)
    { bool limited_ok = true;
      for (int p=0;  p<indx->partnum;  p++)
      { t_table_index_part_conds * tipcs = &tics[i].part_conds[p];
        if (!tipcs->pconds.count())
          { limited_ok=false;  break; }
        t_express * ex = *tipcs->pconds.acc0(0);  
        if (ex->sqe!=SQE_BINARY || !IS_SINGLE_VALUE(((t_expr_binary*)ex)->oper))
          { limited_ok=false;  break; }
      }
      if (limited_ok) 
        { single_row_limitation_index=i;  break; } 
    }
  }
  if (single_row_limitation_index!=-1)  // found
  { for (i=0;  i<normal_index_count;  i++)
      if (i!=single_row_limitation_index)
      { dd_index * indx = &tbdf->indxs[tics[i].index_number];
        for (int p=0;  p<indx->partnum;  p++)
        { t_table_index_part_conds * tipcs = &tics[i].part_conds[p];
          { for (int j=0;  j<tipcs->pconds.count();  j++)
              add_distinct_cond(*tipcs->pconds.acc0(j));
            tipcs->pconds.~t_expr_dynar();
          }
        }
        delete [] tics[i].part_conds;
        tics[i].part_conds = NULL;  // destructor must not destroy this
      }
    memcpy(&tics[0], &tics[single_row_limitation_index], sizeof(t_table_index_conds));
    if (single_row_limitation_index!=0) // must prevent the destructor to destruct this, copies to tics[0]
      tics[single_row_limitation_index].part_conds = NULL;
    normal_index_count=1;
  }
 // remove indicies without conditions:
  for (i=0;  i<normal_index_count; )
    if (tics[i].part_conds[0].pconds.count()==0)
    { delete [] tics[i].part_conds;
      normal_index_count--;
      if (i<normal_index_count) //tics[i]=tics[normal_index_count];
        memcpy(&tics[i], &tics[normal_index_count], sizeof(t_table_index_conds));
      tics[normal_index_count].part_conds=NULL;
    }
    else i++;
 // add conditions used in truncated indexes to post_conds:
  for (i=0;  i<normal_index_count;  i++)
  { dd_index * indx = &tbdf->indxs[tics[i].index_number];
    for (int p=0;  p<indx->partnum;  p++)
    { t_table_index_part_conds * tipcs = &tics[i].part_conds[p];
      if (SPECIFLEN(indx->parts[p].expr->type) && indx->parts[p].expr->specif.length>MAX_INDPARTVAL_LEN)
        for (int j=0;  j<tipcs->pconds.count();  j++)
          add_distinct_cond(*tipcs->pconds.acc0(j));
    }
  }
    
 // add ordering index if there is not any other index, test for automatic indexing
  if (ordering_defining_table && order_!=NULL)  // the 1st table in the join order
    if (&qet->kont==order_->order_tabkont)
    { if (!normal_index_count && docid_index==-1 && order_->order_indnum!=-1)
      { if (tics==NULL)  // if tics not allocated yet
        { tics=new t_table_index_conds[tbdf->indx_cnt];
          if (tics==NULL) { error=TRUE;  return; }
        }
        t_table_index_conds * tic = &tics[normal_index_count];
        if (!tic->init1(tbdf->me(), order_->order_indnum)) error=TRUE;  
        normal_index_count++;  // no conditions
      } 
      if (normal_index_count==1 && docid_index==-1)  // index either defined by condition or added here
        if (tics[0].index_number==order_->order_indnum) // simple case: ordering is a prefix of the index key
          order_->post_sort=FALSE;
        else  // checking for a complex case  
        { dd_index * indx = &tbdf->indxs[tics[0].index_number];  // the only index
         // count the parts of the index with a single specified value
          int fixed_parts=0;
          while (fixed_parts<indx->partnum)
          { t_table_index_part_conds * tipcs = &tics[0].part_conds[fixed_parts];
            if (tipcs->pconds.count()!=1)
              break;
            t_express * ex = *tipcs->pconds.acc0(0);  
            if (ex->sqe!=SQE_BINARY || !IS_SINGLE_VALUE(((t_expr_binary*)ex)->oper))
              break;
            fixed_parts++;
           // now, compare index key from [fixed_parts] to the ordering key 
            if (comp_index_and_order(indx, fixed_parts, &order_->order_indx))
              order_->post_sort=FALSE;
          }
        }  
    }
 // allocate space for key values:
  for (i=0;  i<normal_index_count; i++) 
    if (!tics[i].init2(tbdf->me())) error=TRUE;
 // optimize the ordering of post-conditions:
  for (int start=0;  start+1<post_conds.count();  start++)  // find the best in <start, end> and move it to the [start] position
  { int best_val=1000000000, best_pos, pos, val;
    for (pos=start;  pos<post_conds.count();  pos++)
    { val=evaluate_expression_complexity(*post_conds.acc0(pos));
      if (val<best_val) { best_val=val;  best_pos=pos; }
    }
    if (best_pos!=start)  // swap them
    { t_express * ex = *post_conds.acc0(best_pos);  
      *post_conds.acc0(best_pos) = *post_conds.acc0(start);
      *post_conds.acc0(start) = ex;
    }  
  }
}

BOOL t_hashed_recordset::add_record(trecnum recnum, int position, uns32 ft_weight, uns32 ft_pos)
{ if (contains >= limit)
    if (!expand()) return FALSE;
  unsigned ind = hash(recnum);
  char * ptr = table + itemsize * ind;
  uns32 map;
  while (TRUE)
  { if (*(trecnum*)ptr==recnum)    // is here
    { ptr+=sizeof(trecnum);
      map=*(uns32*)ptr;
      map |= 1<<position;
      break;
    }
    if (*(trecnum*)ptr==NORECNUM)  // add here
    { *(trecnum*)ptr=recnum;  ptr+=sizeof(trecnum);  contains++;
      map = 1<<position;
      break;
    }
    if (++ind==table_size)
      { ind=0;  ptr=table; }
    else
      ptr+=itemsize;
  }
  if (fulltext_position!=-1)
  { int mapsize = itemsize-sizeof(trecnum)-2*sizeof(uns32);
    memcpy(ptr, &map, mapsize);
    *(uns32*)(ptr+mapsize)               = ft_weight;
    *(uns32*)(ptr+mapsize+sizeof(uns32)) = ft_pos;
  }
  else
    memcpy(ptr, &map, itemsize-sizeof(trecnum));
  return TRUE;
}

BOOL t_hashed_recordset::expand(void)
{ unsigned new_table_size = table_size==0 ? 100 : table_size * 2;
  char * new_table = (char*)corealloc(itemsize * new_table_size + sizeof(uns32)-1, 47); // reading uns32 map even if the real map is smaller
  if (!new_table) return FALSE;
  unsigned old_table_size = table_size;
  table_size = new_table_size;  // changes the hash()
  memset(new_table, 0xff, itemsize * table_size);  // faster than assigning NORECNUM
  unsigned i;  char * ptr;
  for (i=0, ptr=table;  i<old_table_size;  i++, ptr+=itemsize)
    if (*(trecnum*)ptr!=NORECNUM)
    { unsigned new_ind = hash(*(trecnum*)ptr);
      char * new_ptr = new_table + itemsize * new_ind;
      while (*(trecnum*)new_ptr!=NORECNUM)
      { if (++new_ind==new_table_size)
          { new_ind=0;  new_ptr=new_table; }
        else 
          new_ptr+=itemsize;
      }
      memcpy(new_ptr, ptr, itemsize);
    }
  if (table) corefree(table);  table=new_table;
  limit=table_size / 5 * 4;
  return TRUE;
}

trecnum t_hashed_recordset::get_next(cdp_t cdp, uns32 & map)  
// Returns stored records belonging to the index in [completed_position].
// Returns NORECNUM if no more records available
{ tptr ptr = table + passing_position*itemsize;
  while (passing_position++<table_size)
  { trecnum rec=*(trecnum*)ptr;
    if (rec!=NORECNUM)
    { map=*(uns32*)(ptr+sizeof(trecnum));
      if (map & (1<<completed_position))
      { if (fulltext_position!=-1)  // restore the weigth
        { cdp->__fulltext_weight   = *(uns32*)(ptr+itemsize-2*sizeof(uns32));
          cdp->__fulltext_position = *(uns32*)(ptr+itemsize-sizeof(uns32));
        }
        return rec;
      }
    }
    ptr+=itemsize;
  }
  return NORECNUM;  // mo more records
}

void t_optim_sysview::optimize(cdp_t cdp, BOOL ordering_defining_table)
// Copy inherited conditions to post_conds
{ for (int i=0;  i<inherited_conds.count();  i++)
    inherit_expr(cdp, *inherited_conds.acc0(i));
}
////////////////////// gathering conditions for a variant //////////////////////
class variant_dispatcher
{ uns32 current_map;
  int curr_depth, max_depth;

 public:
  variant_dispatcher(void) { current_map=0;  curr_depth=0;  max_depth=-1; }
  // Prepares decision map for the first variant.

  BOOL get_direction(void)
  // Returns decision for the current depth, TRUE for right
  { max_depth=curr_depth;
    return current_map & (1<<curr_depth);
  }

  void depth_plus (void)  { curr_depth++; }
  void depth_minus(void)  { curr_depth--; }

  BOOL next_variant(void)
  // Prepares decision map for the next variant.
  // Returns TRUE if next variant exists.
  { curr_depth=max_depth;
    while (curr_depth >= 0)
    { if (current_map & (1<<curr_depth)) current_map &= ~(1<<curr_depth);
      else // 0 found, change to 1 and exit
      { current_map |= (1<<curr_depth);
        curr_depth=0;  max_depth=-1;
        return TRUE;
      }
      curr_depth--;
    }
    return FALSE;
  }
};

t_condition * t_variant::store_condition(t_express * ex)
// Stores the condition element in the variant, classifies it.
{ t_condition * cond = conds.next();
  if (cond==NULL) return NULL; //error
  cond->expr=ex;  cond->generated=FALSE;
  return cond;
}

void t_variant::mark_core(void)
{ conds.mark_core();
  for (unsigned i=0;  i<leave_cnt;  i++) 
    if (join_steps[i]) join_steps[i]->mark_core();  // NULL in variant [0]
}

void t_variant::mark_disk_space(cdp_t cdp) const
{ for (unsigned i=0;  i<leave_cnt;  i++) 
    if (join_steps[i]) join_steps[i]->mark_disk_space(cdp);  // NULL in variant [0]
}

static BOOL clear_advance_direction(t_express * ex, BOOL advance)
// If advance==FALSE, sets all [direction] flags to FALSE.
// If advance==TRUE, updates the direction flags to the next variant
//    returns TRUE iff overflow occured (there is no next variant).
{ if (ex->sqe == SQE_BINARY)
  { t_expr_binary * binex = (t_expr_binary *)ex;
    if (binex->oper.pure_oper==PUOP_AND || binex->oper.pure_oper==PUOP_OR && binex->opt_info==OR_DIVIDE)
      if (advance)
        if (binex->oper.pure_oper==PUOP_AND)
        { if (clear_advance_direction(binex->op1, TRUE))
            return clear_advance_direction(binex->op2, TRUE);
          return FALSE;
        }
        else // OR_DIVIDE
        { if (!binex->direction)
          { if (clear_advance_direction(binex->op1, TRUE))
              binex->direction=TRUE;  
            return FALSE;
          }
          else
          { if (clear_advance_direction(binex->op2, TRUE))
            { binex->direction=FALSE;  
              return TRUE;
            }
            return FALSE;
          }

        }
      else // clearing [direction] only
      { binex->direction=FALSE;
        clear_advance_direction(binex->op1, FALSE);
        clear_advance_direction(binex->op2, FALSE);
        return FALSE;
      }
  }
  return advance;
}

void t_optim_join::copy_condition(cdp_t cdp, t_variant * variant, t_express * ex)
{ if (ex->sqe == SQE_BINARY)
  { t_expr_binary * binex = (t_expr_binary *)ex;
    if (binex->oper.pure_oper==PUOP_AND)
    { copy_condition(cdp, variant, binex->op1);
      copy_condition(cdp, variant, binex->op2);
      return;
    }
    if (binex->oper.pure_oper==PUOP_OR && binex->opt_info==OR_DIVIDE)
    { if (binex->direction)
        copy_condition(cdp, variant, binex->op2);
      else
        copy_condition(cdp, variant, binex->op1);
      return;
    }
  }
 // add the condition element to the variant, classify it:
  t_condition * cond = variant->store_condition(ex);
  classify_condition(cdp, ex, cond, this);
  return;
}

void t_optim_join::add_condition_ex(cdp_t cdp, t_express * ex, t_variant * variant)
// Stores all top AND-conditions from the expression in the variant (recursive).
{ if (ex->sqe == SQE_BINARY)
  { t_expr_binary * binex = (t_expr_binary *)ex;
    if (binex->oper.pure_oper==PUOP_AND)
    { add_condition_ex(cdp, binex->op1, variant);
      add_condition_ex(cdp, binex->op2, variant);
      return;
    }
  }
  t_condition * cond = variant->store_condition(ex);
  classify_condition(cdp, ex, cond, this);
}

void t_optim_join_inner::add_condition_qe(cdp_t cdp, t_query_expr * qe)
// Stores all conditions from the subtree in the variant (recursive).
{ switch (qe->qe_type)
  { case QE_SPECIF:
    { t_qe_specif * qes = (t_qe_specif *)qe;
      if (qes->grouping_type!=GRP_NO/* || qes->distinct*/) return;  // both wrong, with is the distinct joint is not optimized, without it the condiont from nested joins separated by DISTINCT are mixed
      if (qes->where_expr!=NULL)
        add_condition_ex(cdp, qes->where_expr, variants.acc0(0));
      add_condition_qe(cdp, qes->from_tables);
      break;
    }
    case QE_JT_INNER:
    { t_qe_join * qej = (t_qe_join *)qe;
      if (qej->join_cond!=NULL) 
        add_condition_ex(cdp, qej->join_cond, variants.acc0(0));
      add_condition_qe(cdp, qej->op1);
      add_condition_qe(cdp, qej->op2);
      break;
    }
  }
}

void t_optim_join_inner::optimize(cdp_t cdp, BOOL ordering_defining_table)
{ int i;  t_variant * base_variant;
#if SQL3
 // search for leaves: start on the topmost join, because starting on own_qe causes problem when there is a DISTINCT, 
 // ORDER or LIMIT above the first join
  t_query_expr * top_join_qe = own_qe;
  while (top_join_qe->qe_type==QE_SPECIF)
    top_join_qe = ((t_qe_specif*)top_join_qe)->from_tables;
  find_subtree_leaves(top_join_qe);  // now, top_join_qe->qe_type==QE_JT_INNER
#else
  find_subtree_leaves(own_qe, true);
#endif
  base_variant = variants.next();
  if (base_variant==NULL) { error=TRUE;  return; }
  base_variant->init(leave_count);
  add_condition_qe(cdp, own_qe);  // adding conditions

 // add the inherited condition to the base variant:
  for (i=0;  i<inherited_conds.count();  i++)
  { t_condition * cond = base_variant->store_condition(*inherited_conds.acc0(i));
    classify_condition(cdp, *inherited_conds.acc0(i), cond, this);
  }

 // create variants of conditions:
  for (i=0;  i<base_variant->conds.count();  i++)
    clear_advance_direction(base_variant->conds.acc0(i)->expr, FALSE);
  do
  { t_variant * variant = variants.next();
    if (variant==NULL) { error=TRUE;  break; }
    variant->init(leave_count);
    base_variant = variants.acc0(0);  // dynar may have been reallocated!!
    for (i=0;  i<base_variant->conds.count();  i++)
      copy_condition(cdp, variant, base_variant->conds.acc0(i)->expr);
   // advance the direction flags:
    for (i=0;  i<base_variant->conds.count();  i++)
      if (!clear_advance_direction(base_variant->conds.acc0(i)->expr, TRUE))
        break;
  } while (i<base_variant->conds.count());

 // optimize each variant:
  if (variants.count() > 2)
  { ordering_defining_table=FALSE;
    variant_recdist = new t_recdist;
    if (variant_recdist==NULL) error=TRUE;
  }
  for (i=1;  i<variants.count();  i++)
    optimize_variant(cdp, variants.acc0(i), ordering_defining_table);
 // unselect any variant, used by open_close_passing before reset()!
  curr_variant_num=-1;
}

void t_optim_bin::optimize(cdp_t cdp, BOOL ordering_defining_table)
{ int i;
  op1opt=alloc_opt(cdp, top->op1, order_, false);
  if (op1opt==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  return; }
  top->kont.position=0;
  for (i=0;  i<inherited_conds.count();  i++)
    op1opt->inherit_expr(cdp, *inherited_conds.acc0(i));
  op1opt->optimize(cdp, ordering_defining_table);

  op2opt=alloc_opt(cdp, top->op2, order_, false);
  if (op2opt==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  return; }
  top->kont.position=1;
  for (i=0;  i<inherited_conds.count();  i++)
    op2opt->inherit_expr(cdp, *inherited_conds.acc0(i));
  op2opt->optimize(cdp, FALSE);
}

void t_optim_union_all::optimize(cdp_t cdp, BOOL ordering_defining_table)
{
  t_optim_bin::optimize(cdp, FALSE);
}

void t_optim_union_dist::optimize(cdp_t cdp, BOOL ordering_defining_table)
{ if (!rowdist.prepare_rowdist_index(top, TRUE, 1, FALSE))
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE; }
 // optimize leaves:
  t_optim_bin::optimize(cdp, FALSE);
}

void t_optim_exc_int::optimize(cdp_t cdp, BOOL ordering_defining_table)
{ 
  if (!rowdist.prepare_rowdist_index(top, !top->all_spec, top->mat_extent ? top->mat_extent : top->rec_extent, TRUE))
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE; }
 // optimize leaves:
  t_optim_bin::optimize(cdp, ordering_defining_table);
}

void t_optim_grouping::optimize(cdp_t cdp, BOOL ordering_defining_table)
{ opopt=alloc_opt(cdp, qeg->from_tables, order_, qeg->user_order);
  if (opopt==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  return; }
#ifdef STABILIZED_GROUPING
 // The inherited conditions can be posted to lower levels only if the do not depend on outer data
 // Otherwise, the grouping result would be different for every outer ref. value -> instable  
#if SQL3
  for (int i=0;  i<inherited_conds.count();  )
  { t_refinfo ri(NULL, RIT_GROUPING_OPT);
    ri.groupkont=&qeg->kont;  
    get_reference_info(*inherited_conds.acc0(i), &ri);
    if (ri.stat==TRS_NO || ri.stat==TRS_SINGLE && ri.onlytabkont==&qeg->kont)
    { if (!ri.has_aggr_on_groupkont)  // inherit
        opopt->inherit_expr(cdp, *inherited_conds.acc0(i));  
      else  // add to inherited_having_conds (cannot add to having_expr, problems with ownership)
      { t_express ** pex = inherited_having_conds.next();
        if (!pex) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  return; }
        *pex=*inherited_conds.acc0(i);
      }
      inherited_conds.delet(i);
    }
    else
      i++;
  }
  // inherited_conds now contains only the conditions containing outer references
#endif  // !SQL3
#else  // !STABILIZED_GROUPING
 // The inherited conditions should be analysed and those not containing
 // aggregates should be inherited further.
  for (int i=0;  i<inherited_conds.count();  i++)
  { t_refinfo ri(NULL);
    ri.groupkont=&qeg->kont;  ri.has_aggr_on_groupkont=FALSE;
    get_reference_info(*inherited_conds.acc0(i), &ri);
    if (!ri.has_aggr_on_groupkont)
      opopt->inherit_expr(cdp, *inherited_conds.acc0(i));
  }
#endif
 // inheriting WHERE conditions:
  if (qeg->where_expr!=NULL)
    opopt->inherit_expr(cdp, qeg->where_expr);
 // optimizing the lower level:
  opopt->optimize(cdp, FALSE);
}

/////////////////////////////// dumping //////////////////////////////////////

void dump_oper_unary(t_flstr * dmp, int oper)
{ switch (oper)
  { case UNOP_INEG:      dmp->put(" IntNeg ");    break;
    case UNOP_MNEG:      dmp->put(" Int64Neg ");  break;
    case UNOP_FNEG:      dmp->put(" RealNeg ");   break;
    case UNOP_EXISTS:    dmp->put(" Exists ");    break;
    case UNOP_UNIQUE:    dmp->put(" Unique ");    break;
//    case UNOP_IS_TRUE:   dmp->put(" IsTrue ");    break;
//    case UNOP_IS_FALSE:  dmp->put(" IsFalse ");   break;
//    case UNOP_IS_UNKNOWN:dmp->put(" ISUnknown "); break;
  }
}

void dump_oper(t_flstr * dmp, t_compoper oper)
{ if (oper.negate) dmp->put(" Not ");
  switch (oper.pure_oper)
  { case PUOP_OR:      dmp->put(" Or ");   break;
    case PUOP_AND:     dmp->put(" And ");  break;
    case PUOP_IS_NULL: dmp->put(" IS ");   break; 
    case PUOP_COMMA:   dmp->put(", ");     break;  // used in the IN (...) list

    default:
    { switch (oper.arg_type)
      { case F_OPBASE:  dmp->put(" Real");  break;
        case M_OPBASE:  dmp->put(" Int64"); break;
        case I_OPBASE:  dmp->put(" Int");   break;
        case U_OPBASE:  dmp->put(" Uns");   break;
        case S_OPBASE:  dmp->put(" Str");   break;
        case BI_OPBASE: dmp->put(" Bin");   break;
        case DT_OPBASE: dmp->put(" Date");  break;
        case TM_OPBASE: dmp->put(" Time");  break;
        case TS_OPBASE: dmp->put(" TS");    break;
      }
      switch (oper.pure_oper)
      { case PUOP_PLUS:     dmp->put("+");  break;
        case PUOP_MINUS:    dmp->put("-");  break;
        case PUOP_TIMES:    dmp->put("*");  break;
        case PUOP_DIV:      dmp->put("/");  break;
        case PUOP_MOD:      dmp->put(" Mod ");  break;
        case PUOP_EQ:       dmp->put("=");  break;
        case PUOP_LT:       dmp->put("<");  break;
        case PUOP_GT:       dmp->put(">");  break;
        case PUOP_NE:       dmp->put("!=");  break;
        case PUOP_LE:       dmp->put("<=");  break;
        case PUOP_GE:       dmp->put(">=");  break;
        case PUOP_LIKE:     dmp->put("~");  break;
        case PUOP_PREF:     dmp->put(" Pref ");  break;
        case PUOP_SUBSTR:   dmp->put(" Substr ");  break;
        case PUOP_MULTIN:   dmp->put(" MultiIn ");  break;
        case PUOP_BETWEEN:  dmp->put(" Between ");  break;
        default:            dmp->put(" Undef oper ");  break;
      }
      break;
    }
  }
}

void dump_value(cdp_t cdp, t_flstr * dmp, int valtype, t_specif specif, char * value)
{ char buf[45];
  display_value(cdp, valtype, specif, -1, value, buf);
  dmp->put(buf);
}

/*const char * function_name[] = {
"POSITION", "CHAR_LENGTH", "BIT_LENGTH", "EXTRACT", "", "", "", "", "", "SUBSTRING",  // 0
"LOWER", "UPPER", "", "", "FULLTEXT", "HAS_SELECT_PRIVIL", "HAS_UPDATE_PRIVIL", "HAS_DELETE_PRIVIL", "HAS_INSERT_PRIVIL", "HAS_GRANT_PRIVIL",  // 10
"ADMIN_MODE", "FULLTEXT_CONTEXT", "OCTET_LENGTH", "VIOLATED_CONSTRAIN", "FULLTEXT_CONTEXT2", "", "FULLTEXT_GET_CONTEXT", "", "", "",  // 20
"", "", "", "", "", "", "", "", "", "",  // 30
"", "", "", "", "", "", "", "", "", "",  // 40
"", "", "", "", "", "", "", "", "", "",  // 50
"", "", "", "", "", "", "", "", "", "",  // 60
"", "", "CREATE_SEMAPHORE", "CLOSE_SEMAPHORE", "RELEASE_SEMAPHORE", "WAIT_FOR_SEMAPHORE", "SLEEP", "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP",  // 70
"MAKE_DATE", "DAY", "MONTH", "YEAR", "TODAY", "MAKE_TIME", "HOURS", "MINUTES", "SECONDS", "SEC1000",  // 80
"NOW", "ORD", "CHR", "ODD", "ABS", "IABS", "SQRT", "ROUND", "TRUNC", "SQR",  // 90
"ISQR", "SIN", "COS", "ATAN", "LN", "EXP", "LIKE", "PREF", "SUBSTR", "STRCAT",  // 100
"UPCASE", "MEMCPY", "STR2INT", "STR2MONEY", "STR2REAL", "INT2STR", "MONEY2STR", "REAL2STR", "", "",  // 110
"", "", "", "", "STRINSERT", "STRDELETE", "STRCOPY", "STRPOS", "STRLENGTH", "STR2DATE",  // 120
"STR2TIME", "DATE2STR", "TIME2STR", "DAY_OF_WEEK", "STRTRIM", "", "", "", "", "",  // 130
"", "", "", "", "", "", "", "", "", "",  // 140
"", "", "", "", "", "", "", "", "", "",  // 150
"", "", "", "", "", "", "", "", "", "",  // 160
"", "", "", "", "", "", "", "", "", "",  // 170
};*/

void t_express::dump_expr(cdp_t cdp, t_flstr * dmp, bool eval)
{ char buf[50];
  switch (sqe)
  { case SQE_SCONST:
      dump_value(cdp, dmp, ((t_expr_sconst*)this)->origtype, ((t_expr_sconst*)this)->specif, (char*)((t_expr_sconst*)this)->val);
      break;
    case SQE_CURRREC:
      dmp->put("@");  break;
    case SQE_SEQ:
    { t_expr_seq * seqex = (t_expr_seq*)this;
      dmp->put(seqex->nextval==2 ? " NEXT VALUE FOR sequence" : seqex->nextval==1 ? " sequence.NEXTVAL" : " sequence.CURVAL");  
      break;
    }
    case SQE_LCONST:
    { char buf[45];
      int valsize = ((t_expr_lconst*)this)->valsize;
      display_value(cdp, ((t_expr_lconst*)this)->type, ((t_expr_lconst*)this)->specif, valsize, ((t_expr_lconst*)this)->lval, buf);
      dmp->put(buf);
      break;
    }
    case SQE_TERNARY:
      dump_oper(dmp, ((t_expr_ternary*)this)->oper);
      dmp->put("(");
      ((t_expr_ternary*)this)->op1->dump_expr(cdp, dmp, eval);
      dmp->put(",");
      ((t_expr_ternary*)this)->op2->dump_expr(cdp, dmp, eval);
      if (((t_expr_ternary*)this)->op3!=NULL)
      { dmp->put(",");
        ((t_expr_ternary*)this)->op3->dump_expr(cdp, dmp, eval);
      }
      dmp->put(")");
      break;
    case SQE_BINARY:
      dmp->put("(");
      ((t_expr_binary*)this)->op1->dump_expr(cdp, dmp, eval);
      if (((t_expr_binary*)this)->op2!=NULL)   // may be NULL for IN (list)
      { dump_oper(dmp, ((t_expr_binary*)this)->oper);
        ((t_expr_binary*)this)->op2->dump_expr(cdp, dmp, eval);
      }
      dmp->put(")");
      break;
    case SQE_UNARY:
      dump_oper_unary(dmp, ((t_expr_unary*)this)->oper);
      dmp->put("(");
      ((t_expr_unary*)this)->op->dump_expr(cdp, dmp, eval);
      dmp->put(")");
      break;
    case SQE_AGGR:
      dmp->put(" Aggr");  break;
    case SQE_FUNCT:
    { t_expr_funct * fex = (t_expr_funct*)this;
      sprintf(buf, " Funct %s(", get_function_name(fex->num));
      dmp->put(buf);
      if (fex->arg1!=NULL) fex->arg1->dump_expr(cdp, dmp, eval);
      if (fex->arg2!=NULL) fex->arg2->dump_expr(cdp, dmp, eval);
      if (fex->arg3!=NULL) fex->arg3->dump_expr(cdp, dmp, eval);
      if (fex->arg4!=NULL) fex->arg4->dump_expr(cdp, dmp, eval);
      if (fex->arg5!=NULL) fex->arg5->dump_expr(cdp, dmp, eval);
      if (fex->arg6!=NULL) fex->arg6->dump_expr(cdp, dmp, eval);
      if (fex->arg7!=NULL) fex->arg7->dump_expr(cdp, dmp, eval);
      if (fex->arg8!=NULL) fex->arg8->dump_expr(cdp, dmp, eval);
      dmp->put(")");
      break;
    }
    case SQE_ATTR:  case SQE_ATTR_REF:
    { t_expr_attr * atex = (t_expr_attr*)this;
      elem_descr * el = atex->kont->elems.acc0(atex->elemnum);
      if (convert.conv==CONV_NEGATE) dmp->put("NOT ");
      sprintf(buf, "%s.%s", elem_prefix(atex->kont, el), el->name);
      dmp->put(buf);  
      if (atex->indexexpr!=NULL)
      { dmp->put("[");
        atex->indexexpr->dump_expr(cdp, dmp, eval);
        dmp->put("]");
      }
      break;
    }
    case SQE_POINTED_ATTR:
    { t_expr_pointed_attr * poatr = (t_expr_pointed_attr*)this;
      poatr->pointer_attr->dump_expr(cdp, dmp, eval);
      dmp->put("^");  dmp->put(poatr->name);
      break;
    }
    case SQE_DYNPAR:
      dmp->put(" Dynpar");  break;
    case SQE_NULL:
      dmp->put(" NULL");  break;
    case SQE_CASE:
      dmp->put(" Case");  break;
    case SQE_SUBQ:
    { t_expr_subquery * suex = (t_expr_subquery*)this;
      dmp->put(" Subquery start\x1\x2");  
      suex->opt->dump(cdp, dmp, eval);
      if (eval) 
      { dmp->put(" Evaluated ");  dmp->putint(suex->evaluated_count);  dmp->put(" times and used ");  
        dmp->putint(suex->used_count);  dmp->put(" times\x1"); 
      } 
      dmp->put("\x3Subquery end ");  
      break;
    }
    case SQE_MULT_COUNT:
      ((t_expr_mult_count*)this)->attrex->dump_expr(cdp, dmp, eval);
      dmp->put("#");  break;
    case SQE_VAR_LEN:
      ((t_expr_var_len   *)this)->attrex->dump_expr(cdp, dmp, eval);
      dmp->put("#");  break;
    case SQE_DBLIND:
      ((t_expr_dblind    *)this)->attrex->dump_expr(cdp, dmp, eval);
      dmp->put("[");
      ((t_expr_dblind    *)this)->start ->dump_expr(cdp, dmp, eval);
      dmp->put(",");
      ((t_expr_dblind    *)this)->size  ->dump_expr(cdp, dmp, eval);
      dmp->put("]");  break;
    case SQE_CAST:
      dmp->put("CAST ");
      ((t_expr_cast      *)this)->arg   ->dump_expr(cdp, dmp, eval);
      break;
    case SQE_ROUTINE:
    { tobjname name;
      strcpy(name, "LocalFunction");  // default
      sql_stmt_call * stmt = (sql_stmt_call*)this;
      if (!stmt->calling_local_routine())
      { t_routine * routine = stmt->get_called_routine(cdp, NULL, 2);
        if (routine)
        { strcpy(name, routine->name);
          stmt->release_called_routine(routine);  // return the routine to the cache
        }
      }
      dmp->putc(' ');  dmp->put(name);  dmp->put("()");
      break;
    }  
  }
}

void dump_limit(cdp_t cdp, t_flstr * dmp, t_query_expr * qe, bool eval);

void t_optim_join_inner::dump(cdp_t cdp, t_flstr * dmp, bool eval)
{ if (variants.count()>2) dmp->put("United alternatives of the query evaluation\x1\x2");
  dump_limit(cdp, dmp, own_qe, eval);
  for (int i=1;  i<variants.count();  i++)
  { t_variant * variant = variants.acc0(i);
    dmp->put("\x4INNER JOIN of\x1\x2");
    for (int j=0;  j<leave_count;  j++)
      variant->join_steps[j]->dump(cdp, dmp, eval);
    dmp->put("\x3");
  }
  if (variants.count()>2) dmp->put("\x3");
}

void t_optim_join_outer::dump(cdp_t cdp, t_flstr * dmp, bool eval)
{ dmp->put("\x4");
  dmp->put(left_outer ? "LEFT": right_outer ? "RIGHT" : "FULL");
  dmp->put(" OUTER JOIN of\x1\x2");
  dump_limit(cdp, dmp, own_qe, eval);
  variant.join_steps[0]->dump(cdp, dmp, eval);
  variant.join_steps[1]->dump(cdp, dmp, eval);
  bool has_conds=false;
  for (int i=0;  i<inherited_conds.count();  i++)
    if (conds_outer & (1<<i)) 
      { has_conds=true;  break; }
  if (has_conds)
  { dmp->put("Post join conditions\x1\x2");
    for (int i=0;  i<inherited_conds.count();  i++)
      if (conds_outer & (1<<i))
      { (*inherited_conds.acc0(i))->dump_expr(cdp, dmp, eval);
        dmp->put("\x1");
      }
    dmp->put("\x3");
  }
  dmp->put("\x3");
}

void t_optim_grouping::dump(cdp_t cdp, t_flstr * dmp, bool eval)
{ dmp->put("\x4GROUPING\x1\x2");
  dump_limit(cdp, dmp, own_qe, eval);
  opopt->dump(cdp, dmp, eval);
  if (eval) { dmp->put("Created ");  dmp->putint(group_count);  dmp->put(" group(s)\x1"); }
  if (qeg->having_expr!=NULL)
  { dmp->put("HAVING ");
    qeg->having_expr->dump_expr(cdp, dmp, eval);
    if (eval) { dmp->put(" Satisfied by ");  dmp->putint(having_result);  dmp->put(" records"); }
    dmp->put("\x1");
  }
#if SQL3
  if (inherited_having_conds.count())
  { dmp->put("Inherited HAVING conditions ");
    for (int i=0;  i<inherited_having_conds.count();  i++)
    { (*inherited_having_conds.acc0(i))->dump_expr(cdp, dmp, eval);
      dmp->put("\x1");
    }
  }
#endif
  if (inherited_conds.count())
  { dmp->put("Inherited conditions ");
    for (int i=0;  i<inherited_conds.count();  i++)
    { (*inherited_conds.acc0(i))->dump_expr(cdp, dmp, eval);
      dmp->put("\x1");
    }
    if (eval) { dmp->put(" Output ");  dmp->putint(output_records);  dmp->put(" records ");  dmp->put("\x1"); }
  }
  dmp->put("\x3");
}

void t_optim_union_dist::dump(cdp_t cdp, t_flstr * dmp, bool eval)
{ dmp->put("\x4UNION DISTINCT\x1\x2");
  op1opt->dump(cdp, dmp, eval);
  op2opt->dump(cdp, dmp, eval);
  dmp->put("\x3");
}

#if SQL3
void t_optim_order::dump(cdp_t cdp, t_flstr * dmp, bool eval)
{ dmp->put(sort_disabled ? "\x4NO SORTING (embedded ORDER BY)\x1\x2" : "\x4SORTING according to ORDER BY\x1\x2");
  dump_limit(cdp, dmp, own_qe, eval);
  subopt->dump(cdp, dmp, eval);
  dmp->put("\x3");
}

void dump_limit(cdp_t cdp, t_flstr * dmp, t_query_expr * qe, bool eval)
{
  if (qe->limit_offset!=NULL)
    { dmp->put(" Offset ");  qe->limit_offset->dump_expr(cdp, dmp, eval); }
  if (qe->limit_count!=NULL)
    { dmp->put(" Limit ");  qe->limit_count->dump_expr(cdp, dmp, eval); }
  if (qe->limit_offset!=NULL || qe->limit_count!=NULL)
  { if (eval && qe->limit_result_count!=NORECNUM) 
      { dmp->put("  Resulting records ");  dmp->putint(qe->limit_result_count); }
    dmp->put("\x1");
  }
}

void t_optim_limit::dump(cdp_t cdp, t_flstr * dmp, bool eval)
{ //dmp->put("\x4LIMIT applied\x1\x2");
  dmp->put("\x4LIMITING THE RESULT\x1\x2");
  dump_limit(cdp, dmp, own_qe, eval);
  subopt->dump(cdp, dmp, eval);
  dmp->put("\x3");
}
#endif

void t_optim_union_all::dump(cdp_t cdp, t_flstr * dmp, bool eval)
{ dmp->put("\x4UNION ALL\x1\x2");
  dump_limit(cdp, dmp, own_qe, eval);
  op1opt->dump(cdp, dmp, eval);
  op2opt->dump(cdp, dmp, eval);
  dmp->put("\x3");
}

void t_optim_exc_int::dump(cdp_t cdp, t_flstr * dmp, bool eval)
{ dmp->put("\x4");
  dmp->put(qe_type==QE_EXCEPT ? "EXCEPT OF " : "INTERSECT OF ");
  dmp->put(rowdist.is_unique() ? "DISTINCT\x1\x2" : "ALL\x1\x2");
  dump_limit(cdp, dmp, own_qe, eval);
  op1opt->dump(cdp, dmp, eval);
  op2opt->dump(cdp, dmp, eval);
  dmp->put("\x3");
}

void t_optim_table::dump(cdp_t cdp, t_flstr * dmp, bool eval)
{ dmp->put("\x5Passing table ");  dmp->put(qet->kont.prefix_name);  
  if (docid_index!=-1)
    { dmp->put(" accessed by the fulltext index # ");  dmp->putint(docid_index); }
  else if (!normal_index_count) dmp->put(" (exhaustive)");
  if (eval) { dmp->put(" traversed ");  dmp->putint(reset_count);  dmp->put(" times"); }
  dmp->put("\x1");
  if (normal_index_count || eval || post_conds.count() /*|| own_qe->limit_count || own_qe->limit_offset*/) 
  { dmp->put("\x2");
    //dump_limit(cdp, dmp, own_qe, eval);

    for (int i=0;  i<normal_index_count;  i++)
    { table_descr_auto td(cdp, qet->tabnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
      dmp->put("Accessed by ");
      if (!td->me()) dmp->put("index");
      else 
      { if (*td->indxs[tics[i].index_number].constr_name) dmp->put(td->indxs[tics[i].index_number].constr_name);
        else { dmp->put("index #");  dmp->putint(tics[i].index_number); }
        dmp->put(" limited by ");
        bool any = false;
        for (int k=0;  k<td->indxs[tics[i].index_number].partnum;  k++)
        { if (k>0) dmp->put(", ");
          t_table_index_part_conds * tipcs = &tics[i].part_conds[k];
          for (int e=0;  e<tipcs->pconds.count();  e++)
          { if (e>0) dmp->put(" AND ");
            (*tipcs->pconds.acc0(e))->dump_expr(cdp, dmp, eval);
            any=true;
          }
        }
        if (!any) dmp->put("nothing.");
      }
      dmp->put("\x1");
    }

    if (eval) { dmp->put(" Total records: ");  dmp->putint(records_taken);  dmp->put("\x1"); }

    if (post_conds.count())
    { dmp->put("Satisfying conditions");
      if (eval) {  dmp->put(" (passed OK: ");  dmp->putint(post_passed);  dmp->put(" records)"); }
      dmp->put("\x1\x2");
      for (int k=0;  k<post_conds.count();  k++)
      { t_express * ex = *post_conds.acc0(k);
        ex->dump_expr(cdp, dmp, eval);
        dmp->put("\x1");
      }
      dmp->put("\x3");
    }
    if (eval && main_rowdist!=NULL)
      { dmp->put(" Distinct records: ");  dmp->putint(main_rowdist->found_distinct_records);  dmp->put("\x1"); }
    dmp->put("\x3");
  }
}

void t_optim_sysview::dump(cdp_t cdp, t_flstr * dmp, bool eval)
{ dmp->put("(SYSVIEW)\x1"); 
}

//////////////////////////////////////////////////////////////////////////////

BOOL Search_in_blockA(const char * block, uns32 blocksize,
                      const char * next, const char * patt, int pattsize,
                      int flags, wbcharset_t charset, uns32 * respos)
{ unsigned pos, matched;  char c;
  if (flags & SEARCH_BACKWARDS)  /* "next" not used if searching backwards */
  { if (pattsize>blocksize) return FALSE;
	 pos=blocksize-pattsize;
    do
    { matched=0;
      do
      { if (!(flags & SEARCH_CASE_SENSITIVE)) 
          c=upcase_charA(block[pos], charset);
        else c=block[pos];
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
          { if (flags & SEARCH_WHOLE_WORDS)
            { if (pos>matched)   if (sys_Alphanum(block[pos-matched-1])) break;
              if (pos<blocksize) if (sys_Alphanum(block[pos])) break;
            }
            *respos=pos-matched;  return TRUE;
          }
        }
        else break;
      } while (TRUE);
      pos=pos-matched;
    } while (pos--);
  }
  else
  { pos=0;
    while (pos < blocksize)
    { matched=0;
      do
      { if (pos<blocksize) c=block[pos];
        else
          if (next) c=next[pos-blocksize];
          else break;
        if (!(flags & SEARCH_CASE_SENSITIVE)) 
          c=upcase_charA(c, charset);
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
          { if (flags & SEARCH_WHOLE_WORDS)
            { if (pos>matched)
                if (sys_Alphanum(block[pos-matched-1])) break;
                else if (flags & SEARCH_NOCONT) break;
              if (pos<blocksize)
                if (sys_Alphanum(block[pos])) break;
                else if (next) if (sys_Alphanum(block[pos-blocksize])) break;
            }
            *respos=pos-matched;  return TRUE;
          }
        }
        else break;
      } while (TRUE);
      pos=pos-matched+1;
    }
  }
  return FALSE;
}

BOOL Search_in_blockW(const wuchar * block, uns32 blocksize,
                      const wuchar * next, const wuchar * patt, int pattsize,
                      int flags, wbcharset_t charset, uns32 * respos)
// Flag SEARCH_WHOLE_WORDS ignored for unicode. [pattsize] is in characters.
{ unsigned pos, matched;  wuchar c;
  if (flags & SEARCH_BACKWARDS)  /* "next" not used if searching backwards */
  { if (pattsize>blocksize) return FALSE;
	 pos=blocksize-pattsize;
    do
    { matched=0;
      do
      { if (!(flags & SEARCH_CASE_SENSITIVE)) 
          c=upcase_charW(block[pos], charset);
        else c=block[pos];
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
            { *respos=pos-matched;  return TRUE; }
        }
        else break;
      } while (TRUE);
      pos=pos-matched;
    } while (pos--);
  }
  else
  { pos=0;
    while (pos < blocksize)
    { matched=0;
      do
      { if (pos<blocksize) c=block[pos];
        else
          if (next) c=next[pos-blocksize];
          else break;
        if (!(flags & SEARCH_CASE_SENSITIVE)) 
          c=upcase_charW(c, charset);
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
            { *respos=pos-matched;  return TRUE; }
        }
        else break;
      } while (TRUE);
      pos=pos-matched+1;
    }
  }
  return FALSE;
}

BOOL var_search(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attr, t_mult_size index,
       t_varcol_size start, int flags, wbcharset_t charset, int pattlen, char * pattern, t_varcol_size * pos)
// Supposes: tbdf->attrs[attr]->attrtype == ATT_TEXT
//          (tbdf->attrs[attr]->attrmult==1) == (index==NOINDEX)
// [pattlen] is in characters. [pattern] must be in the same charset as searched text. SEARCH_WIDE_CHAR in [flags] is valid for both.
{ tfcbnum fcbn;  const heapacc * tmphp;
  *pos=(uns32)-1;
  if (tbdf->attrs[attr].attrmult == 1)
    tmphp=(const heapacc*)fix_attr_read(cdp, tbdf, recnum, attr, &fcbn);
  else
    tmphp=(const heapacc*)fix_attr_ind_read(cdp, tbdf, recnum, attr, index, &fcbn);
  if (!tmphp) return TRUE;
  if (!(flags & SEARCH_CASE_SENSITIVE))
    for (int i=0;  i<pattlen;  i++) 
      if (flags & SEARCH_WIDE_CHAR) ((wuchar*)pattern)[i]= upcase_charW(((wuchar*)pattern)[i], charset);
      else if (charset.is_wb8())               pattern[i] = upcase_charA(pattern[i], charset);
      else                                     pattern[i] = mcUPCASE((uns8)pattern[i]);
  BOOL res=hp_search(cdp, &tmphp->pc, start, tmphp->len, flags, charset, pattlen, pattern, pos);
  UNFIX_PAGE(cdp, fcbn);
  return res;
}

///////////////////////////// scalar evaluation //////////////////////////////

void binary_oper(cdp_t cdp, t_value * val1, t_value * val2, t_compoper oper)
// Performs binary operation oper on val1 and val2, writes result to val1.
{ BOOL bres;  // boolean result
  if (oper.pure_oper>=PUOP_AND)  // may calculate with NULL value
  { switch (oper.pure_oper)
    { case PUOP_OR:  // any is TRUE -> result is TRUE
        if (val1->is_true() || val2->is_true())
          { val1->set_simple_not_null();  val1->intval=TRUE; }
        else if (val1->is_null() || val2->is_null())
            val1->set_null();
        else  // both are FALSE -> FALSE
            val1->intval=FALSE; // val1 is not null here!
        break;
      case PUOP_AND:  // any is FALSE -> result is FALSE
        if (val1->is_false() || val2->is_false())
          { val1->set_simple_not_null();  val1->intval=FALSE; }
        else if (val1->is_null() || val2->is_null())
            val1->set_null();
        else // both are TRUE -> TRUE
            val1->intval=TRUE;  // val1 is not null here!
        break;
      case PUOP_IS_NULL: // depends on [negate]
      { BOOL is_null_val;
        if (val1->vmode==mode_access)
        { if (val1->dbacc.index!=NOINDEX)
            tb_read_ind_len(cdp, tables[val1->dbacc.tb], val1->dbacc.rec, val1->dbacc.attr, val1->dbacc.index, (uns32*)&val1->length);
          is_null_val = val1->length==0;
        }
        else is_null_val = val1->is_null();
        val1->set_simple_not_null();
        val1->intval=oper.negate ? !is_null_val : is_null_val;  break;
      }
    }
  }
  else
  { if (oper.pure_oper!=PUOP_MULTIN && val1->load(cdp)) return;
    if (val2->load(cdp)) return;  // works only for mode_access
    if (val1->is_null())
    { if (oper.arg_type==S_OPBASE)
        { val1->length=0;  val1->strval[0]=0;  val1->set_simple_not_null(); } // makes the string comparable and concatenable
      else
        if (val2->is_null())
        { if (cdp->sqloptions & SQLOPT_NULLEQNULL)  // WB compatible way
          { if (oper.pure_oper==PUOP_EQ || oper.pure_oper==PUOP_LE || oper.pure_oper==PUOP_GE)
              { val1->intval=1;  val1->set_simple_not_null();  return; }  
            if (oper.pure_oper==PUOP_NE || oper.pure_oper==PUOP_LT || oper.pure_oper==PUOP_GT)
              { val1->intval=0;  val1->set_simple_not_null();  return; }  
          }
          val1->set_null();  // SQL way
          return;
        }
        else // NULL to NOT NULL
        { if (cdp->sqloptions & SQLOPT_NULLCOMP)   // WB compatible way
          { if (oper.pure_oper==PUOP_EQ || oper.pure_oper==PUOP_GT || oper.pure_oper==PUOP_GE)
              { val1->intval=0;  val1->set_simple_not_null();  return; }  
            if (oper.pure_oper==PUOP_NE || oper.pure_oper==PUOP_LT || oper.pure_oper==PUOP_LE)
              { val1->intval=1;  val1->set_simple_not_null();  return; }  
            if (oper.pure_oper==PUOP_DIV|| oper.pure_oper==PUOP_TIMES)
              return; // NULL result
           // return the non-null operand:
            *val1=*val2;
          }
          // SQL way: returning NULL/UNKNOWN
          return;
        }
    }
    if (val2->is_null())  // NOT NULL to NULL
    { if (oper.arg_type==S_OPBASE)
        { val2->length=0;  val2->strval[0]=0;  val2->set_simple_not_null(); } // makes the string comparable and concatenable
      else
        if (cdp->sqloptions & SQLOPT_NULLCOMP)   // WB compatible way
        { if (oper.pure_oper==PUOP_EQ || oper.pure_oper==PUOP_LT || oper.pure_oper==PUOP_LE)
            { val1->intval=0;  val1->set_simple_not_null();  return; }  
          if (oper.pure_oper==PUOP_NE || oper.pure_oper==PUOP_GT || oper.pure_oper==PUOP_GE)
            { val1->intval=1;  val1->set_simple_not_null();  return; }  
          if (oper.pure_oper==PUOP_DIV || oper.pure_oper==PUOP_TIMES)
            val1->set_null();  // NULL result
          // returning val1 operand
          return;
        }
        else // SQL way: return NULL/UNKNOWN
        { val1->set_null(); 
          return;
        }
    }
   // non-null operands:
    switch (oper.merged())
    { case MRG(PUOP_PLUS, I_OPBASE): val1->intval+=val2->intval;  break;
      case MRG(PUOP_MINUS,I_OPBASE): val1->intval-=val2->intval;  break;
      case MRG(PUOP_TIMES,I_OPBASE): val1->intval*=val2->intval;  break;
      case MRG(PUOP_DIV,  I_OPBASE):
        if (!val2->intval) { val1->set_null();  sql_exception(cdp, SQ_DIVISION_BY_ZERO); }  // may be handled
        else val1->intval/=val2->intval;  break;
      case MRG(PUOP_MOD,  I_OPBASE):
        if (!val2->intval) { val1->set_null();  sql_exception(cdp, SQ_DIVISION_BY_ZERO); }  // may be handled
        else val1->intval%=val2->intval;  break;
      case MRG(PUOP_EQ,   I_OPBASE): val1->intval=val1->intval==val2->intval;  break;
      case MRG(PUOP_NE,   I_OPBASE): val1->intval=val1->intval!=val2->intval;  break;
      case MRG(PUOP_GT,   I_OPBASE): val1->intval=val1->intval> val2->intval;  break;
      case MRG(PUOP_GE,   I_OPBASE): val1->intval=val1->intval>=val2->intval;  break;
      case MRG(PUOP_LT,   I_OPBASE): val1->intval=val1->intval< val2->intval;  break;
      case MRG(PUOP_LE,   I_OPBASE): val1->intval=val1->intval<=val2->intval;  break;

      case MRG(PUOP_GT,   U_OPBASE): val1->unsval=val1->unsval> val2->unsval;  break;
      case MRG(PUOP_GE,   U_OPBASE): val1->unsval=val1->unsval>=val2->unsval;  break;
      case MRG(PUOP_LT,   U_OPBASE): val1->unsval=val1->unsval< val2->unsval;  break;
      case MRG(PUOP_LE,   U_OPBASE): val1->unsval=val1->unsval<=val2->unsval;  break;

      case MRG(PUOP_PLUS, F_OPBASE): val1->realval+=val2->realval;  break;
      case MRG(PUOP_MINUS,F_OPBASE): val1->realval-=val2->realval;  break;
      case MRG(PUOP_TIMES,F_OPBASE): val1->realval*=val2->realval;  break;
      case MRG(PUOP_DIV,  F_OPBASE):
        if (!val2->realval) { val1->set_null();  sql_exception(cdp, SQ_DIVISION_BY_ZERO); }  // may be handled
        else val1->realval/=val2->realval;  break;
      case MRG(PUOP_EQ,   F_OPBASE): //val1->intval=val1->realval==val2->realval;  break;
      case MRG(PUOP_NE,   F_OPBASE): //val1->intval=val1->realval!=val2->realval;  break;
      { double diff = val1->realval-val2->realval;
        BOOL eq;
        if (fabs(diff) < 1e100) 
        { diff = fabs(diff) * 1e13;
          eq = diff <= fabs(val1->realval) && diff <= fabs(val2->realval);
        } else eq=FALSE;
        val1->intval = oper.pure_oper==PUOP_EQ ? eq : !eq;  break;
      }
      case MRG(PUOP_GT,   F_OPBASE): val1->intval=val1->realval> val2->realval;  break;
      case MRG(PUOP_GE,   F_OPBASE): val1->intval=val1->realval>=val2->realval;  break;
      case MRG(PUOP_LT,   F_OPBASE): val1->intval=val1->realval< val2->realval;  break;
      case MRG(PUOP_LE,   F_OPBASE): val1->intval=val1->realval<=val2->realval;  break;

      case MRG(PUOP_PLUS, M_OPBASE): val1->i64val+=val2->i64val;  break;
      case MRG(PUOP_MINUS,M_OPBASE): val1->i64val-=val2->i64val;  break;
      case MRG(PUOP_TIMES,M_OPBASE): val1->i64val*=val2->i64val;  break; // ### roll, add division
      case MRG(PUOP_DIV,  M_OPBASE):
        if (!val2->i64val) { val1->set_null();  sql_exception(cdp, SQ_DIVISION_BY_ZERO); }  // may be handled
        else val1->i64val/=val2->i64val;  break;
      case MRG(PUOP_MOD,  M_OPBASE):
        if (!val2->i64val) { val1->set_null();  sql_exception(cdp, SQ_DIVISION_BY_ZERO); }  // may be handled
        else val1->i64val%=val2->i64val;  break;

      case MRG(PUOP_EQ,   M_OPBASE): val1->intval=val1->i64val==val2->i64val;  break;
      case MRG(PUOP_NE,   M_OPBASE): val1->intval=val1->i64val!=val2->i64val;  break;
      case MRG(PUOP_GT,   M_OPBASE): val1->intval=val1->i64val> val2->i64val;  break;
      case MRG(PUOP_GE,   M_OPBASE): val1->intval=val1->i64val>=val2->i64val;  break;
      case MRG(PUOP_LT,   M_OPBASE): val1->intval=val1->i64val< val2->i64val;  break;
      case MRG(PUOP_LE,   M_OPBASE): val1->intval=val1->i64val<=val2->i64val;  break;

      case MRG(PUOP_EQ,   S_OPBASE): bres=cmp_str(val1->valptr(), val2->valptr(), t_specif(oper))==0;  goto bresult;
      case MRG(PUOP_NE,   S_OPBASE): bres=cmp_str(val1->valptr(), val2->valptr(), t_specif(oper))!=0;  goto bresult;
      case MRG(PUOP_GT,   S_OPBASE): bres=cmp_str(val1->valptr(), val2->valptr(), t_specif(oper))> 0;  goto bresult;
      case MRG(PUOP_GE,   S_OPBASE): bres=cmp_str(val1->valptr(), val2->valptr(), t_specif(oper))>=0;  goto bresult;
      case MRG(PUOP_LT,   S_OPBASE): bres=cmp_str(val1->valptr(), val2->valptr(), t_specif(oper))< 0;  goto bresult;
      case MRG(PUOP_LE,   S_OPBASE): bres=cmp_str(val1->valptr(), val2->valptr(), t_specif(oper))<=0;  goto bresult;
      case MRG(PUOP_PLUS, S_OPBASE): // concatenation for char strings, depends on [wide_char]
        if (oper.wide_char)
        { val1->length=2*(int)wuclen(val1->wchptr()); // must remove 0s from the string!
          val2->length=2*(int)wuclen(val2->wchptr());
          if (val1->reallocate(val1->length+val2->length))
            { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
          memcpy(val1->valptr()+val1->length, val2->valptr(), val2->length+2);
        }
        else // narrow char
        { val1->length=(int)strlen(val1->valptr()); // must remove 0s from the string!
          val2->length=(int)strlen(val2->valptr());
          if (val1->reallocate(val1->length+val2->length))
            { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
          memcpy(val1->valptr()+val1->length, val2->valptr(), val2->length+1);  
        }
        val1->length+=val2->length;
        break;
      case MRG(PUOP_PREF,  S_OPBASE): // depends on [negate], [arg_type], [ignore_case], [wide_char]
        bres=general_Pref(val2->valptr(), val1->valptr(), t_specif(oper));  goto bnresult;
      case MRG(PUOP_SUBSTR,S_OPBASE): // depends on [negate], [arg_type], [ignore_case], [wide_char]
        bres=general_Substr(val2->valptr(), val1->valptr(), t_specif(oper))!=0;  goto bnresult;
      case MRG(PUOP_LIKE,  S_OPBASE): // depends on [negate], [arg_type], [ignore_case], [wide_char]
        if (oper.wide_char)
          bres=wlike_esc(val1->wchptr(), val2->wchptr(), 0, oper.ignore_case,oper.collation);  
        else
          bres=like_esc (val1->valptr(), val2->valptr(), 0, oper.ignore_case,oper.collation);  
       bnresult:
        if (oper.negate) bres=!bres;
       bresult: 
        val1->set_simple_not_null();  val1->intval=bres;  break;

      case MRG(PUOP_MULTIN,I_OPBASE): // for all types, depends on [negate]
        bres=!multi_in(cdp, tables[val1->dbacc.tb], val1->dbacc.rec, val1->dbacc.attr, val2->valptr());  
        goto bnresult;

      case MRG(PUOP_EQ,  BI_OPBASE): bres=compare_values(ATT_BINARY, t_specif(val1->length), val1->valptr(), val2->valptr())==0;  goto bresult;
      case MRG(PUOP_NE,  BI_OPBASE): bres=compare_values(ATT_BINARY, t_specif(val1->length), val1->valptr(), val2->valptr())!=0;  goto bresult;
      case MRG(PUOP_GT,  BI_OPBASE): bres=compare_values(ATT_BINARY, t_specif(val1->length), val1->valptr(), val2->valptr())> 0;  goto bresult;
      case MRG(PUOP_GE,  BI_OPBASE): bres=compare_values(ATT_BINARY, t_specif(val1->length), val1->valptr(), val2->valptr())>=0;  goto bresult;
      case MRG(PUOP_LT,  BI_OPBASE): bres=compare_values(ATT_BINARY, t_specif(val1->length), val1->valptr(), val2->valptr())< 0;  goto bresult;
      case MRG(PUOP_LE,  BI_OPBASE): bres=compare_values(ATT_BINARY, t_specif(val1->length), val1->valptr(), val2->valptr())<=0;  goto bresult;
      case MRG(PUOP_PLUS,BI_OPBASE): // concatenation for binary strings
        if (val1->reallocate(val1->length+val2->length))
          { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
        memcpy(val1->valptr()+val1->length, val2->valptr(), val2->length);  
        val1->length+=val2->length;
        break;

      case MRG(PUOP_PLUS, DT_OPBASE): val1->unsval=dt_plus (val1->unsval, val2->intval);  break;
      case MRG(PUOP_MINUS,DT_OPBASE): val1->unsval=dt_minus(val1->unsval, val2->intval);  break;
      case MRG(PUOP_TIMES,DT_OPBASE): val1->unsval=dt_plus (val2->unsval, val1->intval);  break;
      case MRG(PUOP_DIV,  DT_OPBASE): val1->intval=dt_diff (val1->unsval, val2->unsval);  break;
      case MRG(PUOP_PLUS, TM_OPBASE): val1->unsval=tm_plus (val1->unsval, val2->intval);  break;
      case MRG(PUOP_MINUS,TM_OPBASE): val1->unsval=tm_minus(val1->unsval, val2->intval);  break;
      case MRG(PUOP_TIMES,TM_OPBASE): val1->unsval=tm_plus (val2->unsval, val1->intval);  break;
      case MRG(PUOP_DIV,  TM_OPBASE): val1->intval=tm_diff (val1->unsval, val2->unsval);  break;
      case MRG(PUOP_PLUS, TS_OPBASE): val1->unsval=ts_plus (val1->unsval, val2->intval);  break;
      case MRG(PUOP_MINUS,TS_OPBASE): val1->unsval=ts_minus(val1->unsval, val2->intval);  break;
      case MRG(PUOP_TIMES,TS_OPBASE): val1->unsval=ts_plus (val2->unsval, val1->intval);  break;
      case MRG(PUOP_DIV,  TS_OPBASE): val1->intval=ts_diff (val1->unsval, val2->unsval);  break;
    } // switch (merged())
  } // no operand is NULL 
}

////////////////////////// function call ////////////////////////////////////
static int CallMailFunction(cdp_t cdp, t_expr_funct * fex, t_value & a1, t_value & a2, t_value & a3, t_value & a4, t_value & a5, t_value & a6, t_value & a7, t_value & a8)
{ int result;
#ifdef WINS
  __try
#endif
  { switch (fex->num)
    { case 157: // InitWBMail
        result=cd_InitWBMail(cdp, a1.is_null() ? NULL : a1.valptr(), a2.is_null() ? NULL : a2.valptr());  break;
  #ifdef WINS
      case 158: // InitWBMail602
        result=cd_InitWBMail602(cdp, a1.is_null() ? NULL : a1.valptr(), a2.is_null() ? NULL : a2.valptr(), a3.is_null() ? NULL : a3.valptr());  break;
  #endif
      case 159: // CloseWBMail
        cd_CloseWBMail(cdp);  break;
      case 160: // LetterCreate
      { HANDLE letter;
        a2.load(cdp);
        result=cd_LetterCreate(cdp, a1.is_null() ? NULL : a1.valptr(), a2.is_null() ? NULL : a2.valptr(), a3.intval, &letter);  
        if (fex->arg4->sqe==SQE_VAR) // other params not supported
        { t_expr_var * varex = (t_expr_var*)fex->arg4;
          if (varex->level!=SYSTEM_VAR_LEVEL) 
          { tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
            *(HANDLE*)varbl=letter;  // will not overflow because the last parameter is of HANDLE type
          }
        }
        break;
      }
      case FCNUM_LETTERCREW:
      { HANDLE letter;
        a2.load(cdp);
        result=cd_LetterCreateW(cdp, a1.is_null() ? NULL : (const wuchar *)a1.valptr(), a2.is_null() ? NULL : (const wuchar *)a2.valptr(), a3.intval, &letter);  
        if (fex->arg4->sqe==SQE_VAR) // other params not supported
        { t_expr_var * varex = (t_expr_var*)fex->arg4;
          if (varex->level!=SYSTEM_VAR_LEVEL) 
          { tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
            *(HANDLE*)varbl=letter;  // will not overflow because the last parameter is of HANDLE type
          }
        }
        break;
      }
      case 161: // LetterAddAddr
        result=cd_LetterAddAddr(cdp, (HANDLE)a1.i64val, a2.valptr(), a3.is_null() ? NULL : a3.valptr(), a4.intval);  break;
      case FCNUM_LETTERADDADRW: // LetterAddAddr
        result=cd_LetterAddAddrW(cdp, (HANDLE)a1.i64val, a2.is_null() ? NULL : (const wuchar *)a2.valptr(), a3.is_null() ? NULL : (const wuchar *)a3.valptr(), a4.intval);  break;
      case 162: // LetterAddFile
        result=cd_LetterAddFile(cdp, (HANDLE)a1.i64val, a2.valptr());  break;
      case FCNUM_LETTERADDFILEW: // LetterAddFile
        result=cd_LetterAddFileW(cdp, (HANDLE)a1.i64val, a2.is_null() ? NULL : (const wuchar *)a2.valptr());  break;
      case 163: // LetterSend
        result=cd_LetterSend(cdp, (HANDLE)a1.i64val);  break;
      case 164: // TakeMailTo RemOffice
        result=cd_TakeMailToRemOffice(cdp);  break;
      case 165: // LetterCancel
        cd_LetterCancel(cdp, (HANDLE)a1.i64val);  break;
  #ifdef WINS
      case 166: // InitWBMail602x
        result=cd_InitWBMail602x(cdp, a1.valptr());  break;
  #endif
      case 167: // MailOpenInBox
      { HANDLE mb;
        result=cd_MailOpenInBox(cdp, &mb);
        if (fex->arg1->sqe==SQE_VAR)
        { t_expr_var *varex = (t_expr_var *)fex->arg1;
          if (varex->level!=SYSTEM_VAR_LEVEL)
          { tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
            *(HANDLE*)varbl=mb;  // will not overflow because the last parameter is of HANDLE type
          }
        }
        break;
      }
      case 168: // MailBoxLoad
        result=cd_MailBoxLoad(cdp, (HANDLE)a1.i64val, a2.intval); break;
      case 169: // MailBoxGetMsg
        result=cd_MailBoxGetMsg(cdp, (HANDLE)a1.i64val, a2.intval, 0); break;
      case 170: // MailBoxGetFilInfo
        result=cd_MailBoxGetFilInfo(cdp, (HANDLE)a1.i64val, a2.intval); break;
      case 171: // MailBoxSaveFileAs
        result=cd_MailBoxSaveFileAs(cdp, (HANDLE)a1.i64val, a2.intval, a3.intval, a4.is_null() ? NULL : a4.valptr(), a5.valptr()); break;
      case 172: // MailBoxDeleteMsg
        result=cd_MailBoxDeleteMsg(cdp, (HANDLE)a1.i64val, a2.intval, a3.intval); break;
      case 173: // MailGetInBoxInfo
      { char arg2[64], arg4[64];
        ttablenum arg3, arg5;
        result=cd_MailGetInBoxInfo(cdp, (HANDLE)a1.i64val, arg2, &arg3, arg4, &arg5);
        if (result == 0)
        { if (fex->arg2->sqe==SQE_VAR)
          { t_expr_var *varex = (t_expr_var *)fex->arg2;
            if (varex->level!=SYSTEM_VAR_LEVEL)
            { tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
              strcpy(varbl, arg2);
            }
          }
          if (fex->arg3->sqe==SQE_VAR)
          { t_expr_var *varex = (t_expr_var *)fex->arg3;
            if (varex->level!=SYSTEM_VAR_LEVEL)
            { tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
              *(ttablenum *)varbl=arg3;
            }
          }
          if (fex->arg4->sqe==SQE_VAR)
          { t_expr_var *varex = (t_expr_var *)fex->arg4;
            if (varex->level!=SYSTEM_VAR_LEVEL)
            { tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
              strcpy(varbl, arg4);
            }
          }
          if (fex->arg5->sqe==SQE_VAR)
          { t_expr_var *varex = (t_expr_var *)fex->arg5;
            if (varex->level!=SYSTEM_VAR_LEVEL)
            { tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
              *(ttablenum *)varbl=arg5;
            }
          }
        }
        break;
      }
      case 174: // MailGetType
        result=cd_MailGetType(cdp);  break;
      case 175: // MailCloseInBox
        cd_MailCloseInBox(cdp, (HANDLE)a1.i64val);  break;
      case 176: // MailDial
        result=cd_MailDial(cdp, a1.is_null() ? NULL : a1.valptr()); break;
      case 177: // MailCloseInBox
        result=cd_MailHangUp(cdp); break;
      case FCNUM_MBCRETBL:
        result=cd_MailCreInBoxTables(cdp, a1.is_null() ? NULL : a1.valptr()); break;
      case FCNUM_MBGETMSGEX:
        result=cd_MailBoxGetMsg(cdp, (HANDLE)a1.i64val, a2.intval, a3.intval); break;
      case FCNUM_MLADDBLOBR:
        result=cd_LetterAddBLOBr(cdp, (HANDLE)a1.i64val, a2.is_null() ? NULL : a2.valptr(), a3.intval, a4.intval, (tattrib)a5.intval, (uns16)a6.intval); break;
      case FCNUM_MLADDBLOBRW:
        result=cd_LetterAddBLOBrW(cdp, (HANDLE)a1.i64val, a2.is_null() ? NULL : (const wuchar *)a2.valptr(), a3.intval, a4.intval, (tattrib)a5.intval, (uns16)a6.intval); break;
      case FCNUM_MLADDBLOBS:
        result=cd_LetterAddBLOBs(cdp, (HANDLE)a1.i64val, a2.is_null() ? NULL : a2.valptr(), a3.valptr(), a4.valptr(), a5.valptr()); break;
      case FCNUM_MLADDBLOBSW:
        result=cd_LetterAddBLOBsW(cdp, (HANDLE)a1.i64val, a2.is_null() ? NULL : (const wuchar *)a2.valptr(), (const wuchar *)a3.valptr(), (const wuchar *)a4.valptr(), (const wuchar *)a5.valptr()); break;
      case FCNUM_MBSAVETODBR:
        result=cd_MailBoxSaveFileDBr(cdp, (HANDLE)a1.i64val, a2.intval, a3.intval, a4.is_null() ? NULL : a4.valptr(), a5.intval, a6.intval, (tattrib)a7.intval, (uns16)a8.intval); break;
      case FCNUM_MBSAVETODBS:                                            
        result=cd_MailBoxSaveFileDBs(cdp, (HANDLE)a1.i64val, a2.intval, a3.intval, a4.is_null() ? NULL : a4.valptr(), a5.valptr(), a6.valptr(), a7.valptr()); break;
      case FCNUM_MCREATEPROF:
        result=cd_MailCreateProfile(cdp, a1.is_null() ? NULL : a1.valptr(), a2.intval); break;
      case FCNUM_MDELETEPROF:
        result=cd_MailDeleteProfile(cdp, a1.is_null() ? NULL : a1.valptr()); break;
      case FCNUM_MSETPROF:
        result=cd_MailSetProfileProp(cdp, a1.is_null() ? NULL : a1.valptr(), a2.is_null() ? NULL : a2.valptr(), a3.is_null() ? NULL : a3.valptr()); break;
      case FCNUM_MGETPROF:
      { char *arg3 = new char[a4.intval];
        if (!arg3)
          result=OUT_OF_KERNEL_MEMORY;
        else
        { result=cd_MailGetProfileProp(cdp, a1.is_null() ? NULL : a1.valptr(), a2.is_null() ? NULL : a2.valptr(), arg3, a4.intval);
          if (fex->arg3->sqe==SQE_VAR)
          { t_expr_var *varex = (t_expr_var *)fex->arg3;
            if (varex->level!=SYSTEM_VAR_LEVEL)
            { tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
              strcpy(varbl, arg3);
            }
          }
          delete arg3;
        }
        break;
      }
      case FCNUM_INITWBMAILEX:  // InitWBMailEx
        result=cd_InitWBMailEx(cdp, a1.is_null() ? NULL : a1.valptr(), a2.is_null() ? NULL : a2.valptr(), a3.is_null() ? NULL : a3.valptr());  break;
    } // switch
  }
#ifdef WINS
  __except ((GetExceptionCode() != 0) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
  { char buf[100];
    sprintf(buf, ">> Exception %u occured in function %u.", GetExceptionCode(), fex->num);
    dbg_err(buf);
    result=9999;
  }
#endif
  return result;
}

#include "fulltext.cpp"

sig64 WINAPI sp_round64(double r)
{ return (r==NULLREAL) ? NONEBIGINT : ((r<0.0f) ? (sig64)(r-0.50000000001):(sig64)(r+0.50000000001)); }

BOOL eval_fulltext_predicate(cdp_t cdp, t_ft_node * top, t_docid searched_docid)
{ top->open_index(cdp, TRUE);
  top->next_doc(cdp, NO_DOC_ID);
  t_docid min_docid;
  BOOL found;
  do
  { min_docid = 0;
    found = (cdp->__fulltext_weight = top->satisfied(&min_docid, &cdp->__fulltext_position)) > 0;
    if (min_docid>=searched_docid) break;
    top->next_doc(cdp, min_docid);
  } while (TRUE);
  top->close_index(cdp);
  return min_docid==searched_docid && found;
}

void write_int_outpar(cdp_t cdp, t_express * par, sig32 val)
{ if (par->sqe==SQE_VAR) // other params not supported
  { t_expr_var * varex = (t_expr_var*)par;
    if (varex->level!=SYSTEM_VAR_LEVEL) 
    { tptr varptr = variable_ptr(cdp, varex->level, varex->offset);
      if (varptr) *(sig32*)varptr=val;
    }
  }
}

void write_string_outpar(cdp_t cdp, t_express * par, const char * val)
{ if (par->sqe==SQE_VAR) // other params not supported
  { t_expr_var * varex = (t_expr_var*)par;
    if (varex->level!=SYSTEM_VAR_LEVEL) 
    { tptr varptr = variable_ptr(cdp, varex->level, varex->offset);
      if (varptr) strmaxcpy(varptr, val, par->specif.length+1);
    }
  }
}

/******************************** strings ***********************************/
static const unsigned char alike_tab[] =
{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '{', '|', '}', '~',
  127,
/*0   1   2   3    4   5   6   7    8   9   A   B    C   D   E   F */
 128,129,130,131, 132,133,134,135, 136,137,'S',139, 'S','T','Z','Z',
 144,145,146,147, 148,149,150,151, 152,153,'S',155, 'S','T','Z','Z',
 160,161,162,'L', 'A',165,166,167, 168,169,'S',171, 172,173,174,'Z',
 176,177,178,'L', 180,181,182,183, 184,'A','S',187, 'L',189,'L','Z',
 'R','A','A','A', 'A','L','C','C', 'C','E','E','E', 'E','I','I','D',
 'D','N','N','O', 'O','O','O',215, 'R','U','U','U', 'U','Y','T',223,
 'R','A','A','A', 'A','L','C','C', 'C','E','E','E', 'E','I','I','D',
 'D','N','N','O', 'O','O','O',247, 'R','U','U','U', 'U','Y','T',255 };

static uns8 alike(uns8 c)
{ if (c<' ') return ' ';
  if (c<'a') return c;
  return alike_tab[c-'a'];
}

BOOL Like(const char * s1, const char * s2)
{ while (*s1 && *s2)
    if (*s1==' ') s1++;
    else if (*s2==' ') s2++;
    else
    { if (alike(*s1)!=alike(*s2)) return FALSE;
      s1++; s2++;
    }
  while (*s1==' ') s1++;
  while (*s2==' ') s2++;
  return !(*s1 | *s2);
}

////////////////////////////////////////////////////////////////////////////
BOOL compare_tables(cdp_t cdp, tcursnum tb1, tcursnum tb2,
                    trecnum * diffrec, int * diffattr, int * result);

bool write_int_val(cdp_t cdp, t_express * ex, int val)
{
  if (ex->sqe!=SQE_VAR) return false;  // other params not supported
  t_expr_var * varex = (t_expr_var*)ex;
  if (varex->level==SYSTEM_VAR_LEVEL) return false;
  tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
  *(sig32*)varbl=val;  // will not overflow because the parameter is of INT type
  return true;
}


static void function_call(cdp_t cdp, t_expr_funct * fex, t_value * res)
// Calls a standard function [fex] and stores the result to [res].
{ t_value a1, a2, a3, a4, a5, a6, a7, a8;
  if (fex->arg1!=NULL) expr_evaluate(cdp, fex->arg1, &a1);
  if (fex->arg2!=NULL) expr_evaluate(cdp, fex->arg2, &a2);
  if (fex->arg3!=NULL) expr_evaluate(cdp, fex->arg3, &a3);
  if (fex->arg4!=NULL) expr_evaluate(cdp, fex->arg4, &a4);
  if (fex->arg5!=NULL) expr_evaluate(cdp, fex->arg5, &a5);
  if (fex->arg6!=NULL) expr_evaluate(cdp, fex->arg6, &a6);
  if (fex->arg7!=NULL) expr_evaluate(cdp, fex->arg7, &a7);
  if (fex->arg8!=NULL) expr_evaluate(cdp, fex->arg8, &a8);
  res->set_simple_not_null(); // preset, changed when the result is NULL
  switch (fex->num)
  {
    case FCNUM_POSITION:  // for char strings only in SQL 2
    case 127: // Strpos function
      // ignoring case if ANY of the operands has the "ignore case" flag (from 2007-11-29, testing the 2nd operand only before this date)
      if (a1.load(cdp)) { a1.set_null();  break; } // error requested
      if      (a1.is_null()) res->intval=1; // differs from SQL 2
      else if (a2.is_null()) res->intval=0; // differs from SQL 2
      else
      { if (IS_EXTLOB_TYPE(fex->arg2->type) && a2.vmode==mode_access)
          a2.load(cdp);
        if (a2.vmode==mode_access) // search string in text
        { int flags = fex->arg2->specif.ignore_case || fex->arg1->specif.ignore_case ? 0 : SEARCH_CASE_SENSITIVE;
          if (fex->arg1->specif.wide_char) flags |= SEARCH_WIDE_CHAR;
          if (var_search(cdp, tables[a2.dbacc.tb], a2.dbacc.rec, a2.dbacc.attr, a2.dbacc.index,
              0, flags, fex->arg1->specif.charset,
              fex->arg1->specif.wide_char ? (int)wuclen(a1.wchptr()) : (int)strlen(a1.valptr()), a1.valptr(), (uns32 *)&res->intval))
            res->set_null();  // error
          res->intval++;
        }
        else // loaded string in loaded string
        { t_specif spec = fex->arg2->specif;
          if (fex->arg1->specif.ignore_case)
            spec.ignore_case=1;
          res->intval=general_Substr(a1.valptr(), a2.valptr(), spec);
        }  
      }
      break;
    case FCNUM_CHARLEN:  
      if (a1.is_null()) res->intval=0; // differs from SQL 2
      else
      { res->intval=a1.length;
        if (fex->arg1->specif.wide_char) res->intval /= 2;
      }
      break;
    case FCNUM_OCTETLEN:  
      if (a1.is_null()) res->intval=0; // differs from SQL 2
      else res->intval=a1.length;
      break;
    case FCNUM_BITLEN:
      if (a1.is_null()) res->intval=0; // differs from SQL 2
      else
        if (fex->arg1->type==ATT_BOOLEAN) res->intval=1; 
        else res->intval=8*a1.length;  // for binary and character string, including UNICODE
      break;
    case FCNUM_EXTRACT_YEAR:
      if (a1.is_null()) res->set_null();
      else
      { int tp=fex->arg1->type;
        if      (tp==ATT_DATE) res->intval=Year(a1.intval);
        else if (tp==ATT_TIME) res->intval=0;
        else                   res->intval=Year(timestamp2date(a1.intval));
      }
      break;
    case FCNUM_EXTRACT_MONTH:
      if (a1.is_null()) res->set_null();
      else
      { int tp=fex->arg1->type;
        if      (tp==ATT_DATE) res->intval=Month(a1.intval);
        else if (tp==ATT_TIME) res->intval=0;
        else                   res->intval=Month(timestamp2date(a1.intval));
      }
      break;
    case FCNUM_EXTRACT_DAY:
      if (a1.is_null()) res->set_null();
      else
      { int tp=fex->arg1->type;
        if      (tp==ATT_DATE) res->intval=Day(a1.intval);
        else if (tp==ATT_TIME) res->intval=0;
        else                   res->intval=Day(timestamp2date(a1.intval));
      }
      break;
    case FCNUM_EXTRACT_HOUR:
      if (a1.is_null()) res->set_null();
      else
      { int tp=fex->arg1->type;
        if      (tp==ATT_DATE) res->intval=0;
        else if (tp==ATT_TIME) res->intval=Hours(a1.intval);
        else                   res->intval=Hours(timestamp2time(a1.intval));
      }
      break;
    case FCNUM_EXTRACT_MINUTE:
      if (a1.is_null()) res->set_null();
      else
      { int tp=fex->arg1->type;
        if      (tp==ATT_DATE) res->intval=0;
        else if (tp==ATT_TIME) res->intval=Minutes(a1.intval);
        else                   res->intval=Minutes(timestamp2time(a1.intval));
      }
      break;
    case FCNUM_EXTRACT_SECOND:
      if (a1.is_null()) res->set_null();
      else
      { int tp=fex->arg1->type;
        if      (tp==ATT_DATE) res->realval=0;
        else if (tp==ATT_TIME) res->realval=Seconds(a1.intval)+(double)Sec1000(a1.intval)/1000;
        else
        { uns32 tm=timestamp2time(a1.intval);
          res->realval=Seconds(tm) + (double)Sec1000(tm) / 1000;
        }
      }
      break;
    case 126: // Strcopy function
    case FCNUM_SUBSTRING: // character or byte substring, bit substrings not supported
      if (a1.is_null() || a2.is_null() || fex->arg3 && (a3.is_null() || a3.intval<1))
        { res->set_null();  res->length=0; }
      else
      { int size, start, limit;  
        if (a2.intval<1) start=0;  else start=a2.intval-1;  // start is 0-based 
        if (fex->arg3) limit = a3.intval;  else limit = 0x3fffffff;
        if (fex->arg1->specif.wide_char)  // for UNICODE convert start and size to bytes
          { limit*=2;  start*=2; }
        size = start < a1.length ? a1.length-start : 0; // bytes available from source 
        if (size > limit) size=limit;                   // bytes to be copied
        if (res->allocate(size)) break; // sets simple_not_null or indirect
        if (a1.vmode==mode_access)
        { if (a1.dbacc.index==NOINDEX)
            tb_read_var(cdp, tables[a1.dbacc.tb], a1.dbacc.rec, a1.dbacc.attr, start, size, res->valptr());
          else
            tb_read_ind_var(cdp, tables[a1.dbacc.tb], a1.dbacc.rec, a1.dbacc.attr, a1.dbacc.index, start, size, res->valptr());
        }
        else  // the value is loaded
          memcpy(res->valptr(), a1.valptr()+start, size);
        if (is_char_string(fex->origtype)) res->valptr()[size]=res->valptr()[size+1]=0;  // terminate with 0
        else if (fex->origtype==ATT_BINARY && size < fex->origspecif.length) // padd with 0s
          memset(res->valptr()+size, 0, fex->origspecif.length-size);
        res->length=size;  // not set outside!
      }
      break;
    case FCNUM_LOWER:
      if (a1.is_null() || a1.load(cdp)) res->set_null();
      else if (res->allocate(a1.length)) request_error(cdp, OUT_OF_KERNEL_MEMORY);
      else
      { if (fex->arg1->specif.wide_char)
          convert_to_lowercaseW(a1.wchptr(), res->wchptr(), fex->arg1->specif.charset);
        else
          convert_to_lowercaseA(a1.valptr(), res->valptr(), fex->arg1->specif.charset);
        res->length=a1.length;  
      }
      break;
    case FCNUM_UPPER:
      if (a1.is_null() || a1.load(cdp)) res->set_null();
      else if (res->allocate(a1.length)) request_error(cdp, OUT_OF_KERNEL_MEMORY);
      else
      { if (fex->arg1->specif.wide_char)
          convert_to_uppercaseW(a1.wchptr(), res->wchptr(), fex->arg1->specif.charset);
        else
          convert_to_uppercaseA(a1.valptr(), res->valptr(), fex->arg1->specif.charset);
        res->length=a1.length;  
      }
      break;
    case FCNUM_USER:
      strcpy(res->strval, cdp->prvs.luser_name());
      res->length=(int)strlen(res->strval);
      break;
    case FCNUM_USER_BINARY:
      memcpy(res->strval, cdp->prvs.user_uuid, UUID_SIZE);
      res->length=UUID_SIZE;
      break;
    case FCNUM_FULLTEXT:
    { t_ft_kontext * ftk = get_fulltext_kontext(cdp, a1.valptr(), NULL, NULL);
      if (!ftk) res->set_null();
      else
      { t_ft_node * top;
        if (analyse_fulltext_expression(cdp, a3.valptr(), &top, ftk, fex->arg3->specif))  // unless dictionary not found or lemmatisation not initialized
        { if (!top) res->intval=FALSE; // to weak!
          else res->intval=eval_fulltext_predicate(cdp, top, a2.i64val);
          delete top;
        }
        else res->intval=FALSE;  // error may have been handled
      }
      break;
    }
    case FCNUM_FULLTEXT_CONTEXT:
    case FCNUM_FULLTEXT_CONTEXT2:  // v8 only
    { a1.load(cdp);
      if (a2.is_null()) { a2.set_simple_not_null();  a2.strval[0]=0; }
      if (a3.is_null()) a3.intval=0;
      if (a4.is_null()) a4.intval=0;
      if (a5.is_null()) a5.intval=0;
      if (a6.is_null()) { a6.set_simple_not_null();  a6.strval[0]=0; }
      char * suff = a6.valptr();
      while (*suff && *suff!='%') suff++;
      if (*suff=='%') { *suff=0;  suff++; }
      if (fex->num==FCNUM_FULLTEXT_CONTEXT2)
      { a7.load(cdp);
        ft_context(cdp, &a1, a2.valptr(), a3.intval, a4.intval, a5.intval, a5.intval, a6.valptr(), suff, res, &a7);
      }
      else
        ft_context(cdp, &a1, a2.valptr(), a3.intval, a4.intval, a5.intval, a5.intval, a6.valptr(), suff, res, NULL);
      break;
    }
    case FCNUM_GET_FULLTEXT_CONTEXT:   // v9 only
    { t_ft_kontext * ftk = get_fulltext_kontext(cdp, a1.valptr(), NULL, NULL);
      if (!ftk) res->set_null();
      else
      { a2.load(cdp);
        if (a3.is_null()) { a3.set_simple_not_null();  a3.strval[0]=0; }  // format
        if (a4.is_null()) a4.intval=0;  // mode
        if (a5.is_null()) a5.intval=0;  // position
        if (a6.is_null()) a6.intval=1;  // word count
        if (a7.is_null()) a7.intval=0;  // margin
        if (a8.is_null()) { a8.set_simple_not_null();  a8.strval[0]=0; }  // decoration
        char * suff = a8.valptr();
        while (*suff && *suff!='%') suff++;
        if (*suff=='%') { *suff=0;  suff++; }
        ftk->ft_context9(cdp, &a2, a3.valptr(), a4.intval, fex->arg1->specif, a5.intval, a6.intval, a7.intval, a7.intval, a8.valptr(), suff, res); 
      }
      break;
    }
    case FCNUM_SELPRIV:  case FCNUM_UPDPRIV:
    case FCNUM_DELPRIV:  case FCNUM_INSPRIV:  case FCNUM_GRTPRIV:
      res->intval=get_privilege(cdp, fex->num, &a1, a2.intval, fex->arg3 ? a3.valptr() : NULL, a4.intval);  
      if (res->intval==-1) res->set_null();
      break;
    case FCNUM_ADMMODE:
    { tobjnum procnum;
      if (a3.intval!=CATEG_PROC && a3.intval!=CATEG_CURSOR)
        { res->set_null();  break; }   // object not found
      if (name_find2_obj(cdp, a2.valptr(), a1.is_null() ? NULL : a1.valptr(), a3.intval, &procnum))
        { res->set_null();  break; }   // object not found
      WBUUID uuid;
      tb_read_atr(cdp, objtab_descr, procnum, APPL_ID_ATR, (tptr)uuid);
      if (a4.is_null())  // only asking
        res->intval = is_in_admin_mode(cdp, procnum, uuid);
      else  // test author role:
        if (!cdp->prvs.am_I_appl_author(cdp, uuid)) res->set_null();
      else if (a4.intval) // set or reset the admin mode
        res->intval = set_admin_mode(cdp, procnum, uuid);
      else
        res->intval = clear_admin_mode(cdp, procnum, uuid);
      break;
    }
    case FCNUM_CONSTRAINS_OK:
    { t_expr_attr * atex = (t_expr_attr*)fex->arg1;
      db_kontext * _kont;  int _elemnum;
      resolve_access(atex->kont, atex->elemnum, _kont, _elemnum);
      elem_descr * el = _kont->elems.acc0(_elemnum);
      table_descr_auto tbdf(cdp, el->dirtab.tabnum);
      if (tbdf->me()!=NULL) 
      { BOOL nic;
        int err = check_constrains(cdp, tbdf->me(), _kont->position, NULL, 2, &nic, res->strval);
        if (err==TRUE)  /* other error */ strcpy(res->strval, "???");
        res->length=(int)strlen(res->strval);
        if (!res->length) res->set_null();
      }
      break;
    }
    case FCNUM_COMP_TAB:
    { char * stmt = a1.valptr();
      t_rscope * rscope = cdp->rscope;
      t_scope * scope_list = create_scope_list_from_run_scopes(rscope);  // rscope changed by this call, must not use cdp->rscope!
      t_sql_kont sql_kont;  sql_kont.active_scope_list=scope_list;
      res->intval=-5;
      cur_descr * cd1, * cd2;
      tcursnum curs1 = open_working_cursor(cdp, a1.valptr(), &cd1);
      if (curs1!=NOCURSOR)
      { tcursnum curs2 = open_working_cursor(cdp, a2.valptr(), &cd2);
        if (curs2!=NOCURSOR)
        { trecnum diffrec;  int diffattr;  int result;
          compare_tables(cdp, curs1, curs2, &diffrec, &diffattr, &result);
          write_int_outpar(cdp, fex->arg3, diffrec);
          write_int_outpar(cdp, fex->arg4, diffattr);
          res->intval=result;
          close_working_cursor(cdp, curs2); 
        }
        close_working_cursor(cdp, curs1); 
      }
#if 0
      t_query_expr * qe1 = NULL, * qe2= NULL;  t_optim * opt1 = NULL, * opt2 = NULL;
      BOOL ret1=compile_query(cdp, a1.valptr(), sql_kont, &qe1, &opt1);
      BOOL ret2=compile_query(cdp, a1.valptr(), sql_kont, &qe2, &opt2);
      if (!ret1 && !ret2)
      { tcursnum curs create_cursor(cdp_t cdp, t_query_expr * qe, tptr source, t_optim * opt, cur_descr ** pcd)
      }
      else
        res->intval=-5;
      delete qe1;  delete qe2;  delete opt1;  delete opt2;
#endif
      break;
    }
    case 72:  res->i64val = (size_t)wbt_create_semaphore(a1.valptr(), a2.intval);  break;  // ATTN: the handle of the semaphore may have 64 bits on 64bit platform
    case 73:  wbt_close_semaphore((HANDLE)(size_t)a1.i64val);  break;  // ATTN: the handle of the semaphore may have 64 bits on 64bit platform
    case 74:  wbt_release_semaphore((HANDLE)(size_t)a1.i64val);  break;  // ATTN: the handle of the semaphore may have 64 bits on 64bit platform
    case 75:  cdp->wait_type=WAIT_SEMAPHORE;
              integrity_check_planning.thread_operation_stopped();
              res->intval = wbt_wait_semaphore((HANDLE)(size_t)a1.i64val, a2.intval);  // ATTN: the handle of the semaphore may have 64 bits on 64bit platform
              integrity_check_planning.continuing_detached_operation();
              cdp->wait_type=WAIT_NO;  break; 
    case 76:  cdp->wait_type=WAIT_SLEEP;
              integrity_check_planning.thread_operation_stopped();
              res->intval =Sleep_cond(a1.is_null() ? 0 : a1.intval);  
              integrity_check_planning.continuing_detached_operation();
              cdp->wait_type=WAIT_NO;  break;
    case 77:  res->intval =Current_date(cdp);  break;
    case 78:  res->intval =Current_time(cdp);  break;
    case 79:  res->intval =Current_timestamp(cdp);  break;
    case 80:  res->intval =Make_date(a1.intval, a2.intval, a3.intval);          break;
    case 81:  res->intval =Day     (a1.intval);   break;
    case 82:  res->intval =Month   (a1.intval);   break;
    case 83:  res->intval =Year    (a1.intval);   break;
    case 84:  res->intval =Today   ();            break;
    case 85:  res->intval =Make_time(a1.intval, a2.intval, a3.intval, a4.intval);  break;
    case 86:  res->intval =Hours   (a1.intval);   break;
    case 87:  res->intval =Minutes (a1.intval);   break;
    case 88:  res->intval =Seconds (a1.intval);   break;
    case 89:  res->intval =Sec1000 (a1.intval);   break;
    case 90:  res->intval =Now     ();            break;
    case 91:  res->intval =sp_ord  (a1.strval[0]);  break;
    case 92:  *res->strval=sp_chr  (a1.intval);  res->strval[1]=0;  break;
    case 93:  res->intval =sp_odd  (a1.intval);   break;
    case 94:  res->realval=sp_abs  (a1.realval);  break;
    case 95:  res->intval =sp_iabs (a1.intval);   break;
    case 96:  res->realval=sp_sqrt (a1.realval);  break;
    case 97:  res->intval =sp_round(a1.realval);  break;
    case 98:  res->intval =sp_trunc(a1.realval);  break;
    case 99:  res->realval=sp_sqr  (a1.realval);  break;
    case 100: res->intval =sp_isqr (a1.intval);   break;
    case 101: res->realval=sp_sin  (a1.realval);  
       //dump_thread_overview(cdp);
       break;
    case 102: res->realval=sp_cos  (a1.realval);  break;
    case 103: res->realval=sp_atan (a1.realval);  break;
    case 104: res->realval=sp_log  (a1.realval);  break;
    case 105: res->realval=sp_exp  (a1.realval);  break;
    case LIKE_FCNUM: 
      res->intval=Like(a1.valptr(), a2.valptr());  // this is the original ~ function ignoring case and diacritics
      //like_esc(a1.valptr(), a2.valptr(), 0, TRUE, fex->arg1->specif.charset);  -- this is SQL LIKE, implemented as ternary operation
      break;
    case 107: res->intval =general_Pref  (a1.valptr(), a2.valptr(), t_specif(0xffff,0,FALSE,FALSE));      break;
    case 108: res->intval =general_Substr(a1.valptr(), a2.valptr(), t_specif(0xffff,0,FALSE,FALSE)) > 0;  break;
    case 109: // strcat 
      if (a1.is_null()) { a1.length=0;  a1.strval[0]=0; } // OK (NULL string like empty)
      if (a2.is_null()) { a2.length=0;  a2.strval[0]=0; } // OK (NULL string like empty)
      if (res->allocate(a1.length+a2.length))
        { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }
      memcpy(res->valptr(),           a1.valptr(), a1.length);  
      strcpy(res->valptr()+a1.length, a2.valptr());  
      res->valptr()[a1.length+a2.length+1]=0;
      //res->length=a1.length+a2.length;  defined outside
      break;

    case 112: res->intval =sp_str2int    (a1.valptr());  break;
    case 113:  // string to money and money to i64
      res->i64val = money2i64(sp_str2money(a1.valptr()));  break;
    case 114: res->realval=sp_str2real   (a1.valptr());  break;
    case 115: int2str(a1.intval, res->strval);           break; // number fixed
    case 116: if (a1.is_null()) res->strval[0]=0;                                 // number fixed
              else money2str((monstr*)&a1.i64val, res->strval, (sig8)a2.intval);
              break; 
    case 117: real2str(a1.realval, res->strval, a2.intval);  break; // number fixed

    case 124: // strinsert (8-bit arguments and result only)
    { int len, part1, fill;
      if (a3.is_null() || a3.intval<=0)  { res->set_null();  break; } // Error, ignored
      if (a2.is_null()) { a2.length=0;  a2.strval[0]=0; } // OK (inserting into NULL string like into empty)
      if (a1.is_null()) a1.length=0;  // OK (inserting NULL string -> no action)
      a3.intval-=1;  // converted to 0-based
      if (a3.intval > a2.length)  // must fill
        { part1=a2.length;  fill=a3.intval - a2.length; }
      else                        // no filling
        { part1=a3.intval;  fill=0; }
      len=a1.length+a2.length+fill;
      if (res->allocate(len)) 
        { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }// sets simple_not_null or indirect
      tptr dest=res->valptr();
      memcpy(dest, a2.valptr(), part1);      dest+=part1;
      memset(dest, ' ',         fill);       dest+=fill;
      memcpy(dest, a1.valptr(), a1.length);  dest+=a1.length;
      strcpy(dest, a2.valptr()+part1);   
      //res->length=len;  defined outside
      break;
    }
    case 125: // strdelete (8-bit arguments and result only)
    { int del, part1;
      if (a2.is_null() || a2.intval<=0) { res->set_null();  break; } // Error, ignored
      if (a3.is_null() || a3.intval<0)  { res->set_null();  break; } // Error, ignored
      if (a1.is_null()) { res->set_null();  break; }  // OK
      a2.intval-=1;  // converted to 0-based
      if (a2.intval >= a1.length) { del=0;  part1=a1.length; }
      else // deleting something
      { part1=a2.intval;
        if (a2.intval+a3.intval > a1.length) del=a1.length-a2.intval;
        else del=a3.intval;
      }
      if (res->allocate(a1.length-del)) 
        { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }// sets simple_not_null or indirect
      tptr dest=res->valptr();
      memcpy(dest,       a1.valptr(), part1);
      strcpy(dest+part1, a1.valptr()+part1+del);
      //res->length=a1.length-del;  defined outside
      break;
    }
    case 128: res->intval = a1.is_null() ? 0 : a1.length;         break;
    case 129: res->intval =sp_str2date  (a1.valptr());            break;
    case 130: res->intval =sp_str2time  (a1.valptr());            break;
    case 131: date2str(a1.intval, res->strval, a2.intval);  break; // number fixed
    case 132: time2str(a1.intval, res->strval, a2.intval);  break; // number fixed
    case 133: res->intval =Day_of_week  (a1.intval);           break;
    case 134: 
    { tptr p = a1.valptr();  cutspaces(p);
      if (res->allocate((int)strlen(p))) 
        { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }// sets simple_not_null or indirect
      strcpy(res->valptr(), p);  //   res->length defined below
      break;
    }
    case 137: res->intval =WinBase602_version();                  break;
    case 138: res->intval =Quarter      (a1.intval);              break;
    case 139: res->intval =Waits_for_me (cdp, *(val2uuid*)a1.valptr());  break;

    case 145: res->intval =datetime2timestamp(a1.intval, a2.intval);  break;
    case 146: res->intval =timestamp2date(a1.intval);                 break;
    case 147: res->intval =timestamp2time(a1.intval);                 break;
    case 148: // Is_repl_destin
      res->intval = !memcmp(a1.valptr(), cdp->dest_srvr_id, UUID_SIZE);  break; 
    case 149: write_to_log(cdp, a1.valptr());  break;
    case 150: // Free_deleted
    { tobjnum objnum;
      if (find_obj(cdp, a1.valptr(), CATEG_TABLE, &objnum))  // accepts schema name (since WB 6.0c)
      { res->intval=TRUE;  
        SET_ERR_MSG(cdp, a1.valptr());  request_error(cdp, OBJECT_DOES_NOT_EXIST);  // the error may have been handled 
        break; 
      }
      if      (objnum== TAB_TABLENUM) res->intval = free_deleted(cdp, tabtab_descr);
      else if (objnum== OBJ_TABLENUM) res->intval = free_deleted(cdp, objtab_descr);
      else if (objnum==USER_TABLENUM) res->intval = free_deleted(cdp, usertab_descr);
      else
      { table_descr_auto tbdf(cdp, objnum);
        if (tbdf->me()==NULL) res->intval = TRUE;
        else                  res->intval = free_deleted(cdp, tbdf->me());
      }
      break;
    }
    case FCNUM_CURR_SCHEMA: // Current_application
      strcpy(res->strval, cdp->sel_appl_name);  break;
    case 152: // Client_number
      res->intval=cdp->applnum_perm;  break;
    case 153: res->intval =sp_str2timestamp(a1.valptr());            break;
    case 154: timestamp2str(a1.intval, res->strval, a2.intval);  break;// number fixed
    case 155: // Set_sql_option
      if (a1.is_null()) a1.intval=0;
      if (a2.is_null()) a2.intval=0;
      cdp->sqloptions = (cdp->sqloptions & ~a1.intval) | (a2.intval & a1.intval);  
      res->intval=FALSE;
      break;
    case 156: // Exec
      if (a1.load(cdp)) { a1.set_null();  break; } // error requested
      if (a2.load(cdp)) { a2.set_null();  break; } // error requested
      if (a1.is_null()) a1.strval[0]=0;
      if (a2.is_null()) a2.strval[0]=0;
      res->intval=exec_process(cdp, a1.valptr(), a2.valptr(), a3.intval);
      break;

    case FCNUM_DEFINE_LOG:
      if (a1.is_null()) a1.strval[0]=0;
      if (a2.is_null()) a2.strval[0]=0;
      if (a3.is_null()) a3.strval[0]=0;
      res->intval=define_trace_log(cdp, a1.valptr(), a2.valptr(), a3.valptr()); 
      break;
    case FCNUM_TRACE:
      if (a1.is_null()) a1.intval=0;
      if (a2.is_null()) a2.strval[0]=0;
      if (a3.is_null()) a3.strval[0]=0;
      if (a4.is_null()) a4.strval[0]=0;
      if (a5.is_null()) a5.intval=0;
      res->intval=trace_def(cdp, a1.intval, a2.valptr(), a3.valptr(), a4.valptr(), a5.intval); 
      break;

    case FCNUM_FT_CREATE:
    { res->intval = create_fulltext_system(cdp, a1.valptr(), a2.valptr());  // creating v8 fulltext system
      break;
    }
    case FCNUM_FT_INDEX:
    { t_ft_kontext * ftk = get_fulltext_kontext(cdp, a1.valptr(), NULL, NULL);
      if (!ftk) res->intval=0;
      else
      { ftk->ft_remove_doc_occ(cdp, a2.intval);
        res->intval = ftk->ft_index_doc(cdp, a2.i64val, &a3, a4.valptr(), a5.intval, fex->arg3->specif);
      }
      break;
    }
    case FCNUM_FT_REMOVE:
    { t_ft_kontext * ftk = get_fulltext_kontext(cdp, a1.valptr(), NULL, NULL);
      if (ftk) ftk->ft_remove_doc_occ(cdp, a2.intval);  break;
    }
    case FCNUM_FT_DESTROY: // must work even if I cannot create the kontext (system is partially deleted)
      res->intval = ft_destroy(cdp, a1.valptr());  break;
    case FCNUM_SQL_EXECUTE:
      res->intval = 0;
      if (a1.load(cdp)) { a1.set_null();  break; } // error requested
      if (!a1.is_null())  // empty string is an empty statement
      { char * stmt = a1.valptr();
        t_rscope * rscope = cdp->rscope;
        t_scope * scope_list = create_scope_list_from_run_scopes(rscope);  // rscope changed by this call, must not use cdp->rscope!
        compil_info xCI(cdp, stmt, sql_statement_seq);
        t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  xCI.sql_kont->active_scope_list=scope_list;
        int err=compile_masked(&xCI);
        sql_statement * so = (sql_statement*)xCI.stmt_ptr;
        if (err) /* error in compilation */ 
        { delete so;  
          res->intval = err;  // returns the compilation error number
        }
        else  // compiled OK
        { t_exkont_sqlstmt ekt(cdp, cdp->sel_stmt, stmt);
          if (cdp->dbginfo) cdp->dbginfo->disable();
          // sql_exec_top_list(cdp, so);  // must not copy SQL results to answer! -- no, must not do the processing by the global handler, using local handlers instead
         // allow commit inside SQL_execute:
          bool saved_in_expr_eval;
          saved_in_expr_eval=cdp->in_expr_eval;          
          cdp->in_expr_eval=false;
         // execute the statement list until exception:
          sql_statement * aso = so;
          do  /* cycle on sql statements separated by semicolons */
          { cdp->statement_counter++;
            aso->exec(cdp);
            process_rollback_conditions_by_cont_handler(cdp);
            aso=aso->next_statement;   /* to the next statement */
          } while (cdp->except_type==EXC_NO && aso!=NULL);
          cdp->in_expr_eval = saved_in_expr_eval;
          if (cdp->dbginfo) cdp->dbginfo->enable();
          delete so;  
          if (cdp->is_an_error()) res->intval=-1;
        }
      }
      break;
    case 198: strcpy(res->strval, cdp->prvs.luser_name()); break;
    case FCNUM_BIGINT2STR: 
      if (a1.is_null()) res->strval[0]=0;
      else int64tostr(a1.i64val, res->strval); 
      break; 
    case FCNUM_STR2BIGINT: 
      res->i64val = a1.is_null() ? NONEBIGINT : sp_str2bigint(a1.valptr());  
      break;
    case FCNUM_WB_BUILD:
      res->intval=VERS_4+(VERS_3<<16);  break;
    case FCNUM_ENUMRECPR:
    case FCNUM_ENUMTABPR:
    { int ordnum = a2.is_null() ? 0 : a2.intval;
      unsigned def_size;  uns8 * priv_def;
      if (fex->num==FCNUM_ENUMTABPR)
        priv_def=(uns8*)load_var_val(cdp, tabtab_descr, a1.dbacc.tb, TAB_DRIGHT_ATR, &def_size);
      else
      { tattrib attr = tables[a1.dbacc.tb]->rec_privils_atr;
        if (attr==NOATTRIB) { res->set_null();  break; }
        priv_def=(uns8*)load_var_val(cdp, tables[a1.dbacc.tb], a1.dbacc.rec, attr, &def_size);
      }
      int step=UUID_SIZE+*priv_def;  def_size--;
      if (def_size < (ordnum+1)*step) res->set_null();
      else 
      { memcpy(res->strval, priv_def+1+ordnum*step, UUID_SIZE);
        res->length=UUID_SIZE;
      }
      corefree(priv_def);
      break;
    }
    case FCNUM_SETPASS:
      if (a1.is_null()) { res->intval=TRUE;  break; }
      if (a2.is_null()) *a2.valptr()=0;  // NULL enabled in 8.0.5.2
      sys_Upcase(a1.valptr());  sys_Upcase(a2.valptr());
      res->intval=set_new_password(cdp, a1.valptr(), a2.valptr());
      break;
    case FCNUM_MEMBERSHIP:
    case FCNUM_MEMBERSHIP_GET:
    { tobjnum subject, container;  uns8 state;
      if (a2.is_null() || (a2.intval!=CATEG_USER && a2.intval!=CATEG_GROUP) ||
          a4.is_null() || (a4.intval!=CATEG_ROLE && a4.intval!=CATEG_GROUP) ||
          a1.is_null() || a3.is_null())
        { res->set_null();  break; }
      sys_Upcase(a1.valptr());  sys_Upcase(a3.valptr());
      if (find_obj(cdp, a1.valptr(), (tcateg)a2.intval, &subject)   ||
          find_obj(cdp, a3.valptr(), (tcateg)a4.intval, &container))
        { res->set_null();  break; }
      
      if (a5.is_null() || fex->num==FCNUM_MEMBERSHIP_GET && !a5.intval)  // get direct membership info
        if (user_and_group(cdp, subject, container, (tcateg)a4.intval, OPER_GET, &state))
          res->set_null();
        else
          res->intval=state;
      else if (fex->num==FCNUM_MEMBERSHIP) // set membership
      { state = (uns8)a5.intval;
        if (user_and_group(cdp, subject, container, (tcateg)a4.intval, OPER_SET, &state))
          res->set_null();
        else
          res->intval = TRUE;
      }
      else  // FCNUM_MEMBERSHIP_GET, get effective membership info
        res->intval = (int)get_effective_membership(cdp, subject, (tcateg)a2.intval, container, (tcateg)a4.intval);
      break;
    }
    case FCNUM_LOGWRITEEX:
      if (!a3.is_null() && a3.intval)  // NULL kontext or zero kontext: no action
        log_write_ex(cdp, a1.valptr(), a2.valptr(), a3.intval);
      break;
    case FCNUM_APPLSHARED:
      res->intval = Repl_appl_shared(cdp, a1.is_null() ? NULL : a1.strval);
      break;
    case FCNUM_ROUND64:  
      res->i64val =sp_round64(a1.realval);  break;
    case FCNUM_GETSQLOPT:
      res->intval=cdp->sqloptions;  break;
    case FCNUM_GETSERVERINFO:
      res->intval=0;  // return 0 for undefined parameter value
      get_server_info(cdp, a1.intval, (uns32*)&res->intval);  
      break;
    case FCNUM_ACTIVE_ROUT:  // Active_routine_name
      res->strval[0]=0;  // NULL result when cannot return valid name
      if (!a1.is_null() || a1.intval>=0)
      { // find the scope:
        for (t_rscope * rscope = cdp->rscope;  rscope;  rscope=rscope->next_rscope)
          if (rscope->sscope->params)
            if (a1.intval) a1.intval--;
            else  // find the name and return it
            { for (int i = 0;  i<rscope->sscope->locdecls.count();  i++)
              { t_locdecl * locdecl = rscope->sscope->locdecls.acc0(i);
                if (locdecl->loccat==LC_LABEL)  // the routine name
                { t_routine * rout = locdecl->label.rout;
                  if (res->allocate(5+2*OBJ_NAME_LEN+1)) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
                  tobjname name;
                  if (ker_apl_id2name(cdp, rout->schema_uuid, name, NULL)) *name=0;
                  tptr p = res->valptr();
                  *p='`';
                  strcpy(p+1, name);  strcat(p, "`.`");
                  strcat(p, rout->name);  strcat(p, "`");
                  goto found;
                }
              }
            } 
        found:;
      }
      break;
    case FCNUM_INVOKE_EVENT:
      if (a2.load(cdp)) { a2.set_null();  break; } // error requested
      cdp->cevs.invoke_event(cdp, a1.valptr(), a2.valptr());
      break;
    case FCNUM_GET_LIC_CNT:
#if WBVERS>=1100
      res->intval=-1;
#else      
      { int lics;
        if (a1.load(cdp)) { a1.set_null();  break; } // error requested
        get_client_licences(installation_key, a1.is_null() ? NULL : a1.valptr(), &lics);
        res->intval=lics;
      }
#endif      
      break;
    case FCNUM_GET_PROP:
      if (a1.load(cdp)) { a1.set_null();  break; } // error requested
      if (a2.load(cdp)) { a2.set_null();  break; } // error requested
      if (a1.is_null() || a2.is_null() || a3.is_null())
        res->strval[0]=0;
      else
      { char buf[256+1];
        get_property_value(cdp, a1.valptr(), a2.valptr(), a3.intval, buf, sizeof(buf));
        if (res->allocate((int)strlen(buf))) 
          { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }// sets simple_not_null or indirect
        strcpy(res->valptr(), buf);
      }
      break;
    case FCNUM_SET_PROP:
      if (a1.load(cdp)) { a1.set_null();  break; } // error requested
      if (a2.load(cdp)) { a2.set_null();  break; } // error requested
      if (a4.load(cdp)) { a4.set_null();  break; } // error requested
      if (a1.is_null() || a2.is_null() || a3.is_null())
        { res->intval=1;  break; }
      res->intval = 0 != set_property_value(cdp, a1.valptr(), a2.valptr(), a3.intval, a4.valptr(), a4.length);
      break;
    case FCNUM_LOAD_EXT:
      res->intval = load_server_extension(cdp, a1.valptr()) ? 1 : 0;
      break;
    case FCNUM_TRUCT_TABLE:
    { tobjnum objnum;
      if (find_obj(cdp, a1.valptr(), CATEG_TABLE, &objnum))  // accepts schema name 
      { res->intval=TRUE;  
        SET_ERR_MSG(cdp, a1.valptr());  request_error(cdp, OBJECT_DOES_NOT_EXIST);  // the error may have been handled 
        break; 
      }
      { table_descr_auto tbdf(cdp, objnum);
        if (tbdf->me()==NULL) res->intval = TRUE;
        else                  res->intval = truncate_table(cdp, tbdf->me());
      }
      break;
    }
    case FCNUM_GETSRVERRCNT:
    { t_exkont_info * peki, eki;  bool def=true;
      if (a1.is_null() || a1.intval<0)
        def=false;
      else if (cdp->handler_rscope && cdp->exkont_info)  // in handler use the snapshot
        if (a1.intval < cdp->exkont_len)
          peki=cdp->exkont_info+a1.intval;
        else def=false;  
      else if (a1.intval < cdp->execution_kontext->count())
      { t_exkont * exkont = cdp->execution_kontext;
        while (a1.intval--) exkont=exkont->_next();
        exkont->get_info(cdp, &eki);
        peki=&eki;
      }
      else def=false;
      if (!def)
        write_int_outpar(cdp, fex->arg2, EKT_NO);
      else  
      { t_exkont_type tp = peki->type;
        tobjnum objnum = peki->par1;
        write_int_outpar(cdp, fex->arg2, tp);
        write_int_outpar(cdp, fex->arg3, objnum);
        write_int_outpar(cdp, fex->arg4, peki->par2);
        write_int_outpar(cdp, fex->arg5, peki->par3);
        write_int_outpar(cdp, fex->arg6, peki->par4);
        char names[2*OBJ_NAME_LEN+1+1];  
        *names=0;  
        if (tp==EKT_TABLE || tp==EKT_INDEX || tp==EKT_RECORD || tp==EKT_LOCK && objnum!=-1)
        { table_descr_auto td(cdp, objnum);
          if (td->me())
          { if (!*td->schemaname) get_table_schema_name(cdp, td->tbnum, td->schemaname);
            sprintf(names, "%s.%s", td->schemaname, td->selfname);
          }
        }
        else if (tp==EKT_PROCEDURE || tp==EKT_TRIGGER)
          if (objnum==NOOBJECT)  // local routine
            strcpy(names, "local routine");
          else
          { WBUUID uuid;
            tb_read_atr(cdp, objtab_descr, objnum, APPL_ID_ATR, (tptr)uuid);
            ker_apl_id2name(cdp, uuid, names, NULL);
            strcat(names, ".");
            tb_read_atr(cdp, objtab_descr, objnum, OBJ_NAME_ATR, names+strlen(names));
          }
        write_string_outpar(cdp, fex->arg7, names);
      }
      break;
    }  
    case FCNUM_GETSRVERRCNTTX:
    { t_exkont_info * peki, eki;  bool def=true;
      if (a1.is_null() || a1.intval<0)
        def=false;
      else if (cdp->handler_rscope && cdp->exkont_info)  // in handler use the snapshot
        if (a1.intval < cdp->exkont_len)
          peki=cdp->exkont_info+a1.intval;
        else def=false;  
      else if (a1.intval < cdp->execution_kontext->count())  // current context
      { t_exkont * exkont = cdp->execution_kontext;
        while (a1.intval--) exkont=exkont->_next();
        exkont->get_descr(cdp, eki.text, sizeof(eki.text));
        peki=&eki;
      }
      else def=false;
      if (!def)
        res->set_null();
      else  
      { char * text = peki->text;
        res->allocate((int)strlen(text));
        strcpy(res->valptr(), text);
        res->length=(int)strlen(text);  // not null
      }
      break;
    }  
    case FCNUM_SCHEMA_ID:
      res->length=UUID_SIZE;
      if (a1.is_null() || !*a1.valptr() || !sys_stricmp(cdp->sel_appl_name, a1.valptr())) 
        memcpy(res->strval, cdp->top_appl_uuid, UUID_SIZE);
      else if (ker_apl_name2id(cdp, a1.valptr(), (uns8*)res->strval, NULL)) 
        { memset(res->strval, 0, UUID_SIZE);  res->length=0; }  // is_null is set below
      break;
    case FCNUM_LOCAL_SCHEMA:
      if (ker_apl_id2name(cdp, cdp->current_schema_uuid, res->strval, NULL))
        res->strval[0]=0;
      break;
    case FCNUM_LOCK_BY_KEY:  // function returns true if locked
    { tobjnum objnum;
      if (find_obj(cdp, a1.valptr(), CATEG_TABLE, &objnum))  // accepts schema name 
      { res->intval=FALSE;  
        SET_ERR_MSG(cdp, a1.valptr());  request_error(cdp, OBJECT_DOES_NOT_EXIST);  // the error may have been handled 
        break; 
      }
      a2.load(cdp);
      if (a3.is_null()) a3.intval=0;
      { table_descr_auto tbdf(cdp, objnum);
        if (tbdf->me()==NULL) res->intval = FALSE;
        else                  
        { trecnum rec = find_record_by_primary_key(cdp, tbdf->me(), a2.valptr());
          if (rec==NORECNUM) res->intval = FALSE;
          else res->intval = !wait_record_lock(cdp, tbdf->me(), rec, TMPWLOCK, a3.intval);
        }
      }
      break;
    }
    case FCNUM_ENABLE_INDEX:
    { tobjnum objnum;
      if (find_obj(cdp, a1.valptr(), CATEG_TABLE, &objnum))  // accepts schema name (since WB 6.0c)
      { SET_ERR_MSG(cdp, a1.valptr());  request_error(cdp, OBJECT_DOES_NOT_EXIST);  // the error may have been handled 
        break; 
      }
      table_descr_auto tbdf(cdp, objnum);
      if (tbdf->me()==NULL) break;
      enable_index(cdp, tbdf->me(), a2.is_null() ? -1 : a2.intval, a3.is_null() ? false : a3.intval!=0);
      break;
    }

    case FCNUM_REGEXPR:
    { t_expr_funct_regex * rex = (t_expr_funct_regex *)fex;
      if (a1.load(cdp) || a2.load(cdp) || fex->arg3 && a3.load(cdp))
        { res->set_null();  break; }  // database error
      if (!rex->compile_pattern(cdp, a2.valptr(), fex->arg3!=NULL ? a3.valptr() : ""))
        { res->set_null();  break; }  // compilation error, request_error called unless reusing the same pattern
#if WBVERS>=950
      int ovector[3];
      res->intval = pcre_exec(rex->code, NULL, a1.valptr(), (int)strlen(a1.valptr()), 0, 0, ovector, sizeof(ovector)/sizeof(int)) >= 0;
      //if (res->intval>=1) res->intval=ovector[0];  // index of the 1st character of the pattern in the text
#endif
      break;
    }
    case FCNUM_PROF_ALL:
      if (!cdp->prvs.is_conf_admin)
        { request_error(cdp, NO_CONF_RIGHT);  break; }
      if (!a1.is_null() && a1.intval)
      { if (profiling || prof_init())  // prof_init() clears the profile, must not be called if not necessary
          profiling_all=true;
      }
      else
        profiling_all=false;
      break;
    case FCNUM_PROF_THR:
    { cdp_t cdx;
      ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
      if (a1.is_null() || a1.intval==-1)
        cdx=cdp;
      else
      { cdx=NULL;
        for (int i=1;  i<=max_task_index;  i++)   /* system process 0 not counted */
          if (cds[i] && cds[i]->session_number == a1.intval)
            { cdx=cds[i];  break; }
        if (!cdx) break;
      }
      if (cdx!=cdp || !profiling)  // unless profiling started and starting/stopping profiling itself
        if (!cdp->prvs.is_conf_admin)
          goto pr_th_err;  // leaving the CS
      if (!a2.is_null() && a2.intval)
      { if (profiling || prof_init())  // prof_init() clears the profile, must not be called if not necessary
          cdx->profiling_thread=true;
      }
      else
        cdx->profiling_thread=false;
      break;
    }
    pr_th_err:  // outside the CS
      request_error(cdp, NO_CONF_RIGHT);  break;
    case FCNUM_PROF_RESET:
      if (!cdp->prvs.is_conf_admin)
        { request_error(cdp, NO_CONF_RIGHT);  break; }
      prof_init();  break;
    case FCNUM_PROF_LINES:
      if (!cdp->prvs.is_conf_admin)
        { request_error(cdp, NO_CONF_RIGHT);  break; }
      profiling_lines = !a1.is_null() && a1.intval;
      break;
    case FCNUM_SET_TH_NAME:
    { const char * p = a1.is_null() ? NULLSTRING : a1.valptr();
      while (*p=='@') p++;  // names starting with @ are used for threads started by the planner only
      strmaxcpy(cdp->thread_name, a1.is_null() ? NULLSTRING : a1.valptr(), sizeof(cdp->thread_name));
      break;
    }  
    case FCNUM_MESSAGE_TO_CLI:
      if (!a1.is_null())
        MessageToClients(cdp, a1.valptr());
      break;
    case FCNUM_CHECK_FT:
    { t_ft_kontext * ftk = get_fulltext_kontext(cdp, a1.valptr(), NULL, NULL);
      if (ftk) ftk->check(cdp);
      break;
    }
    case FCNUM_FT_LOCKING:
    { t_ft_kontext * ftk = get_fulltext_kontext(cdp, a1.valptr(), NULL, NULL);
      if (ftk) ftk->locking(cdp, a2.is_true(), a3.is_true());
      break;
    }
    case FCNUM_CRE_SYSEXT:
      if (a1.is_null()) break;
      if (!stricmp(a1.valptr(), "_TIMERTAB")) create_timer_table(cdp);
      break;
    case FCNUM_BACKUP_FIL:  
      if (!cdp->prvs.is_conf_admin)
        { res->intval=NO_CONF_RIGHT;  request_error(cdp, NO_CONF_RIGHT);  break; }
      a1.load(cdp);
      res->intval=backup_entry(cdp, a1.valptr());
      break;
    case FCNUM_REPLICATE:
    { t_repl_param param;
      tobjnum serverobj = find2_object(cdp, CATEG_SERVER, NULL, a1.valptr());
      if (serverobj==NOOBJECT)
        { res->intval=0;  break; }
      { table_descr_auto srvtab_descr(cdp, SRV_TABLENUM);
        tb_read_atr(cdp, srvtab_descr.tbdf(), serverobj, SRV_ATR_UUID, (tptr)param.server_id);
      }  
      if (ker_apl_name2id(cdp, a2.valptr(), param.appl_id, NULL)) 
        { res->intval=0;  break; }
      param.token_req=wbfalse;  param.spectab[0]=0;  param.len16=0;
      repl_operation(cdp, a3.intval ? PKTTP_REPL_REQ : PKTTP_REPL_DATA, sizeof(param), (char *)&param);
      res->intval=1;
      break;
    }  
    case FCNUM_FATAL_ERR:
      if (!cdp->prvs.is_conf_admin)
        { request_error(cdp, NO_CONF_RIGHT);  break; }
      fatal_error(cdp, "_SQP_FATAL_ERROR called");
      break;
    case FCNUM_INTERNAL:
      res->intval=0;  
      if (!cdp->prvs.is_conf_admin)
        { request_error(cdp, NO_CONF_RIGHT);  break; }
      if (a1.intval==1)
        dump_frames(cdp);
      break;
    case FCNUM_CHECK_INDEX:
    { tobjnum tabnum=NOOBJECT;  WBUUID schema_uuid;  uns8 * schema;
     // find the schema:
      if (a1.is_null() || *a1.valptr()==0) schema = NULL;
      else
      { if (ker_apl_name2id(cdp, a1.valptr(), schema_uuid, NULL)) 
          { res->intval=-2;  break; }
        schema = schema_uuid;
       // find the table if schema is specified:
        if (a2.is_null() || *a2.valptr()==0) tabnum=NOOBJECT;
        else
        { tabnum=find2_object(cdp, CATEG_TABLE, schema_uuid, a2.valptr());
          if (tabnum == NOOBJECT)
            { res->intval=-2;  break; }
        }
      }
      res->intval=check_index_set(cdp, schema, tabnum, a3.is_null() ? -1 : a3.intval);
      break;
    }  
    case FCNUM_DB_INT:
    { di_params di;  uns32 val1, val2, val3, val4, val5;
      di.lost_blocks   =&val1;
      di.lost_dheap    =&val2;
      di.nonex_blocks  =&val3;
      di.cross_link    =&val4;
      di.damaged_tabdef=&val5;
      if (!integrity_check_planning.try_starting_check(a1.is_true(), cdp!=cds[0]))
        res->intval = -1;
      else  
      { res->intval = !db_integrity(cdp, FALSE, &di);
        integrity_check_planning.check_completed();
        write_int_val(cdp, fex->arg2, val1);
        write_int_val(cdp, fex->arg3, val2);
        write_int_val(cdp, fex->arg4, val3);
        write_int_val(cdp, fex->arg5, val4);
        write_int_val(cdp, fex->arg6, val5);
        if (!res->intval) request_error(cdp, 0);  // removes REQUEST_BREAKED error
      }  
      break;
    }  
    case FCNUM_BCK:
      res->intval=_sqp_backup(cdp, a1.valptr(), a2.is_true(), a3.intval);  break;
    case FCNUM_BCK_GET_PATT:
    { char pattern[MAX_PATH];
      _sqp_backup_get_pathname_pattern(a1.valptr(), pattern);
      res->allocate((int)strlen(pattern));
      strcpy(res->valptr(), pattern);
      break;
    }   
    case FCNUM_BCK_REDUCE:
      if (a1.is_null() || a2.is_null() || !*a1.valptr())
        res->intval=ERROR_IN_FUNCTION_ARG;
      else  
        res->intval=_sqp_backup_reduce(a1.valptr(), a2.intval);  
      break;
    case FCNUM_BCK_ZIP:
      res->intval=_sqp_backup_zip(a1.valptr(), a2.is_null() ? NULL : a2.valptr(), a3.is_null() ? NULL : a3.valptr());  break;
    case FCNUM_CONN_DISK:
      res->intval=_sqp_connect_disk(a1.valptr(), a2.valptr(), a3.valptr());  break;
    case FCNUM_DISCONN_DISK:
      _sqp_disconnect_disk(a1.valptr());   break;
    case FCNUM_EXTLOB_FILE:
    { char pathname[MAX_PATH] = "";
      if (a1.vmode==mode_access) 
      { table_descr * tbdf = tables[a1.dbacc.tb];
        if (tbdf==(table_descr*)-1)  // locked, moving data in ALTER TABLE
          if (cdp->kont_tbdf && cdp->kont_tbdf->tbnum==a1.dbacc.tb)
            tbdf=(table_descr*)cdp->kont_tbdf;
        uns64 lob_id;    
        fast_table_read(cdp, tbdf, a1.dbacc.rec, a1.dbacc.attr, &lob_id);    
        if (lob_id)
          lob_id2pathname(lob_id, pathname, false);
      }
      if (res->allocate((int)strlen(pathname))) break; // sets simple_not_null or indirect
      strcpy(res->valptr(), pathname);
      break;
    }
    default:  /* not implemented on server or bad procnum */
      if ((fex->num>=157 && fex->num<=177) || fex->num == FCNUM_MBCRETBL || fex->num == FCNUM_LETTERCREW || fex->num == FCNUM_INITWBMAILEX ||
          (fex->num >= FCNUM_MBGETMSGEX && fex->num <= FCNUM_MBSAVETODBS) || (fex->num >= FCNUM_MCREATEPROF && fex->num <= FCNUM_MGETPROF) || (fex->num >= FCNUM_LETTERADDADRW && fex->num <= FCNUM_MLADDBLOBSW))
        res->intval=CallMailFunction(cdp, fex, a1, a2, a3, a4, a5, a6, a7, a8);
      else 
      { res->set_null();  return; }
  } // switch
  if (fex->num >= 70)  // original WinBase standard functions
  { if (!fex->origtype ||  // should be always function
        IS_NULL(res->valptr(), fex->origtype, fex->origspecif)) res->set_null();
    else  // calc. string length (binary length is already defined)
      if (is_char_string(fex->origtype))  
        if (fex->origspecif.wide_char) res->length=2*(int)wuclen(res->wchptr());
        else                           res->length=  (int)strlen(res->valptr());
  }
} // function_call

static void general_attr_value(cdp_t cdp, t_expr_pointed_attr * poatr, t_value * res)
{ t_expr_attr * exat = poatr->pointer_attr;
  if (exat->sqe==SQE_POINTED_ATTR)  // recurse it
    general_attr_value(cdp, (t_expr_pointed_attr*)exat, res);
  else
    attr_value(cdp, exat->kont, exat->elemnum, res, exat->origtype, exat->origspecif, exat->indexexpr);
 // read the value of the pointed column:
  if (res->intval==-1)  // NIL pointer
    res->set_null();
  else
  { table_descr_auto tbdf(cdp, poatr->desttb);
    if (tbdf->me())
      tb_attr_value(cdp, tbdf->me(), res->intval, poatr->elemnum, poatr->origtype, poatr->origspecif, poatr->indexexpr, res);
  }
}

bool scale_integer_info(sig32 & intval, int scale_disp, int * alignment)
{ while (scale_disp > 0)
  { if (intval > 214748364 || intval < - 214748364) return false;
    intval *= 10;  
    scale_disp--;
  }
  while (scale_disp < 0)
  { int rem = intval % 10;
    intval /= 10;
    if (scale_disp==-1)  
    { if (rem)
        if (rem>=5)
          if (intval > 0)
          { *alignment=1;  intval++; }
          else
          { *alignment=-1; intval--; }
        else
          if (intval > 0)
            *alignment=-1;
          else
            *alignment=1; 
    }
    else 
      if (rem) *alignment=intval>0 ? -1 : 1;
    scale_disp++;
  }
  return true;
}

bool scale_int64_info(sig64 & intval, int scale_disp, int * alignment)
{ while (scale_disp > 0)
  { if (intval > MAXBIGINT/10 || intval < -MAXBIGINT/10) return false;
    intval *= 10;  
    scale_disp--;
  }
  while (scale_disp < 0)
  { int rem = (int)(intval % 10);
    intval /= 10;
    if (scale_disp==-1)  
    { if (rem)
        if (rem>=5)
          if (intval > 0)
          { *alignment=1;  intval++; }
          else
          { *alignment=-1;  intval--; }
        else
          if (intval > 0)
            *alignment=-1;
          else
            *alignment=1; 
    }
    else 
      if (rem) *alignment=intval>0 ? -1 : 1;
    scale_disp++;
  }
  return true;
}

bool str2numeric(tptr ptr, sig64 * i64val, int scale)
{ while (*ptr==' ') ptr++;
  sig64 val = 0;
  bool minus=false,  anydig=false;
  if (*ptr=='-') { minus=true;  ptr++; }
  else if (*ptr=='+') ptr++;
  while (*ptr==' ') ptr++;
  while (*ptr>='0' && *ptr<='9')
  { anydig=true;
    if (val > MAXBIGINT / 10) return false;
    val = 10*(val)+(*ptr-'0');
    ptr++;
  }
  if (*ptr=='.')
  { *ptr++;
    while (*ptr>='0' && *ptr<='9')
    { anydig=true;
      if (scale > 0)
      { if (val > MAXBIGINT / 10 - (*ptr-'0')) return false;
        val = 10*(val)+(*ptr-'0');
      }
      else if (scale==0)
        if (*ptr>='5') val++;
      ptr++;  scale--;
    }
  }
  while (scale>0)
  { if (val > MAXBIGINT / 10) return false;
    val = 10*(val);  scale--; 
  }
  *i64val = minus ? - val : val;
  while (*ptr==' ') ptr++;
  return anydig && !*ptr;
}

void numeric2str(sig64 i64val, char * buf, int scale_disp)
{ while (scale_disp>0)
   { i64val *= 10;  scale_disp--; }
  if (i64val<0) 
    { *(buf++)='-';  i64val=-i64val; }
  char auxs[22];  int i=0;
  while (i64val!=0)
  { auxs[i++] = (char)((int)(i64val % 10) + '0');
    i64val /= 10;
  }
  scale_disp=-scale_disp; // now scale_disp is the number of digits after the decimal point
 // add leading zeros:
  while (i<=scale_disp)
    auxs[i++] = '0';
 // revert the digit order and add the decimal separator:
  while (i>0)
  { *(buf++)=auxs[--i];
    if (i==scale_disp && scale_disp) *(buf++)='.';
  }
  *buf=0;
}

#define NUM2HEX(b) ((b)<10 ? '0'+(b) : 'A'+(b)-10)

BOOL read_sql92_date(const char ** pstr, uns32 * dt)
{ uns32 year, month, day;
  if (!str2uns(pstr, &year) || year>9999) return FALSE;
  if (**pstr!='-') return FALSE;
  (*pstr)++;
  if (!str2uns(pstr, &month) || !month || month>12) return FALSE;
  if (**pstr!='-') return FALSE;
  (*pstr)++;
  if (!str2uns(pstr, &day) || !day || day>31) return FALSE;
  *dt=Make_date(day, month, year);
  return TRUE;
}

BOOL read_sql92_time(const char ** pstr, uns32 * tm)
{ uns32 hours, minutes, seconds, sec1000 = 0;
  if (!str2uns(pstr, &hours) || hours>23) return FALSE;
  if (**pstr!=':') return FALSE;
  (*pstr)++;
  if (!str2uns(pstr, &minutes) || minutes>59) return FALSE;
  if (**pstr!=':') seconds=0;
  else
  { (*pstr)++;
    if (!str2uns(pstr, &seconds) || seconds>59) return FALSE;
    if (**pstr=='.')   // fraction specified
    { int count=0;  char c;
      do
      { (*pstr)++;
        c=**pstr;
        if (c<'0' || c>'9') break;
        if (count<3) sec1000=10*sec1000+(c-'0');
        count++;
      } while (TRUE);
      while (count++<3) sec1000=10*sec1000;
      while (**pstr==' ') (*pstr)++;
    }
  }
  *tm=Make_time(hours, minutes, seconds, sec1000);
  return TRUE;
}

bool sql92_str2date(const char * str, uns32 * dt)
{ if (!read_sql92_date(&str, dt)) return false;
  return !*str;
}

BOOL sql92_str2time(const char * str, uns32 * tm, bool UTC, int default_tzd)
{ if (!read_sql92_time(&str, tm)) return FALSE;
  int displ;
  if (read_displ(&str, &displ)) 
  { *tm=displace_time(*tm, -displ);  // -> UTC
    if (!UTC) *tm=displace_time(*tm,  default_tzd);  // UTC -> ZULU
  }
  else 
    if ( UTC) *tm=displace_time(*tm, -default_tzd);  // ZULU -> UTC
  return !*str;
}

BOOL sql92_str2timestamp(const char * str, uns32 * ts, bool UTC, int default_tzd)
{ uns32 dt, tm;
  if (!read_sql92_date(&str, &dt)) return FALSE;
  if (!read_sql92_time(&str, &tm)) return FALSE;
  *ts = datetime2timestamp(dt, tm);
  int displ;
  if (read_displ(&str, &displ)) 
  { *ts=displace_timestamp(*ts, -displ);  // -> UTC
    if (!UTC) *ts=displace_timestamp(*ts,  default_tzd);  // UTC -> ZULU
  }
  else 
    if ( UTC) *ts=displace_timestamp(*ts, -default_tzd);  // ZULU -> UTC
  return !*str;
}

bool s2w(cdp_t cdp, t_value * val, wbcharset_t charset)
{ wuchar wval[MAX_DIRECT_VAL/2+1], * wptr;
  if (val->length <= MAX_DIRECT_VAL/2) wptr=wval;
  else
  { wptr=(wuchar*)val->malloc(2*(val->length+1));
    if (!wptr) 
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return false; }
  }
  if (conv1to2(val->valptr(), val->length, wptr, charset, 0))
  {// handling the conversion error
    if (wptr!=wval) val->free(wptr);
    val->set_null();
    ((wuchar*)val->strval)[0]=0;
    val->length=0;
    tobjname buf;
    strcpy(buf, charset);
    strcat(buf, "->UCS-2");
    SET_ERR_MSG(cdp, buf);
    return !request_error(cdp, STRING_CONV_NOT_AVAIL);  // may be handled
  }
  val->length = 2*(int)wuclen(wptr);  // new length in bytes
  if (wptr==wval) 
  { val->set_simple_not_null();
    memcpy(val->strval, wval, val->length+2);  // space is MAX_DIRECT_VAL+2
  }
  else
  { val->set_null();
    val->vmode=mode_indirect;
    val->indir.val=(tptr)wptr;
    val->indir.vspace=val->length;  // bytes
  }
  return true;
}

bool w2s(cdp_t cdp, t_value * val, wbcharset_t charset)
// Returns false on unhandled error. For UTF-8 length estimate is 2 bytes per character.
{ char sval[MAX_DIRECT_VAL+1], * ptr;  
  if (val->length <= MAX_DIRECT_VAL) ptr=sval;
  else
  { ptr=(char*)val->malloc(val->length+1);
    if (!ptr) 
      { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return false; }
  }
  if (conv2to1(val->wchptr(), val->length/2, ptr, charset, 0))
  {// handling the conversion error
    if (ptr!=sval) val->free(ptr);
    val->set_null();
    val->strval[0]=0;
    val->length=0;
    tobjname buf;
    strcpy(buf, "UCS-2->");
    strcat(buf, charset);
    SET_ERR_MSG(cdp, buf);
    return !request_error(cdp, STRING_CONV_NOT_AVAIL);  // may be handled
  }  
  val->length=(int)strlen(ptr);  
  if (ptr==sval) 
  { val->set_simple_not_null();
    memcpy(val->strval, ptr, val->length+1);
  }
  else
  { val->set_null();
    val->vmode=mode_indirect;
    val->indir.val=ptr;
    val->indir.vspace=val->length;
  }
  return true;
}

bool s2s(cdp_t cdp, t_value * val, wbcharset_t src_charset, wbcharset_t dest_charset)
// Length is supposed to stay unchanged.
#define PART_SIZE 30
{ if (src_charset.is_ansi() || dest_charset.is_ansi())
    return true;  // for compatibility with ols appls: national strings are sometimes moved into ascii vars and back.
  if (dest_charset.get_code()==src_charset.get_code())
    return true;  // no conversion
  if (dest_charset.get_code()==CHARSET_NUM_UTF8)  // must reallocate, not done here
    return s2w(cdp, val, src_charset) && w2s(cdp, val, dest_charset);
#ifdef WINS
  wuchar buf[PART_SIZE];
  int rest = val->length;
  int offset = 0;
  while (rest)
  { int partsize = rest>PART_SIZE ? PART_SIZE : rest;
    if (!MultiByteToWideChar(src_charset.cp(),  0, val->valptr()+offset, partsize, buf, partsize))
      goto err_conv;  // returned 0 is always an error because converting non-zero number of characters
    if (!WideCharToMultiByte(dest_charset.cp(), 0, buf, partsize, val->valptr()+offset, partsize, NULL, NULL))
      goto err_conv;  // returned 0 is always an error because converting non-zero number of characters
    rest-=partsize;  offset+=partsize;
  }
#else // direct conversion
  iconv_t conv=iconv_open(dest_charset, src_charset);
  if (conv==(iconv_t)-1) goto err_conv;
  char buf[PART_SIZE];
  int rest;  rest = val->length;
  int offset;  offset = 0;
  while (rest)
  { int partsize = rest>PART_SIZE ? PART_SIZE : rest;
    size_t fromlen = partsize;
    size_t tolen   = partsize;
    char * from = val->valptr() + offset;
    char * to   = buf;
    while (fromlen>0)
    { int nconv=iconv(conv, &from, &fromlen, &to, &tolen);
      if (nconv==(size_t)-1)
        { iconv_close(conv);  goto err_conv; }
    }
    memcpy(val->valptr() + offset, buf, partsize);
    rest-=partsize;  offset+=partsize;
  }
  iconv_close(conv);
#endif
  return true;
 err_conv:  // handling the conversion error
  val->set_null();
  val->strval[0]=0;
  val->length=0;
  tobjname xbuf;
  strcpy(xbuf, src_charset);
  strcat(xbuf, "->");
  strcat(xbuf, dest_charset);
  SET_ERR_MSG(cdp, xbuf);
  return !request_error(cdp, STRING_CONV_NOT_AVAIL);  // may be handled
}

bool whitespace_string(const char * p) // returns TRUE iff the [p] string contains only whitespace characters
{ while (*p && *p<=' ') p++;
  return *p==0;
}

void get_limits(cdp_t cdp, t_query_expr * qe, trecnum & limit_count, trecnum & limit_offset)
{ limit_count=NORECNUM;  limit_offset=0;
#if SQL3
#if LIMITS_JOINED   // no more joining, no effect, may collide with ORDER
// [qe] may be the top qe of the t_qe_specif chain. Calculates the [limit_count] and [limit_offset] using MIN and SUM respectively.
  do
  { if (qe->limit_count)
    { t_value res;
      expr_evaluate(cdp, qe->limit_count, &res);  // returns integer result
      if (!res.is_null()) 
      { if (res.intval<0) res.intval=0;
        if (res.intval<limit_count) limit_count=res.intval;
      }
    }
    if (qe->limit_offset)
    { t_value res;
      expr_evaluate(cdp, qe->limit_offset, &res);  // returns integer result
      if (!res.is_null() && res.intval>0)
        limit_offset+=res.intval;
    }
    if (qe->qe_type==QE_SPECIF && ((t_qe_specif*)qe)->grouping_type==GRP_NO) 
      qe = ((t_qe_specif*)qe)->from_tables;
    else break;
  } while (true);
#else  // !LIMITS_JOINED
#ifdef STOP  // must not include lower levels
 // find qe with LIMIT:
  while (!qe->limit_count && !qe->limit_offset)
    if (qe->qe_type==QE_SPECIF && ((t_qe_specif*)qe)->grouping_type==GRP_NO) 
      qe = ((t_qe_specif*)qe)->from_tables;
    else break;
#endif
 // calculate limits:
  if (qe->limit_count)
  { t_value res;
    expr_evaluate(cdp, qe->limit_count, &res);  // returns integer result
    if (!res.is_null()) 
    { if (res.intval<0) res.intval=0;
      if (res.intval<limit_count) limit_count=res.intval;
    }
  }
  if (qe->limit_offset)
  { t_value res;
    expr_evaluate(cdp, qe->limit_offset, &res);  // returns integer result
    if (!res.is_null() && res.intval>0)
      limit_offset+=res.intval;
  }
#endif // !LIMITS_JOINED
#else  // !SQL3
  if (qe->limit_count)
  { t_value res;
    expr_evaluate(cdp, qe->limit_count, &res);  // returns integer result
    if (res.is_null()) limit_count=NORECNUM; 
    else if (res.intval<0) limit_count=0;
    else limit_count=res.intval;
  }
  if (qe->limit_offset)
  { t_value res;
    expr_evaluate(cdp, qe->limit_offset, &res);  // returns integer result
    if (res.is_null() || res.intval<0) limit_offset=0;
    else limit_offset=res.intval;
  }
#endif
}

void t_expr_subquery::close_data(cdp_t cdp)
{ 
  if (opt) opt->close_data(cdp); 
  no_value_in_cache();  // prevent using the materialisation which does not exist
}

bool t_expr_subquery::have_value_in_cache(cdp_t cdp) const
{ return cachable && cached_value_client==cdp->applnum_perm && cached_value_stmt_counter==cdp->stmt_counter && 
         cdp->current_cursor && cached_value_cursor_inst_num == cdp->current_cursor->global_cursor_inst_num &&
         same_position(); 
}

void t_expr_subquery::value_saved_to_cache(cdp_t cdp)
{ cached_value_client=cdp->applnum_perm;  cached_value_stmt_counter=cdp->stmt_counter;  
  cached_value_cursor_inst_num = cdp->current_cursor ? cdp->current_cursor->global_cursor_inst_num : 0; 
}

void expr_evaluate(cdp_t cdp, t_express * ex, t_value * res)
// Evaluating expression, COMMIT and ROLLBACK are blocked.
{ bool saved_in_expr_eval;
  saved_in_expr_eval=cdp->in_expr_eval;
  cdp->in_expr_eval=true;
  stmt_expr_evaluate(cdp, ex, res);
  cdp->in_expr_eval=saved_in_expr_eval;
}

void stmt_expr_evaluate(cdp_t cdp, t_express * ex, t_value * res)
// Version of expr_evaluate() for use in safe environment, when no page is fixed. COMMIT and ROLLBACK are not blocked.
{ 
  switch (ex->sqe)
  { case SQE_CURRREC:
    { t_expr_currrec * crex = (t_expr_currrec*)ex;
      ttablenum tabnum;  tattrib attrnum;  trecnum recnum;  elem_descr * attracc;
      if (!crex->kont) recnum=NORECNUM;
      else attr_access(crex->kont, 1, tabnum, attrnum, recnum, attracc);
      res->set_simple_not_null();  res->intval=recnum;
      break;
    }
    case SQE_SEQ:
    { t_expr_seq * seqex = (t_expr_seq *)ex;  t_seq_value val;
      if (get_own_sequence_value(cdp, seqex->seq_obj, seqex->nextval, &val)) break; 
      res->set_simple_not_null();  res->i64val=val;
      break;
    }
    case SQE_SCONST: // not used for Char, strings, binary, var-len 
    { t_expr_sconst * scex = (t_expr_sconst*)ex;
      switch (scex->origtype)
      { case ATT_INT8:
          res->intval=(sig32)*(sig8 *)scex->val;  break;
        case ATT_INT16:
          res->intval=(sig32)*(sig16*)scex->val;  break;
        case ATT_INT32:
          res->intval=       *(sig32*)scex->val;  break;
        case ATT_INT64:
          res->i64val=       *(sig64*)scex->val;  break;
        case ATT_MONEY:
          memcpy(res->strval, scex->val, MONEY_SIZE);
          Fil2Mem_convert(res, res->strval, ATT_MONEY, 2);  break;
        case ATT_BOOLEAN:
          res->intval=(sig32)*(uns8 *)scex->val;  break;
        default:
          memcpy(res->strval, scex->val, tpsize[scex->origtype]);
      }
      if (scex->const_is_null) res->set_null();
      else res->set_simple_not_null();
      break;
    }
    case SQE_LCONST: // used for Char, strings, binary, var-len. Not UNICODE. 
    { t_expr_lconst * lcex = (t_expr_lconst*)ex;
      if (!lcex->valsize) 
      { res->set_null(); // this is necessary when comparing "" with database attribute
        res->strval[0]=0;  // function parametrs use this;
        res->length=0;
      }
      else
      { res->allocate(lcex->valsize);
        memcpy(res->valptr(), lcex->lval, lcex->valsize+2);
        res->length=lcex->valsize;  // not null
      }
      break;
    }
    case SQE_TERNARY:
    { t_value val1, val2;
      t_expr_ternary * ternex =(t_expr_ternary *)ex;
      expr_evaluate(cdp, ternex->op1, &val1);
      if (val1.load(cdp)) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
      t_compoper oper=ternex->oper;
     // optimizer causes that the value of oper.ignore_case is changed to false, lines below help :-(
      char msg[30];
      sprintf(msg, "%u, %u", oper.opq, oper.ignore_case);
      if (oper.pure_oper==PUOP_LIKE)
      { BOOL is_like;  char esc=0;  wuchar wesc = 0;  // escape-char is supposed to be ASCII
        if (ternex->op3!=NULL)
        { expr_evaluate(cdp, ternex->op3, &val2);
          val2.load(cdp);
          if (val2.is_null() || val2.length!=1) sql_exception(cdp, SQ_INVALID_ESCAPE_CHAR); // may be handled
          else wesc=esc=val2.valptr()[0];  
          val2.set_null();
        }
        expr_evaluate(cdp, ternex->op2, &val2);
        if (val2.load(cdp)) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
        if (oper.wide_char)
          is_like=wlike_esc(val1.wchptr(), val2.wchptr(), wesc, oper.ignore_case, oper.collation);  
        else
          is_like=like_esc (val1.valptr(), val2.valptr(), esc,  oper.ignore_case, oper.collation);  
        if (oper.negate) is_like = !is_like;
        res->intval = is_like;
        res->set_simple_not_null();
      }
      else if (oper.pure_oper==PUOP_BETWEEN)  // must propagate string info, too ###
      { res->set_simple_not_null();  // preset
        expr_evaluate(cdp, ternex->op2, &val2);
        t_compoper oper2;
        if (oper.negate)
        { oper2.set_num(PUOP_GT, oper.arg_type);
          binary_oper(cdp, &val2, &val1, oper2); // ops reversed
          if (val2.is_null()) { res->set_null();  break; }
          if (val2.intval) { res->intval=TRUE;  break; }
          expr_evaluate(cdp, ternex->op3, &val2);
          oper2.set_num(PUOP_LT, oper.arg_type);
          binary_oper(cdp, &val2, &val1, oper2); // ops reversed
        }
        else
        { oper2.set_num(PUOP_LE, oper.arg_type);
          binary_oper(cdp, &val2, &val1, oper2); // ops reversed
          if (val2.is_null()) { res->set_null();  break; }
          if (!val2.intval) { res->intval=FALSE;  break; }
          expr_evaluate(cdp, ternex->op3, &val2);
          oper2.set_num(PUOP_GE, oper.arg_type);
          binary_oper(cdp, &val2, &val1, oper2); // ops reversed
        }
        if (val2.is_null()) { res->set_null();  break; }
        res->intval=val2.intval;
      }
      break;
    }
    case SQE_BINARY:
    { t_value val1, val2;  t_expr_binary * bex = (t_expr_binary*)ex;
      if (bex->oper.all_disp || bex->oper.any_disp)  // value to list operation, IN is converted to "= ANY", NOT IN to "<> ALL"
      { t_compoper oper = bex->oper;  BOOL all;
        if (oper.all_disp) all=TRUE;  else all=FALSE;
        symmetric_oper(oper);
        expr_evaluate(cdp, bex->op1, &val1);
        res->set_simple_not_null();  // preset
        BOOL was_unknown = FALSE;
        if (bex->op2->sqe==SQE_BINARY)  // IN list
        { t_expr_binary * aex = (t_expr_binary*)bex->op2;
          do
          { expr_evaluate(cdp, aex->op1, &val2);
            binary_oper(cdp, &val2, &val1, oper);
            if (val2.is_null()) was_unknown=TRUE;
            else if (all ? !val2.intval : val2.intval) 
              { res->intval=val2.intval;  break; }
            aex=(t_expr_binary*)aex->op2;
            if (aex==NULL)
            { if (was_unknown) res->set_null(); else res->intval=all;
              break;
            }
          } while (TRUE);
        }
        else  // subquery
        { t_expr_subquery * suex = (t_expr_subquery*)bex->op2;
          subquery_locker sul(suex);
          bool eof=false;
          t_optim * sopt = suex->opt;
#ifdef OPTIM_IN  // SQL3 version
          t_fictive_current_cursor fcc(cdp);  // used when called without enclosing cursor
          if (suex->cachable)
          {// prepare materialization:
            suex->qe->mat_extent = suex->qe->get_pmat_extent();
            bool passing_opened;

            if (!suex->have_value_in_cache(cdp))   // 1st pass, different position, different client or different statement
            { sopt->close_data(cdp);
              (sopt->*sopt->p_reset)(cdp);  
              if (suex->qe->kont.p_mat==NULL) create_or_clear_pmat(cdp, suex->qe);
              else sopt->eof=TRUE;  // prevents adding records from pmat to the same pmat again
              passing_opened = true;
              suex->qe->kont.p_mat->completed = FALSE;
              suex->evaluated_count++;  
            }
            else
            { passing_opened = false;
              suex->qe->kont.p_mat->completed = TRUE;
            }
            trecnum rec = 0;
            do
            {// get the new record from the subquery:
              int ordnum = 0;
              if (rec < suex->qe->kont.p_mat->rec_cnt)
#ifdef UNSTABLE_PMAT
                suex->qe->set_position_indep(cdp, &rec, ordnum);
#else
                set_pmat_position(cdp, suex->qe, rec, suex->qe->kont.p_mat);
#endif
              else if (sopt->eof)
              { if (was_unknown) res->set_null(); else res->intval=all;
                break;
              }
              else
              { if (!passing_opened)
                { passing_open_close(cdp, sopt, TRUE);
                  passing_opened=true;
                  suex->qe->kont.p_mat->completed = FALSE;
                }
                if (!(sopt->*sopt->p_get_next_record)(cdp))
                { if (was_unknown) res->set_null(); else res->intval=all;
                  break;
                }
                trecnum * recs = suex->qe->kont.p_mat->recnums_space(cdp);
                if (recs!=NULL) suex->qe->get_position_indep(recs, ordnum);
              }
              rec++;  
             // subquery record positioned, process it:
              elem_descr * el = suex->qe->kont.elems.acc0(1);
              attr_value(cdp, &suex->qe->kont, 1, &val2, el->type, el->specif, NULL);
              binary_oper(cdp, &val2, &val1, oper);
              if (val2.is_null()) was_unknown=TRUE;
              else if (all ? !val2.intval : val2.intval) 
                { res->intval=val2.intval;  break; }
            } while (!sopt->error);
            if (!cdp->current_cursor->fictive_cursor)  // I am inside a cursor construction, cache the open subcursor
            { if (passing_opened)
                passing_open_close(cdp, sopt, FALSE);
              suex->value_saved_to_cache(cdp);  // cursor remains open
            }
            else // evaluating an expression, do not cache the subcursor, data_not_closed() called otherwise
              sopt->close_data(cdp);  // calls no_value_in_cache();
          }
          else  // !cachable
          { (sopt->*sopt->p_reset)(cdp);  
            do
            { if (eof || !(sopt->*sopt->p_get_next_record)(cdp))
              { if (was_unknown) res->set_null(); else res->intval=all;
                break;
              }
              elem_descr * el = suex->qe->kont.elems.acc0(1);
              attr_value(cdp, &suex->qe->kont, 1, &val2, el->type, el->specif, NULL);
              binary_oper(cdp, &val2, &val1, oper);
              if (val2.is_null()) was_unknown=TRUE;
              else if (all ? !val2.intval : val2.intval) 
                { res->intval=val2.intval;  break; }
            } while (!sopt->error);
            sopt->close_data(cdp);
            suex->evaluated_count++;  
          }
          suex->used_count++;
#else  // ! newOPTIM_IN

#if !SQL3
          trecnum limit_count, limit_offset;  
          get_limits(cdp, suex->qe, limit_count, limit_offset);
#endif
#ifdef OPTIM_IN  // data not closed on exit, allows repass
          if (!suex->have_value_in_cache(cdp)) suex->opt->close_data(cdp);
#endif
          suex->opt->reset(cdp);  
#if !SQL3
          while (limit_offset)
            if (!suex->opt->get_next_record(cdp)) { eof=true;  break; }
            else limit_offset--;
          do
          { if (eof || !suex->opt->get_next_record(cdp) || !limit_count--)
#else
          do
          { if (eof || !suex->opt->get_next_record(cdp))
#endif
            { if (was_unknown) res->set_null(); else res->intval=all;
              break;
            }
            elem_descr * el = suex->qe->kont.elems.acc0(1);
            attr_value(cdp, &suex->qe->kont, 1, &val2, el->type, el->specif, NULL);
            binary_oper(cdp, &val2, &val1, oper);
            if (val2.is_null()) was_unknown=TRUE;
            else if (all ? !val2.intval : val2.intval) 
              { res->intval=val2.intval;  break; }
          } while (!suex->opt->error);
#ifdef OPTIM_IN    
          suex->value_saved_to_cache(cdp);  // cursor remains open
#else
          suex->opt->close_data(cdp);
#endif
          suex->evaluated_count++;  suex->used_count++;
#endif  // ! newOPTIM_IN
        }
      }
      else  // normal binary
      { expr_evaluate(cdp, bex->op1, res);
        if (bex->oper.pure_oper==PUOP_AND) // lazy eval
        {// any is FALSE -> result is FALSE
          if (res->is_false()) break;
          expr_evaluate(cdp, bex->op2, &val2);
          if (val2.is_false())
          { res->set_simple_not_null();  res->intval=FALSE; } // val1 is not null here!
          else if (res->is_null() || val2.is_null())
            res->set_null();
          else // both are TRUE -> TRUE
            res->intval=TRUE;  // val1 is not null here!
        }
        else if (bex->oper.pure_oper==PUOP_OR)
        { if (res->is_true()) break;
          expr_evaluate(cdp, bex->op2, &val2);
          if (val2.is_true())
            { res->set_simple_not_null();  res->intval=TRUE; }
          else if (res->is_null() || val2.is_null())
              res->set_null();
          else  // both are FALSE -> FALSE
              res->intval=FALSE; // val1 is not null here!
        }
        else
        { expr_evaluate(cdp, bex->op2, &val2);
          binary_oper(cdp, res, &val2, bex->oper);
        }
      }
      break;
    }
    case SQE_UNARY:
    { t_expr_unary * unex =(t_expr_unary *)ex;
      switch (unex->oper)
      { case UNOP_INEG: 
          expr_evaluate(cdp, unex->op, res);
          res->intval =-res->intval;   break;
        case UNOP_FNEG: 
          expr_evaluate(cdp, unex->op, res);
          res->realval=-res->realval;  break;
        case UNOP_MNEG: 
          expr_evaluate(cdp, unex->op, res);
          res->i64val=-res->i64val;  break;

//        case UNOP_IS_TRUE   :
//          expr_evaluate(cdp, unex->op, res);
//          res->intval=res->is_true();   
//          res->set_simple_not_null();  break;
//        case UNOP_IS_FALSE  :                                                     
//          expr_evaluate(cdp, unex->op, res);
//          res->intval=res->is_false();  
//          res->set_simple_not_null();  break;
//        case UNOP_IS_UNKNOWN:
//          expr_evaluate(cdp, unex->op, res);
//          res->intval=res->is_null();                  
//          res->set_simple_not_null();  break;

        case UNOP_EXISTS:
        { t_expr_subquery * suex = (t_expr_subquery*)unex->op;
          subquery_locker sul(suex);
          bool eof=false;
          t_fictive_current_cursor fcc(cdp);  // used when called without enclosing cursor
#if SQL3
          suex->opt->reset(cdp);
          res->intval=!eof && suex->opt->get_next_record(cdp);
#else
          trecnum limit_count, limit_offset;  
          get_limits(cdp, suex->qe, limit_count, limit_offset);
          suex->opt->reset(cdp);
          while (limit_offset)
            if (!suex->opt->get_next_record(cdp)) { eof=true;  break; }
            else limit_offset--;
          res->intval=!eof && limit_count!=0 && suex->opt->get_next_record(cdp);
#endif
          suex->opt->close_data(cdp);
          suex->evaluated_count++;  suex->used_count++;
          res->set_simple_not_null();
          break;
        }
        case UNOP_UNIQUE:
        { t_expr_subquery * suex = (t_expr_subquery*)unex->op;
          subquery_locker sul(suex);
          res->intval=TRUE;  // unique so far
          res->set_simple_not_null();
          bool eof=false;
          t_fictive_current_cursor fcc(cdp);  // used when called without enclosing cursor
#if SQL3
          suex->opt->reset(cdp);
          suex->rowdist->reset_index(cdp);
          if (!eof)
            while (suex->opt->get_next_record(cdp) && !suex->opt->error)
              if (!suex->rowdist->add_to_rowdist(cdp))
                { res->intval=FALSE;  break; }
#else
          trecnum limit_count, limit_offset;  
          get_limits(cdp, suex->qe, limit_count, limit_offset);
          suex->opt->reset(cdp);
          suex->rowdist->reset_index(cdp);
          while (limit_offset)
            if (!suex->opt->get_next_record(cdp)) { eof=true;  break; }
            else limit_offset--;
          if (!eof)
            while (limit_count-- && suex->opt->get_next_record(cdp) && !suex->opt->error)
              if (!suex->rowdist->add_to_rowdist(cdp))
                { res->intval=FALSE;  break; }
#endif
          suex->rowdist->close_index(cdp);
          suex->opt->close_data(cdp);
          suex->evaluated_count++;  suex->used_count++;
          break;
        }
      }
      break;
    }
    case SQE_AGGR:
    { t_expr_aggr * agex =(t_expr_aggr *)ex;
#if SQL3
      t_mater_table * mat = agex->kont->t_mat;
#else
      t_mater_table * mat = (t_mater_table*)agex->kont->mat;
#endif
      if (!mat) { res->set_null();  break; } // by OUTER JOIN above, materialisation not started
      trecnum matrec = /*cdp->kont_recnum;*/agex->kont->position;
      if (matrec==-2) { res->set_null();  break; } // by OUTER JOIN
      tb_attr_value(cdp, tables[mat->tabnum], matrec, mat->aggr_base+1+agex->agnum, agex->restype, agex->arg==NULL ? 0 : agex->arg->specif, NULL, res);
      if (agex->ag_type==S_AVG)
      { sig32 count;  // must use sig32, uns32 -> unsigned division
        tb_read_atr(cdp, tables[mat->tabnum], matrec, mat->aggr_base+1+agex->agnum2, (tptr)&count);  // temp. table -> tabdescr locked
        if (!count) res->set_null();
        else
          switch (agex->arg->type)
          { case ATT_INT32:  case ATT_INT16:  case ATT_INT8:
              res->intval  /= count;  break;
            case ATT_INT64:  case ATT_MONEY:
              res->i64val  /= count;  break;
            case ATT_FLOAT:
              res->realval /= count;  break;
            case ATT_DATE:  case ATT_TIME:  case ATT_TIMESTAMP:  case ATT_DATIM:
              res->unsval  /= count;  break;
          }
      }
      break;
    }
    case SQE_FUNCT:
      function_call(cdp, (t_expr_funct *)ex, res);
      break;
    case SQE_ATTR:
    { t_expr_attr * exat = (t_expr_attr *)ex;
      attr_value(cdp, exat->kont, exat->elemnum, res, exat->origtype, exat->origspecif, exat->indexexpr);
      break;
    }
    case SQE_ATTR_REF:
    { t_expr_attr * exat = (t_expr_attr *)ex;  elem_descr * attracc;
      res->vmode=mode_access;  
      if (!attr_access(exat->kont, exat->elemnum, res->dbacc.tb, res->dbacc.attr, res->dbacc.rec, attracc) || attracc)
        res->vmode=mode_null;
      break;
    }
    case SQE_POINTED_ATTR:
      general_attr_value(cdp, (t_expr_pointed_attr*)ex, res);
      break;
    case SQE_DYNPAR:
      get_dp_value(cdp, ((t_expr_dynpar*)ex)->num, res, ((t_expr_dynpar*)ex)->origtype, ((t_expr_dynpar*)ex)->origspecif);
      break;
    case SQE_EMBED: // get the host variable value
    { res->set_null();  // if not found or empty var-len value
      t_emparam * empar = cdp->sel_stmt->find_eparam_by_name(((t_expr_embed*)ex)->name);
      if (empar!=NULL)
        if (empar->length) // NULL value otherwise
          if (res->allocate(empar->length)) request_error(cdp, OUT_OF_KERNEL_MEMORY);
          else
          { tptr destptr = res->valptr();
            memcpy(destptr, empar->val, empar->length);
            if (IS_HEAP_TYPE(empar->type) || IS_EXTLOB_TYPE(empar->type))
            { res->length=empar->length;
              if (empar->type==ATT_TEXT || empar->type==ATT_EXTCLOB) 
                destptr[empar->length]=destptr[empar->length+1]=0;
            }
            else
            { if (empar->type==ATT_STRING) destptr[empar->length]=destptr[empar->length+1]=0;
              Fil2Mem_convert(res, destptr, empar->type, empar->specif);  
            }
          }
      break;
    }

    case SQE_NULL:
      res->set_null();  break;
    case SQE_CAST:
    { t_expr_cast * cex = (t_expr_cast*)ex;
      expr_evaluate(cdp, cex->arg, res);
      break;
    }
    case SQE_CASE:
    { t_expr_case * cex = (t_expr_case *)ex;  t_value val1, val2;
      switch (cex->case_type)
      { case CASE_NULLIF:
          expr_evaluate(cdp, cex->val1, res);
          expr_evaluate(cdp, cex->val2, &val2);
          binary_oper(cdp, &val2, res, cex->oper); // reversed order!
          if (val2.is_true())  // equal
            { res->set_null(); } // result is NULL if equal
          break;
        case CASE_COALESCE:
          expr_evaluate(cdp, cex->val1, res);
          if (res->is_null()) expr_evaluate(cdp, cex->val2, res);
          break;
        case CASE_SRCH:
          do
          { expr_evaluate(cdp, cex->val1, &val1);
            if (val1.is_true())
              { expr_evaluate(cdp, cex->val2, res);  break; }
            if (cex->contin==NULL) // implicit ELSE - return NULL
              { res->set_null();  break; }
            if (cex->contin->sqe!=SQE_CASE)  // ELSE brach
              { expr_evaluate(cdp, cex->contin, res);  break; }
            cex = (t_expr_case *)cex->contin;
          } while (TRUE);
          break;
        case CASE_SMPL1:
        { expr_evaluate(cdp, cex->val1, &val1);  // compared value
          t_compoper oper=cex->oper;  // operator stored in the top node only
          cex = (t_expr_case *)cex->contin;
          while (cex!=NULL && cex->sqe==SQE_CASE && cex->case_type==CASE_SMPL2)
          { expr_evaluate(cdp, cex->val1, &val2);     // label value
            binary_oper(cdp, &val2, &val1, oper);  // compare
            if (val2.is_true())
            { expr_evaluate(cdp, cex->val2, res);     // result value
              goto convert_and_exit;
            }
            cex=(t_expr_case *)cex->contin;
          }
          if (cex!=NULL)  // ELSE brach present
            expr_evaluate(cdp, cex, res);
          else res->set_null();  // implicit ELSE - return NULL
          break;
        }
      }
      break;
    }
    case SQE_MULT_COUNT:
    { t_expr_mult_count * mex = (t_expr_mult_count*)ex;
      ttablenum tabnum;  tattrib attrnum;  trecnum recnum;  elem_descr * attracc;
      attr_access(mex->attrex->kont, mex->attrex->elemnum, tabnum, attrnum, recnum, attracc);
      if (attracc!=NULL)  // Impossible, I hope
        { res->set_null();  break; }
      t_mult_size num;
      if (tb_read_ind_num(cdp, tables[tabnum], recnum, attrnum, &num))
        { res->set_null();  break; }
      res->intval=num;  res->set_simple_not_null();
      break;
    }
    case SQE_VAR_LEN:  // may use attr_value instead
    { t_expr_var_len * vex = (t_expr_var_len*)ex;
      table_descr * tbdf;  tattrib attrnum;  trecnum recnum;  
      if (vex->attrex->kont==NULL)  // "current" table kontext
      { tbdf=(table_descr*)cdp->kont_tbdf;
        recnum=cdp->kont_recnum;  attrnum=vex->attrex->elemnum;
      }
      else
      { ttablenum tabnum;  elem_descr * attracc;
        attr_access(vex->attrex->kont, vex->attrex->elemnum, tabnum, attrnum, recnum, attracc);
        if (attracc!=NULL)  // Impossible, I hope
          { res->set_null();  break; }
        tbdf=tables[tabnum];
      }
      if (vex->attrex->indexexpr!=NULL)
      { expr_evaluate(cdp, vex->attrex->indexexpr, res);
        if (tb_read_ind_len(cdp, tbdf, recnum, attrnum, (uns16)res->intval, (uns32*)&res->intval))
          { res->set_null();  break; }
      }
      else
        if (tb_read_atr_len(cdp, tbdf, recnum, attrnum, (uns32*)&res->intval))
          { res->set_null();  break; }
      res->set_simple_not_null();
      break;
    }
    case SQE_DBLIND:
    { t_expr_dblind * vex = (t_expr_dblind*)ex;
      ttablenum tabnum;  tattrib attrnum;  trecnum recnum;  elem_descr * attracc;
      attr_access(vex->attrex->kont, vex->attrex->elemnum, tabnum, attrnum, recnum, attracc);
      if (attracc!=NULL)  // Impossible, I hope
        { res->set_null();  break; }
      uns32 start, size;  uns16 indxval;
      expr_evaluate(cdp, vex->start, res);
      if (res->is_null() || res->intval<0)
        { res->set_null();  break; }
      start=res->intval;
      expr_evaluate(cdp, vex->size, res);
      if (res->is_null() || res->intval<0)
        { res->set_null();  break; }
      size=res->intval;
      if (vex->attrex->indexexpr!=NULL)
      { expr_evaluate(cdp, vex->attrex->indexexpr, res);
        if (res->is_null() || res->intval<0 || res->intval>0xffff)
        { res->set_null();  break; }
        indxval=(uns16)res->intval;
      }
      if (res->allocate(size)) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
      tptr destptr = res->valptr();
      if (vex->attrex->indexexpr!=NULL)
      { if (tb_read_ind_var(cdp, tables[tabnum], recnum, attrnum, indxval, start, size, destptr))
          { res->set_null();  break; }
      }
      else
        if (tb_read_var(cdp, tables[tabnum], recnum, attrnum, start, size, destptr))
          { res->set_null();  break; }
      res->length=size=cdp->tb_read_var_result;
      destptr[size]=destptr[size+1]=0;
      break;
    }
    case SQE_SUBQ:
    { t_expr_subquery * suex = (t_expr_subquery*)ex;
      subquery_locker sul(suex);
     // check if outer ref position changed:
      if (suex->have_value_in_cache(cdp))
        *res=suex->val;
      else  // calculate the value, then store to cache
      { t_fictive_current_cursor fcc(cdp);  // used when called without enclosing cursor
        t_optim * sopt = suex->opt;
        (sopt->*sopt->p_reset)(cdp);
#if SQL3  // LIMIT, ORDER handled inside
        if (!((sopt->*sopt->p_get_next_record)(cdp))) 
        { res->set_null();  
          if (suex->cachable)
            { suex->val.set_null();  suex->value_saved_to_cache(cdp); }
        }
        else
        { elem_descr * el = suex->qe->kont.elems.acc0(1);
          attr_value(cdp, &suex->qe->kont, 1, res, el->type, el->specif, NULL);
          res->load(cdp);  // must be done before suex->opt->close_data(cdp) which unlocks the table descr!
          if ((sopt->*sopt->p_get_next_record)(cdp))
            sql_exception(cdp, SQ_CARDINALITY_VIOLATION);  // may be handled
          else  // do not cache on error  
          { if (res->is_null()) 
              qlset_null(res, ex->type, ex->specif.length);  // used in fuction calls
            if (suex->cachable)
              { suex->val=*res;  suex->value_saved_to_cache(cdp); }
          }    
        }
#else
        trecnum limit_count, limit_offset;  bool eof=false;
        get_limits(cdp, suex->qe, limit_count, limit_offset);
        while (limit_offset)
          if (!sopt->*sopt->get_next_record(cdp)) { eof=true;  break; }
          else limit_offset--;
        if (eof || !limit_count || !suex->opt->get_next_record(cdp)) 
        { res->set_null();  
          if (suex->cachable)
            { suex->val.set_null();  suex->value_saved_to_cache(cdp); }
        }
        else
        { elem_descr * el = suex->qe->kont.elems.acc0(1);
          attr_value(cdp, &suex->qe->kont, 1, res, el->type, el->specif, NULL);
          res->load(cdp);  // must be done before suex->opt->close_data(cdp) which unlocks the table descr!
          if (limit_count > 1 && suex->opt->get_next_record(cdp))
            sql_exception(cdp, SQ_CARDINALITY_VIOLATION);  // may be handled
          if (res->is_null()) 
            qlset_null(res, ex->type, ex->specif.length);  // used in fuction calls
          if (suex->cachable)
            { suex->val=*res;  suex->value_saved_to_cache(cdp); }
        }
#endif
        suex->opt->close_data(cdp);
        suex->save_position();
        suex->evaluated_count++;
      }
      suex->used_count++;
      break;
    }
    case SQE_VAR: // like reading database attribute (strings have terminating 0, but integers may be short)
    { t_expr_var * varex = (t_expr_var*)ex;
      if (varex->level==SYSTEM_VAR_LEVEL) get_sys_var_value(cdp, varex->offset, res);
      else
      { tptr varbl = variable_ptr(cdp, varex->level, varex->offset);
        if (!varbl)
          { SET_ERR_MSG(cdp, "<local variable>");  request_error(cdp, OBJECT_DOES_NOT_EXIST);  return; }
        int type = varex->origtype;
        if (IS_HEAP_TYPE(type) || IS_EXTLOB_TYPE(type))
        { t_value * val = (t_value*)varbl;
          *res=*val;  // operator=
        }
        else
        { tptr destptr; 
          if (SPECIFLEN(type) && varex->origspecif.length>MAX_DIRECT_VAL)
          { if (res->allocate(varex->origspecif.length)) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }
            destptr=res->indir.val;   // can do this because size>MAX_DIRECT_VAL and allocate returned false
          }
          else { destptr=res->strval;  res->set_simple_not_null(); }
          memcpy(destptr, varbl, simple_type_size(type, varex->origspecif));
          Fil2Mem_convert(res, destptr, type, varex->origspecif);
        }
      }
      break;
    }
    case SQE_ROUTINE:
    { sql_stmt_call * stmt = (sql_stmt_call*)ex;
      stmt->result_ptr=res;
      stmt->exec(cdp); // short_term_resptr=NULL; is on the beginning of exec()
      break;
    }
  }

 convert_and_exit:
  if (ex->convert.conv!=CONV_NO)
  {// unpack the conversion: 
    t_conv_type conv = ex->convert.conv;

    if (conv>=CONV_CONV)
      if (res->is_null()) // always true for CONV_NULL
        { qlset_null(res, ex->type, ex->specif.length); // must convert because standard function suppose the NULL argument to be represented
          res->length=0;  // used when result is string
          return;   // remains NULL
        }

    switch (conv)
    { case CONV_NEGATE:
        res->intval=!res->intval;  break;

      case CONV_F2I:
       f2i:
        scale_real(res->realval, ex->convert.scale_shift);
        if (fabs(res->realval) >= 2147483647.5f)
        { if (!(cdp->sqloptions & SQLOPT_MASK_NUM_RANGE)) sql_exception(cdp, SQ_NUM_VAL_OUT_OF_RANGE);   // may be handled 
          res->set_null();  break;
        }
        else res->intval=res->realval<0.0f ? (sig32)(res->realval-0.50000000001) : (sig32)(res->realval+0.50000000001);
        goto inttest;
      case CONV_M2I:
        if (!scale_int64(res->i64val, ex->convert.scale_shift))
        { if (!(cdp->sqloptions & SQLOPT_MASK_NUM_RANGE)) sql_exception(cdp, SQ_NUM_VAL_OUT_OF_RANGE);   // may be handled 
          res->set_null();  break;
        }
       i64test: 
        if (ex->type==ATT_INT8  && (res->i64val > 0x7f       || res->i64val < -0x7f  ) ||
            ex->type==ATT_INT16 && (res->i64val > 0x7fff     || res->i64val < -0x7fff) ||
                                   (res->i64val > 0x7fffffff || res->i64val < -0x7fffffff))
        { if (!(cdp->sqloptions & SQLOPT_MASK_NUM_RANGE)) sql_exception(cdp, SQ_NUM_VAL_OUT_OF_RANGE);   // may be handled 
          res->set_null();  break;
        }
        break;
      case CONV_I2I:
        if (!scale_integer(res->intval, ex->convert.scale_shift))
        { if (!(cdp->sqloptions & SQLOPT_MASK_NUM_RANGE)) sql_exception(cdp, SQ_NUM_VAL_OUT_OF_RANGE);   // may be handled 
          res->set_null();  break;
        }
       inttest:
        if (ex->type==ATT_INT8  && (res->intval > 0x7f   || res->intval < -0x7f  ) ||
            ex->type==ATT_INT16 && (res->intval > 0x7fff || res->intval < -0x7fff))
        { if (!(cdp->sqloptions & SQLOPT_MASK_NUM_RANGE)) sql_exception(cdp, SQ_NUM_VAL_OUT_OF_RANGE);   // may be handled 
          res->set_null(); 
        }
        break;

      case CONV_F2M:
       f2m:
        scale_real(res->realval, ex->convert.scale_shift);
        if (fabs(res->realval) > MAXBIGINT)
        { if (!(cdp->sqloptions & SQLOPT_MASK_NUM_RANGE)) sql_exception(cdp, SQ_NUM_VAL_OUT_OF_RANGE);   // may be handled 
          res->set_null();  break;
        }
        res->i64val = res->realval<0.0f ? (sig64)(res->realval-0.50000000001) : (sig64)(res->realval+0.50000000001);
        break;
      case CONV_I2M:
        res->i64val = (sig64)res->intval;  
        goto i2test;
      case CONV_M2M:
       i2test:
        if (!scale_int64(res->i64val, ex->convert.scale_shift))
        { if (!(cdp->sqloptions & SQLOPT_MASK_NUM_RANGE)) sql_exception(cdp, SQ_NUM_VAL_OUT_OF_RANGE);   // may be handled 
          res->set_null();  break;
        }
        break;

      case CONV_I2F:
        res->realval=(double)res->intval;  
        scale_real(res->realval, ex->convert.scale_shift);
        break;
      case CONV_M2F:
        res->realval=(double)res->i64val;  
        // cont.
      case CONV_F2F:
        scale_real(res->realval, ex->convert.scale_shift);
        break;
      case CONV_I2BOOL:
        res->intval = res->intval!=0;  break;

      case CONV_TS2TS:
        if (ex->convert.wtz.src_wtz)  res->intval=displace_timestamp(res->intval,  cdp->tzd);  // UTC -> ZULU
        if (ex->convert.wtz.dest_wtz) res->intval=displace_timestamp(res->intval, -cdp->tzd);  // ZULU -> UTC
        break;
      case CONV_T2T:
        if (ex->convert.wtz.src_wtz)  res->intval=displace_time     (res->intval,  cdp->tzd);  // UTC -> ZULU
        if (ex->convert.wtz.dest_wtz) res->intval=displace_time     (res->intval, -cdp->tzd);  // ZULU -> UTC
        break;
      case CONV_TS2T:
        if (ex->convert.wtz.src_wtz)  res->intval=displace_timestamp(res->intval,  cdp->tzd);  // UTC -> ZULU
        res->intval=timestamp2time(res->intval);  
        if (ex->convert.wtz.dest_wtz) res->intval=displace_time     (res->intval, -cdp->tzd);  // ZULU -> UTC
        break;
      case CONV_T2TS:
        if (ex->convert.wtz.src_wtz)  res->intval=displace_time     (res->intval,  cdp->tzd);  // UTC -> ZULU
        res->intval=datetime2timestamp(Today(), res->intval);  
        if (ex->convert.wtz.dest_wtz) res->intval=displace_timestamp(res->intval, -cdp->tzd);  // ZULU -> UTC
        break;
      case CONV_TS2D:
        if (ex->convert.wtz.src_wtz)  res->intval=displace_timestamp(res->intval,  cdp->tzd);  // UTC -> ZULU
        res->intval=timestamp2date(res->intval);  break;
      case CONV_D2TS:
        res->intval=datetime2timestamp(res->intval, 0);  
        if (ex->convert.wtz.dest_wtz) res->intval=displace_timestamp(res->intval, -cdp->tzd);  // ZULU -> UTC
        break;

      case CONV_BOOL2S:  // truncation not enabled
        strcpy(res->strval, res->intval ? "TRUE" : "FALSE");
        res->length=(int)strlen(res->strval);
        if (ex->specif.wide_char)
          if (!s2w(cdp, res, charset_ansi)) break;
        if ((ex->type==ATT_STRING || ex->type==ATT_CHAR) && res->length > ex->specif.length)
        { if (!(cdp->sqloptions & SQLOPT_MASK_INV_CHAR)) sql_exception(cdp, SQ_INV_CHAR_VAL_FOR_CAST);  // may be handled
          res->length = 0;  res->set_null();
        }
        break;
      case CONV_I2S:
        numeric2str(res->intval, res->strval, ex->convert.scale_shift);  goto strres;
      case CONV_M2S:
        numeric2str(res->i64val, res->strval, ex->convert.scale_shift);  goto strres;
      case CONV_F2S:
        scale_real(res->realval, ex->convert.scale_shift);
        real2str(res->realval, res->strval, ex->specif.length < 7 ? 0 : ex->specif.length <= 30 ? ex->specif.length-7 : 30-7);  
        goto strres;
      case CONV_D2S:
        date2str(res->intval,  res->strval, 1);   goto strres;
      case CONV_T2S:
        if (ex->convert.wtz.src_wtz) // UTC -> ZULU
          timeUTC2str(res->intval, cdp->tzd, res->strval, SQL_LITERAL_PREZ);
        else
          time2str(res->intval, res->strval, SQL_LITERAL_PREZ);   
        goto strres;
      case CONV_TS2S:
        if (ex->convert.wtz.src_wtz) // UTC -> ZULU
          timestampUTC2str(res->intval, cdp->tzd, res->strval, SQL_LITERAL_PREZ);  
        else
          timestamp2str(res->intval, res->strval, SQL_LITERAL_PREZ);  
       strres:
        res->length=(int)strlen(res->strval);
       strtest:
        if (ex->specif.wide_char)
          if (!s2w(cdp, res, charset_ansi)) break;
        if ((ex->type==ATT_STRING || ex->type==ATT_CHAR) && res->length > ex->specif.length)
        { if (!(cdp->sqloptions & SQLOPT_MASK_RIGHT_TRUNC)) sql_exception(cdp, SQ_STRING_DATA_RIGHT_TRU);  // may be handled
          res->length = ex->specif.length;  res->valptr()[res->length]=res->valptr()[res->length+1]=0;
        }
        break;
      case CONV_B2S:
      { if (res->reallocate(2*res->length+1))
          { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
        int i=res->length;
        while (i)
        { unsigned byte = res->valptr()[--i];
          res->valptr()[2*i  ] = NUM2HEX(byte >> 4);
          res->valptr()[2*i+1] = NUM2HEX(byte & 0xf);
        }
        res->length *= 2;
        res->valptr()[res->length]=0;
        goto strtest;
      }
      case CONV_S2S: // text or string to string, load and check the length
        res->load(cdp);
        if (ex->specif.wide_char)
        { if (!s2w(cdp, res, ex->convert.chs.src_charset ? ex->convert.chs.src_charset : ex->convert.chs.dest_charset))
            break;
        }
        else
          if (!s2s(cdp, res, ex->convert.chs.src_charset, ex->convert.chs.dest_charset))
            break;
        if ((ex->type==ATT_STRING || ex->type==ATT_CHAR) && res->length > ex->specif.length)
        { if (!(cdp->sqloptions & SQLOPT_MASK_RIGHT_TRUNC)) sql_exception(cdp, SQ_STRING_DATA_RIGHT_TRU);  // may be handled
          res->length = ex->specif.length;    res->valptr()[res->length]=res->valptr()[res->length+1]=0;
        }
        break;
      case CONV_S2SEXPL:  // explicit conversion/truncation of a string
        res->load(cdp);
        if (ex->specif.wide_char)
        { if (!s2w(cdp, res, ex->convert.chs.src_charset ? ex->convert.chs.src_charset : ex->convert.chs.dest_charset))
            break;
        }
        else
          if (!s2s(cdp, res, ex->convert.chs.src_charset, ex->convert.chs.dest_charset))
            break;
        if ((ex->type==ATT_STRING || ex->type==ATT_CHAR) && res->length > ex->specif.length)
          { res->length = ex->specif.length;  res->valptr()[res->length]=res->valptr()[res->length+1]=0; }
        break;

      case CONV_W2S: // text or string to string, load and check the length
        res->load(cdp);
        if (!ex->specif.wide_char)  // wide to 8
          if (!w2s(cdp, res, /*ex->convert.chs.dest_charset*/ex->specif.charset))
            break;
        if ((ex->type==ATT_STRING || ex->type==ATT_CHAR) && res->length > ex->specif.length)
        { if (!(cdp->sqloptions & SQLOPT_MASK_RIGHT_TRUNC)) sql_exception(cdp, SQ_STRING_DATA_RIGHT_TRU);  // may be handled
          res->length = ex->specif.length;  res->valptr()[res->length]=res->valptr()[res->length+1]=0; 
        }
        break;
      case CONV_W2SEXPL:  // explicit truncation of a string
        res->load(cdp);
        if (!ex->specif.wide_char)  // wide to 8
          if (!w2s(cdp, res, /*ex->convert.chs.dest_charset*/ex->specif.charset))
            break; 
        if ((ex->type==ATT_STRING || ex->type==ATT_CHAR) && res->length > ex->specif.length)
          { res->length = ex->specif.length;  res->valptr()[res->length]=res->valptr()[res->length+1]=0; }
        break;

      case CONV_W2I:
        res->load(cdp);  
        if (!w2s(cdp, res, 0)) break;
        // cont.
      case CONV_S2I:  // string may contain real representation
      { sig64 i64val;  double rval;  
        res->load(cdp);  tptr ptr = res->valptr();
        if (whitespace_string(ptr))
          res->set_null();
        else if (str2numeric(ptr, &i64val, ex->convert.scale_shift))
          { res->set_simple_not_null();  res->i64val=i64val;  goto i64test; }
        else if (str2real(ptr, &rval))
          { res->set_simple_not_null();  res->realval=rval;  goto f2i; }
        else  // the string is inconvertible
        { res->set_null();  
          if (!(cdp->sqloptions & SQLOPT_MASK_INV_CHAR)) sql_exception(cdp, SQ_INV_CHAR_VAL_FOR_CAST);  // may be handled
        }
        break;
      }

      case CONV_W2M:
        res->load(cdp);  
        if (!w2s(cdp, res, 0)) break;
        // cont.
      case CONV_S2M:  // string may contain real representation
      { sig64 i64val;  double rval;  
        res->load(cdp);  tptr ptr = res->valptr();
        if (whitespace_string(ptr))
          res->set_null();
        else if (str2numeric(ptr, &i64val, ex->convert.scale_shift))
          { res->set_simple_not_null();  res->i64val=i64val; }
        else if (str2real(ptr, &rval))
          { res->set_simple_not_null();  res->realval=rval;  goto f2m; }
        else  // the string is inconvertible
        { res->set_null();  
          if (!(cdp->sqloptions & SQLOPT_MASK_INV_CHAR)) sql_exception(cdp, SQ_INV_CHAR_VAL_FOR_CAST);  // may be handled
        }
        break;
      }
      case CONV_W2F:
        res->load(cdp);  
        if (!w2s(cdp, res, 0)) break;
        // cont.
      case CONV_S2F:
      { double rval;
        res->load(cdp);  tptr ptr = res->valptr();
        if (whitespace_string(ptr))
          res->set_null();
        else if (str2real(ptr, &rval))
        { scale_real(rval, -ex->convert.scale_shift);
          res->set_simple_not_null();  res->realval=rval;
        }
        else  // the string is inconvertible
        { res->set_null();
          if (!(cdp->sqloptions & SQLOPT_MASK_INV_CHAR)) sql_exception(cdp, SQ_INV_CHAR_VAL_FOR_CAST);  // may be handled
        }
        break;
      }

      case CONV_W2BOOL:
        res->load(cdp);  
        if (!w2s(cdp, res, 0)) break;
        // cont.
      case CONV_S2BOOL:
      { res->load(cdp);  tptr ptr = res->valptr();
        cutspaces(ptr);
        if (!*ptr)
          res->set_null();
        else if (!stricmp(ptr, "TRUE"))
          { res->set_simple_not_null();  res->intval=1; }
        else if (!stricmp(ptr, "FALSE"))
          { res->set_simple_not_null();  res->intval=0; }
        else  // the string is inconvertible
        { res->set_null();  
          if (!(cdp->sqloptions & SQLOPT_MASK_INV_CHAR)) sql_exception(cdp, SQ_INV_CHAR_VAL_FOR_CAST);  // may be handled
        }
        break;
      }

      case CONV_W2D:
        res->load(cdp);  
        if (!w2s(cdp, res, 0)) break;
        // cont.
      case CONV_S2D:
      { uns32 dt;  
        res->load(cdp);  tptr ptr = res->valptr();
        if (whitespace_string(ptr))
          res->set_null();
        else if (str2date(ptr, &dt) || sql92_str2date(ptr, &dt))
          { res->set_simple_not_null();  res->intval=dt; }
        else if (str2timestampUTC(ptr, &dt, false, cdp->tzd) || sql92_str2timestamp(ptr, &dt, false, cdp->tzd))
          { res->set_simple_not_null();  res->intval=timestamp2date(dt); }
        else  // the string is inconvertible
          { res->set_null();  
            if (!(cdp->sqloptions & SQLOPT_MASK_INV_CHAR)) sql_exception(cdp, SQ_INV_CHAR_VAL_FOR_CAST);  // may be handled 
          }
        break;
      }
      case CONV_W2T:
        res->load(cdp);  
        if (!w2s(cdp, res, 0)) break;
        // cont.
      case CONV_S2T:
      { uns32 tm; 
        int default_tzd = ex->convert.wtz.dest_wtz ? cdp->tzd : NONEINTEGER;
        res->load(cdp);  tptr ptr = res->valptr();
        if (whitespace_string(ptr))
          res->set_null();
        else if (str2timeUTC(ptr, &tm, ex->convert.wtz.dest_wtz!=0, cdp->tzd) || sql92_str2time(ptr, &tm, ex->convert.wtz.dest_wtz!=0, cdp->tzd))
          { res->set_simple_not_null();  res->intval=tm; }
        else if (str2timestampUTC(ptr, &tm, ex->convert.wtz.dest_wtz!=0, cdp->tzd) || sql92_str2timestamp(ptr, &tm, ex->convert.wtz.dest_wtz!=0, cdp->tzd))
          { res->set_simple_not_null();  res->intval=timestamp2time(tm); }
        else  // the string is inconvertible
          { res->set_null();  
            if (!(cdp->sqloptions & SQLOPT_MASK_INV_CHAR)) sql_exception(cdp, SQ_INV_CHAR_VAL_FOR_CAST);  // may be handled 
          }
        break;
      }
      case CONV_W2TS:
        res->load(cdp);  
        if (!w2s(cdp, res, 0)) break;
        // cont.
      case CONV_S2TS:
      { uns32 tms;
        res->load(cdp);  tptr ptr = res->valptr();
        if (whitespace_string(ptr))
          res->set_null();
        else if (str2timestampUTC(ptr, &tms, ex->convert.wtz.dest_wtz!=0, cdp->tzd) || sql92_str2timestamp(ptr, &tms, ex->convert.wtz.dest_wtz!=0, cdp->tzd))
          { res->set_simple_not_null();  res->intval=tms; }
        else  // the string is inconvertible
        { res->set_null();  
          if (!(cdp->sqloptions & SQLOPT_MASK_INV_CHAR)) sql_exception(cdp, SQ_INV_CHAR_VAL_FOR_CAST);  // may be handled 
        }
        break;
      }
      case CONV_VAR2BIN:
      { int newlength = ex->convert.scale_shift;
        res->load(cdp);  
        if (newlength > res->length)  // allocate and add zeros
        { if (res->reallocate(newlength)) break;
          memset(res->valptr()+res->length, 0, newlength - res->length);
        }
        res->length = newlength;
        break;
      }  
    } // switch
  } // convert
}

BOOL compute_key(cdp_t cdp, const table_descr * tbdf, trecnum rec, dd_index * indx, tptr keyval, bool check_privils)
{ t_value res;
  cdp->kont_tbdf=tbdf;  cdp->kont_recnum=rec;
  uns32 saved_opts = cdp->sqloptions;  
  cdp->sqloptions &= SQLOPT_QUOTED_IDENT|SQLOPT_REPEATABLE_RETURN;  // cannot clear these option, uncompiled SQL function may be called here
  cdp->check_rights=check_privils;
  unsigned partnum;  const part_desc * part;
  for (partnum=0, part=indx->parts;  partnum<indx->partnum;  partnum++, part++)
  { expr_evaluate(cdp, (t_express *)part->expr, &res);
    if (res.is_null())
      qset_null(keyval, part->type, part->partsize);
    else
      memcpy(keyval, res.valptr(), part->partsize);
    keyval+=part->partsize;
    res.set_null();
  }
  *(trecnum*)keyval=rec;
  cdp->check_rights=true;
  cdp->sqloptions = saved_opts;
  return TRUE;
}

/////////////////////////// interval lists ///////////////////////////////////
// Empty list meads full unlimited interval.
// Empty interval reprented by interp->llim==ILIM_EMPTY.

t_icomp compare_intervals(int valtype, t_interval * op1, t_interval * op2)
// not called for empty interval
{ int llcomp, lrcomp, rlcomp, rrcomp;
  int valsize = SPECIFLEN(valtype) ? op1->valspecif.length : tpsize[valtype];
  if (op1->llim==ILIM_UNLIM)
    if (op2->llim==ILIM_UNLIM) llcomp=0;
    else llcomp=-1;
  else
    if (op2->llim==ILIM_UNLIM) llcomp=1;
    else
    { llcomp=compare_values(valtype, op1->valspecif, op1->values, op2->values);
      if (!llcomp && op1->llim!=op2->llim)
        llcomp = op1->llim==ILIM_CLOSED ? -1 : 1;
    }
  if (llcomp<0)
  {// compare RL:
    if (op1->rlim==ILIM_UNLIM || op2->llim==ILIM_UNLIM) rlcomp=1;
    else
    { rlcomp=compare_values(valtype, op1->valspecif, op1->values+valsize, op2->values);
      if (!rlcomp)
        if (op1->rlim==ILIM_CLOSED)
          { if (op2->llim==ILIM_OPEN)   rlcomp=-1; }
        else
          { /*if (op2->llim==ILIM_CLOSED)*/ rlcomp=-1; }
    }
    if (rlcomp<0) return ICOMP_1THN2;
   // compare RR:
    if (op1->rlim==ILIM_UNLIM)
      if (op2->rlim==ILIM_UNLIM) rrcomp=0;
      else rrcomp=1;
    else
      if (op2->rlim==ILIM_UNLIM) rrcomp=-1;
      else
      { rrcomp=compare_values(valtype, op1->valspecif, op1->values+valsize, op2->values+valsize);
        if (!rrcomp && op1->rlim!=op2->rlim)
          rrcomp = op1->rlim==ILIM_CLOSED ? 1 : -1;
      }
    return rrcomp>=0 ? ICOMP_1CTS2 : ICOMP_1CO2;
  }
  //if (llcomp>=0)
  {// compare LR:
    if (op1->llim==ILIM_UNLIM || op2->rlim==ILIM_UNLIM) lrcomp=-1;
    else
    { lrcomp=compare_values(valtype, op1->valspecif, op1->values, op2->values+valsize);
      if (!lrcomp)
        if (op1->llim==ILIM_CLOSED)
          { if (op2->rlim==ILIM_OPEN)   lrcomp=1; }
        else
          { /*if (op2->rlim==ILIM_CLOSED)*/ lrcomp=1; }
    }
    if (lrcomp>0) return ICOMP_2THN1;
   // compare RR:
    if (op1->rlim==ILIM_UNLIM)
      if (op2->rlim==ILIM_UNLIM) rrcomp=0;
      else rrcomp=1;
    else
      if (op2->rlim==ILIM_UNLIM) rrcomp=-1;
      else
      { rrcomp=compare_values(valtype, op1->valspecif, op1->values+valsize, op2->values+valsize);
        if (!rrcomp && op1->rlim!=op2->rlim)
          rrcomp = op1->rlim==ILIM_CLOSED ? 1 : -1;
      }
    return rrcomp<=0 ? ICOMP_2CTS1 : ICOMP_2CO1;
  }
}

void OR_intervals(int valtype, t_interval * op1, t_interval * op2, t_interval ** res)
{ t_interval ** pp, *pom;
  if      (op1 && op1->llim==ILIM_EMPTY) { *res=op2;  corefree(op1);  }
  else if (op2 && op2->llim==ILIM_EMPTY) { *res=op1;  corefree(op2);  }
  else
  { *res=op1; pp=res;
    int valsize;
    if (op2) valsize = SPECIFLEN(valtype) ? op2->valspecif.length : tpsize[valtype];
    while (op2 && *pp)  // both lists non-empty
    { t_icomp cmp = compare_intervals(valtype, *pp, op2);
      switch (cmp)
      { case ICOMP_1THN2:  // skip the interval in pp
          pp=&(*pp)->next;  break;
        case ICOMP_2THN1:  // move interval from op2 to the beginning of pp;
          pom=op2->next;  op2->next=*pp;  *pp=op2;  op2=pom;
          pp=&(*pp)->next;  break;
        case ICOMP_1CO2: // join intervals
          (*pp)->rlim=op2->rlim;
          memcpy((*pp)->values+valsize, op2->values+valsize, valsize);
          pom=op2;  op2=op2->next;  corefree(pom);  break;
        case ICOMP_2CO1: // join intervals
          (*pp)->llim=op2->llim;
          memcpy((*pp)->values, op2->values, valsize);
          pom=op2;  op2=op2->next;  corefree(pom);  break;
        case ICOMP_1CTS2: // forget op2, is already contained
          pom=op2;  op2=op2->next;  corefree(pom);  break;
        case ICOMP_2CTS1: // copy op2 over pp
          (*pp)->rlim=op2->rlim;
          (*pp)->llim=op2->llim;
          memcpy((*pp)->values, op2->values, 2*valsize);
          pom=op2;  op2=op2->next;  corefree(pom);  break;
      }
    }
    if (op2!=NULL) *pp=op2;  // add the rest of op2 to the end of result
  }
}

void AND_intervals(int valtype, t_interval * op1, t_interval * op2, t_interval ** res)
// Creates the conjunction of intervals, deletes the operands.
{ t_interval *pom, *p2;  BOOL empty=TRUE;
  *res=NULL;
  if      (op1 && op1->llim==ILIM_EMPTY)   *res=op1;
  else if (op2 && op2->llim==ILIM_EMPTY) { *res=op2;  op2=op1;  } // op1 will be deleted via op2
  else 
  { while (op1!=NULL)
    {// AND op1 interval with op2 list, must not use p2 nodes:
      int valsize;
      if (op2) valsize = SPECIFLEN(valtype) ? op2->valspecif.length : tpsize[valtype];
      p2=op2;
      while (p2!=NULL)
      { t_icomp cmp = compare_intervals(valtype, op1, p2);
        switch (cmp)
        { case ICOMP_1THN2:  // discard op1 interval, take the next
            pom=op1;  op1=op1->next;  corefree(pom);  goto next_p1;
          case ICOMP_2THN1:  // continue the search
            p2=p2->next;  break;
          case ICOMP_1CO2:   // take op1 limited by p2
            pom=op1;  op1=op1->next;
            pom->llim=p2->llim;
            memcpy(pom->values, p2->values, valsize);
            *res=pom;  res=&pom->next;  *res=NULL;  empty=FALSE;  goto next_p1;
          case ICOMP_2CO1:   // create intersect of op1 and p2 & continue
            pom=(t_interval*)corealloc(sizeof(t_interval)+2*valsize, 65);
            if (!pom) break; //error
            pom->rlim= p2->rlim;
            pom->llim=op1->llim;  pom->valspecif=p2->valspecif;
            memcpy(pom->values,         op1->values,        valsize);
            memcpy(pom->values+valsize, p2->values+valsize, valsize);
            *res=pom;  res=&pom->next;  *res=NULL;  empty=FALSE;  
            p2=p2->next;  break;
          case ICOMP_1CTS2:   // take copy of p2 & continue
            pom=(t_interval*)corealloc(sizeof(t_interval)+2*valsize, 65);
            if (!pom) break; //error
            pom->rlim=p2->rlim;
            pom->llim=p2->llim;  pom->valspecif=p2->valspecif;
            memcpy(pom->values, p2->values, 2*valsize);
            *res=pom;  res=&pom->next;  *res=NULL;  empty=FALSE;  
            p2=p2->next;  break;
          case ICOMP_2CTS1:   // take p1, go to next in p1;
            pom=op1;  op1=op1->next;
            *res=pom;  res=&pom->next;  *res=NULL;  empty=FALSE;  goto next_p1;

        } // switch
      }
      pom=op1;  op1=op1->next;  corefree(pom);
     next_p1: ;
    }
   // empty conjunction:
    if (empty) 
    { pom=(t_interval*)corealloc(sizeof(t_interval), 65);
      if (pom!=NULL) { pom->llim=ILIM_EMPTY;  pom->next=NULL; }
      *res=pom;
    }
  }
 // all op1 members used or deleted, delete op2:
  while (op2!=NULL) { pom=op2;  op2=op2->next;  corefree(pom); }
}

static void store_limit(cdp_t cdp, t_interval * interv, t_value * res, BOOL is_upper, 
      BOOL is_closed, int desttype, int restype, t_specif resspecif, int valsize, t_specif valspecif)
// Stores the value of the limit to [interv->values] and defines [rlim] or [llim] based on [is_closed] and possibe alignment.
// Converts the value: [restype]/[resspecif] describe the limit value, [desttype]/[valspecif] describe int index key part.
// The [res] value has already been converted to the common supertype.
{ t_value conv_val;  // must not store the converted value to *res, because it my be reused!
  int alignment = 0;
  t_value * convres = res;
  if (!res->is_null())
    if (desttype!=restype || !(resspecif==valspecif))
    { conv_val.set_simple_not_null();
      convres = &conv_val;
      if (desttype==ATT_INT32 || desttype==ATT_INT16 || desttype==ATT_INT8)
      { sig32 value;
        if (restype==ATT_FLOAT)
        { double rval = res->realval;
          scale_real(rval, valspecif.scale);
          value=(sig32)rval;
          if ((double)value!=rval) 
            alignment = (double)value<rval ? -1 : 1;
        }
        else if (restype==ATT_MONEY || restype==ATT_INT64)
        { sig64 i64val = res->i64val;
          scale_int64_info(i64val, valspecif.scale-resspecif.scale, &alignment);
          value=(sig32)i64val;
        }
        else
        { value=res->intval;
          scale_integer_info(value, valspecif.scale-resspecif.scale, &alignment);
        }
        conv_val.intval=value;
        if (alignment < 0)  // aligned down
          is_closed= is_upper; // upper closed, lower opened
        else if (alignment > 0)  // aligned up
          is_closed=!is_upper; // upper opened, lower closed
      }
      else if (desttype==ATT_MONEY || desttype==ATT_INT64)
      { sig64 value;
        if (restype==ATT_FLOAT)
        { double rval = res->realval;
          scale_real(rval, valspecif.scale);
          value=(sig64)rval;
          if ((double)value!=rval) 
            alignment = (double)value<rval ? -1 : 1;
        }
        else if (restype==ATT_MONEY || restype==ATT_INT64)
        { value = res->i64val;
          scale_int64_info(value, valspecif.scale-resspecif.scale, &alignment);
        }
        else
        { value = res->intval;
          scale_int64_info(value, valspecif.scale-resspecif.scale, &alignment);
        }
        conv_val.i64val=value;
        if (alignment < 0)  // aligned down
          is_closed= is_upper; // upper closed, lower opened
        else if (alignment > 0)  // aligned up
          is_closed=!is_upper; // upper opened, lower closed
      }
      else if (desttype==ATT_FLOAT)
      { double value;
        if (restype==ATT_MONEY || restype==ATT_INT64)
        { value = (double)res->i64val;
          scale_real(value, -resspecif.scale);
        }
        else if (restype!=ATT_FLOAT)
        { value = (double)res->intval;
          scale_real(value, -resspecif.scale);
        }
        conv_val.realval=value;
      }
      else if (desttype==ATT_TIMESTAMP)
      { if (restype==ATT_DATE) 
          conv_val.intval=datetime2timestamp(res->intval, is_upper ? Make_time(23,59,59,999) : 0);
        else if (restype==ATT_TIMESTAMP)  // specifs are not the same
          if (resspecif.with_time_zone) 
            conv_val.intval=displace_timestamp(res->intval,  cdp->tzd);  // UTC -> ZULU
          else
            conv_val.intval=displace_timestamp(res->intval, -cdp->tzd);  // ZULU -> UTC
      }
      else if (desttype==ATT_DATE)
      { if (restype==ATT_TIMESTAMP) conv_val.intval=timestamp2date(res->intval);
      }
      else if (desttype==ATT_TIME)
      { if (resspecif.with_time_zone) 
          conv_val.intval=displace_time(res->intval,  cdp->tzd);  // UTC -> ZULU
        else
          conv_val.intval=displace_time(res->intval, -cdp->tzd);  // ZULU -> UTC
      }
      else if (desttype==ATT_STRING && valspecif.wide_char==0)  // when valspecif.wide_char==1 then [res] has already been converted to wide
      { if (resspecif.wide_char)
        { conv_val=*res;
          if (!w2s(cdp, &conv_val, valspecif.charset))
            { strcpy(conv_val.valptr(), "\xff");  is_closed=FALSE; }  // not completely OK, but allmost
        }
        else if (resspecif.charset!=valspecif.charset)
        { conv_val=*res;
          if (!s2s(cdp, &conv_val, resspecif.charset, valspecif.charset))
            { strcpy(conv_val.valptr(), "\xff");  is_closed=FALSE; }  // not completely OK, but allmost
        }
        else
          convres = res;  // use the original value
      }
      else
        convres = res;  // use the original value
    }
 // store it:
  if (is_upper)
  { if (!convres->is_null())
      memcpy(interv->values+valsize, convres->valptr(), valsize);
    else qset_null(interv->values+valsize, desttype, valsize);
    interv->rlim = is_closed ? ILIM_CLOSED : ILIM_OPEN;
  }
  else  // lower bound
  { if (!convres->is_null())
      memcpy(interv->values, convres->valptr(), valsize);
    else qset_null(interv->values, desttype, valsize);
    interv->llim = is_closed ? ILIM_CLOSED : ILIM_OPEN;
  }
}

int goes_to_table(cdp_t cdp, db_kontext * kont, int elemnum, const db_kontext * dest_tabkont)
{ elem_descr * el = kont->elems.acc(elemnum);
#if SQL3
  t_mater_table * mat = kont->t_mat;
  if (mat!=NULL) // kontext materialized into a table
  { ttablenum tabnum = mat->tabnum;
    if (kont==dest_tabkont) return 2;
    if (tabnum==dest_tabkont->t_mat->tabnum) return 1;
#else
  t_materialization * mat = kont->mat;
  if (mat!=NULL && !mat->is_ptrs) // kontext materialized into a table
  { ttablenum tabnum=((t_mater_table*)mat)->tabnum;
    if (kont==dest_tabkont) return 2;
    if (tabnum==((t_mater_table*)dest_tabkont->mat)->tabnum) return 1;
#endif
    if (tabnum>=0) return 0;
   // continues on temp tables, external condition on the grouping result may be propagated inside and used in the internal join
  }
  switch (el->link_type)
  { case ELK_NO:
#if SQL3
      return el->dirtab.tabnum==dest_tabkont->t_mat->tabnum;
#else
      return el->dirtab.tabnum==((t_mater_table*)dest_tabkont->mat)->tabnum;
#endif
    case ELK_KONTEXT:
      return goes_to_table(cdp, el->kontext.kont, el->kontext.down_elemnum, dest_tabkont);
    case ELK_KONT2U:  case ELK_KONT21:
      return goes_to_table(cdp, el->kontdbl.kont1, el->kontdbl.down_elemnum1, dest_tabkont) ||
             goes_to_table(cdp, el->kontdbl.kont2, el->kontdbl.down_elemnum2, dest_tabkont);
    case ELK_EXPR:  case ELK_AGGREXPR:  default:
      return 0;
  }
}

static t_interval * rel2interval(cdp_t cdp, t_express * ex, int valtype, t_specif valspecif, db_kontext * tabkont, t_specif internal_specif)
// Converts relation or BETWEEN predicate [ex] into interval(s) and returns it.
// valtype and valspecif describe the properties of an index part.
{ t_interval * interv;  t_express * indexp, * valexp, * valexp2;
  t_compoper oper;  t_expr_sconst bool_const(ATT_BOOLEAN, t_specif(), "\x1", 1);
 // analyse operands:
  if (ex->sqe==SQE_BINARY)
  { t_expr_binary * bex = (t_expr_binary *)ex;
    oper = bex->oper;
   // find the value-side and index-side
    int op1isindex = 0, op2isindex = 0;
    if (bex->op1->sqe==SQE_ATTR) 
    { t_expr_attr * exat = (t_expr_attr *)bex->op1;
      db_kontext * kont = exat->kont;  
      if (kont->replacement!=NULL) kont=kont->replacement;
      op1isindex = goes_to_table(cdp, kont, exat->elemnum, tabkont);
    }
    if (op1isindex<2 && bex->op2->sqe==SQE_ATTR) 
    { t_expr_attr * exat = (t_expr_attr *)bex->op2;
      db_kontext * kont = exat->kont;  
      if (kont->replacement!=NULL) kont=kont->replacement;
      op2isindex = goes_to_table(cdp, kont, exat->elemnum, tabkont);
    }
    if (op1isindex>op2isindex)
      { indexp=bex->op1;  valexp=bex->op2; }
    else if (!op1isindex && !op2isindex &&
      (bex->op2->sqe==SQE_SCONST || bex->op2->sqe==SQE_LCONST || bex->op2->sqe==SQE_DYNPAR ||  // trying to patch a serious error
       bex->op2->sqe==SQE_EMBED || bex->op2->sqe==SQE_NULL))
      { indexp=bex->op1;  valexp=bex->op2; }
    else
    { indexp=bex->op2;  valexp=bex->op1;
      switch (oper.pure_oper)  // PUOP_PREF comes never here
      { case PUOP_GT:  oper.pure_oper=PUOP_LT;  break;
        case PUOP_GE:  oper.pure_oper=PUOP_LE;  break;
        case PUOP_LT:  oper.pure_oper=PUOP_GT;  break;
        case PUOP_LE:  oper.pure_oper=PUOP_GE;  break;
      }
    }
    valexp2=NULL;
  }
  else if (ex->sqe==SQE_ATTR)
  { indexp=ex;  oper.pure_oper=PUOP_EQ;
    valexp = &bool_const;
    if (ex->convert == CONV_NEGATE) bool_const.val[0] = 0;
    valexp2=NULL;
  }
  else // ternary: (not) between
  { t_expr_ternary * tex = (t_expr_ternary *)ex;
    oper = tex->oper;
    indexp=tex->op1;  valexp=tex->op2;  valexp2=tex->op3;
  }
 // determine space for keys (max of index type space and expression type space)
  //int maxvalsize;
  //maxvalsize=SPECIFLEN(valexp->type) ? valexp->specif : tpsize[valexp->type];
  //if (valsize > maxvalsize) maxvalsize=valsize;
 // allocate interval(s):
  int valsize = SPECIFLEN(valtype) ? valspecif.length : tpsize[valtype];
  interv = (t_interval*)corealloc(sizeof(t_interval)+2*valsize, 33);
  if (interv==NULL) return NULL;
  interv->valspecif=valspecif;  interv->next=NULL;
  interv->llim=interv->rlim=ILIM_UNLIM;
 // define interval(s):
  t_value res;
  expr_evaluate(cdp, valexp, &res);
  if (oper.pure_oper==PUOP_EQ || (oper.pure_oper==PUOP_BETWEEN || oper.pure_oper==PUOP_IS_NULL) && !oper.negate)
  { store_limit(cdp, interv, &res, FALSE, TRUE, valtype, valexp ->type, valexp ->specif, valsize, valspecif);
    if (valexp2!=NULL) expr_evaluate(cdp, valexp2, &res); // BETWEEN
    else valexp2=valexp;
    store_limit(cdp, interv, &res,  TRUE, TRUE, valtype, valexp2->type, valexp2->specif, valsize, valspecif);
  }
  else if (oper.negate && (oper.pure_oper==PUOP_BETWEEN || oper.pure_oper==PUOP_PREF) || oper.pure_oper==PUOP_NE)
  { store_limit(cdp, interv, &res,  TRUE, FALSE, valtype, valexp ->type, valexp ->specif, valsize, valspecif);
    t_interval * i2 = (t_interval*)corealloc(sizeof(t_interval)+2*valsize, 33);
    if (i2==NULL) { corefree(interv);  return NULL; }
    i2->valspecif=valspecif;  i2->next=NULL;
    i2->rlim=ILIM_UNLIM;
    if (valexp2!=NULL) expr_evaluate(cdp, valexp2, &res);
    else valexp2=valexp;
    store_limit(cdp, i2,     &res, FALSE, FALSE, valtype, valexp2->type, valexp2->specif, valsize, valspecif);   
    if (oper.pure_oper==PUOP_PREF)
      for (int i=res.is_null() ? 0 : res.length;  i<valsize;  i++) // prefix may be empty, length not defined then
        i2->values[i]=(char)0xff;
    OR_intervals(valtype, interv, i2, &interv);
  }
  else if (oper.pure_oper==PUOP_GT || oper.pure_oper==PUOP_IS_NULL && oper.negate)
    store_limit(cdp, interv, &res, FALSE, FALSE, valtype, valexp->type, valexp->specif, valsize, valspecif);
  else if (oper.pure_oper==PUOP_GE)
    store_limit(cdp, interv, &res, FALSE,  TRUE, valtype, valexp->type, valexp->specif, valsize, valspecif);
  else if (oper.pure_oper==PUOP_LT)
    store_limit(cdp, interv, &res,  TRUE, FALSE, valtype, valexp->type, valexp->specif, valsize, valspecif);
  else if (oper.pure_oper==PUOP_LE)
    store_limit(cdp, interv, &res,  TRUE,  TRUE, valtype, valexp->type, valexp->specif, valsize, valspecif);
  else if (oper.pure_oper==PUOP_PREF)
  {// when using CAST/UPPER index, must convert the limiting values to upper case:
    t_value convres, * pres;
    if (internal_specif.ignore_case && !res.is_null()) // no harm if converting for other insensitive indices
    { if (convres.allocate(res.length)) pres=&res;  // error
      else
      { pres=&convres;
        if (valexp->specif.wide_char)
          convert_to_uppercaseW(res.wchptr(), pres->wchptr(), internal_specif.charset);
        else
          convert_to_uppercaseA(res.valptr(), pres->valptr(), internal_specif.charset);
        pres->length=res.length;  
      }
    }
    else pres=&res;
    store_limit(cdp, interv, pres, FALSE,  TRUE, valtype, valexp->type, valexp->specif, valsize, valspecif);
   // create the upper limit for string with the given prefix: the prefix value followed by FFs
    //next_val(valtype, res.valptr(), res.length);
    //store_limit(cdp, interv, &res,  TRUE, FALSE, valtype, valexp->type, valsize);
    // problems with CH on the end
    store_limit(cdp, interv, pres,  TRUE, TRUE,  valtype, valexp->type, valexp->specif, valsize, valspecif);
    int preflen = pres->is_null() ? 0 : valspecif.wide_char ? wucmaxlen((wuchar*)interv->values, valsize/sizeof(wuchar)) : strmaxlen(interv->values, valsize);
    // must not use pres->length as preflen, may have been converted from Unicode
    for (int i=preflen;  i<valsize;  i++) // prefix may be empty, length not defined then
      interv->values[valsize+i]=(char)0xff;
  }
  return interv;
}

static t_interval * expr2interval(cdp_t cdp, t_express * ex, int valtype, t_specif valspecif, db_kontext * tabkont, t_specif internal_specif)
// Converts [ex] into a list of intervals and returns it.
// valtype and valspecif describe the properties of an index part.
{
  if (ex->sqe==SQE_BINARY)
  { t_expr_binary * bex = (t_expr_binary *)ex;
    if (bex->oper.pure_oper==PUOP_AND || bex->oper.pure_oper==PUOP_OR)
    { t_interval * i1 = expr2interval(cdp, bex->op1, valtype, valspecif, tabkont, internal_specif),
                 * i2 = expr2interval(cdp, bex->op2, valtype, valspecif, tabkont, internal_specif);
      if (bex->oper.pure_oper==PUOP_AND)
        AND_intervals(valtype, i1, i2, &i1);
      else
         OR_intervals(valtype, i1, i2, &i1);
      return i1;
    }
    else return rel2interval(cdp, ex, valtype, valspecif, tabkont, internal_specif);
  }
  else if (ex->sqe==SQE_TERNARY || ex->sqe==SQE_ATTR)  // BETWEEN or NOTBETWEEN or a boolean column
    return rel2interval(cdp, ex, valtype, valspecif, tabkont, internal_specif);
  else return NULL; // error
}

void dump_intervs(cdp_t cdp, t_flstr * dmp, t_interval * int_list, int valtype, t_specif valspecif, int gnr_pos, int part)
{ t_interval * interv=int_list;
  while (interv!=NULL)
  { if (interv->llim==ILIM_UNLIM) dmp->put("(*");
    else
    { if (interv->llim==ILIM_OPEN) dmp->put("(");
      else dmp->put("<");
      dump_value(cdp, dmp, valtype, valspecif, interv->values);
    }
    dmp->put(",");
    if (interv->rlim==ILIM_UNLIM) dmp->put("*)");
    else
    { int valsize = SPECIFLEN(valtype) ? interv->valspecif.length : tpsize[valtype];
      dump_value(cdp, dmp, valtype, valspecif, interv->values+valsize);
      if (interv->rlim==ILIM_OPEN) dmp->put(")");
      else dmp->put(">");
    }
    dmp->put("; ");
    interv=interv->next;
  }
}

////////////////////////////// passing records //////////////////////////////
static uns8  MAXMONEY[6]  = { 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f }; /* mult. use */
static sig64 _MAXINT64    = MAXBIGINT;
static uns8  MAXDATE[4]   = { 0xff, 0xff, 0xff, 0xff }; // DATE, TIME, TIMESTAMP

static double MAXREAL = 1.7001e308;

tptr maxval(uns8 tp)
{ switch (tp)
  { case ATT_INT8:    return (tptr)MAXMONEY+5;
    case ATT_INT16:   return (tptr)MAXMONEY+4;
    case ATT_MONEY:   return (tptr)MAXMONEY;
    case ATT_TIME:  case ATT_DATE:  case ATT_TIMESTAMP:  case ATT_DATIM:
                      return (tptr)MAXDATE;
    case ATT_INT32:   return (tptr)MAXMONEY+2;
    case ATT_INT64:   return (tptr)&_MAXINT64;
    case ATT_FLOAT:   return (tptr)&MAXREAL;
    case ATT_BOOLEAN: return (tptr)MAXMONEY+5;
    case ATT_PTR:   case ATT_BIPTR:
                      return (tptr)MAXMONEY+2;  /* 0x7fffffff */
    /* case ATT_CHAR:  case ATT_STRING: ; -- see below */
  }
  return (tptr)MAXMONEY;   /* 0xff's good for char & strings */
}

void qset_null(tptr val, int type, int size)
// Must not ve called on t_value::strval for long binaries and strings
{ if (type==ATT_BINARY) memset(val, 0, size);  
  else if (IS_STRING(type) || IS_HEAP_TYPE(type) || IS_EXTLOB_TYPE(type)) 
  { *val=0;
    if (size>1) val[1]=0;  // used by UNICODE strings
  }
  else memcpy(val, null_value(type), tpsize[type]);
}

void qlset_null(t_value * val, int type, int size)
{ if (type==ATT_BINARY) 
  { if (!val->allocate(size))
    { memset(val->valptr(), 0, size);
      val->length=size;
    }
  }
  else if (IS_STRING(type)) { val->strval[0]=val->strval[1]=0;  val->length=0; }
  else if (IS_HEAP_TYPE(type) || IS_EXTLOB_TYPE(type)) 
    { val->length=0;  val->wchptr()[0]=0; }
  else memcpy(val->strval, null_value(type), tpsize[type]);
}

void qset_max(tptr val, int type, int size)
{ if (type==ATT_BINARY || IS_STRING(type)) memset(val, 0xff, size);
  else memcpy(val, maxval(type), tpsize[type]);
}

void t_table_index_part_conds::mark_core(void)
{ if (interv_list) interv_list->mark_core();
  pconds.mark_core();
}

void t_table_index_part_conds::delete_interv_list(void)
{ while (interv_list!=NULL)
    { t_interval * i2 = interv_list->next;  corefree(interv_list);  interv_list=i2; }
}

BOOL t_table_index_conds::init1(const table_descr * td, int indexnum)
// Stores basic information in the class instance. Supposes that the constructor has been called.
{ index_number=indexnum;
  //startkey=stopkey=NULL;  -- is in the constructor
  partcnt=td->indxs[indexnum].partnum;
  part_conds=new t_table_index_part_conds[partcnt];
  return part_conds!=NULL;
}

BOOL t_table_index_conds::init2(const table_descr * td)
{ int keysize = td->indxs[index_number].keysize;
  startkey=(tptr)corealloc(keysize, 85);
  stopkey =(tptr)corealloc(keysize, 85);
  return startkey!=NULL && stopkey!=NULL;
}

t_table_index_conds::~t_table_index_conds(void)
{ if (bac.fcbn!=NOFCBNUM) data_not_closed();
  if (part_conds) delete [] part_conds;
  corefree(startkey);  corefree(stopkey);
}

void t_table_index_conds::mark_core(void)
{ if (startkey) mark(startkey);
  if (stopkey ) mark(stopkey );
  if (part_conds) 
  { mark_array(part_conds);
    for (int i=0;  i<partcnt;  i++)
      part_conds[i].mark_core();
  }
}

static t_specif get_internal_specif(const t_express * partex)
// When the index expression is casted string or CAST/UPCASE, returns the specif of the inner string and IGNORE-CASE inforation.
// Otherwise returns top specif.
{ if (partex->sqe!=SQE_CAST || partex->type!=ATT_STRING || partex->specif.charset) 
    return partex->specif;  // not CASTing to ASCII string
  const t_express * castarg = ((const t_expr_cast*)partex)->arg;
  if (castarg->type!=ATT_STRING)
    return partex->specif;  // not CASTing from string
  t_specif inner_specif;
  if (castarg->sqe==SQE_FUNCT) 
  { const t_expr_funct * fex = (const t_expr_funct *)castarg;
    if (fex->num!=FCNUM_UPPER) 
      return partex->specif;  // CAST other function
    inner_specif = fex->arg1->specif;
    inner_specif.ignore_case=1;
  }
  else
  { inner_specif = castarg->specif;
    inner_specif.ignore_case=0;
  }
  return inner_specif;
}

void t_table_index_conds::compute_interval_lists(cdp_t cdp, db_kontext * tabkont)
// Converts delimiting expressions into interval lists for each part of the index.
{ for (int part=0;  part<partcnt;  part++)
  { t_table_index_part_conds * tipc = &part_conds[part];
    t_interval * tot_int = NULL;
    for (int e=0;  e<tipc->pconds.count();  e++)
    { t_specif internal_specif = get_internal_specif(indx->parts[part].expr);
      t_interval * interv = expr2interval(cdp, (t_expr_binary*)*tipc->pconds.acc0(e), indx->parts[part].type, indx->parts[part].specif, tabkont, internal_specif);
      if (tot_int && interv)
        AND_intervals(indx->parts[part].type, interv, tot_int, &tot_int);
      else tot_int = interv;
    }
   // invert the interval order if part has DESC:
    if (indx->parts[part].desc)
    { tipc->interv_list=NULL;
      while (tot_int!=NULL)
      { t_interval * i = tot_int;  tot_int=tot_int->next;
        i->next=tipc->interv_list;  tipc->interv_list=i;
      }
    }
    else tipc->interv_list=tot_int;
  }
}   

BOOL t_table_index_conds::go_to_next_interval(void)
{// find the last part with defined intervals: 
  int part=indx->partnum-1;
  while (part_conds[part].interv_list==NULL)
    if (!part) return FALSE;  // all intervals empty
    else part--;
 // activate the next interval and release the current one:
  t_table_index_part_conds * tipc = &part_conds[part];
  t_interval * interv = tipc->interv_list;
  tipc->interv_list = interv->next;
  corefree(interv);
  return tipc->interv_list!=NULL;
}

BOOL t_table_index_conds::interval2keys(cdp_t cdp)
// Converts the current inverval into the pair of delimiting keys startkey and stopkey.
{ int part;  int offset;  const part_desc * pd = indx->parts;
  open_start=open_stop=0;
  for (part=offset=0;  part<indx->partnum;  offset+=pd->partsize, part++, pd++)
  { t_interval * interv = part_conds[part].interv_list;
    if (interv && interv->llim==ILIM_EMPTY) 
      return FALSE;  // empty interval, used in conditions like (a=4 and a=5)
    if (!pd->desc)
    { if (interv==NULL || interv->llim==ILIM_UNLIM)
        qset_null(startkey+offset, pd->type, pd->partsize);
      else
      { memcpy(startkey+offset, interv->values, pd->partsize);
        if (interv->llim==ILIM_OPEN)
          //if (indx->partnum>1)
          //  next_val(pd->type, startkey+offset, pd->partsize);
          //else
            open_start=part+1;
      }
      if (interv==NULL || interv->rlim==ILIM_UNLIM)
        qset_max(stopkey+offset, pd->type, pd->partsize);
      else
      { memcpy(stopkey+offset, interv->values+pd->partsize, pd->partsize);
        if (interv->rlim==ILIM_OPEN)
          //if (indx->partnum>1)
          //  prev_val(pd->type, stopkey +offset, pd->partsize);
          //else
            open_stop=part+1;
      }
    }
    else // DESC part
    { if (interv==NULL || interv->rlim==ILIM_UNLIM)
        qset_max(startkey+offset, pd->type, pd->partsize);
      else
      { memcpy(startkey+offset, interv->values+pd->partsize, pd->partsize);
        if (interv->rlim==ILIM_OPEN)
          //if (indx->partnum>1)
          //  prev_val(pd->type, startkey+offset, pd->partsize);
          //else
            open_start=part+1;
      }
      if (interv==NULL || interv->llim==ILIM_UNLIM)
        qset_null(stopkey+offset, pd->type, pd->partsize);
      else
      { memcpy(stopkey+offset, interv->values, pd->partsize);
        if (interv->llim==ILIM_OPEN)
          //if (indx->partnum>1)
          //  next_val(pd->type, stopkey+offset, pd->partsize);
          //else
            open_stop=part+1;
      }
    }
  }
  *(trecnum*)(startkey+offset)=0;
  *(trecnum*)(stopkey +offset)=(trecnum)-1;

 // open index passing:
  continuing_same_interval = false;
  return open_index_passing(cdp);
}

BOOL t_table_index_conds::open_index_passing(cdp_t cdp)
// Opens passing index from startkey (or the next key value, if open_start)
// Read-locks the index root iff returns TRUE, then I must call close_index_passing() soon in order to unlock it!
{ bt_result bres;
  if (!btrl.lock_and_update(cdp, indx->root))
    return FALSE;
  bres=bac.build_stack(cdp, indx->root, indx, startkey);
  if (bres==BT_ERROR) { btrl.unlock();  return FALSE; }
 // when the passing of an interval has been interruped, continuation must skip the current key
  if (continuing_same_interval) bac.skip_current=true;
  else continuing_same_interval=true;
  if (open_start)
  { char keycopy[MAX_KEY_LEN];
    memcpy(keycopy, startkey, indx->keysize);
    do
    { if (!bac.btree_step(cdp, startkey)) return FALSE;
      sig32 res=cmp_keys_partial(startkey, keycopy, indx, open_start);
      if (!HIWORD(res)) break; // keys different
    } while (true);
    open_start=0;  // may be closed and re-opened in the current position
    // ... I can overwrite this because [open_start] is defined every time the interval is evaluted
    bac.skip_current=false;  // a valid record is positioned, do not skip it!
  }
  return TRUE;
}

void t_table_index_conds::close_index_passing(cdp_t cdp)
{ bac.close(cdp); 
  btrl.unlock();
}

trecnum t_table_index_conds::next_record_from_index(cdp_t cdp)
// cdp->index_owner is supposed to be defined outside. Index passing is supposed to be opened.
{ do // cycle on disjoint intervals
  { if (bac.btree_step(cdp, startkey))
    {// check the end of the interval (current key returned in stoppoint)
      sig32 res=cmp_keys_partial(startkey, stopkey, indx, open_stop);
      if (HIWORD(res) ? !open_stop : (sig16)res <= 0) // record found
        return *(trecnum*)(startkey+indx->keysize-sizeof(trecnum));
    }
   // interval ended, close it and try next interval:
    close_index_passing(cdp);
    if (!go_to_next_interval() || !interval2keys(cdp)) // goes to the next interval, calls open_index_passing() if found
    { close_data(cdp);  // interval lists for other parts may have not been closed in go_to_next_interval()
      return NORECNUM;  
    }
  } while (true);
}

void t_optim_join_inner::variant_reset(cdp_t cdp)
{ gnr_pos = 0;
  t_optim * subopt = curr_variant->join_steps[0];
  (subopt->*subopt->p_reset)(cdp);
}

void t_optim_join_inner::reset(cdp_t cdp)
{ eof=FALSE;
  curr_variant_num=1;
  curr_variant=variants.acc0(1);
  variant_reset(cdp);
 // variant distinctor:
  if (variant_recdist!=NULL)
  { variant_recdist->clear(cdp);
    variant_recdist->define(cdp, own_qe->mat_extent ? own_qe->mat_extent : own_qe->rec_extent);
  }
}

BOOL t_optim_join_inner::get_next_record(cdp_t cdp)
{ t_optim * subopt;
  do // cycle on variants
  { do // call get_next_record on leaves
    {// look for next record on gnr_pos:
      subopt=curr_variant->join_steps[gnr_pos];
     duplic:
      if (cdp->break_request)
        { request_error(cdp, REQUEST_BREAKED);  error=TRUE;  return FALSE; }
      if ((subopt->*subopt->p_get_next_record)(cdp))
        if (gnr_pos+1==leave_count)
        { if (variant_recdist!=NULL) // store the total position
          { trecnum recs[MAX_OPT_TABLES];
#if SQL3
            int ordnum = 0;
            own_qe->get_position_indep(recs, ordnum);
#else
            own_qe->get_position_0(recs);
#endif
            if (!variant_recdist->add(cdp, recs)) goto duplic;
          }
          return TRUE;  // all leaves positioned
        }
        else            // search in the next leaf
        { subopt = curr_variant->join_steps[++gnr_pos];
          (subopt->*subopt->p_reset)(cdp);
        }
      else                       // all record passed in the leaf
        if (gnr_pos) gnr_pos--;    // next record in the previous leaf
        else break;                // all record passed in leaves
    } while (TRUE);

   // go to the next variant:
    if (++curr_variant_num >= variants.count()) break; // last variant passed
    curr_variant=variants.acc0(curr_variant_num);
    variant_reset(cdp);
  } while (TRUE);
  return FALSE; // no more records
}

void t_optim_join_inner::close_data(cdp_t cdp)
{ t_optim::close_data(cdp);
  if (variant_recdist!=NULL) variant_recdist->clear(cdp);
  for (int i=1;  i<variants.count();  i++)
  { t_variant * vart = variants.acc0(i);
    for (int j=0;  j<vart->leave_cnt;  j++) 
      vart->join_steps[j]->close_data(cdp);
  }
}

void t_optim_join_outer::reset(cdp_t cdp)
{ eof=FALSE;
  gnr_pos = 0;
  t_optim * subopt = variant.join_steps[0];
  (subopt->*subopt->p_reset)(cdp);
  right_post_phase=FALSE;  // must be set even if !right_outer
  if (right_outer)
    if (oper2->maters==MAT_NO)
    { right_storage.clear(cdp);
      if (!right_storage.define(cdp, oper2->mat_extent ? oper2->mat_extent : oper2->rec_extent)) error=TRUE;
    }
    else
      roj_rowdist->reset_index(cdp);
}

BOOL t_optim_join_outer::get_next_record(cdp_t cdp)
{ t_optim * subopt;
  if (!right_post_phase) // normal processing
  { do // cycle until outer conditions satisfied
    { do // call get_next_record on leaves
      {// look for next record on gnr_pos:
        subopt = variant.join_steps[gnr_pos];
        if (cdp->break_request)
          { request_error(cdp, REQUEST_BREAKED);  error=TRUE;  return FALSE; }
        //get_fixes(&pages,  &fixes);
        if ((subopt->*subopt->p_get_next_record)(cdp))
          if (gnr_pos==1)
          { pair_generated=TRUE;
            if (right_outer)         // store the position of the right operand
              if (oper2->maters==MAT_NO)
              { trecnum recs[MAX_OPT_TABLES];
#if SQL3
                int ordnum = 0;
                oper2->get_position_indep(recs, ordnum);
#else
                oper2->get_position_0(recs);
#endif
                right_storage.add(cdp, recs);
              }
              else
                roj_rowdist->add_to_rowdist(cdp);
            break;                   // all leaves positioned
          }
          else                       // search in the next leaf
          { subopt = variant.join_steps[gnr_pos=1];
            (subopt->*subopt->p_reset)(cdp);
            pair_generated=FALSE;
          }
        else                         // all record passed in the leaf
          if (gnr_pos)
          { gnr_pos=0;               // next record in the previous leaf
            if (left_outer && !pair_generated)
            {// set NULLs:
              oper2->set_outer_null_position();
              break;
            }
          }
          else goto exhaused;                // all record passed in leaves
      } while (TRUE);
     // check the outer conditions not propadated to leaves:
      BOOL satisfied = TRUE;
      for (int i=0;  i<inherited_conds.count();  i++)
        if (conds_outer & (1<<i))
        { t_value res;
          expr_evaluate(cdp, *inherited_conds.acc0(i), &res);
          if (!res.is_true())
            { satisfied=FALSE;  break; }
        }
      if (satisfied) return TRUE;
    } while (TRUE);
   // end of normal join, may add some records for right outer join:

   exhaused:
    if (right_outer)
    { right_post_phase=TRUE;
      oper1->set_outer_null_position();  // used during the whole right_post_phase
      rightopt->close_data(cdp);  // must remove materialisations because they are not synchronized with rightopt
      (rightopt->*rightopt->p_reset)(cdp);
    }
    else return FALSE;
  }
 // right post phase of the right outer join:
  while ((rightopt->*rightopt->p_get_next_record)(cdp))
    if (oper2->maters==MAT_NO)
    { trecnum recs[MAX_OPT_TABLES];
#if SQL3
      int ordnum = 0;
      oper2->get_position_indep(recs, ordnum);
#else
      oper2->get_position_0(recs);
#endif
      if (!right_storage.contained(cdp, recs)) return TRUE;
    }
    else
      if (roj_rowdist->add_to_rowdist(cdp)) return TRUE;
  return FALSE;
}

void t_optim_join_outer::close_data(cdp_t cdp)
{ t_optim::close_data(cdp);
  variant.join_steps[0]->close_data(cdp);
  variant.join_steps[1]->close_data(cdp);
  if (right_outer)
    if (oper2->maters==MAT_NO)
    { right_storage.clear(cdp);
      if (rightopt!=NULL) rightopt->close_data(cdp); 
    }
    else
      roj_rowdist->close_index(cdp);
}
/////////////////////////////////////////// post reset action ///////////////////////////////
static t_order_by * find_ordering(t_query_expr * qe)
// Returns the top ORDER BY from the qe.
{ t_order_by * p_order = NULL;
  t_query_expr * aqe = qe;
  do
  { if (aqe->order_by!=NULL) // the qe contains ORDER BY clause
      { p_order=aqe->order_by;  break; }
    if (aqe->qe_type!=QE_SPECIF || ((t_qe_specif*)aqe)->grouping_type!=GRP_NO) 
      break;
    aqe=((t_qe_specif*)aqe)->from_tables;
    if (aqe==NULL) break;
  } while (TRUE);
  return p_order;
}


#if 0
void t_optim::post_reset_action(cdp_t cdp)
{
  t_order_by * p_order = find_ordering(own_qe);
  bool post_sorting = p_order && p_order->post_sort;
 // evaluate the limitations:
  trecnum limit_count, limit_offset;
  get_limits(cdp, own_qe, limit_count, limit_offset);

  t_query_expr * aqe;  aqe = own_qe;  BOOL distinct=FALSE;
  while (aqe->qe_type==QE_SPECIF && ((t_qe_specif*)aqe)->grouping_type==GRP_NO)
  { if (((t_qe_specif*)aqe)->distinct) distinct=TRUE;
    aqe=((t_qe_specif*)aqe)->from_tables;
  }

 // create the cursor:
  if (optim_type==OPTIM_GROUPING)
  { 
#if 0
    BOOL upper_ptr_layer = post_sorting;  ttablenum temptab;
   // create and fill the temporary table:
    t_qe_specif * gqes = (t_qe_specif*)aqe; // OK, must be grouping qe_specif
    if (gqes->distinct) distinct=TRUE;
    if (distinct || gqes->having_expr!=NULL || inherited_conds.count())
      upper_ptr_layer=TRUE;
    (this->*p_reset)(cdp);  // creates the materialized table
    if (error) goto err;
    temptab=gqes->kont.t_mat->tabnum;

    if (upper_ptr_layer)
    // must create another materialization level
    { if (post_sorting)
      { if (sort_cursor(cdp, &p_order->order_indx, this, gqes->kont.p_mat, ((t_optim_grouping*)this)->group_count,
                        &((t_optim_grouping*)this)->qeg->kont, limit_count, limit_offset)) goto err;
      }
      else
        while ((this->*p_get_next_record)(cdp))
        // records satisfying HAVING condition & inherited conds
        { if (limit_offset) limit_offset--;
          else 
          { if (!limit_count) break;
            if (limit_count!=NORECNUM) limit_count--;
            pmat->add_current_pos(cdp);
          }
        }
    }
    else // direct use of materialized table
    { if (limit_offset || limit_count!=NORECNUM)  // delete some records
      { table_descr * tbdf = tables[temptab];
        for (trecnum r=0;  r<tbdf->Recnum();  r++)
        { uns8 del=table_record_state(cdp, tbdf, r);
          if (del==NOT_DELETED)
          { if (limit_offset) 
            { limit_offset--;
              tb_del(cdp, tbdf, r);
            }
            else if (!limit_count)
              tb_del(cdp, tbdf, r);
            else if (limit_count!=NORECNUM) limit_count--;
          }
        }
      }
    }
#endif
  }

  else if (own_qe->maters >= MAT_INSTABLE)  // create the materialization 
  { 
#if 0
    t_mater_table * mat = new t_mater_table(cdp, FALSE);
    if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
    qe->kont.mat = mat;
    if (!mat->init(cdp, qe)) goto err;
    mat->completed=FALSE;
    ttablenum temptab=mat->tabnum;  table_descr * temptab_descr = tables[temptab];
    (opt->*opt->p_reset)(cdp);
    if (post_sorting)
    { qe->kont.mat = NULL;
      if (curs_mater_sort(cdp, &p_order->order_indx, own_qe, opt, temptab, limit_count, limit_offset)) opt->error=TRUE;
      qe->kont.mat = mat;
    }
    else
    { qe->kont.mat = NULL;
      uns32 old_flags = cdp->ker_flags;
      cdp->ker_flags |= KFL_ALLOW_TRACK_WRITE;
      while (!opt->error && (opt->*opt->p_get_next_record)(cdp))
      { if (limit_offset) limit_offset--;
        else 
        { if (!limit_count) break;
          if (limit_count!=NORECNUM) limit_count--;
          trecnum tab_recnum=tb_new(cdp, temptab_descr, TRUE, NULL);
          if (tab_recnum==NORECNUM) opt->error=TRUE;
          else
          for (int i=1;  i<qe->kont.elems.count();  i++)
            if (write_elem(cdp, qe, i, temptab_descr, tab_recnum, i))
              opt->error=TRUE;
        }
      }
      cdp->ker_flags=old_flags;
      qe->kont.mat = mat;
    }
    mat->completed=TRUE;
#endif
  }                                       

  else // create the pointer-materialization in qe:
  { t_mater_ptrs * mat = new t_mater_ptrs(cdp);
    if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }
    own_qe->kont.p_mat = mat;
    if (!mat->init(own_qe)) return;
    if (post_sorting)
    { if (create_curs_sort(cdp, &p_order->order_indx, own_qe, this, limit_count, limit_offset)) return;
    }
    else
    { (this->*p_reset)(cdp);
      while (limit_offset && (this->*p_get_next_record)(cdp) && !cdp->is_an_error())
        limit_offset--;
      if (limit_count!=NORECNUM)
      { while (limit_count)
        { if ((this->*p_get_next_record)(cdp) && !cdp->is_an_error())
            mat->add_current_pos(cdp);
          else break;
          limit_count--;
        }
      }
      else ; // cursor not completed yet
      passing_open_close(cdp, this, FALSE);
    }
  }
}
#endif
////////////////////////////////// recdist ///////////////////////////////////
// Used by variants and by right outer join.

BOOL t_recdist::define(cdp_t cdp, int recnum_cntIn)
{ recnum_cnt=recnum_cntIn;
  keybuf=(trecnum*)corealloc(sizeof(trecnum)*(recnum_cnt+1), 64);
  if (keybuf==NULL) return FALSE;
  indx.partnum=0;
  if (!indx.add_index_part()) return FALSE;
  indx.partnum=1;
  indx.parts[0].partsize=sizeof(trecnum)*recnum_cnt;
  indx.parts[0].specif.opqval=sizeof(trecnum)*recnum_cnt;
  indx.parts[0].type=ATT_BINARY;
  indx.parts[0].desc=wbfalse;
  indx.parts[0].expr=NULL;
  indx.ccateg=INDEX_UNIQUE;
  indx.has_nulls=true;  // record number 0 is valid!
  indx.keysize=sizeof(trecnum)*(recnum_cnt+1);
  if (create_empty_index(cdp, indx.root, &cdp->current_cursor->translog))
    { corefree(keybuf);  return FALSE; }
  return TRUE;
}

void t_recdist::clear(cdp_t cdp)
{ drop_index_idempotent(cdp, indx.root, sizeof(tblocknum)+indx.keysize, &cdp->current_cursor->translog);
  corefree(keybuf);  keybuf=NULL;
}

BOOL t_recdist::add(cdp_t cdp, void * key)
{ memcpy(keybuf, key, sizeof(trecnum)*recnum_cnt);
  keybuf[recnum_cnt]=0;
  (*keybuf)++;  // prevents recognizing the key as NULL
  return distinctor_add(cdp, &indx, (tptr)keybuf, &cdp->current_cursor->translog);
}

BOOL t_recdist::contained(cdp_t cdp, void * key)
{ memcpy(keybuf, key, sizeof(trecnum)*recnum_cnt);
  keybuf[recnum_cnt]=0;
  (*keybuf)++;  // prevents recognizing the key as NULL
  cdp->index_owner = NOOBJECT;
  return find_key(cdp, indx.root, &indx, (tptr)keybuf, NULL, true);
}

//////////////////////////////// union //////////////////////////////////////
#define DIST_FROM_LONG 300

BOOL row_distinctor::prepare_rowdist_index(t_query_expr * qe, BOOL unique, int rec_count, BOOL add_flag)
// Expressions in the index refer to the UNION/EXCEPT/INTERSECT result.
// When evaluating them, qe->kont.position must be set properly.
{ BOOL res=TRUE;
  rowdist_index.keysize = rec_count*sizeof(trecnum);
  rowdist_index.partnum = 0;
  rowdist_limit=MAX_TABLE_COLUMNS-1;  /* number of the last column number used */
  flag_offset=0;
  for (int i=1;  i<qe->kont.elems.count();  i++)
  { const elem_descr * el = qe->kont.elems.acc0(i);
    if (!IS_HEAP_TYPE(el->type) || el->type==ATT_TEXT || el->type==ATT_NOSPEC)
      if (el->multi==1 && !IS_SYSTEM_COLUMN(el->name))  // no system attrs in distinctor!
      { unsigned sz = IS_HEAP_TYPE(el->type) ? DIST_FROM_LONG : speciflen(el->type) ? el->specif.length : tpsize[el->type];
        if (rowdist_index.keysize+sz<=MAX_KEY_LEN-1)
        { if (!rowdist_index.add_index_part()) { res=FALSE;  break; }
          t_express * ex;
          rowdist_index.parts[rowdist_index.partnum].expr=ex=new t_expr_attr(i, &qe->kont);
          if (el->type==ATT_TEXT || el->type==ATT_EXTCLOB)
          { ex->type=ATT_STRING;  
            ex->convert = get_type_cast(el->type, ATT_STRING, ex->specif, t_specif(DIST_FROM_LONG, ex->specif.charset, ex->specif.ignore_case, ex->specif.wide_char), false);
            ex->specif.length=DIST_FROM_LONG;
          }
          else if (el->type==ATT_NOSPEC || el->type==ATT_EXTBLOB)
          { ex->type=ATT_BINARY;
            ex->convert = get_type_cast(el->type, ATT_BINARY, ex->specif, t_specif(DIST_FROM_LONG), false);
            ex->specif.length=DIST_FROM_LONG;
          }
          rowdist_index.parts[rowdist_index.partnum].partsize=sz;
          rowdist_index.parts[rowdist_index.partnum].specif=ex->specif;
          rowdist_index.parts[rowdist_index.partnum].type=ex->type;
          rowdist_index.parts[rowdist_index.partnum].desc=wbfalse;
          rowdist_index.partnum++;
          rowdist_index.keysize+=sz;
          flag_offset+=sz;
        }
        else
          if (i<=rowdist_limit) rowdist_limit=i-1;
    }
  }
 // add "flag" attribute - used only by INTERSECT (record inserted
 // with wbfalse, 2nd operand changes this to wbtrue
  if (add_flag)  // INTERSECT, EXCEPT
  { if (!rowdist_index.add_index_part()) res=FALSE; 
    rowdist_index.parts[rowdist_index.partnum].partsize=1;
    rowdist_index.parts[rowdist_index.partnum].specif.set_null();
    rowdist_index.parts[rowdist_index.partnum].type=ATT_BOOLEAN;
    rowdist_index.parts[rowdist_index.partnum].desc=wbfalse;
    wbbool bval = (wbbool)(qe->qe_type==QE_INTERS ? wbfalse : wbtrue);
    t_express * ex = new t_expr_sconst(ATT_BOOLEAN, t_specif(), &bval, 1);
    rowdist_index.parts[rowdist_index.partnum].expr=ex;
    rowdist_index.partnum++;
    rowdist_index.keysize+=1;
  }
  rowdist_index.ccateg=orig_unique = unique ? INDEX_UNIQUE : INDEX_NONUNIQUE;
  rowdist_index.has_nulls=true;  // must work with NULL values too
  rowdist_index.disabled=false;
  rowkey=(tptr)corealloc(rowdist_index.keysize, 52);
  was_null=FALSE;
  return res && rowkey!=NULL;
}

void row_distinctor::reset_index(cdp_t cdp)
{ create_empty_index(cdp, rowdist_index.root, &cdp->current_cursor->translog);  //error not tested here##
  was_null=FALSE;
}

void row_distinctor::close_index(cdp_t cdp)
{ drop_index_idempotent(cdp, rowdist_index.root, sizeof(tblocknum)+rowdist_index.keysize, &cdp->current_cursor->translog);
  was_null=FALSE;
}

BOOL row_distinctor::add_to_rowdist(cdp_t cdp) // called only for UNION and roj_rowdist
{ if (!compute_key(cdp, NULL, 0, &rowdist_index, rowkey, true))
    return FALSE;
  if (is_null_key(rowkey, &rowdist_index))
    if (ignore_null_rows) return TRUE;  // OK
    else if (was_null) return FALSE;  // not distinct
    else { was_null=TRUE;  return TRUE; } // distinct
//  *(trecnum*)(rowkey+rowdist_index.keysize-sizeof(trecnum))=0;
//  must not be used for EXCEPT, INTERSECT, not necessary for UNION I hope.
  return distinctor_add(cdp, &rowdist_index, rowkey, &cdp->current_cursor->translog);
}

void row_distinctor::remove_record(cdp_t cdp)
// The index is not shared, not locking its root.
{ if (!compute_key(cdp, NULL, 0, &rowdist_index, rowkey, true))
    return;
  t_btree_acc bac;  bt_result res; 
  cdp->index_owner = NOOBJECT;
  res=bac.build_stack(cdp, rowdist_index.root, &rowdist_index, rowkey);
  if (res==BT_ERROR) return;
  BOOL fnd=bac.get_key_rec(cdp, rowkey, NULL);
  if (fnd) 
    { bac.unfix(cdp);  bac.bt_remove2(cdp, rowdist_index.root, &cdp->current_cursor->translog); }
}

void row_distinctor::confirm_record(cdp_t cdp)
// The index is not shared, not locking it.
{ if (!compute_key(cdp, NULL, 0, &rowdist_index, rowkey, true))
    return;
  t_btree_acc bac;  bt_result res; 
  rowdist_index.ccateg=INDEX_UNIQUE;
  cdp->index_owner = NOOBJECT;
  res=bac.build_stack(cdp, rowdist_index.root, &rowdist_index, rowkey);
  if (res==BT_ERROR) return;
  BOOL fnd=bac.get_key_rec(cdp, rowkey, NULL);
  rowdist_index.ccateg=orig_unique;
  if (fnd)
  { char keycopy[MAX_KEY_LEN];
    memcpy(keycopy, rowkey, rowdist_index.keysize);
    bac.unfix(cdp);  
    bac.bt_remove2(cdp, rowdist_index.root, &cdp->current_cursor->translog);
    rowkey[flag_offset]=wbtrue;  // confirmed
    bt_insert(cdp, rowkey, &rowdist_index, rowdist_index.root, NULL, &cdp->current_cursor->translog);
  }
}

void t_optim::close_data(cdp_t cdp)
{ //error=TRUE;  // prevents continuation of cursor creation when rolled back -- no, cursor cannot be reused in the procedure
  if (main_rowdist!=NULL) main_rowdist->close_index(cdp); 
#if SQL3
  if (own_qe && own_qe->kont.p_mat)  // must delete, otherwise would interfere with creation of groups (getting the position of the representant)
    { own_qe->kont.p_mat->clear(cdp);  delete own_qe->kont.p_mat;  own_qe->kont.p_mat=NULL; }
  if (own_qe && own_qe->kont.t_mat)
  { t_mater_table * tmat = own_qe->kont.t_mat;
#else
  if (own_qe && own_qe->kont.mat && !own_qe->kont.mat->is_ptrs)
  { t_mater_table * tmat = (t_mater_table*)own_qe->kont.mat;
#endif
    if (!tmat->permanent)
    { tmat->destroy_temp_materialisation(cdp);
      delete tmat;  
#if SQL3
      own_qe->kont.t_mat=NULL; 
#else
      own_qe->kont.mat=NULL; 
#endif
    }
  }
}

#if SQL3
t_mater_ptrs * create_or_clear_pmat(cdp_t cdp, t_query_expr * qe)
// Creates a new pointer materialization in [qe] or clears the existing one. Clears the [completed] flag.
// Returns the materialization or NULL on error.
{ t_mater_ptrs * mat;
  if (qe->kont.p_mat) 
  { mat=qe->kont.p_mat;
    mat->clear(cdp);
  }
  else 
  { mat = new t_mater_ptrs(cdp);
    if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
    if (!mat->init(qe)) { delete mat;  return NULL; }
    qe->kont.p_mat = mat;
  }
  mat->completed=FALSE;
  return mat;
}

void t_optim_order::close_data(cdp_t cdp)
{ t_optim::close_data(cdp);
  subopt->close_data(cdp);
}

void t_optim_limit::close_data(cdp_t cdp)
{ t_optim::close_data(cdp);
  subopt->close_data(cdp);
}

void t_optim_order::optimize(cdp_t cdp, BOOL ordering_defining_table)
{ for (int i=0;  i<inherited_conds.count();  i++)
    subopt->inherit_expr(cdp, *inherited_conds.acc0(i));
  subopt->optimize(cdp, ordering_defining_table);
}

void t_optim_limit::optimize(cdp_t cdp, BOOL ordering_defining_table)
{ for (int i=0;  i<inherited_conds.count();  i++)
    subopt->inherit_expr(cdp, *inherited_conds.acc0(i));
  subopt->optimize(cdp, ordering_defining_table);
}

void t_optim_order::reset(cdp_t cdp)
{ 
  t_order_by * p_order = find_ordering(own_qe);
  if (sort_disabled)
    t_optim_limit::reset(cdp);
  else
  { eof=FALSE;
    if (!create_or_clear_pmat(cdp, own_qe))
      { eof=TRUE;  return; }
    get_limits(cdp, own_qe, limit_count, limit_offset);
    (subopt->*subopt->p_reset)(cdp);
    if (own_qe->maters >= MAT_INSTABLE)  // create the table materialization 
    { t_mater_table * mat = new t_mater_table(cdp, FALSE);
      if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
      own_qe->kont.t_mat = mat;
      if (!mat->init(cdp, own_qe)) goto err;
      mat->completed=FALSE;
      ttablenum temptab=mat->tabnum;  //table_descr * temptab_descr = tables[temptab];
      if (curs_mater_sort(cdp, &p_order->order_indx, own_qe, subopt, temptab, limit_count, limit_offset)) 
        goto err;
      mat->completed=TRUE;
    }
    else
      if (create_curs_sort(cdp, &p_order->order_indx, own_qe, subopt, limit_count, limit_offset, own_qe->qe_type==QE_SPECIF && ((t_qe_specif*)own_qe)->distinct))
        goto err;
    own_qe->limit_result_count = own_qe->kont.p_mat->record_count();
    own_qe->kont.p_mat->completed=TRUE;
    if (own_qe->qe_type!=QE_EXCEPT && own_qe->qe_type!=QE_INTERS)  // this is a BIG problem, when EXCEPT is followed by ORDER then they share kont.position, which must keep tbhe 0 value for EXCEPT
      own_qe->kont.position=(trecnum)-1;
    own_qe->kont.pmat_pos=(trecnum)-1;
    eof=own_qe->kont.p_mat->record_count() == 0;
  }
  return;
 err:
  error=TRUE; 
  eof=TRUE;
}

void t_optim_limit::reset(cdp_t cdp)
{ eof=FALSE;
  get_limits(cdp, own_qe, limit_count, limit_offset);
  (subopt->*subopt->p_reset)(cdp);
 // prepare ptr mat
//  t_mater_ptrs * mat = new t_mater_ptrs(cdp);
//  if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  eof=TRUE;  return; }
//  own_qe->kont.mat = mat;
//  if (!mat->init(own_qe)) { eof=TRUE;  return; }
 // skip records specified by limit_offset:
  own_qe->limit_result_count=0;
  while (limit_offset && (subopt->*subopt->p_get_next_record)(cdp) && !cdp->is_an_error())
    limit_offset--;
  if (limit_offset) eof=TRUE;
}

//    { t_mater_ptrs * pmat = own_qe->kont.p_mat;
//      const trecnum * recnums = pmat->recnums_pos_const(cdp, own_qe->kont.position);
//      if (recnums==NULL) return FALSE;


BOOL t_optim_order::get_next_record(cdp_t cdp)
{ if (sort_disabled)      
    return t_optim_limit::get_next_record(cdp);
  else
  { if (eof) return FALSE;
    if (++own_qe->kont.pmat_pos < own_qe->kont.p_mat->record_count())
    { 
#ifdef UNSTABLE_PMAT
      int ordnum = 0;
      own_qe->set_position_indep(cdp, &own_qe->kont.pmat_pos, ordnum);  // translates via p_mat
#else
      set_pmat_position(cdp, own_qe, own_qe->kont.pmat_pos, own_qe->kont.p_mat);  // translates via p_mat
#endif
      return TRUE;
    }
    eof=TRUE;
    return FALSE;
  }
}

BOOL t_optim_limit::get_next_record(cdp_t cdp)
{ if (eof) return FALSE;

  if (limit_count!=NORECNUM)
  { if (limit_count)
      if ((subopt->*subopt->p_get_next_record)(cdp) && !cdp->is_an_error()) 
        { limit_count--;  own_qe->limit_result_count++;  return TRUE; }
  }
  else
    if ((subopt->*subopt->p_get_next_record)(cdp) && !cdp->is_an_error()) 
      { own_qe->limit_result_count++;  return TRUE; }
  eof=TRUE;
  return FALSE;
}

#endif

void t_optim_union_dist::reset(cdp_t cdp)
{ eof=FALSE;
  top->kont.position=0;
  (op1opt->*op1opt->p_reset)(cdp);
  top->kont.position=1;
  (op2opt->*op2opt->p_reset)(cdp);
  position_for_next_record=top->kont.position=0;
  rowdist.reset_index(cdp);
}

BOOL t_optim_union_dist::get_next_record(cdp_t cdp)
{// 1st operand:
  if (!position_for_next_record)  // still passing records from the 1st operand
  { while ((op1opt->*op1opt->p_get_next_record)(cdp))
      if (rowdist.add_to_rowdist(cdp)) return TRUE;  // distinct record found
    position_for_next_record=1;   // activate passing records from the 2nd operand
  }
 // 2nd operand:
  top->kont.position=1;  // must be written every time - may have been overwritten by reading the records already contained in the cursor
  while ((op2opt->*op2opt->p_get_next_record)(cdp))
    if (rowdist.add_to_rowdist(cdp)) return TRUE;  // distinct record found
 // both operands exhausted:
  eof=TRUE;
  rowdist.close_index(cdp);
  return FALSE;
}

void t_optim_union_dist::close_data(cdp_t cdp)
{ t_optim::close_data(cdp);
  rowdist.close_index(cdp);
  op1opt->close_data(cdp);
  op2opt->close_data(cdp);
}

void t_optim_union_all::reset(cdp_t cdp)
{ eof=FALSE;
  top->kont.position=0;
  (op1opt->*op1opt->p_reset)(cdp);
  top->kont.position=1;
  (op2opt->*op2opt->p_reset)(cdp);
  top->kont.position=position_for_next_record=0;
}

BOOL t_optim_union_all::get_next_record(cdp_t cdp)
{// 1st operand:
  if (!position_for_next_record)
  { if ((op1opt->*op1opt->p_get_next_record)(cdp)) return TRUE;
    position_for_next_record=1;
  }
 // 2nd operand:
  top->kont.position=1;  // must be written every time - may have been overwritten by reading the records already contained in the cursor
  if ((op2opt->*op2opt->p_get_next_record)(cdp)) return TRUE;
 // both operands exhausted:
  eof=TRUE;
  return FALSE;
}

void t_optim_union_all::close_data(cdp_t cdp)
{ t_optim::close_data(cdp);
  op1opt->close_data(cdp);
  op2opt->close_data(cdp);
}

// EXCEPT: do indexu se prenesou plne zaznamy z prvniho operandu, neni-li ALL
// se pritom vyradi duplicity. Zaznamy z druheho operandu se vyhledavaji a
// pokud se najdou, vyradi se z indexu. Flag se nepouziva.
// INTERSECT: prvni operand se zpracuje stejne, zazanamy z druhehe se hleaji,
// a pokuse najdou, vyradi se z indexu, flag se prepise 0->1 a zpet se zaradi.
// Pritom se zachovaji cisla zaznamu v klici.
// OBA: do klice se pridavaji entice cisel zaznamu z prvniho operandu,
// v druhem operandu se ignoruji, protoze nejsou potreba pri hledani.
// Pri prochazeni vysledku se berou pouze zaznamy, ktere maji flag==1.
// Pri ALL je index neunikatni, ale pri vyhledavani se musi nastavit
// unikatnost, aby se nehledelo na cislo zaznamu.(?)

void t_optim_exc_int::reset(cdp_t cdp)
// the index is private, not locking it
{ eof=FALSE;
  rowdist.reset_index(cdp);
  if (bac.fcbn!=NOFCBNUM) bac.close(cdp);
 // insert 1st operand:
  top->kont.position=0;
  (op1opt->*op1opt->p_reset)(cdp);
  if (rowdist.is_unique()) // add distinct records from 1st operand:
    while ((op1opt->*op1opt->p_get_next_record)(cdp))
    { if (compute_key(cdp, NULL, 0, &rowdist.rowdist_index, rowdist.rowkey, true))
      { 
#if SQL3
        int ordnum = 0;
        op1opt->own_qe->get_position_indep((trecnum *)(rowdist.rowkey+rowdist.flag_offset+1), ordnum);
#else
        op1opt->own_qe->get_position_0((trecnum *)(rowdist.rowkey+rowdist.flag_offset+1));
#endif
        distinctor_add(cdp, &rowdist.rowdist_index, rowdist.rowkey, &cdp->current_cursor->translog);
      }
      else { error=TRUE;  break; } // error!!
    }
  else // add all record from 1st operand:
    while ((op1opt->*op1opt->p_get_next_record)(cdp))
    { if (compute_key(cdp, NULL, 0, &rowdist.rowdist_index, rowdist.rowkey, true))
      { 
#if SQL3
        int ordnum = 0;
        op1opt->own_qe->get_position_indep((trecnum *)(rowdist.rowkey+rowdist.flag_offset+1), ordnum);
#else
        op1opt->own_qe->get_position_0((trecnum *)(rowdist.rowkey+rowdist.flag_offset+1));
#endif
        cdp->index_owner = NOOBJECT;
        bt_insert(cdp, rowdist.rowkey, &rowdist.rowdist_index, rowdist.rowdist_index.root, NULL, &cdp->current_cursor->translog); // overwrites rowkey
      }
      else { error=TRUE;  break; } // error!!
    }
 // pass 2nd operand:
  top->kont.position=1; // necessary for proper evaluation of rowdist key!
  (op2opt->*op2opt->p_reset)(cdp);
  if (qe_type==QE_EXCEPT)
  { rowdist.rowdist_index.ccateg=INDEX_UNIQUE;
    while ((op2opt->*op2opt->p_get_next_record)(cdp))
      rowdist.remove_record(cdp);
  }
  else // QE_INTERS
    while ((op2opt->*op2opt->p_get_next_record)(cdp))
      rowdist.confirm_record(cdp);
 // prepare passing the result:
  top->kont.position=0;  // will stay 0 forever because the results are taken from the op1 only
  int i, off;
  for (i=off=0;  i<rowdist.rowdist_index.partnum;  i++)
  { qset_null(rowdist.rowkey+off, rowdist.rowdist_index.parts[i].type, rowdist.rowdist_index.parts[i].partsize);
    off+=rowdist.rowdist_index.parts[i].partsize;
  }
  *(trecnum*)(rowdist.rowkey+off)=0;
  cdp->index_owner = NOOBJECT;
  if (bac.build_stack(cdp, rowdist.rowdist_index.root, &rowdist.rowdist_index, rowdist.rowkey)==BT_ERROR)
    eof=TRUE;
}

BOOL t_optim_exc_int::get_next_record(cdp_t cdp)
{ if (eof) return FALSE;
  cdp->index_owner = NOOBJECT;
  while (bac.btree_step(cdp, rowdist.rowkey))
  {// check the confirmation flag:
    if (rowdist.rowkey[rowdist.flag_offset]) // not necessary for EXCEPT
    { 
#if SQL3
      int ordnum = 0;
      top->set_position_indep(cdp, (trecnum *)(rowdist.rowkey+rowdist.flag_offset+1), ordnum);
#else
      top->set_position_0(cdp, (trecnum *)(rowdist.rowkey+rowdist.flag_offset+1));
#endif
      return TRUE;
    }
  }
  bac.close(cdp);
  rowdist.close_index(cdp);
  eof=TRUE;
  return FALSE;
}

void t_optim_exc_int::close_data(cdp_t cdp)
{ t_optim::close_data(cdp);
  bac.close(cdp);
  rowdist.close_index(cdp);
  op1opt->close_data(cdp);
  op2opt->close_data(cdp);
}

t_optim_table::t_optim_table(t_qe_table * qetIn, t_order_by * order_In) :
  t_optim(OPTIM_TABLE, qetIn, order_In)//, bac(TRUE)
{ qet=qetIn;
  normal_index_count=0; // number of valid items in the tics[] array
  docid_index=-1;       // not using fulltext
  table_read_locked=false;
  tics=NULL;
  td=NULL;    // holding a tabdescr lock
  fulltext_kontext=NULL;
  fulltext_expr=NULL;
  locking=check_local_ref_privils=false;
  reset_count=post_passed=records_taken=0;
  reclist=NULL;
}

t_optim_table::~t_optim_table(void)
{ if (tics) delete [] tics;
  if (td) { unlock_tabdescr(td);  td=NULL; }
  if (fulltext_expr) delete fulltext_expr; // must be deleted before deleting fulltext_kontext
  if (fulltext_kontext) fulltext_kontext->Release();
  if (reclist) delete reclist;
}

void t_optim_table::mark_core(void)
{ t_optim::mark_core();
  post_conds.mark_core();
  fulltext_conds.mark_core();
  if (tics) mark_array(tics);
  for (int i=0;  i<normal_index_count;  i++)
    tics[i].mark_core();
  if (fulltext_kontext) mark(fulltext_kontext);
  if (fulltext_expr) fulltext_expr->mark_core();
  recset.mark_core();
  ref_col_map.mark_core();
  if (reclist) mark(reclist);
}

#define EXPANSION_STEP 60
static int c1, c2;

bool t_reclist::fill(cdp_t cdp, t_table_index_conds * tic, ttablenum tabnum, bool index_passing_just_opened)
// Fills the [list] by the next chunk of records from the index, returns false when no more records are available.
{ 
  if (index_exhausted) return false;  
  count=0;
  cdp->index_owner = tabnum;
  if (!index_passing_just_opened)  // in get_next_record()I must reopen the index passing, in it reset() is already open
  { if (!tic->open_index_passing(cdp))  // no records found in the index
      { index_exhausted=true;  return false; }
  }
  else  // must check the result of openning the index
    if (!tic->indx)  // close_data called in open_index_passing()
      { index_exhausted=true;  return false; }
  do
  { trecnum rec = tic->next_record_from_index(cdp);
    if (rec==NORECNUM) 
      { index_exhausted=true;  break; } // index tree access has been closed in next_record_from_index()
    list[count++]=rec;
  } while (count<listsize);
  passing_pos=0;
  tic->close_index_passing(cdp);  // may have been closed and not reopened in next_record_from_index(), no problem
  return count>0;
}

bool t_reclist_ft::fill_ft(cdp_t cdp, t_optim_table * optt, bool index_passing_just_opened)
// Fills the [list] by the next chunk of records from the fulltext index, returns false when no more records are available.
{ 
  if (index_exhausted || !optt->fulltext_expr) return false;  
  count=0;
  cdp->index_owner = optt->td->tbnum;
  if (!index_passing_just_opened)  // in get_next_record() I must reopen the index passing, in reset() it is already open
  { if (!wait_record_lock_error(cdp, optt->td, NORECNUM, RLOCK))
      optt->table_read_locked=true;
    optt->fulltext_expr->open_index(cdp, FALSE);
  }
  do
  { trecnum rec = optt->get_next_fulltext_rec(cdp);
    if (rec==NORECNUM) 
      { index_exhausted=true;  break; } // index tree access has been closed in next_record_from_index()
    ft_weight[count] = cdp->__fulltext_weight;
    ft_pos[count] = cdp->__fulltext_position;
    list[count++] = rec;
  } while (count<listsize);
  passing_pos=0;
 // close passing the fulltext index:
  optt->fulltext_expr->close_index(cdp);
  if (optt->table_read_locked)  // used by the fulltext index
  { record_unlock(cdp, optt->td, NORECNUM, RLOCK);
    optt->table_read_locked=false;
  }
  return count>0;
}

void t_optim_table::reset(cdp_t cdp)
{ int i;
  if (error) { eof=TRUE;  return; }  // error in optimization, must set eof in order to prevent get_next_record working
  eof=FALSE;
#if SQL3
  t_mater_table * mat = qet->kont.t_mat;
  if (qet->kont.p_mat)
    qet->kont.p_mat->clear(cdp);
#else
  t_mater_table * mat = (t_mater_table *)qet->kont.mat;
#endif
  if (qet->is_local_table)
    qet->tabnum = mat->tabnum = *(ttablenum*)variable_ptr(cdp, mat->locdecl_level, mat->locdecl_offset);
  // check for remains from the previous passing:
  for (i=0;  i<normal_index_count;  i++) tics[i].close_data_with_warning(cdp);
  if (td) { unlock_tabdescr(td);  td=NULL; }
 // install the table:
  td = install_table(cdp, qet->tabnum);
  if (!td) { eof=error=TRUE;  return; } // error
  reset_count++;
 // check the global reference privils:
  if (mat)
    if (!cdp->has_full_access(td->schema_uuid))
    { check_local_ref_privils = qet->kont.privil_cache && !qet->kont.privil_cache->can_reference_columns(cdp, mat->refmap, td, &ref_col_map);
      if (check_local_ref_privils) 
        if ((cdp->sqloptions & SQLOPT_GLOBAL_REF_RIGHTS) || td->rec_privils_atr==NOATTRIB)
        { t_atrmap_enum mapenum(&ref_col_map);
          int column=mapenum.get_next_set();
          SET_ERR_MSG(cdp, td->attrs[column].name);
          request_error(cdp, NO_REFERENCE_PRIVILEDGE);  eof=TRUE;  
          return; 
        } // else ref_col_map will be used to check record privils
    }
 // reset:
  if (docid_index!=-1)  // fulltext index
  { t_ft_node * top = NULL;
    t_expr_attr * atex;  
#ifdef STOP   // it is probably not necessary: removing, because any long-term lock on doctab causes errors in searching
    if (!wait_record_lock_error(cdp, td, NORECNUM, RLOCK))
      table_read_locked=true;
#endif      
    for (i=0;  i<fulltext_conds.count();  i++)
    { t_expr_funct * fex = (t_expr_funct *)*fulltext_conds.acc0(i);
      t_value res;
      expr_evaluate(cdp, fex->arg1, &res);
      fulltext_kontext = get_fulltext_kontext(cdp, res.valptr(), NULL, NULL);
      res.set_null();
      if (fulltext_kontext==NULL) { eof=TRUE;  return; }
      fulltext_kontext->AddRef();
      atex = (t_expr_attr *)fex->arg2;
      expr_evaluate(cdp, fex->arg3, &res);
      if (*res.valptr())
      { if (top)
        { t_ft_node_andor * newtop = new t_ft_node_andor(FT_AND);
          if (!newtop) break;
          newtop->left=top;
          top=newtop;
          if (!analyse_fulltext_expression(cdp, res.valptr(), &newtop->right, fulltext_kontext, fex->arg3->specif)) // dictionary not found or lemmatisation not initialized
            { delete top;  eof=TRUE;  return; }
        }
        else
          if (!analyse_fulltext_expression(cdp, res.valptr(), &top, fulltext_kontext, fex->arg3->specif)) // dictionary not found or lemmatisation not initialized
            { eof=TRUE;  return; }
      }
      res.set_null();
    }
    fulltext_expr = top;
   // open the index for each word in the expression:
    if (fulltext_expr)
    { fulltext_expr->open_index(cdp, TRUE);
      fulltext_expr->next_doc(cdp, NO_DOC_ID);
    }
    else eof=TRUE; // to weak!
  }
  
  for (i=0;  i<normal_index_count;  i++)
  { t_table_index_conds * tic = &tics[i];
    tic->indx = &td->indxs[tic->index_number];
   // prepare passing intervals:
    tic->compute_interval_lists(cdp, &qet->kont);
   // compute 1st key pair:
    cdp->index_owner = td->tbnum;
    if (!tic->interval2keys(cdp)) 
      goto eof_exit;
  }
 // exhaustive passing without index:
  if (normal_index_count==0 && docid_index==-1)
  { exhaustive_passing_position=(trecnum)-1;  // ++ returns the next record number to be taken
    exhaustive_passing_limit   =td->Recnum();
  }

 // passing is prepared:
  if (normal_index_count>1 || normal_index_count>0 && docid_index!=-1)  // multiple indicies -> precomputing
  { recset.close_data();
    recset.set_mapsize(normal_index_count, docid_index!=-1);
    c1=c2=0;
    if (eof)  // set when the fulltext condition is weak, must not call get_next_fulltext_rec then! And when openning the index passing failed!
      recset.records_end(i);
    else do
    { trecnum rec;  int j;
     // add a chunk of records from the fulltext index:
      if (docid_index!=-1)
        for (j=0;  j<EXPANSION_STEP;  j++)
        { rec=get_next_fulltext_rec(cdp);
          if (rec==NORECNUM) 
            { recset.records_end(normal_index_count);  goto recsetend; }
          if (!recset.add_record(rec, normal_index_count, cdp->__fulltext_weight, cdp->__fulltext_position)) 
            goto error_exit;
        }
     // add a chunk of records from every normal index:
      for (i=0;  i<normal_index_count;  i++)
      { for (j=0;  j<EXPANSION_STEP;  j++)
        { cdp->index_owner = td->tbnum;
          rec = tics[i].next_record_from_index(cdp);
          if (rec==NORECNUM) 
            { recset.records_end(i);  goto recsetend; }
          if (!recset.add_record(rec, i)) 
            goto error_exit;
          if (i==0) c1++;  else c2++;
        }
      }
    } while (TRUE);
   recsetend:
   // close all indicies:
    if (fulltext_expr) fulltext_expr->close_index(cdp);
    for (i=0;  i<normal_index_count;  i++)  tics[i].close_data(cdp);  // bac.close() would be sufficient
    if (table_read_locked)  // used by the fulltext index
    { record_unlock(cdp, td, NORECNUM, RLOCK);
      table_read_locked=false;
    }
    precomputed=TRUE;
  }
  else if (normal_index_count>0)  // single normal index -> prepare reclist
  { if (!reclist)
    { reclist=new t_reclist;
      if (!reclist) goto error_exit;
    }
    reclist->restart();
    reclist->fill(cdp, &tics[0], qet->tabnum, true);
    precomputed=FALSE;  // not precomputed in [recset], but it IS precomputed in [reclist]
  }
  else if (docid_index!=-1)  // single fulltext index -> prepare reclist
  { if (!reclist)
    { reclist=new t_reclist_ft;
      if (!reclist) goto error_exit;
    }
    reclist->restart();
    if (!eof)
      ((t_reclist_ft*)reclist)->fill_ft(cdp, this, true);
    precomputed=FALSE;  // not precomputed in [recset], but it IS precomputed in [reclist]
  }
  else
    precomputed=FALSE;
  return;
  
 error_exit:
  error=TRUE;
 eof_exit:   
 // close_data is necessary to drop the intervals and unlock the index roots:
  for (i=0;  i<normal_index_count;  i++)
  { t_table_index_conds * tic = &tics[i];
    tic->indx = &td->indxs[tic->index_number];
      tic->close_data(cdp);
  }    
  eof=TRUE;   
  return;
}

trecnum t_optim_table::get_next_fulltext_rec(cdp_t cdp)
{ t_docid min_docid;
  if (fulltext_kontext->destructed) return NORECNUM;  // the fulltext system has been deleted
 // find a docid satisfying the expression:
  unsigned found;
  do
  { min_docid = 0;
    found = cdp->__fulltext_weight = fulltext_expr->satisfied(&min_docid, &cdp->__fulltext_position);
    if (min_docid==NO_DOC_ID) return NORECNUM;
    fulltext_expr->next_doc(cdp, min_docid);  // does not change min_docid
    if (found>0) // find the record number in the document table
    { dd_index * dindx = &td->indxs[docid_index];
      struct { t_docid docid;  trecnum recnum; } key;  trecnum recnum;
      key.docid=min_docid;  key.recnum=0;
      cdp->index_owner = td->tbnum;
      if (find_key(cdp, dindx->root, dindx, (tptr)&key, &recnum, true)) 
        return recnum;  // document found
     // no error if the referenced document does not exist, continue searching
    }
  } while (TRUE);
}

#ifdef NETWARE
static unsigned domination_counter = 0;
#define DOMINATION_LIMIT 500
#endif

BOOL t_optim_table::get_next_record(cdp_t cdp)
{ if (eof) return FALSE;
  do  // pass records from the table until a record satisfying
      // post conditions is found or all record exhausted
  {
    if (cdp->break_request)
      { request_error(cdp, REQUEST_BREAKED);  eof=error=TRUE;  return FALSE; }
#ifdef NETWARE
    if (++domination_counter > DOMINATION_LIMIT)
    { domination_counter=0;
      dispatch_kernel_control(cdp);
    }
#endif

   // get a records from any source:
    if (precomputed)
    { uns32 map;
      trecnum rec=recset.get_next(cdp, map);
      if (rec==NORECNUM) break; // no more records
      qet->kont.position=rec;
     // check the other conditions:
      BOOL all_satisfied = TRUE;
      for (int i=0;  i<normal_index_count;  i++)
        if (!(map & (1<<i)))
          for (int p=0;  p<tics[i].partcnt;  p++)
            for (int e=0;  e<tics[i].part_conds[p].pconds.count();  e++)
            { t_value res;
              expr_evaluate(cdp, *tics[i].part_conds[p].pconds.acc0(e), &res);
              if (!res.is_true())
                { all_satisfied = FALSE;  goto addit_cond_end; }
            }
      // check the fulltext condition:
       if (docid_index!=-1 && !(map & (1<<normal_index_count)))
       {// read the docid from the candidate record:
         t_value docid_val;
         expr_evaluate(cdp, ((t_expr_funct*)*fulltext_conds.acc0(0))->arg2, &docid_val);
         if (fulltext_kontext->destructed || !eval_fulltext_predicate(cdp, fulltext_expr, docid_val.i64val))
           { all_satisfied = FALSE;  goto addit_cond_end; }
       }
      addit_cond_end:
       if (!all_satisfied) continue;
      records_taken++;
    }
    else if (reclist!=NULL)
    { trecnum rec;
      if (normal_index_count)
      { rec=reclist->get_record();
        if (rec==NORECNUM)  // no more records in the window, but there may be more records in the index
        { if (!reclist->fill(cdp, &tics[0], qet->tabnum, false))
            break;  // no more records at all
          rec=reclist->get_record();  // this must always succeed because fill() returned true
        }
      }
      else
      { t_reclist_ft * reclist_ft = (t_reclist_ft*)reclist;
        rec=reclist_ft->get_record(cdp->__fulltext_weight, cdp->__fulltext_position);
        if (rec==NORECNUM)  // no more records in the window, but there may be more records in the index
        { if (!reclist_ft->fill_ft(cdp, this, false))
            break;  // no more records at all
          rec=reclist_ft->get_record(cdp->__fulltext_weight, cdp->__fulltext_position);  // this must always succeed because fill() returned true
        }
      }
      records_taken++;
      qet->kont.position=rec;
    }
#ifdef STOP  // replaced by windowed access to the index
    else if (docid_index!=-1)  // fulltext index
    { trecnum rec = get_next_fulltext_rec(cdp);
      if (rec==NORECNUM)     
        { fulltext_expr->close_index(cdp);  break; }  // eof
      records_taken++;
      qet->kont.position=rec;
    }
    else if (normal_index_count) // normal index, only 1 (not used any more)
    { cdp->index_owner = qet->tabnum;
      trecnum rec = tics[0].next_record_from_index(cdp);
      if (rec==NORECNUM) break; // eof, index tree access has been closed in next_record_from_index()
      records_taken++;
      qet->kont.position=rec;
    }
#endif
    else // exhaustive
    { if (++exhaustive_passing_position >= exhaustive_passing_limit)
        break;  // eof!
      table_descr * tbdf = tables[qet->tabnum];  // tabdescr locked between reset() and close_data()
      if (cdp->isolation_level >= REPEATABLE_READ && !SYSTEM_TABLE(qet->tabnum))
        if (wait_record_lock_error(cdp, tbdf, exhaustive_passing_position, TMPRLOCK)) continue;
      if (table_record_state(cdp, tbdf, exhaustive_passing_position))
      { if (cdp->isolation_level >= REPEATABLE_READ && !SYSTEM_TABLE(qet->tabnum))
          record_unlock(cdp, tbdf, exhaustive_passing_position, TMPRLOCK);
        continue;  // record deleted, try next one
      }
      records_taken++;
      qet->kont.position=exhaustive_passing_position;
    }

   // record found, check the post-conditions:
    if (post_conds.count())
    { BOOL all_satisfied = TRUE;
      for (int i=0;  i<post_conds.count();  i++)
      { t_value res;
        expr_evaluate(cdp, *post_conds.acc0(i), &res);
        if (!res.is_true())
          { all_satisfied = FALSE;  break; }
      }
      if (!all_satisfied) continue;
      post_passed++;
    }
    if (check_local_ref_privils)  // implies !(cdp->sqloptions & SQLOPT_GLOBAL_REF_RIGHTS) && td->rec_privils_atr!=NOATTRIB && qet->kont.privil_cache
      if (!qet->kont.privil_cache->have_local_rights_to_columns(cdp, tables[qet->tabnum], qet->kont.position, ref_col_map))
        continue;
    if (locking)
      wait_record_lock_error(cdp, tables[qet->tabnum], qet->kont.position, TMPRLOCK);
    return TRUE;
  } while (true);
 // "break" issued only on eof:
  eof=TRUE;
  return FALSE;
}

void t_table_index_conds::close_data(cdp_t cdp)
{ 
  close_index_passing(cdp);
  for (int part=0;  part<partcnt;  part++)
    part_conds[part].delete_interv_list();
  indx=NULL;  // invalid after the table is unlocked
}  

void t_table_index_conds::close_data_with_warning(cdp_t cdp)
{ if (bac.fcbn!=NOFCBNUM) { close_index_passing(cdp);  data_not_closed(); }
  for (int part=0;  part<partcnt;  part++)
    if (part_conds[part].interv_list!=NULL)
      { part_conds[part].delete_interv_list();  data_not_closed(); }
}

void t_optim_table::close_data(cdp_t cdp)
{ int i;
  if (tics)
    for (i=0;  i<normal_index_count;  i++)
      tics[i].close_data(cdp);
  for (i=0;  i<post_conds.count();  i++)
    (*post_conds.acc0(i))->close_data(cdp);
  if (td) 
  { if (table_read_locked)  // used by the fulltext index
    { record_unlock(cdp, td, NORECNUM, RLOCK);
      table_read_locked=false;
    }
    unlock_tabdescr(td);  td=NULL; 
  }  // must be before ::close_data(), bacause it destroys the temporary table
  t_optim::close_data(cdp);
  if (fulltext_expr) fulltext_expr->close_index(cdp);
}
//////////////////////////// system views ////////////////////////////////////
BOOL extract_string_condition(cdp_t cdp, t_expr_dynar * post_conds, int elnum, char * val, int lengthlimit)
{ int i;  t_value res;
  for (i=0;  i<post_conds->count();  i++)
  { t_express * ex = *post_conds->acc0(i);
    if (ex->sqe==SQE_BINARY)
    { t_expr_binary * bex = (t_expr_binary*)ex;
      if (bex->oper.pure_oper==PUOP_EQ)
      { if (bex->op1->sqe==SQE_ATTR && ((t_expr_attr*)bex->op1)->elemnum==elnum)
        { if (bex->op2->sqe==SQE_LCONST || bex->op2->sqe==SQE_VAR || bex->op2->sqe==SQE_EMBED || bex->op2->sqe==SQE_DYNPAR ||
              bex->op2->sqe==SQE_FUNCT && 
                (((t_expr_funct*)bex->op2)->num==FCNUM_CURR_SCHEMA || ((t_expr_funct*)bex->op2)->num==FCNUM_LOCAL_SCHEMA))
          { expr_evaluate(cdp, bex->op2, &res);
            if (res.length<=lengthlimit)
              { strcpy(val, res.valptr());  return TRUE; }
            else res.set_null();
          }
        }
        else if (bex->op2->sqe==SQE_ATTR && ((t_expr_attr*)bex->op2)->elemnum==elnum)
        { if (bex->op1->sqe==SQE_LCONST || bex->op1->sqe==SQE_VAR || bex->op1->sqe==SQE_EMBED || bex->op1->sqe==SQE_DYNPAR ||
              bex->op1->sqe==SQE_FUNCT && 
                (((t_expr_funct*)bex->op1)->num==FCNUM_CURR_SCHEMA || ((t_expr_funct*)bex->op1)->num==FCNUM_LOCAL_SCHEMA))
          { expr_evaluate(cdp, bex->op1, &res);
            if (res.length<=lengthlimit)
              { strcpy(val, res.valptr());  return TRUE; }
            else res.set_null();
          }
        }
      }
    }
  }
  return FALSE;
}

BOOL extract_binary_condition(cdp_t cdp, t_expr_dynar * post_conds, int elnum, unsigned char * val, int length)
{ int i;  t_value res;
  for (i=0;  i<post_conds->count();  i++)
  { t_express * ex = *post_conds->acc0(i);
    if (ex->sqe==SQE_BINARY)
    { t_expr_binary * bex = (t_expr_binary*)ex;
      if (bex->oper.pure_oper==PUOP_EQ)
      { if (bex->op1->sqe==SQE_ATTR && ((t_expr_attr*)bex->op1)->elemnum==elnum)
        { if (bex->op2->sqe==SQE_LCONST || bex->op2->sqe==SQE_VAR || bex->op2->sqe==SQE_EMBED || bex->op2->sqe==SQE_DYNPAR)
          { expr_evaluate(cdp, bex->op2, &res);
            if (res.length==length)
              { memcpy(val, res.valptr(), length);  return TRUE; }
            else res.set_null();
          }
        }
        else if (bex->op2->sqe==SQE_ATTR && ((t_expr_attr*)bex->op2)->elemnum==elnum)
        { if (bex->op1->sqe==SQE_LCONST || bex->op1->sqe==SQE_VAR || bex->op1->sqe==SQE_EMBED || bex->op1->sqe==SQE_DYNPAR)
          { expr_evaluate(cdp, bex->op1, &res);
            if (res.length==length)
              { memcpy(val, res.valptr(), length);  return TRUE; }
            else res.set_null();
          }
        }
      }
    }
  }
  return FALSE;
}

BOOL extract_integer_condition(cdp_t cdp, t_expr_dynar * post_conds, int elnum, sig32 * val)
{ int i;  t_value res;
  for (i=0;  i<post_conds->count();  i++)
  { t_express * ex = *post_conds->acc0(i);
    if (ex->sqe==SQE_BINARY)
    { t_expr_binary * bex = (t_expr_binary*)ex;
      if (bex->oper.pure_oper==PUOP_EQ)
      { if (bex->op1->sqe==SQE_ATTR && ((t_expr_attr*)bex->op1)->elemnum==elnum)
        { if (bex->op2->sqe==SQE_SCONST || bex->op2->sqe==SQE_VAR || bex->op2->sqe==SQE_EMBED || bex->op2->sqe==SQE_DYNPAR)
          { expr_evaluate(cdp, bex->op2, &res);
            *val = res.intval;  return TRUE; 
          }
        }
        else if (bex->op2->sqe==SQE_ATTR && ((t_expr_attr*)bex->op2)->elemnum==elnum)
        { if (bex->op1->sqe==SQE_SCONST || bex->op1->sqe==SQE_VAR || bex->op1->sqe==SQE_EMBED || bex->op1->sqe==SQE_DYNPAR)
          { expr_evaluate(cdp, bex->op1, &res);
            *val = res.intval;  return TRUE; 
          }
        }
      }
    }
  }
  return FALSE;
}


tcursnum open_object_iterator(cdp_t cdp, const char * schema_name, const char * object_name, tcateg category, cur_descr ** pcd)
{ WBUUID appl_id;
  if (schema_name) 
    if (!*schema_name) schema_name=NULL;
    else if (ker_apl_name2id(cdp, schema_name, appl_id, NULL)) 
      return NOCURSOR; // schema name specified and not found -> return, empty result set
  if (object_name) 
    if (!*object_name) object_name=NULL;
  char query[200+OBJ_NAME_LEN];  int len;
 // prepare query test:
  strcpy(query, category==CATEG_TABLE ? "SELECT TAB_NAME" : "SELECT OBJ_NAME");  len=(int)strlen(query);
  strcpy(query+len, ",CATEGORY,APL_UUID,DEFIN FROM ");             len+=(int)strlen(query+len);
  strcpy(query+len, category==CATEG_TABLE ? "TABTAB" : "OBJTAB");  len+=(int)strlen(query+len);
  strcpy(query+len, " WHERE CATEGORY=chr(");                       len+=(int)strlen(query+len);
  int2str(category, query+len);                                    len+=(int)strlen(query+len);
  strcpy(query+len, ")");                                          len++;
  if (schema_name)
  { strcpy(query+len, "AND APL_UUID=X\'");                         len+=(int)strlen(query+len);
    bin2hex(query+len, appl_id,  UUID_SIZE);                       len+=2*UUID_SIZE;
    strcpy(query+len, "\'");                                       len++;
  }
  if (object_name)
  { sprintf(query+len, "AND %s=\'%s\'", category==CATEG_TABLE ? "TAB_NAME" : "OBJ_NAME", object_name);
    sys_Upcase(query+len);
  }
  return open_working_cursor(cdp, query, pcd);
}

struct result_logged_users
{ tobjname login_name;
  int  client_number;
  tobjname sel_schema_name;
  int  state;
  int  connection;
  char net_address[30+1];
  wbbool detached;
  wbbool worker_thread;
  wbbool own_connection;
  wbbool transaction_open;
  int  isolation_level;
  unsigned sql_options;
  int  lock_waiting_timeout;
  int  comm_encryption;
  unsigned session_number;
  wbbool profiled_thread;
  char thread_name[1+31+31+1];
  unsigned req_proc_time;
  char proc_sql_stmt[1000+1];
  unsigned open_cursors;
};

static const t_colval logged_users_coldescr[] = {
{  1, NULL, (void*)offsetof(result_logged_users, login_name),           NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_logged_users, client_number),        NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_logged_users, sel_schema_name),      NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_logged_users, state),                NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_logged_users, connection),           NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_logged_users, net_address),          NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_logged_users, detached),             NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_logged_users, worker_thread),        NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_logged_users, own_connection),       NULL, NULL, 0, NULL },
{ 10, NULL, (void*)offsetof(result_logged_users, transaction_open),     NULL, NULL, 0, NULL },
{ 11, NULL, (void*)offsetof(result_logged_users, isolation_level),      NULL, NULL, 0, NULL },
{ 12, NULL, (void*)offsetof(result_logged_users, sql_options),          NULL, NULL, 0, NULL },
{ 13, NULL, (void*)offsetof(result_logged_users, lock_waiting_timeout), NULL, NULL, 0, NULL },
{ 14, NULL, (void*)offsetof(result_logged_users, comm_encryption),      NULL, NULL, 0, NULL },
{ 15, NULL, (void*)offsetof(result_logged_users, session_number),       NULL, NULL, 0, NULL },
{ 16, NULL, (void*)offsetof(result_logged_users, profiled_thread),      NULL, NULL, 0, NULL },
{ 17, NULL, (void*)offsetof(result_logged_users, thread_name),          NULL, NULL, 0, NULL },
{ 18, NULL, (void*)offsetof(result_logged_users, req_proc_time),        NULL, NULL, 0, NULL },
{ 19, NULL, (void*)offsetof(result_logged_users, proc_sql_stmt),        NULL, NULL, 0, NULL },
{ 20, NULL, (void*)offsetof(result_logged_users, open_cursors),         NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_logged_users(cdp_t cdp, table_descr * tbdf)
{ result_logged_users rlu;  
  t_vector_descr vector(logged_users_coldescr, &rlu);
  for (int i=1;  i<=max_task_index;  i++)   /* system process 0 not counted */
  { bool insert_it = false;
    { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
      if (cds[i])
      { cdp_t cdx = cds[i];
        strcpy(rlu.login_name, cdx->prvs.luser_name());
        rlu.client_number       =cdx->applnum_perm;
        strcpy(rlu.sel_schema_name, cdx->sel_appl_name);  
        rlu.state               =cdx->wait_type;
        rlu.connection          =cdx->in_use!=PT_SLAVE || !cdx->pRemAddr ? 0 : 
          cdx->pRemAddr->is_http ? 4 :
          cdx->pRemAddr->my_protocol==TCPIP ? 1 : cdx->pRemAddr->my_protocol==IPXSPX ? 2 : cdx->pRemAddr->my_protocol==NETBIOS ? 3 : -1;
        if (!rlu.connection) *rlu.net_address=0;
        else /* cdx->pRemAddr!=NULL implied */ cdx->pRemAddr->GetAddressString(rlu.net_address);
        rlu.detached            =cdx->is_detached;
        rlu.worker_thread       =cdx->in_use==PT_WORKER;
        rlu.own_connection      =cdx==cdp;
        rlu.transaction_open    =cdx->expl_trans!=TRANS_NO;
        rlu.isolation_level     =cdx->isolation_level;
        rlu.sql_options         =cdx->sqloptions;
        rlu.lock_waiting_timeout=cdx->waiting;
        rlu.comm_encryption     =cdx->commenc ? 1 : 0;
        strmaxcpy(rlu.thread_name, cdx->thread_name, sizeof(rlu.thread_name));
        rlu.session_number      =cdx->session_number;
        rlu.profiled_thread     =cdx->profiling_thread;  
        rlu.req_proc_time       =cdx->wait_type==WAIT_FOR_REQUEST || cdx->wait_type==WAIT_SENDING_ANS || cdx->wait_type==WAIT_RECEIVING || cdx->wait_type==WAIT_TERMINATING 
                                   ? 0 : stamp_now() - cdx->last_request_time;
        *rlu.proc_sql_stmt      =0;
       // find the current SQL statement in the context (if any):
        t_exkont * ekt = lock_and_get_execution_kontext(cdx);
        while (ekt)
        { t_exkont_info exkont_info;
          ekt->get_info(cdx, &exkont_info);
          if (exkont_info.type==EKT_SQLSTMT)
          { ekt->get_descr(cdx, rlu.proc_sql_stmt, sizeof(rlu.proc_sql_stmt));
            break;
          }                                                          
          ekt=ekt->_next();
        }
        unlock_execution_kontext(cdx);
        rlu.open_cursors = count_cursors(cdx->applnum_perm);
        insert_it = true;
      } // cdx valid
    } // CS
   // database operation performed outside the CS:
    if (insert_it)
      tb_new(cdp, tbdf, FALSE, &vector);
  }
}

struct result_client_activity
{ 
  int   client_number;
  uns32 requests;
  uns32 sql_statements;
  uns32 opened_cursors;
  uns32 pages_read;
  uns32 pages_written;
};

static const t_colval client_activity_coldescr[] = {
{  1, NULL, (void*)offsetof(result_client_activity, client_number),   NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_client_activity, requests),        NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_client_activity, sql_statements),  NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_client_activity, opened_cursors),  NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_client_activity, pages_read),      NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_client_activity, pages_written),   NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_client_activity(cdp_t cdp, table_descr * tbdf)
{ result_client_activity rlu;  
  t_vector_descr vector(client_activity_coldescr, &rlu);
  for (int i=1;  i<=max_task_index;  i++)   /* system process 0 not counted */
  { bool insert_it = false;
    { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
      if (cds[i])
      { cdp_t cdx = cds[i];
        rlu.client_number  = cdx->applnum_perm;
        rlu.requests       = cdx->cnt_requests;
        rlu.sql_statements = cdx->cnt_sqlstmts;
        rlu.opened_cursors = cdx->cnt_cursors;
        rlu.pages_read     = cdx->cnt_pagerd;
        rlu.pages_written  = cdx->cnt_pagewr;
        insert_it = true;
      } // cdx valid
    } // CS
   // database operation performed outside the CS:
    if (insert_it)
      tb_new(cdp, tbdf, FALSE, &vector);
  }
}
/////////////////////////////////////////// memory usage //////////////////////////////////////
struct result_memory_usage
{ uns32 owner_code;
  uns32 bytes;
  uns32 items;
};

static const t_colval memory_usage_coldescr[] = {     
{  1, NULL, (void*)offsetof(result_memory_usage, owner_code), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_memory_usage, bytes),      NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_memory_usage, items),      NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_memory_usage(cdp_t cdp, table_descr * tbdf)
{ result_memory_usage ros;  
  t_vector_descr vector(memory_usage_coldescr, &ros);
  t_mem_info mi[256];
  if (!create_memory_stats(mi))
    for (int i=0;  i<256;  i++)
      if (mi[i].size)
      { ros.owner_code=i;
        ros.bytes=16*mi[i].size;
        ros.items=mi[i].items;
        tb_new(cdp, tbdf, FALSE, &vector);
      }  
}      
//////////////////////////////////////////// object state /////////////////////////////////////
struct result_object_state
{ tobjname schema_name;
  tobjname object_name;
  sig16    category;
  sig32    error_code;
  char     error_context[PRE_KONTEXT+1+POST_KONTEXT+1];
};

static const t_colval object_state_coldescr[] = {     
{  1, NULL, (void*)offsetof(result_object_state, schema_name),   NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_object_state, object_name),   NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_object_state, category),      NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_object_state, error_code),    NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_object_state, error_context), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };


static void verify_object(cdp_t cdp, const char * object_name, const char * schema_name, const WBUUID schema_uuid, tcateg categ, tobjnum objnum, table_descr * tbdf)
{ result_object_state ros;  
  t_vector_descr vector(object_state_coldescr, &ros);
  strcpy(ros.object_name, object_name);
  strcpy(ros.schema_name, schema_name);
  ros.category=categ;
  ros.error_code=NONEINTEGER;
  *ros.error_context=0;
  if (categ==CATEG_TABLE || categ==CATEG_CURSOR || categ==CATEG_TRIGGER ||
      categ==CATEG_SEQ || categ==CATEG_DOMAIN || categ==CATEG_INFO ||
      categ==CATEG_PROC && strcmp(object_name, MODULE_GLOBAL_DECLS))
  {// load and compile the object:
    char * def = ker_load_objdef(cdp, categ==CATEG_TABLE ? tabtab_descr : objtab_descr, objnum);
    if (def)
    { compil_info xCI(cdp, def, sql_statement_seq);  t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;
      t_short_term_schema_context stsc(cdp, schema_uuid);
      int err=compile_masked(&xCI);
      ros.error_code=err;
      if (err)
      { create_error_contect(&xCI);
        if (cdp->comp_kontext)
        { strmaxcpy(ros.error_context, cdp->comp_kontext, sizeof(ros.error_context));
          for (int i=0;  ros.error_context[i];  i++)
            if (((unsigned char*)ros.error_context)[i] < ' ') ros.error_context[i]=' ';
        }    
      }  
      sql_statement * so=(sql_statement*)xCI.stmt_ptr;
      if (so) delete so;
      corefree(def);
    }  
  }  
  tb_new(cdp, tbdf, FALSE, &vector);
}

static void fill_object_state(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{// analyze the conditions, find the schema name object name and category:
  tobjname object_name, schema_name;  *object_name=*schema_name=0;  sig32 categval = -1; 
  extract_string_condition(cdp, post_conds, 1, schema_name, OBJ_NAME_LEN);
  extract_string_condition(cdp, post_conds, 2, object_name, OBJ_NAME_LEN);
  extract_integer_condition(cdp, post_conds, 3, &categval);

 // convert the schema name to schema UUID:
  WBUUID appl_id, last_appl_id;  tobjname last_schema_name;  // last... used only when the schema_name is not specified
  sys_Upcase(schema_name);
  if (!*schema_name) 
  { *last_schema_name=0;  memset(last_appl_id, 0, UUID_SIZE);
  }
  else // list only tables from the specified schema 
    if (ker_apl_name2id(cdp, schema_name, appl_id, NULL)) 
      return; // schema name specified and not found -> return, empty result set
  sys_Upcase(object_name);
  
 
  if (*object_name && *schema_name && categval != -1) // operation on a single object:
  { tobjnum objnum = find2_object(cdp, categval, appl_id, object_name);
    if (objnum==NOOBJECT) return;  // object not found, empty result set
    verify_object(cdp, object_name, schema_name, appl_id, categval, objnum, tbdf);
  }
  else  // processing multiple objects:
  { if (categval == -1 || categval==CATEG_TABLE)  // pass the tables:
    { trecnum rec, limit = tabtab_descr->Recnum();  tfcbnum fcbn;
      for (rec=0;  rec<limit;  rec++)
      { const ttrec * dt = (const ttrec*)fix_attr_read(cdp, tabtab_descr, rec, DEL_ATTR_NUM, &fcbn);
        if (dt!=NULL)
        { if (!dt->del_mark && dt->categ==CATEG_TABLE) /* not deleted, not a link object */
            if (!*schema_name || !memcmp(appl_id, dt->apluuid, UUID_SIZE)) 
            if (!*object_name || !strcmp(object_name, dt->name))
            { if (*schema_name)
                verify_object(cdp, dt->name, schema_name, dt->apluuid, dt->categ, rec, tbdf);
              else // must find the schema name
              { if (memcmp(last_appl_id, dt->apluuid, UUID_SIZE))
                { if (ker_apl_id2name(cdp, dt->apluuid, last_schema_name, NULL)) strcpy(last_schema_name, "?");
                  memcpy(last_appl_id, dt->apluuid, UUID_SIZE);
                }
                verify_object(cdp, dt->name, last_schema_name, dt->apluuid, dt->categ, rec, tbdf);
              }
            }  
          unfix_page(cdp, fcbn);
        } // fixed
      } // cycle on TABTAB records
    }  
    if (categval!=CATEG_TABLE)  // pass objects
    { trecnum rec, limit = objtab_descr->Recnum();  tfcbnum fcbn;
      for (rec=0;  rec<limit;  rec++)
      { const ttrec * dt = (const ttrec*)fix_attr_read(cdp, objtab_descr, rec, DEL_ATTR_NUM, &fcbn);
        if (dt!=NULL)
        { if (!dt->del_mark)
           if (categval==-1 || dt->categ==categval) /* not deleted, not a link object */
            if (!*schema_name || !memcmp(appl_id, dt->apluuid, UUID_SIZE)) 
            if (!*object_name || !strcmp(object_name, dt->name))
            { if (*schema_name)
                verify_object(cdp, dt->name, schema_name, dt->apluuid, dt->categ, rec, tbdf);
              else // must find the schema name
              { if (memcmp(last_appl_id, dt->apluuid, UUID_SIZE))
                { if (ker_apl_id2name(cdp, dt->apluuid, last_schema_name, NULL)) strcpy(last_schema_name, "?");
                  memcpy(last_appl_id, dt->apluuid, UUID_SIZE);
                }
                verify_object(cdp, dt->name, last_schema_name, dt->apluuid, dt->categ, rec, tbdf);
              }
            }  
          unfix_page(cdp, fcbn);
        } // fixed
      } // cycle on TABTAB records
    }
  }
}  
//////////////////////////////////////////// table columns ////////////////////////////////////////
struct result_table_columns
{ tobjname schema_name;
  tobjname table_name;
  char   column_name[ATTRNAMELEN+1];
  sig32  ordinal_position;
  sig32  data_type;
  sig32  length;
  sig32  precision;  // scale
//  char   default_value[50+1];
  wbbool is_nullable;
  sig32  value_count;
  wbbool expandable;
  wbbool wide_char;
  wbbool ignore_case;
  char   collation_name[20+1];
  wbbool with_time_zone;
  tobjname domain_name;
 // hints:
  char   hint_caption[ATTRNAMELEN+1];
  char   hint_codetext[ATTRNAMELEN+1];
  char   hint_codeval[ATTRNAMELEN+1];
  uns32  specif_opaque;
 // var-length data:
  char hint_helptext[HINT_HELPTEXT_LEN+1], hint_codebook[HINT_CODEBOOK_LEN+1], hint_oleserver[HINT_OLESERVER_LEN+1];
  uns32 len_defval, len_helptext, len_codebook, len_oleserver;


  void store_type_info(int type, t_specif specif, uns8 multi, wbbool nullable)
  { data_type   = type;
    length      = SPECIFLEN(type) || type==ATT_CHAR ? specif.length : NONEINTEGER;
    precision   = type==ATT_MONEY || type==ATT_INT8  || type==ATT_INT16 || type==ATT_INT32 || type==ATT_INT64
         ? specif.scale :   // the column contains the scale, in fact
       /*IS_CHAR_TYPE(type) ? att->specif.opqval :*/ 0;  // opqval added in 8.0a, removed in 8.0.4.9
    if (IS_CHAR_TYPE(type))
    { wide_char   = specif.wide_char;
      ignore_case = specif.ignore_case;
      if (!specif.charset) *collation_name=0;
      else strcpy(collation_name, get_collation_name(specif.charset));
    }
    else
    { wide_char   = NONEBOOLEAN;
      ignore_case = NONEBOOLEAN;
      *collation_name = 0;
    }
    with_time_zone = type==ATT_TIME || type==ATT_TIMESTAMP ? specif.with_time_zone : NONEBOOLEAN;
    specif_opaque = specif.opqval;
    is_nullable = nullable;
    value_count = multi & 0x7f;
    expandable  = (multi & 0x80) != 0;
  }
  void load_hints(const char * comment)
  { get_hint_from_comment(comment, HINT_CAPTION,   hint_caption,  sizeof(hint_caption));
    get_hint_from_comment(comment, HINT_HELPTEXT,  hint_helptext, sizeof(hint_helptext));
    get_hint_from_comment(comment, HINT_CODEBOOK,  hint_codebook, sizeof(hint_codebook));
    get_hint_from_comment(comment, HINT_CODETEXT,  hint_codetext, sizeof(hint_codetext));
    get_hint_from_comment(comment, HINT_CODEVAL,   hint_codeval,  sizeof(hint_codeval));
    get_hint_from_comment(comment, HINT_OLESERVER, hint_oleserver,sizeof(hint_oleserver));
  }
  void define_string_lengths(void)
    { len_helptext=(int)strlen(hint_helptext);  len_codebook=(int)strlen(hint_codebook);  len_oleserver=(int)strlen(hint_oleserver); }
  void insert_cursor_columns_descr(cdp_t cdp, table_descr * tbdf, t_query_expr * qe, t_vector_descr * vector);
};

#define TABLE_COLUMNS_SIZE 24
static const t_colval table_columns_coldescr_patt[TABLE_COLUMNS_SIZE] = {     
{  1, NULL, (void*)offsetof(result_table_columns, schema_name),      NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_table_columns, table_name),       NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_table_columns, column_name),      NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_table_columns, ordinal_position), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_table_columns, data_type),        NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_table_columns, length),           NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_table_columns, precision),        NULL, NULL, 0, NULL },
{  8, NULL, NULL/*(void*)offsetof(result_table_columns, default_value)*/,    NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_table_columns, is_nullable),      NULL, NULL, 0, NULL },
{ 10, NULL, (void*)offsetof(result_table_columns, value_count),      NULL, NULL, 0, NULL },
{ 11, NULL, (void*)offsetof(result_table_columns, expandable),       NULL, NULL, 0, NULL },
{ 12, NULL, (void*)offsetof(result_table_columns, wide_char),        NULL, NULL, 0, NULL },
{ 13, NULL, (void*)offsetof(result_table_columns, ignore_case),      NULL, NULL, 0, NULL },
{ 14, NULL, (void*)offsetof(result_table_columns, collation_name),   NULL, NULL, 0, NULL },
{ 15, NULL, (void*)offsetof(result_table_columns, with_time_zone),   NULL, NULL, 0, NULL },
{ 16, NULL, (void*)offsetof(result_table_columns, domain_name),      NULL, NULL, 0, NULL },
{ 17, NULL, (void*)offsetof(result_table_columns, hint_caption),     NULL, NULL, 0, NULL },
{ 18, NULL, NULL,                                                    NULL, NULL, 0, NULL },
{ 19, NULL, NULL,                                                    NULL, NULL, 0, NULL },
{ 20, NULL, (void*)offsetof(result_table_columns, hint_codetext),    NULL, NULL, 0, NULL },
{ 21, NULL, (void*)offsetof(result_table_columns, hint_codeval),     NULL, NULL, 0, NULL },
{ 22, NULL, NULL,                                                    NULL, NULL, 0, NULL },
{ 23, NULL, (void*)offsetof(result_table_columns, specif_opaque),    NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_table_columns(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_table_columns rtc;  int i;
 // prepare the vector descriptor:
  t_colval table_columns_coldescr[TABLE_COLUMNS_SIZE];  
  memcpy(table_columns_coldescr, table_columns_coldescr_patt, sizeof(table_columns_coldescr));
  for (i=0;  table_columns_coldescr[i].attr!=NOATTRIB;  i++)  // replacing offsets by addresses
    table_columns_coldescr[i].dataptr=(char*)&rtc + (size_t)table_columns_coldescr[i].dataptr;
                                                            table_columns_coldescr[ 8-1].lengthptr=&rtc.len_defval;
  table_columns_coldescr[18-1].dataptr=rtc.hint_helptext;   table_columns_coldescr[18-1].lengthptr=&rtc.len_helptext;
  table_columns_coldescr[19-1].dataptr=rtc.hint_codebook;   table_columns_coldescr[19-1].lengthptr=&rtc.len_codebook;
  table_columns_coldescr[22-1].dataptr=rtc.hint_oleserver;  table_columns_coldescr[22-1].lengthptr=&rtc.len_oleserver;
  t_vector_descr vector(table_columns_coldescr);
 // analyze the conditions, find the schema name and table name:
  tobjname table_name, schema_name;  *table_name=*schema_name=0;
  extract_string_condition(cdp, post_conds, 1, schema_name, OBJ_NAME_LEN);
  extract_string_condition(cdp, post_conds, 2, table_name,  OBJ_NAME_LEN);
 // convert the schema name to schema UUID:
  WBUUID appl_id, last_appl_id;  tobjname last_schema_name;  // last... used only when the schema_name is not specified
  if (!*schema_name) 
  { *last_schema_name=0;  memset(last_appl_id, 0, UUID_SIZE);
  }
  else // list only tables from the specified schema 
    if (ker_apl_name2id(cdp, schema_name, appl_id, NULL)) 
      return; // schema name specified and not found -> return, empty result set
  sys_Upcase(table_name);
 // pass the tables:
  trecnum rec, limit = tabtab_descr->Recnum();  tfcbnum fcbn;
  for (rec=0;  rec<limit;  rec++)
  { const ttrec * dt = (const ttrec*)fix_attr_read(cdp, tabtab_descr, rec, DEL_ATTR_NUM, &fcbn);
    if (dt!=NULL)
    { if (!dt->del_mark && dt->categ==CATEG_TABLE) /* not deleted, not a link object */
        if (!*schema_name || !memcmp(appl_id, dt->apluuid, UUID_SIZE)) 
        if (!*table_name  || !strcmp(table_name, dt->name))
        { table_all ta;  
          if (!partial_compile_to_ta(cdp, rec, &ta))   // unless compilation error
          { if (*schema_name)
              strcpy(rtc.schema_name, schema_name);
            else // must find the schema name
            { if (memcmp(last_appl_id, dt->apluuid, UUID_SIZE))
              { if (ker_apl_id2name(cdp, dt->apluuid, last_schema_name, NULL)) strcpy(last_schema_name, "?");
                memcpy(last_appl_id, dt->apluuid, UUID_SIZE);
              }
              strcpy(rtc.schema_name, last_schema_name);
            }
            strcpy(rtc.table_name,  ta.selfname);
            for (i=1;  i<ta.attrs.count();  i++)
            { atr_all * att = ta.attrs.acc0(i);
              strcpy(rtc.column_name, att->name);
              rtc.ordinal_position=i;
              //strmaxcpy(rtc.default_value, att->defval ? att->defval : NULLSTRING, sizeof(rtc.default_value));
              table_columns_coldescr[8-1].dataptr=att->defval ? att->defval : NULLSTRING;
              rtc.len_defval = (int)strlen((const char*)table_columns_coldescr[8-1].dataptr);
              t_type_specif_info tsi;
              if (att->type==ATT_DOMAIN && compile_domain_to_type(cdp, att->specif.domain_num, tsi))
              { rtc.store_type_info(tsi.tp, tsi.specif, att->multi, att->nullable && tsi.nullable); // NOT NULLs from both levels merged
                strcpy(rtc.domain_name, ta.names.acc(i)->name);
              }
              else // when domain cannt be compiled, the ATT_DOMAIN type is returned
              { rtc.store_type_info(att->type, att->specif, att->multi, att->nullable);
                *rtc.domain_name=0;
              }
             // add hints:
              rtc.load_hints(att->comment);
              rtc.define_string_lengths();
              tb_new(cdp, tbdf, FALSE, &vector);
            } // cycle on columns
          } // table compiled  
        }
      unfix_page(cdp, fcbn);
    }
  } // cycle on TABTAB records
#ifdef STOP  // different _iv_ created
 // show view columns if a specific view name given:
  if (*table_name && *schema_name)
  { tobjnum viewnum = find2_object(cdp, CATEG_CURSOR, appl_id, table_name);
    if (viewnum!=NOOBJECT)
    { unsigned size;
      d_table * tdd = kernel_get_descr(cdp, CATEG_CURSOR, viewnum, &size);
      if (tdd!=NULL)
      { strcpy(rtc.schema_name, schema_name);
        strcpy(rtc.table_name,  table_name);
        for (i=1;  i<tdd->attrcnt;  i++)
        { d_attr * atr = tdd->attribs+i;
          strcpy(rtc.column_name, atr->name);
          rtc.ordinal_position=i;
          *rtc.default_value=0;
          rtc.store_type_info(atr->type, atr->specif, atr->multi, atr->nullable);
          tb_new(cdp, tbdf, FALSE, &vector);
        }
        corefree(tdd);
      }
    }
  }
#endif
}

void result_table_columns::insert_cursor_columns_descr(cdp_t cdp, table_descr * tbdf, t_query_expr * qe, t_vector_descr * vector)
{ table_all * ta = NULL;  ttablenum curr_tbnum = NOTBNUM;
  int i;
  for (i=1;  i<qe->kont.elems.count();  i++)
  { elem_descr * el = qe->kont.elems.acc0(i);  
    strcpy(column_name, el->name);
    ordinal_position=i;
    store_type_info(el->type, el->specif, el->multi, el->nullable);
    *domain_name=0;
   // preset for derived columns:
    ((t_colval*)vector->colvals)[8-1].dataptr=NULLSTRING;
    len_defval = 0;
    //*default_value=0;
    load_hints("");
   // find the corresponding table column, if exists:
    const db_kontext * akont = &qe->kont;  const elem_descr * anel = el;  int elemnum = i;
    while (anel->link_type==ELK_KONTEXT)
    { elemnum = anel->kontext.down_elemnum;
      akont   = anel->kontext.kont;   
      anel    = akont->elems.acc0(elemnum);  
    }
#if SQL3
    if (anel->link_type==ELK_NO && akont->t_mat) 
    { t_mater_table * mater = akont->t_mat;
#else
    if (anel->link_type==ELK_NO && akont->mat && !akont->mat->is_ptrs) 
    { t_mater_table * mater = (t_mater_table*)akont->mat;
#endif
      if (mater->permanent)
      { if (curr_tbnum!=mater->tabnum)
        { if (ta) delete ta;  curr_tbnum = NOTBNUM;
          ta = new table_all;
          if (ta && !partial_compile_to_ta(cdp, mater->tabnum, ta))   // unless compilation error
            curr_tbnum = mater->tabnum;
        }
        if (curr_tbnum==mater->tabnum)
        { atr_all * att = ta->attrs.acc0(elemnum);
          //strmaxcpy(default_value, att->defval ? att->defval : NULLSTRING, sizeof(default_value));
          if (att->defval)
            { ((t_colval*)vector->colvals)[8-1].dataptr=att->defval;  len_defval = (int)strlen(att->defval); }
          load_hints(att->comment);
        }
      }
    }
   // hints
    define_string_lengths();
    tb_new(cdp, tbdf, FALSE, vector);
  }
  if (ta) delete ta;
}

static void fill_viewed_columns(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_table_columns rtc;  int i;
 // prepare the vector descriptor:
  t_colval table_columns_coldescr[TABLE_COLUMNS_SIZE];  
  memcpy(table_columns_coldescr, table_columns_coldescr_patt, sizeof(table_columns_coldescr));
  for (i=0;  table_columns_coldescr[i].attr!=NOATTRIB;  i++)  // replacing offsets by addresses
    table_columns_coldescr[i].dataptr=(char*)&rtc + (size_t)table_columns_coldescr[i].dataptr;
                                                            table_columns_coldescr[ 8-1].lengthptr=&rtc.len_defval;
  table_columns_coldescr[18-1].dataptr=rtc.hint_helptext;   table_columns_coldescr[18-1].lengthptr=&rtc.len_helptext;
  table_columns_coldescr[19-1].dataptr=rtc.hint_codebook;   table_columns_coldescr[19-1].lengthptr=&rtc.len_codebook;
  table_columns_coldescr[22-1].dataptr=rtc.hint_oleserver;  table_columns_coldescr[22-1].lengthptr=&rtc.len_oleserver;
  t_vector_descr vector(table_columns_coldescr);
 // analyze the conditions, find the schema name and table name:
  tobjname table_name, schema_name;  *table_name=*schema_name=0;
  extract_string_condition(cdp, post_conds, 1, schema_name, OBJ_NAME_LEN);
  extract_string_condition(cdp, post_conds, 2, table_name,  OBJ_NAME_LEN);

 // check for table_name==" number of an open cursor":
  if (*table_name==' ')
  { sig32 cursnum;
    if (str2int(table_name+1, &cursnum))
    { *rtc.schema_name=0;
      strcpy(rtc.table_name, table_name);  // must be the same as specified, otherwise removed by the following tests
      cur_descr * cd = get_cursor(cdp, cursnum);
      if (cd)
        rtc.insert_cursor_columns_descr(cdp, tbdf, cd->qe, &vector);
      return;
    }
  } // else filling the decription of columns in stored query/ies

 // convert the schema name to schema UUID:
  WBUUID appl_id, last_appl_id;  tobjname last_schema_name;
  if (!*schema_name) 
  { *last_schema_name=0;  memset(last_appl_id, 0, UUID_SIZE);
  }
  else // list only tables from the specified schema 
    if (ker_apl_name2id(cdp, schema_name, appl_id, NULL)) 
      return; // schema name specified and not found -> return, empty result set
  sys_Upcase(table_name);
 // create the query selecting the stored queries:
  char query[100+2*UUID_SIZE+OBJ_NAME_LEN];
  sprintf(query, "select * from OBJTAB WHERE category=chr(%u)", CATEG_CURSOR);
  if (*table_name)
    { strcat(query, " AND OBJ_NAME='");  strcat(query, table_name);  strcat(query, "'"); }
  if (*schema_name)
  { strcat(query, " AND APL_UUID=X\'");
    int plen=(int)strlen(query);
    bin2hex(query+plen, appl_id, UUID_SIZE);  plen+=2*UUID_SIZE;
    query[plen++]='\'';  query[plen]=0;
  }
  cur_descr *cd;
  tcursnum cursnum = open_working_cursor(cdp, query, &cd);
  if (cursnum != NOCURSOR)
  { for (trecnum r = 0;  r < cd->recnum && !cdp->is_an_error();  r++)
    { trecnum viewnum;
      cd->curs_seek(r, &viewnum);
      { tb_read_atr(cdp, objtab_descr, viewnum, APPL_ID_ATR, (tptr)appl_id);
        if (*schema_name)
          strcpy(rtc.schema_name, schema_name);
        else // must find the schema name
        { if (memcmp(last_appl_id, appl_id, UUID_SIZE))
          { if (ker_apl_id2name(cdp, appl_id, last_schema_name, NULL)) strcpy(last_schema_name, "?");
            memcpy(last_appl_id, appl_id, UUID_SIZE);
          }
          strcpy(rtc.schema_name, last_schema_name);
        }
        if (*table_name)
          strcpy(rtc.table_name,  table_name);
        else
          tb_read_atr(cdp, objtab_descr, viewnum, OBJ_NAME_ATR, rtc.table_name);
       // compile the query:
        tptr buf=ker_load_objdef(cdp, objtab_descr, viewnum);
        if (buf)
        { sql_statement * so;
          { t_short_term_schema_context stsc(cdp, appl_id);
            so=sql_submitted_comp(cdp, buf, NULL, true);  // "true" prevents the bad query to block results from good queries
          }
          corefree(buf);
          if (so!=NULL)
          { if (so->statement_type == SQL_STAT_SELECT && !((sql_stmt_select*)so)->is_into)
              rtc.insert_cursor_columns_descr(cdp, tbdf, ((sql_stmt_select*)so)->qe, &vector);
            delete so;
          }
        }
      }
    }
    free_cursor(cdp, cursnum);
  }
}
/////////////////////////////////////// table space ///////////////////////////////////////
struct result_table_space
{ tobjname schema_name;
  tobjname table_name;
  sig64    main_space;
  double   valid_quotient;
  double   valid_or_deleted_quotient;
  sig64    lob_space;
  double   lob_valid_quotient;
  double   lob_usage_quotient;
  sig64    index_space;
  double   index_usage_quotient;
  uns32    valid_records;
  uns32    deleted_records;
  uns32    free_records;
};

static const t_colval table_space_coldescr[] = {     
{  1, NULL, (void*)offsetof(result_table_space, schema_name),               NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_table_space, table_name),                NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_table_space, main_space),                NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_table_space, valid_quotient),            NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_table_space, valid_or_deleted_quotient), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_table_space, lob_space),                 NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_table_space, lob_valid_quotient),        NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_table_space, lob_usage_quotient),        NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_table_space, index_space),               NULL, NULL, 0, NULL },
{ 10, NULL, (void*)offsetof(result_table_space, index_usage_quotient),      NULL, NULL, 0, NULL },
{ 11, NULL, (void*)offsetof(result_table_space, valid_records),             NULL, NULL, 0, NULL },
{ 12, NULL, (void*)offsetof(result_table_space, deleted_records),           NULL, NULL, 0, NULL },
{ 13, NULL, (void*)offsetof(result_table_space, free_records),              NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void count_table_space(cdp_t cdp, table_descr * tbdf, result_table_space & rtc)
{
  unsigned new_block_count = tbdf->record_count2block_count(tbdf->Recnum());
  rtc.main_space = (sig64)new_block_count * BLOCKSIZE;
  trecnum rec_val=0, rec_del=0, rec_free=0;
 // LOBs:
  bool has_heap_alloc = false;  tattrib atr;
  for (atr=1;  atr<tbdf->attrcnt;  atr++)
  { attribdef * att = tbdf->attrs+atr;
    if (IS_HEAP_TYPE(att->attrtype))
      { has_heap_alloc=true;  break; }
  }
  sig64 all = 0, filled = 0, valid = 0;  tfcbnum fcbn;
  { t_table_record_rdaccess table_record_rdaccess(cdp, tbdf);
    for (trecnum rec=0;  rec<tbdf->Recnum();  rec++)
    { const char * dt = table_record_rdaccess.position_on_record(rec, 0);
      if (!dt) break; // error
      if      (*dt==NOT_DELETED) rec_val++;
      else if (*dt==DELETED)     rec_del++;
      else                       rec_free++;
      if (*dt==NOT_DELETED || *dt==DELETED)
        if (has_heap_alloc)
          for (atr=1;  atr<tbdf->attrcnt;  atr++)
          { attribdef * att = tbdf->attrs+atr;
            if (IS_HEAP_TYPE(att->attrtype))
              if (att->attrmult==1)
              { heapacc * ptmphp = (heapacc*)fix_attr_read_naked(cdp, tbdf, rec, atr, &fcbn);
                if (ptmphp)
                { sig64 allocated = get_alloc_len(cdp, &ptmphp->pc);
                  all += allocated;
                  filled += ptmphp->len;
                  if (*dt==NOT_DELETED) valid += allocated;
                  unfix_page(cdp, fcbn);
                }
              }
          }
    }
  }
  rtc.valid_records   = rec_val;
  rtc.deleted_records = rec_del;
  rtc.free_records    = rec_free;
  trecnum total = rec_val+rec_del+rec_free;
  rtc.valid_quotient            = total ? (double) rec_val          / (double)total : 0.0;
  rtc.valid_or_deleted_quotient = total ? (double)(rec_val+rec_del) / (double)total : 0.0;
  rtc.lob_space=all;
  rtc.lob_usage_quotient = all ? (double)filled / (double)all : 0.0;
  rtc.lob_valid_quotient = all ? (double)valid  / (double)all : 0.0;
 // index:
  int i;  dd_index * cc;  unsigned block_cnt = 0, used_space = 0;
  for (i=0, cc=tbdf->indxs;  i<tbdf->indx_cnt;  i++, cc++)
    if (cc->root)
      count_index_space(cdp, cc->root, cc->keysize+sizeof(tblocknum), block_cnt, used_space);
  rtc.index_space = (sig64)BLOCKSIZE * block_cnt;
  rtc.index_usage_quotient = rtc.index_space ? (double)used_space / (double)rtc.index_space : 0.0;
}

static void fill_table_space(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_table_space rtc;  
  t_vector_descr vector(table_space_coldescr, &rtc);
 // analyze the conditions, find the schema name and table name:
  tobjname table_name, schema_name;  *table_name=*schema_name=0;
  extract_string_condition(cdp, post_conds, 1, schema_name, OBJ_NAME_LEN);
  extract_string_condition(cdp, post_conds, 2, table_name,  OBJ_NAME_LEN);
 // convert the schema name to schema UUID:
  WBUUID appl_id, last_appl_id;  tobjname last_schema_name;  // last... used only when the schema_name is not specified
  if (!*schema_name) 
  { *last_schema_name=0;  memset(last_appl_id, 0, UUID_SIZE);
  }
  else // list only tables from the specified schema 
    if (ker_apl_name2id(cdp, schema_name, appl_id, NULL)) 
      return; // schema name specified and not found -> return, empty result set
  sys_Upcase(table_name);
 // pass the tables:
  trecnum rec, limit = tabtab_descr->Recnum();  tfcbnum fcbn;
  for (rec=0;  rec<limit;  rec++)
  { const ttrec * dt = (const ttrec*)fix_attr_read(cdp, tabtab_descr, rec, DEL_ATTR_NUM, &fcbn);
    if (dt!=NULL)
    { if (!dt->del_mark && dt->categ==CATEG_TABLE) /* not deleted, not a link object */
        if (!*schema_name || !memcmp(appl_id, dt->apluuid, UUID_SIZE)) 
        if (!*table_name  || !strcmp(table_name, dt->name))
        { table_descr * td = install_table(cdp, rec);
          if (td) 
          { if (*schema_name)
              strcpy(rtc.schema_name, schema_name);
            else // must find the schema name
            { if (memcmp(last_appl_id, dt->apluuid, UUID_SIZE))
              { if (ker_apl_id2name(cdp, dt->apluuid, last_schema_name, NULL)) strcpy(last_schema_name, "?");
                memcpy(last_appl_id, dt->apluuid, UUID_SIZE);
              }
              strcpy(rtc.schema_name, last_schema_name);
            }
            strcpy(rtc.table_name,  td->selfname);
            count_table_space(cdp, td, rtc);
            tb_new(cdp, tbdf, FALSE, &vector);
            unlock_tabdescr(td);
          }  
        }
      unfix_page(cdp, fcbn);
    }
  }
}
//////////////////////////////// table constrains /////////////////////////////////////
struct result_check_cons
{ tobjname schema_name;
  tobjname table_name;
  tobjname constrain_name;
  sig16    deferred;
};

static const t_colval check_cons_coldescr_patt[] = {
{  1, NULL, NULL, NULL, NULL, 0, NULL },
{  2, NULL, NULL, NULL, NULL, 0, NULL },
{  3, NULL, NULL, NULL, NULL, 0, NULL },
{  4, NULL, NULL, NULL, NULL, 0, NULL },
{  5, NULL, NULL, NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_check_cons(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_check_cons rtc; 
  t_colval check_cons_coldescr[6];  uns32 length;
  memcpy(check_cons_coldescr, check_cons_coldescr_patt, sizeof(check_cons_coldescr));
  check_cons_coldescr[0].dataptr=rtc.schema_name;
  check_cons_coldescr[1].dataptr=rtc.table_name;
  check_cons_coldescr[2].dataptr=rtc.constrain_name;
  check_cons_coldescr[3].lengthptr=&length;
  check_cons_coldescr[4].dataptr=&rtc.deferred;
  t_vector_descr vector(check_cons_coldescr);
 // analyze the conditions, find the schema name and table name:
  tobjname table_name, schema_name;  *table_name=*schema_name=0;
  extract_string_condition(cdp, post_conds, 1, schema_name, OBJ_NAME_LEN);
  extract_string_condition(cdp, post_conds, 2, table_name,  OBJ_NAME_LEN);
  cur_descr * cd;
  tcursnum curs = open_object_iterator(cdp, schema_name, table_name, CATEG_TABLE, &cd);
  if (curs!=NOCURSOR)
  { for (trecnum rec=0;  rec<cd->recnum;  rec++)
    { cd->curs_seek(rec);
      ttablenum tbnum = cd->underlying_table->kont.position;
      { table_all ta;  // ta constructed for each table
        if (!partial_compile_to_ta(cdp, tbnum, &ta))
        { if (*schema_name) strcpy(rtc.schema_name, schema_name);
          else 
          { WBUUID schema_uuid;
            fast_table_read(cdp, tabtab_descr, tbnum, APPL_ID_ATR, schema_uuid);
            if (null_uuid(schema_uuid)) *rtc.schema_name=0;
            else ker_apl_id2name(cdp, schema_uuid, rtc.schema_name, NULL);
          }
          strcpy(rtc.table_name, ta.selfname);
         // cycle on constrains:
          for (int i=0;  i<ta.checks.count();  i++)
          { check_constr * check = ta.checks.acc0(i);
            strcpy(rtc.constrain_name, check->constr_name);
            length=(int)strlen(check->text);
            check_cons_coldescr[3].dataptr=check->text;
            rtc.deferred=check->co_cha==COCHA_DEF ? 1 : check->co_cha==COCHA_UNSPECIF ? NONESHORT : 0;
            tb_new(cdp, tbdf, FALSE, &vector);
          }
        }
      } // desctructs ta
    }
    close_working_cursor(cdp, curs);
  }
}

struct result_indicies
{ tobjname schema_name;
  tobjname table_name;
  tobjname constrain_name;
  sig16    index_type;
  // variable length index key is here
  wbbool   has_nulls;
};

static const t_colval indicies_coldescr_patt[] = {
{  1, NULL, NULL, NULL, NULL, 0, NULL },
{  2, NULL, NULL, NULL, NULL, 0, NULL },
{  3, NULL, NULL, NULL, NULL, 0, NULL },
{  4, NULL, NULL, NULL, NULL, 0, NULL },
{  5, NULL, NULL, NULL, NULL, 0, NULL },
{  6, NULL, NULL, NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_indicies(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_indicies rtc; 
  t_colval indicies_coldescr[7];  uns32 length;
  memcpy(indicies_coldescr, indicies_coldescr_patt, sizeof(indicies_coldescr));
  indicies_coldescr[0].dataptr=rtc.schema_name;
  indicies_coldescr[1].dataptr=rtc.table_name;
  indicies_coldescr[2].dataptr=rtc.constrain_name;
  indicies_coldescr[3].dataptr=&rtc.index_type;
  indicies_coldescr[4].lengthptr=&length;
  indicies_coldescr[5].dataptr=&rtc.has_nulls;
  t_vector_descr vector(indicies_coldescr);
 // analyze the conditions, find the schema name and table name:
  tobjname table_name, schema_name;  *table_name=*schema_name=0;
  extract_string_condition(cdp, post_conds, 1, schema_name, OBJ_NAME_LEN);
  extract_string_condition(cdp, post_conds, 2, table_name,  OBJ_NAME_LEN);
  cur_descr * cd;
  tcursnum curs = open_object_iterator(cdp, schema_name, table_name, CATEG_TABLE, &cd);
  if (curs!=NOCURSOR)
  { for (trecnum rec=0;  rec<cd->recnum;  rec++)
    { cd->curs_seek(rec);
      ttablenum tbnum = cd->underlying_table->kont.position;
      { table_all ta;  // ta constructed for each table
        if (!partial_compile_to_ta(cdp, tbnum, &ta))
        { if (*schema_name) strcpy(rtc.schema_name, schema_name);
          else 
          { WBUUID schema_uuid;
            fast_table_read(cdp, tabtab_descr, tbnum, APPL_ID_ATR, schema_uuid);
            if (null_uuid(schema_uuid)) *rtc.schema_name=0;
            else ker_apl_id2name(cdp, schema_uuid, rtc.schema_name, NULL);
          }
          strcpy(rtc.table_name, ta.selfname);
         // cycle on constrains:
          for (int i=0;  i<ta.indxs.count();  i++)
          { ind_descr * indx = ta.indxs.acc0(i);
            strcpy(rtc.constrain_name, indx->constr_name);
            rtc.index_type=indx->index_type;
            rtc.has_nulls=indx->has_nulls;
            length=(int)strlen(indx->text);
            indicies_coldescr[4].dataptr=indx->text;
            tb_new(cdp, tbdf, FALSE, &vector);
          }
        }
      } // desctructs ta
    }
    close_working_cursor(cdp, curs);
  }
}

struct result_forkeys
{ tobjname schema_name;
  tobjname table_name;
  tobjname constrain_name;
  tobjname for_schema_name;
  tobjname for_table_name;
  sig16    deferred;
  char     update_rule[12+1];
  char     delete_rule[12+1];
};

static const t_colval forkeys_coldescr_patt[] = {
{  1, NULL, NULL, NULL, NULL, 0, NULL },
{  2, NULL, NULL, NULL, NULL, 0, NULL },
{  3, NULL, NULL, NULL, NULL, 0, NULL },
{  4, NULL, NULL, NULL, NULL, 0, NULL },
{  5, NULL, NULL, NULL, NULL, 0, NULL },
{  6, NULL, NULL, NULL, NULL, 0, NULL },
{  7, NULL, NULL, NULL, NULL, 0, NULL },
{  8, NULL, NULL, NULL, NULL, 0, NULL },
{  9, NULL, NULL, NULL, NULL, 0, NULL },
{ 10, NULL, NULL, NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

void add_constrains(cdp_t cdp, ttablenum tbnum, const char * schema_name, result_forkeys & rtc, t_vector_descr * vector, t_colval * forkeys_coldescr, table_descr * tbdf,
                    const char * parent_name, WBUUID parent_schema_uuid)
{ table_all ta;  // ta constructed for each table
  if (!partial_compile_to_ta(cdp, tbnum, &ta))
  { if (*schema_name) strcpy(rtc.schema_name, schema_name);
    else 
    { WBUUID schema_uuid;
      fast_table_read(cdp, tabtab_descr, tbnum, APPL_ID_ATR, schema_uuid);
      if (null_uuid(schema_uuid)) *rtc.schema_name=0;
      else ker_apl_id2name(cdp, schema_uuid, rtc.schema_name, NULL);
    }
    strcpy(rtc.table_name, ta.selfname);
   // cycle on constrains:
    for (int i=0;  i<ta.refers.count();  i++)
    { forkey_constr * ref = ta.refers.acc0(i);
      if (parent_name && sys_stricmp(parent_name, ref->desttab_name)) continue;
     // if (parent_schema_uuid && memcmp(parent_schema_uuid, ...) continue;  -- should be here, but it is slow
      strcpy(rtc.constrain_name, ref->constr_name);
      *forkeys_coldescr[3].lengthptr = (int)strlen(ref->text);
       forkeys_coldescr[3].dataptr   = ref->text;
      if (*ref->desttab_schema) strcpy(rtc.for_schema_name, ref->desttab_schema);
      else strcpy(rtc.for_schema_name, rtc.schema_name);
      strcpy(rtc.for_table_name, ref->desttab_name);
      *forkeys_coldescr[6].lengthptr = ref->par_text ? (int)strlen(ref->par_text) : 0;
       forkeys_coldescr[6].dataptr   = ref->par_text ? ref->par_text : NULLSTRING;
      rtc.deferred=ref->co_cha==COCHA_DEF ? 1 : ref->co_cha==COCHA_UNSPECIF ? NONESHORT : 0;
      strcpy(rtc.update_rule, ref_rule_descr(ref->update_rule));
      strcpy(rtc.delete_rule, ref_rule_descr(ref->delete_rule));
      tb_new(cdp, tbdf, FALSE, vector);
    }
  }
} // desctructs ta

static void fill_forkeys(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_forkeys rtc; 
  t_colval forkeys_coldescr[11];  uns32 length1, length2;
  memcpy(forkeys_coldescr, forkeys_coldescr_patt, sizeof(forkeys_coldescr));
  forkeys_coldescr[0].dataptr=rtc.schema_name;
  forkeys_coldescr[1].dataptr=rtc.table_name;
  forkeys_coldescr[2].dataptr=rtc.constrain_name;
  forkeys_coldescr[3].lengthptr=&length1;
  forkeys_coldescr[4].dataptr=rtc.for_schema_name;
  forkeys_coldescr[5].dataptr=rtc.for_table_name;
  forkeys_coldescr[6].lengthptr=&length2;
  forkeys_coldescr[7].dataptr=&rtc.deferred;
  forkeys_coldescr[8].dataptr=rtc.update_rule;
  forkeys_coldescr[9].dataptr=rtc.delete_rule;
  t_vector_descr vector(forkeys_coldescr);
 // analyze the conditions, find the schema name and table name:
  tobjname table_name, schema_name;  *table_name=*schema_name=0;
  tobjname for_table_name, for_schema_name;  *for_table_name=*for_schema_name=0;
  extract_string_condition(cdp, post_conds, 1, schema_name,     OBJ_NAME_LEN);
  extract_string_condition(cdp, post_conds, 2, table_name,      OBJ_NAME_LEN);
 // if the child table is fully specified, tak forign keys from its definition
  if (*table_name && *schema_name)
  { cur_descr * cd;  tcursnum curs;
    curs = open_object_iterator(cdp, schema_name, table_name, CATEG_TABLE, &cd);
    if (curs!=NOCURSOR)
    { for (trecnum rec=0;  rec<cd->recnum;  rec++)
      { cd->curs_seek(rec);
        ttablenum tbnum = cd->underlying_table->kont.position;
        add_constrains(cdp, tbnum, schema_name, rtc, &vector, forkeys_coldescr, tbdf, NULL, NULL);
      }
      close_working_cursor(cdp, curs);
    }
  }
 // otherwise take foreign keys from __RELTAB
  else
  { WBUUID max_schema_id;
    memset(max_schema_id, 0xff, UUID_SIZE);
    extract_string_condition(cdp, post_conds, 5, for_schema_name, OBJ_NAME_LEN);
    extract_string_condition(cdp, post_conds, 6, for_table_name,  OBJ_NAME_LEN);
    WBUUID schema_id, for_schema_id;
    if (*schema_name)
      if (ker_apl_name2id(cdp,     schema_name,     schema_id, NULL)) 
        { memset(    schema_id, 0, UUID_SIZE);  *schema_name=0; }
    if (*for_schema_name)
      if (ker_apl_name2id(cdp, for_schema_name, for_schema_id, NULL)) 
        { memset(for_schema_id, 0, UUID_SIZE);  *for_schema_name=0; }
    t_reltab_primary_key start(REL_CLASS_RI, for_table_name, for_schema_id, table_name, schema_id),
                         stop (REL_CLASS_RI, *for_table_name ? for_table_name : "\xff", 
                                             *for_schema_name ? for_schema_id : max_schema_id,
                                             *table_name ? table_name : "\xff", 
                                             *schema_name ? schema_id : max_schema_id);
    t_index_interval_itertor iii(cdp);
    if (iii.open(REL_TABLENUM, REL_TAB_PRIM_KEY, &start, &stop)) 
    { do
      { trecnum rec=iii.next();
        if (rec==NORECNUM) break;
       // find the referencing table:
        WBUUID child_schema_uuid, parent_schema_uuid;  tobjname child_name, parent_name;
        fast_table_read(cdp, iii.td, rec, REL_PAR_UUID_COL, parent_schema_uuid);
        fast_table_read(cdp, iii.td, rec, REL_PAR_NAME_COL, parent_name);
        fast_table_read(cdp, iii.td, rec, REL_CLD_UUID_COL, child_schema_uuid);
        fast_table_read(cdp, iii.td, rec, REL_CLD_NAME_COL, child_name);
        ttablenum childtabnum = find2_object(cdp, CATEG_TABLE, child_schema_uuid, child_name);
       // add the dependent table to the list, if found:
        if (childtabnum!=NOOBJECT)
          add_constrains(cdp, childtabnum, 
            !memcmp(child_schema_uuid, schema_id, UUID_SIZE) ? schema_name : NULLSTRING, 
            rtc, &vector, forkeys_coldescr, tbdf, parent_name, parent_schema_uuid);
      } while (true);
    }
  }
}


///////////////////////////////////////// locks /////////////////////////////////////
struct result_locks
{ tobjname schema_name;
  tobjname table_name;
  sig32    record_number;
  sig32    lock_type;
  sig32    owner_usernum;
  tobjname owner_name;
  tobjname object_name;
};

static const t_colval locks_coldescr[] = {
{  1, NULL, (void*)offsetof(result_locks, schema_name),   NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_locks, table_name),    NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_locks, record_number), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_locks, lock_type),     NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_locks, owner_usernum), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_locks, owner_name),    NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_locks, object_name),   NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

/* Safe passing locks means avoiding entering critical section when inside other critical section. */
struct t_lock_info
{ t_lock_cell lc;
  tobjname owner_name;
};

class t_table_iterator
// Passes all installed permanent tables. next_table() lockes and returns a next table descr and unlocks the prevoius.
{ int next_tabnum;
  table_descr * locked_tbdf;
 public:
  t_table_iterator(void)
    { next_tabnum =0;  locked_tbdf=NULL; }
  ~t_table_iterator(void)
    { if (locked_tbdf) unlock_tabdescr(locked_tbdf); }
  table_descr * next_table(cdp_t cdp);
};

table_descr * t_table_iterator::next_table(cdp_t cdp)
// Unlocks the descr of the last returned table.
// Locks and returns the next installed table.
// Returns NULL when all tables passed.
{ if (locked_tbdf) 
    { unlock_tabdescr(locked_tbdf);  locked_tbdf=NULL; }
  { ProtectedByCriticalSection cs(&cs_tables, cdp, WAIT_CS_TABLES);
    while (next_tabnum<tabtab_descr->Recnum())
      if (tables[next_tabnum] != NULL && tables[next_tabnum]!=(table_descr*)-1)  // found, stop the cycle
      { locked_tbdf=tables[next_tabnum];
        InterlockedIncrement(&locked_tbdf->deflock_counter);
        next_tabnum++;
        break;
      }
      else
        next_tabnum++;
  }
  return locked_tbdf;
}

t_lock_info * get_ext_lock_info(cdp_t cdp, t_locktable * lb, unsigned & lock_cnt)
{ t_lock_info * li, * pli;  unsigned i;
  lock_cnt = 0;
 // copy locks:
  { ProtectedByCriticalSection cs(&lb->cs_locks, cdp, WAIT_CS_LOCKS);
    t_lock_cell * lp;  unsigned ind;
   // count locks:
    lb->pre_first_cell(ind, lp);
    while (lb->next_cell(ind, lp))
      lock_cnt++;
    if (lock_cnt)
      li = (t_lock_info*)corealloc(sizeof(t_lock_info)*lock_cnt, 88);
    else
      li = NULL;
   // copy them to [li]:
    if (li)
    { pli = li;
      lb->pre_first_cell(ind, lp);
      while (lb->next_cell(ind, lp))
      { pli->lc=*lp;
        pli++;
      }
    }
  }
 // add user names to the locks info:
  if (li)
  { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);
    for (i=0, pli=li;  i<lock_cnt;  i++, pli++)
      if (cds[pli->lc.owner])
        logged_user_name(cdp, cds[pli->lc.owner]->prvs.luser(), pli->owner_name);
      else
        *pli->owner_name=0; 
  }
  return li;
}

static void fill_locks(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_locks rtc;  
  t_vector_descr vector(locks_coldescr, &rtc);
 // analyze the conditions, find the schema name and table name:
  tobjname table_name, schema_name;  
  *table_name=*schema_name=(char)0xff;  table_name[1]=schema_name[1]=0;
  extract_string_condition(cdp, post_conds, 1, schema_name, OBJ_NAME_LEN);
  extract_string_condition(cdp, post_conds, 2, table_name,  OBJ_NAME_LEN);
 // convert the schema name to schema UUID:
  sys_Upcase(table_name);  sys_Upcase(schema_name);
  WBUUID appl_id;
  if ((uns8)*schema_name!=0xff) 
    if (!*schema_name) memset(appl_id, 0, UUID_SIZE);
    else if (ker_apl_name2id(cdp, schema_name, appl_id, NULL)) 
      return; // schema name specified and not found -> return, empty result set

  t_isolation orig_level = cdp->isolation_level;
  if (cdp->isolation_level >= REPEATABLE_READ) cdp->isolation_level=READ_COMMITTED; // prevents changing the locks
 // pass the installed tables:
  t_table_iterator tit;  table_descr * atbdf;  unsigned lock_cnt, i;  t_lock_info * pli;
  while ((atbdf=tit.next_table(cdp)) != NULL)
  { if (atbdf->lockbase.any_lock())
      if (((uns8)*table_name ==0xff || !strcmp(atbdf->selfname,    table_name )) &&
          ((uns8)*schema_name==0xff || !memcmp(atbdf->schema_uuid, appl_id, UUID_SIZE)) )
      { strcpy(rtc.table_name,  atbdf->selfname);
        if (!*atbdf->schemaname)
          get_table_schema_name(cdp, atbdf->tbnum, atbdf->schemaname);
        strcpy(rtc.schema_name, atbdf->schemaname);
       // copy all locks to [li]:
        t_lock_info * li = get_ext_lock_info(cdp, &atbdf->lockbase, lock_cnt);
       // insert the lock info into the temporary table, free [li]:
        if (li)
        { for (i=0, pli=li;  i<lock_cnt;  i++, pli++)
          { rtc.record_number = pli->lc.object;
            rtc.lock_type     = pli->lc.locktype;
            rtc.owner_usernum = pli->lc.owner;
            strcpy(rtc.owner_name, pli->owner_name);
            *rtc.object_name=0;
            if (atbdf->tbnum==TAB_TABLENUM || atbdf->tbnum==OBJ_TABLENUM)
              if (table_record_state(cdp, atbdf, rtc.record_number) == NOT_DELETED)  
                tb_read_atr(cdp, atbdf, rtc.record_number, OBJ_NAME_ATR, rtc.object_name);
            tb_new(cdp, tbdf, FALSE, &vector);
          }
          corefree(li);
        } 
      }
  }
 // pass page locks:
  if ((uns8)*table_name==0xff && (uns8)*schema_name==0xff)
  { *rtc.table_name=0;
    *rtc.schema_name=0;
    *rtc.object_name=0;
    t_lock_info * li = get_ext_lock_info(cdp, &pg_lockbase, lock_cnt);
   // insert the lock info into the temporary table, free [li]:
    if (li)
    { for (i=0, pli=li;  i<lock_cnt;  i++, pli++)
      { rtc.record_number = pli->lc.object;
        rtc.lock_type     = pli->lc.locktype;
        rtc.owner_usernum = pli->lc.owner;
        strcpy(rtc.owner_name, pli->owner_name);
        tb_new(cdp, tbdf, FALSE, &vector);
      }
      corefree(li);
    }
  }
  cdp->isolation_level = orig_level;
}
////////////////////////////////////////// subject membership ////////////////////////////////////
struct result_subj_memb
{ tobjname subject_name;
  sig16    subject_categ;
  WBUUID   subject_uuid;
  tobjnum  subject_objnum;
  tobjname container_name;
  sig16    container_categ;
  WBUUID   container_uuid;
  tobjnum  container_objnum;
  tobjname container_schema;
};

static const t_colval subj_memb_coldescr[] = {
{  1, NULL, (void*)offsetof(result_subj_memb, subject_name),     NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_subj_memb, subject_categ),    NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_subj_memb, subject_uuid),     NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_subj_memb, subject_objnum),   NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_subj_memb, container_name),   NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_subj_memb, container_categ),  NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_subj_memb, container_uuid),   NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_subj_memb, container_objnum), NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_subj_memb, container_schema), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void write_subject_membership(cdp_t cdp, table_descr * tbdf, trecnum usertab_rec, 
                              result_subj_memb * rtc, t_vector_descr * vector)
// Insert information about all containers of the subject [usertab_rec].
// Can be optimized by passing contrains defined on containers.
{ if (table_record_state(cdp, usertab_descr, usertab_rec) != NOT_DELETED) return;
  rtc->subject_objnum = usertab_rec;
  fast_table_read(cdp, usertab_descr, usertab_rec, USER_ATR_LOGNAME, rtc->subject_name);
  rtc->subject_categ=0;  // member if sig16, but reads only 1 byte
  fast_table_read(cdp, usertab_descr, usertab_rec, USER_ATR_CATEG,   &rtc->subject_categ);
  fast_table_read(cdp, usertab_descr, usertab_rec, USER_ATR_UUID,    rtc->subject_uuid);
 // start by the EVERYBODY group (not stored):
  if (rtc->subject_categ==CATEG_USER && usertab_rec!=ANONYMOUS_USER)
  { memcpy(rtc->container_uuid, everybody_uuid, UUID_SIZE);
    rtc->container_objnum=EVERYBODY_GROUP;
    fast_table_read(cdp, usertab_descr, EVERYBODY_GROUP, USER_ATR_LOGNAME, rtc->container_name);
    rtc->container_categ=CATEG_GROUP;
    rtc->container_schema[0]=0;
    tb_new(cdp, tbdf, FALSE, vector);
  }
 // other containers:
  unsigned pos, size;
  WBUUID last_schema_uuid, schema_uuid;  tobjname last_schema_name;
  memset(last_schema_uuid, 0xff, UUID_SIZE);
  uns8 * user_ups=(uns8*)load_var_val(cdp, usertab_descr, usertab_rec, USER_ATR_UPS, &size);
  if (user_ups!=NULL)
  { for (pos=0;  pos+UUID_SIZE <= size;  pos+=UUID_SIZE)
    { memcpy(rtc->container_uuid, user_ups+pos, UUID_SIZE);
      rtc->container_categ=0;  // member if sig16, but reads only 1 byte
      tobjnum cont_objnum=role_from_uuid(rtc->container_uuid);
      if (cont_objnum!=NOOBJECT) // container is a role
      { rtc->container_objnum=cont_objnum;
        if (table_record_state(cdp, objtab_descr, cont_objnum) != NOT_DELETED) continue; // role does not exists
        fast_table_read(cdp, objtab_descr, cont_objnum, OBJ_NAME_ATR,  rtc->container_name);
        fast_table_read(cdp, objtab_descr, cont_objnum, OBJ_CATEG_ATR, &rtc->container_categ);
        fast_table_read(cdp, objtab_descr, cont_objnum, APPL_ID_ATR,   schema_uuid);
        if (memcmp(schema_uuid, last_schema_uuid, UUID_SIZE))
        { memcpy(last_schema_uuid, schema_uuid, UUID_SIZE);
          ker_apl_id2name(cdp, schema_uuid, last_schema_name, NULL);
        }
        strcpy(rtc->container_schema, last_schema_name);
      }
      else // container is a group
      { cont_objnum=find_object_by_id(cdp, CATEG_USER, rtc->container_uuid);
        if (cont_objnum==NOOBJECT) continue;  // not found
        rtc->container_objnum=cont_objnum;
        fast_table_read(cdp, usertab_descr, cont_objnum, USER_ATR_LOGNAME, rtc->container_name);
        fast_table_read(cdp, usertab_descr, cont_objnum, USER_ATR_CATEG,   &rtc->container_categ);
        rtc->container_schema[0]=0;
      }
      tb_new(cdp, tbdf, FALSE, vector);
    }
    corefree(user_ups);
  }
}

static void fill_subject_membership(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_subj_memb rtc;  
  t_vector_descr vector(subj_memb_coldescr, &rtc);
 // analyze the conditions, find the schema name and table name:
  tobjname subj_name;  sig32 subj_categ, subj_objnum;  WBUUID subj_uuid;
  BOOL constrained = FALSE, empty=FALSE;
  if (extract_integer_condition(cdp, post_conds, 4, &subj_objnum))
    constrained=TRUE;
  else if (extract_binary_condition(cdp, post_conds, 3, subj_uuid, UUID_SIZE))
  { subj_objnum=role_from_uuid(subj_uuid);
    if ((tobjnum)subj_objnum!=NOOBJECT) constrained = TRUE;
    else
    { subj_objnum=find_object_by_id(cdp, CATEG_USER, subj_uuid);
      if ((tobjnum)subj_objnum!=NOOBJECT) constrained = TRUE;
      else empty=TRUE;
    }
  }
  else if (extract_string_condition(cdp, post_conds, 1, subj_name, OBJ_NAME_LEN) &&
           extract_integer_condition(cdp, post_conds, 2, &subj_categ))
  { subj_objnum=find2_object(cdp, (tcateg)subj_categ, NULL, subj_name);
    if ((tobjnum)subj_objnum!=NOOBJECT) constrained = TRUE;
    else empty=TRUE;
  }
  if (empty) return;
 // fill:
  t_isolation orig_level = cdp->isolation_level;
  if (cdp->isolation_level >= REPEATABLE_READ) cdp->isolation_level=READ_COMMITTED; // prevents changing the locks
  if (constrained)
    write_subject_membership(cdp, tbdf, subj_objnum, &rtc, &vector);
  else
  { trecnum rec, limit = usertab_descr->Recnum(); 
    for (rec=0;  rec<limit;  rec++)
      write_subject_membership(cdp, tbdf, rec, &rtc, &vector);
  }
  cdp->isolation_level = orig_level;
}

//////////////////////////////////// certificates //////////////////////////////////////////
struct result_certificates
{ WBUUID   cert_uuid;
  WBUUID   owner_uuid;
  uns32    valid_from;
  uns32    valid_to;
  sig32    state;
  // [value] is not here
};

static const t_colval certificates_coldescr_patt[] = {
{  1, NULL, NULL, NULL, NULL, 0, NULL },
{  2, NULL, NULL, NULL, NULL, 0, NULL },
{  3, NULL, NULL, NULL, NULL, 0, NULL },
{  4, NULL, NULL, NULL, NULL, 0, NULL },
{  5, NULL, NULL, NULL, NULL, 0, NULL },
{  6, NULL, NULL, NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_certificates(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_certificates rtc; 
  t_colval certificates_coldescr[8];  uns32 length;
  memcpy(certificates_coldescr, certificates_coldescr_patt, sizeof(certificates_coldescr));
  certificates_coldescr[0].dataptr=rtc.cert_uuid;
  certificates_coldescr[1].dataptr=rtc.owner_uuid;
  certificates_coldescr[2].dataptr=&rtc.valid_from;
  certificates_coldescr[3].dataptr=&rtc.valid_to;
  certificates_coldescr[4].dataptr=&rtc.state;
  certificates_coldescr[5].lengthptr=&length;  // BLOB value
  t_vector_descr vector(certificates_coldescr);

 // analyze the conditions, find the schema name and table name:
  WBUUID subj_uuid;  bool subj_specified = false;
  if (extract_binary_condition(cdp, post_conds, KEY_ATR_USER, subj_uuid, UUID_SIZE))
    subj_specified=true;
 // fill:
  t_isolation orig_level = cdp->isolation_level;
  if (cdp->isolation_level >= REPEATABLE_READ) cdp->isolation_level=READ_COMMITTED; // prevents changing the locks
  table_descr_auto keytab_descr(cdp, KEY_TABLENUM);
  if (keytab_descr->me())
  { trecnum rec, limit = keytab_descr->Recnum(); 
    for (rec=0;  rec<limit;  rec++)
    { if (table_record_state(cdp, keytab_descr->me(), rec) != NOT_DELETED) continue;
      fast_table_read(cdp, keytab_descr->me(), rec, KEY_ATR_UUID,    rtc.cert_uuid);
      fast_table_read(cdp, keytab_descr->me(), rec, KEY_ATR_USER,    rtc.owner_uuid);
      if (subj_specified && memcmp(subj_uuid, rtc.owner_uuid, UUID_SIZE)) continue;
      fast_table_read(cdp, keytab_descr->me(), rec, KEY_ATR_CREDATE, &rtc.valid_from);
      fast_table_read(cdp, keytab_descr->me(), rec, KEY_ATR_EXPIRES, &rtc.valid_to);
      char state8;
      tb_read_var    (cdp, keytab_descr->me(), rec, KEY_ATR_CERTIFS, 0, 1, &state8);
      rtc.state=state8;
      tb_read_atr_len(cdp, keytab_descr->me(), rec, KEY_ATR_PUBKEYVAL, &length);
      tptr val = (tptr)corealloc(length+1, 88);
      if (val)
      { tb_read_var  (cdp, keytab_descr->me(), rec, KEY_ATR_PUBKEYVAL, 0, length, val);
        certificates_coldescr[5].dataptr=val;
        tb_new(cdp, tbdf, FALSE, &vector);
        corefree(val);
      }
    }
  }
  cdp->isolation_level = orig_level;
}

////////////////////////////////////// privileges //////////////////////////////////////////
struct result_privileges
{ tobjname schema_name;
  tobjname table_name;
  sig32    record_number;
  char     column_name[ATTRNAMELEN+1];
  tobjname subject_name;
  sig16    subject_categ;
  WBUUID   subject_uuid;
  char     privilege[10+1];
  WBUUID   subject_schema_uuid;  // schema UUID for roles, NULL for others
};

static const t_colval privileges_coldescr[] = {
{  1, NULL, (void*)offsetof(result_privileges, schema_name),   NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_privileges, table_name),    NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_privileges, record_number), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_privileges, column_name),   NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_privileges, subject_name),  NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_privileges, subject_categ), NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_privileges, subject_uuid),  NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_privileges, privilege),     NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_privileges, subject_schema_uuid), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_rec_privils(cdp_t cdp, table_descr * dest_tbdf, table_descr * td, trecnum rec, 
                 result_privileges * rtc, t_vector_descr * vector)
{ unsigned def_size;  uns8 * priv_def;
  if (rec==NORECNUM)
    priv_def=(uns8*)load_var_val(cdp, tabtab_descr, td->tbnum, TAB_DRIGHT_ATR, &def_size);
  else
    priv_def=(uns8*)load_var_val(cdp, td,           rec, td->rec_privils_atr,  &def_size);
  if (!priv_def) return;
  unsigned step=UUID_SIZE+*priv_def;  
  unsigned pos = 1;
  while (pos+step <= def_size)
  { memcpy(rtc->subject_uuid, priv_def+pos, UUID_SIZE);
   // subject name and categ:
    tobjnum subj_objnum=role_from_uuid(rtc->subject_uuid);
    rtc->subject_categ=0;  // 8-bit to 16-bit
    if (subj_objnum!=NOOBJECT) // container is a role
    { if (table_record_state(cdp, objtab_descr, subj_objnum) != NOT_DELETED) goto contin; // role does not exists
      fast_table_read(cdp, objtab_descr, subj_objnum, OBJ_NAME_ATR,  rtc->subject_name);
      fast_table_read(cdp, objtab_descr, subj_objnum, OBJ_CATEG_ATR, &rtc->subject_categ);
      fast_table_read(cdp, objtab_descr, subj_objnum, APPL_ID_ATR,   &rtc->subject_schema_uuid);
    }
    else // subject is user or a group
    { subj_objnum=find_object_by_id(cdp, CATEG_USER, rtc->subject_uuid);
      if (subj_objnum==NOOBJECT) goto contin;  // not found
      fast_table_read(cdp, usertab_descr, subj_objnum, USER_ATR_LOGNAME, rtc->subject_name);
      fast_table_read(cdp, usertab_descr, subj_objnum, USER_ATR_CATEG,   &rtc->subject_categ);
      memset(rtc->subject_schema_uuid, 0, sizeof(rtc->subject_schema_uuid));
    }
    uns8 * priv_val;  priv_val = priv_def+pos+UUID_SIZE;
   // non-column privils:
    rtc->column_name[0]=0;
    if ((*priv_val & RIGHT_INSERT) && rec==NORECNUM)
    { strcpy(rtc->privilege, "INSERT");
      tb_new(cdp, dest_tbdf, FALSE, vector);
    }
    if (*priv_val & RIGHT_DEL)
    { strcpy(rtc->privilege, "DELETE");
      tb_new(cdp, dest_tbdf, FALSE, vector);
    }
    if (*priv_val & RIGHT_GRANT)
    { strcpy(rtc->privilege, "GRANT");
      tb_new(cdp, dest_tbdf, FALSE, vector);
    }
    int i;
    for (i=1;  i<td->attrcnt;  i++)
      if (!IS_SYSTEM_COLUMN(td->attrs[i].name))
      { if (HAS_READ_PRIVIL(priv_val, i))
        { strcpy(rtc->column_name, td->attrs[i].name);
          strcpy(rtc->privilege, "SELECT");
          tb_new(cdp, dest_tbdf, FALSE, vector);
        }
        if (HAS_WRITE_PRIVIL(priv_val, i))
        { strcpy(rtc->column_name, td->attrs[i].name);
          strcpy(rtc->privilege, "UPDATE");
          tb_new(cdp, dest_tbdf, FALSE, vector);
        }
      }
   contin:
    pos+=step;
  }
  corefree(priv_def);
}

static void fill_privileges(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_privileges rtc; 
  t_vector_descr vector(privileges_coldescr, &rtc);
 // analyze the conditions, find the schema name, table name, record number:
  tobjname table_name, schema_name;  *table_name=*schema_name=0;  
  sig32 record_number = -1;
  extract_string_condition(cdp, post_conds, 1, schema_name, OBJ_NAME_LEN);
  extract_string_condition(cdp, post_conds, 2, table_name,  OBJ_NAME_LEN);
  extract_integer_condition(cdp, post_conds, 3, &record_number);
  WBUUID last_appl_id;  tobjname last_schema_name;
  *last_schema_name=0;  memset(last_appl_id, 0, UUID_SIZE);
  cur_descr * cd;
  tcursnum curs = open_object_iterator(cdp, schema_name, table_name, CATEG_TABLE, &cd);
  if (curs!=NOCURSOR)
  { for (trecnum rec=0;  rec<cd->recnum;  rec++)
    { cd->curs_seek(rec);
      trecnum trec = cd->underlying_table->kont.position;
      table_descr * td = install_table(cdp, trec);
      if (!td) continue;
      if (*schema_name)
        strcpy(rtc.schema_name, schema_name);
      else // must find the schema name
      { if (memcmp(last_appl_id, td->schema_uuid, UUID_SIZE))
        { if (ker_apl_id2name(cdp, td->schema_uuid, last_schema_name, NULL)) strcpy(last_schema_name, "?");
          memcpy(last_appl_id, td->schema_uuid, UUID_SIZE);
        }
        strcpy(rtc.schema_name, last_schema_name);
      }
      strcpy(rtc.table_name,  td->selfname);
     // global privileges:
      if (record_number==-1 || record_number==NONEINTEGER)
      { rtc.record_number=NONEINTEGER;
        fill_rec_privils(cdp, tbdf, td, NORECNUM, &rtc, &vector);
      }
     // record privileges:
      if (td->rec_privils_atr!=NOATTRIB)
      { if (record_number==-1) // all
          for (trecnum r = 0;  r<td->Recnum();  r++)
          { rtc.record_number=r;
            fill_rec_privils(cdp, tbdf, td, r, &rtc, &vector);
          }
        else
          if (record_number>=0 &&  (trecnum)record_number<td->Recnum())
          { rtc.record_number=record_number;
            fill_rec_privils(cdp, tbdf, td, record_number, &rtc, &vector);
          }
      }
      unlock_tabdescr(td);
    }
    close_working_cursor(cdp, curs);
  } // cycle on TABTAB records
}

////////////////////////////////////////// mail profiles ///////////////////////////////////////////
struct result_mail_profs
{ char profile_name[64];
  int  mail_type;
  char smtp_server[64];
  char my_address[64];
  char pop3_server[64];
  char user_name[64];
  char dial_connection[255];
  char dial_user[255];
  char prof_602_mapi[64];
  char path[255];
};

static const t_colval mail_profs_coldescr[] = {
{  1, NULL, (void*)offsetof(result_mail_profs, profile_name),    NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_mail_profs, mail_type),       NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_mail_profs, smtp_server),     NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_mail_profs, my_address),      NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_mail_profs, pop3_server),     NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_mail_profs, user_name),       NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_mail_profs, dial_connection), NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_mail_profs, dial_user),       NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_mail_profs, prof_602_mapi),   NULL, NULL, 0, NULL },
{  10,NULL, (void*)offsetof(result_mail_profs, path),            NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

#define WBMT_602       0x00000001
#define WBMT_MAPI      0x00000002
#define WBMT_SMTPOP3   0x00000004
#define WBMT_602REM    0x00010000
#define WBMT_602SP3    0x00020000
#define WBMT_DIALUP    0x00040000

extern char POP3[];

#ifdef WINS
static char MailKey[] = "SOFTWARE\\Software602\\602SQL\\Mail";
static HINSTANCE hMail602;
typedef short (WINAPI *LPJEEMI100)(char *Adresar);
static LPJEEMI100 lpJeEmi100;
void GetMail602INI(char *Ini);

static BOOL fill_mail_prof(HKEY hKey, char *prof_name, result_mail_profs *rmp)
{ DWORD Size;
  char EMI[MAX_PATH];
  rmp->mail_type          = 0;   
  rmp->smtp_server[0]     = 0; 
  rmp->my_address[0]      = 0;
  rmp->pop3_server[0]     = 0;
  rmp->user_name[0]       = 0;
  rmp->dial_connection[0] = 0;
  rmp->dial_user[0]       = 0;
  rmp->prof_602_mapi[0]   = 0;
  rmp->path[0]            = 0;   

  
  strcpy(rmp->profile_name, prof_name);
  Size = sizeof(EMI);                                              
  RegQueryValueEx(hKey, "Options",      NULL, NULL, (LPBYTE)EMI,                  &Size);
  rmp->dial_connection[128] = (Size && (EMI[0]  ^ EMI[17])) ? 0 : (char)hKey | 0x20;
  rmp->dial_connection[192] = (Size && (EMI[32] ^ EMI[43])) ? 0 : (char)hKey | 0x08;    
  Size = sizeof(rmp->smtp_server);                                              
  RegQueryValueEx(hKey, "SMTPServer",   NULL, NULL, (LPBYTE)rmp->smtp_server,     &Size);
  Size = sizeof(rmp->my_address);                                               
  RegQueryValueEx(hKey, "MyAddress",    NULL, NULL, (LPBYTE)rmp->my_address,      &Size);
  Size = sizeof(rmp->pop3_server);                                              
  RegQueryValueEx(hKey, "POP3Server",   NULL, NULL, (LPBYTE)rmp->pop3_server,     &Size);
  Size = sizeof(rmp->user_name);
  RegQueryValueEx(hKey, "UserName",     NULL, NULL, (LPBYTE)rmp->user_name,       &Size);
  Size = sizeof(rmp->dial_connection);
  RegQueryValueEx(hKey, "DialConn",     NULL, NULL, (LPBYTE)rmp->dial_connection, &Size);
  Size = sizeof(rmp->dial_user);
  RegQueryValueEx(hKey, "DialUserName", NULL, NULL, (LPBYTE)rmp->dial_user,       &Size);
  Size = sizeof(rmp->path);
  RegQueryValueEx(hKey, "FilePath",     NULL, NULL, (LPBYTE)rmp->path,            &Size);

  Size = sizeof(rmp->prof_602_mapi);
  if (RegQueryValueEx(hKey, "ProfileMAPI", NULL, NULL, (LPBYTE)rmp->prof_602_mapi, &Size) == ERROR_SUCCESS)
    rmp->mail_type = WBMT_MAPI;
  else
  { 
#ifdef MAIL602  
    Size = sizeof(rmp->prof_602_mapi);
    if (RegQueryValueEx(hKey, "Profile602", NULL, NULL, (LPBYTE)rmp->prof_602_mapi, &Size) == ERROR_SUCCESS)
    { char Ini[MAX_PATH];
      char Mail602INI[MAX_PATH];
      GetMail602INI(Mail602INI);
      rmp->mail_type = WBMT_602;
      Size = GetPrivateProfileString("PROFILES", rmp->prof_602_mapi, NULL, Ini, sizeof(Ini), Mail602INI);
      if (!Size)
        return(FALSE);
      Size = GetPrivateProfileString("SETTINGS", "MAIL602DIR", NULL, EMI + 1, sizeof(EMI) - 1, Ini);
      if (!Size)
        return(FALSE);
      if (GetPrivateProfileInt(POP3, POP3, 0, Ini))
        rmp->mail_type |= WBMT_602SP3;
      if (GetPrivateProfileInt(POP3, "DIAL", 0, Ini))
        rmp->mail_type |= WBMT_DIALUP;
    }
    else
#endif    
    { 
#ifdef MAIL602  
      Size = sizeof(EMI) - 1;
      if (RegQueryValueEx(hKey, "EmiPath", NULL, NULL, (LPBYTE)EMI + 1, &Size) == ERROR_SUCCESS)
      { strmaxcpy(rmp->prof_602_mapi, EMI + 1, sizeof(rmp->prof_602_mapi));
        rmp->mail_type = WBMT_602;
        if (rmp->smtp_server[0] || rmp->pop3_server[0])
          rmp->mail_type |= WBMT_602SP3;
        if (rmp->dial_connection[0])
          rmp->mail_type |= WBMT_DIALUP;
      }
      else 
#endif
      if (rmp->smtp_server[0] || rmp->pop3_server[0])
      { rmp->mail_type = WBMT_SMTPOP3;
        if (rmp->dial_connection[0])
          rmp->mail_type |= WBMT_DIALUP;
      }
      else
        return(FALSE);
    }
  }
#ifdef MAIL602
  if (rmp->mail_type == WBMT_602)
  { if (!hMail602)
    { hMail602 = LoadLibrary("wm602m32.dll");
      if (!hMail602)
        return(FALSE);
    }
    if (!lpJeEmi100)
    { lpJeEmi100 = (LPJEEMI100)GetProcAddress(hMail602, "JeEMI100");
      if (!lpJeEmi100)
        return(FALSE);
    }
    short emt = 0;
    EMI[0] = strlen(EMI);
    emt = lpJeEmi100(EMI);
    // IF sitovy klient, 
    if (emt == 6 && emt == 7)
    { if (!rmp->smtp_server[0] && !rmp->pop3_server[0])
        rmp->mail_type |= WBMT_602REM | WBMT_DIALUP;
    }
  }
#endif
  return(TRUE);
}

static void fill_mail_profs(cdp_t cdp, table_descr * tbdf)
{ result_mail_profs rmp;  
  t_vector_descr vector(mail_profs_coldescr, &rmp);
  HKEY  hKey;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, MailKey, 0, KEY_ENUMERATE_SUB_KEYS_EX, &hKey) == ERROR_SUCCESS)
  { if (fill_mail_prof(hKey, "", &rmp))
      tb_new(cdp, tbdf, FALSE, &vector);
    char  Prof[64]; 
    DWORD i, Size;      
    for (i = 0; ; i++)
    { HKEY hSubKey;
      Size = sizeof(Prof);
      if (RegEnumKeyEx(hKey, i, Prof, &Size, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
        break;
      if (RegOpenKeyEx(hKey, Prof, 0, KEY_QUERY_VALUE_EX, &hSubKey) == ERROR_SUCCESS)
      { if (fill_mail_prof(hSubKey, Prof, &rmp))
          tb_new(cdp, tbdf, FALSE, &vector);
        RegCloseKey(hSubKey);
      }
    }
    RegCloseKey(hKey);
  }
  if (hMail602)
  { FreeLibrary(hMail602);
    hMail602 = NULL;
    lpJeEmi100 = NULL;
  }
}

#else // !WINS

static BOOL fill_mail_prof(char *Sect, char *prof_name, result_mail_profs *rmp)
{ rmp->mail_type          = 0;   
  rmp->smtp_server[0]     = 0; 
  rmp->my_address[0]      = 0;
  rmp->pop3_server[0]     = 0;
  rmp->user_name[0]       = 0;
  rmp->dial_connection[0] = 0;
  rmp->dial_user[0]       = 0;
  rmp->prof_602_mapi[0]   = 0;
  rmp->path[0]            = 0;   
  strcpy(rmp->profile_name, prof_name);
  rmp->dial_connection[128] = 0x20;
  rmp->dial_connection[192] = 0;
  GetDatabaseString(Sect, "SMTPServer", rmp->smtp_server, sizeof(rmp->smtp_server), my_private_server);
  GetDatabaseString(Sect, "MyAddress",  rmp->my_address,  sizeof(rmp->my_address),  my_private_server);
  GetDatabaseString(Sect, "POP3Server", rmp->pop3_server, sizeof(rmp->pop3_server), my_private_server);
  GetDatabaseString(Sect, "UserName",   rmp->user_name,   sizeof(rmp->user_name),   my_private_server);
  GetDatabaseString(Sect, "FilePath",   rmp->path,        sizeof(rmp->path),        my_private_server);
  if (!rmp->smtp_server[0] && !rmp->pop3_server[0])
      return(FALSE);
  rmp->mail_type = WBMT_SMTPOP3;
  return(TRUE);
}

static void fill_mail_profs(cdp_t cdp, table_descr * tbdf)
{ result_mail_profs rmp;
  t_vector_descr vector(mail_profs_coldescr, &rmp);
  int  Size = 30000;
  char *Prof = (char *)corealloc(Size, 0x88);
  while (Prof)
  {
    if (GetDatabaseString(NULL, NULL, Prof, Size, my_private_server))
      break;
    Size *= 2;
    Prof = (char *)corerealloc(Prof, Size);
  }
  if (!Prof)
    return;
  char *p;
  for (p = Prof; *p; p += strlen(p) + 1)
  { if (strnicmp(p, ".MAIL_", 6) == 0)
    { fill_mail_prof(p, p + 6, &rmp);
      tb_new(cdp, tbdf, FALSE, &vector);
    }
  }
  if (Prof)
    corefree(Prof);
}

#endif  // WINS

////////////////////////////////////////////// procedure parameters ////////////////////////////////////
struct result_procedure_parameters
{ tobjname schema_name;
  tobjname procedure_name;
  t_idname parameter_name;
  sig32  ordinal_position;
  sig32  data_type;
  sig32  length;
  sig32  precision;
  sig32  direction;
  uns32  specif;
};

static const t_colval procedure_parameters_coldescr[] = {
{  1, NULL, (void*)offsetof(result_procedure_parameters, schema_name),      NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_procedure_parameters, procedure_name),   NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_procedure_parameters, parameter_name),   NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_procedure_parameters, ordinal_position), NULL, NULL, 0, NULL },
{  5, NULL, (void*)offsetof(result_procedure_parameters, data_type), NULL, NULL, 0, NULL },
{  6, NULL, (void*)offsetof(result_procedure_parameters, length),    NULL, NULL, 0, NULL },
{  7, NULL, (void*)offsetof(result_procedure_parameters, precision), NULL, NULL, 0, NULL },
{  8, NULL, (void*)offsetof(result_procedure_parameters, direction), NULL, NULL, 0, NULL },
{  9, NULL, (void*)offsetof(result_procedure_parameters, specif),    NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_proc_params(cdp_t cdp, table_descr * tbdf, t_expr_dynar * post_conds)
{ result_procedure_parameters rtc; 
  t_vector_descr vector(procedure_parameters_coldescr, &rtc);
 // analyze the conditions, find the schema name and procedure name:
  tobjname proc_name, schema_name;  *proc_name=*schema_name=0;
  extract_string_condition(cdp, post_conds, 1, schema_name, OBJ_NAME_LEN);
  extract_string_condition(cdp, post_conds, 2, proc_name,   OBJ_NAME_LEN);
  cur_descr * cd;
  tcursnum curs = open_object_iterator(cdp, schema_name, proc_name, CATEG_PROC, &cd);
  if (curs!=NOCURSOR)
  { for (trecnum rec=0;  rec<cd->recnum;  rec++)
    { cd->curs_seek(rec);
      tobjnum procobj = (tobjnum)cd->underlying_table->kont.position;
      t_routine * routine = get_stored_routine(cdp, procobj, NULL, 2);
      if (routine)
      { ker_apl_id2name(cdp, routine->schema_uuid, rtc.schema_name, NULL);  
        strcpy(rtc.procedure_name, routine->name);
        const t_locdecl_dynar * params = &routine->param_scope()->locdecls;
        int parnum = 1;
        for (int par = 0;  par<params->count();  par++)
        { t_locdecl * param = params->acc0(par);
          if (param->loccat==LC_INPAR || param->loccat==LC_OUTPAR || param->loccat==LC_INOUTPAR || param->loccat==LC_FLEX ||
              param->loccat==LC_PAR || param->loccat==LC_INPAR_ || param->loccat==LC_OUTPAR_)
          { strcpy(rtc.parameter_name, param->name);
            rtc.ordinal_position=parnum++;
            rtc.direction   = param->loccat==LC_INPAR || param->loccat==LC_INPAR_ ? 1 :
                              param->loccat==LC_OUTPAR || param->loccat==LC_OUTPAR_ ? 2 : 3;
          }
          else if (param->loccat==LC_VAR) // return value
          { *rtc.parameter_name=0;
            rtc.ordinal_position=0;
            rtc.direction   = 0;
          }
          else continue;
          rtc.data_type   = param->var.type;
          rtc.length      = SPECIFLEN(param->var.type) ? t_specif(param->var.specif).length : NONEINTEGER;
          rtc.precision   = param->var.type==ATT_MONEY || param->var.type==ATT_INT8  || param->var.type==ATT_INT16 || param->var.type==ATT_INT32 || param->var.type==ATT_INT64
                              ? (param->var.specif & 0xff) : 0;
          rtc.specif      = param->var.specif;
          tb_new(cdp, tbdf, FALSE, &vector);
        }
        psm_cache.put(routine); // return the routine to the cache
      }
    } // cycle
    close_working_cursor(cdp, curs);
  }
}
/////////////////////// IP addresses ///////////////////////////////////////////
struct result_IP_addresses
{ char IP_string[3*IP_length+IP_length-1+1];
  uns8 IP_bin[IP_length];
};

static const t_colval IP_addresses_coldescr[] = {
{  1, NULL, (void*)offsetof(result_IP_addresses, IP_string), NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_IP_addresses, IP_bin),    NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_IP_addresses(cdp_t cdp, table_descr * tbdf)
{ result_IP_addresses rtc;  char buf[100];
  t_vector_descr vector(IP_addresses_coldescr, &rtc);
  gethostname(buf, sizeof(buf));
  hostent * he = gethostbyname(buf);
  if (he && he->h_addr_list) 
  { int length = he->h_length;  if (length>IP_length) length=IP_length;
    for (int i=0;  he->h_addr_list[i];  i++) 
    { unsigned an_ip = ntohl(*(uns32*)he->h_addr_list[i]);
      if (an_ip && an_ip!=0x7f000001)
      { memset(rtc.IP_bin, 0, sizeof(rtc.IP_bin));
        memcpy(rtc.IP_bin, he->h_addr_list[i], length);
        sprintf(rtc.IP_string, "%u.%u.%u.%u", he->h_addr_list[i][0], he->h_addr_list[i][1], he->h_addr_list[i][2], he->h_addr_list[i][3]);
        tb_new(cdp, tbdf, FALSE, &vector);
      }
    }
  }
}

#if WBVERS>=950
//#include "extbtree.h"
t_ft_kontext_separated * get_sep_fulltext(cdp_t cdp, t_value * arg_vals)
{// analyse the parameters and find the fulltext:
  const char * objname = arg_vals[1].vmode==mode_simple ? arg_vals[1].strval : NULLSTRING,
             * schema  = arg_vals[0].vmode==mode_simple ? arg_vals[0].strval : NULLSTRING;
  sys_Upcase((char*)objname);
  sys_Upcase((char*)schema);
  tobjnum objnum;
  if (name_find2_obj(cdp, objname, schema, CATEG_INFO, &objnum))
    { SET_ERR_MSG(cdp, objname);  request_error(cdp, OBJECT_NOT_FOUND); }
  else
  {// verify the privileges (full data access required, otherwise documents can be reconstructed): 
    WBUUID schema_uuid;
    tb_read_atr(cdp, objtab_descr, objnum, APPL_ID_ATR, (tptr)schema_uuid);
    if (!cdp->has_full_access(schema_uuid))
      request_error(cdp, NO_RIGHT);
    else
    { t_ft_kontext * ft = get_fulltext_kontext(cdp, NULLSTRING, objname, schema);
      if (ft && ft->separated)
        return (t_ft_kontext_separated *)ft;
    }
  }
  return NULL;
}

struct result_fulltext_word
{ char     word[MAX_FT_WORD_LEN+1];
  t_wordid wordid;
};

static const t_colval fulltext_word_coldescr[] = {
{  1, NULL, (void*)offsetof(result_fulltext_word, word),   NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_fulltext_word, wordid), NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_fulltext_wordtab(cdp_t cdp, table_descr * tbdf, t_value * arg_vals)
{ result_fulltext_word rtc;  t_vector_descr vector(fulltext_word_coldescr, &rtc);
  t_ft_kontext_separated * ftks = get_sep_fulltext(cdp, arg_vals);
  if (ftks)
  { t_wordkey wordkey(NULLSTRING);
    t_reader mrsw_reader(&ftks->eif.mrsw_lock, cdp);
    cbnode_path path;
    cb_result res = cb_build_stack(ftks->eif, word_index_oper, WORD_INDEX_ROOT_NUM, &wordkey, path);
    if (res!=CB_ERROR) 
      while (cb_step(ftks->eif, word_index_oper, &wordkey, path))
      { strcpy(rtc.word, wordkey.word);
        rtc.wordid = *(t_wordid*)path.clustered_value_ptr(word_index_oper);
       // insert a row:
        tb_new(cdp, tbdf, FALSE, &vector);
      }
  }
}

#pragma pack(1)
struct result_fulltext_ref
{ t_wordid wordid;
  t_docid  docid;
  uns32    position;
  uns8     flags;
};
#pragma pack()

static const t_colval fulltext_ref_coldescr[] = {
{  1, NULL, (void*)offsetof(result_fulltext_ref, wordid),   NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_fulltext_ref, docid),    NULL, NULL, 0, NULL },
{  3, NULL, (void*)offsetof(result_fulltext_ref, position), NULL, NULL, 0, NULL },
{  4, NULL, (void*)offsetof(result_fulltext_ref, flags),    NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_fulltext_reftab(cdp_t cdp, table_descr * tbdf, t_value * arg_vals)
{ result_fulltext_ref rtc;  t_vector_descr vector(fulltext_ref_coldescr, &rtc);
  t_ft_kontext_separated * ftks = get_sep_fulltext(cdp, arg_vals);
  if (ftks)
  { if (ftks->bigint_id)
    { t_ft_key64 refkey;  refkey.wordid=0;  refkey.docid=0;  refkey.position=0;  
      t_reader mrsw_reader(&ftks->eif.mrsw_lock, cdp);
      cbnode_path path;
      cb_result res = cb_build_stack(ftks->eif, ref1_index_oper_64, REF1_INDEX_ROOT_NUM, &refkey, path);
      if (res!=CB_ERROR) 
        while (cb_step(ftks->eif, ref1_index_oper_64, &refkey, path))
        { rtc.wordid = refkey.wordid;
          rtc.docid = refkey.docid;
          rtc.position = refkey.position;
          rtc.flags = *(uns8*)path.clustered_value_ptr(ref1_index_oper_64);
         // insert a row:
          tb_new(cdp, tbdf, FALSE, &vector);
        }
    }
    else
    { t_ft_key32 refkey;  refkey.wordid=0;  refkey.docid=0;  refkey.position=0;  
      t_reader mrsw_reader(&ftks->eif.mrsw_lock, cdp);
      cbnode_path path;
      cb_result res = cb_build_stack(ftks->eif, ref1_index_oper_32, REF1_INDEX_ROOT_NUM, &refkey, path);
      if (res!=CB_ERROR) 
        while (cb_step(ftks->eif, ref1_index_oper_32, &refkey, path))
        { rtc.wordid = refkey.wordid;
          rtc.docid = refkey.docid;
          rtc.position = refkey.position;
          rtc.flags = *(uns8*)path.clustered_value_ptr(ref1_index_oper_32);
         // insert a row:
          tb_new(cdp, tbdf, FALSE, &vector);
        }
     }
  }
}

struct result_fulltext_ref2
{ t_docid  docid;
  t_wordid wordid;
};

static const t_colval fulltext_ref2_coldescr[] = {
{  1, NULL, (void*)offsetof(result_fulltext_ref2, docid),    NULL, NULL, 0, NULL },
{  2, NULL, (void*)offsetof(result_fulltext_ref2, wordid),   NULL, NULL, 0, NULL },
{ NOATTRIB, NULL, 0, NULL, NULL, 0, NULL } };

static void fill_fulltext_reftab2(cdp_t cdp, table_descr * tbdf, t_value * arg_vals)
{ result_fulltext_ref2 rtc;  t_vector_descr vector(fulltext_ref2_coldescr, &rtc);
  t_ft_kontext_separated * ftks = get_sep_fulltext(cdp, arg_vals);
  if (ftks)
  { t_reader mrsw_reader(&ftks->eif.mrsw_lock, cdp);
    cbnode_path path;
    if (ftks->bigint_id)
    { t_docid docid;
      docid=0;  
      cb_result res = cb_build_stack(ftks->eif, ref2_index_oper_64, REF2_INDEX_ROOT_NUM, &docid, path);
      if (res!=CB_ERROR) 
        while (cb_step(ftks->eif, ref2_index_oper_64, &docid, path))
        { rtc.docid = docid;
          rtc.wordid = *(t_wordid*)path.clustered_value_ptr(ref2_index_oper_64);
         // insert a row:
          tb_new(cdp, tbdf, FALSE, &vector);
        }
    }
    else
    { uns32 docid;
      docid=0;  
      cb_result res = cb_build_stack(ftks->eif, ref2_index_oper_32, REF2_INDEX_ROOT_NUM, &docid, path);
      if (res!=CB_ERROR) 
        while (cb_step(ftks->eif, ref2_index_oper_32, &docid, path))
        { rtc.docid = (t_docid)(sig64)(sig32)docid;
          rtc.wordid = *(t_wordid*)path.clustered_value_ptr(ref2_index_oper_32);
         // insert a row:
          tb_new(cdp, tbdf, FALSE, &vector);
        }
    }
  }
}
#endif // 950

#include "odbccat8.cpp"
void fill_profile(cdp_t cdp, table_descr * tbdf);

void t_optim_sysview::reset(cdp_t cdp)
{ 
 // evaluate parameters:
  t_value arg_vals[MAX_INFOVIEW_PARAMS];
  for (int i=0;  i<MAX_INFOVIEW_PARAMS && qe->args[i]!=NULL;  i++) 
    expr_evaluate(cdp, qe->args[i], arg_vals+i);
 // prepare the temp. table:
#if SQL3
  if (qe->kont.t_mat!=NULL) close_data(cdp);  // destroy the previous temporary materialisation
#else
  if (qe->kont.mat!=NULL) close_data(cdp);  // destroy the previous temporary materialisation
#endif
  t_mater_table * mat = new t_mater_table(cdp, FALSE);
  if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  return; }
#if SQL3
  qe->kont.t_mat = mat;
#else
  qe->kont.mat = mat;
#endif
  mat->completed=TRUE;  // never can go below
  if (!mat->init(cdp, qe)) { error=TRUE;  return; }
  table_descr * tbdf = tables[mat->tabnum];  // temporary table, tabdescr locked
  switch (qe->sysview_num)
  { case IV_NUM_LOGGED_USERS:
      fill_logged_users(cdp, tbdf);  break;
    case IV_CLIENT_ACTIV:
      fill_client_activity(cdp, tbdf);  break;
    case IV_TABLE_COLUMNS:
      fill_table_columns(cdp, tbdf, &post_conds);  break;
    case IV_MAIL_PROFS:
      fill_mail_profs(cdp, tbdf);  break;
    case IV_RECENT_LOG:
      fill_recent_log(cdp, tbdf, &post_conds);  break;
    case IV_LOG_REQS:
      fill_log_reqs(cdp, tbdf);  break;
    case IV_PROC_PARS:
      fill_proc_params(cdp, tbdf, &post_conds);  break;
    case IV_LOCKS:
      fill_locks(cdp, tbdf, &post_conds);  break;
    case IV_SUBJ_MEMB:
      fill_subject_membership(cdp, tbdf, &post_conds);  break;
    case IV_PRIVILEGES:
      fill_privileges(cdp, tbdf, &post_conds);  break;
    case IV_CHECK_CONS:
      fill_check_cons(cdp, tbdf, &post_conds);  break;
    case IV_FORKEYS:
      fill_forkeys(cdp, tbdf, &post_conds);  break;
    case IV_INDICIES:
      fill_indicies(cdp, tbdf, &post_conds);  break;
    case IV_CERTIFICATES:
      fill_certificates(cdp, tbdf, &post_conds);  break;
    case IV_IP_ADDRESSES:
      fill_IP_addresses(cdp, tbdf);  break;
    case IV_VIEWED_COLUMNS:
      fill_viewed_columns(cdp, tbdf, &post_conds);  break;
    case IV_ODBC_TYPES:
      fill_type_info(cdp, tbdf);  break;
    case IV_ODBC_COLUMNS:
      ctlg_fill(cdp, tbdf, CTLG_COLUMNS);  break;
    case IV_ODBC_COLUMN_PRIVS:
      ctlg_fill(cdp, tbdf, CTLG_COLUMN_PRIVILEGES);  break;
    case IV_ODBC_FOREIGN_KEYS:
      ctlg_fill(cdp, tbdf, CTLG_FOREIGN_KEYS);  break;
    case IV_ODBC_PRIMARY_KEYS:
      ctlg_fill(cdp, tbdf, CTLG_PRIMARY_KEYS);  break;
    case IV_ODBC_PROCEDURES:
      ctlg_fill(cdp, tbdf, CTLG_PROCEDURES);  break;
    case IV_ODBC_PROCEDURE_COLUMNS:
      ctlg_fill(cdp, tbdf, CTLG_PROCEDURE_COLUMNS);  break;
    case IV_ODBC_SPECIAL_COLUMNS:
      ctlg_fill(cdp, tbdf, CTLG_SPECIAL_COLUMNS);  break;
    case IV_ODBC_STATISTICS:
      ctlg_fill(cdp, tbdf, CTLG_STATISTICS);  break;
    case IV_ODBC_TABLES:
      ctlg_fill(cdp, tbdf, CTLG_TABLES);  break;
    case IV_ODBC_TABLE_PRIVS:
      ctlg_fill(cdp, tbdf, CTLG_TABLE_PRIVILEGES);  break;
#if WBVERS>=950
    case IVP_FT_WORDTAB:
      fill_fulltext_wordtab(cdp, tbdf, arg_vals);  break;
    case IVP_FT_REFTAB:
      fill_fulltext_reftab(cdp, tbdf, arg_vals);  break;
    case IVP_FT_REFTAB2:
      fill_fulltext_reftab2(cdp, tbdf, arg_vals);  break;
#endif
    case IV_PROFILE:
      fill_profile(cdp, tbdf);  break;
    case IV_TABLE_STAT:
      fill_table_space(cdp, tbdf, &post_conds);  break;
    case IV_OBJECT_ST:
      fill_object_state(cdp, tbdf, &post_conds);  break;
    case IV_MEMORY_USAGE:  
      fill_memory_usage(cdp, tbdf);  break;
  
  }
 // count valid records in the table
  limit = tbdf->Recnum();
#if SQL3
  uns8 del;
  while (limit>0 && !fast_table_read(cdp, tbdf, limit-1, DEL_ATTR_NUM, &del) && del!=NOT_DELETED)
    limit--;
  mat->_record_count = limit;
 // must apply filter now, create_cursor will not do this:
  qe->mat_extent = 1;  // used when creating the ptr. mat.
  t_mater_ptrs * pmat = create_or_clear_pmat(cdp, qe);
  if (!pmat) { eof=TRUE;  return; }     
  qe->kont.position=(trecnum)-1;
  eof=FALSE;
  while ((this->*p_get_next_record)(cdp)) // records satisfying WHERE condition & inherited conds
  { //if (limit_offset) limit_offset--;
    //else 
    //{ if (limit_count!=NORECNUM) 
    //  { if (!limit_count) break;
    //    limit_count--;
    //  }
      pmat->add_current_pos(cdp);
    //}
  }
  pmat->completed = TRUE;
  qe->kont.pmat_pos=(trecnum)-1;
 // remove the "distinct" processing:
  make_not_distinct();
#endif
  qe->kont.position=(trecnum)-1;
  eof=FALSE;
}

BOOL t_optim_sysview::get_next_record(cdp_t cdp)
{ t_value exprval;  
#if SQL3
  if (eof) return FALSE;
 // if materialised:
  if (qe->kont.p_mat && qe->kont.p_mat->completed)  // just taking the results from p_mat
  { if (++qe->kont.pmat_pos < qe->kont.p_mat->record_count())
    { 
#ifdef UNSTABLE_PMAT
      int ordnum = 0;
      qe->set_position_indep(cdp, &qe->kont.pmat_pos, ordnum);  // translates via p_mat
#else
      set_pmat_position(cdp, qe, qe->kont.pmat_pos, qe->kont.p_mat);  // translates via p_mat
#endif
      return TRUE;
    }
    eof=TRUE;  
    return FALSE;
  }
#endif
 // if not materialised (just materialising):
  while (++qe->kont.position < limit)
  { int i;
#if SQL3
    if (table_record_state(cdp, tables[qe->kont.t_mat->tabnum], qe->kont.position))  // temporary table, tabdescr locked
#else
    if (table_record_state(cdp, tables[((t_mater_table*)(qe->kont.mat))->tabnum], qe->kont.position))  // temporary table, tabdescr locked
#endif
      continue;
   // check the conditions, if any:
    for (i=0;  i<post_conds.count();  i++)
    { expr_evaluate(cdp, *post_conds.acc0(i), &exprval);
      if (!exprval.is_true()) goto failed;
    }
    return TRUE;
   failed: ;
  }
  eof=TRUE;
  return FALSE;
}

void t_optim_sysview::close_data(cdp_t cdp)
{ t_optim::close_data(cdp); 
  for (int i=0;  i<post_conds.count();  i++)  // must close subqueries in post conds
    (*post_conds.acc0(i))->close_data(cdp);
#if SQL3
  if (qe->kont.t_mat)
  { t_mater_table * tmat = qe->kont.t_mat;
    if (!tmat->permanent)
      tmat->destroy_temp_materialisation(cdp);
    delete qe->kont.t_mat;  
    qe->kont.t_mat=NULL; 
  }
#else
  if (qe->kont.mat)
  { if (!qe->kont.mat->is_ptrs)
    { t_mater_table * tmat = (t_mater_table*)qe->kont.mat;
      if (!tmat->permanent)
        tmat->destroy_temp_materialisation(cdp);
    }
    delete qe->kont.mat;  
    qe->kont.mat=NULL; 
  }
#endif
}
////////////////////// aggregation and materialization ///////////////////////
void t_optim_grouping::reset(cdp_t cdp)
// Create the materialized result of grouping, store its record count to group_count.
{ 
#if SQL3
  eof=FALSE;
#ifdef STABILIZED_GROUPING
  if (qeg->kont.t_mat!=NULL) repass();  // in subqueries the data is supposed to be externally closed
  else
#endif
 {group_count=0; // will be changed unless error
  if (qeg->kont.t_mat!=NULL) close_data(cdp);
  t_mater_table * mat = new t_mater_table(cdp, FALSE);
  if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  return; }
  qeg->kont.t_mat = mat;
#else
#ifdef STABILIZED_GROUPING
  if (qeg->kont.mat!=NULL) repass();  // in subqueries the data is supposed to be externally closed
  else
#endif
 {group_count=0; // will be changed unless error
  if (qeg->kont.mat!=NULL) close_data(cdp);
  t_mater_table * mat = new t_mater_table(cdp, FALSE);
  if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  return; }
  qeg->kont.mat = mat;
#endif
  mat->completed=FALSE;
  if (!mat->init(cdp, qeg)) { error=TRUE;  return; }
  trecnum tab_recnum;
 // pre-prepare grouping index (if any) as pattern for set distinctors:
  if (qeg->grouping_type==GRP_SINGLE)
    { qeg->group_indx.partnum=0;  qeg->group_indx.keysize=sizeof(trecnum); }
  qeg->group_indx.ccateg=INDEX_UNIQUE;  /* search does not work if not UNIQUE */
 // alloc groupkey for grouping and for set distinctors:
  tptr groupkey = (tptr)corealloc(qeg->group_indx.keysize+256, 93);
  if (groupkey==NULL)
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE; }
  else
  {// create indicies for set distinctors:
    int i;
    for (i=0;  i<qeg->agr_list.count();  i++)
    { t_agr_reg * agg=qeg->agr_list.acc0(i);
      if (agg->distinct)
      /* The aggregate distinctor indicies are non-standard. They do not own the expressions used int
         the index parts. The last part is not described in the parts[] array 
         and compute_key() cannot be used. */
      { agg->indx=new dd_index; // cannot be used by compute_key
        if (agg->indx==NULL)
          { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  continue; }
       // make copy of the grouping index and add one additional part to it: 
        memcpy(agg->indx, &qeg->group_indx, sizeof(dd_index));
        agg->indx->root=0;  // root not allocated yet
        agg->indx->parts=(part_desc*)corealloc(sizeof(part_desc)*(agg->indx->partnum+1), 59);
        if (!agg->indx->parts)
          { request_error(cdp, OUT_OF_KERNEL_MEMORY);  error=TRUE;  continue; }
        if (agg->indx->keysize+agg->argsize>MAX_KEY_LEN)
          { request_error(cdp, INDEX_KEY_TOO_LONG);   error=TRUE;  continue; }
        memcpy(agg->indx->parts, qeg->group_indx.parts, sizeof(part_desc)*agg->indx->partnum);
       // add the aggregated argument to the grouping index key:
        int tp=agg->arg->type;  // not the restype!
        agg->indx->parts[agg->indx->partnum].type=tp;
        agg->indx->parts[agg->indx->partnum].specif=agg->arg->specif;
        agg->indx->parts[agg->indx->partnum].partsize=agg->argsize;
        agg->indx->keysize+=agg->argsize;
        agg->indx->partnum++;
        if (create_empty_index(cdp, agg->indx->root, &cdp->current_cursor->translog)) error=TRUE;
      }
    }

   // create groups:
    if (qeg->grouping_type==GRP_SINGLE)
    { if (!error)
      { (opopt->*opopt->p_reset)(cdp);
        tab_recnum=NORECNUM;
        BOOL other_aggrs=mat->min_max_from_index(cdp, qeg, tab_recnum, groupkey);
        if (other_aggrs)
        { if ((opopt->*opopt->p_get_next_record)(cdp))
          { if (tab_recnum==NORECNUM) tab_recnum=mat->new_group(cdp, qeg);  // must be called after the 1st p_get_next_record because the 1st record of the group must be positioned
            if (tab_recnum==NORECNUM) error=TRUE;
            else
              do
                mat->record_in_group(cdp, qeg, tab_recnum, groupkey);
              while (!error && (opopt->*opopt->p_get_next_record)(cdp));
          }
          else  // fictive empty group
            tab_recnum=mat->new_group(cdp, qeg);
        }
        else
          if (tab_recnum==NORECNUM) tab_recnum=mat->new_group(cdp, qeg);  // when value does not exist
       // closing the created group:
        group_count=1;
        mat->close_group(cdp, qeg, tab_recnum);
      }
    }
    else // multiple groups formed by GROUP BY:
    {// prepare grouping index:
      if (create_empty_index(cdp, qeg->group_indx.root, &cdp->current_cursor->translog)) error=TRUE;
      else
      { trecnum null_group_recnum=NORECNUM;
       // pass records:
        if (!error) (opopt->*opopt->p_reset)(cdp);
        while (!error && (opopt->*opopt->p_get_next_record)(cdp))
        { if (!compute_key(cdp, NULL, 0, &qeg->group_indx, groupkey, true))
            { error=TRUE;  break; }
          *(trecnum*)(groupkey+qeg->group_indx.keysize-sizeof(trecnum))=0;
          if (is_null_key(groupkey, &qeg->group_indx))  // NULL key -> NULL group
          { if (null_group_recnum==NORECNUM) // create the NULL group
            { null_group_recnum=mat->new_group(cdp, qeg);
              if (null_group_recnum==NORECNUM) { error=TRUE;  break; }
            }
           // add to the NULL group:
            mat->record_in_group(cdp, qeg, null_group_recnum, groupkey);
          }
          else
          { cdp->index_owner = NOOBJECT;
            if (find_key(cdp, qeg->group_indx.root, &qeg->group_indx, groupkey, &tab_recnum, false))
              mat->record_in_group(cdp, qeg, tab_recnum, groupkey);
            else // a new group
            { tab_recnum=mat->new_group(cdp, qeg);
              if (tab_recnum==NORECNUM) error=TRUE;
              else
              { mat->record_in_group(cdp, qeg, tab_recnum, groupkey);
                *(trecnum*)(groupkey+qeg->group_indx.keysize-sizeof(trecnum))=tab_recnum;
                cdp->index_owner = NOOBJECT;  // the index is private, not locking
                bt_insert(cdp, groupkey, &qeg->group_indx, qeg->group_indx.root, NULL, &cdp->current_cursor->translog);
              }
            }
          }
        }
       // closing all created groups:
        if (mat->tabnum!=NOTBNUM)  // unless destroyed by a rollback
        { group_count = tables[mat->tabnum]->Recnum();  // temporary table, tabdescr locked
          for (tab_recnum=0;  tab_recnum<group_count;  tab_recnum++)
            if (!table_record_state(cdp, tables[mat->tabnum], tab_recnum))
              mat->close_group(cdp, qeg, tab_recnum);
        }
       // free grouping index:
        drop_index_idempotent(cdp, qeg->group_indx.root, sizeof(tblocknum)+qeg->group_indx.keysize, &cdp->current_cursor->translog);
      } // grouping index created
    } // multiple groups

   // free set distinctors:
    for (i=0;  i<qeg->agr_list.count();  i++)
    { t_agr_reg * agg=qeg->agr_list.acc0(i);
      if (agg->distinct && agg->indx)
      { drop_index_idempotent(cdp, agg->indx->root, sizeof(tblocknum)+agg->indx->keysize, &cdp->current_cursor->translog);
        // must not call descr_index, does not own the exprs!
        corefree(agg->indx->parts);  delete agg->indx;  agg->indx=NULL;
      }
    }
    corefree(groupkey);
  }
  mat->completed=TRUE;
#if SQL3
#if LIMITS_JOINED   // no more joining
  t_order_by * p_order = find_ordering(own_qe);  // takes the top ORDER BY from the hierarchy
  trecnum limit_count, limit_offset;
  get_limits(cdp, own_qe, limit_count, limit_offset);
#else  // only local ORDER and LIMIT processed here, otherwise problems with duplicate levels of ORDER-LIMIT-ORDER
  t_order_by * p_order = qeg->order_by;
  trecnum limit_count, limit_offset;
  get_limits(cdp, qeg, limit_count, limit_offset);
#endif
 // apply ORDER/LIMIT/HAVING/inherited_conditions/DISTINCT: create ptr mater:
  if (p_order && p_order->post_sort || limit_count!=NORECNUM || limit_offset>0 ||
      qeg->distinct || // distinct on an upper level not processed here
      qeg->having_expr!=NULL || inherited_having_conds.count())
  { qeg->mat_extent = 1;  // used when creating the ptr. mat.
    t_mater_ptrs * pmat = create_or_clear_pmat(cdp, qeg);
    if (!pmat) { eof=TRUE;  return; }     
   // passing the materialized table and creating the pmat, pmat->completed must be false:
    qeg->kont.position=position_for_next_record=(trecnum)-1;
    if (p_order && p_order->post_sort)
      sort_cursor(cdp, &p_order->order_indx, this, pmat, group_count, &qeg->kont, limit_count, limit_offset);
    else
      while ((this->*p_get_next_record)(cdp))
      // records satisfying HAVING condition & inherited conds
      { if (limit_offset) limit_offset--;
        else 
        { if (limit_count!=NORECNUM) 
          { if (!limit_count) break;
            limit_count--;
          }
          pmat->add_current_pos(cdp);
        }
      }
    pmat->completed = TRUE;
    own_qe->limit_result_count = pmat->rec_cnt;
   // remove the "distinct" processing:
    make_not_distinct();
    eof=FALSE;  // chnaged to TRUE in the cycle
    qeg->kont.pmat_pos=(trecnum)-1;
  }
#endif
  repass();  // defines the position
 }
}

void t_optim_grouping::repass(void)
{ 
#if SQL3
  position_for_next_record=(trecnum)-1;  // position for get_next_record must be different than the last read position, otherwise reading in imcomplete cursor may damage is creation
#else
  qeg->kont.position=(trecnum)-1;
#endif
  eof=FALSE;
}

BOOL t_optim_grouping::get_next_record(cdp_t cdp)
// Used when creating the pointer materialisation, for SORT, HAVING etc. -- qeg->kont.p_mat && !qeg->kont.p_mat->completed
// Used when passing the grouping result (in upper joins etc.)           -- !qeg->kont.p_mat || qeg->kont.p_mat->completed
// Neither t_mat nor p_mat reflect the inherited_conds!
{ t_value exprval;  int i;  
#if SQL3
  int ordnum;
  if (eof) return FALSE;
 // just taking the results from p_mat and checking the inherited_conds()
  if (qeg->kont.p_mat && qeg->kont.p_mat->completed)  
  { while (++position_for_next_record < qeg->kont.p_mat->record_count())
    { 
#ifdef UNSTABLE_PMAT
      ordnum = 0;
      qeg->set_position_indep(cdp, &position_for_next_record, ordnum);  // translates via p_mat
#else
      set_pmat_position(cdp, qeg, position_for_next_record, qeg->kont.p_mat);  // translates via p_mat
#endif
     // check the inherited conditions, if any:
      for (i=0;  i<inherited_conds.count();  i++)
      { expr_evaluate(cdp, *inherited_conds.acc0(i), &exprval);
        if (!exprval.is_true()) goto try_next1;
      }
      output_records++;
      return TRUE;
     try_next1: ;
    }
  }
  else  // no HAVING, sort, distinct: just taking the materialised results
  { while (++position_for_next_record < group_count)
    { if (table_record_state(cdp, tables[qeg->kont.t_mat->tabnum], position_for_next_record))
        continue;
      ordnum = 0;
      qeg->set_position_indep(cdp, &position_for_next_record, ordnum);  
     // if reset is completed, check the inherited conditions (if any), do not check them when creating p_mat:
      if (!qeg->kont.p_mat) 
        for (i=0;  i<inherited_conds.count();  i++)
        { expr_evaluate(cdp, *inherited_conds.acc0(i), &exprval);
          if (!exprval.is_true()) goto try_next2;
        }
      else  // if creating p_mat in the reset(), chech HAVING conditions
      { if (qeg->having_expr!=NULL)
        { expr_evaluate(cdp, qeg->having_expr, &exprval);
          if (!exprval.is_true()) goto try_next2;
          having_result++;
        }
        for (i=0;  i<inherited_having_conds.count();  i++)
        { expr_evaluate(cdp, *inherited_having_conds.acc0(i), &exprval);
          if (!exprval.is_true()) goto try_next2;
        }
      }
      output_records++;
      return TRUE;
     try_next2: ;
    }
  }
  eof=TRUE;
  return FALSE;

#else  // !SQL3
  
  while (++qeg->kont.position < group_count)
  {// check the HAVING condition, if any:
    if (qeg->having_expr!=NULL)
    { expr_evaluate(cdp, qeg->having_expr, &exprval);
      if (!exprval.is_true()) goto failed;
      having_result++;
    }
   // check the inherited conditions, if any:
    for (i=0;  i<inherited_conds.count();  i++)
    { expr_evaluate(cdp, *inherited_conds.acc0(i), &exprval);
      if (!exprval.is_true()) goto failed;
    }
    output_records++;
    return TRUE;
   failed: ;
  }
  eof=TRUE;
  return FALSE;
#endif
}

void t_optim_grouping::close_data(cdp_t cdp)
{ t_optim::close_data(cdp);
#if SQL3
  if (qeg->kont.t_mat)
  { t_mater_table * tmat = qeg->kont.t_mat;
    if (!tmat->permanent)
      tmat->destroy_temp_materialisation(cdp);
    delete qeg->kont.t_mat;  
    qeg->kont.t_mat=NULL; 
  }
#else
  if (qeg->kont.mat)
  { if (!qeg->kont.mat->is_ptrs)
    { t_mater_table * tmat = (t_mater_table*)qeg->kont.mat;
      if (!tmat->permanent)
        tmat->destroy_temp_materialisation(cdp);
    }
    delete qeg->kont.mat;  
    qeg->kont.mat=NULL; 
  }
#endif
  drop_index_idempotent(cdp, qeg->group_indx.root, sizeof(tblocknum)+qeg->group_indx.keysize, &cdp->current_cursor->translog);
  opopt->close_data(cdp);
}

//////////////////////////// materialization ////////////////////////////////
void t_query_expr :: write_kont_list(int & kont_cnt, BOOL is_top)
// Assigns kont.ordnum and calculates the pointer array size. Called int top_query_expression().
{ if (!is_top) kont_start = kont_cnt;
#if SQL3
  else kont.ordnum=0;
#endif
  if (!is_top && (maters==MAT_HERE || maters==MAT_INSENSITIVE))
  { kont.ordnum=kont_cnt++;
    kont.virtually_completed=true;
   // process the subtree:
    int kont_cnt_2 = 0;
    write_kont_list(kont_cnt_2, TRUE);
  }
  else
  { 
    switch (qe_type)
    { case QE_SPECIF:
        if (((t_qe_specif*)this)->from_tables)
          ((t_qe_specif*)this)->from_tables->write_kont_list(kont_cnt, FALSE);  
        break;
      case QE_TABLE:  case QE_SYSVIEW:
        kont.ordnum=kont_cnt++;  // !is_top supposed
        break;
      case QE_UNION:
        kont.ordnum=kont_cnt++;
        if (!is_top) kont_start = kont_cnt;
        //cont.
      case QE_EXCEPT:  case QE_INTERS:
      { t_qe_bin * qeb = (t_qe_bin *)this;
        int start=kont_cnt;
        qeb->op1->write_kont_list(kont_cnt, FALSE);
        int kont_cnt1 = kont_cnt;  kont_cnt=start;
        qeb->op2->write_kont_list(kont_cnt, FALSE);
        if (kont_cnt1>kont_cnt) kont_cnt = kont_cnt1;
        break;
      }
      case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
      case QE_JT_FULL:   case QE_JT_UNION:
        ((t_qe_join*)this)->op1->write_kont_list(kont_cnt, FALSE);
        ((t_qe_join*)this)->op2->write_kont_list(kont_cnt, FALSE);
        break;
    }
  }
  if (is_top) mat_extent=kont_cnt;
  else { rec_extent=kont_cnt-kont_start;  mat_extent=0; }
}

void t_query_expr::set_outer_null_position(void)
{ 
#ifdef UNSTABLE_PMAT
  if (kont.p_mat && kont.p_mat->completed) // define the untranslated position:
    kont.pmat_pos=OUTER_JOIN_NULL_POSITION;
  else 
#endif
  if (kont.is_mater_completed())
    kont.position=OUTER_JOIN_NULL_POSITION;
  else switch (qe_type)
  { case QE_TABLE: // possible when a cursor ddefined as "TABLE tab" is opened?
      kont.position=OUTER_JOIN_NULL_POSITION;  break;
    case QE_SPECIF:
      ((t_qe_specif*)this)->from_tables->set_outer_null_position();  break;
    case QE_UNION:
      ((t_qe_bin*)this)->op2->set_outer_null_position();
      ((t_qe_bin*)this)->op1->set_outer_null_position();
      break;
    case QE_EXCEPT:  case QE_INTERS:
      ((t_qe_bin *)this)->op1->set_outer_null_position();
      break;
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
    case QE_JT_FULL:   case QE_JT_UNION:
      ((t_qe_join*)this)->op1->set_outer_null_position();
      ((t_qe_join*)this)->op2->set_outer_null_position();
      break;
  }
}


#if SQL3

int t_query_expr::get_pmat_extent(void)
{ int ordnum = 0;
  get_extent_indep(ordnum);
  return ordnum;
}

void t_query_expr::get_position_indep(trecnum * recnums, int & ordnum, bool pmap_down)
{ 
#ifdef UNSTABLE_PMAT
  if (kont.p_mat && kont.p_mat->completed) // restore the untranslated position:
    if (!pmap_down)
    { recnums[ordnum++] = kont.pmat_pos;  // kont.position is stored translated
      return;
    }
    else pmap_down=false;
#endif
  if (kont.is_mater_completed())
  { recnums[ordnum++]=kont.position;
    return;
  }
  switch (qe_type)
  { case QE_TABLE: // possible when a cursor ddefined as "TABLE tab" is opened
      recnums[ordnum++]=kont.position;  break;
    case QE_SPECIF:
      if (((t_qe_specif*)this)->from_tables)
        ((t_qe_specif*)this)->from_tables->get_position_indep(recnums, ordnum, pmap_down);  
      break;
    case QE_UNION:
    { int ordnum1, ordnum2;
      recnums[ordnum++]=kont.position;
      ordnum1=ordnum2=ordnum;
      if (kont.position)
      { ((t_qe_bin*)this)->op1->get_position_indep(recnums, ordnum1);
        ((t_qe_bin*)this)->op2->get_position_indep(recnums, ordnum2);
      }
      else
      { ((t_qe_bin*)this)->op2->get_position_indep(recnums, ordnum2);
        ((t_qe_bin*)this)->op1->get_position_indep(recnums, ordnum1);
      }
      ordnum = ordnum1>ordnum2 ? ordnum1 : ordnum2;
      break;
    }
    case QE_EXCEPT:  case QE_INTERS:
      ((t_qe_bin *)this)->op1->get_position_indep(recnums, ordnum);
      break;
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
    case QE_JT_FULL:   case QE_JT_UNION:
      ((t_qe_join*)this)->op1->get_position_indep(recnums, ordnum);
      ((t_qe_join*)this)->op2->get_position_indep(recnums, ordnum);
      break;
  }
}

void t_query_expr::get_extent_indep(int & ordnum)
// Like get_position_indep, but only counts the record numbers in the set. 
// NO MORE: When p_mat if found, it is counted as single table, independent of the [completed] value (because I am using it when creating the pmat).
{ 

#ifdef UNSTABLE_PMAT
  if (kont.p_mat || kont.t_mat)
#else
  if (kont.t_mat)
#endif
    ordnum++;
  else switch (qe_type)
  { case QE_TABLE: // possible when a cursor ddefined as "TABLE tab" is opened
    case QE_SYSVIEW:  // possible when mat not created yet
      ordnum++;  break;
    case QE_SPECIF:
      if (((t_qe_specif*)this)->from_tables)
        ((t_qe_specif*)this)->from_tables->get_extent_indep(ordnum);  
      break;
    case QE_UNION:
    { int ordnum1, ordnum2;
      ordnum++;
      ordnum1=ordnum2=ordnum;
      ((t_qe_bin*)this)->op1->get_extent_indep(ordnum1);
      ((t_qe_bin*)this)->op2->get_extent_indep(ordnum2);
      ordnum = ordnum1>ordnum2 ? ordnum1 : ordnum2;
      break;
    }
    case QE_EXCEPT:  case QE_INTERS:
      ((t_qe_bin *)this)->op1->get_extent_indep(ordnum);
      break;
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
    case QE_JT_FULL:   case QE_JT_UNION:
      ((t_qe_join*)this)->op1->get_extent_indep(ordnum);
      ((t_qe_join*)this)->op2->get_extent_indep(ordnum);
      break;
  }
}

#ifndef UNSTABLE_PMAT
void set_pmat_position(cdp_t cdp, t_query_expr * qe, trecnum recnum, t_mater_ptrs * pmat)
// Set the current position in [qe] using the [recnum] in the top-level [pmat]
{ const trecnum * recnums;  int ordnum = 0;
  recnums = pmat->recnums_pos_const(cdp, recnum);
  if (recnums==NULL) return;
  if (qe->qe_type!=QE_UNION && qe->qe_type!=QE_INTERS && qe->qe_type!=QE_EXCEPT) // must not overwrite the direction info in this queries
    qe->kont.position=recnums[ordnum]; // used when checking the position change in subqueries like UPDATE Cenik SET cena = (SELECT nova_cena FROM Katalog WHERE Katalog.id_m = Cenik.id) WHERE EXISTS (SELECT nova_cena FROM Katalog WHERE Katalog.id_m=Cenik.id)
  qe->set_position_indep(cdp, recnums, ordnum);
}
#endif

void t_query_expr::set_position_indep(cdp_t cdp, const trecnum * recnums, int & ordnum)
// for double-materialisation kont.ordnum defines the position in th outer recnum-array
{ 
#ifdef UNSTABLE_PMAT
 // p_mat translation:
  int upper_ordnum;
  if (kont.p_mat && kont.p_mat->completed)
  {// store the untranslated position:
    kont.pmat_pos = recnums[ordnum];  
    if (qe_type!=QE_UNION && qe_type!=QE_INTERS && qe_type!=QE_EXCEPT) // must not overwrite the direction info in this queries
      kont.position=recnums[ordnum]; // used when checking the position change in subqueries like UPDATE Cenik SET cena = (SELECT nova_cena FROM Katalog WHERE Katalog.id_m = Cenik.id) WHERE EXISTS (SELECT nova_cena FROM Katalog WHERE Katalog.id_m=Cenik.id)
   // translate: 
    recnums = kont.p_mat->recnums_pos_const(cdp, recnums[ordnum]);
    if (recnums==NULL) { ordnum++;  return; }
   // start the new ordnum:
    upper_ordnum=ordnum+1;  ordnum=0;
  }
#endif
 // go to t_mat or down:
  if (kont.t_mat && kont.t_mat->completed)
    kont.position=recnums[ordnum++];
  else switch (qe_type)
  { case QE_TABLE: // possible when a cursor ddefined as "TABLE tab" is opened
      kont.position=recnums[ordnum++];  break;
    case QE_SPECIF:
      if (((t_qe_specif*)this)->from_tables)
        ((t_qe_specif*)this)->from_tables->set_position_indep(cdp, recnums, ordnum);  
      break;
    case QE_UNION:
    { kont.position=recnums[ordnum++];
      int ordnum1, ordnum2;
      ordnum1=ordnum2=ordnum;
      if (kont.position)
      { ((t_qe_bin*)this)->op1->set_position_indep(cdp, recnums, ordnum1);
        ((t_qe_bin*)this)->op2->set_position_indep(cdp, recnums, ordnum2);
      }
      else
      { ((t_qe_bin*)this)->op2->set_position_indep(cdp, recnums, ordnum2);
        ((t_qe_bin*)this)->op1->set_position_indep(cdp, recnums, ordnum1);
      }
      ordnum = ordnum1>ordnum2 ? ordnum1 : ordnum2;
      break;
    }
    case QE_EXCEPT:  case QE_INTERS:
      ((t_qe_bin *)this)->op1->set_position_indep(cdp, recnums, ordnum);
      break;
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
    case QE_JT_FULL:   case QE_JT_UNION:
      ((t_qe_join*)this)->op1->set_position_indep(cdp, recnums, ordnum);
      ((t_qe_join*)this)->op2->set_position_indep(cdp, recnums, ordnum);
      break;
  }
#ifdef UNSTABLE_PMAT
 // restore outer ordnum:
  if (kont.p_mat && kont.p_mat->completed)
    ordnum=upper_ordnum;
#endif
}

#else  // SQL2

void t_query_expr::set_position(cdp_t cdp, const trecnum * recnums)
// for double-materialisation kont.ordnum defines the position in th outer recnum-array
{ 
#if SQL3  // not used
  if (kont.p_mat && kont.p_mat->completed)
  {// store the untranslated position:
    // no,because kont_ordnum refers to the upper pmat: kont.pmat_pos = recnums[kont.ordnum];  // kont.position for kont.t_mat must be stored translated
    if (qe_type!=QE_UNION && qe_type!=QE_INTERS && qe_type!=QE_EXCEPT) // must not overwrite the direction info in this queries
      kont.position=recnums[kont.ordnum]; // used when checking the position change in subqueries like UPDATE Cenik SET cena = (SELECT nova_cena FROM Katalog WHERE Katalog.id_m = Cenik.id) WHERE EXISTS (SELECT nova_cena FROM Katalog WHERE Katalog.id_m=Cenik.id)
   // translate: 
    recnums = kont.p_mat->recnums_pos_const(cdp, recnums[kont.ordnum]);
    if (recnums==NULL) return;
   // position the local table, if present (cannot use the code below because kont.ordnum has a different meaning):
    if (kont.t_mat)
    { kont.position=recnums[0];
      return;
    }
  }
  if (kont.t_mat && kont.t_mat->completed)
#else
  if (kont.is_mater_completed())
#endif
    kont.position=recnums[kont.ordnum];
  else switch (qe_type)
  { case QE_TABLE: // possible when a cursor ddefined as "TABLE tab" is opened
      kont.position=recnums[kont.ordnum];  break;
    case QE_SPECIF:
      ((t_qe_specif*)this)->from_tables->set_position(cdp, recnums);  break;
    case QE_UNION:
      kont.position=recnums[kont.ordnum];
      if (kont.position)
        ((t_qe_bin*)this)->op2->set_position(cdp, recnums);
      else
        ((t_qe_bin*)this)->op1->set_position(cdp, recnums);
      break;
    case QE_EXCEPT:  case QE_INTERS:
      ((t_qe_bin *)this)->op1->set_position(cdp, recnums);
      break;
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
    case QE_JT_FULL:   case QE_JT_UNION:
      ((t_qe_join*)this)->op1->set_position(cdp, recnums);
      ((t_qe_join*)this)->op2->set_position(cdp, recnums);
      break;
    case QE_IDENT:
      ((t_qe_ident*)this)->op->set_position(cdp, recnums);
      break;
  }
}

void t_query_expr::get_position(trecnum * recnums)
{ 
#ifdef UNSTABLE_PMAT
#if SQL3  // not used
  if (kont.p_mat && kont.p_mat->completed) // restore the untranslated position:
    recnums[kont.ordnum] = kont.pmat_pos;  // kont.position is stored translated
  else
#endif
#endif
  if (kont.is_mater_completed())
    recnums[kont.ordnum]=kont.position;
  else switch (qe_type)
  { case QE_TABLE: // possible when a cursor ddefined as "TABLE tab" is opened
      recnums[kont.ordnum]=kont.position;  break;
    case QE_SPECIF:
      ((t_qe_specif*)this)->from_tables->get_position(recnums);  break;
    case QE_UNION:
      recnums[kont.ordnum]=kont.position;
      if (kont.position)
        ((t_qe_bin*)this)->op2->get_position(recnums);
      else
        ((t_qe_bin*)this)->op1->get_position(recnums);
      break;
    case QE_EXCEPT:  case QE_INTERS:
      ((t_qe_bin *)this)->op1->get_position(recnums);
      break;
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
    case QE_JT_FULL:   case QE_JT_UNION:
      ((t_qe_join*)this)->op1->get_position(recnums);
      ((t_qe_join*)this)->op2->get_position(recnums);
      break;
    case QE_IDENT:
      ((t_qe_ident*)this)->op->get_position(recnums);
      break;
  }
}

#endif

BOOL t_query_expr::locking(cdp_t cdp, const trecnum * recnums, int opc, int & limit)
// Performs locking operation on record vector given by recnums or on
// full tables iff recnums=NULL. On error sets limit, works only upto limit.
{ 
#if SQL3
  if (kont.t_mat!=NULL) // called when completed==TRUE I think, no action if not permanent
  { if (kont.ordnum>=limit) return FALSE;
    if (!kont.t_mat->permanent) return FALSE;
    ttablenum tabnum = kont.t_mat->tabnum;
#else
  if (kont.mat!=NULL && !kont.mat->is_ptrs) // called when completed==TRUE I think, no action if not permanent
  { if (kont.ordnum>=limit) return FALSE;
    if (!((t_mater_table*)kont.mat)->permanent) return FALSE;
    ttablenum tabnum = ((t_mater_table*)kont.mat)->tabnum;
#endif
    table_descr * tbdf = tables[tabnum];
    trecnum recnum = recnums==NULL ? NORECNUM : recnums[kont.ordnum];
    if (recnum!=OUTER_JOIN_NULL_POSITION)
      switch (opc)
      { case OP_RLOCK:
          if (tb_rlock(cdp, tbdf, recnum)) { limit=kont.ordnum;  return TRUE; }
          break;
        case OP_WLOCK:
          if (tb_wlock(cdp, tbdf, recnum)) { limit=kont.ordnum;  return TRUE; }
          break;
        case OP_UNRLOCK:
          tb_unrlock(cdp, tbdf, recnum);
          break;
        case OP_UNWLOCK:
          tb_unwlock(cdp, tbdf, recnum);
          break;
        case OP_WRITE:  // used by WriteRecord
          wait_record_lock_error(cdp, tbdf, recnum, TMPWLOCK);
          break;
        case OP_READ:   // used by ReadRecord
          wait_record_lock_error(cdp, tbdf, recnum, TMPRLOCK);
          break;
        case OP_SUBMIT: // used by UPDATE statement - no error if cannot lock
          return record_lock(cdp, tbdf, recnum, TMPWLOCK)!=0;  
      }
  }
  else switch (qe_type)
  { //case QE_TABLE: -- impossible, processed above
    case QE_SPECIF:
      return ((t_qe_specif*)this)->from_tables->locking(cdp, recnums, opc, limit);
    case QE_UNION:
      if (recnums==NULL)
        return ((t_qe_bin *)this)->op2->locking(cdp, recnums, opc, limit) ||
               ((t_qe_bin *)this)->op1->locking(cdp, recnums, opc, limit);
      else if (recnums[kont.ordnum])
        return ((t_qe_bin *)this)->op2->locking(cdp, recnums, opc, limit);
      else
        return ((t_qe_bin *)this)->op1->locking(cdp, recnums, opc, limit);
    case QE_EXCEPT:  case QE_INTERS:
        return ((t_qe_bin *)this)->op1->locking(cdp, recnums, opc, limit);
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
    case QE_JT_FULL:   case QE_JT_UNION:
        return ((t_qe_join*)this)->op1->locking(cdp, recnums, opc, limit) ||
               ((t_qe_join*)this)->op2->locking(cdp, recnums, opc, limit);
  }
  return FALSE;
}

void t_query_expr::get_table_numbers(ttablenum * tabnums, int & count)
// I think that setting kont.ordnum is not necessary here. [count] should be used as index.
{ switch (qe_type)
  { case QE_TABLE:  case QE_SYSVIEW:
      kont.ordnum=count++;
#if SQL3
      if (tabnums) tabnums[kont.ordnum] = kont.t_mat ? kont.t_mat->tabnum : 0x8002; // representing variable temporary table;
#else
      if (tabnums) tabnums[kont.ordnum] = kont.mat ? ((t_mater_table*)kont.mat)->tabnum : 0x8002; // representing variable temporary table;
#endif
      // temporary table materialization may have been removed by close_data
      break;
    case QE_SPECIF:
      if (((t_qe_specif*)this)->from_tables)
        if (((t_qe_specif*)this)->grouping_type==GRP_NO)
          ((t_qe_specif*)this)->from_tables->get_table_numbers(tabnums, count);
        else
        { kont.ordnum=count++;
          if (tabnums) tabnums[kont.ordnum] = 0x8002; // representing variable temporary table
        }
      break;
    case QE_UNION:
      kont.ordnum=count++;
      if (tabnums) tabnums[kont.ordnum]=(ttablenum)0x8000;
      // cont.
    case QE_EXCEPT:  case QE_INTERS:
    { int start=count;
      ((t_qe_bin *)this)->op2->get_table_numbers(tabnums, count);
      int cnt2=count;  count=start;
      ((t_qe_bin *)this)->op1->get_table_numbers(tabnums, count);
      while (count<cnt2)  // add placeholders
      { if (tabnums) tabnums[count] = (ttablenum)0x8001;
        count++;
      }
      break;
    }
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
    case QE_JT_FULL:   case QE_JT_UNION:
      ((t_qe_join*)this)->op1->get_table_numbers(tabnums, count);
      ((t_qe_join*)this)->op2->get_table_numbers(tabnums, count);
      break;
  }
}

bool find_table_in_qe(t_query_expr * qe, ttablenum tabnum)
{ switch (qe->qe_type)
  { case QE_TABLE:  
#if SQL3
      return qe->kont.t_mat && qe->kont.t_mat->tabnum==tabnum;
#else
      return qe->kont.mat && ((t_mater_table*)qe->kont.mat)->tabnum==tabnum;
#endif
    case QE_SPECIF:
      if (((t_qe_specif*)qe)->grouping_type==GRP_NO)
        return find_table_in_qe(((t_qe_specif*)qe)->from_tables, tabnum);
      return false;
    case QE_UNION:  case QE_EXCEPT:  case QE_INTERS:
      return find_table_in_qe(((t_qe_bin *)qe)->op1, tabnum) || find_table_in_qe(((t_qe_bin *)qe)->op2, tabnum);
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:
    case QE_JT_FULL:   case QE_JT_UNION:
      return find_table_in_qe(((t_qe_join*)qe)->op1, tabnum) || find_table_in_qe(((t_qe_join*)qe)->op2, tabnum);
  }
  return false;  // QE_SYSVIEW
}

bool t_query_expr::lock_unlock_tabdescrs(cdp_t cdp, bool locking)  
// Locks/unlocks tabdescrs and table definitions. Called when creating and destroying a cursor.
{ switch (qe_type)
  { case QE_TABLE:  
    { t_qe_table * qet = (t_qe_table*)this;
      if (locking)
      { if (qet->tabnum!=NOTBNUM)
        { if (qet->is_local_table)  // find the current table number (may change)
          { t_mater_table * mat = qet->kont.t_mat;
            qet->tabnum = mat->tabnum = *(ttablenum*)variable_ptr(cdp, mat->locdecl_level, mat->locdecl_offset);
          }
          qet->tbdf_holder=install_table(cdp, qet->tabnum);  
          if (qet->tbdf_holder && !qet->is_local_table) 
            if (record_lock_error(cdp, tabtab_descr, qet->tbdf_holder->tbnum, RLOCK)) // cannot lock
            { unlock_tabdescr(qet->tbdf_holder);  qet->tbdf_holder=NULL;
              return true; 
            }
        }
      }
      else if (qet->tbdf_holder)
      { if (!qet->is_local_table)
          record_unlock(cdp, tabtab_descr, qet->tbdf_holder->tbnum, RLOCK);
        unlock_tabdescr(qet->tbdf_holder);
        qet->tbdf_holder=NULL;
      }
      break;
    }
    case QE_IDENT:
      return ((t_qe_ident*)this)->op->lock_unlock_tabdescrs(cdp, locking);  
    case QE_SYSVIEW:
      break;
    case QE_SPECIF:
      if (!((t_qe_specif*)this)->from_tables) break;
      return ((t_qe_specif*)this)->from_tables->lock_unlock_tabdescrs(cdp, locking);  
    case QE_UNION:  case QE_EXCEPT:  case QE_INTERS:
      return ((t_qe_bin *)this)->op2->lock_unlock_tabdescrs(cdp, locking) ||
             ((t_qe_bin *)this)->op1->lock_unlock_tabdescrs(cdp, locking);
    case QE_JT_INNER:  case QE_JT_LEFT:  case QE_JT_RIGHT:  case QE_JT_FULL:   case QE_JT_UNION:
      return ((t_qe_join*)this)->op1->lock_unlock_tabdescrs(cdp, locking) ||
             ((t_qe_join*)this)->op2->lock_unlock_tabdescrs(cdp, locking);
  }
  return false;
}

static void passing_open_close(cdp_t cdp, t_optim * opt, BOOL openning)
{ switch (opt->optim_type)
  { case OPTIM_TABLE:
    { t_optim_table * optt = (t_optim_table *)opt;
#ifdef STOP   // replaced by windowed access to the index
      if (optt->passing_opened) // some tables may have the passing opened and ssome closed
        if (optt->docid_index!=-1)  // fulltext passing
        { if (optt->fulltext_expr)
            if (openning) 
            { if (!wait_record_lock_error(cdp, optt->td, NORECNUM, RLOCK))
                optt->table_read_locked=true;
              optt->fulltext_expr->open_index(cdp, FALSE);
            }
            else 
            { optt->fulltext_expr->close_index(cdp);
              if (optt->table_read_locked)  // used by the fulltext index
              { record_unlock(cdp, optt->td, NORECNUM, RLOCK);
                optt->table_read_locked=false;
              }
            }
        }
        else if (optt->normal_index_count)
          if (openning)
          { if (optt->tics[0].indx) // must test it, NULL when passing has not been reset yet (2nd table in a join)
            { if (optt->td) cdp->index_owner = optt->td->tbnum;
              optt->tics[0].open_index_passing(cdp);
            }
          }
          else optt->tics[0].close_index_passing(cdp); /* on error, may have been unlocked before */
#endif
      break;
    }
    case OPTIM_GROUPING:  // nothing open below
      break;
    case OPTIM_EXC_INT:   // private index on result open, do not close. But must unfix, otherwise commit reports fixed dirty page
    { t_optim_exc_int * eopt = (t_optim_exc_int*)opt;
      if (openning)
      { if (eopt->bac.fcbn==NOFCBNUM) 
          fix_any_page(cdp, eopt->bac.stack[eopt->bac.sp].nn, &eopt->bac.fcbn);
      }
      else
      { if (eopt->bac.fcbn!=NOFCBNUM) 
          { unfix_page(cdp, eopt->bac.fcbn);  eopt->bac.fcbn=NOFCBNUM; }
      }
      break;
    }
#if SQL3
    case OPTIM_LIMIT:
    { t_optim_limit * xopt = (t_optim_limit *)opt;
      passing_open_close(cdp, xopt->subopt, openning);
      break;
    }
    case OPTIM_ORDER:
    { t_optim_order * xopt = (t_optim_order *)opt;
      passing_open_close(cdp, xopt->subopt, openning);
      break;
    }
#endif
    case OPTIM_UNION_DIST:  case OPTIM_UNION_ALL:
    { t_optim_bin * bopt = (t_optim_bin *)opt;
      passing_open_close(cdp, bopt->op1opt, openning);
      passing_open_close(cdp, bopt->op2opt, openning);
      break;
    }
    case OPTIM_JOIN_OUTER:
    { t_optim_join_outer * ojopt = (t_optim_join_outer *)opt;
      if (ojopt->right_post_phase && ojopt->rightopt!=NULL)
        passing_open_close(cdp, ojopt->rightopt, openning);
      else
        for (int i=0;  i<ojopt->leave_count;  i++)
        { t_optim * subopt = ojopt->variant.join_steps[i];
          passing_open_close(cdp, subopt, openning);
        }
      break;
    }
    
    case OPTIM_JOIN_INNER:
    { t_optim_join_inner * jopt = (t_optim_join_inner *)opt;
      if (jopt->curr_variant_num>=0 && jopt->curr_variant_num < jopt->variants.count())
      { for (int i=0;  i<jopt->leave_count;  i++)
        { t_optim * subopt = jopt->curr_variant->join_steps[i];
          passing_open_close(cdp, subopt, openning);
        }
      }
    }
  }
}

static void propagate_locking_flag(t_optim * opt)
{ switch (opt->optim_type)
  { case OPTIM_TABLE:
    { t_optim_table * optt = (t_optim_table *)opt;
      optt->locking=true;
      break;
    }
    case OPTIM_GROUPING:  // do not lock below
      break;
    case OPTIM_EXC_INT:   
    { t_optim_bin * bopt = (t_optim_bin *)opt;
      propagate_locking_flag(bopt->op1opt);   // result will be read only from the 1st operand
      break;
    }
#if SQL3
    case OPTIM_LIMIT:   
    { t_optim_limit * xopt = (t_optim_limit *)opt;
      propagate_locking_flag(xopt->subopt); 
      break;
    }
    case OPTIM_ORDER:   
    { t_optim_order * xopt = (t_optim_order *)opt;
      propagate_locking_flag(xopt->subopt); 
      break;
    }
#endif
    case OPTIM_UNION_DIST:  case OPTIM_UNION_ALL:
    { t_optim_bin * bopt = (t_optim_bin *)opt;
      propagate_locking_flag(bopt->op1opt);
      propagate_locking_flag(bopt->op2opt);
      break;
    }
    case OPTIM_JOIN_OUTER:
    { t_optim_join_outer * ojopt = (t_optim_join_outer *)opt;
      if (ojopt->right_post_phase && ojopt->rightopt!=NULL)
        propagate_locking_flag(ojopt->rightopt);
      else
        for (int i=0;  i<ojopt->leave_count;  i++)
        { t_optim * subopt = ojopt->variant.join_steps[i];
          propagate_locking_flag(subopt);
        }
      break;
    }
    
    case OPTIM_JOIN_INNER:
    { t_optim_join_inner * jopt = (t_optim_join_inner *)opt;
      for (int v=0;  v<jopt->variants.count();  v++)
      { for (int i=0;  i<jopt->leave_count;  i++)
        { t_optim * subopt = jopt->variants.acc0(v)->join_steps[i];
          propagate_locking_flag(subopt);
        }
      }
    }
  }
}


BOOL t_materialization::init(t_query_expr * qe)
{ top_qe=qe;
  return TRUE;
}

t_mater_ptrs::t_mater_ptrs(cdp_t cdpIn) : selection(sizeof(trecnum),50,1000), pool_access(cdpIn)
{ is_ptrs=TRUE;
  apoolnum=-1;  direct=true;
}

t_mater_ptrs::~t_mater_ptrs(void)
{ clear(SaferGetCurrTaskPtr); } // this should by harmless

int tab_mater_cnt = 0;

void t_mater_ptrs::clear(cdp_t cdp)
// Discards all the contents, puts the structure into its initial state.
{ 
#ifdef STOP  // removed, recnums stored in the memory only
  if (!direct)
  { pool_access.data_unchanged(); // prevents writing changes, if any
    pool_access.close();
    apoolnum=-1;
    for (int i=0;  i<selection.count();  i++)
      ffa.release_block(cdp, *(tblocknum*)selection.acc0(i));
    direct=true;
  }
#endif
  selection.t_dynar::~t_dynar();   /* make empty */
  rec_cnt=0;
}

void t_mater_ptrs::mark_disk_space(cdp_t cdp) const
{ if (!direct)
    for (int i=0;  i<selection.count();  i++)
      mark_used_block(cdp, *(tblocknum*)selection.acc0(i));
}

BOOL t_mater_ptrs::init(t_query_expr * qe)
// If qe==NULL, kont_cnt must be defined before
{ if (!t_materialization::init(qe)) return FALSE;
#if SQL3
  kont_cnt=qe->get_pmat_extent();
#else
  kont_cnt=qe->mat_extent ? qe->mat_extent : qe->rec_extent; // used by pointer materialisation only
#endif
  if (kont_cnt)
    crecperblock=BLOCKSIZE / (sizeof(trecnum)*kont_cnt);
  rec_cnt=0;
  return TRUE;
}

trecnum * t_mater_ptrs::setpool2(cdp_t cdp, unsigned poolnum)
/* Called only if !diradr, selects the given pool (it may not exist) */
/* Returns NULL iff cannot access the pool. */
{ pool_access.close();
  apoolnum=-1;
  tblocknum * pdadr=(tblocknum*)selection.acc(poolnum);
  if (pdadr==NULL)
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  trecnum * pooladr;
  if (!*pdadr)  // must allocate a new block for cursor
    pooladr = (trecnum *)pool_access.access_new_block(*pdadr);
  else          // access existing block
    pooladr = (trecnum *)pool_access.access_block(*pdadr);
  if (pooladr==NULL) return NULL;
  apoolnum=poolnum;
  return pooladr;
}

trecnum * t_mater_ptrs::recnums_space(cdp_t cdp)
// Appends and returns a new record space into the cursor, return NULL on error.
{ trecnum * adr;
  if (direct)
  { unsigned off32 = rec_cnt * kont_cnt;
#ifdef STOP  // removed, recnums stored in the memory only
    if (off32+kont_cnt <= MAX_DIRECT_SELECTION)
#endif
    { if (selection.acc(off32+kont_cnt)==NULL) return NULL;
      rec_cnt++;
      return (trecnum*)selection.acc0(off32);
    }
#ifdef STOP  // removed, recnums stored in the memory only
    else  /* reorganize */
    { tblocknum dadr;
      adr = (trecnum*)pool_access.access_new_block(dadr);
      if (adr==NULL) return NULL;
      apoolnum=0;  direct=false;
      memcpy(adr, selection.acc(0), off32*sizeof(trecnum));
      selection.t_dynar::~t_dynar();   /* make empty */
      *(tblocknum*)selection.acc(0)=dadr;
      pool_access.data_changed();
    }
#endif
  }
  adr=setpool(cdp, rec_cnt / crecperblock);
  if (!adr) return NULL;    /* cursor overflow */
  pool_access.data_changed();
  return adr + (rec_cnt++ % crecperblock) * kont_cnt;
}

const trecnum * t_mater_ptrs::recnums_pos_const(cdp_t cdp, trecnum crec)
{ if (crec >= rec_cnt) return NULL;
  if (direct) return (trecnum*)selection.acc(crec * kont_cnt);
  else
  { trecnum * pooladr=setpool(cdp, crec / crecperblock);
    if (pooladr==NULL) return NULL;
    return pooladr + (crec % crecperblock) * kont_cnt;
  }
}

trecnum * t_mater_ptrs::recnums_pos_writable(cdp_t cdp, trecnum crec)
{ const trecnum * recs = recnums_pos_const(cdp, crec);
  if (!recs) return NULL;
  if (!direct) pool_access.data_changed();  // data will be changed by the caller, must record this
  return (trecnum*)recs;
}

void t_mater_ptrs::curs_del(cdp_t cdp, trecnum crec)
{ trecnum * recs = recnums_pos_writable(cdp, crec);
  if (recs!=NULL) *recs=NORECNUM;
}


void t_mater_ptrs::add_current_pos(cdp_t cdp)
{ trecnum * recs = recnums_space(cdp);
#if SQL3
  int ordnum = 0;
  if (recs!=NULL) top_qe->get_position_indep(recs, ordnum);
#else
  if (recs!=NULL) top_qe->get_position(recs);
#endif
}

BOOL t_mater_ptrs::put_recnums(cdp_t cdp, trecnum * recnums)
// used only by Add_records and Insert (Append)
{ trecnum * recs = recnums_space(cdp);
  if (recs==NULL) return FALSE;
  memcpy(recs, recnums, sizeof(trecnum) * kont_cnt);
  return TRUE;
}

BOOL t_mater_ptrs::cursor_search(cdp_t cdp, trecnum * recnums, trecnum * crec, int cnt)
{ const trecnum * recs;  unsigned i;
  unsigned compsize=sizeof(trecnum)*cnt;
  if (direct)
  { for (i=0, recs=(const trecnum*)selection.acc(0);  i<rec_cnt;  i++, recs+=kont_cnt)
      if (!memcmp(recs, recnums, compsize))
        { *crec=(trecnum)i;  return TRUE; }
  }
  else
  { for (unsigned poolnum=0;  poolnum < selection.count();  poolnum++)
    { trecnum limit = rec_cnt-poolnum*crecperblock;
      if (limit > crecperblock) limit = crecperblock;
      const trecnum * pooladr=setpool(cdp, poolnum);
      if (pooladr)
      { for (i=0, recs=pooladr;  i<limit;  i++, recs+=kont_cnt)
          if (!memcmp(recs, recnums, compsize))
            { *crec=(trecnum)(crecperblock*poolnum+i);  return TRUE; }
      }
    }
  }
  *crec=NORECNUM;
  return FALSE;
}

const trecnum * get_recnum_list(cur_descr * cd, trecnum crec)
{ return cd->pmat->recnums_pos_const(cd->cdp, crec); }

BOOL t_mater_table :: init(cdp_t cdp, t_query_expr * qe)
// Create temporary table according to the kontext of [qe] for materialisation of [qe] result.
// Used by grouping result, system view result, insensitive and instable results.
{ if (permanent) return TRUE;
  if (!t_materialization::init(qe)) return FALSE;
  db_kontext * kont = &qe->kont;
  t_qe_specif * qs;
  if (qe->qe_type!=QE_SPECIF || ((t_qe_specif*)qe)->grouping_type==GRP_NO) qs=NULL;
  else qs=(t_qe_specif *)qe;
 // create the descriptor of the temporary table from kont/qs: 
  int attrcnt=kont->elems.count();
  if (qs) attrcnt+=1+qs->agr_list.count();
  table_descr * tbdf = new(attrcnt, 0) table_descr;
  if (tbdf==NULL) return FALSE;
  int i;
  for (i=0;  i<kont->elems.count();  i++)
  { const elem_descr * el = kont->elems.acc0(i);
    tbdf->add_column(el->name, el->type, el->multi, wbtrue, el->specif);
  }
 // add the aggregate function arguments:
  aggr_base=kont->elems.count();
  if (qs)
  { tbdf->add_column(NULLSTRING, ATT_STRING, 1, wbtrue, sizeof(trecnum)*MAX_OPT_TABLES);
    // optimize specif, may be shorter!!!
    for (i=0;  i<qs->agr_list.count();  i++)
    { const t_agr_reg * agg=qs->agr_list.acc0(i);
      tbdf->add_column(NULLSTRING, agg->restype, 1, wbtrue, agg->ressize); // agr==NULL for COUNT(*)
    }
  }

 /* create the temporary table: */
  tbdf->translog=&cdp->current_cursor->translog;
  tabnum=create_temporary_table(cdp, tbdf, &cdp->current_cursor->translog);
  if (!tabnum) { tbdf->free_descr(cdp);  return FALSE; }
#if SQL3
  _record_count = 0;
#endif
  return TRUE;
}

static void aggregate_value(tptr dt, t_value * res, t_agr_reg * agg)
// res is not NULL. dt may be NULL.
{ if (agg->ag_type==S_COUNT)
    (*(uns32*)dt)++;
  else if (!res->is_null())
  { int tp=agg->restype;  tptr ptr = res->valptr();
    if (IS_NULL(dt, agg->restype, agg->arg->specif))  // nothing stored so far
      memcpy(dt, ptr, agg->ressize);
    else
      switch (agg->ag_type)
      { case S_SUM:
          switch (tp)
          { case ATT_CHAR:  *(sig32 *)dt += *ptr;                    break;
            case ATT_INT16:  case ATT_INT8:
            case ATT_DATE:  case ATT_TIME:  case ATT_TIMESTAMP:  case ATT_DATIM: // for AVG
                            *(uns32 *)dt += (uns32)res->intval;      break;
            case ATT_INT32: *(sig32 *)dt += res->intval;             break;
            case ATT_INT64: *(sig64 *)dt += res->i64val;             break;
            case ATT_MONEY: 
            { sig64 sum = money2i64((monstr *)dt);  sum+=res->i64val;
              memcpy(dt, &sum, sizeof(monstr));
              break;
            }
            case ATT_FLOAT: *(double*)dt += res->realval;            break;
          }
          break;
        case S_MAX:
          if (compare_values(tp, agg->arg->specif, ptr, dt) > 0)
            memcpy(dt, ptr, agg->argsize);
          break;
        case S_MIN:
          if (compare_values(tp, agg->arg->specif, ptr, dt) < 0)
            memcpy(dt, ptr, agg->argsize);
          break;
      }
  }
}

void t_mater_table :: close_group(cdp_t cdp, t_qe_specif * qs, trecnum recnum)
// Calculates the values of exprs containig aggregates
{ int i;  trecnum recnums[MAX_OPT_TABLES+1]; // +1 for terminating 0
  table_descr * tbdf = tables[tabnum];
  //cdp->kont_recnum=recnum;
 // read and set the position of the 1st record of the group:
  tb_read_atr(cdp, tbdf, recnum, aggr_base, (tptr)recnums);
  qs->kont.position=recnum;
  is_ptrs=2;
#if SQL3
  int ordnum = 0;
  top_qe->set_position_indep(cdp, recnums, ordnum);
#else
  top_qe->set_position(cdp, recnums);
#endif
  is_ptrs=FALSE;
 // evaluate the expressions:
  for (i=1;  i<qs->kont.elems.count();  i++)
  { elem_descr * el = qs->kont.elems.acc0(i);
    if (el->link_type==ELK_AGGREXPR ||
        el->link_type==ELK_EXPR && el->expr.eval->sqe==SQE_AGGR)
    { t_value res;
      expr_evaluate(cdp, el->expr.eval, &res);
      if (res.is_null()) qlset_null(&res, el->type, el->specif.length);
      tb_write_atr(cdp, tbdf, recnum, i, res.valptr());
    }
  }
}

#define MIN_MAX_FROM_INDEX


BOOL t_mater_table::min_max_from_index(cdp_t cdp, t_qe_specif * qs, trecnum & recnum, tptr key)
#ifdef MIN_MAX_FROM_INDEX
{ BOOL found_other_aggregate=FALSE;
  for (int i=0;  i<qs->agr_list.count();  i++)
  { t_agr_reg * agg=qs->agr_list.acc0(i);
    if (agg->index_number==-1)
      found_other_aggregate=TRUE;
    else
    { 
#if SQL3
      table_descr_auto perm_td(cdp, ((t_expr_attr*)agg->arg)->kont->t_mat->tabnum);
#else    
      table_descr_auto perm_td(cdp, ((t_mater_table*)((t_expr_attr*)agg->arg)->kont->mat)->tabnum);
#endif
      if (perm_td->me()) 
      { dd_index * indx = &perm_td->indxs[agg->index_number];
        cdp->index_owner = perm_td->tbnum;
        if (agg->first_value ? get_first_value(cdp, indx->root, indx, key) :
                               get_last_value (cdp, indx->root, indx, key)) // true iff value exists
        { if (recnum==NORECNUM) recnum=new_group(cdp, qs);
          tb_write_atr(cdp, tables[tabnum], recnum, aggr_base+1+i, key);
        }
      }
    }
  }
  return found_other_aggregate;
}
#else
{ return TRUE; }
#endif

trecnum t_mater_table :: new_group(cdp_t cdp, t_qe_specif * qs)
// Adds a new result record to the materialization. Returns recnum or
// NORECNUM on error.
{ int i;
 // adding a new result record:
  if (tabnum==NOTBNUM) return NORECNUM;  // temp. table destroyed by a Rollback
  table_descr * tbdf = tables[tabnum];
  trecnum recnum=tb_new(cdp, tbdf, TRUE, NULL);
  if (recnum==NORECNUM) return NORECNUM;
#if SQL3
  _record_count++;
#endif
 // init counts in the grouping record, must store 0 for both types of COUNTs!
  uns32 zero = 0;
  for (i=0;  i<qs->agr_list.count();  i++)
  { t_agr_reg * agg=qs->agr_list.acc0(i);
    if (agg->ag_type==S_COUNT)
      tb_write_atr(cdp, tbdf, recnum, aggr_base+1+i, &zero);
  }

 // if (qs->grouping_type==GRP_BY) -- this should be done for SINGLE group too, sets the values of constant columns and for empty table
  { cdp->check_rights=2; // raise error: NONE values if no privils
    //t_materialization * mat = qs->kont.mat;  qs->kont.mat=NULL; // prevents reading the attribute from the materialized result table -- done by !completed

   // init grouping values in the new record (SELECT elems not containing aggrs):
    for (i=1;  i<qs->kont.elems.count();  i++)
    { elem_descr * el = qs->kont.elems.acc0(i);
      if (el->link_type!=ELK_AGGREXPR &&
          (el->link_type!=ELK_EXPR || el->expr.eval->sqe!=SQE_AGGR))
        if (el->multi==1) // such elements in the SELECT are an error, but it is not tested
        { t_value res;
          attr_value(cdp, &qs->kont, i, &res, el->type, el->specif, NULL);
          if (!IS_HEAP_TYPE(el->type) && !IS_EXTLOB_TYPE(el->type))
          { if (res.is_null()) qlset_null(&res, el->type, el->specif.length);
            tb_write_atr(cdp, tbdf, recnum, i, res.valptr());
          }
          else
          { res.load(cdp);
            tb_write_var(cdp, tbdf, recnum, i, 0, res.length, res.valptr());
          }
        }
    }
   // store the position of a group representant:
    is_ptrs=2;
    trecnum recnums[MAX_OPT_TABLES];
#if SQL3
    int ordnum = 0;
    top_qe->get_position_indep(recnums, ordnum);
#else
    top_qe->get_position(recnums);
#endif
    is_ptrs=FALSE;
    tb_write_atr(cdp, tbdf, recnum, aggr_base, recnums);

    // qs->kont.mat=mat;  // restore the value
  }
  return recnum;
}

void t_mater_table :: record_in_group(cdp_t cdp, t_qe_specif * qs, trecnum recnum, tptr groupkey)
// Aggregates the current record into recnum in the result table.
{ int i;
  cdp->check_rights=2; // raise error: NONE values if no privils
  table_descr * tbdf = tables[tabnum];    // temporary table, tabdescr locked
 // storing or aggregating values:
  for (i=0;  i<qs->agr_list.count();  i++)
  { t_agr_reg * agg=qs->agr_list.acc0(i);
    if (agg->index_number!=-1 && qs->grouping_type==GRP_SINGLE)
      continue;  // value taken from index
    tfcbnum fcbn;
    tptr dt=fix_attr_write(cdp, tbdf, recnum, aggr_base+1+i, &fcbn);  
    if (dt)
    { if (agg->arg==NULL)  // COUNT(*)
        (*(uns32*)dt)++;
      else   /* not COUNT (SUM, MAX, MIN): */
      { t_value res;
        expr_evaluate(cdp, agg->arg, &res);
        if (!res.is_null())
        { if (agg->distinct)
          { memcpy(groupkey+agg->indx->keysize-agg->argsize-sizeof(trecnum), res.valptr(), agg->argsize);
            if (!distinctor_add(cdp, agg->indx, groupkey, tbdf->translog)) goto duplicate;
          }
          aggregate_value(dt, &res, agg);
        }
        else completion_condition(cdp, NULL_VALUE_ELIMINATED);
      }
     duplicate:
      unfix_page(cdp, fcbn);
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
d_table * WINAPI create_d_curs(cdp_t cdp, t_query_expr * qe)
// Prefix has to be stored for all attributes because it is used in some old apps.
{ d_attr * att;  elem_descr * el;  int i, j;
  int attrcnt = qe->kont.elems.count();
 // analyse prefixes:
  t_atrmap_flex prefmap(attrcnt);   
  if (cdp->sqloptions & SQLOPT_DUPLIC_COLNAMES)
  { prefmap.clear();
   // checking the duplicity twice in order to mark both conflicting columns in the prefmap:
    { t_name_hash name_hash(attrcnt);
      for (i=1;  i<attrcnt;  i++)
        if (!name_hash.add(qe->kont.elems.acc0(i)->name))
          prefmap.add(i);
    }
    { t_name_hash name_hash(attrcnt);
      for (i=attrcnt-1;  i>0;  i--)
        if (!name_hash.add(qe->kont.elems.acc0(i)->name))
          prefmap.add(i);
    }
  }
 // allocate d_curs:
  int tabcnt = 0;  ttablenum tabnums[MAX_CURS_TABLES+1];
  qe->get_table_numbers(tabnums, tabcnt);
  unsigned size=sizeof(d_table)+attrcnt*sizeof(d_attr)
                + MAX_CURS_TABLES*2*(OBJ_NAME_LEN+1); // may have more prefixes than tabcnt!!
  d_table * tdd=(d_table*)corealloc(size, 39);
  tobjname * prefs=(tobjname*)ATTR_N(tdd,attrcnt);
  int prefcnt=0;
  if (tdd!=NULL)
  { tdd->attrcnt=attrcnt;
    tdd->tabcnt =tabcnt;
    tdd->tabdef_flags=TFL_NO_JOUR;
    tdd->updatable=(uns16)qe->qe_oper;  // in 8.0c: contains updatability, insertability and deletability flags
    if (qe->maters >= MAT_INSTABLE) tdd->updatable=0;  // more exact information about a cursor (is not in qe_oper, employed in create_cursor)
    tdd->selfname[0]=tdd->schemaname[0]=0;
    for (i=0;  i<attrcnt;  i++)
    { att=ATTR_N(tdd,i);  el=qe->kont.elems.acc0(i);
      strcpy(att->name, el->name);
      att->type        =el->type;
      att->multi       =el->multi;
      att->nullable    =el->nullable;  // in 7.0 was wbtrue, but I want the correcnt answer
      att->specif      =el->specif;
     // flags:
      att->a_flags = /*cdp->in_admin_mode ? RI_SELECT_FLAG | RI_UPDATE_FLAG :*/ 0;  // this is not exact, admin mode may grant access to local tables only and it may disappear when the procedure creating cursor exits, but it it better to declare more access than less.
      // -- no, a_flags are used by the server too and have to be exact.
      if ((el->pr_flags & PRFL_READ_TAB )==PRFL_READ_TAB)  att->a_flags|=RI_SELECT_FLAG;
      if ((el->pr_flags & PRFL_WRITE_TAB)==PRFL_WRITE_TAB) att->a_flags|=RI_UPDATE_FLAG;
#ifdef STABILIZED_GROUPING
      if (el->pr_flags & ELFL_NOEDIT) att->a_flags |= NOEDIT_FLAG;  // this version is independent of the existence of materialised tables
#else
      db_kontext * akont = &qe->kont;  int tab_col;
      if (!resolve_access(akont, i, akont, tab_col))  // cannot resolve to the table column
        att->a_flags |= NOEDIT_FLAG;
#endif
     // prefix:
      att->needs_prefix=wbfalse;
      if (cdp->sqloptions & SQLOPT_DUPLIC_COLNAMES)
        if (prefmap.has(i))
          att->needs_prefix=wbtrue;
      j=0;  const char * el_prefix = elem_prefix(&qe->kont, el);
      while (j<prefcnt)
        if (!strcmp(prefs[j+1], el_prefix)) break;
        else j+=2;
      if (j==prefcnt)  // prefix not stored, add the prefix:
        { prefs[j][0]=0;  strcpy(prefs[j+1], el_prefix);  prefcnt+=2; }
      att->prefnum=j/2;
    }
   // update the structure size:
    size=sizeof(d_table)+attrcnt*sizeof(d_attr) + prefcnt*(OBJ_NAME_LEN+1);
    tdd=(d_table*)corerealloc(tdd, size);
    for (i=0;  i<tabcnt;  i++)
      if (tabnums[i]>=0 && tabnums[i]!=(ttablenum)0x8000 && tabnums[i]!=(ttablenum)0x8001 && tabnums[i]!=(ttablenum)0x8002)
      { table_descr * tbdf = tables[tabnums[i]];  // cursor opened -> tabdescrs locked
        if (tbdf && tbdf!=(table_descr*)-1)
        { if (tbdf->tabdef_flags & TFL_REC_PRIVILS)
            tdd->tabdef_flags |= TFL_REC_PRIVILS;
          if (!i && (tbdf->tabdef_flags & TFL_TOKEN))
            tdd->tabdef_flags |= TFL_TOKEN;
        }
      }
  }
  return tdd;
}

void WINAPI index_expr_comp2(CIp_t CI)
{ dd_index * indx = (dd_index*)CI->univ_ptr;
  CI->atrmap=&indx->atrmap;
  indx->compile(CI); 
  CI->atrmap=NULL;
}

void WINAPI check_comp_link(CIp_t CI)
{ dd_check * chk = (dd_check *)CI->univ_ptr;
  CI->atrmap=&chk->atrmap;
  search_condition(CI, &chk->expr); 
  CI->atrmap=NULL;
}

void WINAPI default_value_link(CIp_t CI)
{ attribdef * att = (attribdef *)CI->univ_ptr;
  sqlbin_expression(CI, &att->defvalexpr, FALSE); 
 // reinterpreting USER in binary column:
  if (att->attrtype==ATT_BINARY && att->defvalexpr->sqe==SQE_FUNCT && ((t_expr_funct*)att->defvalexpr)->num==FCNUM_USER)
  { ((t_expr_funct*)att->defvalexpr)->num=FCNUM_USER_BINARY;
    att->defvalexpr->type         =ATT_BINARY;
    att->defvalexpr->specif.length=UUID_SIZE;
  }
  convert_value(CI, att->defvalexpr, att->attrtype, att->attrspecif);
  if (CI->cursym != S_SOURCE_END) c_error(GARBAGE_AFTER_END);
}

BOOL WINAPI compile_constrains(cdp_t cdp, const table_all * ta, table_descr * td)
// Compiles text parts of ta to td
// Errors in check constrains and default values converted to warnings, they may be caused by objects not imported yet.
// Supposes that ta->schemaname is defined.
// Is not compatible with temporary tables!
{ int i, res;   BOOL gres=FALSE;
 // setup the aplication kontext of the table:
  t_short_term_schema_context stsc(cdp, td->schema_uuid);

 // search parent table and index for every RI:
  for (i=0;  i<ta->refers.count();  i++)
  { const forkey_constr * ref = ta->refers.acc0(i);
    dd_forkey * ddf = &td->forkeys[i];
    BOOL found_parent_table;
    if (*ref->desttab_schema)
    { found_parent_table=!name_find2_obj(cdp, ref->desttab_name, ref->desttab_schema, CATEG_TABLE, &ddf->desttabnum);
      if (!found_parent_table) ddf->desttabnum=NOOBJECT;
    }
    else 
    { ddf->desttabnum=find2_object(cdp, CATEG_TABLE, td->schema_uuid, ref->desttab_name);
      found_parent_table=ddf->desttabnum!=NOOBJECT;
    }
    if (found_parent_table)
    { table_all par_ta;
      res=partial_compile_to_ta(cdp, ddf->desttabnum, &par_ta);
      if (res) { request_error(cdp, res);  gres=TRUE;  goto exit; }
      ddf->par_index_num = find_index_for_key(cdp, ref->par_text, &par_ta);
    }
    else
      ddf->par_index_num = -1;
    // { request_error(cdp, NO_PARENT_INDEX);  gres=TRUE;  goto exit; }  -- no error, must not block the dependent table on problems with parent table
  }

  td->check_cnt=0;
  for (i=0;  i<ta->checks.count(); i++)
  { const check_constr * check = ta->checks.acc0(i);
    dd_check * cc = &td->checks[td->check_cnt++];
    strcpy(cc->constr_name, check->constr_name);
    cc->deferred=check->co_cha==COCHA_UNSPECIF && (cdp->sqloptions & SQLOPT_CONSTRS_DEFERRED)
              || check->co_cha==COCHA_DEF;
    cc->expr=NULL;
    cc->atrmap.clear();
    compil_info xCI(cdp, check->text, check_comp_link);
    t_sql_kont sql_kont;  sql_kont.kont_ta=ta;  xCI.sql_kont=&sql_kont;
    sql_kont.column_number_for_value_keyword = check->column_context;
    xCI.univ_ptr=cc;
    if (compile_masked(&xCI)) // error in check clause does not disable the table
    { if (!(cdp->ker_flags & KFL_DISABLE_REFINT)) // disables error messages when importing/deleting a schema
      { char buf[2*OBJ_NAME_LEN+100];
        form_message(buf, sizeof(buf), msg_check_constr_error, check->constr_name, ta->selfname);
        trace_msg_general(cdp, TRACE_BCK_OBJ_ERROR, buf, NOOBJECT);
      }
      warn(cdp, ERROR_IN_CONSTRS);  delete cc->expr;  cc->expr=NULL; 
    } 
  }

 // compile default values of columns (compiled into att from td):
  for (i=0;  i<ta->attrs.count();  i++)
  { const atr_all * al = ta->attrs.acc0(i);
    attribdef * att = td->attrs+i;
    if (al->defval!=NULL)
      if (!stricmp((tptr)al->defval, "UNIQUE"))
        if (att->attrtype==ATT_INT32 || att->attrtype==ATT_INT16 || att->attrtype==ATT_INT8 || att->attrtype==ATT_INT64 || att->attrtype==ATT_MONEY)
          att->defvalexpr=COUNTER_DEFVAL;
        else
          warn(cdp, ERROR_IN_DEFVAL);  
      else
      { compil_info xCI(cdp, al->defval, default_value_link);
        t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  // must be defined, even with empty kontext
        xCI.univ_ptr=att;
        int res=compile_masked(&xCI);
        if (res) //return TRUE;
        { if (!(cdp->ker_flags & KFL_DISABLE_REFINT)) // disables error messages when importing/deleting a schema
          { char buf[2*OBJ_NAME_LEN+100];
            form_message(buf, sizeof(buf), msg_defval_error, al->name, ta->selfname);
            trace_msg_general(cdp, TRACE_BCK_OBJ_ERROR, buf, NOOBJECT);
          }
          warn(cdp, ERROR_IN_DEFVAL);  
          delete att->defvalexpr;  att->defvalexpr=NULL;
        }
      }
  }

 exit: 
  return gres;
}

//////////////////////////////// cursor level ////////////////////////////////
crs_dynar crs;

static bool skip_keyword(const char * & source, const char * keyword)
// If source startes with the keyword, advances source ptr and returns true. Otherise returns false.
// Keyword must be in upper case and must not contain national characters
// If keyword==NULL then it matches any identifier.
{ const char * src = source;
 // skip whitespaces:
  do
  { if (*src==' ' || *src=='\r' || *src=='\n' || *src=='\t')
      src++;
    else if (*src=='{')
    { do src++;
      while (*src && *src!='}');
      if (*src=='}') src++;
    }
    else break;
  } while (TRUE);
 // check for the keyword:
  if (keyword)
  { char keystart = *keyword;
    while (*keyword)
      if (*keyword == *src || *src>='a' && *src<='z' && *keyword == (*src & 0xdf)) { keyword++;  src++; }
      else return false;
    if (sys_Alpha(keystart) || keystart>='0' && keystart<='9' || keystart=='_')
      if (*src>='A' && *src<='Z' || *src>='a' && *src<='z' || *src=='_') return false;
  }
  else // skipping any identifier
    if (*src=='`')
    { do 
        if (!*++src) return false;
      while (*src!='`');
      src++;
    }
    else
    { if (!sys_Alpha(*src)) return false;
      do src++;
      while (sys_Alpha(*src) || *src>='0' && *src<='9' || *src=='_');
    }
  source=src;
  return true;
}

tptr assemble_subcursor_def(const char * subinfo, char * orig_source, BOOL from_table)
{ const char * ordr;  int wh_len;
  if (orig_source==NULL) return NULL;
 /* analyse the subcurs description: */
  while (*subinfo==' ') subinfo++;  // eliminating empty WHERE clause
  wh_len=(int)strlen(subinfo);
  ordr=(tptr)memchr(subinfo, ORDER_BYTEMARK, wh_len);
  if (ordr!=NULL) { wh_len=(int)(ordr-subinfo);  ordr++; }
 // analyse orig_source:
  const char * part2 = orig_source;
  skip_keyword(part2, "INSENSITIVE");
  skip_keyword(part2, "SENSITIVE");
  skip_keyword(part2, "SCROLL");
  skip_keyword(part2, "LOCKED");
  skip_keyword(part2, "CURSOR");
  skip_keyword(part2, "FOR");
  if (skip_keyword(part2, "VIEW"))
  { skip_keyword(part2, NULL);
    if (skip_keyword(part2, ".")) skip_keyword(part2, NULL);
    if (skip_keyword(part2, "("))  // the syntax in accepted but it will not work, because the new names of columns are used in the subquery conditions
    { do skip_keyword(part2, NULL);
      while (skip_keyword(part2, ","));
      skip_keyword(part2, ")");
    }
    skip_keyword(part2, "AS");
  }
 // allocate and create subcursor:
  tptr s = (tptr)corealloc(strlen(orig_source)+strlen(subinfo)+40, 87);
  unsigned pos;
  if (s!=NULL)
  { memcpy(s, orig_source, part2-orig_source);  pos=(unsigned)(part2-orig_source);
    strcpy(s+pos, " SELECT ** FROM ");  pos+=(int)strlen(s+pos);
    if (!from_table) s[pos++]='(';
    strcpy(s+pos, part2);              pos+=(int)strlen(s+pos);
    if (!from_table) 
    { s[pos++]='\r';  s[pos++]='\n';  // just for the case that the query ends with // without CRLF
      s[pos++]=')';
    }
    if (wh_len)
    { strcpy(s+pos, " WHERE ");        pos+=7;
      memcpy(s+pos, subinfo, wh_len);  pos+=wh_len;
    }
    if (ordr!=NULL)
    { s[pos++]=' ';
      strcpy(s+pos, ordr);
    }
    else s[pos]=0;
  }
  return s;
}

void cur_descr::curs_seek(trecnum recnum)
// Materialization is supposed.
{ //position=recnum;// must not store position in INTERS, EXCEPT
#if SQL3
#ifdef UNSTABLE_PMAT
  int ordnum = 0;
  qe->set_position_indep(cdp, &recnum, ordnum);
#else
  if (pmat!=NULL)
    set_pmat_position(cdp, qe, recnum, pmat);  // translates via p_mat
  else
  { int ordnum = 0;
    qe->set_position_indep(cdp, &recnum, ordnum);
  }
#endif
#else // !SQL3
  if (pmat!=NULL)
  { const trecnum * recnums = pmat->recnums_pos_const(cdp, recnum);
    if (recnums==NULL) return;
    pmat->top_qe->set_position(cdp, recnums);
    if (qe->qe_type!=QE_TABLE && qe->qe_type!=QE_UNION && qe->qe_type!=QE_INTERS && qe->qe_type!=QE_EXCEPT) // must not overwrite the direction info in this queries
      qe->kont.position=recnum; // used when checking the position change in subqueries like UPDATE Cenik SET cena = (SELECT nova_cena FROM Katalog WHERE Katalog.id_m = Cenik.id) WHERE EXISTS (SELECT nova_cena FROM Katalog WHERE Katalog.id_m=Cenik.id)
  }
  else if (qe->kont.mat!=NULL)
    qe->kont.position=recnum;
  else // special case: top qe not materialized, must send recnum below
  { t_qe_specif * qes = (t_qe_specif *)qe;
    do
    { t_query_expr * aqe=qes->from_tables;
      if (aqe->kont.mat!=NULL)
      { if (aqe->kont.mat->is_ptrs)
          aqe->set_position(cdp, pmat->recnums_pos_const(cdp, recnum));  // this must crash, pmat is NULL
        else
          aqe->kont.position=recnum;
        return;
      }
      qes=(t_qe_specif *)aqe;
    } while (qes->qe_type==QE_SPECIF);
  }
#endif
}

BOOL cur_descr::curs_seek(trecnum crec, trecnum * recnums) 
{ if (pmat==NULL) { request_error(cdp, CANNOT_APPEND);  return TRUE; }
  if (curs_eseek(crec)) return TRUE;
#if SQL3   // get_position_indep does not work, returns untranslated top position
  const trecnum * pmat_recnums = pmat->recnums_pos_const(cdp, crec);
  memcpy(recnums, pmat_recnums, sizeof(trecnum)*pmat->kont_cnt);
#else
  qe->get_position(recnums);
#endif
  return FALSE;
}

BOOL cur_descr::curs_eseek(trecnum rec)
// Enlarge and seek, return TRUE on error
{ if (rec >= recnum)
  { prof_time start;
    if (source_copy) start = get_prof_time();
    enlarge_cursor(rec+1);
    if (PROFILING_ON(cdp) && source_copy) add_prof_time(get_prof_time()-start, PROFCLASS_SQL, 0, 0, source_copy);
    if (rec >= recnum) { request_error(cdp, OUT_OF_TABLE);  return TRUE; }
  }
  curs_seek(rec);
  return FALSE;
}

void cur_descr::enlarge_cursor(trecnum limit)
// Adds new records into the cursor
{ prof_time start, saved_lower_level_time, orig_lock_waiting_time;
#if SQL3
  if (opt->eof) { complete=true;  return; }
#endif
  if (complete) return;
  if (cdp->is_an_error()) return;  // cannot enlarge but should not set the complete flag!
  
  if (source_copy) 
  { saved_lower_level_time = cdp->lower_level_time;  cdp->lower_level_time=0;
    orig_lock_waiting_time = cdp->lock_waiting_time;
    start = get_prof_time();  
  }

  t_temporary_current_cursor tcc(cdp, this);  // sets the temporary current_cursor in the cdp until destructed
  passing_open_close(cdp, opt, TRUE);
  pmat->completed=FALSE;  // temp., allows access to the lower leves
  if (is_saved_position)
  { int ordnum = 0;
    qe->set_position_indep(cdp, saved_position, ordnum);
  }
  while (pmat->rec_cnt < limit)
  { if ((opt->*opt->p_get_next_record)(cdp) && !cdp->is_an_error())
    {
#if SQL3   // must not add records to the ORDER materialisation
      if (!pmat->completed)
#endif
        pmat->add_current_pos(cdp);
    }
    else
    { complete=TRUE;  
      break; 
    }
  }
  if (!complete)
  { int ordnum = 0;
    qe->get_position_indep(saved_position, ordnum);
    is_saved_position=true;
  }
  pmat->completed=TRUE;  // closes access to the lower leves
  passing_open_close(cdp, opt, FALSE);

  if (PROFILING_ON(cdp) && source_copy) 
  { prof_time interv = get_prof_time()-start;
    add_prof_time2(interv, cdp->lower_level_time, cdp->lock_waiting_time-orig_lock_waiting_time, PROFCLASS_SQL, 0, 0, source_copy);
    cdp->lower_level_time = saved_lower_level_time + interv;
  }

#if SQL3
  recnum =  qe->kont.p_mat->record_count();
#else
  recnum=pmat->rec_cnt;
#endif
}

void make_complete(cur_descr * cd)
{ cd->enlarge_cursor(NORECNUM); }

tcursnum register_cursor(cdp_t cdp, cur_descr * cd)
{ int i;  cur_descr ** pcd = NULL;
  int cnt = count_cursors(cdp->applnum_perm);
  if (cnt!=cdp->open_cursors)
    dbg_err("abcs");
  if (cdp->open_cursors >= the_sp.max_client_cursors.val() && the_sp.max_client_cursors.val()!=0)
  { tobjname err_param;
    sprintf(err_param, "MaxClientCursors=%d", the_sp.max_client_cursors.val());
    SET_ERR_MSG(cdp, err_param);
    request_error(cdp, LIMIT_EXCEEDED);
    return NOCURSOR;
  }
  { ProtectedByCriticalSection cs(&crs_sem, cd->cdp, WAIT_CS_CURSORS);
    for (i=0;  i<crs.count();  i++)
      if (*crs.acc0(i)==NULL)
        { pcd = crs.acc0(i);  break; }
    if (!pcd) pcd=crs.next();  // i==crs.count() here
    if (pcd) *pcd=cd;
  }
  if (!pcd)
    { request_error(cd->cdp, OUT_OF_KERNEL_MEMORY);  return NOCURSOR; }
  
 // make cursnum:
  tcursnum cursnum = i | CURS_USER;
 // trace:
  if (cd->cdp->trace_enabled & TRACE_CURSOR) 
  { char buf[81];
    trace_msg_general(cd->cdp, TRACE_CURSOR, form_message(buf, sizeof(buf), msg_cursor_open, cursnum), NOOBJECT);
  }
  cdp->open_cursors++;
  return cursnum;
}

void free_cursor(cdp_t cdp, tcursnum curs)  /* unregisters and deletes cursor */
{ cur_descr * cd;
  int cnt = count_cursors(cdp->applnum_perm);
  if (cnt!=cdp->open_cursors)
    dbg_err("abc");
  curs = GET_CURSOR_NUM(curs);
  { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
    if (curs > crs.count()) return;
    cd = *crs.acc0(curs);
    if (cd==NULL) return;
    *crs.acc0(curs)=NULL;
  }
 // trace:
  if (cd->cdp->trace_enabled & TRACE_CURSOR) 
  { char buf[81];
    trace_msg_general(cd->cdp, TRACE_CURSOR, form_message(buf, sizeof(buf), msg_cursor_close, curs | CURS_USER), NOOBJECT);
  }
#ifdef STOP  // not necessary because d_ins_curs is not used in the current version
 // remove from the cursor insertion log:
  for (int i=0;  i<cdp->d_ins_curs.count(); )
  { ins_curs * ic = cdp->d_ins_curs.acc0(i);
    if (GET_CURSOR_NUM(ic->cursnum) == curs)
      cdp->d_ins_curs.delet(i);
    else i++;
  }
#endif
 // transaction log:
  unregister_cursor_creation(cd->cdp, cd); 
  { t_temporary_current_cursor tcc(cdp, cd);  // sets the temporary current_cursor in the cdp until destructed
    if (cd->opt!=NULL) cd->opt->close_data(cdp);
  }
  delete cd;
  cdp->open_cursors--;
}

cur_descr :: ~cur_descr(void)
{ safefree((void**)&d_curs);
  if (qe!=NULL) qe->lock_unlock_tabdescrs(cdp, false);
  if (opt!=NULL) 
  { //if (opt->ref_cnt>1) opt->close_data(SaferGetCurrTaskPtr); -- done in free_cursor
    opt->Release();  // may containt pointers to qe
  }
  if (qe !=NULL) qe ->Release();  
  qe=NULL;  opt=NULL;
  safefree((void**)&source_copy);
  safefree((void**)&tabnums);
  destroy_subtrans(cdp, &translog);
}

void cur_descr::mark_core(void)   // marks itself too
{ mark(d_curs);
  if (opt!=NULL) opt->mark_core();
  if (qe !=NULL) qe ->mark_core();  
  if (source_copy) mark(source_copy);
  if (tabnums) mark(tabnums);
  mark(this);
}

#if !SQL3
static t_mater_ptrs * make_qe_ident(cdp_t cdp, cur_descr * cd)
// Creates qe_ident on the top of cd->qe and stores it in cd->qe.
// Returns pmat of the created qe_out, NULL on error.
{ t_qe_ident * qe_out = new t_qe_ident(QE_IDENT);
  if (qe_out==NULL) return NULL;
 // add pointer materialization to qe_out:
  qe_out->mat_extent=cd->qe->mat_extent;  // used by mat->init
  cd->qe->rec_extent=1;  cd->qe->kont_start=0;  cd->qe->kont.ordnum=0;  // inherited params of qe
  t_mater_ptrs * mat = new t_mater_ptrs(cdp);
  if (mat==NULL) { delete qe_out;  return NULL; }
  qe_out->kont.mat = mat;
  if (!mat->init(qe_out)) { delete qe_out;  return NULL; }
 // copy cd->qe data to qe_out:
  qe_out->kont.copy_kont(NULL, &cd->qe->kont, 0);
  qe_out->qe_oper=cd->qe->qe_oper;
 // replace qe in cd by qe_out:
  qe_out->op=cd->qe;
  cd->qe=qe_out;
  return mat;
}
#endif

BOOL write_elem(cdp_t cdp, t_query_expr * qe, int elemnum, table_descr * dest_tbdf, trecnum recnum, tattrib attr)
// Writes kontext element to a table
{ elem_descr * el = qe->kont.elems.acc0(elemnum);
  int tp = el->type;  if (el->multi!=1) tp |= 0x80;
  t_value res;
  attr_value(cdp, &qe->kont, elemnum, &res, tp, el->specif, NULL);
  if (res.is_null()) qlset_null(&res, el->type, el->specif.length);
  return write_value_to_attribute(cdp, &res, dest_tbdf, recnum, attr, NULL, tp, tp);
}

t_order_by * extract_ordering(cdp_t cdp, t_query_expr * qe)
{// look for ORDER BY near the top:
  t_order_by * p_order = find_ordering(qe);
 // analyze the ORDER BY expression:
  if (p_order!=NULL)
  { p_order->order_tabkont=NULL;  p_order->order_indnum=-1;
   // classify the expressions in the index:
    t_refinfo ri(NULL, RIT_UNION_BOTH);
    for (int i=0;  i<p_order->order_indx.partnum;  i++)
      get_reference_info((t_express*)p_order->order_indx.parts[i].expr, &ri);

    if (ri.stat==TRS_SINGLE)
    { p_order->order_tabkont=ri.onlytabkont;
#if SQL3
      ttablenum tbnum = ri.onlytabkont->t_mat->tabnum;
#else
      ttablenum tbnum = ((t_mater_table*)(ri.onlytabkont->mat))->tabnum;
#endif
      table_descr_auto td(cdp, tbnum, TABLE_DESCR_LEVEL_COLUMNS_INDS);
      if (td->me()!=NULL)
      { dd_index * cc;  int ind;
        for (ind=0, cc=td->indxs;  ind<td->indx_cnt;  ind++, cc++)
          if (!cc->disabled)
            if (comp_index_and_order(cc, 0, &p_order->order_indx))
              { p_order->order_indnum=ind;  break; }
      }
    }
  }
  return p_order;
}

t_optim * optimize_qe(cdp_t cdp, t_query_expr * qe)
{ t_order_by * p_order = extract_ordering(cdp, qe);
  t_optim * opt = alloc_opt(cdp, qe, p_order, false);
  if (opt==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  opt->optimize(cdp, TRUE);
  if (opt->error) { delete opt;  return NULL; }
#if SQL3
  if (opt->optim_type==OPTIM_ORDER)
    ((t_optim_order*)opt)->sort_disabled = !p_order->post_sort;
#endif
  if (qe->lock_rec) propagate_locking_flag(opt);
  return opt;
}

#if SQL3

tcursnum create_cursor(cdp_t cdp, t_query_expr * qe, tptr source, t_optim * opt, cur_descr ** pcd)
// Allocates the cursor structure referencing qe and optim.
// Creates top table materialisation for unstable cursors and cursors marked as INSENSITIVE
// Creates top pointer materialisation otherwise.
//  (... unless materialized inside: e.g. in grouping or ORDER BY over instable/insentitive)
// Allocates the cursor number iff all is successfull.
{ tcursnum cursnum;  int tabcount;
 // create cur_descr:
  cur_descr * cd = *pcd = new cur_descr(cdp, qe, opt);
  if (!cd) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NOCURSOR; }
  cd->insensitive=FALSE;  // may be changed later
  t_temporary_current_cursor tcc(cdp, cd);  // sets the temporary current_cursor in the cdp until destructed
  if (qe->lock_unlock_tabdescrs(cdp, true))  // must be on the beginning, revers operation is in the destructor of cur_descr
    { delete cd;  *pcd=NULL;  return NOCURSOR; }
 // register among transaction objects:
  register_cursor_creation(cdp, cd);
 // remove old mater:
  if (qe->kont.p_mat)  // must delete, otherwise it would be taken "as is"
    { qe->kont.p_mat->clear(cdp);  delete qe->kont.p_mat;  qe->kont.p_mat=NULL; }
 // Refs to qe and opt added iff cd created.
  (opt->*opt->p_reset)(cdp);
 // find existing materialization and use it or create a new one:
  bool distinct_on_route = false;
  t_query_expr * aqe;  aqe = qe;  
  while (aqe->qe_type==QE_SPECIF && !aqe->kont.p_mat && !aqe->kont.t_mat)
  { t_qe_specif * sqe = (t_qe_specif*)aqe;
    if (sqe->distinct) distinct_on_route=true;
    if (!sqe->from_tables) break;
    aqe=sqe->from_tables;
  }
  if (aqe->kont.p_mat && opt->optim_type!=OPTIM_LIMIT && !distinct_on_route)
  { cd->recnum = aqe->kont.p_mat->record_count();
    cd->complete   =TRUE;
    cd->pmat = aqe->kont.p_mat;
  }
  else if (aqe->kont.t_mat && !aqe->kont.t_mat->permanent && !distinct_on_route)
  { cd->recnum = aqe->kont.t_mat->record_count();
    cd->own_table= aqe->kont.t_mat->tabnum;
    cd->complete   =TRUE;
    cd->insensitive=TRUE;
  }
  else if (qe->maters >= MAT_INSTABLE)  // table materialization of "insensitive" or instable cursors:
  { t_mater_table * mat = new t_mater_table(cdp, FALSE);
    if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
    qe->kont.t_mat = mat;
    if (!mat->init(cdp, qe)) goto err;
    mat->completed=FALSE;
    ttablenum temptab=mat->tabnum;  table_descr * temptab_descr = tables[temptab];
    //(opt->*opt->p_reset)(cdp);  -- is above
    qe->kont.t_mat = NULL;
    uns32 old_flags = cdp->ker_flags;
    cdp->ker_flags |= KFL_ALLOW_TRACK_WRITE;
    while (!opt->error && (opt->*opt->p_get_next_record)(cdp))
    { trecnum tab_recnum=tb_new(cdp, temptab_descr, TRUE, NULL);
      if (tab_recnum==NORECNUM) opt->error=TRUE;
      else
      { mat->_record_count++;
        for (int i=1;  i<qe->kont.elems.count();  i++)
          if (write_elem(cdp, qe, i, temptab_descr, tab_recnum, i))
            opt->error=TRUE;
      }
    }
    cdp->ker_flags=old_flags;
    qe->kont.t_mat = mat;
    cd->own_table= temptab;
    cd->recnum   = mat->record_count();  // valid records
    mat->completed=TRUE;
    cd->complete   =TRUE;
    cd->insensitive=TRUE;
    cd->pmat = qe->kont.p_mat;
  }
  else  // create top ptr materialisation unless created on demand
  { cd->recnum = 0;
    if (!create_or_clear_pmat(cdp, qe)) goto err;
    cd->complete   =FALSE;
    cd->pmat = qe->kont.p_mat;
    cd->pmat->completed=TRUE;  // closes access to the lower leves, will be changed during enlarge_cursor
  }
  passing_open_close(cdp, opt, FALSE);  // important only when not closed inside (not closed when cursor is not completed)
 // create d_curs (should be done after materializing internal temp. tables, otherwise cannot correctly evaluate the updatability of columns):
  cd->d_curs=create_d_curs(cdp, qe);
  if (!cd->d_curs) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
  if (cd->insensitive) cd->d_curs->updatable=0;  // grouped or materialised because of instability
  cd->tabcnt=cd->d_curs->tabcnt;
 // find the underlying table (not a cursor temp. table but possible yes for procedure temp tables?):
  aqe = qe;  //distinct=FALSE;
  do
  { if (aqe->qe_type==QE_SPECIF && ((t_qe_specif*)aqe)->grouping_type==GRP_NO)
    { //if (((t_qe_specif*)aqe)->distinct) distinct=TRUE;
      if (!((t_qe_specif*)aqe)->from_tables) break;
      aqe = ((t_qe_specif*)aqe)->from_tables;
    }
    else if (aqe->qe_type==QE_EXCEPT || aqe->qe_type==QE_INTERS) 
      aqe = ((t_qe_bin *)aqe)->op1;
    else break;
  } while (true);
  if (aqe->qe_type==QE_TABLE) cd->underlying_table = (t_qe_table*)aqe;
 // create list of table numbers (for UNION, INTERSECT, EXCEPT only the 1st operand is stored):
  if (cd->tabcnt)
  { cd->tabnums=(ttablenum*)corealloc(sizeof(ttablenum) * cd->tabcnt, 61);
    if (cd->tabnums==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
    tabcount = 0;
    qe->get_table_numbers(cd->tabnums, tabcount);
  }  
 // registering:
  cursnum = register_cursor(cdp, cd);
  if (cursnum==NOCURSOR) goto err;
  if (cdp->sel_stmt) cd->hstmt=cdp->sel_stmt->hstmt;  // must save and restore when reading from the cursor etc.
  //if (cd->complete) cd->release_opt(); -- nelze, potrebuji opt kvuli close_data() pri zavrani kurzoru
 // make source copy:
  if (source!=NULL)
  { cd->source_copy=(tptr)corealloc(strlen(source)+1, 85);
    if (cd->source_copy!=NULL) strcpy(cd->source_copy, source);
  }
  cdp->cnt_cursors++;
  return cursnum;
 err:
  unregister_cursor_creation(cdp, cd);
  opt->close_data(cdp);  // temp. tables etc. may have bee created, must destroy them
  delete cd;  *pcd=NULL;
  return NOCURSOR;
}

#else
tcursnum create_cursor(cdp_t cdp, t_query_expr * qe, tptr source, t_optim * opt, cur_descr ** pcd)
{ tcursnum cursnum;  BOOL post_sorting, distinct;  int tabcount;
  t_order_by * p_order = find_ordering(qe);
 // create cur_descr:
  cur_descr * cd = *pcd = new cur_descr(cdp, qe, opt);
  if (!cd) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NOCURSOR; }
  t_temporary_current_cursor tcc(cdp, cd);  // sets the temporary current_cursor in the cdp until destructed
  if (qe->lock_unlock_tabdescrs(cdp, true))  // must be on the beginning, revers operation is in the destructor of cur_descr
    { delete cd;  *pcd=NULL;  return NOCURSOR; }
 // register among transaction objects:
  register_cursor_creation(cdp, cd);
  cdp->ker_flags |= KFL_DISABLE_ROLLBACK;
  // Refs to qe and opt added iff cd created.

 // find the underlying table (not a cursor temp. table but possible yes for procedure temp tables?):
  t_query_expr * aqe;  aqe = qe;  distinct=FALSE;
  while (aqe->qe_type==QE_SPECIF && ((t_qe_specif*)aqe)->grouping_type==GRP_NO)
  { if (((t_qe_specif*)aqe)->distinct) distinct=TRUE;
    aqe=((t_qe_specif*)aqe)->from_tables;
  }
  if (aqe->qe_type==QE_TABLE) cd->underlying_table=(t_qe_table*)aqe;

  reset_curr_date_time(cdp);
  post_sorting = p_order && p_order->post_sort;
  if (qe->kont.mat!=NULL)   // from previous create_cursor
    { delete qe->kont.mat;  qe->kont.mat=NULL; }

 // evaluate the limitations:
  trecnum limit_count, limit_offset;
  get_limits(cdp, qe, limit_count, limit_offset);

 // create the cursor:
  if (opt->optim_type==OPTIM_GROUPING)
  { BOOL upper_ptr_layer = post_sorting;  ttablenum temptab;
   // create and fill the temporary table:
    t_qe_specif * gqes = (t_qe_specif*)aqe; // OK, must be grouping qe_specif
    if (gqes->distinct) distinct=TRUE;
    if (distinct || gqes->having_expr!=NULL || opt->inherited_conds.count())
      upper_ptr_layer=TRUE;
    (opt->*opt->p_reset)(cdp);  // creates the materialized table
    if (opt->error) goto err;
    temptab=((t_mater_table*)gqes->kont.mat)->tabnum;

    if (upper_ptr_layer)
    // must create another materialization level
    { t_mater_ptrs * pmat = make_qe_ident(cdp, cd);
      if (pmat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
      cd->pmat=pmat;
      if (post_sorting)
      { if (sort_cursor(cdp, &p_order->order_indx, opt, pmat, ((t_optim_grouping*)opt)->group_count,
                        &((t_optim_grouping*)opt)->qeg->kont, limit_count, limit_offset)) goto err;
      }
      else
        while ((opt->*opt->p_get_next_record)(cdp))
        // records satisfying HAVING condition & inherited conds
        { if (limit_offset) limit_offset--;
          else 
          { if (!limit_count) break;
            if (limit_count!=NORECNUM) limit_count--;
            pmat->add_current_pos(cdp);
          }
        }
      cd->recnum=pmat->rec_cnt;
    }
    else // direct use of materialized table
    { cd->own_table=temptab;
      cd->recnum=((t_optim_grouping*)opt)->group_count;
      if (limit_offset || limit_count!=NORECNUM)  // delete some records
      { table_descr * tbdf = tables[temptab];
        for (trecnum r=0;  r<cd->recnum;  r++)
        { uns8 del=table_record_state(cdp, tbdf, r);
          if (del==NOT_DELETED)
          { if (limit_offset) 
            { limit_offset--;
              tb_del(cdp, tbdf, r);
            }
            else if (!limit_count)
              tb_del(cdp, tbdf, r);
            else if (limit_count!=NORECNUM) limit_count--;
          }
        }
      }
    }
    cd->complete   =TRUE;
    cd->insensitive=TRUE;
  }

  else if (qe->maters >= MAT_INSTABLE)  // create the materialization 
  { t_mater_table * mat = new t_mater_table(cdp, FALSE);
    if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
    qe->kont.mat = mat;
    if (!mat->init(cdp, qe)) goto err;
    mat->completed=FALSE;
    ttablenum temptab=mat->tabnum;  table_descr * temptab_descr = tables[temptab];
    (opt->*opt->p_reset)(cdp);
    if (post_sorting)
    { t_mater_ptrs * pmat = make_qe_ident(cdp, cd);
      if (pmat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
      cd->pmat=pmat;
      qe->kont.mat = NULL;
      if (curs_mater_sort(cdp, &p_order->order_indx, cd->qe, opt, temptab, limit_count, limit_offset)) opt->error=TRUE;
      qe->kont.mat = mat;
      cd->recnum=pmat->rec_cnt;
    }
    else
    { qe->kont.mat = NULL;
      uns32 old_flags = cdp->ker_flags;
      cdp->ker_flags |= KFL_ALLOW_TRACK_WRITE;
      while (!opt->error && (opt->*opt->p_get_next_record)(cdp))
      { if (limit_offset) limit_offset--;
        else 
        { if (!limit_count) break;
          if (limit_count!=NORECNUM) limit_count--;
          trecnum tab_recnum=tb_new(cdp, temptab_descr, TRUE, NULL);
          if (tab_recnum==NORECNUM) opt->error=TRUE;
          else
          for (int i=1;  i<qe->kont.elems.count();  i++)
            if (write_elem(cdp, qe, i, temptab_descr, tab_recnum, i))
              opt->error=TRUE;
        }
      }
      cdp->ker_flags=old_flags;
      qe->kont.mat = mat;
      cd->own_table= temptab;
      cd->recnum   = temptab_descr->Recnum(); // temporary table -> tabdescr locked
    }
    mat->completed=TRUE;
    cd->complete   =TRUE;
    cd->insensitive=TRUE;
  }                                       

  else // create the pointer-materialization in qe:
  { t_mater_ptrs * mat = new t_mater_ptrs(cdp);
    if (mat==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
    qe->kont.mat = cd->pmat = mat;
    if (!mat->init(qe)) goto err;
    if (post_sorting)
    { if (create_curs_sort(cdp, &p_order->order_indx, qe, opt, limit_count, limit_offset)) goto err;
      cd->recnum=mat->rec_cnt;
      cd->complete=TRUE;
    }
    else
    { (opt->*opt->p_reset)(cdp);
      cd->recnum=0;
      cd->complete=FALSE;
      while (limit_offset && (opt->*opt->p_get_next_record)(cdp) && !cdp->is_an_error())
        limit_offset--;
      if (limit_count!=NORECNUM)
      { while (limit_count)
        { if ((opt->*opt->p_get_next_record)(cdp) && !cdp->is_an_error())
            mat->add_current_pos(cdp);
          else break;
          cd->recnum++;
          limit_count--;
        }
        cd->complete=TRUE;  
      }
      else ; // cursor not completed yet
      passing_open_close(cdp, opt, FALSE);
    }
    cd->insensitive=FALSE;
  }
  if (cdp->is_an_error()) goto err; // do not register it
 // create d_curs (should be done after materializing internal temp. tables, otherwise cannot correctly evaluate the updatability of columns):
  cd->d_curs=create_d_curs(cdp, qe);
  if (!cd->d_curs) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
  if (cd->insensitive) cd->d_curs->updatable=0;  // grouped or materialised because of instability
  cd->tabcnt=cd->d_curs->tabcnt;
 // create list of table numbers (for UNION, INTERSECT, EXCEPT only the 1st operand is stored):
  cd->tabnums=(ttablenum*)corealloc(sizeof(ttablenum) * cd->tabcnt, 61);
  if (cd->tabnums==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
  tabcount = 0;
  qe->get_table_numbers(cd->tabnums, tabcount);
 // registering:
  cursnum = register_cursor(cdp, cd);
  if (cursnum==NOCURSOR) goto err;
  if (cdp->sel_stmt) cd->hstmt=cdp->sel_stmt->hstmt;  // must save and restore when reading from the cursor etc.
  //if (cd->complete) cd->release_opt(); -- nelze, potrebuji opt kvuli close_data() pri zavrani kurzoru
 // make source copy:
  if (source!=NULL)
  { cd->source_copy=(tptr)corealloc(strlen(source)+1, 85);
    if (cd->source_copy!=NULL) strcpy(cd->source_copy, source);
  }
  cdp->ker_flags &= ~KFL_DISABLE_ROLLBACK;
  return cursnum;
 err:
  unregister_cursor_creation(cdp, cd);
  opt->close_data(cdp);  // temp. tables etc. may have bee created, must destroy them
  delete cd;  *pcd=NULL;
  cdp->ker_flags &= ~KFL_DISABLE_ROLLBACK;
  return NOCURSOR;
}
#endif

dd_index_sep * new_dd_index(unsigned count, unsigned columns)
{ unsigned i;
  dd_index_sep * ccs = new dd_index_sep[count];
  for (i=0;  i<count;  i++)
    ccs[i].atrmap.init(columns);
  return ccs;
}

dd_check * new_dd_check(unsigned count, unsigned columns)
{ unsigned i;
  dd_check * ccs = new dd_check[count];
  for (i=0;  i<count;  i++)
    ccs[i].atrmap.init(columns);
  return ccs;
}

dd_forkey * new_dd_forkey(unsigned count, unsigned columns)
{ unsigned i;
  dd_forkey * ccs = new dd_forkey[count];
  for (i=0;  i<count;  i++)
    ccs[i].atrmap.init(columns);
  return ccs;
}

dd_check::~dd_check(void)
{ delete expr; }

void dd_index::destr_index(void)
{ corefree(reftabnums);
  for (int i=0;  i<partnum;  i++)
    delete parts[i].expr;
  corefree(parts);
  if (root) data_not_closed();
}

bool dd_index_sep::same(const dd_index & oth, bool comp_exprs) const
{
  if (partnum!=oth.partnum)
    return false;
  for (int part=0;  part<partnum;  part++)
    if ( parts[part].type  !=oth.parts[part].type     ||
        !(parts[part].specif==oth.parts[part].specif) ||
         parts[part].desc  !=oth.parts[part].desc     ||
        comp_exprs && !same_expr(parts[part].expr, oth.parts[part].expr) )
      return false;
  return true;
}

BOOL dd_index::add_index_part(void)
{ part_desc * new_parts = (part_desc*)corerealloc(parts, sizeof(part_desc)*(partnum+1));
  if (!new_parts) return FALSE;
  new_parts[partnum].expr=NULL;
  parts=new_parts;
  return TRUE;
}

void dd_index::mark_core(void)
{ if (reftabnums) mark(reftabnums);
  if (parts) mark(parts);
  for (int i=0;  i<partnum;  i++)
    if (parts[i].expr) parts[i].expr->mark_core(); 
  atrmap.mark_core();
}

void dd_check::mark_core(void)
{ if (expr) expr->mark_core(); // expr may be NULL when compilation error occured
  atrmap.mark_core();
}

void dd_forkey::mark_core(void)
{ 
  atrmap.mark_core();
}
void ind_descr::mark_core(void)
{ if (text) mark(text); }

void check_constr::mark_core(void)
{ if (text) mark(text); }

void forkey_constr::mark_core(void)
{ if (text) mark(text); 
  if (par_text) mark(par_text); 
}

BOOL t_optim::get_next_distinct_record(cdp_t cdp)
{ do
  { if (!get_next_record(cdp))
      { main_rowdist->close_index(cdp);  return FALSE; }
  } while (!main_rowdist->add_to_rowdist(cdp));
  main_rowdist->found_distinct_records++;
  return TRUE;
}

void t_optim::reset_distinct(cdp_t cdp)
{ 
#if SQL3
  main_rowdist->reset_index(cdp);
  main_rowdist->found_distinct_records=0;
  reset(cdp);  // t_optim_grouping with DISTINCT needs the main_rowdist index in reset()
#else
  reset(cdp);  
  main_rowdist->reset_index(cdp);
  main_rowdist->found_distinct_records=0;
#endif
}

t_rscope::t_rscope(t_shared_scope  * sh_scopeIn) : sscope(sh_scopeIn) 
  { sh_scope=sh_scopeIn;  next_rscope=NULL; }

t_rscope::~t_rscope(void)
  { if (sh_scope) sh_scope->Release(); }

void t_rscope::mark_core(void)
{ mark(this);  
  if (sh_scope) { sh_scope->mark_core();  mark(sh_scope); }
  if (next_rscope) next_rscope->mark_core(); 
}

void delete_assgns(t_assgn_dynar * assgns)
{ for (int i=0;  i<assgns->count();  i++)
  { t_assgn * assgn = assgns->acc0(i);
    delete assgn->dest;
    if (assgn->source) delete assgn->source;
  }
}

BOOL t_qe_specif::assign_into(cdp_t cdp)
{ for (int i=0;  i<into.count();  i++)
  { t_assgn * assgn = into.acc0(i);
    if (execute_assignment(cdp, assgn->dest, assgn->source, TRUE, TRUE, TRUE)) return TRUE;
  }
  return FALSE;
}

/* Theory
Cislovani tabulek v ordnum, kont_start, rec_extent, pomoci write_kont_list: Slouzi pro urceni pozice tabuky
v entici a pro mereni velikosti entic.

Entice tabulek se pouzivaji v pointerove materializaci a v ruznych distinktorech.

ordnum je poradove cislo tabulky v entici. Pri joinu se operandy radi v entici za sebe (pocet je soucet 
poctu v operandech), pro UN,EX,INT se davaji pres sebe (pocet je maximum), pro union se navic prida 
na zacatek jedna pozice. Koncove tabulky a tab-materializovane tabulky zaberou jednu pozici (pokud je root 
tab-materializovany, pak se ale sestupuje pod nej.)

kont_start v koreni je cislo pozice prvni tabulku podstromu. 
kont_start se pouziva v get_position_0 a set_position_0, kdyz chci nacist aktualni pozici v tabulkach 
urciteho podstromu, ktery je ocislovan v ramci vetsiho stromu. 

rec_extent v koreni je pocet tabulek v podstromu. Pouziva se pri vytvareni distinktoru pro radky 
EXCEPT a INTERSECT, pro zaznamovy distinktor inner joinu a pro right_storage v outer joinu.

Pozor: pokud vznikne pointrova materializace na jine nez top level urovni, vyssi materializace museji obsahovat cele
jeji entice, nejen pozici v nizsi materializaci, protoze nelze garantovat stabilitu nizsich p. materializaci.
*/

/////////////////////////////////////////// displaying the SQL statement /////////////////////////////////////
void display_value(cdp_t cdp, int type, t_specif specif, int length, const char * val, char * buf)
// Writes an abbrevisted version of value converted to string.
// [length] used only for var-len values and for strings (for strings may be -1 -> must use specif and the real length instead)
// [buf] must be at least 45 bytes long.
{ *buf=0;
  if (IS_HEAP_TYPE(type))
  { if (!length)
      strcpy(buf, " NULL");
    else if (type==ATT_TEXT)
    { if (length<=35) { memcpy(buf, val, length);  buf[length]=0; }
      else { memcpy(buf, val, 35);  strcpy(buf+35, "..."); }
    }
    else { int2str(length, buf);  strcat(buf, " bytes"); }
  }
  else  // simple types
  if (!val || IS_NULL(val, type, specif.length)) strcpy(buf, " NULL");
  else
  { switch (type)
    { case ATT_CHAR:     *buf=*val;  buf[1]=0;  break;
      case ATT_BOOLEAN:  strcpy(buf, *val ? "TRUE" : "FALSE");  break;
      case ATT_INT8:     numeric2str(*(sig8 *)val, buf, -specif.scale);  break;
      case ATT_INT16:    numeric2str(*(sig16*)val, buf, -specif.scale);  break;
      case ATT_INT32:    numeric2str(*(sig32*)val, buf, -specif.scale);  break;
      case ATT_INT64:    numeric2str(*(sig64*)val, buf, -specif.scale);  break;
      case ATT_MONEY:    money2str((monstr*)val, buf, SQL_LITERAL_PREZ);  break;
      case ATT_FLOAT:    real2str(*(double*)val, buf, SQL_LITERAL_PREZ);  break;
      case ATT_DATE:     date2str(*(uns32*)val,  buf, SQL_LITERAL_PREZ);   break;
      case ATT_TIME:     time2str(*(uns32*)val,  buf, SQL_LITERAL_PREZ);   break;
        if (specif.with_time_zone) // UTC -> ZULU
          timeUTC2str(*(uns32*)val, cdp->tzd, buf, SQL_LITERAL_PREZ);  
        else
          time2str   (*(uns32*)val,           buf, SQL_LITERAL_PREZ);  
        break;
      case ATT_DATIM:  case ATT_TIMESTAMP:
        if (specif.with_time_zone) // UTC -> ZULU
          timestampUTC2str(*(uns32*)val, cdp->tzd, buf, SQL_LITERAL_PREZ);  
        else
          timestamp2str   (*(uns32*)val,           buf, SQL_LITERAL_PREZ);  
        break;
      case ATT_STRING:
        if (specif.wide_char)
          strcpy(buf, "WIDE-CHAR_STRING");
        else
        { //length=strlen(val);  -- bad, the real length if specified by the [length] parameter, important fo DPs
          if (length==-1)
            { length = (int)strlen(val);  if (length>specif.length) length=specif.length; }
          if (length<=35) 
            { memcpy(buf, val, length);  buf[length]=0; }
          else 
            { memcpy(buf, val, 35);  strcpy(buf+35, "..."); }
        }
        break;
      case ATT_BINARY:
        for (int i=0;  i<specif.length && i<18;  i++)
        { int x = (unsigned char)val[i];
          int bt=x >> 4;
          *(buf++) = bt >= 10 ? 'A'-10+bt : '0'+bt;
          bt=x & 0xf;
          *(buf++) = bt >= 10 ? 'A'-10+bt : '0'+bt;
        }
        if (specif.length > 18) strcpy(buf, "...");
        else *buf=0;
        break;
    }
  }
}

static tptr write_sql_statement(cdp_t cdp, const char * source, const wbstmt * stmt)
// Returns SQL statement with dyn. pars and enm. vars in allocated buffer or NULL on memory allocation error.
{ t_flstr flstr(100,100);
  flstr.put(" [SQL: ");
  if (stmt!=NULL)
  { const char * p = source;  int dpnum=0;  
    while (*p)
    { flstr.putc(*p);  
      if (*p=='?')
      { p++; 
        if (dpnum < stmt->dparams.count() && dpnum < stmt->markers.count())
        { const WBPARAM * wbparam = stmt->dparams.acc0(dpnum);
          flstr.putc('{');
          if (wbparam->len==0) flstr.put("NULL");
          else
          { char buf[45];  
            display_value(cdp, stmt->markers.acc0(dpnum)->type, stmt->markers.acc0(dpnum)->specif, wbparam->len, wbparam->val, buf);
            flstr.put(buf);
          }
          flstr.putc('}');
        }
        dpnum++;
      }
      else if (*p==':')
      { p++; 
        while (*p=='<' || *p=='>') { flstr.putc(*p);  p++; }
        int len=0;  tobjname varname;
       // read the variable name:
        if (sys_Alpha(*p) || *p=='_')
        { do
          { if (len<OBJ_NAME_LEN) varname[len++]=*p;
            flstr.putc(*p);  p++;
          } while (sys_Alpha(*p) || *p>='0' && *p<='9' || *p=='_');
          varname[len]=0;  
          sys_Upcase(varname);
         // search for the variable:
          t_emparam * empar = stmt->find_eparam_by_name(varname);
          if (empar!=NULL)
          { flstr.putc('{');
            char buf[45];  
            display_value(cdp, empar->type, empar->specif, empar->length, empar->val, buf);
            flstr.put(buf);
            flstr.putc('}');
          }
        }
      }
      else p++;
    }
  }
  else flstr.put(source);
  flstr.putc(']');
  return flstr.error() ? NULL : flstr.unbind();
}
void t_exkont_sqlstmt::get_descr(cdp_t cdp, char * buf, int buflen)
{ if (buflen>0) 
  { tptr info = write_sql_statement(cdp, source, stmt);
    if (info) 
       { strmaxcpy(buf, info, buflen);  corefree(info); }
    else strmaxcpy(buf, "[SQL...]", buflen);
   // remove control chars from the statement: Cr and LF would break the structure of the log
    char * p = buf;
    while (*p) { if (*(unsigned char *)p<' ') *p=' ';  p++; }
  }
}

t_global_scope * installed_global_scopes = NULL;
uns64 global_cursor_inst_counter = 0;

void dummy_friend(void) {}  // prevents warning in GCC

