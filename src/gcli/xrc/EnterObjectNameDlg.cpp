/////////////////////////////////////////////////////////////////////////////
// Name:        EnterObjectNameDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/10/04 16:51:41
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "EnterObjectNameDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "EnterObjectNameDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * EnterObjectNameDlg type definition
 */

IMPLEMENT_CLASS( EnterObjectNameDlg, wxDialog )

/*!
 * EnterObjectNameDlg event table definition
 */

BEGIN_EVENT_TABLE( EnterObjectNameDlg, wxDialog )

////@begin EnterObjectNameDlg event table entries
    EVT_BUTTON( wxID_OK, EnterObjectNameDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, EnterObjectNameDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, EnterObjectNameDlg::OnHelpClick )

////@end EnterObjectNameDlg event table entries

END_EVENT_TABLE()

/*!
 * EnterObjectNameDlg constructors
 */

EnterObjectNameDlg::EnterObjectNameDlg(cdp_t cdpIn, wxString promptIn, bool renamingIn, int max_name_lenIn)
{ cdp=cdpIn;  prompt=promptIn;  renaming=renamingIn;  max_name_len=max_name_lenIn; }

/*!
 * EnterObjectNameDlg creator
 */

bool EnterObjectNameDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin EnterObjectNameDlg member initialisation
    mPrompt = NULL;
    mName = NULL;
////@end EnterObjectNameDlg member initialisation

////@begin EnterObjectNameDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end EnterObjectNameDlg creation
    return TRUE;
}

/*!
 * Control creation for EnterObjectNameDlg
 */

void EnterObjectNameDlg::CreateControls()
{    
////@begin EnterObjectNameDlg content construction

    EnterObjectNameDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxStaticText* item3 = new wxStaticText;
    item3->Create( item1, CD_NAME_PROMPT, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
    mPrompt = item3;
    item2->Add(item3, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item4 = new wxTextCtrl;
    item4->Create( item1, CD_NAME_NAME, _T(""), wxDefaultPosition, wxSize(250, -1), 0 );
    mName = item4;
    item2->Add(item4, 0, wxGROW|wxALL, 5);

    wxStaticLine* item5 = new wxStaticLine;
    item5->Create( item1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    item2->Add(item5, 0, wxGROW|wxALL, 5);

    wxBoxSizer* item6 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item6, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* item7 = new wxButton;
    item7->Create( item1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item7->SetDefault();
    item6->Add(item7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item8 = new wxButton;
    item8->Create( item1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item6->Add(item8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item9 = new wxButton;
    item9->Create( item1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    item6->Add(item9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end EnterObjectNameDlg content construction
  mPrompt->SetLabel(prompt);
  mName->SetMaxLength(max_name_len);
  mName->SetValue(name);
  mName->SetSelection(-1, -1);
  mName->SetFocus();  // necessary on Linux
}

/*!
 * Should we show tooltips?
 */

bool EnterObjectNameDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void EnterObjectNameDlg::OnOkClick( wxCommandEvent& event )
{ name = mName->GetValue();
  name.Trim(false);  name.Trim(true);
  if (name.IsEmpty())
    { unpos_box(_("The name cannot be empty."), this);  return; }
  if (!is_object_name(cdp, name, this)) return;  // the conversion may not the the proper one, but is it sufficient
  event.Skip();  // the result is in [name]
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void EnterObjectNameDlg::OnCancelClick( wxCommandEvent& event )
{ event.Skip(); }


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void EnterObjectNameDlg::OnHelpClick( wxCommandEvent& event )
{
  wxGetApp().frame->help_ctrl.DisplaySection(renaming ? wxT("xml/html/renaming.html") : wxT("xml/html/name.html"));
}


