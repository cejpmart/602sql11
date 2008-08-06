/////////////////////////////////////////////////////////////////////////////
// Name:        NewTriggerWiz.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/17/04 09:26:01
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////
/* Theory
- disabling the Next/Finish button is not supported by wx
- displaying both Next and Finish buttons is not supported by wx
- PageChanged event is the Activation event! It is received by the page being activated.
- Cannot complete the wizard without specifying the trigger name or table name
*/

#ifdef __GNUG__
//#pragma implementation "NewTriggerWiz.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "NewTriggerWiz.h"

////@begin XPM images
////@end XPM images

/*!
 * NewTriggerWiz type definition
 */

IMPLEMENT_CLASS( NewTriggerWiz, wxWizard )

/*!
 * NewTriggerWiz event table definition
 */

BEGIN_EVENT_TABLE( NewTriggerWiz, wxWizard )

////@begin NewTriggerWiz event table entries
////@end NewTriggerWiz event table entries

END_EVENT_TABLE()

/*!
 * NewTriggerWiz constructors
 */

NewTriggerWiz::NewTriggerWiz(cdp_t cdpIn, const char * tabnameIn)
{ cdp=cdpIn;  
  trigger_name=wxEmptyString;
  trigger_table = wxString(tabnameIn, *cdp->sys_conv);  
  tabobj=NOOBJECT;  // will be defined when exiting page 1, then the attrmap will be init.
  trigger_oper=TO_INSERT;
  explicit_attrs=false;  
  before=false;  statement_granularity=false;
  oldname=wxEmptyString;  newname=wxEmptyString;  
}

NewTriggerWiz::NewTriggerWiz( wxWindow* parent, wxWindowID id, const wxPoint& pos )
{
    Create(parent, id, pos);
}

/*!
 * NewTriggerWiz creator
 */

bool NewTriggerWiz::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos )
{
////@begin NewTriggerWiz member initialisation
    mPage1 = NULL;
    mPage2 = NULL;
    mPage3 = NULL;
////@end NewTriggerWiz member initialisation

////@begin NewTriggerWiz creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS|wxWIZARD_EX_HELPBUTTON);
    wxBitmap wizardBitmap(trigwiz_xpm);
    wxWizard::Create( parent, id, _("Create a new trigger"), wizardBitmap, pos );

    CreateControls();
////@end NewTriggerWiz creation
    SetBackgroundColour(GetDefaultAttributes().colBg);  // needed in wx 2.5.3
    return TRUE;
}

/*!
 * Control creation for NewTriggerWiz
 */

void NewTriggerWiz::CreateControls()
{    
////@begin NewTriggerWiz content construction

    wxWizard* item1 = this;

    TriggerWizardPage1* item2 = new TriggerWizardPage1;
    item2->Create( item1 );
    mPage1 = item2;
    item1->FitToPage(item2);
    TriggerWizardPage2* item12 = new TriggerWizardPage2;
    item12->Create( item1 );
    mPage2 = item12;
    item1->FitToPage(item12);
    TriggerWizardPage3* item17 = new TriggerWizardPage3;
    item17->Create( item1 );
    mPage3 = item17;
    item1->FitToPage(item17);
    wxWizardPageSimple::Chain(item2, item12);
    wxWizardPageSimple::Chain(item12, item17);
////@end NewTriggerWiz content construction
}

/*!
 * Runs the wizard.
 */

bool NewTriggerWiz::Run2()
{
    if (GetChildren().GetCount() > 0)
    {
        wxWizardPage* startPage = mPage1;  //wxDynamicCast(GetChildren().GetFirst()->GetData(), wxWizardPage);
        if (startPage) return RunWizard(startPage);
    }
    return false;
}

/*!
 * Should we show tooltips?
 */

bool NewTriggerWiz::ShowToolTips()
{
    return TRUE;
}

/*!
 * TriggerWizardPage1 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( TriggerWizardPage1, wxWizardPageSimple )

/*!
 * TriggerWizardPage1 event table definition
 */

BEGIN_EVENT_TABLE( TriggerWizardPage1, wxWizardPageSimple )

////@begin TriggerWizardPage1 event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, TriggerWizardPage1::OnTrigWizPage1PageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, TriggerWizardPage1::OnTrigWizPage1PageChanging )
    EVT_WIZARD_HELP( -1, TriggerWizardPage1::OnTrigWizPage1Help )

    EVT_TEXT( CD_TWZ1_NAME, TriggerWizardPage1::OnCdTwz1NameUpdated )

    EVT_LISTBOX( CD_TWZ1_TABLE, TriggerWizardPage1::OnCdTwz1TableSelected )

////@end TriggerWizardPage1 event table entries

END_EVENT_TABLE()

/*!
 * TriggerWizardPage1 constructors
 */

TriggerWizardPage1::TriggerWizardPage1( )
{
}

TriggerWizardPage1::TriggerWizardPage1( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPage creator
 */

bool TriggerWizardPage1::Create( wxWizard* parent )
{
////@begin TriggerWizardPage1 member initialisation
    mName = NULL;
    mTable = NULL;
////@end TriggerWizardPage1 member initialisation

////@begin TriggerWizardPage1 creation
    wxBitmap wizardBitmap;
    wxWizardPageSimple::Create( parent, NULL, NULL, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end TriggerWizardPage1 creation
    return TRUE;
}

/*!
 * Control creation for WizardPage
 */

void TriggerWizardPage1::CreateControls()
{    
////@begin TriggerWizardPage1 content construction

    TriggerWizardPage1* item2 = this;

    wxBoxSizer* item3 = new wxBoxSizer(wxVERTICAL);
    item2->SetSizer(item3);
    item2->SetAutoLayout(TRUE);

    wxStaticText* item4 = new wxStaticText;
    item4->Create( item2, wxID_STATIC, _("Trigger &name:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item4, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* item5 = new wxBoxSizer(wxHORIZONTAL);
    item3->Add(item5, 0, wxGROW|wxALL, 0);

    item5->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxTextCtrl* item7 = new wxTextCtrl;
    item7->Create( item2, CD_TWZ1_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mName = item7;
    item5->Add(item7, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* item8 = new wxStaticText;
    item8->Create( item2, wxID_STATIC, _("Trigger &belongs to the table:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add(item8, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* item9 = new wxBoxSizer(wxHORIZONTAL);
    item3->Add(item9, 1, wxGROW|wxALL, 0);

    item9->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxString* item11Strings = NULL;
    wxListBox* item11 = new wxListBox;
    item11->Create( item2, CD_TWZ1_TABLE, wxDefaultPosition, wxDefaultSize, 0, item11Strings, wxLB_SINGLE|wxLB_SORT );
    mTable = item11;
    item9->Add(item11, 1, wxGROW|wxALL, 5);

////@end TriggerWizardPage1 content construction
    NewTriggerWiz * ntw = (NewTriggerWiz *)GetParent();
   // limit name length:
    mName->SetMaxLength(OBJ_NAME_LEN);
   // fill table names, except for ODBC a linked tables:
    void * en = get_object_enumerator(ntw->cdp, CATEG_TABLE, NULL);
    tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
    while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
      if (!(flags & (CO_FLAG_ODBC | CO_FLAG_LINK)))
        mTable->Append(wxString(name, *ntw->cdp->sys_conv), (void*)(size_t)objnum);
    free_object_enumerator(en);
    int ind = mTable->FindString(ntw->GetTriggerTable());
    if (ind!=-1) mTable->SetSelection(ind);
}

/*!
 * Should we show tooltips?
 */

bool TriggerWizardPage1::ShowToolTips()
{
    return TRUE;
}

/*!
 * TriggerWizardPage2 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( TriggerWizardPage2, wxWizardPageSimple )

/*!
 * TriggerWizardPage2 event table definition
 */

BEGIN_EVENT_TABLE( TriggerWizardPage2, wxWizardPageSimple )

////@begin TriggerWizardPage2 event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, TriggerWizardPage2::OnTrigWizPage2PageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, TriggerWizardPage2::OnTrigWizPage2PageChanging )
    EVT_WIZARD_HELP( -1, TriggerWizardPage2::OnTrigWizPage2Help )

    EVT_RADIOBOX( CD_TWZ2_OPER, TriggerWizardPage2::OnCdTwz2OperSelected )

    EVT_CHECKBOX( CD_TWZ2_EXPLIC, TriggerWizardPage2::OnCdTwz2ExplicClick )

////@end TriggerWizardPage2 event table entries

END_EVENT_TABLE()

/*!
 * TriggerWizardPage2 constructors
 */

TriggerWizardPage2::TriggerWizardPage2( )
{
}

TriggerWizardPage2::TriggerWizardPage2( wxWizard* parent )
{
    Create( parent );
}

/*!
 * TriggerWizardPage2 creator
 */

bool TriggerWizardPage2::Create( wxWizard* parent )
{
////@begin TriggerWizardPage2 member initialisation
    mOper = NULL;
    mExplic = NULL;
    mList = NULL;
////@end TriggerWizardPage2 member initialisation

////@begin TriggerWizardPage2 creation
    wxBitmap wizardBitmap;
    wxWizardPageSimple::Create( parent, NULL, NULL, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end TriggerWizardPage2 creation
    return TRUE;
}

/*!
 * Control creation for TriggerWizardPage2
 */

void TriggerWizardPage2::CreateControls()
{    
////@begin TriggerWizardPage2 content construction

    TriggerWizardPage2* item12 = this;

    wxBoxSizer* item13 = new wxBoxSizer(wxVERTICAL);
    item12->SetSizer(item13);
    item12->SetAutoLayout(TRUE);

    wxString item14Strings[] = {
        _("&INSERT"),
        _("&DELETE"),
        _("&UPDATE")
    };
    wxRadioBox* item14 = new wxRadioBox;
    item14->Create( item12, CD_TWZ2_OPER, _("Trigger is fired by the selected operation:"), wxDefaultPosition, wxDefaultSize, 3, item14Strings, 1, wxRA_SPECIFY_COLS );
    mOper = item14;
    item13->Add(item14, 0, wxALIGN_LEFT|wxALL, 5);

    wxCheckBox* item15 = new wxCheckBox;
    item15->Create( item12, CD_TWZ2_EXPLIC, _("Fire only when the selected colums are updated"), wxDefaultPosition, wxDefaultSize, 0 );
    mExplic = item15;
    item15->SetValue(FALSE);
    item13->Add(item15, 0, wxALIGN_LEFT|wxALL, 5);

    wxString* item16Strings = NULL;
    wxCheckListBox* item16 = new wxCheckListBox;
    item16->Create( item12, CD_TWZ2_LIST, wxDefaultPosition, wxDefaultSize, 0, item16Strings, 0 );
    mList = item16;
    item13->Add(item16, 1, wxGROW|wxALL, 5);

////@end TriggerWizardPage2 content construction
}

/*!
 * Should we show tooltips?
 */

bool TriggerWizardPage2::ShowToolTips()
{
    return TRUE;
}

/*!
 * TriggerWizardPage3 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( TriggerWizardPage3, wxWizardPageSimple )

/*!
 * TriggerWizardPage3 event table definition
 */

BEGIN_EVENT_TABLE( TriggerWizardPage3, wxWizardPageSimple )

////@begin TriggerWizardPage3 event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, TriggerWizardPage3::OnTrigWizPage3PageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, TriggerWizardPage3::OnTrigWizPage3PageChanging )
    EVT_WIZARD_HELP( -1, TriggerWizardPage3::OnTrigWizPage3Help )

    EVT_CHECKBOX( CD_TWZ3_STAT, TriggerWizardPage3::OnCdTwz3StatClick )

////@end TriggerWizardPage3 event table entries

END_EVENT_TABLE()

/*!
 * TriggerWizardPage3 constructors
 */

TriggerWizardPage3::TriggerWizardPage3( )
{
}

TriggerWizardPage3::TriggerWizardPage3( wxWizard* parent )
{
    Create( parent );
}

/*!
 * TriggerWizardPage3 creator
 */

bool TriggerWizardPage3::Create( wxWizard* parent )
{
////@begin TriggerWizardPage3 member initialisation
    mTime = NULL;
    mStat = NULL;
    mOldCapt = NULL;
    mOld = NULL;
    mNewCapt = NULL;
    mNew = NULL;
////@end TriggerWizardPage3 member initialisation

////@begin TriggerWizardPage3 creation
    wxBitmap wizardBitmap;
    wxWizardPageSimple::Create( parent, NULL, NULL, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end TriggerWizardPage3 creation
    return TRUE;
}

/*!
 * Control creation for TriggerWizardPage3
 */

void TriggerWizardPage3::CreateControls()
{    
////@begin TriggerWizardPage3 content construction

    TriggerWizardPage3* item17 = this;

    wxBoxSizer* item18 = new wxBoxSizer(wxVERTICAL);
    item17->SetSizer(item18);
    item17->SetAutoLayout(TRUE);

    wxString item19Strings[] = {
        _("&Before the operation"),
        _("&After the operation")
    };
    wxRadioBox* item19 = new wxRadioBox;
    item19->Create( item17, CD_TWZ3_TIME, _("Trigger should be fired:"), wxDefaultPosition, wxDefaultSize, 2, item19Strings, 1, wxRA_SPECIFY_COLS );
    mTime = item19;
    item18->Add(item19, 0, wxALIGN_LEFT|wxALL, 5);

    item18->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxCheckBox* item21 = new wxCheckBox;
    item21->Create( item17, CD_TWZ3_STAT, _("&Run only once per statement"), wxDefaultPosition, wxDefaultSize, 0 );
    mStat = item21;
    item21->SetValue(FALSE);
    item18->Add(item21, 0, wxALIGN_LEFT|wxALL, 5);

    item18->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStaticText* item23 = new wxStaticText;
    item23->Create( item17, CD_TWZ3_OLD_CAPT, _("Reference the &OLD values as:"), wxDefaultPosition, wxDefaultSize, 0 );
    mOldCapt = item23;
    item18->Add(item23, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item24 = new wxTextCtrl;
    item24->Create( item17, CD_TWZ3_OLD, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mOld = item24;
    item18->Add(item24, 0, wxGROW|wxALL, 5);

    wxStaticText* item25 = new wxStaticText;
    item25->Create( item17, CD_TWZ3_NEW_CAPT, _("Reference the &NEW values as:"), wxDefaultPosition, wxDefaultSize, 0 );
    mNewCapt = item25;
    item18->Add(item25, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item26 = new wxTextCtrl;
    item26->Create( item17, CD_TWZ3_NEW, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mNew = item26;
    item18->Add(item26, 0, wxGROW|wxALL, 5);

////@end TriggerWizardPage3 content construction
    NewTriggerWiz * ntw = (NewTriggerWiz *)GetParent();
   // limit name length:
    mOld->SetMaxLength(OBJ_NAME_LEN);
    mNew->SetMaxLength(OBJ_NAME_LEN);
   // show data independent of other pages:
    mTime->SetSelection(ntw->before ? 0 : 1);
    mStat->SetValue(ntw->statement_granularity);
}

/*!
 * Should we show tooltips?
 */

bool TriggerWizardPage3::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for TRIG_WIZ_PAGE1
 */

void TriggerWizardPage1::OnTrigWizPage1PageChanged( wxWizardEvent& event )
// No need to show data because the data cannot be changed on other pages
{ event.Skip(); }


/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for TRIG_WIZ_PAGE1
 */

void TriggerWizardPage1::OnTrigWizPage1PageChanging( wxWizardEvent& event )
{   NewTriggerWiz * ntw = (NewTriggerWiz *)GetParent();
   // get trigger name, check for empty and used:
    ntw->SetTriggerName(mName->GetValue());
    ntw->trigger_name.Trim(true);  ntw->trigger_name.Trim(false);  // must not use GetTriggerName, this would change the copy only
    if (ntw->GetTriggerName().IsEmpty())
      { wxBell();  mName->SetFocus();  event.Veto();  return; }
    if (!convertible_name(ntw->cdp, ntw, ntw->GetTriggerName()))
      { event.Veto();  return; }
    tobjnum objnum;
    if (!cd_Find2_object(ntw->cdp, ntw->GetTriggerName().mb_str(*ntw->cdp->sys_conv), NULL, CATEG_TRIGGER, &objnum))
      { error_box(_("An object with the same name and category already exists."), ntw);  event.Veto();  return; }
   // get table name and number:
    int ind = mTable->GetSelection();
    if (ind==-1)
      { wxBell();  mTable->SetFocus();  event.Veto();  return; }
    ntw->SetTriggerTable(mTable->GetString(ind));
    ttablenum newtabobj=(ttablenum)(size_t)mTable->GetClientData(ind);
    if (ntw->tabobj!=newtabobj) // table changed
    { const d_table * td = cd_get_table_d(ntw->cdp, newtabobj, CATEG_TABLE);
      if (td!=NULL)
      { ntw->attrmap.init(td->attrcnt);  // more than necessary because indexing only columns from the list box
        ntw->attrmap.clear();
        release_table_d(td);
      }
      ntw->tabobj=newtabobj;
    }
    event.Skip();
}

/*!
 * wxEVT_WIZARD_HELP event handler for TRIG_WIZ_PAGE1
 */

void TriggerWizardPage1::OnTrigWizPage1Help( wxWizardEvent& event )
{ wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_trigger.html"));
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TWZ1_NAME
 */

void TriggerWizardPage1::OnCdTwz1NameUpdated( wxCommandEvent& event )
// Not used because cannot disable the Next button
{ event.Skip(); }

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for CD_TWZ1_TABLE
 */

void TriggerWizardPage1::OnCdTwz1TableSelected( wxCommandEvent& event )
// Clearing the column map when a new table is selected <- will be done when exiting page 1
{ event.Skip(); }

void TriggerWizardPage2::enabling(void)
{ NewTriggerWiz * ntw = (NewTriggerWiz *)GetParent();
  mExplic->Enable((t_trigger_oper)mOper->GetSelection()==TO_UPDATE);
  mList->Enable((t_trigger_oper)mOper->GetSelection()==TO_UPDATE && mExplic->GetValue());
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for TRIG_WIZ_PAGE2
 */

void TriggerWizardPage2::OnTrigWizPage2PageChanged( wxWizardEvent& event )
// Page 2 being activated, must show columns if the table selected on Page 1
{ NewTriggerWiz * ntw = (NewTriggerWiz *)GetParent();
  mOper->SetSelection((int)ntw->trigger_oper);
  enabling();
  mList->Clear();
 // fill and check columns (must fill event if >trigger_oper!=TO_UPDATE in the moment, may be changed soon):
  mExplic->SetValue(ntw->GetExplicitAttrs());
  const d_table * td = cd_get_table_d(ntw->cdp, ntw->tabobj, CATEG_TABLE);
  if (td!=NULL)
  { for (int i=first_user_attr(td);  i<td->attrcnt;  i++)
    { const d_attr * att = ATTR_N(td,i);
      if (att->multi==1)
      { int ind = mList->Append(wxString(att->name, *ntw->cdp->sys_conv));
        if (ntw->attrmap.has(ind)) mList->Check(ind);
      }
    }
    release_table_d(td);
  }
  event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for TRIG_WIZ_PAGE2
 */

void TriggerWizardPage2::OnTrigWizPage2PageChanging( wxWizardEvent& event )
// Saves data.
{ NewTriggerWiz * ntw = (NewTriggerWiz *)GetParent();
  ntw->SetTriggerOper((t_trigger_oper)mOper->GetSelection());
 // get columns:
  if (ntw->trigger_oper==TO_UPDATE)
  { ntw->SetExplicitAttrs(mExplic->GetValue());
    ntw->attrmap.clear();
    for (int i=0;  i<mList->GetCount();  i++)
      if (mList->IsChecked(i))
        ntw->attrmap.add(i);
  }
  else 
    ntw->SetExplicitAttrs(false);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TWZ2_OPER
 */

void TriggerWizardPage2::OnCdTwz2OperSelected( wxCommandEvent& event )
// Enables or disables controls related to the UPDATE triggers.
{ enabling();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TWZ2_EXPLIC
 */

void TriggerWizardPage2::OnCdTwz2ExplicClick( wxCommandEvent& event )
{ enabling();
  event.Skip();
}


/*!
 * wxEVT_WIZARD_HELP event handler for TRIG_WIZ_PAGE2
 */

void TriggerWizardPage2::OnTrigWizPage2Help( wxWizardEvent& event )
{ wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_trigger.html"));
}

void TriggerWizardPage3::enabling(void)
{ NewTriggerWiz * ntw = (NewTriggerWiz *)GetParent();
  bool row_gran = !mStat->GetValue();
  mOld    ->Enable(ntw->trigger_oper!=TO_INSERT && row_gran);
  mOldCapt->Enable(ntw->trigger_oper!=TO_INSERT && row_gran);
  mNew    ->Enable(ntw->trigger_oper!=TO_DELETE && row_gran);
  mNewCapt->Enable(ntw->trigger_oper!=TO_DELETE && row_gran);
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for TRIG_WIZ_PAGE3
 */

void TriggerWizardPage3::OnTrigWizPage3PageChanged( wxWizardEvent& event )
// No need to show data because the data cannot be changed on other pages.
{ enabling();
  event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for TRIG_WIZ_PAGE3
 */

void TriggerWizardPage3::OnTrigWizPage3PageChanging( wxWizardEvent& event )
// Save values.
{ NewTriggerWiz * ntw = (NewTriggerWiz *)GetParent();
  ntw->before = mTime->GetSelection() == 0;
  ntw->statement_granularity = mStat->GetValue();
  ntw->oldname = mOld->GetValue();
  ntw->oldname.Trim(false);  ntw->oldname.Trim(true);
  if (!ntw->oldname.IsEmpty())
    if (!convertible_name(ntw->cdp, ntw, ntw->oldname))
      { event.Veto();  return; }
  ntw->newname = mNew->GetValue();
  ntw->newname.Trim(false);  ntw->newname.Trim(true);
  if (!ntw->newname.IsEmpty())
    if (!convertible_name(ntw->cdp, ntw, ntw->newname))
      { event.Veto();  return; }
  event.Skip();
}

/*!
 * wxEVT_WIZARD_HELP event handler for TRIG_WIZ_PAGE3
 */

void TriggerWizardPage3::OnTrigWizPage3Help( wxWizardEvent& event )
{ wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/des_trigger.html"));
}



/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TWZ3_STAT
 */

void TriggerWizardPage3::OnCdTwz3StatClick( wxCommandEvent& event )
{ enabling();
  event.Skip();
}


/*!
 * Runs the wizard.
 */

bool NewTriggerWiz::Run()
{
    if (GetChildren().GetCount() > 0)
    {
        wxWizardPage* startPage = wxDynamicCast(GetChildren().GetFirst()->GetData(), wxWizardPage);
        if (startPage) return RunWizard(startPage);
    }
    return false;
}


