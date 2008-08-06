// dataproc.cpp - view data access                         
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#ifndef WINS
#include "winrepl.h"
#endif
#include "cint.h"
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop
#include "wprez9.h"
#include "comp.h"
#include "datagrid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#ifdef LINUX
#if WBVERS<1000
#define SQLExecDirectA SQLExecDirect // A prevents translating to W in Linux 10 but is not defined in Linux 9 
#define SQLErrorA SQLError  
#define SQLGetCursorNameA SQLGetCursorName
#define SQLPrepareA SQLPrepare
#endif
#endif


tptr item_text(t_ctrl * itm)
{ tptr tx = get_var_value(&itm->ctCodeSize, CT_NAMEA);
  if (tx!=NULL) return tx;
  return get_var_value(&itm->ctCodeSize, CT_NAMEV);
}

#ifdef CLIENT_ODBC_9

void odbc_signalize(t_connection * conn)
{ wchar_t buf[500];
  if (Get_error_num_textW(conn, GENERR_ODBC_DRIVER, buf, sizeof(buf)/sizeof(wchar_t)))
    error_box(buf);
}

void odbc_stmt_error(t_connection * conn, HSTMT hStmt)  // like cd_Signalize for ODBC
{ 
  if (odbc_error(conn, hStmt))
    odbc_signalize(conn);
}

BOOL odbc_reset_cursor(view_dyn * inst)
{ return odbc_reset_cursors(inst, inst->dt->select_st);}

BOOL odbc_reset_cursors(view_dyn * inst, char *stm)
{ RETCODE retcode;
  SQLFreeStmt(inst->dt->hStmt, SQL_CLOSE);
#if defined(WINS) //|| (WBVERS>=1000)
  { wxString sstm(stm, *wxConvCurrent);
    retcode=SQLExecDirectW(inst->dt->hStmt, (SQLWCHAR*)sstm.c_str(), SQL_NTS);
    if (retcode==1 && !stricmp(inst->dt->conn->dbms_name, "Oracle"))
      retcode=SQLExecDirectW(inst->dt->hStmt, (SQLWCHAR*)sstm.c_str(), SQL_NTS);
  }
#else  
  retcode=SQLExecDirectA(inst->dt->hStmt, (UCHAR*)stm, SQL_NTS);
#endif  
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    { odbc_stmt_error(inst->dt->conn, inst->dt->hStmt);  return FALSE; }
  if (inst->dt->supercurs!=NOOBJECT)
  { SQLFreeStmt((HSTMT)(size_t)inst->dt->supercurs, SQL_CLOSE);
#if defined(WINS) //|| (WBVERS>=1000)
    retcode=SQLExecDirectW((HSTMT)(size_t)inst->dt->supercurs, (SQLWCHAR*)wxString(inst->upper_select_st, *wxConvCurrent).c_str(), SQL_NTS);
#else
    retcode=SQLExecDirectA((HSTMT)(size_t)inst->dt->supercurs, (UCHAR*)inst->upper_select_st, SQL_NTS);
#endif    
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
      { odbc_stmt_error(inst->dt->conn, (HSTMT)(size_t)inst->dt->supercurs);  return FALSE; }
  }
  return TRUE;
}

#define MAX_COMMAND_LEN  5000

SWORD catr_to_c_type(atr_info * catr)
{ if (catr->flags & CA_INDIRECT)
    return catr->type==ATT_RASTER || catr->type==ATT_NOSPEC || catr->type==ATT_SIGNAT
             ? SQL_C_BINARY : catr->specif.wide_char ? SQL_C_WCHAR : SQL_C_CHAR;
  switch (catr->type)
  { case ATT_INT8:    return SQL_C_TINYINT;
    case ATT_INT16:   return SQL_C_SHORT;  /* must use ODBC 1.0 types due to old drivers */
    case ATT_INT32:   return SQL_C_LONG;   /* must use ODBC 1.0 types due to old drivers */
    case ATT_INT64:   return SQL_C_SBIGINT;
    case ATT_FLOAT:   return SQL_C_DOUBLE;
    case ATT_CHAR:  case ATT_STRING:
                      return catr->specif.wide_char ? SQL_C_WCHAR : SQL_C_CHAR;
    case ATT_BINARY:  return SQL_C_BINARY;
    case ATT_BOOLEAN: return SQL_C_BIT;
    case ATT_ODBC_NUMERIC:  case ATT_ODBC_DECIMAL:  return SQL_C_CHAR;
    case ATT_ODBC_DATE:       return SQL_C_DATE;
    case ATT_ODBC_TIME:       return SQL_C_TIME;
    case ATT_ODBC_TIMESTAMP:  return SQL_C_TIMESTAMP;
  }
  return 0;
}


BOOL delete_odbc_record(view_dyn * inst, trecnum extrec, BOOL on_current)
{ unsigned i;  atr_info * catr;  RETCODE retcode;  UINT parnum;
  BOOL firstattr;
  if (inst->sel_rec==NORECNUM) return TRUE;
  if (inst->dt->edt_rec != NORECNUM) cache_roll_back(inst);  /* reload identifying values */
  t_connection * conn = inst->dt->conn;
  if (inst->sel_rec <  inst->dt->cache_top ||
      inst->sel_rec >= inst->dt->cache_top+inst->dt->cache_reccnt) return TRUE;
  tptr old_vals = (tptr)(inst->dt->cache_ptr +
                  inst->dt->rec_cache_size * (unsigned)(inst->sel_rec-inst->dt->cache_top));
 /* create the command: */
  char quote_char;
  if (conn!=NULL) quote_char=conn->identifier_quote_char;
  else quote_char=' ';
  tptr comm=(tptr)corealloc(MAX_COMMAND_LEN, 88);
  if (comm==NULL) return TRUE;
  sprintf(comm, "DELETE FROM %s WHERE ", inst->dt->fulltablename);
  firstattr=TRUE;
  if (on_current)
  { strcat(comm, "CURRENT OF ");
    SQLGetCursorNameA(inst->dt->hStmt, (UCHAR*)comm+strlen(comm), 40, NULL);
  }
  else
  { for (i=1, catr=inst->dt->attrs+1;  i<inst->dt->attrcnt;  i++, catr++)
      if (catr->flags & CA_ROWID && !catr_is_null(old_vals, catr))
      { if (firstattr) firstattr=FALSE; else strcat(comm, " AND ");
        cpy_full_attr_name(quote_char, comm+strlen(comm), inst->dt->co_owned_td, i);
        strcat(comm, catr_is_null(old_vals, catr) ? " IS NULL" : "=?");
      }
    if (firstattr)
    { error_box(_("Cannot edit this table"));
      return TRUE;
    }
  }
 /* position: */
  if (on_current)
  { odbc_rec_status(inst->cdp, inst->dt, extrec); /* ExtFetch */
    retcode=SQLSetPos(inst->dt->hStmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
      { odbc_stmt_error(inst->dt->conn, inst->dt->hStmt);  return TRUE; }
  }
 /* prepare: */
#if defined(WINS) //|| (WBVERS>=1000)
  retcode=SQLPrepareW(conn->hStmt, (SQLWCHAR*)wxString(comm, *wxConvCurrent).c_str(), SQL_NTS);
#else  
  retcode=SQLPrepare(conn->hStmt, (SQLWCHAR*)comm, SQL_NTS);  // ANSI version
  // in version 10 #defined as SQLPrepareW in sqlucode.h, big problem! ### (SQLPrepare is missing in the driver manager)
#endif  
  corefree(comm);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    { odbc_stmt_error(conn, conn->hStmt);  return TRUE; }
 /* bind the new values and the identifying values: */
  parnum=1;
  if (!on_current)
  { SQLLEN * pcbValue=(SQLLEN*)(old_vals+inst->dt->rr_datasize);
    for (i=1, catr=inst->dt->attrs+1;  i<inst->dt->attrcnt;  i++, catr++)
    { if (catr->flags & CA_ROWID && !catr_is_null(old_vals, catr))
      {
        if (SQLBindParameter(conn->hStmt, (UWORD)parnum++, SQL_PARAM_INPUT,
          catr_to_c_type(catr),  catr->odbc_sqltype,
          catr->odbc_precision, catr->odbc_scale,
          old_vals+catr->offset, 0, pcbValue) != SQL_SUCCESS)
            odbc_stmt_error(conn, conn->hStmt);
      }
      if (!(catr->flags & (CA_INDIRECT | CA_MULTIATTR))) pcbValue++;
    }
  }
 /* execute: */
  if (conn->can_transact)
    retcode=SQLSetConnectOption(conn->hDbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);
  retcode=SQLExecute(conn->hStmt);
  if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
  { SQLLEN rowcnt=0;
    SQLRowCount(conn->hStmt, &rowcnt);
    if (conn->can_transact)
      SQLTransact(SQL_NULL_HENV, conn->hDbc, rowcnt==1 ? SQL_COMMIT : SQL_ROLLBACK);
  }
  else odbc_stmt_error(conn, conn->hStmt);
  if (conn->can_transact)
    SQLSetConnectOption(conn->hDbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_ON);
  SQLFreeStmt(conn->hStmt, SQL_RESET_PARAMS);
  return retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO;
}

CFNC DllPrezen BOOL WINAPI setpos_delete_record(cdp_t cdp, ltable * dt, trecnum extrec)
{ RETCODE retcode;
 /* position: */
  odbc_rec_status(cdp, dt, extrec); /* ExtFetch */
  retcode=SQLSetPos(dt->hStmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    { odbc_stmt_error(dt->conn, dt->hStmt);  return TRUE; }
 /* execute: */
  retcode=SQLSetPos(dt->hStmt, 1, SQL_DELETE, SQL_LOCK_NO_CHANGE);
  if (retcode!=SQL_SUCCESS) odbc_stmt_error(dt->conn, dt->hStmt);
  return retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO;
}
#endif


#if 0
// from the original remap_reset
 // DeleteRows must be after decreasing rm_int_recnum, otherwise records are re-inserted immediately
 // must not delete more records than is the number of valid rows in the grid
  int maxrows = ((DataGrid*)inst->hWnd)->GetNumberRows();
  if (maxrows && !(inst->stat->vwHdFt & NO_VIEW_FIC)) maxrows--;
  if (rows>maxrows) rows=maxrows;
  if (inst->hWnd && ldt->remap_a) 
    ((DataGrid*)inst->hWnd)->DeleteRows(0, rows);  // not removing the fictive row, if any, because it will not be added in remap_expand
  cache_reset(inst);
#endif

trecnum view_recnum(view_dyn * inst)
/* Returns number of records in a view (incl. fictive if any) or NORECNUM
   on error. */
{ trecnum recnum;
  ltable * ldt = inst->dt;
//  Set_status_text(IS_CURSOR_NUM(ldt->cursnum) ? MAKEINTRESOURCE(STBR_CURCON) : MAKEINTRESOURCE(STBR_RECCOUNT));
  if (ldt->remap_a)
  { while (!ldt->rm_completed)
      if (remap_expand(inst, 512)) break;
    SET_FI_INT_RECNUM(inst);
  }
  else if (!ldt->rm_completed && !(inst->stat->vwHdFt & FORM_OPERATION))
  { 
#ifdef CLIENT_ODBC_9
    if (!inst->cdp)
    { do
      { int status=odbc_rec_status(ldt->cdp, ldt, ldt->rm_ext_recnum);
        if (status!=SQL_ROW_SUCCESS && status!=SQL_ROW_DELETED) break;
        ldt->rm_ext_recnum++;
      } while (TRUE);
    }
    else
#endif    
    { if (ldt->cursnum == NOOBJECT || cd_Rec_cnt(inst->cdp, ldt->cursnum, &recnum))
        { wxGetApp().frame->SetStatusText(wxEmptyString);  return NORECNUM; }
      ldt->rm_ext_recnum=recnum;
    }
    ldt->rm_completed=true;
    SET_FI_EXT_RECNUM(inst);
  }
  wxGetApp().frame->SetStatusText(wxEmptyString);
  return ldt->fi_recnum;
}

void set_cache_top(ltable * dt, view_dyn * inst, trecnum new_top)
{ unsigned step, size;
  if (dt->cache_top==NORECNUM) /* cache empty */
  { dt->cache_top=new_top;
    cache_load(dt, inst, 0, dt->cache_reccnt, new_top);
  }
  else if (dt->cache_top < new_top &&
           dt->cache_top+dt->cache_reccnt > new_top)
  { step=(unsigned)(new_top-dt->cache_top);
    size=dt->cache_reccnt-step;  /* number of copied records */
    cache_free(dt, 0, step);
    memmove(dt->cache_ptr,
            dt->cache_ptr+dt->rec_cache_size*step,
                                dt->rec_cache_size*size);
    dt->cache_top=new_top;
    cache_load(dt, inst, size, step, new_top);
  }
  else if (dt->cache_top > new_top &&
           dt->cache_top < new_top+dt->cache_reccnt)
  { step=(unsigned)(dt->cache_top-new_top);
    size=dt->cache_reccnt-step;  /* number of copied records */
    cache_free(dt, size, step);
    memmove(dt->cache_ptr+dt->rec_cache_size*step,
            dt->cache_ptr,dt->rec_cache_size*size);
    dt->cache_top=new_top;
    cache_load(dt, inst, 0, step, new_top);
  }
  else if (dt->cache_top != new_top)
  { cache_free(dt, 0, dt->cache_reccnt);
    dt->cache_top=new_top;
    cache_load(dt, inst, 0, dt->cache_reccnt, new_top);
  }
}

BOOL cache_resize(view_dyn * inst, unsigned new_riw)
// Called when a view is resized or when a new view for the same cache is opened
{ BOOL ok=TRUE;
  if (new_riw==inst->recs_in_win) return TRUE;
  if (inst->dt->cursnum!=NOOBJECT) // cachable
  {
   // compute the new cache top
    unsigned new_cache_top = inst->toprec;
   // compute number of records in the cache (max of views using the cache):
    unsigned new_cache_recnum = inst->toprec-new_cache_top + new_riw;
   // synchronize if the edited record will be removed from the cache:
    if (inst->dt->edt_rec!=NORECNUM)
      if (inst->dt->edt_rec< new_cache_top ||
          inst->dt->edt_rec>=new_cache_top+new_cache_recnum)
        if (!cache_synchro(inst)) return FALSE;
   // set the new cache top:
    //if (new_cache_top < inst->dt->cache_top) -- pocet zaznamu v cache se mohl zmensit, proto nutno inkrementovat top
      set_cache_top(inst->dt, inst, new_cache_top);
   // resize the cache:
    if (inst->dt->cache_reccnt!=new_cache_recnum) // must change the cache
    { HGLOBAL new_hnd;
     // calculate the new record & cache sizes:
      UINT old_rec_sz=inst->dt->rec_cache_real;
      UINT new_rec_sz=inst->dt->rec_cache_real;
      BOOL diff=old_rec_sz != new_rec_sz;
     // free released part:
      if (diff)  // must free & reload all
        cache_free(inst->dt, 0, inst->dt->cache_reccnt);
      else if (new_cache_recnum < inst->dt->cache_reccnt)
        cache_free(inst->dt, new_cache_recnum, inst->dt->cache_reccnt-new_cache_recnum);
     // reallocate cache:
      if (inst->dt->cache_hnd)
      {// move the "old vals" now, if cache shrinking:
        if (new_cache_recnum < inst->dt->cache_reccnt)
          memcpy(inst->dt->cache_ptr + new_rec_sz * new_cache_recnum,
                 inst->dt->cache_ptr + old_rec_sz * inst->dt->cache_reccnt,
                 inst->dt->rec_cache_real);
        GlobalUnlock(inst->dt->cache_hnd);
        new_hnd=GlobalReAlloc(inst->dt->cache_hnd, new_rec_sz * (new_cache_recnum+1), GMEM_MOVEABLE);
      }
      else new_hnd=GlobalAlloc(GMEM_MOVEABLE, new_rec_sz * (new_cache_recnum+1));
      if (new_hnd)
      { tptr newptr = (tptr)GlobalLock(new_hnd);
       // move the "old vals" now, if cache growing:
        if (inst->dt->cache_hnd && new_cache_recnum>inst->dt->cache_reccnt)
          memcpy(newptr + new_rec_sz * new_cache_recnum,
                 newptr + old_rec_sz * inst->dt->cache_reccnt,
                 inst->dt->rec_cache_real);
        inst->dt->cache_hnd=new_hnd;  inst->dt->rec_cache_size=new_rec_sz;  inst->dt->cache_ptr=newptr;
      }
      else // error: use the original cache
      { new_cache_recnum=inst->dt->cache_reccnt;  ok=FALSE;
        inst->dt->cache_ptr=(tptr)GlobalLock(inst->dt->cache_hnd);
      }
     // load the rest of the cache:
      if (diff)
        cache_load(inst->dt, inst, 0, new_cache_recnum, inst->dt->cache_top);
      else if (new_cache_recnum > inst->dt->cache_reccnt)
        cache_load(inst->dt, inst, inst->dt->cache_reccnt, new_cache_recnum-inst->dt->cache_reccnt, inst->dt->cache_top);
      if (ok) inst->dt->cache_reccnt=new_cache_recnum;
    }
  }
 // update local view info:
  if (ok) inst->recs_in_win=new_riw; /* must be assigned even in non-cached variable views */
  if (inst->sel_rec!=NORECNUM && new_riw) /* not a column selected, not destroying the view */
    if (inst->sel_rec >= inst->toprec+new_riw)
    { inst->sel_rec  = inst->toprec+new_riw-1;
    }
  return ok;
}

void cache_reset(view_dyn * inst)
{ if (inst->dt->cache_ptr==NULL) return;
  cache_roll_back(inst);
  cache_free(inst->dt,       0, inst->dt->cache_reccnt);
  inst->dt->rm_completed=false;  /* remap reset implied */
  cache_load(inst->dt, inst, 0, inst->dt->cache_reccnt, inst->dt->cache_top);
}

#ifdef CLIENT_ODBC_9

static char nobuffer[1] = "";

CFNC DllPrezen BOOL WINAPI setpos_synchronize_view(cdp_t cdp, BOOL update,
  ltable * dt, tptr new_vals, trecnum extrec, BOOL positioned)
{ unsigned i;  atr_info * catr;  RETCODE retcode;
 /* write the pcbValues (important when a NULL value in cursor library cache should be replaced by non-NULL */
  SQLLEN * pcbValue = (SQLLEN*)(new_vals+dt->rr_datasize);
  for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
    if (!(catr->flags & (CA_INDIRECT | CA_MULTIATTR)))
    { if (!catr->changed) *pcbValue=SQL_IGNORE;
      else if (catr_is_null(new_vals, catr)) *pcbValue=SQL_NULL_DATA;
      else if (catr->type==ATT_ODBC_NUMERIC || catr->type==ATT_ODBC_DECIMAL)
        *pcbValue=(SQLLEN)strlen(new_vals+catr->offset);
      else if (catr->type==ATT_STRING || catr->type==ATT_TEXT)
        *pcbValue=(SQLLEN)(catr->specif.wide_char ? 2*wuclen((wuchar*)(new_vals+catr->offset)) : strlen(new_vals+catr->offset));
      else if (catr->type==ATT_BINARY) *pcbValue=catr->size;
      else *pcbValue=WB_type_length(catr->type);
      pcbValue++;
    }
 /* position: */
  if (update && !positioned)
  { odbc_rec_status(cdp, dt, extrec); /* ExtFetch */
    retcode=SQLSetPos(dt->hStmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
      { odbc_error(dt->conn, dt->hStmt);  return TRUE; }
  }
 /* allow updating the contents of the cursor library cache: (!) */
  memcpy(dt->odbc_buf, new_vals, dt->rec_cache_real);
 /* write AT_EXEC flags and tokens, create binding - necessary for MS SQL: */
  for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
    if ((catr->flags & (CA_INDIRECT|CA_MULTIATTR)) == CA_INDIRECT)
    { indir_obj * iobj = (indir_obj*)(dt->odbc_buf+catr->offset);
      uns32 thesize = iobj->actsize;
      iobj->actsize=catr->changed ? SQL_LEN_DATA_AT_EXEC((long)thesize) : SQL_IGNORE;
#if 1  // without this temporary binding the MS SQL ODBC driver cannot perform the update of CA_INDIRECT column when no bound column in updated the same momment
      if (iobj->obj)
        retcode=SQLBindCol(dt->hStmt, i, catr_to_c_type(catr), iobj->obj->data, thesize, &iobj->actsize);  
      else
        retcode=SQLBindCol(dt->hStmt, i, catr_to_c_type(catr), nobuffer, 0, &iobj->actsize);  
#endif
    }
 /* execute: */
  SQLLEN opt = 0;  // MS Native Driver error: 64-bit version stores 8 bytes for SQL_ATTR_CONCURRENCY!
  retcode=SQLGetStmtAttr(dt->hStmt, SQL_ATTR_CONCURRENCY, &opt, SQL_IS_UINTEGER, NULL);
  retcode=SQLSetPos(dt->hStmt, 1, update ? SQL_UPDATE : SQL_ADD, /*SQL_LOCK_UNLOCK*/ SQL_LOCK_NO_CHANGE);
  i=0;
  while (retcode==SQL_NEED_DATA)
  { PTR token;
    retcode=SQLParamData(dt->hStmt, &token);
    if (retcode==SQL_NEED_DATA)
    {// find the next column for which SQL_LEN_DATA_AT_EXEC has been specified above: 
      do i++;
      while ((dt->attrs[i].flags & (CA_INDIRECT|CA_MULTIATTR)) != CA_INDIRECT || !dt->attrs[i].changed);
      indir_obj * iobj = (indir_obj*)(new_vals+dt->attrs[i].offset);  // odbc_buf has overwritten sizes, must use new_vals! 
      SQLPutData(dt->hStmt, iobj->obj->data, iobj->actsize);
    }
  }
 // report errors (must be before SQlBindCol):
  if (retcode!=SQL_SUCCESS)  // SQL_SUCCESS_WITH_INFO reports "no rows updated" 
    odbc_error(dt->conn, dt->hStmt);
#if 1
 // unbind temp. var-len buffers:
  for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
    if ((catr->flags & (CA_INDIRECT|CA_MULTIATTR)) == CA_INDIRECT)
    { indir_obj * iobj = (indir_obj*)(dt->odbc_buf+catr->offset);
      iobj->actsize=10;  // anything, will be updated
      SQLBindCol(dt->hStmt, i, catr_to_c_type(catr), NULL, 0, &iobj->actsize);  
    }
#endif
  return retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO;
}
#endif

BOOL view_record_locking(view_dyn * inst, trecnum erec, BOOL read_lock, BOOL unlock)
{ BOOL res;
  if (erec==NORECNUM) return FALSE;  /* fictive rec, nothing to be done */
#ifdef CLIENT_ODBC_9
  if (!inst->cdp)
  { if (!inst->dt->conn->can_lock) return FALSE;  /* no action */
    if (unlock!=TRUE) odbc_rec_status(inst->cdp, inst->dt, erec); /* ExtFetch */
    RETCODE retcode=SQLSetPos(inst->dt->hStmt, 1, SQL_POSITION,
       unlock ? SQL_LOCK_UNLOCK : SQL_LOCK_EXCLUSIVE);
    res=retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO;
    if (!unlock && res) odbc_stmt_error(inst->dt->conn, inst->dt->hStmt);
  }
  else /* WB data */
#endif
  { if (unlock)
      if (read_lock) res=cd_Read_unlock_record (inst->cdp, inst->dt->cursnum, erec);
      else           res=cd_Write_unlock_record(inst->cdp, inst->dt->cursnum, erec);
    else
      if (read_lock) res=cd_Read_lock_record   (inst->cdp, inst->dt->cursnum, erec);
      else           res=cd_Write_lock_record  (inst->cdp, inst->dt->cursnum, erec);
    if (!unlock && res) stat_signalize(inst->cdp);
  }
  return res;
}

static int check_collision(view_dyn * inst, trecnum extrec)
/* called before writing the changed value */
/* Returns 0: can write,  1: can write, must unlock then,
           2: cannot write, not locked, 3: restore old val, nol locked
           4: cannot lock, ignore but do not unlock later
           */
{ int i;  atr_info * catr;
#ifdef CLIENT_ODBC_9
  if (!inst->cdp)   /* lock and reload */
  { odbc_rec_status(inst->cdp, inst->dt, extrec); /* ExtFetch */
    if (inst->dt->conn->can_lock)
    { RETCODE retcode=SQLSetPos(inst->dt->hStmt, 1, SQL_POSITION, SQL_LOCK_EXCLUSIVE);
      if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
        { odbc_error(inst->dt->conn, inst->dt->hStmt);  return 2; }
    }
   /* convert the NULL representation: */
    SQLLEN * pcbValue;
    pcbValue=(SQLLEN*)(inst->dt->odbc_buf+inst->dt->rr_datasize);
    for (i=1, catr=inst->dt->attrs+1;  i<inst->dt->attrcnt;  i++, catr++)
      if (!(catr->flags & (CA_INDIRECT|CA_MULTIATTR)))
        if (*(pcbValue++) == SQL_NULL_DATA || !(catr->flags & CA_RI_SELECT))
        // no record privils on ODBC tables
          catr_set_null(inst->dt->odbc_buf+catr->offset, catr->type);
  }
  else /* WB data */
#endif  
  { if (cd_Write_lock_record(inst->cdp, inst->dt->cursnum, extrec))
      if (cd_Write_lock_record(inst->cdp, inst->dt->cursnum, extrec))
        if (cd_Write_lock_record(inst->cdp, inst->dt->cursnum, extrec))
//          { stat_signalize();  return 2; }
          return 4;    // better if error is reported later (dialog appears)
    if (cd_Read_record(inst->cdp, inst->dt->cursnum, extrec, inst->dt->odbc_buf+1, inst->dt->rr_datasize-1))
      { stat_signalize(inst->cdp);  cd_Write_unlock_record(inst->cdp, inst->dt->cursnum, extrec);  return 2; }
  }
 /* compare values: */
  BOOL same=TRUE;
  tptr oldvals=(tptr)(inst->dt->cache_ptr+inst->dt->rec_cache_size*inst->dt->cache_reccnt);
  for (i=1, catr=inst->dt->attrs+1;  same && i<inst->dt->attrcnt;  i++, catr++)
  { if (catr->changed)  /* other attrs may have been changed by program */
    if (!(catr->flags & (CA_MULTIATTR | CA_INDIRECT)))
    if (is_string(catr->type) || catr->type==ATT_ODBC_NUMERIC || catr->type==ATT_ODBC_DECIMAL)
      same=!strcmp(inst->dt->odbc_buf+catr->offset, oldvals+catr->offset);
    else
      same=!memcmp(inst->dt->odbc_buf+catr->offset, oldvals+catr->offset, catr->size);
  }
  return !inst->cdp && !inst->dt->conn->can_lock ? 0:1;
}

CFNC DllPrezen BOOL WINAPI cache_synchro(view_dyn * inst)
/* Writes all record changes into the database. Returns FALSE or error. */
{ unsigned i;  atr_info * catr;  BOOL res, ins_rec;  trecnum erec;
  int coll_res;
 /* commit subviews: */
  if (inst->dt->cache_ptr==NULL) return TRUE;
  if (inst->sel_rec==NORECNUM) return TRUE;
  int off=(int)(inst->sel_rec-inst->dt->cache_top);
  if (off<0 || off>inst->dt->cache_reccnt) return TRUE;
 /* look for changes: */
  if (inst->dt->edt_rec == NORECNUM) return TRUE;
  BOOL any_change = FALSE;
  for (i=0, catr=inst->dt->attrs;  i<inst->dt->attrcnt;  i++, catr++)
    if (catr->changed) { any_change=TRUE;  break; }
  if (!any_change)  /* editation started, but nothing changed */
  { inst->dt->edt_rec = NORECNUM;
    return TRUE;
  }
  tptr p=(tptr)(inst->dt->cache_ptr + (LONG)off*inst->dt->rec_cache_size);
  if (*p!=REC_EXISTS)  /* fictive record */
    { ins_rec=TRUE;  erec=NORECNUM; }
  else   /* updating an existing recoed */
    { ins_rec=FALSE;  erec=inter2exter(inst, inst->dt->edt_rec); }

 /* check collision with other users unless inserting a new record: */
 /* positions on the updated rec (used for updating the data in cursor library) */
  if (!ins_rec)
  { coll_res=check_collision(inst, erec);
    if (coll_res==2) return FALSE;  /* cannot lock or cannot reload */
    if (coll_res==3)  /* revoke changes */
    { cache_roll_back(inst);
      return TRUE;
    }
  }
 /* locked, updating/inserting: */
  res=FALSE;
#ifdef CLIENT_ODBC_9
  if (!inst->cdp)
  { t_connection * conn = inst->dt->conn;
    if (conn->SetPos_support)
      res=setpos_synchronize_view(inst->cdp, !ins_rec, inst->dt, p, erec, TRUE);
    else
      res=odbc_synchronize_view9(!ins_rec, conn, inst->dt, erec, p, 
        (tptr)(inst->dt->cache_ptr+inst->dt->rec_cache_size*inst->dt->cache_reccnt), FALSE);  // CURRENT OF not supported by the access driver
   /* insert/write errors: */
    if (!conn->SetPos_support)
    { if (conn->can_transact)
        SQLSetConnectOption(conn->hDbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_ON);
      SQLFreeStmt(conn->hStmt, SQL_RESET_PARAMS);
    }
    if (!ins_rec && coll_res==1)  /* locked, must unlock */
      SQLSetPos(inst->dt->hStmt, 1, SQL_POSITION, SQL_LOCK_UNLOCK);
    if (res) return FALSE;
  }
  else  /* WinBase table: */
#endif
  { if (ins_rec)  /* fictive record */
    { if (IS_CURSOR_NUM(inst->dt->cursnum) || inst->stat->vwHdFt & FORM_OPERATION)
           erec=(trecnum)-1;
      else erec=(trecnum)-2;
    }
    cd_Start_transaction(inst->cdp);
    res=wb_write_cache(inst->cdp, inst->dt, inst->dt->cursnum, &erec, p, FALSE);
    if (!res) res=cd_Commit(inst->cdp);
   /* insert/write errors: */
    if (res)
    { Convert_error_to_generic(inst->cdp);
      cd_Roll_back(inst->cdp);
      if (!ins_rec && coll_res!=4) view_record_locking(inst, erec, FALSE, TRUE);
      client_error(inst->cdp, GENERR_CONVERTED);
      return FALSE;
    }
    if (!ins_rec && coll_res!=4) view_record_locking(inst, erec, FALSE, TRUE);
  }
 /* mark the view as fully synchronized (must be done before propagating cursor changes!): */
  for (i=0, catr=inst->dt->attrs;  i<inst->dt->attrcnt;  i++, catr++)
    catr->changed=wbfalse;
  inst->dt->edt_rec = NORECNUM;
  if (ins_rec)
  { if (inst->cdp)
    { remap_insert(inst, erec);  // adds the row to the grid
      trecnum irec=exter2inter(inst, erec);
      int off=irec-inst->dt->cache_top;
      if (off>=0 && off < inst->dt->cache_reccnt)  /* in cache, visible */
      { //tptr p = (tptr)(inst->dt->cache_ptr + off * inst->dt->rec_cache_size);
        //*p=REC_EXISTS;
       /* invalidate the new recs: new contents of the old fictive rec & new fictive rec */
        cache_free(inst->dt,       (int)off, (int)(inst->dt->cache_reccnt-off));
        cache_load(inst->dt, inst, (int)off, (int)(inst->dt->cache_reccnt-off), inst->dt->cache_top);
      }                                                  
    }  
  }
  else  
  {/* reload dependent values: */
    BOOL has_dependent=FALSE;
    for (i=0, catr=inst->dt->attrs;  i<inst->dt->attrcnt;  i++, catr++)
      if (catr->flags & CA_ROWVER) has_dependent=TRUE;
    if (has_dependent)
    { cache_free(inst->dt,       off, 1);
      cache_load(inst->dt, inst, off, 1, inst->dt->cache_top);
    }
  }  
  return TRUE;
}

CFNC DllPrezen void WINAPI set_top_rec(view_dyn * inst, trecnum new_top)
/* Loads the cache contents for new_top. Sets toprec to new_top or less iff
   new_top is too big. Supposes that edt_rec==NORECNUM. */
{ trecnum extrec, ext2;
  if (inst->dt->cache_ptr==NULL)  
    cache_resize(inst, 1);  // if the window is not visible, must init the cache 
  if (inst->toprec==new_top) return;
  if (new_top==NORECNUM) return;   /* fuse against errors */
 //  if (inst->stat->vwHdFt & FORM_OPERATION) return;
  if (inst->dt->cache_top<=new_top &&
      new_top+inst->recs_in_win <= inst->dt->cache_top+inst->dt->cache_reccnt)
    goto is_ok;
  
  if (inst->dt->cache_ptr!=NULL)
  {/* check new_top, decrease if necessary: */
    extrec=inter2exter(inst, new_top);
    if (extrec==NORECNUM)
      while (new_top)
      { ext2=inter2exter(inst, --new_top);
        if (ext2!=NORECNUM)  /* existing record found */
        { if (!(inst->stat->vwHdFt & NO_VIEW_FIC)) new_top++;  /* extrec==NORECNUM, newtop fictive */
          else extrec=ext2;              /* view without fictive record */
          if (inst->sel_rec > new_top)
              inst->sel_rec = new_top;
          break;
        }
      }
   /* update the cache: */
    set_cache_top(inst->dt, inst, new_top);
  }
 is_ok:
  inst->toprec=new_top;
}


/* Caching axioms:
- For identifying the record may be used the record number or a subset
  of attributes marked by "identif".
- When the 1st attribute of a record is about to be changed, the record
  contents is copied, except for multiatrs and var-len-atrs. Record is
  marked as being edited.
- Multiatr or var-len-atr cannot be used for identifying the record and
  the record must be locked when it is changed (or simult. changes will
  not be determined).
- Cache contains resc_in_win records. Tha last record has 2 functions:
  - when updating an existing record, it containt its original values,
  - when inserting a new record by INS, it is filled by implicit values.
*/

BOOL grow_object(indir_obj * iobj, UINT size)
{ HGLOBAL hnd, hnd1;  cached_longobj * obj1;
  if (iobj->obj && size <= iobj->obj->maxsize) return TRUE;
 /* allocate a new object: */
  hnd1=GlobalAlloc(GMEM_MOVEABLE, sizeof(cached_longobj)+size+1);
  if (!hnd1) return FALSE;
  obj1=(cached_longobj*)GlobalLock(hnd1);
 /* copy info: */
  obj1->maxsize=size;
  memcpy(obj1->data, iobj->obj->data, iobj->actsize);
 /* free the old object and replace them: */
  if (iobj->obj)
    { hnd=GlobalPtrHandle(iobj->obj);  GlobalUnlock(hnd);  GlobalFree(hnd); }
  iobj->obj=obj1;
  return TRUE;
}

static UWORD row_status;  /* must not be automatic,
  SQLExtendedFetch stores the address, SQLSetPos writes there. */
#ifdef CLIENT_ODBC_9
CFNC DllPrezen int WINAPI odbc_rec_status(cdp_t cdp, ltable * dt, trecnum extrec)
{ RETCODE retcode;  SQLULEN fetched;
  if (dt->conn->SetPos_support)   /* indirect objects are bound! */
  { //unsigned i;  atr_info * catr;
    //for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
    //  if ((catr->flags & (CA_MULTIATTR | CA_INDIRECT)) == CA_INDIRECT)
    //  { indir_obj * iobj = (indir_obj*)(dt->odbc_buf+catr->offset);
    //    iobj->obj=NULL;  iobj->actsize=0;
    //  }
    memset(dt->odbc_buf, 0, dt->rec_cache_real);
  }
  retcode=SQLExtendedFetch(dt->hStmt, SQL_FETCH_ABSOLUTE, 1+extrec,
                           &fetched, &row_status);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    return SQL_ROW_NOROW;
  if (!fetched)
    return SQL_ROW_NOROW;
  return row_status==SQL_ROW_UPDATED || row_status==SQL_ROW_ADDED ||
         row_status==SQL_ROW_ERROR ? // added in 5.1c, VUKROM, reading Excel data sometimes returns Error in row, Numeric Field Overflow
    SQL_ROW_SUCCESS : row_status;
}

#endif

void cache_roll_back(view_dyn * inst)
{ unsigned i;  atr_info * catr;  trecnum erec;  BOOL any_change;
  if (inst->dt->cache_ptr==NULL) return;
  if (inst->sel_rec==NORECNUM) return;
  trecnum intrec=inst->sel_rec;
  if (intrec <  inst->dt->cache_top ||
      intrec >= inst->dt->cache_top+inst->dt->cache_reccnt) return;
  tptr p = (tptr)(inst->dt->cache_ptr +
                  inst->dt->rec_cache_size * (unsigned)(intrec-inst->dt->cache_top));
  if (*p!=REC_EXISTS)  /* fictive record */ erec=NORECNUM;
  else erec=inter2exter(inst, inst->dt->edt_rec);
 /* mark the view as fully synchronized: */
  any_change=FALSE;
  for (i=0, catr=inst->dt->attrs;  i<inst->dt->attrcnt;  i++, catr++)
    if (catr->changed)
      { any_change=TRUE;  catr->changed=wbfalse; }
  if (inst->dt->edt_rec!=NORECNUM)
    inst->dt->edt_rec = NORECNUM;
 /* free the old and load the new values: */
  if (any_change)
  { cache_free(inst->dt,       intrec - inst->dt->cache_top, 1);
    cache_load(inst->dt, inst, intrec - inst->dt->cache_top, 1, inst->dt->cache_top);
  }
}

void cache_insert(view_dyn * inst, trecnum extrec)
{ if (inst->dt->cache_ptr==NULL) return;
  trecnum intrec = exter2inter(inst, extrec);
  if (intrec <  inst->dt->cache_top ||
      intrec >= inst->dt->cache_top+inst->dt->cache_reccnt)
    return;
  unsigned offset = (unsigned)(intrec - inst->dt->cache_top);
  unsigned size = inst->dt->cache_reccnt - offset - 1; /* number or moved records */
  memmove(inst->dt->cache_ptr+inst->dt->rec_cache_size*(offset+1),
          inst->dt->cache_ptr+inst->dt->rec_cache_size*offset,
                              inst->dt->rec_cache_size*size);
  cache_load(inst->dt, inst, offset, 1, inst->dt->cache_top);
}

CFNC DllPrezen BOOL WINAPI register_edit(view_dyn * inst, int attrnum)
/* Called when an editing action is started in the current record.
   If attrnum == -1, no attribute is changed so far (Editor opened).
   Returns FALSE if editing is disabled. */
{ if (inst->sel_rec==NORECNUM) return TRUE;
  if (inst->dt->edt_rec != inst->sel_rec)
  { if (inst->dt->edt_rec != NORECNUM)  /* Impossible, but ... */
      if (!cache_synchro(inst)) return FALSE;
    inst->dt->edt_rec=inst->sel_rec;
    memcpy(inst->dt->cache_ptr+inst->dt->rec_cache_size*inst->dt->cache_reccnt,
           inst->dt->cache_ptr+inst->dt->rec_cache_size*(unsigned)
           (inst->sel_rec-inst->dt->cache_top), inst->dt->rec_cache_real);
  }
  if (attrnum!=-1) inst->dt->attrs[attrnum].changed=wbtrue;
//  if (inst->dt->attrs[attrnum].flags & (CA_INDIRECT | CA_MULTIATTR))
//  { /* locking... */
//  }
  return TRUE;
}

/*************************** remaping record numbers ************************/
//typedef unsigned int size_t;

#if 0   // not used
uns16 rec_status(view_dyn * inst, trecnum inter, BOOL search)
{ uns8 del;
  if (inst->stat->vwHdFt & FORM_OPERATION)
    return inter ? REC_NONEX : REC_FICTIVE;
  ltable * ldt = inst->dt;
  if (ldt->remap_a)
  { if (inter2exter(inst, inter) != NORECNUM)
      return REC_EXISTS;  /* record OK */
    if (!(inst->stat->vwHdFt & NO_VIEW_FIC))
      if (!inter || (inter2exter(inst, inter-1) != NORECNUM))
        return REC_FICTIVE;   /* fictive */
    return REC_NONEX;
  }
  else
  { if (inter < ldt->rm_ext_recnum) return REC_EXISTS;
    if (inst->stat->vwHdFt & FORM_OPERATION) REC_FICTIVE;
   /* check the record existence in the cursor */
#ifdef CLIENT_ODBC_9
    if (!inst->cdp)
    { int status=odbc_rec_status(ldt->cdp, ldt, inter);
      if (status==SQL_ROW_SUCCESS || status==SQL_ROW_DELETED)
      { ldt->rm_ext_recnum=inter+1;
        SET_FI_EXT_RECNUM(inst);
        return REC_EXISTS;
      }
     /* inter is OUT_OF_CURSOR */
      if (inst->stat->vwHdFt & NO_VIEW_FIC)   /* no fictive record */
        return REC_NONEX;
     /* check to see if inter is a fictive record */
      if (!inter) return REC_FICTIVE;
      status=odbc_rec_status(ldt->cdp, ldt, inter);
      if (status!=SQL_ROW_SUCCESS && status!=SQL_ROW_DELETED)
        return REC_NONEX;
      ldt->rm_ext_recnum=inter;  ldt->fi_recnum=inter+1;
      return REC_FICTIVE;
    }
    else
#endif    
    { del=OUT_OF_CURSOR;
      Recstate(ldt->cdp, ldt->cursnum, inter, &del);
      if (del!=OUT_OF_CURSOR)
      { ldt->rm_ext_recnum=inter+1;
        SET_FI_EXT_RECNUM(inst);
        return REC_EXISTS;
      }
     /* inter is OUT_OF_CURSOR */
      if (inst->stat->vwHdFt & NO_VIEW_FIC)   /* no fictive record */
        return REC_NONEX;
     /* check to see if inter is a fictive record */
      if (!inter) return REC_FICTIVE;
      Recstate(ldt->cdp, ldt->cursnum, inter-1, &del);
      if (del==OUT_OF_CURSOR) return REC_NONEX;
      ldt->rm_ext_recnum=inter;  ldt->fi_recnum=inter+1;
      return REC_FICTIVE;
    }
  }
}
#endif

///////////////////////////// basic_vd //////////////////////////////////////
BOOL basic_vd::VdGetNativeVal(UINT ctrlID, trecnum irec, unsigned offset, int * length, char * val)
// Does not append terminator after string value
{ t_ctrl * itm = get_control_def(this, ctrlID);
 // query filend for text:
  if (!cdp)  // ODBC
  { //tptr code=get_var_value(&itm->ctCodeSize, CT_TEXT);   // column label --  why?  causes problems with Boolean checkboxes
    //if (code) return 2;
    if (itm->access==ACCESS_CACHE && itm->column_number!=-1 && dt!=NULL && get_var_value(&itm->ctCodeSize, CT_GET_ACCESS)!=NULL) 
    { atr_info * catr = dt->attrs+itm->column_number;
      tptr p=load_inst_cache_addr(this, inter2exter(this, irec), itm->column_number, NOINDEX, FALSE);
      if (p==NULL) /* non-existing multiattribute element must have NULL value */
        return FALSE; // db read error, no right, not in the cache, index out of range, ...
      if (catr->flags & CA_INDIRECT)
      { indir_obj * iobj = (indir_obj*)p;
        int size = (iobj->obj==NULL) ? 0 : iobj->actsize;
        if (val==NULL) *length=size; // reading the length only
        else // reading the interval
        { if (offset+*length > size) // required length must be reduced
          { if (offset >= size) size=0;
            else size -= offset;
            *length=size;
          }
          else size=*length; // reading the required length
          memcpy(val, iobj->obj->data+offset, size);  
        }
      }
      else  // fixed length value
      { int size = type_specif_size(itm->type, itm->specif);
        if (IS_ODBC_TYPE(itm->type)) size=odbc_tpsize(itm->type, itm->specif);  // this types are not supported by type_specif_size
        memcpy(val, p, size);
        int tp = itm->type & 0x7f;
        if (is_string(tp))
          { val[size]=0;  val[size+1]=0; }  // Unicode wide terminator
        else if (tp==ATT_ODBC_NUMERIC || tp==ATT_ODBC_DECIMAL)  // size includes first terminator
          val[size]=0;  // Unicode wide terminator
        if (length) *length=size;  // used by ATT_BINARY
      }
      return TRUE;
    } // cached access
    return FALSE;
  }

 // try ACCESS to the value:
  tptr code=get_var_value(&itm->ctCodeSize, CT_GET_ACCESS);
  if (code!=NULL)
  { *cdp->RV.kont_c=dt->cursnum;  cdp->RV.inst=this;
    *cdp->RV.kont_p=inter2exter(this, irec);
    run_prep(cdp, (tptr)stat, code-(tptr)stat, NO_EXCEPTION);
    cdp->RV.interrupt_disabled=true;
    int i=run_pgm(cdp);
    cdp->RV.interrupt_disabled=false;
    if (i!=EXECUTED_OK) { client_error(cdp, i);  return FALSE; }
    if (itm->access==ACCESS_PROJECT)  /* project variable, fixed size supposed */
    { int size = type_specif_size(itm->type, itm->specif);
      if (val)
      { memcpy(val, cdp->RV.stt->ptr, size);
        if (is_string(itm->type)) { val[size]=0;  val[size+1]=0; }
      }
      if (length) *length=size; // used by ATT_BINARY
    }
    else
    { curpos * wbacc = &cdp->RV.stt->wbacc;
      if ((itm->type & 0x80) && wbacc->index==NOINDEX) // access the 1st value, if not indexed
        wbacc->index=0;  // otherwise cache access error when multiattribute without index specified
      if (itm->access==ACCESS_CACHE && dt!=NULL) // dt==NULL for qbe view page header items!
      { atr_info * catr = dt->attrs+wbacc->attr;
        tptr p=load_inst_cache_addr(this, wbacc->position, wbacc->attr, wbacc->index, FALSE);
        if (p==NULL) /* non-existing multiattribute element must have NULL value */
          return FALSE; // db read error, no right, not in the cache, index out of range, ...
        if (itm->type==ATT_INDNUM)
          *(uns32*)val=(uns32) ((t_dynar*)p)->count();
        else if (itm->type==ATT_VARLEN)
          *(uns32*)val=((indir_obj*)p)->actsize;
        else if (catr->flags & CA_INDIRECT)
        { indir_obj * iobj = (indir_obj*)p;
          int size = (iobj->obj==NULL) ? 0 : iobj->actsize;
          if (val==NULL) *length=size; // reading the length only
          else // reading the interval
          { if (offset+*length > size) // required length must be reduced
            { if (offset >= size) size=0;
              else size -= offset;
              *length=size;
            }
            else size=*length; // reading the required length
            memcpy(val, iobj->obj->data+offset, size);  
          }
        }
        else  // fixed length value
        { int size = type_specif_size(itm->type, itm->specif);
          if (IS_ODBC_TYPE(itm->type)) size=odbc_tpsize(itm->type, itm->specif);  // this types are not supported by type_specif_size
          memcpy(val, p, size);
          int tp = itm->type & 0x7f;
          if (is_string(tp))
            { val[size]=0;  val[size+1]=0; }  // Unicode wide terminator
          else if (tp==ATT_ODBC_NUMERIC || tp==ATT_ODBC_DECIMAL)  // size includes first terminator
            val[size]=0;  // Unicode wide terminator
          if (length) *length=size;  // used by ATT_BINARY
        }
      } // cached access
      else /* access the normal database attribute */
      { if (itm->type==ATT_INDNUM || itm->type==ATT_VARLEN)
        { if (cd_Read_len(cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, (uns32*)val))
            { *(uns32*)val=0;  return FALSE; }  // 0 for safety, not necessary too much
        }
        else if (IS_HEAP_TYPE(itm->type & 0x7f))
        { uns32 rdsize;
          if (val==NULL) // reading the length only
          { if (cd_Read_len(cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, &rdsize)) 
              return FALSE;
          }
          else // reading the interval:
            if (cd_Read_var(cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index,
                           offset, *length, val, &rdsize)) return FALSE;
          *length=rdsize;
        }
        else // fixed sized value
        { if (cd_Read_ind(cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, val)) return FALSE;
          if (length) *length=type_specif_size(itm->type, itm->specif);
        }
      } // direct from database
    } // not project variable
  } // ACCESS found
  else // ACCESS not found, try VALUE
  { cdp->RV.interrupt_disabled=true;
    int i=run_code(CT_GET_VALUE, this, itm, irec);
    cdp->RV.interrupt_disabled=false;
    if (i)
    { if (i!=EXECUTED_OK) { client_error(cdp, i);  return FALSE; }
      if (IS_HEAP_TYPE(itm->type) && val!=NULL)
        if (itm->type==ATT_SIGNAT)  // access generated
        { curpos * wbacc = &cdp->RV.stt->wbacc;
          tptr p=load_inst_cache_addr(this, wbacc->position, wbacc->attr, wbacc->index, FALSE);
          if (p==NULL || offset) /* non-existing multiattribute element must have NULL value */
            return FALSE;
          indir_obj * iobj = (indir_obj*)p;
          unsigned size = (iobj->obj==NULL) ? 0 : iobj->actsize;
          if (*length < size) size=*length;
          memcpy(val, iobj->obj->data, size);  *length=size;
        }
        else return 2;
      else  // fixed size type
      { int size = type_specif_size(itm->type, itm->specif);
        if (is_string(itm->type)) // specif may not be properly calculated
        { if (itm->specif.wide_char)
          { size=(int)wuclen((wuchar*)cdp->RV.stt->ptr);
            if (size>MAX_FIXED_STRING_LENGTH) size=MAX_FIXED_STRING_LENGTH;
            size*=2;
            memcpy(val, cdp->RV.stt->ptr, size);  val[size]=val[size+1]=0;
          }
          else
            strmaxcpy(val, cdp->RV.stt->ptr, MAX_FIXED_STRING_LENGTH+1);
        }
        else if (itm->type==ATT_BINARY)
          memcpy(val, cdp->RV.stt->ptr, size);
        else
          memcpy(val, cdp->RV.stt,      size);
        if (length!=NULL) *length=size;
      }
    }
    else return 2;  // not ACCESS nor VALUE found
  } // trying VALUE
  return TRUE;  // OK
}

BOOL basic_vd::VdPutNativeVal(UINT ctrlID, trecnum irec, unsigned offset, int length, const char * val)
{ int res;
  t_ctrl * itm=get_control_def(this, ctrlID);
 // integrity check (cannot be done before inserting the new record):
  //if (!(stat->vwHdFt & FORM_OPERATION)) /* IFORM: integrity has already been checked */
 /* access & write */
  if (!cdp)  // ODBC
  { if (itm->access==ACCESS_CACHE && itm->column_number!=-1 && dt!=NULL && get_var_value(&itm->ctCodeSize, CT_GET_ACCESS)!=NULL) 
    { atr_info * catr = dt->attrs+itm->column_number;
      if (!(catr->flags & CA_RI_UPDATE) && !dt->recprivs)
      // does not check record write privils
      { unpos_box(_("Updating this value in not allowed"));
        return FALSE;
      }
      tptr p=load_inst_cache_addr(this, inter2exter(this, irec), itm->column_number, NOINDEX, TRUE);
      if (p==NULL) return FALSE; 
      if (!register_edit(this, itm->column_number)) return FALSE;
      if (catr->flags & CA_INDIRECT)
      { indir_obj * iobj = (indir_obj*)p;
        if (grow_object(iobj, offset+length))
        { if (val!=NULL) // writing data
          { memcpy(iobj->obj->data+offset, val, length);
            if (offset+length>iobj->actsize) iobj->actsize=offset+length;
          }
          else iobj->actsize=offset+length; // writing length
        }
      }
      else
      { 
        if (IS_ODBC_TYPE(itm->type)) 
             memcpy(p, val, odbc_tpsize(itm->type, itm->specif));  // this types are not supported by type_specif_size
        else 
          memcpy(p, val, type_specif_size(itm->type, itm->specif));
        if (is_string(itm->type & 0x7f)) p[itm->specif.length]=0;
      }
      return TRUE;
    }
    return FALSE;
  }

  int i=run_code(CT_GET_ACCESS, this, itm, irec);
  if (i)
    if (i==EXECUTED_OK)
    { if (itm->access==ACCESS_PROJECT)  /* project variable */
      { memcpy(cdp->RV.stt->ptr, val, type_specif_size(itm->type, itm->specif));
        if (is_string(itm->type)) cdp->RV.stt->ptr[itm->specif.length]=0;
        res=0;
      }
      else // not a project var
      { curpos * wbacc = &cdp->RV.stt->wbacc;
        if ((itm->type & 0x80) && wbacc->index==NOINDEX) // access the 1st value, if not indexed
          wbacc->index=0;  // otherwise cache access error when multiattribute without index specified
        if (itm->access==ACCESS_CACHE && dt!=NULL) // dt==NULL for qbe view page header items!
        { atr_info * catr = dt->attrs+wbacc->attr;
          if (!(catr->flags & CA_RI_UPDATE) && !dt->recprivs)
          // does not check record write privils
          { unpos_box(_("Updating this value in not allowed"));
            return FALSE;
          }
          tptr p=load_inst_cache_addr(this, wbacc->position, wbacc->attr, wbacc->index, TRUE);
          if (p==NULL) return FALSE;
          if (!register_edit(this, wbacc->attr)) return FALSE;
          if (itm->type==ATT_INDNUM)
          { t_dynar * dy = (t_dynar*)p;
            while (dy->count() > *(uns16*)val)
            { if (catr->flags & CA_INDIRECT)
              { indir_obj * iobj = (indir_obj*)dy->acc0(dy->count()-1);
                if (iobj->obj)
                { GlobalUnlock(GlobalPtrHandle(iobj->obj));
                  GlobalFree  (GlobalPtrHandle(iobj->obj));
                }
              }
              dy->delet(dy->count()-1);
            }
          }
          else if (itm->type==ATT_VARLEN)
          { if (((indir_obj*)p)->actsize > *(uns32*)val)
              ((indir_obj*)p)->actsize=*(uns32*)val;
          }
          else if (catr->flags & CA_INDIRECT)
          { indir_obj * iobj = (indir_obj*)p;
            if (grow_object(iobj, offset+length))
            { if (val!=NULL) // writing data
              { memcpy(iobj->obj->data+offset, val, length);
                if (offset+length>iobj->actsize) iobj->actsize=offset+length;
              }
              else iobj->actsize=offset+length; // writing length
            }
          }
          else
          { 
            if (IS_ODBC_TYPE(itm->type)) 
                 memcpy(p, val, odbc_tpsize(itm->type, itm->specif));  // this types are not supported by type_specif_size
            else 
              memcpy(p, val, type_specif_size(itm->type, itm->specif));
            if (is_string(itm->type & 0x7f)) p[itm->specif.length]=0;
          }
          res=0;
        }
        else   /* database attribute */
        { curpos * wbacc = &cdp->RV.stt->wbacc;
          if      (itm->type==ATT_INDNUM)
            res=cd_Write_ind_cnt(cdp, wbacc->cursnum, wbacc->position, wbacc->attr, *(uns16*)val);
          else if (itm->type==ATT_VARLEN)
            res=cd_Write_len    (cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, *(uns32*)val);
          else if (IS_HEAP_TYPE(itm->type & 0x7f))   
            if (val!=NULL) // writing data
              res=cd_Write_var  (cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, offset, length, val);
            else // writing length
              res=cd_Write_len  (cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, offset+length);
          else
          { if (wbacc->attr==DEL_ATTR_NUM)
              if (*val!=DELETED && *val!=NOT_DELETED)
              { wxGetApp().frame->SetStatusText(_("Cannot write the 3rd state, you must select DELETED or NOT DELETED"));
                return FALSE;
              }
            res=cd_Write_ind    (cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, (tptr)val, type_specif_size(itm->type, itm->specif));
          }
          if (res || wbacc->attr==DEL_ATTR_NUM && cd_Sz_warning(cdp))  /* reporting database errors */
          { stat_signalize(cdp);
            return FALSE;
          }
          else
            wxGetApp().frame->SetStatusText(wxEmptyString); /* remove last error, ESC may have not been used */
        } // direct to database
      } // not a project variable
     /* The "changed" attribute must be removed before CT_ACTION is runned,
        otherwise "stack overflow" may occur. */
      if (vwOptions & SIMPLE_VIEW)
        subinst->editchanged=FALSE;  /* even if will not be synchronized */
//          else sel_is_changed=FALSE;
//          now, done in write_val
     /* value written OK, run the foll. actions: */
      //post_notif(hWnd, NOTIF_CHANGE);
      if (itm->ctClassNum!=NO_TEXT && itm->ctClassNum!=NO_RASTER && itm->ctClassNum!=NO_OLE)
      { run_code(CT_ACTION, this, itm, irec);  /* run the change-proc (but not the activate-proc) */
        if (!stat) return TRUE; // update action closed the form
      }
      return TRUE;
    }
    else { stat_signalize(cdp); /* may not always be good */ return FALSE; }
  return TRUE;
}

int  basic_vd::VdIsEnabled(UINT ctrlID, trecnum irec)
/* enabled if code not defined, enabled on error on fictive record */
{ return run_code(CT_ENABLED, this, get_control_def(this, ctrlID), irec)
         ==EXECUTED_OK ? cdp->RV.stt->int8+1 : 2;
}

void basic_vd::VdRecordEnter(BOOL hard_synchro)
{ if (last_sel_rec==sel_rec) return;
  if (stat->vwHdFt & FORM_OPERATION) return;
  view_dyn::VdRecordEnter(hard_synchro);  // writes status nums
  if (lock_state & LOCK_REC)
  { trecnum erec=inter2exter(this, sel_rec);
    if (erec==NORECNUM) return;
    if (!view_record_locking(this, erec, TRUE, FALSE))
    { locked_record=erec;
      lock_state=LOCK_REC | LOCKED_REC;
    }
  }
  last_sel_rec=sel_rec;
#if 0
  if (!(stat->vwStyle3 & DISABLE_AUTOSYNC))
    if (sel_rec!=NORECNUM)  // unless column selected
    // column selection in subview synchronizes on last record in superview ->
    // and cache is emptied.
      synchronize_views(cdp, hWnd, hard_synchro);  /* usually soft-synchro */
  run_view_code(VW_NEWRECACT, this);
  post_notif(hWnd, NOTIF_RECENTER);
#endif
}

///////////////////////////// view_dyn members /////////////////////////////

int  view_dyn::VdIsEnabled (UINT ctrlID, trecnum irec) { return 2; }
void view_dyn::VdRecordEnter(BOOL hard_synchro)
{ 
#if 0
  if (IsPopup() || hWnd==(HWND)SendMessage(cdp->RV.hClient, WM_MDIGETACTIVE, 0, 0))
  { if (inter2exter(this, sel_rec) == NORECNUM) Set_status_nums(NORECNUM, NORECNUM);
    else Set_status_nums(sel_rec,
      dt->rm_completed ? (Remapping() ? dt->rm_int_recnum : dt->rm_ext_recnum) : NORECNUM);
  }
  else // is a child
  { HWND hParent = (HWND)GetWindowLong(hWnd, GWL_HWNDPARENT);
    if (hParent) SendMessage(hParent, SZM_CHILDPOSITION, (WPARAM)hWnd, 0);
  }
#endif
}

BOOL view_dyn::VdGetNativeVal(UINT ctrlID, trecnum irec, unsigned offset, int * length, char * val) { return FALSE; }
BOOL view_dyn::VdPutNativeVal(UINT ctrlID, trecnum irec, unsigned offset, int length, const char * val) { return FALSE; }

void view_dyn::destruct(void)
// If destroy_on_close then hWnd must be 0!
{ if (hWnd) close();  // does not destroy the inst
#if 0
  if (print_fonts.count())  /* this view is used for printing (exclusively or not) */
    cancel_printing();
#endif
  if (table_def_locked) 
  { cd_Read_unlock_record(cdp, TAB_TABLENUM, dt->supercurs != NOOBJECT ? dt->supercurs : dt->cursnum);
    table_def_locked=false;
  }
  if (lock_state & LOCKED_ALL && dt->conn==NULL)
    cd_Read_unlock_table(cdp, dt->cursnum);
  else if (lock_state & LOCKED_REC)
    view_record_locking(this, locked_record, TRUE, 2);
  if (designed_toolbar) designed_toolbar->Release();

  if (dt !=NULL) dt ->Release(); // closes cursor & supercursor if requred
  if (qbe!=NULL) qbe->Release(); // removes QBE & other conditions
  corefree(relsyn_cond);

  corefree(upper_select_st);
#if 0
  free_aggr_info(&aggr);
  release_print_fonts();
  release_fonts_brushes();
  if (pall) { DeleteObject(pall);  pall=0; }
#endif
//  if (vwOptions & SIMPLE_VIEW) -- cannot check, stat may be NULL
  if (subinst)
    { free_sinst((sview_dyn*)subinst);  subinst=NULL; }
  safefree((void**)&stat);
  if (bind)
  { delete bind;  bind=NULL;
    wxGetApp().frame->SetStatusText(wxEmptyString);  /* removes the number of bound recs */
  }
 /* remove from the list off all views (if present): */
#if 0
  set_temp_inst(NULL);
  if (hWnd) SetWindowLong(hWnd, 0, 0);
  view_dyn ** pinst = &cdp->all_views;
  while (*pinst != NULL)
    if (*pinst==this)
      { *pinst=next_view;  break; }
    else pinst=&(*pinst)->next_view;
#endif
  if (enums!=NULL) delete enums;
 // must put into clear state, because destruct may be called twice:
 //  ...during unsuccessfull compilation and when destrructed the variable:
  init(xcdp, destroy_on_close);
  corefree(filter);
  corefree(order);
}

view_dyn::~view_dyn(void)
{ hWnd=0;  destruct(); }

view_dyn::view_dyn(xcdp_t xcdpIn)
{ init(xcdpIn, TRUE); }

void view_dyn::init(xcdp_t xcdpIn, BOOL destroy_on_closeIn)
{ xcdp=xcdpIn;  
  cdp=xcdp->get_cdp();
  destroy_on_close=destroy_on_closeIn;
  hWnd         =0;
  bind         =NULL;
  vwOptions    =0;
  par_inst     =NULL;
  hParent      =0;
  idadr        =NULL;
  query_mode   =NO_QUERY;
  vwobjname[0] =0;
  locked_record=NORECNUM;
  lock_state   =(uns8)0;
  upper_select_st=NULL;
  qbe          =NULL;
  relsyn_cond  =NULL;
  form_fonts.init();
  print_fonts.init();
  form_brushes.init();
  aggr         =NULL;
  pall         =0;
  olecldoc     =NULL;
  subinst      =NULL;
  stat         =NULL;
  recheight    =0;   /* this is used in 1st WM_GETMINMAXINFO! */
  has_subview  =wbfalse;
  last_sel_rec =(trecnum)-2;  /* diffrent from all record numbers & NORECNUM */
  disabling_view=0;
  dt           =NULL;
  insert_inst  =NULL;
  penv         =NULL;
  next_view    =NULL;
  hDisabled    =0;
  view_mode    =VWMODE_MDI;
  synchronizing=wbfalse;
  info_flags   =0;
  hToolTT      =0;
  enums        =NULL;
  toprec=sel_rec=0;
  hordisp      =0;
  sel_itm      =0;
  designed_toolbar = NULL;
  wheel_delta  =0;
  ClientRect.x =0;
  ClientRect.y =0;
  has_def_push_button = NULL;
  SubViewID    = -1;
  table_def_locked=false;
  recs_in_win  = (unsigned)-1;  // will be compared to the calculated value and must not be the same
  filter       = NULL;
  order        = NULL;
}

int view_dyn::get_current_item(void)
{ if (vwOptions & SIMPLE_VIEW)
  { int sel_col=subinst->sel_col;
    if (sel_col==-1) return 0;
    return subinst->sitems[sel_col].itm->ctId;
  }
  else 
    return get_control_pos(this, sel_itm)->ctId;
}

typedef char tpiece[6];

uns8 tpsize[ATT_AFTERLAST] =
 {0,                                    /* fictive, NULL */
  1,1,2,4,6,8,0,0,0,0,4,4,4,sizeof(trecnum),sizeof(trecnum), /* normal  */
  UUID_SIZE,4,4+sizeof(tpiece),         /* special */
  4+sizeof(tpiece),4+sizeof(tpiece),    /* raster, text */
  4+sizeof(tpiece),4+sizeof(tpiece),    /* nospec, signature */
  9,0,0,0,0,                            /* UNTYPED, not used & ATT_NIL */
  1,sizeof(ttablenum),sizeof(tobjnum),8,0,3*sizeof(tcursnum), /* file, table, cursor, dbrec, view, statcurs */
  0,0,0,0,0,0,0,0,0,0,0,1,8             /* ..., int8, int64 */
};  
/* NOTE: 0 size for string type is used when generating I_LOAD */
/////////////////////////// replacements //////////////////////////////////////////
int run_code(uns8 ct, view_dyn * inst, t_ctrl * itm, trecnum irec)
{ tptr code;  int res;
  int codesize = itm->ctCodeSize;  var_start * ptr = (var_start*)itm->ctCode;
  cdp_t cdp = inst->cdp;  // inst may be deleted during the execution!
  if (cdp->RV.holding) return 0;
 /* find the code */
  do
  { if (codesize <= 0) return 0;
    if (ptr->type==ct) break;
    codesize-=ptr->size + 3;
    ptr=(var_start*)((tptr)ptr + ptr->size + 3);
  } while (TRUE);
 /* prepare the run */
  *cdp->RV.kont_c=inst->dt->cursnum;  cdp->RV.inst=inst;
  cdp->RV.view_position=irec;
  *cdp->RV.kont_p=inter2exter(inst, irec);
//  if (*cdp->RV.kont_p==NORECNUM)
//    if (!(inst->stat->vwHdFt & FORM_OPERATION))
//      if (itm->pane==PANE_DATA)   /* otherwise recnum may be NORECNUM */
//        if ((ct!=CT_VISIBLE) && (ct!=CT_ENABLED))
//          return 0;
 /* run the copy of the code */
  code=(tptr)corealloc(ptr->size, 72);
  if (!code) return 0;
  memcpy(code, ptr->value, ptr->size);
 /* executing the code */
  if (cdp->RV.global_state!=DEBUG_BREAK && cdp->RV.bpi)  /* not breaked but debugable (must prevent re-entering the breaked state)! */
  { run_prep(cdp, code, 0, SET_BP);
    uns8 outer_state=cdp->RV.global_state;
    res=run_pgm(cdp);   /* executes the initial jump */
#if 0
    if (res==PGM_BREAKED)
    { enter_breaked_state(cdp);
      MSG msg;
      while ((cdp->RV.global_state!=DEVELOP_MENU) && GetMessage(&msg, 0,0,0))
      { if (!TranslateMDISysAccel(cdp->RV.hClient, &msg))
        { TranslateMessage(&msg);       /* Translates virtual key codes         */
          DispatchMessage(&msg);       /* Dispatches message to window         */
        }
      }
      res=cdp->RV.pc;
    }
#endif
    cdp->RV.global_state=outer_state;
  }
  else
  { run_prep(cdp, code, 0);
    res=run_pgm(cdp);
  }
 /* must copy the result string before corefree(code); because it may be a literal stored in the code */
  if ((ct==CT_GET_VALUE) && is_string(itm->type) && (res==EXECUTED_OK))
    if ((cdp->RV.stt->ptr!=cdp->RV.hyperbuf) && (cdp->RV.stt->ptr!=cdp->RV.hyperbuf+2))
    { if (itm->specif.wide_char)
        wucmaxcpy((wuchar*)cdp->RV.hyperbuf, (wuchar*)cdp->RV.stt->ptr, sizeof(cdp->RV.hyperbuf)/sizeof(wuchar));
      else
        strmaxcpy(cdp->RV.hyperbuf, cdp->RV.stt->ptr, sizeof(cdp->RV.hyperbuf));
      cdp->RV.stt->ptr=cdp->RV.hyperbuf;
    }
  corefree(code);
  if (res!=EXECUTED_OK) client_error(cdp, res); // puts the result of the inner run into the monitor window
  return res;
}

int run_view_code(uns8 ct, view_dyn * inst)
{ tptr code;  int res;  run_state * RV = &inst->cdp->RV;
  if (RV->holding) return 0;
  code=get_var_value(&inst->stat->vwCodeSize, ct);
  if (code==NULL) return 0;
 /* prepare the run */
  *RV->kont_c=inst->dt->cursnum;  RV->inst=inst;
  RV->view_position=inst->sel_rec;
  *RV->kont_p=inter2exter(inst, inst->sel_rec);
 /* executing the code */
  if (RV->bpi)  /* not breaked */
  { uns8 outer_state=RV->global_state;
    run_prep(inst->cdp, code, 0, SET_BP);
    res=run_pgm(inst->cdp);   /* executes the initial jump */
#if 0
    if (res==PGM_BREAKED)
    { enter_breaked_state(inst->cdp);
      MSG msg;
      while ((RV->global_state!=DEVELOP_MENU) && GetMessage(&msg, 0,0,0))
      { if (!TranslateMDISysAccel(RV->hClient, &msg))
        { TranslateMessage(&msg);       /* Translates virtual key codes         */
          DispatchMessage(&msg);       /* Dispatches message to window         */
        }
      }
      res=RV->pc;
    }
#endif
    RV->global_state=outer_state;
  }
  else 
  { run_prep(inst->cdp, code, 0);
    res=run_pgm(inst->cdp);
  }
  return res;
}


