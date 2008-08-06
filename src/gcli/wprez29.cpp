// wprez29.cpp
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
#include "compil.h"
#include "datagrid.h"

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif 

int code_report=0;  /* print uses it */
UINT cf_rtf;

wbbool static synchro_locked = wbfalse;

static char        ViewClassName[]      = "SezamView32";
static char        SviewClassName[]     = "SimpleSezamView32";
static char        PictureClassName[17] = "WBPictureClass32";
       char        WBCOMBO_LIST_NAME[]  = "ViewComboList32";

extern char BUTTON[];

#if 0
CFNC DllPrezen HWND WINAPI CreateMDIWindow31(const char * window_class, const char * window_title,
      DWORD style, int x, int y, int cx, int cy,
      HWND hParent, HINSTANCE hInst, LPARAM lParam)
{ MDICREATESTRUCT MdiCreate;
  MdiCreate.szClass = window_class;
  MdiCreate.szTitle = window_title;
  MdiCreate.style   = style;
  MdiCreate.hOwner  = hInst;
  MdiCreate.x       = x;
  MdiCreate.y       = y;
  MdiCreate.cx      = cx;
  MdiCreate.cy      = cy;
  MdiCreate.lParam  = lParam;
  return (HWND)SendMessage(hParent, WM_MDICREATE, 0, (LPARAM)&MdiCreate);
}
#endif
/****************************** programmer interface ************************/
CFNC DllPrezen BOOL WINAPI IsView(HWND hWnd)
{ return true;
#if 0
  if (!IsWindow(hWnd)) return FALSE;
  return (int)GetProp(hWnd, "WBCHILDTYPE") == VIEW_MDI_CHILD;
#endif
}

BOOL IsWBChild(HWND hWnd)
{ return false;
#if 0
  if (!IsWindow(hWnd)) return FALSE;
//  int chType = (int)SendMessage(hWnd, SZM_MDI_CHILD_TYPE, 0, 0);
//  return(chType >= FIRST_MDI_CHILD && chType <= LAST_MDI_CHILD);
  return GetProp(hWnd, "WBCHILDTYPE") != 0;
#endif
}

#ifndef WINS
#define GetWindowLong(a,b) 0
#endif

#if 0
CFNC DllPrezen int WINAPI QBE_state(HWND hWnd)
{ if (!IsView(hWnd)) return NONEINTEGER;
  view_dyn * inst = GetViewInst(hWnd);
  if (inst==NULL) return NONEINTEGER;
  return inst->query_mode;
}

CFNC DllPrezen HWND WINAPI find_view(char * viewdef)
{ HWND hwChild;  view_dyn * inst;
  cdp_t cdp = GetCurrTaskPtr();
  while (*viewdef==' ') viewdef++;
  if (*viewdef=='*') viewdef++;
  while (*viewdef==' ') viewdef++;
  Upcase(viewdef);
 /* look for mdi-child views: */
  hwChild=GetWindow(cdp->RV.hClient, GW_CHILD);
  while (hwChild)
  { if (IsView(hwChild))
    { inst = (view_dyn*)GetWindowLong(hwChild, 0);
      if (!my_stricmp(viewdef, inst->vwobjname)) return hwChild;
    }
    hwChild=GetWindow(hwChild, GW_HWNDNEXT);
  }
 /* look for modal views: */
  hwChild=GetWindow(GetDesktopWindow(), GW_CHILD);
  while (hwChild)
  { if (IsView(hwChild))
    { inst = GetViewInst(hwChild);
      if (!my_stricmp(viewdef, inst->vwobjname)) return hwChild;
    }
    hwChild=GetWindow(hwChild, GW_HWNDNEXT);
  }
 // subview of a modal view etc:
  inst=find_inst(cdp, viewdef);
  return inst ? inst->hWnd : 0;
}
#endif

CFNC DllPrezen view_dyn * WINAPI find_inst(cdp_t cdp, const char * viewdef)
{ 
#if 0
  view_dyn * an_inst = cdp->all_views;
  while (an_inst != NULL)
  { if (!my_stricmp(viewdef, an_inst->vwobjname)) return an_inst;
    an_inst=an_inst->next_view;
  }
#endif
  return NULL;
}

#if 0
CFNC DllPrezen void WINAPI Close_view(HWND hView)
{ if (IsView(hView))
  { view_dyn * inst = GetViewInst(hView);
    if (inst!=NULL) inst->close();
  }
}

CFNC DllPrezen void WINAPI Close_all_views(void)
{ HWND hwChild, hwClient=get_RV()->hClient;  int mdi_child_type;
  int counter = 0; /* prevents infinite loop (Libicher observed sometimes) */
  ShowWindow(hwClient, SW_HIDE);
  hwChild=GetWindow(hwClient, GW_CHILD);
  while (hwChild && (counter < 500))
  { if (!GetWindow(hwChild, GW_OWNER))  /* FALSE for icon title */
    { mdi_child_type=(int)SendMessage(hwChild, SZM_MDI_CHILD_TYPE, 0, 0);
      if (mdi_child_type==VIEW_MDI_CHILD)
      { SendMessage(hwClient, WM_MDIDESTROY, (WPARAM)hwChild, 0L);
        hwChild=GetWindow(hwClient, GW_CHILD);       /* restart the search */
      }
      else hwChild=GetWindow(hwChild, GW_HWNDNEXT);  /* check the next child */
    }
    else hwChild=GetWindow(hwChild, GW_HWNDNEXT);  /* check the next child */
    counter++;
  }
  ShowWindow(hwClient, SW_SHOWNA);
}
#endif

static BOOL find_view_object(cdp_t cdp, const char * vwobjname, tobjnum * objnum)
{ if (!cd_Find2_object(cdp, vwobjname, NULL, CATEG_VIEW, objnum))
    return TRUE;
#if 0
  if (!strcmp(vwobjname, "???")) unpbox(MAKEINTRESOURCE(BINDVIEW_UNDEF));
  else
#endif
  { SET_ERR_MSG(cdp, vwobjname);  client_error(cdp, VIEW_NOT_FOUND);  cd_Signalize(cdp); }
  return FALSE;
}

static HWND aux_hWnd;

t_relation * get_relation(cdp_t cdp, tobjnum objnum)
// Given relation object number returns relation description.
{ for (int relind=0;  relind<cdp->odbc.apl_relations.count();  relind++)
  { t_relation * rel = cdp->odbc.apl_relations.acc0(relind);
    if (rel->objnum == objnum) return rel;
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////

#if 0
static void cursor_changed(view_dyn * inst, int notify)
{ 
  inst->sel_rec=inst->toprec=inst->dt->cache_top=0;
  if (inst->Remapping())
    remap_reset(inst);
  else 
  { if (inst->hWnd) ((DataGrid*)inst->hWnd)->DeleteRows(0, inst->dt->rm_ext_recnum);  // not removing the fictive row, if any, bacause it will not be added in remap_expand
    inst->dt->rm_int_recnum=inst->dt->rm_ext_recnum=inst->dt->rm_curr_int=inst->dt->rm_curr_ext=0;
    inst->dt->rm_completed=false;  /* used even if no remap */
    SET_FI_EXT_RECNUM(inst);
    cache_reset(inst);
  }
  if (inst->hWnd)
  { ((DataGrid*)inst->hWnd)->dgt->prepare();  // otherwise records not shown
    ((DataGrid*)inst->hWnd)->last_recnum=inst->dt->fi_recnum;
    ((DataGrid*)inst->hWnd)->synchronize_rec_count();
    ((DataGrid*)inst->hWnd)->write_current_pos(inst->sel_rec);
    inst->last_sel_rec=(trecnum)-2;
    inst->VdRecordEnter(TRUE);
  }
}
#endif

/******************************* qbe & order ********************************/
#define MAX_SUBCOND_SIZE 5000   /* subcursor definition max. size */
#define MAX_COND_SZ      250    /* subcursor item max. size */

t_ctrl * get_control_by_text(view_dyn * inst, tptr text)
{ t_ctrl * itm;  int i;  tptr tx;
  for (i=0, itm=FIRST_CONTROL(inst->stat);  i<inst->stat->vwItemCount;
       i++, itm=NEXT_CONTROL(itm))
    if (!(itm->ctStyle & (QUERY_STATIC | QUERY_DYNAMIC)))
    // not searching a query field because it conatins query pattern which
    // must not be used in constrains entered via QBE view!
    { tx=get_var_value(&itm->ctCodeSize, CT_NAMEA);
      if (tx!=NULL && !my_stricmp(text, tx)) return itm;
      tx=get_var_value(&itm->ctCodeSize, CT_NAMEV);
      if (tx!=NULL && !my_stricmp(text, tx)) return itm;
    }
  return NULL;
}

tptr prep_qbe_subcurs(view_dyn * inst, view_dyn * dest_inst)
/* converts conditions entered in a query-view into a SQL-query */
{ 
#if WBVERS>=900
  return NULL;
#else
  tptr fullexpr, p, q, objtxt;  BOOL isnextrec, rel, is_rel;  unsigned objlen, l, par;
  uns8 rec, lastgenrec;  add_cond * acond;  char c;  t_ctrl * itm;
  tptr enumdef;  int index;
  BOOL multi;  char numbuf[14];

  if (!(p=fullexpr=sigalloc(MAX_SUBCOND_SIZE, OW_PREZ))) return NULL;
  *(p++)=' ';            /* may be overwritten by QUERY_TEST_MARK */
  rec=0;  lastgenrec=0xff;
  do
  { isnextrec=FALSE;  acond=inst->qbe->add_conds;
    while (acond!=NULL)
    { if (acond->rec==rec)
      { q     =acond->cond_text;
        objtxt=acond->item_text;
        if (acond->item_id)  // query-field entered this condition
          itm=get_control_def(inst->qbe->query_field_inst, acond->item_id);
        else // condition from a query-view
          itm=get_control_by_text(inst, acond->item_text);
        if (itm==NULL) goto next_cond; // unsupported item in the query view
        if (q && objtxt && acond->cond_active)
        { objlen=strlen(objtxt);
#ifdef NEVER
          if (itm->ctClassNum==NO_LIST)
          { while (objlen && (objtxt[objlen-1]==' ')) objlen--;
            if (objlen && (objtxt[objlen-1]==']'))
            { objlen--;
              while (objlen && (objtxt[objlen-1]==' ')) objlen--;
              if (objlen && (objtxt[objlen-1]=='['))
                { multi=TRUE;  objlen--; }
            }
          }
#endif
          if (itm->ctStyle & INDEXABLE)
          { multi=TRUE;
            while (objlen && (objtxt[objlen-1]==' ' ||
                   objtxt[objlen-1]=='[' ||  objtxt[objlen-1]==']'))
              objlen--;
          } else multi=FALSE;
         /* translating enumer values */
          enumdef=get_enum_def(acond->item_id ? inst->qbe->query_field_inst : inst, itm);
          if (enumdef &&   /* translate if itm has a proper type */
              (itm->ctClassNum==NO_VAL || itm->ctClassNum==NO_COMBO))
          { index=*(uns16*)q;
            while (--index)
            { if (!*enumdef) { corefree(fullexpr);  return NULL; }  /* strange error */
              enumdef+=strlen(enumdef)+1+sizeof(sig32);
            }
            if (itm->type==ATT_CHAR)
              { numbuf[0]=enumdef[strlen(enumdef)+1];  numbuf[1]=0; }
            else int2str(*(sig32*)(enumdef+strlen(enumdef)+1), numbuf);
            q=numbuf;
          }

          if ((itm->ctClassNum==NO_CHECK || itm->ctClassNum==NO_RADIO) &&
              itm->type==ATT_BOOLEAN)
          { if (p+objlen+10 > fullexpr+MAX_SUBCOND_SIZE)
              { corefree(fullexpr);  return NULL; }
                                if (lastgenrec==0xff) *(p++)='(';
            else if (lastgenrec==rec) { strcpy(p,"and(");  p+=4; }
            else { strcpy(p,"or(");  p+=3; }
            memcpy(p, objtxt, objlen);  p+=objlen;
            if (*q=='-') { strcpy(p, "=FALSE");  p+=6; }
            else { strcpy(p, "=TRUE");  p+=5; }
            *(p++)=')';
            lastgenrec=rec;
          }
          else do
          { while (*q==' ') q++;
            if (!*q) break;
            l=par=0;
            while (q[l])
            { if (!par)
                if (q[l]==',')
                { tptr qq;
                  qq=q+l+1;  while (*qq==' ') qq++;
                  if ((*qq=='<') || (*qq=='>') || (*qq=='.') || (*qq=='~'))
                    break;
                }
              if (q[l]=='(') par++; else if (q[l]==')') par--;
              l++;
            }
            if (p+objlen+l+18 > fullexpr+MAX_SUBCOND_SIZE)
              { corefree(fullexpr);  return NULL; }
            if (lastgenrec==0xff) *(p++)='(';
            else if (lastgenrec==rec) { strcpy(p,"and(");  p+=4; }
            else { strcpy(p,"or(");  p+=3; }

            tptr pattern=get_var_value(&itm->ctCodeSize, CT_QUERYCOND);
            if (pattern!=NULL)
            { while (*pattern)
              { if (*pattern=='%') p+=copy_decorated_value(p, q, l, itm->type, dest_inst);
                else *(p++)=*pattern;
                pattern++;
              }
            }
            else
            { memcpy(p,objtxt, objlen);  p+=objlen;
             /* look for relation operator: */
              is_rel=FALSE;
              do
              { c=*q;
                rel=(c=='=')||(c=='<')||(c=='>')||(c=='!')||(c=='~')||(c=='.');
                if (!rel) break;
                is_rel=TRUE;
                *(p++)=c;  q++;  l--;
              } while (TRUE);
              while (*q==' ') { q++; l--; }
             /* add implicit relation operator: */
              if (!is_rel)
                if (itm->type==ATT_TEXT)
                  { *(p++)='.';  *(p++)='=';  *(p++)='.'; }
                else if (multi)
                  { *(p++)='.';  *(p++)='.'; }
                else *(p++)='=';
             /* add decorated value: */
              p+=copy_decorated_value(p, q, l, itm->type, dest_inst);
            }
            *(p++)=')';
            lastgenrec=rec;
            q+=l;
            if (*q==',') q++;
          } while (TRUE);
        } /* condition active, non-empty and attribute name defined */
      }
      else if (acond->rec > rec) isnextrec=TRUE;
     next_cond:
      acond=acond->next_cond;
    }
    rec++;
  } while (isnextrec);

  if (inst->qbe->order_active)
  { BOOL first_order=TRUE;
    for (int i=0;  i<MAX_INDEX_PARTS;  i++)
    { objtxt=inst->qbe->add_order[i].item_text;
      if (objtxt!=NULL)
      {// join with the previous text:
        if (first_order)
        { *(p++)=ORDER_BYTEMARK;
          strcpy(p, "ORDER BY ");  p+=9;
          first_order=FALSE;
        }
        else *(p++)=',';
       // add the ordering part:
        objlen=strlen(objtxt);
        if (p+objlen+4+5 > fullexpr+MAX_SUBCOND_SIZE)
          { corefree(fullexpr);  return NULL; }
        memcpy(p, objtxt, objlen);  p+=objlen;
        if (inst->qbe->add_order[i].desc)
          { strcpy(p," DESC");  p+=5; }
      }
    }
  }

  *p=0;
  return fullexpr;
#endif
}

#if 0
HSTMT create_odbc_subcurs(tptr subcurscond, view_dyn * inst, BOOL really, tptr * new_select)
// Returns NOOBJECT on error. If OK, returns handle if really, 1 otherwise.
// If really and OK returns the used select statement in new_select.
{ HSTMT hSubStmt;  RETCODE retcode;  uns32 opt;
  if (new_select) *new_select=NULL;
  tptr orig_select = inst->upper_select_st ? inst->upper_select_st : inst->dt->select_st;
  tptr cond=sigalloc(strlen(orig_select)+20+strlen(subcurscond), 89);
  if (cond==NULL) return (HSTMT)NOOBJECT;
  merge_condition(cond, orig_select, subcurscond);
 /* prepare statement handle: */
  t_connection * conn = inst->dt->conn;
  retcode=SQLAllocStmt(conn->hDbc, &hSubStmt);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    { corefree(cond);  return (HSTMT)NOOBJECT; }
       if (conn->scroll_options & SQL_SO_KEYSET_DRIVEN) opt=SQL_CURSOR_KEYSET_DRIVEN;
  else if (conn->scroll_options & SQL_SO_MIXED) opt=SQL_CURSOR_KEYSET_DRIVEN;
  else if (conn->scroll_options & SQL_SO_DYNAMIC) opt=SQL_CURSOR_DYNAMIC;
  else opt=SQL_CURSOR_STATIC;
  retcode=SQLSetStmtOption(hSubStmt, SQL_CURSOR_TYPE, opt);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    odbc_stmt_error(inst->cdp, hSubStmt);
       if (conn->scroll_concur & SQL_SCCO_OPT_VALUES) opt=SQL_CONCUR_VALUES;
  else if (conn->scroll_concur & SQL_SCCO_OPT_ROWVER) opt=SQL_CONCUR_ROWVER;
  else if (conn->scroll_concur & SQL_SCCO_LOCK)       opt=SQL_CONCUR_LOCK;
  else opt=SQL_CONCUR_READ_ONLY;
  retcode=SQLSetStmtOption(hSubStmt, SQL_CONCURRENCY, opt);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    odbc_stmt_error(inst->cdp, hSubStmt);
 /* create/open cursor: */
  if (really)
    retcode=SQLExecDirect(hSubStmt, (UCHAR*)cond, SQL_NTS);
  else
    retcode=SQLPrepare(hSubStmt, (UCHAR*)cond, SQL_NTS);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
  { odbc_stmt_error(inst->cdp, hSubStmt);
    corefree(cond);
    SQLFreeStmt(hSubStmt, SQL_CLOSE);  return (HSTMT)NOOBJECT;
  }
  if (really)
  { if (new_select) *new_select=cond;  
    else corefree(cond); 
  }
  else
    { corefree(cond);  SQLFreeStmt(hSubStmt, SQL_CLOSE);  hSubStmt=(HSTMT)1; }
  return hSubStmt;
}
#endif

uns32 create_qbe_subcurs(view_dyn * inst, BOOL really, view_dyn * dest_inst, tptr * new_select)
// Returns 0 for empty condition, NOOBJECT on error
{ tptr fullexpr;
  fullexpr=prep_qbe_subcurs(inst, dest_inst);
  if (fullexpr==NULL) return (uns32)NOOBJECT;
  if (!fullexpr[1])  /* empty definition */
    { corefree(fullexpr);  return 0; }
 /* Creates subcursor defined by fullexpr and frees fullexpr. Returns NOOBJECT on error. */
  uns32 result;
#if 0
  if (really) Set_status_text(MAKEINTRESOURCE(STBR_CURCON));
  if (dest_inst->dt->conn!=NULL)
  { //if (!really) { corefree(fullexpr);  return 1; } /* confirms that condition is OK */
    HSTMT hSubStmt=create_odbc_subcurs(fullexpr, dest_inst, really, new_select);
    result=(uns32)hSubStmt;
  }
  else
#endif
  { if (!really) *fullexpr=(uns8)QUERY_TEST_MARK;
    tcursnum newcurs, supercurs = dest_inst->dt->supercurs==NOOBJECT ?
      dest_inst->dt->cursnum : (tcursnum)dest_inst->dt->supercurs;
    if (cd_Open_subcursor(inst->cdp, supercurs, fullexpr, &newcurs))
      { newcurs=NOOBJECT;  /*Signalize(); --reported upper */ }
    result=newcurs;
  }
  if (really) wxGetApp().frame->SetStatusText(wxEmptyString);
  corefree(fullexpr);
  return result;
}

static BOOL restore_curs(view_dyn * inst)
{ 
  if (inst->dt->supercurs!=NOOBJECT)
  { close_dependent_editors(inst->cdp, inst->dt->cursnum, true);
    { cd_Close_cursor(inst->cdp, inst->dt->cursnum);
      inst->dt->cursnum=(tcursnum)inst->dt->supercurs;
    }
    inst->dt->supercurs=(uns32)NOOBJECT; /* for ODBC: signal, that view is not using a subcursor */
    return TRUE;
  }
  return FALSE;
}

/*******************************************************************************/
#define COPY_BUF_SIZE 4096
#define OW_IMPORT    12

CFNC DllPrezen BOOL WINAPI copy_from_file(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr,
                                                  uns16 index, FHANDLE hFile, int recode, uns32 offset)
{ tptr buf;  DWORD rd;  BOOL res=TRUE;  
  if (!(buf=sigalloc(COPY_BUF_SIZE, OW_IMPORT)))
    { CloseHandle(hFile);  return FALSE; }
  do
  { if (!ReadFile(hFile, buf, COPY_BUF_SIZE, &rd, NULL))
      { /*syserr(FILE_READ_ERROR); temp in 9*/  res=FALSE;  break; }
    if (!rd) break;
    if (recode) encode(buf, rd, TRUE, recode);
    if (cd_Write_var(cdp, tb, recnum, attr, index, offset, rd, buf))
      { cd_Signalize(cdp);  res=FALSE;  break; }
    offset+=rd;
  } while (rd==COPY_BUF_SIZE);
  cd_Write_len(cdp, tb, recnum, attr, index, offset);
  corefree(buf);
  CloseHandle(hFile);
  return res;
}

CFNC DllPrezen BOOL WINAPI old_copy_from_file(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr,
                                                  uns16 index, int handle, int recode)
{ tptr buf;  int rd;  uns32 off;  BOOL res=TRUE;  
  if (!(buf=sigalloc(COPY_BUF_SIZE, OW_IMPORT)))
    { _lclose(handle); return FALSE; }
  off=0;
  do
  { rd=_lread(handle, buf, COPY_BUF_SIZE);
    if (rd==-1)
      { /*syserr(FILE_READ_ERROR);temp 9*/  res=FALSE;  break; }
    if (!rd) break;
    if (recode) encode(buf, rd, TRUE, recode);
    if (cd_Write_var(cdp, tb, recnum, attr, index, off, rd, buf))
      { cd_Signalize(cdp);  res=FALSE;  break; }
    off+=rd;
  } while (rd==COPY_BUF_SIZE);
  cd_Write_len(cdp, tb, recnum, attr, index, off);
  corefree(buf);
  _lclose(handle);
  return res;
}
/****************************************************************************/
/************************* openning a view **********************************/
CFNC DllPrezen BOOL WINAPI analyze_base(cdp_t cdp, const char * p,
  char * basename, char * prefix, tobjnum * basenum, tcateg * cat, BOOL * dflt_view)
/* Analyses the view definition, returns base category (TABLE, CURSOR,
   DIRCURS, VIEW), allocated string containing the base name, base object
   number (tabnum, cursobj, cursnum or NOOBJECT) and the "default" sign. */
{ int i;  char basetype[11+1];  bool quoted = false;
  if (*p=='{')   // object descriptor skipped
  { while (*p && *p!='}') p++;
    if (*p=='}') p++;
  }
 // skip DEFAULT:
  while (*p && (unsigned char)*p<=' ') p++;             // leading spaces skipped 
  if (!strnicmp(p, "DEFAULT ", 8))  // must check trailing space, table name may start with "default..."
    { *dflt_view=TRUE;  p+=7; }
  else *dflt_view=FALSE;
 // read (possible prefixed) name:
  while (*p && (unsigned char)*p<=' ') p++;             // leading spaces skipped 
  i=0;
  if (*p=='`')
  { quoted=true;
    p++;
    while (*p && *p!='`') basename[i++] = *(p++);  
    if (*p=='`') p++;
  }
  else
    while (*p && *p!=' ' && *p!='.') basename[i++] = *(p++);  
  basename[i]=0;
  if (*p=='.')
  { p++;
    strcpy(prefix, basename);
    i=0;
    if (*p=='`')
    { p++;
      while (*p && *p!='`') basename[i++] = *(p++);  
      if (*p=='`') p++;
    }
    else
      while (*p && *p!=' ' && *p!='.') basename[i++] = *(p++);  
    basename[i]=0;
  }
  else *prefix=0;
 // analyse the base name: 
  if (*basename && !*prefix && !quoted) 
  { sig32 aux_base;  uns32 aux_ubase;  const char * p = basename;
    if (str2int(basename, &aux_base))   // base is a direct cursor number - signed form
      { *basenum=(tcursnum)aux_base;  *cat=CATEG_DIRCUR;  return TRUE; }
    if (str2uns(&p, &aux_ubase) && !*p)  // base is a direct cursor number - unsigned form 
      { *basenum=(tcursnum)aux_ubase;  *cat=CATEG_DIRCUR;  return TRUE; }
  }
 // read basetype:
  while (*p && ((unsigned char)*p<=' ')) p++;  /* skip the spaces foll. base name */
  i=0;  while ((i<11) && ((unsigned char)*p>' ')) basetype[i++]=*(p++);
  basetype[i]=0;
  if (!my_stricmp(basetype, "TABLEVIEW")) *cat=CATEG_TABLE;
  else if (!my_stricmp(basetype, "CURSORVIEW")) *cat=CATEG_CURSOR;
  else /* if (!my_stricmp(basetype, "DBDIALOG")) */   /* no base */
    *cat=CATEG_VIEW;  /* different */
 /* look for the base object: */
  *basenum=NOOBJECT;  /* view editor uses this */
  if (cdp)
    if (*cat==CATEG_TABLE || *cat==CATEG_CURSOR)
    { if (cd_Find_prefixed_object(cdp, prefix, basename, *cat, basenum))
        return FALSE; // viewedit may use the basename
    }
  return TRUE;
}
#if 0
static void recalc_view(view_stat * vdef)
{ WORD xm, ym;  int i;  t_ctrl * ctrl;
  xm=LOWORD(GetDialogBaseUnits());  /* divide by 8  */
  ym=HIWORD(GetDialogBaseUnits());  /* divide by 16 */
  if ((xm==8) && (ym==16)) return;
  vdef->vwX =(int)(vdef->vwX *(LONG)xm/8);
  vdef->vwY =(int)(vdef->vwY *(LONG)ym/16);
  vdef->vwCX=(int)(vdef->vwCX*(LONG)xm/8);
  vdef->vwCY=(int)(vdef->vwCY*(LONG)ym/16);
  ctrl=FIRST_CONTROL(vdef);
  for (i=0; i<vdef->vwItemCount; i++)
  { ctrl->ctX =(int)(ctrl->ctX *(LONG)xm/8);
    ctrl->ctY =(int)(ctrl->ctY *(LONG)ym/16);
    ctrl->ctCX=(int)(ctrl->ctCX*(LONG)xm/8);
    ctrl->ctCY=(int)(ctrl->ctCY*(LONG)ym/16);
    ctrl=NEXT_CONTROL(ctrl);
  }
}

CFNC DllPrezen void WINAPI resize_dlg_item(HWND hDlg, UINT ID)
{ UINT xm, ym, cx, cy;  HWND hCtrl;  POINT pt;  RECT Rect;
  xm=LOWORD(GetDialogBaseUnits());  /* divide by 8  */
  ym=HIWORD(GetDialogBaseUnits());  /* divide by 16 */
  if ((xm==8) && (ym==16)) return;
  hCtrl=GetDlgItem(hDlg, ID);
  GetWindowRect(hCtrl, &Rect);
  if (!hCtrl) return;
  pt.x=Rect.left;  pt.y=Rect.top;    ScreenToClient(hDlg, &pt);  Rect.left =pt.x;  Rect.top=pt.y;
  pt.x=Rect.right; pt.y=Rect.bottom; ScreenToClient(hDlg, &pt);  Rect.right=pt.x;  Rect.bottom=pt.y;
  cx=(int)((Rect.right-Rect.left) * (LONG)8/xm);
  cy=(int)((Rect.bottom-Rect.top) * (LONG)16/ym);
  MoveWindow(hCtrl, Rect.left+(Rect.right-Rect.left-cx)/2,
                    Rect.top +(Rect.bottom-Rect.top-cy)/2, cx, cy, FALSE);
}
#endif
static int err_object;
DllExport void WINAPI get_view_error(int * perr_object)
{ *perr_object=err_object; }

BOOL view_dyn::construct(const char * viewsource, tcursnum base, 
                         uns32 flags, HWND hParentIn)
{ tptr tmpsource;
  vwobjname[0]=0;
  while (*viewsource==' ') viewsource++;
  if (*viewsource=='*')
  { tobjnum objnum;
    do viewsource++; while (*viewsource==' ');
    strmaxcpy(vwobjname, viewsource, sizeof(tobjname));
    if (!find_view_object(cdp, vwobjname, &objnum))
      return FALSE;
    viewsource = tmpsource = cd_Load_objdef(cdp, objnum, CATEG_VIEW);
    if (tmpsource==NULL) { cd_Signalize(cdp);  return FALSE; }
  }
#if 0
  else if (*viewsource=='+')   /* file stored view */
  { int handle;  long len;
    if ((handle=_lopen(viewsource+1, OF_READ)) == -1)
    { errbox(MAKEINTRESOURCE(BINDFILE_NOT_FOUND));
      return FALSE;
    }
    len=_llseek(handle, 0, 2);  /* under NT filelength doesn't exist and _filenegth doesn't work */
    _llseek(handle, 0, 0);
    if (len<=0) { _lclose(handle);  return FALSE; }
    viewsource = tmpsource = sigalloc(len+1, 99);
    if (tmpsource==NULL) { _lclose(handle);  return FALSE; }
    _lread(handle, tmpsource, len);  tmpsource[len]=0;
    _lclose(handle);
  }
#endif
  else tmpsource=NULL;  /* direct view */
  BOOL res = create_vwstat(viewsource, base, flags, hParentIn);
  corefree(tmpsource);  // NULL for direct view
  return res;
}

#ifdef STOP
  { view_dyn * inst = flags & QUERY_VIEW ? (view_dyn *)new qbe_vd(cdp) : (view_dyn *)new basic_vd(cdp);
    if (inst==NULL) return 0;
    if (!inst->open_on_desk(viewsource, base, flags, pos, bindl,
                hParent, viewid!=NULL ? viewid : &aux_hWnd, NULL)) return 0;
    HWND hWnd = viewid!=NULL ? *viewid : aux_hWnd;
    return !hWnd ? (HWND)1 : hWnd;
  }
#endif

BOOL view_dyn::create_vwstat(const char * viewsource, tcursnum base, uns32 flags, HWND hParentIn)
{ tobjnum basenum;  tcateg cat;  BOOL dflt_view=FALSE;
  char basename[MAX_ODBC_NAME_LEN+1], baseprefix[MAX_ODBC_NAME_LEN+1];  // ODBC
  BOOL modal_parent=FALSE;
  view_stat * vw_stat;  int res;  t_tb_descr * tb;  tcursnum cursnum;

 // analyse parameters, update flags depending on parent:
  kont_descr compil_kont, *pkntx;  int i;
  if (!base) base=NOOBJECT; // masking a common programming error
 // init members:
  vwOptions    =(flags & 0xffff) << 16;
  cursnum      =base;
  hParent      =hParentIn;
  query_mode   =(BYTE)((flags & QUERY_VIEW) ? QUERY_QBE   :
                         ((flags & ORDER_VIEW) ? QUERY_ORDER : NO_QUERY));
 /* create & initialize view object */
  if (flags & PARENT_CURSOR)
    if (par_inst!=NULL && par_inst->dt->cursnum!=NOCURSOR)
    { base=par_inst->dt->cursnum;  
      if (!(flags & IS_SUBVIEW) && !modal_parent) hParent=0;
      /* parent should not be disabled */
    }
    else 
      flags &= ~(LONG)PARENT_CURSOR;
    // PARENT_CURSOR ignored if parent not specif, is not a view or is variables-only view
  orig_flags   =flags;
#if 0
  next_view=cdp->all_views;  cdp->all_views=this;
#endif
  if (flags & (QUERY_VIEW | ORDER_VIEW) && viewsource==NULL)
  { if (par_inst==NULL) goto errex0;
    /* use parent as the template */
    view_stat * par_stat;  t_ctrl * ctrl;
    unsigned stat_size;
   /* use view description */
    par_stat=par_inst->stat;
    stat_size=sizeof(view_stat)+par_stat->vwCodeSize;
    ctrl=FIRST_CONTROL(par_stat);
    for (i=0; i<par_stat->vwItemCount; i++)
    { stat_size+=sizeof(t_ctrl)+ctrl->ctCodeSize;
      ctrl=NEXT_CONTROL(ctrl);
    }
    stat=vw_stat=(view_stat*)sigalloc(stat_size, OW_PREZ);
    if (!vw_stat) goto errex0;
    memcpy(vw_stat, par_stat, stat_size);
   /* update the dimensions */
#if 0
    RECT Rect;  POINT pt;  
    GetWindowRect(hParent, &Rect);
    /*GetWindowRect(cdp->RV.hClient, &RectC);*/
    vw_stat->vwCX=Rect.right-Rect.left;
    vw_stat->vwCY=Rect.bottom-Rect.top;
    if ((flags & IS_SUBVIEW) || (flags & VIEW_OLECTRL))
    {
        vw_stat->vwX=-1;
        vw_stat->vwY=-1;
    }
    else
    {
        pt.x=Rect.left;  pt.y=Rect.top;
        ScreenToClient(cdp->RV.hClient, &pt);
        vw_stat->vwX=pt.x;
        vw_stat->vwY=pt.y;
    }
#endif
    // U popupu to bude asi jeste jinak
   /* set otehr inst values */
    cursnum=NOOBJECT;
    vwOptions |= par_inst->vwOptions &   /* copy basic options */
      (SIMPLE_VIEW | REP_VW_STYLE_MASK | GP_MASK | SUPPRESS_BACK);
    stat->vwStyle3 &= ~(int)REC_SELECT_FORM;
  }
  else
  {/* preparation of kontext */
    pkntx=&compil_kont;  /* compilation kontext (may be set to NULL later) */
    if (base==NOOBJECT)  /* base not specified, use the one from viewsource */
    { if (!analyze_base(cdp, viewsource, basename, baseprefix, &basenum, &cat, &dflt_view))
      { if (cdp)
          if (cd_Sz_error(cdp)==OBJECT_NOT_FOUND) 
            error_box(wxT("Form cannot be open because its table\nor query does not exist."));
          else cd_Signalize(cdp);
        goto errex0;
      }
      compil_kont.kontcateg=cat;
      if (!cdp)
      { compil_kont.kontcateg=CATEG_D_TABLE;
        if (cat==CATEG_DIRCUR)
          compil_kont.record_tobj=make_result_descriptor9(xcdp->get_odbcconn(), (HSTMT)(size_t)basenum);
        else
          compil_kont.record_tobj=make_odbc_descriptor9(xcdp->get_odbcconn(), basename, baseprefix, TRUE);
      }
      else if (cat==CATEG_CURSOR)  /* open the view cursor */
      {
#if 0
        Set_status_text(MAKEINTRESOURCE(STBR_CURCON));
#endif
        if (cd_Open_cursor(cdp, basenum, &base))
        { cd_Signalize(cdp);  /* cursor close flag not set yet */
          wxGetApp().frame->SetStatusText(wxEmptyString);
          goto errex0;
        }
        wxGetApp().frame->SetStatusText(wxEmptyString);
        vwOptions|=VIEW_AUTO_CURSOR;
        compil_kont.kontcateg=CATEG_DIRCUR;   /* CURSOR -> DIRCUR */
      }
      else if (cat==CATEG_TABLE)
        base=basenum;
      else if (cat==CATEG_DIRCUR)
        base=basenum;
      else  /* cat==CATEG_VIEW */
        pkntx=NULL;   /* no kontext for the compilation */
    }
    else /* a base given: table or an open cursor */
      compil_kont.kontcateg=(tcateg)(IS_CURSOR_NUM(base) ? CATEG_DIRCUR : CATEG_TABLE);
    compil_kont.kontobj=cursnum=base;

   /* access to definition not checked, but should be */
    if (dflt_view)  /* then basename valid, must free it */
    { int xpos=0, ypos=0; // POINT pt = { 8, 8 };
//      if (hParent)
//      { 
//        if (pt.x<0) pt.x=0;  if (pt.y<0) pt.y=0;
//        ext=GetSystemMetrics(SM_CXFULLSCREEN);  if (pt.x > ext-32) pt.x=ext-32;  
//        ext=GetSystemMetrics(SM_CYFULLSCREEN);  if (pt.y > ext-32) pt.y=ext-32;
//        if (!IsPopup() && cdp->RV.hClient) 
//        { ClientToScreen(hParent, &pt);
//          ScreenToClient(cdp->RV.hClient, &pt);
//        }
//        xpos=pt.x;  ypos=pt.y;
//      }
      viewsource=initial_view(xcdp, xpos, ypos, VIEWTYPE_SIMPLE, basename, cat, basenum, baseprefix);
      if (!viewsource) goto errex0;
    }
    compil_info xCI(cdp, viewsource, MEMORY_OUT, view_def_comp);
    compil_kont.next_kont=NULL;
    xCI.kntx=pkntx;  // compil_kont or NULL
    xCI.thisform=1;  // defined by set_this_form_source(cdp, viewsource);
    if (!destroy_on_close) xCI.comp_flags |= COMP_FLAG_DISABLE_PROJECT_CHANGE; // in-code forms must not change project!
    const char * prev = set_this_form_source(cdp, viewsource);
    res=compile(&xCI);
    set_this_form_source(cdp, prev);
    if (dflt_view) corefree((tptr)viewsource);
    aggr=xCI.aggr;
    err_object=xCI.err_object;
    if (res) 
    { if (cdp) client_error(cdp, res);
      goto errex0;
    }
   /* add stat to inst */
    stat=vw_stat=(view_stat *)xCI.univ_code;
    vwOptions |= stat->vwStyle;
    if (stat->vwStyle & VIEW_MODAL_VIEW)
      if (view_mode==VWMODE_MDI) view_mode=VWMODE_MODAL;
      else vwOptions &= ~VIEW_MODAL_VIEW;
    if (stat->vwStyle3 & MODELESS_VIEW3)
      if (view_mode==VWMODE_MDI) view_mode=VWMODE_MODELESS;
#if 0
    transform_coords(stat, TRUE, FALSE);
#endif
   // remove fictive record etc. from views into joined tables:
    if (cdp)
      if (base!=NOOBJECT)
      { const d_table * td = cd_get_table_d(cdp, base, compil_kont.kontcateg);
        if (td != NULL)
        { if (!(td->updatable & QE_IS_INSERTABLE))
            stat->vwHdFt |= NO_VIEW_FIC | NO_VIEW_INS;
          if (!(td->updatable & QE_IS_UPDATABLE))
            { vwOptions |= NO_VIEW_EDIT; info_flags |= INFO_FLAG_NONUPDAT; }
         // setting info-flags:
          if (td->tabdef_flags & TFL_TOKEN)       info_flags |= INFO_FLAG_TOKENS;
          if (td->tabdef_flags & TFL_REC_PRIVILS) info_flags |= INFO_FLAG_RECPRIVS;
          release_table_d(td);
        }
      }
      else // set the "single record" option
        vwOptions &= ~REP_VW_STYLE_MASK;
   /* load the private toolbar bitmaps: must be done before stat=vw_stat */
    tb=(t_tb_descr *)get_var_value(&vw_stat->vwCodeSize, VW_TOOLBAR);
    if (tb) designed_toolbar = new(tb) t_designed_toolbar(tb);
   /* set the flag if openning special query view: */
    if (flags & (QUERY_VIEW | ORDER_VIEW)) /* openning independent query view */
    { if (par_inst==NULL) goto errex0;
      vwOptions |= VIEW_INDEP_QBE;
      if (cat==CATEG_CURSOR && cursnum!=NOOBJECT) // close own cursor of the indep. query view
        { cd_Close_cursor(cdp, cursnum);  cursnum=NOOBJECT; }
    }
  }

 // read the enumerations (must be before openning the view), both for normal and query views:
 // define the combo type:
  { t_ctrl * ctrl;
    for (i=0, ctrl=FIRST_CONTROL(stat);  i<stat->vwItemCount;
         i++, ctrl=NEXT_CONTROL(ctrl))
    { if (ctrl->ctClassNum==NO_EC || ctrl->ctClassNum==NO_COMBO)
      { if (ctrl->type)
        { 
          if (ctrl->type==ATT_INT32 || ctrl->type==ATT_INT16 || ctrl->type==ATT_CHAR || ctrl->type==ATT_INT8)
            ctrl->ctClassNum=NO_COMBO;
          else //if (is_string(ctrl->type)) and other types
          { ctrl->ctClassNum=NO_EC;
            //ctrl->key_flags=DEFKF_EDIT;
          }
        }
      }
      if (get_var_value(&ctrl->ctCodeSize, CT_COMBOSEL) != NULL)
      { enum_cont * enm = new enum_cont(this, ctrl->ctId);
        enm->enum_reset(this, TRUE, NULL);
      }
    }
  }

 /************************** prepare QBE conditions *************************/
  if (flags & (QUERY_VIEW | ORDER_VIEW | PARENT_CURSOR))
  // par_inst defined, checked for all variants
  { qbe=par_inst->qbe;
    qbe->AddRef();
  }
  else qbe=new t_qbe;
 /***************************** start caching: ******************************/
  if ((!cdp || cursnum!=NOOBJECT) && !(flags & (QUERY_VIEW | ORDER_VIEW)))
  { if (flags & PARENT_CURSOR && !(vw_stat->vwHdFt & FORM_OPERATION))
    { dt=par_inst->dt;
      dt->AddRef();
    }
    else // create a new ltable (with the initial cache):
    { 
      if (!cdp)
      { //make_ext_odbc_name(base_tc, tabname, SQL_QU_DML_STATEMENTS);
        if (cat==CATEG_DIRCUR)
          dt=create_cached_access_for_stmt(xcdp->get_odbcconn(), (HSTMT)(size_t)basenum, (const d_table *)compil_kont.record_tobj);
        else
          dt=create_cached_access9(NULL, xcdp->get_odbcconn(), NULL, basename, baseprefix, NOOBJECT);
      }
      else
        dt=create_cached_access9(cdp, NULL, NULL, NULL, NULL, cursnum);
      if (dt==NULL) goto errex0;
      if (vwOptions & VIEW_AUTO_CURSOR) dt->close_cursor=TRUE;
    }
  }
  else
  { dt=new ltable(cdp, NULL);  // fictive ltable with cursnum==NOOBJECT
    if (dt==NULL) goto errex0;
  }

  if (flags & NO_INSERT) stat->vwHdFt |= NO_VIEW_INS | NO_VIEW_FIC;
#if 0
  if (view_mode!=VWMODE_PRINT) recalc_view(stat);
//  if (vw_stat->vwHdFt & FORM_OPERATION)   /* input form implies */ -- done in comp2.c
//    vwOptions &= ~REP_VW_STYLE_MASK;   /* only 1 record per screen! */
  if (flags & (QUERY_VIEW | ORDER_VIEW))  /* modify the controls */
  { t_ctrl * ctrl;
    for (i=0, ctrl=FIRST_CONTROL(vw_stat);  i<vw_stat->vwItemCount;
         i++, ctrl=NEXT_CONTROL(ctrl))
      if (ctrl->ctClassNum==NO_VAL && !get_enum_def(this, ctrl) ||
          ctrl->ctClassNum==NO_TEXT || ctrl->ctClassNum==NO_RTF || ctrl->ctClassNum==NO_LIST)
      { ctrl->ctClassNum=NO_EDI;
        ctrl->ctStyle=(ctrl->ctStyle & 0xffffff00L) | (ES_AUTOHSCROLL | WS_BORDER | ES_LEFT);
        ctrl->key_flags |= KF_LEFTRIGHT | KF_UPDOWN | KF_HOMEEND | KF_PGUPDN;
      }
  }

 /* prepare opening */
  if (flags & QUERY_VIEW)
  { dt->rm_int_recnum=dt->rm_ext_recnum=dt->fi_recnum=10;
    rec_counted=TRUE;
    if (!viewsource) sel_itm=par_inst->sel_itm;
    vwOptions|=VIEW_DEL_RECS;   /* fuse, rec_counted should make this ignored */
  }
  else if (flags & ORDER_VIEW)
  { dt->rm_int_recnum=dt->rm_ext_recnum=dt->fi_recnum=1;
    rec_counted=TRUE;
    if (!viewsource) sel_itm=par_inst->sel_itm;
    vwOptions|=VIEW_DEL_RECS;   /* fuse, rec_counted should make this ignored */
  }
  else 
#endif
  if (vw_stat->vwHdFt & FORM_OPERATION)   /* input form */
  { dt->rm_int_recnum=dt->rm_ext_recnum=0;  dt->fi_recnum=1;
    rec_counted=TRUE;
    vwOptions|=VIEW_DEL_RECS;
    if (flags & PARENT_CURSOR) insert_inst=par_inst;
  }
  else /* normal view */
  { rec_counted=compil_kont.kontcateg==CATEG_VIEW;
    if (rec_counted)
      vwOptions|=VIEW_DEL_RECS; /* fuse, rec_counted should make this ignored */
   /* if necessary, allocate & inits. the rec_remap */
    if (remap_init(this, cdp && !(vwOptions & VIEW_DEL_RECS)))
      vwOptions |= VIEW_DEL_RECS;   /* cannot remap */
    if (rec_counted)   /* must be after remap_init */
    { dt->rm_int_recnum=dt->rm_ext_recnum=dt->fi_recnum=1;
      dt->rm_completed=true;
    }
    if (vwOptions & VIEW_COUNT_RECS) view_recnum(this);
  }
  if (!cdp && compil_kont.kontcateg==CATEG_D_TABLE)
    if (cat!=CATEG_DIRCUR)  // unless td us owned by the dt
      corefree(compil_kont.record_tobj);  // releasing td
  return TRUE;
 errex0: // base not stored yet
  if ((vwOptions & VIEW_AUTO_CURSOR)!=0 && base!=NOOBJECT) // base given as parameter or cursor opened here
    cd_Close_cursor(cdp, base);
  destruct(); // releases allocated data
  if (!cdp && compil_kont.kontcateg==CATEG_D_TABLE)
    if (cat!=CATEG_DIRCUR)  // unless td us owned by the dt
      corefree(compil_kont.record_tobj);  // releasing td
  return FALSE;
}

BOOL view_dyn::position_it(trecnum pos)  // must be called after defining "bind"
// position in the unopened view:
{ if (!(orig_flags & (QUERY_VIEW|ORDER_VIEW)) && !(stat->vwHdFt & FORM_OPERATION))
  { if ((sig32)pos < 0)
    { pos=exter2inter(this, -(sig32)pos);
      if (pos==NORECNUM) pos=0;
    }
    toprec=sel_rec=pos;
  }
  else
    toprec=sel_rec=0; // mapped on the fictive record in the cache if FORM_OPERATION
  return TRUE;
}

BOOL view_dyn::open_on_desk(const char * viewsource, tcursnum base, uns32 flags, trecnum pos,
     bind_def * bindl, HWND hParentIn, HWND * viewid, tptr viewnameIn)
// IS_SUBVIEW must not be specified when openning the ole control (or its qbe)
{ if (viewnameIn!=NULL) strmaxcpy(vwobjname, viewnameIn, OBJ_NAME_LEN+1);
  if (!create_vwstat(viewsource, base, flags, hParentIn)) return FALSE;
 // locking source table definition:
  if (!dt->conn && dt->cursnum != NOOBJECT)
    if (!IS_CURSOR_NUM(dt->cursnum)) 
      if (cd_Read_lock_record(cdp, TAB_TABLENUM, dt->cursnum)) { cd_Signalize(cdp);  return FALSE; }
      else table_def_locked=true;

  bind         =bindl;
  if (orig_flags & PARENT_CURSOR) pos=par_inst->sel_rec;
  if (!position_it(pos)) return FALSE;
 /* openning the view window */
#if 0
  if (!open(viewid)) return FALSE;
#endif
  return TRUE;
}

void view_dyn::view_closed(void) // called on WM_NCDESTROY
{ 
#if 0
  post_notif(hWnd, NOTIF_DESTROY);
  SetWindowLong(hWnd, 0, 0);
#endif
  if (idadr!=NULL) *idadr=NULL;
  hWnd=0;
  if (destroy_on_close) delete this;
}


void view_dyn::close(void)
{ 
#if 0
  if (hWnd)
    SendMessage(hWnd, WM_CLOSE, 12345, 0);  /* suitable for modal views too */
    /* PostMessage does not work! */
#endif
}

void view_dyn::resize_view(int xdisp)
// Creates or destroys or moves controls according to size & displacement, resets then.
// xdisp - new x displacement
#if 0
{ int riw, vrec, maxrec, i;  HWND hCtrl;  t_ctrl * itm;
  HCURSOR oldcurs=SetCursor(LoadCursor(NULL, IDC_WAIT));
  int linesize=div_line_width(this);
 /* count the new number of records */
  if (!(vwOptions & REP_VW_STYLE_MASK)) riw=1;  // single record per form
  else
  { int data_pane_space = ClientRect.y-yrecoff-yfooter;
    riw=data_pane_space / recheight;
    if (riw <= 0)
       riw = 1;
    else if (!yfooter && data_pane_space % recheight > 4) riw++;
  }
  cache_resize(this, riw); // stores riw to recs_in_win (deleted controls will not use its cache, I hope)
  maxrec = riw>recs_in_win ? riw : recs_in_win;

 // creating and deleting controls:
  for (i=0, itm=FIRST_CONTROL(stat);  i<stat->vwItemCount;  i++, itm=NEXT_CONTROL(itm))
  { UINT x, y, cx, cy;
    x=itm->ctX + xrecoff - xdisp;  y=itm->ctY;  cx=itm->ctCX;  cy=itm->ctCY;
    BOOL ctrl_shall=itm->ctX+itm->ctCX > xdisp && itm->ctX < xdisp+ClientRect.x;  // the control should exist
    if (itm->pane==PANE_DATA)
    { for (vrec=0; vrec<maxrec; vrec++)  /* deleting children */
      { hCtrl=GetDlgItem(hWnd, MAKE_FID(itm->ctId,vrec));
        if (hCtrl) // control exists
        { if (ctrl_shall && vrec < riw || itm->ctClassNum==NO_GRAPH || itm->ctClassNum==NO_VIEW)
            SetWindowPos(hCtrl, 0, x, y + yrecoff + linesize + vrec * recheight,
                         cx, cy, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOREDRAW);
            else DestroyWindow(hCtrl);
        }
        else /* has not existed yet */
          if (ctrl_shall && vrec < riw)
            create_control(hWnd, this, itm, yrecoff+linesize + vrec * recheight, vrec, i, xdisp);
          /* else it doesn't exist & will not */
      }
    }
    else
      if (itm->pane==PANE_PAGESTART || itm->pane==PANE_PAGEEND)
      { int yoff = itm->pane==PANE_PAGEEND ? ClientRect.y-yfooter : 0;
        hCtrl=GetDlgItem(hWnd, itm->ctId);
        if (hCtrl) // control exists
          if (ctrl_shall || itm->ctClassNum==NO_GRAPH || itm->ctClassNum==NO_VIEW)
            SetWindowPos(hCtrl, 0, x, yoff+y, cx, cy, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOREDRAW);
          else DestroyWindow(hCtrl);
        else /* has not existed yet */
          if (ctrl_shall)
            create_control(hWnd, this, itm, yoff, 0, i, xdisp);
      }
  }
 /* creating (and deleting) binding buttons: */
  if (bind)
  { for (vrec=0; vrec<maxrec; vrec++)  /* deleting children */
    { hCtrl=GetDlgItem(hWnd, MAKE_FID(BINDID,vrec));
      if (hCtrl)
      { if (vrec >= riw || xdisp>=BINDSZX)
          DestroyWindow(hCtrl);
        else
          SetWindowPos(hCtrl, 0, -xdisp, yrecoff + linesize + vrec * recheight,
              BINDSZX, BINDSZY, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOREDRAW);
      }
      else
        if (vrec < riw && (xdisp<BINDSZX))
          CreateWindow(BUTTON, NULL,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_OWNERDRAW,
            -xdisp, yrecoff+linesize + vrec * recheight,
            BINDSZX, BINDSZY, hWnd, (HMENU)MAKE_FID(BINDID,vrec), hPrezInst, NULL);
    }
  }

 // moving tab control back:
  for (i=0, itm=FIRST_CONTROL(stat);  i<stat->vwItemCount;
       i++, itm=NEXT_CONTROL(itm))
    if (itm->ctClassNum==NO_TAB)
      for (vrec=0;  vrec<riw;  vrec++)  /* deleting children */
      { hCtrl=GetDlgItem(hWnd, MAKE_FID(itm->ctId,vrec));
        if (hCtrl)
          SetWindowPos(hCtrl, HWND_BOTTOM, 0, 0, 0, 0,
                       SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW);
      }

 /* store new view size & displacement, reload values */
  hordisp=xdisp;
  InvalidateRect(hWnd, NULL, TRUE);
  SCROLLINFO scroll_info;
  scroll_info.cbSize=sizeof(SCROLLINFO);
  scroll_info.fMask=SIF_POS;
  scroll_info.nPos =xdisp;
  SetScrollInfo(hWnd, SB_HORZ, &scroll_info, TRUE);
// Reset_view should be called always, in order to be able to select the proper radio button when openning the form
  Reset_view(hWnd, NORECNUM, RESET_ALL);
  if (openning_window==TRUE) openning_window=2;
  SetCursor(oldcurs);
}
#endif
{}

bool view_dyn::accept_query(tptr afilter, tptr aorder)
{ int Len = 0, fLen = 0, oLen = 0;
  if (afilter)
    Len = fLen = (int)strlen(afilter);
  if (aorder)
  { oLen = (int)strlen(aorder);
    Len += 1 + 9 + oLen;  // delimiter 1, "ORDER BY ", aorder
  }
  if (Len)
  { tptr Query = sigalloc(Len + 1, 35);
    if (!Query)
      return false;
    if (afilter)
      memcpy(Query, afilter, fLen + 1);
    if (aorder)
    { Query[fLen] = 1;
      memcpy(Query + fLen + 1, "ORDER BY ", 9);
      strcpy(Query + fLen + 1 + 9, aorder);
    }
    if (cdp)
    { tcursnum newcurs;
      tcursnum supercurs = dt->supercurs == NOOBJECT ? dt->cursnum : (tcursnum)dt->supercurs;
      BOOL Err = cd_Open_subcursor(cdp, supercurs, Query, &newcurs);
      corefree(Query);
      if (Err)
      { cd_Signalize(cdp);
        return false;
      }
      if (!newcurs) restore_curs(this);  /* empty definition */
      else if (newcurs!=NOOBJECT)
      { if (dt->conn!=NULL)
        { if (dt->supercurs==NOOBJECT)
            dt->supercurs=(uns32)(size_t)dt->hStmt;
          dt->hStmt=(HSTMT)(size_t)newcurs;
         // update the stored select statement:
          if (upper_select_st) corefree(dt->select_st);
          else upper_select_st=dt->select_st;
          dt->select_st=NULL;
        }
        else
        { if (dt->supercurs==NOOBJECT) dt->supercurs=dt->cursnum;
          else cd_Close_cursor(cdp, dt->cursnum);
          dt->cursnum=(tcursnum)newcurs;
        }
      }
    }
    else
    { int lc  = (int)strlen(dt->select_st);
      int Len = 15 + lc + 1 + 7 + fLen + 30 + oLen + 1;
      Query   = (char *)sigalloc(Len, 35);
      if (!Query)
         return false;
      strcpy(Query, "SELECT * FROM (");
      char *pq = Query + 15;
      memcpy(pq, dt->select_st, lc);
      pq += lc;
      memcpy(pq, ") AS ORIGTAB", 12);  pq+=12;
      if (afilter)
      { memcpy(pq, " WHERE ", 7);
        pq += 7;
        memcpy(pq, afilter, fLen);
        pq += fLen;
      }
      if (aorder)
      { memcpy(pq, " ORDER BY ", 10);
        pq += 10;
        memcpy(pq, aorder, oLen);
        pq += oLen;
      }
      *pq = 0;
      odbc_reset_cursors(this, Query);
    }
  }
  else if (cdp)
    restore_curs(this);
  else
  { odbc_reset_cursors(this, dt->select_st);
  }
  if (filter != afilter)
  { corefree(filter);
    filter = afilter;
  }
  if (order != aorder)
  { corefree(order);
    order = aorder;
  }
  return true;
}

static char user_name3[sizeof(t_user_ident)+5];

#if 0
CFNC DllPrezen char * WINAPI Next_user_name(cdp_t cdp, HWND hView, trecnum erec)
{ *user_name3=0;
  if (IsView(hView))
  { view_dyn * inst = GetViewInst(hView);
    if (erec!=NORECNUM)
    { d_attr info;
      if (find_attr(cdp, inst->dt->cursnum, CATEG_TABLE, "_W5_DOCFLOW", NULL, NULL, &info))
      { tattrib attr=info.get_num();
        cd_GetSet_next_user(cdp, inst->dt->cursnum, erec, attr, OPER_GET, VT_NAME3, user_name3);
      }
    }
  }
  return user_name3;
}
#endif

CFNC DllPrezen int WINAPI sig_state(cdp_t cdp, t_signature * sig, int length)
// Used by Signature_state function and method
{ if (length != sizeof(t_signature))
    return length ? -(int)sigstate_error : -(int)sigstate_notsigned;
  if (sig->state!=sigstate_valid) return -(int)sig->state;
  tobjnum objnum;  WBUUID user_uuid;
  if (cd_Find_object_by_id(cdp, sig->key_uuid, CATEG_KEY, &objnum))
    return -(int)sigstate_error;
  if (cd_Read(cdp, KEY_TABLENUM, objnum, KEY_ATR_USER, NULL, user_uuid))
    return -(int)sigstate_error;
  if (cd_Find_object_by_id(cdp, user_uuid, CATEG_USER, &objnum))
    return -(int)sigstate_error;
  return objnum;
}

#if 0
CFNC DllPrezen int WINAPI Signature_state(cdp_t cdp, HWND hView, trecnum irec, const char * attrname)
{ char aname[ATTRNAMELEN+1];
  strmaxcpy(aname, attrname, sizeof(aname));  Upcase9(cdp, aname);
  if (!IsView(hView)) return -(int)sigstate_error;
  view_dyn * inst = GetViewInst(hView);
  trecnum erec = (sig32)irec < 0 ? -(sig32)irec : inter2exter(inst, irec);
  if (erec==NORECNUM) return -(int)sigstate_error;
  d_attr info;
  if (!find_attr(cdp, inst->dt->cursnum, CATEG_TABLE, aname, NULL, NULL, &info))
    return -(int)sigstate_error;
  tattrib attr=info.get_num();
  t_signature sig;  uns32 rd;
  if (cd_Read_var(cdp, inst->dt->cursnum, erec, attr, NOINDEX, 0, sizeof(t_signature), &sig, &rd))
    return -(int)sigstate_error;
  return sig_state(cdp, &sig, rd);
}

CFNC DllPrezen int WINAPI Tab_page(HWND hView, int ID)
// Returns -1 on error or 0-based selected page number of the ID tab control.
// If ID is not a tab control, returns info about the last tab control.
{ if (!IsView(hView)) return -1;
  view_dyn * inst = GetViewInst(hView);
 // find the control:
  t_ctrl * itm, * itmanytab = NULL, * itmthetab = NULL;  int i;
  for (i=0, itm=FIRST_CONTROL(inst->stat);  i<inst->stat->vwItemCount;
       i++, itm=NEXT_CONTROL(itm))
    if (itm->ctClassNum==NO_TAB)
      if (itm->ctId==ID) itmthetab=itm;
      else               itmanytab=itm;
  if (itmthetab==NULL)
    if (itmanytab==NULL) return -1;
    else itmthetab=itmanytab;
 // get control state:
  HWND hCtrl=GetDlgItem(hView, MAKE_FID(itmthetab->ctId, inst->sel_rec-inst->toprec));
  if (!hCtrl) return /*-1;*/ 0;  // 0 used when printing view with tab control
  return TabCtrl_GetCurSel(hCtrl);
}
#endif
////////////////////////////// combo filling ////////////////////////////
DllPrezen WINAPI enum_cont::enum_cont(view_dyn * inst, UINT ctIdIn)
{ defin=NULL;  ctId=ctIdIn;  next=inst->enums;  inst->enums=this; }

typedef struct
{ char  strbuf[256];
  uns32 numbuf;
} c_buf;

static BOOL same_type(int t1, int t2)
{ return t1==t2 || t1==ATT_INT16 && t2==ATT_INT32 ||
                   t1==ATT_INT32 && t2==ATT_INT16;
}

DllPrezen void WINAPI enum_cont::enum_reset(view_dyn * inst, BOOL initial, tptr query)
{ tcursnum curs;  const d_table * td;  BOOL bad = FALSE;  BOOL transl;  int tp2;

  t_ctrl * itm=get_control_def(inst, ctId);
  if (query!=NULL)
    { while (*query==' ') query++;  if (!*query) query=NULL; }
  tptr def = query==NULL ? get_var_value(&itm->ctCodeSize, CT_COMBOSEL) : query;
  if (def==NULL) return;
  while (*def==' ') def++;
  if (!strnicmp(def, "SELECT", 6))
  { if (cd_Open_cursor_direct(inst->cdp, def, &curs))
      { cd_Signalize(inst->cdp);  return; }
  }
  else   /* load the cursor definition */
  { tobjnum objnum;  cutspaces(def);
    if (*def=='`') *def=' ';
    if (*def && def[strlen(def)-1]=='`') def[strlen(def)-1]=' ';
    cutspaces(def);
    if (cd_Find_object(inst->cdp, def, CATEG_CURSOR, &objnum))
      { cd_Signalize(inst->cdp);  return; }
    if (cd_Open_cursor(inst->cdp, objnum, &curs))
      { cd_Signalize(inst->cdp);  return; }
  }
 /* analyse the cursor */
  t_specif str_specif;
  td=cd_get_table_d(inst->cdp, curs, CATEG_DIRCUR);
  if (!td) bad=TRUE;
  else
  { if (td->attrcnt < 2) bad=TRUE;
    else
    { transl = itm->ctClassNum==NO_COMBO;
      const d_attr * a_d=ATTR_N(td,1);
      str_specif = a_d->specif;
      if (initial)
      { if (!is_string(a_d->type) || a_d->multi!=1) bad=TRUE;
        else
        { if (transl)
          { a_d = NEXT_ATTR(td, a_d);  tp2=a_d->type;
            if (tp2!=ATT_CHAR && tp2!=ATT_INT16 && tp2!=ATT_INT32 && tp2!=ATT_INT8 ||
                !same_type(tp2, itm->type) || a_d->multi!=1) bad=TRUE;
          }
        }
      } // initial
    }
    release_table_d(td);
  }

  if (bad)
  { cd_Close_cursor(inst->cdp, curs);
    client_error(inst->cdp, COMBO_SELECT);  cd_Signalize(inst->cdp);  return;
  }
  else  /* cursor is OK */
  { trecnum rec, reccnt;  t_flwstr enmdf(60,100);
    if (!cd_Rec_cnt(inst->cdp, curs, &reccnt))
    { if (reccnt>4000) reccnt=4000;
      int combo_step=transl ? MAX_PACKAGED_REQS/2 : MAX_PACKAGED_REQS;
      if (combo_step>20) combo_step=20;
      c_buf * bufs = (c_buf*)corealloc(sizeof(c_buf)*combo_step, 51);
      if (bufs!=NULL)
      { rec=0;
        while (rec<reccnt)
        { int this_cnt=0;  cd_start_package(inst->cdp);
          do
          {   cd_Read(inst->cdp, curs, rec, 1, NULL,        bufs[this_cnt].strbuf);
            if (transl)
            { bufs[this_cnt].numbuf=0;   /* may read Char! */
              cd_Read(inst->cdp, curs, rec, 2, NULL, (tptr)&bufs[this_cnt].numbuf);
            }
            this_cnt++;  rec++;
          } while (this_cnt<combo_step && rec<reccnt);
          cd_send_package(inst->cdp);
          if (!cd_Sz_error(inst->cdp))
            for (int j=0;  j<this_cnt;  j++)
            /* records with empty strings are omitted, otherwise they would end the list */
              if (bufs[j].strbuf[0])
              {// convert: 
                char convbuf[MAX_FIXED_STRING_LENGTH+2];
                superconv(ATT_STRING, ATT_STRING, bufs[j].strbuf, convbuf, str_specif, wx_string_spec, NULL);
                enmdf.put((wchar_t*)convbuf);
                enmdf.putn(L"", 1);
                sig32 val = 0;
                if (transl)
                { val = bufs[j].numbuf;
                  if (tp2==ATT_INT16) // sign extension for Short
                    val=(sig32)(sig16)val;
                }
                enmdf.putn((wchar_t*)&val, sizeof(sig32)/sizeof(wchar_t)); /* undefined iff !transl */
              }
        }
        corefree(bufs);
      }
    }
    enmdf.putn(L"", 1);
   // replace enumdef:
    corefree(defin);
    defin=enmdf.get();  enmdf.unbind();
  }
  cd_Close_cursor(inst->cdp, curs);
}

#if 0
DllPrezen void WINAPI enum_cont::enum_assign(tptr newdef)
{ corefree(defin);  defin=newdef; }
#endif
////////////////////////////// compile view /////////////////////////////////////
#if 0
BOOL check_view(cdp_t cdp, const char * viewsource)
{ int res;  kont_descr compil_kont, *pkntx;  tcursnum base;
  tptr basename;  tobjnum basenum;  tcateg cat;  BOOL dflt_view=FALSE;
  /* preparation of kontext */
  pkntx=&compil_kont;  /* compilation kontext (may be set to NULL later) */
  if (!analyze_base(cdp, viewsource, &basename, &basenum, &cat, &dflt_view))
  { corefree(basename);
    return FALSE;
  }
  safefree((void**)&basename);
  if (dflt_view) return TRUE;
  compil_kont.kontcateg=cat;
  if (cat==CATEG_CURSOR)  /* open the view cursor */
  { if (cd_Open_cursor(cdp, basenum, &base))
    { if (dflt_view) corefree(basename);
      return FALSE;
    }
    compil_kont.kontcateg=CATEG_DIRCUR;   /* CURSOR -> DIRCUR */
  }
  else if (cat==CATEG_TABLE)
    base=basenum;
  else if (cat==CATEG_DIRCUR)
    base=basenum;
  else  /* cat==CATEG_VIEW */
    pkntx=NULL;   /* no kontext for the compilation */
  compil_kont.kontobj=base;

 /* access to definition not checked, but should be */
  compil_info xCI(cdp, viewsource, MEMORY_OUT, view_def_comp);
  compil_kont.next_kont=NULL;
  xCI.kntx=pkntx;  // compil_kont or NULL
  xCI.thisform=1;  // defined by set_this_form_source(cdp, viewsource);
  const char * prev = set_this_form_source(cdp, viewsource);
  res=compile(&xCI);
  set_this_form_source(cdp, prev);
  if (cat==CATEG_CURSOR) cd_Close_cursor(cdp, base);
  if (!res)
    { corefree(xCI.univ_code);  return TRUE; }
  return FALSE;
}

CFNC DllExport BOOL WINAPI Check_apl_views(void)
{ trecnum rec, limit;  tcursnum cursnum;  BOOL ok=TRUE;
  char query[100+OBJ_NAME_LEN+2*UUID_SIZE+70];  tobjnum objnum;
  char objname[OBJ_NAME_LEN+1];  tptr def;
  cdp_t cdp = GetCurrTaskPtr();
  make_apl_query(query, "OBJTAB", cdp->front_end_uuid);
  sprintf(query+strlen(query), " and (category=chr(%u) or category=chr(%u))", CATEG_VIEW, CATEG_VIEW | IS_LINK);
  if (cd_Open_cursor_direct(cdp, query, &cursnum)) { ok=FALSE;  cd_Signalize(cdp); }
  else
  {
    if (cd_Rec_cnt(cdp, cursnum, &limit)) { ok=FALSE;  Signalize(); }
    else for (rec=0;  rec<limit;  rec++)
    { if (cd_Read(cdp, cursnum, rec, APLOBJ_NAME, NULL, objname))
        { ok=FALSE;  Signalize();  continue; }
      Set_status_text(objname);
      if (cd_Find_object(cdp, objname, CATEG_VIEW, &objnum))
        { ok=FALSE;  Signalize();  continue; }
      def=cd_Load_objdef(cdp, objnum, CATEG_VIEW);
      if (def==NULL)
        { ok=FALSE;  Signalize();  continue; }
      BOOL view_ok=check_view(cdp, def);
      corefree(def);
      if (!view_ok) Info_box("Chyba v kompilaci", objname);
    } // cycle
    cd_Close_cursor(cdp, cursnum);
  }
  return ok;
}
#endif

#if 0
CFNC DllExport BOOL WINAPI IsViewChanged(HWND hView)
{ int i; atr_info * catr; view_dyn *inst; 
  inst = (view_dyn*)GetViewInst(hView);
  for (i=0, catr=inst->dt->attrs;  i<inst->dt->attrcnt;  i++, catr++)
    if (catr->changed)
      return TRUE;
  if (inst->vwOptions & SIMPLE_VIEW)
  { if (inst->subinst->editchanged)
      return(TRUE);
  }
  else
  { if (inst->sel_is_changed)
      return TRUE;
    t_ctrl *itm;
    for (i=0, itm=FIRST_CONTROL(inst->stat); i < inst->stat->vwItemCount; i++, itm=NEXT_CONTROL(itm))
      if (itm->ctClassNum == NO_VIEW && IsViewChanged(GetDlgItem(hView, itm->ctId)))
        return TRUE;
  }
  return FALSE;
}
#endif

/////////////////////////// replacements /////////////////////////////
CFNC BOOL SetInPlaceActWnd(HWND hActWnd, t_tb_descr *tb_descr) { return TRUE; }

CFNC void enable_parent(view_dyn * inst, BOOL state)
/* parent must not be disabled by EnableWindow: mouse clicking would go through the window! */
{ 
#if 0
  view_dyn * par_inst;
  if (IsView(inst->hParent))
  { par_inst = (view_dyn*)GetWindowLong(inst->hParent, 0);
    if (par_inst)
      par_inst->disabling_view=state ? 0 : inst->hWnd;
  }
#endif
}

CFNC BOOL is_button(t_ctrl * itm)
{ return false; }

CFNC BOOL is_check(t_ctrl * itm)
{ return itm->ctClassNum==NO_CHECK || itm->ctClassNum==NO_RADIO; }

t_ctrl_base empty_control;

const char * sigstate_names[] = { "Not signed",
   "Fabricated signature (couterfeit)",
   "Record changed after being signed",
   "User unknown, cannot check the signature",
   "Signature id valid, user identity uncertified",
   "Signed after certificate expired or invalidated",
   "Indentity certified by a user not being CA",
   "Signature is valid",
   "Error in the signature" };

void signature2texts(t_signature * sig, int siglen, char * bufstate, char * bufname)
{ t_sigstate state;
 // find sigstate:
  if      (siglen < 0)                   state=sigstate_error;
  else if (siglen < sizeof(t_signature)) state=sigstate_notsigned;
  else                                   state=(t_sigstate)sig->state;
 // create state name:
  if (bufstate!=NULL) strcpy(bufstate, sigstate_names[(int)state]);
 // create user name(s):
  if (state!=sigstate_notsigned && state!=sigstate_error)  // sig defined
  { strcpy(bufname, sig->user_ident.name1);
    if (*bufname && sig->user_ident.name2)
      { strcat(bufname, " ");  strcat(bufname, sig->user_ident.name2); }
    if (*bufname && sig->user_ident.name3)
      { strcat(bufname, " ");  strcat(bufname, sig->user_ident.name3); }
    if (!*bufname) strcpy(bufname, sig->user_ident.identif);
   // add the timestamp:
    if (state==sigstate_valid)
    { int pos=(int)strlen(bufstate);
      bufstate[pos++]=' ';
      bufstate[pos++]='-';
      bufstate[pos++]=' ';
      datim2str(sig->dtm, bufstate+pos, 5);
    }
  }
  else *bufname=0;
}

void post_notif(HWND hWnd, UINT notif_num) { }

void free_sinst(sview_dyn * sinst)   /* called iff sinst!=NULL */
{ corefree(sinst->sitems); /* may be NULL if NO_MEMORY accored when allocating sitems */
  corefree(sinst);         /* may be NULL if NO_MEMORY accored when allocating sinst */
}

void enter_breaked_state(cdp_t cdp)
{}

void prez_set_null(tptr adr, uns8 tp)
{ switch (tp)
  { case ATT_CHAR:      case ATT_STRING:
    default:
    case ATT_ODBC_NUMERIC:  case ATT_ODBC_DECIMAL: *adr=0; break;
    case ATT_FLOAT: *(double*)adr=NONEREAL;                break;
    case ATT_BOOLEAN: *adr=(uns8)0x80;                     break;
    case ATT_AUTOR: memset(adr, 0, UUID_SIZE);             break;
    case ATT_INT8:  *(sig8*)adr=NONETINY;                  break;
    case ATT_INT16: *(sig16*)adr=NONESHORT;                break;
    case ATT_INT64: *(sig64*)adr=NONEBIGINT;               break;
    case ATT_MONEY: *(uns16*)adr=0;  *(sig32*)(adr+2)=NONEINTEGER;  break;
    case ATT_INT32:
    case ATT_DATE:  case ATT_DATIM:  case ATT_TIMESTAMP:
    case ATT_TIME:  *(sig32*)adr=NONEINTEGER;                    break;
    case ATT_PTR:
    case ATT_BIPTR: *(trecnum*)adr=NORECNUM;               break;
    case ATT_ODBC_DATE:
    case ATT_ODBC_TIME: memset(adr, 0, sizeof(basicval));  break;
  }
}

HWND hModalView = 0;

CFNC DllPrezen tptr WINAPI get_var_value(WORD * varcode, BYTE type)
{ int codesize = *varcode;
  var_start * ptr = (var_start*)(varcode+1);
  while (codesize>0)
  { if (ptr->type==type) return (tptr)ptr->value;
    codesize-=ptr->size + 3;
    ptr=(var_start*)((tptr)ptr + ptr->size + 3);
  }
  return NULL;
}

CFNC DllPrezen const wchar_t * WINAPI get_enum_def(view_dyn * inst, t_ctrl * itm)
{ if (inst!=NULL)
  { enum_cont * enm = inst->enums;
    while (enm && enm->ctId != itm->ctId) enm=enm->next;
    if (enm && enm->defin /*&& *enm->defin*/) return enm->defin; // empty enum allowed!
  }
  return (const wchar_t *)get_var_value(&itm->ctCodeSize, CT_ENUMER);
}

CFNC DllPrezen t_ctrl * WINAPI get_control_def(view_dyn * inst, int Id)
{ t_ctrl * ctrl;  int i;
  if (!inst->stat) /* error trap */ return (t_ctrl*)&empty_control;
  //Id=IDBASE(Id);  -- no more necessary
  for (i=0, ctrl=FIRST_CONTROL(inst->stat);  i<inst->stat->vwItemCount;
       i++, ctrl=NEXT_CONTROL(ctrl))
    if (ctrl->ctId==Id) return ctrl;
  return (t_ctrl*)&empty_control;
}

CFNC DllExport t_ctrl * WINAPI get_control_pos(view_dyn * inst, int sel)
{ t_ctrl * ctrl;
  if ((sel>=inst->stat->vwItemCount) || (sel<0)) return (t_ctrl*)&empty_control;
  ctrl=FIRST_CONTROL(inst->stat);
  while (sel--) ctrl=NEXT_CONTROL(ctrl);
  return ctrl;
}


CFNC DllPrezen unsigned WINAPI calc_tb_size(const t_tb_descr * tb) // returns the full size of the t_tb_descr structure
{ return 0; }

CFNC DllPrezen void WINAPI load_toolbar_bitmaps(t_tb_descr * tb)
{}
CFNC DllPrezen void WINAPI release_toolbar_bitmaps(const t_tb_descr * tb)
{}
void view_dyn::update_menu_toolbar(BOOL activating, HWND next_active)
{ }

