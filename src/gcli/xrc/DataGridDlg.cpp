/////////////////////////////////////////////////////////////////////////////
// Name:        DataGridDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     12/15/06 09:38:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "DataGridDlg.h"
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

#include "DataGridDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * DataGridDlg type definition
 */

IMPLEMENT_DYNAMIC_CLASS( DataGridDlg, wxDialog )

/*!
 * DataGridDlg event table definition
 */

BEGIN_EVENT_TABLE( DataGridDlg, wxDialog )

////@begin DataGridDlg event table entries
    EVT_BUTTON( wxID_CLOSE, DataGridDlg::OnCloseClick )

////@end DataGridDlg event table entries

END_EVENT_TABLE()

/*!
 * DataGridDlg constructors
 */

DataGridDlg::DataGridDlg( )
{
}

DataGridDlg::DataGridDlg(cdp_t cdpIn, wxString captIn, const char * queryIn, const char * formdefIn)
{
  cdp=cdpIn;  capt=captIn;  query=queryIn;  formdef=formdefIn;
}

/*!
 * DataGridDlg creator
 */

bool DataGridDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DataGridDlg member initialisation
    mGridSizer = NULL;
    mGrid = NULL;
////@end DataGridDlg member initialisation

////@begin DataGridDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end DataGridDlg creation
    return TRUE;
}

/*!
 * Control creation for DataGridDlg
 */

void DataGridDlg::CreateControls()
{    
////@begin DataGridDlg content construction
    DataGridDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mGridSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(mGridSizer, 1, wxGROW|wxALL, 0);

    mGrid = new wxStaticText;
    mGrid->Create( itemDialog1, CD_DGD_PLACEHOLDER, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
    mGridSizer->Add(mGrid, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton6 = new wxButton;
    itemButton6->Create( itemDialog1, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemButton6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end DataGridDlg content construction
  SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);  // otherwise Enter in the grid cell editors focuses the Close button (makes it the default button)
  SetTitle(capt);
  tcursnum cursnum;
  if (cd_SQL_execute(cdp, query, (uns32*)&cursnum))
    { cd_Signalize2(cdp, this);  return; }
  grid = new DataGrid(cdp);
  if (!grid) return;
  grid->translate_captions=true;
  if (!grid->open_form(this, cdp, formdef, cursnum, AUTO_CURSOR|CHILD_VIEW))  // deletes the grid on error, closes the cursor
    return;
  mGrid->Destroy();
  mGridSizer->Prepend(grid, 1, wxGROW|wxALL, 0);
  mGridSizer->SetItemMinSize(grid, get_grid_width(grid), get_grid_height(grid, 7));
}

/*!
 * Should we show tooltips?
 */

bool DataGridDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap DataGridDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin DataGridDlg bitmap retrieval
    return wxNullBitmap;
////@end DataGridDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon DataGridDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin DataGridDlg icon retrieval
    return wxNullIcon;
////@end DataGridDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void DataGridDlg::OnCloseClick( wxCommandEvent& event )
{
  if (grid->IsCellEditControlEnabled())
    grid->DisableCellEditControl();
  if (grid->dgt->write_changes(OWE_CONT_OR_IGNORE))
    EndModal(1);
}


