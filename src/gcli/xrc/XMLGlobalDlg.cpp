/////////////////////////////////////////////////////////////////////////////
// Name:        XMLGlobalDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/25/04 14:30:50
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "XMLGlobalDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "XMLGlobalDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * XMLGlobalDlg type definition
 */

IMPLEMENT_CLASS( XMLGlobalDlg, wxDialog )

/*!
 * XMLGlobalDlg event table definition
 */

BEGIN_EVENT_TABLE( XMLGlobalDlg, wxDialog )

////@begin XMLGlobalDlg event table entries
    EVT_BUTTON( CD_XMPTOP_SELENC, XMLGlobalDlg::OnCdXmptopSelencClick )

    EVT_BUTTON( wxID_OK, XMLGlobalDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, XMLGlobalDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, XMLGlobalDlg::OnHelpClick )

////@end XMLGlobalDlg event table entries
    EVT_MENU(-1, XMLGlobalDlg::OnEncoding)
END_EVENT_TABLE()

/*!
 * XMLGlobalDlg constructors
 */

XMLGlobalDlg::XMLGlobalDlg(xml_designer * dadeIn)
{ dade=dadeIn;
  dad_top=dade->dad_tree;
  cdp=dade->cdp;
}

/*!
 * XMLGlobalDlg creator
 */

bool XMLGlobalDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin XMLGlobalDlg member initialisation
    mQuery = NULL;
    mSpacer10_2 = NULL;
    mDataSource = NULL;
    mProlog = NULL;
    mDoctype = NULL;
    mStyleSheet = NULL;
    mEmitWhitespaces = NULL;
    mSpacer10 = NULL;
    mDisableOpt = NULL;
    mNoValidation = NULL;
    mIgnoreText = NULL;
    mIgnoreElements = NULL;
    mNmspPrefix = NULL;
    mNmspName = NULL;
    mDAD2 = NULL;
    mForm2 = NULL;
    mOKMessage = NULL;
    mFailMessage = NULL;
////@end XMLGlobalDlg member initialisation

////@begin XMLGlobalDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end XMLGlobalDlg creation
    return TRUE;
}

/*!
 * Control creation for XMLGlobalDlg
 */

void XMLGlobalDlg::CreateControls()
{    
////@begin XMLGlobalDlg content construction
    XMLGlobalDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxNotebook* itemNotebook3 = new wxNotebook;
    itemNotebook3->Create( itemDialog1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );
#if !wxCHECK_VERSION(2,5,2)
    wxNotebookSizer* itemNotebook3Sizer = new wxNotebookSizer(itemNotebook3);
#endif

    wxPanel* itemPanel4 = new wxPanel;
    itemPanel4->Create( itemNotebook3, ID_PANEL, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemPanel4->SetSizer(itemBoxSizer5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemPanel4, wxID_STATIC, _("Query for the base cursor (empty when data is associated with individual tables):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mQuery = new wxTextCtrl;
    mQuery->Create( itemPanel4, CD_XMPTOP_QUERY, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    itemBoxSizer5->Add(mQuery, 1, wxGROW|wxALL, 5);

    mSpacer10_2 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(mSpacer10_2, 0, wxGROW|wxALL, 0);
    wxStaticText* itemStaticText9 = new wxStaticText;
    itemStaticText9->Create( itemPanel4, wxID_STATIC, _("Data source:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSpacer10_2->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mDataSourceStrings = NULL;
    mDataSource = new wxOwnerDrawnComboBox;
    mDataSource->Create( itemPanel4, CD_XMLTOP_DATA_SOURCE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDataSourceStrings, wxCB_READONLY );
    mSpacer10_2->Add(mDataSource, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemNotebook3->AddPage(itemPanel4, _("Base cursor"));

    wxPanel* itemPanel11 = new wxPanel;
    itemPanel11->Create( itemNotebook3, ID_PANEL1, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxVERTICAL);
    itemPanel11->SetSizer(itemBoxSizer12);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer12->Add(itemBoxSizer13, 0, wxGROW|wxALL, 0);
    wxStaticText* itemStaticText14 = new wxStaticText;
    itemStaticText14->Create( itemPanel11, wxID_STATIC, _("Prologue <?xml ... ?>:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    itemBoxSizer13->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton16 = new wxButton;
    itemButton16->Create( itemPanel11, CD_XMPTOP_SELENC, _("Encoding"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton16, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    mProlog = new wxTextCtrl;
    mProlog->Create( itemPanel11, CD_XMPTOP_PROLOG, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(mProlog, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText18 = new wxStaticText;
    itemStaticText18->Create( itemPanel11, wxID_STATIC, _("DOCTYPE declaration <!DOCTYPE ... >:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemStaticText18, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mDoctype = new wxTextCtrl;
    mDoctype->Create( itemPanel11, CD_XMPTOP_DOCTYPE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(mDoctype, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText20 = new wxStaticText;
    itemStaticText20->Create( itemPanel11, CD_XMPTOP_SS_C, _("Stylesheet <?xml-stylesheet ... ?>:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemStaticText20, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mStyleSheetStrings = NULL;
    mStyleSheet = new wxOwnerDrawnComboBox;
    mStyleSheet->Create( itemPanel11, CD_XMPTOP_STYLESHEET, _T(""), wxDefaultPosition, wxDefaultSize, 0, mStyleSheetStrings, wxCB_DROPDOWN|wxCB_SORT );
    itemBoxSizer12->Add(mStyleSheet, 0, wxGROW|wxALL, 5);

    mEmitWhitespaces = new wxCheckBox;
    mEmitWhitespaces->Create( itemPanel11, CD_XMLTOP_EMIT_WHITESPACES, _("Include formatting whitespace"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mEmitWhitespaces->SetValue(FALSE);
    if (ShowToolTips())
        mEmitWhitespaces->SetToolTip(_("The XML output will be more readable when checked"));
    itemBoxSizer12->Add(mEmitWhitespaces, 0, wxGROW|wxALL, 5);

    mSpacer10 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer12->Add(mSpacer10, 0, wxGROW|wxALL, 0);
    mDisableOpt = new wxCheckBox;
    mDisableOpt->Create( itemPanel11, CD_XMLTOP_DISABLE_OPT, _("Disable cursor optimization"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mDisableOpt->SetValue(FALSE);
    if (ShowToolTips())
        mDisableOpt->SetToolTip(_("Prevents joining tables when exporting data. Usefull rarely in ODBC data source"));
    mSpacer10->Add(mDisableOpt, 0, wxGROW|wxALL, 5);

    itemNotebook3->AddPage(itemPanel11, _("Export to XML"));

    wxPanel* itemPanel25 = new wxPanel;
    itemPanel25->Create( itemNotebook3, ID_PANEL2, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxVERTICAL);
    itemPanel25->SetSizer(itemBoxSizer26);

    wxStaticText* itemStaticText27 = new wxStaticText;
    itemStaticText27->Create( itemPanel25, wxID_STATIC, _("Properties of XML import:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemStaticText27, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    mNoValidation = new wxCheckBox;
    mNoValidation->Create( itemPanel25, CD_TOPXML_NOVALID, _("No validation"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mNoValidation->SetValue(FALSE);
    itemBoxSizer26->Add(mNoValidation, 0, wxALIGN_LEFT|wxALL, 5);

    mIgnoreText = new wxCheckBox;
    mIgnoreText->Create( itemPanel25, CD_XMLTOP_IGNORETEXT, _("Ignore unassociated XML text"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mIgnoreText->SetValue(FALSE);
    itemBoxSizer26->Add(mIgnoreText, 0, wxALIGN_LEFT|wxALL, 5);

    mIgnoreElements = new wxCheckBox;
    mIgnoreElements->Create( itemPanel25, CD_XMPTOP_IGNOREELEMS, _("Ignore unknown elements"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mIgnoreElements->SetValue(FALSE);
    itemBoxSizer26->Add(mIgnoreElements, 0, wxALIGN_LEFT|wxALL, 5);

    itemNotebook3->AddPage(itemPanel25, _("Import from XML"));

    wxPanel* itemPanel31 = new wxPanel;
    itemPanel31->Create( itemNotebook3, ID_PANEL4, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer32 = new wxBoxSizer(wxVERTICAL);
    itemPanel31->SetSizer(itemBoxSizer32);

    wxStaticText* itemStaticText33 = new wxStaticText;
    itemStaticText33->Create( itemPanel31, wxID_STATIC, _("Prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemStaticText33, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mNmspPrefix = new wxTextCtrl;
    mNmspPrefix->Create( itemPanel31, CD_NMSP_PREFIX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(mNmspPrefix, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText35 = new wxStaticText;
    itemStaticText35->Create( itemPanel31, wxID_STATIC, _("Namepace:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemStaticText35, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mNmspName = new wxTextCtrl;
    mNmspName->Create( itemPanel31, CD_NMSP_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(mNmspName, 0, wxGROW|wxALL, 5);

    itemNotebook3->AddPage(itemPanel31, _("Root namespace"));

    wxPanel* itemPanel37 = new wxPanel;
    itemPanel37->Create( itemNotebook3, ID_PANEL5, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer38 = new wxBoxSizer(wxVERTICAL);
    itemPanel37->SetSizer(itemBoxSizer38);

    wxFlexGridSizer* itemFlexGridSizer39 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer39->AddGrowableCol(1);
    itemBoxSizer38->Add(itemFlexGridSizer39, 0, wxGROW|wxALL, 0);
    wxStaticText* itemStaticText40 = new wxStaticText;
    itemStaticText40->Create( itemPanel37, wxID_STATIC, _("After import, return the XML document exported by:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer39->Add(itemStaticText40, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mDAD2Strings = NULL;
    mDAD2 = new wxOwnerDrawnComboBox;
    mDAD2->Create( itemPanel37, CD_XMLTOP_DAD2, _T(""), wxDefaultPosition, wxDefaultSize, 0, mDAD2Strings, wxCB_DROPDOWN|wxCB_SORT );
    itemFlexGridSizer39->Add(mDAD2, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    wxStaticText* itemStaticText42 = new wxStaticText;
    itemStaticText42->Create( itemPanel37, wxID_STATIC, _("Insert the exported document into the form:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer39->Add(itemStaticText42, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mForm2Strings = NULL;
    mForm2 = new wxOwnerDrawnComboBox;
    mForm2->Create( itemPanel37, CD_XMLTOP_FORM2, _T(""), wxDefaultPosition, wxDefaultSize, 0, mForm2Strings, wxCB_DROPDOWN|wxCB_SORT );
    itemFlexGridSizer39->Add(mForm2, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText44 = new wxStaticText;
    itemStaticText44->Create( itemPanel37, wxID_STATIC, _("Return the HTML text (ignored when an export is specified above):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer38->Add(itemStaticText44, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mOKMessage = new wxTextCtrl;
    mOKMessage->Create( itemPanel37, CD_XMLTOP_OK_MESSAGE, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    itemBoxSizer38->Add(mOKMessage, 1, wxGROW|wxALL, 5);

    itemNotebook3->AddPage(itemPanel37, _("Import result"));

    wxPanel* itemPanel46 = new wxPanel;
    itemPanel46->Create( itemNotebook3, ID_PANEL6, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer47 = new wxBoxSizer(wxVERTICAL);
    itemPanel46->SetSizer(itemBoxSizer47);

    wxStaticText* itemStaticText48 = new wxStaticText;
    itemStaticText48->Create( itemPanel46, wxID_STATIC, _("When the XML import fails, return the HTML text:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer47->Add(itemStaticText48, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mFailMessage = new wxTextCtrl;
    mFailMessage->Create( itemPanel46, CD_XMLTOP_FAIL_MESSAGE, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
    itemBoxSizer47->Add(mFailMessage, 1, wxGROW|wxALL, 5);

    itemNotebook3->AddPage(itemPanel46, _("Import failure message"));

#if !wxCHECK_VERSION(2,5,2)
    itemBoxSizer2->Add(itemNotebook3Sizer, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);
#else
    itemBoxSizer2->Add(itemNotebook3, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);
#endif

    wxBoxSizer* itemBoxSizer50 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer50, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton51 = new wxButton;
    itemButton51->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton51->SetDefault();
    itemBoxSizer50->Add(itemButton51, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton52 = new wxButton;
    itemButton52->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer50->Add(itemButton52, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton53 = new wxButton;
    itemButton53->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer50->Add(itemButton53, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end XMLGlobalDlg content construction
 // query:
  mQuery     ->SetValue(dad_top->sql_stmt   ? wxString(dad_top->sql_stmt, *cdp->sys_conv)   : wxGetEmptyString());
 // export:
  // fill stored style sheets:
  mStyleSheet->Append(wxEmptyString);
  mStyleSheet->Append(wxT("href=\"../stylesheet/*\" type=\"text/xsl\""));
  void * en = get_object_enumerator(cdp, CATEG_STSH, NULL);
  if (en)
  { tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
    while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
      if (!(flags & CO_FLAGS_OBJECT_TYPE))
        mStyleSheet->Append(wxT("href=\"../stylesheet/") + wxString(name, *cdp->sys_conv) + wxT("\" type=\"text/xsl\""));
    free_object_enumerator(en);
  }
 // fill DADs:
  en = get_object_enumerator(cdp, CATEG_PGMSRC, NULL);
  if (en)
  { tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
    while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
      if ((flags & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_XML)
        mDAD2->Append(wxString(name, *cdp->sys_conv));
    free_object_enumerator(en);
  }
 // fill forms:
  en = get_object_enumerator(cdp, CATEG_XMLFORM, NULL);
  if (en)
  { tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
    while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
      mForm2->Append(wxString(name, *cdp->sys_conv));
    free_object_enumerator(en);
  }

  mProlog    ->SetValue(dad_top->prolog     ? wxString(dad_top->prolog, *cdp->sys_conv)     : wxGetEmptyString());
  mDoctype   ->SetValue(dad_top->doctype    ? wxString(dad_top->doctype, *cdp->sys_conv)    : wxGetEmptyString());
  mStyleSheet->SetValue(dad_top->stylesheet ? wxString(dad_top->stylesheet, *cdp->sys_conv) : wxGetEmptyString());
  mEmitWhitespaces->SetValue(dad_top->emit_whitespaces);
 // import:
  mIgnoreElements->SetValue(dad_top->ignore_elems);
  mIgnoreText    ->SetValue(dad_top->ignore_text);
  mNoValidation  ->SetValue(dad_top->no_validate);
#if WBVERS>=1000
  mDisableOpt    ->SetValue(dad_top->disable_cursor_opt);
 // data source:
  mDataSource->Append(wxEmptyString);
  for (t_avail_server * server = available_servers;  server;  server = server->next)
    if (server->odbc ? server->conn!=NULL : 
         server->cdp!=NULL && (!server->notreg || !stricmp(server->locid_server_name, dade->cdp->locid_server_name)))
      mDataSource->Append(wxString(server->locid_server_name, *wxConvCurrent));
  int ind = mDataSource->FindString(wxString(dad_top->top_dsn, *wxConvCurrent));
  if (ind!=wxNOT_FOUND) // SetStringSelection displays Assert when not found
    mDataSource->SetSelection(ind);
  else  // selects empty when not found
    mDataSource->SetSelection(0);
#else
  mSpacer10->Hide(mDisableOpt);
  itemBoxSizer5->Hide(mSpacer10_2);
#endif
 // namespace:
  mNmspPrefix->SetValue(dad_top->nmsp_prefix ? wxString(dad_top->nmsp_prefix, *cdp->sys_conv) : wxGetEmptyString());
  mNmspName  ->SetValue(dad_top->nmsp_name   ? wxString(dad_top->nmsp_name, *cdp->sys_conv)   : wxGetEmptyString());
 // responses
  mOKMessage  ->SetValue(dad_top->ok_response   ? wxString(dad_top->ok_response,   *cdp->sys_conv) : wxGetEmptyString());
  mFailMessage->SetValue(dad_top->fail_response ? wxString(dad_top->fail_response, *cdp->sys_conv) : wxGetEmptyString());
  mDAD2       ->SetValue(dad_top->dad2          ? wxString(dad_top->dad2,          *cdp->sys_conv) : wxGetEmptyString());
  mForm2      ->SetValue(dad_top->form2         ? wxString(dad_top->form2        , *cdp->sys_conv) : wxGetEmptyString());
}

/*!
 * Should we show tooltips?
 */

bool XMLGlobalDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

#define CPM_ENCODING 20000

void XMLGlobalDlg::OnEncoding(wxCommandEvent & event)
{ if (event.GetId() < CPM_ENCODING+1 || event.GetId() > CPM_ENCODING+10) 
    { event.Skip();  return; }
  const char * new_name = link_get_encoding_name(event.GetId()-CPM_ENCODING-1);
  wxString prolog = mProlog->GetValue(), new_prolog;
  prolog.UpperCase();
  int fnd = prolog.Find(wxT("ENCODING"));
  prolog = mProlog->GetValue();  // undoes the Upcase()
  if (fnd!=-1)
  { const wxChar * start, * stop;
    start=prolog.c_str()+fnd+8;
    stop=start;
    while (*stop==' ') stop++;
    if (*stop=='=')
    { stop++;
      while (*stop==' ') stop++;
      if (*stop=='\"' || *stop=='\'')
      { stop++;
        while (*stop && *stop!='\"' && *stop!='\'') stop++;
        if (*stop) stop++;
      }
    }
    new_prolog.Printf(wxT("%s=\"%s\"%s"), prolog.Left(fnd+8).c_str(), wxString(new_name, *wxConvCurrent).c_str(), prolog.Right(prolog.Length()-(stop-prolog.c_str())).c_str());
  }
  else
    new_prolog.Printf(wxT("%s encoding=\"%s\""), prolog.c_str(), wxString(new_name, *wxConvCurrent).c_str());
  mProlog->SetValue(new_prolog);
}

void XMLGlobalDlg::OnCdXmptopSelencClick( wxCommandEvent& event )
{ wxMenu * popup_menu = new wxMenu;
  popup_menu->Append(CPM_ENCODING+1,  _("UTF-8  (XML standard, character has one or more 8-bit bytes)"));
  popup_menu->Append(CPM_ENCODING+2,  _("UTF-7  (Character has 1 or more 7-bit codes)"));
  popup_menu->Append(CPM_ENCODING+3,  _("UTF-16  (16-bit character codes, extension of Windows Unicode)"));
  popup_menu->Append(CPM_ENCODING+4,  _("ISO-646  (Pure ASCII)"));
  popup_menu->Append(CPM_ENCODING+7,  _("windows-1250  (Central European Windows code page)"));
  popup_menu->Append(CPM_ENCODING+8,  _("windows-1252  (Western European Windows code page)"));
  popup_menu->Append(CPM_ENCODING+9,  _("ISO-8859-2  (ISO Latin 2)"));
  popup_menu->Append(CPM_ENCODING+10, _("ISO-8859-1  (ISO Latin 1)"));
  popup_menu->Append(CPM_ENCODING+5,  _("ibm852  (DOS Latin 2)"));
  PopupMenu(popup_menu, ScreenToClient(wxGetMousePosition()));
  delete popup_menu;
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void XMLGlobalDlg::OnOkClick( wxCommandEvent& event )
{ d_table * new_td;  t_ucd_cl * new_ucdp;  tcursnum new_top_curs;  wxString dsn;
#if WBVERS>=1000
 // dsn:
  int ind=mDataSource->GetSelection();
  if (ind!=-1)
    dsn=mDataSource->GetString(ind);
#endif
 // query:
  wxString q = mQuery->GetValue();
  q.Trim(false);  q.Trim(true);
  if (!q.IsEmpty())
  { new_td = get_expl_descriptor(dade->cdp, q.mb_str(*cdp->sys_conv), dsn.mb_str(*wxConvCurrent), dade, &new_ucdp, &new_top_curs); 
    if (!new_td) return;  // error reported
  }
  else 
  { new_td=NULL;  new_ucdp=NULL;  new_top_curs=NOOBJECT; }
 // replace [td], [top_ucdp], [sql_stmt] by the new values:
  if (dade->td) dade->top_ucdp->release_table_d(dade->td);  dade->td=new_td;  // must use the original [top_ucdp]
  if (dade->top_curs!=NOOBJECT) dade->top_ucdp->close_cursor(dade->top_curs);  dade->top_curs=new_top_curs;
  delete dade->top_ucdp;  dade->top_ucdp=new_ucdp;
  if (dad_top->sql_stmt)   link_delete_chars(dad_top->sql_stmt);  
  dad_top->sql_stmt  =GetStringAlloc(cdp, mQuery->GetValue());
  dade->dad_tree->explicit_sql_statement = dad_top->sql_stmt!=NULL;
#if WBVERS>=1000
  strcpy(dad_top->top_dsn, dsn.mb_str(*wxConvCurrent));
#endif
 // export:
  if (dad_top->prolog)     link_delete_chars(dad_top->prolog);  
  dad_top->prolog    =GetStringAlloc(cdp, mProlog->GetValue());
  if (dad_top->doctype)    link_delete_chars(dad_top->doctype);  
  dad_top->doctype   =GetStringAlloc(cdp, mDoctype->GetValue());
  if (dad_top->stylesheet) link_delete_chars(dad_top->stylesheet);  
  dad_top->stylesheet=GetStringAlloc(cdp, mStyleSheet->GetValue());
  dad_top->emit_whitespaces=mEmitWhitespaces->GetValue();
#if WBVERS>=1000
  dad_top->disable_cursor_opt=mDisableOpt->GetValue();
#endif
 // import:
  dad_top->ignore_elems=mIgnoreElements->GetValue();
  dad_top->ignore_text =mIgnoreText->GetValue();
  dad_top->no_validate =mNoValidation->GetValue();
 // namespace:
  if (dad_top->nmsp_prefix) link_delete_chars(dad_top->nmsp_prefix);  
  dad_top->nmsp_prefix = GetStringAlloc(cdp, mNmspPrefix->GetValue());
  if (dad_top->nmsp_name)   link_delete_chars(dad_top->nmsp_name);  
  dad_top->nmsp_name   = GetStringAlloc(cdp, mNmspName->GetValue());
 // responses:
  if (dad_top->ok_response)   link_delete_chars(dad_top->ok_response);  
  dad_top->ok_response   = GetStringAlloc(cdp, mOKMessage->GetValue());
  strmaxcpy(dad_top->dad2,  mDAD2->GetValue().mb_str(*wxConvCurrent),  sizeof(dad_top->dad2 ));
  strmaxcpy(dad_top->form2, mForm2->GetValue().mb_str(*wxConvCurrent), sizeof(dad_top->form2));
  if (dad_top->fail_response) link_delete_chars(dad_top->fail_response);  
  dad_top->fail_response = GetStringAlloc(cdp, mFailMessage->GetValue());
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void XMLGlobalDlg::OnCancelClick( wxCommandEvent& event )
{ event.Skip(); }

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void XMLGlobalDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/dadtop.html"));
}



/*!
 * Get bitmap resources
 */

wxBitmap XMLGlobalDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin XMLGlobalDlg bitmap retrieval
    return wxNullBitmap;
////@end XMLGlobalDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon XMLGlobalDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin XMLGlobalDlg icon retrieval
    return wxNullIcon;
////@end XMLGlobalDlg icon retrieval
}
