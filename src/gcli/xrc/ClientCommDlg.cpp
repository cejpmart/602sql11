/////////////////////////////////////////////////////////////////////////////
// Name:        ClientCommDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/02/06 15:15:16
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "ClientCommDlg.h"
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

#include "ClientCommDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ClientCommDlg type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ClientCommDlg, wxDialog )

/*!
 * ClientCommDlg event table definition
 */

BEGIN_EVENT_TABLE( ClientCommDlg, wxDialog )

////@begin ClientCommDlg event table entries
    EVT_CHECKBOX( CD_COMM_SCAN, ClientCommDlg::OnCdCommScanClick )

    EVT_CHECKBOX( CD_COMM_NOTIFS, ClientCommDlg::OnCdCommNotifsClick )

    EVT_BUTTON( wxID_OK, ClientCommDlg::OnOkClick )

    EVT_BUTTON( wxID_HELP, ClientCommDlg::OnHelpClick )

////@end ClientCommDlg event table entries

END_EVENT_TABLE()

/*!
 * ClientCommDlg constructors
 */

ClientCommDlg::ClientCommDlg( )
{
}

ClientCommDlg::ClientCommDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ClientCommDlg creator
 */

bool ClientCommDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ClientCommDlg member initialisation
    mScan = NULL;
    mBroadcast = NULL;
    mNotifs = NULL;
    mModulus = NULL;
////@end ClientCommDlg member initialisation

////@begin ClientCommDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ClientCommDlg creation
    return TRUE;
}

/*!
 * Control creation for ClientCommDlg
 */

void ClientCommDlg::CreateControls()
{    
////@begin ClientCommDlg content construction
    ClientCommDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Scannig active SQL Servers"));
    wxStaticBoxSizer* itemStaticBoxSizer3 = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer3, 0, wxGROW|wxALL, 5);

    mScan = new wxCheckBox;
    mScan->Create( itemDialog1, CD_COMM_SCAN, _("Permanent scanning when client is not connected"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mScan->SetValue(FALSE);
    itemStaticBoxSizer3->Add(mScan, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer3->Add(itemBoxSizer5, 0, wxGROW|wxALL, 0);

    mBroadcast = new wxCheckBox;
    mBroadcast->Create( itemDialog1, CD_COMM_BROADCAST, _("Search for unregistered SQL servers in the network segment"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mBroadcast->SetValue(FALSE);
    itemBoxSizer5->Add(mBroadcast, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer7Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Notifications from the server"));
    wxStaticBoxSizer* itemStaticBoxSizer7 = new wxStaticBoxSizer(itemStaticBoxSizer7Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer7, 0, wxGROW|wxALL, 5);

    mNotifs = new wxCheckBox;
    mNotifs->Create( itemDialog1, CD_COMM_NOTIFS, _("Receive notifications during long operations"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mNotifs->SetValue(FALSE);
    itemStaticBoxSizer7->Add(mNotifs, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer7->Add(itemBoxSizer9, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText10 = new wxStaticText;
    itemStaticText10->Create( itemDialog1, wxID_STATIC, _("Get a notification every"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemStaticText10, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mModulus = new wxSpinCtrl;
    mModulus->Create( itemDialog1, CD_COMM_MODULUS, _("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 1000000, 0 );
    itemBoxSizer9->Add(mModulus, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText12 = new wxStaticText;
    itemStaticText12->Create( itemDialog1, wxID_STATIC, _("processed units"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer13, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton14 = new wxButton;
    itemButton14->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton15 = new wxButton;
    itemButton15->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton16 = new wxButton;
    itemButton16->Create( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ClientCommDlg content construction
    mNotifs->SetValue(notification_modulus>0);
    if (notification_modulus)
      mModulus->SetValue(notification_modulus);
    else
      mModulus->Disable();
#ifdef LINUX
    mScan->Disable();
#else
    mScan->SetValue(browse_enabled);
    if (!browse_enabled)  // broadcast does not work when browsing is not enabled
      mBroadcast->Disable();
    else
#endif
      mBroadcast->SetValue(broadcast_enabled);
}

/*!
 * Should we show tooltips?
 */

bool ClientCommDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap ClientCommDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ClientCommDlg bitmap retrieval
    return wxNullBitmap;
////@end ClientCommDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ClientCommDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ClientCommDlg icon retrieval
    return wxNullIcon;
////@end ClientCommDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_COMM_SCAN
 */

void ClientCommDlg::OnCdCommScanClick( wxCommandEvent& event )
{
#ifdef WINS  
  if (mScan->GetValue())
    mBroadcast->Enable();
  else
  { mBroadcast->SetValue(false);
    mBroadcast->Disable();
  }
#endif
  event.Skip();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_COMM_NOTIFS
 */

void ClientCommDlg::OnCdCommNotifsClick( wxCommandEvent& event )
{
  mModulus->Enable(mNotifs->GetValue());
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void ClientCommDlg::OnOkClick( wxCommandEvent& event )
{// get new values:
  if (mNotifs->GetValue())
    notification_modulus = mModulus->GetValue();
  else
    notification_modulus = 0;
  browse_enabled = mScan->GetValue();
  broadcast_enabled = mBroadcast->GetValue();  // is set to false if disabled
 // save:
#ifndef LINUX
  write_profile_bool("Network", "SearchServers", browse_enabled);
#endif
  write_profile_bool("Network", "Broadcast", broadcast_enabled);
  write_profile_int("Network", "Notifications", notification_modulus);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void ClientCommDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/client_commpars.html"));
}


