/////////////////////////////////////////////////////////////////////////////
// Name:        PasswordExpiredDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/10/04 12:25:52
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "PasswordExpiredDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "PasswordExpiredDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * PasswordExpiredDlg type definition
 */

IMPLEMENT_CLASS( PasswordExpiredDlg, wxDialog )

/*!
 * PasswordExpiredDlg event table definition
 */

BEGIN_EVENT_TABLE( PasswordExpiredDlg, wxDialog )

////@begin PasswordExpiredDlg event table entries
    EVT_BUTTON( wxID_OK, PasswordExpiredDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, PasswordExpiredDlg::OnCancelClick )

////@end PasswordExpiredDlg event table entries

END_EVENT_TABLE()

/*!
 * PasswordExpiredDlg constructors
 */

PasswordExpiredDlg::PasswordExpiredDlg(cdp_t cdpIn)
{ cdp=cdpIn; }

/*!
 * PasswordExpiredDlg creator
 */

bool PasswordExpiredDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin PasswordExpiredDlg member initialisation
    mPass1C = NULL;
    mPass1 = NULL;
    mPass2C = NULL;
    mPass2 = NULL;
////@end PasswordExpiredDlg member initialisation

////@begin PasswordExpiredDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end PasswordExpiredDlg creation
    return TRUE;
}

/*!
 * Control creation for PasswordExpiredDlg
 */

void PasswordExpiredDlg::CreateControls()
{    
////@begin PasswordExpiredDlg content construction
    PasswordExpiredDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticText* itemStaticText3 = new wxStaticText;
    itemStaticText3->Create( itemDialog1, wxID_STATIC, _("Your password has expired.\nEnter a new password now."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer4->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer4, 0, wxGROW|wxALL, 0);

    mPass1C = new wxStaticText;
    mPass1C->Create( itemDialog1, wxID_STATIC, _("&Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(mPass1C, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPass1 = new wxTextCtrl;
    mPass1->Create( itemDialog1, CD_EXP_PASS1, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemFlexGridSizer4->Add(mPass1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mPass2C = new wxStaticText;
    mPass2C->Create( itemDialog1, wxID_STATIC, _("&Verify:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(mPass2C, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mPass2 = new wxTextCtrl;
    mPass2->Create( itemDialog1, CD_EXP_PASS2, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemFlexGridSizer4->Add(mPass2, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer9, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton10 = new wxButton;
    itemButton10->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton10->SetDefault();
    itemBoxSizer9->Add(itemButton10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton11 = new wxButton;
    itemButton11->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end PasswordExpiredDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool PasswordExpiredDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void PasswordExpiredDlg::OnOkClick( wxCommandEvent& event )
{ if (mPass1->GetValue()!=mPass2->GetValue())
    { error_box(_("The passwords are not the same."), this);  return; }
  strcpy(password, mPass1->GetValue().mb_str(*cdp->sys_conv));
  if (password_short(cdp, password))
    return;
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void PasswordExpiredDlg::OnCancelClick( wxCommandEvent& event )
{ event.Skip(); }



/*!
 * Get bitmap resources
 */

wxBitmap PasswordExpiredDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin PasswordExpiredDlg bitmap retrieval
    return wxNullBitmap;
////@end PasswordExpiredDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PasswordExpiredDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin PasswordExpiredDlg icon retrieval
    return wxNullIcon;
////@end PasswordExpiredDlg icon retrieval
}
