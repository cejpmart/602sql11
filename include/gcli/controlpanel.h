// controlpanel.h
#ifndef _CONTROLPANEL_H_
#define _CONTROLPANEL_H_

#include "wx/dnd.h"
#include "wx/window.h"
#include "wx/splitter.h"
#include "wx/notebook.h"
#include "wx/treectrl.h"

#include "objlist9.h"
#include "impexpui.h"

#define CATNAME_SYSTEM     _("System")
#define CATNAME_UPFOLDER   wxT("..")

#define  CTI_SERVER      "SQL server "
#define  CTI_CLIENTS     "Clients and threads"
#define  CTI_LOCKS       "Locks"
#define  CTI_TRACE       "Tracing"
#define  CTI_LOGS        "Server logs"
#define  CTI_CONSISTENCY _("Database consistency")
#define  CTI_BACKUP      _("Automatic backups")
#define  CTI_REPLAY      "Replay journal"
#define  CTI_RUNPROP     _("Runtime parameters")
#define  CTI_PROTOCOLS   _("Protocols and Ports")
#define  CTI_NETWORK     "Network access"
#define  CTI_IP          "IP address filtering"
#define  CTI_SECURITY    "Identification and security"
#define  CTI_FILENC      _("Database encryption")
#define  CTI_DLLDIRS     "Directories for DLL/SO"
#define  CTI_COMPACT     "Database size"
#define  CTI_EXTINFO     "Extended information"
#if WBVERS>=1100
#define  CTI_EXTENSIONS   _("Extensions manager")
#else
#define  CTI_LICENCES     _("Licenses")
#endif
#define  CTI_SERVICE      "Service" 
#define  CTI_CLIENT       "Client access options" 
#define  CTI_ODBC         "ODBC data sources" 
#define  CTI_ODMA         "ODMA and document stores"
#define  CTI_OLE          "OLE objects" 
#define  CTI_SERVERLIST   "SQL servers"
#define  CTI_LOCSERVEREPL "Replication parameters"
#define  CTI_SRVCERT      _("Server certificate")
#define  CTI_NEW_SERVICE  "Registered SQL server services"
#define  CTI_MOVEUSERS    "Copying users from a domain"
#define  CTI_INETCLIENT   "Internet client options"
#define  CTI_INETCLIENT_LOG "Logs"
#define  CTI_INETCLIENT_FASTCGI "FastCGI client"
#define  CTI_SERV_RENAME  _("Rename server")
#define  CTI_EXPORT_USERS  _("Export users")
#define  CTI_IMPORT_USERS  _("Import users")
#define  CTI_SQLOPT       "SQL compatibility"
#define  CTI_USERS        "Users"
#define  CTI_GROUPS       "Groups of users"
#define  CTI_APPLICATIONS "Applications"
#define  CTI_MAIL         "Mail"
#define  CTI_MONITOR      "Server monitor"
#define  CTI_SYSOBJS      _("System objects")
#define  CTI_PARAMS       _("Parameters")
#define  CTI_TOOLS        _("Tools")
#define  CTI_PROFNAME     "Profile name"
#define  CTI_PROFTYPE     "Type"
#define  CTI_LABNAME      "Name"
#define  CTI_LABSERVER    "Server"
#define  CTI_LABAPPL      "Application"
#define  CTI_LABSERVERNAME "Server name"
#define  CTI_LABLOCREM     "Type"
#define  CTI_LABSRVADDR    "Local path / Network address"
#define  CTI_LABINSTKEY    "Product ID"
#define  CTI_SYSTABLES    _("System tables")
#define  CTI_MESSAGE      _("Message to clients")
#define  CTI_PROFILER     _("Profiler")
#define  CTI_HTTP         _("Web XML interface")

#define ACTION_MENU_STRING _("&Object")

enum tcpitemid  // defines the image order in make_image_list()
{ 
    // Hlavni vetve stromu
    IND_SERVERS, IND_CLIENT, IND_ODBCDSNS,
    // Objekty
    IND_SRVLOCIDL, IND_SRVLOCRUN, IND_SRVLOCCON,                // local server: idle, running, connected
    IND_SRVREMIDL, IND_SRVREMRUN, IND_SRVREMCON, IND_SRVREMUNK, // remote server: idle, running, connected, unknown state
    IND_SRVODBC, IND_SRVODBCCON,                                // ODBC data set: idle, connected
    IND_APPL, 
    IND_TABLE, IND_REPLTAB, IND_LINK, IND_ODBCTAB, IND_VIEW, 
//    IND_STVIEW, IND_LABEL, IND_GRAPH, IND_REPORT,
//    IND_MENU, IND_POPUP,
    IND_CURSOR,
    IND_PROGR, IND_INCL, IND_TRANSP, IND_XML, //IND_SQL,
    IND_PICT,
    IND_DRAW, 
    IND_RELAT,
    IND_ROLE,
    IND_CONN, //IND_BADCONN,
    IND_REPLREL,
    IND_PROC, 
    IND_TRIGGER,
//    IND_WWWTEMPL, IND_WWWCONN, IND_WWWSEL, 
    IND_SEQ, 
    IND_DOM, IND_FULLTEXT, IND_FULLTEXT_SEP,
    IND_USER, IND_GROUP, IND_SYSTABLE, IND_REPLSRV,
    IND_FOLDER, 
    
    // Specialni slozky
    IND_UPFOLDER, IND_FOLDER_SYSTEM, IND_FOLDER_MONITOR, IND_FOLDER_SYSOBJS, IND_FOLDER_PARAMS, IND_FOLDER_TOOLS,
    // Licence
    IND_LICENCES, 
    // Monitor
    IND_CLIENTS, IND_LOCKS, IND_TRACE, IND_LOGS, IND_ALOG, IND_EXTINFO,
    // Parametry
    IND_RUNPROP, IND_PROTOCOLS, IND_NETWORK, IND_EXP_USERS, IND_SECURITY, IND_FILENC, IND_IMP_USERS, IND_SRVCERT,
    // Nastroje
    IND_BACKUP, IND_REPLAY, IND_CONSISTENCY, IND_COMPACT, IND_SERV_RENAME, 
    IND_MESSAGE, IND_PROFILER, IND_HTTP,
    // Client:
    IND_FONTS, IND_FORMATS, IND_MAIL, IND_MAILPROF, IND_LOCALE, IND_FTXHELPERS, IND_COMM, IND_EDITORS,
    // output window:
    IND_SYNTAX, IND_SEARCH, IND_PROPS, IND_SQL_CONSOLE,
    // XML Form, Style sheet, cascade style
    IND_XMLFORM, IND_STSH, IND_STSHCASC,

    IND_SERVER, 
    IND_SERVICE, 
    IND_AREGSERVER, IND_ODMA, IND_OLE,
    IND_SERVERLIST, IND_LOCSERVEREPL, IND_NEW_SERVICE, 
    IND_INETCLIENT, IND_INETCLIENT_LOG, IND_INETCLIENT_FASTCGI, 
    IND_NONE    
};

#define IND_CONSOLE_START IND_LICENCES

enum top_categories { TOPCAT_NONE, TOPCAT_SERVERS, TOPCAT_CLIENT, TOPCAT_MAILPROFS, TOPCAT_ODBCDSN };
enum system_categories { 
  SYSCAT_NONE,      // default value
  SYSCAT_APPLS,     // list of applacations
  SYSCAT_APPLTOP,   // list of application contents, shows categories and folders, folder_name may be specified
  SYSCAT_CATEGORY,  // list of objects in a category, folder_name may be specified
  SYSCAT_OBJECT,    // object or a group of objects
  SYSCAT_CONSOLE    // console page
  //SYSCAT_PARAMS,    // system params
  //SYSCAT_TOOLS      // system tools
   };

struct item_info
{ top_categories topcat;
 // for TOPCAT_SERVERS and TOPCAT_ODBCDSN:
  char server_name[MAX_OBJECT_NAME_LEN+1];
  cdp_t cdp;  // specified only when *server_name, not NULL iff connected to the 602SQL server
  t_connection * conn;  // specified only when *server_name, not NULL iff connected to the ODBC data set
  xcdp_t xcdp(void)
    { return cdp ? (xcdp_t)cdp : (xcdp_t)conn; }
  system_categories syscat;
  char schema_name[MAX_OBJECT_NAME_LEN+1];
  tcateg category;
  tobjname folder_name;
 // additional info about an object:
  char object_name[MAX_OBJECT_NAME_LEN+1];
  tobjnum  objnum;
  int object_count;  // number of selected objects
  int flags;
 // other navigation info:
  tcpitemid cti;
  bool tree_info;

  item_info(void)
  { cdp=NULL;  // used in Push_cdp()
    conn=NULL;
    *folder_name=*object_name=*schema_name=*server_name=0;
    topcat=TOPCAT_NONE;
    syscat=SYSCAT_NONE;  // default, unless found other
    flags=0;  object_count=0;
    cti=IND_NONE;
    category= NONECATEG;
  }
  item_info(top_categories topcatIn, const tobjname server_nameIn, system_categories syscatIn = SYSCAT_NONE, const char *schema_nameIn = "", tcateg categoryIn = NONECATEG, const char *folder_nameIn = "", const char *object_nameIn = "")
  { topcat      = topcatIn;
    syscat      = syscatIn;
    category    = categoryIn;
    strcpy(server_name, server_nameIn);
    strcpy(schema_name, schema_nameIn);
    strcpy(folder_name, folder_nameIn);
    strcpy(object_name, object_nameIn);
  }
  bool operator == (item_info & x)
  { return topcat==x.topcat && !my_stricmp(server_name, x.server_name) &&
           syscat==x.syscat && category==x.category && !wb_stricmp(cdp, schema_name, x.schema_name) &&
           !wb_stricmp(cdp, folder_name, x.folder_name) && !wb_stricmp(cdp, object_name, x.object_name);
  }
  void make_control_panel_popup_menu(wxMenu * popup_menu, bool formain, bool can_paste = true);
  int get_folder_index();
  // Make the schema specified in [info] the current schema, unless no schema is specified.
  void new_current_schema()
  { if (cdp && *schema_name && wb_stricmp(cdp, schema_name, cdp->sel_appl_name))
    if (cd_Set_application_ex(cdp, schema_name, TRUE))
      cd_Signalize(cdp);
  }

  bool is_expandable(void) const 
  { if (topcat==TOPCAT_SERVERS || topcat==TOPCAT_ODBCDSN)
      if (syscat == SYSCAT_CONSOLE)
        return cti==IND_FOLDER_SYSTEM || cti==IND_FOLDER_TOOLS || cti==IND_FOLDER_SYSOBJS;
      else return !*object_name;
    else if (topcat==TOPCAT_CLIENT)
      return cti==IND_CLIENT;
    else return true; 
  }
  bool IsRoot() const {return(topcat == TOPCAT_NONE);}
  bool IsServers() const {return(cti == IND_SERVERS);}
  bool IsODBCDSs() const {return(cti == IND_ODBCDSNS);}
  bool IsLocServer() const {return cti == IND_SRVLOCIDL || cti == IND_SRVLOCRUN || cti == IND_SRVLOCCON;}
  bool IsRemServer() const {return cti == IND_SRVREMIDL || cti == IND_SRVREMRUN || cti == IND_SRVREMCON || cti == IND_SRVREMUNK;}
  bool IsODBCServer() const {return cti == IND_SRVODBC || cti == IND_SRVODBCCON;}
  bool IsServer() const {return IsLocServer() || IsRemServer() || IsODBCServer();}
  bool IsSysObjectsFolder() const {return(cti == IND_FOLDER_SYSOBJS);}
  bool IsSysCategory() const {return(cti == IND_USER || cti == IND_GROUP || cti == IND_SERVER || cti == IND_SYSTABLE);}
  bool IsSchema() const {return(category == CATEG_APPL);}
  bool IsCategory() const {return(category != NONECATEG && category != CATEG_APPL && !*object_name && cti != IND_FOLDER);}
  bool IsUpFolder() const {return(cti == IND_UPFOLDER);}
  bool IsFolder() const {return(cti == IND_FOLDER);}
  bool IsObject() const {return(category != NONECATEG && *object_name);}
  bool IsObject(tcateg Categ) const {return(category == Categ && *object_name);}
  bool CanCopy()
  {
      return cdp && object_count && *schema_name && 
             (IsObject() || IsFolder() || IsCategory()) && 
             category!=CATEG_ROLE &&
             !(flags & CO_FLAG_ENCRYPTED)
             /*&& category!=CATEG_INFO*/; 
  }
  wxDragResult CanPaste(const C602SQLObjects *objs, bool DragDrop);
  wxString server_name_str(void) const
    { return wxString(server_name, *wxConvCurrent); }
  wxString schema_name_str(void) const
    { return wxString(schema_name, *cdp->sys_conv); }
  wxString folder_name_str(void) const
    { return wxString(folder_name, *cdp->sys_conv); }
  wxString object_name_str(void) const
    { return wxString(object_name, *cdp->sys_conv); }
};

////////////////////// support for iterative commands performed on the CP selection ///////////////////
class selection_iterator
{protected:
  int count;
 public:
  item_info info;
  bool whole_folder;
  virtual bool next(void) = 0;
  virtual void reset(void) = 0;
  int total_count(void) const
    { return count; }
};

class single_selection_iterator : public selection_iterator
{ bool returned;
 public:
  single_selection_iterator(item_info & infoIn)
    { info=infoIn;  count=1;  returned=false; whole_folder = false;}
  bool next(void)
    { returned = !returned;
      return returned;
    }
  void reset(void){returned=false;}
};

class ControlPanelList;

class list_selection_iterator : public selection_iterator
{ ControlPanelList * cpl;
  long pos;
 public:
  list_selection_iterator(ControlPanelList * cplIn);
  bool next(void);
  void reset(void){pos=-1;}
};

#ifdef CONTROL_PANEL_INPLECE_EDIT

class CItemEdit : public wxEvtHandler
{
protected:

    wxWindow *m_Parent;
    bool      m_InEdit;

    void OnKeyDown(wxKeyEvent & event);
    void OnKillFocus(wxFocusEvent &event);

public:
    
    DECLARE_EVENT_TABLE()
    friend class ControlPanelTree;
    friend class ControlPanelList;
};

#endif  // CONTROL_PANEL_INPLECE_EDIT

extern wxColour WindowColour;
extern wxColour WindowTextColour;
extern wxColour HighlightColour;
extern wxColour HighlightTextColour;

////////////////////////////////////// Drag&Drop in control panel ///////////////////////////////////////////

//
// wxDropSource implementation
//
class CCPDragSource : public wxDropSource
{
protected:

    wxWindow *m_Owner;      // Drag&Drop source window, used as draged image coordinates base

    static C602SQLObjects *m_DraggedObjects;

    virtual bool GiveFeedback(wxDragResult effect);

public:

    static C602SQLObjects *DraggedObjects(){return(m_DraggedObjects);}

#ifdef WINS
    CCPDragSource(wxWindow *Owner, C602SQLDataObject *dobj);
#else
    CCPDragSource(wxWindow *Owner, C602SQLDataObject *dobj, int Ico);
#endif
    ~CCPDragSource()
    {
        if (m_data)
        {
            m_DraggedObjects = NULL;
            delete m_data;
        }
    }
};
//
// wxDropTarget implementation - control panel tree and list DropTarget common ancestor
//
class CCPDropTarget : public wxDropTarget
{
protected:

    wxWindow    *m_Owner;   // DropTarget owning window
    wxDragResult m_Result;  // Last DragResult
    cdp_t        m_cdp;     // cdp
    tobjname     m_Schema;  // Current schema name
    tobjname     m_Folder;  // Current folder name

    virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
    // Where move only is posible (from folder to folder), result is wxDragMove, where copy is posible result is given by Ctrl key hit
    wxDragResult GetResult(){return((m_Result == wxDragCopy && IsCtrlDown()) ? wxDragMove : m_Result);}

public:

    CCPDropTarget(wxWindow *Owner)
    {
        m_Owner  = Owner;
        SetDataObject(new C602SQLDataObject());
    }

};

void image2categ_flags(int image, tcateg & category, int & flags);
void * get_object_enumerator(cdp_t cdp, tcateg categ, const char * schema);

class ControlPanelTree;
	
////////////////////////// splitter for control panel ////////////////
class wxCPSplitterWindow : public wxSplitterWindow, public CCPInterface
{
 public:
  MyFrame * frame;
  ControlPanelTree *tree_view;      // databese objects tree
  ControlPanelList *objlist;        // the list view coupled with this tree
  wxControl        *m_ActiveChild;  // Holds last child with focus, determines menu or toolbar actions acceptor
  bool              m_InSetFocus;   // Blocks ControlPanelTree::OnSetFocus cycling

  wxCPSplitterWindow(void)
    { tree_view=NULL;  objlist=NULL;  frame=NULL; m_InSetFocus = false;}
  ~wxCPSplitterWindow(void);
  
  item_info &GetRootInfo();
  bool ShowFolders;
  bool ShowEmptyFolders;
  bool fld_bel_cat;
  void OnSize(wxSizeEvent & event);
  void RefreshItem(const wxTreeItemId &Item, bool ListNo = false);
  void RefreshSelItem(bool ListNo = false);

  void OnConnect(selection_iterator & si);
  void OnDisconnect(selection_iterator & si);
  void OnDelete(selection_iterator & si);
  void OnNew(selection_iterator & si);
  void OnModify(selection_iterator & si, bool alternative);
  void OnRun(selection_iterator & si);
  void OnSyntaxValidation(selection_iterator & si);
  void OnFreeDeleted(selection_iterator & si);
  void OnCompacting(selection_iterator & si);
  void OnReindex(selection_iterator & si);
  void OnPing(selection_iterator & si);
  void OnConnSpeed(selection_iterator & si);
  void OnRelogin(selection_iterator & si);
  void OnSequenceRestart(selection_iterator & si);
  void OnExport(selection_iterator & si);
  void OnObjectPrivils(selection_iterator & si);
  void OnDataPrivils(selection_iterator & si);

  virtual void AddObject(const char *ServerName, const char *SchemaName, const char *FolderName, const char *ObjName, tcateg Categ, uns16 Flags, uns32 Modif, bool SelectIt);
  virtual void DeleteObject(const char *ServerName, const char *SchemaName, const char *ObjName, tcateg Categ);
  virtual void RefreshSchema(const char *ServerName, const char *SchemaName);
  virtual void RefreshServer(const char *ServerName);
  virtual void RefreshPanel();
  void OnRefresh(wxCommandEvent & event);

#ifdef WINS
#ifdef XMLFORMS
  void new_XML_form(const item_info &info);
#endif
#endif

  DECLARE_EVENT_TABLE()
};

wxCPSplitterWindow *ControlPanel();

int GetFolderState(cdp_t cdp, const char *SchemaName, const char *FolderName, tcateg category, bool Items = true, bool Folders = false);
int GetFolderState(cdp_t cdp, const char *SchemaName, const char *FolderName, bool Items = true, bool Folders = false);

bool WINAPI get_next_sel_obj(char *ObjName, CCateg *Categ, void *Param);
void CPCopy(selection_iterator &si, bool Cut);

void update_CP_toolbar(MyFrame * frame, item_info & info);
void create_tools(wxToolBarBase * tb, t_tool_info * ti);
///////////////////////////// actions ///////////////////////////////
t_avail_server * Connect(const char *ServerName);

bool create_object(wxWindow * parent, item_info & info);
bool open_object(wxWindow * parent, item_info & info);
bool open_object_alt(wxWindow * parent, item_info & info);
bool modify_object(wxWindow * parent, item_info & info, bool alternative);
bool run_object(wxWindow * parent, item_info & info);
//bool delete_object(wxWindow * parent, item_info & info, bool confirm);
bool delete_server(wxWindow * parent, item_info & info, bool confirm);
bool delete_appl(wxWindow * parent, item_info & info, bool confirm);
void remove_server(const char * server_name);
void Check_component_syntax(cdp_t cdp, const char * schema_name, SearchResultList * hReportList, wxTextCtrl * hObjectName, wxTextCtrl * hObjectCount);
void console_action(tcpitemid cti, cdp_t cdp);
void client_action(wxCPSplitterWindow * control_panel, item_info & info);
bool create_trigger(wxWindow * parent, item_info & info, const char * tabname);
#ifdef WINS
#ifdef XMLFORMS
void modi_XML_form(const item_info &info);
void fill_XML_form(cdp_t cdp, char * final_form);
#endif
#endif

class t_ie_run;
void start_transport_wizard(item_info & info, t_ie_run * dsgn, bool xml);

void insert_cp_item(item_info & info, uns32 modtime, bool select_it);
void update_cp_item(item_info & info, uns32 modtime);
//void select_cp_item(item_info & info);
BOOL cd_Restart_sequence(cdp_t cdp, tobjnum objnum, const char * object_name, const char * schema_name);
void List_users(cdp_t cdp, MyFrame * frame, const char * objname, tobjnum objnum);
void List_triggers(cdp_t cdp, MyFrame * frame, const char * objname, tobjnum objnum);

void show_source(item_info & info, wxTextCtrl * source_window);
void show_data(item_info & info, wxWindow * parent_page);
#define CATEGORY_WITH_SOURCE(category) (category==CATEG_TABLE || category==CATEG_PROC || category==CATEG_TRIGGER || \
 category==CATEG_DOMAIN || category==CATEG_CURSOR || category==CATEG_SEQ || category==CATEG_INFO || category==CATEG_PGMSRC || \
 category==CATEG_ROLE || category==CATEG_USER || category==CATEG_GROUP || category==CATEG_XMLFORM)

wxString get_licence_text(bool server_type);
extern bool ODBCConfig_available;
//////////////////////////////////// output notebook ///////////////////////////////////////////
class wxOutputNotebook : public wxNotebook
{public:
  void OnPageChanged(wxNotebookEvent& event);
  void OnFocus(wxFocusEvent& event);
  DECLARE_EVENT_TABLE()
};

#define QDTablePanelName wxT("SQL602QDTablePanel")

#define SOURCE_PAGE_NUM      0
#define DATA_PAGE_NUM        1
#define PROP_PAGE_NUM        2
#define SQL_OUTPUT_PAGE_NUM  3
#define SEARCH_PAGE_NUM      4
#define SYNTAX_CHK_PAGE_NUM  5
#define LOG_PAGE_NUM         6

wxString GetSmallStringName(const CObjectList & ol);
wxString GetWName(const CObjectList & ol);

bool load_XML_form(cdp_t cdp, const wxString &fName, tobjnum obj);
bool save_XML_form(cdp_t cdp, const wxString &fName, tobjnum obj);
wxString get_stsh_editor_path(bool for_cascaded_style);

#endif // _CONTROLPANEL_H_
