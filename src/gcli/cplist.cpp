//
// cplist.cpp
//
// Control panel listview
//
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#ifndef WINS
#include "winrepl.h"
#include "wx/caret.h"
#endif
#include "cint.h"            
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop
#ifndef WINS
#define _stricoll(a,b) strcasecmp(a,b)
#endif
#include "objlist9.h"
#include "controlpanel.h"
#include "cptree.h"
#include "cplist.h"
#include "regstr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#define NOT_CONNECTED_TO_THE_SERVER _("Not connected to the server")
#define LOCAL_SERVER                _("Local")
#define NETWORK_SERVER              _("Network")

BEGIN_EVENT_TABLE( ControlPanelList, wbListCtrl )
#ifdef CONTROL_PANEL_INPLECE_EDIT
    EVT_LIST_BEGIN_LABEL_EDIT(LIST_ID,    ControlPanelList::OnBeginLabelEdit)
    EVT_LIST_END_LABEL_EDIT(LIST_ID,      ControlPanelList::OnEndLabelEdit)
    EVT_COMMAND(LIST_ID, myEVT_INITEDIT,  ControlPanelList::OnInitEdit)
    EVT_COMMAND(LIST_ID, myEVT_AFTEREDIT, ControlPanelList::OnAfterEdit)
#else
    EVT_MENU(CPM_RENAME,                  ControlPanelList::OnRename)
#endif
    EVT_LIST_ITEM_ACTIVATED(LIST_ID,      ControlPanelList::OnItemActivated)
    EVT_LIST_ITEM_SELECTED(LIST_ID,       ControlPanelList::OnItemSelected)
    EVT_LIST_ITEM_DESELECTED(LIST_ID,     ControlPanelList::OnItemSelected)
    EVT_LIST_COL_CLICK(LIST_ID,           ControlPanelList::OnSort)
    EVT_LIST_COL_END_DRAG(LIST_ID,        ControlPanelList::OnColResize)
    EVT_MENU(CPM_CUT,                     ControlPanelList::OnCut)
    EVT_MENU(CPM_COPY,                    ControlPanelList::OnCopy)
    EVT_MENU(CPM_PASTE,                   ControlPanelList::OnPaste)
    EVT_MENU(CPM_DUPLICATE,               ControlPanelList::OnDuplicate)
    EVT_MENU(CPM_REFRPANEL,               ControlPanelList::OnRefresh)      // must not send this to OnCPCommand because it sends this to the focused window
    EVT_MENU(-1,                          ControlPanelList::OnCPCommand)
    EVT_LIST_BEGIN_DRAG(LIST_ID,          ControlPanelList::OnBeginDrag)
#ifdef WINS
    EVT_COMMAND_RIGHT_CLICK(LIST_ID,      ControlPanelList::OnRightClick)   // exists only on MSW
#else
    EVT_RIGHT_UP(                         ControlPanelList::OnRightClick)   // enters selection mode on MSW
    EVT_LEFT_DOWN(                        ControlPanelList::OnLeftMouseDown)
#endif
    EVT_KEY_DOWN(                         ControlPanelList::OnKeyDown)
    EVT_RIGHT_DOWN(                       ControlPanelList::OnRightMouseDown) 
    EVT_SET_FOCUS(                        ControlPanelList::OnSetFocus)
    EVT_TIMER(-1,                         ControlPanelList::OnTimer)
    //EVT_RIGHT_UP(                         ControlPanelList::OnRightMouseUp) -- event not generated for wxListCtrl!
    //EVT_LIST_ITEM_RIGHT_CLICK(LIST_ID,    ControlPanelList::OnRightMouseClick)  -- is not emitted when user clicks the list control below list items
END_EVENT_TABLE()
//
// Returns image index for object of given category and type
//
int get_image_index(tcateg cat, int flags)
{ switch (cat)
  { case CATEG_TABLE:
      return flags & CO_FLAG_ODBC    ? IND_ODBCTAB :
             flags & CO_FLAG_REPLTAB ? IND_REPLTAB :
             flags & CO_FLAG_LINK    ? IND_LINK    : IND_TABLE;
    case CATEG_VIEW:
      if (flags & CO_FLAG_LINK) return IND_LINK;
      return IND_VIEW;
#if 0
      switch (flags & CO_FLAGS_OBJECT_TYPE)
      { case CO_FLAG_SIMPLE_VW : return IND_STVIEW;
        case CO_FLAG_LABEL_VW  : return IND_LABEL;
        case CO_FLAG_GRAPH_VW  : return IND_GRAPH;
        case CO_FLAG_REPORT_VW : return IND_REPORT;
        default                : return IND_VIEW;
      }
    case CATEG_MENU:
      if (flags & CO_FLAG_LINK) return IND_LINK;
      return flags & CO_FLAG_POPUP_MN ? IND_POPUP : IND_MENU;
#endif
    case CATEG_CURSOR: return (flags & CO_FLAG_LINK) ? IND_LINK : IND_CURSOR;
    case CATEG_PGMSRC:
      if (flags & CO_FLAG_LINK) return IND_LINK;
      switch (flags & CO_FLAGS_OBJECT_TYPE)
      { case CO_FLAG_INCLUDE   : return IND_INCL;
        case CO_FLAG_TRANSPORT : return IND_TRANSP;
        case CO_FLAG_XML       : return IND_XML;
        default                : return IND_INCL;
      }
#if 0
    case CATEG_CONNECTION:
      return flags & CO_FLAG_POPUP_MN ? IND_BADCONN : IND_CONN;
    case CATEG_WWW:
      if (flags & CO_FLAG_LINK) return IND_LINK;
      switch (flags & CO_FLAGS_OBJECT_TYPE)
      { case CO_FLAG_WWWCONN   : return IND_WWWCONN;
        case CO_FLAG_WWWSEL    : return IND_WWWSEL;
        default                : return IND_WWWTEMPL;
      }
#endif
#ifdef XMLFORMS
#ifdef WINS
    case CATEG_XMLFORM:
        return IND_XMLFORM;
#endif
    case CATEG_STSH:
      return flags & CO_FLAG_CASC ? IND_STSHCASC : IND_STSH;
#endif
    case CATEG_PICT:             return (flags & CO_FLAG_LINK) ? IND_LINK : IND_PICT;
    case CATEG_ROLE:             return IND_ROLE;
    case CATEG_RELATION:         return IND_RELAT;
    case CATEG_USER:             return IND_USER;
    case CATEG_GROUP:            return IND_GROUP;
    case CATEG_DRAWING:          return IND_DRAW;
    case CATEG_SERVER:         //return IND_REPLSRV;
    case CATEG_REPLREL:          return IND_REPLSRV;     
    case CATEG_PROC:             return IND_PROC;
    case CATEG_TRIGGER:          return IND_TRIGGER;
//    case CATEG_DUMMY:            return IND_DUMMY;
    case CATEG_SEQ:              return IND_SEQ;
    case CATEG_FOLDER:           return IND_FOLDER;
    case CATEG_DOMAIN:           return IND_DOM;
    case CATEG_INFO:             return (flags & CO_FLAG_FT_SEP) ? IND_FULLTEXT_SEP : IND_FULLTEXT;
  }
  return 0;
}
//
// Command event handler
//
void ControlPanelList::OnCPCommand( wxCommandEvent& event )
{ my_control_panel->frame->OnCPCommand( event );
  // SetFocus();  // takes the focus back -- this is wrong, command may have opened a window which must have the focus!
}

static t_column_schema column_schema_empty[] =
{ { NULL, 0 } };

static t_column_schema column_schema_unnamed[] =
{ { wxT(" "), 300 }, { NULL, 0 } };

static t_column_schema column_schema_name_only[] =
{ { wxTRANSLATE("Name"), 110 }, { NULL, 0 } };

static t_column_schema column_schema_name_modif[] =
{ { wxTRANSLATE("Name"), 110 }, { wxTRANSLATE("Modified on"), 120 }, { NULL, 0 } };

static t_column_schema column_schema_server[] =
{ { wxTRANSLATE("Name"), 110 }, { wxTRANSLATE("Type"), 54 }, { wxTRANSLATE("Logged in as"), 100 }, { wxTRANSLATE("Path or address"), 240 }, /*{ wxT("Nest installation key"), 190 },*/ { NULL, 0 } };

static t_column_schema column_schema_dsn[] =
{ { wxTRANSLATE("Name"), 110 }, { wxTRANSLATE("Type"), 54 }, { wxTRANSLATE("Logged in as"), 100 }, { wxTRANSLATE("Driver"), 240 }, /*{ wxT("Nest installation key"), 190 },*/ { NULL, 0 } };
//
// Sets listview column schema
//
void ControlPanelList::set_column_schema(t_column_schema * new_column_schema)
{ 
    if (new_column_schema==column_schema)
        return;  // no change
    column_schema = new_column_schema;

    int old_columns_cnt = GetColumnCount(), curr_column;
    // FOR each item in schema, update listview column
    for (curr_column = 0; new_column_schema->colname!=NULL; curr_column++, new_column_schema++)
    {
        wxListItem itemCol;
        itemCol.m_mask   = wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT | wxLIST_MASK_WIDTH;
        itemCol.m_format = wxLIST_FORMAT_LEFT;
        wxString colname_transl = wxGetTranslation(new_column_schema->colname);
        itemCol.m_text   = colname_transl;
        itemCol.m_width  = new_column_schema->colwidth;
        if (curr_column  < old_columns_cnt)  // modify the existing column
            SetColumn(curr_column, itemCol);
        else
            InsertColumn(curr_column, itemCol);
    }
    // delete unused existing columns:
    for ( ; old_columns_cnt > curr_column; old_columns_cnt--)
        DeleteColumn(old_columns_cnt-1);
}

static char ListSchemaUnnamed[]   = "List schema unnamed";
static char ListSchemaNameOnly[]  = "List schema name only";
static char ListSchemaNameModif[] = "List schema name modif";
static char ListSchemaServer[]    = "List schema server";
static char LayoutNormal[]        = "LayoutNormal";
//
// Loads column widths from inifile
//
void ControlPanelList::LoadLayout()
{
    char Buf[256];

    if (read_profile_string(LayoutNormal, ListSchemaUnnamed,   Buf, sizeof(Buf)))
        sscanf(Buf, " %d", &column_schema_unnamed[0].colwidth);
    if (read_profile_string(LayoutNormal, ListSchemaNameOnly,  Buf, sizeof(Buf)))
        sscanf(Buf, " %d", &column_schema_name_only[0].colwidth);
    if (read_profile_string(LayoutNormal, ListSchemaNameModif, Buf, sizeof(Buf)))
        sscanf(Buf, " %d %d", &column_schema_name_modif[0].colwidth, &column_schema_name_modif[1].colwidth);
    if (read_profile_string(LayoutNormal, ListSchemaServer,    Buf, sizeof(Buf)))
        sscanf(Buf, " %d %d %d %d", &column_schema_server[0].colwidth, &column_schema_server[1].colwidth, &column_schema_server[2].colwidth, &column_schema_server[3].colwidth/*, &column_schema_server[4].colwidth*/);
}
//
// Saves column widths from inifile
//
void ControlPanelList::SaveLayout()
{
    char Buf[128];
    sprintf(Buf, "%d", column_schema_unnamed[0].colwidth);
    write_profile_string(LayoutNormal, ListSchemaUnnamed,   Buf);
    sprintf(Buf, "%d", column_schema_name_only[0].colwidth);
    write_profile_string(LayoutNormal, ListSchemaNameOnly,  Buf);
    sprintf(Buf, "%d %d", column_schema_name_modif[0].colwidth, column_schema_name_modif[1].colwidth);
    write_profile_string(LayoutNormal, ListSchemaNameModif, Buf);
    sprintf(Buf, "%d %d %d %d", column_schema_server[0].colwidth, column_schema_server[1].colwidth, column_schema_server[2].colwidth, column_schema_server[3].colwidth/*, column_schema_server[4].colwidth*/);
    write_profile_string(LayoutNormal, ListSchemaServer,    Buf);
}


void ControlPanelList::update_server_data(int ind, const char * server_name, const t_avail_server * as)
// Updates server item in the ControlPanelList
// [as] may be NULL.
{ //char ik[MAX_LICENCE_LENGTH+1];  
  char path[MAX_PATH+1];
  SetItem(ind, 2, as && as->conn ? 
                    as->conn->uid ? wxString(as->conn->uid, *wxConvCurrent).c_str() : _("N/A") : 
    as && as->cdp ? wxString(cd_Who_am_I(as->cdp), *as->cdp->sys_conv).c_str() : 
    _("Not connected"));
  if (as && as->odbc)
    strmaxcpy(path, as->description, sizeof(path));
  else
  { GetPrimaryPath(server_name, as && as->private_server, path);
    if (!*path && as)
    { sprintf(path, "%u.%u.%u.%u:%u", as->ip & 255, (as->ip>>8)&255, (as->ip>>16)&255, as->ip>>24, as->port);
      if (as && as->cdp && !as->notreg)  // add the remote database name unless included in the local name
        sprintf(path+strlen(path), " %s", as->cdp->conn_server_name);  
    }
  }
  SetItem(ind, 3, wxString(path, *wxConvCurrent));
  //SetItem(ind, 4, wxString(ik,   *wxConvCurrent));
 // update image:
  if (as)
    SetItemImage(ind, as->server_image, as->server_image);
}
//
// Fills listview with object list
//
void ControlPanelList::Fill()
{
    if (m_NoRelist)         // List filling is blocked during tree filling
        return;
    bool     ItemFound;

    wxBusyCursor Busy;
    Freeze();               // Prevent list redrawing while filling 
    m_SelIndex = -1;        // Reset current item
    DeleteAllItems();       // Clear old content
    my_control_panel->frame->update_object_info(my_control_panel->frame->output_window->GetSelection());
    // IF root tree item is Servers OR ODBC datasources
    if (m_BranchInfo->topcat == TOPCAT_SERVERS || m_BranchInfo->topcat == TOPCAT_ODBCDSN)
    {
        // IF no server OR ODBC datasource selected
        if (!*m_BranchInfo->server_name)
        {
            // Set column schema
            set_column_schema(m_BranchInfo->topcat == TOPCAT_ODBCDSN ? column_schema_dsn : column_schema_server);
            // Clear translation table from object ID (object ID is item index) to object name
            m_SpecList.Clear();
            CObjectList ol;
            tcateg Categ = m_BranchInfo->topcat == TOPCAT_ODBCDSN ? XCATEG_ODBCDS : XCATEG_SERVER;
            // FOR each item in ObjectList
            for (ItemFound = ol.FindFirst(Categ); ItemFound; ItemFound = ol.FindNext())
            {
                t_avail_server *Server = (t_avail_server *)ol.GetSource();
                wxString ServerName = wxString(Server->locid_server_name, *wxConvCurrent);
                // Add object name to translation table
                size_t ind = m_SpecList.Add(ServerName);
                // Insert item to listview
                size_t tmp = Insert(ServerName, Server->server_image, (long)ind);
                // Set type column
                SetItem((long)tmp, 1, Server->odbc ? _("ODBC Data Source") : Server->local ? LOCAL_SERVER : NETWORK_SERVER);
                // Set resting columns
                update_server_data((int)tmp, Server->locid_server_name, Server);
            }
        }
        // ELSE server OR ODBC datasource selected in tree
        else
        {   // find the server
            t_avail_server *as = find_server_connection_info(m_BranchInfo->server_name);
            if (!as)
            {
                Thaw();
                return; // internal error?
            }
            // IF source is not connected, insert "Not connected" message to the list
            if (!as->cdp && !as->conn) 
            { 
                set_column_schema(column_schema_unnamed);
                Insert(NOT_CONNECTED_TO_THE_SERVER, as->server_image, IDT_NOTCONNECTED, 0);
            }
            // IF source connected
            else if (!m_BranchInfo->cdp || cd_Sz_error(m_BranchInfo->cdp) != CONNECTION_LOST)
            {   // Reset object and folder ObjectList
                m_ObjectList.SetCdp(m_BranchInfo->cdp, m_BranchInfo->schema_name, m_BranchInfo->conn);
                m_FolderList.SetCdp(m_BranchInfo->cdp, m_BranchInfo->schema_name, m_BranchInfo->conn);
                CObjectList ol(m_BranchInfo->cdp, m_BranchInfo->schema_name, m_BranchInfo->conn);
                // IF schema list requested
                if (m_BranchInfo->syscat == SYSCAT_APPLS)
                {
                    // Set column schema
                    set_column_schema(column_schema_name_only);
                    // Insert .. and system folder item
                    if (m_BranchInfo->cdp)
                    { 
                        Insert(CATNAME_UPFOLDER, IND_UPFOLDER,      IDT_UPFOLDER,     0);
                        if (m_BranchInfo->cdp)
                            Insert(CATNAME_SYSTEM, IND_FOLDER_SYSTEM, IDT_SYSTEMFOLDER, 1);
                    }
                    // Insert item for each schema
                    for (ItemFound = ol.FindFirst(CATEG_APPL); ItemFound; ItemFound = ol.FindNext())
                        Insert(GetSmallStringName(ol), IND_APPL, ol.GetObjnum());
                }
                // IF control console
                else if (m_BranchInfo->syscat == SYSCAT_CONSOLE)
                {
                    // IF system folder content requested
                    if (m_BranchInfo->cti == IND_FOLDER_SYSTEM)
                    { set_column_schema(column_schema_unnamed);
                      // Insert .., Licenses, Runtime parameters, Protocols, System objects, Tools items
                      if (m_BranchInfo->cdp)
                      { Insert(CATNAME_UPFOLDER, IND_UPFOLDER,       IDT_UPFOLDER,     0);
#if WBVERS<1100
                        Insert(CTI_LICENCES,     IND_LICENCES,       IDT_LICENCES,     1);
#endif                        
                        Insert(CTI_RUNPROP,      IND_RUNPROP,        IDT_RUNPROP,      2);
                        Insert(CTI_PROTOCOLS,    IND_PROTOCOLS,      IDT_PROTOCOLS,    3);
                        Insert(CTI_SYSOBJS,      IND_FOLDER_SYSOBJS, IDT_SYSOBJS,      4);
                        Insert(CTI_TOOLS,        IND_FOLDER_TOOLS,   IDT_TOOLS,        5);
                      }
                    }
                    // IF tools folder content requested
                    else if (m_BranchInfo->cti == IND_FOLDER_TOOLS)
                    {   set_column_schema(column_schema_unnamed);
                        Insert(CATNAME_UPFOLDER, IND_UPFOLDER,       IDT_UPFOLDER,     0);
                        Insert(CTI_BACKUP,       IND_BACKUP,         IDT_BACKUP,       1);
                        //Insert(CTI_REPLAY,       IND_REPLAY,         IDT_REPLAY,       2);
                        Insert(CTI_MESSAGE,      IND_MESSAGE,        IDT_MESSAGE,      2);
                        Insert(CTI_CONSISTENCY,  IND_CONSISTENCY,    IDT_CONSISTENCY,  3);
                        Insert(CTI_FILENC,       IND_FILENC,         IDT_FILENC,       4);
                        Insert(CTI_SRVCERT,      IND_SRVCERT,        IDT_SRVCERT,      5);
                        //Insert(CTI_COMPACT,      IND_COMPACT,        IDT_COMPACT,      4);
                        Insert(CTI_SERV_RENAME,  IND_SERV_RENAME,    IDT_SERV_RENAME,  6);
                        Insert(CTI_EXPORT_USERS, IND_EXP_USERS,      IDT_EXP_USERS,    7);
                        Insert(CTI_IMPORT_USERS, IND_IMP_USERS,      IDT_IMP_USERS,    8);
                        Insert(CTI_PROFILER,     IND_PROFILER,       IDT_PROFILER,     9);
                        int num=10;
#if WBVERS>=1100
                        Insert(CTI_EXTENSIONS,   IND_LICENCES,       IDT_EXTENSIONS,   num++);
#endif                        
#ifdef XMLFORMS
                        Insert(CTI_HTTP,         IND_HTTP,           IDT_HTTP,         num++);
#endif
                    }
                    // IF System objects folder content rquested
                    else if (m_BranchInfo->cti == IND_FOLDER_SYSOBJS)
                    {
                        wchar_t wc[64];
                        Insert(CATNAME_UPFOLDER, IND_UPFOLDER, IDT_UPFOLDER);
                        Insert(CCategList::Plural(m_BranchInfo->cdp, CATEG_USER, wc, true),  CategItemDescr[CATEG_USER].m_Image,   CategItemDescr[CATEG_USER].m_ItemData, 1);
                        Insert(CCategList::Plural(m_BranchInfo->cdp, CATEG_GROUP, wc, true), CategItemDescr[CATEG_GROUP].m_Image,  CategItemDescr[CATEG_GROUP].m_ItemData, 2);
                        //Insert(CCategList::CaptFirst(CATEG_SERVER), CategItemDescr[CATEG_SERVER].m_Image, CategItemDescr[CATEG_SERVER].m_ItemData, 3);
                        Insert(CCategList::Plural(m_BranchInfo->cdp, CATEG_TABLE, wc, true), IND_SYSTABLE,                         IDT_SYSTABLE, 3);
                    }
                    // ELSE Runtime parameters, Protocols or tool selected in tree  
                    else
                    { set_column_schema(column_schema_unnamed);
                      Insert(_("Double-click the tree item to open"), m_BranchInfo->cti, IDT_NOTCONNECTED, 0);
                    }
                }
                // IF list of object categories requested
                else if (m_BranchInfo->syscat == SYSCAT_APPLTOP)
                {
                    set_column_schema(column_schema_name_only);
                    // Insert .. item
                    Insert(CATNAME_UPFOLDER, IND_UPFOLDER,      IDT_UPFOLDER,     0);
                    // IF categories below folders, insert folders (602SQL only)
                    if (my_control_panel->ShowFolders && !my_control_panel->fld_bel_cat && m_BranchInfo->cdp)
                    {
                        for (ItemFound = ol.FindFirst(CATEG_FOLDER, m_BranchInfo->folder_name); ItemFound; ItemFound = ol.FindNext())
                        {
                            if (my_control_panel->ShowEmptyFolders || (GetFolderState(m_BranchInfo->cdp, m_BranchInfo->schema_name, ol.GetName(), true, true) != FST_EMPTY))  
                                Insert(GetSmallStringName(ol), IND_FOLDER, ol.GetIndex() | IDT_FOLDER);
                        }
                    }
                    CCategList cl;
                    // Insert categories
                    do
                    {  if (m_BranchInfo->cdp || cl.GetCateg()==CATEG_TABLE || cl.GetCateg()==CATEG_CURSOR)
                        if (cl.HasFolders() || IsRootFolder(m_BranchInfo->folder_name))
                            InsertCategorie(cl.GetCateg());
                    }
                    while (cl.FindNext());
                }
                // IF object list requested
                else if (m_BranchInfo->syscat==SYSCAT_CATEGORY)  
                {
                    // IF system category
                    if (m_BranchInfo->IsSysCategory()) // listing only the system objects, not the console dialogs
                    {
                        set_column_schema(column_schema_name_only);
                        Insert(CATNAME_UPFOLDER, IND_UPFOLDER, IDT_UPFOLDER, 0);
                        if (m_BranchInfo->category == CATEG_TABLE)  // list of system tables
                        {
                            Insert(wxT("Tabtab"),  IND_SYSTABLE, TAB_TABLENUM,  1);
                            Insert(wxT("Objtab"),  IND_SYSTABLE, OBJ_TABLENUM,  2);
                            Insert(wxT("Usertab"), IND_SYSTABLE, USER_TABLENUM, 3);
                            //Insert(wxT("Srvtab"),  IND_SYSTABLE, SRV_TABLENUM,  4);
                            //Insert(wxT("Repltab"), IND_SYSTABLE, REPL_TABLENUM, 5);
                            //Insert("KeyTab",  CATEG_TABLE, 0 , KEY_TABLENUM);
                        }
                        else // users, group, replication servers
                        {
                            for (ItemFound = ol.FindFirst(m_BranchInfo->category); ItemFound; ItemFound = ol.FindNext())
                                InsertObject(GetSmallStringName(ol), m_BranchInfo->category, ol.GetFlags(), ol.GetObjnum(), ol.GetModif());
                        }
                    }
                    // ELSE ordinary object list
                    else  
                    {
                        set_column_schema(column_schema_name_modif);
                        // .. Item
                        Insert(CATNAME_UPFOLDER, IND_UPFOLDER,      IDT_UPFOLDER,     0);
                        // IF folders below categories, insert folders
                        if (my_control_panel->ShowFolders && my_control_panel->fld_bel_cat && CCategList::HasFolders(m_BranchInfo->category) && m_BranchInfo->cdp)
                        {
                            for (ItemFound = ol.FindFirst(CATEG_FOLDER, m_BranchInfo->folder_name); ItemFound; ItemFound = ol.FindNext())
                            {
                                if (my_control_panel->ShowEmptyFolders || GetFolderState(m_BranchInfo->cdp, m_BranchInfo->schema_name, ol.GetName(), m_BranchInfo->category, true, true) != FST_EMPTY) 
                                    Insert(GetSmallStringName(ol), IND_FOLDER, ol.GetIndex() | IDT_FOLDER);
                            }
                        }
                        // Insert objects
                        for (ItemFound = ol.FindFirst(m_BranchInfo->category, my_control_panel->ShowFolders ? m_BranchInfo->folder_name : NULL); ItemFound; ItemFound = ol.FindNext())
                            InsertObject(GetSmallStringName(ol), m_BranchInfo->category, ol.GetFlags(), ol.GetObjnum(), ol.GetModif());
                    }
                }
            } // server connected
        } // server specified
    }
    // IF root tree item is client settings
    else if (m_BranchInfo->topcat == TOPCAT_CLIENT) 
    { 
        // IF client settings item is selected in tree
        if (m_BranchInfo->cti == IND_CLIENT)
        {   set_column_schema(column_schema_unnamed);
            Insert(_("Fonts"),               IND_FONTS,   IDT_FONTS,   0);
            Insert(_("Grid Data Format"),    IND_FORMATS, IDT_FORMATS, 1);
            Insert(_("Locale and Language"), IND_LOCALE,  IDT_LOCALE,  2);
            Insert(_("Fulltext Plug-ins"),    IND_FTXHELPERS, IDT_FTXHELPERS, 3);
            if (ODBCConfig_available)
              Insert(_("ODBC Manager"),      IND_ODBCTAB, IDT_ODBC,    4);
            Insert(_("Mail Profiles"),       IND_MAIL,    IDT_MAIL,    5);
            Insert(_("Communication to server"), IND_COMM,IDT_COMM,    6);
            Insert(_("External Editors"),    IND_EDITORS, IDT_EDITORS, 7);
        }
        // IF mail profiles item is selected in tree
        else if (m_BranchInfo->cti == IND_MAIL)
        {   set_column_schema(column_schema_name_only);
            CMailProfList fl;
            m_SpecList.Clear();
            for (bool Found = fl.FindFirst(); Found; Found = fl.FindNext())
            {
                wxString Profile = fl.GetName();
                size_t ind = m_SpecList.Add(Profile);
                Insert(Profile, IND_MAILPROF, (long)ind);
            }
        }
        // Other local setting items
        else
        { set_column_schema(column_schema_unnamed);
          Insert(_("Double-click the tree item to open"), m_BranchInfo->cti, IDT_NOTCONNECTED, 0);
        }
    }
    Thaw();
}
//
// Inserts category item to list
//
void ControlPanelList::InsertCategorie(tcateg Categ)
{
    CObjectList ol(m_BranchInfo->cdp, m_BranchInfo->schema_name, m_BranchInfo->conn);
    // IF any object of given category in current folder, insert category item
    if (my_control_panel->ShowEmptyFolders || (GetFolderState(m_BranchInfo->cdp, m_BranchInfo->schema_name, m_BranchInfo->folder_name, Categ, true, my_control_panel->fld_bel_cat) != FST_EMPTY))  
    {
        wchar_t wc[64];
        Insert(CCategList::Plural(m_BranchInfo->cdp, Categ, wc, true), CategItemDescr[Categ].m_Image, CategItemDescr[Categ].m_ItemData, LII_APPEND);
    }
}
//
// Refreshes object list after database change
//
void ControlPanelList::Refresh()
{
    bool ItemFound;

    wxBusyCursor Busy;
    m_InRefresh = true;     // Prevent current item change actions during list refresh
    m_ObjectList.Release(); // Reset ObjectList
    Freeze();               // Prevent list redrawing while refreshing
    // IF root tree item is Servers OR ODBC datasources
    if (m_BranchInfo->topcat == TOPCAT_SERVERS || m_BranchInfo->topcat == TOPCAT_ODBCDSN)
    {
        // IF Servers OR ODBC datasources item is selected in tree
        if (!*m_BranchInfo->server_name)
        {
            // I can't ensure uniqueness of object ID in ItemData, so I read whole list again
            CObjectList ol;
            DeleteAllItems();  
            m_SpecList.Clear();
            tcateg Categ = m_BranchInfo->topcat == TOPCAT_ODBCDSN ? XCATEG_ODBCDS : XCATEG_SERVER;
            for (ItemFound = ol.FindFirst(Categ); ItemFound; ItemFound = ol.FindNext())
            {
                t_avail_server *Server = (t_avail_server *)ol.GetSource();
                size_t ind = m_SpecList.Add(wxString(Server->locid_server_name, *wxConvCurrent));
                size_t tmp = Insert(wxString(Server->locid_server_name, *wxConvCurrent), Server->server_image, (long)ind);
                SetItem((long)tmp, 1, Server->local ? LOCAL_SERVER : NETWORK_SERVER);
                update_server_data((int)tmp, Server->locid_server_name, Server);
            }
        }
        // ELSE server OR ODBC datasource selected in tree
        else
        {   // find the server
            t_avail_server *as = find_server_connection_info(m_BranchInfo->server_name);
            if (!as)
            {
                m_InRefresh = false;
                Thaw();
                return; // internal error?
            }
            // IF source is not connected, insert "Not connected" message to the list
            if (!as->cdp && !as->conn) 
            { 
                if (GetItemCount() != 1 || GetItemData(0) != IDT_NOTCONNECTED)  // GetItemCount() may be 0 after unsuccessfull connection to the server
                {
                    DeleteAllItems();
                    set_column_schema(column_schema_unnamed);
                    Insert(NOT_CONNECTED_TO_THE_SERVER, as->server_image, IDT_NOTCONNECTED, 0);
                }
            }
            // IF server connected
            else 
            {   
                // IF cdp changed
                if (m_BranchInfo->cdp != as->cdp || m_BranchInfo->conn != as->conn || !m_ObjectList.GetCdp())
                { 
                    // Reset cdp in selected tree item info
                    m_BranchInfo->cdp = as->cdp;
                    m_BranchInfo->conn = as->conn;
                    // Reset object and folder ObjectList
                    m_ObjectList.SetCdp(m_BranchInfo->cdp, m_BranchInfo->schema_name, m_BranchInfo->conn);
                    m_FolderList.SetCdp(m_BranchInfo->cdp, m_BranchInfo->schema_name, m_BranchInfo->conn);
                    // Clear old list content
                    DeleteAllItems();
                }
                CObjectList ol(m_BranchInfo->cdp, m_BranchInfo->schema_name, m_BranchInfo->conn);
                // Schema list
                if (m_BranchInfo->syscat == SYSCAT_APPLS)
                {
                    // IF server just connected, fill schema list
                    if (GetItemCount() && GetItemData(0) == IDT_NOTCONNECTED)
                    {
                        Fill();
                    }
                    // ELSE old schema list refresh
                    else
                    {
                        // FOR each item excluding .., if not found in ObjectList, remove
                        for (long i = 2; i < GetItemCount();)
                        {
                            if (ol.Find(GetItemText(i).mb_str(*as->cdp->sys_conv), CATEG_APPL))
                                i++;
                            else
                                DeleteItem(i);
                        }
                        // Insert items existing in ObjectList and not found in listview
                        for (ItemFound = ol.FindFirst(CATEG_APPL); ItemFound; ItemFound = ol.FindNext())
                        {
                            if (FindItem(1, ol.GetObjnum()) == -1)
                                Insert(GetSmallStringName(ol), IND_APPL, ol.GetObjnum());
                        }
                    }
                }
                // Categories list
                else if (m_BranchInfo->syscat == SYSCAT_APPLTOP)
                {
                    // IF categories below folders, traverse folders
                    if (my_control_panel->ShowFolders && !my_control_panel->fld_bel_cat)
                    { if (m_BranchInfo->cdp)
                      { // Remove deleted folder
                        for (long i = 1; i < GetItemCount() && IsFolder((long)GetItemData(i));)
                        {
                            if (ol.Find(GetItemText(i).mb_str(*as->cdp->sys_conv), CATEG_FOLDER) && (my_control_panel->ShowEmptyFolders || GetFolderState(m_BranchInfo->cdp, m_BranchInfo->schema_name, ol.GetName(), true, true) != FST_EMPTY))
                                i++;
                            else
                                DeleteItem(i);
                        }
                        // Inser new folders
                        for (ItemFound = ol.FindFirst(CATEG_FOLDER, m_BranchInfo->folder_name); ItemFound; ItemFound = ol.FindNext())
                        {
                            if (FindItem(-1, ol.GetIndex() | IDT_FOLDER) == -1 && (my_control_panel->ShowEmptyFolders || GetFolderState(m_BranchInfo->cdp, m_BranchInfo->schema_name, ol.GetName(), true, true) != FST_EMPTY))
                                Insert(GetSmallStringName(ol), IND_FOLDER, ol.GetIndex() | IDT_FOLDER);
                        }
                      }
                    }
                    // Refresh categories list
                    CCategList cl;
                    do
                    {
                        if (cl.HasFolders() || IsRootFolder(m_BranchInfo->folder_name))
                            RefreshCategorie(CATEG_TABLE);
                    }
                    while (cl.FindNext());
                }
                // Object list
                else if (m_BranchInfo->syscat == SYSCAT_CATEGORY)  
                {
                    // System objects
                    if (!*m_BranchInfo->schema_name) // listing only the system objects, not the console dialogs
                    {   // users, group, replication servers
                        if (m_BranchInfo->category != CATEG_TABLE)
                        {
                            // Remove deleted objects
                            for (long i = 1; i < GetItemCount();)
                            {
                                if (ol.Find(GetItemText(i).mb_str(*as->cdp->sys_conv), m_BranchInfo->category))
                                    i++;
                                else
                                    DeleteItem(i);
                            }
                            // Insert new objects
                            for (ItemFound = ol.FindFirst(m_BranchInfo->category); ItemFound; ItemFound = ol.FindNext())
                            {
                                if (FindItem(-1, ol.GetObjnum()) == -1)
                                   InsertObject(GetSmallStringName(ol), m_BranchInfo->category, ol.GetFlags(), ol.GetObjnum(), ol.GetModif());
                            }
                        }
                    }
                    // Ordinary objects
                    else  
                    {
                        int Folder = m_BranchInfo->get_folder_index();
                        // IF folders below categories, traverse folders
                        long FirstObj = 1;
                        if (my_control_panel->ShowFolders && my_control_panel->fld_bel_cat)
                        { if (m_BranchInfo->cdp)
                          { // Remove deleted folders
                            while (FirstObj < GetItemCount() && IsFolder((long)GetItemData(FirstObj)))
                            {
                                if (ol.Find(GetItemText(FirstObj).mb_str(*as->cdp->sys_conv), CATEG_FOLDER) && (my_control_panel->ShowEmptyFolders || GetFolderState(m_BranchInfo->cdp, m_BranchInfo->schema_name, ol.GetName(), true, true) != FST_EMPTY))
                                    FirstObj++;
                                else
                                    DeleteItem(FirstObj);
                            }
                            // Insert new folders
                            for (ItemFound = ol.FindFirst(CATEG_FOLDER, m_BranchInfo->folder_name); ItemFound; ItemFound = ol.FindNext())
                            {
                                if (FindItem(-1, ol.GetIndex() | IDT_FOLDER) == -1 && (my_control_panel->ShowEmptyFolders || GetFolderState(m_BranchInfo->cdp, m_BranchInfo->schema_name, ol.GetName(), m_BranchInfo->category, true, true) != FST_EMPTY)) 
                                {
                                    Insert(GetSmallStringName(ol), IND_FOLDER, ol.GetIndex() | IDT_FOLDER);
                                    FirstObj++;
                                }
                            }
                          }
                        }
                        // Remove deleted objects
                        for (long i = FirstObj; i < GetItemCount();)
                        {
                            if (ol.Find(GetItemText(i).mb_str(*as->cdp->sys_conv), m_BranchInfo->category, m_BranchInfo->folder_name))
                                i++;
                            else
                                DeleteItem(i);
                        }
                        // Insert new objects, update old objects
                        for (ItemFound = ol.FindFirst(m_BranchInfo->category, m_BranchInfo->folder_name); ItemFound; ItemFound = ol.FindNext())
                        {
                            long Item = -1;
                            // Try find object by object number
                            if (m_BranchInfo->cdp)
                                Item = FindItem(-1, ol.GetObjnum());
                            // IF not found, try find object by name
                            if (Item == -1)
                                Item = FindItem(-1, wxString(ol.GetName(), *wxConvCurrent));
                            // IF not found, insert new
                            if (Item == -1)
                                InsertObject(GetSmallStringName(ol), m_BranchInfo->category, ol.GetFlags(), ol.GetObjnum(), ol.GetModif());
                            // ELSE update
                            else
                            {
                                char buf[24];  
                                // Modif timestamp
                                if (!ol.GetModif() || ol.GetModif() == NONETIMESTAMP)
                                    *buf = 0;
                                else
                                    datim2str(ol.GetModif(), buf, 99);
                                SetItem(Item, 1, wxString(buf, *wxConvCurrent));
                                //int image = get_image_index(m_BranchInfo->category, ol.GetFlags());
                                //wxListItem iinfo;  
                                //iinfo.SetId(Item);  
                                //iinfo.m_mask  = wxLIST_MASK_IMAGE;
                                //GetItem(iinfo);
                                //if (iinfo.m_image != image)
                                //    SetItemImage(Item, image, image);
                            }
                        }
                    }
                }
            } // server connected
        } // server specified
    }
    // Mail profile list
    if (m_BranchInfo->topcat == TOPCAT_CLIENT && m_BranchInfo->cti == IND_MAIL)
    {
        // I can't ensure uniqueness of object ID in ItemData, so I read whole list again
        Fill();
    }
    if (my_control_panel->frame->output_window)
      my_control_panel->frame->update_object_info(my_control_panel->frame->output_window->GetSelection());
    m_InRefresh = false;
    Thaw();
}
//
// Refreshes category item in categories list
//
void ControlPanelList::RefreshCategorie(tcateg Categ)
{
    CObjectList ol(m_BranchInfo->cdp, m_BranchInfo->schema_name, m_BranchInfo->conn);
    // Check if category item exist in list view
    long Item  = FindItem(-1, CategItemDescr[Categ].m_ItemData);
    bool Empty = !my_control_panel->ShowEmptyFolders && GetFolderState(m_BranchInfo->cdp, m_BranchInfo->schema_name, m_BranchInfo->folder_name, Categ, true, false) == FST_EMPTY;
    if (Item != -1)
    {
        // IF no object of given category in current folder, remove item
        if (Empty)
            DeleteItem(Item);
    }
    else
    {
        // IF any object of given category in current folder, insert new item
        if (!Empty)
        {
            wchar_t wc[64];
            Insert(CCategList::Plural(m_BranchInfo->cdp, Categ, wc, true), CategItemDescr[Categ].m_Image, CategItemDescr[Categ].m_ItemData);
        }
    }
}
//
// Inserts an object item into the control panel list.
//
long ControlPanelList::InsertObject(const wxChar *Name, tcateg Categ, uns16 flags, tobjnum Objnum, uns32 modif_timestamp)
{
    long tmp = Insert(Name, get_image_index(Categ, flags), Objnum);
    if (Categ!=CATEG_USER && Categ!=CATEG_GROUP && Categ!=CATEG_SERVER) // must not write the non-existing column!
    { 
        char buf[24];
        if (!modif_timestamp || modif_timestamp == NONETIMESTAMP)
            *buf=0;
        else
            datim2str(modif_timestamp, buf, 99);
        SetItem(tmp, 1, wxString(buf, *wxConvCurrent));
    }
    return tmp;
}

tcpitemid ConsoleTable[] =
{
    IND_NONE,
    IND_LICENCES,           // IDT_LICENCES
    IND_RUNPROP,
    IND_PROTOCOLS,
    IND_FOLDER_SYSOBJS,     // IDT_SYSOBJS
//    IND_FOLDER_PARAMS,      // IDT_PARAMS
    IND_FOLDER_TOOLS        // IDT_TOOLS
};

void ControlPanelList::add_list_item_info(long item, item_info & info)
// info is supposed to be initialised by the m_BranchInfo before.
// Transforms [info] so that it describes the [item].
{ 
    info.category  = NONECATEG;
    info.tree_info = false;
    // IF root tree item is Servers OR ODBC datasources
    if (info.topcat == TOPCAT_SERVERS || info.topcat == TOPCAT_ODBCDSN)
    { 
        long Data = (long)GetItemData(item);
        // IF item is ..
        if (IsUpFolder(Data))
            info.cti = IND_UPFOLDER;
        // IF server is not selected in the tree, write server properties to info
        else if (!*info.server_name)
        {
            strcpy(info.server_name, GetItemText(item).mb_str(*wxConvCurrent));
            t_avail_server * as = find_server_connection_info(info.server_name);
            if (as)
            { info.cdp=as->cdp;  info.conn=as->conn; }
            wxListItem iinfo;  
            iinfo.SetId(item);  
            iinfo.m_mask  = wxLIST_MASK_IMAGE;
            GetItem(iinfo);
            info.cti      = (tcpitemid)iinfo.m_image; 
            info.flags    = info.IsRemServer() ? 1 : 0;  // must be AFTER setting info.cti, flags is 1 for remote servers, 0 for local
            info.syscat   = SYSCAT_APPLS;
            //info.category = CATEG_APPL;  // used when creating a new application
        }
        // IF listview contains schema list
        else if (m_BranchInfo->syscat == SYSCAT_APPLS)
        {
            // IF item is System folder 
            if (IsSystemFolder(Data))
            {
                info.syscat   = SYSCAT_CONSOLE;
                info.cti      = IND_FOLDER_SYSTEM;
            }
            // ELSE schema
            else if (info.xcdp())
            {
                strcpy(info.schema_name, GetItemText(item).mb_str(GetConv(info.cdp)));
                if (info.cdp) cd_Find_prefixed_object(info.cdp, NULL, info.schema_name, CATEG_APPL, &info.objnum);
                info.category = CATEG_APPL;
                info.flags    = 0;
                info.syscat   = SYSCAT_APPLTOP;
                info.cti      = IND_APPL;
            }
        }
        // IF listview contains categories list
        else if (m_BranchInfo->syscat == SYSCAT_APPLTOP)
        {
            // IF item is folder
            if (IsFolder(Data))
            {
                strcpy(info.folder_name, GetItemText(item).mb_str(*info.cdp->sys_conv));
                info.cti = IND_FOLDER;
            }
            // IF item is category
            else if (IsCategory(Data))
            {
                info.syscat   = SYSCAT_CATEGORY;
                info.category = CategFromData(Data);
                info.cti      = (tcpitemid)CategItemDescr[info.category].m_Image;
            }
        }
        // IF listview contains object list
        else if (m_BranchInfo->syscat == SYSCAT_CATEGORY)  // for both application categories and system categories
        {
            info.category = m_BranchInfo->category;
            if (IsFolder(Data))  // a folder
            {
                strcpy(info.folder_name, GetItemText(item).mb_str(*info.cdp->sys_conv));
                *info.object_name = 0;
                info.objnum       = NOOBJECT;
                info.cti          = IND_FOLDER;
            }
            // ELSE object
            else
            {
                strcpy(info.object_name, GetItemText(item).mb_str(GetConv(info.cdp)));
                info.syscat = SYSCAT_OBJECT;
                // IF system table
                if (m_BranchInfo->cti == IND_SYSTABLE)
                {
                    info.flags  = 0;
                    info.objnum = Data;
                    info.cti    = IND_SYSTABLE;
                }
                // ELSE other object
                else if (info.conn)  // do not load the list of ODBC objects again
                { info.flags  = 0;
                  info.objnum = NOOBJECT;  // internal error
                  info.cti = (tcpitemid)get_image_index(info.category, 0);
                }  
                else
                {
                    CObjectList ol(info.cdp, info.schema_name, info.conn);
                    if (ol.Find(info.object_name, info.category))
                    {
                        info.flags  = ol.GetFlags();
                        info.objnum = ol.GetObjnum();
                    }
                    else
                        info.objnum = NOOBJECT;  // internal error
                    info.cti = (tcpitemid)get_image_index(info.category, info.flags);
                }
            }
        }
        // IF listview show System folder content
        else if (m_BranchInfo->syscat == SYSCAT_CONSOLE)
        {
            // Root console
            if (!*m_BranchInfo->folder_name)
            {
                info.cti = ConsoleTable[Data & 0xFF];
                strcpy(info.folder_name, GetItemText(item).mb_str(*wxConvCurrent));
            }
            // Systemove objects
            else if (m_BranchInfo->IsSysObjectsFolder())
            {
                info.syscat   = SYSCAT_CATEGORY;
                info.category = CategFromData(Data);
                if (info.category == CATEG_TABLE)
                    info.cti  = IND_SYSTABLE;
                else
                    info.cti  = (tcpitemid)CategItemDescr[info.category].m_Image;
            }
            // Other system items
            else
            {
                wxListItem iinfo;
                iinfo.SetId(item);
                iinfo.m_mask  = wxLIST_MASK_IMAGE;
                GetItem(iinfo);
                info.cti      = (tcpitemid)iinfo.m_image;
            }
        }
    }
    // IF root tree item is client settings 
    else if (info.topcat==TOPCAT_CLIENT)
    { 
        wxListItem iinfo;
        iinfo.SetId(item);
        iinfo.m_mask = wxLIST_MASK_IMAGE;
        GetItem(iinfo);
        info.cti     = (tcpitemid)iinfo.m_image;
        // IF listview shows mail profiles list
        if (info.cti == IND_MAILPROF)
            strcpy(info.object_name, GetItemText(item).mb_str(*wxConvCurrent));
    }
}
//
// Updates list item
//
void ControlPanelList::update_cp_item(item_info & info, uns32 modtime)
{ int ind;
  if (info.topcat!=m_BranchInfo->topcat) return;
  switch (info.topcat)
  { case TOPCAT_SERVERS:
    case TOPCAT_ODBCDSN:
      switch (m_BranchInfo->syscat)
      { case SYSCAT_NONE:  // !*m_BranchInfo->server_name -- list of servers
        { ind = FindItem(-1, wxString(info.server_name, *wxConvCurrent));
          if (ind==-1) return;  // object not found
          t_avail_server * as = find_server_connection_info(info.server_name);
          update_server_data(ind, info.server_name, as);
          break;
        }
        case SYSCAT_APPLS:  // applications listed
          break;
        case SYSCAT_CATEGORY:
          if (my_stricmp(info.server_name, m_BranchInfo->server_name)) return;  // same server
          if (wb_stricmp(info.cdp, info.schema_name, m_BranchInfo->schema_name)) return;  // same schema (may be "")
          if (info.syscat!=SYSCAT_OBJECT) return;  // adding an object
          if (info.category!=m_BranchInfo->category) return;  // same category
          //if (my_stricmp(info.folder_name, m_BranchInfo->folder_name)) return;  // same folder (may be "") -- folder is not specified in info
          ind = FindItem(-1, wxString(info.object_name, *info.cdp->sys_conv));
          if (ind==-1) return;  // object not found
          int image = get_image_index(info.category, info.flags);
          SetItemImage(ind, image, image);
          char buf[24];  
          if (!modtime || modtime==NONETIMESTAMP) *buf=0;
          else datim2str(modtime, buf, 99);
          SetItem(ind, 1, wxString(buf, *wxConvCurrent));
         // if the item is selected, update the contents of Output pages (not done when multple objects selected)
          if (GetSelectedItemCount() == 1)
            if (ind == GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED))
              my_control_panel->frame->update_object_info(my_control_panel->frame->output_window->GetSelection());
          break;
      }
      break;
  }
}
//
// Selects new list item
//
void ControlPanelList::SelectItem(int ind)
{ 
    DeselectAll();
    SetItemState(ind, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
    EnsureVisible(ind);
}

#ifdef STOP  // replaced by selecting inside inserting a new item
void ControlPanelList::select_item_by_name(cdp_t cdp, const char * name)
{ int ind = FindItem(-1, wxString(name, GetConv(cdp));
  if (ind!=-1) SelectItem(ind);
}

void ControlPanelList::select_cp_item(item_info & info)
{ if (info.topcat!=m_BranchInfo->topcat) return;
  switch (info.topcat)
  { case TOPCAT_SERVERS:
    case TOPCAT_ODBCDSN:
      switch (m_BranchInfo->syscat)
      { case SYSCAT_APPLS:  // applications listed
          if (my_stricmp(info.server_name, m_BranchInfo->server_name)) return;  // same server
          if (info.syscat!=SYSCAT_APPLTOP) return;  // adding an application
          select_item_by_name(info.schema_name);
          break;
        case SYSCAT_CATEGORY:
          if (my_stricmp(info.server_name, m_BranchInfo->server_name)) return;  // same server
          if (my_stricmp(info.schema_name, m_BranchInfo->schema_name)) return;  // same schema (may be "")
          if (info.syscat!=SYSCAT_OBJECT) return;  // adding an object
          if (info.category!=m_BranchInfo->category) return;  // same category
          if (wb_stricmp(info.cdp, info.folder_name, m_BranchInfo->folder_name)) return;  // same folder (may be "")
          select_item_by_name(info.object_name);
          break;
      }
      break;
  }
}
#endif
//
// Right mouse down event handler
//
void ControlPanelList::OnRightMouseDown(wxMouseEvent & event)
{
    event.Skip();
#ifdef WINS
    if (GetKeyState(VK_APPS) & 1)
        return;
#endif

    wxPoint position = wxPoint(event.m_x, event.m_y);
    int  flags;
    // Set menu/toolbar commands acceptor
    my_control_panel->m_ActiveChild = (wxControl *)this;
    long hit = HitTest(position, flags);
    // IF mouse above list item
    if ((flags & wxLIST_HITTEST_ONITEM) && hit!=-1)
    { 
        // IF item not selected, deselect all other items, select current item
        if (!GetItemState(hit, wxLIST_STATE_SELECTED))
        {
            DeselectAll();
            SetItemState(hit, wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED, wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED);
        }
    }
    // ELSE outside any item, deselect all items
    else
    {
        DeselectAll();
    }
}
//
// Right mouse click event handler - shows popup menu
//
#ifdef LINUX
void ControlPanelList::OnRightClick(wxMouseEvent & event)
#else
void ControlPanelList::OnRightClick(wxCommandEvent & event)
#endif
{   
    wxPoint Position = 
#ifdef LINUX
    wxPoint(event.m_x, event.m_y);
#else
    ScreenToClient(wxGetMousePosition());
#endif
    ShowPopupMenu(Position);
}
//
// Shows popup menu
//
void ControlPanelList::ShowPopupMenu(const wxPoint &Position)
{
    // update info according to the number of selected objects:
    item_info info = *m_BranchInfo;
    info.object_count = GetSelectedItemCount();
    if (info.object_count >= 1)  // get name, find objnum etc. for the only or the first selected object
    {
        int sel = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        add_list_item_info(sel, info);
        if (info.IsUpFolder())
            return;
    }
    else
    {
        *info.object_name = 0;
        info.tree_info    = false;
        info.objnum       = NOOBJECT;
    }
    
    C602SQLDataObject Data;
    wxMenu * Popup = new wxMenu;
#ifdef LINUX
    info.make_control_panel_popup_menu(Popup, false, true);
#else
    info.make_control_panel_popup_menu(Popup, false, wxIsDragResultOk(info.CanPaste(Data.GetDataFromClipboard(), false)));
#endif
    PopupMenu(Popup, Position);
    delete Popup;
}

#ifdef LINUX
//
// Left mouse down event handler
//
void ControlPanelList::OnLeftMouseDown(wxMouseEvent &event)
{
#ifdef CONTROL_PANEL_INPLECE_EDIT
    if (m_EditItem != -1)
    {
        // Zahodit pokud prave editujeme aktualni polozku
        int  flags;
        long Item = HitTest(wxPoint(event.m_x, event.m_y), flags);
        m_EditSame = Item == m_EditItem;
    }
#endif

    // Set menu/toolbar commands acceptor
    my_control_panel->m_ActiveChild = (wxControl *)this;
    // Set keyboard focus
    SetFocus();
    event.Skip();
}

void ControlPanelList::SetFocus()
{
    ((wxWindow *)m_mainWin)->wxWindow::SetFocus();
}

#endif
//
// Set focus event handler - Sets menu/toolbar commands acceptor
//
void ControlPanelList::OnSetFocus(wxFocusEvent &event)
{
    my_control_panel->m_ActiveChild = (wxControl *)this;
#ifdef CONTROL_PANEL_INPLECE_EDIT
#ifdef LINUX
    if (m_ItemEdit.m_InEdit)
    {
        wxWindowList     &List = ((wxWindow *)m_mainWin)->GetChildren();
        wxWindowListNode *Node = List.GetFirst();
        if (Node)
            ((wxTextCtrl *)Node->GetData())->SetFocus();
        return;
    }        
#endif    
#endif
    event.Skip();
}
//
// List item selected event handler
//
void ControlPanelList::OnItemSelected(wxListEvent &event)
{
    event.Skip();
    if (m_InRefresh)                    // Ignore when during list refresh
        return;
    if (event.GetIndex() == m_SelIndex) // Ignore if selected item not changed
        return;
    if (!m_FromKeyboard)                // Ignore if from keyboard
        m_InSelChange = true;           // ELSE Initialize delayed selected item change action
}
//
// Timeout elapsed event handler - Initializes delayed selected item change action
//
void ControlPanelList::OnTimer(wxTimerEvent &event)
{
    m_InSelChange  = true;
    m_FromKeyboard = false;
}
//
// Idle event handler
//
// Selected item change action is especialy due to update_object_info call rather slow operation, 
// therefore it blocks fast listview scroll using up or down key. So any up or down key hit starts
// new short timeout, when timeout elapses, change action is provided for last selected item
// 
void ControlPanelList::OnInternalIdle()
{
    wxListCtrl::OnInternalIdle();
    
    if (m_InSelChange)
    {
        // Get item info
        item_info info    = *m_BranchInfo;
        info.object_count = GetSelectedItemCount();
        m_SelIndex        = -1;
        if (info.object_count >= 1)
        {
            long sel = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            add_list_item_info(sel, info);
            if (info.object_count == 1)
                m_SelIndex = sel;
        }
        MyFrame *frame = my_control_panel->frame;
        if (frame)  // update action menu
        {
            C602SQLDataObject Data;
#ifdef LINUX
            frame->update_action_menu_based_on_selection(true);
#else
            frame->update_action_menu_based_on_selection(wxIsDragResultOk(info.CanPaste(Data.GetDataFromClipboard(), false)));
#endif
            // updates data about selected objects(s) on the selected Output page
            frame->update_object_info(frame->output_window->GetSelection());
            wxMenuBar *menu = frame->GetMenuBar();
            menu->EnableTop(1, !info.IsUpFolder());
            update_CP_toolbar(frame, info);
        }
        m_InSelChange = false;
    }
#ifdef CONTROL_PANEL_INPLECE_EDIT
#ifdef LINUX
    if (m_EditWait)
    {
        if (wxGetLocalTimeMillis() < m_EditTimeout)
        {
            wxCommandEvent nextEvent(myEVT_INITEDIT, LIST_ID);
            OnInitEdit(nextEvent);
        }
        else
            m_EditWait = false;    
    }    
#endif
#endif
}

// For expandable items, must select it in the tree and expand (and refill the list with child items)
// For other items, performs the default action.
void ControlPanelList::OnItemActivated(wxListEvent & event)
{  
    item_info info = *m_BranchInfo;
    add_list_item_info(event.GetIndex(), info);
    if (info.IsUpFolder())
        tree_view->select_parent();
    else if (info.is_expandable())  // select te same item in the tree and expand
    {
        tree_view->select_child(info);  // refills this list view
        tree_view->expand_selected();
    }
    else // perform the default action
    { 
        if (info.topcat == TOPCAT_SERVERS || info.topcat == TOPCAT_ODBCDSN)
        {
            if (info.syscat == SYSCAT_OBJECT)
            { 
                switch (info.category)
                {
                case CATEG_TABLE:
                case CATEG_CURSOR:
                    open_object(this, info);
                    break;
                case CATEG_DOMAIN:
                case CATEG_SEQ:
                case CATEG_TRIGGER:
                case CATEG_USER:
                case CATEG_DRAWING:
                    modify_object(this, info, false);
                    break;
                case CATEG_INFO:
                case CATEG_PROC:
                case CATEG_PGMSRC:
                    run_object(wxGetApp().frame, info);
                    break;
                case CATEG_GROUP:
                    Edit_relation(info.cdp, wxGetApp().frame, CATEG_GROUP, info.objnum, CATEG_USER);
                    break;
#ifdef WINS
#ifdef XMLFORMS
                case CATEG_XMLFORM:
                    open_object(wxGetApp().frame, info);
                    break;
#endif
#endif
                }
            }
            else if (info.syscat == SYSCAT_CONSOLE)
                console_action(info.cti, info.cdp);
        }
        else if (info.topcat == TOPCAT_CLIENT)  // client
          client_action(my_control_panel, info);
    }
}

#if 0

void ControlPanelList::select_by_name(cdp_t cdp, const char * name)
{
    long i  = 0;
    long cnt = GetItemCount();
    while (i < cnt)
    {
        if (!wb_stricmp(cdp, name, GetItemText(i).mb_str(GetConv(cdp)))  // supposes that no items have been selected before - change if not true!
        {
            wxListItem iinfo;
            iinfo.m_mask      = wxLIST_MASK_STATE;
            iinfo.m_stateMask = iinfo.m_state = wxLIST_STATE_SELECTED;
            iinfo.m_itemId    = i;
            SetItem(iinfo);
            EnsureVisible(i);
            break;
        }
        i++;
    }
}

#endif

#ifdef CONTROL_PANEL_INPLECE_EDIT

void ControlPanelList::OnBeginLabelEdit(wxListEvent &event)
{ 
#ifdef LINUX
    if (m_EditSame)
    {
        event.Veto();
        m_EditSame = false;
        return;
    }    
#endif    
    item_info info = *m_BranchInfo;
    add_list_item_info(event.GetIndex(), info);
    if 
    (
        // Neni objekt ani schema ani slozka
        (!info.IsObject() && !info.IsSchema() && !info.IsFolder()) ||
        // Systemova nebo ODBC tabulka
        (info.IsObject(CATEG_TABLE) && (info.objnum <= REL_TABLENUM || (info.flags & CO_FLAG_ODBC))) ||
        // Anonymous nebo systemova skupina
        ((info.IsObject(CATEG_USER) || info.IsObject(CATEG_GROUP)) && info.objnum < FIXED_USER_TABLE_OBJECTS) ||
        // ODBC connection, replikacni server
        (info.IsObject(CATEG_CONNECTION) || info.IsObject(CATEG_SERVER)  || info.IsObject(CATEG_INFO) /*|| info.IsObject(CATEG_TRIGGER) || info.IsObject(CATEG_DOMAIN)*/) ||
        // Zasifrovany objekt
        (info.flags & CO_FLAG_ENCRYPTED)
    )
    {
        event.Veto();
        return;
    }

    tobjnum lobjnum = info.objnum;
    if ((info.flags & CO_FLAG_LINK) && cd_Find_prefixed_object(info.cdp, info.schema_name, info.object_name, info.category | IS_LINK, &lobjnum))
    {
        event.Veto();
        return;
    }
    if (!check_object_access(info.cdp, info.category, lobjnum, 1))
    {
        event.Veto();
        return;
    }
    char *OldName;

	m_InEdit = true;
    if (info.category == CATEG_APPL)
    {
        OldName     = info.schema_name;
        m_EditCateg = CATEG_APPL;
    }
    else if (info.IsFolder())
    {
        OldName     = info.folder_name;
        m_EditCateg = CATEG_FOLDER;
    }
    else
    {
        OldName     = info.object_name;
        m_EditCateg = info.category;
    }
    if (!*m_OldName)
        strcpy(m_OldName, OldName);
    m_EditItem      = event.GetIndex();
    m_EditCancelled = false;

#ifdef WINS

    // Protoze wxListCtrl bere potvrzeni puvodni hodnoty jako zruseni editace a pri zruseni editace ani nevyvola
    // wxEVT_COMMAND_LIST_END_LABEL_EDIT, potrehuji link na EditControl udalost si emulovat sam, ale wxWindows 2.4.2 vubec
    // neumeji vratit EditControl pokud editace nebyla vyvolana explicitne pomoci EditLabel, pokud tedy byla editace vyvolana
    // kliknutim na polozku, musim zacatek editace tady vetovat a editaci spustit explicitne o chvili pozdeji.
    if (!m_F2Edit)
    {
        event.Veto();
    }

#else
    m_EditTimeout = wxGetLocalTimeMillis() + 5000;
    m_EditWait    = true;
#endif

    wxCommandEvent nextEvent(myEVT_INITEDIT, LIST_ID);
    AddPendingEvent(nextEvent);
}

void ControlPanelList::OnInitEdit(wxCommandEvent &event)
{
#ifdef WINS // ###
    // IF neni EditControl, protoze byla editace vyvolana kliknutim, spustit explicitne
    wxTextCtrl *Edit = GetEditControl();
    if (!Edit)
    {
        m_F2Edit = true;
        EditLabel(m_EditItem);
    }
    // Kvuli rozliseni potvrzeni puvodni hodnoty od zruseni editace potrebuju udelat nejakou pseudozmenu
    else
    {
        wxString txt = GetItemText(m_EditItem);
        Edit->SetMaxLength(OBJ_NAME_LEN);
        Edit->WriteText(txt);
        Edit->SetSelection(-1, -1);
        Edit->PushEventHandler(&m_ItemEdit);
    }

#else  // LINUX

    wxWindowList     &List = ((wxWindow *)m_mainWin)->GetChildren();
    wxWindowListNode *Node = List.GetFirst();
    if (!Node)
        return;
    m_EditWait = false;
    wxTextCtrl *Edit = (wxTextCtrl *)Node->GetData();
    wxRect Rect = Edit->GetRect();
    int    inc  = PatchedGetMetric(wxSYS_SMALLICON_X) + 6;
    Edit->SetSizeHints(Rect.width - inc - 6, Rect.height);
    Edit->SetSize(Rect.x + inc, Rect.y, Rect.width - inc - 6, Rect.height);
    Edit->PushEventHandler(&m_ItemEdit);
    m_ItemEdit.m_InEdit = true;

    //wxCaret *Caret = Edit->GetCaret();
    //if (!Caret)
    //{
    //    long x, y;
    //    long Pos  = Edit->GetLastPosition();
    //    Edit->PositionToXY(Pos, &x, &y);
    //    wxSize sz = Edit->GetClientSize();
    //    Caret     = new wxCaret(Edit, 2, sz.y - 4);
    //    Edit->SetCaret(Caret);
    //    Caret->Move(x, y);
    //    Caret->Show();
    //    //Edit->SetCaret(Caret);
    //    //Caret->Show();
    //    //Caret = Edit->GetCaret();
    //}

#endif
}

void ControlPanelList::OnEndLabelEdit(wxListEvent &event)
{
    m_EditCancelled = event.IsEditCancelled();
    wxCommandEvent nextEvent(myEVT_AFTEREDIT, LIST_ID);
    AddPendingEvent(nextEvent);
}

void ControlPanelList::OnAfterEdit(wxCommandEvent &event)
{
    m_InEdit = false;
    // IF editace zrusena
    if (m_EditCancelled)
    {
        // IF vytvareni nove slozky, zrusit nove vlozenou polozku
        if (m_NewFolder)
        {
            m_NewFolder = false;
            DeleteItem(m_EditItem);
        }
        // ELSE vratit puvodni text
        else
        {
            SetItemText(m_EditItem, wxString(m_OldName, *m_BranchInfo->cdp->sys_conv));
        }
        m_EditItem = -1;
        *m_OldName=0;
        m_F2Edit   = false;
        return;
    }

    wxString NewName = GetItemText(m_EditItem);
    // IF nebyla zmena, nic
    if (!m_NewFolder && NewName==wxString(m_OldName, *m_BranchInfo->cdp->sys_conv))
    {
        m_EditItem  = -1;
        *m_OldName=0;
        m_NewFolder = false;
        m_F2Edit    = false;
#ifdef LINUX        
//        wxWindow::SetFocus();
#endif        
        return;
    }
    // Zkontrolovat platnost a unikatnost jmena
    if (!is_valid_object_name(m_BranchInfo->cdp, NewName.mb_str(*m_BranchInfo->cdp->sys_conv), my_control_panel->frame) || ObjNameExists(m_BranchInfo->cdp, NewName.mb_str(*m_BranchInfo->cdp->sys_conv), m_EditCateg))
    {
#ifdef LINUX
        m_EditSame = false;
#endif        
        SelectItem(m_EditItem);
        EditLabel(m_EditItem);
        return;
    }

    // IF Vytvorit novy folder 
    if (m_NewFolder)
    {
        if (!Create_folder(m_BranchInfo->cdp, NewName.mb_str(*m_BranchInfo->cdp->sys_conv), m_BranchInfo->folder_name))
        {
            CObjectList ol(m_BranchInfo->cdp);
            ol.Find(NewName.mb_str(*m_BranchInfo->cdp->sys_conv), CATEG_FOLDER);
            SetItemData(m_EditItem, ol.GetIndex() | IDT_FOLDER);
            tree_view->RefreshSelItem();
        }
        else
        {
            cd_Signalize2(m_BranchInfo->cdp, my_control_panel->frame);
            DeleteItem(m_EditItem);
        }
        m_NewFolder = false;
    }
    // ELSE prejmenovat
    else
    {
        if (Rename_object(m_BranchInfo->cdp, m_OldName, NewName.mb_str(*m_BranchInfo->cdp->sys_conv), m_EditCateg))
        {
            cd_Signalize2(m_BranchInfo->cdp, my_control_panel->frame);
            SetItemText(m_EditItem, wxString(m_OldName, *m_BranchInfo->cdp->sys_conv));
        }
        else
        {
            if (cd_Sz_warning(m_BranchInfo->cdp) == RENAME_IN_DEFIN)
                info_box(_("The object must be renamed in the definition as well"), my_control_panel->frame);
            else if (m_EditCateg == CATEG_PROC || m_EditCateg == CATEG_TRIGGER || m_EditCateg == CATEG_SEQ || m_EditCateg == CATEG_DOMAIN || m_EditCateg == CATEG_INFO)
                wxGetApp().frame->update_object_info(wxGetApp().frame->output_window->GetSelection());
        }
    }
#if wxUSE_UNICODE
    NewName.LowerCase();
    NewName.GetWritableChar(0) = NewName.Mid(0, 1).Upper().GetChar(0);
#else
    wb_small_string(m_BranchInfo->cdp, (char*)NewName.c_str(), true);
#endif
    SetItemText(m_EditItem, NewName);   // Opravit na prvni velke ostani mala
    m_EditItem = -1;
    *m_OldName=0;
    m_F2Edit   = false;
    if (m_EditCateg == CATEG_FOLDER)
        tree_view->RefreshSchema(tree_view->m_SelInfo.server_name, tree_view->m_SelInfo.schema_name);
    else if (m_EditCateg == CATEG_APPL)
        tree_view->RefreshServer(tree_view->m_SelInfo.server_name);
#ifdef LINUX        
//    wxWindow::SetFocus();
#endif        
}

void ControlPanelList::NewFolder()
{
    cdp_t cdp = m_BranchInfo->cdp;
    tobjname Folder;
    strcpy(Folder, wxString(_("Folder")).mb_str(*cdp->sys_conv));
    if (!GetFreeObjName(cdp, Folder, CATEG_FOLDER))
    {
        cd_Signalize2(cdp, my_control_panel->frame);
        return;
    }
    long NewItem;
    long Cnt = GetItemCount();
    for (NewItem = 1; NewItem < Cnt && IsFolder(GetItemData(NewItem)); NewItem++);
    NewItem = Insert(wxString(Folder, *cdp->sys_conv), IND_FOLDER, IDT_FOLDER, NewItem);
    if (NewItem == -1)
        return;
    m_F2Edit    = true;
    m_NewFolder = true;
    EditLabel(NewItem);
}

void ControlPanelList::OnDuplicate(wxCommandEvent &event)
{
    tobjname ObjName;
    list_selection_iterator si(this);
    while (si.next())
    {
        if (si.info.IsFolder())
            continue;
        if (si.info.flags & CO_FLAG_ENCRYPTED)
          { wxBell();  continue; }
        char *def = cd_Load_objdef(si.info.cdp, si.info.objnum, si.info.category);
        if (!def)
        {
            cd_Signalize(si.info.cdp);
            continue;
        }
	    int Len = strlen(Src);
		CBuffer Buf;
		if (!Buf.Alloc(2 * Len);
		{
			no_memory();
    	    corefree(def);
			break;
		}
		else if (superconv(ATT_STRING, ATT_STRING, Src, Buf, t_specif(Len, cdp->sys_charset, 0, 0), t_specif(2 * Len, 7, 0, 0), NULL) < 0)
    	{
		    corefree(def);
			break;
		}		
        strcpy(ObjName, si.info.object_name);
        if (GetFreeObjName(si.info.cdp, ObjName, si.info.category))
        {
            if (ImportObject(si.info.cdp, si.info.category, Buf, ObjName, si.info.folder_name))
                cd_Signalize(si.info.cdp);
        }
        corefree(def);
    }
    Refresh();
    if (si.total_count() == 1)
    {
        long Item = FindItem(-1, wxString(ObjName, *si.info.cdp->sys_conv));
        if (Item != -1)
            EditLabel(Item);
    }
}

#else // !CONTROL_PANEL_INPLECE_EDIT

//
// Renames current item
//
void ControlPanelList::OnRename(wxCommandEvent &event)
{
    if (GetSelectedItemCount() != 1)
        return;
    // Get selected item
    long Item      = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    // Get item info
    item_info info = *m_BranchInfo;
    add_list_item_info(Item, info);
    // Object name and category
    tobjname Name;
    tcateg   Categ;
    if (info.IsSchema())
    {
        strcpy(Name, info.schema_name);
        Categ = CATEG_APPL;
    }
    else if (info.IsFolder())
    {
        strcpy(Name, info.folder_name);
        Categ = CATEG_FOLDER;
    }
    else
    {
        strcpy(Name, info.object_name);
        Categ = info.category;
    }
    // Rename object in database - refreshes current object list
    if (!RenameObject(info.cdp, Name, Categ))
        return;
    // Find object by new name and select it
    wb_small_string(info.cdp, Name, true);
    Item = FindItem(-1, wxString(Name, *m_BranchInfo->cdp->sys_conv));
    if (Item != -1)
        SelectItem(Item);
    // IF Folder, refresh folder list in tree
    if (Categ == CATEG_FOLDER)
        tree_view->RefreshSchema(tree_view->m_SelInfo.server_name, tree_view->m_SelInfo.schema_name);
    // IF Schema, refresh Schema list in tree
    else if (Categ == CATEG_APPL)
    {   
        tree_view->RefreshServer(tree_view->m_SelInfo.server_name);
        wxGetApp().frame->update_object_info(wxGetApp().frame->output_window->GetSelection());
    }
    else // most object have its name in the source text/information
        wxGetApp().frame->update_object_info(wxGetApp().frame->output_window->GetSelection());
}
//
// Creates new folder
//
void ControlPanelList::NewFolder()
{
    cdp_t cdp = m_BranchInfo->cdp;
    wxTreeItemId Parent = tree_view->m_SelItem;
    tobjname NewName;
    *NewName = 0;
    // Get new free folder name
    if (!get_name_for_new_object(cdp, wxGetApp().frame, cdp->sel_appl_name, CATEG_FOLDER, _("Enter the new folder name:"), NewName))
        return;

    // Create folder in database
    if (Create_folder(m_BranchInfo->cdp, NewName, m_BranchInfo->folder_name))
    {
        cd_Signalize(m_BranchInfo->cdp);
        return;
    }

    // Refresh control panel
    my_control_panel->RefreshSchema(m_BranchInfo->server_name, m_BranchInfo->schema_name);
    wxTreeItemIdValue cookie;
    // Find new folder in listview and select it
    for (wxTreeItemId Item = tree_view->GetFirstChild(Parent, cookie); Item.IsOk(); Item = tree_view->GetNextChild(Item, cookie))
    {
        if (wb_stricmp(cdp, tree_view->GetItemText(Item).mb_str(*cdp->sys_conv), NewName) == 0)
        {
            tree_view->SelectItem(Item);
            break;
        }
    }
}
//
// Duplicates selected objects
//
void ControlPanelList::OnDuplicate(wxCommandEvent &event)
{
    tobjname ObjName;
    list_selection_iterator si(this);
    // IF only one object selected
    if (si.total_count() == 1)
    {
        si.next();
        // Imposible for encrypted object
        if (si.info.flags & CO_FLAG_ENCRYPTED)
        { wxBell();  return; }
        // Get new free object name
        tobjname NewName;
        strcpy(NewName, si.info.object_name);
        if (!get_name_for_new_object(si.info.cdp, wxGetApp().frame, si.info.cdp->sel_appl_name, CATEG_FOLDER, _("Enter a name for the new object:"), NewName))
            return;
        // Load object definition
        char *def = cd_Load_objdef(si.info.cdp, si.info.objnum, si.info.category);
        if (!def)
        {
            cd_Signalize(si.info.cdp);
            return;
        }
	    int Len = (int)strlen(def);
		CBuffer Buf;
        char *pdef = def;
        // IF object has textual definition, convert definition to UNICODE
        if (CCategList::HasTextDefin(si.info.category))
        {
		    if (!Buf.Alloc(2 * Len))
		    {
			    no_memory();
    	        corefree(def);
                return;
		    }
            else if (superconv(ATT_STRING, ATT_STRING, def, Buf, t_specif(Len, si.info.cdp->sys_charset, 0, 0), t_specif(2 * Len, 7, 0, 0), NULL) < 0)
    	    {
		        corefree(def);
                return;
		    }
            pdef = Buf;
        }
        // Create new object
        if (ImportObject(si.info.cdp, si.info.category, pdef, NewName, si.info.folder_name, 0))
            cd_Signalize(si.info.cdp);
        corefree(def);
    }
    // ELSE multiple objects selected
    else
    {
        // FOR each selected object
        while (si.next())
        {
            // Ignore folders
            if (si.info.IsFolder())
                continue;
            // Imposible for encrypted object
            if (si.info.flags & CO_FLAG_ENCRYPTED)
            { wxBell();  continue; }
            // Load object definition
            char *def = cd_Load_objdef(si.info.cdp, si.info.objnum, si.info.category);
            if (!def)
            {
                cd_Signalize(si.info.cdp);
                continue;
            }
			int Len = (int)strlen(def);
			CBuffer Buf;
            char *pdef = def;
            // IF object has textual definition, convert definition to UNICODE
            if (CCategList::HasTextDefin(si.info.category))
            {
			    if (!Buf.Alloc(2 * Len))
			    {
				    no_memory();
				    corefree(def);
				    break;
			    }
			    else if (superconv(ATT_STRING, ATT_STRING, def, Buf, t_specif(Len, si.info.cdp->sys_charset, 0, 0), t_specif(2 * Len, 7, 0, 0), NULL) < 0)
			    {
				    corefree(def);
				    break;
			    }		
                pdef = Buf;
            }
            // Get new free object name
            strcpy(ObjName, si.info.object_name);
            if (GetFreeObjName(si.info.cdp, ObjName, si.info.category))
            {
                // Create new object
                if (ImportObject(si.info.cdp, si.info.category, pdef, ObjName, si.info.folder_name, 0))
                    cd_Signalize(si.info.cdp);
            }
            corefree(def);
        }
    }
    // Refresh object list
    Refresh();
}

#endif  // CONTROL_PANEL_INPLECE_EDIT

//
// Key down event handler
//
void ControlPanelList::OnKeyDown(wxKeyEvent & event)
{
#ifdef CONTROL_PANEL_INPLECE_EDIT
	if (m_InEdit)
	    return;
#endif
    int  Code     = event.GetKeyCode();
    bool CtrlDown = event.ControlDown();
    // Delete
    if (Code == WXK_DELETE)
    {
        // Ignore if no item is selected OR selected item is ..
        if (GetSelectedItemCount() == 0 || (GetSelectedItemCount() == 1 && GetItemText(GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) == wxT("..")))
            return;
        // Execute delete action
        wxCommandEvent cmd; // (wxEVT_COMMAND_BUTTON_CLICKED, CPM_DELETE) does not compile on GTK
        cmd.SetId(CPM_DELETE);
        wxGetApp().frame->OnCPCommand(cmd);
    }
    // F1 - shows help
    else if (Code == WXK_F1)
        wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/controlpanel.html"));
#ifdef CONTROL_PANEL_INPLECE_EDIT
    else if (Code == WXK_F2)
    {
        if (GetSelectedItemCount() == 1 && !m_InEdit)
        {
            m_F2Edit = true;
			m_InEdit = true;
            EditLabel(GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED));
        }
    }
#endif  // CONTROL_PANEL_INPLECE_EDIT
    // Ctrl/C
    else if (Code == 'C' && CtrlDown)
    {
        // Ignore if no item is selected OR selected item is ..
        if (GetSelectedItemCount() == 0 || (GetSelectedItemCount() == 1 && GetItemText(GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) == wxT("..")))
            return;
        // Execute copy action
        wxCommandEvent event;
        OnCopy(event);
    }
    // Ctrl/X
    else if (Code == 'X' && CtrlDown)
    {
        // Ignore if no item is selected OR selected item is ..
        if (GetSelectedItemCount() == 0 || (GetSelectedItemCount() == 1 && GetItemText(GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) == wxT("..")))
            return;
        // Execute cut action
        wxCommandEvent event;
        OnCut(event);
    }
    // Ctrl/V
    else if (Code == 'V' && CtrlDown)
    {
        // Execute paste action
        wxCommandEvent event;
        OnPaste(event);
    }
    // Menu
#ifdef WINS
    else if (Code == WXK_WINDOWS_MENU)
#else
    else if (Code == WXK_MENU)
#endif        
	{
		wxPoint Position;
		long Item = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (Item == -1)
		{
			Position.x = 60;
			Position.y = 40;
		}
		else
		{
			wxRect Rect;
			GetItemRect(Item, Rect);
			Position.x = 40;
			Position.y = Rect.y;
		}
        // Show popup menu
		ShowPopupMenu(Position);
	}
    // Down, Up
    else if (Code == WXK_DOWN || Code == WXK_UP)
    {
        // Initialize delayed selected item change action
        m_Timer.Stop();
        m_Timer.Start(220, true);
        m_FromKeyboard = true;
        event.Skip();
    }
    else
    {
        event.Skip();
    }
}
//
// Listview column resized event handler - updates column width in current column schema
//
void ControlPanelList::OnColResize(wxListEvent & event)
{
    column_schema[event.GetColumn()].colwidth = GetColumnWidth(event.GetColumn());
}

CPListOrder ServerOrder[] = {CPLO_DEFAULT, CPLO_SRVTYPE, CPLO_SRVLOGAS, CPLO_SRVADDR, CPLO_SRVINSTKEY};
CPListOrder ObjectOrder[] = {CPLO_DEFAULT, CPLO_OBJMODIFDESC};
//
// Listview column header click event handler
//
void ControlPanelList::OnSort(wxListEvent &event)
{
    CPListOrder Order = CPLO_DEFAULT;
    // IF Root tree item is Servers OR ODBC datasources
    if (m_BranchInfo->topcat == TOPCAT_SERVERS || m_BranchInfo->topcat == TOPCAT_ODBCDSN)
    {
        // Server list
        if (!*m_BranchInfo->server_name)
            Order = GetOrder(ServerOrder, sizeof(ServerOrder) / sizeof(CPListOrder), event.GetColumn());
        // Object list
        else if (m_BranchInfo->syscat == SYSCAT_CATEGORY && *m_BranchInfo->schema_name && (CCategList::HasFolders(m_BranchInfo->category) || m_BranchInfo->category == CATEG_XMLFORM || m_BranchInfo->category == CATEG_STSH)) 
        {
            Order = GetOrder(ObjectOrder, sizeof(ObjectOrder) / sizeof(CPListOrder), event.GetColumn());
            if (Order == m_Order)
            {
                if (m_Order == CPLO_OBJMODIFDESC)
                    Order = CPLO_OBJMODIFASC;
                else if (m_Order == CPLO_OBJMODIFASC)
                    Order = CPLO_OBJMODIFDESC;
                else if (m_Order == CPLO_DEFAULT)
                    Order = CPLO_NAMEDESC;
                else if (m_Order == CPLO_NAMEDESC)
                    Order = CPLO_DEFAULT;
            }
        }
    }
    // IF order changed refill list
    if (Order != m_Order)
    {
        m_Order = Order;
        Freeze();
        Fill();
        Thaw();
    }
}
//
// Inserts new item to list
//
long ControlPanelList::Insert(const wxChar *Text, int ImageIndex, long Data, int Index)
{
    // IF insert by order
    if (Index == LII_DOSORT)
    {
        // IF list is empty, Index = 0
        long Max = GetItemCount();
        if (!Max)
            Index = 0;
        // ELSE find Index by dividing interval
        else
        {
            long Min = 0;
            for (;;)
            {
                long Mid = (Max + Min) / 2;
                if (Compare(Data, (long)GetItemData(Mid)) >= 0)
                {
                    Min  = Mid;
                    if (Max - Min <= 1)
                        break;
                }
                else
                {
                    Max  = Mid;
                    if (Max == Min)
                        break;
                }
            }
            Index = Max;
        }
    }
    // IF Append, Index = item count
    else if (Index == LII_APPEND)
        Index = GetItemCount();
    // Insert item, set item data
    Index = InsertItem(Index, Text, ImageIndex);
    SetItemData(Index, Data);
    return(Index);
}
//
// wx callback for sort gets on input ItemData (GetItemData/SetItemData), ControlPanelList implementation uses
// as ItemData object number, problem is that I need sort beside ordinar objects special items as ".." folder,
// control console items, etc, second prolem is that object number may collide with folder index. Used solution
// will be functional until highest object number will be less then 0x80000000. ControlPanelList ItemData 
// indicates:
//
// 80000000 - FFFF1FFF  - Folders = Folder index | IDT_FOLDER
// FFFF2000 - FFFF3FFF  - Special objects = IDT_UPFOLDER, IDT_USER - IDT_DOMAIN, IDT_SYSTEMFOLDER
// FFFF4000 - FFFFFFFF  - ODBC tables
// 00000000 - 7FFFFFFF  - Ordinar objects or tables
//
int ControlPanelList::Compare(long Item1, long Item2)
{
    const char *Name1, *Name2;
    bool        Folder1, Folder2;
    
    // IF tree root item is Servers OR ODBC datasources
    if (m_BranchInfo->topcat == TOPCAT_SERVERS || m_BranchInfo->topcat == TOPCAT_ODBCDSN)
    {
        // IF list of servers OR ODBC datasources is sorted
        if (!*m_BranchInfo->server_name)
        {
            char Path1[MAX_PATH] = "\0xFF", Path2[MAX_PATH] = "\0xFF";
            // IF some of items is special object (..), special object is less
            if (IsSpecObj(Item1) || IsSpecObj(Item2))
                return(Item1 - Item2);
            // Get Item1 name
            const wxChar *wName1 = m_SpecList[Item1];
            tcateg Categ = m_BranchInfo->topcat == TOPCAT_ODBCDSN ? XCATEG_ODBCDS : XCATEG_SERVER;
            // Get server descriptor
            if (!m_ObjectList.Find(ToChar(wName1, wxConvCurrent), Categ))
                return(1);
            t_avail_server *Server1 = (t_avail_server *)m_ObjectList.GetSource();
            // Get Item2 name
            const wxChar *wName2 = m_SpecList[Item2];
            // Get server descriptor
            if (!m_ObjectList.Find(ToChar(wName2, wxConvCurrent), Categ))
                return(-1);
            t_avail_server *Server2 = (t_avail_server *)m_ObjectList.GetSource();
            // IF order by type
            if (m_Order == CPLO_SRVTYPE)
            {
                // IF Server1 is local AND Server2 is remote OR vice-versa, local server is less
                if (Server1->local != Server2->local)
                    return(Server1->local ? -1 : 1);
            }
            // IF order by logged user
            else if (m_Order == CPLO_SRVLOGAS)
            {
                // IF both servers logged
                if (Server1->cdp && Server2->cdp)
                {
                    // Compare user names
                    strcpy(Path1, cd_Who_am_I(Server1->cdp));
                    int Result = _stricoll(Path1, cd_Who_am_I(Server2->cdp));
                    if (Result)
                        return(Result);
                }
                // ELSE logged server is less
                else if (!Server1->cdp &&  Server2->cdp)
                    return(1);
                else if (Server1->cdp  && !Server2->cdp)
                    return(-1);
            }
            // IF order by server address
            else if (m_Order == CPLO_SRVADDR)
            {
                // IF both servers are local, compare fil paths
                if (Server1->local && Server2->local)
                { GetPrimaryPath((const char *)ToChar(wName1, wxConvCurrent), Server1->private_server, Path1);
                  GetPrimaryPath((const char *)ToChar(wName2, wxConvCurrent), Server2->private_server, Path2);
                  int Result = _stricoll(Path1, Path2);
                  if (Result)
                      return(Result);
                }
                // IF both servers are remote, IP addresses
                else if (!Server1->local && !Server2->local)
                {   GetDatabaseString((const char *)ToChar(wName1, wxConvCurrent), IP_str, Path1, sizeof(Path1), Server1->private_server);
                    GetDatabaseString((const char *)ToChar(wName2, wxConvCurrent), IP_str, Path2, sizeof(Path2), Server2->private_server);
                    int Result = _stricoll(Path1, Path2);
                    if (Result)
                        return(Result);
                }
                // ELSE loacal server is less
                else if (Server1->local)
                    return(-1);
                else
                    return(1);
            }
            // IF order by instalation key, compare instalation keys (NOT USED)
            else if (m_Order == CPLO_SRVINSTKEY)
            {
                GetDatabaseString((const char *)ToChar(wName1, wxConvCurrent), database_ik, Path1, sizeof(Path1), Server1->private_server);
                GetDatabaseString((const char *)ToChar(wName2, wxConvCurrent), database_ik, Path2, sizeof(Path2), Server2->private_server);
                int Result = _stricoll(Path1, Path2);
                if (Result)
                    return(Result);
            }
            // Otherwise compare server names
            return(_tcsicmp(wName1, wName2));
        }
        // IF schema list is sorted
        else if (m_BranchInfo->syscat == SYSCAT_APPLS)
        {
            // IF some of items is special object, .. is first, System folder is second, then schemas
            if (IsSpecObj(Item1) || IsSpecObj(Item2))
                return(Item1 - Item2);
    
            // Get schema name
            if (!m_ObjectList.FindByObjnum(Item1, CATEG_APPL))
                return(1);
            Name1 = m_ObjectList.GetName(); 
            // _sysext is less
            if (stricmp(Name1, "_sysext") == 0)
                return(-1);
            if (!m_ObjectList.FindByObjnum(Item2, CATEG_APPL))
                return(-1);
            Name2 = m_ObjectList.GetName();
            if (stricmp(Name2, "_sysext") == 0)
                return(1);
            // Compare schema names
            return(_stricoll(Name1, Name2));
        }
        // IF categories list is sorted
        else if (m_BranchInfo->syscat == SYSCAT_APPLTOP)  
        {
            // .. is less
            if (IsUpFolder(Item1))
                return(-1);
            if (IsUpFolder(Item2))
                return(1);
            Folder1 = IsFolder(Item1);
            Folder2 = IsFolder(Item2);
            // IF Item1 is folder
            if (Folder1)
            {
                // IF Item2 is not folder, Item1 is less
                if (!Folder2)
                    return(-1);                
                if (!m_FolderList.FindByIndex(FolderIndexFromData(Item1)))
                    return(1);
                Name1 = m_FolderList.GetName();
                if (!m_FolderList.FindByIndex(FolderIndexFromData(Item2)))
                    return(-1);
                // Compare folder names
                return(_stricoll(Name1, m_FolderList.GetName()));
            }
            // IF Item2 is folder, Item2 is less
            else if (Folder2)
                return(1);
            // ELSE compare category ID
            return(Item1 - Item2);
        }
        // IF object list is sorted
        else if (m_BranchInfo->syscat == SYSCAT_CATEGORY)  
        {
            uns32 Modif1, Modif2;
            // IF system objects
            if (!*m_BranchInfo->schema_name)
            {
                // Users, groups (, replication servers)
                if (m_BranchInfo->category != CATEG_TABLE)
                {
                    // .. first, then other objects by name
                    return(CompareByName(Item1, Item2, m_BranchInfo->category));
                }
            }
            // Folders below categories
            else if (my_control_panel->ShowFolders && my_control_panel->fld_bel_cat) 
            {
                // IF not schema root
                if (m_BranchInfo->get_folder_index() != FOLDER_ROOT || m_BranchInfo->category != NONECATEG)
                {
                    // .. is less
                    if (IsUpFolder(Item1))
                        return(-1);
                    if (IsUpFolder(Item2))
                        return(1);
                    // IF Item1 is folder, get folder name
                    Folder1 = IsFolder(Item1);
                    if (Folder1)
                    {
                        if (!m_FolderList.FindByIndex(FolderIndexFromData(Item1)))
                            return(1);
                        Name1 = m_FolderList.GetName();
                    }
                    // ELSE get object name and modif timestamp
                    else
                    {
                        if (!m_ObjectList.FindByObjnum(Item1, m_BranchInfo->category))
                            return(1);
                        Name1  = m_ObjectList.GetName();
                        Modif1 = m_ObjectList.GetModif();
                    }
                    // IF Item2 is folder
                    Folder2 = IsFolder(Item2);
                    if (Folder2)
                    {
                        // IF Item1 is not folder, Item2 is less
                        if (!Folder1)
                            return(1);
                        // Get folder name
                        if (!m_FolderList.FindByIndex(FolderIndexFromData(Item2)))
                            return(-1);
                        Name2 = m_FolderList.GetName();
                    }
                    // ELSE Item2 is not folder
                    else
                    {
                        // IF Item1 is folder, Item1 is less
                        if (Folder1 || !m_ObjectList.FindByObjnum(Item2, m_BranchInfo->category))
                            return(-1);
                        // Get object name and modif timestamp
                        Name2  = m_ObjectList.GetName();
                        Modif2 = m_ObjectList.GetModif();
                    }
                    // IF order by modif descending AND both item are not foders AND modif timestamps are not equal, compare timestamps
                    if (m_Order == CPLO_OBJMODIFDESC && !Folder1 && Modif1 != Modif2)
                        return(Modif2 - Modif1);
                    // IF order by modif ascending AND both item are not foders AND modif timestamps are not equal, compare timestamps
                    else if (m_Order == CPLO_OBJMODIFASC && !Folder1 && Modif1 != Modif2)
                        return(Modif1 - Modif2);
                    // Otherwise compare object names
                    else
                    {
                        int res = _stricoll(Name1, Name2);
                        return(m_Order == CPLO_DEFAULT ? res : -res); 
                    }
                }
            }
            // ELSE categories below folders
            else
            {
                // IF folder root
                if (m_BranchInfo->category == NONECATEG)
                {
                    // IF some of Items is not folder, folders first, then categories by ID
                    if (!IsFolder(Item1) || !IsFolder(Item2))
                        return(Item1 - Item2);
                    // ELSE compare folder names
                    if (!m_FolderList.FindByIndex(FolderIndexFromData(Item1)))
                        return(1);
                    Name1 = m_FolderList.GetName();
                    if (!m_FolderList.FindByIndex(FolderIndexFromData(Item2)))
                        return(-1);
                    return(_stricoll(Name1, m_FolderList.GetName()));
                }
                // ELSE object list
                else
                {
                    // IF some of items is .., .. is less
                    if (IsSpecObj(Item1) || IsSpecObj(Item2))
                        return(Item1 - Item2);
                    // Get Item1 name and modif timestamp
                    if (!m_ObjectList.FindByObjnum(Item1, m_BranchInfo->category))
                        return(1);
                    Name1  = m_ObjectList.GetName();
                    Modif1 = m_ObjectList.GetModif();
                    // Get Item2 name and modif timestamp
                    if (!m_ObjectList.FindByObjnum(Item2, m_BranchInfo->category))
                        return(-1);
                    Name2  = m_ObjectList.GetName();
                    Modif2 = m_ObjectList.GetModif();
                    // IF order by modif descending AND modif timestamps are not equal, compare modif timestamps
                    if (m_Order == CPLO_OBJMODIFDESC && Modif1 != Modif2)
                        return(Modif2 - Modif1);
                    // IF order by modif ascending AND modif timestamps are not equal, compare modif timestamps
                    else if (m_Order == CPLO_OBJMODIFASC && Modif1 != Modif2)
                        return(Modif1 - Modif2);
                    // Otherwise compare object names
                    else
                    {
                        int res = _stricoll(Name1, Name2);
                        return(m_Order == CPLO_DEFAULT ? res : -res); 
                    }
                }
            }
        }
    }
    // IF Mail profiles list is sorted, compare profile names
    else if (m_BranchInfo->topcat == TOPCAT_CLIENT && m_BranchInfo->cti == IND_MAIL) // topcat -> client
    {
        return(_tcsicmp(m_SpecList[Item1], m_SpecList[Item2]));
    }
    // Otherwise compare items by ID
    return(Item1 - Item2);
}
//
// Compares two objects
//
int ControlPanelList::CompareByName(long Item1, long Item2, tcateg Categ)
{
    tobjname Name;
    // IF some of objects is .., .. is less
    if (IsSpecObj(Item1) || IsSpecObj(Item2))
        return(Item1 - Item2);
    // Get object names
    if (!m_ObjectList.FindByObjnum(Item1, Categ))
        return(1);
    strcpy(Name, m_ObjectList.GetName());
    if (!m_ObjectList.FindByObjnum(Item2, Categ))
        return(-1);
    // Compare object names
    return(_stricoll(Name, m_ObjectList.GetName()));
}

static tcateg CategFromDataTB[] =    // indexed by IDT_...
{
    CATEG_TABLE,        // IDT_TABLE       
    CATEG_VIEW,         // IDT_VIEW       
    CATEG_MENU,         // IDT_MENU        
    CATEG_CURSOR,       // IDT_CURSOR     
    CATEG_PGMSRC,       // IDT_PROG       
    CATEG_PICT,         // IDT_PICT       
    CATEG_DRAWING,      // IDT_DRAWING    
    CATEG_RELATION,     // IDT_RELATION   
    CATEG_ROLE,         // IDT_ROLE       
    CATEG_CONNECTION,   // IDT_CONNECTION 
    CATEG_REPLREL,      // IDT_REPLREL    
    CATEG_PROC,         // IDT_PROC       
    CATEG_TRIGGER,      // IDT_TRIGGER    
    CATEG_XMLFORM,      // IDT_XMLFORM        
    CATEG_SEQ,          // IDT_SEQUENCE   
    CATEG_DOMAIN,       // IDT_DOMAIN     
    CATEG_INFO,         // IDT_FULLTEXT
    CATEG_STSH,         // IDT_STSH

    CATEG_USER,         // IDT_USER       
    CATEG_GROUP,        // IDT_GROUP      
    CATEG_TABLE,        // IDT_SYSTABLE   
    CATEG_SERVER        // IDT_REPLSERVER 
};
//
// Converts category ID from ItemData to tcateg
//
tcateg ControlPanelList::CategFromData(long Data)
{
    return(CategFromDataTB[Data - IDT_TABLE]);
}
//
// Cut action
//
void ControlPanelList::OnCut(wxCommandEvent &event)
{
    // Create selection iterator
    list_selection_iterator si(this);
    // Fill item info
    add_list_item_info(GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED), si.info);
    // IF objects can be copied, provide copy
    if (si.info.CanCopy())
        CPCopy(si, true);
}
//
// Copy action
//
void ControlPanelList::OnCopy(wxCommandEvent &event)
{
    // Create selection iterator
    list_selection_iterator si(this);
    // Fill item info
    add_list_item_info(GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED), si.info);
    // IF objects can be copied, provide copy
    if (si.info.CanCopy())
        CPCopy(si, false);
}
//
// Paste action
//
void ControlPanelList::OnPaste(wxCommandEvent &event)
{
    C602SQLDataObject Data;
    // IF clipboard content can be pasted, provide paste
    if (wxIsDragResultOk(m_BranchInfo->CanPaste(Data.GetDataFromClipboard(), false)))
        PasteObjects(m_BranchInfo->cdp, m_BranchInfo->folder_name);
}
//
// Refresh action
//
void ControlPanelList::OnRefresh(wxCommandEvent &event)
{ 
    my_control_panel->OnRefresh(event); 
}
//
// Begin drag event handler
//
void ControlPanelList::OnBeginDrag(wxListEvent &event)
{
    SetFocus();
#ifdef LINUX
    // On linux is event invoked even if click is outside any item
	if (event.GetIndex() >= GetItemCount())
		return;
#endif	
    // Select current item
    if (!GetItemState(event.GetIndex(), wxLIST_STATE_SELECTED))
        SelectItem(event.GetIndex());
    // Create selection iterator
    list_selection_iterator si(this);
    // Fill item info
    add_list_item_info(GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED), si.info);
    // EXIT IF objects can't be copied
    if (!si.info.CanCopy())
        return;
    // Create custom copy descriptor
    t_copyobjctx ctx;
    memset(&ctx, 0, sizeof(t_copyobjctx));
    ctx.strsize  = sizeof(t_copyobjctx);
    ctx.count    = GetSelectedItemCount();
    ctx.param    = &si;
    ctx.nextfunc = get_next_sel_obj;
    strcpy(ctx.folder, m_BranchInfo->folder_name);
    // Create copy context
    CCopyObjCtx Ctx(m_BranchInfo->cdp, m_BranchInfo->schema_name);
    // Get DataObject with copied objects
    C602SQLDataObject *dobj = Ctx.Copy(&ctx);
    if (!dobj)
        return;

#ifdef WINS    
    // Image dragging on linux conflicts with Drag&Drop implementation so it is not used
    CWBDragImage *di;

#ifdef WINS    
    // Get mouse position
    wxPoint Pos = event.GetPoint();
#else
    wxPoint Pos = wxGetMousePosition();
    Pos         = ScreenToClient(Pos);
#endif
    wxPoint diPos = Pos;
    
    // IF single object is selected
    if (GetSelectedItemCount() == 1)
    {
        wxRect Rect;
        // Calculate drag image position
        GetItemRect(event.GetIndex(), Rect, wxLIST_RECT_LABEL);
#ifdef WINS    
        Rect.x -= PatchedGetMetric(wxSYS_SMALLICON_X) + 3;
#else        
        Rect.x += PatchedGetMetric(wxSYS_SMALLICON_X) - 10;
#endif        
        Pos    -= Rect.GetPosition();
        // Create drag image for single object
        di      = new CWBDragImage(*this, event.GetIndex());
    }
    // ELSE multiple objects
    else
    {
        // Calculate drag image position
        Pos.x = PatchedGetMetric(wxSYS_SMALLICON_X) / 2;
        Pos.y = Pos.x;
        // Create drag image for multiple object
        di    = new CWBDragImage(wxBitmap(CCategList::MultiSelImage(m_BranchInfo->category)));
    }
    // Start image dragging
    if (di->BeginDrag(Pos, this, true))
    {
        di->Show();
        di->Move(diPos);
    }
#endif        

    // Create DropSource
#ifdef WINS    
    CCPDragSource ds(this, dobj);
#else
    si.reset();
    si.next();
    CCPDragSource ds(this, dobj, get_image_index(si.info.category, si.info.flags));
#endif    
    // Start Drag&Drop operation
    wxDragResult Res = ds.DoDragDrop(wxDrag_AllowMove);
#ifdef WINS    
    // Stop image dragging
    di->EndDrag();
    delete di;
#endif
#ifdef LINUX
    Refresh();
#endif
}
//
// Adds object to list, returns item index
//
long ControlPanelList::AddObject(const char *ServerName, const char *SchemaName, const char *FolderName, const char *ObjName, tcateg Categ, uns16 Flags, uns32 Modif)
{
    long Item;
    if (!ServerName)
        return(-1);
    if (!SchemaName)
        SchemaName = "";
    if (!FolderName)
        FolderName = "";
    // Server
    if (m_BranchInfo->syscat == SYSCAT_NONE && !*SchemaName)
    {
        // I can't ensure uniqueness server of object ID in ItemData, so I read whole list again
        Refresh(); 
        return(FindItem(-1, wxString(ServerName, *wxConvCurrent)));
    }
    // Reset ObjectList
    m_ObjectList.Release();
    // IF not user, group nor repliction server
    if (Categ != CATEG_USER && Categ != CATEG_GROUP && Categ != CATEG_SERVER)
    {
        // Check server name
        if (!m_BranchInfo->cdp || stricmp(ServerName, m_BranchInfo->server_name) != 0)
            return(-1);
        // Check empty schema name
        if (!*SchemaName && m_BranchInfo->cdp)
            SchemaName = m_BranchInfo->cdp->sel_appl_name;
        // IF schema name add
        if (m_BranchInfo->syscat == SYSCAT_APPLS && (!*ObjName))
        {
            // Get schema properties
            if (!m_ObjectList.Find(SchemaName, CATEG_APPL))
                return(-1);
            // IF schema  doesn't exist in listview, insert it
            Item = FindItem(1, GetSmallStringName(m_ObjectList));
            if (Item == -1)
                Item = Insert(GetSmallStringName(m_ObjectList), IND_APPL, m_ObjectList.GetObjnum());
            return(Item);
        }
        // Check current schema name
        if (wb_stricmp(m_BranchInfo->cdp, SchemaName, m_BranchInfo->schema_name) != 0)
            return(-1);
    }
    // IF Schema root AND show folders AND categories below folders
    if (m_BranchInfo->syscat == SYSCAT_APPLTOP && my_control_panel->ShowFolders && !my_control_panel->fld_bel_cat)
    {
        const char *Folder;
        // Get new folder name
        if (*FolderName)
            Folder = FolderName;
        else if (Categ == CATEG_FOLDER)
            Folder = ObjName;
        else
            return(-1);
        // Get folder properties
        if (!m_FolderList.Find(Folder, CATEG_FOLDER, m_BranchInfo->folder_name))
            return(-1);
        // IF folder doesn't exist in listview, insert it
        Item = FindItem(-1, m_FolderList.GetIndex() | IDT_FOLDER);
        if (Item == -1)
            Item = Insert(GetSmallStringName(m_FolderList), IND_FOLDER, m_FolderList.GetIndex() | IDT_FOLDER);
        return(Item);
    }
    // Object
    if (m_BranchInfo->syscat == SYSCAT_CATEGORY)  
    {
        const char *Folder = NULL;
        // IF not user, group nor repliction server
        if (Categ != CATEG_USER && Categ != CATEG_GROUP && Categ != CATEG_SERVER)
        {
            // Folder
            if (my_control_panel->ShowFolders)
            {
                const char *Folder;
                // Get folder name
                if (*FolderName)
                    Folder = FolderName;
                else if (Categ == CATEG_FOLDER)
                    Folder = ObjName;
                else
                    Folder = NULL;
                // IF folder doesn't exist in listview, insert it
                if (Folder && m_FolderList.Find(Folder, CATEG_FOLDER, m_BranchInfo->folder_name))
                {
                    Item = FindItem(-1, m_FolderList.GetIndex() | IDT_FOLDER);
                    if (Item == -1)
                    {
                        Item = Insert(GetSmallStringName(m_FolderList), IND_FOLDER, m_FolderList.GetIndex() | IDT_FOLDER);
                        return(Item);
                    }
                }
                // Check current folder name
                if (wb_stricmp(m_BranchInfo->cdp, FolderName, m_BranchInfo->folder_name) != 0)
                    return(-1);
            }
            if (Categ != m_BranchInfo->category)
                return(-1);
            Folder = m_BranchInfo->folder_name;
        }
        // Get object properties
        if (!m_ObjectList.Find(ObjName, Categ, Folder))
            return(-1);
        // IF object doesn't exist in listview, insert it
        Item = FindItem(-1, m_ObjectList.GetObjnum());
        if (Item == -1)
           Item = InsertObject(GetSmallStringName(m_ObjectList), m_BranchInfo->category, m_ObjectList.GetFlags(), m_ObjectList.GetObjnum(), m_ObjectList.GetModif());
        return(Item);
    }
    return(-1);
}
//
// Deletes object
// 
void ControlPanelList::DeleteObject(const char *ServerName, const char *SchemaName, const char *ObjName, tcateg Categ)
{
    long Item;
    // EXIT IF not in current schema
    if (!ServerName)
        return;
    if (!SchemaName)
        SchemaName = "";
    if (stricmp(ServerName, m_BranchInfo->server_name) != 0 || stricmp(SchemaName, m_BranchInfo->schema_name) != 0)
        return;
    // Deleting server, I can't ensure uniqueness server of object ID in ItemData, so I read whole list again
    if (m_BranchInfo->syscat == SYSCAT_NONE && !*SchemaName)
        Refresh();
    // Check schema name
    if (!*SchemaName)
        SchemaName = m_BranchInfo->cdp->sel_appl_name;
    else if (wb_stricmp(m_BranchInfo->cdp, SchemaName, m_BranchInfo->schema_name) != 0)
        return;
    // Deleting schema, find schema item, delete item
    if (m_BranchInfo->syscat == SYSCAT_APPLS && (!*ObjName))
    {
        Item = FindItem(1, wxString(SchemaName, *m_BranchInfo->cdp->sys_conv));
        if (Item != -1)
            DeleteItem(Item);
        return;
    }
    // Deleting folder in schema root, find folder item, delete item
    if (m_BranchInfo->syscat == SYSCAT_APPLTOP && my_control_panel->ShowFolders && !my_control_panel->fld_bel_cat && Categ == CATEG_FOLDER)
    {
        for (Item = -1; (Item = FindItem(Item, wxString(ObjName, *m_BranchInfo->cdp->sys_conv))) != -1; )
        {
            wxListItem iinfo;  
            iinfo.SetId(Item);  
            iinfo.m_mask  = wxLIST_MASK_IMAGE;
            GetItem(iinfo);
            if (iinfo.m_image == IND_FOLDER)
            {
                DeleteItem(Item);
                return;
            }
        }
        return;
    }
    // Objects
    if (m_BranchInfo->syscat == SYSCAT_CATEGORY)  
    {
        // Deleting folder, find folder item, delete item
        if (my_control_panel->ShowFolders)
        {
            if (Categ == CATEG_FOLDER)
            {
                for (Item = -1; (Item = FindItem(Item, wxString(ObjName, *m_BranchInfo->cdp->sys_conv))) != -1; )
                {
                    wxListItem iinfo;  
                    iinfo.SetId(Item);  
                    iinfo.m_mask  = wxLIST_MASK_IMAGE;
                    GetItem(iinfo);
                    if (iinfo.m_image == IND_FOLDER)
                    {
                        DeleteItem(Item);
                        return;
                    }
                }
            }
        }
        if (Categ != m_BranchInfo->category)
            return;
        
        // Deleting object, find object item, delete item
        for (Item = -1; (Item = FindItem(Item, wxString(ObjName, *m_BranchInfo->cdp->sys_conv))) != -1; )
        {
            wxListItem iinfo;  
            iinfo.SetId(Item);  
            iinfo.m_mask  = wxLIST_MASK_IMAGE;
            GetItem(iinfo);
            if (iinfo.m_image != IND_FOLDER)
            {
                DeleteItem(Item);
                return;
            }
        }
    }
}
//
// Refreshes schema root folder content
//
void ControlPanelList::RefreshSchema(const char *ServerName, const char *SchemaName)
{
    // Ignore for other then schema selected in tree
    if (!ServerName)
        return;
    if (stricmp(ServerName, m_BranchInfo->server_name) != 0)
        return;
    if (!SchemaName || !*SchemaName)
        SchemaName = m_BranchInfo->cdp->sel_appl_name;
    else if (wb_stricmp(m_BranchInfo->cdp, SchemaName, m_BranchInfo->schema_name) != 0)
        return;
    // Refresh list
    Refresh();
}
//
// Refreshes shema list
//
void ControlPanelList::RefreshServer(const char *ServerName)
{
    // Ignore for other then server selected in tree
    if (!ServerName)
        return;
    if (stricmp(ServerName, m_BranchInfo->server_name) != 0)
        return;
    // Refresh list
    Refresh();
}
//
// Highlights item under mouse cursor during Drag&Drop
//
void ControlPanelList::HighlightItem(long Item, bool Highlight)
{
    if (Item != -1)
    {
#ifdef WINS        
        SetItemState(Item, Highlight ? wxLIST_STATE_DROPHILITED : 0, wxLIST_STATE_DROPHILITED);
#else        
        SetItemTextColour(Item,       Highlight ? HighlightTextColour : GetForegroundColour());
        SetItemBackgroundColour(Item, Highlight ? HighlightColour     : GetBackgroundColour());
#endif        
    }
}
//
// Deselects all items
//
void ControlPanelList::DeselectAll()
{
#ifdef WINS

    SetItemState(-1,  0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);

#else

    for (long Item = -1; (Item = GetNextItem(Item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1;)
        SetItemState(Item, 0, wxLIST_STATE_SELECTED);

#endif
}
//
// Drop target window enter event handler
//
wxDragResult CCPListDropTarget::OnEnter(wxCoord x, wxCoord y, wxDragResult def)
{
    // Reset last item, last highlighted item, last DragResult
    m_Item    = -1;
    m_SelItem = -1;
    m_Result  = wxDragNone;
    return(OnDragOver(x, y, def));
}
//
// Drop target window drag over event handler
//
wxDragResult CCPListDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    long SelItem = -1;
    long Item;
    // Check mouse position
    int  Flags   = Owner()->DragTest(x, y, &Item);
    // IF over item
    if (Flags == CPHR_ONITEM)
    {
        // IF not last item
        if (Item != m_Item)
        {
            item_info  info;
            // pinfo = tree selected item info
            item_info *pinfo =  Owner()->m_BranchInfo;
            // IF over no item 
            if (Item == -1)
                m_Result = wxDragNone;
            // ELSE over some item
            else
            {
                info = *pinfo;
                // Get list item info
                Owner()->add_list_item_info(Item, info);
                // IF over schema, folder or category, pinfo = list item info
                if (info.IsSchema() || info.IsCategory() || info.IsFolder())
                {
                    pinfo   = &info;
                    SelItem = Item;
                }
                // Check if item defined by pinfo can accept dragged objects
                m_Result = pinfo->CanPaste(CCPDragSource::DraggedObjects(), true);
                if (wxIsDragResultOk(m_Result))
                {
                    m_cdp = pinfo->cdp;
                    strcpy(m_Schema, pinfo->schema_name);
                    strcpy(m_Folder, pinfo->folder_name);            
                }
            }
            // IF new highlighted item other then old highlighted item
            if (SelItem != m_SelItem)
            {
#ifdef WINS    
                // Hide drag image
                CWBDragImage::Image->Hide();
#endif    
                // Unhighlight old item, highlight new item
                Owner()->HighlightItem(m_SelItem, false);
                Owner()->HighlightItem(SelItem,   true);
                Owner()->wxWindow::Update();
#ifdef WINS    
                // Show drag image
                CWBDragImage::Image->Show();
#endif    
                m_SelItem = SelItem;
            }
        }
    }
    else if (Flags != CPHR_NOWHERE)
    {
        // IF over top of list client area, get item preceeding top item
        if (Flags == CPHR_ABOVE)
        {
            Item = m_Item - 1;
            if (Item < -1)
                Item = -1;
        }
        // IF over top of list client area, get item folowing last visible item
        else if (Flags == CPHR_BELOW)
        {
            Item = m_Item + 1;
            if (Item >= Owner()->GetItemCount())
                Item = Owner()->GetItemCount() - 1;
        }
        // IF item exists, scroll list
        if (Item != -1)
        {
#ifdef WINS
            CWBDragImage::Image->Hide();
#endif    
            Owner()->EnsureVisible(Item);
            Owner()->HighlightItem(m_SelItem, false);
            Owner()->wxWindow::Update();
#ifdef WINS
            CWBDragImage::Image->Show();
#endif    
        }
    }
    m_Item = Item;  
    return(GetResult());
}
//
// Drop target window leave event handler
//
void CCPListDropTarget::OnLeave()
{
    // Unhighlight last item, redraw tree
    Owner()->HighlightItem(m_SelItem, false);
    Owner()->wxWindow::Update();
}
//
// Draged data drop event handler
//
bool CCPListDropTarget::OnDrop(wxCoord x, wxCoord y)
{
    // Unhighlight last item, redraw tree
    Owner()->HighlightItem(m_SelItem, false);
    Owner()->wxWindow::Update();
    return(wxIsDragResultOk(m_Result));
}

