/////////////////////////////////////////////////////////////////////////////
// Name:        DuplFilDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/09/05 14:25:18
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DUPLFIL_H_
#define _DUPLFIL_H_

#include "wx/variant.h"
#include "api.h"
#include "DuplFilProg.h"

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

    WBUUID    m_FRCSrv;     // Parent server for replication conflicts UUID
    WBUUID    m_TASrv;      // Token administrator server UUID
    bool      m_ExpOK;      // Schema export OK
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

    CObjDescs(){m_Count = 0; m_Arr = NULL; m_ExpOK = false;}
    ~CObjDescs()
    {
        if (m_Arr)
            corefree(m_Arr);
    }
};
//
// Server properties
//
// Copies Delphi component TSQL602ServerProps model
// m_Section determines server property area DuplFil uses "@SQLSERVER", for security properties and "Database"
// for server registration properties. m_SubSection determines property owner, for "@SQLSERVER" has no meaning
// for "Database" specifies registred server name
//
class CServerProps : public C602SQLApi
{
protected:

    wxString m_Section;     // Current server properties section
    wxString m_SubSection;  // Current server properties subsection
#ifdef WINS
    HKEY     m_hKey;        // Server properties key
#endif

    void Init(wxString Section, wxString SubSection = wxEmptyString)
    {
        m_Section    = Section;
        m_SubSection = SubSection;
#ifdef WINS
        m_hKey       = NULL;
#endif
    }
    void OpenDBKey(const wxString &Server);

public:

    CServerProps(){}
    CServerProps(cdp_t cdp, wxString Section) : C602SQLApi(cdp)
    {
        Init(Section);
    }
    CServerProps(wxString Section, wxString SubSection = wxEmptyString) : C602SQLApi(NULL)
    {
        Init(Section, SubSection);
    }
    ~CServerProps()
    {
#ifdef WINS
        if (m_hKey != NULL)
            RegCloseKey(m_hKey);
#endif
    }

    void SetSubSection(const wxString &SubSection)
    {
#ifdef WINS
        if (m_hKey != NULL)
        {
            RegCloseKey(m_hKey);
            m_hKey = NULL;
        }
#endif
        m_SubSection = SubSection;
    }

    wxVariant GetProp(const wxString &PropName);
    wxString  GetPropStr(const wxString &PropName);
    bool      GetPropBool(const wxString &PropName);
    int       GetPropInt(const wxString &PropName);
    void      SetProp(const wxString &PropName, const wxVariant &Value);
    void      SetProp(const wxString &PropName, const wxString &Value);
    void      SetProp(const wxString &PropName, bool Value);
    void      SetProp(const wxString &PropName, int Value);
    void      CreateSubSection(const wxString &SubSect);

    wxArrayString *GetPropNames();
    wxArrayString *GetSubSectNames();
};
//
// wxHashMap for schema descriptors
//
WX_DECLARE_STRING_HASH_MAP(CObjDescs *, CApplList);

/*!
 * CDuplFil class declaration
 */

class CDuplFil: public wxDialog, public C602SQLApi
{    
protected:

    CApplList       m_Appls;        // wxHashMap for schema descriptors
    CDuplFilProgDlg m_ProgDlg;      // Progress dialog instance
    wxWindow       *m_Parent;       // Parent window
    wxString        m_SrvName;      // Original server name
    wxString        m_BackName;     // Backup server name
    wxString        m_ApplsPath;    // Folder for exported schemas
    wxString        m_BackPath;     // Folder for fil backup
    wxString        m_UserName;     // Admin name
    wxString        m_Password;     // Admin password
    wxString        m_NoAdmin;      // Name of user that is config admin but not dbadmin
    wxString        m_Msg;          // Message that explains long time operation
    wxString        m_BreakMsg;     // Message that explains long time operation terminating
    wxString        m_Domain;       // ThrustedDomain property value
    WBUUID          m_UUID;         // Original server UUID
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
    bool            m_WasApplsPath; // Folder for exported schemas did not exist
    bool            m_Connected;    // Server is connected
    bool            m_ImportOK;     // Fil duplication OK
    bool            m_WasActive;    // Server was active
    bool            m_Locked;       // Server is locked
#ifdef WINS
    HANDLE          m_SrvHandle;    // Server process handle
    HWND            m_SrvWnd;       // Server process window
#endif

    bool DetectLocalServer(const wxString &SrvName){return srv_Get_state_local(SrvName.mb_str(*wxConvCurrent)) == 1;}
    bool StopService();
    bool EnsureDBAdmin();

    void BackupExist();
    void StartServer();
    void StopServer(bool Find);
    void Login();
    void EnsureExclusive();
    void FindServer();
    void SaveSecur();
    void LoadSecur();
    void GetExportable(WBUUID UUID, CObjDescs *od);
    void ExportAppls();
    void ExportUsers();
    void ExportRepl();
    void BackupServer();
    void ImportUsers();
    void ImportRepl();
    void ImportAppls();
    void UpdateGcrLimit(const wxString &Appl);
    void RemoveDBAdmin();
    void DeleteApplFiles();
    void TimeoutInit(wxString Msg, wxString BreakMsg);
    void TimeoutWait();
    void SetExportable(WBUUID UUID, CObjDescs *ObjDescs);
    void ResetAppls();
    void ForceDirectories(wxString Dir);

    tobjnum CreateTmpAdmin(char *TmpUserN, char *TmpUserP);
    wxWindow *GetParent(){return m_ProgDlg.IsShown() ? &m_ProgDlg : m_Parent;}

    void Connect(const wxString &UserName, const wxString &Password)
    {
        connect(ToChar(m_SrvName, wxConvCurrent)); 
        make_sys_conv(m_cdp);
        C602SQLApi::Login(ToChar(UserName, m_cdp->sys_conv), ToChar(Password, wxConvCurrent));
        m_Connected = true;
    }
    void Disconnect()
    {
        if (m_Connected)
        {
            cd_disconnect(m_cdp);
            m_Connected = false;
        }
    }
    void Disable_integrity(){cd_Enable_integrity(m_cdp, false);}
    void Reset_replication(){cd_Reset_replication(m_cdp);}

public:
    /// Constructors
    CDuplFil(wxWindow *Parent, const wxString &SrvName, const wxString &BackName, const wxString &ApplsPath, const wxString &BackPath) : C602SQLApi(cdp_alloc()), m_ProgDlg(m_cdp, Parent)
    {
        m_Parent    = Parent;
        m_SrvName   = SrvName;
        m_BackName  = BackName;
        m_ApplsPath = ApplsPath;
        m_BackPath  = BackPath;
        m_Connected = false;
        m_Locked    = false;
#ifdef WINS
        m_SrvHandle = NULL;
#endif
    }
    virtual ~CDuplFil();
    void Perform();

    friend BOOL CALLBACK EnumThreadWindowsProc(HWND Handle, LPARAM lParam);
};

#endif
    // _DUPLFIL_H_
