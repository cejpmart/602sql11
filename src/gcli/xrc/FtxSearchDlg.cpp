/////////////////////////////////////////////////////////////////////////////
// Name:        FtxSearchDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     09/21/06 16:05:46
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "FtxSearchDlg.h"
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

#include "FtxSearchDlg.h"
#include "tabledsgn.h"

////@begin XPM images
////@end XPM images

/*!
 * FtxSearchDlg type definition
 */

IMPLEMENT_CLASS( FtxSearchDlg, wxDialog )

/*!
 * FtxSearchDlg event table definition
 */

BEGIN_EVENT_TABLE( FtxSearchDlg, wxDialog )

////@begin FtxSearchDlg event table entries
    EVT_BUTTON( CD_FTXSRCH_APPLY, FtxSearchDlg::OnCdFtxsrchApplyClick )

    EVT_COMBOBOX( CD_FTXSRCH_TABLE, FtxSearchDlg::OnCdFtxsrchTableSelected )

    EVT_BUTTON( wxID_CLOSE, FtxSearchDlg::OnCloseClick )

    EVT_BUTTON( wxID_HELP, FtxSearchDlg::OnHelpClick )

////@end FtxSearchDlg event table entries

END_EVENT_TABLE()

/*!
 * FtxSearchDlg constructors
 */

FtxSearchDlg::FtxSearchDlg(cdp_t cdpIn, tobjnum objnum)
{ data = NULL;
  cdp=cdpIn;
  char * def = cd_Load_objdef(cdp, objnum, CATEG_DRAWING, NULL);
  if (def) 
  { ftdef.modifying=false;
    int res=ftdef.compile(cdp, def);
    corefree(def);
  }  
}

FtxSearchDlg::FtxSearchDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * FtxSearchDlg creator
 */

bool FtxSearchDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin FtxSearchDlg member initialisation
    mVertSizer = NULL;
    mFind = NULL;
    mApply = NULL;
    mSizerAuto = NULL;
    mList = NULL;
    mSizerMan = NULL;
    mTable = NULL;
    mTxCol = NULL;
    mIDCol = NULL;
    mFormat = NULL;
    mIndirect = NULL;
    mPanel = NULL;
    mSizerResult = NULL;
////@end FtxSearchDlg member initialisation

////@begin FtxSearchDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end FtxSearchDlg creation
    return TRUE;
}

/*!
 * Control creation for FtxSearchDlg
 */

void FtxSearchDlg::CreateControls()
{    
////@begin FtxSearchDlg content construction
    FtxSearchDlg* itemDialog1 = this;

    mVertSizer = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(mVertSizer);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    mVertSizer->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("&Find:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mFind = new wxTextCtrl;
    mFind->Create( itemDialog1, CD_FTXSRCH_FIND, _T(""), wxDefaultPosition, wxSize(200, -1), 0 );
    itemBoxSizer3->Add(mFind, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 5);

    mApply = new wxButton;
    mApply->Create( itemDialog1, CD_FTXSRCH_APPLY, _("Search"), wxDefaultPosition, wxDefaultSize, 0 );
    mApply->SetDefault();
    itemBoxSizer3->Add(mApply, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSizerAuto = new wxBoxSizer(wxVERTICAL);
    mVertSizer->Add(mSizerAuto, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText8 = new wxStaticText;
    itemStaticText8->Create( itemDialog1, wxID_STATIC, _("&Search in the document sources:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerAuto->Add(itemStaticText8, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mListStrings = NULL;
    mList = new wxCheckListBox;
    mList->Create( itemDialog1, CD_FTXSRCH_LIST, wxDefaultPosition, wxDefaultSize, 0, mListStrings, wxLB_SINGLE );
    mSizerAuto->Add(mList, 1, wxGROW|wxALL, 5);

    mSizerMan = new wxBoxSizer(wxVERTICAL);
    mVertSizer->Add(mSizerMan, 0, wxGROW|wxALL, 0);

    wxFlexGridSizer* itemFlexGridSizer11 = new wxFlexGridSizer(2, 4, 0, 0);
    itemFlexGridSizer11->AddGrowableCol(1);
    itemFlexGridSizer11->AddGrowableCol(3);
    mSizerMan->Add(itemFlexGridSizer11, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText12 = new wxStaticText;
    itemStaticText12->Create( itemDialog1, wxID_STATIC, _("Document table:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemStaticText12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mTableStrings = NULL;
    mTable = new wxOwnerDrawnComboBox;
    mTable->Create( itemDialog1, CD_FTXSRCH_TABLE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTableStrings, wxCB_READONLY );
    itemFlexGridSizer11->Add(mTable, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText14 = new wxStaticText;
    itemStaticText14->Create( itemDialog1, wxID_STATIC, _("Document column:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemStaticText14, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mTxColStrings = NULL;
    mTxCol = new wxOwnerDrawnComboBox;
    mTxCol->Create( itemDialog1, CD_FTXSRCH_TXCOL, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTxColStrings, 0 );
    itemFlexGridSizer11->Add(mTxCol, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText16 = new wxStaticText;
    itemStaticText16->Create( itemDialog1, wxID_STATIC, _("Document ID column:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemStaticText16, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mIDColStrings = NULL;
    mIDCol = new wxOwnerDrawnComboBox;
    mIDCol->Create( itemDialog1, CD_FTXSRCH_IDCOL, _T(""), wxDefaultPosition, wxDefaultSize, 0, mIDColStrings, wxCB_READONLY );
    itemFlexGridSizer11->Add(mIDCol, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText18 = new wxStaticText;
    itemStaticText18->Create( itemDialog1, wxID_STATIC, _("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemStaticText18, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mFormatStrings = NULL;
    mFormat = new wxOwnerDrawnComboBox;
    mFormat->Create( itemDialog1, CD_FTXSRCH_FORMAT, _T(""), wxDefaultPosition, wxDefaultSize, 0, mFormatStrings, wxCB_READONLY );
    itemFlexGridSizer11->Add(mFormat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mIndirect = new wxCheckBox;
    mIndirect->Create( itemDialog1, CD_FTXSRCH_INDIRECT, _("Indirect"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mIndirect->SetValue(FALSE);
    mSizerMan->Add(mIndirect, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText21 = new wxStaticText;
    itemStaticText21->Create( itemDialog1, wxID_STATIC, _("Results:"), wxDefaultPosition, wxDefaultSize, 0 );
    mVertSizer->Add(itemStaticText21, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mPanel = new wxPanel;
    mPanel->Create( itemDialog1, CD_FTXSRCH_PANEL, wxPoint(300, 200), wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    mVertSizer->Add(mPanel, 2, wxGROW|wxALL, 5);

    mSizerResult = new wxBoxSizer(wxVERTICAL);
    mPanel->SetSizer(mSizerResult);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    mVertSizer->Add(itemBoxSizer24, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton25 = new wxButton;
    itemButton25->Create( itemDialog1, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemButton25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton26 = new wxButton;
    itemButton26->Create( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemButton26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end FtxSearchDlg content construction
 // title:
  wxString title, patt = GetTitle();
  title.Printf(patt, wxString(ftdef.name, *cdp->sys_conv).c_str());
  SetTitle(title);
 // result sizer:
  mSizerResult->SetMinSize(wxSize(400,150));
  if (ftdef.items.count())
  { mVertSizer->Hide(mSizerMan, true);
   // fill sources
    for (int i=0;  i<ftdef.items.count();  i++)
    { t_autoindex * ai = ftdef.items.acc0(i);
      wxString descr;
      descr = wxString(_("Column"))+wxT(" ")+wxString(ai->text_expr, *cdp->sys_conv)+wxT(" ")+_("from table")+wxT(" ")+
              wxString(ai->doctable_name, *cdp->sys_conv);
      mList->Append(descr);
      mList->Check(i);
    }
  }
  else
  { mVertSizer->Hide(mSizerAuto, true);
   // fill tables:
    void * en = get_object_enumerator(cdp, CATEG_TABLE, ftdef.schema);
    tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
    while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
      if (memcmp(name, "FTX_WORDTAB", 11) && memcmp(name, "FTX_REFTAB", 10) && memcmp(name, "FTX_DOCPART", 11))
        mTable->Append(wxString(name, *cdp->sys_conv));
    free_object_enumerator(en);
   // fill formats:
    mFormat->Append(wxT("AUTO"));
    mFormat->Append(wxT("plain"));
    mFormat->Append(wxT("HTML"));
    mFormat->Append(wxT("XML"));
    mFormat->Append(wxT("DOC"));
    mFormat->Append(wxT("XLS"));
    mFormat->Append(wxT("PPT"));
    mFormat->Append(wxT("PDF"));
    mFormat->Append(wxT("OOO"));
    mFormat->Append(wxT("FO"));
    mFormat->Append(wxT("ZFO"));
    mFormat->SetSelection(0);
  }
  mFind->SetFocus();
}

/*!
 * Should we show tooltips?
 */

bool FtxSearchDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap FtxSearchDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin FtxSearchDlg bitmap retrieval
    return wxNullBitmap;
////@end FtxSearchDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon FtxSearchDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin FtxSearchDlg icon retrieval
    return wxNullIcon;
////@end FtxSearchDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FTXSRCH_APPLY
 */

wxString FtxSearchDlg::make_select_for_doc_source(const char * doctable_name, const char * id_column, const char * text_expr, const char * format, int mode, int wcnt, wxString fnd)
{ wxString sql;
  sql=wxT(" SELECT `")+wxString(id_column, *cdp->sys_conv)+wxT("` AS `")+_("Id")  
          +wxT("`,Fulltext_get_context('")+wxString(ftdef.schema, *cdp->sys_conv)+wxT(".")+wxString(ftdef.name, *cdp->sys_conv)
            +wxT("',")+wxString(text_expr, *cdp->sys_conv)
            +wxT(",'")+wxString(format, *cdp->sys_conv)
            +wxT("',")+int2wxstr(mode)
            +wxT(",@@FULLTEXT_POSITION,")+int2wxstr(wcnt)+wxT(",5,'>>%<<') AS `")+_("Context")
          +wxT("`,CAST('")+wxString(doctable_name, *cdp->sys_conv)+wxT("'AS NCHAR(31))+'/'+CAST('")+
                           wxString(text_expr, *cdp->sys_conv)+wxT("'AS NCHAR(31))")+
          wxT(" AS `")+_("Source")  
          +wxT("` FROM ")+wxString(doctable_name, *cdp->sys_conv)
          +wxT(" WHERE Fulltext('")+wxString(ftdef.schema, *cdp->sys_conv)+wxT(".")+wxString(ftdef.name, *cdp->sys_conv)+wxT("',`")
          +wxString(id_column, *cdp->sys_conv)+wxT("`,'")+fnd+wxT("')");
  return sql;
}            
void FtxSearchDlg::OnCdFtxsrchApplyClick( wxCommandEvent& event )
{
  wxString fnd = mFind->GetValue();
 // count words in the 1st phrase:
  const wxChar * p = fnd.c_str();
  int wcnt=0;
  while (*p && *p<=' ') p++;
  if (*p=='"')
  { p++;
    do
    { while (*p && *p<=' ') p++;
      if (*p=='"' || !*p) break;
      while (*p>' ' && *p!='"') p++;
      wcnt++;
    } while (true);  
  }
  else wcnt=1;  // not a phrase, select 1st word only
  
  wxString sql;  bool first=true,  any=false;
  sql = wxT("INSENSITIVE CURSOR FOR ");
  if (ftdef.items.count()>0)
  { for (int i=0;  i<ftdef.items.count();  i++)
      if (mList->IsChecked(i))
      { t_autoindex * ai = ftdef.items.acc0(i);
        any=true;
        if (!first) sql=sql+wxT(" UNION ");
        else first=false;
        sql=sql+make_select_for_doc_source(ai->doctable_name, ai->id_column, ai->text_expr, ai->format, ai->mode, wcnt, fnd);
      }
  }
  else    
  { wxString doctab, idcol, txcol, format;
    int sel=mTable->GetSelection();
    if (sel!=wxNOT_FOUND)
    { doctab=mTable->GetString(sel);
      int sel=mIDCol->GetSelection();
      if (sel!=wxNOT_FOUND)
      { idcol=mIDCol->GetString(sel);
        int sel=mTxCol->GetSelection();
        if (sel!=wxNOT_FOUND)
        { txcol=mTxCol->GetString(sel);
          format=mFormat->GetString(mFormat->GetSelection());
          sql=sql+make_select_for_doc_source(doctab.mb_str(*cdp->sys_conv), idcol.mb_str(*cdp->sys_conv), 
                      txcol.mb_str(*cdp->sys_conv), format.mb_str(*cdp->sys_conv), mIndirect->IsChecked()?1:0, wcnt, fnd);
          any=true;            
        }
      }
    }
  }                    
  if (!any)
    { wxBell();  return; }
  BOOL res;  tcursnum curs;
  { wxBusyCursor busy;
    res = cd_SQL_execute(cdp, sql.mb_str(*cdp->sys_conv), (uns32*)&curs);
  }
  if (res) 
    { cd_Signalize2(cdp, this);  return; }
  if (data) 
    { data->Destroy();  data=NULL; }
  data = open_data_grid(mPanel, cdp, CATEG_DIRCUR, curs, "", false, NULL);
  if (!data)
    { cd_Close_cursor(cdp, res);  return; }
  data->SetMinSize(wxSize(400,150));
  mSizerResult->Add(data, 2, wxGROW|wxALL, 0);
  mSizerResult->Layout();
  Layout();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void FtxSearchDlg::OnCloseClick( wxCommandEvent& event )
{
    Destroy();
    event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void FtxSearchDlg::OnHelpClick( wxCommandEvent& event )
{
  wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/fulltext_search.html"));
}


/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_FTXSRCH_TABLE
 */

void FtxSearchDlg::OnCdFtxsrchTableSelected( wxCommandEvent& event )
{ 
  mTxCol->Clear();  mIDCol->Clear();
  int sel=mTable->GetSelection();
  if (sel!=wxNOT_FOUND)
  { tobjnum objnum;  
    if (!cd_Find_prefixed_object(cdp, ftdef.schema, mTable->GetString(sel).mb_str(*cdp->sys_conv), CATEG_TABLE, &objnum))
    {// load and compile table definition:
      const char * defin=cd_Load_objdef(cdp, objnum, CATEG_TABLE);
      if (!defin) return;
      table_all * ta = new table_all;
      int err=compile_table_to_all(cdp, defin, ta);
      corefree(defin);
      if (err) { delete ta;  return; }
      for (int a=1;  a<ta->attrs.count();  a++)
      { atr_all * atr = ta->attrs.acc0(a);
       // ID: for columns with the proper type check the default value and index:
        if (atr->type==ATT_INT32 || atr->type==ATT_INT64)
        { ind_descr * indx = get_index_by_column(cdp, ta, atr->name);
          if (indx && indx->index_type!=INDEX_NONUNIQUE)  // PRIMARY KEY or UNIQUE
            mIDCol->Append(wxString(atr->name, *cdp->sys_conv));  // not checking the default value - ID may be generated in other way
        }
       // Txt:
        if (atr->type==ATT_TEXT || atr->type==ATT_STRING || atr->type==ATT_NOSPEC)
          mTxCol->Append(wxString(atr->name, *cdp->sys_conv));
      }
    }
    if (mIDCol->GetCount()>0) mIDCol->SetSelection(0);
    if (mTxCol->GetCount()>0) mTxCol->SetSelection(0);
  }
}


