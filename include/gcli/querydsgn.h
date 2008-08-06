#ifndef _QUERYDSGN_H_
#define _QUERYDSGN_H_
//
// Declarations for query designer
//
#include "querydef.h"
#include "tlview.h"
#include "framedchild.h"
#include "advtooltip.h"
//
// Table/cursor definition encapsulation
//
class CTableDef
{
protected:

    const d_table *m_Def; 

public:

    CTableDef()
    {
        m_Def  = NULL;
    }
    ~CTableDef()
    {
        if (m_Def)
            release_table_d(m_Def);
    }
    bool Create(cdp_t cdp, const wxChar *wxSource, tcateg Categ, struct t_clivar * hostvars, unsigned hostvars_count);
    const d_attr *GetAttr(int Index){return Index < m_Def->attrcnt ? &m_Def->attribs[Index] : NULL;}

    friend class CQDTableBox;
};
//
// Listbox implementation for TableBox
//
class CQDListBox : public wbListCtrl
{
protected:

    void OnScroll(wxScrollWinEvent &event);
    void OnLeftClick(wxMouseEvent &event);
    void OnSize(wxSizeEvent &event);
    void OnItemSelected(wxListEvent &event);
    void OnKillFocus(wxFocusEvent &event);
    void OnKeyDown(wxKeyEvent &event);
    void OnMouseMove(wxMouseEvent &event);
#ifdef LINUX
    void OnRightClick(wxMouseEvent & event);
#else
    void OnRightClick(wxCommandEvent &event);
#endif

    void RightClick(int x, int y);

    DECLARE_EVENT_TABLE()

public:

    void HighlightItem(long Item, bool Highlight)
    {
        if (Item != -1)
            SetItemState(Item, Highlight ? wxLIST_STATE_DROPHILITED : 0, wxLIST_STATE_DROPHILITED);
    }
    friend class CQDTableBox;
};

class CQDQTable;
class CTablesPanel;
//
// TableBox - Framed listbox with list of table columns
//
class CQDTableBox : public wbFramedChild 
{
protected:
    
    CQDListBox    m_List;           // Listbox instance
    wxTreeItemId  m_Node;           // Corresponding tree item
    CTableDef     m_TableDef;       // Source table definition
    bool          m_HiddenAttrs;    // Show hidden columns (_W5_...)
    wxString      m_Source;         // Source table name / source query text
    tcateg        m_Categ;          // Source category
    wxCSConv     *m_Conv;           // Conversoin to server codepage
#ifdef LINUX
    long          m_LastTop;        // On Linuxu doesn't wxListView become wxScrollEvents, so I must
                                    // provide join link redrawing by myself in CTablesPanel::OnInternalIdle
#endif
    bool          m_Deleting;       // CQDTableBox becomes wx events even during destructor execution
                                    // m_Deleting locks event handling
    void OnSize(wxSizeEvent &event);
    void OnMove(wxMoveEvent &event);
    void OnSetFocus(wxFocusEvent &event);
    void OnHiddenAttrs(wxCommandEvent &event);
    void OnRefresh(wxCommandEvent &event);
    void OnRightClick(wxMouseEvent &event);

public:

    CQDTableBox(wxTreeItemId Node, const wxString &Source, tcateg Categ)
    {
        m_Node        = Node;
        m_HiddenAttrs = false;
        m_Deleting    = false;
        m_Source      = Source;
        m_Categ       = Categ;
    }
    virtual ~CQDTableBox();
    bool Create(CTablesPanel *Parent);
    void SetList(cdp_t cdp, struct t_clivar * hostvars, unsigned hostvars_count);
    void Relist(cdp_t cdp);
    //void Add(const char *Attr, int Type){m_List.Insert(Attr, m_List.GetCount(), (void *)Type);}
    void Add(const char *Attr, int Type)
    {
        long i = m_List.InsertItem(m_List.GetItemCount(), wxString(Attr, *m_Conv).c_str()); 
        m_List.SetItemData(i, (long)Type);
    }
    int  IndexOf(const wxChar *Attr);
    int  TopIndex(){return m_List.GetTopItem();}
    int  ItemHeight(){wxRect Rect; m_List.GetItemRect(0, Rect); return Rect.GetHeight();}
    //bool IsRightDown(){return GetKeyState(VK_RBUTTON) < 0;}
    //bool IsLeftDown(){return GetKeyState(VK_LBUTTON) < 0;}

    DECLARE_EVENT_TABLE()

    friend class CQueryDesigner;
    friend class CTablesPanel;
    friend class CQDQTable;
    friend class CQDListBox;
};

///////////////////////////////////// Design elements records ///////////////////////////////////////////

//
// Common source table - Database table, query or join ancestor
//
class CQDQTable : public wxTreeItemData
{
protected:

    static int TableID;             // Unique table ID source

    TTableType     m_Type;          // Source table type
    wxString       m_Alias;         // Alias 
    int            m_QueryNO;       // Unnamed query/join number
    int            m_TableID;       // Unique table ID
    CQDTableBox   *m_TableBox;      // Corresponding TableBox instance

public:
    
    CQDQTable()
    {
        m_QueryNO  = 0;
        m_TableID  = ++TableID;
        m_TableBox = NULL;
    }
    CQDQTable(CQTable *Table, wxCSConv *conv)
    {
        m_Alias        = wxString(Table->Alias, *conv);
        m_Type         = Table->TableType;
        m_QueryNO      = 0;
        m_TableID      = ++TableID;
        m_TableBox     = NULL;
        Table->TableID = m_TableID;
    }
    CQDQTable(const CQDQTable *Table)
    {
        m_Type     = Table->m_Type;
        m_Alias    = Table->m_Alias;
        m_QueryNO  = Table->m_QueryNO;
        m_TableBox = Table->m_TableBox;
        m_TableID  = Table->m_TableID;
    }
    virtual ~CQDQTable()
    {
    }

    friend class CQDDropTargetTB;
    friend class CQueryDesigner;
    friend class CQDFromGrid;
    friend class CQDGrid;
    friend class CQDTableBox;
    friend class CQDListBox;
};
//
// Database table record
//
class CQDDBTable : public CQDQTable
{
protected:

    wxString m_Schema;                // Schema name
    wxString m_Name;                  // Table name
    wxString m_Index;                 // Prefered index specification (not supported)

public:

    CQDDBTable(){m_Type = TT_TABLE;}
    CQDDBTable(CQDBTable *Table, wxCSConv *conv) : CQDQTable(Table, conv)
    {
        m_Schema = GetNakedIdent(wxString(Table->Schema, *conv));
        m_Name   = GetNakedIdent(wxString(Table->Name,   *conv));
        m_Index  = GetNakedIdent(wxString(Table->Index,  *conv));
    }
    CQDDBTable(const CQDDBTable *Table) : CQDQTable(Table)
    {
        m_Schema = Table->m_Schema;
        m_Name   = Table->m_Name;
        m_Index  = Table->m_Index;
    }
    virtual ~CQDDBTable(){}

    const wxChar *GetAliasName(){return m_Alias.IsEmpty() ? m_Name : m_Alias;}

    friend class CQueryDesigner;
    friend class CQDFromGrid;
    friend class CQDGrid;
    friend class CQDTableBox;
    friend class CQDListBox;
};
//
// Join record
//
class CQDJoin : public CQDQTable
{
protected:

    TJoinType m_JoinType;               // Join type and flags
    wxString  m_Cond;                   // Join condition
    int       m_DstTableID;             // Destination table ID
    wxString  m_DstAttr;                // Destination column name
    int       m_SrcTableID;             // Source table ID
    wxString  m_SrcAttr;                // Source column name

public:

    CQDJoin()
    {
        m_Type     = TT_JOIN;
        m_JoinType = 0;
    }
    CQDJoin(CQJoin *Join, wxCSConv *conv) : CQDQTable(Join, conv)
    {
        m_Cond     = wxString(Join->Cond, *conv);
        m_JoinType = Join->JoinType;
    }
    CQDJoin(const CQDJoin *Join) : CQDQTable(Join)
    {
        m_JoinType   = Join->m_JoinType;
        m_Cond       = Join->m_Cond;
        m_DstAttr    = Join->m_DstAttr;
        m_SrcAttr    = Join->m_SrcAttr;
        m_DstTableID = Join->m_DstTableID;
        m_SrcTableID = Join->m_SrcTableID;
    }
    virtual ~CQDJoin(){}

    friend class CQueryDesigner;
    friend class CQDJoinGrid;
    friend class CQDGrid;
};
//
// ORDER BY expression record
//
class CQDOrder : public wxTreeItemData
{
protected:

    wxString m_Expr;        // Expression test
    bool     m_Desc;        // DESC flag

public:

    CQDOrder(){m_Desc = false;}
    CQDOrder(const CQOrder *Order, wxCSConv *conv)
    {
        m_Expr = wxString(Order->Expr, *conv);
        m_Desc = Order->Desc;
    }
    CQDOrder(const CQDOrder *Order)
    {
        m_Expr = Order->m_Expr;
        m_Desc = Order->m_Desc;
    }
    virtual ~CQDOrder(){}

    friend class CQueryDesigner;
    friend class CQDOrderGrid;
};
//
// LIMIT expression destriptor
//
class CQDLimit : public wxTreeItemData
{
protected:

    wxString m_Offset;      // Offset value
    wxString m_Count;       // Count value

public:

    CQDLimit(){}
    CQDLimit(const CQUQuery *Query, wxCSConv *conv)
    {
        m_Offset = wxString(Query->LimitOffset, *conv);
        m_Count  = wxString(Query->LimitCount, *conv);
    }
    CQDLimit(const CQDLimit *Limit)
    {
        m_Offset = Limit->m_Offset;
        m_Count  = Limit->m_Count;
    }
    friend class CQDGrid;
    friend class CQDLimitGrid;
    friend class CQueryDesigner;
};
//
// Common query and query expression ancestor
//
class CQDUQuery : public CQDQTable
{
protected:

    TQueryAssoc m_Assoc;        // Association to next query in expression
    bool        m_All;          // ALL flag
    bool        m_Corresp;      // CORRESPONDING flag
    wxString    m_By;           // CORRESPONDING BY expression
    wxString    m_Source;       // Query source text
    wxString    m_Prefix;       // Text before SELECT in column or condition definition

public:

    CQDUQuery()
    {
        m_Assoc   = QA_NONE;
        m_All     = false;
        m_Corresp = false;
    }
    CQDUQuery(CQUQuery *Query, wxCSConv *conv) : CQDQTable(Query, conv) 
    {
        m_By      = wxString(Query->By, *conv);
        m_Prefix  = wxString(Query->Prefix, *conv);
        m_Assoc   = Query->Assoc;
        m_All     = Query->All;
        m_Corresp = Query->Corresp;
    }
    CQDUQuery(const CQDUQuery *Query) : CQDQTable(Query) 
    {
        m_Assoc   = Query->m_Assoc;
        m_All     = Query->m_All;
        m_Corresp = Query->m_Corresp;
        m_By      = Query->m_By;
        m_Prefix  = Query->m_Prefix;
    }
    virtual ~CQDUQuery(){}

    friend class CQueryDesigner;
    friend class CQDQueryGrid;
    friend class CQDDropTargetTI;
    friend class CQDGrid;
};
//
// Column or condition definition common ancestor
//
class CQDQueryTerm : public wxTreeItemData
{
protected:

    wxString m_Expr;        // Column or condition definition text
    wxString m_Postfix;     // Text after last SELECT statement in definition text
    bool     m_Error;       // Syntax error in definition

public:

    CQDQueryTerm()
    {
        m_Error   = false;
    }
    CQDQueryTerm(const CQQueryTerm *Term, wxCSConv *conv)
    {
        m_Expr    = wxString(Term->Expr, *conv);
        m_Postfix = wxString(Term->Postfix, *conv);
        m_Error   = false;
    }
    CQDQueryTerm(const CQDQueryTerm *Term)
    {
        m_Expr    = Term->m_Expr;
        m_Postfix = Term->m_Postfix;
        m_Error   = false;
    }
    virtual ~CQDQueryTerm(){}

    friend class CQueryDesigner;
    friend class CQDSelectGrid;
};
//
// Column definition record
//
class CQDColumn : public CQDQueryTerm
{
protected:

    wxString m_Alias;       // Column Alias
    wxString m_Into;        // INTO varible naem

public:

    CQDColumn(){}
    CQDColumn(const CQColumn *Column, wxCSConv *conv) : CQDQueryTerm(Column, conv)
    {
        m_Alias = wxString(Column->Alias, *conv);
        m_Into  = wxString(Column->Into, *conv);
    }
    CQDColumn(const CQDColumn *Column) : CQDQueryTerm(Column)
    {
        m_Alias = Column->m_Alias;
        m_Into  = Column->m_Into;
    }
    virtual ~CQDColumn(){}

    friend class CQueryDesigner;
    friend class CQDSelectGrid;
    friend class CQDGrid;
};
//
// WHERE/HAVING condition record
//
class CQDCond : public CQDQueryTerm
{
protected:
    
    TAndOr   m_Assoc;       // Association to next subcondition
    bool     m_SubConds;    // Contains subconditions

public:

    CQDCond(bool SubConds)
    {
        m_Assoc    = CA_NONE;
        m_SubConds = SubConds;
    }
    CQDCond(const CQCond *Cond, wxCSConv *conv)  : CQDQueryTerm(Cond, conv)
    {
        m_Assoc    = Cond->Assoc;
        m_SubConds = Cond->SubConds.GetCount() != 0;
    }
    CQDCond(const CQDCond *Cond) : CQDQueryTerm(Cond)
    {
        m_Assoc    = Cond->m_Assoc;
        m_SubConds = Cond->m_SubConds;
    }
    virtual ~CQDCond(){}

    friend class CQueryDesigner;
    friend class CQDGrid;
    friend class CQDCondGrid;
    friend class CQDDropTargetTI;
};
//
// GROUP BY clause record
//
class CQDGroup : public wxTreeItemData
{
protected:

    wxString m_Expr;    // GROUP BY expression

public:
    
    CQDGroup(){}
    CQDGroup(const CQGroup *Group, wxCSConv *conv){m_Expr = wxString(Group->Expr, *conv);}
    CQDGroup(const CQDGroup *Group){m_Expr = Group->m_Expr;}
    virtual ~CQDGroup(){}

    friend class CQueryDesigner;
    friend class CQDGroupGrid;
};

class CQDQueryExpr;
//
// Query record
//
class CQDQuery : public CQDUQuery
{
protected:

    bool         m_Distinct;        // DISTINCT flag
    int          m_JoinNO;          // Unique number source for unnamed join name
    int          m_CondNO;          // Unique condition number bource

    CQDQTable  *BuildJoin(CQDQTable *Table);
    CQDJoin    *BuildJoin(CQDJoin *Root, CQDQTable *Table, CQDQTable *Parent);
    CQDJoin    *FindRootJoin(CQDJoin *Join);
    bool        FindJoin(CQDJoin *ScanJoin, CQDJoin *Join);
    CQDDBTable *FindTable(const char *Name, const char *Alias, CQDQTable *Table);
    void        SetFrom(int Index, CQDQTable *Table);

public:

    CQDQuery()
    {
        m_Type     = TT_QUERY;
        m_Distinct = false;
        m_JoinNO   = 0;
        m_CondNO   = 0;
    }
    CQDQuery(CQQuery *Query, wxCSConv *conv) : CQDUQuery(Query, conv)
    {
        m_Distinct = Query->Distinct;
        m_JoinNO   = 0;
        m_CondNO   = 0;
    }
    CQDQuery(const CQDQuery *Query) : CQDUQuery(Query)
    {
        m_Distinct = Query->m_Distinct;
        m_JoinNO   = Query->m_JoinNO;
        m_CondNO   = Query->m_CondNO;
    }
    virtual ~CQDQuery(){}

    friend class CQueryDesigner;
    friend class CQDSelectGrid;
};
//
// Query expression record
//
class CQDQueryExpr : public CQDUQuery
{
protected:

    int      m_CursorFlags;     // Cursor flags
    wxString m_ViewName;        // VIEW name

public:

    CQDQueryExpr(){m_Type = TT_QUERYEXPR; m_CursorFlags = 0;}
    CQDQueryExpr(CQueryExpr *Query, wxCSConv *conv) : CQDUQuery(Query, conv)
    {
        m_CursorFlags = Query->CursorFlags;
        if (Query->ViewName)
            m_ViewName = wxString(Query->ViewName, *conv);
    }
    CQDQueryExpr(const CQDQueryExpr *Query) : CQDUQuery(Query){}
    
    virtual ~CQDQueryExpr(){}

    friend class CQueryDesigner;
};
//
// Query designer child windows IDs
//
#define QDID_RIGHTPANEL 1
#define QDID_TREE       2
#define QDID_PROPS      3
#define QDID_GRID       4
#define QDID_TABLES     5
#define QDID_DISTINCT   1024
#define QDID_TABLELIST  1025
//
// Query elemets image number and type ID
//
typedef enum {qtiQueryExpr, qtiQuery, qtiSelect, qtiFrom, qtiWhere, qtiGroupBy, qtiHaving,
              qtiOrderBy, qtiTable, qtiJoin, qtiColumn, qtiCond, qtiSubConds, qtiGColumn, qtiOColumn, qtiLimit} TQTreeImg;
#define qtiLast qtiLimit
//
// Parent of query properties grids
//
class CPropsPanel : public wxPanel
{
protected:

    void OnSize(wxSizeEvent & event);

public:

    void Resize();

    DECLARE_EVENT_TABLE()
};
//
// Join link node record
//
struct CLinkedAttr
{
    CQDTableBox *m_TableBox;    // TableBox instance
    int          m_AttrNO;      // Column number
};
//
// Join link record
//
struct CAttrLink
{
    CLinkedAttr  m_DstAttr;     // Destination table and column
    CLinkedAttr  m_SrcAttr;     // Source table and column
    wxTreeItemId m_Node;        // Corresponding design tree node (join or condition)
};
//
// Join links list
//
SPECIF_DYNAR(CAttrLinks, CAttrLink, 1);
//
// Join link cycle test node
//
struct CCycleLink
{
    int  m_SrcTableID;          // Source table ID
    int  m_DstTableID;          // Destination table ID
    bool m_Visited;             // This node was passed during cycle test
};
//
// Join link cycle test node list
//
SPECIF_DYNAR(CCycleLinks, CCycleLink, 8);

class CQueryDesigner;
//
// Tables panel - parent for TableBoxes
//
class CTablesPanel : public wxScrolledWindow
{
protected:

    CQueryDesigner *m_Designer;         // Parent query designer
    CAttrLinks      m_AttrLinks;        // Join links list
    CAttrLink      *m_SelAttrLink;      // Selected join link
    CQDTableBox    *m_SelTable;         // Selected table
    wxPen           m_SelPen;           // Selected join link pen
    int             m_TBClOffset;       // Y offset from TableBox frame to listbox top - Height of TableBox title
    bool            m_InScroll;         // Blocks virtual resizing duering scroll
    bool            m_HasFocus;         // In wx panel cannot have focus, focus is propmptly transfered to first child,
                                        // children of TablesPanel are TableBoxes, m_HasFocus is set by mouse click
                                        // and idicates who is acceptor of Delete action
                                        // true  - TablesPanel was last time clicked Delete will delete selected link
                                        // false - TableBox was last time clicked Delete will delete selected table
    void OnPaint(wxPaintEvent &event);
    void OnLeftMouseDown(wxMouseEvent &event);
    void OnRightMouseDown(wxMouseEvent &event);
#ifdef LINUX    
    void OnKeyDown(wxKeyEvent &event);
#endif    

    void Resize();

    wxPoint    GetPoint(CAttrLink *AttrLink, bool Src);
    CAttrLink *AttrLinkFromPoint(int x, int y);
    CAttrLink *SelectAttrLink(int x, int y);
    void       SelectAttrLink(const wxTreeItemId &Item);
    int        GetTBClOffset();

public:

    CTablesPanel(CQueryDesigner *Designer) : m_SelPen(*wxBLACK, 3)
    {
        m_Designer    = Designer;
        m_SelAttrLink = NULL;
        m_TBClOffset  = 0;
        m_SelTable    = NULL;
        m_InScroll    = false;
        m_HasFocus    = false;
    }

    void Clear()
    {
        DestroyChildren();
        m_SelTable = NULL;
    }
    void Select(CQDTableBox *Table)
    {
        m_SelTable = Table;
        if (Table)
        {
            m_SelAttrLink = NULL;
            m_HasFocus    = false;
        }
        Refresh();
    }
    void DeleteTable(CQDTableBox *Table);
    virtual void ScrollWindow( int dx, int dy, const wxRect* rect = (wxRect *) NULL );
    CQDTableBox *GetSelTable(){return m_SelTable;}
    CQDTableBox *FindTableBox(const wxTreeItemId &Node);
    CAttrLink   *FindAttrLink(const wxTreeItemId &Node);
#ifdef LINUX
    void OnInternalIdle();
#endif    

    DECLARE_EVENT_TABLE()

    friend class CQueryDesigner;
    friend class CQDTableBox;
    friend class CQDDropTargetTB;
    friend class CQDFromGrid;
    friend class CQDJoinGrid;
    friend class CQDCondGrid;
    friend class CQDListBox;
};
//
// SQL statement generation style
//
enum TStatmStyle
{
    ssWrapNever,                        // Never wrap - single line statement
    ssWrapAlways,                       // Always wrap subqueries
    ssWrapLong                          // Wrap only long subqueries
};

#define SINGLELINE_LEN_LIMIT 100        // Default line length limit
#define SINGLELINE_LEN       0x7FFFFFFF // No line length limit
#define DEF_INDENT           4          // Default indent

#define CATEG_SYSQUERY 100              // System query category
#define QDTREE_APPEND  ((size_t)-1)     // Append to end of list by Drag&Drop in tree
//
// Flags for GetTableName
//
#define GTN_PROPER 1                    // Cover name in `` if necessary
#define GTN_ALIAS  2                    // If Alias available, use Alias
#define GTN_TITLE  4                    // For caption in tree item or TableBox title, without ``
#define GTN_UNIQUE 8                    // If name is not unique in current query, create and return unique alias
//
// SQL statement geheration primitive defintions
//
typedef void (CQueryDesigner :: *LPPUTPROC)();                      // Puts separator to statement
typedef void (CQueryDesigner :: *LPPUTSEPPROC)(int Count);          // Puts clause separator to statement
typedef void (CQueryDesigner :: *LPPUTSTRPROC)(const wxChar *Val);  // Puts Val to statement
//
// SQL statement generation context
//
struct CPutCtx
{
    LPPUTSTRPROC Item;                  // Puts text to statement
    LPPUTSTRPROC NewLineItem;           // Puts text to statement,on new line if necessary
    LPPUTSEPPROC ClauseSep;             // Puts clause separator to statement
    LPPUTPROC    Separator;             // Puts simple separator to statement
    bool         MultiLine;             // Multiline/Singleline statement
};

class CElementPopup;
class CQDGrid;
//
// Query designer window
//
class CQueryDesigner : public wxSplitterWindow, public any_designer
{ 
protected:

    wxSplitterWindow m_RightPanel;      // Parent of m_Props and m_Tables
    wbTreeCtrl       m_Tree;            // Design tree
    CPropsPanel      m_Props;           // Parent of preoperty grids
    CTablesPanel     m_Tables;          // Tables panel
    int              m_TablesHeight;    // Height of tables panel
    CQDGrid         *m_Grid;            // Current property grid
    TStatmStyle      m_StatmStyle;      // SQL statement generation style
    bool             m_LineTooLong;     // Flag for line to long exception
    bool             m_WrapItems;       // Wrap before each item
    bool             m_WrapAfterClause; // Wrap after clause
    bool             m_ToText;          // true  - statement building for designer output, syntax error is not fatal
                                        // false - statement building for syntax check, preview or optimization, syntax error is fatal, action cannot continue 
    bool             m_JoinConv;        // true  - Drop of new join link creates new WHERE condition, joins and proper WHERE conditions are displayed as table links
                                        // false - Drop of new join link creates new join, joins only are displayed as table links
    bool             m_PrefixColumns;   // Prefix column names with table names
    bool             m_PrefixTables;    // Prefix table names with schema names
    bool             m_ShowIndex;       // Show index column in WHERE grid (unused INDEX clause is not supported)
    bool             m_InReset;         // Blocks item deleting cycle by designer reset
    bool             m_Resize;          // Blocks resize cycle
    bool             m_FromPopup;       // Indicates who is object of syntax check and preview action, if from popup menu
                                        // current subquery selected in tree, if from main menu, whole design 
    bool             m_CurQueryChanged; // Current subquery changed, parent column or condition should be rebuilded
    int              m_LinePos;         // Current position in generated SQL statement
    int              m_LineLen;         // Current line length limit (SINGLELINE_LEN for single line statement, m_LineLenLimit for multiline)
    int              m_LineLenLimit;    // Line length limit (set by properties dialog)
    int              m_Indent;          // SQL statement indent increment
    int              m_ResultLevel;     // Current block indent
    int              m_ShowErr;         // Error code of compilation provided during designer initialization when designer window doesn't exist
    wxString         m_Result;          // Resulting SQL statement
    wxString         m_Schema;          // Current schema
    wxImageList      m_ImageList;       // Image list for tree items
    CElementPopup   *m_ElementPopup;    // Instance of popup window with statement elemets
    wxTreeItemId     m_CurQuery;        // Tree item representing selected subquery
    wxTreeItemId     m_DeletedItem;     // Tree item reprezenting delayed deleted element
    int              m_QueryXNO;        // Unique number source for unnamed query expression name
    int              m_QueryNO;         // Unique number source for unnamed query name
    int              m_SplitVert;       // Vertical splitter position
    CPutCtx          Put;               // SQL statement generation context
    
    text_object_editor_type m_Edit;     // Text editor
    t_dad_param_dynar       m_Params;   // List of statement parameters

    void LoadImageList();
    
    void LoadQueryExpr(const wxTreeItemId *Parent, CQueryExpr *QueryExpr);
    void LoadQuery(const wxTreeItemId &Parent, CQQuery *Query);
    void LoadColumn(const wxTreeItemId &Parent, CQColumn *Column);
    void LoadTable(const wxTreeItemId &Parent, CQTable *Table);
    void LoadWhereCond(const wxTreeItemId &Parent, CQCond *Cond);
    void LoadGroup(const wxTreeItemId &Parent, CQGroup *Group);
    void LoadOrderBy(const wxTreeItemId &Parent, CQueryObjList *OrderBy);
    void LoadLimit(const wxTreeItemId &Parent, CQUQuery *Query);
    void LoadColumnQueries(const wxTreeItemId &Parent, CQueryObjList *Queries);
    void BuildResult();
    void BuildResultML();
    void BuildUQuery(const wxTreeItemId &Node);
    void BuildQueryExpr(const wxTreeItemId &Node);
    void BuildQuery(const wxTreeItemId &Node);
    bool BuildColumn(const wxTreeItemId &Node, int Index);
    void BuildFrom(const wxTreeItemId &Node);
    bool BuildTable(const wxTreeItemId &Node, int Index);
    void BuildJoin(const wxTreeItemId &Node);
    void BuildWhere(const wxTreeItemId &Node);
    void BuildOrderBy(const wxTreeItemId &Node);
    void BuildLimit(const wxTreeItemId &Node);
    void BuildSubQuery(const wxTreeItemId &Node);
    void BuildSubQueryML(const wxTreeItemId &Node);
    bool BuildColQuery(const wxTreeItemId &Node);
    void BuildColQueryML(const wxTreeItemId &Node);
    void MLPut(const wxChar *Val);
    void SLPut(const wxChar *Val);
    void MLPutNewLine(const wxChar *Val);
    void PutComma();
    void MLPutSeparator();
    void SLPutSeparator();
    void SLPutClauseSeparator(int Count);
    void MLPutClauseSeparator(int Count);
    void GetGrid(int GridId, const wxTreeItemId &Node);
    void ShowTables(const wxTreeItemId &Node);
    void ShowTable(const wxTreeItemId &Node);
    void AddQueryExpr();
    void AddQuery(wxTreeItemId Node);
    void AddTable();
    void AddTable(const wxString &Source, tcateg Categ, wxString Caption, const wxTreeItemId &Node);
    void AddJoin();
    void AddSubCond();
    void AddGroupBy();
    void AddHaving();
    void AddOrderBy();
    void AddLimit();
    void AddAttrLink(const wxTreeItemId &Node);
    void AddAttrLink(CQDTableBox *DstTable, const wxChar *DstAttr, CQDTableBox *SrcTable, const wxChar *SrcAttr, const wxTreeItemId &Node);
    void AddAttrLink(CQDTableBox *DstTable, const wxChar *DstAttr, CQDTableBox *SrcTable, const wxChar *SrcAttr);
    void AddCycleLink(const wxTreeItemId &Node, CQDJoin *Join, CCycleLinks *cls);
    void AddColumn(const wxTreeItemId &QueryNode, const wxChar *Expr, const wxChar *Alias);
    void ResetAttrLinks();
    void DeleteItem(const wxTreeItemId &Item);
    void DeleteQuery();
    void DeleteColumn(const wxTreeItemId &Node);
    void DeleteTable();
    void DeleteTable(wxTreeItemId Node, bool Ask = true);
    void DeleteJoin(wxTreeItemId Node, bool Ask = true);
    void DeleteWhere();
    void DeleteCond(const wxTreeItemId &Node, bool Ask = true);
    void DeleteGroupBy();
    void DeleteHaving();
    void DeleteExpr(const wxTreeItemId &Node);
    void DeleteOrderBy();
    void DeleteLimit(const wxTreeItemId &Node);
    void DeleteAttrLink(const wxTreeItemId &Node);
    void AddNewTable(const wxString &Schema, const wxString &Name, tcateg Categ);
    void CheckRootWhere();
    void CopyTree(const wxTreeItemId &DstNode, const wxTreeItemId &SrcNode);
    void ShowElementPopup();
	void ShowPopupMenu(const wxTreeItemId &Item, const wxPoint &Position);
    void SwitchEditor(bool TextEdit, bool Empty, CQueryExpr *Expr);
    void UpdateColQuery();
    
    void OnTextEdit(wxCommandEvent &Event);
    void OnTreeSelChanged(wxTreeEvent &event);
    void OnGridColSize(wxGridSizeEvent &event);
    void OnKeyDown(wxKeyEvent & event);
    void OnHorzSplitterChanged(wxSplitterEvent &event);
    void OnTreeBeginDrag(wxTreeEvent &event);
    void OnListBeginDrag(wxListEvent &event);
    void OnTreeKeyDown(wxTreeEvent &event);
    void OnDeleteItem(wxTreeEvent &event);
    void OnSyntaxCheck();
    void OnDataPreview();
    void OnTreeExpanding(wxTreeEvent &event);
    void OnErrDefin(wxCommandEvent &event);
    void OnDistinct(wxCommandEvent &event);
    void OnOptimize();
    void OnSetDesignerProps();
#ifdef LINUX
    void OnMove(wxMoveEvent &event);
    void OnTreeRightDown(wxTreeEvent &event);
    void OnTreeRightClick(wxMouseEvent &event);
#else	
    void OnTreeRightClick(wxTreeEvent &event);
    void OnGridLabelLeftClick(wxGridEvent & event);
#endif    

    void OnSize(wxSizeEvent & event);
    void OnResize();

    wxTreeItemId  AddWhere();
    wxString      GetTableName(CQDQTable *Table, unsigned Flags);
    wxString      GetAttrName(CQDQTable *Table, wxString Attr);
    const wxChar *GetSchema(CQDDBTable *Table){return Table->m_Schema.IsEmpty() ? m_Schema : Table->m_Schema;}
    void          ReleaseParams(t_clivar *clivars, int Cnt);
    int           GetParams(t_clivar **pclivar);
    int           GetTreeItemIndex(const wxTreeItemId &Item);
    int           GetQueryCount(const wxTreeItemId &QueryNode);
    int           GetColumnCount(const wxTreeItemId &QueryNode);
    int           GetFromCount(const wxTreeItemId &QueryNode);
    int           GetWhereCount(const wxTreeItemId &QueryNode);
    int           GetGroupByCount(const wxTreeItemId &QueryNode);
    int           IndexOf(const wxTreeItemId &Node);
    wxTreeItemId  GetItemByImage(const wxTreeItemId &Node, int Image);
    wxTreeItemId  GetFromNode(const wxTreeItemId &QueryNode);
    wxTreeItemId  GetWhereNode(const wxTreeItemId &QueryNode);
    wxTreeItemId  GetGroupByNode(const wxTreeItemId &QueryNode);
    wxTreeItemId  GetHavingNode(const wxTreeItemId &QueryNode){return GetItemByImage(QueryNode, qtiHaving);}
    wxTreeItemId  GetOrderByNode(const wxTreeItemId &QueryNode){return GetItemByImage(QueryNode, qtiOrderBy);}
    wxTreeItemId  GetLimitNode(const wxTreeItemId &QueryNode){return GetItemByImage(QueryNode, qtiLimit);}
    wxTreeItemId  GetItem(const wxTreeItemId &Node, int Index);
    wxTreeItemId  GetItemByTableID(const wxTreeItemId &Node, int TableID);
    wxTreeItemId  GetJoinRoot(const wxTreeItemId &Node);
    wxTreeItemId  CopyTreeItem(const wxTreeItemId &Parent, const wxTreeItemId &OldItem, size_t Before = QDTREE_APPEND);
    CQDQTable    *TableFromAliasAttr(const wxChar *Schema, const wxChar *Alias, const wxChar *Attr);
    CQDQTable    *TableFromAliasAttr(const wxChar *Schema, const wxChar *Alias, const wxChar *Attr, const wxTreeItemId &Node);
    wxString      GetFreeAlias(const wxString &Name);
    bool          IsFreeAlias(const wxChar *Name, const wxTreeItemId &ParNode);
    bool          GetAttrDef(const wxTreeItemId &Node, const char *AttrName);
    bool          WhereCondToJoin(const wxChar *Cond, CQDQTable **DstTable, wxChar *DstAttr, CQDQTable **SrcTable, wxChar *SrcAttr);
    bool          AttrNeedsPrefix(CQDQTable *TableName, const wxChar *AttrName);
    bool          TableNeedsPrefix(CQDDBTable  *Table, const wxTreeItemId &ParNode);
    bool          OnlyAndConds(const wxTreeItemId &Node);
    bool          MakesLinkCycle(int SrcTableID, int DstTableID);
    bool          CheckLinkCycle(CCycleLinks *cls, int ParentID, int TableID);
    CQDDBTable   *GetTableWithSameName(CQDDBTable *Table, const wxTreeItemId &QueryNode);
    wxToolBarBase *GetToolBar() const;
    tccharptr     Tochar(const wxChar *Src);

public:

    // member variables:
    bool     changed;
    tobjname object_name;

    CQueryDesigner(cdp_t cdpIn, tobjnum objnumIn, const char *folder_nameIn, const char *schema_nameIn) :
     any_designer(cdpIn, objnumIn, folder_nameIn, schema_nameIn, CATEG_CURSOR, _query_xpm, &querydsng_coords),
     m_Edit(cdpIn, objnumIn, CATEG_CURSOR, folder_nameIn, false),
     m_Schema(cdpIn->sel_appl_name, *cdpIn->sys_conv),
     m_Tables(this)
    { 
        changed           = false;
        m_QueryNO         = 0;
        m_QueryXNO        = 0;
        m_Grid            = NULL;
        m_TablesHeight    = 200;
        m_ElementPopup    = NULL;
        m_ShowErr         = 0;
        m_ShowIndex       = false;
        m_InReset         = false;
        m_FromPopup       = false;
        m_CurQueryChanged = false;
        m_Resize          = false;
    }
    ~CQueryDesigner(void);
    bool open(wxWindow *parent, t_global_style my_style);
 
    ////////// typical designer methods and members: ///////////////////////
    enum { TOOL_COUNT = 20 };
    void set_designer_caption(void);
    static t_tool_info tool_info[TOOL_COUNT+1];

    // virtual methods (commented in class any_designer):
    char * make_source(bool alter);
    bool IsChanged(void) const;  
    wxString SaveQuestion(void) const;  
    bool save_design(bool save_as);
    void OnCommand(wxCommandEvent & event);
    t_tool_info * get_tool_info(void) const
    { return tool_info; }
    wxMenu * get_designer_menu(void);
    void _set_focus(void)
    { if (GetHandle()) SetFocus(); }
    virtual void OnInternalIdle();
    
    // Callbacks:

    DECLARE_EVENT_TABLE()

    friend class CQDGrid;
    friend class CQDQueryGrid;
    friend class CQDSelectGrid;
    friend class CQDFromGrid;
    friend class CQDCondGrid;
    friend class CQDGroupGrid;
    friend class CQDOrderGrid;
    friend class CQDJoinGrid;
    friend class CQDLimitGrid;
    friend class CQDDropTargetTP;
    friend class CQDDropTargetTB;
    friend class CQDDropTargetPG;
    friend class CQDDropTargetTI;
    friend class CQDDragSource;
    friend class CQDTableBox;
    friend class CTablesPanel;
    friend class CElementPopup;
    friend class CQDListBox;
    friend class XMLParamsDlg;
    friend class ParamsGridTable;
    friend bool start_query_designer(cdp_t cdp, tobjnum objnum, const char * folder_name);

};
//
// Tool tip for long strings in designer grid cells
//
class CQDGridToolTip : public wxAdvToolTipText
{
protected:

    CQDGrid *m_Grid;    // Source grid
    int      m_Row;     // Cell coordinates
    int      m_Col;

public:
    
    CQDGridToolTip(CQDGrid *Grid);
    wxString GetText(const wxPoint& pos, wxRect& boundary);
};
//
// Common ancestor for property grids
//
class CQDGrid : public wxGrid, public wxGridTableBase
{
protected:

    CQueryDesigner *m_Designer;     // Parent designer
    CQDGridToolTip  m_ToolTip;      // Current tool tip
    wxTreeItemId    m_Node;         // Source design element tree item

    wxTreeItemData *GetData(int Row);
    wxTreeItemId    GetItem(int Row){return m_Designer->GetItem(m_Node, Row);}

    void Open(const wxTreeItemId &Node);
    void AddTextCol(int Col, int Width);
    void AddStaticCol(int Col, int Width);
    void AddComboCol(int Col, int Width, const wxString &Param);
    void AddBoolCol(int Col, int Width);
    void AddFictCol(int Col);
    void OnDelete();
    void OnCopy();
    void OnCut();
    void OnPaste();
    int  RowSize();

    virtual void OnOpen() = 0;
    virtual bool IsTextEditor(int Col) = 0;
    virtual int *GetColWidths() = 0;
    virtual bool OnDragOver(int Row, int Col) = 0;

    void OnCreateEditor(wxGridEditorCreatedEvent &event);
    void OnColSize(wxGridSizeEvent &event);
    void OnKeyDown(wxKeyEvent &event);

public:

    CQDGrid(CQueryDesigner *Designer, TQTreeImg ID);
    ~CQDGrid()
    {
        SetTable(NULL);
    }

    void Resize();
    void Select(int Row, int Col){MakeCellVisible(Row, Col); SetGridCursor(Row, Col);}
    void Refresh();
    
    virtual int GetNumberRows();
    virtual bool Destroy();

    DECLARE_EVENT_TABLE()

    friend class CQueryDesigner;
    friend class CQDDropTargetPG;
};
//
// Query expression property grid
//
class CQDQueryGrid : public CQDGrid
{
protected:

    static  int  ColWidth[6];
    virtual void OnOpen();
    virtual bool IsTextEditor(int Col);
    virtual int *GetColWidths(){return ColWidth;}
    virtual bool OnDragOver(int Row, int Col);

public:

    CQDQueryGrid(CQueryDesigner *Designer) : CQDGrid(Designer, qtiQueryExpr){}

    virtual int      GetNumberRows();
    virtual int      GetNumberCols();
    virtual bool     IsEmptyCell(int row, int col);
    virtual wxString GetValue(int row, int col);
    virtual void     SetValue(int row, int col, const wxString& value);
    virtual wxString GetColLabelValue(int col);

    friend class CQueryDesigner;
};
//
// SELECT property grid
//
class CQDSelectGrid : public CQDGrid
{
protected:

    wxCheckBox m_DistinctCB;    // Distinct checkbox

    static  int  ColWidth[3];

    virtual void OnOpen();
    virtual bool IsTextEditor(int Col);
    virtual int *GetColWidths(){return ColWidth;}
    virtual bool OnDragOver(int Row, int Col);

public:

    CQDSelectGrid(CQueryDesigner *Designer) : CQDGrid(Designer, qtiSelect){}

    virtual ~CQDSelectGrid(){}

    virtual int      GetNumberCols();
    virtual bool     IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void     SetValue( int row, int col, const wxString& value );
    virtual wxString GetColLabelValue( int col );
    virtual bool     Destroy();

    friend class CQueryDesigner;
    friend class CPropsPanel;
};
//
// FROM property grid
//
class CQDFromGrid : public CQDGrid
{
protected:

    static  int  ColWidth[3];
    virtual void OnOpen();
    virtual bool IsTextEditor(int Col);
    virtual int *GetColWidths(){return ColWidth;}
    virtual bool OnDragOver(int Row, int Col);

    void OnSelChange(wxGridEvent &event);
    bool ScanValue(const wxString &Value, wxString &SchemaName, wxString &TableName);

public:

    CQDFromGrid(CQueryDesigner *Designer) : CQDGrid(Designer, qtiFrom){}

    virtual int      GetNumberCols();
    virtual bool     IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void     SetValue( int row, int col, const wxString& value );
    virtual wxString GetColLabelValue( int col );

    DECLARE_EVENT_TABLE()

    friend class CQueryDesigner;
};
//
// Join property grid
//
class CQDJoinGrid : public CQDGrid
{
protected:

    static  int  ColWidth[6];
    virtual void OnOpen();
    virtual bool IsTextEditor(int Col);
    virtual int *GetColWidths(){return ColWidth;}
    virtual bool OnDragOver(int Row, int Col);

    void OnSelChange(wxGridEvent &event);

public:

    CQDJoinGrid(CQueryDesigner *Designer) : CQDGrid(Designer, qtiJoin){}

    virtual int      GetNumberRows();
    virtual int      GetNumberCols();
    virtual bool     IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void     SetValue( int row, int col, const wxString& value );
    virtual wxString GetColLabelValue( int col );

    DECLARE_EVENT_TABLE()

    friend class CQueryDesigner;
};
//
// WHERE/HAVING conditions property grid
//
class CQDCondGrid : public CQDGrid
{
protected:

    static  int  ColWidth[3];
    virtual void OnOpen();
    virtual bool IsTextEditor(int Col);
    virtual int *GetColWidths(){return ColWidth;}
    virtual bool OnDragOver(int Row, int Col);

    void OnSelChange(wxGridEvent &event);

public:

    CQDCondGrid(CQueryDesigner *Designer) : CQDGrid(Designer, qtiWhere){}

    virtual int      GetNumberCols();
    virtual bool     IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void     SetValue( int row, int col, const wxString& value );
    virtual wxString GetColLabelValue( int col );

    DECLARE_EVENT_TABLE()

    friend class CQueryDesigner;
};
//
// GROUP BY property grid
//
class CQDGroupGrid : public CQDGrid
{
protected:

    static  int  ColWidth[2];
    virtual void OnOpen();
    virtual bool IsTextEditor(int Col);
    virtual int *GetColWidths(){return ColWidth;}
    virtual bool OnDragOver(int Row, int Col);

public:

    CQDGroupGrid(CQueryDesigner *Designer) : CQDGrid(Designer, qtiGroupBy){}

    virtual int      GetNumberCols();
    virtual bool     IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void     SetValue( int row, int col, const wxString& value );
    virtual wxString GetColLabelValue( int col );

    friend class CQueryDesigner;
};
//
// ORDER BY property grid
//
class CQDOrderGrid : public CQDGrid
{
protected:

    static  int  ColWidth[3];
    virtual void OnOpen();
    virtual bool IsTextEditor(int Col);
    virtual int *GetColWidths(){return ColWidth;}
    virtual bool OnDragOver(int Row, int Col);

public:

    CQDOrderGrid(CQueryDesigner *Designer) : CQDGrid(Designer, qtiOrderBy){}

    virtual int      GetNumberCols();
    virtual bool     IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void     SetValue( int row, int col, const wxString& value );
    virtual wxString GetColLabelValue( int col );

    friend class CQueryDesigner;
};
//
// LIMIT property grid
//
class CQDLimitGrid : public CQDGrid
{
protected:

    static  int  ColWidth[3];
    virtual void OnOpen();
    virtual bool IsTextEditor(int Col);
    virtual int *GetColWidths(){return ColWidth;}
    virtual bool OnDragOver(int Row, int Col);

public:

    CQDLimitGrid(CQueryDesigner *Designer) : CQDGrid(Designer, qtiLimit){}

    virtual int      GetNumberRows();
    virtual int      GetNumberCols();
    virtual bool     IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void     SetValue( int row, int col, const wxString& value );
    virtual wxString GetColLabelValue( int col );

    friend class CQueryDesigner;
};
//
// Query designer property grid cell editor
//
class CQDGridTextEditor : public wxEvtHandler, public wxGridCellTextEditor
{
protected:
    
    CQDGrid *m_Grid;    // Parent grid

    void OnKeyDown(wxKeyEvent &event);
    void OnKillFocus(wxFocusEvent &event);
    DECLARE_EVENT_TABLE()

public:

    CQDGridTextEditor(CQDGrid *Grid){m_Grid = Grid;} 
#ifdef LINUX    
    wxTextCtrl *GetCtrl(){return Text();}
#endif    
};

#ifdef WINS

#define wbNotebook wxNotebook

#else

class wbNotebook : public wxNotebook
{
public:

    wbNotebook(){}
    virtual ~wbNotebook()
    {
        DeleteAllPages();
    }
    virtual bool DeletePage(size_t n)
    {
        return DoRemovePage(n) != NULL;
    }
};

#endif
//
// Popup window with SQL statement elemets
//
class CElementPopup : public wxFrame
{
protected:

    CQueryDesigner *m_Designer;     // Parent designer
    wbNotebook      m_Notebook;     // Tabbed notebook
    wbListCtrl      m_Operators;    // List of operators
    wbListCtrl      m_Predicates;   // List of predicates
    wbListCtrl      m_Functions;    // List of functions
    wbListCtrl      m_Separators;   // List of separators
    wbListCtrl      m_Constants;    // List of constants

    bool CreateList(wxListCtrl *List, wxWindowID id, const wxChar *Caption, const wxChar **Items);
    void OnBeginDrag(wxListEvent &event);
    void OnClose(wxCloseEvent &event);

public:

    CElementPopup(CQueryDesigner *Designer)
    {
        m_Designer  = Designer;
    }
    virtual ~CElementPopup()
    {
        m_Designer->m_ElementPopup = NULL;
    }
    bool Create();

    DECLARE_EVENT_TABLE()
};
//
// AddTableDlg controls IDs
//
#define ID_TYPECOMBO   0
#define ID_SCHEMACOMBO 1
#define ID_LIST        2
//
// Add new source table dialog
//
class CAddTableDlg : public wxDialog
{
protected:

    CQueryDesigner *m_Designer;     // Parent designer
    wxOwnerDrawnComboBox      m_TypeCB;       // Source table type combo
    wxOwnerDrawnComboBox      m_SchemaCB;     // Schema combo
    wbListCtrl      m_List;         // Object list
    wxButton        m_OkBut;        // OK button
    wxButton        m_CancelBut;    // Cancel button

    void OnTypeChange(wxCommandEvent &event);
    void OnSchemaChange(wxCommandEvent &event);
    void OnSelChange(wxListEvent &event);
    void OnItemActivated(wxListEvent &event);

public:

    bool Create(CQueryDesigner *Parent);

    DECLARE_EVENT_TABLE()

    friend class CQueryDesigner;
};
//
// DropTarget for table panel - Accepts tables from control panel
//
class CQDDropTargetTP : public wxDropTarget
{
protected:

    CQueryDesigner *m_Designer;                         // Parent designer
    char            m_Server[MAX_OBJECT_NAME_LEN + 1];  // Current server name
    wxDragResult    m_Result;                           // Last DragREsult

    virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    virtual bool OnDrop(wxCoord x, wxCoord y);
    virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);

public:

    CQDDropTargetTP(CQueryDesigner *Designer)
    {
        m_Designer = Designer;
        m_Result   = wxDragNone;
        strcpy(m_Server, Designer->cdp->locid_server_name);
        wxDataObject* dob = new C602SQLDataObject();
        SetDataObject(dob);
    }
};
//
// DataObject for column from TableBox or element from ElementPopup
//
class CQDDataObjectE_data_contents
{   wxChar      m_Data[OBJ_NAME_LEN + 1];   // Source column name / Source element text
    CQDQTable  *m_Table;                    // Source table record
    long        m_Item;                     // Source column index
    uns8        m_Type;                     // Source column type
    wxListCtrl *m_List;                     // Source list instance
};

class CQDDataObjectE : public wxDataObject
{
protected:

    wxChar      m_Data[OBJ_NAME_LEN + 1];   // Source column name / Source element text
    CQDQTable  *m_Table;                    // Source table record
    long        m_Item;                     // Source column index
    uns8        m_Type;                     // Source column type
    wxListCtrl *m_List;                     // Source list instance

public:

    CQDDataObjectE()
    {
       *m_Data  = 0;
        m_Table = NULL;
        m_Type  = 0;
        m_List  = NULL;
    }
    CQDDataObjectE(const wxChar *Data)
    {
        wxStrcpy(m_Data, Data);
        m_Table = NULL;
        m_Type  = 0;
        m_List  = NULL;
    }
    CQDDataObjectE(CQDQTable *Table, const wxChar *Attr, long Item, uns8 Type, wxListCtrl *List)
    {
        wxStrcpy(m_Data,  Attr);
        m_Table = Table;
        m_Item  = Item;
        m_Type  = Type;
        m_List  = List;
    }
    virtual ~CQDDataObjectE()
    {
        CurData = NULL;
    }

    static CQDDataObjectE *CurData;
    virtual wxDataFormat GetPreferredFormat(Direction Dir = Get) const;
    virtual size_t GetFormatCount(Direction Dir = Get) const;
    virtual void GetAllFormats(wxDataFormat *Formats, Direction Dir = Get) const;
    virtual size_t GetDataSize(const wxDataFormat &Format) const;
    virtual bool GetDataHere(const wxDataFormat &Format, void *Buf) const;
    virtual bool SetData(const wxDataFormat& Format, size_t Len, const void *Buf);
    
    bool IsAttr() const {return m_Table != NULL;}
    const wxChar *GetData() const {return m_Data;}
    const wxChar *GetAttr() const {return m_Data;}
    CQDQTable    *GetTable() const {return m_Table;}
    uns8          GetType() const {return m_Type;}
    wxListCtrl   *GetList(){return m_List;}
    long          GetItem() const {return m_Item;}
};

#define QDDATAOBJECT_SIZE sizeof(CQDDataObjectE_data_contents)  // (sizeof(CQDDataObjectE) - offsetof(CQDDataObjectE, m_Data))
//
// DataObject for designer tree items Drag&Drop
//
class CQDDataObjectI : public wxDataObject
{
protected:

    int GetSize() const {return sizeof(wxTreeItemId) + sizeof(int);}

public:

    wxTreeItemId m_Item;    // Dragged item
    int          m_Image;   // Dragged item type

    CQDDataObjectI(){}
    CQDDataObjectI(const wxTreeItemId Item, int Image)
    {
        m_Item  = Item;
        m_Image = Image;
    }
    virtual ~CQDDataObjectI()
    {
        CurData = NULL;
    }

    static CQDDataObjectI *CurData;
    virtual wxDataFormat GetPreferredFormat(Direction Dir = Get) const;
    virtual size_t GetFormatCount(Direction Dir = Get) const;
    virtual void GetAllFormats(wxDataFormat *Formats, Direction Dir = Get) const;
    virtual size_t GetDataSize(const wxDataFormat &Format) const;
    virtual bool GetDataHere(const wxDataFormat &Format, void *Buf) const;
    virtual bool SetData(const wxDataFormat& Format, size_t Len, const void *Buf);
};
//
// DropTarget for TableBox - Accepts column from other TableBox
//
class CQDDropTargetTB : public wxDropTarget
{
protected:

    CQueryDesigner *m_Designer;     // Parent designer
    CQDListBox     *m_List;         // Parent list box
    long            m_Item;         // Last item
    long            m_SelItem;      // Last highlighted item
    bool            m_Invalid;      // This TableBox is not proper
    int             m_TableID;      // Source table ID
    wxDragResult    m_Result;       // Last DragResult

    virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    virtual void OnLeave();
    virtual bool OnDrop(wxCoord x, wxCoord y);
    virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);

public:

    CQDDropTargetTB(CQDListBox *List, int TableID, CQueryDesigner *Designer)
    {
        m_List     = List;
        m_TableID  = TableID;
        m_Designer = Designer;
        SetDataObject(new CQDDataObjectE());
    }
};
//
// DropTarget for property grids - Accepts columns from TableBox or element from ElementPopup
//
class CQDDropTargetPG : public wxDropTarget
{
protected:

    CQDGrid     *m_Grid;    // Parent grid
    wxDragResult m_Result;  // Last DragResult

    virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    virtual bool OnDrop(wxCoord x, wxCoord y);
    virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);

    bool IsSeparator(const wxChar *Data);

public:

    CQDDropTargetPG(CQDGrid *Grid)
    {
        m_Grid = Grid;
        SetDataObject(new CQDDataObjectE());
    }
};

#ifdef  WINS
#define MAX_EXPTIME 0x7FFFFFFFFFFFFFFFi64
#else
#define MAX_EXPTIME 0x7FFFFFFFFFFFFFFFll
#endif
//
// DropTarget for design tree - Accepts tree items
//
class CQDDropTargetTI : public wxDropTarget
{
protected:

    wbTreeCtrl  *m_Tree;        // Parent tree
    wxTreeItemId m_Item;        // Current item
    wxTreeItemId m_SelItem;     // Last highlighted item
    wxDragResult m_Result;      // Last DragResult
    wxLongLong   m_ExpTime;     // Timeout for expanding collapsed item
    int          m_Image;       // Current item type

    virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    virtual bool OnDrop(wxCoord x, wxCoord y);
    virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
    
    void HighlightItem(const wxTreeItemId &Item, bool Highlight);

public:

    CQDDropTargetTI(wbTreeCtrl *Tree)
    {
        m_Tree    = Tree;
        m_Result  = wxDragNone;
        m_ExpTime = (wxLongLong)MAX_EXPTIME;
        SetDataObject(new CQDDataObjectI());
    }
};

#ifdef LINUX
extern char *no_xpm[];
#endif
//
// Drag source for query designer Drag&Drop operations
//
class CQDDragSource : public wxDropSource
{
protected:

    CQueryDesigner *m_Designer;     // Parent designer

    virtual bool GiveFeedback(wxDragResult effect);

public:

    CQDDragSource(CQueryDesigner *Designer, wxDataObject *Data/*, bool ItemSource = false*/) : 
#ifdef WINS    
    wxDropSource(Designer)
#else     
    wxDropSource(Designer, wxIcon(no_xpm))
#endif    
    {
        m_Designer   = Designer;
        SetData(*Data);
    }
    ~CQDDragSource()
    {
        if (m_data)
            delete m_data;
    }
};
//
// Dummy DropTarget for ElementPopup, it is necessary in order to OnDragOver does return answers
// and CQDDragSource can accept Feedback and move dragged image
//
class CQDDummyDropTarget : public wxDropTarget
{
public:

    CQDDummyDropTarget()
    {
        SetDataObject(new wxDataObjectSimple());
    }
    virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def){return wxDragNone;}
};
//
// Query designer popup menu descritor
//
struct CMenuDesc
{
    int           m_ID;     // Menu command ID
    char        **m_Icon;   // Menu icon
    const wxChar *m_Text;   // Menu text
};

#endif // _QUERYDSGN_H_

