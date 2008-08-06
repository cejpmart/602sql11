/////////////////////////////////////////////////////////////////////////////
// Name:        LoginDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/23/04 09:38:05
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//## #pragma implementation "LoginDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "LoginDlg.h"
#include "../bmps/logo.xpm"

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
    EVT_CHECKBOX( CD_LOGIN_ANONYMOUS, LoginDlg::OnCdLoginAnonymousClick )

////@end LoginDlg event table entries

END_EVENT_TABLE()

/*!
 * LoginDlg constructors
 */

LoginDlg::LoginDlg( )
{
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
    mBitmap = NULL;
    mAnonym = NULL;
    mNameCapt = NULL;
    mName = NULL;
    mPassCapt = NULL;
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

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBitmap mBitmapBitmap(wxNullBitmap);
    mBitmap = new wxStaticBitmap;
    mBitmap->Create( itemDialog1, wxID_STATIC, mBitmapBitmap, wxDefaultPosition, wxSize(111, 114), 0 );
    itemBoxSizer2->Add(mBitmap, 0, wxALIGN_TOP|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer4, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mAnonym = new wxCheckBox;
    mAnonym->Create( itemDialog1, CD_LOGIN_ANONYMOUS, _("Anonymous login"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mAnonym->SetValue(FALSE);
    if (ShowToolTips())
        mAnonym->SetToolTip(_("If checked, you will login without a user name and password. If not checked you must enter a user name and password."));
    itemBoxSizer4->Add(mAnonym, 0, wxALIGN_LEFT|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer6 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer6->AddGrowableCol(1);
    itemBoxSizer4->Add(itemFlexGridSizer6, 0, wxGROW|wxLEFT|wxRIGHT, 5);

    mNameCapt = new wxStaticText;
    mNameCapt->Create( itemDialog1, CD_LOGIN_NAME_C, _("User name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer6->Add(mNameCapt, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mNameStrings = NULL;
    mName = new wxOwnerDrawnComboBox;
    mName->Create( itemDialog1, CD_LOGIN_NAME, _T(""), wxDefaultPosition, wxSize(170, -1), 0, mNameStrings, wxCB_DROPDOWN|wxCB_SORT );
    itemFlexGridSizer6->Add(mName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mPassCapt = new wxStaticText;
    mPassCapt->Create( itemDialog1, CD_LOGIN_PASSWORD_C, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer6->Add(mPassCapt, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPass = new wxTextCtrl;
    mPass->Create( itemDialog1, CD_LOGIN_PASSWORD, _T(""), wxDefaultPosition, wxSize(170, -1), wxTE_PASSWORD );
    itemFlexGridSizer6->Add(mPass, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton12->SetDefault();
    itemBoxSizer11->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    wxButton* itemButton13 = new wxButton;
    itemButton13->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemButton13, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

////@end LoginDlg content construction
    wxBitmap bmp(logo_xpm);
    mBitmap->SetBitmap(bmp);
}

/*!
 * Should we show tooltips?
 */

bool LoginDlg::ShowToolTips()
{
    return TRUE;
}

bool LoginDlg::TransferDataToWindow(void)
{// dialog caption:
  wxString buf;
  buf.Printf(_("Login to Server %s"), wxString(cdp->locid_server_name, wxConvLocal).c_str());
  SetTitle(buf);
 // text limits:
  //mName->SetMaxLength(OBJ_NAME_LEN); -- this method is not supported in the wx
  mPass->SetMaxLength(MAX_PASSWORD_LEN);
 // fill the list of user names:
  void * en = get_object_enumerator(cdp, CATEG_USER, NULL);
  tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
  bool any_name_returned = false;
  while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
  { any_name_returned=true;
    if (stricmp(name, "Anonymous"))
      mName->Append(wxString(name, *cdp->sys_conv));
  }
  free_object_enumerator(en);
  if (!any_name_returned) mAnonym->Disable();
 // preset the last login name used:
  char last_name_utf8[2*OBJ_NAME_LEN+1];  bool prefer_anonymous;
  if (read_profile_string("Login", cdp->locid_server_name, last_name_utf8, sizeof(last_name_utf8)))
  { mName->SetValue(wxString(last_name_utf8, wxConvUTF8));
    prefer_anonymous = !*last_name_utf8;
  }
  else
    prefer_anonymous = true;  // arguable
  wxCommandEvent ce;
  if (prefer_anonymous && any_name_returned)
  { mAnonym->SetValue(true);
    OnCdLoginAnonymousClick(ce);  // disables controls
    FindWindow(wxID_OK)->SetFocus();
  }
  else
  { mAnonym->SetValue(false);
    mPass->SetFocus();
  }
  return true;
}

bool LoginDlg::TransferDataFromWindow(void)
{ char last_name_utf8[2*OBJ_NAME_LEN+1];  
  if (!mAnonym->GetValue())
    strcpy(last_name_utf8, mName->GetValue().mb_str(wxConvUTF8));
  else  // anonymous login
    *last_name_utf8=0;
  write_profile_string("Login", cdp->locid_server_name, last_name_utf8);
  return true;
}

bool LoginDlg::Validate(void)
{ char password[MAX_PASSWORD_LEN+1];  tobjname username;
 // sometimes the clients disconnects and the [cdp] if deleted, check for it
  if (cdp->connection_type != CONN_TP_602)
    return false;
  if (!mAnonym->GetValue())
  { 
    wxString u = mName->GetValue();
    if (u.Length() > OBJ_NAME_LEN)  // must check the length, cannot limit length in the combo control!
      { error_box(TEXT_TOO_LONG, this);  mName->SetFocus();  return false; }
    strcpy(username, u.mb_str(*cdp->sys_conv));
    strcpy(password, mPass->GetValue().mb_str(*cdp->sys_conv));
  }
  else  // anonymous login
    *password=*username=0;
  if (cd_Login_par(cdp, username, password, 0))
    if (cd_Sz_error(cdp)==PASSWORD_EXPIRED)
    { PasswordExpiredDlg dlg(cdp);
      dlg.Create(this);
      if (dlg.ShowModal()!=wxID_OK) return false;  // cancelled
      if (cd_Set_password(cdp, NULL, dlg.password))
        { report_last_error(cdp, this);  return false; }  // internal error
    }
    else
      { report_last_error(cdp, this);  return false; }
 // empty the caches: they contain privilege information depending on user!
  inval_table_d(cdp, NOOBJECT, CATEG_TABLE);  
  inval_table_d(cdp, NOOBJECT, CATEG_CURSOR);
  return true;
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_LOGIN_ANONYMOUS
 */

void LoginDlg::OnCdLoginAnonymousClick( wxCommandEvent& event )
{
  bool enabled = !mAnonym->GetValue();
  mName->Enable(enabled);
  mPass->Enable(enabled);
  mNameCapt->Enable(enabled);
  mPassCapt->Enable(enabled);
 // remove the old contents from user name edit, if "anonymous" checked:
  if (!enabled)
    mName->SetValue(wxEmptyString);
  event.Skip();
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
