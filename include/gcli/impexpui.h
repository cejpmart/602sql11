#ifndef _IMPEXPUI_H_
#define _IMPEXPUI_H_

#include "wx/dataobj.h"
#include "wx/filename.h"

#include "impexp.h"

extern wxDataFormat * DF_602SQLObject;

//
// Copy, Paste, Drag&Drop
// 
#define COF_CUT       0x0001  // Cut - po paste na puvodnim miste smazat
//
// Vraci jmeno a kategorii dalsiho objektu, false pokud dalsi objekt neni. Param je param z t_copyobjctx
// Categ | ALL_CATEG_OBJS = kopirovat vsechny objekty dane kategorie z folderu ObjName
// 
typedef bool     (CALLBACK *LPCONEXT)(char *ObjName, CCateg *Categ, void *Param);
//
// Custom object copy description
//
struct t_copyobjctx
{
    unsigned  strsize;           // Velikost struktury
    unsigned  count;             // Pocet kopirovanych objektu
    unsigned  flags;             // Priznaky
    tobjname  schema;            // Zdrojova aplikace      
    tobjname  folder;            // Zdrojova slozka
    void     *param;             // Uzivatelsky parametr
    LPCONEXT  nextfunc;          // Vraci jmeno a kategorii dalsiho objektu
};
//
// Single copied object
//
class C602SQLObjName
{
public:

    tobjnum  m_ObjNum;
    tobjname m_ObjName;
    tcateg   m_Categ;
};

#define CBO_CUT 1               // Cut - after paste delete source objects
//
// List of copied objects
//
class C602SQLObjects
{
public:

    char           m_Server[MAX_OBJECT_NAME_LEN + 1];   // Source server
    tobjname       m_Schema;                            // Source schema
    tobjname       m_Folder;                            // Source folder
    unsigned long  m_SrcThread;                         // ID of current thread
    int            m_CharSet;                           // Source charset       
    unsigned       m_Flags;                             // Flags of copy - CBO_CUT
    unsigned       m_Count;                             // Copyied object count
    unsigned       m_Size;                              // Size of m_Obj array
    C602SQLObjName m_Obj[0];                            // Array of copied objects
    //
    // Allocates array for Count objects
    //
    static C602SQLObjects *Allocc(unsigned Count)
    {
        C602SQLObjects *Objs = (C602SQLObjects *)corealloc(sizeof(C602SQLObjects) + Count * sizeof(C602SQLObjName), 5);
        if (Objs)
        {
            Objs->m_Count = 0;
            Objs->m_Size  = Count;
        }
        return(Objs);
    }
    //
    // Allocates array for copied objects
    //
    static C602SQLObjects *Allocs(unsigned Size)
    {
        return((C602SQLObjects *)corealloc(Size, 5));
    }
    //
    // Returns Index-th element of array Objs, array is reallocated if needed
    //
    static C602SQLObjName *GetItem(C602SQLObjects **Objs, unsigned Index)
    {
        if (Index >= (*Objs)->m_Size)
        {
            (*Objs)->m_Size += 10;
            *Objs = (C602SQLObjects *)corerealloc(*Objs, sizeof(C602SQLObjects) + (*Objs)->m_Size * sizeof(C602SQLObjName));
            if (!*Objs)
                return(NULL);
        }
        return(&(*Objs)->m_Obj[Index]);
    }
    void Init(cdp_t cdp, const char *Schema, const char *Folder, bool Cut);
    bool RootOnlyObjects() const;
    bool Contains(CCateg Categ) const;
    bool ContainsOnly(CCateg Categ) const;
    bool ContainsFolder(cdp_t cdp, const char *Folder, bool DestToo) const;

    unsigned GetSize(){return(sizeof(C602SQLObjects) + m_Count * sizeof(C602SQLObjName));}
    void Free(){if (this) corefree(this);}
};
//
// wxDataObject implementation for copying database objects
//
class C602SQLDataObject : public wxDataObject
{
protected:

    C602SQLObjects *m_ObjData;
    size_t          m_ObjSize;

public:

    C602SQLDataObject()
    {
        m_ObjData = NULL;
        m_ObjSize = 0;
    }
    C602SQLDataObject(C602SQLObjects *Data, size_t Size)
    {
        m_ObjData = Data;
        m_ObjSize = Size;
    }

   ~C602SQLDataObject()
    {
        m_ObjData->Free();
    }
    virtual wxDataFormat GetPreferredFormat(Direction Dir = Get) const;
    virtual size_t GetFormatCount(Direction Dir = Get) const;
    virtual void GetAllFormats(wxDataFormat *Formats, Direction Dir = Get) const;
    virtual size_t GetDataSize(const wxDataFormat &Format) const;
    virtual bool GetDataHere(const wxDataFormat &Format, void *Buf) const;
    virtual bool SetData(const wxDataFormat& Format, size_t Len, const void *Buf);

#ifdef WINS
    virtual const void* GetSizeFromBuffer( const void* buffer, size_t* size, const wxDataFormat& format);
#endif

    C602SQLObjects *GetDataFromClipboard();
    C602SQLObjects *GetObjects(){return(m_ObjData);}

    friend class CCPDragSource;
};
//
// Copy context
//
class CCopyObjCtx
{
protected:

    cdp_t           m_cdp;      // cdp
    C602SQLObjects *m_Objs;     // Copied object list
    CObjectList     m_ol;       // ObjectList

    bool CopyObject(const char *ObjName, tobjnum ObjNum, tcateg Categ);
    bool CopyCateg(const char *FolderName, CCateg Categ);
    bool CopyFolder(const char *FolderName);

public:

    CCopyObjCtx(cdp_t cdp, const char *Schema) : m_ol(cdp, Schema)
    {
        m_cdp  = cdp;
        m_Objs = NULL;
    }
    C602SQLDataObject *Copy(const t_copyobjctx *ctx);
    C602SQLDataObject *Copy(const char *ObjName, CCateg Categ, const char *FolderName, const char *SchemaName, bool Cut);
};
//
// Paste context
//
class CPasteObjCtx : public CImpObjCtx
{
protected:

    const char     *m_dSchema;      // Destination schema
    const char     *m_dFolder;      // Destination folder
    cdp_t           m_scdp;         // Source cdp
    const char     *m_sSchema;      // Source schema
    unsigned        m_Index;        // Current object index
    C602SQLObjects *m_Objs;         // Copied object list
    CObjectList     m_sfl;          // ObjectList for souce folder browse
    CObjectList     m_dfl;          // ObjectList for destination folder browse
    CBuffer         m_Defin;        // Current pasted object definition

    virtual tnextimpres Next(char *ObjName, tcateg *Categ, char *FolderName, uns32 *Modif, uns32 *Size); // For paste
    virtual unsigned    Read(char *Defin, unsigned Size);
    virtual bool        State(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ);

    const char *GetFolder(const char *SrcFolder, char *DstFolder);
    void MoveToFolder();
    void Copy(char *Dst, const char *Src)
    {
        if (m_cdp->sys_charset == m_Objs->m_CharSet)
            strcpy(Dst, Src);
        else
            superconv(ATT_STRING, ATT_STRING, Src, Dst, t_specif(0, m_Objs->m_CharSet, 0, 0), t_specif(0, m_cdp->sys_charset, 0, 0), NULL);
    }
    static bool CALLBACK Next(char *ObjName, CCateg *Categ, void *Param); // For delete

public:

    CPasteObjCtx(cdp_t cdp, const char *Schema, const char *Folder) : CImpObjCtx(cdp, NULL, false)
    {
        m_dSchema    = Schema;
        m_dFolder    = Folder;
        m_Index      = 0;
        m_Objs       = NULL;
    }
    void PasteObjects(C602SQLObjects *objs);

};

class wbFileDialog : public wxFileDialog
{ long saved_style;
#ifdef LINUX
	virtual bool Validate();
    long GetStyle(void)  // method removed from wxFileDialog in version 2.8.0
      { return saved_style; }
#endif

public:

    wbFileDialog(wxWindow* parent, const wxString& message, const wxString& defaultDir, const wxString& defaultFile, const wxString& wildcard, long style) : wxFileDialog(parent, message, defaultDir, defaultFile, wildcard, style)
      { saved_style=style; }
};

class wbDirDialog : public wxDirDialog
{ 
#ifdef LINUX
	virtual bool Validate();
#endif

public:

    wbDirDialog(wxWindow* parent, const wxString& message, const wxString& defaultPath, long style) : wxDirDialog(parent, message, defaultPath, style){}
};

wxString GetFileTypeFilter(tcateg Categ);
wxString GetPictFilter(int PictType);

bool ImportSelObject(cdp_t cdp, tcateg Categ, const char *FileName, const char *FolderName);
bool ExportSelObject(cdp_t cdp, const char *objname, tcateg Categ, const char *Schema = NULL);
bool ImportSchema(cdp_t cdp);
bool ExportSchema(cdp_t cdp, const char *SchemaName);
bool CopySelObject(cdp_t cdp, const char *ObjName, tcateg Categ, const char *FolderName = NULL, const char *SchemaName = NULL, bool Cut = false); 
bool CopySelObjects(cdp_t cdp, const t_copyobjctx *CopyCtx); 
bool RenameObject(cdp_t cdp, char *Name, tcateg Categ);
bool ObjNameExists(cdp_t cdp, const char *ObjName, tcateg Categ);
unsigned ImportSelObjects(cdp_t cdp, tcateg Categ, const char *Folder);
bool ImpExpState(cdp_t cdp, bool OK, const wxChar *Fmt, const wxChar *Msg, const char *ObjName, tcateg Categ, bool Single);
bool CALLBACK ExpState(unsigned Stat, const wxChar *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param);
tiocresult CALLBACK FileExistsWarning(char *FileName, const char *ObjName, tcateg Categ, bool Single, void *Param);

void PasteObjects(cdp_t cdp, const char *Folder, const char *Schema = NULL);

wxArrayString SelectFileNames(wxString &Path, wxString Filter, wxWindow * parent = NULL);
wxString SelectFileName(wxString &Path, wxString Name, wxString Filter, bool ForSave, wxWindow * parent = NULL, wxString title = wxEmptyString);
wxString GetImportFileName(wxString InitName, wxString Filter, wxWindow * parent = NULL, wxString title = wxEmptyString);
wxArrayString GetImportFileNames(wxString Filter, wxWindow * parent = NULL);
wxString GetExportFileName(wxString InitName, wxString Filter, wxWindow * parent = NULL, wxString title = wxEmptyString);
wxString GetExportPath(wxWindow * parent = NULL);
wxString GetLastExportPath();

#ifdef WINS
#define FilterEXE  _("Executables (*.exe)|*.exe|")
#else
#define FilterEXE  _("Executables (*)|*|")
#endif
#define FilterAll  _("All files (*.*)|*.*")

#endif  // _IMPEXPUI_H_
