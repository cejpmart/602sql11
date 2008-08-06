/////////////////////////////////////////////////////////////////////////////
// Name:        UNCBackupDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/24/08 11:48:04
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "UNCBackupDlg.h"
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

#include "UNCBackupDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * UNCBackupDlg type definition
 */

IMPLEMENT_CLASS( UNCBackupDlg, wxDialog )

/*!
 * UNCBackupDlg event table definition
 */

BEGIN_EVENT_TABLE( UNCBackupDlg, wxDialog )

////@begin UNCBackupDlg event table entries
    EVT_BUTTON( wxID_OK, UNCBackupDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, UNCBackupDlg::OnCancelClick )

////@end UNCBackupDlg event table entries

END_EVENT_TABLE()

/*!
 * UNCBackupDlg constructors
 */

UNCBackupDlg::UNCBackupDlg(cdp_t cdpIn) : cdp(cdpIn)
{
}

/*!
 * UNCBackupDlg creator
 */

bool UNCBackupDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin UNCBackupDlg member initialisation
    mUserName = NULL;
    mPass1 = NULL;
    mPass2 = NULL;
////@end UNCBackupDlg member initialisation

////@begin UNCBackupDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end UNCBackupDlg creation
    return TRUE;
}

/*!
 * Control creation for UNCBackupDlg
 */

void UNCBackupDlg::CreateControls()
{    
////@begin UNCBackupDlg content construction
    UNCBackupDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticText* itemStaticText3 = new wxStaticText;
    itemStaticText3->Create( itemDialog1, wxID_STATIC, _("Temporary connection parameters for backups with UNC-path"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer4->AddGrowableRow(0);
    itemFlexGridSizer4->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer4, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("User name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText5, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mUserName = new wxTextCtrl;
    mUserName->Create( itemDialog1, CD_UNC_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(mUserName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPass1 = new wxTextCtrl;
    mPass1->Create( itemDialog1, CD_UNC_PASS1, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemFlexGridSizer4->Add(mPass1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText9 = new wxStaticText;
    itemStaticText9->Create( itemDialog1, wxID_STATIC, _("Password confirmation:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPass2 = new wxTextCtrl;
    mPass2->Create( itemDialog1, CD_UNC_PASS2, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemFlexGridSizer4->Add(mPass2, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton12->SetDefault();
    itemBoxSizer11->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton13 = new wxButton;
    itemButton13->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemButton13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end UNCBackupDlg content construction
  mUserName->SetMaxLength(100);
  mPass1->SetMaxLength(100);
  mPass2->SetMaxLength(100);
  char username[100+1];
  if (!cd_Get_property_value(cdp, sqlserver_owner, "BackupNetUsername", 0, username, sizeof(username)))
    mUserName->SetValue(wxString(username, *wxConvCurrent));
}

/*!
 * Should we show tooltips?
 */

bool UNCBackupDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap UNCBackupDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin UNCBackupDlg bitmap retrieval
    return wxNullBitmap;
////@end UNCBackupDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon UNCBackupDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin UNCBackupDlg icon retrieval
    return wxNullIcon;
////@end UNCBackupDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void UNCBackupDlg::OnOkClick( wxCommandEvent& event )
{
  if (mPass1->GetValue() != mPass2->GetValue())
    { error_box(_("Passwords are different"), this);  return; }
  char username[100+1], password[100+1];
  strmaxcpy(username, mUserName->GetValue().mb_str(*wxConvCurrent), sizeof(username));
  strmaxcpy(password, mPass1   ->GetValue().mb_str(*wxConvCurrent), sizeof(password));
  if (cd_Set_property_value(cdp, sqlserver_owner, "BackupNetUsername", 0, username))
    cd_Signalize2(cdp, this);
  if (cd_Set_property_value(cdp, sqlserver_owner, "BackupNetPassword", 0, password))
    cd_Signalize2(cdp, this);
  else
    event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void UNCBackupDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}


