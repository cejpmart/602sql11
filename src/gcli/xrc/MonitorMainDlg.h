/////////////////////////////////////////////////////////////////////////////
// Name:        MonitorMainDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/28/04 15:42:38
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _MONITORMAINDLG_H_
#define _MONITORMAINDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "MonitorMainDlg.cpp"
#endif

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
class wxOwnerDrawnComboBox;
class wxSpinCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_MONITORMAINDLG_STYLE wxCAPTION
#define SYMBOL_MONITORMAINDLG_TITLE _T("")
#define SYMBOL_MONITORMAINDLG_IDNAME ID_DIALOG
#define SYMBOL_MONITORMAINDLG_SIZE wxSize(400, 300)
#define SYMBOL_MONITORMAINDLG_POSITION wxDefaultPosition
#define CD_MONIT_SERVER 10009
#define CD_MONIT_ACCESS 10004
#define CD_MONIT_BACKGROUND 10005
#define CD_MONIT_USER 10002
#define CD_MONIT_SECURADM 10007
#define CD_MONIT_CONFADM 10006
#define CD_MONIT_DATAADM 10008
#define CD_MONIT_REFRESH 10003
#define CD_MONIT_REFRESH_NOW 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MonitorMainDlg class declaration
 */

class MonitorMainDlg: public wxScrolledWindow
{    
    DECLARE_CLASS( MonitorMainDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MonitorMainDlg( );
    MonitorMainDlg( wxWindow* parent, wxWindowID id = SYMBOL_MONITORMAINDLG_IDNAME, const wxString& caption = SYMBOL_MONITORMAINDLG_TITLE, const wxPoint& pos = SYMBOL_MONITORMAINDLG_POSITION, const wxSize& size = SYMBOL_MONITORMAINDLG_SIZE, long style = SYMBOL_MONITORMAINDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_MONITORMAINDLG_IDNAME, const wxString& caption = SYMBOL_MONITORMAINDLG_TITLE, const wxPoint& pos = SYMBOL_MONITORMAINDLG_POSITION, const wxSize& size = SYMBOL_MONITORMAINDLG_SIZE, long style = SYMBOL_MONITORMAINDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MonitorMainDlg event handler declarations

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_MONIT_SERVER
    void OnCdMonitServerSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_MONIT_ACCESS
    void OnCdMonitAccessClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_MONIT_BACKGROUND
    void OnCdMonitBackgroundClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for CD_MONIT_REFRESH
    void OnCdMonitRefreshUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_MONIT_REFRESH
    void OnCdMonitRefreshTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_MONIT_REFRESH_NOW
    void OnCdMonitRefreshNowClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end MonitorMainDlg event handler declarations
    void OnRefresh(wxCommandEvent & event);

////@begin MonitorMainDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end MonitorMainDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MonitorMainDlg member variables
    wxOwnerDrawnComboBox* mServer;
    wxCheckBox* mAccess;
    wxCheckBox* mBackground;
    wxTextCtrl* mUser;
    wxStaticText* mSecurAdm;
    wxStaticText* mConfAdm;
    wxStaticText* mDataAdm;
    wxSpinCtrl* mRefresh;
    wxButton* mRefreshNow;
////@end MonitorMainDlg member variables
};

#endif
    // _MONITORMAINDLG_H_
