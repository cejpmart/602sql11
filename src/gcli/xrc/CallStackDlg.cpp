/////////////////////////////////////////////////////////////////////////////
// Name:        CallStackDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/20/04 08:09:59
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "CallStackDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
#include "wx/wx.h"
////@end includes

#include "CallStackDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * CallStackDlg type definition
 */

IMPLEMENT_CLASS( CallStackDlg, wxPanel )

/*!
 * CallStackDlg event table definition
 */

BEGIN_EVENT_TABLE( CallStackDlg, wxPanel )

////@begin CallStackDlg event table entries
    EVT_LISTBOX_DCLICK( CD_CALLSTACK_LIST, CallStackDlg::OnCdCallstackListDoubleClicked )

////@end CallStackDlg event table entries

END_EVENT_TABLE()

/*!
 * CallStackDlg constructors
 */

CallStackDlg::CallStackDlg(server_debugger * my_server_debuggerIn)
{ my_server_debugger=my_server_debuggerIn;  running_info=false;  stk=NULL; }

CallStackDlg::~CallStackDlg(void)
{ if (stk) corefree(stk); }

/*!
 * CallStackDlg creator
 */

bool CallStackDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CallStackDlg member initialisation
////@end CallStackDlg member initialisation

////@begin CallStackDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CallStackDlg creation
    return TRUE;
}

/*!
 * Control creation for CallStackDlg
 */

void CallStackDlg::CreateControls()
{    
////@begin CallStackDlg content construction

    CallStackDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxString* item3Strings = NULL;
    wxListBox* item3 = new wxListBox;
    item3->Create( item1, CD_CALLSTACK_LIST, wxDefaultPosition, wxDefaultSize, 0, item3Strings, wxLB_SINGLE|wxHSCROLL );
    mListBox = item3;
    item2->Add(item3, 1, wxGROW|wxALL, 0);

////@end CallStackDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool CallStackDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for CD_CALLSTACK_LIST
 */

void CallStackDlg::OnCdCallstackListDoubleClicked( wxCommandEvent& event )
{ int ind = mListBox->GetSelection();
  // event.Skip(false);  -- does not help
  if (ind==-1 || !stk) return;
      editor_type * edd=find_editor_by_objnum9(my_server_debugger->cdp, OBJ_TABLENUM, stk[ind].objnum, OBJ_DEF_ATR, NONECATEG, true);
  if (!edd) return;
  edd->position_on_line_column(stk[ind].line, 0);
}

void CallStackDlg::update(bool breaked)
{ if (stk)
    { corefree(stk);  stk=NULL; }
  if (breaked)
  { mListBox->Clear();
    if (!cd_Server_stack9(my_server_debugger->cdp, &stk) && stk)
    { for (t_stack_info9 * pstk = stk;  pstk->objnum;  pstk++)
      { //tobjname objname;
        //cd_Read(my_server_debugger->cdp, OBJ_TABLENUM, pstk->objnum, OBJ_NAME_ATR, NULL, objname);
        wb_small_string(my_server_debugger->cdp, pstk->rout_name, true);
        mListBox->Append(wxString(pstk->rout_name, *my_server_debugger->cdp->sys_conv));
      }
    }
    running_info=false;
  }
  else
    if (!running_info)
    { mListBox->Clear();
      mListBox->Append(_("The client is not breaked now. Call stack is not shown."));
      running_info=true;
    }
}

