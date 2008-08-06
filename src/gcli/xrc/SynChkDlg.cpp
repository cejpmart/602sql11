/////////////////////////////////////////////////////////////////////////////
// Name:        SynChkDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/21/04 15:50:58
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "SynChkDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
#include "wx/wx.h"
////@end includes

#include "SynChkDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * SynChkDlg type definition
 */

IMPLEMENT_CLASS( SynChkDlg, wxDialog )

/*!
 * SynChkDlg event table definition
 */

BEGIN_EVENT_TABLE( SynChkDlg, wxDialog )

////@begin SynChkDlg event table entries
////@end SynChkDlg event table entries

END_EVENT_TABLE()

/*!
 * SynChkDlg constructors
 */

SynChkDlg::SynChkDlg( )
{
}

SynChkDlg::SynChkDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * SynChkDlg creator
 */

bool SynChkDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SynChkDlg member initialisation
////@end SynChkDlg member initialisation

////@begin SynChkDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end SynChkDlg creation
    return TRUE;
}

/*!
 * Control creation for SynChkDlg
 */

void SynChkDlg::CreateControls()
{    
////@begin SynChkDlg content construction

    SynChkDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxFlexGridSizer* item3 = new wxFlexGridSizer(2, 2, 0, 0);
    item2->Add(item3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStaticText* item4 = new wxStaticText( item1, wxID_STATIC, _("Checking:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item4, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item5 = new wxTextCtrl( item1, CD_SYNCHK_OBJECT, _T(""), wxDefaultPosition, wxSize(200, -1), wxTE_READONLY );
    item3->Add(item5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* item6 = new wxStaticText( item1, wxID_STATIC, _("Total checked:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item7 = new wxTextCtrl( item1, CD_SYNCHK_COUNT, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    item3->Add(item7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end SynChkDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool SynChkDlg::ShowToolTips()
{
    return TRUE;
}
