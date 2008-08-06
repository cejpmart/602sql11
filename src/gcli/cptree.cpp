//
// cptree.cpp
//
// Control panel treeview
//
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
#ifndef WINS
#define _stricoll(a,b) strcasecmp(a,b)
#endif
#include "objlist9.h"
#include "controlpanel.h"
#include "cptree.h"
#include "cplist.h"
#include "impexpui.h"

#include "xrc/LoginDlg.h"
#include "xrc/SQLConsoleDlg.h"
#include "xrc/ColumnsInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#define ROOTITM_SERVERS    _("602SQL Servers")
#define ROOTITM_ODBC       _("ODBC Data Sources")
#define ROOTITM_LOCAL_SETT _("Local Settings")

DEFINE_LOCAL_EVENT_TYPE(myEVT_INITEDIT)
DEFINE_LOCAL_EVENT_TYPE(myEVT_AFTEREDIT)

BEGIN_EVENT_TABLE( ControlPanelTree, wbTreeCtrl )
    EVT_TREE_ITEM_EXPANDING(TREE_ID,       ControlPanelTree::OnItemExpanding)
    EVT_TREE_ITEM_ACTIVATED(TREE_ID,       ControlPanelTree::OnItemActivated)
#ifdef CONTROL_PANEL_INPLECE_EDIT
    EVT_TREE_BEGIN_LABEL_EDIT(TREE_ID,     ControlPanelTree::OnBeginLabelEdit)
    EVT_TREE_END_LABEL_EDIT(TREE_ID,       ControlPanelTree::OnEndLabelEdit)
    EVT_COMMAND(TREE_ID, myEVT_INITEDIT,   ControlPanelTree::OnInitEdit)
    EVT_COMMAND(TREE_ID, myEVT_AFTEREDIT,  ControlPanelTree::OnAfterEdit)
    EVT_TREE_SEL_CHANGING(TREE_ID,         ControlPanelTree::OnItemSelecting)
#else
    EVT_MENU(CPM_RENAME,                   ControlPanelTree::OnRename)
#endif
    EVT_TREE_SEL_CHANGED(TREE_ID,          ControlPanelTree::OnItemSelected)
    EVT_KEY_DOWN(                          ControlPanelTree::OnKeyDown)
    EVT_MENU(CPM_CUT,                      ControlPanelTree::OnCut)
    EVT_MENU(CPM_COPY,                     ControlPanelTree::OnCopy)
    EVT_MENU(CPM_PASTE,                    ControlPanelTree::OnPaste)
    EVT_TREE_BEGIN_DRAG(TREE_ID,           ControlPanelTree::OnBeginDrag)
    EVT_RIGHT_DOWN(                        ControlPanelTree::OnRightMouseDown)
    EVT_SET_FOCUS(                         ControlPanelTree::OnSetFocus)
    EVT_TIMER(-1,                          ControlPanelTree::OnTimer)
#ifdef LINUX
    EVT_RIGHT_UP(                          ControlPanelTree::OnRightClick)  // enters selection mode on MSW
    EVT_LEFT_DOWN(                         ControlPanelTree::OnLeftMouseDown)
#else
    EVT_TREE_ITEM_RIGHT_CLICK(TREE_ID,     ControlPanelTree::OnRightClick)  // on LINUX emitted on ButtonDown
#endif
END_EVENT_TABLE()
//
// Inserts tree root items - Servers, ODBC Data Sources, Local Settings
//                                                    
void ControlPanelTree::insert_top_items(void)
{ 
  rootId = AddRoot(wxString(wxT("602SQL")));  // invisible
  wxTreeItemId item;
  item = AppendItem(rootId, ROOTITM_SERVERS,    IND_SERVERS,  IND_SERVERS);
  AppendFictiveChild(item);
#ifdef CLIENT_ODBC_9
  item = AppendItem(rootId, ROOTITM_ODBC,       IND_ODBCDSNS, IND_ODBCDSNS);
  AppendFictiveChild(item);
#endif
  item = AppendItem(rootId, ROOTITM_LOCAL_SETT, IND_CLIENT,   IND_CLIENT);
  AppendFictiveChild(item);
}
//
// Item activated event handler - invokes actions for control console and local settings
//
void ControlPanelTree::OnItemActivated(wxTreeEvent & event)
{ 
    item_info info;
    get_item_info(event.GetItem(), info);
    if (info.topcat == TOPCAT_SERVERS || info.topcat == TOPCAT_ODBCDSN)
    {
        if (info.syscat == SYSCAT_CONSOLE)
            console_action(info.cti, info.cdp);
    }
    else if (info.topcat == TOPCAT_CLIENT)
        client_action(my_control_panel, info);
    event.Skip();
}

// If the item expanded has a fictive child, replace it by the real children.
void ControlPanelTree::OnItemExpanding(wxTreeEvent & event)
{ 
#ifdef CONTROL_PANEL_INPLECE_EDIT
    // Disable during inplace edit
    if (m_EditItem.IsOk())
    {
        event.Veto();
        return;
    }
#endif
    // IF not from Drag&Drop, select current item
    if (!m_FromDrag)
        SelectItem(event.GetItem());
    if (HasFictiveChild(event.GetItem()))   // check if the 1st child is fictive:
    { 
        item_info info;
        get_item_info(event.GetItem(), info);
        if (info.topcat == TOPCAT_SERVERS || info.topcat == TOPCAT_ODBCDSN)
        { 
            if (!*info.server_name)  // list of servers
            {
                DeleteChildren(event.GetItem());    // delete the fictive child
                InsertAvailableServers(event.GetItem(), info.topcat == TOPCAT_ODBCDSN);
            }
            else // server specified
            {   // check if the server is connected, connect and login or Veto if failed:
                if (!info.xcdp())  // not connected
                {   // find the server
                    t_avail_server * as = ::Connect(info.server_name);
                    if (!as)
                    {   event.Veto();
                        //event.Skip();  -- with this the Veto() does not work - item is expanded with the fictive child
                        return;
                    }
                    info.cdp = as->cdp;   info.conn = as->conn;
                    // list objects on the server:
                    if (event.GetItem() == m_SelItem)
                    {
                        strcpy(m_SelInfo.server_name, info.server_name);
                        m_SelInfo.cdp = info.cdp;
                        m_SelInfo.conn = info.conn;
                        objlist->Fill();
                    }
                    my_control_panel->frame->update_action_menu_based_on_selection();  // update the Connect/Disconnect items
                }
                DeleteChildren(event.GetItem());    // delete the fictive child
                if (info.syscat == SYSCAT_APPLS)  // list of applications
                {   InsertAppls(&info, event.GetItem());
                    int ch_cnt = (int)GetChildrenCount(event.GetItem(), false);
                    if (info.topcat == TOPCAT_SERVERS && ch_cnt<=2)
                      info_box(_("You have connected to a server that does not contain an application.\nYou may create a new application or import an application from backup files.\n\nClick the right mouse button to display the pop-up menu on the control panel."));
                }
                else if (info.syscat == SYSCAT_CONSOLE)
                { if (info.cdp)
                    InsertServerConsoleItems(info.cdp, event.GetItem());
                }
                else if (info.syscat == SYSCAT_APPLTOP)
                { if (info.cdp)
                    if (my_control_panel->ShowFolders && !my_control_panel->fld_bel_cat)
                        InsertFolders(info, event.GetItem());
                  InsertCategories(info, event.GetItem());
                }
                else if (info.syscat == SYSCAT_CATEGORY) // show folders (used only when fld_bel_cat is true)
                { if (info.cdp)
                    InsertFolders(info, event.GetItem());
                }
            }
        }
        else if (info.topcat == TOPCAT_CLIENT)
        { 
            DeleteChildren(event.GetItem());    // delete the fictive child
            AppendItem(event.GetItem(), _("Fonts"),               IND_FONTS);
            AppendItem(event.GetItem(), _("Grid Data Format"),    IND_FORMATS);
            AppendItem(event.GetItem(), _("Locale and Language"), IND_LOCALE);
            AppendItem(event.GetItem(), _("Fulltext Plug-ins"),    IND_FTXHELPERS);
            if (ODBCConfig_available)
              AppendItem(event.GetItem(), _("ODBC Manager"),      IND_ODBCTAB);
            AppendItem(event.GetItem(), _("Mail Profiles"),       IND_MAIL);
            AppendItem(event.GetItem(), _("Communication to server"),IND_COMM);
            AppendItem(event.GetItem(), _("External Editors"),    IND_EDITORS);
        }
    }
    event.Skip();                                               
}
//
// Inserts set of servers or ODB datasources to tree
//
void ControlPanelTree::InsertAvailableServers(const wxTreeItemId &Parent, bool odbc)
{
    CObjectList ol((cdp_t)NULL);
    tcateg Categ = odbc ? XCATEG_ODBCDS : XCATEG_SERVER;
    for (bool ItemFound = ol.FindFirst(Categ); ItemFound; ItemFound = ol.FindNext())
    { 
        t_avail_server *Server = (t_avail_server *)ol.GetSource();
        wxString nm (Server->locid_server_name, *wxConvCurrent);
        wxTreeItemId new_item  = InsertItem(Parent, GetNameIndex(Parent, nm), wxString(Server->locid_server_name, *wxConvCurrent), Server->server_image, Server->server_image);
        SetItemBold(new_item, !Server->notreg);
        AppendFictiveChild(new_item);  // fictive child
    }
}
//
// Inserts set of server schemas to tree
//
void ControlPanelTree::InsertAppls(item_info * info, const wxTreeItemId &Parent)
{   wxTreeItemId new_item;
    CObjectList ol(info->cdp, NULL, info->conn);
    // System folder
    if (info->cdp)
    { new_item = AppendItem(Parent, CATNAME_SYSTEM, IND_FOLDER_SYSTEM);
      AppendFictiveChild(new_item);  // fictive child
    }
    // Schemas
    for (bool ItemFound = ol.FindFirst(CATEG_APPL); ItemFound; ItemFound = ol.FindNext())
    { 
        wxString nm = GetSmallStringName(ol);
        new_item = InsertItem(Parent, GetNameIndex(Parent, nm), nm, IND_APPL);
        AppendFictiveChild(new_item);  // fictive child
    }
}
//
// Right mouse down event handler - sets focus to tree, selects current item
//
void ControlPanelTree::OnRightMouseDown(wxMouseEvent & event)
{ 
    int flags;
#ifdef WINS
    SetFocus();
#else
    Activate();
#endif
    wxTreeItemId item = HitTest(ScreenToClient(wxGetMousePosition()), flags);
    // VK_APPS is translated to right mouse click: minimising the damage by limiting the positions of the mouse
    if (item.IsOk() && (flags & (wxTREE_HITTEST_ONITEMBUTTON | wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL/*wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMRIGHT*/)))
    {
        SelectItem(item);
    }
    event.Skip();
}

//
// Right mouse click event handler, shows popup menu
//
#ifdef LINUX

void ControlPanelTree::OnRightClick(wxMouseEvent & event)
{ 
	wxPoint Position = wxPoint(event.m_x, event.m_y);
  	int Flags;
  	wxTreeItemId Item = HitTest(Position, Flags);
  	// VK_APPS is translated to right mouse click: minimising the damage by limiting the positions of the mouse
  	if (Item.IsOk() && (Flags & (wxTREE_HITTEST_ONITEMBUTTON | wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL/*wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMRIGHT*/)))
		ShowPopupMenu(Item, Position);
}

#else

void ControlPanelTree::OnRightClick(wxTreeEvent & event)
{
 	wxPoint Position = ScreenToClient(wxGetMousePosition());
  	wxTreeItemId Item = event.GetItem();
  	// VK_APPS is translated to right mouse click: minimising the damage by limiting the positions of the mouse
  	if (Item.IsOk())
		ShowPopupMenu(Item, Position);
}

#endif
//
// Shows popup menu for Item
//
void ControlPanelTree::ShowPopupMenu(const wxTreeItemId &Item, const wxPoint &Position)
{
    item_info info;
    get_item_info(Item, info);
    C602SQLDataObject Data;
    wxMenu * Popup = new wxMenu;
#ifdef WINS
    info.make_control_panel_popup_menu(Popup, false, wxIsDragResultOk(info.CanPaste(Data.GetDataFromClipboard(), false)));
#else
    info.make_control_panel_popup_menu(Popup, false, true);
#endif
    PopupMenu(Popup, Position);
    delete Popup;
}
//
// Set focus event handler
//
void ControlPanelTree::OnSetFocus(wxFocusEvent &event)
{
    if (my_control_panel->m_InSetFocus)
        return;
    my_control_panel->m_InSetFocus = true;
    Activate();
    my_control_panel->m_InSetFocus = false;
    wxMenuBar *Menu = my_control_panel->frame->GetMenuBar();
    if (Menu)
        Menu->EnableTop(1, true);
#ifdef LINUX
    // This trick prevents inplace edit activating when tree becomes input focus
    m_FromFocus = true;
#endif    
    event.Skip();
}

#ifdef LINUX

void ControlPanelTree::OnLeftMouseDown(wxMouseEvent & event)
{
    if (m_FromFocus)
    {
        m_FromFocus = false;
        wxPoint Pos = wxGetMousePosition();
        Pos         = ScreenToClient(Pos);
        int Flags;
        wxTreeItemId Item = HitTest(Pos, Flags);
        if (Item == m_SelItem)
            return;
    }    
    event.Skip();
}

#endif    // LINUX
//
// Activates tree
//
void ControlPanelTree::Activate()
{
    // Tree is acceptor for menu/toolbar commands 
    my_control_panel->m_ActiveChild = (wxControl *)this;
    // Deselect all items in control panel list to ensure what is subject for menu/toolbar commands if item in tree or selected items in list
    objlist->DeselectAll();
    C602SQLDataObject Data;
    // Update action menu
#ifdef LINUX
    my_control_panel->frame->update_action_menu_based_on_selection(true);
#else
    my_control_panel->frame->update_action_menu_based_on_selection(wxIsDragResultOk(m_SelInfo.CanPaste(Data.GetDataFromClipboard(), false)));
#endif
}
//
// Fills info with item properties
//
void ControlPanelTree::get_item_info(wxTreeItemId item, item_info &info)
{ 
    item_info image_info;  // auxiliary
    info                  = root_info;
    bool just_starting    = true;  // persists when passing folders
    bool folder_specified = false;
    info.object_count     = 0;  // always in the tree, minor exception below
    info.flags            = 0;
    *info.server_name     = 0;
    *info.schema_name     = 0;
    *info.folder_name     = 0;
    *info.object_name     = 0;
    info.syscat           = SYSCAT_NONE;
    info.category         = NONECATEG;
    info.tree_info        = true;
  // find cdp for conversions:
    cdp_t cdp = NULL;
    wxTreeItemId item2 = item;
    while (item2.IsOk())
    { 
        if (item2 == rootId)
            break;  // prevents reading from the virtual root
        int image = GetItemImage(item2);
        image_info.cti = (tcpitemid)image;
        if (image_info.IsServer())
        {
            t_avail_server * as = find_server_connection_info(GetItemText(item2).mb_str(*wxConvCurrent));
            if (as) cdp = as->cdp;
        }
        item2 = GetItemParent(item2);
    }
    
    while (item.IsOk())
    { 
        if (item == rootId)
            break;  // prevents reading from the virtual root
        int image = GetItemImage(item);
        image_info.cti = (tcpitemid)image;
        if (just_starting)
            info.cti = image_info.cti;
        if (image == IND_FOLDER)
        {
            if (!folder_specified)  // take the folder name from the deepest level only, ignore the others
            // must not depend on *root_info.folder_name!
            {
                strcpy(info.folder_name, GetItemText(item).mb_str(*cdp->sys_conv));
                folder_specified = true;
                info.object_count = 1;
            }
        }
        else if (image == IND_SERVERS)
            info.topcat=TOPCAT_SERVERS;
        else if (image == IND_ODBCDSNS)
            info.topcat=TOPCAT_ODBCDSN;
        else if (image == IND_CLIENT)
            info.topcat=TOPCAT_CLIENT;
        else if (image_info.IsServer())  // 602SQL or ODBC DSN
        {
            strcpy(info.server_name, GetItemText(item).mb_str(*wxConvCurrent));
            t_avail_server * as = find_server_connection_info(info.server_name);
            if (as)
              { info.cdp = as->cdp;  info.conn = as->conn; }
            if (just_starting) 
            { 
                info.syscat       = SYSCAT_APPLS;
                info.object_count = 1;
                info.flags        = image_info.IsRemServer() ? 1 : 0;
            }
        }
        else if (image == IND_APPL)
        {
            strcpy(info.schema_name, GetItemText(item).mb_str(GetConv(cdp)));
            if (info.syscat == SYSCAT_NONE)
                info.syscat = SYSCAT_APPLTOP;
            if (!strcmp(info.schema_name, "<Default>"))
              *info.schema_name=0;
            if (just_starting)
            {
                info.category     = CATEG_APPL;
                info.object_count = 1;
            }
        }
        else if (image == IND_FOLDER_SYSTEM || image == IND_FOLDER_SYSOBJS || image == IND_FOLDER_TOOLS || 
                 image == IND_CONSISTENCY ||  image == IND_LICENCES ||  image == IND_RUNPROP || image == IND_PROTOCOLS ||   
                 image == IND_FILENC ||
                 image == IND_MESSAGE ||  image == IND_BACKUP ||  image == IND_SRVCERT || image == IND_EXP_USERS ||
                 image == IND_IMP_USERS ||  image == IND_SERV_RENAME || image == IND_PROFILER || image==IND_HTTP)
        {
            if (info.syscat == SYSCAT_NONE)
                info.syscat = SYSCAT_CONSOLE;
            if (!folder_specified && image != IND_FOLDER_SYSTEM)
            {
                strcpy(info.folder_name, GetItemText(item).mb_str(*wxConvCurrent));
                folder_specified = true;
            }
        }
        else if (image == IND_USER)
        { 
            info.syscat   = SYSCAT_CATEGORY;
            info.category = CATEG_USER;
        }
        else if (image == IND_GROUP)
        { 
            info.syscat   = SYSCAT_CATEGORY;
            info.category = CATEG_GROUP;
        }
        else if (image == IND_SYSTABLE)
        { 
            info.syscat   = SYSCAT_CATEGORY;
            info.category = CATEG_TABLE;
        }
        else if (image != -1 && image != IND_MAIL) // top
        { 
            image2categ_flags(image, info.category, info.flags);
            info.syscat = SYSCAT_CATEGORY;  
            info.object_count = 1;
        }
        item = GetItemParent(item);
        just_starting = false;
    }
    // now info.cdp is valid, can use (only schema names and folder names can be in the tree):
    if (*info.schema_name && info.cdp)
    { 
        cd_Find_prefixed_object(info.cdp, NULL, info.schema_name, CATEG_APPL, &info.objnum);
        info.flags=0;
    }
}
//
// Finds tree item defined by info
//
wxTreeItemId ControlPanelTree::FindChild(const wxTreeItemId &parent, item_info & info)
{ 
    wxTreeItemIdValue cookie;
    wxTreeItemId ch = GetFirstChild(parent, cookie);
    while (ch.IsOk())
    {
        item_info chinfo;
        get_item_info(ch, chinfo);
        if (info==chinfo)
            return(ch);
        ch = GetNextChild(parent, cookie);
    }
    return wxTreeItemId();  // must be !IsOk()
}
//
// Finds server or ODBC datasource item
//
wxTreeItemId ControlPanelTree::FindServer(const wxChar *ServerName)
{
    wxTreeItemId Result, Servers;
    Servers = GetItem(rootId, ROOTITM_SERVERS);
    if (Servers.IsOk())
      Result = GetItem(Servers, ServerName);
   // try ODBC if not found among 602SQL servers:
    if (!Result.IsOk())
    { Servers = GetItem(rootId, ROOTITM_ODBC);
      if (Servers.IsOk())
        Result = GetItem(Servers, ServerName);
    }
    return Result;
}
//
// Adds to tree not registered runnuing serevers found in network
//
void ControlPanelTree::add_running_server(const char *ServerName)
{ 
    const wxString nm(ServerName, *wxConvCurrent);
    wxTreeItemId Parent = GetItem(GetRootItem(), ROOTITM_SERVERS);
    if (Parent.IsOk() && !FindServer(nm).IsOk())
    { Freeze();  // without this the tree control is not updated when the fictive item is added
      wxTreeItemId new_item = InsertItem(Parent, GetNameIndex(Parent, (const wxChar *)nm), nm, IND_SRVREMRUN, IND_SRVREMRUN);  
      AppendFictiveChild(new_item);  // fictive child
      Thaw();
      if (IsSelected(Parent) && objlist->FindItem(-1, nm) == -1)
        objlist->Refresh();
    }
}
//
// Selects tree item activated in listview, causes refilling list
//
void ControlPanelTree::select_child(item_info & info)
{ 
    wxTreeItemId item = GetSelection();
    if (!IsExpanded(item))
        Expand(item);
    wxTreeItemId ch = FindChild(item, info);
    if (ch.IsOk())
        SelectItem(ch);
}

void ControlPanelTree::select_parent(void)
// Used when ".." folder is activated in the list. Changing the root of the tree not supported yet.
{ wxTreeItemId parent = GetItemParent(GetSelection());
  if (parent.IsOk() && parent!=rootId)  // crashes is the invisible root is selected
    SelectItem(parent);
}
//
// Expands selected item
//
void ControlPanelTree::expand_selected(void)
{ wxTreeItemId item = GetSelection();
  if (!IsExpanded(item)) Expand(item);
}
//
// Inserts folders located below the current folder.
//
void ControlPanelTree::InsertFolders(item_info &info, const wxTreeItemId &item)
{ 
    wxTreeItemId new_item;
    // list folders in the current folder:
    CObjectList ol(info.cdp, info.schema_name);
    for (bool ItemFound = ol.FindFirst(CATEG_FOLDER, info.folder_name); ItemFound; ItemFound = ol.FindNext())
    {
        wxString name = GetSmallStringName(ol);
        new_item = InsertItem(item, GetNameIndex(item, name), name, IND_FOLDER);
        // IF folders below categories
        if (my_control_panel->fld_bel_cat)
        {
            // IF folder has subfolders, append fictive child
            if (GetFolderState(info.cdp, info.schema_name, ol.GetName(), CATEG_FOLDER, true, false) != FST_EMPTY)
                AppendFictiveChild(new_item);
        }
        // IF categories below folders and folder is not empty, append fictive child
        else if (my_control_panel->ShowEmptyFolders || GetFolderState(info.cdp, info.schema_name, ol.GetName(), CATEG_FOLDER, true, false) != FST_EMPTY || GetFolderState(info.cdp, info.schema_name, ol.GetName(), true, true) != FST_EMPTY)
            AppendFictiveChild(new_item);
    }
}

CCategItemDescr CategItemDescr[] =
{
    {IND_TABLE,    IDT_TABLE},        // CATEG_TABLE      0
    {IND_USER,     IDT_USER},         // CATEG_USER       1
    {IND_VIEW,     IDT_VIEW},         // CATEG_VIEW       2
    {IND_CURSOR,   IDT_CURSOR},       // CATEG_CURSOR     3
    {IND_PROGR,    IDT_PROG},         // CATEG_PGMSRC     4
    {IND_PROGR,    IDT_PROG},         // CATEG_PGMEXE     5
    {-1,           IDT_MENU},         // CATEG_MENU       6
    {IND_APPL,     0},                // CATEG_APPL       7
    {IND_PICT,     IDT_PICT},         // CATEG_PICT       8
    {IND_GROUP,    IDT_GROUP},        // CATEG_GROUP      9
    {IND_ROLE,     IDT_ROLE},         // CATEG_ROLE       10
    {-1,           IDT_CONNECTION},   // CATEG_CONNECTION 11
    {IND_RELAT,    IDT_RELATION},     // CATEG_RELATION   12
    {IND_DRAW,     IDT_DRAWING},      // CATEG_DRAWING    13
    {-1,           0},                // CATEG_GRAPH      14
    {IND_REPLSRV,  IDT_REPLREL},      // CATEG_REPLREL    15
    {IND_PROC,     IDT_PROC},         // CATEG_PROC       16
    {IND_TRIGGER,  IDT_TRIGGER},      // CATEG_TRIGGER    17
    {-1,           0},                // CATEG_WWW        18
    {IND_FOLDER,   0},                // CATEG_FOLDER     19 
    {IND_SEQ,      IDT_SEQUENCE},     // CATEG_SEQ        20
    {IND_FULLTEXT, IDT_FULLTEXT},     // CATEG_INFO       21
    {IND_DOM,      IDT_DOMAIN},       // CATEG_DOMAIN     22
    {IND_STSH,     IDT_STSH},         // CATEG_STSH       23
    {IND_XMLFORM,  IDT_XMLFORM},      // CATEG_XMLFORM    24
    {-1,           0},                // CATEG_KEY        25 
    {IND_REPLSRV,  IDT_REPLSERVER},   // CATEG_SERVER     26 
};

// Inserts subtree of category names.
// For [fld_bel_cat] categories have subtrees if any folder exists
// For [!fld_bel_cat] categories are listed only if any object exists in the category and folder.
void ControlPanelTree::InsertCategories(item_info & info, const wxTreeItemId &item)
{ 
    CObjectList ol(info.cdp, info.schema_name);
    bool ShowAllways    = my_control_panel->ShowEmptyFolders || !*info.folder_name;
    bool FoldersBelow   = my_control_panel->ShowFolders && my_control_panel->fld_bel_cat && info.cdp && (ol.GetCount(CATEG_FOLDER) > 0);

    CCategList cl;
    do
    { if (info.cdp || cl.GetCateg()==CATEG_TABLE || cl.GetCateg()==CATEG_CURSOR)
        if (cl.HasFolders() || IsRootFolder(info.folder_name))
            InsertCategorie(info.cdp, item, info.schema_name, info.folder_name, cl.GetCateg(), ShowAllways, FoldersBelow);
    }
    while (cl.FindNext());
}
//
// Inserts categorie item to tree
// Input:   Parent       - Parent item
//          Schema       - Schema name
//          Folder       - Folder name
//          Categ        - Category
//          ShowAllways  - true for root folder or when show empty folders is requested
//          FoldersBelow - true when folders should be displayed below categories
//
void ControlPanelTree::InsertCategorie(cdp_t cdp, const wxTreeItemId &Parent, const char *Schema, const char *Folder, tcateg Categ, bool ShowAllways, bool FoldersBelow)
{
    int FolderState = FST_UNKNOWN;
    // IF show allways is requested OR given folder contains some object of given category
    if (ShowAllways || (FolderState = GetFolderState(cdp, Schema, Folder, Categ, true, false)) != FST_EMPTY)
    { 
        wchar_t wc[64];
        // Add category item
        wxTreeItemId Item = AppendItem(Parent, CCategList::Plural(cdp, Categ, wc, true), CategItemDescr[Categ].m_Image);
        // Folders below categories AND category with folders
        if (FoldersBelow && Categ != CATEG_ROLE && Categ != CATEG_WWW && Categ != CATEG_XMLFORM && Categ != CATEG_STSH)
        {   
            // IF Folder has subfolders, append fictive child
            if (FolderState == FST_UNKNOWN)
                FolderState = GetFolderState(cdp, Schema, Folder, CATEG_FOLDER, true, false);
            if (FolderState & FST_HASITEMS)
                AppendFictiveChild(Item);
        }
    }
}
//
// Inserts console items to tree
//
void ControlPanelTree::InsertServerConsoleItems(cdp_t cdp, const wxTreeItemId &parent)
{ 
    wxTreeItemId Item, SubItem;
    wchar_t wc[64];
#if WBVERS<1100
    AppendItem(parent, CTI_LICENCES, IND_LICENCES);
#endif    
    AppendItem(parent, CTI_RUNPROP,  IND_RUNPROP);
    AppendItem(parent, CTI_PROTOCOLS,IND_PROTOCOLS);

    Item     = AppendItem(parent,  CTI_SYSOBJS,                        IND_FOLDER_SYSOBJS);
               AppendItem(Item,    CCategList::Plural(cdp, CATEG_USER,  wc, true),  IND_USER);
               AppendItem(Item,    CCategList::Plural(cdp, CATEG_GROUP, wc, true), IND_GROUP);
               //AppendItem(Item,    CATNAME_REPLSERVER, IND_REPLSERVER);
               AppendItem(Item,    CTI_SYSTABLES,                      IND_SYSTABLE);
    Item     = AppendItem(parent,  CTI_TOOLS,                          IND_FOLDER_TOOLS);
               AppendItem(Item,    CTI_BACKUP,                         IND_BACKUP);
               //AppendItem(Item,    CTI_REPLAY,                         IND_REPLAY);
               AppendItem(Item,    CTI_MESSAGE,                        IND_MESSAGE);
               AppendItem(Item,    CTI_CONSISTENCY,                    IND_CONSISTENCY);
               AppendItem(Item,    CTI_FILENC,                         IND_FILENC);
               AppendItem(Item,    CTI_SRVCERT,                        IND_SRVCERT);
               //AppendItem(Item,    CTI_COMPACT,                        IND_COMPACT);
               AppendItem(Item,    CTI_SERV_RENAME,                    IND_SERV_RENAME);
               AppendItem(Item,    CTI_EXPORT_USERS,                   IND_EXP_USERS);
               AppendItem(Item,    CTI_IMPORT_USERS,                   IND_IMP_USERS);
               AppendItem(Item,    CTI_PROFILER,                       IND_PROFILER);
#if WBVERS>=1100
               AppendItem(Item,    CTI_EXTENSIONS,                     IND_LICENCES);
#endif    
#ifdef XMLFORMS
               AppendItem(Item,    CTI_HTTP,                           IND_HTTP);
#endif
}
//
// Item selected event handler
//
void ControlPanelTree::OnItemSelected(wxTreeEvent &event)
{ 
    wxTreeItemId Item = event.GetItem();
    // Ignore if selected item not chnged
    if (Item == m_SelItem)
        return;
    m_SelItem = Item;
    // Get item info
    get_item_info(Item, m_SelInfo);
    // IF not from keyboard, provide selection change action
    if (!m_FromKeyboard)
        SelChange();
}
//
// Timeout elapsed event handler - provides delayed selected item change action
//
void ControlPanelTree::OnTimer(wxTimerEvent &event)
{
    m_FromKeyboard = false;
    SelChange();
}
//
// Selected item change action is especialy due to listview fill rather slow operation, 
// therefore it blocks fast treeview scroll using up or down key. So any up or down key hit starts
// new short timeout, when timeout elapses, change action is provided for last selected item
//
void ControlPanelTree::SelChange()
{
    // Change current schema if needed
    m_SelInfo.new_current_schema();
    // Refill object listview
    if (objlist)
        objlist->Fill();

    MyFrame *frame = my_control_panel->frame;
    if (frame)  // update action menu
    {
#ifdef LINUX
        frame->update_action_menu_based_on_selection(true);  // may be more robust
#else
        C602SQLDataObject Data;
        frame->update_action_menu_based_on_selection(wxIsDragResultOk(m_SelInfo.CanPaste(Data.GetDataFromClipboard(), false)));  // may be more robust
#endif
        // global settings:
        if (frame->SQLConsole)
            frame->SQLConsole->Push_cdp(m_SelInfo.xcdp());
        if (frame->ColumnList)
            frame->ColumnList->Push_cdp(m_SelInfo.xcdp());
        update_CP_toolbar(frame, m_SelInfo);   // must be called AFTER updating the action menu
        frame->update_object_info(frame->output_window->GetSelection());
    }
}

#ifdef LINUX

void ControlPanelTree::OnInternalIdle()
{
    wxTreeCtrl::OnInternalIdle();
    m_FromFocus = false;
}

#endif    

// Creates a fictive child
void ControlPanelTree::AppendFictiveChild(const wxTreeItemId &item)
{
    AppendItem(item, wxT("\1")); 
    Collapse(item);
}
//
// Returns true if item has only fictive child
//
bool ControlPanelTree::HasFictiveChild(const wxTreeItemId &item)
{ 
    wxTreeItemIdValue cookie;
    wxTreeItemId first_child = GetFirstChild(item, cookie);
    if (!first_child.IsOk())
        return false;
    wxString tx = GetItemText(first_child);
    return tx[0u] == '\1';
}
//
// Returns index of child in Parent item where to insert item named Name
//
size_t ControlPanelTree::GetNameIndex(const wxTreeItemId &Parent, const wxChar *Name)
{
    wxTreeItemIdValue Cookie;
    size_t i = 0;
    // IF no children, return 0
    wxTreeItemId Item = GetFirstChild(Parent, Cookie);
    if (!Item.IsOk())
        return 0;
    // IF first child is System folder
    if (GetItemImage(Item) == IND_FOLDER_SYSTEM)
    {
        // Increment index, 
        i++;
        // IF no next child, return 1
        Item = GetNextChild(Parent, Cookie);
        if (!Item.IsOk())
            return 1;
    }
    int ii = GetItemImage(Parent);
    item_info ix;  ix.cti=(tcpitemid)ii;
    // IF schema list
    if (ix.IsServer())
    {
        // _sysext is allways just after System folder
        if (wxStrcmp(Name, wxT("_sysext")) == 0)
            return 1;
        // IF current child is _sysext, increment index, get next child
        if (GetItemText(Item)==wxT("_sysext"))
        {
            i++ ;
            Item = GetNextChild(Parent, Cookie);
        }
    }
    // IF next child exists, find next child until its name is > Name
    if (Item.IsOk())
    {
        ii = GetItemImage(Item);
        wxString wName(Name, GetConv(m_SelInfo.cdp));
        while (wxStricmp(GetItemText(Item), wName) <= 0)
        {
            i++;
            Item = GetNextChild(Parent, Cookie);
            if (!Item.IsOk())
                break;
            if (ii == IND_FOLDER && GetItemImage(Item) != ii)
                break;
        }
    }
    return i;
}

#ifdef CONTROL_PANEL_INPLECE_EDIT

void ControlPanelTree::OnBeginLabelEdit(wxTreeEvent & event)
{ 
    int Image = GetItemImage(event.GetItem()); 
    // Editovat lze jen aplikaci nebo folder
    if (Image == IND_APPL)
        m_EditCateg = CATEG_APPL;
    else if (Image == IND_FOLDER)
        m_EditCateg = CATEG_FOLDER;
    else
    {
        event.Veto();
        return;
    }
#ifdef _DEBUG
    //::wxLogDebug(wxT("OnBeginLabelEdit %s"), GetItemText(event.GetItem()).c_str());
#endif
    m_InEdit   = true;
    m_EditItem = m_SelItem;
    // Potrehuji link na EditControl, ale v tomto okamziku jeste neexistuje, tak ho musim zjistit o udalost pozdeji
    wxCommandEvent nextEvent(myEVT_INITEDIT, TREE_ID);
    AddPendingEvent(nextEvent);
}

void ControlPanelTree::OnInitEdit(wxCommandEvent &event)
{
    // Editace v TreeView skonci se stavem IsEditCancelled, kdyz editaci zrusim, ale i kdyz editaci beze zmeny potvrdim,
    // Proto potrebuju udelat nejakou pseudo zmenu
    m_EditCancelled  = false;
    wxTextCtrl *Edit = GetEditControl();
    Edit->SetMaxLength(OBJ_NAME_LEN);
    Edit->WriteText(GetItemText(m_EditItem));
    Edit->SetSelection(-1, -1);
    Edit->PushEventHandler(&m_ItemEdit);
#ifdef LINUX
    m_ItemEdit.m_InEdit = true;
#endif
}

void ControlPanelTree::OnEndLabelEdit(wxTreeEvent &event)
{
    // Osetreni noveho jmena nejde delat tady (v polozce je videt jeste puvodni hodnota, znovuzavolani EditLabel funguje blbe),
    // je treba ho zpracovat o udalost pozdeji
    m_EditCancelled = event.IsEditCancelled();
    wxCommandEvent nextEvent(myEVT_AFTEREDIT, TREE_ID);
    AddPendingEvent(nextEvent);
}

void ControlPanelTree::OnAfterEdit(wxCommandEvent &event)
{
    m_InEdit = false;
    // IF editace zrusena
    char *OldName = m_EditCateg == CATEG_APPL ? m_SelInfo.schema_name : m_SelInfo.folder_name;
    if (m_EditCancelled)
    {
        // IF vytvareni nove slozky, zrusit nove vlozenou polozku
        if (m_NewFolder)
        {
            m_NewFolder = false;
            Delete(m_EditItem);
        }
        // ELSE vratit puvodni text
        else
        {
            SetItemText(m_EditItem, wxString(OldName, *m_SelInfo.cdp->sys_conv));
        }
        m_EditItem = wxTreeItemId();
#ifdef LINUX
        wxWindow::SetFocus();
#endif                
        return;
    }

    wxString sNewName = GetItemText(m_EditItem);
    CaptFirst(sNewName);
    tccharptr NewName = sNewName.mb_str(*m_SelInfo.cdp->sys_conv);
    // IF nebyla zmena, nic
    if (!m_NewFolder && strcmp(NewName, OldName) == 0)
    {
        m_EditItem  = wxTreeItemId();
        m_NewFolder = false;
#ifdef LINUX
        wxWindow::SetFocus();
#endif                
        return;
    }
    // Zkontrolovat platnost a unikatnost jmena
    if (!is_valid_object_name(m_SelInfo.cdp, NewName, my_control_panel->frame) || ObjNameExists(m_SelInfo.cdp, NewName, m_EditCateg))
    {
        SelectItem(m_EditItem);
        EditLabel(m_EditItem);
        return;
    }

    // IF Vytvorit novy folder 
    if (m_NewFolder)
    {
        wxString ParentName;
        wxTreeItemId Item = GetItemParent(m_EditItem);
        if (GetItemImage(Item) == IND_FOLDER)
            ParentName = GetItemText(Item);
        Create_folder(m_SelInfo.cdp, NewName, ParentName.mb_str(*m_SelInfo.cdp->sys_conv)); 
        m_NewFolder = false;
    }
    // ELSE prejmenovat
    else
    {
        if (Rename_object(m_SelInfo.cdp, OldName, NewName, m_EditCateg))
            cd_Signalize2(m_SelInfo.cdp, my_control_panel->frame);
    }
    SetItemText(m_EditItem, sNewName);
    m_EditItem = wxTreeItemId();
    if (m_EditCateg == CATEG_FOLDER)
        RefreshSchema(m_SelInfo.server_name, m_SelInfo.schema_name);
#ifdef LINUX    
    wxWindow::SetFocus();
#endif    
}

void ControlPanelTree::NewFolder()
{
    cdp_t cdp = m_SelInfo.cdp;
    tobjname Folder;

    strcpy(Folder, ToChar(_("Folder"), cdp->sys_conv));

    if (!GetFreeObjName(cdp, Folder, CATEG_FOLDER))
    {
        cd_Signalize2(cdp, my_control_panel->frame);
        return;
    }
    wxTreeItemId Parent = GetSelection();
    Expand(Parent);
    wxTreeItemIdValue cookie;
    wxTreeItemId      bItem;
    size_t            Before;
    for (bItem = GetFirstChild(Parent, cookie), Before = 0; bItem.IsOk() && GetItemImage(bItem) == IND_FOLDER; bItem = GetNextChild(Parent, cookie), Before++);
    wxTreeItemId NewItem = InsertItem(Parent, Before, wxString(Folder, *cdp->sys_conv), IND_FOLDER, IND_FOLDER);
    if (!NewItem.IsOk())
        return;
    SelectItem(NewItem);
    m_NewFolder = true;
    EditLabel(NewItem);
}

void ControlPanelTree::OnItemSelecting(wxTreeEvent &event)
{
    // Nepovolit, pokud editujeme polozku
    if (m_EditItem.IsOk() || objlist->m_EditItem != -1)
        event.Veto();
    else
        event.Skip();
}

#else  // !CONTROL_PANEL_INPLECE_EDIT

//
// Create new folder action
//
void ControlPanelTree::NewFolder()
{
    cdp_t cdp = m_SelInfo.cdp;
    tobjname Folder = "";

    // Get free name for folder
    wxTreeItemId Parent = m_SelItem;
    if (!get_name_for_new_object(cdp, wxGetApp().frame, cdp->sel_appl_name, CATEG_FOLDER, _("Enter the new folder name:"), Folder))
        return;
    // Create folder in database
    if (Create_folder(cdp, Folder, m_SelInfo.folder_name))
        cd_Signalize(cdp);
    // Refresh schema in tree 
    RefreshSchema(m_SelInfo.server_name, m_SelInfo.schema_name);
    wxTreeItemIdValue cookie;
    // Find new folder item, select it
    for (wxTreeItemId Item = GetFirstChild(Parent, cookie); Item.IsOk(); Item = GetNextChild(Item, cookie))
    {
        if (wb_stricmp(cdp, GetItemText(Item).mb_str(*cdp->sys_conv), Folder) == 0)
        {
            SelectItem(Item);
            break;
        }
    }
}
//
// Rename object action
//
void ControlPanelTree::OnRename(wxCommandEvent &event)
{
    // Ignore if no item selected
    if (!m_SelItem.IsOk())
        return;
    tobjname Name;
    tcateg   Categ;
    // Get selected object name and category
    if (m_SelInfo.IsSchema())
    {
        strcpy(Name, m_SelInfo.schema_name);
        Categ = CATEG_APPL;
    }
    else
    {
        strcpy(Name, m_SelInfo.folder_name);
        Categ = CATEG_FOLDER;
    }
    wxTreeItemId Parent = GetItemParent(m_SelItem);
    // Rename object in database - refreshes control panel
    if (!RenameObject(m_SelInfo.cdp, Name, Categ))
        return;
    wb_small_string(m_SelInfo.cdp, Name, true);
    strcpy(m_SelInfo.IsSchema() ? m_SelInfo.schema_name : m_SelInfo.folder_name, Name);
    // Find item with new name, select it
    wxTreeItemId Item = FindItem(Parent, m_SelInfo);
    if (Item.IsOk())
        SelectItem(Item);
    if (Categ == CATEG_APPL)  // name is on the source page
        wxGetApp().frame->update_object_info(wxGetApp().frame->output_window->GetSelection());
}

#endif // CONTROL_PANEL_INPLECE_EDIT


void ControlPanelTree::insert_cp_item(item_info & info, bool select_it)
// Only adding below the selected item impemented
{
    wxTreeItemId item = AddObject(info.server_name, info.schema_name, info.folder_name, info.object_name, info.category);
    if (select_it && item.IsOk())
        SelectItem(item);
}
//
// Key down event handler
//
void ControlPanelTree::OnKeyDown(wxKeyEvent &event)
{
#ifdef CONTROL_PANEL_INPLECE_EDIT
	if (m_InEdit)
		return;
#endif
    int  Code     = event.GetKeyCode();
    bool CtrlDown = event.ControlDown();
    // Delete - Execute delete object action
    if (Code == WXK_DELETE)
    {
        wxCommandEvent cmd; // (wxEVT_COMMAND_BUTTON_CLICKED, CPM_DELETE) does not compile on GTK
        cmd.SetId(CPM_DELETE);
        my_control_panel->frame->OnCPCommand(cmd);
    }
    // F1 - show help
    else if (Code == WXK_F1)
        my_control_panel->frame->help_ctrl.DisplaySection(wxT("xml/html/controlpanel.html"));
#ifdef CONTROL_PANEL_INPLECE_EDIT
    else if (Code == WXK_F2 && !m_InEdit)
    {
	    m_InEdit = true;
        EditLabel(m_SelItem);
    }
#endif
    // Enter - expand item
    else if (Code == WXK_RETURN)
    { 	// on GTK return falls back to parent if not precessed explicitly
      	// note: event.GetItem() is invalid
      	Expand(m_SelItem);
    }
    // Ctrl/C - Execute copy action
    else if (Code == 'C' && CtrlDown)
    {
        OnCopy((wxCommandEvent &)event);
    }
    // Ctrl/X - Execute cut action
    else if (Code == 'X' && CtrlDown)
    {
        OnCut((wxCommandEvent &)event);
    }
    // Ctrl/X - Execute paste action
    else if (Code == 'V' && CtrlDown)
    {
        OnPaste((wxCommandEvent &)event);
    }
    // Menu - Show popup menu
#ifdef WINS
    else if (Code == WXK_WINDOWS_MENU)
#else
    else if (Code == WXK_MENU)
#endif
	{
		wxTreeItemId Item = GetSelection();
		if (Item.IsOk())
		{
			int Flags;
			wxPoint Position;
			wxRect Rect = GetClientRect();
			Position.x = 0;
			for (Position.y = 0; Position.y < Rect.height && HitTest(Position, Flags) != Item; Position.y += 10);
			if (Position.y >= Rect.height)
				Position.y = 40;
			Position.x = 60;
			ShowPopupMenu(Item, Position);
		}
	}
    // Down, Up - Initialize delayed selected item change action
    else if (Code == WXK_DOWN || Code == WXK_UP)
    {
        m_Timer.Stop();
        m_Timer.Start(220, true);
        m_FromKeyboard = true;
        event.Skip();
    }
    else
        event.Skip();
}
//
// Begin drag event handler
//
void ControlPanelTree::OnBeginDrag(wxTreeEvent &event)
{
    SetFocus();
    wxTreeItemId Item = event.GetItem();
    SelectItem(Item);

#ifdef WINS
    wxPoint Pos = event.GetPoint();
#else
    // On linux is event invoked even if click is outside any item
    wxPoint Pos = wxGetMousePosition();
    Pos         = ScreenToClient(Pos);
    int Flags;
    HitTest(Pos, Flags);
    if (!(Flags & wxTREE_HITTEST_ONITEM))
        return;
#endif        
        
    // EXIT IF object can't be copied
    if (!m_SelInfo.CanCopy())
        return;
    // Create selection iterator
    single_selection_iterator si(m_SelInfo);
    // Create custom copy descriptor
    t_copyobjctx ctx;
    memset(&ctx, 0, sizeof(t_copyobjctx));
    ctx.strsize  = sizeof(t_copyobjctx);
    ctx.count    = 1;
    ctx.param    = &si;
    ctx.nextfunc = get_next_sel_obj;
    // If object is folder, write parent folder name to descriptor
    if (m_SelInfo.IsFolder())
    {
        CObjectList ol(m_SelInfo.cdp, m_SelInfo.schema_name);
        ol.Find(m_SelInfo.folder_name, CATEG_FOLDER);
        strcpy(ctx.folder, ol.GetFolderName());
    }
    // If object is category, write folder name to descriptor
    else if (m_SelInfo.IsCategory())
    {
        strcpy(ctx.folder, m_SelInfo.folder_name);
    }
    // Create copy context
    CCopyObjCtx Ctx(m_SelInfo.cdp, m_SelInfo.schema_name);
    // Get DataObject with copied object
    C602SQLDataObject *dobj = Ctx.Copy(&ctx);
    if (!dobj)
        return;

#ifdef WINS
    // Image dragging on linux conflicts with Drag&Drop implementation so it is not used
    wxRect Rect;
    // Get item boundig rectangle
    GetBoundingRect(event.GetItem(), Rect, true);
    // Calculate drag image position
#ifdef WINS
    Rect.x -= PatchedGetMetric(wxSYS_SMALLICON_X) + 3;
#else
    Rect.x += PatchedGetMetric(wxSYS_SMALLICON_X) - 10;
    Rect.y += 4;
#endif    
    Pos    -= Rect.GetPosition();
    // Create drag image
    CWBDragImage di(*this, Item);
    // Start image dragging
    if (di.BeginDrag(Pos, this, true))
    {
        di.Show();
        di.Move(event.GetPoint());
    }
#endif  // WINS

    // Start Drag&Drop operation
#ifdef WINS    
    CCPDragSource ds(this, dobj);
#else    
    CCPDragSource ds(this, dobj, get_image_index(m_SelInfo.category, m_SelInfo.flags));
#endif    
    wxDragResult Res = ds.DoDragDrop(wxDrag_AllowMove);
    
#ifdef WINS
    di.EndDrag();
#else
    Refresh();
#endif
}
//
// Refresches tree content
//
void ControlPanelTree::Refresh(void)
{
    // Refresh server list
    refresh_server_list();
    wxBusyCursor Busy;
    // Prevent tree redrawing while refreshing
    Freeze();
    // Prevent list refilling while refreshing
    objlist->m_NoRelist = true;
    // Refresh Serevers branch
    wxTreeItemId Parent = GetItem(GetRootItem(), ROOTITM_SERVERS);
    RefreshServers(Parent);
    Parent = GetItem(GetRootItem(), ROOTITM_ODBC);
    // Refresh ODBC datasources branch
    RefreshODBCDSs(Parent);
    wxWindow::Refresh(false);
    objlist->m_NoRelist = false;
    // Update current item info
    get_item_info(GetSelection(), m_SelInfo);
    // Refill list
    objlist->Fill();
    Thaw();
}
//
// Refreshes subtree
//
void ControlPanelTree::RefreshItem(const wxTreeItemId &Item)
{
    item_info info;
    // IF item is selected item, update info
    get_item_info(Item, info);
    if (Item == m_SelItem)
        m_SelInfo = info;
    // IF Item is root, refresh all servers and ODBC data sources
    if (info.IsRoot())
    {
        RefreshServers(GetItem(GetRootItem(), ROOTITM_SERVERS));
        RefreshODBCDSs(GetItem(GetRootItem(), ROOTITM_ODBC));
    }
    // IF Item is servers branch, refresh servers
    else if (info.IsServers())
    {
        RefreshServers(Item);
    }
    // IF Item is ODBC data sources branch, refresh ODBC data sources
    else if (info.IsODBCDSs())
    {
        RefreshODBCDSs(Item);
    }
    // IF Item is server branch, refresh server content
    else if (info.IsServer())
    { 
        t_avail_server * as = find_server_connection_info(info.server_name);
        if (as)
            RefreshServer(as, Item);
    }
    // IF Item is schema branch, refresh schema content
    else if (info.IsSchema())
        RefreshAppl(info.cdp, Item, info.schema_name);
    // IF Item is folder branch, refresh folder content
    else if (info.IsFolder())
        RefreshFolder(info.cdp, Item, info.schema_name, info.folder_name, info.category);
    // IF Item is category branch, refresh category content
    else if (info.IsCategory())
    {
        CObjectList ol(info.cdp, info.schema_name);
        bool ShowAllways    = my_control_panel->ShowEmptyFolders || *info.folder_name == 0;
        bool FoldersBelow   = my_control_panel->ShowFolders && my_control_panel->fld_bel_cat && (ol.GetCount(CATEG_FOLDER) > 0);
        RefreshCategorie(info.cdp, GetItemParent(Item), info.schema_name, info.folder_name, info.category, ShowAllways, FoldersBelow);
    }
}
//
// Refreshes servers set
//
void ControlPanelTree::RefreshServers(const wxTreeItemId &Parent)
{
    // IF only fictive child i.e. Parent not expanded, nothing to do
    if (HasFictiveChild(Parent))
        return;
    wxTreeItemIdValue Cookie;
    wxTreeItemId Item;
    CObjectList  ol;
    // FOR each child
    for (Item = GetFirstChild(Parent, Cookie); Item.IsOk();)
    {
        wxTreeItemId DelItem;
        // IF server found in ObjectList, refresh server
        if (ol.Find(GetItemText(Item).mb_str(*wxConvCurrent), XCATEG_SERVER))
        {
            t_avail_server * Server = (t_avail_server *)ol.GetSource();
            RefreshServer(Server, Item);
        }
        // ELSE remove server from tree
        else
            DelItem = Item;
        Item = GetNextChild(Parent, Cookie);
        if (DelItem.IsOk())
            Delete(DelItem);
    }

    // FOR each server in ObjectList
    for (bool ItemFound = ol.FindFirst(XCATEG_SERVER); ItemFound; ItemFound = ol.FindNext())
    {
        // IF server item not exists in tree, insert
        Item = GetItem(Parent, GetWName(ol));
        if (!Item.IsOk())
        {
            t_avail_server *Server = (t_avail_server *)ol.GetSource();
            wxString nm (Server->locid_server_name, *wxConvCurrent);
            wxTreeItemId new_item = InsertItem(Parent, GetNameIndex(Parent, nm), nm, Server->server_image, Server->server_image);
            AppendFictiveChild(new_item);  // fictive child
        }
    }
}
//
// Refreshes server content
//
void ControlPanelTree::RefreshServer(t_avail_server * Server, const wxTreeItemId &Parent)
{ 
    // Update item image
    SetItemImage(Parent, Server->server_image);
    SetItemImage(Parent, Server->server_image, wxTreeItemIcon_Selected);
    // IF only fictive child i.e. Parent not expanded, nothing to do
    if (HasFictiveChild(Parent))
        return;
    // IF server not conected
    if (!Server->cdp && !Server->conn)
    {
        bool SelIsChild = IsChild(m_SelItem, Parent);
        // Collapse server item, remove children, append fictive child
        Collapse(Parent);
        DeleteChildren(Parent);
        AppendFictiveChild(Parent);
        // IF selected item is child of server item, select server item
        if (SelIsChild)
        {
            objlist->Reset();
            SelectItem(Parent);
        }
        return;
    }

    wxTreeItemIdValue Cookie;
    wxTreeItemId Item;
    CObjectList  ol(Server);

    // Skip System folder item
    Item = GetFirstChild(Parent, Cookie);
    if (Item.IsOk() && GetItemImage(Item) == IND_FOLDER_SYSTEM)
        Item = GetNextChild(Parent, Cookie);

    // FOR each schema item in tree
    while (Item.IsOk())
    {
        wxTreeItemId DelItem;
        // IF schema found in ObjectList, refresh schema
        tccharptr SchemaName = GetItemText(Item).mb_str(GetConv(Server->cdp));
        if (ol.Find(SchemaName, CATEG_APPL))
            RefreshAppl(Server->cdp, Item, SchemaName);
        // ELSE remove schema item
        else
            DelItem = Item;
        Item = GetNextChild(Parent, Cookie);
        if (DelItem.IsOk())
            Delete(DelItem);
    }

    // FOR each schema in ObjectList
    for (bool ItemFound = ol.FindFirst(CATEG_APPL); ItemFound; ItemFound = ol.FindNext())
    {
        // IF schema item not exists in tree, insert
        Item = GetItem(Parent, GetWName(ol));
        if (!Item.IsOk())
        {
            wxString nm = GetSmallStringName(ol);
            wxTreeItemId new_item = InsertItem(Parent, GetNameIndex(Parent, nm), nm, IND_APPL);
            AppendFictiveChild(new_item);  // fictive child
        }
    }
}
//
// Refreshes ODBC datasources
//
void ControlPanelTree::RefreshODBCDSs(const wxTreeItemId &Parent)
{
    // IF only fictive child i.e. Parent not expanded, nothing to do
    if (HasFictiveChild(Parent))
        return;
    wxTreeItemIdValue Cookie;
    wxTreeItemId Item;
    CObjectList  ol;
    // FOR each datasource item in tree
    for (Item = GetFirstChild(Parent, Cookie); Item.IsOk();)
    {
        wxTreeItemId DelItem;
        // IF datasource found in ObjectList
        if (ol.Find(GetItemText(Item).mb_str(*wxConvCurrent), XCATEG_ODBCDS))
        {
            t_avail_server * Server = (t_avail_server *)ol.GetSource();
            //RefreshServer(Server, Item);
        }
        // ELSE remove datasource item
        else
            DelItem = Item;
        Item = GetNextChild(Parent, Cookie);
        if (DelItem.IsOk())
            Delete(DelItem);
    }

    // FOR each datasource in ObjectList
    for (bool ItemFound = ol.FindFirst(XCATEG_ODBCDS); ItemFound; ItemFound = ol.FindNext())
    {
        // IF datasource item not exists in tree, insert
        Item = GetItem(Parent, GetWName(ol));
        if (!Item.IsOk())
        {
            t_avail_server *Server = (t_avail_server *)ol.GetSource();
            wxString nm (Server->locid_server_name, *wxConvCurrent);
            wxTreeItemId new_item = InsertItem(Parent, GetNameIndex(Parent, nm), nm, Server->server_image, Server->server_image);
            AppendFictiveChild(new_item);  // fictive child
        }
    }
}
//
// Refreshes schema content
//
void ControlPanelTree::RefreshAppl(cdp_t cdp, const wxTreeItemId &Parent, const char *Schema)
{
    // IF only fictive child i.e. Parent not expanded, nothing to do
    if (HasFictiveChild(Parent))
        return;
    // Check if schema is current schema
    bool MySchema = wb_stricmp(cdp, Schema, cdp->sel_appl_name) == 0;
    m_ol.SetCdp(cdp, Schema);

    wxTreeItemIdValue Cookie;
    wxTreeItemId Item;
    CObjectList  ol(cdp, Schema);
    item_info    info;
    bool         SelExpanded;

    // IF selected item is child of Parent, store it 
    wxTreeItemId Selection = GetSelection();
    if (IsChild(Selection, Parent))
    {
        get_item_info(Selection, info);
        SelExpanded = IsExpanded(Selection);
    }

    // IF categories below folders
    if (my_control_panel->ShowFolders && !my_control_panel->fld_bel_cat)
    {
        // FOR each folder item in Parent
        for (Item = GetFirstChild(Parent, Cookie); Item.IsOk() && GetItemImage(Item) == IND_FOLDER;)
        {
            wxTreeItemId DelItem;
            // IF folder found in ObjectList, refresh folder
            if (ol.Find(GetItemText(Item).mb_str(GetConv(cdp)), CATEG_FOLDER, ""))
                RefreshFolder(cdp, Item, Schema, ol.GetName(), NONECATEG);
            // ELSE IF current schema, remove item from tree
            // (folders from foreign schema cannot be removed because that they absent in object list doesn't mean that they not exist)
            else if (MySchema)
                DelItem = Item;
            Item = GetNextChild(Parent, Cookie);
            if (DelItem.IsOk())
                Delete(DelItem);
        }
        // FOR each folder in ObjectList
        for (bool ItemFound = ol.FindFirst(CATEG_FOLDER, ""); ItemFound; ItemFound = ol.FindNext())
        {
            // IF folder item not exists it tree, insert
            Item = GetItem(Parent, GetWName(ol), IND_FOLDER);
            if (!Item.IsOk())
            {
                wxString nm = GetSmallStringName(ol);
                wxTreeItemId new_item = InsertItem(Parent, GetNameIndex(Parent, nm), nm, IND_FOLDER);
                CObjectList fl(cdp, Schema);
                if (GetFolderState(cdp, Schema, ol.GetName(), true, true) != FST_EMPTY || GetFolderState(cdp, Schema, ol.GetName(), CATEG_FOLDER, true, false) != FST_EMPTY)
                    AppendFictiveChild(new_item);  // fictive child
            }
        }
    }
    // ELSE folders below categories
    else
    {
        // Remove all folder items
        for (Item = GetFirstChild(Parent, Cookie); Item.IsOk() && GetItemImage(Item) == IND_FOLDER;)
        {
            wxTreeItemId DelItem = Item;
            Item = GetNextChild(Parent, Cookie);
            Delete(DelItem);
        }

    }
    // Refresh categories
    RefreshCategories(cdp, Parent, Schema, "");

    // IF selected item was child of Parent, find and select it again
    if (Selection.IsOk())
    {
        Item = GetSelection();
        if (!Item.IsOk() || Item != Selection)
        {
            Item = FindItem(Parent, info);
            SelectItem(Item);
            if (SelExpanded)
                Expand(Item);
        }
    }
}
//
// Refreshes Folder content
//
void ControlPanelTree::RefreshFolder(cdp_t cdp, const wxTreeItemId &Parent, const char *Schema, const char *Folder, tcateg Categ)
{
    wxTreeItemIdValue Cookie;

    // IF no children
    wxTreeItemId Item = GetFirstChild(Parent, Cookie);
    if (!Item.IsOk())
    {
        // IF folders below categories
        if (my_control_panel->fld_bel_cat)
        {
            // IF folder has subfolders, append fictive child
            if (GetFolderState(cdp, Schema, Folder, CATEG_FOLDER, true, true) != FST_EMPTY)
                AppendFictiveChild(Parent);  // fictive child
        }
        // ELSE categories below folders
        else
        {
            // IF folder is not empty, append fictive child
            if (GetFolderState(cdp, Schema, Folder, CATEG_FOLDER, true, false) != FST_EMPTY || GetFolderState(cdp, Schema, Folder, true, false) != FST_EMPTY)
                AppendFictiveChild(Parent);  // fictive child
        }
        return;
    }
    // IF not expanded
    if (!IsExpanded(Parent))
    {
        // IF folders below categories
        if (my_control_panel->fld_bel_cat)
        {
            // IF folder has no subfolders, delete children
            if (GetFolderState(cdp, Schema, Folder, CATEG_FOLDER, true, true) == FST_EMPTY)
                DeleteChildren(Parent);
        }
        // ELSE categories below folders
        else
        {
            // IF folder is empty, delete children
            if (GetFolderState(cdp, Schema, Folder, CATEG_FOLDER, true, false) == FST_EMPTY && GetFolderState(cdp, Schema, Folder, true, true) == FST_EMPTY)
                DeleteChildren(Parent);
        }
        return;
    }

    CObjectList ol(cdp, Schema);
    // FOR each subfolder item in Parent
    for (;Item.IsOk() && GetItemImage(Item) == IND_FOLDER;)
    {
        wxTreeItemId DelItem;
        // IF subfolder found in ObjectList, refresh subfolder
        if (ol.Find(GetItemText(Item).mb_str(*cdp->sys_conv), CATEG_FOLDER, Folder))
            RefreshFolder(cdp, Item, Schema, ol.GetName(), Categ);
        // ELSE remove subfolder item from tree
        else
            DelItem = Item;
        Item = GetNextChild(Parent, Cookie);
        if (DelItem.IsOk())
            Delete(DelItem);
    }
    // FOR each subfolder in ObjectList
    for (bool ItemFound = ol.FindFirst(CATEG_FOLDER, Folder); ItemFound; ItemFound = ol.FindNext())
    {
        // IF subfolder item not exists in tree, insert
        Item = GetItem(Parent, GetWName(ol), IND_FOLDER);
        if (!Item.IsOk())
        {
            wxString nm = GetSmallStringName(ol);
            wxTreeItemId new_item = InsertItem(Parent, GetNameIndex(Parent, nm), nm, IND_FOLDER);
            CObjectList fl(cdp, Schema);
            if (fl.FindFirst(CATEG_FOLDER, ol.GetName()))
                AppendFictiveChild(new_item);  // fictive child
        }
    }
    // IF categiries below folders, refresh categories
    if (!my_control_panel->fld_bel_cat)
        RefreshCategories(cdp, Parent, Schema, Folder);
}
//
// Refreshes categories
//
void ControlPanelTree::RefreshCategories(cdp_t cdp, const wxTreeItemId &Parent, const char *Schema, const char *Folder)
{
    bool ShowAllways    = my_control_panel->ShowEmptyFolders || !*Folder;
    bool FoldersBelow   = my_control_panel->ShowFolders && my_control_panel->fld_bel_cat && m_ol.GetCount(CATEG_FOLDER) > 0;

    CCategList cl;
    // FOR each category, if category can have folders OR Folder is root folder, refresh category
    do
    {
        if (cl.HasFolders() || IsRootFolder(Folder))
        {
            RefreshCategorie(cdp, Parent, Schema, Folder, cl.GetCateg(), ShowAllways, FoldersBelow);
        }
    }
    while (cl.FindNext());
}
//
// Refreshes category content
//
void ControlPanelTree::RefreshCategorie(cdp_t cdp, const wxTreeItemId &Parent, const char *Schema, const char *Folder, tcateg Categ, bool ShowAllways, bool FoldersBelow)
{
    wxTreeItemIdValue Cookie;

    // IF category not found in tree, insert
    wchar_t wc[64];
    wxTreeItemId cItem = GetItem(Parent, CCategList::Plural(cdp, Categ, wc, true), CategItemDescr[Categ].m_Image);
    if (!cItem.IsOk())
    {
        InsertCategorie(cdp, Parent, Schema, Folder, Categ, ShowAllways, FoldersBelow);
        return;
    }
    // IF folders below categories AND not root folder
    if (!my_control_panel->fld_bel_cat && !IsRootFolder(Folder))
    {
        // IF no object of given category in given folder, remove category item
        CObjectList ol(cdp, Schema);
        if (!ol.FindFirst(Categ, Folder))
        {
            Delete(cItem);
            return;
        }
    }

    // IF new item has no children AND folders below categories AND category can have folders, append fictive child
    wxTreeItemId Item = GetFirstChild(cItem, Cookie);
    if (!Item.IsOk() && FoldersBelow && Categ != CATEG_ROLE && Categ != CATEG_WWW && Categ != CATEG_XMLFORM && Categ != CATEG_STSH)
    {
        AppendFictiveChild(cItem);  // fictive child
        return;
    }
    
    // IF not foldes below categories, remove category item children
    if (!FoldersBelow)
    {
        Collapse(cItem);
        DeleteChildren(cItem);
        return;
    }

    // IF category item expanded
    if (!IsExpanded(cItem))
        return;

    CObjectList ol(cdp, Schema);
    // FOR each folder item in category item
    bool HasFolders = false;
    for (Item = GetFirstChild(cItem, Cookie); Item.IsOk(); Item = GetNextChild(cItem, Cookie))
    {
        // IF folder found in ObjectList, refresh folder
        if (ol.Find(GetItemText(Item).mb_str(*cdp->sys_conv), CATEG_FOLDER, Folder))
        {
            RefreshFolder(cdp, Item, Schema, ol.GetName(), Categ);
            HasFolders = true;
        }
        // ELSE delete folder item
        else
            Delete(Item);
    }
    // FOR each folder in ObjectList
    for (bool ItemFound = ol.FindFirst(CATEG_FOLDER, Folder); ItemFound; ItemFound = ol.FindNext())
    {
        // IF folder item not exists in tree, append
        Item = GetItem(cItem, GetWName(ol));
        if (!Item.IsOk())
        {
            wxString nm = GetSmallStringName(ol);
            wxTreeItemId new_item = InsertItem(cItem, GetNameIndex(cItem, nm), nm, IND_FOLDER);
            if (GetFolderState(cdp, Schema, ol.GetName(), CATEG_FOLDER, true, false) != FST_EMPTY)
                AppendFictiveChild(new_item);  // fictive child
            HasFolders = true;
        }
    }
}
//
// Returns child item of Parent named Name
//
wxTreeItemId ControlPanelTree::GetItem(const wxTreeItemId &Parent, const wxChar *Name)
{
    wxTreeItemIdValue Cookie;  wxTreeItemId  Item;
    for (Item = GetFirstChild(Parent, Cookie); Item.IsOk() && wxStricmp(GetItemText(Item), Name); Item = GetNextChild(Parent, Cookie));
    return(Item);
}
//
// Returns child item of Parent, which name is Name and image index Image
//
wxTreeItemId ControlPanelTree::GetItem(const wxTreeItemId &Parent, const wxChar *Name, int Image)
{
    wxTreeItemIdValue Cookie;  wxTreeItemId  Item;
    for (Item = GetFirstChild(Parent, Cookie); Item.IsOk() && (GetItemImage(Item) != Image || wxStricmp(GetItemText(Item), Name) != 0); Item = GetNextChild(Parent, Cookie));
    return(Item);
}
//
// Returns item which reprezents schema given by info
//
wxTreeItemId ControlPanelTree::GetCurrApplItem(const item_info &info)
{
    wxTreeItemId Result;
    if (!*info.schema_name)
        return(Result);
    Result = GetItem(rootId, info.cdp!=NULL ? ROOTITM_SERVERS : ROOTITM_ODBC);  // server must be connected when looking for schema
    if (!Result.IsOk())
        return(Result);
    Result = GetItem(Result, info.server_name_str());
    if (!Result.IsOk())
        return(Result);
    return(GetItem(Result, info.schema_name_str()));
}
//
// Returns child of Parent described by info
// Input:   Parent - Parent item
//          info   - Wanted item description
//          Exact  - Exact search flag
// Returns: ID of found item, if item not found and Exact is true, returns invalid item,
//          if Exact is false, returns nearst parent found
//
wxTreeItemId ControlPanelTree::FindItem(wxTreeItemId Parent, const item_info &info, bool Exact)
{
    wxTreeItemId  Item;
    if (!Parent.IsOk())
        Parent = rootId;

    // IF find folder in schema
    if (*info.folder_name != 0)
    {
        wxArrayString as;
        CObjectList ol(info.cdp, info.schema_name);
        // IF folder not found, return invalid item or parent
        if (!ol.Find(info.folder_name, CATEG_FOLDER))
            return(Exact ? Item : Parent);
        // IF folders below categories AND category specified
        if (my_control_panel->ShowFolders && my_control_panel->fld_bel_cat && info.category != NONECATEG)
        {
            wchar_t wc[64];
            // IF category not found, return invalid item or parent
            Item = GetItem(Parent, CCategList::Plural(info.cdp, info.category, wc, true), CategItemDescr[info.category].m_Image);
            if (!Item.IsOk())
                return(Exact ? Item : Parent);
            // Expand category, search from category
            Expand(Item);
        }
        // ELSE categories below folders, search from Parent
        else
        {
            Item = Parent;
        }
        folder_dynar *fd = &info.cdp->odbc.apl_folders;
        t_folder *fld;
        int i;
        // Store parent folder names to stringlist
        for (i = ol.GetFolderIndex(); i != FOLDER_ROOT; i = fld->folder)
        {
            fld = fd->acc0(i);
            as.Insert(wxString(fld->name, *info.cdp->sys_conv), 0);
        }
        // Expand parent folders
        for (i = 0; i < as.Count(); i++)
        {
            wxTreeItemId SubItem = GetItem(Item, as[i], IND_FOLDER);
            if (!SubItem.IsOk())
                return(Exact ? SubItem : Item);
            Item = SubItem;
            Expand(Item);
        }
        // Find folder in parent folder
        wxTreeItemId SubItem = GetItem(Item, info.folder_name_str(), IND_FOLDER);
        if (!SubItem.IsOk())
            return(Exact ? SubItem : Item);
        Item = SubItem;
        // IF categories below folders, find category
        if (!my_control_panel->ShowFolders || !my_control_panel->fld_bel_cat)
        {
            wchar_t wc[64];
            Expand(Item);
            SubItem = GetItem(Item, CCategList::Plural(info.cdp, info.category, wc, true), CategItemDescr[info.category].m_Image);
            if (!SubItem.IsOk())
                return(Exact ? SubItem : Item);
            Item = SubItem;
        }
    }
    // IF find schema on server
    else if (*info.schema_name != 0)
    {
        Item = GetItem(Parent, info.schema_name_str(), IND_APPL);
    }
    // IF find server
    else if (*info.server_name != 0)
    {
        Item = GetItem(Parent, info.cdp!=NULL ? ROOTITM_SERVERS : ROOTITM_ODBC);  // connected state supposed
        Expand(Item);
        Item = GetItem(Item, info.server_name_str());
    }
    return(Item);
}
//
// Returns true if Child is child of Parent
//
bool ControlPanelTree::IsChild(const wxTreeItemId &Child, const wxTreeItemId &Parent)
{
    wxTreeItemId Item = Child;
    while (Item != Parent)
    {
        if (!Item.IsOk())
            return(false);
        Item = GetItemParent(Item);
    }
    return(true);
}
//
// Cut action
//
void ControlPanelTree::OnCut(wxCommandEvent &event)
{
    // IF selected object can be copied, provide cut
    if (m_SelInfo.CanCopy())
    {
        single_selection_iterator si(m_SelInfo);
        CPCopy(si, true);
    }
}
//
// Copy action
//
void ControlPanelTree::OnCopy(wxCommandEvent &event)
{
    // IF selected object can be copied, provide copy
    if (m_SelInfo.CanCopy())
    {
        single_selection_iterator si(m_SelInfo);
        CPCopy(si, false);
    }
}
//
// Paste action
//
void ControlPanelTree::OnPaste(wxCommandEvent &event)
{
    C602SQLDataObject Data;
    // IF content of clipboard can be pasted, provide paste
    if (wxIsDragResultOk(m_SelInfo.CanPaste(Data.GetDataFromClipboard(), false)))
    {
        PasteObjects(m_SelInfo.cdp, m_SelInfo.folder_name);
    }
}
//
// Adds object to tree, returns new item ID
//
wxTreeItemId ControlPanelTree::AddObject(const char *ServerName, const char *SchemaName, const char *FolderName, const char *ObjName, tcateg Categ)
// Does not support adding in the ODBC subtree.
{
    wxTreeItemIdValue Cookie;
    // EXIT IF no server specified OR server not found
    if (!ServerName || !*ServerName)
        return wxTreeItemId();
    if (!m_ol.Find(ServerName, XCATEG_SERVER))
        return wxTreeItemId();
    t_avail_server *Server = (t_avail_server *)m_ol.GetSource();
    // Get server item
    wxTreeItemId Parent = GetItem(rootId, ROOTITM_SERVERS);
    wxTreeItemId Item   = GetItem(Parent, wxString(ServerName, *wxConvCurrent));
    // IF server item not found, insert
    if (!Item.IsOk())
    {
        wxString nm(/*m_ol.GetSmallStringName() -- case in server names should not be changed */ServerName, *wxConvCurrent);
        Item  = InsertItem(Parent, GetNameIndex(Parent, nm), nm, Server->server_image, Server->server_image);
        SetItemBold(Item, !Server->notreg);
        AppendFictiveChild(Item);
        return(Item);
    }
    // IF only fictive child i.e. server item not expanded, nothing to do
    if (HasFictiveChild(Item))
        return wxTreeItemId();
    // EXIT IF user or group requested 
    if (Categ == CATEG_USER || Categ == CATEG_GROUP)
        return wxTreeItemId();
    // IF Schema not specified, set current schema
    if (!SchemaName || !*SchemaName)
        SchemaName = Server->cdp->sel_appl_name;
    wxString nm(SchemaName, *Server->cdp->sys_conv);
    Parent = Item;
    Item   = GetItem(Parent, nm, IND_APPL);
    // IF schema item not found, insert
    if (!Item.IsOk())
    {
        tobjname Schema;
        strcpy(Schema, SchemaName);
        wb_small_string(Server->cdp, Schema, true);
        Item  = InsertItem(Parent, GetNameIndex(Parent, nm), nm, IND_APPL, IND_APPL);
        AppendFictiveChild(Item);
        return(Item);
    }
    // IF only fictive child i.e. schema item not expanded, nothing to do
    if (HasFictiveChild(Item))
        return wxTreeItemId();
    Parent = Item;
    // EXIT IF no folder, root folder OR . specified
    if (!FolderName || !*FolderName || *(WORD *)FolderName == '.')
        return wxTreeItemId();
    m_ol.SetCdp(Server, SchemaName);
    // IF folders below categories, add folder to each category
    if (my_control_panel->fld_bel_cat)
    {
        for (Item = GetFirstChild(Parent, Cookie); Item.IsOk(); Item = GetNextChild(Parent, Cookie))
            AddFolders(Item, SchemaName, FolderName, ObjName, Categ);
        return wxTreeItemId();
    }
    // ELSE Add folder to schema
    else
    {
        return AddFolders(Parent, SchemaName, FolderName, ObjName, Categ);
    }
}
//
// Ensures that FolderName exist, if Categ is CATEG_FOLDER, ensures that folder ObjName exists
//
wxTreeItemId ControlPanelTree::AddFolders(wxTreeItemId &Parent, const char *SchemaName, const char *FolderName, const char *ObjName, tcateg Categ)
{
    wxTreeItemId Item = AddFolder(Parent, SchemaName, "", FolderName);
    if (Item.IsOk() && Categ == CATEG_FOLDER)
        Item = AddFolder(Item, SchemaName, FolderName, ObjName);
    return(Item);
}
//
// If FolderName is not folder of ObjName, inserts parent folder of ObjName, if folder ObjName not exists inserts it
//
wxTreeItemId ControlPanelTree::AddFolder(wxTreeItemId &Parent, const char *SchemaName, const char *FolderName, const char *ObjName)
{
    wxTreeItemId Item;
    if (!m_ol.Find(ObjName, CATEG_FOLDER))
        return(Item);
    // IF folder of ObjName is not FolderName, add folder of ObjName to FolderName
    if (wb_stricmp(m_SelInfo.cdp, FolderName, m_ol.GetFolderName()) != 0)
        Parent = AddFolder(Parent, SchemaName, FolderName, m_ol.GetFolderName());
    // EXIT IF parent not exists
    if (!Parent.IsOk())
        return(Item);
    // EXIT IF parent not expanded
    if (HasFictiveChild(Parent))
        return(Item);
    // IF ObjName found, return ObjName item
    wxString nm(ObjName, *m_SelInfo.cdp->sys_conv);
    Item = GetItem(Parent, nm);
    if (Item.IsOk())
        return(Item);
    // Otherwise insert new folder
    Item  = InsertItem(Parent, GetNameIndex(Parent, nm), nm, IND_FOLDER, IND_FOLDER);
    if (GetFolderState(m_ol.GetCdp(), SchemaName, ObjName, CATEG_FOLDER, true, false) != FST_EMPTY)
        AppendFictiveChild(Item);
    return(Item);
}
//
// Deletes object in tree
//
void ControlPanelTree::DeleteObject(const char *ServerName, const char *SchemaName, const char *ObjName, tcateg Categ)
{
    // EXIT IF no server specified 
    if (!ServerName || !*ServerName)
        return;
    // EXIT IF server item not found OR server item is not expanded
    wxTreeItemId Item = FindServer(wxString(ServerName, *wxConvCurrent));
    if (!Item.IsOk() || HasFictiveChild(Item))
        return;
    // IF schema and object name not specified, delete server item
    if ((!SchemaName || !*SchemaName) && (!ObjName || !*ObjName))
    {
        Delete(Item);
        return;
    }
    // EXIT IF server not exists
    if (!m_ol.Find(ServerName, XCATEG_SERVER))
        return;
    t_avail_server *Server = (t_avail_server *)m_ol.GetSource();
    // IF no schema specified, set current schema
    if (!SchemaName || !*SchemaName)
        SchemaName = Server->cdp->sel_appl_name;
    wxTreeItemId Parent = Item;
    // EXIT IF schema item not found OR schema item is not expanded
    Item   = GetItem(Parent, wxString(SchemaName, *Server->cdp->sys_conv), IND_APPL);
    if (!Item.IsOk() || HasFictiveChild(Item))
        return;
    // IF no object name specified, delete schema item
    if (!ObjName || !*ObjName)
    {
        Delete(Item);
        return;
    }
    // EXIT IF categ is not CATEG_FOLDER
    if (Categ != CATEG_FOLDER)
        return;
    // IF folders below categories, delete folder item from all categories
    if (my_control_panel->fld_bel_cat)
    {
        wxTreeItemIdValue Cookie;
        for (Item = GetFirstChild(Parent, Cookie); Item.IsOk(); Item = GetNextChild(Parent, Cookie))
            DeleteFolder(Item, ObjName);
    }
    // ELSE delete folder item
    else
    {
        DeleteFolder(Item, ObjName);
    }
}
//
// Deletes folder item
//
bool ControlPanelTree::DeleteFolder(wxTreeItemId Parent, const char *FolderName)
{
    wxTreeItemIdValue Cookie;  wxTreeItemId  Item;
    const wxChar *fName = wxString(FolderName, *m_SelInfo.cdp->sys_conv).c_str();
    // FOR each child of Parent
    for (Item = GetFirstChild(Parent, Cookie); Item.IsOk(); Item = GetNextChild(Parent, Cookie))
    {
        // IF FolderName found, delete folder
        if (wxStricmp(GetItemText(Item), fName) == 0)
        {
            Delete(Item);
            return(true);
        }
        // IF Item has valid children, find FolderName between children
        if (!HasFictiveChild(Item))
        {
            if (DeleteFolder(Item, FolderName))
                return(true);
        }
    }
    return(false);
}
//
// Refreshes schema content
//
void ControlPanelTree::RefreshSchema(const char *ServerName, const char *SchemaName)
{
    // EXIT IF no server specified
    if (!ServerName || !*ServerName)
        return;
    // EXIT IF server item not found
    wxTreeItemId Item = FindServer(wxString(ServerName, *wxConvCurrent));
    if (!Item.IsOk())
        return;
    // EXIT IF server item is not expanded
    if (HasFictiveChild(Item))
        return;
    // EXIT IF server not exists
    if (!m_ol.Find(ServerName, XCATEG_SERVER))  // Probably does not support refreshing in the ODBC subtree.###
        return;
    t_avail_server *Server = (t_avail_server *)m_ol.GetSource();
    // IF schema not specified, set current schema
    if (!SchemaName || !*SchemaName)
        SchemaName = Server->cdp->sel_appl_name;
    wxTreeItemId Parent = Item;
    // EXIT IF schema item not found
    Item   = GetItem(Parent, wxString(SchemaName, *Server->cdp->sys_conv), IND_APPL);
    if (!Item.IsOk())
        return;
    // Refresh schema, redraw tree
    RefreshAppl(Server->cdp, Item, SchemaName);
    wxWindow::Refresh(false);
}
//
// Refreshes server content
//
void ControlPanelTree::RefreshServer(const char *ServerName)
{
    // EXIT IF no server specified
    if (!ServerName || !*ServerName)
        return;
    // EXIT IF server item not found
    wxTreeItemId Item = FindServer(wxString(ServerName, *wxConvCurrent));
    if (!Item.IsOk())
        return;
    // EXIT IF server not exists
    if (!m_ol.Find(ServerName, XCATEG_SERVER))  // Does not support refreshing in the ODBC subtree.  ###
        return;
    t_avail_server *Server = (t_avail_server *)m_ol.GetSource();
    // Refresh server
    RefreshServer(Server, Item);
}
//
// Highlights item under mouse cursor during Drag&Drop
//
void ControlPanelTree::HighlightItem(const wxTreeItemId &Item, bool Highlight)
{
    if (Item.IsOk())
    {
        SetItemTextColour(Item,       Highlight ? HighlightTextColour : GetForegroundColour());
        SetItemBackgroundColour(Item, Highlight ? HighlightColour     : GetBackgroundColour());
    }
}
//
// Drop target window enter event handler
//
wxDragResult CCPTreeDropTarget::OnEnter(wxCoord x, wxCoord y, wxDragResult def)
{
    // Reset last item, last highlighted item, last DragResult
    m_Item.Unset();
    m_SelItem.Unset();
    m_Result  = wxDragNone;
    return(OnDragOver(x, y, def));
}

#ifdef LINUX

//
// Returns true if item is in collapsed branch
//
bool ControlPanelTree::IsHidden(const wxTreeItemId &Item) const
{
    wxTreeItemId Par;
    if (!Item.IsOk())
        return(true);

    for (wxTreeItemId Id = Item; ; Id = Par)
    {
        Par = GetItemParent(Id);
        if (!Par.IsOk())
            break;
        if (!IsExpanded(Par))
            return(true);
    }
    return(false);
}
//
// Returns previous visible item of Item 
//
wxTreeItemId ControlPanelTree::GetPrevVisible(const wxTreeItemId &Item) const
{
    wxTreeItemId Result;

    if (!Item.IsOk())
        return(Result);
        
    for (wxTreeItemId id = GetRootItem(); id != Item; id = GetNext(id))
    {
        if (!IsHidden(id))
            Result = id;
    }

    return(Result);
}
//
// Returns next visible item of Item 
//
wxTreeItemId ControlPanelTree::GetNextVisible(const wxTreeItemId &Item) const
{
    if (!Item.IsOk())
        return wxTreeItemId();
    for (wxTreeItemId id = GetNext(Item); id.IsOk(); id = GetNext(id))
    {
        if (!IsHidden(id))
        {
            return(id);
        }
    }
    return wxTreeItemId();
}

#endif
//
// Drop target window drag over event handler
//
wxDragResult CCPTreeDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    wxTreeItemId SelItem;
    wxTreeItemId Item;
    CPHitResult  Hit = Owner()->DragTest(x, y, &Item);
    // IF over item
    if (Hit == CPHR_ONITEM)
    {
        // IF over other item then last time
        if (m_Item != Item)
        {
            // Get item info
            item_info info = Owner()->root_info;
            Owner()->get_item_info(Item, info);
            // IF item can accept dragged objects, set new highlighted item, schema nad folder
            m_Result  = info.CanPaste(CCPDragSource::DraggedObjects(), true);
            if (wxIsDragResultOk(m_Result))
            {
                SelItem = Item;
                m_cdp   = info.cdp;
                strcpy(m_Schema, info.schema_name);
                strcpy(m_Folder, info.folder_name);
            }
            // ELSE reset highlighted item
            else
                SelItem.Unset();
             
            if 
            (
                // IF Item not expanded AND has children AND
                !Owner()->IsExpanded(Item) && Owner()->GetChildrenCount(Item, false) && 
                // item that can be pasted, set timeout for expanding
                ((info.IsServer() && info.cdp) || info.IsSchema() || ((info.IsCategory() || info.IsFolder()) && wxIsDragResultOk(m_Result)))
            )
                m_ExpTime = wxGetLocalTimeMillis() + 1000;
            else
                m_ExpTime = MAX_EXPTIME;
        }
        // IF timeout for expanding ellapsed, expand item
        else if (wxGetLocalTimeMillis() > m_ExpTime)
        {
#ifdef WINS
            CWBDragImage::Image->Hide();
#endif            
            Owner()->m_FromDrag = true;
            Owner()->Expand(Item);
            Owner()->m_FromDrag = false;
            Owner()->Update();
#ifdef WINS
            CWBDragImage::Image->Show();
#endif            
            m_ExpTime = MAX_EXPTIME;
        }
    }
    // IF over top of tree client area
    else if (Hit == CPHR_ABOVE && m_Item.IsOk())
    {
        // IF previous visible item OK, scroll to previous item
        wxTreeItemId PrevItem = Owner()->GetPrevVisible(m_Item);
        if (PrevItem.IsOk())
        {
            Item = PrevItem;
#ifdef WINS
            CWBDragImage::Image->Hide();
#endif            
            Owner()->EnsureVisible(PrevItem);
            Owner()->Update();
#ifdef WINS
            CWBDragImage::Image->Show();
#endif            
        }
    }
    // IF over bootom of tree client area
    else if (Hit == CPHR_BELOW && m_Item.IsOk())
    {
        // IF next visible item OK, scroll to next item
        wxTreeItemId NextItem = Owner()->GetNextVisible(m_Item);
        if (NextItem.IsOk())
        {
            Item = NextItem;
#ifdef WINS
            CWBDragImage::Image->Hide();
#endif            
            Owner()->EnsureVisible(NextItem);
            Owner()->Update();
#ifdef WINS
            CWBDragImage::Image->Show();
#endif            
        }
    }
    // IF current item changed
    if (Item != m_Item)
    {
        m_Item = Item;
        // IF highlighted ietm changed, unhighlight old item, highlight new item
        if (SelItem != m_SelItem)
        {
#ifdef WINS
            CWBDragImage::Image->Hide();
#endif            
            Owner()->HighlightItem(m_SelItem, false);
            Owner()->HighlightItem(SelItem,   true);
            Owner()->Update();
#ifdef WINS
            CWBDragImage::Image->Show();
#endif            
            m_SelItem = SelItem;
        }
    }

    return(GetResult());
}
//
// Drop target window leave event handler
//
void CCPTreeDropTarget::OnLeave()
{
    // Unhighlight last item, redraw list
    Owner()->HighlightItem(m_SelItem, false);
    Owner()->Update();
}
//
// Draged data drop event handler
//
bool CCPTreeDropTarget::OnDrop(wxCoord x, wxCoord y)
{
    // Unhighlight last item, redraw list
    Owner()->HighlightItem(m_SelItem, false);
#ifdef WINS
    CWBDragImage::Image->Hide();
#endif
    Owner()->Update();
    return(wxIsDragResultOk(m_Result));
}

