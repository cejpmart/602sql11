/////////////////////////////////////////////////////////////////////////////
// Name:        xmlformwizard.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     27/02/2006 11:04:13
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLFORMWIZARD_H_
#define _XMLFORMWIZARD_H_

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "xmlformwizard.h"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/wizard.h"
#include "wx/treectrl.h"
#include "wx/grid.h"
////@end includes
#include "xmlforms.h"

/*!
 * Forward declarations
 */

////@begin forward declarations
class CXMLFormWizardLayoutPage;
class CXMLFormWizardSourcePage;
class CXMLFormWizardDestPage;
class CXMLFormWizardStructPage;
class wxTreeCtrl;
class wxGrid;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_XMLFORMWIZARD 10002
#define SYMBOL_CXMLFORMWIZARD_IDNAME ID_XMLFORMWIZARD
#define ID_LAYOUTPAGE 10008
#define ID_TEXTCTRL 10001
#define ID_TYPEREPORT 10030
#define ID_TYPEINS 10031
#define ID_TYPEUPD 10032
#define ID_LAYOUTBUT0 10020
#define ID_BITMAPBUT0 10010
#define ID_LAYOUTBUT1 10021
#define ID_BITMAPBUTTON3 10011
#define ID_LAYOUTBUT2 10022
#define ID_BITMAPBUTTON2 10012
#define ID_SOURCEPAGE 10003
#define ID_FROMTABLE 10004
#define ID_FROMQUERY 10005
#define ID_FROMSQL 10015
#define ID_FROMDAD 10006
#define ID_LISTBOX1 10007
#define ID_QUERYED 10016
#define ID_DADNAME 10017
#define ID_DESTPAGE 10000
#define ID_LISTBOX 10025
#define ID_STRUCTPAGE 10013
#define ID_TREECTRL 10009
#define ID_GRID1 10018
#define ID_BUTTON 10019
#define ID_BUTTON1 10020
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

enum TXMLFormSource     // XML form data source
{
    XFS_DAD,            // DAD
    XFS_TABLE,          // Table
    XFS_QUERY,          // Stored query
    XFS_SQL             // Select statement
};
/*!
 * CXMLFormWizard class declaration
 */

class CXMLFormWizard: public wxWizard
{    
    DECLARE_DYNAMIC_CLASS( CXMLFormWizard )
    DECLARE_EVENT_TABLE()
    
    cdp_t m_cdp;            // cdp
    bool  m_FromDAD;        // Form based on DAD
    bool  m_DefaultDAD;     // Form based table or query, default DAD created

    bool GetQueryDAD(const char *Query);
    bool LoadDad(const char *DadName, bool Dst = false);

public:
    /// Constructors
    CXMLFormWizard( );
    CXMLFormWizard( wxWindow* parent, wxWindowID id = SYMBOL_CXMLFORMWIZARD_IDNAME, const wxPoint& pos = wxDefaultPosition );
    CXMLFormWizard( wxWindow* parent, cdp_t cdp, const char *DAD);

    /// Creation
    bool Create( wxWindow* parent, const char *DAD, wxWindowID id = SYMBOL_CXMLFORMWIZARD_IDNAME, const wxPoint& pos = wxDefaultPosition );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CXMLFormWizard event handler declarations

    /// wxEVT_WIZARD_FINISHED event handler for ID_XMLFORMWIZARD
    void OnXmlformwizardFinished( wxWizardEvent& event );

    /// wxEVT_WIZARD_HELP event handler for ID_XMLFORMWIZARD
    void OnXmlformwizardHelp( wxWizardEvent& event );

////@end CXMLFormWizard event handler declarations

////@begin CXMLFormWizard member function declarations

    /// Runs the wizard.
    bool Run();

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CXMLFormWizard member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CXMLFormWizard member variables
    CXMLFormWizardLayoutPage* LayoutPage;
    CXMLFormWizardSourcePage* SourcePage;
    CXMLFormWizardDestPage* DestPage;
    CXMLFormWizardStructPage* StructPage;
////@end CXMLFormWizard member variables

    wxString GetDAD() const;
    t_dad_root *GetDadTree() const;
    void SetDadTree(t_dad_root *aDadTree, bool Dst = false);

    CXMLFormStruct *GetStruct();
    TXMLFormSource GetSource() const;

    friend class CXMLFormWizardSourcePage;
    friend class CXMLFormWizardDestPage;
    friend class CXMLFormWizardLayoutPage;
    friend class CXMLFormWizardStructPage;
};
//
// Form layout tree item data - pointer to group struct
//
class CLevelTreeItemData : public wxTreeItemData
{
protected:
    
    CXMLFormStructLevel *Level;

public:

    CLevelTreeItemData(CXMLFormStructLevel *aLevel){Level = aLevel;}

    friend class CXMLFormWizardStructPage;
};

/*!
 * CXMLFormWizardLayoutPage class declaration
 */

class CXMLFormWizardLayoutPage: public wxWizardPageSimple
{    
    DECLARE_DYNAMIC_CLASS( CXMLFormWizardLayoutPage )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CXMLFormWizardLayoutPage( );

    CXMLFormWizardLayoutPage( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CXMLFormWizardLayoutPage event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_LAYOUTPAGE
    void OnLayoutpagePageChanging( wxWizardEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_TYPEREPORT
    void OnTypebutSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUT0
    void OnButtonClick( wxCommandEvent& event );

////@end CXMLFormWizardLayoutPage event handler declarations

////@begin CXMLFormWizardLayoutPage member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CXMLFormWizardLayoutPage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CXMLFormWizardLayoutPage member variables
    wxTextCtrl* FormNameED;
////@end CXMLFormWizardLayoutPage member variables
};

/*!
 * CXMLFormWizardSourcePage class declaration
 */

class CXMLFormWizardSourcePage: public wxWizardPageSimple
{    
    DECLARE_DYNAMIC_CLASS( CXMLFormWizardSourcePage )
    DECLARE_EVENT_TABLE()

protected:

    cdp_t m_cdp;            // cdp
    bool  m_ListLoaded;     // Indicates if object list load is needed
    bool  m_Changed;        // Change on page is made

    void LoadList(tcateg Categ);
    void SetDADName();

    cdp_t Getcdp(){return ((CXMLFormWizard *)GetParent())->m_cdp;}
    wxString CXMLFormWizardSourcePage::GetDAD() const;

public:
    /// Constructors
    CXMLFormWizardSourcePage( );

    CXMLFormWizardSourcePage( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CXMLFormWizardSourcePage event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for ID_SOURCEPAGE
    void OnSourcepagePageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_SOURCEPAGE
    void OnSourcepagePageChanging( wxWizardEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_FROMTABLE
    void OnRadiobuttonSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX1
    void OnListSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_QUERYED
    void OnQueryedUpdated( wxCommandEvent& event );

////@end CXMLFormWizardSourcePage event handler declarations

////@begin CXMLFormWizardSourcePage member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CXMLFormWizardSourcePage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CXMLFormWizardSourcePage member variables
    wxStaticBoxSizer* Sizer;
    wxRadioButton* TableRB;
    wxRadioButton* QueryRB;
    wxRadioButton* SQLRB;
    wxRadioButton* DADRB;
    wxListBox* List;
    wxTextCtrl* QueryED;
    wxStaticText* DADNameLab;
    wxTextCtrl* DADNameED;
////@end CXMLFormWizardSourcePage member variables
    void SetDadTree(t_dad_root *aDadTree);
    friend class CXMLFormWizard;
};

/*!
 * CXMLFormWizardDestPage class declaration
 */

class CXMLFormWizardDestPage: public wxWizardPageSimple
{    
    DECLARE_DYNAMIC_CLASS( CXMLFormWizardDestPage )
    DECLARE_EVENT_TABLE()

protected:

    cdp_t m_cdp;            // cdp
    bool  m_ListLoaded;     // Indicates if object list load is needed
    bool  m_Changed;        // Change on page is made

    void LoadList(tcateg Categ);
    void SetDADName();

    cdp_t Getcdp(){return ((CXMLFormWizard *)GetParent())->m_cdp;}
    //wxString CXMLFormWizardDestPage::GetDAD() const;

public:
    /// Constructors
    CXMLFormWizardDestPage( );

    CXMLFormWizardDestPage( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CXMLFormWizardDestPage event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for ID_DESTPAGE
    void OnDestpagePageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_DESTPAGE
    void OnDestpagePageChanging( wxWizardEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX
    void OnListBoxSelected( wxCommandEvent& event );

////@end CXMLFormWizardDestPage event handler declarations

////@begin CXMLFormWizardDestPage member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CXMLFormWizardDestPage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CXMLFormWizardDestPage member variables
    wxStaticBoxSizer* Sizer;
    wxListBox* List;
////@end CXMLFormWizardDestPage member variables

    friend class CXMLFormWizard;
};

/*!
 * CXMLFormWizardStructPage class declaration
 */

class CXMLFormWizardStructPage: public wxWizardPageSimple, public CXMLFormStruct
{    
    DECLARE_DYNAMIC_CLASS( CXMLFormWizardStructPage )
    DECLARE_EVENT_TABLE()

protected:

    wxString             m_DAD;             // DAD name
    cdp_t                m_cdp;             // cdp
    int                  m_ElemSelRow;      // Selected element row index   
    bool                 m_ElemChanged;     // Element row values are changed
    bool                 m_Loaded;          // Indicates if page content load is needed
    bool                 m_InElemSelCell;   // Element cell selection lock
    CXMLFormStructLevel *m_SelLevel;        // Selected element group
    bool                 m_InDrag;          // Row drag indicator
    int                  m_DragPos;         // Current drag y coordinate
    int                  m_Dragged;         // Dragged row index

    CXMLFormStructNode **BuildTree(const wxTreeItemId &Parent, const dad_element_node *node, CXMLFormStructNode **pLevel, const char *Path);
    CXMLFormStructNode **BuildNode(const wxTreeItemId &Parent, const dad_element_node *Node, CXMLFormStructNode **pElem, char *Path);
    bool GetDataElements(wxArrayString &Elems, CXMLFormStructLevel *Level, bool SubLevel);
    bool IsElemValid(int Row);
    bool IsAggregatableElem(const wxString &Path);
    bool LevelIsEmpty(int Row);
    void AddDataElems(dad_element_node *node, const wxString &xPath, wxArrayString &Elems, bool SubLevel);
    void SaveElem(int Row);
    bool NextElemRow(int Row);
    void ShowAggrCols(bool Show);
    void RemoveAggrs(CXMLFormStructLevel *Level);
    void StopDrag();
    void FillGrid();
    void RemoveElems(CXMLFormStructNode **Elem);

    dad_node *GetNode(dad_element_node *Node, const char *Name);
    wxWindow *GetSrc(wxGrid **Grid, int *Row, int *Col);
    bool IsCombo(wxGrid *Grid, int Col);
    wxTreeItemId FindNotEmptyLevel(const wxTreeItemId &Parent);

    void OnLabelMotion( wxMouseEvent& event );

#ifdef WINS

    virtual bool MSWProcessMessage(WXMSG* pMsg);

#endif

public:
    /// Constructors
    CXMLFormWizardStructPage( );

    CXMLFormWizardStructPage( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CXMLFormWizardStructPage event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for ID_STRUCTPAGE
    void OnStructpagePageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_STRUCTPAGE
    void OnStructpagePageChanging( wxWizardEvent& event );

    /// wxEVT_COMMAND_TREE_SEL_CHANGED event handler for ID_TREECTRL
    void OnTreectrlSelChanged( wxTreeEvent& event );

    /// wxEVT_GRID_LABEL_LEFT_CLICK event handler for ID_GRID1
    void OnLabelLeftClick( wxGridEvent& event );

    /// wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID1
    void OnElemCellChange( wxGridEvent& event );

    /// wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID1
    void OnElemSelectCell( wxGridEvent& event );

    /// wxEVT_LEFT_DOWN event handler for ID_GRID1
    void OnElemLeftDown( wxMouseEvent& event );

    /// wxEVT_LEFT_UP event handler for ID_GRID1
    void OnLeftUp( wxMouseEvent& event );

    /// wxEVT_MOTION event handler for ID_GRID1
    void OnMotion( wxMouseEvent& event );

    /// wxEVT_KEY_DOWN event handler for ID_GRID1
    void OnKeyDown( wxKeyEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
    void OnAddButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1
    void OnRemoveButton1Click( wxCommandEvent& event );

////@end CXMLFormWizardStructPage event handler declarations

////@begin CXMLFormWizardStructPage member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CXMLFormWizardStructPage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CXMLFormWizardStructPage member variables
    wxTreeCtrl* RepLevelTree;
    wxGrid* RepElemGrid;
    wxButton* AddBut;
    wxButton* RemoveBut;
////@end CXMLFormWizardStructPage member variables
    friend class CXMLFormWizard;
    friend class CXMLFormWizardDestPage;
    friend class CXMLFormWizardSourcePage;
    friend class CXMLFormWizardLayoutPage;
};

#endif
    // _XMLFORMWIZARD_H_
