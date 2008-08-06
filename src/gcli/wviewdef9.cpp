/****************************************************************************/
/* View definition editor                                                   */
/****************************************************************************/
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
#include "compil.h"   /* only sometimes needed */
#include "viewwiz.h"
#include "wx/dc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#define START_X_POS          4  /* X starting position of items in a view */
#define START_Y_POS          4  /* Y upper margin in form & plain types */
#define X_WRITE_OFF          4  /* X displacement when writing value */
#define Y_EDIT_FRAME         3  /* space above & below text in edit control */
#define X_SEPAR              4  /* item separating space in plain type */
#define Y_SEPAR              6  /* item separating space in form & plain types */
#define POS_UNINIT  0xffffffff  /* initial value for xpos_a & xpos_b */
#define BIG_KOEF             4  /* Y enlargement of picture & text items */
#define CAPT_Y_SHIFT         4  /* shift down of the caption to align with edit */
#define SIMPLE_CAPT_MARG     4  /* additional margin in the column caption */

const char * short_class_names[] = { "Val", "String", "Edit", "Push", "Combo", "List", "Check", "Radio",
  "Text", "Raster", "Frame", "Ole", "View", "EdiCombo", "Rtf", "Slider", "Updown", "Progress", 
  "Signat", "Tab", "ActX", "Calendar", "Dtp", "Barcode" };

CFNC DllPrezen void WINAPI make_control_name(cdp_t cdp, char * buf, const char * name, int ClassNum, int ID)
{ strcpy(buf, name);
  if (cdp) ToASCII(buf, cdp->sys_charset);
  tptr p=buf+strlen(buf);  
  if (*name) { *p='_';  p++; }
  strcpy(p, short_class_names[ClassNum]);  
  char smallbuf[10];  int2str(ID, smallbuf);
  int len=(int)strlen(buf), slen=(int)strlen(smallbuf);
  if (len+slen>NAMELEN) len=NAMELEN-slen;
  strcpy(buf+len, smallbuf);  Upcase(buf);
}

#if 0
CFNC DllPrezen view_stat * WINAPI load_template(cdp_t cdp, const char * template_name)
{ 
  tobjnum templ_obj;  tptr basename;  tobjnum basenum;  tcateg cat;  BOOL dflt_view=FALSE;
  if (cd_Find_object(cdp, template_name, CATEG_VIEW, &templ_obj)) return NULL;
  tptr def=cd_Load_objdef(cdp, templ_obj, CATEG_VIEW);
  if (def==NULL) return NULL;
  if (!analyze_base(cdp, def, &basename, &basenum, &cat, &dflt_view))
    { corefree(basename);  corefree(def);  return NULL; }
  kont_descr compil_kont;
  compil_kont.kontcateg=cat;
  compil_kont.kontobj=basenum;
  compil_kont.next_kont=NULL;
  compil_info xCI(cdp, def, MEMORY_OUT, view_def_comp); // I need the types!
  xCI.kntx=&compil_kont;
  xCI.translation_disabled=wbtrue;
  xCI.comp_flags|=COMP_FLAG_NO_COLOR_TRANSLATE;
  int res=compile(&xCI);
  corefree(def);
  if (res) return NULL;
  return (view_stat*)xCI.univ_code;
  return NULL;  
}
#endif

CFNC DllPrezen t_ctrl * WINAPI find_item_by_class(view_stat * vdef, int ClassNum, int tp)
{ int i;  t_ctrl * itm, * an_itm=NULL;
  for (i=0, itm=FIRST_CONTROL(vdef);  i<vdef->vwItemCount;  i++, itm=NEXT_CONTROL(itm))
    if (itm->ctClassNum==ClassNum) 
      if (itm->type==tp) return itm;
      else if (!an_itm) an_itm=itm;
  return an_itm;
}

CFNC DllPrezen void WINAPI write_font(t_fontdef * font, char * dest)
{ sprintf(dest, "FONT %u %u %u \"%s\" %s%s%s%s\r\n",
    font->size, font->charset, font->pitchfam, font->name,
           (font->flags & SFNT_BOLD)  ? "BOLD "       : NULLSTRING,
           (font->flags & SFNT_ITAL)  ? "ITALICS "    : NULLSTRING,
           (font->flags & SFNT_UNDER) ? "UNDERLINED " : NULLSTRING,
           (font->flags & SFNT_STRIKE)? "STRIKEOUT"   : NULLSTRING);
}

#if 0
CFNC DllPrezen void WINAPI write_toolbar_source(cdp_t cdp, t_tb_descr * tbd, t_flstr * src)
{ tobjname buf;
  if (!tbd) return;
  src->put("TOOLBAR\r\n");
  while (!(tbd->flags & TBFLAG_STOP))
  { src->putc(' ');     /* indent */
    if (tbd->flags & TBFLAG_PRIVATE)
    { if (cd_Read(cdp, OBJ_TABLENUM, (trecnum)tbd->bitmap, OBJ_NAME_ATR, NULL, buf) || !*buf)
      { goto skip_button; }
      /* otherwise it would create syntax error in view def. */
      src->putq(buf);
    }
    else
      { sprintf(buf, "%u", tbd->bitmap);  src->put(buf); }
    src->putint((unsigned)(tbd->flags & TBFLAG_PRIVATE ? 0 : tbd->msg));
    src->putint(tbd->offset);
    src->putc(' ');
    if (tbd->flags & TBFLAG_COMMAND)  src->put("COMMAND ");
    if (tbd->flags & TBFLAG_FRAMEMSG) src->put("FRAME ");
    if (tbd->flags & TBFLAG_RIGHT)    src->put("RIGHT ");
    src->putss(tbd->txt);
    src->put(" _STMT_START ");
    src->put(tbd->txt+strlen(tbd->txt)+1);
    src->put(" _STMT_END\r\n");
   skip_button:
    tbd=NEXT_TB_BUTTON(tbd);
  }
  src->put("END\r\n");
}
#endif

CFNC DllPrezen int WINAPI default_precision(int tp)
{ 
#if 0
  prez_params * pp = GetPP();
  switch (tp)
  { case ATT_FLOAT:  return pp->view_prec_real;
    case ATT_MONEY:  return pp->view_prec_money;
    case ATT_DATE:   return pp->view_prec_date;
    case ATT_TIME:   return pp->view_prec_time;
    case ATT_RASTER: return pp->view_prec_raster;
    case ATT_NOSPEC: return pp->view_prec_ole;
    case ATT_SIGNAT: return SIGNATURE_SIGN | SIGNATURE_CHECK | SIGNATURE_CLEAR;
  }
#endif  
  if (tp==ATT_FLOAT) return format_real;
  return 5;
}

static const uns8 szoftp[ATT_LASTSPEC+1] =
{ 0,4,3,6,12,12,12, /*-,wbbool,char,16,32,money,float*/
  20,20,20,20,      /*strings, binary*/
  10,12,23,17,17,   /*date,time,datim,ptrs*/
  14,17,13          /*autor,datim,hist*/ };

static void add_pane(tptr p, int pn)
{ if (pn==PANE_DATA) { *p=0;  return; }
  strcpy(p, " pane ");  p+=6;
  switch (pn)
  { case PANE_PAGESTART:   strcpy(p, "pageheader");    break;
    case PANE_REPORTSTART: strcpy(p, "reportheader");  break;
    case PANE_PAGEEND:     strcpy(p, "pagefooter");    break;
    case PANE_REPORTEND:   strcpy(p, "reportfooter");  break;
    default: if (pn>PANE_DATA)
             { *p='e'-(pn-PANE_GROUPEND);    strcpy(p+1, "footer"); }
             else
             { *p='a'+(pn-PANE_GROUPSTART);  strcpy(p+1, "header"); }
             break;
  }
}

class view_generator
{ xcdp_t xcdp;
  cdp_t cdp;
  unsigned xpos, ypos;  // coordinates of the next control
  unsigned xpos_a, xpos_b, ypos_a, ypos_b;  // next small/big control coords, plain only
  unsigned ID;          // current ID
  unsigned AveCharWidth, itemheight, checkbox_width;  /* constant info */
  wxClientDC dc;  wxFont oldfont;
  tcateg basecat;  tobjnum base_num;
  unsigned xgrid, ygrid, width_limit;
  const char * baseprefix;
  t_flstr src;  char buf[140];

  UINT free_line_y(void);
  void init_coords(void);
  void insert_button(UINT butid, tptr actn);
  void generate_item(t_sel_attr * sa, int pn, BOOL is_query, int aggrs);
  view_stat * templ;
  wxFont * locfont;

 public:
  BOOL listtype, plaintype, formtype, simpleview, staticview,
       labelview, graphview, is_report, gray_background;
  const d_table * td;
  view_generator(xcdp_t xcdpIn, int ini_vwtype, tcateg ini_basecat, tobjnum ini_base_num, const char * tabname, const char * odbc_prefix);
  ~view_generator(void);
  tptr generate(int x_view_pos, int y_view_pos, view_specif * vs);
};

view_generator::view_generator(xcdp_t xcdpIn, int ini_vwtype, tcateg ini_basecat, tobjnum ini_base_num, const char * tabname, const char * odbc_prefix) 
               : src(10000,5000), dc(wxGetApp().frame)
{ xcdp=xcdpIn;
  cdp=xcdp->get_cdp();
  basecat=ini_basecat;  base_num=ini_base_num;
  baseprefix=odbc_prefix;
  listtype  =(ini_vwtype & VIEWTYPE_MASK) == VIEWTYPE_HORIZ;
  formtype  =(ini_vwtype & VIEWTYPE_MASK) == VIEWTYPE_VERT;
  plaintype =(ini_vwtype & VIEWTYPE_MASK) == VIEWTYPE_PLAIN;
  simpleview=(ini_vwtype & VIEWTYPE_MASK) == VIEWTYPE_SIMPLE;
  labelview =(ini_vwtype & VIEWTYPE_MASK) == VIEWTYPE_LABEL;
  graphview =(ini_vwtype & VIEWTYPE_MASK) == VIEWTYPE_GRAPH;
  staticview=(ini_vwtype & VIEWTYPE_STATIC ) != 0;
  is_report =(ini_vwtype & VIEWTYPE_GROUPS ) != 0;
  //cre_subvw =(ini_vwtype & VIEWTYPE_SUBVIEW) != 0;
  locfont = new wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, FALSE);
  oldfont = dc.GetFont();
  dc.SetFont(*locfont);
  { wxCoord cx, cy;
    dc.GetTextExtent(wxT("Mys"), &cx, &cy);
    itemheight   = cy+2*Y_EDIT_FRAME;
    AveCharWidth = cx/3;
    dc.GetTextExtent(wxT("MM"), &cx, &cy);
    checkbox_width=cx;
  }
  if (labelview) plaintype=TRUE;
  gray_background = (plaintype || listtype || formtype) && !labelview && !is_report;

  if (basecat!=CATEG_VIEW && !graphview)
  { if (cdp)
    { td=cd_get_table_d(cdp, base_num, basecat);
      if (td==NULL) cd_Signalize(cdp);
    }
    else // ODBC
    { if (basecat==CATEG_DIRCUR)
        td=make_result_descriptor9(xcdp->get_odbcconn(), (HSTMT)(size_t)base_num);
      else
        td=make_odbc_descriptor9(xcdp->get_odbcconn(), tabname, odbc_prefix, TRUE);
    }
    if (td && !(td->updatable & QE_IS_UPDATABLE) || labelview) staticview=TRUE;
  }
  else td=NULL;
#if 0
  xgrid=GetPP()->view_xraster;  ygrid=GetPP()->view_yraster;
#else
  xgrid=ygrid=0;
#endif
  if (!xgrid || xgrid > 4) xgrid=4;  if (!ygrid || ygrid > 4) ygrid=4;
  templ=NULL;
}

view_generator::~view_generator(void)
{
  if (td!=NULL) 
    if (cdp)
      release_table_d(td);
    else
      corefree(td);
  dc.SetFont(oldfont);
  delete locfont;
  corefree(templ);
}

void view_generator::init_coords(void)
{ ypos=(listtype || simpleview) ? 0 : START_Y_POS;
  xpos=simpleview ? 0 : START_X_POS;
  xpos_a=xpos_b=POS_UNINIT;  ypos_a=ypos_b=0;  /* no need, prevents warning */
}

UINT view_generator::free_line_y(void)
// Returns y-coordinate of a next free line
// If result assigned to ypos_n, xpos_n must be asigned too!
{ UINT nx, fline = START_Y_POS;
  if (xpos_a!=POS_UNINIT)
  { nx=ypos_a+itemheight+Y_SEPAR;
    if (nx>fline) fline=nx;
  }
  if (xpos_b!=POS_UNINIT)
  { nx=ypos_b+BIG_KOEF*itemheight+Y_SEPAR;
    if (nx>fline) fline=nx;
  }
  return fline;
}

void view_generator::insert_button(UINT butid, tptr actn)
{ BOOL std = butid < 30;
  src.put("PUSHBUTTON \"");
#if 0
  if (std) { buf[0]=buf[1]='@';  int2str(butid, buf+2); }
  else LoadString(hPrezInst, butid, buf, sizeof(buf));
  src.put(buf);
  sprintf(buf, "\" %d %d %d %d %d %lu ACTION _ %s _ PANE pagefooter\r\n",
    ID++, xpos, ypos, std ? 24 : 112, 24, WS_TABSTOP | WS_BORDER, actn);  src.put(buf);
  xpos+=std ? 28 : 120;
#endif
}

void qstrcpy(char * dest, const char * src)
{ do
  { *dest=*src;  dest++;
    if (*src=='\'' || *src=='"') *(dest++)=*src;
  } while (*(src++));
}

tptr view_generator::generate(int x_view_pos, int y_view_pos, view_specif * vs)
{ unsigned winsize_pos;  int i;  t_sel_attr * sa;
  width_limit = labelview ? vs->vwXX+4 : 600; // used by plain format
#if 0
 // template:
  if (*vs->vw_template)
    templ=load_template(cdp, vs->vw_template);
#endif
 /* base: */
  if (basecat==CATEG_VIEW) src.put("a DBDIALOG");
  else
  { 
#if 0
    if (IS_ODBC_TABLEC(basecat, base_num))  /* user 1st part of the name only */
      src.putq(get_odbc_tc(cdp, base_num)->s.odbc_link->tabname);
    else 
#endif
    if (!cdp)
    { if (*baseprefix)
      { src.putq(baseprefix);
        src.putc('.');
      }
      src.putq(vs->basename);
    }
    else if (specname(cdp, vs->basename) && basecat!=CATEG_DIRCUR && *vs->basename!='`')
      src.putq(vs->basename);
    else 
      src.put(vs->basename);
    src.put(basecat==CATEG_TABLE ? " TABLEVIEW" : " CURSORVIEW");
  }
  sprintf(buf, " %d %d ", x_view_pos, y_view_pos);  src.put(buf);
  winsize_pos=src.getlen();  src.put("          \r\n");

 // project & template:
  if (*vs->project)
    { sprintf(buf, "_PROJECT_NAME `%s`\r\n",  vs->project);      src.put(buf); }
  if (*vs->vw_template)
    { sprintf(buf, "_TEMPLATE_NAME `%s`\r\n", vs->vw_template);  src.put(buf); }

 /* styles: */
  if (basecat==CATEG_VIEW) vs->vwz_flg=VWZ_FLG_EDIT; // no insert, delete, move, query
  LONG style     = SUPPRESS_BACK;
  LONG hdftstyle = 0;
  LONG style3    = 0;
  if (simpleview)      style |= SIMPLE_VIEW;    // nebrat ze sablony, muze byt predefinovano ve wizardu
  if (vs->multirec)    style |= REP_VW_STYLE_0; // nebrat ze sablony, muze byt predefinovano ve wizardu
  if (graphview)       hdftstyle |= VIEW_IS_GRAPH | STD_SYSMENU;
  if (is_report)       style3 |= VIEW_IS_REPORT;
  if (vs->vwz_flg & VWZ_FLG_INPFORM) // no move, delete, query
  { hdftstyle |= FORM_OPERATION;  
    vs->vwz_flg = VWZ_FLG_INPFORM | VWZ_FLG_INSERT | VWZ_FLG_INSERT_FICT | VWZ_FLG_INSERT_BUT | VWZ_FLG_EDIT;
  }
  if (vs->vwz_flg & VWZ_FLG_SELFORM) // no insert, delete, edit
  { style3 |= REC_SELECT_FORM; 
    vs->vwz_flg = VWZ_FLG_SELFORM | VWZ_FLG_MOVE | VWZ_FLG_MOVE_SCROLL | VWZ_FLG_QUERY | VWZ_FLG_QUERY_MENU;
  }
  if (vs->vwz_flg & VWZ_FLG_MOVE)
    { if (!(vs->vwz_flg & VWZ_FLG_MOVE_SCROLL)) hdftstyle |= NO_VERT_SCROLL; }
  else { style|=NO_VIEW_MOVE;  hdftstyle |= NO_VERT_SCROLL; }
  if (vs->vwz_flg & VWZ_FLG_INSERT)
    { if (!(vs->vwz_flg & VWZ_FLG_INSERT_FICT)) hdftstyle |= NO_VIEW_FIC; }
  else hdftstyle |= NO_VIEW_INS | NO_VIEW_FIC;
  if (vs->vwz_flg & VWZ_FLG_DEL)
    { if (vs->vwz_flg & VWZ_FLG_DEL_ASK) style|=QUERY_DEL; }
  else style|=NO_VIEW_DELETE;
  if (!(vs->vwz_flg & VWZ_FLG_EDIT)) style|=NO_VIEW_EDIT;
  if (vs->vwz_flg & VWZ_FLG_QUERY) 
    { if (!(vs->vwz_flg & VWZ_FLG_QUERY_MENU)) hdftstyle |= STD_SYSMENU; }
  else { style3|=QBE_DISABLED;  hdftstyle |= STD_SYSMENU; }
 // template-dependent global info:
  if (templ) hdftstyle |= templ->vwHdFt & 
    ( MAX_DISABLED | MIN_DISABLED | VIEW_3D | CLOSE_DISABLED | REP_FLOATINGPGFT | REP_HDMASK | REP_HDBEFPG |
      REP_HDAFTPG | REP_HDNOPG | REP_FTMASK | REP_FTBEFPG | REP_FTAFTPG | REP_FTNOPG |
      VW_FIXED_SIZE | REC_SYNCHRO | DEACT_SYNCHRO | ASK_SYNCHRO | NO_FAST_SORT);
  else
  { if (gray_background) hdftstyle |= VIEW_3D;
    hdftstyle |= REC_SYNCHRO;
  }

  sprintf(buf, "STYLE %lu\r\n", style);  src.put(buf);
 // caption:
  src.put("CAPTION '");
  if (*vs->caption) 
    src.put(vs->caption);
  else if (basecat!=CATEG_VIEW)
  { const wxChar * capt;
#if 0
    if (graphview) 
      capt = basecat==CATEG_TABLE ? wxT("Graph from Table ") : wxT("Graf from Query ");
    else if (vs->vwz_flg & VWZ_FLG_INPFORM)  // input form
      capt = basecat==CATEG_TABLE ? wxT("Data Input into Table ") : wxT("Data Input into Query ");
    else
#endif
      capt = basecat==CATEG_TABLE ? _("Data from Table ") : _("Data from Query ");
    src.put(wxString(capt).mb_str(GetConv(cdp)));
    src.putc(' ');
    qstrcpy(buf, vs->basename);  
    if (cdp) wb_small_string(cdp, buf, true);  
    src.put(buf);
  }
  src.put("'\r\n");
 // label dimensions:
  if (labelview)
  { sprintf(buf, "LABEL %u %u %u %u %u %u %u %u\r\n", vs->vwXX, vs->vwYY, vs->Xspace, vs->Yspace, vs->Xcnt, vs->Ycnt, vs->Xmarg, vs->Ymarg);
    src.put(buf);
  }

  sprintf(buf, "HDFTSTYLE %lu\r\n", hdftstyle);  src.put(buf);
  sprintf(buf, "_STYLE3 %lu\r\n",   style3);     src.put(buf);

  if (templ)
  { 
#if 0
    if (SYSCOLORPART(templ->vwBackCol)!=COLOR_WINDOW)
#endif
      { sprintf(buf, "BACKGROUND *%lu\r\n", templ->vwBackCol);  src.put(buf); }
    t_fontdef * fd = (t_fontdef*)get_var_value(&templ->vwCodeSize, VW_FONT);
    if (fd) { write_font(fd, buf);  src.put(buf); }
  }
  else if (gray_background)
  { strcpy(buf, "BACKGROUND *281596116\r\n");  src.put(buf); } // COLOR_BTNFACE

  if (*vs->groupA)
    { sprintf(buf, "GROUP A %s\r\n", vs->groupA);  src.put(buf); }
  if (*vs->groupB)
    { sprintf(buf, "GROUP B %s\r\n", vs->groupB);  src.put(buf); }
 // toolbar, help:
  if (!(style3 & NO_TOOLBAR) && templ)
  { 
#if 0
    write_toolbar_source(cdp, (t_tb_descr*)get_var_value(&templ->vwCodeSize, VW_TOOLBAR), &src);
    if (templ->vwHelpInfo && templ->vwHelpInfo != HE_ANY_VIEW)
      { sprintf(buf, "HELP %u\r\n", templ->vwHelpInfo);  src.put(buf); }
#endif
  }
 // controls:
  src.put("BEGIN\r\n");
//  lineheight=0;
  ID=3;
  BOOL added_buttons = FALSE;
  if (td!=NULL)
  { if (plaintype)
    {/////////////////////// query fields: ///////////////////////////////
      init_coords();
      for (i=0;  i<vs->selatr.count();  i++)
      { sa = vs->selatr.acc(i);
        if (sa->attrnum && sa->query==1) generate_item(sa, PANE_PAGESTART, TRUE, 0);
      }
      if (plaintype && xpos!=START_X_POS)  /* any query field generated, add separator */
      { ypos=free_line_y();
        sprintf(buf, " LTEXT %d 0 %d 580 1 7 pane pageheader\r\n",
                   ID++, ypos);
        src.put(buf);
      }
    }
   /////////////////////////// report header & footer ////////////////////
#if 0
    if (is_report) // control width is small, width must be determined by data pane control
    {// report header:
      strcpy(buf, simpleview ? "LTEXT \"" : "CTEXT \"");  src.put(buf); // not centered in simple report because it can overflow
      strcpy(buf, *vs->caption ? vs->caption : wxT("Report header"));
      src.put(buf);
      sprintf(buf, "\" %d %d 4 %d 32 %u 0 pane reportheader font 16 0 34 \"Arial\" BOLD\r\n", ID++, simpleview ? 0 : 70, simpleview ? 20 : 410, WS_BORDER | (simpleview ? CAN_OVERFLOW : SS_CENTER));
      src.put(buf);
     // page header if specified:
      if (*vs->caption2)
      { sprintf(buf, "LTEXT \"%s\" %d %d 0 %d 24 %u 0 pane pageheader\r\n", vs->caption2, ID++, 0, simpleview ? 60 : 300, CAN_OVERFLOW);
        src.put(buf);
      }
     // default page footer:
      sprintf(buf, " LTEXT %d %d 2 %d 24 %u 0 pane pagefooter value \"", ID++, 0, simpleview ? 20 : 94, CAN_OVERFLOW);  src.put(buf);
      src.put(_("page "));
      strcpy(buf, "\"+Int2str(##)\r\n");  src.put(buf);
    }
#endif
   //////////////////////// data pane fields: ////////////////////////////
    init_coords();
    for (i=0;  i<vs->selatr.count();  i++)
    { sa = vs->selatr.acc0(i);
      if ((sa->attrnum || !i) && sa->mainview > 1) generate_item(sa, PANE_DATA, FALSE, 0);
      // problem: sa->attrnum is 0 for empty rows in view wizard and for the DELETED attrbute
      else sa->xpos=-1;
      if (listtype) // for plaintype generated elsewhere
        if (sa->attrnum && sa->query==1) generate_item(sa, PANE_PAGESTART, TRUE, 0);
    }
   // find space for subview/buttons:
    if (plaintype)
    { ypos=ypos_b=free_line_y();  xpos_b=8;   /* must update _a or _b: window height */
    }
    else if (listtype)
      ypos+=BIG_KOEF*itemheight+Y_SEPAR;
   // add subview item:
//    if (cre_subvw && vs->subviewname)
//    { sprintf(vo+pos,"SUBVIEW %d %d %d %d %d %lu DEFAULT \"%s\"\r\n",
//        ID++, 8, ypos, 540, 140,
//        WS_TABSTOP | WS_BORDER | INHERIT_CURS, vs->subviewname);
//      pos+=strlen(vo+pos);
//      ypos=ypos_b=ypos+140+Y_SEPAR;
//    }
#if 0
   //////////////////// add automatic buttons: //////////////////////////////
    int saved_xpos = xpos;  int saved_ypos = ypos;
    xpos=8;  ypos=4;
    if (vs->vwz_flg & VWZ_FLG_INPFORM)
    { insert_button(STDBUT_INSERT,   "ACTION_(20,'','','') _ ENABLED _ qbe_state(!!)=0 ");
      insert_button(STDBUT_INSCLOSE, "ACTION_(20,'','','');thisform.Close _ ENABLED _ qbe_state(!!)=0 ");
      insert_button(STDBUT_NOINSERT, "thisform.Close _ ENABLED _ qbe_state(!!)=0 ");
      insert_button(STDBUT_CLEAR,    "thisform.DelRec _ ENABLED _ qbe_state(!!)=0 ");
      added_buttons = TRUE;
    }
    else
    { if (vs->vwz_flg & VWZ_FLG_INSERT_BUT)
        insert_button(STDBUT_INSERT,   "thisform.Insert _ ENABLED _ qbe_state(!!)=0 ");
      if (vs->vwz_flg & VWZ_FLG_DEL_BUT)
        insert_button(STDBUT_DELETE,   "thisform.DelRec _ ENABLED _ qbe_state(!!)=0 ");
      if (vs->vwz_flg & VWZ_FLG_EDIT_BUT)
      { insert_button(STDBUT_OK,     "thisform.Close _ ENABLED _ qbe_state(!!)=0 ");
        insert_button(STDBUT_CANCEL, "thisform.Roll_back_view;thisform.Close _ ENABLED _ qbe_state(!!)=0 ");
      }
      if (vs->vwz_flg & VWZ_FLG_MOVE_BUT)
      { insert_button(7, "thisform.FirstRec ");
        insert_button(5, "thisform.PrevPage ");
        insert_button(3, "thisform.PrevRec ");
        insert_button(4, "thisform.NextRec ");
        insert_button(6, "thisform.NextPage ");
        insert_button(8, "thisform.LastRec ");
      }
      if (vs->vwz_flg & VWZ_FLG_QUERY_BUT)
      { insert_button(STDBUT_QBE,     "ACTION_(33,'','','') _ ENABLED _ qbe_state(!!)=0 ");
        insert_button(STDBUT_ORDER,   "ACTION_(34,'','','') _ ENABLED _ qbe_state(!!)=0 ");
        insert_button(STDBUT_UNLIMIT, "ACTION_(35,'','','')");
        insert_button(STDBUT_ACCEPT,  "ACTION_(36,'','','') _ ENABLED _ qbe_state(!!)<>0 ");
      }
      if (vs->vwz_flg & (VWZ_FLG_INSERT_BUT | STDBUT_DELETE | VWZ_FLG_EDIT_BUT | VWZ_FLG_QUERY_BUT | VWZ_FLG_MOVE_BUT))
        added_buttons = TRUE;
    }
    ypos=saved_ypos;
    if (saved_xpos > xpos) xpos=saved_xpos;
#endif
   ////////////////////// group headers and footers: /////////////////////////
    if (is_report)
    { int saved_xpos = xpos;
      init_coords();
      for (i=0;  i<vs->selatr.count();  i++)
      { sa = vs->selatr.acc(i);
        if (sa->attrnum && sa->A_head) generate_item(sa, PANE_GROUPSTART, FALSE, sa->A_head);
      }
      init_coords();
      for (i=0;  i<vs->selatr.count();  i++)
      { sa = vs->selatr.acc(i);
        if (sa->attrnum && sa->B_head) generate_item(sa, PANE_GROUPSTART+1, FALSE, sa->B_head);
      }
      init_coords();
      for (i=0;  i<vs->selatr.count();  i++)
      { sa = vs->selatr.acc(i);
        if (sa->attrnum && sa->A_foot) generate_item(sa, PANE_GROUPEND+4, FALSE, sa->A_foot);
      }
      init_coords();
      for (i=0;  i<vs->selatr.count();  i++)
      { sa = vs->selatr.acc(i);
        if (sa->attrnum && sa->B_foot) generate_item(sa, PANE_GROUPEND+3, FALSE, sa->B_foot);
      }
      if (saved_xpos > xpos) xpos=saved_xpos;
    } // headers & footers
  } // td OK
 /* view height, fictive column: */
  unsigned view_ysize, xsize;
  if (td==NULL || !vs->selatr.count())   /* no items generated */
  { view_ysize=270;   /* default ysize */
    if (simpleview)   /* add fictive column */
      { strcpy(buf, "LTEXT \"...\" 3 0 0 20 10 0 ");  src.put(buf); }
  }
  else if (listtype) view_ysize=270;
  else if (plaintype) 
  { view_ysize=free_line_y()+22+24+34;
    if (*vs->caption2) view_ysize+=24+4;
    if (added_buttons) view_ysize+=24+4;
  }
  else if (simpleview) view_ysize=250;
  else 
  { view_ysize=ypos+14;
    if (added_buttons) view_ysize+=24+4;
  }
  src.put("END\r\n\0");
  if (src.error()) { no_memory();  return NULL; }
  tptr p=src.unbind();
 /* add window size info: */
  if (simpleview)
  { 
#if 0
    xsize=DEF_XRECOFF+xpos+2*GetSystemMetrics(SM_CXFRAME)+GetSystemMetrics(SM_CXVSCROLL);
#else
    xsize=xpos+20;
#endif
    if (is_report) xsize+=50;  // approx., headers and footers change the counted column sizes!
    if (xsize<160) xsize=160; else if (xsize>600) xsize=600;
  }
  else xsize=600;
  sprintf(buf, "%u %u", xsize, view_ysize+TOPBUTTONS_Y+
#if 0
    2*GetSystemMetrics(SM_CYFRAME)+GetSystemMetrics(SM_CYCAPTION)-1);
#else
    10);
#endif
  if (p) memcpy(p+winsize_pos, buf, strlen(buf));
  return p;
}

static char * aggr_prefix[5] =
{ "Sum(", "Avg(", "Max(", "Min(", "Count" };
static char LTEXT_STR[6] = "LTEXT";

void view_generator::generate_item(t_sel_attr * sa, int pn, BOOL is_query, int aggrs)
/* Puts the item into pn pane, except for captions in listtype or simpleview:
     -- genereated for pn==PANE_DATA, but placed into PANE_PAGESTART
*/
{ char aname[3*OBJ_NAME_LEN+7+1+4];  t_ctrl * templ_itm;  int ClassNum;
  uns8 tp;  unsigned long style;  char * clnm;
  char item_text[2*OBJ_NAME_LEN+10];  const d_attr * pdatr;
  BOOL create_caption;  UINT itmsz, cptsz;
  UINT thisitemheight;//, lineheight;
  BOOL bigitem;  UINT xx, xs, yy, ys;  BOOL is_multi, combo;

 /* create the full attribute name */
  pdatr = &td->attribs[sa->attrnum];
  tp=pdatr->type;
  if (is_report && (tp==ATT_PTR || tp==ATT_BIPTR || tp==ATT_HIST)) return;
  if (tp==ATT_ODBC_TIME) tp=ATT_TIME; else
  if (tp==ATT_ODBC_DATE) tp=ATT_DATE; else
  if (tp==ATT_ODBC_NUMERIC) tp=ATT_MONEY; else
  if (tp==ATT_ODBC_DECIMAL) tp=ATT_MONEY;
  //  ATT_ODBC_TIMESTAMP not converted, editbox will be created
  BOOL value_item=staticview || sa->mainview==3 || aggrs>0;
  if (cdp)
  { aname[0]=0;
    if (td->tabcnt > 0 && pdatr->needs_prefix)
    { const char * p = COLNAME_PREFIX(td,pdatr->prefnum);
      if (*p)   /* schema name */
        { strcpy(aname, p);  strcat(aname,"."); }
      p+=OBJ_NAME_LEN+1;
      ident_to_sql(cdp, aname, p);
      strcat(aname,".");
    }
    ident_to_sql(cdp, aname+strlen(aname), pdatr->name);
    wb_small_string(cdp, aname, false);
  }
  else // ODBC 9
  { const char * longname=(const char *)(td->attribs+td->attrcnt);
    int i = sa->attrnum;
    while (--i) longname+=strlen(longname)+1;
    bool quote;
    if (strlen(longname) > ATTRNAMELEN) quote=true;
    else
    { i=1;  quote=false;
      if (!is_AlphaA(longname[0], 0)) quote=true;
      else while (longname[i])
        if (!is_AlphanumA(longname[i], 0)) { quote=true;  break; }
        else i++;
      if (!quote)
        if (check_atr_name((tptr)longname) & (IDCL_SQL_KEY | IDCL_IPL_KEY))
          quote=true;
    }
    if (quote)
    { aname[0]='`';   /* must use WB quoting!! */
      strcpy(aname+1, longname);  
      strcat(aname, "`");
    }
    else
      strcpy(aname, longname);  
  }
  is_multi = pdatr->multi!=1 && tp!=ATT_PTR && tp!=ATT_BIPTR;
  if (is_multi)  /* [ ] may be removed later! */
    strcat(aname, simpleview && is_report ? "[0]" : "[ ]");
  combo = (sa->cursobj && sa->cursobj!=NOOBJECT || sa->combo_query && *sa->combo_query) &&
          (COMBO_TRANSL_TYPE(tp) || tp==ATT_STRING) && !is_multi;

 /* item caption and its size (caption may be used as the item text as well): */
  int szcx, szcy;
  dc.GetTextExtent(wxString(sa->caption, GetConv(cdp)), &szcx, &szcy);
  cptsz=X_WRITE_OFF+szcx+AveCharWidth;  /* + space for '?' in query field */
  if (simpleview) cptsz+=SIMPLE_CAPT_MARG;    
  if (formtype) cptsz=20*AveCharWidth;
  if (!sa->caption[0]) create_caption=FALSE;
  else create_caption=simpleview || tp!=ATT_PTR  && tp!=ATT_BIPTR  &&
                                    tp!=ATT_HIST && tp!=ATT_RASTER &&
                                   (tp!=ATT_BOOLEAN || value_item || is_multi);
  if (pn!=PANE_DATA && pn!=PANE_PAGESTART && pn!=PANE_PAGEEND)  /* group header/footer */
    if (sa->xpos!=-1)  /* will be aligned, caption is in the page header */
      create_caption=FALSE;
 /* item size */
  if (tp==ATT_BOOLEAN)  /* add space for the checkbox */
    if (simpleview) itmsz=(cptsz < itemheight+4) ? itemheight+4 : cptsz;
    else itmsz=cptsz+checkbox_width;
  else if (tp==ATT_NOSPEC) itmsz=simpleview ? 40 : 100;
  else if (tp==ATT_RASTER) itmsz=simpleview ? 60 : 100;
  else if (tp==ATT_SIGNAT) itmsz=simpleview || value_item ? 100 : 180;
  else /* width derived from char num & font size */
  { if (is_string(tp))
    { itmsz=pdatr->specif.length;
      if (pdatr->specif.wide_char) itmsz /= 2;  // number of characters
      if (itmsz>30) itmsz=30;
    }
	else if (tp==ATT_BINARY)
    { itmsz=2*pdatr->specif.length;
      if (itmsz>30) itmsz=30;
    }
    else if (tp==ATT_INT8)   itmsz=4;
    else if (tp==ATT_INT64)  itmsz=20;
    else if (tp==ATT_ODBC_TIMESTAMP) itmsz=17;   /* editable timestamp */
    else if (combo) itmsz=15;
    else itmsz=(tp<=ATT_LASTSPEC) ? szoftp[tp] : (formtype?50:35);
    itmsz=X_WRITE_OFF+itmsz*AveCharWidth+4;  /* 4=empirical val */
  }
  if (simpleview) if (cptsz>itmsz) itmsz=cptsz;

  thisitemheight=itemheight;
  bigitem = tp==ATT_TEXT || tp==ATT_RASTER || tp==ATT_NOSPEC || tp==ATT_SIGNAT && !value_item
            || is_multi && !simpleview;
  if (bigitem) thisitemheight*=BIG_KOEF;

 /* plain type: check the end of the line */
  if (plaintype)  /* check the x-position */
  { unsigned needspace;
    needspace=itmsz;
    if (create_caption) needspace+=cptsz;
    if (bigitem)
    { if ((xpos_b==POS_UNINIT) || (xpos_b+needspace>width_limit))
      { ypos_b=free_line_y();  xpos_b=START_X_POS;
      }
      xpos=xpos_b;  ypos=ypos_b;
    }
    else
    { if ((xpos_a==POS_UNINIT) || (xpos_a+needspace>width_limit))
      { ypos_a=free_line_y();  xpos_a=START_X_POS;
      }
      xpos=xpos_a;  ypos=ypos_a;
    }
  }

 /* create caption & move the x-position */
  if (create_caption)  /* item caption */
  { char capt[sizeof(sa->caption)+1];
    strcpy(capt, sa->caption);
    if (is_query)
    { int len=(int)strlen(capt);  if (len && capt[len-1]==':') len--;
      capt[len]='?';  capt[len+1]=0;
    }
    xx=xpos;  xs=cptsz;  yy=ypos;  ys=itemheight;
    if (pn!=PANE_DATA && pn!=PANE_PAGESTART && pn!=PANE_PAGEEND)  /* group header/footer, not aligned */
      yy=itemheight+Y_SEPAR;
    if (!value_item && !simpleview)
      if ((tp!=ATT_TEXT) || !plaintype)
        { yy+=CAPT_Y_SHIFT;  ys-=CAPT_Y_SHIFT; }
    xx=(unsigned)((LONG)xx+xgrid/2)/xgrid*xgrid;
    xs=(unsigned)((LONG)xs+xgrid/2)/xgrid*xgrid;
    yy=(unsigned)((LONG)yy+ygrid/2)/ygrid*ygrid;
    ys=(unsigned)((LONG)ys+ygrid/2)/ygrid*ygrid;
    style=0;
   // find caption template:
    if (templ) 
    { templ_itm=find_item_by_class(templ, NO_STR, 0);
      if (templ_itm)
        { ys=templ_itm->ctCY;  style=templ_itm->ctStyle; }
    }
    else templ_itm=NULL;
    cptsz=xs;
    if (!is_query || !listtype)  // do not create captions of query fields in horizontal format
    { sprintf(buf, "LTEXT \"%s\" %d %d %d %d %d %lu ", capt, ID++, xx, yy, xs, ys, style);
      src.put(buf);
      add_pane(buf, (pn==PANE_DATA) && (listtype||simpleview) ? PANE_PAGESTART : pn);
      src.put(buf);
      src.put(" NAME ");  make_control_name(cdp, buf, NULLSTRING, NO_STR, ID-1);  src.putq(buf);
      src.put("\r\n");
      if (templ_itm)
      { t_fontdef * fd = (t_fontdef*)get_var_value(&templ_itm->ctCodeSize, CT_FONT);
        if (fd) { write_font(fd, buf);  src.put(buf); }
        if (templ_itm->ctBackCol!=templ->vwBackCol)
          { sprintf(buf, " BACKGROUND *%lu\r\n", templ_itm->ctBackCol);  src.put(buf); }
#if 0
	if (SYSCOLORPART(templ_itm->forecol)!=COLOR_WINDOWTEXT)
#endif
          { sprintf(buf, " COLOUR *%lu\r\n",     templ_itm->forecol);    src.put(buf); }
      }
    }
  }
  if (formtype || plaintype && create_caption && (tp!=ATT_TEXT) ||
      listtype && pn!=PANE_DATA && sa->xpos==-1)  /* unaligned header/footer info */
    xpos+=cptsz+4;

 /* item class and style: */
#if 0
  style=WS_TABSTOP;
#else
  style=0;
#endif
  if (is_query)
    style |=  is_string(tp) ? QUERY_DYNAMIC : QUERY_STATIC;
  *item_text=0;
#if 0
  LoadString(hPrezInst, (tp==ATT_HIST) ? SHOW_HIST :
     (((tp==ATT_PTR)||(tp==ATT_BIPTR)) ? BIND_RECS : TX_NULL),
    item_text, sizeof(item_text));
#endif
  if (combo)
  { clnm="COMBOBOX";  
#if 0
    style |= WS_BORDER | CBS_DROPDOWNLIST;
#endif
    ClassNum=NO_COMBO;
    value_item=FALSE;  thisitemheight*=3;
  }
  else if (tp==0)  // NULL constant value
    { clnm=LTEXT_STR;  value_item=TRUE;  ClassNum=NO_VAL; }
  else if (tp==ATT_BOOLEAN)
  {
#if 0
    if (value_item && !is_query || is_report)
      { clnm=LTEXT_STR;  ClassNum=NO_VAL; }
    else if (is_multi)  /* multiattribute indexed window needs edit */
      { clnm="EDITTEXT";  style |= ES_AUTOHSCROLL;  ClassNum=NO_EDI; }
    else
#endif
    { clnm="CHECKBOX";  
#if 0
      style |= BS_3STATE;
#endif      
      ClassNum=NO_CHECK;
      strcpy(item_text, " \"");
      strcat(item_text, sa->caption);
      strcat(item_text, "\"");
    }
  }
#if 0
  else if (is_multi && !simpleview)
    { clnm="LISTBOX";  style |= WS_BORDER | FLEXIBLE_SIZE;  ClassNum=NO_LIST; }
  else if ((tp==ATT_HIST)||(tp==ATT_PTR)||(tp==ATT_BIPTR))
  { clnm="PUSHBUTTON";  ClassNum=NO_BUTT; }
  else if (tp==ATT_NOSPEC)
  { clnm="OLEITEM";    ClassNum=NO_OLE;
    strcpy(item_text, " \"");
    strcat(item_text, aname);
    strcat(item_text, "\"");
  }
  else if (tp==ATT_SIGNAT)
  { if (simpleview || value_item || is_report)
    { clnm=LTEXT_STR;  style=IT_INFLUEN; /* no WS_TABSTOP */
      value_item=TRUE;    ClassNum=NO_VAL;
    }
    else
    { clnm="SIGNATURE";  ClassNum=NO_SIGNATURE;
      style |= WS_BORDER | IT_INFLUEN;
    }
  }
  else if (is_query)  /* unless boolean checkbox an other special types */
  { clnm="EDITTEXT";  style |= ES_AUTOHSCROLL;  ClassNum=NO_EDI; }
#endif
  else if (IS_HEAP_TYPE(tp))
  { clnm=LTEXT_STR;    /* text, raster */
    if      (tp==ATT_TEXT)   { style |= SS_TEXT;    ClassNum=NO_TEXT;   }
    else if (tp==ATT_RASTER) { style |= SS_RASTER;  ClassNum=NO_RASTER; }
    else ClassNum=NO_VAL;
    value_item=FALSE;   /* cannot use value, must use access */
  }
  else if (tp==ATT_DATIM || tp==ATT_AUTOR)
  { clnm=LTEXT_STR;  style=IT_INFLUEN;  ClassNum=NO_VAL; /* no WS_TABSTOP */
    value_item=TRUE;
  }
  else if (pdatr->a_flags & NOEDIT_FLAG)
  { clnm=LTEXT_STR;  ClassNum=NO_VAL;
    value_item=TRUE;
  }
  else if (value_item || is_report)
  { clnm=LTEXT_STR;  ClassNum=NO_VAL; } /* LTEXT on simple types */
  else  /* edit items on simple types */
    { clnm="EDITTEXT";  /*style |= ES_AUTOHSCROLL;9*/  ClassNum=NO_EDI; }
#if 0
  if (tp==ATT_INT16||tp==ATT_INT32||tp==ATT_INT8||tp==ATT_INT64||tp==ATT_FLOAT||tp==ATT_MONEY)
    style |= SS_RIGHT;
#endif
 /* item position and size: */
  xx=xpos;  xs=itmsz;  yy=ypos;  ys=thisitemheight;
  if ((tp==ATT_TEXT) && plaintype)
    { yy+=itemheight;  ys-=itemheight; }
  else if (((tp==ATT_DATIM) || (tp==ATT_AUTOR)) && !staticview)
    { yy+=CAPT_Y_SHIFT;  ys-=CAPT_Y_SHIFT; }
  if (pn==PANE_DATA) sa->xpos=xx;  /* define column position for header/footer */
  else if (pn!=PANE_PAGESTART || is_query && listtype)  /* use defined column position */
    if (sa->xpos==-1) yy=itemheight+Y_SEPAR; /* or put on the next line */
    else xx=sa->xpos;
  if (is_query && listtype) yy=itemheight+Y_SEPAR; // query fields located below captions in horiz type
  xx=(unsigned)((LONG)xx+xgrid/2)/xgrid*xgrid;
  xs=(unsigned)((LONG)xs+xgrid/2)/xgrid*xgrid;
  yy=(unsigned)((LONG)yy+ygrid/2)/ygrid*ygrid;
  ys=(unsigned)((LONG)ys+ygrid/2)/ygrid*ygrid;

 // look for the item in the template:
  if (templ)
  { templ_itm=find_item_by_class(templ, ClassNum, tp);
    if (templ_itm)
      { ys=templ_itm->ctCY;  style=templ_itm->ctStyle; }
  }
  else templ_itm=NULL;

 /* output the item: */
  src.put(clnm);  src.put(item_text);
  sprintf(buf," %d %d %d %d %d %lu ", ID++, xx, yy, xs, ys, style);  src.put(buf);
  if (templ_itm && templ_itm->key_flags) 
    { sprintf(buf,"%u ", templ_itm->key_flags);  src.put(buf); }
  src.put(value_item ? "value _ " : "access _ ");
  if (aggrs > 1)
  { src.put(aggr_prefix[aggrs-2]);
    if (aggrs != 6) { src.put(aname);  src.putc(')'); }
  }
  else src.put(aname);
  src.put(" _");
  int prec = templ_itm && templ_itm->type==tp ? templ_itm->precision : default_precision(tp);
  if (prec != 5)
    { sprintf(buf," PRECISION %d ", prec);  src.put(buf); }
  if (templ_itm)
  { t_fontdef * fd = (t_fontdef*)get_var_value(&templ_itm->ctCodeSize, CT_FONT);
    if (fd) { write_font(fd, buf);  src.put(buf); }
  }
  add_pane(buf, pn);  src.put(buf);
  if (combo && cdp)
    if (sa->cursobj && sa->cursobj!=NOOBJECT)  // not used any more
    { tobjname cursname;  cd_Read(cdp, OBJ_TABLENUM, sa->cursobj, OBJ_NAME_ATR, NULL, cursname);
      sprintf(buf, " COMBOSELECT _ `%s` _", cursname);  src.put(buf);
    }
    else if (sa->combo_query && *sa->combo_query)
    { src.put(" COMBOSELECT _ ");
      src.put(sa->combo_query);
      src.put(" _\r\n");
    }
  src.put(" NAME ");  make_control_name(cdp, buf, pdatr->name, ClassNum, ID-1);  src.putq(buf);
  src.put("\r\n");
 /* output special info: query condition, action: */
  if (is_query)
    if (is_string(tp))      /* prefix query */
      { sprintf(buf, "QUERYCOND \"%s.=%%\"\r\n", aname);  src.put(buf); }
    else if (tp==ATT_TEXT)  /* infix query */
      { sprintf(buf, "QUERYCOND \"%s.=.%%\"\r\n", aname);  src.put(buf); }
  if (!is_query && tp==ATT_TEXT)
    { sprintf(buf, "ACTION _ Edit_text(%s,40,40,500,200)_\r\n", aname);  src.put(buf); }
#if 0
  else if ((tp==ATT_PTR)||(tp==ATT_BIPTR))
  { char destname[OBJ_NAME_LEN+1];
    tptr p=aname+strlen(aname)-3;  /* remove '[ ]' from the end of aname */
    if ((p[2]==']')&&(p[1]==' ')&&(p[0]=='[')) p[0]=0;
    if (cd_Read(cdp, TAB_TABLENUM, pdatr->specif.destination_table, OBJ_NAME_ATR, NULL, destname))
      { destname[0]='?';  destname[1]=0; }
    sprintf(buf, "ACTION _ Bind_records(%s,\"DEFAULT %s TABLEVIEW\",no_redir,0)_\r\n", aname, destname);
    src.put(buf);
  }
#endif
 // disable editing not-editable texts:
  if (tp==ATT_TEXT && (pdatr->a_flags & NOEDIT_FLAG))
    src.put(" ENABLED false\r\n");
 // help text:
  if (sa->helptext && *sa->helptext)
  { src.put(" STATUSTEXT ");
    src.putss(sa->helptext);
    src.put("\r\n");
  }
#if 0  // this clause is not supported since version 9 (CT_EDIT created)
 // ole server:
  if (tp==ATT_NOSPEC && sa->oleserver && *sa->oleserver)
  { src.put(" DEFAULT _ ");
    src.putss(sa->oleserver);
    src.put(" _\r\n");
  }
#endif

  if (templ_itm)
  { if (templ_itm->ctBackCol!=templ->vwBackCol)
      { sprintf(buf, " BACKGROUND *%lu\r\n", templ_itm->ctBackCol);  src.put(buf); }
#if 0
    if (SYSCOLORPART(templ_itm->forecol)!=COLOR_WINDOWTEXT)
#endif
      { sprintf(buf, " COLOUR *%lu\r\n",     templ_itm->forecol);    src.put(buf); }
  }
  else if (gray_background)  // must specify white BKGND for non-transparent controls
    if (tp!=ATT_DATIM && tp!=ATT_AUTOR && !value_item)
      { strcpy(buf, " BACKGROUND 0\r\n");                           src.put(buf); }

  if (listtype)   /* caption size not added to the xpos */
  { if (pn==PANE_DATA || pn==PANE_PAGESTART || sa->xpos==-1)  /* unless aligned head/footer item */
      if (!is_query) // unless a query field 
        xpos+=X_SEPAR + (create_caption && cptsz > xs ? cptsz : xs);
  }
  else if (simpleview)
    xpos+=xs;   /* must be exactly xs, itmsz has been rounded, itmsz is not smaller than cptsz for simpleview */
  else if (formtype)
    { ypos+=thisitemheight+Y_SEPAR;  xpos=START_X_POS; }
  else   /* plain: write new xpos to xpos_a or xpos_b */
  { xpos+=xs+2*X_SEPAR;
    if (bigitem) xpos_b=xpos; else xpos_a=xpos;
  }
}

#define SEARCHABLE_TYPE(tp) (tp!=ATT_NOSPEC && tp!=ATT_RASTER && tp!=ATT_PTR\
                             && tp!=ATT_BIPTR && tp!=ATT_HIST)

CFNC DllPrezen void WINAPI define_from_extended_info(t_sel_attr * sa, ltable * extinfo)
{ if (!sa->attrnum) return;  // no extended info for the DELETED column
  trecnum rec = sa->attrnum-1;
  if (extinfo->cache_reccnt>rec && extinfo->attrcnt>=22)  // if server supports hints
  { void * valptr;  indir_obj * iobj;
    const atr_info * catr = access_data_cache(extinfo, 17, rec, valptr);  // hint caption
    if (*(char*)valptr)
      strmaxcpy(sa->caption, (char*)valptr, sizeof(sa->caption)); 
   // help text:
    access_data_cache(extinfo, 18, rec, valptr);  // hint helptext
    iobj = (indir_obj*)valptr;
    if (iobj->obj && iobj->actsize) 
    { sa->helptext = (char*)corealloc(iobj->actsize+1, 88);
      if (sa->helptext) strmaxcpy(sa->helptext, iobj->obj->data, iobj->actsize+1);
    }
   // combo query
    access_data_cache(extinfo, 19, rec, valptr);  // hint codebook
    iobj = (indir_obj*)valptr;
    if (iobj->obj && iobj->actsize) 
    { access_data_cache(extinfo, 20, rec, valptr);  // hint codetext
      char * codetext = (char*)valptr;
      access_data_cache(extinfo, 21, rec, valptr);  // hint codeval
      char * codeval = (char*)valptr;
      if (codetext)
      { sa->combo_query = (char*)corealloc(iobj->actsize+1+2*ATTRNAMELEN+30, 88);
        if (sa->combo_query) 
        { strcpy(sa->combo_query, "SELECT ");
          strcat(sa->combo_query, codetext);
          if (*codeval) { strcat(sa->combo_query, ","); strcat(sa->combo_query, codeval); }
          strcat(sa->combo_query, " FROM (");
          strmaxcpy(sa->combo_query+strlen(sa->combo_query), iobj->obj->data, iobj->actsize+1);
          strcat(sa->combo_query, ")");
        }
      }
    }
   // ole server:
    access_data_cache(extinfo, 22, rec, valptr);  // hint oleserver
    iobj = (indir_obj*)valptr;
    if (iobj->obj && iobj->actsize) 
    { sa->oleserver = (char*)corealloc(iobj->actsize+1, 88);
      if (sa->oleserver) strmaxcpy(sa->oleserver, iobj->obj->data, iobj->actsize+1);
    }
  }
}

CFNC DllPrezen void WINAPI create_extended_column_info(cdp_t cdp, view_specif * vs, const d_table * td)
{ char query[200];  tcursnum infocurs;  WBUUID uuid;  tobjname schema_name;
  if (vs->basecateg==CATEG_TABLE) 
  { cd_Read(cdp, TAB_TABLENUM, vs->basenum, APPL_ID_ATR,  NULL, uuid);
    cd_Apl_id2name(cdp, uuid, schema_name);
    sprintf(query, "SELECT * FROM _iv_table_columns WHERE table_name='%s' AND schema_name='%s'", vs->basename, schema_name);
  }
  else if (vs->basecateg==CATEG_CURSOR) 
  { cd_Read(cdp, OBJ_TABLENUM, vs->basenum, APPL_ID_ATR,  NULL, uuid);
    cd_Apl_id2name(cdp, uuid, schema_name);
    sprintf(query, "SELECT * FROM _iv_viewed_table_columns WHERE table_name='%s' AND schema_name='%s'", vs->basename, schema_name);
  }
  else if (vs->basecateg==CATEG_DIRCUR) 
    sprintf(query, "SELECT * FROM _iv_viewed_table_columns WHERE table_name=' %d'", vs->basenum);
  else return;
  if (cd_Open_cursor_direct(cdp, query, &infocurs)) return;
  trecnum cnt;
  cd_Rec_cnt(cdp, infocurs, &cnt);
  vs->ext_col_info_dt=create_data_cache(cdp, infocurs, cnt);
  if (vs->ext_col_info_dt)
  { vs->ext_col_info_dt->close_cursor=TRUE;
    vs->ext_col_info_dt->cache_top=0;  // without this cache_free will not release anything later
    cache_load(vs->ext_col_info_dt, NULL, 0, cnt, 0);
  }
}


CFNC DllPrezen tptr WINAPI initial_view(xcdp_t xcdp, int x_view_pos, int y_view_pos, int vw_type,
  const char * basename, tcateg basecat, tobjnum base_num, const char * odbc_prefix)
{ view_specif vs;  int i;
  strcpy(vs.basename, basename);  vs.basecateg=basecat;  vs.basenum=base_num;
  view_generator vg(xcdp, vw_type, basecat, base_num, basename, odbc_prefix);
  vs.multirec=vg.listtype || vg.simpleview || vg.labelview || vg.is_report;
  if (vg.td!=NULL)
  { if (vg.td->updatable & QE_IS_INSERTABLE) vs.vwz_flg |=   VWZ_FLG_INSERT | VWZ_FLG_INSERT_FICT;
    else                                     vs.vwz_flg &= ~(VWZ_FLG_INSERT | VWZ_FLG_INSERT_FICT);
    if (vg.td->updatable & QE_IS_UPDATABLE)  vs.vwz_flg |= VWZ_FLG_EDIT;
    else                                     vs.vwz_flg &= ~VWZ_FLG_EDIT;
    if (vg.td->updatable & QE_IS_DELETABLE)  vs.vwz_flg |= VWZ_FLG_DEL;
    else                                     vs.vwz_flg &= ~VWZ_FLG_DEL;
    if (xcdp->get_cdp()) create_extended_column_info(xcdp->get_cdp(), &vs, vg.td);
    vs.selatr.acc(vg.td->attrcnt);  // init the full size, prevent many small steps
    for (i=(vw_type & VIEWTYPE_DEL) && (basecat==CATEG_TABLE) ? 0 : 1;
         i < vg.td->attrcnt;  i++)
    if (memcmp(ATTR_N(vg.td,i)->name, "_W5_", 4))
    { t_sel_attr * sa = vs.selatr.next();
      sa->attrnum=i;
     /* item caption (or item text for checkboxes): */
      if ((basecat==CATEG_TABLE) && !i) strcpy(sa->caption, wxString(_("Deleted?")).mb_str(*wxConvCurrent));
      else if (IS_ODBC_TABLEC(basecat, base_num) && i)
      { tptr longname=(tptr)(vg.td->attribs+vg.td->attrcnt);
        int j=i;
        while (--j) longname+=strlen(longname)+1;
        strmaxcpy(sa->caption, longname, sizeof(sa->caption)-1);
      }
      else
      { strcpy(sa->caption, vg.td->attribs[i].name);
        if (xcdp->get_cdp()) wb_small_string(xcdp->get_cdp(), sa->caption, true);
      }
      if ((vw_type & VIEWTYPE_MASK) != VIEWTYPE_SIMPLE)
        strcat(sa->caption, ":");
     /* other default info : */
      sa->mainview=vw_type & VIEWTYPE_STATIC ? 3 : 2;
      sa->query=FALSE;
      sa->cursobj=NOOBJECT;
      { int tp    =vg.td->attribs[i].type;
        uns8 multi=vg.td->attribs[i].multi;
        if (!SEARCHABLE_TYPE(tp) || multi!=1) sa->query=0x80;
      }
      sa->groupdef=FALSE;
      sa->A_head=sa->A_foot=sa->B_head=sa->B_foot=0;
      if (vs.ext_col_info_dt) define_from_extended_info(sa, vs.ext_col_info_dt);
    }
  }
  return vg.generate(x_view_pos, y_view_pos, &vs);
}

//////////////////////////// calling the generator //////////////////////////
CFNC DllExport tptr WINAPI specific_view(cdp_t cdp, view_specif * vs, HWND hWnd)
{ view_generator vg(cdp, vs->vw_type, vs->basecateg, vs->basenum, NULL, NULL);
  return vg.generate(0, 0, vs);
}
