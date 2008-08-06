/////////////////////////////////////////////////////////////////////////////
// Name:        delcateg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/14/04 14:32:32
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
////@begin includes
#include "wx/wx.h"
////@end includes
#endif
#ifndef WINS
#include "winrepl.h"
#endif
#include "cint.h"
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop
#include "impobjconfl.h"
#include "delcateg.h"

#ifdef __GNUG__
//#pragma implementation "delcateg.h"
#endif

////@begin XPM images
////@end XPM images

/*!
 * CDelCateg type definition
 */

IMPLEMENT_CLASS( CDelCateg, wxDialog )

/*!
 * CDelCateg event table definition
 */

BEGIN_EVENT_TABLE( CDelCateg, wxDialog )

////@begin CDelCateg event table entries
    EVT_BUTTON( wxID_OK, CDelCateg::OnOKClick )

    EVT_BUTTON( wxID_CANCEL, CDelCateg::OnCancelClick )

////@end CDelCateg event table entries

END_EVENT_TABLE()

/*!
 * CDelCateg constructors
 */

CDelCateg::CDelCateg( )
{
}

CDelCateg::CDelCateg(cdp_t cdp, CCateg Categ, const wxChar *Folder)
{
    wchar_t wc[64];
    if (Folder)
    {
        m_CategObjs   = wxString::Format(_("Delete &all %s in folder \"%s\""), CCategList::Plural(cdp, Categ, wc), Folder);
        m_WholeFolder = wxString::Format(_("Delete &folder \"%s\" and all objects it contains"), Folder);
    }
    else
    {
        m_CategObjs   = wxString::Format(_("Delete &all %s in the selected folders"), CCategList::Plural(cdp, Categ, wc));
        m_WholeFolder = _("Delete selected &folders and all objects it contains");
    }
    Create(NULL);
}

/*!
 * CDelCateg creator
 */

bool CDelCateg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CDelCateg member initialisation
    RadioBox = NULL;
////@end CDelCateg member initialisation

////@begin CDelCateg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CDelCateg creation
    return TRUE;
}

/*!
 * Control creation for CDelCateg
 */

void CDelCateg::CreateControls()
{    
    ////@begin CDelCateg content construction

    CDelCateg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);
    wxFlexGridSizer* item3 = new wxFlexGridSizer(2, 2, 0, 0);
    item2->Add(item3, 0, wxGROW|wxALL, 5);
    wxBitmap item4Bitmap(question_xpm);
    wxStaticBitmap* item4 = new wxStaticBitmap( item1, wxID_STATIC, item4Bitmap, wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxString item5Strings[] = {m_CategObjs, m_WholeFolder};
    wxRadioBox* item5 = new wxRadioBox( item1, ID_RADIO, _("Do you want to "), wxDefaultPosition, wxDefaultSize, 2, item5Strings, 1, wxRA_SPECIFY_COLS);
    RadioBox = item5;
    item3->Add(item5, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxBoxSizer* item6 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item6, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    wxButton* item7 = new wxButton( item1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item6->Add(item7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxButton* item8 = new wxButton( item1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item8->SetDefault();
    item6->Add(item8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
////@end CDelCateg content construction
}

/*!
 * Should we show tooltips?
 */

bool CDelCateg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_YES
 */

void CDelCateg::OnOKClick( wxCommandEvent& event )
{
    // Insert custom code here
    EndModal(RadioBox->GetSelection());
    event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void CDelCateg::OnCancelClick( wxCommandEvent& event )
{
    // Insert custom code here
    EndModal(wxID_CANCEL);
    event.Skip();
}
