/////////////////////////////////////////////////////////////////////////////
// Name:        MoveColValueDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/03/04 10:30:07
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "MoveColValueDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "MoveColValueDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * MoveColValueDlg type definition
 */

IMPLEMENT_CLASS( MoveColValueDlg, wxScrolledWindow )

/*!
 * MoveColValueDlg event table definition
 */

BEGIN_EVENT_TABLE( MoveColValueDlg, wxScrolledWindow )

////@begin MoveColValueDlg event table entries
    EVT_RADIOBOX( CD_TBUP_RADIO, MoveColValueDlg::OnCdTbupRadioSelected )

////@end MoveColValueDlg event table entries

END_EVENT_TABLE()

/*!
 * MoveColValueDlg constructors
 */

MoveColValueDlg::MoveColValueDlg( )
{
}

MoveColValueDlg::MoveColValueDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * MoveColValueDlg creator
 */

bool MoveColValueDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MoveColValueDlg member initialisation
    mCapt = NULL;
    mRadio = NULL;
    mValCapt = NULL;
    mVal = NULL;
    mListCapt = NULL;
    mList = NULL;
////@end MoveColValueDlg member initialisation
#ifdef WINS  // probably not necessary on GTK, and cannot be called before Create() 
   // change the backgroud colour so that it is the same as in dialogs:
    wxVisualAttributes va = wxDialog::GetClassDefaultAttributes();
    SetBackgroundColour(va.colBg);
#endif
////@begin MoveColValueDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxScrolledWindow::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end MoveColValueDlg creation
   // start the functionality of scrollbars:
    FitInside();  
    SetScrollRate(8,8);
    return TRUE;
}

/*!
 * Control creation for MoveColValueDlg
 */

void MoveColValueDlg::CreateControls()
{    
////@begin MoveColValueDlg content construction

    MoveColValueDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxStaticText* item3 = new wxStaticText;
    item3->Create( item1, CD_TBUP_CAPT, _("Value of the column in the edited table:"), wxDefaultPosition, wxSize(300, -1), 0 );
    mCapt = item3;
    item2->Add(item3, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxString item4Strings[] = {
        _("&The default value as specified for the column (NULL if not specified)"),
        _("&The value of the expression specified below")
    };
    wxRadioBox* item4 = new wxRadioBox;
    item4->Create( item1, CD_TBUP_RADIO, _("Value of the column in the edited table:"), wxDefaultPosition, wxDefaultSize, 2, item4Strings, 1, wxRA_SPECIFY_COLS );
    mRadio = item4;
    item2->Add(item4, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* item5 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item5, 1, wxALIGN_LEFT|wxLEFT|wxRIGHT, 5);

    item5->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* item7 = new wxBoxSizer(wxVERTICAL);
    item5->Add(item7, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* item8 = new wxStaticText;
    item8->Create( item1, wxID_STATIC, _("Value of the column:"), wxDefaultPosition, wxDefaultSize, 0 );
    mValCapt = item8;
    item7->Add(item8, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item9 = new wxTextCtrl;
    item9->Create( item1, CD_TBUP_VAL, _T(""), wxDefaultPosition, wxSize(280, -1), 0 );
    mVal = item9;
    item7->Add(item9, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* item10 = new wxStaticText;
    item10->Create( item1, CD_TBUP_LIST_CAPT, _("The expression can refer to columns from the original table:"), wxDefaultPosition, wxDefaultSize, 0 );
    mListCapt = item10;
    item7->Add(item10, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* item11Strings = NULL;
    wxListBox* item11 = new wxListBox;
    item11->Create( item1, CD_TBUP_LIST, wxDefaultPosition, wxSize(150, -1), 0, item11Strings, wxLB_SORT );
    mList = item11;
    item7->Add(item11, 1, wxALIGN_LEFT|wxLEFT|wxRIGHT, 5);

////@end MoveColValueDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool MoveColValueDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_TBUP_DEF
 */

/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TBUP_RADIO
 */

void MoveColValueDlg::OnCdTbupRadioSelected( wxCommandEvent& event )
{
  mVal    ->Enable(mRadio->GetSelection()==1);
  mValCapt->Enable(mRadio->GetSelection()==1);
  event.Skip();
}


