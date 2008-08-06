/////////////////////////////////////////////////////////////////////////////
// Name:        XMLTranslationDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/25/05 10:14:57
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLTRANSLATIONDLG_H_
#define _XMLTRANSLATIONDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "XMLTranslationDlg.cpp"
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
#define SYMBOL_XMLTRANSLATIONDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_XMLTRANSLATIONDLG_TITLE _("Translation functions")
#define SYMBOL_XMLTRANSLATIONDLG_IDNAME ID_DIALOG
#define SYMBOL_XMLTRANSLATIONDLG_SIZE wxSize(400, 300)
#define SYMBOL_XMLTRANSLATIONDLG_POSITION wxDefaultPosition
#define CD_XMLTRANS_IMP 10001
#define CD_XMLTRANS_IMP_NEW 10002
#define CD_XMLTRANS_EXP 10003
#define CD_XMLTRANS_EXP_NEW 10004
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * XMLTranslationDlg class declaration
 */

class XMLTranslationDlg: public wxDialog
{    
    DECLARE_CLASS( XMLTranslationDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    XMLTranslationDlg(cdp_t cdpIn, char * inconv_fncIn, char * outconv_fncIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_XMLTRANSLATIONDLG_IDNAME, const wxString& caption = SYMBOL_XMLTRANSLATIONDLG_TITLE, const wxPoint& pos = SYMBOL_XMLTRANSLATIONDLG_POSITION, const wxSize& size = SYMBOL_XMLTRANSLATIONDLG_SIZE, long style = SYMBOL_XMLTRANSLATIONDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin XMLTranslationDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLTRANS_IMP_NEW
    void OnCdXmltransImpNewClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLTRANS_EXP_NEW
    void OnCdXmltransExpNewClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end XMLTranslationDlg event handler declarations

////@begin XMLTranslationDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end XMLTranslationDlg member function declarations
    void fill(void);
    wxString create_new_function(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin XMLTranslationDlg member variables
    wxOwnerDrawnComboBox* mImpFnc;
    wxButton* mImpNew;
    wxOwnerDrawnComboBox* mExpFnc;
    wxButton* mExpNew;
////@end XMLTranslationDlg member variables
    cdp_t cdp;
    char * inconv_fnc;
    char * outconv_fnc;
};

#endif
    // _XMLTRANSLATIONDLG_H_
