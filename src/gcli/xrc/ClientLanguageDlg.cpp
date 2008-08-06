/////////////////////////////////////////////////////////////////////////////
// Name:        ClientLanguageDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     10/01/04 09:08:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "ClientLanguageDlg.h"
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

#include "ClientLanguageDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ClientLanguageDlg type definition
 */

IMPLEMENT_CLASS( ClientLanguageDlg, wxDialog )

/*!
 * ClientLanguageDlg event table definition
 */

BEGIN_EVENT_TABLE( ClientLanguageDlg, wxDialog )

////@begin ClientLanguageDlg event table entries
    EVT_BUTTON( wxID_OK, ClientLanguageDlg::OnOkClick )

////@end ClientLanguageDlg event table entries

END_EVENT_TABLE()

/*!
 * ClientLanguageDlg constructors
 */

ClientLanguageDlg::ClientLanguageDlg( )
{
}

ClientLanguageDlg::ClientLanguageDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ClientLanguageDlg creator
 */

bool ClientLanguageDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ClientLanguageDlg member initialisation
    mLocale = NULL;
    mLoadCatalog = NULL;
////@end ClientLanguageDlg member initialisation

////@begin ClientLanguageDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ClientLanguageDlg creation
    return TRUE;
}

/*!
 * Control creation for ClientLanguageDlg
 */

void ClientLanguageDlg::CreateControls()
{    
////@begin ClientLanguageDlg content construction
    ClientLanguageDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticText* itemStaticText3 = new wxStaticText;
    itemStaticText3->Create( itemDialog1, wxID_STATIC, _("Select &locales for the client:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mLocaleStrings = NULL;
    mLocale = new wxOwnerDrawnComboBox;
    mLocale->Create( itemDialog1, CD_LOCALE_CHOICE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mLocaleStrings, wxCB_READONLY );
    itemBoxSizer2->Add(mLocale, 0, wxGROW|wxALL, 5);

    mLoadCatalog = new wxCheckBox;
    mLoadCatalog->Create( itemDialog1, CD_LOCALE_LOAD_CATALOG, _("&Translate all strings in the client"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mLoadCatalog->SetValue(FALSE);
    itemBoxSizer2->Add(mLoadCatalog, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer6, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton7 = new wxButton;
    itemButton7->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemButton7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton8 = new wxButton;
    itemButton8->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ClientLanguageDlg content construction
   // create the choice: 
    mLocale->Append(_("- System default - "), (void*)wxLANGUAGE_DEFAULT);
    mLocale->Append(_("English"),             (void*)wxLANGUAGE_ENGLISH);
    mLocale->Append(_("Czech"),               (void*)wxLANGUAGE_CZECH);
    mLocale->Append(_("Slovak"),              (void*)wxLANGUAGE_SLOVAK);
    mLocale->Append(_("Polish"),              (void*)wxLANGUAGE_POLISH);
    mLocale->Append(_("French"),              (void*)wxLANGUAGE_FRENCH);
    mLocale->Append(_("German"),              (void*)wxLANGUAGE_GERMAN);
    mLocale->Append(_("Italian"),             (void*)wxLANGUAGE_ITALIAN);
   // show current values:
    int lang, load, i;
    if (!read_profile_int("Locale", "Language", &lang)) lang=wxLANGUAGE_DEFAULT;
    load=read_profile_bool("Locale", "LoadCatalog", true);
    for (i=0;  i<mLocale->GetCount();  i++)
      if (mLocale->GetClientData(i) == (void*)(size_t)lang)
        break;
    mLocale->SetSelection(i<mLocale->GetCount() ? i : 0);  // use wxLANGUAGE_DEFAULT on error
    mLoadCatalog->SetValue(load!=0);
}

/*!
 * Should we show tooltips?
 */

bool ClientLanguageDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void ClientLanguageDlg::OnOkClick( wxCommandEvent& event )
{
 // save values:
  write_profile_int ("Locale", "Language", (int)(size_t)mLocale->GetClientData(mLocale->GetSelection()));
  write_profile_bool("Locale", "LoadCatalog", mLoadCatalog->GetValue());
 // info:
  info_box(_("The new locales and language will take effect after the client has been restarted."), this);
  event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap ClientLanguageDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ClientLanguageDlg bitmap retrieval
    return wxNullBitmap;
////@end ClientLanguageDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ClientLanguageDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ClientLanguageDlg icon retrieval
    return wxNullIcon;
////@end ClientLanguageDlg icon retrieval
}
