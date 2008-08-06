/////////////////////////////////////////////////////////////////////////////
// Name:        XMLAttributeDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/25/04 11:30:02
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLATTRIBUTEDLG_H_
#define _XMLATTRIBUTEDLG_H_

#ifdef __GNUG__
//#pragma interface "XMLAttributeDlg.cpp"
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
#define SYMBOL_XMLATTRIBUTEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_XMLATTRIBUTEDLG_TITLE _("XML attribute properties")
#define SYMBOL_XMLATTRIBUTEDLG_IDNAME ID_DIALOG
#define SYMBOL_XMLATTRIBUTEDLG_SIZE wxSize(400, 300)
#define SYMBOL_XMLATTRIBUTEDLG_POSITION wxDefaultPosition
#define ID_TEXTCTRLa 10001
#define ID_TEXTCTRL1a 10002
#define ID_RADIOBUTTON 10003
#define ID_COMBOBOXa 10006
#define ID_COMBOBOX1a 10007
#define CD_XMLCOL_UPDATE_KEY 10013
#define ID_RADIOBUTTON1 10004
#define ID_TEXTCTRL4 10008
#define ID_RADIOBUTTON2 10005
#define ID_TEXTCTRL5 10009
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
 * XMLAttributeDlg class declaration
 */

class XMLAttributeDlg: public wxDialog
{    
    DECLARE_CLASS( XMLAttributeDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    XMLAttributeDlg(xml_designer * dadeIn, t_dad_attribute_name attr_nameIn, wxString attrcondIn, wxString formatIn, wxString fixed_valIn,
                    t_dad_col_name column_nameIn, dad_table_name table_nameIn, t_link_types link_typeIn,
                    tobjname inconv_fncIn, tobjname outconv_fncIn, bool updkeyIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("XML attribute properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin XMLAttributeDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON
    void OnRadiobuttonSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOXa
    void OnComboboxaSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON1
    void OnRadiobutton1Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON2
    void OnRadiobutton2Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLCOL_TRANS
    void OnCdXmlcolTransClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end XMLAttributeDlg event handler declarations

////@begin XMLAttributeDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end XMLAttributeDlg member function declarations
  void enabling(void);
  void link_show(void);
  void link_store(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin XMLAttributeDlg member variables
    wxTextCtrl* mAttrName;
    wxTextCtrl* mAttrCond;
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
////@end XMLAttributeDlg member variables
  xml_designer * dade;
  cdp_t cdp;
 // attribute:
  t_dad_attribute_name attr_name;
  wxString attrcond;
  wxString format, fixed_val;
 // link:
  t_dad_col_name column_name;  // or var name or constant
  dad_table_name table_name;   // valid for link_type==LT_COLUMN, empty for synthetic DAD
  t_link_types link_type;
  tobjname inconv_fnc, outconv_fnc;
  bool updkey;
};

#endif
    // _XMLATTRIBUTEDLG_H_
