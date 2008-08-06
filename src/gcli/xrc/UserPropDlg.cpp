/////////////////////////////////////////////////////////////////////////////
// Name:        UserPropDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/09/04 16:25:40
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "UserPropDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "UserPropDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * UserPropDlg type definition
 */

IMPLEMENT_CLASS( UserPropDlg, wxDialog )

/*!
 * UserPropDlg event table definition
 */

BEGIN_EVENT_TABLE( UserPropDlg, wxDialog )

////@begin UserPropDlg event table entries
    EVT_CHECKBOX( CD_USR_SPECPASS, UserPropDlg::OnCdUsrSpecpassClick )

    EVT_TEXT( CD_USR_PASS1, UserPropDlg::OnCdUsrPass1Updated )

    EVT_CHECKBOX( CD_USR_MUSTCHANGE, UserPropDlg::OnCdUsrMustchangeClick )

    EVT_BUTTON( wxID_OK, UserPropDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, UserPropDlg::OnCancelClick )

////@end UserPropDlg event table entries

END_EVENT_TABLE()

/*!
 * UserPropDlg constructors
 */

UserPropDlg::UserPropDlg(cdp_t cdpIn, tobjnum objnumIn)
{ cdp=cdpIn;  objnum=objnumIn;  any_password_change=false; }

/*!
 * UserPropDlg creator
 */

bool UserPropDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin UserPropDlg member initialisation
    mLoginName = NULL;
    mDisabled = NULL;
    mFirstName = NULL;
    mMiddleName = NULL;
    mLastName = NULL;
    mIdent = NULL;
    mSpecPass = NULL;
    mPass1C = NULL;
    mPass1 = NULL;
    mPass2C = NULL;
    mPass2 = NULL;
    mMustChange = NULL;
////@end UserPropDlg member initialisation

////@begin UserPropDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end UserPropDlg creation
    return TRUE;
}

/*!
 * Control creation for UserPropDlg
 */

void UserPropDlg::CreateControls()
{    
////@begin UserPropDlg content construction
    UserPropDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("Login &name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLoginName = new wxTextCtrl;
    mLoginName->Create( itemDialog1, CD_USR_LNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(mLoginName, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mDisabled = new wxCheckBox;
    mDisabled->Create( itemDialog1, CD_USR_DISABLED, _("Accout is &disabled"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mDisabled->SetValue(FALSE);
    itemBoxSizer2->Add(mDisabled, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer7Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Description of the user"));
    wxStaticBoxSizer* itemStaticBoxSizer7 = new wxStaticBoxSizer(itemStaticBoxSizer7Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer7, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxFlexGridSizer* itemFlexGridSizer8 = new wxFlexGridSizer(4, 2, 0, 0);
    itemFlexGridSizer8->AddGrowableCol(1);
    itemStaticBoxSizer7->Add(itemFlexGridSizer8, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText9 = new wxStaticText;
    itemStaticText9->Create( itemDialog1, wxID_STATIC, _("&First name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer8->Add(itemStaticText9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mFirstName = new wxTextCtrl;
    mFirstName->Create( itemDialog1, CD_USR_NAME1, _T(""), wxDefaultPosition, wxSize(130, -1), 0 );
    itemFlexGridSizer8->Add(mFirstName, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText11 = new wxStaticText;
    itemStaticText11->Create( itemDialog1, wxID_STATIC, _("&Middle initial:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer8->Add(itemStaticText11, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mMiddleName = new wxTextCtrl;
    mMiddleName->Create( itemDialog1, CD_USR_NAME2, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemFlexGridSizer8->Add(mMiddleName, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText13 = new wxStaticText;
    itemStaticText13->Create( itemDialog1, wxID_STATIC, _("&Last name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer8->Add(itemStaticText13, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mLastName = new wxTextCtrl;
    mLastName->Create( itemDialog1, CD_USR_NAME3, _T(""), wxDefaultPosition, wxSize(130, -1), 0 );
    itemFlexGridSizer8->Add(mLastName, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText15 = new wxStaticText;
    itemStaticText15->Create( itemDialog1, wxID_STATIC, _("&Additional identification:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer8->Add(itemStaticText15, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mIdent = new wxTextCtrl;
    mIdent->Create( itemDialog1, CD_USR_IDENT, _T(""), wxDefaultPosition, wxSize(130, -1), 0 );
    itemFlexGridSizer8->Add(mIdent, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticBox* itemStaticBoxSizer17Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Login password"));
    wxStaticBoxSizer* itemStaticBoxSizer17 = new wxStaticBoxSizer(itemStaticBoxSizer17Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer17, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer17->Add(itemBoxSizer18, 0, wxGROW|wxALL, 0);

    mSpecPass = new wxCheckBox;
    mSpecPass->Create( itemDialog1, CD_USR_SPECPASS, _("Specify the password for the login"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mSpecPass->SetValue(FALSE);
    itemBoxSizer18->Add(mSpecPass, 0, wxALIGN_LEFT|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer20 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer20->AddGrowableCol(1);
    itemBoxSizer18->Add(itemFlexGridSizer20, 0, wxGROW|wxALL, 0);

    mPass1C = new wxStaticText;
    mPass1C->Create( itemDialog1, wxID_STATIC, _("&Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer20->Add(mPass1C, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPass1 = new wxTextCtrl;
    mPass1->Create( itemDialog1, CD_USR_PASS1, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemFlexGridSizer20->Add(mPass1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mPass2C = new wxStaticText;
    mPass2C->Create( itemDialog1, wxID_STATIC, _("&Verify:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer20->Add(mPass2C, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mPass2 = new wxTextCtrl;
    mPass2->Create( itemDialog1, CD_USR_PASS2, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemFlexGridSizer20->Add(mPass2, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mMustChange = new wxCheckBox;
    mMustChange->Create( itemDialog1, CD_USR_MUSTCHANGE, _("&User must change password on next login"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mMustChange->SetValue(FALSE);
    itemBoxSizer18->Add(mMustChange, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer26, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton27 = new wxButton;
    itemButton27->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton27->SetDefault();
    itemBoxSizer26->Add(itemButton27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton28 = new wxButton;
    itemButton28->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemButton28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end UserPropDlg content construction
  if (objnum==NOOBJECT)
    SetTitle(_("Create a New User"));
 // determine privileges to the account:
  if (objnum!=NOOBJECT)
  { uns8 priv_buf[PRIVIL_DESCR_SIZE];
    editable = !cd_GetSet_privils(cdp, cdp->logged_as_user, CATEG_USER, USER_TABLENUM, objnum, OPER_GETEFF, priv_buf) &&
               HAS_WRITE_PRIVIL(priv_buf, USER_ATR_LOGNAME);
    password_editable = objnum==cdp->logged_as_user || cd_Am_I_security_admin(cdp);
  }
  else
    editable = password_editable = true;
 // set limits:
  mLoginName->SetMaxLength(OBJ_NAME_LEN);
  mFirstName->SetMaxLength(PUK_NAME1);
  mMiddleName->SetMaxLength(PUK_NAME2);
  mLastName->SetMaxLength(PUK_NAME3);
  mIdent->SetMaxLength(PUK_IDENT);
  mPass1->SetMaxLength(MAX_PASSWORD_LEN);
  mPass2->SetMaxLength(MAX_PASSWORD_LEN);
  if (!editable)
  { //mLoginName->Disable();
    //mFirstName->Disable();
    //mMiddleName->Disable();
    //mLastName->Disable();
    //mIdent->Disable();
    mLoginName->SetEditable(false);
    mFirstName->SetEditable(false);
    mMiddleName->SetEditable(false);
    mLastName->SetEditable(false);
    mIdent->SetEditable(false);
  }
 // Account_disabled: checkbox enabled for new users, disabled for itself, enabled for I am secur. admin, enabled when I am config. admin except for ANONYMOUS account
  if (objnum!=NOOBJECT)
  { if (cdp->logged_as_user==objnum)
        mDisabled->Disable();  
    if (!cd_Am_I_security_admin(cdp))
      if (objnum==ANONYMOUS_USER || !cd_Am_I_config_admin(cdp))
        mDisabled->Disable();  
  }
  if (!password_editable)
  { mPass1->Disable();  mPass1C->Disable();
    mPass2->Disable();  mPass2C->Disable();
    mMustChange->Disable();
  }
 // disable password operation:
  mPass1->Enable(false);
  mPass1C->Enable(false);
  mPass2->Enable(false);
  mPass2C->Enable(false);
  mMustChange->Enable(false);
 // show values:
  if (objnum!=NOOBJECT)
  { t_user_ident user_ident;
    cd_Read(cdp, USER_TABLENUM, objnum, USER_ATR_LOGNAME, NULL, orig_logname);
    mLoginName ->SetValue(wxString(orig_logname,       *cdp->sys_conv));
    cd_Read(cdp, USER_TABLENUM, objnum, USER_ATR_INFO,    NULL, &user_ident);
    mFirstName ->SetValue(wxString(user_ident.name1,   *cdp->sys_conv));
    mMiddleName->SetValue(wxString(user_ident.name2,   *cdp->sys_conv));
    mLastName  ->SetValue(wxString(user_ident.name3,   *cdp->sys_conv));
    mIdent     ->SetValue(wxString(user_ident.identif, *cdp->sys_conv));
   // account enabled:
    if (objnum==ANONYMOUS_USER) 
    { char result[2];
      cd_Get_property_value(cdp, "@SQLSERVER", "DisableAnonymous", 0, result, sizeof(result));
      orig_account_disabled = *result=='1';
    }
    else
    { wbbool state;
      cd_Read(cdp, USER_TABLENUM, objnum, USER_ATR_STOPPED, NULL, &state);
      orig_account_disabled = state;  // NULL is like false
    }
    mDisabled->SetValue(orig_account_disabled==wbtrue);
  }
  else
  { *orig_logname=0;  // used after OK click
    orig_account_disabled=false;
  }
}

/*!
 * Should we show tooltips?
 */

bool UserPropDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void UserPropDlg::OnOkClick( wxCommandEvent& event )
{ BOOL res = FALSE;
  tobjname logname;  t_user_ident user_ident;  char pass[1+MAX_PASSWORD_LEN+1];  wbbool new_account_disabled;
 // read values from the dialog and check them:
  if (password_editable)
  { if (mPass1->GetValue()!=mPass2->GetValue())
      { error_box(_("The passwords are not the same."));  return; }
    strcpy(pass,               mPass1     ->GetValue().mb_str(*cdp->sys_conv));
    if (password_short(cdp, pass))
      return;
    if (mMustChange->GetValue())
    { *pass=1;  // "must change" flag
      strcpy(pass+1,           mPass1     ->GetValue().mb_str(*cdp->sys_conv));
    }
  }
  if (editable)
  { wxString name = mLoginName ->GetValue().c_str();
   // check the login name:
    name.Trim(false);  name.Trim(true);
    if (name.IsEmpty())
      { error_box(_("The login name cannot be empty."), this);  return; }
    if (!is_object_name(cdp, name, this)) return;
   // check for login name conflict:
    strcpy(logname, name.mb_str(*cdp->sys_conv));
    tobjnum new_objnum;
    if (!cd_Find2_object(cdp, logname, NULL, CATEG_USER, &new_objnum))
      if (new_objnum!=objnum)  // condition is true for new users
        { error_box(_("This login name is already in use."));  return; }
    new_account_disabled = mDisabled->GetValue();
    if (cdp->logged_as_user==objnum && new_account_disabled)  // condition is false for new users
      error_box(_("Cannot disable your own account."));
    strcpy(user_ident.name1,   mFirstName ->GetValue().mb_str(*cdp->sys_conv));
    strcpy(user_ident.name2,   mMiddleName->GetValue().mb_str(*cdp->sys_conv));
    strcpy(user_ident.name3,   mLastName  ->GetValue().mb_str(*cdp->sys_conv));
    strcpy(user_ident.identif, mIdent     ->GetValue().mb_str(*cdp->sys_conv));
  }

 // same data from the dialog:
  if (objnum==NOOBJECT)  // creating a new user
    res=cd_Create_user(cdp, logname, user_ident.name1, user_ident.name2, user_ident.name3, user_ident.identif, 
                       NULL, pass, &objnum);
  else  // modifying an existing user
  { if (password_editable)
      if (mSpecPass->GetValue())
        res |= cd_Set_password(cdp, orig_logname, pass);  // password must be stored before [orig_logname] is changed!
    if (editable)
    { if (my_stricmp(orig_logname, logname))
        res|=cd_Write(cdp, USER_TABLENUM, objnum, USER_ATR_LOGNAME, NULL,  logname, OBJ_NAME_LEN);
      res|=cd_Write(cdp, USER_TABLENUM, objnum, USER_ATR_INFO,    NULL,  &user_ident, sizeof(t_user_ident));
    }
  }
 // account_disabled: common for new and existing users:
  if (mDisabled->IsEnabled() && new_account_disabled != orig_account_disabled)
    if (objnum==ANONYMOUS_USER)
      res|=cd_Set_property_value(cdp, "@SQLSERVER", "DisableAnonymous", 0, new_account_disabled ? "1" : "0");
    else
      res|=cd_Write(cdp, USER_TABLENUM, objnum, USER_ATR_STOPPED, NULL, &new_account_disabled, 1);
 // report errors:
  if (res)
    { cd_Signalize(cdp);  return; }
 // users are not cached, update the object list only:
  if (my_stricmp(orig_logname, logname))
    wxGetApp().frame->control_panel->objlist->Refresh();
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void UserPropDlg::OnCancelClick( wxCommandEvent& event )
{ event.Skip(); }



/*!
 * Get bitmap resources
 */

wxBitmap UserPropDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin UserPropDlg bitmap retrieval
    return wxNullBitmap;
////@end UserPropDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon UserPropDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin UserPropDlg icon retrieval
    return wxNullIcon;
////@end UserPropDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_USR_PASS1
 */

void UserPropDlg::OnCdUsrPass1Updated( wxCommandEvent& event )
{
  any_password_change=true;  // not used any more
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_USR_MUSTCHANGE
 */

void UserPropDlg::OnCdUsrMustchangeClick( wxCommandEvent& event )
{
  any_password_change=true;  // not used any more
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_USR_SPECPASS
 */

void UserPropDlg::OnCdUsrSpecpassClick( wxCommandEvent& event )
{
  bool edit_pass = mSpecPass->GetValue();
  mPass1->Enable(edit_pass);
  mPass1C->Enable(edit_pass);
  mPass2->Enable(edit_pass);
  mPass2C->Enable(edit_pass);
  mMustChange->Enable(edit_pass);
}


