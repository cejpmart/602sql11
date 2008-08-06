/////////////////////////////////////////////////////////////////////////////
// Name:        SearchObjectsDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/15/04 10:05:47
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SEARCHOBJECTSDLG_H_
#define _SEARCHOBJECTSDLG_H_

#ifdef __GNUG__
//#pragma interface "SearchObjectsDlg.cpp"
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
#define SYMBOL_SEARCHOBJECTSDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_SEARCHOBJECTSDLG_TITLE _("Find in Objects")
#define SYMBOL_SEARCHOBJECTSDLG_IDNAME ID_DIALOG
#define SYMBOL_SEARCHOBJECTSDLG_SIZE wxSize(400, 300)
#define SYMBOL_SEARCHOBJECTSDLG_POSITION wxDefaultPosition
#define CD_SRCH_CAPT 10001
#define CD_SRCH_PATTERN 10002
#define CD_SRCH_WORDS 10003
#define CD_SRCH_SENSIT 10004
#define CD_SRCH_DETAIL 10008
#define CD_SRCH_ALL 10005
#define CD_SRCH_SELECTED 10006
#define CD_SRCH_TABLE 10009
#define CD_SRCH_QUERY 10010
#define CD_SRCH_PROG 10015
#define CD_SRCG_DIAGR 10017
#define CD_SRCH_PROC 10012
#define CD_SRCH_TRIGGER 10013
#define CD_SRCH_SEQ 10014
#define CD_SRCH_DOMAIN 10011
#define CD_SRCH_TIME 10016
#define CD_SRCH_OPENSELECTOR 10007
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SearchObjectsDlg class declaration
 */

class SearchObjectsDlg: public wxDialog
{    
    DECLARE_CLASS( SearchObjectsDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    SearchObjectsDlg(cdp_t cdpIn, wxString pattIn, const char * schema_nameIn, const char * folder_nameIn);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_SEARCHOBJECTSDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin SearchObjectsDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SRCH_DETAIL
    void OnCdSrchDetailClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_SRCH_ALL
    void OnCdSrchAllSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_SRCH_SELECTED
    void OnCdSrchSelectedSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SRCH_OPENSELECTOR
    void OnCdSrchOpenselectorClick( wxCommandEvent& event );

////@end SearchObjectsDlg event handler declarations

////@begin SearchObjectsDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end SearchObjectsDlg member function declarations
  void show_details(bool show);
  void enable_categs(bool enable);
  void search(wxString pattern);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SearchObjectsDlg member variables
    wxOwnerDrawnComboBox* srch_pattern;
    wxCheckBox* match_word;
    wxCheckBox* match_case;
    wxButton* mDetails;
    wxStaticBox* static1;
    wxRadioButton* all_categs;
    wxRadioButton* sel_categs;
    wxCheckBox* tables;
    wxCheckBox* queries;
    wxCheckBox* programs;
    wxCheckBox* diagrams;
    wxCheckBox* procedures;
    wxCheckBox* triggers;
    wxCheckBox* sequences;
    wxCheckBox* domains;
    wxStaticBox* static2;
    wxTextCtrl* srch_time;
    wxBitmapButton* mDateSelector;
////@end SearchObjectsDlg member variables
  wxStaticBoxSizer* statsize1;
  wxStaticBoxSizer* statsize2;
  cdp_t cdp;
  wxString patt;  // search pattern
  bool in_schema;
  bool in_folder;
  tobjname folder_name;  // defined when in_folder==true
  tobjname search_schema;  // defined when in_schema==true
  static bool selected_categs, sel_table, sel_query, sel_program, sel_trigger, sel_proc, sel_domain, sel_seq, sel_diagr;
  static bool matching_case, matching_word;
  static string_history object_search_history;
  static uns32 modif_limit;
};

#endif
    // _SEARCHOBJECTSDLG_H_
