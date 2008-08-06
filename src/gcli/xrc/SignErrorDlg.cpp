/////////////////////////////////////////////////////////////////////////////
// Name:        SignErrorDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/22/04 14:51:03
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "SignErrorDlg.h"
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

#include "SignErrorDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * SignErrorDlg type definition
 */

IMPLEMENT_CLASS( SignErrorDlg, wxDialog )

/*!
 * SignErrorDlg event table definition
 */

BEGIN_EVENT_TABLE( SignErrorDlg, wxDialog )

////@begin SignErrorDlg event table entries
    EVT_BUTTON( wxID_OK, SignErrorDlg::OnOkClick )

    EVT_BUTTON( wxID_HELP, SignErrorDlg::OnHelpClick )

////@end SignErrorDlg event table entries

END_EVENT_TABLE()

/*!
 * SignErrorDlg constructors
 */

SignErrorDlg::SignErrorDlg(wxString captIn, wxString msgIn, int topicIn)
{ capt=captIn;  msg=msgIn;  topic=topicIn;
}


/*!
 * SignErrorDlg creator
 */

bool SignErrorDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SignErrorDlg member initialisation
    mMsg = NULL;
    mHelp = NULL;
////@end SignErrorDlg member initialisation

////@begin SignErrorDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end SignErrorDlg creation
    return TRUE;
}

/*!
 * Control creation for SignErrorDlg
 */

void SignErrorDlg::CreateControls()
{    
////@begin SignErrorDlg content construction

    SignErrorDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxStaticText* item3 = new wxStaticText;
    item3->Create( item1, CD_ERROR_MESSAGE, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
    mMsg = item3;
    item2->Add(item3, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 16);

    wxBoxSizer* item4 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item4, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* item5 = new wxButton;
    item5->Create( item1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item5->SetDefault();
    item4->Add(item5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item6 = new wxButton;
    item6->Create( item1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    mHelp = item6;
    item4->Add(item6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end SignErrorDlg content construction
    SetTitle(capt);
    mMsg->SetLabel(msg);
    mHelp->Enable(topic!=0);
}

/*!
 * Should we show tooltips?
 */

bool SignErrorDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void SignErrorDlg::OnOkClick( wxCommandEvent& event )
{
    event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void SignErrorDlg::OnHelpClick( wxCommandEvent& event )
{ wxString pathname;
  pathname.Printf(wxT("xml/html/err%u.html"), topic);
  wxGetApp().frame->help_ctrl.DisplaySection(pathname); 
}


