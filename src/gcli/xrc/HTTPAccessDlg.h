/////////////////////////////////////////////////////////////////////////////
// Name:        HTTPAccessDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/27/06 12:33:45
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _HTTPACCESSDLG_H_
#define _HTTPACCESSDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "HTTPAccessDlg.cpp"
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
class wxBoxSizer;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_MYDIALOG3 10091
#define SYMBOL_HTTPACCESSDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_HTTPACCESSDLG_TITLE _("Web XML Interface")
#define SYMBOL_HTTPACCESSDLG_IDNAME ID_MYDIALOG3
#define SYMBOL_HTTPACCESSDLG_SIZE wxSize(400, 300)
#define SYMBOL_HTTPACCESSDLG_POSITION wxDefaultPosition
#define CD_HOST_NAME_LABEL 10094
#define CD_HTTP_HOSTNAME 10092
#define CD_HTTP_SQLHOST 10099
#define CD_HTTP_STANDALONE 10003
#define CD_HTTP_EMULATED 10004
#define CD_HTTP_START_EMULATION 10093
#define CD_HTTP_PORT 10005
#define CD_HTTP_PATH 10095
#define CD_HTTP_URL 10098
#define CD_HTTP_ANONYMOUS 10000
#define CD_HTTP_AUTHENTIFICATE 10001
#define CD_HTTP_PASSWORD 10002
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
 * HTTPAccessDlg class declaration
 */

class HTTPAccessDlg: public wxDialog
{    
    DECLARE_CLASS( HTTPAccessDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    HTTPAccessDlg(cdp_t cdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_HTTPACCESSDLG_IDNAME, const wxString& caption = SYMBOL_HTTPACCESSDLG_TITLE, const wxPoint& pos = SYMBOL_HTTPACCESSDLG_POSITION, const wxSize& size = SYMBOL_HTTPACCESSDLG_SIZE, long style = SYMBOL_HTTPACCESSDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin HTTPAccessDlg event handler declarations

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_HTTP_HOSTNAME
    void OnCdHttpHostnameUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_HTTP_SQLHOST
    void OnCdHttpSqlhostClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_HTTP_STANDALONE
    void OnCdHttpStandaloneSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_HTTP_EMULATED
    void OnCdHttpEmulatedSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_HTTP_START_EMULATION
    void OnCdHttpStartEmulationClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_HTTP_PORT
    void OnCdHttpPortUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_HTTP_PATH
    void OnCdHttpPathUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_HTTP_ANONYMOUS
    void OnCdHttpAnonymousSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_HTTP_AUTHENTIFICATE
    void OnCdHttpAuthentificateSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_HTTP_PASSWORD
    void OnCdHttpPasswordClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_APPLY
    void OnApplyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end HTTPAccessDlg event handler declarations
    void OnClose(wxCloseEvent & event);

////@begin HTTPAccessDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end HTTPAccessDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();
    wxString get_normalized_path(void);
    wxString get_host_info(void);
    wxString make_URL_pattern(void);
    void show_url(void);
    bool verify(void);
    bool save(void);
    void emulation_mode(bool emulating);

////@begin HTTPAccessDlg member variables
    wxBoxSizer* mHostSizer;
    wxStaticText* mHostNameLabel;
    wxTextCtrl* mHostName;
    wxButton* mSQLHost;
    wxRadioButton* mStandalone;
    wxRadioButton* mEmulated;
    wxButton* mStartEmulation;
    wxTextCtrl* mPort;
    wxStaticBoxSizer* mApache;
    wxTextCtrl* mPath;
    wxStaticText* mURL;
    wxRadioButton* mAnonymous;
    wxRadioButton* mAuthentificate;
    wxButton* mPasswords;
    wxButton* mOK;
    wxButton* mApply;
////@end HTTPAccessDlg member variables
    cdp_t cdp;
};

#endif
    // _HTTPACCESSDLG_H_
