/////////////////////////////////////////////////////////////////////////////
// Name:        NewDataTransferDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/02/04 14:05:22
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "NewDataTransferDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "NewDataTransferDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * NewDataTransferDlg type definition
 */

IMPLEMENT_CLASS( NewDataTransferDlg, wxWizard )

/*!
 * NewDataTransferDlg event table definition
 */

BEGIN_EVENT_TABLE( NewDataTransferDlg, wxWizard )

////@begin NewDataTransferDlg event table entries
    EVT_WIZARD_HELP( ID_WIZARD, NewDataTransferDlg::OnWizardHelp )

////@end NewDataTransferDlg event table entries

END_EVENT_TABLE()

/*!
 * NewDataTransferDlg constructors
 */

NewDataTransferDlg::NewDataTransferDlg(t_ie_run * dsgnIn, bool xmlIn, t_xsd_gen_params * gen_paramsIn)
{ dsgn=dsgnIn;  gen_params=gen_paramsIn;
  xml_transport=xmlIn;
  mPageMainSel=NULL;  mPageDataSource=NULL;  mPageDataTarget=NULL;  mPageXML=NULL;  // used in FitToPage before creation!
  mPageText=NULL;  mPageXSD = NULL;
  updating=false;
}

/*!
 * NewDataTransferDlg creator
 */

bool NewDataTransferDlg::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos )
{
////@begin NewDataTransferDlg member initialisation
    mPageMainSel = NULL;
    mPageDataSource = NULL;
    mPageDataTarget = NULL;
    mPageXML = NULL;
    mPageText = NULL;
    mPageXSD = NULL;
////@end NewDataTransferDlg member initialisation
   // remove all except mPage...=NULL; from above

////@begin NewDataTransferDlg creation
    SetExtraStyle(GetExtraStyle()|wxWIZARD_EX_HELPBUTTON);
    wxBitmap wizardBitmap(GetBitmapResource(wxT("trigwiz")));
    wxWizard::Create( parent, id, _("Create a New Data Transport"), wizardBitmap, pos );

    CreateControls();
////@end NewDataTransferDlg creation
    SetBackgroundColour(GetDefaultAttributes().colBg);  // needed in wx 2.5.3
    return TRUE;
}

/*!
 * Control creation for NewDataTransferDlg
 */

void NewDataTransferDlg::CreateControls()
{    
////@begin NewDataTransferDlg content construction
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
    wxWizardPage* lastPage = NULL;
////@end NewDataTransferDlg content construction
}

/*!
 * Runs the wizard.
 */

bool NewDataTransferDlg::Run2(void)
// Normal start is with xml_transport==true, when started from a table, xml_transport==false
{ return RunWizard(xml_transport ? (wxWizardPage*)mPageMainSel : (wxWizardPage*)mPageDataSource); }

bool NewDataTransferDlg::RunUpdate(int page)
{ updating=true;
  SetTitle(_("Update Data Transport Design"));
  return RunWizard(page==1 ? (wxWizardPage*)mPageDataSource : page==2 ? (wxWizardPage*)mPageDataTarget :
                   (wxWizardPage*)mPageText); 
}

/*!
 * Should we show tooltips?
 */

bool NewDataTransferDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * WizardPage type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPage, wxWizardPageSimple )

/*!
 * WizardPage event table definition
 */

BEGIN_EVENT_TABLE( WizardPage, wxWizardPage )

////@begin WizardPage event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, WizardPage::OnWizardpagePageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, WizardPage::OnWizardpagePageChanging )

    EVT_RADIOBOX( CD_TR_SOURCE_ORIGIN, WizardPage::OnCdTrSourceOriginSelected )

    EVT_COMBOBOX( CD_TR_SOURCE_TYPE, WizardPage::OnCdTrSourceTypeSelected )

    EVT_BUTTON( CD_TR_SOURCE_NAME_SEL, WizardPage::OnCdTrSourceNameSelClick )

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
    mSourceTypeC = NULL;
    mSourceType = NULL;
    mSrcEncC = NULL;
    mSourceEnc = NULL;
    mSourceNameC = NULL;
    mSourceName = NULL;
    mSourceNameSel = NULL;
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
        _("&Database"),
        _("&External file")
    };
    mSourceOrigin = new wxRadioBox;
    mSourceOrigin->Create( itemWizardPage5, CD_TR_SOURCE_ORIGIN, _("Take data from"), wxDefaultPosition, wxDefaultSize, 2, mSourceOriginStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer6->Add(mSourceOrigin, 0, wxGROW|wxALL, 5);

    mSourceTypeC = new wxStaticText;
    mSourceTypeC->Create( itemWizardPage5, CD_TR_SOURCE_TYPE_C, _("Format of the source file:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mSourceTypeC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer6->Add(itemBoxSizer9, 0, wxGROW|wxALL, 0);

    wxString* mSourceTypeStrings = NULL;
    mSourceType = new wxComboBox;
    mSourceType->Create( itemWizardPage5, CD_TR_SOURCE_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mSourceTypeStrings, wxCB_READONLY );
    itemBoxSizer9->Add(mSourceType, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSrcEncC = new wxStaticText;
    mSrcEncC->Create( itemWizardPage5, CD_TR_SOURCE_ENC_C, _("Source file encoding:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mSrcEncC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer6->Add(itemBoxSizer12, 0, wxGROW|wxALL, 0);

    wxString* mSourceEncStrings = NULL;
    mSourceEnc = new wxComboBox;
    mSourceEnc->Create( itemWizardPage5, CD_TR_SOURCE_ENC, _T(""), wxDefaultPosition, wxDefaultSize, 0, mSourceEncStrings, wxCB_DROPDOWN );
    itemBoxSizer12->Add(mSourceEnc, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSourceNameC = new wxStaticText;
    mSourceNameC->Create( itemWizardPage5, CD_TR_SOURCE_NAME_C, _("Source file name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mSourceNameC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer15 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer6->Add(itemBoxSizer15, 0, wxGROW|wxALL, 0);

    wxString* mSourceNameStrings = NULL;
    mSourceName = new wxComboBox;
    mSourceName->Create( itemWizardPage5, CD_TR_SOURCE_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mSourceNameStrings, wxCB_DROPDOWN|wxCB_SORT );
    itemBoxSizer15->Add(mSourceName, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSourceNameSel = new wxButton;
    mSourceNameSel->Create( itemWizardPage5, CD_TR_SOURCE_NAME_SEL, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemBoxSizer15->Add(mSourceNameSel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    mConditionC = new wxStaticText;
    mConditionC->Create( itemWizardPage5, CD_TR_CONDITION_C, _("Take only rows satisfying the condition:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mConditionC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mCondition = new wxTextCtrl;
    mCondition->Create( itemWizardPage5, CD_TR_CONDITION, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mCondition, 0, wxGROW|wxALL, 5);

////@end WizardPage content construction
}

void WizardPage::fill_source_types(int selection)
{ mSourceType->Clear();
  if (selection==0)
  { mSourceType->Append(_("Table"), (void*)IETYPE_WB);
    mSourceType->Append(_("Query"), (void*)IETYPE_CURSOR);
    mSourceTypeC->SetLabel(_("&Database data source:"));
    mSourceNameC->SetLabel(_("&Source object name:"));
    mConditionC->Enable();  mCondition->Enable();
    mSourceNameSel->Disable();
  }
  else
  { mSourceType->Append(_("Native 602SQL table data"),       (void*)IETYPE_TDT);
    mSourceType->Append(_("Comma separated values"), (void*)IETYPE_CSV);
    mSourceType->Append(_("Values in columns"), (void*)IETYPE_COLS);
    mSourceType->Append(_("dBase DBF"),         (void*)IETYPE_DBASE);
    mSourceType->Append(_("FoxPro DBF"),        (void*)IETYPE_FOX);
    mSourceTypeC->SetLabel(_("&Format of the source file:"));
    mSourceNameC->SetLabel(_("&Source file name:"));
    mConditionC->Disable();  mCondition->Disable();
    mSourceNameSel->Enable();
  }
}

void WizardPage::fill_source_objects(t_ie_run * dsgn)
{ mSourceName->Clear();
  tcateg cat = mSourceType->GetSelection()==IETYPE_WB ? CATEG_TABLE : CATEG_CURSOR;
  void * en = get_object_enumerator(dsgn->cdp, cat, NULL);
  tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
  while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
    if (cat==CATEG_TABLE ? !(flags & (CO_FLAG_ODBC | CO_FLAG_LINK)) : !(flags & CO_FLAG_LINK))
      mSourceName->Append(wxString(name, *dsgn->cdp->sys_conv));
  free_object_enumerator(en);
}
/*!
 * Should we show tooltips?
 */

bool WizardPage::ShowToolTips()
{
    return TRUE;
}

/*!
 * WizardPage1 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPage1, wxWizardPageSimple )

/*!
 * WizardPage1 event table definition
 */

BEGIN_EVENT_TABLE( WizardPage1, wxWizardPage )

////@begin WizardPage1 event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, WizardPage1::OnWizardpage1PageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, WizardPage1::OnWizardpage1PageChanging )

    EVT_RADIOBOX( CD_TR_DEST_ORIGIN, WizardPage1::OnCdTrDestOriginSelected )

    EVT_COMBOBOX( CD_TR_DEST_TYPE, WizardPage1::OnCdTrDestTypeSelected )

    EVT_COMBOBOX( CD_TR_DEST_NAME, WizardPage1::OnCdTrDestNameSelected )

    EVT_BUTTON( CD_TR_DEST_NAME_SEL, WizardPage1::OnCdTrDestNameSelClick )

    EVT_RADIOBOX( CD_TR_INDEXES, WizardPage1::OnCdTrIndexesSelected )

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
    mDestNameC = NULL;
    mDestName = NULL;
    mDestNameSel = NULL;
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
    WizardPage1* itemWizardPage20 = this;

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage20->SetSizer(itemBoxSizer21);

    wxString mDestOriginStrings[] = {
        _("&Database"),
        _("&External file")
    };
    mDestOrigin = new wxRadioBox;
    mDestOrigin->Create( itemWizardPage20, CD_TR_DEST_ORIGIN, _("Copy data to"), wxDefaultPosition, wxDefaultSize, 2, mDestOriginStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer21->Add(mDestOrigin, 0, wxGROW|wxALL, 5);

    mDestTypeC = new wxStaticText;
    mDestTypeC->Create( itemWizardPage20, CD_TR_DEST_TYPE_C, _("Format of the target file:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(mDestTypeC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer21->Add(itemBoxSizer24, 0, wxGROW|wxALL, 0);

    wxString* mDestTypeStrings = NULL;
    mDestType = new wxComboBox;
    mDestType->Create( itemWizardPage20, CD_TR_DEST_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDestTypeStrings, wxCB_READONLY );
    itemBoxSizer24->Add(mDestType, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mDestEncC = new wxStaticText;
    mDestEncC->Create( itemWizardPage20, CD_TR_DEST_ENC_C, _("Encoding of the target file:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(mDestEncC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer21->Add(itemBoxSizer27, 0, wxGROW|wxALL, 0);

    wxString* mDestEncStrings = NULL;
    mDestEnc = new wxComboBox;
    mDestEnc->Create( itemWizardPage20, CD_TR_DEST_ENC, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDestEncStrings, wxCB_READONLY );
    itemBoxSizer27->Add(mDestEnc, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mDestNameC = new wxStaticText;
    mDestNameC->Create( itemWizardPage20, CD_TR_DEST_NAME_C, _("Target file name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(mDestNameC, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer30 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer21->Add(itemBoxSizer30, 0, wxGROW|wxALL, 0);

    wxString* mDestNameStrings = NULL;
    mDestName = new wxComboBox;
    mDestName->Create( itemWizardPage20, CD_TR_DEST_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDestNameStrings, wxCB_DROPDOWN|wxCB_SORT );
    itemBoxSizer30->Add(mDestName, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mDestNameSel = new wxButton;
    mDestNameSel->Create( itemWizardPage20, CD_TR_DEST_NAME_SEL, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemBoxSizer30->Add(mDestNameSel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxString mIndexesStrings[] = {
        _("&Incremental update during import"),
        _("&Rebuild index after import")
    };
    mIndexes = new wxRadioBox;
    mIndexes->Create( itemWizardPage20, CD_TR_INDEXES, _("Updating index on the target table"), wxDefaultPosition, wxDefaultSize, 2, mIndexesStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer21->Add(mIndexes, 0, wxGROW|wxALL, 5);

////@end WizardPage1 content construction
}

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

void fill_encodings(wxComboBox * combo, wxStaticText * capt, int data_type)
{ combo->Clear();
  if (data_type==IETYPE_WB || data_type==IETYPE_CURSOR || data_type==IETYPE_TDT)
  { combo->Disable();  
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

void WizardPage1::fill_dest_types(int selection)
{ mDestType->Clear();
  if (selection==0)
  { mDestType->Append(_("Table (new or existing)"), (void*)IETYPE_WB);
    mDestTypeC->SetLabel(_("&Database data target:"));
    mIndexes->Enable();
    mDestNameSel->Disable();
    mDestNameC->SetLabel(_("&Target object name:"));
    mDestEnc->Disable();
    mDestEncC->Disable();
  }
  else
  { mDestType->Append(_("Native 602SQL table data"),       (void*)IETYPE_TDT);
    mDestType->Append(_("Comma separated values"), (void*)IETYPE_CSV);
    mDestType->Append(_("Values in columns"), (void*)IETYPE_COLS);
    mDestType->Append(_("dBase DBF"),         (void*)IETYPE_DBASE);
    mDestType->Append(_("FoxPro DBF"),        (void*)IETYPE_FOX);
    mDestTypeC->SetLabel(_("Format of the target file:"));
    mIndexes->Disable();
    mDestNameSel->Enable();
    mDestNameC->SetLabel(_("&Target file name:"));
    mDestEnc->Enable();
    mDestEncC->Enable();
  }
}

void WizardPage1::fill_dest_objects(t_ie_run * dsgn)
{ mDestName->Clear();
  tcateg cat = CATEG_TABLE;  // mDestType->GetSelection()==IETYPE_WB ? CATEG_TABLE : CATEG_CURSOR;
  void * en = get_object_enumerator(dsgn->cdp, cat, NULL);
  tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
  while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
    if (!(flags & (CO_FLAG_ODBC | CO_FLAG_LINK)))
      mDestName->Append(wxString(name, *dsgn->cdp->sys_conv));
  free_object_enumerator(en);
}
/*!
 * Should we show tooltips?
 */

bool WizardPage1::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TR_SOURCE_ORIGIN
 */

void WizardPage::OnCdTrSourceOriginSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  fill_source_types(mSourceOrigin->GetSelection());
  mSourceType->SetSelection(0);
  //mSourceName->SetValue(wxEmptyString); -- retaining, usefull when the origin is changed forth and back
  wxCommandEvent cmd;
  OnCdTrSourceTypeSelected( cmd );
  if (mSourceOrigin->GetSelection()==0) // restore the original database object name
  { tobjname upcase_name;
    strmaxcpy(upcase_name, dsgn->inpath, sizeof(upcase_name));
    Upcase9(dsgn->cdp, upcase_name);
    int ind = mSourceName->FindString(wxString(upcase_name, *dsgn->cdp->sys_conv));
    if (ind!=wxNOT_FOUND) // setStringSelection displays Assert when not found
      mSourceName->SetSelection(ind);
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TR_DEST_ORIGIN
 */

void WizardPage1::OnCdTrDestOriginSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  fill_dest_types(mDestOrigin->GetSelection());
  mDestType->SetSelection(0);
  //mDestName->SetValue(wxEmptyString); -- retaining, usefull when the origin is changed forth and back
  wxCommandEvent cmd;
  OnCdTrDestTypeSelected( cmd );
  if (mDestOrigin->GetSelection()==0) // restore the original database object name
  { tobjname upcase_name;
    strmaxcpy(upcase_name, dsgn->outpath, sizeof(upcase_name));
    Upcase9(dsgn->cdp, upcase_name);
    int ind = mDestName->FindString(wxString(upcase_name, *dsgn->cdp->sys_conv));
    if (ind!=wxNOT_FOUND) // setStringSelection displays Assert when not found
      mDestName->SetSelection(ind);
  }
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX
 */

void WizardPage1::OnCdTrIndexesSelected( wxCommandEvent& event )
{
    event.Skip();
}


/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_SOURCE_TYPE
 */

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

void update_file_name_suffix(wxComboBox * ctrl, int src_targ_type)
{ if (src_targ_type==IETYPE_WB || src_targ_type==IETYPE_CURSOR) return; // no file
  wxString fname;
  fname = ctrl->GetValue();
  if (fname.Length()>4 && fname.GetChar(fname.Length()-4)=='.')
  { fname = fname.Mid(0, fname.Length()-3);  // strip the suffix
    fname = fname + (src_targ_type==IETYPE_CSV ? wxT("csv") : src_targ_type==IETYPE_COLS ? wxT("txt") : 
                     src_targ_type==IETYPE_TDT ? wxT("tdt") : wxT("dbf"));
    ctrl->SetValue(fname);
  }
}

void WizardPage::OnCdTrSourceTypeSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  if (mSourceOrigin->GetSelection()==0)
    fill_source_objects(dsgn);
  else if (mSourceName->GetCount()>0) // if there is a choice, then clear it
    mSourceName->Clear(); // ... but if tere is not choice, then retain the file name in mSourceName
 // this will be done in ...PageChanging, but fill_encodings and file suffix need dsgn->source_type:
  if (mSourceOrigin->GetSelection()==0)
    dsgn->source_type = mSourceType->GetSelection()==0 ? IETYPE_WB : IETYPE_CURSOR;
  else
    dsgn->source_type = mSourceType->GetSelection()==0 ? IETYPE_TDT : mSourceType->GetSelection()==1 ? IETYPE_CSV : mSourceType->GetSelection()==2 ? IETYPE_COLS : mSourceType->GetSelection()==3 ? IETYPE_DBASE : IETYPE_FOX;
 // show possible encodings:
  fill_encodings(mSourceEnc, mSrcEncC, dsgn->source_type);
  if (dsgn->src_recode>mSourceEnc->GetCount()) dsgn->src_recode=0;
  mSourceEnc->SetSelection(dsgn->src_recode);
  update_file_name_suffix(mSourceName, dsgn->source_type);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_SOURCE_NAME_SEL
 */

void WizardPage::OnCdTrSourceNameSelClick( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  wxString filter = get_filter(mSourceType->GetSelection());  // must not be only the parameter of the next function!
  wxString str = GetImportFileName(mSourceName->GetValue(), filter, GetParent());
  if (!str.IsEmpty())
    mSourceName->SetValue(str);
}


/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_DEST_TYPE
 */

void WizardPage1::OnCdTrDestTypeSelected( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  if (mDestOrigin->GetSelection()==0)
    fill_dest_objects(dsgn);
  else if (mDestName->GetCount()>0) // if there is a choice, then clear it
    mDestName->Clear(); // ... but if tere is not choice, then retain the file name in mDestName
 // this will be done in ...PageChanging, but NextPage is called before and it needs the dest_type! fill_encodings too!
  if (mDestOrigin->GetSelection()==0)
    dsgn->dest_type = IETYPE_WB;
  else
    dsgn->dest_type = mDestType->GetSelection()==0 ? IETYPE_TDT : mDestType->GetSelection()==1 ? IETYPE_CSV : mDestType->GetSelection()==2 ? IETYPE_COLS : mDestType->GetSelection()==3 ? IETYPE_DBASE : IETYPE_FOX;
 // show possible encodings:
  fill_encodings(mDestEnc, mDestEncC, dsgn->dest_type);
  if (dsgn->dest_recode>mDestEnc->GetCount()) dsgn->dest_recode=0;
  mDestEnc->SetSelection(dsgn->dest_recode);
  update_file_name_suffix(mDestName, dsgn->dest_type);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_DEST_NAME_SEL
 */

void WizardPage1::OnCdTrDestNameSelClick( wxCommandEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  wxString filter = get_filter(mDestType->GetSelection());  // must not be only the parameter of the next function!
  wxString str = GetExportFileName(mDestName->GetValue(), filter, GetParent());
  if (!str.IsEmpty())
    mDestName->SetValue(str);
}


/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE
 */

void WizardPage::OnWizardpagePageChanged( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  if (dsgn->source_type==IETYPE_WB || dsgn->source_type==IETYPE_CURSOR)
  { mSourceOrigin->SetSelection(0);
    fill_source_types(0);
    mSourceType->SetSelection(dsgn->source_type==IETYPE_WB ? 0 : 1);
    fill_source_objects(dsgn);
  }
  else
  { mSourceOrigin->SetSelection(1);
    fill_source_types(1);
    mSourceType->SetSelection(dsgn->source_type==IETYPE_TDT ? 0 : dsgn->source_type==IETYPE_CSV ? 1 : dsgn->source_type==IETYPE_COLS ? 2 : dsgn->source_type==IETYPE_DBASE ? 3 : 4);
    mSourceName->Clear();
  }
 // propose the default file name for import into the database:
  if (dsgn->dest_type==IETYPE_WB && *dsgn->outpath && !*dsgn->inpath && dsgn->source_type!=IETYPE_WB)  // target is a database object, source is a file
  { if (!read_profile_string("Directories", "Import Path", dsgn->inpath, sizeof(dsgn->inpath)))
      strcpy(dsgn->inpath, wxGetCwd().mb_str(*wxConvCurrent));
    append(dsgn->inpath, "");
    const char * suff = dsgn->source_type==IETYPE_CSV ? ".csv" : dsgn->source_type==IETYPE_COLS ? ".txt" : ".dbf";
    char aName[64];
    strcat(dsgn->inpath, object_fname(dsgn->cdp, aName, dsgn->outpath, suff));
  }
  if (dsgn->source_type==IETYPE_WB || dsgn->source_type==IETYPE_CURSOR)
    mSourceName->SetValue(wxString(dsgn->inpath, *dsgn->cdp->sys_conv));  // object name
  else
    mSourceName->SetValue(wxString(dsgn->inpath, *wxConvCurrent));        // file name
  mCondition->SetValue(wxString(dsgn->cond, *dsgn->cdp->sys_conv));
  fill_encodings(mSourceEnc, mSrcEncC, dsgn->source_type);
  if (dsgn->src_recode>mSourceEnc->GetCount()) dsgn->src_recode=0;
  mSourceEnc->SetSelection(dsgn->src_recode);
  event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE
 */

void WizardPage::OnWizardpagePageChanging( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  if (mSourceOrigin->GetSelection()==0)
  { dsgn->source_type = mSourceType->GetSelection()==0 ? IETYPE_WB : IETYPE_CURSOR;
    strmaxcpy(dsgn->inpath, mSourceName->GetValue().mb_str(*dsgn->cdp->sys_conv), sizeof(tobjname)); // object name
  }
  else
  { dsgn->source_type = mSourceType->GetSelection()==0 ? IETYPE_TDT : mSourceType->GetSelection()==1 ? IETYPE_CSV : mSourceType->GetSelection()==2 ? IETYPE_COLS : mSourceType->GetSelection()==3 ? IETYPE_DBASE : IETYPE_FOX;
    strmaxcpy(dsgn->inpath, mSourceName->GetValue().mb_str(*wxConvCurrent), sizeof(dsgn->inpath));   // file name
  }
  strmaxcpy(dsgn->cond, mCondition->GetValue().mb_str(*dsgn->cdp->sys_conv), sizeof(dsgn->cond));
 // propose the default file name for export from the database:
  if (mSourceOrigin->GetSelection()==0 && *dsgn->inpath && !*dsgn->outpath && dsgn->dest_type!=IETYPE_WB)  // source is a database object, target is a file
  { if (!read_profile_string("Directories", "Export Path", dsgn->outpath, sizeof(dsgn->outpath)))
      strcpy(dsgn->outpath, wxGetCwd().mb_str(*wxConvCurrent));
    append(dsgn->outpath, "");
    const char * suff = dsgn->dest_type==IETYPE_CSV ? ".csv" : dsgn->dest_type==IETYPE_COLS ? ".txt" : ".dbf";
    char aName[64];
    strcat(dsgn->outpath, object_fname(dsgn->cdp, aName, dsgn->inpath, suff));
  }
  dsgn->src_recode = mSourceEnc->GetSelection();
  event.Skip();
}


/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE1
 */

void WizardPage1::OnWizardpage1PageChanged( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  if (dsgn->dest_type==IETYPE_WB)
  { mDestOrigin->SetSelection(0);
    fill_dest_types(0);
    mDestType->SetSelection(0);
    fill_dest_objects(dsgn);
    mIndexes->Enable();
    mDestName->SetValue(wxString(dsgn->outpath, *dsgn->cdp->sys_conv));  // object name
  }
  else
  { mDestOrigin->SetSelection(1);
    fill_dest_types(1);
    mDestType->SetSelection(dsgn->dest_type==IETYPE_TDT ? 0 : dsgn->dest_type==IETYPE_CSV ? 1 : dsgn->dest_type==IETYPE_COLS ? 2 : dsgn->dest_type==IETYPE_DBASE ? 3 : 4);
    mDestName->Clear();
    mIndexes->Disable();
    mDestName->SetValue(wxString(dsgn->outpath, *wxConvCurrent));       // file name
  }
  mIndexes->SetSelection(dsgn->index_disabled ? 0 : 1);
  fill_encodings(mDestEnc, mDestEncC, dsgn->dest_type);
  if (dsgn->dest_recode>mDestEnc->GetCount()) dsgn->dest_recode=0;
  mDestEnc->SetSelection(dsgn->dest_recode);
  event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE1
 */

void WizardPage1::OnWizardpage1PageChanging( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  if (mDestOrigin->GetSelection()==0)
  { dsgn->dest_type = IETYPE_WB;
    strmaxcpy(dsgn->outpath, mDestName->GetValue().mb_str(*dsgn->cdp->sys_conv), sizeof(tobjname));  // object name
  }
  else
  { dsgn->dest_type = mDestType->GetSelection()==0 ? IETYPE_TDT : mDestType->GetSelection()==1 ? IETYPE_CSV : mDestType->GetSelection()==2 ? IETYPE_COLS : mDestType->GetSelection()==3 ? IETYPE_DBASE : IETYPE_FOX;
    strmaxcpy(dsgn->outpath, mDestName->GetValue().mb_str(*wxConvCurrent), sizeof(dsgn->outpath));   // file name
  }
  dsgn->index_disabled = mIndexes->GetSelection()==0;
  dsgn->dest_recode = mDestEnc->GetSelection();
  event.Skip();
}


/*!
 * Runs the wizard.
 */

bool NewDataTransferDlg::Run()
{
    if (GetChildren().GetCount() > 0)
    {
        wxWizardPage* startPage = wxDynamicCast(GetChildren().GetFirst()->GetData(), wxWizardPage);
        if (startPage) return RunWizard(startPage);
    }
    return false;
}




/*!
 * WizardPage2 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPage2, wxWizardPageSimple )

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
    NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
    mMainSel->SetSelection(dlg->xml_transport ? 0 : 1);  // used when wizars started on the data source page and the Back pressed
}

/*!
 * Should we show tooltips?
 */

bool WizardPage2::ShowToolTips()
{
    return TRUE;
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE2
 */

void WizardPage2::OnWizardpage2PageChanging( wxWizardEvent& event )
{ NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
  dlg->xml_transport = mMainSel->GetSelection()==0;
  event.Skip();
}


/*!
 * WizardPage3 type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WizardPage3, wxWizardPageSimple )

/*!
 * WizardPage3 event table definition
 */

BEGIN_EVENT_TABLE( WizardPage3, wxWizardPage )

////@begin WizardPage3 event table entries
    EVT_WIZARD_PAGE_CHANGED( -1, WizardPage3::OnWizardpage3PageChanged )
    EVT_WIZARD_PAGE_CHANGING( -1, WizardPage3::OnWizardpage3PageChanging )

    EVT_RADIOBUTTON( CD_XMLSTART_TABLE, WizardPage3::OnCdXmlstartTableSelected )

    EVT_RADIOBUTTON( CD_XMLSTART_QUERY, WizardPage3::OnCdXmlstartQuerySelected )

    EVT_RADIOBUTTON( CD_XMLSTART_EMPTY, WizardPage3::OnCdXmlstartEmptySelected )

    EVT_RADIOBUTTON( CD_XMLSTART_SCHEMA, WizardPage3::OnCdXmlstartSchemaSelected )

    EVT_BUTTON( CD_XMLSTART_SELSCHEMA, WizardPage3::OnCdXmlstartSelschemaClick )

////@end WizardPage3 event table entries

END_EVENT_TABLE()

/*!
 * WizardPage3 constructors
 */

WizardPage3::WizardPage3( )
{ xml_init=0; 
}

WizardPage3::WizardPage3( wxWizard* parent )
{
    Create( parent );
}

/*!
 * WizardPage3 creator
 */

bool WizardPage3::Create( wxWizard* parent )
{
////@begin WizardPage3 member initialisation
    mByTable = NULL;
    mTableNameC = NULL;
    mTableName = NULL;
    mTableAliasC = NULL;
    mTableAlias = NULL;
    mByQuery = NULL;
    mQuery = NULL;
    mByEmpty = NULL;
    mBySchema = NULL;
    mSchema = NULL;
    mSelSchema = NULL;
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
    WizardPage3* itemWizardPage34 = this;

    wxBoxSizer* itemBoxSizer35 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage34->SetSizer(itemBoxSizer35);

    mByTable = new wxRadioButton;
    mByTable->Create( itemWizardPage34, CD_XMLSTART_TABLE, _("Start by mapping columns of the top-level table:"), wxDefaultPosition, wxDefaultSize, 0 );
    mByTable->SetValue(FALSE);
    itemBoxSizer35->Add(mByTable, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer35->Add(itemBoxSizer37, 0, wxGROW|wxALL, 0);

    itemBoxSizer37->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer39 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer39->AddGrowableCol(1);
    itemBoxSizer37->Add(itemFlexGridSizer39, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    mTableNameC = new wxStaticText;
    mTableNameC->Create( itemWizardPage34, wxID_STATIC, _("Table name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer39->Add(mTableNameC, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    wxString* mTableNameStrings = NULL;
    mTableName = new wxComboBox;
    mTableName->Create( itemWizardPage34, ID_COMBOBOX, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTableNameStrings, wxCB_DROPDOWN|wxCB_SORT );
    itemFlexGridSizer39->Add(mTableName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    mTableAliasC = new wxStaticText;
    mTableAliasC->Create( itemWizardPage34, wxID_STATIC, _("Alias:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer39->Add(mTableAliasC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mTableAlias = new wxTextCtrl;
    mTableAlias->Create( itemWizardPage34, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer39->Add(mTableAlias, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mByQuery = new wxRadioButton;
    mByQuery->Create( itemWizardPage34, CD_XMLSTART_QUERY, _("Start by mapping columns of the query:"), wxDefaultPosition, wxDefaultSize, 0 );
    mByQuery->SetValue(FALSE);
    itemBoxSizer35->Add(mByQuery, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer45 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer35->Add(itemBoxSizer45, 1, wxGROW|wxALL, 0);

    itemBoxSizer45->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mQuery = new wxTextCtrl;
    mQuery->Create( itemWizardPage34, ID_TEXTCTRL1, _T(""), wxDefaultPosition, wxSize(-1, 50), wxTE_MULTILINE );
    itemBoxSizer45->Add(mQuery, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mByEmpty = new wxRadioButton;
    mByEmpty->Create( itemWizardPage34, CD_XMLSTART_EMPTY, _("Start from an empty design"), wxDefaultPosition, wxDefaultSize, 0 );
    mByEmpty->SetValue(FALSE);
    itemBoxSizer35->Add(mByEmpty, 0, wxALIGN_LEFT|wxALL, 5);

    mBySchema = new wxRadioButton;
    mBySchema->Create( itemWizardPage34, CD_XMLSTART_SCHEMA, _("Start by creating a table and mapping for an XML schema:"), wxDefaultPosition, wxDefaultSize, 0 );
    mBySchema->SetValue(FALSE);
    itemBoxSizer35->Add(mBySchema, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer50 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer35->Add(itemBoxSizer50, 0, wxGROW|wxALL, 0);

    itemBoxSizer50->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSchema = new wxTextCtrl;
    mSchema->Create( itemWizardPage34, CD_XMLSTART_SCHEMA, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer50->Add(mSchema, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    mSelSchema = new wxButton;
    mSelSchema->Create( itemWizardPage34, CD_XMLSTART_SELSCHEMA, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemBoxSizer50->Add(mSelSchema, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxADJUST_MINSIZE, 5);

////@end WizardPage3 content construction
    t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
    mTableAlias->SetMaxLength(OBJ_NAME_LEN);
    fill_table_names(dsgn->cdp, mTableName, false, "");
    mByTable->SetValue(true);  xml_init=1;
    enabling(0);
#if WBVERS<950
    mBySchema->Disable();
#endif
#ifndef WINS
    mBySchema->Disable();
#endif
}

void WizardPage3::enabling(int sel)
{ bool bytable = sel==0;
  mTableNameC ->Enable(bytable);  mTableName ->Enable(bytable);
  mTableAliasC->Enable(bytable);  mTableAlias->Enable(bytable);
  mQuery->Enable(sel==1);
  mSchema->Enable(sel==3);  mSelSchema->Enable(sel==3);
}

/*!
 * Should we show tooltips?
 */

bool WizardPage3::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_TABLE
 */

void WizardPage3::OnCdXmlstartTableSelected( wxCommandEvent& event )
{ mByQuery->SetValue(false);  mByEmpty->SetValue(false);   mBySchema->SetValue(false);
  enabling(0);
  xml_init=1;  // this will be done in ...PageChanging, but NextPage is called before and it needs the xml_init! 
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_QUERY
 */

void WizardPage3::OnCdXmlstartQuerySelected( wxCommandEvent& event )
{ mByTable->SetValue(false);  mByEmpty->SetValue(false);  mBySchema->SetValue(false);
  enabling(1);
  xml_init=2;  // this will be done in ...PageChanging, but NextPage is called before and it needs the xml_init! 
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_EMPTY
 */

void WizardPage3::OnCdXmlstartEmptySelected( wxCommandEvent& event )
{ mByQuery->SetValue(false);  mByTable->SetValue(false);  mBySchema->SetValue(false);
  enabling(2);
  xml_init=3;  // this will be done in ...PageChanging, but NextPage is called before and it needs the xml_init! 
  event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_SCHEMA
 */

void WizardPage3::OnCdXmlstartSchemaSelected( wxCommandEvent& event )
{ mByQuery->SetValue(false);  mByTable->SetValue(false);   mByEmpty->SetValue(false);
  enabling(3);
  xml_init=4;  // this will be done in ...PageChanging, but NextPage is called before and it needs the xml_init! 
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
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE3
 */

void WizardPage3::OnWizardpage3PageChanged( wxWizardEvent& event )
{
    // Insert custom code here
    event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE3
 */

void WizardPage3::OnWizardpage3PageChanging( wxWizardEvent& event )
{ 
  t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
  if (mByTable->GetValue())
  { int ind = mTableName->GetSelection();
    if (ind==-1) 
      { event.Veto();  mTableName->SetFocus();  wxBell();  return; }
    strcpy(table_name, mTableName->GetString(ind).mb_str(*dsgn->cdp->sys_conv));
    wxString alias = mTableAlias->GetValue();
    alias.Trim(true);  alias.Trim(false);
    strcpy(table_alias, alias.mb_str(*dsgn->cdp->sys_conv));
    xml_init=1;
  }
  else if (mByQuery->GetValue())
  { query=mQuery->GetValue();
    new_td = get_expl_descriptor(dsgn->cdp, query.mb_str(*dsgn->cdp->sys_conv), NULL); 
    if (!new_td) 
      { event.Veto();  mQuery->SetFocus();  wxBell();  return; }
    xml_init=2;
  }
  else if (mBySchema->GetValue())
  { query=mSchema->GetValue();
    query.Trim(false);  query.Trim(true);
    if (query.IsEmpty())
      { event.Veto();  mSchema->SetFocus();  wxBell();  return; }
    xml_init=4;
  }
  else
    xml_init=3;
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
    WizardPage4* itemWizardPage54 = this;

    wxBoxSizer* itemBoxSizer55 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage54->SetSizer(itemBoxSizer55);

    mCreHeader = new wxCheckBox;
    mCreHeader->Create( itemWizardPage54, CD_TR_CRE_HEADER, _("Create header line in the target text file"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mCreHeader->SetValue(FALSE);
    itemBoxSizer55->Add(mCreHeader, 0, wxALIGN_LEFT|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer57 = new wxFlexGridSizer(5, 3, 0, 0);
    itemFlexGridSizer57->AddGrowableCol(1);
    itemBoxSizer55->Add(itemFlexGridSizer57, 0, wxGROW|wxALL, 0);

    mSkipHeaderC = new wxStaticText;
    mSkipHeaderC->Create( itemWizardPage54, CD_TR_SKIP_HEADER_C, _("Number of header lines in source file:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(mSkipHeaderC, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mSkipHeader = new wxSpinCtrl;
    mSkipHeader->Create( itemWizardPage54, CD_TR_SKIP_HEADER, _("0"), wxDefaultPosition, wxSize(-1, 20), wxSP_ARROW_KEYS, 0, 100, 0 );
    itemFlexGridSizer57->Add(mSkipHeader, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer57->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText61 = new wxStaticText;
    itemStaticText61->Create( itemWizardPage54, wxID_STATIC, _("CSV field separator:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText61, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mCsvSeparStrings = NULL;
    mCsvSepar = new wxComboBox;
    mCsvSepar->Create( itemWizardPage54, CD_TR_CSV_SEPAR, _T(""), wxDefaultPosition, wxDefaultSize, 0, mCsvSeparStrings, wxCB_DROPDOWN );
    itemFlexGridSizer57->Add(mCsvSepar, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer57->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText64 = new wxStaticText;
    itemStaticText64->Create( itemWizardPage54, wxID_STATIC, _("CSV value quote:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText64, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mCsvDelimStrings = NULL;
    mCsvDelim = new wxComboBox;
    mCsvDelim->Create( itemWizardPage54, CD_TR_CSV_DELIM, _T(""), wxDefaultPosition, wxDefaultSize, 0, mCsvDelimStrings, wxCB_DROPDOWN );
    itemFlexGridSizer57->Add(mCsvDelim, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer57->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText67 = new wxStaticText;
    itemStaticText67->Create( itemWizardPage54, wxID_STATIC, _("Date format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText67, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mDateFormatStrings = NULL;
    mDateFormat = new wxComboBox;
    mDateFormat->Create( itemWizardPage54, CD_TR_DATE_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDateFormatStrings, wxCB_DROPDOWN );
    itemFlexGridSizer57->Add(mDateFormat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer57->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText70 = new wxStaticText;
    itemStaticText70->Create( itemWizardPage54, wxID_STATIC, _("Time format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText70, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mTimeFormatStrings = NULL;
    mTimeFormat = new wxComboBox;
    mTimeFormat->Create( itemWizardPage54, CD_TR_TIME_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTimeFormatStrings, wxCB_DROPDOWN );
    itemFlexGridSizer57->Add(mTimeFormat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer57->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText73 = new wxStaticText;
    itemStaticText73->Create( itemWizardPage54, wxID_STATIC, _("Timestamp format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText73, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mTsFormatStrings = NULL;
    mTsFormat = new wxComboBox;
    mTsFormat->Create( itemWizardPage54, CD_TR_TS_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTsFormatStrings, wxCB_DROPDOWN );
    itemFlexGridSizer57->Add(mTsFormat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer57->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText76 = new wxStaticText;
    itemStaticText76->Create( itemWizardPage54, wxID_STATIC, _("Boolean encoding:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText76, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mBoolFormatStrings = NULL;
    mBoolFormat = new wxComboBox;
    mBoolFormat->Create( itemWizardPage54, CD_TR_BOOL_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mBoolFormatStrings, wxCB_DROPDOWN );
    itemFlexGridSizer57->Add(mBoolFormat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer57->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText79 = new wxStaticText;
    itemStaticText79->Create( itemWizardPage54, wxID_STATIC, _("Decimal separator:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText79, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* mDecSeparStrings = NULL;
    mDecSepar = new wxComboBox;
    mDecSepar->Create( itemWizardPage54, CD_TR_DEC_SEPAR, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDecSeparStrings, wxCB_DROPDOWN );
    itemFlexGridSizer57->Add(mDecSepar, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer57->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSemilog = new wxCheckBox;
    mSemilog->Create( itemWizardPage54, CD_TR_SEMILOG, _("Write real numbers in semi-logarithmic format (e.g. 5.3e2)"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mSemilog->SetValue(FALSE);
    itemBoxSizer55->Add(mSemilog, 0, wxALIGN_LEFT|wxALL, 5);

    mCRLines = new wxCheckBox;
    mCRLines->Create( itemWizardPage54, CD_TR_CR_LINES, _("Accept lines terminated by CR only"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mCRLines->SetValue(FALSE);
    itemBoxSizer55->Add(mCRLines, 0, wxALIGN_LEFT|wxALL, 5);

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
  t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
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
 * Should we show tooltips?
 */

bool WizardPage4::ShowToolTips()
{
    return TRUE;
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE4
 */

void WizardPage4::OnWizardpage4PageChanged( wxWizardEvent& event )
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
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
{ t_ie_run * dsgn = ((NewDataTransferDlg*)GetParent())->dsgn;
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



/************** page order **********************************/
wxWizardPage* WizardPage2::GetPrev() const   // this is the initial page
{ return NULL; }  

wxWizardPage* WizardPage2::GetNext() const
{ NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
  if (mMainSel->GetSelection()==0) return dlg->mPageXML;
  else return dlg->mPageDataSource;
}

wxWizardPage* WizardPage::GetPrev() const    // data source
{ NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
  return dlg->updating ? NULL : dlg->mPageMainSel;
}

wxWizardPage* WizardPage::GetNext() const
{ NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
  return dlg->mPageDataTarget;
}

wxWizardPage* WizardPage1::GetPrev() const   // data target
{ NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
  return dlg->mPageDataSource;
}

wxWizardPage* WizardPage1::GetNext() const
{ NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
  if (dlg->dsgn->text_source() || dlg->dsgn->text_target())
    return dlg->mPageText;
  return NULL;   // the last page if no format is text
}

wxWizardPage* WizardPage3::GetPrev() const   // XML
{ NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
  return dlg->mPageMainSel;
}

wxWizardPage* WizardPage3::GetNext() const
{ NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
  if (!xml_init || xml_init==4) return dlg->mPageXSD;  // xml_init==0 on start
  return NULL;
}

wxWizardPage* WizardPage4::GetPrev() const   // Text options
{ NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
  return dlg->mPageDataTarget;
}

wxWizardPage* WizardPage4::GetNext() const
{ return NULL; }  // the last page

wxWizardPage* WizardPageXSD::GetPrev() const
{ NewDataTransferDlg * dlg = (NewDataTransferDlg*)GetParent();
  return dlg->mPageXML;
}

wxWizardPage* WizardPageXSD::GetNext() const
{ return NULL; }  // the last page

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_DEST_NAME
 */

void WizardPage1::OnCdTrDestNameSelected( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
}


/*!
 * wxEVT_WIZARD_HELP event handler for ID_WIZARD
 */

void NewDataTransferDlg::OnWizardHelp( wxWizardEvent& event )
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



/*!
 * Get bitmap resources
 */

wxBitmap NewDataTransferDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NewDataTransferDlg bitmap retrieval
    if (name == wxT("trigwiz"))
    {
        wxBitmap bitmap(trigwiz_xpm);
        return bitmap;
    }
    return wxNullBitmap;
////@end NewDataTransferDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NewDataTransferDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NewDataTransferDlg icon retrieval
    return wxNullIcon;
////@end NewDataTransferDlg icon retrieval
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
    WizardPageXSD* itemWizardPage84 = this;

    wxBoxSizer* itemBoxSizer85 = new wxBoxSizer(wxVERTICAL);
    itemWizardPage84->SetSizer(itemBoxSizer85);

    wxFlexGridSizer* itemFlexGridSizer86 = new wxFlexGridSizer(3, 2, 0, 0);
    itemFlexGridSizer86->AddGrowableCol(1);
    itemBoxSizer85->Add(itemFlexGridSizer86, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText87 = new wxStaticText;
    itemStaticText87->Create( itemWizardPage84, wxID_STATIC, _("Table name prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer86->Add(itemStaticText87, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPrefix = new wxTextCtrl;
    mPrefix->Create( itemWizardPage84, CD_XMLXSD_PREFIX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        mPrefix->SetToolTip(_("Prefix the names of all tables created for the XML data by the wizard"));
    itemFlexGridSizer86->Add(mPrefix, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText89 = new wxStaticText;
    itemStaticText89->Create( itemWizardPage84, wxID_STATIC, _("Default string length:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer86->Add(itemStaticText89, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mLength = new wxSpinCtrl;
    mLength->Create( itemWizardPage84, CD_XMLXSD_LENGTH, _("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
    if (ShowToolTips())
        mLength->SetToolTip(_("Length of database columns for XML strings without a specified length in the schema"));
    itemFlexGridSizer86->Add(mLength, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer86->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mUnicode = new wxCheckBox;
    mUnicode->Create( itemWizardPage84, CD_XMLXSD_UNICODE, _("Use Unicode strings"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mUnicode->SetValue(FALSE);
    if (ShowToolTips())
        mUnicode->SetToolTip(_("Use Unicode string columns for storing string data from XML documents in the database"));
    itemFlexGridSizer86->Add(mUnicode, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText93 = new wxStaticText;
    itemStaticText93->Create( itemWizardPage84, wxID_STATIC, _("ID column type:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer86->Add(itemStaticText93, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mIDTypeStrings = NULL;
    mIDType = new wxChoice;
    mIDType->Create( itemWizardPage84, CD_XMLXSD_IDTYPE, wxDefaultPosition, wxDefaultSize, 0, mIDTypeStrings, 0 );
    if (ShowToolTips())
        mIDType->SetToolTip(_("Database type of the ID and UPPER_ID columns joining the related database tables containing XML data"));
    itemFlexGridSizer86->Add(mIDType, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end WizardPageXSD content construction
   // set restrictions:
    mPrefix->SetMaxLength(20);
    mLength->SetRange(2, MAX_FIXED_STRING_LENGTH);
    mIDType->Append(_("Integer (32-bit)"));
    mIDType->Append(_("BigInt (64-bit)"));
}

/*!
 * Gets the previous page.
 */

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
 * wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE_XSD
 */

void WizardPageXSD::OnWizardpageXsdPageChanged( wxWizardEvent& event )
{ t_xsd_gen_params * gen_params = ((NewDataTransferDlg*)GetParent())->gen_params;
  wxString fname = ((NewDataTransferDlg*)GetParent())->mPageXML->query;
  const wxChar * p = fname;
  int i=wcslen(p);
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
  event.Skip();
}

/*!
 * wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE_XSD
 */

void WizardPageXSD::OnWizardpageXsdPageChanging( wxWizardEvent& event )
{ t_xsd_gen_params * gen_params = ((NewDataTransferDlg*)GetParent())->gen_params;
  wxString pref;
  pref = mPrefix->GetValue();  pref.Trim(false);  pref.Trim(true);
  strcpy(gen_params->tabname_prefix, pref.mb_str(*wxConvCurrent));
  gen_params->id_type = mIDType->GetSelection() ? ATT_INT64 : ATT_INT32;
  gen_params->string_length=mLength->GetValue();
  gen_params->unicode_strings = mUnicode->GetValue();
  event.Skip();
}


