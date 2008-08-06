/////////////////////////////////////////////////////////////////////////////
// Name:        DatabaseConsistencyDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/26/04 12:46:48
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "DatabaseConsistencyDlg.h"
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

#include "DatabaseConsistencyDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * DatabaseConsistencyDlg type definition
 */

IMPLEMENT_CLASS( DatabaseConsistencyDlg, wxDialog )

/*!
 * DatabaseConsistencyDlg event table definition
 */

BEGIN_EVENT_TABLE( DatabaseConsistencyDlg, wxDialog )

////@begin DatabaseConsistencyDlg event table entries
    EVT_CHECKBOX( CD_DBINT_INDEX, DatabaseConsistencyDlg::OnCdDbintIndexClick )

    EVT_CHECKBOX( CD_DBINT_CACHE, DatabaseConsistencyDlg::OnCdDbintCacheClick )

    EVT_CHECKBOX( CD_DBINT_REPAIR, DatabaseConsistencyDlg::OnCdDbintRepairClick )

    EVT_BUTTON( wxID_OK, DatabaseConsistencyDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, DatabaseConsistencyDlg::OnCancelClick )

////@end DatabaseConsistencyDlg event table entries
    EVT_IDLE(DatabaseConsistencyDlg::OnIdle)
END_EVENT_TABLE()

/*!
 * DatabaseConsistencyDlg constructors
 */

DatabaseConsistencyDlg::DatabaseConsistencyDlg(cdp_t cdpIn)
{ cdp=cdpIn;  phase=0; }

/*!
 * DatabaseConsistencyDlg creator
 */

bool DatabaseConsistencyDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DatabaseConsistencyDlg member initialisation
    mErrCapt = NULL;
    mLost = NULL;
    mLostErr = NULL;
    mCross = NULL;
    mCrossErr = NULL;
    mDamaged = NULL;
    mDamagedErr = NULL;
    mNonex = NULL;
    mNonexErr = NULL;
    mIndex = NULL;
    mIndexErr = NULL;
    mCache = NULL;
    mCacheErr = NULL;
    mRepair = NULL;
    mStart = NULL;
    mClose = NULL;
////@end DatabaseConsistencyDlg member initialisation

////@begin DatabaseConsistencyDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end DatabaseConsistencyDlg creation
    return TRUE;
}

/*!
 * Control creation for DatabaseConsistencyDlg
 */

void DatabaseConsistencyDlg::CreateControls()
{    
////@begin DatabaseConsistencyDlg content construction
    DatabaseConsistencyDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("Check type:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mErrCapt = new wxStaticText;
    mErrCapt->Create( itemDialog1, CD_DBINT_ERRCAPT, _("Errors found:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mErrCapt, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLost = new wxCheckBox;
    mLost->Create( itemDialog1, CD_DBINT_LOST, _("Lost disk space"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mLost->SetValue(TRUE);
    itemFlexGridSizer3->Add(mLost, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mLostErr = new wxStaticText;
    mLostErr->Create( itemDialog1, CD_DBINT_LOSTERR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mLostErr, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mCross = new wxCheckBox;
    mCross->Create( itemDialog1, CD_DBINT_CROSS, _("Cross-linked disk space"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mCross->SetValue(TRUE);
    itemFlexGridSizer3->Add(mCross, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mCrossErr = new wxStaticText;
    mCrossErr->Create( itemDialog1, CD_DBINT_CROSSERR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mCrossErr, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mDamaged = new wxCheckBox;
    mDamaged->Create( itemDialog1, CD_DBINT_DAMAGED, _("Damaged tables"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mDamaged->SetValue(TRUE);
    itemFlexGridSizer3->Add(mDamaged, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mDamagedErr = new wxStaticText;
    mDamagedErr->Create( itemDialog1, CD_DBINT_DAMAGEDERR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mDamagedErr, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mNonex = new wxCheckBox;
    mNonex->Create( itemDialog1, CD_DBINT_NONEX, _("Non-existing disk blocks"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mNonex->SetValue(TRUE);
    itemFlexGridSizer3->Add(mNonex, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mNonexErr = new wxStaticText;
    mNonexErr->Create( itemDialog1, CD_DBINT_NONEXERR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mNonexErr, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mIndex = new wxCheckBox;
    mIndex->Create( itemDialog1, CD_DBINT_INDEX, _("Outdated index"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mIndex->SetValue(TRUE);
    itemFlexGridSizer3->Add(mIndex, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mIndexErr = new wxStaticText;
    mIndexErr->Create( itemDialog1, CD_DBINT_INDEXERR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mIndexErr, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mCache = new wxCheckBox;
    mCache->Create( itemDialog1, CD_DBINT_CACHE, _("Invalid free record cache"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mCache->SetValue(TRUE);
    itemFlexGridSizer3->Add(mCache, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mCacheErr = new wxStaticText;
    mCacheErr->Create( itemDialog1, CD_DBINT_CACHEERR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mCacheErr, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticLine* itemStaticLine18 = new wxStaticLine;
    itemStaticLine18->Create( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer2->Add(itemStaticLine18, 0, wxGROW|wxALL, 5);

    mRepair = new wxCheckBox;
    mRepair->Create( itemDialog1, CD_DBINT_REPAIR, _("Repair errors"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mRepair->SetValue(FALSE);
    itemBoxSizer2->Add(mRepair, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer20, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mStart = new wxButton;
    mStart->Create( itemDialog1, wxID_OK, _("Start check"), wxDefaultPosition, wxDefaultSize, 0 );
    mStart->SetDefault();
    itemBoxSizer20->Add(mStart, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mClose = new wxButton;
    mClose->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(mClose, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end DatabaseConsistencyDlg content construction
  mStart->Enable(cd_Am_I_config_admin(cdp)!=FALSE);
}

/*!
 * Should we show tooltips?
 */

bool DatabaseConsistencyDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void int_label(wxStaticText * ctrl, int val)
{ wxString str;
  str.Printf(wxT("%d"), val);
  ctrl->SetLabel(str);
}

void DatabaseConsistencyDlg::display1(void)
{ if (mLost->GetValue())
  { wxString str;
    str.Printf(wxT("%d+%d"), lost_blocks, lost_dheap);
    mLostErr->SetLabel(str);
  }
  if (mCross->GetValue()) int_label(mCrossErr, cross_link);
  if (mNonex->GetValue()) int_label(mNonexErr, nonex_blocks);
  if (mDamaged->GetValue()) int_label(mDamagedErr, damaged_tabdef);
}

void DatabaseConsistencyDlg::display2(void)
{ if (mIndex->GetValue()) int_label(mIndexErr, index_err);
  if (mCache->GetValue()) int_label(mCacheErr, cache_err);
}

#include "netapi.h"

void DatabaseConsistencyDlg::OnOkClick( wxCommandEvent& event )
// Never closes the dialog
{ 
  BOOL res;
  mClose->SetLabel(_("Cancel"));
  mErrCapt->SetLabel(mRepair->GetValue() ? _("Errors repaired:") : _("Errors found:"));
  mLostErr->SetLabel(wxEmptyString);
  mCrossErr->SetLabel(wxEmptyString);
  mDamagedErr->SetLabel(wxEmptyString);
  mNonexErr->SetLabel(wxEmptyString);
  mIndexErr->SetLabel(wxEmptyString);
  mCacheErr->SetLabel(wxEmptyString);
  mStart->Disable();
 // disable controls:
  mLost->Disable();
  mNonex->Disable();
  mCross->Disable();
  mDamaged->Disable();
  mIndex->Disable();
  mCache->Disable();
  mStart->Disable();
  mRepair->Disable();
  Layout();  Fit();   // mErrCapt label size changed, update the layout and the dialog size
 // close hidden cursors: 
  if (wxGetApp().frame->output_window)
  { wxPanel * data_panel = (wxPanel*)wxGetApp().frame->output_window->GetPage(DATA_PAGE_NUM);
    if (data_panel)
    { wxWindowList & chlist = data_panel->GetChildren();
      if (!chlist.IsEmpty())
      { DataGrid * grid = (DataGrid*)chlist.GetFirst()->GetData();
        ServerDisconnectingEvent disev(cdp);
        grid->OnDisconnecting(disev);
        if (!disev.ignored) grid->Destroy();
      }
    }
  }
  wxGetApp().frame->monitor_window->Stop_monitoring_server(cdp);  // otherwise open cursors prevent performing the check
  bool concurrent = cdp->pRemAddr && !cdp->pRemAddr->is_http && !cd_concurrent(cdp, TRUE);  // not using concurrency when HTTP tunnelling is active because the requests cannot be breaked
  extent = 0;
  if (mIndex->GetValue()) extent |= INTEGR_CHECK_INDEXES;
  if (mCache->GetValue()) extent |= INTEGR_CHECK_CACHE;
 // basic integrity:
  wxBeginBusyCursor();  // does not work well in the concurrent case
  mClose->Disable();
  res=cd_Database_integrity(cdp, mRepair->GetValue(),
  mLost->GetValue()    ? &lost_blocks   :NULL,
  mLost->GetValue()    ? &lost_dheap    :NULL,
  mNonex->GetValue()   ? &nonex_blocks  :NULL,
  mCross->GetValue()   ? &cross_link    :NULL,
  mDamaged->GetValue() ? &damaged_tabdef:NULL);
  if (concurrent)
  { phase=1;
    mClose->Enable();
  }
  else
  { wxEndBusyCursor();
    if (res) 
      if (cd_Sz_error(cdp)==CANNOT_LOCK_KERNEL && extent!=0)
      { if (!yesno_box(_("Cannot lock the server when other clients are currently connected.\nDo you want to continue and only perform checks that do not require exclusive access?")))
          goto ex; 
      }
      else cd_Signalize(cdp);
    else display1();
   // extended integrity:
    { wxBusyCursor wait;
      if (!extent) res=0;
      else res = cd_Check_indexes(cdp, extent, mRepair->GetValue(), &index_err, &cache_err);
    }
    if (res) cd_Signalize(cdp);
    else display2();
   // enable controls:
   ex:
    completed();
    mClose->Enable();
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DatabaseConsistencyDlg::OnCancelClick( wxCommandEvent& event )
// Just closes the dialog
{ 
  if (phase==1 || phase==2)
  { cd_Break(cdp);
    cd_wait_for_answer(cdp);
    cd_concurrent(cdp, FALSE);
    wxEndBusyCursor();
  }
  event.Skip(); 
}

void DatabaseConsistencyDlg::completed(void)
{ 
  mLost->Enable();
  mNonex->Enable();
  mCross->Enable();
  mDamaged->Enable();
  mIndex->Enable();
  mCache->Enable();
  mStart->Enable();
  mRepair->Enable();
  mClose->SetLabel(_("Close"));
  phase=0;
  cd_concurrent(cdp, FALSE);
}

void DatabaseConsistencyDlg::OnIdle( wxIdleEvent& event )
{
  if (phase==1)
  { if (cd_answered(cdp))
    { wxEndBusyCursor();
      if (cd_Sz_error(cdp)==CANNOT_LOCK_KERNEL && extent!=0)
      { phase=3;
        if (!yesno_box(_("Cannot lock the server when other clients are currently connected.\nDo you want to continue and only perform checks that do not require exclusive access?")))
          { completed();  return; }
      }
      else if (cd_Sz_error(cdp))
        cd_Signalize(cdp);
      else display1();
     // extended integrity:
      wxBeginBusyCursor();
      if (extent) 
      { cd_Check_indexes(cdp, extent, mRepair->GetValue(), &index_err, &cache_err);
        phase = 2;
      }  
      else 
      { phase=3;
        completed();
      }  
    }
  }
  else if (phase==2)
  { if (cd_answered(cdp))
    { wxEndBusyCursor();
      if (cd_Sz_error(cdp))
        cd_Signalize(cdp);
      else display2();
      completed();
    }
  }
  event.Skip();
}


/*!
 * Get bitmap resources
 */

wxBitmap DatabaseConsistencyDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin DatabaseConsistencyDlg bitmap retrieval
    return wxNullBitmap;
////@end DatabaseConsistencyDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon DatabaseConsistencyDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin DatabaseConsistencyDlg icon retrieval
    return wxNullIcon;
////@end DatabaseConsistencyDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DBINT_REPAIR
 */

void DatabaseConsistencyDlg::OnCdDbintRepairClick( wxCommandEvent& event )
{
  if (mRepair->GetValue())
  { mIndex->SetValue(false);
    mCache->SetValue(false);
  }
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DBINT_CACHE
 */

void DatabaseConsistencyDlg::OnCdDbintCacheClick( wxCommandEvent& event )
{
  if (mCache->GetValue())
    mRepair->SetValue(false);
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DBINT_INDEX
 */

void DatabaseConsistencyDlg::OnCdDbintIndexClick( wxCommandEvent& event )
{
  if (mIndex->GetValue())
    mRepair->SetValue(false);
}


