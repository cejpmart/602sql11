/////////////////////////////////////////////////////////////////////////////
// Name:        XMLGlobalDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/25/04 14:30:50
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLGLOBALDLG_H_
#define _XMLGLOBALDLG_H_

#ifdef __GNUG__
//#pragma interface "XMLGlobalDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxOwnerDrawnComboBox;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_XMLGLOBALDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_XMLGLOBALDLG_TITLE _("XML global properties")
#define SYMBOL_XMLGLOBALDLG_IDNAME ID_DIALOG
#define SYMBOL_XMLGLOBALDLG_SIZE wxDefaultSize
#define SYMBOL_XMLGLOBALDLG_POSITION wxDefaultPosition
#define ID_NOTEBOOK 10001
#define ID_PANEL 10002
#define CD_XMPTOP_QUERY 10003
#define CD_XMLTOP_DATA_SOURCE 10112
#define ID_PANEL1 10004
#define CD_XMPTOP_SELENC 10005
#define CD_XMPTOP_PROLOG 10006
#define CD_XMPTOP_DOCTYPE 10007
#define CD_XMPTOP_SS_C 10013
#define CD_XMPTOP_STYLESHEET 10123
#define CD_XMLTOP_EMIT_WHITESPACES 10074
#define CD_XMLTOP_DISABLE_OPT 10110
#define ID_PANEL2 10009
#define CD_TOPXML_NOVALID 10011
#define CD_XMLTOP_IGNORETEXT 10010
#define CD_XMPTOP_IGNOREELEMS 10012
#define ID_PANEL4 10111
#define CD_NMSP_PREFIX 10113
#define CD_NMSP_NAME 10114
#define ID_PANEL5 10008
#define CD_XMLTOP_DAD2 10015
#define CD_XMLTOP_FORM2 10125
#define CD_XMLTOP_OK_MESSAGE 10126
#define ID_PANEL6 10014
#define CD_XMLTOP_FAIL_MESSAGE 10124
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * XMLGlobalDlg class declaration
 */

class XMLGlobalDlg: public wxDialog
{    
    DECLARE_CLASS( XMLGlobalDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    XMLGlobalDlg(xml_designer * dadeIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("XML global properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin XMLGlobalDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMPTOP_SELENC
    void OnCdXmptopSelencClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end XMLGlobalDlg event handler declarations
  void OnEncoding(wxCommandEvent & event);

////@begin XMLGlobalDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end XMLGlobalDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin XMLGlobalDlg member variables
    wxTextCtrl* mQuery;
    wxBoxSizer* mSpacer10_2;
    wxOwnerDrawnComboBox* mDataSource;
    wxTextCtrl* mProlog;
    wxTextCtrl* mDoctype;
    wxOwnerDrawnComboBox* mStyleSheet;
    wxCheckBox* mEmitWhitespaces;
    wxBoxSizer* mSpacer10;
    wxCheckBox* mDisableOpt;
    wxCheckBox* mNoValidation;
    wxCheckBox* mIgnoreText;
    wxCheckBox* mIgnoreElements;
    wxTextCtrl* mNmspPrefix;
    wxTextCtrl* mNmspName;
    wxOwnerDrawnComboBox* mDAD2;
    wxOwnerDrawnComboBox* mForm2;
    wxTextCtrl* mOKMessage;
    wxTextCtrl* mFailMessage;
////@end XMLGlobalDlg member variables
  xml_designer * dade;
  cdp_t cdp;
  t_dad_root * dad_top;
};

#endif
    // _XMLGLOBALDLG_H_
