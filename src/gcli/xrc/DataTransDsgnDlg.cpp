/////////////////////////////////////////////////////////////////////////////
// Name:        DataTransDsgnDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/05/04 11:13:01
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "DataTransDsgnDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "DataTransDsgnDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * DataTransDsgnDlg type definition
 */

IMPLEMENT_CLASS( DataTransDsgnDlg, wxPanel )

/*!
 * DataTransDsgnDlg event table definition
 */

BEGIN_EVENT_TABLE( DataTransDsgnDlg, wxPanel )

////@begin DataTransDsgnDlg event table entries
    EVT_BUTTON( CD_TR_BACK_FROM, DataTransDsgnDlg::OnCdTrBackFromClick )

    EVT_BUTTON( CD_TR_BACK_TO, DataTransDsgnDlg::OnCdTrBackToClick )

////@end DataTransDsgnDlg event table entries

END_EVENT_TABLE()

BEGIN_EVENT_TABLE( DelRowHandler, wxEvtHandler )
  EVT_KEY_DOWN(DelRowHandler::OnKeyDown)
  EVT_GRID_RANGE_SELECT(DelRowHandler::OnRangeSelect)
END_EVENT_TABLE()

/*!
 * DataTransDsgnDlg constructors
 */

DataTransDsgnDlg::DataTransDsgnDlg(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn) :
    any_designer(cdpIn, objnumIn, folder_nameIn, schema_nameIn, CATEG_PGMSRC, _transp_xpm, &transdsng_coords),
    x(cdp, false)
{ changed=false; 
  mGrid=NULL;
}
/*!
 * DataTransDsgnDlg creator
 */

bool DataTransDsgnDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DataTransDsgnDlg member initialisation
    mFromType = NULL;
    mFromName = NULL;
    mFromButton = NULL;
    mToType = NULL;
    mToName = NULL;
    mToButton = NULL;
    mGrid = NULL;
////@end DataTransDsgnDlg member initialisation

////@begin DataTransDsgnDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end DataTransDsgnDlg creation
    return TRUE;
}

/*!
 * Control creation for DataTransDsgnDlg
 */

void DataTransDsgnDlg::CreateControls()
{    
////@begin DataTransDsgnDlg content construction
    DataTransDsgnDlg* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemPanel1, wxID_ANY, _("Summary"));
    wxStaticBoxSizer* itemStaticBoxSizer3 = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxHORIZONTAL);
    itemBoxSizer2->Add(itemStaticBoxSizer3, 0, wxGROW|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(3, 4, 0, 0);
    itemFlexGridSizer4->AddGrowableCol(2);
    itemStaticBoxSizer3->Add(itemFlexGridSizer4, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemFlexGridSizer4->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemPanel1, wxID_STATIC, _("Type:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemPanel1, wxID_STATIC, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText7, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    itemFlexGridSizer4->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText9 = new wxStaticText;
    itemStaticText9->Create( itemPanel1, wxID_STATIC, _("From:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mFromType = new wxStaticText;
    mFromType->Create( itemPanel1, CD_TR_FROM, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(mFromType, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mFromName = new wxStaticText;
    mFromName->Create( itemPanel1, CD_TR_FROM_NAME, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(mFromName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxFIXED_MINSIZE, 5);

    wxBitmap mFromButtonBitmap(wxNullBitmap);
    mFromButton = new wxBitmapButton;
    mFromButton->Create( itemPanel1, CD_TR_BACK_FROM, mFromButtonBitmap, wxDefaultPosition, wxSize(23, 23), wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemFlexGridSizer4->Add(mFromButton, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxStaticText* itemStaticText13 = new wxStaticText;
    itemStaticText13->Create( itemPanel1, wxID_STATIC, _("To:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText13, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mToType = new wxStaticText;
    mToType->Create( itemPanel1, CD_TR_TO, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(mToType, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mToName = new wxStaticText;
    mToName->Create( itemPanel1, CD_TR_TO_NAME, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(mToName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 5);

    wxBitmap mToButtonBitmap(wxNullBitmap);
    mToButton = new wxBitmapButton;
    mToButton->Create( itemPanel1, CD_TR_BACK_TO, mToButtonBitmap, wxDefaultPosition, wxSize(23, 23), wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemFlexGridSizer4->Add(mToButton, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    mGrid = new wxGrid( itemPanel1, CD_TR_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    itemBoxSizer2->Add(mGrid, 1, wxGROW|wxALL, 5);

////@end DataTransDsgnDlg content construction
    //mGrid = new wxGrid( itemPanel1, CD_TR_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    //itemBoxSizer2->Add(mGrid, 1, wxGROW|wxALL, 5);
    // remove calling grid methods
  SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);
 // set bitmaps:
  wxBitmap bmp(openselector_xpm);
  mFromButton->SetBitmapLabel(bmp);
  mToButton->SetBitmapLabel(bmp);
 // prepare grid:
    DelRowHandler * handler = new DelRowHandler(this);
    mGrid->PushEventHandler(handler);
    CreateSpecificGrid();
}

/*!
 * Should we show tooltips?
 */

bool DataTransDsgnDlg::ShowToolTips()
{
    return TRUE;
}

#if WBVERS<950
#include "NewDataTransferDlg.h"
#else
#include "NewDataTransfer95Dlg.h"
#endif

void DataTransDsgnDlg::show_source_target_info(void)
{ wxString str;
  str = x.source_type==IETYPE_WB   ? _("Table")      : x.source_type==IETYPE_CURSOR ? _("Query") : 
        x.source_type==IETYPE_ODBC ? _("ODBC Table") : x.source_type==IETYPE_CURSOR ? _("ODBC View") : 
                                     _("File");
  mFromType->SetLabel(str);
  if (x.db_source() || x.odbc_source())
  { if (*x.inserver)
      str = wxString(x.inserver, *wxConvCurrent) + L":";
    else 
      str = wxEmptyString;
    if (*x.inschema)
      str = str + wxString(x.inschema, *cdp->sys_conv) + L".";
    str = str + wxString(x.inpath, *cdp->sys_conv);
  }
  else
    str = wxString(x.inpath, *wxConvCurrent);
  if (x.source_type!=IETYPE_WB && x.source_type!=IETYPE_CURSOR && x.source_type!=IETYPE_ODBC && x.source_type!=IETYPE_ODBCVIEW && x.source_type!=IETYPE_TDT)
    str=str+wxT(" [")+encoding_name(x.src_recode)+wxT("]");
  mFromName->SetLabel(str);
  str = x.dest_type==IETYPE_WB ? _("Table") : x.dest_type==IETYPE_ODBC ? _("ODBC Table") : _("File");
  mToType  ->SetLabel(str);
  if (x.db_target() || x.odbc_target())
  { if (*x.outserver)
      str = wxString(x.outserver, *wxConvCurrent) + L":";
    else 
      str = wxEmptyString;
    if (*x.outschema)
      str = str + wxString(x.outschema, *cdp->sys_conv) + L".";
    str = str + wxString(x.outpath, *cdp->sys_conv);
  }
  else
    str = wxString(x.outpath, *wxConvCurrent);
  if (x.dest_type!=IETYPE_WB && x.dest_type!=IETYPE_ODBC && x.dest_type!=IETYPE_TDT)
    str=str+wxT(" [")+encoding_name(x.dest_recode)+wxT("]");
  mToName  ->SetLabel(str);
}

int type_class(int ds_type)
// Says is 2 formats are similiar.
{ switch (ds_type)
  { case IETYPE_WB:  case IETYPE_CURSOR:
      return 0;
    case IETYPE_CSV:  case IETYPE_COLS:
      return 1;
    case IETYPE_DBASE:  case IETYPE_FOX:
      return 2;
    case IETYPE_TDT:
      return 3;
    default:
      return 4;
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_BACK_FROM
 */

void DataTransDsgnDlg::OnCdTrBackFromClick( wxCommandEvent& event )
{ if (back_and_restart(1))
    changed=true;  // may have been changed
}

bool DataTransDsgnDlg::back_and_restart(int page)
{ 
  if (mGrid) mGrid->DisableCellEditControl();
  t_ie_run cpy(x.cdp, x.silent);  // must edit the copy
  cpy.copy_from(x);
#if WBVERS<950
  NewDataTransferDlg dlg(&cpy, false, NULL);
#else
  NewDataTransfer95Dlg dlg(&cpy, false, NULL);
#endif
  dlg.Create(dialog_parent());
  if (!dlg.RunUpdate(page)) return false;
  bool big_change = (type_class(x.source_type)!=type_class(cpy.source_type) ||
                     type_class(x.dest_type  )!=type_class(cpy.dest_type  ) ||
                     x.creating_target        !=cpy.creating_target);
  x.copy_from(cpy);
  find_servers(&x, this);
  x.analyse_dsgn();
  show_source_target_info();
  if (big_change)
    OnCdTrCreImplClick();
  return true;
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_BACK_TO
 */

void DataTransDsgnDlg::OnCdTrBackToClick( wxCommandEvent& event )
{ if (back_and_restart(2))
    changed=true;  // may have been changed
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_CRE_IMPL
 */

void DataTransDsgnDlg::OnCdTrCreImplClick( void )
{ wxSizer* topsizer = GetSizer();
  if (mGrid)
  { topsizer->Detach(mGrid);
    mGrid->PopEventHandler(true);
    mGrid->Destroy();
  }
  x.dd.~detail_dynar();
  find_servers(&x, this);
  x.analyse_dsgn();  // data source may not have been found, ignoring here
  x.OnCreImpl();  
  x.dd.next();  // new fictive item
  mGrid = new wxGrid(this, CD_TR_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
  topsizer->Add(mGrid, 1, wxGROW|wxALL, 5);
  DelRowHandler * handler = new DelRowHandler(this);
  mGrid->PushEventHandler(handler);
  CreateSpecificGrid();
  Layout();
  if (x.dest_exists)
    report_mapping_result(x, this);
}



/*!
 * Get bitmap resources
 */

wxBitmap DataTransDsgnDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin DataTransDsgnDlg bitmap retrieval
    return wxNullBitmap;
////@end DataTransDsgnDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon DataTransDsgnDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin DataTransDsgnDlg icon retrieval
    return wxNullIcon;
////@end DataTransDsgnDlg icon retrieval
}
