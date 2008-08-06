/////////////////////////////////////////////////////////////////////////////
// Name:        NewDataTransfer95Dlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     07/22/05 08:36:25
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "NewDataTransfer95Dlg.h"
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

#include "NewDataTransfer95Dlg.h"

////@begin XPM images
////@end XPM images

bool any_odbc_ds_connected(void)
{ 
  for (t_avail_server * server = available_servers;  server;  server = server->next)
    if (server->odbc && server->conn)
      return true;
  return false;
}

/*!
 * NewDataTransfer95Dlg type definition
 */

IMPLEMENT_CLASS( NewDataTransfer95Dlg, wxWizard )

/*!
 * NewDataTransfer95Dlg event table definition
 */

BEGIN_EVENT_TABLE( NewDataTransfer95Dlg, wxWizard )

////@begin NewDataTransfer95Dlg event table entries
    EVT_WIZARD_HELP( ID_MYWIZARD, NewDataTransfer95Dlg::OnMywizardHelp )

////@end NewDataTransfer95Dlg event table entries

END_EVENT_TABLE()

/*!
 * NewDataTransfer95Dlg constructors
 */

NewDataTransfer95Dlg::NewDataTransfer95Dlg(t_ie_run * dsgnIn, bool xmlIn, t_xsd_gen_params * gen_paramsIn)
{ dsgn=dsgnIn;  gen_params=gen_paramsIn;
  xml_transport=xmlIn;
  mPageMainSel=NULL;  mPageDataSource=NULL;  mPageDataTarget=NULL;  mPageXML=NULL;  // used in FitToPage before creation!
  mPageText=NULL;  mPageXSD = NULL;
  updating=false;
}


/*!
 * NewDataTransfer95Dlg creator
 */

bool NewDataTransfer95Dlg::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos )
{
////@begin NewDataTransfer95Dlg member initialisation
    mPageMainSel = NULL;
    mPageDataSource = NULL;
    mPageDataTarget = NULL;
    mPageXML = NULL;
    mPageText = NULL;
    mPageXSD = NULL;
    mPageDbxml = NULL;
////@end NewDataTransfer95Dlg member initialisation

////@begin NewDataTransfer95Dlg creation
    SetExtraStyle(GetExtraStyle()|wxWIZARD_EX_HELPBUTTON);
    wxBitmap wizardBitmap(GetBitmapResource(wxT("trigwiz")));
    wxWizard::Create( parent, id, _("Data Transport Design Wizard"), wizardBitmap, pos );

    CreateControls();
////@end NewDataTransfer95Dlg creation
    SetBackgroundColour(GetDefaultAttributes().colBg);  // needed in wx 2.5.3
    return TRUE;
}

/*!
 * Control creation for NewDataTransfer95Dlg
 */

void NewDataTransfer95Dlg::CreateControls()
{    
////@begin NewDataTransfer95Dlg content construction
    wxWizard* itemWizard1 = this;

    mPageMainSel = new WizardPage2;
    mPageMainSel->Create( itemWizard1 );

    itemWizard1->FitToPage(mPageMainSel);
    mPageDataSource = new WizardPage;
    mPageDataSource->Create( itemWizard1 );

    itemWizard1->FitToPage(mPageDataSource);
    mPageDataTarget = new WizardPage1;
    mPageDataTarget->Create( itemWizard1 );

    itemWizard1->FitToPage(mPageDataTarget);
    mPageXML = new WizardPage3;
    mPageXML->Create( itemWizard1 );

    itemWizard1->FitToPage(mPageXML);
    mPageText = new WizardPage4;
    mPageText->Create( itemWizard1 );

    itemWizard1->FitToPage(mPageText);
    mPageXSD = new WizardPageXSD;
    mPageXSD->Create( itemWizard1 );

    itemWizard1->FitToPage(mPageXSD);
    mPageDbxml = new WizardPageDbxml;
    mPageDbxml->Create( itemWizard1 );

    itemWizard1->FitToPage(mPageDbxml);
    wxWizardPageSimple* lastPage = NULL;
////@end NewDataTransfer95Dlg content construction
}

/*!
 * Runs the wizard.
 */

bool NewDataTransfer95Dlg::Run()
{
    wxWizardPage* startPage = NULL;
    wxWindowListNode* node = GetChildren().GetFirst();
    while (node)
    {
        wxWizardPage* startPage = wxDynamicCast(node->GetData(), wxWizardPage);
        if (startPage) return RunWizard(startPage);
        node = node->GetNext();
    }
    return FALSE;
}

bool NewDataTransfer95Dlg::Run2(void)
// Normal start is with xml_transport==true, when started from a table, xml_transport==false
{ return RunWizard(xml_transport ? (wxWizardPage*)mPageMainSel : (wxWizardPage*)mPageDataSource); }

bool NewDataTransfer95Dlg::RunUpdate(int page)
{ updating=true;
  SetTitle(_("Update Data Transport Design"));
  return RunWizard(page==1 ? (wxWizardPage*)mPageDataSource : page==2 ? (wxWizardPage*)mPageDataTarget :
                   (wxWizardPage*)mPageText); 
}

/*!
 * Should we show tooltips?
 */

bool NewDataTransfer95Dlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap NewDataTransfer95Dlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NewDataTransfer95Dlg bitmap retrieval
    if (name == wxT("trigwiz"))
    {
        wxBitmap bitmap(trigwiz_xpm);
        return bitmap;
    }
    return wxNullBitmap;
////@end NewDataTransfer95Dlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NewDataTransfer95Dlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NewDataTransfer95Dlg icon retrieval
    return wxNullIcon;
////@end NewDataTransfer95Dlg icon retrieval
}

/*!
 * WizardPage2 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPage2, wxWizardPage )

/*!
 * WizardPage2 event table definition
 */

BEGIN_EVENT_TABLE( WizardPage2, wxWizardPage )

////@begin WizardPage2 event table entries
    EVT_WIZARD_PAGE_CHANGING( -1, WizardPage2::OnWizardpage2PageChanging )

////@end WizardPage2 event table entries

END_EVENT_TABLE()

/*!
 * WizardPage2 constructors
 */

WizardPage2::WizardPage2( )
{
}

WizardPage2::WizardPage2( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPage2 creator
 */

bool WizardPage2::Create( wxWizard* parent )
{
////@begin WizardPage2 member initialisation
    mMainSel = NULL;
////@end WizardPage2 member initialisation

////@begin WizardPage2 creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPage::Create( parent, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end WizardPage2 creation
    return TRUE;
}

/*!
 * Control creation for WizardPage2
 */

void WizardPage2::CreateControls()
{    
////@begin WizardPage2 content construction
    WizardPage2* itemWizardPage2 = this;

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage2->SetSizer(itemBoxSizer3);

    wxString mMainSelStrings[] = {
        _("&XML files"),
        _("&Non-XML file formats")
    };
    mMainSel = new wxRadioBox;
    mMainSel->Create( itemWizardPage2, CD_TR_MAIN_SEL, _("Create import / export mapping for:"), wxDefaultPosition, wxDefaultSize, 2, mMainSelStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer3->Add(mMainSel, 0, wxGROW|wxALL, 5);

////@end WizardPage2 content construction
    NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
    mMainSel->SetSelection(dlg->xml_transport ? 0 : 1);  // used when wizars started on the data source page and the Back pressed
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE2
 */

void WizardPage2::OnWizardpage2PageChanging( wxWizardEvent& ev )
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  dlg->xml_transport = mMainSel->GetSelection()==0;
  if (dlg->xml_transport && !check_xml_library()) ev.Veto();
  else ev.Skip();
}

/*!
 * Should we show tooltips?
 */

bool WizardPage2::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap WizardPage2::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WizardPage2 bitmap retrieval
    return wxNullBitmap;
////@end WizardPage2 bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WizardPage2::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WizardPage2 icon retrieval
    return wxNullIcon;
////@end WizardPage2 icon retrieval
}

/*!
 * WizardPage type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPage, wxWizardPage )

/*!
 * WizardPage event table definition
 */

BEGIN_EVENT_TABLE( WizardPage, wxWizardPage )

////@begin WizardPage event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, WizardPage::OnWizardpagePageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, WizardPage::OnWizardpagePageChanging )

    EVT_RADIOBOX( CD_TR_SOURCE_ORIGIN, WizardPage::OnCdTrSourceOriginSelected )

    EVT_CHECKBOX( CD_TR_SOURCE_VIEW, WizardPage::OnCdTrSourceViewClick )

    EVT_COMBOBOX( CD_TR_SOURCE_TYPE, WizardPage::OnCdTrSourceTypeSelected )

    EVT_BUTTON( CD_TR_SOURCE_NAME_SEL, WizardPage::OnCdTrSourceNameSelClick )

    EVT_COMBOBOX( CD_TR_SOURCE_SERVER, WizardPage::OnCdTrSourceServerSelected )

    EVT_COMBOBOX( CD_TR_SOURCE_SCHEMA, WizardPage::OnCdTrSourceSchemaSelected )

////@end WizardPage event table entries

END_EVENT_TABLE()

/*!
 * WizardPage constructors
 */

WizardPage::WizardPage( )
{
}

WizardPage::WizardPage( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPage creator
 */

bool WizardPage::Create( wxWizard* parent )
{
////@begin WizardPage member initialisation
    mSourceOrigin = NULL;
    mSourceView = NULL;
    mSourceTypeC = NULL;
    mSourceType = NULL;
    mSrcEncC = NULL;
    mSourceEnc = NULL;
    mSourceFnameC = NULL;
    mSourceFname = NULL;
    mSourceNameSel = NULL;
    mSourceServerC = NULL;
    mSourceServer = NULL;
    mSourceSchemaC = NULL;
    mSourceSchema = NULL;
    mSrcObjectC = NULL;
    mSrcObject = NULL;
    mConditionC = NULL;
    mCondition = NULL;
////@end WizardPage member initialisation

////@begin WizardPage creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPage::Create( parent, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end WizardPage creation
    return TRUE;
}

/*!
 * Control creation for WizardPage
 */

void WizardPage::CreateControls()
{    
////@begin WizardPage content construction
    WizardPage* itemWizardPage5 = this;

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage5->SetSizer(itemBoxSizer6);

    wxString mSourceOriginStrings[] = {
        _("602SQL &Database"),
        _("&ODBC Data Source"),
        _("&External file")
    };
    mSourceOrigin = new wxRadioBox;
    mSourceOrigin->Create( itemWizardPage5, CD_TR_SOURCE_ORIGIN, _("Take data from"), wxDefaultPosition, wxDefaultSize, 3, mSourceOriginStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer6->Add(mSourceOrigin, 0, wxGROW|wxALL, 5);

    mSourceView = new wxCheckBox;
    mSourceView->Create( itemWizardPage5, CD_TR_SOURCE_VIEW, _("Data source is a VIEW"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mSourceView->SetValue(FALSE);
    itemBoxSizer6->Add(mSourceView, 0, wxALIGN_LEFT|wxALL, 5);

    mSourceTypeC = new wxStaticText;
    mSourceTypeC->Create( itemWizardPage5, CD_TR_SOURCE_TYPE_C, _("Format of the source file:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSourceTypeC->Show(FALSE);
    itemBoxSizer6->Add(mSourceTypeC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mSourceTypeStrings = NULL;
    mSourceType = new wxOwnerDrawnComboBox;
    mSourceType->Create( itemWizardPage5, CD_TR_SOURCE_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mSourceTypeStrings, wxCB_READONLY );
    mSourceType->Show(FALSE);
    itemBoxSizer6->Add(mSourceType, 1, wxGROW|wxALL, 5);

    mSrcEncC = new wxStaticText;
    mSrcEncC->Create( itemWizardPage5, CD_TR_SOURCE_ENC_C, _("Encoding of the source file:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSrcEncC->Show(FALSE);
    itemBoxSizer6->Add(mSrcEncC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mSourceEncStrings = NULL;
    mSourceEnc = new wxOwnerDrawnComboBox;
    mSourceEnc->Create( itemWizardPage5, CD_TR_SOURCE_ENC, _T(""), wxDefaultPosition, wxDefaultSize, 0, mSourceEncStrings, wxCB_DROPDOWN );
    mSourceEnc->Show(FALSE);
    itemBoxSizer6->Add(mSourceEnc, 1, wxGROW|wxALL, 5);

    mSourceFnameC = new wxStaticText;
    mSourceFnameC->Create( itemWizardPage5, CD_TR_SOURCE_FNAME_C, _("Source file name:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSourceFnameC->Show(FALSE);
    itemBoxSizer6->Add(mSourceFnameC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer6->Add(itemBoxSizer14, 0, wxGROW|wxALL, 0);

    mSourceFname = new wxTextCtrl;
    mSourceFname->Create( itemWizardPage5, CD_TR_SOURCE_FNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mSourceFname->Show(FALSE);
    itemBoxSizer14->Add(mSourceFname, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSourceNameSel = new wxButton;
    mSourceNameSel->Create( itemWizardPage5, CD_TR_SOURCE_NAME_SEL, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    mSourceNameSel->Show(FALSE);
    itemBoxSizer14->Add(mSourceNameSel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    mSourceServerC = new wxStaticText;
    mSourceServerC->Create( itemWizardPage5, CD_TR_SOURCE_SERVER_C, _("Server name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mSourceServerC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mSourceServerStrings = NULL;
    mSourceServer = new wxOwnerDrawnComboBox;
    mSourceServer->Create( itemWizardPage5, CD_TR_SOURCE_SERVER, _T(""), wxDefaultPosition, wxDefaultSize, 0, mSourceServerStrings, wxCB_READONLY|wxCB_SORT );
    itemBoxSizer6->Add(mSourceServer, 0, wxGROW|wxALL, 5);

    mSourceSchemaC = new wxStaticText;
    mSourceSchemaC->Create( itemWizardPage5, CD_TR_SOURCE_SCHEMA_C, _("Schema name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mSourceSchemaC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mSourceSchemaStrings = NULL;
    mSourceSchema = new wxOwnerDrawnComboBox;
    mSourceSchema->Create( itemWizardPage5, CD_TR_SOURCE_SCHEMA, _T(""), wxDefaultPosition, wxDefaultSize, 0, mSourceSchemaStrings, wxCB_READONLY|wxCB_SORT );
    itemBoxSizer6->Add(mSourceSchema, 0, wxGROW|wxALL, 5);

    mSrcObjectC = new wxStaticText;
    mSrcObjectC->Create( itemWizardPage5, CD_TR_SOURCE_OBJECT_C, _("Table name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mSrcObjectC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mSrcObjectStrings = NULL;
    mSrcObject = new wxOwnerDrawnComboBox;
    mSrcObject->Create( itemWizardPage5, CD_TR_SOURCE_OBJECT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mSrcObjectStrings, wxCB_READONLY|wxCB_SORT );
    itemBoxSizer6->Add(mSrcObject, 0, wxGROW|wxALL, 5);

    mConditionC = new wxStaticText;
    mConditionC->Create( itemWizardPage5, CD_TR_CONDITION_C, _("Take only rows satisfying the condition:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mConditionC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mCondition = new wxTextCtrl;
    mCondition->Create( itemWizardPage5, CD_TR_CONDITION, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mCondition, 0, wxGROW|wxALL, 5);

    itemBoxSizer6->Add(0, 0, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

////@end WizardPage content construction
    t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
    if (!dsgn->odbc_source() && !any_odbc_ds_connected())
      mSourceOrigin->Enable(1, false);
    mSourceType->Append(_("Native 602SQL table data"),       (void*)IETYPE_TDT);
    mSourceType->Append(_("Comma separated values"), (void*)IETYPE_CSV);
    mSourceType->Append(_("Values in columns"), (void*)IETYPE_COLS);
    mSourceType->Append(_("dBase DBF"),         (void*)IETYPE_DBASE);
    mSourceType->Append(_("FoxPro DBF"),        (void*)IETYPE_FOX);
    mSourceFname->SetMaxLength(MAX_PATH-1);
    mCondition->SetMaxLength(MAX_COND_SIZE);
}
// source_type, inserver, inschema: changes stored immediately

wxString encoding_name(int recode)
{ switch (recode)
  { case CHARSET_CP1250:   return _("Windows Middle Europe (CP1250)");
    case CHARSET_CP1252:   return _("Windows Western (CP1252)");
    case CHARSET_ISO646:   return _("ASCII (ISO646)");
    case CHARSET_ISO88592: return _("ISO 8859-2 (Middle Europe)");
    case CHARSET_IBM852:   return _("DOS Latin-2 (ibm852)");
    case CHARSET_ISO88591: return _("ISO 8859-1 (Western Europpe)");
    case CHARSET_UTF8:     return _("UTF-8"); 
    case CHARSET_UCS2:     return _("UCS-2");
    case CHARSET_UCS4:     return _("UCS-4 ");
  }
  return wxEmptyString;
}

void fill_encodings(wxOwnerDrawnComboBox * combo, wxStaticText * capt, int data_type)
{ 
  combo->Clear();
  if (data_type==IETYPE_WB || data_type==IETYPE_CURSOR || data_type==IETYPE_ODBC || data_type==IETYPE_ODBCVIEW || data_type==IETYPE_TDT)
  { combo->SetValue(wxEmptyString);  // necessary on Linux, otherwise the old value remains in the edit field
    combo->Disable();  
    capt->Disable(); 
  }
  else
  { combo->Enable();  
    capt->Enable(); 
    combo->Append(encoding_name(CHARSET_CP1250),   (void*)CHARSET_CP1250);
    combo->Append(encoding_name(CHARSET_CP1252),   (void*)CHARSET_CP1252);
    combo->Append(encoding_name(CHARSET_ISO646),   (void*)CHARSET_ISO646);
    combo->Append(encoding_name(CHARSET_ISO88592), (void*)CHARSET_ISO88592);
    combo->Append(encoding_name(CHARSET_IBM852),   (void*)CHARSET_IBM852);
    combo->Append(encoding_name(CHARSET_ISO88591), (void*)CHARSET_ISO88591);
    combo->Append(encoding_name(CHARSET_UTF8),     (void*)CHARSET_UTF8); 
    if (data_type==IETYPE_CSV || data_type==IETYPE_COLS)
    { combo->Append(encoding_name(CHARSET_UCS2), (void*)CHARSET_UCS2);
      combo->Append(encoding_name(CHARSET_UCS4), (void*)CHARSET_UCS4);
    }
  }
}

wxString get_filter(int sel)
{ wxString filter;
  switch (sel)
  { case 0: filter = _("602SQL Table data files (*.tdt)|*.tdt|");  break;
    case 1: filter = _("CSV files (*.csv)|*.csv|");  break;
    case 2: filter = _("Files with data in columns (*.txt)|*.txt|");  break;
    case 3: filter = _("dBase files (*.dbf)|*.dbf|");  break;
    case 4: filter = _("FoxPro files (*.dbf)|*.dbf|");  break;
  }
#ifdef WINS
  return filter+_("All files|*.*");
#else
  return filter+_("All files|*");
#endif
}

void update_file_name_suffix(wxTextCtrl * ctrl, int src_targ_type)
{ if (src_targ_type==IETYPE_WB || src_targ_type==IETYPE_CURSOR || src_targ_type==IETYPE_ODBC || src_targ_type==IETYPE_ODBCVIEW) 
    return; // no file
  wxString fname;
  fname = ctrl->GetValue();
  if (fname.Length()>4 && fname.GetChar(fname.Length()-4)=='.')
  { fname = fname.Mid(0, fname.Length()-3);  // strip the suffix
    fname = fname + (src_targ_type==IETYPE_CSV ? wxT("csv") : src_targ_type==IETYPE_COLS ? wxT("txt") : 
                     src_targ_type==IETYPE_TDT ? wxT("tdt") : wxT("dbf"));
    ctrl->SetValue(fname);
  }
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE
 */

void WizardPage::OnWizardpagePageChanged( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // propose a file source if database target is specified:
  if (!*dsgn->inpath && *dsgn->outpath && dsgn->text_source() && !dsgn->text_target())
  { if (!read_profile_string("Directories", "Import Path", dsgn->inpath, sizeof(dsgn->inpath)))
      strcpy(dsgn->inpath, wxGetCwd().mb_str(*wxConvCurrent));
    append(dsgn->inpath, "");
    const char * suff = dsgn->source_type==IETYPE_CSV ? ".csv" : dsgn->source_type==IETYPE_COLS ? ".txt" : ".dbf";
    char name[64];
    strcat(dsgn->inpath, object_fname(dsgn->cdp, name, dsgn->outpath, suff));
  }
 // show the source origin:
  if (dsgn->source_type==IETYPE_WB || dsgn->source_type==IETYPE_CURSOR)
  { mSourceView->SetValue(dsgn->source_type==IETYPE_CURSOR);
    mSourceOrigin->SetSelection(0);
  }
  else if (dsgn->source_type==IETYPE_ODBC || dsgn->source_type==IETYPE_ODBCVIEW)
  { mSourceView->SetValue(dsgn->source_type==IETYPE_ODBCVIEW);
    mSourceOrigin->SetSelection(1);
  }
  else
    mSourceOrigin->SetSelection(2);
 // show the condition:
  mCondition->SetValue(wxString(dsgn->cond, *dsgn->cdp->sys_conv));
 // explicit feedback:
  wxCommandEvent event2;
  OnCdTrSourceOriginSelected(event2);
  event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE
 */

void WizardPage::OnWizardpagePageChanging( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  bool propose = false;
  if (dsgn->source_type==IETYPE_WB || dsgn->source_type==IETYPE_CURSOR || dsgn->source_type==IETYPE_ODBC || dsgn->source_type==IETYPE_ODBCVIEW)
  { strmaxcpy(dsgn->inpath, mSrcObject->GetValue().mb_str(*dsgn->cdp->sys_conv), sizeof(dsgn->inpath)); // object name
    propose = *dsgn->inpath && !*dsgn->outpath && dsgn->dest_type!=IETYPE_WB && dsgn->dest_type!=IETYPE_ODBC;
  }
  else
  { strmaxcpy(dsgn->inpath, mSourceFname->GetValue().mb_str(*wxConvCurrent), sizeof(dsgn->inpath));   // file name
    dsgn->src_recode = mSourceEnc->GetSelection();
  }
  if (!event.GetDirection()) return;  // going back, no more tests
  if (!*dsgn->inpath)
  { if (dsgn->source_type==IETYPE_WB || dsgn->source_type==IETYPE_CURSOR || dsgn->source_type==IETYPE_ODBC || dsgn->source_type==IETYPE_ODBCVIEW) 
         mSrcObject->SetFocus();
    else mSourceFname->SetFocus();
    wxBell();  event.Veto();  return;
  }
 // save the condition (last value is stored event if the current data source type does not support conditions):
  strmaxcpy(dsgn->cond, mCondition->GetValue().mb_str(*dsgn->cdp->sys_conv), sizeof(dsgn->cond));
 // propose the default file name for export from the database:
  if (propose)  // source is a database object, target is a file
  { if (!read_profile_string("Directories", "Export Path", dsgn->outpath, sizeof(dsgn->outpath)))
      strcpy(dsgn->outpath, wxGetCwd().mb_str(*wxConvCurrent));
    append(dsgn->outpath, "");
    const char * suff = dsgn->dest_type==IETYPE_CSV ? ".csv" : dsgn->dest_type==IETYPE_COLS ? ".txt" : ".dbf";
    char aName[64];
    strcat(dsgn->outpath, object_fname(dsgn->cdp, aName, dsgn->inpath, suff));
  }
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TR_SOURCE_ORIGIN
 */

void WizardPage::OnCdTrSourceOriginSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // saving the change immediately:
  if (mSourceOrigin->GetSelection()==0)
    dsgn->source_type = mSourceView->GetValue() ? IETYPE_CURSOR : IETYPE_WB;
  else if (mSourceOrigin->GetSelection()==1)
    dsgn->source_type = mSourceView->GetValue() ? IETYPE_ODBCVIEW : IETYPE_ODBC;
  else
    if (dsgn->source_type==IETYPE_WB || dsgn->source_type==IETYPE_CURSOR || dsgn->source_type==IETYPE_ODBC || dsgn->source_type==IETYPE_ODBCVIEW)
      dsgn->source_type = IETYPE_CSV;

 // showing and hiding:
  bool file = !(dsgn->source_type==IETYPE_WB || dsgn->source_type==IETYPE_CURSOR || dsgn->source_type==IETYPE_ODBC || dsgn->source_type==IETYPE_ODBCVIEW);
  mSourceType->Show(file);     mSourceTypeC->Show(file);
  mSourceEnc->Show(file);      mSrcEncC->Show(file);
  mSourceFname->Show(file);    mSourceFnameC->Show(file);
  mSourceServer->Show(!file);  mSourceServerC->Show(!file);
  mSourceSchema->Show(!file);  mSourceSchemaC->Show(!file);
  mSrcObject->Show(!file);     mSrcObjectC->Show(!file);
  mCondition->Show(!file);     mConditionC->Show(!file);
  mSourceView->Show(!file);
  mSourceNameSel->Show(file);
  Layout();

  if (mSourceOrigin->GetSelection()==0 || mSourceOrigin->GetSelection()==1) 
  {// fill connected 602SQL servers: 
    mSourceServer->Clear();
    for (t_avail_server * server = available_servers;  server;  server = server->next)
      if (dsgn->source_type == IETYPE_ODBC || dsgn->source_type == IETYPE_ODBCVIEW ? 
            server->odbc && server->conn : 
            !server->odbc && server->cdp && 
              (!server->notreg || !stricmp(server->locid_server_name, dsgn->cdp->locid_server_name)))
        mSourceServer->Append(wxString(server->locid_server_name, *wxConvCurrent));
   // show server:
    int ind = mSourceServer->FindString(wxString(*dsgn->inserver ? dsgn->inserver : dsgn->cdp->locid_server_name, *wxConvCurrent));
    if (ind!=wxNOT_FOUND) // setStringSelection displays Assert when not found
      mSourceServer->SetSelection(ind);
    else if (!*dsgn->inserver && mSourceServer->GetCount()==1)
      mSourceServer->SetSelection(0);
   // feedback:
    wxCommandEvent event2;
    OnCdTrSourceServerSelected(event2);
  }
  else
  {// show the file name:
    mSourceFname->SetValue(wxString(dsgn->inpath, *wxConvCurrent));
   // show file format:
    mSourceType->SetSelection(dsgn->source_type==IETYPE_TDT ? 0 : dsgn->source_type==IETYPE_CSV ? 1 : dsgn->source_type==IETYPE_COLS ? 2 : dsgn->source_type==IETYPE_DBASE ? 3 : 4);
   // feedback: may update the file name, shows the encoding
    wxCommandEvent event2;
    OnCdTrSourceTypeSelected(event2);
  }
  event.Skip();
}

void WizardPage::OnCdTrSourceViewClick( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // saving the change immediately:
  dsgn->source_type = mSourceView->GetValue() ? 
    (dsgn->source_type==IETYPE_WB ? IETYPE_CURSOR : IETYPE_ODBCVIEW) :
    (dsgn->source_type==IETYPE_CURSOR ? IETYPE_WB : IETYPE_ODBC);
 // feedback: shiws the same schema, but a new object list
  wxCommandEvent event2;
  OnCdTrSourceServerSelected(event2);
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_SOURCE_SERVER
 */

void WizardPage::OnCdTrSourceServerSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // saving the change immediately:
  int sel = GetNewComboSelection(mSourceServer);
  if (sel>=0) strmaxcpy(dsgn->inserver, mSourceServer->GetString(sel).mb_str(*wxConvCurrent), sizeof(dsgn->inserver)); 
  else *dsgn->inserver=0;
 // fill schema names:
  mSourceSchema->Clear();
  if (*dsgn->inserver)
  { const t_avail_server * as = find_server_connection_info(dsgn->inserver);
    if (as)
    { CObjectList ol(as, NULL);
      //ol.Init(CATEG_APPL);
      for (bool ItemFound = ol.FindFirst(CATEG_APPL); ItemFound; ItemFound = ol.FindNext())
        mSourceSchema->Append(GetSmallStringName(ol));
    }
   // show the schema name:
    wxString orig(*dsgn->inschema || !dsgn->cdp ? dsgn->inschema : dsgn->cdp->sel_appl_name, GetConv(as->cdp));
	CaptFirst(orig);
    int ind = mSourceSchema->FindString(orig);
    if (ind!=wxNOT_FOUND) // setStringSelection displays Assert when not found
      mSourceSchema->SetSelection(ind);
    else 
	{ if (!*dsgn->inschema && mSourceSchema->GetCount()==1)
        mSourceSchema->SetSelection(0);
	}	
   // feedback:
    wxCommandEvent event2;
    OnCdTrSourceSchemaSelected(event2);
  }
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_SOURCE_SCHEMA
 */

void WizardPage::OnCdTrSourceSchemaSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // find the selected server, I need the cdp:
  const t_avail_server * as = find_server_connection_info(dsgn->inserver);
  if (!as) return;  // fuse
 // saving the change immediately:
  int sel = GetNewComboSelection(mSourceSchema);
  if (sel>=0) strmaxcpy(dsgn->inschema, mSourceSchema->GetString(sel).mb_str(GetConv(as->cdp)), sizeof(dsgn->inschema)); 
  else *dsgn->inschema=0;
 // fill schema names:
  mSrcObject->Clear();
  if (*dsgn->inschema)
  { 
    { CObjectList ol(as, dsgn->inschema);
      //ol.Init(CATEG_APPL);
      for (bool ItemFound = ol.FindFirst(dsgn->source_type==IETYPE_WB || dsgn->source_type==IETYPE_ODBC ? CATEG_TABLE : CATEG_CURSOR); ItemFound; ItemFound = ol.FindNext())
        mSrcObject->Append(GetSmallStringName(ol));

    }
   // show the schema name:
    wxString orig(wxString(dsgn->inpath, GetConv(as->cdp)));
    CaptFirst(orig);   
    int ind = mSrcObject->FindString(orig);
    if (ind!=wxNOT_FOUND) // setStringSelection displays Assert when not found
      // on GTK I cannot remove the previous selection when the list is empty, even by SetSelection(wxNOT_FOUND)
      mSrcObject->SetSelection(ind);
  }
}

#if 0
    tobjname upcase_name;
    strmaxcpy(upcase_name, dsgn->inpath, sizeof(upcase_name));
    Upcase9(dsgn->cdp, upcase_name);
    int ind = mSourceName->FindString(wxString(upcase_name, *dsgn->cdp->sys_conv));
    if (ind!=wxNOT_FOUND) // setStringSelection displays Assert when not found
      mSourceName->SetSelection(ind);
#endif

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_SOURCE_TYPE
 */

void WizardPage::OnCdTrSourceTypeSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // saving the change immediately:
  int sel = GetNewComboSelection(mSourceType);
  dsgn->source_type = sel==0 ? IETYPE_TDT : sel==1 ? IETYPE_CSV : sel==2 ? IETYPE_COLS : sel==3 ? IETYPE_DBASE : IETYPE_FOX;
 // update lists:
  fill_encodings(mSourceEnc, mSrcEncC, dsgn->source_type);
 // show the encoding:
  if (dsgn->src_recode>=mSourceEnc->GetCount()) dsgn->src_recode=0;
  if (mSourceEnc->GetCount()) mSourceEnc->SetSelection(dsgn->src_recode);
 // update the file name suffix:
  update_file_name_suffix(mSourceFname, dsgn->source_type);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_SOURCE_NAME_SEL
 */

void WizardPage::OnCdTrSourceNameSelClick( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  wxString filter = get_filter(mSourceType->GetSelection());  // must not be only the parameter of the next function!
  wxString str = GetImportFileName(mSourceFname->GetValue(), filter, GetParent());
  if (!str.IsEmpty())
    mSourceFname->SetValue(str);
}

/*!
 * Should we show tooltips?
 */

bool WizardPage::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap WizardPage::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WizardPage bitmap retrieval
    return wxNullBitmap;
////@end WizardPage bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WizardPage::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WizardPage icon retrieval
    return wxNullIcon;
////@end WizardPage icon retrieval
}

/*!
 * WizardPage1 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPage1, wxWizardPage )

/*!
 * WizardPage1 event table definition
 */

BEGIN_EVENT_TABLE( WizardPage1, wxWizardPage )

////@begin WizardPage1 event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, WizardPage1::OnWizardpage1PageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, WizardPage1::OnWizardpage1PageChanging )

    EVT_RADIOBOX( CD_TR_DEST_ORIGIN, WizardPage1::OnCdTrDestOriginSelected )

    EVT_COMBOBOX( CD_TR_DEST_TYPE, WizardPage1::OnCdTrDestTypeSelected )

    EVT_BUTTON( CD_TR_DEST_NAME_SEL, WizardPage1::OnCdTrDestNameSelClick )

    EVT_COMBOBOX( CD_TR_DEST_SERVER, WizardPage1::OnCdTrDestServerSelected )

    EVT_COMBOBOX( CD_TR_DEST_SCHEMA, WizardPage1::OnCdTrDestSchemaSelected )

////@end WizardPage1 event table entries

END_EVENT_TABLE()

/*!
 * WizardPage1 constructors
 */

WizardPage1::WizardPage1( )
{
}

WizardPage1::WizardPage1( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPage1 creator
 */

bool WizardPage1::Create( wxWizard* parent )
{
////@begin WizardPage1 member initialisation
    mDestOrigin = NULL;
    mDestTypeC = NULL;
    mDestType = NULL;
    mDestEncC = NULL;
    mDestEnc = NULL;
    mDestFnameC = NULL;
    mDestFname = NULL;
    mDestNameSel = NULL;
    mDestServerC = NULL;
    mDestServer = NULL;
    mDestSchemaC = NULL;
    mDestSchema = NULL;
    mDestObjectC = NULL;
    mDestObject = NULL;
    mIndexes = NULL;
////@end WizardPage1 member initialisation

////@begin WizardPage1 creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPage::Create( parent, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end WizardPage1 creation
    return TRUE;
}

/*!
 * Control creation for WizardPage1
 */

void WizardPage1::CreateControls()
{    
////@begin WizardPage1 content construction
    WizardPage1* itemWizardPage26 = this;

    wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage26->SetSizer(itemBoxSizer27);

    wxString mDestOriginStrings[] = {
        _("602SQL &Database"),
        _("&ODBC Data Source"),
        _("&External file")
    };
    mDestOrigin = new wxRadioBox;
    mDestOrigin->Create( itemWizardPage26, CD_TR_DEST_ORIGIN, _("Copy data to"), wxDefaultPosition, wxDefaultSize, 3, mDestOriginStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer27->Add(mDestOrigin, 0, wxGROW|wxALL, 5);

    mDestTypeC = new wxStaticText;
    mDestTypeC->Create( itemWizardPage26, CD_TR_DEST_TYPE_C, _("Format of the target file:"), wxDefaultPosition, wxDefaultSize, 0 );
    mDestTypeC->Show(FALSE);
    itemBoxSizer27->Add(mDestTypeC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mDestTypeStrings = NULL;
    mDestType = new wxOwnerDrawnComboBox;
    mDestType->Create( itemWizardPage26, CD_TR_DEST_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDestTypeStrings, wxCB_READONLY );
    mDestType->Show(FALSE);
    itemBoxSizer27->Add(mDestType, 0, wxGROW|wxALL, 5);

    mDestEncC = new wxStaticText;
    mDestEncC->Create( itemWizardPage26, CD_TR_DEST_ENC_C, _("Encoding of the target file:"), wxDefaultPosition, wxDefaultSize, 0 );
    mDestEncC->Show(FALSE);
    itemBoxSizer27->Add(mDestEncC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mDestEncStrings = NULL;
    mDestEnc = new wxOwnerDrawnComboBox;
    mDestEnc->Create( itemWizardPage26, CD_TR_DEST_ENC, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDestEncStrings, wxCB_READONLY );
    mDestEnc->Show(FALSE);
    itemBoxSizer27->Add(mDestEnc, 0, wxGROW|wxALL, 5);

    mDestFnameC = new wxStaticText;
    mDestFnameC->Create( itemWizardPage26, CD_TR_DEST_FNAME_C, _("Target file name:"), wxDefaultPosition, wxDefaultSize, 0 );
    mDestFnameC->Show(FALSE);
    itemBoxSizer27->Add(mDestFnameC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer34 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer27->Add(itemBoxSizer34, 0, wxGROW|wxALL, 0);

    mDestFname = new wxTextCtrl;
    mDestFname->Create( itemWizardPage26, CD_TR_DEST_FNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mDestFname->Show(FALSE);
    itemBoxSizer34->Add(mDestFname, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mDestNameSel = new wxButton;
    mDestNameSel->Create( itemWizardPage26, CD_TR_DEST_NAME_SEL, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    mDestNameSel->Show(FALSE);
    itemBoxSizer34->Add(mDestNameSel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    mDestServerC = new wxStaticText;
    mDestServerC->Create( itemWizardPage26, CD_TR_DEST_SERVER_C, _("Server name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer27->Add(mDestServerC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mDestServerStrings = NULL;
    mDestServer = new wxOwnerDrawnComboBox;
    mDestServer->Create( itemWizardPage26, CD_TR_DEST_SERVER, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDestServerStrings, wxCB_READONLY|wxCB_SORT );
    itemBoxSizer27->Add(mDestServer, 0, wxGROW|wxALL, 5);

    mDestSchemaC = new wxStaticText;
    mDestSchemaC->Create( itemWizardPage26, CD_TR_DEST_SCHEMA_C, _("Schema name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer27->Add(mDestSchemaC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mDestSchemaStrings = NULL;
    mDestSchema = new wxOwnerDrawnComboBox;
    mDestSchema->Create( itemWizardPage26, CD_TR_DEST_SCHEMA, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDestSchemaStrings, wxCB_READONLY|wxCB_SORT );
    itemBoxSizer27->Add(mDestSchema, 0, wxGROW|wxALL, 5);

    mDestObjectC = new wxStaticText;
    mDestObjectC->Create( itemWizardPage26, CD_TR_DEST_OBJECT_C, _("Table name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer27->Add(mDestObjectC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mDestObjectStrings = NULL;
    mDestObject = new wxOwnerDrawnComboBox;
    mDestObject->Create( itemWizardPage26, CD_TR_DEST_OBJECT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDestObjectStrings, 0 );
    itemBoxSizer27->Add(mDestObject, 0, wxGROW|wxALL, 5);

    wxString mIndexesStrings[] = {
        _("&Incrementally update during import"),
        _("&Rebuild from scratch after import")
    };
    mIndexes = new wxRadioBox;
    mIndexes->Create( itemWizardPage26, CD_TR_INDEXES, _("Updating index on the target table"), wxDefaultPosition, wxDefaultSize, 2, mIndexesStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer27->Add(mIndexes, 0, wxGROW|wxALL, 5);

    itemBoxSizer27->Add(0, 0, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

////@end WizardPage1 content construction
    t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
    if (!dsgn->odbc_target() && !any_odbc_ds_connected())
      mDestOrigin->Enable(1, false);
    mDestType->Append(_("Native 602SQL table data"),       (void*)IETYPE_TDT);
    mDestType->Append(_("Comma separated values"), (void*)IETYPE_CSV);
    mDestType->Append(_("Values in columns"), (void*)IETYPE_COLS);
    mDestType->Append(_("dBase DBF"),         (void*)IETYPE_DBASE);
    mDestType->Append(_("FoxPro DBF"),        (void*)IETYPE_FOX);
    mDestFname->SetMaxLength(MAX_PATH-1);
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE1
 */

void WizardPage1::OnWizardpage1PageChanged( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // show the target origin:
  if (dsgn->dest_type==IETYPE_WB)
    mDestOrigin->SetSelection(0);
  else if (dsgn->dest_type==IETYPE_ODBC)
    mDestOrigin->SetSelection(1);
  else
    mDestOrigin->SetSelection(2);
 // show index mode:
  mIndexes->SetSelection(dsgn->index_past ? 1 : 0);
 // explicit feedback:
  wxCommandEvent event2;
  OnCdTrDestOriginSelected(event2);
  event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE1
 */

void WizardPage1::OnWizardpage1PageChanging( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  if (dsgn->dest_type==IETYPE_WB || dsgn->dest_type==IETYPE_ODBC)
  { strmaxcpy(dsgn->outpath, mDestObject->GetValue().mb_str(*dsgn->cdp->sys_conv), sizeof(dsgn->outpath)); // object name
    if (dsgn->dest_type==IETYPE_ODBC)
      if (mDestObject->FindString(mDestObject->GetValue()) == wxNOT_FOUND) // value is not from the list
        { error_box(_("Data can only be transported to an EXISTING ODBC table"), GetParent());  event.Veto();  return; }
  }
  else
  { strmaxcpy(dsgn->outpath, mDestFname->GetValue().mb_str(*wxConvCurrent), sizeof(dsgn->outpath));   // file name
    dsgn->dest_recode = mDestEnc->GetSelection();
  }
  if (!*dsgn->outpath)
  { if (dsgn->dest_type==IETYPE_WB || dsgn->dest_type==IETYPE_ODBC) mDestObject->SetFocus();
    else mDestFname->SetFocus();
    wxBell();  event.Veto();  return;
  }
 // save index mode (even if not using):
  dsgn->index_past = mIndexes->GetSelection()==1;
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TR_DEST_ORIGIN
 */

void WizardPage1::OnCdTrDestOriginSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // clear the object/file name when changing the class:
 // saving the change immediately:
  if (mDestOrigin->GetSelection()==0)
  { if (dsgn->text_target()) *dsgn->outpath=0;
    dsgn->dest_type = IETYPE_WB;
  }
  else if (mDestOrigin->GetSelection()==1)
  { if (dsgn->text_target()) *dsgn->outpath=0;
    dsgn->dest_type = IETYPE_ODBC;
  }
  else
    if (!dsgn->text_target() && !dsgn->dbf_target())
    { dsgn->dest_type = IETYPE_CSV;
      *dsgn->outpath=0;
    }
 // showing and hiding:
  bool file = !(dsgn->dest_type==IETYPE_WB || dsgn->dest_type==IETYPE_ODBC);
  mDestType->Show(file);     mDestTypeC->Show(file);
  mDestEnc->Show(file);      mDestEncC->Show(file);
  mDestFname->Show(file);    mDestFnameC->Show(file);
  mDestServer->Show(!file);  mDestServerC->Show(!file);
  mDestSchema->Show(!file);  mDestSchemaC->Show(!file);
  mDestObject->Show(!file);  mDestObjectC->Show(!file);
  mIndexes->Show(dsgn->dest_type==IETYPE_WB);     
  mDestNameSel->Show(file);
  Layout();

  if (mDestOrigin->GetSelection()==0 || mDestOrigin->GetSelection()==1) 
  {// fill connected 602SQL servers: 
    mDestServer->Clear();
    for (t_avail_server * server = available_servers;  server;  server = server->next)
      if ((dsgn->dest_type == IETYPE_ODBC) ? 
            server->odbc && server->conn : 
            !server->odbc && server->cdp && 
              (!server->notreg || !stricmp(server->locid_server_name, dsgn->cdp->locid_server_name)))
        mDestServer->Append(wxString(server->locid_server_name, *wxConvCurrent));
   // show server:
    int ind = mDestServer->FindString(wxString(*dsgn->outserver ? dsgn->outserver : dsgn->cdp->locid_server_name, *wxConvCurrent));
    if (ind!=wxNOT_FOUND) // setStringSelection displays Assert when not found
      mDestServer->SetSelection(ind);
    else if (!*dsgn->outserver && mDestServer->GetCount()==1)
      mDestServer->SetSelection(0);
   // feedback:
    wxCommandEvent event2;
    OnCdTrDestServerSelected(event2);
  }
  else
  {// show the file name:
    mDestFname->SetValue(wxString(dsgn->outpath, *wxConvCurrent));
   // show file format:
    mDestType->SetSelection(dsgn->dest_type==IETYPE_TDT ? 0 : dsgn->dest_type==IETYPE_CSV ? 1 : dsgn->dest_type==IETYPE_COLS ? 2 : dsgn->dest_type==IETYPE_DBASE ? 3 : 4);
   // feedback: may update the file name, shows the encoding
    wxCommandEvent event2;
    OnCdTrDestTypeSelected(event2);
  }
  event.Skip();
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_DEST_SERVER
 */

void WizardPage1::OnCdTrDestServerSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // saving the change immediately:
  int sel = GetNewComboSelection(mDestServer);
  if (sel>=0) strmaxcpy(dsgn->outserver, mDestServer->GetString(sel).mb_str(*wxConvCurrent), sizeof(dsgn->outserver)); 
  else *dsgn->outserver=0;
 // fill schema names:
  mDestSchema->Clear();
  if (*dsgn->outserver)
  { const t_avail_server * as = find_server_connection_info(dsgn->outserver);
    if (as)
    { CObjectList ol(as, NULL);
      //ol.Init(CATEG_APPL);
      for (bool ItemFound = ol.FindFirst(CATEG_APPL); ItemFound; ItemFound = ol.FindNext())
        mDestSchema->Append(GetSmallStringName(ol));

    }
   // show the schema name:
    wxString orig(*dsgn->outschema || !dsgn->cdp ? dsgn->outschema : dsgn->cdp->sel_appl_name, GetConv(as->cdp));
    CaptFirst(orig);   
    int ind = mDestSchema->FindString(orig);
    if (ind!=wxNOT_FOUND) // setStringSelection displays Assert when not found
      mDestSchema->SetSelection(ind);
    else if (!*dsgn->outschema && mDestSchema->GetCount()==1)
      mDestSchema->SetSelection(0);
   // feedback:
    wxCommandEvent event2;
    OnCdTrDestSchemaSelected(event2);
  }
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_DEST_SCHEMA
 */

void WizardPage1::OnCdTrDestSchemaSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // find the selected server, I need the cdp:
  const t_avail_server * as = find_server_connection_info(dsgn->outserver);
  if (!as) return;  // fuse
 // saving the change immediately:
  int sel = GetNewComboSelection(mDestSchema);
  if (sel>=0) strmaxcpy(dsgn->outschema, mDestSchema->GetString(sel).mb_str(GetConv(as->cdp)), sizeof(dsgn->outschema)); 
  else *dsgn->outschema=0;
 // fill schema names:
  mDestObject->Clear();
  if (*dsgn->outschema)
  { 
    { CObjectList ol(as, dsgn->outschema);
      //ol.Init(CATEG_APPL);
      for (bool ItemFound = ol.FindFirst(CATEG_TABLE); ItemFound; ItemFound = ol.FindNext())
        mDestObject->Append(GetSmallStringName(ol));

    }
  }
 // show the object name (may not be contained in the list):
  wxString orig(dsgn->outpath, GetConv(as->cdp));
  CaptFirst(orig);   
  int ind = mDestObject->FindString(orig);
  if (ind!=wxNOT_FOUND) // setStringSelection displays Assert when not found
    mDestObject->SetSelection(ind);
  mDestObject->SetValue(wxString(dsgn->outpath, GetConv(as->cdp)));
}


/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_DEST_TYPE
 */

void WizardPage1::OnCdTrDestTypeSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // saving the change immediately:
  int sel = GetNewComboSelection(mDestType);
  dsgn->dest_type = sel==0 ? IETYPE_TDT : sel==1 ? IETYPE_CSV : sel==2 ? IETYPE_COLS : sel==3 ? IETYPE_DBASE : IETYPE_FOX;
 // update lists:
  fill_encodings(mDestEnc, mDestEncC, dsgn->dest_type);
 // show the encoding:
  if (dsgn->dest_recode>=mDestEnc->GetCount()) dsgn->dest_recode=0;
  if (mDestEnc->GetCount()) mDestEnc->SetSelection(dsgn->dest_recode);
 // update the file name suffix:
  update_file_name_suffix(mDestFname, dsgn->dest_type);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_DEST_NAME_SEL
 */

void WizardPage1::OnCdTrDestNameSelClick( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  wxString filter = get_filter(mDestType->GetSelection());  // must not be only the parameter of the next function!
  wxString str = GetExportFileName(mDestFname->GetValue(), filter, GetParent());
  if (!str.IsEmpty())
    mDestFname->SetValue(str);
}


/*!
 * Should we show tooltips?
 */

bool WizardPage1::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap WizardPage1::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WizardPage1 bitmap retrieval
    return wxNullBitmap;
////@end WizardPage1 bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WizardPage1::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WizardPage1 icon retrieval
    return wxNullIcon;
////@end WizardPage1 icon retrieval
}

/*!
 * WizardPage3 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPage3, wxWizardPage )

/*!
 * WizardPage3 event table definition
 */

BEGIN_EVENT_TABLE( WizardPage3, wxWizardPage )

////@begin WizardPage3 event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, WizardPage3::OnWizardpage3PageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, WizardPage3::OnWizardpage3PageChanging )

    EVT_RADIOBUTTON( CD_XMLSTART_TABLE, WizardPage3::OnCdXmlstartTableSelected )

    EVT_RADIOBUTTON( CD_XMLSTART_EMPTY, WizardPage3::OnCdXmlstartEmptySelected )

    EVT_RADIOBUTTON( CD_XMLSTART_XSD, WizardPage3::OnCdXmlstartXsdSelected )

    EVT_BUTTON( CD_XMLSTART_SELSCHEMA, WizardPage3::OnCdXmlstartSelschemaClick )

    EVT_RADIOBUTTON( CD_XMLSTART_FO, WizardPage3::OnCdXmlstartFoSelected )

    EVT_BUTTON( CD_XMLSTART_SELFO, WizardPage3::OnCdXmlstartSelfoClick )

////@end WizardPage3 event table entries

END_EVENT_TABLE()

/*!
 * WizardPage3 constructors
 */

WizardPage3::WizardPage3( )
{ xml_init=XST_TABLE; 
}

WizardPage3::WizardPage3( wxWizard* parent )
{ xml_init=XST_TABLE; 
  Create( parent );
}

/*!
 * WizardPage3 creator
 */

bool WizardPage3::Create( wxWizard* parent )
{
////@begin WizardPage3 member initialisation
    mByTable = NULL;
    mByEmpty = NULL;
    mBySchema = NULL;
    mSchema = NULL;
    mSelSchema = NULL;
    mByFO = NULL;
    mFO = NULL;
    mSelFO = NULL;
////@end WizardPage3 member initialisation

////@begin WizardPage3 creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPage::Create( parent, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end WizardPage3 creation
    return TRUE;
}

/*!
 * Control creation for WizardPage3
 */

void WizardPage3::CreateControls()
{    
////@begin WizardPage3 content construction
    WizardPage3* itemWizardPage45 = this;

    wxBoxSizer* itemBoxSizer46 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage45->SetSizer(itemBoxSizer46);

    mByTable = new wxRadioButton;
    mByTable->Create( itemWizardPage45, CD_XMLSTART_TABLE, _("Start by mapping columns of a table or query"), wxDefaultPosition, wxDefaultSize, 0 );
    mByTable->SetValue(FALSE);
    itemBoxSizer46->Add(mByTable, 0, wxALIGN_LEFT|wxALL, 5);

    mByEmpty = new wxRadioButton;
    mByEmpty->Create( itemWizardPage45, CD_XMLSTART_EMPTY, _("Start from an empty design"), wxDefaultPosition, wxDefaultSize, 0 );
    mByEmpty->SetValue(FALSE);
    itemBoxSizer46->Add(mByEmpty, 0, wxALIGN_LEFT|wxALL, 5);

    mBySchema = new wxRadioButton;
    mBySchema->Create( itemWizardPage45, CD_XMLSTART_XSD, _("Start by creating tables and mapping for a XML schema:"), wxDefaultPosition, wxDefaultSize, 0 );
    mBySchema->SetValue(FALSE);
    itemBoxSizer46->Add(mBySchema, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer50 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer46->Add(itemBoxSizer50, 0, wxGROW|wxALL, 0);

    itemBoxSizer50->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSchema = new wxTextCtrl;
    mSchema->Create( itemWizardPage45, ID_TEXTCTRL2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer50->Add(mSchema, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    mSelSchema = new wxButton;
    mSelSchema->Create( itemWizardPage45, CD_XMLSTART_SELSCHEMA, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemBoxSizer50->Add(mSelSchema, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxADJUST_MINSIZE, 5);

    mByFO = new wxRadioButton;
    mByFO->Create( itemWizardPage45, CD_XMLSTART_FO, _("Start by creating tables and mapping for a FO form:"), wxDefaultPosition, wxDefaultSize, 0 );
    mByFO->SetValue(FALSE);
    itemBoxSizer46->Add(mByFO, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer55 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer46->Add(itemBoxSizer55, 0, wxGROW|wxALL, 0);

    itemBoxSizer55->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mFO = new wxTextCtrl;
    mFO->Create( itemWizardPage45, ID_TEXTCTRL3, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer55->Add(mFO, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    mSelFO = new wxButton;
    mSelFO->Create( itemWizardPage45, CD_XMLSTART_SELFO, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemBoxSizer55->Add(mSelFO, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxADJUST_MINSIZE, 5);

////@end WizardPage3 content construction
    mByTable->SetValue(true);  // xml_init==XST_TABLE initially
    enabling(xml_init);
#if (WBVERS<950) || !defined(WINS) && (WBVERS<1000)
    mBySchema->Disable();
    mByFO->Disable();
#endif
}

void WizardPage3::enabling(t_xml_start_type sel)
{ 
  mSchema->Enable(sel==XST_XSD);  mSelSchema->Enable(sel==XST_XSD);
  mFO->Enable(sel==XST_FO);       mSelFO->Enable(sel==XST_FO);
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE3
 */

void WizardPage3::OnWizardpage3PageChanged( wxWizardEvent& event )
{
    event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE3
 */

void WizardPage3::OnWizardpage3PageChanging( wxWizardEvent& event )
{ 
  t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  if (!event.GetDirection()) return;  // going back, no more tests
  if (mByTable->GetValue())
  { if (xml_init!=XST_QUERY) xml_init=XST_TABLE;
  }
  else if (mBySchema->GetValue())
  { query=mSchema->GetValue();
    query.Trim(false);  query.Trim(true);
    if (query.IsEmpty())
      { event.Veto();  mSchema->SetFocus();  wxBell();  return; }
    xml_init=XST_XSD;
  }
  else if (mByFO->GetValue())
  { query=mFO->GetValue();
    query.Trim(false);  query.Trim(true);
    if (query.IsEmpty())
      { event.Veto();  mFO->SetFocus();  wxBell();  return; }
    xml_init=XST_FO;
  }
  else if (mByEmpty->GetValue())
    xml_init=XST_EMPTY;
  else
  { event.Veto();  
    wxBell();  
  }
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_TABLE
 */

void WizardPage3::OnCdXmlstartTableSelected( wxCommandEvent& event )
{ mByEmpty->SetValue(false);   mBySchema->SetValue(false);  mByFO->SetValue(false);
  if (xml_init!=XST_QUERY) xml_init=XST_TABLE;  // this will be done in ...PageChanging, but NextPage is called before and it needs the xml_init! 
  enabling(xml_init);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_EMPTY
 */

void WizardPage3::OnCdXmlstartEmptySelected( wxCommandEvent& event )
{ mByTable->SetValue(false);  mBySchema->SetValue(false);  mByFO->SetValue(false);
  xml_init=XST_EMPTY;  // this will be done in ...PageChanging, but NextPage is called before and it needs the xml_init! 
  enabling(xml_init);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON
 */

void WizardPage3::OnCdXmlstartXsdSelected( wxCommandEvent& event )
{ mByTable->SetValue(false);   mByEmpty->SetValue(false);  mByFO->SetValue(false);
  xml_init=XST_XSD;  // this will be done in ...PageChanging, but NextPage is called before and it needs the xml_init! 
  enabling(xml_init);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_FO
 */

void WizardPage3::OnCdXmlstartFoSelected( wxCommandEvent& event )
{ mByTable->SetValue(false);  mByEmpty->SetValue(false);  mBySchema->SetValue(false);
  xml_init=XST_FO;  // this will be done in ...PageChanging, but NextPage is called before and it needs the xml_init! 
  enabling(xml_init);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLSTART_SELSCHEMA
 */

void WizardPage3::OnCdXmlstartSelschemaClick( wxCommandEvent& event )
{ wxString filter = _("XML schema files (*.xsd)|*.xsd|");
#ifdef WINS
  filter+=_("All files|*.*");
#else
  filter+=_("All files|*");
#endif
  wxString str = GetImportFileName(mSchema->GetValue(), filter, GetParent());
  if (!str.IsEmpty())
    mSchema->SetValue(str);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLSTART_SELFO
 */

void WizardPage3::OnCdXmlstartSelfoClick( wxCommandEvent& event )
{ wxString filter = _("XML Form files (*.fo)|*.fo|Zipped Form files (*.zfo)|*.zfo|");
#ifdef WINS
  filter+=_("All files|*.*");
#else
  filter+=_("All files|*");
#endif
  wxString str = GetImportFileName(mFO->GetValue(), filter, GetParent());
  if (!str.IsEmpty())
    mFO->SetValue(str);
}


/*!
 * Should we show tooltips?
 */

bool WizardPage3::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap WizardPage3::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WizardPage3 bitmap retrieval
    return wxNullBitmap;
////@end WizardPage3 bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WizardPage3::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WizardPage3 icon retrieval
    return wxNullIcon;
////@end WizardPage3 icon retrieval
}

/*!
 * WizardPage4 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPage4, wxWizardPage )

/*!
 * WizardPage4 event table definition
 */

BEGIN_EVENT_TABLE( WizardPage4, wxWizardPage )

////@begin WizardPage4 event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, WizardPage4::OnWizardpage4PageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, WizardPage4::OnWizardpage4PageChanging )

////@end WizardPage4 event table entries

END_EVENT_TABLE()

/*!
 * WizardPage4 constructors
 */

WizardPage4::WizardPage4( )
{
}

WizardPage4::WizardPage4( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPage4 creator
 */

bool WizardPage4::Create( wxWizard* parent )
{
////@begin WizardPage4 member initialisation
    mCreHeader = NULL;
    mSkipHeaderC = NULL;
    mSkipHeader = NULL;
    mCsvSepar = NULL;
    mCsvDelim = NULL;
    mDateFormat = NULL;
    mTimeFormat = NULL;
    mTsFormat = NULL;
    mBoolFormat = NULL;
    mDecSepar = NULL;
    mSemilog = NULL;
    mCRLines = NULL;
////@end WizardPage4 member initialisation

////@begin WizardPage4 creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPage::Create( parent, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end WizardPage4 creation
    return TRUE;
}

/*!
 * Control creation for WizardPage4
 */

void WizardPage4::CreateControls()
{    
////@begin WizardPage4 content construction
    WizardPage4* itemWizardPage59 = this;

    wxBoxSizer* itemBoxSizer60 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage59->SetSizer(itemBoxSizer60);

    mCreHeader = new wxCheckBox;
    mCreHeader->Create( itemWizardPage59, CD_TR_CRE_HEADER, _("Create header line in the target text file"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mCreHeader->SetValue(FALSE);
    itemBoxSizer60->Add(mCreHeader, 0, wxALIGN_LEFT|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer62 = new wxFlexGridSizer(5, 3, 0, 0);
    itemFlexGridSizer62->AddGrowableCol(1);
    itemBoxSizer60->Add(itemFlexGridSizer62, 0, wxGROW|wxALL, 0);

    mSkipHeaderC = new wxStaticText;
    mSkipHeaderC->Create( itemWizardPage59, CD_TR_SKIP_HEADER_C, _("Number of header lines in source file:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer62->Add(mSkipHeaderC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mSkipHeader = new wxSpinCtrl;
    mSkipHeader->Create( itemWizardPage59, CD_TR_SKIP_HEADER, _("0"), wxDefaultPosition, wxSize(-1, 20), wxSP_ARROW_KEYS, 0, 100, 0 );
    itemFlexGridSizer62->Add(mSkipHeader, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer62->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText66 = new wxStaticText;
    itemStaticText66->Create( itemWizardPage59, wxID_STATIC, _("CSV field separator:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer62->Add(itemStaticText66, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mCsvSeparStrings = NULL;
    mCsvSepar = new wxOwnerDrawnComboBox;
    mCsvSepar->Create( itemWizardPage59, CD_TR_CSV_SEPAR, _T(""), wxDefaultPosition, wxDefaultSize, 0, mCsvSeparStrings, wxCB_DROPDOWN );
    itemFlexGridSizer62->Add(mCsvSepar, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer62->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText69 = new wxStaticText;
    itemStaticText69->Create( itemWizardPage59, wxID_STATIC, _("CSV value quote:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer62->Add(itemStaticText69, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mCsvDelimStrings = NULL;
    mCsvDelim = new wxOwnerDrawnComboBox;
    mCsvDelim->Create( itemWizardPage59, CD_TR_CSV_DELIM, _T(""), wxDefaultPosition, wxDefaultSize, 0, mCsvDelimStrings, wxCB_DROPDOWN );
    itemFlexGridSizer62->Add(mCsvDelim, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer62->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText72 = new wxStaticText;
    itemStaticText72->Create( itemWizardPage59, wxID_STATIC, _("Date format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer62->Add(itemStaticText72, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mDateFormatStrings = NULL;
    mDateFormat = new wxOwnerDrawnComboBox;
    mDateFormat->Create( itemWizardPage59, CD_TR_DATE_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDateFormatStrings, wxCB_DROPDOWN );
    itemFlexGridSizer62->Add(mDateFormat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer62->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText75 = new wxStaticText;
    itemStaticText75->Create( itemWizardPage59, wxID_STATIC, _("Time format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer62->Add(itemStaticText75, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mTimeFormatStrings = NULL;
    mTimeFormat = new wxOwnerDrawnComboBox;
    mTimeFormat->Create( itemWizardPage59, CD_TR_TIME_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTimeFormatStrings, wxCB_DROPDOWN );
    itemFlexGridSizer62->Add(mTimeFormat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer62->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText78 = new wxStaticText;
    itemStaticText78->Create( itemWizardPage59, wxID_STATIC, _("Timestamp format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer62->Add(itemStaticText78, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mTsFormatStrings = NULL;
    mTsFormat = new wxOwnerDrawnComboBox;
    mTsFormat->Create( itemWizardPage59, CD_TR_TS_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTsFormatStrings, wxCB_DROPDOWN );
    itemFlexGridSizer62->Add(mTsFormat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer62->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText81 = new wxStaticText;
    itemStaticText81->Create( itemWizardPage59, wxID_STATIC, _("Boolean encoding:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer62->Add(itemStaticText81, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mBoolFormatStrings = NULL;
    mBoolFormat = new wxOwnerDrawnComboBox;
    mBoolFormat->Create( itemWizardPage59, CD_TR_BOOL_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mBoolFormatStrings, wxCB_DROPDOWN );
    itemFlexGridSizer62->Add(mBoolFormat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer62->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText84 = new wxStaticText;
    itemStaticText84->Create( itemWizardPage59, wxID_STATIC, _("Decimal separator:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer62->Add(itemStaticText84, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mDecSeparStrings = NULL;
    mDecSepar = new wxOwnerDrawnComboBox;
    mDecSepar->Create( itemWizardPage59, CD_TR_DEC_SEPAR, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDecSeparStrings, wxCB_DROPDOWN );
    itemFlexGridSizer62->Add(mDecSepar, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer62->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSemilog = new wxCheckBox;
    mSemilog->Create( itemWizardPage59, CD_TR_SEMILOG, _("Write real numbers in semi-logarithmic format (e.g. 5.3e2)"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mSemilog->SetValue(FALSE);
    itemBoxSizer60->Add(mSemilog, 0, wxALIGN_LEFT|wxALL, 5);

    mCRLines = new wxCheckBox;
    mCRLines->Create( itemWizardPage59, CD_TR_CR_LINES, _("Accept lines terminated by CR only"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mCRLines->SetValue(FALSE);
    itemBoxSizer60->Add(mCRLines, 0, wxALIGN_LEFT|wxALL, 5);

////@end WizardPage4 content construction
  mSkipHeader->SetRange(0, 30000);
  mCsvSepar->Append(wxT(","));
  mCsvSepar->Append(wxT(";"));
  mCsvSepar->Append(wxT("|"));
  mCsvSepar->Append(wxT("%"));
  mCsvSepar->Append(wxT("#"));
  mCsvSepar->Append(wxT("TAB"));
  mCsvSepar->Append(_("Space"));
  mCsvDelim->Append(wxT("\""));
  mCsvDelim->Append(wxT("\'"));
  mDateFormat->Append(wxT("DD.MM.CCYY"));
  mDateFormat->Append(wxT("MM/DD/CCYY"));
  mDateFormat->Append(wxT("CCYY-MM-DD"));
  mDateFormat->Append(wxT("D.M.CCYY"));
  mDateFormat->Append(wxT("DD.MM."));
  mDateFormat->Append(wxT("MM/DD"));
  mDateFormat->Append(wxT("D.M."));
  mDateFormat->Append(wxT("DDMMCCYY"));
  mDateFormat->Append(wxT("CCYYMMDD"));
  //LIMITTEXT, FORMAT_STRING_SIZE
  mTimeFormat->Append(wxT("hh:mm"));
  mTimeFormat->Append(wxT("h:m"));
  mTimeFormat->Append(wxT("hh:mm:ss"));
  mTimeFormat->Append(wxT("h:m:s"));
  mTimeFormat->Append(wxT("hh:mm:ss.fff"));
  mTimeFormat->Append(wxT("h:m:s.fff"));
  mTimeFormat->Append(wxT("hhmmss"));
  mTimeFormat->Append(wxT("hhmmssfff"));
  mTsFormat->Append(wxT("DD.MM.CCYY hh:mm:ss"));
  mTsFormat->Append(wxT("CCYY-MM-DD hh:mm:ss"));
  mTsFormat->Append(wxT("CCYYMMDDhhmmss"));
  mBoolFormat->Append(wxT("False,True"));
  mBoolFormat->Append(wxT("0,1"));
  mDecSepar->Append(wxT("."));
  mDecSepar->Append(wxT(","));
 // add selected values:
  t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  if (mDateFormat->FindString(wxString(dsgn->date_form, *wxConvCurrent)) == wxNOT_FOUND) 
    mDateFormat->Append(wxString(dsgn->date_form, *wxConvCurrent));
  mDateFormat->SetValue(wxString(dsgn->date_form, *wxConvCurrent));
  if (mTimeFormat->FindString(wxString(dsgn->time_form, *wxConvCurrent)) == wxNOT_FOUND) 
    mTimeFormat->Append(wxString(dsgn->time_form, *wxConvCurrent));
  mTimeFormat->SetValue(wxString(dsgn->time_form, *wxConvCurrent));
  if (mTsFormat->FindString(wxString(dsgn->timestamp_form, *wxConvCurrent)) == wxNOT_FOUND) 
    mTsFormat->Append(wxString(dsgn->timestamp_form, *wxConvCurrent));
  mTsFormat->SetValue(wxString(dsgn->timestamp_form, *wxConvCurrent));
  if (mBoolFormat->FindString(wxString(dsgn->boolean_form, *wxConvCurrent)) == wxNOT_FOUND) 
    mBoolFormat->Append(wxString(dsgn->boolean_form, *wxConvCurrent));
  mBoolFormat->SetValue(wxString(dsgn->boolean_form, *wxConvCurrent));
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE4
 */

void WizardPage4::OnWizardpage4PageChanged( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  mCreHeader ->Enable(dsgn->text_target());
  mSkipHeader->Enable(dsgn->text_source());  mSkipHeaderC->Enable(dsgn->text_source());
  mSemilog   ->Enable(dsgn->text_target());
  mCRLines   ->Enable(dsgn->text_source());
 // show values:
  mSkipHeader->SetValue(dsgn->skip_lines);
  char aux[2];  aux[1]=0;
  aux[0]=dsgn->csv_separ;  
  mCsvSepar->SetValue(wxString(aux, *wxConvCurrent));
  aux[0]=dsgn->csv_quote;  
  mCsvDelim->SetValue(wxString(aux, *wxConvCurrent));
  mDateFormat->SetValue(wxString(dsgn->date_form, *wxConvCurrent));
  mTimeFormat->SetValue(wxString(dsgn->time_form, *wxConvCurrent));
  mTsFormat->SetValue(wxString(dsgn->timestamp_form, *wxConvCurrent));
  mBoolFormat->SetValue(wxString(dsgn->boolean_form, *wxConvCurrent));
  wxChar sep[2];  sep[0]=dsgn->decim_separ;  sep[1]=0; 
  mDecSepar->SetValue(sep);
  mCreHeader->SetValue(dsgn->add_header!=0);
  mSemilog->SetValue(dsgn->semilog!=0);
  mCRLines->SetValue(!dsgn->lfonly!=0);
  event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE4
 */

void WizardPage4::OnWizardpage4PageChanging( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  char separ, quote;
 // CSV separator:
  if (mCsvSepar->GetValue()==_("Space")) separ=' ';
  else if (mCsvSepar->GetValue()==wxT("TAB")) separ='\t';
  else if (mCsvSepar->GetValue().Length()==1) separ=mCsvSepar->GetValue()[0u];
  else { error_box(_("Bad CSV field separator."));  mCsvSepar->SetFocus();  return; }
 // CSV quote:
  if (mCsvDelim->GetValue().Length()==1 && separ!=mCsvDelim->GetValue()[0u])
    quote=mCsvDelim->GetValue()[0u];
  else { error_box(_("Bad CSV quote field."));  mCsvSepar->SetFocus();  return; }
 // decimal separator:
  if (mDecSepar->GetValue().Length()==1)
    dsgn->decim_separ=(char)mDecSepar->GetValue().GetChar(0);
  else { error_box(_("Bad decimal separator."));  mDecSepar->SetFocus();  return; }

  int sl=mSkipHeader->GetValue();
  if (sl<0 || sl>99999)
    { error_box(_("Bad number of skipped header lines."));   mSkipHeader->SetFocus();   return; }
 // data OK, commit changes:
  dsgn->csv_separ=separ;  dsgn->csv_quote=quote;  dsgn->skip_lines=sl;
  strmaxcpy(dsgn->date_form,      mDateFormat->GetValue().mb_str(*wxConvCurrent), sizeof(dsgn->date_form));
  strmaxcpy(dsgn->time_form,      mTimeFormat->GetValue().mb_str(*wxConvCurrent), sizeof(dsgn->time_form));
  strmaxcpy(dsgn->timestamp_form, mTsFormat  ->GetValue().mb_str(*wxConvCurrent), sizeof(dsgn->timestamp_form));
  strmaxcpy(dsgn->boolean_form,   mBoolFormat->GetValue().mb_str(*wxConvCurrent), sizeof(dsgn->boolean_form));
  dsgn->add_header= mCreHeader->GetValue();
  dsgn->semilog   = mSemilog  ->GetValue();
  dsgn->lfonly    =!mCRLines  ->GetValue();
  event.Skip();
}

/*!
 * Should we show tooltips?
 */

bool WizardPage4::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap WizardPage4::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WizardPage4 bitmap retrieval
    return wxNullBitmap;
////@end WizardPage4 bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WizardPage4::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WizardPage4 icon retrieval
    return wxNullIcon;
////@end WizardPage4 icon retrieval
}

/*!
 * WizardPageXSD type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPageXSD, wxWizardPage )

/*!
 * WizardPageXSD event table definition
 */

BEGIN_EVENT_TABLE( WizardPageXSD, wxWizardPage )

////@begin WizardPageXSD event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, WizardPageXSD::OnWizardpageXsdPageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, WizardPageXSD::OnWizardpageXsdPageChanging )

////@end WizardPageXSD event table entries

END_EVENT_TABLE()

/*!
 * WizardPageXSD constructors
 */

WizardPageXSD::WizardPageXSD( )
{
}

WizardPageXSD::WizardPageXSD( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPageXSD creator
 */

bool WizardPageXSD::Create( wxWizard* parent )
{
////@begin WizardPageXSD member initialisation
    mPrefix = NULL;
    mLength = NULL;
    mUnicode = NULL;
    mIDType = NULL;
    mDADNameCapt = NULL;
    mDADName = NULL;
    mRefInt = NULL;
////@end WizardPageXSD member initialisation

////@begin WizardPageXSD creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPage::Create( parent, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end WizardPageXSD creation
    return TRUE;
}

/*!
 * Control creation for WizardPageXSD
 */

void WizardPageXSD::CreateControls()
{    
////@begin WizardPageXSD content construction
    WizardPageXSD* itemWizardPage89 = this;

    wxBoxSizer* itemBoxSizer90 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage89->SetSizer(itemBoxSizer90);

    wxFlexGridSizer* itemFlexGridSizer91 = new wxFlexGridSizer(3, 2, 0, 0);
    itemFlexGridSizer91->AddGrowableCol(1);
    itemBoxSizer90->Add(itemFlexGridSizer91, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText92 = new wxStaticText;
    itemStaticText92->Create( itemWizardPage89, wxID_STATIC, _("Table name prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer91->Add(itemStaticText92, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPrefix = new wxTextCtrl;
    mPrefix->Create( itemWizardPage89, CD_XMLXSD_PREFIX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        mPrefix->SetToolTip(_("Prefix the names of all tables created for the XML data by the wizard"));
    itemFlexGridSizer91->Add(mPrefix, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText94 = new wxStaticText;
    itemStaticText94->Create( itemWizardPage89, wxID_STATIC, _("Default string length:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer91->Add(itemStaticText94, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLength = new wxSpinCtrl;
    mLength->Create( itemWizardPage89, CD_XMLXSD_LENGTH, _("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
    if (ShowToolTips())
        mLength->SetToolTip(_("Length of database columns for XML strings without a specified length in the schema"));
    itemFlexGridSizer91->Add(mLength, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer91->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mUnicode = new wxCheckBox;
    mUnicode->Create( itemWizardPage89, CD_XMLXSD_UNICODE, _("Use Unicode strings"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mUnicode->SetValue(FALSE);
    if (ShowToolTips())
        mUnicode->SetToolTip(_("Use Unicode string columns for storing string data from XML documents in the database"));
    itemFlexGridSizer91->Add(mUnicode, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText98 = new wxStaticText;
    itemStaticText98->Create( itemWizardPage89, wxID_STATIC, _("ID column type:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer91->Add(itemStaticText98, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mIDTypeStrings = NULL;
    mIDType = new wxOwnerDrawnComboBox;
    mIDType->Create( itemWizardPage89, CD_XMLXSD_IDTYPE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mIDTypeStrings, wxCB_READONLY );
    if (ShowToolTips())
        mIDType->SetToolTip(_("Database type of the ID and UPPER_ID columns joining the related database tables containing XML data"));
    itemFlexGridSizer91->Add(mIDType, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mDADNameCapt = new wxStaticText;
    mDADNameCapt->Create( itemWizardPage89, CD_XMLXSD_DADNAME_CAPT, _("DAD name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer91->Add(mDADNameCapt, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mDADName = new wxTextCtrl;
    mDADName->Create( itemWizardPage89, CD_XMLXSD_DADNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer91->Add(mDADName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxString mRefIntStrings[] = {
        _("&Do not create"),
        _("&Create constrains who do not increase the number of tables"),
        _("Create &always")
    };
    mRefInt = new wxRadioBox;
    mRefInt->Create( itemWizardPage89, CD_XMLXSD_REFINT, _("Referential integrity constraints in tables"), wxDefaultPosition, wxDefaultSize, 3, mRefIntStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer90->Add(mRefInt, 0, wxGROW|wxALL, 5);

////@end WizardPageXSD content construction
   // set restrictions:
    mPrefix->SetMaxLength(20);
    mLength->SetRange(2, MAX_FIXED_STRING_LENGTH);
    mIDType->Append(_("Integer (32-bit)"));
    mIDType->Append(_("BigInt (64-bit)"));
    mDADName->SetMaxLength(OBJ_NAME_LEN);
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE_XSD
 */

void WizardPageXSD::OnWizardpageXsdPageChanged( wxWizardEvent& event )
{ t_xsd_gen_params * gen_params = ((NewDataTransfer95Dlg*)GetParent())->gen_params;
  wxString fname = ((NewDataTransfer95Dlg*)GetParent())->mPageXML->query;
  const wxChar * p = fname;
  int i=(int)wcslen(p);
  while (i && p[i-1]!=PATH_SEPARATOR) i--;
  p+=i;
  for (i=0;  p[i] && p[i]!='.' && i<20-1;  i++)
    gen_params->tabname_prefix[i]=p[i];
  gen_params->tabname_prefix[i]='_';
  gen_params->tabname_prefix[i+1]=0;
 // show values:
  mPrefix->SetValue(wxString(gen_params->tabname_prefix, *wxConvCurrent));
  mIDType->SetSelection(gen_params->id_type==ATT_INT32 ? 0 : 1);
  mLength->SetValue(gen_params->string_length);
  mUnicode->SetValue(gen_params->unicode_strings);
  mRefInt->SetSelection(gen_params->refint);
  if (((NewDataTransfer95Dlg*)GetParent())->mPageXML->xml_init!=XST_FO)
    { mDADName->Disable();  mDADNameCapt->Disable(); } 
  else
    mDADName->SetValue(wxString(gen_params->dad_name, *wxConvCurrent));
  event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE_XSD
 */

void WizardPageXSD::OnWizardpageXsdPageChanging( wxWizardEvent& event )
{ t_xsd_gen_params * gen_params = ((NewDataTransfer95Dlg*)GetParent())->gen_params;
  wxString pref, dad;
  pref = mPrefix->GetValue();  pref.Trim(false);  pref.Trim(true);
  strcpy(gen_params->tabname_prefix, pref.mb_str(*wxConvCurrent));
  gen_params->id_type = mIDType->GetSelection() ? ATT_INT64 : ATT_INT32;
  gen_params->string_length=mLength->GetValue();
  gen_params->unicode_strings = mUnicode->GetValue();
  gen_params->refint = mRefInt->GetSelection();
 // DAD name - will be ignored for XSD:
  dad = mDADName->GetValue();  dad.Trim(false);  dad.Trim(true);
  strcpy(gen_params->dad_name, dad.mb_str(*wxConvCurrent));
  if (!event.GetDirection()) return;  // going back, no more tests
  if (((NewDataTransfer95Dlg*)GetParent())->mPageXML->xml_init!=XST_FO) 
    return;  // ignoring DAD name
  t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  if (dad.IsEmpty() || !is_object_name(dsgn->cdp, dad, this))
    { mDADName->SetFocus();  event.Veto();  return; }
  tobjnum auxobj;
  if (!cd_Find_prefixed_object(dsgn->cdp, NULL, gen_params->dad_name, CATEG_PGMSRC, &auxobj)) 
    { unpos_box(OBJ_NAME_USED);  mDADName->SetFocus();  event.Veto();  return; }
}



/*!
 * Should we show tooltips?
 */

bool WizardPageXSD::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap WizardPageXSD::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WizardPageXSD bitmap retrieval
    return wxNullBitmap;
////@end WizardPageXSD bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WizardPageXSD::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WizardPageXSD icon retrieval
    return wxNullIcon;
////@end WizardPageXSD icon retrieval
}
/*!
 * wxEVT_WIZARD_HELP event handler for ID_MYWIZARD
 */

void NewDataTransfer95Dlg::OnMywizardHelp( wxWizardEvent& event )
{ wxString page;
  wxWizardPage * curr = GetCurrentPage();
  if      (curr==mPageMainSel)    page=wxT("xml/html/tran_designer.html#page1");
  else if (curr==mPageDataSource) page=wxT("xml/html/tran_designer.html#page2");
  else if (curr==mPageDataTarget) page=wxT("xml/html/tran_designer.html#page3");
  else if (curr==mPageText)       page=wxT("xml/html/tran_designer.html#page4");
  else if (curr==mPageXML)        page=wxT("xml/html/tran_designer.html");
  else                            page=wxT("xml/html/tran_designer.html");
  wxGetApp().frame->help_ctrl.DisplaySection(page);  
}


//*********************************** XML DB ***************************************
/*!
 * WizardPageDbxml type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPageDbxml, wxWizardPage )

/*!
 * WizardPageDbxml event table definition
 */

BEGIN_EVENT_TABLE( WizardPageDbxml, wxWizardPage )

////@begin WizardPageDbxml event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, WizardPageDbxml::OnWizardpageDbxmlPageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, WizardPageDbxml::OnWizardpageDbxmlPageChanging )

    EVT_COMBOBOX( CD_XMLDB_SERVER, WizardPageDbxml::OnCdXmldbServerSelected )

    EVT_RADIOBUTTON( CD_XMLDB_BY_TABLE, WizardPageDbxml::OnCdXmldbByTableSelected )

    EVT_COMBOBOX( CD_XMLDB_SCHEMA, WizardPageDbxml::OnCdXmldbSchemaSelected )

    EVT_RADIOBUTTON( CD_XMLDB_BY_QUERY, WizardPageDbxml::OnCdXmldbByQuerySelected )

////@end WizardPageDbxml event table entries

END_EVENT_TABLE()

/*!
 * WizardPageDbxml constructors
 */

WizardPageDbxml::WizardPageDbxml( )
{
}

WizardPageDbxml::WizardPageDbxml( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPageDbxml creator
 */

bool WizardPageDbxml::Create( wxWizard* parent )
{
////@begin WizardPageDbxml member initialisation
    mServer = NULL;
    mByTable = NULL;
    mSchemaC = NULL;
    mSchema = NULL;
    mTableNameC = NULL;
    mTableName = NULL;
    mTableAliasC = NULL;
    mTableAlias = NULL;
    mByQuery = NULL;
    mQuery = NULL;
////@end WizardPageDbxml member initialisation

////@begin WizardPageDbxml creation
    wxBitmap wizardBitmap(wxNullBitmap);
    wxWizardPage::Create( parent, wizardBitmap );

    CreateControls();
    GetSizer()->Fit(this);
////@end WizardPageDbxml creation
    return TRUE;
}

/*!
 * Control creation for WizardPageDbxml
 */

void WizardPageDbxml::CreateControls()
{    
////@begin WizardPageDbxml content construction
    WizardPageDbxml* itemWizardPage103 = this;

    wxBoxSizer* itemBoxSizer104 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage103->SetSizer(itemBoxSizer104);

    wxFlexGridSizer* itemFlexGridSizer105 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer105->AddGrowableCol(1);
    itemBoxSizer104->Add(itemFlexGridSizer105, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText106 = new wxStaticText;
    itemStaticText106->Create( itemWizardPage103, wxID_STATIC, _("Data source:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText106, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mServerStrings = NULL;
    mServer = new wxOwnerDrawnComboBox;
    mServer->Create( itemWizardPage103, CD_XMLDB_SERVER, _T(""), wxDefaultPosition, wxDefaultSize, 0, mServerStrings, wxCB_READONLY );
    itemFlexGridSizer105->Add(mServer, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mByTable = new wxRadioButton;
    mByTable->Create( itemWizardPage103, CD_XMLDB_BY_TABLE, _("Table"), wxDefaultPosition, wxDefaultSize, 0 );
    mByTable->SetValue(FALSE);
    itemBoxSizer104->Add(mByTable, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer109 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer104->Add(itemBoxSizer109, 0, wxGROW|wxALL, 0);

    itemBoxSizer109->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer111 = new wxFlexGridSizer(3, 2, 0, 0);
    itemFlexGridSizer111->AddGrowableCol(1);
    itemBoxSizer109->Add(itemFlexGridSizer111, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    mSchemaC = new wxStaticText;
    mSchemaC->Create( itemWizardPage103, CD_XMLDB_SCHEMA_CAPT, _("Schema:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer111->Add(mSchemaC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    wxString* mSchemaStrings = NULL;
    mSchema = new wxOwnerDrawnComboBox;
    mSchema->Create( itemWizardPage103, CD_XMLDB_SCHEMA, _T(""), wxDefaultPosition, wxDefaultSize, 0, mSchemaStrings, wxCB_READONLY );
    itemFlexGridSizer111->Add(mSchema, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mTableNameC = new wxStaticText;
    mTableNameC->Create( itemWizardPage103, wxID_STATIC, _("Table name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer111->Add(mTableNameC, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    wxString* mTableNameStrings = NULL;
    mTableName = new wxOwnerDrawnComboBox;
    mTableName->Create( itemWizardPage103, CD_XMLDB_TABLENAME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTableNameStrings, wxCB_DROPDOWN|wxCB_SORT );
    itemFlexGridSizer111->Add(mTableName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    mTableAliasC = new wxStaticText;
    mTableAliasC->Create( itemWizardPage103, wxID_STATIC, _("Alias:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer111->Add(mTableAliasC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    mTableAlias = new wxTextCtrl;
    mTableAlias->Create( itemWizardPage103, CD_XMLDB_TABLEALIAS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer111->Add(mTableAlias, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mByQuery = new wxRadioButton;
    mByQuery->Create( itemWizardPage103, CD_XMLDB_BY_QUERY, _("Query"), wxDefaultPosition, wxDefaultSize, 0 );
    mByQuery->SetValue(FALSE);
    itemBoxSizer104->Add(mByQuery, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer119 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer104->Add(itemBoxSizer119, 0, wxGROW|wxALL, 0);

    itemBoxSizer119->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mQuery = new wxTextCtrl;
    mQuery->Create( itemWizardPage103, CD_XMLDB_QUERY, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    itemBoxSizer119->Add(mQuery, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end WizardPageDbxml content construction
    t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
    WizardPage3 * xmlpg = ((NewDataTransfer95Dlg*)GetParent())->mPageXML;
    mTableAlias->SetMaxLength(OBJ_NAME_LEN);
   // fill data sources and select:
    for (t_avail_server * server = available_servers;  server;  server = server->next)
      if (server->odbc ? server->conn!=NULL : 
           server->cdp!=NULL && (!server->notreg || !stricmp(server->locid_server_name, dsgn->cdp->locid_server_name)))
        mServer->Append(wxString(server->locid_server_name, *wxConvCurrent));
    int ind = mServer->FindString(wxString(dsgn->cdp->locid_server_name, *wxConvCurrent));
    if (ind!=wxNOT_FOUND) // SetStringSelection displays Assert when not found
      mServer->SetSelection(ind);
   // emulate selection:
    wxCommandEvent event2;
    OnCdXmldbServerSelected(event2);


    //fill_table_names(dsgn->cdp, mTableName, false, "");
}

void WizardPageDbxml::enabling(t_xml_start_type sel)
{ bool bytable = sel==XST_TABLE;
  mSchemaC    ->Enable(bytable);  mSchema    ->Enable(bytable);
  mTableNameC ->Enable(bytable);  mTableName ->Enable(bytable);
  mTableAliasC->Enable(bytable);  mTableAlias->Enable(bytable);
  mQuery->Enable(sel==XST_QUERY);
}

/*!
 * Should we show tooltips?
 */

bool WizardPageDbxml::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap WizardPageDbxml::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WizardPageDbxml bitmap retrieval
    return wxNullBitmap;
////@end WizardPageDbxml bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WizardPageDbxml::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WizardPageDbxml icon retrieval
    return wxNullIcon;
////@end WizardPageDbxml icon retrieval
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE_DBXML
 */

void WizardPageDbxml::OnWizardpageDbxmlPageChanged( wxWizardEvent& event )
{
  t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  WizardPage3 * xmlpg = ((NewDataTransfer95Dlg*)GetParent())->mPageXML;
  bool is_table = xmlpg->xml_init==XST_TABLE;
  mByTable->SetValue(is_table);
  mByQuery->SetValue(!is_table);
  enabling(xmlpg->xml_init);
  event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE_DBXML
 */

void WizardPageDbxml::OnWizardpageDbxmlPageChanging( wxWizardEvent& event )
{
  t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  WizardPage3 * xmlpg = ((NewDataTransfer95Dlg*)GetParent())->mPageXML;
 // server and schema:
  int ind=mServer->GetSelection();
  if (ind!=-1)
    strcpy(xmlpg->server, mServer->GetString(ind).mb_str(*wxConvCurrent));
  else *xmlpg->server=0;
  ind=mSchema->GetSelection();
  if (ind!=-1)
    strcpy(xmlpg->schema, mSchema->GetString(ind).mb_str(*dsgn->cdp->sys_conv));

  if (mByTable->GetValue())
  { xmlpg->xml_init=XST_TABLE;
    if (!event.GetDirection()) return;  // going back, no more tests
    int ind = mTableName->GetSelection();
    if (ind==-1) 
      { event.Veto();  mTableName->SetFocus();  wxBell();  return; }
    strcpy(xmlpg->table_name, mTableName->GetString(ind).mb_str(*dsgn->cdp->sys_conv));
    wxString alias = mTableAlias->GetValue();
    alias.Trim(true);  alias.Trim(false);
    strcpy(xmlpg->table_alias, alias.mb_str(*dsgn->cdp->sys_conv));
    
  }
  else if (mByQuery->GetValue())
  { xmlpg->xml_init=XST_QUERY;
    if (!event.GetDirection()) return;  // going back, no more tests
    xmlpg->query=mQuery->GetValue();
    xmlpg->new_td = get_expl_descriptor(dsgn->cdp, xmlpg->query.mb_str(*dsgn->cdp->sys_conv), xmlpg->server, NULL, &xmlpg->new_ucdp, &xmlpg->new_top_curs); 
    if (!xmlpg->new_td) 
      { event.Veto();  mQuery->SetFocus();  wxBell();  return; }
  }
  else
  { event.Veto();  
    wxBell();  
  }
}



/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_XMLDB_SERVER
 */

void WizardPageDbxml::OnCdXmldbServerSelected( wxCommandEvent& event )
{
  t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // fill schema names:
  mSchema->Clear();
  wxString dsn;  
  int sel = GetNewComboSelection(mServer);
  if (sel>=0) dsn=mServer->GetString(sel); 
  const t_avail_server * as = find_server_connection_info(dsn.IsEmpty() ? dsgn->cdp->locid_server_name : dsn.mb_str(*wxConvCurrent));
  if (as)
  { CObjectList ol(as, NULL);
    for (bool ItemFound = ol.FindFirst(CATEG_APPL); ItemFound; ItemFound = ol.FindNext())
      mSchema->Append(GetSmallStringName(ol));
    if (!as->odbc || mSchema->GetCount()==0)
      mSchema->Append(wxEmptyString);  // is sorted, will be the first
   // show the current schema name (may not be found if a different server selected):
    wxString orig(dsgn->cdp->sel_appl_name, GetConv(as->cdp));
	  CaptFirst(orig);
    int ind = mSchema->FindString(orig);
    if (ind!=wxNOT_FOUND) // setStringSelection displays Assert when not found
      mSchema->SetSelection(ind);
  }
 // feedback:
  wxCommandEvent event2;
  OnCdXmldbSchemaSelected(event2);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_XMLDB_SCHEMA
 */

void WizardPageDbxml::OnCdXmldbSchemaSelected( wxCommandEvent& event )
{
  t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
 // find the selected server, I need the cdp:
  wxString dsn;  
  int sel = GetNewComboSelection(mServer);
  if (sel>=0) dsn=mServer->GetString(sel); 
  const t_avail_server * as = find_server_connection_info(dsn.IsEmpty() ? dsgn->cdp->locid_server_name : dsn.mb_str(*wxConvCurrent));
  if (!as) return;  // fuse
 // get schema name:
  sel = GetNewComboSelection(mSchema);
  wxString schema;
  if (sel>=0) schema=mSchema->GetString(sel);
 // prevent using empty schema on ODBC when schemas exist (this would place schmea names into combo for tables)
  //if (as->odbc && sel==0 && mSchema->GetCount()>1)         -- empty schema not inserted above
  //  schema=wxT("@@@");
 // fill table names:
  mTableName->Clear();
  { wxCharBuffer cbu = schema.mb_str(GetConv(as->cdp));  // must exist until ol is destructed!
    CObjectList ol(as, cbu);
    for (bool ItemFound = ol.FindFirst(CATEG_TABLE);  ItemFound;  ItemFound = ol.FindNext())
      mTableName->Append(GetSmallStringName(ol), (void*)(size_t)(as->odbc ? -2 : ol.GetObjnum()));  // objnum must not be 0 for ODBC, wx replaces -1 by 0!
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON3
 */

void WizardPageDbxml::OnCdXmldbByTableSelected( wxCommandEvent& event )
{ mByQuery->SetValue(false);  
  t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  WizardPage3 * xmlpg = ((NewDataTransfer95Dlg*)GetParent())->mPageXML;
  xmlpg->xml_init=XST_TABLE;  
  enabling(xmlpg->xml_init);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON4
 */

void WizardPageDbxml::OnCdXmldbByQuerySelected( wxCommandEvent& event )
{ mByTable->SetValue(false);  
  t_ie_run * dsgn = ((NewDataTransfer95Dlg*)GetParent())->dsgn;
  WizardPage3 * xmlpg = ((NewDataTransfer95Dlg*)GetParent())->mPageXML;
  xmlpg->xml_init=XST_QUERY;  
  enabling(xmlpg->xml_init);
  event.Skip();
}


/************** page order **********************************/
wxWizardPage* WizardPage2::GetPrev() const   // this is the initial page
{ return NULL; }  

wxWizardPage* WizardPage2::GetNext() const
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  if (mMainSel->GetSelection()==0) return dlg->mPageXML;
  else return dlg->mPageDataSource;
}

wxWizardPage* WizardPage::GetPrev() const    // data source
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  return dlg->updating ? NULL : dlg->mPageMainSel;
}

wxWizardPage* WizardPage::GetNext() const
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  return dlg->mPageDataTarget;
}

wxWizardPage* WizardPage1::GetPrev() const   // data target
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  return dlg->mPageDataSource;
}

wxWizardPage* WizardPage1::GetNext() const
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  if (dlg->dsgn->text_source() || dlg->dsgn->text_target())
    return dlg->mPageText;
  return NULL;   // the last page if no format is text
}

wxWizardPage* WizardPage3::GetPrev() const   // XML
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  return dlg->mPageMainSel;
}

wxWizardPage* WizardPage3::GetNext() const
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  if (xml_init==XST_XSD   || xml_init==XST_FO) return dlg->mPageXSD;
  if (xml_init==XST_TABLE || xml_init==XST_QUERY) return dlg->mPageDbxml;
  return NULL;
}

wxWizardPage* WizardPage4::GetPrev() const   // Text options
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  return dlg->mPageDataTarget;
}

wxWizardPage* WizardPage4::GetNext() const
{ return NULL; }  // the last page

wxWizardPage* WizardPageXSD::GetPrev() const
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  return dlg->mPageXML;
}

wxWizardPage* WizardPageXSD::GetNext() const
{ return NULL; }  // the last page

wxWizardPage* WizardPageDbxml::GetPrev() const
{ NewDataTransfer95Dlg * dlg = (NewDataTransfer95Dlg*)GetParent();
  return dlg->mPageXML;
}

wxWizardPage* WizardPageDbxml::GetNext() const
{ return NULL; }  // the last page


