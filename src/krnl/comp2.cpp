/****************************************************************************/
/* The S-Pascal compiler - Sezam object part - comp2.c                      */
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
#include "wprez.h"
#include "whelp.h"
#include "messages.h"
#include "globdef.h"
#include "mask.h"
#else
#include "wprez9.h"
#endif
#ifdef LINUX
#include <setjmp.h>
#include <wchar.h>
#endif

/* List of the univ_ptr usage:
menu_comp receives windows menu handle there (main menu handle is in temp_object),
menu_struct_comp receives ptr to the describing structure in it and fills it,
view_access & view_value return the type there, used by view designer & debugger evaluator
sql_from receives select_upart * there,
sql_analysis receives t_sql_anal * there,
t_table_aux when compiling a table def. or adding/removing an index
table_def_comp_link receives table_all * there
extract_sql returns a string there
divide_sql receives ptr_dynar * there
view compilation: integrity check info
*/

/* ctStyle usage:
CBS_DROPDOWN, CBS_DROPDOWNLIST
ES_MULTILINE | ES_WANTRETURN, ES_LEFT
SS_LEFT, SS_RIGHT, SS_CENTER
BS_...
LBS_NOTIFY

INDEXABLE, NO_REFORMAT, FLEXIBLE_SIZE, INHERIT_CURS, FRAMED, SHADED,
CAN_OVERFLOW, IT_INFLUEN, QUERY_STATIC, QUERY_DYNAMIC

WS_TABSTOP, WS_BORDER, WS_GROUP, WS_VSCROLL

default clip:
WS_TABSTOP: edit, combo, button, list, check, radio, text, raster, ole, subview, slider
WS_BORDER:  edit, combo, list, subview, rtf, updown,
edit:   ES_AUTOHSCROLL
button: BS_PUSHBUTTON
combo:  CBS_AUTOHSCROLL | CBS_DROPDOWN(LIST)     WS_VSCROLL
list:                                            WS_VSCROLL
check:  BS_3STATE|                               WS_GROUP
radio:  BS_RADIOBUTTON|                          WS_GROUP
text:   SS_TEXT
raster: SS_RASTER
frame:  SS_BLACKFRAME
ole:
subview:INHERIT_CURS
val, edi v simple: numericke SS_RIGHT, jinak SS_LEFT

Automaticke doplneni pri kompilaci:
list: WS_VSCROLL | WS_BORDER
edit on text: ES_MULTILINE | ES_WANTRETURN
updown: WS_BORDER
str, val, text, raster: WS_BORDER pro FRAMED, SHADED

Automaticke doplneni pri otevirani:
WS_CHILD, WS_VISIBLE, (WS_CLIPSIBLINGS v navrhari)
edit WS_BORDER, on text: AUTOHSCROLL -> MULTILINE, VSCROLL

Picture on button changes BS_PUSHBUTTON or BS_DEFPUSHBUTTON to BS_OWNERDRAW -- not here, in create control

*/

void eqtest(CIp_t CI)
{ test_and_next(CI,'=', EQUAL_EXPECTED); }

int findkeytab(CIp_t CI, tptr table, BOOL end_allowed)
{ int i;
  if (end_allowed)
    if (CI->cursym==S_END)
      { next_sym(CI);  return -1; }
  if (CI->cursym!=S_IDENT) c_error(DEF_KEY_EXPECTED);
  i=0;
  while (*table)
  { if (!my_stricmp(table,CI->curr_name))
      { next_sym(CI);  return i; }
    table += strlen(table)+1;
    i++;
  }
  c_error(BAD_IDENT);
  return 0;
}

static char wvw_tab_h[] = "FONT\0PANE\0BPICTURE\0COLOUR\0BACKGROUND\0_RCLICK\0TABLEVIEW\0"
                   "CURSORVIEW\0DBDIALOG\0STYLE\0CAPTION\0"
                   "NEWREC\0HELP\0GROUP\0LABEL\0TOOLBAR\0HDFTSTYLE\0_STMT_START\0_STMT_END\0"
                   "OPENACTION\0CLOSEACTION\0_PROJECT_NAME\0_PANE_HEIGHTS\0"
                   "_PRINTER_OPTS\0_STYLE3\0_TEMPLATE_NAME\0_NEWRECACTION\0_DBLCLICK\0";
static char wvw_tab_o[] = "FONT\0PANE\0BPICTURE\0COLOUR\0BACKGROUND\0_RCLICK\0PRECISION\0"
                   "VISIBILITY\0"
                   "ENABLED\0INTEGRITY\0DEFAULT\0ENUMERATE\0VALUE\0"
                   "ONERROR\0ACTION\0ACCESS\0HELP\0COMBOSELECT\0STATUSTEXT\0"
                   "QUERYCOND\0FORMATMASK\0CTRLSPECIF\0CTRLPOPUP\0NAME\0PROP\0EVENT\0STYLEX\0FRAMELINEWIDTH\0";
static char wvw_tab_c[] = "LTEXT\0RTEXT\0CTEXT\0CHECKBOX\0PUSHBUTTON\0LISTBOX\0"
                   "GROUPBOX\0DEFPUSHBUTTTON\0RADIOBUTTON\0EDITTEXT\0COMBOBOX\0"
                   "ICON\0CONTROL\0OLEITEM\0SUBVIEW\0GRAPH\0RTF\0SLIDER\0"
                   "UPDOWNCTRL\0PROGRESSBAR\0SIGNATURE\0TABCONTROL\0ACTIVEX\0CALENDAR\0_DTP_CTRL\0BARCODE\0";
static char wvw_tab_x[] = "VALUE\0ACCESS\0FONT\0COLOUR\0BPICTURE\0DBPICTURE\0DBAPICTURE\0";

BOOL is_any_key(symbol cursym, tptr curr_name)
{ tptr p;
  if ((cursym==S_END)||(cursym==S_BEGIN))
    return TRUE;
  if (cursym!=S_IDENT) return FALSE;
  p=wvw_tab_h;
  while (*p)
  { if (!strcmp(curr_name, p)) return TRUE;
   p+=strlen(p)+1;
  }
  p=wvw_tab_o;
  while (*p)
  { if (!strcmp(curr_name, p)) return TRUE;
   p+=strlen(p)+1;
  }
  p=wvw_tab_c;
  while (*p)
  { if (!strcmp(curr_name, p)) return TRUE;
   p+=strlen(p)+1;
  }
  return FALSE;
}

/****************************** windows view *********************************/
#ifndef SRVR

char panetab[]   = "REPORTHEADER\0PAGEHEADER\0AHEADER\0BHEADER\0CHEADER\0"
  "DHEADER\0EHEADER\0DATA\0EFOOTER\0DFOOTER\0CFOOTER\0BFOOTER\0AFOOTER\0"
  "PAGEFOOTER\0REPORTFOOTER\0";
char fonttab[]   = "ITALICS\0BOLD\0UNDERLINED\0STRIKEOUT\0";

typedef enum { Y_NOIDENT, Y_NOTFOUND, Y_BEGIN, Y_END,
               Y_FONT,Y_PANE,Y_PICTURE, Y_COLOUR, Y_BACKGROUND, Y_RCLICK,
               Y_TABDIALOG,Y_CURDIALOG,Y_DIALOG,Y_STYLE,Y_CAPTION,
               Y_NEWREC,Y_HELP,Y_GROUP,Y_LABEL,Y_TOOLBAR, Y_HDFTSTYLE,
               Y_STMTSTART, Y_STMTEND, Y_OPENACTION, Y_CLOSEACTION,
               Y_PROJECT_NAME, Y_PANE_HEIGHTS, Y_PRINTER_OPTS, Y_STYLE3,
               Y_TEMPLATE_NAME, Y_NEWRECACT, Y_DBLCLICK } tauxenum1;
typedef enum { /* Y_NOIDENT, Y_NOTFOUND, Y_BEGIN, Y_END,
               Y_FONT,Y_PANE,Y_PICTURE, Y_COLOUR, Y_BACKGROUND, Y_RCLICK,*/
               Y_PRECISION=10, Y_VISIBILITY,
               Y_ENABLED,Y_INTEGRITY,Y_DEFAULT,Y_ENUMERATE, Y_VALUE,
               Y_ONERROR, Y_ACTION, Y_ACCESS, Y_CTHELP, Y_COMBOSELECT,
               Y_STATUSTEXT, Y_QUERYCOND, Y_MASK, Y_SPECIF, Y_CTRLPOPUP,
               Y_NAME, Y_PROP, Y_EVENT, Y_STYLEX, Y_FRAMELNWDTH } tauxenum2;
typedef enum { /* Y_NOIDENT, Y_NOTFOUND, Y_BEGIN, Y_END, */
               Y_LTEXT=4,Y_RTEXT,Y_CTEXT,Y_CHECKBOX,Y_PUSHBUTTON,Y_LISTBOX,
               Y_GROUPBOX,Y_DEFPUSHBUTTON,Y_RADIOBUTTON,Y_EDITTEXT,Y_COMBOBOX,
               Y_ICON,Y_CONTROL,Y_OLEITEM,Y_SUBVIEW, Y_GRAPH,
               Y_RTF, Y_SLIDER, Y_UPDOWN, Y_PROGRESS, Y_SIGNATURE,
               Y_TABCONTROL, Y_ACTIVEX, Y_CALENDAR, Y_DTP, Y_BARCODE } tauxenum3;
typedef enum { Y_ITALICS=4, Y_BOLD, Y_UNDERLINED, Y_STRIKEOUT } tauxenum4;

unsigned pre_key_pos;  unsigned char pre_key_char;

int find_tab_id(CIp_t CI, char * p)
{ int res;
  pre_key_pos=(unsigned)(CI->compil_ptr-CI->univ_source);
  pre_key_char=CI->curchar;
  if ((CI->cursym!=S_IDENT) && (p!=fonttab))
  { if (CI->cursym==S_BEGIN)    { next_sym(CI);  return Y_BEGIN;  }
    if (CI->cursym==S_END) return Y_END;
    return Y_NOIDENT;
  }
  else
  { res=Y_FONT;
    while (*p)
      { if (!strcmp(CI->curr_name, p)) { next_sym(CI);  return res; }
    res++;
    p+=strlen(p)+1;
   }
  }
  return Y_NOTFOUND;
}

void non_comp(CIp_t CI)  /* copies the source into code insead of compiling */
{ unsigned pre_sym;
  pre_sym=0;
  if (CI->cursym==S_IDENT && CI->curr_name[0]=='_' && !CI->curr_name[1])
  { pre_key_pos=(unsigned)(CI->compil_ptr-CI->univ_source);
    do
    { pre_sym=(unsigned)(CI->compil_ptr-CI->univ_source);
      next_sym(CI);
    }
    while (CI->cursym && (CI->cursym!=S_IDENT || CI->curr_name[0]!='_' || CI->curr_name[1]));
    next_sym(CI);
  }
  else // not delimited: ignore new keywords!
    while ((!is_any_key(CI->cursym, CI->curr_name) /*|| !strcmp(CI->curr_name, "NAME")*/) && CI->cursym)  // removed: NAME after ENUMARATE must be recognized as a delimiter!
    { pre_sym=(unsigned)(CI->compil_ptr-CI->univ_source);
      next_sym(CI);
    }
 // output the scanned text:
  if (pre_sym)
  { const char * start;  unsigned size;
    start=CI->univ_source+pre_key_pos-1;
    size=pre_sym-pre_key_pos;
    while (size && ((start[size-1]=='\r') || (start[size-1]==' ') ||
                    (start[size-1]=='\n'))) size--;
    while (size && (*start==' ')) { start++;  size--; }
    code_out(CI, start, CI->code_offset, size);
    CI->code_offset+=size;
    gen(CI,0);  /* string delimiter */
  }
}

BOOL skipdelim(CIp_t CI)
{ if (CI->cursym==S_IDENT && CI->curr_name[0]=='_' && !CI->curr_name[1]) 
    { next_sym(CI);  return TRUE; }
  return FALSE;
}

static void skip_comp(CIp_t CI)
{ do
  { if (CI->cursym==S_IDENT && CI->curr_name[0]=='_' && !CI->curr_name[1]) 
      do next_sym(CI);
      while (CI->cursym && (CI->cursym!=S_IDENT || CI->curr_name[0]!='_' || CI->curr_name[1]));
    if (is_any_key(CI->cursym, CI->curr_name) || !CI->cursym) break;
    next_sym(CI);
  } while (TRUE);
}

#ifdef WINS
#if WBVERS<900
static void skip_comp_actx(CIp_t CI, int cd)
{ do
  { if (cd == Y_PROP)
    { next_sym(CI); // PropName
      cd = find_tab_id(CI, wvw_tab_x); // Key
      if (cd == CTAX_FONT)
        do next_sym(CI); while (!is_any_key(CI->cursym, CI->curr_name) && CI->cursym);
      else if (cd == CTAX_PICTURE)
      { next_sym(CI); // PictName
      }
      else            // CTAX_VALUE  CTAX_ACCESS CTAX_COLOR  CTAX_DBPICT  CTAX_DBAPICT)
      { next_sym(CI); // _
        do next_sym(CI);
        while (CI->cursym && (CI->cursym!=S_IDENT || CI->curr_name[0]!='_' || CI->curr_name[1]));
        next_sym(CI); // Key
      }
    }
    else if (cd == Y_EVENT)
    { next_sym(CI); // EventName
      next_sym(CI); // _
      do next_sym(CI);
      while (CI->cursym && (CI->cursym!=S_IDENT || CI->curr_name[0]!='_' || CI->curr_name[1]));
      next_sym(CI); // Key
    }
    if (!CI->cursym)
      break;
    cd = find_tab_id(CI, wvw_tab_o);
  } while (CI->cursym && (cd == Y_PROP || cd == Y_EVENT));
}
#endif
#endif

static void skip_comp_rest(CIp_t CI, BOOL delimited)
{ if (delimited)
  { while (CI->cursym)
    { if (CI->cursym==S_IDENT && CI->curr_name[0]=='_' && !CI->curr_name[1]) 
        { next_sym(CI);  return; }
      next_sym(CI);
    }
  }
  else while (!is_any_key(CI->cursym, CI->curr_name) && CI->cursym) 
    next_sym(CI);
}

void cskip(CIp_t CI)
{ if (CI->cursym==',' || CI->cursym==';') next_sym(CI); }

void gen_var_string(CIp_t CI, uns8 code)
{ if (CI->cursym!=S_STRING) c_error(VIEW_COMP);
  gen(CI,code);
  gen2(CI,(uns16)(strlen(CI->curr_string())+1));
  genstr(CI, CI->curr_string());
  next_sym(CI);
}

#if WBVERS>=900

t_specif _wx_string_spec(0, 0, 0,
#ifdef LINUX
  2);
#else  // MSW
  1);
#endif

void genwstr(CIp_t CI, char * str)
{ int i=0;
  char convbuf[MAX_FIXED_STRING_LENGTH+2];
  superconv(ATT_STRING, ATT_STRING, str, convbuf, t_specif(), _wx_string_spec, NULL);
  int len=(int)(wcslen((wchar_t*)convbuf)+1)*(int)sizeof(wchar_t);
  code_out(CI, (tptr)convbuf, CI->code_offset, len);
  CI->code_offset+=len;

}
#endif

#ifdef LINUX
#define ANSI_CHARSET  0
#define FF_DONTCARE   0
#define DEFAULT_PITCH 0
#endif

void gen_font(CIp_t CI)
{ uns16 size;  code_addr aux_offset;  BYTE fnt;
  aux_offset=(code_addr)CI->code_offset;
  gen2(CI,0);
  gen2(CI,(WORD)num_val(CI, 1,65535L));   /* font size */
  gen(CI,0);    /* flags */
  cskip(CI);
  if (CI->cursym!=S_STRING) GEN(CI,(uns8)num_val(CI, 0,255))  /* ANSI/OEM/SYMBOL */
  else gen(CI,ANSI_CHARSET);
  cskip(CI);
  if (CI->cursym!=S_STRING) GEN(CI,(uns8)num_val(CI, 0,255))  /* Pitch & family */
  else gen(CI,FF_DONTCARE | DEFAULT_PITCH);
  cskip(CI);
  if (CI->cursym!=S_STRING) c_error(VIEW_COMP);
  genstr(CI, CI->curr_string());
  next_sym(CI);
  fnt=0;
  do
  { cskip(CI);
    switch (find_tab_id(CI, fonttab))
    { case Y_ITALICS:      fnt|=SFNT_ITAL;   break;
      case Y_BOLD:         fnt|=SFNT_BOLD;   break;
      case Y_UNDERLINED:   fnt|=SFNT_UNDER;  break;
      case Y_STRIKEOUT:    fnt|=SFNT_STRIKE; break;
      default: goto fontend;
    }
  } while (TRUE);
 fontend:
  size=(uns16)(CI->code_offset-aux_offset-2);
  code_out(CI, (tptr)&size, aux_offset,   2);
  code_out(CI, (tptr)&fnt,  aux_offset+4, 1);
}

static void gen_pict(CIp_t CI, BOOL full_comp)
{ uns16 size;
  code_addr aux_offset=CI->code_offset;
  CI->err_subobj=ERR_PICTURE;
  gen2(CI,0);
  if (full_comp)
  { tobjnum objnum;
   if (cd_Find_object(CI->cdp, CI->curr_name, CATEG_PICT, &objnum)) objnum=NOOBJECT;
    gen4(CI,objnum);
  }
  else
  { int i=0;
    do GEN(CI,CI->curr_name[i])  while (CI->curr_name[i++]);
  }
  next_sym(CI);
  size=(uns16)(CI->code_offset-aux_offset-2);
  code_out(CI, (tptr)&size, aux_offset,   2);
}

static void gen_toolbar(CIp_t CI, BOOL full_comp)
/* Toolbar buttons with deleted picures are removed during the compilation.
   Otherwise, they would be visible in the view, but not in the designer
   test view, because designer cannot re-create view source for deleted
   pictures. */
{ unsigned size;  code_addr aux_offset;  WORD flags, offset, msg;  int i;
  tobjnum bitmapnum;  code_addr startoff;
  unsigned loff=(WORD)-TBB_STEP_0, roff=(WORD)-TBB_STEP_0, step;
  aux_offset=CI->code_offset;
  CI->err_subobj=ERR_TOOLBAR;
  gen2(CI,0);
  while (CI->cursym != S_END)
  { if (CI->cursym==S_IDENT)   /* private picture sored in the database */
    { if (cd_Find_object(CI->cdp, CI->curr_name, CATEG_PICT, &bitmapnum))
        bitmapnum=NOOBJECT;
      flags=TBFLAG_PRIVATE;
    }
    else if (CI->cursym==S_INTEGER)  /* standard button */
    { bitmapnum=(tobjnum)CI->curr_int;
      flags=0;
    }
    else c_error(INTEGER_EXPECTED);
    next_sym(CI);
   /* old message number or name (currently always 0): */
    ctptr old_msg_start, old_msg_stop;
    old_msg_start=CI->prepos;
    msg=(WORD)num_val(CI, -1, 0xffff);
    old_msg_stop=CI->prepos;
    while (old_msg_stop>old_msg_start && old_msg_stop[-1]==' ')
      old_msg_stop--;
   /* offset and modifiers: */
    offset=(WORD)num_val(CI, 0, 2);
    while (CI->cursym==S_IDENT)
    { if (!strcmp(CI->curr_name, "COMMAND")) flags |= TBFLAG_COMMAND;
      if (!strcmp(CI->curr_name, "FRAME"))   flags |= TBFLAG_FRAMEMSG;
      if (!strcmp(CI->curr_name, "RIGHT"))   flags |= TBFLAG_RIGHT;
      next_sym(CI);
    }
    if (bitmapnum!=NOOBJECT)  /* skip deleted pictures: */
    { if (full_comp)
      { step=!offset ? TBB_STEP_0 : (offset==1) ? TBB_STEP_1 : TBB_STEP_2;
        if (flags & TBFLAG_RIGHT) offset=roff=roff+step;
        else offset=loff=loff+step;
      }
      startoff=CI->code_offset;
      gen2(CI,0);  gen2(CI,flags);  gen4(CI,bitmapnum);  gen2(CI,msg);  gen2(CI,offset);
    }
   /* status text: */
    if (CI->cursym!=S_STRING && CI->cursym!=S_CHAR) c_error(STRING_EXPECTED);
    if (bitmapnum!=NOOBJECT)  /* unless skipping deleted picture: */
    { i=0;
      do GEN(CI,CI->curr_string()[i])  while (CI->curr_string()[i++]);
    }
    next_sym(CI);
   /* action: */
    if (CI->cursym==S_IDENT && !strcmp(CI->curr_name, "_STMT_START"))
    { next_sym(CI);
      if (full_comp && bitmapnum!=NOOBJECT)  /* unless skipping deleted picture: */
      { while (CI->cursym!=S_IDENT || strcmp(CI->curr_name, "_STMT_END"))
        { statement(CI);
          if (CI->cursym==';') next_sym(CI);
        }
        gen(CI, I_STOP);
      }
      else
      { ctptr stmt_start=CI->prepos;
        while (CI->cursym!=S_IDENT || strcmp(CI->curr_name, "_STMT_END"))
          next_sym(CI);
        if (bitmapnum!=NOOBJECT)  /* unless skipping deleted picture: */
        { while (stmt_start < CI->prepos) gen(CI, *(stmt_start++));
          gen(CI, 0);   /* statement source terminator */
        }
      }
      next_sym(CI);   /* skipping "_STMT_END" */
    }
    else   /* obsolete version: create the "return xyz" statement: */
      if (bitmapnum!=NOOBJECT)  /* unless skipping deleted picture: */
      { if (full_comp) { geniconst(CI, msg);  gen(CI, I_EVENTRET); }
        { genstr(CI, "RETURN ");  CI->code_offset--;
          while (old_msg_start < old_msg_stop)
            gen(CI, *(old_msg_start++));
          gen(CI, 0);
        }
      }
    if (bitmapnum!=NOOBJECT)  /* unless skipping deleted picture: */
    { size=(uns16)(CI->code_offset-startoff);
      code_out(CI, (tptr)&size, startoff, 2);
    }
  }
  gen2(CI,0);
  gen2(CI,TBFLAG_STOP);
  next_sym(CI);
  size=(uns16)(CI->code_offset-aux_offset-2);
  code_out(CI, (tptr)&size, aux_offset,   2);
}

static void gen_qbe_name(CIp_t CI, code_addr * auxpos, uns8 ct_code)
{ unsigned auxsize;
  if ((CI->output_type!=DUMMY_OUT) && CI->univ_source) /* does not work unless univ_source used */
  { gen(CI,ct_code);        gen2(CI,0);   /* for QBE */
    unsigned prepos = pre_key_pos;
    non_comp(CI);
    auxsize=CI->code_offset-*auxpos-3;
    code_out(CI, (tptr)&auxsize, *auxpos+1, 2);
    CI->compil_ptr=CI->univ_source + prepos;  /* back in code */
    CI->curchar=pre_key_char;
    next_sym(CI);
    *auxpos=CI->code_offset;
  }
}

static typeobj * selref(CIp_t CI, uns8 * tp, t_specif * specif, uns8 * access)  
/* variable or attribute reference in a view */
{ int aflag;
  typeobj * tobj=selector(CI, SEL_REFPAR, aflag);
  specif->set_null();
  if (DIRECT(tobj))   /* a project variable */
  { *tp=tobj->type.simple.typenum;
    switch (*tp)
    { case ATT_TABLE:  case ATT_CURSOR:  case ATT_DBREC:   case ATT_STATCURS:
      case ATT_NIL:    case ATT_FILE:    case ATT_VARLEN:
        c_error(INCOMPATIBLE_TYPES);
#ifdef STOP
      case ATT_ODBC_TIME:       *size=sizeof(TIME_STRUCT);       break;
      case ATT_ODBC_DATE:       *size=sizeof(DATE_STRUCT);       break;
      case ATT_ODBC_TIMESTAMP:  *size=sizeof(TIMESTAMP_STRUCT);  break;
      case ATT_ODBC_DECIMAL:  case ATT_ODBC_NUMERIC:
        *size=CI->glob_db_specif.precision + CI->glob_db_specif.scale + 2;  break;
      case ATT_STRING:  
      case ATT_BINARY:
        *size=CI->glob_db_specif.length;  break;
      default:
        *size=tpsize[*tp];  break;
#endif
      default:
        *specif=CI->glob_db_specif;  break;
    }
    if      (aflag & CACHED_ATR) *access=ACCESS_CACHE;
    else if (aflag & PROJ_VAR)   *access=ACCESS_PROJECT;
    else                         *access=ACCESS_DB;
  }
  else
  { *tp=TtoATT(tobj);
    if (!*tp) c_error(INCOMPATIBLE_TYPES);  /* this type cannot be in a view item */
    *specif=T2specif(tobj);
    *access=ACCESS_PROJECT;
  }
  if (aflag & MULT_OBJ) *tp|=0x80;
  return tobj;
}

static typeobj * selval(CIp_t CI, uns8 * tp, t_specif * specif, int * eflag = NULL)  
/* variable or attribute reference in a view */
{ int aflag;
  if (eflag==NULL) eflag=&aflag;
  typeobj * tobj=expression(CI, eflag);
  specif->set_null();
  if (DIRECT(tobj))   /* a project variable */
  { *tp=ATT_TYPE(tobj);
    switch (*tp)
    { case ATT_NIL:    case ATT_FILE:    case ATT_VARLEN:  case ATT_STATCURS:
        c_error(INCOMPATIBLE_TYPES);
      default: // includes case ATT_DBREC:  case ATT_TABLE:  case ATT_CURSOR:  
        *specif=CI->glob_db_specif;  break;
    }
    if (*eflag & MULT_OBJ) c_error(INCOMPATIBLE_TYPES);
  }
  else
  { *tp    =TtoATT(tobj);   // may be 0
    *specif=T2specif(tobj);
  }
  return tobj;
}

#ifdef WINS
#if WBVERS<900
static void gen_prop(CIp_t CI, BOOL full_comp)
{ uns8 type, access = 0;  t_specif specif;  uns16 size;  code_addr aux_offset;
  if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
  CAXTypeInfo pInfo(CI->curr_name);
  if (CI->AXInfo->GetNameInfo(&pInfo))
  { SET_ERR_MSG(CI->cdp, CI->curr_name);
    c_error(IDENT_NOT_DEFINED);
  }
  gen(CI, CT_PROP);         // CT_PROP
  gen2(CI, 0);              // Size
  gen4(CI, pInfo.m_ID);     // MEMID
  gen2(CI, pInfo.m_Flags);  // Flags
  gen(CI,  0);              // Stat
  next_sym(CI);
  int sym = find_tab_id(CI, wvw_tab_x);
  gen(CI, sym);             // Prop categ
  switch (sym)
  {
  case CTAX_ACCESS:
    aux_offset=CI->code_offset;
    gen(CI, 0);             // Attr type
    gen2(CI, 0);            // Attr size
    gen(CI, 0);             // Attr acc
    CI->err_subobj=ERR_ACCESS;
    empty_index_inside=wbfalse;
    if (full_comp) 
    { skipdelim(CI);  
      selref(CI, &type, &specif, &access);
    }
    goto accval;
  case CTAX_COLOR:
    if (!pInfo.IsColor())
      c_error(INCOMPATIBLE_TYPES);
  case CTAX_VALUE:
    aux_offset=CI->code_offset;
    gen(CI, 0);             // Attr type
    gen2(CI, 0);            // Attr size
    gen(CI, 0);             // Attr acc
    CI->err_subobj=ERR_VALUE;
    empty_index_inside=wbfalse;
    if (full_comp) 
    { skipdelim(CI);  
      selval(CI, &type, &specif);
    }
  accval:
    size = type_specif_size(type, specif);
    code_out(CI, (tptr)&type,   aux_offset,   1);
    code_out(CI, (tptr)&size,   aux_offset+1, 2);
    code_out(CI, (tptr)&access, aux_offset+2, 1);
    if (full_comp) 
    { if (!same_type(type, pInfo.m_WBType.dtype) && (pInfo.m_WBType.dtype != ATT_UNTYPED))
        c_error(INCOMPATIBLE_TYPES);
      if (empty_index_inside)
        c_error(INDEX_MISSING);
      skipdelim(CI); 
      gen(CI,I_STOP);
    }
    else 
      non_comp(CI);
    break;
  case CTAX_FONT:
    if (!pInfo.IsFont())
      c_error(INCOMPATIBLE_TYPES);
    gen4(CI, 0);
    gen_font(CI);
    break;
  case CTAX_PICTURE:
  case CTAX_DBPICT:
  case CTAX_DBAPICT:
    if (!pInfo.IsPicture())
      c_error(INCOMPATIBLE_TYPES);
    gen4(CI, 0);
    if (sym == CTAX_PICTURE)
      gen_pict(CI, full_comp);
    else
    { gen2(CI, 0);
      if (!full_comp)
        non_comp(CI);
      else if (sym == CTAX_DBPICT)
      { skipdelim(CI);  
        selval(CI, &type, &specif);
        goto accpict;
      }
      else
      { skipdelim(CI);  
        selref(CI, &type, &specif, &access);
    accpict:
        if (!same_type(type, ATT_RASTER))
          c_error(INCOMPATIBLE_TYPES);
        if (empty_index_inside)
          c_error(INDEX_MISSING);
        skipdelim(CI);
        gen(CI,I_STOP);
      }
    }
    break;
  default:
    c_error(DEF_KEY_EXPECTED);
  }
}
#endif
#endif

void subr_def(CIp_t CI);
void new_id_table(CIp_t CI);

#ifdef WINS
#if WBVERS<900
static void gen_event(CIp_t CI, BOOL full_comp)
{ if (CI->cursym!=S_IDENT) c_error(IDENT_EXPECTED);
  CAXTypeInfo pInfo(CI->curr_name);
  if (CI->AXInfo->GetEventInfo(&pInfo))
  { SET_ERR_MSG(CI->cdp, CI->curr_name);
    c_error(IDENT_NOT_DEFINED);
  }
  gen(CI, CT_EVENT);        // CT_EVENT
  code_addr aux_offset=CI->code_offset;
  gen2(CI,0);               // Size
  gen4(CI, pInfo.m_ID);     // MEMID
  gen4(CI, 0);              // LocVarSz
  next_sym(CI);
  if (full_comp)
  { skipdelim(CI);
    new_id_table(CI);
    subr_def(CI);
    gen(CI,I_STOP);
    objtable * tb = CI->id_tables;
    CI->id_tables = tb->next_table;
    int ind;
    Upcase9(CI->cdp, pInfo.m_Name);
    search_table(pInfo.m_Name, (tptr)tb->objects, tb->objnum, sizeof(objdef), &ind);
    subrobj * subr = (subrobj *)tb->objects[ind].descr;
    int size = CI->code_offset - aux_offset;
    code_out(CI, (tptr)&size,   aux_offset, 2);
    code_out(CI, (tptr)&subr->localsize, aux_offset + 6, 4);
    corefree(tb);
    if (CI->cursym==';') 
      next_sym(CI);
    skipdelim(CI);
  }
  else
    non_comp(CI);  
}
#endif
#endif

#ifdef STOP
typedef struct
{ char  strbuf[256];
  uns32 numbuf;
} c_buf;

static void combo_select_compile(CIp_t CI, t_ctrl * itm)
/* current symbol is SELECT or query name */
{ const char * start, * stop;  char auxchar;
  tcursnum curs;  trecnum rec, reccnt;  const d_table * td;  const d_attr * a_d;
  uns8 tp2;  int i;  BOOL bad = FALSE;
  start=CI->prepos;
  while (*start==' ') start++;
  if (!strnicmp(start, "SELECT", 6))
  { /* end the cursor definition by null */
    while (!is_any_key(CI->cursym, CI->curr_name)) next_sym(CI);
    stop=CI->prepos;
    auxchar=*stop;  *(char*)stop=0;
    if (Open_cursor_direct(start, &curs)) c_error(COMBO_SELECT);
  }
  else   /* load the cursor definition */
  { next_sym(CI);
    stop=start;
#if WBVERS<900
    while (SYMCHAR(*stop)) stop++;
#else
    while (is_AlphanumA(*stop, CI->cdp->sys_charset)) stop++;
#endif
    auxchar=*stop;  *(char*)stop=0;  tobjnum objnum;
    if (Find_object(start, CATEG_CURSOR, &objnum)) c_error(COMBO_SELECT);
    if (Open_cursor(objnum, &curs)) c_error(COMBO_SELECT);
  }
  itm->ctStyle &= ~(long)(CBS_DROPDOWN | CBS_DROPDOWNLIST);
 /* analyse the cursor */
  td=cd_get_table_d(CI->cdp, curs, CATEG_DIRCUR);
  if (!td) bad=TRUE;
  else
  { if (td->attrcnt > 3) bad=TRUE;
    else
    { a_d=ATTR_N(td,1);
      if (!is_string(a_d->type) || (a_d->multi!=1)) bad=TRUE;
      else
      { if (td->attrcnt > 2)
        { a_d=NEXT_ATTR(td,a_d);  tp2=a_d->type;
          itm->ctStyle |= CBS_DROPDOWNLIST;  itm->ctClassNum=NO_COMBO;
          if ((tp2!=ATT_CHAR) && (tp2!=ATT_INT16) && (tp2!=ATT_INT32) ||
              !same_type(tp2, itm->type) || (a_d->multi!=1)) bad=TRUE;
        }
        else   /* single attribute: editable combo, no translation */
        { tp2=0;
          itm->ctStyle |= CBS_DROPDOWN;  itm->ctClassNum=NO_EC;
          itm->key_flags=DEFKF_EDIT;
        }
      }
    }
    release_table_d(td);
  }
  if (bad)
  { Close_cursor(curs);
    c_error(COMBO_SELECT);
  }
  else  /* cursor is OK */
  { if (!Rec_cnt(curs, &reccnt))
    { if (reccnt>1000) reccnt=1000;
#ifdef OLD_COMBO
      char strbuf[256];  uns32 numbuf;
      for (rec=0;  rec<reccnt;  rec++)
        /* records with empty strings are omitted, otherwise they would end the list */
        if (!Read(curs, rec, 1, NULL, strbuf) && *strbuf)
        { numbuf=0;
          if (!tp2 || !Read(curs, rec, 2, NULL, (tptr)&numbuf))
          { i=0;  while (strbuf[i]) gen(CI,strbuf[i++]);
            gen(CI,0);
            gen4(CI, numbuf);
          }
        }
#else  // read the combo list in packages!
      int combo_step=tp2 ? MAX_PACKAGED_REQS/2 : MAX_PACKAGED_REQS;
      if (combo_step>20) combo_step=20;
      c_buf * bufs = (c_buf*)corealloc(sizeof(c_buf)*combo_step, 51);
      if (bufs!=NULL)
      { rec=0;
        while (rec<reccnt)
        { int this_cnt=0;  start_package();
          do
          {   Read(curs, rec, 1, NULL,        bufs[this_cnt].strbuf);
            if (tp2)
            { bufs[this_cnt].numbuf=0;   /* may read char or short! */
              Read(curs, rec, 2, NULL, (tptr)&bufs[this_cnt].numbuf);
            }
            this_cnt++;  rec++;
          } while (this_cnt<combo_step && rec<reccnt);
          send_package();
          if (!Sz_error())
            for (int j=0;  j<this_cnt;  j++)
            /* records with empty strings are omitted, otherwise they would end the list */
              if (bufs[j].strbuf[0])
              { i=0;  while (bufs[j].strbuf[i]) gen(CI, bufs[j].strbuf[i++]);
                gen(CI, 0);
                gen4(CI, bufs[j].numbuf);   /* undefined iff tp2==0 */
              }
        }
        corefree(bufs);
      }
#endif
    }
  }
  Close_cursor(curs);
  gen(CI,0);
  *(char*)stop=auxchar;
}
#endif

enum view_comp_type { VWC_STRUCT, VWC_FULL, VWC_INFO };

static LONG gen_color(CIp_t CI, view_comp_type vwc, BOOL foreground)
// Compiles color information in the form.
{ CI->err_subobj = foreground ? ERR_FORECOL : ERR_BACKCOL;
  if (CI->cursym=='*') // new static color
    { next_sym(CI);  return num_val(CI, 0, 0x7fffffff); }
  else if (CI->cursym==S_IDENT && CI->curr_name[0]=='_' && !CI->curr_name[1])  // dynamic color
  { code_addr auxpos;  unsigned auxsize;
    auxpos=CI->code_offset;
    gen(CI, foreground ? CT_FORECOL : CT_BACKCOL);  gen2(CI,0);
    if (vwc==VWC_FULL)
    { next_sym(CI);
      typeobj * tobj=expression(CI);
      if (tobj!=&att_int16 && tobj!=&att_int32) c_error(MUST_BE_INTEGER);
      gen(CI, I_STOP);
      skipdelim(CI);
    }
    else non_comp(CI);
    auxsize=CI->code_offset-auxpos-3;
    code_out(CI, (tptr)&auxsize, auxpos+1, 2);
    return DYNAMIC_COLOR;
  }
  else                 // old static color
  { LONG col=num_val(CI, 0, 0x7fffffff);
#if WBVERS<900
#ifdef WINS
    if (!col) return foreground ? WBSYSCOLOR(COLOR_WINDOWTEXT) : WBSYSCOLOR(COLOR_WINDOW);
    return color_short2long((uns8)col);
#else
    return 128;    
#endif
#else
    return 128;
#endif
  }
}

#ifdef LINUX
#define SS_LEFT  0
#define SS_RIGHT  1
#define SS_CENTER  2
#define SS_ICON  4
#define CBS_DROPDOWN   0
#define CBS_DROPDOWNLIST 0
#define SS_BLACKFRAME 123
#define WS_BORDER 0
#define WS_VSCROLL 0
#define ES_MULTILINE 1
#define ES_LEFT 0
#define BS_3STATE 0
#define BS_RADIOBUTTON 2
#define BS_PUSHBUTTON 0
#define BS_DEFPUSHBUTTON 1
#define BS_GROUPBOX 8
#define LBS_NOTIFY 0
#endif


static void view_comp(CIp_t CI, view_comp_type vwc)
{ int cd, i;  code_addr auxpos;  unsigned auxsize;  typeobj * tobj;
  t_integrity integrity;  t_control_info control_info;  BOOL explicit_control_name;
  view_stat a_view;  /* compiled view info */
  t_ctrl    a_ctrl;  /* compiled contol info */
  code_addr ctrl_offset;  /* start of the control code */
  LONG      vwForeCol;  /* default foreground for items */
  uns8 duplic[MAX_CTRL_ID/8+1];  memset(duplic, 0, sizeof(duplic));
  WORD key_flags;  BOOL key_flags_specified;
#if WBVERS<900
  BOOL AXEventIID;
#endif
  CI->comp_flags|=COMP_FLAG_VIEW;
/* header - fixed part */
  CI->err_object=ERR_VIEW_HEADER;
  if (CI->cursym=='-') next_sym(CI);  // cursor number
  next_sym(CI);  /* skipping name: */
  if (CI->cursym=='.') { next_sym(CI);  next_sym(CI); }
  if (CI->cursym=='.') { next_sym(CI);  next_sym(CI); }
  cd=find_tab_id(CI, wvw_tab_h);
  if ((cd!=Y_TABDIALOG)&&(cd!=Y_CURDIALOG)&&(cd!=Y_DIALOG)) c_error(VIEW_COMP);
  cskip(CI);
  CI->err_subobj=ERR_X;
  a_view.vwX =(int)num_val(CI, 0, 0xff0000);
  cskip(CI);
  CI->err_subobj=ERR_Y;
  a_view.vwY =(int)num_val(CI, 0, 65535L);
  cskip(CI);
  CI->err_subobj=ERR_CX;
  a_view.vwCX=(int)num_val(CI, 0, 65535L);
  cskip(CI);
  CI->err_subobj=ERR_CY;
  a_view.vwCY=(int)num_val(CI, 0, 65535L);
  cskip(CI);
  a_view.vwStyle=a_view.vwHdFt=a_view.vwStyle3=0;  a_view.vwItemCount=0;   
  a_view.vwCodeSize=0;
#ifdef WINS
  a_view.vwBackCol=WBSYSCOLOR(COLOR_WINDOW); /* == default */ vwForeCol=WBSYSCOLOR(COLOR_WINDOWTEXT);
#else
  a_view.vwBackCol=(uns32)-1; /* == default */ vwForeCol=0;
#endif
#if WBVERS<=810
  a_view.vwHelpInfo=HE_ANY_VIEW;
#endif  
  a_view.vwIsLabel=FALSE;
  for (i=0;  i<ALL_PANES;  i++) a_view.panesz[i] = -1;  // not specified
  /* code_out((tptr)&a_view, 0, sizeof(view_stat)); moved to the end */
  if (vwc!=VWC_INFO) CI->code_offset+=sizeof(view_stat);
 /* header - variable part */
 
  if (vwc==VWC_INFO) // skip the header
  { bool in_statement = false;
    do  // cycle until BEGIN not inside _ ... _ found
    { cd=find_tab_id(CI, wvw_tab_h);
      if (cd==Y_BEGIN && !in_statement) break;
      if (cd==Y_NOTFOUND)
        if (skipdelim(CI)) in_statement=!in_statement;
        else next_sym(CI); 
      else if (cd==Y_NOIDENT || cd==Y_END) next_sym(CI); 
    } while (true);
  }
  else // compile the header
  while ((cd=find_tab_id(CI, wvw_tab_h)) != Y_BEGIN)
  { switch (cd)
    { case Y_STYLE:
        a_view.vwStyle=num_val(CI, (sig32)0x80000000L, 0x7fffffffL);
        if (a_view.vwStyle & NO_VIEW_INSERT) /* obsolete flag */
          a_view.vwHdFt |= NO_VIEW_INS | NO_VIEW_FIC;
        break;
      case Y_HDFTSTYLE:
        a_view.vwHdFt |= num_val(CI, (sig32)0x80000000L, 0x7fffffffL);
        break;
      case Y_STYLE3:
        a_view.vwStyle3 = num_val(CI, (sig32)0x80000000L, 0x7fffffffL);
        break;
      case Y_COLOUR:
        vwForeCol       =gen_color(CI, vwc, TRUE);   break;
      case Y_BACKGROUND:
        a_view.vwBackCol=gen_color(CI, vwc, FALSE);  break;
      case Y_CAPTION:
        CI->err_subobj=ERR_CAPTION;
        gen_var_string(CI, VW_CAPTION);
        break;
      case Y_FONT:
        CI->err_subobj=ERR_FONT;
        gen(CI,VW_FONT);
        gen_font(CI);
        break;
      case Y_HELP:
        CI->err_subobj=ERR_HELP;
        a_view.vwHelpInfo=(UINT)num_val(CI, -1, 0x7fffffffLU);
        break;
      case Y_LABEL:
        CI->err_subobj=ERR_LABEL;
        a_view.vwIsLabel=TRUE;
        a_view.vwXX=(WORD)num_val(CI, 1, 65535L);
        a_view.vwYY=(WORD)num_val(CI, 1, 65535L);
        a_view.vwXspace=(WORD)num_val(CI, 0, 65535L);
        a_view.vwYspace=(WORD)num_val(CI, 0, 65535L);
        a_view.vwXcnt  =(BYTE)num_val(CI, 1, 255);
        a_view.vwYcnt  =(BYTE)num_val(CI, 1, 255);
        a_view.vwXmarg =(WORD)num_val(CI, 0, 65535L);
        a_view.vwYmarg =(WORD)num_val(CI, 0, 65535L);
        break;
      case Y_GROUP:
        CI->err_object=ERR_GROUP_DEF;  CI->err_subobj=ERR_NOGROUP;
        *CI->curr_name &= 0xdf;
        if ((CI->cursym!=S_IDENT) || CI->curr_name[1] ||
            (*CI->curr_name < 'A') || (*CI->curr_name > 'E'))
          c_error(VIEW_COMP);
        auxpos=CI->code_offset;
        gen(CI,(uns8)(VW_GROUPA+*CI->curr_name-'A'));       gen2(CI,0);
      CI->err_subobj=ERR_GROUPA+*CI->curr_name-'A';
        pre_key_pos=(unsigned)(CI->compil_ptr-CI->univ_source);
        pre_key_char=CI->curchar;
        next_sym(CI);
        cd=0;
       /* this part can be removed in future versions (manuals allow this) */
      if ((CI->cursym==S_IDENT) && !my_stricmp(CI->curr_name, "PREPAGE"))
        { /* cd|=BREAK_BEF; -- obsolete */
          pre_key_pos=(unsigned)(CI->compil_ptr-CI->univ_source);
          pre_key_char=CI->curchar;
          next_sym(CI);
        }
      if ((CI->cursym==S_IDENT) && !my_stricmp(CI->curr_name, "POSTPAGE"))
        { /*cd|=BREAK_AFT; -- obsolete */
          pre_key_pos=(unsigned)(CI->compil_ptr-CI->univ_source);
          pre_key_char=CI->curchar;
          next_sym(CI);
        }
      if ((CI->cursym==S_IDENT) && !my_stricmp(CI->curr_name, "PREPAGE"))
        { /*cd|=BREAK_BEF; -- obsolete */
          pre_key_pos=(unsigned)(CI->compil_ptr-CI->univ_source);
          pre_key_char=CI->curchar;
          next_sym(CI);
        }

        gen(CI,(uns8)cd);  /* compiled even if !full_comp */
        if (vwc==VWC_FULL)
        { gen(CI,0);  gen4(CI,0);   // reserved for type and specif at auxpos+4
          tobj=expression(CI);
          uns8 tp = (uns8)DIRTYPE(tobj);
          t_specif specif = CI->glob_db_specif;
          if (is_string(tp)) specif.length++;
          else if (tp!=ATT_BINARY) specif.length=tpsize[tp];
          code_out(CI, (tptr)&tp, auxpos+4, 1);
          code_out(CI, (tptr)&specif, auxpos+5, sizeof(t_specif));
          gen(CI,I_STOP);
        }
        else non_comp(CI);
        auxsize=CI->code_offset-auxpos-3;
        code_out(CI, (tptr)&auxsize, auxpos+1, 2);
        CI->err_object=ERR_VIEW_HEADER;  /* return to header */
        break;
      case Y_PICTURE:
        gen(CI,VW_PICTURE);
        gen_pict(CI, vwc==VWC_FULL);
        break;
      case Y_TOOLBAR:
        gen(CI,VW_TOOLBAR);
        gen_toolbar(CI, vwc==VWC_FULL);
        break;
      case Y_NEWREC:
        break;
      case Y_PROJECT_NAME:
        if (CI->cursym!=S_IDENT) c_error(VIEW_COMP);
       // set the project up:
        if (strcmp(CI->curr_name, CI->cdp->RV.run_project.project_name))
        { if (CI->comp_flags & COMP_FLAG_DISABLE_PROJECT_CHANGE) // in-code forms must not change project!
            { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(CANNOT_SET_PROJECT); }
          if (CI->cdp->RV.global_state==PROG_RUNNING)
            if (vwc==VWC_FULL)
              { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(CANNOT_SET_PROJECT); }
          if (CI->cdp->RV.run_project.global_decls==CI->id_tables)
            CI->id_tables=CI->id_tables->next_table; // remove the old global table, will be deleted
#if WBVERS<900
          if (set_project_up(CI->cdp, CI->curr_name, NOOBJECT, FALSE, FALSE)) 
          { if (vwc==VWC_FULL)
              { SET_ERR_MSG(CI->cdp, CI->curr_name);  c_error(CANNOT_SET_PROJECT); }
          }
          else // activate the new global table
#endif
          { CI->cdp->RV.run_project.global_decls->next_table = CI->id_tables;
            CI->id_tables=CI->cdp->RV.run_project.global_decls;
          }
        }
       // generate to the view structures:
        gen(CI, VW_PROJECT_NAME);
       gen_name:
        auxpos=CI->code_offset;
        gen2(CI,0);
        genstr(CI, CI->curr_name);
        auxsize=CI->code_offset-auxpos-2;
        code_out(CI, (tptr)&auxsize, auxpos, 2);
        next_sym(CI);
        break;
      case Y_TEMPLATE_NAME:
        if (CI->cursym!=S_IDENT) c_error(VIEW_COMP);
        gen(CI, VW_TEMPLATE_NAME);
        goto gen_name;
      case Y_PANE_HEIGHTS:
        for (i=PANE_FIRST;  i<=PANE_LAST;  i++)
          a_view.panesz[i] = (short)num_val(CI, -1, 0x7fff);
        break;
      case Y_OPENACTION:
        gen(CI, VW_OPENACTION);   CI->err_subobj=ERR_OPENACTION;
        goto vw_statement_code;
      case Y_CLOSEACTION:
        gen(CI, VW_CLOSEACTION);  CI->err_subobj=ERR_CLOSEACTION;
        goto vw_statement_code;
      case Y_RCLICK:
        gen(CI, VW_RCLICK);       CI->err_subobj=ERR_RCLICK;
        goto vw_statement_code;
      case Y_DBLCLICK:
        gen(CI, VW_DBLCLICK);     CI->err_subobj=ERR_DBLCLICK;
        goto vw_statement_code;
      case Y_NEWRECACT:
        gen(CI, VW_NEWRECACT);    CI->err_subobj=ERR_NEWRECACT;
       vw_statement_code:
        auxpos=CI->code_offset;
        gen2(CI,0);
        if (vwc==VWC_FULL)
        { BOOL delimited = skipdelim(CI);
          do
          { statement(CI);
            if (CI->cursym!=';') { skipdelim(CI);  break; }
            next_sym(CI);
          } while (delimited ? !skipdelim(CI) : !is_any_key(CI->cursym, CI->curr_name));
        }
        else non_comp(CI);
        gen(CI,I_STOP);
        auxsize=CI->code_offset-auxpos-2;
        code_out(CI, (tptr)&auxsize, auxpos, 2);
        break;

      default: c_error(VIEW_COMP);
    }
    cskip(CI);
  } // cycle on header elements
  a_view.vwCodeSize=(uns16)(CI->code_offset-sizeof(view_stat));

 /* controls */
  while ((cd=find_tab_id(CI, wvw_tab_c)) != Y_END)
  { if (vwc==VWC_INFO)
    { control_info.name[0]=0;  control_info.type=0; 
      explicit_control_name=FALSE;
    }
    else
    { a_ctrl.pane=PANE_DATA;
      a_ctrl.ctStyle=0;
      a_ctrl.ctStylEx=0;
      a_ctrl.precision=5;
      a_ctrl.forecol=vwForeCol;
      a_ctrl.ctBackCol=a_view.vwBackCol;
      a_ctrl.type=0;  a_ctrl.specif.set_null();
      a_ctrl.access=ACCESS_NO;
      a_ctrl.ctCodeSize=0;
      a_ctrl.key_flags=DEFKF_STD;  key_flags_specified=FALSE;
      a_ctrl.ctHelpInfo=0;
      a_ctrl.framelnwdth=1;
#if WBVERS>=900
      a_ctrl.column_number = -1;
#endif
      integrity.name[0]=0;
      ctrl_offset=CI->code_offset;
      /* code_out((tptr)&a_ctrl, ctrl_offset, sizeof(t_ctrl)); moved to the end */
      CI->code_offset+=sizeof(t_ctrl);
    }
    switch (cd)
    { case Y_LTEXT:
        a_ctrl.ctStyle=SS_LEFT;
        a_ctrl.ctClassNum=NO_STR;
       std_control:
       /* text part */
        if (CI->cursym==S_STRING)
        { if (*CI->curr_string() && vwc!=VWC_INFO)
          { if (a_ctrl.ctClassNum==NO_TAB && !CI->translation_disabled) // must translate per partes
            { tptr inp=(tptr)corealloc(strlen(CI->curr_string())+1, 77);
              tptr out=(tptr)corealloc(MAX_STRING_LEN, 77);
              if (inp && out)
              { strcpy(inp, CI->curr_string());
                tptr p=inp;  unsigned outpos=0;
                while (*p)
                { unsigned i=0;
                  do i++; while (p[i] && p[i]!=',');
                  memcpy(CI->curr_string(), p, i);  CI->curr_string()[i]=0;
                  cutspaces(CI->curr_string());  translate_string(CI);
                  if (outpos && outpos<MAX_STRING_LEN) out[outpos++]=',';
                  strmaxcpy(out+outpos, CI->curr_string(), MAX_STRING_LEN-outpos);  outpos+=(int)strlen(out+outpos);
                  if (p[i]==',') i++;  p+=i;
                }
                put_string_char(CI, (int)strlen(out), 0);  // allocate the string
                strcpy(CI->curr_string(), out);
                corefree(inp);  corefree(out);
              }
            }
            gen_var_string(CI, CT_TEXT);
          }
          else next_sym(CI);
          if (a_ctrl.ctClassNum==NO_VIEW) a_ctrl.ctStyle|=SUBVW_RELATIONAL;
          cskip(CI);
        }
       /* control ID */
        CI->err_subobj=ERR_ID;
        a_ctrl.ctId=(int)num_val(CI, 1, MAX_CTRL_ID);
        CI->err_object=ERR_VIEW_ITEM+a_ctrl.ctId; /* set when item started */
          if (duplic[a_ctrl.ctId/8] & (1 << (a_ctrl.ctId % 8)))
            c_error(ID_DUPLICITY);
        else duplic[a_ctrl.ctId/8] |= (1 << (a_ctrl.ctId % 8));
        cskip(CI);
       /* dimensions */
        CI->err_subobj=ERR_X;
        a_ctrl.ctX =(int)num_val(CI, MINUS_LIMIT, 0xff0000U);
        cskip(CI);
        CI->err_subobj=ERR_Y;
        a_ctrl.ctY =(int)num_val(CI, MINUS_LIMIT, 65535L);
        cskip(CI);
        CI->err_subobj=ERR_CX;
        a_ctrl.ctCX=(int)num_val(CI, 0, 65535L);
        cskip(CI);
        CI->err_subobj=ERR_CY;
        a_ctrl.ctCY=(int)num_val(CI, 0, 65535L);
        cskip(CI);
       /* optional style */
        CI->err_subobj=ERR_STYLE;
        if (CI->cursym==S_INTEGER)
        { a_ctrl.ctStyle|=num_val(CI, (sig32)0x80000000L, 0x7fffffffL);
          if (a_ctrl.ctClassNum==NO_COMBO &&
              (a_ctrl.ctStyle&(CBS_DROPDOWN|CBS_DROPDOWNLIST))==CBS_DROPDOWN)
          { a_ctrl.ctClassNum=NO_EC;
            a_ctrl.key_flags=DEFKF_EDIT;
          }
          if (a_ctrl.ctClassNum==NO_VAL || a_ctrl.ctClassNum==NO_STR)
          {      if ((a_ctrl.ctStyle & 0xff)==SS_TEXT)   a_ctrl.ctClassNum=NO_TEXT;
            else if ((a_ctrl.ctStyle & 0xff)==SS_RASTER) a_ctrl.ctClassNum=NO_RASTER;
            else if ((a_ctrl.ctStyle & 0xff)==SS_BLACKFRAME) a_ctrl.ctClassNum=NO_FRAME;
            /* for old views only: */
            if (a_ctrl.ctStyle & (FRAMED | SHADED)) a_ctrl.ctStyle |= WS_BORDER;
          }
          cskip(CI);
        }
        CI->err_subobj=ERR_KEY_FLAGS;
        if (CI->cursym==S_INTEGER)
        { key_flags=(uns16)num_val(CI, 0, 0xffffLU);  key_flags_specified=TRUE;
          cskip(CI);
        }
       /* additional specifications - Sezam extension */
        do
        { cskip(CI);
          cd=find_tab_id(CI, wvw_tab_o);
          auxpos=CI->code_offset;
          if (vwc==VWC_INFO)
          { if (cd==Y_ACCESS || cd==Y_VALUE)
            { BOOL delimited=skipdelim(CI);  
              if (CI->cursym==S_IDENT && a_ctrl.ctClassNum!=NO_VIEW)
              { if (!explicit_control_name)
                  if (!control_info.name[0] || cd==Y_ACCESS)
                    strmaxcpy(control_info.name, CI->curr_name, sizeof(tname));
              }
              CI->output_type=DUMMY_OUT;
#ifdef WINS
              __try
#else
	      LONGJMPBUF jmp;
             if (!Catch(jmp))
#endif
              { uns8 tp;
                if (cd==Y_VALUE)
                { selval(CI, &tp, &a_ctrl.specif);
                  if (!control_info.type) control_info.type=tp;  // unless type defined by Y_ACCESS
                }
                else
                { selref(CI, &tp, &a_ctrl.specif, &a_ctrl.access);
                  control_info.type=tp;
                }
                skipdelim(CI);
              }
#ifdef WINS
              __except ((GetExceptionCode() >= FIRST_COMP_ERROR &&
                         GetExceptionCode() <= LAST_COMP_ERROR ||
                         GetExceptionCode()==99)  /* ignored error */
                   ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
#else
              else
#endif
            { skip_comp_rest(CI, delimited); };
              CI->code_offset=auxpos;  CI->output_type=MEMORY_OUT;
              if (a_ctrl.ctClassNum==NO_STR) a_ctrl.ctClassNum=NO_VAL;
            }
            else if (cd==Y_DEFAULT && (a_ctrl.ctClassNum==NO_VIEW || a_ctrl.ctClassNum==NO_ACTX))
            { skipdelim(CI);
              if (CI->cursym!=S_STRING && CI->cursym!=S_CHAR) c_error(STRING_EXPECTED);
              if (a_ctrl.ctClassNum==NO_VIEW)
              { if (!explicit_control_name)
                { strmaxcpy(control_info.name, CI->curr_string(), sizeof(control_info.name)); // object name used as the default control name
                  Upcase9(CI->cdp, control_info.name);
                }
                cd_Find_object(CI->cdp, CI->curr_string(), CATEG_VIEW, (tobjnum*)control_info.clsid);
              }
#ifdef WINS
#if WBVERS<900
              else //if (a_ctrl.ctClassNum==NO_ACTX)
              { skipdelim(CI);
                if (CI->cursym!=S_STRING && CI->cursym!=S_CHAR) c_error(STRING_EXPECTED);
                OLECHAR buff[256];
                A2W(CI->curr_string(), buff, lstrlen(CI->curr_string())+1);
                CLSIDFromProgID(buff, (CLSID*)control_info.clsid);
              }
#endif
#endif
              next_sym(CI);  skipdelim(CI);  
            }
            else if (cd==Y_NAME)
            { strmaxcpy(control_info.name, CI->curr_name, sizeof(tname));
              explicit_control_name=TRUE;  next_sym(CI);
            }
            else if (cd==Y_NOTFOUND || cd==Y_END || !cd) break;
#ifdef WINS
#if WBVERS<900
	    else if (cd==Y_PROP || cd==Y_EVENT)
              skip_comp_actx(CI, cd);
#endif	      
#endif	      
            else
              skip_comp(CI);
          }
          else 
          { BOOL emitted=FALSE;
            switch (cd)
            { case Y_ACCESS:
                gen_qbe_name(CI, &auxpos, CT_NAMEA);
                if (vwc==VWC_FULL) skipdelim(CI);  
                if (CI->cursym==S_IDENT)
                  strcpy(integrity.name, CI->curr_name);
                gen(CI,CT_GET_ACCESS);  gen2(CI,0);
                CI->err_subobj=ERR_ACCESS;
                empty_index_inside=wbfalse;
                if (vwc==VWC_FULL) 
                { selref(CI, &a_ctrl.type, &a_ctrl.specif, &a_ctrl.access);
#if WBVERS>=900
                  a_ctrl.column_number = CI->last_column_number;
#endif
                  if (a_ctrl.ctClassNum==NO_LIST) 
                    if (!empty_index_inside) c_error(NOT_AN_ARRAY);  // otherwise does not work
                  skipdelim(CI);
                }
                else non_comp(CI);
                goto objacc;
              case Y_VALUE:
                gen_qbe_name(CI, &auxpos, CT_NAMEV);
                if (vwc==VWC_FULL) skipdelim(CI);  
                gen(CI,CT_GET_VALUE);   gen2(CI,0);
                CI->err_subobj=ERR_VALUE;
                empty_index_inside=wbfalse;
                if (vwc==VWC_FULL) 
                { uns8 tp;  t_specif specif;
                  selval(CI, &tp, &specif);
                  if (tp==ATT_DBREC || tp==ATT_TABLE || tp==ATT_CURSOR || tp==ATT_STATCURS) c_error(INCOMPATIBLE_TYPES);
                  if (!a_ctrl.type) { a_ctrl.type=tp;  a_ctrl.specif=specif; } // unless type/specif defined by Y_ACCESS
                  else if (!same_type(a_ctrl.type, tp))
                    c_error(INCOMPATIBLE_TYPES);
                  else if (a_ctrl.type==ATT_INT32 && tp==ATT_INT16)
                    a_ctrl.type=ATT_INT16;  /* checkbox on short needs it ?? */
                  skipdelim(CI); 
                }
                else non_comp(CI);
               objacc:
                if (a_ctrl.ctClassNum==NO_STR) a_ctrl.ctClassNum=NO_VAL;
                if (empty_index_inside) a_ctrl.ctStyle |= INDEXABLE;
                if (vwc==VWC_FULL)
                { gen(CI,I_STOP);
                  if (a_ctrl.type==ATT_TEXT && a_ctrl.ctClassNum==NO_EDI)
                  { a_ctrl.ctStyle|=ES_MULTILINE;
                    a_ctrl.key_flags=DEFKF_MULEDIT;
                  }
                }
                emitted=TRUE;  break;
              case Y_INTEGRITY:
                gen(CI,CT_INTEGRITY);
                CI->err_subobj=ERR_INTEGRITY; /* this enables '#' in expressions */
                gen2(CI,0);  /* size */
                if (integrity.name[0])
                { integrity.att_type=a_ctrl.type; /* must be defined before */
                  CI->univ_ptr=&integrity;
                } else CI->univ_ptr=NULL;
                if (vwc==VWC_FULL) 
                  { skipdelim(CI);  bool_expr(CI);  skipdelim(CI); }
                else non_comp(CI);
                CI->univ_ptr=NULL;
                CI->err_subobj=0; /* this disables '#' in expressions */
                gen(CI,I_STOP);
                emitted=TRUE;  break;
              case Y_ENABLED:
                gen(CI,CT_ENABLED);              CI->err_subobj=ERR_ENABLED;
                goto boolexpr;
              case Y_VISIBILITY:
                gen(CI,CT_VISIBLE);              CI->err_subobj=ERR_VISIBILITY;
               boolexpr:
                gen2(CI,0);  /* size */
                if (vwc==VWC_FULL) 
                  { skipdelim(CI);  bool_expr(CI);  skipdelim(CI); }
                else non_comp(CI);
                gen(CI,I_STOP);
                emitted=TRUE;  break;
              case Y_ONERROR:
                gen(CI,CT_ONERROR);              CI->err_subobj=ERR_ONERROR;
                goto statement_code;
              case Y_RCLICK:
                gen(CI,CT_RCLICK);               CI->err_subobj=ERR_RCLICK;
                goto statement_code;
              case Y_ACTION:
                gen(CI,CT_ACTION);               CI->err_subobj=ERR_ACTION;
               statement_code:
                gen2(CI,0);
                if (vwc==VWC_FULL)
                { BOOL delimited = skipdelim(CI);
                  tobjname loc_name;  *loc_name=0; // name of the opened view
                  CI->univ_ptr=loc_name;
                  do
                  { statement(CI);
                    if (CI->cursym!=';') { skipdelim(CI);  break; }
                    next_sym(CI);
                  } while (delimited ? !skipdelim(CI) : !is_any_key(CI->cursym, CI->curr_name));
                  gen(CI,I_STOP);
                  CI->univ_ptr=NULL;
                  if (*loc_name)
                  { auxsize=CI->code_offset-auxpos-3;
                    code_out(CI, (tptr)&auxsize, auxpos+1, 2);
                    auxpos=CI->code_offset;
                    gen(CI,CT_NAMEV);  gen2(CI,0);
                    genstr(CI, loc_name);
                  }
                }
                else non_comp(CI);
                emitted=TRUE;  break;
              case Y_QUERYCOND:
                CI->err_subobj=ERR_QUERYCOND;
                gen_var_string(CI, CT_QUERYCOND);
                emitted=TRUE;  break;
              case Y_FONT:
                gen(CI,CT_FONT);                 CI->err_subobj=ERR_FONT;
                gen_font(CI);
                break;
              case Y_ENUMERATE:
                gen(CI,CT_ENUMER);               CI->err_subobj=ERR_ENUMER;
                gen2(CI,0);
                if (vwc==VWC_FULL)
                { skipdelim(CI);
                  long nextval=0;
                  while ((CI->cursym==S_STRING) || (CI->cursym==S_CHAR))
                  { 
#if WBVERS>=900
                    genwstr(CI, CI->curr_string());
#else
                    genstr(CI, CI->curr_string());
#endif
                    next_sym(CI);
                    cskip(CI);
                    if ((CI->cursym==S_INTEGER)||(CI->cursym=='-'))
                      nextval=num_val(CI, (sig32)0x80000000L, 0x7fffffffL);
                    else if (CI->cursym==S_CHAR)
                      { nextval=*CI->curr_string();  next_sym(CI); }
                    else if (CI->cursym==S_IDENT)
                    { object * obj;  uns8 obj_level;
                      obj=search_obj(CI, CI->curr_name, &obj_level);
                      if (obj)  /* program-defined identifier */
                        if (obj->any.categ==O_CONST)
                          nextval=num_val(CI, (sig32)0x80000000L, 0x7fffffffL);
                    }
                    cskip(CI);
                    gen4(CI, nextval);
                    nextval++;
                  }
                  skipdelim(CI);
#if WBVERS>=900
                  gen4(CI,0);  // 4 for LINUX, 2 would be sufficient for MSW
#else
                  gen(CI,0);
#endif
                }
                else non_comp(CI);
                emitted=TRUE;  break;
              case Y_COMBOSELECT:
                gen(CI,CT_COMBOSEL);               CI->err_subobj=ERR_ENUMER;
                gen2(CI,0);  /* part size */
                CI->err_subobj=ERR_COMBOSEL;
                emitted=TRUE;  non_comp(CI);
                break;
              case Y_PICTURE:
                //if (a_ctrl.ctClassNum==NO_BUTT || a_ctrl.ctClassNum==NO_CHECK ||
                //    a_ctrl.ctClassNum==NO_RADIO)
  //              if ((a_ctrl.ctStyle & 0xff) == BS_PUSHBUTTON || (a_ctrl.ctStyle & 0xff) == BS_DEFPUSHBUTTON)
  //              check boxes and radio buttons can have a picture too: 2-state buttons
                  //if (vwc==VWC_FULL) // view designed must not receive BS_OWNERDRAW, it cannot remove this
                  //  a_ctrl.ctStyle=(a_ctrl.ctStyle & 0xffffffe0L) | BS_OWNERDRAW;
                  // BS_DEFPUSHBUTTON info must be preserved even for buttons with a picture
                gen(CI,CT_PICTURE);
                gen_pict(CI, vwc==VWC_FULL);
                break;
              case Y_STATUSTEXT:
                CI->err_subobj=ERR_STATUSTEXT;
               // like gen_var_string(CI, CT_STATUSTEXT), but with additional translation of ^@...
                if (CI->cursym!=S_STRING) c_error(VIEW_COMP);
                gen(CI, CT_STATUSTEXT);
                if (CI->curr_string()[0]=='^' && CI->curr_string()[1]=='@')
                { memmov(CI->curr_string(), CI->curr_string()+1, strlen(CI->curr_string())+1);
                  translate_string(CI);
                  memmov(CI->curr_string()+1, CI->curr_string(), strlen(CI->curr_string())+1);
                  CI->curr_string()[0]='^';
                }
                gen2(CI, (uns16)(strlen(CI->curr_string())+1));
                genstr(CI, CI->curr_string());
                next_sym(CI);
                break;
#if WBVERS<=810                
              case Y_MASK:
                CI->err_subobj=ERR_MASK;
                if (CI->cursym!=S_STRING) c_error(VIEW_COMP);
                if (vwc==VWC_FULL && CI->curr_string()[1] & MASK_STD)
                { //if (strlen(CI->curr_string()) < 2+2*STD_MASK_MAX_SIZE) c_error(VIEW_COMP);
                  //no standard mask is bigger than MIN_STRING_LEN-3!
                  tptr p=NULL;
                  switch (CI->curr_string()[2]-MASK_STDBASE)
                  { case 0: p="\x41\x40\x41\x40\x41\x40 \x43\x41\x40\x41\x40";
                      break;  /* PSC */
                    case 1: p="\x43\x48\x43\x48\x43\x4a \x43\x41\x40\x41\x40-\x43\x41\x40\x41\x40";
                      break;  /* SPZ */
                    case 2: p="\x41\x40\x41\x40\x41\x40\x41\x40\x41\x40\x41\x40/\x43\x41\x40\x41\x40\x41\x40\x41\x42";
                      break;  /* rodne cislo */
                    case 3: p="\x41\x40\x41\x40\x41\x40\x41\x40\x41\x40\x41\x40\x41\x40\x41\x40";
                      break;  /* ICO */
                    case 4:
                      if (a_ctrl.precision==0) /* no year */
                        p="\x41\x42\x41\x42.\x43\x41\x42\x41\x42.\x43";
                      else
                        p="\x41\x42\x41\x42.\x43\x41\x42\x41\x42.\x43\x41\x42\x41\x42\x41\x42\x41\x42";
                      break;  /* date */
                    case 5:
                      if (a_ctrl.precision==0) /* no seconds */
                        p="\x41\x42\x41\x42:\x43\x41\x42\x41\x42";
                      else if (a_ctrl.precision==1) /* no fraction */
                        p="\x41\x42\x41\x42:\x43\x41\x42\x41\x42:\x43\x41\x42\x41\x42";
                      else
                        p="\x41\x42\x41\x42:\x43\x41\x42\x41\x42:\x43\x41\x42\x41\x42.\x43\x41\x42\x41\x42\x41\x42";
                      break;  /* time */
                  }
                  if (p!=NULL) strcpy(CI->curr_string()+2, p); //no standard mask is bigger than MIN_STRING_LEN-3!
                }
                gen_var_string(CI, CT_MASK);
                break;
#endif
              case Y_SPECIF:
                CI->err_subobj=ERR_SPECIF;
                gen(CI, CT_SPECIF);
                if (a_ctrl.ctClassNum==NO_SLIDER) /* MIN, MAX, FREQ for a slider */
                { gen2(CI, (uns16)sizeof(t_slider_specif));
                  cskip(CI);
                  gen4(CI, num_val(CI, (sig32)0x80000000L, 0x7fffffffL));
                  cskip(CI);
                  gen4(CI, num_val(CI, (sig32)0x80000000L, 0x7fffffffL));
                  cskip(CI);
                  gen4(CI, num_val(CI, (sig32)0x80000000L, 0x7fffffffL));
                }
                else if (a_ctrl.ctClassNum==NO_UPDOWN) /* BUDDY, MIN, MAX, STEP for an updown */
                { BOOL minus;
                  gen2(CI, (uns16)sizeof(t_updown_specif));
                  gen2(CI, (short)num_val(CI, 0, MAX_CTRL_ID));
                  cskip(CI);
                  if (CI->cursym=='-')   { next_sym(CI); minus=TRUE; } else minus=FALSE;
                  if (CI->cursym != S_REAL) c_error(MUST_BE_NUMERIC);
                  gen8(CI, minus ? -CI->curr_real : CI->curr_real);
                  if (next_sym(CI)=='-') { next_sym(CI); minus=TRUE; } else minus=FALSE;
                  if (CI->cursym != S_REAL) c_error(MUST_BE_NUMERIC);
                  gen8(CI, minus ? -CI->curr_real : CI->curr_real);
                  if (next_sym(CI)=='-') next_sym(CI);
                  if (CI->cursym != S_REAL) c_error(MUST_BE_NUMERIC);
                  gen8(CI, CI->curr_real); // minus ignored in step
                  next_sym(CI);
                }
                else if (a_ctrl.ctClassNum==NO_CALEND)
                { gen2(CI, (uns16)sizeof(t_calendar_specif));
                  cskip(CI);  gen(CI, num_val(CI, 0, 1));
                  cskip(CI);  gen(CI, num_val(CI, 0, 1));
                  cskip(CI);  gen(CI, num_val(CI, 0, 1));
                }
                else if (a_ctrl.ctClassNum==NO_EDI || a_ctrl.ctClassNum==NO_EC)
                { gen2(CI, (uns16)sizeof(t_edit_specif));
                  cskip(CI);  gen4(CI, num_val(CI, 0, 0x7fffffff));
                  cskip(CI);  gen4(CI, num_val(CI, 0, 0x7fffffff));
                }
                else if (a_ctrl.ctClassNum==NO_BARCODE)
                { BOOL minus;
                  gen2(CI, (uns16)sizeof(t_barcode_specif));
                              gen4(CI, num_val(CI, 0, 9999999));
                  cskip(CI);  gen4(CI, num_val(CI, 0, 9999999));
                  cskip(CI);  gen4(CI, num_val(CI, -99999, 99999));           // redution
                  cskip(CI);  gen4(CI, num_val(CI, 0, 9999999));
                  cskip(CI);  gen4(CI, num_val(CI, 0, 9999999));
                  cskip(CI);  gen4(CI, num_val(CI, 0, 9999999));
                  cskip(CI);  gen4(CI, num_val(CI, -0x7fffffff, 0x7fffffff)); // prefers
                  cskip(CI);  gen4(CI, num_val(CI, 0, 0x7fffffff));  // commcolor
                  cskip(CI);
                  if (CI->cursym=='-') { next_sym(CI);  minus=TRUE; } else minus=FALSE;
                  if (CI->cursym != S_REAL) c_error(MUST_BE_NUMERIC);
                  gen8(CI, minus ? -CI->curr_real : CI->curr_real);  next_sym(CI);
                  cskip(CI);  gen4(CI, num_val(CI, 0, 9999999));
                  cskip(CI);  gen4(CI, num_val(CI, 0, 9999999));
                  cskip(CI);  gen4(CI, num_val(CI, 0, 9999999));
                  cskip(CI);  gen4(CI, num_val(CI, 0, 9999999));
                  cskip(CI);  gen4(CI, num_val(CI, 0, 9999999));
                }
                else c_error(VIEW_COMP);   /* invalid control type */
                break;
              case Y_NAME:
                CI->err_subobj=ERR_NAME;
                if (CI->cursym!=S_IDENT) c_error(VIEW_COMP);
                gen(CI, CT_NAME);  gen2(CI,0);
                genstr(CI, CI->curr_name);
                next_sym(CI);
                emitted=TRUE;  break;
              case Y_DEFAULT:
                gen(CI,CT_DEFAULT);   gen2(CI,0);   CI->err_subobj=ERR_DEFAULT;
                { int valtype;  code_addr typepos=CI->code_offset;
#ifdef WINS
#if WBVERS<900
                  if (a_ctrl.ctClassNum==NO_ACTX)
                  { skipdelim(CI);
                    if (CI->AXInfo)
                    { CI->AXInfo->Release();
                      CI->AXInfo = NULL;
                    }
                    OLECHAR buff[256];
                    A2W(CI->curr_string(), buff, lstrlen(CI->curr_string())+1);
                    CLSIDFromProgID(buff, (CLSID*)control_info.clsid);
                    if (CreateActiveXInfo((CLSID*)control_info.clsid, &CI->AXInfo))
                    { SET_ERR_MSG(CI->cdp, CI->curr_string());
                      c_error(TYPELIB_ERROR);
                    }
                    AXEventIID = FALSE;
                    skipdelim(CI);
                  }
#endif
#endif
                  if (a_ctrl.ctClassNum==NO_OLE || a_ctrl.ctClassNum==NO_ACTX || a_ctrl.ctClassNum==NO_VIEW)  /* server name */
                  { skipdelim(CI);
                    if (CI->cursym!=S_STRING && CI->cursym!=S_CHAR) c_error(STRING_EXPECTED);
                    if (vwc==VWC_FULL)
                      genstr(CI, CI->curr_string());
                    else
                    { gen(CI, '"');
                      genstr(CI, CI->curr_string());
                      CI->code_offset--;
                      gen(CI, '"');  gen(CI, 0);
                    }
                    next_sym(CI);
                    skipdelim(CI);
                  }
                  else if (vwc==VWC_FULL)
                  { skipdelim(CI);
                    if (a_ctrl.ctClassNum==NO_VIEW) /* subview name */
                      if (CI->cursym==S_STRING || CI->cursym==S_CHAR)
                      { genstr(CI, CI->curr_string());
                        next_sym(CI);
                      }
                      else c_error(STRING_EXPECTED);
                    else                             /* value for other items */
                    { gen(CI,0);  /* space for type */
                      tobj=expression(CI);
                      gen(CI,I_STOP);
                      if (!DIRECT(tobj))
                      { if (!is_string(a_ctrl.type) || !STR_ARR(tobj))
                          c_error(INCOMPATIBLE_TYPES);
                        valtype=ATT_STRING;
                      }
                      else  /* DIRECT */
                      { valtype=DIRTYPE(tobj);
                        if (!same_type(a_ctrl.type, valtype))
                        { if (a_ctrl.type==ATT_FLOAT &&
                              (valtype==ATT_INT16 || valtype==ATT_INT32 || valtype==ATT_INT64 || valtype==ATT_INT8)) goto ok;
                          if (a_ctrl.type==ATT_INT64 &&
                              (valtype==ATT_INT16 || valtype==ATT_INT32 || valtype==ATT_INT8)) goto ok;
                          if (a_ctrl.type==ATT_MONEY &&
                              (valtype==ATT_INT16 || valtype==ATT_INT32 || valtype==ATT_INT8 || valtype==ATT_FLOAT))
                            goto ok;
                          c_error(INCOMPATIBLE_TYPES);
                        }
                       ok:;
                      }
                      code_out(CI, (tptr)&valtype, typepos, 1);
                    }
                    skipdelim(CI);  
                  }
                  else non_comp(CI);
                }
                emitted=TRUE;  break;
              case Y_COLOUR:
                a_ctrl.forecol  =gen_color(CI, vwc, TRUE);   continue;
              case Y_BACKGROUND:
                a_ctrl.ctBackCol=gen_color(CI, vwc, FALSE);  continue;
              case Y_PRECISION:
                CI->err_subobj=ERR_PRECISION;
                a_ctrl.precision=(signed char)num_val(CI, -128, 255);
                continue;
              case Y_CTHELP:
                CI->err_subobj=ERR_HELP;
                a_ctrl.ctHelpInfo=(UINT)num_val(CI, -1, 0x7fffffff);
                continue;
              case Y_PANE:
                CI->err_subobj=ERR_PANE;
                cd=find_tab_id(CI, panetab);
                if (cd<Y_FONT) c_error(VIEW_COMP);
                a_ctrl.pane=(uns8)(cd-Y_FONT+1);
                continue;
#ifdef WINS
#if WBVERS<900
              case Y_PROP:
                CI->err_subobj=ERR_PROP;
                auxpos=CI->code_offset;
                gen_prop(CI, vwc==VWC_FULL);
                emitted = TRUE;
                break;
              case Y_EVENT:
                if (!AXEventIID && vwc==VWC_FULL)
                { gen(CI, CT_EVENTIID);
                  gen2(CI, sizeof(GUID));
                  code_out(CI, (tptr)CI->AXInfo->GetEventIID(), CI->code_offset, sizeof(GUID));
                  CI->code_offset+=sizeof(GUID);
                  AXEventIID = TRUE;
                }
                CI->err_subobj=ERR_EVENT;
                auxpos=CI->code_offset;
                gen_event(CI, vwc==VWC_FULL);
                emitted = TRUE;
                break;
#endif
#endif
              case Y_STYLEX:
                a_ctrl.ctStylEx=num_val(CI, (sig32)0x80000000L, 0x7fffffffL);
                break;
              case Y_FRAMELNWDTH:
                a_ctrl.framelnwdth=(uns32)num_val(CI, 1, 127);
                break;
              default:
                CI->err_subobj=ERR_UNDEF;
                goto controlend;  /* this is not an error, but may come */
            }
            if (emitted)
            { auxsize=CI->code_offset-auxpos-3;
              code_out(CI, (tptr)&auxsize, auxpos+1, 2);
            }
          } // not a VWC_INFO
        } while (TRUE);
       controlend:
        if (vwc==VWC_INFO)
        { control_info.classnum=a_ctrl.ctClassNum;
          control_info.ID       =a_ctrl.ctId;
          if (!control_info.name[0]) { control_info.name[0]=1;  control_info.name[1]=0; }
          // ... empty name is a delimiter, must be replaced by invalid name
          code_out(CI, (tptr)&control_info, CI->code_offset, sizeof(t_control_info));  
          CI->code_offset+=sizeof(t_control_info);
        }
        break;
      case Y_RTEXT:
        a_ctrl.ctStyle=SS_RIGHT;
        a_ctrl.ctClassNum=NO_STR;
        goto std_control;
      case Y_CTEXT:
        a_ctrl.ctStyle=SS_CENTER;
        a_ctrl.ctClassNum=NO_STR;
        goto std_control;

      case Y_CHECKBOX:
        a_ctrl.ctStyle=BS_3STATE;
        a_ctrl.ctClassNum=NO_CHECK;
        goto std_control;
      case Y_RADIOBUTTON:
        a_ctrl.ctStyle=BS_RADIOBUTTON;
        a_ctrl.ctClassNum=NO_RADIO;
        goto std_control;
      case Y_PUSHBUTTON:
        a_ctrl.ctStyle=BS_PUSHBUTTON;
        a_ctrl.ctClassNum=NO_BUTT;
        goto std_control;
      case Y_DEFPUSHBUTTON:
        a_ctrl.ctStyle=BS_DEFPUSHBUTTON;
        a_ctrl.ctClassNum=NO_BUTT;
        goto std_control;
      case Y_GROUPBOX:
        a_ctrl.ctStyle=BS_GROUPBOX;
        a_ctrl.ctClassNum=NO_BUTT;
        goto std_control;

      case Y_LISTBOX:
        a_ctrl.ctStyle=LBS_NOTIFY | WS_VSCROLL;
        a_ctrl.ctClassNum=NO_LIST;
        a_ctrl.key_flags=DEFKF_LIST;
        goto std_control;
      case Y_EDITTEXT:
        a_ctrl.ctStyle=ES_LEFT;
        a_ctrl.ctClassNum=NO_EDI;
        a_ctrl.key_flags=DEFKF_EDIT;
        goto std_control;
      case Y_COMBOBOX:
        a_ctrl.ctClassNum=NO_COMBO;
        a_ctrl.key_flags=DEFKF_LCOMBO;
        goto std_control;
      case Y_OLEITEM:
        a_ctrl.ctClassNum=NO_OLE;
        goto std_control;
      case Y_SUBVIEW:
        a_ctrl.ctClassNum=NO_VIEW;
        goto std_control;
      case Y_GRAPH:
        a_view.vwHdFt |= VIEW_IS_GRAPH;
        a_ctrl.ctClassNum=NO_GRAPH;
        goto std_control;
      case Y_RTF:
        a_ctrl.ctClassNum=NO_RTF;
        a_ctrl.key_flags=DEFKF_RTF;
        goto std_control;
      case Y_SLIDER:
        a_ctrl.ctClassNum=NO_SLIDER;
        a_ctrl.key_flags=DEFKF_SLIDER;
        goto std_control;
      case Y_UPDOWN:
        a_ctrl.ctStyle=WS_BORDER;
        a_ctrl.ctClassNum=NO_UPDOWN;
        goto std_control;
      case Y_PROGRESS:
        a_ctrl.ctClassNum=NO_PROGRESS;
        goto std_control;
      case Y_SIGNATURE:
        a_ctrl.ctClassNum=NO_SIGNATURE;
        goto std_control;
      case Y_TABCONTROL:
        a_ctrl.ctClassNum=NO_TAB;
        goto std_control;
      case Y_ACTIVEX:
        a_ctrl.ctClassNum=NO_ACTX;
        if (vwc==VWC_INFO)
          memset(control_info.clsid, 0, sizeof(control_info.clsid));
        goto std_control;
      case Y_ICON:
        a_ctrl.ctStyle=SS_ICON;
        a_ctrl.ctClassNum=NO_STR;
        goto std_control;
      case Y_CALENDAR:
        a_ctrl.ctClassNum=NO_CALEND;
        a_ctrl.key_flags=DEFKF_CALEND;
        goto std_control;
      case Y_DTP:
        a_ctrl.ctClassNum=NO_DTP;
        a_ctrl.key_flags=DEFKF_CALEND;
        goto std_control;
      case Y_BARCODE:
        a_ctrl.ctClassNum=NO_BARCODE;
        goto std_control;

      default: c_error(ITEM_EXPECTED);
    }
   /* check and repair some non-consistencies, then output: */
    if (a_ctrl.type==ATT_DATE)
    { if (a_ctrl.precision < 0 || a_ctrl.precision >= EXT_DATE_FORMATS)
        a_ctrl.precision = EXT_DATE_FORMATS-1;
      if (a_ctrl.ctClassNum==NO_EDI && a_ctrl.precision >= BAS_DATE_FORMATS)
        a_ctrl.precision = BAS_DATE_FORMATS-1;
    }
    if (a_ctrl.type==ATT_TIME)
    { if (a_ctrl.precision < 0 || a_ctrl.precision >= EXT_TIME_FORMATS)
        a_ctrl.precision = EXT_TIME_FORMATS-1;
      if (a_ctrl.ctClassNum==NO_EDI && a_ctrl.precision >= BAS_TIME_FORMATS)
        a_ctrl.precision = BAS_TIME_FORMATS-1;
    }
    if (key_flags_specified) a_ctrl.key_flags=key_flags;
    if (vwc == VWC_FULL)
    {
      a_ctrl.ctBackCol=final_color(a_ctrl.ctBackCol);
      a_ctrl.forecol  =final_color(a_ctrl.forecol);
    }
    a_ctrl.ctCodeSize=(uns16)(CI->code_offset-ctrl_offset-sizeof(t_ctrl));
    if (vwc!=VWC_INFO) code_out(CI, (tptr)&a_ctrl, ctrl_offset, sizeof(t_ctrl));
    a_view.vwItemCount++;
  }
  if (a_view.vwHdFt & FORM_OPERATION)
  { a_view.vwHdFt  &= ~(LONG)(DEACT_SYNCHRO | NO_VIEW_FIC);
    a_view.vwHdFt  |= STD_SYSMENU | REC_SYNCHRO;
    a_view.vwStyle |= NO_VIEW_MOVE;
    a_view.vwStyle &= ~(LONG)REP_VW_STYLE_MASK;
  }
  if (!(a_view.vwHdFt & REC_SYNCHRO))   /* item synchronization */
    a_view.vwHdFt  &= ~(LONG)(DEACT_SYNCHRO | ASK_SYNCHRO);
  if (vwc == VWC_FULL)
    a_view.vwBackCol=final_color(a_view.vwBackCol);
 /* updating vwItemCount and other changes in view header */
  if (vwc!=VWC_INFO) code_out(CI, (tptr)&a_view, 0, sizeof(view_stat));
  else  // add terminator
  { control_info.name[0]=0;  control_info.classnum=0;  control_info.type=0;
    code_out(CI, (tptr)&control_info, CI->code_offset, sizeof(t_control_info));  
    CI->code_offset+=sizeof(t_control_info);
  }
  CI->err_object=ERR_VIEW_HEADER;  /* in order not to oen any dialog if compilation OK but test view not opened */
}

CFNC DllKernel void WINAPI view_struct_comp(CIp_t CI)
{ CI->comp_flags|=COMP_FLAG_NO_COLOR_TRANSLATE;
  view_comp(CI, VWC_STRUCT); 
}
CFNC DllKernel void WINAPI view_def_comp(CIp_t CI)
{ view_comp(CI, VWC_FULL); }
CFNC DllKernel void WINAPI view_info_comp(CIp_t CI)
{ view_comp(CI, VWC_INFO); }
CFNC DllKernel void WINAPI view_access(CIp_t CI)
{ uns8 type, access;  t_specif specif;
  CI->univ_ptr=selref(CI, &type, &specif, &access);
  // V.B. 9.12.98 tady by se mel testovat konec vyrazu
  if (CI->cursym!=S_SOURCE_END)
    CI->univ_ptr=0;
}
CFNC DllKernel void WINAPI view_value(CIp_t CI)
{ uns8 type;  typeobj * tobj;  t_specif specif;  int eflag;
  CI->univ_ptr=tobj=selval(CI, &type, &specif, &eflag);
  CI->err_object = eflag;
  if (tobj->type.anyt.typecateg==T_RECCUR)
    gen(CI, I_LOADOBJNUM); // use by eval window in the debugger
}
//
// Priznak dynamicke barvy a statickou barvu necha beze zmeny,
// systemovou barvu zkonveruje na aktualni statickou
//
CFNC DllKernel LONG WINAPI final_color(LONG color)
{ if (!(color & 0x7f000000) || color == DYNAMIC_COLOR) return color;
#ifdef WINS
  return GetSysColor(SYSCOLORPART(color));
#else
  return 0;
#endif
}
//////////////////////////  replication relation //////////////////////////
#include "replic.h"

CFNC DllKernel void WINAPI replrel_comp(CIp_t CI)
{ t_replrel * rel = (t_replrel *)CI->univ_ptr;
  rel->konflres=num_val(CI, 0, 3);
  for (int direc=0;  direc<2;  direc++)
  { t_relatio * rr = &rel->r[direc];
    rr->hours  =num_val(CI, 0, 1000);
    rr->minutes=num_val(CI, 0, 60);
    while (CI->cursym=='*')
    { next_and_test(CI, S_IDENT, IDENT_EXPECTED);
      t_reltab * rt = rr->get_reltab(CI->curr_name);
      next_sym(CI);
      if (CI->cursym!=S_STRING && CI->cursym!=S_CHAR) c_error(STRING_EXPECTED);
      strmaxcpy(rt->condition, CI->curr_string(), MAX_REPLCOND+1);
      next_sym(CI);
      rt->can_del=num_val(CI, 0, 2);
      while (CI->cursym==S_IDENT)
      { t_attrname * atr = rt->atrlist.next();
        if (atr==NULL) c_error(OUT_OF_MEMORY);
        strmaxcpy(*atr, CI->curr_name, ATTRNAMELEN+1);
        next_sym(CI);
      }
    }
  }
}

#endif /* !SRVR */
