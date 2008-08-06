/////////////////////////////////////////////////////////////////////////////
// Name:        qdprops.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/17/04 13:08:16
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _QDPROPS_H_
#define _QDPROPS_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "qdprops.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
#include "wx/spinctrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxSpinCtrl;
////@end forward declarations

extern const char qdSection[];
/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_CQDPROPSDLG_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CQDPROPSDLG_TITLE _("Query Designer Properties")
#define SYMBOL_CQDPROPSDLG_IDNAME ID_DIALOG
#define SYMBOL_CQDPROPSDLG_SIZE wxSize(400, 300)
#define SYMBOL_CQDPROPSDLG_POSITION wxDefaultPosition
#define ID_NOTEBOOK 10001
#define ID_PANEL 10002
#define ID_RADIOBUTTON0 0
#define ID_RADIOBUTTON1 1
#define ID_RADIOBUTTON2 2
#define ID_CHECKBOX1 10007
#define ID_SPINCTRL1 10008
#define ID_SPINCTRL2 10009
#define ID_TEXTCTRL 10005
#define ID_PANEL1 10003
#define ID_CHECKBOX 10010
#define ID_CHECKBOX2 10011
#define ID_CHECKBOX3 10012
#define ID_CHECKBOX4 10013
#define ID_TEXTCTRL1 10006
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CQDPropsDlg class declaration
 */

class CQDPropsDlg: public wxDialog
{    
    DECLARE_CLASS( CQDPropsDlg )
    DECLARE_EVENT_TABLE()

    bool m_InPageChange;

    void ShowSample1();
    void ShowSample2();
    void PutLine(const wxChar *Val, int Indent = 0);

public:
    /// Constructors
    CQDPropsDlg( );
    CQDPropsDlg( wxWindow* parent, wxWindowID id = SYMBOL_CQDPROPSDLG_IDNAME, const wxString& caption = SYMBOL_CQDPROPSDLG_TITLE, const wxPoint& pos = SYMBOL_CQDPROPSDLG_POSITION, const wxSize& size = SYMBOL_CQDPROPSDLG_SIZE, long style = SYMBOL_CQDPROPSDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CQDPROPSDLG_IDNAME, const wxString& caption = SYMBOL_CQDPROPSDLG_TITLE, const wxPoint& pos = SYMBOL_CQDPROPSDLG_POSITION, const wxSize& size = SYMBOL_CQDPROPSDLG_SIZE, long style = SYMBOL_CQDPROPSDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CQDPropsDlg event handler declarations

    void OnPageChanging(wxNotebookEvent &event);
    void OnPageChanged(wxNotebookEvent &event);
    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON0
    void OnRadioSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX1
    void OnWrapItemsClick( wxCommandEvent& event );
    void OnWrapAfterClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL1
    void OnIndentChanged( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL2
    void OnRowLengthChanged( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX
    void OnConvertClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX2
    void OnPrefixColumnsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX3
    void OnPrefixTablesClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

////@end CQDPropsDlg event handler declarations

////@begin CQDPropsDlg member function declarations

////@end CQDPropsDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CQDPropsDlg member variables
    wxRadioButton* m_DontWrapRB;
    wxRadioButton* m_WrapAllwaysRB;
    wxRadioButton* m_WrapLongRB;
    wxCheckBox* m_WrapItemsCB;
    wxCheckBox* m_WrapAfterClauseCB;
    wxSpinCtrl* m_BlockIndentUD;
    wxSpinCtrl* m_RowLengthUD;
    wxTextCtrl* m_Sample1;
    wxCheckBox* m_ConvertJoinCB;
    wxCheckBox* m_PrefixColumnsCB;
    wxCheckBox* m_PrefixTablesCB;
    wxTextCtrl* m_Sample2;
////@end CQDPropsDlg member variables

    int      m_Wrap;         
    int      m_Indent;       
    int      m_RowLength;    
    bool     m_WrapItems;     
    bool     m_WrapAfterClause;     
    bool     m_ConvertJoin;  
    bool     m_PrefixColumns;
    bool     m_PrefixTables; 
    wxString m_Sample;
};

#endif
    // _QDPROPS_H_
