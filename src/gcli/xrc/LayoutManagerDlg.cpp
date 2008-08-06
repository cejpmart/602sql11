/////////////////////////////////////////////////////////////////////////////
// Name:        LayoutManagerDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/14/04 10:00:13
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "LayoutManagerDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "LayoutManagerDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * LayoutManagerDlg type definition
 */

IMPLEMENT_CLASS( LayoutManagerDlg, wxDialog )

/*!
 * LayoutManagerDlg event table definition
 */

BEGIN_EVENT_TABLE( LayoutManagerDlg, wxDialog )

////@begin LayoutManagerDlg event table entries
    EVT_LISTBOX( CD_FL_LIST, LayoutManagerDlg::OnCdFlListSelected )

    EVT_RADIOBUTTON( CD_FL_TOP, LayoutManagerDlg::OnCdFlTopSelected )

    EVT_RADIOBUTTON( CD_FL_LEFT, LayoutManagerDlg::OnCdFlLeftSelected )

    EVT_RADIOBUTTON( CD_FL_RIGHT, LayoutManagerDlg::OnCdFlRightSelected )

    EVT_RADIOBUTTON( CD_FL_BOTTOM, LayoutManagerDlg::OnCdFlBottomSelected )

    EVT_RADIOBUTTON( CD_FL_FLOATING, LayoutManagerDlg::OnCdFlFloatingSelected )

    EVT_RADIOBUTTON( CD_FL_HIDDEN, LayoutManagerDlg::OnCdFlHiddenSelected )

    EVT_BUTTON( wxID_CANCEL, LayoutManagerDlg::OnCancelClick )

    EVT_BUTTON( CD_FL_RESTORE, LayoutManagerDlg::OnCdFlRestoreClick )

////@end LayoutManagerDlg event table entries

END_EVENT_TABLE()

/*!
 * LayoutManagerDlg constructors
 */

LayoutManagerDlg::LayoutManagerDlg( )
{ frame=wxGetApp().frame; }

/*!
 * LayoutManagerDlg creator
 */

bool LayoutManagerDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin LayoutManagerDlg member initialisation
    mList = NULL;
    mTop = NULL;
    mLeft = NULL;
    mRight = NULL;
    mBottom = NULL;
    mFloating = NULL;
    mHidden = NULL;
////@end LayoutManagerDlg member initialisation

////@begin LayoutManagerDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end LayoutManagerDlg creation
    return TRUE;
}

/*!
 * Control creation for LayoutManagerDlg
 */

void LayoutManagerDlg::CreateControls()
{    
////@begin LayoutManagerDlg content construction

    LayoutManagerDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxBoxSizer* item3 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item3, 0, wxGROW|wxALL, 0);

    wxString* item4Strings = NULL;
    wxListBox* item4 = new wxListBox;
    item4->Create( item1, CD_FL_LIST, wxDefaultPosition, wxSize(150, -1), 0, item4Strings, wxLB_SINGLE );
    mList = item4;
    item3->Add(item4, 1, wxGROW|wxALL, 5);

    wxBoxSizer* item5 = new wxBoxSizer(wxVERTICAL);
    item3->Add(item5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxRadioButton* item6 = new wxRadioButton;
    item6->Create( item1, CD_FL_TOP, _("Dock at top"), wxDefaultPosition, wxDefaultSize, 0 );
    mTop = item6;
    item6->SetValue(FALSE);
    item5->Add(item6, 0, wxALIGN_LEFT|wxALL, 5);

    wxRadioButton* item7 = new wxRadioButton;
    item7->Create( item1, CD_FL_LEFT, _("Dock left"), wxDefaultPosition, wxDefaultSize, 0 );
    mLeft = item7;
    item7->SetValue(FALSE);
    item5->Add(item7, 0, wxALIGN_LEFT|wxALL, 5);

    wxRadioButton* item8 = new wxRadioButton;
    item8->Create( item1, CD_FL_RIGHT, _("Dock right"), wxDefaultPosition, wxDefaultSize, 0 );
    mRight = item8;
    item8->SetValue(FALSE);
    item5->Add(item8, 0, wxALIGN_LEFT|wxALL, 5);

    wxRadioButton* item9 = new wxRadioButton;
    item9->Create( item1, CD_FL_BOTTOM, _("Dock at bottom"), wxDefaultPosition, wxDefaultSize, 0 );
    mBottom = item9;
    item9->SetValue(FALSE);
    item5->Add(item9, 0, wxALIGN_LEFT|wxALL, 5);

    wxRadioButton* item10 = new wxRadioButton;
    item10->Create( item1, CD_FL_FLOATING, _("Floating"), wxDefaultPosition, wxDefaultSize, 0 );
    mFloating = item10;
    item10->SetValue(FALSE);
    item5->Add(item10, 0, wxALIGN_LEFT|wxALL, 5);

    wxRadioButton* item11 = new wxRadioButton;
    item11->Create( item1, CD_FL_HIDDEN, _("Hidden"), wxDefaultPosition, wxDefaultSize, 0 );
    mHidden = item11;
    item11->SetValue(FALSE);
    item5->Add(item11, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* item12 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* item13 = new wxButton;
    item13->Create( item1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    item13->SetDefault();
    item12->Add(item13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item14 = new wxButton;
    item14->Create( item1, CD_FL_RESTORE, _("Restore the default layout"), wxDefaultPosition, wxDefaultSize, 0 );
    item12->Add(item14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end LayoutManagerDlg content construction
    for (int i=0;  i</*BAR_COUNT*/BAR_ID_FIRST_TOOLBAR;  i++)
      if (the_server_debugger || i!=BAR_ID_CALLS && i!=BAR_ID_WATCH)
        mList->Append(bar_name[i]);
    mFloating->Enable(frame->mpLayout->CanReparent());
    mList->SetSelection(0);  // prevents the "no selection" state
    wxCommandEvent cmd;
    OnCdFlListSelected(cmd);
}

/*!
 * Should we show tooltips?
 */

bool LayoutManagerDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void LayoutManagerDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();   // close
}


/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX
 */

void LayoutManagerDlg::OnCdFlListSelected( wxCommandEvent& event )
{ mLeft->Enable(mList->GetSelection()<BAR_ID_FIRST_TOOLBAR);
  mRight->Enable(mList->GetSelection()<BAR_ID_FIRST_TOOLBAR);
  cbBarInfo * bar = frame->mpLayout->FindBarByWindow(frame->bars[mList->GetSelection()]);
  if (!bar) mHidden->SetValue(true);
  else
    if      (bar->mState == wxCBAR_HIDDEN  ) mHidden->SetValue(true);
    else if (bar->mState == wxCBAR_FLOATING) mFloating->SetValue(true);
    else if (bar->mState == wxCBAR_DOCKED_HORIZONTALLY)
      if (bar->mAlignment == FL_ALIGN_TOP) mTop->SetValue(true);
      else mBottom->SetValue(true);
    else
      if (bar->mAlignment == FL_ALIGN_LEFT) mLeft->SetValue(true);
      else mRight->SetValue(true);
    event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON
 */

void LayoutManagerDlg::OnCdFlTopSelected( wxCommandEvent& event )
{ cbBarInfo * bar = frame->mpLayout->FindBarByWindow(frame->bars[mList->GetSelection()]);
  if (!bar) return;
  if (bar->mState == wxCBAR_HIDDEN || bar->mState == wxCBAR_FLOATING)
    frame->mpLayout->SetBarState(bar, wxCBAR_DOCKED_HORIZONTALLY, TRUE );
  frame->mpLayout->RedockBar(bar, frame->bars[mList->GetSelection()]->GetRect(), frame->mpLayout->GetPane(FL_ALIGN_TOP), true);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON1
 */

void LayoutManagerDlg::OnCdFlLeftSelected( wxCommandEvent& event )
{ cbBarInfo * bar = frame->mpLayout->FindBarByWindow(frame->bars[mList->GetSelection()]);
  if (bar->mState == wxCBAR_HIDDEN || bar->mState == wxCBAR_FLOATING)
    frame->mpLayout->SetBarState(bar, wxCBAR_DOCKED_VERTICALLY, TRUE );
  frame->mpLayout->RedockBar(bar, frame->bars[mList->GetSelection()]->GetRect(), frame->mpLayout->GetPane(FL_ALIGN_LEFT), true);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON2
 */

void LayoutManagerDlg::OnCdFlRightSelected( wxCommandEvent& event )
{ cbBarInfo * bar = frame->mpLayout->FindBarByWindow(frame->bars[mList->GetSelection()]);
  if (bar->mState == wxCBAR_HIDDEN || bar->mState == wxCBAR_FLOATING)
    frame->mpLayout->SetBarState(bar, wxCBAR_DOCKED_VERTICALLY, TRUE );
  frame->mpLayout->RedockBar(bar, frame->bars[mList->GetSelection()]->GetRect(), frame->mpLayout->GetPane(FL_ALIGN_RIGHT), true);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON3
 */

void LayoutManagerDlg::OnCdFlBottomSelected( wxCommandEvent& event )
{ cbBarInfo * bar = frame->mpLayout->FindBarByWindow(frame->bars[mList->GetSelection()]);
  if (!bar) return;
  if (bar->mState == wxCBAR_HIDDEN || bar->mState == wxCBAR_FLOATING)
    frame->mpLayout->SetBarState(bar, wxCBAR_DOCKED_HORIZONTALLY, TRUE );
  frame->mpLayout->RedockBar(bar, frame->bars[mList->GetSelection()]->GetRect(), frame->mpLayout->GetPane(FL_ALIGN_BOTTOM), true);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON4
 */

void LayoutManagerDlg::OnCdFlFloatingSelected( wxCommandEvent& event )
{ cbBarInfo * bar = frame->mpLayout->FindBarByWindow(frame->bars[mList->GetSelection()]);
  if (!bar) return;
  if (bar->mState == wxCBAR_FLOATING)
    return;  // otherwise opens again 
  frame->mpLayout->SetBarState(bar, wxCBAR_FLOATING, TRUE);
	frame->mpLayout->RepositionFloatedBar(bar); 
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON5
 */

void LayoutManagerDlg::OnCdFlHiddenSelected( wxCommandEvent& event )
{ cbBarInfo * bar = frame->mpLayout->FindBarByWindow(frame->bars[mList->GetSelection()]);
  if (!bar) return;
  frame->mpLayout->SetBarState(bar, wxCBAR_HIDDEN, TRUE);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FL_RESTORE
 */

void LayoutManagerDlg::OnCdFlRestoreClick( wxCommandEvent& event )
{
  restore_default_request=true;
  EndModal(123);
}


