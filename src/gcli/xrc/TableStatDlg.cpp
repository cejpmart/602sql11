/////////////////////////////////////////////////////////////////////////////
// Name:        TableStatDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     09/27/07 12:40:32
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "TableStatDlg.h"
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

#include "TableStatDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * TableStatDlg type definition
 */

IMPLEMENT_CLASS( TableStatDlg, wxDialog )

/*!
 * TableStatDlg event table definition
 */

BEGIN_EVENT_TABLE( TableStatDlg, wxDialog )

////@begin TableStatDlg event table entries
    EVT_BUTTON( CD_TABSTAT_COMP, TableStatDlg::OnCdTabstatCompClick )

    EVT_BUTTON( wxID_CLOSE, TableStatDlg::OnCloseClick )

    EVT_BUTTON( wxID_HELP, TableStatDlg::OnHelpClick )

////@end TableStatDlg event table entries

END_EVENT_TABLE()

/*!
 * TableStatDlg constructors
 */

TableStatDlg::TableStatDlg(cdp_t cdpIn, tobjnum objnumIn, wxString table_nameIn, wxString schema_nameIn)
{ 
  cdp=cdpIn;  objnum=objnumIn;  table_name=table_nameIn;  schema_name=schema_nameIn;
}

/*!
 * TableStatDlg creator
 */

bool TableStatDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin TableStatDlg member initialisation
    mMainCapt = NULL;
    mList1 = NULL;
    mCompute = NULL;
    mExt = NULL;
    mList2 = NULL;
////@end TableStatDlg member initialisation

////@begin TableStatDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end TableStatDlg creation
    return TRUE;
}

/*!
 * Control creation for TableStatDlg
 */

void TableStatDlg::CreateControls()
{    
////@begin TableStatDlg content construction
    TableStatDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mMainCapt = new wxStaticText;
    mMainCapt->Create( itemDialog1, CD_TABSTAT_MAIN_CAPT, _("Table `%s`.`%s`:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(mMainCapt, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mList1 = new wxListCtrl;
    mList1->Create( itemDialog1, CD_TABSTAT_LIST1, wxDefaultPosition, wxSize(100, 100), wxLC_REPORT|wxLC_NO_HEADER|wxLC_HRULES );
    itemBoxSizer2->Add(mList1, 0, wxGROW|wxALL, 5);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("Additional statistics on data contents:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText5, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer6, 0, wxALIGN_LEFT|wxALL, 0);

    mCompute = new wxButton;
    mCompute->Create( itemDialog1, CD_TABSTAT_COMP, _("Compute"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(mCompute, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mExt = new wxCheckBox;
    mExt->Create( itemDialog1, CD_TABSTAT_EXT, _("Include all available information (slow)"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mExt->SetValue(FALSE);
    itemBoxSizer6->Add(mExt, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mList2 = new wxListCtrl;
    mList2->Create( itemDialog1, CD_TABSTAT_LIST2, wxDefaultPosition, wxSize(100, 100), wxLC_REPORT|wxLC_NO_HEADER|wxLC_HRULES );
    itemBoxSizer2->Add(mList2, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer10, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton11 = new wxButton;
    itemButton11->Create( itemDialog1, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end TableStatDlg content construction
   // get the cluster size:
    uns32 block_size;
    cd_Get_server_info(cdp, OP_GI_CLUSTER_SIZE, &block_size, sizeof(block_size));
   // create columns of the list views:
    int w, h, tot_w;  wxListItem itemCol;
    itemCol.m_mask   = wxLIST_MASK_FORMAT | wxLIST_MASK_WIDTH;
    itemCol.m_format = wxLIST_FORMAT_LEFT;
    tot_w=wxSystemSettings::GetMetric(wxSYS_VSCROLL_X)+4;
    wxString msg;
    msg.Printf(_("Number of disc clusters (%u bytes) occupied by a record:"), block_size);
    mList1->GetTextExtent(msg, &w, &h);
    tot_w += itemCol.m_width = 16+w;
    mList1->InsertColumn(0, itemCol);
    tot_w += itemCol.m_width = 70;
    mList1->InsertColumn(1, itemCol);
    mList1->SetMinSize(wxSize(tot_w, 7*(5*h/4)+5));
    tot_w=wxSystemSettings::GetMetric(wxSYS_VSCROLL_X)+4;
    mList2->GetTextExtent(_("Allocated space for variable-length data:"), &w, &h);
    tot_w += itemCol.m_width = 16+w;
    mList2->InsertColumn(0, itemCol);
    tot_w += itemCol.m_width = 110;  // bigints
    mList2->InsertColumn(1, itemCol);
    mList2->SetMinSize(wxSize(tot_w, 11*(5*h/4)+5));
   // show table name:
    wxString capt;
    capt.Printf(mMainCapt->GetLabel(), schema_name.c_str(), table_name.c_str());
    mMainCapt->SetLabel(capt);
   // load and compile table definition:
    table_all ta;  trecnum reccnt;
    char * defin=cd_Load_objdef(cdp, objnum, CATEG_TABLE);
    if (!defin) cd_Signalize(cdp);  
    else
    { int comp_err = compile_table_to_all(cdp, defin, &ta);
      corefree(defin);  // release the buffer iff allocated here
      if (!comp_err)
      { int offset=1, page=0, i, lobs=0, extlobs=0;
        for (i=1;  i<ta.attrs.count();  i++)
        { atr_all * atr = ta.attrs.acc0(i);
          if (atr->type>=ATT_FIRST_HEAPTYPE && atr->type<=ATT_LAST_HEAPTYPE) 
            lobs++;
          else if (atr->type==ATT_EXTCLOB || atr->type==ATT_EXTBLOB) 
            extlobs++;
          int size = type_specif_size(atr->type, atr->specif);
          if (offset+size > block_size)
            { page++;  offset=size; }
          else offset+=size;
        }
       // show information about table components:
        mList1->InsertItem(0, _("Number of columns:"));
        mList1->SetItem(0, 1, int2wxstr(ta.attrs.count()-1));
        mList1->InsertItem(1, _("Number of LOB columns:"));
        mList1->SetItem(1, 1, int2wxstr(lobs));
        mList1->InsertItem(2, _("Number of external LOB columns:"));
        mList1->SetItem(2, 1, int2wxstr(extlobs));
        mList1->InsertItem(3, _("Number of indexes:"));
        mList1->SetItem(3, 1, int2wxstr(ta.indxs.count()));
        mList1->InsertItem(4, _("Number of check constrains:"));
        mList1->SetItem(4, 1, int2wxstr(ta.checks.count()));
        mList1->InsertItem(5, _("Number of foreign keys:"));
        mList1->SetItem(5, 1, int2wxstr(ta.refers.count()));
        mList1->InsertItem(6, _("Allocated space for records:"));
        cd_Rec_cnt(cdp, objnum, &reccnt);
        mList1->SetItem(6, 1, int2wxstr(reccnt));
        if (page>0 || offset>block_size/2)
        { msg.Printf(_("Number of disc clusters (%u bytes) occupied by a record:"), block_size);
          mList1->InsertItem(7, msg);
          mList1->SetItem(7, 1, int2wxstr(page+1));
        }
        else  
        { msg.Printf(_("Number of records located in a disc cluster (%u bytes):"), block_size);
          mList1->InsertItem(7, msg);
          mList1->SetItem(7, 1, int2wxstr(block_size/offset));
        }
      }
    }
   // prepare captions for computed data: 
   mList2->InsertItem(0,  _("Valid records:"));
   mList2->InsertItem(1,  _("Deleted records:"));
   mList2->InsertItem(2,  _("Free record space:"));
   mList2->InsertItem(3,  _("Allocated space for fixed-length data:"));
   mList2->InsertItem(4,  _("Quotient of valid records:"));
   mList2->InsertItem(5,  _("Quotient of valid and deleted records:"));
   mList2->InsertItem(6,  _("Allocated space for variable-length data:"));
   mList2->InsertItem(7,  _("Quotient used by valid records:"));
   mList2->InsertItem(8,  _("Quotient used by data:"));
   mList2->InsertItem(9,  _("Allocated space for indexes:"));
   mList2->InsertItem(10, _("Quotient used by data:"));
}

/*!
 * Should we show tooltips?
 */

bool TableStatDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap TableStatDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin TableStatDlg bitmap retrieval
    return wxNullBitmap;
////@end TableStatDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon TableStatDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin TableStatDlg icon retrieval
    return wxNullIcon;
////@end TableStatDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TABSTAT_COMP
 */

wxString ui64tostr(uns64 val)
{ char s[20];
#ifdef WINS
  _ui64toa(val, s, 10);
#else
  sprintf(s,"%llu", val);
#endif
  return wxString(s, *wxConvCurrent);
}  

wxString doubletostr(double val)
{ char s[20];
  sprintf(s, "%.3f", val);
  return wxString(s, *wxConvCurrent);
}  

void TableStatDlg::OnCdTabstatCompClick( wxCommandEvent& event )
{
  mList2->SetItem(3, 1, _("N/A"));
  mList2->SetItem(4, 1, _("N/A"));
  mList2->SetItem(5, 1, _("N/A"));
  mList2->SetItem(6, 1, _("N/A"));
  mList2->SetItem(7, 1, _("N/A"));
  mList2->SetItem(8, 1, _("N/A"));
  mList2->SetItem(9, 1, _("N/A"));
  mList2->SetItem(10,1, _("N/A"));
  if (mExt->GetValue())
  { wxString q;  tcursnum curs;  trecnum cnt;  uns64 u64val;  double dval;  
    q.Printf(wxT("select * from _iv_table_space where schema_name='%s' and table_name='%s'"), schema_name.c_str(), table_name.c_str());
    if (cd_Open_cursor_direct(cdp, q.mb_str(*cdp->sys_conv), &curs))
      cd_Signalize2(cdp, this);
    else
    { cd_Rec_cnt(cdp, curs, &cnt);  
      if (cnt>0)
      { cd_Read(cdp, curs, 0, 11, NULL, (char*)&cnt);
        mList2->SetItem(0, 1, int2wxstr(cnt));
        cd_Read(cdp, curs, 0, 12, NULL, (char*)&cnt);
        mList2->SetItem(1, 1, int2wxstr(cnt));
        cd_Read(cdp, curs, 0, 13, NULL, (char*)&cnt);
        mList2->SetItem(2, 1, int2wxstr(cnt));
        cd_Read(cdp, curs, 0, 3, NULL, (char*)&u64val);
        mList2->SetItem(3, 1, ui64tostr(u64val));
        cd_Read(cdp, curs, 0, 4, NULL, (char*)&dval);
        mList2->SetItem(4, 1, doubletostr(dval));
        cd_Read(cdp, curs, 0, 5, NULL, (char*)&dval);
        mList2->SetItem(5, 1, doubletostr(dval));
        cd_Read(cdp, curs, 0, 6, NULL, (char*)&u64val);
        mList2->SetItem(6, 1, ui64tostr(u64val));
        cd_Read(cdp, curs, 0, 7, NULL, (char*)&dval);
        mList2->SetItem(7, 1, doubletostr(dval));
        cd_Read(cdp, curs, 0, 8, NULL, (char*)&dval);
        mList2->SetItem(8, 1, doubletostr(dval));
        cd_Read(cdp, curs, 0, 9, NULL, (char*)&u64val);
        mList2->SetItem(9, 1, ui64tostr(u64val));
        cd_Read(cdp, curs, 0,10, NULL, (char*)&dval);
        mList2->SetItem(10,1, doubletostr(dval));
      }
      cd_Close_cursor(cdp, curs);
    }  
  }
  else
  { trecnum infos[4];  BOOL res;
    { wxBusyCursor wait;
      res=cd_Table_stat(cdp, objnum, infos);
    }
    if (res) cd_Signalize(cdp);
    else
    { mList2->SetItem(0, 1, int2wxstr(infos[0]));
      mList2->SetItem(1, 1, int2wxstr(infos[1]));
      mList2->SetItem(2, 1, int2wxstr(infos[2]));
    }
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void TableStatDlg::OnCloseClick( wxCommandEvent& event )
{
  Destroy();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void TableStatDlg::OnHelpClick( wxCommandEvent& event )
{ wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/_iv_table_space.html")); }


