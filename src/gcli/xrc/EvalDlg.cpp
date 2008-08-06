/////////////////////////////////////////////////////////////////////////////
// Name:        EvalDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/03/04 08:57:40
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "EvalDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "EvalDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * EvalDlg type definition
 */

IMPLEMENT_CLASS( EvalDlg, wxDialog )

/*!
 * EvalDlg event table definition
 */

BEGIN_EVENT_TABLE( EvalDlg, wxDialog )

////@begin EvalDlg event table entries
    EVT_TEXT( CD_EVAL_VAL, EvalDlg::OnCdEvalValUpdated )

    EVT_BUTTON( CD_EVAL_EVAL, EvalDlg::OnCdEvalEvalClick )

    EVT_BUTTON( CD_EVAL_SET, EvalDlg::OnCdEvalSetClick )

    EVT_BUTTON( CD_EVAL_WATCH, EvalDlg::OnCdEvalWatchClick )

    EVT_BUTTON( wxID_CANCEL, EvalDlg::OnCancelClick )

////@end EvalDlg event table entries

END_EVENT_TABLE()

/*!
 * EvalDlg constructors
 */

EvalDlg::EvalDlg(cdp_t cdpIn, wxString exprIn)
{ cdp=cdpIn;  expr=exprIn; }

/*!
 * EvalDlg creator
 */

bool EvalDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin EvalDlg member initialisation
    mEval = NULL;
////@end EvalDlg member initialisation

////@begin EvalDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end EvalDlg creation
    return TRUE;
}

/*!
 * Control creation for EvalDlg
 */

void EvalDlg::CreateControls()
{    
////@begin EvalDlg content construction
    EvalDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer4, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("&Expression or variable:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText5, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mEvalStrings = NULL;
    mEval = new wxOwnerDrawnComboBox;
    mEval->Create( itemDialog1, CD_EVAL_EXPR, _T(""), wxDefaultPosition, wxSize(240, -1), 0, mEvalStrings, wxCB_DROPDOWN );
    itemBoxSizer4->Add(mEval, 1, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("&Value:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText7, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxTextCtrl* itemTextCtrl8 = new wxTextCtrl;
    itemTextCtrl8->Create( itemDialog1, CD_EVAL_VAL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemTextCtrl8, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer9, 0, wxALIGN_TOP|wxALL, 0);

    wxButton* itemButton10 = new wxButton;
    itemButton10->Create( itemDialog1, CD_EVAL_EVAL, _("Ev&aluate"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton10->SetDefault();
    itemBoxSizer9->Add(itemButton10, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 3);

    wxButton* itemButton11 = new wxButton;
    itemButton11->Create( itemDialog1, CD_EVAL_SET, _("&Set value"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemButton11, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 3);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, CD_EVAL_WATCH, _("Add &Watch"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemButton12, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 3);

    wxButton* itemButton13 = new wxButton;
    itemButton13->Create( itemDialog1, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemButton13, 0, wxGROW|wxALL, 3);

////@end EvalDlg content construction
    persistent_eval_history.fill(mEval);
    mEval->SetValue(expr);  // set the input value
    evaluate_and_show(expr);  // and evaluates it
}

/*!
 * Should we show tooltips?
 */

bool EvalDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_EVAL_VAL
 */

void EvalDlg::OnCdEvalValUpdated( wxCommandEvent& event )
{ FindWindow(CD_EVAL_SET)->Enable();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_EVAL_EVAL
 */

void EvalDlg::OnCdEvalEvalClick( wxCommandEvent& event )
{ evaluate_and_show(mEval->GetValue()); }


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_EVAL_SET
 */

void EvalDlg::OnCdEvalSetClick( wxCommandEvent& event )
{ expr = mEval->GetValue();
  expr.Trim(false);  expr.Trim(true);
  if (expr.IsEmpty()) { wxBell();  return; }
  wxString val = ((wxTextCtrl*)FindWindow(CD_EVAL_VAL))->GetValue();
  val.Trim(false);  val.Trim(true);
  if (cd_Server_assign(cdp, 0, expr.mb_str(*cdp->sys_conv), val.IsEmpty() ? "NULL" : val.mb_str(*cdp->sys_conv))) 
    { cd_Signalize(cdp);  return; }
 // read the updated value and show it (may have been affected by a trigger etc.):
  evaluate_and_show(expr);
 // update watch:
  the_server_debugger->watch_view->recalc_all();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_EVAL_WATCH
 */

void EvalDlg::OnCdEvalWatchClick( wxCommandEvent& event )
{ expr = mEval->GetValue();
  expr.Trim(false);  expr.Trim(true);
  if (expr.IsEmpty()) { wxBell();  return; }
  the_server_debugger->watch_view->append(expr);
}
///////////////////////////////////////////////////////////////////////
void EvalDlg::evaluate_and_show(wxString ex)
{ ex.Trim(false);  ex.Trim(true);
  wxString msg;
  bool err=!eval_expr_to_string(cdp, ex, msg);
  ((wxTextCtrl*)FindWindow(CD_EVAL_VAL))->SetValue(msg);
  FindWindow(CD_EVAL_SET)->Disable();
  if (err) cd_Signalize2(cdp, this);
  else 
  { persistent_eval_history.add_to_history(ex);  // not action if ex is empty
    persistent_eval_history.fill(mEval);  // update the history
    mEval->SetValue(ex);  // return the expression cleared by fill_history();
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void EvalDlg::OnCancelClick( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap EvalDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin EvalDlg bitmap retrieval
    return wxNullBitmap;
////@end EvalDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon EvalDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin EvalDlg icon retrieval
    return wxNullIcon;
////@end EvalDlg icon retrieval
}
