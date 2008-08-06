/////////////////////////////////////////////////////////////////////////////
// Name:        LoginDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/25/07 09:14:56
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
#include "wbapiex.h"
#include "mngmain.h"
#include "wbvers.h"

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "LoginDlg.h"
#endif

#include "LoginDlg.h"

////@begin includes
////@end includes


////@begin XPM images
////@end XPM images

/*!
 * LoginDlg type definition
 */

IMPLEMENT_CLASS( LoginDlg, wxDialog )

/*!
 * LoginDlg event table definition
 */

BEGIN_EVENT_TABLE( LoginDlg, wxDialog )

////@begin LoginDlg event table entries
    EVT_BUTTON( wxID_OK, LoginDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, LoginDlg::OnCancelClick )

////@end LoginDlg event table entries

END_EVENT_TABLE()

/*!
 * LoginDlg constructors
 */

LoginDlg::LoginDlg(cdp_t cdpIn)
{
  cdp=cdpIn;
}

LoginDlg::LoginDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * LoginDlg creator
 */

bool LoginDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin LoginDlg member initialisation
    mUser = NULL;
    mPass = NULL;
////@end LoginDlg member initialisation

////@begin LoginDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end LoginDlg creation
    return TRUE;
}

/*!
 * Control creation for LoginDlg
 */

void LoginDlg::CreateControls()
{    
////@begin LoginDlg content construction
    LoginDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText( itemDialog1, wxID_STATIC, _("User name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mUserStrings = NULL;
    mUser = new wxComboBox( itemDialog1, CD_LOGIN_USER, _T(""), wxDefaultPosition, wxDefaultSize, 0, mUserStrings, wxCB_DROPDOWN );
    itemFlexGridSizer3->Add(mUser, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText( itemDialog1, wxID_STATIC, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPass = new wxTextCtrl( itemDialog1, CD_LOGIN_PASS, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemFlexGridSizer3->Add(mPass, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer8, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton9 = new wxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton10 = new wxButton( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemButton10, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end LoginDlg content construction
  mPass->SetMaxLength(MAX_PASSWORD_LEN);
 // fill the list of user names:
  char * list, *curr;
  if (!cd_List_objects(cdp, CATEG_USER, NULL, &list)) 
  { curr=list;
    while (*curr)
    { mUser->Append(wxString(curr, *wxConvCurrent));  // system conversion is not available here
      curr+=strlen(curr)+1+sizeof(tobjnum)+sizeof(uns16);
    }
    corefree(list);
  }  
  mUser->SetFocus();
}

/*!
 * Should we show tooltips?
 */

bool LoginDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap LoginDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin LoginDlg bitmap retrieval
    return wxNullBitmap;
////@end LoginDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon LoginDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin LoginDlg icon retrieval
    return wxNullIcon;
////@end LoginDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void LoginDlg::OnOkClick( wxCommandEvent& event )
{ 
  if (mUser->GetValue().Length()>OBJ_NAME_LEN)
    { wxBell();  mUser->SetFocus();  return; }
  if (cd_Login_par(cdp, mUser->GetValue().mb_str(*wxConvCurrent), mPass->GetValue().mb_str(*wxConvCurrent), 0))
    { wxMessageBox(_("Login failed"), _("Error"), wxICON_ERROR | wxOK, this);  return; }
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void LoginDlg::OnCancelClick( wxCommandEvent& event )
{
  event.Skip();
}


