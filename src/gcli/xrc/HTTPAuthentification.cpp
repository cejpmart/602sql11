/////////////////////////////////////////////////////////////////////////////
// Name:        HTTPAuthentification.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/29/06 12:12:50
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "HTTPAuthentification.h"
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

#include "HTTPAuthentification.h"

////@begin XPM images
////@end XPM images

/*!
 * HTTPAuthentification type definition
 */

IMPLEMENT_CLASS( HTTPAuthentification, wxDialog )

/*!
 * HTTPAuthentification event table definition
 */

BEGIN_EVENT_TABLE( HTTPAuthentification, wxDialog )

////@begin HTTPAuthentification event table entries
    EVT_BUTTON( wxID_OK, HTTPAuthentification::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, HTTPAuthentification::OnCancelClick )

////@end HTTPAuthentification event table entries

END_EVENT_TABLE()

/*!
 * HTTPAuthentification constructors
 */

HTTPAuthentification::HTTPAuthentification(bool againIn)
{ again=againIn; }

HTTPAuthentification::HTTPAuthentification( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * HTTPAuthentification creator
 */

bool HTTPAuthentification::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin HTTPAuthentification member initialisation
    mName = NULL;
    mPassword = NULL;
////@end HTTPAuthentification member initialisation

////@begin HTTPAuthentification creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end HTTPAuthentification creation
    return TRUE;
}

/*!
 * Control creation for HTTPAuthentification
 */

void HTTPAuthentification::CreateControls()
{    
////@begin HTTPAuthentification content construction
    HTTPAuthentification* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("User name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mName = new wxTextCtrl;
    mName->Create( itemDialog1, CD_HTTPA_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemDialog1, wxID_STATIC, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPassword = new wxTextCtrl;
    mPassword->Create( itemDialog1, CD_HTTPA_PASSWORD, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemFlexGridSizer3->Add(mPassword, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer8, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton9 = new wxButton;
    itemButton9->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton9->SetDefault();
    itemBoxSizer8->Add(itemButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton10 = new wxButton;
    itemButton10->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemButton10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end HTTPAuthentification content construction
    if (again) SetTitle(_("Not authorized - try again"));
}

/*!
 * Should we show tooltips?
 */

bool HTTPAuthentification::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap HTTPAuthentification::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin HTTPAuthentification bitmap retrieval
    return wxNullBitmap;
////@end HTTPAuthentification bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon HTTPAuthentification::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin HTTPAuthentification icon retrieval
    return wxNullIcon;
////@end HTTPAuthentification icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void HTTPAuthentification::OnOkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in HTTPAuthentification.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in HTTPAuthentification. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void HTTPAuthentification::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in HTTPAuthentification.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in HTTPAuthentification. 
}


