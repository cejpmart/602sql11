#define MAX_MAIL_PASS 40
// V.B.
#define MAX_PROFILE   64
#define MAX_MAPIPWORD 64
#define MAX_ADDRTYPE  12

typedef struct
{ char  input_dir[MAX_PATH];
  char  output_dir[MAX_PATH];
  uns32 input_threads;
  uns32 reserved;
  uns32 scanning_period;
  char  emi_path[MAX_PATH];
  char  mail_id[8+1];
  char  password[MAX_MAIL_PASS+1];
  // V.B.
  char  profile[MAX_PROFILE+1];
  char  mailpword[MAX_MAPIPWORD+1];
  char  dialpword[MAX_MAPIPWORD+1];
  char  dipdconn[MAX_PROFILE+1];
  char  dipduser[MAX_MAPIPWORD+1];
  char  dipdpword[MAX_MAPIPWORD+1];
  wbbool log_tables;
  wbbool repl_triggers;
  wbbool err_causes_nack;
} t_my_server_info;

typedef struct
{ WBUUID server_id;
  WBUUID appl_id;
  wbbool token_req;
  char spectab[OBJ_NAME_LEN+1];
  uns16 len16;
} t_repl_param;

//////////////////////////// replication relation //////////////////////////
#define MAX_REPLCOND 255

typedef char t_attrname[ATTRNAMELEN+1];

SPECIF_DYNAR(attrname_dynar, t_attrname, 3);

typedef struct t_reltab
{ char tabname[OBJ_NAME_LEN+1];
  char condition[MAX_REPLCOND+1];
  BOOL can_del;
  attrname_dynar atrlist;
} t_reltab;

SPECIF_DYNAR(reltab_dynar, t_reltab, 1);

typedef struct t_relatio
{ int hours, minutes;
  reltab_dynar tabs;
  t_reltab * get_reltab(const char * tabname)
  { t_reltab * rt;
    for (int i=0;  i<tabs.count();  i++)
    { rt=tabs.acc(i);
      if (!sys_stricmp(rt->tabname, tabname)) return rt;
    }
    rt=tabs.next();
    strmaxcpy(rt->tabname, tabname, OBJ_NAME_LEN+1);
    rt->atrlist.init();
    return rt;
  }
} t_relatio;

//t_reltab * t_relatio::get_reltab(const char * tabname)

struct t_replrel
{ cdp_t cdp;
  BOOL mask_changes;
  tobjnum * prelobj;
  BOOL changed;
  int  konflres;
  t_relatio r[2];
  t_replrel(cdp_t cdpIn, tobjnum * prelobjIn)
  { cdp=cdpIn;  prelobj=prelobjIn;
    mask_changes=changed=FALSE;
  }
  ~t_replrel(void)
    { for (int direc=0;  direc<2;  direc++)
      { for (int i=0;  i<r[direc].tabs.count();  i++)
        { t_reltab * rt=r[direc].tabs.acc(i);
          rt->atrlist.attrname_dynar::~attrname_dynar();
        }
        r[direc].tabs.reltab_dynar::~reltab_dynar();
      }
    }
};

/////////////////////////// editing rules, replicating with ////////////////
typedef struct
{ char tabname[OBJ_NAME_LEN];
  trecnum recnum;  // recnum in REPLTAB, NORECNUM if not included
} t_tabinfo;

SPECIF_DYNAR(tables_dynar, t_tabinfo, 5);

typedef class t_replrul
{
 public:
  cdp_t cdp;
  BOOL changed;
  BOOL notify_lock;
  BOOL is_server_selected;
  WBUUID appl_id, sel_server, my_server_id;
  tobjnum aplobj;
  tables_dynar tabs;
  int sel_table_ind;      // index to dynar and to the list box
  int atrnum_base;
  int attrcnt;
  char sel_atrmap[256/8];
 // application options:
  int hours, minutes, konflres;
  uns32 next_dtm;
  //BOOL repl_def;          // na nic se nepouziva
  t_replapl replapl;
  trecnum apl_trec;
  int aux_direc;
  BOOL skipping;
  t_replrel *new_rel;
  BOOL       new_rel_changed;

  t_replrul(cdp_t init_cdp, WBUUID init_appl_id, tobjnum init_aplobj)
  { cdp=init_cdp;  aplobj=init_aplobj;  skipping=FALSE;
    memcpy(appl_id, init_appl_id, UUID_SIZE);
    new_rel = NULL;
    new_rel_changed = FALSE;
  }
  ~t_replrul(){if (new_rel) delete new_rel;}
} t_replrul;

DllPrezen int WINAPI fill_server_names(t_replrul * rr, BOOL is_combo,
  BOOL direc, HWND hWnd1, HWND hWnd2, HWND hWnd3);

class CReqList
{
protected:
    UINT   m_Cnt;
    UINT   m_Max;
    DWORD *m_Elems;

public:

    CReqList(){m_Cnt = 0; m_Max = 0; m_Elems = NULL;}
    ~CReqList(){if (m_Elems) delete []m_Elems;}
    void Add(DWORD Req);
    void Rem(DWORD Req);
    UINT IsIn(DWORD Req);
};

DWORD cd_InitWBMail(cdp_t cdp, const char *Profile, const char *PassWord);
DWORD cd_InitWBMailEx(cdp_t cdp, const char *Profile, const char *RecvPassWord, const char *SendPassWord);
DWORD cd_InitWBMail602(cdp_t cdp, char *EmiPath, char *UserID, char *PassWord);
DWORD cd_InitWBMail602x(cdp_t cdp, char *Profile);
void  cd_CloseWBMail(cdp_t cdp);

DWORD cd_LetterCreate(cdp_t cdp, char *Subj, char *Msg, UINT Flags, HANDLE *lpLetter);
DWORD cd_LetterCreateW(cdp_t cdp, const wuchar *Subj, const wuchar *Msg, UINT Flags, HANDLE *lpLetter);
DWORD cd_LetterAddAddr(cdp_t cdp, HANDLE Letter, char *Addr, char *Type, BOOL CC);
DWORD cd_LetterAddAddrW(cdp_t cdp, HANDLE Letter, const wuchar *Addr, const wuchar *Type, BOOL CC);
DWORD cd_LetterAddFile(cdp_t cdp, HANDLE Letter, char *fName);
DWORD cd_LetterAddFileW(cdp_t cdp, HANDLE Letter, const wuchar *fName);
DWORD cd_LetterAddBLOBs(cdp_t cdp, HANDLE Letter, char *fName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond);
DWORD cd_LetterAddBLOBsW(cdp_t cdp, HANDLE Letter, const wuchar *fName, const wuchar *Table, const wuchar *Attr, const wuchar *Cond);
DWORD cd_LetterAddBLOBr(cdp_t cdp, HANDLE Letter, char *fName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
DWORD cd_LetterAddBLOBrW(cdp_t cdp, HANDLE Letter, const wuchar *fName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
DWORD cd_LetterSend(cdp_t cdp, HANDLE Letter);
DWORD cd_TakeMailToRemOffice(cdp_t cdp);
void  cd_LetterCancel(cdp_t cdp, HANDLE Letter);

DWORD cd_MailCreInBoxTables(cdp_t cdp, char *Profile);
DWORD cd_MailOpenInBox(cdp_t cdp, HANDLE *lpMailBox);
DWORD cd_MailBoxLoad(cdp_t cdp, HANDLE MailBox, UINT Flags);
DWORD cd_MailBoxGetMsg(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD Flags);
DWORD cd_MailBoxGetFilInfo(cdp_t cdp, HANDLE MailBox, DWORD MsgID);
DWORD cd_MailBoxSaveFileAs(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, char *DstPath);
DWORD cd_MailBoxSaveFileDBs(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond);
DWORD cd_MailBoxSaveFileDBr(cdp_t cdp, HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, tcurstab Table, trecnum Pos, tattrib Attr, uns16 Index);
DWORD cd_MailBoxDeleteMsg(cdp_t cdp, HANDLE MailBox, DWORD MsgID, BOOL RecToo);
DWORD cd_MailGetInBoxInfo(cdp_t cdp, HANDLE MailBox, char *mTblName, ttablenum *mTblNum, char *fTblName, ttablenum *fTblNum);
void  cd_MailCloseInBox(cdp_t cdp, HANDLE MailBox);
DWORD cd_MailGetType(cdp_t cdp);
DWORD cd_MailDial(cdp_t cdp, char *PassWord);
DWORD cd_MailHangUp(cdp_t cdp);

DWORD cd_MailCreateProfile(cdp_t cdp, const char *ProfileName, BOOL Temp);
DWORD cd_MailDeleteProfile(cdp_t cdp, const char *ProfileName);
DWORD cd_MailSetProfileProp(cdp_t cdp, const char *ProfileName, const char *PropName, const char *PropValue);
DWORD cd_MailGetProfileProp(cdp_t cdp, const char *ProfileName, const char *PropName, char *PropValue, int ValSize);
