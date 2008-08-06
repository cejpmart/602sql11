/////////////////////////////////////////////////////////////////////////////
// Name:        AboutDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/12/04 17:54:10
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "AboutDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "AboutDlg.h"
extern char *logo_xpm[];
#include "wx/hyperlink.h"
////@begin XPM images
////@end XPM images

/*!
 * AboutDlg type definition
 */

IMPLEMENT_CLASS( AboutDlg, wxDialog )

/*!
 * AboutDlg event table definition
 */

BEGIN_EVENT_TABLE( AboutDlg, wxDialog )

////@begin AboutDlg event table entries
////@end AboutDlg event table entries

END_EVENT_TABLE()

/*!
 * AboutDlg constructors
 */

AboutDlg::AboutDlg( )
{
}

AboutDlg::AboutDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * AboutDlg creator
 */

bool AboutDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin AboutDlg member initialisation
    mBitmap = NULL;
    mSizer = NULL;
    mMainName = NULL;
    mBuild = NULL;
    mCopyr1 = NULL;
    mCopyr2 = NULL;
    mLicenc = NULL;
////@end AboutDlg member initialisation

////@begin AboutDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end AboutDlg creation
    return TRUE;
}

/*!
 * Control creation for AboutDlg
 */

void AboutDlg::CreateControls()
{    
////@begin AboutDlg content construction
    AboutDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    wxBitmap mBitmapBitmap(wxNullBitmap);
    mBitmap = new wxStaticBitmap;
    mBitmap->Create( itemDialog1, CD_ABOUT_BITMAP, mBitmapBitmap, wxDefaultPosition, wxSize(111, 114), 0 );
    itemBoxSizer3->Add(mBitmap, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSizer = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(mSizer, 1, wxGROW|wxALL, 0);

    mMainName = new wxStaticText;
    mMainName->Create( itemDialog1, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mMainName->SetFont(wxFont(12, wxSWISS, wxNORMAL, wxNORMAL, FALSE, _T("MS Sans Serif")));
    mSizer->Add(mMainName, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    mBuild = new wxStaticText;
    mBuild->Create( itemDialog1, CD_ABOUT_BUILD, _("Build:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizer->Add(mBuild, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    mSizer->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mCopyr1 = new wxStaticText;
    mCopyr1->Create( itemDialog1, wxID_STATIC, _("Copyright Janus Drozd, Vitezslav Boukal, Vaclav Pecuch, 1992-2007"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizer->Add(mCopyr1, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    mCopyr2 = new wxStaticText;
    mCopyr2->Create( itemDialog1, wxID_STATIC, _("Copyright Software602, Inc."), wxDefaultPosition, wxDefaultSize, 0 );
    mSizer->Add(mCopyr2, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    mLicenc = new wxTextCtrl;
    mLicenc->Create( itemDialog1, CD_ABOUT_LICENC, _T(""), wxDefaultPosition, wxSize(-1, 180), wxTE_MULTILINE|wxTE_READONLY );
    itemBoxSizer2->Add(mLicenc, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_CANCEL, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton12->SetDefault();
    itemBoxSizer2->Add(itemButton12, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end AboutDlg content construction
    wxBitmap bmp(logo_xpm);
    mBitmap->SetBitmap(bmp);
    wxString name;
#if VERS_2==0
    name.Printf(wxT("602SQL %d"), VERS_1);
#else
    name.Printf(wxT("602SQL %d.%d"), VERS_1, VERS_2);
#endif
    mMainName->SetLabel(name);
    mBuild->SetLabel(wxString(_("Build:"))+wxT(" ")+wxT(VERS_STR)/*+wxT(" beta")*/);

    mLicenc->SetValue(get_licence_text(false));  // parameter value is undefined in fact
    wxString text;
#ifdef WINS
    text=mCopyr1->GetLabel();
    text.Replace(wxT("Copyright"), wxT("Copyright ©"), false);
    mCopyr1->SetLabel(text);
    text=mCopyr2->GetLabel();
    text.Replace(wxT("Copyright"), wxT("Copyright ©"), false);
    mCopyr2->SetLabel(text);
#endif
   // add the hyperlink:
    wxHyperlinkCtrl * mURL = new wxHyperlinkCtrl;
    mURL->Create(this, wxID_ANY, wxEmptyString, _("http://www.software602.com"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHL_CONTEXTMENU|wxHL_ALIGN_LEFT );
    mSizer->Add(mURL, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);
}

/*!
 * Should we show tooltips?
 */

bool AboutDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for OK_button
 */


/*!
 * Get bitmap resources
 */

wxBitmap AboutDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin AboutDlg bitmap retrieval
    return wxNullBitmap;
////@end AboutDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon AboutDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin AboutDlg icon retrieval
    return wxNullIcon;
////@end AboutDlg icon retrieval
}
