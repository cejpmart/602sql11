/////////////////////////////////////////////////////////////////////////////
// Name:        QueryOptimizationDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/03/04 12:49:06
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "QueryOptimizationDlg.h"
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

#include "QueryOptimizationDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * QueryOptimizationDlg type definition
 */

IMPLEMENT_CLASS( QueryOptimizationDlg, wxDialog )

/*!
 * QueryOptimizationDlg event table definition
 */

BEGIN_EVENT_TABLE( QueryOptimizationDlg, wxDialog )

////@begin QueryOptimizationDlg event table entries
    EVT_BUTTON( CD_OPTIM_EVAL, QueryOptimizationDlg::OnCdOptimEvalClick )

    EVT_BUTTON( wxID_CANCEL, QueryOptimizationDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, QueryOptimizationDlg::OnHelpClick )

////@end QueryOptimizationDlg event table entries

END_EVENT_TABLE()

/*!
 * QueryOptimizationDlg constructors
 */

QueryOptimizationDlg::QueryOptimizationDlg(cdp_t cdpIn, const char * sourceIn)
{ cdp=cdpIn;  source=sourceIn; }

/*!
 * QueryOptimizationDlg creator
 */

bool QueryOptimizationDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin QueryOptimizationDlg member initialisation
    mSizer = NULL;
    mTree = NULL;
    mEval = NULL;
////@end QueryOptimizationDlg member initialisation

////@begin QueryOptimizationDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end QueryOptimizationDlg creation
    return TRUE;
}

/*!
 * Control creation for QueryOptimizationDlg
 */

void QueryOptimizationDlg::CreateControls()
{    
////@begin QueryOptimizationDlg content construction
    QueryOptimizationDlg* itemDialog1 = this;

    mSizer = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(mSizer);

    mTree = new wxTreeCtrl;
    mTree->Create( itemDialog1, CD_OPTIM_TREE, wxDefaultPosition, wxSize(300, 300), wxTR_HIDE_ROOT|wxTR_SINGLE );
    mSizer->Add(mTree, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    mSizer->Add(itemBoxSizer4, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mEval = new wxButton;
    mEval->Create( itemDialog1, CD_OPTIM_EVAL, _("Add evaluation statistics"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(mEval, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton6 = new wxButton;
    itemButton6->Create( itemDialog1, wxID_CANCEL, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemButton6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton7 = new wxButton;
    itemButton7->Create( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemButton7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end QueryOptimizationDlg content construction
  mSizer->SetItemMinSize(mTree, 300, 300);
  mTree->SetImageList(wxGetApp().frame->main_image_list);  // does not take the ownership
  show_opt(false);
}

/*!
 * Should we show tooltips?
 */

bool QueryOptimizationDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_OPTIM_EVAL
 */

void QueryOptimizationDlg::OnCdOptimEvalClick( wxCommandEvent& event )
{
  show_opt(true);
}

void QueryOptimizationDlg::show_opt(bool evaluate)
{ wxBusyCursor wait;
  mTree->DeleteAllItems();
  tptr result;
  if (cd_Query_optimization(cdp, source, evaluate, &result)) 
    cd_Signalize(cdp);
  else 
  { char * p = result;
    wxTreeItemId hParent = mTree->GetRootItem(), hLast; 
    do
    { int image=IND_PROGR;  bool bold=false;
      if (*p==4)
        { image=IND_LINK;  bold=true;  p++; }
      if (*p==5)
        { image=IND_TABLE;  p++; }
      const char * itemtext = p; 
      while (*p>1) p++;
      if (*p==1) { *p=0;  p++; }
     // Append the item to the tree view control:
      hLast = mTree->AppendItem(hParent, wxString(itemtext, *cdp->sys_conv), image, image);
      if (bold) mTree->SetItemBold(hLast);
      if (*p==2) { p++;  hParent=hLast; }
      else while (*p==3) 
      { p++;
        mTree->Expand(hParent);  
        hParent=mTree->GetItemParent(hParent);
      }
    } while (*p);
    corefree(result);
  }
}

/*!
 * Get bitmap resources
 */

wxBitmap QueryOptimizationDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin QueryOptimizationDlg bitmap retrieval
    return wxNullBitmap;
////@end QueryOptimizationDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon QueryOptimizationDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin QueryOptimizationDlg icon retrieval
    return wxNullIcon;
////@end QueryOptimizationDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void QueryOptimizationDlg::OnHelpClick( wxCommandEvent& event )
{
  wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/optiminfo.html"));
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void QueryOptimizationDlg::OnCancelClick( wxCommandEvent& event )
{
  event.Skip();
}


