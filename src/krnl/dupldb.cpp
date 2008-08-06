// dupldb.cpp - duplication of the whole database file
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#include "objlist9.h"
#include "replic.h"
#include "enumsrv.h"

#define DUPLDB
#ifdef DUPLDB

//#include "api.h"

#pragma pack(1)
//
// Descriptor of not exportable object
//
struct CObjDesc
{
    tobjname Name;          // Object name
    tcateg   Categ;         // Object category
    short    Flags;         // Object flags
};
#pragma pack()
//
// Schema descriptor
//
class CObjDescs
{
public:
    tobjname owning_schema;
    CObjDescs * next;

    WBUUID    m_FRCSrv;     // Parent server for replication conflicts UUID
    WBUUID    m_TASrv;      // Token administrator server UUID
    bool      m_ExpOK;      // Schema export OK
    bool      everybody_is_junior;  // everybody group is in the junior_user role
    int       m_Count;      // Count of not exportable objects
    CObjDesc *m_Arr;        // Not exportable objects list

    CObjDesc *GetObjDesc(int Index){return &m_Arr[Index];}
    int       GetCount(){return m_Count;}
    void      SetCount(int Value)
    {
        m_Arr = (CObjDesc *)corerealloc(m_Arr, Value * sizeof(CObjDesc));
        if (!m_Arr)
            throw new CNoMemoryException();
        m_Count = Value;
    }

    CObjDescs(){m_Count = 0; m_Arr = NULL; m_ExpOK = false; next=NULL; }
    ~CObjDescs()
    {
        if (m_Arr)
            corefree(m_Arr);
    }
};

/*!
 * CDuplFil class declaration
 */

class CDuplFil
{public: 
  cdp_t m_cdp;   

    CObjDescs * m_Appls;        // list of schema descriptors
    LPIMPEXPAPPLCALLBACK callback;
    void * callback_param;

    tobjname m_SrvName;      // Original server name
    char * m_ApplsPath;    // Folder for exported schemas
    tobjname m_UserName;     // Admin name
    char * m_Password;     // Admin password
/*    
    wxString        m_Msg;          // Message that explains long time operation
    wxString        m_BreakMsg;     // Message that explains long time operation terminating
    wxString        m_Domain; */      // ThrustedDomain property value
    int             m_MinPwLen;     // MinPasswordLen property value
    int             m_PwExpir;      // PasswordExpiration property value
    long            m_MsgTime;      // Timeout for displaying m_Msg 
    long            m_BreakTime;    // Timeout for break long time opration
    bool            m_DsbleAnn;     // DisableAnonymous property value
    bool            m_UnlimPw;      // UnlimitedPasswords property value
    bool            m_EncReq;       // ReqCommEnc property value
    bool            m_OwnGroup;     // ConfAdmsOwnGroup property value
    bool            m_UserGroup;    // ConfAdmsUserGroups property value
    bool            m_WasRepl;      // Original replication state
    bool            m_Connected;    // Server is connected
    bool            m_ImportOK;     // Fil duplication OK
    bool            m_Locked;       // Server is locked
    wchar_t msg_transl_buf[256];

//    void GetExportable(WBUUID UUID, CObjDescs *od);
    void ExportAppls();
    int  ExportUsers();
    int  ExportRepl();
    void BackupServer();
    int  ImportUsers();
    int  ImportRepl();
    void ImportAppls();
    void UpdateGcrLimit(const char * Appl);
    void DeleteApplFiles();
    void TimeoutInit(const char *  Msg, const char *  BreakMsg);
    void TimeoutWait();
//    void SetExportable(WBUUID UUID, CObjDescs *ObjDescs);
    void ResetAppls();
    tobjnum CreateTmpAdmin(char *TmpUserN, char *TmpUserP);

public:
    /// Constructors
    CDuplFil(cdp_t cdp, const char * SrvName, const char * ApplsPath, LPIMPEXPAPPLCALLBACK callbackIn, void * callback_paramIn)
      : callback(callbackIn), callback_param(callback_paramIn)
    {   m_cdp=cdp;
        strmaxcpy(m_SrvName, SrvName, sizeof(m_SrvName));
        m_ApplsPath = (char*)corealloc(strlen(ApplsPath)+1, 88);  strcpy(m_ApplsPath, ApplsPath);
        m_Connected = false;
        m_Locked    = false;
        m_Appls=NULL;
    }
    virtual ~CDuplFil();
    void ReportProgress(int Categ, const char * Value);
};
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Creates needed folders if they not exist
//

bool make_directory_path(char *  dir)
{ bool failed = false;
  int i=0;
#ifdef WINS
  if (*dir && dir[1]==':') i=2;  // skip D: on the beginning
  else if (*dir=='\\' && dir[1]=='\\')  // or skip \\share
  { i=2;
    while (dir[i] && dir[i]!='\\') i++;
  }  
#endif
 // cycle on directories
  do
  { if (dir[i]==PATH_SEPARATOR) i++;
    bool new_dir = false;
    while (dir[i] && dir[i]!=PATH_SEPARATOR) 
      { new_dir=true;  i++; }
    if (!new_dir) break;
    bool cont = dir[i]==PATH_SEPARATOR;
    dir[i]=0;  // temporary terminator of the prefix of the path
    if (!CreateDirectory(dir, NULL)  
#ifdef WINS
        && GetLastError() != ERROR_ALREADY_EXISTS
#endif    
      ) failed=true;  // do not stop creating, may not detect some ERROR_ALREADY_EXISTS
    if (cont) dir[i]=PATH_SEPARATOR; // return the separator
  } while (true);   
  return !failed;
}

void CDuplFil::ReportProgress(int Categ, const char * Value)
{
  if (callback)
    callback(Categ, (void *)Value, callback_param);
}

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
#ifdef STOP
//
// Performs intrinsic duplication
//
void CDuplFil::Perform()
{
    try
    {
        // Export schemas
        ExportAppls();
        // Export users and keys
        ExportUsers();
        // Export replication servers & relations
        ExportRepl();

        // Backup original server
        BackupServer();

        // Start new server
        StartServer();
		cdp_t cdp = m_cdp;
		m_cdp = cdp_alloc();
		cdp_free(cdp);
        // Logon as anonymous
        Connect("", "");
        // Lock server
        Lock_server();
        m_Locked = true;

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
        Connect("", "");
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
        cd_Login(m_cdp, m_UserName, m_Password);
        // FOR each schema
        for (CObjDescs *NoExp = m_Appls;  NoExp;  NoExp=NoExp->next)
        {
          WBUUID ApplUUID;
          // Restore not exportable objects
          tobjnum ApplObj;
          cd_Find_object(cdp, NoExp->owning_schema, CATEG_APPL, &ApplObj);
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
        // Delete temporary admin
        Delete(USER_TABLENUM, TmpUserO);

        // Delete exported schemas files
        DeleteApplFiles();
        if (m_ImportOK)
          ReportProgress(IMPEXP_FILENAME, "Database duplication was successful");
    }
    // Unlock server and stop it:
    StopServer(false);
}
#endif

#ifdef STOP
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
            ReportProgress(IMPEXP_FILENAME, "Stopping replication");
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
#endif


#ifdef STOP
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
#endif
//
// Exports all schemas
//
void CDuplFil::ExportAppls()
{ char path[MAX_PATH];
    CObjDescs  *NoExp     = NULL;
        // Set export schema parametres
        t_export_param ep;
        ep.cbSize            = sizeof(t_export_param);
        ep.hParent           = 0;
        ep.with_data         = 1 | 0x80;  // with data and ignore NOEXPORT and NOEXPORTDATA flags
        ep.with_role_privils = true;
        ep.with_usergrp      = true;
        ep.back_end          = false;
        ep.date_limit        = 0;
        ep.long_names        = true;
        ep.report_progress   = true;
        ep.callback          = callback;
        ep.param             = callback_param;
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
            // Set schema as current schema
            cd_Set_application_ex(m_cdp, Appl, false);
            strcpy(path, m_ApplsPath);  append(path, Appl);
            ReportProgress(CATEG_APPL, Appl);
            // Get schema objnum
            tobjnum ApplObj;
            cd_Find_object(m_cdp, Appl, CATEG_APPL, &ApplObj);
            apx.appl_locked        = false;
            // Read schema header
            cd_Read_var(m_cdp, OBJ_TABLENUM, ApplObj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &apx, NULL);
            ep.exp_locked          = apx.appl_locked;
            // Read schema UUID
            cd_Read(m_cdp, OBJ_TABLENUM, ApplObj, APPL_ID_ATR, NULL, ApplUUID);
            // Create new schema descriptor
            NoExp = new CObjDescs();
            strcpy(NoExp->owning_schema, Appl);
            // Ensure all objects
//            GetExportable(ApplUUID, NoExp);
           // Store replication conflict parent and token admin server UUID
            if (!IsNullUUID(apx.os_uuid) || !IsNullUUID(apx.dp_uuid))
            {
                memcpy(NoExp->m_FRCSrv, apx.os_uuid, sizeof(WBUUID));
                memcpy(NoExp->m_TASrv,  apx.dp_uuid, sizeof(WBUUID));
            }
           // store information if EVERYBODY is in the JUNIOR_USER role:
            tobjnum junior_role;  uns32 rel; 
            NoExp->everybody_is_junior=true;  // the default
            if (!cd_Find2_object(m_cdp, "JUNIOR_USER", ApplUUID, CATEG_ROLE, &junior_role)) 
              if (!cd_GetSet_group_role(m_cdp, EVERYBODY_GROUP, junior_role, CATEG_ROLE, OPER_GET, &rel)) 
                if (!rel)
                  NoExp->everybody_is_junior=false;
            // Create folders for exported files if not exist
            make_directory_path((char*)path);
            append(path, Appl);  strcat(path, ".apl");
            ep.file_name    = path;
            ep.schema_name  = Appl;
            NoExp->next=m_Appls;  m_Appls=NoExp;
            NoExp->m_ExpOK  = Export_appl_param(m_cdp, &ep) != false;
//            SetExportable(ApplUUID, NoExp);
            NoExp           = NULL;
        }
}
//
// Exports users and keys
//
int CDuplFil::ExportUsers()
{ char path[MAX_PATH];
 // Show progress in progress dialog
  ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Exporting list of users, groups & keys"), msg_transl_buf));

  // Export users to USERTAB.TDT
  ReportProgress(IMPEXP_PROGMAX, (const char *)2);
  strcpy(path, m_ApplsPath);  append(path, "USERTAB.TDT");
  FHANDLE hFile  = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile == INVALID_FHANDLE_VALUE)
    return DD_ERR_CREATE_FILE;
  ReportProgress(IMPEXP_FILENAME, path);
  if (cd_Export_user(m_cdp, NOOBJECT, hFile))
    { CloseHandle(hFile);  return cd_Sz_error(m_cdp); }
  CloseHandle(hFile);
  
  // Export keys to KEYTAB.TDT
  ReportProgress(IMPEXP_PROGSTEP, NULL);
  strcpy(path, m_ApplsPath);  append(path, "KEYTAB.TDT");
  hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile == INVALID_FHANDLE_VALUE)
    return DD_ERR_CREATE_FILE;
  ReportProgress(IMPEXP_FILENAME, path);
  if (cd_xSave_table(m_cdp, KEY_TABLENUM, hFile))
    { CloseHandle(hFile);  return cd_Sz_error(m_cdp); }
 // Show progress in progress dialog
  ReportProgress(IMPEXP_PROGSTEP, NULL);
  ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Successfully exported users, groups & keys"), msg_transl_buf));
  CloseHandle(hFile);
  return 0;
}
//
// Exports replication servers and rules and server properties
//
int CDuplFil::ExportRepl()
{ char path[MAX_PATH];  trecnum cnt;  FHANDLE hFile;
 // IF any replication servers registered:
  cd_Rec_cnt(m_cdp, SRV_TABLENUM, &cnt);
  if (cnt > 1)
  { // Show progress in progress dialog
    ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Exporting replication servers & relations"), msg_transl_buf));
    ReportProgress(IMPEXP_PROGMAX, (const char *)2);
    // Export replication servers to SRVTAB.TDT
    strcpy(path, m_ApplsPath);  append(path, "SRVTAB.TDT");
    hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_FHANDLE_VALUE)
      return DD_ERR_CREATE_FILE;
    ReportProgress(CATEG_TABLE,     "SRVTAB");
    ReportProgress(IMPEXP_FILENAME, path);
    if (cd_xSave_table(m_cdp, SRV_TABLENUM, hFile))
      { CloseHandle(hFile);  return cd_Sz_error(m_cdp); }
    CloseHandle(hFile);
    ReportProgress(IMPEXP_PROGSTEP, NULL);

    // Export replication rules to REPLTAB.TDT
    strcpy(path, m_ApplsPath);  append(path, "REPLTAB.TDT");
    hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_FHANDLE_VALUE)
      return DD_ERR_CREATE_FILE;
    ReportProgress(CATEG_TABLE,     "REPLTAB");
    ReportProgress(IMPEXP_FILENAME, path);
    if (cd_xSave_table(m_cdp, REPL_TABLENUM, hFile))
      { CloseHandle(hFile);  return cd_Sz_error(m_cdp); }
    CloseHandle(hFile);
    ReportProgress(IMPEXP_PROGSTEP, NULL);
  }
 // Export server properties to PROPTAB.TDT
  strcpy(path, m_ApplsPath);  append(path, "PROPTAB.TDT");
  hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile == INVALID_FHANDLE_VALUE)
    return DD_ERR_CREATE_FILE;
  ReportProgress(CATEG_TABLE,     "__PROPTAB");
  ReportProgress(IMPEXP_FILENAME, path);
  if (cd_xSave_table(m_cdp, PROP_TABLENUM, hFile))
    { CloseHandle(hFile);  return cd_Sz_error(m_cdp); }
  CloseHandle(hFile);
  ReportProgress(IMPEXP_PROGSTEP, NULL);
 // Show progress in progress dialog
  ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Successfully exported replication servers & relations"), msg_transl_buf));
  return 0;
}

#ifdef STOP
//
// Backups original server
//
void  CDuplFil::BackupServer()
{
    CServerProps SrcProps(wxT("Database"), m_SrvName);
    CServerProps DstProps(wxT("Database"));
    // Show progress in progress dialog
    ReportProgress("Creating backup of original database");
    ReportProgress(IMPEXP_PROGMAX, (const char *)3);
    // Stop server
    StopServer(true);
    ReportProgress(IMPEXP_PROGSTEP, NULL);
    // Get fil path
    wxString Path = SrcProps.GetPropStr(wxT("Path"));
    if (Path.IsEmpty())
        Path = SrcProps.GetPropStr(wxT("Path1"));
    if (Path.Last() != wxFileName::GetPathSeparator())
        Path += wxFileName::GetPathSeparator();
    Path += FilName;
    // Create folder for fil backup if not exists
    make_directory_path(m_BackPath);
    // Be sure that backup fil not exists
    if (m_BackPath.Last() != wxFileName::GetPathSeparator())
        m_BackPath += wxFileName::GetPathSeparator();
    wxString BackPath = m_BackPath + FilName;
    wxRemoveFile(BackPath);
    // Move fil to backup folder
    if (!wxRenameFile(Path, BackPath))
        throw new CSysException(wxT("wxRenameFile"), _("Cannot move database file to %ls"), BackPath.c_str());
    ReportProgress(IMPEXP_PROGSTEP, NULL);

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
        ReportProgress(IMPEXP_PROGSTEP, NULL);
        ReportProgress("Successfully created original database backup");
        delete PropNames;
    }
    catch (...)
    {
        delete PropNames;
        throw;
    }
}
#endif

//
// Imports users and keys
//
int CDuplFil::ImportUsers()
{ char path[MAX_PATH];
 // Show progress in progress dialog
  ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Importing list of users, groups & keys"), msg_transl_buf));

  ReportProgress(IMPEXP_PROGMAX, (const char *)2);
 // Import keys from KEYTAB.TDT
  strcpy(path, m_ApplsPath);  append(path, "KEYTAB.TDT");
  FHANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile == INVALID_FHANDLE_VALUE)
    return DD_ERR_CREATE_FILE;
  ReportProgress(IMPEXP_FILENAME, path);
  if (cd_Restore_table(m_cdp, KEY_TABLENUM, hFile, false))
    { CloseHandle(hFile);  return cd_Sz_error(m_cdp); }
  CloseHandle(hFile);
  ReportProgress(IMPEXP_PROGSTEP, NULL);

 // Import users from USERTAB.TDT
  strcpy(path, m_ApplsPath);  append(path, "USERTAB.TDT");
  hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile == INVALID_FHANDLE_VALUE)
    return DD_ERR_CREATE_FILE;
  ReportProgress(IMPEXP_FILENAME, path);
  if (cd_Restore_table(m_cdp, TAB_TABLENUM, hFile, false))  // TABTAB represents the whole USERTAB!
    { CloseHandle(hFile);  return cd_Sz_error(m_cdp); }
  // Show progress in progress dialog
  ReportProgress(IMPEXP_PROGSTEP, NULL);
  ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Successfully imported users, groups & keys"), msg_transl_buf));
  CloseHandle(hFile);
  return 0;
}
//
// Imports replication servers and rules and server properties
//
int CDuplFil::ImportRepl()
{ char path[MAX_PATH];
  ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Importing replication servers & relations"), msg_transl_buf));
  ReportProgress(IMPEXP_PROGMAX,  (const char *)2);
 // Import replication servers from SRVTAB.TDT
  strcpy(path, m_ApplsPath);  append(path, "SRVTAB.TDT");
  FHANDLE  hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile != INVALID_FHANDLE_VALUE)
  { ReportProgress(CATEG_TABLE,     "SRVTAB");
    ReportProgress(IMPEXP_FILENAME, path);
    cd_Delete(m_cdp, SRV_TABLENUM, 0);            // Delete local server record
    cd_Free_deleted(m_cdp, SRV_TABLENUM);
    if (cd_Restore_table(m_cdp, SRV_TABLENUM, hFile, false))
      { CloseHandle(hFile);  return cd_Sz_error(m_cdp); }
    CloseHandle(hFile);
  }  
  ReportProgress(IMPEXP_PROGSTEP, NULL);

 // Import replication rules from REPLTAB.TDT
  strcpy(path, m_ApplsPath);  append(path, "REPLTAB.TDT");
  hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile != INVALID_FHANDLE_VALUE)
  { ReportProgress(CATEG_TABLE, "REPLTAB");
    ReportProgress(IMPEXP_FILENAME, path);
    if (cd_Restore_table(m_cdp, REPL_TABLENUM, hFile, false))
      { CloseHandle(hFile);  return cd_Sz_error(m_cdp); }
    CloseHandle(hFile);
  }  
  ReportProgress(IMPEXP_PROGSTEP, NULL);

 // Import server properties from PROPTAB.TDT
  strcpy(path, m_ApplsPath);  append(path, "PROPTAB.TDT");
  hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile != INVALID_FHANDLE_VALUE)
  { ReportProgress(CATEG_TABLE, "__PROPTAB");
    ReportProgress(IMPEXP_FILENAME, path);
    if (cd_Restore_table(m_cdp, PROP_TABLENUM, hFile, false))
      { CloseHandle(hFile);  return cd_Sz_error(m_cdp); }
    CloseHandle(hFile);
  }  
  // Show progress in progress dialog
  ReportProgress(IMPEXP_PROGSTEP, NULL);
  ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Successfully imported replication servers & relations"), msg_transl_buf));
  return 0;
}

//
// Imports all schemas
//
void  CDuplFil::ImportAppls()
{ char path[MAX_PATH];
  // Set import schema parametres
  t_import_param ip;
  ip.cbSize          = sizeof(t_import_param);
  ip.hParent         = 0;
  ip.flags           = IMPAPPL_REPLACE + IMPAPPL_NO_COMPILE + IMPAPPL_WITH_INFO;
  ip.alternate_name  = NULL;
  ip.report_progress = true;
  ip.callback        = callback;
  ip.param           = callback_param;
  m_ImportOK         = true;
  // FOR each exported schema
  for (CObjDescs * NoExp = m_Appls;  NoExp!=NULL;  NoExp=NoExp->next)
  {
    strcpy(path, m_ApplsPath);  append(path, NoExp->owning_schema);  
    append(path, NoExp->owning_schema);  strcat(path, ".apl");
    ReportProgress(CATEG_APPL, NoExp->owning_schema);
    ip.file_name = path;
   // Import schema
    if (Import_appl_param(m_cdp, &ip))
    {
          ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Failed to import application."), msg_transl_buf));
          m_ImportOK = false;
    }
    UpdateGcrLimit(NoExp->owning_schema);
   // JUNIOR_USER and EVERYBODY_GROUP:
    tobjnum junior_role;  uns32 rel=0; 
    if (!NoExp->everybody_is_junior)  // muts change the default
      if (!cd_Find_object(m_cdp, "JUNIOR_USER", CATEG_ROLE, &junior_role)) 
        cd_GetSet_group_role(m_cdp, EVERYBODY_GROUP, junior_role, CATEG_ROLE, OPER_SET, &rel);
  }
}
//
// Updates replication record change counter
//
void CDuplFil::UpdateGcrLimit(const char * Appl)
{ t_repl_param rp;
  rp.spectab[0]  = 0;
  rp.token_req   = false;
  rp.len16       = 0;
  char query[200];  tcursnum curs;  trecnum cnt;
  sprintf(query, "SELECT Dest_uuid, Appl_uuid AS Auuid  FROM ReplTab, ObjTab WHERE ReplTab.Appl_uuid=ObjTab.Apl_uuid AND Obj_name='%s' AND Category=Chr(7) AND Tabname IS NULL", Appl);
  if (cd_Open_cursor_direct(m_cdp, query, &curs))
  { cd_Rec_cnt(m_cdp, curs, &cnt);
    for (trecnum pos = 0;  pos<cnt;  pos++)
    { cd_Read(m_cdp, curs, pos, 1, NULL, rp.server_id);
      cd_Read(m_cdp, curs, pos, 2, NULL, rp.appl_id);
      cd_Repl_control(m_cdp, OPER_SKIP_REPL, sizeof(rp), &rp);
    }
  }
    //  error_boxp(wxT("Due to error:'\n%ls\nreplication record change counter for schema %ls failed"), GetParent(), e->GetMsg(), Appl.c_str());
}
//
// Deletes exported schemas files
//
void CDuplFil::DeleteApplFiles(void)
{ char path[MAX_PATH];
 // FOR each exported schema
  ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Deleting application files"), msg_transl_buf));
  for (CObjDescs * NoExp = m_Appls;  NoExp!=NULL;  NoExp=NoExp->next)
  {
    ReportProgress(IMPEXP_FILENAME, NoExp->owning_schema);
   // Count schema files
    int Cnt = 0;  WIN32_FIND_DATA FindFileData;  HANDLE hFindFile;
    strcpy(path, m_ApplsPath);  append(path, NoExp->owning_schema);  append(path, "*.*");
    hFindFile = FindFirstFile(path, &FindFileData);
    if (hFindFile!=INVALID_HANDLE_VALUE)
    { do Cnt++;
      while (FindNextFile(hFindFile, &FindFileData));
      FindClose(hFindFile);
    }  
    ReportProgress(IMPEXP_PROGMAX, (const char *)(size_t)Cnt);
   // Delete schema files
    strcpy(path, m_ApplsPath);  append(path, NoExp->owning_schema);  append(path, "*.*");
    hFindFile = FindFirstFile(path, &FindFileData);
    if (hFindFile!=INVALID_HANDLE_VALUE)
    { do 
      { if (*FindFileData.cFileName != '.')
          DeleteFile(FindFileData.cFileName);
        ReportProgress(IMPEXP_PROGSTEP, NULL);
      }
      while (FindNextFile(hFindFile, &FindFileData));
      FindClose(hFindFile);
    }  
   // Delete schema folder
    strcpy(path, m_ApplsPath);  append(path, NoExp->owning_schema);  
#ifdef WINS
    RemoveDirectory(path);
#else
    rmdir(path);
#endif    
  }
 // Delete special tables files
  ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Deleting user, group & key files"), msg_transl_buf));
  strcpy(path, m_ApplsPath);  append(path, "KEYTAB.TDT");
  DeleteFile(path);
  strcpy(path, m_ApplsPath);  append(path, "USERTAB.TDT");
  DeleteFile(path);
  strcpy(path, m_ApplsPath);  append(path, "REPLTAB.TDT");
  DeleteFile(path);
  strcpy(path, m_ApplsPath);  append(path, "SRVTAB.TDT");
  DeleteFile(path);
  strcpy(path, m_ApplsPath);  append(path, "PROPTAB.TDT");
  DeleteFile(path);
}
//
// Creates temporary admin, returns new admin objnum, name and password
//
tobjnum CDuplFil::CreateTmpAdmin(char *TmpUserN, char *TmpUserP)
{ tobjnum my_user_objnum;
 // Set password as current time
  int2str(Now(), TmpUserP);
 // Get unused user name if form _n
  for (int i = 0; ;i++)
  {
      sprintf(TmpUserN, "_%d", i);
      if (!cd_Create_user(m_cdp, TmpUserN, NULL, NULL, NULL, NULL, NULL, TmpUserP, &my_user_objnum))
          break;
      if (cd_Sz_error(m_cdp) != KEY_DUPLICITY)
        return NOOBJECT;
  }
 // Set new user as Db_admin, Config_admin, Security_admin:
  tobjnum adm;  uns32 rel=1; 
  if (cd_Find_object(m_cdp, "DB_ADMIN", CATEG_GROUP, &adm)) return NOOBJECT;
  if (cd_GetSet_group_role(m_cdp, my_user_objnum, adm, CATEG_GROUP, OPER_SET, &rel)) return NOOBJECT;
  if (cd_Find_object(m_cdp, "CONFIG_ADMIN", CATEG_GROUP, &adm)) return NOOBJECT;
  if (cd_GetSet_group_role(m_cdp, my_user_objnum, adm, CATEG_GROUP, OPER_SET, &rel)) return NOOBJECT;
  if (cd_Find_object(m_cdp, "SECURITY_ADMIN", CATEG_GROUP, &adm)) return NOOBJECT;
  if (cd_GetSet_group_role(m_cdp, my_user_objnum, adm, CATEG_GROUP, OPER_SET, &rel)) return NOOBJECT;
  return my_user_objnum;
}

#ifdef STOP
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
#endif

//
// Clears schema descriptors
//
void CDuplFil::ResetAppls()
{
  while (m_Appls!=NULL)
  { CObjDescs * NoExp = m_Appls;
    m_Appls = NoExp->next;
    delete NoExp;
  }  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Ensures that logged user has exclusive access to server
// No other clients, no replications, Lock_server succedes
//
int EnsureExclusive(cdp_t cdp)
{
    tcursnum Curs;  trecnum cnt;
    // Check other clients 
    if (!cd_Open_cursor_direct(cdp, "SELECT Client_number FROM _IV_LOGGED_USERS WHERE NOT Own_connection AND NOT Worker_thread", &Curs))
    { if (!cd_Rec_cnt(cdp, Curs, &cnt) && cnt!=0)
      { cd_Close_cursor(cdp, Curs);
        return DD_ERR_CLIENTS;
      }  
      cd_Close_cursor(cdp, Curs);
    }  
    // Stop replications
    wbbool m_WasRepl;
    cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_ACKN, NULL, &m_WasRepl);
    if (m_WasRepl)
    {   wbbool Enb = false;
        cd_Write(cdp, SRV_TABLENUM, 0, SRV_ATR_ACKN, NULL, &Enb, 1);
        cd_Reset_replication(cdp);
    }
    // Lock server
    if (cd_Lock_server(cdp))
      return DD_ERR_CANNOT_LOCK;
    return 0;
}


CFNC DllKernel int WINAPI RebuildDb_export(cdp_t cdp, const char * destdir, LPIMPEXPAPPLCALLBACK callback, void * callback_param, void ** handle)
// cdp is connected to a server a [destdir] allows creating files and subdirectories
{ int res = 0;
  *handle=NULL;
 // Create folders for exported files if not exist
  make_directory_path((char*)destdir);
 // check the admin status, create temp db_admin if necessary:
  bool temp_db_admin = false;  tobjnum my_user_objnum, db_admin_group;
  if (!cd_Am_I_config_admin(cdp))
    return DD_ERR_NOT_CONFIG_ADMIN;
  if (!cd_Am_I_db_admin(cdp))
  { if (!cd_Am_I_security_admin(cdp))
      return DD_ERR_NOT_DB_SECUR_ADMIN;
    const char * my_name = cd_Who_am_I(cdp);
    if (!my_name || !*my_name) return cd_Sz_error(cdp);
    if (cd_Find_object(cdp, my_name, CATEG_USER, &my_user_objnum)) return cd_Sz_error(cdp);
    if (cd_Find_object(cdp, "DB_ADMIN", CATEG_GROUP, &db_admin_group)) return cd_Sz_error(cdp);
    uns32 rel=1; 
    if (cd_GetSet_group_role(cdp, my_user_objnum, db_admin_group, CATEG_GROUP, OPER_SET, &rel)) return cd_Sz_error(cdp);
    temp_db_admin=true;
  }
 // Ensure exclusive access to server, other clients, replications etc.
  res=EnsureExclusive(cdp);
  if (!res)
  { CDuplFil * cdf = new CDuplFil(cdp, cdp->conn_server_name, destdir, callback, callback_param);  
   // Export schemas
    cd_Set_lobrefs_export(cdp, TRUE);   
    cdf->ExportAppls();
    cd_Set_lobrefs_export(cdp, FALSE);   
   // Export users and keys
    cdf->ExportUsers();
   // Export replication servers & relations
    cdf->ExportRepl();
   // save internal server settings into a file: 
    // conn_server_name is the same as the local registration name for local servers (synchronized on server start)
    cd_Park_server_settings(cdp);
   // record to the basic log:
     cd_SQL_execute(cdp, "CALL Log_write('The export of full database contents completed.')", NULL);
   // IF logged user was not DBAdmin, remove him from DB_Admin group (must do the same on the new database, users exported in the changed state)
    if (temp_db_admin)
    { uns32 rel=0; 
      cd_GetSet_group_role(cdp, my_user_objnum, db_admin_group, CATEG_GROUP, OPER_SET, &rel);
    }  
    *handle=cdf;
  }  
  return res;
}

bool check_rec_count(cdp_t cdp, const char * query, trecnum req_cnt)
{ tcursnum curs;  trecnum cnt;
  if (cd_Open_cursor_direct(cdp, query, &curs)) return false;
  cd_Rec_cnt(cdp, curs, &cnt);
  cd_Close_cursor(cdp, curs);
  return cnt==req_cnt;
}  

CFNC DllKernel int WINAPI RebuildDb_import(HANDLE handle)
// Starts the new server and imports everything. Then stops the server.
{ int res=0;  cd_t cd;  cdp_t cdp = &cd;
  CDuplFil * cdf = (CDuplFil *)handle;
 // start the server and connect:
  res=srv_Start_server_local(cdf->m_SrvName, 1, NULL);
  if (res != KSE_OK) return res;
  cdp_init(cdp);
  res=cd_connect(cdp, cdf->m_SrvName, 0);
  if (res == KSE_OK) 
  { if (cd_Login(cdp, "", "")) res=cd_Sz_error(cdp);
    else
    { if (cd_Lock_server(cdp)) res=cd_Sz_error(cdp);
      else
      { if (!check_rec_count(cdp, "select * from TABTAB",  8) ||
            //!check_rec_count(cdp, "select * from OBJTAB",  1) ||
            !check_rec_count(cdp, "select * from USERTAB", 5))
          res=DD_ERR_NEW_DB_NOT_EMPTY;
        else if (!cd_Am_I_config_admin(cdp)) res=DD_ERR_NOT_CONFIG_ADMIN;
        else if (!cd_Am_I_db_admin(cdp)) res=DD_ERR_NOT_DB_SECUR_ADMIN;
        else
        { cdf->m_cdp=cdp;
          cdf->ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Importing:"), cdf->msg_transl_buf));
         // record to the basic log:
           cd_SQL_execute(cdp, "CALL Log_write('Re-creating the database file and importing applications.')", NULL);
         // Delete default _sysext
          cd_Set_application(cdp, "_sysext");
          cd_SQL_execute(cdp, "DROP SCHEMA _sysext", NULL);
          cd_Set_application(cdp, "");
         // Logon as anonymous
          cd_Mask_duplicity_error(cdp, TRUE);   
         // Import replication servers & relations
          res=cdf->ImportRepl();
         // Import users and keys
          if (!res) res=cdf->ImportUsers();
         // Create temporary admin
          tobjname TmpUserN, TmpUserP; 
          tobjnum  TmpUserO = cdf->CreateTmpAdmin(TmpUserN, TmpUserP);
          // Logon as temporary admin
          cd_Login(cdp, TmpUserN, TmpUserP);
          // Import schemas
          cd_Enable_integrity(cdp, false);
          if (!res) cdf->ImportAppls();
          cd_Mask_duplicity_error(cdp, FALSE);   
          if (!res)
           // FOR each schema
            for (CObjDescs *NoExp = cdf->m_Appls;  NoExp;  NoExp=NoExp->next)
            { tobjnum ApplObj;
              cd_Find_object(cdp, NoExp->owning_schema, CATEG_APPL, &ApplObj);
              // Restore replication conflict parent and token admin servers
              if (!IsNullUUID(NoExp->m_FRCSrv) || !IsNullUUID(NoExp->m_TASrv))
              {   apx_header apx;
                  cd_Read_var(cdp, OBJ_TABLENUM, ApplObj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &apx, NULL);
                  memcpy(&apx.os_uuid, &NoExp->m_FRCSrv, sizeof(WBUUID));
                  memcpy(&apx.dp_uuid, &NoExp->m_TASrv, sizeof(WBUUID));
                  cd_Write_var(cdp, OBJ_TABLENUM, ApplObj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &apx);
              }  
            }
         // record to the basic log:
           cd_SQL_execute(cdp, "CALL Log_write('The import of the database contents completed.')", NULL);
         // Delete the temporary admin:
          cd_Delete(cdp, USER_TABLENUM, TmpUserO);
          cdf->ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Restoring of the database completed."), cdf->msg_transl_buf));
        } // the database is in the initial state
      } // server locked
    } // Login OK     
    srv_Stop_server_local(cdf->m_SrvName, 0, cdp);
  }
  else // not connected 
    srv_Stop_server_local(cdf->m_SrvName, 0, NULL);
  return res;
}

CFNC DllKernel int WINAPI RebuildDb_move(HANDLE handle, const char * destdir)
// Moves or erases the original database file. Multipart databases are not supported.
{ char oldpath[MAX_PATH], newpath[MAX_PATH];
  CDuplFil * cdf = (CDuplFil *)handle;
 // find the original database file:
  if (!GetDatabaseString(cdf->m_SrvName, "PATH",  oldpath, sizeof(oldpath), false) &&
      !GetDatabaseString(cdf->m_SrvName, "PATH1", oldpath, sizeof(oldpath), false) &&
      !GetDatabaseString(cdf->m_SrvName, "PATH",  oldpath, sizeof(oldpath), true ) &&
      !GetDatabaseString(cdf->m_SrvName, "PATH1", oldpath, sizeof(oldpath), true ))
    return FALSE;  // server is not local
  if (!*oldpath)
    return FALSE;  // server is not local
  append(oldpath, FIL_NAME);
 // move or delete: 
  int res;  wchar_t msg_transl_buf[200];
  if (destdir && *destdir)
  { strcpy(newpath, destdir);
    make_directory_path(newpath);
    append(newpath, FIL_NAME);
    cdf->ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Moving the original database file..."), msg_transl_buf));
    res = MoveFileEx(oldpath, newpath, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING) ? 0 : DD_ERR_CANNOT_MOVE_DELETE;
  }
  else
  { cdf->ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("Deleting the original database file..."), msg_transl_buf));
    res = DeleteFile(oldpath) ? 0 : DD_ERR_CANNOT_MOVE_DELETE;
  }  
  cdf->ReportProgress(IMPEXP_MSG_UNICODE, (const char *)Get_transl(NULL, gettext_noopL("...done."), msg_transl_buf));
  return res;  
}

CFNC DllKernel void WINAPI RebuildDb_close(HANDLE handle, BOOL erase_export)
// If [erase_export] erases all exported files. Then deletes the handle.
{ if (!handle) return;
  CDuplFil * cdf = (CDuplFil *)handle;
  if (erase_export)
    cdf->DeleteApplFiles();
  delete cdf;
}

#endif  // DUPLDB

