/////////////////////////////////////////////////////////////////////////////
// Name:        SearchObjectsDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/15/04 10:05:47
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "SearchObjectsDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "SearchObjectsDlg.h"
#include "fsed9.h"

////@begin XPM images
////@end XPM images

/*!
 * SearchObjectsDlg type definition
 */

IMPLEMENT_CLASS( SearchObjectsDlg, wxDialog )

/*!
 * SearchObjectsDlg event table definition
 */

BEGIN_EVENT_TABLE( SearchObjectsDlg, wxDialog )

////@begin SearchObjectsDlg event table entries
    EVT_BUTTON( wxID_OK, SearchObjectsDlg::OnOkClick )

    EVT_BUTTON( CD_SRCH_DETAIL, SearchObjectsDlg::OnCdSrchDetailClick )

    EVT_RADIOBUTTON( CD_SRCH_ALL, SearchObjectsDlg::OnCdSrchAllSelected )

    EVT_RADIOBUTTON( CD_SRCH_SELECTED, SearchObjectsDlg::OnCdSrchSelectedSelected )

    EVT_BUTTON( CD_SRCH_OPENSELECTOR, SearchObjectsDlg::OnCdSrchOpenselectorClick )

////@end SearchObjectsDlg event table entries

END_EVENT_TABLE()

// static members:
bool SearchObjectsDlg::selected_categs=false, SearchObjectsDlg::sel_table=false, SearchObjectsDlg::sel_query=false, 
     SearchObjectsDlg::sel_program=false, SearchObjectsDlg::sel_trigger=false, SearchObjectsDlg::sel_proc=false, 
     SearchObjectsDlg::sel_domain=false, SearchObjectsDlg::sel_seq=false, SearchObjectsDlg::sel_diagr=false;
bool SearchObjectsDlg::matching_case=false, SearchObjectsDlg::matching_word=false;
string_history SearchObjectsDlg::object_search_history(400);
uns32 SearchObjectsDlg::modif_limit = NONEINTEGER;  // NONEINTEGER means no limit and empty control

/*!
 * SearchObjectsDlg constructors
 */

SearchObjectsDlg::SearchObjectsDlg(cdp_t cdpIn, wxString pattIn, const char * schema_nameIn, const char * folder_nameIn)
{ cdp=cdpIn;  patt=pattIn;  
  in_schema = schema_nameIn && *schema_nameIn;
  if (in_schema)  
    strmaxcpy(search_schema, schema_nameIn, sizeof(search_schema));
  in_folder = folder_nameIn!=NULL; 
  if (in_folder)
    strmaxcpy(folder_name, folder_nameIn, sizeof(folder_name));
}

/*!
 * SearchObjectsDlg creator
 */

bool SearchObjectsDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SearchObjectsDlg member initialisation
    srch_pattern = NULL;
    match_word = NULL;
    match_case = NULL;
    mDetails = NULL;
    static1 = NULL;
    all_categs = NULL;
    sel_categs = NULL;
    tables = NULL;
    queries = NULL;
    programs = NULL;
    diagrams = NULL;
    procedures = NULL;
    triggers = NULL;
    sequences = NULL;
    domains = NULL;
    static2 = NULL;
    srch_time = NULL;
    mDateSelector = NULL;
////@end SearchObjectsDlg member initialisation

////@begin SearchObjectsDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end SearchObjectsDlg creation
    return TRUE;
}

/*!
 * Control creation for SearchObjectsDlg
 */

#include "bmps/openselector.xpm"

void timestamp2str_default(uns32 ts, char * buf)
{ 
#if 0
  if (ts==NONEINTEGER) *buf=0;
  else
  { f_date2str(timestamp2date(ts), buf, format_date.mb_str(*wxConvCurrent));
    if (timestamp2time(ts))
    { strcat(buf, format_dtsepar.mb_str(*wxConvCurrent));
      f_time2str(timestamp2time(ts), buf+strlen(buf), format_time.mb_str(*wxConvCurrent));
    }
  }
#endif
  superconv(ATT_TIMESTAMP, ATT_STRING, &ts, buf, t_specif(), t_specif(), format_timestamp.mb_str(*wxConvCurrent));
}

bool str2timestamp_default(const char * tx, uns32 * val)
{ 
#if 0
  uns32 dt, tm;
 // convert date:
  int off=f_str2date(tx, &dt, format_date.mb_str(*wxConvCurrent));
 // skip spaces and date-time separator:
  int sep = 0;
  while (tx[off]==' ') off++;
  while (tx[off] && tx[off]==format_dtsepar.GetChar(sep)) { off++;  sep++; }
  while (tx[off]==' ') off++;
 // convert the time (set to 0 if not present):
  int off2 = f_str2time(tx+off, &tm, format_time.mb_str(*wxConvCurrent));
  if (!off2) tm=0;
  off+=off2;
  while (tx[off]==' ') off++;
  if (tx[off] != 0) return false;
  *val=datetime2timestamp(dt, tm);
  return true;
#endif
  return 0==superconv(ATT_STRING, ATT_TIMESTAMP, tx, (char*)val, t_specif(), t_specif(), format_timestamp.mb_str(*wxConvCurrent));
}

void SearchObjectsDlg::CreateControls()
{    
////@begin SearchObjectsDlg content construction
    SearchObjectsDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer4, 1, wxALIGN_TOP|wxALL, 0);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer5, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemDialog1, CD_SRCH_CAPT, _("Fi&nd what:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* srch_patternStrings = NULL;
    srch_pattern = new wxOwnerDrawnComboBox;
    srch_pattern->Create( itemDialog1, CD_SRCH_PATTERN, _T(""), wxDefaultPosition, wxSize(180, -1), 0, srch_patternStrings, wxCB_DROPDOWN );
    itemBoxSizer5->Add(srch_pattern, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    match_word = new wxCheckBox;
    match_word->Create( itemDialog1, CD_SRCH_WORDS, _("Match whole word only"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    match_word->SetValue(FALSE);
    itemBoxSizer4->Add(match_word, 0, wxALIGN_LEFT|wxALL, 5);

    match_case = new wxCheckBox;
    match_case->Create( itemDialog1, CD_SRCH_SENSIT, _("Match case"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    match_case->SetValue(FALSE);
    itemBoxSizer4->Add(match_case, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer10, 0, wxALIGN_TOP|wxALL, 0);

    wxButton* itemButton11 = new wxButton;
    itemButton11->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton11->SetDefault();
    itemBoxSizer10->Add(itemButton11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    mDetails = new wxButton;
    mDetails->Create( itemDialog1, CD_SRCH_DETAIL, _("Details"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(mDetails, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    static1 = new wxStaticBox(itemDialog1, wxID_ANY, _("Select category"));
    wxStaticBoxSizer* itemStaticBoxSizer14 = new wxStaticBoxSizer(static1, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer14, 0, wxGROW|wxALL, 5);

    all_categs = new wxRadioButton;
    all_categs->Create( itemDialog1, CD_SRCH_ALL, _("Search all text objects"), wxDefaultPosition, wxDefaultSize, 0 );
    all_categs->SetValue(FALSE);
    itemStaticBoxSizer14->Add(all_categs, 0, wxALIGN_LEFT|wxALL, 5);

    sel_categs = new wxRadioButton;
    sel_categs->Create( itemDialog1, CD_SRCH_SELECTED, _("Search only the selected categories:"), wxDefaultPosition, wxDefaultSize, 0 );
    sel_categs->SetValue(FALSE);
    itemStaticBoxSizer14->Add(sel_categs, 0, wxALIGN_LEFT|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer17 = new wxFlexGridSizer(3, 4, 0, 0);
    itemStaticBoxSizer14->Add(itemFlexGridSizer17, 0, wxGROW|wxALL, 0);

    itemFlexGridSizer17->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    tables = new wxCheckBox;
    tables->Create( itemDialog1, CD_SRCH_TABLE, _("Tables"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    tables->SetValue(FALSE);
    itemFlexGridSizer17->Add(tables, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    queries = new wxCheckBox;
    queries->Create( itemDialog1, CD_SRCH_QUERY, _("Queries"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    queries->SetValue(FALSE);
    itemFlexGridSizer17->Add(queries, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    programs = new wxCheckBox;
    programs->Create( itemDialog1, CD_SRCH_PROG, _("Transports"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    programs->SetValue(FALSE);
    itemFlexGridSizer17->Add(programs, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer17->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    diagrams = new wxCheckBox;
    diagrams->Create( itemDialog1, CD_SRCG_DIAGR, _("Diagrams"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    diagrams->SetValue(FALSE);
    itemFlexGridSizer17->Add(diagrams, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    procedures = new wxCheckBox;
    procedures->Create( itemDialog1, CD_SRCH_PROC, _("Procedures"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    procedures->SetValue(FALSE);
    itemFlexGridSizer17->Add(procedures, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    triggers = new wxCheckBox;
    triggers->Create( itemDialog1, CD_SRCH_TRIGGER, _("Triggers"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    triggers->SetValue(FALSE);
    itemFlexGridSizer17->Add(triggers, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer17->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    sequences = new wxCheckBox;
    sequences->Create( itemDialog1, CD_SRCH_SEQ, _("Sequences"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    sequences->SetValue(FALSE);
    itemFlexGridSizer17->Add(sequences, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    domains = new wxCheckBox;
    domains->Create( itemDialog1, CD_SRCH_DOMAIN, _("Domains"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    domains->SetValue(FALSE);
    itemFlexGridSizer17->Add(domains, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    static2 = new wxStaticBox(itemDialog1, wxID_ANY, _("Select modified date"));
    wxStaticBoxSizer* itemStaticBoxSizer29 = new wxStaticBoxSizer(static2, wxHORIZONTAL);
    itemBoxSizer2->Add(itemStaticBoxSizer29, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText30 = new wxStaticText;
    itemStaticText30->Create( itemDialog1, wxID_STATIC, _("Search only objects modified after:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer29->Add(itemStaticText30, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    srch_time = new wxTextCtrl;
    srch_time->Create( itemDialog1, CD_SRCH_TIME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer29->Add(srch_time, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBitmap mDateSelectorBitmap(wxNullBitmap);
    mDateSelector = new wxBitmapButton;
    mDateSelector->Create( itemDialog1, CD_SRCH_OPENSELECTOR, mDateSelectorBitmap, wxDefaultPosition, wxSize(23, 23), wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemStaticBoxSizer29->Add(mDateSelector, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5);

////@end SearchObjectsDlg content construction
 // set the bitmap:
  wxBitmap bmp(openselector_xpm);
  mDateSelector->SetBitmapLabel(bmp);
 // store some handles in members:
  statsize1=itemStaticBoxSizer14;
  statsize2=itemStaticBoxSizer29;
 // show details iff selected categories specified or modif_limit specified
  if (selected_categs || modif_limit!=NONEINTEGER)
  { sel_categs->SetValue(true);
    mDetails->Disable();
  }
  else
  { all_categs->SetValue(true);
    show_details(false);
  } 
 // set recent values:
  match_word->SetValue(matching_word);
  match_case->SetValue(matching_case);
  enable_categs(selected_categs);
  tables->SetValue(sel_table);
  queries->SetValue(sel_query);
  domains->SetValue(sel_domain);
  procedures->SetValue(sel_proc);
  triggers->SetValue(sel_trigger);
  sequences->SetValue(sel_seq);
  programs->SetValue(sel_program);
  diagrams->SetValue(sel_diagr);
  object_search_history.fill(srch_pattern);
  char buf[50];
  timestamp2str_default(modif_limit, buf);
  srch_time->SetValue(wxString(buf, wxConvLocal));
 // set the input value:
  srch_pattern->SetValue(patt);  
}

void SearchObjectsDlg::enable_categs(bool enable)
{ 
  tables->Enable(enable);
  queries->Enable(enable);
  domains->Enable(enable);
  procedures->Enable(enable);
  triggers->Enable(enable);
  sequences->Enable(enable);
  programs->Enable(enable);
  diagrams->Enable(enable);
}

void SearchObjectsDlg::show_details(bool show)
{ static1->Show(show);
  GetSizer()->Show(statsize1, show);
  static2->Show(show);
  GetSizer()->Show(statsize2, show);
  GetSizer()->Layout();
}

/*!
 * Should we show tooltips?
 */

bool SearchObjectsDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_SRCH_ALL
 */

void SearchObjectsDlg::OnCdSrchAllSelected( wxCommandEvent& event )
{ selected_categs=false;
  enable_categs(selected_categs);
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_SRCH_SELECTED
 */

void SearchObjectsDlg::OnCdSrchSelectedSelected( wxCommandEvent& event )
{ selected_categs=true;
  enable_categs(selected_categs);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SRCH_DETAIL
 */

void SearchObjectsDlg::OnCdSrchDetailClick( wxCommandEvent& event )
{ show_details(true); 
  GetSizer()->Fit(this);
  mDetails->Disable();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void SearchObjectsDlg::OnOkClick( wxCommandEvent& event )
{// get the options from the dialog to member variables and validate their values:
  wxString str = srch_pattern->GetValue();
  str.Trim(false);  str.Trim(true);
  if (str.IsEmpty())
    { wxBell();  return; }
  if (!str2timestamp_default(srch_time->GetValue().mb_str(wxConvLocal), &modif_limit))
    { srch_time->SetFocus();  wxBell();  return; }
  matching_word=match_word->GetValue();
  matching_case=match_case->GetValue();
  sel_table=tables->GetValue();
  sel_query=queries->GetValue();
  sel_domain=domains->GetValue();
  sel_proc=procedures->GetValue();
  sel_trigger=triggers->GetValue();
  sel_seq=sequences->GetValue();
  sel_program=programs->GetValue();
  sel_diagr=diagrams->GetValue();
 // do search:
  search(srch_pattern->GetValue());  // searching the string WITH leading a trailing spaces
  object_search_history.add_to_history(srch_pattern->GetValue());  // adding to history the string in the original case
  event.Skip();
}

void SearchObjectsDlg::search(wxString pattern)
// Warning: overwrites the patters in the caller! (sharing values among strings)
{ uns16 flags=matching_word ? SEARCH_WHOLE_WORDS : 0;
  if (matching_case) flags|=SEARCH_CASE_SENSITIVE;
  else WXUpcase(cdp, pattern);
 // find and show the result window:
  MyFrame * frame = wxGetApp().frame;
  wxNotebook * output = frame->show_output_page(SEARCH_PAGE_NUM);
  if (!output) return;  // impossible
  SearchResultList * srd = (SearchResultList*)output->GetPage(SEARCH_PAGE_NUM);
 // clear the old results:
  srd->ClearAll();
  strcpy(srd->server_name, cdp->locid_server_name);
 // pass objects:
  wxBusyCursor wait;
  ttablenum tb;  char query[150+OBJ_NAME_LEN+2*UUID_SIZE];  tcursnum cursnum;
  trecnum rec, limit, trec;  tcateg categ;  tobjname objname;
  int encrypted_objs = 0;
  for (tb=TAB_TABLENUM;  tb<=OBJ_TABLENUM;  tb++)
  { if (in_schema)
    { WBUUID schema_uuid;
      cd_Apl_name2id(cdp, search_schema, schema_uuid);
      make_apl_query(query, tb ? "OBJTAB" : "TABTAB", schema_uuid); // searches own objects only
    }  
    else if (tb==OBJ_TABLENUM)
      strcpy(query, "select OBJ_NAME,CATEGORY,APL_UUID,DEFIN,FLAGS,FOLDER_NAME,LAST_MODIF from OBJTAB");
    else
      strcpy(query, "select TAB_NAME,CATEGORY,APL_UUID,DEFIN,FLAGS,FOLDER_NAME,LAST_MODIF from TABTAB");
    strcat(query, tb ? " ORDER BY CATEGORY,OBJ_NAME" : " ORDER BY TAB_NAME");
    if (!cd_Open_cursor_direct(cdp, query, &cursnum))
    { if (!cd_Rec_cnt(cdp, cursnum, &limit)) 
      for (rec=0;  rec<limit;  rec++)
      { if (!cd_Read(cdp, cursnum, rec, APLOBJ_CATEG, NULL, &categ))
        if (categ==CATEG_TABLE   && (sel_table  ||!selected_categs) || 
            categ==CATEG_DOMAIN  && (sel_domain ||!selected_categs) || 
            categ==CATEG_SEQ     && (sel_seq    ||!selected_categs) ||
            categ==CATEG_CURSOR  && (sel_query  ||!selected_categs) || 
            categ==CATEG_PROC    && (sel_proc   ||!selected_categs) || 
            categ==CATEG_TRIGGER && (sel_trigger||!selected_categs) || 
            categ==CATEG_DRAWING && (sel_diagr  ||!selected_categs) || 
            categ==CATEG_PGMSRC  && (sel_program||!selected_categs) ||
            categ==CATEG_VIEW    && !selected_categs )
        {
          if (cd_Translate(cdp, cursnum, rec, 0, &trec)) continue;  // internal error
         // do not search system tables (possible when [in_schema]==false):
          if (tb==TAB_TABLENUM && SYSTEM_TABLE(trec)) continue;
         // check for encryption:
          if (!not_encrypted_object(cdp, categ, trec))
            { encrypted_objs++;  continue; }
         // check the folder name:
          if (in_folder)
          { tobjname object_folder_name;
            if (cd_Read(cdp, tb, trec, OBJ_FOLDER_ATR, NULL, object_folder_name)) continue;
            if (!strcmp(folder_name, ".")) *folder_name=0;
            if (my_stricmp(folder_name, object_folder_name)) continue;
          }
         // check the modif time:
          if (modif_limit!=NONEINTEGER)
          { uns32 obj_modif_time;
            if (Get_obj_modif_time(cdp, trec, categ, &obj_modif_time))
              if (obj_modif_time < modif_limit)
                continue;
          }
         // search the object:
          if (cd_Read(cdp, tb, trec, OBJ_NAME_ATR, NULL, objname)) continue;
          wxGetApp().frame->SetStatusText(wxString(objname, *cdp->sys_conv));    
          tptr def=cd_Load_objdef(cdp, trec, categ);
          uns32 respos;
          if (def!=NULL)
          { if (Search_in_blockA(def, (uns32)strlen(def), NULL, pattern.mb_str(*cdp->sys_conv), (uns16)pattern.Length(), flags, cdp->sys_charset, &respos))
            { search_result * data = new search_result(categ, trec, respos);
              const wxChar * catname;
              switch (categ)
              { case CATEG_TABLE:   catname=_("Table");  break;
                case CATEG_CURSOR:  catname=_("Query");  break;
                case CATEG_TRIGGER: catname=_("Trigger");  break;
                case CATEG_PROC:    catname=_("Procedure");  break;
                case CATEG_DOMAIN:  catname=_("Domain");  break;
                case CATEG_SEQ:     catname=_("Sequence");  break;
                case CATEG_PGMSRC:  catname=_("Transport");  break;
                case CATEG_DRAWING: catname=_("Diagram");  break;
                default: catname=wxT("?");  break;
              }
             // get the schema name:
              tobjname schema_name;  
              if (!in_schema)
              { WBUUID schema_uuid;
                cd_Read(cdp, tb, trec, APPL_ID_ATR, NULL, schema_uuid);
                cd_Apl_id2name(cdp, schema_uuid, schema_name);
              }
             // get the folder name:
              tobjname object_folder_name;
              if (in_folder) strcpy(object_folder_name, folder_name);
              else Get_obj_folder_name(cdp, trec, categ, object_folder_name);
              if (!*object_folder_name || *object_folder_name=='.') strcpy(object_folder_name, "<root>");
             // form the result line:
              wxString msg;
              msg.Printf(_("%s %s in application %s, folder %s"), catname, 
                      wxString(objname, *cdp->sys_conv).c_str(),
                      wxString(in_schema ? search_schema : schema_name, *cdp->sys_conv).c_str(), 
                      wxString(object_folder_name, *cdp->sys_conv).c_str());
              srd->Append(msg, data);
            }
            corefree(def);
          }
        }
      }
      wxGetApp().frame->SetStatusText(wxEmptyString);
      cd_Close_cursor(cdp, cursnum);
    }
  }
 // finalize:
  if (srd->GetCount() > 0)
  { srd->SetSelection(0);
    srd->SetFocus();
  }
  else
  { wxString msg;
    msg.Printf(_("String '%s' not found."), srch_pattern->GetValue().c_str());
    srd->Append(msg, (void*)0);
  }
  if (encrypted_objs>0)
  { wxString msg;
    msg.Printf(_("%u encrypted object(s) not searched."), encrypted_objs);
    srd->Append(msg, (void*)0);
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SRCH_OPENSELECTOR
 */
#include <wx/calctrl.h>
#include "DateSelectorDlg.cpp"

#if 0
class PopupCalendarCtrl : public wxCalendarCtrl
{ void OnSelected(wxCalendarEvent & event);
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( PopupCalendarCtrl, wxCalendarCtrl )
    EVT_CALENDAR(-1, PopupCalendarCtrl::OnSelected)
END_EVENT_TABLE()

void PopupCalendarCtrl::OnSelected(wxCalendarEvent & event)
{ wxDateTime dtm=event.GetDate();
  uns32 date = Make_date(dtm.GetDay(), dtm.GetMonth(), dtm.GetYear());
  char buf[30];
  f_date2str(date, buf, format_date.mb_str(*wxConvCurrent));
  SearchObjectsDlg * dlg = (SearchObjectsDlg*)GetParent();
  dlg->Enable();
  dlg->srch_time->SetValue(buf);
  Destroy();
}
#endif

void SearchObjectsDlg::OnCdSrchOpenselectorClick( wxCommandEvent& event )
{ uns32 ts, date, time;
 // get the specified date or today's date:
  if (str2timestamp_default(srch_time->GetValue().mb_str(wxConvLocal), &ts) && ts!=NONEINTEGER)
    { date=timestamp2date(ts);  time=timestamp2time(ts); }
  else //if (!str2date(srch_time->GetValue().mb_str(wxConvLocal), &date) || date==NONEINTEGER)
    { date=Today();  time=0; }
 // show the calendar:
  wxDateTime dtm(Day(date), (wxDateTime::Month)(Month(date)-1), Year(date), 0, 0, 0, 0);
  DateSelectorDlg cal(&dtm);
  int x, y, w, h;
  mDateSelector->GetPosition(&x, &y); 
  wxPoint pt = ClientToScreen(wxPoint(x+20, y+20));
  cal.Create(this, -1, SYMBOL_DATESELECTORDLG_TITLE, pt, wxSize(170,180));
  cal.GetPosition(&x, &y); 
  cal.GetSize(&w, &h);
  if (x+w > wxSystemSettings::GetMetric(wxSYS_SCREEN_X))
    x = wxSystemSettings::GetMetric(wxSYS_SCREEN_X) - w;
  if (y+h > wxSystemSettings::GetMetric(wxSYS_SCREEN_Y))
    y = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) - h;
  cal.Move(x,y);
  if (cal.ShowModal()==1)
  { date = Make_date(dtm.GetDay(), dtm.GetMonth()+1, dtm.GetYear());
    char buf[50];
    timestamp2str_default(datetime2timestamp(date, time), buf);
    srch_time->SetValue(wxString(buf, wxConvLocal));
  }
}



/*!
 * Get bitmap resources
 */

wxBitmap SearchObjectsDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin SearchObjectsDlg bitmap retrieval
    return wxNullBitmap;
////@end SearchObjectsDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon SearchObjectsDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin SearchObjectsDlg icon retrieval
    return wxNullIcon;
////@end SearchObjectsDlg icon retrieval
}
