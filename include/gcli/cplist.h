#ifndef _CPLIST_H_
#define _CPLIST_H_

#include "wx/listctrl.h"
#include "controlpanel.h"
#include "tlview.h"

////////////////////////// list control for control panel /////////////////
struct t_column_schema  // used in an array terminated by colname==NULL
{ const wxChar * colname;
  int colwidth;
};
//
// Special tree and list objects identificators, used by list sorting
//
#define IDT_FOLDER       0x80000000 
#define IDT_ODBCTABLE    (long)0xFFFF4000
#define IDT_UPFOLDER     (long)0xFFFF2000 
#define IDT_SYSTEMFOLDER (long)0xFFFF2001
#define IDT_TABLE        (long)0xFFFF2001
#define IDT_VIEW         (long)0xFFFF2002
#define IDT_MENU         (long)0xFFFF2003 
#define IDT_CURSOR       (long)0xFFFF2004
#define IDT_PROG         (long)0xFFFF2005
#define IDT_PICT         (long)0xFFFF2006
#define IDT_DRAWING      (long)0xFFFF2007
#define IDT_RELATION     (long)0xFFFF2008
#define IDT_ROLE         (long)0xFFFF2009
#define IDT_CONNECTION   (long)0xFFFF200A
#define IDT_REPLREL      (long)0xFFFF200B
#define IDT_PROC         (long)0xFFFF200C
#define IDT_TRIGGER      (long)0xFFFF200D
#define IDT_XMLFORM      (long)0xFFFF200E
#define IDT_SEQUENCE     (long)0xFFFF200F
#define IDT_DOMAIN       (long)0xFFFF2010
#define IDT_FULLTEXT     (long)0xFFFF2011  
#define IDT_STSH         (long)0xFFFF2012 // update IsCategory when changed!!
// indexing CategFromDataTB[] continues!!
#define IDT_USER         (long)0xFFFF2013 
#define IDT_GROUP        (long)0xFFFF2014
#define IDT_SYSTABLE     (long)0xFFFF2015
#define IDT_REPLSERVER   (long)0xFFFF2016

#define IDT_LICENCES     (long)0xFFFF2001
#define IDT_RUNPROP      (long)0xFFFF2002
#define IDT_PROTOCOLS    (long)0xFFFF2003
#define IDT_SYSOBJS      (long)0xFFFF2004
#define IDT_TOOLS        (long)0xFFFF2005

#define IDT_CLIENTS      (long)0xFFFF2001
#define IDT_LOCKS        (long)0xFFFF2002
#define IDT_TRACE        (long)0xFFFF2003
#define IDT_LOGS         (long)0xFFFF2004
#define IDT_EXTINFO      (long)0xFFFF2005

#define IDT_BACKUP       (long)0xFFFF2001
//#define IDT_REPLAY       (long)0xFFFF2002
#define IDT_MESSAGE      (long)0xFFFF2002
#define IDT_CONSISTENCY  (long)0xFFFF2003
#define IDT_FILENC       (long)0xFFFF2004
#define IDT_SRVCERT      (long)0xFFFF2005
//#define IDT_COMPACT      (long)0xFFFF2004
#define IDT_SERV_RENAME  (long)0xFFFF2006
#define IDT_EXP_USERS    (long)0xFFFF2007
#define IDT_IMP_USERS    (long)0xFFFF2008
#define IDT_PROFILER     (long)0xFFFF2009
#define IDT_EXTENSIONS   (long)0xFFFF200a
#define IDT_HTTP         (long)0xFFFF200b

#define IDT_NOTCONNECTED (long)0xFFFF2002

#define IDT_FONTS        (long)0xFFFF2001
#define IDT_FORMATS      (long)0xFFFF2002
#define IDT_LOCALE       (long)0xFFFF2003
#define IDT_ODBC         (long)0xFFFF2004
#define IDT_MAIL         (long)0xFFFF2005
#define IDT_FTXHELPERS   (long)0xFFFF2006
#define IDT_COMM         (long)0xFFFF2007
#define IDT_EDITORS      (long)0xFFFF2008

#define LII_APPEND       -1
#define LII_DOSORT       -2
//
// List sorting order
//
enum CPListOrder 
{
    CPLO_DEFAULT,           // By name ascending
    CPLO_NAMEDESC,          // By name descending
    CPLO_OBJMODIFDESC,      // By modify timestamp descending
    CPLO_OBJMODIFASC,       // By modify timestamp asccending
    CPLO_SRVTYPE,           // By server type
    CPLO_SRVLOGAS,          // By logged user
    CPLO_SRVADDR,           // By server address
    CPLO_SRVINSTKEY         // By instalation key - not used
};

#define CPListName wxT("SQL602ControlPanelList")
//
// 
//
class ControlPanelList : public wbListCtrl
{
protected:

    ControlPanelTree   *tree_view;          // Brother tree
    t_column_schema    *column_schema;      // Current column schema - not owning
    wxCPSplitterWindow *my_control_panel;   // Control panel instance
    CPListOrder         m_Order;            // Current list sorting order
    CObjectList         m_ObjectList;       // CObjectList instance for objects
    CObjectList         m_FolderList;       // CObjectList instance for folders
    wxTimer             m_Timer;            // Timer used for delaying selected item change action
    wxArrayString       m_SpecList;         // List of objects, which have not unique ID (servers, mail profiles), used as translation table
                                            // from object ID, to object name, during sorting
#ifdef CONTROL_PANEL_INPLECE_EDIT
    long                m_EditItem;     
    tcateg              m_EditCateg;
    tobjname            m_OldName;
    CItemEdit           m_ItemEdit;
    bool                m_NewFolder;
    bool                m_F2Edit;
	bool                m_InEdit;
    bool                m_EditCancelled;
#ifdef LINUX
    bool                m_EditSame;
    bool                m_EditWait;
    wxLongLong          m_EditTimeout;
#endif    
#endif

    long                m_SelIndex;         // Last time selected object index
    bool                m_NoRelist;         // Blocks refilling list during filling tree
    bool                m_InRefresh;        // Blocks current item change during list refresh
    bool                m_InSelChange;      // Indicates delayed current item change 
    bool                m_FromKeyboard;     // true - selected item change invoked by keyboard, false - by mouse
 
    long Insert(const wxChar *Text, int ImageIndex, long Data, int Index = LII_DOSORT);
    void InsertCategorie(tcateg Categ);
    long InsertObject(const wxChar *Name, tcateg Categ, uns16 flags, tobjnum Objnum, uns32 modif_timestamp);
    int  CompareByName(long Item1, long Item2, tcateg Categ);
    int  Compare(long Item1, long Item2);
    static int wxCALLBACK Compare(long Item1, long Item2, HANDLE SortData){return(((ControlPanelList *)SortData)->Compare(Item1, Item2));}
    CPListOrder GetOrder(CPListOrder *Orders, int Count, int Col){return(Col >= Count ? CPLO_DEFAULT : Orders[Col]);}
    tcateg CategFromData(long Data);
    void NewFolder();
    void RefreshCategorie(tcateg Categ);
    void HighlightItem(long Item, bool Highlight);

#ifdef CONTROL_PANEL_INPLECE_EDIT
    void OnBeginLabelEdit(wxListEvent & event);
    void OnEndLabelEdit(wxListEvent & event);
    void OnInitEdit(wxCommandEvent &event);
    void OnAfterEdit(wxCommandEvent &event);
#else
    void OnRename(wxCommandEvent& event);
#endif
    void OnItemSelected(wxListEvent & event);
    void OnItemActivated(wxListEvent & event);
    void OnDelete(wxCommandEvent & event);
    void OnCPCommand( wxCommandEvent& event );
    void OnCut(wxCommandEvent &event);
    void OnCopy(wxCommandEvent &event);
    void OnPaste(wxCommandEvent &event);
    void OnDuplicate(wxCommandEvent &event);
    void OnBeginDrag(wxListEvent &event);
    void OnSort(wxListEvent & event);
    void OnRefresh(wxCommandEvent &event);
    void OnSetFocus(wxFocusEvent &event);
    void OnRightMouseDown(wxMouseEvent & event);
#ifdef WINS
    void OnRightClick(wxCommandEvent & event);
#else
    void OnRightClick(wxMouseEvent & event);
    void OnLeftMouseDown(wxMouseEvent & event);

    void SetFocus();
#endif
    void OnKeyDown(wxKeyEvent & event);
    void OnColResize(wxListEvent & event);
    void OnTimer(wxTimerEvent &event);
	void ShowPopupMenu(const wxPoint &Position);

public:

    item_info             *m_BranchInfo;    // Info of item currently selected in tree

    void set_column_schema(t_column_schema * new_column_schema);
  
    ControlPanelList(wxCPSplitterWindow *splitter, ControlPanelTree *Tree) : m_Timer(this)
    {
        tree_view           = Tree;
        m_BranchInfo        = &Tree->m_SelInfo;
        column_schema       = NULL;
        my_control_panel    = splitter;
        m_NoRelist          = false;
        m_InRefresh         = false;
        m_InSelChange       = false;
        m_FromKeyboard      = false;
        m_Order             = CPLO_DEFAULT;

#ifdef CONTROL_PANEL_INPLECE_EDIT
        m_EditItem          = -1;
        *m_OldName          = 0;
        m_NewFolder         = false;
        m_F2Edit            = false;
		m_InEdit            = false;
        m_ItemEdit.m_Parent = this;

#ifdef LINUX
        m_EditSame          = false;
        m_EditWait          = true;
#endif        
#endif
    }
    void Fill();
    void Refresh();
    void add_list_item_info(long item, item_info & info);
    void SelectItem(int ind);
    void update_cp_item(item_info & info, uns32 modtime);
    //void select_item_by_name(cdp_t cdp, const char * name);
    //void select_by_name(cdp_t cdp, const char * name);
    //void select_cp_item(item_info & info);
    void update_server_data(int ind, const char * server_name, const t_avail_server * as);

    bool IsSpecObj(long Data) const {return(Data < IDT_ODBCTABLE);}
    bool IsFolder(long Data) const {return(Data >= IDT_FOLDER && Data < IDT_UPFOLDER);}
    bool IsSystemFolder(long Data) const {return(Data == IDT_SYSTEMFOLDER);}
    bool IsUpFolder(long Data) const {return(Data == IDT_UPFOLDER);}
    bool IsCategory(long Data) const {return(Data >= IDT_TABLE && Data <= IDT_STSH);}
    int  FolderIndexFromData(long Data) const {return(Data & ~IDT_FOLDER);}

    long AddObject(const char *ServerName, const char *SchemaName, const char *FolderName, const char *ObjName, tcateg Categ, uns16 Flags, uns32 Modif);
    void DeleteObject(const char *ServerName, const char *SchemaName, const char *ObjName, tcateg Categ);
    void DeselectAll();
    void RefreshSchema(const char *ServerName, const char *SchemaName);
    void RefreshServer(const char *ServerName);
    static void LoadLayout();
    static void SaveLayout();
    void Reset()
    {
        m_ObjectList.SetCdp((cdp_t)NULL);
        m_FolderList.SetCdp((cdp_t)NULL);
    }
    virtual void OnInternalIdle();

    friend class list_selection_iterator;
    friend class CCPListDropTarget;
    friend class ControlPanelTree;
    friend class CItemEdit;
    friend class MyFrame;

    DECLARE_EVENT_TABLE()
};
//
// wxDropTarget implementation for control panel list
//
class CCPListDropTarget : public CCPDropTarget
{
protected:

    long m_Item;        // Last item under mouse
    long m_SelItem;     // Last highlighted item

    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
    virtual void OnLeave();
    virtual bool OnDrop(wxCoord x, wxCoord y);

    ControlPanelList *Owner() const {return((ControlPanelList *)m_Owner);}

    CPHitResult HitTest(wxCoord x, wxCoord y, long *pItem);

public:

    CCPListDropTarget(wxWindow *Owner) : CCPDropTarget(Owner){}

};

#endif  // _CPLIST_H_
