/////////////////////////////////////////////////////////////////////////////
// Name:        MonitorLogDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/30/04 12:31:07
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "MonitorLogDlg.h"
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

#include "MonitorLogDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * MonitorLogDlg type definition
 */

IMPLEMENT_CLASS( MonitorLogDlg, wxPanel )

/*!
 * MonitorLogDlg event table definition
 */

BEGIN_EVENT_TABLE( MonitorLogDlg, wxPanel )

////@begin MonitorLogDlg event table entries
    EVT_LISTBOX( CD_LOG_LIST, MonitorLogDlg::OnCdLogListSelected )

////@end MonitorLogDlg event table entries
    EVT_MENU(-1, MonitorLogDlg::OnCommand)

END_EVENT_TABLE()

BEGIN_EVENT_TABLE( LogRefreshHandler, wxEvtHandler )
  EVT_RIGHT_DOWN(LogRefreshHandler::OnRightClick)
END_EVENT_TABLE()

void LogRefreshHandler::OnRightClick(wxMouseEvent & event)
{
  wxMenu * popup_menu = new wxMenu;
  popup_menu->Append(CPM_REFRPANEL, _("Refresh the log"));
  //char buf [100];
  //sprintf(buf, "x %d y %d", event.GetPosition().x, event.GetPosition().y);
  //log_client_error(buf);
  log->mList->PopupMenu(popup_menu, event.GetPosition());
  delete popup_menu;
  event.Skip();
}

/*!
 * MonitorLogDlg constructors
 */

MonitorLogDlg::MonitorLogDlg(cdp_t cdpIn, const char * log_nameIn)
{ cdp=cdpIn;
  strcpy(log_name, log_nameIn);
}

/*!
 * MonitorLogDlg creator
 */

bool MonitorLogDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MonitorLogDlg member initialisation
    mList = NULL;
    mLine = NULL;
    mFile = NULL;
////@end MonitorLogDlg member initialisation

////@begin MonitorLogDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end MonitorLogDlg creation
    return TRUE;
}

/*!
 * Control creation for MonitorLogDlg
 */

void MonitorLogDlg::CreateControls()
{
////@begin MonitorLogDlg content construction

    MonitorLogDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxString* item3Strings = NULL;
    wxListBox* item3 = new wxListBox;
    item3->Create( item1, CD_LOG_LIST, wxDefaultPosition, wxDefaultSize, 0, item3Strings, wxLB_SINGLE|wxHSCROLL );
    mList = item3;
    item2->Add(item3, 6, wxGROW, 5);
    wxTextCtrl* item4 = new wxTextCtrl;
    item4->Create( item1, CD_LOG_LINE, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_WORDWRAP );
    mLine = item4;
    item2->Add(item4, 1, wxGROW, 5);
    wxStaticText* item5 = new wxStaticText;
    item5->Create( item1, CD_LOG_FILE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mFile = item5;
    item2->Add(item5, 0, wxGROW|wxADJUST_MINSIZE, 5);
////@end MonitorLogDlg content construction
#ifdef LINUX  // since wx 2.6.0 cycles in the layout algorithm when mFile exists
   item2->Remove(item5);
   mFile->Hide();
   mFile=NULL;
#endif
  LogRefreshHandler * handler = new LogRefreshHandler(this);
  mList->PushEventHandler(handler);
}

/*!
 * Should we show tooltips?
 */

bool MonitorLogDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for CD_LOG_LIST
 */

void MonitorLogDlg::OnCdLogListSelected( wxCommandEvent& event )
{
  mLine->SetValue(mList->GetStringSelection());
  event.Skip();
}


void MonitorLogDlg::refresh_log(void)
{ bool unicode;
  char buf[1024];
  mList->Clear();
  tcursnum curs;  trecnum cnt, r;  char query[150];
  sprintf(query, "select log_message_w from _iv_recent_log where log_name=\'%s\' order by seq_num", log_name);
  if (!cd_Open_cursor_direct(cdp, query, &curs))
    unicode=true;
  else
  { sprintf(query, "select log_message from _iv_recent_log where log_name=\'%s\' order by seq_num", log_name);
    if (cd_Open_cursor_direct(cdp, query, &curs))
    { mList->Append(Get_error_num_textWX(cdp, cd_Sz_error(cdp)));
      return;
    }
    unicode=false;
  }

  { cd_Rec_cnt(cdp, curs, &cnt);
    for (r=0;  r<cnt;  r++)
    { uns32 size;
      if (!cd_Read_var(cdp, curs, r, 1, NOINDEX, 0, sizeof(buf)-1, buf, &size))
      { buf[size & 0x7ffffffe]=0;  buf[(size & 0x7ffffffe) + 1]=0;
        if (unicode)
        { wxChar buf4[4096];
          superconv(ATT_STRING, ATT_STRING, buf, (char*)buf4, t_specif(0, 0, 0, 1), wx_string_spec, NULL);
          mList->Append(wxString(buf4));
        }
        else
          mList->Append(wxString(buf, *cdp->sys_conv));
      }
    }
    cd_Close_cursor(cdp, curs);
  }
  cnt = mList->GetCount();
  if (cnt)
  { mList->SetSelection(cnt-1);
    mLine->SetValue(mList->GetStringSelection());
  }
  else
    mLine->SetValue(wxEmptyString);
}

void MonitorLogDlg::OnCommand(wxCommandEvent & event)
{ switch (event.GetId())
  { case CPM_REFRPANEL:
      refresh_log();  break;
    default:
      event.Skip();  break;
  }
}

