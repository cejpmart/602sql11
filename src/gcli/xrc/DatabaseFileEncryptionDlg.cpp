/////////////////////////////////////////////////////////////////////////////
// Name:        DatabaseFileEncryptionDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/02/04 17:47:35
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "DatabaseFileEncryptionDlg.h"
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

#include "DatabaseFileEncryptionDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * DatabaseFileEncryptionDlg type definition
 */

IMPLEMENT_CLASS( DatabaseFileEncryptionDlg, wxDialog )

/*!
 * DatabaseFileEncryptionDlg event table definition
 */

BEGIN_EVENT_TABLE( DatabaseFileEncryptionDlg, wxDialog )

////@begin DatabaseFileEncryptionDlg event table entries
    EVT_RADIOBOX( CD_FILENC_TYPE, DatabaseFileEncryptionDlg::OnCdFilencTypeSelected )

    EVT_BUTTON( wxID_OK, DatabaseFileEncryptionDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, DatabaseFileEncryptionDlg::OnCancelClick )

////@end DatabaseFileEncryptionDlg event table entries

END_EVENT_TABLE()

/*!
 * DatabaseFileEncryptionDlg constructors
 */

#define MAX_PASS_LEN 98

DatabaseFileEncryptionDlg::DatabaseFileEncryptionDlg(cdp_t cdpIn)
{ cdp=cdpIn; }

/*!
 * DatabaseFileEncryptionDlg creator
 */

bool DatabaseFileEncryptionDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DatabaseFileEncryptionDlg member initialisation
    mType = NULL;
    mPass1 = NULL;
    mPass2 = NULL;
    mOK = NULL;
////@end DatabaseFileEncryptionDlg member initialisation

////@begin DatabaseFileEncryptionDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end DatabaseFileEncryptionDlg creation
    return TRUE;
}

/*!
 * Control creation for DatabaseFileEncryptionDlg
 */

void DatabaseFileEncryptionDlg::CreateControls()
{    
////@begin DatabaseFileEncryptionDlg content construction
    DatabaseFileEncryptionDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxString mTypeStrings[] = {
        _("&No encryption"),
        _("&Simple encryption without password"),
        _("&Fast encryption with password"),
        _("&DES encryption")
    };
    mType = new wxRadioBox;
    mType->Create( itemDialog1, CD_FILENC_TYPE, _("Encryption type"), wxDefaultPosition, wxDefaultSize, 4, mTypeStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer2->Add(mType, 0, wxGROW|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer4->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer4, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("&Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText5, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPass1 = new wxTextCtrl;
    mPass1->Create( itemDialog1, CD_FILENC_PASS1, _T(""), wxDefaultPosition, wxSize(130, -1), wxTE_PASSWORD );
    itemFlexGridSizer4->Add(mPass1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("Confirm pass&word:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mPass2 = new wxTextCtrl;
    mPass2->Create( itemDialog1, CD_FILENC_PASS2, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemFlexGridSizer4->Add(mPass2, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText9 = new wxStaticText;
    itemStaticText9->Create( itemDialog1, wxID_STATIC, _("Warning: Changing the encryption of a large \ndatabase file may consume a significant time."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText9, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer10, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mOK = new wxButton;
    mOK->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(mOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end DatabaseFileEncryptionDlg content construction
  mPass1->SetMaxLength(MAX_PASS_LEN);
  mPass2->SetMaxLength(MAX_PASS_LEN);
  if (!cd_Am_I_security_admin(cdp))   // security-related property
    mOK->Disable();
 // show the encryption:
  char buf[2];
  if (cd_Get_property_value(cdp, sqlserver_owner, "FilEncrypt", 0, buf, sizeof(buf)))
    *buf=0;
  mType->SetSelection(*buf-'0');
  if (*buf<'2')
    { mPass1->Disable();  mPass2->Disable(); }
}

/*!
 * Should we show tooltips?
 */

bool DatabaseFileEncryptionDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DatabaseFileEncryptionDlg::OnOkClick( wxCommandEvent& event )
{ char buf[1+MAX_PASS_LEN+1];  BOOL res;
  if (mType->GetSelection()>1)
  { if (mPass1->GetValue() != mPass2->GetValue())
      { error_box(_("The passwords are different!"), this);  return; }
    if (mPass1->GetValue().IsEmpty())
      { error_box(_("The password cannot be empty!"), this);  return; }
  }
  *buf = '0' + mType->GetSelection();
  strcpy(buf+1, mPass1->GetValue().mb_str(*wxConvCurrent));
  { wxBusyCursor wait;
    res=cd_Set_property_value(cdp, sqlserver_owner, "FilEncrypt", 0, buf);
  }
  if (res) { cd_Signalize(cdp);  return; }
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DatabaseFileEncryptionDlg::OnCancelClick( wxCommandEvent& event )
{
  event.Skip();
}


/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_FILENC_TYPE
 */

void DatabaseFileEncryptionDlg::OnCdFilencTypeSelected( wxCommandEvent& event )
{
  bool pass = mType->GetSelection()>1;
  mPass1->Enable(pass);
  mPass2->Enable(pass);
  event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap DatabaseFileEncryptionDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin DatabaseFileEncryptionDlg bitmap retrieval
    return wxNullBitmap;
////@end DatabaseFileEncryptionDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon DatabaseFileEncryptionDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin DatabaseFileEncryptionDlg icon retrieval
    return wxNullIcon;
////@end DatabaseFileEncryptionDlg icon retrieval
}
