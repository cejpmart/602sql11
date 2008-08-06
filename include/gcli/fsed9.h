// fsed9.h
#include "flstr.h"
#include <wx/fdrepdlg.h>

#define ADVTOOLTIP
#ifdef ADVTOOLTIP
#include "advtooltip.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////
#define TOOLTIP_TIMER_ID 14
#define TOOPTIP_TIME     300  // in ms, tooltip delay time adds to this

typedef wxChar * t_posit;  // can be safely changed to uns32
SPECIF_DYNAR(t_posit_dynar, t_posit, 4);
#define NO_LINENUM (t_posit)-1

class editor_type;

class t_linenum_list
{public:
  t_posit_dynar list;
  void add(t_posit posit);
  void del(t_posit posit);
  bool contains(t_posit posit);
  BOOL del_all(void);
  t_posit next(t_posit posit);
  t_posit prev(t_posit posit);
  void adjust(wxChar * pos, int sz);
  void set_bps(editor_type * edd);  // sets all stored breakpoints
  BOOL empty(void);
};

#define ACTFL_WAS_UNCHANGED 1  // text was in unchanged state before this action
#define ACTFL_SEQUENCED     2  // this record is followed by another record created by the same action
#define ACT_TP_INS   1
#define ACT_TP_REPL  2
#define ACT_TP_BS1   3
#define ACT_TP_DEL1  4
#define ACT_TP_BSn   5
#define ACT_TP_DELn  6
#define ACT_TP_PARAM(tp) ((tp) >= ACT_TP_BSn) // has pointer to allocated memory in data

struct t_action
{ uns8     action_type;
  uns8     action_flags;
  wxChar   short_data;
  unsigned offset;
  wxChar * data;  // pointer to allocated memory OR number of inserted chars OR nothing
  void clear(void) 
    { if (ACT_TP_PARAM(action_type)) safefree((void**)&data); }
  void store(uns8 action_typeIn, uns8 action_flagsIn, unsigned offsetIn, wxChar short_dataIn, wxChar * dataIn)
  { action_type=action_typeIn;  action_flags=action_flagsIn;  
    offset=offsetIn;  short_data=short_dataIn;  data=dataIn; 
  }
  void undoit(editor_type * edd);
};

class t_action_register
{ int size;
  int head, tail; // head is the 1st valid, tail is after the last valid, empty iff tail==head
  t_action * reg;      // action sequence: growing index 
 public:
  void clear_all(void)
  { for (int i=head;  i!=tail;  !i ? i=size-1 : i--)
      reg[i].clear();
    head=tail;
  }
  bool empty(void) const
    { return head==tail; }
  t_action_register(unsigned sizeIn)
  { size=sizeIn;
    reg=(t_action *)corealloc(size*sizeof(t_action), 74);
    if (reg==NULL) size=0;
    else memset(reg, 0, size*sizeof(t_action));
    head=tail=0;
  }
  ~t_action_register(void)
  { clear_all();
    corefree(reg);
  }
  void add(uns8 action, uns8 flags, unsigned offset, wxChar short_data, wxChar * ptr)
  { if (!size) return; // not initialized because of the lack of memory
   // make free space in the buffer:
    if (++head>=size) head=0;
    if (head==tail) // is full, must clear the oldest item
    { reg[tail].clear();
      if (++tail>=size) tail=0;
    }
   // register in reg[head]:
    reg[head].store(action, flags, offset, short_data, ptr);
  }

  void do_top(editor_type * edd)
  { if (empty()) wxBell(); // buffer empty!
    else
      do
      { reg[head].undoit(edd);
       // remove the action from the buffer:
        reg[head].clear();
        if (head) head--; else head=size-1;
      } while (!empty() && (reg[head].action_flags & ACTFL_SEQUENCED));
  }

  void saved_now(void) // called when saving text
  { for (int i=head;  i!=tail;  !i ? i=size-1 : i--)
      reg[i].action_flags &= ~ACTFL_WAS_UNCHANGED;
  }
  void add_seq_flag(void)
    { reg[head].action_flags |= ACTFL_SEQUENCED; }
};

enum t_undo_state { UDS_NORMAL, UDS_UNDOING, UDS_REDOING };
#define UNDO_BUFFER_SIZE 100
#define REDO_BUFFER_SIZE  40


class t_line_kontext;
class t_line_kontexts;

enum t_content_category { CONTCAT_TEXT, CONTCAT_SQL, CONTCAT_SQL_NODEBUG, CONTCAT_CLIENT };

class gutter_editor_panel : public wxWindow
{ editor_type * my_editor;
  wxChar * popup_line_a;  
  int popup_line_n;
 public:
  gutter_editor_panel(editor_type * my_editorIn);
  DECLARE_EVENT_TABLE()
  void OnErase(wxEraseEvent & event);
  void OnPaint(wxPaintEvent& event);
  void OnLeftButtonDown(wxMouseEvent & event);
  void OnLeftButtonUp(wxMouseEvent & event);
  void OnRightButtonDown(wxMouseEvent & event);
  void OnCommand(wxCommandEvent & event);
};


class scrolled_editor_panel : public wxWindow
{ editor_type * my_editor;
 public:
  scrolled_editor_panel(editor_type * my_editorIn);
  void new_caret(void);
  DECLARE_EVENT_TABLE()
  void OnSetFocus(wxFocusEvent & event);
  void OnKillFocus(wxFocusEvent & event);
  void OnSize(wxSizeEvent & event);
  void OnErase(wxEraseEvent & event);
  void OnPaint(wxPaintEvent& event);
  void OnKeyDown(wxKeyEvent & event);
  void OnChar(wxKeyEvent & event);
  void OnLeftButtonDown(wxMouseEvent & event);
  void OnLeftButtonUp(wxMouseEvent & event);
  void OnLeftButtonDclick(wxMouseEvent & event);
  void OnRightButtonDown(wxMouseEvent & event);
  void OnRightButtonUp(wxMouseEvent & event);
  void OnMouseMotion(wxMouseEvent & event);
  void OnMouseWheel(wxMouseEvent & event);
#ifndef ADVTOOLTIP
  void OnTooltipTimer(wxTimerEvent & event);
#endif
};

#ifdef ADVTOOLTIP
class MyToolTip : public wxAdvToolTipText
{ editor_type * ed;
public:
    MyToolTip(wxWindow *w, editor_type * edIn) : wxAdvToolTipText(w), ed(edIn) {}
    
    wxString GetText(const wxPoint& pos, wxRect& boundary);
}; 
#endif

struct t_prof_data
{ uns32 line;
  uns32 brutto;
  uns32 netto;
  uns32 hits;
};

#define SIZEOF_CW_SEARCHWORD  80+1
class editor_type : public wxScrolledWindow, public any_designer  /* full screen editor data structure */
{ friend struct t_action;
  friend class t_line_kontext_www;
  friend class scrolled_editor_panel;
  friend class gutter_editor_panel;
  wxFont hFont;  
  void set_font(wxFont Font);
  wxBrush gutter_brush;
  wxPen gutter_pen;
  void draw_gutter_symbol(wxDC & dc, int x, int y, int width, int height, char ** bitmap);
  int last_gutter_offset;
 protected:
  bool auto_read_only;
 public:
  virtual void OnDraw(wxDC& dc);
  gutter_editor_panel * gutter_panel;
  scrolled_editor_panel * scrolled_panel;
  void DrawScrolled(wxDC& dc);
  void DrawGutter(wxDC& dc);
  unsigned edbsize, txtsize;  // in characters, not bytes!
  wxChar * block_start, * block_end;
  bool     block_on;
  wxChar * blockpos;  // start of the pending block selection, NULL if not selecting a block
  int      cumulative_wheel_delta;
 // data used for comparing editors:
  trecnum  recnum;  // same as objnum in any_designer
  tattrib  attr;
  uns16    index;

  BOOL     cached;
  editor_type * next_editor;
  HWND     edwnd;
  bool     _changed;
  int      noedit;     // 0=editable, 1=edit disabled, 2=editing program temp. disabled during debugging
  bool     popup,      // editor opened in popup window (currently modal operation implies)
           in_edcont,  // editor opened in the editor container
           is_dad;     // 1 only when editing a DAD
#ifdef ADVTOOLTIP
  wxAdvToolTip * m_tooltip; 
  wxString GetTooltipText(void);
#else
  wxTimer  tooltip_timer;
#endif
 private:
  int      y_curs; 
  unsigned win_xlen, win_ylen; // editor window dimensions in chars (partially visible chars included)
  unsigned nLineHeight;
  double   nCharWidth; // may be fractional
  bool     is_wrap, is_align, is_turbo, is_overwr;
  t_linenum_list bookmarks;
  t_line_kontext * lktx;
  t_line_kontexts * lktxs;
  int      wheel_delta;
  wxChar * cw_start, * cw_stop; // zacatek a konec oblasti, ve ktere se hledaji slova, kterymi by bylo mozno dokoncit aktualni slovo
  wxChar cw_searchword[SIZEOF_CW_SEARCHWORD]; // aktualni slovo, ktere se zkousi dokoncit; ='\0' pokud nyni neni spusten proces hledani
  t_flstr cw_already_found_words; // obsahuje mezerou oddelena slova, ktera byla nalezena predchazejicimi stisky Ctrl+M, nebo Ctrl+N
  void clear_completeword_params();
  int add_word_to_found_words(const char *w); // prida retezec w do cw_already_found_words
    // vraci 1, pokud slovo dosud nebylo v cw_already_found_words a ted tam bylo pridano
    //      -1, pokud slovo uz v cw_already_found_words bylo a ted se tam nepridalo
    //       0 v pripade nejake chyby
  int bookmark_gutter;    // 0 if not wisible
  int breakpoint_gutter;  // 0 if not wisible
  inline int gutter_size(void) const
    { return bookmark_gutter+breakpoint_gutter; }
  bool * local_drop_flag;    
 public:
  t_linenum_list breakpts;
  void ToggleBreakpointLocal(int line_n);
  void get_debug_pos(int & pcline, int & filenum);
  wxChar * cur_line_a;  /* current line address */
  unsigned  cur_char_n;       /* char # on the currnet line, x displacement in chars */
  unsigned  cur_line_n, total_line_n, virtual_width;
  tobjname text_name;
  wxWindow * hOwner;
  view_dyn * par_inst;
  HWND   hDisabled;    // defined only if popup
  bool local_debug_break(void) { return false; }
  virtual t_content_category content_category(void) const = 0;
  bool srv_dbg_mode(void);
  wxChar * line_num2ptr(int num);
  int line2num(wxChar * line);
  void refresh_current_line(void);
  void refresh_from_line(int start_line);
  void Refresh2(void);  // full refresh of both panels
  void undo_action(void);
  void redo_action(void);
  void refresh_profile(void);
  void toggle_profile_mode(void);

 private:
  void hide_block(void);
  LRESULT DefEditorProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  BOOL check_xpos(void);
  void adjust(wxChar * pos, int sz);
  void del_chars(wxChar * pos, int count, BOOL back);
  void del_cur_char(BOOL back)
    { del_chars(cur_line_a+cur_char_n, 1, back); }
  void delete_line_delim(bool back);
  BOOL delete_block(void);
  void seq_delete_block(void)
    { if (delete_block()) add_seq_flag(); }
  void move_block(BOOL copying);
  BOOL insert_chars(const wxChar * chs, unsigned len);
  void replace_char(wxChar ch);
  void fill_line(void);
  wxChar * prev_line(wxChar * line) const;
  inline const wxChar * cprev_line(const wxChar * line) const
    { return prev_line((wxChar*)line); }
  void set_y_curs(void);
  void set_position(wxChar * position);
  BOOL mkspace(wxChar * dest, wxChar * * psrc, unsigned size);
  void reformat(BOOL multiline);
  void kontext_menu(bool bymouse);
 // undo/redo buffers:
  t_undo_state undo_state;
  t_action_register undo_buffer;
  t_action_register redo_buffer;
  void register_action(uns8 action, unsigned offset, wxChar short_data, wxChar * ptr);
  void add_seq_flag(void)
    { undo_buffer.add_seq_flag(); }
 protected:
 // profiler:
  int profile_mode;
  t_prof_data * profiler_data;
  const t_prof_data * get_profiler_data(uns32 line);
  wxToolBarBase * my_toolbar(void) const;
  void saved_now(void)
    { undo_buffer.saved_now(); }
  void changed(void);
 private:
  void block_change(wxChar * prepos);
  void vert_scroll(int code);
  void scroll_to_cursor(void);
  void wheel_track(void);
  /* precte slovo, ktere je vlevo od kurzoru
     na rozdil od read_current_word neprecte pripadne pokracovani slova, ktere je vpravo od kurzoru
     nebere take ohled na pripadny oznaceny blok, slovo cte stejne, jako by blok nebyl oznacen
     vyuziva se v prikazu MI_FSED_COMPLETE_WORD */
  BOOL read_current_word_to_cursor(tptr keyword, unsigned bufsize);
 public:
  editor_type(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn, tcateg categIn, bool auto_read_onlyIn, char ** small_iconIn); 
  ~editor_type(void);
  bool alloc_buffer(unsigned size);
  wxChar * next_line(wxChar * line) const;
  inline const wxChar * cnext_line(const wxChar * line) const
    { return next_line((wxChar*)line); }
  LRESULT CALLBACK FsedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  LRESULT CALLBACK Fsed_COMMAND(WPARAM cmd, LPARAM lParam);
  LRESULT CALLBACK Fsed_INITMENUPOPUP(HWND hWnd, WPARAM wParam, LPARAM lParam);
  void get_comment_kontext(const wxChar * line, wbbool * in_comment, wbbool * in_sql) const;
  void mdi_activate_editor(HWND hWnd, BOOL activate, HWND activated_wnd);
  void write_lines(wxDC & dc);
  void move_caret(void);
  void count_lines(void);
  void update_virtual_size(void);
  void remove_pc_line(void);
  wxString read_current_word(void);
  void context_menu(bool by_mouse);
  BOOL read_word_at_pos(wxChar * text_start, wxChar * pos, wxChar * keyword, unsigned bufsize);
  void get_kontext_string(wxChar * buf, int maxsize);
  void update_menu_state(HMENU hMenu);
  void get_dims(void);
  int  posit2line(wxChar * pos);
  void update_toolbar(void) const;
  HWND Open(int x, int y, int xl, int yl, BOOL EdCont);
  bool prep(wxWindow * parent);
  void position_on_line_column(unsigned line, unsigned column);
  void position_on_offset(uns32 offset);
  void set_designer_caption(void);
  virtual wxString get_title(bool long_title) = 0;
  bool can_undo(void) const
    { return !undo_buffer.empty(); }
  bool can_redo(void) const
    { return !redo_buffer.empty(); }

  void get_mouse_position(wxMouseEvent & event, wxChar *& line_a, int & line_n, int & char_n);
  bool OnCopy(void);
  void OnCut(void);
  void OnPaste(void);
  void OnSelectAll(void);
  void OnLeftButtonDown(wxMouseEvent & event);
  void OnLeftButtonUp(wxMouseEvent & event);
  void OnLeftButtonDclick(wxMouseEvent & event);
  void OnRightButtonDown(wxMouseEvent & event);
  void OnRightButtonUp(wxMouseEvent & event);
  void OnMouseMotion(wxMouseEvent & event);
  void OnMouseWheel(wxMouseEvent & event);
  void OnTooltipTimer(wxTimerEvent & event);
  bool OnDropText(long x, long y, const wxString & data);
  void OnIndentBlock(void);
  void OnUnindentBlock(void);
  void BookmarkClearAll(void);
  void BookmarkPrev(void);
  void BookmarkToggle(wxChar * line_a);
  void BookmarkNext(void);
  void OnHelp(void);
  void OnFindReplace(bool replace);
  void OnFindReplaceAction(wxFindDialogEvent & event);  // event from the modeless dialog

  DECLARE_EVENT_TABLE()
  void OnSetFocus(wxFocusEvent & event);
  void OnSize(wxSizeEvent & event);
  void OnKeyDown(wxKeyEvent & event);
  void OnChar(wxKeyEvent & event);
  void OnCloseRequest(bool close_all_pages);
  void OnScroll(wxScrollWinEvent & event);

 protected:
  void ch_status(bool is_changed);
  BOOL IsEventHandler(){return false;}
  
  bool IsSame(const editor_type *editor);
 public:
  void DoClose(void);
 // virtual methods (commented in class any_designer):
  bool open(wxWindow * parent, t_global_style my_style);
  bool IsChanged(void) const;  
  void SetChanged(bool Val){_changed = Val;}
  wxString SaveQuestion(void) const;  
  //bool save_design(bool save_as); -- is in subclasses
  void OnCommand(wxCommandEvent & event);
  void OnRecordUpdate(RecordUpdateEvent & event);
  wxTopLevelWindow * dialog_parent(void);
  void _set_focus(void)
    { if (GetHandle()) if (scrolled_panel) scrolled_panel->SetFocus();  else SetFocus(); }
  void remove_tools(void);
  void remove_tools_internal(void);
  void update_designer_menu(void);

  virtual bool Compile(void)
    { return false; }
  virtual bool Run(void)
    { return false; }
  virtual bool load(uns32 len);
  virtual bool analyse_object(void)  // no action and success by default
    { return true; }

  wxChar * editbuf;

  void SetText(const wxChar *Text);
  void Activated(void);
};

#define TXED_TOOL_COUNT 17
#define TRIGGER_TXED_TOOL_COUNT 15

class text_object_editor_type : public editor_type
{ tobjname objname;
  // other data are stored in the any_designer class
 public:
  uns16 flags;
  text_object_editor_type(cdp_t cdpIn, tobjnum objnumIn, tcateg categoryIn, const char * folder_nameIn, bool auto_read_onlyIn) 
  : editor_type(cdpIn, objnumIn, folder_nameIn, cdpIn->sel_appl_name, categoryIn, auto_read_onlyIn,
                categoryIn==CATEG_PROC    ? _proc_xpm  : categoryIn==CATEG_TRIGGER ? _trigger_xpm :
                categoryIn==CATEG_CURSOR  ? _query_xpm : categoryIn==CATEG_PGMSRC  ? _xml_xpm   : _program_xpm)
    { cursnum = categoryIn==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM;  // used when locking etc.
      recnum=objnumIn;  attr=OBJ_DEF_ATR;  index=NOINDEX;
      *objname=0;  flags=0;
    }
  wxString get_title(bool long_title);
  bool load(uns32 len);
  bool analyse_object(void);
 // virtual methods:
  bool save_design(bool save_as);
  bool Compile(void);
  bool Run(void);
  void update_toolbar(void) const;
  t_content_category content_category(void) const
  { return category==CATEG_PROC   || category==CATEG_TRIGGER ? CONTCAT_SQL : 
           category==CATEG_CURSOR || category==CATEG_TABLE   ? CONTCAT_SQL_NODEBUG : 
           CONTCAT_CLIENT; 
  }
  static t_tool_info txed_tool_info[TXED_TOOL_COUNT+1];
  static t_tool_info trigger_txed_tool_info[TRIGGER_TXED_TOOL_COUNT+1];
  t_tool_info * get_tool_info(void) const;
  wxMenu * get_designer_menu(void);
  void update_designer_menu(void);
  ~text_object_editor_type(void)
  { remove_tools();  // in edcont the tools have to be removed sooner, before the other page add own tools
    // remove_tools() must be called here even for edcont in order to remove designer menu when the whole app is being closed (otherwise crashes)
  }
};

#define CLOB_TOOL_COUNT 11

class CLOB_editor_type : public editor_type
{ t_specif specif;
  // other data are stored in the any_designer class
 public:
  CLOB_editor_type(cdp_t cdpIn, tcursnum cursnumIn, tobjnum objnumIn, tattrib attrIn, bool auto_read_onlyIn) 
  : editor_type(cdpIn, objnumIn, NULLSTRING, cdpIn->sel_appl_name, (tcateg)-1, auto_read_onlyIn, _program_xpm)
    { cursnum = cursnumIn;
      recnum=objnumIn;  attr=attrIn;  index=NOINDEX;
    }
  wxString get_title(bool long_title);
  bool load(uns32 len);
 // virtual methods:
  bool save_design(bool save_as);
  t_content_category content_category(void) const
    { return CONTCAT_TEXT; }
  static t_tool_info CLOB_tool_info[CLOB_TOOL_COUNT+1];
  t_tool_info * get_tool_info(void) const
    { return CLOB_tool_info; }
  wxMenu * get_designer_menu(void);
  ~CLOB_editor_type(void)
  { remove_tools();  // in edcont the tools have to be removed sooner, before the other page add own tools
    // remove_tools() must be called here even for edcont in order to remove designer menu when the whole app is being closed (otherwise crashes)
  }
};

editor_type * find_editor_by_objnum9(cdp_t cdp, tcursnum cursnum, tobjnum objnum, tattrib attrnum, tcateg category, bool open_if_not_opened);
void realize_bps(cdp_t cdp2);
void debug_edit_enable(bool enable);
bool eval_expr_to_string(cdp_t cdp, wxString expr, wxString & res);

CFNC int WINAPI set_project_up(cdp_t cdp, const char * progname, tobjnum srcobj, BOOL recompile, BOOL report_success);
void run_pgm9(cdp_t cdp, const char * srcname, tobjnum srcobj, BOOL recompile);
