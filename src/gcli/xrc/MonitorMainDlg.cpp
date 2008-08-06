/////////////////////////////////////////////////////////////////////////////
// Name:        MonitorMainDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/28/04 15:42:38
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "MonitorMainDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
////@end includes

#include "MonitorMainDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * MonitorMainDlg type definition
 */

IMPLEMENT_CLASS( MonitorMainDlg, wxScrolledWindow )

/*!
 * MonitorMainDlg event table definition
 */

BEGIN_EVENT_TABLE( MonitorMainDlg, wxScrolledWindow )

////@begin MonitorMainDlg event table entries
    EVT_COMBOBOX( CD_MONIT_SERVER, MonitorMainDlg::OnCdMonitServerSelected )

    EVT_CHECKBOX( CD_MONIT_ACCESS, MonitorMainDlg::OnCdMonitAccessClick )

    EVT_CHECKBOX( CD_MONIT_BACKGROUND, MonitorMainDlg::OnCdMonitBackgroundClick )

    EVT_SPINCTRL( CD_MONIT_REFRESH, MonitorMainDlg::OnCdMonitRefreshUpdated )
    EVT_TEXT( CD_MONIT_REFRESH, MonitorMainDlg::OnCdMonitRefreshTextUpdated )

    EVT_BUTTON( CD_MONIT_REFRESH_NOW, MonitorMainDlg::OnCdMonitRefreshNowClick )

    EVT_BUTTON( wxID_HELP, MonitorMainDlg::OnHelpClick )

////@end MonitorMainDlg event table entries
    EVT_MENU(CPM_REFRPANEL, MonitorMainDlg::OnRefresh)
END_EVENT_TABLE()

/*!
 * MonitorMainDlg constructors
 */

MonitorMainDlg::MonitorMainDlg( )
{
}

MonitorMainDlg::MonitorMainDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * MonitorMainDlg creator
 */

bool MonitorMainDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MonitorMainDlg member initialisation
    mServer = NULL;
    mAccess = NULL;
    mBackground = NULL;
    mUser = NULL;
    mSecurAdm = NULL;
    mConfAdm = NULL;
    mDataAdm = NULL;
    mRefresh = NULL;
    mRefreshNow = NULL;
////@end MonitorMainDlg member initialisation
#ifdef WINS  // probably not necessary on GTK, and cannot be called before Create() 
   // change the backgroud colour so that it is the same as in dialogs:
    wxVisualAttributes va = wxDialog::GetClassDefaultAttributes();
    SetBackgroundColour(va.colBg);
#endif    
////@begin MonitorMainDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxScrolledWindow::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end MonitorMainDlg creation
   // start the functionality of scrollbars:
    FitInside();  
    SetScrollRate(8,8);
    return TRUE;
}

/*!
 * Control creation for MonitorMainDlg
 */

void MonitorMainDlg::CreateControls()
{    
////@begin MonitorMainDlg content construction
    MonitorMainDlg* itemScrolledWindow1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemScrolledWindow1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_TOP|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemScrolledWindow1, wxID_STATIC, _("&Monitoring server:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mServerStrings = NULL;
    mServer = new wxOwnerDrawnComboBox;
    mServer->Create( itemScrolledWindow1, CD_MONIT_SERVER, _T(""), wxDefaultPosition, wxDefaultSize, 0, mServerStrings, wxCB_READONLY );
    itemBoxSizer3->Add(mServer, 0, wxGROW|wxALL, 5);

    mAccess = new wxCheckBox;
    mAccess->Create( itemScrolledWindow1, CD_MONIT_ACCESS, _("&Client access is locked"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mAccess->SetValue(FALSE);
    itemBoxSizer3->Add(mAccess, 0, wxALIGN_LEFT|wxALL, 5);

    mBackground = new wxCheckBox;
    mBackground->Create( itemScrolledWindow1, CD_MONIT_BACKGROUND, _("&Background processes enabled"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mBackground->SetValue(FALSE);
    itemBoxSizer3->Add(mBackground, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer8, 0, wxALIGN_TOP|wxALL, 0);

    wxStaticText* itemStaticText9 = new wxStaticText;
    itemStaticText9->Create( itemScrolledWindow1, wxID_STATIC, _("Logged in as:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText9, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mUser = new wxTextCtrl;
    mUser->Create( itemScrolledWindow1, CD_MONIT_USER, _T(""), wxDefaultPosition, wxSize(140, -1), wxTE_READONLY );
    itemBoxSizer8->Add(mUser, 0, wxALIGN_LEFT|wxALL, 5);

    mSecurAdm = new wxStaticText;
    mSecurAdm->Create( itemScrolledWindow1, CD_MONIT_SECURADM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(mSecurAdm, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mConfAdm = new wxStaticText;
    mConfAdm->Create( itemScrolledWindow1, CD_MONIT_CONFADM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(mConfAdm, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mDataAdm = new wxStaticText;
    mDataAdm->Create( itemScrolledWindow1, CD_MONIT_DATAADM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(mDataAdm, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer14, 0, wxALIGN_TOP|wxALL, 0);

    wxStaticText* itemStaticText15 = new wxStaticText;
    itemStaticText15->Create( itemScrolledWindow1, wxID_STATIC, _("&Refresh all pages every (seconds):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText15, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mRefresh = new wxSpinCtrl;
    mRefresh->Create( itemScrolledWindow1, CD_MONIT_REFRESH, _("0"), wxDefaultPosition, wxSize(140, -1), wxSP_ARROW_KEYS, 0, 1000, 0 );
    itemBoxSizer14->Add(mRefresh, 0, wxALIGN_LEFT|wxALL, 5);

    mRefreshNow = new wxButton;
    mRefreshNow->Create( itemScrolledWindow1, CD_MONIT_REFRESH_NOW, _("Refresh &now"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(mRefreshNow, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxButton* itemButton18 = new wxButton;
    itemButton18->Create( itemScrolledWindow1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemButton18, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end MonitorMainDlg content construction
    mServer->Append(_(" - None - "));
    mServer->SetSelection(0);
    //mRefresh->SetFont(mDataAdm->GetFont()); // does not help
#ifdef WINS  // repairing error in spin control in WX 2.5.2
    HWND hEdit  = (HWND)::SendMessage((HWND)mRefresh->GetHandle(), WM_USER+106, 0, 0);
    ::SendMessage(hEdit, WM_SETFONT, (WPARAM)system_gui_font.GetResourceHandle(), MAKELPARAM(TRUE, 0));
#endif
}

/*!
 * Should we show tooltips?
 */

bool MonitorMainDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for CD_MONIT_REFRESH
 */

bool monitor_refresh_callback_locked = false;
 
void MonitorMainDlg::OnCdMonitRefreshUpdated( wxSpinEvent& event )
{  
  if (monitor_refresh_callback_locked) return;
  monitor_refresh_callback_locked = true;
  set_monitor_refresh(mRefresh->GetValue());
  event.Skip();
  monitor_refresh_callback_locked = false;
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_MONIT_REFRESH
 */

void MonitorMainDlg::OnCdMonitRefreshTextUpdated( wxCommandEvent& event )
{
  if (monitor_refresh_callback_locked) return;
  monitor_refresh_callback_locked = true;
  set_monitor_refresh(mRefresh->GetValue());
  event.Skip();
  monitor_refresh_callback_locked = false;
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_MONIT_ACCESS
 */

void MonitorMainDlg::OnCdMonitAccessClick( wxCommandEvent& event )
{ MonitorNotebook * monit = (MonitorNotebook*)GetParent();
  if (mAccess->GetValue())
    { if (cd_Lock_server(monit->cdp)) cd_Signalize(monit->cdp); }
  else
    cd_Unlock_server(monit->cdp);
  monit->reset_main_info();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_MONIT_BACKGROUND
 */

void MonitorMainDlg::OnCdMonitBackgroundClick( wxCommandEvent& event )
{ MonitorNotebook * monit = (MonitorNotebook*)GetParent();
  if (mBackground->GetValue())
    { if (cd_Operation_limits(monit->cdp, WORKER_RESTART)) cd_Signalize(monit->cdp); }
  else
    { if (cd_Operation_limits(monit->cdp, WORKER_STOP)) cd_Signalize(monit->cdp); }
  monit->reset_main_info();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for CD_MONIT_SERVER
 */

void MonitorMainDlg::OnCdMonitServerSelected( wxCommandEvent& event )
{ int sel = mServer->GetSelection();
  monitored_server_selected(sel!=wxNOT_FOUND ? mServer->GetString(sel).mb_str(*wxConvCurrent) : "");
  event.Skip();
}

void MonitorMainDlg::OnRefresh(wxCommandEvent & event)
// Must pass the event explicitly
{ MonitorNotebook * monit = wxGetApp().frame->monitor_window;
  monit->OnRefresh(event);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_MONIT_REFRESH_NOW
 */

void MonitorMainDlg::OnCdMonitRefreshNowClick( wxCommandEvent& event )
{ MonitorNotebook * monit = wxGetApp().frame->monitor_window;
  monit->OnRefresh(event);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void MonitorMainDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/monitor.html"));
}



/*!
 * Get bitmap resources
 */

wxBitmap MonitorMainDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin MonitorMainDlg bitmap retrieval
    return wxNullBitmap;
////@end MonitorMainDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon MonitorMainDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin MonitorMainDlg icon retrieval
    return wxNullIcon;
////@end MonitorMainDlg icon retrieval
}
