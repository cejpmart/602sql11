/////////////////////////////////////////////////////////////////////////////
// Name:        TextOutputDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/31/04 09:38:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "TextOutputDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "TextOutputDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * TextOutputDlg type definition
 */

IMPLEMENT_CLASS( TextOutputDlg, wxDialog )

/*!
 * TextOutputDlg event table definition
 */

BEGIN_EVENT_TABLE( TextOutputDlg, wxDialog )

////@begin TextOutputDlg event table entries
////@end TextOutputDlg event table entries

END_EVENT_TABLE()

/*!
 * TextOutputDlg constructors
 */

TextOutputDlg::TextOutputDlg( )
{
}

TextOutputDlg::TextOutputDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * TextOutputDlg creator
 */

bool TextOutputDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin TextOutputDlg member initialisation
    mXMLText = NULL;
////@end TextOutputDlg member initialisation

////@begin TextOutputDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end TextOutputDlg creation
    return TRUE;
}

/*!
 * Control creation for TextOutputDlg
 */

void TextOutputDlg::CreateControls()
{    
////@begin TextOutputDlg content construction

    TextOutputDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxTextCtrl* item3 = new wxTextCtrl;
    item3->Create( item1, CD_XML_TEXT, _T(""), wxDefaultPosition, wxSize(300, 300), wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL );
    mXMLText = item3;
    item2->Add(item3, 1, wxGROW|wxALL, 5);

    wxButton* item4 = new wxButton;
    item4->Create( item1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    item4->SetDefault();
    item2->Add(item4, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

////@end TextOutputDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool TextOutputDlg::ShowToolTips()
{
    return TRUE;
}
