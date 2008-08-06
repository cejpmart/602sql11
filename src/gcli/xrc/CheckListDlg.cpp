/////////////////////////////////////////////////////////////////////////////
// Name:        CheckListDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/04/04 16:48:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "CheckListDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "CheckListDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * CheckListDlg type definition
 */

IMPLEMENT_CLASS( CheckListDlg, wxPanel )

/*!
 * CheckListDlg event table definition
 */

BEGIN_EVENT_TABLE( CheckListDlg, wxPanel )

////@begin CheckListDlg event table entries
////@end CheckListDlg event table entries
    EVT_BUTTON( CD_CHKL_DROP, CheckListDlg::OnCdChklDropClick )
    EVT_BUTTON( wxID_HELP, CheckListDlg::OnHelpButton)
    EVT_GRID_RANGE_SELECT( CheckListDlg::OnRangeSelect )

END_EVENT_TABLE()

/*!
 * CheckListDlg constructors
 */

CheckListDlg::CheckListDlg( )
{
}

CheckListDlg::CheckListDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * CheckListDlg creator
 */

bool CheckListDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CheckListDlg member initialisation
////@end CheckListDlg member initialisation

////@begin CheckListDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CheckListDlg creation
    return TRUE;
}

/*!
 * Control creation for CheckListDlg
 */

void CheckListDlg::CreateControls()
{    
////@begin CheckListDlg content construction

    CheckListDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxBoxSizer* item3 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item3, 0, wxGROW, 5);

    wxButton* item4 = new wxButton;
    item4->Create( item1, CD_CHKL_DROP, _("Drop check"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    item3->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item6 = new wxButton;
    item6->Create( item1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
    wxGrid * item7 = tde->create_check_list_grid(this);
    item2->Add(item7, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end CheckListDlg content construction
    // above should be:
    //table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
    //wxGrid * item7 = tde->create_check_list_grid(this);
  SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);  // otherwise Enter in the grid cell editors focuses the button (makes it the default button)
}

/*!
 * Should we show tooltips?
 */

bool CheckListDlg::ShowToolTips()
{
    return TRUE;
}
