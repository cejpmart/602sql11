#ifndef _OBJLIST_H_
#define _OBJLIST_H_

#ifdef WINS
#define MIDL_PASS  // usefull for express VC with old SDK
#undef BOOKMARK  // workaround on 64-bit platforms
#define BOOKMARK MAPI_BOOKMARK
#include "mapix.h"
#undef BOOKMARK
#undef   FOLDER_ROOT
#endif
#include "exceptions.h"
#include "buffer.h"

struct t_avail_server  // used for 602SQL servers and ODBC DSNs
{ char locid_server_name[MAX_OBJECT_NAME_LEN+1];  // local registration name or "IP database" when not registered
  int  server_image;
  bool odbc;
  bool local;
  bool notreg;
  bool committed;  // used only when refreshing the list
  bool private_server;
  cdp_t cdp;
  t_avail_server * next;
 // data for not-registered servers (notreg==true), for remote servers defined when IP is specified in the form of dotted IP address:
  uns32 ip;
  uns16 port;
 // ODBC information:
  char * description;
  t_connection * conn;
  t_avail_server(void)
  { description=NULL;  cdp=NULL;  conn=NULL;
  }
  ~t_avail_server(void)
  { if (description) delete [] description; 
  }
};
//
// Special folder ID
//
#define FOLDER_ROOT   (t_folder_index)0     // Root folder: foler name is empty string or "."
#define FOLDER_ANY    (t_folder_index)-1    // Any folder
#define FOLDER_PAR    (t_folder_index)-2    // .. folder
#define FOLDER_SYSTEM (t_folder_index)-3    // System folder
//
// Object copy flags
//
#define CO_ALLINFOLDER 1                    // Category or folder below category is selected - copy all objects of given category in the folder
#define CO_FOLDERTOO   2                    // Folder below category is selected - copy current folder too

#define NONECATEG     (tcateg)-1

inline bool IsRootFolder(const char *Folder){return(!*Folder);}
//
// Object category, contains tcateg and copy options 
//
class CCateg
{
public:

    tcateg m_Categ;     // Category ID
    uns8   m_Opts;      // Category options, CO_ALLINFOLDER, CO_FOLDERTOO

    CCateg(){m_Categ = NONECATEG; m_Opts = 0;}
    CCateg(tcateg Categ){m_Categ = Categ; m_Opts = 0;}
    CCateg(tcateg Categ, uns8 Opts){m_Categ = Categ; m_Opts = Opts;}

    bool IsFolder(){return(m_Opts & CO_ALLINFOLDER);}
    operator tcateg() const{return(m_Categ);} 
};
//
// Object descriptor - provides information of an object from the object list
//
struct CObjDescr
{
    const char    *m_ObjName;           // Object name
    const char    *m_FolderName;        // Object folder name, always defined
    t_folder_index m_FolderIndex;       // Object folder index, FOLDER_ANY means folder index is not known (it may not be defined)
    tobjnum        m_ObjNum;            // Object number
    unsigned       m_Flags;             // Object flags
    uns32          m_Modif;             // Object modif timestamp
    unsigned       m_Index;             // Object index
    void          *m_Source;            // engine-dependent object information

    void Reset()
    {
        m_ObjName     = "";
        m_ObjNum      = NOOBJECT;
        m_FolderIndex = FOLDER_ROOT;
        m_Flags       = 0;
        m_Modif       = 0;
        m_Index       = 0;
    }
    bool IsRootFolder(){return m_FolderIndex == FOLDER_ROOT;}
};

class CObjectList;
//
// Common object list search engine ancestor
//
class COLEng
{
  protected:
    tcateg m_MyCateg;   // the category the m_eng is created for
  public:
    COLEng(tcateg categ) : m_MyCateg(categ) { }
    virtual ~COLEng() { }
   // functions finding object by some information in [od] and storing the full description to [od]: 
    virtual bool Find(CObjDescr *od) = 0;
    virtual bool FindByObjnum(CObjDescr *od){return false;}
    virtual bool FindByIndex(CObjDescr *od){return false;}
   // passing all objects: 
    virtual void Reset() = 0;  // restarts passing of the loaded list of objects
    virtual bool Next(CObjDescr *od) = 0;

    virtual t_folder_index GetFolderIndex(const char *Name){return FOLDER_ROOT;}
   // information about the state of the engine: 
    tcateg GetMyCateg(void) const { return m_MyCateg; }
    virtual unsigned GetCount(void) = 0;
};
//
// Object list search engine for cached objects
//
class COLCache :  public COLEng       
{
protected:

    cdp_t          m_cdp;           // cdp
    comp_dynar    *m_Comp;          // Object cache
    comp_dynar    *m_FolderCache;   // Folder cache
    int            m_Index;         // Index of selected object

    void Fill(CObjDescr *od, t_comp*comp);
    const char *GetFolderName(t_folder_index Index);

public:

    COLCache(cdp_t cdp, comp_dynar *Comp, comp_dynar *Folder, tcateg categ) : COLEng(categ)
      {m_cdp = cdp; m_Comp = Comp; m_FolderCache = Folder; m_Index = 0;}

    virtual bool Next(CObjDescr *od);
    virtual bool Find(CObjDescr *od);
    virtual bool FindByObjnum(CObjDescr *od);
    virtual bool FindByIndex(CObjDescr *od);
    virtual unsigned GetCount(void);
    virtual void Reset(){m_Index = 0;}
};
//
// Object list search engine for not cached objects
//
class COLDB : public COLEng
{
protected:

    const char *m_Src;          // Current object pointer
    const char *m_Info;         // Object list source
    bool        m_Extended;     // Extended category flag
    comp_dynar *m_FolderCache;  // Folder cache
    cdp_t       m_cdp;
public:

    COLDB(cdp_t cdp, char *Info, comp_dynar *Folder, bool Extended, tcateg categ) : COLEng(categ)
    {   m_cdp      = cdp;
        m_Src      = Info; 
        m_Info     = Info; 
        m_FolderCache = Folder;
        m_Extended    = Extended;
    }

    virtual ~COLDB(){if (m_Info) corefree(m_Info);}
    virtual bool Next(CObjDescr *od);
    virtual bool Find(CObjDescr *od);
    virtual bool FindByObjnum(CObjDescr *od);
    virtual unsigned GetCount(void);
    virtual t_folder_index GetFolderIndex(const char *Name);
    virtual void Reset(){m_Src = m_Info;}
};
//
// Object list search engine for ODBC data source objects
//
SPECIF_DYNAR(t_char_dynar, char, 200);

class COLODBC : public COLEng
{
protected:

    t_char_dynar names;
    int curr_pos;

public:

    COLODBC(tcateg categ) : COLEng(categ) { curr_pos=0; }
    virtual ~COLODBC() { }
    void fill(t_connection * conn, int column, char * catalog, char * schema, tcateg categ);
    virtual bool Next(CObjDescr *od);
    virtual bool Find(CObjDescr *od);
    virtual unsigned GetCount(void);
    virtual void Reset(void) { curr_pos = 0; }
};

typedef t_avail_server * (WINAPI *LPGETSERVERLIST)();
//
// Encapsulates list of all kinds of database objects
// Enables to traverse object list of given category in given folder
// Intrinsic search provides a search engine
// The description of the current object (when passing a list) or the found object (when searching) is stored into the CObjDescr structure 
// 
class DllKernel CObjectList : public CObjDescr
{
protected:

    cdp_t           m_cdp;                  // cdp
    t_connection   *m_conn;                 // ODBC connection
    const char     *m_Schema;               // Selected schema schema
    tobjname        m_SelFolderName;        // Selected folder
    t_folder_index  m_SelFolder;            // Index of selected folder, FOLDER_ANY means folder index is not known
    COLEng         *m_eng;                  // Search engine
    bool            m_MySchema;             // true if selected schema is current schema
    bool            m_AnyFolder;            // true if search through all folders

    bool Init(CCateg Categ);
    void ChangeFolder(const char *Folder)
    {
        m_AnyFolder = Folder == NULL;
        *m_SelFolderName   = 0;
        if (Folder && *Folder && *(WORD *)Folder != '.')
        {
            strcpy(m_SelFolderName, Folder);
            Upcase9(m_cdp, m_SelFolderName);
        }
    }
    bool AnyFolder(){return(m_AnyFolder);}
    bool IsRootFolder(const char *Folder){return(!*Folder);}
    unsigned GetCategCount(CCateg Categ, const char *Folder);
    bool IsFromSelFolder();

public:

    inline CObjectList(cdp_t cdp = NULL, const char *Schema = NULL, t_connection *Conn = NULL)
    {
        m_cdp      = cdp;
        m_conn     = Conn;
        m_eng      = NULL;
        m_Schema   = Schema;
        m_MySchema = !Schema || !*Schema || (cdp && wb_stricmp(cdp, Schema, cdp->sel_appl_name) == 0);
       *m_SelFolderName = 0;
    }
    CObjectList(const t_avail_server * as, const char *Schema = NULL);

    virtual ~CObjectList()
    {
        Release();
    }

    void Release(void)          // Deletes the current search engine
    {
        if (m_eng)
        { delete m_eng;
          m_eng = NULL;
        }
    }
    static LPGETSERVERLIST GetServerList;   // Pointer to custom GetServerList function
    static void SetServerList(LPGETSERVERLIST ServerList){GetServerList = ServerList;}

    void SetCdp(cdp_t cdp, const char *Schema = NULL, t_connection *Conn = NULL)
    {
        m_cdp      = cdp;
        m_conn     = Conn; 
        m_Schema   = Schema;
        m_MySchema = !Schema || !cdp || wb_stricmp(m_cdp, Schema, m_cdp->sel_appl_name) == 0;
        Release();
    }
    void SetCdp(const t_avail_server * as, const char *Schema = NULL);
    cdp_t GetCdp() const {return(m_cdp);}
    void SetConn(t_connection * conn)
      { m_conn=conn; }
   // passing all objects for given xcdp/schema/Categ(/Folder):
    bool FindFirst(CCateg Categ, const char *Folder = NULL);
    bool FindNext();
   // finding object in the current xcdp/schema, result stored to CObjDescr: 
    bool Find(const char *Name, CCateg Categ, const char *Folder = NULL);
    //bool Find(const wxChar *Name, CCateg Categ, const wxChar *Folder = NULL);
    bool FindByObjnum(tobjnum Objnum, CCateg Categ);
    bool FindByIndex(int Index, CCateg Categ = CATEG_FOLDER);
    bool IsSubFolder(const char *SubFolder, const char *Folder);
   // functions returning information about the selected object:
    const char *GetName() const {return(m_ObjName);}
    const char *GetFolderName() const {return m_FolderName;}
    t_folder_index GetFolderIndex();
    uns16 GetFlags() const {return(m_Flags);}
    uns32 GetModif() const {return(m_Modif);}
    tobjnum GetObjnum() const {return(m_ObjNum);}
    tobjnum GetLinkOrObjnum() const;                   
    t_folder_index GetIndex() const {return(m_Index);} // funguje jen pro polozky v cachi
    void *GetSource(void) const { return m_Source; }

    unsigned GetCount(CCateg Categ, const char *Folder = ""); 
};
//
// Object list search engine for servers and ODBC data sources
//
class COLServ : public COLEng
{
    t_avail_server *m_Server;           // pointer to the next server (not returned yet), used when passing
    bool            m_ODBC;             // true for ODBC data sources
    t_avail_server *GetServerList()
    {
        if (CObjectList::GetServerList)
            return CObjectList::GetServerList();
        return NULL;
    }

  public:
    COLServ(tcateg categ) : COLEng(categ)
    { 
      m_ODBC = categ==XCATEG_ODBCDS;
      Reset();
    }

    virtual bool Find(CObjDescr *od);
    virtual bool FindByObjnum(CObjDescr *od);
    virtual unsigned GetCount(void);
    virtual void Reset(void)
      { m_Server = GetServerList(); }
    virtual bool Next(CObjDescr *od);
};
//
// Popisy kategorii
//
#define FF_HIDEN    0x01   // Neni videt v seznamu kategorii v aplikaci
#define FF_FOLDERS  0x02   // Muze mit slozky
#define FF_EXPORT   0x04   // Exportuje se
#define FF_ENCRYPT  0x08   // Sifruje se
#define FF_BACKEND  0x10   // Backendovy objekt 
#define FF_TEXTDEF  0x20   // Objekt ma textovou definici

struct tcategdescr
{
    unsigned       Flags;
    const char    *Original;
    const wchar_t *CaptFirst;
    const wchar_t *Lower;
    const wchar_t *Plural;
    const char    *ExportName;
    const char    *FileType;
    const wchar_t *FileTypeFilter;
    char         **DragImages;
    unsigned       NextInPanel;
};
//
// Folder content flags
//
#define FST_EMPTY      0    // Folder is empty
#define FST_HASITEMS   1    // Folder contains objects
#define FST_HASFOLDERS 2    // Folder contains subfolders
#define FST_UNKNOWN    4    // Folder content is unknown
//
// Encapsulates list all object categories
//
class DllKernel CCategList
{
protected:

    tcategdescr *m_Descr;       // Current category description

public:

    CCategList();

    int  Count;
    void FindFirst();
    bool FindNext();            // Vraci dalsi kategorii, ktera se zobrazuje v ridicim panelu

    tcateg         GetCateg()       const;
    unsigned       GetFlags()       const {return(m_Descr->Flags);}
    bool           HasFolders()     const {return((m_Descr->Flags & FF_FOLDERS) != 0);}
    bool           HasTextDefin()   const {return((m_Descr->Flags & FF_TEXTDEF) != 0);}
    bool           Exportable()     const {return((m_Descr->Flags & FF_EXPORT) != 0);}
    const wchar_t *CaptFirst()      const {return(m_Descr->CaptFirst);}
    const wchar_t *Lower()          const {return(m_Descr->Lower);}
    const char    *FileType()       const {return(m_Descr->FileType);}
    const wchar_t *FileTypeFilter() const {return(m_Descr->FileTypeFilter);}

    static unsigned       GetFlags(CCateg Categ);
    static bool           HasFolders(CCateg Categ);
    static bool           HasTextDefin(CCateg Categ);
    static bool           Exportable(CCateg Categ);
    static bool           Encryptable(CCateg Categ);
    static bool           IsBackendCateg(CCateg Categ);
    static const char    *Original(CCateg Categ);
    static const wchar_t *CaptFirst(CCateg Categ);
    static const wchar_t *CaptFirst(cdp_t cdp, CCateg Categ, wchar_t *Val);
    static const wchar_t *Lower(CCateg Categ);
    static const wchar_t *Lower(cdp_t cdp, CCateg Categ, wchar_t *Val);
    static const wchar_t *Plural(CCateg Categ);
    static const wchar_t *Plural(cdp_t cdp, CCateg Categ, wchar_t *Val, bool CaptFirst = false);
    static const char    *ShortName(CCateg Categ);
    static const char    *FileType(CCateg Categ);
    static const wchar_t *FileTypeFilter(CCateg Categ);
    static const char   **MultiSelImage(CCateg Categ);
};

void refresh_server_list(void);
//
// Mail profiles list
//
class DllKernel CMailProfList
{
protected:

    wchar_t m_Name[65];         // Profile name
    wchar_t m_Value[256];       // Buffer for property value
    int     m_Index;            // Current profile index

#ifdef WINS

    HKEY      m_hRootKey;       // Profile list root key
    HKEY      m_hKey;           // Profile key
    CWBuffer  m_Profs602;       // List of Mail602 profiles
    wchar_t  *m_Prof602;        // Current Mail602 profile
    LPSRowSet m_RowSet;         // List of MAPI profiles
    int       m_IndexMAPI;      // Index of current MAPI profile

    bool GetRootKey();
    void CloseKey()
    {
        if (m_hKey)
        {
            RegCloseKey(m_hKey);
            m_hKey = NULL;
        }
    }

#else

    const char *m_ProfList;     // Profile list
    const char *m_ProfPtr;      // Current profile
    char        m_Profile[65];  // Profile name

    bool GetProfList();

#endif

public:

    CMailProfList()
    {
#ifdef WINS

        m_hRootKey   = NULL;
        m_hKey       = NULL;
        m_RowSet     = NULL;

#else
        
        m_ProfList   = NULL;
        m_ProfPtr    = NULL;  
        m_Profile[0] = 0;     

#endif
    }
    ~CMailProfList();
    bool FindFirst();
    bool FindNext();
    bool Open(const wchar_t *ProfileName, bool WriteAccess);
    
    const wchar_t *GetStrValue(const wchar_t *ValName);
    void  SetStrValue(const wchar_t *ValName, const wchar_t *Val);

    int   GetIndex(){return(m_Index);} 
    void  DeleteAllValues();
    bool  Delete(const wchar_t *ProfileName);
    const wchar_t *GetName(){return(m_Name);}

#ifdef WINS
    
    bool FindFirst602();
    bool FindNext602();
    const wchar_t *GetName602(){return(m_Prof602);}
    bool FindFirstMAPI();
    bool FindNextMAPI()
    {
        m_IndexMAPI++;
        return(m_IndexMAPI < m_RowSet->cRows);
    }
    const wchar_t *GetNameMAPI()
    {
        LPSRow lpRow  = m_RowSet->aRow + m_IndexMAPI;
        return lpRow->lpProps->Value.lpszW;
    }
    void Close(){CloseKey();}
    int      GetBinValue(const wchar_t *ValName, char *Buf, unsigned long Size);
    void     SetBinValue(const wchar_t *ValName, const char *Val, unsigned Size);

#else        
    
    void Close(){*m_Profile = 0;}
    bool Exists(const wchar_t *ValName);
    
#endif  // WINS

};

#define IMPEXP_MSG_UNICODE  -200

#endif // _OBJLIST_H_


