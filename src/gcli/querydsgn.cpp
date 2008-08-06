//
// querydsgn.cpp
//
// Query designer
// ==============
//
// On input takes set of structures CQueryExpr, CQQuery, CQColumn, ... produced by compile_query 
// (queryanal.cpp). This set of structures is by Load... methods of CQueryDesigner class translated
// to design tree implemented by wxTreeCtrl. Each wxTreeCtrl item has assigned as ItemData pointer
// to query element record CQDQueryExpr, CQDQuery, CQDColumn, ..., which contains element values.
// Query element type nor relations between query elements and clauses are stored in record. Element
// type is defined by tree item icon index, relations are given only by item position in wxTreeCtrl.
// Selecting tree item invokes displaying of appropriate property grid and set of source tables that
// belongs to current subquery. Property grids read and write values to corresponding records. On
// output Build... methods traverse design tree and on base of query element records content assemble
// resulting SQL statement text. According which kind of output is requested (single/multi line),
// CPutCtx structure is filled with competent method pointers. Statement generator calls CPutCtx
// structure members to output statement parts with appropriate separatos.
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
#include "xmldsgn.h"

#include <wx/dataobj.h>
#include <wx/dnd.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/clipbrd.h>
#include <wx/caret.h>

#define  DEBUG
#include "wprez9.h"
#include "compil.h"
#include "objlist9.h"
#include "impexpui.h"
#include "controlpanel.h"
#include "fsed9.h"
#include "querydsgn.h"
#include "queryopt.h"
#include "qdprops.h"
#include "xrc/XMLParamsDlg.h"
#include "xrc/QueryOptimizationDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#include "bmps/qdQueryExpr.xpm"
#include "bmps/qdQuery.xpm"  
#include "bmps/qdSelect.xpm" 
#include "bmps/qdFrom.xpm"   
#include "bmps/qdWhere.xpm"  
#include "bmps/qdGroupBy.xpm"
#include "bmps/qdHaving.xpm" 
#include "bmps/qdOrderBy.xpm"
#include "bmps/qdTable.xpm" 
#include "bmps/qdJoin.xpm"   
#include "bmps/qdColumn.xpm" 
#include "bmps/qdCond.xpm"   
#include "bmps/qdLimit.xpm"  
#include "bmps/elementpopup.xpm"  
#include "bmps/addtable.xpm"  
#include "bmps/deletetable.xpm"  
#include "bmps/dsgnprops.xpm"  
#include "bmps/optimize.xpm"  
#include "bmps/optcond.xpm"  
#include "bmps/ta.xpm"  
#include "bmps/td.xpm"  

#include "bmps/editSQL.xpm"  
#include "bmps/querypreview.xpm"  

#include "qdprops.cpp"

#define tnQueryExpr   _("Query expression")
#define tnQuery       _("Query")
#define tnJoin        _("Join")
#define tnCondr       _("Condition")
#define tnAlias       _("Alias")
#define tnExpression  _("Expression")
#define tnName        _("Name")

wxDataFormat *DF_QueryDesignerDataE;
wxDataFormat *DF_QueryDesignerDataI;

int CQDQTable::TableID;

DEFINE_LOCAL_EVENT_TYPE(myEVT_ERRDEFIN)

const wxChar *AndOrTbl[] = {NULL, wxT("AND"), wxT("OR")};

//////////////////////////////////////////// Opening / Destoying ////////////////////////////////////////////

//
// Starts query designer
//
bool start_query_designer(cdp_t cdp, tobjnum objnum, const char * folder_name)
{ 
    // Create new designer object
    char           *def  = NULL;
    CQueryDesigner *dsgn = new CQueryDesigner(cdp, objnum, folder_name, cdp->sel_appl_name);
    CQueryExpr      Expr;

    // IF modifying existng query
    if (dsgn->modifying())
    {
        // Load query definition
        def = cd_Load_objdef(cdp, objnum, CATEG_CURSOR, NULL);
        if (!def)
        { 
            cd_Signalize(cdp);  
            return false; 
        }
        // Red object name
        cd_Read(cdp, OBJ_TABLENUM, objnum, OBJ_NAME_ATR, NULL, dsgn->object_name);
        // Compile query
        dsgn->m_ShowErr = compile_query(cdp, def, &Expr);
    }
    else  // new design
    { 
        *dsgn->object_name = 0;
    }
    // Initialize designer ancestor
    bool OK = open_standard_designer(dsgn);
    if (OK)
    {
        // Set text editor content
        if (def)
            dsgn->m_Edit.SetText(TowxChar(def, cdp->sys_conv));
        else
            dsgn->m_Edit.SetText(wxT(""));
        
        // IF syntax OK show visual editor ELSE show text editor
        dsgn->SwitchEditor(dsgn->m_ShowErr != 0, !dsgn->modifying(), &Expr);
        // IF compile error, show error message, set cursor to error position
        if (dsgn->m_ShowErr)
        {
            wxCommandEvent Event(myEVT_ERRDEFIN, 1);
            dsgn->AddPendingEvent(Event);
        }
    }
    if (def)
        corefree(def);
    return OK;
}
//
// Opens designer
//
bool CQueryDesigner::open(wxWindow *parent, t_global_style my_style)
{ 
    int res=0;
    wxBusyCursor wait;

    // Read designer properties
    m_StatmStyle      = (TStatmStyle)read_profile_int(qdSection,  "WrapStatements", ssWrapLong);
    m_Indent          = read_profile_int(qdSection,  "BlockIndent",     DEF_INDENT);
    m_LineLenLimit    = read_profile_int(qdSection,  "LineLength",      128);
    m_WrapItems       = read_profile_bool(qdSection, "WrapItems",       false);
    m_WrapAfterClause = read_profile_bool(qdSection, "WrapAfterClause", false);
    m_JoinConv        = read_profile_bool(qdSection, "ConvertJoin",     false);
    m_PrefixColumns   = read_profile_bool(qdSection, "PrefixColumns",   false);
    m_PrefixTables    = read_profile_bool(qdSection, "PrefixTables",    false);
    if (!m_WrapAfterClause)
        m_Indent      = 9;

    // open the window:
    if (!Create(parent, COMP_FRAME_ID, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH) || !m_Edit.prep(parent))
        res=5;  // window not opened
    else
    {
        wxGetApp().frame->SetStatusText(wxString(), 1);
        competing_frame::focus_receiver = this;
        try
        {
            // Create designer content
            LoadImageList();
            m_SplitVert = read_profile_int(qdSection, "VerticalSplit", 150);
#ifdef WINS
            m_Tree.Create(this, QDID_TREE);
#else
            m_Tree.Create(this, QDID_TREE, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxTR_HAS_BUTTONS);
            m_Tree.SetBackgroundColour(GetDefaultAttributes().colBg);
#endif
            m_Tree.SetImageList(&m_ImageList);
            m_RightPanel.Create(this, QDID_RIGHTPANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
            m_RightPanel.SetMinSize(wxSize(m_SplitVert + 20, 50));
#ifdef WINS
            m_Props.Create(&m_RightPanel, QDID_PROPS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
            m_Props.SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
            m_Tables.Create(&m_RightPanel, QDID_TABLES, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxCLIP_CHILDREN, QDTablePanelName);
#else
            m_Props.Create(&m_RightPanel, QDID_PROPS, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER);
            m_Tables.Create(&m_RightPanel, QDID_TABLES, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER /*| wxCLIP_CHILDREN*/, QDTablePanelName);
#endif
            m_Tables.SetScrollbars(1, 1, 1, 1);
            m_Tables.SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
            m_Tables.SetDropTarget(new CQDDropTargetTP(this));
            SplitVertically(&m_Tree, &m_RightPanel, m_SplitVert);
            m_RightPanel.SplitHorizontally(&m_Props, &m_Tables);
            DF_QueryDesignerDataE = new wxDataFormat(wxT("602SQL Query Designer Data E"));
            DF_QueryDesignerDataI = new wxDataFormat(wxT("602SQL Query Designer Data I"));
            m_Tree.SetDropTarget(new CQDDropTargetTI(&m_Tree));
            // initial design:
            res = 0;  // OK
        }
        catch (CWBException *e)
        {
            error_box(e->GetMsg(), parent);
            delete e;
            res = 6;
        }
        catch (...)
        {
            res = 7;
        }
    }
    return res==0;
}
//
// Creates ImageList designer tree icons
//
void CQueryDesigner::LoadImageList()
{ 
    if (!m_ImageList.Create(16, 16, TRUE, 15))
        return;

    { wxBitmap bmp(qdQueryExpr_xpm); m_ImageList.Add(bmp); }  // qtiQueryExpr
    { wxBitmap bmp(qdQuery_xpm);     m_ImageList.Add(bmp); }  // qtiQuery
    { wxBitmap bmp(qdSelect_xpm);    m_ImageList.Add(bmp); }  // qtiSelect
    { wxBitmap bmp(qdFrom_xpm);      m_ImageList.Add(bmp); }  // qtiFrom
    { wxBitmap bmp(qdWhere_xpm);     m_ImageList.Add(bmp); }  // qtiWhere
    { wxBitmap bmp(qdGroupBy_xpm);   m_ImageList.Add(bmp); }  // qtiGroupBy
    { wxBitmap bmp(qdHaving_xpm);    m_ImageList.Add(bmp); }  // qtiHaving
    { wxBitmap bmp(qdOrderBy_xpm);   m_ImageList.Add(bmp); }  // qtiOrderBy
    { wxBitmap bmp(qdTable_xpm);     m_ImageList.Add(bmp); }  // qtiTable
    { wxBitmap bmp(qdJoin_xpm);      m_ImageList.Add(bmp); }  // qtiJoin
    { wxBitmap bmp(qdColumn_xpm);    m_ImageList.Add(bmp); }  // qtiColumn
    { wxBitmap bmp(qdCond_xpm);      m_ImageList.Add(bmp); }  // qtiCond
    { wxBitmap bmp(qdCond_xpm);      m_ImageList.Add(bmp); }  // qtiSubCond
    { wxBitmap bmp(qdColumn_xpm);    m_ImageList.Add(bmp); }  // qtiGColumn
    { wxBitmap bmp(qdColumn_xpm);    m_ImageList.Add(bmp); }  // qtiOColumn
    { wxBitmap bmp(qdLimit_xpm);     m_ImageList.Add(bmp); }  // qtiLimit
}
//
// Destructor
//
CQueryDesigner::~CQueryDesigner(void)
{ 
    if (DF_QueryDesignerDataE)
    {
        delete DF_QueryDesignerDataE;
        DF_QueryDesignerDataE = NULL;
    }
    if (DF_QueryDesignerDataI)
    {
        delete DF_QueryDesignerDataI;
        DF_QueryDesignerDataI = NULL;
    }
    remove_tools();
    write_profile_int(qdSection, "VerticalSplit", GetSashPosition());
}

/////////////////////////// Conversion from compiled structutes to design tree //////////////////////////////

wxChar *QueryAssocTbl[] = {NULL, wxT("UNION"), wxT("INTERSECT"), wxT("EXCEPT")};
//
// Builds design tree for query expression from compiled structure 
//
void CQueryDesigner::LoadQueryExpr(const wxTreeItemId *Parent, CQueryExpr *QueryExpr)
{
    wxTreeItemId  Node;

    // Create new query expression record
    CQDQueryExpr *Data = new CQDQueryExpr(QueryExpr, cdp->sys_conv);
    
    // IF root query, create root tree item, set record as its ItemData
    if (!Parent)
        Node = m_Tree.AddRoot(_("Query root"), qtiQueryExpr, qtiQueryExpr, Data);
    // ELSE append new item to Parent, set record as its ItemData
    else
    {
        wxString Caption = GetTableName(Data, GTN_ALIAS | GTN_TITLE);
        if (QueryExpr->Assoc)
        {
            Caption += wxT(" - ");
            Caption += QueryAssocTbl[QueryExpr->Assoc];
        }
        Node = m_Tree.AppendItem(*Parent, Caption, qtiQueryExpr, qtiQueryExpr, Data);
    }
    // FOR each subquery in source structure query list build subquery tree
    for (CQUQuery *Query = (CQUQuery *)QueryExpr->Queries.GetFirst(); Query; Query = (CQUQuery *)Query->GetNext())
    {
        if (Query->TableType == TT_QUERYEXPR)
            LoadQueryExpr(&Node, (CQueryExpr *)Query);
        else 
            LoadQuery(Node,      (CQQuery *)Query);
    }
    // Build ORDER BY and LIMIT clause
    LoadOrderBy(Node, &QueryExpr->OrderBy);
    LoadLimit(Node, QueryExpr);
}
//
// Builds design tree for query from compiled structure 
//
void CQueryDesigner::LoadQuery(const wxTreeItemId &Parent, CQQuery *Query)
{
    // Create new query record
    CQDQuery *Data = new CQDQuery(Query, cdp->sys_conv);
    if (!Data)
        return;
    wxString Caption = GetTableName(Data, GTN_ALIAS | GTN_TITLE);
    if (Query->Assoc)
    {
        Caption += wxT(" - ");
        Caption += QueryAssocTbl[Query->Assoc];
    }
    wxTreeItemId OldQuery = m_CurQuery;
    // Append new tree item, set record as its ItemData
    m_CurQuery   = m_Tree.AppendItem(Parent, Caption, qtiQuery, qtiQuery, Data);
    // Append SELECT child to query item
    wxTreeItemId Node  = m_Tree.AppendItem(m_CurQuery,  wxT("SELECT"), qtiSelect);
    // Append column list to SELECT
    for (CQColumn *Column = (CQColumn *)Query->Select.GetFirst(); Column; Column = (CQColumn *)Column->GetNext())
        LoadColumn(Node, Column);
    // Append FROM child to query item
    Node = m_Tree.AppendItem(m_CurQuery, wxT("FROM"), qtiFrom);
    // Append source table list to FROM
    for (CQTable *Table = (CQTable *)Query->From.GetFirst(); Table; Table = (CQTable *)Table->GetNext())
        LoadTable(Node, Table);
    // IF some WHERE condition specified
    if (Query->Where.GetCount() > 0)
    {
        // Append WHERE child to query item
        Node = m_Tree.AppendItem(m_CurQuery, wxT("WHERE"), qtiWhere);
        // Append condition tree to WHERE
        for (CQCond *Cond = (CQCond *)Query->Where.GetFirst(); Cond; Cond = (CQCond *)Cond->GetNext())
            LoadWhereCond(Node, Cond);
    }
    // IF some GROUP BY expression specified
    if (Query->GroupBy.GetCount() > 0)
    {
        // Append GROUP BY child to query item
        Node = m_Tree.AppendItem(m_CurQuery, wxT("GROUP BY"), qtiGroupBy);
        // Append expression list to GROUP BY
        for (CQGroup *Group = (CQGroup *)Query->GroupBy.GetFirst(); Group; Group = (CQGroup *)Group->GetNext())
            LoadGroup(Node, Group);
    }
    // IF some HAVING condition specified
    if (Query->Having.GetCount() > 0) 
    {
        // Append HAVING child to query item
        Node = m_Tree.AppendItem(m_CurQuery, wxT("HAVING"), qtiHaving);
        // Append condition tree to HAVING
        for (CQCond *Cond  = (CQCond *)Query->Having.GetFirst(); Cond; Cond = (CQCond *)Cond->GetNext())
            LoadWhereCond(Node, Cond);
    }
    // Append ORDER BY and LIMIT clause
    LoadOrderBy(m_CurQuery, &Query->OrderBy);
    LoadLimit(m_CurQuery, Query);
    m_CurQuery = OldQuery;
}
//
// Builds design tree for column from compiled structure 
//
void CQueryDesigner::LoadColumn(const wxTreeItemId &Parent, CQColumn *Column)
{
    const char *cName = Column->Alias;
    if (!cName || !*cName)
        cName = Column->Expr;
    // Create new column record
    CQDColumn   *Data = new CQDColumn(Column, cdp->sys_conv);
    // Append new item to Parent, set record as its ItemData
    wxTreeItemId Node = m_Tree.AppendItem(Parent, GetNakedIdent(wxString(cName, *cdp->sys_conv)), qtiColumn, qtiColumn, Data);
    // Build column subqueries tree
    LoadColumnQueries(Node, &Column->Queries);
}
//
// Builds design tree for source table from compiled structure 
//
void CQueryDesigner::LoadTable(const wxTreeItemId &Parent, CQTable *Table)
{
    wxTreeItemData *Data;
    // IF table is database table, create new record, append new tree item, set record as its ItemData
    if (Table->TableType == TT_TABLE)
    {
        Data = new CQDDBTable((CQDBTable *)Table, cdp->sys_conv);
        wxTreeItemId Node = m_Tree.AppendItem(Parent, ((CQDDBTable *)Data)->GetAliasName(), qtiTable, qtiTable, Data);
    }
    // IF table is join
    else if (Table->TableType == TT_JOIN)
    {
        // Create new join record
        CQDJoin  *Data     = new CQDJoin((CQJoin *)Table, cdp->sys_conv);
        wxString Caption   = GetTableName(Data, GTN_ALIAS | GTN_TITLE);
        // Append new tree item, set record as its ItemData
        wxTreeItemId Node  = m_Tree.AppendItem(Parent, Caption, qtiJoin, qtiJoin, Data);
        // Build tree for join source tables
        LoadTable(Node, ((CQJoin *)Table)->DstTable.Table);
        LoadTable(Node, ((CQJoin *)Table)->SrcTable.Table);
        if (((CQJoin *)Table)->DstTable.CondTable)
        {
            Data->m_DstTableID = ((CQJoin *)Table)->DstTable.CondTable->TableID;
            Data->m_DstAttr    = wxString(((CQJoin *)Table)->DstTable.CondAttr, *cdp->sys_conv);
        }
        if (((CQJoin *)Table)->SrcTable.CondTable)
        {
            Data->m_SrcTableID = ((CQJoin *)Table)->SrcTable.CondTable->TableID;
            Data->m_SrcAttr    = wxString(((CQJoin *)Table)->SrcTable.CondAttr, *cdp->sys_conv);
        }
    } 
    // ELSE build tree for query
    else 
    {
        if (Table->TableType == TT_QUERYEXPR)
            LoadQueryExpr(&Parent, (CQueryExpr *)Table);
        else
            LoadQuery(Parent,      (CQQuery *)Table);
    }
}
//
// Builds design tree for WHERE/HAVING condition from compiled structure 
//
void CQueryDesigner::LoadWhereCond(const wxTreeItemId &Parent, CQCond *Cond)
{
    wxString Caption;

    // Create new condition record
    CQDCond *Data = new CQDCond(Cond, cdp->sys_conv);
    if (Cond->SubConds.GetCount() == 0)
        Caption = wxString(Cond->Expr, *cdp->sys_conv);
    else
    {
        CQDQuery *Query = (CQDQuery *)m_Tree.GetItemData(m_CurQuery);
        Caption         = wxString::Format(wxT("%s %d"), tnCondr, ++Query->m_CondNO);
        Data->m_Expr    = Caption;
    }
    if (Cond->Assoc != CA_NONE)
        Caption = Caption + wxT(" - ") + AndOrTbl[Cond->Assoc];
    // IF condition has no subconditions, append new tree item, build condition subqueries tree
    if (Cond->SubConds.GetCount() == 0)
    {
        wxTreeItemId    Node = m_Tree.AppendItem(Parent, Caption, qtiCond, qtiCond, Data);
        LoadColumnQueries(Node, &Cond->Queries);
    }
    // ELSE append new tree item, build subconditions tree
    else
    {
        wxTreeItemId    Node = m_Tree.AppendItem(Parent, Caption, qtiSubConds, qtiSubConds, Data);
        for (Cond = (CQCond *)Cond->SubConds.GetFirst(); Cond; Cond = (CQCond *)Cond->GetNext())
            LoadWhereCond(Node, Cond);
    }
}
//
// Creates design tree item for GROUP BY expression from compiled structure
//
void CQueryDesigner::LoadGroup(const wxTreeItemId &Parent, CQGroup *Group)
{
    // Create new GROUP BY expression record
    CQDGroup    *Data = new CQDGroup(Group, cdp->sys_conv);
    // Append new tree item, set record as its ItemData
    wxTreeItemId Node = m_Tree.AppendItem(Parent, GetNakedIdent(wxString(Group->Expr, *cdp->sys_conv)), qtiGColumn, qtiGColumn, Data);
}
//
// Builds design tree for ORDER BY clause from compiled structure
//
void CQueryDesigner::LoadOrderBy(const wxTreeItemId &Parent, CQueryObjList *OrderBy)
{
    if (OrderBy->GetCount() > 0)
    {
        // Append ORDER BY item
        wxTreeItemId OrderNode = m_Tree.AppendItem(Parent, wxT("ORDER BY"), qtiOrderBy);
        // Append ORDER BY expression item for each ORDER BY expression
        for (CQOrder *Order = (CQOrder *)OrderBy->GetFirst(); Order; Order = (CQOrder *)Order->GetNext())
        {
            wxString Caption = GetNakedIdent(wxString(Order->Expr, *cdp->sys_conv));
            if (Order->Desc)
                Caption += wxT(" - DESC");
            CQDOrder    *Data = new CQDOrder(Order, cdp->sys_conv);
            wxTreeItemId Node = m_Tree.AppendItem(OrderNode, Caption, qtiOColumn, qtiOColumn, Data);
        }
    }
}
//
// Creates design tree item for LIMIT clause from compiled structure
//
void CQueryDesigner::LoadLimit(const wxTreeItemId &Parent, CQUQuery *Query)
{
    if ((Query->LimitOffset && *Query->LimitOffset) || (Query->LimitCount && *Query->LimitCount)) 
    {
        wxTreeItemData *Data = new CQDLimit(Query, cdp->sys_conv);
        m_Tree.AppendItem(Parent, wxT("LIMIT"), qtiLimit, qtiLimit, Data);
    }
}
//
// Builds design tree for column/condition subqueries from compiled structure
//
void CQueryDesigner::LoadColumnQueries(const wxTreeItemId &Parent, CQueryObjList *Queries)
{
    // Delete old content
    m_Tree.DeleteChildren(Parent);
    // FOR each column/condition subquery, build subquery tree
    for (CQUQuery *Query = (CQUQuery *)Queries->GetFirst(); Query; Query = (CQUQuery *)Query->GetNext())
    {
        if (Query->TableType == TT_QUERYEXPR)
            LoadQueryExpr(&Parent, (CQueryExpr *)Query);
        else
            LoadQuery(Parent,      (CQQuery *)Query);
    }
}

////////////////////////////// Conversion from design tree to SQL statement /////////////////////////////////

//
// Builds resulting SQL statement by tree structure
//
void CQueryDesigner::BuildResult()
{
    // Set max line length
    m_LineLen = m_StatmStyle == ssWrapNever ? SINGLELINE_LEN : m_LineLenLimit;
    // IF always wrap subqueries, build multiline statement
    if (m_StatmStyle == ssWrapAlways)
        BuildResultML();
    // ELSE wrap only long statements, try single line statement first
    else 
    {
        // Initialize single line building
        m_Result.Empty();
        m_LineTooLong   = false;
        m_ResultLevel   = 0;
        m_LinePos       = 0;
        Put.MultiLine   = false;
        Put.Item        = &CQueryDesigner::SLPut;
        Put.NewLineItem = &CQueryDesigner::SLPut;
        Put.ClauseSep   = &CQueryDesigner::SLPutClauseSeparator;
        Put.Separator   = &CQueryDesigner::SLPutSeparator;
        try
        {
            BuildQueryExpr(m_Tree.GetRootItem());
        }
        catch (...)
        {
            // IF build interrupted due line too long
            if (!m_LineTooLong)
                throw;
            // Build statement again as multiline
            BuildResultML();
        }
    }
}
//
// Builds multiline resulting SQL statement by tree structure
//
void CQueryDesigner::BuildResultML()
{
    // Initialize multi line building
    m_Result.Empty();
    m_LineTooLong   = false;
    m_ResultLevel   = 0;
    m_LinePos       = 0;
    Put.MultiLine   = true;
    Put.Item        = &CQueryDesigner::MLPut;
    Put.NewLineItem = &CQueryDesigner::MLPutNewLine;
    Put.ClauseSep   = &CQueryDesigner::MLPutClauseSeparator;
    Put.Separator   = &CQueryDesigner::MLPutSeparator;
    BuildQueryExpr(m_Tree.GetRootItem());
}
//
// Builds SQL statement for query given by Node item
//
void CQueryDesigner::BuildUQuery(const wxTreeItemId &Node)
{
    // Try to build single line statement first
    m_Result.Empty();
    m_LineLen       = SINGLELINE_LEN;
    m_LineTooLong   = false;
    m_ResultLevel   = 0;
    m_LinePos       = 0;
    Put.MultiLine   = false;
    Put.Item        = &CQueryDesigner::SLPut;
    Put.NewLineItem = &CQueryDesigner::SLPut;
    Put.ClauseSep   = &CQueryDesigner::SLPutClauseSeparator;
    Put.Separator   = &CQueryDesigner::SLPutSeparator;
    try
    {
        // IF Node is query expression, build query expression ELSE build simple query
        if (m_Tree.GetItemImage(Node) == qtiQueryExpr)
            BuildQueryExpr(Node);
        else
            BuildQuery(Node);
    }
    catch (...)
    {
        // IF build interrupted due line too long
        if (!m_LineTooLong)
            throw;
        // Build statement again as multiline
        m_Result.Empty();
        m_LineTooLong   = false;
        m_ResultLevel   = 0;
        m_LinePos       = 0;
        Put.MultiLine   = true;
        Put.Item        = &CQueryDesigner::MLPut;
        Put.NewLineItem = &CQueryDesigner::MLPutNewLine;
        Put.ClauseSep   = &CQueryDesigner::MLPutClauseSeparator;
        Put.Separator   = &CQueryDesigner::MLPutSeparator;
        if (m_Tree.GetItemImage(Node) == qtiQueryExpr)
            BuildQueryExpr(Node);
        else
            BuildQuery(Node);
    }
}
//
// Builds SQL statement for query expression given by Node item
//
void CQueryDesigner::BuildQueryExpr(const wxTreeItemId &Node)
{
    wxTreeItemIdValue cookie;

    // EXIT IF QueryExpr contains empty SELECT only
    wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie);
    if (!Item.IsOk())
        return;
    int ii = m_Tree.GetItemImage(Item);
    if (ii == qtiQuery && GetColumnCount(Item) == 0 && GetFromCount(Item) == 0 && GetWhereCount(Item) == 0)
        return;
    int QueryCount = GetQueryCount(Node);
    // INSENSITIVE
    CQDQueryExpr *QueryExpr = (CQDQueryExpr *)m_Tree.GetItemData(Node); 
    if (QueryExpr->m_CursorFlags & CDF_INSENSITIVE)
        (this->*Put.Item)(wxT("INSENSITIVE "));
    // SENSITIVE
    else if (QueryExpr->m_CursorFlags & CDF_SENSITIVE)
        (this->*Put.Item)(wxT("SENSITIVE "));
    // LOCKED
    else if (QueryExpr->m_CursorFlags & CDF_LOCKED)
        (this->*Put.Item)(wxT("LOCKED "));
    // SCROLL
    if (QueryExpr->m_CursorFlags & CDF_SCROLL)
        (this->*Put.Item)(wxT("SCROLL "));
    // CURSOR FOR
    if (QueryExpr->m_CursorFlags & (CDF_INSENSITIVE | CDF_SENSITIVE | CDF_LOCKED))
        (this->*Put.Item)(wxT("CURSOR FOR "));
    // VIEW ViewName AS
    else if (QueryExpr->m_CursorFlags & CDF_VIEW)
    {
        (this->*Put.Item)(wxT("VIEW "));
        (this->*Put.Item)(QueryExpr->m_ViewName);
        (this->*Put.Item)(wxT(" AS "));
    }
    bool First = true;
    // FOR each subquery i expression
    do
    {
        // IF not first query, append statement separator
        if (First)
            First = false;
        else
            (this->*Put.Separator)();
        // IF subquery is simple query
        if (ii == qtiQuery)
        {
            // IF input query expression contains more subqueries AND current subquery contains ORDER BY or LIMIT, 
            // enclose result to brackets
            wxTreeItemId OrderLimit;
            if (QueryCount > 1)
            {
                OrderLimit = GetOrderByNode(Item);
                if (!OrderLimit.IsOk())
                    OrderLimit = GetLimitNode(Item);
            }
            if (OrderLimit.IsOk())
            {
                (this->*Put.Item)(wxT("("));
                m_ResultLevel += m_Indent;
                (this->*Put.Separator)();
            }
            // Build subquery
            BuildQuery(Item);
            // IF needed, append close bracket
            if (OrderLimit.IsOk())
            {
                m_ResultLevel -= m_Indent;
                if (Put.MultiLine)
                    MLPutSeparator();
                (this->*Put.Item)(wxT(")"));
            }
        }
        // ELSE subquery is query expression
        else 
        {
            // Append open bracket, increment indent
            (this->*Put.Item)(wxT("("));
            m_ResultLevel += m_Indent;
            // Build query expression
            BuildQueryExpr(Item);
            // Decrement indent
            m_ResultLevel -= m_Indent;
            // IF mulitiline, append new line
            if (Put.MultiLine)
                MLPutSeparator();
            // Append close bracket
            (this->*Put.Item)(wxT(")"));
        }
        // IF association to next subquery
        CQDUQuery *Query = (CQDUQuery *)m_Tree.GetItemData(Item);
        if (Query->m_Assoc != QA_NONE)
        {
            // Append association
            (this->*Put.Separator)();
            (this->*Put.Separator)();
            (this->*Put.Item)(QueryAssocTbl[Query->m_Assoc]);
            // ALL
            if (Query->m_All)
                (this->*Put.Item)(wxT(" ALL"));
            // CORRESPONDING
            if (Query->m_Corresp)
                (this->*Put.Item)(wxT(" CORRESPONDING"));
            // BY (expression)
            if (!Query->m_By.IsEmpty())
            {
                (this->*Put.Item)(wxT(" BY ("));
                (this->*Put.Item)(Query->m_By.Strip(wxString::both));
                (this->*Put.Item)(wxT(")"));
            }
            (this->*Put.Separator)();
        }
        Item = m_Tree.GetNextChild(Node, cookie);
        if (!Item.IsOk())
            break;
        ii = m_Tree.GetItemImage(Item);
    }
    while (ii == qtiQuery || ii == qtiQueryExpr);

    // IF ORDER BY or LIMIT follows AND expression contains more subqueries, append separator
    if ((ii == qtiOrderBy || ii == qtiLimit) && QueryCount > 1)
        (this->*Put.Separator)();
    // IF ORDER BY
    if (ii == qtiOrderBy)
    {
        // Build ORDER BY clause
        BuildOrderBy(Item);
        Item = m_Tree.GetNextChild(Node, cookie);
        if (!Item.IsOk())
            return;
        ii = m_Tree.GetItemImage(Item);
    }
    // IF LIMIT, build LIMIT clause
    if (ii == qtiLimit)
        BuildLimit(Item);
}
//
// Builds SQL statement for query given by Node item
//
void CQueryDesigner::BuildQuery(const wxTreeItemId &Node)
{
    // EXIT IF query is empty
    if (GetColumnCount(Node) == 0 && GetFromCount(Node) == 0 && GetWhereCount(Node) == 0 && GetGroupByCount(Node) == 0)
        return;

    int Sep         = 3;
    CQDQuery *Query = (CQDQuery *)m_Tree.GetItemData(Node);
    // SELECT
    (this->*Put.Item)(wxT("SELECT"));
    // DISTINCT
    if (Query->m_Distinct)
    {
        (this->*Put.Item)(wxT(" DISTINCT"));
        Sep = 1;
    }
    // Increment indent
    m_ResultLevel += m_Indent;
    // Append clause separator
    (this->*Put.ClauseSep)(Sep);

    wxTreeItemIdValue cookie;
    // SubNode = SELECT item
    wxTreeItemId      SubNode = m_Tree.GetFirstChild(Node, cookie);
    wxTreeItemId      Item;
    int i   = 0;
    int Len = (int)m_Result.Len();

    wxTreeItemIdValue ccookie;
    bool Empty = true;
    // FOR each child of SELECT
    for (Item = m_Tree.GetFirstChild(SubNode, ccookie); Item.IsOk(); Item = m_Tree.GetNextChild(SubNode, ccookie))
    {
        // Build column expression
        bool Alone = BuildColumn(Item, i++);
        Len        = (int)m_Result.Len();
        // Append ,
        PutComma();
        // IF single column OR wrap  AND multiline, append new line
        if (Alone || (m_WrapItems && Put.MultiLine))
            MLPutSeparator();
        Empty = false;
    }
    // Truncate last ,
    m_Result.Truncate(Len);
    // IF SELECT clause is empty, append *
    if (Empty)
        (this->*Put.Item)(wxT("*"));
    // Decrement indent, append separator
    m_ResultLevel -= m_Indent;
    (this->*Put.Separator)();

    // FROM
    (this->*Put.Item)(wxT("FROM"));
    // Increment indent, append clause separator
    m_ResultLevel += m_Indent;
    (this->*Put.ClauseSep)(5);
    SubNode = m_Tree.GetNextChild(Node, cookie);
    // Build FROM clause
    BuildFrom(SubNode);
    // Decrement indent
    m_ResultLevel -= m_Indent;

    // EXIT IF no clause follows
    SubNode = m_Tree.GetNextChild(Node, cookie);
    if (!SubNode.IsOk())
        return;
    int ii  = m_Tree.GetItemImage(SubNode);
    // IF WHERE follows AND is not empty
    if (ii == qtiWhere && m_Tree.GetChildrenCount(SubNode, false) > 0) 
    {
        // Append WHERE
        (this->*Put.Separator)();
        (this->*Put.Item)(wxT("WHERE"));
        // Increment indent, append clause separator
        m_ResultLevel += m_Indent;
        (this->*Put.ClauseSep)(4);
        // Build WHERE content
        BuildWhere(SubNode);
        // Decrement indent
        m_ResultLevel -= m_Indent;
        SubNode = m_Tree.GetNextChild(Node, cookie);
        // EXIT IF no clause follows
        if (!SubNode.IsOk())
            return;
        ii = m_Tree.GetItemImage(SubNode);
    }
    // IF GROUP BY follows AND is not empty
    if (ii == qtiGroupBy && m_Tree.GetChildrenCount(SubNode, false) > 0) 
    {
        // Append GROUP BY
        (this->*Put.Separator)();
        (this->*Put.Item)(wxT("GROUP BY"));
        // Increment indent, append clause separator
        m_ResultLevel += m_Indent;
        (this->*Put.ClauseSep)(1);
        bool First = true;
        // FOR each GROUP BY child
        for (Item = m_Tree.GetFirstChild(SubNode, ccookie); Item.IsOk(); Item = m_Tree.GetNextChild(SubNode, ccookie))
        {
            // IF wrap AND multiline AND not first expression, append separator
            if (m_WrapItems && Put.MultiLine && !First)
                (this->*Put.Separator)();
            First = false;
            CQDGroup *Group = (CQDGroup *)m_Tree.GetItemData(Item);
            // Append expression,
            (this->*Put.NewLineItem)(Group->m_Expr.Strip(wxString::both));
            PutComma();
        }
        // Trunate last ,
        m_Result.Truncate(m_Result.Len() - 2);
        // Decrement indent
        m_ResultLevel -= m_Indent;

        SubNode = m_Tree.GetNextChild(Node, cookie);
        // EXIT IF no clause follows
        if (!SubNode.IsOk())
            return;
        ii = m_Tree.GetItemImage(SubNode);
        // IF HAVING follows AND is not empty
        if (ii == qtiHaving && m_Tree.GetChildrenCount(SubNode, false) > 0)
        {
            // Append HAVING
            (this->*Put.Separator)();
            (this->*Put.Item)(wxT("HAVING"));
            // Increment indent, append clause separator
            m_ResultLevel += m_Indent;
            (this->*Put.ClauseSep)(3);
            // Build HAVING conditions
            BuildWhere(SubNode);
            // Decrement indent
            m_ResultLevel -= m_Indent;
            // EXIT IF no clause follows
            SubNode = m_Tree.GetNextChild(Node, cookie);
            if (!SubNode.IsOk())
                return;
            ii = m_Tree.GetItemImage(SubNode);
        }
    }
    // IF ORDER BY follows
    if (ii == qtiOrderBy)
    {
        // Build ORDER BY content
        BuildOrderBy(SubNode);
        SubNode = m_Tree.GetNextChild(Node, cookie);
        // EXIT IF no clause follows
        if (!SubNode.IsOk())
            return;
        ii = m_Tree.GetItemImage(SubNode);
    }
    // IF LIMIT follows, build LIMIT content
    if (ii == qtiLimit)
        BuildLimit(SubNode);
}

#define ColumnSyntaxErrorNO  _("Syntax error in column %d")
#define ColumnSyntaxErrorAl  _("Syntax error in column %s")
//
// Builds column expression for column given by Node item
// Returns true if new line should be added after column definition 
//
bool CQueryDesigner::BuildColumn(const wxTreeItemId &Node, int Index)
{
    bool Result;
    CQDColumn *Column = (CQDColumn *)m_Tree.GetItemData(Node);
    // IF error in column definition
    if (Column->m_Error) 
    {
        wxString Err;
        if (Column->m_Alias.IsEmpty())
            Err = wxString::Format(ColumnSyntaxErrorNO, Index + 1);
        else
            Err = wxString::Format(ColumnSyntaxErrorAl, Column->m_Alias.c_str());
        // IF for syntax check, preview or optimization, interupt build
        if (!m_ToText)
            throw new CWBException(Err);
        // ELSE show error message
        error_box(Err, this);
    }
    wxTreeItemIdValue cookie;
    // IF has no subqueries OR syntax error
    if (!m_Tree.GetFirstChild(Node, cookie).IsOk() || Column->m_Error)
    {
        Result = false;
        // Append column definition
        wxString Val = Column->m_Expr.Strip(wxString::both);
        // IF Alias specified, append AS Alias
        if (!Column->m_Alias.IsEmpty())
            Val += wxT(" AS ") + GetProperIdent(Column->m_Alias);
        // IF INTO specified, append INTO variable
        if (!Column->m_Into.IsEmpty())
            Val += wxT(" INTO ") + Column->m_Into;
        (this->*Put.NewLineItem)(Val);
    } 
    // ELSE has subqueries
    else
    {
        // IF Multiline AND not first column AND current position > line begining, append new line
        Result = Put.MultiLine && (m_Result.Len() - m_LinePos > m_ResultLevel);
        if (Result && Index > 0)
            MLPutSeparator();
        // Build column expression
        bool ml = BuildColQuery(Node);
        // IF Alias OT INTO
        if (!Column->m_Alias.IsEmpty() || !Column->m_Into.IsEmpty())
        {
            // Append separator
            if (ml)
                MLPutSeparator();
            else
                SLPutSeparator();
            // IF Alias, append AS Alias
            if (!Column->m_Alias.IsEmpty())
            {
                MLPut(wxT("AS "));
                MLPut(Column->m_Alias);
            }
            // IF INTO, append INTO variable
            if (!Column->m_Into.IsEmpty())
            {
                MLPut(wxT("INTO "));
                MLPut(Column->m_Into);
            }
        }
    }
    return Result;
}
//
// Builds FROM clause for Node item
//
void CQueryDesigner::BuildFrom(const wxTreeItemId &Node)
{
    wxTreeItemIdValue cookie;
    int i   = 0;
    int Len = (int)m_Result.Len();

    // FOR each child of Node
    for (wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
    {
        // Build table definition
        bool Alone = BuildTable(Item, i);
        Len = (int)m_Result.Len();
        // Append ,
        PutComma();
        // IF needed append new line
        if (Alone || (m_WrapItems && Put.MultiLine))
            MLPutSeparator();
    }
    // Truncete last ,
    m_Result.Truncate(Len);
}
//
// Builds table definition for table given by Node item
// Returns true if new line should be appended after table definition
//
bool CQueryDesigner::BuildTable(const wxTreeItemId &Node, int Index)
{
    CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Node);
    // IF table is database table
    if (Table->m_Type == TT_TABLE)
    {
        // Append table name
        wxString Val = GetTableName(Table, 0);
        // IF INDEX specified, append INDEX expression
        if (!((CQDDBTable *)Table)->m_Index.IsEmpty())
            Val += wxT(" INDEX ") + ((CQDDBTable *)Table)->m_Index;
        // IF Alias specified, append Alias
        if (!Table->m_Alias.IsEmpty())
            Val += wxT(" ") + GetProperIdent(Table->m_Alias);
        (this->*Put.NewLineItem)(Val);
        return false;
    } 
    else 
    {
        // IF Multiline AND current position is not line begining, append new line
        bool Result = Put.MultiLine && (m_Result.Len() - m_LinePos > m_ResultLevel);
        if (Result)
            MLPutSeparator();
        // IF table is join, build join ELSE build subquery
        if (Table->m_Type == TT_JOIN)
            BuildJoin(Node);
        else
            BuildSubQuery(Node);
        // IF Alias specified, append Alias
        if (!Table->m_Alias.IsEmpty())
        {
            (this->*Put.Separator)();
            (this->*Put.Item)(GetProperIdent(Table->m_Alias));
        }
        return Result;
    }
}
//
// Builds join definition for join given by Node item
//
void CQueryDesigner::BuildJoin(const wxTreeItemId &Node)
{
    wxString Val;
    wxTreeItemIdValue cookie;
    wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie);
    if (!Item.IsOk())
        return;
    // Build first source table
    BuildTable(Item, 0);
    CQDJoin *Join = (CQDJoin *)m_Tree.GetItemData(Node);
    // CROSS JOIN
    if (Join->m_JoinType & JT_CROSS)
        Val = wxT(" CROSS JOIN ");
    else 
    {
        // NATURAL
        if (Join->m_JoinType & JT_NATURAL)
            Val = wxT(" NATURAL");
        // INNER
        if (Join->m_JoinType & JT_INNER)
            Val = wxT(" INNER");
        else 
        {
            // LEFT
            if (Join->m_JoinType & JT_LEFT)
                Val = wxT(" LEFT");
            // RIGHT
            else if (Join->m_JoinType & JT_RIGHT)
                Val = wxT(" RIGHT");
            // FULL
            else if (Join->m_JoinType & JT_FULL)
                Val = wxT(" FULL");
            // OUTER
            if (Join->m_JoinType & JT_OUTER)
                Val += wxT(" OUTER");
        }
        // JOIN
        Val += wxT(" JOIN ");
    }
    (this->*Put.Item)(Val);
    Item = m_Tree.GetNextChild(Node, cookie);
    // Build second source table
    BuildTable(Item, 0);
    // IF ON specified, append ON expression
    if (Join->m_JoinType & JT_ON)
        Val = wxT(" ON ") + Join->m_Cond.Strip(wxString::both);
    // ELSE append USING expression
    else
        Val = wxT(" USING ") + Join->m_Cond.Strip(wxString::both);
    (this->*Put.Item)(Val);
}
//
// Builds WHERE clause for Node item
//
void CQueryDesigner::BuildWhere(const wxTreeItemId &Node)
{
    wxTreeItemIdValue cookie;
    int i = 0;

    // FOR each child of Node
    for (wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie), i++)
    {
        CQDCond *Cond = (CQDCond *)m_Tree.GetItemData(Item);
        int ii = m_Tree.GetItemImage(Item);
        // IF subcondition set AND set is not empty
        if (ii == qtiSubConds && m_Tree.GetChildrenCount(Item) > 0) 
        {
            // IF Multilene AND not first subcondition AND not position on line begining, append new line
            if (Put.MultiLine && i > 0 && m_Result.Len() - m_LinePos > m_ResultLevel)
                MLPutSeparator();
            // Append open bracket
            (this->*Put.Item)(wxT("("));
            // Increment indent
            m_ResultLevel += m_Indent;
            // IF multiline, append new line
            if (Put.MultiLine)
                MLPutSeparator();
            // Build subcondition set
            BuildWhere(Item);
            // Decrement indent
            m_ResultLevel -= m_Indent;
            // IF multiline, append new line
            if (Put.MultiLine)
                MLPutSeparator();
            // Append close bracket
            (this->*Put.Item)(wxT(")"));
        } 
        // ELSE simple condition
        else
        {
            // IF no subqueries, append condition definition
            if (m_Tree.GetChildrenCount(Item) == 0)
                (this->*Put.Item)(Cond->m_Expr.Strip(wxString::both));
            // ELSE build subqueries
            else
                BuildColQuery(Item);
        }
        // IF association specified
        if (Cond->m_Assoc != CA_NONE)
        {
            // IF not last item, append association
            if (m_Tree.GetNextSibling(Item).IsOk())
            {
                (this->*Put.Item)(wxT(" "));
                (this->*Put.Item)(AndOrTbl[Cond->m_Assoc]);
                (this->*Put.Separator)();
            }
            // ELSE last item, reset association, redraw grid
            else
            {
                Cond->m_Assoc = CA_NONE;
                m_Grid->ForceRefresh();
            }
        }
    }
}
//
// Builds ORDER BY clause for Node item
//
void CQueryDesigner::BuildOrderBy(const wxTreeItemId &Node)
{
    if (m_Tree.GetChildrenCount(Node, false) > 0)
    {
        // Append ORDER BY
        (this->*Put.Separator)();
        (this->*Put.Item)(wxT("ORDER BY"));
        // Increment indent
        m_ResultLevel += m_Indent;
        // Append clause separator
        (this->*Put.ClauseSep)(1);
        bool First = true;
        wxTreeItemIdValue cookie;
        // FOR each child of Node
        for (wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
        {
            // Append expression
            CQDOrder *Order = (CQDOrder *)m_Tree.GetItemData(Item);
            wxString Val    = Order->m_Expr.Strip(wxString::both);
            // IF DESC specified, append DESC
            if (Order->m_Desc)
                Val += wxT(" DESC");
            // IF wrap AND multiline AND not first expression, append new line
            if (m_WrapItems && Put.MultiLine && !First)
                MLPutSeparator();
            First = false;
            (this->*Put.NewLineItem)(Val);
            // Append ,
            PutComma();
        }
        // Decrement indent
        m_ResultLevel -= m_Indent;
        // Truncate last ,
        m_Result.Truncate(m_Result.Len() - 2);
    }
}
//
// Builds LIMIT clause for Node item
//
void CQueryDesigner::BuildLimit(const wxTreeItemId &Node)
{
    CQDLimit *Limit = (CQDLimit *)m_Tree.GetItemData(Node);
    if (Limit->m_Offset.IsEmpty() && Limit->m_Count.IsEmpty())
        return;
    // Append LIMIT
    (this->*Put.Separator)();
    (this->*Put.Item)(wxT("LIMIT"));
    // Increment indent
    m_ResultLevel += m_Indent;
    // Append clause separator
    (this->*Put.ClauseSep)(4);
    // IF offset specified, append offset,
    if (!Limit->m_Offset.IsEmpty()) 
    {
        (this->*Put.Item)(Limit->m_Offset.Strip(wxString::both));
        PutComma();
    }
    // IF count specified, append count
    if (!Limit->m_Count.IsEmpty())
        (this->*Put.Item)(Limit->m_Count.Strip(wxString::both));
    m_ResultLevel -= m_Indent;
}
//
// Builds subquery definition for query given by Node item
//
void CQueryDesigner::BuildSubQuery(const wxTreeItemId &Node)
{
    // IF always wrap subqueries, build multiline subquery
    if (m_StatmStyle == ssWrapAlways)
        BuildSubQueryML(Node);
    else 
    {
        // Initialize single line statement
        int LinePos     = m_LinePos;
        int EndPos      = (int)m_Result.Len();
        int ResultLevel = m_ResultLevel;
        CPutCtx OldPut  = Put;
        Put.MultiLine   = false;
        Put.Item        = &CQueryDesigner::SLPut;
        Put.NewLineItem = &CQueryDesigner::SLPut;
        Put.ClauseSep   = &CQueryDesigner::SLPutClauseSeparator;
        Put.Separator   = &CQueryDesigner::SLPutSeparator;
        try
        {
            // Append open bracket
            (this->*Put.Item)(wxT("("));
            // IF query expression, try to build single line query expression ELSE try to build single line simple query
            if (m_Tree.GetItemImage(Node) == qtiQueryExpr)
                BuildQueryExpr(Node);
            else
                BuildQuery(Node);
            // Append close bracket
            (this->*Put.Item)(wxT(")"));
            Put = OldPut;
        }
        catch (...)
        {
            // IF build interrupted due line too long
            if (!m_LineTooLong || !OldPut.MultiLine)
                throw;
            // Build multiline subquery again
            m_Result.Truncate(EndPos);
            m_LinePos     = LinePos;
            m_ResultLevel = ResultLevel;
            BuildSubQueryML(Node);
        }
    }
}
//
// Builds multiline subquery definition for query given by Node item
//
void CQueryDesigner::BuildSubQueryML(const wxTreeItemId &Node)
{
    // Initialize multiline build
    Put.MultiLine   = true;
    Put.Item        = &CQueryDesigner::MLPut;
    Put.NewLineItem = &CQueryDesigner::MLPutNewLine;
    Put.ClauseSep   = &CQueryDesigner::MLPutClauseSeparator;
    Put.Separator   = &CQueryDesigner::MLPutSeparator;
    // Append open bracket
    MLPut(wxT("("));
    // Increment indent, append new line
    m_ResultLevel += m_Indent;
    MLPutSeparator();
    // IF query expression, build query expression ELSE build simple query
    if (m_Tree.GetItemImage(Node) == qtiQueryExpr)
        BuildQueryExpr(Node);
    else
        BuildQuery(Node);
    // Decrement indent, append new line
    m_ResultLevel -= m_Indent;
    MLPutSeparator();
    // Append close bracket
    MLPut(wxT(")"));
}
//
// Builds column/condition with subqueries definition for query given by Node item
//
bool CQueryDesigner::BuildColQuery(const wxTreeItemId &Node)
{
    // IF allways wrap subqueries, build multiline definition
    if (m_StatmStyle == ssWrapAlways)
    {
        BuildColQueryML(Node);
        return true;
    }
    else
    {
        // Initialize single line build
        int LinePos     = m_LinePos;
        int EndPos      = (int)m_Result.Len();
        int ResultLevel = m_ResultLevel;
        CPutCtx OldPut  = Put;
        Put.MultiLine   = false;
        Put.Item        = &CQueryDesigner::SLPut;
        Put.NewLineItem = &CQueryDesigner::SLPut;
        Put.ClauseSep   = &CQueryDesigner::SLPutClauseSeparator;
        Put.Separator   = &CQueryDesigner::SLPutSeparator;
        // Try build single line definition
        try
        {
            wxTreeItemIdValue cookie;
            // FOR each child of Node
            for (wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
            {
                CQDUQuery *Query = (CQDUQuery *)m_Tree.GetItemData(Item);
                // Append prefix
                (this->*Put.Item)(Query->m_Prefix.Strip(wxString::both));
                // Append open bracket
                (this->*Put.Separator)();
                (this->*Put.Item)(wxT("("));
                // IF query expression, build query expression ELSE build simple query
                if (Query->m_Type == TT_QUERYEXPR)
                    BuildQueryExpr(Item);
                else
                    BuildQuery(Item);
                // Append close bracket
                (this->*Put.Item)(wxT(")"));
                (this->*Put.Separator)();
            }
            // Append postfix
            CQDQueryTerm *Term = (CQDQueryTerm *)m_Tree.GetItemData(Node);
            (this->*Put.Item)(Term->m_Postfix.Strip(wxString::both));
            Put    = OldPut;
            return false;
        }
        catch (...)
        {
            // IF build interrupted due line too long
            if (!m_LineTooLong || !OldPut.MultiLine)
                throw;
            // Build multiline definition again
            m_Result.Truncate(EndPos);
            m_LinePos     = LinePos;
            m_ResultLevel = ResultLevel;
            BuildColQueryML(Node);
            return true;
        }
    }
}
//
// Builds column/condition with subqueries multiline definition for query given by Node item
//
void CQueryDesigner::BuildColQueryML(const wxTreeItemId &Node)
{
    // Initialize multiline definition
    Put.MultiLine   = true;
    Put.Item        = &CQueryDesigner::MLPut;
    Put.NewLineItem = &CQueryDesigner::MLPutNewLine;
    Put.ClauseSep   = &CQueryDesigner::MLPutClauseSeparator;
    Put.Separator   = &CQueryDesigner::MLPutSeparator;
    int Len         = (int)m_Result.Len();
    wxTreeItemIdValue cookie;

     // FOR each child of Node
    for (wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
    {
        CQDUQuery *Query = (CQDUQuery *)m_Tree.GetItemData(Item);
        // IF prefix is not empty, append prefix and new line
        if (!Query->m_Prefix.IsEmpty())
        {
            MLPut(Query->m_Prefix);
            MLPutSeparator();
        }
        // Append open bracket
        MLPut(wxT("("));
        // Increment indent, append new line
        m_ResultLevel += m_Indent;
        MLPutSeparator();
        // IF query expression, build query expression ELSE build simple query
        if (Query->m_Type == TT_QUERYEXPR)
            BuildQueryExpr(Item);
        else
            BuildQuery(Item);
        // Decrement indent, append new line
        m_ResultLevel -= m_Indent;
        MLPutSeparator();
        // Append close bracket
        MLPut(wxT(")"));
        Len = (int)m_Result.Len();
        // Append new line
        MLPutSeparator();
    }
    CQDQueryTerm *Term = (CQDQueryTerm *)m_Tree.GetItemData(Node);
    // IF postfix is empty, truncate last new line
    wxString Postfix   = Term->m_Postfix.Strip(wxString::both);
    if (Postfix.IsEmpty())
        m_Result.Truncate(Len);
    // ELSE append postfix
    else
        (this->*Put.Item)(Postfix);
}
//
// Multiline output, simply appends Val to result
//
void CQueryDesigner::MLPut(const wxChar *Val)
{
    m_Result += Val;
}
//
// Singleline output, appends Val to result, if line is longer then max line length, throws exception
//
void CQueryDesigner::SLPut(const wxChar *Val)
{
    m_Result += Val;
    if (m_Result.Len() - m_LinePos > m_LineLen)
    {
        m_LineTooLong = true;
        throw 0;
    }
}
//
// Multiline output to new line if necessary
//
void CQueryDesigner::MLPutNewLine(const wxChar *Val)
{
    int rl = (int)(m_Result.Len() - m_LinePos);
    // IF current position > line bigining AND current position + Val length > max line length, append new line
    if (rl > m_ResultLevel && rl + wxStrlen(Val) > m_LineLen)
        MLPutSeparator();
    // Append Val
    m_Result += Val;
}
//
// Appends comma and space
//
void CQueryDesigner::PutComma()
{
    m_Result += wxT(", ");
}
//
// Multiline separator output, appends CR LF and string of spaces corresponding to current indent
//
void CQueryDesigner::MLPutSeparator()
{
    m_Result += wxT("\r\n");
    m_LinePos = (int)m_Result.Len();
    m_Result  = m_Result + wxString(wxT(' '), m_ResultLevel);
}
//
// Multiline clause separator output
//
void CQueryDesigner::MLPutClauseSeparator(int Count)
{
    // IF wrap OR wrap after clause, append new line
    if (m_WrapItems || m_WrapAfterClause)
    {
        m_Result += wxT("\r\n");
        m_LinePos = (int)m_Result.Len();
        m_Result  = m_Result + wxString(wxT(' '), m_ResultLevel);
    }
    // ELSE append Count spaces
    else
    {
        m_Result  = m_Result + wxString(wxT(' '), Count);
    }
}
//
// Singleline separator output, appends single space
//
void CQueryDesigner::SLPutSeparator()
{
    m_Result += wxT(' ');
}
//
// Singleline clause separator output, appends single space
//
void CQueryDesigner::SLPutClauseSeparator(int Count)
{
    m_Result += wxT(' ');
}

///////////////////////////////////////////// Event handlers ////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CQueryDesigner, wxSplitterWindow)
    EVT_KEY_DOWN(CQueryDesigner::OnKeyDown)
    EVT_TREE_ITEM_EXPANDING(QDID_TREE, CQueryDesigner::OnTreeExpanding)
    EVT_TREE_SEL_CHANGED(QDID_TREE, CQueryDesigner::OnTreeSelChanged)
    EVT_TREE_KEY_DOWN(QDID_TREE, CQueryDesigner::OnTreeKeyDown) 
    EVT_TREE_DELETE_ITEM(QDID_TREE, CQueryDesigner::OnDeleteItem) 
    EVT_TREE_BEGIN_DRAG(QDID_TREE,  CQueryDesigner::OnTreeBeginDrag) 
    EVT_GRID_COL_SIZE(CQueryDesigner::OnGridColSize)
    EVT_LIST_BEGIN_DRAG(QDID_TABLELIST, CQueryDesigner::OnListBeginDrag) 
    EVT_SPLITTER_SASH_POS_CHANGED(QDID_RIGHTPANEL, CQueryDesigner::OnHorzSplitterChanged)
    EVT_COMMAND(1, myEVT_ERRDEFIN, CQueryDesigner::OnErrDefin) 
    EVT_CHECKBOX(QDID_DISTINCT, CQueryDesigner::OnDistinct)
#ifdef LINUX    
    EVT_MOVE(CQueryDesigner::OnMove)
    EVT_TREE_ITEM_RIGHT_CLICK(QDID_TREE, CQueryDesigner::OnTreeRightDown) 
    EVT_RIGHT_UP(CQueryDesigner::OnTreeRightClick) 
#else	
    EVT_TREE_ITEM_RIGHT_CLICK(QDID_TREE, CQueryDesigner::OnTreeRightClick) 
    EVT_GRID_LABEL_LEFT_CLICK(CQueryDesigner::OnGridLabelLeftClick)
#endif    
    EVT_SIZE(CQueryDesigner::OnSize)
END_EVENT_TABLE()
//
// Key down event handler
//
void CQueryDesigner::OnKeyDown( wxKeyEvent& event )
{ 
    OnCommonKeyDown(event);
}
//
// Resize event handler
//
void CQueryDesigner::OnSize(wxSizeEvent & event)
{
    wxSize sz = event.GetSize();
    m_RightPanel.SetSashPosition(sz.y - m_TablesHeight, true);
    // Set delayed resize
    m_Resize = true;
    event.Skip();
}
//
// OnResize virtual function implementation
//
void CQueryDesigner::OnResize()
{
    int Width, Height;
#if WBVERS>=1000
    if (global_style==ST_MDI)
    {
        dsgn_mng_wnd->GetClientSize(&Width, &Height);
        SetSize(Width, Height);
        m_Edit.SetSize(Width, Height);
    }
#else
    dsgn_mng_wnd->GetClientSize(&Width, &Height);
    SetSize(Width, Height);
    m_Edit.SetSize(Width, Height);
#endif
}
//
// Horizontal splitter moved event handler
//
void CQueryDesigner::OnHorzSplitterChanged(wxSplitterEvent &event)
{
    int Width, Height;
    m_RightPanel.GetClientSize(&Width, &Height);
    if (Height)
        m_TablesHeight = Height - event.GetSashPosition();
}
//
// Grid column width changed event handler
//
void CQueryDesigner::OnGridColSize(wxGridSizeEvent &event)
{
    m_Grid->Resize();
    event.Skip();
}
//
// Tree item expanding event handler, selects expanding item
//
void CQueryDesigner::OnTreeExpanding(wxTreeEvent &event)
{
    m_Tree.SelectItem(event.GetItem());
}
//
// Selected tree item changed event handler
//
void CQueryDesigner::OnTreeSelChanged(wxTreeEvent &event)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId Par;
    wxTreeItemId Chld;
    wxTreeItemId Chld2;
    wxTreeItemId Item   = event.GetItem();
    if (!Item.IsOk())
        return;
    wxTreeItemId Node   = Item;
    bool         Sel    = false;
    int          GridId = m_Tree.GetItemImage(Item);
    int          ii     = GridId;
    
    switch (GridId)
    {
    case qtiQueryExpr:
        // IF more queries in query expression, show query expression grid
        Chld = m_Tree.GetFirstChild(Item, cookie);
        if (!Chld.IsOk())
            return;
        Chld2 = m_Tree.GetNextChild(Item, cookie);
        // IF single query in query expression
        if (!Chld2.IsOk() || m_Tree.GetItemImage(Chld2) == qtiOrderBy || m_Tree.GetItemImage(Chld2) == qtiLimit)
        {
            // IF Item == Root, show grid for first SELECT
            if (Item == m_Tree.GetRootItem())
            {
                GridId = qtiSelect;
                Node   = m_Tree.GetFirstChild(Chld, cookie);
            }
            else
            {
                Par    = m_Tree.GetItemParent(Item);
                GridId = m_Tree.GetItemImage(Par);
                // IF query expression parent is column or condition, show grid for first SELECT, grid for parent otherwise
                if (GridId == qtiColumn && GridId == qtiCond)
                {
                    GridId = qtiSelect;
                    Node   = m_Tree.GetFirstChild(Chld, cookie);
                }
                else
                {
                    Node = Par;
                    Sel  = true;
                }
            }
        }
        break;        
    case qtiQuery:
        Par    = m_Tree.GetItemParent(Item);
        GridId = m_Tree.GetItemImage(Par);
        // IF query parent is query expression
        if (GridId == qtiQueryExpr)
        {
            // IF query is single query in query expression, show grid for first SELECT
            Chld = m_Tree.GetNextSibling(Item);
            if (!m_Tree.GetPrevSibling(Item).IsOk() && (!Chld.IsOk() || m_Tree.GetItemImage(Chld) == qtiOrderBy || m_Tree.GetItemImage(Chld) == qtiLimit))
            {
                GridId = qtiSelect;
                Node   = m_Tree.GetFirstChild(Item, cookie);
            }
            // ELSE show grid for query expression
            else
            {
                Node   = Par;
                Sel    = true;
            }
        }
        // IF query parent is column or condition, show grid for first SELECT
        else if (GridId == qtiColumn || GridId == qtiCond)
        {
            GridId = qtiSelect;
            Node   = m_Tree.GetFirstChild(Item, cookie);
        }
        // ELSE show grid for parent
        else 
        {
            Node = Par;
            Sel  = true;
        }
        break;        
    case qtiColumn:
        GridId = qtiSelect;
        Node   = m_Tree.GetItemParent(Item);
        Sel    = true;
        break;
    case qtiTable:
        GridId = qtiFrom;
        Node   = m_Tree.GetItemParent(Item);
        Sel    = true;
        break;
    case qtiCond:
        GridId = qtiWhere;
        Node   = m_Tree.GetItemParent(Item);
        Sel    = true;
        break;
    case qtiSubConds:
        GridId = qtiWhere;
        break;
    case qtiGColumn:
        GridId = qtiGroupBy;
        Node   = m_Tree.GetItemParent(Item);
        Sel    = true;
        break;
    case qtiOColumn:
        GridId = qtiOrderBy;
        Node   = m_Tree.GetItemParent(Item);
        Sel    = true;
        break;
    }

    // Par = tree item for current query
    if (ii == qtiQueryExpr)
    {
        // IF parent of query expression is FROM or join, find parent query
        Par = m_Tree.GetItemParent(Item);
        if (Par.IsOk() && (m_Tree.GetItemImage(Par) == qtiFrom || m_Tree.GetItemImage(Par) == qtiJoin))
        {
            do
                Par = m_Tree.GetItemParent(Par);
            while (m_Tree.GetItemImage(Par) != qtiQuery);
        }
        // ELSE Par = first query in query expression
        else
        {
            wxTreeItemIdValue cookie;
            Par = m_Tree.GetFirstChild(Item, cookie);
        }
    }
    else if (ii == qtiQuery)
    {
        Par = m_Tree.GetItemParent(Item);
        ii = m_Tree.GetItemImage(Par);
        // IF parent of query is FROM or join, find parent query
        if (ii == qtiFrom || ii == qtiJoin)
        {
            do
                Par = m_Tree.GetItemParent(Par);
            while (m_Tree.GetItemImage(Par) != qtiQuery);
        }
        // ELSE Par = current item
        else
        {
            Par = Item;
        }
    }
    else if (ii == qtiOrderBy || ii == qtiLimit)
    {
        // Par = parent item
        Par = m_Tree.GetItemParent(Item);
        ii  = m_Tree.GetItemImage(Par);
        // IF parent item is query expression, Par = first query in query expression
        if (ii == qtiQueryExpr)
            Par = m_Tree.GetFirstChild(Par, cookie);
    }
    else if (ii == qtiOColumn)
    {
        // Par = gradfather
        Par = m_Tree.GetItemParent(Item);
        Par = m_Tree.GetItemParent(Par);
        ii  = m_Tree.GetItemImage(Par);
        // IF gradfather is query expression, Par = first query in query expression
        if (ii == qtiQueryExpr)
            Par = m_Tree.GetFirstChild(Par, cookie);
    }
    else
    {
        // Find parent query
        Par = Item;
        do
            Par = m_Tree.GetItemParent(Par);
        while (m_Tree.GetItemImage(Par) != qtiQuery);
    }
    // Show tables of current query 
    ShowTables(Par);
    // Show grid for current tree item
    GetGrid(GridId, Node);
    // IF grid of parent tree item
    if (Sel)
    {
        // Select current item row
        int r = IndexOf(Item);
        if (GridId == qtiQueryExpr &&  r >= m_Tree.GetChildrenCount(m_Tree.GetItemParent(Item), false) - 1)
            r--;
        m_Grid->Select(r, 0); 
        // IF FROM list grid, select current table on table panel 
        if (GridId == qtiFrom)
        {
            CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Item);
            m_Tables.Select(Table->m_TableBox);
        }
    }
}
//
// Shows grid for selected item
// Input:   GridId  - Grid type
//          Node    - Parent tree item
//
void CQueryDesigner::GetGrid(int GridId, const wxTreeItemId &Node)
{
    // IF grid exists
    if (m_Grid)
    {
        // Close grid cell editor
        m_Grid->DisableCellEditControl();
        // EXIT IF grid for same node
        if (m_Grid->m_Node == Node && m_Grid->GetId() == GridId)
            return;
        // Destroy old grid
        m_Grid->Destroy();
    }
    // Create new grid
    switch (GridId)
    {
    case qtiQueryExpr:
        m_Grid = new CQDQueryGrid(this);
        break;
    case qtiSelect:
        m_Grid = new CQDSelectGrid(this);
        break;
    case qtiFrom:
        m_Grid = new CQDFromGrid(this);
        break;
    case qtiJoin:
        m_Grid = new CQDJoinGrid(this);
        break;
    case qtiWhere:
    case qtiHaving:
        m_Grid = new CQDCondGrid(this);
        break;
    case qtiGroupBy:
        m_Grid = new CQDGroupGrid(this);
        break;
    case qtiOrderBy:
        m_Grid = new CQDOrderGrid(this);
        break;
    case qtiLimit:
        m_Grid = new CQDLimitGrid(this);
        break;
    }
    if (!m_Grid)
        no_memory();
    m_Grid->Open(Node);
}
//
// Shows tables for current query
//
void CQueryDesigner::ShowTables(const wxTreeItemId &Node)
{
    // EXIT IF current query not changed
    if (Node == m_CurQuery)
        return;
    wxTreeItemIdValue cookie;
    wxTreeItemId Item;
    
    // Save changes to column or condition subquery
    UpdateColQuery();
    // Clear tables panel
    m_Tables.Clear();
    m_CurQuery            = Node;
    m_CurQueryChanged     = false;
    m_ShowIndex           = false;
    wxTreeItemId FromNode = GetFromNode(Node);
    // FOR each child of FROM tree item, show corresponding TableBox
    for (Item = m_Tree.GetFirstChild(FromNode, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(FromNode, cookie))
    {
        CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Item);
        // Not meaningful, INDEX setting is not supported
        if (Table->m_Type == TT_TABLE && !((CQDDBTable *)Table)->m_Index.IsEmpty())
            m_ShowIndex = true;
        ShowTable(Item);
    }
    // Show join links
    ResetAttrLinks();
}
//
// Redraws links between joined tables in current query
//
void CQueryDesigner::ResetAttrLinks()
{
    wxTreeItemIdValue cookie;
    wxTreeItemId Item;
    wxTreeItemId FromNode = GetFromNode(m_CurQuery);
    // Clear old link list
    m_Tables.m_AttrLinks.clear();
    // FOR each table in current query, add 
    for (Item = m_Tree.GetFirstChild(FromNode, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(FromNode, cookie))
        AddAttrLink(Item);
    // IF joins are to be converted to WHERE condition
    if (m_JoinConv)
    {
        CQDQTable *DstTable;
        CQDQTable *SrcTable;
        wxChar     DstAttr[OBJ_NAME_LEN + 3];  
        wxChar     SrcAttr[OBJ_NAME_LEN + 3];  
        // IF table count in current query > 1
        if (m_Tree.GetChildrenCount(GetFromNode(m_CurQuery), false) > 1)
        {
            // Find WHERE item
            wxTreeItemId WhereNode = GetWhereNode(m_CurQuery);
            if (WhereNode.IsOk())
            {
                // FOR each condition on root level
                for (Item = m_Tree.GetFirstChild(WhereNode, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(WhereNode, cookie))
                {
                    // IF condition is join condition, show link
                    CQDCond *Cond = (CQDCond *)m_Tree.GetItemData(Item);
                    if (!Cond->m_SubConds && WhereCondToJoin(Cond->m_Expr, &DstTable, DstAttr, &SrcTable, SrcAttr))
                        AddAttrLink(DstTable->m_TableBox, DstAttr, SrcTable->m_TableBox, SrcAttr, Item);
                }
            }
        }
    }
    // Redraw tables panel
    m_Tables.Refresh();
}
//
// Parses WHERE condition, if it is diplayable join condition, returns source table records and column names 
// Input:   Cond     - WHERE condition
//          DstTable - Destination table record
//          DstAttr  - Destination column name
//          SrcTable - Source table record
//          SrcAttr  - Source column name
// Returns: true if Cond is diplayable join condition
//
bool CQueryDesigner::WhereCondToJoin(const wxChar *Cond, CQDQTable **DstTable, wxChar *DstAttr, CQDQTable **SrcTable, wxChar *SrcAttr)
{
    char dSchema[OBJ_NAME_LEN + 3];
    char sSchema[OBJ_NAME_LEN + 3];
    char dTable[OBJ_NAME_LEN + 3];
    char sTable[OBJ_NAME_LEN + 3];
    char dAttr[OBJ_NAME_LEN + 3];
    char sAttr[OBJ_NAME_LEN + 3];

    // Parse condition
    if (is_join_cond(cdp, wxString(Cond).mb_str(*cdp->sys_conv), dSchema, dTable, dAttr, sSchema, sTable, sAttr))
    {
        cdp->sys_conv->MB2WC(DstAttr, dAttr, OBJ_NAME_LEN + 3);
        cdp->sys_conv->MB2WC(SrcAttr, sAttr, OBJ_NAME_LEN + 3);
        // Find destination table record
        *DstTable = TableFromAliasAttr(GetNakedIdent(wxString(dSchema, *cdp->sys_conv)), GetNakedIdent(wxString(dTable, *cdp->sys_conv)), DstAttr);
        if (*DstTable)
        {
            // Find source table record
            *SrcTable = TableFromAliasAttr(GetNakedIdent(wxString(sSchema, *cdp->sys_conv)), GetNakedIdent(wxString(sTable, *cdp->sys_conv)), SrcAttr);
            if (*SrcTable)
                return true;
        }
    }
    return false;
}
//
// Searchs table record for table given by schema name, alias and column name in current query
//
CQDQTable *CQueryDesigner::TableFromAliasAttr(const wxChar *Schema, const wxChar *Alias, const wxChar *Attr)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId Node = GetFromNode(m_CurQuery);
    // Search all source tables in current query
    for (wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
    {
        CQDQTable *Result = TableFromAliasAttr(Schema, Alias, Attr, Item);
        if (Result)
            return Result;
    }
    return NULL;
}
//
// Searchs table record for table given by schema name, alias and column name in given Node
//
CQDQTable *CQueryDesigner::TableFromAliasAttr(const wxChar *Schema, const wxChar *Alias, const wxChar *Attr, const wxTreeItemId &Node)
{
    wxTreeItemIdValue cookie;
    // Get current table record
    CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Node);
    // IF Alias specified
    if (*Alias)
    {
        // IF Alias == current table alias, found
        if (wxStricmp(Alias, Table->m_Alias) == 0)
            return Table;
        // IF current table is database table
        else if (Table->m_Type == TT_TABLE)
        {
            const wxChar *ss = NULL;
            const wxChar *ds = ((CQDDBTable *)Table)->m_Schema;
            if (Schema && *Schema)
            {
                ss = Schema;
                if (!ds || !*ds)
                    ds = m_Schema;
            }
            else if (ds && *ds)
            {
                ss = m_Schema;
            }
            // IF Schema == current table schema && Alias == current table name, found
            if ((!ss || wxStricmp(ss, ds) == 0) && wxStricmp(Alias, ((CQDDBTable *)Table)->m_Name) == 0)
                return Table;
        }
        // Not found
        return NULL;
    }
    // IF current table is join
    if (Table->m_Type == TT_JOIN)
    {
        wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie);
        CQDQTable *Result = TableFromAliasAttr(Schema, Alias, Attr, Item);
        // IF Attr don't belongs to first join source table, try second join soure table
        if (!Result)
        {
            Item   = m_Tree.GetNextChild(Node, cookie);
            Result = TableFromAliasAttr(Schema, Alias, Attr, Item);
        }
        return Result;
    }
    // Alias not specified AND not join
    // IF Attr belongs to current table, found
    tccharptr aAttr = Tochar(Attr);
    if (!aAttr)
        return NULL;
    if (GetAttrDef(Node, aAttr))
        return Table;
    return NULL;
}
//
// Returns true if column AttrName belongs to table given by Node
//
bool CQueryDesigner::GetAttrDef(const wxTreeItemId &Node, const char *AttrName)
{
    tcateg Categ;
    wxString Source;
    CTableDef TabDef;
    CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Node);
    // IF Node is database table, Source = table name
    if (Table->m_Type == TT_TABLE)
    {
        Categ  = CATEG_TABLE;
        Source = GetTableName(Table, 0);
    }
    // ELSE build SQL statement text
    else
    {
        BuildUQuery(Node);
        Categ  = CATEG_DIRCUR;
        Source = m_Result;
    }
    struct t_clivar * hostvars; 
    // Fill clivar array
    unsigned hostvars_count = GetParams(&hostvars);
    // Get table definition
    TabDef.Create(cdp, Source.c_str(), Categ, hostvars, hostvars_count);
    const d_attr *Attr;
    int i;
    // Find column in table definition
    for (i = 1; (Attr = TabDef.GetAttr(i)) != NULL; i++)
    {
        if (wb_stricmp(cdp, AttrName, Attr->name) == 0)
            return true;
    }
    return false;
}
//
// If column or condition subquery changed, saves changes to column or condition definition
//
void CQueryDesigner::UpdateColQuery()
{
    // EXIT IF no change or current item is not column nor condition
    if (!m_CurQueryChanged)
        return;
    wxTreeItemId Parent = m_Tree.GetItemParent(m_CurQuery);
    int          Image  = m_Tree.GetItemImage(Parent);
    if (Image != qtiColumn && Image != qtiCond)
        return;
    TStatmStyle StatmStyle = m_StatmStyle;      // Save statement formating
    Put.MultiLine          = false;             // Set single line formating
    m_StatmStyle           = ssWrapNever; 
    m_LineLen              = SINGLELINE_LEN;
    m_LinePos              = 0;
    m_Result.Empty();
    BuildColQuery(Parent);                      // Build column/condition expression
    m_StatmStyle           = StatmStyle;        // Restore statement formating
    m_Tree.SetItemText(Parent, m_Result);
    // Store result to record
    CQDQueryTerm *Item     = (CQDQueryTerm *)m_Tree.GetItemData(Parent);
    if (Image != qtiColumn || ((CQDColumn *)Item)->m_Alias.IsEmpty())
        Item->m_Expr       = m_Result; 
}
//
// Shows TableBox for table given by Node
//
void CQueryDesigner::ShowTable(const wxTreeItemId &Node)
{
    CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Node);
    // IF table is database table, add table to table panel
    if (Table->m_Type == TT_TABLE)
        AddTable(GetTableName(Table, 0), CATEG_TABLE, GetTableName(Table, GTN_ALIAS | GTN_TITLE), Node);
    // IF table is join, show join source tables
    else if (Table->m_Type == TT_JOIN)
    {
        wxTreeItemIdValue cookie;
        wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie);
        ShowTable(Item);
        Item = m_Tree.GetNextChild(Node, cookie);
        ShowTable(Item);
    }
    // IF table is query OR query expression
    else if (Table->m_Type == TT_QUERY || Table->m_Type == TT_QUERYEXPR)
    {
        // IF query expression OR not empty query
        if (Table->m_Type == TT_QUERYEXPR || GetFromCount(Node) > 0)
        {
            // IF query has no columns, add * column
            if (Table->m_Type == TT_QUERY && GetColumnCount(Node) == 0)
                AddColumn(Node, wxT("*"), wxString());
            // Build statement
            BuildUQuery(Node);
            // Set statement as table source
            ((CQDUQuery *)Table)->m_Source = m_Result;
            // Add table to table panel
            AddTable(((CQDUQuery *)Table)->m_Source, CATEG_DIRCUR, GetTableName(Table, GTN_ALIAS | GTN_TITLE), Node);
        }
    }
}
#ifdef LINUX
//
// Tree right mouse button down event handler, selects given item
//
void CQueryDesigner::OnTreeRightDown(wxTreeEvent & event)
{
	wxTreeItemId Item = event.GetItem();
	if (Item.IsOk());
		m_Tree.SelectItem(Item);
}
//
// Tree right mouse button up event handler, shows popup menu for current item
//
void CQueryDesigner::OnTreeRightClick(wxMouseEvent & event)
{ 
	wxPoint Position = wxPoint(event.m_x, event.m_y);
  	int Flags;
  	wxTreeItemId Item = m_Tree.HitTest(Position, Flags);
  	// VK_APPS is translated to right mouse click: minimising the damage by limiting the positions of the mouse
  	if (Item.IsOk() && (Flags & (wxTREE_HITTEST_ONITEMBUTTON | wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL/*wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMRIGHT*/)))
    {
        m_Tree.SelectItem(Item);
		ShowPopupMenu(Item, Position);
    }
}

#else
//
// Tree right mouse button click event handler, shows popup menu for current item
//
void CQueryDesigner::OnTreeRightClick(wxTreeEvent & event)
{
 	wxPoint Position = ScreenToClient(wxGetMousePosition());
  	wxTreeItemId Item = event.GetItem();//HitTest(position, flags);
  	// VK_APPS is translated to right mouse click: minimising the damage by limiting the positions of the mouse
  	if (Item.IsOk())// && (flags & (wxTREE_HITTEST_ONITEMBUTTON | wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL/*wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMRIGHT*/)))
    {
        m_Tree.SelectItem(Item);
		ShowPopupMenu(Item, Position);
    }
}
//
// Grid row label left mouse button click event handler, selects given row
//
void CQueryDesigner::OnGridLabelLeftClick(wxGridEvent & event)
{
    ((wxWindow *)event.GetEventObject())->SetFocus();
    m_Grid->Select(event.GetRow(), 0);
    event.Skip();
}

#endif
//
// Popup menu descriptor for query expression
//
CMenuDesc mdQueryExpr[] =
{
    {TD_CPM_ADDORDERBY,   qdOrderBy_xpm,     _("Add ORDER BY")},
    {TD_CPM_ADDLIMIT,     qdLimit_xpm,       _("Add LIMIT")},
    {TD_CPM_ADDQUERY,     qdQuery_xpm,       _("Add query")},
    //{TD_CPM_ADDQUERYEXPR, qdQueryExpr_xpm,   _("Add query expression")},
    {wxID_SEPARATOR},
    {TD_CPM_DELQUERYEXPR, qdQueryExpr_xpm,   _("Delete query expression")},
    {wxID_SEPARATOR},
    {TD_CPM_CHECK,        validateSQL_xpm,   _("Validate")},
    {TD_CPM_DATAPREVIEW,  querypreview_xpm,  _("Data preview")},
    {0}
};
//
// Popup menu descriptor for simple query
//
CMenuDesc mdQuery[] =
{
    {TD_CPM_ADDWHERE,     qdWhere_xpm,       _("Add WHERE")},
    {TD_CPM_ADDGROUPBY,   qdGroupBy_xpm,     _("Add GROUP BY")},
    {TD_CPM_ADDHAVING,    qdHaving_xpm,      _("Add HAVING")},
    {TD_CPM_ADDORDERBY,   qdOrderBy_xpm,     _("Add ORDER BY")},
    {TD_CPM_ADDLIMIT,     qdLimit_xpm,       _("Add LIMIT")},
    {wxID_SEPARATOR},
    {TD_CPM_DELQUERY,     qdQuery_xpm,       _("Delete query")},
    {wxID_SEPARATOR},
    {TD_CPM_CHECK,        validateSQL_xpm,   _("Validate")},
    {TD_CPM_DATAPREVIEW,  querypreview_xpm,  _("Data preview")},
    {0}
};
//
// Popup menu descriptor for SELECT
//
CMenuDesc mdSelect[] =
{
    {TD_CPM_ADDQUERY,     qdQuery_xpm,       _("Add subquery")},
    {0}
};
//
// Popup menu descriptor for FROM
//
CMenuDesc mdFrom[] =
{
    {TD_CPM_ADDTABLE,     ta_xpm,            _("Add table")},
    {TD_CPM_ADDQUERY,     qdQuery_xpm,       _("Add subquery")},
    {TD_CPM_ADDQUERYEXPR, qdQueryExpr_xpm,   _("Add query expression")},
    {TD_CPM_ADDJOIN,      qdJoin_xpm,        _("Add join")},
    {0}
};
//
// Popup menu descriptor for WHERE
//
CMenuDesc mdWhere[] =
{
    {TD_CPM_ADDSUBCOND,   qdCond_xpm,        _("Add subcondition")},
    {TD_CPM_ADDQUERY,     qdQuery_xpm,       _("Add subquery")},
    {wxID_SEPARATOR},
    {TD_CPM_DELWHERE,     qdWhere_xpm,       _("Delete WHERE")},
    {0}
};
//
// Popup menu descriptor for GROUP BY
//
CMenuDesc mdGroupBy[] =
{
    {TD_CPM_DELGROUPBY,   qdGroupBy_xpm,     _("Delete GROUP BY")},
    {0}
};
//
// Popup menu descriptor for HAVING
//
CMenuDesc mdHaving[] =
{
    {TD_CPM_ADDSUBCOND,   qdCond_xpm,        _("Add subcondition")},
    {TD_CPM_ADDQUERY,     qdQuery_xpm,       _("Add subquery")},
    {wxID_SEPARATOR},
    {TD_CPM_DELHAVING,    qdHaving_xpm,      _("Delete HAVING")},
    {0}
};
//
// Popup menu descriptor for ORDER BY
//
CMenuDesc mdOrderBy[] =
{
    {TD_CPM_DELORDERBY,   qdOrderBy_xpm,     _("Delete ORDER BY")},
    {0}
};
//
// Popup menu descriptor table
//
CMenuDesc mdTable[] =
{
    {TD_CPM_DELETETABLE,  td_xpm,            _("Delete table")},
    //{TD_CPM_ADDINDEX,     qdOrderBy_xpm,     _("Add INDEX")},     // NOT supported
    {0}
};
//
// Popup menu descriptor for join
//
CMenuDesc mdJoin[] =
{
    {TD_CPM_DELJOIN,      qdJoin_xpm,        _("Delete join")},
    {0}
};
//
// Popup menu descriptor for column
//
CMenuDesc mdColumn[] =
{
    {TD_CPM_ADDQUERY,     qdQuery_xpm,       _("Add subquery")},
    {wxID_SEPARATOR},
    {TD_CPM_DELCOLUMN,    qdColumn_xpm,      _("Delete column")},
    {0}
};
//
// Popup menu descriptor for condition
//
CMenuDesc mdCond[] =
{
    {TD_CPM_ADDQUERY,     qdQuery_xpm,       _("Add subquery")},
    {wxID_SEPARATOR},
    {TD_CPM_DELCOND,      qdCond_xpm,        _("Delete condition")},
    {0}
};
//
// Popup menu descriptor for subcondition set
//
CMenuDesc mdSubConds[] =
{
    {TD_CPM_ADDSUBCOND,   qdCond_xpm,        _("Add subcondition")},
    {TD_CPM_ADDQUERY,     qdQuery_xpm,       _("Add subquery")},
    {wxID_SEPARATOR},
    {TD_CPM_DELCOND,      qdCond_xpm,        _("Delete condition")},
    {0}
};
//
// Popup menu descriptor for ORDER BY/GROUP BY expression
//
CMenuDesc mdExpr[] =
{
    {TD_CPM_DELEXPR,      qdColumn_xpm,      _("Delete expression")},
    {0}
};
//
// Popup menu descriptor for LIMIT
//
CMenuDesc mdLimit[] =
{
    {TD_CPM_DELLIMIT,     qdColumn_xpm,      _("Delete LIMIT")},
    {0}
};
//
// Popup menu descriptor for subquery
//
CMenuDesc mdSubQuery[] =
{
    {TD_CPM_ADDQUERY,     qdQuery_xpm,       _("Add query")},
    {wxID_SEPARATOR},
    {TD_CPM_DELQUERYEXPR, qdQueryExpr_xpm,   _("Delete query expression")},
    {wxID_SEPARATOR},
    {TD_CPM_CHECK,        validateSQL_xpm,   _("Validate")},
    {TD_CPM_DATAPREVIEW,  querypreview_xpm,  _("Data preview")},
    {0}
};

CMenuDesc *MenuTB[] =
{
    mdQueryExpr, mdQuery, mdSelect, mdFrom, mdWhere, mdGroupBy, mdHaving, mdOrderBy, mdTable, mdJoin, mdColumn, mdCond, mdSubConds, mdExpr, mdExpr, mdLimit, mdSubQuery
};
//
// Shows popup menu for Item
//
void CQueryDesigner::ShowPopupMenu(const wxTreeItemId &Item, const wxPoint &Position)
{
    wxMenu Menu;
    // Get type of item
    int Image = m_Tree.GetItemImage(Item);
    // IF item is query expression but not root query, type is subquery
    if (Image == qtiQueryExpr && Item != m_Tree.GetRootItem())
        Image = qtiLast + 1;
    for (CMenuDesc *MenuDesc = MenuTB[Image]; MenuDesc->m_ID; MenuDesc++)
    {
        if (MenuDesc->m_ID == wxID_SEPARATOR)
            Menu.AppendSeparator();
        else
        {
            bool Enb = true;
            switch (MenuDesc->m_ID)
            {
            case TD_CPM_ADDORDERBY:
                // IF statement contains ORDER BY clause, disable Add ORDER BY menu item
                Enb = !GetOrderByNode(Item).IsOk();
                break;
            case TD_CPM_ADDLIMIT:
                // IF statement contains LIMIT clause, disable Add LIMIT menu item
                Enb = !GetLimitNode(Item).IsOk();
                break;
            case TD_CPM_DELQUERY:
                // IF parent is root query expression and item is last query, disable Remove query menu item
                Enb = m_Tree.GetItemParent(Item) != m_Tree.GetRootItem() || IndexOf(Item) > 0;
                break;
            case TD_CPM_DELQUERYEXPR:
                // IF parent is root query expression, disable Remove query expression menu item
                Enb = Item != m_Tree.GetRootItem();
                break;
            case TD_CPM_ADDWHERE:
                // IF current query contains WHERE clause, disable Add WHERE menu item
                Enb = !GetWhereNode(Item).IsOk();
                break;
            case TD_CPM_ADDGROUPBY:
                // IF current query contains GROUP BY clause, disable Add GROUP BY menu item
                Enb = !GetGroupByNode(Item).IsOk();
                break;
            case TD_CPM_ADDHAVING:
                // IF current query contains HAVING clause, disable Add HAVING menu item
                Enb = !GetHavingNode(Item).IsOk() && GetGroupByNode(Item).IsOk();
                break;
            case TD_CPM_ADDINDEX:
                // IF not supported
                Enb = !m_ShowIndex;
                break;
            }
            AppendXpmItem(&Menu, MenuDesc->m_ID, wxGetTranslation(MenuDesc->m_Text), MenuDesc->m_Icon, Enb);
        }
    }
    // Show menu
    m_FromPopup = true;
    PopupMenu(&Menu, Position);
    m_FromPopup = false;
}
//
// Tree key down event handler
//
void CQueryDesigner::OnTreeKeyDown(wxTreeEvent &event)
{
    // IF Delete, delete item
    if (event.GetKeyCode() == WXK_DELETE)
        DeleteItem(m_Tree.GetSelection());
    // IF Menu, show popup menu
#ifdef WINS
    else if (event.GetKeyCode() == WXK_WINDOWS_MENU)
#else
    else if (event.GetKeyCode() == WXK_MENU)
#endif       
    {
		wxTreeItemId Item = m_Tree.GetSelection();
		if (Item.IsOk())
		{
			int Flags;
			wxPoint Position;
			wxRect Rect = m_Tree.GetClientRect();
			Position.x = 0;
			for (Position.y = 0; Position.y < Rect.height && m_Tree.HitTest(Position, Flags) != Item; Position.y += 10);
			if (Position.y >= Rect.height)
				Position.y = 40;
			Position.x = 60;
            ShowPopupMenu(Item, Position);
        }
    }
    else 
        event.Skip();
}
//
// Delete design item event handler
//
void CQueryDesigner::OnDeleteItem(wxTreeEvent &event)
{
#ifdef LINUX
    // On Linuxu occurs event also in case when tree is destroyed, but in this time tables panel does not exist
    if (GetParent()->IsBeingDeleted())
        return;
#endif        
    wxTreeItemId Item  = event.GetItem();
    int          Image = m_Tree.GetItemImage(Item);
    // IF table, query or query expression, delete corresponding TableBox
    if (Image == qtiTable || Image == qtiQuery || Image == qtiQueryExpr)
    {
        CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Item);
        CQDTableBox *Box = Table->m_TableBox;
        if (Box)
            m_Tables.DeleteTable(Box);
    }
    // IF condition or join, delete corresponding link
    else if (Image == qtiCond || Image == qtiJoin)
    {
        CAttrLink *AttrLink = m_Tables.FindAttrLink(Item);
        if (AttrLink)
            m_Tables.m_AttrLinks.delet(m_Tables.m_AttrLinks.find(AttrLink));
    }
}
//
// Tree item begin drag event handler
//
void CQueryDesigner::OnTreeBeginDrag(wxTreeEvent &event)
{
#ifdef WINS
    wxPoint Pos = event.GetPoint();
#else
    // On linux event raises also in case when click is out side any item
    wxPoint Pos = wxGetMousePosition();
    Pos         = m_Tree.ScreenToClient(Pos);
    int Flags;
    // Check if click is on item
    m_Tree.HitTest(Pos, Flags);
    if (!(Flags & wxTREE_HITTEST_ONITEM))
        return;
#endif        
    
    wxTreeItemId Item  = event.GetItem();
    int          Image = m_Tree.GetItemImage(Item);
    // EXIT IF not dragable item
    if (Image == qtiSelect || Image == qtiFrom || Image == qtiWhere || Image == qtiGroupBy || Image == qtiHaving || Image == qtiOrderBy || Image == qtiLimit)
        return;
    // Query expression, query, table and join can be dragged only if it is child of query expression or source table list
    if (Image == qtiQueryExpr || Image == qtiQuery || Image == qtiTable || Image == qtiJoin)
    {
        wxTreeItemId pItem  = m_Tree.GetItemParent(Item);
        if (!pItem.IsOk())
            return;
        int          pImage = m_Tree.GetItemImage(pItem);
        if (pImage != qtiQueryExpr && pImage != qtiFrom)
            return;
    }
    // Create DataObject
    m_Tree.SelectItem(Item);
    CQDDataObjectI *Data = new CQDDataObjectI(Item, Image);
    if (!Data)
    {
        no_memory();
        return;
    }
    // wx does not enable to DropTarget discover what is in the DataObject and corectly answer if draged 
    // data are acceptable or not, so I store DataObject to static variable, where DropTarget could it find
    CQDDataObjectI::CurData = Data;
    wxRect Rect;
    m_Tree.GetBoundingRect(Item, Rect, true);
#ifdef WINS
    Rect.x -= PatchedGetMetric(wxSYS_SMALLICON_X) + 3;
#else
    Rect.x += PatchedGetMetric(wxSYS_SMALLICON_X) - 10;
    Rect.y += 4;
#endif    
    Pos -= Rect.GetPosition();
    
#ifdef WINS
    // Create drag image, start image drag
    CWBDragImage di(m_Tree, Item);
    if (di.BeginDrag(Pos, this, true))
    {
        di.Show();
        di.Move(event.GetPoint());
    }
#endif    
    // Create drag source
    CQDDragSource ds(this, Data);
    // Do DragAndDrop operation
    wxDragResult Res = ds.DoDragDrop(wxDrag_AllowMove);
#ifdef WINS
    // Stop image drag
    di.EndDrag();
#endif    
}
//
// Table column begin drag event handler
// 
void CQueryDesigner::OnListBeginDrag(wxListEvent &event)
{
    wxListCtrl  *List = (wxListCtrl *)event.GetEventObject();
    CQDTableBox *Box  = (CQDTableBox *)List->GetParent();
#ifdef WINS    
    long         Item = event.GetIndex();
#else
    long         Item = List->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
#endif
    if (Item == -1)
        return;
    // Create DataObject with selected column
    CQDDataObjectE *Data = new CQDDataObjectE((CQDQTable *)m_Tree.GetItemData(Box->m_Node), List->GetItemText(Item), Item, (uns8)List->GetItemData(Item), List);
    if (!Data)
    {
        no_memory();
        return;
    }
    // wx does not enable to DropTarget discover what is in the DataObject and corectly answer if draged 
    // data are acceptable or not, so I store DataObject to static variable, where DropTarget could it find
    CQDDataObjectE::CurData = Data;
    wxRect Rect;
    List->GetItemRect(Item, Rect, wxLIST_RECT_LABEL);
    wxPoint Pos = event.GetPoint() - Rect.GetPosition();
    
#ifdef WINS    
    // Create drag image, start image drag
    CWBDragImage di(*List, Item);
    if (di.BeginDrag(Pos, this, true))
    {
        di.Show();
        di.Move(event.GetPoint());
    }
#endif // WINS
    
    // Create DragSource
    CQDDragSource ds(this, Data);
    // Do DragAndDrop operation
    wxDragResult Res = ds.DoDragDrop(wxDrag_AllowMove);
#ifdef WINS    
    // Stop drag image
    di.EndDrag();
#endif
}
//
// DISTINCT check box checked even handler, sets DISTINCT flag in record
//
void CQueryDesigner::OnDistinct(wxCommandEvent &event)
{
    CQDQuery *Query   = (CQDQuery *)m_Tree.GetItemData(m_CurQuery);
    Query->m_Distinct = event.IsChecked();
    changed = true;
}

#ifdef LINUX

void CQueryDesigner::OnMove(wxMoveEvent &event)
{
	event.Skip();
}

#endif
//
// Idle event handler
//
void CQueryDesigner::OnInternalIdle()
{
    // IF delayed deleting, delete item
    if (m_DeletedItem.IsOk())
    {
        DeleteItem(m_DeletedItem);
        m_DeletedItem.Unset();
    }
    // IF delayed resize
    if (m_Resize)
    {
        // IF vertical SashPosition not set
        if (m_SplitVert)
        {
            int Width, Height;
            // Set horizontal SashPosition
            m_RightPanel.GetClientSize(&Width, &Height);
            m_RightPanel.SetSashPosition(Height - m_TablesHeight, true);
            // Set vertical SashPosition
            if (GetSashPosition() < m_SplitVert)
                SetSashPosition(m_SplitVert);
            m_SplitVert = 0;
        }
        m_Resize = false;
    }
}
//
// Initial query compilation failed event handler
// Initial compilation is provided before designer window is created
// Error message should be shown when designer window is wisible
//
void CQueryDesigner::OnErrDefin(wxCommandEvent &event)
{
    if (m_ShowErr)
    {
        // Show error message
        client_error(cdp, m_ShowErr);  
        cd_Signalize2(cdp, dialog_parent());
        // Set cursor to error position
        m_Edit.position_on_line_column(cdp->RV.comp_err_line, cdp->RV.comp_err_column);
        m_ShowErr = 0;
    }
}

////////////////////////////////////////// Menu/toolbar actions /////////////////////////////////////////////

//
// Command event handler
//
void CQueryDesigner::OnCommand(wxCommandEvent & event)
{ 
    switch (event.GetId())
    { 
    case TD_CPM_CUT:
        if (m_Edit.IsShown())
            m_Edit.OnCut();
        break;
    case TD_CPM_COPY:
        if (m_Edit.IsShown())
            m_Edit.OnCopy();
        break;
    case TD_CPM_PASTE:
        if (m_Edit.IsShown())
            m_Edit.OnPaste();
        break;
    case TD_CPM_UNDO:
        if (m_Edit.IsShown())
            m_Edit.undo_action();
        break;
    case TD_CPM_REDO:
        if (m_Edit.IsShown())
            m_Edit.redo_action();
        break;
    case TD_CPM_SAVE:
        if (!IsChanged())
            break;  // no message if no changes
        if (save_design(false))
            info_box(_("Your design has been saved."), dialog_parent());
        break;
    case TD_CPM_SAVE_AS:
        if (save_design(true))
            info_box(_("Your design was saved under a new name."), dialog_parent());
        break;
    case TD_CPM_EXIT:  // closing by command (may be cancelled)
    case TD_CPM_EXIT_FRAME:
        if (m_Grid)
            m_Grid->EnableCellEditControl(false);
        if (exit_request(this, true))
            destroy_designer(GetParent(), &querydsng_coords);  // must not call Close, Close is directed here
        break;
    case TD_CPM_EXIT_UNCOND:  // closing by global event (cannot be cancelled)
        m_Grid->EnableCellEditControl(false);
        exit_request(this, false);
        destroy_designer(GetParent(), &querydsng_coords);  // must not call Close, Close is directed here
        break;
    case TD_CPM_SQL:
        OnTextEdit(event);
        break;
    case TD_CPM_CHECK:
        OnSyntaxCheck();
        break;
    case TD_CPM_ELEMENTPOPUP:
        ShowElementPopup();
        break;
    case TD_CPM_ADDQUERYEXPR:
        AddQueryExpr();
        break;
    case TD_CPM_DELQUERYEXPR:
    case TD_CPM_DELQUERY:
        DeleteQuery();
        break;
    case TD_CPM_ADDQUERY:   
        AddQuery(m_Tree.GetSelection());
        break;
    case TD_CPM_DELCOLUMN:
        DeleteColumn(m_Tree.GetSelection());
        break;
    case TD_CPM_ADDTABLE:
        AddTable();
        break;
    case TD_CPM_DELETETABLE:
        DeleteTable();
        break;
    case TD_CPM_ADDJOIN:
        AddJoin();
        break;
    case TD_CPM_DELJOIN:
        DeleteJoin(m_Tree.GetSelection());
        break;
    case TD_CPM_ADDWHERE:
        AddWhere();
        break;
    case TD_CPM_DELWHERE:   
        DeleteWhere();
        break;
    case TD_CPM_ADDSUBCOND:
        AddSubCond();
        break;
    case TD_CPM_DELCOND:
        DeleteCond(m_Tree.GetSelection());
        break;
    case TD_CPM_ADDGROUPBY:
        AddGroupBy();
        break;
    case TD_CPM_DELGROUPBY:
        DeleteGroupBy();
        break;
    case TD_CPM_ADDHAVING:
        AddHaving();
        break;
    case TD_CPM_DELHAVING:
        DeleteHaving();
        break;
    case TD_CPM_DELEXPR:
        DeleteExpr(m_Tree.GetSelection());
        break;
    case TD_CPM_ADDORDERBY:
        AddOrderBy();
        break;
    case TD_CPM_DELORDERBY:
        DeleteOrderBy();
        break;
    case TD_CPM_ADDLIMIT:
        AddLimit();
        break;
    case TD_CPM_DELLIMIT:
        DeleteLimit(m_Tree.GetSelection());
        break;
    case TD_CPM_DELETELINK:
    {
        CAttrLink *AttrLink = m_Tables.m_SelAttrLink;
        DeleteAttrLink(AttrLink->m_Node);
        break;
    }
    case TD_CPM_DATAPREVIEW:
        OnDataPreview();
        break;
    case TD_CPM_OPTIMIZE:
        OnOptimize();
        break;
    case TD_CPM_DSGN_PARAMS:
        OnSetDesignerProps();
        break;
    case TD_CPM_ADDINDEX:
        // Not supported
        m_ShowIndex = true;
        m_Grid->ForceRefresh();
        m_Grid->Select(IndexOf(m_Tree.GetSelection()), 2);
        m_Grid->Update();
        m_Grid->EnableCellEditControl();
        break;
    case TD_CPM_HELP:
        wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_query.html"));
        break;
    case TD_CPM_XML_PARAMS:
    { 
        if (m_Grid)
            m_Grid->DisableCellEditControl();
        XMLParamsDlg dlg(this);
        // wxTAB_TRAVERSAL causes the dialog to grab Return key and makes impossible to edit in the grid!!
        dlg.Create(dialog_parent(), -1, _("Query design parameters"));
        dlg.SetWindowStyleFlag(dlg.GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);
        dlg.ShowModal();
        break;
    }
    }
}
//
// Deletes design item
//
void CQueryDesigner::DeleteItem(const wxTreeItemId &Item)
{
    int Image = m_Tree.GetItemImage(Item);
    switch (Image)
    {
    case qtiQueryExpr:
        if (Item != m_Tree.GetRootItem())
            DeleteQuery();
        break;
    case qtiQuery:
        if (m_Tree.GetItemParent(Item) != m_Tree.GetRootItem() || IndexOf(Item) > 0)
            DeleteQuery();
        break;
    case qtiWhere: 
        DeleteWhere();
        break;
    case qtiGroupBy: 
        DeleteGroupBy();
        break;
    case qtiHaving:
        DeleteHaving(); 
        break;
    case qtiOrderBy: 
        DeleteOrderBy();
        break;
    case qtiTable: 
        DeleteTable(Item);
        break;
    case qtiJoin:
        DeleteJoin(Item);
        break;
    case qtiColumn:
        DeleteColumn(Item);
        break;
    case qtiCond:
    case qtiSubConds:
        DeleteCond(Item);
        break;
    case qtiGColumn:
    case qtiOColumn:
        DeleteExpr(Item);
        break;
    case qtiLimit:
        DeleteLimit(Item);
        break;
    }
}
//
// Add query expression action
//
void CQueryDesigner::AddQueryExpr()
{
    wxTreeItemId Last;
    wxTreeItemId Parent = m_Tree.GetSelection();
    int          Image  = m_Tree.GetItemImage(Parent);
  
    // IF parent is query expression, find last item before ORDER BY and LIMIT
    if (Image == qtiQueryExpr)
    {
        Last = m_Tree.GetLastChild(Parent);
        if (Last.IsOk())
        {
            Image = m_Tree.GetItemImage(Last);
            if (Image == qtiLimit)
            {
                Last  = m_Tree.GetPrevSibling(Last);
                Image = m_Tree.GetItemImage(Last);
            }
            if (Image == qtiOrderBy)
            {
                Last  = m_Tree.GetPrevSibling(Last);
            }
        }
    }
    // IF Parent is SELECT, create new column, set it as new query expression parent 
    else if (Image == qtiSelect)
    {
        CQDColumn *Column = new CQDColumn();
        if (!Column)
        {
            no_memory();
            return;
        }
        Parent = m_Tree.AppendItem(Parent, wxString(), qtiColumn, qtiColumn, Column);
    }
    // IF Parent is WHERE, condition set or HAVING
    else if (Image == qtiWhere || Image == qtiSubConds || Image == qtiHaving)
    {
        // Create new condition
        CQDCond *Cond = new CQDCond(false);
        if (!Cond)
        {
            no_memory();
            return;
        }
        // IF parent has children, find last child, if last association not set, set AND
        if (m_Tree.GetChildrenCount(Parent, false) > 0)
        {
            wxTreeItemId Prev = m_Tree.GetLastChild(Parent);
            CQDCond    *pCond = (CQDCond *)m_Tree.GetItemData(Prev);
            if (pCond->m_Assoc == CA_NONE)
            {
                pCond->m_Assoc = CA_AND;
                m_Tree.SetItemText(Prev, pCond->m_Expr + wxT(" - AND"));
            }
        }
        // Append condition and set it as new query expression parent 
        Parent = m_Tree.AppendItem(Parent, wxString(), qtiCond, qtiCond, Cond);
    }
    // Create new query expression record
    CQDQueryExpr *QueryExpr = new CQDQueryExpr();
    if (!QueryExpr)
    {
        no_memory();
        return;
    }
    wxTreeItemId Item;
    // IF last child not found, append new item to parent, set record as ItemData
    if (!Last.IsOk())
        Item = m_Tree.AppendItem(Parent, GetTableName(QueryExpr, GTN_ALIAS | GTN_TITLE), qtiQueryExpr, qtiQueryExpr, QueryExpr);
    else
    {
        // ELSE insert new item after last, set record as ItemData
        Item = m_Tree.InsertItem(Parent, Last, GetTableName(QueryExpr, GTN_ALIAS | GTN_TITLE), qtiQueryExpr, qtiQueryExpr, QueryExpr);
        Image = m_Tree.GetItemImage(Parent);
        // IF Parent is query expression, set assotiation between last and new item as UNION
        if (Image == qtiQueryExpr)
        {
            QueryExpr          = (CQDQueryExpr *)m_Tree.GetItemData(Last);
            QueryExpr->m_Assoc = QA_UNION;
            wxString Caption   = m_Tree.GetItemText(Last);
            Caption           += wxT(" - UNION");
            m_Tree.SetItemText(Last, Caption);
        }
    }
    // Add empty query to new expression
    AddQuery(Item);
    // Expand new item
    m_Tree.Expand(Item);
    // IF query expression grid is shown, redraw it
    if (m_Grid->GetId() == qtiQueryExpr)
        m_Grid->Refresh();
    changed = true;
}
//
// Add new query action
//
void CQueryDesigner::AddQuery(wxTreeItemId Node)
{
    wxTreeItemId Last;
    int Image = m_Tree.GetItemImage(Node);
    // IF parent is query expression, find last item before ORDER BY and LIMIT
    if (Image == qtiQueryExpr)
    {
        Last = m_Tree.GetLastChild(Node);
        if (Last.IsOk())
        {
            Image = m_Tree.GetItemImage(Last);
            if (Image == qtiLimit)
            {
                Last  = m_Tree.GetPrevSibling(Last);
                Image = m_Tree.GetItemImage(Last);
            }
            if (Image == qtiOrderBy)
            {
                Last  = m_Tree.GetPrevSibling(Last);
            }
        }
    }
    // IF parent is SELECT, create new column, set it as new query parent
    else if (Image == qtiSelect)
    {
        CQDColumn *Column = new CQDColumn();
        if (!Column)
        {
            no_memory();
            return;
        }
        Node = m_Tree.AppendItem(Node, wxString(), qtiColumn, qtiColumn, Column);
    }
    // IF Parent is WHERE, condition set or HAVING
    else if (Image == qtiWhere || Image == qtiSubConds || Image == qtiHaving)
    {
        // Create new condition
        CQDCond *Cond = new CQDCond(false);
        if (!Cond)
        {
            no_memory();
            return;
        }
        // IF parent has children, find last child, if last association not set, set AND
        if (m_Tree.GetChildrenCount(Node, false) > 0)
        {
            wxTreeItemId Prev = m_Tree.GetLastChild(Node);
            CQDCond    *pCond = (CQDCond *)m_Tree.GetItemData(Prev);
            if (pCond->m_Assoc == CA_NONE)
            {
                pCond->m_Assoc = CA_AND;
                m_Tree.SetItemText(Prev, pCond->m_Expr + wxT(" - AND"));
            }
        }
        // Append condition and set it as new query expression parent 
        Node = m_Tree.AppendItem(Node, wxString(), qtiCond, qtiCond, Cond);
    }
    // Create new query record
    CQDQuery *Query = new CQDQuery();
    if (!Query)
    {
        no_memory();
        return;
    }
    Image = m_Tree.GetItemImage(Node);
    wxTreeItemId Item;
    // IF parent is column
    if (Image == qtiColumn)
    {
        wxString Pref;
        int Cnt = (int)m_Tree.GetChildrenCount(Node, false);
        CQDColumn *Col = (CQDColumn *)m_Tree.GetItemData(Node);
        // IF column has no subqueries, set column definition as new query prefix, clear column definition
        if (!Cnt)
        {
            Pref = Col->m_Expr;
            Col->m_Expr.Clear();
        }
        // ELSE set column postfix as new query prefix, clear colump postfix
        else
        {
            Pref = Col->m_Postfix;
            Col->m_Postfix.Clear();
        }
        Query->m_Prefix = Pref;
    }
    // IF parent is condition
    else if (Image == qtiCond)
    {
        wxString Pref;
        wxTreeItemIdValue cookie;
        // Find subquery in condition
        for (Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Item, cookie))
        {
            Image = m_Tree.GetItemImage(Item);
            if (Image == qtiQuery || Image == qtiQueryExpr)
                break;
        }
        CQDCond *Cond = (CQDCond *)m_Tree.GetItemData(Node);
        // IF condition has subqueries, set condition postfix as new query prefix, clear condition postfix
        if (Image == qtiQuery || Image == qtiQueryExpr)
        {
            Pref = Cond->m_Postfix;
            Cond->m_Postfix.Clear();
        }
        // ELSE set condition definition as query prefix, clear condition definition
        else
        {
            Pref = Cond->m_Expr;
            Cond->m_Expr.Clear();
        }
        Query->m_Prefix = Pref;
    }

    // IF Last item not foun, append new item to parent, set record as ItemData
    if (!Last.IsOk())
        Item = m_Tree.AppendItem(Node, GetTableName(Query, GTN_ALIAS | GTN_TITLE), qtiQuery, qtiQuery, Query);
    else
    {
        // Insert new item after last item, set record as ItemData
        Item  = m_Tree.InsertItem(Node, Last, GetTableName(Query, GTN_ALIAS | GTN_TITLE), qtiQuery, qtiQuery, Query);
        Image = m_Tree.GetItemImage(Node);
        // IF parent is query expression, set association between last and new query as UNION
        if (Image == qtiQueryExpr)
        {
            CQDQueryExpr *Expr =(CQDQueryExpr *)m_Tree.GetItemData(Last);
            Expr->m_Assoc      = QA_UNION;
            wxString Caption   = m_Tree.GetItemText(Last);
            Caption           += wxT(" - UNION");
            m_Tree.SetItemText(Last, Caption);
        }
    }
    // Append SELECT and FROM to query
    m_Tree.AppendItem(Item,  wxT("SELECT"), qtiSelect);
    m_Tree.AppendItem(Item,  wxT("FROM"),   qtiFrom);
    // Select and expand new item
    m_Tree.SelectItem(Item);
    m_Tree.Expand(Item);
    changed = true;
}
//
// Delete query action
//
void CQueryDesigner::DeleteQuery()
{
    wxTreeItemId Item  = m_Tree.GetSelection();
    CQDQTable   *Query = (CQDQTable *)m_Tree.GetItemData(Item);
    // Ask user
    if (!yesno_boxp(_("Do you realy want to delete the query %s?"), dialog_parent(), GetTableName(Query, GTN_ALIAS | GTN_TITLE).c_str()))
        return;
    wxTreeItemId Parent;
    int Image;
    // Find parent query expression, column, condition or other element in which Item is not last child
    for (;;)
    {
        Parent = m_Tree.GetItemParent(Item);
        Image  = m_Tree.GetItemImage(Parent);
        if (Image != qtiQueryExpr && Image != qtiColumn && Image != qtiCond)
            break;
        int cc = (int)m_Tree.GetChildrenCount(Parent, false);
        if (cc > 1)
            break;
        Item = Parent;
    }
    // IF parent is FROM or join, delete source table
    if (Image == qtiFrom || Image == qtiJoin)
        DeleteTable(Item, false);
    // IF parent is WHERE, condition set or HAVING, delete condition
    else if (Image == qtiWhere || Image == qtiSubConds || Image == qtiHaving)
        DeleteCond(Item, false);
    // IF parent is SELECT, delete column, redraw grid
    else if (Image == qtiSelect)
    {
        m_Tree.Delete(Item);
        m_Grid->Refresh();
    }
    else
    {
        wxTreeItemId Prev;
        // IF Item is last query in expression, reset association in previos query
        wxTreeItemId Next = m_Tree.GetNextSibling(Item);
        if (!Next.IsOk())
            Prev = m_Tree.GetPrevSibling(Item);
        m_Tree.Delete(Item);
        if (Prev.IsOk())
        {
            CQDUQuery *Query = (CQDUQuery *)m_Tree.GetItemData(Prev);
            Query->m_Assoc   = QA_NONE;
            Query->m_All     = false;
            Query->m_Corresp = false;
            Query->m_By.Clear();
            m_Tree.SetItemText(Prev, GetTableName(Query, GTN_ALIAS | GTN_TITLE));
        }
    }
    changed = true;
}
//
// Adds new column to design
// Input:   QueryNode   - Tree item of parent query
//          Expr        - Column definition
//          Alias       - Column alias
//
void CQueryDesigner::AddColumn(const wxTreeItemId &QueryNode, const wxChar *Expr, const wxChar *Alias)
{
    wxTreeItemIdValue cookie;
    // Get SELECT item
    wxTreeItemId SelectNode = m_Tree.GetFirstChild(QueryNode, cookie);
    // Create new column record, set definition and alias
    CQDColumn *Column   = new CQDColumn();
    Column->m_Expr      = wxString(Expr);
    Column->m_Alias     = wxString(Alias);
    const wxChar *cName = Alias;
    if (!cName || !*cName)
        cName = Expr;
    // Append new tree item to SELECT, set record as ItemData
    m_Tree.AppendItem(SelectNode, cName, qtiColumn, qtiColumn, Column);
    changed = true;
}
//
// Delete column action
//
void CQueryDesigner::DeleteColumn(const wxTreeItemId &Node)
{
    // Ask user
    if (yesno_box(_("Do you realy want to delete the selected column?"), dialog_parent()))
    {
        m_Tree.Delete(Node);    // Delete tree item
        m_Grid->Refresh();      // Refresh grid
        changed = true;
    }
}

static tcateg NewTableCat[] = {CATEG_TABLE, CATEG_CURSOR, CATEG_TABLE, CATEG_SYSQUERY};
//
// Add new table action
//
void CQueryDesigner::AddTable()
{
    // Available in visual editor only
    if (m_Edit.IsShown())
        return;
    // Close grid cell editor
    m_Grid->DisableCellEditControl();
    // Show select table dialog
    CAddTableDlg Dlg;
    if (!Dlg.Create(this))
        return;
    if (Dlg.ShowModal() != wxID_OK)
        return;
    int Type        = Dlg.m_TypeCB.GetSelection();
    tcateg   Categ  = NewTableCat[Type];
    wxString Name;
    wxString Schema;
    // IF schema table or query AND Schema is current schema, clear schema name
    if (Type < 2)
    {
        Schema = Dlg.m_SchemaCB.GetString(Dlg.m_SchemaCB.GetSelection());
        if (wxStricmp(Schema, m_Schema) == 0)
            Schema.Clear();
    }
    long i = -1;
    // FOR each selected item add new table
    for (;;)
    {
        i = Dlg.m_List.GetNextItem(i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (i == -1)
            break;
        Name = GetProperIdent(Dlg.m_List.GetItemText(i));
        AddNewTable(Schema, Name, Categ);
    }
    // IF current grid is FROM grid, redraw it
    if (m_Grid->GetId() == qtiFrom)
        m_Grid->Refresh();
    changed = true;
}

#define HIDDEN_ATTR ('_' + ('W' << 8) + ('5' << 16) + ('_' << 24))
//
// Adds TableBox to table panel, called when curernt query changes
// Input:   Source  - TableBox source
//          Categ   - Source category
//          Caption - TableBox caption
//          Node    - Table tree item
//
void CQueryDesigner::AddTable(const wxString &Source, tcateg Categ, wxString Caption, const wxTreeItemId &Node)
{
    // IF caption not specified, caption = source
    if (Caption.IsEmpty())
        Caption = Source;
    // Create new TableBox
    CQDTableBox *Box = new CQDTableBox(Node, Source, Categ);
    if (!Box->Create(&m_Tables))
        return;
    // Fill clivar array with param values
    struct t_clivar * hostvars; 
    unsigned hostvars_count = GetParams(&hostvars);
    Box->SetTitle(Caption);
    // Fill column list
    Box->SetList(cdp, hostvars, hostvars_count);
    // Set TableBox link tree item
    CQDQTable *Table  = (CQDQTable *)m_Tree.GetItemData(Node);
    Table->m_TableBox = Box;
}
//
// Adds new table to design, called by Add new table action or Drag&Drop
//
void CQueryDesigner::AddNewTable(const wxString &Schema, const wxString &Name, tcateg Categ)
{
    wxTreeItemId FromNode = GetFromNode(m_CurQuery);
    // Create new table record
    CQDDBTable *Table = new CQDDBTable();
    if (!Table)
    {
        no_memory();
        return;
    }
    // Set schema and table name
    Table->m_Schema   = Schema;
    Table->m_Name     = Name;
    // Append new tree item to FROM, set record as ItemData
    wxString Alias    = GetTableName(Table, GTN_ALIAS | GTN_TITLE | GTN_UNIQUE);
    wxTreeItemId Item = m_Tree.AppendItem(FromNode, Alias, qtiTable, qtiTable, Table);
    // Expand FROM tree item
    m_Tree.Expand(FromNode);
    m_Tree.wxWindow::Refresh();

    wxString Source(Name);
    // IF system query, create SELECT statement
    if (Categ == CATEG_SYSQUERY)
    {
        Source = wxT("SELECT * FROM ") + Name + wxT(" WHERE FALSE");
        Categ  = CATEG_DIRCUR;
    }
    // Add TableBox to table panel
    AddTable(Source, Categ, Alias, Item);
}
//
// Delete table action
//
void CQueryDesigner::DeleteTable()
{
    // Available in visual editor only
    if (m_Edit.IsShown())
        return;
    // Close grid cell editor
    m_Grid->DisableCellEditControl();
    // EXIT if no table selected
    CQDTableBox *Box = m_Tables.GetSelTable();
    if (!Box)
        return;
    // Delete table item
    DeleteTable(Box->m_Node);
}
//
// Deletes table
// Input:   Node - Table tree item
//          Ask  - true for ask user, no ask is necessery when deleting any parent item
//
void CQueryDesigner::DeleteTable(wxTreeItemId Node, bool Ask)
{
    CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Node);
    // Ask user
    if (Ask && !yesno_boxp(_("Do you realy want to delete the table %s?"), dialog_parent(), GetTableName(Table, GTN_ALIAS | GTN_TITLE).c_str()))
        return;
    int          TableID = Table->m_TableID;
    // IF exists TableBox, delete TableBox
    CQDTableBox *Box     = Table->m_TableBox;
    if (Box)
        m_Tables.DeleteTable(Box);

    Node = GetItemByTableID(GetFromNode(m_CurQuery), TableID);
    wxTreeItemId Parent = m_Tree.GetItemParent(Node);
    int          Image  = m_Tree.GetItemImage(Parent);
    // IF Parent is join, delete join
    if (Image == qtiJoin)
    {
        DeleteJoin(Parent, false);
        Node = GetItemByTableID(GetFromNode(m_CurQuery), TableID);
    }
    // Delete tree item
    m_Tree.Delete(Node);
    // IF FROM grid, redraw it
    if (m_Grid->GetId() == qtiFrom)
        m_Grid->Refresh();
    changed  = true;
}
//
// Add join action
//
void CQueryDesigner::AddJoin()
{
    // Create new join record
    CQDJoin *Join = new CQDJoin();
    if (!Join)
    {
        no_memory();
        return;
    }
    // Append new tree item to selected item, set record as ItemData
    wxTreeItemId Item = m_Tree.AppendItem(m_Tree.GetSelection(), GetTableName(Join, GTN_ALIAS | GTN_TITLE), qtiJoin, qtiJoin, Join);
    m_Tree.SelectItem(Item);
    changed = true;
}
//
// Deletes join
// Input:   Node - Join tree item
//          Ask  - true for ask user, no ask is necessery when deleting any parent item
// 
void CQueryDesigner::DeleteJoin(wxTreeItemId Node, bool Ask)
{
    // Ask user
    if (Ask && !yesno_box(_("Do you realy want to delete the selected join?"), dialog_parent()))
        return;
    wxTreeItemIdValue cookie;
    wxTreeItemId ParNode;
    wxTreeItemId Parent   = m_Tree.GetItemParent(Node);
    wxTreeItemId FromNode = GetFromNode(m_CurQuery);
    int          Image    = m_Tree.GetItemImage(Parent);
    CQDJoin     *ParJoin  = (CQDJoin *)m_Tree.GetItemData(Parent);
    wxTreeItemId Item     = m_Tree.GetFirstChild(Node, cookie);
    // IF first source table exists
    if (Item.IsOk())
    {
        CQDQTable   *Table    = (CQDQTable *)m_Tree.GetItemData(Item);
        // IF join parent is another join and source table is by join condition linked to parent join, new parent of source table will be parent join
        if (Image == qtiJoin && (GetItemByTableID(Item, ParJoin->m_DstTableID).IsOk() || GetItemByTableID(Item, ParJoin->m_SrcTableID).IsOk()))
            ParNode = Parent;
        // ELSE new parent of source table will be FROM item
        else
            ParNode = FromNode;
        // Copy source table to new parent
        CopyTreeItem(ParNode, Item);
        // Get second join source table
        Item  = m_Tree.GetNextChild(Node, cookie);
        Table = (CQDQTable *)m_Tree.GetItemData(Item);
        // IF join parent is another join and source table is by join condition linked to parent join, new parent of source table will be parent join
        if (Image == qtiJoin && (GetItemByTableID(Item, ParJoin->m_DstTableID).IsOk() || GetItemByTableID(Item, ParJoin->m_SrcTableID).IsOk()))
            ParNode = Parent;
        // ELSE new parent of source table will be FROM item
        else
            ParNode = FromNode;
        // Copy source table to new parent
        CopyTreeItem(ParNode, Item);
        // Delete source tables link if exists
        CAttrLink *AttrLink = m_Tables.FindAttrLink(Node);
        if (AttrLink)
            m_Tables.m_AttrLinks.delet(m_Tables.m_AttrLinks.find(AttrLink));
    }
    // Delete tree node
    m_Tree.Delete(Node);
    // Select parent node
    m_Tree.SelectItem(ParNode);
    // Redraw grid
    if (ParNode = FromNode)
        m_Grid->ForceRefresh();
    // Redraw tables panel
    m_Tables.Refresh();
    changed = true;
}
//
// Add WHERE clause action
//
wxTreeItemId CQueryDesigner::AddWhere()
{
    wxTreeItemIdValue cookie;
    // Find FROM item
    m_Tree.GetFirstChild(m_CurQuery, cookie);                       // SELECT
    wxTreeItemId Last = m_Tree.GetNextChild(m_CurQuery, cookie);    // FROM
    // Insert WHERE item after FROM
    wxTreeItemId Item = m_Tree.InsertItem(m_CurQuery, Last, wxT("WHERE"), qtiWhere);
    m_Tree.SelectItem(Item);
    changed = true;
    return Item;
}
//
// Delete WHERE clause action
//
void CQueryDesigner::DeleteWhere()
{
    // Ask user
    if (!yesno_box(_("Do you realy want to delete all WHERE conditions?"), dialog_parent()))
        return;
    // Delete tree item
    m_Tree.Delete(m_Tree.GetSelection());
    changed = true;
}
//
// Add subcondition action
//
void CQueryDesigner::AddSubCond()
{
    // Create new condition record
    CQDCond *Cond = new CQDCond(true);
    if (!Cond)
    {
        no_memory();
        return;
    }
    // Append new tree item to selected item, set record as ItemData
    CQDQuery   *Query = (CQDQuery *)m_Tree.GetItemData(m_CurQuery); 
    Cond->m_Expr      = wxString::Format(wxT("%s %d"), tnCondr, ++Query->m_CondNO);
    wxTreeItemId Par  = m_Tree.GetSelection();
    wxTreeItemId Item = m_Tree.AppendItem(Par, Cond->m_Expr, qtiSubConds, qtiSubConds, Cond);
    // Expand parent
    m_Tree.Expand(Par);
    // Select new item
    m_Tree.SelectItem(Item);
    // IF new item has sibling AND association is not specified, set association to AND
    Item = m_Tree.GetPrevSibling(Item);
    if (Item.IsOk())
    {
        Cond = (CQDCond *)m_Tree.GetItemData(Item);
        if (Cond->m_Assoc == CA_NONE)
        {
            Cond->m_Assoc = CA_AND;
            wxString Capt = Cond->m_Expr + wxT(" - AND");
            m_Tree.SetItemText(Item, Capt);
        }
    }
    changed = true;
}
//
// Delete condition action
// Input:   Node - condition tree item
//          Ask  - true for ask user, no ask is necessery when deleting any parent item
//
void CQueryDesigner::DeleteCond(const wxTreeItemId &Node, bool Ask)
{
    // Ask user
    if (Ask && !yesno_box(_("Do you realy want to delete the selected condition?"), dialog_parent()))
        return;
    wxTreeItemId Parent = m_Tree.GetItemParent(Node);
    int          cc     = (int)m_Tree.GetChildrenCount(Parent, false);
    int          Index  = IndexOf(Node);
    // IF condition is last condition in parent, remove assotiation from previous item
    if (Index > 0 && Index == cc - 1)
    {
        wxTreeItemId PrevItem = GetItem(Parent, cc - 2);
        CQDCond *Cond         = (CQDCond *)m_Tree.GetItemData(PrevItem);
        Cond->m_Assoc         = CA_NONE;
        wxString Capture      = m_Tree.GetItemText(PrevItem);
        const wxChar *pCapt   = Capture;
        int Len               = (int)Capture.Len();
        if (wxStrcmp(pCapt + Len - 5, wxT(" - OR")) == 0)
            Len -= 5;
        else if (wxStrcmp(pCapt + Len - 6, wxT(" - AND")) == 0)
            Len -= 6;
        Capture.Truncate(Len);
        m_Tree.SetItemText(PrevItem, Capture);
    }
    // Delete tree item
    m_Tree.Delete(Node);
    // IF single subcondition remains in parent AND parent is condition set, move remaining subcondition to parent level
    if (cc == 2)
    {
        int Image = m_Tree.GetItemImage(Parent);
        if (Image == qtiSubConds)
        {
            wxTreeItemIdValue cookie;
            wxTreeItemId OldItem = m_Tree.GetFirstChild(Parent, cookie);
            Image                = m_Tree.GetItemImage(OldItem);
            CQDCond *Cond        = new CQDCond((CQDCond *)m_Tree.GetItemData(OldItem));
            CQDCond *ParCond     = (CQDCond *)m_Tree.GetItemData(Parent);
            Cond->m_Assoc        = ParCond->m_Assoc;
            wxString Caption(Cond->m_Expr);
            if (Cond->m_Assoc)
            {
                Caption          += wxT(" - ");
                Caption          += AndOrTbl[Cond->m_Assoc];
            }
            wxTreeItemId NewItem  = m_Tree.InsertItem(m_Tree.GetItemParent(Parent), Parent, Caption, Image, Image, Cond);
            CopyTree(NewItem, OldItem);
            m_Tree.Delete(Parent);
        }
    }
    // IF parent is WHERE and is now empty, delete WHERE item
    else if (cc == 1)
    {
        int Image = m_Tree.GetItemImage(Parent);
        if (Image == qtiWhere)
            m_Tree.Delete(Parent);
    }
    // IF WHERE grid shown, redraw it
    if (m_Grid->GetId() == qtiWhere)
        m_Grid->Refresh();
    // Redraw links in tables panel
    m_Tables.Refresh(); 
    changed = true;
}
//
// Add GROUP BY clause action
//
void CQueryDesigner::AddGroupBy()
{
    wxTreeItemIdValue cookie;
    // Last = FROM item
    m_Tree.GetFirstChild(m_CurQuery, cookie);                      // SELECT
    wxTreeItemId Last = m_Tree.GetNextChild(m_CurQuery, cookie);   // FROM
    wxTreeItemId Item = m_Tree.GetNextChild(m_CurQuery, cookie);   // WHERE
    // IF WHERE item exists, Last = WHERE item
    if (Item.IsOk())
    {
        int ii = m_Tree.GetItemImage(Item);
        if (ii == qtiWhere)
            Last = Item;
    }
    // Insert new tree item after Last, select new item
    Item = m_Tree.InsertItem(m_CurQuery, Last, wxT("GROUP BY"), qtiGroupBy);
    m_Tree.SelectItem(Item);
    changed = true;
}
//
// Delete GROUP BY clause action
//
void CQueryDesigner::DeleteGroupBy()
{
    // Ask user
    if (!yesno_box(_("Do you realy want to delete all GROUP BY expressions?"), dialog_parent()))
        return;
    wxTreeItemId GroupByItem = m_Tree.GetSelection();
    wxTreeItemId HavingItem  = m_Tree.GetNextSibling(GroupByItem);
    // Delete GROUP BY tree item
    m_Tree.Delete(GroupByItem);
    // IF HAVING exists, delete HAVING tree item
    if (HavingItem.IsOk())
        m_Tree.Delete(HavingItem);
    changed = true;
}
//
// Add HAVING clause action
//
void CQueryDesigner::AddHaving()
{
    wxTreeItemIdValue cookie;
    // Last = FROM item
    m_Tree.GetFirstChild(m_CurQuery, cookie);                       // SELECT
    wxTreeItemId Last = m_Tree.GetNextChild(m_CurQuery, cookie);    // FROM
    wxTreeItemId Item = m_Tree.GetNextChild(m_CurQuery, cookie);    // WHERE
    // IF WHERE item exists, Last = WHERE utem
    if (Item.IsOk())
    {
        int ii = m_Tree.GetItemImage(Item);
        if (ii == qtiWhere)
        {
            Item = m_Tree.GetNextChild(m_CurQuery, cookie);         // GROUP BY
            ii   = m_Tree.GetItemImage(Item);
            // IF GROUP BY item exists, Last = GROUP BY item
            if (ii == qtiGroupBy)
                Last = Item;
        }
    }
    // Insert new tree item after Last, select new item
    Item = m_Tree.InsertItem(m_CurQuery, Last, wxT("HAVING"), qtiHaving);
    m_Tree.SelectItem(Item);
    changed = true;
}
//
// Delete HAVING clause action
//
void CQueryDesigner::DeleteHaving()
{
    // Ask user
    if (yesno_box(_("Do you realy want to delete all HAVING conditions?"), dialog_parent()))
    {
        // Delete tree item, redraw grid
        m_Tree.Delete(m_Tree.GetSelection());
        m_Grid->Refresh();
        changed = true;
    }
}
//
// Delete ORDER BY/GROUP BY expression action
//
void CQueryDesigner::DeleteExpr(const wxTreeItemId &Node)
{
    // Ask user
    if (yesno_box(_("Do you realy want to delete the selected expression?"), dialog_parent()))
    {
        wxTreeItemId Parent = m_Tree.GetItemParent(Node);
        // Delete tree item
        m_Tree.Delete(Node);
        // IF expression was last item in ORDER BY/GROUP BY, delete parent item
        if (m_Tree.GetChildrenCount(Parent, false) == 0)
        {
            int ii = m_Tree.GetItemImage(Parent);
            if (ii == qtiOrderBy || ii == qtiGroupBy)
                m_Tree.Delete(Parent);
        }
        // Redraw grid
        m_Grid->Refresh();
        changed = true;
    }
}
//
// Add ORDER BY clause action
//
void CQueryDesigner::AddOrderBy()
{
    // Last = last child of parent item
    wxTreeItemId Parent = m_Tree.GetSelection();
    wxTreeItemId Last   = m_Tree.GetLastChild(Parent);
    int          Image  = m_Tree.GetItemImage(Last);
    // IF Last is LIMIT, Last = previous sibling
    if (Image == qtiLimit)
        Last  = m_Tree.GetPrevSibling(Last);
    // Create new ORDER BY record
    CQDOrder *OrderBy = new CQDOrder();
    if (!OrderBy)
    {
        no_memory();
        return;
    }
    wxTreeItemId Item;
    // IF last item is LIMIT, insert new tree item before LIMIT, set record as its ItemData
    if (Image == qtiLimit)
        Item = m_Tree.InsertItem(Parent, Last, wxT("ORDER BY"), qtiOrderBy, qtiOrderBy, OrderBy);
    // ELSE append new tree item, set record as its ItemData
    else
        Item = m_Tree.AppendItem(Parent, wxT("ORDER BY"), qtiOrderBy, qtiOrderBy, OrderBy);
    m_Tree.SelectItem(Item);
    changed = true;
}
//
// Delete ORDER BY clause action
//
void CQueryDesigner::DeleteOrderBy()
{
    // Ask user
    if (yesno_box(_("Do you realy want to delete all ORDER BY expressions?"), dialog_parent()))
    {
        // Delete tree item
        m_Tree.Delete(m_Tree.GetSelection());
        changed = true;
    }
}
//
// Add LIMIT clause action
//
void CQueryDesigner::AddLimit()
{
    // Create new LIMIT record
    CQDLimit *Limit = new CQDLimit();
    if (!Limit)
    {
        no_memory();
        return;
    }
    // Append new tree item, set record as its ItemData
    wxTreeItemId Item= m_Tree.AppendItem(m_Tree.GetSelection(), wxT("LIMIT"), qtiLimit, qtiLimit, Limit);
    m_Tree.SelectItem(Item);
    changed = true;
}
//
// Delete LIMIT clause action
//
void CQueryDesigner::DeleteLimit(const wxTreeItemId &Node)
{
    // Ask user
    if (yesno_box(_("Do you realy want to delete the LIMIT?"), dialog_parent()))
    {
        // Delete tree item
        m_Tree.Delete(Node);
        changed = true;
    }
}
//
// If given item is join and join condition can be displayed, creates join link
// Caled when current query changes
//
void CQueryDesigner::AddAttrLink(const wxTreeItemId &Node)
{
    CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Node);
    // IF item is join AND I can draw join condition
    if (Table->m_Type == TT_JOIN && (((CQDJoin *)Table)->m_JoinType & JT_WIZARDABLE))
    {
        wxTreeItemIdValue cookie;
        // Build join link for first source table
        wxTreeItemId Item   = m_Tree.GetFirstChild(Node, cookie);
        CQDQTable *DstTable = (CQDQTable *)m_Tree.GetItemData(GetItemByTableID(Node, ((CQDJoin *)Table)->m_DstTableID));
        AddAttrLink(Item);
        // Build join link for second source table
        Item = m_Tree.GetNextChild(Node, cookie);
        AddAttrLink(Item);
        // Add current join link to list
        CQDQTable *SrcTable = (CQDQTable *)m_Tree.GetItemData(GetItemByTableID(Node, ((CQDJoin *)Table)->m_SrcTableID));
        AddAttrLink(DstTable->m_TableBox, ((CQDJoin *)Table)->m_DstAttr, SrcTable->m_TableBox, ((CQDJoin *)Table)->m_SrcAttr, Node);
    }
}
//
// Adds join link to AttrLinks list
// Input:   DstTable - Destination TableBox
//          DstAttr  - Destination column name
//          SrcTable - Source TableBox
//          SrcAttr  - Source column name
//          Node     - Join tree item
//
void CQueryDesigner::AddAttrLink(CQDTableBox *DstTable, const wxChar *DstAttr, CQDTableBox *SrcTable, const wxChar *SrcAttr, const wxTreeItemId &Node)
{
    if (!DstTable || !SrcTable)
        return;
	wxChar Attr[OBJ_NAME_LEN + 3];
	const wxChar *pAttr = DstAttr;
    // Remove ` from destination column name
	if (*pAttr == '`')
	{
		int len   = (int)wcslen(pAttr) - 2;
		memcpy(Attr, DstAttr + 1, len * sizeof(wxChar));
		Attr[len] = 0;
		pAttr     = Attr;
	}
    // Find column in destination table
    int DstItem = DstTable->IndexOf(pAttr);
    if (DstItem == -1)
        return;
	pAttr = SrcAttr;
    // Remove ` from source column name
	if (*pAttr == '`')
	{
		int len   = (int)wcslen(pAttr) - 2;
		memcpy(Attr, SrcAttr + 1, len * sizeof(wxChar));
		Attr[len] = 0;
		pAttr     = Attr;
	}
    // Find column in source table
    int SrcItem = SrcTable->IndexOf(pAttr);
    if (SrcItem == -1)
        return;

    // Add new AttrLink to list
    CAttrLink *AttrLink = m_Tables.m_AttrLinks.acc2(m_Tables.m_AttrLinks.count());
    if (!AttrLink)
    {
        no_memory();
        return;
    }
    // Set AttrLink
    AttrLink->m_DstAttr.m_TableBox = DstTable;
    AttrLink->m_DstAttr.m_AttrNO   = DstItem;
    AttrLink->m_SrcAttr.m_TableBox = SrcTable;
    AttrLink->m_SrcAttr.m_AttrNO   = SrcItem;
    AttrLink->m_Node               = Node;
}
//
// Creates new join link or new WHERE condition
// Caled from Drag&Drop operation
// Input:   DstTable - Destination TableBox
//          DstAttr  - Destination column name
//          SrcTable - Source TableBox
//          SrcAttr  - Source column name
//
void CQueryDesigner::AddAttrLink(CQDTableBox *DstTable, const wxChar *DstAttr, CQDTableBox *SrcTable, const wxChar *SrcAttr)
{
    wxTreeItemId Item;
    wxString Expr;
    // Get destination and source table record
    CQDQTable *dTable  = (CQDQTable *)m_Tree.GetItemData(DstTable->m_Node);
    CQDQTable *sTable  = (CQDQTable *)m_Tree.GetItemData(SrcTable->m_Node);
    // IF joins are to be converted to WHERE condition, create WHERE condition
    if (m_JoinConv)
    {
        // Get WHERE tree item
        wxTreeItemId WhereNode = GetWhereNode(m_CurQuery);
        if (!WhereNode.IsOk())
            WhereNode = AddWhere();
        // Ensure that WHERE contains only with AND associated subconditions
        CheckRootWhere();
        // Create new condition record
        CQDCond *Cond = new CQDCond(false);
        // Build condition expression
        Cond->m_Expr  = GetAttrName(dTable, DstAttr) + wxT(" = ") + GetAttrName(sTable, SrcAttr);
        // Append new tree item to WHERE, set record as new item ItemData
        Item = m_Tree.AppendItem(WhereNode, Cond->m_Expr, qtiCond, qtiCond, Cond);
        int wc = GetWhereCount(m_CurQuery);
        // IF new condition is not single condition, associate it with previos item by AND
        if (wc > 1)
        {
            wxTreeItemId PrevItem = GetItem(WhereNode, wc - 2);
            CQDCond     *PrevCond = (CQDCond *)m_Tree.GetItemData(PrevItem);
            PrevCond->m_Assoc = CA_AND;
            Expr = PrevCond->m_Expr + wxT(" - AND");
            m_Tree.SetItemText(PrevItem, Expr);
        }
        // Redraw grid
        if (m_Grid->GetId() == qtiWhere && m_Grid->m_Node == WhereNode)
            m_Grid->Refresh();
    }
    // ELSE create join
    else
    {
        // Cretate new join record
        CQDJoin *Join = new CQDJoin();
        Join->m_JoinType   = JT_ON | JT_WIZARDABLE;
        Join->m_DstTableID = dTable->m_TableID;
        Join->m_DstAttr    = DstAttr;
        Join->m_SrcTableID = sTable->m_TableID;
        Join->m_SrcAttr    = SrcAttr;
        Join->m_Cond       = GetAttrName(dTable, DstAttr) + wxT(" = ") + GetAttrName(sTable, SrcAttr);
        // Append new tree item to FROM, set record as its ItemData
        Item               = m_Tree.AppendItem(GetFromNode(m_CurQuery), GetTableName(Join, GTN_ALIAS | GTN_TITLE), qtiJoin, qtiJoin, Join);
        // IF destination table is member of join, find join root
        wxTreeItemId jr    = GetJoinRoot(DstTable->m_Node);
        // Copy destination to new item
        CopyTreeItem(Item, jr);
        // Delete original branch
        m_Tree.Delete(jr);
        // IF source table is member of join, find join root
        jr                 = GetJoinRoot(SrcTable->m_Node);
        // Copy source to new item
        CopyTreeItem(Item, jr);
        // Delete original branch
        m_Tree.Delete(jr);   
        // Select new item
        m_Tree.SelectItem(Item);
    }
    // Add new link
    AddAttrLink(DstTable, DstAttr, SrcTable, SrcAttr, Item);
    // Redraw tables panel
    m_Tables.Refresh();
    changed = true;
}
//
// If WHERE condition of current query does not contain only with AND associated subconditions,
// coveres these subcondition to one separeted condition.
//
void CQueryDesigner::CheckRootWhere()
{
    wxTreeItemId WhereNode = GetWhereNode(m_CurQuery);
    // IF subconditions are not with AND associated
    if (!OnlyAndConds(WhereNode))
    {
        CQDQuery   *Query = (CQDQuery *)m_Tree.GetItemData(m_CurQuery);
        // Create new condition record
        CQDCond     *Cond = new CQDCond(true);
        Cond->m_Expr      = wxString::Format(wxT("%ls %d"), tnCondr, ++Query->m_CondNO);
        // Append new tree item to WHERE, set record as new item ItemData
        wxTreeItemId Item = m_Tree.AppendItem(WhereNode, Cond->m_Expr, qtiSubConds, qtiSubConds, Cond);
        // Copy original WHERE children to no item
        CopyTree(Item, WhereNode);
    }
}
//
// Returns true if subconditions in condition set are not associated with OR
//
bool CQueryDesigner::OnlyAndConds(const wxTreeItemId &Node)
{
    wxTreeItemIdValue cookie;
    for (wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
    {
        CQDCond *Cond = (CQDCond *)m_Tree.GetItemData(Item);
        if (Cond->m_Assoc == CA_OR)
            return false;
    }
    return true;
}
//
// Returns ancestor of given table item which is direct descendant of WHERE item
//
wxTreeItemId CQueryDesigner::GetJoinRoot(const wxTreeItemId &Node)
{

    wxTreeItemId Item = Node;
    for (;;)
    {
        wxTreeItemId Parent = m_Tree.GetItemParent(Item);
        int Image = m_Tree.GetItemImage(Parent);
        if (Image == qtiFrom)
            break;
        Item = Parent;
    }
    return Item;
}
//
// Delete join link action
//
void CQueryDesigner::DeleteAttrLink(const wxTreeItemId &Node)
{
    if (m_InReset)
        return;
    // IF item is condition, delete condition ELSE delete join
    int Image = m_Tree.GetItemImage(Node);
    if (Image == qtiCond)
        DeleteCond(Node, false);
    else
        DeleteJoin(Node, false);
}
//
// Moves SrcNode tree item descendants to DstNode item
//
void CQueryDesigner::CopyTree(const wxTreeItemId &DstNode, const wxTreeItemId &SrcNode)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId      Next;
    for (wxTreeItemId Item = m_Tree.GetFirstChild(SrcNode, cookie); Item.IsOk(); Item = Next)
    {
        CopyTreeItem(DstNode, Item);
        Next = m_Tree.GetNextSibling(Item);
        m_Tree.Delete(Item);
    }
}
//
// Copies OldItem to Parent 
// Before is index of folowing item, QDTREE_APPEND means append new item to the end
// Returns new item
//
wxTreeItemId CQueryDesigner::CopyTreeItem(const wxTreeItemId &Parent, const wxTreeItemId &OldItem, size_t Before)
{
    wxString        Caption = m_Tree.GetItemText(OldItem);
    wxTreeItemData *OldData = m_Tree.GetItemData(OldItem);
    wxTreeItemData *NewData;

    // Create copy of original record
    int Image = m_Tree.GetItemImage(OldItem);
    switch (Image)
    {
    case qtiQueryExpr:
        NewData = new CQDQueryExpr((CQDQueryExpr *)OldData);
        break;
    case qtiQuery:
        NewData = new CQDQuery((CQDQuery *)OldData);
        break;
    case qtiTable:
        NewData = new CQDDBTable((CQDDBTable *)OldData);
        break;
    case qtiJoin:
        NewData = new CQDJoin((CQDJoin *)OldData);
        break;
    case qtiColumn:
        NewData = new CQDColumn((CQDColumn *)OldData);
        break;
    case qtiCond:
    case qtiSubConds:
        NewData = new CQDCond((CQDCond *)OldData);
        break;
    case qtiGColumn:
        NewData = new CQDGroup((CQDGroup *)OldData);
        break;
    case qtiOColumn:
        NewData = new CQDOrder((CQDOrder *)OldData);
        break;
    case qtiLimit:
        NewData = new CQDLimit((CQDLimit *)OldData);
        break;
    default:
        NewData = NULL;
        break;
    }
    wxTreeItemId NewItem;
    // IF append, append new item to Parent, set record as its ItemData
    if (Before == QDTREE_APPEND)
        NewItem = m_Tree.AppendItem(Parent, Caption, Image, Image, NewData);
    // ELSE insert new item before Before, set record as its ItemData
    else
        NewItem = m_Tree.InsertItem(Parent, Before, Caption, Image, Image, NewData);

    // IF grid is linked to OldItem, set NewItem
    if (m_Grid->m_Node == OldItem)
        m_Grid->m_Node  = NewItem;
    // IF OldItem is table, query or query expression
    if (Image == qtiTable || Image == qtiQuery || Image == qtiQueryExpr)
    {
        // Clear OldItem TableBox link, set NewItem TableBox link
        CQDTableBox *Box = ((CQDQTable *)OldData)->m_TableBox;
        if (Box)
        {
            Box->m_Node                        = NewItem;
            ((CQDQTable *)NewData)->m_TableBox = Box;
            ((CQDQTable *)OldData)->m_TableBox = NULL;
        }
    }
    // IF OldItem is join or condition
    if (Image == qtiJoin || Image == qtiCond)
    {
        // Reset AttrLink item link to NewItem
        CAttrLink *AttrLink = m_Tables.FindAttrLink(OldItem);
        if (AttrLink)
            AttrLink->m_Node = NewItem;
    }
    // IF OldItem has children, copy children
    if (m_Tree.ItemHasChildren(OldItem))
        CopyTree(NewItem, OldItem);
    return NewItem;
}
//
// Switch Visual/Text editor action
//
void CQueryDesigner::OnTextEdit(wxCommandEvent &Event)
{
    CQueryExpr Expr;
    bool       Empty     = false;
    bool       EditShown = m_Edit.IsShown();
    // IF text editor is now visible
    if (EditShown)
    {
        m_QueryXNO = 0;
        m_QueryNO  = 0;
        // IF Statement text is not empty
        Empty      = !*m_Edit.editbuf;
        if (!Empty)
        {
            t_temp_appl_context tac(cdp, schema_name);
            tccharptr Cmd = Tochar(m_Edit.editbuf);
            if (!Cmd)
                return;
            // Compile query
            int Err = compile_query(cdp, Cmd, &Expr);
            // IF syntax error, show message, set cursor to error position, cancel event
            if (Err)
            {
                client_error(cdp, Err);  
                cd_Signalize2(cdp, dialog_parent());
                m_Edit.position_on_line_column(cdp->RV.comp_err_line, cdp->RV.comp_err_column);
                m_Edit.scrolled_panel->SetFocus();
                return;
            }
        }
        // Clear tree content
        m_InReset = true;
        m_Tree.DeleteAllItems();
        m_InReset = false;
        m_CurQuery.Unset();
        // IF text edit changed, set visual editor changed flag
        if (m_Edit.IsChanged())
            changed = true;
        wxGetApp().frame->SetStatusText(wxString(), 1);
    }
    // ELSE visual editor shown
    else
    {
        // Close grid cell editor
        m_Grid->DisableCellEditControl();
        // IF design changed
        if (changed)
        {
            // Build statement text, set it to text editor
            m_ToText          = true;
            m_CurQueryChanged = false;
            BuildResult();
            m_Edit.SetText(m_Result);
        };
        if (m_Grid)
        {
            m_Grid->Destroy();
            m_Grid = NULL;
        }
#ifdef LINUX                
        m_Edit.scrolled_panel->GetCaret()->Show(true);
#endif                
    }
    // Switch editor
    SwitchEditor(!EditShown, Empty, &Expr);
}
//
// Switches designer between visual and text editor
// Input:   TextEdit  - true to text editor, false to visual editor
//          Empty     - Statement is empty
//          Expr      - Root query record
//
void CQueryDesigner::SwitchEditor(bool TextEdit, bool Empty, CQueryExpr *Expr)
{
    // IF switch to text editor
    if (TextEdit)
    {
        // Hide visual editor, show text editor
        Show(false);
        m_Edit.Show(true);
#if WBVERS>=1000
        // IF popup or tabs
        if (global_style!=ST_MDI)
        {
            wxWindow *Parent = GetParent();
            wxSizer  *Sizer  = Parent->GetSizer();
            if (Sizer)
            { 
                // Replace visual editor in sizer with text editor
                Sizer->Remove(1);
                Sizer->Add(&m_Edit, 1, wxGROW);
                Parent->Layout();
            }
        }
#endif
        // Set focus to to text editor
        m_Edit.scrolled_panel->new_caret();
        m_Edit.scrolled_panel->SetFocus();
    }
    // ELSE switch to visual editor
    else
    {
        wxTreeItemIdValue cookie;
        // Load query to designer
        LoadQueryExpr(NULL, Expr);
        wxTreeItemId Item = m_Tree.GetRootItem();
        // IF design is empty, add simple query
        if (Empty)
            AddQuery(Item);
        // Expand root tree item
        m_Tree.Expand(Item);
        // Expand first query item
        Item = m_Tree.GetFirstChild(Item, cookie);
        if (Item.IsOk())
        {
            m_Tree.Expand(Item);
            Item = m_Tree.GetFirstChild(Item, cookie);
        }
#ifdef LINUX                
        m_Edit.scrolled_panel->GetCaret()->Show(false);
#endif                
        // Hide text editor, show visual editor
        m_Edit.Show(false);
        Show(true);
#if WBVERS>=1000
        // IF popup or tabs
        if (global_style!=ST_MDI)
        {
            wxWindow *Parent = GetParent();
            wxSizer  *Sizer  = Parent->GetSizer();
            if (Sizer)
            {
                // Replace text editor in sizer with visual editor
                Sizer->Remove(1);
                Sizer->Add(this, 1, wxGROW);
                Parent->Layout();
            }
        }
#endif
#ifdef WINS
        // Redraw table windows
        HWND hWnd;
        for (hWnd = GetWindow((HWND)m_Tables.GetHandle(), GW_CHILD); hWnd; hWnd = GetWindow(hWnd, GW_HWNDNEXT))
            SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);
#endif
        m_Tree.SetFocus();  // must focus the designer, its parent already has the focus and would not post it to the tree
    }
    // Enable toolbar buttons
    wxToolBarBase *tb = GetToolBar();
    if (tb)
    {
        tb->EnableTool(TD_CPM_CUT,          TextEdit);
        tb->EnableTool(TD_CPM_COPY,         TextEdit);
        tb->EnableTool(TD_CPM_PASTE,        TextEdit);
        tb->EnableTool(TD_CPM_UNDO,         TextEdit);
        tb->EnableTool(TD_CPM_REDO,         TextEdit);
        tb->EnableTool(TD_CPM_ELEMENTPOPUP, !TextEdit);
        tb->EnableTool(TD_CPM_ADDTABLE,     !TextEdit);
        tb->EnableTool(TD_CPM_DELETETABLE,  !TextEdit);
    }
}
//
// Returns designer toolbar
//
wxToolBarBase * CQueryDesigner::GetToolBar() const
{ 
    if (global_style == ST_MDI)
        return designer_toolbar;
    else
      //tb = (wxToolBar *)the_editor_container_frame->FindWindow(OWN_TOOLBAR_ID);
        return (wxToolBar *)GetParent()->FindWindow(OWN_TOOLBAR_ID);
}
//
// Check statement syntax action
//
extern uns8 _tpsize[];

void CQueryDesigner::OnSyntaxCheck()
{
    wxString Defin = wxChar(QUERY_TEST_MARK);
    // IF text editor, get statement text
    if (m_Edit.IsShown())
        Defin += m_Edit.editbuf;
    else
    {
        // Close grid cell editor
        m_Grid->DisableCellEditControl();
        m_ToText          = false;
        wxTreeItemId Item;
        // IF from popup menu
        if (m_FromPopup)
        {
            // IF selected item is query OR query expression, source is selected item ELSE source is root query
            Item = m_Tree.GetSelection();
            int Image = m_Tree.GetItemImage(Item);
            if (Image != qtiQueryExpr && Image != qtiQuery)
                Item = m_Tree.GetRootItem();
        }
        // ELSE from menu or toolbar, source is root query
        else
            Item = m_Tree.GetRootItem();
        // Build statement text
        BuildUQuery(Item);
        Defin += m_Result;
    }
    tccharptr Cmd = Tochar(Defin);
    if (!Cmd)
        return;
    BOOL Err;
    t_clivar *clivars;
    // Get statement parameters
    int Cnt = GetParams(&clivars);
    // Compile query
    if (Cnt == 0) 
    {
        t_temp_appl_context tac(cdp, schema_name);
        Err = cd_SQL_execute(cdp, Cmd, NULL);
    }
    else
    {
        t_temp_appl_context tac(cdp, schema_name);
        Err = cd_SQL_host_execute(cdp, Cmd, NULL, clivars, Cnt);
        ReleaseParams(clivars, Cnt);
    }

    // IF OK
    if (!Err)
        info_box(_("Query syntax is correct"), dialog_parent());
    else
    {
        // Show error message
        cd_Signalize2(cdp, dialog_parent());
        // IF text editor shown, set cursor to error position
        if (m_Edit.IsShown())
        {
            m_Edit.SetFocus();
            uns32 line;
            uns16 column;
            char buf[PRE_KONTEXT + POST_KONTEXT + 2];
            if (!cd_Get_error_info(cdp, &line, &column, buf) && *buf)
                m_Edit.position_on_line_column(line, column);
        }
    }
}
//
// Data preview action
//
void CQueryDesigner::OnDataPreview()
{
    wxString Defin;
    // IF text editor, get statement text
    if (m_Edit.IsShown())
        Defin = m_Edit.editbuf;
    else
    {
        // Close grid cell editor
        m_Grid->DisableCellEditControl();
        m_ToText          = false;
        wxTreeItemId Item;
        // IF from popup menu
        if (m_FromPopup)
        {
            // IF selected item is query OR query expression, source is selected item ELSE source is root query
            Item = m_Tree.GetSelection();
            int Image = m_Tree.GetItemImage(Item);
            if (Image != qtiQueryExpr && Image != qtiQuery)
                Item = m_Tree.GetRootItem();
        }
        // ELSE from menu or toolbar, source is root query
        else
            Item = m_Tree.GetRootItem();
        // Build statement text
        BuildUQuery(Item);
        Defin = m_Result;
    }

    tcursnum Curs;
    BOOL Err;
    tccharptr Cmd = Tochar(Defin);
    if (!Cmd)
        return;
    t_clivar *clivars;
    t_temp_appl_context tac(cdp, schema_name);
    // Get statement parameters
    int Cnt = GetParams(&clivars);
    // Open cursor
    if (Cnt == 0) 
    {
        Err = cd_Open_cursor_direct(cdp, Cmd, &Curs);
    }
    else
    {
        Err = cd_SQL_host_execute(cdp, Cmd, (uns32 *)&Curs, clivars, Cnt);
        ReleaseParams(clivars, Cnt);
    }
    if (Err)
    {
        cd_Signalize2(cdp, dialog_parent());
        return;
    }

    // Show grid with cursor content
    char Src[32];
    sprintf(Src, "DEFAULT %u CURSORVIEW", Curs);
    open_form(NULL, cdp, Src, NOCURSOR, NO_EDIT | NO_INSERT | AUTO_CURSOR);
}
//
// Optimize action
//
void CQueryDesigner::OnOptimize()
{
    wxString Defin;
    // IF text editor is shown, get statement text
    if (m_Edit.IsShown())
        Defin = m_Edit.editbuf;
    else
    {
        // Close grid cell editor
        m_Grid->DisableCellEditControl();
        m_ToText          = false;
        // Build statement text
        BuildResult();
        Defin = m_Result;
    }
   // Show the Optimize dialog:
    //CQueryOptDlg Dlg;
    //Dlg.Create(dialog_parent(), cdp, Defin.mb_str(*cdp->sys_conv));
    wxCharBuffer bytes = Defin.mb_str(*cdp->sys_conv);
    QueryOptimizationDlg dlg(cdp,bytes);
    dlg.Create(dialog_parent());
    dlg.ShowModal();
}
//
// Set designer properties action
//
void CQueryDesigner::OnSetDesignerProps()
{
    // CLose grid cell editor
    if (m_Grid)
        m_Grid->DisableCellEditControl();
    // Show property dialog
    CQDPropsDlg Dlg;
    Dlg.Create(dialog_parent());
    if (Dlg.ShowModal() == wxID_OK)
    {
        // Set designer properties
        m_StatmStyle      = (TStatmStyle)Dlg.m_Wrap;
        m_Indent          = Dlg.m_Indent;
        m_LineLenLimit    = Dlg.m_RowLength;
        m_WrapItems       = Dlg.m_WrapItems;
        m_WrapAfterClause = Dlg.m_WrapAfterClause;
        m_JoinConv        = Dlg.m_ConvertJoin;
        m_PrefixColumns   = Dlg.m_PrefixColumns;
        m_PrefixTables    = Dlg.m_PrefixTables;
        if (!m_WrapAfterClause)
            m_Indent      = 9;

        // IF text editor is shown, rebuild statement text
        if (m_Edit.IsShown())
        {
            m_ToText = true;
            BuildResult();
            m_Edit.SetText(m_Result);
            m_Edit.scrolled_panel->Refresh(); //>Update();
        }
        // ELSE visual editor is shown, reset join links
        else
        {
            ResetAttrLinks();
        }
    }
}
//
// Shows query elements action
//
void CQueryDesigner::ShowElementPopup()
{
    if (!m_Edit.IsShown() && !m_ElementPopup)
    {
        m_ElementPopup = new CElementPopup(this);
        if (!m_ElementPopup)
        {
            no_memory();
            return;
        }
        if (!m_ElementPopup->Create())
            m_ElementPopup->Destroy();
    }
}
//
// According m_Params list fills clivar array and returns parameter count
//
int  CQueryDesigner::GetParams(t_clivar **pclivar)
{
    int Cnt = m_Params.count();
    if (Cnt == 0)
        return 0;
    // Alloc clivar array
    t_clivar *clivars = (t_clivar *)corealloc(sizeof(t_clivar) * Cnt, 66);
    if (!clivars)
    { 
        no_memory();
        return 0;
    }
    // Fill clivar array
    memset(clivars, 0, sizeof(t_clivar) * Cnt);
    t_clivar *clivar = clivars;
    for (int i = 0; i < Cnt; i++)
    {
        t_dad_param *Param = m_Params.acc0(i);
        if (!Param->name[0])
            continue;
        strmaxcpy(clivar->name, Param->name, sizeof(clivars->name));
        clivar->wbtype = Param->type;
        clivar->specif = Param->specif.opqval;
        clivar->mode   = MODE_IN;
        clivar->buflen = Param->type == ATT_STRING || Param->type == ATT_BINARY ? t_specif(clivar->specif).length + 1 : IS_HEAP_TYPE(Param->type) ? MAX_PARAM_VAL_LEN + 1 : _tpsize[Param->type];
        clivar->buf    = (tptr)corealloc(clivar->buflen, 66);
        clivar->actlen = clivar->buflen;
        wxString string_format = get_grid_default_string_format(Param->type);
        int err=superconv(ATT_STRING, Param->type, Param->val, (char*)clivar->buf, wx_string_spec, Param->specif, string_format.mb_str(*wxConvCurrent));
        if (err<0)
          superconv(ATT_STRING, Param->type, wxT(""), (char*)clivar->buf, wx_string_spec, Param->specif, "");
        clivar++;
    }
    Cnt = clivar - clivars;
    if (Cnt == 0)
    {
        corefree(clivars);
        clivars = NULL;
    }
    *pclivar = clivars;
    // Return real parameter count
    return Cnt;
}
//
// Releases clivar array
//
void CQueryDesigner::ReleaseParams(t_clivar *clivars, int Cnt)
{
    for (t_clivar *clivar = clivars; clivar < clivars + Cnt; clivar++)
        corefree(clivar->buf);
    corefree(clivars);
}

//
// Returns Table name for several purposes
// Flags specifies purpose: GTN_PROPER - Cover name in `` if necessary
//                          GTN_ALIAS  - If Alias available, use Alias
//                          GTN_TITLE  - For caption in tree item or TableBox title, without ``
//                          GTN_UNIQUE - If name is not unique in current query, create and return unique alias
//
wxString CQueryDesigner::GetTableName(CQDQTable *Table, unsigned Flags)
{
    // IF Alias requested AND Alias specified, return Alias
    if ((Flags & GTN_ALIAS) && !Table->m_Alias.IsEmpty())
        return Flags & GTN_TITLE ? Table->m_Alias : GetProperIdent(Table->m_Alias);
    // IF Table is database table
    if (Table->m_Type == TT_TABLE)
    {
        // IF unique name requested
        wxTreeItemId FromNode = GetFromNode(m_CurQuery);
        if (Flags & GTN_UNIQUE)
        {
            // IF exists table with same name in current query
            CQDDBTable *SameTable = GetTableWithSameName((CQDDBTable *)Table, FromNode);
            if (SameTable)
            {
                // IF same schema
                if (wxStricmp(GetSchema((CQDDBTable *)Table), GetSchema((CQDDBTable *)SameTable)) == 0)
                {
                    // IF Alias for second not specified, create new Alias
                    if (SameTable->m_Alias.IsEmpty())
                    {
                        SameTable->m_Alias = GetFreeAlias(SameTable->m_Name);
                        if (SameTable->m_TableBox)
                            SameTable->m_TableBox->SetTitle(SameTable->m_Alias);
                        wxTreeItemId Item = GetItemByTableID(FromNode, SameTable->m_TableID);
                        if (Item.IsOk())
                            m_Tree.SetItemText(Item, SameTable->m_Alias);
                    }
                    // Create new Alias for current table
                    Table->m_Alias = GetFreeAlias(SameTable->m_Name);
                    if (Table->m_TableBox)
                        Table->m_TableBox->SetTitle(Table->m_Alias);
                    wxTreeItemId Item = GetItemByTableID(FromNode, Table->m_TableID);
                    if (Item.IsOk())
                        m_Tree.SetItemText(Item, Table->m_Alias);
                    // Redraw grid
                    if (m_Grid)
                        m_Grid->Refresh();
                    // Return new Alias
                    return Table->m_Alias;
                }
                // ELSE return Table name prefixed with schema name
                return GetSchema((CQDDBTable *)Table) + wxT('.') + ((CQDDBTable *)Table)->m_Name;
            }
        }
        // IF not for title
        else if (!(Flags & GTN_TITLE))
        {
            // IF Table needs prefix, return Table name prefixed with schema name, cover name in ` if necessary
            if (TableNeedsPrefix((CQDDBTable *)Table, FromNode))
                return GetProperIdent(GetSchema((CQDDBTable *)Table)) + wxT('.') + GetProperIdent(((CQDDBTable *)Table)->m_Name);
            // ELSE return Table name covered in ` if necessary
            return GetProperIdent(((CQDDBTable *)Table)->m_Name);
        }
        // ELSE return table name without `
        return ((CQDDBTable *)Table)->m_Name;
    }
    const wxChar *Alias;
    // IF Table is query, return "Query nn"
    if (Table->m_Type == TT_QUERY)
    {
        if (Table->m_QueryNO == 0)
            Table->m_QueryNO = ++m_QueryNO;
        Alias = tnQuery;
    }
    // IF Table is query expression, return "Query expression nn"
    else if (Table->m_Type == TT_QUERYEXPR)
    {
        if (Table->m_QueryNO == 0)
            Table->m_QueryNO = ++m_QueryXNO;
        Alias = tnQueryExpr;
    }
    // IF Table is join expression, return "Join nn"
    else if (Table->m_Type == TT_JOIN)
    {
        if (Table->m_QueryNO == 0)
        {
            CQDQuery *Query  = (CQDQuery *)m_Tree.GetItemData(m_CurQuery);
            Table->m_QueryNO = ++Query->m_JoinNO;
        }
        Alias = wxT("Join");
    }
    return wxString::Format(wxT("%s %d"), Alias, Table->m_QueryNO);
}
//
// Returns column name, if necessary prefixed wth table name, eventyaly covered in ``
//
wxString CQueryDesigner::GetAttrName(CQDQTable *Table, wxString Attr)
{
    if (AttrNeedsPrefix(Table,Attr))
        return GetTableName(Table, GTN_ALIAS) + wxT('.') + GetProperIdent(Attr);
    return GetProperIdent(Attr);
}
//
// Returns true if column name needs table name prefix
//
bool CQueryDesigner::AttrNeedsPrefix(CQDQTable *Table, const wxChar *AttrName)
{
    // IF allways prefix columns, return true
    if (m_PrefixColumns)
        return true;

    tccharptr aName = Tochar(AttrName);
    if (!aName)
        return false;
    wxWindowList &List = m_Tables.GetChildren();
    // FOR each TableBox in tables panel
    for (wxWindowListNode *BoxNode = List.GetFirst(); BoxNode; BoxNode = BoxNode->GetNext())
    {
        // Get table definition
        CQDTableBox *Box    = (CQDTableBox *)BoxNode->GetData();
        CQDQTable   *dTable = (CQDQTable *)m_Tree.GetItemData(Box->m_Node);
        // IF current table continue
        if (dTable == Table)
            continue;
        int i = 1;
        // IF column with same name found, return true
        for (const d_attr *Attr = Box->m_TableDef.GetAttr(1); Attr; Attr = Box->m_TableDef.GetAttr(++i))
            if (wb_stricmp(cdp, Attr->name, aName) == 0)
                return true;
    }
    // return false otherwise
    return false;
}
//
// System tables
//
static const wxChar *SysTable[] =
{
    wxT("KEYTAB"),
    wxT("OBJTAB"),
    wxT("REPLTAB"),
    wxT("SRVTAB"),
    wxT("TABTAB"),
    wxT("USERTAB"),
    wxT("__PROPTAB"),
    wxT("__RELTAB")
};
//
// System queries
//
static const wxChar *SysQuery[] =
{
    wxT("_IV_CERTIFICATES"),
    wxT("_IV_CHECK_CONSTRAINS"),
    wxT("_IV_FOREIGN_KEYS"),
    wxT("_IV_INDICIES"),
    wxT("_IV_IP_ADDRESSES"),
    wxT("_IV_LOCKS"),
    wxT("_IV_LOGGED_USERS"),
    wxT("_IV_MAIL_PROFS"),
    wxT("_IV_ODBC_COLUMNS"),         
    wxT("_IV_ODBC_COLUMN_PRIVS"),
    wxT("_IV_ODBC_FOREIGN_KEYS"),    
    wxT("_IV_ODBC_PRIMARY_KEYS"),    
    wxT("_IV_ODBC_PROCEDURES"),      
    wxT("_IV_ODBC_PROCEDURE_COLUMNS"),
    wxT("_IV_ODBC_SPECIAL_COLUMNS"), 
    wxT("_IV_ODBC_STATISTICS"),
    wxT("_IV_ODBC_TABLES"),     
    wxT("_IV_ODBC_TABLE_PRIVS"),     
    wxT("_IV_ODBC_TYPE_INFO"),       
    wxT("_IV_PENDING_LOG_REGS"),
    wxT("_IV_PRIVILEGES"),
    wxT("_IV_PROCEDURE_PARAMETERS"),
    wxT("_IV_RECENT_LOG"),
    wxT("_IV_SUBJECT_MEMBERSHIP"),
    wxT("_IV_TABLE_COLUMNS"),
    wxT("_IV_VIEWED_TABLE_COLUMNS"),
};
//
// Returns true if Table needs prefix
//
bool CQueryDesigner::TableNeedsPrefix(CQDDBTable *Table, const wxTreeItemId &ParNode)
{ 
    int i;
    // IF system table, return false
    for (i = 0; i < sizeof(SysTable) / sizeof(const wxChar *); i++)
    {
        int eq = wxStricmp(Table->m_Name, SysTable[i]); 
        if (eq == 0)
            return false;
        if (eq < 0)
            break;
    }
    // IF system query, return false
    for (i = 0; i < sizeof(SysQuery) / sizeof(const wxChar *); i++)
    {
        int eq = wxStricmp(Table->m_Name, SysQuery[i]); 
        if (eq == 0)
            return false;
        if (eq < 0)
            break;
    }
    // IF allways prefix table names, return true
    if (m_PrefixTables)
        return true;
    // IF Table schema specified AND it is not current schema, return true
    if (!Table->m_Schema.IsEmpty() && wxStricmp(Table->m_Schema, m_Schema) != 0)
        return true;
    // Find table with same name in current query
    CQDDBTable *SameTable = GetTableWithSameName(Table, ParNode);
    // IF Found AND tables schema names doesn't match
    return SameTable != NULL && wxStricmp(GetSchema(Table), GetSchema(SameTable)) != 0;
}
//
// Finds table with same name as Table has in query elemnt given by ParNode
//
CQDDBTable *CQueryDesigner::GetTableWithSameName(CQDDBTable *Table, const wxTreeItemId &ParNode)
{
    wxTreeItemIdValue cookie;
    // FOR each child of ParNode
    for (wxTreeItemId Node = m_Tree.GetFirstChild(ParNode, cookie); Node.IsOk(); Node = m_Tree.GetNextChild(ParNode, cookie))
    {
        int ii = m_Tree.GetItemImage(Node);
        // IF child is database table
        if (ii == qtiTable)
        {
            // IF input table, continue
            CQDDBTable *Result = (CQDDBTable *)m_Tree.GetItemData(Node);
            if (Result == Table)
                continue;
            // IF names match, found
            if (wxStricmp(Table->m_Name, Result->m_Name) == 0)
                return Result;
        }
        // IF child is join OR query expression
        else if (ii == qtiJoin || ii == qtiQueryExpr)
        {
            // Search grandchildren
            CQDDBTable *Result = GetTableWithSameName(Table, Node);
            if (Result)
                return Result;
        }
        // IF child is query
        else if (ii == qtiQuery)
        {
            // Search children of WHERE item
            CQDDBTable *Result = GetTableWithSameName(Table, GetFromNode(Node));
            if (Result)
                return Result;
        }
        else 
        {
            return NULL;
        }
    }
    // Not found
    return NULL;
}
//
// Returns table name unique alias
//
wxString CQueryDesigner::GetFreeAlias(const wxString &Name)
{
    wxString Result;
    int i = 0;
    do
    {
        Result =  Name;
        Result << ++i;
    }
    while (!IsFreeAlias(Result, m_Tree.GetRootItem())); 
    return Result;
}
//
// Returns true if Name is unique table Alias in query element given by ParNode
//
bool CQueryDesigner::IsFreeAlias(const wxChar *Name, const wxTreeItemId &ParNode)
{
    wxTreeItemIdValue cookie;
    // FOR each children of ParNode
    for (wxTreeItemId Node = m_Tree.GetFirstChild(ParNode, cookie); Node.IsOk(); Node = m_Tree.GetNextChild(ParNode, cookie))
    {
        int ii = m_Tree.GetItemImage(Node);
        // IF child is query epression OR join
        if (ii == qtiQueryExpr || ii == qtiJoin)
        {
            // IF Name matches with current item Alias or any grandchild Alias, return false
            CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Node);
            if (wxStricmp(Table->m_Alias, Name) == 0 || !IsFreeAlias(Name, Node))
                return false;
        }
        // IF child is query
        else if (ii == qtiQuery)
        {
            // IF Name matches with query Alias or any query source table Alias, return false
            CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Node);
            if (wxStricmp(Table->m_Alias, Name) == 0 || !IsFreeAlias(Name, GetFromNode(Node)))
                return false;
        }
        // IF child is database table
        else if (ii == qtiTable)
        {
            // IF Name matches with table Alias or table name, return false
            CQDDBTable *Table = (CQDDBTable *)m_Tree.GetItemData(Node);
            if (wxStricmp(Table->m_Alias, Name) == 0 || wxStricmp(Table->m_Name, Name) == 0)
                return false;
        }
        else
        {
            return true;
        }
    }
    return true;
}
//
// Returns index of Item in parent, -1 if not found
//
int CQueryDesigner::IndexOf(const wxTreeItemId &Item)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId Par = m_Tree.GetItemParent(Item);
    int          i   = 0;
    for (wxTreeItemId Node = m_Tree.GetFirstChild(Par, cookie); Node.IsOk(); Node = m_Tree.GetNextChild(Par, cookie), i++)
    {
        if (Node == Item)
            return i;
    }
    return -1;
}
//
// Returns tree item representing FROM clause for tree item representing query
//
wxTreeItemId CQueryDesigner::GetFromNode(const wxTreeItemId &QueryNode)
{
    wxTreeItemIdValue cookie;
    // Return second child of parent
    wxTreeItemId Node = m_Tree.GetFirstChild(QueryNode, cookie);
    return m_Tree.GetNextChild(QueryNode, cookie);
}
//
// Returns tree item representing WHERE clause for tree item representing query
//
wxTreeItemId CQueryDesigner::GetWhereNode(const wxTreeItemId &QueryNode)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId Node = m_Tree.GetFirstChild(QueryNode, cookie);    // SELECT
    Node = m_Tree.GetNextChild(QueryNode, cookie);                  // FROM
    if (Node.IsOk())
    {
        // Get next sibling of FROM item, check if it is WHERE
        Node = m_Tree.GetNextChild(QueryNode, cookie);
        if (Node.IsOk() && m_Tree.GetItemImage(Node) == qtiWhere)
            return Node;
    }
    return wxTreeItemId();
}
//
// Returns tree item representing GROUP BY clause for tree item representing query
//
wxTreeItemId CQueryDesigner::GetGroupByNode(const wxTreeItemId &QueryNode)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId Node = m_Tree.GetFirstChild(QueryNode, cookie);    // SELECT
    Node = m_Tree.GetNextChild(QueryNode, cookie);                  // FROM
    if (Node.IsOk())
    {
        // Get next sibling of FROM item, check if it is GROUP BY
        Node = m_Tree.GetNextChild(QueryNode, cookie);
        if (Node.IsOk())
        {
            if (m_Tree.GetItemImage(Node) == qtiGroupBy)
                return Node;
            // Get next sibling, check if it is GROUP BY
            Node = m_Tree.GetNextChild(QueryNode, cookie);
            if (Node.IsOk() && m_Tree.GetItemImage(Node) == qtiGroupBy)
                return Node;
        }
    }
    return wxTreeItemId();
}
//
// Returns tree item of type specified by Image which is child of Node item
//
wxTreeItemId CQueryDesigner::GetItemByImage(const wxTreeItemId &Node, int Image)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId Item;
    for (Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
    {
        if (m_Tree.GetItemImage(Item) == Image)
            break;
    }
    return Item;
}
//
// Ruturns count of subqueries for query specified by QueryNode
//
int CQueryDesigner::GetQueryCount(const wxTreeItemId &QueryNode)
{
    // Get children count for QueryNode
    int          Count = (int)m_Tree.GetChildrenCount(QueryNode, false);
    wxTreeItemId Item  = m_Tree.GetLastChild(QueryNode);
    int          Image = m_Tree.GetItemImage(Item);
    // IF last child is LIMIT, decrement count
    if (Image == qtiLimit)
    {
        Count--;
        Item  = m_Tree.GetPrevSibling(Item);
        Image = m_Tree.GetItemImage(Item);
    }
    // IF ORDER BY exists, decrement count
    if (Image == qtiOrderBy)
        Count--;
    return Count;
}
//
// Returns column count for query specified by QueryNode
//
int CQueryDesigner::GetColumnCount(const wxTreeItemId &QueryNode)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId SelectNode = m_Tree.GetFirstChild(QueryNode, cookie);
    return (int)m_Tree.GetChildrenCount(SelectNode, false);
}
//
// Returns source table count for query specified by QueryNode
//
int CQueryDesigner::GetFromCount(const wxTreeItemId &QueryNode)
{
    return (int)m_Tree.GetChildrenCount(GetFromNode(QueryNode), false);
}
//
// Returns WHERE condition count for query specified by QueryNode
//
int CQueryDesigner::GetWhereCount(const wxTreeItemId &QueryNode)
{
    wxTreeItemId WhereNode = GetWhereNode(QueryNode);
    return WhereNode.IsOk() ? (int)m_Tree.GetChildrenCount(WhereNode, false) : 0;
}
//
// Returns GROUP BY expression count for query specified by QueryNode
//
int CQueryDesigner::GetGroupByCount(const wxTreeItemId &QueryNode)
{
    wxTreeItemId GroupByNode = GetGroupByNode(QueryNode);
    return GroupByNode.IsOk() ? (int)m_Tree.GetChildrenCount(GroupByNode, false) : 0;
}
//
// Returns Indext-th child for tree item Node
//
wxTreeItemId CQueryDesigner::GetItem(const wxTreeItemId &Node, int Index)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId      Item;
    for (Item = m_Tree.GetFirstChild(Node, cookie); Index && Item.IsOk(); Index--, Item = m_Tree.GetNextChild(Node, cookie));
    return Item;
}
//
// Returns tree item for table specified by unique ID
//
wxTreeItemId CQueryDesigner::GetItemByTableID(const wxTreeItemId &Node, int TableID)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId Result;
    int Image = m_Tree.GetItemImage(Node);
    // IF Node is FROM or join
    if (Image == qtiFrom || Image == qtiJoin)
    {
        // Scan Node children
        for (wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
        {
            Result = GetItemByTableID(Item, TableID);
            if (Result.IsOk())
                break;
        }
    }
    // ELSE IF TableID matches, found
    else
    {
        CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Node);
        if (Table->m_TableID == TableID)
            Result = Node;
    }
    return Result;
}
//
// Returns true if eventual SrcTable and DstTable join creates cycle
//
bool CQueryDesigner::MakesLinkCycle(int SrcTableID, int DstTableID)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId Item;
    // Create link list
    CCycleLinks  cls;
    // Add DstTable - SrcTable link to list
    CCycleLink *cl   = cls.acc2(0);
    cl->m_DstTableID = DstTableID;
    cl->m_SrcTableID = SrcTableID;
    cl->m_Visited    = false;

    // Add all join links to the list
    wxTreeItemId Node = GetFromNode(m_CurQuery);
    for (Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
    {
        CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Item);
        if (Table->m_Type == TT_JOIN)
            AddCycleLink(Item, (CQDJoin *)Table, &cls);
    }

    // Add all WHERE join conditions to the list
    CQDQTable *DstTable;
    CQDQTable *SrcTable;
    wxChar     DstAttr[OBJ_NAME_LEN + 3];  
    wxChar     SrcAttr[OBJ_NAME_LEN + 3];  
    Node     = GetWhereNode(m_CurQuery);
    if (Node.IsOk())
    {
        for (Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
        {
            CQDCond   *Cond = (CQDCond *)m_Tree.GetItemData(Item);
            if (!Cond->m_SubConds && WhereCondToJoin(Cond->m_Expr, &DstTable, DstAttr, &SrcTable, SrcAttr))
            {
                cl               = cls.acc2(cls.count());
                cl->m_DstTableID = DstTable->m_TableID;
                cl->m_SrcTableID = SrcTable->m_TableID;
                cl->m_Visited    = false;
            }
        }
    }
    // Check if list contains cycle
    return CheckLinkCycle(&cls, DstTableID, SrcTableID);
}
//
// Adds Join link to cls list
//
void CQueryDesigner::AddCycleLink(const wxTreeItemId &Node, CQDJoin *Join, CCycleLinks *cls)
{
    // Add join link to the list
    CCycleLink *cl   = cls->acc2(cls->count());
    cl->m_DstTableID = Join->m_DstTableID;
    cl->m_SrcTableID = Join->m_SrcTableID;
    cl->m_Visited    = false;
    wxTreeItemIdValue cookie;
    // IF join source tables are join, add child join links to the list
    for (wxTreeItemId Item = m_Tree.GetFirstChild(Node, cookie); Item.IsOk(); Item = m_Tree.GetNextChild(Node, cookie))
    {
        CQDQTable *Table = (CQDQTable *)m_Tree.GetItemData(Item);
        if (Table->m_Type == TT_JOIN)
            AddCycleLink(Item, (CQDJoin *)Table, cls);
    }
}
//
// Checks if table links in cls contain cycle
// Input:   cls       - List of all table links
//          ParentID  - Preceding table ID
//          TableID   - Checked table ID
// Returns: true if cycle found
//
bool CQueryDesigner::CheckLinkCycle(CCycleLinks *cls, int ParentID, int TableID)
{
    int NextID;
    // FOR each link in list
    for (int i = 0; i < cls->count(); i++)
    {
        CCycleLink *cl = cls->acc0(i);
        // IF source table is checked table, next checked is destination table
        if (cl->m_SrcTableID == TableID)
            NextID = cl->m_DstTableID;
        // IF destination table is checked table, next checked is source table
        else if (cl->m_DstTableID == TableID)
            NextID = cl->m_SrcTableID;
        // ELSE continue
        else
            continue;
        // IF ParentID - TableID link found, mark it and continue
        if (NextID == ParentID)
        {
            cl->m_Visited = true;
            continue;
        }
        // IF marked link, cycle found
        if (cl->m_Visited)
            return true;
        // Check links for NextID
        if (CheckLinkCycle(cls, TableID, NextID))
            return true;
        cl->m_Visited = true;
    }
    // Cycle not found
    return false;
}
//
// Converts Src to ASCII
//
tccharptr CQueryDesigner::Tochar(const wxChar *Src)
{
    tccharptr Result = cdp->sys_conv->cWC2MB(Src);
    if (Result)
        return Result;
    // IF conversion failed
    int Len = (int)wxStrlen(Src);
    int i;
    // Locate invalid character
    for (i = 1; i < Len && cdp->sys_conv->cWC2MB(wxString(Src, i).c_str()); i++);
    // Create error message
    wxString Msg(wxT("  \""));
    if (i > 16)
        Msg += wxT("... ") + wxString(Src + i - 17, 16);
    else
    {
        const wxChar *s = Src;
        if (*s == QUERY_TEST_MARK)
        {
            s++;
            i--;
        }
        Msg += wxString(s, i - 1);
    }
    Msg += wxT('|');
    if (i + 16 < Len)
        Msg += wxString(Src + i, 16) + wxT(" ...\"");
    else
        Msg += wxString(Src + i) + wxT('"');
    // Show error message
    error_box(wxString(_("Illegal character in the source text")) + Msg, this);
    return tccharptr((char *)NULL);
}

/////////////////////////// virtual methods (commented in class any_designer): //////////////////////////////////

t_tool_info CQueryDesigner::tool_info[TOOL_COUNT + 1] = 
{
    t_tool_info(TD_CPM_SAVE,         File_save_xpm,       wxTRANSLATE("Save design")),
    t_tool_info(TD_CPM_EXIT,         exit_xpm,            wxTRANSLATE("Exit")),
    t_tool_info(-1, NULL, NULL),
    t_tool_info(TD_CPM_CUT,          cut_xpm,             wxTRANSLATE("Cut Text")),
    t_tool_info(TD_CPM_COPY,         Kopirovat_xpm,       wxTRANSLATE("Copy Text")),
    t_tool_info(TD_CPM_PASTE,        Vlozit_xpm,          wxTRANSLATE("Paste Text")),
    t_tool_info(-1, NULL, NULL),
    t_tool_info(TD_CPM_UNDO,         Undo_xpm,            wxTRANSLATE("Undo")),
    t_tool_info(TD_CPM_REDO,         Redo_xpm,            wxTRANSLATE("Redo")),
    t_tool_info(-1, NULL, NULL),
    t_tool_info(TD_CPM_CHECK,        validateSQL_xpm,     wxTRANSLATE("Validate")),
    t_tool_info(TD_CPM_SQL,          editSQL_xpm,         wxTRANSLATE("Edit SQL")),  // wxITEM_CHECK),
//    t_tool_info(TD_CPM_OPTIMIZE,     optimize_xpm,        wxTRANSLATE("Query optimization")),
    t_tool_info(TD_CPM_XML_PARAMS,   params_xpm,          wxTRANSLATE("Parameters")),             
    t_tool_info(TD_CPM_DATAPREVIEW,  querypreview_xpm,    wxTRANSLATE("Data preview")),
//    t_tool_info(TD_CPM_DSGN_PARAMS,  tableProperties_xpm, wxTRANSLATE("Designer properties")),  // no space
    t_tool_info(-1, NULL, NULL),
    t_tool_info(TD_CPM_ELEMENTPOPUP, elementpopup_xpm,    wxTRANSLATE("Show panel with lexical elements")),
    t_tool_info(TD_CPM_ADDTABLE,     addtable_xpm,        wxTRANSLATE("Add new table")),
    t_tool_info(TD_CPM_DELETETABLE,  deletetable_xpm,     wxTRANSLATE("Delete selected table")),
    t_tool_info(-1, NULL, NULL),
    t_tool_info(TD_CPM_HELP,         help_xpm,            wxTRANSLATE("Help")),
    t_tool_info(0,  NULL, NULL)
};

//
// Creates designer menu
//
wxMenu * CQueryDesigner::get_designer_menu(void)
{ 
#ifndef RECREATE_MENUS
    if (designer_menu) return designer_menu;
#endif
    // create menu and add the common items: 
    any_designer::get_designer_menu();  
    // add specific items:
    AppendXpmItem(designer_menu, TD_CPM_SQL,          _("Edit SQL"),              editSQL_xpm);
    AppendXpmItem(designer_menu, TD_CPM_OPTIMIZE,     _("Query optimization"));
    AppendXpmItem(designer_menu, TD_CPM_XML_PARAMS,   _("&Parameters"),           params_xpm);
    AppendXpmItem(designer_menu, TD_CPM_DATAPREVIEW,  _("Data preview"),          querypreview_xpm);
    AppendXpmItem(designer_menu, TD_CPM_DSGN_PARAMS,  _("Designer properties"),   tableProperties_xpm);
    designer_menu->AppendSeparator();
    AppendXpmItem(designer_menu, TD_CPM_ELEMENTPOPUP, _("Lexical elements"),      elementpopup_xpm);
    AppendXpmItem(designer_menu, TD_CPM_ADDTABLE,     _("Add new table"),         addtable_xpm);        
    AppendXpmItem(designer_menu, TD_CPM_DELETETABLE,  _("Delete selected table"), deletetable_xpm);
    return designer_menu;
}

char *CQueryDesigner::make_source(bool alter)
{
    const wxChar *Src;
    // IF text editor is shown, Src = editor content 
    if (m_Edit.IsShown())
        Src = m_Edit.editbuf;
    // ELSE close cell editor, Src = resulting statement
    else
    {
        m_Grid->DisableCellEditControl();
        m_ToText = true;
        BuildResult();
        Src = m_Result;
    }
    // Convert Src to ASCII
    tccharptr Cmd = Tochar(Src);
    if (!Cmd)
        return NULL;
    char *Result = (char *)corealloc(strlen(Cmd) + 1, 89);
    if (Result)
        strcpy(Result, Cmd);
    return Result; 
}

void CQueryDesigner::set_designer_caption(void)
{ 
    wxString caption;
    if (!modifying())
        caption = _("Design a New Query");
    else
        caption.Printf(_("Editing Query %s"), wxString(object_name, *cdp->sys_conv).c_str());
    set_caption(caption);
}

bool CQueryDesigner::IsChanged(void) const
{ 
    return changed || m_Edit.IsChanged();
}

wxString CQueryDesigner::SaveQuestion(void) const
{ 
    return modifying() ?
        _("Your changes have not been saved. Do you want to save your changes?") :
        _("The query object has not been created yet. Do you want to save the query?");
}

bool CQueryDesigner::save_design(bool save_as)
// Saves the domain, returns true on success.
{ 
#ifdef LINUX
    const char *Src = make_source(false);
    if (!Src)
        return false;
    corefree(Src);
#endif
    if (!save_design_generic(save_as, object_name, 0))  // using the generic save from any_designer
        return false;  
    inval_table_d(cdp, objnum, CATEG_CURSOR);
    changed = false;
    m_Edit.SetChanged(false);
    set_designer_caption();
    return true;
}

/////////////////////////////////// Designer grids - common ancestor ////////////////////////////////////////

//
// Constructor
//
CQDGrid::CQDGrid(CQueryDesigner *Designer, TQTreeImg ID) : wxGrid(&Designer->m_Props, ID), m_ToolTip(this)
{
    m_Designer = Designer;
}
//
// Destruction
//
bool CQDGrid::Destroy()
{
    if ( !wxPendingDelete.Member((wxGrid *)this) )
        wxPendingDelete.Append((wxGrid *)this);

    Hide();
    Reparent(m_Designer);

    return true;
}
//
// Returns height of given font
//
static int font_space(const wxFont &font)
{ 
    wxScreenDC dc;
    dc.SetFont(font);
    wxCoord w, h, descent, externalLeading;
    dc.GetTextExtent(wxT("pQgy�"), &w, &h, &descent, &externalLeading);
    return (h+descent+externalLeading) * 5 / 4;
}
//
// Returns row height by height of combobox
//
int CQDGrid::RowSize()
{
    wxOwnerDrawnComboBox cb(this, 0, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL);
    return cb.GetSize().y - 1;
}
//
// Opens grid for query element given by Node
//
void CQDGrid::Open(const wxTreeItemId &Node)
{
    m_Node = Node;
    BeginBatch();
    SetLabelFont(*grid_label_font);
    SetDefaultCellFont(*grid_cell_font);
    SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
    SetColLabelSize(font_space(*grid_label_font));
    SetDefaultRowSize(RowSize());
    if (!SetTable(this))
        return;
    SetRowLabelSize(24);
    SetColLabelSize(20);
    EnableDragRowSize(false);
    OnOpen();
    m_Designer->m_Props.Resize();
    SetDropTarget(new CQDDropTargetPG(this));
    EndBatch();
}
//
// Returns query element record for given row
//
wxTreeItemData *CQDGrid::GetData(int Row)
{
    wxTreeItemId Item = GetItem(Row);
    if (Item.IsOk())
        return m_Designer->m_Tree.GetItemData(Item);
    else
        return NULL;
}
//
// Adds new editable column to grid
//
void CQDGrid::AddTextCol(int Col, int Width)
{
    SetColSize(Col,    Width);
    wxGridCellAttr *CellAttr = new wxGridCellAttr();
    if (CellAttr)
    {
        CellAttr->SetOverflow(false);
        wxGridCellTextEditor *CellEdit = new CQDGridTextEditor(this);
        if (CellEdit)
            CellAttr->SetEditor(CellEdit);
        wxGrid::SetColAttr(Col, CellAttr);
    }
}
//
// Adds new static (readonly) column to grid
//
void CQDGrid::AddStaticCol(int Col, int Width)
{
    SetColSize(Col,    Width);
    wxGridCellAttr *CellAttr = new wxGridCellAttr();
    if (CellAttr)
    {
        CellAttr->SetReadOnly();
        wxGrid::SetColAttr(Col, CellAttr);
    }
}
//
// Adds new combo column to grid
//
void CQDGrid::AddComboCol(int Col, int Width, const wxString &Param)
{
    SetColSize(Col,    Width);
    wxGridCellAttr *CellAttr = new wxGridCellAttr();
    if (CellAttr)
    {
        wxGridCellODChoiceEditor *CellEdit = new wxGridCellODChoiceEditor();
        if (CellEdit)
        {
            CellEdit->SetParameters(Param);
            CellAttr->SetEditor(CellEdit);
        }
        wxGrid::SetColAttr(Col, CellAttr);
    }
}
//
// Adds new checkbox column to grid
//
void CQDGrid::AddBoolCol(int Col, int Width)
{
    SetColSize(Col,    Width);
    wxGridCellAttr *CellAttr = new wxGridCellAttr();
    if (CellAttr)
    {
        wxGridCellBoolEditor *CellEdit = new wxGridCellBoolEditor();
        if (CellEdit)
            CellAttr->SetEditor(CellEdit);
        wxGridCellBoolRenderer *BoolRenderer = new wxGridCellBoolRenderer();
        if (BoolRenderer)
            CellAttr->SetRenderer(BoolRenderer);
        CellAttr->SetAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
        wxGrid::SetColAttr(Col, CellAttr);
    }
}
//
// Adds fictive column (last empty column) to grid
//
void CQDGrid::AddFictCol(int Col)
{
    wxGridCellAttr *CellAttr = new wxGridCellAttr();
    if (CellAttr)
    {
        CellAttr->SetReadOnly();
        wxGrid::SetColAttr(Col, CellAttr);
    }
}
//
// Sets width of last empty column
//
void CQDGrid::Resize()
{
    int Width, Height;
    GetClientSize(&Width, &Height);
    int LastCol = wxGrid::GetNumberCols() - 1;
    wxRect Rect = CellToRect(0, LastCol);
    Width -= Rect.x + 29; //+ wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
    if (Width > 0)
        SetColSize(LastCol, Width);
}
//
// Asks tree for needed row count
//
int CQDGrid::GetNumberRows()
{
    return (int)m_Designer->m_Tree.GetChildrenCount(m_Node, false) + 1;
}
//
// Redraws grid
//
void CQDGrid::Refresh()
{
    int TableRows = GetNumberRows();
    int GridRows  = wxGrid::GetNumberRows();
    int RowInc    = TableRows - GridRows;
    // IF row count changed
    if (RowInc)
    {
        // IF row count increased, append rows
        if (RowInc > 0)
        {
            wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, RowInc);
            GetView()->ProcessTableMessage(msg);
        }
        // ELSE delete rows
        else
        {
            wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED,  GridRows, -RowInc);
            GetView()->ProcessTableMessage(msg);
        }
    }
    ForceRefresh();
}

BEGIN_EVENT_TABLE(CQDGrid, wxGrid)
    EVT_GRID_EDITOR_CREATED(CQDGrid::OnCreateEditor)
    EVT_GRID_COL_SIZE(CQDGrid::OnColSize)
    EVT_KEY_DOWN(CQDGrid::OnKeyDown) 
END_EVENT_TABLE()
//
// Cell editor created event handler, appends my EventHandler to editor
// 
void CQDGrid::OnCreateEditor(wxGridEditorCreatedEvent &event)
{
    if (IsTextEditor(event.GetCol()))
    {
        CQDGridTextEditor *Edit = (CQDGridTextEditor *)(wxGridCellTextEditor *)GetCellEditor(event.GetRow(), event.GetCol());
        event.GetControl()->PushEventHandler(Edit);
#ifdef LINUX        
        UpdateSize(Edit->GetCtrl());
#endif        
    }
}
//
// Column size changed event handler
//
void CQDGrid::OnColSize(wxGridSizeEvent &event)
{
    int Col      = event.GetRowOrCol();
    int *pWidths = GetColWidths();
    pWidths[Col] = GetColSize(Col);
}
//
// Key down event handler
//
void CQDGrid::OnKeyDown(wxKeyEvent &event)
{
    event.Skip();
    int Code = event.GetKeyCode();
    // Delete
    if (Code == WXK_DELETE)
    {
        OnDelete();
        event.Skip(false);
    }
    // Ctrl/C
    else if (Code == 'C' && event.ControlDown())
        OnCopy();
    // Ctrl/X
    else if (Code == 'X' && event.ControlDown())
        OnCut();
    // Ctrl/V
    else if (Code == 'V' && event.ControlDown())
        OnPaste();
    else
        event.ResumePropagation(2);
}
//
// Delete action
//
void CQDGrid::OnDelete()
{
    int Row;
    int GridId = GetId();
    wxArrayInt Rows = GetSelectedRows();
    // IF no row selected
    int Count = (int)Rows.Count();
    if (!Count)
    {
        // Get slected cell
        Row     = GetGridCursorRow();
        int Col = GetGridCursorCol();
        wxTreeItemData *Data;
        // Get corresponding record
        if (GridId == qtiJoin || GridId == qtiLimit)
            Data = m_Designer->m_Tree.GetItemData(m_Node);
        else
            Data = GetData(Row);
        if (!Data)
            return;
        switch (GridId)
        {
        case qtiQueryExpr:
            // IF BY column, clear BY condition
            if (Col == 4)
            {
                ((CQDQueryExpr *)Data)->m_By.Clear();
                ForceRefresh();
                m_Designer->changed = true;
            }
            return;
        case qtiSelect:
            // IF expression column, delete whole column 
            if (Col == 0)
            {
                Count = 1;
                break;
            }
            // IF alias column, delete Alias
            else if (Col == 1)
            {
                ((CQDColumn *)Data)->m_Alias.Clear();
                ForceRefresh();
                m_Designer->changed = true;
            }
            return;
        case qtiFrom:
            // IF table name column, delete whole table
            if (Col == 0)
            {
                Count = 1;
                break;
            }
            // IF alias column, delete Alias
            else if (Col == 1)
            {
                ((CQDQTable *)Data)->m_Alias.Clear();
                ForceRefresh();
                wxTreeItemId Item   = GetItem(Row);
                // Set new caption of tree item
                wxString Title      = m_Designer->GetTableName((CQDQTable *)Data, GTN_ALIAS | GTN_TITLE);
                m_Designer->m_Tree.SetItemText(Item, Title);
                // Set new title of TableBox
                if (((CQDQTable *)Data)->m_TableBox)
                    ((CQDQTable *)Data)->m_TableBox->SetTitle(Title);
                m_Designer->changed = true;
            }
            // IF INDEX column, delete Index (INDEX not supported)
            else if (Col == 2 && ((CQDQTable *)Data)->m_Type == TT_TABLE)
            {
                ((CQDDBTable *)Data)->m_Index.Clear();
                ForceRefresh();
                m_Designer->changed = true;
            }
            return;
        case qtiJoin:
            // IF join condition column, delete join condition
            if (Col == 4)
            {
                ((CQDJoin *)Data)->m_Cond.Clear();
                ForceRefresh();
                m_Designer->changed = true;
            }
            // IF alias column, smazat alias
            else if (Col == 2 && ((CQDQTable *)Data)->m_Type == TT_TABLE)
            {
                ((CQDQTable *)Data)->m_Alias.Clear();
                ForceRefresh();
                wxTreeItemId Item   = GetItem(Row);
                // Set new caption of tree item
                wxString Title      = m_Designer->GetTableName((CQDQTable *)Data, GTN_ALIAS | GTN_TITLE);
                m_Designer->m_Tree.SetItemText(Item, Title);
                // Set new title of TableBox
                if (((CQDQTable *)Data)->m_TableBox)
                    ((CQDQTable *)Data)->m_TableBox->SetTitle(Title);
                m_Designer->changed = true;
            }
            return;
        case qtiWhere:
        case qtiHaving:
        case qtiSubConds:
            // IF expression column, delete whole condition
            if (Col == 0)
            {
                Count = 1;
                break;
            }
            // IF assosciation column, remove association
            else if (Col == 1)
            {
                wxTreeItemId Item = GetItem(Row);
                CQDCond *Cond = (CQDCond *)m_Designer->m_Tree.GetItemData(Item);
                Cond->m_Assoc = CA_NONE;
                ForceRefresh();
                m_Designer->changed = true;
            }
            return;
        case qtiGroupBy:
        case qtiOrderBy:
            // IF expression column, delete whole expression
            if (Col == 0)
            {
                Count = 1;
                break;
            }
            return;
        case qtiLimit:
            // IF offset column, delete whole LIMIT
            if (Col == 0)
            {
                Count = 1;
                break;
            }
            // IF count column, delete Count
            else if (Col == 1)
            {
                ((CQDLimit *)Data)->m_Count.Clear();
                ForceRefresh();
                m_Designer->changed = true;
            }
            return;
        }
    }
    // ELSE one whole row selected
    else if (Count == 1)
        Row = Rows[0];
    if (GridId == qtiQueryExpr)
        return;
    // IF one whole row selected
    if (Count == 1)
    {
        // IF join or LIMIT, initialize delayed delete
        if (GridId == qtiJoin || GridId == qtiLimit)
            m_Designer->m_DeletedItem = m_Node;
        // ELSE delete item
        else
        {
            wxTreeItemId Item = GetItem(Row);
            if (!Item.IsOk())
                return;
            m_Designer->DeleteItem(Item);
        }
    }
    // ELSE multiple rows selected
    else
    {
        // Ask user
        wxString Prompt;
        switch (GridId)
        {
        case qtiSelect:
            Prompt = _("Do you realy want to delete the selected colums?");
            break;
        case qtiFrom:
            Prompt = _("Do you realy want to delete the selected tables?");
            break;
        case qtiWhere:
        case qtiHaving:
        case qtiSubConds:
            Prompt = _("Do you realy want to delete the selected conditions?");
            break;
        case qtiGroupBy:
        case qtiOrderBy:
            Prompt = _("Do you realy want to delete the selected expressions?");
            break;
        }
        if (!yesno_box(Prompt, m_Designer->dialog_parent()))
            return;
        if (Count > 100)
            Count = 100;
        wxTreeItemId Items[100];
        int i, j;
        // Fill Items with valid tree items
        for (i = 0, j = 0; i < Count; i++)
        {
            Items[j] = GetItem(Rows[i]);
            SelectRow(Rows[i], false);
            if (Items[j].IsOk())
                j++;
        }
        Count = j;
        // Delete selected items
        switch (GridId)
        {
        case qtiSelect:
        case qtiGroupBy:
        case qtiOrderBy:
            for (i = 0; i < Count; i++)
                m_Designer->m_Tree.Delete(Items[i]);
            Refresh();
            break;    
        case qtiFrom:
            for (i = 0; i < Count; i++)
                m_Designer->DeleteTable(Items[i], false);
            break;
        case qtiWhere:
        case qtiHaving:
        case qtiSubConds:
            for (i = 0; i < Count; i++)
                m_Designer->DeleteCond(Items[i], false);
            break;
        }
    }
}

#define CAN_COPY  1
#define CAN_PASTE 2

uns8 CCPFlags[][6] =
{
    {CAN_COPY, CAN_COPY, 0, 0, CAN_COPY, CAN_COPY},                                                                 // qtiQueryExpr
    {0, 0, 0, 0, 0, 0},                                                                                             // qtiQuery
    {CAN_COPY | CAN_PASTE, CAN_COPY | CAN_PASTE, 0, 0, 0, 0},                                                       // qtiSelect
    {CAN_COPY | CAN_PASTE, CAN_COPY | CAN_PASTE, 0, 0, 0, 0},                                                       // qtiFrom
    {CAN_COPY | CAN_PASTE, CAN_COPY, 0, 0, 0, 0},                                                                   // qtiWhere
    {CAN_COPY | CAN_PASTE, 0, 0, 0, 0, 0},                                                                          // qtiGroupBy
    {CAN_COPY | CAN_PASTE, CAN_COPY, 0, 0, 0, 0},                                                                   // qtiHaving
    {CAN_COPY | CAN_PASTE, 0, 0, 0, 0, 0},                                                                          // qtiOrderBy
    {0, 0, 0, 0, 0, 0},                                                                                             // qtiTable
    {CAN_COPY | CAN_PASTE, CAN_COPY, CAN_COPY | CAN_PASTE, CAN_COPY, CAN_COPY | CAN_PASTE, CAN_COPY | CAN_PASTE},   // qtiJoin
    {0, 0, 0, 0, 0, 0},                                                                                             // qtiColumn
    {0, 0, 0, 0, 0, 0},                                                                                             // qtiCond
    {0, 0, 0, 0, 0, 0},                                                                                             // qtiSubConds
    {0, 0, 0, 0, 0, 0},                                                                                             // qtiGColumn
    {0, 0, 0, 0, 0, 0},                                                                                             // qtiOColumn
    {CAN_COPY | CAN_PASTE, CAN_COPY | CAN_PASTE, 0, 0, 0, 0}                                                        // qtiLimit
};
//
// Copy action
//
void CQDGrid::OnCopy()
{
    int GridId = GetId();
    int Row    = GetGridCursorRow();
    int Col    = GetGridCursorCol();
    // EXIT IF Item cannot be copied OR cell is empty 
    if (!(CCPFlags[GridId][Col] & CAN_COPY) || IsEmptyCell(Row, Col))
        return;
    // Copy cell value to clipboard
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(GetValue(Row, Col)));
        wxTheClipboard->Close();
    }
}
//
// Cut action
//
void CQDGrid::OnCut()
{
    int GridId = GetId();
    int Row    = GetGridCursorRow();
    int Col    = GetGridCursorCol();
    // EXIT IF Item cannot be copied OR cell is empty 
    if (!(CCPFlags[GridId][Col] & CAN_COPY) || IsEmptyCell(Row, Col))
        return;
    // Copy cell value to clipboard
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(GetValue(Row, Col)));
        wxTheClipboard->Close();
    }
    // IF item can be pasted, clear cell content
    if (CCPFlags[GridId][Col] & CAN_PASTE)
        SetValue(Row, Col, wxString());
}
//
// Paste action
// 
void CQDGrid::OnPaste()
{
    int GridId = GetId();
    int Row    = GetGridCursorRow();
    int Col    = GetGridCursorCol();
    // EXIT IF Item cannot be pasted
    if (!(CCPFlags[GridId][Col] & CAN_PASTE))
        return;
    // Paste text from clipboard
    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->IsSupported( wxDF_TEXT ))
        {
            wxTextDataObject Text;
            wxTheClipboard->GetData(Text);
            wxString Val = Text.GetText();
            if (!Val.IsEmpty())
                SetValue(Row, Col, Val);
        }
        wxTheClipboard->Close();
    }
}

////////////////////////////////////////// My grid cell editor //////////////////////////////////////////////

BEGIN_EVENT_TABLE(CQDGridTextEditor, wxEvtHandler)
    EVT_KEY_DOWN(CQDGridTextEditor::OnKeyDown)
    EVT_KILL_FOCUS(CQDGridTextEditor::OnKillFocus)
END_EVENT_TABLE()
//
// Key down event handler
//
void CQDGridTextEditor::OnKeyDown(wxKeyEvent &event)
{
    int Code = event.GetKeyCode();
    // IF Down OR Up, close grid cell editor, save changes to item record
    if (Code == WXK_DOWN || (Code == WXK_UP && m_Grid->GetCursorRow() > 0))
    {
        m_Grid->DisableCellEditControl();
        m_Grid->ProcessEvent(event);
        return;
    }
    event.Skip();
}
//
// Kill focus event handler
//
void CQDGridTextEditor::OnKillFocus(wxFocusEvent &event)
{
    // Close grid cell editor, save changes to item record
    m_Grid->DisableCellEditControl();
    event.Skip();
}

////////////////////////////////////////// Query expression grid ////////////////////////////////////////////

int CQDQueryGrid::ColWidth[6] = {100, 100, 30, 120, 200, 100};
//
// OnOpen virtual function implementation - adds needed columns
//
void CQDQueryGrid::OnOpen()
{
    AddStaticCol(0,  ColWidth[0]);                                  // First Query
    AddComboCol(1,   ColWidth[1], wxT("UNION,EXCEPT,INTERSECT"));   // Association
    AddBoolCol(2,    ColWidth[2]);                                  // ALL
    AddBoolCol(3,    ColWidth[3]);                                  // CORRESPONDING
    AddTextCol(4,    ColWidth[4]);                                  // BY
    AddStaticCol(5,  ColWidth[5]);                                  // Second query
}
//
// Returns true for editable column - BY only is editable
//
bool CQDQueryGrid::IsTextEditor(int Col)
{
    return Col == 4;
}
//
// Returns row count = subquery count - 1
//
int CQDQueryGrid::GetNumberRows()
{
    return m_Designer->GetQueryCount(m_Node) - 1;
}
//
// Returns column count
//
int CQDQueryGrid::GetNumberCols()
{
    return 6;
}
//
// Returns true if cell value is empty
//
bool CQDQueryGrid::IsEmptyCell(int row, int col)
{
    if (col != 4)
        return false;
    CQDUQuery *Query = (CQDUQuery *)GetData(row);
    return Query->m_By.IsEmpty();
}
//
// Returns cell value
//
wxString CQDQueryGrid::GetValue(int row, int col)
{
    CQDUQuery *Query = (CQDUQuery *)GetData(col == 5 ? row + 1 : row);
    // First OR second subquery
    if (col == 0 || col == 5)
        return m_Designer->GetTableName(Query, GTN_ALIAS);
    // Association
    if (col == 1)
        return wxString(Query->m_Assoc == QA_INTERSECT ? wxT("INTERSECT") : Query->m_Assoc == QA_EXCEPT ? wxT("EXCEPT") : wxT("UNION"));
    // ALL
    if (col == 2)
        return Query->m_All ? wxT("1") : wxString();
    // CORRESPONDING
    if (col == 3)
        return Query->m_Corresp ? wxT("1") : wxString();
    // BY
    else
        return Query->m_By;
}
//
// Stores cell value to record
//
void CQDQueryGrid::SetValue(int row, int col, const wxString& value)
{
    CQDUQuery *Query = (CQDUQuery *)GetData(row);
    // Association
    if (col == 1)
    {
        if (wxStrcmp((const wxChar *)value, wxT("INTERSECT")) == 0)
            Query->m_Assoc = QA_INTERSECT;
        else if (wxStrcmp((const wxChar *)value, wxT("EXCEPT")) == 0)
            Query->m_Assoc = QA_EXCEPT;
        else
            Query->m_Assoc = QA_UNION;
        wxTreeItemId  Item = GetItem(row);
        // Reset tree item caption
        wxString   Caption = m_Designer->GetTableName(Query, GTN_ALIAS | GTN_TITLE) +  wxT(" - ") + QueryAssocTbl[Query->m_Assoc];
        m_Designer->m_Tree.SetItemText(Item, Caption);
    }
    // ALL
    else if (col == 2)
        Query->m_All = value[0] == wxT('1');
    // CORRESPONDING
    else if (col == 3)
        Query->m_Corresp = value[0] == wxT('1');
    // BY
    else
    {
        Query->m_By = value;
    }
    m_Designer->changed           = true;
    m_Designer->m_CurQueryChanged = true;
}
//
// Returns column heading
//
wxString CQDQueryGrid::GetColLabelValue(int col)
{
    // Subquery
    if (col == 0 || col == 5)
        return wxString(tnQuery);
    // Association
    if (col == 1)
        return wxString(_("Relation"));
    // ALL
    if (col == 2)
        return wxString(wxT("ALL"));
    // CORRESPONDING
    if (col == 3)
        return wxString(wxT("CORRESPONDING"));
    // BY
    else 
        return wxString(wxT("BY"));
}

//
// Returns true if cell can accept draged data - BY only
//
bool CQDQueryGrid::OnDragOver(int Row, int Col)
{
    return Col == 4;
}

///////////////////////////////////////////// SELECT grid ///////////////////////////////////////////////////

int CQDSelectGrid::ColWidth[3] = {200, 100, 0};
//
// OnOpen virtual function implementation
//
void CQDSelectGrid::OnOpen()
{
    // Add columns
    AddTextCol(0,    ColWidth[0]);  // Expression
    AddTextCol(1,    ColWidth[1]);  // Alias
    AddFictCol(2);
    // Create DISTINCT checkbox
#ifdef WINS    
    m_DistinctCB.Create(&m_Designer->m_Props, QDID_DISTINCT, wxT("DISTINCT"), wxPoint(6, 5));
#else    
    m_DistinctCB.Create(&m_Designer->m_Props, QDID_DISTINCT, wxT("DISTINCT"), wxPoint(6, 0));
#endif    
    UpdateSize(&m_DistinctCB);
    // Set DISTINCT checkbox value
    CQDQuery *Query   = (CQDQuery *)m_Designer->m_Tree.GetItemData(m_Designer->m_CurQuery);
    m_DistinctCB.SetValue(Query->m_Distinct);
}
//
// Destruction
//
bool CQDSelectGrid::Destroy()
{
    m_DistinctCB.Hide();
    m_DistinctCB.Reparent(m_Designer);
    return CQDGrid::Destroy();
}
//
// Returns true for editable column - expression and alias are editable
//
bool CQDSelectGrid::IsTextEditor(int Col)
{
    return Col <= 1;
}
//
// Returns column count
//
int CQDSelectGrid::GetNumberCols()
{
    return 3;
}
//
// Returns true if cell value is empty
//
bool CQDSelectGrid::IsEmptyCell(int row, int col)
{
    if (row >= GetNumberRows() - 1 || col >= 2)
        return true;
    CQDColumn *Column = (CQDColumn *)GetData(row);
    if (col)
        return Column->m_Alias.IsEmpty();
    else
        return Column->m_Expr.IsEmpty();
}
//
// Returns cell value
//
wxString CQDSelectGrid::GetValue(int row, int col)
{
    if (row >= GetNumberRows() - 1 || col >= 2)
        return wxString();
    CQDColumn *Column = (CQDColumn *)GetData(row);
    if (col)
        return Column->m_Alias.IsEmpty() ? Column->m_Alias : GetProperIdent(Column->m_Alias);
    else
        return Column->m_Expr;
}
//
// Stores cell value to record
//
void CQDSelectGrid::SetValue(int row, int col, const wxString& value)
{
    wxTreeItemId Item;
    CQDColumn   *Column;
    // IF old row, get column record
    if (row < GetNumberRows() - 1)
    {
        Item   = GetItem(row);
        Column = (CQDColumn *)m_Designer->m_Tree.GetItemData(Item);
    }
    // ELSE new row, create new record, append new tree item, set record as its ItemData
    else
    {
        Column = new CQDColumn();
        Item   = m_Designer->m_Tree.AppendItem(m_Node, value, qtiColumn, qtiColumn, Column);
        m_Designer->m_Tree.wxWindow::Refresh();
        Refresh();
    }
    // IF Alias, store value, set tree item caption
    if (col)
    {
        Column->m_Alias = value;
        if (value.IsEmpty())
            m_Designer->m_Tree.SetItemText(Item, Column->m_Expr);
        else
            m_Designer->m_Tree.SetItemText(Item, Column->m_Alias);
    }
    // ELSE expression
    else
    {
        CQColumn Expr;
        cdp_t cdp = m_Designer->cdp;
        // Compile expression because of subqueries
        int Err = compile_column(cdp, value.mb_str(*cdp->sys_conv), &Expr);
        m_Designer->m_Tree.DeleteChildren(Item);
        // IF syntax error, show message, mark column
        if (Err)
        {
            client_error(cdp, Err);  
            cd_Signalize2(cdp, m_Designer->dialog_parent());
            Column->m_Postfix.Clear(); 
            Column->m_Error = true;
        }
        // ELSE Rebuild column tree item
        else
        {
            Column->m_Postfix = wxString(Expr.Postfix, *cdp->sys_conv);
            m_Designer->LoadColumnQueries(Item, &Expr.Queries);
            Column->m_Error = false;
        }
        // Store expression
        Column->m_Expr  = value;
        // IF alias is empty, reset tree item caption
        if (Column->m_Alias.IsEmpty())
            m_Designer->m_Tree.SetItemText(Item, GetNakedIdent(value));
    }
    m_Designer->changed           = true;
    m_Designer->m_CurQueryChanged = true;
}
//
// Returns column heading
//
wxString CQDSelectGrid::GetColLabelValue(int col)
{
    // Expression
    if (col == 0)
        return wxString(tnExpression);
    // Alias
    if (col == 1)
        return wxString(tnAlias);
    else
        return wxString();
}
//
// Returns true if cell can accept draged data
//
bool CQDSelectGrid::OnDragOver(int Row, int Col)
{
    // Column is expression AND first row OR first column is not *
    return Col == 0 && (Row == 0 || wxStrcmp(GetValue(0, 0), wxT("*")) != 0);
}

/////////////////////////////////////////////// FROM grid /////////////////////////////////////////////////

int CQDFromGrid::ColWidth[3] = {150, 150, 0};
//
// OnOpen virtual function implementation - adds needed columns
//
void CQDFromGrid::OnOpen()
{
    AddTextCol(0,    ColWidth[0]);  // Table name
    AddTextCol(1,    ColWidth[1]);  // Alias
    AddTextCol(2,    0);
}
//
// Returns true for editable column - name and alias are editable
//
bool CQDFromGrid::IsTextEditor(int Col)
{
    return Col <= 1; // || m_Designer->m_ShowIndex);
}
//
// Returns column count
//
int CQDFromGrid::GetNumberCols()
{
    return 3;
}
//
// Returns true if cell value is empty
//
bool CQDFromGrid::IsEmptyCell(int row, int col)
{
    if (row >= GetNumberRows() - 1 || col >= 3)
        return true;
    CQDQTable *Table = (CQDQTable *)GetData(row);
    if (col == 0)
        return m_Designer->GetTableName(Table, 0).IsEmpty();
    if (col == 1)
        return Table->m_Alias.IsEmpty();
    else
    {
        //SetReadOnly(row, 2, !m_Designer->m_ShowIndex || Table->m_Type != TT_TABLE);
        //return Table->m_Type != TT_TABLE || ((CQDDBTable *)Table)->m_Index.IsEmpty();
        return true;
    }
}
//
// Returns cell value
//
wxString CQDFromGrid::GetValue(int row, int col)
{
    if (row >= GetNumberRows() - 1 || (col >= 2 && !m_Designer->m_ShowIndex))
        return wxString();
    CQDQTable *Table = (CQDQTable *)GetData(row);
    if (col == 0)
    {
        return m_Designer->GetTableName(Table, 0);
    }
    else if (col == 1)
    {
        return Table->m_Alias.IsEmpty() ? Table->m_Alias : GetProperIdent(Table->m_Alias);
    }
    else
    {
        //if (Table->m_Type == TT_TABLE)
        //    return ((CQDDBTable *)Table)->m_Index;
        //else
            return wxString();
    }
}
//
// Stores cell value to record
//
void CQDFromGrid::SetValue(int row, int col, const wxString& value)
{
    wxTreeItemId Item;
    CQDQTable *Table;
    // IF old row, get table record
    if (row < GetNumberRows() - 1)
    {
        Item   = GetItem(row);
        Table = (CQDQTable *)m_Designer->m_Tree.GetItemData(Item);
    }
    // ELSE new row, create new record, append new tree item, set record as its ItemData
    else
    {
        Table = new CQDDBTable();
        Item  = m_Designer->m_Tree.AppendItem(m_Node, wxEmptyString, qtiTable, qtiTable, Table); 
        m_Designer->m_Tree.wxWindow::Refresh();
        Refresh();
    }
    // IF table name
    if (col == 0)
    {
        wxString SchemaName;
        wxString TableName;
        // Parse name to schema name and table name
        if (ScanValue(value, SchemaName, TableName))
        {
            // Store values to record
            ((CQDDBTable *)Table)->m_Schema = SchemaName;
            ((CQDDBTable *)Table)->m_Name   = TableName;
            // Reset tree item caption
            if (Table->m_Alias.IsEmpty())
                m_Designer->m_Tree.SetItemText(Item, value);
            // Reset TableBox title
            if (Table->m_TableBox)
                m_Designer->m_Tables.DeleteTable(Table->m_TableBox);
            // Add table to table panel
            m_Designer->AddTable(value, CATEG_TABLE, GetValue(row, 1), Item);
        }
    }
    // IF alias
    else if (col == 1)
    {
        // Store value to record
        Table->m_Alias = value;
        wxString Title = m_Designer->GetTableName(Table, GTN_ALIAS | GTN_TITLE);
        // Reset tree item caption
        m_Designer->m_Tree.SetItemText(Item, Title);
        // Reset TableBox title
        if (Table->m_TableBox)
            Table->m_TableBox->SetTitle(Title);
    }
    //else
    //{
    //    ((CQDDBTable *)Table)->m_Index = value;   
    //}
    m_Designer->changed           = true;
    m_Designer->m_CurQueryChanged = true;
}
//
// If Value is table name, returns true and table specitication in SchemaName and TableName
// If Value is subquery, returns false
//
bool CQDFromGrid::ScanValue(const wxString &Value, wxString &SchemaName, wxString &TableName)
{
    const wxChar *ps = Value.c_str();
    const wxChar *pe;
    // IF Value begins with `, return TableName without `` 
    if (*ps == '`')
    {
        pe = wxStrchr(ps + 1, '`');
        if (!pe)
        {
            TableName = Value;
            return true;
        }
        TableName = Value.Mid(1, pe - ps - 1);
        pe++;
    }
    // IF Value begins with (, return false
    else if (*ps == '(')
    {
        return false;
    }
    // ELSE IF Value doesn't contain ., TableName = Value
    else
    {
        pe = wxStrchr(ps + 1, '.');
        if (!pe)
        {
            TableName = Value;
            return true;
        }
    }
    // IF Value contains .
    if (*pe == '.')
    {
        // SchemaName = TableName
        SchemaName = TableName;
        ps = pe + 1;
        // IF `, remove ``, set TableName
        if (*ps == '`')
        {
            pe = wxStrchr(ps + 1, '`');
            if (!pe)
            {
                TableName = wxString(ps + 1);
                return true;
            }
            TableName = Value.Mid(1, pe - ps - 1);
            pe++;
            if (*pe == 0)
                return true;
        }
        TableName = wxString(ps);
        return true;
    }
    else if (*pe != 0)
    {
        TableName = Value;
    }
    return true;
}
//
// Returns column heading
//
wxString CQDFromGrid::GetColLabelValue(int col)
{
    // Name
    if (col == 0)
        return wxString(tnName);
    // Alias
    if (col == 1)
        return wxString(tnAlias);
    //if (m_Designer->m_ShowIndex)
    //    return wxT("Index");
    else
        return wxString();
}
//
// Returns true if cell can accept draged data - cannot
//
bool CQDFromGrid::OnDragOver(int Row, int Col)
{
    return false;
}

BEGIN_EVENT_TABLE(CQDFromGrid, CQDGrid)
    EVT_GRID_SELECT_CELL(CQDFromGrid::OnSelChange)
    EVT_GRID_LABEL_LEFT_CLICK(CQDFromGrid::OnSelChange) 
END_EVENT_TABLE()
//
// Select cell, left click event handler, selects corresponding TableBox
//
void CQDFromGrid::OnSelChange(wxGridEvent &event)
{
    event.Skip();
    CQDQTable *Table = (CQDQTable *)GetData(event.GetRow());
    if (!Table)
        return;
    if (Table->m_TableBox && Table->m_TableBox != m_Designer->m_Tables.m_SelTable)
        m_Designer->m_Tables.Select(Table->m_TableBox);
}

/////////////////////////////////////////////// Join grid /////////////////////////////////////////////////

int CQDJoinGrid::ColWidth[6] = {110, 60, 110, 50, 200, 100};
//
// OnOpen virtual function implementation - adds needed columns
//
void CQDJoinGrid::OnOpen()
{
    AddTextCol(0,  ColWidth[0]);                                                // First table name
    AddComboCol(1, ColWidth[1], wxT("INNER,LEFT,RIGHT,FULL,NATURAL,CROSS"));    // Join type
    AddTextCol(2,  ColWidth[2]);                                                // Second table name
    AddComboCol(3, ColWidth[3], wxT("ON,USING"));                               // ON/USING
    AddTextCol(4,  ColWidth[4]);                                                // Condition
    AddTextCol(5,  ColWidth[5]);                                                // Alias
}
//
// Returns true for editable column - First, second tablename, condition or alias 
//
bool CQDJoinGrid::IsTextEditor(int Col)
{
    return Col == 0 || Col == 2 || Col == 4 || Col == 5;
}
//
// Returns row count - allways 1
//
int CQDJoinGrid::GetNumberRows()
{
    return 1;
}
//
// Returns column count
//
int CQDJoinGrid::GetNumberCols()
{
    return 6;
}
//
// Returns true if cell value is empty
//
bool CQDJoinGrid::IsEmptyCell(int row, int col)
{
    CQDJoin *Join = (CQDJoin *)m_Designer->m_Tree.GetItemData(m_Node);
    // First table name
    if (col == 0)
    {
        wxTreeItemIdValue cookie;
        wxTreeItemId DstNode = m_Designer->m_Tree.GetFirstChild(m_Node, cookie);
        return !DstNode.IsOk();
    }
    // Join type
    if (col == 1)
        return (Join->m_JoinType & (JT_LEFT | JT_RIGHT | JT_INNER | JT_CROSS | JT_NATURAL | JT_FULL)) == 0;
    // Second table name
    if (col == 2)
    {
        wxTreeItemIdValue cookie;
        wxTreeItemId DstNode = m_Designer->m_Tree.GetFirstChild(m_Node, cookie);
        if (!DstNode.IsOk())
            return true;
        wxTreeItemId SrcNode = m_Designer->m_Tree.GetNextChild(m_Node, cookie);
        return !SrcNode.IsOk();
    }
    // ON/USING
    if (col == 3)
        return (Join->m_JoinType & (JT_ON | JT_USING)) == 0;
    // Join condition
    if (col == 4)
        return Join->m_Cond.IsEmpty();
    // Alias
    else
        return Join->m_Alias.IsEmpty();
}
//
// Returns cell value
//
wxString CQDJoinGrid::GetValue(int row, int col)
{
    CQDJoin *Join = (CQDJoin *)m_Designer->m_Tree.GetItemData(m_Node);
    // First table name
    if (col == 0)
    {
        wxTreeItemIdValue cookie;
        wxTreeItemId DstNode = m_Designer->m_Tree.GetFirstChild(m_Node, cookie);
        if (!DstNode.IsOk())
            return wxString();
        return m_Designer->GetTableName((CQDQTable *)m_Designer->m_Tree.GetItemData(DstNode), GTN_ALIAS);
    }
    // Join type
    if (col == 1)
    {
        if (Join->m_JoinType & JT_LEFT)
            return wxT("LEFT");
        if (Join->m_JoinType & JT_RIGHT)
            return wxT("RIGHT");
        if (Join->m_JoinType & JT_INNER)
            return wxT("INNER");
        if (Join->m_JoinType & JT_CROSS)
            return wxT("CROSS");
        if (Join->m_JoinType & JT_NATURAL)
            return wxT("NATURAL");
        if (Join->m_JoinType & JT_FULL)
            return wxT("FULL");
        return wxString();
    }
    // Second table name
    if (col == 2)
    {
        wxTreeItemIdValue cookie;
        wxTreeItemId DstNode = m_Designer->m_Tree.GetFirstChild(m_Node, cookie);
        if (!DstNode.IsOk())
            return wxString();
        wxTreeItemId SrcNode = m_Designer->m_Tree.GetNextChild(m_Node, cookie);
        return m_Designer->GetTableName((CQDQTable *)m_Designer->m_Tree.GetItemData(SrcNode), GTN_ALIAS);
    }
    // ON/USING
    if (col == 3)
    {
        if (Join->m_JoinType & JT_ON)
            return wxT("ON");
        if (Join->m_JoinType & JT_USING)
            return wxT("USING");
        return wxString();
    }
    // Join condition
    if (col == 4)
        return Join->m_Cond;
    // Alias
    else
        return Join->m_Alias;
}
//
// Stores cell value to record
//
void CQDJoinGrid::SetValue(int row, int col, const wxString& value)
{
    CQDJoin *Join = (CQDJoin *)m_Designer->m_Tree.GetItemData(m_Node);
    // First table name
    if (col == 0)
        return;
    // Join type
    else if (col == 1)
    {
        Join->m_JoinType &= ~(JT_LEFT | JT_RIGHT | JT_INNER | JT_CROSS | JT_NATURAL | JT_FULL);
        if (value[0] == wxT('L'))
            Join->m_JoinType |= JT_LEFT;
        else if (value[0] == wxT('R'))
            Join->m_JoinType |= JT_RIGHT;
        else if (value[0] == wxT('I'))
            Join->m_JoinType |= JT_INNER;
        else if (value[0] == wxT('C'))
            Join->m_JoinType |= JT_CROSS;
        else if (value[0] == wxT('N'))
            Join->m_JoinType |= JT_NATURAL;
        else if (value[0] == wxT('F'))
            Join->m_JoinType |= JT_FULL;
    }
    // Second table name
    else if (col == 2)
        return;
    // ON/USING
    else if (col == 3)
    {
        Join->m_JoinType &= ~(JT_ON | JT_USING);
        if (value[0] == wxT('O'))
            Join->m_JoinType |= JT_ON;
        else if (value[0] == wxT('U'))
            Join->m_JoinType |= JT_USING;
    }
    // Join condition
    else if (col == 4)
        Join->m_Cond = value;
    // Alias
    else
        Join->m_Alias = value;
    m_Designer->changed           = true;
    m_Designer->m_CurQueryChanged = true;
}
//
// Returns column heading
//
wxString CQDJoinGrid::GetColLabelValue(int col)
{
    // First table name
    if (col == 0)
        return wxString(_("Destination table"));
    // Join type
    if (col == 1)
        return wxString(_("Join type"));
    // Second table name
    if (col == 2)
        return wxString(_("Source table"));
    // ON/USING
    if (col == 3)
        return wxString();
    // Join condition
    if (col == 4)
        return wxString(_("Specification"));
    // Alias
    else
        return wxString(tnAlias);
}
//
// Returns true if cell can accept draged data - join condition only
//
bool CQDJoinGrid::OnDragOver(int Row, int Col)
{
    return Col == 4;
}

BEGIN_EVENT_TABLE(CQDJoinGrid, CQDGrid)
    EVT_GRID_SELECT_CELL(CQDJoinGrid::OnSelChange)
    EVT_GRID_LABEL_LEFT_CLICK(CQDJoinGrid::OnSelChange) 
END_EVENT_TABLE()
//
// Select cell, left click event handler, selects corresponding join link
//
void CQDJoinGrid::OnSelChange(wxGridEvent &event)
{
    event.Skip();
    m_Designer->m_Tables.SelectAttrLink(m_Node);
}

/////////////////////////////////////////// WHERE/HAVING grid /////////////////////////////////////////////////

int CQDCondGrid::ColWidth[3] = {200, 50, 0};
//
// OnOpen virtual function implementation - adds needed columns
//
void CQDCondGrid::OnOpen()
{
    AddTextCol(0,  ColWidth[0]);                // Expression            
    AddComboCol(1, ColWidth[1], wxT("AND,OR")); // Association
    AddFictCol(2);
}
//
// Returns true for editable column - expression only
//
bool CQDCondGrid::IsTextEditor(int Col)
{
    return Col == 0;
}
//
// Returns column count
//
int CQDCondGrid::GetNumberCols()
{
    return 3;
}
//
// Returns true if cell value is empty
//
bool CQDCondGrid::IsEmptyCell(int row, int col)
{
    if (row >= GetNumberRows() - 1 || col >= 2)
        return true;
    CQDCond *Cond = (CQDCond *)GetData(row);
    if (col)
        return Cond->m_Assoc == CA_NONE;
    else
        return Cond->m_Expr.IsEmpty();
}
//
// Returns cell value
//
wxString CQDCondGrid::GetValue(int row, int col)
{
    if (row >= GetNumberRows() - 1 || col >= 2)
        return wxString();
    CQDCond *Cond = (CQDCond *)GetData(row);
    // Association
    if (col)
        return wxString(AndOrTbl[Cond->m_Assoc]);
    // Expression
    else
    {
        SetReadOnly(row, col, Cond->m_SubConds);
        return Cond->m_Expr;
    }
}
//
// Stores cell value to record
//
void CQDCondGrid::SetValue(int row, int col, const wxString& value)
{
    wxTreeItemId Item;
    CQDCond *Cond;
    // IF old row, get condition record
    if (row < GetNumberRows() - 1)
    {
        Item = GetItem(row);
        Cond = (CQDCond *)m_Designer->m_Tree.GetItemData(Item);
    }
    // ELSE new row, create new record, append new tree item, set record as its ItemData
    else
    {
        Cond = new CQDCond(false);
        Item = m_Designer->m_Tree.AppendItem(m_Node, wxString(), qtiCond, qtiCond, Cond); 
        m_Designer->m_Tree.wxWindow::Refresh();
        Refresh();
    }
    // Association
    if (col)
    {
        if (value[0] == wxT('A'))
            Cond->m_Assoc = CA_AND;
        else if (value[0] == wxT('O'))
            Cond->m_Assoc = CA_OR;
        else
            Cond->m_Assoc = CA_NONE;
    }
    // Expression
    else
    {
        CQCond Expr;
        cdp_t cdp = m_Designer->cdp;
        // Compile expression because of subqueries
        int Err = compile_cond(cdp, value.mb_str(*cdp->sys_conv), &Expr);
        m_Designer->m_Tree.DeleteChildren(Item);
        // IF syntax error, show message
        if (Err)
        {
            client_error(cdp, Err);  
            cd_Signalize2(cdp, m_Designer->dialog_parent());
            Cond->m_Postfix.Clear(); 
        }
        // ELSE rebuild corresponding tree item
        else
        {
            Cond->m_Postfix = wxString(Expr.Postfix, *cdp->sys_conv);
            m_Designer->LoadColumnQueries(Item, &Expr.Queries);
        }
        // Store expression
        Cond->m_Expr = value;
    }
    wxString Caption = Cond->m_Expr;
    // IF not last item, append association to caption
    if (row < GetNumberRows() - 2)
    {
        Caption += wxT(" - ");
        Caption += AndOrTbl[Cond->m_Assoc];
    }
    // Reset tree item caption
    m_Designer->m_Tree.SetItemText(Item, Caption);
    // IF not first row AND previous association is empty, set it to AND
    if (row > 0)
    {
        Item = GetItem(row - 1);
        Cond = (CQDCond *)m_Designer->m_Tree.GetItemData(Item);
        if (Cond->m_Assoc == CA_NONE)
        {
            Cond->m_Assoc = CA_AND;
            m_Designer->m_Tree.SetItemText(Item, Cond->m_Expr + wxT(" - AND"));
        }
    }
    m_Designer->changed           = true;
    m_Designer->m_CurQueryChanged = true;
}
//
// Returns column heading
//
wxString CQDCondGrid::GetColLabelValue(int col)
{
    // Expression
    if (col == 0)
        return wxString(tnExpression);
    // Association
    if (col == 1)
        return wxString(_("Link"));
    else
        return wxString();
}
//
// Returns true if cell can accept draged data
//
bool CQDCondGrid::OnDragOver(int Row, int Col)
{
    // Only for expression AND condition dosn't have to have subconditions
    if (Col)
        return false;
    CQDCond *Cond = (CQDCond *)GetData(Row);
    return !Cond || !Cond->m_SubConds;
}

BEGIN_EVENT_TABLE(CQDCondGrid, CQDGrid)
    EVT_GRID_SELECT_CELL(CQDCondGrid::OnSelChange)
    EVT_GRID_LABEL_LEFT_CLICK(CQDCondGrid::OnSelChange) 
END_EVENT_TABLE()
//
// Select cell, left click event handler, selects corresponding join link
//
void CQDCondGrid::OnSelChange(wxGridEvent &event)
{
    event.Skip();
    wxTreeItemId Item = GetItem(event.GetRow());
    if (!Item.IsOk())
        return;
    m_Designer->m_Tables.SelectAttrLink(Item);
}

///////////////////////////////////////////// GROUP BY grid /////////////////////////////////////////////////

int CQDGroupGrid::ColWidth[2] = {200, 0};
//
// OnOpen virtual function implementation - adds needed columns
//
void CQDGroupGrid::OnOpen()
{
    AddTextCol(0,  ColWidth[0]);    // Experssion
    AddFictCol(1);
}
//
// Returns true for editable column - expression only
//
bool CQDGroupGrid::IsTextEditor(int Col)
{
    return Col == 0;
}
//
// Returns column count
//
int CQDGroupGrid::GetNumberCols()
{
    return 2;
}
//
// Returns true if cell value is empty
//
bool CQDGroupGrid::IsEmptyCell(int row, int col)
{
    if (row >= GetNumberRows() - 1 || col >= 1)
        return true;
    CQDGroup *Group = (CQDGroup *)GetData(row);
    return Group->m_Expr.IsEmpty();
}
//
// Returns cell value
//
wxString CQDGroupGrid::GetValue(int row, int col)
{
    if (row >= GetNumberRows() - 1 || col >= 1)
        return wxString();
    CQDGroup *Group = (CQDGroup *)GetData(row);
    return wxString(Group->m_Expr);
}
//
// Stores cell value to record
//
void CQDGroupGrid::SetValue(int row, int col, const wxString& value)
{
    wxTreeItemId Item;
    CQDGroup   *Group;
    // IF old row, get record
    if (row < GetNumberRows() - 1)
    {
        Item  = GetItem(row);
        Group = (CQDGroup *)m_Designer->m_Tree.GetItemData(Item);
    }
    // ELSE new row, create new record, append new tree item, set record as its ItemData
    else
    {
        Group = new CQDGroup();
        Item  = m_Designer->m_Tree.AppendItem(m_Node, GetNakedIdent(value), qtiGColumn, qtiGColumn, Group); 
        m_Designer->m_Tree.wxWindow::Refresh();
        Refresh();
    }
    // Store value
    Group->m_Expr                 = value;
    m_Designer->changed           = true;
    m_Designer->m_CurQueryChanged = true;
}
//
// Returns column heading
//
wxString CQDGroupGrid::GetColLabelValue(int col)
{
    if (col == 0)
        return wxString(tnExpression);
    else
        return wxString();
}
//
// Returns true if cell can accept draged data - expression only
//
bool CQDGroupGrid::OnDragOver(int Row, int Col)
{
    return Col == 0;
}

///////////////////////////////////////////// ORDER BY grid /////////////////////////////////////////////////

int CQDOrderGrid::ColWidth[3] = {150, 50};
//
// OnOpen virtual function implementation - adds needed columns
//
void CQDOrderGrid::OnOpen()
{
    AddTextCol(0, ColWidth[0]);     // Expression
    AddBoolCol(1, ColWidth[1]);     // DESC
    AddFictCol(2);
}
//
// Returns true for editable column - expression only
//
bool CQDOrderGrid::IsTextEditor(int Col)
{
    return Col == 0;
}
//
// Returns column count
//
int CQDOrderGrid::GetNumberCols()
{
    return 3;
}
//
// Returns true if cell value is empty
//
bool CQDOrderGrid::IsEmptyCell(int row, int col)
{
    if (row >= GetNumberRows() - 1 || col >= 2)
        return true;
    CQDOrder *Order = (CQDOrder *)GetData(row);
    if (col)
        return !Order->m_Desc;
    else
        return Order->m_Expr.IsEmpty();
}
//
// Returns cell value
//
wxString CQDOrderGrid::GetValue(int row, int col)
{
    if (row >= GetNumberRows() - 1 || col >= 2)
        return wxString();
    CQDOrder *Order = (CQDOrder *)GetData(row);
    if (col)
        return Order->m_Desc ? wxT("1") : wxString();
    else
        return wxString(Order->m_Expr);
}
//
// Stores cell value to record
//
void CQDOrderGrid::SetValue(int row, int col, const wxString& value)
{
    wxTreeItemId Item;
    CQDOrder *Order;
    // IF old row, get record
    if (row < GetNumberRows() - 1)
    {
        Item  = GetItem(row);
        Order = (CQDOrder *)m_Designer->m_Tree.GetItemData(Item);
    }
    // ELSE new row, create new record, append new tree item, set record as its ItemData
    else
    {
        Order = new CQDOrder();
        Item = m_Designer->m_Tree.AppendItem(m_Node, wxString(), qtiOColumn, qtiOColumn, Order); 
        m_Designer->m_Tree.wxWindow::Refresh();
        Refresh();
    }
    // DESC
    if (col)
    {
        Order->m_Desc = !value.IsEmpty();
    }
    // Expression
    else
    {
        Order->m_Expr = value;
    }
    // Reset tree item caption
    wxString Caption = GetNakedIdent(Order->m_Expr);
    if (Order->m_Desc)
        Caption += wxT(" - DESC");
    m_Designer->m_Tree.SetItemText(Item, Caption);
    m_Designer->changed           = true;
    m_Designer->m_CurQueryChanged = true;
}
//
// Returns column heading
//
wxString CQDOrderGrid::GetColLabelValue(int col)
{
    // Expression
    if (col == 0)
        return wxString(tnExpression);
    // DESC
    if (col == 1)
        return wxT("DESC");
    else
        return wxString();
}
//
// Returns true if cell can accept draged data - expression only
//
bool CQDOrderGrid::OnDragOver(int Row, int Col)
{
    return Col == 0;
}

///////////////////////////////////////////// GROUP BY grid /////////////////////////////////////////////////

int CQDLimitGrid::ColWidth[3] = {100, 100, 0};
//
// OnOpen virtual function implementation - adds needed columns
//
void CQDLimitGrid::OnOpen()
{
    AddTextCol(0, ColWidth[0]);     // Offset
    AddTextCol(1, ColWidth[1]);     // Count
    AddFictCol(2);
}
//
// Returns true for editable column - Offset or count
//
bool CQDLimitGrid::IsTextEditor(int Col)
{
    return Col <= 1;
}
//
// Returns row count - allways 1
//
int CQDLimitGrid::GetNumberRows()
{
    return 1;
}
//
// Returns column count
//
int CQDLimitGrid::GetNumberCols()
{
    return 3;
}
//
// Returns true if cell value is empty
//
bool CQDLimitGrid::IsEmptyCell(int row, int col)
{
    CQDLimit *Limit = (CQDLimit *)m_Designer->m_Tree.GetItemData(m_Node);
    if (col == 0)
        return Limit->m_Offset.IsEmpty();
    if (col == 1)
        return Limit->m_Count.IsEmpty();
    else
        return true;
}
//
// Returns cell value
//
wxString CQDLimitGrid::GetValue(int row, int col)
{
    CQDLimit *Limit = (CQDLimit *)m_Designer->m_Tree.GetItemData(m_Node);
    if (col == 0)
        return Limit->m_Offset;
    if (col == 1)
        return Limit->m_Count;
    else
        return wxString();
}
//
// Stores cell value to record
//
void CQDLimitGrid::SetValue(int row, int col, const wxString& value)
{
    CQDLimit *Limit = (CQDLimit *)m_Designer->m_Tree.GetItemData(m_Node);
    if (col == 0)
        Limit->m_Offset = value;
    if (col == 1)
        Limit->m_Count = value;
    m_Designer->changed           = true;
    m_Designer->m_CurQueryChanged = true;
}
//
// Returns column heading
//
wxString CQDLimitGrid::GetColLabelValue(int col)
{
    if (col == 0)
        return wxString(_("Offset"));
    if (col == 1)
        return wxString(_("Count"));
    else
        return wxString();
}
//
// Returns true if cell can accept draged data
//
bool CQDLimitGrid::OnDragOver(int Row, int Col)
{
    return true;
}

/////////////////////////////////// Property panel - parent of grids ////////////////////////////////////////

BEGIN_EVENT_TABLE(CPropsPanel, wxPanel)
    EVT_SIZE(CPropsPanel::OnSize)
END_EVENT_TABLE()
//
// Size event handler
//
void CPropsPanel::OnSize(wxSizeEvent & event)
{
    Resize();
    event.Skip();
}
//
// Resizes child grid
//
void CPropsPanel::Resize()
{
    // Get first child
    wxWindowList     &List = GetChildren();
    wxWindowListNode *Node = List.GetFirst();
    if (Node)
    {
        int Width, Height;
        GetClientSize(&Width, &Height);
        CQDGrid *Grid = (CQDGrid *)(wxWindow *)Node->GetData();
        // IF child is SELECT grid, set space for DISTINCT checkbox
        if (Grid->GetId() == qtiSelect)
        {
            wxSize Size = ((CQDSelectGrid *)Grid)->m_DistinctCB.GetSize();
#ifdef WINS            
            Size.y     += 11;
#else             
            Size.y     -= 2; 
#endif            
            Grid->SetSize(0, Size.y, Width, Height - Size.y);
        }
        // ELSE resize grid to whole client space
        else
            Grid->SetSize(Width, Height);
        Grid->Resize();
        Grid->ForceRefresh();
    }
}

///////////////////////////////////////////// Tables panel //////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CTablesPanel, wxScrolledWindow)
    EVT_PAINT(CTablesPanel::OnPaint)
    EVT_LEFT_DOWN(CTablesPanel::OnLeftMouseDown) 
    EVT_RIGHT_DOWN(CTablesPanel::OnRightMouseDown) 
#ifdef LINUX
    EVT_KEY_DOWN(CTablesPanel::OnKeyDown)
#endif    
END_EVENT_TABLE()

#ifdef LINUX
//
// Key down event handler
//
void CTablesPanel::OnKeyDown(wxKeyEvent &event)
{
    // IF Delete AND join link is selected, delete it
    if (event.GetKeyCode() == WXK_DELETE)
    {
        if (m_SelAttrLink)
        {
            wxTreeItemId Node = m_SelAttrLink->m_Node;
            int Image = m_Designer->m_Tree.GetItemImage(Node);
            if (Image == qtiCond)
                m_Designer->DeleteCond(Node, true);
            else
                m_Designer->DeleteJoin(Node, true);
        }
        return;
    }
    event.Skip();
}
//
// Idle event handler - checks if some TableBox list scrolled, if true, redraws links
//
void CTablesPanel::OnInternalIdle()
{
    wxScrolledWindow::OnInternalIdle();
    bool Moved = false;
    // FOR each join link
    for (int i = 0; i < m_AttrLinks.count(); i++)
    {
        CAttrLink *AttrLink = m_AttrLinks.acc0(i);
        // Get source TableBox
        CQDTableBox *Box    = AttrLink->m_DstAttr.m_TableBox;
        long Top            = Box->m_List.GetTopItem();
        // IF TableBox list scrolled, set Moved
        if (Box->m_LastTop != Top)    
        {
            Box->m_LastTop  = Top;
            Moved           = true;
        }
        // Get destination TableBox
        Box                 = AttrLink->m_SrcAttr.m_TableBox;
        Top                 = Box->m_List.GetTopItem();
        // IF TableBox list scrolled, set Moved
        if (Box->m_LastTop != Top)    
        {
            Box->m_LastTop  = Top;
            Moved           = true;
        }
    }
    // IF Moved, redraw panel
    if (Moved)
        Refresh();
}

#endif    
//
// Paint event handler, draws join links and selected table frame
//
void CTablesPanel::OnPaint(wxPaintEvent &event)
{
    wxPaintDC DC(this);
    wxPoint   Line[4];

    // FOR each link
    for (int i = 0; i < m_AttrLinks.count(); i++)
    {
        CAttrLink *AttrLink = m_AttrLinks.acc0(i);
        // Set link line points
        Line[0] = GetPoint(AttrLink, true);
        Line[3] = GetPoint(AttrLink, false);
        // Set link line pen
        if (AttrLink == m_SelAttrLink)
            DC.SetPen(m_SelPen);
        else
            DC.SetPen(*wxBLACK_PEN);
        Line[1].x = Line[0].x + 5;
        Line[1].y = Line[0].y;
        Line[2].x = Line[3].x - 5;
        Line[2].y = Line[3].y;
        // Draw line
        DC.DrawLines(4, Line);
    }
    // IF selected table, draw frame around TableBox
    if (m_SelTable)
    {
        wxRect Rect = m_SelTable->GetRect();
        DC.SetPen(m_SelPen);
#ifdef WINS		
        DC.DrawRectangle(Rect.x, Rect.y, Rect.width, Rect.height);
        m_SelTable->Refresh();
#else		
        DC.DrawRectangle(Rect.x - 1, Rect.y - 1, Rect.width + 1, Rect.height + 1);
#endif		
    }
}
//
// Returns point on TableBox border for join link
// Src - true  for point on right border of table that is more left
//       false for point on left border of table that is more right
//
wxPoint CTablesPanel::GetPoint(CAttrLink *AttrLink, bool Src)
{
    wxPoint      Result;
    CLinkedAttr *Table;

    int sLeft, sTop, dLeft, dTop;
    // Get source and destination table position
    AttrLink->m_SrcAttr.m_TableBox->GetPosition(&sLeft, &sTop);
    AttrLink->m_DstAttr.m_TableBox->GetPosition(&dLeft, &dTop);
    // IF source table is more left AND Src or viceversa, scaned box is source box
    if ((sLeft < dLeft) == Src)
    {
        Table = &AttrLink->m_SrcAttr;
    }
    // ELSE scaned box is destination box
    else
    {
        Table = &AttrLink->m_DstAttr;
        sTop  = dTop;
    }
    // Get scaned box position
    Result = Table->m_TableBox->GetPosition();
    // IF Src, add box witdh to x, line will start on right border (else line will end on left border)
    if (Src)
    {
        int Width, Height;
        Table->m_TableBox->GetSize(&Width, &Height);
        Result.x += Width;
    }
    // IF link column not specified, line will start/end on box title
    if (Table->m_AttrNO < 0)
        Result.y -= 8;
    else 
    {
        // y = 0 for columns before top item
        Result.y = 0;
        int ClIndex = Table->m_AttrNO - Table->m_TableBox->TopIndex();
        if (ClIndex >= 0) 
        {
            int Width;
            // Calculate y for column
            int Height = Table->m_TableBox->ItemHeight();
            Result.y   = (ClIndex * Height) + Height / 2;
            Table->m_TableBox->GetClientSize(&Width, &Height);
            // IF column is behind the last visible item, y = Height
            if (Result.y > Height)
                Result.y = Height;
        }
    }
    // Recalculate y to tables panel coordinates
    Result.y += sTop + GetTBClOffset();
    return Result;
}
//
// Returns y offset from top of TableBox to top of listbox = TableBox title height
//
int CTablesPanel::GetTBClOffset()
{ 
    if (!m_TBClOffset)
    {
        wxWindowList     &List = GetChildren();
        wxWindowListNode *Node = List.GetFirst();
        if (Node)
        {
            CQDTableBox *Box = (CQDTableBox *)Node->GetData();
            wxPoint P        = ClientToScreen(Box->GetPosition());
            P                = Box->m_List.ScreenToClient(P);
            m_TBClOffset     = -P.y;
        }
    }
    return m_TBClOffset;
}
//
// Left mouse down event handler - selects join link
// 
void CTablesPanel::OnLeftMouseDown(wxMouseEvent &event)
{
    m_HasFocus = true;
    SelectAttrLink(event.GetX(), event.GetY());
    //SetFocus();
}
//
// Selects join link near point x,y
//
CAttrLink *CTablesPanel::SelectAttrLink(int x, int y)
{
    // Find attr link near x,y
    CAttrLink *AttrLink = AttrLinkFromPoint(x, y);
    // IF found and corresponding tree item is valid
    if (AttrLink && AttrLink->m_Node.IsOk())
    {
        // IF link is join
        if (m_Designer->m_Tree.GetItemImage(AttrLink->m_Node) == qtiJoin)
        {
            // IF link is not current link, show join grid
            if (AttrLink != m_SelAttrLink) 
            {
                m_Designer->GetGrid(qtiJoin, AttrLink->m_Node);
            }
        }
        // ELSE WHERE condition link
        else 
        {
            // IF link is not current link, show WHERE grid, select corresponding condition
            if (AttrLink != m_SelAttrLink)
            {
                m_Designer->GetGrid(qtiWhere, m_Designer->m_Tree.GetItemParent(AttrLink->m_Node));
                m_Designer->m_Grid->Select(m_Designer->IndexOf(AttrLink->m_Node), 0);
            }
        }
        //SetFocus();
    }
    // IF link is not current link, set current link, reset selected table, redraw panel
    if (AttrLink != m_SelAttrLink)
    {
        m_SelAttrLink = AttrLink;
        m_SelTable    = NULL;
        Refresh();
    }
    return AttrLink;
}
//
// Selects join link for Item
//
void CTablesPanel::SelectAttrLink(const wxTreeItemId &Item)
{
    // Find link for Item
    CAttrLink *AttrLink = FindAttrLink(Item);
    // IF link is not current link, set current link, reset selected table, redraw panel
    if (AttrLink != m_SelAttrLink)
    {
        m_SelAttrLink = AttrLink;
        m_SelTable    = NULL;
        Refresh();
    }
}
//
// Right mouse down event handler 
//
void CTablesPanel::OnRightMouseDown(wxMouseEvent &event)
{
    wxMenu Menu;
    m_HasFocus = true;
    // Find join link near mouse position
    CAttrLink *AttrLink = SelectAttrLink(event.GetX(), event.GetY());
    // IF not found, appnend "Add new table" menu item
    if (!AttrLink)
        AppendXpmItem(&Menu, TD_CPM_ADDTABLE,   _("Add new table"),             addtable_xpm);
    // IF found AND link is join, appnend "Delete selected join" menu item
    else if (m_Designer->m_Tree.GetItemImage(AttrLink->m_Node) == qtiJoin)
        AppendXpmItem(&Menu, TD_CPM_DELETELINK, _("Delete selected join"),      qdJoin_xpm);
    // ELSE append "Delete selected condition" menu item
    else
        AppendXpmItem(&Menu, TD_CPM_DELETELINK, _("Delete selected condition"), qdCond_xpm);
    // Show popup menu
    PopupMenu(&Menu, event.GetX(), event.GetY());
}
//
// Returns CAttrLink instance near point x,y
//
CAttrLink *CTablesPanel::AttrLinkFromPoint(int x, int y)
{
    for (int i = 0; i < m_AttrLinks.count(); i++)
    {
        CAttrLink *Result = m_AttrLinks.acc0(i);
        wxPoint    Src    = GetPoint(Result, true);
        if (x >= Src.x && x <= Src.x + 5 && y >= Src.y - 2 && y <= Src.y + 2)
            return Result;
        wxPoint Dst = GetPoint(Result, false);
        if (x >= Dst.x - 5 && x <= Dst.x && y >= Dst.y - 2 && y <= Dst.y + 2)
            return Result;
        if (x < Src.x || x > Dst.x)
            continue;
        Src.x += 5;
        Dst.x -= 5;
        if (Src.x != Dst.x && x >= Src.x && x <= Dst.x)
        {
            int yr = (Dst.y - Src.y) * (x - Src.x) / (Dst.x - Src.x) + Src.y;
            if (y >= yr - 2 && y <= yr + 2)
                return Result;
        }
        if (Src.y != Dst.y && ((y >= Src.y && y <= Dst.y) || (y <= Src.y && y >= Dst.y)))
        {
            int xr = (Dst.x - Src.x) * (y - Src.y) / (Dst.y - Src.y) + Src.x;
            if (x >= xr - 2 && x <= xr + 2)
                return Result;
        }
    }
    return NULL;
}
//
// Deletes TableBox
//
void CTablesPanel::DeleteTable(CQDTableBox *Table)
{
    // IF selected table, reset
    if (Table == m_SelTable)
        m_SelTable = NULL;
    // Delete all join links for this table
    for (int i = 0; i < m_AttrLinks.count();)
    {
        CAttrLink *AttrLink = m_AttrLinks.acc0(i);
        if (AttrLink->m_DstAttr.m_TableBox == Table || AttrLink->m_SrcAttr.m_TableBox == Table)
        {
            m_Designer->DeleteAttrLink(AttrLink->m_Node);
            m_AttrLinks.delet(i);
        }
        else
            i++;
    }
    // Destroy TableBox
    Table->Destroy();
    // Redraw panel and scrollbars
    Resize();
    Refresh();
}
//
// Finds TableBox for Node
//
CQDTableBox *CTablesPanel::FindTableBox(const wxTreeItemId &Node)
{
    wxWindowList &List = GetChildren();
    // FOR each child of tables panel
    for (wxWindowListNode *BoxNode = List.GetFirst(); BoxNode; BoxNode = BoxNode->GetNext())
    {
        // IF link to tree item matches, found
        CQDTableBox *Box = (CQDTableBox *)BoxNode->GetData();
        if (Box->m_Node == Node)
            return Box;
    }
    return NULL;
}
//
// Finds join link for Node
//
CAttrLink *CTablesPanel::FindAttrLink(const wxTreeItemId &Node)
{
    for (int i = 0; i < m_AttrLinks.count(); i++)
    {
        CAttrLink *AttrLink = m_AttrLinks.acc0(i);
        if (AttrLink->m_Node == Node)
            return AttrLink;
    }
    return NULL;
}
//
// Resets tables panel vitrtual size according TableBoxes positions
//
void CTablesPanel::Resize()
{
    if (m_InScroll)
        return;
    wxSize Size(0, 0);
    wxWindowList &List = GetChildren();
    // FOR each TableBox, get max x and y coordinate
    for (wxWindowListNode *BoxNode = List.GetFirst(); BoxNode; BoxNode = BoxNode->GetNext())
    {
        CQDTableBox *Box = (CQDTableBox *)BoxNode->GetData();
        wxRect Rect = Box->GetRect();
        if (Rect.GetRight() > Size.x)
            Size.x = Rect.GetRight();
        if (Rect.GetBottom() > Size.y)
            Size.y = Rect.GetBottom();
    }
    wxSize cSize = GetClientSize();
    // IF max x > client width, set virtual width to max x + 20
    if (Size.x > cSize.x)
        Size.x += 20;
    // IF max y > client client height, set virtual height to max y + 20
    if (Size.y > cSize.y)
        Size.y += 20;
    SetVirtualSize(Size);
}
//
// Blocks virtual resizing duering scroll
//
void CTablesPanel::ScrollWindow( int dx, int dy, const wxRect* rect)
{
    m_InScroll = true;
    wxScrolledWindow::ScrollWindow(dx, dy, rect);
    m_InScroll = false;
}

/////////////////////////////////////////////// TableBox ////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CQDTableBox, wbFramedChild)
    EVT_MOVE(CQDTableBox::OnMove)
    EVT_SET_FOCUS(CQDTableBox::OnSetFocus)
    EVT_MENU(TD_CPM_HIDDENATTRS, CQDTableBox::OnHiddenAttrs)
    EVT_MENU(TD_CPM_REFRESH, CQDTableBox::OnRefresh)
    EVT_RIGHT_UP(CQDTableBox::OnRightClick)
END_EVENT_TABLE()
//
// Destructor
//
CQDTableBox::~CQDTableBox()
{
    m_Deleting = true;
    // IF tree item link is valid, get table record and reset pointer to table box in record
    if (m_Node.IsOk())
    {
        CQueryDesigner *Designer = ((CTablesPanel *)GetParent())->m_Designer;
        CQDQTable *Table  = (CQDQTable *)Designer->m_Tree.GetItemData(m_Node);
        Table->m_TableBox = NULL;
    }
}
//
// Creates TableBox
//
bool CQDTableBox::Create(CTablesPanel *Parent)
{
    wxPoint Pos(20, 20);
    wxWindowList  &ls = Parent->GetChildren();
    // Find existing TableBox that lies most right
    for (wxWindowListNode *Node = ls.GetFirst(); Node; Node = Node->GetNext())
    {
        wxRect Rect = ((wxWindow *)Node->GetData())->GetRect();
        if (Rect.x >= Pos.x)
            Pos.x = Rect.x + Rect.width + 20;
    }
    // Create TableBox frame 20 pixels right from last box
    if (!wbFramedChild::Create(Parent, 0, Pos, wxSize(100, 150), wxCAPTION))
        return false;

    // Create TableBox list
#ifdef WINS    
    if (!m_List.Create(this, QDID_TABLELIST, wxDefaultPosition, GetClientSize(), wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxNO_BORDER))
#else
    if (!m_List.Create(this, QDID_TABLELIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxSUNKEN_BORDER))
#endif
        return false;
    Show();
    
    m_List.InsertColumn(0, wxString());
    CQDQTable *Table = (CQDQTable *)Parent->m_Designer->m_Tree.GetItemData(m_Node);
    m_List.SetDropTarget(new CQDDropTargetTB(&m_List, Table->m_TableID, Parent->m_Designer));
    Parent->Resize();
    return true;
}
//
// Sets list of table columns
//
void CQDTableBox::SetList(cdp_t cdp, struct t_clivar * hostvars, unsigned hostvars_count)
{
    // Get table definition
    if (!m_TableDef.Create(cdp, m_Source.c_str(), m_Categ, hostvars, hostvars_count))
        return;
    m_Conv = cdp->sys_conv;
    // Relist content
    Relist(cdp);
}
//
// Relists list of table columns
//
void CQDTableBox::Relist(cdp_t cdp)
{
    // EXIT IF table definition not available
    if (!m_TableDef.m_Def)
        return;
    const d_attr *Attr;
    m_List.Freeze();
    m_List.DeleteAllItems();
    int i;
    // FOR each table column
    for (i = 1; (Attr = m_TableDef.GetAttr(i)) != NULL;  i++)
    {
        // IF hidden column AND hide hidden columns, continue
        if ((*(DWORD *)Attr->name & 0xFFFFDFFF) == (DWORD)HIDDEN_ATTR && !m_HiddenAttrs)
            continue;
        // Add column name to listbox
        tobjname Name;
        strcpy(Name, Attr->name);
        wb_small_string(cdp, Name, true);
        Add(Name, Attr->type);
    }
    m_List.Thaw();
}
//
// Passes right click event to listbox
//
void CQDTableBox::OnRightClick(wxMouseEvent &event)
{
    CTablesPanel *Panel = (CTablesPanel *)GetParent();
    Panel->m_HasFocus   = false;
    event.m_y          -= GetTitleHeight();
    m_List.RightClick(event.m_x, event.m_y);
}
//
// Selects box and passes set focus event to listbox
//
void CQDTableBox::OnSetFocus(wxFocusEvent &event)
{
    if (!m_Deleting)
    {
        ((CTablesPanel *)GetParent())->Select(this);
        m_List.SetFocus();
        event.Skip();
    }
}
//
// Changes HiddenAttrs flag and relists listbox
//
void CQDTableBox::OnHiddenAttrs(wxCommandEvent &event)
{
    m_HiddenAttrs = !m_HiddenAttrs;
    CQueryDesigner *Designer = ((CTablesPanel *)GetParent())->m_Designer;
    Relist(Designer->cdp);
    Refresh();
    Designer->ResetAttrLinks();
}
//
// Refreshes content of listbox
//
void CQDTableBox::OnRefresh(wxCommandEvent &event)
{
    CQueryDesigner  *Designer = ((CTablesPanel *)GetParent())->m_Designer;
    struct t_clivar *hostvars; 
    unsigned hostvars_count = Designer->GetParams(&hostvars);
    SetList(Designer->cdp, hostvars, hostvars_count);
    event.Skip();
}
//
// Move event handler
//
void CQDTableBox::OnMove(wxMoveEvent &event)
{
    CTablesPanel *Parent = (CTablesPanel *)GetParent();
#ifdef WINS
    int dx, dy;
    Parent->GetViewStart(&dx, &dy);
    wxPoint Pos = GetPosition();
    if (Pos.x < -dx || Pos.y < -dy)
    {
        if (Pos.x < -dx)
            Pos.x = -dx;
        if (Pos.y < -dy)
            Pos.y = -dy;
        Move(Pos);
    }
#endif
    Parent->Resize();
    Parent->Refresh();
}
//
// Returns index of Attr in column list
//
int CQDTableBox::IndexOf(const wxChar *Attr)
{
    for (int i = 0; i < m_List.GetItemCount(); i++)
    {
        if (wxStricmp(m_List.GetItemText(i), Attr) == 0)
            return i;
    }
    return -1;
}

///////////////////////////////////////////// Listbox for TableBox //////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CQDListBox, wbListCtrl)
    EVT_SIZE(CQDListBox::OnSize) 
    EVT_SCROLLWIN(CQDListBox::OnScroll) 
    EVT_LEFT_DOWN(CQDListBox::OnLeftClick)
    EVT_LIST_ITEM_SELECTED(QDID_TABLELIST, CQDListBox::OnItemSelected)
    EVT_KILL_FOCUS(CQDListBox::OnKillFocus)
    EVT_KEY_DOWN(CQDListBox::OnKeyDown)
    EVT_MOTION(CQDListBox::OnMouseMove)
#ifdef WINS
    EVT_COMMAND_RIGHT_CLICK(QDID_TABLELIST, CQDListBox::OnRightClick)
#else
    EVT_RIGHT_UP(CQDListBox::OnRightClick)
#endif
END_EVENT_TABLE()
//
// If column exists, sets column width to full client width
//
void CQDListBox::OnSize(wxSizeEvent &event)
{
    if (GetColumnCount())
    {
        wxSize Size = GetClientSize();
        SetColumnWidth(0, Size.x - 20);
    }
    event.Skip();
}
//
// Refreshes table panel to redraw jon links
//
// If linux doesn't redraw join links, it is due to OnScroll event didn't pass here from
// wxListMainWindow::OnScroll. It is necessary in  wxListMainWindow::OnScroll method
// (generic/listctrl.cpp) call GetListCtrl()->ProcessEvent(event);
//
void CQDListBox::OnScroll(wxScrollWinEvent &event)
{
    GetParent()->GetParent()->Refresh();
    event.Skip();
}
//
// Selects current box
//
void CQDListBox::OnLeftClick(wxMouseEvent &event)
{
    SetFocus();
    CQDTableBox  *Box   = (CQDTableBox *)GetParent();
    CTablesPanel *Panel = (CTablesPanel *)Box->GetParent();
    Panel->m_HasFocus   = false;
    Panel->Select(Box);
    event.Skip();
}
//
// Passes event to TableBox
//
void CQDListBox::OnMouseMove(wxMouseEvent &event)
{
    event.Skip();
    event.ResumePropagation(1);
}

#ifdef LINUX

void CQDListBox::OnRightClick(wxMouseEvent &event)
{
    RightClick(event.GetX(), event.GetY());
}

#else

void CQDListBox::OnRightClick(wxCommandEvent &event)
{   
    wxPoint Pos = ScreenToClient(wxGetMousePosition());
    RightClick(Pos.x, Pos.y);
}

#endif
//
// Right click event handler
//
void CQDListBox::RightClick(int x, int y)
{
    wxMenu Menu;
    CQDTableBox *Box    = (CQDTableBox *)GetParent();
    CTablesPanel *Panel = (CTablesPanel *)Box->GetParent();
    Panel->m_HasFocus   = false;
    // Select current TableBox
    Panel->Select(Box);
    // Create popup menu
    AppendXpmItem(&Menu, TD_CPM_DELETETABLE, _("Delete selected table"), deletetable_xpm, true);
    AppendXpmItem(&Menu, TD_CPM_REFRESH,     _("Refresh"));
    CQDQTable *Table = (CQDQTable *)((CTablesPanel *)Box->GetParent())->m_Designer->m_Tree.GetItemData(Box->m_Node);
    // IF database table, append ("Show hidden columns" item
    if (Table->m_Type == TT_TABLE)
    {
        Menu.AppendSeparator();
        Menu.AppendCheckItem(TD_CPM_HIDDENATTRS, _("Show hidden columns"));
        Menu.Check(TD_CPM_HIDDENATTRS, Box->m_HiddenAttrs);
    }
    // Show popup menu
    PopupMenu(&Menu, x, y);
}
//
// Shows message on status bar
//
void ShowStatusText(const wxString &Val)
{
    wxGetApp().frame->SetStatusText(Val, 1);
}
//
// Item selected event handler
//
void CQDListBox::OnItemSelected(wxListEvent &event)
{
    long Item      = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    // Get column name
    wxString Val   = GetItemText(Item);
    tccharptr sVal = Val.mb_str(*((CQDTableBox *)GetParent())->m_Conv);

    const d_attr *Attr;
    CQDTableBox *Box = (CQDTableBox *)GetParent();
    cdp_t cdp = ((CTablesPanel *)Box->GetParent())->m_Designer->cdp;
    // Find column definition
    for (int i = 1; (Attr = Box->m_TableDef.GetAttr(i)) != NULL;  i++)
    {
        // Show column name and column type on statusbar
        if (wb_stricmp(cdp, sVal, Attr->name) == 0)
        {
            char TypeName[55 + OBJ_NAME_LEN];
            sql_type_name(Attr->type, Attr->specif, true, TypeName);
            if (*TypeName)
                ShowStatusText(Val + wxT("  (") + wxString(TypeName, *wxConvCurrent) + wxT(")"));
            return;
        }
    }
}
//
// Kill focus even handler - clears status bar
//
void CQDListBox::OnKillFocus(wxFocusEvent &event)
{
    if (wxGetApp().frame && wxGetApp().frame->GetStatusBar())   // unless closing the client
        ShowStatusText(wxString());
}
//
// Key down event handler
//
void CQDListBox::OnKeyDown(wxKeyEvent &event)
{
    // IF delete
    if (event.GetKeyCode() == WXK_DELETE)
    {
        CQDTableBox *Box    = (CQDTableBox *)GetParent();
        CTablesPanel *Panel = (CTablesPanel *)Box->GetParent();
        // IF tables panel has not focus, delete table
        if (!Panel->m_HasFocus)
            Panel->m_Designer->DeleteTable(Box->m_Node, true);
        // ELSE delete join link
        else if (Panel->m_SelAttrLink)
        {
            wxTreeItemId Node = Panel->m_SelAttrLink->m_Node;
            int Image = Panel->m_Designer->m_Tree.GetItemImage(Node);
            if (Image == qtiCond)
                Panel->m_Designer->DeleteCond(Node, true);
            else
                Panel->m_Designer->DeleteJoin(Node, true);
        }
        return;
    }
    event.ResumePropagation(4); // TableBox, TablesPanel, RightPanel, QueryDesigner
    event.Skip();
}

/////////////////////////////////////////////// CTableDef ///////////////////////////////////////////////////

//
// Loads table/cursor definition
// Input:   wxSource        - Source table name/source SQL statement
//          Categ           - Source category
//          hostvars        - List SQL statement parameters
//          hostvars_count  - SQL statement parameter count
// Returns: true on success
//
bool CTableDef::Create(cdp_t cdp, const wxChar *wxSource, tcateg Categ, struct t_clivar * hostvars, unsigned hostvars_count)
{
    tcurstab   CursTab = NOOBJECT; 
    uns32      Handle  = 0;
    // Convert source to ASCII
    tccharptr  bSource = ToChar(wxSource, cdp->sys_conv);
    if (!bSource)
    {
        error_box(_("Illegal character in the source text"));
        return false;
    }
    const char *Source = bSource;
    // IF query, prepare query
    if (Categ == CATEG_DIRCUR || (strnicmp(Source, "SELECT", 6) == 0 && (Source[6] == ' ' || Source[6] == '\t' || Source[6] == '\r' || Source[6] == '\n')))
    {
        if (!*Source)
            return true;
        if (cd_SQL_host_prepare(cdp, Source, &Handle, hostvars, hostvars_count))
        {
            cd_Signalize(cdp);
            return false;
        }
    }
    else
    {
        // Find object, if not found try stored query
        if (cd_Find_object(cdp, Source, Categ, &CursTab))
        {
            Categ   = CATEG_CURSOR;
            if (cd_Find_object(cdp, Source, CATEG_CURSOR, &CursTab))
            {
                cd_Signalize(cdp);
                return false;
            }
        }
        // IF stored query, create SELECT statement for source, prepare query
        if (Categ == CATEG_CURSOR && CursTab != NOOBJECT)
        {
            wxString stm = wxT("SELECT * FROM ");
            stm         += wxSource;
            if (cd_SQL_host_prepare(cdp, stm.mb_str(*cdp->sys_conv), &Handle, hostvars, hostvars_count))
            {
                cd_Signalize(cdp);
                return false;
            }
        }
    }
    // IF query, get resultset info
    if (Handle)
    {
        BOOL Err = cd_SQL_get_result_set_info(cdp, Handle, 0, (d_table **)&m_Def);
        cd_SQL_drop(cdp, Handle);
        if (Err)
        {
            cd_Signalize(cdp);
            return false;
        }
    }
    // ELSE get table definition
    else
    {
        m_Def  = cd_get_table_d(cdp, CursTab, Categ);
    }
    return true;
}

/////////////////////////////////////////////// ToolTips //////////////////////////////////////////////////// 

//
// Constructor
//
CQDGridToolTip::CQDGridToolTip(CQDGrid *Grid) : wxAdvToolTipText(Grid->GetGridWindow())
{
    m_Grid = Grid;
    m_Row  = -1;
    m_Col  = -1;
}
//
// Returns tooltip source text
//
wxString CQDGridToolTip::GetText(const wxPoint &pos, wxRect &boundary)
{
    int x, y;
    // Convert mouse position to cell coordinates
    m_Grid->CalcUnscrolledPosition(pos.x, pos.y, &x, &y);
    int Col = m_Grid->XToCol(x);
    int Row = m_Grid->YToRow(y);
    
    // IF other cell then previous time
    if (Col >= 0 && Row >= 0 && (Row != m_Row || Col != m_Col))
    {
        m_Row = Row;
        m_Col = Col;
        wxString Text = m_Grid->GetCellValue(Row, Col);
        if (!Text.IsEmpty())
        {
            // Calculate cell text width in pixels
            wxClientDC dc(m_Grid);
            dc.SetFont(m_Grid->GetCellFont(Row, Col));
            wxCoord w, h;
            dc.GetTextExtent(Text, &w, &h);
            // IF text width > column width, set tooltip bounderies, return text
            if (w > m_Grid->GetColSize(Col))
            {
                boundary.x      = pos.x - 2;
                boundary.y      = pos.y - 2;
                boundary.width  = 4;
                boundary.height = 4;
                return Text;
            }
        }
    }
    // Return empty string otherwise
    return wxString();
}

///////////////////////////////// SQL statement elements popup window /////////////////////////////

static const wxChar *lePredicates[] = {wxT("UNIQUE"), wxT("LIKE"), wxT("IS UNKNOWN"), wxT("IS TRUE"), wxT("IS NULL"), wxT("IS FALSE"), wxT("EXISTS"), wxT("IN"), wxT("BETWEEN"), wxT("")};
static const wxChar *leFunctions[]  = {wxT("UPPER"), wxT("SUM"), wxT("SUBSTRING"), wxT("POSITION"), wxT("OCTET_LENGTH"), wxT("NULLIF"), wxT("MIN"), wxT("MAX"), wxT("LOWER"), wxT("CHAR_LENGTH"), wxT("EXTRACT"), wxT("CURRENT_TIMESTAMP"), wxT("CURRENT_TIME"), wxT("CURRENT_DATE"), wxT("COUNT"), wxT("COALESCE"), wxT("CAST"), wxT("BIT_LENGTH"), wxT("AVG"), wxT("")};
#ifdef WINS
static const wxChar *leOperators[]  = {wxT("OR"), wxT("AND"), wxT("NOT"), wxT(":<>"), wxT(":>"), wxT(":<"), wxT("||"), wxT("<="), wxT("<"), wxT(">="), wxT(">"), wxT("<>"), wxT("="), wxT("MOD"), wxT("DIV"), wxT("/"), wxT("*"), wxT("-"), wxT("+"), wxT("")};
static const wxChar *leSeparators[] = {wxT("&"), wxT("^"), wxT("$"), wxT("#"), wxT("@"), wxT("~"), wxT("`"), wxT("\\"), wxT("\"\""), wxT("\""), wxT("''"), wxT("'"), wxT("()"), wxT(")"), wxT("("), wxT(","), wxT("."), wxT(" "), wxT("")};
static const wxChar *leConstants[]  = {wxT("TRUE"), wxT("FALSE"), wxT("9"), wxT("8"), wxT("7"), wxT("6"), wxT("5"), wxT("4"), wxT("3"), wxT("2"), wxT("1"), wxT("0"), wxT("")};
#else
static const wxChar *leOperators[]  = {wxT("OR        "), wxT("AND       "), wxT("NOT       "), wxT(":<>       "), wxT(":>        "), wxT(":<        "), wxT("||        "), wxT("~         "), wxT(".=.      "), wxT(".=        "), wxT("<=        "), wxT("<         "), wxT(">=        "), wxT(">         "), wxT("<>        "), wxT("=         "), wxT("MOD       "), wxT("DIV       "), wxT("/         "), wxT("*        "), wxT("-         "), wxT("+         "), wxT("")};
static const wxChar *leSeparators[] = {wxT("&         "), wxT("^         "), wxT("$         "), wxT("#         "), wxT("@         "), wxT("~         "), wxT("`         "), wxT("\\         "), wxT("\"\"        "), wxT("\"         "), wxT("''        "), wxT("'         "), wxT("()        "), wxT(")        "), wxT("(         "), wxT(",         "), wxT(".         "), wxT("          "), wxT("")};
static const wxChar *leConstants[]  = {wxT("UNKNOWN   "), wxT("TRUE      "), wxT("FALSE     "), wxT("9         "), wxT("8         "), wxT("7         "), wxT("6         "), wxT("5         "), wxT("4         "), wxT("3         "), wxT("2         "), wxT("1         "), wxT("0         "), wxT("")};
#endif
//
// Creates SQL statement elements popup window
//
bool CElementPopup::Create()
{
    wxPoint Pos;
    wxSize  Size;
    char    Buf[50];
    // Read stored popup position and size
    if (!read_profile_string("QueryDesigner", "ElementPopup", Buf, sizeof(Buf)) || sscanf(Buf, " %d %d %d %d", &Pos.x, &Pos.y, &Size.x, &Size.y) < 4)
    {
        Pos    = wxDefaultPosition;
        Size.x = 312;
        Size.y = 214;
    }
    // Create frame
    if (!wxFrame::Create(m_Designer->GetParent(), 0, _("Lexical Elements"), Pos, Size, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER | wxSYSTEM_MENU | wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxCLIP_CHILDREN))
        return false;
    // Crate tabbed notebook
    if (!m_Notebook.Create(this,   0, wxDefaultPosition, wxDefaultSize,  wxCLIP_CHILDREN))
        return false;
    UpdateSize(&m_Notebook);
    // Create lists
    if (!CreateList(&m_Operators,  0, _("Operators"),  leOperators))
        return false;
    if (!CreateList(&m_Predicates, 0, _("Predicates"), lePredicates))
        return false;
    if (!CreateList(&m_Functions,  0, _("Functions"),  leFunctions))
        return false;
    if (!CreateList(&m_Separators, 0, _("Separators"), leSeparators))
        return false;
    if (!CreateList(&m_Constants,  0, _("Constants"),  leConstants))
        return false;
    Show();
    return true;
}
//
// Creates listbox of SQL statement elements
// Input:   List    - Listbox instance
//          id      - Listox ID (not used)
//          Caption - Listbox tab caption
//          Items   - Item list
//
bool CElementPopup::CreateList(wxListCtrl *List, wxWindowID id, const wxChar *Caption, const wxChar **Items)
{
    // Create lisbox
    if (!List->Create(&m_Notebook, id, wxDefaultPosition, wxDefaultSize, wxLC_LIST | wxSUNKEN_BORDER))
        return false;
    // Load item list
    for (const wxChar **Item = Items; **Item; Item++)
        List->InsertItem(0, *Item);
    // Add new notebook page
    if (!m_Notebook.AddPage(List, Caption))
        return false;
    // Assign dummy DropTarget, it is necessary in order to CQDDummyDropTarget::OnDragOver does return answers
    // and CQDDragSource can accept Feedback and move dragged image
    List->SetDropTarget(new CQDDummyDropTarget());
    return true;
}

BEGIN_EVENT_TABLE(CElementPopup, wxFrame)
    EVT_LIST_BEGIN_DRAG(-1, CElementPopup::OnBeginDrag)
    EVT_CLOSE(CElementPopup::OnClose) 
END_EVENT_TABLE()
//
// Begin drag event handler
//
void CElementPopup::OnBeginDrag(wxListEvent &event)
{
    wxListCtrl *List = (wxListCtrl *)event.GetEventObject();
    // Get dragged item text
#ifdef WINS    
    long        Item = event.GetIndex();
#else
    long        Item = List->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
#endif
    
    wxString    Text = List->GetItemText(Item);
#ifdef LINUX    
    Text = Text.Trim();
    if (Text.IsEmpty())
        Text = wxT(" ");
#endif

    // Create DataObject
    CQDDataObjectE *Data = new CQDDataObjectE(Text);
    if (!Data)
    {
        no_memory();
        return;
    }
    // wx does not enable to DropTarget discover what is in the DataObject and corectly answer if draged 
    // data are acceptable or not, so I store DataObject to static variable, where DropTarget could it find
    CQDDataObjectE::CurData = Data;
    wxRect Rect;
    // Calculate dragged image poseition
    List->GetItemRect(Item, Rect, wxLIST_RECT_LABEL);
    wxPoint Pos     = event.GetPoint() - Rect.GetPosition();

#ifdef WINS  
    // Create dragged image, start image dragging
    CWBDragImage di(*List, Item);
    if (di.BeginDrag(Pos, m_Designer, true))
    {
        di.Show();
        di.Move(event.GetPoint());
    }
#endif    
    // Start Drag&Drop operation
    CQDDragSource ds(m_Designer, Data);
    wxDragResult Res = ds.DoDragDrop(wxDrag_AllowMove);
#ifdef WINS  
    // Stop image dragging
    di.EndDrag();
#endif    
}
//
// Close window event handler - stores current popup window position and size
//
void CElementPopup::OnClose(wxCloseEvent &event)
{ 
    wxPoint Pos  = GetPosition();
    wxSize  Size = GetSize();
    char Buf[50];
    sprintf(Buf, "%d %d %d %d", Pos.x, Pos.y, Size.x, Size.y);
    write_profile_string("QueryDesigner", "ElementPopup", Buf);
    event.Skip();
}

///////////////////////////////////////// Add new table dialog //////////////////////////////////////////////

//
// Creates dialog
//
bool CAddTableDlg::Create(CQueryDesigner *Parent)
{
    static wxString atTypes[] = {_("Tables"), _("Queries"), _("System tables"), _("System queries")};

    m_Designer = Parent;
    // Create dialog frame
    if (!wxDialog::Create(Parent->dialog_parent(), 0, _("Add Table"), wxDefaultPosition, wxSize(400, 300), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLIP_CHILDREN))
        return false;
    // Cretae sizers
    wxFlexGridSizer *MainSizer = new wxFlexGridSizer(3, 1, 0, 0);
    if (!MainSizer)
        return false;
    wxBoxSizer *TopSizer = new wxBoxSizer(wxHORIZONTAL);
    if (!TopSizer)
        return false;
    wxBoxSizer *BottomSizer = new wxBoxSizer(wxHORIZONTAL);
    if (!BottomSizer)
        return false;
    MainSizer->AddGrowableRow(1);
    MainSizer->AddGrowableCol(0);
    SetSizer(MainSizer);
    SetAutoLayout(true);

    // Source table type combo
    MainSizer->Add(TopSizer, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE, 4);
    if (!m_TypeCB.Create(this, ID_TYPECOMBO, atTypes[0], wxDefaultPosition, wxDefaultSize, sizeof(atTypes) / sizeof(wxString), atTypes, wxCB_READONLY))
        return false;
    UpdateSize(&m_TypeCB);
    TopSizer->Add(&m_TypeCB, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE, 4);
    // Source schema combo
    if (!m_SchemaCB.Create(this, ID_SCHEMACOMBO, wxString(), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY))
        return false;
    UpdateSize(&m_SchemaCB);
    TopSizer->Add(&m_SchemaCB, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE, 4);
    // Source tables listbox
    if (!m_List.Create(this, ID_LIST, wxDefaultPosition, wxDefaultSize, wxLC_LIST | wxSUNKEN_BORDER))
        return false;
    MainSizer->Add(&m_List, 0, wxGROW| wxLEFT | wxRIGHT, 8);
    // OK button
    MainSizer->Add(BottomSizer, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE, 4);
    if (!m_OkBut.Create(this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize))
        return false;
    BottomSizer->Add(&m_OkBut, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE, 4);
    // Cancel button
    if (!m_CancelBut.Create(this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize))
        return false;
    m_OkBut.Enable(false);
    BottomSizer->Add(&m_CancelBut, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL | wxADJUST_MINSIZE, 4);

    // Fill schema combo
    C602SQLQuery Curs(Parent->cdp);
    Curs.Open("SELECT Obj_name FROM ObjTab WHERE Category=Chr(7) ORDER BY Obj_name");
    trecnum Cnt = Curs.Rec_cnt();
    tobjname Name;
    for (trecnum pos = 0; pos < Cnt; pos++)
    {
        Curs.Read(pos, 1, Name);
        wb_small_string(Parent->cdp, Name, true);
        m_SchemaCB.Append(wxString(Name, *Parent->cdp->sys_conv));
    }
    // Select current schema in combo
    strcpy(Name, Parent->cdp->sel_appl_name);
    wb_small_string(Parent->cdp, Name, true);
	int i = m_SchemaCB.FindString(wxString(Name, *Parent->cdp->sys_conv));
    m_SchemaCB.SetSelection(i);
    // Load source tables list
    wxCommandEvent event;
    OnSchemaChange(event);
    return true;
}

BEGIN_EVENT_TABLE(CAddTableDlg, wxDialog)
    EVT_COMBOBOX(ID_TYPECOMBO,        CAddTableDlg::OnTypeChange)
    EVT_COMBOBOX(ID_SCHEMACOMBO,      CAddTableDlg::OnSchemaChange)
    EVT_LIST_ITEM_SELECTED(ID_LIST,   CAddTableDlg::OnSelChange) 
    EVT_LIST_ITEM_DESELECTED(ID_LIST, CAddTableDlg::OnSelChange) 
    EVT_LIST_ITEM_ACTIVATED(ID_LIST,  CAddTableDlg::OnItemActivated)
END_EVENT_TABLE()
//
// Source table type changed event handler
//
void CAddTableDlg::OnTypeChange(wxCommandEvent &event)
{
    int Sel = GetNewComboSelection(&m_TypeCB);
    switch (Sel)
    {
    // Tables, stored queries
    case 0:
    case 1:

        OnSchemaChange(event);
        break;

    // System tables
    case 2:

        m_List.Freeze();
        m_List.ClearAll();
        m_List.InsertItem(0, wxT("__Reltab"));
        m_List.InsertItem(0, wxT("__Proptab"));
        //m_List.InsertItem(0, wxT("Keytab"));
        //m_List.InsertItem(0, wxT("Repltab"));
        //m_List.InsertItem(0, wxT("Srvtab"));
        m_List.InsertItem(0, wxT("Usertab"));
        m_List.InsertItem(0, wxT("Objtab"));
        m_List.InsertItem(0, wxT("Tabtab"));
        m_List.Thaw();
        break;

    // System queries
    case 3:

        m_List.Freeze();
        m_List.ClearAll();
        m_List.InsertItem(0, wxT("_IV_VIEWED_TABLE_COLUMNS"));
        m_List.InsertItem(0, wxT("_IV_TABLE_COLUMNS"));
        m_List.InsertItem(0, wxT("_IV_SUBJECT_MEMBERSHIP"));
        m_List.InsertItem(0, wxT("_IV_RECENT_LOG"));
        m_List.InsertItem(0, wxT("_IV_PROCEDURE_PARAMETERS"));
        m_List.InsertItem(0, wxT("_IV_PENDING_LOG_REGS"));
        m_List.InsertItem(0, wxT("_IV_MAIL_PROFS"));
        m_List.InsertItem(0, wxT("_IV_LOGGED_USERS"));
        m_List.InsertItem(0, wxT("_IV_LOCKS"));
        m_List.InsertItem(0, wxT("_IV_IP_ADDRESSES"));
        m_List.InsertItem(0, wxT("_IV_INDICIES"));
        m_List.InsertItem(0, wxT("_IV_CHECK_CONSTRAINS"));
        m_List.InsertItem(0, wxT("_IV_FOREIGN_KEYS"));
        m_List.Thaw();
        break;
    }
    // Enable schema combo for tables AND stored queries
    m_SchemaCB.Enable(Sel < 2);
}
//
// Schema changed event handler
//
void CAddTableDlg::OnSchemaChange(wxCommandEvent &event)
{
    // Get selected schema name
    wxString  Schema  = m_SchemaCB.GetValue();
    tccharptr sSchema = Schema.mb_str(*m_Designer->cdp->sys_conv);
    // Create object list
    CObjectList ol(m_Designer->cdp, sSchema);
    tcateg Categ = m_TypeCB.GetSelection() <= 0 ? CATEG_TABLE : CATEG_CURSOR;

    m_List.Freeze();
    m_List.ClearAll();

    int i = 0;
    // Fill listbox
    for (bool Found = ol.FindFirst(Categ); Found; Found = ol.FindNext())
        m_List.InsertItem(i++, GetSmallStringName(ol));

    m_List.Thaw();
}
//
// Selected source table chaged event handler
//
void CAddTableDlg::OnSelChange(wxListEvent &event)
{
    m_OkBut.Enable(m_List.GetSelectedItemCount() > 0);
}
//
// Source tables list item doubleclick event handler - closes dialog
//
void CAddTableDlg::OnItemActivated(wxListEvent &event)
{
    if (m_List.GetSelectedItemCount())
        EndModal(wxID_OK);
}

///////////////////////////////// Drag&Drop table from control panel ////////////////////////////////////////

//
// Drop target window enter event handler
//
wxDragResult CQDDropTargetTP::OnEnter(wxCoord x, wxCoord y, wxDragResult def)
{
    m_Result = wxDragNone;
#ifdef LINUX
    if (!GetMatchingPair())
        return wxDragNone;
#endif
    // Get dragged object list from DataObject stored in static variable
    C602SQLObjects *Objs = CCPDragSource::DraggedObjects();

    // IF from different server, cannot accept
    if (stricmp(Objs->m_Server, m_Server) != 0)
        return wxDragNone;
    // IF dragged object list contanins at last one table or stored query, return can be copied
    for (int i = 0; i < Objs->m_Count; i++)
    {
        if (Objs->m_Obj[i].m_Categ == CATEG_TABLE || Objs->m_Obj[i].m_Categ == CATEG_CURSOR)
        {
            m_Result = wxDragCopy;
            return wxDragCopy;
        }
    }
    // return cannot accept otherwise
    return wxDragNone;
}
//
// Drop target window drag over event handler
//
wxDragResult CQDDropTargetTP::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    // IF suggested value is wxDragNone, return wxDragNone, otherwise return value established in OnEnter
    return def == wxDragNone ? wxDragNone : m_Result;
}
//
// Draged data drop event handler
//
bool CQDDropTargetTP::OnDrop(wxCoord x, wxCoord y)
{
#ifdef WINS  
    // Hidde dragged image
    CWBDragImage::Image->Hide();
#endif    
    // Terminate Drag&Drop operation
    return wxIsDragResultOk(m_Result);
}
//
// Intrinsic draged data drop
//
wxDragResult CQDDropTargetTP::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
    // Read dragged object list from DataObject
    GetData();
    C602SQLObjects *objs = ((C602SQLDataObject *)GetDataObject())->GetObjects();
    wxString Schema      = wxString(objs->m_Schema, *m_Designer->cdp->sys_conv);
    wxString Name;
    // IF dragged objects schema == current schema, clear schema name
    if (wxStricmp(Schema, m_Designer->m_Schema) == 0)
        Schema.Clear();
    int Cnt = 0;
    // FOR each object in dragged list
    for (int i = 0; i < objs->m_Count; i++)
    {
        C602SQLObjName *Obj = &objs->m_Obj[i];
        // IF table OR strored query
        if (Obj->m_Categ == CATEG_TABLE || Obj->m_Categ == CATEG_CURSOR)
        {
            // Add new source table to design
            Name = GetProperIdent(wxString(Obj->m_ObjName, *m_Designer->cdp->sys_conv));
            m_Designer->AddNewTable(Schema, Name, Obj->m_Categ);
            Cnt++;
        }
    }
    // IF current grid is FROM grid, redraw it
    if (m_Designer->m_Grid->GetId() == qtiFrom)
        m_Designer->m_Grid->Refresh();
    m_Designer->changed = true;
    return def;
}

//////////////////////////////////////// Drag&Drop for join links ///////////////////////////////////////////

//
// Drop target window enter event handler
//
wxDragResult CQDDropTargetTB::OnEnter(wxCoord x, wxCoord y, wxDragResult def)
{
    m_Item    = -1;
    m_SelItem = -1;
    m_Invalid = false;
    m_Result  = wxDragNone;
    // Read DataObject from static variable
    // IF gragged object is not column OR current TableBox == Source TableBox OR link to targed table makes link cycle,
    // cannot accept
    if (!CQDDataObjectE::CurData || !CQDDataObjectE::CurData->IsAttr() || CQDDataObjectE::CurData->GetTable()->m_TableID == m_TableID || m_Designer->MakesLinkCycle(CQDDataObjectE::CurData->GetTable()->m_TableID, m_TableID))
    {
        m_Invalid = true;
        return wxDragNone;
    }

    // Check if link to current column is posible
    return OnDragOver(x, y, def);
}
//
// Kindred types table
//
static uns64 CompatArray[] =
{
    0,
    (1 << ATT_BOOLEAN) | (1 << ATT_INT16)  | (1 << ATT_INT32) | (1 << ATT_MONEY) | ((uns64)1 << ATT_INT8) | ((uns64)1 << ATT_INT64),                      // ATT_BOOLEAN
    (1 << ATT_CHAR)    | (1 << ATT_STRING),                                                                                                               // ATT_CHAR
    (1 << ATT_BOOLEAN) | (1 << ATT_INT16)  | (1 << ATT_INT32) | (1 << ATT_MONEY) | (1 << ATT_FLOAT) | ((uns64)1 << ATT_INT8) | ((uns64)1 << ATT_INT64),   // ATT_INT16
    (1 << ATT_BOOLEAN) | (1 << ATT_INT16)  | (1 << ATT_INT32) | (1 << ATT_MONEY) | (1 << ATT_FLOAT) | ((uns64)1 << ATT_INT8) | ((uns64)1 << ATT_INT64),   // ATT_INT32 
    (1 << ATT_BOOLEAN) | (1 << ATT_INT16)  | (1 << ATT_INT32) | (1 << ATT_MONEY) | (1 << ATT_FLOAT) | ((uns64)1 << ATT_INT8) | ((uns64)1 << ATT_INT64),   // ATT_MONEY
    (1 << ATT_BOOLEAN) | (1 << ATT_INT16)  | (1 << ATT_INT32) | (1 << ATT_MONEY) | (1 << ATT_FLOAT) | ((uns64)1 << ATT_INT8) | ((uns64)1 << ATT_INT64),   // ATT_FLOAT
    (1 << ATT_CHAR)    | (1 << ATT_STRING),                                                                                                               // ATT_STRING
    (1 << ATT_CHAR)    | (1 << ATT_STRING),                                                                                                               // ATT_CSSTRING
    (1 << ATT_CHAR)    | (1 << ATT_STRING),                                                                                                               // ATT_CSISTRING
    (1 << ATT_BINARY)  | (1 << ATT_AUTOR),                                                                                                                // ATT_BINARY
    (1 << ATT_DATE)    | (1 << ATT_TIMESTAMP),                                                                                                            // ATT_DATE
    (1 << ATT_TIME)    | (1 << ATT_TIMESTAMP),                                                                                                            // ATT_TIME
    (1 << ATT_DATE)    | (1 << ATT_TIME) | (1 << ATT_TIMESTAMP) | (1 << ATT_DATIM),                                                                       // ATT_TIMESTAMP
    0,                                                                                                                                                    // ATT_PTR
    0,                                                                                                                                                    // ATT_BIPTR
    (1 << ATT_BINARY)    | (1 << ATT_AUTOR),                                                                                                              // ATT_AUTOR
    (1 << ATT_TIMESTAMP) | (1 << ATT_DATIM),                                                                                                              // ATT_DATIM
    0,                                                                                                                                                    // ATT_HIST  
    0,                                                                                                                                                    // ATT_RASTER
    0,                                                                                                                                                    // ATT_TEXT  
    0,                                                                                                                                                    // ATT_NOSPEC
    0,                                                                                                                                                    // ATT_SIGNAT
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    0,                
    (1 << ATT_BOOLEAN) | (1 << ATT_INT16) | (1 << ATT_INT32) | (1 << ATT_MONEY) | (1 << ATT_FLOAT) | ((uns64)1 << ATT_INT8) | ((uns64)1 << ATT_INT64),    // ATT_INT8
    (1 << ATT_BOOLEAN) | (1 << ATT_INT16) | (1 << ATT_INT32) | (1 << ATT_MONEY) | (1 << ATT_FLOAT) | ((uns64)1 << ATT_INT8) | ((uns64)1 << ATT_INT64)     // ATT_INT64
};
//
// Drop target window drag over event handler
//
wxDragResult CQDDropTargetTB::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    wxDragResult Result = wxDragNone;
    // IF link to current table was discoverd as invalid in OnEnter, cannot accept
    if (m_Invalid)
        return wxDragNone;
    // IF dragged data is not column, cannot accept 
    if (!CQDDataObjectE::CurData)
        return wxDragNone;

    int  Flags;
    long Item    = m_List->HitTest(wxPoint(x, y), Flags);
    long SelItem = Item;
    // IF current list item is not last item
    if (Item != m_Item)
    {
        m_Result = wxDragNone;
        // IF above no item
        if (Item == -1)
        {
            // IF above list client area, new item = item preceeding last item
            if ((Flags & wxLIST_HITTEST_ABOVE) && m_Item > 0)
                Item = m_Item - 1;
            // IF below list client area AND last is not last item in the list, new item = item folowing last item
            else if ((Flags & (wxLIST_HITTEST_BELOW | wxLIST_HITTEST_NOWHERE)) && m_Item < m_List->GetItemCount() - 1)
                Item = m_Item + 1;
            // IF before first item OR after last item, cannot accept
            if (Item == -1)
                return wxDragNone;
#ifdef WINS  
            // Hide dragged image
            CWBDragImage::Image->Hide();
#endif            
            // Scroll listbox to show new item
            m_List->EnsureVisible(Item);
            m_List->wxWindow::Update();
#ifdef WINS  
            // Show dragged image
            CWBDragImage::Image->Show();
#endif            
        }
        m_Item = Item;
        // IF current item is not highlighted item
        if (SelItem != m_SelItem)
        {
#ifdef WINS  
            // Hide dragged image
            CWBDragImage::Image->Hide();
#endif            
            // Unhighlight old item, highlight new item
            m_List->HighlightItem(m_SelItem, false);
            m_List->HighlightItem(SelItem,   true);
            m_List->wxWindow::Update();
#ifdef WINS  
            // Show dragged image
            CWBDragImage::Image->Show();
#endif            
            m_SelItem = SelItem;
        }
        // IF source and targed column type corresponds
        uns64 Comp = (uns64)1 << CQDDataObjectE::CurData->GetType();
        if (CompatArray[m_List->GetItemData(Item)] & Comp)
        {
            // IF takovy link jeste neni
            CQDTableBox *Box1 = (CQDTableBox *)m_List->GetParent();
            CQDTableBox *Box2 = (CQDTableBox *)CQDDataObjectE::CurData->GetList()->GetParent();
            CTablesPanel *TablesPanel = (CTablesPanel *)Box1->GetParent();
            int i;
            // FOR each join link
            for (i = 0; i < TablesPanel->m_AttrLinks.count(); i++)
            {
                CAttrLink *AttrLink = TablesPanel->m_AttrLinks.acc0(i);
                // IF joins are to be converted to WHERE condition, check if join condition allready exists
                if (TablesPanel->m_Designer->m_JoinConv)
                {
                    if 
                    (
                        (AttrLink->m_DstAttr.m_TableBox == Box1 && AttrLink->m_DstAttr.m_AttrNO == Item && AttrLink->m_SrcAttr.m_TableBox == Box2 && AttrLink->m_SrcAttr.m_AttrNO == CQDDataObjectE::CurData->GetItem()) ||
                        (AttrLink->m_DstAttr.m_TableBox == Box2 && AttrLink->m_DstAttr.m_AttrNO == CQDDataObjectE::CurData->GetItem() && AttrLink->m_SrcAttr.m_TableBox == Box1 && AttrLink->m_SrcAttr.m_AttrNO == Item)
                    )
                        break;
                }
                // ELSE check if another join between these tables allready exist
                else
                {
                    if 
                    (
                        (AttrLink->m_DstAttr.m_TableBox == Box1 && AttrLink->m_SrcAttr.m_TableBox == Box2) ||
                        (AttrLink->m_DstAttr.m_TableBox == Box2 && AttrLink->m_SrcAttr.m_TableBox == Box1)
                    )
                        break;
                }
            }
            // IF join is posible, can accept
            if (i >= TablesPanel->m_AttrLinks.count())
                m_Result = wxDragCopy;
        }
    }

    return m_Result;
}
//
// Drop target window leave event handler
//
void CQDDropTargetTB::OnLeave()
{
#ifdef WINS  
    // Hide dragged image
    CWBDragImage::Image->Hide();
#endif    
    // Ungighlight last item
    m_List->HighlightItem(m_SelItem, false);
    m_List->wxWindow::Update();
#ifdef WINS  
    // Show dragged image
    CWBDragImage::Image->Show();
#endif    
}
//
// Draged data drop event handler
//
bool CQDDropTargetTB::OnDrop(wxCoord x, wxCoord y)
{
#ifdef WINS  
    // Hide dragged image
    CWBDragImage::Image->Hide();
#endif    
    // Ungighlight last item
    m_List->HighlightItem(m_SelItem, false);
    m_List->wxWindow::Update();
    return true;
}
//
// Intrinsic draged data drop
//
wxDragResult CQDDropTargetTB::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
    CQDTableBox    *SrcTable = (CQDTableBox *)m_List->GetParent();
    CQueryDesigner *Designer = ((CTablesPanel *)SrcTable->GetParent())->m_Designer;

    // Get DataObject
    GetData();
    CQDDataObjectE *Data = (CQDDataObjectE *)GetDataObject();
    // Create new join link by data from DataObject
    Designer->AddAttrLink((CQDTableBox *)Data->GetList()->GetParent(), Data->GetAttr(), SrcTable, m_List->GetItemText(m_Item));
    return def;
}

////////////////////////////////////// Drag&Drop to property grid ///////////////////////////////////////////

//
// Drop target window enter event handler
//
wxDragResult CQDDropTargetPG::OnEnter(wxCoord x, wxCoord y, wxDragResult def)
{
    return OnDragOver(x, y, def);
}
//
// Drop target window drag over event handler
//
wxDragResult CQDDropTargetPG::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    // IF dragged data is not column from TableBox or element form elemet popup, cannot accept
    if (!CQDDataObjectE::CurData)
        return wxDragNone;
    // Convert current mouse position to grid cell coordinates
    int xx, yy;
    m_Grid->CalcUnscrolledPosition(x, y, &xx, &yy);
    int Col = m_Grid->XToCol(xx - m_Grid->GetRowLabelSize());
    int Row = m_Grid->YToRow(yy - m_Grid->GetColLabelSize());
    // IF not above valid cell, cannot accept
    if (Col == wxNOT_FOUND || Row == wxNOT_FOUND || x < m_Grid->GetRowLabelSize() || y < m_Grid->GetColLabelSize())
        m_Result = wxDragNone;
    // Check if current grid can accept these data
    else if (!m_Grid->OnDragOver(Row, Col))
        m_Result = wxDragNone;
    // IF Ctrl key is down, cell content will be replaced
    else if (IsCtrlDown())
        m_Result = wxDragMove;
    // ELSE dragged data will be appended to cell content
    else
        m_Result = wxDragCopy;
    return m_Result;
}
//
// Draged data drop event handler
//
bool CQDDropTargetPG::OnDrop(wxCoord x, wxCoord y)
{
    return wxIsDragResultOk(m_Result);
}
//
// Intrinsic draged data drop
//
wxDragResult CQDDropTargetPG::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
    // Get dragged DataObject
    GetData();
    CQDDataObjectE *Data = (CQDDataObjectE *)GetDataObject();
    wxString Val;
    // IF TableBox column, get column name
    if (Data->IsAttr())
    {
        Val = m_Grid->m_Designer->GetAttrName(Data->GetTable(), Data->GetAttr());
    }
    else
    {
        // Get element text
        Val = Data->GetData();
        // IF element is function name, append (
        for (int i = 0; i < sizeof(leFunctions) / sizeof(const wxChar *); i++)
        {
            if (wxStrcmp(Data->GetData(), leFunctions[i]) == 0)
            {
                // CURRENT_DATE, CURRENT_TIME, CURRENT_TIMESTAMP
                if (i < 11 || i > 13)
                    Val += wxT('(');
                break;
            }
        }       
    }
    // Convert current mouse position to grid cell coordinates
    m_Grid->CalcUnscrolledPosition(x, y, &x, &y);
    int Col = m_Grid->XToCol(x - m_Grid->GetRowLabelSize());
    int Row = m_Grid->YToRow(y - m_Grid->GetColLabelSize());
    // IF replace OR cell is empty, set cell value
    if (m_Result == wxDragMove || m_Grid->IsEmptyCell(Row, Col))
        m_Grid->SetValue(Row, Col, Val);
    else 
    {
        // Get current cell content
        wxString Orig = m_Grid->GetValue(Row, Col);
        // IF last char is not ( OR separator, append space
        wxChar     c  = Orig[Orig.Len() - 1];
        if (c != wxT(' ') && c != wxT('(') && !IsSeparator(Data->GetData()))
            Orig += wxT(' ');
        // Append dragged value
        m_Grid->SetValue(Row, Col, Orig + Val);
    }
    m_Grid->ForceRefresh();
    m_Grid->m_Designer->changed = true;
    return def;
}
//
// Returns true if given char is separator
//
bool CQDDropTargetPG::IsSeparator(const wxChar *Data)
{
    for (int i = 0; i < sizeof(leSeparators) / sizeof(const wxChar *); i++)
    {
        if (wxStrcmp(Data, leSeparators[i]) == 0)
            return true;
    }
    return false;
}

////////////////////////////////////// Drag&Drop design tree items //////////////////////////////////////////

//
// Drop target window enter event handler
//
wxDragResult CQDDropTargetTI::OnEnter(wxCoord x, wxCoord y, wxDragResult def)
{
    // Reset last and last highlighted item
    m_Item.Unset();
    m_SelItem.Unset();
    m_Result  = wxDragNone;
    return OnDragOver(x, y, def);
}
//
// Drop target window drag over event handler
//
wxDragResult CQDDropTargetTI::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    // IF current dragged data is not design tree item, cannot accept
    if (!CQDDataObjectI::CurData)
        return wxDragNone;
    
    int Image = -1;
    wxTreeItemId SelItem;
    wxTreeItemId Item;
    // Get item below mouse
    CPHitResult Hit = m_Tree->DragTest(x, y, &Item);
    // IF any item found
    if (Hit == CPHR_ONITEM)
    {
        // IF other item than last time
        if (m_Item != Item)
        {
            Image = m_Tree->GetItemImage(Item);
            m_Result = wxDragMove;
            // Check if current item corresponds dragged item
            switch (CQDDataObjectI::CurData->m_Image)
            {
            case qtiQueryExpr:
            case qtiQuery:
            case qtiTable:
            case qtiJoin:
                // Query, table or join can be dropped on query, table, join or FROM
                if (Image != qtiQueryExpr && Image != qtiQuery && Image != qtiTable && Image != qtiJoin  && Image != qtiFrom)
                    m_Result = wxDragNone;
                break;
            case qtiColumn:
                // Column can be droppend on column or SELECT
                if (Image != qtiColumn && Image != qtiSelect)
                    m_Result = wxDragNone;
                break;
            case qtiCond:
            case qtiSubConds:
                // Condition can be dropped on condition, WHERE or HAVING
                if (Image != qtiCond && Image != qtiSubConds && Image != qtiWhere && Image != qtiHaving)
                    m_Result = wxDragNone;
                break;
            case qtiGColumn:
                // GROUP BY expression can be dropped on GROUP BY expression or GROUP BY
                if (Image != qtiGColumn && Image != qtiGroupBy)
                    m_Result = wxDragNone;
                break;
            case qtiOColumn:
                // ORDER BY expression can be dropped on ORDER BY expression or ORDER BY
                if (Image != qtiOColumn && Image != qtiOrderBy)
                    m_Result = wxDragNone;
                break;
            }
            // IF item can be dropped
            if (m_Result == wxDragMove)
            {
                // Check if item si not child of dragged item
                wxTreeItemId it;
                for (it = Item; it.IsOk(); it = m_Tree->GetItemParent(it))
                {
                    if (it == CQDDataObjectI::CurData->m_Item)
                        break;
                }
                if (it.IsOk())
                    m_Result = wxDragNone;
                else
                {
                    // Check if item is not preceeding sibling of dragged item
                    it = m_Tree->GetPrevSibling(CQDDataObjectI::CurData->m_Item);
                    if (!it.IsOk())
                        // IF gragged item is first child, check if item is not dragged item parent
                        it = m_Tree->GetItemParent(CQDDataObjectI::CurData->m_Item);
                    if (it.IsOk() && it == Item)
                        m_Result = wxDragNone;
                    // Tables can be dragged only within the same parent
                    else if (CQDDataObjectI::CurData->m_Image == qtiQueryExpr || CQDDataObjectI::CurData->m_Image == qtiQuery || CQDDataObjectI::CurData->m_Image == qtiTable || CQDDataObjectI::CurData->m_Image == qtiJoin) 
                    {
                        wxTreeItemId sp = m_Tree->GetItemParent(CQDDataObjectI::CurData->m_Item);
                        wxTreeItemId dp = m_Tree->GetItemParent(Item);
                        if (sp != dp && sp != Item)
                            m_Result = wxDragNone;
                    }
                }
            }
            // IF item can be dropped, set highlighted item
            if (wxIsDragResultOk(m_Result))
                SelItem = Item;
            else
                SelItem.Unset();
            // IF current tree item is not expanded AND has children, set expanding timeout
            if (!m_Tree->IsExpanded(Item) && m_Tree->GetChildrenCount(Item, false))
                m_ExpTime = wxGetLocalTimeMillis() + 1000;
            else
                m_ExpTime = MAX_EXPTIME;
        }
        // IF expanding time elapsed, expand current item
        else if (wxGetLocalTimeMillis() > m_ExpTime)
        {
#ifdef WINS  
            CWBDragImage::Image->Hide();
#endif            
            m_Tree->Expand(Item);
            m_Tree->Update();
#ifdef WINS  
            CWBDragImage::Image->Show();
#endif            
            m_ExpTime = MAX_EXPTIME;
        }
    }
    // IF above tree client area AND last item is valid
    else if (Hit == CPHR_ABOVE && m_Item.IsOk())
    {
        // IF previous visible sibling exists, scroll to previous visible sibling
        wxTreeItemId PrevItem = m_Tree->GetPrevVisible(m_Item);
        if (PrevItem.IsOk())
        {
            Item = PrevItem;
#ifdef WINS  
            CWBDragImage::Image->Hide();
#endif            
            m_Tree->EnsureVisible(PrevItem);
            m_Tree->Update();
#ifdef WINS  
            CWBDragImage::Image->Show();
#endif            
        }
        m_Result = wxDragNone;
    }
    // IF below tree client area AND last item is valid
    else if (Hit == CPHR_BELOW && m_Item.IsOk())
    {
        // IF folowing visible sibling exists, scroll to folowing visible sibling
        wxTreeItemId NextItem = m_Tree->GetNextVisible(m_Item);
        if (NextItem)
        {
            Item = NextItem;
#ifdef WINS  
            CWBDragImage::Image->Hide();
#endif            
            m_Tree->EnsureVisible(NextItem);
            m_Tree->Update();
#ifdef WINS  
            CWBDragImage::Image->Show();
#endif            
        }
        m_Result = wxDragNone;
    }
    // IF not abowe any tree item, cannot drop
    else if (!Item.IsOk())
    {
        m_Result = wxDragNone;
    }

    // IF current item changed
    if (Item != m_Item)
    {
        m_Item  = Item;
        m_Image = Image;
        // IF highlighted item changed, unhighlight old item, highlight new item
        if (SelItem != m_SelItem)
        {
#ifdef WINS  
            CWBDragImage::Image->Hide();
#endif            
            HighlightItem(m_SelItem, false);
            HighlightItem(SelItem,   true);
            m_Tree->Update();
#ifdef WINS  
            CWBDragImage::Image->Show();
#endif            
            m_SelItem = SelItem;
        }
    }

    return m_Result;
}
//
// Draged data drop event handler
//
bool CQDDropTargetTI::OnDrop(wxCoord x, wxCoord y)
{
    // Unhighlight old item
#ifdef WINS  
    CWBDragImage::Image->Hide();
#endif            
    HighlightItem(m_SelItem, false);
    m_Tree->wxWindow::Update();
    return true;
}
//
// Intrinsic draged data drop
//
wxDragResult CQDDropTargetTI::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
    // Get dragged DataObject
    GetData();
    CQDDataObjectI *Data     = (CQDDataObjectI *)GetDataObject();
    CQueryDesigner *Designer = (CQueryDesigner *)m_Tree->GetParent();
    // pd = parent of current item
    wxTreeItemId pd;
    if (m_Item.IsOk())
        pd = m_Tree->GetItemParent(m_Item);
    wxTreeItemId Parent;
    wxTreeItemId Prev;

    // SWITCH for dragged item type
    switch (Data->m_Image)
    {
    case qtiQueryExpr:
    case qtiQuery:
        // IF current item is query expression
        if (m_Image == qtiQueryExpr)
        {
            wxTreeItemId ps = m_Tree->GetItemParent(Data->m_Item);
            // IF dragg within same parent, move draged item behind current item
            if (pd.IsOk() && ps == pd)
            {
                Parent = pd;
                Prev   = m_Item;
            }
            // Move dragged item as first child of current query expression
            else
            {
                Parent = m_Item;
            }
        }
        // IF current item is FROM, move dragged item as first child of FROM
        else if (m_Image == qtiFrom)
        {
            Parent = m_Item;
        }
        // ELSE move draged item behind current item
        else
        {
            Parent = pd;
            Prev   = m_Item;
        }
        break;
    case qtiTable:
    case qtiJoin:
        // IF current item is FROM, move dragged item as first child of FROM
        if (m_Image == qtiFrom)
        {
            Parent = m_Item;
        }
        // ELSE move draged item behind current item
        else
        {
            Parent = pd;
            Prev   = m_Item;
        }
        break;
    case qtiColumn:
        // IF current item is SELECT, move dragged item as first child of SELECT
        if (m_Image == qtiSelect)
        {
            Parent = m_Item;
        }
        // ELSE move draged item behind current item
        else
        {
            Parent = pd;
            Prev   = m_Item;
        }
        break;
    case qtiCond:
    case qtiSubConds:
        // IF current item is WHERE or HAVING, move dragged item as first child of current item
        if (m_Image == qtiWhere || m_Image == qtiHaving)
        {
            Parent = m_Item;
        }
        // ELSE move draged item behind current item
        else
        {
            Parent = pd;
            Prev   = m_Item;
        }
        break;
    case qtiGColumn:
        // IF current item is GROUP BY, move dragged item as first child GROUP BY
        if (m_Image == qtiGroupBy)
        {
            Parent = m_Item;
        }
        // ELSE move draged item behind current item
        else
        {
            Parent = pd;
            Prev   = m_Item;
        }
        break;
    case qtiOColumn:
        // IF current item is ORDER BY, move dragged item as first child ORDER BY
        if (m_Image == qtiOrderBy)
        {
            Parent = m_Item;
        }
        else
        {
            Parent = pd;
            Prev   = m_Item;
        }
        break;
    }
    bool WasLast = false;
    // IF dragged item is query or condition
    if (Data->m_Image == qtiQueryExpr || Data->m_Image == qtiQuery || Data->m_Image == qtiCond || Data->m_Image == qtiSubConds)
    {
        // Get last item in dragged item parent before ORDER BY and LIMIT
        wxTreeItemId Last = m_Tree->GetLastChild(m_Tree->GetItemParent(Data->m_Item));
        int        lImage = m_Tree->GetItemImage(Last);
        if (lImage == qtiLimit)
        {
            Last   = m_Tree->GetPrevSibling(Last);
            lImage = m_Tree->GetItemImage(Last);
        }
        if (lImage == qtiOrderBy)
            Last   = m_Tree->GetPrevSibling(Last);
        // WasLast is true, when dragged item was last item in parent
        WasLast = Last == Data->m_Item;
    }
    size_t Before = 0;
    // Get previous item index
    if (Prev.IsOk())
    {
        Before = Designer->IndexOf(Prev) + 1;
        if (Before >= m_Tree->GetChildrenCount(Parent, false))
            Before = QDTREE_APPEND;
    }

    // Move dragged item to new position
    wxTreeItemId Item = Designer->CopyTreeItem(Parent, Data->m_Item, Before);
    m_Tree->Delete(Data->m_Item);
    // IF query dragged in query expression
    if ((Data->m_Image == qtiQueryExpr || Data->m_Image == qtiQuery) && m_Tree->GetItemImage(Parent) == qtiQueryExpr)
    {
        // IF dragged item is now the last item in parent
        if (Before == QDTREE_APPEND)
        {
            // Get previous sibling record
            CQDUQuery *Query  = (CQDUQuery *)m_Tree->GetItemData(Item);
            Prev              = m_Tree->GetPrevSibling(Item);
            if (Prev.IsOk())
            {
                // Update previous sibling association
                CQDUQuery *pQuery = (CQDUQuery *)m_Tree->GetItemData(Prev);
                if (pQuery->m_Assoc == QA_NONE)
                {
                    pQuery->m_Assoc  = Query->m_Assoc;
                    wxString Caption = Designer->GetTableName(pQuery, GTN_ALIAS | GTN_TITLE) + wxT(" - ") + QueryAssocTbl[pQuery->m_Assoc]; 
                    m_Tree->SetItemText(Prev, Caption);
                }
            }
            // Reset draged item association
            Query->m_Assoc = QA_NONE;
            m_Tree->SetItemText(Item, Designer->GetTableName(Query, GTN_ALIAS | GTN_TITLE));
        }
        // ELSE IF dragged query was the last item query expression
        else if (WasLast)
        {
            // Update association in current last two items
            Prev              = m_Tree->GetLastChild(Parent);
            CQDUQuery *pQuery = (CQDUQuery *)m_Tree->GetItemData(Prev);
            CQDUQuery *Query  = (CQDUQuery *)m_Tree->GetItemData(Item);
            Query->m_Assoc    = pQuery->m_Assoc;
            pQuery->m_Assoc   = QA_NONE;
            m_Tree->SetItemText(Prev, Designer->GetTableName(pQuery, GTN_ALIAS | GTN_TITLE));
            wxString Caption  = Designer->GetTableName(Query, GTN_ALIAS | GTN_TITLE) + wxT(" - ") + QueryAssocTbl[Query->m_Assoc]; ; 
            m_Tree->SetItemText(Item, Caption);
        }
    }
    // IF dragged item is condition
    if (Data->m_Image == qtiCond || Data->m_Image == qtiSubConds)
    {
        // IF now it is the last
        if (Before == QDTREE_APPEND)
        {
            CQDCond *Cond  = (CQDCond *)m_Tree->GetItemData(Item);
            Prev           = m_Tree->GetPrevSibling(Item);
            // Update assiciation in preceeding item
            if (Prev.IsOk())
            {
                CQDCond *pCond = (CQDCond *)m_Tree->GetItemData(Prev);
                if (pCond->m_Assoc == CA_NONE)
                {
                    pCond->m_Assoc   = Cond->m_Assoc;
                    wxString Caption = pCond->m_Expr + wxT(" - ") + AndOrTbl[pCond->m_Assoc]; 
                    m_Tree->SetItemText(Prev, Caption);
                }
            }
            // Reset association in current item
            Cond->m_Assoc = CA_NONE;
            m_Tree->SetItemText(Item, Cond->m_Expr);
        }
        // ELSE IF it was the last
        else if (WasLast)
        {
            // Update association in current last two items
            Prev             = m_Tree->GetLastChild(Parent);
            CQDCond *pCond   = (CQDCond *)m_Tree->GetItemData(Prev);
            CQDCond *Cond    = (CQDCond *)m_Tree->GetItemData(Item);
            Cond->m_Assoc    = pCond->m_Assoc;
            pCond->m_Assoc   = CA_NONE;
            m_Tree->SetItemText(Prev, pCond->m_Expr);
            wxString Caption = Cond->m_Expr + wxT(" - ") + AndOrTbl[Cond->m_Assoc]; ; 
            m_Tree->SetItemText(Item, Caption);
        }
    }
    // Redraw grid
    Designer->m_Grid->Refresh();
    Designer->changed = true;
    return def;
}
//
// Highlightes/Unhighlightes tree Item
//
void CQDDropTargetTI::HighlightItem(const wxTreeItemId &Item, bool Highlight)
{
    if (Item.IsOk())
    {
        m_Tree->SetItemTextColour(Item,       Highlight ? HighlightTextColour : m_Tree->GetForegroundColour());
        m_Tree->SetItemBackgroundColour(Item, Highlight ? HighlightColour     : m_Tree->GetBackgroundColour());
    }
}

///////////////////// Column from TableBox or element from elemnt popup DataObject //////////////////////////

CQDDataObjectE *CQDDataObjectE::CurData;
//
// Preferred data format is my own DF_QueryDesignerDataE
//
wxDataFormat CQDDataObjectE::GetPreferredFormat(Direction Dir) const
{
    return *DF_QueryDesignerDataE;
}
//
// Only one data format supported
//
size_t CQDDataObjectE::GetFormatCount(Direction Dir) const
{
    return 1;
}
//
// Only one, my own DF_QueryDesignerDataE, data format supported
//
void CQDDataObjectE::GetAllFormats(wxDataFormat *Formats, Direction Dir) const
{
    Formats[0] = *DF_QueryDesignerDataE;
}
//
// Returns size of dragged data
//
size_t CQDDataObjectE::GetDataSize(const wxDataFormat &Format) const
{
    if (Format == *DF_QueryDesignerDataE)
        return QDDATAOBJECT_SIZE;
    return 0;
}
//
// Writes to Buf dragged data in requested Format
//
bool CQDDataObjectE::GetDataHere(const wxDataFormat &Format, void *Buf) const
{
    if (Format == *DF_QueryDesignerDataE)
    {
        memcpy(Buf, m_Data, QDDATAOBJECT_SIZE);
        return true;
    }
    return false;
}
//
// Fills DataObject with data of given format
//
bool CQDDataObjectE::SetData(const wxDataFormat& Format, size_t Len, const void *Buf)
{
    if (Format == *DF_QueryDesignerDataE)
    {
        memcpy(m_Data, Buf, QDDATAOBJECT_SIZE);
        return true;
    }
    return false;
}

////////////////////////////////////// Design tree item DataObject //////////////////////////////////////////

CQDDataObjectI *CQDDataObjectI::CurData;
//
// Preferred data format is my own DF_QueryDesignerDataI
//
wxDataFormat CQDDataObjectI::GetPreferredFormat(Direction Dir) const
{
    return *DF_QueryDesignerDataI;
}
//
// Only one data format supported
//
size_t CQDDataObjectI::GetFormatCount(Direction Dir) const
{
    return 1;
}
//
// Only one, my own DF_QueryDesignerDataI, data format supported
//
void CQDDataObjectI::GetAllFormats(wxDataFormat *Formats, Direction Dir) const
{
    Formats[0] = *DF_QueryDesignerDataI;
}
//
// Returns size of dragged data
//
size_t CQDDataObjectI::GetDataSize(const wxDataFormat &Format) const
{
    if (Format == *DF_QueryDesignerDataI)
        return GetSize();
    return 0;
}
//
// Writes to Buf dragged data in requested Format
//
bool CQDDataObjectI::GetDataHere(const wxDataFormat &Format, void *Buf) const
{
    if (Format == *DF_QueryDesignerDataI)
    {
        memcpy(Buf, &m_Item, GetSize());
        return true;
    }
    return false;
}
//
// Fills DataObject with data of given format
//
bool CQDDataObjectI::SetData(const wxDataFormat& Format, size_t Len, const void *Buf)
{
    if (Format == *DF_QueryDesignerDataI)
    {
        memcpy(&m_Item, Buf, GetSize());
        return true;
    }
    return false;
}

////////////////////////////////////// Query designer drag source ///////////////////////////////////////////

//
// Feedback of Drag&Drop operation
//
bool CQDDragSource::GiveFeedback(wxDragResult effect)
{
    wxPoint Pos;
    bool Show = false;
    // IF current window is query designer table panel, grid, tree or element popup, show drag image

    for (wxWindow *Wnd = wxFindWindowAtPointer(Pos); Wnd; Wnd = Wnd->GetParent())
    {
        Show = Wnd == &m_Designer->m_Tables || Wnd == m_Designer->m_Grid || &m_Designer->m_Tree || Wnd == m_Designer->m_ElementPopup;
        if (Show)
            break;
    }
#ifdef WINS  
    if (Show)
    {
        CWBDragImage::Image->Show();
        CWBDragImage::Image->Move(m_Designer->ScreenToClient(Pos));
    }
    else 
    {
        CWBDragImage::Image->Hide();
    }
#endif    
    return false;
}

void query_delete_bitmaps(void)
{ delete_bitmaps(CQueryDesigner::tool_info); }

#if 0  // the old dialog
///////////////////////////////////// Query optimalization dialog ///////////////////////////////////////////

//
// Crates dialog
//
bool CQueryOptDlg::Create(wxWindow *Parent, cdp_t cdp, const char *Source)
{
    m_Source = Source;
    m_cdp    = cdp;
    // Create dialog frame
    if (!wxDialog::Create(Parent, 0, _("Query Optimization"), wxDefaultPosition, wxSize(600, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLIP_CHILDREN))
        return false;
    // Create sizers
    wxFlexGridSizer *MainSizer = new wxFlexGridSizer(2, 1, 0, 0);
    if (!MainSizer)
        return false;
    wxBoxSizer *BottomSizer = new wxBoxSizer(wxHORIZONTAL);
    if (!BottomSizer)
        return false;
    MainSizer->AddGrowableRow(0);
    MainSizer->AddGrowableCol(0);
    SetSizer(MainSizer);
    SetAutoLayout(true);
    // Create image list
    if (!m_ImageList.Create(16, 16, TRUE, 3))
        return false;
    { wxBitmap bmp(_table_xpm);   m_ImageList.Add(bmp); }
    { wxBitmap bmp(optimize_xpm); m_ImageList.Add(bmp); }
    { wxBitmap bmp(optcond_xpm);  m_ImageList.Add(bmp); }
    // Create optimization tree
    if (!m_Tree.Create(this, ID_TREE, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxSUNKEN_BORDER))
        return false;
    m_Tree.SetImageList(&m_ImageList);
    MainSizer->Add(&m_Tree, 0, wxEXPAND | wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxALL, 12);
    MainSizer->Add(BottomSizer, 0, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM | wxADJUST_MINSIZE, 12);
    // Buttons
    if (!m_EvalBut.Create(this, ID_EVAL, _("&Evaluate query"), wxDefaultPosition, wxDefaultSize))
        return false;
    BottomSizer->Add(&m_EvalBut,  1, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxADJUST_MINSIZE, 8);
    if (!m_CloseBut.Create(this, wxID_OK, _("&Close"), wxDefaultPosition, wxDefaultSize))
        return false;
    BottomSizer->Add(&m_CloseBut, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxADJUST_MINSIZE, 8);
    if (!m_HelpBut.Create(this, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize))
        return false;
    BottomSizer->Add(&m_HelpBut,  1, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxADJUST_MINSIZE, 8);

    ShowOpt(false);
    ShowModal();
    return true;
}
//
// Fills optimization tree
//
void CQueryOptDlg::ShowOpt(bool Eval)
{ 
    wxBusyCursor Busy;
    m_Tree.Freeze();
    // Clear old items
    m_Tree.DeleteAllItems();

    char *Result;
    // Ask server
    if (cd_Query_optimization(m_cdp, m_Source, Eval, &Result))
        cd_Signalize2(m_cdp, this);
    else 
    { 
        unsigned char *p = (unsigned char *)Result;
        bool Bold; 
        wxTreeItemId Parent;
        wxTreeItemId Last;
        int Image;
        // Scan Result and build optimization tree
        do
        { 
            Image = 2;
            Bold  = false;
            if (*p == 4)
            { 
                Image = 1;
                Bold  = true;
                p++;
            }
            if (*p == 5)
            {
                Image = 0;
                p++;
            }
            char *Text = (char*)p; 
            while (*p > 1)
                p++;
            if (*p == 1)
            {
                *p = 0;
                p++;
            }
            if (Parent.IsOk())
                Last = m_Tree.AppendItem(Parent, wxString(Text, *m_cdp->sys_conv), Image);
            else
                Last = m_Tree.AddRoot(wxString(Text, *m_cdp->sys_conv), Image);
            if (Bold)
                m_Tree.SetItemBold(Last);
            if (Parent.IsOk())
                m_Tree.Expand(Parent);
            // Add the item to the tree view control. 
            if (*p == 2)
            {
                p++;
                Parent = Last;
            }
            else while (*p == 3) 
            {
                p++;
                Parent = m_Tree.GetItemParent(Parent);
            }
        } 
        while (*p);
        corefree(Result);
    }
    m_Tree.Thaw();
}

BEGIN_EVENT_TABLE(CQueryOptDlg, wxDialog) 
    EVT_BUTTON(ID_EVAL,        CQueryOptDlg::OnEval)
    EVT_BUTTON(wxID_HELP,      CQueryOptDlg::OnHelp)
    EVT_TREE_KEY_DOWN(ID_TREE, CQueryOptDlg::OnKeyDown)
END_EVENT_TABLE()
//
// Evaluate query action
//
void CQueryOptDlg::OnEval(wxCommandEvent &event)
{
    ShowOpt(true);
}
//
// Key down event handler
//
void CQueryOptDlg::OnKeyDown(wxTreeEvent &event)
{
    event.Skip();
    if (event.GetKeyCode() == WXK_ESCAPE)
        EndModal(wxID_OK);
}
//
// Show help action
//
void CQueryOptDlg::OnHelp(wxCommandEvent &event)
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/optiminfo.html"));
}
#endif

