/////////////////////////////////////////////////////////////////////////////
// Name:        ServerCertificateDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/03/04 17:45:42
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SERVERCERTIFICATEDLG_H_
#define _SERVERCERTIFICATEDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "ServerCertificateDlg.cpp"
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
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_SERVERCERTIFICATEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_SERVERCERTIFICATEDLG_TITLE _("Server Certificate")
#define SYMBOL_SERVERCERTIFICATEDLG_IDNAME ID_DIALOG
#define SYMBOL_SERVERCERTIFICATEDLG_SIZE wxDefaultSize
#define SYMBOL_SERVERCERTIFICATEDLG_POSITION wxDefaultPosition
#define CD_CERT_STATE 10010
#define ID_NOTEBOOK 10001
#define ID_PANEL 10002
#define CD_CERT_START 10004
#define CD_CERT_CANCEL 10005
#define CD_CERT_FIELD 10006
#define CD_CERT_GAUGE 10007
#define ID_PANEL1 10003
#define CD_CERT_IMPCERT 10008
#define CD_CERT_IMPKEY 10009
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ServerCertificateDlg class declaration
 */

#define RANDOM_DATA_SPACE 1000

class ServerCertificateDlg: public wxDialog
{    
    DECLARE_CLASS( ServerCertificateDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ServerCertificateDlg(cdp_t cdp);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SERVERCERTIFICATEDLG_IDNAME, const wxString& caption = SYMBOL_SERVERCERTIFICATEDLG_TITLE, const wxPoint& pos = SYMBOL_SERVERCERTIFICATEDLG_POSITION, const wxSize& size = SYMBOL_SERVERCERTIFICATEDLG_SIZE, long style = SYMBOL_SERVERCERTIFICATEDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ServerCertificateDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_CERT_START
    void OnCdCertStartClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_CERT_CANCEL
    void OnCdCertCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_CERT_IMPCERT
    void OnCdCertImpcertClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_CERT_IMPKEY
    void OnCdCertImpkeyClick( wxCommandEvent& event );

////@end ServerCertificateDlg event handler declarations

////@begin ServerCertificateDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ServerCertificateDlg member function declarations
  void state_info(void);
  void make_key_pair(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ServerCertificateDlg member variables
    wxStaticText* mState;
    wxButton* mStart;
    wxButton* mCancel;
    wxTextCtrl* mField;
    wxGauge* mGauge;
    wxButton* mImpCert;
    wxButton* mImpKey;
////@end ServerCertificateDlg member variables
    cdp_t cdp;
    unsigned char random_data[RANDOM_DATA_SPACE];
    bool accumulating;
    int accumulated;
};

#endif
    // _SERVERCERTIFICATEDLG_H_
