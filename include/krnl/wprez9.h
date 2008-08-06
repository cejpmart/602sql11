/****************************************************************************/
/* wprez9.h - presentation layer headers & definitions                       */
/****************************************************************************/

#ifdef _MSC_VER
#pragma warning(disable : 4001)
#endif

#if WBVERS<=810
#ifdef WINS
#include "wbcapi.h"
#else
typedef void WWWChildDrawingData;
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SRVR

DllPrezen extern HINSTANCE hPrezInst;        /* 602prez8.DLL instance handle */

/* Priznaky "flags" pro Open_view, Select_records, Relate_records, Set_fcursor a Bind_records */
#define NO_EDIT 0x01 /* zakaz editace v pohledu */
#define NO_INSERT 0x02 /* zakaz vkladani zaznamu */
#define NO_MOVE 0x04 /* zakaz prechodu mezi zaznamy */
#define NO_DELETE 0x08 /* zakaz ruseni zaznamu */
#define DEL_RECS 0x10 /* zobrazovat i zrusene zaznamy */
#define AUTO_CURSOR 0x40 /* uzavrit cursor pri zavirani pohledu */
#define COUNT_RECS 0x80 /* spocitat zaznamy pri otevreni pohledu */
#define MODAL_VIEW 0x100 /* otevrit pohled jako modalni popup okno */
#define NO_CHILD_FRAME 0x1000 /* okno subpohledu nebude mit ram */
#define CHILD_VIEW 0x2000 /* otevrit pohled jako child nebo subpohled */
#define QUERY_VIEW 0x10000L /* otevrit pohled pro zadavani dotazu */
#define PARENT_CURSOR 0x80000L /* pouzit kurzor a cache z parent-pohledu */
#define MODELESS_VIEW 0x100000 /* otevrit pohled jako popup okno */

/* typ operace pro funkci Edit_privils */
#define MULTIREC 1
#define MULTITAB 2

/* global view styles */
#define REP_VW_STYLE_MASK 3        /* records per page/window */
#define REP_VW_STYLE_0    1        /* records not separated */
#define REP_VW_STYLE_1    2        /* thin record separating line */
#define REP_VW_STYLE_2    3        /* thick record separating line */

#define QUERY_DEL         0x0008
#define REP_PGBEF         0x0010
#define REP_PGAFT         0x0020
#define SUPPRESS_BACK     0x0040
#define VW_POPUP_HELP     0x0080
#define GP_BASE           0x0100   /* group paging */
#define GP_MASK           0x1f00
#define SIMPLE_VIEW       0x8000

/* HdFt style (0xff bits reserved for report header & footer pos) */
#define MAX_DISABLED      0x8000
#define MIN_DISABLED      0x4000
#define STD_SYSMENU       0x2000
#define VIEW_3D           0x1000
#define CLOSE_DISABLED    0x0800
#define VIEW_IS_GRAPH     0x0400
#define FORM_OPERATION    0x0200   /* works as an input form */
#define REP_FLOATINGPGFT  0x0100   /* floating page footer */
#define REP_HDMASK        0x000f
#define REP_HDBEFPG       0x0000
#define REP_HDAFTPG       0x0001
#define REP_HDNOPG        0x0002
#define REP_FTMASK        0x00f0
#define REP_FTBEFPG       0x0000
#define REP_FTAFTPG       0x0010
#define REP_FTNOPG        0x0020
#define VW_FIXED_SIZE     0x00010000L
#define REC_SYNCHRO       0x00020000L
#define DEACT_SYNCHRO     0x00040000L
#define ASK_SYNCHRO       0x00080000L
#define NO_VIEW_INS       0x00100000L
#define NO_VIEW_FIC       0x00200000L
#define NO_VERT_SCROLL    0x00400000L
#define NO_FAST_SORT      0x00800000L
#define PAGE_COUNT_REQ    0x01000000L

// vwStyle3 constants:
#define VIEW_IS_REPORT    0x00000001
#define REC_SELECT_FORM   0x00000002
#define QBE_DISABLED      0x00000004
#define DISABLE_AUTOSYNC  0x00000008
#define MODELESS_VIEW3    0x00000010
#define DEBUGGER_AUX_FORM 0x00000020  // this form can be closed even on debug break
#define ESC_CLOSES        0x00000040
#define NO_TOOLBAR        0x00000080
#define SYSTEM_FORM       0x00000100  // must not be closed when the program terminates
#define MULTI_SELECT      0x00000200

/* view control styles */
#define NO_REFORMAT       0x00000100
#define FLEXIBLE_SIZE     0x00000200    
#define DISABLE_INACTIVE  0x00000200    // for edit controls, same value as FLEXIBLE_SIZE
#define INDEXABLE         0x00000400
#define IT_INFLUEN        0x00000800
#define CAN_OVERFLOW      0x00001000
#define FRAMED            0x00002000    /* in old views only, replaced by WS_BORDER */
#define SHADED            0x00004000
#define INHERIT_CURS      0x00008000    /* otherwise problems with &= ~ */
#define SUBVW_RELATIONAL  0x00000100    // subview is synchronizaed relationally (used in view designer, ignored in wbprezen for backward compatibility)
#define SUBVW_RELNAME     0x00001000    // subview is synchronizaed relationally with relation name
#define SUBVW_USERACTION  0x00000400    // subpohled je synchronizov� uivatelskou akc�#define SUBVW_STYLEMASK   (SUBVW_RELATIONAL | SUBVW_RELNAME | SUBVW_USERACTION)
#define QUERY_STATIC      0x01000000
#define QUERY_DYNAMIC     0x02000000
#define CT_POPUP_HELP     0x20000000
#define STYLE_MASK        0xdcff00ff

/* Automaticky generovane zpravy */
#define NOTIF_CREATE 1 /* pohled byl otevren */
#define NOTIF_DESTROY 2 /* pohled byl uzavren */
#define NOTIF_RECENTER 3 /* vybran novy zaznam v pohledu */
#define NOTIF_CHANGE 4 /* zmenena slozka pohledu */
#define NOTIF_RESET_RECORD 5 /* vybrany zaznam se prekresluje */
#define NOTIF_RESET_VIEW 6 /* cely pohled se prekresluje */
#define NOTIF_SUBCURSOR 7 /* pohled prechazi k subkurzoru QBE */
#define NOTIF_SUPERCURSOR 8 /* pohled se vraci k superkurzoru */
#define NOTIF_CREATE_DEPENDENT 9 /* otevren zavisly pohled */
#define NOTIF_DESTROY_DEPENDENT 10 /* uzavren zavisly pohled */
#define NOTIF_CREATE_EDITOR 11 /* otevren textovy editor */
#define NOTIF_DESTROY_EDITOR 12 /* uzavren textovy editor */
#define NOTIF_CREATE_VIEWER 13 /* otevreno okno s obrazkem */
#define NOTIF_DESTROY_VIEWER 14 /* uzavreno okno s obrazkem */

#define SS_TEXT           0x0020
#define SS_RASTER         0x0021

#define EX_FLEX_X         0x00000001
#define EX_FLEX_Y         0x00000002
#define EX_FLEX_CX        0x00000004
#define EX_FLEX_CY        0x00000008

#define SIGNATURE_SIGN    1  /* signature control precision */
#define SIGNATURE_CHECK   2
#define SIGNATURE_CLEAR   4

#define TBB_CUT     0
#define TBB_COPY    1
#define TBB_PASTE   2
#define TBB_PREV    3
#define TBB_NEXT    4
#define TBB_PREVP   5
#define TBB_NEXTP   6
#define TBB_FIRST   7
#define TBB_LAST    8
#define TBB_QBE     9
#define TBB_ORDER   10
#define TBB_ACCEPT  11
#define TBB_DELQ    12
#define TBB_DELREC  13
#define TBB_DELALL  14
#define TBB_DELUNB  15
#define TBB_PRINT   16
#define TBB_INDEX   17
#define TBB_LOCK    18
#define TBB_HELP    19
#define TBB_EXPIMP  20
#define TBB_FIND    21
#define TBB_REFIND  22
#define TBB_REPLACE 23
#define TBB_COMPILE 24
#define TBB_RUN     25
#define TBB_SAVE    26
#define TBB_FORMAT  27
#define TBB_REFRESHR 28
#define TBB_TEST    29
#define TBB_SGRID_PX 30
#define TBB_ITINS   31
#define TBB_RUNOBJ  32
#define TBB_ITDEL   33
#define TBB_TEST_CU 34
#define TBB_TEST_ME 35
#define TBB_VECLOSE 36
#define TBB_KEDTEST 37
#define TBB_KEDSQL  38
#define TBB_KEDADD  39
#define TBB_KEDDEL  40
#define TBB_STEP    41
#define TBB_TRACE   42
#define TBB_TORET   43
#define TBB_EVAL    44
#define TBB_WATCH   45
#define TBB_DEBUG   46
#define TBB_KEDGRP  47
#define TBB_KEDHAV  48
#define TBB_FICTIVE 49

#define TBB_RELAT   53
#define TBB_INSCOL  54
#define TBB_PR_COMP 55
#define TBB_PR_RUN  56

#define TBB_PRINTRP 58
#define TBB_GOCURS  59
#define TBB_BREAK   60
#define TBB_SERVERS 61
#define TBB_FLDUPCAT 62
#define TBB_EXTBUTS 63
#define TBB_WAITING 64
#define TBB_ACCEPTCH 65
#define TBB_UNDOCH  66
#define TBB_PRIVILS 67
#define TBB_TOKEN   68
#define TBB_SBINS   69
#define TBB_SPINS   70
#define TBB_UNDO    71
#define TBB_REDO    72
#define TBB_SGRID_CM 73
#define TBB_CALLSTK 74
#define TBB_WATCHES 75
#define TBB_REFRESH 76
#define TBB_DRWLINES 77

#define TBB_UPFOLDER 79
#define TBB_DELSEL   80

#define TBB_TBD_FLGS 84
#define TBB_MON_SRV  85
#define TBB_CONSOLE  86
#define TBB_RELIST   87
#define TBB_MON_CLI  88

#define TOPBUTTONS_X      23
#define TOPBUTTONS_Y      23
#define TBB_STEP_0        22
#define TBB_STEP_1        28
#define TBB_STEP_2        42

#define TBB_ZM_PWIDTH  98
#define TBB_ZM_PWHOLE  99
#define TBC_ZOOM      100

#define SBMP_CX           18 /* top button standard bitmaps dims. & location */
#define SBMP_CY           18
#define PER_LINE          25
#define SBMP_X_OFF(bitmap_index) (18*(bitmap_index % PER_LINE))
#define SBMP_Y_OFF(bitmap_index) (18*(bitmap_index / PER_LINE))

extern HBITMAP st_unsynch, st_unsynch2, st_selected, st_ascorder, st_descorder;
extern HMENU form_menu;

#define ENUMPOPUP         0x7100    /* value of enum   menu items ID -> */
/*#define RA_FIRST        0x7200     value of raster menu items ID -> */

#define PANE_PAGESTART    0x02      /* view pane numbers */
#define PANE_PAGEEND      0x0e
#define PANE_REPORTSTART  0x01
#define PANE_REPORTEND    0x0f
#define PANE_DATA         0x08
#define PANE_GROUPSTART   0x03
#define PANE_GROUPEND     0x09
#define PANE_FIRST PANE_REPORTSTART  
#define PANE_LAST  PANE_REPORTEND    

#define MAINKOEF    96
#define MAINKOEF10  960
#define MAINKOEF100 9600

#define X_FRAME_LIMIT  1
#define Y_FRAME_LIMIT  1

#define OLE_LINKED    1
#define OLE_EMBEDDED  2

#define PREZ_FLEXIBLE  0
#define PREZ_ORIGSIZE  4
#define PREZ_PROPORC   8
#define PREZ_ICON      0xc
#define PREZ_SUMMARY   0x10
#define PREZ_MASK      0x1c

#define PICT_ORIGSIZE   0
#define PICT_FLEXIBLE   1
#define PICT_PROPORC    2

#define DEF_XRECOFF   20
/////////////////////////////// toolbar ////////////////////////////////////////
#define TBFLAG_COMMAND    1
#define TBFLAG_PRIVATE    2
#define TBFLAG_STOP       4
#define TBFLAG_RIGHT      8
#define TBFLAG_FRAMEMSG  16
#define TBFLAG_DISABLED  32
#define TBFLAG_LOADED    64
#define TBFLAG_DOWN     128  
#define TBFLAG_SWITCH   256
#define TBFLAG_FRAMECOMM (TBFLAG_FRAMEMSG | TBFLAG_COMMAND)

#pragma pack(1)
typedef struct
{ WORD total_size; /* offset of the next descriptor */
  WORD flags;
  LONG bitmap;     /* used for bitmap handle for private bitmaps, too */
  WORD msg;        /* 0 in new tool bars! */
  WORD offset;     /* offset code in designer, real offset in a view */
  char txt[1];     /* status text followed by statements (source or compiled) */
} t_tb_descr;
#pragma pack()
#define NEXT_TB_BUTTON(tbb) ((t_tb_descr*)((tptr)(tbb)+(tbb)->total_size))

#define NO_CHILD_TOOL_BAR ((t_tb_descr*)1)

CFNC DllPrezen unsigned WINAPI calc_tb_size(const t_tb_descr * tb);
CFNC DllPrezen void     WINAPI load_toolbar_bitmaps(t_tb_descr * tb); // bitmaps are loaded only in the t_designed_toolbar object, never in the toolbar pattern in VW_TOOLBAR
CFNC DllPrezen void     WINAPI release_toolbar_bitmaps(const t_tb_descr * tb);

// toolbar encapsulation, shareable by the forms
class t_designed_toolbar
{ unsigned ref_cnt;                  // reference counter, number of forms using this toolbar
  t_tb_descr tb[EMPTY_ARRAY_INDEX];  // toolbar description
 public:
  unsigned AddRef(void)  
    { return ++ref_cnt; }
  unsigned Release(void) 
    { if (!--ref_cnt) { delete this;  return 0; } return ref_cnt; }
  void * operator new(size_t base_size, t_tb_descr * tb_pattern) 
    { return corealloc(base_size+calc_tb_size(tb_pattern), 82); }
  void operator delete(void * ptr)
    { corefree(ptr); }
#ifdef WINS // prevents compiler warning
  void operator delete(void * ptr, t_tb_descr * tb_pattern)
    { corefree(ptr); }
#endif
  t_designed_toolbar(t_tb_descr * tb_pattern)
    { ref_cnt=1;
      memcpy(tb, tb_pattern, calc_tb_size(tb_pattern));  // tb_size defined by the new operator
      load_toolbar_bitmaps(tb);
    }
  ~t_designed_toolbar(void)
    { release_toolbar_bitmaps(tb); }
  t_tb_descr * tb_descr(void) 
    { return tb; }
};

#include "wbprezex.h"

/******************************** message boxes *****************************/
class wxString;
void stat_error(wxString msg);
bool stat_signalize(cdp_t cdp);
DllPrezen void WINAPI  syserr(int operat);
DllPrezen void WINAPI  view_err_box(int object, int subobj, int errnum, HWND hParent, const char *name);
DllPrezen void WINAPI  menu_err_box(int subobj, int errnum, HWND hParent);
int find_help_topic_for_error(int err);

 /* modeless dialogs: */
DllPrezen void WINAPI Set_active_dialog(HWND hDlg);
DllPrezen BOOL WINAPI Is_dialog_message(MSG * msg);

//////////////////////// binding records ///////////////////////////////////
typedef enum { PTR_BIND, SELECT_BIND, RELATE_BIND, RELMN_BIND } bind_type;
#define REL_MAXNUM 2000

struct bind_def
{ bind_type btype;
  BOOL      multibind;      /* can bind multiple records */
 private:
  BOOL      prealloc;       /* do not deallocate the binding_list at close */
 public:
  unsigned  binding_limit;  /* max. number of bound records */
  trecnum * binding_list;   /* list of records, 1st item == count */
  BOOL      cached;         /* used by PTR_BIND only */
  tcursnum  cursnum;        /* NOOBJECT for Select_records */
  trecnum   position;       /* undef. for Select_records */
  tattrib   attr[MAX_REL_ATTRS];  /* undef. for Select_records */
  uns16     index;          /* undef. for Select_records */
  view_dyn * par_inst;          /* used in Relate_record (M:N) and PTR_BIND */
  bind_def(int b_limit, bind_type btypeIn, BOOL multibindIn)
  { btype=btypeIn;
    multibind=multibindIn;
    if (b_limit) // will be defined otherwise, if zero
    { binding_list = (trecnum*)sigalloc((b_limit+1) * sizeof(trecnum), OW_PTRS);
      binding_limit=b_limit;
      if (binding_list) *binding_list=0;
      prealloc=FALSE;
    }
    else prealloc=TRUE;
    cursnum      = NOOBJECT;  /* Bind_records will overwrite this */
    cached       = FALSE;
    par_inst     = NULL;
  }

  virtual ~bind_def(void)
    { if (!prealloc) corefree(binding_list); }
  virtual BOOL init_relational_view(view_dyn * inst) 
    { return TRUE; }
  BOOL is_bound(view_dyn * inst, trecnum rec);
};

struct rel_bind_def : public bind_def
{ tattrib  chattr[MAX_REL_ATTRS];
  uns8       type[MAX_REL_ATTRS];
  t_specif specif[MAX_REL_ATTRS];
  rel_bind_def(int b_limit, bind_type btypeIn, BOOL multibindIn) :
       bind_def(b_limit, btypeIn, multibindIn)
    { }
  virtual BOOL init_relational_view(view_dyn * inst);
};

struct rel1_bind_def : public rel_bind_def
{ char      remote_attr[ATTRNAMELEN+1];  /* specific to 1:N rel */
  tobjnum   relobj;
  rel1_bind_def(void) : rel_bind_def(1, RELATE_BIND, FALSE)
    { relobj=NOOBJECT; }
  virtual BOOL init_relational_view(view_dyn * inst);
  BOOL bind_action(view_dyn * binst, trecnum rec);
};

struct relmn_bind_def : public rel_bind_def
{ tobjnum   rel1obj, rel2obj;
  tobjname  linktabname;
  uns8       type3[MAX_REL_ATTRS];  
  t_specif specif3[MAX_REL_ATTRS];
  char      attr21name[MAX_REL_ATTRS][ATTRNAMELEN+1], attr23name[MAX_REL_ATTRS][ATTRNAMELEN+1];
  relmn_bind_def(void) : rel_bind_def(REL_MAXNUM, RELMN_BIND, TRUE)
    { }
  virtual BOOL init_relational_view(view_dyn * inst);
  BOOL bind_action_bind(view_dyn * binst, trecnum viewext);
  BOOL bind_action_unbind(view_dyn * binst, trecnum viewext);
};

#define BINDID   0  /* not ENTER nor ESC id */
BOOL bind_action(view_dyn * binst, trecnum rec, HWND hWnd);
BOOL del_unbind_action(view_dyn * binst, HWND hWnd, BOOL all);
///////////////////////////////////////////////////////////////////////////////////
typedef struct         /* specials for history window */
{ unsigned itemsize; /* size of the full history item (incl. author, datim) */
  unsigned valsize;  /* size of the value in history */
  tcursnum cursnum;
  trecnum recnum; /* record number */
  tattrib attrnum;/* attribute of the history type */
  uns8  descr;    /* description of the contents */
} t_history;

typedef struct         /* specials for the system index view */
{ trecnum recnum;
  UINT ID;
} t_sysindex;

typedef union
{ t_history history;
  t_sysindex si;
} t_aa;

/***************************** view item types *****************************/
#define NO_GRAPH    100
#define NO_VAL      0
#define NO_STR      1
#define NO_EDI      2
#define NO_BUTT     3
#define NO_COMBO    4
#define NO_LIST     5
#define NO_CHECK    6
#define NO_RADIO    7
#define NO_TEXT     8
#define NO_RASTER   9
#define NO_FRAME    10
#define NO_OLE      11
#define NO_VIEW     12
#define NO_EC       13
#define NO_FIRST_32    14
#define NO_RTF       14
#define NO_SLIDER    15
#define NO_UPDOWN    16
#define NO_PROGRESS  17
#define NO_SIGNATURE 18
#define NO_TAB       19
#define NO_ACTX      20
#define NO_CALEND    21
#define NO_DTP       22
#define NO_BARCODE   23
#define NO_CLASS_COUNT 24

#define IS_FRAME(itm)      ((itm)->ctClassNum==NO_FRAME)
#define IS_ANY_BUTTON(itm) ((itm)->ctClassNum==NO_BUTT || (itm)->ctClassNum==NO_CHECK || (itm)->ctClassNum==NO_RADIO)
#define IS_ANY_COMBO(itm)  ((itm)->ctClassNum==NO_COMBO || (itm)->ctClassNum==NO_EC)

#define KF_LEFTRIGHT 1
#define KF_UPDOWN    2
#define KF_HOMEEND   4
#define KF_PGUPDN    8
#define KF_CHOMEEND  16
#define KF_CPGUPDN   32
#define KF_TAB       64
#define KF_DEFPUSH   128            // neni nikde pouzito

#define DEFKF_STD       0
#define DEFKF_EDIT       KF_LEFTRIGHT
#define DEFKF_MULEDIT   (KF_LEFTRIGHT | KF_UPDOWN | KF_HOMEEND | KF_PGUPDN)
#define DEFKF_RTF       (KF_LEFTRIGHT | KF_UPDOWN | KF_HOMEEND | KF_PGUPDN | KF_CHOMEEND | KF_CPGUPDN)
#define DEFKF_SLIDER    (KF_LEFTRIGHT | KF_UPDOWN | KF_PGUPDN)
#define DEFKF_LIST       KF_UPDOWN
#define DEFKF_LCOMBO    0
#define DEFKF_CALEND    (KF_LEFTRIGHT | KF_UPDOWN | KF_HOMEEND | KF_PGUPDN)
#define NEWKF_EDIT      (KF_LEFTRIGHT | KF_HOMEEND)
#define NEWKF_MULEDIT   (KF_LEFTRIGHT | KF_UPDOWN | KF_HOMEEND | KF_PGUPDN | KF_CHOMEEND | KF_CPGUPDN)

enum t_access { ACCESS_NO, ACCESS_PROJECT, ACCESS_CACHE, ACCESS_DB };
#define MINUS_LIMIT (-10)

struct t_ctrl_base
{ int  ctX;
  int  ctY;
  int  ctCX;
  int  ctCY;
  int  ctId;
  uns32 ctStyle;  /* expandable */
  uns32 ctStylEx;
  uns8  pane;
  uns8  type;     /* defined only for DB control classes */
  uns8  access;
  uns8  framelnwdth;
  t_specif specif;
  uns32 forecol;
  uns32 ctBackCol;
  sig8  precision;
  uns8  ctClassNum;     /* 0 if user defined */
  uns32 ctHelpInfo;     /* 0 if not defined */
  int   column_number;  // added in 9.0 for ODBC
  uns16 key_flags;
  uns16 ctCodeSize;
  t_ctrl_base(void)
  { ctX=ctY=ctCX=ctCY=0;
    ctId=-1;
    ctStyle=ctStylEx=0;
    pane=0;  type=0;  access=0;  specif.set_null();  forecol=0xff;  ctBackCol=0;  precision=0;
    ctClassNum=NO_VAL;  ctHelpInfo=0; key_flags=0;  ctCodeSize=0; framelnwdth=1;
  }
};

struct t_ctrl : public t_ctrl_base
{ char ctCode[EMPTY_ARRAY];  /* key definitions, conditions, actions, ...  */
};

#define MAX_CTRL_ID        0x20003   /* duplic array size in comp.h in bits */

#define ALL_PANES    16

typedef struct
{ uns32 vwStyle;      /* repeating, frame */
  int   vwItemCount;
  int   vwX;
  int   vwY;
  int   vwCX;
  int   vwCY;
  uns32 vwBackCol;
  uns32 vwHelpInfo;     /* user help number, 0 if not defined */
  uns32 vwHdFt;        /* various flags */
  uns32 vwStyle3;      // additional flags
 // pane sizes:
  sig16 panesz[ALL_PANES];
 /* label info */
  BOOL  vwIsLabel;
  uns16 vwXX;           /* label width */
  uns16 vwYY;           /* label height */
  uns16 vwXspace;       /* label parameters */
  uns16 vwYspace;       /* space is in hunderths of mm */
  uns8  vwXcnt;
  uns8  vwYcnt;
  uns16 vwXmarg;
  uns16 vwYmarg;
  uns16 vwCodeSize;
  char vwCode[EMPTY_ARRAY];      /* key & pane definitions only */
} view_stat;

#define FIRST_CONTROL(vwst) ((t_ctrl *)(vwst->vwCode + vwst->vwCodeSize))
#define NEXT_CONTROL(item)  ((t_ctrl *)(item->ctCode + item->ctCodeSize))
#define MAX_SVIEW_COLUMNS  255

typedef struct
{ HFONT  hFont;    /* font  handle for the item */
  HBRUSH hBrush;   /* brush handle for the item */
  union
  { t_ctrl * defitem; /* defining item ptr */
    uns32    itemind; /* index of an item with the same font & brush */
  } def;
  int    prev_info;   /* linked list of items with HIWORD(itemind), terminated by -1 */
} fontbrush;
/* Axioms: if HIWORD(itemind) then hBrush exists, hFont is a font or 0.
   if !HIWORD(itemind) then HIWORD(fontbrush[LOWORD(itemind)].itemind) */

typedef uns16 RPART;
#define BITSPERPART 16
#define BMAP_ALLOC_STEP  4096  /* in bytes */
#define SET_FI_INT_RECNUM(inst) \
 inst->dt->fi_recnum=(inst->stat->vwHdFt & NO_VIEW_FIC) ? inst->dt->rm_int_recnum :\
                  inst->dt->rm_int_recnum+1
#define SET_FI_EXT_RECNUM(inst) \
 inst->dt->fi_recnum=(inst->stat->vwHdFt & NO_VIEW_FIC) ? inst->dt->rm_ext_recnum :\
                  inst->dt->rm_ext_recnum+1
/* When no remap is used, remap_a must be NULL,
   rm_ext_recnum & rm_completed are in normal use,
   fi_recnum depends on rm_ext_recnum */
/* Pairs <0, 0>, <rm_curr_int, rm_curr_ext> and <rm_int_recnum, rm_ext_recnum>
   have the following sematics:
   "<A, B> iff A is the number of 1's in remap_a BELOW remap_a[B]"
   If rm_completed, rm_ext_recnum is the first external record number which
   is not in the map (may or may not exist).
   If !rm_completed, rm_ext_recnum is the total number of external records.
   External record rm_curr_ext exists and has rm_curr_int internal number
   unless both are 0.
*/

typedef class  CWBOleClDoc * LPWBOLECLDOC;

class CPrintEnv;
struct sview_dyn;

typedef enum
{ VWMODE_MDI,      // default
  VWMODE_SUBVIEW,  // IS_SUBVIEW specified, parent is a view
  VWMODE_OLECTRL,  // VIEW_OLECTRL specified or parent is InPlace
  VWMODE_CHILD,    // IS_SUBVIEW specified, parent is not a view
  VWMODE_PRINT,    // PRINT_VIEW specified
  VWMODE_MODELESS, // MODELESS_VIEW specified
  VWMODE_MODAL     // MODAL_VIEW specified or taken from view definition
} t_vwmode;

#define INFO_FLAG_RECPRIVS 1
#define INFO_FLAG_TOKENS   2
#define INFO_FLAG_NONUPDAT 4

typedef struct add_cond
{ struct add_cond * next_cond;
  trecnum rec;
  tptr    item_text;  // used in query-views (QBE & user-designed)
  UINT    item_id;    // used only by query_fields, 0 otherwise
  tptr    cond_text;
  BOOL    cond_active;
} add_cond;

typedef struct t_order
{ tptr item_text;
  BOOL desc;
} t_order;

typedef struct t_qbe
{
  unsigned   m_cRef;
  add_cond * add_conds;           /* additional conditions */
  struct view_dyn * query_field_inst;
  BOOL       order_active;
  t_order    add_order[MAX_INDEX_PARTS];/* additional ordering */

  t_qbe(void)
  { m_cRef=1;
    add_conds=NULL;
    memset(add_order, 0, sizeof(add_order));
    query_field_inst=NULL;
  }

  ~t_qbe(void)
  { add_cond * acnd = add_conds;
    while (acnd!=NULL)
    { add_cond * bcnd=acnd->next_cond;
      corefree(acnd->cond_text);  corefree(acnd->item_text);
      corefree(acnd);
      acnd=bcnd;
    }
    for (int i=0;  i<MAX_INDEX_PARTS;  i++)
      corefree(add_order[i].item_text);
  }

  unsigned AddRef(void) { return ++m_cRef; }
  unsigned Release(void){ if (--m_cRef) return m_cRef; delete this; return 0; }
} t_qbe;
////////////////////////////// combo filling ////////////////////////////
struct enum_cont
{ UINT ctId;
  wchar_t * defin;
  enum_cont * next;

  DllPrezen WINAPI enum_cont(struct view_dyn * inst, UINT ctIdIn);
  ~enum_cont(void)
  { if (next) delete next;
    corefree(defin);
  }
  DllPrezen void WINAPI enum_reset(view_dyn * inst, BOOL initial, tptr query);
  //DllPrezen void WINAPI enum_assign(tptr newdef);
};
/********************* fonts & brushes **********************/
class t_fontdef
{public:
  uns16 size;
  uns8  flags;
  uns8  charset;
  uns8  pitchfam;
  char  name[LF_FACESIZE];

  t_fontdef(void) { }
  t_fontdef(const t_fontdef &other)
  { size = other.size;
    flags = other.flags;
    charset = other.charset;
    pitchfam = other.pitchfam;
    strcpy(name, other.name);
  }
  void operator =(const t_fontdef &other)
  { size = other.size;
    flags = other.flags;
    charset = other.charset;
    pitchfam = other.pitchfam;
    strcpy(name, other.name);
  }
  BOOL same(t_fontdef * f)
  { return size==f->size && flags==f->flags && charset==f->charset &&
              pitchfam==f->pitchfam && !stricmp(name, f->name);
  }
  size_t Size(void) { return sizeof(t_fontdef) - LF_FACESIZE + strlen(name) + 1; }
};

CFNC DllPrezen void WINAPI logfont2fontdef(LOGFONT * lf, t_fontdef * pfd);
///////////////////////////// font and brush list ///////////////////////
struct t_view_brush
{ uns32 rgb;
  HBRUSH hBrush;
};

SPECIF_DYNAR(view_brush_dynar, t_view_brush, 2);

struct t_view_font
{ t_fontdef fontdef;
  HFONT hFont;
};

SPECIF_DYNAR(view_font_dynar, t_view_font, 2);

//
// Multiselect
//
DllPrezen BOOL WINAPI SelectIntRec(HWND hView, trecnum Pos, BOOL Sel, BOOL Redraw);
DllPrezen BOOL WINAPI SelectExtRec(HWND hView, trecnum Pos, BOOL Sel, BOOL Redraw);
DllPrezen BOOL WINAPI IsIntRecSelected(HWND hView, trecnum Pos);
DllPrezen BOOL WINAPI IsExtRecSelected(HWND hView, trecnum Pos);

///////////////////////////// view_dyn //////////////////////////////////
class DataGrid;
class wxWindow;

struct /*DllPrezen*/ view_dyn 
{ BOOL destroy_on_close;
  view_stat * stat;
  DataGrid * hWnd;
  UINT recs_in_win;
  uns32 vwOptions, orig_flags;
  view_dyn * par_inst;
  trecnum toprec, sel_rec;
  int sel_itm;    // selected control number, not used in simple views
  int hordisp, recheight, reclenght;
  int xrecoff, yrecoff, yfooter;
  BOOL sel_is_changed;
  BOOL rec_counted;
  WORD limits;
  tptr upper_select_st;
  t_qbe     * qbe;
  tptr      relsyn_cond;
  bind_def  * bind;               /* binding structure pointer in b-window */

  view_font_dynar form_fonts;
  view_font_dynar print_fonts;
  view_brush_dynar form_brushes;
    HFONT get_form_item_font (t_ctrl * itm);
    HFONT get_print_item_font(t_ctrl * itm);
    HFONT get_form_main_font (void) { return form_fonts .acc0(0)->hFont; }
    HFONT get_print_main_font(void) { return print_fonts.acc0(0)->hFont; }
    HBRUSH get_item_brush(uns32 rgb);
    HBRUSH get_back_brush(void) { return form_brushes.acc0(0)->hBrush; }
    void create_palette(HDC hDC);
    BOOL create_fonts_brushes(HDC hDC);
    BOOL create_print_fonts(HDC hDC);
    void release_fonts_brushes(void);
    void release_print_fonts(void);

  trecnum locked_record;          /* read locked record num (valid if (lock_state & LOCKED_REC)) */
  uns8 lock_state;                /* all cursor tables locked */
  BYTE query_mode;
  HWND hParent;
  wxWindow ** idadr;
  tobjname vwobjname;
  aggr_info * aggr;
  HWND disabling_view;
  HPALETTE pall;
  struct sview_dyn * subinst;   /* pointer to presentation specific data */
  LPWBOLECLDOC olecldoc;
  BOOL has_subview;
  trecnum last_sel_rec;
  t_aa aa;
  CPrintEnv * penv;
  ltable * dt;
  view_dyn * insert_inst;
  xcdp_t xcdp;
  cdp_t cdp;
  struct view_dyn * next_view;
  HWND hDisabled;
  BOOL openning_window; // state flag, TRUE during openning the form's window, 2 after initial Reset_windows when openning
  t_ctrl *has_def_push_button;
  t_vwmode view_mode;
  BOOL synchronizing;
  tptr graphstor;
  int  info_flags;
  HWND hToolTT;   // tooltip window, used only by general form
  enum_cont * enums;
  t_designed_toolbar * designed_toolbar; // NULL iff form uses standard toolbar (and for views without toolbar)
  int wheel_delta;
  POINT ClientRect;
  int  SubViewID;
  char formlinebuf[2*(MAX_FIXED_STRING_LENGTH+1)];  // used as temporary buffer for all short text operations
  bool table_def_locked;
  tptr filter;
  tptr order;

  virtual int  VdIsEnabled (UINT ctrlID, trecnum irec);
   // Returns 2-enabled, 1-disabled, 0-cannot evaluate
  virtual void VdRecordEnter(BOOL hard_synchro);
   // notification about entering a record

  virtual BOOL VdGetNativeVal(UINT ctrlID, trecnum irec, unsigned offset, int * length, char * val);
  virtual BOOL VdPutNativeVal(UINT ctrlID, trecnum irec, unsigned offset, int length, const char * val);
   // Native data type level read/write.
  virtual ~view_dyn(void);
  view_dyn(xcdp_t xcdpIn);
  BOOL open_on_desk(const char * viewsource, tcursnum base,
     uns32 flags, trecnum pos,
     bind_def * bindl, HWND hParent, HWND * viewid, tptr viewname);

  BOOL IsPopup(void) // view s popup window
   { return view_mode==VWMODE_MODAL || view_mode==VWMODE_MODELESS; }
  BOOL IsChild(void) // view is child window other than MDI-child
   { return view_mode==VWMODE_SUBVIEW || view_mode==VWMODE_OLECTRL ||
            view_mode==VWMODE_CHILD;
   }
  bool Remapping(void) const { return dt->remap_a != 0; }
  void init(xcdp_t xcdpIn, BOOL destroy_on_closeIn);
  BOOL open(HWND * viewid);
  void close(void);
  void view_closed(void); // called on WM_NCDESTROY
  void resize_view(int xdisp);
  void UpdatePos();
  BOOL position_it(trecnum pos);  // must be called after defining "bind"
  BOOL create_vwstat(const char * viewsource, tcursnum base, uns32 flags, HWND hParentIn);
  BOOL construct(const char * viewsource, tcursnum base, uns32 flags, HWND hParentIn);
  void destruct(void);
  int  get_current_item(void);
  void wheel_track(void);
  virtual void update_menu_toolbar(BOOL activating, HWND next_active);
  bool accept_query(tptr afilter, tptr aorder);

  void *operator new(size_t size, void * prealloc)
    { return prealloc; }
#ifdef WINS
  void operator delete(void * ptr, void * prealloc) // prevents warning
    { }
#endif
  void *operator new(size_t size)
    { return corealloc(size, 97); }
  void operator delete(void * ptr)
    { corefree(ptr); }

//#ifndef KERN
//    wxMBConv *GetConv(){return cdp ? cdp->sys_conv : wxConvCurrent;}
//#endif
};

struct /*DllPrezen*/ basic_vd : public view_dyn
{ basic_vd(xcdp_t xcdpIn) : view_dyn(xcdpIn) { }
  int  VdIsEnabled(UINT ctrlID, trecnum irec);
  void VdRecordEnter(BOOL hard_synchro);
 // specific methods of basic_vd:
  BOOL VdGetNativeVal(UINT ctrlID, trecnum irec, unsigned offset, int * length, char * val);
  BOOL VdPutNativeVal(UINT ctrlID, trecnum irec, unsigned offset, int length, const char * val);
};

#define GetViewInst(hWnd) ((view_dyn*)GetWindowLong(hWnd, 0))

#define LOCK_REC    1   /* bits in lock_state */
/*#define LOCK_ALL    2 not used, LOCKED_ALL or 0 used instead */
#define LOCKED_REC  4
#define LOCKED_ALL  8

typedef struct
{ unsigned col_start, col_size;
  t_ctrl * itm;  /* column describing item ptr */
} sview_item;

typedef struct sview_dyn
{ view_dyn * inst;
  UINT ItemCount;
  sview_item * sitems;
  UINT first_col;     /* horizontal displacement in columns */
  int  sel_col;       /* selected column number */
  tptr preselect_val;  /* value of the selected cell before selection */
  tptr editbuf;
  UINT editpos, editsel, editdisp;
  BOOL editmode, editchanged;
  BOOL selection_on;
  UINT ToolTipID;
} sview_dyn;

#define NO_QUERY     0    /* values of the query_mode item */
#define QUERY_QBE    1
#define QUERY_ORDER  2

#define NOITEM -1

/* control var part may contain:
   font description,
   get_value_text code,
   visibility condition,
   editability/action condition,
   default value,
   action statements (incl. binding, text editing, sysview, send msg),
   enumerate strings,
   check & convert value, error info,
   control key redefinition,
   control text,
   class if user-defined
*/

#define CT_FONT        1/* any */
#define CT_TEXT        2/* button, static */
#define CT_PICTURE     3/* button, raster */
#define CT_GET_VALUE   4/* value field */
#define CT_GET_ACCESS  5/* */
#define CT_VISIBLE     6/* button, value, edit */
#define CT_ENABLED     7/* button, edit */
#define CT_DEFAULT     8/* edit */
#define CT_ACTION      9/* button, txt, pic, value (click), edit (dbl click) */
#define CT_ENUMER      10/* enum edit, enum value */
#define CT_INTEGRITY   11/* edit */
#define CT_ONERROR     12/* edit */
#define CT_CLASS       13/* any, if not standard */
#define CT_NAMEA       14/* access name used by QBE */
#define CT_NAMEV       15/* value expression used by QBE */
#define CT_COMBOSEL    16/* select statement for combo filling */
#define CT_STATUSTEXT  17/* any */
#define CT_QUERYCOND   18/* query field parameter only */
#define CT_MASK        19/* edit, value */
#define CT_SPECIF      20/* slider, up-down */
#define CT_RCLICK      21/* any */
#define CT_NAME        22/* any */
#define CT_PROP        23/* ActiveX property */
#define CT_EVENT       24/* ActiveX event */
#define CT_EVENTIID    25/* ActiveX event interface IID */
#define CT_FORECOL     26 // dynamic foreground color
#define CT_BACKCOL     27 // dynamic background color
#define CT_FRAMELNWDTH 28 // Frame line width
#define CT_LASTCODE    28

#pragma pack(1)
struct t_slider_specif   /* specific info for the slider (trackbar) control, CT_SPECIF */
{ sig32 rangemin;
  sig32 rangemax;
  sig32 freq;
  void set_default(void)
    { rangemin=0;  rangemax=10;  freq=1; }
};

struct t_updown_specif   /* specific info for the updown control, CT_SPECIF */
{ uns16  buddy_id;   /* filled by gen2, fixed size */
  double ulimit;
  double llimit;
  double step;
  void set_default(void)
    { buddy_id=0;  ulimit=100;  llimit=0;  step=1; }
};

struct t_calendar_specif
{ wbbool no_today;
  wbbool no_today_circle;
  wbbool week_numbers;
  void set_default(void)
    { no_today=no_today_circle=week_numbers=wbfalse; }
};

enum t_bar_code_type {
  BC_Code_39, BC_UPC, BC_EAN, BC_BookLan, BC_Code_93, BC_CODABAR, BC_Interleaved_2_5, 
  BC_Code_128, BC_EAN_UCC_128, BC_POSTNET, BC_PDF417 };

struct t_barcode_specif
{ t_bar_code_type barcode_type;
  int width;
  int reduction;
  int height;
  int ratio;
  int orientation;
  int preferences;
  uns32 commcolor;
 // PDF:
  double aspect;
  int security;
  int overhead;
  int compaction;
  int maxrows;
  int maxcols;
  void set_default(void)
  { barcode_type=BC_Code_39; 
    width=0;  reduction=0;  height=0;  ratio=25;  orientation=0;  commcolor=0;
    preferences=512+32768;
    aspect=1;  security=9;  overhead=0;  compaction=0;   maxrows=90;  maxcols=30;
  }
};

enum t_edit_flags { EF_LOWERCASE=1, EF_UPPERCASE=2, EF_PASSWORD=4, EF_SKIPTONEXT=8 };

struct t_edit_specif
{ int edit_flags;
  int length_limit;  // 0 if no limit
  void set_default(void)
    { edit_flags=(t_edit_flags)0;  length_limit=0; }
};

typedef union
{ t_slider_specif   slider;
  t_updown_specif   updown;
  t_calendar_specif calendar;
  t_barcode_specif  barcode;
  t_edit_specif     edit;
} t_control_specif;
#pragma pack()

/* view var part may contain:
   global font description,
   caption text,
   pane definition,
   record enter action statements,
   global control key redefinition
*/

#define VW_FONT        1
#define VW_CAPTION     2
#define VW_PICTURE     3
#define VW_RECENTER    4        // na nic se nepouziva
#define VW_GROUPA      5
#define VW_GROUPB      6
#define VW_GROUPC      7
#define VW_GROUPD      8
#define VW_GROUPE      9
#define VW_TOOLBAR     10
#define VW_OPENACTION  11
#define VW_CLOSEACTION 12
#define VW_PROJECT_NAME 13
#define VW_TEMPLATE_NAME 14
#define VW_NEWRECACT   15
#define VW_RCLICK      16
#define VW_DBLCLICK    17

#define SFNT_ITAL   1
#define SFNT_BOLD   2
#define SFNT_PROP   4   /* not used */
#define SFNT_UNDER  8
#define SFNT_STRIKE 16
/* akce:
   vytvoreni controlu dle velikosti okna,
   zruseni controlu,
   naplneni controlu novym obsahem dle toprec,
   vypocet viditelosti/akceschopnosti controlu dle toprec,
   zapis nove hodnoty, je-li neco zmeneno

   posilani zprav:
   zprava jde dle focusu, do controlu nebo dbdlg,
    bud je v controlu lokalne plne zpracovana nebo poslana nahoru
    (to, co user muze redef, posila se nahoru, napr. click)
   dbdialog:
    je-li user-def MsgProc, vola se, ta bud spolkne nebo ne
    pokud nespolkne, provede se akce dle popisu controlu nebo dbdlg
   User-def dlg proc: dostane zpravu v nezmenene podobe, je-li systemova
   Sezamove zpravy: napr. dotazy na visibility, dotazy na obsah, zapis hodnoty
    HWND - umozni ziskat popisy pohledu
    wP - control ID

   Zpravy, ktere muze redefinovat uzivatel:
    SZM_IS_VISIBLE      : returns 1=FALSE, 2=TRUE, 0=not processed
    SZM_IS_ENABLED      : returns 1=FALSE, 2=TRUE, 0=not processed
    SZM_GET_VALUE       : returns control value pointer or NULL if not processed
    SZM_PUT_VALUE       : writes control value (is converted)
    SZM_GET_ACCESS      : returns database access & index (if any)
    SZM_CHECK_INTEGRITY : returns 1=FALSE, 2=TRUE, 0=not processed
      lP - converted value pointer

    SZM_ERROR_REPORT
    SM_
      lP - converted value pointer
   Interpreted Windows messages:
    keys
    click, double click
*/
#define SM_ERROR_CONV      1
#define SM_ERROR_INTEGRITY 2
#define SM_ERROR_WRITE     3


#ifdef STOP
#define SZM_NEXTREC   WM_SZM_BASE+60
#define SZM_PREVREC   WM_SZM_BASE+61
#define SZM_FIRSTREC  WM_SZM_BASE+62
#define SZM_LASTREC   WM_SZM_BASE+63
#define SZM_NEXTPAGE  WM_SZM_BASE+64
#define SZM_PREVPAGE  WM_SZM_BASE+65
#define SZM_FIRSTITEM WM_SZM_BASE+66
#define SZM_LASTITEM  WM_SZM_BASE+67
#define SZM_NEXTTAB   WM_SZM_BASE+68
#define SZM_PREVTAB   WM_SZM_BASE+69
#define SZM_DOWNITEM  WM_SZM_BASE+70
#define SZM_UPITEM    WM_SZM_BASE+71
#define SZM_INDEX     WM_SZM_BASE+72
#define SZM_QBE       WM_SZM_BASE+74
#define SZM_UNLIMIT   WM_SZM_BASE+75
#define SZM_BIND      WM_SZM_BASE+76
#define SZM_SETIPOS   WM_SZM_BASE+77
#define SZM_INSERT    WM_SZM_BASE+78
#define SZM_DELREC    WM_SZM_BASE+79
#define SZM_REC_STATUS WM_SZM_BASE+82
#define SZM_DELASK    WM_SZM_BASE+83

#define SZM_SETEPOS   WM_SZM_BASE+85
#define SZM_HELP      WM_SZM_BASE+86
#define SZM_RESET     WM_SZM_BASE+87
#define SZM_ORDER     WM_SZM_BASE+88
#define SZM_PRINT     WM_SZM_BASE+89
#define SZM_ACCEPT_Q  WM_SZM_BASE+90
#define SZM_UNBINDDEL WM_SZM_BASE+91
#define SZM_GET_NAME  WM_SZM_BASE+93
#define SZM_GET_TEXT_VAL    WM_SZM_BASE+94
#define SZM_PUT_TEXT_VAL    WM_SZM_BASE+95
#define SZM_GET_REF_VAL     WM_SZM_BASE+96
#define SZM_GET_FEATURES    WM_SZM_BASE+97
#define SZM_REFRESHR        WM_SZM_BASE+98
#define SZM_DELETE_COLUMN   WM_SZM_BASE+99
#define SZM_ALT_TEXT_VAL    WM_SZM_BASE+100
#define SZM_INSERT_COLUMN   WM_SZM_BASE+101
#define SZM_GET_TOOLBAR     WM_SZM_BASE+102
#define SZM_SELECT_ME       WM_SZM_BASE+103
#define SZM_LOCKS           WM_SZM_BASE+104
#define SZM_DROP            WM_SZM_BASE+105
#define SZM_SET_EDIT_POS    WM_SZM_BASE+106
#define SZM_EXPORTRQ        WM_SZM_BASE+107
#define SZM_IMPORTRQ        WM_SZM_BASE+108
#define SZM_TOKEN           WM_SZM_BASE+118
#define SZM_PRIVILS         WM_SZM_BASE+119
#define SZM_POP_DESIGN      WM_SZM_BASE+120

#define SZM_SHOWAPPL        WM_SZM_BASE+200  // wParam==TRUE: show, FALSE: hide
#define SZM_MDI_CHILD_TYPE  WM_SZM_BASE+201
#define SZM_RELISTOBJ       WM_SZM_BASE+202
#define SZM_REFILL           WM_SZM_BASE+202
#define SZM_NEWWINNAME       WM_SZM_BASE+203
#define SZM_GLOBALFREE       WM_SZM_BASE+204
#define SZM_NEW_NUMS         WM_SZM_BASE+205
#define SZM_ANSWER           WM_SZM_BASE+206
#define SZM_DESTROY          WM_SZM_BASE+207
#define SZM_CLOSE_EDITOR     WM_SZM_BASE+208
#define SZM_TAB_INVALID      WM_SZM_BASE+209
#define SZM_INFO_PANEL       WM_SZM_BASE+210
#define SZM_UNLINK           WM_SZM_BASE+211

#define SZM_SETSTATUSTEXT    WM_SZM_BASE+212
#define SZM_SETTOOLBAR       WM_SZM_BASE+213
#endif 

#define SZM_RCLICK          WM_SZM_BASE+116
#define SZM_ITEM_CLASS      WM_SZM_BASE+117
#define SZM_COMMIT          WM_SZM_BASE+109
#define SZM_UNLINK_CACHE_ED WM_SZM_BASE+110
#define SZM_EXPIMP          WM_SZM_BASE+111
#define SZM_SYNCHRO         WM_SZM_BASE+114
#define SZM_ROLL_BACK       WM_SZM_BASE+115
#define SZM_CHILDPOSITION   WM_SZM_BASE+121


#define FIRST_MDI_CHILD    4520
#define APPL_MDI_CHILD     4520   /* return values for SZM_MDI_CHILD_TYPE */
#define VIEW_MDI_CHILD     4521
#define EDIT_MDI_CHILD     4522
#define OBJEDIT_MDI_CHILD  4523
//#define IPANEL_MDI_CHILD   4524
#define PROGEDIT_MDI_CHILD 4525
#define MENU_MDI_CHILD     4526
#define PANE_MDI_CHILD     4527
//#define FPAN_MDI_CHILD     4528
#define PICTURE_MDI_CHILD  4529
#define DRAWING_MDI_CHILD  4530
#define PREVIEW_MDI_CHILD  4531
#define QEDIT_MDI_CHILD    4532
#define LAST_MDI_CHILD     4532



#define ITCL_STRING   1   /* none must be 0 */
#define ITCL_CHECK    2
#define ITCL_BUTTON   3

#define SC_FIRST     (WPARAM)0xea70   /* view system menu items */
#define SC_QBE       (WPARAM)0xea80   /* >60000, <0xf000 */
#define SC_UNLIMIT   (WPARAM)0xea90
#define SC_BIND      (WPARAM)0xeaa0
#define SC_INDEX     (WPARAM)0xeab0
#define SC_ORDER     (WPARAM)0xeac0
#define SC_ACCEPT_Q  (WPARAM)0xead0
#define SC_PRINT     (WPARAM)0xeae0
#define SC_DELUNBND  (WPARAM)0xeaf0
#define SC_INTERRUPT (WPARAM)0xeb00  /* editor & main windows system menu item */
#define SC_EXPIMP    (WPARAM)0xeb10
#define SC_LOCKING   (WPARAM)0xeb20
#define SC_LAST      (WPARAM)0xeb20

/* VIEW FEATURES: */
#define VF_QBE       1
#define VF_ORDER     2
#define VF_DELREC    4
#define VF_DELALL    8
#define VF_PRINT     16
#define VF_INDEX     32
#define VF_INSREC    64
#define VF_DELCOLUMN 128
#define VF_VIEWEDIT  256

#define VF_COLSIZE   0x0200
#define VF_COLPOS    0x0400

/*
NAME DBDIALOG x, y, width, height
[ STYLE ... ]
[ CAPTION "..." ]
[ FONT pointsize,"face"[,options] ]
[ HELP ... ]
[ PANE letter expression ]
[ BPICTURE name ]
[ NEWREC statement ]
BEGIN
controlname ["text",] ID, [class if CONTROL, style], x,y,cx,cy[,style if not CONTROL]
  [ OBJECT definition - expression or access ]
  [ PANE panename ]
  [ COLOUR col ]
  [ PRECISION prec ]
  [ LOCATION loctype ]
  [ FONT pointsize,"face"[,options] ]
  [ VISIBILITY condition ]
  [ ENABLED condition ]
  [ INTEGRITY condition ]
  [ DEFAULT "tx val" ]
  [ ONERROR statement ]
  [ ENUMERATE ".." ".." .. ]
  [ BPICTURE name ]

END
*/

/* values of inst->stat->vwStyle (flags << 16): */
#define NO_VIEW_EDIT       0x10000L  /* bits of inst->limits */
#define NO_VIEW_INSERT     0x20000L  /* obsolete, replaced by NO_VIEW_INS and NO_VIEW_FIC */
#define NO_VIEW_MOVE       0x40000L
#define NO_VIEW_DELETE     0x80000L
#define VIEW_DEL_RECS     0x100000L
#define VIEW_PRINT_VIEW   0x200000L /* view for print only, don't open the window - nikde se nepouziva*/
#define VIEW_AUTO_CURSOR  0x400000L
#define VIEW_COUNT_RECS   0x800000L
#define VIEW_MODAL_VIEW  0x1000000L
#define VIEW_IS_PRINTED  0x2000000L
#define VIEW_INDEP_QBE  0x08000000L /* independent query view */
#define NO_SUBVIEW_FRAME 0x10000000L
#define IS_VIEW_SUBVIEW 0x20000000L /* display as a subview */
#define VIEW_LOCK_REC   0x40000000L /* lock the current record when openning */
#define VIEW_LOCK_ALL   0x80000000L /* lock all records when openning */

#define USER_VIEW_FLAGS (NO_VIEW_EDIT | NO_VIEW_MOVE | \
     NO_VIEW_DELETE | VIEW_AUTO_CURSOR | VIEW_DEL_RECS)

/* private flags moved to the upper word of vwOptions: */
#define PRINT_VIEW     0x20 /* view for print only, don't open the window */
//#define INDEP_QBE    0x0800 -- not used, is implicit
#define NO_CHILD_FRAME 0x1000 // bez ramu kolem subpohledu 
#define IS_SUBVIEW   0x2000
#define LOCKING_REC  0x4000 /* lock the current record when openning */
#define LOCKING_ALL  0x8000 /* lock all records when openning */
/* private view flags, not stored in the view: */
#define QUERY_VIEW    0x10000L /* open in a special query mode */
#define ORDER_VIEW    0x20000L /* open in a special order mode */
//#define SYNCHRONIZE   0x40000L /* register synchronization ith the parent */
#define PARENT_CURSOR 0x80000L /* use parent cursor and position */
#define MODELESS_VIEW 0x100000
#define VIEW_OLECTRL  0x200000

#define X_PRI_GRID        8
#define Y_PRI_GRID       28

typedef struct
{ uns16 size;
  BITMAPFILEHEADER a;
  BITMAPINFOHEADER b;
  RGBQUAD          c[256];    /* 0-256 colors */
} BITMAPSTART;

/* actions: */
#define ACT_EMPTY            0
#define ACT_VIEW_OPEN        1
#define ACT_VIEW_CLOSE       2
#define ACT_VIEW_TOGGLE      3
#define ACT_VIEW_PRINT       4
#define ACT_VIEW_RESET_LIGHT 5
#define ACT_RELATE           6
#define ACT_BINDING          7
#define ACT_EDIT_TEXT        8
#define ACT_MENU_SET         9
#define ACT_MENU_REMOVE      10
#define ACT_CLOSE_VW_MENU    11
#define ACT_MSG_WINDOW       12
#define ACT_MSG_STATUS       13
#define ACT_EXPORT_TABLE     14
#define ACT_IMPORT_TABLE     15
#define ACT_EXPORT_CURSOR    16
#define ACT_TABLE_VIEW       17
#define ACT_CURSOR_VIEW      18
#define ACT_FREE_DEL         19
#define ACT_INSERT           20
#define ACT_SEL_PRINTER      21
#define ACT_PRINT_OPT        22
#define ACT_TILE             23
#define ACT_CASCADE          24
#define ACT_ARRANGE          25
#define ACT_SEND_MSG         26
#define ACT_STMTS            27
#define ACT_VIEW_SYNOPEN     28
#define ACT_VIEW_MODOPEN     29
#define ACT_VIEW_MODSYNOPEN  30
#define ACT_VIEW_COMMIT      31
#define ACT_VIEW_ROLLBACK    32
#define ACT_QBE              33
#define ACT_ORDER            34
#define ACT_UNLIMIT          35
#define ACT_ACCEPT_Q         36
#define ACT_INDEP_QBE        37
#define ACT_VIEW_RESET_CURS  38
#define ACT_REC_PRINT        39
#define ACT_SHOW_HELP        40
#define ACT_ATTR_SORT        41
#define ACT_VIEW_RESET       42
#define ACT_SHOW_HELP_POPUP  43
#define ACT_MENU_REDRAW      44
#define ACT_PAGE_SETUP       45
#define ACT_DEL_REC          46
#define ACT_FLOWUSER         47
#define ACT_FLOWSEL          48
#define ACT_FLOWSTOP         49
#define ACT_PRIVILS          50
#define ACT_TOKEN            51
#define ACT_REPLOUT          52
#define ACT_REPLIN           53
#define ACT_ALL_PRINT        54
#define ACT_INS_OPEN         55
#define ACT_VIEW_RESET_SYNCHRO 56
#define ACT_VIEW_RESET_COMBO 57
#define ACT_SHELL_EXEC       58
#define ACT_RELSYN           59
#define ACT_BEEP             60
#define ACT_TRANSPORT        61
#define ACT_POPUP_MENU       62
#define ACT_ALL_PRINT_REP    63
#define ACT_BIND             64

#define ACT_COUNT            65
/**************************** utils *****************************************/
/* Win32 portability: */
#define GetWindowPortId(hWnd)          GetWindowLong(hWnd,GWL_ID)
#define SetWindowPortId(hWnd,Id)       SetWindowLong(hWnd,GWL_ID,Id)
#define GetWindowPortInst(hWnd)        (HINSTANCE)GetWindowLong(hWnd,GWL_HINSTANCE)
#define SetClassPortCursor(hWnd,hCurs) SetClassLong(hWnd,GCL_HCURSOR,(LONG)hCurs)

#pragma pack(1)
typedef struct   /* variable-size info in the view description */
{ BYTE type;
  WORD size;
#ifdef UNIX
  char value[0];
#else
  char value[];
#endif
} var_start;
#pragma pack()

/***************************** options ***************************************/
#define MAX_GROUP_NAME   80
#define MY_MAX_PATH 260
#define MAX_EXT_EDITOR_PATH 80
typedef struct
{ LOGFONT editor_screen_font;
  LOGFONT editor_printer_font;
  BOOL    editor_turbo;
  BOOL    editor_wrap;
  BOOL    editor_align;
  LOGFONT view_font;
  LOGFONT report_font;
  unsigned view_xraster;
  unsigned view_yraster;
  int view_prec_real;
  int view_prec_money;
  int view_prec_date;
  int view_prec_time;
  int view_prec_raster;
  int view_prec_ole;
  BOOL   oldvermargin;
  char   group_name[MAX_GROUP_NAME+1];
  uns32   break_back;
  uns32   break_fore;
  uns32   pos_back;
  uns32   pos_fore;
  uns32   posbreak_back;
  uns32   posbreak_fore;
  char   mail_conf_path[MY_MAX_PATH];
  char   mail_lib_path[MY_MAX_PATH];
  char   mail_exp_dir[MY_MAX_PATH];
 /* used by mailer only */
  char   mail_imp_dir[MY_MAX_PATH];
  BOOL   del_msg;
  WORD   check_period;
  WORD   xScreenExt;
  WORD   yScreenExt;
  BOOL   edi_frame;
  BOOL   labels_old;
  BOOL   login_dialog;
  char   private_key_dir[MY_MAX_PATH];
  BOOL   longnames;
  BOOL   accept_return;
  char   html_editor[MAX_EXT_EDITOR_PATH];
  char   text_editor[MAX_EXT_EDITOR_PATH];
  BOOL   NoBindTypeErrorReport;
  BOOL   NoPrivils;
} prez_params;

DllPrezen void WINAPI save_prez_params(prez_params *pp);
DllPrezen prez_params * WINAPI GetPP(void);

#ifndef LINUX
BOOL RegisterView(HINSTANCE hInst);
void UnregisterView(void);
DllPrezen HWND WINAPI GetFrame(HWND hWnd);
DllPrezen void WINAPI WB_help(cdp_t cdp, UINT style, uns32 topic);
DllPrezen BOOL process_context_menu(cdp_t cdp, HWND hDlg, WPARAM wP, LPARAM lP);
#ifdef WINS
DllPrezen BOOL WINAPI process_context_popup(cdp_t cdp, const HELPINFO * hi);
#endif
DllPrezen void WINAPI reset_view(HWND hWnd, trecnum rec, int extent);
DllPrezen void WINAPI update_view(view_dyn * inst, trecnum startrec, trecnum stoprec, int extent);
t_ctrl * get_control_by_text(view_dyn * inst, tptr text);
DllPrezen void WINAPI find_top_rec_by_sel_rec(view_dyn * inst);
uns32 WINAPI calc_color(view_dyn * inst, t_ctrl * itm, trecnum irec, uns32 color, BOOL foreground);
#endif
tptr item_text(t_ctrl * itm);

/* values of extent parameter: */
#define RESET_NONE      0
#define RESET_ALL       1
#define RESET_SEL       2
#define RESET_INFLU     3
#define RESET_RADIO     4
#define RESET_ALL_REMAP 5
#define RESET_PROPAG    6
DllPrezen void WINAPI reset_control_contents(view_dyn * inst, t_ctrl * ctrl, HWND hCtrlWnd, trecnum rec);

void free_sinst(sview_dyn * sinst);
int    get_itm_num   (view_dyn  * inst, int   Id);

DllPrezen tptr WINAPI initial_view(xcdp_t xcdp, int x_view_pos, int y_view_pos, int vw_type,
            const char * basename, tcateg basecat, tobjnum base_num, const char * odbc_prefix);
DllPrezen view_stat * WINAPI load_template(cdp_t cdp, const char * template_name);
DllPrezen t_ctrl * WINAPI find_item_by_class(view_stat * vdef, int ClassNum, int tp);
DllPrezen void WINAPI write_font(t_fontdef * font, char * dest);
DllPrezen void WINAPI make_control_name(cdp_t cdp, char * buf, const char * name, int ClassNum, int ID);
#define VIEWTYPE_SIMPLE  3    /* standard view */
#define VIEWTYPE_DEL     8    /* + include "DELETED" attribute */

DllPrezen HFONT WINAPI GetSysFont(HWND hFrame);
DllPrezen BOOL WINAPI  write_val(HWND hWnd);
DllPrezen HFONT WINAPI  cre_font(t_fontdef *fd, unsigned logpix);
DllPrezen uns32 WINAPI  color_short2long(BYTE col);
DllPrezen tptr WINAPI  get_var_value(WORD * varcode, BYTE type);
DllPrezen const wchar_t * WINAPI get_enum_def(view_dyn * inst, t_ctrl * itm);
DllPrezen void WINAPI  draw_frame(HDC hDC, t_ctrl * itm, UINT xdisp, UINT ydisp,
  BOOL nullpos, HBRUSH hBrush, HPALETTE pall, view_stat * vdef);
DllPrezen void WINAPI create_tab_control_pages(HWND hCtrl, tptr tx);
DllPrezen void WINAPI WriteMyProfileInt(const char * section, const char * entry, sig32 val);

BOOL IsWBChild(HWND hWnd);
DllPrezen BOOL WINAPI IsView(HWND hWnd);
DllPrezen void WINAPI DestroyMdiChildren(HWND hwClient, BOOL close_editors);
DllPrezen BOOL WINAPI copy_to_file(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr,
                     uns16 index, FHANDLE hFile, int recode, uns32 offset);
DllPrezen BOOL WINAPI copy_from_file(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr,
              uns16 index, FHANDLE hFile, int recode, uns32 offset);
DllPrezen BOOL WINAPI old_copy_from_file(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr,
              uns16 index, int handle, int recode);
DllPrezen HFONT WINAPI  create_log_font(LOGFONT * lf, HDC hDC, HWND hWnd);
DllPrezen HWND WINAPI find_view(char * viewdef);
DllPrezen view_dyn * WINAPI find_inst(cdp_t cdp, const char * viewdef);

DllPrezen BOOL WINAPI print_stored_view(cdp_t cdp, tobjnum objnum, tcursnum base,
              trecnum start, trecnum stop, tptr objname);
DllPrezen HWND WINAPI  Bind_records(HWND hParent, tcursnum cursnum, trecnum position,
                 tattrib attr, uns16 index, uns16 sizetype,
                 const char * viewdef, tcursnum base, uns32 flags,
                 void * reserved, HWND hClient);
extern uns8 bez_outcode[];
DllPrezen tptr WINAPI  get_clip_text(HWND hWnd);
DllPrezen void WINAPI  put_clip_text(HWND hWnd, tptr txt, int size);
DllPrezen void  WINAPI prez_empty(void);

DllPrezen BOOL WINAPI do_report(trecnum startrec, trecnum stoprec, HWND hWnd, HWND *hPrev, void *res = NULL);
#define IDM_HELPHELP       221
DllPrezen void WINAPI load_prez_params(void);
extern HWND hModalView;   /* actual modal view */
/****************************** menu structures *****************************/
#define MAX_MENU_STRING  80
/* menu item flags */
#define MENU_LAST        0x80  /* last in a menu or popup */
#define HAS_POPUP           1  /* the menu item has a popup */
#define MENU_SEPARATOR      2  /* item is a separator */
#define IS_WINDOWS_SUBMENU  4  /* Windows submenu mark (used with HAS_POPUP) */
#define BREAK_NORM          8  // MF_MENUBREAK
#define BREAK_LINE         16  // MF_MENUBARBREAK
#define RADIO_MENU         32  // part of radio group

#define START_USER_MENU_ID  700
#define USER_MENU_START2    49000
#define USER_MENU_STOP2     50000

#define MI_FRM_START     420  // order and values fixed in form_message_translation[]
#define MI_FRM_CUT    421
#define MI_FRM_COPY   422
#define MI_FRM_PASTE  423
#define MI_FRM_CLEAR  424
// popup      425
#define MI_FRM_1STREC 426
#define MI_FRM_PRPAGE 427
#define MI_FRM_PRREC  428
#define MI_FRM_NXREC  429
#define MI_FRM_NXPAGE 430
#define MI_FRM_LSTREC 431
// popup              432
#define MI_FRM_QBE    433
#define MI_FRM_ORDER  434
#define MI_FRM_UNLIM  435
#define MI_FRM_ACCEPT 436
#define MI_FRM_DELREC 437
#define MI_FRM_DELALL 438
#define MI_FRM_PRINT  439
#define MI_FRM_INDEX  440
#define MI_FRM_LOCKS  441
#define MI_FRM_EXPIMP 442
#define MI_FRM_PRIVIL 443
#define MI_FRM_UNBINDDEL 444
#define MI_FRM_TOKEN     445
#define MI_FRM_REFRESH   446
#define MI_FRM_HELP   447
#define MI_FRM_STOP      447

#define KEY_BEGIN       1
#define KEY_END         2
#define KEY_MENUITEM    3
#define KEY_POPUP       4
#define KEY_ACTIVE      5
#define KEY_CHECKED     6
#define KEY_INFOTEXT    7
#define KEY_MENUACTION  8
#define KEY_PBEGIN      9
#define KEY_PEND       10
#define KEY_BITMAP      11
#define KEY_CHKBITMAP   12
#define KEY_UNCHKBITMAP 13

DllPrezen LRESULT WINAPI set_frame_menu(HWND hClient, HMENU hMenu, HMENU hWinMenu);
DllPrezen void WINAPI Envir_menu(HWND hFrame, HMENU hEnvMenu, BOOL openning);
DllPrezen HBITMAP WINAPI resize_check_bitmap(HBITMAP handle);
DllPrezen int  WINAPI get_menukey(CIp_t CI);
DllPrezen BOOL WINAPI test_menu_def(cdp_t cdp, HWND hParent, const char * source, int * err_object);
DllPrezen int  WINAPI check_menu_def(cdp_t cdp, tptr def);
DllPrezen void WINAPI translate_menu_item_text(char * str);
DllPrezen void WINAPI track_popup_menu(cdp_t cdp, tobjnum menu, HWND hParent);
BOOL reset_menu(cdp_t cdp, HMENU hMenu, tptr menucode);
extern char USERMENU[9];    /* menu code property name */
extern char USERMENUH[10];  /* menu handle property name */
extern char ORIGMENU[9];    /* menu handle property name */
extern const char POPUP_MENU[11];
/**************************** printing **************************************/
DllPrezen HDC GetPrinterDC(view_stat * vdef, int print_dir, BOOL * DC_created);
DllPrezen BOOL WINAPI  AbortProc(HDC hPrnDC, int nCode);
BOOL CALLBACK PrintingDlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP);
DllPrezen void WINAPI  cancel_printing(void);
BOOL RegisterPreview(HINSTANCE hInst);
void LoadPrinterParams(LPSTR pName, LPDEVMODE pDevMode, LPSTR pPort);
extern wbbool print_cancel_request;
extern HWND hPrintDlg;
void write_printer_name(HWND hCtrl);
BOOL text_print_dialog(HWND hParent, BOOL * selection);
#define PRI_DIR_PRINTER 0    /* print direction (to printer, screen, file) */
#define PRI_DIR_SCREEN  1
#define PRI_DIR_FILE    2
#define PRI_DIR_OLEVIEW 4
#define PRI_DIR_OLEREPORT 8
#define PRI_DIR_FILEIMAGE 16
/*************************** full screen editor *****************************/

#define ECF_POPUP     1
#define ECF_EDCONT    2
#define ECF_AXEVHNDLR 4

class CEditCtx
{
public:

    LPCSTR m_Name;
    DWORD  m_Size;
    HWND   m_hPar;
    int    m_x;
    int    m_y;
    int    m_cx;
    int    m_cy;
    WORD   m_Col;
    WORD   m_Row;
    BYTE   m_Flags;
    BYTE   m_Categ;

    CEditCtx(LPCSTR Name, DWORD Size, HWND hPar = (HWND)NULL, WORD Row = 0, WORD Col = 0, BYTE Categ = 0xFF, BYTE Flags = 0, int x = -1, int y = -1, int cx = -1, int cy = -1)
    {
        m_Name  = Name; 
        m_Size  = Size; 
        m_hPar  = hPar; 
        m_x     = x;    
        m_y     = y;    
        m_cx    = cx;   
        m_cy    = cy;   
        m_Col   = Col;
        m_Row   = Row;
        m_Flags = Flags;
        m_Categ = Categ;
    }

    virtual BOOL IsSame(CEditCtx *Other) = 0;
    virtual int  LoadText(char *editbuf) = 0;
    virtual BOOL SaveText(char *editbuf, int Size) = 0;
    virtual BOOL PreCompile(char *editbuf, int Size) = 0;
    virtual void PostCompile() = 0;
};

DllExport HWND WINAPI Edit_textblock(CEditCtx *Ctx);
DllPrezen void open_and_position_editor(cdp_t cdp, tcateg cat, tobjnum objnum, uns32 offset);
DllPrezen void WINAPI search_in_objects(cdp_t cdp, HWND hWnd, const char * init_string);
DllPrezen void WINAPI close_search_dialog(void);
DllPrezen short WINAPI  run_pgm_chained(cdp_t cdp,
  const char * srcname, tobjnum srcobj, HWND hWnd, BOOL recompile);
DllPrezen int WINAPI synchronize(cdp_t cdp, const char * srcname, tobjnum srcobj, tobjnum * codeobj, BOOL recompile, BOOL debug_info, BOOL report_success);
int WINAPI set_project_up(cdp_t cdp, const char * progname, tobjnum srcobj,
            BOOL recompile, BOOL report_success);
DllPrezen void   WINAPI strip_diacr(char * dest, const char * src, const char * app);
//DllPrezen void   WINAPI object_fname(char * dest, const char * src, const char * app);
DllPrezen char * WINAPI get_suffix(tcateg categ);
DllPrezen UINT   WINAPI get_filter(tcateg categ);
DllPrezen void   WINAPI fname2objname(const char * fname, char * objname);
DllPrezen BOOL    WINAPI  save_new_source(tobjnum objnum);
DllPrezen LRESULT WINAPI  debug_command(WPARAM wParam, LPARAM lParam);
LONG WINAPI Fsed_INITMENUPOPUP(HWND hWnd, WPARAM wParam, LPARAM lParam);
LONG WINAPI Fsed_COMMAND      (HWND hWnd, WPARAM wParam, LPARAM lParam);
BOOL RegisterFsed(HINSTANCE hInst);
void UnregisterFsed(void);
DllPrezen BOOL WINAPI check_object_privils(cdp_t cdp, tcateg categ, tobjnum objnum, int privtype);
DllPrezen void WINAPI closing_client(cdp_t cdp);
DllPrezen BOOL WINAPI readable_object(cdp_t cdp, tcateg categ, tobjnum objnum);
DllPrezen BOOL WINAPI check_readable_object(cdp_t cdp, tcateg categ, tobjnum objnum);
/**************************** file selector *********************************/
extern char last_path[MY_MAX_PATH];
DllPrezen char * WINAPI  first_suffix(UINT filter);
DllPrezen char * WINAPI  x_select_file(char * path, int filters, char * initname, HWND hParent, int * recode);
DllPrezen char * WINAPI  create_file(char * path, int filters, char * initname, HWND hParent, int * recode);
DllPrezen BOOL WINAPI  overwrite_file(const char * fname, BOOL * overwrite_all);
/***************************** sezam box ************************************/
DllPrezen int WINAPI  SezamBox(HWND hWnd, const char * text,
              const char * caption, LPCSTR icon, unsigned * buttons);
DllPrezen void WINAPI  modif_sys_menu(HMENU hMenu, BOOL is_main);
DllPrezen BOOL WINAPI  AppendMenuString(HMENU hMenu, UINT itemID, const char * text);
DllPrezen BOOL WINAPI  ModifyMenuString(HMENU hMenu, UINT pos, UINT itemID, const char * text);
DllPrezen HWND WINAPI  GetClient(HWND hWnd);
#define set_status_nums(x,y)  Set_status_nums(x,y)
#define set_status_text(x)    Set_status_text(x)
DllPrezen void  WINAPI Set_client_status_nums(cdp_t cdp, trecnum num0, trecnum num1);
void WINAPI Set_item_text(const char * text, int ID, HWND hWnd);
DllPrezen void WINAPI  menu_info(tptr menucode, HINSTANCE hInst, WPARAM wParam, LPARAM lParam);
BOOL RegisterStatusBar(HINSTANCE hInst);
void UnregisterStatusBar(void);
DllPrezen BOOL WINAPI Make_rectangle_visible(int &x, int &y, int &cx, int &cy);
DllPrezen BOOL WINAPI PrintBarCode(cdp_t cdp, HDC hDC, const char * data, int length, t_ctrl * itm, float x = 0, float y = 0);

/**************************** wprez / wprez2 ********************************/
#define BINDSZX 24
#define BINDSZY 25
int run_view_code(uns8 ct, view_dyn * inst);
DllPrezen void WINAPI  get_view_error(int * perr_object);
LRESULT CALLBACK ViewWndProc (HWND hWnd, UINT msg, WPARAM wP, LPARAM lP);
LRESULT CALLBACK SviewWndProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP);
LRESULT CALLBACK PictWndProc (HWND hWnd, UINT msg, WPARAM wP, LPARAM lP);
LRESULT CALLBACK SysIndexWndProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP);
LRESULT CALLBACK HistoryWndProc (HWND hWnd, UINT msg, WPARAM wP, LPARAM lP);
DllPrezen tptr WINAPI  enum_read (view_dyn * inst, t_ctrl * itm, sig32 val);
DllPrezen BOOL WINAPI  write_enum(view_dyn * inst, t_ctrl * itm, int index, basicval * theval);
DllPrezen LRESULT WINAPI  qbe_dataproc(view_dyn * inst,
                  UINT msg, WPARAM wP, LPARAM lP);
DllPrezen LRESULT WINAPI  order_dataproc(view_dyn * inst,
                  UINT msg, WPARAM wP, LPARAM lP);
DllPrezen t_ctrl * WINAPI  get_control_def(view_dyn * inst, int Id);
DllPrezen void WINAPI  draw_owner_button(HBRUSH hBrush, DRAWITEMSTRUCT * ds, t_ctrl * itm, tobjnum objnum, COLORREF rgbcolor, BOOL is_down);
DllPrezen HBITMAP WINAPI  DrawRaster(HWND hWnd, HDC hDC, view_dyn * inst, curpos *wbacc, t_ctrl *itm);
DllPrezen HBITMAP WINAPI  GetRaster(ttablenum Table, trecnum RecNum, tattrib attr, WORD index, HPALETTE *lpPal = NULL);
DllPrezen void WINAPI  ReleaseRaster(HWND hWnd);
DllPrezen BOOL WINAPI  GetRastSize(view_dyn * inst, curpos *wbacc, UINT *Width, UINT *Height);
/* inst != NULL iff picture is in the cache. */
DllPrezen int WINAPI  ExportPicture(LPSTR FileName, ttablenum tb, trecnum rc, tattrib attr, WORD index, int Konv);
LRESULT ViewInitMenuPopup(HWND hWnd, view_dyn * inst, HMENU hMenu, LPARAM lP);
void prez_set_null(tptr adr, uns8 tp);
DllPrezen void WINAPI  set_temp_inst(view_dyn * inst);
BOOL view_record_locking(view_dyn * inst, trecnum erec, BOOL read_lock, BOOL unlock);
//void select_all_item_text(view_dyn * inst);

DllPrezen t_ctrl * WINAPI get_control_pos(view_dyn * inst, int sel);
DllPrezen void WINAPI  OnSzmHelp(view_dyn * inst, WPARAM wP, LPARAM lP);
#define OW_PTRS       9
#define OW_PREZ      10
void raster_act(WORD action_id, HWND hWnd, view_dyn * inst);
DllPrezen HWND WINAPI  open_picture_window(view_dyn * inst,
  UINT ctId, tobjnum objnum);
void post_notif(HWND hWnd, UINT notif_num);
uns32 create_qbe_subcurs(view_dyn * inst, BOOL really, view_dyn * dest_inst, tptr * new_select);
tptr prep_qbe_subcurs(view_dyn * inst, view_dyn * dest_inst);
HSTMT create_odbc_subcurs(tptr subcurscond, view_dyn * inst, BOOL really, tptr * new_select);
//unsigned odbc_tpsize(int type, t_specif specif);  // space size including 1 byte terminator for DEC/NUM
DllPrezen BOOL WINAPI  odbc_load_record(cdp_t cdp, ltable * dt, trecnum extrec, tptr dest);
DllPrezen void WINAPI cache_load(ltable * dt,
          view_dyn * inst, unsigned offset, unsigned count, trecnum new_top);
DllPrezen BOOL WINAPI wb_write_cache(cdp_t cdp, ltable * dt,
           tcursnum cursnum, trecnum * erec, tptr cache_rec, BOOL global_save);
//DllPrezen BOOL WINAPI odbc_synchronize_view(cdp_t cdp, BOOL update, t_connection * conn,
//  ltable * dt, tcursnum cursnum, trecnum extrec,
//  tptr new_vals, tptr old_vals, BOOL on_current);
DllPrezen BOOL WINAPI cache_synchro(view_dyn * inst);
DllPrezen void WINAPI  cache_free(ltable * dt, unsigned offset, unsigned count);
DllPrezen BOOL WINAPI  setpos_synchronize_view(cdp_t cdp, BOOL update,
   ltable * dt, tptr new_vals, trecnum extrec, BOOL positioned);
DllPrezen BOOL WINAPI  setpos_delete_record(cdp_t cdp, ltable * dt, trecnum extrec);
DllPrezen ltable * WINAPI create_data_cache(cdp_t cdp, tcursnum curs, trecnum reccount);
DllPrezen const atr_info * WINAPI access_data_cache(ltable * dt, int column_num, trecnum rec_offset, void * & valptr);
DllPrezen BOOL WINAPI alloc_data_cache(ltable * dt, trecnum reccount);


DllPrezen BOOL WINAPI  font_dialog(HWND hOwner, LOGFONT * lf, uns32 cfFlags);
void stop_modality(view_dyn * inst);
BOOL init_relational_view(view_dyn * inst, bind_def * bd);
extern HICON ic_bound, ic_unbound, ic_fibound;
extern const char * std_class_names[NO_CLASS_COUNT];
LRESULT CALLBACK SmProcStatic   (HWND hWnd,UINT msg,WPARAM wP,LPARAM lP);
LRESULT CALLBACK SmProcRaster   (HWND hWnd,UINT msg,WPARAM wP,LPARAM lP);
LRESULT CALLBACK SmProcSignature(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP);
LRESULT CALLBACK SmProcRtf      (HWND hWnd, UINT msg, WPARAM wP, LPARAM lP);

void sort_bind_list(trecnum * list);

void set_basic_status_text(view_dyn * inst);
void enable_parent(view_dyn * inst, BOOL state);
int run_code(uns8 ct, view_dyn * inst, t_ctrl * itm, trecnum irec);
BOOL SuperPostMessage(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP);
void set_vert_scroll(HWND hWnd, view_dyn * inst);
DllPrezen object * WINAPI find_symbol(run_state * RV, const char * name);

trecnum view_recnum(view_dyn * inst);
DllPrezen int WINAPI  odbc_rec_status(cdp_t cdp, ltable * dt, trecnum extrec);
void find_visible_control(view_dyn * inst, HWND hWnd);
BOOL odbc_reset_cursor(view_dyn * inst);
BOOL odbc_reset_cursors(view_dyn * inst, char *stm);
#define PROPAGATE_SET       1
#define PROPAGATE_RESTRICT  2
#define PROPAGATE_RESTORE   3

DllPrezen void WINAPI  set_top_rec(view_dyn * inst, trecnum new_top);
BOOL cache_resize(view_dyn * inst, unsigned new_riw);
void cache_roll_back(view_dyn * inst);
void cache_insert(view_dyn * inst, trecnum extrec);
void cache_reset(view_dyn * inst);
DllPrezen tptr WINAPI  load_cache_addr(HWND hWnd, trecnum position, tattrib attr, uns16 index, BOOL grow);
DllPrezen char * WINAPI load_inst_cache_addr(view_dyn * inst, trecnum position, tattrib attr, uns16 index, BOOL grow);
DllPrezen BOOL WINAPI  register_edit(view_dyn * inst, int attrnum);
void unlink_cache_editors(HWND hOwner, trecnum recnum);
void bind_cache(cdp_t cdp, ltable * ltab);
DllPrezen BOOL WINAPI make_ext_odbc_name(odbc_tabcurs * tc, tptr extname, uns32 usage_mask);
void propagate_update(view_dyn * orig_inst);
void confirm_query(view_dyn * inst);
DllPrezen void WINAPI Set_timer(cdp_t cdp, int elapse, unsigned msg);

extern t_ctrl_base empty_control;
extern view_dyn *  TempInst;  /* temporary view inst - used during window creation */
extern signed char TempPrecision; /* used when creting system index view etc. */
extern BYTE        TempType;      /* -"- */
extern t_aa Temp_aa;  /* temporary info when creating aa-windows */
HWND open_history   (view_dyn * inst, t_ctrl * itm, HWND hParent);
HWND open_index_view(view_dyn * inst, t_ctrl * itm, HWND hParent);
BOOL is_button(t_ctrl * itm);
BOOL is_check (t_ctrl * itm);
extern WNDPROC OrigWndProcs[NO_CLASS_COUNT];
LRESULT OnViewVScroll(HWND hWnd, view_dyn * inst, WPARAM wP, LPARAM lP);
DllPrezen void WINAPI  subview_synchronize(view_dyn * inst, BOOL synchro_parent, trecnum rec);
int unb_ask(HWND hWnd);
void del_list_value(view_dyn * inst, t_ctrl * itm, uns16 index);
extern t_tb_descr toolbar_query[];
void fill_back(WORD startoff, WORD stopoff, view_dyn *inst, t_ctrl * itm);
void enter_breaked_state(cdp_t cdp);
void unpack_params(void);
void bind_info(uns16 cnt);
void ResetRtfControl(view_dyn * inst, t_ctrl * itm, HWND hCtrlWnd);
BOOL grow_object(indir_obj * iobj, UINT size);
DllPrezen void WINAPI Signature_support(HWND hWnd, UINT msg, int sigstyle);
DllPrezen void WINAPI transform_coords(view_stat * stat, BOOL from_source_to_window, BOOL designer);

DllPrezen BOOL WINAPI register_synchro(cdp_t cdp, HWND hWndFrom, HWND hWndTo, UINT ctId,
   tptr dest_expr, int owner_offset, long synchStyle);
void unregister_synchro(cdp_t cdp, HWND hWnd);
void synchronize_views(cdp_t cdp, HWND hWnd, BOOL hard);
BOOL is_rel_synchronized(cdp_t cdp, HWND hWnd);
DllPrezen void WINAPI  resize_dlg_item(HWND hDlg, UINT ID);
DllPrezen int WINAPI  default_precision(int tp);
extern char WBCOMBO_LIST_NAME[];
LRESULT CALLBACK WBComboListWndProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP);
DllPrezen int WINAPI DTP_format_style(int type, int precision);
DllPrezen const char * WINAPI DTP_format_string(int type, int precision);
BOOL relation_in_cursors(cdp_t cdp, tobjnum relobj, tcursnum curs1, tcursnum curs2, 
                         tattrib * atrs1, tattrib * atrs2, uns8 * types1, uns8 * types2, t_specif * specifs1, t_specif * specifs2);


//void  DelRaster(HWND hWnd, curpos *wbacc, view_dyn *inst);
//void  LoadRaster(HWND hWnd, curpos *wbacc, view_dyn *inst);
//void  SaveRaster(HWND hWnd, curpos *wbacc);
//void  PasteRaster(HWND hWnd, curpos *wbacc);
//void  CopyRaster(HWND hWnd, curpos *wbacc);
void  PrintRaster(CPrintEnv *penv, curpos* wbacc, WORD precision, LPRECT Dest, BOOL cached);
void  UpdateRasters(HWND hWnd, HWND hOwner);

extern char RASTER_EXT[11];
extern char RASTER_PAL[11];
extern UINT WBrecords;

void GetFrameAndChild(HWND *lpFrame, HWND *lpChild);
HWND GetToolBar();
BOOL IsInPlace(HWND hWnd);
BOOL IsOleServer(void);
BOOL SetInPlaceActWnd(HWND hQBEWnd, t_tb_descr *tb_descr);
void Add_tool_tip(HWND hWnd, view_dyn *inst, HWND hCtrl, t_ctrl *itm);
void create_control(HWND hWnd, view_dyn * inst, t_ctrl * itm, UINT ypos, UINT r, int num, UINT xdisp);
int div_line_width(view_dyn * inst);

DllPrezen HWND WINAPI CreatOleControl(LPWBOLECLDOC DocPtr, int xw, int yw, int ww, int hw, t_ctrl * itm, unsigned nID);
DllPrezen LPWBOLECLDOC WINAPI RegOleClDoc(HWND ViewHndl);
DllPrezen BOOL WINAPI CancOleClDoc(LPWBOLECLDOC DocPtr);
DllPrezen void WINAPI ResetOleControl(HWND hWnd);
DllPrezen BOOL WINAPI PrintOleControl(LPWBOLECLDOC DocPtr, view_dyn* inst, t_ctrl* itm, trecnum recno, HDC mDC, HDC pDC, LPRECT pRect);
DllPrezen HMETAFILE WINAPI GetViewMF(LPSTR viewsrc, HWND hWnd, trecnum Pos, UINT vwCX, UINT vwCY);
DllPrezen BOOL WINAPI cd_LoadGraph(cdp_t cdp, char *Name, LPSTR *grDef, t_varcol_size *grSize, tobjnum *grObj, LPBYTE ApplID = NULL);
DllPrezen void WINAPI CloseGraph(HWND ctrl);
DllExport void WINAPI Set_tool_bar(cdp_t cdp, t_tb_descr * tb_descr, HWND hParent = 0);

DllPrezen HWND WINAPI CreateToolBarWindow(int Width, HWND hFrameWnd, HINSTANCE hInstance);
DllPrezen void WINAPI UpdateToolBar(t_tb_descr * tb_descr, HWND hParent = 0);
DllPrezen void WINAPI ReleaseToolBar(t_tb_descr *tb, HWND hParent = 0);
DllPrezen int  WINAPI ResizeToolBar(int Width, HWND hParent = 0);
DllPrezen UINT WINAPI OnToolbar(LPNMHDR nmhdr);

DllPrezen BOOL WINAPI RemDBContainer(void);
DllPrezen void WINAPI CreateViewDataObj(LPSTR name, tobjnum ObjNum, tcateg Categ, BOOL Drag);

BOOL CreateActiveXItem(HWND hView, t_ctrl * itm, int x, int y, UINT ID);
void ResetActiveXItem(HWND hView, UINT ID);
void DestroyActiveXItem(HWND hView, UINT ID);
void CommitActiveXItems(view_dyn *inst);
DllPrezen BOOL CallActiveXMethod(HWND hView, UINT ID, DWORD Method, untstr *pRes, int *Pars);

#define RADIO_FIRST   1
#define RADIO_PREV    2
#define RADIO_NEXT    3
#define RADIO_LAST    4
#define RADIO_CHECKED 5

t_ctrl *NextRadio(view_dyn *inst, t_ctrl *CurItm, WORD Which);

DllPrezen BOOL WINAPI  get_new_name(cdp_t cdp, HWND hWndParent, tcateg categ, const char * prompt, char * edt_name);

LRESULT DefViewProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP);

DllPrezen void WINAPI draw_check(DRAWITEMSTRUCT * di, int offset, BOOL on);
DllPrezen UINT WINAPI locate_item(HWND hDlg, int * index, int * xoff);
#define LBCHECK_CX  18
#define LBCHECK_CY  14
#define LBCHECK_EXT  8
DllPrezen BOOL WINAPI User_name_by_id(cdp_t cdp, uns8* user_uuid, char * namebuf);
DllPrezen BOOL WINAPI Store_view_info(view_stat * vdef, tobjnum viewobj);

#endif //SRVR

#define COMBO_TRANSL_TYPE(tp) ((tp)==ATT_INT32 || (tp)==ATT_INT16 || (tp)==ATT_INT8 || (tp)==ATT_CHAR)

#define STATUS_BAR_ID  2
#define TOOL_BAR_ID    0xfffe  // common for frame window, forms, menu designer
void show_certif_path(cdp_t cdp, HWND hParent, ttablenum tb, trecnum rec, tattrib atr);
typedef void WINAPI t_PanelOper(tcateg cat, tobjnum objnum, ctptr name, uns16 flags, uns32 modtime, int oper);
#define PO_ADD  0
#define PO_SEL  1
#define PO_DEL  2
#define PO_CHNG 3
#define PO_LADD 4
DllPrezen void WINAPI Select_component(cdp_t cdp, tcateg categ, tobjnum objnum);
DllPrezen int  WINAPI Signature_state(cdp_t cdp, HWND hView, trecnum irec, const char * attrname);
DllPrezen void WINAPI SetPanelCallBack(t_PanelOper * aPanelOper);
DllPrezen void WINAPI Delete_component(cdp_t cdp, tcateg cat, ctptr name, tobjnum objnum);
DllPrezen tobjnum WINAPI Select_object(cdp_t cdp, HWND hParent, tcateg categ);

DllPrezen void WINAPI Set_current_folder(cdp_t cdp, t_folder_index folder);
DllPrezen t_folder_index WINAPI Current_folder(cdp_t cdp);
DllPrezen BOOL WINAPI Delete_folder(cdp_t cdp, t_folder_index folder_indx);
DllPrezen BOOL Set_object_folder(cdp_t cdp, tobjnum objnum, tcateg categ, t_folder_index folder_indx);
DllPrezen void Set_object_time_and_folder(cdp_t cdp, tobjnum objnum, tcateg categ, const char * folder_name, uns32 stamp);
DllPrezen void Get_object_folder_name(cdp_t cdp, tobjnum objnum, tcateg categ, char * folder_name);
DllPrezen BOOL Get_object_modif_time(cdp_t cdp, tobjnum objnum, tcateg categ, uns32 * ts);
//#define OBJECT_DESCRIPTOR_SIZE 46 -- in wbapiex.h
DllPrezen int  WINAPI make_object_descriptor(cdp_t cdp, tcateg categ, tobjnum objnum, char * buf);
//DllPrezen BOOL WINAPI get_object_descriptor_data(const char * buf, tobjname folder_name, uns32 * stamp); -- in wbapiex.h

DllPrezen void WINAPI write_dest_ds(cdp_t cdp, tptr dsn);
DllPrezen tptr WINAPI load_and_check_tab_def(cdp_t cdp, HANDLE hFile, char * tabname);
DllPrezen tobjnum WINAPI store_odbc_link(cdp_t cdp, tptr objname, t_odbc_link * olnk, BOOL select, BOOL limited);
DllPrezen BOOL WINAPI Add_relation(cdp_t cdp, char * relname, char * tab1name, char * tab2name,
                  const char * atr1name, const char * atr2name, BOOL select);
DllPrezen uns32 WINAPI PrezGetVersion(void);

 /* import export designer */
DllPrezen void WINAPI edit_impexp(cdp_t cdp, HWND hParent, tobjnum objnum, int init_tp, tobjnum init_obj, const char * name=NULL);
extern unsigned text_save_buttons[3];
 // designer stack:
DllPrezen void WINAPI dsgn_stk_push(tcateg category, tobjnum objnum, tobjname name);
DllPrezen BOOL WINAPI dsgn_stk_last(tcateg & category, tobjnum & objnum, tobjname name);
DllPrezen BOOL WINAPI dsgn_stk_pop(tcateg & category, tobjnum & objnum, tobjname name);
DllPrezen BOOL WINAPI dsgn_stk_empty(void);
DllPrezen void WINAPI dsgn_stk_clear(void);

// lnk:
DllPrezen BOOL WINAPI lnk_Edit_view(cdp_t cdp, const char * name);
DllPrezen BOOL WINAPI lnk_Edit_query(cdp_t cdp, const char * name);
DllPrezen BOOL WINAPI lnk_Edit_table(cdp_t cdp, HWND hParent, const char * name, tobjnum objnum);
DllPrezen BOOL WINAPI lnk_Export_exx(cdp_t cdp, HWND hParent);
DllPrezen BOOL WINAPI lnk_Export_appl_ex(cdp_t cdp, HWND hParent,
   BOOL with_data, BOOL with_role_privils, BOOL with_usergrp, BOOL exp_locked, BOOL back_end);
DllPrezen BOOL WINAPI lnk_Aexport_appl(cdp_t cdp, HWND hParent);
DllPrezen BOOL WINAPI lnk_Create_local_database(HWND hParent);

#if WBVERS<=810
DllPrezen int   WINAPI lnk_modify_www_object(cdp_t cdp,tobjnum objnum,HWND hParentWnd,uns8 flags);
DllPrezen int   WINAPI lnk_get_www_object_type(cdp_t cdp,tobjnum objnum);
DllPrezen DWORD WINAPI lnk_get_wbcedit_version(void);
DllPrezen BOOL CALLBACK lnk_WWWParamDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
DllPrezen HINSTANCE WINAPI lnk_get_wbcedit_hinstance(void);
DllPrezen int   WINAPI lnk_get_child_www_objects(cdp_t cdp,tobjnum obj_parent,uns8 flag_parent,WWWChildDrawingData **pobj_child);
DllPrezen void  WINAPI lnk_destroy_child_www_objects(WWWChildDrawingData *pobj_child);
DllPrezen int   WINAPI lnk_get_new_www_object_type(HWND hParentWnd);
#endif
DllPrezen BOOL  WINAPI IsViewChanged(HWND hView);
DllPrezen uns32 WINAPI lnk_DvlpGetVersion(void);
DllViewed uns32 WINAPI DvlpGetVersion(void);
// XML extender link:
DllPrezen int   WINAPI lnk_Edit_XML_mapping(cdp_t cdp,tobjnum objnum,HWND hParentWnd,uns8 flags);
DllPrezen BOOL  WINAPI lnk_Export_to_XML(cdp_t cdp, const char * dad_ref, char * fname, tcursnum curs, void * hostvars, int hostvarscount);
DllPrezen BOOL  WINAPI lnk_Import_from_XML(cdp_t cdp, const char * dad_ref, const char * fname, struct t_clivar * hostvars, int hostvars_count);
DllPrezen BOOL  WINAPI lnk_Export_to_XML_buffer(cdp_t cdp, const char * dad_ref, char * buffer, int bufsize, int * xmlsize, tcursnum curs, void * hostvars, int hostvarscount);
DllPrezen BOOL  WINAPI lnk_Import_from_XML_buffer(cdp_t cdp, const char * dad_ref, const char * buffer, int xmlsize, struct t_clivar * hostvars, int hostvars_count);
DllPrezen BOOL  WINAPI lnk_Verify_DAD(cdp_t cdp, const char * dad, int * line, int * column);
DllPrezen uns32 WINAPI lnk_XMLGetVersion(void);

DllPrezen void WINAPI Show_stored_query_optim(cdp_t cdp, HWND hParent, tobjnum objnum);
DllPrezen void WINAPI Show_edited_query_optim(cdp_t cdp, HWND hParent, char * source, bool release_source);

DllPrezen void WINAPI cpy_full_attr_name(char quote_char, tptr dest, const d_table * td, unsigned num);
DllPrezen BOOL WINAPI catr_is_null(tptr vals, atr_info * catr);
DllKernel BOOL WINAPI Move_data(cdp_t cdp, tobjnum move_descr_obj, const char * inpname, tobjnum inpobj, const char * outname, int inpformat, int outformat, int inpcode, int outcode, int silent);
/* Typ preview */
#define PREV_CHILD   1
#define PREV_POPUP   2

#ifdef __cplusplus
 } /* of extern "C" */
#endif

BOOL delete_odbc_record(view_dyn * inst, trecnum extrec, BOOL on_current);

void init_cache_dynars(ltable * dt);

#include "cache9.h"
