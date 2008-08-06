/////////////////////////////////////////////////////////////////////////////
// Name:        TableStatisticsDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/03/04 12:34:30
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "TableStatisticsDlg.h"
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

#include "TableStatisticsDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * TableStatisticsDlg type definition
 */

IMPLEMENT_CLASS( TableStatisticsDlg, wxDialog )

/*!
 * TableStatisticsDlg event table definition
 */

BEGIN_EVENT_TABLE( TableStatisticsDlg, wxDialog )

////@begin TableStatisticsDlg event table entries
////@end TableStatisticsDlg event table entries

END_EVENT_TABLE()

/*!
 * TableStatisticsDlg constructors
 */

TableStatisticsDlg::TableStatisticsDlg( )
{
}

TableStatisticsDlg::TableStatisticsDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * TableStatisticsDlg creator
 */

bool TableStatisticsDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin TableStatisticsDlg member initialisation
    mName = NULL;
    mValid = NULL;
    mDel = NULL;
    mFree = NULL;
    mTotal = NULL;
////@end TableStatisticsDlg member initialisation

////@begin TableStatisticsDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end TableStatisticsDlg creation
    return TRUE;
}

/*!
 * Control creation for TableStatisticsDlg
 */

void TableStatisticsDlg::CreateControls()
{    
////@begin TableStatisticsDlg content construction

    TableStatisticsDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxFlexGridSizer* item3 = new wxFlexGridSizer(5, 2, 0, 0);
    item3->AddGrowableCol(1);
    item2->Add(item3, 0, wxGROW|wxALL, 0);

    wxStaticText* item4 = new wxStaticText;
    item4->Create( item1, wxID_STATIC, _("Table name:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item5 = new wxTextCtrl;
    item5->Create( item1, CD_TABSTAT_NAME, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    mName = item5;
    item3->Add(item5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* item6 = new wxStaticText;
    item6->Create( item1, wxID_STATIC, _("Valid records:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item7 = new wxTextCtrl;
    item7->Create( item1, CD_TABSTAT_VALID, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    mValid = item7;
    item3->Add(item7, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* item8 = new wxStaticText;
    item8->Create( item1, wxID_STATIC, _("Deleted records:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item8, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item9 = new wxTextCtrl;
    item9->Create( item1, CD_TABSTAT_DEL, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    mDel = item9;
    item3->Add(item9, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* item10 = new wxStaticText;
    item10->Create( item1, wxID_STATIC, _("Free space:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item11 = new wxTextCtrl;
    item11->Create( item1, CD_TABSTAT_FREE, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    mFree = item11;
    item3->Add(item11, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* item12 = new wxStaticText;
    item12->Create( item1, wxID_STATIC, _("Total record space:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item13 = new wxTextCtrl;
    item13->Create( item1, CD_TABSTAT_TOTAL, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    mTotal = item13;
    item3->Add(item13, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxButton* item14 = new wxButton;
    item14->Create( item1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    item14->SetDefault();
    item2->Add(item14, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

////@end TableStatisticsDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool TableStatisticsDlg::ShowToolTips()
{
    return TRUE;
}
