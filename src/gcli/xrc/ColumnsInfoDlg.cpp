/////////////////////////////////////////////////////////////////////////////
// Name:        ColumnsInfoDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     11/30/07 10:45:38
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "ColumnsInfoDlg.h"
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

#include "ColumnsInfoDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ColumnsInfoDlg type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ColumnsInfoDlg, wxPanel )

/*!
 * ColumnsInfoDlg event table definition
 */

BEGIN_EVENT_TABLE( ColumnsInfoDlg, wxPanel )

////@begin ColumnsInfoDlg event table entries
    EVT_COMBOBOX( CD_COLI_SCHEMA, ColumnsInfoDlg::OnCdColiSchemaSelected )

    EVT_RADIOBOX( CD_COLI_CATEG, ColumnsInfoDlg::OnCdColiCategSelected )

    EVT_COMBOBOX( CD_COLI_TABLE, ColumnsInfoDlg::OnCdColiTableSelected )

    EVT_BUTTON( CD_COLI_COPY, ColumnsInfoDlg::OnCdColiCopyClick )

    EVT_LISTBOX( D_COLI_LIST, ColumnsInfoDlg::OnDColiListSelected )

////@end ColumnsInfoDlg event table entries

END_EVENT_TABLE()

/*!
 * ColumnsInfoDlg constructors
 */

ColumnsInfoDlg::ColumnsInfoDlg( )
{ cdp=NULL;
}

ColumnsInfoDlg::ColumnsInfoDlg( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{ cdp=NULL;
  Create(parent, id, pos, size, style);
}

/*!
 * ColumnsInfoDlg creator
 */

bool ColumnsInfoDlg::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ColumnsInfoDlg member initialisation
    mSchema = NULL;
    mCateg = NULL;
    mTable = NULL;
    mCopy = NULL;
    mList = NULL;
    mType = NULL;
////@end ColumnsInfoDlg member initialisation

////@begin ColumnsInfoDlg creation
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ColumnsInfoDlg creation
    return TRUE;
}

/*!
 * Control creation for ColumnsInfoDlg
 */

void ColumnsInfoDlg::CreateControls()
{    
////@begin ColumnsInfoDlg content construction
    ColumnsInfoDlg* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxStaticText* itemStaticText3 = new wxStaticText;
    itemStaticText3->Create( itemPanel1, wxID_STATIC, _("Schema name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxString* mSchemaStrings = NULL;
    mSchema = new wxComboBox;
    mSchema->Create( itemPanel1, CD_COLI_SCHEMA, _T(""), wxDefaultPosition, wxDefaultSize, 0, mSchemaStrings, wxCB_READONLY|wxCB_SORT );
    itemBoxSizer2->Add(mSchema, 0, wxGROW|wxALL, 5);

    wxString mCategStrings[] = {
        _("&Table"),
        _("&Query")
    };
    mCateg = new wxRadioBox;
    mCateg->Create( itemPanel1, CD_COLI_CATEG, _("Category"), wxDefaultPosition, wxDefaultSize, 2, mCategStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer2->Add(mCateg, 0, wxGROW|wxLEFT|wxRIGHT, 5);

    wxString* mTableStrings = NULL;
    mTable = new wxComboBox;
    mTable->Create( itemPanel1, CD_COLI_TABLE, _T(""), wxDefaultPosition, wxDefaultSize, 0, mTableStrings, wxCB_READONLY|wxCB_SORT );
    itemBoxSizer2->Add(mTable, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer7, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText8 = new wxStaticText;
    itemStaticText8->Create( itemPanel1, wxID_STATIC, _("Columns:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemStaticText8, 0, wxALIGN_BOTTOM|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    itemBoxSizer7->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mCopy = new wxButton;
    mCopy->Create( itemPanel1, CD_COLI_COPY, _("Copy"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(mCopy, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxString* mListStrings = NULL;
    mList = new wxListBox;
    mList->Create( itemPanel1, D_COLI_LIST, wxDefaultPosition, wxDefaultSize, 0, mListStrings, wxLB_EXTENDED );
    itemBoxSizer2->Add(mList, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    mType = new wxTextCtrl;
    mType->Create( itemPanel1, CD_COLI_TYPE, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
    itemBoxSizer2->Add(mType, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end ColumnsInfoDlg content construction
    mCopy->Disable();  
}

/*!
 * Should we show tooltips?
 */

bool ColumnsInfoDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap ColumnsInfoDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ColumnsInfoDlg bitmap retrieval
    return wxNullBitmap;
////@end ColumnsInfoDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ColumnsInfoDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ColumnsInfoDlg icon retrieval
    return wxNullIcon;
////@end ColumnsInfoDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_COLI_TABLE
 */

void ColumnsInfoDlg::OnCdColiTableSelected( wxCommandEvent& event )
// Fill column names when table/query name is selected
{ tobjnum objnum;
  mList->Clear();  mCopy->Disable();  mType->SetValue(wxEmptyString);
  int selschema=mSchema->GetSelection(), seltable=mTable->GetSelection();
  if (selschema==wxNOT_FOUND || seltable==wxNOT_FOUND ) return;
  tcateg categ = mCateg->GetSelection() ? CATEG_CURSOR : CATEG_TABLE;
  objnum = (tobjnum)(size_t)mTable->GetClientData(seltable);
  //if (cd_Find_prefixed_object(cdp, mSchema->GetString(selschema).mb_str(*cdp->sys_conv), 
  //                                 mTable->GetString(seltable).mb_str(*cdp->sys_conv), 
  //                                 categ, &objnum)) return;  // object not found: nothing selected?
  const d_table * td = cd_get_table_d(cdp, objnum, categ);
  if (td)
  { const d_attr * pdatr;  int i;
    for (i=1, pdatr=ATTR_N(td,1);  i<td->attrcnt;  i++, pdatr=NEXT_ATTR(td,pdatr))
      mList->Append(wxString(pdatr->name, *cdp->sys_conv));
    release_table_d(td);
  }  
}

void ColumnsInfoDlg::fill_tables(void)
// fills tables ot queries
{ mTable->Clear();
  mList->Clear();  mCopy->Disable();  mType->SetValue(wxEmptyString);
  int selschema=mSchema->GetSelection();
  if (selschema==wxNOT_FOUND) return;
  tcateg categ = mCateg->GetSelection() ? CATEG_CURSOR : CATEG_TABLE;
  void * en = get_object_enumerator(cdp, categ, mSchema->GetString(selschema).mb_str(*cdp->sys_conv));
  if (en)
  { tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
    while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
      mTable->Append(wxString(name, *cdp->sys_conv), (void*)(size_t)objnum);
    free_object_enumerator(en);
  }
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_COLI_SCHEMA
 */

void ColumnsInfoDlg::OnCdColiSchemaSelected( wxCommandEvent& event )
{
  fill_tables();
}


/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_COLI_CATEG
 */

void ColumnsInfoDlg::OnCdColiCategSelected( wxCommandEvent& event )
{
  fill_tables();
}  
 
/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for D_COLI_LIST
 */

void ColumnsInfoDlg::OnDColiListSelected( wxCommandEvent& event )
{ tcateg categ = mCateg->GetSelection() ? CATEG_CURSOR : CATEG_TABLE;
  mType->SetValue(wxEmptyString);
  int seltable=mTable->GetSelection();
  wxArrayInt selections;
  int selcount = mList->GetSelections(selections);
  mCopy->Enable(selcount>0);
 // write the type information: 
  if (seltable==wxNOT_FOUND || selcount!=1) return;
  int selcol = selections[0];
  tobjnum objnum = (tobjnum)(size_t)mTable->GetClientData(seltable);
  const d_table * td = cd_get_table_d(cdp, objnum, categ);
  if (td)
  { const d_attr * pdatr = ATTR_N(td,selcol+1);
    char type_descr[100];
    if (pdatr->type==ATT_DOMAIN)
      cd_Read(cdp, OBJ_TABLENUM, pdatr->specif.domain_num, OBJ_NAME_ATR, NULL, type_descr);
    else
      sql_type_name(pdatr->type, pdatr->specif, true, type_descr);
    mType->SetValue(wxString(type_descr, *wxConvCurrent));
    release_table_d(td);
  }  
}

void ColumnsInfoDlg::Push_cdp(xcdp_t xcdpIn)
// Couples the window with a server, or detaches it if xcdpIn==NULL or it is an ODBC connection
{ 
  if (!xcdpIn || !xcdpIn->get_cdp())  // detach
  { mSchema->Clear();
    mTable->Clear();
    mList->Clear();
    mType->SetValue(wxEmptyString);
    cdp=NULL;
    return;
  }
  if (xcdpIn->get_cdp() == cdp)  // no change
    return;
 // list schemas   
  cdp=xcdpIn->get_cdp();
  mSchema->Clear();
  CObjectList ol(cdp, "", NULL);
  for (bool ItemFound = ol.FindFirst(CATEG_APPL); ItemFound; ItemFound = ol.FindNext())
    mSchema->Append(GetSmallStringName(ol));
  mTable->Clear();
  mList->Clear();
  mType->SetValue(wxEmptyString);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_COLI_COPY
 */

void ColumnsInfoDlg::OnCdColiCopyClick( wxCommandEvent& event )
{ wxString names;
  wxArrayInt selections;
  int selcount = mList->GetSelections(selections);
  for (int i=0;  i<selcount;  i++)
  { if (i) names=names+wxT(", ");
    names=names+mList->GetString(selections[i]);
  }  
  if (wxTheClipboard->Open())
  { wxTheClipboard->SetData(new wxTextDataObject(names));
    wxTheClipboard->Close();
  }
}


