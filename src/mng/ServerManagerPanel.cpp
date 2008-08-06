/////////////////////////////////////////////////////////////////////////////
// Name:        ServerManagerPanel.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/18/07 14:59:13
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#pragma hdrstop
#ifdef UNIX
#include <winrepl.h>
#endif
#include "wbkernel.h"
#include "mngmain.h"
#include "wbvers.h"

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ServerManagerPanel.h"
#endif

#include "ServerManagerPanel.h"
#include "ClientsConnectedDlg.h"
#include "EnterPassword.h"
#include "LoginDlg.h"
////@begin includes
////@end includes

////@begin XPM images
////@end XPM images

/*!
 * ServerManagerPanel type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ServerManagerPanel, wxPanel )

/*!
 * ServerManagerPanel event table definition
 */

BEGIN_EVENT_TABLE( ServerManagerPanel, wxPanel )

////@begin ServerManagerPanel event table entries
    EVT_COMBOBOX( CD_SRVMNG_SERVERS, ServerManagerPanel::OnCdSrvmngServersSelected )

    EVT_BUTTON( CD_SRVMNG_STARTTASK, ServerManagerPanel::OnCdSrvmngStarttaskClick )

    EVT_BUTTON( CD_SRVMNG_STOP, ServerManagerPanel::OnCdSrvmngStopClick )

    EVT_BUTTON( wxID_CLOSE, ServerManagerPanel::OnCloseClick )

    EVT_BUTTON( CD_SRVMNG_REFRESH, ServerManagerPanel::OnCdSrvmngRefreshClick )

    EVT_SPINCTRL( CD_SRVMNG_SPIN, ServerManagerPanel::OnCdSrvmngSpinUpdated )

////@end ServerManagerPanel event table entries
    EVT_TIMER(1, ServerManagerPanel::OnTimer)

END_EVENT_TABLE()

/*!
 * ServerManagerPanel constructors
 */

ServerManagerPanel::ServerManagerPanel( ) : refr_timer(this, 1)
{
}

ServerManagerPanel::ServerManagerPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
  : refr_timer(this, 1)
{
    Create(parent, id, pos, size, style);
}

/*!
 * ServerManagerPanel creator
 */

bool ServerManagerPanel::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ServerManagerPanel member initialisation
    mServers = NULL;
    mDaemon = NULL;
    mPass = NULL;
    mStartTask = NULL;
    mStop = NULL;
    mInfo1 = NULL;
    mInfo2 = NULL;
    mInfo3 = NULL;
    mInfo4 = NULL;
    mInfo5 = NULL;
    mRefresh = NULL;
    mSpin = NULL;
    mLog = NULL;
////@end ServerManagerPanel member initialisation

////@begin ServerManagerPanel creation
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ServerManagerPanel creation
    return TRUE;
}

/*!
 * Control creation for ServerManagerPanel
 */

void ServerManagerPanel::CreateControls()
{    
////@begin ServerManagerPanel content construction
    ServerManagerPanel* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer4, 0, wxALIGN_TOP|wxALL, 0);

    itemBoxSizer4->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 7);

    wxStaticText* itemStaticText6 = new wxStaticText( itemPanel1, wxID_STATIC, _("Server:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText6, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer7, 1, wxALIGN_TOP|wxALL, 0);

    wxString* mServersStrings = NULL;
    mServers = new ServersOwnerDrawnComboBox( itemPanel1, CD_SRVMNG_SERVERS, _T(""), wxDefaultPosition, wxDefaultSize, 0, mServersStrings, wxCB_READONLY|wxCB_SORT );
    itemBoxSizer7->Add(mServers, 1, wxGROW|wxALL, 5);

    mDaemon = new wxCheckBox( itemPanel1, CD_SRVMNG_DAEMON, _("Start the server as a system service"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mDaemon->SetValue(FALSE);
    itemBoxSizer7->Add(mDaemon, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 5);

    mPass = new wxCheckBox( itemPanel1, CD_SRVMNG_PASS, _("Specify the password for the database"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mPass->SetValue(FALSE);
    itemBoxSizer7->Add(mPass, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer11, 0, wxALIGN_TOP|wxALL, 0);

    mStartTask = new wxButton( itemPanel1, CD_SRVMNG_STARTTASK, _("Start Server"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(mStartTask, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    mStop = new wxButton( itemPanel1, CD_SRVMNG_STOP, _("Stop Server"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(mStop, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxButton* itemButton14 = new wxButton( itemPanel1, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemButton14, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer15Static = new wxStaticBox(itemPanel1, wxID_ANY, _("Server information"));
    wxStaticBoxSizer* itemStaticBoxSizer15 = new wxStaticBoxSizer(itemStaticBoxSizer15Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer15, 0, wxGROW|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer16 = new wxFlexGridSizer(2, 3, 0, 0);
    itemFlexGridSizer16->AddGrowableCol(1);
    itemStaticBoxSizer15->Add(itemFlexGridSizer16, 0, wxGROW|wxALL, 0);

    mInfo1 = new wxStaticText( itemPanel1, CD_SRVMNG_INFO1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer16->Add(mInfo1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    itemFlexGridSizer16->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    mInfo2 = new wxStaticText( itemPanel1, CD_SRVMNG_INFO2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer16->Add(mInfo2, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mInfo3 = new wxStaticText( itemPanel1, CD_SRVMNG_INFO3, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer16->Add(mInfo3, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    itemFlexGridSizer16->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    mInfo4 = new wxStaticText( itemPanel1, CD_SRVMNG_INFO4, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer16->Add(mInfo4, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mInfo5 = new wxStaticText( itemPanel1, CD_SRVMNG_INFO5, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer15->Add(mInfo5, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer24, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText25 = new wxStaticText( itemPanel1, wxID_STATIC, _("Current server log:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText25, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 5);

    itemBoxSizer24->Add(30, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mRefresh = new wxButton( itemPanel1, CD_SRVMNG_REFRESH, _("Refresh now"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(mRefresh, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText28 = new wxStaticText( itemPanel1, wxID_STATIC, _("Refresh state every:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText28, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mSpin = new wxSpinCtrl( itemPanel1, CD_SRVMNG_SPIN, _("0"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 1000, 0 );
    itemBoxSizer24->Add(mSpin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText30 = new wxStaticText( itemPanel1, wxID_STATIC, _("seconds"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText30, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mLogStrings = NULL;
    mLog = new wxListBox( itemPanel1, CD_SRVMNG_LOG, wxDefaultPosition, wxSize(-1, 100), 0, mLogStrings, wxLB_SINGLE|wxLB_HSCROLL );
    itemBoxSizer2->Add(mLog, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end ServerManagerPanel content construction
    refresh_server_list();
    mSpin->SetValue(0);
    wxSpinEvent ev;
    OnCdSrvmngSpinUpdated(ev);  // start the timer
#ifdef UNIX
    mDaemon->SetLabel(_("Start the server as a daemon"));
#endif    
    mLog->SetInitialSize(wxSize(200,100));  // this prevents creating a very wide manager window when a local SQL server with a long text in its log is running
}

/*!
 * Should we show tooltips?
 */

bool ServerManagerPanel::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap ServerManagerPanel::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ServerManagerPanel bitmap retrieval
    return wxNullBitmap;
////@end ServerManagerPanel bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ServerManagerPanel::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ServerManagerPanel icon retrieval
    return wxNullIcon;
////@end ServerManagerPanel icon retrieval
}
/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_SRVMNG_SERVERS
 */

void ServerManagerPanel::OnCdSrvmngServersSelected( wxCommandEvent& event )
{
  show_selected_server_info();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SRVMNG_REFRESH
 */

void ServerManagerPanel::OnCdSrvmngRefreshClick( wxCommandEvent& event )
{
  refresh_server_list();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SRVMNG_STARTTASK
 */

void ServerManagerPanel::OnCdSrvmngStarttaskClick( wxCommandEvent& event )
{ int err;  char password[MAX_FIL_PASSWORD+1];  bool spec_password=false;
  if (mPass->GetValue())
  { EnterPassword dlg(this);
    if (dlg.ShowModal()==wxID_CANCEL) return;
    strcpy(password, dlg.mPass->GetValue().mb_str(*wxConvCurrent));  
    spec_password=true;
  }
  int i=mServers->GetSelection();
  if (i==wxNOT_FOUND) return;
  i = get_server_list_index_from_combo_selection(mServers, i);
  if (i<0) return;  // fuse
 // register the service, if not registered:
#ifdef WINS
  if (mDaemon->GetValue() && !*loc_server_list[i].service_name)
  { if (wxMessageBox(_("No service is registerd for the selected server. Should it be registered now?"), _("Confirm"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
      return;
    strcpy(loc_server_list[i].service_name, "602SQL_");  strcat(loc_server_list[i].service_name, loc_server_list[i].name);
    if (!srv_Register_service(loc_server_list[i].name, loc_server_list[i].service_name))
    { wxMessageBox(_("Cannot register the service"), _("Error"), wxICON_ERROR | wxOK, this);
      return;
    }
  }
#endif
 // start: 
  do
  {// try starting the server: 
    { wxBusyCursor busy;
      err = srv_Start_server_local(loc_server_list[i].name, mDaemon->GetValue() ? -1 : 6, spec_password ? password : "");  // passing "" in order to prevent the server waiting for entering the password
      if (!err)
      { wxThread::Sleep(1500);  
        show_selected_server_info();
        return;
      }
    }  
    if (err==KSE_BAD_PASSWORD)  
    { EnterPassword dlg(this);
      if (dlg.ShowModal()==wxID_CANCEL) return;
      strcpy(password, dlg.mPass->GetValue().mb_str(*wxConvCurrent));  
      spec_password=true;
    }
    else break;
  } while (true);
  wxString msg;
  msg.Printf(_("Error %u when starting the SQL server."), err); 
  wxMessageBox(msg, _("Error"), wxICON_ERROR | wxOK, this);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SRVMNG_STOP
 */

void ServerManagerPanel::OnCdSrvmngStopClick( wxCommandEvent& event )
{
  int i=mServers->GetSelection();
  if (i==wxNOT_FOUND) return;
  i = get_server_list_index_from_combo_selection(mServers, i);
  if (i<0) return;  // fuse
 // get the current numbert of connected clients:
  show_selected_server_info();
  int wait_time=0;
  if (curr_clients>0)
  { ClientsConnectedDlg dlg(this);
    wxString msg;
    msg.Printf(_("%u clients are connected to the SQL server at the momment."), curr_clients);
    dlg.mMsg->SetLabel(msg);
    dlg.Fit();
    if (dlg.ShowModal() == wxID_CANCEL) 
      return;
    if (dlg.mOpt->GetSelection()==0) wait_time=60;
  }
  int err;
  { wxBusyCursor busy;
    err = srv_Stop_server_local(loc_server_list[i].name, wait_time, NULL);
    if (!err)
    { wxThread::Sleep(1500);  
      show_selected_server_info();
      return;
    }  
  }  
 // try to login and the shutdown: 
  if (err==NO_CONF_RIGHT)
  { cd_t cd;  cdp_t cdp = &cd;
    cdp_init(cdp);
    err=cd_connect(cdp, loc_server_list[i].name, 2000);
    if (err!=KSE_OK)
    { wxString msg;
      msg.Printf(_("Error %u when connecting to the server."), err);
      wxMessageBox(msg, _("Error"), wxICON_ERROR | wxOK, this);
      return; 
    }  
    LoginDlg dlg(cdp);
    dlg.Create(this);
    err=dlg.ShowModal();
    if (err!=wxID_OK)  // Cancelled
    { cd_disconnect(cdp);
      return;
    }
    { wxBusyCursor busy;
      err = srv_Stop_server_local(loc_server_list[i].name, wait_time, cdp);
      cd_disconnect(cdp);
      if (!err)
      { wxThread::Sleep(1500);  
        show_selected_server_info();
        return;
      }  
    }
  }
  wxMessageBox(_("Error stopping the SQL server."), _("Error"), wxICON_ERROR | wxOK, this);
}


/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for CD_SRVMNG_SPIN
 */

void ServerManagerPanel::OnCdSrvmngSpinUpdated( wxSpinEvent& event )
{
  int refr_secs=mSpin->GetValue();
  if (!refr_secs || refr_secs==(int)0x80000000)  // 0x80000000 obtained when the text control becomes empty
    refr_timer.Stop();
  else
    refr_timer.Start(1000*refr_secs, false);
}

void ServerManagerPanel::OnTimer(wxTimerEvent& event)
{ 
  show_selected_server_info();
}



/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void ServerManagerPanel::OnCloseClick( wxCommandEvent& event )
{
  GetParent()->Destroy();
}


