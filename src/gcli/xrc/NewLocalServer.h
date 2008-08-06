/////////////////////////////////////////////////////////////////////////////
// Name:        NewLocalServer.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/12/04 16:14:16
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NEWLOCALSERVER_H_
#define _NEWLOCALSERVER_H_

#ifdef __GNUG__
//#pragma interface "NewLocalServer.cpp"
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
#define SYMBOL_NEWLOCALSERVER_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_NEWLOCALSERVER_TITLE _("Create a new local server")
#define SYMBOL_NEWLOCALSERVER_IDNAME ID_DIALOG
#define SYMBOL_NEWLOCALSERVER_SIZE wxSize(400, 300)
#define SYMBOL_NEWLOCALSERVER_POSITION wxDefaultPosition
#define CD_REGLOC_SERVER_NAME 10001
#define CD_REGLOC_PATH 10002
#define CD_REGLOC_SELPATH 10003
#define ID_REGLOC_CHARSET 10007
#define CD_REGLOC_BIG 10179
#define CD_REGLOC_DEFPORT 10004
#define CD_REGLOC_SPECPORT 10005
#define CD_REGLOC_PORT 10006
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NewLocalServer class declaration
 */

class NewLocalServer: public wxDialog
{    
    DECLARE_CLASS( NewLocalServer )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NewLocalServer( );
    NewLocalServer( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Create a new local server"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_NEWLOCALSERVER_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Create a new local server"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_NEWLOCALSERVER_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NewLocalServer event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_REGLOC_SELPATH
    void OnCdReglocSelpathClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_REGLOC_DEFPORT
    void OnCdReglocDefportSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_REGLOC_SPECPORT
    void OnCdReglocSpecportSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end NewLocalServer event handler declarations

////@begin NewLocalServer member function declarations

    wxString GetServerName() const { return server_name ; }
    void SetServerName(wxString value) { server_name = value ; }

    wxString GetPath() const { return path ; }
    void SetPath(wxString value) { path = value ; }

    bool GetDefaultPort() const { return default_port ; }
    void SetDefaultPort(bool value) { default_port = value ; }

    long GetPortNumber() const { return port_number ; }
    void SetPortNumber(long value) { port_number = value ; }

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NewLocalServer member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NewLocalServer member variables
    wxTextCtrl* mServerName;
    wxTextCtrl* mPath;
    wxOwnerDrawnComboBox* mCharset;
    wxCheckBox* mBig;
    wxRadioButton* mDefPort;
    wxRadioButton* mSpecPort;
    wxTextCtrl* mPort;
    wxString server_name;
    wxString path;
    bool default_port;
    long port_number;
////@end NewLocalServer member variables
};

#endif
    // _NEWLOCALSERVER_H_
