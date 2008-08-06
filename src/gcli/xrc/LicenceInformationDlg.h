/////////////////////////////////////////////////////////////////////////////
// Name:        LicenceInformationDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     10/04/04 15:16:59
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _LICENCEINFORMATIONDLG_H_
#define _LICENCEINFORMATIONDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "LicenceInformationDlg.cpp"
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
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_LICENCEINFORMATIONDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_LICENCEINFORMATIONDLG_TITLE _("Licence Information")
#define SYMBOL_LICENCEINFORMATIONDLG_IDNAME ID_DIALOG
#define SYMBOL_LICENCEINFORMATIONDLG_SIZE wxDefaultSize
#define SYMBOL_LICENCEINFORMATIONDLG_POSITION wxDefaultPosition
#define CD_LIC_IK 10003
#define CD_LIC_LC 10004
#define CD_LIC_TYPE 10005
#define CD_LIC_INFO 10006
#define CD_LIC_START 10007
#define CD_LIC_SHOW 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * LicenceInformationDlg class declaration
 */

class LicenceInformationDlg: public wxDialog
{    
    DECLARE_CLASS( LicenceInformationDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    LicenceInformationDlg(cdp_t cdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_LICENCEINFORMATIONDLG_IDNAME, const wxString& caption = SYMBOL_LICENCEINFORMATIONDLG_TITLE, const wxPoint& pos = SYMBOL_LICENCEINFORMATIONDLG_POSITION, const wxSize& size = SYMBOL_LICENCEINFORMATIONDLG_SIZE, long style = SYMBOL_LICENCEINFORMATIONDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin LicenceInformationDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_START
    void OnCdLicStartClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_SHOW
    void OnCdLicShowClick( wxCommandEvent& event );

////@end LicenceInformationDlg event handler declarations

////@begin LicenceInformationDlg member function declarations

////@end LicenceInformationDlg member function declarations
  void show_info(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin LicenceInformationDlg member variables
    wxTextCtrl* mIK;
    wxTextCtrl* mLC;
    wxTextCtrl* mType;
    wxTextCtrl* mInfo;
    wxButton* mStart;
////@end LicenceInformationDlg member variables
  cdp_t cdp;
  bool network_access_effective;  // defined in show_info()
};

#endif
    // _LICENCEINFORMATIONDLG_H_
