/////////////////////////////////////////////////////////////////////////////
// Name:        ServerManagerPanel.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/21/07 15:29:45
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SERVERMANAGERPANEL_H_
#define _SERVERMANAGERPANEL_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ServerManagerPanel.cpp"
#endif

#include <wx/odcombo.h>
class ServersOwnerDrawnComboBox : public wxOwnerDrawnComboBox
{public:
  ServersOwnerDrawnComboBox(wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, int n = 0, const wxString choices[] = NULL, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxT("comboBox"))
   : wxOwnerDrawnComboBox(parent, id, value, pos, size, n, choices, style, validator, name) { }
  void OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const;
};

int get_server_list_index_from_combo_selection(const ServersOwnerDrawnComboBox * combo, int sel);
/*!
 * Includes
 */

////@begin includes
#include "wx/spinctrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ServersOwnerDrawnComboBox;
class wxSpinCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_SRVMNG_DLG 10000
#define SYMBOL_SERVERMANAGERPANEL_STYLE 0
#define SYMBOL_SERVERMANAGERPANEL_TITLE _T("")
#define SYMBOL_SERVERMANAGERPANEL_IDNAME ID_SRVMNG_DLG
#define SYMBOL_SERVERMANAGERPANEL_SIZE wxSize(400, 300)
#define SYMBOL_SERVERMANAGERPANEL_POSITION wxDefaultPosition
#define CD_SRVMNG_SERVERS 10001
#define CD_SRVMNG_DAEMON 10005
#define CD_SRVMNG_PASS 10015
#define CD_SRVMNG_STARTTASK 10004
#define CD_SRVMNG_STOP 10006
#define CD_SRVMNG_INFO1 10007
#define CD_SRVMNG_INFO2 10008
#define CD_SRVMNG_INFO3 10019
#define CD_SRVMNG_INFO4 10020
#define CD_SRVMNG_INFO5 10021
#define CD_SRVMNG_REFRESH 10002
#define CD_SRVMNG_SPIN 10014
#define CD_SRVMNG_LOG 10003
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
 * ServerManagerPanel class declaration
 */

class ServerManagerPanel: public wxPanel
{    
    DECLARE_DYNAMIC_CLASS( ServerManagerPanel )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ServerManagerPanel( );
    ServerManagerPanel( wxWindow* parent, wxWindowID id = SYMBOL_SERVERMANAGERPANEL_IDNAME, const wxPoint& pos = SYMBOL_SERVERMANAGERPANEL_POSITION, const wxSize& size = SYMBOL_SERVERMANAGERPANEL_SIZE, long style = SYMBOL_SERVERMANAGERPANEL_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SERVERMANAGERPANEL_IDNAME, const wxPoint& pos = SYMBOL_SERVERMANAGERPANEL_POSITION, const wxSize& size = SYMBOL_SERVERMANAGERPANEL_SIZE, long style = SYMBOL_SERVERMANAGERPANEL_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ServerManagerPanel event handler declarations

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_SRVMNG_SERVERS
    void OnCdSrvmngServersSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SRVMNG_STARTTASK
    void OnCdSrvmngStarttaskClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SRVMNG_STOP
    void OnCdSrvmngStopClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    void OnCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SRVMNG_REFRESH
    void OnCdSrvmngRefreshClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for CD_SRVMNG_SPIN
    void OnCdSrvmngSpinUpdated( wxSpinEvent& event );

////@end ServerManagerPanel event handler declarations

////@begin ServerManagerPanel member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ServerManagerPanel member function declarations
    void OnTimer(wxTimerEvent& event);
    void refresh_server_list(void);
    void show_selected_server_info(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ServerManagerPanel member variables
    ServersOwnerDrawnComboBox* mServers;
    wxCheckBox* mDaemon;
    wxCheckBox* mPass;
    wxButton* mStartTask;
    wxButton* mStop;
    wxStaticText* mInfo1;
    wxStaticText* mInfo2;
    wxStaticText* mInfo3;
    wxStaticText* mInfo4;
    wxStaticText* mInfo5;
    wxButton* mRefresh;
    wxSpinCtrl* mSpin;
    wxListBox* mLog;
////@end ServerManagerPanel member variables
    wxTimer refr_timer;      
    int curr_clients;
};

#endif
    // _SERVERMANAGERPANEL_H_
