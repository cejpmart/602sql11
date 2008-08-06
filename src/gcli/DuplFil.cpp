/////////////////////////////////////////////////////////////////////////////
// Name:        DuplFilDlg.cpp
// Purpose:     Duplicates database file. Exports all schemas, copies fil to
//              backup folder and renames original server to backup name,
//              creates new fil on original place and imports all schemas.
// Author:      
// Modified by: 
// Created:     06/09/05 14:25:18
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "DuplFil.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#ifndef WINS
#include "winrepl.h"
#endif
#include "cint.h"
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop

#include "wx/file.h"
#include "wx/filename.h"
#include "xrc/LoginDlg.h"
#include "DuplFil.h"
#include "objlist9.h"
#include "impexpui.h"
#include "query.h"
#include "replic.h"

#include "DuplFilProg.cpp"

#ifdef LINUX
#include <sys/types.h>
#include <sys/file.h>
#include <sys/wait.h>
#endif

const wxChar FilName[]  = wxT(FIL_NAME);
#ifdef WINS
const wxChar DBKey[]    = wxT("SOFTWARE\\Software602\\602SQL\\Database\\");
#endif

#define AllUsers         _("- All users & groups -")
#define AllKeys          _("- All keys -")
#define BackupRuns       _("This operation is impossible, since the server named %ls is running")
#define BackupFileRuns   _("This operation is impossible, since the database file %ls is opened")
#define ServerRuns       _("This operation is impossible, since the requested server is running.\nStop the server and try the operation again.")
#define StopSrvrFailed   _("Stopping the server failed")
#define SrvrWndFailed    _("Server start failed")
#define FileCreateFailed _("Failed to create the file %ls")
#define FileOpenFailed   _("Failed to open the file %ls")
#ifdef  WINS
#define RegOpenFailed    _("Read registry error\nFailed to open the key %ls")
#define RegReadFailed    _("Read registry error\nFailed to read the value %ls from key %ls ")
#define RegEnumKFailed   _("Read registry error\nFailed to read the sub-key names from the key %ls")
#define RegEnumVFailed   _("Read registry error\nFailed to read the value names from the key %ls")
#define RegCreateFailed  _("Write registry error\nFailed to create the key %ls")
#define RegWriteFailed   _("Write registry error\nFailed to write the value %ls to the key %ls")
#else
#define ReadIniFailed    _("Failed to read the file %ls")
#define WriteIniFailed   _("Failed to write the file %ls")
#endif
//
// Destructor - Releases schema descriptors
//
CDuplFil::~CDuplFil()
{
    ResetAppls();
}
//
// Returns true if UUID is empty
//
bool IsNullUUID(WBUUID UUID)
{
    int *pInt   = (int *)UUID;
    return pInt[0] == 0 && pInt[1] == 0 && pInt[2] == 0;
}
//
// Performs intrinsic duplication
//
void CDuplFil::Perform()
{
    try
    {
        // Check parametres
        // Check if backup server is allready registred, check if backup fil allready exists
        BackupExist();
        // Check if folder for exported schema exists
        m_WasApplsPath = wxDirExists(m_ApplsPath);
        // Check if server is active
        m_WasActive = DetectLocalServer(m_SrvName);
        // IF not, start server
        if (!m_WasActive) 
            StartServer();
        // Login
        Login();
        m_Connected=true;
        // Show progress dialog
        m_ProgDlg.SetSize(300, 500);
        m_ProgDlg.Show();
        // Ensure exclusive access to server, other clients, replications etc.
        EnsureExclusive();

        // Ensure that logged user is DBAdmin
        bool WasDB = EnsureDBAdmin();
        // Save security server properties
        SaveSecur();
        // Export schemas
        ExportAppls();
        // Export users and keys
        ExportUsers();
        // Export replication servers & relations
        ExportRepl();
        // IF logged user was not DBAdmin, remove him from DBAdmin group
        if (!WasDB)
            RemoveDBAdmin();

        // Backup original server
        BackupServer();

        // Start new server
        Sleep(3000);  // wait for the real end of operation
        StartServer();
		cdp_t cdp = m_cdp;
		m_cdp = cdp_alloc();
		cdp_free(cdp);
        // Logon as anonymous
        Connect(wxEmptyString, wxEmptyString);
        // Lock server
        Lock_server();
        m_Locked = true;

        m_ProgDlg.CategLab->SetLabel(_("Importing:"));
        // Restore original server UUID
        Write_ind(SRV_TABLENUM, 0, SRV_ATR_UUID, m_UUID, sizeof(WBUUID));
        // Delete default _sysext
        Set_application("_sysext");
        SQL_execute("DROP SCHEMA _sysext");
        Set_application("");
        // Restart server
        StopServer(true);
        Sleep(5000);  // wait for the real end of operation
        StartServer();
        // Logon as anonymous
        Connect(wxEmptyString, wxEmptyString);
        cd_Mask_duplicity_error(m_cdp, TRUE);   
        // Import replication servers & relations
        ImportRepl();
        // Import users and keys
        ImportUsers();
        // Lock server
        Lock_server();
        m_Locked = true;
        // Create temporary admin
        tobjname TmpUserN, TmpUserP; 
        tobjnum  TmpUserO = CreateTmpAdmin(TmpUserN, TmpUserP);
        // Logon as temporary admin
        C602SQLApi::Login((const char *)TmpUserN, TmpUserP);
        // Import schemas
        Disable_integrity();
        ImportAppls();
        cd_Mask_duplicity_error(m_cdp, FALSE);   
        // Logon as original user
        C602SQLApi::Login(ToChar(m_UserName, m_cdp->sys_conv), ToChar(m_Password, wxConvCurrent));
        // FOR each schema
        for (CApplList::iterator it = m_Appls.begin(); it != m_Appls.end(); it++)
        {
            CObjDescs *NoExp = it->second;
            // IF schema exported OK
            if (NoExp /*&& NoExp->m_ExpOK*/) // same apl;lications may have errors
            {
                WBUUID ApplUUID;
                // Restore not exportable objects
                tobjnum ApplObj = Find_objectX(ToChar(it->first, m_cdp->sys_conv), CATEG_APPL);
                Read_ind(OBJ_TABLENUM, ApplObj, APPL_ID_ATR, ApplUUID);
                SetExportable(ApplUUID, NoExp);
                // Restore replication conflict parent and token admin servers
                if (!IsNullUUID(NoExp->m_FRCSrv) || !IsNullUUID(NoExp->m_TASrv))
                {
                    apx_header apx;
                    Read_var(OBJ_TABLENUM, ApplObj, OBJ_DEF_ATR, 0, sizeof(apx_header), &apx);
                    memcpy(&apx.os_uuid, &NoExp->m_FRCSrv, sizeof(WBUUID));
                    memcpy(&apx.dp_uuid, &NoExp->m_TASrv, sizeof(WBUUID));
                    Write_var(OBJ_TABLENUM, ApplObj, OBJ_DEF_ATR, 0, sizeof(apx_header), &apx);
                }
            }
        }
        // Restore security properties
        LoadSecur();
        // IF user was not DBAdmin, remove him from DBAdmin group
        if (!WasDB)
            RemoveDBAdmin();
        // Delete temporary admin
        Delete(USER_TABLENUM, TmpUserO);

        // Delete exported schemas files
        DeleteApplFiles();
        if (m_ImportOK)
            m_ProgDlg.ShowItem(_("Database duplication was successful"));
    }
    catch (CWBException *e)
    {
        wxString Msg = e->GetMsg();
        // IF error message exists
        if (Msg != wxEmptyString)
        {
            // Show message dialog
            error_box(Msg, GetParent());
            // IF progress dialog is shown
            if (m_ProgDlg.IsShown()) 
            {
                int s = 0;
                int e;
                // Write separate lines of error message to progress dialog
                for (e = 0; e < Msg.Length(); e++)
                {
                    wxChar c = Msg.GetChar(e);
                    if (c == '\r' || c == '\n')
                    {
                        if (e - s > 0)
                            m_ProgDlg.ShowItem(Msg.Mid(s, e - s));
                        s = e + 1;
                    }
                }
                if (e - s > 0)
                    m_ProgDlg.ShowItem(Msg.Mid(s, e - s));
            }
        }
        delete e;
    }
    catch (...)
    {
        wxString Msg = _("Database duplication failed");
        error_box(Msg, GetParent());
        if (m_ProgDlg.IsShown())
            m_ProgDlg.ShowItem(Msg);
    }
    // Unlock server and stop it:
    StopServer(false);
    // Enable OK button in progress dialog, wait for user closes it
    if (m_ProgDlg.IsShown())
    {
        m_ProgDlg.OKBut->Enable(true);
        m_ProgDlg.OKBut->SetFocus();
        m_ProgDlg.ShowModal();
    }
}
//
// Check if backup server or fil allready exists
//
void CDuplFil::BackupExist()
{
    wxString BackPath = wxEmptyString;

    // Get registered server list
    CServerProps  SrvProps(wxT("Database"));
    wxArrayString *sl = SrvProps.GetSubSectNames();
    int i;
    try
    {
        // Find backup server
        for (i = 0; i < sl->GetCount(); i++)
        {
            // IF found
            if (m_BackName.IsSameAs((*sl)[i], false))
            {
                // IF backup server is running, interrupt execution
                if (DetectLocalServer(m_BackName))
                    throw new CWBException(BackupRuns, m_BackName.c_str());
                // Ask user
                if (!yesno_boxp(_("Server %ls is already registered. Overwrite the database file?"), GetParent(), m_BackName.c_str()))
                    throw new CWBException();
                SrvProps.SetSubSection(m_BackName);
                // Get backup server path
                BackPath = SrvProps.GetPropStr(wxT("Path"));
                if (BackPath.IsEmpty())
                    BackPath = SrvProps.GetPropStr(wxT("Path1"));
                if (BackPath.Last() != wxFileName::GetPathSeparator())
                    BackPath += wxFileName::GetPathSeparator();
            }
        }

        // IF in backup folder exists other fil AND it is not backup server fil
        if (m_BackPath.Last() != wxFileName::GetPathSeparator())
            m_BackPath += wxFileName::GetPathSeparator();
        if (wxFileName::FileExists(m_BackPath + FilName) && !m_BackPath.IsSameAs(BackPath, false))
        {
            wxString Server;
            // Find server owning this fil
            for (i = 0; i < sl->GetCount(); i++)
            {
                wxString srvr = (*sl)[i];
                SrvProps.SetSubSection(srvr);
                BackPath = SrvProps.GetPropStr(wxT("Path"));
                if (BackPath == wxEmptyString) 
                    BackPath = SrvProps.GetPropStr(wxT("Path1"));
                if (BackPath != wxEmptyString) 
                {
                    if (BackPath.Last() != wxFileName::GetPathSeparator())
                        BackPath += wxFileName::GetPathSeparator();
                    // IF found
                    if (m_BackPath.IsSameAs(BackPath, false))
                    {
                        // IF owning server is running, interrupt execution
                        if (DetectLocalServer(srvr))
                            throw new CWBException(BackupFileRuns, m_BackPath.c_str());
                        Server = srvr;
                        break;
                    }
                }
            }
            wxString Msg;
            // Ask user
            if (Server.IsEmpty())
                Msg = wxString::Format(_("Database file already exists in the folder %ls\n\n"), m_BackPath.c_str());
            else
                Msg = wxString::Format(_("A database file already exists in the folder %ls that that belongs to the server %ls.\n\n"), m_BackPath.c_str(), Server.c_str());
            Msg += _("Overwrite?");
            if (!yesno_box(Msg, GetParent()))
                throw new CWBException();
        }
        delete sl;
    }
    catch (...)
    {
        delete sl;
        throw;
    }
}

#ifdef STOP  // obsolete version
#ifdef WINS
//
// Searches server window
//
BOOL CALLBACK EnumThreadWindowsProc(HWND Handle, LPARAM lParam)
{
    wxChar ClassName[16];
    GetClassName(Handle, ClassName, sizeof(ClassName));
    if (lstrcmp(ClassName, wxT("WBServerClass32")) == 0)
    {
        ((CDuplFil *)lParam)->m_SrvWnd = Handle;
        return false;
    }
    return true;
}

#if WB_VERSION_MAJOR == 9
#if WB_VERSION_MINOR == 5
const wxChar VerKey[] = wxT("SOFTWARE\\Software602\\602SQL\\9.5");
#else
const wxChar VerKey[] = wxT("SOFTWARE\\Software602\\602SQL\\9.0");
#endif
#else
#if WB_VERSION_MINOR == 1
const wxChar VerKey[] = wxT("SOFTWARE\\Software602\\602SQL\\8.1");
#else
const wxChar VerKey[] = wxT("SOFTWARE\\Software602\\602SQL\\8.0");
#endif
#endif
//
// Starts server
//
void CDuplFil::StartServer()
{
    PROCESS_INFORMATION pi;
    wxChar Path[MAX_PATH];

    wxString Cmd = wxEmptyString;
    pi.hThread   = 0;
    try
    {
        // Find server in same folder as curent module
        // Get current module path
        if (GetModuleFileName(0, Path, sizeof(Path) / sizeof(wxChar)))
        {
            // Replace file name with server file name
            wxFileName fn(Path);
            fn.SetFullName(wxString(SERVER_FILE_NAME, *wxConvCurrent));
            // IF such file exists, set command
            if (fn.FileExists())
                Cmd = fn.GetFullPath();
        }
        // IF server not found, find server in registry
        if (!Cmd)
        {
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, VerKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                DWORD sz  = sizeof(Path);
                DWORD Res = RegQueryValueEx(hKey, wxT("Path"), NULL, NULL, (LPBYTE)Path, &sz);
                if (Res != ERROR_SUCCESS)
                    Res = RegQueryValueEx(hKey, wxT("Path1"), NULL, NULL, (LPBYTE)Path, &sz);
                RegCloseKey(hKey);
                if (Res == ERROR_SUCCESS) 
                {
                    wxString sPath = wxString(Path);
                    if (sPath.Last() != wxFileName::GetPathSeparator())
                        sPath += wxFileName::GetPathSeparator();
                    sPath += wxString(SERVER_FILE_NAME, *wxConvCurrent);
                    if (wxFileExists(sPath))
                        Cmd = sPath;
                }
            }
        }
        // IF server not found, try server exe name without path
        if (!Cmd)
            Cmd = wxString(SERVER_FILE_NAME, *wxConvCurrent);
        // Append server name
        Cmd += wxT(" &\"") + m_SrvName + wxT("\" /Q");
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        // Start server
        if (!CreateProcess(NULL, (wxChar *)(const wxChar*)Cmd, NULL, NULL, false, 0, NULL, NULL, &si, &pi))
            throw new CSysException(wxT("CreateProcess"), SrvrWndFailed);
        // Store server process handle
        m_SrvHandle = pi.hProcess;
        m_SrvWnd    = 0;
        DWORD tm    = 0;

        // Initialize timeout
        TimeoutInit(_("Starting SQL Server"), SrvrWndFailed);
        while (true)
        {
            // Find server window
            EnumThreadWindows(pi.dwThreadId, EnumThreadWindowsProc, (LPARAM)this);
            // IF found
            if (m_SrvWnd != 0) 
            {
                // IF window is visible server is running
                if (IsWindowVisible(m_SrvWnd))
                    break;
                // Wait more second
                if (tm == 0)
                    tm = GetTickCount() + 1000;
                // IF timeout elapsed, server is not running
                else if (GetTickCount() > tm)
                    break;
            }
            // IF server failed, interrupt execution
            if (WaitForSingleObject(pi.hThread, 0) != WAIT_TIMEOUT)
                throw new CWBException(SrvrWndFailed);
            TimeoutWait();
        }
        m_WasActive = true;
        CloseHandle(pi.hThread);
    }
    catch (...)
    {
        if (pi.hThread != 0)
            CloseHandle(pi.hThread);
        throw;
    }
}
//
// Stops server
// Input:   Find - unknown server process should be found
//
void CDuplFil::StopServer(bool Find)
{
    // IF process handle is not known AND find server
    if (m_SrvHandle == 0 && Find)
    {
        // IF runs as service, stop service
        if (StopService())
            return;
        // Find server
        FindServer();
    }
    // Disconnect
    Disconnect();
    m_Locked = false;
    // IF server known AND was active
    if (m_SrvHandle != 0 && m_WasActive)
    {
        // Stop server
        if (!PostMessage(m_SrvWnd, WM_ENDSESSION, 1, 0))
            throw new CSysException(wxT("PostMessage"), StopSrvrFailed);
        // Wait 10 seconds until it realy terminates
        DWORD Wait = WaitForSingleObject(m_SrvHandle, 10000);
        if (Wait == WAIT_FAILED)
            throw new CSysException(wxT("WaitForSingleObject"), StopSrvrFailed);
        // IF timeout
        if (Wait == WAIT_TIMEOUT)
        {
            // Show message and wait next 1 minute
            m_ProgDlg.ShowItem(_("Stopping SQL Server"));
            Wait = WaitForSingleObject(m_SrvHandle, 60000);
            if (Wait == WAIT_FAILED || Wait == WAIT_TIMEOUT)
                throw new CSysException(wxT("WaitForSingleObject"), StopSrvrFailed);
        }
    }
    CloseHandle(m_SrvHandle);
    m_SrvHandle = 0;
    m_SrvWnd    = 0;
}

#else   // LINUX
//
// Starts server
//
void CDuplFil::StartServer()
{
    // Start daemon
    srv_Start_server_local(m_SrvName.mb_str(*wxConvCurrent), 0, NULL);
    // Wait until server runs
    TimeoutInit(_("Starting SQL Server"), SrvrWndFailed);
    while (true)
    {
        int st = srv_Get_state_local(m_SrvName.mb_str(*wxConvCurrent));
        if (st == 1)
        {
            sleep(3);
            break;
        }
        TimeoutWait();
    }
}
//
// Stop server
//
void CDuplFil::StopServer(bool Find)
{
    // Stop daemon
    srv_Stop_server_local(m_SrvName.mb_str(*wxConvCurrent), 0, NULL);
    m_Locked = false;
    // Wait until server realy stops
    TimeoutInit(_("Stopping SQL Server"), StopSrvrFailed);
    while (true)
    {
        int st = srv_Get_state_local(m_SrvName.mb_str(*wxConvCurrent));
        if (st != 1)
            break;
        TimeoutWait();
    }
}

#endif
#endif  // obsolete

//
// Starts server
//
void CDuplFil::StartServer()
{
    if (srv_Start_server_local(m_SrvName.mb_str(*wxConvCurrent), 1, NULL) != KSE_OK)
      throw new CWBException(wxString(_("Server start failed")).c_str());
}
//
// Stop server
//
void CDuplFil::StopServer(bool Find)
{
  srv_Stop_server_local(m_SrvName.mb_str(*wxConvCurrent), 0, m_Connected ? m_cdp : NULL);
  m_Connected=false;
}

//
// Ensures that logged user has exclusive access to server
// No other clients, no replications, Lock_server succedes
//
void CDuplFil::EnsureExclusive()
{
    tcursnum Curs = NOCURSOR;
    try
    {
        // Check other clients 
        Curs = Open_cursor_direct("SELECT Client_number FROM _IV_LOGGED_USERS WHERE NOT Own_connection AND NOT Worker_thread");
        if (Rec_cnt(Curs) != 0)
            throw new CWBException(_("This operation is impossible, since other users are currently connected to the database"));
        Close_cursor(Curs);
        Curs = NOCURSOR;
        // Stop replications
        Read_ind(SRV_TABLENUM, 0, SRV_ATR_ACKN, &m_WasRepl);
        if (m_WasRepl)
        {
            m_ProgDlg.ShowItem(_("Stopping replication"));
            bool Enb = false;
            Write_ind(SRV_TABLENUM, 0, SRV_ATR_ACKN, &Enb, 1);
            Reset_replication();
        }
        // Lock server
        m_Locked = Lock_server();
        // IF lock server failes, restart server
        if (!m_Locked)
        {
            StopServer(true);
            StartServer();
            Connect(m_UserName, m_Password);
        }
    }
    catch (...)
    {
        if (Curs != NOCURSOR)
            Close_cursor(Curs);
        throw;
    }
}
//
// Stops server running as service
//
bool CDuplFil::StopService()
{
#ifdef WINS
    if ((int)GetVersion() < 0)
        return false;

    wxChar Name[256];
    wxChar Service[256];
    SC_HANDLE hMngr = 0;
    SC_HANDLE hSrvc = 0;
    HKEY SrvcsKey   = NULL;
    HKEY SrvcKey    = NULL;
    bool Result     = false;
    try
    {
        // Open service manager
        LPCTSTR Key = wxT("SYSTEM\\CurrentControlSet\\Services");
        DWORD Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key, 0, KEY_READ_EX, &SrvcsKey);
        if (Err != ERROR_SUCCESS)
            throw new CSysException(wxT("RegOpenKeyEx"), Err, RegOpenFailed, Key);
        hMngr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!hMngr)
            throw new CSysException(wxT("OpenSCManager"), _("Failed to open the service manager"));
        // Find current service
        for (int i = 0; ;i++)
        {
            DWORD Size = sizeof(Service) / sizeof(wxChar);
            if (RegEnumKeyEx(SrvcsKey, i, Service, &Size, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                break;
            Err = RegOpenKeyEx(SrvcsKey, Service, 0, KEY_READ, &SrvcKey);
            if (Err != ERROR_SUCCESS)
                throw new CSysException(wxT("RegOpenKeyEx"), Err, RegOpenFailed, Service);
            Size = sizeof(Name);
            Err  = RegQueryValueEx(SrvcKey, wxT("CnfPath"), NULL, NULL, (LPBYTE)Name, &Size);
            // IF service found
            if (Err != ERROR_FILE_NOT_FOUND)
            {
                if (Err != ERROR_SUCCESS)
                    throw new CSysException(wxT("RegQueryValueEx"), Err, RegReadFailed, wxT("CnfPath"), Service);
                if (wxStricmp(Name, m_SrvName) == 0)
                {
                    hSrvc = OpenService(hMngr, Service, SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_STOP);
                    if (hSrvc == 0)
                    {
                        if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
                            throw new CSysException(wxT("OpenService"), _("Failed to open service %ls"), Service);
                        continue;
                    }
                    SERVICE_STATUS SrvcStat;
                    SrvcStat.dwCurrentState = 0;
                    QueryServiceStatus(hSrvc, &SrvcStat);
                    // IF service is running
                    if (SrvcStat.dwCurrentState == SERVICE_RUNNING)
                    {
                        // Disconnect
                        Disconnect();
                        m_Locked = false;
                        // Stop service
                        if (ControlService(hSrvc, SERVICE_CONTROL_STOP, &SrvcStat))
                        {
                            TimeoutInit(_("Stopping service"), _("Failed to stop service"));
                            while (SrvcStat.dwCurrentState == SERVICE_STOP_PENDING)
                                TimeoutWait();
                        }
                        Result = true;
                        break;
                    }
                    CloseServiceHandle(hSrvc);
                    hSrvc = 0;
                }
            }
            RegCloseKey(SrvcKey);
            SrvcKey = NULL;
        }
        RegCloseKey(SrvcsKey);
        CloseServiceHandle(hMngr);
        return Result;
    }
    catch (...)
    {
        if (hSrvc != 0)
            CloseServiceHandle(hSrvc);
        if (hMngr != 0)
            CloseServiceHandle(hMngr);
        if (SrvcKey != NULL)
            RegCloseKey(SrvcKey);
        if (SrvcsKey != NULL)
            RegCloseKey(SrvcsKey);
        throw;
    }
#else
    return false;
#endif
}
//
// Log on server
//
void CDuplFil::Login()
{
    wxString tx;
    while (true)
    {
        try
        {
            // Connect to server
            connect(ToChar(m_SrvName, wxConvCurrent));
            make_sys_conv(m_cdp);
            // Show login dialog
            LoginDlg Dlg(GetParent(), true);
            Dlg.SetCdp(m_cdp);
            if (Dlg.ShowModal() != wxID_OK)
                throw new CWBException();
            if (Dlg.mAnonym->GetValue())
            {
                m_UserName = wxEmptyString;
                m_Password = Dlg.mPass->GetValue();
            }
            else
            {
                m_UserName = Dlg.mName->GetValue();
                m_Password = Dlg.mPass->GetValue();
            }
            // IF logged user is not configuration administrator, continue
            if (!Am_I_config_admin()) 
            {
                tx = wxString::Format(_("The user %ls is NOT a configuration administrator"), m_UserName.c_str());
                error_box(tx, GetParent());
            } 
            // IF logged user is not security administrator, continue
            else if (!Am_I_security_admin())
            {
                tx = wxString::Format(_("The user %ls is NOT a security administrator"), m_UserName.c_str());
                error_box(tx, GetParent());
            } 
            else
                break;
        }
        catch (C602SQLException *e)
        {
            int Err = e->GetError();
            if (Err != OBJECT_DOES_NOT_EXIST && Err !=BAD_PASSWORD)
                throw;
            error_box(_("Invalid username or password"), GetParent());
            delete e;
        }
        catch (...)
        {
            Disconnect();
            m_Locked = false;
            throw;
        }
    }
}
//
// Finds running server process
//
void  CDuplFil::FindServer()
{
#ifdef WINS
    wxChar ClassName[70];
    // Find server window
    for (HWND hMain = GetTopWindow(NULL); hMain; hMain = GetWindow(hMain, GW_HWNDNEXT))
    {
        GetClassName(hMain, ClassName, sizeof(ClassName));
        // IF found, check server name
        if (lstrcmp(ClassName, wxT("WBServerClass32")) == 0)
        {
            GetWindowText(hMain, ClassName, sizeof(ClassName));
            // IF my server window, store server window handle and server process handle
            if (lstrcmpi(ClassName + 15, m_SrvName) == 0)
            {
                DWORD ID;
                GetWindowThreadProcessId(hMain, &ID);
                m_SrvHandle = OpenProcess(PROCESS_TERMINATE + SYNCHRONIZE, false, ID);
                if (m_SrvHandle == 0)
                    throw new CSysException(wxT("OpenProcess"), ServerRuns);
                m_SrvWnd = hMain;
                return;
            }
        }

    }
    // IF server process not found, server must be stopped manualy
    throw new CWBException(ServerRuns);
#endif
}
//
// Saves server security properties
// 
void CDuplFil::SaveSecur()
{
    CServerProps sp(m_cdp, wxT("@SQLSERVER"));
    m_DsbleAnn  = sp.GetPropBool(wxT("DisableAnonymous"));
    m_UnlimPw   = sp.GetPropBool(wxT("UnlimitedPasswords"));
    m_MinPwLen  = sp.GetPropInt(wxT("MinPasswordLen"));
    m_PwExpir   = sp.GetPropInt(wxT("PasswordExpiration"));
    m_Domain    = sp.GetPropStr(wxT("ThrustedDomain"));
    m_EncReq    = sp.GetPropBool(wxT("ReqCommEnc"));
    m_OwnGroup  = sp.GetPropBool(wxT("ConfAdmsOwnGroup"));
    m_UserGroup = sp.GetPropBool(wxT("ConfAdmsUserGroups"));
}
//
// Restores server security properties
// 
void CDuplFil::LoadSecur()
{
    CServerProps sp(m_cdp, wxT("@SQLSERVER"));
    sp.SetProp(wxT("DisableAnonymous"),   m_DsbleAnn);
    sp.SetProp(wxT("UnlimitedPasswords"), m_UnlimPw);
    sp.SetProp(wxT("MinPasswordLen"),     m_MinPwLen);
    sp.SetProp(wxT("PasswordExpiration"), m_PwExpir);
    sp.SetProp(wxT("ThrustedDomain"),     m_Domain);
    sp.SetProp(wxT("ReqCommEnc"),         m_EncReq);
    sp.SetProp(wxT("ConfAdmsOwnGroup"),   m_OwnGroup);
    sp.SetProp(wxT("ConfAdmsUserGroups"), m_UserGroup);
}
//
// Exports all schemas
//
void CDuplFil::ExportAppls()
{
    bool        OverWrite = false;
    CObjDescs  *NoExp     = NULL;
    try
    {
        // Get server UUID
        Read_ind(SRV_TABLENUM, 0, SRV_ATR_UUID, m_UUID);
        if (m_ApplsPath.Last() != wxFileName::GetPathSeparator())
            m_ApplsPath += wxFileName::GetPathSeparator();
        // Set export schema parametres
        t_export_param ep;
        ep.cbSize            = sizeof(t_export_param);
        ep.hParent           = 0;
        ep.with_data         = true;
        ep.with_role_privils = true;
        ep.with_usergrp      = true;
        ep.back_end          = false;
        ep.date_limit        = 0;
        ep.long_names        = true;
        ep.report_progress   = true;
        ep.callback          = &CDuplFilProgDlg::Progress;
        ep.param             = &m_ProgDlg;
        ep.overwrite         = true;
        // Clear schema descriptor list
        ResetAppls();
        CObjectList ol(m_cdp);
        apx_header  apx;
        WBUUID      ApplUUID;
        // FOR each schema
        for (bool Found = ol.FindFirst(CCateg(CATEG_APPL)); Found; Found = ol.FindNext())
        {
            const char *Appl       = ol.GetName();
            wxString   wAppl       = wxString(Appl, *m_cdp->sys_conv);
            // Set schema as current schema
            Set_application_ex(Appl, false);
            wxString ApplPath      = m_ApplsPath + wAppl;
            m_ProgDlg.ApplED->SetValue(wAppl);
            // Get schema objnum
            tobjnum ApplObj        = Find_objectX(Appl, CATEG_APPL);
            apx.appl_locked        = false;
            // Read schema header
            Read_var(OBJ_TABLENUM, ApplObj, OBJ_DEF_ATR, 0, sizeof(apx_header), &apx);
            ep.exp_locked          = apx.appl_locked;
            // Read schema UUID
            Read_ind(OBJ_TABLENUM, ApplObj, APPL_ID_ATR, ApplUUID);
            // Create new schema descriptor
            NoExp = new CObjDescs();
            // Ensure all objects
            GetExportable(ApplUUID, NoExp);
            // Store replication conflict parent and token admin server UUID
            if (!IsNullUUID(apx.os_uuid) || !IsNullUUID(apx.dp_uuid))
            {
                memcpy(NoExp->m_FRCSrv, apx.os_uuid, sizeof(WBUUID));
                memcpy(NoExp->m_TASrv,  apx.dp_uuid, sizeof(WBUUID));
            }
            // Create folders for exported files if not exist
            ForceDirectories(ApplPath);
            wxString fn     = ApplPath + wxFileName::GetPathSeparator() + wAppl + wxT(".apl");
            // IF schema file allready exists, ask user
            if (!OverWrite && wxFileExists(fn))
            {
                int Res = yesnoall_boxp(_("Overwrite all"), _("The folder %ls already contains schema %ls files.\n\nOverwrite?"), wxGetApp().frame, ApplPath.c_str(), wAppl.c_str());
                // IF not overwrite, interrupt execution
                if (Res == wxID_NO)
                    throw new CWBException();
                // IF overwrite all, do not ask again
                if (Res == wxID_YESTOALL)
                    OverWrite = true;
                // Set overwrite flag
                ep.overwrite = true;
            }
            // Export schema
            wxCharBuffer cb = fn.mb_str(*wxConvCurrent);
            ep.file_name    = (char *)(const char *)cb;
            ep.schema_name  = Appl;
            m_ProgDlg.ShowItem(_("Exporting application ") + wAppl);
            m_Appls[wAppl]  = NoExp;
            NoExp->m_ExpOK  = Export_appl_param(m_cdp, &ep) != false;
            SetExportable(ApplUUID, NoExp);
            NoExp           = NULL;
            // IF not overwrite all, reset overwrite flag
            if (!OverWrite)
                ep.overwrite = false;
        }
    }
    catch (...)
    {
        if (NoExp)
            delete NoExp;
        throw;
    }
}
//
// Exports users and keys
//
void  CDuplFil::ExportUsers()
{
    // Show progress in progress dialog
    m_ProgDlg.Progress(IMPEXP_STATUS,  _("Exporting list of users, groups & keys"));
    m_ProgDlg.Progress(IMPEXP_PROGMAX, (const wxChar *)2);
    // Create folders for exported files if not exist
    ForceDirectories(m_ApplsPath);
    // Export users to USERTAB.TDT
    wxString fName = m_ApplsPath + wxT("USERTAB.TDT");
    FHANDLE hFile  = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_FHANDLE_VALUE)
        throw new CSysException(wxT("CreateFile"), FileCreateFailed, fName.c_str());
    try
    {
        m_ProgDlg.Progress(CATEG_USER,      AllUsers);
        m_ProgDlg.Progress(IMPEXP_FILENAME, fName);
        if (cd_Export_user(m_cdp, NOOBJECT, hFile))
            throw new C602SQLException(m_cdp, wxT("Export_user"));
        CloseHandle(hFile);
        hFile = INVALID_FHANDLE_VALUE;
        // Export keys to KEYTAB.TDT
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        fName = m_ApplsPath + wxT("KEYTAB.TDT");
        hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_FHANDLE_VALUE)
            throw new CSysException(wxT("CreateFile"), FileCreateFailed, fName.c_str());
        m_ProgDlg.Progress(CATEG_KEY,       AllKeys);
        m_ProgDlg.Progress(IMPEXP_FILENAME, fName);
        xSave_table(KEY_TABLENUM, hFile);
        // Show progress in progress dialog
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        m_ProgDlg.Progress(IMPEXP_STATUS, _("Successfully exported users, groups & keys"));
        m_ProgDlg.NameED->SetValue(wxEmptyString);
        m_ProgDlg.ApplED->SetValue(wxEmptyString);
        CloseHandle(hFile);
    }
    catch (...)
    {
        CloseHandle(hFile);
        throw;
    }
}
//
// Exports replication servers and rules and server properties
//
void  CDuplFil::ExportRepl()
{
    // EXIT IF no replication servers
    if (Rec_cnt(SRV_TABLENUM) <= 1)
        return;
    // Show progress in progress dialog
    m_ProgDlg.Progress(IMPEXP_STATUS, _("Exporting replication servers & relations"));
    m_ProgDlg.Progress(IMPEXP_PROGMAX, (const wxChar *)2);
    // Export replication servers to SRVTAB.TDT
    wxString fName = m_ApplsPath + wxT("SRVTAB.TDT");
    FHANDLE hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_FHANDLE_VALUE)
        throw new CSysException(wxT("CreateFile"), FileCreateFailed, fName.c_str());
    try
    {
        m_ProgDlg.Progress(CATEG_TABLE,     wxT("SRVTAB"));
        m_ProgDlg.Progress(IMPEXP_FILENAME, fName);
        xSave_table(SRV_TABLENUM, hFile);
        CloseHandle(hFile);
        hFile = INVALID_FHANDLE_VALUE;
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        // Export replication rules to REPLTAB.TDT
        fName = m_ApplsPath + wxT("REPLTAB.TDT");
        hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_FHANDLE_VALUE)
            throw new CSysException(wxT("CreateFile"), FileCreateFailed, fName.c_str());
        m_ProgDlg.Progress(CATEG_TABLE,     wxT("REPLTAB"));
        m_ProgDlg.Progress(IMPEXP_FILENAME, fName);
        xSave_table(REPL_TABLENUM, hFile);
        CloseHandle(hFile);
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        // Export server properties to PROPTAB.TDT
        fName = m_ApplsPath + wxT("PROPTAB.TDT");
        hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_FHANDLE_VALUE)
            throw new CSysException(wxT("CreateFile"), FileCreateFailed, fName.c_str());
        m_ProgDlg.Progress(CATEG_TABLE,     wxT("__PROPTAB"));
        m_ProgDlg.Progress(IMPEXP_FILENAME, fName);
        xSave_table(PROP_TABLENUM, hFile);
        CloseHandle(hFile);
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        // Show progress in progress dialog
        m_ProgDlg.Progress(IMPEXP_STATUS, _("Successfully exported replication servers & relations"));
        m_ProgDlg.NameED->SetValue(wxEmptyString);
        m_ProgDlg.ApplED->SetValue(wxEmptyString);
    }
    catch (...)
    {
        CloseHandle(hFile);
        throw;
    }
}
//
// Backups original server
//
void  CDuplFil::BackupServer()
{
    CServerProps SrcProps(wxT("Database"), m_SrvName);
    CServerProps DstProps(wxT("Database"));
    // Show progress in progress dialog
    m_ProgDlg.ShowItem(_("Creating backup of original database"));
    m_ProgDlg.Progress(IMPEXP_PROGMAX, (const wxChar *)3);
    // Stop server
    StopServer(true);
    m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
    // Get fil path
    wxString Path = SrcProps.GetPropStr(wxT("Path"));
    if (Path.IsEmpty())
        Path = SrcProps.GetPropStr(wxT("Path1"));
    if (Path.Last() != wxFileName::GetPathSeparator())
        Path += wxFileName::GetPathSeparator();
    Path += FilName;
    // Create folder for fil backup if not exists
    ForceDirectories(m_BackPath);
    // Be sure that backup fil not exists
    if (m_BackPath.Last() != wxFileName::GetPathSeparator())
        m_BackPath += wxFileName::GetPathSeparator();
    wxString BackPath = m_BackPath + FilName;
    wxRemoveFile(BackPath);
    // Move fil to backup folder
    if (!wxRenameFile(Path, BackPath))
        throw new CSysException(wxT("wxRenameFile"), _("Cannot move database file to %ls"), BackPath.c_str());
    m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);

    // Get original server registration keys
    wxArrayString *PropNames = SrcProps.GetPropNames();
    try
    {
        DstProps.CreateSubSection(m_BackName);
        // Copy registration to backup server
        for (int i = 0; i < PropNames->GetCount(); i++)
        {
            wxString Prop = (*PropNames)[i];
            wxVariant Val = SrcProps.GetProp(Prop);
            DstProps.SetProp(Prop, Val);
        }
        // Reset backup server path property
        DstProps.SetProp(wxT("Path"), m_BackPath);
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        m_ProgDlg.ShowItem(_("Successfully created original database backup"));
        delete PropNames;
    }
    catch (...)
    {
        delete PropNames;
        throw;
    }
}
//
// Imports users and keys
//
void  CDuplFil::ImportUsers()
{
    // Show progress in progress dialog
    m_ProgDlg.Progress(IMPEXP_STATUS, _("Importing list of users, groups & keys"));
    m_ProgDlg.Progress(IMPEXP_PROGMAX, (const wxChar *)2);
    // Import keys from KEYTAB.TDT
    wxString fName = m_ApplsPath + wxT("KEYTAB.TDT");
    FHANDLE hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_FHANDLE_VALUE)
        throw new CSysException(wxT("CreateFile"), FileOpenFailed, fName.c_str());
    try
    {
        m_ProgDlg.Progress(CATEG_KEY,       AllKeys);
        m_ProgDlg.Progress(IMPEXP_FILENAME, fName);
        Restore_table(KEY_TABLENUM, hFile, false);
        CloseHandle(hFile);
        hFile = INVALID_FHANDLE_VALUE;
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        // Import users from USERTAB.TDT
        fName = m_ApplsPath + wxT("USERTAB.TDT");
        hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_FHANDLE_VALUE)
            throw new CSysException(wxT("CreateFile"), FileOpenFailed, fName.c_str());
        m_ProgDlg.Progress(CATEG_USER,      AllUsers);
        m_ProgDlg.Progress(IMPEXP_FILENAME, fName);
        Restore_table(TAB_TABLENUM, hFile, false);  // TABTAB represents the whole USERTAB!
        // Show progress in progress dialog
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        m_ProgDlg.Progress(IMPEXP_STATUS, _("Successfully imported users, groups & keys"));
        CloseHandle(hFile);
    }
    catch (...)
    {
        CloseHandle(hFile);
    }
}
//
// Imports replication servers and rules and server properties
//
void  CDuplFil::ImportRepl()
{
    // Import replication servers from SRVTAB.TDT
    wxString fName = m_ApplsPath + wxT("SRVTAB.TDT");
    FHANDLE  hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_FHANDLE_VALUE)
        return;
    try
    {
        m_ProgDlg.Progress(IMPEXP_STATUS,   _("Importing replication servers & relations"));
        m_ProgDlg.Progress(IMPEXP_PROGMAX,  (const wxChar *)2);
        m_ProgDlg.Progress(CATEG_TABLE,     wxT("SRVTAB"));
        m_ProgDlg.Progress(IMPEXP_FILENAME, fName);
        Delete(SRV_TABLENUM, 0);            // Delete local server record
        Free_deleted(SRV_TABLENUM);
        Restore_table(SRV_TABLENUM, hFile, false);
        CloseHandle(hFile);
        hFile = INVALID_FHANDLE_VALUE;
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        // Import replication rules from REPLTAB.TDT
        fName = m_ApplsPath + wxT("REPLTAB.TDT");
        hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_FHANDLE_VALUE)
            throw new CSysException(wxT("CreateFile"), FileOpenFailed, fName.c_str());
        m_ProgDlg.Progress(CATEG_TABLE, wxT("REPLTAB"));
        m_ProgDlg.Progress(IMPEXP_FILENAME, fName);
        Restore_table(REPL_TABLENUM, hFile, false);
        CloseHandle(hFile);
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        // Import server properties from PROPTAB.TDT
        fName = m_ApplsPath + wxT("PROPTAB.TDT");
        hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_FHANDLE_VALUE)
            throw new CSysException(wxT("CreateFile"), FileOpenFailed, fName.c_str());
        m_ProgDlg.Progress(CATEG_TABLE, wxT("__PROPTAB"));
        m_ProgDlg.Progress(IMPEXP_FILENAME, fName);
        Restore_table(PROP_TABLENUM, hFile, false);
        CloseHandle(hFile);
        // Show progress in progress dialog
        m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        m_ProgDlg.Progress(IMPEXP_STATUS,   _("Successfully imported replication servers & relations"));
    }
    catch (...)
    {
        CloseHandle(hFile);
        throw;
    }
}
//
// Imports all schemas
//
void  CDuplFil::ImportAppls()
{
    try
    {
        // Set import schema parametres
        t_import_param ip;
        ip.cbSize          = sizeof(t_import_param);
        ip.hParent         = 0;
        ip.flags           = IMPAPPL_REPLACE + IMPAPPL_NO_COMPILE + IMPAPPL_WITH_INFO;
        ip.alternate_name  = NULL;
        ip.report_progress = true;
        ip.callback        = &CDuplFilProgDlg::Progress;
        ip.param           = &m_ProgDlg;
        m_ImportOK         = true;
        // FOR each exported schema
        for (CApplList::iterator it = m_Appls.begin(); it != m_Appls.end(); it++)
        {
            // IF not exported successfuly, continue
//            if (!it->second->m_ExpOK)  -- importing applications experted with errors, too
//                continue;
            wxString Appl     = it->first;
            wxString ApplPath = m_ApplsPath + Appl;
            m_ProgDlg.ApplED->SetValue(Appl);
            wxString     fn   = ApplPath + wxFileName::GetPathSeparator() + Appl + wxT(".apl");
            wxCharBuffer cb   = fn.mb_str(*wxConvCurrent);
            ip.file_name      = (char *)(const char *)cb;
            m_ProgDlg.ShowItem(wxString::Format(_("Importing application %ls"), Appl.c_str()));
            // Import schema
            if (Import_appl_param(m_cdp, &ip))
            {
                error_boxp(_("Failed to import application %ls"), GetParent(), Appl.c_str());
                m_ImportOK = false;
            }
            //UpdateGcrLimit(Appl);
        }
    }
    catch (...)
    {
        m_ProgDlg.CategLab->SetLabel(wxEmptyString);
        m_ProgDlg.NameED->SetValue(wxEmptyString);
        m_ProgDlg.ApplED->SetValue(wxEmptyString);
    }
}
//
// Updates replication record change counter
//
void CDuplFil::UpdateGcrLimit(const wxString &Appl)
{
    t_repl_param rp;
    rp.spectab[0]  = 0;
    rp.token_req   = false;
    rp.len16       = 0;

    C602SQLQuery Query(m_cdp);

    try
    {
        Query.Open("SELECT Dest_uuid, Appl_uuid AS Auuid  FROM ReplTab, ObjTab WHERE ReplTab.Appl_uuid=ObjTab.Apl_uuid AND Obj_name='%s' AND Category=Chr(7) AND Tabname IS NULL", (const char *)Appl.mb_str(*m_cdp->sys_conv));
        for (trecnum pos = 0; Query.Read_ind(pos, 1, &rp.server_id); pos++)
        {
            Query.Read_ind(pos, 2, rp.appl_id);
            Repl_control(OPER_SKIP_REPL, sizeof(rp), &rp);
        }
    }
    catch (C602SQLException *e)
    {
        error_boxp(wxT("Due to error:'\n%ls\nreplication record change counter for schema %ls failed"), GetParent(), e->GetMsg(), Appl.c_str());
        delete e;
    }
}
//
// Ensures that logged user is DBAdmin
//
bool CDuplFil::EnsureDBAdmin()
{
    wxString uName = m_UserName;
    // IF empty usern name, user name = Anonymous
    if (uName == wxEmptyString)
        uName = wxT("Anonymous");
    tobjnum usr = Find_objectX(ToChar(uName, m_cdp->sys_conv), CATEG_USER);
    tobjnum gda = Find_objectX("Db_admin", CATEG_GROUP);
    // IF user not id Db_admin group, put him to group, store user name
    bool Result = Get_group_role(usr, gda, CATEG_GROUP);
    if (!Result)
    {
        m_NoAdmin = uName;
        Set_group_role(usr, gda, CATEG_GROUP, true);
    }
    return Result;
}
//
// Removes user that was not DBAdmin from Db_admin group
//
void CDuplFil::RemoveDBAdmin()
{
    tobjnum usr  = Find_objectX(ToChar(m_NoAdmin, m_cdp->sys_conv),  CATEG_USER);
    tobjnum gda  = Find_objectX("Db_admin", CATEG_GROUP);
    Set_group_role(usr, gda, CATEG_GROUP, false);
}
//
// Deletes exported schemas files
//
void CDuplFil::DeleteApplFiles()
{
    // Ask user
    if (!yesno_box(_("Delete exported application files?"), GetParent()))
        return;
    // FOR each exported schema
    for (CApplList::iterator it = m_Appls.begin(); it != m_Appls.end(); it++)
    {
        wxString Appl     = it->first;
        wxString ApplPath = m_ApplsPath + Appl;
        m_ProgDlg.ShowItem(wxString::Format(_("Deleting %ls application files"), Appl.c_str()));
        // Count schema files
        int Cnt;
        wxString f;
        for (f = wxFindFirstFile(ApplPath + wxFileName::GetPathSeparator() + wxT("*.*")), Cnt = 0; !f.IsEmpty(); f = wxFindNextFile(), Cnt++);
        m_ProgDlg.Progress(IMPEXP_PROGMAX, (const wxChar *)(size_t)Cnt);
        // Delete schema files
        for (f = wxFindFirstFile(ApplPath + wxFileName::GetPathSeparator() + wxT("*.*")), Cnt = 0; !f.IsEmpty(); f = wxFindNextFile(), Cnt++)
        {
            wxRemoveFile(f);
            m_ProgDlg.Progress(IMPEXP_PROGSTEP, NULL);
        }
        // Delete schema folder
        wxRmdir(ApplPath);
    }
    m_ProgDlg.ShowItem(_("Deleting user, group & key files"));
    // Delete special tables files
    wxRemoveFile(m_ApplsPath + wxT("KEYTAB.TDT"));
    wxRemoveFile(m_ApplsPath + wxT("USERTAB.TDT"));
    wxRemoveFile(m_ApplsPath + wxT("REPLTAB.TDT"));
    wxRemoveFile(m_ApplsPath + wxT("SRVTAB.TDT"));
    wxRemoveFile(m_ApplsPath + wxT("PROPTAB.TDT"));
    // IF folder for exported schemas did not exist, delete it
    if (!m_WasApplsPath)
        wxRmdir(m_ApplsPath);
}
//
// Creates temporary admin, returns new admin objnum, name and password
//
tobjnum CDuplFil::CreateTmpAdmin(char *TmpUserN, char *TmpUserP)
{
    // Set password as current time
    int2str(wxGetLocalTime(), TmpUserP);
    tobjnum Result;
    // Get free user name if form _n
    for (int i = 0; ;i++)
    {
        sprintf(TmpUserN, "_%d", i);
        if (!cd_Create_user(m_cdp, TmpUserN, NULL, NULL, NULL, NULL, NULL, TmpUserP, &Result))
            break;
        if (Sz_error() != KEY_DUPLICITY)
            throw new C602SQLException(m_cdp, wxT("Create_user"));
    }
    // Set new user as Db_admin, Config_admin, Security_admin
    tobjnum Adm = Find_objectX("Db_admin", CATEG_GROUP);
    Set_group_role(Result, Adm, CATEG_GROUP, true);
    Adm = Find_objectX("Config_admin", CATEG_GROUP);
    Set_group_role(Result, Adm, CATEG_GROUP, true);
    Adm = Find_objectX("Security_admin", CATEG_GROUP);
    Set_group_role(Result, Adm, CATEG_GROUP, true);
    return Result;
}
//
// Initializes timeout for starting or stopping server
//
void CDuplFil::TimeoutInit(wxString Msg, wxString BreakMsg)
{
    m_Msg        = Msg;                             // Set message for explaining operation
    m_BreakMsg   = BreakMsg;                        // Set message for explaining operation termination
    m_MsgTime    = wxGetLocalTime() + 10;           // Set m_Msg timeout
    m_BreakTime  = m_MsgTime + 60;                  // Set termination timeout
}
//
// Waiting for starting or stopping server
//
void CDuplFil::TimeoutWait()
{
    long CurTime = wxGetLocalTime();                // IF now > termination timeout
    if (CurTime >= m_BreakTime)                     //    Interrupt execution with m_BreakMsg
        throw new CWBException(m_BreakMsg.c_str());
    if (CurTime >= m_MsgTime)                       // IF now > m_Msg 
    {
        if (m_ProgDlg.IsShown())                    //    Explain delay
            m_ProgDlg.ShowItem(m_Msg);
        m_MsgTime = m_BreakTime;
    }
#ifdef WINS                                         // Sleep for a moment
    Sleep(20);
#else
    wxThread::This()->Yield();
#endif    
}
//
// Finds objects in schema with "do not export" or "do not export data" flags, stores object state to CObjDesc
// and enables export;
//
void CDuplFil::GetExportable(WBUUID UUID, CObjDescs *od)
{
    C602SQLQuery Curs(m_cdp);
    wxString sUUID = UUID2str(UUID);
    // Find not exportable tables
    Curs.Open("SELECT Tab_name, category, Flags FROM TABTAB WHERE Apl_uuid=X'%s' AND Flags != NULL AND ((Flags MOD 128) >= 64 OR (Flags MOD 32) >= 16)", sUUID.c_str());
    trecnum tc = Curs.Rec_cnt();
    if (tc > 0)
    {
        // Set CObjDesc list size
        od->SetCount(tc);
        // FOR each not exportable table
        for (trecnum pos = 0; pos < tc; pos++)
        {
            CObjDesc *Desc = od->GetObjDesc(pos);
            // Store object state
            Curs.Read_record(pos, Desc, sizeof(CObjDesc));
            // Reset no export flag
            short Flag = Desc->Flags & ~(CO_FLAG_NOEXPORT | CO_FLAG_NOEXPORTD);
            Curs.Write_ind(pos, 3, &Flag, sizeof(Flag));
        }
    }
    Curs.Close_cursor();
    // Find not exportable objects
    Curs.Open("SELECT Obj_name, category, Flags FROM OBJTAB WHERE Apl_uuid=X'%s' AND Flags != NULL AND(Flags MOD 128) >= 64", sUUID.c_str());
    trecnum oc = Curs.Rec_cnt();
    if (oc > 0) 
    {
        // Set CObjDesc list size
        od->SetCount(tc + oc);
        // FOR each not exportable object
        for (trecnum pos = 0; pos < oc; pos++)
        {
            CObjDesc *Desc = od->GetObjDesc(tc + pos);
            // Store object state
            Curs.Read_record(pos, Desc, sizeof(CObjDesc));
            // Reset no export flag
            short Flag = Desc->Flags & ~CO_FLAG_NOEXPORT;
            Curs.Write_ind(pos, 3, &Flag, sizeof(Flag));
        }
    }
}
//
// Restores state of not exportable objects
//
void CDuplFil::SetExportable(WBUUID UUID, CObjDescs *ObjDescs)
{
    if (!ObjDescs)
        return;
    try
    {
        const char *tb;
        const char *no;
        char su[2 * sizeof(WBUUID) + 1];
        bin2hex(su, UUID, sizeof(WBUUID));
        su[2 * sizeof(WBUUID)] = 0;
        // FOR each not exportable object
        for (int i = 0; i < ObjDescs->GetCount(); i++)    
        {
            CObjDesc *ObjDesc = ObjDescs->GetObjDesc(i);
            if (ObjDesc->Categ == CATEG_TABLE) 
            {
                tb = "TABTAB";
                no = "Tab_name";
            }
            else
            {
                tb = "OBJTAB";
                no = "Obj_name";
            }
            char Cmd[256];
            // Create UPDATE statement
            sprintf(Cmd, "UPDATE %s SET Flags=%d WHERE Apl_uuid=X'%s' AND %s='%s' AND Category=Chr(%d)", tb, ObjDesc->Flags, su, no, ObjDesc->Name, ObjDesc->Categ);
            // Update object state
            SQL_execute(Cmd);
        }
    }
    catch (CWBException *e)
    {
        error_box(e->GetMsg(), GetParent());
        delete e;
    }
    catch (...)
    {
    }
}
//
// Clears schema descriptors
//
void CDuplFil::ResetAppls()
{
    for (CApplList::iterator it = m_Appls.begin(); it != m_Appls.end(); it++)
        delete it->second;
}
//
// Creates needed folders if they not exist
//
void CDuplFil::ForceDirectories(wxString Dir)
{
    if (!wxDirExists(Dir))
    {
        if (Dir.Last() == wxFileName::GetPathSeparator())
            Dir = Dir.RemoveLast();
        ForceDirectories(Dir.BeforeLast(wxFileName::GetPathSeparator()));
        if (!wxMkdir(Dir))
            throw new CSysException(wxT("wxMkDir"), _("Failed to create folder %ls"), Dir.c_str());
    }
}
//
// Returns server property value
//
wxVariant CServerProps::GetProp(const wxString &PropName)
{
    // IF registration properties
    if (m_Section.CmpNoCase(wxT("Database")) == 0)
    {
#ifdef WINS
        // Open server key
        OpenDBKey(m_SubSection);
        BYTE  Buf[512];
        DWORD BufSize = sizeof(Buf);
        DWORD Type;
        // Read property value
        DWORD Err = RegQueryValueEx(m_hKey, PropName, NULL, &Type, Buf, &BufSize);
        if (Err != ERROR_SUCCESS)
            throw new CSysException(wxT("RegOpenKeyEx"), Err, RegReadFailed, PropName.c_str(), wxString(DBKey + m_SubSection).c_str());
        if (Type == REG_SZ)
            return wxVariant(wxString((wxChar *)Buf));
        return wxVariant(*(long *)Buf);
#else
        // Read value from ini file
        char Val[256];
        if (GetDatabaseString(m_SubSection.mb_str(*wxConvCurrent), PropName.mb_str(*wxConvCurrent), Val, sizeof(Val), false))
            return wxVariant(wxString(Val, *wxConvCurrent));
        else
            return wxVariant(wxEmptyString);
#endif
    }
    return wxVariant();
}
//
// Returns server property value as string
//
wxString CServerProps::GetPropStr(const wxString &PropName)
{
    // IF property owner is @SQLSERVER, Get_property_value
    if (m_Section.CmpNoCase(wxT("@SQLSERVER")) == 0)
    {
        char Val[256];
        Get_property_value(ToChar(m_Section, m_cdp->sys_conv), ToChar(PropName, m_cdp->sys_conv), Val, sizeof(Val));
        return wxString(Val, *m_cdp->sys_conv);
    }
    // IF registration property
    if (m_Section.CmpNoCase(wxT("Database")) == 0)
    {
#ifdef WINS
        // Open server key
        OpenDBKey(m_SubSection);
        BYTE  Buf[512];
        DWORD BufSize = sizeof(Buf);
        DWORD Type;
        // Read property value
        DWORD Err = RegQueryValueEx(m_hKey, PropName, NULL, &Type, Buf, &BufSize);
        if (Err == ERROR_FILE_NOT_FOUND)
            return wxEmptyString;
        if (Err != ERROR_SUCCESS)
            throw new CSysException(wxT("RegOpenKeyEx"), Err, RegReadFailed, PropName.c_str(), wxString(DBKey + m_SubSection).c_str());
        return wxString((wxChar *)Buf);
#else
        char Val[256];
        // Read value from ini file
        if (GetDatabaseString(m_SubSection.mb_str(*wxConvCurrent), PropName.mb_str(*wxConvCurrent), Val, sizeof(Val), false))
            return wxString(Val, *wxConvCurrent);
        else
            return wxEmptyString;
#endif
    }
    return wxEmptyString;
}
//
// Returns server property value as boolean
//
bool CServerProps::GetPropBool(const wxString &PropName)
{
    // IF property owner is @SQLSERVER, Get_property_value
    if (m_Section.CmpNoCase(wxT("@SQLSERVER")) == 0)
    {
        char Value[16];
        return Get_property_value(ToChar(m_Section, m_cdp->sys_conv), ToChar(PropName, m_cdp->sys_conv), Value, sizeof(Value)) != 0 && *Value != 0 && *Value != '0';
    }
    return false;
}
//
// Returns server property value as integer
//
int CServerProps::GetPropInt(const wxString &PropName)
{
    // IF property owner is @SQLSERVER, Get_property_value
    if (m_Section.CmpNoCase(wxT("@SQLSERVER")) == 0)
    {
        char Value[16];
        if (Get_property_value(ToChar(m_Section, m_cdp->sys_conv), ToChar(PropName, m_cdp->sys_conv), Value, sizeof(Value)) == 0)
            return NONEINTEGER;
        return atoi(Value);
    }
    return NONEINTEGER;
}
//
// Sets server property value
//
void CServerProps::SetProp(const wxString &PropName, const wxVariant &Value)
{
    // IF registration property
    if (m_Section.CmpNoCase(wxT("Database")) == 0)
    {
#ifdef WINS
        // IF value type is string, set string value ELSE set long value
        if (Value.GetType().CmpNoCase(wxT("string")) == 0)
            SetProp(PropName, Value.GetString());
        else
            SetProp(PropName, Value.GetLong());
#else
        // Write value to ini file
        if (!SetDatabaseString(m_SubSection.mb_str(*wxConvCurrent), PropName.mb_str(*wxConvCurrent), Value.GetString().mb_str(*wxConvCurrent), false))
            throw new CSysException(wxT("WritePrivateProfileString"), WriteIniFailed, wxT("/etc/602sql"));
#endif
    }
}
//
// Sets server property string value
//
void CServerProps::SetProp(const wxString &PropName, const wxString &Value)
{
    // IF property owner is @SQLSERVER, Set_property_value
    if (m_Section.CmpNoCase(wxT("@SQLSERVER")) == 0)
        Set_property_value(ToChar(m_Section, m_cdp->sys_conv), ToChar(PropName, m_cdp->sys_conv), ToChar(Value, wxConvCurrent));
    // IF registration property
    else if (m_Section.CmpNoCase(wxT("Database")) == 0)
    {
#ifdef WINS
        // Open server key
        OpenDBKey(m_SubSection);
        // Write string value
        DWORD Err = RegSetValueEx(m_hKey, PropName, 0, REG_SZ, (const uns8 *)Value.c_str(), (DWORD)(Value.Length() * 1) * (int)sizeof(wxChar));
        if (Err != ERROR_SUCCESS)
            throw new CSysException(wxT("RegSetValueEx"), Err, RegWriteFailed, PropName, wxString(DBKey + m_SubSection).c_str());
#else
        // Write value to ini file
        if (!SetDatabaseString(m_SubSection.mb_str(*wxConvCurrent), PropName.mb_str(*wxConvCurrent), Value.mb_str(*wxConvCurrent), false))
            throw new CSysException(wxT("WritePrivateProfileString"), WriteIniFailed, wxT("/etc/602sql"));
#endif
    }
}
//
// Sets server property boolean value
//
void CServerProps::SetProp(const wxString &PropName, bool Value)
{
    // IF property owner is @SQLSERVER, Set_property_value
    if (m_Section.CmpNoCase(wxT("@SQLSERVER")) == 0)
        Set_property_value(ToChar(m_Section, m_cdp->sys_conv), ToChar(PropName, m_cdp->sys_conv), Value);
}
//
// Sets server property boolean value
//
void CServerProps::SetProp(const wxString &PropName, int Value)
{
    // IF property owner is @SQLSERVER, Set_property_value
    if (m_Section.CmpNoCase(wxT("@SQLSERVER")) == 0)
        Set_property_value(ToChar(m_Section, m_cdp->sys_conv), ToChar(PropName, m_cdp->sys_conv), Value);
    // IF registration property
    else if (m_Section.CmpNoCase(wxT("Database")) == 0)
    {
#ifdef WINS
        // Write DWORD value
        DWORD Err = RegSetValueEx(m_hKey, PropName, 0, REG_DWORD, (LPBYTE)&Value, sizeof(int));
        if (Err != ERROR_SUCCESS)
            throw new CSysException(wxT("RegSetValueEx"), Err, RegWriteFailed, PropName, wxString(DBKey + m_SubSection).c_str());
#endif
    }
}
//
// Returns list of property names
//
wxArrayString *CServerProps::GetPropNames()
{
#if WBVERS<1100
    // IF registration properties
    if (m_Section.CmpNoCase(wxT("Database")) == 0)
    {
        wxArrayString *Result = new wxArrayString();
        if (!Result)
            throw new CNoMemoryException();
#ifdef WINS
        // Open server key
        OpenDBKey(m_SubSection);
        wxChar Name[256];
        DWORD  NameSize;
        // Add each value name to list
        for (DWORD i = 0; ; i++)
        {
            NameSize = sizeof(Name);
            DWORD Err = RegEnumValue(m_hKey, i, Name, &NameSize, NULL, NULL, NULL, NULL);
            if (Err == ERROR_NO_MORE_ITEMS)
                break;
            if (Err)
                throw new CSysException(wxT("RegEnumValue"), Err, RegEnumVFailed, wxString(DBKey + m_SubSection).c_str());
            Result->Add(Name);
        }
#else
        CBuffer Names;
        // Read server section value names from ini to buffer Names
        for (unsigned Size = 256; ; Size += 256)
        {
            Names.AllocX(Size);
            if (GetPrivateProfileString(m_SubSection.mb_str(*wxConvCurrent), NULL, "", Names, Size, configfile) < Size - 2)
                break;
        }
        // Copy Names to list
        for (char *p = Names; *p; p = p + strlen(p) + 1)
            Result->Add(wxString(p,*wxConvCurrent)); 
#endif
        return Result;
    }
#endif
    return NULL;
}
//
// Returns list of property subsection names i.e. list of registered servers
//
wxArrayString *CServerProps::GetSubSectNames()
{
#if WBVERS<1100
  // IF registration properties
    if (m_Section.CmpNoCase(wxT("Database")) == 0)
    {
        wxArrayString *Result = new wxArrayString();
        if (!Result)
            throw new CNoMemoryException();
#ifdef WINS
        // Open server key
        OpenDBKey(wxEmptyString);
        wxChar Name[256];
        DWORD  NameSize;
        // Add each subkey name to list
        for (DWORD i = 0; ; i++)
        {
            NameSize = sizeof(Name);
            DWORD Err = RegEnumKeyEx(m_hKey, i, Name, &NameSize, NULL, NULL, NULL, NULL);
            if (Err == ERROR_NO_MORE_ITEMS)
                break;
            if (Err)
                throw new CSysException(wxT("RegEnumKeyEx"), Err, RegEnumKFailed, wxString(DBKey));
            Result->Add(Name);
        }
#else
        CBuffer Names;
        // Read section value names from ini to buffer Names
        for (unsigned Size = 256; ; Size += 256)
        {
            Names.AllocX(Size);
            if (GetPrivateProfileString(NULL, NULL, "", Names, Size, configfile) < Size - 2)
                break;
        }
        // Add each section name not startin with . tao list
        for (char *p = Names; *p; p = p + strlen(p) + 1)
            if (*p != '.')
                Result->Add(wxString(p, *wxConvCurrent)); 
#endif
        return Result;
    }
#endif
    return NULL;
}
//
// Create property subsection i.e. server registration root
//
void CServerProps::CreateSubSection(const wxString &SubSect)
{
#ifdef WINS
    // IF registration properties
    if (m_Section.CmpNoCase(wxT("Database")) == 0)
    {
        wxString Key = wxT("SOFTWARE\\Software602\\602SQL\\Database\\") + SubSect;
        // Create server subkey
        DWORD    Err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, Key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_hKey, NULL);
        if (Err != ERROR_SUCCESS)
            throw new CSysException(wxT("RegCreateKeyEx"), Err, RegCreateFailed, Key.c_str());
    }
#endif
    // Set current subsection
    SetSubSection(SubSect);
}

#ifdef WINS
//
// Opens server key in registry
//
void CServerProps::OpenDBKey(const wxString &Server)
{
    if (!m_hKey)
    {
        wxString Key = DBKey + Server;
        DWORD    Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key, 0, KEY_ALL_ACCESS, &m_hKey);
        if (Err != ERROR_SUCCESS)
            throw new CSysException(wxT("RegOpenKeyEx"), Err, RegOpenFailed, Key.c_str());
    }
}

#endif

