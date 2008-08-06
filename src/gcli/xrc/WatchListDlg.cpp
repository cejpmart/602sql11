/////////////////////////////////////////////////////////////////////////////
// Name:        WatchListDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/24/04 14:25:10
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "WatchListDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
#include "wx/wx.h"
#include "wx/listctrl.h"
////@end includes

#include "WatchListDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * WatchListDlg type definition
 */

IMPLEMENT_CLASS( WatchListDlg, wxPanel )

/*!
 * WatchListDlg event table definition
 */

BEGIN_EVENT_TABLE( WatchListDlg, wxPanel )

////@begin WatchListDlg event table entries
    EVT_LIST_END_LABEL_EDIT( CD_WATCH_LIST, WatchListDlg::OnCdWatchListEndLabelEdit )
    EVT_LIST_KEY_DOWN( CD_WATCH_LIST, WatchListDlg::OnCdWatchListKeyDown )

////@end WatchListDlg event table entries

END_EVENT_TABLE()

class WatchTextDropTarget : public wxTextDropTarget
{ WatchListDlg * watch;
 public: 
  WatchTextDropTarget(WatchListDlg * watchIn)
    { watch=watchIn; }
  bool OnDropText(wxCoord x, wxCoord y, const wxString & data);
};

/*!
 * WatchListDlg constructors
 */

WatchListDlg::WatchListDlg(server_debugger * my_server_debuggerIn)
{ my_server_debugger=my_server_debuggerIn; }

/*!
 * WatchListDlg creator
 */

bool WatchListDlg::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WatchListDlg member initialisation
////@end WatchListDlg member initialisation

////@begin WatchListDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WatchListDlg creation
    return TRUE;
}

/*!
 * Control creation for WatchListDlg
 */

void WatchListDlg::CreateControls()
{    
////@begin WatchListDlg content construction

    WatchListDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxListCtrl* item3 = new wxListCtrl;
    item3->Create( item1, CD_WATCH_LIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_EDIT_LABELS|wxLC_HRULES|wxLC_VRULES );
    mList = item3;
    item2->Add(item3, 1, wxGROW|wxALL, 0);

////@end WatchListDlg content construction
  // SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL); -- does not help to deliver WXK_Return to OnKeyDown
  wxListItem itemCol;
  itemCol.m_mask   = wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT | wxLIST_MASK_WIDTH;
  itemCol.m_format = wxLIST_FORMAT_LEFT;
  itemCol.m_text   = _("Name");
  itemCol.m_width  = 85;
  mList->InsertColumn(0, itemCol);
  itemCol.m_text   = _("Value");
  itemCol.m_width  = 100;
  mList->InsertColumn(1, itemCol);
  mList->InsertItem(0, ADD_WATCH, 0);  // fictive item
  mList->SetDropTarget(new WatchTextDropTarget(this));
}

/*!
 * Should we show tooltips?
 */

bool WatchListDlg::ShowToolTips()
{
    return TRUE;
}

void WatchListDlg::recalc(int ind, wxString name)
{ name.Trim(false);  name.Trim(true);
  wxString result;
  if (!(name==ADD_WATCH))
    eval_expr_to_string(my_server_debugger->cdp, name, result);
  mList->SetItem(ind, 1, result);
}

void WatchListDlg::recalc_all(void)
{ for (int i=0;  i<mList->GetItemCount();  i++)
    recalc(i, mList->GetItemText(i));
}

void WatchListDlg::append(const wxString & data, int ind)
{ if (ind==-1) ind=mList->GetItemCount()-1;  // insert on the end of the list (before the fictive item)
  mList->InsertItem(ind, data);
  mList->EnsureVisible(ind);
  recalc(ind, data);
}

void WatchListDlg::OnDropText(wxCoord x, wxCoord y, const wxString & data)
{ int flags;
  long ind = mList->HitTest(wxPoint(x,y), flags);  // insert before the item below the drop position
  if (!(flags & wxLIST_HITTEST_ONITEM)) ind=-1;  // ... or on the end of the list (before the fictive item)
  append(data, ind);  // ind==-1 on error and on drop not over an existing item
}

bool WatchTextDropTarget::OnDropText(wxCoord x, wxCoord y, const wxString & data)
{ watch->OnDropText(x, y, data);
  return false;  // prevents cutting the text from the source
}

/*!
 * wxEVT_COMMAND_LIST_END_LABEL_EDIT event handler for CD_WATCH_LIST
 */

void WatchListDlg::OnCdWatchListEndLabelEdit( wxListEvent& event )
{ wxString new_label;
  if (!event.IsEditCancelled())  // true when either cancelled or the original value confirmed unchanged
  { 
//#ifdef WINS  
//    if (mList->GetEditControl()) new_label = mList->GetEditControl()->GetValue(); 
//#else	
    new_label = event.GetLabel(); // this does not have the new value on WINS, but it works well on Linux -- no more
//#endif
    new_label.Trim(false);  new_label.Trim(true);
    if (new_label.IsEmpty())
    { if (event.GetIndex()+1!=mList->GetItemCount())  // unless the last (fictive) item
        mList->DeleteItem(event.GetIndex());
    }
    else
    { if (event.GetIndex()+1==mList->GetItemCount())  // if the last (fictive) item defined, add a new one
        mList->InsertItem(mList->GetItemCount(), ADD_WATCH);  // fictive item
      recalc(event.GetIndex(), new_label);
    }
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_LIST_KEY_DOWN event handler for CD_WATCH_LIST
 */

void WatchListDlg::OnCdWatchListKeyDown( wxListEvent& event )
{ if (mList->GetSelectedItemCount()==1)
    if (event.GetKeyCode()==WXK_DELETE)
    { long ind = mList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
      if (ind!=-1 && ind+1!=mList->GetItemCount()) // cannot delete the fictive item
        mList->DeleteItem(ind);
    }
    else if (event.GetKeyCode()==WXK_RETURN || event.GetKeyCode()==WXK_F2 || event.GetKeyCode()>=' ' && event.GetKeyCode()<WXK_DELETE)
    // WXK_RETURN is not delivered here :-(
    { long ind = mList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
      if (ind!=-1) mList->EditLabel(ind);
      event.Veto();  // Skip must not be called and Veto must be called to prevent Beep
      // Cannot send the key to the edit - it has not been created yet.
    }
    else event.Skip();
  else event.Skip();
}


