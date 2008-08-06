// topdecl.h
#ifndef TOPDECL_H_INCLUDED
#define TOPDECL_H_INCLUDED
                         
#include "wx/dataobj.h"

#ifndef WINS
#define HTMLHELP
#endif

#ifdef HTMLHELP
#include "wx/html/helpctrl.h"
#else
#ifdef WINS
#include "wx/file.h"
#if wxMAJOR_VERSION>2 || wxMINOR_VERSION>6 || wxRELEASE_NUMBER>=1
#include "helpchm3.h"
#else
#include "helpchm2.h"
#endif
#else
#undef UNIX
#include "wx/help.h"
#define UNIX
#endif
#endif

#include "wx/imaglist.h" // added in 2.5.1
// fl headers
#ifdef FL
#include "wx/fl/controlbar.h"     // core API
#include "wx/fl/dyntbar.h"       // auto-layout toolbar
#include "wx/fl/dyntbarhnd.h"    // control-bar dimension handler for it
#endif
#include "wx/notebook.h"           // used by MonitorNotebook
#include "wx/statline.h"           // used by wxMySeparatorLine
#include "wx/treectrl.h"           
#include "wx/generic/gridctrl.h"

#include "objlist9.h"

#define OWN_MDI 1
#define LENGTH_SPIN_COL_WIDTH 60  // minimal width of a grid column containing a spin control for column length
#ifdef LINUX
#define RECREATE_MENUS
#endif
///////////////////////////////////// Frame level //////////////////////////////////////
#ifdef FL
class wxFrameLayout;
#else
#include "wx/aui/aui.h"
#endif
class wxCPSplitterWindow;
class wxOutputNotebook;
class SQLConsoleDlg;
class ColumnsInfoDlg;
#include "xrc/ProfilerDlg.h"

#define REFRESH_TIMER_ID 1
class MonitorNotebook : public wxNotebook
{public:
  cdp_t cdp;  // cdp of the monitored server, NULL iff none monitored
  wxTimer m_timer;
  int monitor_fixed_pages;
  MonitorNotebook(void) : m_timer(this, REFRESH_TIMER_ID)
    { cdp=NULL; }
  void Connecting(cdp_t cdp);
  void Stop_monitoring(void);
  void Stop_monitoring_server(cdp_t a_cdp);
  void Disconnecting(cdp_t dis_cdp);
  void AttachToServer(cdp_t cdpIn);
  void OnRefresh(wxCommandEvent & event);
  void OnTimer(wxTimerEvent& event);
  void OnFocus(wxFocusEvent& event);
  void reset_main_info(void);
  void relist_logs(tcursnum curs);
  void refresh_all(void);
  tcursnum get_log_list(void);
  DECLARE_EVENT_TABLE()
};

extern wxWindow * focus_command_receiver;

class wxControlContainerJD  // like wxControlContainer, but prevents giving focus to the MainToolBar
{public:
  wxControlContainerJD(wxWindow *winParent = NULL);
  void SetContainerWindow(wxWindow *winParent) { m_winParent = winParent; }
  void HandleOnFocus(wxFocusEvent& event);
  void HandleOnWindowDestroy(wxWindowBase *child);
  bool DoSetFocus();
  void SetLastFocus(wxWindow *win);
 protected:
  bool SetFocusToChild();
  wxWindow *m_winParent;
  wxWindow *m_winLastFocused;
  DECLARE_NO_COPY_CLASS(wxControlContainerJD)
};

enum t_property_panel_type { PROP_NONE, PROP_BASIC };
enum t_global_style { ST_POPUP, ST_AUINB, ST_MDI, ST_CHILD, ST_EDCONT };

enum bar_id { BAR_ID_CP, BAR_ID_OUTPUT, BAR_ID_SQL, BAR_ID_MONITOR, BAR_ID_COLUMN_LIST, BAR_ID_CALLS, BAR_ID_WATCH, BAR_ID_FIRST_TOOLBAR,
#if WBVERS<1000
              BAR_ID_MAIN_TOOLBAR=BAR_ID_FIRST_TOOLBAR, BAR_ID_DESIGNER_TOOLBAR, BAR_ID_DBG_TOOLBAR, 
#else               
              BAR_ID_DESIGNER_TOOLBAR=BAR_ID_FIRST_TOOLBAR, BAR_ID_DBG_TOOLBAR, 
#endif
              BAR_COUNT };
// for every new bar must define bar_name, bar_name_profile and default sizes/positions

class MyFrame : public wxMDIParentFrame
{
 public:
#ifdef FL
    wxFrameLayout*  mpLayout;
#else  // AUI
    wxAuiManager m_mgr;
    void save_layout(const char * name);
    bool load_layout(const char * name);
#endif
    bool closing_all;
    wxMDIClientWindow * mpClientWnd;
    wxImageList * main_image_list;

    wxToolBarBase * MainToolBar;
    wxCPSplitterWindow * control_panel;
    wxOutputNotebook * output_window;
    MonitorNotebook * monitor_window;
    SQLConsoleDlg * SQLConsole;
    ColumnsInfoDlg * ColumnList;
    wxWindow * bars[BAR_COUNT];
    t_property_panel_type property_panel_type;
#ifdef HTMLHELP
    wxHtmlHelpController help_ctrl;
#else
#ifdef WINS
    wxCHMHelpController2 help_ctrl;
#else
    wxHelpController help_ctrl;
#endif
#endif
    wxMenu *connection_menu, *edit_menu;
    wxWindow * delayed_focus;

 private:
  void restore_rows_in_pane(int alignment, const char * section_name);
  void save_rows_in_pane(int alignment, const char * section_name);

 public:
  MyFrame(wxFrame *frame);
  virtual ~MyFrame();
  void create_layout(void);
  void SetFocusDelayed(wxWindow * w);

  //bool OnClose(void) { Show(FALSE); return TRUE; }
  void OnMenuOpen(wxMenuEvent & event);
  void update_action_menu_based_on_selection(bool can_paste = false);
  void update_object_info(int pagenum);
  wxNotebook * show_output_page(int page);
  void set_style(t_global_style new_global_style);

  void OnAbout( wxCommandEvent& event );
  void OnExit( wxCommandEvent& event );
  void OnCPCommand( wxCommandEvent& event );
  void OnClose(wxCloseEvent & event);
  void OnActivate(wxActivateEvent & event);
  void OnIdle(wxIdleEvent & event);
  void OnSize(wxSizeEvent & event);
  
  DECLARE_EVENT_TABLE()
 public: 
  void OnFocus(wxFocusEvent& event); 
  virtual void OnChildFocus(wxChildFocusEvent& event);
  virtual void SetFocus(); 
  virtual void RemoveChild(wxWindowBase *child); 
  void OnKillFocus(wxFocusEvent& event);
  void OnAuiNotebookPageChanged(wxAuiNotebookEvent & event);
  void OnAuiNotebookPageClose(wxAuiNotebookEvent & event);
 protected: 
  wxControlContainerJD m_container;
};

void switch_layouts(bool to_debug);

#define ID_OUTPUT_NOTEBOOK 1000
#define TREE_ID 1001
#define LIST_ID 1002
#define ID_MONITOR_NOTEBOOK 1004
#define ID_COLUMN_INFO      1005
#define ID_CALL_STACK 1010
#define ID_WATCH_VIEW 1011

///////////////////////////////////// APP level ////////////////////////////////////////
void set_language_support(wxLocale & locale);

class MyApp: public wxApp
{ 
 public:
  MyFrame *frame; // not owning
  bool OnInit(void);
};

DECLARE_APP(MyApp);

// from support.cpp but referring to wxGetApp:
void unpos_box(wxString message, wxWindow * parent = wxGetApp().frame);  
void error_box(wxString message, wxWindow * parent = wxGetApp().frame);
void error_boxp(wxString message, wxWindow * parent, ...);
void info_box(wxString message, wxWindow * parent = wxGetApp().frame);
bool yesno_box(wxString message, wxWindow * parent = wxGetApp().frame);
bool yesno_boxp(wxString message, wxWindow * parent, ...);
int yesnoall_box(wxString allbut, wxString message, wxWindow * parent = wxGetApp().frame);
int yesnoall_boxp(wxString allbut, wxString message, wxWindow * parent, ...);

//////////////////////////////// ancestors of designers ///////////////////////////
// global variables:
extern wxFont * text_editor_font, * grid_label_font, * grid_cell_font, system_gui_font, * control_panel_font;
extern int default_font_height;
extern wxEventType disconnect_event;
extern wxEventType record_update;
extern t_global_style global_style;
extern bool restore_default_request;
extern bool browse_enabled;
extern bool broadcast_enabled;
extern int  notification_modulus;
extern ProfilerDlg * profiler_dlg;
extern wxWindow * global_profile_frame;
extern int ext_process_couner;  // number of partially synchonized external processes 

void DefineColSize(wxGrid * grid, int col, int minsize);

struct t_tool_info  // description of tools of a designer
{ int command;  // -1 for a separator, 0 is a terminator
  const wxChar * tooltip;
  char ** xpm;
  bool bitmap_created;
  wxBitmap * bmp;
  wxItemKind kind;
  t_tool_info(int commandIn, char ** xpmIn, const wxChar * tooltipIn, wxItemKind kindIn = wxITEM_NORMAL) :
    command(commandIn), xpm(xpmIn), tooltip(tooltipIn), kind(kindIn)
      { bitmap_created=false;  bmp=NULL; }
};

class wxMySeparatorLine : public wxStaticLine
{public:
  wxMySeparatorLine() {}
  wxMySeparatorLine( wxWindow *parent, wxWindowID id) : wxStaticLine( parent, id) {}
 protected:
  virtual void DoSetSize( int x, int y, int width, int height, int sizeFlags = wxSIZE_AUTO);
};

class ServerDisconnectingEvent : public wxNotifyEvent
{public:
 // input:
  xcdp_t xcdp;  // xcdp of the server to be disconnected, xcdp==NULL used when closing all editor container
 // output:
  bool  cancelled; // the window has unsaved contents and the user cancelled saving it
  bool  ignored;   // the window has contents from another server, not set when cdp==NULL
  ServerDisconnectingEvent(xcdp_t xcdpIn) : wxNotifyEvent(disconnect_event, 0)
    { xcdp=xcdpIn;  cancelled=ignored=false; }
};

class RecordUpdateEvent : public wxNotifyEvent
// Grid notifies CLOB editors when it is about to be closed ([closing_cursor]==true)
// CLOB editor notifies its parent grid when changes have been saved ([closing_cursor]==false)
{public:
 // input:
  xcdp_t xcdp;  // xcdp of the server 
  tcursnum cursnum;  // cursor affected
  bool closing_cursor;
  trecnum recnum;    // update record, valid when [closing_cursor]==false
 // output:
  bool  unconditional;  // the window must be closed, cannot cancel the operation
  bool  cancelled; // the window has unsaved contents and the user cancelled saving it
  RecordUpdateEvent(xcdp_t xcdpIn, tcursnum cursnumIn, bool unconditionalIn) : wxNotifyEvent(record_update, 0)
    { xcdp=xcdpIn;  cursnum=cursnumIn;  unconditional=unconditionalIn;  closing_cursor=true;  cancelled=false; }
  RecordUpdateEvent(xcdp_t xcdpIn, tcursnum cursnumIn, trecnum recnumIn) : wxNotifyEvent(record_update, 0)
    { xcdp=xcdpIn;  cursnum=cursnumIn;  recnum=recnumIn;  closing_cursor=false;  unconditional=cancelled=false; }
};

#define DESIGNER_MENU_STRING _("&Design")
#define DATA_MENU_STRING     _("&Data")

struct t_designer_window_coords  // stored positions and sizes of designers and comp. frames
{ const char * designer_name;
  bool coords_loaded;
  bool coords_updated;
  wxPoint pos;
  wxSize size;
  void load(void);
  void save(void);
  t_designer_window_coords(const char * designer_nameIn, wxSize sizeIn = wxDefaultSize)
    { designer_name=designer_nameIn;  size=sizeIn;  coords_loaded=coords_updated=false; }
  const wxPoint & get_pos(void)
  { if (!coords_loaded) load();
    return pos;
  }
  const wxSize & get_size(void)
  { if (!coords_loaded) load();
    return size;
  }
};

extern t_designer_window_coords tabledsng_coords;
extern t_designer_window_coords querydsng_coords;
extern t_designer_window_coords transdsng_coords;
extern t_designer_window_coords xmldsng_coords;
extern t_designer_window_coords diagram_coords;
extern t_designer_window_coords editor_cont_coords;
extern t_designer_window_coords editor_sep_coords;
extern t_designer_window_coords seqdsng_coords;
extern t_designer_window_coords domdsng_coords;
extern t_designer_window_coords datagrid_coords;
extern t_designer_window_coords fulltext_coords;

class wxAuiNotebookExt : public wxAuiNotebook
{public:
  wxAuiNotebookExt(wxWindow* parent, wxWindowID id = wxID_ANY, 
     const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxAUI_NB_DEFAULT_STYLE) :     
   wxAuiNotebook(parent, id, pos, size, style)
   {}
  bool has_right_button_visible(void);
  void increase_offset(void);
  void normalize_offset(void);
  void OnTabButton(wxCommandEvent& evt)
   { wxAuiNotebook::OnTabButton(evt); }
  wxAuiTabCtrl* GetActiveTabCtrl(void)
   { return wxAuiNotebook::GetActiveTabCtrl(); }
};

extern wxAuiNotebookExt * aui_nb;

class competing_frame : public wxObject
{
  DECLARE_ABSTRACT_CLASS(competing_frame)
 public:
  competing_frame * next;
  bool toolbar_is_active;
  static competing_frame * active_tool_owner;
  t_global_style cf_style;  
#ifdef FL
#ifdef OWN_TOOLBARS
  wxDynamicToolBar * designer_toolbar;
#else
  static wxDynamicToolBar * designer_toolbar;
#endif
#else
  static wxToolBar * designer_toolbar;
#endif
  wxTopLevelWindow * dsgn_mng_wnd;   // managable window (can call Destroy() on it to close the designer, Activate(), NULL if the competing_frame is a child)
  wxWindow * focus_receiver;  // "this", but in the window base branch
  const wxChar * saved_menu_name;
  char ** small_icon;  // not owned
  t_designer_window_coords * persistent_coords;  // not owned
 protected:
  wxMenu * designer_menu;  // owned here
 public:
  virtual void add_tools(const wxChar * menu_name = DESIGNER_MENU_STRING);
  virtual void remove_tools(void);
   // Stores current position and size into [coords] and destroys the designer
  void destroy_designer(wxWindow * dsgnmain, t_designer_window_coords * coords);
   // Returns a top-level window suitable as parent for dialogs opened from the designer.
  virtual wxTopLevelWindow * dialog_parent(void) 
    // On MDI the frame must be the parent! Otherwise the dialog is modal on MSW!
    // On Popup the frame must not be the parent! Otherwise the dialog is not activated on GTK!
    { return global_style!=ST_POPUP ? wxGetApp().frame : dsgn_mng_wnd; }
   // Set focus to the designer
  virtual void _set_focus(void) 
    { }  // should not be pure virtual because it is called when focus arrives during the destruction of the designer
  competing_frame(void);
  ~competing_frame(void);
   // Enables and disables tools on the toolbar
  virtual void update_toolbar(void) const
    { }
   // Returns the description of tools on the toolbar.
  virtual t_tool_info * get_tool_info(void) const = 0;
   // Creates the designer-specific menu
  virtual wxMenu * get_designer_menu(void)
    { return NULL; }
   // Updates own menu, called by OnMenuOpen of the main menu bar
  virtual void update_designer_menu(void)
    { }
  virtual void table_altered_notif(ttablenum tbnum)
    { }
   // Executes a command
  virtual void OnCommand(wxCommandEvent & event) = 0;
   // Tries to save the changed contents. The event gets the results.
  virtual void OnDisconnecting(ServerDisconnectingEvent & event) = 0;
   // Receives a notification
  virtual void OnRecordUpdate(RecordUpdateEvent & event) = 0;
   // checks if the frame is a gui designer of the given object
  virtual bool IsDesignerOf(cdp_t a_cdp, tcateg a_category, tobjnum an_objnum) const
    { return false; }  // default for the data grid etc, redefined in designers
   // Called when the frame is activated. Shows own toolbar, adds own menus to the menu bar.
  virtual void Activated(void);
   // Called when the frame is deactivated. Hides own toolbar, removes own menus from the menu bar.
  virtual void Deactivated(void);
  void activate_other_competing_frame(void);
   // creates the designer windows below [parent]:
  virtual bool open(wxWindow * parent, t_global_style my_style) = 0;
   // creates and shows the current caption
  virtual void set_designer_caption(void) { }
   // Creates a toolbar in the popup frame since version 10.0
  void put_designer_into_frame(wxWindow * frame, wxWindow * dsgn);
  virtual void OnResize(void) {}
  void set_caption(wxString caption);
  void to_top(void);
};

extern competing_frame * competing_frame_list;

wxMenu * get_empty_designer_menu(void);

class any_designer : public competing_frame
{
  DECLARE_ABSTRACT_CLASS(any_designer)
 public:
  cdp_t    cdp;
  tobjnum  objnum; // NOOBJECT for objects not created yet
  tcursnum cursnum; // initially OBJ_TABLENUM, changed to TAB_TABLENUM in table designer, changed in text_object_editor_type
  tobjname folder_name;  // used when creating a new object
  tobjname schema_name;  // used when creating a new object or modifying an existing one
  tcateg   category;
  bool     read_only_mode;  // prevents locking the object definition
  bool     object_locked;  // the object [objnum] is write-locked
 public:
 // services for designers:
   // If the design is changed, asks about saving changes and possibly saves them. Returns false if exit vetoed.
  bool exit_request(wxWindow * parent, bool can_veto);
   // Says if the designer if modifying an existing object
  inline bool modifying(void) const
    { return objnum!=NOOBJECT; }
   // Locks the underlying object. Returns false iff cannot lock.
  bool lock_the_object(void);
   // Unlocks the underlying object, if locked
  void unlock_the_object(void);
   // Writes the information about the active editor to the status bar.
  void write_status_message(void);
   // Generic save: Caller must profide the pointer to the object_name. If true returned, caller must clear the "changed" flag.
  bool save_design_generic(bool save_as, char * object_name, int flags);
   // Process the common keys like Ctrl+S or Ctrl+F4, returns true if processed
  bool OnCommonKeyDown(wxKeyEvent & event);

 // virtual methods:
   // Called by save_design_generic. Some designers may use it.
  virtual bool TransferDataFromWindow(void)
    { return true; }
   // Called by save_design_generic. Returns memory allocate by corealloc or NULL or error.
  virtual char * make_source(bool alter)
    { return NULL; }
   // Says if the design has been changed by the user action.
  virtual bool IsChanged(void) const = 0;  
   // Returns the question about saving changes. May depend on the fact if the object is new or existing.
  virtual wxString SaveQuestion(void) const = 0;  
   // Saves the desing, reports errors to the user. Clears the "changed flag. Updates the caches and the control panel data.
   // Returns true on success.
  virtual bool save_design(bool save_as) = 0;
   // Tries to save the changed contents. The event gets the results.
  virtual void OnDisconnecting(ServerDisconnectingEvent & event);
   // Receives a notification
  virtual void OnRecordUpdate(RecordUpdateEvent & event) {}
   // Called when the designer is activated. Shows own toolbar, adds own menus to the menu bar.
  virtual void Activated(void);
   // Called when the designer is deactivated. Hides own toolbar, removes own menus from the menu bar.
  virtual void Deactivated(void);
   // Creates the designer-specific menu (any_designer::create_designer_menu adds the common items)
  wxMenu * get_designer_menu(void);
   // checks if this is a gui designer of the given object
  virtual bool IsDesignerOf(cdp_t a_cdp, tcateg a_category, tobjnum an_objnum) const
    { return cdp==a_cdp && category==a_category && objnum==an_objnum; }
   // Returns a top-level window suitable as parent for dialogs opened from the designer.
  virtual wxTopLevelWindow * dialog_parent(void) 
    { return category==CATEG_SEQ || category==CATEG_DOMAIN ? dsgn_mng_wnd : competing_frame::dialog_parent(); }

  any_designer(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn, tcateg categoryIn, 
               char ** small_iconIn, t_designer_window_coords * persistent_coordsIn)
    : competing_frame()
  { cdp=cdpIn;  
    objnum=objnumIn;
    cursnum=OBJ_TABLENUM;  // changed in some designers
    strcpy(folder_name, folder_nameIn);
    strcpy(schema_name, schema_nameIn); // for existing objects the schema_name is supposed to be OK!
    category=categoryIn;
    read_only_mode=false;
    object_locked=false;
    small_icon=small_iconIn;
    persistent_coords=persistent_coordsIn;
  }
  any_designer(void) {}  // never used, used only for dynamic cast
   // Destroys the own toobar and menus.
  virtual ~any_designer(void);
};

wxPoint get_best_designer_pos(void);

class DesignerFrameMDI : public wxMDIChildFrame
{public:
  DesignerFrameMDI(wxMDIParentFrame* parent, wxWindowID id, const wxString& title, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE)
    : wxMDIChildFrame(parent, id, title, pos, size, style, wxT("designer_frame_MDI"))
    { dsgn = NULL; }
  ~DesignerFrameMDI(){ dsgn = NULL; }
  competing_frame * dsgn;
  void OnCommand( wxCommandEvent& event );
  void OnCloseWindow(wxCloseEvent& event);
  void OnActivate(wxActivateEvent & event);
  void OnFocus(wxFocusEvent& event); 
  void OnDisconnecting(wxEvent & event);
  void OnRecordUpdate(wxEvent & event);
  void OnSize(wxSizeEvent & event);

  DECLARE_EVENT_TABLE()
};

#define DSGN_IN_FRAME  // both version work well, wxFrame is more tested

#ifdef DSGN_IN_FRAME
class DesignerFramePopup : public wxFrame
{public:
  DesignerFramePopup(wxWindow * parent, wxWindowID id, const wxString& title, const wxPoint& pos = get_best_designer_pos()/*wxDefaultPosition*/, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxWANTS_CHARS)
  // Warning: in wx 2.5.3 the style wxFRAME_NO_TASKBAR makes it impossible to focus the designer
    : wxFrame(parent, id, title, pos, size, style, wxT("designer_frame_popup"))
#else
class DesignerFramePopup : public wxDialog
{public:
  DesignerFramePopup(wxWindow * parent, wxWindowID id, const wxString& title, const wxPoint& pos = get_best_designer_pos()/*wxDefaultPosition*/, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR)
    : wxDialog(parent, id, title, pos, size, /*style*/wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxWANTS_CHARS, wxT("designer_frame_popup"))
#endif    
    { dsgn = NULL; }
  competing_frame * dsgn;
  void OnCommand( wxCommandEvent& event );
  void OnCloseWindow(wxCloseEvent& event);
  void OnActivate(wxActivateEvent & event);
  void OnFocus(wxFocusEvent& event); 
  void OnDisconnecting(wxEvent & event);
  void OnRecordUpdate(wxEvent & event);
  void OnSize(wxSizeEvent & event);

  DECLARE_EVENT_TABLE()
};

///////////////////////////////////// less comon shared objects //////////////////////////////////////
#ifdef WINS
#define wxGridCellODChoiceEditor wxGridCellChoiceEditor
#else  
/* On some GTK versions the wxComboBox does not have the keyboard interface 
   (unless the arrow is clicked)
   wxGridCellODChoiceEditor is the same as wxGridCellChoiceEditor but is uses 
   wxOwnerDrawnComboBox instead of wxComboBox.
*/
#include <wx/odcombo.h>
// the editor for string data allowing to choose from the list of strings
class WXDLLIMPEXP_ADV wxGridCellODChoiceEditor : public wxGridCellEditor
{
public:
    // if !allowOthers, user can't type a string not in choices array
    wxGridCellODChoiceEditor(size_t count = 0,
                           const wxString choices[] = NULL,
                           bool allowOthers = false);
    wxGridCellODChoiceEditor(const wxArrayString& choices,
                           bool allowOthers = false);

    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler);
    virtual wxGridCellEditor *Clone() const;

    virtual void PaintBackground(const wxRect& rectCell, wxGridCellAttr *attr);

    virtual void BeginEdit(int row, int col, wxGrid* grid);
    virtual bool EndEdit(int row, int col, wxGrid* grid);

    virtual void Reset();

    // parameters string format is "item1[,item2[...,itemN]]"
    virtual void SetParameters(const wxString& params);


    // added GetValue so we can get the value which is in the control
    virtual wxString GetValue() const;

protected:
    wxOwnerDrawnComboBox *Combo() const { return (wxOwnerDrawnComboBox *)m_control; }

// DJC - (MAPTEK) you at least need access to m_choices if you
//                wish to override this class
protected:
    wxString        m_startValue;
    wxArrayString   m_choices;
    bool            m_allowOthers;

    DECLARE_NO_COPY_CLASS(wxGridCellODChoiceEditor)
};
#endif

struct t_type_list
{ const wxChar * short_name;
  const wxChar * long_name;
  int  wbtype;
  uns32 default_specif;
  unsigned flags;
};

extern t_type_list wb_type_list[];

int convert_type_for_edit(const t_type_list * type_list, int type, t_specif & specif);
void convert_type_for_sql_1(const t_type_list * type_list, int type, t_specif & specif);
int convert_type_for_sql_2(const t_type_list * type_list, int type, t_specif & specif);

class wxGridCellMyEnumEditor : public wxGridCellODChoiceEditor
{
public:
    wxGridCellMyEnumEditor( const wxString& choicesIn = wxEmptyString );
    virtual ~wxGridCellMyEnumEditor() {}

    virtual wxGridCellEditor*  Clone() const;
    void Reset(void);
    virtual bool EndEdit(int row, int col, wxGrid* grid);
    virtual void BeginEdit(int row, int col, wxGrid* grid);

protected://private:
    wxString choices;
    long oldval;

    DECLARE_NO_COPY_CLASS(wxGridCellMyEnumEditor)
};

class wxGridCellChoiceEditorNoKeys : public wxGridCellODChoiceEditor
// Reducing the number of accepted keys.
{
public:
    wxGridCellChoiceEditorNoKeys(size_t count = 0, const wxString choices[] = NULL, bool allowOthers = false)
     : wxGridCellODChoiceEditor(count, choices, allowOthers)
      { }
    wxGridCellChoiceEditorNoKeys(const wxArrayString& choices, bool allowOthers = false)
     : wxGridCellODChoiceEditor(choices, allowOthers)
      { }
    virtual bool IsAcceptedKey(wxKeyEvent& event);
};


wxGridCellTextEditor * make_limited_string_editor(int width);

///////////////////////////////////////////// entry points ////////////////////////////////////////////////
class WizardPage3;
struct t_xsd_gen_params;
struct t_dad_root;
struct dad_element_node;
void create_default_design_analytical(const d_table * top_td, const t_avail_server * as, const char * tablename, const char * schemaname, 
                const char * alias, t_dad_root * dad_tree, wxTreeCtrl * tree, dad_element_node * rowel, wxTreeItemId hRowItem);
void create_default_design(cdp_t cdp, const d_table * td, t_dad_root * dad_tree, wxTreeCtrl * tree, const char * server);

bool open_standard_designer(any_designer * dsgn);
bool open_dialog_designer(any_designer * dsgn);
wxWindow * open_standard_frame(competing_frame * dsgn, t_global_style my_style = global_style);
bool open_table_designer(cdp_t cdp, tobjnum objnum, const char * folder_name, const char * schema_name);
bool start_query_designer(cdp_t cdp, tobjnum objnum, const char * folder_name);
bool start_domain_designer(cdp_t cdp, tobjnum objnum, const char * folder_name);
bool start_sequence_designer(cdp_t cdp, tobjnum objnum, const char * folder_name);
bool open_text_object_editor(cdp_t cdp, tcursnum cursnum, tobjnum objnum, tattrib attrnum, tcateg category, const char * folder_name, int ce_flags);
bool start_transport_designer(cdp_t cdp, tobjnum objnum, const char * folder_name, char * initial_design);
bool start_diagram_designer(cdp_t cdp, tobjnum objnum, const char * folder_name);
bool start_dad_designer(cdp_t cdp, tobjnum objnum, const char * folder_name, WizardPage3 * wpg, t_xsd_gen_params * gen_params);
bool start_fulltext_designer(cdp_t cdp, tobjnum objnum, const char * folder_name);
bool Edit_relation(cdp_t cdp, wxWindow * parent, tcateg subject1, tobjnum subjnum, tcateg subject2);
bool Edit_privileges(cdp_t cdp, wxWindow * hParent, ttablenum * tbs, trecnum * recnums, int multioper);

wxWindow * open_data_grid(wxWindow * parent, xcdp_t cdp, tcateg categ, tcursnum curs, char * objname, bool delrecs, const char * odbc_prefix);
wxWindow * open_form(wxWindow * parent, cdp_t cdp, const char * source, tcursnum curs, int flags);

int get_image_index(tcateg cat, int flags);
void call_routine(wxWindow * parent, cdp_t cdp, tobjnum objnum);

void delete_bitmaps(t_tool_info * ti);
void dom_delete_bitmaps(void);
void seq_delete_bitmaps(void);
void table_delete_bitmaps(void);
void fsed_delete_bitmaps(void);
void grid_delete_bitmaps(void);
void xml_delete_bitmaps(void);
void diagram_delete_bitmaps(void);
void query_delete_bitmaps(void);
void trans_delete_bitmaps(void);
void fulltext_delete_bitmaps(void);

/////////////////////////////////////////// command IDs ////////////////////////////////////
#define CPM_SHOW_HIDE 90  // cca 10 values
#define CPM_OPEN     100
#define CPM_MODIFY   101
#define CPM_DELETE   102
#define CPM_NEW      103
#define CPM_LINK     104
#define CPM_IMPORT   105
#define CPM_EXPORT   106
#define CPM_PRIVILS  107
#define CPM_RUN      108
#define CPM_CUT      109  // for control panel only
#define CPM_COPY     110  // for control panel only
#define CPM_PASTE    111  // for control panel only
#define CPM_DISCONN  112
#define CPM_RELOGIN  113
#define CPM_REFRPANEL 114 // for control panel only

#define CPM_IMPAPPL  115
#define CPM_CONNSPD  116
#define CPM_CONNECT  117
#define CPM_PING     118
#define CPM_SYNTAX   119
#define CPM_DATAPRIV  120
#define CPM_COMPACT   121
#define CPM_FREEDEL   122
#define CPM_DATAIMP   123
#define CPM_DATAEXP   124
#define CPM_LAYOUT_MANAGER 125
#define CPM_REG_SRV   126
#define CPM_EDIT_SOURCE 127
#define CPM_DB_MN     128
#define CPM_CREFLD    129
#define CPM_FLDBELCAT 130
#define CPM_UP        131
#define CPM_IMPEXP    132
#define CPM_XMLIMPEXP 133
#define CPM_SEARCH    134
#define CPM_LANGUAGES 135
#define CPM_DBGATT   136
#define CPM_CROSSHELP 138
#define CPM_ABOUT     139
#define CPM_SEL_USERS 140
#define CPM_SEL_GROUPS 141
#define CPM_SEL_ROLES 142
#define CPM_ENABLE_ACCOUNT 143
#define CPM_DISABLE_ACCOUNT 144
#define CPM_RESTART 145
#define CPM_SHOWFOLDERS     146
#define CPM_SHOWEMPTYFLDRS  147
#define CPM_NEW_TEXT  148
#define CPM_MONITOR   149
#define CPM_EXIT      150
#define CPM_DISCONN_ALL 151
#define CPM_ACTION    152      // ID of the submenu
#define CPM_VIEW_TOOLBARS 153  // ID of the submenu
#define CPM_MDI 154
#define CPM_MODIFY_ALT 155
#define CPM_TABLESTAT  156
#define CPM_OPTIM      157
#define CPM_REINDEX    158
#define CPM_EMPTY_DESIGNER 159  // never emitted
#define CPM_START_SERVER 160
#define CPM_STOP_SERVER  161
#define CPM_DUPLICATE    162
#define CPM_RENAME       163
#define CPM_LIST_USERS   164
#define CPM_LIST_TRIGGERS 165
#define CPM_NEW_XMLFORM   166
#define CPM_ALT_OPEN      167
#define CPM_ODBC_MANAGE   168
#define CPM_POPUP         169
#define CPM_AUINB         170
#define CPM_CREATE_ODBC_DS 171
#define CPM_CREATE_TRIGGER 172

#define FIRST_MDICHILD_MSG  1000
#define TD_CPM_SAVE     1000
#define TD_CPM_EXIT     1001  // exit the active designer
#define TD_CPM_INSCOL   1002
#define TD_CPM_DELCOL   1003
#define TD_CPM_OPTIONS  1004
#define TD_CPM_SQL      1005
#define TD_CPM_CHECK    1006
#define TD_CPM_HELP     1007
#define TD_CPM_UNDO     1008
#define TD_CPM_REDO     1009
#define TD_CPM_EXIT_UNCOND 1010
#define TD_CPM_RUN      1011
#define TD_GUTTER_BOOK  1012
#define TD_GUTTER_BREAK 1013
#define TD_CPM_FIND               1014
#define TD_CPM_FIND_ALL           1015
#define TD_CPM_OPEN               1016
#define TD_CPM_EVAL               1017
#define TD_CPM_WATCH              1018
#define TD_CPM_TOGGLE_BOOKMARK    1019
#define TD_CPM_TOGGLE_BREAKPOINT  1020
#define TD_CPM_SAVE_AS            1021
#define TD_CPM_XML_IMPORT         1022
#define TD_CPM_XML_EXPORT         1023
#define TD_CPM_XML_TESTEXP        1024
#define TD_CPM_XML_PARAMS         1025
#define TD_CPM_TEXT_PROP          1026
#define TD_CPM_REPLACE            1027
#define TD_CPM_UNINDENT_BLOCK     1028
#define TD_CPM_INDENT_BLOCK       1029
#define TD_CPM_NEXT_BOOKMARK      1030
#define TD_CPM_PREV_BOOKMARK      1031
#define TD_CPM_CLEAR_BOOKMARKS    1032
#define TD_CMP_BOOKMARKS          1033  // submenu
#define TD_CPM_PASTE_TABLES       1034
#define TD_CPM_REMOVE_TABLE       1035
#define TD_CPM_ALTER_TABLE        1036
#define TD_CPM_REMOVE_CONSTR      1037
#define TD_CPM_REFRESH            1038  // for competing frame
#define TD_CPM_OPTIM              1039
#define TD_CPM_FIRST              1040
#define TD_CPM_PREVPAGE           1041
#define TD_CPM_PREV               1042
#define TD_CPM_NEXT               1043
#define TD_CPM_NEXTPAGE           1044
#define TD_CPM_LAST               1045
#define TD_CPM_DATAPRIV           1046
#define TD_CPM_DELREC             1047
#define TD_CPM_DELALL             1048
#define TD_CPM_PRINT              1049
#define TD_CPM_PRINT_PREVIEW      1050
#define TD_CPM_PRINT_SETUP        1051
#define TD_CPM_PAGE_SETUP         1052
#define TD_CPM_ELEMENTPOPUP       1053
#define TD_CPM_ADDTABLE           1054
#define TD_CPM_DELETETABLE        1055
#define TD_CPM_DELETELINK         1056
#define TD_CPM_ADDORDERBY         1057
#define TD_CPM_DELORDERBY         1058
#define TD_CPM_ADDLIMIT           1059
#define TD_CPM_DELLIMIT           1060
#define TD_CPM_ADDQUERY           1061
#define TD_CPM_DELQUERY           1062
#define TD_CPM_ADDQUERYEXPR       1063
#define TD_CPM_DELQUERYEXPR       1064
#define TD_CPM_ADDWHERE           1065
#define TD_CPM_DELWHERE           1066
#define TD_CPM_ADDGROUPBY         1067
#define TD_CPM_DELGROUPBY         1068
#define TD_CPM_ADDHAVING          1069
#define TD_CPM_DELHAVING          1070
#define TD_CPM_ADDJOIN            1071
#define TD_CPM_DELJOIN            1072
#define TD_CPM_ADDSUBCOND         1073
#define TD_CPM_DELCOND            1074
#define TD_CPM_DELCOLUMN          1075
#define TD_CPM_DELEXPR            1076
#define TD_CPM_DATAPREVIEW        1077
#define TD_CPM_OPTIMIZE           1078
#define TD_CPM_DSGN_PARAMS        1079
#define TD_CPM_HIDDENATTRS        1080
#define TD_CPM_ADDINDEX           1081
#define TD_CPM_CUT                1082  // for competing frame
#define TD_CPM_COPY               1083  // for competing frame
#define TD_CPM_PASTE              1084  // for competing frame
#define TD_CPM_EXIT_FRAME         1085  // close the while frame (editor container)
#define TD_CPM_CRE_IMPL           1086
#define TD_CPM_FILTORDER          1087
#define TD_CPM_REMFILTORDER       1088
#define TD_CPM_SHOWCOL            1089
#define TD_CPM_PROFILE            1090

#define LAST_MDICHILD_MSG  1999


#define FIRST_DBG_MSG   2000
#define DBG_STEP_INTO   FIRST_DBG_MSG+0
#define DBG_STEP_OVER   FIRST_DBG_MSG+1
#define DBG_RUN         FIRST_DBG_MSG+2
#define DBG_GOTO        FIRST_DBG_MSG+3
#define DBG_RETURN      FIRST_DBG_MSG+4
#define DBG_BREAK       FIRST_DBG_MSG+5
#define DBG_TOGGLE      FIRST_DBG_MSG+6
#define DBG_KILL        FIRST_DBG_MSG+7
#define DBG_EXIT        FIRST_DBG_MSG+8
#define DBG_EVAL        FIRST_DBG_MSG+9

#define DBG_STATE       FIRST_DBG_MSG+50
#define LAST_DBG_MSG    2099

///////////////// internal commands ////////////////////////////
// These commands are not processed by the main frame window.
// They are emited by local popup menus and sent directly to the proper window.
#define FIRST_INTERN_MSG   2100
#define CPM_CLIENT_BREAK FIRST_INTERN_MSG+0
#define CPM_CLIENT_KILL  FIRST_INTERN_MSG+1
#define CPM_LOCK_REMOVE  FIRST_INTERN_MSG+2
#define CPM_TRACE_REMOVE FIRST_INTERN_MSG+3
#define CPM_NEW_LOG      FIRST_INTERN_MSG+4

#define LAST_INTERN_MSG    2199

extern wxDataFormat * DF_602SQLDataRow;

extern t_avail_server * available_servers;
void list_available_servers(void);
t_avail_server * find_server_connection_info(const char * server_name);
bool try_disconnect(t_avail_server * as);
bool is_port_unique(uns16 port, const char * except_server_name);
void component_change_notif(cdp_t cdp, tcateg category, const char * object_name, const char * schema_name, int flags, tobjnum objnum);

#define OWN_TOOLBAR_ID 123
#define COMP_FRAME_ID  124

#endif // TOPDECL_H_INCLUDED

