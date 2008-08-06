/////////////////////////////////////////////////////////////////////////////
// Name:        DatabaseDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/13/04 11:39:21
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "DatabaseDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#include "wx/dir.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "DatabaseDlg.h"
#if 0  // obsolete version
#include "DuplFil.h"
#endif
////@begin XPM images
////@end XPM images

/*!
 * DatabaseDlg type definition
 */

IMPLEMENT_CLASS( DatabaseDlg, wxDialog )

/*!
 * DatabaseDlg event table definition
 */

BEGIN_EVENT_TABLE( DatabaseDlg, wxDialog )

////@begin DatabaseDlg event table entries
    EVT_BUTTON( CD_FD_SEL2, DatabaseDlg::OnCdFdSel2Click )

    EVT_BUTTON( CD_FD_SEL3, DatabaseDlg::OnCdFdSel3Click )

    EVT_BUTTON( CD_FD_SEL4, DatabaseDlg::OnCdFdSel4Click )

    EVT_BUTTON( CD_FD_SELTRANS, DatabaseDlg::OnCdFdSeltransClick )

    EVT_BUTTON( CD_FD_SELLOG, DatabaseDlg::OnCdFdSellogClick )

    EVT_TEXT( CD_RECORVERY_DIR, DatabaseDlg::OnCdRecorveryDirUpdated )

    EVT_BUTTON( CD_RECORVERY_DIRSEL, DatabaseDlg::OnCdRecorveryDirselClick )

    EVT_LISTBOX( CD_RECORVERY_LIST, DatabaseDlg::OnCdRecorveryListSelected )

    EVT_BUTTON( CD_RECORVERY_START, DatabaseDlg::OnCdRecorveryStartClick )

    EVT_BUTTON( CD_PATCH_START, DatabaseDlg::OnCdPatchStartClick )

    EVT_BUTTON( CD_ERASE_START, DatabaseDlg::OnCdEraseStartClick )

    EVT_TEXT( CD_ED_APPLS, DatabaseDlg::OnCdEdApplsUpdated )

    EVT_BUTTON( CD_ED_APPLS, DatabaseDlg::OnCdEdApplsClick )

    EVT_TEXT( CD_ED_BACK_FOLDER, DatabaseDlg::OnCdEdBackFolderUpdated )

    EVT_BUTTON( CD_ED_BUTBACKFOLDER, DatabaseDlg::OnCdEdButbackfolderClick )

    EVT_BUTTON( CD_ED_DUPLFIL, DatabaseDlg::OnCdEdDuplfilClick )

    EVT_BUTTON( CD_SERVICE_UNREGISTER, DatabaseDlg::OnCdServiceUnregisterClick )

    EVT_TEXT( CD_SERVICE_NAME, DatabaseDlg::OnCdServiceNameUpdated )

    EVT_BUTTON( CD_SERVICE_REGISTER, DatabaseDlg::OnCdServiceRegisterClick )

    EVT_BUTTON( CD_SERVICE_START, DatabaseDlg::OnCdServiceStartClick )

    EVT_BUTTON( CD_SERVICE_STOP, DatabaseDlg::OnCdServiceStopClick )

    EVT_BUTTON( wxID_OK, DatabaseDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, DatabaseDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, DatabaseDlg::OnHelpClick )

////@end DatabaseDlg event table entries

END_EVENT_TABLE()

/*!
 * DatabaseDlg constructors
 */

DatabaseDlg::DatabaseDlg(tobjname server_nameIn)
{ strcpy(server_name, server_nameIn); mApplsChanged = false;}

/*!
 * DatabaseDlg creator
 */

bool DatabaseDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DatabaseDlg member initialisation
    mPart1 = NULL;
    mLim1 = NULL;
    mPart2 = NULL;
    mSel2 = NULL;
    mLim2 = NULL;
    mPart3 = NULL;
    mSel3 = NULL;
    mLim3 = NULL;
    mPart4 = NULL;
    mSel4 = NULL;
    mTrans = NULL;
    mSelTrans = NULL;
    mLog = NULL;
    mSelLog = NULL;
    mServerLang = NULL;
    mRecovDir = NULL;
    mRecovDirSel = NULL;
    mRecovList = NULL;
    mRestoreNow = NULL;
    EDAppls = NULL;
    Appls = NULL;
    EDBackFolder = NULL;
    Butbackfolder = NULL;
    Duplfil = NULL;
    mPanelSizer = NULL;
    mServiceBox = NULL;
    mOldName = NULL;
    mUnregister = NULL;
    mServiceName = NULL;
    mRegister = NULL;
    mAutoStart = NULL;
    mState = NULL;
    mStart = NULL;
    mStop = NULL;
////@end DatabaseDlg member initialisation

////@begin DatabaseDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end DatabaseDlg creation
    return TRUE;
}

/*!
 * Control creation for DatabaseDlg
 */

void DatabaseDlg::CreateControls()
{    
////@begin DatabaseDlg content construction
    DatabaseDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxNotebook* itemNotebook3 = new wxNotebook;
    itemNotebook3->Create( itemDialog1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );
#if !wxCHECK_VERSION(2,5,2)
    wxNotebookSizer* itemNotebook3Sizer = new wxNotebookSizer(itemNotebook3);
#endif

    wxPanel* itemPanel4 = new wxPanel;
    itemPanel4->Create( itemNotebook3, ID_PANEL, wxDefaultPosition, wxSize(100, 80), wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemPanel4->SetSizer(itemBoxSizer5);

    wxStaticBox* itemStaticBoxSizer6Static = new wxStaticBox(itemPanel4, wxID_ANY, _("Database file"));
    wxStaticBoxSizer* itemStaticBoxSizer6 = new wxStaticBoxSizer(itemStaticBoxSizer6Static, wxVERTICAL);
    itemBoxSizer5->Add(itemStaticBoxSizer6, 0, wxGROW|wxALL, 5);
    wxFlexGridSizer* itemFlexGridSizer7 = new wxFlexGridSizer(5, 4, 0, 0);
    itemFlexGridSizer7->AddGrowableCol(1);
    itemStaticBoxSizer6->Add(itemFlexGridSizer7, 0, wxGROW|wxALL, 0);
    wxStaticText* itemStaticText8 = new wxStaticText;
    itemStaticText8->Create( itemPanel4, wxID_STATIC, _("Part:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText8, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText9 = new wxStaticText;
    itemStaticText9->Create( itemPanel4, wxID_STATIC, _("Directory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    itemFlexGridSizer7->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText11 = new wxStaticText;
    itemStaticText11->Create( itemPanel4, wxID_STATIC, _("Max. size (MB):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText11, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText12 = new wxStaticText;
    itemStaticText12->Create( itemPanel4, wxID_STATIC, _("Basic:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mPart1 = new wxTextCtrl;
    mPart1->Create( itemPanel4, CD_FD_PART1, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    itemFlexGridSizer7->Add(mPart1, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer7->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mLim1 = new wxTextCtrl;
    mLim1->Create( itemPanel4, CD_FD_LIM1, _T(""), wxDefaultPosition, wxSize(70, -1), 0 );
    itemFlexGridSizer7->Add(mLim1, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText16 = new wxStaticText;
    itemStaticText16->Create( itemPanel4, wxID_STATIC, _("2nd:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText16, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mPart2 = new wxTextCtrl;
    mPart2->Create( itemPanel4, CD_FD_PART2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(mPart2, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mSel2 = new wxButton;
    mSel2->Create( itemPanel4, CD_FD_SEL2, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer7->Add(mSel2, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

    mLim2 = new wxTextCtrl;
    mLim2->Create( itemPanel4, CD_FD_LIM2, _T(""), wxDefaultPosition, wxSize(70, -1), 0 );
    itemFlexGridSizer7->Add(mLim2, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText20 = new wxStaticText;
    itemStaticText20->Create( itemPanel4, wxID_STATIC, _("3rd:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText20, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mPart3 = new wxTextCtrl;
    mPart3->Create( itemPanel4, CD_FD_PART3, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(mPart3, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mSel3 = new wxButton;
    mSel3->Create( itemPanel4, CD_FD_SEL3, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer7->Add(mSel3, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

    mLim3 = new wxTextCtrl;
    mLim3->Create( itemPanel4, CD_FD_LIM3, _T(""), wxDefaultPosition, wxSize(70, -1), 0 );
    itemFlexGridSizer7->Add(mLim3, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText24 = new wxStaticText;
    itemStaticText24->Create( itemPanel4, wxID_STATIC, _("4th:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText24, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mPart4 = new wxTextCtrl;
    mPart4->Create( itemPanel4, CD_FD_PART4, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(mPart4, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mSel4 = new wxButton;
    mSel4->Create( itemPanel4, CD_FD_SEL4, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer7->Add(mSel4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

    itemFlexGridSizer7->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer28Static = new wxStaticBox(itemPanel4, wxID_ANY, _("Other files"));
    wxStaticBoxSizer* itemStaticBoxSizer28 = new wxStaticBoxSizer(itemStaticBoxSizer28Static, wxVERTICAL);
    itemBoxSizer5->Add(itemStaticBoxSizer28, 0, wxGROW|wxALL, 5);
    wxFlexGridSizer* itemFlexGridSizer29 = new wxFlexGridSizer(2, 3, 0, 0);
    itemFlexGridSizer29->AddGrowableCol(1);
    itemStaticBoxSizer28->Add(itemFlexGridSizer29, 1, wxGROW|wxALL, 0);
    wxStaticText* itemStaticText30 = new wxStaticText;
    itemStaticText30->Create( itemPanel4, wxID_STATIC, _("Transaction file:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(itemStaticText30, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mTrans = new wxTextCtrl;
    mTrans->Create( itemPanel4, CD_FD_TRANS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(mTrans, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mSelTrans = new wxButton;
    mSelTrans->Create( itemPanel4, CD_FD_SELTRANS, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer29->Add(mSelTrans, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText33 = new wxStaticText;
    itemStaticText33->Create( itemPanel4, wxID_STATIC, _("Basic log:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(itemStaticText33, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mLog = new wxTextCtrl;
    mLog->Create( itemPanel4, CD_FD_LOG, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(mLog, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mSelLog = new wxButton;
    mSelLog->Create( itemPanel4, CD_FD_SELLOG, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer29->Add(mSelLog, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

    wxStaticBox* itemStaticBoxSizer36Static = new wxStaticBox(itemPanel4, wxID_ANY, _("Language for Server Messages"));
    wxStaticBoxSizer* itemStaticBoxSizer36 = new wxStaticBoxSizer(itemStaticBoxSizer36Static, wxHORIZONTAL);
    itemBoxSizer5->Add(itemStaticBoxSizer36, 0, wxGROW|wxALL, 5);
    wxStaticText* itemStaticText37 = new wxStaticText;
    itemStaticText37->Create( itemPanel4, wxID_STATIC, _("Language for Server Console and Log Files:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer36->Add(itemStaticText37, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* mServerLangStrings = NULL;
    mServerLang = new wxOwnerDrawnComboBox;
    mServerLang->Create( itemPanel4, CD_FD_SERVER_LANG, _T(""), wxDefaultPosition, wxDefaultSize, 0, mServerLangStrings, wxCB_READONLY );
    if (ShowToolTips())
        mServerLang->SetToolTip(_("The SQL Server will only use this language if the corresponding .mo file is installed."));
    itemStaticBoxSizer36->Add(mServerLang, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemNotebook3->AddPage(itemPanel4, _("Files"));

    wxPanel* itemPanel39 = new wxPanel;
    itemPanel39->Create( itemNotebook3, ID_PANEL1, wxDefaultPosition, wxSize(100, 80), wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxVERTICAL);
    itemPanel39->SetSizer(itemBoxSizer40);

    wxStaticBox* itemStaticBoxSizer41Static = new wxStaticBox(itemPanel39, wxID_ANY, _("Recover database from a backup copy"));
    wxStaticBoxSizer* itemStaticBoxSizer41 = new wxStaticBoxSizer(itemStaticBoxSizer41Static, wxVERTICAL);
    itemBoxSizer40->Add(itemStaticBoxSizer41, 1, wxGROW|wxALL, 5);
    wxBoxSizer* itemBoxSizer42 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer41->Add(itemBoxSizer42, 0, wxGROW|wxALL, 0);
    wxStaticText* itemStaticText43 = new wxStaticText;
    itemStaticText43->Create( itemPanel39, wxID_STATIC, _("Backup directory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer42->Add(itemStaticText43, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mRecovDir = new wxTextCtrl;
    mRecovDir->Create( itemPanel39, CD_RECORVERY_DIR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer42->Add(mRecovDir, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mRecovDirSel = new wxButton;
    mRecovDirSel->Create( itemPanel39, CD_RECORVERY_DIRSEL, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemBoxSizer42->Add(mRecovDirSel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer46 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer41->Add(itemBoxSizer46, 1, wxGROW|wxALL, 0);
    itemBoxSizer46->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxString* mRecovListStrings = NULL;
    mRecovList = new wxListBox;
    mRecovList->Create( itemPanel39, CD_RECORVERY_LIST, wxDefaultPosition, wxSize(180, 100), 0, mRecovListStrings, wxLB_SINGLE|wxLB_ALWAYS_SB );
    itemBoxSizer46->Add(mRecovList, 0, wxGROW|wxALL, 5);

    mRestoreNow = new wxButton;
    mRestoreNow->Create( itemPanel39, CD_RECORVERY_START, _("Restore"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer46->Add(mRestoreNow, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer50Static = new wxStaticBox(itemPanel39, wxID_ANY, _("Patch a damaged database"));
    wxStaticBoxSizer* itemStaticBoxSizer50 = new wxStaticBoxSizer(itemStaticBoxSizer50Static, wxHORIZONTAL);
    itemBoxSizer40->Add(itemStaticBoxSizer50, 0, wxGROW|wxALL, 5);
    wxStaticText* itemStaticText51 = new wxStaticText;
    itemStaticText51->Create( itemPanel39, wxID_STATIC, _("This function will patch a damaged database so the server can be started."), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer50->Add(itemStaticText51, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxButton* itemButton52 = new wxButton;
    itemButton52->Create( itemPanel39, CD_PATCH_START, _("Patch"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer50->Add(itemButton52, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer53Static = new wxStaticBox(itemPanel39, wxID_ANY, _("Erase the database"));
    wxStaticBoxSizer* itemStaticBoxSizer53 = new wxStaticBoxSizer(itemStaticBoxSizer53Static, wxHORIZONTAL);
    itemBoxSizer40->Add(itemStaticBoxSizer53, 0, wxGROW|wxALL, 5);
    wxStaticText* itemStaticText54 = new wxStaticText;
    itemStaticText54->Create( itemPanel39, wxID_STATIC, _("This function will erase all data in the database."), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer53->Add(itemStaticText54, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxButton* itemButton55 = new wxButton;
    itemButton55->Create( itemPanel39, CD_ERASE_START, _("Erase"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer53->Add(itemButton55, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer56Static = new wxStaticBox(itemPanel39, wxID_ANY, _("Regenerate database file"));
    wxStaticBoxSizer* itemStaticBoxSizer56 = new wxStaticBoxSizer(itemStaticBoxSizer56Static, wxVERTICAL);
    itemBoxSizer40->Add(itemStaticBoxSizer56, 0, wxGROW|wxALL, 5);
    wxStaticText* itemStaticText57 = new wxStaticText;
    itemStaticText57->Create( itemPanel39, wxID_STATIC, _("This function will regenerate the database. It will attempt to export the data from a damaged database \ninto a new database file. It does this by exporting all data, rename the orignal database file, create\na new database file with the original name and then import all original data."), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer56->Add(itemStaticText57, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    wxFlexGridSizer* itemFlexGridSizer58 = new wxFlexGridSizer(2, 3, 0, 0);
    itemFlexGridSizer58->AddGrowableCol(1);
    itemStaticBoxSizer56->Add(itemFlexGridSizer58, 0, wxGROW|wxALL, 0);
    wxStaticText* itemStaticText59 = new wxStaticText;
    itemStaticText59->Create( itemPanel39, wxID_STATIC, _("Folder for export of applications:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer58->Add(itemStaticText59, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    EDAppls = new wxTextCtrl;
    EDAppls->Create( itemPanel39, CD_ED_APPLS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer58->Add(EDAppls, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 5);

    Appls = new wxButton;
    Appls->Create( itemPanel39, CD_ED_APPLS, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer58->Add(Appls, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText62 = new wxStaticText;
    itemStaticText62->Create( itemPanel39, wxID_STATIC, _("Backup database folder:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer58->Add(itemStaticText62, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    EDBackFolder = new wxTextCtrl;
    EDBackFolder->Create( itemPanel39, CD_ED_BACK_FOLDER, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer58->Add(EDBackFolder, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM, 5);

    Butbackfolder = new wxButton;
    Butbackfolder->Create( itemPanel39, CD_ED_BUTBACKFOLDER, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    itemFlexGridSizer58->Add(Butbackfolder, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    Duplfil = new wxButton;
    Duplfil->Create( itemPanel39, CD_ED_DUPLFIL, _("Regenerate"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer56->Add(Duplfil, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxTOP, 5);

    itemNotebook3->AddPage(itemPanel39, _("Management"));

    wxPanel* itemPanel66 = new wxPanel;
    itemPanel66->Create( itemNotebook3, ID_PANEL3, wxDefaultPosition, wxSize(100, 80), wxTAB_TRAVERSAL );
    mPanelSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel66->SetSizer(mPanelSizer);

    mServiceBox = new wxStaticBox(itemPanel66, wxID_ANY, _("Windows service"));
    wxStaticBoxSizer* itemStaticBoxSizer68 = new wxStaticBoxSizer(mServiceBox, wxVERTICAL);
    mPanelSizer->Add(itemStaticBoxSizer68, 0, wxGROW|wxALL, 5);
    wxFlexGridSizer* itemFlexGridSizer69 = new wxFlexGridSizer(2, 3, 0, 0);
    itemFlexGridSizer69->AddGrowableCol(1);
    itemStaticBoxSizer68->Add(itemFlexGridSizer69, 0, wxGROW|wxALL, 0);
    wxStaticText* itemStaticText70 = new wxStaticText;
    itemStaticText70->Create( itemPanel66, wxID_STATIC, _("Current service name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer69->Add(itemStaticText70, 0, wxALIGN_LEFT|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 5);

    mOldName = new wxTextCtrl;
    mOldName->Create( itemPanel66, CD_SERVICE_OLD_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer69->Add(mOldName, 1, wxGROW|wxGROW|wxALL, 5);

    mUnregister = new wxButton;
    mUnregister->Create( itemPanel66, CD_SERVICE_UNREGISTER, _("Unregister the service"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer69->Add(mUnregister, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText73 = new wxStaticText;
    itemStaticText73->Create( itemPanel66, wxID_STATIC, _("New service name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer69->Add(itemStaticText73, 0, wxALIGN_LEFT|wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 5);

    mServiceName = new wxTextCtrl;
    mServiceName->Create( itemPanel66, CD_SERVICE_NAME, _T(""), wxDefaultPosition, wxSize(150, 20), 0 );
    itemFlexGridSizer69->Add(mServiceName, 1, wxGROW|wxGROW|wxALL, 5);

    mRegister = new wxButton;
    mRegister->Create( itemPanel66, CD_SERVICE_REGISTER, _("Register the service"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer69->Add(mRegister, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mAutoStart = new wxCheckBox;
    mAutoStart->Create( itemPanel66, CD_SERVICE_AUTOSTART, _("Start the SQL Server on system startup"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mAutoStart->SetValue(FALSE);
    mPanelSizer->Add(mAutoStart, 0, wxALIGN_LEFT|wxALL, 5);

    mPanelSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    mState = new wxStaticText;
    mState->Create( itemPanel66, CD_SERVICE_STATE, _("The service / daemon is NOT running."), wxDefaultPosition, wxDefaultSize, 0 );
    mPanelSizer->Add(mState, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer79 = new wxBoxSizer(wxHORIZONTAL);
    mPanelSizer->Add(itemBoxSizer79, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);
    mStart = new wxButton;
    mStart->Create( itemPanel66, CD_SERVICE_START, _("Start"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer79->Add(mStart, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mStop = new wxButton;
    mStop->Create( itemPanel66, CD_SERVICE_STOP, _("Stop"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer79->Add(mStop, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemNotebook3->AddPage(itemPanel66, _("Service / Daemon"));

#if !wxCHECK_VERSION(2,5,2)
    itemBoxSizer2->Add(itemNotebook3Sizer, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);
#else
    itemBoxSizer2->Add(itemNotebook3, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);
#endif

    wxBoxSizer* itemBoxSizer82 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer82, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton83 = new wxButton;
    itemButton83->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton83->SetDefault();
    itemBoxSizer82->Add(itemButton83, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton84 = new wxButton;
    itemButton84->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer82->Add(itemButton84, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton85 = new wxButton;
    itemButton85->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer82->Add(itemButton85, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end DatabaseDlg content construction
 // set limits:
  mPart1->SetMaxLength(MAX_PATH);
  mPart2->SetMaxLength(MAX_PATH);
  mPart3->SetMaxLength(MAX_PATH);
  mPart4->SetMaxLength(MAX_PATH);
  mLog  ->SetMaxLength(MAX_PATH);
  mTrans->SetMaxLength(MAX_PATH);
 // title:
  wxString title;
  title.Printf(_("Database Management for '%s'"), wxString(server_name, *wxConvCurrent).c_str());
  SetTitle(title);
 // private or shared?
  t_avail_server * as = find_server_connection_info(server_name);
  private_server = as && as->private_server;
 // show database file data:
  char direc[MAX_PATH+1];  sig32 lim;
  GetPrimaryPath(server_name, private_server, direc);
  mPart1->SetValue(wxString(direc, *wxConvCurrent));
  *direc=0;  GetDatabaseString(server_name, "Path2", direc, sizeof(direc), private_server);
  mPart2->SetValue(wxString(direc, *wxConvCurrent));
  *direc=0;  GetDatabaseString(server_name, "Path3", direc, sizeof(direc), private_server);
  mPart3->SetValue(wxString(direc, *wxConvCurrent));
  *direc=0;  GetDatabaseString(server_name, "Path4", direc, sizeof(direc), private_server);
  mPart4->SetValue(wxString(direc, *wxConvCurrent));
  lim=-1;  GetDatabaseNum(server_name, "Limit1", lim, private_server);
  if (lim!=-1) SetIntValue(mLim1, lim);
  lim=-1;  GetDatabaseNum(server_name, "Limit2", lim, private_server);
  if (lim!=-1) SetIntValue(mLim2, lim);
  lim=-1;  GetDatabaseNum(server_name, "Limit3", lim, private_server);
  if (lim!=-1) SetIntValue(mLim3, lim);
  *direc=0;  GetDatabaseString(server_name, "TransactPath", direc, sizeof(direc), private_server);
  mTrans->SetValue(wxString(direc, *wxConvCurrent));
  *direc=0;  GetDatabaseString(server_name, "LogPath",      direc, sizeof(direc), private_server);
  mLog  ->SetValue(wxString(direc, *wxConvCurrent));
 // language:
  mServerLang->Append(_("- Platform default - "), (void*)wxLANGUAGE_DEFAULT);
  mServerLang->Append(_("English"),             (void*)wxLANGUAGE_ENGLISH);
  mServerLang->Append(_("Czech"),               (void*)wxLANGUAGE_CZECH);
  mServerLang->Append(_("Slovak"),              (void*)wxLANGUAGE_SLOVAK);
  mServerLang->Append(_("Polish"),              (void*)wxLANGUAGE_POLISH);
  mServerLang->Append(_("French"),              (void*)wxLANGUAGE_FRENCH);
  mServerLang->Append(_("German"),              (void*)wxLANGUAGE_GERMAN);
  mServerLang->Append(_("Italian"),             (void*)wxLANGUAGE_ITALIAN);
  *direc=0;  GetDatabaseString(server_name, "LanguageOfMessages", direc, sizeof(direc), private_server);
  direc[2]=0;  int sel = 0;
  if (!strcmp(direc, "en")) sel=1;  else
  if (!strcmp(direc, "cs")) sel=2;  else
  if (!strcmp(direc, "sk")) sel=3;  else
  if (!strcmp(direc, "pl")) sel=4;  else
  if (!strcmp(direc, "fr")) sel=5;  else
  if (!strcmp(direc, "de")) sel=6;  else
  if (!strcmp(direc, "it")) sel=7;
  mServerLang->SetSelection(sel);

 // management:
  mRecovDir->SetValue(read_profile_string("Directories", "Restore Path"));
  mRestoreNow->Disable();
 // service/daemon:
  mOldName->Disable();
  mServiceName->SetMaxLength(MAX_SERVICE_NAME);
#ifndef WINS  // for MS Windows only
  mServiceBox->Hide();
  mPanelSizer->Hide(itemStaticBoxSizer68);  // unstable, must be the the sizer below mServiceBox
#endif
  wxString service_name = FindRegisteredServiceForServer();
  show_service_register_state(service_name);  // shows the run state, too
  if (!service_name.IsEmpty())
    mAutoStart->SetValue(get_autostart(service_name));
  
  Duplfil->Enable(false);
}

/*!
 * Should we show tooltips?
 */

bool DatabaseDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FD_SEL2
 */

void DatabaseDlg::OnCdFdSel2Click( wxCommandEvent& event )
{ wxString str = wxDirSelector(_("Select Directory"), mPart2->GetValue().c_str(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!str.IsEmpty()) mPart2->SetValue(str);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FD_SEL3
 */

void DatabaseDlg::OnCdFdSel3Click( wxCommandEvent& event )
{ wxString str = wxDirSelector(_("Select Directory"), mPart3->GetValue().c_str(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!str.IsEmpty()) mPart3->SetValue(str);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FD_SEL4
 */

void DatabaseDlg::OnCdFdSel4Click( wxCommandEvent& event )
{ wxString str = wxDirSelector(_("Select Directory"), mPart4->GetValue().c_str(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!str.IsEmpty()) mPart4->SetValue(str);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FD_SELTRANS
 */

void DatabaseDlg::OnCdFdSeltransClick( wxCommandEvent& event )
{ wxString str = wxDirSelector(_("Select Directory"), mTrans->GetValue().c_str(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!str.IsEmpty()) mTrans->SetValue(str);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FD_SELLOG
 */

void DatabaseDlg::OnCdFdSellogClick( wxCommandEvent& event )
{ wxString str = wxDirSelector(_("Select Directory"), mLog->GetValue().c_str(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!str.IsEmpty()) mLog->SetValue(str);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DatabaseDlg::OnOkClick( wxCommandEvent& event )
{ unsigned long val;  wxString str;  bool ok=true;
  str=mLim1->GetValue();  str.Trim(false);  str.Trim(true);
  if (str.IsEmpty()) ok&=DeleteDatabaseValue(server_name, "Limit1", private_server);
  else if (str.ToULong(&val)) ok&=SetDatabaseNum(server_name, "Limit1", val, private_server);
  else { mLim1->SetFocus();  wxBell();  return; }
  str=mLim2->GetValue();  str.Trim(false);  str.Trim(true);
  if (str.IsEmpty()) ok&=DeleteDatabaseValue(server_name, "Limit2", private_server);
  else if (str.ToULong(&val)) ok&=SetDatabaseNum(server_name, "Limit2", val, private_server);
  else { mLim2->SetFocus();  wxBell();  return; }
  str=mLim3->GetValue();  str.Trim(false);  str.Trim(true);
  if (str.IsEmpty()) ok&=DeleteDatabaseValue(server_name, "Limit3", private_server);
  else if (str.ToULong(&val)) ok&=SetDatabaseNum(server_name, "Limit3", val, private_server);
  else { mLim3->SetFocus();  wxBell();  return; }

  str=mPart2->GetValue();  str.Trim(false);  str.Trim(true);
  if (!str.IsEmpty()) ok&=SetDatabaseString(server_name, "Path2", str.mb_str(*wxConvCurrent), private_server);
  else ok&=DeleteDatabaseValue(server_name, "Path2", private_server);
  str=mPart3->GetValue();  str.Trim(false);  str.Trim(true);
  if (!str.IsEmpty()) ok&=SetDatabaseString(server_name, "Path3", str.mb_str(*wxConvCurrent), private_server);
  else ok&=DeleteDatabaseValue(server_name, "Path3", private_server);
  str=mPart4->GetValue();  str.Trim(false);  str.Trim(true);
  if (!str.IsEmpty()) ok&=SetDatabaseString(server_name, "Path4", str.mb_str(*wxConvCurrent), private_server);
  else ok&=DeleteDatabaseValue(server_name, "Path4", private_server);

  str=mTrans->GetValue();  str.Trim(false);  str.Trim(true);
  if (!str.IsEmpty()) ok&=SetDatabaseString(server_name, "TransactPath", str.mb_str(*wxConvCurrent), private_server);
  else ok&=DeleteDatabaseValue(server_name, "TransactPath", private_server);
  str=mLog  ->GetValue();  str.Trim(false);  str.Trim(true);
  if (!str.IsEmpty()) ok&=SetDatabaseString(server_name, "LogPath", str.mb_str(*wxConvCurrent), private_server);
  else ok&=DeleteDatabaseValue(server_name, "LogPath", private_server);

 // language:
  int sel = mServerLang->GetSelection();
  if (!sel) ok&=DeleteDatabaseValue(server_name, "LanguageOfMessages", private_server);
  else
  { char lang[2+1+2+1];
    strcpy(lang, sel==1 ? "en_EN" : sel==2 ? "cs_CZ" : sel==3 ? "sk_SK" : sel==4 ? "pl_PL" : sel==5 ? "fr_FR" : sel==6 ? "de_DE" : /*sel==7 ?*/ "it_IT");
    ok&=SetDatabaseString(server_name, "LanguageOfMessages", lang, private_server);
  }
 // service/daemon:
  wxString service_name = FindRegisteredServiceForServer();
  if (!service_name.IsEmpty())
    if (!set_autostart(service_name, mAutoStart->GetValue()))
      { error_box(_("Error configuring the service."));  return; }

  if (!ok) 
    { error_box(_("Error writing to the registry."));  return; }
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DatabaseDlg::OnCancelClick( wxCommandEvent& event )
{ event.Skip(); }


void DatabaseDlg::list_backups(void)
{ mRecovList->Clear();
  int count=0;
  wxString dir = mRecovDir->GetValue();
  dir.Trim(false);  dir.Trim(true);
  if (dir.IsEmpty() || !wxDirExists(dir)) return;  // not using the "current" dir as default
  wxString f = wxFindFirstFile(dir+wxT(PATH_SEPARATOR_STR)+wxT("*.*"));
  while (!f.IsEmpty())  // looking for A and B parts only, and for Z archives
  { unsigned i=(unsigned)f.Length();
    while (i && f[i]!=PATH_SEPARATOR) i--;
    f=f.Mid(i+1);
    if (f.Length()==13 && f[0u]=='1') f=f.Mid(1);
    if (f.Length()==12)
      if (f[0u]>='0' && f[0u]<='9' && f[1u]>='0' && f[1u]<='9' && f[2u]>='0' && f[2u]<='9' && f[3u]>='0' && f[3u]<='9' &&
          f[4u]>='0' && f[4u]<='9' && f[5u]>='0' && f[5u]<='9' && f[6u]>='0' && f[6u]<='9' && f[7u]>='0' && f[7u]<='9' &&
          f[8u]=='.' && 
          f[9u]>='0' && f[9u]<='9' && f[10u]>='0' && f[10u]<='9')
      if (f[11u]>='A' && f[11u]<='B' || f[11u]=='z' || f[11u]=='Z') // z and Z
      { unsigned data, min, hour, day, month, year;  wxString date;
        year = 10*(f[0u]-'0') + f[1u]-'0';
        month= 10*(f[2u]-'0') + f[3u]-'0';
        day  = 10*(f[4u]-'0') + f[5u]-'0';
        hour = 10*(f[6u]-'0') + f[7u]-'0';
        min  = 10*(f[9u]-'0') + f[10u]-'0';
        if (min<60 && hour<24 && day && day<=31 && month && month<=12)
        { data = min + 60*(hour + 24*((day-1) + 31*((month-1) + 12*year)));
          date.Printf(wxT("%u.%u.%u  %02u:%02u"), day, month, year==99 ? year+1900 : year+2000, hour, min);
         // look for entry with the same date:
          bool found=false;  int ind;
          for (ind=0;  ind<count && !found;  ind++)
            if (data==(unsigned)(size_t)mRecovList->GetClientData(ind)) 
              { found=true;  break; }
          if (!found)
          { mRecovList->Append(date, (void*)(size_t)data);
            count++;
          }
        }
      }
    f = wxFindNextFile();
  }
  mRestoreNow->Disable();
  if (mRecovList->GetCount() > 0)  // last successfull path
    write_profile_string("Directories", "Restore Path", dir.mb_str(*wxConvCurrent));
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_RECORVERY_DIRSEL
 */

void DatabaseDlg::OnCdRecorveryDirselClick( wxCommandEvent& event )
{ wxString str = wxDirSelector(_("Select Directory"), mRecovDir->GetValue().c_str(), 0, wxDefaultPosition, this);
  if (!str.IsEmpty()) 
  { mRecovDir->SetValue(str);
    list_backups();
  }
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for CD_RECORVERY_LIST
 */

void DatabaseDlg::OnCdRecorveryListSelected( wxCommandEvent& event )
{
  mRestoreNow->Enable();
  event.Skip();
}

#ifdef LINUX
#define NO_ASM
#define NO_MEMORYMAPPEDFILES
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <general.h>
#include <sys/file.h>
#include <sys/stat.h>
#endif

#define ZIP602_STATIC
#include "Zip602.h"

#if NEVER // to je zcela spatne. Soubory z zipu musim parovat s castmi DB souboru podle jmena, ne podle adresare
bool unzip_files(char * zipfile, const char * server_name, bool must_rename)
// Restores backup files from the ZIP. Returns true on error.
{ int len, zlen;
  char keyname[10];  int part;  char fil_direc[MAX_PATH], filename[MAX_PATH];
 // check the location of fil parts:
  { /*CPINDUnZipInfo*/ CUnZipInfo uzi;
    if (!uzi.Open(zipfile)) { wxBell();  return true; }
    for (LPZIPENTRYINFO zei = uzi.GetFirstFile();  zei!=NULL;  zei = uzi.GetNextFile())
    { zei->GetFileName(filename);
      zlen=strlen(filename);  while (zlen && filename[zlen-1]!=PATH_SEPARATOR) zlen--;
      part=1;
      do
      { sprintf(keyname, "Path%u", part);
        *fil_direc=0;
        GetDatabaseString(server_name, keyname, fil_direc, sizeof(fil_direc));
        if (!*fil_direc && part==1) GetDatabaseString(server_name, "Path", fil_direc, sizeof(fil_direc));
        if (!*fil_direc) return true;
        part++;
        append(fil_direc, "");  // adds PATH_SEPARATOR, if not present
        len=strlen(fil_direc);
        if (len==zlen && !memcmp(filename, fil_direc, len)) break;  // found
      } while (true);
    }
  }
 // unzip:
  wxDateTime dtm = wxDateTime::Now();
  char fil_direc2[MAX_PATH], fname2[8+1+3+1];
  { /*CPINDUnZip*/ CUnZip uzi;
    if (!uzi.Open(zipfile)) return true;
    for (LPZIPENTRYINFO zei = uzi.GetFirstFile();  zei!=NULL;  zei = uzi.GetNextFile())
    { zei->GetFileName(filename);
      zlen=strlen(filename);  while (zlen && filename[zlen-1]!=PATH_SEPARATOR) zlen--;
     // find the corresponing database file part:
      part=1;
      do
      { sprintf(keyname, "Path%u", part);
        *fil_direc=0;
        GetDatabaseString(server_name, keyname, fil_direc, sizeof(fil_direc));
        if (!*fil_direc && part==1) GetDatabaseString(server_name, "Path", fil_direc, sizeof(fil_direc));
        if (!*fil_direc) return true;
        part++;
        strcpy(fil_direc2, fil_direc);
        append(fil_direc, "");  // adds PATH_SEPARATOR, if not present
        len=strlen(fil_direc);
        if (len==zlen && !memcmp(filename, fil_direc, len)) // found
        { strcat(fil_direc, FIL_NAME);
          if (must_rename)  // rename
          { sprintf(fname2, "%02u%02u%02u%02u.%02u%c", dtm.GetYear() % 100, dtm.GetMonth()+1, dtm.GetDay(), dtm.GetHour(), dtm.GetMinute(), 'P'+part-1);
            append(fil_direc2, fname2);
            if (wxFileExists(wxString(fil_direc2, *wxConvCurrent))) wxRemoveFile(wxString(fil_direc2, *wxConvCurrent));
            wxRenameFile(wxString(fil_direc, *wxConvCurrent), wxString(fil_direc2, *wxConvCurrent));
          }
          else // erase current
            wxRemoveFile(wxString(fil_direc, *wxConvCurrent));
         // copy the backup part to database file part (direc->fil_direc):
          if (!uzi.DecompEntry(zei, fil_direc))
            return true;
          break;
        }
      } while (true);
    }
  }
  return false;
}
#endif

bool unzip_files(char * zipfile, const char * server_name, bool must_rename, bool private_server)
// Restores backup files from the ZIP. Returns true on error.
// Offline ZIP contains the renamed database files!
{ int zlen;
  char keyname[10];  int part;  char fil_direc[MAX_PATH], filename[MAX_PATH];
 // check the location of fil parts:
  INT_PTR hUnZip, hFind, hFile;
  wchar_t wname[MAX_PATH];
  superconv(ATT_STRING, ATT_STRING, zipfile, (char *)wname, t_specif(0, GetHostCharSet(), 0, 0), t_specif(0, 0, 0, 1), NULL);
  if (UnZipOpen(wname, &hUnZip)) { wxBell();  return true; }
  int Err;
  for (Err = UnZipFindFirst(hUnZip, NULL, true, &hFind, &hFile);  Err==Z_OK;  Err = UnZipFindNext(hFind, &hFile))
  { if (UnZipIsFolder(hUnZip, hFile))
      continue;
    UnZipGetFileName(hUnZip, hFile, wname, sizeof(wname) / sizeof(wchar_t));
    superconv(ATT_STRING, ATT_STRING, (char *)wname, filename, t_specif(0, 0, 0, 1), t_specif(0, GetHostCharSet(), 0, 0), NULL);
    zlen=(int)strlen(filename);  
    char id = filename[zlen-1] & 0xdf;  // A-D
    part=1+(id-'A');
    sprintf(keyname, "Path%u", part);
    *fil_direc=0;
    GetDatabaseString(server_name, keyname, fil_direc, sizeof(fil_direc), private_server);
    if (!*fil_direc && part==1) GetDatabaseString(server_name, "Path", fil_direc, sizeof(fil_direc), private_server);
    if (!*fil_direc) {UnZipClose(hUnZip); return true;}  // error
  }
  
 // unzip:
  wxDateTime dtm = wxDateTime::Now();
  char fil_direc2[MAX_PATH], fname2[8+1+3+1];
  if (Err >= 0)
  { 
    for (Err = UnZipFindFirst(hUnZip, NULL, true, &hFind, &hFile);  Err==Z_OK;  Err = UnZipFindNext(hFind, &hFile))
    { if (UnZipIsFolder(hUnZip, hFile))
        continue;
      UnZipGetFileName(hUnZip, hFile, wname, sizeof(wname) / sizeof(wchar_t));
      superconv(ATT_STRING, ATT_STRING, (char *)wname, filename, t_specif(0, 0, 0, 1), t_specif(0, GetHostCharSet(), 0, 0), NULL);
      zlen=(int)strlen(filename);  
	  char id = filename[zlen-1] & 0xdf;  // A-D
      part=1+(id-'A');
      sprintf(keyname, "Path%u", part);
      *fil_direc=0;
      GetDatabaseString(server_name, keyname, fil_direc, sizeof(fil_direc), private_server);
      if (!*fil_direc && part==1) GetDatabaseString(server_name, "Path", fil_direc, sizeof(fil_direc), private_server);
      if (!*fil_direc) {UnZipClose(hUnZip); true;}
      strcpy(fil_direc2, fil_direc);
      append(fil_direc, FIL_NAME);  // forms the pathname of the database file, adds PATH_SEPARATOR, if not present
	 // rename or remove the existing database file: 
      if (must_rename)  // rename
      { sprintf(fname2, "%02u%02u%02u%02u.%02u%c", dtm.GetYear() % 100, dtm.GetMonth()+1, dtm.GetDay(), dtm.GetHour(), dtm.GetMinute(), 'P'+part-1);
        append(fil_direc2, fname2);
        if (wxFileExists(wxString(fil_direc2, *wxConvCurrent))) wxRemoveFile(wxString(fil_direc2, *wxConvCurrent));
        wxRenameFile(wxString(fil_direc, *wxConvCurrent), wxString(fil_direc2, *wxConvCurrent));
      }
      else // erase current
        wxRemoveFile(wxString(fil_direc, *wxConvCurrent));
     // copy the backup part to database file part (direc->fil_direc):
      superconv(ATT_STRING, ATT_STRING, fil_direc, (char *)wname, t_specif(0, GetHostCharSet(), 0, 0), t_specif(0, 0, 0, 1), NULL);
      Err = UnZipFile(hUnZip, hFile, wname);
      if (Err)
        break;
    }
  }
  UnZipClose(hUnZip); 
  return Err < 0;
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_RECORVERY_START
 */

void DatabaseDlg::OnCdRecorveryStartClick( wxCommandEvent& event )
// Restores file based on its date/time in the client data.
{// find selected backup:
  int sel = mRecovList->GetSelection();
  if (sel==wxNOT_FOUND) { wxBell();  return; }  // nothig selected in the list, should be impossible
  uns32 data = (uns32)(size_t)mRecovList->GetClientData(sel);
  if (!data) return;  // impossble
 // confirm restoring:
  RestoringFileDlg dlg(this);
  dlg.SetMode(0);  // "rename" is the default
  if (dlg.ShowModal()!=wxID_OK) return;  // retoring cancelled
  wxDateTime dtm = wxDateTime::Now();
  bool error=false, any_copied=false;
 // scan the backup file(s):
  unsigned min, hour, day, month, year;  
  min  =data % 60;      data /= 60;
  hour =data % 24;      data /= 24;
  day  =data % 31 + 1;  data /= 31;
  month=data % 12 + 1;  data /= 12;
  year =data;
#if 0 
  char fname[MAX_PATH]; 
  sprintf(fname, "%02u%02u%02u%02u.%02u?", year, month, day, hour, min);
  char direc[MAX_PATH];  strcpy(direc, mRecovDir->GetValue().mb_str(*wxConvCurrent));
  append(direc, fname);
 // pass the backup files:
  wxString f = wxFindFirstFile(wxString(direc, *wxConvCurrent), wxFILE);
  while (!f.IsEmpty())  
  { wxBusyCursor wait;
    unsigned i=(unsigned)f.Length();
    char fil_direc[MAX_PATH], fil_direc2[MAX_PATH], fname2[8+1+3+1];
    char  letter = f[i-1] & 0xdf;
    if      (letter=='A') 
      GetPrimaryPath(server_name, private_server, fil_direc);
    else if (letter=='B' || letter=='C' || letter=='D')
    { char keyname[4+1+1];
      strcpy(keyname, "Pathx");  keyname[4]='1'+(letter-'A');
      GetDatabaseString(server_name, keyname, fil_direc, sizeof(fil_direc), private_server);
    }  
    else if (letter=='Z') 
    { direc[strlen(direc)-1]='z';  // zip file pathname
      error=unzip_files(direc, server_name, dlg.GetMode()==0, private_server);
      any_copied=true;
      break;
    }
    else continue;
   // erase or rename the current database file part (fil_direc->fil_direc2):
    strcpy(fil_direc2, fil_direc);
    append(fil_direc, FIL_NAME);
    if (dlg.GetMode()==0)  // rename
    { sprintf(fname2, "%02u%02u%02u%02u.%02u%c", dtm.GetYear() % 100, dtm.GetMonth()+1, dtm.GetDay(), dtm.GetHour(), dtm.GetMinute(), letter+('P'-'A'));
      append(fil_direc2, fname2);
      wxRemoveFile(wxString(fil_direc2, *wxConvCurrent));
      wxRenameFile(wxString(fil_direc, *wxConvCurrent), wxString(fil_direc2, *wxConvCurrent));
    }
    else // erase current
      wxRemoveFile(wxString(fil_direc, *wxConvCurrent));
   // copy the backup part to database file part (direc->fil_direc):
    direc[strlen(direc)-1]=letter;
    if (!wxCopyFile(wxString(direc, *wxConvCurrent), wxString(fil_direc, *wxConvCurrent))) error=true; 
    else any_copied=true;
    f = wxFindNextFile();
  } 
#ifdef STOP  // the location of fulltext external files is not known when the database is not running
 // restore the external fulltext files:
  sprintf(fname, "%02u%02u%02u%02u.%02u*", year, month, day, hour, min);
  strcpy(direc, mRecovDir->GetValue().mb_str(*wxConvCurrent));
  append(direc, fname);
  wxString f = wxFindFirstFile(wxString(direc, *wxConvCurrent), wxFILE);
  while (!f.IsEmpty())  
  { wxBusyCursor wait;
    f = wxFindNextFile();
  }
#endif
 // report the result:
  if (error)           error_box(_("Error copying the database backup file."), this);
  else if (any_copied) info_box (_("Database restored."), this);
  else                 error_box(_("Backup file not found"), this);
#else
  char pathname_pattern[MAX_PATH];  
  sprintf(pathname_pattern, "%s%c%02u%02u%02u%02u.%02u", 
    (const char*)mRecovDir->GetValue().mb_str(*wxConvCurrent), PATH_SEPARATOR, year, month, day, hour, min);
  error = 0!=srv_Restore(server_name, pathname_pattern, dlg.GetMode()==0);
 // report the result:
  if (error) error_box(_("Error restoring the database backup file."), this);
  else info_box (_("Database restored."), this);
#endif
  event.Skip();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_RECORVERY_DIR
 */

void DatabaseDlg::OnCdRecorveryDirUpdated( wxCommandEvent& event )
{
  list_backups();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PATCH_START
 */

bool start_process_detached(char * command)
#ifdef WINS
{ STARTUPINFOA si;  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof STARTUPINFO);
  memset(&pi, 0, sizeof PROCESS_INFORMATION);
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_SHOW;
  if (!CreateProcessA(NULL, // No module name (use command line).
        command, // Command line.
        NULL,             // Process handle not inheritable.
        NULL,             // Thread handle not inheritable.
        FALSE,            // Set handle inheritance to FALSE.
        CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_PROCESS_GROUP, // creation flags.
        NULL,             // Use parent's environment block.
        NULL,             // Use parent's starting directory.
        &si,              // Pointer to STARTUPINFO structure.
        &pi))             // Pointer to PROCESS_INFORMATION structure.
     return false; // failed
 // Close process and thread handles.
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return true;
}
#else
{ return wxExecute(wxString(command, *wxConvCurrent))!=0; }
#endif

void DatabaseDlg::OnCdPatchStartClick( wxCommandEvent& event )
{ wxString msg;
  msg.Printf(_("Do you want to patch the database '%s'?\n\nThis is only useful if the SQL Server refuses to start using this database.\nMake sure you have a backup copy of the database before patching."), wxString(server_name, *wxConvCurrent).c_str());
  wxMessageDialog dlg(this, msg, STR_WARNING_CAPTION, wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
  if (dlg.ShowModal() != wxID_YES) return;
 // path:
  char path[MAX_PATH+OBJ_NAME_LEN+20];
  if (!get_server_path(path))
    { error_box(_("The server is not properly installed and/or registered"));  return; }
  strcat(path, " -n\"");
  strcat(path, server_name);
  strcat(path, "\" /Berle");
  if (!start_process_detached(path)) 
    { error_box(_("Error staring a detached process."));  return; }
  info_box(_("The database is being patched."));
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_ERASE_START
 */

void DatabaseDlg::OnCdEraseStartClick( wxCommandEvent& event )
{ wxString msg;
  msg.Printf(_("Do you realy want to erase the database '%s'?\n\nAll data and objects will be deleted.\nThis process cannot be undone."), wxString(server_name, *wxConvCurrent).c_str());
  wxMessageDialog dlg(this, msg, STR_WARNING_CAPTION, wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
  if (dlg.ShowModal() != wxID_YES) return;
 // erase:
  char pathname[MAX_PATH+1], last_path[MAX_PATH+1];  
  GetPrimaryPath(server_name, private_server, pathname);
  strcpy(last_path, pathname);
  append(pathname, FIL_NAME);
  if (wxFileExists(wxString(pathname, *wxConvCurrent)))   // unless unregistering a database which hat not been created yet.
    if (!wxRemoveFile(wxString(pathname, *wxConvCurrent)))
      { error_box(_("Cannot erase the database file."), this);  return; }
 // other parts:
  *pathname=0;  GetDatabaseString(server_name, "Path2", pathname, sizeof(pathname), private_server);
  if (*pathname)
    { strcpy(last_path, pathname);  append(pathname, FIL_NAME);  wxRemoveFile(wxString(pathname, *wxConvCurrent)); }
  *pathname=0;  GetDatabaseString(server_name, "Path3", pathname, sizeof(pathname), private_server);
  if (*pathname)
    { strcpy(last_path, pathname);  append(pathname, FIL_NAME);  wxRemoveFile(wxString(pathname, *wxConvCurrent)); }
  *pathname=0;  GetDatabaseString(server_name, "Path4", pathname, sizeof(pathname), private_server);
  if (*pathname)
    { strcpy(last_path, pathname);  append(pathname, FIL_NAME);  wxRemoveFile(wxString(pathname, *wxConvCurrent)); }
 // transact, log (delete from the last FIL directory if explicit path not specified):
  *pathname=0;  GetDatabaseString(server_name, "TransactPath", pathname, sizeof(pathname), private_server);
  if (!*pathname) strcpy(pathname, last_path);
  append(pathname, TRANS_NAME);  wxRemoveFile(wxString(pathname, *wxConvCurrent)); 
  *pathname=0;  GetDatabaseString(server_name, "JournalPath", pathname, sizeof(pathname), private_server);
  if (!*pathname) strcpy(pathname, last_path);
  append(pathname, JOURNAL_NAME);  wxRemoveFile(wxString(pathname, *wxConvCurrent)); 
  *pathname=0;  GetDatabaseString(server_name, "LogPath", pathname, sizeof(pathname), private_server);
  if (!*pathname) strcpy(pathname, last_path);
  append(pathname, MAIN_LOG_NAME);  wxRemoveFile(wxString(pathname, *wxConvCurrent)); 
 // unregistering
  if (!srv_Unregister_server(server_name, private_server))
    error_box(_("Error unregistering the server/service, your privileges may not be sufficient."), this);
     // exits because the database file has been deleted
 // close the properties dialog:
  EndModal(wxID_CLEAR);
}

///////////////////////////// service / daemon //////////////////////////////////
// Only the AutoStart option is saved on the global OK. Other properties are defined immediately by buttons
wxString DatabaseDlg::FindRegisteredServiceForServer(void)
// Returns name of the service associated with the server_name or empty string if not found.
{ char service_name[MAX_SERVICE_NAME+1];
  if (!srv_Get_service_name(server_name, service_name)) 
    *service_name=0;
  return wxString(service_name, *wxConvCurrent);
  // on Unix: must be non-empty, says that the service/daemon can be managed
}

bool DatabaseDlg::get_autostart(wxString service_name)
{
#ifdef LINUX
  char buf[20];
  GetPrivateProfileString(server_name, "AUTOSTART", "0", buf, sizeof(buf), configfile);
  return *buf!='0';
#else
  bool autostart=false;
  SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, GENERIC_READ);//SC_MANAGER_ALL_ACCESS);
  if (hSCM)
  { SC_HANDLE hService = ::OpenService(hSCM, service_name, SERVICE_QUERY_CONFIG | SERVICE_INTERROGATE);
    if (hService)
    { QUERY_SERVICE_CONFIG * sc = (QUERY_SERVICE_CONFIG *)malloc(5000);
      DWORD needed;
      if (sc!=NULL)
      { if (QueryServiceConfig(hService, sc, 5000, &needed))
          autostart = sc->dwStartType==SERVICE_AUTO_START;
        free(sc);
      }
      ::CloseServiceHandle(hService);
    }
    ::CloseServiceHandle(hSCM);
  }
  return autostart;
#endif
}

BOOL WINAPI xWritePrivateProfileString(const char * lpszSection,  const char * lpszEntry, const char * lpszString, const char * lpszFilename);

bool DatabaseDlg::set_autostart(wxString service_name, bool autostart)
// Returns false on error
{
#ifdef LINUX
  return xWritePrivateProfileString(server_name, "AUTOSTART", autostart ? "1" : "0", configfile) != FALSE;
#else
  bool res = false;
  DWORD start_type = autostart ? SERVICE_AUTO_START : SERVICE_DEMAND_START;
  SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (hSCM)
  { SC_HANDLE hService = OpenService(hSCM, service_name, SERVICE_ALL_ACCESS);
    if (hService)  /* change the configuration: */
    { if (ChangeServiceConfig(hService, SERVICE_NO_CHANGE, start_type,
          SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, service_name))
        res=true;
      CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCM);
  }
  return res;
#endif
}

int DatabaseDlg::is_service_running(wxString service_name)
// 0-not running, 1-running, 2-unknown
{
#ifdef LINUX
  return srv_Get_state_local(server_name)==1;
#else
  bool running=false;
  SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, GENERIC_READ);//SC_MANAGER_ALL_ACCESS);
  if (hSCM)
  { SC_HANDLE hService = ::OpenService(hSCM, service_name, SERVICE_QUERY_CONFIG | SERVICE_INTERROGATE);
    if (hService)
    { SERVICE_STATUS ssStatus;
      if (ControlService(hService, SERVICE_CONTROL_INTERROGATE, &ssStatus)) 
        running = ssStatus.dwCurrentState == SERVICE_RUNNING; 
      ::CloseServiceHandle(hService);
    }
    ::CloseServiceHandle(hSCM);
  }
  return running ? 1 : 0;
#endif
}

void DatabaseDlg::show_service_run_state(int running)
{
  mState->SetLabel(running==1 ? _("The service / daemon is running.") : 
                   running==2 ? _("The daemon state is unknown.") : 
                                _("The service / daemon is NOT running."));
  mStart->Enable(running==0);  
  mStop ->Enable(running==1);
}

void DatabaseDlg::show_service_register_state(wxString service_name)
// [service_name] is empty iff service is not registered
{ 
  mOldName->SetValue(service_name);
  if (private_server)
  { mUnregister->Disable();  mRegister->Disable();  mServiceName->Disable();
    mAutoStart->Disable();  mState->Disable();  mStart->Disable();  mStop->Disable();
  }
  else if (service_name.IsEmpty())
  { mUnregister->Disable();  mRegister->Enable();  mServiceName->Enable();
    mAutoStart->Disable();  mState->Disable();  mStart->Disable();  mStop->Disable();
  }
  else
  { mRegister->Disable();  mServiceName->Disable();
   // find is it is running:
    int running=is_service_running(service_name);

    mUnregister->Enable(running==0);  
    mAutoStart->Enable();  
    mState->Enable();  
    show_service_run_state(running);
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SERVICE_UNREGISTER
 */

void DatabaseDlg::OnCdServiceUnregisterClick( wxCommandEvent& event )
// Supposes that the service is not running
{
#ifdef WINS
  if (!srv_Unregister_service(mOldName->GetValue().mb_str(*wxConvCurrent)))
    error_box(_("Error unregistering the service"), this);
 // wait and display the resulting state:
  { wxBusyCursor wait;
    wxThread::Sleep(1000);
  }
  wxString service_name = FindRegisteredServiceForServer();
  show_service_register_state(service_name);
#endif  // not called for LINUX
  event.Skip();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SERVICE_NAME
 */

void DatabaseDlg::OnCdServiceNameUpdated( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SERVICE_REGISTER
 */

void DatabaseDlg::OnCdServiceRegisterClick( wxCommandEvent& event )
{
#ifdef WINS
  wxString service_name = mServiceName->GetValue();
  if (!srv_Register_service(server_name, service_name.mb_str(*wxConvCurrent)))
    error_box(_("Error registering the service"));
  else 
    mServiceName->SetValue(wxEmptyString);
  service_name = FindRegisteredServiceForServer();
  show_service_register_state(service_name);
#endif  // not called for LINUX
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SERVICE_START
 */

void DatabaseDlg::OnCdServiceStartClick( wxCommandEvent& event )
{ bool ok=false;
  { wxBusyCursor busy;
    ok = srv_Start_server_local(server_name, -1, NULL)==KSE_OK;
#ifdef UNIX
    wxThread::Sleep(800);
#endif
  }
  if (!ok) error_box(_("Error starting the service"));
  show_service_register_state(mOldName->GetValue());  // disables the Unregister button
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SERVICE_STOP
 */

void DatabaseDlg::OnCdServiceStopClick( wxCommandEvent& event )
{ int err;
  wxString service_name = mOldName->GetValue();
  { wxBusyCursor wait;
    err=srv_Stop_server_local(server_name, 0, NULL);
  } 
#ifdef LINUX   
  if      (err==EPERM) error_box(_("You do not have the permission to stop the server."));
  else if (err==ESRCH) error_box(_("The server process was not found."));
  else if (err!=0)     error_box(_("Error stopping the server."));
#else  
  if (err!=0)          error_box(_("Error stopping the service."));
#endif
  show_service_register_state(service_name);  // enables the Unregister button
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void DatabaseDlg::OnHelpClick( wxCommandEvent& event )
{
  wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/dlgdata_management.html"));
}



/*!
 * Get bitmap resources
 */

wxBitmap DatabaseDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin DatabaseDlg bitmap retrieval
    return wxNullBitmap;
////@end DatabaseDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon DatabaseDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin DatabaseDlg icon retrieval
    return wxNullIcon;
////@end DatabaseDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL6
 */

void DatabaseDlg::OnCdEdApplsUpdated( wxCommandEvent& event )
{
    mApplsChanged = true;
    OnCdEdDuplFilUpdated(event);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1
 */

#include "impexpprog.h"

void DatabaseDlg::OnCdEdDuplfilClick( wxCommandEvent& event )
{
#if 0  // obsolete version
  if (!IsCtrlDown())
  { CDuplFil DuplFil(this, wxString(server_name, *wxConvCurrent), EDBackName->GetValue(), EDAppls->GetValue(), EDBackFolder->GetValue());
   DuplFil.Perform();
   event.Skip();
  }  
  else
#endif
  {// check the directory for the old database file for the presence of that file:
    wxFileName fname(EDBackFolder->GetValue(), wxT(FIL_NAME));
    if (wxFile::Exists(fname.GetFullPath()))
    { error_box(_("The directory for the original database file already contains such a file"), this);
      return;
    }
    HANDLE handle;  int res;  
    t_avail_server * as = ::Connect(server_name);
    if (as && as->cdp)
    { CImpExpProgDlg ProgDlg(as->cdp, this, _("Restoring the database"), true);
      res=RebuildDb_export(as->cdp, EDAppls->GetValue().mb_str(*wxConvCurrent), &CImpExpProgDlg::Progress, &ProgDlg, &handle);
      try_disconnect(as);
      server_state_changed(as);
      if (!res)
      { int i=0;
        while (i++<10)
        { Sleep(500);  // wait until the server really terminates
          res=RebuildDb_move(handle, EDBackFolder->GetValue().mb_str(*wxConvCurrent));
          if (!res) break;
        }
        if (!res)  
          res=RebuildDb_import(handle);
      }    
      if (res) 
      { wchar_t msg[100];
        Get_error_num_textW(NULL, res, msg, sizeof(msg)/sizeof(wchar_t));
        error_box(msg, this);
      }  
      RebuildDb_close(handle, FALSE);
      ProgDlg.Progress(IMPEXP_FINAL, "");
    }  
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTAPPLS
 */

void DatabaseDlg::OnCdEdApplsClick( wxCommandEvent& event )
{
    // Before editing this code, remove the block markers.
  wxString str = wxDirSelector(_("Select Directory"), EDAppls->GetValue().c_str(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!str.IsEmpty())
    EDAppls->SetValue(str);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTBACKFOLDER
 */

void DatabaseDlg::OnCdEdButbackfolderClick( wxCommandEvent& event )
{
    // Before editing this code, remove the block markers.
  wxString str = wxDirSelector(_("Select Directory"), EDBackFolder->GetValue().c_str(), wxDD_NEW_DIR_BUTTON, wxDefaultPosition, this);
  if (!str.IsEmpty())
      EDBackFolder->SetValue(str);
  event.Skip();
}
/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_EDBACKNAME
 */

void DatabaseDlg::OnCdEdDuplFilUpdated( wxCommandEvent& event )
{
    // Before editing this code, remove the block markers.
    Duplfil->Enable(!EDAppls->GetValue().IsEmpty() && !EDBackFolder->GetValue().IsEmpty());
    event.Skip();
}



/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_ED_BACK_FOLDER
 */

void DatabaseDlg::OnCdEdBackFolderUpdated( wxCommandEvent& event )
{
    OnCdEdDuplFilUpdated(event);
    event.Skip();
}

