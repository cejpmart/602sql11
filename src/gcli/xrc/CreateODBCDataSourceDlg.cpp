/////////////////////////////////////////////////////////////////////////////
// Name:        CreateODBCDataSourceDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/06/07 16:20:57
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "CreateODBCDataSourceDlg.h"
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

#include "CreateODBCDataSourceDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * CreateODBCDataSourceDlg type definition
 */

IMPLEMENT_CLASS( CreateODBCDataSourceDlg, wxDialog )

/*!
 * CreateODBCDataSourceDlg event table definition
 */

BEGIN_EVENT_TABLE( CreateODBCDataSourceDlg, wxDialog )

////@begin CreateODBCDataSourceDlg event table entries
    EVT_BUTTON( wxID_OK, CreateODBCDataSourceDlg::OnOkClick )

////@end CreateODBCDataSourceDlg event table entries

END_EVENT_TABLE()

/*!
 * CreateODBCDataSourceDlg constructors
 */

CreateODBCDataSourceDlg::CreateODBCDataSourceDlg(wxString server_nameIn, wxString schema_nameIn)
{
  server_name=server_nameIn;
  schema_name=schema_nameIn;
}

CreateODBCDataSourceDlg::CreateODBCDataSourceDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * CreateODBCDataSourceDlg creator
 */

bool CreateODBCDataSourceDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CreateODBCDataSourceDlg member initialisation
    mSourceType = NULL;
    mServerName = NULL;
    mSchemaName = NULL;
    mDataSourceName = NULL;
////@end CreateODBCDataSourceDlg member initialisation

////@begin CreateODBCDataSourceDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CreateODBCDataSourceDlg creation
    return TRUE;
}

/*!
 * Control creation for CreateODBCDataSourceDlg
 */

void CreateODBCDataSourceDlg::CreateControls()
{    
////@begin CreateODBCDataSourceDlg content construction
    CreateODBCDataSourceDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxString mSourceTypeStrings[] = {
        _("&System data source (for all users)"),
        _("&User data source")
    };
    mSourceType = new wxRadioBox;
    mSourceType->Create( itemDialog1, CD_CREODBC_TYPE, _("Type of the ODBC Data Source"), wxDefaultPosition, wxDefaultSize, 2, mSourceTypeStrings, 1, wxRA_SPECIFY_COLS );
    itemBoxSizer2->Add(mSourceType, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer4->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer4, 0, wxALIGN_LEFT|wxALL, 5);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("Server name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText5, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mServerName = new wxStaticText;
    mServerName->Create( itemDialog1, CD_CREODBC_SERVER_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(mServerName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText7 = new wxStaticText;
    itemStaticText7->Create( itemDialog1, wxID_STATIC, _("Schema name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mSchemaName = new wxStaticText;
    mSchemaName->Create( itemDialog1, CD_CREODBC_SCHEMA_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(mSchemaName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText9 = new wxStaticText;
    itemStaticText9->Create( itemDialog1, wxID_STATIC, _("Data source name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mDataSourceName = new wxTextCtrl;
    mDataSourceName->Create( itemDialog1, CD_CREODBC_DATA_SOURCE_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(mDataSourceName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton12 = new wxButton;
    itemButton12->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton12->SetDefault();
    itemBoxSizer11->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton13 = new wxButton;
    itemButton13->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemButton13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end CreateODBCDataSourceDlg content construction
    mDataSourceName->SetMaxLength(32);
    mServerName->SetLabel(server_name);
    mSchemaName->SetLabel(schema_name);
    if (!AmILocalAdmin())
    { mSourceType->SetSelection(1);
      mSourceType->Enable(0, false);
    }  
}

/*!
 * Should we show tooltips?
 */

bool CreateODBCDataSourceDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap CreateODBCDataSourceDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CreateODBCDataSourceDlg bitmap retrieval
    return wxNullBitmap;
////@end CreateODBCDataSourceDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CreateODBCDataSourceDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CreateODBCDataSourceDlg icon retrieval
    return wxNullIcon;
////@end CreateODBCDataSourceDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

typedef BOOL WINAPI t_SQLGetInstalledDrivers(char * buf, uns16 cbBufMax, uns16 * pcbBufOut);
typedef BOOL WINAPI t_SQLInstallDriverEx(const char * lpszDriver, const char * pathin,	char * lpszPathOut, uns16 cbPathOutMax,
	uns16 * pcbPathOut, uns16 fRequest, uns32 * lpdwUsageCount);
typedef BOOL WINAPI t_SQLConfigDataSource(HANDLE hWndParent, uns16 fRequest, const char * lpszDriver, const char * lpszAttributes);
#ifndef ODBC_ADD_DSN
#define  ODBC_ADD_DSN     1         // Add data source
#endif
#ifndef ODBC_ADD_SYS_DSN
#define  ODBC_ADD_SYS_DSN 4				  // add a system DSN
#endif
#ifndef ODBC_INSTALL_COMPLETE	
#define  ODBC_INSTALL_COMPLETE	2
#endif                    

void CreateODBCDataSourceDlg::OnOkClick( wxCommandEvent& event )
{ bool close=false;
  bool system_inst = mSourceType->GetSelection()==0;
  wxString dsn = mDataSourceName->GetValue();
  dsn.Trim(false);  dsn.Trim(true);
  if (dsn.IsEmpty())
    { error_box(_("Data source name must be specified"), this);  return; }
#ifdef WINS    
  HINSTANCE hInst = LoadLibraryA("odbccp32.dll");
#else  
  HINSTANCE hInst = LoadLibrary("libodbcinst.so");
#endif
  if (!hInst)
    error_box(_("ODBC Installer library not found"), this); 
  else
  {// check if the driver is installed:
    t_SQLGetInstalledDrivers * p_SQLGetInstalledDrivers = (t_SQLGetInstalledDrivers*)GetProcAddress(hInst, "SQLGetInstalledDrivers");
    if (!p_SQLGetInstalledDrivers)
      error_box(_("Error in ODBC installer library"), this);
    else  
    { uns16 size=1000, cbBufOut;  char * buf;
      do
      { buf=(char*)malloc(size);
        if (!(*p_SQLGetInstalledDrivers)(buf, size, &cbBufOut))
          { error_box(_("Error in ODBC installer library"), this);  goto err; }
        if (cbBufOut+1>=size) 
          { free(buf);  size*=2; }
        else break;  
      } while (true);
      char *p = buf;  
      while (*p)
      { if (!stricmp(p, ODBC_DRIVER_NAME))
          break;
        p+=strlen(p)+1;
      }  
      if (!*p)
      { 
#ifdef WINS  // on Unix just trying and reporting error if installation fails
        if (!AmILocalAdmin())
          { error_box(_("The ODBC driver is not installed and you do not have the administrative privileges to install it."), this);  goto err; }
#endif      
        if (!yesno_box(_("The ODBC driver is not installed, install now?"), this))
          goto err;
       // install the driver:   
        t_SQLInstallDriverEx * pSQLInstallDriverEx = (t_SQLInstallDriverEx*)GetProcAddress(hInst, "SQLInstallDriverEx");
        if (!pSQLInstallDriverEx) 
          { error_box(_("Error in ODBC installer library"), this);  goto err; }
        else
        { char lpszDriver[2*MAX_PATH+300], PathIn[MAX_PATH], PathOut[MAX_PATH];
          uns16 cbPathOut, fRequest;  uns32 dwUsageCount;  int retval = 0;
         // find path to the seriver/setup library:
#ifdef WINS
          GetSystemDirectoryA(PathIn, sizeof(PathIn));
          strcat(PathIn, PATH_SEPARATOR_STR);
#else
          get_path_prefix(PathIn);
          strcat(PathIn, "/lib/" WB_LINUX_SUBDIR_NAME PATH_SEPARATOR_STR); 
#endif
          strcpy(lpszDriver, ODBC_DRIVER_NAME "\nDriver=");
          strcat(lpszDriver, PathIn);
          strcat(lpszDriver, ODBC_DRIVER_FILE "\nSetup=");
          strcat(lpszDriver, PathIn);
          strcat(lpszDriver, ODBC_SETUP_FILE  "\nSQLLevel=2\nFileUsage=0\nDriverODBCVer=03.00\nConnectFunctions=YYY\nAPILevel=2\n\n");
          for (int i=0;  lpszDriver[i];  i++)
            if (lpszDriver[i]=='\n') lpszDriver[i] = 0;
          fRequest=ODBC_INSTALL_COMPLETE;
#ifdef UNIX
          if (!(*pSQLInstallDriverEx)(lpszDriver, NULL, PathOut, sizeof(PathOut), &cbPathOut, fRequest, &dwUsageCount))
          // will use the default location of the odbcinst.ini file
	          { error_box(_("Error installing the ODBC driver. You may not have the write privilege to the /etc/odbcinst.ini file."), this);  goto err; }
#else          
          if (!(*pSQLInstallDriverEx)(lpszDriver, PathIn, PathOut, sizeof(PathOut), &cbPathOut, fRequest, &dwUsageCount))
	          { error_box(_("Error installing the ODBC driver."), this);  goto err; }
#endif          
	        info_box(_("ODBC driver installed OK."), this);  
        }
      }
     // register the data source: 
      t_SQLConfigDataSource * p_SQLConfigDataSource = (t_SQLConfigDataSource*)GetProcAddress(hInst, "SQLConfigDataSource");
      if (!p_SQLConfigDataSource)
        error_box(_("Error in ODBC installer library"), this);
      else
      { char attrs[300];
        sprintf(attrs, "DSN=%s%cSQLServerName=%s%cApplicationName=%s%c", (const char *)dsn.mb_str(*wxConvCurrent), 
                                 0, (const char *)server_name.mb_str(*wxConvCurrent), 
                                 0, (const char *)schema_name.mb_str(*wxConvCurrent), 0);
        if (!(*p_SQLConfigDataSource)(0, system_inst ? ODBC_ADD_SYS_DSN : ODBC_ADD_DSN, ODBC_DRIVER_NAME, attrs))
#ifdef UNIX  // on MSW only administrators are allowed to select the "system_inst" option
          if (system_inst) 
            error_box(_("Error creating the data source. You may not have the write privilege to the /etc/odbc.ini file."), this);
          else  
#endif        
            error_box(_("Error creating the data source"), this);
        else  
        { info_box(_("Data source created OK."), this);   
          close=true;
        }  
      }
        
    }

   err:
    FreeLibrary(hInst);
  }
  if (close)
    event.Skip();
}


