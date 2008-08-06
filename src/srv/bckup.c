/************************ journal replaying service *************************/
#ifdef WINS
#include <io.h>
#endif

static tptr kernel_oper(cdp_t cdp, tptr request)
{ uns8 opc;  ttablenum tbnum;
  trecnum recnum;  tattrib attr;  
  uns8 opc_type;  table_descr * tbdf, * locked_tabdescr;

  t_vector_descr default_deleted_vector(&default_colval);
  default_deleted_vector.delete_record=true;

  cdp->request_init();
  locked_tabdescr=NULL;
  { opc=*(request++);
   /* decoding on request types:
      0 - nothing to do
      1 - fetch cursnum, set crnm, cd & std, check table or cursor number, install table
      2 - fetch record number, translate it for cursor, error if deleted
      3 - fetch attribute, translate it for cursor, re-set crnm, recnum
      4 - go through pointers
   */
    opc_type=opc_types[opc];
    if (opc_type)  /* take the table/cursor number, set cursnum, crnm, std */
 	  { tbnum = *(ttablenum*)request;  request+=sizeof(ttablenum);
      if (IS_CURSOR_NUM(tbnum) || tbnum<0)   /* temporary table (or cursor?) */
        { request_error(cdp, IS_DELETED);  goto err_goto1; }
      tbdf=locked_tabdescr=install_table(cdp, tbnum);
      if (!tbdf) goto err_goto1;  /* error inside */
      if (opc_type > 1)  /* take the record number, translate it */
      { recnum=*(trecnum*)request;  request+=sizeof(trecnum);
        if (opc_type > 2)
        { attr=*(request++);
          if (attr>=tbdf->attrcnt)
            { request_error(cdp, BAD_ELEM_NUM);  goto err_goto; }
        }
      }
    }

    switch (opc)
    {
    case OP_DELETE: /*2*/
      tb_del(cdp, tbdf, recnum);       break;
    case OP_UNDEL: /*2 undelete allowed only for tables, not for cursors */
      tb_undel(cdp, tbdf, recnum);     break;
    case OP_APPEND:
      tb_new(cdp, tbdf, TRUE, NULL);    break;
    case OP_INSERT:
      tb_new(cdp, tbdf, FALSE, NULL);   break;
    case OP_INSDEL:
      tb_new(cdp, tbdf, TRUE, &default_deleted_vector);    break;

    case OP_WRITE: /*3*/
    { uns16 index;  uns32 varstart, varsize;  
      if (*request==MODPTR || *request==MODINDPTR)
        if (ptrsteps(cdp, &tbdf, &tbnum, &recnum, &attr, &request)) break;
     /* checking access rights */
      if (SYSTEM_TABLE(tbnum)) /* object rights */
      { if (tbnum==TAB_TABLENUM && attr==OBJ_DEF_ATR)  /* tabdef changed */
        /* to se doufam nikdy nedeje */
          if (recnum < tabtab_descr->Recnum())
            force_uninst_table(cdp, (ttablenum)recnum);
      }
     // [attr] is supposed not to be DEL_ATTR_NUM and modifiers are supposed to be OK.
      attribdef * att = tbdf->attrs + attr;
      if (*request==MODSTOP)
      { request++;
        if (tb_write_atr(cdp, tbdf, recnum, attr, request)) break;
        request+=TYPESIZE(att);
      }
      else if (*request==MODIND)
      { request++;
        index=*(uns16*)request;  request+=sizeof(uns16);
        if (*request==MODSTOP)
        { request++;
          if (tb_write_ind(cdp, tbdf, recnum, attr, index, request)) break;
          request+=TYPESIZE(att);
        }
        else if (*request==MODINT)
        { request++;
          varstart=*(uns32*)request;  request+=sizeof(uns32);
          varsize =*(uns32*)request;  request+=sizeof(uns32);
          request++;   /* MOD_STOP not checked */
          if (tb_write_ind_var(cdp, tbdf, recnum, attr, index, varstart, varsize, request)) break;
          request+=varsize;
        }
        else if (*request==MODLEN)
        { request+=2;   /* MOD_STOP not checked */
          varstart=*(uns32*)request;  request+=sizeof(uns32);
          if (tb_write_ind_len(cdp, tbdf, recnum, attr, index, varstart)) break;
        }
        else { request_error(cdp,BAD_MODIF);  break; }
      }
      else if (*request==MODINT)
      { request++;
        varstart=*(uns32*)request;  request+=sizeof(uns32);
        varsize =*(uns32*)request;  request+=sizeof(uns32);
        request++;   /* MOD_STOP not checked */
        if (tb_write_var(cdp, tbdf, recnum, attr, varstart, varsize, request)) break;
        request+=varsize;
      }
      else if (*request==MODLEN)
      { request+=2;   /* MOD_STOP not checked */
        if (IS_HEAP_TYPE(tbdf->attrs[attr].attrtype))
        { varstart=*(uns32*)request;  request+=sizeof(uns32);
          if (tb_write_atr_len(cdp, tbdf, recnum, attr, varstart)) break;
        }
        else
        { index=*(t_mult_size*)request;  request+=sizeof(t_mult_size);
          if (tb_write_ind_num(cdp, tbdf, recnum, attr, index)) break;
        }
      }
      else { request_error(cdp,BAD_MODIF);  break; }
      break;
    }
    case OP_SUBMIT:    /*0*/
    { sql_statement * so, * so1;
     // restore the appl name: used only by the function returning it:
      strmaxcpy(cdp->sel_appl_name, request, sizeof(tobjname));  request+=sizeof(tobjname);
     // restore sel_appl_uuid: CREATE TABLE etc. need this! 
      memcpy(cdp->sel_appl_uuid, request, UUID_SIZE);  request+=UUID_SIZE;
      unsigned len = (unsigned)strlen(request)+1;  /* compilation may add 0 into statement */
     // dynamic parameters not allowed in statements in the journal, direct compile
      so=sql_submitted_comp(cdp, request);
     /* execute only selected statements: */
      so1 = so;
      while (so1 != NULL)  /* NULL if error or empty statement */
      { if (so1->statement_type==SQL_STAT_ALTER_TABLE  ||
            so1->statement_type==SQL_STAT_CREATE_TABLE ||
            so1->statement_type==SQL_STAT_DROP         ||
            so1->statement_type==SQL_STAT_CREATE_INDEX ||
            so1->statement_type==SQL_STAT_DROP_INDEX  )
        { if (so1->exec(cdp)) break;
          cdp->ker_flags=KFL_ALLOW_TRACK_WRITE | KFL_NO_JOUR | KFL_DISABLE_REFINT;
        }
        so1=so1->next_statement;  
      }
      delete so;
      request+=len;
      break;
    }

    default:
      request_error(cdp,BAD_OPCODE);  break;
    }  /* end of switch */

   err_goto:  /* exiting the request processing */
    if (locked_tabdescr!=NULL) { unlock_tabdescr(locked_tabdescr);  locked_tabdescr=NULL; } /* flags may be set by OP_INDEX too */
   err_goto1:   /* tabinfo not locked yet */
    if (cdp->break_request) request_error(cdp,REQUEST_BREAKED);
    if (cdp->is_an_error())
    { if (cdp->nonempty_transaction()) roll_back(cdp);  /* error roll back only on some operations */
		  request=NULL;
	  }
  }
#ifdef FIX_CHECKS
  unfix_all();  /* unfix all pages forgoten to be unfixed */
#endif
  cdp->appl_state=WAITING_FOR_REQ;
  return request;
}

#define JBUFSIZE  0xfff0
static uns32 bufpos;
static unsigned bufcont;
static void * jbhandle;
static tptr jb;

static BOOL setpos(cdp_t cdp, uns32 pos)
{ if (bufpos!=pos)
  { if ((bufpos < pos) && (pos < bufpos+bufcont))
    { memmov(jb, jb+(uns16)(pos-bufpos), bufcont-(uns16)(pos-bufpos));
      bufcont-=(uns16)(pos-bufpos);
    }
    else bufcont=0;
    bufpos=pos;
  }
  if (bufcont < JBUFSIZE)
  { DWORD rd;  
    if (!ReadFile(ffa.j_handle, jb+bufcont, JBUFSIZE-bufcont, &rd, NULL))
      return FALSE;
	  bufcont+=rd;
  }
  return TRUE;
}

static BOOL remake_action(cdp_t cdp, uns32 pos)
{ tptr newpos;
  while (TRUE)
  { if (!setpos(cdp, pos)) return FALSE;
    if (*jb==OP_END) break;
    the_sp.WriteJournal.string_to_mem("0", 0);  // disables writing to the journal, without changing the persistent property value
    newpos=kernel_oper(cdp, jb);
    cdp->answer.free_answer();
    if (!newpos) return FALSE;   /* cannot continue, pos is bad */
    pos+=(uns16)(newpos-jb);
  }
  commit(cdp);
  return TRUE;
}

int replay(cdp_t cdp, tobjnum bad_autor, timestamp lim_dt)
/* Replays journal form >header.closetime till lim_dt or to its end.
   Journal must be open before. Returns 3 long counts + long result. */
{ uns32 jpos, partsize;  DWORD rd;
  jourheader jh;  int res=0;
  uns32 nop_count=0, err_count=0, oper_count=0, * pa;
  if (!FILE_HANDLE_VALID(ffa.j_handle)) return REPLAY_NO_JOURNAL;
  jbhandle=aligned_alloc(JBUFSIZE, &jb);
  if (!jbhandle) { res=REPLAY_NO_MEMORY;  goto wr_answer; }
 /* find the journal position fot the closetime */
  SetFilePointer(ffa.j_handle, 0, NULL, FILE_BEGIN);   /* to the journal start */
  do
  { if (!ReadFile(ffa.j_handle, &jh, sizeof(jourheader), &rd, NULL) || rd!=sizeof(jourheader))
      { res=REPLAY_NO_VALID_RECORD;  goto to_exit; }
    if ((jh.spec_size >> 24) != JOURMARK)
      { res=REPLAY_BAD_JOURNAL;  goto to_exit; }
    if (jh.timestamp > header.closetime) break;   /* valid record found! */
    partsize=jh.spec_size & 0xffffffL;
    SetFilePointer(ffa.j_handle, partsize-sizeof(jourheader), NULL, FILE_CURRENT);
  } while (TRUE);

 /* replaying */
  bufcont=0;  bufpos=0;
  jpos=SetFilePointer(ffa.j_handle, 0, NULL, FILE_CURRENT);  // GetFilePointer
  if (jpos==(uns32)-1)
    { res=REPLAY_BAD_JOURNAL;  goto to_exit; }
  jpos-=sizeof(jourheader);
  SetFilePointer(ffa.j_handle, jpos, NULL, FILE_BEGIN);
  cdp->ker_flags=KFL_ALLOW_TRACK_WRITE | KFL_NO_JOUR | KFL_DISABLE_REFINT;
  the_sp.WriteJournal.string_to_mem("0", 0);  // disables writing to the journal, without changing the persistent property value
  do
  { setpos(cdp, jpos);
    if (bufcont <= sizeof(jourheader)) break;  /* end of journal contents */
    jh=*(jourheader*)jb;
   /* now, the operation to be replayed is on pos in the journal chain achn */
    if ((jh.spec_size >> 24) != JOURMARK)
      { res=REPLAY_BAD_JOURNAL;  break; }
    if (jh.timestamp>=lim_dt)   /* limit reached */
      { res=REPLAY_LIMIT_REACHED;  break; }
    if (jh.usernum==bad_autor) nop_count++;
    else
    { cdp->prvs.set_user(jh.usernum, CATEG_USER, FALSE);  /* in order to know the owner of new objects */
      if (!remake_action(cdp, jpos+sizeof(jourheader)))
        err_count++;
      oper_count++;
      header.closetime=jh.timestamp;
    }
    report_step_sum(cdp, oper_count, err_count);
   /* read the new journal record */
    partsize=jh.spec_size & 0xffffffL;
    jpos+=partsize;
  } while (TRUE);
  ffa.write_header();   /* record the new close time */
  report_total(cdp, oper_count, nop_count);

 to_exit:
  SetFilePointer(ffa.j_handle, 0, NULL, FILE_END);   /* back to the journal end */
  aligned_free(jbhandle);
 wr_answer:
  cdp->answer.ans_init();
  pa=(uns32 *)cdp->get_ans_ptr(4*sizeof(uns32));
  pa[0]=oper_count;  pa[1]=nop_count;  pa[2]=err_count;  pa[3]=res;
  cdp->set_return_code(ANS_OK);
  cdp->ker_flags=0;
  *cdp->sel_appl_name=0;
 // must restore the journalling property:
  char buf[255+1];
  if (the_sp.WriteJournal.load_to_memory(cdp, &the_sp, "@SQLSERVER", 0, buf))
    the_sp.WriteJournal.string_to_mem(buf, 0);
  else
    the_sp.WriteJournal.set_default();
  return res;
}


