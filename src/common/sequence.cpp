//////////////////////////////////////// sequence edit ///////////////////////////////////////////////////
// Jmeno schematu: da se pouzit v SQL prikazech CREATE i ALTER, ale neuschovava se na serveru.
//   (Pokud je jmeno schematu uschovano na serveru, musi se ignorovat.)
// Bezna hodnota: je odlisna od startovni hodnoty. 
//   Je v komentari na zacatku definice, za descriptorem.
//   Pri ALTER se na serveru zachova existujici, pro create se prevezme startovni.
//   Zmena: Pri ALTER se bere startovni, pokud je uvedena! Take se bere startovni v prazdnem ALTER!
// GUI: pri analyze existujici sekvence se z ni precte aktualni hodnota, jmeno a vsechny vlastnosti
//      na klientske strane se nikdy rekurzivne nevola, prestoze je nastaveno modifying
// Server: v doslem SQL prikazu se ignoruji komentare.....
//   Pri analyze ALTER se napred struktura naplni z definice a pak se modifikuje, pritom se nemeni jmeno schematu
// Folder name: in ALTER the original folder preserved (analysed after the folder specified in ALTER)
//   Pro CREATE se zaslany folder ztrati, nutno dopsat do definice.

t_seq_value signed_num_val(CIp_t CI) 
// Number or sign is the next symbol on entry, the number is the current symbol on exit.
{ bool minus;
  if (next_sym(CI)=='-') { minus=true;  next_sym(CI); }
  else 
  { minus=false;
    if (CI->cursym=='+') next_sym(CI); 
  }
  if (CI->cursym!=S_INTEGER) c_error(INTEGER_EXPECTED);
  return minus ? -CI->curr_int : CI->curr_int;
}

bool compile_sequence(cdp_t cdp, tptr defin, t_sequence * seq);

void WINAPI sequence_anal(CIp_t CI) 
// SEQUENCE is the current symbol, univ_ptr contains t_sequence *, "modifying" defined in it.
// Supposes that the default value of seq->startval=0 is set outside.
{ t_sequence * seq = (t_sequence*)CI->univ_ptr;
  if (CI->cursym==S_CREATE) next_sym(CI);
  else if (CI->cursym==S_ALTER) { next_sym(CI);  seq->modifying=true; }
  
 // sequence name: 
  next_and_test(CI, S_IDENT, IDENT_EXPECTED);
  strmaxcpy(seq->name, CI->curr_name, sizeof(tobjname));
  if (next_sym(CI)=='.')
  { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
    if (!seq->recursive) // must not overwrite schema name in the recursive call (schema name not stored on server, but if stored, it must be ignored)
      strcpy(seq->schema, seq->name);
    strmaxcpy(seq->name, CI->curr_name, sizeof(tobjname));
    next_sym(CI);
  }
  // when schema is not specified, the recursive compilation must not overwrite the schema from the ALTER statement!
#ifdef SRVR
 // load existing SEQUENCE, if altering:
  if (seq->modifying && !seq->recursive)
  { if (name_find2_obj(CI->cdp, seq->name, seq->schema, CATEG_SEQ, &seq->objnum))
      { SET_ERR_MSG(CI->cdp, seq->name);  c_error(OBJECT_NOT_FOUND); }
    tptr defin=ker_load_objdef(CI->cdp, objtab_descr, seq->objnum);
    if (!defin) c_error(OUT_OF_MEMORY);
    seq->recursive=true;
    bool res=compile_sequence(CI->cdp, defin, seq);
    seq->recursive=false;
    corefree(defin);
    if (!res) c_error(CI->cdp->get_return_code()); // propagates the error
  }
  else
#endif
 // set default values, if creating:
  { seq->step=1;  
    seq->cache=20;
    seq->hasmax=seq->hasmin=seq->cycles=false;  seq->hascache=seq->ordered=true;
    seq->start_specified=false;
  }
 // sequence options:
  seq->anything_specified=false;
  seq->restart_specified=false;
  do
  { if (CI->cursym==S_IDENT)
    { seq->anything_specified=true;
      if      (!strcmp(CI->curr_name, "INCREMENT"))
      { next_and_test(CI, S_BY, BY_EXPECTED);
        seq->step=signed_num_val(CI);
        if (!seq->step) c_error(BAD_SEQUENCE_PARAMS);
      }
      else if (!strcmp(CI->curr_name, "MAXVALUE"))
        { seq->maxval=signed_num_val(CI);  seq->hasmax=true;  seq->anything_specified=true; }
      else if (!strcmp(CI->curr_name, "MINVALUE"))
        { seq->minval=signed_num_val(CI);  seq->hasmin=true;  seq->anything_specified=true; }
      else if (!strcmp(CI->curr_name, "CACHE"))
      { next_and_test(CI, S_INTEGER, INTEGER_EXPECTED);
        if (CI->curr_int<2 || CI->curr_int>1000) c_error(INT_OUT_OF_BOUND);
        seq->cache=(int)CI->curr_int;  seq->hascache=true;  seq->anything_specified=true;
      }
      else if (!strcmp(CI->curr_name, "NOMAXVALUE")) { seq->hasmax  =false;  seq->anything_specified=true; }
      else if (!strcmp(CI->curr_name, "NOMINVALUE")) { seq->hasmin  =false;  seq->anything_specified=true; }
      else if (!strcmp(CI->curr_name, "NOCACHE"))    { seq->hascache=false;  seq->anything_specified=true; }
      else if (!strcmp(CI->curr_name, "CYCLE"))      { seq->cycles  =true;   seq->anything_specified=true; }
      else if (!strcmp(CI->curr_name, "NOCYCLE"))    { seq->cycles  =false;  seq->anything_specified=true; }
      else if (!strcmp(CI->curr_name, "NORODER"))    { seq->ordered =false;  seq->anything_specified=true; }
      else if (!strcmp(CI->curr_name, "RESTART") && seq->modifying)
      { next_and_test(CI, SQ_WITH, WITH_EXPECTED);
        seq->restartval=signed_num_val(CI);
        seq->restart_specified=true;
        seq->anything_specified=true;
      }
      else break;
    }
    else if (CI->cursym==SQ_START)  // extension: allowed in the ALTER statement, too
    { next_and_test(CI, SQ_WITH, WITH_EXPECTED);
      seq->startval=signed_num_val(CI);
      seq->start_specified=true;
      seq->anything_specified=true;
    }
    else if (CI->cursym==S_ORDER)
    { seq->ordered=true;
      seq->anything_specified=true;
    }
    else if (CI->cursym==S_AS && !seq->modifying)  // for compatibility with the SQL 2003 standard, type not stored and not used
    { next_sym(CI);
      int tp;  t_specif specif;
      analyse_type(CI, tp, specif, NULL, 1);
      if (!(tp==ATT_INT32 || tp==ATT_INT16 || tp==ATT_INT8 || tp==ATT_INT64) || specif.scale!=0)  // NUMERIC and DECIMAL types are converted to these types
        c_error(MUST_BE_INTEGER);
      continue;  // skips the next_sym(CI) below
    }
    else break;
    next_sym(CI);
  } while (true);
 // compatibility with version 8: if ALTER contains START but does not contain RESTART, use the start value as the restart value, too.
  if (seq->modifying && !seq->recursive)
    if (seq->start_specified && !seq->restart_specified)
    { seq->restartval=seq->startval;
      seq->restart_specified=true;
    }
 // integrity checks on specified values:
  if (seq->hasmin            && seq->hasmax && seq->maxval    <= seq->minval) c_error(BAD_SEQUENCE_PARAMS);
  if (seq->start_specified   && seq->hasmax && seq->maxval   < seq->startval) c_error(BAD_SEQUENCE_PARAMS);
  if (seq->start_specified   && seq->hasmin && seq->startval   < seq->minval) c_error(BAD_SEQUENCE_PARAMS);
  if (seq->restart_specified && seq->hasmax && seq->maxval < seq->restartval) c_error(BAD_SEQUENCE_PARAMS);
  if (seq->restart_specified && seq->hasmin && seq->restartval < seq->minval) c_error(BAD_SEQUENCE_PARAMS);
 // derived defaults:
  if (!seq->hasmin) 
  { seq->minval = seq->step>0 ? 1 : -MAXBIGINT;
    if (seq->start_specified && seq->startval < seq->minval) seq->minval=seq->startval;
  }
  if (!seq->hasmax) 
  { seq->maxval = seq->step>0 ? MAXBIGINT : -1;
    if (seq->start_specified && seq->startval > seq->maxval) seq->maxval=seq->startval;
  }
  if (!seq->start_specified)
    seq->startval = seq->step>0 ? seq->minval : seq->maxval;
 // value used on the server:
  if (!seq->hascache) seq->cache=1;
}

bool compile_sequence(cdp_t cdp, tptr defin, t_sequence * seq)
{ seq->currval=0;  
  if (*defin=='{')  // read the current value
  { bool minus=false, digit=false;  t_seq_value val=0;
    tptr p = defin+1;
    while (*p==' ') p++;
    if (*p=='-') { p++;  minus=true; }
    while (*p>='0' && *p<='9')
      { val=10*val+*p-'0';  p++;  digit=true; }
    if (minus) val=-val;
    if (digit) seq->currval=val;
  }
#ifdef SRVR
  compil_info xCI(cdp, defin, sequence_anal);
#else
  compil_info xCI(cdp, defin, DUMMY_OUT, sequence_anal);
  set_compiler_keywords(&xCI, NULL, 0, 1); /* SQL keywords selected */
#endif
  xCI.univ_ptr=seq;
  return !compile(&xCI);
}

#define SEQ_VALUE_SPACE 30

tptr sequence_to_source(cdp_t cdp, t_sequence * seq, bool altering)
{ t_flstr src(500,500);
#ifdef SRVR // this cannot change the current value in the ATRER SEQUENCE statement
  { char buf[SEQ_VALUE_SPACE+1];  
    int64tostr(seq->currval, buf);  int len=(int)strlen(buf);
    memset(buf+len, ' ', SEQ_VALUE_SPACE-len);  buf[SEQ_VALUE_SPACE]=0;
    src.putc('{');  src.put(buf);  src.put("}\r\n");
  }
#else
  src.putc(' ');  // space for testing-mark
#endif
  src.put(altering ? "ALTER" : "CREATE");  src.put(" SEQUENCE `");
#ifndef SRVR
 // schema name added only when creating the SQL statement from GUI
  if (*seq->schema) { src.put(seq->schema);  src.put("`.`"); }
#endif
  src.put(seq->name);  src.put("`\r\n");
 // options:
  src.put("START WITH ");    src.putint64(seq->startval);  src.put("\r\n");
  src.put("INCREMENT BY ");  src.putint64(seq->step);      src.put("\r\n"); // persistent starting value
  if (seq->hasmax)   { src.put("MAXVALUE ");  src.putint64(seq->maxval); } else src.put("NOMAXVALUE");
  src.put("\r\n");
  if (seq->hasmin)   { src.put("MINVALUE ");  src.putint64(seq->minval); } else src.put("NOMINVALUE");
  src.put("\r\n");
  if (seq->hascache) { src.put("CACHE ");     src.putint(seq->cache);  } else src.put("NOCACHE");
  src.put("\r\n");
  src.put(seq->cycles  ? "CYCLE" : "NOCYCLE");  src.put("\r\n");
  src.put(seq->ordered ? "ORDER" : "NOORDER");  src.put("\r\n");
  if (src.error()) return NULL; 
  return src.unbind();
}

