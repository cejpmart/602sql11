/////////////////////////////////////////////////////////////////////////////
// Name:        ClientsConnectedDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/21/07 15:20:10
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
#include "wbvers.h"

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ClientsConnectedDlg.h"
#endif

#include "ClientsConnectedDlg.h"

////@begin includes
////@end includes


////@begin XPM images
////@end XPM images

/*!
 * ClientsConnectedDlg type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ClientsConnectedDlg, wxDialog )

/*!
 * ClientsConnectedDlg event table definition
 */

BEGIN_EVENT_TABLE( ClientsConnectedDlg, wxDialog )

////@begin ClientsConnectedDlg event table entries
    EVT_BUTTON( wxID_OK, ClientsConnectedDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, ClientsConnectedDlg::OnCancelClick )

////@end ClientsConnectedDlg event table entries

END_EVENT_TABLE()

/*!
 * ClientsConnectedDlg constructors
 */

ClientsConnectedDlg::ClientsConnectedDlg( )
{
}

ClientsConnectedDlg::ClientsConnectedDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ClientsConnectedDlg creator
 */

bool ClientsConnectedDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ClientsConnectedDlg member initialisation
    mMsg = NULL;
    mOpt = NULL;
////@end ClientsConnectedDlg member initialisation

////@begin ClientsConnectedDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ClientsConnectedDlg creation
    return TRUE;
}

/*!
 * Control creation for ClientsConnectedDlg
 */

void ClientsConnectedDlg::CreateControls()
{    
////@begin ClientsConnectedDlg content construction
    ClientsConnectedDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mMsg = new wxStaticText( itemDialog1, CD_DOWN_MSG, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(mMsg, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxString mOptStrings[] = {
        _("&Warn network clients and wait (max. 60 seconds)"),
        _("&Shutdown the SQL server immediately")
    };
    mOpt = new wxRadioBox( itemDialog1, CD_DOWN_OPT, _("Do you want to:"), wxDefaultPosition, wxDefaultSize, 2, mOptStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer2->Add(mOpt, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton6 = new wxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemButton6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton7 = new wxButton( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemButton7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ClientsConnectedDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool ClientsConnectedDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap ClientsConnectedDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ClientsConnectedDlg bitmap retrieval
    return wxNullBitmap;
////@end ClientsConnectedDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ClientsConnectedDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ClientsConnectedDlg icon retrieval
    return wxNullIcon;
////@end ClientsConnectedDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void ClientsConnectedDlg::OnOkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in ClientsConnectedDlg.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in ClientsConnectedDlg. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void ClientsConnectedDlg::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in ClientsConnectedDlg.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in ClientsConnectedDlg. 
}


