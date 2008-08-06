/////////////////////////////////////////////////////////////////////////////
// Name:        TablePropertiesDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/08/04 07:35:41
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _TABLEPROPERTIESDLG_H_
#define _TABLEPROPERTIESDLG_H_

#ifdef __GNUG__
//#pragma interface "TablePropertiesDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/valgen.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_TABLEPROPERTIESDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_TABLEPROPERTIESDLG_TITLE _("Table properties")
#define SYMBOL_TABLEPROPERTIESDLG_IDNAME ID_DIALOG
#define SYMBOL_TABLEPROPERTIESDLG_SIZE wxSize(400, 300)
#define SYMBOL_TABLEPROPERTIESDLG_POSITION wxDefaultPosition
#define CD_TFL_REC_PRIVILS 10001
#define CD_TFL_ZCR 10003
#define CD_TFL_UNIKEY 10004
#define CD_TFL_LUO 10005
#define CD_TFL_DETECT 10006
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * TablePropertiesDlg class declaration
 */

class TablePropertiesDlg: public wxDialog
{    
    DECLARE_CLASS( TablePropertiesDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    TablePropertiesDlg( );
    TablePropertiesDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Table properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_TABLEPROPERTIESDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Table properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_TABLEPROPERTIESDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin TablePropertiesDlg event handler declarations

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TFL_ZCR
    void OnCdTflZcrClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TFL_LUO
    void OnCdTflLuoClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TFL_DETECT
    void OnCdTflDetectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end TablePropertiesDlg event handler declarations

////@begin TablePropertiesDlg member function declarations

    bool GetPrivils() const { return rec_privils ; }
    void SetPrivils(bool value) { rec_privils = value ; }

    bool GetJournal() const { return journal ; }
    void SetJournal(bool value) { journal = value ; }

    bool GetZcr() const { return zcr ; }
    void SetZcr(bool value) { zcr = value ; }

    bool GetUnikey() const { return unikey ; }
    void SetUnikey(bool value) { unikey = value ; }

    bool GetLuo() const { return luo ; }
    void SetLuo(bool value) { luo = value ; }

    bool GetDetect() const { return detect ; }
    void SetDetect(bool value) { detect = value ; }

////@end TablePropertiesDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin TablePropertiesDlg member variables
    wxBoxSizer* mTopSizer;
    wxBoxSizer* mReplSizer;
    wxCheckBox* mRepl;
    wxStaticBox* mReplGroup;
    bool rec_privils;
    bool journal;
    bool zcr;
    bool unikey;
    bool luo;
    bool detect;
////@end TablePropertiesDlg member variables
};

#endif
    // _TABLEPROPERTIESDLG_H_
