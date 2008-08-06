/////////////////////////////////////////////////////////////////////////////
// Name:        XMLStartDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/24/04 18:01:22
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLSTARTDLG_H_
#define _XMLSTARTDLG_H_

#ifdef __GNUG__
#pragma interface "XMLStartDlg.cpp"
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
#define ID_DIALOG 10000
#define SYMBOL_XMLSTARTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_XMLSTARTDLG_TITLE _("Create initial design")
#define SYMBOL_XMLSTARTDLG_IDNAME ID_DIALOG
#define SYMBOL_XMLSTARTDLG_SIZE wxSize(400, 300)
#define SYMBOL_XMLSTARTDLG_POSITION wxDefaultPosition
#define CD_XMLSTART_TABLE 10001
#define ID_COMBOBOX 10004
#define ID_TEXTCTRL 10005
#define CD_XMLSTART_QUERY 10002
#define ID_TEXTCTRL1 10006
#define CD_XMLSTART_EMPTY 10003
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * XMLStartDlg class declaration
 */

class XMLStartDlg: public wxDialog
{    
    DECLARE_CLASS( XMLStartDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    XMLStartDlg(cdp_t cdpIn);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Create initial design"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin XMLStartDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_TABLE
    void OnCdXmlstartTableSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_QUERY
    void OnCdXmlstartQuerySelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_EMPTY
    void OnCdXmlstartEmptySelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end XMLStartDlg event handler declarations

////@begin XMLStartDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end XMLStartDlg member function declarations
  void enabling(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin XMLStartDlg member variables
    wxRadioButton* mByTable;
    wxStaticText* mTableNameC;
    wxOwnerDrawnComboBox* mTableName;
    wxStaticText* mTableAliasC;
    wxTextCtrl* mTableAlias;
    wxRadioButton* mByQuery;
    wxTextCtrl* mQuery;
    wxRadioButton* mByEmpty;
////@end XMLStartDlg member variables
  cdp_t cdp;
  tobjname table_name, table_alias;
  wxString query;
  d_table * new_td;
};

#endif
    // _XMLSTARTDLG_H_
