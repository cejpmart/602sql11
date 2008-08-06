/////////////////////////////////////////////////////////////////////////////
// Name:        RestoringFileDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/18/04 09:34:39
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "RestoringFileDlg.h"
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

#include "RestoringFileDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * RestoringFileDlg type definition
 */

IMPLEMENT_CLASS( RestoringFileDlg, wxDialog )

/*!
 * RestoringFileDlg event table definition
 */

BEGIN_EVENT_TABLE( RestoringFileDlg, wxDialog )

////@begin RestoringFileDlg event table entries
////@end RestoringFileDlg event table entries

END_EVENT_TABLE()

/*!
 * RestoringFileDlg constructors
 */

RestoringFileDlg::RestoringFileDlg( )
{
}

RestoringFileDlg::RestoringFileDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * RestoringFileDlg creator
 */

bool RestoringFileDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin RestoringFileDlg member initialisation
////@end RestoringFileDlg member initialisation

////@begin RestoringFileDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end RestoringFileDlg creation
    return TRUE;
}

/*!
 * Control creation for RestoringFileDlg
 */

void RestoringFileDlg::CreateControls()
{    
////@begin RestoringFileDlg content construction
    RestoringFileDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticText* itemStaticText3 = new wxStaticText;
    itemStaticText3->Create( itemDialog1, wxID_STATIC, _("Restoring the database file from a backup will overwrite the current database file.\nAll data added or changed since the backup was created will be lost."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("The current database file will be renamed to YYMMDDHH.mmX,\nwhere YY is the year, MM is the month, and DD is the day\nHH:mm is the current time and X is the database file part."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText4, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemRadioBox5Strings[] = {
        _("&Rename"),
        _("&Erase")
    };
    wxRadioBox* itemRadioBox5 = new wxRadioBox;
    itemRadioBox5->Create( itemDialog1, ID_RADIOBOX, _("What should be done with the current database file?"), wxDefaultPosition, wxDefaultSize, 2, itemRadioBox5Strings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer2->Add(itemRadioBox5, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemDialog1, wxID_STATIC, _("Do you want to restore the database from a backup?"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton8 = new wxButton;
    itemButton8->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton8->SetDefault();
    itemBoxSizer7->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton9 = new wxButton;
    itemButton9->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    // Set validators
    itemRadioBox5->SetValidator( wxGenericValidator(& mode) );
////@end RestoringFileDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool RestoringFileDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap RestoringFileDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin RestoringFileDlg bitmap retrieval
    return wxNullBitmap;
////@end RestoringFileDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon RestoringFileDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin RestoringFileDlg icon retrieval
    return wxNullIcon;
////@end RestoringFileDlg icon retrieval
}
