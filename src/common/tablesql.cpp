// tablesql.cpp - common stable definitioon analysis routines for client ans SQL server

BOOL is_pointer(int type)
{ return (type==ATT_PTR) || (type==ATT_BIPTR); }

static int calc_hist_specif(table_all * ta)
// For domains must be calculated again after the translation.
{ atr_all * att;  unsigned info = 0;
  int pos = ta->attrs.count()-2;
  if (pos<=0) return 0;
  do
  { att=ta->attrs.acc(pos);
    if (att->type==ATT_AUTOR) info=4*info+0x10;
    else if (att->type==ATT_DATIM) info=4*info+0x20;
    else break;
  } while (--pos>0);
  return info + (att->type & 0xf) + (typesize((const d_attr*)att)<<8);
}

sig32 check_integer(CIp_t CI, sig32 lbound, sig32 ubound)
{ if (CI->curr_scale) c_error(MUST_BE_INTEGER);
  if (CI->curr_int<lbound || CI->curr_int>ubound) c_error(INT_OUT_OF_BOUND);
  return (sig32)CI->curr_int;
}

#ifdef STOP  // support discontinued
static int findkeytab(CIp_t CI, tptr table)
{ int i=0;
  while (*table)
  { if (!my_stricmp(table,CI->curr_name)) return i;
    table += strlen(table)+1;
    i++;
  }
  c_error(BAD_IDENT);
  return 0;
}

static void name_convert(CIp_t CI)
{ int len = strlen(CI->curr_name);
  if (len >= ATTRNAMELEN) len = ATTRNAMELEN-1;
  CI->curr_name[len]='_';  CI->curr_name[len+1]=0;
}

char tnames[]="BOOLEAN\0CHAR\0SHORT\0INTEGER\0MONEY\0REAL\0STRING\0CSSTRING\0CSISTRING\0BINARY\0DATE\0TIME\0TIMESTAMP\0POINTER\0BIPTR\0AUTORIZACE\0DATUMOVKA\0HISTORIE\0RASTER\0TEXT\0NOSPEC\0SIGNATURE\0";

static void old_table_def_comp(CIp_t CI, table_all * ta)
{/* journal flag: */
  if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "NO_JOURNAL"))
    { next_sym(CI);  ta->tabdef_flags=TFL_NO_JOUR; }
  else ta->tabdef_flags=0;
 /* attributes: */
  while ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "ATTRIBUTE"))
  { atr_all * att;  int i;
    next_sym(CI);
    if (ta->attrs.count() >= 255) c_error(TOO_MANY_TABLE_ATTRS);
    att=ta->attrs.next();
    if (!att) c_error(OUT_OF_MEMORY);
    att->nullable=wbtrue;  att->multi=1;  /* non-null inplicit values */
   /* attribute name: */
    if (CI->cursym!=S_IDENT)
      if (CI->cursym >= 128)   /* keyword */ name_convert(CI);
      else c_error(IDENT_EXPECTED);
    else if (check_atr_name(CI->curr_name)) name_convert(CI);
    for (i=0;  i<ta->attrs.count();  i++)
      if (!strcmp(CI->curr_name, ta->attrs.acc(i)->name))
        c_error(NAME_DUPLICITY);
    strmaxcpy(att->name, CI->curr_name, ATTRNAMELEN+1);   /* name is OK */
    next_sym(CI);
   /* attribute type: */
    att->specif.set_null();
    switch (CI->cursym)
    { case SQ_CHAR:     att->type=ATT_CHAR;   break;
      case SQ_INTEGER:  att->type=ATT_INT32;  break;
      case SQ_REAL:     att->type=ATT_FLOAT;  break;
      case SQ_DATE:     att->type=ATT_DATE;   break;
      case SQ_TIME:     att->type=ATT_TIME;   break;
      case S_POINTER:   att->type=ATT_PTR;    break;
      case S_BIPTR:     att->type=ATT_BIPTR;  break;
      case S_AUTHOR:
      case S_AUTOR:     att->type=ATT_AUTOR;  break;
      case S_DATIM:     att->type=ATT_DATIM;  break;
      case S_HISTORY:   att->type=ATT_HIST;
        att->specif.opqval=calc_hist_specif(ta);
        if (!att->specif.opqval) c_error(SQL_SYNTAX);
        break;
      case S_IDENT:     att->type=(uns8)(findkeytab(CI, tnames) + 1);  break;
      default: c_error(UNKNOWN_TYPE_NAME); /*other error*/
    }
    next_sym(CI);
    if (is_pointer(att->type)) att->specif.destination_table=SELFPTR;
    if (is_string (att->type)) att->specif.length=12;
    if (CI->cursym=='[')
    { if (is_pointer(att->type))
      { if (next_sym(CI)==S_IDENT)
        { tobjnum tabnum;
          tptr name = (tptr)ta->names.acc(ta->attrs.count()-1);
          if (!name) c_error(OUT_OF_MEMORY);
          strmaxcpy(name+sizeof(tobjname), CI->curr_name, OBJ_NAME_LEN+1);
#ifdef SRVR
          if (find_obj(CI->cdp, name+sizeof(tobjname), CATEG_TABLE, &tabnum))
#else
          if (cd_Find_object(CI->cdp, name+sizeof(tobjname), CATEG_TABLE, &tabnum))
#endif
            att->specif.destination_table=NOT_LINKED;  /* to be linked */
          else att->specif.destination_table=tabnum;
          next_sym(CI);
        }
      }
      else if (is_string(att->type))
      { next_and_test(CI,S_INTEGER, INTEGER_EXPECTED);
        att->specif.length=check_integer(CI, 1, 255);
        next_sym(CI);
      }
      else c_error(BRACE_NOT_ALLOWED);
      test_and_next(CI,']', RIGHT_BRACE_EXPECTED);
    }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "PREALLOC"))
      { next_sym(CI);  att->multi=(uns8)get_num_val(CI, 0, 127); }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "EXPANDABLE"))
      { next_sym(CI);  att->multi |= 0x80; }
  }
  if (ta->attrs.count()==1) c_error(NO_ATTRS_DEFINED);

 /* indicies: */
  while (CI->cursym==S_INDEX)
  { ind_descr * indx;  const char * ind_start, * ind_stop;  unsigned isize;
    next_sym(CI);
    if (ta->indxs.count() >= INDX_MAX) c_error(TOO_MANY_INDXS);
    indx=ta->indxs.next();
    if (!indx) c_error(OUT_OF_MEMORY);
    indx->state=INDEX_NEW;
    if (CI->cursym=='!')
    { indx->index_type=INDEX_UNIQUE;  next_sym(CI); }
    else indx->index_type=INDEX_NONUNIQUE;
    indx->has_nulls = indx->index_type == INDEX_NONUNIQUE;  // default
    ind_start=CI->prepos;
    do next_sym(CI);
    while (CI->cursym && (CI->cursym!=S_INDEX) &&
           ((CI->cursym != S_IDENT) || strcmp(CI->curr_name, "END")));
    ind_stop=CI->prepos;
    while ((ind_stop > ind_start) &&  /* remove trailing CR, LF, spaces */
       ((ind_stop[-1]==' ') || (ind_stop[-1]=='\n') || (ind_stop[-1]=='\r')))
      ind_stop--;
   /* copy the index source: */
    isize=(unsigned)(ind_stop-ind_start);
    indx->text=(tptr)corealloc(isize+1, 97);
    if (!indx->text) c_error(OUT_OF_MEMORY);
    memcpy(indx->text, ind_start, isize);  indx->text[isize]=0;
  }
 /* end check */
  if ((CI->cursym == S_IDENT) && !strcmp(CI->curr_name, "END")) next_sym(CI);
  if (CI->cursym != S_SOURCE_END) c_error(GARBAGE_AFTER_END);
}
#endif

static BOOL check_constrain_name_duplicity(table_all * ta, const char * co_name)
// Allows multiple constrains with the empty name
// Must not check the constrains marked for deletion
{ if (!*co_name) return FALSE;
  int i;  BOOL dupl=FALSE;
  for (i=0;  i<ta->indxs.count();  i++) 
    if (!strcmp(ta->indxs .acc0(i)->constr_name, co_name) && ta->indxs .acc0(i)->state!=INDEX_DEL) 
      dupl=TRUE; 
  for (i=0;  i<ta->checks.count();  i++) 
    if (!strcmp(ta->checks.acc0(i)->constr_name, co_name) && ta->checks.acc0(i)->state!=ATR_DEL)  
      dupl=TRUE; 
  for (i=0;  i<ta->refers.count();  i++) 
    if (!strcmp(ta->refers.acc0(i)->constr_name, co_name) && ta->refers.acc0(i)->state!=ATR_DEL)
      dupl=TRUE; 
  return dupl;
}

static int analyse_constraint_characteristics(CIp_t CI)
// ATTN: LL(1) conflict with NOT NULL in domain definition
{ int co_cha;
  if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "INITIALLY"))
  { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    if      (!strcmp(CI->curr_name, "DEFERRED") ) co_cha=COCHA_DEF;
    else if (!strcmp(CI->curr_name, "IMMEDIATE")) co_cha=COCHA_IMM_NONDEF; // default when initially immediate
    else    c_error(GENERAL_SQL_SYNTAX);
    if (next_sym(CI)==SQ_NOT)
    { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      if (strcmp(CI->curr_name, "DEFERRABLE")) c_error(GENERAL_SQL_SYNTAX);
      if (co_cha==COCHA_DEF) c_error(SPECIF_CONFLICT);
      next_sym(CI);
    }
    else if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "DEFERRABLE"))
    { if (co_cha==COCHA_IMM_NONDEF) co_cha=COCHA_IMM_DEFERRABLE;  // unless initially deferred
      next_sym(CI);
    }
  }
  else
  { if (CI->cursym==SQ_NOT)
    { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      if (strcmp(CI->curr_name, "DEFERRABLE")) c_error(GENERAL_SQL_SYNTAX);
      co_cha=COCHA_IMM_NONDEF;
      next_sym(CI);
    }
    else if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "DEFERRABLE"))
    { co_cha=COCHA_IMM_DEFERRABLE;  // initially immediate is implicit
      next_sym(CI);
    }
    else return COCHA_UNSPECIF;  // constraint characteristics not specified
    if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "INITIALLY"))
    { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      if (!strcmp(CI->curr_name, "DEFERRED"))
      { if (co_cha==COCHA_IMM_NONDEF) c_error(SPECIF_CONFLICT);
        co_cha=COCHA_DEF;
      }
      else if (strcmp(CI->curr_name, "IMMEDIATE"))
        c_error(GENERAL_SQL_SYNTAX);
      next_sym(CI);
    }
  }
  return co_cha;
}

tptr read_check_clause(CIp_t CI, uns8 & co_cha)
// Called on CHECK, returns after ')'.
{ const char * start, * stop;  
  next_and_test(CI, '(', LEFT_PARENT_EXPECTED);
  next_sym(CI);
  start=CI->prepos;
  int inside=0;
  do
  { if (CI->cursym == ')')
      if (inside) inside--; else break;
    else if (CI->cursym == '(') inside++;
    else if (!CI->cursym) break; // c_error(RIGHT_PARENT_EXPECTED); -- if error is not reported, it can be corrected
    next_sym(CI);
  } while (TRUE);
  stop=CI->prepos;
  size_t size=stop-start;
  tptr tx=(tptr)corealloc(size+1, 92);
  if (!tx) c_error(OUT_OF_MEMORY);
  memcpy(tx, start, size);  tx[size]=0;
  next_sym(CI);
  co_cha=analyse_constraint_characteristics(CI);
  return tx;
}

void check_clause(CIp_t CI, table_all * ta, const char * co_name, int column_context)
// when [column_context]!=-1 then "VALUE" references column [column_context].
{ if (check_constrain_name_duplicity(ta, co_name)) c_error(DUPLICATE_NAME);
  check_constr * check=ta->checks.next();
  if (!check) c_error(OUT_OF_MEMORY);
  strcpy(check->constr_name, co_name);
  check->state=ATR_NEW;
  check->column_context=column_context;
  check->text=read_check_clause(CI, check->co_cha);
}

static void make_forkey(CIp_t CI, table_all * ta, const char * co_name,
                               const char * loc_text, unsigned loc_size)
/* cursym==S_REFERENCES on entry, cursym is after the last symbol on the end */
{ forkey_constr * ref;  const char * start;  tptr tx;  unsigned size, inside;
  if (check_constrain_name_duplicity(ta, co_name)) c_error(DUPLICATE_NAME);
 /* allocate a new reference constrains */
  ref=ta->refers.next();
  if (!ref) c_error(OUT_OF_MEMORY);
  ref->update_rule=ref->delete_rule=REFRULE_NO_ACTION;
  strcpy(ref->constr_name, co_name);
  ref->state=ATR_NEW;
 /* local text: */
  tx=(tptr)corealloc(loc_size+1, 92);
  if (!tx) c_error(OUT_OF_MEMORY);
  memcpy(tx, loc_text, loc_size);  tx[loc_size]=0;
  ref->text=tx;
 /* parent table name: */
  next_and_test(CI, S_IDENT, IDENT_EXPECTED);
  strmaxcpy(ref->desttab_name, CI->curr_name, OBJ_NAME_LEN+1);
  if (next_sym(CI)=='.')
  { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    strcpy(ref->desttab_schema, ref->desttab_name);
#ifdef SRVR
    reduce_schema_name(CI->cdp, ref->desttab_schema);
#endif
    strmaxcpy(ref->desttab_name, CI->curr_name, OBJ_NAME_LEN+1);
    next_sym(CI);
  }
  else *ref->desttab_schema=0;
 /* parent attributes: */
  if (CI->cursym=='(')
  { next_sym(CI);
    start=CI->prepos;
    inside=0;
    do
    { if (CI->cursym == ')')
        if (inside) inside--; else break;
      else if (CI->cursym == '(') inside++;
      else if (!CI->cursym) c_error(RIGHT_PARENT_EXPECTED);
      next_sym(CI);
    } while (TRUE);
    size=(unsigned)(CI->prepos-start);
    tx=(tptr)corealloc(size+1, 92);
    if (!tx) c_error(OUT_OF_MEMORY);
    memcpy(tx, start, size);  tx[size]=0;
    next_sym(CI);   /* skip ')' */
  }
  else   /* parent attributes same as local */
#ifdef STOP  // used up to WB 6.0b
  { tx=(tptr)corealloc(loc_size+1, 92);
    if (!tx) c_error(OUT_OF_MEMORY);
    memcpy(tx, loc_text, loc_size);  tx[loc_size]=0;
  }
#endif
    tx=NULL;  // not specified: primery key of the parent table should be used
  ref->par_text=tx;
 // rules & actions:
  while (CI->cursym==S_ON)
  { BOOL upd;  uns8 rul;
    if (next_sym(CI)== S_UPDATE) upd=TRUE;
    else if (CI->cursym == S_DELETE) upd=FALSE;
    else c_error(GENERAL_SQL_SYNTAX);
    if (next_sym(CI)==S_SET)
    { if (next_sym(CI)== S_NULL) rul=REFRULE_SET_NULL;
      else if (CI->cursym == S_DEFAULT) rul=REFRULE_SET_DEFAULT;
      else c_error(GENERAL_SQL_SYNTAX);
    }
    else if (CI->cursym==S_CASCADE) rul=REFRULE_CASCADE;
    else
    { rul=REFRULE_NO_ACTION;
      if (CI->cursym  !=S_IDENT || strcmp(CI->curr_name, "NO") ||
          next_sym(CI)!=S_IDENT || strcmp(CI->curr_name, "ACTION"))
        c_error(GENERAL_SQL_SYNTAX);
    }
    if (upd) ref->update_rule=rul;  else ref->delete_rule=rul;
    next_sym(CI);
  }
  ref->co_cha=analyse_constraint_characteristics(CI);
}

static void foreign_key_clause(CIp_t CI, table_all * ta, const char * co_name)
{ next_and_test(CI, S_KEY, KEY_EXPECTED);
  next_and_test(CI, '(', LEFT_PARENT_EXPECTED);
  next_sym(CI);
  const char * start=CI->prepos;
  while (next_sym(CI) != ')') if (CI->cursym==S_SOURCE_END)
    c_error(RIGHT_PARENT_EXPECTED);
  unsigned loc_size = (unsigned)(CI->prepos-start);
  next_and_test(CI, S_REFERENCES, REFERENCES_EXPECTED);
  make_forkey(CI, ta, co_name, start, loc_size);
}

static ind_descr * make_attr_index(CIp_t CI, table_all * ta, atr_all * att, symbol ind_sym, int inspos)
// Creates a new index consisting of a single column.
{ ind_descr * indx;
  indx = inspos<0 ? ta->indxs.next() : (ind_descr*)ta->indxs.insert(inspos);
  if (!indx) c_error(OUT_OF_MEMORY);
  indx->state=INDEX_NEW;
  indx->index_type=(ind_sym==S_PRIMARY) ? INDEX_PRIMARY :
                   (ind_sym==S_UNIQUE) ? INDEX_UNIQUE : INDEX_NONUNIQUE;
  indx->has_nulls = indx->index_type == INDEX_NONUNIQUE;  // default
  indx->text=(tptr)corealloc(strlen(att->name)+1, 92);
  if (!indx->text) c_error(OUT_OF_MEMORY);
  strcpy(indx->text, att->name);
  /* const_name, root, code_size, code, text, reftables,
     reftabnums are NULL, disabled is FALSE, attrmap was 0 init. */
  return indx;
}

static void add_table_index(CIp_t CI, table_all * ta, const char * co_name, int index_type, bool conditional = false)
{ bool duplicate = false;  ind_descr * indx;
  if (check_constrain_name_duplicity(ta, co_name)) 
    if (conditional) duplicate=true;
    else c_error(DUPLICATE_NAME);
  if (!duplicate)
  { indx = ta->indxs.next();
    if (!indx) c_error(OUT_OF_MEMORY);
    strcpy(indx->constr_name, co_name);
    indx->state=INDEX_NEW;
    indx->index_type= index_type;
    indx->has_nulls = indx->index_type == INDEX_NONUNIQUE;  // default
  }
 // make the copy of the index definition:
  test_and_next(CI, '(', LEFT_PARENT_EXPECTED);
  const char * start=CI->prepos;
  int parnum=0;
  while ((CI->cursym != ')') || parnum)
  { if (CI->cursym==S_SOURCE_END) c_error(RIGHT_PARENT_EXPECTED);
    if (CI->cursym=='(') parnum++;
    else if (CI->cursym==')') parnum--;
    next_sym(CI);
  }
  const char * stop=CI->prepos;
  next_sym(CI);
  if (!duplicate)
  { size_t size = stop-start;
    tptr tx=(tptr)corealloc(size+1, 92);
    if (!tx) c_error(OUT_OF_MEMORY);
    memcpy(tx, start, size);  tx[size]=0;
    indx->text=tx;
#ifdef SRVR  // must check the syntax as soon as possible
    int res;
    { dd_index_sep cc;
      res=compile_index(CI->cdp, indx->text, ta, indx->constr_name, indx->index_type, indx->has_nulls, &cc);
    }
    if (res) c_error(res);
#endif    
  }
 // NULL or NOT NULL:
  if (CI->cursym==S_NULL)
    { if (!duplicate) indx->has_nulls = true;  next_sym(CI); }
  else if (CI->cursym==SQ_NOT)
    { next_and_test(CI, S_NULL, NULL_EXPECTED);  if (!duplicate) indx->has_nulls = false;  next_sym(CI); }
 // read referencing tables (not used from 8.0, for compatibility only): 
  if (index_type==INDEX_PRIMARY || index_type==INDEX_UNIQUE)
    if (CI->cursym==S_BY)
    { next_sym(CI);
      while (CI->cursym==S_IDENT)
      { if (next_sym(CI)=='.')
        { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
          next_sym(CI);
        }
      }
    }
  if (!duplicate)
    indx->co_cha=analyse_constraint_characteristics(CI);
  else
    analyse_constraint_characteristics(CI);
}

static void write_att(CIp_t CI, atr_all * att, ctptr name, int type, t_specif specif, BOOL preserve_value)
{ strcpy(att->name, name);  att->type=(uns8)type;  att->specif=specif;
  att->multi=1;  att->defval=NULL;  att->nullable=wbtrue;
  if (preserve_value) att->alt_value=NULL; // the original value of the column will be preserved
  else // column will be assigned default value
  { att->alt_value=(tptr)corealloc(1, 92);
    if (!att->alt_value) c_error(OUT_OF_MEMORY);
    att->alt_value[0]=0;
  }
}

sig32 get_length(CIp_t CI)
{ if (CI->cursym!='(') c_error(LEFT_PARENT_EXPECTED);
  next_and_test(CI, S_INTEGER, INTEGER_EXPECTED);
  sig32 length=check_integer(CI, 1, 0x7fffffff);
  next_and_test(CI, ')', RIGHT_PARENT_EXPECTED);
  next_sym(CI);
  return length;
}

static void analyse_collation(CIp_t CI, int tp, t_specif & specif)
// Called twice for each column, must not set default values on the beginning. specif is initialised outside.
{ if (CI->cursym==S_COLLATE)
  { if (!is_string(tp) && tp!=ATT_TEXT && tp!=ATT_EXTCLOB) c_error(INCOMPATIBLE_TYPES);
    next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    if (!strcmp(CI->curr_name, "CSISTRING")) 
      { specif.charset=1;  specif.ignore_case=TRUE;  }
    else if (!strcmp(CI->curr_name, "IGNORE_CASE")) 
      specif.ignore_case=TRUE;
    else
    { int chrs = find_collation_name(CI->curr_name);
      if (chrs<0) { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(IDENT_NOT_DEFINED); }
      specif.charset=chrs;
    }
    next_sym(CI);
  }
  if (CI->cursym==S_IDENT && !strcmp(CI->curr_name,"IGNORE_CASE")) // IGNORE_CASE may be without COLLATE for ANSI collation
    { specif.ignore_case=TRUE;  next_sym(CI); }
}

CFNC DllKernel void WINAPI analyse_type(CIp_t CI, int & tp, t_specif & specif, table_all * ta, int curr_attr_num)
{ sig32 length;  
  specif.set_null();
  switch (CI->cursym)  /* analyse type: */
  { case S_LONG:     // ODBC only
      next_sym(CI);
      if (CI->cursym==S_VARCHAR)
        { tp=ATT_TEXT;    specif.length=SPECIF_VARLEN;  next_sym(CI);  break; }
      if (CI->cursym==S_VARBINARY)
        { tp=ATT_NOSPEC;  specif.length=SPECIF_VARLEN;  next_sym(CI);  break; }
      c_error(UNKNOWN_TYPE_NAME);

    case S_NATIONAL: /* SQL 2 extension */
      next_sym(CI);
      if ((CI->cursym!=SQ_CHAR) && (CI->cursym!=S_CHARACTER)) c_error(UNKNOWN_TYPE_NAME);
      // cont. 
    case S_NCHAR:
      specif.wide_char=TRUE;  // cont.
    case S_CHARACTER:  case SQ_CHAR:  
      if (next_sym(CI)==S_VARYING)  // if VARYING specified then (length) must be specified
      { next_sym(CI);  length=get_length(CI); 
        tp = length<=(specif.wide_char ? MAX_FIXED_STRING_LENGTH/2 : MAX_FIXED_STRING_LENGTH) ? ATT_STRING : ATT_TEXT;
      }
      else if (CI->cursym==SQ_LARGE)  // SQL 3
      { next_and_test(CI, SQ_OBJECT, OBJECT_EXPECTED);
        if (next_sym(CI)=='(') length=get_length(CI);  else length=SPECIF_VARLEN;
        tp = ATT_TEXT;  
      }
      else 
      { if (CI->cursym=='(') 
        { length=get_length(CI);  
          tp = length<=(specif.wide_char ? MAX_FIXED_STRING_LENGTH/2 : MAX_FIXED_STRING_LENGTH) ? ATT_STRING : ATT_TEXT;
        }
        else 
        { length=1;
          tp = ATT_CHAR;
        }
      }
      specif.length = specif.wide_char ? 2*length : length;
      break;
    case SQ_NCLOB:      // SQL 3
      specif.wide_char=TRUE;  // cont.
    case SQ_CLOB:       // SQL 3
      if (next_sym(CI)=='(') length=get_length(CI);  else length=SPECIF_VARLEN;
      tp=ATT_TEXT;  specif.length = length;
      break;
    case S_VARCHAR:
      next_sym(CI);  length=get_length(CI); 
      tp = length<=MAX_FIXED_STRING_LENGTH ? ATT_STRING : ATT_TEXT;
      specif.length = length;
      break;

    case SQ_BLOB:      // SQL 3
      if (next_sym(CI)=='(') length=get_length(CI);  else length=SPECIF_VARLEN;
      tp=ATT_NOSPEC;  specif.length=length;  
      break;
    case SQ_BINARY:    // ODBC only, SQL 3 with LARGE OBJECT
      if (next_sym(CI)==SQ_LARGE)  // SQL 3
      { next_and_test(CI, SQ_OBJECT, OBJECT_EXPECTED);
        if (next_sym(CI)=='(') length=get_length(CI);  else length=SPECIF_VARLEN;
        tp=ATT_NOSPEC;  
      }
      else
      { length = get_length(CI);
        tp = length>MAX_FIXED_STRING_LENGTH ? ATT_RASTER : ATT_BINARY;
      }
      specif.length=length;  
      break;
    case S_VARBINARY:  // ODBC only, WB extension: no () possible
      if (next_sym(CI)=='(') length=get_length(CI);  else length=SPECIF_VARLEN;
      tp=ATT_RASTER;  specif.length=length;  
      break;
    case SQ_BOOLEAN:   // SQL 3
      tp=ATT_BOOLEAN;  specif.set_null();  
      next_sym(CI);
      break;
    case S_BIT:        // ODBC + SQL II
      tp=ATT_BOOLEAN;  specif.set_null();  
      if (next_sym(CI)==S_VARYING)  // SQL II over ODBC
        next_and_test(CI, '(',  LEFT_PARENT_EXPECTED);
      if (CI->cursym=='(')
      { next_and_test(CI, S_INTEGER, INTEGER_EXPECTED);
        length=check_integer(CI, 1, 0x7fffffff);
        next_and_test(CI, ')', RIGHT_PARENT_EXPECTED);
        next_sym(CI);
        if (length>1)
        { length=(length-1) / 8 + 1;
          tp = length<=MAX_FIXED_STRING_LENGTH*8 ? ATT_BINARY : ATT_RASTER;  
          specif.length=length;
        }
      }
      break;
    case S_DECIMAL:  case S_NUMERIC:  case S_DEC: /* DEC in SQL 2 */
    { int precis, scale;
      if (next_sym(CI)=='(')  // parameters are optional in SQL 3
      { next_and_test(CI, S_INTEGER, INTEGER_EXPECTED);
        precis=check_integer(CI, 1, 255);
        if (next_sym(CI)==',')  // may not be used in SQL II
        { next_and_test(CI, S_INTEGER, INTEGER_EXPECTED);
          scale=check_integer(CI, 0, precis);
          next_sym(CI);
        }
        else scale=0;
        test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
        tp = precis<=2 ? ATT_INT8 : precis<=4 ? ATT_INT16 : precis<=9 ? ATT_INT32 : ATT_INT64;
        if (scale==2 && precis <= 14 && precis >= 10) tp=ATT_MONEY; // specific for compatibilty, new table designers do not create such type
      }
      else  // default 
      { tp=ATT_INT32;  scale=0;  precis=10; }
      specif.set_num(scale, precis);
      break;
    }
    case SQ_TINYINT:
      tp=ATT_INT8;   specif.set_num(0, i8_precision );  next_sym(CI);  break;
    case S_SMALLINT:
      tp=ATT_INT16;  specif.set_num(0, i16_precision);  next_sym(CI);  break;
    case SQ_INTEGER:
    case S_INT:         /* SQL 2 extension */
      tp=ATT_INT32;  specif.set_num(0, i32_precision);  next_sym(CI);  break;
    case S_BIGINT:      /* BIGINT in ODBC only, changed in v. 7.0 */
      tp=ATT_INT64;  specif.set_num(0, i64_precision);  next_sym(CI);  break;
    case S_FLOAT:
      next_sym(CI);  specif.opqval=0;
      if (CI->cursym == '(')   /* SQL 2, ODBC 2.0 */
      { next_and_test(CI, S_INTEGER, INTEGER_EXPECTED);
        check_integer(CI, 0, 255);
        specif.set_num(0, (int)CI->curr_int);
        next_and_test(CI, ')', RIGHT_PARENT_EXPECTED);
        next_sym(CI);
      }
      tp=ATT_FLOAT;  break;
    case S_DOUBLE:
      if (next_sym(CI)!=S_PRECISION) c_error(PRECISION_EXPECTED);
      /* cont. */
    case SQ_REAL:
      next_sym(CI);  tp=ATT_FLOAT;      specif.opqval=0;  break;
    case SQ_TIMESTAMP:
      next_sym(CI);  tp=ATT_TIMESTAMP;  specif.opqval=0;  goto check_tz;
    case SQ_DATE:
      next_sym(CI);  tp=ATT_DATE;       specif.opqval=0;  break;
    case SQ_TIME:
      next_sym(CI);  tp=ATT_TIME;       specif.opqval=0;  
     check_tz:
      if (CI->cursym==SQ_WITH)
      { next_and_test(CI, SQ_TIME, GENERAL_SQL_SYNTAX);
        next_sym(CI);
        if (!CI->is_ident("ZONE")) c_error(GENERAL_SQL_SYNTAX);
        next_sym(CI);
        specif.with_time_zone=1;
      }
      break;
   /* 602SQL types: */
    case S_AUTOR:  case S_AUTHOR:
      next_sym(CI);  tp=ATT_AUTOR;      specif.opqval=0;  break;
    case S_DATIM:
      next_sym(CI);  tp=ATT_DATIM;      specif.opqval=0;  break;
    case S_SIGNATURE:
      next_sym(CI);  tp=ATT_SIGNAT;     specif.opqval=SPECIF_VARLEN;  break;
    case S_HISTORY:
      next_sym(CI);  tp=ATT_HIST;
      if (ta!=NULL)
      { specif.opqval=calc_hist_specif(ta);
        if (!specif.opqval) c_error(SQL_SYNTAX);
      }
      break;
    case S_POINTER:  tp=ATT_PTR;   goto ptrs;
    case S_BIPTR:    tp=ATT_BIPTR;
     ptrs:
      { char * nm;  tobjnum tabnum;
        next_and_test(CI, '[', LEFT_BRACE_EXPECTED);
        if (next_sym(CI)==S_IDENT && ta!=NULL)
        { nm=(char*)ta->names.acc(curr_attr_num);   /* same index as the attribute */
          /* This may not be the last attribute (when altering) */
          if (!nm) c_error(OUT_OF_MEMORY);
          strmaxcpy(nm+sizeof(tobjname), CI->curr_name, sizeof(tobjname));
          next_sym(CI);
          if (CI->cursym=='.')
          { next_and_test(CI, S_IDENT,  IDENT_EXPECTED);
            strcpy(nm, nm+sizeof(tobjname));
            strmaxcpy(nm+sizeof(tobjname), CI->curr_name, sizeof(tobjname));
            next_sym(CI);
          }
          else *nm=0;
#ifdef SRVR
          if (*nm)
            reduce_schema_name(CI->cdp, nm);
          if (name_find2_obj(CI->cdp, nm+sizeof(tobjname), nm, CATEG_TABLE, &tabnum))
#else
          if (cd_Find_prefixed_object(CI->cdp, nm, nm+sizeof(tobjname), CATEG_TABLE, &tabnum))
#endif
            specif.destination_table=NOT_LINKED;  /* to be linked */
          else specif.destination_table=tabnum;
        }
        else specif.destination_table=SELFPTR;  /* destination table name not specified */
        test_and_next(CI, ']', RIGHT_BRACE_EXPECTED);
        break;
      }
    case S_IDENT:   // non-keyword type names or a domain name
    { if (!strcmp(CI->curr_name, "HANDLE"))
        { tp=ATT_HANDLE;  specif.set_num(0, handle_precision);  next_sym(CI);  break; }
      if (!strcmp(CI->curr_name, "EXTCLOB"))
        { tp=ATT_EXTCLOB;  specif.length = SPECIF_VARLEN;  next_sym(CI);  break; }
      if (!strcmp(CI->curr_name, "EXTNCLOB"))
        { tp=ATT_EXTCLOB;  specif.length = SPECIF_VARLEN;  specif.wide_char=true;  next_sym(CI);  break; }
      if (!strcmp(CI->curr_name, "EXTBLOB"))
        { tp=ATT_EXTBLOB;  specif.length = SPECIF_VARLEN;  next_sym(CI);  break; }

      tobjname domname, schemaname;
      strmaxcpy(domname, CI->curr_name, sizeof(tobjname));
      if (next_sym(CI)=='.')
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        strcpy(schemaname, domname);
        strmaxcpy(domname, CI->curr_name, sizeof(tobjname));
        next_sym(CI);
      }
      else *schemaname=0;
#ifdef SRVR
      if (*schemaname)
        reduce_schema_name(CI->cdp, schemaname);
      if (name_find2_obj(CI->cdp, domname, schemaname, CATEG_DOMAIN, &specif.domain_num))
#else
      if (cd_Find_prefixed_object(CI->cdp, schemaname, domname, CATEG_DOMAIN, &specif.domain_num))
#endif
        { SET_ERR_MSG(CI->cdp, domname);  c_error(OBJECT_NOT_FOUND); }
      tp=ATT_DOMAIN;
     // store the domain name in ta: 
      if (ta!=NULL)
      { char * nm=(char*)ta->names.acc(curr_attr_num);   /* same index as the attribute */
        /* This may not be the last attribute (when altering) */
        if (!nm) c_error(OUT_OF_MEMORY);
        strcpy(nm, schemaname);  strcpy(nm+sizeof(tobjname), domname);
      }
      break;
    }

    default: c_error(UNKNOWN_TYPE_NAME);
  }
 // COLLATE clause may be specified as part of the type (for table columns may be specified on the end of the column definition, too):
  analyse_collation(CI, tp, specif);
}

tptr get_default_value(CIp_t CI)  // DEFAULT is current on entry, copies the default clause
{ tptr defval = NULL;
  next_sym(CI);
  if (CI->cursym==S_NULL) next_sym(CI);
  else
  { ctptr start = CI->prepos;
    int inpar=0;
    if (CI->cursym==S_UNIQUE) next_sym(CI);  // single symbol
    else 
      while (CI->cursym!=S_INDEX && CI->cursym!=S_UNIQUE && CI->cursym!=S_PRIMARY && 
             CI->cursym!=S_FOREIGN && CI->cursym!=S_ADD && CI->cursym!=S_DROP && CI->cursym!=S_ALTER && 
             CI->cursym!=S_VALUES && CI->cursym &&
             CI->cursym!=S_REFERENCES && CI->cursym!=S_CHECK && CI->cursym!=S_COLLATE && CI->cursym!=SQ_NOT &&
             !CI->is_ident("IGNORE_CASE") &&
             (CI->cursym!=',' && CI->cursym!=';' || inpar))
      { if (CI->cursym=='(') inpar++;
        if (CI->cursym==')') if (inpar) inpar--;  else break; 
        next_sym(CI);
      }
    size_t len=CI->prepos-start;
   // remove spaces and CRLFs from the end (used in domain definition):
    while (len && (start[len-1]==' ' || start[len-1]=='\r' || start[len-1]=='\n')) len--;
    if (len)
    { defval=(tptr)corealloc(len+1, 67);
      if (defval==NULL) c_error(OUT_OF_MEMORY);
      memcpy(defval, start, len);  defval[len]=0;
    }
  }
  return defval;
}

CFNC DllKernel void WINAPI compile_domain_ext(CIp_t CI, t_type_specif_info * tsi)
{ do
  { if (CI->cursym==S_DEFAULT)
    { if (tsi->defval!=NULL) c_error(GARBAGE_AFTER_END);
      tsi->defval=get_default_value(CI);
    }
    else if (CI->cursym==SQ_NOT)  // must be before CHECK in the source, otherwise LL(1) conflict
    { next_and_test(CI, S_NULL, NULL_EXPECTED);
      next_sym(CI);
      tsi->nullable=false;
    }
    else if (CI->cursym==S_CHECK)
    { if (tsi->check!=NULL) c_error(GARBAGE_AFTER_END);
      tsi->check=read_check_clause(CI, tsi->co_cha);
    }
    else break;
  } while (true);
}

void table_def_comp(CIp_t CI, table_all * ta, table_all * ta_old, table_statement_types statement_type)
// Compiles the SQL table description, declaration and CREATE TABLE, ALTER TABLE, CREATE INDEX and DROP INDEX statements.
/* On entry, TABLE or [UNIQUE] INDEX is in the cursym */
// ta_old==NULL iff compiling description, declaration or CREATE TABLE statement.
//  do ta_old se pak zapisou jmena a typy sloupcu a prenesou se tam stare indexy a omezeni
// Pravidlo: v prikazech muze byt jmeno schematu, v databazi se neuklada (kdyz je ulozeno, ignoruje se)
{ tobjname constr_name;  int tp;  int curr_attr_num;
  atr_all * att;  int i;
  bool altering = ta_old != NULL;  bool was_primary_key=false;
  bool preserve_flags = false;  
 // init. the [ta]:
  ta->attr_changed=FALSE;
  ta->tabdef_flags=0;
  if (!altering)  /* the DELETED attribute: */
  { att=ta->attrs.next();  ta->names.next();
    if (!att) c_error(OUT_OF_MEMORY);
    att->nullable=wbtrue;  att->type=ATT_BOOLEAN;  att->specif.set_null();  att->multi=1;  /* non-null values */
    strcpy(att->name, "DELETED");
  }
 // analyse the beginning (stops on the sybol before table name):
  if (statement_type==CREATE_INDEX || statement_type==CREATE_INDEX_COND)  // the CI->cursym has been tested
  { if (CI->cursym==S_UNIQUE)
       { tp = INDEX_UNIQUE;  next_sym(CI); }
    else tp = INDEX_NONUNIQUE;
    test_and_next(CI, S_INDEX, INDEX_EXPECTED);
    if (CI->cursym != S_IDENT) c_error(IDENT_EXPECTED);
    strmaxcpy(constr_name, CI->curr_name, sizeof(constr_name));
    next_and_test(CI, S_ON, ON_EXPECTED);
    preserve_flags = true;
  }
  else if (statement_type==DROP_INDEX || statement_type==DROP_INDEX_COND)  // the CI->cursym has been tested
  { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    strmaxcpy(constr_name, CI->curr_name, sizeof(constr_name));
    next_and_test(CI, S_FROM, FROM_EXPECTED);
    preserve_flags = true;
  }
  else
    if (CI->cursym != SQ_TABLE) c_error(GENERAL_TABDEF_ERROR);
 // analyse table name: 
  if (next_sym(CI) != S_IDENT) c_error(IDENT_EXPECTED);
  strmaxcpy(ta->selfname, CI->curr_name, OBJ_NAME_LEN+1);
  if (next_sym(CI)=='.')
  { strcpy(ta->schemaname, ta->selfname);
#ifdef SRVR
    reduce_schema_name(CI->cdp, ta->schemaname);
#endif
    next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    strmaxcpy(ta->selfname, CI->curr_name, OBJ_NAME_LEN+1);
    next_sym(CI);
  }
  // when schema is not specified, the recursive compilation must not overwrite the schema from the ALTER statement!
  if (statement_type==ALTER_TABLE && CI->cursym==S_IDENT && !strcmp(CI->curr_name, "PRESERVE_FLAGS"))
  { next_sym(CI);
    preserve_flags=true;
  }
  int inspos=1, indx_inspos=0;

#ifdef SRVR
  if (altering)  // compile the old table to ta
  { bool old_indexes_are_valid = statement_type==CREATE_INDEX      || statement_type==DROP_INDEX      ||
                                 statement_type==CREATE_INDEX_COND || statement_type==DROP_INDEX_COND ||
                                 !(CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE);
    tobjname tn, sn;  strcpy(tn, ta->selfname);  strcpy(sn, ta->schemaname);    /* save name: old table def. clears it: */
   // find the original table:
    tobjnum tabnum;  if (name_find2_obj(CI->cdp, tn, sn, CATEG_TABLE, &tabnum))
      { SET_ERR_MSG(CI->cdp, sn);  c_error(TABLE_DOES_NOT_EXIST); }
    if (partial_compile_to_ta(CI->cdp, tabnum, ta))
      CI->propagate_error();
    strcpy(ta->selfname, tn);  strcpy(ta->schemaname, sn); // orig. names restored
   // copy column names to ta_old, used when converting privileges:
    for (i=0;  i<ta->attrs.count();  i++)
    { atr_all * aln = ta->attrs.acc0(i);
      atr_all * alo = ta_old->attrs.next();
      if (alo) 
      { strcpy(alo->name, aln->name);
        alo->type=aln->type;  alo->specif=aln->specif;  alo->multi=aln->multi;  alo->nullable=aln->nullable;  // type info used when searching new indices in the old table def.
      }
    }
   // remove the system attributes and indicies from the old table (will be created according the new flags)
    if (preserve_flags)
      inspos=ta->attrs.count();
    else
      while (ta->attrs.count() > 1 && !memcmp(ta->attrs.acc0(1)->name, "_W5_", 4))
        { ta->attrs.delet(1);   ta->names.delet(1); }
   /* move old constrains to ta_old */
    if (!old_indexes_are_valid)
    { memcpy(&ta_old->indxs,  &ta->indxs,  sizeof(index_dynar));
      memcpy(&ta_old->checks, &ta->checks, sizeof(check_dynar));
      memcpy(&ta_old->refers, &ta->refers, sizeof(refer_dynar));
      ta->indxs.drop();  ta->checks.drop();  ta->refers.drop();
    }
    else  // mark old constrains as OLD, automatic as INDEX_DEL
    { for (i=0;  i<ta->indxs.count();  i++) 
      { ind_descr * indx = ta->indxs.acc0(i);
        if (!memcmp(indx->constr_name, "_W5_", 4))
          { if (preserve_flags) indx_inspos++;  else indx->state=INDEX_DEL; }
        else if (*indx->constr_name=='@')
          indx->state=INDEX_DEL;
        else 
          indx->state=i;
        if (indx->index_type==INDEX_PRIMARY) was_primary_key=true;
      }
      for (i=0;  i<ta->checks.count();  i++) ta->checks.acc0(i)->state=ATR_OLD;
      for (i=0;  i<ta->refers.count();  i++) ta->refers.acc0(i)->state=ATR_OLD;
     // must define the number of old indicies, used later:
      if (ta->indxs.count()) ta_old->indxs.acc(ta->indxs.count()-1);
    }
    ta->attr_changed=FALSE;
  }
#endif // SRVR

  if (statement_type==CREATE_INDEX || statement_type==CREATE_INDEX_COND)
    add_table_index(CI, ta, constr_name, tp, statement_type==CREATE_INDEX_COND);
  else if (statement_type==DROP_INDEX || statement_type==DROP_INDEX_COND)
  { bool found=false;
    for (i=0;  !found && i<ta->indxs.count();  i++) 
      if (!strcmp(ta->indxs.acc0(i)->constr_name, constr_name))
        { ta->indxs .acc0(i)->state=INDEX_DEL;  found=true;  break; }
    if (!found && statement_type==DROP_INDEX)
      { SET_ERR_MSG(CI->cdp, constr_name);  c_error(OBJECT_NOT_FOUND); }
    // otherwise the table remains unchanged
  }
  else
  {// table flags:
    uns16 orig_flags=ta->tabdef_flags;  
    if (!preserve_flags) ta->tabdef_flags=0;
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "NO_JOURNAL"))
    { next_sym(CI);
      ta->tabdef_flags|=TFL_NO_JOUR;
    }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "ZCR"))
    { next_sym(CI);
      att=ta->attrs.insert(inspos);  if (!att) c_error(OUT_OF_MEMORY);
      ta->names.insert(inspos++);
      write_att(CI, att, "_W5_ZCR", ATT_BINARY, t_specif(ZCR_SIZE), FALSE);
      ind_descr * indx=make_attr_index(CI, ta, att, S_INDEX, indx_inspos++);
      strcpy(indx->constr_name, "_W5_ZCR_KEY");
      ta->tabdef_flags|=TFL_ZCR;
    }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "UNIKEY"))
    { next_sym(CI);
      att=ta->attrs.insert(inspos);  if (!att) c_error(OUT_OF_MEMORY);
      ta->names.insert(inspos++);
      write_att(CI, att, "_W5_UNIKEY", ATT_BINARY, t_specif(UUID_SIZE+sizeof(uns32)), orig_flags & TFL_UNIKEY);
      ind_descr * indx=make_attr_index(CI, ta, att, S_UNIQUE, indx_inspos++);
      strcpy(indx->constr_name, "_W5_UNIKEY");
      ta->tabdef_flags|=TFL_UNIKEY;
    }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "LUO"))
    { next_sym(CI);
      att=ta->attrs.insert(inspos);  if (!att) c_error(OUT_OF_MEMORY);
      ta->names.insert(inspos++);
      write_att(CI, att, "_W5_LUO", ATT_BINARY, t_specif(UUID_SIZE), FALSE);
      ta->tabdef_flags|=TFL_LUO;
    }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "TOKEN"))
    { next_sym(CI);
      att=ta->attrs.insert(inspos);  if (!att) c_error(OUT_OF_MEMORY);
      ta->names.insert(inspos++);
      write_att(CI, att, "_W5_TOKEN", ATT_INT16, t_specif(), orig_flags & TFL_TOKEN);
      ind_descr * indx=make_attr_index(CI, ta, att, S_INDEX, indx_inspos++);
      strcpy(indx->constr_name, "_W5_TOKEN");
      ta->tabdef_flags|=TFL_TOKEN;
    }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "DETECT"))
    { next_sym(CI);
      att=ta->attrs.insert(inspos);  if (!att) c_error(OUT_OF_MEMORY);
      ta->names.insert(inspos++);
      write_att(CI, att, "_W5_DETECT1", ATT_BINARY, t_specif(2*ZCR_SIZE), orig_flags & TFL_DETECT);
      ta->tabdef_flags|=TFL_DETECT;
    }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "VARIANT"))
    { next_sym(CI);
      att=ta->attrs.insert(inspos);  if (!att) c_error(OUT_OF_MEMORY);
      ta->names.insert(inspos++);
      write_att(CI, att, "_W5_VARIANT", ATT_BINARY, t_specif(16), orig_flags & TFL_VARIANT);
      ta->tabdef_flags|=TFL_VARIANT;
    }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "REC_PRIVILS"))
    { next_sym(CI);
      att=ta->attrs.insert(inspos);  if (!att) c_error(OUT_OF_MEMORY);
      ta->names.insert(inspos++);
      write_att(CI, att, "_W5_RIGHTS", ATT_NOSPEC, t_specif(SPECIF_VARLEN), orig_flags & TFL_REC_PRIVILS);
      ta->tabdef_flags|=TFL_REC_PRIVILS;
    }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "DOCFLOW"))
    { next_sym(CI);
      att=ta->attrs.insert(inspos);  if (!att) c_error(OUT_OF_MEMORY);
      ta->names.insert(inspos++);
      write_att(CI, att, "_W5_DOCFLOW", ATT_BINARY, t_specif(24), orig_flags & TFL_DOCFLOW);
      ta->tabdef_flags|=TFL_DOCFLOW;
    }
    if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "REPL_DEL"))
    { next_sym(CI);
      ta->tabdef_flags|=TFL_REPL_DEL;
    }
    if (altering) 
    { if (ta->tabdef_flags!=orig_flags) ta->attr_changed=TRUE;
      if (CI->cursym == S_SOURCE_END || CI->cursym == ';') return;  /* all constrains removed only */
    }
    if (CI->cursym=='(') next_sym(CI);   /* SQL requires this! */
   /* table element list: */
    do
    { if (CI->cursym==S_CONSTRAINT)
      { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        strmaxcpy(constr_name, CI->curr_name, OBJ_NAME_LEN+1);  next_sym(CI);
      }
      else constr_name[0]=0;
      att=NULL;  // important when adding a comment to the column
      switch(CI->cursym)
      { case S_CHECK:
#ifdef SRVR
          if (altering && !(CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE)) c_error(INCOMPATIBLE_SYNTAX);
#endif
          check_clause(CI, ta, constr_name, -1);    break;
        case S_FOREIGN:
#ifdef SRVR
          if (altering && !(CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE)) c_error(INCOMPATIBLE_SYNTAX);
#endif
          foreign_key_clause(CI, ta, constr_name);  break;
        case S_PRIMARY:
#ifdef SRVR
          if (altering && !(CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE)) c_error(INCOMPATIBLE_SYNTAX);
#endif
          next_and_test(CI, S_KEY, KEY_EXPECTED);
          if (was_primary_key) c_error(TWO_PRIMARY_KEYS); else was_primary_key=true;
          next_sym(CI);
          add_table_index(CI, ta, constr_name, INDEX_PRIMARY);    break;
        case S_UNIQUE:
#ifdef SRVR
          if (altering && !(CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE)) c_error(INCOMPATIBLE_SYNTAX);
#endif
          next_sym(CI);
          add_table_index(CI, ta, constr_name, INDEX_UNIQUE);     break;
        case S_INDEX:
#ifdef SRVR
          if (altering && !(CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE)) c_error(INCOMPATIBLE_SYNTAX);
#endif
          next_sym(CI);
          add_table_index(CI, ta, constr_name, INDEX_NONUNIQUE);  break;
        case S_DROP:
          if (constr_name[0]) c_error(GENERAL_TABDEF_ERROR);
          if (!altering) c_error(GENERAL_TABDEF_ERROR);
          if (next_sym(CI)==S_CONSTRAINT)
          { 
#ifdef SRVR
            if (CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE) c_error(INCOMPATIBLE_SYNTAX);
#endif
            next_and_test(CI, S_IDENT, IDENT_EXPECTED);
            BOOL found=FALSE;
            for (i=0;  !found && i<ta->indxs.count();  i++) 
              if (!strcmp(ta->indxs .acc0(i)->constr_name, CI->curr_name))
              { if (ta->indxs .acc0(i)->index_type==INDEX_PRIMARY) was_primary_key=false; // can ADD PRIMARY KAY later in this statement
                  ta->indxs .acc0(i)->state=INDEX_DEL;  found=TRUE; 
              }
            for (i=0;  !found && i<ta->checks.count();  i++) 
              if (!strcmp(ta->checks.acc0(i)->constr_name, CI->curr_name))
                { ta->checks.acc0(i)->state=ATR_DEL;  found=TRUE; }
            for (i=0;  !found && i<ta->refers.count();  i++) 
              if (!strcmp(ta->refers.acc0(i)->constr_name, CI->curr_name))
                { ta->refers.acc0(i)->state=ATR_DEL;  found=TRUE; }
            if (!found)
              { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(OBJECT_NOT_FOUND); }
          }
          else // dropping column
          { if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "COLUMN")) next_sym(CI); // SQL syntax
            if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
            ta->attr_changed=TRUE;
            BOOL found=FALSE;
            for (i=1; i<ta->attrs.count(); i++) 
            { att=ta->attrs.acc0(i);
              if (!strcmp(att->name, CI->curr_name))
              { att->name[0]=0;  // marking for deletion, must not drop immediately, column numbers would be changed
                found=TRUE;  break;
              }
            }
            if (!found)
              { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(ATTRIBUTE_NOT_FOUND); } /* attribute name not found */
          }
          next_sym(CI);
          if      (CI->cursym==S_CASCADE)  { ta->cascade=TRUE;   next_sym(CI); }
          else if (CI->cursym==S_RESTRICT) { ta->cascade=FALSE;  next_sym(CI); }
          break;
        case S_ALTER:
        { if (constr_name[0]) c_error(GENERAL_TABDEF_ERROR);
          if (!altering) c_error(GENERAL_TABDEF_ERROR);
          if (next_sym(CI)==S_IDENT && !strcmp(CI->curr_name, "COLUMN")) next_sym(CI); // SQL syntax
          if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
          BOOL found=FALSE;
          for (i=1; i<ta->attrs.count(); i++)
          { att=ta->attrs.acc0(i);
            if (!strcmp(att->name, CI->curr_name))
              { curr_attr_num=i;  found=TRUE;  break; }
          }
          if (!found)
            { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(ATTRIBUTE_NOT_FOUND); } /* attribute name not found */
          next_sym(CI);
          if (CI->cursym==S_DROP)
          { next_and_test(CI, S_DEFAULT, ERROR_IN_DEFAULT_VALUE);
            corefree(att->defval);  att->defval=NULL;
            next_sym(CI);
          }
          else if (CI->cursym==S_SET)
          { next_and_test(CI, S_DEFAULT, ERROR_IN_DEFAULT_VALUE);
            corefree(att->defval);  att->defval=NULL;
            att->defval=get_default_value(CI);
          }
          else 
          { att->atr_all::~atr_all();  /* free and null default value */
            ta->attr_changed=TRUE;
            goto alter_attr;
          }
          break;    
        }
        case S_ADD:
          if (constr_name[0]) c_error(GENERAL_TABDEF_ERROR);
          if (!altering) c_error(GENERAL_TABDEF_ERROR);
          next_sym(CI);
          if (CI->cursym==S_CONSTRAINT)
          { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
            strmaxcpy(constr_name, CI->curr_name, sizeof(constr_name));  next_sym(CI);
          }
          else constr_name[0]=0;
          switch (CI->cursym)
          { case S_PRIMARY:
#ifdef SRVR
              if (CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE) c_error(INCOMPATIBLE_SYNTAX);
#endif
              next_and_test(CI, S_KEY, KEY_EXPECTED);
              if (was_primary_key) c_error(TWO_PRIMARY_KEYS); else was_primary_key=true;
              next_sym(CI);
              add_table_index(CI, ta, constr_name, INDEX_PRIMARY);    break;
            case S_UNIQUE:
#ifdef SRVR
              if (CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE) c_error(INCOMPATIBLE_SYNTAX);
#endif
              next_sym(CI);
              add_table_index(CI, ta, constr_name, INDEX_UNIQUE);     break;
            case S_INDEX:
#ifdef SRVR
              if (CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE) c_error(INCOMPATIBLE_SYNTAX);
#endif
              next_sym(CI);
              add_table_index(CI, ta, constr_name, INDEX_NONUNIQUE);  break;
            case S_CHECK:
#ifdef SRVR
              if (CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE) c_error(INCOMPATIBLE_SYNTAX);
#endif
              check_clause(CI, ta, constr_name, -1);                  break;
            case S_FOREIGN:
#ifdef SRVR
              if (CI->cdp->sqloptions & SQLOPT_OLD_ALTER_TABLE) c_error(INCOMPATIBLE_SYNTAX);
#endif
              foreign_key_clause(CI, ta, constr_name);                break;
            default:
              ta->attr_changed=TRUE;
              if (CI->cursym=='(') next_sym(CI);   /* SQL ALTER requires this! ?? */
              if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "COLUMN")) next_sym(CI); // SQL syntax
              if (CI->cursym==S_INTEGER)
                { curr_attr_num=inspos+check_integer(CI, 0, MAX_TABLE_COLUMNS-1)-1;  next_sym(CI); }
              else curr_attr_num=-1;
              if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
              goto adding_column;                  
          }
          break;
        case S_IDENT:   /* attribute definition */
          curr_attr_num=-1;
         adding_column:
          if (constr_name[0]) c_error(GENERAL_TABDEF_ERROR);
          if (curr_attr_num==-1)
          { curr_attr_num=ta->attrs.count();
            att=ta->attrs.next();
          }
          else
          { att=ta->attrs.insert(curr_attr_num);
            ta->names.insert(curr_attr_num);
          }
          if (!att) c_error(OUT_OF_MEMORY);
          strmaxcpy(att->name, CI->curr_name, ATTRNAMELEN+1);  /* attribute name */
          next_sym(CI);
         alter_attr:
          analyse_type(CI, tp, att->specif, ta, curr_attr_num);
          att->type=(uns8)tp;  

         /* multiattribute properties: */
          if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "PREALLOC"))
            { next_sym(CI);  att->multi=(uns8)get_num_val(CI, 0, 127); }
          else att->multi=1;
          if ((CI->cursym==S_IDENT) && !strcmp(CI->curr_name, "EXPANDABLE"))
            { next_sym(CI);  att->multi |= 0x80; }
          if (att->multi!=1)
            if (IS_EXTLOB_TYPE(att->type))  // may disable for some other types, too
              c_error(SQL_SYNTAX);

         /* default clause (ta->attr_changed must have been set due to UNIQUE default value) */
          att->defval=NULL;
          if (CI->cursym==S_DEFAULT)
            att->defval=get_default_value(CI);

         /* column constraints: */
          att->nullable=wbtrue;  // default is NULLABLE
          do
          { switch (CI->cursym)
            { case SQ_NOT:
                next_and_test(CI, S_NULL, NULL_EXPECTED);
                att->nullable=wbfalse;
                next_sym(CI);  /* skip the last symbol of the column constraint */
                break;
              case S_NULL:
                att->nullable=wbtrue;
                next_sym(CI);  
                break;
              case S_PRIMARY:
                next_and_test(CI, S_KEY, KEY_EXPECTED);
                if (was_primary_key) c_error(TWO_PRIMARY_KEYS); else was_primary_key=true;
                make_attr_index(CI, ta, att, S_PRIMARY, -1);
                next_sym(CI);  /* skip the last symbol of the column constraint */
                break;
              case S_UNIQUE:
              case S_INDEX:
                make_attr_index(CI, ta, att, CI->cursym, -1);
                next_sym(CI);  /* skip the last symbol of the column constraint */
                break;
              case S_CHECK:
                check_clause(CI, ta, NULLSTRING, curr_attr_num);
                break;
              case S_REFERENCES:
                make_forkey(CI, ta, NULLSTRING, att->name, (int)strlen(att->name));
                break;
              default: goto columnend;
            }
          } while (TRUE);  /* cycle on column constrains */
         columnend:
         // this is in analyse_type, but COLLATE may be after the DEFAULT clause!!
          analyse_collation(CI, tp, att->specif);
          if (altering) /* VALUES always generated for NEW and MODIF attributes */
          { const char * specif_val;
            if (CI->cursym==S_VALUES)
            { next_sym(CI);
              if (CI->cursym!=S_STRING && CI->cursym!=S_CHAR) c_error(STRING_EXPECTED);
              specif_val = CI->curr_string();
              att->alt_value=(tptr)corealloc(strlen(specif_val)+1, 92);
              if (!att->alt_value) c_error(OUT_OF_MEMORY);
              strcpy(att->alt_value, specif_val);
              next_sym(CI);
            }
            else  /* used when compiling ODBC ALTER statement, prevents copying the value */
            { att->alt_value=(tptr)corealloc(1, 92);
              if (!att->alt_value) c_error(OUT_OF_MEMORY);
              att->alt_value[0]=0;
            }
          }
          break;
        case ')': 
          if (altering) break;  // empty list is OK when all contrains deleted in the ALTER and there is no other change
          if (ta->attrs.count()==1) c_error(NO_ATTRS_DEFINED);  // more informative error
          // cont.
        default: c_error(IDENT_EXPECTED);
      } /* switch on create table element */
     // comments: if there are comments before and after ',' or ')', the first one is lost
     //  comment in alter replaces the original comment
      if (att && CI->last_comment.has_comment()) { corefree(att->comment);  att->comment=CI->last_comment.get_comment(); }
      if (CI->cursym!=',') break;
      next_sym(CI);
      if (att && CI->last_comment.has_comment()) { corefree(att->comment);  att->comment=CI->last_comment.get_comment(); }
    } while (TRUE);  /* cycle on creating table element */
    if (CI->cursym==')') next_sym(CI);   /* SQL requires this! */
    if (att && CI->last_comment.has_comment()) { corefree(att->comment);  att->comment=CI->last_comment.get_comment(); }
   /* remove the deleted attributes: */
    for (i=1;  i<ta->attrs.count(); )
      if (!ta->attrs.acc(i)->name[0])
        { ta->attrs.delet(i);  ta->names.delet(i); }
      else i++;
   // check the number of columns and the name duplicity:
    if (ta->attrs.count()==1) c_error(NO_ATTRS_DEFINED);
    if (ta->attrs.count()>MAX_TABLE_COLUMNS) c_error(TOO_MANY_TABLE_ATTRS);  // after removing drooped columns (MAX_TABLE_COLUMNS incl DELETED is allowed)
    { t_name_hash name_hash(ta->attrs.count());
      for (i=1;  i<ta->attrs.count();  i++)
        if (!name_hash.add(ta->attrs.acc0(i)->name))
          c_error(NAME_DUPLICITY);
    }
  } // CREATE or ALTER TABLE statements

 // add automatic indicies defined by local RI rules, define ref->loc_index_num:
 // If not added here ALTER TABLE cannot determine which indicies have to be added/deleted/moved.
#ifdef SRVR
  for (i=0;  i<ta->refers.count(); i++)
  { forkey_constr * ref = ta->refers.acc0(i);
    if (ref->state==ATR_DEL) continue;
    ref->loc_index_num = find_index_for_key(CI->cdp, ref->text, ta);
    if (ref->loc_index_num<0)
    { if (ref->loc_index_num!=-1) // index cannot be added
        c_error(NO_LOCAL_INDEX);
      ref->loc_index_num=ta->indxs.count();
      ind_descr * indx = ta->indxs.next();
      if (!indx) c_error(OUT_OF_KERNEL_MEMORY);
      strcpy(indx->constr_name, "@");
      indx->index_type=INDEX_NONUNIQUE;
      indx->has_nulls = true;
      indx->state=INDEX_NEW;
      indx->co_cha=COCHA_UNSPECIF;
      indx->text=(tptr)corealloc(strlen(ref->text)+1, 89);
      if (!indx->text) c_error(OUT_OF_KERNEL_MEMORY);
      strcpy(indx->text, ref->text);
    }
  }
#endif
 // check the number of indicies:
  unsigned ind_count=0;
  for (i=0;  i<ta->indxs.count();  i++) 
    if (ta->indxs.acc0(i)->state!=INDEX_DEL) 
      ind_count++;
  if (ind_count>INDX_MAX) c_error(TOO_MANY_INDXS);     
}

DllKernel void WINAPI table_def_comp_link(CIp_t CI)
{ if (CI->cursym==(CI->stmt_ptr ? S_ALTER : S_CREATE)) next_sym(CI);
  table_def_comp(CI, (table_all *)CI->univ_ptr, (table_all *)CI->stmt_ptr, CI->stmt_ptr ? ALTER_TABLE : CREATE_TABLE);
}

///////////////////////////////////////// hints ////////////////////////////////////////
/* Storing "hints" in the comment:
Every hint is identified by the hint_name. Hints are stored on the beginning of the comment in the following form:
%hint_name space hint text with duplicated %-characters %
Every hint ends before a non-duplicate %
The real comment is after the hints. Starts with % and a space, if there are any hints, otherwise starts on the very beginning.
*/
CFNC DllKernel void WINAPI get_hint_from_comment(const char * comment, const char * hint_name, char * buffer, int bufsize)
// Looks for the hint specified by the [hint_name] stored in the [comment]. 
// If found, copies it to the buffer (and removes duplicate % in it). Otherwise sets *buffer to 0.
// [bufsize]>0 supposed.
{ if (comment)
    while (*comment=='%')  // cycle on parts of the comment
    { comment++;
      if (*comment==' ') // no more hints, the real comment starts here
        break;
     // check the name of the current hint:
      int pos=0;
      while (hint_name[pos] && hint_name[pos]==comment[pos]) pos++;
      if (!hint_name[pos] && comment[pos]==' ') // found [hint_name]
      { comment+=pos+1;
       // copy the hint text:
        while (*comment)
        { if (*comment=='%')
            if (comment[1]!='%') break;  // end of the hint
            else comment++;  // skip the 1st % in the duplicated %
          if (bufsize>1)
            { *buffer=*comment;  buffer++;  bufsize--; }
          comment++;
        }
        break; // hint copied, stop analysing the comment
      }
      else // skip the hint
      { comment+=pos;
        while (*comment)
        { if (*comment=='%')
            if (comment[1]!='%') break;  // end of the hint
            else comment++;  // skip the 1st % in the duplicated %
          comment++;
        }
        // now either *comment==0 or *comment=='%' on the beginning of the next part
      }
    }
  *buffer=0;  // terminate the (possibly non-existing) hint
}

CFNC DllKernel const char * WINAPI skip_hints_in_comment(const char * comment)
// Returns the pointer to the real comment, after all hints. If there are no hints, returns [comment].
// If there are only hints in the comment, returns "".
{ if (!comment) return NULLSTRING;
  while (*comment=='%')  // cycle on parts of the comment
  { comment++;
    if (*comment==' ') // no more hints, the real comment starts here
      { comment++;  break; }
   // skip the hint:
    while (*comment)
    { if (*comment=='%')
        if (comment[1]!='%') break;  // end of the hint
        else comment++;  // skip the 1st % in the duplicated %
      comment++;
    }
    // now either *comment==0 or *comment=='%' on the beginning of the next part
  }
  return comment;
}
/***************************** to source ************************************/
CFNC DllKernel void WINAPI write_constraint_characteristics(int co_cha, t_flstr * src)
{ src->put( 
    co_cha==COCHA_UNSPECIF       ? "" :
    co_cha==COCHA_IMM_DEFERRABLE ? " INITIALLY IMMEDIATE DEFERRABLE " :
    co_cha==COCHA_IMM_NONDEF     ? " INITIALLY IMMEDIATE " :   // NOT DEFERRABLE IMPLIED
    co_cha==COCHA_DEF            ? " INITIALLY DEFERRED " : "");
}

void write_collation_description(t_specif specif, char * buf)
// Puts a space before and after the specification.
{ *buf=0;  // allows simple use of strcat, remains unchanged for specif.charset==0 && !specif.ignore_case
  if (specif.charset!=0 || specif.ignore_case) // write unless default
  { if (specif.charset==1) // for backward compatibility not using CZ
      if (specif.ignore_case) strcpy(buf, " COLLATE CSISTRING ");
      else                    strcpy(buf, " COLLATE CSSTRING ");
    else 
    { if (specif.charset>1)
      { const char * col = get_collation_name(specif.charset);
        if (*col)  // found
          { strcpy(buf, " COLLATE ");  strcat(buf, col);  strcat(buf, " "); }
      }
      if (specif.ignore_case)  // for specif.charset>1 and specif.charset==0 
        strcat(buf, " IGNORE_CASE ");
    }
  }
}

CFNC DllKernel void WINAPI write_collation(t_specif specif, t_flstr & src)
{ char buf[25+OBJ_NAME_LEN+1];
  write_collation_description(specif, buf);
  src.put(buf);
}

CFNC DllKernel void WINAPI sql_type_name(int wbtype, t_specif specif, bool full_name, char * buf)
// Since version 9.0 writes the collation clause for character types, too.
{ switch (wbtype)
  { case ATT_BOOLEAN:  strcpy(buf, "BIT");  break;
    case ATT_CHAR:     strcpy(buf, specif.wide_char ? "NCHAR" : "CHAR");  break; /* differs from CHAR(1) */
    case ATT_INT8:     
      if (specif.scale==0) strcpy(buf, "TINYINT");
      else sprintf(buf, full_name ? "NUMERIC(2,%d)" : "NUMERIC", specif.scale);  break;
    case ATT_INT64:
      if (specif.scale==0) strcpy(buf, "BIGINT");
      else sprintf(buf, full_name ? "NUMERIC(18,%d)": "NUMERIC", specif.scale);  break;
    case ATT_INT16:    
      if (specif.scale==0) strcpy(buf, "SMALLINT");
      else sprintf(buf, full_name ? "NUMERIC(4,%d)" : "NUMERIC", specif.scale);  break;
    case ATT_INT32:    
      if (specif.scale==0) strcpy(buf, "INTEGER");
      else sprintf(buf, full_name ? "NUMERIC(9,%d)" : "NUMERIC", specif.scale);  break;
    case ATT_MONEY:    strcpy(buf, full_name ? "NUMERIC(14,2)" : "NUMERIC");      break;
    case ATT_FLOAT:    strcpy(buf, "REAL");                break;
    case ATT_STRING:  
      if (specif.wide_char) sprintf(buf, full_name ? "NCHAR(%u)" : "NCHAR", specif.length/2); 
      else                  sprintf(buf, full_name ? "CHAR(%u)" : "CHAR",  specif.length);  break;
    case ATT_BINARY:
      sprintf(buf, full_name ? "BINARY(%u)" : "BINARY",  specif.length);  break;
    case ATT_DATE:     strcpy(buf, "DATE");                break;
    case ATT_TIME:     strcpy(buf, specif.with_time_zone ? "TIME WITH TIME ZONE"      : "TIME");       break;
    case ATT_TIMESTAMP:strcpy(buf, specif.with_time_zone ? "TIMESTAMP WITH TIME ZONE" : "TIMESTAMP");  break;
    case ATT_PTR:      strcpy(buf, "POINTER");             break;
    case ATT_BIPTR:    strcpy(buf, "BIPTR");               break;
    case ATT_AUTOR:    strcpy(buf, "AUTOR");               break;
    case ATT_DATIM:    strcpy(buf, "DATIM");               break;
    case ATT_HIST:     strcpy(buf, "HISTORY");             break;
    case ATT_TEXT:     strcpy(buf, specif.wide_char ? "NCLOB" : "CLOB");  break;
    case ATT_EXTCLOB:  strcpy(buf, specif.wide_char ? "EXTNCLOB" : "EXTCLOB");    break;
    case ATT_RASTER:   strcpy(buf, full_name ? "BINARY(65000)" : "BINARY");       break;
    case ATT_NOSPEC:   strcpy(buf, "BLOB");                break;
    case ATT_EXTBLOB:  strcpy(buf, "EXTBLOB");             break;
    case ATT_SIGNAT:   strcpy(buf, "SIGNATURE");           break;
    case ATT_DOMAIN:   *buf=0;  break;  // domain name will be copied later
    default:           *buf=0;  break;  /* error */
  }
#if WBVERS>=900
 /* COLLATE: */
  if (full_name)
    if (wbtype==ATT_STRING || wbtype==ATT_CHAR || wbtype==ATT_TEXT || wbtype==ATT_EXTCLOB)
      write_collation_description(specif, buf+strlen(buf));
#endif
}

void comment_and_newline(t_flstr & src, const char ** pcomm, char delimiter)
// Outputs the dlimiter, comment and newline. Then clears the *pcomm (not owning pointer to the comment)
{ src.putc(delimiter);
  if (*pcomm!=NULL)
    { src.put("/*");  src.put(*pcomm);  src.put("*/");  *pcomm=NULL; }
  src.put("\r\n");
}

const char * ref_rule_descr(int rule)
{ return rule==REFRULE_SET_NULL ? "SET NULL" : rule==REFRULE_SET_DEFAULT ? "SET DEFAULT" : 
         rule==REFRULE_CASCADE  ? "CASCADE"  : rule==REFRULE_NO_ACTION   ? "NO ACTION" : "";
}

CFNC DllKernel tptr WINAPI table_all_to_source(table_all * ta, BOOL altering)
// Creates the SQL description of the table in ta. Never stores the schema name of the table.
// ta->schemaname must be specified (although not stored).
// Stores schema names of the referenced tables iff they are specified and are different than the ta->schemaname.
{ int i;  t_flstr src(3000,3000);
  src.putc(' ');  // space for the "testing" flag
  src.put(altering ? "ALTER TABLE " : "CREATE TABLE ");  
#ifndef SRVR  // must not store schema name on the server
  if (*ta->schemaname)
    { src.putq(ta->schemaname);  src.putc('.'); }
#endif
  src.putq(ta->selfname);
  if (ta->tabdef_flags & TFL_NO_JOUR    ) src.put(" NO_JOURNAL");
  if (ta->tabdef_flags & TFL_ZCR        ) src.put(" ZCR");
  if (ta->tabdef_flags & TFL_UNIKEY     ) src.put(" UNIKEY");
  if (ta->tabdef_flags & TFL_LUO        ) src.put(" LUO");
  if (ta->tabdef_flags & TFL_TOKEN      ) src.put(" TOKEN");
  if (ta->tabdef_flags & TFL_DETECT     ) src.put(" DETECT");
  if (ta->tabdef_flags & TFL_VARIANT    ) src.put(" VARIANT");
  if (ta->tabdef_flags & TFL_REC_PRIVILS) src.put(" REC_PRIVILS");
  if (ta->tabdef_flags & TFL_DOCFLOW    ) src.put(" DOCFLOW");
  if (ta->tabdef_flags & TFL_REPL_DEL   ) src.put(" REPL_DEL");
  src.put(" (\r\n");

  int startatr = (ta->attrs.count() && !stricmp(ta->attrs.acc(0)->name, "DELETED")) ? 1 : 0;
  while (ta->attrs.count() > startatr &&
         !memcmp(ta->attrs.acc(startatr)->name, "_W5_", 4))
    startatr++;

  BOOL firstel = TRUE;
  if (altering)   /* dropped attributes first: */
    for (i=startatr; i<ta->attrs.count(); i++)
    { atr_all * att = ta->attrs.acc0(i);
      if (*att->name && att->state==ATR_DEL)
      { if (firstel) firstel=FALSE;  else src.put(",\r\n");
        src.put("DROP ");  src.putq(att->name);
      }
    }
  const char * pending_comment = NULL;  // not owning pointer to the comment belonging to the previous column, to be output after ',' or ')'
  for (i=startatr; i<ta->attrs.count(); i++)
  { atr_all * att = ta->attrs.acc0(i);
    if (!*att->name || !att->type) continue;
    if (altering)
    { if (att->state==ATR_OLD || att->state==ATR_DEL) continue;
      if (firstel) firstel=FALSE;  else comment_and_newline(src, &pending_comment, ',');
      src.put(att->state==ATR_NEW ? "ADD " : /* att->state==ATR_MODIF, MINOR_MODIF */ "ALTER ");
    }
    else // CREATE TABLE
      if (firstel) firstel=FALSE;  else comment_and_newline(src, &pending_comment, ',');

    if (altering && att->state==ATR_NEW)
      { src.putint(i+1-startatr);  src.putc(' '); }
    src.putq(att->name);  src.putc(' ');
   /* type: */
    char type_name[55+OBJ_NAME_LEN];
    sql_type_name(att->type, att->specif, true, type_name);
    src.put(type_name);
    if (att->type==ATT_PTR || att->type==ATT_BIPTR)
    { src.putc('[');
      double_name * dnm = ta->names.acc(i);
      if (*dnm->schema_name) { src.putq(dnm->schema_name);  src.putc('.'); }
      src.putq(dnm->name);
      src.putc(']');
    }
    if (att->type==ATT_DOMAIN)
    { src.putc(' ');
      double_name * dnm = ta->names.acc(i);
      if (*dnm->schema_name) { src.putq(dnm->schema_name);  src.putc('.'); }
      src.putq(dnm->name);
    }
    src.putc(' ');
   /* multiattribute: */
    if (att->multi!=1)
    { src.put("PREALLOC");  src.putint(att->multi & 0x7f);  src.putc(' ');
      if (att->multi & 0x80) src.put("EXPANDABLE ");
    }
   /* default value */
    if (att->defval!=NULL)
      { src.put("DEFAULT ");  src.put(att->defval);  src.putc(' '); }
  /* NOT NULL: */
    if (!att->nullable)  src.put("NOT NULL ");
#if WBVERS<900  // since 9.00 the COLLATE clause is specified in the sql_type_name
  /* COLLATE: */
    if (att->type==ATT_STRING || att->type==ATT_CHAR || att->type==ATT_TEXT)
      write_collation(att->specif, src);
#endif
  /* VALUES: */
    if (altering)
      { src.put("VALUES ");  src.putssx(att->alt_value ? att->alt_value : NULLSTRING); }
    if (att->comment && *att->comment) pending_comment=att->comment;  // pending_comment is NULL here
  }

  for (i=0; i<ta->indxs.count(); i++)
  { ind_descr * indx=ta->indxs.acc0(i);
    if (indx->state==INDEX_DEL || !indx->text || !*indx->text || !indx->index_type) continue;
    if (!memcmp(indx->constr_name, "_W5_", 4) ||
        !memcmp(indx->constr_name, "@", 2)) continue;
    if (firstel) firstel=FALSE;  else comment_and_newline(src, &pending_comment, ',');
    if (*indx->constr_name)
      { src.put("CONSTRAINT ");  src.putq(indx->constr_name);  src.putc(' '); }
    if      ((indx->index_type & 0x7f)==INDEX_PRIMARY)   src.put("PRIMARY KEY (");
    else if ((indx->index_type & 0x7f)==INDEX_UNIQUE)    src.put("UNIQUE (");
    else if ((indx->index_type & 0x7f)==INDEX_NONUNIQUE) src.put("INDEX (");
    src.put(indx->text);  src.putc(')');
    if (indx->has_nulls != ((indx->index_type & 0x7f) == INDEX_NONUNIQUE)) // non-default state
      src.put(indx->has_nulls ? " NULL " : " NOT NULL ");
    write_constraint_characteristics(indx->co_cha, &src);
  }

  for (i=0; i<ta->checks.count(); i++)
  { check_constr * check=ta->checks.acc0(i);
    if (check->state==ATR_DEL || !check->text || !*check->text) continue;
    if (firstel) firstel=FALSE;  else comment_and_newline(src, &pending_comment, ',');
    if (*check->constr_name)
      { src.put("CONSTRAINT "); src.putq(check->constr_name); }
    src.put(" CHECK (");  src.put(check->text);  src.putc(')');
    write_constraint_characteristics(check->co_cha, &src);
  }

  for (i=0; i<ta->refers.count(); i++)
  { forkey_constr * ref=ta->refers.acc0(i);
    if (ref->state==ATR_DEL || !ref->text || !*ref->text || !*ref->desttab_name) continue;
    if (firstel) firstel=FALSE;  else comment_and_newline(src, &pending_comment, ',');
    if (*ref->constr_name)
      { src.put("CONSTRAINT "); src.putq(ref->constr_name); }
    src.put(" FOREIGN KEY (");  src.put(ref->text);  src.put(") REFERENCES ");
    if (*ref->desttab_schema && strcmp(ref->desttab_schema, ta->schemaname))
      { src.putq(ref->desttab_schema);  src.putc('.'); }
    src.putq(ref->desttab_name);  
    if (ref->par_text && *ref->par_text) 
      { src.putc('(');  src.put(ref->par_text);  src.putc(')'); }
    if (ref->update_rule!=REFRULE_NO_ACTION)
      { src.put(" ON UPDATE ");  src.put(ref_rule_descr(ref->update_rule)); }
    if (ref->delete_rule!=REFRULE_NO_ACTION)
      { src.put(" ON DELETE ");  src.put(ref_rule_descr(ref->delete_rule)); }
    write_constraint_characteristics(ref->co_cha, &src);
  }
  comment_and_newline(src, &pending_comment, ')');  // emitting ')' even for ALTER in order to delimit the comment from the DEFAULT value
  return src.error() ? NULL : src.unbind();
}

////////////////////////////////////////////////////////////////////////////////////////////
#ifdef SRVR
t_idname 
#else
tname 
#endif
      sql_keynames[SQL_KEY_COUNT] = {
"ADD", "AFTER", "ALL", "ALTER", "AND", "ANY", "AS",
"ASC", "AUTHOR", "AUTOR", "AVG",
"BEFORE", "BEGIN", "BETWEEN", "BIGINT", "BINARY", "BIPTR", "BIT", 
"BLOB", "BOOLEAN", "BY",
"CALL", "CASCADE", "CASE", "CAST", "CHAR", "CHARACTER",
"CHECK", "CLOB", "CLOSE", "COALESCE", "COLLATE", "COMMIT", "CONCAT", "CONDITION",
"CONSTANT", "CONSTRAINT", "CONTINUE",
"CORRESPONDING", "COUNT", "CREATE", "CROSS",
"CURRENT", "CURSOR", "DATE", "DATIM", "DEC", "DECIMAL", "DECLARE", "DEFAULT",
"DELETE", "DESC", "DISTINCT", "DIV", "DO", "DOMAIN", "DOUBLE", "DROP",
"ELSE", "ELSEIF", "END", "ESCAPE", "EXCEPT", "EXISTS", "EXIT", "EXTERNAL",
"FETCH", "FLOAT", "FOR", "FOREIGN",
"FROM", "FULL", "FUNCTION", "GRANT", "GROUP", "HANDLER", "HAVING", "HISTORY",
"IF", "IN", "INDEX", "INNER", "INOUT",
"INSERT", "INT", "INTEGER", "INTERSECT", "INTO", "IS", "JOIN",
"KEY", "LARGE", "LEAVE", "LEFT", "LIKE", /*"LIMIT", */ "LONG", "LOOP", "MAX", "MIN", "MOD",
"NATIONAL", "NATURAL", "NCHAR", "NCLOB", "NOT", "NULL", "NULLIF",
"NUMERIC", "OBJECT", "OF", "ON", "OPEN", "OR", "ORDER", "OTHERS", "OUT",
"OUTER", "POINTER", "PRECISION", "PRIMARY", "PROCEDURE", "PUBLIC",
"REAL", "REDO", "REFERENCES", "REFERENCING", "RELEASE", "REPEAT", "RESIGNAL", "RESTRICT",
"RETURN", "RETURNS", "REVOKE", "RIGHT", "ROLLBACK", "SAVEPOINT",
"SCHEMA", "SELECT", "SEQUENCE", "SET", "SIGNAL", "SIGNATURE", "SMALLINT", "SOME",
"SQLEXCEPTION", "SQLSTATE", "SQLWARNING", "START", "SUM",
"TABLE", "THEN", "TIME", "TIMESTAMP", "TINYINT", "TO", "TRIGGER", "TUPLE",
"UNDO", "UNION", "UNIQUE", "UNTIL", "UPDATABLE", "UPDATE", "USER",
"USING", "VALUES", "VARBINARY", "VARCHAR",
"VARYING", "VIEW", "WHEN", "WHERE", "WHILE", "WITH" };

#ifdef SRVR
t_idname 
#else
tname 
#endif
         keynames[KEY_COUNT] = {
"AND", "ARRAY", "BEGIN", "BINARY", "CASE", "COLLATE", "CONST", "CSISTRING", "CSSTRING",
"CURSOR", "DIV", "DO", "DOWNTO", "ELSE", "END",
"FILE", "FOR", "FORM", "FUNCTION", "HALT", "IF", "INCLUDE", 
"MOD", "NOT", "OF", "OR",
"PROCEDURE", "RECORD", "REPEAT", "RETURN", "STRING", "TABLE", "THEN", "THISFORM", "TO",
"TYPE", "UNTIL", "VAR", "WHILE", "WITH" };

bool search_table(const char * key, const char * patt, int pattnum, int pattstep, int * pos)
// Returns TRUE if NOT found, otherwise in [pos] returns the match/insert position.
{ int off0, off1, off2;  int res;  char termchar;
  if (!pattnum) { *pos=0;  return true; }
  size_t keylen=strlen(key);
  if (keylen>COMP_SIGNIF_CHARS)  /* NAMELEN chars significant here */
    { termchar=key[COMP_SIGNIF_CHARS];  ((tptr)key)[COMP_SIGNIF_CHARS]=0; }
  off0=0; off1=pattnum;
  while (off0+1 < off1)  /* searching by division */
  { off2=(off0+off1)/2;
    res=strcmp(key, patt+off2*pattstep);
    if (res<0) off1=off2;  else off0=off2;
  }
  if ((res=strcmp(key, patt+off0*pattstep))<=0)
    *pos=off0;
  else *pos=off0+1;
  if (keylen>COMP_SIGNIF_CHARS) /* restore the full name lenght */
    ((tptr)key)[COMP_SIGNIF_CHARS]=termchar;
  return res!=0;
}

////////////////////////// name hash ///////////////////////////////////////
t_name_hash::t_name_hash(unsigned total_count)
{
  table_size=total_count*5/3;
  hash_table = (char (*)[ATTRNAMELEN+1])corealloc(table_size*(ATTRNAMELEN+1), 57);
  if (!hash_table) return;
  for (int i=0;  i<table_size;  i++)
    hash_table[i][0]=0;
}

unsigned t_name_hash::hash(const char * name)
{ int sum=0, i=0;
  while (name[i])
    { sum+=*name;  i++; }
  return sum % table_size;
}

bool t_name_hash::add(const char * name)
{
  if (!hash_table) return true;  // allocation error, masked
  int ind = hash(name);
 // find duplicity or a free cell:
  while (hash_table[ind][0])
  { if (!strcmp(name, hash_table[ind]))
      return false;  // duplicity found
    if (++ind>=table_size) ind=0;
  }
 // free cell found, insert:
  strcpy(hash_table[ind], name);
  return true;
}

