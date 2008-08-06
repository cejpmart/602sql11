// viewwiz.h - Definition for the view wizard

#define VIEWTYPE_HORIZ   0    /* horizontal view */
#define VIEWTYPE_VERT    1    /* vertical view */
#define VIEWTYPE_PLAIN   2    /* plain view */
// in wprez.h: #define VIEWTYPE_SIMPLE  3    /* standard view */
#define VIEWTYPE_LABEL   4
#define VIEWTYPE_GRAPH   5
#define VIEWTYPE_MASK    7
// in wprez.h: #define VIEWTYPE_DEL     8    /* + include "DELETED" attribute */
#define VIEWTYPE_STATIC 128   /* use non-editable items */
#define VIEWTYPE_GROUPS  64
//#define VIEWTYPE_SUBVIEW 32

#define MAX_CAPT_DEF  (ATTRNAMELEN+1) // column name + ":" 

struct view_specif;

struct t_sel_attr  // item description
{ tattrib attrnum;
  char caption[MAX_CAPT_DEF+1];
  uns8 mainview;   /* means datapane */
  BOOL query;
  uns8 groupdef;
  uns8 A_head;
  uns8 A_foot;
  uns8 B_head;
  uns8 B_foot;
  tobjnum cursobj; // translation cursor number - not used any more
  int xpos;        /* auxiliary variable used to align columns */
  char * combo_query;
  char * helptext;
  char * oleserver;
  void init_attr(int i, const d_table * td, view_specif * pvs, uns8 Ini = 2);
  void release(void)
  { corefree(combo_query);  corefree(helptext);  corefree(oleserver);
    combo_query=helptext=oleserver=NULL;
  }
};

CFNC DllPrezen void WINAPI define_from_extended_info(t_sel_attr * sa, ltable * extinfo);

SPECIF_DYNAR(selatr_dynar, t_sel_attr, 5);

#define MAX_VIEW_CAPTION  100
#define MAX_GROUP_DEF      69

#define VWZ_FLG_SELFORM      0x00000001
#define VWZ_FLG_INPFORM      0x00000002
#define VWZ_FLG_GENFORM      0x00000004
#define VWZ_FLG_MOVE         0x00000008
#define VWZ_FLG_MOVE_BUT     0x00000010
#define VWZ_FLG_MOVE_SCROLL  0x00000020
#define VWZ_FLG_INSERT       0x00000040
#define VWZ_FLG_INSERT_BUT   0x00000080
#define VWZ_FLG_INSERT_FICT  0x00000100
#define VWZ_FLG_DEL          0x00000200
#define VWZ_FLG_DEL_BUT      0x00000400
#define VWZ_FLG_DEL_ASK      0x00000800
#define VWZ_FLG_EDIT         0x00001000
#define VWZ_FLG_EDIT_BUT     0x00002000
#define VWZ_FLG_QUERY        0x00004000
#define VWZ_FLG_QUERY_BUT    0x00008000
#define VWZ_FLG_QUERY_MENU   0x00010000

struct view_specif
{ int      vw_type;
  tcateg   basecateg;
#if WBVERS<900
  tobjname basename;
#else
  char basename[MAX_OBJECT_NAME_LEN+1];
#endif
  tobjnum  basenum;
  tobjname vw_template;
  tobjname project;
  char     caption[MAX_VIEW_CAPTION+1];
  char     caption2[MAX_VIEW_CAPTION+1];
  BOOL     multirec;
  char groupA[MAX_GROUP_DEF+1], groupB[MAX_GROUP_DEF+1];
  unsigned vwz_flg;
    WORD vwXX;
    WORD vwYY;
    WORD Xspace;
    WORD Yspace;
    BYTE Xcnt;
    BYTE Ycnt;
    WORD Xmarg;
    WORD Ymarg;
  //int Style, HdftStyle, Style3; // additional info from the template
  bool editable_data_source;
  selatr_dynar selatr;
  ltable * ext_col_info_dt;

  void init(void)
  { vw_type=VIEWTYPE_PLAIN;
    basecateg=CATEG_TABLE;  *basename=0;  basenum=NOOBJECT;
    *vw_template=*caption=*caption2=*project=0;  multirec=FALSE;
    *groupA=*groupB=0;  
    vwz_flg=VWZ_FLG_GENFORM | VWZ_FLG_MOVE | VWZ_FLG_MOVE_SCROLL | VWZ_FLG_INSERT |
      VWZ_FLG_INSERT_FICT  | VWZ_FLG_DEL          | VWZ_FLG_DEL_ASK      | VWZ_FLG_EDIT |
      VWZ_FLG_QUERY        | VWZ_FLG_QUERY_MENU;
    vwXX=384;
    vwYY=135;
    Xspace=0;
    Yspace=9;
    Xcnt=1;
    Ycnt=8;
    Xmarg=0;
    Ymarg=0;
    editable_data_source=true;
    ext_col_info_dt=NULL;
  }
  void release_selatr(void)
  { for (int i=0;  i<selatr.count();  i++)
      selatr.acc0(i)->release();
    selatr.selatr_dynar::~selatr_dynar();
  }
  view_specif(void)
  { init(); }
  ~view_specif(void)
  { if (ext_col_info_dt) destroy_cached_access(ext_col_info_dt);  // must not use delete ext_col_info_dt, ltable must be deallocated in the module which allocated it
    release_selatr();
  }
  void Reset(void)
  { if (ext_col_info_dt) 
    { destroy_cached_access(ext_col_info_dt);  // must not use delete ext_col_info_dt, ltable must be deallocated in the module which allocated it
      ext_col_info_dt=NULL; 
    }
    release_selatr();
    init();
  }
};

CFNC DllPrezen void WINAPI create_extended_column_info(cdp_t cdp, view_specif * vs, const d_table * td);

extern view_specif view_spec;

#define SCVI_NOITEMS  0
#define SCVI_ALLVALS  1
#define SCVI_ALLEDITS 2

CFNC DllPrezen tptr WINAPI specific_view(cdp_t cdp, view_specif * vs, HWND hWnd);

