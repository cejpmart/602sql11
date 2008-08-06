/****************************************************************************/
/* wprocs.c - standard procedures for internal language                     */
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

#if WBVERS<900
#ifdef UNIX
#include "wbprezen.h"
#else
#include "wprez.h"
#endif
#include "messages.h"
#endif

#ifdef WINS
#include "commctrl.h"
#include <windowsx.h>
#else
#ifndef UNIX
#include <io.h>
#endif
#endif
#include "replic.h"  // contains mail functions

#ifdef UNIX
#include <unistd.h>
#include <sys/stat.h>
#else
#include <direct.h>
#endif
#include <math.h>
#include <time.h>
#include <stdlib.h>

#if defined(WINS) && (WBVERS<=810)
#include "mystdio.cpp"
#include <richedit.h>
#else
#define myFILE          FILE
#define myfopen(n,a)    fopen(n,a)
#define myfclose(f)     fclose(f)
#define myfgets(a,b,c)  fgets(a,b,c)
#define myfgetc(a)      fgetc(a)
#define myfputs(a,b)    fputs(a,b)
#define myfputc(a,b)    fputc(a,b)
#define myungetc(a,b)   ungetc(a,b)
#define myfseek(a,b,c)  fseek(a,b,c)
#define myftell(a)      ftell(a)
#endif

#undef  MCS_NOTODAY
#define MCS_NOTODAYCIRCLE   0x0008
#define MCS_NOTODAY         0x0010

void WINAPI sp_err_mask(cdp_t cdp, wbbool mask)
{ cdp->RV.mask=mask; }

#ifdef WINS
#if WBVERS<900
static HWND WINAPI sp_open_view(cdp_t cdp, char * viewsource, tcursnum base, uns32 flags, trecnum pos, HWND hParent, HWND * viewid)
{ return cd_Open_view(cdp, viewsource, base, flags, pos, NULL, hParent, viewid); }

static BOOL WINAPI sp_print_view(cdp_t cdp, char * viewsource, tcursnum base, trecnum start, trecnum stop)
{ return cd_Print_view(cdp, viewsource, base, start, stop, NULL); }

static HWND WINAPI sp_bind_records(cdp_t cdp, HWND hParent, tcursnum cursnum, trecnum position,
                         tattrib attr, uns16 index, uns32 sizetype,
                         char * viewdef, tcursnum base, uns32 flags)
{ return Bind_records(hParent, cursnum, position, attr, index, (uns16)sizetype,
                      viewdef, base, flags, NULL, cdp->RV.hClient);
}

static HWND WINAPI sp_select_records(char * viewsource, tcursnum base, uns32 flags,
           uns32 limit, trecnum * bindl, HWND hParent, HWND * viewid)
{ return Select_records(viewsource, base, flags, limit, bindl, NULL, hParent, viewid); }

static HWND WINAPI sp_relate_record(valstring loc_attr, char * viewsource,
         tcursnum base, uns32 flags,HWND * viewid, valstring remote_attr)
{ return Relate_record(loc_attr.s, viewsource, base, flags, NULL, viewid, remote_attr.s); }

static tptr WINAPI sp_get_view_item(cdp_t cdp, HWND hView, unsigned ID)
{ GetDlgItemText(hView, ID, cdp->RV.hyperbuf, 256);
  if (cdp->RV.prev_run_state)
  { strcpy(cdp->RV.prev_run_state->hyperbuf, cdp->RV.hyperbuf);
    return cdp->RV.prev_run_state->hyperbuf;
  } /* ... because run state was saved!! */
  else return cdp->RV.hyperbuf;
}

static BOOL WINAPI sp_get_item_value(cdp_t cdp, HWND hView, trecnum intrec, int itemid, untstr * unt)
{ if (is_string(unt->uany.type))
    { safefree((void**)&unt->uptr.val);  unt->uany.type=0; }
  if (!Get_item_value(hView, intrec, itemid, cdp->RV.prev_run_state->hyperbuf))
    return FALSE;
  tptr val = (tptr)corealloc(strlen(cdp->RV.prev_run_state->hyperbuf)+1, 88);
  if (!val) return FALSE;
  strcpy(val, cdp->RV.prev_run_state->hyperbuf);
  unt->uptr.val=val;  unt->uany.type=ATT_STRING;  unt->uany.specif=0;
  return TRUE;
}

static BOOL WINAPI sp_set_item_value(cdp_t cdp, HWND hView, trecnum intrec, int itemid, untstr * unt)
{ basicval srcval, destval;  char buf[256];
  srcval.unt=unt;  destval.ptr=buf;
  int res=untyped_assignment(cdp, &destval, &srcval, ATT_STRING, ATT_UNTYPED, 0, 255);
  if (res) return FALSE;
  return Set_item_value(hView, intrec, itemid, buf);
}

DllExport BOOL WINAPI TranslateRegisteredKeys(run_state * RV, MSG * msg)
/* Problem: Whem modal view is opened, messages are translated, but they are
   inserted into the queue and the are performed after closing the view.
   Disabling the translation would make impossible to mask standard keys. */
{ if ((msg->message==WM_KEYDOWN)    || (msg->message==WM_KEYUP) ||
      (msg->message==WM_SYSKEYDOWN) || (msg->message==WM_SYSKEYUP))
  { key_trap * kt = RV->key_registry;
    if (kt)
    { BOOL is_shift=GKState(VK_SHIFT);
      BOOL is_ctrl =GKState(VK_CONTROL);
      BOOL is_alt  =GKState(VK_MENU);
      while (kt)
      { if ((kt->keycode  ==msg->wParam) && (kt->with_shift==is_shift) &&
            (kt->with_ctrl==is_ctrl)     && (kt->with_alt  ==is_alt))
        { if ((msg->message==WM_KEYDOWN) || (msg->message==WM_SYSKEYDOWN))
            SendMessage(GetParent(RV->hClient), WM_COMMAND, kt->msgnum, 0);
          return FALSE;  /* message destroyed */
        }
        kt=kt->next;
      }
    }
  }
  return TRUE;
}

static BOOL WINAPI sp_get_ext_message(cdp_t cdp, unsigned * mmsg, HWND * hWnd, sig32 * info)
{ unsigned wbmsg;  run_state * RV = &cdp->RV;
  while (RV->int_msg_queue.empty())
  { MSG msg;
    if (!GetMessage(&msg, 0,0,0)) //break;
      { PostQuitMessage(0);  return FALSE; }
    if (TranslateRegisteredKeys(RV, &msg))
    if (!TranslateMDISysAccel(RV->hClient, &msg))
    { TranslateMessage(&msg);       /* Translates virtual key codes */
//      if ((msg.message==WM_KEYDOWN) && (msg.wParam==VK_CANCEL))
//        if ((RV->global_state==PROG_RUNNING) && RV->bpi && (RV->wexception!=BREAK_EXCEPTION))
//          RV->wexception=BREAK_EXCEPTION;
//        else
//          RV->break_request=wbtrue;
//      else
        DispatchMessage(&msg);
    }
    if (RV->break_request || (RV->wexception==BREAK_EXCEPTION)) break;
  }
  if (!RV->int_msg_queue.pick_msg(&wbmsg, hWnd)) // queue empty -> breaked
    wbmsg = RV->break_request ? -1 : -2;
  if (mmsg) *mmsg=wbmsg;
  return wbmsg!=(unsigned)-1;
}

static BOOL WINAPI sp_get_message(sig32 * mess)  // very obsolete
{ unsigned msg;  HWND hWnd;
  BOOL res = sp_get_ext_message(GetCurrTaskPtr(), &msg, &hWnd, NULL);
  *mess = ((sig32)(uns16)hWnd << 16) | msg;
  return res;
}

static BOOL WINAPI sp_peek_message(cdp_t cdp)
{ MSG msg;  run_state * RV = &cdp->RV;
 /* must call PeekMessage many times even if there is an int_msg */
  while (PeekMessage(&msg, 0,0,0, PM_REMOVE) && !RV->break_request)
  { if (TranslateRegisteredKeys(RV, &msg))
      if (!TranslateMDISysAccel(RV->hClient, &msg))
      { TranslateMessage(&msg);       /* Translates virtual key codes         */
        DispatchMessage(&msg);       /* Dispatches message to window         */
      }
  }
  return !RV->int_msg_queue.empty();
}

static void WINAPI sp_register_key(cdp_t cdp, int keycode, wbbool with_shift, wbbool with_ctrl,
                            wbbool with_alt, unsigned msgnum)
{ key_trap * kt, ** pkt;
  if (!msgnum)
  { pkt=&cdp->RV.key_registry;
    while (*pkt)
    { if (((*pkt)->keycode  ==keycode)   && ((*pkt)->with_shift==with_shift) &&
          ((*pkt)->with_ctrl==with_ctrl) && ((*pkt)->with_alt  ==with_alt))
      { kt=*pkt;
        *pkt=(*pkt)->next;
        corefree(kt);
        break;
      }
      pkt=&(*pkt)->next;
    }
  }
  else
  { kt=(key_trap*)corealloc(sizeof(key_trap), 86);
    if (kt==NULL) return;
    kt->keycode   =keycode;
    kt->with_shift=with_shift;
    kt->with_ctrl =with_ctrl;
    kt->with_alt  =with_alt;
    kt->msgnum    =msgnum;
    kt->next=cdp->RV.key_registry;
    cdp->RV.key_registry=kt;
  }
}

static char * check_aster(char * viewdef, char * buf)
{ while (*viewdef==' ') viewdef++;
  if ((*viewdef=='*') || !*viewdef)return viewdef;
  *buf='*';
  strmaxcpy(buf+1, viewdef, OBJ_NAME_LEN+1);
  return buf;
}
static void WINAPI sp_viewclose(char * viewdef)
{ HWND id;  char buf[1+OBJ_NAME_LEN+1];
  viewdef=check_aster(viewdef, buf);
  id = find_view(viewdef);
  if (id) Close_view(id);
}
static void WINAPI sp_viewopen(cdp_t cdp, char * viewdef)
{ HWND id;  char buf[1+OBJ_NAME_LEN+1];
  viewdef=check_aster(viewdef, buf);
  id = find_view(viewdef);
  if (id) Pick_window(id);
  else sp_open_view(cdp, viewdef, NO_REDIR, 0, 0, 0, NULL);
}
static BOOL WINAPI sp_viewprint(cdp_t cdp, char * viewdef)
{ char buf[1+OBJ_NAME_LEN+1];
  viewdef=check_aster(viewdef, buf);
  return sp_print_view(cdp, viewdef, NO_REDIR, 0, NORECNUM);
}
static BOOL WINAPI sp_viewstate(char * viewdef)
{ char buf[1+OBJ_NAME_LEN+1];
  viewdef=check_aster(viewdef, buf);
  return find_view(viewdef) ? 1 : 0;
}
static void WINAPI sp_viewtoggle(cdp_t cdp, char * viewdef)
{ HWND id;  char buf[1+OBJ_NAME_LEN+1];
  viewdef=check_aster(viewdef, buf);
  id = find_view(viewdef);
  if (id) Close_view(id);
  else sp_open_view(cdp, viewdef, NO_REDIR, 0, 0, 0, NULL);
}

static sig32 WINAPI sp_set_cursor(sig32 shape)
{ HCURSOR oldcurs;
  oldcurs=SetCursor(LoadCursor(NULL, shape ? IDC_WAIT : IDC_ARROW));
  return (oldcurs==LoadCursor(NULL, IDC_ARROW)) ? 0 : 1;
}
#endif
#endif
tptr WINAPI sp_strcat(tptr s1, tptr s2)
{ int l1,l2;  char mybuf[STR_BUF_SIZE];
  l1=(int)strlen(s1);  l2=(int)strlen(s2);
  if (l1>=STR_BUF_SIZE)
   memcpy(mybuf,s1,STR_BUF_SIZE-1);
  else
  { strcpy(mybuf,s1);
    if (l1+l2>=STR_BUF_SIZE)
      memcpy(mybuf+l1,s2,(STR_BUF_SIZE-1)-l1);
    else
    strcat(mybuf,s2);
  }
  mybuf[STR_BUF_SIZE-1]=0;
  strcpy(get_RV()->hyperbuf,mybuf);
  return get_RV()->hyperbuf;
}

tptr WINAPI sp_strcopy(cdp_t cdp, valstring d, unsigned index, unsigned count)
{ unsigned len = (int)strlen(d.s);
  *cdp->RV.hyperbuf=0;
  if (!index) return cdp->RV.hyperbuf;
  index--;  /* 0 based */
  if (index >= len) return cdp->RV.hyperbuf;  /* nothing to be deleted */
  if (count > len-index) count=len-index;
  if (count>255) count=255;
  memcpy(cdp->RV.hyperbuf, d.s+index, count);
  cdp->RV.hyperbuf[count]=0;
  return cdp->RV.hyperbuf;
}

static tptr WINAPI sp_sql_value(cdp_t cdp, valstring sql_expr)
{ if (cd_SQL_value(cdp, sql_expr.s, cdp->RV.hyperbuf, sizeof(cdp->RV.hyperbuf)))
    return NULLSTRING;
  return cdp->RV.hyperbuf;
}
/***************************** database io **********************************/
typedef struct
{ uns8 val[16];
} valunikey;

BOOL is_repl_destin(valunikey unikey)
{ cdp_t cdp=GetCurrTaskPtr();
  return cdp->all_views ?
     !memcmp(unikey.val, cdp->all_views, UUID_SIZE) : FALSE;
}

BOOL WINAPI sp_restore_curs(cdp_t cdp, tcursnum * cdf)
{ if (cdf[2] && (cdf[2]!=NOCURSOR))   /* cdf[2]==0 if uninitialied variable */
  { cd_Close_cursor(cdp, *cdf);  /*curs_closed(*cdf);*/
    *cdf=cdf[2]; cdf[2]=NOCURSOR;
    return FALSE;
  }
  return TRUE;
}

BOOL WINAPI sp_close_curs(cdp_t cdp, tcursnum * cdf)  // may be a variable cursor reference!!
{ BOOL res;
  if (*cdf && (*cdf!=NOCURSOR))  /* *cdf==0 if uninitialied variable */
  { sp_restore_curs(cdp, cdf);
    res=cd_Close_cursor(cdp, *cdf);
    *cdf=NOCURSOR;
    return res;
  }
  else return TRUE;
}

BOOL WINAPI sp_open_curs(cdp_t cdp, tcursnum * cdf)
{ sp_restore_curs(cdp, cdf);              /* if subcursor open, close it */
  if (*cdf && (*cdf!=NOCURSOR)) sp_close_curs(cdp, cdf); /* if cursor open, close it */
  if (!cdf[1]) return TRUE;   /* this is variable cursor! */
  return cd_Open_cursor(cdp, cdf[1],cdf);
}

BOOL WINAPI sp_restrict_curs(cdp_t cdp, tcursnum * cdf, char * constrain)
{ tcursnum newcurs;
  if (!*cdf || (*cdf==NOCURSOR)) return TRUE;  /* cursor not open */
  if (cd_Open_subcursor(cdp, *cdf,constrain,&newcurs))
    return TRUE;
  if (cdf[2] && (cdf[2]!=NOCURSOR)) cd_Close_cursor(cdp, *cdf);  /* old subcursor */
  else cdf[2]=*cdf;
  *cdf=newcurs;
  return FALSE;
}

BOOL WINAPI sp_open_sql_c(cdp_t cdp, tcursnum * cursnum, char * def)
{ if (cd_Open_cursor_direct(cdp, def, cursnum)) { *cursnum=NOCURSOR;  return TRUE; }
  return FALSE;
}


BOOL WINAPI sp_open_sql_p(cdp_t cdp, tcursnum * cursnum, valstring d1, valstring  d2, valstring d3, valstring d4)
{ unsigned len;  tptr query, p;  BOOL res;
  len=(int)strlen(d1.s)+(int)strlen(d2.s)+(int)strlen(d3.s)+(int)strlen(d4.s)+32;
  query=(tptr)corealloc(len, OW_PGRUN);
  if (!query) rr_error(OUT_OF_R_MEMORY);
  strcpy(query, "SELECT ");
  strcat(query, d1.s);
  strcat(query, " FROM ");
  strcat(query, d2.s);
  p=d3.s;  while (*p==' ') p++;
  if (*p)
  { strcat(query, " WHERE ");
   strcat(query, p);
  }
  p=d4.s;  while (*p==' ') p++;
  if (*p)
  { strcat(query, " ORDER BY ");
   strcat(query, p);
  }
  res=sp_open_sql_c(cdp, cursnum, query);
  corefree(query);
  return res;
}

/****************************** untyped operations **************************/
static tptr prepare_unt(cdp_t cdp, untstr * unt, int newtype, t_specif specif)
{ if (is_string(unt->uany.type)) unt_unregister(cdp, unt);
  unt->uany.type = newtype==ATT_INT16 ? ATT_INT32 : newtype;
  unt->uany.specif = specif.opqval;
  if (is_string(newtype))
    return unt->uptr.val=cdp->RV.hyperbuf;  // not registered because not allocated!
  else return (tptr)unt->uany.val;
}

BOOL WINAPI sp_c_sum(cdp_t cdp, tcursnum curs, tptr attrname, tptr condition, untstr * unt)
{ d_attr datr;  tptr ptr;
  char aname[ATTRNAMELEN+1];
  strmaxcpy(aname, attrname, ATTRNAMELEN+1);
  Upcase9(cdp, aname);
  if (!find_attr(cdp, curs, CATEG_TABLE, aname, NULL, NULL, &datr)) return TRUE;
  ptr=prepare_unt(cdp, unt, datr.type, datr.specif);
  return cd_C_sum(cdp, curs, aname, condition, ptr);
}

BOOL WINAPI sp_c_avg(cdp_t cdp, tcursnum curs, tptr attrname, tptr condition, untstr * unt)
{ d_attr datr;  tptr ptr;
  char aname[ATTRNAMELEN+1];
  strmaxcpy(aname, attrname, ATTRNAMELEN+1);
  Upcase9(cdp, aname);
  if (!find_attr(cdp, curs, CATEG_TABLE, aname, NULL, NULL, &datr)) return TRUE;
  ptr=prepare_unt(cdp, unt, datr.type, datr.specif);
  return cd_C_avg(cdp, curs, aname, condition, ptr);
}

BOOL WINAPI sp_c_max(cdp_t cdp, tcursnum curs, tptr attrname, tptr condition, untstr * unt)
{ d_attr datr;  tptr ptr;
  char aname[ATTRNAMELEN+1];
  strmaxcpy(aname, attrname, ATTRNAMELEN+1);
  Upcase9(cdp, aname);
  if (!find_attr(cdp, curs, CATEG_TABLE, aname, NULL, NULL, &datr)) return TRUE;
  ptr=prepare_unt(cdp, unt, datr.type, datr.specif);
  BOOL res=cd_C_max(cdp, curs, aname, condition, ptr);
  if (datr.type==ATT_INT16) unt->usig32.val = 
    (sig16)unt->usig32.val==NONESHORT ? NONEINTEGER : (sig32)(sig16)unt->usig32.val;
  return res;
}

BOOL WINAPI sp_c_min(cdp_t cdp, tcursnum curs, tptr attrname, tptr condition, untstr * unt)
{ d_attr datr;  tptr ptr;
  char aname[ATTRNAMELEN+1];
  strmaxcpy(aname, attrname, ATTRNAMELEN+1);
  Upcase9(cdp, aname);
  if (!find_attr(cdp, curs, CATEG_TABLE, aname, NULL, NULL, &datr)) return TRUE;
  ptr=prepare_unt(cdp, unt, datr.type, datr.specif);
  BOOL res=cd_C_min(cdp, curs, aname, condition, ptr);
  if (datr.type==ATT_INT16) unt->usig32.val = 
    (sig16)unt->usig32.val==NONESHORT ? NONEINTEGER : (sig32)(sig16)unt->usig32.val;
  return res;
}

trecnum WINAPI sp_look_up(cdp_t cdp, tcursnum curs, const char * attrname, untstr * unt)
{ char cond[530];  tcursnum subcurs;  trecnum rec;
  if (!unt->uany.type) return NORECNUM;
  strcpy(cond, attrname);
  cd_convert_to_SQL_literal(cdp, cond+strlen(cond), is_string(unt->uany.type) ? unt->uptr.val : (tptr)unt->uany.val, unt->uany.type, t_specif(unt->uany.specif), true);
  if (cd_Open_subcursor(cdp, curs, cond, &subcurs)) return NORECNUM;
  if (cd_Super_recnum(cdp, subcurs, curs, 0, &rec)) rec=NORECNUM;
  cd_Close_cursor(cdp, subcurs);
  return rec;
}

/***************************** the file interface ***************************/
static BOOL file_open(cdp_t cdp, uns8 * fl, char * name, BOOL create)
{ int i=0;  
  while ((myFILE*)cdp->RV.files[i]) if (++i >= MAX_OPEN_FILES) return FALSE;
  cdp->RV.files[i]=myfopen(name, create ? "w+b" : "r+b");
  if (!cdp->RV.files[i]){ /* Try if can be opened for read at least */
#ifdef LINUX
	  if (create||! (cdp->RV.files[i]=myfopen(name, "rb")))
#endif
	  return FALSE;
	  }
  *fl=(uns8)i;
  return TRUE;
}

static BOOL WINAPI sp_reset(cdp_t cdp, uns8 * fl, char * name)
{ return file_open(cdp, fl, name, FALSE); }

static BOOL WINAPI sp_rewrite(cdp_t cdp, uns8 * fl, char * name)
{ return file_open(cdp, fl, name, TRUE); }

static void WINAPI sp_close(cdp_t cdp, uns8 fl)
{ if ((fl<MAX_OPEN_FILES) && (myFILE*)cdp->RV.files[fl])
  { myfclose((myFILE*)cdp->RV.files[fl]);
    cdp->RV.files[fl]=NULL;
  }
}

BOOL close_all_files(cdp_t cdp)
{ int i;  BOOL closed=FALSE;
  for (i=0; i<MAX_OPEN_FILES; i++)
    if ((myFILE*)cdp->RV.files[i])
    { sp_close(cdp, (uns8)i);
      closed=TRUE;
    }
  return closed;
}

static BOOL WINAPI skipspaces(cdp_t cdp, uns8 fl)  /* returns TRUE iff eof */
/* skips chars <= ' ' (CR, LF, FF, TAB, NULL, 1a) */
{ int ival;
  if ((fl>=MAX_OPEN_FILES) || !(myFILE*)cdp->RV.files[fl]) return TRUE;
  do
  { ival=myfgetc((myFILE*)cdp->RV.files[fl]);
    if (ival==EOF) return TRUE;
    if (ival > ' ')
    { myungetc(ival, (myFILE*)cdp->RV.files[fl]);
      return FALSE;
    }
  } while (TRUE);
}

static sig32 WINAPI sp_filelength(cdp_t cdp, uns8 fl)
{ long prepos, endpos;  
  if ((fl<MAX_OPEN_FILES) && (myFILE*)cdp->RV.files[fl])
  { prepos=myftell((myFILE*)cdp->RV.files[fl]);
    myfseek((myFILE*)cdp->RV.files[fl], 0, SEEK_END);
    endpos=myftell((myFILE*)cdp->RV.files[fl]);
    myfseek((myFILE*)cdp->RV.files[fl], prepos, SEEK_SET);
    return endpos;
  }
  else return -1;
}

BOOL WINAPI sp_seek(cdp_t cdp, uns8 fl, uns32 pos)
{ if ((fl<MAX_OPEN_FILES) && (myFILE*)cdp->RV.files[fl])
    return !myfseek((myFILE*)cdp->RV.files[fl], pos, SEEK_SET);
  else return FALSE;
}

extern char decim_separ[2];

void win_read(cdp_t cdp, int fl, uns8 type, run_state * RV)
/* size is not removed from NOS, destination is on NNOS */
/* removes size, adds result & TRUE or removes dest. & adds FALSE twice */
{ BOOL err=FALSE;  int ival, pos;  basicval0 bval;  myFILE * f;
  if (fl>=MAX_OPEN_FILES) { err=TRUE;  goto rd_result; }
  f = (myFILE*)RV->files[fl];
  if (f==NULL)            { err=TRUE;  goto rd_result; }
  pos=0;
  switch (type)
  { case ATT_CHAR:
      ival=myfgetc(f);
      if (ival==EOF) err=TRUE;  else bval.int16=ival;
      break;
    case ATT_STRING:  
      do
      { ival=myfgetc(f);
        if (ival==EOF) break;
        if (ival=='\n') break;
        if (ival!='\r') RV->hyperbuf[pos++]=(char)ival;
      } while (pos<MAX_FIXED_STRING_LENGTH);
      RV->hyperbuf[pos]=0;  bval.ptr=RV->hyperbuf;  break;
    case ATT_INT16:  case ATT_INT32:
      skipspaces(cdp, fl);
      do
      { ival=myfgetc(f);
        if (ival==EOF) break;
        if (!((ival>='0')&&(ival<='9')||(ival=='-')||(ival=='+')))
          { myungetc(ival, f);  break; }
        RV->hyperbuf[pos++]=(char)ival;
      } while (pos<255);
      RV->hyperbuf[pos]=0;
      cutspaces(RV->hyperbuf);
      if (!convert_from_string(type, RV->hyperbuf, &bval, t_specif())) err=TRUE;
      break;
    case ATT_FLOAT:
      skipspaces(cdp, fl);
      do
      { ival=myfgetc(f);
        if (ival==EOF) break;
        if (!(ival>='0' && ival<='9'|| ival=='-' || ival=='+' ||
              ival=='E' || ival=='.'|| ival==*decim_separ))
          { myungetc(ival, f);  break; }
        RV->hyperbuf[pos++]=(char)ival;
      } while (pos<255);
      RV->hyperbuf[pos]=0;
      cutspaces(RV->hyperbuf);
      if (!convert_from_string(type, RV->hyperbuf, &bval, t_specif())) err=TRUE;
      break;
    case ATT_BOOLEAN:
      skipspaces(cdp, fl);
      do
      { ival=myfgetc(f);
        if (ival==EOF) break;
        if (!((ival>='A')&&(ival<='Z')||(ival>='a')&&(ival<='z')))
          { myungetc(ival, f);  break; }
        RV->hyperbuf[pos++]=(char)ival;
      } while (pos<255);
      RV->hyperbuf[pos]=0;
      if (!my_stricmp(RV->hyperbuf, "TRUE") || !my_stricmp(RV->hyperbuf, "ANO") ||
          !my_stricmp(RV->hyperbuf, "T")    || !my_stricmp(RV->hyperbuf, "A"))
        bval.int16=1;
      else if (!my_stricmp(RV->hyperbuf, "FALSE") || !my_stricmp(RV->hyperbuf, "NE") ||
               !my_stricmp(RV->hyperbuf, "F")     || !my_stricmp(RV->hyperbuf, "N"))
        bval.int16=0;
      else err=TRUE;
      break;
    case ATT_DATE:
      skipspaces(cdp, fl);
      do
      { ival=myfgetc(f);
        if (ival==EOF) break;
        if (!(ival>='0' && ival<='9' || ival=='.'))
          { myungetc(ival, f);  break; }
        RV->hyperbuf[pos++]=(char)ival;
      } while (pos<255);
      RV->hyperbuf[pos]=0;
      cutspaces(RV->hyperbuf);
      if (!convert_from_string(type, RV->hyperbuf, &bval, t_specif())) err=TRUE;
      break;
    case ATT_TIME:
      skipspaces(cdp, fl);
      do
      { ival=myfgetc(f);
        if (ival==EOF) break;
        if (!(ival>='0' && ival<='9' || ival==':' || ival=='.'))
          { myungetc(ival, f);  break; }
        RV->hyperbuf[pos++]=(char)ival;
      } while (pos<255);
      RV->hyperbuf[pos]=0;
      cutspaces(RV->hyperbuf);
      if (!convert_from_string(type, RV->hyperbuf, &bval, t_specif())) err=TRUE;
      break;
  }
 rd_result:
  if (!err)
  { *RV->stt=*(basicval*)&bval;
    (++RV->stt)->int16=1; /* result replaced size, "TRUE" added */
  }
  else
  { RV->stt->int16=0;    /* "FALSE" replacing size */
    (RV->stt-1)->int16=0;  /* "FALSE" replacing destination address */
  }
}

void win_write(cdp_t cdp, int fl, uns8 type, run_state * RV)
{ if ((fl>=MAX_OPEN_FILES) || !(myFILE*)RV->files[fl])
    { if (type!=0xff) RV->stt-=2; }   /* error, but must empty the stack */
  else
  { myFILE * f = (myFILE*)RV->files[fl];
    if (type==0xff) myfputs("\r\n", f);
    else if (is_string(type))
    { RV->stt-=2;
      myfputs(RV->stt[1].ptr, f);
    }
    else
    { conv2str((basicval0*)(RV->stt-1), type, RV->hyperbuf, (sig8)RV->stt->int16);
      RV->stt-=2;
      if (type==ATT_CHAR)  /* must write even the null char */
        myfputc(*RV->hyperbuf, f);
      else myfputs(RV->hyperbuf, f);
    }
  }
}

BOOL WINAPI sp_delete_file(const char * fname)
{ return unlink(fname) != 0; }

BOOL WINAPI sp_make_directory(const char * dname)
#ifdef UNIX
{ return mkdir(dname, S_IRWXU|S_IRWXG|S_IRWXO) != 0; }
#else
{ return mkdir(dname) != 0; }
#endif

#ifdef WINS
#if WBVERS<900
sig32 WINAPI sp_exec(char * pathname, char * params)
{ char CmdLine[256];
  strcpy(CmdLine, pathname);
  strcat(CmdLine, " ");
  strcat(CmdLine, params);
  return WinExec(CmdLine, SW_SHOW);
}

/*x0,x1,x2,3,x4,x9,10,11,52,53,55,57,88,89,93,94,95,96,x99,103*/
/****************************** link to other DLLs **************************/
static void WINAPI sp_setDlgItemText(HWND hDlg, unsigned nIDDlgItem, tptr lpString)
{ HWND hCtrl = GetDlgItem(hDlg, nIDDlgItem);
  SetWindowText(hCtrl, lpString);
  InvalidateRect(hCtrl, NULL, TRUE);
  UpdateWindow(hCtrl);
}
static BOOL WINAPI sp_edit_text(HWND hParent, tcursnum cursnum, trecnum position, tattrib attr, uns16 index,
              uns32 sizetype, int x, int y, int xl, int yl)
  { return Edit_text(hParent, cursnum, position, attr, index, (uns16)sizetype, x, y, xl, yl)!=0; }

BOOL WINAPI sp_Db2file(cdp_t cdp, HWND hParent, tcursnum cursnum, trecnum position, tattrib attr, uns16 index, uns16 sizetype,
                       const char * pathname)
#define DB2FILE_BUFSIZE 8192
{ BOOL res=TRUE;
  HANDLE hFile = CreateFile(pathname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hFile==INVALID_HANDLE_VALUE) return FALSE;
  tptr buf = (tptr)corealloc(DB2FILE_BUFSIZE, 84);
  if (!buf) res=FALSE;
  else
  { uns32 size, start = 0;  DWORD wr;
    do
    { if (cd_Read_var(cdp, cursnum, position, attr, index, start, DB2FILE_BUFSIZE, buf, &size))
        { res=FALSE;  break; }
      if (!WriteFile(hFile, buf, size, &wr, NULL) || wr!=size)
        { res=FALSE;  break; }
      start+=size;
    } while (size==DB2FILE_BUFSIZE);
    corefree(buf);
  }
  CloseHandle(hFile);
  return res;
}

BOOL WINAPI sp_File2db3(cdp_t cdp, const char * pathname,
                       HWND hParentB, tcursnum cursnumB, trecnum positionB, tattrib attrB, uns16 indexB, uns16 sizetypeB,
                       HWND hParentN, tcursnum cursnumN, trecnum positionN, tattrib attrN, uns16 indexN, uns16 sizetypeN,
                       HWND hParentC, tcursnum cursnumC, trecnum positionC, tattrib attrC, uns16 indexC, uns16 sizetypeC)
{ BOOL res=TRUE;
 // copy contents:
  HANDLE hFile = CreateFile(pathname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hFile==INVALID_HANDLE_VALUE) return FALSE;
  tptr buf = (tptr)corealloc(DB2FILE_BUFSIZE, 84);
  if (!buf) res=FALSE;
  else
  { uns32 start = 0;  DWORD rd;
    do
    { if (!ReadFile(hFile, buf, DB2FILE_BUFSIZE, &rd, NULL))
        { res=FALSE;  break; }
      if (cd_Write_var(cdp, cursnumB, positionB, attrB, indexB, start, rd, buf))
        { res=FALSE;  break; }
      start+=rd;
    } while (rd==DB2FILE_BUFSIZE);
    corefree(buf);
  }
  CloseHandle(hFile);
 // write file name:
  const d_table * td = cd_get_table_d(cdp, cursnumN, CATEG_TABLE);
  if (!td) res=FALSE;
  else
  { if (cd_Write(cdp, cursnumN, positionN, attrN, NULL, pathname, td->attribs[attrN].specif.length)) res=FALSE;
    release_table_d(td);
  }
 // find & write the content type:
#ifdef STOP
  SHFILEINFO sfi;
  if (!SHGetFileInfo(pathname, 0, &sfi, sizeof(sfi), SHGFI_TYPENAME)) res=FALSE;
  else
  { td = cd_get_table_d(cdp, cursnumC, CATEG_TABLE);
    if (!td) res=FALSE;
    else
    { if (cd_Write(cdp, cursnumC, positionC, attrC, NULL, sfi.szTypeName, td->attribs[attrC].specif)) res=FALSE;
      release_table_d(td);
    }
  }
#else
  int dotpos = strlen(pathname);  HKEY hKey;  
  char content_type[60];  DWORD size=sizeof(content_type);
  strcpy(content_type, "application/octet-stream");
  while (dotpos && pathname[dotpos]!='.') dotpos--;
  if (dotpos)
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, pathname+dotpos, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    { RegQueryValueEx(hKey, "Content Type", NULL, NULL, (BYTE*)content_type, &size);
      RegCloseKey(hKey);
    }
  td = cd_get_table_d(cdp, cursnumC, CATEG_TABLE);
  if (!td) res=FALSE;
  else
  { if (cd_Write(cdp, cursnumC, positionC, attrC, NULL, content_type, td->attribs[attrC].specif.length)) res=FALSE;
    release_table_d(td);
  }
#endif
  return res;
}

static HWND get_active_view(cdp_t cdp)
/* Returns the view, than is used as defualt in actions. */
{ if (cdp->RV.inst) return cdp->RV.inst->hWnd;
  return Active_view();
}

#endif
#endif

BOOL WINAPI sp_user_in_group(cdp_t cdp, tobjnum user, tobjnum group, wbbool * state)
{ BOOL res, Bstate;
  res=User_in_group(user, group, &Bstate);
  if (!res) *state=(wbbool)Bstate;
  return res;
}

BOOL WINAPI sp_sql_submit(cdp_t cdp, tptr sql_stat)
{ return cd_SQL_execute(cdp, sql_stat, NULL); }

void WINAPI sp_memmov(void * dest, void * source, unsigned size)
{ memmov(dest, source, size); }

BOOL add_privils(cdp_t cdp, tcursnum curs, trecnum rec, tobjnum user)
{// translate cursor record to table record:
  if (IS_CURSOR_NUM(curs))
  { uns16 num=1;  ttablenum tbnum;
    if (cd_Get_base_tables(cdp, curs, &num, &tbnum)) return TRUE;
    if (cd_Translate(cdp, curs, rec, 0, &rec)) return TRUE;
    curs=tbnum;
  }
 // check to see if the table has record privileges:
  { tattrib attrnum;  uns8 attrtype;  uns8 attrmult;  t_specif attrspecif;
    if (!cd_Attribute_info_ex(cdp, curs, "_W5_RIGHTS", &attrnum, &attrtype, &attrmult, &attrspecif))
      return FALSE; // no record privils
  }
 // get own & his privils:
  uns8 my_privs[PRIVIL_DESCR_SIZE], his_privs[PRIVIL_DESCR_SIZE];
  if (cd_GetSet_privils(cdp, cdp->logged_as_user, CATEG_USER, curs, rec, OPER_GET, my_privs)) return TRUE;
  if (cd_GetSet_privils(cdp, user,   CATEG_USER, curs, rec, OPER_GET, his_privs)) return TRUE;
  if (my_privs[0] & RIGHT_GRANT)
  { my_privs[0] &= ~RIGHT_DEL;  // do not grant deleting privilege
    for (int i=0;  i<PRIVIL_DESCR_SIZE;  i++) his_privs[i] |= my_privs[i];
    if (cd_GetSet_privils(cdp, user, CATEG_USER, curs, rec, OPER_SET, his_privs)) return TRUE;
  }
  return FALSE;
}

/************************ dynamic allocation ******************************/
#ifdef WINS
#if WBVERS<900
HANDLE hHeap = 0;

static void WINAPI sp_shell_execute(char * filename, char * verb)  
{ while (*verb==' ') verb++;
  if (!*verb) verb=NULL;  // default verb shall be specified as NULL
  ShellExecute(0, verb, filename, NULL, NULL, SW_SHOWNORMAL);
}

static BOOL WINAPI sp_replicate(cdp_t cdp, BOOL pull)
{ return AReplicate(cdp, 0, pull); }

CFNC void WINAPI dynmem_dispose(void * ptr)
{ if (ptr) HeapFree(hHeap, HEAP_NO_SERIALIZE, ptr); }

CFNC void WINAPI dynmem_new(cdp_t cdp, void ** pptr, unsigned size)
{ *pptr = HeapAlloc(hHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, size); 
  cdp->RV.testedval.ptr = (tptr)*pptr;
}

CFNC DllKernel void WINAPI dynmem_stop(void)
{ if (hHeap) { HeapDestroy(hHeap);  hHeap=0; } }

CFNC DllKernel void WINAPI dynmem_start(void)
{ if (hHeap) dynmem_stop();
  hHeap=HeapCreate(HEAP_NO_SERIALIZE, 1, 0);
}

void * WINAPI dynmem_addr(void * ptr)
{ return ptr; }
/************************** actions ******************************************/
static void WINAPI sp_action(cdp_t cdp, uns32 act_num, valstring objname, valstring text, valstring attr)
{ run_state * RV = &cdp->RV;
  HWND hView;  tobjnum objnum;
  switch ((unsigned)act_num)
  { case ACT_EMPTY:              break;   /* not called */
    case ACT_VIEW_OPEN:
      sp_viewopen(cdp, objname.s);    break;
    case ACT_VIEW_CLOSE:
      sp_viewclose(objname.s);   break;
    case ACT_VIEW_TOGGLE:
      sp_viewtoggle(cdp, objname.s);  break;
    case ACT_VIEW_PRINT:
      sp_viewprint(cdp, objname.s);   break;
    case ACT_VIEW_RESET:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      hView = find_view(p);
      if (hView) Reset_view(hView, NORECNUM, RESET_ALL|RESET_CACHE);
      break;
    }
    case ACT_VIEW_RESET_CURS:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      hView = find_view(p);
      if (hView) Reset_view(hView, NORECNUM, RESET_ALL|RESET_CACHE|RESET_CURSOR);
      break;
    }
    case ACT_VIEW_RESET_LIGHT:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      hView = find_view(p);
      if (hView) Reset_view(hView, NORECNUM, RESET_ALL);
      break;
    }
    case ACT_VIEW_RESET_SYNCHRO:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      hView = find_view(p);
      if (hView) Reset_view(hView, NORECNUM, RESET_ALL+RESET_SYNCHRO);
      break;
    }
    case ACT_VIEW_RESET_COMBO:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      hView = find_view(p);
      if (hView) Reset_view(hView, NORECNUM, RESET_COMBOS);
      break;
    }
    case ACT_VIEW_SYNOPEN:
    { char buf[1+OBJ_NAME_LEN+1];  HWND hParent, hWnd;
      tptr p=check_aster(objname.s, buf);
      hParent=get_active_view(cdp);
      if (!hParent) break;
      hWnd=cd_Open_view(cdp, p, NO_REDIR, PARENT_CURSOR, 0, NULL, hParent, NULL);
      if (hWnd && hWnd!=(HWND)1)   /* open and not modal */
        Register_rec_syn(hParent, hWnd);
      break;
    }
    case ACT_REC_PRINT:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      hView=get_active_view(cdp);
      if (!hView) break;
      view_dyn * inst = GetViewInst(hView);
      cd_Print_view(cdp, p, inst->dt->cursnum, inst->sel_rec, inst->sel_rec, NULL);
      break;
    }
    case ACT_ALL_PRINT_REP:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      hView=get_active_view(cdp);
      if (!hView) break;
      view_dyn * inst = GetViewInst(hView);
      cd_Print_view(cdp, p, inst->dt->cursnum, 0, NORECNUM, NULL);
      break;
    }
    case ACT_ALL_PRINT:
      hView=get_active_view(cdp);
      if (!hView) break;
      SendMessage(hView, SZM_PRINT, 0, 0);
      break;
    case ACT_VIEW_MODOPEN: // hParent used in order to disable the current view
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      cd_Open_view(cdp, p, NO_REDIR, MODAL_VIEW, 0, NULL, get_active_view(cdp), NULL);
      break;
    }
    case ACT_VIEW_MODSYNOPEN:
    { char buf[1+OBJ_NAME_LEN+1];  HWND hParent;
      tptr p=check_aster(objname.s, buf);
      hParent=get_active_view(cdp);
      if (!hParent) break;
     /* get cursor and position from the parent view: */
      view_dyn * par_inst = (view_dyn*)GetWindowLong(hParent, 0);
      tcursnum curs=par_inst->dt->cursnum;
      trecnum rec=par_inst->sel_rec;
     /* if hParent is subview, go to superview: */
      bool parent_changed = false;
      while (par_inst->vwOptions & IS_VIEW_SUBVIEW)
      { hParent=GetParent(hParent);
        if (!IsView(hParent)) break;
        par_inst = (view_dyn*)GetWindowLong(hParent, 0);
        parent_changed=true;
      }
      // must not use PARENT_CURSOR when hParent changed!
      cd_Open_view(cdp, p, curs, parent_changed ? MODAL_VIEW : MODAL_VIEW|PARENT_CURSOR, rec, NULL, hParent, NULL);
      break;
    }
    case ACT_INS_OPEN:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      cd_Open_view(cdp, p, NO_REDIR, 0, NONEINTEGER, NULL, 0, NULL);
      break;
    }
    case ACT_VIEW_COMMIT:
    case ACT_VIEW_ROLLBACK:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      hView = find_view(p);
      if (hView)
        if (act_num==ACT_VIEW_COMMIT)
        { if (!Commit_view(hView, FALSE, TRUE))  /* no ask, report errors */
            { wrestore_run(cdp);  r_error(ACTION_BREAK); }
        }
        else Roll_back_view(hView);
      break;
    }
    case ACT_DEL_REC:
    { hView=get_active_view(cdp);
      if (!hView) break;
      SendMessage(hView, SZM_DELREC, 0, 0);
      break;
    }
    case ACT_RELATE:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      Relate_record(attr.s, p, NO_REDIR, 0, NULL, NULL, text.s);
      break;
    }
    case ACT_RELSYN:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      hView=get_active_view(cdp);
      if (!hView) break;
      view_dyn * inst = GetViewInst(hView);
      HWND hWnd = cd_Open_view(cdp, p, NO_REDIR, 0, 0, NULL, 0, NULL);
      register_synchro(cdp, hView, hWnd, 0, attr.s, inst->toprec-inst->sel_rec, SUBVW_RELNAME);
      update_view(inst, 0, 0, RESET_SYNCHRO);
      break;
    }
    case ACT_BIND:
    { hView=get_active_view(cdp);  if (!hView) break;
      view_dyn * inst = GetViewInst(hView);
      if (inst->sel_rec!=NORECNUM) SendMessage(hView, SZM_BIND, 0, inst->sel_rec);
      break;
    }
    case ACT_MENU_SET:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      Main_menu(p);
      break;
    }
    case ACT_CLOSE_VW_MENU:
      if (!cdp->non_mdi_client)
        DestroyMdiChildren(RV->hClient, FALSE);   // closing object editors disabled in version 6.0
      /* cont. */
    case ACT_MENU_REMOVE:
      Main_menu(NULL);
      break;
    case ACT_MENU_REDRAW:
      Main_menu(NULLSTRING);
      break;
    case ACT_POPUP_MENU:
    { tptr name=objname.s;  if (*name=='*') name++;
      if (cd_Find_object(cdp, name, CATEG_MENU, &objnum)) cd_Signalize(cdp);
      else if (RV->inst) track_popup_menu(cdp, objnum, RV->inst->hWnd);
      break;
    }
    case ACT_SEL_PRINTER:
      if (!Printer_dialog(Active_view()))
        { wrestore_run(cdp);  r_error(ACTION_BREAK); }
      break;
    case ACT_PRINT_OPT:
      if (!Print_opt(Active_view()))
        { wrestore_run(cdp);  r_error(ACTION_BREAK); }
      break;
    case ACT_PAGE_SETUP:
      if (!Page_setup(Active_view()))
        { wrestore_run(cdp);  r_error(ACTION_BREAK); }
      break;
    case ACT_MSG_WINDOW:
      Info_box(attr.s, text.s);
      break;
    case ACT_MSG_STATUS:
      Set_status_text(text.s);
      break;
    case ACT_SHOW_HELP:
    { sig32 num;
      if (str2int(text.s, &num)) WB_help(cdp, HELP_CONTEXT,      num);
      break;
    }
    case ACT_SHOW_HELP_POPUP:
    { sig32 num;
      if (str2int(text.s, &num)) WB_help(cdp, HELP_CONTEXTPOPUP, num);
      break;
    }
    case ACT_EXPORT_TABLE:
    case ACT_EXPORT_CURSOR:
    { tptr fname;
      tcateg cat = (act_num==ACT_EXPORT_TABLE) ? CATEG_TABLE:CATEG_CURSOR;
      cutspaces(text.s);
      if (cd_Find_object(cdp, objname.s, cat, &objnum)) cd_Signalize(cdp);
      else
      { if ((*text.s=='?') || !*text.s)
        { char path[MAX_PATH] = "";
          fname=create_file(path, FILTER_TDT, *text.s ? text.s+1 : text.s, GetParent(RV->hClient), NULL);
          if (!fname) break;
        }
        else fname=text.s;
        Set_status_text(MAKEINTRESOURCE(STBR_IMPEXP));
        SetCursor(LoadCursor(NULL, IDC_WAIT));
        if (!Data_export(objnum, cat, fname, IMPEXP_FORMAT_WINBASE, 0))
          cd_Signalize(cdp);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        Set_status_text(NULLSTRING);
      }
      break;
    }
    case ACT_IMPORT_TABLE:
    { tptr fname;
      cutspaces(text.s);
      if (cd_Find_object(cdp, objname.s, CATEG_TABLE, &objnum))
        cd_Signalize(cdp);
      else
      { if ((*text.s=='?') || !*text.s)
        { char path[MAX_PATH] = "";
          fname=x_select_file(path, FILTER_TDT, *text.s ? text.s+1 : text.s, GetParent(RV->hClient), NULL);
          if (!fname) break;
        }
        else fname=text.s;
        Set_status_text(MAKEINTRESOURCE(STBR_IMPEXP));
        SetCursor(LoadCursor(NULL, IDC_WAIT));
        if (Restore_table(objnum, fname)) cd_Signalize(cdp);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        Set_status_text(NULLSTRING);
      }
      break;
    }
    case ACT_TABLE_VIEW:
    { char buf[30+OBJ_NAME_LEN];
      strcpy(buf, "DEFAULT `");
      strcat(buf, objname.s);
      strcat(buf, "` TABLEVIEW");
      cd_Open_view(cdp, buf, NO_REDIR, 0, 0, NULL, 0, NULL);
      break;
    }
    case ACT_CURSOR_VIEW:
    { char buf[30+OBJ_NAME_LEN];
      strcpy(buf, "DEFAULT `");
      strcat(buf, objname.s);
      strcat(buf, "` CURSORVIEW");
      cd_Open_view(cdp, buf, NO_REDIR, 0, 0, NULL, 0, NULL);
      break;
    }
    case ACT_FREE_DEL:
      if (cd_Find_object(cdp, objname.s, CATEG_TABLE, &objnum))
        cd_Signalize(cdp);
      else if (cd_Free_deleted(cdp, objnum)) cd_Signalize(cdp);
      break;
    case ACT_INSERT:
    { hView=Active_view();
      if (hView)
        if (SendMessage(hView, SZM_INSERT, 0, 0) != 2)
          { wrestore_run(cdp);  r_error(ACTION_BREAK); }
      break;
    }
    case ACT_QBE:
    { hView=get_active_view(cdp);  if (!hView) break;
      SendMessage(hView, SZM_QBE, 0, 0);
      break;
    }
    case ACT_ORDER:
    { hView=get_active_view(cdp);  if (!hView) break;
      SendMessage(hView, SZM_ORDER, 0, 0);
      break;
    }
    case ACT_UNLIMIT:
    { hView=get_active_view(cdp);  if (!hView) break;
      SendMessage(hView, SZM_UNLIMIT, 0, 0);
      break;
    }
    case ACT_ACCEPT_Q:
    { hView=get_active_view(cdp);  if (!hView) break;
      SendMessage(hView, SZM_ACCEPT_Q, 0, 0);
      break;
    }
    case ACT_INDEP_QBE:
    { char buf[1+OBJ_NAME_LEN+1];
      tptr p=check_aster(objname.s, buf);
      hView=get_active_view(cdp);  if (!hView) break;
      cd_Open_view(cdp, p, NO_REDIR, QUERY_VIEW|MODAL_VIEW, 0, NULL, hView, NULL);
      break;
    }
    case ACT_ATTR_SORT:
    { hView=get_active_view(cdp);  if (!hView) break;
      view_dyn * inst = GetViewInst(hView);  tptr new_select;
      uns32 subcurs=create_sort_subcurs(attr.s, inst, &new_select);
      insert_subcursor(inst, subcurs, new_select);
      break;
    }
    case ACT_TILE:
      SendMessage(RV->hClient, WM_MDITILE,        0, 0L); break;
    case ACT_CASCADE:
      SendMessage(RV->hClient, WM_MDICASCADE,     0, 0L); break;
    case ACT_ARRANGE:
      SendMessage(RV->hClient, WM_MDIICONARRANGE, 0, 0L); break;

    case ACT_FLOWUSER:
    { hView=get_active_view(cdp);  if (!hView) break;
      if (cd_Find_object(cdp, objname.s, CATEG_USER, &objnum))
        { cd_Signalize(cdp);  break; }
     set_user:
      { tcursnum curs;  trecnum irec, erec;
        Get_fcursor(hView, &curs, NULL);
        Get_view_pos(hView, &irec, &erec);
        if (cd_GetSet_next_user(cdp, curs, erec, 0, OPER_SET, VT_OBJNUM, &objnum))
          cd_Signalize(cdp);
        else
        { add_privils(cdp, curs, erec, objnum);
          Reset_view(hView, NORECNUM, RESET_ALL|RESET_CACHE);
        }
      }
      break;
    }
    case ACT_FLOWSEL:
    { hView=get_active_view(cdp);  if (!hView) break;
      objnum=Select_object(cdp, hView, CATEG_USER);
      if (objnum==NOOBJECT) break;
      goto set_user;
    }
    case ACT_FLOWSTOP:
    { hView=get_active_view(cdp);  if (!hView) break;
      tcursnum curs;  trecnum irec, erec;
      Get_fcursor(hView, &curs, NULL);
      Get_view_pos(hView, &irec, &erec);
      objnum = NOOBJECT;
      if (cd_GetSet_next_user(cdp, curs, erec, 0, OPER_SET, VT_OBJNUM, &objnum))
        cd_Signalize(cdp);
      else Reset_view(hView, NORECNUM, RESET_ALL|RESET_CACHE);
      break;
    }
    case ACT_PRIVILS:
      hView=get_active_view(cdp);  if (!hView) break;
      SendMessage(hView, SZM_PRIVILS, 0, 0);
      break;
    case ACT_TOKEN:
      hView=get_active_view(cdp);  if (!hView) break;
      SendMessage(hView, SZM_TOKEN, 0, 0);
      break;
    case ACT_REPLOUT:
      sp_replicate(cdp, FALSE);  break;
    case ACT_REPLIN:
      sp_replicate(cdp, TRUE);   break;
    case ACT_SHELL_EXEC:
      sp_shell_execute(attr.s, text.s);  break;
    case ACT_BEEP:
      MessageBeep(-1);  Sleep(100);  break;
    case ACT_TRANSPORT:
      if (cd_Find_object(cdp, objname.s, CATEG_PGMSRC, &objnum)) cd_Signalize(cdp);
      else Move_data(cdp, objnum, NULL, NOOBJECT, NULL, -1,-1,-1,-1, FALSE);
      break;
  }
}

static BOOL WINAPI sp_Edit_table(cdp_t cdp, const char * name)
{ tobjnum objnum = NOOBJECT;
  if (*name) cd_Find_object(cdp, name, CATEG_TABLE, &objnum);
  return lnk_Edit_table(cdp, 0, name, objnum); 
}

static BOOL WINAPI sp_Edit_privils(cdp_t cdp, HWND hParent, ttablenum tb, trecnum * recnums, int multioper)
// tb not passed by reference!
{ return Edit_privils (cdp, hParent, &tb, recnums, multioper); }

static DWORD WINAPI sp_MailBoxGetMsg(cdp_t cdp, HANDLE MailBox, DWORD MsgID)
{ return cd_MailBoxGetMsg(cdp, MailBox, MsgID, 0); }

static BOOL WINAPI sp_Export_to_XML(cdp_t cdp, const char * dad_ref, char * fname, tcursnum curs, struct t_clivar * hostvars, int hostvars_count)
{ return lnk_Export_to_XML(cdp, dad_ref, fname, curs, NULL, 0); }
static BOOL WINAPI sp_Export_to_XML_buffer(cdp_t cdp, const char * dad_ref, char * buffer, int bufsize, int * xmlsize, tcursnum curs, struct t_clivar * hostvars, int hostvars_count)
{ return lnk_Export_to_XML_buffer(cdp, dad_ref, buffer, bufsize, xmlsize, curs, NULL, 0); }
static BOOL WINAPI sp_Import_from_XML(cdp_t cdp, const char * dad_ref, const char * fname)
{ return lnk_Import_from_XML(cdp, dad_ref, fname, NULL, 0); }
static BOOL WINAPI sp_Import_from_XML_buffer(cdp_t cdp, const char * dad_ref, const char * buffer, int xmlsize)
{ return lnk_Import_from_XML_buffer(cdp, dad_ref, buffer, xmlsize, NULL, 0); }

#endif // WINS
#endif

static tobjname appl_name;
char * WINAPI Current_application(cdp_t cdp) // internal only
{ strcpy(appl_name, cdp->sel_appl_name);
  return appl_name;
}

BOOL WINAPI sp_SQL_exec_prepared(cdp_t cdp, uns32 handle)
{ return cd_SQL_exec_prepared(cdp, handle, NULL, NULL); }

static BOOL WINAPI sp_Get_server_error_suppl(cdp_t cdp, tptr buf)
{ strcpy(buf, cdp->errmsg);
  return FALSE;
}

tptr WINAPI sp_int2str(cdp_t cdp, sig32 val)
{ int32tostr   (val, cdp->RV.hyperbuf);              return cdp->RV.hyperbuf; }
tptr WINAPI sp_money2str(cdp_t cdp, monstr val, int prez)
{ money2str    (&val,cdp->RV.hyperbuf, (sig8)prez);  return cdp->RV.hyperbuf; }
tptr WINAPI sp_real2str(cdp_t cdp, double val, int prez)
{ real2str     (val, cdp->RV.hyperbuf, (sig8)prez);  return cdp->RV.hyperbuf; }
tptr WINAPI sp_date2str(cdp_t cdp, uns32 val, int prez)
{ date2str     (val, cdp->RV.hyperbuf, (sig8)prez);  return cdp->RV.hyperbuf; }
tptr WINAPI sp_time2str(cdp_t cdp, uns32 val, int prez)
{ time2str     (val, cdp->RV.hyperbuf, (sig8)prez);  return cdp->RV.hyperbuf; }
tptr WINAPI sp_timestamp2str(cdp_t cdp, uns32 val, int prez)
{ timestamp2str(val, cdp->RV.hyperbuf, (sig8)prez);  return cdp->RV.hyperbuf; }
tptr WINAPI sp_bigint2str(cdp_t cdp, sig64 val)
{ int64tostr   (val, cdp->RV.hyperbuf);              return cdp->RV.hyperbuf; }

BOOL WINAPI sp_Get_property_value(cdp_t cdp, const char * owner, const char * name, int num, char * buffer, unsigned buffer_size, uns32 * valsize)
{ return cd_Get_property_value(cdp, owner, name, num, buffer, buffer_size, NULL); }
BOOL WINAPI sp_Set_property_value(cdp_t cdp, const char * owner, const char * name, int num, const char * value, sig32 valsize)
{ return cd_Set_property_value(cdp, owner, name, num, value); }

static cdp_t WINAPI sp_get_cdp(cdp_t cdp)
{ return cdp; }

////////////////////////////////////////////// fulltext ///////////////////////////////////////////
#include "ftinc.h"
#include "ftinc.cpp"

#include "regstr.h"
#include "flstr.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "assert.h"
#define ZIP602_STATIC
#include "Zip602.h"
#ifndef WINS
#include <stdarg.h> // va_start and other for t_input_convertor::request_errorf()
#include <sys/wait.h> // waitpid() 
#include <limits.h> // INT_MAX
#define wcsicmp wcscasecmp
#endif
#include "ftconv.cpp"

struct t_ft_kontext : t_ft_kontext_base
{private: 
  char ft_label[2*OBJ_NAME_LEN+2];
  char * buf;
  enum { bufsize=10000 };
  unsigned bufpos;
  int posit;
  unsigned startpos;
 public:
  t_ft_kontext(const char * ft_labelIn, const char * limitsIn, const char * word_startersIn, const char * word_insidersIn, bool bigint_idIn);
  ~t_ft_kontext(void);
  BOOL ft_insert_word_occ(cdp_t cdp, t_docid64 docid, const char * word, unsigned position, unsigned weight, t_specif specif = t_specif());
  void ft_set_summary_item(cdp_t cdp, t_docid64 docid, unsigned infotype, const void * info);
  void ft_starting_document_part(cdp_t cdp, t_docid64 docid, unsigned position, const char * relative_path);
  bool breaked(cdp_t cdp) const
    { return false; }
  BOOL close(cdp_t cdp, t_docid64 docid);
};

t_ft_kontext::t_ft_kontext(const char * ft_labelIn, const char * limitsIn, const char * word_startersIn, const char * word_insidersIn, bool bigint_idIn) 
  : t_ft_kontext_base(limitsIn, word_startersIn, word_insidersIn, bigint_idIn)
{ strmaxcpy(ft_label, ft_labelIn, sizeof(ft_label));
  buf=(char*)corealloc(bufsize, 44);
  bufpos=0;  posit=-1;  startpos=0;
}

t_ft_kontext::~t_ft_kontext(void)
{ corefree(buf); }

BOOL t_ft_kontext::close(cdp_t cdp, t_docid64 docid)
{ if (!buf) return FALSE;  // initialization error
  if (bufpos)
    return !cd_Send_fulltext_words64(cdp, ft_label, docid, startpos, buf, bufpos);
  else return TRUE;
}

#ifdef LINUX  // temporary stub
struct FILETIME
{ int i;
};
#endif

void t_ft_kontext::ft_set_summary_item(cdp_t cdp, t_docid64 docid, unsigned infotype, const void * info)
// Sending like words, but with negative position in the document.
{ int infosize;
 // determine the size of the summary info to be sent to the server:
  if (infotype==PID_REVNUMBER)
    infosize=sizeof(int);
  else if (infotype==PID_CREATE_DTM_RO || infotype==PID_EDITTIME || infotype==PID_LASTSAVE_DTM)
    infosize=sizeof(FILETIME);
  else 
    infosize=(int)strlen((const char*)info)+1;
 // send it:
  cd_Send_fulltext_words64(cdp, ft_label, docid, (unsigned)-(int)infotype, (const char*)info, infosize);
}

BOOL t_ft_kontext::ft_insert_word_occ(cdp_t cdp, t_docid64 docid, const char * word, unsigned position, unsigned weight, t_specif specif)
{ char lword[MAX_FT_WORD_LEN+1];
  if (!buf) return FALSE;  // initialization error
  posit++;
  //lemmatize(language, word, lword, MAX_FT_WORD_LEN+1, 0);
  //if (simple_word(lword, 0)) return TRUE;
  // this will be done on the server side, lemmatisation not initialised here
  strmaxcpy(lword, word, sizeof(lword));
  int len=(int)strlen(lword);
  if (bufpos+len>=bufsize) 
  { if (cd_Send_fulltext_words64(cdp, ft_label, docid, startpos, buf, bufpos))
      return FALSE;
    bufpos=0;  startpos=posit;
  }
  strcpy(buf+bufpos, lword);  bufpos+=len+1;
  return TRUE;
}

void t_ft_kontext::ft_starting_document_part(cdp_t cdp, t_docid64 docid, unsigned position, const char * relative_path)
{
  cd_Send_fulltext_words64(cdp, ft_label, docid, position, relative_path, (unsigned)-1);
}

CFNC DllKernel BOOL WINAPI cd_Fulltext_index_doc(cdp_t cdp, const char * ft_label, t_docid32 docid, const char * filename, const char * format)
{ return cd_Fulltext_index_doc64(cdp, ft_label, docid, filename, format); }

#include "ftanal.cpp"

CFNC DllKernel BOOL WINAPI cd_Fulltext_index_doc64(cdp_t cdp, const char * ft_label, t_docid64 docid, const char * filename, const char * format)
{// analyse the fulltext label:
  tobjname schemaname, suffix, doctabname;
  if (!analyse_ft_label(cdp, ft_label, schemaname, suffix, doctabname))
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return FALSE; }
 // remove references to the original document:
  { char cmd[OBJ_NAME_LEN+100];
    if (*schemaname)
      sprintf(cmd, "DELETE FROM `%s`.`FTX_REFTAB%s` WHERE DOCID = ", schemaname, suffix);
    else  
      sprintf(cmd, "DELETE FROM `FTX_REFTAB%s` WHERE DOCID = ", suffix);
    int64tostr(docid, cmd+strlen(cmd));  
    if (cd_SQL_execute(cdp, cmd, NULL)) return FALSE;
  }
 // analyse the FT system and prepare the context:
  tobjname infoname;  bool is_v9_system=true;  tobjnum infonum;  
  strcpy(infoname, suffix);
  t_fulltext ftdef;
  if (cd_Find_prefixed_object(cdp, schemaname, infoname, CATEG_INFO, &infonum))  // trying the v9 FT name
  { strcpy(infoname, "FTX_DESCR");   strcat(infoname, suffix);  // v9 FT name not found, try the v8 FT name
    if (!cd_Find_prefixed_object(cdp, schemaname, infoname, CATEG_INFO, &infonum))  // trying the v9 FT name
      { SET_ERR_MSG(cdp, suffix);  client_error(cdp, OBJECT_NOT_FOUND);  return FALSE; }    
    is_v9_system=false;  //  ... even if not found
  }
  tptr info = cd_Load_objdef(cdp, infonum, CATEG_INFO, NULL);
  if (!info) return FALSE;
  if (is_v9_system)
  { int err = ftdef.compile(cdp, info);
    corefree(info);
    if (err) return FALSE;
  }
  else
  { //get_info_value(info, NULLSTRING, "language",   ATT_INT32, &language);  // protected against info==NULL
    //get_info_value(info, NULLSTRING, "basic_form", ATT_INT32, &basic_form);
    //get_info_value(info, NULLSTRING, "weighted",   ATT_INT32, &weighted);
    //int bi;
    //if (get_info_value(info, NULLSTRING, "bigint_id",  ATT_INT32, &bi))
    //  ftdef.bigint_id = bi==1;
    corefree(info);
  }
 // indexing: 
  t_ft_kontext ftk(ft_label, ftdef.limits, ftdef.word_starters, ftdef.word_insiders, ftdef.bigint_id);
  t_agent_indexing agent_data(cdp, docid, &ftk);  
  if (is_v9_system)
  {// prepare the input stream:
   t_input_stream * is;
   is = new t_input_stream_file(cdp, filename, filename);
   if (!is) { client_error(cdp, OUT_OF_MEMORY);  return FALSE; }
  // prepare the convertor:
   t_input_convertor * ic = ic_from_format(cdp, format);
   t_ft_limits_evaluator limits_evaluator(cdp, ftdef.limits);
   if (!ic)
   { ic = ic_from_text(cdp,is);
     is->reset();
     if (!ic)
       { delete is;  SET_ERR_MSG(cdp, "<Unknown format>");  client_error(cdp, CONVERSION_NOT_SUPPORTED);  return FALSE; }
   }
   ic->set_stream(is);
   ic->set_agent(&agent_data);
   ic->set_depth_level(1);
   ic->set_limits_evaluator(&limits_evaluator);
   ic->set_owning_ft(&ftk);
   bool res = ic->convert() && !is->was_any_error() && !ic->was_any_error();
   delete ic;
   delete is;
   ftk.close(cdp, docid);
   return res;
  }
  else
  { 
#ifdef STOP
   // analyse file format:
    char libname[8+1];  
    get_format_library(filename, format, libname);
   // read the text:
    if (!*libname)  // plain text
    { HANDLE hnd=CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
      if (!FILE_HANDLE_VALID(hnd)) return FALSE;
      char buf[1024];  DWORD rd;
      while (ReadFile(hnd, buf, sizeof(buf), &rd, NULL) && rd) 
        agentConvert97Callback(&agent_data, C97_INS_TEXT, (T_CC97ARG*)buf, *(T_CC97ARG*)&rd, NULL, NULL);
      CloseHandle(hnd);
      return ftk.close(cdp, docid);
    }
    else // must convert the text
#endif
    { BOOL result=FALSE;
     // load the conversion routine:
      HINSTANCE hXDll;
#ifdef WINS
      IMPORTFILE * lpFnConvert = get_conversion_routine(/*libname*/"wb_ximp", hXDll);
      if (!lpFnConvert)
        if (!hXDll)
          { SET_ERR_MSG(cdp, /*libname*/"wb_ximp");  client_error(cdp, LIBRARY_NOT_FOUND);  return FALSE; }
        else
          { client_error(cdp, SQ_EXT_ROUT_NOT_AVAIL);  FreeLibrary(hXDll);  return FALSE; }
      FILEERROR sError;  sError = (*lpFnConvert)(&agent_data, filename, agentConvert97Callback);
      FreeLibrary(hXDll);
#else
      FILEERROR sError=convert_HTML_file(&agent_data, filename, agentConvert97Callback);
#endif
      return !sError && ftk.close(cdp, docid);
    }
  } 
}

BOOL WINAPI Fulltext_index_doc(const char * ft_label, t_docid32 docid, const char * filename, const char * format)
{ return cd_Fulltext_index_doc(GetCurrTaskPtr(), ft_label, docid, filename, format); }

//////////////////////////////////////////////////////////////////////////////////////////////
void WINAPI sp_empty(void) {}

double WINAPI sp_sin (double arg)  { return arg==NULLREAL ? NULLREAL : sin (arg); }
double WINAPI sp_cos (double arg)  { return arg==NULLREAL ? NULLREAL : cos (arg); }
double WINAPI sp_atan(double arg)  { return arg==NULLREAL ? NULLREAL : atan(arg); }
double WINAPI sp_exp (double arg)  { return arg==NULLREAL ? NULLREAL : exp (arg); }
double WINAPI sp_log (double arg)  { return arg==NULLREAL ? NULLREAL : log (arg); }
double WINAPI sp_sqrt(double arg)  { return arg==NULLREAL ? NULLREAL : sqrt(arg); }

CFNC DllKernel BOOL WINAPI Pref(const char * s1, const char * s2)
{ return general_Pref(s1, s2, t_specif(0xffff,0,FALSE,FALSE)); }

CFNC DllKernel BOOL WINAPI Substr(const char * s1, const char * s2)
{ return general_Substr(s1, s2, t_specif(0xffff,0,FALSE,FALSE))!=0; }

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

CFNC DllKernel BOOL WINAPI Like(const char * s1, const char * s2)
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

int WINAPI sp_strlenght(tptr s)
{ return (int)strlen(s); }

tptr WINAPI sp_strtrim(tptr s)
{ if (s) cutspaces(s);
  return s;
}

void WINAPI Set_client_property(int propnum, const char * val)
{ switch (propnum)
  { case 0:
      strmaxcpy(explic_decim_separ, val, sizeof(explic_decim_separ));
      break;
  }
}

void WINAPI Info_box_replacement(const char * caption, const char * message)
{
  client_error_message(NULL, "Info: %s: %s", caption, message);
}

void WINAPI Signalize_replacement(cdp_t cdp)
{
  int num = cd_Sz_error(cdp);
  if (num)
    client_error_param(cdp, cd_Sz_error(cdp));  // parameters are substituted in Get_error_num_text
}

#if defined LINUX || (WBVERS>=900)
#define sp_get_view_item         sp_empty     /* 0*/
#define sp_setDlgItemText         sp_empty
#define Reset_view         sp_empty
#define Main_menu         sp_empty
#define sp_open_view          sp_empty
#define sp_print_view         sp_empty
#define Close_view         sp_empty
#define sp_bind_records         sp_empty
#define sp_select_records         sp_empty
#define Set_int_pos         sp_empty
#define Set_ext_pos         sp_empty       /*10*/
#define Get_view_pos         sp_empty
#define sp_edit_text         sp_empty
#define Pick_window         sp_empty
#define Put_pixel         sp_empty
#define Draw_line         sp_empty
#define Commit_view         sp_empty
#define cd_Alogin         sp_empty
#define Print_opt         sp_empty
#define Roll_back_view         sp_empty
#define Set_printer         sp_empty       /*20*/
#define sp_exec         sp_empty
#define sp_get_message         sp_empty
#define Info_box         Info_box_replacement
#define Yesno_box         sp_empty
#define sp_replicate         sp_empty
#define Get_fcursor         sp_empty
#define Set_fcursor         sp_empty
#define sp_viewclose         sp_empty
#define sp_viewopen         sp_empty
#define sp_viewprint         sp_empty         /*30*/
#define sp_viewstate         sp_empty
#define sp_viewtoggle         sp_empty
#define sp_peek_message         sp_empty
#define Printer_dialog         sp_empty
#define sp_set_cursor         sp_empty
#define Set_status_nums         sp_empty
#define Set_status_text         sp_empty
#define From_xbase         sp_empty
#define To_xbase         sp_empty
#define Print_margins         sp_empty        /*40*/
#define Current_item         sp_empty
#define Active_view         sp_empty
#define lnk_Export_appl_ex  sp_empty
#define Close_all_views         sp_empty
#define Input_box         sp_empty
#define Print_copies         sp_empty
#define Show_help         sp_empty
#define cd_Help_file         sp_empty
#ifndef WINS
#define SendMessage         sp_empty
#endif
#define sp_action         sp_empty            /*50*/
#define Printer_select         sp_empty
#define Select_file         sp_empty
#define sp_relate_record         sp_empty
#define sp_get_item_value         sp_empty
#define sp_set_item_value         sp_empty
#define sp_register_key         sp_empty
#define sp_get_ext_message         sp_empty
#define Data_export         sp_empty
#define Data_import         sp_empty
#define sp_Edit_table       sp_empty
#define Register_rec_syn         sp_empty
#define Select_directory         sp_empty
#define QBE_state         sp_empty
#define Acreate_user         sp_empty
#define Next_user_name         sp_empty
#define Signature_state         sp_empty
#define Tab_page         sp_empty
#define Page_setup         sp_empty
#define Show_help_popup         sp_empty      /*70*/
#define lnk_Edit_view         sp_empty
#define AToken_control         sp_empty
#define Edit_relation         sp_empty
#define sp_Edit_privils         sp_empty
#define Aset_password         sp_empty
#define Amodify_user         sp_empty
#define lnk_Edit_query         sp_empty
#define Edit_impexp         sp_empty

#define dynmem_dispose         sp_empty
#define dynmem_new         sp_empty
#define dynmem_addr         sp_empty
#define Set_timer        sp_empty
#define View_pattern    sp_empty
#define Set_first_label    sp_empty
#define Restore_table    sp_empty
#define Create_link    sp_empty
#define ODBC_create_connection    sp_empty
#define ODBC_find_connection    sp_empty
#define ODBC_direct_connection    sp_empty
#define ODBC_open_cursor    sp_empty
#define Token_control    sp_empty
#define Create2_link    sp_empty
#define sp_Db2file    sp_empty
#define sp_File2db3    sp_empty
#define IsViewChanged sp_empty
#define SelectIntRec sp_empty
#define SelectExtRec sp_empty
#define IsIntRecSelected sp_empty
#define IsExtRecSelected sp_empty
#define ViewRecCount sp_empty

#define cd_ODBC_direct_connection sp_empty
#define cd_ODBC_find_connection sp_empty
#define cd_Create_link sp_empty
#define cd_ODBC_open_cursor sp_empty
#define cd_ODBC_create_connection sp_empty
#define cd_Create2_link sp_empty
#define ASkip_repl sp_empty

#define sp_Import_from_XML sp_empty
#define sp_Import_from_XML_buffer sp_empty
#define sp_Export_to_XML sp_empty
#define sp_Export_to_XML_buffer sp_empty
#define cd_ODBC_exec_direct sp_empty
#define Background_bitmap sp_empty
#endif

#if WBVERS>=900
#define Move_data sp_empty
#define cd_Save_table sp_empty
#define Repl_appl_shared sp_empty
#endif

#if defined LINUX || (WBVERS>=900)
#define cd_Signalize Signalize_replacement
#endif
/***************************** tables ***************************************/
void (*sproc_table[])(void) = {
(void (*)(void))sp_get_view_item,     /* 0*/
(void (*)(void))sp_setDlgItemText,
(void (*)(void))Reset_view,
(void (*)(void))Main_menu,
(void (*)(void))sp_open_view ,
(void (*)(void))sp_print_view,
(void (*)(void))Close_view,
(void (*)(void))sp_bind_records,
(void (*)(void))sp_select_records,
(void (*)(void))Set_int_pos,
(void (*)(void))Set_ext_pos,       /*10*/
(void (*)(void))Get_view_pos,
(void (*)(void))sp_edit_text,
(void (*)(void))Pick_window,
(void (*)(void))Put_pixel,
(void (*)(void))Draw_line,
(void (*)(void))Commit_view,
(void (*)(void))cd_Alogin,
(void (*)(void))Print_opt,
(void (*)(void))Roll_back_view,
(void (*)(void))Set_printer,       /*20*/
(void (*)(void))sp_exec,
(void (*)(void))sp_get_message,
(void (*)(void))Info_box,
(void (*)(void))Yesno_box,
(void (*)(void))sp_replicate,
(void (*)(void))Get_fcursor,
(void (*)(void))Set_fcursor,
(void (*)(void))sp_viewclose,
(void (*)(void))sp_viewopen,
(void (*)(void))sp_viewprint,         /*30*/
(void (*)(void))sp_viewstate,
(void (*)(void))sp_viewtoggle,
(void (*)(void))sp_peek_message,
(void (*)(void))Printer_dialog,
(void (*)(void))sp_set_cursor,
(void (*)(void))Set_status_nums,
(void (*)(void))Set_status_text,
(void (*)(void))From_xbase,
(void (*)(void))To_xbase,
(void (*)(void))Print_margins,        /*40*/
(void (*)(void))Current_item,
(void (*)(void))Active_view,
(void (*)(void))lnk_Export_appl_ex,
(void (*)(void))Close_all_views,
(void (*)(void))Input_box,
(void (*)(void))Print_copies,
(void (*)(void))Show_help,
(void (*)(void))cd_Help_file,
(void (*)(void))SendMessage,
(void (*)(void))sp_action,            /*50*/
(void (*)(void))Printer_select,
(void (*)(void))Select_file,
(void (*)(void))sp_relate_record,
(void (*)(void))sp_get_item_value,
(void (*)(void))sp_set_item_value,
(void (*)(void))sp_register_key,
(void (*)(void))sp_get_ext_message,
(void (*)(void))Data_export,
(void (*)(void))Data_import,
(void (*)(void))sp_Edit_table,        /*60*/
(void (*)(void))Register_rec_syn,
(void (*)(void))Select_directory,
(void (*)(void))QBE_state,
(void (*)(void))Acreate_user,
(void (*)(void))Next_user_name,
(void (*)(void))Signature_state,
(void (*)(void))Move_data,
(void (*)(void))Tab_page,
(void (*)(void))Page_setup,
(void (*)(void))Show_help_popup,      /*70*/
(void (*)(void))lnk_Edit_view,
(void (*)(void))Chng_component_flag,
(void (*)(void))AToken_control,
(void (*)(void))Edit_relation,
(void (*)(void))sp_Edit_privils,
(void (*)(void))Aset_password,
(void (*)(void))Amodify_user,
(void (*)(void))lnk_Edit_query,
(void (*)(void))Edit_impexp,

(void (*)(void))Make_date    ,      /*80*/
(void (*)(void))Day          ,
(void (*)(void))Month        ,
(void (*)(void))Year         ,
(void (*)(void))Today        ,
(void (*)(void))Make_time    ,
(void (*)(void))Hours        ,
(void (*)(void))Minutes      ,
(void (*)(void))Seconds      ,
(void (*)(void))Sec1000      ,
(void (*)(void))Now          ,      /*90*/
(void (*)(void))sp_ord       ,
(void (*)(void))sp_chr       ,
(void (*)(void))sp_odd       ,
(void (*)(void))sp_abs       ,
(void (*)(void))sp_iabs      ,
(void (*)(void))sp_sqrt      ,
(void (*)(void))sp_round     ,
(void (*)(void))sp_trunc     ,
(void (*)(void))sp_sqr       ,
(void (*)(void))sp_isqr      ,      /*100*/
(void (*)(void))sp_sin       ,
(void (*)(void))sp_cos       ,
(void (*)(void))sp_atan      ,
(void (*)(void))sp_log       ,
(void (*)(void))sp_exp       ,
(void (*)(void))Like         ,
(void (*)(void))Pref         ,
(void (*)(void))Substr       ,
(void (*)(void))sp_strcat    ,
(void (*)(void))Upcase9      ,      /*110*/
(void (*)(void))sp_memmov    ,
(void (*)(void))sp_str2int   ,
(void (*)(void))sp_str2money ,
(void (*)(void))sp_str2real  ,
(void (*)(void))sp_int2str   ,
(void (*)(void))sp_money2str ,
(void (*)(void))sp_real2str  ,
(void (*)(void))sp_reset     ,
(void (*)(void))sp_rewrite   ,
(void (*)(void))sp_close     ,      /*120*/
(void (*)(void))skipspaces   ,
(void (*)(void))sp_seek      ,
(void (*)(void))sp_filelength,
(void (*)(void))sp_strinsert ,
(void (*)(void))sp_strdelete ,
(void (*)(void))sp_strcopy   ,
(void (*)(void))sp_strpos    ,
(void (*)(void))sp_strlenght ,
(void (*)(void))sp_str2date  ,
(void (*)(void))sp_str2time  ,      /*130*/
(void (*)(void))sp_date2str  ,
(void (*)(void))sp_time2str  ,
(void (*)(void))Day_of_week  ,
(void (*)(void))sp_strtrim   ,
(void (*)(void))sp_delete_file   ,
(void (*)(void))sp_make_directory,
(void (*)(void))cd_WinBase602_version,
(void (*)(void))Quarter,
(void (*)(void))sp_empty,  //Waits_for_me,
(void (*)(void))View_pattern,       /*140*/
(void (*)(void))Set_first_label,
(void (*)(void))dynmem_dispose,
(void (*)(void))dynmem_new,
(void (*)(void))dynmem_addr,
(void (*)(void))datetime2timestamp,
(void (*)(void))timestamp2date,
(void (*)(void))timestamp2time,
(void (*)(void))is_repl_destin,
(void (*)(void))Set_timer,

(void (*)(void))sp_str2timestamp,   /*150*/
(void (*)(void))sp_timestamp2str,
(void (*)(void))cd_Sz_error    ,
(void (*)(void))cd_Sz_warning  ,
(void (*)(void))sp_err_mask  ,
(void (*)(void))cd_Login       ,
(void (*)(void))cd_Logout      ,
(void (*)(void))cd_Start_transaction,
(void (*)(void))cd_Commit      ,
(void (*)(void))cd_Roll_back   ,
(void (*)(void))sp_open_curs ,      /*160*/
(void (*)(void))sp_close_curs,
(void (*)(void))sp_open_sql_c,
(void (*)(void))sp_open_sql_p,
(void (*)(void))sp_restrict_curs,
(void (*)(void))sp_restore_curs,
(void (*)(void))cd_Rec_cnt   ,
(void (*)(void))cd_Delete    ,
(void (*)(void))cd_Undelete  ,
(void (*)(void))cd_Free_deleted,
(void (*)(void))cd_Append    ,      /*170*/
(void (*)(void))cd_Insert    ,
(void (*)(void))cd_Translate   ,
(void (*)(void))sp_empty, // was Define_table
(void (*)(void))cd_Find_object,
(void (*)(void))cd_Save_table,
(void (*)(void))Restore_table,
(void (*)(void))Get_data_rights,
(void (*)(void))Get_object_rights,
(void (*)(void))Set_data_rights,
(void (*)(void))Set_object_rights,  /*180*/
(void (*)(void))cd_Read_lock_table,
(void (*)(void))cd_Write_lock_table,
(void (*)(void))cd_Read_unlock_table,
(void (*)(void))cd_Write_unlock_table,
(void (*)(void))cd_Read_lock_record,
(void (*)(void))cd_Write_lock_record,
(void (*)(void))cd_Read_unlock_record,
(void (*)(void))cd_Write_unlock_record,
(void (*)(void))cd_Add_record,
(void (*)(void))cd_Super_recnum,       /*190*/
(void (*)(void))cd_Text_substring,
(void (*)(void))cd_Uninst_table,
(void (*)(void))cd_Set_password,
(void (*)(void))cd_Insert_object,
(void (*)(void))cd_Signalize,
(void (*)(void))cd_Create_link,
(void (*)(void))cd_Relist_objects,
(void (*)(void))cd_Who_am_I,      /*198*/
(void (*)(void))cd_Delete_all_records,
(void (*)(void))sp_user_in_group,   /*200*/
(void (*)(void))User_to_group,
(void (*)(void))cd_Enable_index,
(void (*)(void))Create_group,
(void (*)(void))sp_sql_submit,
(void (*)(void))sp_c_sum,
(void (*)(void))sp_c_avg,
(void (*)(void))sp_c_max,
(void (*)(void))sp_c_min,
(void (*)(void))cd_C_count,
(void (*)(void))sp_look_up,         /*210*/
#if WBVERS<=1000
(void (*)(void))cd_Available_memory,
#else
(void (*)(void))sp_empty,  
#endif
(void (*)(void))cd_Owned_cursors,
(void (*)(void))cd_Database_integrity,
(void (*)(void))cd_ODBC_create_connection,
(void (*)(void))cd_ODBC_find_connection,
(void (*)(void))cd_ODBC_direct_connection,
(void (*)(void))cd_ODBC_open_cursor,   /* 217*/
(void (*)(void))cd_GetSet_privils,
(void (*)(void))cd_GetSet_group_role,
(void (*)(void))cd_GetSet_next_user, /*220*/
(void (*)(void))cd_Am_I_db_admin,
(void (*)(void))cd_Find_object_by_id,
(void (*)(void))Current_application,
(void (*)(void))cd_GetSet_fil_size,
(void (*)(void))cd_waiting,
(void (*)(void))cd_Create_user,
(void (*)(void))cd_Enable_task_switch,
(void (*)(void))Token_control,
(void (*)(void))cd_Create2_link,
(void (*)(void))cd_SQL_prepare,      /*230*/
(void (*)(void))sp_SQL_exec_prepared,
(void (*)(void))cd_SQL_drop,
(void (*)(void))cd_Set_sql_option,
(void (*)(void))cd_Compact_table,
(void (*)(void))cd_Compact_database,
(void (*)(void))cd_Log_write,
(void (*)(void))cd_Appl_inst_count,
(void (*)(void))cd_Set_appl_starter,
(void (*)(void))cd_Set_progress_report_modulus,
(void (*)(void))cd_Get_logged_user, /*240*/
#ifdef WINS
(void (*)(void))cd_Connection_speed_test,
#else
(void (*)(void))sp_empty,
#endif
(void (*)(void))cd_Message_to_clients,
(void (*)(void))cd_GetSet_fil_blocks,
#ifndef WINS
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
#else
(void (*)(void))cd_InitWBMail,
(void (*)(void))cd_InitWBMail602,
(void (*)(void))cd_CloseWBMail,
(void (*)(void))cd_LetterCreate,
(void (*)(void))cd_LetterAddAddr,
(void (*)(void))cd_LetterAddFile,
(void (*)(void))cd_LetterSend,      /*250*/
(void (*)(void))cd_TakeMailToRemOffice,
(void (*)(void))cd_LetterCancel,
#endif
(void (*)(void))cd_Used_memory,
(void (*)(void))cd_Replicate,
(void (*)(void))cd_Repl_control,
(void (*)(void))cd_Set_transaction_isolation_level,
#ifdef WINS
(void (*)(void))Background_bitmap,
#else
(void (*)(void))sp_empty,
#endif
#if defined LINUX || (WBVERS>=900)
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
#else
(void (*)(void))cd_MailOpenInBox,
(void (*)(void))cd_MailBoxLoad,
(void (*)(void))sp_MailBoxGetMsg,  /*260*/
(void (*)(void))cd_MailBoxGetFilInfo,
(void (*)(void))cd_MailBoxSaveFileAs,
(void (*)(void))cd_MailBoxDeleteMsg,
(void (*)(void))cd_MailGetInBoxInfo,
(void (*)(void))cd_MailGetType,
(void (*)(void))cd_MailCloseInBox,
(void (*)(void))cd_MailDial,
(void (*)(void))cd_MailHangUp,
#endif
(void (*)(void))cd_Fulltext_index_doc,
(void (*)(void))sp_Get_server_error_suppl,  // 270
(void (*)(void))Get_server_error_context,
(void (*)(void))Get_error_num_text,
(void (*)(void))cd_Get_server_info,
(void (*)(void))sp_Db2file,
(void (*)(void))sp_File2db3,
(void (*)(void))IsViewChanged,
(void (*)(void))sp_bigint2str,
(void (*)(void))sp_str2bigint,
(void (*)(void))Move_obj_to_folder,
(void (*)(void))Current_timestamp,           // 280
(void (*)(void))SelectIntRec,
(void (*)(void))SelectExtRec,
(void (*)(void))IsIntRecSelected,
(void (*)(void))IsExtRecSelected,
(void (*)(void))ViewRecCount,
#ifdef WINS
(void (*)(void))unload_library,
(void (*)(void))cd_MailBoxGetMsg,
(void (*)(void))cd_LetterAddBLOBr,
(void (*)(void))cd_LetterAddBLOBs,
(void (*)(void))cd_MailBoxSaveFileDBr,          // 290
(void (*)(void))cd_MailBoxSaveFileDBs,
#else
(void (*)(void))sp_empty, 
(void (*)(void))sp_empty,
(void (*)(void))cd_LetterAddBLOBr,
(void (*)(void))cd_LetterAddBLOBs,
(void (*)(void))sp_empty,                       // 290
(void (*)(void))sp_empty,
#endif
(void (*)(void))Repl_appl_shared,
(void (*)(void))sp_Get_property_value,
(void (*)(void))sp_Set_property_value,
(void (*)(void))cd_Get_sql_option,
(void (*)(void))cd_MailCreateProfile,
(void (*)(void))cd_MailDeleteProfile,
(void (*)(void))cd_MailSetProfileProp,
(void (*)(void))cd_MailGetProfileProp,
(void (*)(void))cd_Invalidate_cached_table_info, // 300
(void (*)(void))cd_Am_I_config_admin,
(void (*)(void))ASkip_repl,
(void (*)(void))cd_Skip_repl,
(void (*)(void))Set_client_property,
(void (*)(void))cd_Rolled_back,
(void (*)(void))cd_Transaction_open,
(void (*)(void))sp_get_cdp,
(void (*)(void))sp_Import_from_XML,
(void (*)(void))sp_Import_from_XML_buffer,
(void (*)(void))sp_Export_to_XML,               // 310
(void (*)(void))sp_Export_to_XML_buffer,
(void (*)(void))cd_Truncate_table,
(void (*)(void))sp_sql_value,
(void (*)(void))cd_ODBC_exec_direct,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))sp_empty,
(void (*)(void))cd_InitWBMailEx,
};

wbbool has_cdp_param[] = {
  wbtrue ,wbfalse,wbfalse,wbfalse,wbtrue, wbtrue, wbfalse,wbtrue, wbfalse,wbfalse,  //  0
  wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbtrue, wbfalse,wbfalse,  // 10
  wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbtrue, wbfalse,wbfalse,wbfalse,wbtrue,   // 20
  wbtrue, wbfalse,wbtrue, wbtrue, wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,  // 30
  wbfalse,wbfalse,wbfalse,wbtrue, wbfalse,wbfalse,wbfalse,wbfalse,wbtrue, wbfalse,  // 40
  wbtrue, wbfalse,wbfalse,wbfalse,wbtrue, wbtrue, wbtrue, wbtrue, wbfalse,wbfalse,  // 50
  wbtrue, wbfalse,wbfalse,wbfalse,wbtrue, wbtrue, wbtrue, wbtrue, wbfalse,wbfalse,  // 60
  wbfalse,wbtrue, wbtrue, wbtrue, wbtrue, wbtrue, wbtrue, wbtrue, wbtrue, wbtrue,   // 70

  wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,  // 80
  wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,  // 90
  wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,  //100
  wbtrue, wbfalse,wbfalse,wbfalse,wbfalse,wbtrue, wbtrue, wbtrue, wbtrue, wbtrue,   //110
  wbtrue, wbtrue ,wbtrue ,wbtrue ,wbfalse,wbfalse,wbtrue, wbfalse,wbfalse,wbfalse,  //120
  wbfalse,wbtrue, wbtrue, wbfalse,wbfalse,wbfalse,wbfalse,wbtrue, wbfalse,wbfalse,  //130
  wbfalse,wbfalse,wbfalse,wbtrue, wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbtrue ,  //140
  wbfalse,wbtrue, wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //150
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //160
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue, wbfalse,wbfalse,wbfalse,wbfalse,  //170
  wbfalse,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //180
  wbtrue ,wbtrue ,wbtrue, wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //190
  wbtrue ,wbfalse,wbtrue ,wbfalse,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //200
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //210
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbfalse,wbtrue ,  //220
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //230
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //240
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //250
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //260
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbfalse,wbtrue, wbfalse,wbtrue ,  //270
  wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbfalse,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //280
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //290
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbfalse,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //300
  wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,wbtrue ,  //310
};

uns8 sfnc_table[] = {
 ATT_STRING                    /* 0*/
,0
,0
,ATT_BOOLEAN
,ATT_WINID
,ATT_BOOLEAN
,0
,ATT_WINID
,ATT_WINID
,ATT_BOOLEAN
,ATT_BOOLEAN                   /*10*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,0
,0
,0
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,0
,0                             /*20*/
,ATT_INT32
,ATT_BOOLEAN
,0
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,0
,0
,ATT_BOOLEAN                   /*30*/
,ATT_BOOLEAN
,0
,ATT_BOOLEAN
,0
,ATT_INT32
,0
,0
,ATT_BOOLEAN
,ATT_BOOLEAN
,0                             /*40*/
,ATT_INT32
,ATT_WINID
,ATT_BOOLEAN
,0
,ATT_BOOLEAN
,ATT_BOOLEAN
,0
,0
,ATT_INT32
,0                             /*50*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_WINID
,ATT_BOOLEAN
,ATT_BOOLEAN
,0
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN                   /*60*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_INT32
,ATT_BOOLEAN
,ATT_STRING
,ATT_INT32
,ATT_BOOLEAN
,ATT_INT32
,ATT_BOOLEAN
,0                             /*70*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,0

,ATT_DATE                      /*80*/
,ATT_WINID
,ATT_WINID
,ATT_WINID
,ATT_DATE
,ATT_TIME
,ATT_WINID
,ATT_WINID
,ATT_WINID
,ATT_WINID
,ATT_TIME                      /*90*/
,ATT_WINID
,ATT_CHAR
,ATT_BOOLEAN
,ATT_FLOAT
,ATT_INT32
,ATT_FLOAT
,ATT_INT32
,ATT_INT32
,ATT_FLOAT
,ATT_INT32                     /*100*/
,ATT_FLOAT
,ATT_FLOAT
,ATT_FLOAT
,ATT_FLOAT
,ATT_FLOAT
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_STRING
,0                             /*110*/
,0
,ATT_INT32
,ATT_MONEY
,ATT_FLOAT
,ATT_STRING
,ATT_STRING
,ATT_STRING
,ATT_BOOLEAN
,ATT_BOOLEAN
,0                             /*120*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_INT32
,ATT_STRING
,ATT_STRING
,ATT_STRING
,ATT_WINID
,ATT_WINID
,ATT_DATE
,ATT_TIME                      /*130*/
,ATT_STRING
,ATT_STRING
,ATT_WINID
,ATT_STRING
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_INT32
,ATT_WINID
,ATT_BOOLEAN
,0                             /*140*/
,0
,0
,0
,ATT_NIL
,ATT_TIMESTAMP
,ATT_DATE
,ATT_TIME
,ATT_BOOLEAN
,0
,ATT_TIMESTAMP                 /*150*/
,ATT_STRING

,ATT_INT16                     /*152*/
,ATT_INT16
,0
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN                   /*160*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_INT32                     /*170*/
,ATT_INT32
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN                   /*180*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN                   /*190*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_STRING
,ATT_BOOLEAN
,ATT_BOOLEAN                   /*200*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_INT32                     /*210*/
,ATT_INT32
,ATT_WINID
,ATT_BOOLEAN
,ATT_INT32
,ATT_INT32
,ATT_INT32
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN                   /*220*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_STRING
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,0
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN                   /*230*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,0
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN                   /*240*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_INT32
,ATT_INT32
,0
,ATT_INT32
,ATT_INT32
,ATT_INT32
,ATT_INT32                     /*250*/
,ATT_INT32
,0
,ATT_INT32
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_INT32
,ATT_INT32
,ATT_INT32                    /*260*/
,ATT_INT32
,ATT_INT32
,ATT_INT32
,ATT_INT32
,ATT_INT32
,0        
,ATT_INT32
,ATT_INT32
,ATT_BOOLEAN
,ATT_BOOLEAN                  /*270*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_STRING
,ATT_INT64
,ATT_BOOLEAN
,ATT_TIMESTAMP               /*280*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_INT32
,ATT_BOOLEAN
,ATT_INT32
,ATT_INT32
,ATT_INT32
,ATT_INT32                   /*290*/
,ATT_INT32
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_INT32
,ATT_INT32
,ATT_INT32
,ATT_INT32
,0                           /*300*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN
,0
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_INT32
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_BOOLEAN                 /*310*/
,ATT_BOOLEAN
,ATT_BOOLEAN
,ATT_STRING
,ATT_BOOLEAN
};
/****************************** obsolete functions *************************/

#if WBVERS<900
CFNC DllKernel BOOL WINAPI cd_Save_table(cdp_t cdp, ttablenum table, const char * filename)
{ return !Move_data(cdp, NOOBJECT, NULL, table, filename, 10, // 10 = table, dircurs
    IMPEXP_FORMAT_WINBASE, -1, 0, FALSE);
} // used by application export

#ifdef WINS

CFNC DllKernel BOOL WINAPI Save_table(ttablenum table, const char * filename)
{ return cd_Save_table(GetCurrTaskPtr(), table, filename); }

CFNC DllKernel BOOL WINAPI Restore_table(ttablenum table, const char * filename)
{ char tabname[OBJ_NAME_LEN+1], schemaname[OBJ_NAME_LEN+1+OBJ_NAME_LEN+1];
  uns8 appl_id[UUID_SIZE];
  cdp_t cdp = GetCurrTaskPtr();
  if (cd_Read(cdp, TAB_TABLENUM, (trecnum)table, OBJ_NAME_ATR,  NULL, tabname))
    return TRUE;
  if (cd_Read(cdp, TAB_TABLENUM, (trecnum)table, APPL_ID_ATR,   NULL, appl_id))
    return TRUE;
  cd_Apl_id2name(cdp, appl_id, schemaname);
  strcat(schemaname, ".");
  strcat(schemaname, tabname);
  return !Data_import(schemaname, FALSE, filename, IMPEXP_FORMAT_WINBASE, 0);
}
#endif
#endif

/////////////////////////////////// view methods //////////////////////////////
#ifdef WINS
#if WBVERS<900

char * WINAPI vwpropg_caption(view_dyn * inst)
{ tptr p = inst->cdp->RV.hyperbuf;
  if (inst->hWnd) GetWindowText(inst->hWnd, p, 256);
  else 
  { tptr pp = get_var_value(&inst->stat->vwCodeSize, VW_CAPTION);
    if (pp==NULL) *p=0; else strcpy(p, pp);
  }
  return p;
}
void WINAPI vwprops_caption(view_dyn * inst, tptr caption)
{ if (!inst->hWnd) return; // nelze nastavit
  SetWindowText(inst->hWnd, caption); 
}

void GetViewRect(view_dyn * inst, RECT * Rect)
{ if (inst->hWnd)
  { GetWindowRect(inst->hWnd, Rect);
    Rect->right -=Rect->left;  Rect->bottom-=Rect->top;
    if (inst->view_mode==VWMODE_SUBVIEW || inst->view_mode==VWMODE_CHILD || 
        inst->view_mode==VWMODE_OLECTRL || inst->view_mode==VWMODE_MDI)
    { POINT pt;  HWND hParent = GetParent(inst->hWnd);
      pt.x=Rect->left;  pt.y=Rect->top;    ScreenToClient(hParent, &pt);  Rect->left=pt.x;  Rect->top   =pt.y;
    }
    else if (inst->IsPopup())
    { RECT Rect2;  HWND hParent = GetWindow(inst->hWnd, GW_OWNER);
      BOOL modal_parent = IsView(hParent) && GetViewInst(hParent)->IsPopup();
      if (!modal_parent) hParent=GetFrame(hParent);
      GetWindowRect(hParent, &Rect2);
      Rect->left-=Rect2.left;  Rect->top -=Rect2.top;   
    }
  }
  else
  { Rect->left =inst->stat->vwX;   Rect->top   =inst->stat->vwY;
    Rect->right=inst->stat->vwCX;  Rect->bottom=inst->stat->vwCY;
  }
}
void SetViewRect(view_dyn * inst, RECT * Rect)
{ inst->stat->vwX =Rect->left;   inst->stat->vwY =Rect->top;
  inst->stat->vwCX=Rect->right;  inst->stat->vwCY=Rect->bottom;
  if (inst->hWnd)
  { if (inst->IsPopup())
    { RECT Rect2;  HWND hParent = GetWindow(inst->hWnd, GW_OWNER);
      BOOL modal_parent = IsView(hParent) && GetViewInst(hParent)->IsPopup();
      if (!modal_parent) hParent=GetFrame(hParent);
      GetWindowRect(hParent, &Rect2);
      Rect->left+=Rect2.left;  Rect->top +=Rect2.top;   
    }
    MoveWindow(inst->hWnd, Rect->left, Rect->top, Rect->right, Rect->bottom, TRUE);
  }
}

int WINAPI vwpropg_x(view_dyn * inst)
{ RECT Rect;  GetViewRect(inst, &Rect);
  return Rect.left;
}
void WINAPI vwprops_x(view_dyn * inst, int val)
{ RECT Rect;  GetViewRect(inst, &Rect);
  Rect.left=val;  SetViewRect(inst, &Rect);
}
int WINAPI vwpropg_y(view_dyn * inst)
{ RECT Rect;  GetViewRect(inst, &Rect);
  return Rect.top;
}
void WINAPI vwprops_y(view_dyn * inst, int val)
{ RECT Rect;  GetViewRect(inst, &Rect);
  Rect.top=val;  SetViewRect(inst, &Rect);
}
int WINAPI vwpropg_width(view_dyn * inst)
{ RECT Rect;  GetViewRect(inst, &Rect);
  return Rect.right;
}
void WINAPI vwprops_width(view_dyn * inst, int val)
{ RECT Rect;  GetViewRect(inst, &Rect);
  Rect.right=val;  SetViewRect(inst, &Rect);
}
int WINAPI vwpropg_height(view_dyn * inst)
{ RECT Rect;  GetViewRect(inst, &Rect);
  return Rect.bottom;
}
void WINAPI vwprops_height(view_dyn * inst, int val)
{ RECT Rect;  GetViewRect(inst, &Rect);
  Rect.bottom=val;  SetViewRect(inst, &Rect);
}
BOOL WINAPI vwpropg_visible(view_dyn * inst)
{ if (!inst->hWnd) return FALSE;
  return IsWindowVisible(inst->hWnd);
}
void WINAPI vwprops_visible(view_dyn * inst, wbbool val)
{ if (!inst->hWnd) return; // nelze nastavit
  ShowWindow(inst->hWnd, val ? SW_SHOWNA : SW_HIDE);
}
trecnum WINAPI vwpropg_curpos(view_dyn * inst)
{ return inst->sel_rec; }
void WINAPI vwprops_curpos(view_dyn * inst, trecnum pos)
{ if (!inst->hWnd) 
  { inst->sel_rec=pos;
    find_top_rec_by_sel_rec(inst);
  }
  else SendMessage(inst->hWnd, SZM_SETIPOS, -1, pos);
}
trecnum WINAPI vwpropg_curextrec(view_dyn * inst)
{ return inter2exter(inst, inst->sel_rec); }
void WINAPI vwprops_curextrec(view_dyn * inst, trecnum pos)
{ if (!inst->hWnd) 
  { pos=exter2inter(inst, pos);
    if (pos==NORECNUM) return;
    inst->sel_rec=pos;
    find_top_rec_by_sel_rec(inst);
  }
  else SendMessage(inst->hWnd, SZM_SETEPOS, -1, pos);
}
int WINAPI vwpropg_curitem(view_dyn * inst)
{ return inst->get_current_item(); }
void WINAPI vwprops_curitem(view_dyn * inst, int ID)
{ if (!inst->hWnd) 
  { int i;  t_ctrl * itm;
    for (i=0, itm=FIRST_CONTROL(inst->stat);  i<inst->stat->vwItemCount;
         i++, itm=NEXT_CONTROL(itm))
      if (itm->ctId==ID) inst->sel_itm=i;
  }
  else SendMessage(inst->hWnd, SZM_SETIPOS, ID, inst->sel_rec);
}
int WINAPI vwpropg_cursnum(view_dyn * inst)
{ if (inst==NULL) return (int)NOCURSOR;
  return (int)inst->dt->cursnum;
}
void WINAPI vwprops_cursnum(view_dyn * inst, tcursnum curs)
{ if (!inst->hWnd) Store_cursor(inst, curs);
  else Set_fcursor(inst->hWnd, curs, -1);
}
BOOL WINAPI vwpropg_editable(view_dyn * inst)
{ return !(inst->vwOptions & NO_VIEW_EDIT);
}
void WINAPI vwprops_editable(view_dyn * inst, wbbool editable)
{ if (editable) inst->vwOptions &= ~NO_VIEW_EDIT;
  else inst->vwOptions |= NO_VIEW_EDIT;
}
BOOL WINAPI vwpropg_movable(view_dyn * inst)
{ return !(inst->vwOptions & NO_VIEW_MOVE);
}
void WINAPI vwprops_movable(view_dyn * inst, wbbool movable)
{ if (movable) inst->vwOptions &= ~NO_VIEW_MOVE;
  else inst->vwOptions |= NO_VIEW_MOVE;
}
BOOL WINAPI vwpropg_insertable(view_dyn * inst)
{ return !(inst->stat->vwHdFt & NO_VIEW_INS);
}
void WINAPI vwprops_insertable(view_dyn * inst, wbbool insertable)
{ if (insertable) inst->stat->vwHdFt &= ~(LONG)(NO_VIEW_INS | NO_VIEW_FIC);
  else inst->stat->vwHdFt |= NO_VIEW_INS | NO_VIEW_FIC;
}
int WINAPI vwpropg_deletable(view_dyn * inst)
{ return inst->vwOptions & NO_VIEW_DELETE ? 0 :
         inst->vwOptions & QUERY_DEL ? 2 : 1;
}
void WINAPI vwprops_deletable(view_dyn * inst, int deletable)
{ if (deletable) inst->vwOptions &= ~NO_VIEW_DELETE;
  else inst->vwOptions |= NO_VIEW_DELETE;
  if (deletable==2) inst->vwOptions |= QUERY_DEL;
  else inst->vwOptions &= ~QUERY_DEL;
}

int WINAPI vwpropg_backcolor(view_dyn * inst)
{ return inst->stat->vwBackCol; }
void WINAPI vwprops_backcolor(view_dyn * inst, int color)
{ inst->stat->vwBackCol = RGBCOLOR(color);
  if (inst->form_brushes.count()) // brushes have been created (window has been opened, but may be closed now)
  { DeleteObject(inst->form_brushes.acc0(0)->hBrush);
    inst->form_brushes.acc0(0)->hBrush = CreateSolidBrush(inst->stat->vwBackCol);
  }
  if (inst->hWnd) 
    InvalidateRect(inst->hWnd, NULL, TRUE);
}
///////////////////////////// view methods /////////////////////////////
void WINAPI vwmtd_pick(view_dyn * inst)
{ if (inst->hWnd) Pick_window(inst->hWnd); }

BOOL WINAPI vwmtd_commit(view_dyn * inst, wbbool can_ask, wbbool report_error)
{ if (inst->hWnd) 
    return Commit_view(inst->hWnd, can_ask, report_error);
  return FALSE;
}
void WINAPI vwmtd_rollback(view_dyn * inst)
{ if (inst->hWnd) Roll_back_view(inst->hWnd); }
void WINAPI vwmtd_firstrec(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_FIRSTREC, 0, 0); }
void WINAPI vwmtd_lastrec(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_LASTREC,  0, 0); }
void WINAPI vwmtd_nextrec(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_NEXTREC,  0, 0); }
void WINAPI vwmtd_prevrec(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_PREVREC,  0, 0); }
void WINAPI vwmtd_nextpage(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_NEXTPAGE, 0, 0); }
void WINAPI vwmtd_prevpage(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_PREVPAGE, 0, 0); }
void WINAPI vwmtd_firstitem(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_FIRSTITEM,0, 0); }
void WINAPI vwmtd_lastitem(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_LASTITEM, 0, 0); }
void WINAPI vwmtd_nexttab(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_NEXTTAB,  0, 0); }
void WINAPI vwmtd_prevtab(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_PREVTAB,  0, 0); }
void WINAPI vwmtd_upitem(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_UPITEM,   0, 0); }
void WINAPI vwmtd_downitem(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_DOWNITEM, 0, 0); }
void WINAPI vwmtd_open_qbe(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_QBE,      0, 0); }
void WINAPI vwmtd_open_sort(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_ORDER,    0, 0); }
void WINAPI vwmtd_accept_query(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_ACCEPT_Q, 0, 0); }
void WINAPI vwmtd_cancel_query(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_UNLIMIT,  0, 0); }
int  WINAPI vwmtd_qbe_state(view_dyn * inst)
{ if (inst->hWnd) return inst->query_mode;  else return NONEINTEGER; }
void WINAPI vwmtd_insert(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_INSERT,   0, 0); }
void WINAPI vwmtd_del_rec(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_DELREC,   0, 0); }
void WINAPI vwmtd_del_ask(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_DELASK,   0, 0); }
void WINAPI vwmtd_print(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_PRINT,    0, 0); 
  else 
  { inst->view_mode=VWMODE_PRINT;
    inst->open(NULL);
    do_report(inst->cdp, 0, NORECNUM, inst->hWnd, NULL);
    inst->close();
  }
}
void WINAPI vwmtd_help(view_dyn * inst)
{ if (inst->hWnd) SendMessage(inst->hWnd, SZM_HELP,     0, 0); }

void * WINAPI vwmtd_parinst(view_dyn * inst)
{ if (!inst->hWnd) return NULL;
  HWND hParent = GetParent(inst->hWnd);
  if (!hParent || !IsView(hParent)) return NULL; // parent of a subview open in the view designer is not a view!
  return GetViewInst(hParent);
}
void WINAPI vwmtd_reset(view_dyn * inst, trecnum recnum, uns16 extent)
{ if (!inst->hWnd) return;
  if (recnum==NORECNUM)
       update_view(inst, 0,      NORECNUM, extent);
  else update_view(inst, recnum, recnum,   extent);
}

BOOL WINAPI vwmtd_open(view_dyn * inst, HWND * viewid)
{ return inst->open(viewid); }

HWND WINAPI vwmtd_handle(view_dyn * inst)
{ return inst->hWnd; }

void WINAPI vwmtd_close(view_dyn * inst)
{ if (inst) inst->close(); }  // may be called even when inst==NULL

BOOL WINAPI vwmtd_is_open(view_dyn * inst)  // may be called even when inst==NULL
{ return inst!=NULL && inst->hWnd; }

BOOL WINAPI vwmtd_set_source(view_dyn * inst, tptr src) // may be called even when inst==NULL
{ if (!inst) return FALSE;
  return inst->construct(src, NOOBJECT, 0, 0);
}

BOOL WINAPI vwmtd_attach_toolbar(view_dyn * inst, const char * name) 
{ if (!name || !*name) 
  { if (inst->designed_toolbar) 
    { inst->designed_toolbar->Release();  inst->designed_toolbar=NULL; 
      Set_tool_bar(inst->cdp, inst->VdGetToolbar());
    }
    return TRUE;
  }
  else 
  { t_designed_toolbar * dtb = NULL;
    view_dyn * patt_inst = find_inst(inst->cdp, name);
    if (patt_inst)
    { dtb=patt_inst->designed_toolbar;
      if (dtb) dtb->AddRef(); 
    }
    else // pattern form not open yet
    { tobjnum objnum;  
      if (!cd_Find_object(inst->cdp, name, CATEG_VIEW, &objnum)) 
      { tptr source = cd_Load_objdef(inst->cdp, objnum, CATEG_VIEW);
        if (source!=NULL) 
        { patt_inst = (view_dyn *)new basic_vd(inst->cdp);
          if (patt_inst!=NULL) 
          { strmaxcpy(inst->vwobjname, name, OBJ_NAME_LEN+1);
            if (patt_inst->create_vwstat(source, NO_REDIR, 0, 0))
            {  dtb=patt_inst->designed_toolbar;
              if (dtb) dtb->AddRef();   // must be before delete patt_inst!
            }
            delete patt_inst;
          }
          corefree(source);
        }
      }
    }
    if (dtb)
    { if (inst->designed_toolbar) inst->designed_toolbar->Release();
      inst->designed_toolbar=dtb;  
      Set_tool_bar(inst->cdp, inst->VdGetToolbar());
      return TRUE;
    }
    else return FALSE; // pattern form does not have a designed toolbar or pattern not opened and cannot be open
  }
}

BOOL WINAPI vwmtd_SelectIntRec(view_dyn *inst, trecnum Pos, BOOL Sel, BOOL Redraw)
{ return(inst->SetSel(Pos, Sel, Redraw)); }

BOOL WINAPI vwmtd_SelectExtRec(view_dyn *inst, trecnum Pos, BOOL Sel, BOOL Redraw)
{ return(inst->SetSel(exter2inter(inst, Pos), Sel, Redraw)); }

BOOL WINAPI vwmtd_IsIntRecSelected(view_dyn *inst, trecnum Pos)
{ return(inst->GetSel(Pos)); }

BOOL WINAPI vwmtd_IsExtRecSelected(view_dyn *inst, trecnum Pos)
{ return(inst->GetSel(exter2inter(inst, Pos))); }

trecnum WINAPI vwmtd_RecCount(view_dyn *inst, BOOL FictRec)
{ return inst->RecCount(FictRec);
}

static int make_font_descr(const char * facename, int height, int charset, BOOL bold, BOOL italic, BOOL underline, BOOL strikeout, t_fontdef * fd)
{ HDC hDC=GetDC(NULL);
  int logpix=GetDeviceCaps(hDC, LOGPIXELSY);
  ReleaseDC(NULL, hDC);
  strmaxcpy(fd->name, facename, sizeof(fd->name));
  fd->size   =(uns16)height;
  fd->charset=(uns8)(charset<0 ? DEFAULT_CHARSET : charset);
  fd->flags  =0; 
  if (bold)      fd->flags|=SFNT_BOLD;   if (italic)    fd->flags|=SFNT_ITAL;  
  if (underline) fd->flags|=SFNT_UNDER;  if (strikeout) fd->flags|=SFNT_STRIKE;
  fd->pitchfam=DEFAULT_PITCH | FF_DONTCARE;
  return logpix;
}

BOOL WINAPI vwmtd_setfont(view_dyn * inst, const char * facename, int height, int charset, BOOL bold, BOOL italic, BOOL underline, BOOL strikeout)
{ if (!(inst->vwOptions & SIMPLE_VIEW))
    { client_error_param(inst->cdp, CANNOT_IN_GENERAL_FORM, "Set_font");  return FALSE; }
  if (!inst->form_fonts.count()) // fonts not created yet
    { client_error_param(inst->cdp, CANNOT_IN_CLOSED_FORM, "Set_font");  return FALSE; }
  t_fontdef fd;
  int logpix=make_font_descr(facename, height, charset, bold, italic, underline, strikeout, &fd);
  HFONT hFont=cre_font(&fd, logpix);
  if (!hFont) return FALSE;
  DeleteObject(inst->form_fonts.acc0(0)->hFont);
  inst->form_fonts.acc0(0)->hFont=hFont;
  if (inst->hWnd) InvalidateRect(inst->hWnd, NULL, TRUE);
  return TRUE;
}

///////////////////////////// controls ////////////////////////////////////////
HWND get_view_control(view_dyn * inst, int ID, t_ctrl ** pitm = NULL)
{ if (!inst->hWnd) return 0;
  t_ctrl * itm = get_control_def(inst, ID);
  if (pitm!=NULL) *pitm=itm;
  if (itm->pane==PANE_DATA) ID=MAKE_FID(itm->ctId, inst->sel_rec-inst->toprec);
  else if (itm->pane==PANE_PAGESTART || itm->pane==PANE_PAGEEND) ID=MAKE_FID(itm->ctId, 0);
  else return 0;
  return GetDlgItem(inst->hWnd, ID);
}
 
char * WINAPI vwcpropg_text(view_dyn * inst, int ID)
{ t_ctrl * itm;  get_view_control(inst, ID, &itm);
  tptr p = inst->cdp->RV.hyperbuf;
  tptr src = inst->VdGetTextVal(ID, inst->sel_rec);
  if (HIWORD(src)) 
  { strmaxcpy(p, src, sizeof(inst->cdp->RV.hyperbuf));
    if (itm->ctClassNum==NO_COMBO)
    { tptr enumdef=get_enum_def(inst, itm);
      int ord = *(uns16*)p;
      if (ord)
      { while (--ord && *enumdef)
          enumdef+=strlen(enumdef)+1+sizeof(uns32);
        strcpy(p, enumdef);
      }
    }
  }
  else *p=0;
  return p;
}

void WINAPI vwcprops_text(view_dyn * inst, int ID, tptr text)
{ t_ctrl * itm;  get_view_control(inst, ID, &itm);
  char selection[3] = "\0\0";
  if (itm->ctClassNum==NO_COMBO)
  { if (*text)
    { tptr enumdef=get_enum_def(inst, itm);
      if (enumdef==NULL) return;
      do
      { if (!*enumdef) return;  // not found
        (*(uns16*)selection)++;
        if (!strcmp(enumdef, text)) break;
        enumdef+=strlen(enumdef)+1+sizeof(uns32);
      } while (TRUE);
    }
    text=selection;
  }
  inst->VdPutTextVal(ID, text); 
}

void WINAPI vwcpropg_nativevalue(view_dyn * inst, int ID)
{ t_ctrl * itm;  uns32 pom32, next32;  uns16 pom16;  double pomdbl;
  HWND hCtrl = get_view_control(inst, ID, &itm);
  //if (!hCtrl) return NULLSTRING;  // the result is undefined
  tptr p = inst->cdp->RV.hyperbuf;  int length = sizeof(inst->cdp->RV.hyperbuf)-1;
  inst->VdGetNativeVal(ID, inst->sel_rec, 0, &length, p);
  switch (itm ? itm->type : 0)
  { case ATT_STRING:  
    case ATT_BINARY:
      p[length]=0;
      _asm mov eax,p
      break;
    case ATT_INT32:  case ATT_DATE:  case ATT_TIME:  case ATT_TIMESTAMP:
    case ATT_PTR:    case ATT_BIPTR:
      pom32=*(uns32*)p;
      _asm mov eax,pom32
      break;
    case ATT_INT16:
      pom32=(sig32)*(sig16*)p;
      _asm mov eax,pom32
      break;
    case ATT_INT8:
      pom32=(sig32)*(sig8*)p;
      _asm mov eax,pom32
      break;
    case ATT_INT64:
      pom32 =((uns32*)p)[0];
      next32=((uns32*)p)[1];
      _asm mov eax,pom32
      _asm mov edx,next32
      break;
    case ATT_CHAR:  case ATT_BOOLEAN:
      pom32=(uns8)*p;
      _asm mov eax,pom32
      break;
    case ATT_MONEY:
      pom32=((monstr*)p)->money_hi4;
      pom16=((monstr*)p)->money_lo2;
      _asm mov eax,pom32
      _asm mov dx,pom16
      break;
    case ATT_FLOAT:
//      pom32 =((uns32*)p)[0];
//      next32=((uns32*)p)[1];
//      _asm mov edx,pom32
//      _asm mov eax,next32
      pomdbl=*(double*)p;
      _asm fld [pomdbl]
      break;
    default:
      _asm mov eax,0
      break;
  }
}
void WINAPI vwcprops_nativevalue(view_dyn * inst, int ID, char * val)
{ t_ctrl * itm;
  HWND hCtrl = get_view_control(inst, ID, &itm);
  if (itm!=NULL) 
  { if (is_string(itm->type) || itm->type==ATT_BINARY || IS_HEAP_TYPE(itm->type))
      inst->VdPutNativeVal(ID, inst->sel_rec, 0, 0, val);
    else inst->VdPutNativeVal(ID, inst->sel_rec, 0, 0, (tptr)&val);
  }
}

void * WINAPI vwcpropg_value_int(view_dyn * inst, int ID, unsigned offset, int length)
{ tptr p = inst->cdp->RV.hyperbuf;  t_ctrl * itm = NULL;
  inst->VdGetNativeVal(ID, inst->sel_rec, offset, &length, p);
  get_view_control(inst, ID, &itm);
  if (itm && (itm->type==ATT_TEXT || is_string(itm->type)))
    p[length]=0; // to muze vadit, kdyz ma buffer presnou velikost
  return p;
}

void WINAPI vwcprops_value_int(view_dyn * inst, int ID, unsigned offset, int length, char * val)
{ inst->VdPutNativeVal(ID, inst->sel_rec, offset, length, val); }

int WINAPI vwcpropg_value_len(view_dyn * inst, int ID)
{ int length;
  inst->VdGetNativeVal(ID, inst->sel_rec, 0, &length, NULL);
  return length;
}
void WINAPI vwcprops_value_len(view_dyn * inst, int ID, int length)
{ inst->VdPutNativeVal(ID, inst->sel_rec, 0, length, NULL); }

void * WINAPI vwcmtd_subinst(view_dyn * inst, int ID)
{ HWND hCtrl = get_view_control(inst, ID);
  if (!hCtrl) return NULL;
  return GetViewInst(hCtrl);
}

void WINAPI vwcmtd_generic(view_dyn * inst, int ID, DWORD methodID, int ResType, int Param1Type)
{ if (!inst->hWnd) return;
  untstr resval;  tptr pomstr;  sig32 pom32;
  resval.uany.type = ResType;  resval.uany.specif=0;
 // prepare parameters:
  int res=CallActiveXMethod(inst->hWnd, MAKE_FID(ID, inst->cdp->RV.view_position - inst->toprec), methodID, &resval, &Param1Type);
  switch (ResType)
  { case ATT_STRING:  
      if (!res) 
      { strcpy(inst->cdp->RV.hyperbuf, resval.uptr.val);
        delete [] resval.uptr.val;
      }
      pomstr=inst->cdp->RV.hyperbuf;
      _asm mov eax,pomstr
      break;
    case ATT_INT32:  case ATT_DATE:  case ATT_TIME:  case ATT_TIMESTAMP:
      pom32=resval.usig32.val;
      _asm mov eax,pom32
      break;
    case ATT_INT16:
      pom32=resval.usig32.val & 0xffff;
      _asm mov eax,pom32
      break;
    case ATT_INT8:
      pom32=resval.usig32.val & 0xff;
      _asm mov eax,pom32
      break;
    case ATT_CHAR:  case ATT_BOOLEAN:
      pom32=resval.usig32.val & 0xff;
      _asm mov eax,pom32
      break;
    default:
      _asm mov eax,0
      break;
  }
}

void get_real_format(signed char precision, int *decim, int *form)
{ if (decim!=NULL)
  { int val=precision<0 ? -precision : precision;
    if (val>=100) val-=100;
    *decim=val;
  }
  if (form!=NULL)
  { if (precision<=-100 || precision>=100) *form=2;
    else *form = precision<0 ? 0 : 1;
  }
}

void set_real_format(signed char * precision, int decim, int form)
{ if (decim<0) get_real_format(*precision, &decim, NULL);
  if (form <0) get_real_format(*precision, NULL, &form);
  if (form==2) decim+=100;
  *precision = form==0 ? -decim : decim;
}

int WINAPI vwcpropg_precision(view_dyn * inst, int ID)
{ t_ctrl * itm;  get_view_control(inst, ID, &itm);
  if (itm==NULL) return 0;
  int decim;
  get_real_format(itm->precision, &decim, NULL);
  return decim;
}

void WINAPI vwcprops_precision(view_dyn * inst, int ID, int val)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  if (itm!=NULL) set_real_format(&itm->precision, val, -1);
  if (inst->vwOptions & SIMPLE_VIEW) update_view(inst, inst->sel_rec, inst->sel_rec, 1);
  else 
  { reset_control_contents(inst, itm, hCtrl, inter2exter(inst, inst->sel_rec)==NORECNUM ? NORECNUM : inst->sel_rec);
    update_view(inst, inst->sel_rec, inst->sel_rec, RESET_INFLU);
  }
}

int WINAPI vwcpropg_format(view_dyn * inst, int ID)
{ t_ctrl * itm;  get_view_control(inst, ID, &itm);
  if (itm==NULL) return 0;
  if (itm->type==ATT_FLOAT)
  { int format;
    get_real_format(itm->precision, NULL, &format);
    return format;
  }
  if (itm->type==ATT_NOSPEC)
    return itm->precision & PREZ_MASK;
  return itm->precision;
}

void WINAPI vwcprops_format(view_dyn * inst, int ID, int val)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  if (itm==NULL) return;
  if (itm->type==ATT_FLOAT)
    set_real_format(&itm->precision, -1, val);
  else if (itm->type==ATT_NOSPEC)
    itm->precision = (itm->precision & ~PREZ_MASK) | (val & PREZ_MASK);
  else itm->precision=val;
  if (inst->vwOptions & SIMPLE_VIEW) update_view(inst, inst->sel_rec, inst->sel_rec, 1);
  else 
  { reset_control_contents(inst, itm, hCtrl, inter2exter(inst, inst->sel_rec)==NORECNUM ? NORECNUM : inst->sel_rec);
    update_view(inst, inst->sel_rec, inst->sel_rec, RESET_INFLU);
  }
}

BOOL WINAPI vwcpropg_checked(view_dyn * inst, int ID)
{ t_ctrl * itm;  get_view_control(inst, ID, &itm);
  tptr p=inst->VdGetTextVal(ID, inst->sel_rec);
  if (!HIWORD(p)) return NONEBOOLEAN;
  if (itm->ctClassNum==NO_RADIO)
  { char txt2[200];
    strmaxcpy(txt2, p, 200);  /* would be overwritten by reftxt */
    tptr reftxt=inst->VdGetRefVal(ID, inst->sel_rec);
    if (!HIWORD(reftxt)) return FALSE; /* ref. val not specified */
    return !strcmp(txt2, reftxt);
  }
  else
  { if (!*p) return NONEBOOLEAN;
    return *p=='+';
  }
}

void WINAPI vwcprops_checked(view_dyn * inst, int ID, BOOL val)
{ t_ctrl * itm;  get_view_control(inst, ID, &itm);
  if (itm->ctClassNum==NO_RADIO)
  { if (val) /* was not checked or is grayed */
    { tptr reftxt=inst->VdGetRefVal(ID, inst->sel_rec);
      if (HIWORD(reftxt))
        inst->VdPutTextVal(ID, reftxt);
    } /* checked ON */
    else if (inst->query_mode == QUERY_QBE)  /* can write out */
      inst->VdPutTextVal(ID, NULLSTRING);
  }
  else
    inst->VdPutTextVal(ID, val==TRUE ? "+" : val==FALSE ? "-" : NULLSTRING); 
}

int WINAPI vwcpropg_page(view_dyn * inst, int ID)
{ HWND hCtrl = get_view_control(inst, ID);
  if (!hCtrl) return -1;
  return TabCtrl_GetCurSel(hCtrl);
}

void WINAPI vwcprops_page(view_dyn * inst, int ID, int val)
{ HWND hCtrl = get_view_control(inst, ID);
  TabCtrl_SetCurSel(hCtrl, val);
  update_view(inst, inst->sel_rec, inst->sel_rec, RESET_INFLU);
}

BOOL WINAPI vwcmtd_isnull(view_dyn * inst, int ID)
{ tptr p=inst->VdGetTextVal(ID, inst->sel_rec);
  if (!HIWORD(p)) return NONEBOOLEAN;
  return !*p;
}

BOOL WINAPI vwcmtd_open(view_dyn * inst, int ID)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  switch (itm->ctClassNum)
  { case NO_RASTER:
      return open_picture_window(inst, ID, NOOBJECT) != 0;
    case NO_TEXT:
    { if (inst->VdGetAccess(itm->ctId, inst->sel_rec) != 2) return FALSE;
      unsigned ftype = itm->type;
      if (itm->access==ACCESS_CACHE) ftype|=CACHED_ATR;
      return Edit_text(inst->hWnd, inst->cdp->RV.stt->wbacc.cursnum, inst->cdp->RV.stt->wbacc.position,
              inst->cdp->RV.stt->wbacc.attr, inst->cdp->RV.stt->wbacc.index,
              ftype, 40, 40, 500, 200) != 0;
    }
    case NO_OLE:
      if (hCtrl) SendMessage(hCtrl, WM_LBUTTONDBLCLK, 0, 0);
      break;
    default: return FALSE;
  }
  return TRUE;
}

#define IO_BUF_SIZE 0x4000

BOOL WINAPI vwcmtd_readfile(view_dyn * inst, int ID, const char * fname)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  if (!IS_HEAP_TYPE(itm->type)) return FALSE;
  HANDLE hFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hFile==INVALID_HANDLE_VALUE) return FALSE;
  tptr buf=(tptr)corealloc(IO_BUF_SIZE, 45);
  if (buf==NULL) goto err0;
  unsigned offset;  offset = 0;
  do
  { DWORD rd;
    if (!ReadFile(hFile, buf, IO_BUF_SIZE, &rd, NULL)) goto err1;
    if (rd)
    { if (!inst->VdPutNativeVal(ID, inst->sel_rec, offset, rd, buf)) goto err1;
      offset+=rd;
    }
    else break;
  } while (TRUE);
 // write the length:
  inst->VdPutNativeVal(ID, inst->sel_rec, offset, 0, NULL);
  corefree(buf);
  CloseHandle(hFile);  
  if (hCtrl)
    reset_control_contents(inst, itm, hCtrl, inter2exter(inst, inst->sel_rec)==NORECNUM ? NORECNUM : inst->sel_rec);
  update_view(inst, inst->sel_rec, inst->sel_rec, RESET_INFLU);
  return TRUE;
 err1:
  corefree(buf);
 err0:
  CloseHandle(hFile);  
  return FALSE;
}

BOOL WINAPI vwcmtd_writefile(view_dyn * inst, int ID, const char * fname, wbbool append)
{ t_ctrl * itm;  get_view_control(inst, ID, &itm);
  if (!IS_HEAP_TYPE(itm->type)) return FALSE;
  HANDLE hFile = CreateFile(fname, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hFile==INVALID_HANDLE_VALUE) return FALSE;
  if (append) SetFilePointer(hFile, 0, NULL, FILE_END);
  tptr buf=(tptr)corealloc(IO_BUF_SIZE, 45);
  if (buf==NULL) goto err0;
  unsigned offset;  offset = 0;
  do
  { DWORD wr;  int length=IO_BUF_SIZE;
    if (!inst->VdGetNativeVal(ID, inst->sel_rec, offset, &length, buf)) goto err1;
    if (length)
    { if (!WriteFile(hFile, buf, length, &wr, NULL)) goto err1;
      offset+=length;
    }
    else break;
  } while (TRUE);
 // write the length:
  SetEndOfFile(hFile);
  corefree(buf);
  CloseHandle(hFile);  
  return TRUE;
 err1:
  corefree(buf);
 err0:
  CloseHandle(hFile);  
  return FALSE;
}

BOOL WINAPI vwcmtd_resetcombo(view_dyn * inst, unsigned ID, tptr query)
{ t_ctrl * itm;  get_view_control(inst, ID, &itm);
  enum_cont * enm = inst->enums;
  while (enm && enm->ctId!=ID) enm=enm->next; 
  if (enm==NULL) return FALSE; // enum not defined for this control
  enm->enum_reset(inst, FALSE, query);
 // for general form refill the combo lists:
  if (!(inst->vwOptions & SIMPLE_VIEW) && enm->defin)
  { for (trecnum r=0;  r<inst->recs_in_win;  r++)
    { HWND hCtrl=GetDlgItem(inst->hWnd, MAKE_FID(ID,r));
      if (hCtrl)
      { SendMessage(hCtrl, CB_RESETCONTENT, 0, 0);
        tptr en2=enm->defin;
        while (*en2)
        { SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)en2);
          en2+=strlen(en2)+1+sizeof(uns32);
        }
       // restore the original selection:
        if (inter2exter(inst, inst->toprec+r)!=NORECNUM)
          reset_control_contents(inst, itm, hCtrl, inst->toprec+r);
      }
    }
  }
  return TRUE;
}

BOOL WINAPI vwcmtd_setnull(view_dyn * inst, int ID)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  if (itm->ctClassNum==NO_SIGNATURE)
  { if (write_val(inst->hWnd)) return FALSE;
    if (!cache_synchro(inst, TRUE, TRUE)) return FALSE;
    if (inst->VdGetAccess(ID, inst->sel_rec) != 2) return FALSE;
    run_state * RV = &inst->cdp->RV;
    if (RV->stt->wbacc.position==NORECNUM) return TRUE;
    if (cd_Signature(inst->cdp, inst->hWnd, RV->stt->wbacc.cursnum, RV->stt->wbacc.position, RV->stt->wbacc.attr, 2, NULL))
      { Signalize();  return FALSE; }
    update_view(inst, inst->sel_rec, inst->sel_rec, RESET_CACHE|RESET_INFLU);
    return TRUE;
  }
  else return inst->VdPutTextVal(ID, NULLSTRING)==2; 
}

int WINAPI vwcpropg_selindex(view_dyn * inst, int ID)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  if (!hCtrl) return -1;
  int sel=SendMessage(hCtrl, itm->ctClassNum==NO_LIST ? LB_GETCURSEL : CB_GETCURSEL, 0, 0);
  return sel==LB_ERR ? -1 : sel;
}

void WINAPI vwcprops_selindex(view_dyn * inst, int ID, int val)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  if (!hCtrl) return;
  if (itm->ctClassNum==NO_LIST)
    SendMessage(hCtrl, LB_SETCURSEL, val<0 ? -1 : val, 0);
  else // combo
  { char selection[3] = "\0\0";
    if (val<0) val=-1;
    *(uns16*)selection=(uns16)val;
    inst->VdPutTextVal(ID, selection); 
  }
}

int WINAPI vwcpropg_backcolor(view_dyn * inst, int ID)
{ t_ctrl * itm;  get_view_control(inst, ID, &itm);
  return itm->ctBackCol;
}

void WINAPI vwcprops_backcolor(view_dyn * inst, int ID, int val)
{ if (inst->vwOptions & SIMPLE_VIEW)
    { client_error_param(inst->cdp, CANNOT_IN_STD_FORM, "BackColor");  return; }
  t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  itm->ctBackCol=RGBCOLOR(val);
  switch (itm->ctClassNum)
  { case NO_RTF:
      if (hCtrl) SendMessage(hCtrl, EM_SETBKGNDCOLOR, 0, RGBCOLOR(itm->ctBackCol));  break;
    case NO_CALEND:
      if (hCtrl) 
      { MonthCal_SetColor(hCtrl, MCSC_BACKGROUND, RGBCOLOR(itm->ctBackCol)); 
        MonthCal_SetColor(hCtrl, MCSC_MONTHBK,    RGBCOLOR(itm->ctBackCol));
      }
      break;
    case NO_FRAME: 
      if (inst->hWnd) InvalidateRect(inst->hWnd, NULL, TRUE);  break;
    default:
      if (hCtrl) InvalidateRect(hCtrl, NULL, TRUE);  break;
  }
}

int WINAPI vwcpropg_textcolor(view_dyn * inst, int ID)
{ t_ctrl * itm;  get_view_control(inst, ID, &itm);
  return itm->forecol;
}

void WINAPI vwcprops_textcolor(view_dyn * inst, int ID, int val)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  itm->forecol=RGBCOLOR(val);
  if (inst->vwOptions & SIMPLE_VIEW)
  { if (inst->hWnd) InvalidateRect(inst->hWnd, NULL, TRUE);
  }
  else switch (itm->ctClassNum)
  { case NO_CALEND:
      if (hCtrl) MonthCal_SetColor(hCtrl, MCSC_TEXT, RGBCOLOR(itm->forecol));  break;
    case NO_FRAME: 
      if (inst->hWnd) InvalidateRect(inst->hWnd, NULL, TRUE);  break;
    default:
      if (hCtrl) InvalidateRect(hCtrl, NULL, TRUE);  break;
  }
}

static WNDPROC CalendarOrigProc;

static LRESULT CALLBACK CalendarWndProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP)
{ switch (msg)
  { case WM_CHAR:
      if (wP==VK_ESCAPE)
        PostMessage(GetParent(hWnd), WM_CLOSE, 0, 0);  
      else if (wP==VK_RETURN)
      { SYSTEMTIME * systm = (SYSTEMTIME*)GetWindowLong(GetParent(hWnd), 0);
        MonthCal_GetCurSel(hWnd, systm);
        PostMessage(GetParent(hWnd), WM_CLOSE, 0, 0);  
      }
      // cont.
  }
  return CallWindowProc(CalendarOrigProc, hWnd, msg, wP, lP);
}

static LRESULT CALLBACK CalendarPopupWndProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP)
{ switch (msg)
  { case WM_ACTIVATE:
      if (GET_WM_ACTIVATE_STATE(wP, lP)==WA_INACTIVE) 
        PostMessage(hWnd, WM_CLOSE, 0, 0);
      return DefWindowProc(hWnd, msg, wP, lP);
    case WM_SIZE:
    { HWND hCal = GetDlgItem(hWnd, 100);  if (!hCal) break;
      RECT Rect;  GetClientRect(hWnd, &Rect);
      SetWindowPos(hCal, 0, 0, 0, Rect.right, Rect.bottom, SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
      break;
    }
    case WM_NOTIFY:
      if (((NMHDR*)lP)->code==MCN_SELECT)
      { SYSTEMTIME * systm = (SYSTEMTIME*)GetWindowLong(hWnd, 0);
        MonthCal_GetCurSel(GetDlgItem(hWnd, 100), systm);
        PostMessage(hWnd, WM_CLOSE, 0, 0);
      }
      break;
    default:
      return DefWindowProc(hWnd, msg, wP, lP);
  }
  return 0;
}

void PositionWindow(HWND hWnd, RECT * RefRect, int xsize, int ysize)
{ int cxscreen = GetSystemMetrics(SM_CXSCREEN), cyscreen = GetSystemMetrics(SM_CYSCREEN);
  int xpos, ypos;
  if (RefRect->bottom+ysize <= cyscreen) ypos=RefRect->bottom;
  else if (RefRect->top >= ysize) ypos=RefRect->top - ysize;
  else ypos=cyscreen-ysize;
  xpos=RefRect->left;
  if (xpos+xsize > cxscreen) xpos=cxscreen-xsize;
  SetWindowPos(hWnd, 0, xpos, ypos, xsize, ysize, SWP_NOZORDER | SWP_SHOWWINDOW);
}

static const char CALENDAR_POPUP_NAME[] = "CalenadarPopup";

void WINAPI vwcmtd_calendar(view_dyn * inst, int ID, BOOL show_today, BOOL circle_today, BOOL week_numbers)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);  uns32 dt;
  if (!hCtrl) return;
  if (itm->type!=ATT_DATE && itm->type!=ATT_TIMESTAMP) { MessageBeep(-1);  return; }
  if (!inst->VdGetNativeVal(ID, inst->sel_rec, 0, NULL, (tptr)&dt)) return;
  SYSTEMTIME systime, systm2;  memset(&systime, 0, sizeof(SYSTEMTIME));
  if (dt!=NONEDATE)
  { systime.wYear=Year(dt);  systime.wMonth=Month(dt);  systime.wDay=Day(dt); }
  else // NONE
  { systime.wYear=1900;  systime.wMonth=1;  systime.wDay=1; }
  memcpy(&systm2, &systime, sizeof(SYSTEMTIME));
 // register class:
  WNDCLASS WndClass;
  WndClass.lpszClassName=CALENDAR_POPUP_NAME;
  WndClass.hInstance=hKernelLibInst;
  WndClass.lpfnWndProc=CalendarPopupWndProc;
  WndClass.style=CS_GLOBALCLASS | CS_DBLCLKS;
  WndClass.lpszMenuName=NULL;
  WndClass.hIcon=0;
  WndClass.hCursor=LoadCursor(0, IDC_ARROW);
  WndClass.hbrBackground=(HBRUSH)GetStockObject(NULL_BRUSH);
  WndClass.cbClsExtra=0;
  WndClass.cbWndExtra=sizeof(void*);
  RegisterClass(&WndClass);
 // create popup and calendar:
  HWND hPopup = CreateWindow(CALENDAR_POPUP_NAME, NULLSTRING, WS_POPUP | WS_BORDER,
    0, 0, 100, 100, inst->hWnd, 0, hKernelLibInst, NULL);
  int style = WS_CHILD;
  if (!show_today)   style |= MCS_NOTODAY;
  if (!circle_today) style |= MCS_NOTODAYCIRCLE;
  if (week_numbers)  style |= MCS_WEEKNUMBERS;
  HWND hCal = CreateWindow(MONTHCAL_CLASS, NULLSTRING, style,
    0, 0, 100, 100, hPopup, (HMENU)100, hKernelLibInst, NULL);
  if (!hPopup || !hCal) return;
  MonthCal_SetCurSel(hCal, &systime);
  //SetProp(hCal, "DATAPTR", (HANDLE)&systm2);
  SetWindowLong(hPopup, 0, (LPARAM)&systm2);
  RECT MinRect;  MonthCal_GetMinReqRect(hCal, &MinRect);
 // find control's position and position the calendar:
  RECT Rect;  GetWindowRect(hCtrl, &Rect);
  PositionWindow(hPopup, &Rect, MinRect.right+2*GetSystemMetrics(SM_CXBORDER), MinRect.bottom+2*GetSystemMetrics(SM_CYBORDER));
  CalendarOrigProc=(WNDPROC)SetWindowLong(hCal, GWL_WNDPROC, (LPARAM)CalendarWndProc);
  SetFocus(hCal);
 // cycle:
  MSG msg;
  while (IsWindow(hPopup) && GetMessage(&msg, 0,0,0))
  { TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  UnregisterClass(CALENDAR_POPUP_NAME, hKernelLibInst);
 // write the value, if changed:
  if (IsWindow(inst->hWnd)) // fuse
    if (systime.wYear!=systm2.wYear || systime.wMonth!=systm2.wMonth || systime.wDay!=systm2.wDay)
    { if (systm2.wYear==1900 && systm2.wMonth==1 && systm2.wDay==1)
        dt=NONEDATE;
      else dt=Make_date(systm2.wDay, systm2.wMonth, systm2.wYear);
      inst->VdPutNativeVal(ID, inst->sel_rec, 0, 0, (tptr)&dt);
    }
}

HWND WINAPI vwcmtd_handle(view_dyn * inst, int ID)
{ return get_view_control(inst, ID, NULL); }

void WINAPI vwcmtd_setfocus(view_dyn * inst, int ID)
{ if (!inst->hWnd) return;
  SendMessage(inst->hWnd, SZM_SETIPOS, ID, inst->sel_rec);
}

void WINAPI vwcmtd_selection(view_dyn * inst, int ID, int start, int stop)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  if (!hCtrl) return;
  switch (itm->ctClassNum)
  { case NO_EDI:
      SendMessage(hCtrl, EM_SETSEL, start, stop);
      SendMessage(hCtrl, EM_SCROLLCARET, 0, 0);
      break;
    case NO_EC:
      SendMessage(hCtrl, CB_SETEDITSEL, 0, MAKELPARAM(start, stop));
      break;
    case NO_RTF:
    { CHARRANGE charrange;  charrange.cpMin=start;  charrange.cpMax=stop;
      SendMessage(hCtrl, EM_EXSETSEL, 0, (LPARAM)&charrange);
      break;
    }
  }
}

void WINAPI vwcmtd_refresh(view_dyn * inst, int ID)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  write_val(inst->hWnd);  // save changes in other controls
  if (inst->vwOptions & SIMPLE_VIEW)
    update_view(inst, inst->sel_rec, inst->sel_rec, RESET_ALL);
  else
    if (hCtrl)
      reset_control_contents(inst, itm, hCtrl, inst->sel_rec);
}

int  WINAPI vwcmtd_sigstate(view_dyn * inst, int ID)
{ if (!inst->hWnd) return -(int)sigstate_error;
  t_signature sig;  int length = sizeof(t_signature);
  if (inst->VdGetNativeVal(ID, inst->sel_rec, 0, &length, (tptr)&sig)!=TRUE) 
    return -(int)sigstate_error;
  return sig_state(inst->cdp, &sig, length);
}

void WINAPI vwcmtd_sigcheck(view_dyn * inst, int ID)
{ if (!inst->hWnd) return;
  if (write_val(inst->hWnd)) return;
  if (!cache_synchro(inst, TRUE, TRUE)) return;
  if (inst->VdGetAccess(ID, inst->sel_rec) != 2) return;
  run_state * RV = &inst->cdp->RV;
  if (RV->stt->wbacc.position==NORECNUM) 
    { client_error(inst->cdp, NOT_ON_FICT);  Signalize();  return; }
  if (cd_Signature(inst->cdp, inst->hWnd, RV->stt->wbacc.cursnum, RV->stt->wbacc.position, RV->stt->wbacc.attr, 0, NULL))
    Signalize();
  update_view(inst, inst->sel_rec, inst->sel_rec, RESET_CACHE|RESET_INFLU);
}

void WINAPI vwcmtd_sigsign(view_dyn * inst, int ID)
{ if (!inst->hWnd) return;
  if (write_val(inst->hWnd)) return;
  if (!cache_synchro(inst, TRUE, TRUE)) return;
  if (inst->VdGetAccess(ID, inst->sel_rec) != 2) return;
  run_state * RV = &inst->cdp->RV;
  if (RV->stt->wbacc.position==NORECNUM) 
    { client_error(inst->cdp, NOT_ON_FICT);  Signalize();  return; }
  if (inst->cdp->logged_as_user==ANONYMOUS_USER) 
    { client_error(inst->cdp, ANONYM_CANNOT_SIGN);  Signalize();  return; }
  if (cd_Signature(inst->cdp, inst->hWnd, RV->stt->wbacc.cursnum, RV->stt->wbacc.position, RV->stt->wbacc.attr, 14, NULL))
    Signalize();
  update_view(inst, inst->sel_rec, inst->sel_rec, RESET_CACHE|RESET_INFLU);
}

BOOL WINAPI vwcmtd_setfont(view_dyn * inst, int ID, const char * facename, int height, int charset, BOOL bold, BOOL italic, BOOL underline, BOOL strikeout)
{ t_ctrl * itm;  HWND hCtrl = get_view_control(inst, ID, &itm);
  if (inst->vwOptions & SIMPLE_VIEW)
    { client_error_param(inst->cdp, CANNOT_IN_STD_FORM, "Set_font");  return FALSE; }
  if (!hCtrl) // cannot store the assigned font
    { client_error_param(inst->cdp, CANNOT_IN_CLOSED_FORM, "Set_font");  return FALSE; }
  t_fontdef fd;
  int logpix=make_font_descr(facename, height, charset, bold, italic, underline, strikeout, &fd);
 // search for the font description:
  unsigned ind=0;  HFONT hFont;
  while (ind<inst->form_fonts.count())
  { if (inst->form_fonts.acc0(ind)->fontdef.same(&fd)) break;
    ind++;
  }
  if (ind<inst->form_fonts.count()) hFont=inst->form_fonts.acc0(ind)->hFont;  // existing font
  else
  { t_view_font * pvf = inst->form_fonts.next();
    if (!pvf) return FALSE;
    pvf->fontdef=fd;
    pvf->hFont=hFont=cre_font(&fd, logpix);
  }
  SendMessage(hCtrl, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE,0));
  return TRUE;
}

BOOL WINAPI vwmtd_isviewchanged(view_dyn * inst)
{ return IsViewChanged(inst->hWnd);
}

//////////////////////////// view method entry table ////////////////////////////
void (*vwmtd_table[])(void) = {
(void (*)(void))vwpropg_caption,
(void (*)(void))vwprops_caption,
(void (*)(void))vwpropg_x,
(void (*)(void))vwprops_x,
(void (*)(void))vwpropg_y,
(void (*)(void))vwprops_y,
(void (*)(void))vwpropg_width,
(void (*)(void))vwprops_width,
(void (*)(void))vwpropg_height,
(void (*)(void))vwprops_height,
(void (*)(void))vwpropg_visible,
(void (*)(void))vwprops_visible,
(void (*)(void))vwmtd_pick,
(void (*)(void))vwmtd_is_open,
(void (*)(void))vwmtd_commit,
(void (*)(void))vwmtd_rollback,
(void (*)(void))vwmtd_firstrec,
(void (*)(void))vwmtd_lastrec,
(void (*)(void))vwmtd_nextrec,
(void (*)(void))vwmtd_prevrec,
(void (*)(void))vwmtd_nextpage,
(void (*)(void))vwmtd_prevpage,
(void (*)(void))vwmtd_firstitem,
(void (*)(void))vwmtd_lastitem,
(void (*)(void))vwmtd_nexttab,
(void (*)(void))vwmtd_prevtab,
(void (*)(void))vwmtd_upitem,
(void (*)(void))vwmtd_downitem,
(void (*)(void))vwmtd_open_qbe,
(void (*)(void))vwmtd_open_sort,
(void (*)(void))vwmtd_accept_query,
(void (*)(void))vwmtd_cancel_query,
(void (*)(void))vwmtd_qbe_state,
(void (*)(void))vwmtd_insert,
(void (*)(void))vwmtd_del_rec,
(void (*)(void))vwmtd_del_ask,
(void (*)(void))vwmtd_print,
(void (*)(void))vwmtd_help,
(void (*)(void))vwpropg_curpos,
(void (*)(void))vwprops_curpos,
(void (*)(void))vwpropg_curextrec,
(void (*)(void))vwprops_curextrec,
(void (*)(void))vwpropg_curitem,
(void (*)(void))vwprops_curitem,
(void (*)(void))vwpropg_cursnum,
(void (*)(void))vwprops_cursnum,
(void (*)(void))vwpropg_editable,
(void (*)(void))vwprops_editable,
(void (*)(void))vwpropg_movable,
(void (*)(void))vwprops_movable,
(void (*)(void))vwpropg_deletable,
(void (*)(void))vwprops_deletable,
(void (*)(void))vwpropg_insertable,
(void (*)(void))vwprops_insertable,
(void (*)(void))vwcpropg_text,
(void (*)(void))vwcprops_text,
(void (*)(void))vwcmtd_subinst,
(void (*)(void))vwmtd_parinst,
(void (*)(void))vwmtd_reset,
(void (*)(void))vwcpropg_nativevalue,
(void (*)(void))vwcprops_nativevalue, // 60
(void (*)(void))vwmtd_open, 
(void (*)(void))vwmtd_close,
(void (*)(void))vwcmtd_generic,
(void (*)(void))vwcpropg_precision,
(void (*)(void))vwcprops_precision,
(void (*)(void))vwcpropg_checked,
(void (*)(void))vwcprops_checked,
(void (*)(void))vwcpropg_format,
(void (*)(void))vwcprops_format,
(void (*)(void))vwcpropg_page,    // 70
(void (*)(void))vwcprops_page,
(void (*)(void))vwcmtd_isnull,
(void (*)(void))vwcmtd_open,
(void (*)(void))vwcmtd_readfile,
(void (*)(void))vwcmtd_resetcombo,
(void (*)(void))vwcmtd_setnull,
(void (*)(void))vwcmtd_writefile,
(void (*)(void))vwcpropg_value_int,
(void (*)(void))vwcprops_value_int,
(void (*)(void))vwcpropg_value_len,  //80
(void (*)(void))vwcprops_value_len,
(void (*)(void))vwcpropg_selindex,
(void (*)(void))vwcprops_selindex,
(void (*)(void))vwmtd_set_source,
(void (*)(void))vwcmtd_calendar,
(void (*)(void))vwmtd_handle,
(void (*)(void))vwcmtd_handle,
(void (*)(void))vwcmtd_setfocus,
(void (*)(void))vwcmtd_selection,
(void (*)(void))vwcmtd_refresh,      //90
(void (*)(void))vwcmtd_sigstate,
(void (*)(void))vwcmtd_sigcheck,
(void (*)(void))vwcmtd_sigsign,
(void (*)(void))vwmtd_attach_toolbar,
(void (*)(void))vwcpropg_backcolor,
(void (*)(void))vwcprops_backcolor,
(void (*)(void))vwcpropg_textcolor,
(void (*)(void))vwcprops_textcolor,
(void (*)(void))vwpropg_backcolor,
(void (*)(void))vwprops_backcolor,  //100
(void (*)(void))vwcmtd_setfont,
(void (*)(void))vwmtd_setfont,
(void (*)(void))vwmtd_isviewchanged,
(void (*)(void))vwmtd_SelectIntRec,
(void (*)(void))vwmtd_SelectExtRec,
(void (*)(void))vwmtd_IsIntRecSelected,
(void (*)(void))vwmtd_IsExtRecSelected,
(void (*)(void))vwmtd_RecCount,
};

#endif
#endif


