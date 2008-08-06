#ifndef _FODLG_H_
#define _FODLG_H_

#include "wx/notebook.h"
#include "myctrls.h"

#define FODLG_ID 10000
#define FODLG_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX|wxRESIZE_BORDER
#define FODLG_TITLE _("Filter and Order")

#define ID_NOTEBOOK 10001
#define ID_PANEL1 10006
#define ID_FILTERGRID 10008
#define ID_PANEL 10005
#define ID_ORDERGRID 10009
#define ID_PANEL2 10007
#define ID_FILTERTEXT 10011
#define ID_ORDERTEXT 10010
#define ID_VALIDATE 10002

class CFODlg;
//
// CGridFilter and CGridOrder common ancestor
//
class CFOItem
{
protected:

    CFOItem *m_Next;        // Link to next item

public:

    CFOItem(){m_Next = NULL;}
    virtual ~CFOItem(){}
    friend class CFOList;
    friend class CFilterGridTable;
    friend class COrderGridTable;
};
//
// CGridFilterList and CGridOrderList common ancestor
//
class CFOList
{
protected:

    CFOItem *m_First;       // Link to first item

    virtual CFOItem *NewItem() = 0;
    CFOItem *GetItem(int Index)
    {
        CFOItem *Item;
        for (Item = m_First; Index; Item = Item->m_Next, Index--);
        return(Item);
    }

public:

    CFOList(){m_First = NULL;}
    virtual ~CFOList(){Clear();}
    //
    // Returns item count
    //
    int Count()
    {
        int Result = 0;
        for (CFOItem *Item = m_First; Item; Item = Item->m_Next, Result++);
        return(Result);   
    }
    //
    // Sets item count
    //
    void SetCount(int Cnt)
    {
        CFOItem **Item = &m_First;
        for (int i = 0; i < Cnt; i++)
        {
            if (!*Item)
                *Item = NewItem();
            Item = &(*Item)->m_Next;
        }
    }
    //
    // Deletes Count items from Index-th position
    //
    void Delete(int Index, int Count)
    {
        int i;
        CFOItem **pItem;
        for (pItem = &m_First, i = 0; *pItem; pItem = &(*pItem)->m_Next, i++)
        {
            if (i == Index)
            {
                for (i = 0; i < Count; i++)
                {
                    CFOItem *Item = *pItem;
                    *pItem        = Item->m_Next;
                    if (!Item)
                        break;
                    delete Item;
                }
                return;
            }
        }
    }
    //
    // Clear the list
    //
    void Clear()
    {
        for (CFOItem *Item = m_First; Item;)
        {   
            CFOItem *Next = Item->m_Next;
            delete Item;
            Item = Next;
        }
        m_First = NULL;
    }
    friend class CFilterGridTable;
    friend class COrderGridTable;
};
//
// Simple filter condition
//
class CGridFilter : public CFOItem
{
public:

    wxString Column;        // Column name
    wxString Operator;      // Operator
    wxString Value;         // Column value

    CGridFilter *GetNext(){return((CGridFilter *)m_Next);}
    virtual ~CGridFilter(){}
};
//
// List of filter conditions
//
class CGridFilterList : public CFOList
{
protected:

    virtual CFOItem *NewItem(){return new CGridFilter();}
    
public:

    CGridFilter &operator[](int Index){return *(CGridFilter *)GetItem(Index);}
    CGridFilter *GetFirst(){return((CGridFilter *)m_First);}
};
//
// Simple order expression
//
class CGridOrder : public CFOItem
{
public:

    wxString Column;        // Column name
    bool     Desc;          // Descending flag

    CGridOrder(){Desc = false;}
    virtual ~CGridOrder(){}
    CGridOrder *GetNext(){return((CGridOrder *)m_Next);}
};
//
// List of order expressions
//
class CGridOrderList : public CFOList
{
protected:

    virtual CFOItem *NewItem(){return new CGridOrder();}
    
public:

    CGridOrder &operator[](int Index){return *(CGridOrder *)GetItem(Index);}
    CGridOrder *GetFirst(){return((CGridOrder *)m_First);}
};
//
// CFilterGridTable and COrderGridTable common ancestor
//
class CFOGridTable : public wxGridTableBase
{
protected:

    CFODlg   *m_Dlg;        // Owning dialog

public:

    CFOGridTable(){}
    CFOGridTable(CFODlg *Dlg){m_Dlg = Dlg;}
    virtual void SetCount(int Cnt) = 0;
    virtual bool IsEmpty(int Row) = 0;
    virtual bool IsEmptyCell(int row, int col){return(false);}
    virtual bool IsTextEditor(int Col) = 0;
    virtual int  GetItemCount() = 0;
    virtual CFOList *GetList() = 0;
    virtual bool DeleteRows(size_t pos = 0, size_t numRows = 1){GetList()->Delete((int)pos, (int)numRows); return(true);}
    virtual int  GetNumberRows(){return(GetItemCount() + 1);}
};
//
// Filter grid datasource
//
class CFilterGridTable : public CFOGridTable
{
protected:

    CGridFilterList m_Filter;   // List of filter conditions

    wxString BuildCond();

public:

    CFilterGridTable(){}
    CFilterGridTable(CFODlg *Dlg) : CFOGridTable(Dlg){}

    virtual int      GetNumberCols(){return(3);}
    virtual wxString GetValue( int row, int col );
    virtual void     SetValue( int row, int col, const wxString& value );
    virtual wxString GetColLabelValue( int col );
    virtual void     SetCount(int Cnt){m_Filter.SetCount(Cnt);}
    virtual bool     IsEmpty(int Row){return(Row >= m_Filter.Count() || (m_Filter[Row].Column.IsEmpty() && m_Filter[Row].Operator.IsEmpty() && m_Filter[Row].Value.IsEmpty()));}
    virtual bool     IsTextEditor(int Col){return(true);}
    virtual CFOList *GetList(){return(&m_Filter);}
    virtual int      GetItemCount(){return(m_Filter.Count());};

    friend class CFODlg;
};
//
// Order grid datasource
//
class COrderGridTable : public CFOGridTable
{
protected:

    CGridOrderList m_Order;     // List of order expressions

    wxString BuildCond();

public:

    COrderGridTable(){}
    COrderGridTable(CFODlg *Dlg) : CFOGridTable(Dlg){}

    virtual int      GetNumberCols(){return(2);}
    virtual wxString GetValue(int row, int col);
    virtual void     SetValue(int row, int col, const wxString& value);
    virtual wxString GetColLabelValue(int col);
    virtual void     SetCount(int Cnt){m_Order.SetCount(Cnt);}
    virtual bool     IsEmpty(int Row){return(Row >= m_Order.Count() || m_Order[Row].Column.IsEmpty());}
    virtual bool     IsTextEditor(int Col){return(Col == 0);}
    virtual CFOList *GetList(){return(&m_Order);}
    virtual int      GetItemCount(){return(m_Order.Count());};

    friend class CFODlg;
};
//
// Filter and order specification grid
//
class CFOGrid : public wxGrid
{
protected:

    DECLARE_EVENT_TABLE()

    bool    m_Canceled;     // In place editing was with escape terminated
    CFODlg *m_Dlg;          // Owning dialog

    void OnKeyDown(wxKeyEvent &event);
    void OnCreateEditor(wxGridEditorCreatedEvent &event);
    void RefreshRowCount();
    void NextCell(bool Forward);

public:

    CFOGrid(wxWindow* parent, wxWindowID id, CFODlg *Dlg) : wxGrid(parent, id, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
    {
        m_Canceled    = false;
        m_Dlg         = Dlg;
    }
    friend class CFilterGridTable;
    friend class COrderGridTable;
    friend class CFODlg;
};
//
// Filter and order specification dialog
//
class CFODlg : public wxDialog
{
protected:

    DECLARE_CLASS( CFODlg )
    DECLARE_EVENT_TABLE()

    view_dyn        *m_inst;                    // Source grid description
    wxArrayString    m_fAttrs;                  // Column list for filter grid inplace editor
    wxArrayString    m_oAttrs;                  // Column list for order grid inplace editor
    wxNotebook      *m_Notebook;                // Tabbed notebook
    CFOGrid         *m_FilterGrid;              // Filter grid
    CFOGrid         *m_OrderGrid;               // Order  grid
    CFilterGridTable m_Filter;                  // Filter grid data source
    COrderGridTable  m_Order;                   // Order  grid data source
    wxTextCtrl      *m_WhereED;                 // Filter text editor
    wxTextCtrl      *m_OrderED;                 // Order  text editor
    wxButton        *m_ValidateBut;             // Validate button
    const d_table   *m_Def;                     // Source table/cursor definition
    bool             m_FilterTextChanged;       // Filter text changed
    bool             m_OrderTextChanged;        // Order  text changed
    bool             m_FromInit;                // Prevents buildning filter and order expressions during dialog initializing

    void OnFilterTextChanged(wxCommandEvent &event){m_FilterTextChanged = true;}
    void OnOrderTextChanged(wxCommandEvent &event){m_OrderTextChanged = true;}
    void OnNavigationKey(wxNavigationKeyEvent &event);
    void OnPageChanging(wxNotebookEvent &event);
    void OnPageChanged(wxNotebookEvent &event);
    void OnValidate(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);
    void OnOK(wxCommandEvent &event);
    void OnHelp(wxCommandEvent &event);

    void AddAttrCol(CFOGrid *Grid);
    void AddOperatorCol(CFOGrid *Grid);
    void AddTextCol(CFOGrid *Grid);
    void AddDescCol(wxGrid *Grid);
    bool LoadFilterGrid(bool Init);
    bool LoadOrderGrid(bool Init);
    bool IsAttr(const wxString &Label);
    int  FontSize(const wxFont &font);
    int  RowSize();
    const wxArrayString &GetAttrs(bool Filter);
    const char *GetAttrName(const d_table *Def, const d_attr *at, char *Buf);
    const char *GetAttrPart(const char *Attr, char *Buf, const char **Next);
    wxString BuildWhereCond(){return(m_Filter.BuildCond());}
    wxString BuildOrderCond(){return(m_Order.BuildCond());}
    wxString LabelToAttr(const wxString &Label);
    wxString AttrToLabel(const char *Attr);
    CFOGrid *GetGrid();
    //
    // Returs table/cursor definition
    //
    const d_table *GetDef()
    {
        if (!m_Def)
        {
            if (m_inst->cdp)
                m_Def = cd_get_table_d(m_inst->cdp, m_inst->dt->cursnum, IS_CURSOR_NUM(m_inst->dt->cursnum) ? CATEG_DIRCUR : CATEG_TABLE);
            else
                m_Def = make_result_descriptor9(m_inst->dt->conn, (HSTMT)m_inst->dt->hStmt);
            //else
            //    m_Def = make_odbc_descriptor9(xcdp->get_odbcconn(), tabname, odbc_prefix, TRUE);
        }
        return(m_Def);
    }
    xcdp_t Getxcd()
    {
        if (m_inst->cdp)
            return m_inst->cdp;
        else
            return m_inst->xcdp;
    }

public:
    /// Constructors
    CFODlg(){}
    CFODlg(wxWindow *parent, view_dyn *inst) : m_Filter(this), m_Order(this)
    {
        m_inst              = inst;
        m_FilterTextChanged = false;
        m_OrderTextChanged  = false;
        m_FromInit          = false;
        m_OrderGrid         = NULL;
        m_OrderED           = NULL;
        m_Def               = NULL;
        m_FilterGrid        = NULL;
        m_OrderGrid         = NULL;
        Create(parent);
    }
    ~CFODlg()
    {
        if (m_FilterGrid)
            m_FilterGrid->SetTable(NULL);
        if (m_OrderGrid)
            m_OrderGrid->SetTable(NULL);
        if (m_Def)
            release_table_d(m_Def);
    }

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = FODLG_ID, const wxString& caption = FODLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = FODLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();
    static bool ShowToolTips();

    friend class CFOGrid;
    friend class CFilterGridTable;
    friend class COrderGridTable;
    friend class CFOGridEditor;
};

#endif  // _FODLG_H_
