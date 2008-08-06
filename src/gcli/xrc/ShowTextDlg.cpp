/////////////////////////////////////////////////////////////////////////////
// Name:        ShowTextDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/03/04 14:39:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "ShowTextDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "ShowTextDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ShowTextDlg type definition
 */

IMPLEMENT_CLASS( ShowTextDlg, wxDialog )

/*!
 * ShowTextDlg event table definition
 */

BEGIN_EVENT_TABLE( ShowTextDlg, wxDialog )

////@begin ShowTextDlg event table entries
////@end ShowTextDlg event table entries

END_EVENT_TABLE()

/*!
 * ShowTextDlg constructors
 */

ShowTextDlg::ShowTextDlg( )
{
}

ShowTextDlg::ShowTextDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ShowTextDlg creator
 */

bool ShowTextDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ShowTextDlg member initialisation
    mText = NULL;
////@end ShowTextDlg member initialisation

////@begin ShowTextDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ShowTextDlg creation
    return TRUE;
}

/*!
 * Control creation for ShowTextDlg
 */

void ShowTextDlg::CreateControls()
{    
////@begin ShowTextDlg content construction

    ShowTextDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxTextCtrl* item3 = new wxTextCtrl;
    item3->Create( item1, CD_TD_SQL, _T(""), wxDefaultPosition, wxSize(450, 200), wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL );
    mText = item3;
    item2->Add(item3, 1, wxGROW|wxALL, 5);

    wxButton* item4 = new wxButton;
    item4->Create( item1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    item2->Add(item4, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

////@end ShowTextDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool ShowTextDlg::ShowToolTips()
{
    return TRUE;
}
