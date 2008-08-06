/////////////////////////////////////////////////////////////////////////////
// Name:        XMLKeyGenDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     11/03/06 10:49:20
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLKEYGENDLG_H_
#define _XMLKEYGENDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "XMLKeyGenDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxOwnerDrawnComboBox;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_MYDIALOG10 10134
#define SYMBOL_XMLKEYGENDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_XMLKEYGENDLG_TITLE _("Generating keys")
#define SYMBOL_XMLKEYGENDLG_IDNAME ID_MYDIALOG10
#define SYMBOL_XMLKEYGENDLG_SIZE wxSize(400, 300)
#define SYMBOL_XMLKEYGENDLG_POSITION wxDefaultPosition
#define CD_KG_ID_COLUMN_CAPT 10139
#define CD_KG_ID_COLUMN 10135
#define CD_KG_EXPR_CAPT 10138
#define CD_KG_EXPR_CAPT2 10140
#define CD_KG_EXPR 10136
#define CD_KG_TYPE 10137
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif
#ifndef wxFIXED_MINSIZE
#define wxFIXED_MINSIZE 0
#endif

/*!
 * XMLKeyGenDlg class declaration
 */

class XMLKeyGenDlg: public wxDialog
{    
    DECLARE_CLASS( XMLKeyGenDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    XMLKeyGenDlg(XMLElementDlg * owning_dlgIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_XMLKEYGENDLG_IDNAME, const wxString& caption = SYMBOL_XMLKEYGENDLG_TITLE, const wxPoint& pos = SYMBOL_XMLKEYGENDLG_POSITION, const wxSize& size = SYMBOL_XMLKEYGENDLG_SIZE, long style = SYMBOL_XMLKEYGENDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin XMLKeyGenDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_KG_TYPE
    void OnCdKgTypeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end XMLKeyGenDlg event handler declarations

////@begin XMLKeyGenDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end XMLKeyGenDlg member function declarations
    void enabling(bool en);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin XMLKeyGenDlg member variables
    wxStaticText* mIdColumnCapt;
    wxOwnerDrawnComboBox* mIdColumn;
    wxStaticText* mExprCapt;
    wxStaticText* mExprCapt2;
    wxTextCtrl* mExpr;
    wxRadioBox* mType;
////@end XMLKeyGenDlg member variables
    XMLElementDlg * owning_dlg;
};

#endif
    // _XMLKEYGENDLG_H_
