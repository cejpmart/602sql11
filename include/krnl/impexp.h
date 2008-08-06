#ifndef _IMPEXP_H_
#define _IMPEXP_H_

#include "wbfile.h"
#include "objlist9.h"
#include "query.h"
#include "api.h"
//
// Schema import flags
//
#define IMPAPPL_NEW_INST 1
#define IMPAPPL_USE_ALT_NAME 2
#define IMPAPPL_REPLACE 4
#define IMPAPPL_NO_COMPILE 8
#define IMPAPPL_WITH_INFO 0x10
//
// Schema Import/Export progress callback states
//
#define IMPEXP_DATA -1
#define IMPEXP_DATAPRIV -2
#define IMPEXP_ICON -3
#define IMPEXP_PROGMAX -100
#define IMPEXP_PROGSTEP -101
#define IMPEXP_PROGPOS -102
#define IMPEXP_FILENAME -103
#define IMPEXP_STATUS -104
#define IMPEXP_ERROR -105
//
// Data Import/Export formats
//
#define IMPEXP_FORMAT_WINBASE 0
#define IMPEXP_FORMAT_TEXT_COLUMNS 1
#define IMPEXP_FORMAT_TEXT_CSV 2
#define IMPEXP_FORMAT_DBASE 3
#define IMPEXP_FORMAT_FOXPRO 4
#define IMPEXP_FORMAT_ODBC 5
#define IMPEXP_FORMAT_CURSOR 6
#define IMPEXP_FORMAT_TABLE 10
#define IMPEXP_FORMAT_TABLE_REIND 11
//
// Schema Import/Export progress callback declaration
//
typedef void (WINAPI IMPEXPAPPLCALLBACK)( int Categ, const void * Value, void * Param );
typedef IMPEXPAPPLCALLBACK *LPIMPEXPAPPLCALLBACK;

#if 0
/* parametr Export_appl_param */
struct t_export_param {
    uns32 cbSize;
    HWND hParent;
    int with_data;
    int with_role_privils;
    int with_usergrp;
    int exp_locked;
    int back_end;
    uns32 date_limit;
    const char * file_name;
    int long_names;
    int report_progress;
    LPIMPEXPAPPLCALLBACK callback;
    void * param;
    const char * schema_name;
    int overwrite;

    t_export_param(void)
    { cbSize=sizeof(t_export_param);
        hParent=0;  with_data=FALSE;  with_role_privils=TRUE;  with_usergrp=FALSE;
        exp_locked=FALSE;  back_end=FALSE;
        file_name=NULL;  date_limit=0;  long_names=TRUE;  report_progress=FALSE;
        callback=NULL;
        schema_name=NULL;
        overwrite=false;
    }
};

/* parametr Import_appl_param */
struct t_import_param {
    uns32 cbSize;
    HWND hParent;
    unsigned flags;
    const char * file_name;
    const char * alternate_name;
    int report_progress;
    LPIMPEXPAPPLCALLBACK callback;
    void * param;

    t_import_param(void)
    { cbSize=sizeof(t_import_param);
        memset(this, 0, sizeof(t_import_param));
    }
};
#endif
//
// Control panel refresh interface
//
class CCPInterface
{
public:

    // Adds object to control panel
    virtual void AddObject(const char *ServerName, const char *SchemaName, const char *FolderName, const char *ObjName, tcateg Categ, uns16 Flags, uns32 Modif, bool SelectIt) = 0;
    // Removes object to control panel
    virtual void DeleteObject(const char *ServerName, const char *SchemaName, const char *ObjName, tcateg Categ) = 0;
    // Refresches schema content on control panel
    virtual void RefreshSchema(const char *ServerName, const char *SchemaName) = 0;
    // Refresches server content on control panel
    virtual void RefreshServer(const char *ServerName) = 0;
    // Refresches whole content of control panel
    virtual void RefreshPanel() = 0;
};
//
// Import/Export query answers
//
enum tiocresult 
{
    iocYes,         // Ano, pokracovat v akci
    iocNo,          // Ne, preskocit akci, pokracovat dalsim objektem
    iocYesToAll,    // Ano pro vsehny dalsi objekty
    iocNewName,     // Ano, pokracovat v akci s novym jmenem objektu
    iocCancel       // Zrusit akci pro zbyvajici objekty
};

#define TEMP_ERROR (unsigned)-1
#define ODBC_TABLES_LINKED -1       // ODBC zdroj dat nelze zrusit, jsou z nej pripojene tabulky

#define ALL_CATEG_OBJS 0x40

CFNC DllExport void Remove_from_cache(cdp_t cdp, tcateg cat, ctptr name);

//
// Import
//
// Priznaky varovani
enum tdiwarn 
{
    IOW_REPLACE     // Nahradit objekt?, iocYes = Nahradit stavajici, iocYesToAll = Nahradit vsechny bez ptani, iocNewName = vytvorit objekt s novym jmenem
};
// Vysledek funkce Next pro import
enum tnextimpres
{
    NEXT_CONTINUE,  // Pokracovat
    NEXT_SKIP,      // Neimportovat, pokracovat dalsim objektem
    NEXT_END        // Konec, dalsi objekt neni, nebo chyba
};
//
// Vraci jmeno, kategorii, folder, cas zmeny dalsiho a velikost definice objektu. Param je param z t_impobjctx
// 
typedef tnextimpres (CALLBACK *LPIONEXT)(char *ObjName, tcateg *Categ, char *FolderName, uns32 *Modif, uns32 *Size, void *Param);
//
// V Defin vraci Size bytu z definice objektu, hodnotou je pocet skutecme nactenych bytu, (unsigned)-1 pri chybe
//
typedef unsigned   (CALLBACK *LPIOREAD)(char *Defin, unsigned Size, void *Param);
//
// Umoznuje uzivatelskou reakci na nejasne podminky Stat = situace z enum tdiwarn, Msg = Odpovidajici hlaska, 
// Single = import jednoho objektu / import mnoziny. Pri iocNewName vraci v ObjName nove jmeno
//
typedef tiocresult (CALLBACK *LPIOWARN)(unsigned Stat, const wchar_t *Msg, char *ObjName, tcateg Categ, bool Single, void *Param);
//
// Stat = vysledek importu, 0 = OK, jinak kod 602 chyby, Msg = odpovidajici hlaseni, Single = import jednoho objektu / import mnoziny
// Hodnota: true = pokracovat dalsim objektem, false = skoncit
//
typedef bool       (CALLBACK *LPIOSTAT)(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param);

#define IOF_REWRITE 0x0001  // Prepsat existujici objekt
//
// Custom object import description
// Enables external applications to implement object import with own user interface
//
struct t_impobjctx
{
    unsigned    strsize;            // Velikost struktury
    unsigned    count;              // Pocet importovanych objektu
    unsigned    flags;              // Priznaky
    tcateg      categ;              // Defaultni kategorie, pouzije se pokud nextfunc vrati NONECATEG   
    const char *file_names;         // Seznam zdrojovych souboru, pokud neni musi byt nextfunc
    void       *param;              // Uzivatelsky parametr
    LPIONEXT    nextfunc;           // Vraci jmeno, kategorii, folder, ... dalsiho objektu
    LPIOREAD    readfunc;           // Nacte definici objektu
    LPIOWARN    warnfunc;           // Varovani / uzivatelska modifikace
    LPIOSTAT    statfunc;           // Vysledek importu
};
//
// Object import context 
//
class DllKernel CImpObjCtx : public t_impobjctx 
{
protected:

    cdp_t       m_cdp;              // cdp
    tobjname    m_ObjName;          // Current object name
    tcateg      m_Categ;            // Current object category
    bool        m_Single;           // true - single object, false - object set import 
    bool        m_ReplaceAll;       // Replace existing objects
    bool        m_Duplicate;        // Duplicate existing objects        
    CBuffer     m_Buf;              // Buffer for object definition
    unsigned    m_Imported;         // Imported objects count 

    // Builds error message and calls virtual State function
    bool CallState(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ);
    bool CallState(unsigned Stat, const wchar_t *Msg)
    { return(CallState(Stat, Msg, m_ObjName, m_Categ));}

    tiocresult CheckObjName();

    // Returns next imported object description
    // Default implementation calls custom nextfunc function
    virtual tnextimpres Next(char *ObjName, tcateg *Categ, char *FolderName, uns32 *Modif, uns32 *Size)
    {
        if (!nextfunc)
            return(NEXT_END);
        return(nextfunc(ObjName, Categ, FolderName, Modif, Size, param));
    }
    // Reads object definition, returns number of bytes read
    // Default implementation calls custom readfunc function
    virtual unsigned  Read(char *Defin, unsigned Size)
    {
        if (!readfunc)
            return(WBF_INVALIDSIZE);
        return(readfunc(Defin, Size, param));
    }
    // Asks user how to continue in dubious situation
    // Default implementation calls custom warnfunc function
    virtual tiocresult Warning(unsigned Stat, const wchar_t *Msg, char *ObjName, tcateg Categ);
    // Shows object import result
    // Default implementation calls custom warnfunc function
    virtual bool       State(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ)
    {
        if (!statfunc)
            return(true);
        return(statfunc(Stat, Msg, ObjName, Categ, m_Single, param));
    }

public:

    CImpObjCtx(){}
    CImpObjCtx(cdp_t cdp, const t_impobjctx *ImpCtx, bool FailIfExist)
    {
        m_cdp          = cdp;
        m_ReplaceAll   = false;
        m_Duplicate    = false;
        m_Imported     = 0;
        if (ImpCtx)
            memcpy((t_impobjctx *)this, ImpCtx, sizeof(t_impobjctx));
        else
        {
            memset((t_impobjctx *)this, 0, sizeof(t_impobjctx));
            strsize = sizeof(t_impobjctx);
            count   = 1;
        }
        m_Single    = count == 1;
        if (!ImpCtx && !FailIfExist)
            flags = IOF_REWRITE;
    }

    unsigned Import();
    bool ImportObject(const char *ObjName, tcateg Categ, unsigned Size, const char *FolderName, uns32 Modif);
};
//
// Object import from file
//
class CFileImpObj : public CImpObjCtx
{
protected:

    CWBFile       m_File;                   // Source file
    const char   *m_Current;                // Pointer to current file name in file_names list
    char          m_GraphName[MAX_PATH];    // Buffer for graph object file name
public:
    const char   *m_FolderName;             // Destination folder name
protected:    
    tobjname      m_Folder;                 // Buffer for folder from object descriptor
    uns32         m_Modif;                  // Object modif timestamp
    bool          m_TryGraph;               // Try to import graph object for view object

    virtual tnextimpres Next(char *ObjName, tcateg *Categ, char *FolderName, uns32 *Modif, uns32 *pSize);
    virtual unsigned    Read(char *Defin, unsigned Size);
    virtual bool        State(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ);

    const char *NextFileName();

public:

    //
    // Constructor
    // Input:   Category    - Object category
    //          FileNames   - List of input file names terminated with additional \0 character
    //          FolderName  - Destination folder name
    //          FailIfExist - If true and imported object exists, function fails, if false, object will be replaced
    //
    CFileImpObj(cdp_t cdp, tcateg Categ, const char *FileNames, const char *FolderName, bool FailIfExist) : CImpObjCtx(cdp, NULL, FailIfExist)
    {
        m_Current    = NULL;
        m_FolderName = FolderName;
        m_TryGraph   = false;
        m_Single     = FileNames[strlen(FileNames) + 1] == 0;
        categ        = Categ;
        file_names   = FileNames;
    }
    //
    // Constructor
    // Input:   ImpCtx      - Custom import description
    //
    CFileImpObj(cdp_t cdp, const t_impobjctx *ImpCtx) : CImpObjCtx(cdp, ImpCtx, true)
    {
        m_Current    = NULL;
        m_FolderName = NULL;
        m_TryGraph   = false;
        m_Single     = file_names[strlen(file_names) + 1] == 0;
    }
};
//
// Export
//
//
// Vraci jmeno a kategorii dalsiho objektu, false pokud dalsi objekt neni. 
// Categ | ALL_CATEG_OBJS = exportovat vsechny objekty dane kategorie z folderu ObjName
// 
typedef bool (CALLBACK *LPEONEXT)(char *ObjName, CCateg *Categ, void *Param);
//
// Informuje o zahajeni exportu objektu, Size = velikost definice objektu, vraci false pokud se nema dale pokracovat
//
typedef tiocresult (CALLBACK *LPEOOPEN)(const char *ObjName, tcateg Categ, unsigned Size, void *Param);
//
// V Defin vraci Size bytu definice objektu, vraci false pokud se nema dale pokracovat
//
typedef bool (CALLBACK *LPEOWRITE)(const char *Defin, unsigned Size, void *Param);
//
// Varovani pokud soubor existuje
//
typedef tiocresult (CALLBACK *LPEOWARN)(char *FileName, const char *ObjName, tcateg Categ, bool Single, void *Param);
//
// Stat = vysledek exportu, 0 = OK, jinak kod 602 chyby, Msg = odpovidajici hlaseni, Single = export jednoho objektu / export mnoziny
// Hodnota: true = pokracovat dalsim objektem, false = skoncit
//
typedef bool (CALLBACK *LPEOSTAT)(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param);

// Priznaky exportu
#define EOF_WITHDESCRIPTOR       0x0001  // S informaci o folderu a casu posledni modifikace
//
// Custom object export description
// Enables external applications to implement object export with own user interface
//
struct t_expobjctx
{
    unsigned    strsize;            // Velikost struktury
    unsigned    count;              // Pocet exportovanych objektu
    unsigned    flags;              // Priznaky
    tobjname    schema;             // Zdrojova aplikace
    const char *obj_name;           // Jmeno exportovaneho objektu, pokud neni, musi byt nextfunc
    tcateg      categ;              // Kategorie exportovaneho objektu
    const char *file_name;          // Jmeno souboru nebo adresare, pokud neni, musi byt writefunc 
    void       *param;              // Uzivatelsky parametr
    LPEONEXT    nextfunc;           // Vraci jmeno a kategorii dalsiho objektu
    LPEOOPEN    openfunc;           // Otevre storage pro exportovany objekt
    LPEOWRITE   writefunc;          // Zapise definici objektu
    LPEOWARN    warnfunc;           // Varovani / uzivatelska modifikace
    LPEOSTAT    statfunc;           // Vysledek exportu
};
//
// Object export context
//
class CExpObjCtx : public t_expobjctx 
{
protected:

    cdp_t       m_cdp;              // cdp
    bool        m_Single;           // true - single object, false - object set export                
    CBuffer     m_Buf;              // Buffer for object definition
    CObjectList m_ol;               // Object enumerator
    unsigned    m_Exported;         // Exported object count

    bool CallState(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ);
    bool ExportObject(const char *ObjName, tcateg Categ);
    bool ExportObject(const char *ObjName, tcateg Categ, tobjnum ObjNum);
    bool ExportCateg(const char *Folder, CCateg Categ);
    bool ExportFolder(const char *Folder);
    t_varcol_size GetDefin(tobjnum ObjNum, tcateg Categ, bool WithDescr);

    // Returns next exported object name and category
    // Default implementation calls custom nextfunc function
    bool Next(char *ObjName, CCateg *Categ)
    {
        if (!nextfunc)
             return(false);
        return(nextfunc(ObjName, Categ, param));
    }
    // Opens storage for exported object
    // Default implementation calls custom openfunc function
    virtual tiocresult Open(const char *ObjName, tcateg Categ, unsigned Size)
    {
        if (!openfunc)
            return(iocYes);
        return(openfunc(ObjName, Categ, Size, param));
    }
    // Writes object definition to destination storage
    // Default implementation calls custom writefunc function
    virtual bool Write(const char *Defin, unsigned Size)
    {
        if (!writefunc)
            return(true);
        return(writefunc(Defin, Size, param));
    }
    // Shows object export result
    // Default implementation calls custom statfunc function
    virtual bool State(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ)
    {
        if (!statfunc)
            return(Stat == 0);
        return(statfunc(Stat, Msg, ObjName, Categ, m_Single, param));
    }
    bool     Export(const char *ObjName, CCateg Categ);

public:

    CExpObjCtx(){}
    CExpObjCtx(cdp_t cdp, const t_expobjctx *ExpCtx);
    unsigned Export();
};
//
// Object export to file
//
class CFileExpObj : public CExpObjCtx
{
    CWBFile     m_File;         // Destination file
    bool        m_FailIfExists; // If true and destination file exists, export fails, if false, destination file will be rewriten

    virtual tiocresult Open(const char *ObjName, tcateg Categ, unsigned Size);
    virtual bool       Write(const char *Defin, unsigned Size);
    virtual bool       State(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ);

public:

    CFileExpObj(cdp_t cdp, const t_expobjctx * ExpCtx) : CExpObjCtx(cdp, ExpCtx)
    {
        flags         |= EOF_WITHDESCRIPTOR;
        m_FailIfExists = true;
    }
    void SetGraphFileName();
};
//
// Schema export context
//
class CExpSchemaCtx : public t_export_param, public C602SQLApi
{
protected:

    bool         m_AskRewrite;              // Ask for rewrite destination files
    bool         m_OK;                      // Export result
    CWBFileX     m_AplFile;                 // Schema description file
    char         m_ExportPath[MAX_PATH];    // Destination path
    CWBFileTime  m_ExportDT;                // Date and time of export
    ttablenum    m_Table;                   // Source table number (TABTAB/OBJTAB)
    tobjname     m_CurSchema;               // Current schema name
    C602SQLQuery m_SchemaObjs;              // Exported object query

    void ExportObjects();
    void ReportProgress(int Categ, const wchar_t *Value);
    void write_role_def(tobjnum robjnum);
    void write_obj_privils(BOOL datapriv, ttablenum tab, tobjnum objnum, BOOL with_usergrp, int column_coun);
    bool GetObjectFileName(const char *ObjName, const char *Suffix, char *Dst);
#if 0
    bool is_include(trecnum rec);
#endif

public:

    CExpSchemaCtx(cdp_t cdp, const t_export_param *ep);
    bool Export();
};
//
// Schema import
//
#define SCAN_MAX_SYM  200  // >= 2*PRIVIL_DESCR_SIZE

typedef enum { SC_NO=0,
  SC_TABLE, SC_VIEW,    SC_MENU,  SC_CURSOR,  SC_PROGRAM, SC_PICTURE,
  SC_ROLE,  SC_CONNECT, SC_RELAT, SC_DRAWING, SC_GRAPH,   SC_REPLREL,
  SC_DATA,  SC_ASCII,   SC_GROUP, SC_ICONA,   SC_DATAPRV, SC_APPLICATION,
  SC_PROC,  SC_TRIGGER, SC_WWW,   SC_FOLDER,  SC_SEQUEN,  SC_INFO,
  SC_DOMAIN,SC_STSH,    SC_END
} t_sckey;
//
// Schema import context
//
class CImpSchemaCtx : public t_import_param, public C602SQLApi
{
protected:

    CBuffer  m_ApplBuf;                     // Buffer for schema description APL file
    CBuffer  m_ObjBuf;                      // Buffer for object definition
    int      m_bufpos;                      // Position in schema description
    char     m_cc;                          // Current char in schema description
    char     m_sym[SCAN_MAX_SYM + 1];       // Current symbol in schema description (system charset)
    char     m_UTF8sym[SCAN_MAX_SYM + 1];   // Current symbol in schema description (UTF8)
    bool     m_quoted;                      // Current symbol is by ` covered
    int      m_SrcCharSet;                  // Charset of APl file, Windows1250 for old exports, UTF8 for new exports
    CWBFileX m_ObjFile;                     // Object definition file
    bool     m_some_subjects_not_found;

    void next_char(void){m_cc = ((const char *)m_ApplBuf)[m_bufpos++];}
    void next_sym(void);
    t_sckey clasify(void);
    int curr_pos(void) const { return m_bufpos; }
    void conv_hex(const char *source, uns8 *dest, unsigned size);
    void conv_hex_long(uns8 *dest, unsigned size);
    bool apx_start(tptr fname, apx_header *apx, unsigned &header_size);
    void define_privils(tobjnum table, trecnum rec, tobjnum importer);
    void define_role(tobjnum objnum);
    void make_link(tptr defin, tcateg categ, char *objname, BOOL from_v4);
    void open_item_file(char *pathname, const char *srcpath, const char *itemname, const char *app);
    void apx_conversion(tptr fname, unsigned header_size);
    void ReportProgress(int Categ, const wchar_t *Value);
    void ReportError();
    void ReportSysError();
    void Done();
    void delayed_removing_importers_global_data_privils(trecnum & tabnum);

#ifdef WINS
    void copy_file(const char *filen, const char *ipath, const char *opath);
#endif

public:

    CImpSchemaCtx(cdp_t cdp, const t_import_param *ip);

    bool Import();
};
//
// Mazani
//
// Vraci jmeno a kategorii dalsiho objektu, false pokud dalsi objekt neni. Param je param z t_delobjctx
// Categ | ALL_CATEG_OBJS = smazat vsechny objekty dane kategorie z folderu ObjName
// 
typedef bool       (CALLBACK *LPDONEXT)(char *ObjName, CCateg *Categ, void *Param);
//
// Umoznuje uzivatelskou reakci na nejasne podminky Stat = situace z enum tdowarn, Msg = Odpovidajici hlaska, 
// Single = mazani jednoho objektu / mazani mnoziny.
//
typedef tiocresult (CALLBACK *LPDOWARN)(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param);
//
// Stat = vysledek mazani, 0 = OK, jinak kod 602 chyby, Msg = odpovidajici hlaseni, Single = import jednoho objektu / import mnoziny
// Hodnota: true = pokracovat dalsim objektem, false = skoncit
//
typedef bool       (CALLBACK *LPDOSTAT)(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param);

// Verejne priznaky mazani
#define DOF_NOCONFIRM       0x0001  // Nepotvrzovat mazani
#define DOF_CONFSTEPBYSTEP  0x0002  // Potvrzovat jednotlive
   
// Interni priznaky
#define DOPF_NOCONFIRM      0x0001  // Nepotvrzovat mazani objektu
#define DOPF_CONFSTEPBYSTEP 0x0002  // Potvrzovat jednotlive
#define DOPF_NOEMPTYFLDRS   0x0003  // Nepotvrzovat mazani neprazdnych adresaru
#define DOPF_NOCONFADMIN    0x0004  // Nepotvrzovat mazanini Conf_admina
#define DOPF_NOSECURADMIN   0x0008  // Nepotvrzovat mazanini Secur_admina
#define DOPF_NOCASCADEDEL   0x0010  // Nepotvrzovat kaskadni mazani tabulek a domen
#define DOPF_NOCONFIRMASK  (DOPF_NOCONFIRM | DOPF_NOEMPTYFLDRS | DOPF_NOCONFADMIN | DOPF_NOSECURADMIN | DOPF_NOCASCADEDEL)

// Priznaky varovani
enum tdowarn 
{
    DOW_CONFIRM,                    // Smazat objekt?
    DOW_CONFIRMALL,                 // Smazat vsechny objekty?
    DOW_FLDRNOTEMPTY,               // Slozka neni prazdna?
    DOW_CONFADMIN,                  // Existuje jiny Conf_admin?
    DOW_SECURADMIN,                 // Existuje jiny Secur_admin?
    DOW_CASCADEDEL                  // Smazat zavisle tabulky?
};
//
// Custom object delete description
// Enables external applications to implement object delete with own user interface
//
struct t_delobjctx
{
    unsigned strsize;               // Velikost struktury
    unsigned count;                 // Pocet mazanych objektu
    unsigned flags;                 // Priznaky
    tobjname schema;                // Aplikace
    void    *param;                 // Uzivatelsky parametr
    LPDONEXT nextfunc;              // Vraci jmeno a kategorii dalsiho objektu
    LPDOWARN warnfunc;              // Varovani
    LPDOSTAT statfunc;              // Vysledek
};
//
// Object delete context
//
class CDelObjCtx : public t_delobjctx
{                                       
protected:

    cdp_t       m_cdp;              // cdp
    bool        m_Single;           // true - single object, false - object set delete
    unsigned    m_Flags;            // Object delete flags DOPF_...
    unsigned    m_Deleted;          // Deleted object count
    CObjectList m_ol;               // Object enumerator

    bool CallState(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ);
    bool DeleteObject(const char *ObjName, tcateg Categ);
    bool DeleteFolder(const char *FolderName);
    bool DeleteCateg(const char *FolderName, tcateg Categ);

    // Returns next deleted object and category
    // Default implementation calls custom nextfunc function
    virtual bool Next(char *ObjName, CCateg *Categ)
    {
        if (!nextfunc)
            return(false);
        return(nextfunc(ObjName, Categ, param));
    }
    // Asks user how to continue in dubious situation
    // Default implementation calls custom warnfunc function
    virtual tiocresult Warning(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ);
    // Shows object delete result
    // Default implementation calls custom statfunc function
    virtual bool       State(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ);

public:

    CDelObjCtx(cdp_t cdp, const char *Schema, bool Ask);
    CDelObjCtx(cdp_t cdp, const t_delobjctx *DelCtx);
    unsigned Delete();
    bool Delete(const char *ObjName, CCateg Categ);
};
//
// Returns current schema UUID as string
//
inline const char *SelSchemaUUID2str(cdp_t cdp, char *Buf)
{
    bin2hex(Buf, cdp->sel_appl_uuid, UUID_SIZE);
    Buf[2 * UUID_SIZE] = 0;
    return Buf;
}

#if 0
const char *GetPictSuffix(int PictType);
#endif

#define UTF_8_BOM   0xBFBBEF
#define UTF_8_BOMSZ 3

const wchar_t *GetErrMsg(cdp_t cdp, int Err, wchar_t *Buf);

bool DeleteObject(cdp_t cdp, const char *ObjName, tcateg Categ, const char *SchemaName = NULL);

int  get_object_descriptor_data(cdp_t cdp, const char* buf, bool UTF8, tobjname folder_name, uns32 * stamp);
void update_cached_info(cdp_t cdp);
bool IsFolderEmpty(cdp_t cdp, const char *Folder);
bool rename_object_sql_nopref(cdp_t cdp, const char * categ_name, const char *OldName, const char *NewName);

extern "C"
{
    DllKernel void WINAPI enc_buf(WBUUID uuid, const char * objname, char *buf, int len);
    DllKernel BOOL WINAPI Export_appl_param(cdp_t cdp, const t_export_param *epIn);
    DllKernel BOOL WINAPI Import_appl_param(cdp_t cdp, const t_import_param *ip);
    DllKernel BOOL WINAPI Rename_object(cdp_t cdp, const char *OldName, const char *NewName, tcateg Categ);
    DllKernel BOOL WINAPI Set_object_folder_name(cdp_t cdp, tobjnum objnum, tcateg categ, const char * folder_name);
    DllKernel BOOL WINAPI Create_folder(cdp_t cdp, const char *FolderName, const char *ParentName);
    DllKernel BOOL WINAPI Delete_object(cdp_t cdp, const char *ObjName, tcateg Categ, const char *SchemaName, bool Ask);
    DllKernel BOOL WINAPI Import_object(cdp_t cdp, tcateg Categ, const char *FileName, const char *FolderName, bool FailIfExists);
    DllKernel BOOL WINAPI Import_objects(cdp_t cdp, tcateg Categ, const char *FileNames, const char *FolderName, bool FailIfExists);
    DllKernel unsigned WINAPI Import_objects_ex(cdp_t cdp, const t_impobjctx *ImpCtx, const char * FolderName);
    DllKernel unsigned WINAPI Delete_objects(cdp_t cdp, const t_delobjctx *DelCtx);
    DllKernel unsigned WINAPI Export_objects(cdp_t cdp, const t_expobjctx *ExpCtx);

    DllKernel BOOL WINAPI ImportObject(cdp_t cdp, tcateg Categ, const char *Defin, const char *ObjName, const char *FolderName, uns32 Modif);
    DllKernel void WINAPI SetControlPanelInterf(CCPInterface *Interf);
    DllKernel void WINAPI AddObjectToCache(cdp_t cdp, const char *ObjName, tobjnum objnum, tcateg Categ, uns16 Flags, const char *FolderName);
    DllKernel void WINAPI RefreshControlPanel(const char *ServerName, const char *SchemaName);
    DllKernel const char *WINAPI object_fname(cdp_t cdp, char *dst, const char *src, const char * app);
    DllKernel const char *WINAPI ToUTF8(cdp_t cdp, const char *Src, char *Buf, int Size);
    DllKernel const char *WINAPI GetSuffix(tcateg Categ);
    DllKernel bool WINAPI IsUTF8(const char *Text, int Size = -1);
    DllKernel bool WINAPI GetFreeObjName(cdp_t cdp, char *ObjName, tcateg Categ);
    DllKernel bool WINAPI not_encrypted_object(cdp_t cdp, tcateg categ, tobjnum objnum);
    DllKernel bool WINAPI Get_obj_modif_time(cdp_t cdp, tobjnum objnum, tcateg categ, uns32 * ts);
    DllKernel bool WINAPI check_object_access(cdp_t cdp, tcateg categ, tobjnum objnum, int privtype);
    DllKernel bool WINAPI IsObjNameFree(cdp_t cdp, const char *ObjName, tcateg Categ, tobjnum *pObjnum = NULL, tcateg *pCateg = NULL);
}

inline const char *object_fname(cdp_t cdp, char *dst, const char *src, tcateg Categ)
{
    return object_fname(cdp, dst, src, GetSuffix(Categ));
}

#endif // _IMPEXP_H_
