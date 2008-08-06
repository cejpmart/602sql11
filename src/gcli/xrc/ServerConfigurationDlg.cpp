/////////////////////////////////////////////////////////////////////////////
// Name:        ServerConfigurationDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/02/04 08:18:58
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "ServerConfigurationDlg.h"
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

#include "ServerConfigurationDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ServerConfigurationDlg type definition
 */

IMPLEMENT_CLASS( ServerConfigurationDlg, wxDialog )

/*!
 * ServerConfigurationDlg event table definition
 */

BEGIN_EVENT_TABLE( ServerConfigurationDlg, wxDialog )

////@begin ServerConfigurationDlg event table entries
    EVT_GRID_CELL_CHANGE( ServerConfigurationDlg::OnCdOperGridCellChange )

    EVT_GRID_CELL_CHANGE( ServerConfigurationDlg::OnCdSecurGridCellChange )

    EVT_GRID_CELL_CHANGE( ServerConfigurationDlg::OnCdCompatGridCellChange )

    EVT_LISTBOX( CD_DIRS_LIST, ServerConfigurationDlg::OnCdDirsListSelected )
    EVT_LISTBOX_DCLICK( CD_DIRS_LIST, ServerConfigurationDlg::OnCdDirsListDoubleClicked )

    EVT_TEXT( CD_DIRS_EDIT, ServerConfigurationDlg::OnCdDirsEditUpdated )

    EVT_BUTTON( CD_DIRS_SEL, ServerConfigurationDlg::OnCdDirsSelClick )

    EVT_BUTTON( CD_DIRS_NEW, ServerConfigurationDlg::OnCdDirsNewClick )

    EVT_BUTTON( CD_DIR_MODIF, ServerConfigurationDlg::OnCdDirModifClick )

    EVT_BUTTON( CD_DIRS_REMOVE, ServerConfigurationDlg::OnCdDirsRemoveClick )

    EVT_GRID_CELL_CHANGE( ServerConfigurationDlg::OnCellChange )

    EVT_BUTTON( wxID_OK, ServerConfigurationDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, ServerConfigurationDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, ServerConfigurationDlg::OnHelpClick )

////@end ServerConfigurationDlg event table entries
    // remove OnCdIpEnaCellChange  OnCdIpDisCellChange
END_EVENT_TABLE()

/*!
 * ServerConfigurationDlg constructors
 */

ServerConfigurationDlg::ServerConfigurationDlg(cdp_t cdpIn, PropertyGrid * pg1In, PropertyGrid * pg2In, PropertyGrid * pg3In)
{ cdp=cdpIn;
  pg1=pg1In;  pg2=pg2In;  pg3=pg3In;
  dirs_changed=false;
  ip_changed=false;
  conf_adm = cd_Am_I_config_admin(cdp) == TRUE;
}

#define MAX_IP_LEN (6*3+5)  // for IP6 with dots

/*!
 * ServerConfigurationDlg creator
 */

bool ServerConfigurationDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ServerConfigurationDlg member initialisation
    mNotebook = NULL;
    mOperGrid = NULL;
    mConftabSecur = NULL;
    mSecurGrid = NULL;
    mCompatGrid = NULL;
    mDirsList = NULL;
    mEdit = NULL;
    mSel = NULL;
    mNewDir = NULL;
    mModifDir = NULL;
    mRemDir = NULL;
    mSizerContainingGrids = NULL;
    mIPEna = NULL;
    mIPDis = NULL;
    mSizerContainingGrids1 = NULL;
    mIP1Ena = NULL;
    mIP1Dis = NULL;
    mSizerContainingGrids2 = NULL;
    mIP2Ena = NULL;
    mIP2Dis = NULL;
////@end ServerConfigurationDlg member initialisation

////@begin ServerConfigurationDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ServerConfigurationDlg creation
    SetWindowStyleFlag(GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);  // otherwise Enter and ESC in the grid cell editors close the dialog
    return TRUE;
}

static void prep_and_load_IP(cdp_t cdp, wxSizer * mSizerContainingGrids, wxGrid * mIP, const char * addr_name, const char * mask_name)
{ mIP->SetColLabelValue(0, _("Address"));
  mIP->SetColLabelValue(1, _("Mask"));
  mIP->SetColLabelSize(default_font_height);
  mIP->SetDefaultRowSize(default_font_height);
  mIP->SetLabelFont(system_gui_font);
  mSizerContainingGrids->SetItemMinSize(mIP, 288, 90);
  //mIP->SetDefaultEditor(make_limited_string_editor(MAX_IP_LEN)); -- does not work
  wxGridCellAttr * attr = new wxGridCellAttr;
  attr->SetEditor(make_limited_string_editor(MAX_IP_LEN));
  mIP->SetColAttr(0, attr);
  attr = new wxGridCellAttr;
  attr->SetEditor(make_limited_string_editor(MAX_IP_LEN));
  mIP->SetColAttr(1, attr);
  char buf_ip[MAX_IP_LEN+1], buf_mask[MAX_IP_LEN+1];
  int counter=1;
  do
  { if (cd_Get_property_value(cdp, sqlserver_owner, addr_name, counter, buf_ip, sizeof(buf_ip)) || !*buf_ip)
      break;
    mIP->InsertRows(counter-1, 1);
    mIP->SetCellValue(counter-1, 0, wxString(buf_ip, *wxConvCurrent));
    if (cd_Get_property_value(cdp, sqlserver_owner, mask_name, counter, buf_mask, sizeof(buf_mask)))
      *buf_mask=0;
    mIP->SetCellValue(counter-1, 1, wxString(buf_mask, *wxConvCurrent));
    counter++;
  } while (true);
}

/*!
 * Control creation for ServerConfigurationDlg
 */

void ServerConfigurationDlg::CreateControls()
{    
////@begin ServerConfigurationDlg content construction
    ServerConfigurationDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mNotebook = new wxNotebook;
    mNotebook->Create( itemDialog1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP|wxNB_MULTILINE );
#if !wxCHECK_VERSION(2,5,2)
    wxNotebookSizer* mNotebookSizer = new wxNotebookSizer(mNotebook);
#endif

    wxPanel* itemPanel4 = new wxPanel;
    itemPanel4->Create( mNotebook, CD_CONFTAB_OPER, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemPanel4->SetSizer(itemBoxSizer5);

    mOperGrid = new wxGrid( itemPanel4, CD_OPER_GRID, wxDefaultPosition, wxSize(200, 150), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mOperGrid->SetDefaultColSize(50);
    mOperGrid->SetDefaultRowSize(25);
    mOperGrid->SetColLabelSize(25);
    mOperGrid->SetRowLabelSize(50);
    mOperGrid->CreateGrid(5, 5, wxGrid::wxGridSelectCells);
    itemBoxSizer5->Add(mOperGrid, 1, wxGROW|wxALL, 0);

    mNotebook->AddPage(itemPanel4, _("Operation"));

    mConftabSecur = new wxPanel;
    mConftabSecur->Create( mNotebook, CD_CONFTAB_SECUR, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxVERTICAL);
    mConftabSecur->SetSizer(itemBoxSizer8);

    mSecurGrid = new wxGrid( mConftabSecur, CD_SECUR_GRID, wxDefaultPosition, wxSize(400, 250), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mSecurGrid->SetDefaultColSize(50);
    mSecurGrid->SetDefaultRowSize(25);
    mSecurGrid->SetColLabelSize(25);
    mSecurGrid->SetRowLabelSize(0);
    mSecurGrid->CreateGrid(5, 2, wxGrid::wxGridSelectCells);
    itemBoxSizer8->Add(mSecurGrid, 1, wxGROW|wxALL, 0);

    mNotebook->AddPage(mConftabSecur, _("Security"));

    wxPanel* itemPanel10 = new wxPanel;
    itemPanel10->Create( mNotebook, CD_CONFTAB_COMPAT, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxVERTICAL);
    itemPanel10->SetSizer(itemBoxSizer11);

    mCompatGrid = new wxGrid( itemPanel10, CD_COMPAT_GRID, wxDefaultPosition, wxSize(200, 150), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mCompatGrid->SetDefaultColSize(50);
    mCompatGrid->SetDefaultRowSize(25);
    mCompatGrid->SetColLabelSize(25);
    mCompatGrid->SetRowLabelSize(50);
    mCompatGrid->CreateGrid(5, 5, wxGrid::wxGridSelectCells);
    itemBoxSizer11->Add(mCompatGrid, 1, wxGROW|wxALL, 0);

    mNotebook->AddPage(itemPanel10, _("SQL Compatibility"));

    wxPanel* itemPanel13 = new wxPanel;
    itemPanel13->Create( mNotebook, CD_CONFTAB_DIRS, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxVERTICAL);
    itemPanel13->SetSizer(itemBoxSizer14);

    wxStaticText* itemStaticText15 = new wxStaticText;
    itemStaticText15->Create( itemPanel13, wxID_STATIC, _("Directories containing external DLL/SO used by the SQL Server:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText15, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer14->Add(itemBoxSizer16, 1, wxGROW|wxALL, 0);
    wxString* mDirsListStrings = NULL;
    mDirsList = new wxListBox;
    mDirsList->Create( itemPanel13, CD_DIRS_LIST, wxDefaultPosition, wxDefaultSize, 0, mDirsListStrings, wxLB_SINGLE );
    itemBoxSizer16->Add(mDirsList, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer14->Add(itemBoxSizer18, 0, wxGROW|wxALL, 0);
    mEdit = new wxTextCtrl;
    mEdit->Create( itemPanel13, CD_DIRS_EDIT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(mEdit, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSel = new wxButton;
    mSel->Create( itemPanel13, CD_DIRS_SEL, _("&Browse"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(mSel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer14->Add(itemBoxSizer21, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);
    mNewDir = new wxButton;
    mNewDir->Create( itemPanel13, CD_DIRS_NEW, _("Add directory"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(mNewDir, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mModifDir = new wxButton;
    mModifDir->Create( itemPanel13, CD_DIR_MODIF, _("Replace directory"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(mModifDir, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mRemDir = new wxButton;
    mRemDir->Create( itemPanel13, CD_DIRS_REMOVE, _("Remove directory"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(mRemDir, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mNotebook->AddPage(itemPanel13, _("DLL/SO Directories"));

    wxPanel* itemPanel25 = new wxPanel;
    itemPanel25->Create( mNotebook, CD_CONFTAB_IP, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    mSizerContainingGrids = new wxBoxSizer(wxVERTICAL);
    itemPanel25->SetSizer(mSizerContainingGrids);

    wxStaticText* itemStaticText27 = new wxStaticText;
    itemStaticText27->Create( itemPanel25, wxID_STATIC, _("Clients can connect to the server from the IP addresses:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids->Add(itemStaticText27, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mIPEna = new wxGrid( itemPanel25, CD_IP_ENA, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mIPEna->SetDefaultColSize(130);
    mIPEna->SetDefaultRowSize(20);
    mIPEna->SetColLabelSize(20);
    mIPEna->SetRowLabelSize(0);
    mIPEna->CreateGrid(1, 2, wxGrid::wxGridSelectCells);
    mSizerContainingGrids->Add(mIPEna, 1, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText29 = new wxStaticText;
    itemStaticText29->Create( itemPanel25, wxID_STATIC, _("NOTE: If no address is specified above, all addresses\nare enabled, except for the addresses specified below."), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids->Add(itemStaticText29, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText30 = new wxStaticText;
    itemStaticText30->Create( itemPanel25, wxID_STATIC, _("Disabled addresses:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids->Add(itemStaticText30, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mIPDis = new wxGrid( itemPanel25, CD_IP_DIS, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mIPDis->SetDefaultColSize(130);
    mIPDis->SetDefaultRowSize(20);
    mIPDis->SetColLabelSize(20);
    mIPDis->SetRowLabelSize(0);
    mIPDis->CreateGrid(1, 2, wxGrid::wxGridSelectCells);
    mSizerContainingGrids->Add(mIPDis, 1, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText32 = new wxStaticText;
    itemStaticText32->Create( itemPanel25, wxID_STATIC, _("NOTE: If mask is not specified, the exact IP address is used."), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids->Add(itemStaticText32, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    mNotebook->AddPage(itemPanel25, _("IP Filtering for TCP"));

    wxPanel* itemPanel33 = new wxPanel;
    itemPanel33->Create( mNotebook, CD_CONFTAB_IP1, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    mSizerContainingGrids1 = new wxBoxSizer(wxVERTICAL);
    itemPanel33->SetSizer(mSizerContainingGrids1);

    wxStaticText* itemStaticText35 = new wxStaticText;
    itemStaticText35->Create( itemPanel33, wxID_STATIC, _("Clients can connect to the server from the IP addresses:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids1->Add(itemStaticText35, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mIP1Ena = new wxGrid( itemPanel33, CD_IP_ENA, wxDefaultPosition, wxSize(290, 70), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mIP1Ena->SetDefaultColSize(130);
    mIP1Ena->SetDefaultRowSize(20);
    mIP1Ena->SetColLabelSize(20);
    mIP1Ena->SetRowLabelSize(0);
    mIP1Ena->CreateGrid(1, 2, wxGrid::wxGridSelectCells);
    mSizerContainingGrids1->Add(mIP1Ena, 1, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText37 = new wxStaticText;
    itemStaticText37->Create( itemPanel33, wxID_STATIC, _("NOTE: If no address is specified above, all addresses\nare enabled, except for the addresses specified below."), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids1->Add(itemStaticText37, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText38 = new wxStaticText;
    itemStaticText38->Create( itemPanel33, wxID_STATIC, _("Disabled addresses:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids1->Add(itemStaticText38, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mIP1Dis = new wxGrid( itemPanel33, CD_IP_DIS, wxDefaultPosition, wxSize(290, 70), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mIP1Dis->SetDefaultColSize(130);
    mIP1Dis->SetDefaultRowSize(20);
    mIP1Dis->SetColLabelSize(20);
    mIP1Dis->SetRowLabelSize(0);
    mIP1Dis->CreateGrid(1, 2, wxGrid::wxGridSelectCells);
    mSizerContainingGrids1->Add(mIP1Dis, 1, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText40 = new wxStaticText;
    itemStaticText40->Create( itemPanel33, wxID_STATIC, _("NOTE: If mask is not specified, the exact IP address is used."), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids1->Add(itemStaticText40, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    mNotebook->AddPage(itemPanel33, _("IP Filtering for Tunnelling"));

    wxPanel* itemPanel41 = new wxPanel;
    itemPanel41->Create( mNotebook, CD_CONFTAB_IP2, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    mSizerContainingGrids2 = new wxBoxSizer(wxVERTICAL);
    itemPanel41->SetSizer(mSizerContainingGrids2);

    wxStaticText* itemStaticText43 = new wxStaticText;
    itemStaticText43->Create( itemPanel41, wxID_STATIC, _("Clients can connect to the server from the IP addresses:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids2->Add(itemStaticText43, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mIP2Ena = new wxGrid( itemPanel41, CD_IP_ENA, wxDefaultPosition, wxSize(290, 70), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mIP2Ena->SetDefaultColSize(130);
    mIP2Ena->SetDefaultRowSize(20);
    mIP2Ena->SetColLabelSize(20);
    mIP2Ena->SetRowLabelSize(0);
    mIP2Ena->CreateGrid(1, 2, wxGrid::wxGridSelectCells);
    mSizerContainingGrids2->Add(mIP2Ena, 1, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText45 = new wxStaticText;
    itemStaticText45->Create( itemPanel41, wxID_STATIC, _("NOTE: If no address is specified above, all addresses\nare enabled, except for the addresses specified below."), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids2->Add(itemStaticText45, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText46 = new wxStaticText;
    itemStaticText46->Create( itemPanel41, wxID_STATIC, _("Disabled addresses:"), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids2->Add(itemStaticText46, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mIP2Dis = new wxGrid( itemPanel41, CD_IP_DIS, wxDefaultPosition, wxSize(290, 70), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    mIP2Dis->SetDefaultColSize(130);
    mIP2Dis->SetDefaultRowSize(20);
    mIP2Dis->SetColLabelSize(20);
    mIP2Dis->SetRowLabelSize(0);
    mIP2Dis->CreateGrid(1, 2, wxGrid::wxGridSelectCells);
    mSizerContainingGrids2->Add(mIP2Dis, 1, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText48 = new wxStaticText;
    itemStaticText48->Create( itemPanel41, wxID_STATIC, _("NOTE: If mask is not specified, the exact IP address is used."), wxDefaultPosition, wxDefaultSize, 0 );
    mSizerContainingGrids2->Add(itemStaticText48, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    mNotebook->AddPage(itemPanel41, _("IP Filtering for Web"));

#if !wxCHECK_VERSION(2,5,2)
    itemBoxSizer2->Add(mNotebookSizer, 1, wxGROW|wxALL, 5);
#else
    itemBoxSizer2->Add(mNotebook, 1, wxGROW|wxALL, 5);
#endif

    wxBoxSizer* itemBoxSizer49 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer49, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton50 = new wxButton;
    itemButton50->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer49->Add(itemButton50, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton51 = new wxButton;
    itemButton51->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer49->Add(itemButton51, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton52 = new wxButton;
    itemButton52->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer49->Add(itemButton52, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ServerConfigurationDlg content construction
  //CreateGrid and grid methods -- must be removed for the first 3 grids, but not for IP enable/disable
  SetEscapeId(wxID_NONE);  // prevents the ESC in the grid from closing the dialog
  pg1->create(mSecurGrid);
  pg1->load_and_show();
  pg2->create(mOperGrid);
  pg2->load_and_show();
  pg3->create(mCompatGrid, new CompatPropertyGridTable(pg3));
  DefineColSize(mCompatGrid, 2, 200);
  wxGridCellAttr * catr = new wxGridCellAttr;
  catr->SetReadOnly();
  mCompatGrid->SetColAttr(2, catr);
  pg3->load_and_show();
 // load and show directories:
  char buf[MAX_PATH+1];
  int counter=1;
  do
  { if (cd_Get_property_value(cdp, sqlserver_owner, "Dir", counter, buf, sizeof(buf)) || !*buf)
      break;
    mDirsList->Append(wxString(buf, *wxConvCurrent));
    counter++;
  } while (true);
  mNewDir->Disable();
  mModifDir->Disable();
  mRemDir->Disable();
 // IP filtering:
  prep_and_load_IP(cdp, mSizerContainingGrids,  mIPEna,  "IP_enabled_addr",         "IP_enabled_mask");
  prep_and_load_IP(cdp, mSizerContainingGrids,  mIPDis,  "IP_disabled_addr",        "IP_disabled_mask");
  mIPEna->EnableEditing(conf_adm);  mIPDis->EnableEditing(conf_adm);
#if WBVERS>=1000
  prep_and_load_IP(cdp, mSizerContainingGrids1, mIP1Ena, "IP_tunnel_enabled_addr",  "IP_tunnel_enabled_mask");
  prep_and_load_IP(cdp, mSizerContainingGrids1, mIP1Dis, "IP_tunnel_disabled_addr", "IP_tunnel_disabled_mask");
  mIP1Ena->EnableEditing(conf_adm);  mIP1Dis->EnableEditing(conf_adm);
  prep_and_load_IP(cdp, mSizerContainingGrids2, mIP2Ena, "IP_web_enabled_addr",     "IP_web_enabled_mask");
  prep_and_load_IP(cdp, mSizerContainingGrids2, mIP2Dis, "IP_web_disabled_addr",    "IP_web_disabled_mask");
  mIP2Ena->EnableEditing(conf_adm);  mIP2Dis->EnableEditing(conf_adm);
#else
  mNotebook->RemovePage(4);
  mNotebook->RemovePage(5);
#endif
}

/*!
 * Should we show tooltips?
 */

bool ServerConfigurationDlg::ShowToolTips()
{
    return TRUE;
}

void ServerConfigurationDlg::save_IP(wxGrid * mIPEna, wxGrid * mIPDis, const char * enabled_addr, const char * enabled_mask, const char * disabled_addr, const char * disabled_mask)
{
  if (mIPEna->IsCellEditControlEnabled()) mIPEna->DisableCellEditControl(); // commit cell edit
  if (mIPDis->IsCellEditControlEnabled()) mIPDis->DisableCellEditControl(); // commit cell edit
  if (ip_changed) // addresses and masks counted from 1, terminated by the empty value
  { wxString str;  int counter;
    for (counter=0;  counter<mIPEna->GetNumberRows()-1;  counter++)
    { str = mIPEna->GetCellValue(counter, 0);
      if (cd_Set_property_value(cdp, sqlserver_owner, enabled_addr, counter+1, str.mb_str(*wxConvCurrent)))
        { cd_Signalize(cdp);  mNotebook->SetSelection(4);  return; }
      str = mIPEna->GetCellValue(counter, 1);
      if (cd_Set_property_value(cdp, sqlserver_owner, enabled_mask, counter+1, str.mb_str(*wxConvCurrent)))
        { cd_Signalize(cdp);  mNotebook->SetSelection(4);  return; }
    } 
    cd_Set_property_value(cdp, sqlserver_owner, enabled_addr, counter+1, "");  // delete others
    cd_Set_property_value(cdp, sqlserver_owner, enabled_mask, counter+1, "");  // delete others
    for (counter=0;  counter<mIPDis->GetNumberRows()-1;  counter++)
    { str = mIPDis->GetCellValue(counter, 0);
      if (cd_Set_property_value(cdp, sqlserver_owner, disabled_addr, counter+1, str.mb_str(*wxConvCurrent)))
        { cd_Signalize(cdp);  mNotebook->SetSelection(4);  return; }
      str = mIPDis->GetCellValue(counter, 1);
      if (cd_Set_property_value(cdp, sqlserver_owner, disabled_mask, counter+1, str.mb_str(*wxConvCurrent)))
        { cd_Signalize(cdp);  mNotebook->SetSelection(4);  return; }
    } 
    cd_Set_property_value(cdp, sqlserver_owner, disabled_addr, counter+1, "");  // delete others
    cd_Set_property_value(cdp, sqlserver_owner, disabled_mask, counter+1, "");  // delete others
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void ServerConfigurationDlg::OnOkClick( wxCommandEvent& event )
{
  if (!pg1->save()) { mNotebook->SetSelection(0);  return; }
  if (!pg2->save()) { mNotebook->SetSelection(1);  return; }
  if (!pg3->save()) { mNotebook->SetSelection(2);  return; }
 // DLL/SO directories:
  if (dirs_changed)
  { char buf[MAX_PATH+1];  int counter;
    for (counter=0;  counter<mDirsList->GetCount();  counter++)
    { strmaxcpy(buf, mDirsList->GetString(counter).mb_str(*wxConvCurrent), sizeof(buf));
      if (cd_Set_property_value(cdp, sqlserver_owner, "Dir", counter+1, buf))
        { cd_Signalize(cdp);  mNotebook->SetSelection(3);  return; }
    } 
    cd_Set_property_value(cdp, sqlserver_owner, "Dir", counter+1, "");  // deletes others
  }
 // IP filtering:
  save_IP(mIPEna,  mIPDis,  "IP_enabled_addr",        "IP_enabled_mask",        "IP_disabled_addr",        "IP_disabled_mask");
#if WBVERS>=1000
  save_IP(mIP1Ena, mIP1Dis, "IP_tunnel_enabled_addr", "IP_tunnel_enabled_mask", "IP_tunnel_disabled_addr", "IP_tunnel_disabled_mask");
  save_IP(mIP2Ena, mIP2Dis, "IP_web_enabled_addr",    "IP_web_enabled_mask",    "IP_web_disabled_addr",    "IP_web_disabled_mask");
#endif
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void ServerConfigurationDlg::OnCancelClick( wxCommandEvent& event )
{
    EndModal(0);
}


/*!
 * wxEVT_GRID_CELL_CHANGE event handler for CD_SECUR_GRID
 */

void ServerConfigurationDlg::OnCdSecurGridCellChange( wxGridEvent& event )
{
  if (event.GetId()==CD_SECUR_GRID)
    pg1->items[event.GetRow()]->changed=true;  
  event.Skip();
}


/*!
 * wxEVT_GRID_CELL_CHANGE event handler for CD_OPER_GRID
 */

void ServerConfigurationDlg::OnCdOperGridCellChange( wxGridEvent& event )
{
  if (event.GetId()==CD_OPER_GRID)
    pg2->items[event.GetRow()]->changed=true;  
  event.Skip();
}

/*!
 * wxEVT_GRID_CELL_CHANGE event handler for CD_COMPAT_GRID
 */

void ServerConfigurationDlg::OnCdCompatGridCellChange( wxGridEvent& event )
{
  if (event.GetId()==CD_COMPAT_GRID)
    pg3->items[event.GetRow()]->changed=true;  
  event.Skip();
}


/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for CD_DIRS_LIST
 */

void ServerConfigurationDlg::OnCdDirsListSelected( wxCommandEvent& event )
{
  int sel = mDirsList->GetSelection();
  if (sel!=wxNOT_FOUND)
  { mEdit->SetValue(mDirsList->GetString(sel));
    mRemDir->Enable(conf_adm && sel!=-1);
    mModifDir->Disable();
    mNewDir->Disable();
  }
}

/*!
 * wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for CD_DIRS_LIST
 */

void ServerConfigurationDlg::OnCdDirsListDoubleClicked( wxCommandEvent& event )
{
  //OnCdDirModifClick(event);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DIRS_NEW
 */

void ServerConfigurationDlg::OnCdDirsNewClick( wxCommandEvent& event )
// adding a directory
{
  wxString newdir = mEdit->GetValue();
  newdir.Trim(false);  newdir.Trim(true);
  if (newdir.IsEmpty()) wxBell();
  else
  { mDirsList->Append(newdir);
    dirs_changed=true;
    mEdit->SetValue(wxEmptyString);
    mModifDir->Disable();
    mNewDir->Disable();
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DIR_MODIF
 */

void ServerConfigurationDlg::OnCdDirModifClick( wxCommandEvent& event )
{ int sel = mDirsList->GetSelection();
  if (sel!=wxNOT_FOUND) 
  { wxString newdir = mEdit->GetValue();
    newdir.Trim(false);  newdir.Trim(true);
    if (newdir.IsEmpty()) wxBell();
    else
    { mDirsList->SetString(sel, newdir);
      dirs_changed=true;
      //mEdit->SetValue(wxEmptyString);
      mModifDir->Disable();
      mNewDir->Disable();
    }
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DIRS_REMOVE
 */

void ServerConfigurationDlg::OnCdDirsRemoveClick( wxCommandEvent& event )
{ int sel = mDirsList->GetSelection();
  if (sel!=wxNOT_FOUND) 
  { mDirsList->Delete(sel);
    mRemDir->Disable();
    dirs_changed=true;
  }
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_DIRS_EDIT
 */

void ServerConfigurationDlg::OnCdDirsEditUpdated( wxCommandEvent& event )
{
  mNewDir->Enable(conf_adm);
  mModifDir->Enable(conf_adm && mDirsList->GetSelection()!=wxNOT_FOUND);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DIRS_SEL
 */

void ServerConfigurationDlg::OnCdDirsSelClick( wxCommandEvent& event )
{
  if (!remote_operation_confirmed(cdp, this)) 
    return;
  wxString newdir = wxDirSelector(_("Choose a directory"), mEdit->GetValue(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!newdir.IsEmpty())
  { mEdit->SetValue(newdir);
    mModifDir->Enable(conf_adm && mDirsList->GetSelection()!=wxNOT_FOUND);
    mNewDir->Enable(conf_adm);
  }
}


/*!
 * wxEVT_GRID_CELL_CHANGE event handler for CD_IP_ENA
 */

static bool recursion_breaker = false;
#if 0
void ServerConfigurationDlg::OnCdIpEnaCellChange( wxGridEvent& event )
{ if (recursion_breaker) return;
  if (event.GetId()==CD_IP_ENA)
  { recursion_breaker=true;
    wxString val = mIPEna->GetCellValue(event.GetRow(), event.GetCol());
    val.Trim(true);  val.Trim(false);
    bool on_fict = event.GetRow()==mIPEna->GetNumberRows()-1;
    if (val.IsEmpty())
      { if (!on_fict && event.GetCol()==0) mIPEna->DeleteRows(event.GetRow(), 1); }
    else if (on_fict) mIPEna->AppendRows(1);
    ip_changed=true;
    recursion_breaker=false;
  }
  event.Skip();
}

/*!
 * wxEVT_GRID_CELL_CHANGE event handler for CD_IP_DIS
 */

void ServerConfigurationDlg::OnCdIpDisCellChange( wxGridEvent& event )
{ if (recursion_breaker) return;
  if (event.GetId()==CD_IP_DIS)
  { recursion_breaker=true;
    wxString val = mIPDis->GetCellValue(event.GetRow(), event.GetCol());
    val.Trim(true);  val.Trim(false);
    bool on_fict = event.GetRow()==mIPDis->GetNumberRows()-1;
    if (val.IsEmpty())
      { if (!on_fict && event.GetCol()==0) mIPDis->DeleteRows(event.GetRow(), 1); }
    else if (on_fict) mIPDis->AppendRows(1);
    ip_changed=true;
    recursion_breaker=false;
  }
  event.Skip();
}
#endif

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void ServerConfigurationDlg::OnHelpClick( wxCommandEvent& event )
{
  wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/syspar_rt.html"));
}



/*!
 * Get bitmap resources
 */

wxBitmap ServerConfigurationDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ServerConfigurationDlg bitmap retrieval
    return wxNullBitmap;
////@end ServerConfigurationDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ServerConfigurationDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ServerConfigurationDlg icon retrieval
    return wxNullIcon;
////@end ServerConfigurationDlg icon retrieval
}
/*!
 * wxEVT_GRID_CELL_CHANGE event handler for CD_IP_ENA
 */

void ServerConfigurationDlg::OnCellChange( wxGridEvent& event )
{ 
  if (event.GetId()==CD_IP_ENA || event.GetId()==CD_IP_DIS)
  { if (recursion_breaker) return;
    recursion_breaker=true;
    wxGrid * grid = (wxGrid*)event.GetEventObject();
    wxString val = grid->GetCellValue(event.GetRow(), event.GetCol());
    val.Trim(true);  val.Trim(false);
    bool on_fict = event.GetRow()==grid->GetNumberRows()-1;
    if (val.IsEmpty())
      { if (!on_fict && event.GetCol()==0) grid->DeleteRows(event.GetRow(), 1); }
    else if (on_fict) grid->AppendRows(1);
    ip_changed=true;
    recursion_breaker=false;
  }
  event.Skip();
}


