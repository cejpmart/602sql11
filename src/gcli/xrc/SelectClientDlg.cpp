/////////////////////////////////////////////////////////////////////////////
// Name:        SelectClientDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/02/04 15:01:57
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "SelectClientDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "SelectClientDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * SelectClientDlg type definition
 */

IMPLEMENT_CLASS( SelectClientDlg, wxDialog )

/*!
 * SelectClientDlg event table definition
 */

BEGIN_EVENT_TABLE( SelectClientDlg, wxDialog )

////@begin SelectClientDlg event table entries
    EVT_LIST_ITEM_SELECTED( CD_SELPROC_LIST, SelectClientDlg::OnCdSelprocListSelected )

////@end SelectClientDlg event table entries

END_EVENT_TABLE()

/*!
 * SelectClientDlg constructors
 */

SelectClientDlg::SelectClientDlg(cdp_t cdpIn)
{ cdp=cdpIn; }

/*!
 * SelectClientDlg creator
 */

bool SelectClientDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SelectClientDlg member initialisation
    mSizerContainingGrid = NULL;
    mList = NULL;
    mOK = NULL;
////@end SelectClientDlg member initialisation

////@begin SelectClientDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end SelectClientDlg creation
    return TRUE;
}

static wxChar * column_captions[] = {
  wxTRANSLATE("Client is logged in as"),
  wxTRANSLATE("Schema opened"),
  wxTRANSLATE("Client number"),
  wxTRANSLATE("IP Address"),
  NULL
};

/*!
 * Control creation for SelectClientDlg
 */

void SelectClientDlg::CreateControls()
{    
////@begin SelectClientDlg content construction
    SelectClientDlg* itemDialog1 = this;

    mSizerContainingGrid = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(mSizerContainingGrid);

    mList = new wxListCtrl;
    mList->Create( itemDialog1, CD_SELPROC_LIST, wxDefaultPosition, wxSize(230, 100), wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_HRULES|wxLC_VRULES );
    mSizerContainingGrid->Add(mList, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    mSizerContainingGrid->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    itemBoxSizer4->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mOK = new wxButton;
    mOK->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(mOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer4->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton8 = new wxButton;
    itemButton8->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer4->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end SelectClientDlg content construction
  int w, h, tot_w=wxSystemSettings::GetMetric(wxSYS_VSCROLL_X)+4;
  mOK->Disable();
 // create columns of the list view:
  wxListItem itemCol;
  itemCol.m_mask   = wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT | wxLIST_MASK_WIDTH;
  itemCol.m_format = wxLIST_FORMAT_LEFT;
  for (int col=0;  column_captions[col]!=NULL;  col++)
  { itemCol.m_text   = wxGetTranslation(column_captions[col]);
    mList->GetTextExtent(itemCol.m_text, &w, &h);
    tot_w += itemCol.m_width  = 16+w;
    mList->InsertColumn(col, itemCol);
  }
  mSizerContainingGrid->SetItemMinSize(mList, tot_w, -1);

 // list clients:
  tcursnum curs;  trecnum rec, cnt;  int pos=0;
  if (!cd_Open_cursor_direct(cdp, "select login_name, sel_schema_name, client_number, net_address, worker_thread, own_connection from _iv_logged_users", &curs))
  { cd_Rec_cnt(cdp, curs, &cnt);
    for (rec=0;  rec<cnt;  rec++)
    { wbbool flag;  tobjname buf;  int num;
      cd_Read(cdp, curs, rec, 5, NULL, &flag);
      if (flag) continue;
      cd_Read(cdp, curs, rec, 6, NULL, &flag);
      if (flag) continue;
      cd_Read(cdp, curs, rec, 1, NULL, buf);  // user name
      wb_small_string(cdp, buf, true);  
      mList->InsertItem(pos, wxString(buf, *cdp->sys_conv));
      cd_Read(cdp, curs, rec, 2, NULL, buf);  // schema name
      wb_small_string(cdp, buf, true);  
      mList->SetItem(pos, 1, wxString(buf, *cdp->sys_conv));
      cd_Read(cdp, curs, rec, 3, NULL, (tptr)&num);  // client number
      mList->SetItem(pos, 2, int2wxstr(num));
      cd_Read(cdp, curs, rec, 4, NULL, buf);  // IP address
      mList->SetItem(pos, 3, wxString(buf, *wxConvCurrent));
      mList->SetItemData(pos, num);
      pos++;  
    }
    cd_Close_cursor(cdp, curs);
  }
}

/*!
 * Should we show tooltips?
 */

bool SelectClientDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for CD_SELPROC_LIST
 */

void SelectClientDlg::OnCdSelprocListSelected( wxListEvent& event )
{ mOK->Enable();
  event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap SelectClientDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin SelectClientDlg bitmap retrieval
    return wxNullBitmap;
////@end SelectClientDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon SelectClientDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin SelectClientDlg icon retrieval
    return wxNullIcon;
////@end SelectClientDlg icon retrieval
}
