// ftanal.cpp

const char * ft_lang_name[] = {
  "Czech",
  "Slovak",
  "German",
  "US English",
  "UK English",
#if 0  // language dictionaries not available
  "Polish",
  "Hungarian",
  "French",
#endif  
  NULL
};

void WINAPI fulltext_anal(CIp_t CI) 
{ t_fulltext * ftdef = (t_fulltext*)CI->univ_ptr;
  ftdef->compile_intern(CI);
}

bool t_fulltext::compile_flag(CIp_t CI, bool positive)
// Compiles boolean flags of the fulltext system, [positive]==false when prefixed by NOT.
{ if (!strcmp(CI->curr_name, "LEMMATIZED"))
  { basic_form=positive;
    next_sym(CI);
    return true;
  }
  if (!strcmp(CI->curr_name, "SUBSTRUCTURED"))
  { with_substructures=positive;
    next_sym(CI);
    return true;
  }
  if (!strcmp(CI->curr_name, "SEPARATED"))
  { separated=positive;
    next_sym(CI);
    return true;
  }
  if (!strcmp(CI->curr_name, "BIGINT_ID"))
  { bigint_id=positive;
    next_sym(CI);
    return true;
  }
  return false;
}

void t_fulltext::compile_intern(CIp_t CI)
// CREATE/ALTER or FULLTEXT is the current symbol, univ_ptr contains t_fulltext *, "modifying" defined in it.
// If FULLTEXT is the current symbol then [modifying] is defined outside (used when the incoming stream of SQL statements is complied).
// When [modifying] then the [objnum] is returned.
{ if      (CI->cursym==S_ALTER ) { next_sym(CI);  modifying=true;  }
  else if (CI->cursym==S_CREATE) { next_sym(CI);  modifying=false; }
 // object name: 
  next_and_test(CI, S_IDENT, IDENT_EXPECTED);
  strmaxcpy(name, CI->curr_name, sizeof(name));
  if (next_sym(CI)=='.')
  { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    if (!recursive) // must not overwrite schema name in the recursive call (schema name not stored on server, but if stored, it must be ignored)
      strcpy(schema, name);
    strmaxcpy(name, CI->curr_name, sizeof(name));
    next_sym(CI);
  }
  // when schema is not specified, the recursive compilation must not overwrite the schema from the ALTER statement!

  if (strlen(name) > 20) c_error(IDENT_EXPECTED);  // too long
#ifdef SRVR
 // load existing definition, if altering:
  if (modifying && !recursive)
  { if (name_find2_obj(CI->cdp, name, schema, CATEG_INFO, &objnum))
      { SET_ERR_MSG(CI->cdp, name);  c_error(OBJECT_NOT_FOUND); }
    char * defin = ker_load_objdef(CI->cdp, objtab_descr, objnum);
    if (!defin) c_error(CI->cdp->get_return_code());  // propagates the error
    recursive=true;
    int res = compile(CI->cdp, defin);  // compliling into my own structure
    recursive=false;  modifying=true;  // values restored 
    corefree(defin);
    if (res) c_error(res);  // propagates the error
  }
  else
#endif
 // set default values, if creating:
  { language=FT_LANG_CZ;
    basic_form=true;
    weighted=false;
    with_substructures=true;
    separated=false;  // the "NOT SEPARAED" option is not stored so this must be the default
    bigint_id=false;
    if (!recursive) objnum=NOOBJECT;
  }
 // options (not allowed in the ALTER statement):
  if (modifying && !recursive) 
    rebuild=true;  // will be changed back to false if anything follows
  if (!modifying || recursive)
    do
    { if (CI->cursym==S_IDENT)
      { rebuild=false;
        if (!strcmp(CI->curr_name, "LANGUAGE"))
        { if (next_sym(CI)=='=') next_sym(CI);
          if (CI->cursym!=S_INTEGER) c_error(INTEGER_EXPECTED);
          language = (int)CI->curr_int;
          next_sym(CI);
        }
        else if (!compile_flag(CI, true))
          break;
      }
      else if (CI->cursym==SQ_NOT)
      { rebuild=false;
        next_and_test(CI, S_IDENT, IDENT_EXPECTED);
        if (!compile_flag(CI, false))
          c_error(GENERAL_SQL_SYNTAX);   
      }
      else break;
    } while (true);
 // LIMITS option:
  if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "LIMITS"))
  { rebuild=false;
    next_and_test(CI, S_STRING, STRING_EXPECTED);
    const char * lim_val = CI->curr_string();
    corefree(limits);  // used in ALTER FULLTEXT
    limits=(tptr)corealloc(strlen(lim_val)+1, 99);
    if (!limits) c_error(OUT_OF_MEMORY);
    strcpy(limits, lim_val);
    if (*limits && !validate_limits(CI->cdp,limits)) c_error(LIMITS_CLAUSE_SYNTAX_ERROR);
    next_sym(CI);
  }
 // word characters definition
  if (!modifying || recursive)
  { if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "WORD_STARTERS"))
    { next_and_test(CI, S_STRING, STRING_EXPECTED);
      const char * sval = CI->curr_string();
      word_starters=(tptr)corealloc(strlen(sval)+1, 99);
      if (!word_starters) c_error(OUT_OF_MEMORY);
      strcpy(word_starters, sval);
      next_sym(CI);
    }
    if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "WORD_CHARS"))
    { next_and_test(CI, S_STRING, STRING_EXPECTED);
      const char * sval = CI->curr_string();
      word_insiders=(tptr)corealloc(strlen(sval)+1, 99);
      if (!word_insiders) c_error(OUT_OF_MEMORY);
      strcpy(word_insiders, sval);
      next_sym(CI);
    }
  }

  bool in_par;
  if (CI->cursym=='(') { in_par=true;  next_sym(CI); }
  else in_par=false;

  while (CI->cursym && CI->cursym!=')' && CI->cursym!=';')
  { rebuild=false;
    if (modifying && CI->cursym==S_DROP) 
    { tobjname tabname, colname, doccolname;  int i;  bool found;
      next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      strmaxcpy(tabname, CI->curr_name, sizeof(tabname));
      next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      strmaxcpy(colname, CI->curr_name, sizeof(colname));
      next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      strmaxcpy(doccolname, CI->curr_name, sizeof(doccolname));
      next_sym(CI);
      found = false;
      for (i=0;  i<items.count();  i++)
      { t_autoindex * ai = items.acc0(i);
        if (ai->tag==ATR_OLD && 
            !strcmp(ai->doctable_name, tabname) && !strcmp(ai->id_column, colname) && !strcmp(ai->text_expr, doccolname))
          { ai->tag=ATR_DEL;  found=true;  break; }
      }
      if (!found) { SET_ERR_MSG(CI->cdp, colname);  c_error(ATTRIBUTE_NOT_FOUND); } /* attribute name not found */
    }
    else
    { if (modifying)
        test_and_next(CI, S_ADD, GENERAL_SQL_SYNTAX);
      t_autoindex * ai = items.next();
      if (!ai) c_error(OUT_OF_MEMORY);
      ai->tag = recursive ? ATR_OLD : ATR_NEW;
      if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
      strmaxcpy(ai->doctable_name, CI->curr_name, sizeof(ai->doctable_name));
      next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      strmaxcpy(ai->id_column, CI->curr_name, sizeof(ai->id_column));
      next_sym(CI);
      if (CI->cursym==S_IDENT)
        strmaxcpy(ai->text_expr, CI->curr_name, sizeof(ai->text_expr));
      else if (CI->cursym==S_STRING || CI->cursym==S_CHAR)   // obsolete
        strmaxcpy(ai->text_expr, CI->curr_string(), sizeof(ai->text_expr));
      else
        c_error(IDENT_EXPECTED);
      if (next_sym(CI) == ',') next_sym(CI);
      if (CI->cursym==S_INTEGER)
        { ai->mode = (int)CI->curr_int;  next_sym(CI); }
      else ai->mode = 0;
      if (CI->cursym == ',') next_sym(CI);
      if (CI->cursym==S_STRING || CI->cursym==S_CHAR)
        { strmaxcpy(ai->format, CI->curr_string(), sizeof(ai->format));  next_sym(CI); }
      else *ai->format=0;
    }
    if (CI->cursym==',') next_sym(CI);   // comma may have been skipped above, when looking for mode/format
  }
  if (in_par) test_and_next(CI, ')', RIGHT_PARENT_EXPECTED);
}

int t_fulltext::compile(cdp_t cdp, tptr defin)
{ 
#ifdef SRVR
  compil_info xCI(cdp, defin, fulltext_anal);
#else
  compil_info xCI(cdp, defin, DUMMY_OUT, fulltext_anal);
  set_compiler_keywords(&xCI, NULL, 0, 1); /* SQL keywords selected */
#endif
  xCI.univ_ptr=this;
#ifdef SRVR
  return ::compile_masked(&xCI);
#else
  return ::compile(&xCI);
#endif
}

void base_column_name(const char * ext_name, char * name)
// Removes the " (?)" suffix from the [ext_name] and copies it to [name].
{
  int idlen=(int)strlen(ext_name);
  if (idlen>4 && !strcmp(ext_name+idlen-4, " (?)"))
    idlen-=4;
  memcpy(name, ext_name, idlen); 
  name[idlen]=0;     
}

tptr t_fulltext::to_source(cdp_t cdp, bool altering)
// Creates the SQL statement creating or altering the fulltext system.
// When [altering] the [items] with tag==ATR_OLD are ignored, the other formthe ADD and DROP clauses.
// Otherwise all [items] are writen as clauses.
{ t_flstr src(500,500);
  src.putc(' ');  // space for testing-mark
  src.put(altering ? "ALTER" : "CREATE");  src.put(" FULLTEXT `");
#ifndef SRVR
 // schema name added only when creating the SQL statement from GUI
  if (*schema) { src.put(schema);  src.put("`.`"); }
#endif
  src.put(name);  src.put("`\r\n");
 // options:
  if (!altering)
  { src.put("LANGUAGE ");    src.putint(language);  src.put("\r\n");
    if (!basic_form) src.put("NOT ");  
    src.put("LEMMATIZED\r\n");
#if WBVERS>=950
    if (!with_substructures) src.put("NOT SUBSTRUCTURED\r\n");  // compatible with version 9.0: with_substructures==true by default (but it is ignored when the table does not exist)
#endif
    if (separated) src.put("SEPARATED\r\n");  // compatible with the original 9.5 version: the original option value (NOT SEPARATED) is the default
    if (bigint_id) src.put("BIGINT_ID\r\n");  // compatible with the former versions: the original option value (NOT BIGINT_ID) is the default
  }
 
  if (limits && *limits || altering)  // if altering, must specify the empty LIMITS too, in order to remove the old LIMITS clause   
    { src.put("LIMITS ");  src.putssx(limits ? limits : "");  src.put("\r\n"); }
  if (!altering)
  { if (word_starters)
      { src.put("WORD_STARTERS ");  src.putssx(word_starters);  src.put("\r\n"); }
    if (word_insiders)
      { src.put("WORD_CHARS ");     src.putssx(word_insiders);  src.put("\r\n"); }
  }

  if (!altering) src.putc('(');
  bool first=true;
  for (int i=0;  i<items.count();  i++)
  { t_autoindex * ai = items.acc0(i);
    if (altering ? ai->tag==ATR_OLD : ai->tag==ATR_DEL) continue;
    if (!*ai->doctable_name || !*ai->id_column || !*ai->text_expr) continue;
    if (first) first=false;
    else src.put(",\r\n ");  
    if (altering)
      src.put(ai->tag==ATR_DEL ? "DROP " : "ADD  ");
    src.putq(ai->doctable_name);  src.putc(' ');
    char basename[ATTRNAMELEN+1];
    base_column_name(ai->id_column, basename);
    src.putq(basename);  src.putc(' ');
    src.putq(ai->text_expr);  
    if (ai->tag!=ATR_DEL)
    { if (ai->mode) 
        { src.putc(',');  src.putint2(ai->mode); }
      if (*ai->format && stricmp(ai->format, "AUTO"))
        { src.putc(',');  src.putssx(ai->format); }
    }
  }
  src.put(altering ? "\r\n" : ")\r\n");
  if (src.error()) return NULL; 
  return src.unbind();
}

