/////////////////////////////////////////////////////////////////////////////
// Name:        FtxDsgn.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/07/05 09:59:19
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "FtxDsgn.h"
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

#include "FtxDsgn.h"

////@begin XPM images
////@end XPM images

/*!
 * FtxDsgn type definition
 */

IMPLEMENT_CLASS( FtxDsgn, wxPanel )

/*!
 * FtxDsgn event table definition
 */

BEGIN_EVENT_TABLE( FtxDsgn, wxPanel )

////@begin FtxDsgn event table entries
    EVT_TEXT( CD_FTD_LIMITS, FtxDsgn::OnCdFtdLimitsUpdated )

////@end FtxDsgn event table entries

END_EVENT_TABLE()

/*!
 * FtxDsgn constructors
 */

FtxDsgn::FtxDsgn(class fulltext_designer * ftdsgnIn)
{ ftdsgn=ftdsgnIn;
}

/*!
 * FtxDsgn creator
 */

bool FtxDsgn::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin FtxDsgn member initialisation
    mLang = NULL;
    mLemma = NULL;
    mSubstruct = NULL;
    mExternal = NULL;
    mGrid = NULL;
    mLimits = NULL;
    mReindex = NULL;
////@end FtxDsgn member initialisation

////@begin FtxDsgn creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end FtxDsgn creation
    return TRUE;
}

/*!
 * Control creation for FtxDsgn
 */

void FtxDsgn::CreateControls()
{    
////@begin FtxDsgn content construction
    FtxDsgn* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemPanel1, wxID_STATIC, _("Language:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLang = new wxStaticText;
    mLang->Create( itemPanel1, CD_FTD_LANG, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(mLang, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLemma = new wxStaticText;
    mLemma->Create( itemPanel1, CD_FTD_LEMMA, _("Lemmatized"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(mLemma, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mSubstruct = new wxStaticText;
    mSubstruct->Create( itemPanel1, CD_FTD_SUBSTRUCT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(mSubstruct, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mExternal = new wxStaticText;
    mExternal->Create( itemPanel1, CD_FTD_EXTERNAL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(mExternal, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mGrid = new wxGrid( itemPanel1, CD_FTD_GRID, wxDefaultPosition, wxSize(200, 150), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mGrid->SetDefaultColSize(50);
    mGrid->SetDefaultRowSize(25);
    mGrid->SetColLabelSize(25);
    mGrid->SetRowLabelSize(50);
    mGrid->CreateGrid(5, 5, wxGrid::wxGridSelectCells);
    itemBoxSizer2->Add(mGrid, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer10, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText11 = new wxStaticText;
    itemStaticText11->Create( itemPanel1, wxID_STATIC, _("File limits:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemStaticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLimits = new wxTextCtrl;
    mLimits->Create( itemPanel1, CD_FTD_LIMITS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(mLimits, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer13, 0, wxGROW|wxALL, 0);

    mReindex = new wxButton;
    mReindex->Create( itemPanel1, CD_FTD_REINDEX, _("Reindex documents in the table(s)"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        mReindex->SetToolTip(_("Reindex all documents on the selected rows"));
    itemBoxSizer13->Add(mReindex, 0, wxALIGN_LEFT|wxALL, 5);

////@end FtxDsgn content construction
}

/*!
 * Should we show tooltips?
 */

bool FtxDsgn::ShowToolTips()
{
    return TRUE;
}











/*!
 * Get bitmap resources
 */

wxBitmap FtxDsgn::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin FtxDsgn bitmap retrieval
    return wxNullBitmap;
////@end FtxDsgn bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon FtxDsgn::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin FtxDsgn icon retrieval
    return wxNullIcon;
////@end FtxDsgn icon retrieval
}
/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_FTD_LIMITS
 */

void FtxDsgn::OnCdFtdLimitsUpdated( wxCommandEvent& event )
// This event handler is not called. It must be in the derived class: fulltext_designer
{
  ftdsgn->changed=true;  
  event.Skip();
}


