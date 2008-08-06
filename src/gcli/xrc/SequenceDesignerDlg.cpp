/////////////////////////////////////////////////////////////////////////////
// Name:        SequenceDesignerDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/21/04 11:47:52
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "SequenceDesignerDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "SequenceDesignerDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * SequenceDesignerDlg type definition
 */

IMPLEMENT_CLASS( SequenceDesignerDlg, wxDialog )

/*!
 * SequenceDesignerDlg event table definition
 */

BEGIN_EVENT_TABLE( SequenceDesignerDlg, /*wxPanel*/wxDialog )

////@begin SequenceDesignerDlg event table entries
    EVT_TEXT( CD_SEQ_START, SequenceDesignerDlg::OnCdSeqStartUpdated )

    EVT_TEXT( CD_SEQ_STEP, SequenceDesignerDlg::OnCdSeqStepUpdated )

    EVT_CHECKBOX( CD_SEQ_MAX, SequenceDesignerDlg::OnCdSeqMaxClick )

    EVT_TEXT( CD_SEQ_MAXVAL, SequenceDesignerDlg::OnCdSeqMaxvalUpdated )

    EVT_CHECKBOX( CD_SEQ_MIN, SequenceDesignerDlg::OnCdSeqMinClick )

    EVT_TEXT( CD_SEQ_MINVAL, SequenceDesignerDlg::OnCdSeqMinvalUpdated )

    EVT_CHECKBOX( CD_SEQ_CYCLE, SequenceDesignerDlg::OnCdSeqCycleClick )

    EVT_CHECKBOX( CD_SEQ_CACHE, SequenceDesignerDlg::OnCdSeqCacheClick )

    EVT_SPINCTRL( CD_SEQ_CACHEVAL, SequenceDesignerDlg::OnCdSeqCachevalUpdated )
    EVT_TEXT( CD_SEQ_CACHEVAL, SequenceDesignerDlg::OnCdSeqCachevalTextUpdated )

    EVT_BUTTON( wxID_OK, SequenceDesignerDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, SequenceDesignerDlg::OnCancelClick )

    EVT_BUTTON( CD_SEQ_VALIDATE, SequenceDesignerDlg::OnCdSeqValidateClick )

    EVT_BUTTON( CD_SEQ_SQL, SequenceDesignerDlg::OnCdSeqSqlClick )

////@end SequenceDesignerDlg event table entries
    EVT_ACTIVATE(SequenceDesignerDlg::OnActivate)
    EVT_CLOSE(SequenceDesignerDlg::OnCloseWindow)
#ifndef WINS
    EVT_KEY_DOWN(SequenceDesignerDlg::OnKeyDown)
#endif
#if WBVERS>=1000
    EVT_MENU(-1, SequenceDesignerDlg::OnxCommand)
#endif
END_EVENT_TABLE()

/*!
 * SequenceDesignerDlg constructors
 */

SequenceDesignerDlg::SequenceDesignerDlg(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn) :
  any_designer(cdpIn, objnumIn, folder_nameIn, schema_nameIn, CATEG_SEQ, _seq_xpm, &seqdsng_coords)
    { }

SequenceDesignerDlg::~SequenceDesignerDlg(void)
{ if (global_style==ST_POPUP) 
    remove_tools();
}

void SequenceDesignerDlg::set_designer_caption(void)
{ wxString caption;
  if (!modifying()) caption = _("Design a New Sequence");
  else caption.Printf(_("Edit Design of Sequence %s"), wxString(seq.name, *cdp->sys_conv).c_str());
  SetTitle(caption);
}

bool SequenceDesignerDlg::open(wxWindow * parent, t_global_style my_style)
{ if (modifying())
  { tptr defin=cd_Load_objdef(cdp, objnum, CATEG_SEQ);
    if (!defin) { cd_Signalize(cdp);  return false; }
    if (!compile_sequence(cdp, defin, &seq))
      { corefree(defin);  cd_Signalize(cdp);  return false; }
    corefree(defin);
  }
  else
  { seq.name[0]=seq.schema[0]=0;
    seq.startval=0;  seq.step=1;  seq.cache=20;
    seq.hasmax=seq.hasmin=seq.cycles=FALSE;  seq.hascache=seq.ordered=TRUE;
  }
  if (!Create(parent, SYMBOL_SEQUENCEDESIGNERDLG_IDNAME, SYMBOL_SEQUENCEDESIGNERDLG_TITLE, persistent_coords->get_pos())) return false;
  competing_frame::focus_receiver = this;
  TransferDataToWindow();
  return true;
}

void SequenceDesignerDlg::Activated(void)
{ if (global_style==ST_POPUP)  // otherwise is modal
  { add_tools();
    write_status_message();
  }
}  

/*!
 * SequenceDesignerDlg creator
 */

bool SequenceDesignerDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SequenceDesignerDlg member initialisation
    mTopSizer = NULL;
    mTB = NULL;
    mButtons = NULL;
////@end SequenceDesignerDlg member initialisation

////@begin SequenceDesignerDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end SequenceDesignerDlg creation
    return TRUE;
}

/*!
 * Control creation for SequenceDesignerDlg
 */

void SequenceDesignerDlg::CreateControls()
{    
////@begin SequenceDesignerDlg content construction
    SequenceDesignerDlg* itemDialog1 = this;

    mTopSizer = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(mTopSizer);

    mTB = new wxToolBar;
    mTB->Create( itemDialog1, ID_TOOLBAR, wxDefaultPosition, wxDefaultSize, wxTB_FLAT|wxTB_HORIZONTAL|wxTB_NODIVIDER|wxNO_BORDER );
    mTB->Realize();
    mTopSizer->Add(mTB, 0, wxGROW|wxALL, 3);

    wxStaticLine* itemStaticLine4 = new wxStaticLine;
    itemStaticLine4->Create( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    mTopSizer->Add(itemStaticLine4, 0, wxGROW|wxALL, 0);

    wxFlexGridSizer* itemFlexGridSizer5 = new wxFlexGridSizer(6, 2, 0, 0);
    itemFlexGridSizer5->AddGrowableCol(1);
    mTopSizer->Add(itemFlexGridSizer5, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemDialog1, wxID_STATIC, _("&Start value:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* itemTextCtrl7 = new wxTextCtrl;
    itemTextCtrl7->Create( itemDialog1, CD_SEQ_START, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemTextCtrl7, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText8 = new wxStaticText;
    itemStaticText8->Create( itemDialog1, wxID_STATIC, _("S&tep:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemStaticText8, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* itemTextCtrl9 = new wxTextCtrl;
    itemTextCtrl9->Create( itemDialog1, CD_SEQ_STEP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemTextCtrl9, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxCheckBox* itemCheckBox10 = new wxCheckBox;
    itemCheckBox10->Create( itemDialog1, CD_SEQ_MAX, _("Ma&ximal value:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox10->SetValue(FALSE);
    itemFlexGridSizer5->Add(itemCheckBox10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxTextCtrl* itemTextCtrl11 = new wxTextCtrl;
    itemTextCtrl11->Create( itemDialog1, CD_SEQ_MAXVAL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemTextCtrl11, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxCheckBox* itemCheckBox12 = new wxCheckBox;
    itemCheckBox12->Create( itemDialog1, CD_SEQ_MIN, _("M&inimal value:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox12->SetValue(FALSE);
    itemFlexGridSizer5->Add(itemCheckBox12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxTextCtrl* itemTextCtrl13 = new wxTextCtrl;
    itemTextCtrl13->Create( itemDialog1, CD_SEQ_MINVAL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer5->Add(itemTextCtrl13, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxCheckBox* itemCheckBox14 = new wxCheckBox;
    itemCheckBox14->Create( itemDialog1, CD_SEQ_CYCLE, _("&Cycle"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox14->SetValue(FALSE);
    itemFlexGridSizer5->Add(itemCheckBox14, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer5->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxCheckBox* itemCheckBox16 = new wxCheckBox;
    itemCheckBox16->Create( itemDialog1, CD_SEQ_CACHE, _("Cache si&ze:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox16->SetValue(FALSE);
    itemFlexGridSizer5->Add(itemCheckBox16, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxSpinCtrl* itemSpinCtrl17 = new wxSpinCtrl;
    itemSpinCtrl17->Create( itemDialog1, CD_SEQ_CACHEVAL, _("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
    itemFlexGridSizer5->Add(itemSpinCtrl17, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mButtons = new wxBoxSizer(wxHORIZONTAL);
    mTopSizer->Add(mButtons, 0, wxGROW|wxALL, 0);

    wxButton* itemButton19 = new wxButton;
    itemButton19->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    mButtons->Add(itemButton19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton20 = new wxButton;
    itemButton20->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    mButtons->Add(itemButton20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton21 = new wxButton;
    itemButton21->Create( itemDialog1, CD_SEQ_VALIDATE, _("Validate"), wxDefaultPosition, wxDefaultSize, 0 );
    mButtons->Add(itemButton21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton22 = new wxButton;
    itemButton22->Create( itemDialog1, CD_SEQ_SQL, _("Show SQL"), wxDefaultPosition, wxDefaultSize, 0 );
    mButtons->Add(itemButton22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end SequenceDesignerDlg content construction
    ((wxSpinCtrl*)FindWindow(CD_SEQ_CACHEVAL))->SetRange(2,1000);
    GetSizer()->Show(mButtons, global_style!=ST_POPUP);
#if WBVERS<1000
    mTopSizer->Hide(mTB);
#else
    create_tools(mTB, get_tool_info());
#endif
    GetSizer()->Layout();
}

void SetDlgItemInt64(wxTextCtrl * edit, sig64 Value, bool bSigned)
{ char buf[40];
  if (bSigned)
    int64tostr(Value, buf);
  else
  { int64tostr((uns64)Value/10, buf);
    int len=(int)strlen(buf);
    buf[len]=(char)('0'+(uns64)Value%10);
    buf[len+1]=0;
  }
  edit->SetValue(wxString(buf, wxConvLocal));
}

bool SequenceDesignerDlg::TransferDataToWindow(void)
// Called on entry:
{ feedback_disabled=true;
 // show values:
  SetDlgItemInt64((wxTextCtrl*)FindWindow(CD_SEQ_START), seq.startval, true);                    
  SetDlgItemInt64((wxTextCtrl*)FindWindow(CD_SEQ_STEP),  seq.step,     true);
  if (seq.hasmax) SetDlgItemInt64((wxTextCtrl*)FindWindow(CD_SEQ_MAXVAL), seq.maxval, true);
  if (seq.hasmin) SetDlgItemInt64((wxTextCtrl*)FindWindow(CD_SEQ_MINVAL), seq.minval, true);
  if (seq.hascache) 
  { wxSpinCtrl * spin = (wxSpinCtrl*)FindWindow(CD_SEQ_CACHEVAL);
    wxString val;  val.Printf(wxT("%u"), seq.cache);
    spin->SetValue(val);
  }
  ((wxCheckBox*)FindWindow(CD_SEQ_MAX  ))->SetValue(seq.hasmax);
  ((wxCheckBox*)FindWindow(CD_SEQ_MIN  ))->SetValue(seq.hasmin);
  ((wxCheckBox*)FindWindow(CD_SEQ_CYCLE))->SetValue(seq.cycles);
  ((wxCheckBox*)FindWindow(CD_SEQ_CACHE))->SetValue(seq.hascache);
  FindWindow(CD_SEQ_MAXVAL  )->Enable(seq.hasmax);
  FindWindow(CD_SEQ_MINVAL  )->Enable(seq.hasmin);
  FindWindow(CD_SEQ_CACHEVAL)->Enable(seq.hascache);
 // init:
  feedback_disabled=false;
  changed = !modifying();  // the new design is valid and can be saved
  return true;
}

bool GetDlgItemInt64(wxTextCtrl * edit, sig64 & val, bool bSigned)
// Returns true iff translated
{ wxString str = edit->GetValue();
  if (bSigned)
    return str2int64(str.mb_str(wxConvLocal), &val)!=0;
  else
  { const char * p = str.mb_str(wxConvLocal);
    return str2uns64(&p, &val) && !*p;
  }
} 

bool SequenceDesignerDlg::TransferDataFromWindow(void)
// Validates
{
  seq.hasmax  =((wxCheckBox*)FindWindow(CD_SEQ_MAX  ))->GetValue();
  seq.hasmin  =((wxCheckBox*)FindWindow(CD_SEQ_MIN  ))->GetValue();
  seq.cycles  =((wxCheckBox*)FindWindow(CD_SEQ_CYCLE))->GetValue();
  seq.hascache=((wxCheckBox*)FindWindow(CD_SEQ_CACHE))->GetValue();
    if (!GetDlgItemInt64((wxTextCtrl*)FindWindow(CD_SEQ_START ), seq.startval, true))
      { client_error(cdp, INT_OUT_OF_BOUND);  cd_Signalize(cdp);  FindWindow(CD_SEQ_START   )->SetFocus();  return false; }
    if (!GetDlgItemInt64((wxTextCtrl*)FindWindow(CD_SEQ_STEP  ), seq.step, true))
      { client_error(cdp, INT_OUT_OF_BOUND);  cd_Signalize(cdp);  FindWindow(CD_SEQ_STEP    )->SetFocus();  return false; }
  if (seq.hasmax)
    if (!GetDlgItemInt64((wxTextCtrl*)FindWindow(CD_SEQ_MAXVAL), seq.maxval, true))
      { client_error(cdp, INT_OUT_OF_BOUND);  cd_Signalize(cdp);  FindWindow(CD_SEQ_MAXVAL  )->SetFocus();  return false; }
  if (seq.hasmin)
    if (!GetDlgItemInt64((wxTextCtrl*)FindWindow(CD_SEQ_MINVAL), seq.minval, true))
      { client_error(cdp, INT_OUT_OF_BOUND);  cd_Signalize(cdp);  FindWindow(CD_SEQ_MINVAL  )->SetFocus();  return false; }
  if (seq.hascache)
  { //unsigned long lval;
    //if (((wxSpinCtrl*)FindWindow(CD_SEQ_CACHEVAL))->GetValue().ToULong(&lval))
    //  seq.cache=lval;
    //else 
    seq.cache = ((wxSpinCtrl*)FindWindow(CD_SEQ_CACHEVAL))->GetValue();
    if (!seq.cache || seq.cache==0x80000000)
      { client_error(cdp, INT_OUT_OF_BOUND);  cd_Signalize(cdp);  FindWindow(CD_SEQ_CACHEVAL)->SetFocus();  return false; }
  }
  return true;
}

bool SequenceDesignerDlg::Validate(void)
{
  return true;
}

/////////////////////////// virtual methods (commented in class any_designer): //////////////////////////////////
char * SequenceDesignerDlg::make_source(bool alter)
{ return sequence_to_source(cdp, &seq, alter); }

bool SequenceDesignerDlg::IsChanged(void) const
{ return changed; }

wxString SequenceDesignerDlg::SaveQuestion(void) const
{ return modifying() ? 
    _("Your changes have not been saved.\nDo you want to save your changes to the sequence?") :
    _("The sequence has not been created yet.\nDo you want to save the sequence?");
}

bool SequenceDesignerDlg::save_design(bool save_as)
// Saves the domain, returns true on success.
{ if (!save_design_generic(save_as, seq.name, 0)) return false;  // using the generic save from any_designer
  changed=false;
  set_designer_caption();
  return true;
}

void SequenceDesignerDlg::OnCommand(wxCommandEvent & event)
{ OnxCommand(event); }

void SequenceDesignerDlg::destroy_designer(void)
{ 
  persistent_coords->pos=GetPosition();  
  persistent_coords->coords_updated=true;  // valid and should be saved
  if (global_style!=ST_POPUP) EndModal(1);  else Destroy();  // must not call Close, Close is directed here
}

void SequenceDesignerDlg::OnxCommand(wxCommandEvent & event)
{ 
  switch (event.GetId())
  { case TD_CPM_SAVE:
      if (!IsChanged()) break;  // no message if no changes
      if (!save_design(false)) break;  // error saving, break
      info_box(_("Your design has been saved."));
      break;
    case TD_CPM_SAVE_AS:
      if (!save_design(true)) break;  // error saving, break
      info_box(_("Your design was saved under a new name."));
      break;
    case TD_CPM_EXIT:  // closing by command (may be cancelled)
      if (exit_request(this, true))
        destroy_designer();
      break;
    case TD_CPM_EXIT_UNCOND:  // closing by global event (cannot be cancelled)
      exit_request(this, false);
      destroy_designer();
      break;
    case TD_CPM_SQL:
    { if (!TransferDataFromWindow()) return;  
      ShowTextDlg dlg(this);  bool faked=false;
      if (!*seq.name) 
        { strcpy(seq.name, "<name>");  faked=true; } // fake name for a new domain which does not have a name externally specified
      char * src=make_source(modifying());
      if (src) 
      { ((wxTextCtrl*)dlg.FindWindow(CD_TD_SQL))->SetValue(wxString(src, *cdp->sys_conv));
        dlg.ShowModal();
        corefree(src); 
      }
      if (faked) *seq.name=0;
      break;
    }
    case TD_CPM_CHECK:
    { bool faked=false;
      if (!TransferDataFromWindow()) return;  
      if (!*seq.name) 
        { strcpy(seq.name, "<name>");  faked=true; } // fake name for a new domain which does not have a name externally specified
      char * src=make_source(modifying());
      if (src) 
      { *src=(char)QUERY_TEST_MARK;
        if (cd_SQL_execute(cdp, src, NULL)) cd_Signalize(cdp);
        else info_box(_("The design is valid."));
        corefree(src); 
      }
      if (faked) *seq.name=0;
      break;
    }
    case TD_CPM_HELP:
      wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_sequence.html"));  
      break;
  }
}

void SequenceDesignerDlg::OnKeyDown( wxKeyEvent& event )
{ 
  OnCommonKeyDown(event);  // event.Skip() is inside
}

t_tool_info SequenceDesignerDlg::seq_tool_info[SEQ_TOOL_COUNT+1] = {
  t_tool_info(TD_CPM_SAVE,    File_save_xpm,   wxTRANSLATE("Save design")),
  t_tool_info(TD_CPM_EXIT,    exit_xpm,        wxTRANSLATE("Exit")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_CHECK,   validateSQL_xpm, wxTRANSLATE("Validate")),
  t_tool_info(TD_CPM_SQL,     showSQLtext_xpm, wxTRANSLATE("Show SQL")),
  t_tool_info(-1, NULL, NULL),
  t_tool_info(TD_CPM_HELP,    help_xpm,        wxTRANSLATE("Help")),
  t_tool_info(0,  NULL, NULL)
};

wxMenu * SequenceDesignerDlg::get_designer_menu(void)
{ 
#ifndef RECREATE_MENUS
  if (designer_menu) return designer_menu;
#endif
 // create menu and add the common items: 
  any_designer::get_designer_menu();  
 // add specific items:
  AppendXpmItem(designer_menu, TD_CPM_SQL, _("Show S&QL"), showSQLtext_xpm);
  return designer_menu;
}

/*!
 * Should we show tooltips?
 */

bool SequenceDesignerDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SEQ_MAXVAL
 */

void SequenceDesignerDlg::OnCdSeqMaxvalUpdated( wxCommandEvent& event )
{ if (((wxTextCtrl*)FindWindow(CD_SEQ_MAXVAL))->GetValue().Length() > 0)
    ((wxCheckBox*)FindWindow(CD_SEQ_MAX))->SetValue(true);
  if (!feedback_disabled) changed=true;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SEQ_MINVAL
 */

void SequenceDesignerDlg::OnCdSeqMinvalUpdated( wxCommandEvent& event )
{ if (((wxTextCtrl*)FindWindow(CD_SEQ_MINVAL))->GetValue().Length() > 0)
    ((wxCheckBox*)FindWindow(CD_SEQ_MIN))->SetValue(true);
  if (!feedback_disabled) changed=true;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for CD_SEQ_CACHEVAL
 */

void SequenceDesignerDlg::OnCdSeqCachevalUpdated( wxSpinEvent& event )
{ if (((wxSpinCtrl*)FindWindow(CD_SEQ_CACHEVAL))->GetValue() > 0)
    ((wxCheckBox*)FindWindow(CD_SEQ_CACHE))->SetValue(true);
  if (!feedback_disabled) changed=true;
  event.Skip();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SEQ_CACHEVAL
 */

void SequenceDesignerDlg::OnCdSeqCachevalTextUpdated( wxCommandEvent& event )
{ if (((wxSpinCtrl*)FindWindow(CD_SEQ_CACHEVAL))->GetValue() > 0)
    ((wxCheckBox*)FindWindow(CD_SEQ_CACHE))->SetValue(true);
  if (!feedback_disabled) changed=true;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_SEQ_MAX
 */

void SequenceDesignerDlg::OnCdSeqMaxClick( wxCommandEvent& event )
{ FindWindow(CD_SEQ_MAXVAL)->Enable(((wxCheckBox*)FindWindow(CD_SEQ_MAX))->GetValue());
  if (!feedback_disabled) changed=true;
  event.Skip();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_SEQ_MIN
 */

void SequenceDesignerDlg::OnCdSeqMinClick( wxCommandEvent& event )
{ FindWindow(CD_SEQ_MINVAL)->Enable(((wxCheckBox*)FindWindow(CD_SEQ_MIN))->GetValue());
  if (!feedback_disabled) changed=true;
  event.Skip();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_SEQ_CACHE
 */

void SequenceDesignerDlg::OnCdSeqCacheClick( wxCommandEvent& event )
{ FindWindow(CD_SEQ_CACHEVAL)->Enable(((wxCheckBox*)FindWindow(CD_SEQ_CACHE))->GetValue());
  if (!feedback_disabled) changed=true;
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SEQ_START
 */

void SequenceDesignerDlg::OnCdSeqStartUpdated( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SEQ_STEP
 */

void SequenceDesignerDlg::OnCdSeqStepUpdated( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_SEQ_CYCLE
 */

void SequenceDesignerDlg::OnCdSeqCycleClick( wxCommandEvent& event )
{ if (!feedback_disabled) changed=true;
  event.Skip();
}

void seq_delete_bitmaps(void)
{ delete_bitmaps(SequenceDesignerDlg::seq_tool_info); }

void SequenceDesignerDlg::OnActivate(wxActivateEvent & event)
{ if (event.GetActive())
    Activated();
  else
    Deactivated();
  event.Skip();
}

void SequenceDesignerDlg::OnCloseWindow(wxCloseEvent& event)
{ wxCommandEvent cmd; 
  cmd.SetId(event.CanVeto() ? TD_CPM_EXIT : TD_CPM_EXIT_UNCOND);
  OnxCommand(cmd);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void SequenceDesignerDlg::OnOkClick( wxCommandEvent& event )
{ if (IsChanged()) 
    if (!save_design(false)) return;  // error saving, break
  destroy_designer();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void SequenceDesignerDlg::OnCancelClick( wxCommandEvent& event )
{ 
  destroy_designer();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SEQ_VALIDATE
 */

void SequenceDesignerDlg::OnCdSeqValidateClick( wxCommandEvent& event )
{ wxCommandEvent cmd;
  cmd.SetId(TD_CPM_CHECK);
  OnxCommand(cmd);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SEQ_SQL
 */

void SequenceDesignerDlg::OnCdSeqSqlClick( wxCommandEvent& event )
{ wxCommandEvent cmd;
  cmd.SetId(TD_CPM_SQL);
  OnxCommand(cmd);
}



/*!
 * Get bitmap resources
 */

wxBitmap SequenceDesignerDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin SequenceDesignerDlg bitmap retrieval
    return wxNullBitmap;
////@end SequenceDesignerDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon SequenceDesignerDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin SequenceDesignerDlg icon retrieval
    return wxNullIcon;
////@end SequenceDesignerDlg icon retrieval
}
