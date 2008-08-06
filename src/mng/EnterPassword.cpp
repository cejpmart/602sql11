/////////////////////////////////////////////////////////////////////////////
// Name:        EnterPassword.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/21/07 16:10:26
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#pragma hdrstop
#ifdef UNIX
#include <winrepl.h>
#endif
#include "wbkernel.h"
#include "mngmain.h"
#include "wbvers.h"

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "EnterPassword.h"
#endif

#include "EnterPassword.h"

////@begin XPM images
////@end XPM images

/*!
 * EnterPassword type definition
 */

IMPLEMENT_DYNAMIC_CLASS( EnterPassword, wxDialog )

/*!
 * EnterPassword event table definition
 */

BEGIN_EVENT_TABLE( EnterPassword, wxDialog )

////@begin EnterPassword event table entries
    EVT_BUTTON( wxID_OK, EnterPassword::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, EnterPassword::OnCancelClick )

////@end EnterPassword event table entries

END_EVENT_TABLE()

/*!
 * EnterPassword constructors
 */

EnterPassword::EnterPassword( )
{
}

EnterPassword::EnterPassword( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * EnterPassword creator
 */

bool EnterPassword::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin EnterPassword member initialisation
    mPass = NULL;
////@end EnterPassword member initialisation

////@begin EnterPassword creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end EnterPassword creation
    return TRUE;
}

/*!
 * Control creation for EnterPassword
 */

void EnterPassword::CreateControls()
{    
////@begin EnterPassword content construction
    EnterPassword* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticText* itemStaticText3 = new wxStaticText( itemDialog1, wxID_STATIC, _("If the database file is encrypted the SQL server needs the password."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText5 = new wxStaticText( itemDialog1, wxID_STATIC, _("Enter the password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPass = new wxTextCtrl( itemDialog1, CD_PASS_PASS, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemBoxSizer4->Add(mPass, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton8 = new wxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton8->SetDefault();
    itemBoxSizer7->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton9 = new wxButton( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end EnterPassword content construction
  mPass->SetMaxLength(MAX_FIL_PASSWORD);
  mPass->SetFocus();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void EnterPassword::OnOkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in EnterPassword.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in EnterPassword. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void EnterPassword::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in EnterPassword.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in EnterPassword. 
}

/*!
 * Should we show tooltips?
 */

bool EnterPassword::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap EnterPassword::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin EnterPassword bitmap retrieval
    return wxNullBitmap;
////@end EnterPassword bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon EnterPassword::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin EnterPassword icon retrieval
    return wxNullIcon;
////@end EnterPassword icon retrieval
}
