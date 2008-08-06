/////////////////////////////////////////////////////////////////////////////
// Name:        IndexListDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/04/04 09:19:12
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "IndexListDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "IndexListDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * IndexListDlg type definition
 */

IMPLEMENT_CLASS( IndexListDlg, wxPanel )

/*!
 * IndexListDlg event table definition
 */

BEGIN_EVENT_TABLE( IndexListDlg, wxPanel )

////@begin IndexListDlg event table entries
////@end IndexListDlg event table entries
    EVT_BUTTON( CD_TDCC_DEL, IndexListDlg::OnCdTdccDelClick )
    EVT_BUTTON( wxID_HELP, IndexListDlg::OnHelpButton)
    EVT_GRID_RANGE_SELECT( IndexListDlg::OnRangeSelect )
END_EVENT_TABLE()

/*!
 * IndexListDlg constructors
 */

IndexListDlg::IndexListDlg( )
{
}

IndexListDlg::IndexListDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * IndexListDlg creator
 */

bool IndexListDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin IndexListDlg member initialisation
////@end IndexListDlg member initialisation
#ifdef WINS  // probably not necessary on GTK, and cannot be called before Create() 
   // change the backgroud colour so that it is the same as in dialogs:
    wxVisualAttributes va = wxDialog::GetClassDefaultAttributes();
    SetBackgroundColour(va.colBg);
#endif
////@begin IndexListDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end IndexListDlg creation
    return TRUE;
}

/*!
 * Control creation for IndexListDlg
 */

void IndexListDlg::CreateControls()
{    
////@begin IndexListDlg content construction

    IndexListDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxBoxSizer* item3 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item3, 0, wxGROW, 5);

    wxButton* item4 = new wxButton;
    item4->Create( item1, CD_TDCC_DEL, _("Drop index"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    item3->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item6 = new wxButton;
    item6->Create( item1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
    wxGrid * item7 = tde->create_index_list_grid(this);
    item2->Add(item7, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end IndexListDlg content construction
    // this should be above:
    //table_designer * tde = (table_designer *)GetParent()->GetParent();  // from dialog to notebook to splitter
    //wxGrid * item7 = tde->create_index_list_grid(this);
    //item2->Add(item7, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);
  SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);  // otherwise Enter in the grid cell editors focuses the button (makes it the default button)
}

/*!
 * Should we show tooltips?
 */

bool IndexListDlg::ShowToolTips()
{
    return TRUE;
}

