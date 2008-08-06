/////////////////////////////////////////////////////////////////////////////
// Name:        ConnSpeedDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/15/04 10:31:33
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "ConnSpeedDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "ConnSpeedDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ConnSpeedDlg type definition
 */

IMPLEMENT_CLASS( ConnSpeedDlg, wxDialog )

/*!
 * ConnSpeedDlg event table definition
 */

BEGIN_EVENT_TABLE( ConnSpeedDlg, wxDialog )

////@begin ConnSpeedDlg event table entries
////@end ConnSpeedDlg event table entries

END_EVENT_TABLE()

/*!
 * ConnSpeedDlg constructors
 */

ConnSpeedDlg::ConnSpeedDlg( int speed1In, int speed2In) : speed1(speed1In), speed2(speed2In)
{
}

ConnSpeedDlg::ConnSpeedDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ConnSpeedDlg creator
 */

bool ConnSpeedDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ConnSpeedDlg member initialisation
    mSpeed1 = NULL;
    mSpeed2 = NULL;
////@end ConnSpeedDlg member initialisation

////@begin ConnSpeedDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ConnSpeedDlg creation
    return TRUE;
}

/*!
 * Control creation for ConnSpeedDlg
 */

void ConnSpeedDlg::CreateControls()
{    
////@begin ConnSpeedDlg content construction
    ConnSpeedDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Current speed values"));
    wxStaticBoxSizer* itemStaticBoxSizer3 = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(2, 2, 0, 0);
    itemStaticBoxSizer3->Add(itemFlexGridSizer4, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("Requests/replies per second:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mSpeed1 = new wxTextCtrl;
    mSpeed1->Create( itemDialog1, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    itemFlexGridSizer4->Add(mSpeed1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("Transferred KBytes per second:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mSpeed2 = new wxTextCtrl;
    mSpeed2->Create( itemDialog1, ID_TEXTCTRL1, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    itemFlexGridSizer4->Add(mSpeed2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton9 = new wxButton;
    itemButton9->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton9->SetDefault();
    itemBoxSizer2->Add(itemButton9, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

////@end ConnSpeedDlg content construction
  wxString str;
  str.Printf(wxT("%u"), speed1);
  mSpeed1->SetValue(str);
  str.Printf(wxT("%u"), speed2);
  mSpeed2->SetValue(str);
}

/*!
 * Should we show tooltips?
 */

bool ConnSpeedDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap ConnSpeedDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ConnSpeedDlg bitmap retrieval
    return wxNullBitmap;
////@end ConnSpeedDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ConnSpeedDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ConnSpeedDlg icon retrieval
    return wxNullIcon;
////@end ConnSpeedDlg icon retrieval
}
