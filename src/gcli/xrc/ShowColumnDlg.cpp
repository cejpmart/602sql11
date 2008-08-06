/////////////////////////////////////////////////////////////////////////////
// Name:        ShowColumnDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     08/25/05 09:26:57
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "ShowColumnDlg.h"
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

#include "ShowColumnDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ShowColumnDlg type definition
 */

IMPLEMENT_CLASS( ShowColumnDlg, wxDialog )

/*!
 * ShowColumnDlg event table definition
 */

BEGIN_EVENT_TABLE( ShowColumnDlg, wxDialog )

////@begin ShowColumnDlg event table entries
    EVT_BUTTON( wxID_OK, ShowColumnDlg::OnOkClick )

////@end ShowColumnDlg event table entries

END_EVENT_TABLE()

/*!
 * ShowColumnDlg constructors
 */

ShowColumnDlg::ShowColumnDlg(wxGrid * gridIn)
{
  grid=gridIn;
}

/*!
 * ShowColumnDlg creator
 */

bool ShowColumnDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ShowColumnDlg member initialisation
    mShowcolName = NULL;
////@end ShowColumnDlg member initialisation

////@begin ShowColumnDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ShowColumnDlg creation
    return TRUE;
}

/*!
 * Control creation for ShowColumnDlg
 */

void ShowColumnDlg::CreateControls()
{    
////@begin ShowColumnDlg content construction
    ShowColumnDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticText* itemStaticText3 = new wxStaticText;
    itemStaticText3->Create( itemDialog1, wxID_STATIC, _("Column caption:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mShowcolNameStrings = NULL;
    mShowcolName = new wxOwnerDrawnComboBox;
    mShowcolName->Create( itemDialog1, CD_SHOWCOL_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mShowcolNameStrings, wxCB_READONLY|wxCB_SORT );
    if (ShowToolTips())
        mShowcolName->SetToolTip(_("Select the column that should be made visible in the grid"));
    itemBoxSizer2->Add(mShowcolName, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton6 = new wxButton;
    itemButton6->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton6->SetDefault();
    itemBoxSizer5->Add(itemButton6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton7 = new wxButton;
    itemButton7->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemButton7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ShowColumnDlg content construction
  // fill column names:
   for (int i = 0;  i<grid->GetNumberCols();  i++)
     mShowcolName->Append(grid->GetColLabelValue(i), (void*)(size_t)i);
   int curs = mShowcolName->FindString(grid->GetColLabelValue(grid->GetGridCursorCol()));
   if (curs!=wxNOT_FOUND)  // internal error
     mShowcolName->SetSelection(curs);
}

/*!
 * Should we show tooltips?
 */

bool ShowColumnDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap ShowColumnDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ShowColumnDlg bitmap retrieval
    return wxNullBitmap;
////@end ShowColumnDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ShowColumnDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ShowColumnDlg icon retrieval
    return wxNullIcon;
////@end ShowColumnDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for xID_OK
 */

void ShowColumnDlg::OnOkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for xID_OK in ShowColumnDlg.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for xID_OK in ShowColumnDlg. 
}


