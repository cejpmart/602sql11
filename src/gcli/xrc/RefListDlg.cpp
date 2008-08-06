/////////////////////////////////////////////////////////////////////////////
// Name:        RefListDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/05/04 11:27:49
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "RefListDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "RefListDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * RefListDlg type definition
 */

IMPLEMENT_CLASS( RefListDlg, wxPanel )

/*!
 * RefListDlg event table definition
 */

BEGIN_EVENT_TABLE( RefListDlg, wxPanel )

////@begin RefListDlg event table entries
    EVT_BUTTON( CD_REFL_DROP, RefListDlg::OnCdReflDropClick )

    EVT_BUTTON( wxID_HELP, RefListDlg::OnHelpClick )

////@end RefListDlg event table entries
    EVT_GRID_RANGE_SELECT( RefListDlg::OnRangeSelect )

END_EVENT_TABLE()

/*!
 * RefListDlg constructors
 */

RefListDlg::RefListDlg( )
{
}

RefListDlg::RefListDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * RefListDlg creator
 */

bool RefListDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin RefListDlg member initialisation
////@end RefListDlg member initialisation

////@begin RefListDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end RefListDlg creation
    return TRUE;
}

/*!
 * Control creation for RefListDlg
 */

void RefListDlg::CreateControls()
{    
////@begin RefListDlg content construction

    RefListDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxBoxSizer* item3 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item3, 0, wxGROW, 5);

    wxButton* item4 = new wxButton;
    item4->Create( item1, CD_REFL_DROP, _("Drop foreign key"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    item3->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item6 = new wxButton;
    item6->Create( item1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
    wxGrid * item7 = tde->create_ref_list_grid(this);
    item2->Add(item7, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end RefListDlg content construction
   // above:
   // table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
   // wxGrid * item7 = tde->create_ref_list_grid(this);
  SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);  // otherwise Enter in the grid cell editors focuses the button (makes it the default button)
}

/*!
 * Should we show tooltips?
 */

bool RefListDlg::ShowToolTips()
{
    return TRUE;
}


