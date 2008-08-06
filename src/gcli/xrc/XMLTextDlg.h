/////////////////////////////////////////////////////////////////////////////
// Name:        XMLTextDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/25/04 13:54:03
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLTEXTDLG_H_
#define _XMLTEXTDLG_H_

#ifdef __GNUG__
//#pragma interface "XMLTextDlg.cpp"
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
#define SYMBOL_XMLTEXTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_XMLTEXTDLG_TITLE _("XML text properties")
#define SYMBOL_XMLTEXTDLG_IDNAME ID_DIALOG
#define SYMBOL_XMLTEXTDLG_SIZE wxSize(400, 300)
#define SYMBOL_XMLTEXTDLG_POSITION wxDefaultPosition
#define ID_RADIOBUTTONb 10003
#define ID_STATICTEXT 10001
#define ID_COMBOBOXb 10006
#define ID_STATICTEXT1 10002
#define ID_COMBOBOX1b 10007
#define CD_XMLCOL_UPDATE_KEY 10013
#define ID_RADIOBUTTON1b 10004
#define wxID_STATICb 10010
#define ID_TEXTCTRL4b 10008
#define ID_RADIOBUTTON2b 10005
#define ID_TEXTCTRL5b 10009
#define CD_XMLCOL_FORMAT_CAPT 10011
#define CD_XMLCOL_FORMAT 10010
#define CD_XMLCOL_TRANS 10012
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * XMLTextDlg class declaration
 */

class XMLTextDlg: public wxDialog
{    
    DECLARE_CLASS( XMLTextDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    XMLTextDlg(xml_designer * dadeIn, t_dad_col_name column_nameIn, dad_table_name table_nameIn, t_link_types link_typeIn, 
      wxString formatIn, wxString fixed_valIn, tobjname inconv_fncIn, tobjname outconv_fncIn, bool updkeyIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("XML text properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin XMLTextDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTONb
    void OnRadiobuttonbSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOXb
    void OnComboboxbSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON1b
    void OnRadiobutton1bSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON2b
    void OnRadiobutton2bSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLCOL_TRANS
    void OnCdXmlcolTransClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end XMLTextDlg event handler declarations

////@begin XMLTextDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end XMLTextDlg member function declarations
  void enabling(void);
  void link_show(void);
  void link_store(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin XMLTextDlg member variables
    wxRadioButton* mAssocColumn;
    wxStaticText* mTableC;
    wxOwnerDrawnComboBox* mTable;
    wxStaticText* mColumnC;
    wxOwnerDrawnComboBox* mColumn;
    wxCheckBox* mUpdateKey;
    wxRadioButton* mAssocVariable;
    wxStaticText* mVariableC;
    wxTextCtrl* mVariable;
    wxRadioButton* mAssocConstant;
    wxTextCtrl* mConstant;
    wxStaticText* mFormatCapt;
    wxTextCtrl* mFormat;
////@end XMLTextDlg member variables
  xml_designer * dade;
  cdp_t cdp;
 // link:
  t_dad_col_name column_name;  // or var name or constant
  dad_table_name table_name;   // valid for link_type==LT_COLUMN, empty for synthetic DAD
  t_link_types link_type;
  wxString format, fixed_val;
  tobjname inconv_fnc, outconv_fnc;
  bool updkey;
};

#endif
    // _XMLTEXTDLG_H_
