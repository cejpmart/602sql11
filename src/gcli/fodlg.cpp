//
// Filter and order specification dialog
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
#include "querydef.h"
#pragma hdrstop

#include "fodlg.h"
#include "wx/arrimpl.cpp"

#ifdef LINUX
#if WBVERS<1000
#define SQLExecDirectA SQLExecDirect
#endif
#endif

IMPLEMENT_CLASS(CFODlg, wxDialog)

BEGIN_EVENT_TABLE(CFODlg, wxDialog)
    EVT_NOTEBOOK_PAGE_CHANGING(ID_NOTEBOOK, CFODlg::OnPageChanging)
    EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, CFODlg::OnPageChanged)
    EVT_TEXT(ID_FILTERTEXT, CFODlg::OnFilterTextChanged) 
    EVT_TEXT(ID_ORDERTEXT,  CFODlg::OnOrderTextChanged) 
    EVT_NAVIGATION_KEY(CFODlg::OnNavigationKey)
    EVT_BUTTON(ID_VALIDATE, CFODlg::OnValidate)
    EVT_BUTTON(wxID_OK,     CFODlg::OnOK)
    EVT_BUTTON(wxID_CANCEL, CFODlg::OnCancel) 
    EVT_BUTTON(wxID_HELP,   CFODlg::OnHelp)
END_EVENT_TABLE()
//
// Creates dialog
//
bool CFODlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();                   // Create controls 
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();                           // Centre dialog in parent window
    if (!LoadFilterGrid(true))          // Load filter conditions to filter grid
    {                                   // IF filter condition cannot be edited in grid, swith to texteditor
        m_FromInit = true;
        m_Notebook->SetSelection(2);
    }
    LoadOrderGrid(true);                // Load order expressions to grid

    return TRUE;
}
//
// Creates dialog controls
//
void CFODlg::CreateControls()
{    
    wxFlexGridSizer* item2 = new wxFlexGridSizer(2, 1, 0, 0);
    item2->AddGrowableRow(0);
    item2->AddGrowableCol(0);
    this->SetSizer(item2);
    this->SetAutoLayout(TRUE);
    m_Notebook = new wxNotebook( this, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );
    wxPanel* item4 = new wxPanel(m_Notebook, ID_PANEL1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxGridSizer* item5 = new wxGridSizer(1, 1, 0, 0);
    item4->SetSizer(item5);
    item4->SetAutoLayout(TRUE);
    m_FilterGrid = new CFOGrid( item4, ID_FILTERGRID, this);
    //m_FilterGrid->SetSizeHints(380, 140);
    m_FilterGrid->SetRowLabelSize(20);
    m_FilterGrid->SetTable(&m_Filter);
    m_FilterGrid->SetLabelFont(*grid_label_font);
    m_FilterGrid->SetDefaultCellFont(*grid_cell_font);
    m_FilterGrid->SetColLabelSize(FontSize(*grid_label_font));
    m_FilterGrid->SetDefaultRowSize(RowSize());
    m_FilterGrid->EnableDragRowSize(false);
    AddAttrCol(m_FilterGrid);
    AddOperatorCol(m_FilterGrid);
    AddTextCol(m_FilterGrid);
    item5->Add(m_FilterGrid, 0, wxGROW|wxALL, 5);
    m_Notebook->AddPage(item4, _("Filter"));
    wxPanel* item7 = new wxPanel(m_Notebook, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxGridSizer* item8 = new wxGridSizer(1, 1, 0, 0);
    item7->SetSizer(item8);
    item7->SetAutoLayout(TRUE);
    m_OrderGrid = new CFOGrid( item7, ID_ORDERGRID, this);
    m_OrderGrid->SetRowLabelSize(20);
    m_OrderGrid->SetTable(&m_Order);
    m_OrderGrid->SetLabelFont(*grid_label_font);
    m_OrderGrid->SetDefaultCellFont(*grid_cell_font);
    m_OrderGrid->SetColLabelSize(FontSize(*grid_label_font));
    m_OrderGrid->SetDefaultRowSize(RowSize());
    m_OrderGrid->EnableDragRowSize(false);
    AddAttrCol(m_OrderGrid);
    AddDescCol(m_OrderGrid);
    item8->Add(m_OrderGrid, 0, wxGROW|wxGROW|wxALL, 5);
    m_Notebook->AddPage(item7, _("Sort"));
    wxPanel* item10 = new wxPanel(m_Notebook, ID_PANEL2, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxFlexGridSizer* item11 = new wxFlexGridSizer(4, 1, 0, 0);
    item11->AddGrowableRow(1);
    item11->AddGrowableRow(3);
    item11->AddGrowableCol(0);
    item10->SetSizer(item11);
    item10->SetAutoLayout(TRUE);
    wxStaticText* item12 = new wxStaticText( item10, wxID_STATIC, _("WHERE Condition"), wxDefaultPosition, wxDefaultSize, 0 );
    item11->Add(item12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 8);
    m_WhereED = new wxTextCtrl( item10, ID_FILTERTEXT, wxString(m_inst->filter, GetConv(Getxcd())), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    m_WhereED->SetSizeHints(364, 48);
    item11->Add(m_WhereED, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxStaticText* item14 = new wxStaticText( item10, wxID_STATIC, _("ORDER BY Clause"), wxDefaultPosition, wxDefaultSize, 0 );
    item11->Add(item14, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 8);
    m_OrderED = new wxTextCtrl( item10, ID_ORDERTEXT, wxString(m_inst->order, GetConv(Getxcd())), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    m_OrderED->SetSizeHints(364, 48);
    item11->Add(m_OrderED, 0, wxGROW|wxGROW|wxALL, 5);
    m_Notebook->AddPage(item10, _("By SQL"));
    item2->Add(m_Notebook, 0, wxGROW|wxGROW|wxALL|wxADJUST_MINSIZE, 5);
    wxBoxSizer* item16 = new wxBoxSizer(wxHORIZONTAL);
    m_ValidateBut = new wxButton(this, ID_VALIDATE, _("Validate"), wxDefaultPosition, wxDefaultSize, 0 );
    item16->Add(m_ValidateBut, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxButton* item18 = new wxButton(this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item18->SetDefault();
    item16->Add(item18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxButton* item19 = new wxButton(this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item16->Add(item19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxButton* item20 = new wxButton(this, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    item16->Add(item20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    item2->Add(item16, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 5);
    m_FilterGrid->SetFocus();
}
//
// Creates column name inplace editor for filter or order grid 
//
void CFODlg::AddAttrCol(CFOGrid *Grid)
{
    Grid->SetColSize(0, 100);
    wxGridCellAttr *CellAttr = new wxGridCellAttr();
    if (CellAttr)
    { 
#ifdef ORIG_VITA
        CMyGridComboEditor *CellEdit = new CMyGridComboEditor(Grid, GetAttrs(Grid->GetId() == ID_FILTERGRID));
#else
        wxGridCellODChoiceEditor * CellEdit = new wxGridCellODChoiceEditor(GetAttrs(Grid->GetId() == ID_FILTERGRID), true);
#endif        
        if (CellEdit)
            CellAttr->SetEditor(CellEdit);
        Grid->SetColAttr(0, CellAttr);
    }
}

#define _W5_ ('_' | ('W' << 8) | ('5' << 16) | ('_' << 24))
//
// Returs column names list for inplace editor
//
const wxArrayString &CFODlg::GetAttrs(bool Filter)
{
    CBuffer Buf;
    wxArrayString &Attrs = Filter ? m_fAttrs : m_oAttrs;
    // IF list not exists yet
    if (!Attrs.Count())
    {
        // Get source table/cursor definition
        const d_table *Def = GetDef();
        if (Def)
        {
            char name[3 * (OBJ_NAME_LEN + 3)];
            int i;
            // FOR each column in definition
            for (i = 1; i < Def->attrcnt; i++)
			{
                const d_attr *at = &Def->attribs[i];
                // Skip hidden or BLOB/CLOB columns
                if (*(DWORD *)at->name != _W5_ && (Filter || !IS_HEAP_TYPE(at->type)))
                {
                    const char *nm = GetAttrName(Def, at, name);
    				wxString Attr(nm, GetConv(Getxcd()));
                    // Convert column name to source grid label
                    wxString Label = AttrToLabel(nm);
                    wxString sAttr = Attr;
                    if (sAttr[0] == wxT('`'))
                        sAttr = sAttr.Mid(1, sAttr.Length() - 2);
                    // IF column name not matches grid label, set name as GridLabel (ColumnName)
                    if (wxStricmp(sAttr, Label) != 0)
                        Label += wxT(" (") + Attr + wxT(')');
                    // Add name to list
                    Attrs.Add(Label);
                }
			}
            Attrs.Sort();
        }
    }
    return Attrs;
}
//
// Returns column name from table definition, appends table name prefix if needed
//
const char *CFODlg::GetAttrName(const d_table *Def, const d_attr *at, char *Buf)
{
    char *pn = Buf;
    // IF column name needs prefix
    if (at->needs_prefix)
    {
        // Get prefix schema name
        const char *p  = COLNAME_PREFIX(Def, at->prefnum);
        // IF specified
        if (*p)
        {
            // Store store schema name to buf, cover schema name in `` if needed
            ident_to_sql(Getxcd(), pn, p);
            if (*pn == '`')
                pn++;
            wb_small_string(m_inst->cdp, pn, true);
            pn   += strlen(pn);
            // Append .
            *pn++ = '.';
        }
        // Get prefix table name
        p += OBJ_NAME_LEN + 1;
        // Append table name to buf, cover table name in `` if needed
        ident_to_sql(Getxcd(), pn, p);
        if (*pn == '`')
            pn++;
        wb_small_string(m_inst->cdp, pn, true);
        pn   += strlen(pn);
        // Append .
        *pn++ = '.';
    }
    // Append column name to buf, cover column name in `` if needed
    ident_to_sql(Getxcd(), pn, at->name);
    if (*pn == '`')
        pn++;
    wb_small_string(m_inst->cdp, pn, true);
    return Buf;
}
//
// Adds operator column to filter grid
//
void CFODlg::AddOperatorCol(CFOGrid *Grid)
{
    // Get BETWEEN extent in pixels
    wxScreenDC dc;
    dc.SetFont(wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT));
    wxCoord w, h, descent, externalLeading;	
    dc.GetTextExtent(wxT("BETWEEN"), &w, &h, &descent, &externalLeading);
    // Set column width
	Grid->SetColSize(1, w);
    // Create inplace cell editor with list of operators
    wxGridCellAttr *CellAttr = new wxGridCellAttr();
    if (CellAttr)
    {
#ifdef ORIG_VITA
        CMyGridComboEditor *CellEdit = new CMyGridComboEditor(Grid, wxArrayString());
#else
        wxGridCellODChoiceEditor * CellEdit = new wxGridCellODChoiceEditor(wxArrayString(), true);
#endif        
        if (CellEdit)
        {
            CellEdit->SetParameters(wxT("=,<>,>,>=,<,<=,IN,BETWEEN,LIKE,IS NULL"));
            CellAttr->SetEditor(CellEdit);
        }
        Grid->SetColAttr(1, CellAttr);
    }
}
//
// Adds simple text colum to grid
//
void CFODlg::AddTextCol(CFOGrid *Grid)
{
    Grid->SetColSize(2,    152);
    wxGridCellAttr *CellAttr = new wxGridCellAttr();
    if (CellAttr)
    {
        CellAttr->SetOverflow(false);
#ifdef ORIG_VITA
        CMyGridTextEditor *CellEdit = new CMyGridTextEditor(Grid);
#else
        wxGridCellTextEditor * CellEdit = new wxGridCellTextEditor();
#endif        
        if (CellEdit)
            CellAttr->SetEditor(CellEdit);
        Grid->SetColAttr(2, CellAttr);
    }
}
//
// Adds descending column to order grid
//
void CFODlg::AddDescCol(wxGrid *Grid)
{
    Grid->SetColSize(1,    40);
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
        Grid->SetColAttr(1, CellAttr);
    }
}

bool CFODlg::ShowToolTips()
{
    return TRUE;
}
//
// Returns row height for given font
//
int CFODlg::FontSize(const wxFont &font)
{ 
    wxScreenDC dc;
    dc.SetFont(font);
    wxCoord w, h, descent, externalLeading;
    dc.GetTextExtent(wxT("pQgy�"), &w, &h, &descent, &externalLeading);
    return (h + descent + externalLeading) * 5 / 4;
}
//
// Returns row height derived from combobox height
//
int CFODlg::RowSize()
{
    wxOwnerDrawnComboBox cb(this, 0, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL);
    return(cb.GetSize().y - 1);
}
//
// Navigation key event handler
//
void CFODlg::OnNavigationKey(wxNavigationKeyEvent &event)
{
    CFOGrid *Grid = GetGrid();
    if (Grid && Grid->IsCellEditControlEnabled())
    {
        Grid->NextCell(event.GetDirection());
        return;
    }
    event.Skip();
}
//
// Page changing event handler
//
void CFODlg::OnPageChanging(wxNotebookEvent &event)
{
    // IF current page is text editor page AND filter and order expressions cannot be stored to grid, veto event
    if (m_Notebook->GetSelection() == 2 && (!LoadFilterGrid(false) || !LoadOrderGrid(false)))
    {
        event.Veto();
        return;
    }
    // Close current cell editor
	m_FilterGrid->DisableCellEditControl();
    if (m_OrderGrid)
	    m_OrderGrid->DisableCellEditControl();
    event.Skip();
}
//
// Page changed event handler
//
void CFODlg::OnPageChanged(wxNotebookEvent &event)
{
    // Ignore if order editor not exists yet
    if (!m_OrderED)
        return;
    // IF new page is text editor
    if (event.GetSelection() == 2)
    {
        // Ignore if it is due to initial expression cannot be converted to list of condition for grid
        if (m_FromInit)
            m_FromInit = false;
        else
        {
            // Build filter condition from filter grid content
            m_WhereED->SetValue(BuildWhereCond());
            // Build order expression from order grid content
            m_OrderED->SetValue(BuildOrderCond());
            m_FilterTextChanged = false;
            m_OrderTextChanged  = false;
        }
    }
    event.Skip();
}

#define SyntaxOK _("Syntax is correct")
//
// Syntax check event handler
//
void CFODlg::OnValidate(wxCommandEvent &event)
{
    wxString Filter;
    wxString Order;
    // Close cell editor
	m_FilterGrid->DisableCellEditControl();
	m_OrderGrid->DisableCellEditControl();
    // IF current page is text editor page, get Filter and Order from text editors
    if (m_Notebook->GetSelection() == 2)
    {
        Filter = m_WhereED->GetValue();
        Order  = m_OrderED->GetValue();
    }
    // ELSE build Filter and Order by grids content
    else
    {
        Filter = BuildWhereCond();
        Order  = BuildOrderCond();
    }
    char *Query = NULL;
    // IF 602sql server connnection
    if (m_inst->cdp)
    {
        // Convert Filter and Order to ASCII
        int Len = 1 + (int)Filter.Length() + 1 + 9 + (int)Order.Length() + 1;
        Query   = (char *)corealloc(Len, 47);
        if (!Query)
        {
            no_memory();
            return;
        }
        // With QUERY_TEST_MARK
        Query[0] = (char)QUERY_TEST_MARK;
        char *pq = Query + 1;
        pq      += GetConv(m_inst->cdp).WC2MB(pq, Filter.c_str(), Len - 1);
        if (!Order.IsEmpty())
        {
            *pq++    = 1;
            strcpy(pq, "ORDER BY ");
            GetConv(m_inst->cdp).WC2MB(pq + 9, Order.c_str(), Len - (pq - Query));
        }
        tcursnum supercurs = m_inst->dt->supercurs == NOOBJECT ? m_inst->dt->cursnum : (tcursnum)m_inst->dt->supercurs;
        tcursnum subcurs;
        // Check expression syntax by opening subcursor
        if (cd_Open_subcursor(m_inst->cdp, supercurs, Query, &subcurs))
            cd_Signalize2(m_inst->cdp, this);
        else 
            info_box(SyntaxOK, this);
    }
    // ELSE ODBC datasource connection
    else
    {
        int lc  = (int)strlen(m_inst->dt->select_st);
        int Len = 15 + lc + 1 +/*20+*/ 7 + (int)Filter.Length() + 10 + (int)Order.Length() + 1;
        Query   = (char *)corealloc(Len, 47);
        if (!Query)
        {
            no_memory();
            return;
        }
        // Create SELECT * FROM (source query) statement text
        strcpy(Query, "SELECT * FROM (");
        char *pq = Query + 15;
        memcpy(pq, m_inst->dt->select_st, lc);
        pq += lc;
        *pq++ = ')';
        //strcpy(pq, " AS ABCXYZ12 ");  pq+=strlen(pq);  -- Oracle does not accept this, MySQL needs this
        // IF Filter is not empty, append Filter
        if (!Filter.IsEmpty())
        {
            strcpy(pq, " WHERE ");
            pq += 7;
            pq += GetConv(Getxcd()).WC2MB(pq, Filter.c_str(), Len - (pq - Query));
        }
        // IF Order is not empty, append Order
        if (!Order.IsEmpty())
        {
            strcpy(pq, " ORDER BY ");
            GetConv(Getxcd()).WC2MB(pq + 10, Order.c_str(), Len - (pq - Query));
        }
        HSTMT hStmt;
        // Create ODBC statement
        RETCODE rc = create_odbc_statement(m_inst->xcdp->get_odbcconn(), &hStmt);
        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO || rc == SQL_NO_DATA)
        {
            // Check expression syntax by executing statement
            rc = SQLExecDirectA(hStmt, (SQLCHAR *)Query, SQL_NTS);
            if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO || rc == SQL_NO_DATA)
            {
                info_box(SyntaxOK, this);
                SQLCloseCursor(hStmt);
            }
            else
            {
                odbc_stmt_error(m_inst->xcdp->get_odbcconn(), hStmt);
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
    }
    corefree(Query);
}
//
// Help event handler - Closes cell editor, shows help window
//
void CFODlg::OnHelp(wxCommandEvent &event)
{
	m_FilterGrid->DisableCellEditControl();
	m_OrderGrid->DisableCellEditControl();
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/grid_filter.html"));
}
//
// Cancel button click event handler, handles Escape hit in inplace cell editor too
//
void CFODlg::OnCancel(wxCommandEvent &event)
{
    CFOGrid *Grid = GetGrid();
    // IF cell editor is open
    if (Grid && Grid->IsCellEditControlEnabled())
    {
        // Set flag to not save editor value
        Grid->m_Canceled = true;
        // Close cell editor
        Grid->DisableCellEditControl();
        // IF Escape hit, do not continue event handling
        if (wxGetKeyState(WXK_ESCAPE))
            return;
    }
    event.Skip();
}
//
// OK button click event handler
//
void CFODlg::OnOK(wxCommandEvent &event)
{
    tptr     fCond = NULL;
    tptr     oCond = NULL;
    wxString wCond;
    // Close cell editor
	m_FilterGrid->DisableCellEditControl();
	m_OrderGrid->DisableCellEditControl();
    // IF Filter text changed, get filter from text editor
    if (m_FilterTextChanged)
        wCond = m_WhereED->GetValue();
    // ELSE build filter by grid content
    else
        wCond = BuildWhereCond();
    // Convert filter to ASCII
    int Len = (int)wCond.Length();
    if (Len)
    {
        fCond = sigalloc(Len + 1, 34);
        if (!fCond)
            return;
        GetConv(Getxcd()).WC2MB(fCond, wCond.c_str(), Len + 1);
    }
    // IF Order text changed, get Order from text editor
    if (m_OrderTextChanged)
        wCond = m_OrderED->GetValue();
    // ELSE build order by grid content
    else
        wCond = BuildOrderCond();
    // Convert order to ASCII
    Len   = (int)wCond.Length();
    if (Len)
    {
        oCond = sigalloc(Len + 1, 34);
        if (!oCond)
            return;
        GetConv(Getxcd()).WC2MB(oCond, wCond.c_str(), Len + 1);
    }
    // Apply filter and order
    if (!m_inst->accept_query(fCond, oCond))
    {
        corefree(fCond);
        corefree(oCond);
        event.Skip(false);
        return;
    }
    event.Skip();
}
//
// Fills filter grid with list of filter conditions
//
bool CFODlg::LoadFilterGrid(bool Init)
{
    // IF not during dialog initialization AND filter text not changed, nothing to do
    if (!Init && !m_FilterTextChanged)
        return(true);
    cdp_t cdp = m_inst->cdp;
    CFilterConds Conds;
    CFilterCond *Cond;
    // Get filter text from editor
    wxString FilterCond = m_WhereED->GetValue().Trim();
    if (cdp && !FilterCond.IsEmpty())
    {
        // Compile filter
        int Err = compile_filter(cdp, FilterCond.mb_str(GetConv(cdp)), &Conds);
        // IF syntax error
        if (Err)
        {
            if (cdp)
            {
                // Show error message
                client_error(cdp, Err);  
                cd_Signalize2(cdp, this);
                // Set error position in editor
                m_WhereED->SetInsertionPoint(m_WhereED->XYToPosition(cdp->RV.comp_err_column - 1, cdp->RV.comp_err_line - 1));
            }
            return(false);
        }
        // Check condition list
        for (Cond = Conds.GetFirst(); Cond; Cond = Cond->GetNext())
        {
            if (Cond->Not || (Cond->Assoc != CA_AND && (Cond->Assoc != CA_NONE || Cond->GetNext() != NULL)))
                break;
            if (Cond->SubConds.GetFirst())
                break;
        }
        // IF contains NOT, subconditions OR other assotiation other then AND, disable grid editing
        if (Cond)
        {
            if (!Init)
                info_box(_("WHERE Condition is too complex, it will be opened in the text editor"), this);
            return(false);
        }
    }
    int Count = 0;
    m_Filter.m_Filter.Clear();
    // Fill filter grid datasource
    for (Cond = Conds.GetFirst(); Cond; Cond = Cond->GetNext(), Count++)
    {
        m_Filter.m_Filter.SetCount(Count + 1);
        CGridFilter *Filter = &m_Filter.m_Filter[Count];
        Filter->Column   = AttrToLabel(Cond->ColExpr);
        Filter->Operator = wxString(Cond->Oper, *wxConvCurrent);
        Filter->Value    = wxString(Cond->ValExpr, GetConv(Getxcd()));
    }
    m_FilterGrid->RefreshRowCount();
    return(true);
}
//
// Fills filter grid with list of filter conditions
//
bool CFODlg::LoadOrderGrid(bool Init)
{
    // IF not during dialog initialization AND order text not changed, nothing to do
    if (!Init && !m_OrderTextChanged)
        return(true);
    cdp_t cdp = m_inst->cdp;
    COrderConds Conds;
    CQOrder    *Cond;
    // Get order text from editor
    wxString    OrderCond = m_OrderED->GetValue();
    if (cdp && !OrderCond.IsEmpty())
    {
        // Compile order
        int Err = compile_order(cdp, OrderCond.mb_str(GetConv(cdp)), &Conds);
        // IF syntax error
        if (Err)
        {
            if (cdp)
            {
                // Show error message
                client_error(cdp, Err);  
                cd_Signalize2(cdp, this);
                // Set error position in editor
                m_OrderED->SetInsertionPoint(m_WhereED->XYToPosition(cdp->RV.comp_err_column - 1, cdp->RV.comp_err_line - 1));
            }
            return(false);
        }
    }
    int Count = 0;
    m_Order.m_Order.Clear();
    // Fill order grid datasource
    for (Cond = Conds.GetFirst(); Cond; Cond = Cond->GetNext(), Count++)
    {
        m_Order.m_Order.SetCount(Count + 1);
        CGridOrder *Order = &m_Order.m_Order[Count];
        Order->Column     = AttrToLabel(Cond->Expr);
        Order->Desc       = Cond->Desc;
    }
    m_OrderGrid->RefreshRowCount();
    return(true);
}
//
// Returns current grid
//
CFOGrid *CFODlg::GetGrid()
{
    int Page = m_Notebook->GetSelection();
    if (Page == 0)
        return(m_FilterGrid);
    else if (Page == 1)
        return(m_OrderGrid);
    else
        return(NULL);
}
//
// Returns true if Label is source table/cursor column name
//
bool CFODlg::IsAttr(const wxString &Label)
{
    char sLabel[256];
    // Convert Label to ASCII
    GetConv(Getxcd()).WC2MB(sLabel, Label.c_str(), sizeof(sLabel));
    // Remove ``
    if (*sLabel == '`')
    {
        int Len = (int)strlen(sLabel);
        Len -= 2;
        memcpy(sLabel, sLabel + 1, Len);
        sLabel[Len] = 0;
    }
    // Get source table/cursor definition
    const d_table *Def = GetDef();
    if (!Def)
        return(false);
    // Find Label column
    for (int i = 1; i < Def->attrcnt; i++)
	{
        if (wb_stricmp(m_inst->cdp, Def->attribs[i].name, sLabel) == 0)
            return(true);
	}
    return(false);
}
//
// Converts "GridTitle (ColumnName)" alternatively "GridTitle" to column name
//
wxString CFODlg::LabelToAttr(const wxString &Label)
{
    DataGrid *Grid   = m_inst->hWnd;

    wxString Lab = Label;
    wxString Attr;
    // IF label contains (
    int lb = Label.Find(wxT('('));
    if (lb != wxNOT_FOUND)
    {
        int rb = Label.Find(wxT(')'), true);
        if (rb == wxNOT_FOUND || rb < lb)
            return wxString();
        // Lab = text before (
        Lab  = Label.Left(lb);
        Lab.Trim(true);
        // Attr = text between brackets
        Attr = Label.Mid(lb + 1, rb - lb - 1); 
    }

    // FOR each grid column
    for (int Col = 0; Col < Grid->GetNumberCols(); Col++)
    {
        // Search Lab column
        wxString Val = Grid->GetColLabelValue(Col);
        // IF not found, continue
        if (Val.CmpNoCase(Lab))
            continue;
        t_ctrl *itm;
        int i;
        // Find corresponding data item at same x position
        for (i = 0, itm = FIRST_CONTROL(m_inst->stat); i < m_inst->stat->vwItemCount; i++, itm = NEXT_CONTROL(itm))
        {
            // IF found
            if (itm->pane == PANE_DATA && itm->ctX == m_inst->subinst->sitems[Col].itm->ctX)
            {
                tptr text = item_text(itm);
                bool Same = true;
                // IF Attr part of Label found
                if (!Attr.IsEmpty())
                {
                    tobjname sname, dname;
                    wxCharBuffer cb;
                    const char *sAttr, *dAttr;
                    // Get source table/cursor definition
                    const d_table *Def = GetDef();
                    if (!Def)
                        break;
                    cb    = Attr.mb_str(GetConv(Getxcd()));
                    sAttr = cb;
                    dAttr = text;
                    // Compare column name
                    do
                        Same = wb_stricmp(m_inst->cdp, GetAttrPart(sAttr, sname, &sAttr), GetAttrPart(dAttr, dname, &dAttr)) == 0;
                    while (Same && sAttr != 0 && dAttr != 0);
                    if (sAttr != NULL || dAttr != NULL)
                        Same = false;
                }
                if (Same)
                {
                    wxString Result(text, GetConv(Getxcd()));
                    return CaptFirst(Result);
                }
                break;
            }
        }
    }
    return wxString();
}
//
// Converts source table/cursor column name to grid column label
//
wxString CFODlg::AttrToLabel(const char *Attr)
{
    tobjname sname, dname;
    const char *sAttr, *dAttr;
    DataGrid *Grid   = m_inst->hWnd;

    // FOR each data item
    for (int Col = 0; Col < Grid->GetNumberCols(); Col++)
    {
        sAttr = Attr;
        dAttr = item_text(m_inst->subinst->sitems[Col].itm);
        bool Same;
        // Compare source column name
        do
            Same = wb_stricmp(m_inst->cdp, GetAttrPart(sAttr, sname, &sAttr), GetAttrPart(dAttr, dname, &dAttr)) == 0;
        while (Same && sAttr != NULL && dAttr != NULL);
        // IF found, return corresponding grid label
        if (Same && sAttr == NULL && dAttr == NULL)
            return Grid->GetColLabelValue(Col);
    }
    return wxString(Attr, GetConv(m_inst->cdp));
}
//
// Returns first part of column name, in Next returns pointer to next to next part or NULL if no next part follows
//
const char *CFODlg::GetAttrPart(const char *Attr, char *Buf, const char **Next)
{
    const char *pe;
    *Next = NULL;
    // IF Attr starts with `, copy to buf text between ``
    if (*Attr == '`')
    {
        pe = strchr(Attr + 1, '`');
        if (pe)
        {
            int sz  = pe - Attr - 1;
            memcpy(Buf, Attr + 1, sz);
            Buf[sz] = 0;
            pe++;
            if (*pe == '.')
                *Next = pe + 1;
            return Buf;
        }
    }
    // IF Attr contains ., copy to buf text before .
    pe = strchr(Attr, '.');
    if (pe)
    {
        int sz  = pe - Attr - 1;
        memcpy(Buf, Attr + 1, sz);
        Buf[sz] = 0;
        *Next = pe + 1;
        return Buf;
    }
    return Attr;
}


#define foColumn _("Column")
//
// Returns filter grid column labels
//
wxString CFilterGridTable::GetColLabelValue( int Col )
{
    if (Col == 0)
        return foColumn;
    else if (Col == 1)
        return _("Oper");
    else
        return _("Value");
}
//
// Returns filter grid cell values
//
wxString CFilterGridTable::GetValue( int Row, int Col )
{
    if (Row >= m_Filter.Count())
        return wxString();
    if (Col == 0)
        return m_Filter[Row].Column;
    else if (Col == 1)
        return m_Filter[Row].Operator;
    else
        return m_Filter[Row].Value;
}
//
// Saves filter grid cell values
//
void CFilterGridTable::SetValue( int Row, int Col, const wxString& value )
{
    CFOGrid *Grid = (CFOGrid *)GetView();
    // Ignore if edit was canceled with Escape hit
    if (Grid->m_Canceled)
    {
        Grid->m_Canceled = false;
        return;
    }
    if (Row >= m_Filter.Count())
        m_Filter.SetCount(Row + 1);
    if (Col == 0)
        m_Filter[Row].Column = value;
    else if (Col == 1)
        m_Filter[Row].Operator = value;
    else
        m_Filter[Row].Value = value;
}
//
// Builds filter condition
//
wxString CFilterGridTable::BuildCond()
{
    wxString Result;
    wxString Val;
    wchar_t qc = L'`';
    t_connection *conn = m_Dlg->m_inst->xcdp->get_odbcconn();
    if (conn)
        qc = conn->identifier_quote_char;
    // FOR each simple condition in list
    for (CGridFilter *Cond = m_Filter.GetFirst(); Cond; Cond = Cond->GetNext())
    {
        // IF Column value is column, append `` if needed
        if (m_Dlg->IsAttr(Cond->Column))
            Val = GetProperIdent(Cond->Column, qc);
        else
        {
            // Convert grid label to column name
            Val = m_Dlg->LabelToAttr(Cond->Column);
            // IF conversion failed, use Column content
            if (Val.IsEmpty())
                Val = Cond->Column;
            // ELSE cover column name to `` if needed
            else
            {
                int i;
                wxString v, s;
                while ((i = Val.Find(wxT('.'))) != wxNOT_FOUND) 
                {
                    s   = Val.Left(i);
                    Val = Val.Mid(i + 1);
                    v  += GetProperIdent(s, qc) + wxT('.');
                }
                Val = v + GetProperIdent(Val, qc);
            }
        }
        // Append column name
        Result += Val;
        Result += ' ';
        // Append operator
        Result += Cond->Operator;
        Result += ' ';
        // Append column value
        Result += Cond->Value;
        // Append AND
        Result += wxT(" AND ");
    }
    // Truncate last AND
    int Len = (int)Result.Length();
    if (Len)
        Result.Truncate(Len - 5);
    return Result;
}
//
// Returns order grid column labels
//
wxString COrderGridTable::GetColLabelValue( int Col )
{
    if (Col == 0)
        return foColumn;
    else
        return wxT("DESC");
}
//
// Returns order grid cell value
//
wxString COrderGridTable::GetValue( int Row, int Col )
{
    if (Row >= m_Order.Count())
        return wxString();
    if (Col == 0)
        return m_Order[Row].Column;
    else
        return m_Order[Row].Desc ? wxT("1") : wxString();
}
//
// Saves order grid cell value
//
void COrderGridTable::SetValue(int Row, int Col, const wxString& value)
{
    CFOGrid *Grid = (CFOGrid *)GetView();
    // Ignore if edit was canceled with Escape hit
    if (Grid->m_Canceled)
    {
        Grid->m_Canceled = false;
        return;
    }
    if (Row >= m_Order.Count())
        m_Order.SetCount(Row + 1);
    if (Col == 0)
        m_Order[Row].Column = value;
    else
        m_Order[Row].Desc = !value.IsEmpty();
}
//
// Builds ORDER BY experssion
//
wxString COrderGridTable::BuildCond()
{
    wxString Result;
    wxString Val;
    wchar_t qc = L'`';
    t_connection *conn = m_Dlg->m_inst->xcdp->get_odbcconn();
    if (conn)
        qc = conn->identifier_quote_char;
    // FOR each simple expression in list
    for (CGridOrder *Cond = m_Order.GetFirst(); Cond; Cond = Cond->GetNext())
    {
        // IF Column value is column name, append `` if needed
        if (m_Dlg->IsAttr(Cond->Column))
            Val = GetProperIdent(Cond->Column, qc);
        else
        {
            // Convert grid label to column name
            Val = m_Dlg->LabelToAttr(Cond->Column);
            // IF conversion failed, use column content
            if (Val.IsEmpty())
                Val = Cond->Column;
            // ELSE cover column name to `` if needed
            else
            {
                int i;
                wxString v, s;
                while ((i = Val.Find(wxT('.'))) != wxNOT_FOUND) 
                {
                    s   = Val.Left(i);
                    Val = Val.Mid(i + 1);
                    v  += GetProperIdent(s, qc) + wxT('.');
                }
                Val = v + GetProperIdent(Val, qc);
            }
        }
        // Append column name
        Result += Val;
        // Append DESC
        if (Cond->Desc)
            Result += wxT(" DESC");
        // Append,
        Result += wxT(", ");
    }
    // Truncate last ,
    int Len = (int)Result.Length();
    if (Len)
        Result.Truncate(Len - 2);
    return Result;
}

BEGIN_EVENT_TABLE(CFOGrid, wxGrid)
    EVT_KEY_DOWN(CFOGrid::OnKeyDown)
    EVT_GRID_EDITOR_CREATED(CFOGrid::OnCreateEditor)
END_EVENT_TABLE()
//
// Key down event handler
//
void CFOGrid::OnKeyDown(wxKeyEvent &event)
{
    int Code = event.GetKeyCode();
    // Otherwise Shift hit invokes inplace edit mode
    if (Code == WXK_SHIFT)      
        return;
    // IF Last row AND last row is not amptry append new row
    if (Code == WXK_DOWN)       
    {
        int Cnt = GetNumberRows();
        int Row = GetGridCursorRow();
        if (Row >= Cnt - 1 && !((CFOGridTable *)GetTable())->IsEmpty(Row))
            RefreshRowCount();
    }
    // Move to next cell
    else if (Code == WXK_TAB)   
    {
        NextCell(!event.ShiftDown());
        return;
    }
    else if (Code == WXK_DELETE)
    {
        CFOGridTable *Table = (CFOGridTable *)GetTable();
        wxArrayInt Rows = GetSelectedRows();
        // IF single cell selected
        int Count = (int)Rows.Count();
        if (!Count)
        {
            // Clear cell value
            Table->SetValue(GetGridCursorRow(), GetGridCursorCol(), wxString());
            ForceRefresh();
        }
        else
        {
            int i;
            // Remove selected rows
            for (i = 0; i < Count; i++)
            {
                Table->DeleteRows(Rows[i]);
                for (int j = i + 1; j < Count; j++)
                    Rows[j]--;
            }
            RefreshRowCount();
            if (Count > 1)
                ClearSelection();
        }
        return;
    }
    // Move to next cell
    else if (Code == WXK_RETURN)
    {
        NextCell(true);
        return;
    }
#ifdef LINUX
    // Cancel dialog
    else if (Code == WXK_ESCAPE)
	{
		m_Dlg->EndModal(wxID_CANCEL);
	}
#endif
    event.Skip();
}
//
// Moves to next cell
//
void CFOGrid::NextCell(bool Forward)
{
    int Row = GetGridCursorRow();
    int Col = GetGridCursorCol();
    CFOGridTable *Table = (CFOGridTable *)GetTable();
    // IF forward
    if (Forward)
    {
        // IF Col < column count, next column
        if (Col < Table->GetNumberCols() - 1)
            Col++;
        // ELSE IF Row < row count, first column on next row
        else if (Row < Table->GetItemCount() - 1)
        {
            Col = 0;
            Row++;
        }
        // ELSE IF Row is not empty, append row
        else if (!Table->IsEmpty(Row))
        {
            RefreshRowCount();
            Col = 0;
            Row++;
        }
        // ELSE set focus to Validate button
        else
        {
            ((CFODlg *)(GetParent()->GetParent()->GetParent()))->m_ValidateBut->SetFocus();
            return;
        }
    }
    // ELSE backward
    else
    {
        // IF not first column, previous column
        if (Col > 0)
            Col--;
        // IF not first row, last column on previous row
        else if (Row > 0)
        {
            Col = Table->GetNumberCols() - 1;
            Row--;
        }
        // ELSE set focus to notebook tab
        else
        {
            ((CFODlg *)(GetParent()->GetParent()->GetParent()))->m_Notebook->SetFocus();
            return;
        }
    }
    SetGridCursor(Row, Col);
}
//
// Refreshes row count according data source
//
void CFOGrid::RefreshRowCount()
{
    CFOGridTable *Table = (CFOGridTable *)GetTable();
    // Get row increment
    int ItemCount = Table->GetItemCount();
    int RowInc    = ItemCount - GetNumberRows();
    // IF row count == 0 OR last row is not empty, increment row increment
    if (ItemCount == 0 || !Table->IsEmpty(ItemCount - 1))
        RowInc++;
    if (!RowInc)
        return;

    // Append or delete rows
    if (RowInc > 0)
    {
        wxGridTableMessage msg(GetTable(), wxGRIDTABLE_NOTIFY_ROWS_APPENDED, RowInc);
        ProcessTableMessage(msg);
    }
    else
    {
        wxGridTableMessage msg(GetTable(), wxGRIDTABLE_NOTIFY_ROWS_DELETED,  GetNumberRows(), -RowInc);
        ProcessTableMessage(msg);
    }
    ForceRefresh();
}
//
// Create editor event handler - assignes inplace editor control event handler of cell editor
//
void CFOGrid::OnCreateEditor(wxGridEditorCreatedEvent &event)
{
    if (((CFOGridTable *)GetTable())->IsTextEditor(event.GetCol()))
    {   wxControl *Ctrl = event.GetControl();
#ifdef ORIG_VITA
        CMyGridTextEditor *Edit = (CMyGridTextEditor *)(wxGridCellTextEditor *)GetCellEditor(event.GetRow(), event.GetCol());
        Ctrl->PushEventHandler(Edit);
#else
        wxGridCellTextEditor * Edit = (wxGridCellTextEditor *)GetCellEditor(event.GetRow(), event.GetCol());
#endif        
        Ctrl->SetWindowStyleFlag(Ctrl->GetWindowStyleFlag() | wxPROCESS_ENTER);
    }
}

#include "myctrls.cpp"

