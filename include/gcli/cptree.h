#ifndef _CPTREE_H_
#define _CPTREE_H_

#include "wx/treectrl.h"
#include "controlpanel.h"
#include "tlview.h"

#define CPTreeName wxT("SQL602ControlPanelTree")

class wxCPSplitterWindow;
class ControlPanelList;
class MyFrame;

DECLARE_LOCAL_EVENT_TYPE(myEVT_INITEDIT,  0)
DECLARE_LOCAL_EVENT_TYPE(myEVT_AFTEREDIT, 1)

////////////////////////// tree view for control panel /////////////////
class ControlPanelTree : public wbTreeCtrl
{
protected:

    wxCPSplitterWindow *my_control_panel;   // Control panel instance
    ControlPanelList   *objlist;            // the list view coupled with this tree
    wxTreeItemId       rootId;              // Root item ID
    item_info          root_info;           // Root item info
    item_info          m_SelInfo;           // Selected item info
    wxTreeItemId       m_SelItem;           // Selected item ID
    CObjectList        m_ol;                // ObjectList 
    wxTimer            m_Timer;             // Timer used for delaying selected item change action
    bool               m_FromDrag;          // Prevents selecting expanded item during Drag&Drop
    bool               m_FromKeyboard;      // true - selected item change invoked by keyboard, false - by mouse
#ifdef LINUX
    bool               m_FromFocus;   
#endif
#ifdef CONTROL_PANEL_INPLECE_EDIT
    wxTreeItemId       m_EditItem;
    tcateg             m_EditCateg;
    bool               m_NewFolder;
    bool               m_EditCancelled;
	bool               m_InEdit;
    CItemEdit          m_ItemEdit;
#endif

    size_t GetNameIndex(const wxTreeItemId &parent, const wxChar *Name);
    void   InsertAvailableServers(const wxTreeItemId &Parent, bool odbc);
    void   InsertAppls(item_info * info, const wxTreeItemId &Parent);
    void   InsertServerConsoleItems(cdp_t cdp, const wxTreeItemId &item);
    void   InsertCategories(item_info & info, const wxTreeItemId &item);
    void   InsertCategorie(cdp_t cdp, const wxTreeItemId &Parent, const char *Schema, const char *Folder, tcateg Categ, bool ShowAllways, bool FoldersBelow);
    void   InsertFolders(item_info & info, const wxTreeItemId &item);
    void   RefreshServers(const wxTreeItemId &Parent);
    void   RefreshServer(t_avail_server * Server, const wxTreeItemId &Parent);
    void   RefreshODBCDSs(const wxTreeItemId &Parent);
    void   RefreshAppl(cdp_t cdp, const wxTreeItemId &Parent, const char *Schema);
    void   RefreshFolder(cdp_t cdp, const wxTreeItemId &Parent, const char *Schema, const char *Folder, tcateg Categ);
    void   RefreshCategories(cdp_t cdp, const wxTreeItemId &Parent, const char *Schema, const char *Folder);
    void   RefreshCategorie(cdp_t cdp, const wxTreeItemId &Parent, const char *Schema, const char *Folder, tcateg Categ, bool ShowAllways, bool FoldersBelow);
    void   RefreshItem(const wxTreeItemId &Item);
    void   RefreshSelItem(){RefreshItem(m_SelItem);}
    void   SelChange();
	void   ShowPopupMenu(const wxTreeItemId &Item, const wxPoint &Position);
    void   Activate();
    void   NewFolder();
    bool   DeleteFolder(wxTreeItemId Parent, const char *FolderName);
    wxTreeItemId GetItem(const wxTreeItemId &Parent, const wxChar *Name);
    wxTreeItemId GetItem(const wxTreeItemId &Parent, const wxChar *Name, int Image);
    wxTreeItemId AddFolders(wxTreeItemId &Parent, const char *SchemaName, const char *FolderName, const char *ObjName, tcateg Categ);
    wxTreeItemId AddFolder(wxTreeItemId &Parent, const char *SchemaName, const char *FolderName, const char *ObjName);

    void OnItemExpanding(wxTreeEvent &event);
    void OnItemActivated(wxTreeEvent & event);
    void OnItemSelected(wxTreeEvent &event);
    void OnRightMouseDown(wxMouseEvent &event);
    void OnKeyDown(wxKeyEvent &event);
    void OnCut(wxCommandEvent &event);
    void OnCopy(wxCommandEvent &event);
    void OnPaste(wxCommandEvent &event);
    void OnBeginDrag(wxTreeEvent &event);
    void OnMouseMove(wxMouseEvent &event);
    void OnSetFocus(wxFocusEvent &event);
    void OnTimer(wxTimerEvent &event);
#ifdef WINS
    void OnRightClick(wxTreeEvent & event);
#else
    void OnRightClick(wxMouseEvent & event);
#endif

#ifdef CONTROL_PANEL_INPLECE_EDIT
    void OnBeginLabelEdit(wxTreeEvent &event);
    void OnEndLabelEdit(wxTreeEvent &event);
    void OnInitEdit(wxCommandEvent &event);
    void OnAfterEdit(wxCommandEvent &event);
    void OnItemSelecting(wxTreeEvent &event);
#else
    void OnRename(wxCommandEvent &event);
#endif

#ifdef LINUX
    void OnLeftMouseDown(wxMouseEvent &event);
    wxTreeItemId GetPrevVisible(const wxTreeItemId &Item) const;
    wxTreeItemId GetNextVisible(const wxTreeItemId &Item) const;
    bool IsHidden(const wxTreeItemId &Item) const;
#endif
    
public:

    ControlPanelTree(wxCPSplitterWindow *splitter) : m_Timer(this)
    { 
        objlist             = NULL; /* not created yet */  
        my_control_panel    = splitter;
        m_FromDrag          = false;
        m_FromKeyboard      = false;
        WindowColour        = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        WindowTextColour    = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        HighlightColour     = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
        HighlightTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
#ifdef LINUX
        m_FromFocus         = false;  
#endif
#ifdef CONTROL_PANEL_INPLECE_EDIT
        m_EditItem          = wxTreeItemId();
        m_NewFolder         = false;
		m_InEdit            = false;
        m_ItemEdit.m_Parent = this;
#endif
    }
    void Refresh(void);
    void insert_top_items(void);
    void get_item_info(wxTreeItemId item, item_info & info);
    void select_child(item_info & info);
    void select_parent(void);
    void expand_selected(void);
    void AppendFictiveChild(const wxTreeItemId &item);
    bool HasFictiveChild(const wxTreeItemId &item);
    wxTreeItemId FindChild(const wxTreeItemId &parent, item_info & info);
    wxTreeItemId FindItem(wxTreeItemId Parent, const item_info &info, bool Exact = false);
    wxTreeItemId FindServer(const wxChar *ServerName);
    wxTreeItemId GetCurrApplItem(const item_info &info);
    wxTreeItemId GetSelItem(){return(m_SelItem);}
    void insert_cp_item(item_info & info, bool select_it);
    void add_running_server(const char * server_name);
    bool IsChild(const wxTreeItemId &Child, const wxTreeItemId &Parent);

    wxTreeItemId AddObject(const char *ServerName, const char *SchemaName, const char *FolderName, const char *ObjName, tcateg Categ);
    void DeleteObject(const char *ServerName, const char *SchemaName, const char *ObjName, tcateg Categ);
    void RefreshSchema(const char *ServerName, const char *SchemaName);
    void RefreshServer(const char *ServerName);
    void HighlightItem(const wxTreeItemId &Item, bool Highlight);
    item_info *GetSelInfo(){return(&m_SelInfo);}

#ifdef LINUX
    virtual void OnInternalIdle();
#endif

    DECLARE_EVENT_TABLE()

    friend wxWindow *create_control_panel(MyFrame * parent);
    friend class wxCPSplitterWindow;
    friend class ControlPanelList;
    friend class CItemEdit;
    friend class MyFrame;
    friend class MyApp;
    friend class CCPTreeDropTarget;
};

struct CCategItemDescr
{
    int         m_Image;
    long        m_ItemData;
};

extern CCategItemDescr CategItemDescr[];

#ifdef WINS
#define MAX_EXPTIME 0x7FFFFFFFFFFFFFFFi64
#else
#define MAX_EXPTIME 0x7FFFFFFFFFFFFFFFll
#endif
//
// wxDropTarget implementation for control panel tree
//
class CCPTreeDropTarget : public CCPDropTarget
{
protected:

    wxTreeItemId m_Item;        // Last item under mouse
    wxTreeItemId m_SelItem;     // Last highlighted item
    wxLongLong   m_ExpTime;     // Timeout for expanding item under mouse

    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
    virtual void OnLeave();
    virtual bool OnDrop(wxCoord x, wxCoord y);

    ControlPanelTree *Owner() const {return((ControlPanelTree *)m_Owner);}
    
public:

    CCPTreeDropTarget(wxWindow *Owner) : CCPDropTarget(Owner){m_ExpTime = (wxLongLong)MAX_EXPTIME;}

};

#endif  // _CPTREE_H_
