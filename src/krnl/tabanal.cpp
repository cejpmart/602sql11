/****************************************************************************/
// tabanal.cpp - conversions between table source and ta
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
#include "flstr.h"
#include "opstr.h"

// Mozno presunout k curedit a wtabdef, pak compilator musi exportovat vice funkci

#include "tablesql.cpp"

CFNC DllKernel int WINAPI compile_table_to_all(cdp_t cdp, const char * defin, table_all * pta)
// Does not call request_error on compilation errors!
{ compil_info xCI(cdp, defin, DUMMY_OUT, table_def_comp_link);
  xCI.univ_ptr=pta;
  xCI.comp_flags=COMP_FLAG_TABLE; // chnges meaning of [n] after string attribute in index
  set_compiler_keywords(&xCI, NULL, 0, 1);  // SQL keys
  return compile(&xCI);
}

#if WBVERS<=810
//////////////////////////// analyza dotazu pro navrhar ///////////////////////////
#include "sqlanal.h"

typedef char atrname[ATTRNAMELEN+1];
SPECIF_DYNAR(atrname_dynar, atrname, 2);

BOOL get_table_name(CIp_t CI, char * tabname, char * schemaname, ttablenum * ptabobj)
/* Reads schema and table name, if (ptabobj) searches for the table. */
{ if (CI->cursym!=S_IDENT) c_error(TABLE_NAME_EXPECTED);
  strmaxcpy(tabname, CI->curr_name, OBJ_NAME_LEN+1);
  if (next_sym(CI)=='.')
  { strcpy(schemaname, tabname);
    next_and_test(CI, S_IDENT, TABLE_NAME_EXPECTED);
    strmaxcpy(tabname, CI->curr_name, OBJ_NAME_LEN+1);
    next_sym(CI);
  }
  else 
    schemaname[0]=0;
  if (ptabobj==NULL) return FALSE;  // do not search!
  return cd_Find_prefixed_object(CI->cdp, schemaname, tabname, CATEG_TABLE, ptabobj);
}

void get_table_cursor(CIp_t CI, char * tabname, tobjnum * objnum, tcateg * categ)
{ tobjname schemaname;
  if (!get_table_name(CI, tabname, schemaname, objnum))
    *categ=CATEG_TABLE; 
  else // no such table found, look for a viewed table: 
  { if (cd_Find_prefixed_object(CI->cdp, schemaname, tabname, CATEG_CURSOR, objnum))
      { SET_ERR_MSG(CI->cdp, tabname);  c_error(TABLE_DOES_NOT_EXIST); }
    *categ=CATEG_CURSOR;  
  }
}

static void compile_from_clause(CIp_t CI, tsql_anal * anal)
{ anal->tabnum=0;
  int parenth=0;
  while (CI->cursym!=S_FROM || parenth)  /* skips SELECT clause */
  { if (CI->cursym=='(') parenth++;
    else if (CI->cursym==')') parenth--;
    else if (CI->cursym==S_SOURCE_END) c_error(FROM_OMITTED);
    next_sym(CI);
  }
  do // the symbol before the table name in cursym
  { next_sym(CI);
   /* allocate a new table kontext: */
    if (anal->tabnum>=MAX_CURS_TABLES) c_error(TOO_MANY_CURS_TABLES);
   /* read the (possibly qualified) table name and find the table: */
    anal->tables[anal->tabnum].index_name[0]=0;
    get_table_cursor(CI, anal->tables[anal->tabnum].alias, 
                        &anal->tables[anal->tabnum].tbnum,
                        &anal->tables[anal->tabnum].categ);
   // read the correlation name, if present, and replace the name by the alias 
    if (CI->cursym == S_IDENT)  /* table alias */
    { strcpy(anal->tables[anal->tabnum].alias, CI->curr_name);
      next_sym(CI);
    }
    if (CI->cursym == S_INDEX)
    { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      strcpy(anal->tables[anal->tabnum].index_name, CI->curr_name);
      next_sym(CI);
    }
    anal->tabnum++;
   // skip ON clause:
    while (CI->cursym==S_ON)
    { next_and_test(CI, '(', LEFT_PARENT_EXPECTED);
      int parnum = 0;
      do
      { if (CI->cursym=='(') parnum++;
        else if (CI->cursym==')')
          { if (!--parnum) break; }
        else if (!CI->cursym) c_error(RIGHT_PARENT_EXPECTED);
        next_sym(CI);
      } while (TRUE);
      next_sym(CI);  /* skip ')' */
    }
   /* skip the join type, continue by the next table name or terminate: */
    if      (CI->cursym==S_LEFT)
    { if (next_sym(CI)==S_OUTER) next_sym(CI);
      if (CI->cursym!=S_JOIN) c_error(JOIN_EXPECTED);
    }
    else if (CI->cursym==S_RIGHT)
    { if (next_sym(CI)==S_OUTER) next_sym(CI);
      if (CI->cursym!=S_JOIN) c_error(JOIN_EXPECTED);
    }
    else if (CI->cursym==SQ_FULL)
    { if (next_sym(CI)==S_OUTER) next_sym(CI);
      if (CI->cursym!=S_JOIN) c_error(JOIN_EXPECTED);
    }
    else if (CI->cursym==S_OUTER)
      next_and_test(CI, S_JOIN,   JOIN_EXPECTED);
    else if (CI->cursym==',')
      continue;
    else break;
  } while (TRUE);
 /* check the symbol following the FROM clause: */
  if ((CI->cursym!=S_WHERE)  && (CI->cursym!=S_GROUP) &&
      (CI->cursym!=S_HAVING) && (CI->cursym!=S_ORDER) &&
      (CI->cursym!=S_IDENT || strcmp(CI->curr_name, "LIMIT")) &&
      (CI->cursym!=S_UNION)  && (CI->cursym!=';')     &&
      (CI->cursym!=SQ_INTERS)&& (CI->cursym!=SQ_EXCEPT) &&
      (CI->cursym!=SQ_FOR)   && (CI->cursym!=S_SOURCE_END))
        c_error(ERROR_IN_FROM);
}

CFNC DllKernel void WINAPI sql_from(CIp_t CI) 
{ compile_from_clause(CI, (tsql_anal*)CI->univ_ptr); }


CFNC DllKernel void WINAPI free_sql_anal(tsql_anal * anal)
{ if (!anal) return;
  while (anal->select)
  { tsql_elem * an_elem=anal->select->next;
    corefree(anal->select);
    anal->select=an_elem;
  }
  while (anal->order)
  { tsql_order * an_ordr=anal->order->next;
    corefree(anal->order);
    anal->order=an_ordr;
  }
  while (anal->where)
  { tsql_cond * an_cond=anal->where->next;
    corefree(anal->where->part2);
    corefree(anal->where);
    anal->where=an_cond;
  }
  while (anal->links)
  { tsql_link * an_link=anal->links->next;
    corefree(anal->links);
    anal->links=an_link;
  }
  while (anal->groupby)
  { tsql_grp * an_grp = anal->groupby->next;
    corefree(anal->groupby);
    anal->groupby = an_grp;
  }
  corefree(anal->having);
  corefree(anal->limit_count);
  corefree(anal->limit_offset);
  corefree(anal);
}

void trim_tail(tptr tx)
{ int len;
  len=strlen(tx);
  while (len &&
    ((tx[len-1]==' ') || (tx[len-1]=='\r') || (tx[len-1]=='\n'))) len--;
  tx[len]=0;
}

static BOOL new_relation(symbol oper)
{ return oper=='<'     ||oper=='='         ||oper=='>'       ||
         oper==S_NOT_EQ||oper==S_LESS_OR_EQ||oper==S_GR_OR_EQ||
         oper==S_PREFIX||oper==S_SUBSTR    ||oper=='~'       ||
         oper==S_DDOT  ||oper==S_PAGENUM   ||oper==S_IS      ||
         oper==SQ_IN;
}


static void sqlanal_relation(CIp_t CI, tsql_anal * anal, BOOL * in_links)
/* relation is ended by AND, OR, ), ORDER or NULL */
/* relation is divided by relational operators */
{ const char * part1_start, * part1_end, * part2_start, * part2_end;
  int parenth;  d_attr datr;
  char name1[OBJ_NAME_LEN+1], name2[OBJ_NAME_LEN+1], name3[OBJ_NAME_LEN+1];
  short oper;  int len;
  uns8 tab1_atr, obj_level1, tab2_atr, obj_level2;
  tptr tx;  BOOL negated;
  tsql_link * an_link, * xlink;
  tsql_cond * an_cond, * xcond;
/*  if (CI->cursym==SQ_NOT)
    { next_sym(CI);  negated=TRUE; }
  else */ negated=FALSE;
 /* reading part 1 */
  part1_start=CI->prepos;
  if (*in_links)
    if (CI->cursym!=S_IDENT) *in_links=FALSE;
    else
    { strmaxcpy(name1, CI->curr_name, OBJ_NAME_LEN+1);
      name2[0]=name3[0]=0;
      if (next_sym(CI)=='.')
        if (next_sym(CI)==S_IDENT)
        { strmaxcpy(name2, CI->curr_name, OBJ_NAME_LEN+1);
          if (next_sym(CI)=='.')
            if (next_sym(CI)==S_IDENT)
              { strmaxcpy(name3, CI->curr_name, OBJ_NAME_LEN+1);  next_sym(CI); }
        }
      if (!search_dbobj(CI, name1, name2, name3, CI->kntx, &datr, &obj_level1, NULL))
        *in_links=FALSE;
      tab1_atr=datr.name[0];
    }
  parenth=0;
  while (((CI->cursym!=SQ_AND)  && (CI->cursym!=SQ_OR)    &&
          (CI->cursym!=S_GROUP) && (CI->cursym!=S_HAVING) &&
          (CI->cursym!=S_ORDER) && (CI->cursym!=SQ_FOR)  &&
          (CI->cursym!=S_IDENT || strcmp(CI->curr_name, "LIMIT")) &&
          (CI->cursym!=S_UNION) || parenth) && 
         CI->cursym!=S_SOURCE_END)
  { if (CI->cursym==')') if (parenth) parenth--; else break;
    if (!parenth && new_relation(CI->cursym)) break;
    *in_links=FALSE;
    if (CI->cursym=='(') parenth++;
    next_sym(CI);
  }
  part1_end=CI->prepos;

 /* reading part 2 */
  if (new_relation(CI->cursym))
  { oper=CI->cursym;
    next_sym(CI);
         part2_start=CI->prepos;
    if (*in_links)
      if (oper!='=') *in_links=FALSE;
      else if (CI->cursym!=S_IDENT) *in_links=FALSE;
      else
      { strmaxcpy(name1, CI->curr_name, OBJ_NAME_LEN+1);
        name2[0]=name3[0]=0;
        if (next_sym(CI)=='.')
          if (next_sym(CI)==S_IDENT)
          { strmaxcpy(name2, CI->curr_name, OBJ_NAME_LEN+1);
            if (next_sym(CI)=='.')
              if (next_sym(CI)==S_IDENT)
                { strmaxcpy(name3, CI->curr_name, OBJ_NAME_LEN+1);  next_sym(CI); }
          }
        if (!search_dbobj(CI, name1, name2, name3, CI->kntx, &datr, &obj_level2, NULL))
          *in_links=FALSE;
        tab2_atr=datr.name[0];
      }
    parenth=0;
    while (((CI->cursym!=SQ_AND)  && (CI->cursym!=SQ_OR)    &&
            (CI->cursym!=S_GROUP) && (CI->cursym!=S_HAVING) &&
            (CI->cursym!=S_ORDER) && (CI->cursym!=SQ_FOR)  &&
            (CI->cursym!=S_IDENT || strcmp(CI->curr_name, "LIMIT")) &&
            (CI->cursym!=S_UNION) || parenth) && 
           CI->cursym!=S_SOURCE_END)
    { if (CI->cursym==')') if (parenth) parenth--; else break;
      *in_links=FALSE;
      if (CI->cursym=='(') parenth++;
      next_sym(CI);
    }
    part2_end=CI->prepos;
  }
  else { oper=0;  *in_links=FALSE; }

 /* creating the proper record */
  if (*in_links && (CI->cursym!=SQ_OR))
  { an_link=(tsql_link *)corealloc(sizeof(tsql_link), 34);
    if (!an_link) c_error(OUT_OF_MEMORY);
    an_link->atr1=tab1_atr;  an_link->tabord1=obj_level1;
    an_link->atr2=tab2_atr;  an_link->tabord2=obj_level2;
    an_link->outer=wbfalse;
    an_link->next=NULL;
    if (!anal->links) anal->links=an_link;
    else
    { xlink=anal->links;
      while (xlink->next) xlink=xlink->next;
      xlink->next=an_link;
    }
  }
  else
  { len=(int)(part1_end-part1_start);
    if (!len) c_error(SQL_SYNTAX);
    an_cond=(tsql_cond *)corealloc(sizeof(tsql_cond)+len, 34);
    if (!an_cond) c_error(OUT_OF_MEMORY);
    an_cond->next=NULL;
    an_cond->negate=negated;
    an_cond->relation=oper;  an_cond->relation2=0;
    an_cond->part2=an_cond->part3=NULL;
    an_cond->last_in_term=CI->cursym!=SQ_AND;
    memcpy(an_cond->tx, part1_start, len);
    an_cond->tx[len]=0;
    trim_tail(an_cond->tx);
    if (oper)   /* 2nd part of the relation */
    { len=(int)(part2_end-part2_start);
      if (!len) c_error(SQL_SYNTAX);
      tx=(tptr)corealloc(len+1, 34);
      if (!tx) c_error(OUT_OF_MEMORY);
      memcpy(tx, part2_start, len);
      tx[len]=0;
      trim_tail(tx);
      an_cond->part2=tx;
    }
    if (!anal->where) anal->where=an_cond;
    else
    { xcond=anal->where;
      while (xcond->next) xcond=xcond->next;
      xcond->next=an_cond;
    }
  }
}

static void sqlanal_where(CIp_t CI, tsql_anal * anal)
{ BOOL in_par=FALSE, in_links=TRUE;

  while (CI->cursym==S_IDENT)
  { sqlanal_relation(CI, anal, &in_links);
    if ((CI->cursym==SQ_AND) && in_links) next_sym(CI);
    else break;
  }
  if ((CI->cursym==S_ORDER)  || (CI->cursym==S_UNION) ||
      (CI->cursym==S_HAVING) || (CI->cursym==S_GROUP) ||
      CI->cursym==S_IDENT && !strcmp(CI->curr_name, "LIMIT") ||
      (CI->cursym==SQ_FOR)   || (CI->cursym==S_SOURCE_END)) return;

  if (in_links)
  { in_links=FALSE;
    if (CI->cursym==SQ_NOT) { next_sym(CI);  anal->negate=TRUE; }
    if (CI->cursym=='(')  { next_sym(CI);  in_par=TRUE; }
  }

 /* the analysis may be joined into single cycle */
  do
  { do
    { sqlanal_relation(CI, anal, &in_links);
      if (CI->cursym==SQ_AND) next_sym(CI);
      else break;
    } while (TRUE);
    if (CI->cursym==SQ_OR) next_sym(CI);
    else break;
  } while (TRUE);

  if (in_par) test_and_next(CI,')', RIGHT_PARENT_EXPECTED);
}

static tobjnum anal_table_or_oj(CIp_t CI, tsql_anal * anal)
/* Returns the objnum of the leftmost table. */
{ tobjnum tabobj1, tabobj2;  char tabname[OBJ_NAME_LEN+1];  tcateg categ;
  get_table_cursor(CI, tabname, &tabobj1, &categ);
  if (CI->cursym == S_IDENT) /* table alias */ next_sym(CI);
  if (CI->cursym == S_INDEX) { next_sym(CI);  next_sym(CI); }
  if (CI->cursym==S_LEFT || CI->cursym==S_RIGHT || CI->cursym==SQ_FULL)
  { BOOL is_right= CI->cursym==S_RIGHT;
    BOOL is_full = CI->cursym==SQ_FULL;
    if (next_sym(CI) == S_OUTER) next_sym(CI);
    test_and_next(CI, S_JOIN, JOIN_EXPECTED);
    tabobj2=anal_table_or_oj(CI, anal);
    test_and_next(CI, S_ON, ON_EXPECTED);
    test_and_next(CI, '(', LEFT_PARENT_EXPECTED);
    BOOL in_links=TRUE;
    sqlanal_relation(CI, anal, &in_links);
    if (in_links)
    { tsql_link * an_link=anal->links;
      while (an_link->next!=NULL) an_link=an_link->next;
      an_link->outer = is_full ? 3 : wbtrue;
      kont_descr * kntx=CI->kntx;  int i;
      for (i=0;  i<an_link->tabord2;  i++) kntx=kntx->next_kont;
      if ((kntx->kontobj==tabobj2) == is_right)
      { uns8 aux;
        aux=an_link->tabord1;  an_link->tabord1=an_link->tabord2;
        an_link->tabord2=aux;
        aux=an_link->atr1;     an_link->atr1   =an_link->atr2;
        an_link->atr2   =aux;
      }
    }
    test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
  }                                                            
  return tabobj1;
}

CFNC DllKernel void WINAPI sql_analysis(CIp_t CI)
/* CI->univ_ptr must contain initialized tsql_anal *. */
{ int len, parenth;  const char * prepos, * prepos_ag, * asc_desc_pos;
  tsql_anal * anal;
  BOOL is_desc;  short aggr_found;
  tsql_elem  * * curr_elem, * an_elem;
  tsql_order * * curr_ordr, * an_ordr;
  atrname_dynar as_names;  int elemnum;

  anal=(tsql_anal*)CI->univ_ptr;   /* anal is initialized by NULLs! */
  anal->insensit=FALSE;  anal->locked=FALSE;
 /* copying VIEW attribute names: */
  if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "INSENSITIVE"))
  { next_sym(CI);
    test_and_next(CI, SQ_CURSOR, CURSOR_EXPECTED);
    test_and_next(CI, SQ_FOR, FOR_EXPECTED);
    anal->insensit=TRUE;
  }
  else if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "LOCKED"))
  { next_sym(CI);
    test_and_next(CI, SQ_CURSOR, CURSOR_EXPECTED);
    test_and_next(CI, SQ_FOR, FOR_EXPECTED);
    anal->locked=TRUE;
  }
  else if (CI->cursym==S_VIEW)
  { next_sym(CI);  next_sym(CI);   /* skip "VIEW tabname" */
    if (CI->cursym=='.') { next_sym(CI);  next_sym(CI); }
    if (CI->cursym=='(')
    { do
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        strmaxcpy(*as_names.next(), CI->curr_name, ATTRNAMELEN+1);
      } while (next_sym(CI)==',');
      test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
    }
    test_and_next(CI, S_AS, AS_EXPECTED);
  }
  if (CI->cursym!=S_SELECT) c_error(SELECT_EXPECTED);
  if (next_sym(CI)==S_DISTINCT)
    { anal->distinct=TRUE;  next_sym(CI); }
 /* analyse SELECT clause: */
  curr_elem=&anal->select;  elemnum=0;  BOOL move_names = TRUE;
  do
  { prepos=CI->prepos;  /* start of element */
    aggr_found=0;
    switch (CI->cursym)
    { case S_SUM:   aggr_found=1;  break;
      case S_AVG:   aggr_found=2;  break;
      case S_MAX:   aggr_found=3;  break;
      case S_MIN:   aggr_found=4;  break;
      case S_COUNT: aggr_found=5;  break;
      case '*':     move_names=FALSE;  break;
    }
    if (aggr_found) { next_sym(CI);  prepos_ag=CI->prepos; }
    parenth=0;
    do
    { if (CI->cursym==')')
      { if (parenth) parenth--;
        next_sym(CI);
        if (!parenth)   /* aggregate is in an expression! */
          if ((CI->cursym!=',')  && (CI->cursym!=S_FROM) &&
              (CI->cursym!=S_AS)) aggr_found=0;
      }
      else
      { if (CI->cursym==',' || CI->cursym==S_FROM || CI->cursym==S_AS) 
          if (!parenth) break;
        if (CI->cursym=='(') parenth++;
        else if (!CI->cursym) c_error(FROM_EXPECTED);
        next_sym(CI);
      }
    } while (TRUE);
    if (aggr_found) prepos=prepos_ag;
   /* save the new element */
    if (CI->prepos != prepos)
    { len=(unsigned)(CI->prepos - prepos);
      an_elem=(tsql_elem*)corealloc(sizeof(tsql_elem)+len, 45);
      if (!an_elem) c_error(OUT_OF_MEMORY);
      an_elem->aggr=aggr_found;
      memcpy(an_elem->tx, prepos, len);
      an_elem->tx[len]=0;
                  trim_tail(an_elem->tx);
      if (CI->cursym==S_AS)
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        strmaxcpy(an_elem->as_text, CI->curr_name, ATTRNAMELEN+1);
        next_sym(CI);
      }
      else an_elem->as_text[0]=0;
      if (move_names && (elemnum < as_names.count()))
        strcpy(an_elem->as_text, *as_names.acc(elemnum));
      an_elem->next=NULL;
      *curr_elem=an_elem;  curr_elem=&an_elem->next;  elemnum++;
    }
    if (CI->cursym==',') next_sym(CI); else break;
  } while (TRUE);

  if (CI->cursym!=S_FROM) c_error(FROM_EXPECTED);
  do
  { next_sym(CI);
    anal_table_or_oj(CI, anal);
  } while (CI->cursym==',');

 /* cursym may be WHERE */
  if (CI->cursym==S_WHERE)
  { next_sym(CI);
    sqlanal_where(CI, anal);
  }

  if (CI->cursym==S_GROUP)
  { next_sym(CI);
    if (CI->cursym==S_BY) next_sym(CI);
    tsql_grp ** dest = &anal->groupby;
    do // cycle on group elements
    { int inpar=0;
      const char * start = CI->prepos;
      while (CI->cursym!=S_ORDER && CI->cursym!=S_HAVING && 
             CI->cursym!=S_UNION && CI->cursym!=SQ_EXCEPT && CI->cursym!=SQ_INTERS &&
             (CI->cursym!=S_IDENT || strcmp(CI->curr_name, "LIMIT")) &&
             CI->cursym!=SQ_FOR  && CI->cursym!=S_SOURCE_END && CI->cursym!=';' &&
             (CI->cursym!=',' || inpar))
      { if      (CI->cursym=='(') inpar++;
        else if (CI->cursym==')') if (inpar) inpar--;  else break;
        next_sym(CI);
      }
      if (start == CI->prepos) c_error(IDENT_EXPECTED);  // empty GROUP BY clause
     // add grouping column to the analysis:
      int len=CI->prepos-start;
      while (len && (start[len-1]==' ' || start[len-1]=='\r' || start[len-1]=='\n')) len--;
      tsql_grp * an_grp = (tsql_grp*)corealloc(sizeof(tsql_grp)+len, 45);
      if (an_grp==NULL) c_error(OUT_OF_MEMORY);
      memcpy(an_grp->tx, start, len);  an_grp->tx[len]=0;
      an_grp->next=NULL;
      *dest=an_grp;  dest=&an_grp->next;
      if (CI->cursym==',') next_sym(CI);
      else break;
    } while (TRUE);

  }

  if (CI->cursym==S_HAVING)
  { next_sym(CI);
    prepos=CI->prepos;  /* start of element */
    while (CI->cursym!=S_ORDER && CI->cursym!=S_UNION && 
           (CI->cursym!=S_IDENT || strcmp(CI->curr_name, "LIMIT")) &&
           CI->cursym!=SQ_FOR && CI->cursym)
      next_sym(CI);
          if (CI->prepos != prepos)
          { len=(unsigned)(CI->prepos - prepos);
      anal->having=(tptr)corealloc(len+1, 45);
      if (anal->having==NULL) c_error(OUT_OF_MEMORY);
      memcpy(anal->having, prepos, len);
      anal->having[len]=0;
      trim_tail(anal->having);
      if (!*anal->having)  { corefree(anal->having);  anal->having=NULL; }
    }
  }

 /* cursym may be ORDER */
  if (CI->cursym==S_ORDER)
  { next_sym(CI);
    if (CI->cursym==S_BY) next_sym(CI);
    curr_ordr=&anal->order;
    do
    { prepos=CI->prepos;  /* start of element */
      parenth=0;
      is_desc=FALSE;   /* necessary when expression is empty */
      asc_desc_pos=NULL;
      do
      { if ((CI->cursym==',') && !parenth) break;
        if (!CI->cursym || CI->cursym==S_IDENT && !strcmp(CI->curr_name, "LIMIT")) break;
        if (CI->cursym==S_DESC) { is_desc=TRUE;  asc_desc_pos=CI->prepos; }
        else if (CI->cursym==S_ASC) asc_desc_pos=CI->prepos;
        if (CI->cursym=='(') parenth++;
        else if ((CI->cursym==')') && parenth) parenth--;
        next_sym(CI);
      } while (TRUE);
      if (asc_desc_pos==NULL) asc_desc_pos=CI->prepos;
     /* save the new order element from prepos till asc_desc_pos */
      if (asc_desc_pos != prepos)
      { len=(unsigned)(asc_desc_pos - prepos);
        an_ordr=(tsql_order*)corealloc(sizeof(tsql_order)+len, 45);
        if (!an_ordr) c_error(OUT_OF_MEMORY);
        memcpy(an_ordr->tx, prepos, len);
        an_ordr->tx[len]=0;
        trim_tail(an_ordr->tx);
        an_ordr->descent=is_desc;
        an_ordr->next=NULL;
        *curr_ordr=an_ordr;
        curr_ordr=&an_ordr->next;
      }
      if (CI->cursym==',') next_sym(CI); else break;
    } while (TRUE);
  }

 // LIMIT:
  if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "LIMIT"))
  { next_sym(CI);
   // 1st part:
    prepos=CI->prepos;  /* start of element */
    parenth=0;
    do
    { if (CI->cursym==',' && !parenth) break;
      if (!CI->cursym) break;
      if (CI->cursym=='(') parenth++;
      else if ((CI->cursym==')') && parenth) parenth--;
      next_sym(CI);
    } while (TRUE);
   // save it:
    if (CI->prepos != prepos)
    { len=(unsigned)(CI->prepos - prepos);
      anal->limit_offset = (tptr)corealloc(len+1, 45);
      if (!anal->limit_offset) c_error(OUT_OF_MEMORY);
      memcpy(anal->limit_offset, prepos, len);
      anal->limit_offset[len]=0;
      trim_tail(anal->limit_offset);
    }
   // next part:
    if (CI->cursym==',')
    { next_sym(CI);
      prepos=CI->prepos;  /* start of element */
      parenth=0;
      while (CI->cursym) next_sym(CI);
     // save it:
      if (CI->prepos != prepos)
      { len=(unsigned)(CI->prepos - prepos);
        anal->limit_count = (tptr)corealloc(len+1, 45);
        if (!anal->limit_count) c_error(OUT_OF_MEMORY);
        memcpy(anal->limit_count, prepos, len);
        anal->limit_count[len]=0;
        trim_tail(anal->limit_count);
      }
    }
    if (!anal->limit_count)
    { anal->limit_count=anal->limit_offset;
      anal->limit_offset=NULL;
    }
  }

  if (CI->cursym) c_error(SQL_SYNTAX);  /* may be UNION or FOR */
}

//////////////////////////////////////////////////////////////
CFNC DllKernel int WINAPI get_tables_from_query(cdp_t cdp, const char * query, tobjnum * tables, tcateg * categs)
{ tsql_anal anal;
 // analyse the tables in the query:
  compil_info xCI(cdp, query, DUMMY_OUT, sql_from);
  xCI.univ_ptr=&anal;
  set_compiler_keywords(&xCI, NULL, 0, 1); /* SQL keywords selected */
  if (compile(&xCI)) return -1;
 // copy the results:
  for (int i=0;  i<anal.tabnum;  i++)
  { tables[i] = anal.tables[i].tbnum;
    categs[i] = anal.tables[i].categ;
  }
  return anal.tabnum;
}
#endif
