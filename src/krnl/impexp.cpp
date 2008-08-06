//
// Rutiny pro programovou praci s objekty bez vazby na ridici panel, import / paste objektu predpoklada nastaveny kontext
// cilove aplikace (nejde SQL_execute v jinem kontextu)
//
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#ifdef WINS
//#include <shlguid.h> -- not used any more
#endif
#include <stdio.h>
#include "wprez9.h"
#include "impexp.h"

#ifdef WINS
#define swprintf _snwprintf
#endif

#define gettext_noopL(string) L##string

#define ObjNameExistsMsg           gettext_noopL("%ls named \"%ls\" already exists")
#define EncNoExportMsg             gettext_noopL("Cannot export a protected object without encryption")
#define DEL_LINK_OBJECT            gettext_noopL("link object")
#define CASCADE_TABLE_DEL          gettext_noopL("%ls \"%ls\" cannot be deleted\nwithout deleting dependent tables.\nDelete all tables?")
#define FolderNotEmpty             gettext_noopL("The folder \"%ls\" is not empty. Do you want to delete the folder and all contents?")
#define FileExistsMsg              gettext_noopL("The file %ls already exists")
#define SchemaExportFail           gettext_noopL("Application export was NOT successful")
#define SchemaExportOK             gettext_noopL("Application export was successful")
#define AppFileDemaged             gettext_noopL("Application description file is damaged")
#define SchemaImportOK             gettext_noopL("Application import was successful")
#define SchemaImportFail           gettext_noopL("Application import was NOT successful")
#define CannotConvertLink          gettext_noopL("Non-existing application links cannot be converted from older versions.")
#define CannotConvertOutStr        gettext_noopL("Failed to convert the string \"%ls\"")
#define CannotConvertInStr         gettext_noopL("The string \"%ls\" is not convertible to the SQL Server charset.")
#define SomeSubjectsNotFound       gettext_noopL("Some subjects specified in the appliation are not found on the server.")
#define DELETING_CONF_ADMIN        gettext_noopL("You are about to delete a configuration administrator. Are you sure that there is at least one other user that is assigned to the configuration administrator role?")
#define DELETING_SECUR_ADMIN       gettext_noopL("You are about to delete a security administrator. Are you sure that there is at least one other user that is assigned to the security administrator role?")
#define DELETE_CONFIRM_PATTERN     gettext_noopL("Do you really want to delete the %ls %ls?")
#define DELETE_ALL_CONFIRM_PATTERN gettext_noopL("Do you really want to delete ALL selected objects?")
#define SchemaNameExists           gettext_noopL("The schema \"%ls\" already exists")
#define SchemaUUIDExists           gettext_noopL("The schema \"%ls\" has the same UUID as the imported schema")
//
// Converts string to UTF8
// Input:   Src   - Source string
//          Buf   - Buffer for conversion
//          Size  - Size of buffer
// Returns: UTF8 string
//
CFNC DllKernel const char * WINAPI ToUTF8(cdp_t cdp, const char *Src, char *Buf, int Size)
{
    int Len = (int)strlen(Src);
    if (superconv(ATT_STRING, ATT_STRING, Src, Buf, t_specif(Len, cdp->sys_charset, 0, 0), t_specif(Size, 7, 0, 0), NULL) < 0)
        return NULL;
    return(Buf);
}
//
// Replaces special characters in string with ? and truncates string to 31 characters
//
const char *RegulString(const char *Src, char *Buf)
{
    const char *p;
    char *d = Buf;
    for (p = Src; *p && p < Src + 31; p++)
    {
        uns8 c = *p;
        if ((c < ' ' || c >= 0x80) && c != '\r' && c != '\n')
            c = '?';
        *d++ = c;
    }
    if (p < Src + 31)
        *d = 0;
    else
        strcpy(Buf + 28, "...");
    return Buf;
}
//
// Converts string to UTF8
// Input:   Src   - Source string
//          Buf   - Buffer for conversion
// Returns: UTF8 string
//
const char *ToUTF8(cdp_t cdp, const char *Src, CBuffer &Buf)
{
    int Len = (int)strlen(Src) * 2;
    Buf.AllocX(Len);
    const char *Result = ToUTF8(cdp, Src, Buf, Len);
    if (!Result)
    {
        char b[32];
        wchar_t wb[32];
        wchar_t fmt[64];
        throw new CWBException(Get_transl(cdp, CannotConvertOutStr, fmt), ToUnicode(cdp, RegulString(Src, b), wb, sizeof(wb) / sizeof(wchar_t)));
    }
    return Result;
}
//
// Converts string to system charset
// Input:   Src        - Source string
//          Dst        - Buffer for conversion
//          DstLen     - Size of buffer
//          SrcCharSet - Source string charset 
// Returns: Converted string
//
const char *ToLocale(cdp_t cdp, const char *Src, char *Dst, int DstLen, int SrcCharSet)
{
    if (SrcCharSet == cdp->sys_charset)
    {   //strcpy(Dst, Src);  // J.D.
        return(Src);
    }
    bool Err = superconv(ATT_STRING, ATT_STRING, Src, Dst, t_specif(DstLen, SrcCharSet, 0, 0), t_specif(DstLen, cdp->sys_charset, 0, 0), NULL) < 0;
#ifdef STOP  // allow imports of national charcters into databases with ASCII system charset -> in UTF-8
    if (!Err &&  cdp->sys_charset == 0)
    {
        for (unsigned char *p = (unsigned char *)Dst; *p; p++)
        {
            Err = *p >= 0x80;
            if (Err)
                break;
        }
    }
#endif
    if (Err)
    {
        char b[32];
        wchar_t wb[32];
        wchar_t fmt[64];
        throw new CWBException(Get_transl(cdp, CannotConvertInStr, fmt), ToUnicode(cdp, RegulString(Src, b), wb, sizeof(wb) / sizeof(wchar_t)));
    }
    return(Dst);
}
//
// Converts string to system charset
// Input:   Src        - Source string
//          Buf        - Buffer for conversion
//          SrcCharSet - Source string charset 
// Returns: Converted string
//
const char *ToLocale(cdp_t cdp, const char *Src, CBuffer &Buf, int SrcCharSet)
{
    if (SrcCharSet == cdp->sys_charset || SrcCharSet==CHARSET_NUM_UTF8 && cdp->sys_charset==0)
        return(Src);   // do not prevent import but the text will be in UTF-8
    int Len = (int)strlen(Src) + 1;
    Buf.AllocX(Len);
    return(ToLocale(cdp, Src, Buf, Len, SrcCharSet));
}
//
// Returns true if objects of given category have text definition
//
bool IsTextDefin(tcateg Categ) //, const char *def)
{
    if (!CCategList::HasTextDefin(Categ))
        return(false);
    //if (Categ == CATEG_PGMSRC && strnicmp(def, "<?xml", 5) == 0)
    //    return(false);
    return(true);
}
//
// Control panel refresh interface
//
CCPInterface *CPInterface;

CFNC DllKernel void WINAPI SetControlPanelInterf(CCPInterface *Interf)
{
    CPInterface = Interf;
}
//
// Prejmenovani objektu
//
tptr SkipSep(tptr p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        p++;
    return(p);
}

bool IsSep(char c)
{
    return(c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

tptr SkipIdent(tptr p)
{
    while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_' || *(uns8*)p >= 128)
        p++;
    return(p);
}

#define COMMENT_1 ('-' + ('-' << 8))
#define COMMENT_2 ('/' + ('/' << 8))
#define COMMENT_3 ('/' + ('*' << 8))

tptr SkipComment(tptr p)
{
    while (*p)
    {
        p = SkipSep(p);
        // --
        if (*(WORD *)p == COMMENT_1 || *(WORD *)p == COMMENT_2)
        {
            p = strchr(p, '\n');
            if (!p)
                break;
            p++;
            continue;
        }
        // /*
        if (*(WORD *)p == COMMENT_3)
        {
            p += 2;
            while (*p)
            {
                p = strchr(p, '*');
                if (!p)
                    break;
                p++;
                if (*p == '/')
                    break;
            }
            if (!p || !*p)
                break;
            p++;
            continue;
        }
        // {
        if (*p == '{')
        {
            p = strchr(p, '}');
            if (!p)
                break;
            p++;
            continue;
        }
        break;
    }
    return(p);
}

tptr RenameDefin(cdp_t cdp, tptr Defin, const char *NewName, tcateg Categ)
{
    tptr pe;
    tptr pn = SkipComment(Defin);
    switch (Categ)
    {
    case CATEG_PROC:
        if (strnicmp(pn, "PROCEDURE", 9) == 0 && IsSep(pn[9]))
            pn += 10;
        else if (strnicmp(pn, "FUNCTION", 8) == 0 && IsSep(pn[8]))
            pn += 9;
        else
            goto warn;
        break;
    case CATEG_TRIGGER:
        if (strnicmp(pn, "TRIGGER", 7) == 0 && IsSep(pn[7]))
            pn += 8;
        else
            goto warn;
        break;
    case CATEG_SEQ:
        if (strnicmp(pn, "CREATE", 6) == 0 && IsSep(pn[6]))
            pn += 7;
        else
            goto warn;
        pn = SkipComment(pn);
        if (strnicmp(pn, "SEQUENCE", 8) == 0 && IsSep(pn[8]))
            pn += 9;
        else
            goto warn;
        break;
    case CATEG_DOMAIN:
        if (strnicmp(pn, "DOMAIN", 6) == 0 && IsSep(pn[6]))
            pn += 7;
        else
            goto warn;
        break;
    }
    pn = SkipComment(pn);
    if (*pn == '`')
    {
        pn++;
        pe = strchr(pn, '`');
        if (!pe)
            goto warn;
    }
    else
    {
        pe = SkipIdent(pn);
    }
    int sz;
    sz = (int)strlen(NewName);
    if (pe - pn >= sz)
    {
        memcpy(pn,      NewName, sz);
        if (pe - pn > sz)
            strcpy(pn + sz, pe);
    }
    else
    {
        tptr newdef = (tptr)corealloc(coresize(Defin) + sz - (pe - pn), 210);
        if (!newdef)
        {
            corefree(Defin);
            client_error(cdp, OUT_OF_APPL_MEMORY);
            return(NULL);
        }
        memcpy(newdef, Defin, pn - Defin);
        memcpy(newdef + (pn - Defin), NewName, sz);
        strcpy(newdef + (pn - Defin) + sz, pe);
        corefree(Defin);
        Defin = newdef;
    }
    return(Defin);

warn:

    client_error(cdp, RENAME_IN_DEFIN);
    return(Defin);
}
//
// Import obecne - Vzdycky probiha v kontextu aktualni aplikace, hlavne protoze neumim provest CREATE TABLE v jinem kontextu
//
//
// ImportObject - Import objektu podle definice v bloku pameti
//
CFNC DllKernel BOOL WINAPI ImportObject(cdp_t cdp, tcateg Categ, const char *Defin, const char *ObjName, const char *FolderName, uns32 Modif)
{
    CBuffer  Buf;
    tobjnum  objnum;
    unsigned Len   = (unsigned)strlen(Defin);
    uns16    Flags = 0;
    bool     UTF8  = IsUTF8(Defin, Len);

    // IF na zacatku descriptor, preskocit
    if (Categ != CATEG_CONNECTION && Categ != CATEG_PICT && Categ != CATEG_GRAPH && Categ != CATEG_PGMEXE)
    {
        tobjname Folder;
        uns32 stamp = 0;

        char Desc[OBJECT_DESCRIPTOR_SIZE + OBJ_NAME_LEN + UTF_8_BOMSZ + 1];
        memcpy(Desc, Defin, OBJECT_DESCRIPTOR_SIZE + OBJ_NAME_LEN + UTF_8_BOMSZ);
        Desc[OBJECT_DESCRIPTOR_SIZE + OBJ_NAME_LEN + UTF_8_BOMSZ] = 0;  
        int ds = get_object_descriptor_data(cdp, Desc, UTF8, Folder, &stamp);
        Defin += ds;
        Len   -= ds;
    }
    try
    {
        if (CCategList::HasTextDefin(Categ))
            Defin = ToLocale(cdp, Defin, Buf, UTF8 ? 7 : 1);
    }
    catch (CWBException *e)
    {
        client_error(cdp, SQ_INV_CHAR_VAL_FOR_CAST);
        delete e;
        return true;
    }
    catch (...)
    {
        return true; 
    }
    Len   = (int)strlen(Defin);
    // IF CATEG_TABLE, ignorovat Flags, nacist z definice 
    if (Categ == CATEG_TABLE)
    {
        tptr source = NULL;
        table_all ta;
        int res = compile_table_to_all(cdp, Defin, &ta);
        if (res)
        { 
            client_error(cdp, res);
            return true; 
        }
        if (wb_stricmp(cdp, ta.selfname, ObjName) || (*ta.schemaname && wb_stricmp(cdp, ta.schemaname, cdp->sel_appl_name)))
        {
            strcpy(ta.selfname, ObjName);
            ta.schemaname[0]=0;
            source = table_all_to_source(&ta, FALSE);
            if (!source)
                return true;
            Defin = source;
        }
        Flags = ta.tabdef_flags & TFL_ZCR ? CO_FLAG_REPLTAB : 0;
        BOOL Err = cd_SQL_execute(cdp, Defin, (uns32 *)&objnum);
        if (source)
            corefree(source);
        if (Err)
            return true;
        inval_table_d(cdp, NOOBJECT, CATEG_TABLE);  // remove defs from the cache (intertable links should be refreshed)
    }
    else
    {
        bool NewDef = false;
        if (Categ == CATEG_PROC || Categ == CATEG_TRIGGER || Categ == CATEG_SEQ || Categ == CATEG_DOMAIN)
        {
            tptr def = (tptr)corealloc(Len + 1, 241);
            if (!def)
                return true;
            strcpy(def, Defin);
            Defin = (const char *)RenameDefin(cdp, def, ObjName, Categ);
            if (!Defin)
                return true;
            Len = (int)strlen(Defin);
        }

        if (Categ == CATEG_PGMSRC)
        {
            Flags = program_flags(cdp, (char *)Defin);
        }
        else if (Categ == CATEG_XMLFORM)
        {
            Flags = 0;
        }
        
#if 0
	    else if (Categ == CATEG_VIEW)
        {
            compil_info xCI(cdp, Defin, MEMORY_OUT, view_struct_comp);
            int res = compile(&xCI);
            if (!res)
            {
                Flags = view_flags((view_stat*)xCI.univ_code);
                corefree(xCI.univ_code);
            }
        }
        else if (Categ == CATEG_MENU)
        {   
            const char *p;
            for (p = Defin; *p==' ' || *p=='\r' || *p=='\n'; p++);
            if (!strnicmp(p, "POPUP", 5))
                Flags = CO_FLAG_POPUP_MN;
        }
#endif
        BOOL Err = cd_Insert_object(cdp, ObjName, Categ, &objnum);
        if (!Err)
        {
            Err = cd_Write_var(cdp, OBJ_TABLENUM, (trecnum)objnum, OBJ_DEF_ATR, NOINDEX, 0, Len, Defin);
            if (!Err)
            {
                Err = cd_Write_len(cdp, OBJ_TABLENUM, (trecnum)objnum, OBJ_DEF_ATR, NOINDEX, Len);
                if (!Err)
                {
                    if (Flags)
                        cd_Write_ind(cdp, OBJ_TABLENUM, (trecnum)objnum, OBJ_FLAGS_ATR, NOINDEX, &Flags, sizeof(uns16));
                    if (Categ == CATEG_TRIGGER)
                        cd_Trigger_changed(cdp, objnum, TRUE);  // register the new trigger
                }
            }
        }
        if (NewDef)
            corefree(Defin);
        if (Err)
            return true;
    }
    if (Categ != CATEG_USER && Categ != CATEG_PGMEXE && Categ != CATEG_CONNECTION && Categ != CATEG_GRAPH)
    {
        if (Categ != CATEG_PICT)
            Set_object_folder_name(cdp, objnum, Categ, FolderName);
        AddObjectToCache(cdp, ObjName, objnum, Categ, Flags, FolderName);
    }
    return false;
}
//
// ImportObjects - Import mnoziny objektu podle parametru v ImpCtx
//
CFNC DllKernel unsigned WINAPI Import_objects_ex(cdp_t cdp, const t_impobjctx *ImpCtx, const char * FolderName)
{
    if (ImpCtx->file_names)
    {
        CFileImpObj Ctx(cdp, ImpCtx);
        if (FolderName && *FolderName)  // otherwise use the folder name store in the object definition
          Ctx.m_FolderName = FolderName;
        return(Ctx.Import());
    }
    else
    {
        CImpObjCtx Ctx(cdp, ImpCtx, true);
        return(Ctx.Import());
    }
}
//
// Imports objects set, returns count of objects realy imported
//
unsigned CImpObjCtx::Import()
{
    unsigned    Size;
    tnextimpres Res;
    tcateg      Categ;
    uns32       Modif;
    char        ObjName[256];
    char        FolderName[256];

    // WHILE is next object to import
    while ((Res = Next(ObjName, &Categ, FolderName, &Modif, &Size)) != NEXT_END)
    {
        // IF object category not specified, set default
        if (Categ == NONECATEG)
            Categ = categ;
        // IF skip current object import, continue
        if (Res == NEXT_SKIP)
            continue;
        // IF import failed, break
        if (!ImportObject(ObjName, Categ, Size, FolderName, Modif))
            break;
    }
    // IF anything imported and control panel interface defined, refresh current schema on control panel
    if (m_Imported && CPInterface)
        CPInterface->RefreshSchema(m_cdp->locid_server_name, m_cdp->sel_appl_name);
    return(m_Imported);
}
//
// Imports one object
// Input:   ObjName     - Object name
//          Categ       - Object category
//          Size        - Object definition size
//          FolderName  - Destination folder name
//          Modif       - Object last modification timestamp
// Returns: false for break objects import process
//
bool CImpObjCtx::ImportObject(const char *ObjName, tcateg Categ, unsigned Size, const char *FolderName, uns32 Modif)
{
    strncpy(m_ObjName, ObjName, OBJ_NAME_LEN);
    m_ObjName[OBJ_NAME_LEN] = 0;
    m_Categ                 = Categ;
    
    // Check if object allready exists
    tiocresult Res = CheckObjName();
    // IF not rewrite object, skip import
    if (Res == iocNo)
        return(true);
    //  IF cancel import, break objects import process
    if (Res == iocCancel)
        return(false);
    // Realloc buffer for object definition
    if (!m_Buf.Alloc(Size + 1))
    { 
        client_error(m_cdp, OUT_OF_APPL_MEMORY);
        return(CallState(OUT_OF_APPL_MEMORY, NULL));
    }
    // Read object definition
    Size = Read(m_Buf, Size);
    if (Size == WBF_INVALIDSIZE)
        return(true);
    m_Buf[(int)Size] = 0;
    // Import object definition
    if (::ImportObject(m_cdp, m_Categ, m_Buf, m_ObjName, FolderName, Modif))
    {
        int Err  = cd_Sz_error(m_cdp);
        return(CallState(Err, NULL));
    }
    m_Imported++;
    return(State(0, NULL, ObjName, Categ));
}
//
// Checks if object m_ObjName of category m_Categ already exists
//
tiocresult CImpObjCtx::CheckObjName()
{
    tobjnum origobj;
    tcateg  origcat;
    char    NewName[256];
    tiocresult Res = iocYes;

    // WHILE existuje objekt stejneho jmena
    while (!IsObjNameFree(m_cdp, m_ObjName, m_Categ, &origobj, &origcat))
    {
        // EXIT IF Chyba
        if (origobj == NOOBJECT)
            return(CallState(cd_Sz_error(m_cdp), NULL) ? iocNo : iocCancel);
        // IF Duplikovat, nastavit nove jmeno
        if (m_Duplicate)
        {
            if (!GetFreeObjName(m_cdp, m_ObjName, m_Categ))
                return(CallState(LAST_DB_ERROR + 1, NULL) ? iocNo : iocCancel);
            return(iocYes);
        }
        // IF ReplaceAll, rovnou smazat puvodni
        if (m_ReplaceAll && !Delete_object(m_cdp, m_ObjName, origcat, NULL, false))
            return(iocYes);
        // Jinak zeptat se uzivatele co s tim
        CWBuffer Message;
        wchar_t wName[OBJ_NAME_LEN + 1];
        wchar_t Fmt[80];
        wchar_t wc[64];
        Message.Format(Get_transl(m_cdp, ObjNameExistsMsg, Fmt), CCategList::CaptFirst(m_cdp, m_Categ, wc), ToUnicode(m_cdp, m_ObjName, wName, sizeof(wName) / sizeof(wchar_t)));
        do
        {
            strcpy(NewName, m_ObjName);
            Res = Warning(IOW_REPLACE, Message, NewName, origcat);
            // IF Nahradit, smazat puvodni objekt
            if (Res != iocYes && Res != iocYesToAll)
                break;
            m_ReplaceAll |= (Res == iocYesToAll);
            // IF nejdeto zeptat se znovu
        }
        while (Delete_object(m_cdp, m_ObjName, origcat, NULL, false));
        // IF Nove jmeno, znovu zkontrolovat jestli je volne
        if (Res != iocNewName)
            break;
        strcpy(m_ObjName, NewName);
    }
    return(Res);
}
//
// Default implementation calls custom warnfunc function
//
tiocresult CImpObjCtx::Warning(unsigned Stat, const wchar_t *Msg, char *ObjName, tcateg Categ)
{
    if (warnfunc)
        return warnfunc(Stat, Msg, ObjName, Categ, m_Single, param);
    return((flags & IOF_REWRITE) ? iocYesToAll : iocCancel);
}
//
// Builds error message and calls custom statfunc function
//
bool CImpObjCtx::CallState(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ)
{
    wchar_t wMsg[256];
    if (Stat && !Msg)
    { 
        Get_error_num_textW(m_cdp, Stat, wMsg, sizeof(wMsg) / sizeof(wchar_t));
        Msg = wMsg;
    }
    return(State(Stat, Msg, ObjName, Categ));
}
//
// CFileImpObj Next virtual function implementation
//
tnextimpres CFileImpObj::Next(char *ObjName, tcateg *Categ, char *FolderName, uns32 *Modif, uns32 *pSize)
{
    tcateg      Cat;
    const char *fName;
    unsigned    Size = 0;
    for (;;)
    {
        Cat   = m_TryGraph ? CATEG_GRAPH : categ;
        // Jmeno dalsiho souboru
        fName = NextFileName();
        // EXIT IF dalsi neni
        if (!fName)
            return NEXT_END;
        fname2objname(fName, ObjName);
        m_TryGraph = false;
        // IF nejde otevrit
        wchar_t Msg[256];
        if (!m_File.Open(fName))
        {
            // IF Graph, pokracovat dalsim souborem
            if (Cat == CATEG_GRAPH)
                continue;
            // Zobrazit hlaseni
            if (!State(OS_FILE_ERROR, GetErrMsg(m_cdp, OS_FILE_ERROR, Msg), ObjName, Cat))
                return NEXT_END;
            continue;
        }
        // IF uzivatel, importovat uzivatele a pokracovat dalsim souborem
        if (categ == CATEG_USER)
        {
            int Err = 0;
            if (cd_Import_user(m_cdp, m_File))
                Err = cd_Sz_error(m_cdp);
            if (!State(Err, Err ? GetErrMsg(m_cdp, Err, Msg) : NULL, ObjName, CATEG_USER))
                return NEXT_END;
            continue;

        }
        Size = m_File.Size();
        const char *Folder = m_FolderName;
        if (!Folder && (Cat != CATEG_CONNECTION && Cat != CATEG_PICT && Cat != CATEG_GRAPH && Cat != CATEG_PGMEXE))
        {
            char Desc[OBJECT_DESCRIPTOR_SIZE + OBJ_NAME_LEN + UTF_8_BOMSZ + 1];
            if (m_File.Read(Desc, OBJECT_DESCRIPTOR_SIZE + OBJ_NAME_LEN + UTF_8_BOMSZ) == WBF_INVALIDSIZE)
            {
                wchar_t Msg[256];
                if (!State(OS_FILE_ERROR, GetErrMsg(m_cdp, OS_FILE_ERROR, Msg), ObjName, Cat))
                    return NEXT_END;
                continue;
            }
            uns8 *p = (uns8 *)Desc + OBJECT_DESCRIPTOR_SIZE + OBJ_NAME_LEN + UTF_8_BOMSZ;
            while (*p > 0x80)
                p--;
            *p = 0;
            if (!get_object_descriptor_data(m_cdp, Desc, IsUTF8(Desc), m_Folder, &m_Modif))
                *m_Folder = 0;
            Folder = m_Folder;
            m_File.Seek(0);
        }
        *Categ = Cat;
        if (Folder)
            strcpy(FolderName, Folder);
        else
            *FolderName = 0;
        *Modif = 0;
        break;
    }
    *pSize = Size;
    return NEXT_CONTINUE;
}

//
// Import_object - Import objektu ObjName ze souboru FileName, vraci false pri uspechu, true pri chybe
//
CFNC DllKernel BOOL WINAPI Import_object(cdp_t cdp, tcateg Categ, const char *FileName, const char *FolderName, bool FailIfExists)
{
    // check the privilege to insert records to system tables:
    uns8 Privils[PRIVIL_DESCR_SIZE];
    if (cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, CATEG2SYSTAB(Categ), NORECNUM, OPER_GETEFF, Privils) || !(*Privils & RIGHT_INSERT))
    {
        client_error(cdp, NO_RIGHT);
        return true;
    }
    else
    {
        char fNames[MAX_PATH + 1];
        int sz = (int)strlen(FileName);
        memcpy(fNames, FileName, sz + 1);
        fNames[sz + 1] = 0;
        CFileImpObj Imp(cdp, Categ, fNames, FolderName, FailIfExists);  // design error: FolderName will not be passed via t_impobjctx
        return Import_objects_ex(cdp, &Imp, FolderName) == 0;
    }
}
//
// Imports set of objects from set of files
// Input:   Categ       - Object category
//          FileNames   - List of input file names terminated with additional \0 character
//          FolderName  - Destination folder name
//          FailIfExist - If true and imported object exists, function fails, if false, object will be replaced
// Returns: FALSE on success
//
CFNC DllKernel BOOL WINAPI Import_objects(cdp_t cdp, tcateg Categ, const char *FileNames, const char *FolderName, bool FailIfExists)
{
    // check the privilege to insert records to system tables:
    uns8 Privils[PRIVIL_DESCR_SIZE];
    if (cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, CATEG2SYSTAB(Categ), NORECNUM, OPER_GETEFF, Privils) || !(*Privils & RIGHT_INSERT))
    {
        client_error(cdp, NO_RIGHT);
        return true;
    }
    else
    {
        // Create import context and provide import
        CFileImpObj Imp(cdp, Categ, FileNames, FolderName, FailIfExists);
        return Import_objects_ex(cdp, &Imp, FolderName) == 0;  // design error: FolderName will not be passed via t_impobjctx
    }
}
//
// Returns next file name from input list
// If pervious object was view, return graph object name to try import graph associated with view
//
const char *CFileImpObj::NextFileName()
{
    if (!m_TryGraph)
    {
        if (m_Current)
            m_Current += strlen(m_Current) + 1;
        else
            m_Current = file_names;
    }
    if (!*m_Current)
        return NULL;
    if (!m_TryGraph)
        return m_Current;
    strcpy(m_GraphName, m_Current);
    char *p = strrchr(m_GraphName, '.');
    if (!p)
        p = m_GraphName + strlen(m_GraphName);
    strcpy(p, ".grf");
    return(m_GraphName);
}
//
// Read virtual function implementation
//
unsigned CFileImpObj::Read(char *Defin, unsigned Size)
{
    return(m_File.Read(Defin, Size));
}
//
// State virtual function implementation
//
bool CFileImpObj::State(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ)
{
    // IF OK a importoval se pohled, zkusit k nemu graf
    if (!Stat)
    {
        if (Categ == CATEG_VIEW)
            m_TryGraph = true;
    }
    // ELSE IF chyba a importoval se graf, zrusit pohled
    else
    {
        if (Categ == CATEG_GRAPH)
        {
            tobjnum objnum;
            if (!cd_Find_object(m_cdp, ObjName, CATEG_VIEW, &objnum))
                cd_Delete(m_cdp, OBJ_TABLENUM, objnum);
        }
    }
    if (statfunc)
        return statfunc(Stat, Msg, ObjName, Categ, m_Single, param);
    return Stat == 0;
}
//
// Exportuje mnozinu objektu podle popisu v t_expobjctx, vraci pocet exportovanych objektu
// 
CFNC DllKernel unsigned WINAPI Export_objects(cdp_t cdp, const t_expobjctx *ExpCtx)
{
    // IF destination file specified, create file export context
    if (ExpCtx->file_name)
    {
        CFileExpObj ctx(cdp, ExpCtx);
        return ctx.Export();
    }
    // ELSE create common export context
    else
    {
        CExpObjCtx ctx(cdp, ExpCtx);
        return ctx.Export();
    }
}
//
// Constructor, for common object export
//
CExpObjCtx::CExpObjCtx(cdp_t cdp, const t_expobjctx *ExpCtx) : m_ol(cdp, ExpCtx->schema)
{
    memcpy((t_expobjctx *)this, ExpCtx, sizeof(t_expobjctx));
    m_cdp      = cdp;
    m_Single   = count == 1;
    m_Exported = 0;
}
//
// Exports set of objects, returns count of objects realy exported
//
unsigned CExpObjCtx::Export()
{
    char   ObjName[256];
    CCateg Categ;

    // IF single object export
    if (obj_name && *obj_name)
    {
        strcpy(ObjName, obj_name);
        Export(ObjName, categ);
    }
    else
    {
        // WHILE is next object to export
        while (Next(ObjName, &Categ))
        {
            ObjName[OBJ_NAME_LEN] = 0;
            // IF export failed, break
            if (!Export(ObjName, Categ))
                break;
        }
    }
    return(m_Exported);
}
//
// Exports one object or folder, returns false for break export process
//
bool CExpObjCtx::Export(const char *ObjName, CCateg Categ)
{
    // IF object is folder, export all objects in folder
    if (Categ.m_Categ == CATEG_FOLDER || Categ == NONECATEG)
    {
        if (!ExportFolder(ObjName))
            return(false);
    }
    // IF all objects of given category in folder
    else if (Categ.m_Opts & CO_ALLINFOLDER)
    {
        if (!ExportCateg(ObjName, Categ))
            return(false);
    }
    // ELSE simple object
    else
    {
        if (!ExportObject(ObjName, Categ & ~ALL_CATEG_OBJS))
            return(false);
    }
    return(true);
}
//
// Exports one simple object, returns false for break export process
//
bool CExpObjCtx::ExportObject(const char *ObjName, tcateg Categ)
{
    // Find object
    CObjectList ol(m_cdp);
    if (!ol.Find(ObjName, Categ))  // must find link object
        return(CallState(OBJECT_NOT_FOUND, NULL, ObjName, Categ));
    // IF object is encrypted, error
    tobjnum lobjnum = ol.GetLinkOrObjnum();
    if (!not_encrypted_object(m_cdp, Categ, lobjnum))
    {
        wchar_t Fmt[80];
        return(CallState((unsigned)ENCRYPTED_NO_EXPORT, Get_transl(m_cdp, EncNoExportMsg, Fmt), ObjName, Categ));
    }
    if (!ExportObject(ObjName, Categ, lobjnum))
        return(false);
    // IF object is view, try export associated graph
    if (Categ == CATEG_VIEW)
    {
        if (ol.Find(ObjName, CATEG_GRAPH))
            if (!ExportObject(ObjName, CATEG_GRAPH, ol.GetObjnum()))
                return(false);
    }
    return(true);
}
//
// Exports one simple object, returns false for break export process
//
bool CExpObjCtx::ExportObject(const char *ObjName, tcateg Categ, tobjnum ObjNum)
{
    // Get object definition
    t_varcol_size Size;
    Size = GetDefin(ObjNum, Categ, flags & EOF_WITHDESCRIPTOR);
    if (Size == WBF_INVALIDSIZE)
        return(CallState(cd_Sz_error(m_cdp), NULL, ObjName, Categ));
    // Open destination storage
    tiocresult Res = Open(ObjName, Categ, Size);
    // IF not rewrite destination storage, skip object export
    if (Res == iocNo)
        return(true);
    // IF cancel export, break export process
    if (Res == iocCancel)
        return(false);
    // Write object definition to storage
    if (!Write(m_Buf, Size))
        return(false);
    m_Exported++;
    return(State(0, NULL, ObjName, Categ));    
}

typedef char *(WINAPI *LPGET_XML_FORM)(cdp_t cdp, tobjnum objnum, BOOL translate, const char * url_prefix);

LPGET_XML_FORM pget_xml_form;
//
// Vraci definici nebo cast definice objektu, pripadne spolu s object descriptorem, bez 0 na konci stringu
// Hodnotou je pocet skutecne nactenych bytu 
// 
t_varcol_size CExpObjCtx::GetDefin(tobjnum ObjNum, tcateg Categ, bool WithDescr)
{
    t_varcol_size ds = 0;
    t_varcol_size Size;

#ifdef WINS  // difficult to say if this is necessary, the form contains the parameters which were stored last
    if (Categ == CATEG_XMLFORM)
    {
        if (!pget_xml_form)
        {
            char Name[16];
            strcpy(Name, XML_EXT_NAME);
            strcat(Name, ".dll");
            HINSTANCE hInst = LoadLibrary(Name);
            if (hInst)
                pget_xml_form = (LPGET_XML_FORM)GetProcAddress(hInst, "get_xml_form");
        }
        if (pget_xml_form)
        {
            char *def = pget_xml_form(m_cdp, ObjNum, true, NULL);
            if (!def) 
                return WBF_INVALIDSIZE;
            Size = (int)strlen(def);
            if (!m_Buf.Alloc(Size + Size / 2))
            {
                client_error(m_cdp, OUT_OF_APPL_MEMORY);
                return WBF_INVALIDSIZE;
            }
            memcpy(m_Buf, def, Size);
            corefree(def);
            return Size;
        }
    }
#endif

    // Read object definition length
    if (cd_Read_len(m_cdp, CATEG2SYSTAB(Categ), ObjNum, OBJ_DEF_ATR, NOINDEX, &Size))
        return WBF_INVALIDSIZE;
    // Allocate buffer
    CBuffer Buf;
    if (!Buf.Alloc(Size + 1))
    {
        client_error(m_cdp, OUT_OF_APPL_MEMORY);
        return WBF_INVALIDSIZE;
    }
    // Read object definition
    if (cd_Read_var(m_cdp, CATEG2SYSTAB(Categ), ObjNum, OBJ_DEF_ATR, NOINDEX, 0, Size, Buf, &Size))
        return WBF_INVALIDSIZE;
    Buf[(int)Size] = 0;
    // IF export with object descriptor, increment size by descripror size 
    if (WithDescr && CCategList::HasFolders(Categ))
        Size += OBJECT_DESCRIPTOR_SIZE + 1;
    // Reallocate destination buffer
    if (!m_Buf.Alloc(Size + Size / 2))
    {
        client_error(m_cdp, OUT_OF_APPL_MEMORY);
        return WBF_INVALIDSIZE;
    }
    // IF export with object descriptor, write descriptor to destination buffer
    if (WithDescr && CCategList::HasFolders(Categ))
        ds = make_object_descriptor(m_cdp, Categ, ObjNum, m_Buf);
    // IF object has text definition, convert object definition to UTF8
    if (IsTextDefin(Categ))
    {
        ToUTF8(m_cdp, Buf, (char *)m_Buf + ds, Size - ds);
        return (int)strlen(m_Buf);
    }
    // ELSE copy object definition to destination buffer
    else
    {
        memcpy(m_Buf, Buf, Size);
        return Size;
    }
}
//
// Exports all objects of given category from given folder, returns false for break export process
//
bool CExpObjCtx::ExportCateg(const char *FolderName, CCateg Categ)
{
    bool Found;
    // FOR each object of given category in given folder, export object
    for (Found = m_ol.FindFirst(Categ, FolderName); Found; Found = m_ol.FindNext())
    {
        if (!ExportObject(m_ol.GetName(), Categ))
            return(false);
    }
    // FOR each subfolder in given folder, export all objects of given category
    CObjectList fl(m_cdp, schema);
    for (Found = fl.FindFirst(CATEG_FOLDER, FolderName); Found; Found = fl.FindNext())
    {
        if (!ExportCateg(fl.GetName(), Categ))
            return(false);
    }
    return(true);
}
//
// Exports all objects from given folder, returns false for break export process
//
bool CExpObjCtx::ExportFolder(const char *Folder)
{
    // FOR each object category, export objects from given folder
    for (tcateg Categ = 0; Categ < CATEG_COUNT; Categ++)
    {
        if (CCategList::Exportable(Categ))
            if (!ExportCateg(Folder, Categ))
                return(false);
    }
    return(true);
}
//
// Builds error message and calls State function
//
bool CExpObjCtx::CallState(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ)
{
    wchar_t str[256];
    if (!Stat)
        return(true);
    else if (!Msg)
    {
        Get_error_num_textW(m_cdp, Stat, str, sizeof(str) / sizeof(wchar_t));
        Msg = str;
    }
    return(State(Stat, Msg, ObjName, Categ));
}
//
// Open virtual function implementation
//
tiocresult CFileExpObj::Open(const char *ObjName, tcateg Categ, unsigned Size)
{
    char  fName[MAX_PATH];
    const char *Suffix;
#if 0
    int      PictType;
#endif

    strcpy(fName, file_name);
    // IF object set export or file_name is directory name
    if (!m_Single || IsDirectory(fName))
    {
#if 0
        if (Categ == CATEG_PICT)
        {
            tobjnum objnum;
            if (Categ == CATEG_PICT)
            {
                if (cd_Find_prefixed_object(m_cdp, schema, ObjName, CATEG_PICT, &objnum))
                    return(CallState(cd_Sz_error(m_cdp), NULL, ObjName, CATEG_PICT) ? iocNo : iocCancel);
            }
            PictType = GetPictureType(OBJ_TABLENUM, objnum, OBJ_DEF_ATR, NOINDEX);
            Suffix   = GetPictSuffix(PictType);
        }
        else
#endif
            // Get file suffix
            Suffix = GetSuffix(Categ);
        // Append path separator to directory name
        int sz = (int)strlen(fName);
        if (fName[sz - 1] != PATH_SEPARATOR)
        {
            fName[sz++] = PATH_SEPARATOR;
            fName[sz]   = 0;
        }
        // Append object name and suffix
        object_fname(m_cdp, fName + sz, ObjName, Suffix);
    }
    // IF file allready exists and it is an error
    if (m_FailIfExists && FileExists(fName))
    {
        // IF custom warning function specified
        if (warnfunc)
        {
            // Ask user how to continue
            tiocresult Res = warnfunc(fName, ObjName, Categ, m_Single, param);
            // IF Cancel whole export or not rewrite file, skip object export
            if (Res == iocCancel || Res == iocNo)
                return Res;
            // IF rewrite all next files, reset m_FailIfExists flag
            if (Res == iocYesToAll)
                m_FailIfExists = false;
        }
        // ELSE Show error message and cancel export
        else
        {
            CWBuffer Msg;
            wchar_t fmt[64];
            wchar_t fn[MAX_PATH];
            Msg.Format(Get_transl(m_cdp, FileExistsMsg, fmt), ToUnicode(fName, fn, sizeof(fn) / sizeof(wchar_t)));
            State(OS_FILE_ERROR, Msg, ObjName, Categ);
            return iocCancel;
        }
    }
    // Open destination file
    if (!m_File.Create(fName, true))
    {
        wchar_t Msg[256];
        return(State(OS_FILE_ERROR, GetErrMsg(m_cdp, OS_FILE_ERROR, Msg), ObjName, Categ) ? iocNo : iocCancel);
    }
    return(iocYes);
}
//
// Write virtual function implementation
//
bool CFileExpObj::Write(const char *Defin, unsigned Size)
{
    return(m_File.Write(Defin, Size) != WBF_INVALIDSIZE);
}
//
// State virtual function implementation
//
bool CFileExpObj::State(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ)
{
    if (statfunc)
        return(statfunc(Stat, Msg, ObjName, Categ, m_Single, param));
    return(Stat == 0);
}
//
// Exports schema, returns TRUE on success
//
CFNC DllKernel BOOL WINAPI Export_appl_param(cdp_t cdp, const t_export_param *epIn)
{ 
    CExpSchemaCtx Ctx(cdp, epIn);
    return Ctx.Export();
}
//
// Constructor
//
CExpSchemaCtx::CExpSchemaCtx(cdp_t cdp, const t_export_param *ep) : C602SQLApi(cdp), m_SchemaObjs(cdp)
{
    uns32 size = ep->cbSize;
    if (size > sizeof(t_export_param))
        size = sizeof(t_export_param);
    schema_name = NULL;
    overwrite   = false;
    memcpy((t_export_param *)this, ep, size);

#ifdef WINS
    if (!hParent)
        hParent = GetActiveWindow();
#endif
    if (schema_name == NULL || *schema_name == 0)
    {
        strcpy(m_CurSchema, cdp->sel_appl_name);
        schema_name = m_CurSchema;
    }
    m_AskRewrite = true;
    m_OK         = true;
}
//
// Exports schema, returns true on success
//
bool CExpSchemaCtx::Export()
{
    wchar_t Msg[300];
    CBuffer  Buf;
    tobjname OldSchemaName;
    strcpy(OldSchemaName, m_cdp->sel_appl_name);
    try
    {
        // refresh cached object numbers (if a table has been dropped and re-created recently, problem with exporting its data):
        Set_application(NULLSTRING);
        Set_application_ex(schema_name, TRUE);
 
        // APL-name & path
        if (!file_name || !*file_name)
        {
            client_error(m_cdp, ERROR_IN_FUNCTION_CALL);
            throw new C602SQLException(m_cdp);                    
        }

        // Fail if destination file exists and it is an error
        if (!overwrite && FileExists(file_name))
        {
            client_error(m_cdp, OS_FILE_ERROR);
            wchar_t wName[MAX_PATH];
            throw new CWBException(Get_transl(m_cdp, FileExistsMsg, Msg), ToUnicode(file_name, wName, sizeof(wName) / sizeof(wchar_t)));
        }

        // Get exported files path
        strcpy(m_ExportPath, file_name);
        char *p = strrchr(m_ExportPath, PATH_SEPARATOR);
        if (p)
            p[1] = 0;
        else
            *m_ExportPath = 0;

        // Export date and time is now
        m_ExportDT.Now();

        char SchemaUUID[2*UUID_SIZE + 1];
        // the APL file
        m_AplFile.Create(file_name, true);
        m_AplFile.Write("Application `", 13);
        m_AplFile.Write(ToUTF8(m_cdp, schema_name, Buf));
        m_AplFile.Write("` X ", 4);
        m_AplFile.Write(SelSchemaUUID2str(m_cdp, SchemaUUID));
        // write back and front end links:
        // write @ iff long_names
        m_AplFile.Write(" @ \r\n", 5);
        m_AplFile.Close();
        CWBFileTime ExportDT = GetModifTime(file_name);
        m_AplFile.Open(file_name, GENERIC_READ | GENERIC_WRITE);
        m_AplFile.Seek(0, FILE_END);

#if 0
        // recompiling programs
        if (!back_end && m_cdp->RV.global_state != PROG_RUNNING) // not OK when called from a program
            SynchronizeAllPrograms();
#endif


        // count the objects and create the progress bar:
        m_SchemaObjs.GetSchemaTables();
        int comp_counter = m_SchemaObjs.Rec_cnt();
        m_SchemaObjs.GetSchemaObjects("AND Category<>Chr(7)");
        comp_counter += m_SchemaObjs.Rec_cnt();
        ReportProgress(IMPEXP_PROGMAX, (wchar_t *)(size_t)comp_counter);
        wchar_t wName[MAX_PATH];
        ReportProgress(IMPEXP_FILENAME, ToUnicode(file_name, wName, sizeof(wName) / sizeof(wchar_t)));

        // writing objects into files
        cd_Set_progress_report_modulus(m_cdp, 10000);
  
        // phase 0: domains, phase 1: tables;  phase 2: other objects
        m_Table = OBJ_TABLENUM;
        m_SchemaObjs.GetSchemaObjects("AND Category=Chr(22)");
        ExportObjects();
        m_Table = TAB_TABLENUM;
        m_SchemaObjs.GetSchemaTables("ORDER BY Tab_name");
        ExportObjects();
        m_Table = OBJ_TABLENUM;
        m_SchemaObjs.GetSchemaObjects("AND Category<>Chr(22) AND Category<>Chr(7) ORDER BY Category, Obj_name");
        ExportObjects();

#ifdef STOP
        m_AplFile.Write("Icon    `", 9);
        m_AplFile.Write(ToUTF8(m_cdp, schema_name, Buf));
        m_AplFile.Write("`:Aplikace602\r\n", 15);
#endif
        
        // write component descriptor:
        char ApxName[MAX_PATH];
        char oName[MAX_PATH];
        strcpy(ApxName, m_ExportPath);
        strcat(ApxName, object_fname(m_cdp, oName, schema_name, ".apx"));
        CWBFileX ApxFile;
        ApxFile.Create(ApxName, true);
        apx_header apx;
        memset(&apx, 0, sizeof(apx_header));
        apx.version = CURRENT_APX_VERSION;
        Read_var(OBJ_TABLENUM, (trecnum)m_cdp->applobj, OBJ_DEF_ATR, 0, sizeof(apx_header), &apx);
        apx.appl_locked = exp_locked;
        // if front-end already redirected, preserve it, otherwise redirect to itself
        if (back_end && !*apx.front_end_name) 
            strcpy(apx.front_end_name, schema_name);
        ApxFile.Write(&apx, (int)((char *)&apx.amo - (char *)&apx));
        if (apx.admin_mode_count)
        { 
            CBuffer amos;
            amos.AllocX(sizeof(t_amo) * apx.admin_mode_count);
            Read_var(OBJ_TABLENUM, (trecnum)m_cdp->applobj, OBJ_DEF_ATR, (int)((char *)&apx.amo - (char *)&apx), sizeof(t_amo) * apx.admin_mode_count, amos);
            ApxFile.Write(amos, sizeof(t_amo) * apx.admin_mode_count);
        }
        if (m_OK)
        {
            ApxFile.Close();
            m_AplFile.Close();
            ReportProgress(IMPEXP_STATUS, Get_transl(m_cdp, SchemaExportOK, Msg));
            if (wb_stricmp(m_cdp, OldSchemaName, m_cdp->sel_appl_name))
                cd_Set_application(m_cdp, OldSchemaName);
            cd_Set_progress_report_modulus(m_cdp, 64);
            return true;
        }
    }
    catch (CWBException *e)
    {
        ReportProgress(IMPEXP_ERROR, e->GetMsg());
        delete e;
    }
    catch (...)
    {
    }
    // ulozit kontext chyby
    int rn = m_cdp->RV.report_number;
    wbbool ce = m_cdp->last_error_is_from_client;
    tobjname on;
    strcpy(on, m_cdp->errmsg);
    ReportProgress(IMPEXP_STATUS, Get_transl(m_cdp, SchemaExportFail, Msg));
    if (wb_stricmp(m_cdp, OldSchemaName, m_cdp->sel_appl_name))
        cd_Set_application(m_cdp, OldSchemaName);
    cd_Set_progress_report_modulus(m_cdp, 64);
    // obnovit kontext chyby
    m_cdp->RV.report_number = rn;
    m_cdp->last_error_is_from_client = ce;
    strcpy(m_cdp->errmsg, on);
    return false;
}

void CExpSchemaCtx::ExportObjects()
{
    CBuffer Buf;
    char UTF8Name[2 * (OBJ_NAME_LEN + 1)];
    int Count = m_SchemaObjs.Rec_cnt();
    // Get source table (TABTAB/OBJTAB) column count
    int table_column_count;
    const d_table * td = cd_get_table_d(m_cdp, m_Table, CATEG_TABLE);
    if (td)
    { table_column_count=td->attrcnt;
      release_table_d(td);
    }
    else
      return;

    // FOR each exported object
    for (trecnum rec = 0; rec < Count; rec++)
    { 
        tptr def = NULL;
        try
        {
            tobjname objname;
            tobjnum  objnum;
            tcateg   Categ;
            uns32    object_stamp;
            uns16    flags;

            // Report next object export starts
            ReportProgress(IMPEXP_PROGSTEP, NULL);
                                             
            // Read exported object name, category and flags
            m_SchemaObjs.Read(rec, APLOBJ_NAME,  objname);
            m_SchemaObjs.Read(rec, APLOBJ_CATEG, &Categ);
            m_SchemaObjs.Read(rec, APLOBJ_FLAGS, &flags);

            Categ &= CATEG_MASK;
            if (back_end && !CCategList::IsBackendCateg(Categ))
                continue;
            if (Categ == CATEG_ROLE && *objname >= '0' && *objname <= '9')
                continue;
            if (!*CCategList::ShortName(Categ))
                continue;  // this category is not supported in this version of the server
            // Read object modif timestamp
            objnum = Find_objectX(objname, Categ | IS_LINK);
            Read(m_Table, objnum, OBJ_MODIF_ATR, &object_stamp);
            // IF object modif timestamp < date_limit, skip object export
            if (date_limit && Categ != CATEG_ROLE && object_stamp < date_limit)
                continue;
            // IF object has CO_FLAG_NOEXPORT flag set, skip object export
            if (Categ == CATEG_PGMEXE) 
                flags = get_object_flags(m_cdp, CATEG_PGMSRC, objname);
            if (flags & CO_FLAG_NOEXPORT && !(with_data & 0x80))
                continue;
            // Report next exported object name
            wchar_t wObj[OBJ_NAME_LEN + 1];
            ReportProgress(Categ, ToUnicode(m_cdp, objname, wObj, sizeof(wObj) / sizeof(wchar_t)));
            bool is_a_link = flags & CO_FLAG_LINK || (Categ == CATEG_TABLE && flags & CO_FLAG_ODBC);
            // export object definition:
            if (flags & CO_FLAG_ENCRYPTED)
            {
                if (!exp_locked || !(flags & CO_FLAG_PROTECT)) // object manually un-protected
                {
                    client_error(m_cdp, ENCRYPTED_NO_EXPORT);
                    throw new C602SQLException(m_cdp);
                }
            }
            if (Categ == CATEG_ROLE)
            {
                m_AplFile.Write("Role    `", 9);
                m_AplFile.Write(ToUTF8(m_cdp, objname, Buf));
                m_AplFile.Write("`",         1);
                if (with_usergrp)
                { 
                    m_AplFile.Write(" (",      2);
                    write_role_def(objnum);
                    m_AplFile.Write(")",       1);
                }
            }
            else
            {
                // check the file:
                char    fName[MAX_PATH];
                wchar_t wName[MAX_PATH];
                bool DefaultName = GetObjectFileName(objname, CCategList::FileType(Categ), fName);
                ReportProgress(IMPEXP_FILENAME, ToUnicode(fName, wName, sizeof(wName) / sizeof(wchar_t)));
                CWBFileX ObjFile;
                ObjFile.Create(fName, true);
                bool is_encrypted = exp_locked && CCategList::Encryptable(Categ) && (flags & CO_FLAG_PROTECT);
                
                // write folder name and last modification date:
                if (CCategList::HasFolders(Categ))
                {
                    Buf.AllocX(14 + 2 * OBJ_NAME_LEN + 2);
                    int Len = make_object_descriptor(m_cdp, Categ, objnum, Buf);
                    ObjFile.Write(Buf, Len);
                }
                // get the object definition:
                def = Load_objdef(objnum, Categ);
                int Len   = (int)strlen(def);
                if (Categ == CATEG_SEQ && !(with_data & 1))
                {
                    tptr p = def;
                    if (*p == '{')
                    {
                        p++;
                        while (*p && *p != '}') 
                            *(p++) = ' '; 
                    }
                }
                const char *Defin = def;
                if (IsTextDefin(Categ)) //, def))
                {
                    int sz = Len + Len / 2;
                    Buf.AllocX(sz);
                    Defin = ToUTF8(m_cdp, def, Buf, sz);
                    Len   = (int)strlen(Defin);
                }
                ToUTF8(m_cdp, objname, UTF8Name, sizeof(UTF8Name));
                if (is_encrypted)
                    enc_buf(m_cdp->sel_appl_uuid, UTF8Name, (char *)Defin, Len);
                ObjFile.Write(Defin, Len);
                ObjFile.Close();
                corefree(def);
                def = NULL;

                // set file modif time as stored in the database:
                if (object_stamp && object_stamp != (uns32)-1)  // stamp specified (missing in old appls)
                {
                    CWBFileTime dt(object_stamp);
                    ObjFile.Close();
                    SetFileTimes(fName, m_ExportDT, dt);
                }
                // create the record in APL file
                if (Categ != CATEG_PGMEXE)
                {
                    if (Categ == m_cdp->appl_starter_categ && !wb_stricmp(m_cdp, m_cdp->appl_starter, objname)) // obsolete info, not used on import
                        m_AplFile.Write("*", 1);
                    if (is_a_link)
                        m_AplFile.Write("+", 1);
                    if (is_encrypted)
                        m_AplFile.Write(".", 1);
                    const char *CatName = CCategList::ShortName(Categ);
                    m_AplFile.Write(CatName, (int)strlen(CatName));
                    char buf[14];
                    int2str(flags, buf);
                    strcat(buf, " ");
                    m_AplFile.Write(buf,   (int)strlen(buf));
                    m_AplFile.Write("`",   1);
                    m_AplFile.Write(UTF8Name);
                    m_AplFile.Write("`",   1);
                    if (!DefaultName)   // file renamed, write its name:
                    { 
                        char *p = strrchr(fName, PATH_SEPARATOR);
                        if (p)
                            p++;
                        else
                            p = fName;
                        m_AplFile.Write("   `", 4);
                        m_AplFile.Write(p);
                        m_AplFile.Write("`", 1);
                    }
                    if (with_usergrp || with_role_privils)
                    { 
                        m_AplFile.Write(" (",    2);
                        write_obj_privils(FALSE, m_Table, objnum, with_usergrp, table_column_count);
                        m_AplFile.Write(")",     1);
                    }
                }
            }
            if (Categ != CATEG_PGMEXE)
                m_AplFile.Write("\r\n", 2);
            // write table data, if required and table is not a link
            if (m_Table == TAB_TABLENUM && !is_a_link)
            {   int column_count;
                const d_table * td = cd_get_table_d(m_cdp, objnum, CATEG_TABLE);
                if (td)
                { column_count=td->attrcnt;
                  release_table_d(td);
                }
                else  // exception would be better
                  continue;  
                if ((with_data & 1) && (!(flags & CO_FLAG_NOEXPORTD) || (with_data & 0x80)))
                { 

                    char fName[MAX_PATH];
                    wchar_t wName[MAX_PATH];
                    bool DefaultName = GetObjectFileName(objname, ".tdt", fName);
                    ReportProgress(IMPEXP_FILENAME, ToUnicode(fName, wName, sizeof(fName) / sizeof(wchar_t)));
                    // must find the real table number (not a lnk number)// 10 = table, dircurs
                    ::Move_data(m_cdp, NOOBJECT, NULL, objnum, fName, 10, IMPEXP_FORMAT_WINBASE, -1, 0, 2); // asked for rewrite when checking the file name, do not ask inside again
                    // do not break export on errors
                    cd_Uninst_table(m_cdp, objnum);
                    // Store in APL file only if table def. exported:
                    m_AplFile.Write("Data    `", 9);
                    m_AplFile.Write(ToUTF8(m_cdp, objname, Buf));
                    m_AplFile.Write("`", 1);
                    if (!DefaultName)   /* file renamed, write its name: */
                    { 
                        char *p = strrchr(fName, PATH_SEPARATOR);
                        m_AplFile.Write("   `", 4);
                        m_AplFile.Write(p);
                        m_AplFile.Write("`", 1);
                    }
                    if (with_usergrp || with_role_privils)
                    { 
                        m_AplFile.Write(" (",      2);
                        write_obj_privils(TRUE, m_Table, objnum, with_usergrp, column_count);
                        m_AplFile.Write(")",       1);
                    }
                    m_AplFile.Write("\r\n", 2);
                }
                // if data not exported, write privils only
                else if (with_usergrp || with_role_privils)
                {
                    // only if table exported
                    m_AplFile.Write("DataPrv `", 9);
                    m_AplFile.Write(ToUTF8(m_cdp, objname, Buf));
                    m_AplFile.Write("` (",       3);
                    write_obj_privils(TRUE, m_Table, objnum, with_usergrp, column_count);
                    m_AplFile.Write(")\r\n",     3);
                }
            } 
        }
        catch (CWBException *e)
        {
            m_OK = false;
            ReportProgress(IMPEXP_ERROR, e->GetMsg());
            delete e;
        }
        catch (...)
        {
            m_OK = false;
            wchar_t Msg[64];
            ReportProgress(IMPEXP_ERROR, Get_transl(m_cdp, gettext_noopL("Failed to export the object"), Msg));
        }
        if (def)
        {
            corefree(def);
            def = NULL;
        }
    }
}
//
// Builds object file name
//
bool CExpSchemaCtx::GetObjectFileName(const char *ObjName, const char *Suffix, char *Dst)
{
    char aObjName[OBJ_NAME_LEN + 32];
    strcpy(aObjName, ObjName);
    // Remove diacritics from object name
    ToASCII(aObjName, m_cdp->sys_charset);
    char *pi = aObjName + strlen(aObjName);
    for (int i = 0; ; i++)
    {
        // Build file name as m_ExportPath + aObjName + Suffix
        strcpy(Dst, m_ExportPath);
        strcat(Dst, aObjName);
        strcat(Dst, Suffix);
        if (!FileExists(Dst))
        {
            break;
        }
        // IF file with this file name exists and not belongs to this export
        CWBFileTime AccessDT = GetModifTime(Dst);
        if (AccessDT < m_ExportDT)
        {
            // IF existing file is error, break export
            if (!overwrite)
            {
                wchar_t Buf[300];
                wchar_t wName[MAX_PATH];
                client_error(m_cdp, OS_FILE_ERROR);
                throw new CWBException(Get_transl(m_cdp, FileExistsMsg, Buf), ToUnicode(Dst, wName, sizeof(wName) / sizeof(wchar_t)));
            }
            break;
        }
        // IF existing file belongs to this export, append number to file name
        itoa(i, pi, 10);
    }
    return strcmp(ObjName, aObjName) == 0;
}

#if 0

void SynchronizeAllPrograms()
{ 
    tobjname objname;
    tobjnum  codeobj;

    GetSchemaObjects("AND Category=Chr(4)");
    for (trecnum rec = 0; rec < Rec_cnt();  rec++)
    { 
        Read(rec, APLOBJ_NAME, objname);
        // compiles includes not starting with INCLUDE...
        if (!is_include(rec))
        {
            int flags = get_object_flags(m_cdp, CATEG_PGMSRC, objname);
            if (!(flags & (CO_FLAG_NOEXPORT | CO_FLAG_TRANSPORT | CO_FLAG_XML | CO_FLAG_INCLUDE)))
                synchronize(m_cdp, objname, NOOBJECT, &codeobj, FALSE, FALSE, FALSE);
        }
    }
}

bool CExpSchemaCtx::is_include(trecnum rec)
{   
    char buf[256];
    rec = m_SchemaObjs.Translate(rec);
    uns32 rdsize = Read_var(OBJ_TABLENUM, rec, OBJ_DEF_ATR, 0, sizeof(buf) - 1, buf);
    buf[rdsize]  = 0;
    uns32 flags;
    Read(OBJ_TABLENUM, rec, OBJ_FLAGS_ATR, &flags);
    if (flags & CO_FLAG_ENCRYPTED)
        enc_buffer_total(m_cdp, buf, rdsize, rec);
    // look for INCLUDE:
    char *p = buf;  
    while ((*p==' ') || (*p=='\r') || (*p=='\n')) p++;
    Upcase(p);
    return !memcmp(p, "INCLUDE", 7);
}

#endif
//
// Writes role definition to APL file
//
void CExpSchemaCtx::write_role_def(tobjnum robjnum)
{ 
    trecnum urec, ucnt;  
    tcateg  cat;
    WBUUID  uuid;
    uns8    del;   

    ucnt = Rec_cnt(USER_TABLENUM);
    for (urec = 0;  urec < ucnt; urec++)
    {
        Read(USER_TABLENUM, urec, DEL_ATTR_NUM, &del);
        if (del)
            continue;
        if (!Get_group_role((tobjnum)urec, robjnum, CATEG_ROLE))
            continue;
        Read(USER_TABLENUM, urec, OBJ_CATEG_ATR, &cat);
        Read(USER_TABLENUM, urec, USER_ATR_UUID, uuid);
        m_AplFile.Write(cat == CATEG_USER ? "U " : "G ", 2);
        char hex_buf[2 * UUID_SIZE];
        bin2hex(hex_buf, uuid, UUID_SIZE);
        m_AplFile.Write(hex_buf, 2 * UUID_SIZE);
        m_AplFile.Write(" ", 1);
    }
}

#define TAB_DRIGHT_ATR     10   // copy!!!
//
// Weites object privileges to APL file
//
void CExpSchemaCtx::write_obj_privils(BOOL datapriv, ttablenum tab, tobjnum objnum, BOOL with_usergrp, int column_count)
{ 
    CBuffer Buf;
    char    hex_buf[2*(1+64)];
    tattrib atr = datapriv ? TAB_DRIGHT_ATR : SYSTAB_PRIVILS_ATR;
    uns8    role_uuid_fill[UUID_SIZE - sizeof(tobjnum)];
    memset(role_uuid_fill, 0xff, UUID_SIZE - sizeof(tobjnum));
    uns32 len = Read_len(tab, objnum, atr);
    CBuffer priv_def;
    priv_def.AllocX(len);
    len = Read_var(tab, objnum, atr, 0, len, priv_def);
    if (!len)
        return;
    unsigned descr_size = SIZE_OF_PRIVIL_DESCR(column_count);   // no more *(uns8 *)priv_def;
    unsigned step = UUID_SIZE + descr_size;   
    for (unsigned pos = 1; pos + step <= len; pos += step)
    {
        // write the subject identification:
        if (!memcmp(role_uuid_fill, (uns8 *)priv_def + pos + sizeof(tobjnum), UUID_SIZE - sizeof(tobjnum)))
        {
            tobjnum roleobj = *(tobjnum*)((uns8 *)priv_def + pos); 
            tobjname name;
            Read(OBJ_TABLENUM, roleobj, OBJ_NAME_ATR, name);
            if (*name >= '0' && *name <= '9')
                continue;
            m_AplFile.Write("R `", 3);
            m_AplFile.Write(ToUTF8(m_cdp, name, Buf));
            m_AplFile.Write("` ",  2);
        }
        else if (with_usergrp) // user or group
        {
            tobjnum userobj;
            tcateg  cat;
            if (cd_Find_object_by_id(m_cdp, (uns8 *)priv_def + pos, CATEG_USER, &userobj))
                continue;
            Read(USER_TABLENUM, userobj, OBJ_CATEG_ATR, &cat);
            m_AplFile.Write(cat == CATEG_USER ? "U " : "G ", 2);
            bin2hex(hex_buf, (uns8 *)priv_def + pos, UUID_SIZE);
            m_AplFile.Write(hex_buf, 2 * UUID_SIZE);
            m_AplFile.Write(" ",     1);
        }
        else
            continue;
        // write the privileges:
        uns8 *privils = (uns8 *)priv_def + pos + UUID_SIZE;
        if (datapriv)
        { int rest = descr_size;
          do
          { int wrstep = rest;
            if (wrstep > sizeof(hex_buf)/2) wrstep = sizeof(hex_buf)/2;
            bin2hex(hex_buf, privils, wrstep);
            m_AplFile.Write(hex_buf, 2 * wrstep);
            rest -= wrstep;  privils += wrstep;
            if (!rest) break;
            m_AplFile.Write(" ",     1);
          } while (true);  
        }
        else
        {
            *privils &= ~(RIGHT_READ | RIGHT_WRITE);
            if (HAS_READ_PRIVIL(privils,  OBJ_DEF_ATR))
                *privils |= RIGHT_READ;
            if (HAS_WRITE_PRIVIL(privils, OBJ_DEF_ATR))
                *privils |= RIGHT_WRITE;
            bin2hex(hex_buf, privils, 1);
            m_AplFile.Write(hex_buf, 2);
        }
        m_AplFile.Write(" ", 1);
    }
}
//
// Reports schema export progress
//
void CExpSchemaCtx::ReportProgress(int Categ, const wchar_t * Value)
{
    if (report_progress && callback)
    {
        char Msg[400];
        const void *Val;
        if (Categ == IMPEXP_PROGMAX || Categ == IMPEXP_PROGSTEP)
            Val = Value;
        else
        {
            FromUnicode(Value, Msg, sizeof(Msg));
            Val = Msg;
        }
        callback(Categ, Val, param);
    }
}
//
// Imports schema, returns TRUE on success
//
CFNC DllKernel BOOL WINAPI Import_appl_param(cdp_t cdp, const t_import_param *ip)
{ 
    CImpSchemaCtx Ctx(cdp, ip);
    return !Ctx.Import();
}
//
// Constructor
//
CImpSchemaCtx::CImpSchemaCtx(cdp_t cdp, const t_import_param *ip) : C602SQLApi(cdp)
{
    uns32 size = ip->cbSize;
    if (size > sizeof(t_import_param))
        size = sizeof(t_import_param);
    memcpy((t_export_param *)this, ip, size);

    m_bufpos     = 0;
    m_SrcCharSet = 1;
    m_some_subjects_not_found = false;
}

void CImpSchemaCtx::delayed_removing_importers_global_data_privils(trecnum & tabnum)
{ 
    if (tabnum != NORECNUM)
    { 
        const d_table * td = cd_get_table_d(m_cdp, tabnum, CATEG_TABLE);
        if (td)
        {
            t_privils_flex priv_val(td->attrcnt);
            priv_val.clear();
            if (cd_GetSet_privils(m_cdp, NOOBJECT, CATEG_USER, tabnum, NORECNUM, OPER_SET, priv_val.the_map()))
                ReportError();
            release_table_d(td);
        }
        tabnum = NORECNUM;
    }
}

bool CImpSchemaCtx::Import()
{
    tobjname applname, orig_applname = "";
    tobjnum aplobjnum, importer;
    unsigned header_size;
    char pathname[MAX_PATH];
    char srcpath[MAX_PATH];
    bool appl_name_exists, from_v4 = true;
    bool replacing_objects = false;
    bool appl_id_exists = false, uuid_from_file, uuid_specified = false;
    int pathlen;
    tptr defin = NULL;

    const int catarr[] =
    {
        CATEG_TABLE, CATEG_VIEW,       CATEG_MENU,     CATEG_CURSOR,  CATEG_PGMSRC,    CATEG_PICT,
        CATEG_ROLE,  CATEG_CONNECTION, CATEG_RELATION, CATEG_DRAWING, CATEG_GRAPH,     CATEG_REPLREL,
        IMPEXP_DATA, IMPEXP_DATA,      CATEG_GROUP,    IMPEXP_ICON,   IMPEXP_DATAPRIV, CATEG_APPL,
        CATEG_PROC,  CATEG_TRIGGER,    CATEG_XMLFORM,  CATEG_FOLDER,  CATEG_SEQ,       CATEG_INFO,
        CATEG_DOMAIN,CATEG_STSH,       CATEG_XMLFORM 
    };

#ifdef WINS
    // find WB program location to WB_prog_dir (termiated by '\'):
    char WB_prog_dir[MAX_PATH];
    GetModuleFileNameA(NULL, WB_prog_dir, sizeof(WB_prog_dir));
    pathlen = (int)strlen(WB_prog_dir);
    while (pathlen && WB_prog_dir[pathlen-1]!='\\' && WB_prog_dir[pathlen-1]!='/' && WB_prog_dir[pathlen-1]!=':')
        pathlen--;
    WB_prog_dir[pathlen] = 0;
#endif
 
    if (!file_name || !*file_name)
    {
        client_error(m_cdp, ERROR_IN_FUNCTION_CALL);
        ReportError();
        return false;
    }
    // find the imporing subject:
    importer = m_cdp->logged_as_user;
    // check the privilege to insert objects:
    uns8 priv_val[PRIVIL_DESCR_SIZE];
    if 
    (
        cd_GetSet_privils(m_cdp, NOOBJECT, CATEG_USER, OBJ_TABLENUM, NORECNUM, OPER_GETEFF, priv_val) || !(*priv_val & RIGHT_INSERT) ||
        cd_GetSet_privils(m_cdp, NOOBJECT, CATEG_USER, TAB_TABLENUM, NORECNUM, OPER_GETEFF, priv_val) || !(*priv_val & RIGHT_INSERT)
    )
    { 
        client_error(m_cdp, NO_RIGHT);
        ReportError();
        return false;
    }

    try
    {
        // find the APL file:
        CWBFileX AplFile;
        AplFile.Open(file_name);
        unsigned fSize = AplFile.Size();
        m_ApplBuf.Alloc(fSize + 1);
        AplFile.Read(m_ApplBuf, fSize);
        m_ApplBuf[(int)fSize] = 0;
        AplFile.Close();
        if (::IsUTF8(m_ApplBuf))
            m_SrcCharSet = 7;
        uns32 BOM = *(uns32 *)(char *)m_ApplBuf & 0xFFFFFF;
        if (BOM == UTF_8_BOM)
            m_bufpos += UTF_8_BOMSZ;
        next_char();
        next_sym();

        strcpy(srcpath, file_name);
        pathlen = (int)strlen(srcpath);
        while (pathlen && srcpath[pathlen-1] != '\\' && srcpath[pathlen - 1] != '/' && srcpath[pathlen - 1] != ':')
            pathlen--;
        srcpath[pathlen] = 0;

        // find the name for the application
        WBUUID appl_uuid, orig_appl_uuid;
        if (clasify() == SC_APPLICATION) // read name & UUID
        {
            next_sym();
            // read & check the name:
            strmaxcpy(applname, m_sym, OBJ_NAME_LEN + 1);
            strcpy(orig_applname, applname);
            next_sym();
            // read & check the UUID:

            if (!strcmp(m_sym, "X"))  // take uuid from the file
            {
                from_v4 = false;
                next_sym();
                conv_hex(m_sym, orig_appl_uuid, UUID_SIZE);
                memcpy(appl_uuid, orig_appl_uuid, UUID_SIZE);
                next_sym();
                uuid_specified = true;  // UUID specified
                if (!strcmp(m_sym, "@"))
                    next_sym();
            }
        }
        else // missing "APPLICATION..." line
        {
            fname2objname(file_name, applname);
            strcpy(orig_applname, applname);
            if (alternate_name && *alternate_name)
                strcpy(applname, alternate_name);
        }

        // check for name/UUID conflicts:
        tobjname AltName;
        aplobjnum        = Find_object(applname, CATEG_APPL);
        appl_name_exists = aplobjnum != NOOBJECT;
        if (!uuid_specified)  // means: UUID specified
            create_uuid(appl_uuid);
        else
        {
            trecnum cnt = 0;
            C602SQLQuery Query(m_cdp);
            char sUUID[2 * UUID_SIZE + 1];
            bin2hex(sUUID, orig_appl_uuid, UUID_SIZE);
            sUUID[2 * UUID_SIZE] = 0;
            Query.Open("SELECT OBJ_NAME FROM OBJTAB WHERE Apl_UUID=X'%s' AND Category=Chr(CATEG_APPL)", sUUID);
            appl_id_exists = Query.Rec_cnt() > 0;
            if (appl_id_exists)
                Query.Read(0, 1, AltName);
        }

        // resolve the conflict:
        uuid_from_file = uuid_specified;
        if (appl_name_exists || appl_id_exists)
        {
            if ((flags & (IMPAPPL_NEW_INST | IMPAPPL_REPLACE)) == 0)
            {
                client_error(m_cdp, ERROR_IN_FUNCTION_CALL);
                wchar_t wMsg[80];
                wchar_t wName[OBJ_NAME_LEN + 1];
                if (appl_name_exists)
                    throw new CWBException(Get_transl(m_cdp, SchemaNameExists, wMsg), ToUnicode(m_cdp, applname, wName, sizeof(wName) / sizeof(wchar_t)));
                else
                    throw new CWBException(Get_transl(m_cdp, SchemaUUIDExists, wMsg), ToUnicode(m_cdp, AltName, wName, sizeof(wName) / sizeof(wchar_t)));
            }
            if (flags & IMPAPPL_REPLACE) // replace objects (existing roles must be preserved because privils are related to their object numbers)
            {   
                if (appl_name_exists)
                {
                    Read(OBJ_TABLENUM, aplobjnum, APPL_ID_ATR, appl_uuid);
                }
                else
                {
                    aplobjnum = Find_object_by_id(appl_uuid, CATEG_APPL);
                    strcpy(applname, AltName);  // application to be opened
                }
                replacing_objects = true;
            }
            else // new instance
            {
                if (appl_name_exists)
                {
                    if ((flags & IMPAPPL_USE_ALT_NAME) && alternate_name && *alternate_name)
                    {
                        strcpy(applname, alternate_name);
                    }
                    else
                    {
                        do
                        {
                            if (strlen(applname) < OBJ_NAME_LEN)
                                strcat(applname, "X");
                            else
                                applname[OBJ_NAME_LEN - 1]++;
                            aplobjnum = Find_object(applname, CATEG_APPL);
                        }
                        while (aplobjnum == NOOBJECT);
                    }
                }
                uuid_from_file = false;
            }
        }

        // importing a new application: create it
        if (!replacing_objects)
        {
            // create the new application:
            aplobjnum = Insert_object(applname, CATEG_APPL); // not inserted as "limited", the definition contains data which have to be accessible
        }

#if 0
        free_project(m_cdp, TRUE);                          // closes the project of the previous application (removes project name)
#endif

        Set_application_ex(applname, TRUE);   // mapping not started here because apx_header is empty

        if (!replacing_objects && uuid_from_file)  // take uuid from the file
        {
            Write(OBJ_TABLENUM, aplobjnum, APPL_ID_ATR, appl_uuid, UUID_SIZE);
            if (m_cdp->admin_role  != NOOBJECT)
                Write(OBJ_TABLENUM, m_cdp->admin_role,  APPL_ID_ATR, appl_uuid, UUID_SIZE);
            if (m_cdp->senior_role != NOOBJECT)
                Write(OBJ_TABLENUM, m_cdp->senior_role, APPL_ID_ATR, appl_uuid, UUID_SIZE);
            if (m_cdp->junior_role != NOOBJECT)
                Write(OBJ_TABLENUM, m_cdp->junior_role, APPL_ID_ATR, appl_uuid, UUID_SIZE);
            if (m_cdp->author_role != NOOBJECT)
                Write(OBJ_TABLENUM, m_cdp->author_role, APPL_ID_ATR, appl_uuid, UUID_SIZE);
            Set_application(NULLSTRING);
            Set_application_ex(applname, TRUE);
        }

        if (replacing_objects)
            cd_Set_temporary_authoring_mode(m_cdp, TRUE);
        // add the application to the panel and select it:
        // find the data source
        strcpy(pathname, srcpath);
        strcat(pathname, ToASCII(orig_applname, m_cdp->sys_charset));
        strcat(pathname, ".apx");

        apx_header apx;
        bool apx_found   = apx_start(pathname, &apx, header_size);  // reads and updates the APX file or stores defaults to apx
        bool appl_locked = apx.appl_locked != FALSE;
        if (apx_found || !replacing_objects)  // write the APX file or the default APX, but do not overwrite old apx by defaults
        {
            apx.appl_locked = FALSE;  // temp.
            Write_var(OBJ_TABLENUM, (trecnum)m_cdp->applobj, OBJ_DEF_ATR, 0, sizeof(apx_header), &apx);
        }

#if 0   // WINS

        if (apx_found)
        {
            if (*apx.dest_dsn)
            { if (DialogBoxParam(hVeInst, MAKEINTRESOURCE(DLG_SEL_DATA_SOURCE), ip->hParent, DestDSProc,
                  (LPARAM)apx.dest_dsn))
              { t_connection * conn = connect_data_source(cdp, apx.dest_dsn, FALSE, FALSE);
                if (conn != NULL)
                { conn->selected=2;   /* mapped */
                  cdp->odbc.mapping_on=TRUE;
                  strcpy(dsn, apx.dest_dsn);
                }
              }
            }
        }
#endif
        
        wchar_t wName[MAX_PATH];
        ReportProgress(IMPEXP_PROGMAX, (wchar_t *)(size_t)(fSize / 50));
        ReportProgress(IMPEXP_FILENAME, ToUnicode(file_name, wName, sizeof(wName) / sizeof(wchar_t)));

        // disable referential integrity:
        cd_Enable_refint(m_cdp, FALSE);
        cd_Set_progress_report_modulus(m_cdp, 10000);

        // import components:
        trecnum remove_importers_global_data_privils_from_table = NORECNUM;
        char UTF8Name[2 * (OBJ_NAME_LEN + 1)];

        while (m_sym[0])
        {
            char objname[OBJ_NAME_LEN+2];  tobjnum objnum;
            tcateg categ;  bool is_starter = false, is_link = false;  uns16 exp_flags = 0;

            //SetCursor(LoadCursor(NULL, IDC_WAIT));
            // analyse item description beginning:
            if (m_sym[0] == '*')  // info from APX file used instead
            {
                is_starter = TRUE;
                next_sym();
            }
            if (m_sym[0] == '+')
            {
                is_link = true;
                next_sym();
            }
            if (m_sym[0] == '.')
            {
                exp_flags = CO_FLAG_ENCRYPTED | CO_FLAG_PROTECT;
                next_sym();
            }

            t_sckey itemtype = clasify();
            if (itemtype == SC_NO)
                throw new CWBException(Get_transl(m_cdp, AppFileDemaged, wName));
            next_sym();               // skip item type
            if (!m_sym[0])       // reports syntax error
                throw new CWBException(Get_transl(m_cdp, AppFileDemaged, wName));
            if (!m_quoted && m_sym[0] >= '0' && m_sym[0] <= '9')
            {
                sig32 val;
                if (str2int(m_sym, &val))
                    exp_flags |= val;
                next_sym();
            }
            strmaxcpy(objname,  m_sym,     OBJ_NAME_LEN + 1);
            strmaxcpy(UTF8Name, m_UTF8sym, sizeof(UTF8Name));
            next_sym();               // skip object name
            ReportProgress(catarr[itemtype - 1], ToUnicode(m_cdp, objname, wName, sizeof(wName) / sizeof(wchar_t)));
            ReportProgress(IMPEXP_PROGPOS, (wchar_t *)(size_t)(curr_pos()/50));

            // remove_importers_global_data_privils_from_table
            if (itemtype != SC_DATA && SC_DATA != SC_ASCII && SC_ASCII != SC_DATAPRV)
              delayed_removing_importers_global_data_privils(remove_importers_global_data_privils_from_table);
            // switch on item type:
            switch (itemtype)
            {
            case SC_ICONA:   // create applicatio icon
            {
                tobjname groupname = "", itemfile = "";
                if (m_sym[0] == ':')
                {
                    next_sym();
                    strmaxcpy(groupname, m_sym, OBJ_NAME_LEN + 1);
                    next_sym();
                }
                if (m_sym[0] && m_sym[0] != '(' && clasify() == SC_NO)
                {
                    strmaxcpy(itemfile, m_sym, sizeof(itemfile));
                    next_sym();
                }
                break;
            }
            
            case SC_GROUP:   // create group - obsolete
                objnum = Find_object(objname, CATEG_GROUP);
                if (objnum == NOOBJECT)  /* group not found */
                {
                    if (Create_group(objname))
                        ReportError();
                    ReportProgress(CATEG_GROUP, ToUnicode(m_cdp, objname, wName, sizeof(wName) / sizeof(wchar_t)));
                }
                break;

            case SC_DATA:    // import data into a table
            case SC_ASCII:
            {
                objnum = Find_objectX(objname, CATEG_TABLE);
                open_item_file(pathname, srcpath, objname, itemtype == SC_DATA ? ".tdt" : ".txt");
                Move_data(NOOBJECT, pathname, NOOBJECT, objname, itemtype == SC_DATA ? IMPEXP_FORMAT_WINBASE : IMPEXP_FORMAT_TEXT_COLUMNS, IMPEXP_FORMAT_TABLE_REIND, 0, -1, FALSE);
                // privileges:
                if (m_sym[0] == '(')
                {
                    define_privils(objnum, NORECNUM, importer);
                    delayed_removing_importers_global_data_privils(remove_importers_global_data_privils_from_table);
                    cd_Uninst_table(m_cdp, objnum);
                    objnum = NOOBJECT;
                }
                break;
            }
            
            case SC_DATAPRV:
                objnum = Find_objectX(objname, CATEG_TABLE);
                if (m_sym[0] == '(')
                    define_privils(objnum, NORECNUM, importer);
                delayed_removing_importers_global_data_privils(remove_importers_global_data_privils_from_table);
                cd_Uninst_table(m_cdp, objnum);
                objnum = NOOBJECT;
                break;


            case SC_ROLE:
                if (!is_link) 
                {
                    // check to see if the object already exists:
                    ReportProgress(CATEG_ROLE, ToUnicode(m_cdp, objname, wName, sizeof(wName) / sizeof(wchar_t)));
                    objnum = Find_object(objname, CATEG_ROLE);
                    if (objnum == NOOBJECT)
                    {   //if (strcmp(objname, "ADMINISTRATOR") &&
                        //    strcmp(objname, "SENIOR_USER"  ) &&
                        //    strcmp(objname, "JUNIOR_USER"  ))
                        objnum = Insert_object(objname, CATEG_ROLE);
                    }
                    // must not delete existing role, it may have privils set before in this import!
                    if (m_sym[0] == '(')
                        define_role(objnum);

                    break;
                }
                // cont.
            case SC_TABLE  :      case SC_VIEW   :      case SC_MENU   :      case SC_CURSOR : 
            case SC_PROGRAM:      case SC_PICTURE:      case SC_CONNECT:      case SC_RELAT  :
            case SC_DRAWING:      case SC_GRAPH  :      case SC_REPLREL:      case SC_PROC   :
            case SC_TRIGGER:      case SC_WWW    :      case SC_SEQUEN :      case SC_INFO   :
            case SC_FOLDER :      case SC_DOMAIN :      case SC_STSH   :
            {
                categ = catarr[itemtype - 1];
                // check to see if the object already exists:
                objnum = Find_object(objname, categ | IS_LINK);
                if (objnum != NOOBJECT)  /* object exists! */
                {
                    // delete object:
                    if (categ == CATEG_TABLE)
                    {
                        char cmd[30 + OBJ_NAME_LEN];
                        sprintf(cmd, "DROP TABLE `%s`", objname);
                        // strcat(cmd, " CASCADE");    ! -- both is bad:
                        // without CASCADE sometimes cannot delete table and replace it,
                        // with CASCADE can delete new version of the dependent table!
                        SQL_execute(cmd);
                    }
                    else if (categ == CATEG_TRIGGER)  // the trigger must be unregristered, otherwise server will report error during creation
                    {
                        char cmd[30 + OBJ_NAME_LEN];
                        sprintf(cmd, "DROP TRIGGER `%s`", objname);
                        SQL_execute(cmd);
                    }
                    else  // Fulltext: must not used DROP, the tables/sequence may have already been replaced by new versions
                        Delete(OBJ_TABLENUM, (trecnum)objnum); // deleting linking object, not the linked object
                    objnum = Find_object(objname, categ);
                    if (objnum != NOOBJECT)  // must find linked object
                        Remove_from_cache(m_cdp, categ, objname);
                        // must delete the old component because it contains an invalid object number and data import would fail
                }
                // import object:
                open_item_file(pathname, srcpath, objname, CCategList::FileType(categ));
                // analyse the prefix:
                tobjname folder_name;  uns32 stamp;  *folder_name=0;  stamp=0;
                int ds = 0;
                if (categ != CATEG_CONNECTION && categ != CATEG_PICT && categ != CATEG_GRAPH && categ != CATEG_PGMEXE)
                {
                    char buf[OBJECT_DESCRIPTOR_SIZE + OBJ_NAME_LEN + UTF_8_BOMSZ + 1];
                    if (m_ObjFile.Read(buf, OBJECT_DESCRIPTOR_SIZE + OBJ_NAME_LEN + UTF_8_BOMSZ) >= OBJECT_DESCRIPTOR_SIZE)
                    {
                        uns8 *p = (uns8 *)buf + OBJECT_DESCRIPTOR_SIZE + OBJ_NAME_LEN + UTF_8_BOMSZ;
                        while (*p > 0x80)
                            p--;
                        *p = 0;
                        ds = get_object_descriptor_data(m_cdp, buf, IsUTF8(buf), folder_name, &stamp);
                    }
                    m_ObjFile.Seek(ds);
                }
                // read the object definition
                int Size = m_ObjFile.Size() - ds;
                m_ObjBuf.AllocX(Size + 1);
                Size = m_ObjFile.Read(m_ObjBuf, Size);
                m_ObjBuf[Size] = 0;
                if (exp_flags & CO_FLAG_ENCRYPTED)
                { char * p;

                    enc_buf(orig_appl_uuid, UTF8Name, m_ObjBuf, Size);
                    for (p = m_ObjBuf; *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'; p++);
                    bool OK = true;
                    switch (categ)
                    {
                    case CATEG_TABLE:
                    case CATEG_SEQ:
                    case CATEG_INFO:
                        OK = strnicmp(p, "CREATE", 6) == 0;
                        break;
                    case CATEG_CURSOR:
                        OK = strnicmp(p, "SELECT", 6) == 0;
                        break;
                    case CATEG_PGMSRC:
                        OK = strncmp(p, "<?xml", 5) == 0 || *p == '*';
                        break;
                    case CATEG_DRAWING:
                        OK = *p == '*' || *p == '{';
                        break;
                    case CATEG_PROC:
                        OK = strnicmp(p, "PROCEDURE", 9) == 0 || strnicmp(p, "FUNCTION", 8) == 0;
                        break;
                    case CATEG_TRIGGER:
                        OK = strnicmp(p, "TRIGGER", 7) == 0;
                        break;
                    case CATEG_DOMAIN:
                        OK = strnicmp(p, "DOMAIN", 6) == 0;
                        break;
                    }
                    if (!OK)
                    {
                        m_ObjFile.Seek(ds);
                        m_ObjFile.Read(m_ObjBuf, Size);
                        m_ObjBuf[Size] = 0;
                        enc_buf(orig_appl_uuid, objname, m_ObjBuf, Size);
                    }
                }
                m_ObjFile.Close();
                if (IsTextDefin(categ)) //, m_ObjBuf))
                {
                    ToLocale(m_cdp, m_ObjBuf, m_ObjBuf, Size, IsUTF8(m_ObjBuf) ? 7 : 1);
                    Size = (int)strlen(m_ObjBuf);
                }
                // analyse encrypted descriptor from version 7:
                tptr defstart = (tptr)m_ObjBuf;
                if (exp_flags & CO_FLAG_ENCRYPTED)
                {
                    tobjname fn; uns32 st;
                    ds = get_object_descriptor_data(m_cdp, m_ObjBuf, false, fn, &st);
                    if (ds)
                    {
                        strcpy(folder_name, fn); stamp = st;
                        defstart += ds;
                        Size     -= ds; 
                    }
                }
                if (is_link)
                {
                    make_link(defstart, categ, objname, from_v4);
                    objnum = Find_objectX(objname, categ | IS_LINK);
                }
                else if (categ == CATEG_TABLE)   /* check the file structure */
                {
                    trecnum tabrec = SQL_execute(defstart);
                    objnum = tabrec;
                    inval_table_d(m_cdp, NOOBJECT, CATEG_TABLE);  /* remove defs from the cache (intertable links should be refreshed) */
                    if (exp_flags & CO_FLAG_ENCRYPTED)
                    {
                        enc_buffer_total(m_cdp, defstart, Size, objnum);
                        Write_var(TAB_TABLENUM, (trecnum)objnum, OBJ_DEF_ATR, 0, Size, defstart);
                        uns16 flags;
                        Read(TAB_TABLENUM, (trecnum)objnum, OBJ_FLAGS_ATR, &flags);
                        flags |= exp_flags;
                        Write(TAB_TABLENUM, (trecnum)objnum, OBJ_FLAGS_ATR, &flags, sizeof(uns16));
                    }
                    else // store table def. with descriptor
                        Write_var(TAB_TABLENUM, (trecnum)objnum, OBJ_DEF_ATR, 0, Size, defstart);
                    Write_len(TAB_TABLENUM, (trecnum)objnum, OBJ_DEF_ATR, Size); // CREATE TABLE may have created it longer!
                    Set_object_time_and_folder(m_cdp, objnum, CATEG_TABLE, folder_name, stamp);
                    // remove own privileges:
                    memset(priv_val, 0, PRIVIL_DESCR_SIZE);
                    Set_privils(NOOBJECT, CATEG_USER, TAB_TABLENUM, tabrec, OPER_SET, priv_val);
                    //cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, tabrec,     NORECNUM, OPER_SET, priv_val); -- must do it AFTER defining data privileges, if specified
                    remove_importers_global_data_privils_from_table = tabrec;
                }
                else
                { 
                    //if (categ!=CATEG_SEQ) -- no more
                    // this form is OK for components with errors and for MODULE_GLOBAL_DECLS
                    {
                        objnum = Insert_object_limited(objname, categ);
                        if (exp_flags & CO_FLAG_ENCRYPTED) 
                        { 
                            if (categ == CATEG_PGMSRC)
                                exp_flags |= program_flags(m_cdp, defstart);  // must add INCLUDE, TRANSPORT flags in order to prevent the compilation
                            enc_buffer_total(m_cdp, defstart, Size, objnum);
                        }
                        Write_var(OBJ_TABLENUM, (trecnum)objnum, OBJ_DEF_ATR, 0, Size, defstart);
                    }

#ifdef STOP     // current value will be reserved, if specified during export
                      else // must remove the current value from the seq. definition!
                      { uns32 res;
                        cd_SQL_execute(m_cdp, defin, &res);
                        objnum=(tobjnum)res;
                      }
#endif

                    if (categ != CATEG_CONNECTION)  // connections will be opened at the end
                    {
                        if (CCategList::HasFolders(categ))
                            Set_object_time_and_folder(m_cdp, objnum, categ, folder_name, stamp);
                    }
                    tobjnum exeobj;
                    if (categ == CATEG_PGMSRC)   /* delete exe object if source imported and replacing comps. */
                    {
                        exeobj = Find_object(objname, CATEG_PGMEXE);
                        if (exeobj != NOOBJECT)
                            Write_len(OBJ_TABLENUM, exeobj, OBJ_DEF_ATR, 0);
                    }
                    if ((exp_flags))  //-- must write, not writing later-- & CO_FLAG_ENCRYPTED) && categ == CATEG_WWW) // must define the proper flags not analysed by Add_component, must be after storing the defin
                    { 

#if 0
#ifdef WINS  // ###, should be in all versions
                        exp_flags |= lnk_get_www_object_type(m_cdp, objnum);
#endif
#endif
                        Write(OBJ_TABLENUM, (trecnum)objnum, OBJ_FLAGS_ATR, &exp_flags, sizeof(uns16));
                    }
                    if (categ == CATEG_TRIGGER || categ == CATEG_INFO)
                    {
                        if (cd_Trigger_changed(m_cdp, objnum, TRUE))  // register the new trigger
                            ReportError();
                    }
                }

                // privileges:
                if (m_sym[0] == '(')             
                    define_privils(categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, importer);
                break;
            }
            default:
                throw new CWBException(Get_transl(m_cdp, AppFileDemaged, wName));       // reports syntax error
            } // switch on itemtype
        } // cycle on APL file items
        delayed_removing_importers_global_data_privils(remove_importers_global_data_privils_from_table);

        // re-enable referential integrity:
        Set_application(NULLSTRING);
        Set_application_ex(applname, TRUE);   // this creates the list of triggers in the application
        cd_Uninst_table(m_cdp, NOOBJECT);  // must relink tables to sequences, restore refer. int. links, link triggers to tables
        cd_Procedure_changed(m_cdp);  // global_module may have been imported

#ifdef WINS
        // copy help files, if found:
        { 
            strcpy(pathname, srcpath);
            strcat(pathname, "*.HLP");
            WIN32_FIND_DATA fd;
            HANDLE hfd = FindFirstFile(pathname, &fd);
            if (hfd != INVALID_HANDLE_VALUE)
            {
                char *p = strrchr(fd.cFileName, '\\');
                if (!p)
                    p = fd.cFileName;
                copy_file(p, srcpath, WB_prog_dir);
                FindClose(hfd);
            }
            strcpy(pathname, srcpath);
            strcat(pathname, "*.CNT");
            hfd = FindFirstFile(pathname, &fd);
            if (hfd != INVALID_HANDLE_VALUE)
            {
                char *p = strrchr(fd.cFileName, '\\');
                if (!p)
                    p = fd.cFileName;
                copy_file(p, srcpath, WB_prog_dir);
                FindClose(hfd);
            }
        }
#endif

        // set the "closed" state of the application:
        if (replacing_objects)
            cd_Set_temporary_authoring_mode(m_cdp, TRUE);  // start..................
        // restore the "locked" flag in APX, appl_locked implies apx_found:
        if (appl_locked)  
        {
            if (apx_found || !replacing_objects)  // write the APX file or the default APX, but do not overwrite old apx by defaults
            { 
                apx.appl_locked = TRUE;
                Write_var(OBJ_TABLENUM, (trecnum)m_cdp->applobj, OBJ_DEF_ATR, 0, sizeof(apx_header), &apx);
            }
        }
        // read & convert the old APX file with list of relations:
        if (apx_found && apx.version < 5)
        {
            strcpy(pathname, srcpath);
            strcat(pathname, orig_applname);
            strcat(pathname, ".APX");
            apx_conversion(pathname, header_size);
        }
        //write_dest_ds(m_cdp, dsn);  /* the new (may be changed) dsn */
        if (replacing_objects)
            cd_Set_temporary_authoring_mode(m_cdp, FALSE);  // .................stop
        else  if (appl_locked) // remove itself from the Author role
        {
            uns32 relation = 0;
            Set_group_role(NOOBJECT, m_cdp->author_role, CATEG_ROLE, relation);
        }

#if 0
        if (!(flags & IMPAPPL_NO_COMPILE))
            SynchronizeAllPrograms(m_cdp);  // must compile in order to make the code accesible for all users
#endif

        if (m_some_subjects_not_found)
            ReportProgress(IMPEXP_ERROR, Get_transl(m_cdp, SomeSubjectsNotFound, wName));
        Done();
        ReportProgress(IMPEXP_PROGPOS, (wchar_t *)(size_t)(fSize / 50));
        ReportProgress(IMPEXP_STATUS, Get_transl(m_cdp, SchemaImportOK, wName));
        return(true);
    }
    catch (CWBException *e)
    {
        ReportProgress(IMPEXP_ERROR, e->GetMsg());
        delete e;
    }
    catch (...)
    {
        wchar_t wMsg[64];
        ReportProgress(IMPEXP_ERROR, Get_transl(m_cdp, gettext_noopL("Failed to import the object"), wMsg));
    }
    if (replacing_objects)
        cd_Set_temporary_authoring_mode(m_cdp, FALSE);
    Done();
    wchar_t wMsg[64];
    ReportProgress(IMPEXP_STATUS, Get_transl(m_cdp, SchemaImportFail, wMsg));
    return(false);
}

void CImpSchemaCtx::Done()
{
    cd_Enable_refint(m_cdp, TRUE);
    inval_table_d(m_cdp, NOOBJECT, CATEG_TABLE);
    cd_Set_progress_report_modulus(m_cdp, 64);
    if (CPInterface)
        CPInterface->RefreshServer(m_cdp->locid_server_name);
}
//
// Reads next symbol from schema description
//
void CImpSchemaCtx::next_sym(void)
{
    int i;
    m_quoted = false;
    while (m_cc == ' ' || m_cc == '\r' || m_cc == '\n' || m_cc == 0x1a)
        next_char();
    if (m_cc == '`')  // quoted identifier
    { 
        next_char();
        i = 0;
        m_quoted = true;
        while (m_cc != '`')
        { 
            if (!m_cc || i >= SCAN_MAX_SYM)
            {
                wchar_t wMsg[64];
                throw new CWBException(Get_transl(m_cdp, AppFileDemaged, wMsg));
            }
            m_UTF8sym[i++] = m_cc;
            next_char(); 
        }
        m_UTF8sym[i] = 0;
        strcpy(m_sym, m_UTF8sym);
        ToLocale(m_cdp, m_sym, m_sym, sizeof(m_sym), m_SrcCharSet);
        Upcase9(m_cdp, m_sym);
        next_char();  // skip terminating `
    }
    else if (m_cc >= 'A' && m_cc <= 'Z' || m_cc >= 'a' && m_cc <= 'z' || m_cc >='0' && m_cc <= '9' || m_cc == '_' || (uns8)m_cc >= 128)
    { 
        i = 0;
        do
        { 
            if (i >= SCAN_MAX_SYM)
            {
                wchar_t wMsg[64];
                throw new CWBException(Get_transl(m_cdp, AppFileDemaged, wMsg));
            }
            m_sym[i++] = m_cc;
            next_char();
        }
        while (m_cc >= 'A' && m_cc <= 'Z' || m_cc >= 'a' && m_cc <= 'z' || m_cc >= '0' && m_cc <= '9' || m_cc == '_' || (uns8)m_cc >= 128 || m_cc == '.');
        m_sym[i] = 0;
        ToLocale(m_cdp, m_sym, m_sym, sizeof(m_sym), m_SrcCharSet);
        Upcase9(m_cdp, m_sym);
    }
    else if (!m_cc)
        m_sym[0] = 0;
    else
    { 
        m_sym[0] = m_cc;
        m_sym[1] = 0;
        next_char();
    }
}

static char * sc_patt[] = 
{
    "TABLE", "VIEW",    "MENU",  "CURSOR",  "PROGRAM", "PICTURE",
    "ROLE",  "CONNECT", "RELAT", "DRAWING", "GRAPH",   "REPLREL",
    "DATA",  "ASCII",   "GROUP", "ICON",    "DATAPRV", "APPLICATION",
    "PROC",  "TRIGGER", "FORM",  "FOLDER",  "SEQUEN",  "INFO",
    "DOMAIN","STYLESH", "XMLFORM"
};

t_sckey CImpSchemaCtx::clasify(void)
{
    for (int i=0; i < SC_END - 1; i++)
    {
        if (!strcmp(m_sym, sc_patt[i]))
            return (t_sckey)(i + 1);
    }
    return SC_NO;
}

// Determines the object file pathname, writes it into pathname, opens it.
void CImpSchemaCtx::open_item_file(char *pathname, const char *srcpath, const char *itemname, const char *app)
{ 
    char itemfile[OBJ_NAME_LEN + 32];
    if (m_sym[0] && m_sym[0] != '(' && m_sym[0] != '+' && m_sym[0] != '*' && m_sym[0] != '.' && clasify() == SC_NO)
    {
        strmaxcpy(itemfile, m_sym, sizeof(itemfile));
#ifdef LINUX
		for (char *p = itemfile + strlen(itemfile) - 1; p > itemfile; p--)
		{
			if (*p == '.')
			{
				small_string(p, false);
				break;
			}
		} 
#endif	
        next_sym();
        if (itemfile[2]=='\\')
        {
            if ((itemfile[1] == ':') || (srcpath[1] != ':'))
                strcpy(pathname, itemfile);
            else
            { 
                memcpy(pathname,     srcpath, 2);
                strcpy(pathname + 2, itemfile);
            }
        }
        else
        {
            strcpy(pathname, srcpath);
            strcat(pathname, itemfile);
        }
    }
    else  // file name not specified: like object_fname, but depends on long_fnames flag from APL file
    {
        strcpy(itemfile, itemname);  
        for (int i = 0;  itemfile[i];  i++)
            if (itemfile[i] == '<' || itemfile[i] == '>' || itemfile[i] == '|' || itemfile[i] == '\"' || itemfile[i] == '\\' || itemfile[i] == '/' || itemfile[i] == ';' || itemfile[i] == '?' || itemfile[i] == ':')
                itemfile[i] = '_';
        strcat(itemfile, app); 
        strcpy(pathname, srcpath);
        strcat(pathname, itemfile);
    }
    // open the file:
    wchar_t wName[MAX_PATH];
    ReportProgress(IMPEXP_FILENAME, ToUnicode(pathname, wName, sizeof(wName) / sizeof(wchar_t)));
    m_ObjFile.Open(pathname, GENERIC_READ);
}

#ifdef WINS

void CImpSchemaCtx::copy_file(const char *filen, const char *ipath, const char *opath)
{ 
    char iname[MAX_PATH], oname[MAX_PATH];
    strcpy(iname, ipath);  strcat(iname, filen);
    strcpy(oname, opath);  strcat(oname, filen);

    if (!CopyFile(iname, oname, false))
        ReportSysError();
}

#endif // WINS

#define V4_OBJ_NAME_LEN     10
#define V4_ATTRNAMELEN      10
#define V4_CATEG_RELATION   29

typedef struct
{ uns16 size;
  uns8  valid;
  tcateg cat;
  tobjnum objnum;
  uns16 xpos, ypos;
  uns8  flags;
  char  name[1];
} t_component;

typedef struct  // the APX header structure from version 4
{ char   start_name[V4_OBJ_NAME_LEN];
  tcateg start_categ;
  char   dest_dsn[SQL_MAX_DSN_LENGTH+1];
  uns8   positions_dc;  /* 0xdc iff object window positions are valid */
  uns8   reserverd[79];
} v4_apx_header;

typedef struct
{ char    name[V4_OBJ_NAME_LEN+1];
  uns8    refint, index1, index2;
  char    tab1name[V4_OBJ_NAME_LEN+1];
  char    tab2name[V4_OBJ_NAME_LEN+1];
  char    atr1name[V4_ATTRNAMELEN+1];
  char    atr2name[V4_ATTRNAMELEN+1];
} v4_t_relation;

// Returns TRUE iff DSN read from the header. Header saved in APL descr. Reads and updates the APX file or stores defaults to apx
bool CImpSchemaCtx::apx_start(tptr fname, apx_header *apx, unsigned &header_size)
{ 
    header_size = sizeof(apx_header);
    memset(apx, 0, sizeof(apx_header));

    if (!FileExists(fname))
        return(false);

    CWBFileX ApxFile;
    ApxFile.Open(fname, GENERIC_READ);
    DWORD rdsize = ApxFile.Read(apx, sizeof(apx_header));
    if (!rdsize)
        return(false);
 
    if (apx->version < 5 || apx->version > 11)  // 4.32 and before
    {
        ApxFile.Seek(0);  // position after the header
        v4_apx_header hdr4;
        rdsize = ApxFile.Read(&hdr4, sizeof(v4_apx_header));
        if (rdsize < sizeof(v4_apx_header))
            return(false);
        header_size = sizeof(v4_apx_header);
        // convert header to version 6 first:
        memset(apx, 0, sizeof(apx_header));
        apx->version = 6;
        strcpy(apx->start_name, hdr4.start_name);
        apx->start_categ = hdr4.start_categ;
        strcpy(apx->dest_dsn, hdr4.dest_dsn);
        // clear all the new items:
        apx->sel_language    = 0;
        *apx->front_end_name = *apx->back_end_name = 0;
        apx->appl_locked     = FALSE;
    }               
    else if (apx->version < 6) // set defaults for the new (version 6) values:
    {
        header_size         -= sizeof(int) + 2 * sizeof(tobjname) + sizeof(BOOL);
        apx->version         = 6;
        apx->sel_language    = 0;
        *apx->front_end_name = *apx->back_end_name = 0;
        apx->appl_locked     = FALSE;
    }
    if (apx->version < 8) // set defaults for the new (version 8) values:
    {
        apx->admin_mode_count = 0;
        apx->version          = 8;
    }
    // clear instance-specific info:
    memset(apx->dp_uuid, 0, UUID_SIZE);
    memset(apx->os_uuid, 0, UUID_SIZE);
    memset(apx->vs_uuid, 0, UUID_SIZE);
    // copy admin_mode object names:
    if (apx->version >= 8 && apx->admin_mode_count > 1)
    {
        CBuffer amos;
        amos.AllocX(sizeof(t_amo) * apx->admin_mode_count - 1);
        ApxFile.Read(amos, sizeof(t_amo) * (apx->admin_mode_count - 1));
        Write_var(OBJ_TABLENUM, (trecnum)m_cdp->applobj, OBJ_DEF_ATR, sizeof(apx_header), rdsize, amos);
    }
    return(true);
}

#define MAX_COMP_DESCR_LEN  5000

void CImpSchemaCtx::apx_conversion(tptr fname, unsigned header_size)
{ 
    CWBFileX ApxFile;
    ApxFile.Open(fname, GENERIC_READ);
    CBuffer buf;
    buf.Alloc(MAX_COMP_DESCR_LEN);
    ApxFile.Seek(header_size);  // position after the header

    unsigned bufcont = 0, bufpos = 0;
    t_component *comp;
    
    do
    {
        // get the next component:
        comp = (t_component*)((const char *)buf + bufpos);
        if (bufcont < bufpos + sizeof(t_component) || bufcont < bufpos + comp->size)
        {
            memmov(buf, (const char *)buf + bufpos, bufcont - bufpos);
            bufcont -= bufpos;
            bufpos   = 0;
            unsigned rdsize = ApxFile.Read((char *)buf + bufcont, MAX_COMP_DESCR_LEN - bufcont);
            if (!rdsize)
                break;
            bufcont += rdsize;
        }
        if (bufcont < bufpos + sizeof(t_component) || bufcont < bufpos + comp->size)
            break;
        bufpos += comp->size;

        // process the component:
        if (comp->valid && comp->cat == V4_CATEG_RELATION)
        {
            v4_t_relation * rel = (v4_t_relation*)comp->name;
            Upcase9(m_cdp, rel->atr1name);
            Upcase9(m_cdp, rel->atr2name);
            // if (check_relation(rel)
#if 0
            Add_relation(m_cdp, rel->name, rel->tab1name, rel->tab2name, rel->atr1name, rel->atr2name, FALSE);
#endif
        }
    }
    while (comp->size);  /* end of scanning cycle, fuse against errors in file */
}

void CImpSchemaCtx::define_privils(tobjnum table, trecnum rec, tobjnum importer)
{ 
    tcateg categ;  tobjnum subjnum;  tattrib attrcnt = 32;
    const d_table * td = cd_get_table_d(m_cdp, table, CATEG_TABLE);
    if (td)
    { attrcnt = td->attrcnt;
      release_table_d(td);
    }
    t_privils_flex priv_val(attrcnt);
    next_sym();
    while (m_sym[0] != ')')
    {
        if (!strcmp(m_sym, "U"))
            categ = CATEG_USER;
        else if (!strcmp(m_sym, "G"))
            categ = CATEG_GROUP;
        else if (!strcmp(m_sym, "R"))
            categ = CATEG_ROLE;
        else
            categ = 0;
    
        if (categ)
        {
            next_sym();  // skips U/G/R
            if (categ == CATEG_ROLE)
            { 
                subjnum = Find_object(m_sym, CATEG_ROLE);
                if (subjnum == NOOBJECT)
                    subjnum = Insert_object(m_sym, CATEG_ROLE);
                next_sym();
                conv_hex_long(priv_val.the_map(), priv_val.colbytes()+1);
                if (rec != NORECNUM)  // object privileges
                {
                    if (*priv_val.the_map() & RIGHT_WRITE) 
                        priv_val.set_all_w();  // must allow writing flags, name, folder, modiftime,...
                    // memset must be the first operation, all other must UNION privils!
                    if (*priv_val.the_map() & RIGHT_READ)
                        priv_val.add_read(OBJ_DEF_ATR);
                }
                if (cd_GetSet_privils(m_cdp, subjnum, CATEG_ROLE, table, rec, OPER_SET, priv_val.the_map()))
                    ReportError();
            }
            else // user, group
            {
                WBUUID uuid;
                conv_hex(m_sym, uuid, UUID_SIZE);
                next_sym();
                if (cd_Find_object_by_id(m_cdp, uuid, categ, &subjnum))
                  m_some_subjects_not_found = true;  // not calling ReportError(); here, reporting on the end of import
                else if (categ != CATEG_USER || importer != subjnum)
                // must not set privileges of the importing user!
                {
                    conv_hex_long(priv_val.the_map(), priv_val.colbytes()+1);
                    if (rec != NORECNUM)
                    {
                      if (*priv_val.the_map() & RIGHT_WRITE) 
                          priv_val.set_all_w();  // must allow writing flags, name, folder, modiftime,...
                      // memset must be the first operation, all other must UNION privils!
                      if (*priv_val.the_map() & RIGHT_READ)
                          priv_val.add_read(OBJ_DEF_ATR);
                    }
                    if (cd_GetSet_privils(m_cdp, subjnum, categ, table, rec, OPER_SET, priv_val.the_map()))
                        ReportError();
                }
            }
        }
        else  // old syntax:
        {   uns8 priv_val[PRIVIL_DESCR_SIZE];
            tobjname group;
            tobjnum  groupobj;
            // remove EVERYBODY'S privils:
            memset(priv_val, 0, PRIVIL_DESCR_SIZE);
            if (cd_GetSet_privils(m_cdp, EVERYBODY_GROUP, CATEG_GROUP, table, rec, OPER_SET, priv_val))
                ReportError();
            // grant privils to groups:
            strmaxcpy(group, m_sym, OBJ_NAME_LEN+1);
            next_sym();
            groupobj = Find_objectX(group, CATEG_GROUP);
            if (rec == NORECNUM) /* table data privils */
                *priv_val = RIGHT_INSERT | RIGHT_DEL;
            else         /* object privils     */ 
                *priv_val = 0;
            memset(priv_val + 1, 0x55, PRIVIL_DESCR_SIZE - 1);
            if (cd_GetSet_privils(m_cdp, groupobj, CATEG_GROUP, table, rec, OPER_SET, priv_val))
                ReportError();

            if (m_sym[0] == ',')
            {
                next_sym();
                if (m_sym[0] != ')')
                {
                    strmaxcpy(group, m_sym, OBJ_NAME_LEN + 1);
                    next_sym();
                    groupobj = Find_objectX(group, CATEG_GROUP);
                    if (rec == NORECNUM) /* table data privils */ 
                        *priv_val = RIGHT_INSERT | RIGHT_DEL;
                    else         /* object privils     */
                        *priv_val = 0;
                    memset(priv_val + 1, 0xAA, PRIVIL_DESCR_SIZE - 1);
                    if (cd_GetSet_privils(m_cdp, groupobj, CATEG_GROUP, table, rec, OPER_SET, priv_val))
                        ReportError();
                }
            }
        }
        next_sym();  // skips the last symbol of the privil info
    }
    next_sym();
}

void CImpSchemaCtx::define_role(tobjnum objnum)
{ 
    tcateg categ;  tobjnum subjnum;  WBUUID uuid;
    uns32 rel_true = TRUE;
    next_sym();
    while (m_sym[0] != ')')
    {
        if (strcmp(m_sym, "U"))
            categ = CATEG_USER;
        else if (strcmp(m_sym, "G"))
            categ=CATEG_GROUP;
        else
        {
            wchar_t wMsg[64];
            throw new CWBException(Get_transl(m_cdp, AppFileDemaged, wMsg));
        }
        next_sym();
        conv_hex(m_sym, uuid, UUID_SIZE);
        next_sym();
        if (cd_Find_object_by_id(m_cdp, uuid, categ, &subjnum))
            ReportError();
        else if (cd_GetSet_group_role(m_cdp, subjnum, objnum, CATEG_ROLE, OPER_SET, &rel_true))
            ReportError();
    }
    next_sym();
}

// Creates "limited" links without creator and default role privileges, except for "Author"
void CImpSchemaCtx::make_link(tptr defin, tcateg categ, char *objname, BOOL from_v4)
{ 
    t_wb_link lnk;  t_odbc_link olnk;  bool odbc = FALSE;
    // read the file contents:

#ifdef WINS
  
    if (from_v4)
    {
        tobjname applname;
        memset(lnk.destobjname, 0, sizeof(tobjname));
        memset(applname, 0, sizeof(tobjname));
        memcpy(lnk.destobjname, defin, V4_OBJ_NAME_LEN);
        memcpy(applname,        defin, V4_OBJ_NAME_LEN);
        if (cd_Apl_name2id(m_cdp, applname, lnk.dest_uuid))
        {
            wchar_t wMsg[160];
            throw new CWBException(Get_transl(m_cdp, CannotConvertLink, wMsg));
        }
    }
    else

#endif
    { 
        memcpy(&lnk, defin, sizeof(t_wb_link));
        if (lnk.link_type == LINKTYPE_ODBC)
        {
            memcpy(&olnk, defin, sizeof(t_odbc_link));
            odbc = true;
        }
    }
    // create the link:

#if 0

    if (odbc)
    {
        if (store_odbc_link(m_cdp, objname, &olnk, FALSE, TRUE) == NOOBJECT)
            throw new C602SQLException(m_cdp, "store_odbc_link");
    }
    else 

#endif
        if (cd_Create3_link(m_cdp, lnk.destobjname, lnk.dest_uuid, categ, objname, TRUE)) /* adds component, too */
            throw new C602SQLException(m_cdp, L"Create3_link");
}

void CImpSchemaCtx::conv_hex(const char *source, uns8 *dest, unsigned size)
{ 
    char c;
    memset(dest, 0, size);
    while (size--)
    {
        c = *(source++);
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'A' && c <= 'F')
            c -= 'A' - 10;
        else
            return;
        *dest = c << 4;
        c = *(source++);
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'A' && c <= 'F')
            c -= 'A' - 10;
        else
            return;
        *dest += c;
        dest++;
    }
}

void CImpSchemaCtx::conv_hex_long(uns8 *dest, unsigned size)
// Converts multiple hex strings into the binary string in dest
{ do
  { unsigned partsize = size <= PRIVIL_DESCR_SIZE ? size : PRIVIL_DESCR_SIZE;
    conv_hex(m_sym, dest, partsize);
    size-=partsize;  
    if (!size) break;
    dest+=partsize;  
    next_sym();
  } while (true);
}
//
// Reports schema import progress
//
void CImpSchemaCtx::ReportProgress(int Categ, const wchar_t *Value)
{
    if (report_progress && callback)
    {
        char Msg[400];
        const void *Val;
        if (Categ == IMPEXP_PROGMAX || Categ == IMPEXP_PROGPOS)
            Val = Value;
        else
        {
            FromUnicode(Value, Msg, sizeof(Msg));
            Val = Msg;
        }
        callback(Categ, (void *)Val, param);
    }
}
//
// Reports schema import error caused by 602SQL API call
//
void CImpSchemaCtx::ReportError()
{
    if (report_progress && callback)
    {
        char Err[256];
        Get_error_num_text(m_cdp, cd_Sz_error(m_cdp), Err, sizeof(Err));
        callback(IMPEXP_ERROR, Err, param);
    }
}
//
// Reports schema import error caused by system API call
//
void CImpSchemaCtx::ReportSysError()
{
    if (report_progress && callback)
    {
        char Err[256];
        SysErrorMsg(Err, sizeof(Err));
        callback(IMPEXP_ERROR, Err, param);
    }
}
//
// Maze zadany objekt, nema user interface, nekontroluje pripustnost (prava, referencni vazby a pod.), vraci true pri uspechu
// false pri chybe a kod chyby v Sz_error  
//
bool DeleteObject(cdp_t cdp, const char *ObjName, tcateg Categ, const char *SchemaName)
{
    // Najit objekt
    CObjectList ol(cdp, SchemaName);
    if (!ol.Find(ObjName, Categ))
        return false;

    char qLock[160];
    tcursnum qCurs;
    const char *Tab = "OBJTAB";
    if (Categ == CATEG_TABLE)
        Tab = "TABTAB";
    else if (Categ == CATEG_USER || Categ == CATEG_GROUP)
        Tab = "USERTAB";
    sprintf(qLock, "SELECT * FROM _IV_LOCKS WHERE Schema_name='%s' AND Table_name='%s' AND Record_number=%d", SchemaName, Tab, ol.GetObjnum());
    if (cd_Open_cursor_direct(cdp, qLock, &qCurs))
        return false;
    trecnum rCnt;
    cd_Rec_cnt(cdp, qCurs, &rCnt);
    cd_Close_cursor(cdp, qCurs);
    if (rCnt)
    {
        client_error(cdp, NOT_LOCKED);
        return false;
    }

    bool AltSchema = SchemaName != NULL && *SchemaName != 0 && wb_stricmp(cdp, SchemaName, cdp->sel_appl_name) != 0;

    // IF Tabulka, Procedura, Trigger, Sequence, Domain, dropnout
    if ((Categ == CATEG_TABLE && !(ol.GetFlags() & CO_FLAG_LINK)) || Categ == CATEG_PROC || Categ == CATEG_TRIGGER || Categ == CATEG_SEQ || Categ == CATEG_DOMAIN || Categ == CATEG_INFO)
    {
        char cmd[30 + 2 * OBJ_NAME_LEN];
        if (AltSchema)
            sprintf(cmd, "DROP %s `%s`.`%s`", CCategList::Original(Categ), SchemaName, ObjName);
        else
            sprintf(cmd, "DROP %s `%s`", CCategList::Original(Categ), ObjName);
        BOOL res = cd_SQL_execute(cdp, cmd, NULL);
        if (res)
            return false;
    }
    // IF Role, prejmenovat na cislo = neviditelne jmeno
    else if (Categ == CATEG_ROLE)
    {
        tobjname name;
        int2str(ol.GetObjnum(), name); 
        if (cd_Write(cdp, OBJ_TABLENUM, ol.GetObjnum(), OBJ_NAME_ATR, NULL, name, OBJ_NAME_LEN))
            return false;
    }
    // Ostani normalne smazat v systemovych tabulkach
    else
    {
        ttablenum tb = CATEG2SYSTAB(Categ);
        if (cd_Delete(cdp, tb, ol.GetLinkOrObjnum()))
            return false;
    }
    // IF aktualni aplikace, smazat v cachi
    if (!AltSchema)
    {
        Remove_from_cache(cdp, Categ, ObjName);
#if 0
        if (IS_ODBC_TABLEC(Categ, ol.GetObjnum()))
            Unlink_odbc_table(cdp, ol.GetObjnum());
#endif      
    }
    return true;
}
//
// Maze zadany objekt, kontroluje pripustnost (prava, referencni vazby a pod.), vraci false pri uspechu
// true pri chybe, nebo pokud byla akce prerusena uzivatelem 
//
CFNC DllKernel BOOL WINAPI Delete_object(cdp_t cdp, const char *ObjName, tcateg Categ, const char *SchemaName, bool Ask)
{
    CDelObjCtx Ctx(cdp, SchemaName, Ask);
    bool OK = Ctx.Delete(ObjName, Categ);
    if (OK && CPInterface)
    {
        if (!SchemaName || !*SchemaName)
            SchemaName = cdp->sel_appl_name;
        CPInterface->DeleteObject(cdp->locid_server_name, SchemaName, ObjName, Categ);
    }
    return(!OK);
}
//
// Maze mnozinu objektu
//
CFNC DllKernel unsigned WINAPI Delete_objects(cdp_t cdp, const t_delobjctx *DelCtx)
{
    CDelObjCtx Ctx(cdp, DelCtx);
    unsigned Deleted = Ctx.Delete();
    if (Deleted && CPInterface)
        CPInterface->RefreshSchema(cdp->locid_server_name, DelCtx->schema);
    return(Deleted);
}
//
// Constructor, for single object delete
//
CDelObjCtx::CDelObjCtx(cdp_t cdp, const char *Schema, bool Ask) : m_ol(cdp, Schema)
{
    memset((t_delobjctx *)this, 0, sizeof(t_delobjctx));
    strsize = sizeof(t_delobjctx);
    count   = 1;
    if (Schema)
        strcpy(schema, Schema);
    else
        *schema = 0;
    param     = 0;
    m_cdp     = cdp;
    m_Flags   = Ask ? 0 : DOPF_NOCONFIRMASK;
    m_Single  = true;
    m_Deleted = 0;
}
//
// Constructor, for object set delete
//
CDelObjCtx::CDelObjCtx(cdp_t cdp, const t_delobjctx *DelCtx) : m_ol(cdp, DelCtx->schema)
{
    memcpy((t_delobjctx *)this, DelCtx, sizeof(t_delobjctx));
    if (flags & DOF_NOCONFIRM)
        m_Flags = DOPF_NOCONFIRMASK;
    else
        m_Flags = 0;
    m_Flags  |= (flags & DOF_CONFSTEPBYSTEP);
    m_cdp     = cdp;
    m_Single  = count == 1;
    m_Deleted = 0;
}
//
// Deletes set of objects, returns object count realy deleted
//
unsigned CDelObjCtx::Delete()
{
    char   ObjName[256];
    CCateg Categ;

    // WHILE is next object to delete
    while (Next(ObjName, &Categ))
    {
        ObjName[OBJ_NAME_LEN] = 0;
        // IF object delete failed, break delete process
        if (!Delete(ObjName, Categ))
            break;
    }
    return(m_Deleted);
}
//
// Deletes one object or folder, returns false for break delete process
//
bool CDelObjCtx::Delete(const char *ObjName, CCateg Categ)
{
    // IF object is folder, delete all objects in folder
    if (Categ.m_Categ == CATEG_FOLDER || Categ.m_Categ == NONECATEG)
    {
        if (!DeleteFolder(ObjName))
            return(false);
    }
    // IF delete all objects of given category in the folder
    else if (Categ.m_Opts & CO_ALLINFOLDER)
    {
        if (!DeleteCateg(ObjName, Categ))
            return(false);
    }
    // ELSE simple object
    else
    {
        if (!DeleteObject(ObjName, Categ))
            return(false);
    }
    return(true);
}
//
// Deletes one object, returns false for break delete process
//
bool CDelObjCtx::DeleteObject(const char *ObjName, tcateg Categ)
{
    const wchar_t *Msg;
    wchar_t wMsg[256];
    wchar_t fmt[128];
    wchar_t wName[OBJ_NAME_LEN + 1];
    // find the link object (if exists):
    CObjectList ol(m_cdp, schema);
    if (!ol.Find(ObjName, Categ))  // must find link object
        return(true); //CallState(OBJECT_NOT_FOUND, NULL, ObjName, Categ));

    // check the delete privileges and encryption of the object:
    tobjnum lobjnum = ol.GetLinkOrObjnum();
    if (Categ != CATEG_USER && Categ != CATEG_GROUP && Categ != CATEG_ROLE && !IS_ODBC_TABLEC(Categ, ol.GetObjnum()) && !check_object_access(m_cdp, Categ, lobjnum, 2))
        return(CallState(NO_RIGHT, NULL, ObjName, Categ));

    if (Categ != CATEG_GROUP && Categ != CATEG_CONNECTION && (/*!can_modify(m_cdp) ||*/ Categ != CATEG_ROLE && !not_encrypted_object(m_cdp, Categ, lobjnum)))
        return(CallState(APPL_IS_LOCKED, NULL, ObjName, Categ));

#if 0
    if (Categ == CATEG_CONNECTION)
    {
        t_connection *conn = find_data_source(m_cdp, ObjName);
        if (conn && conn->ltabs.count())
            return(CallState(ODBC_TABLES_LINKED, STR_TABLES_LINKED, ObjName, Categ));
    }
#endif

    // confirm deleting:
    if (!(m_Flags & (DOPF_NOCONFIRM | DOPF_NOEMPTYFLDRS)))
    {
        unsigned Stat;
        if (count != 1 && !(m_Flags & DOPF_CONFSTEPBYSTEP))
        {
            Msg  = Get_transl(m_cdp, DELETE_ALL_CONFIRM_PATTERN, wMsg);
            Stat = DOW_CONFIRMALL;
        }
        else
        {
            wchar_t wc[64];
            const wchar_t *obj_accuss;
            if ((ol.GetFlags() & CO_FLAG_LINK) || Categ == CATEG_TABLE && (ol.GetFlags() & CO_FLAG_ODBC))
                obj_accuss = DEL_LINK_OBJECT;
            else 
                obj_accuss = CCategList::Lower(m_cdp, Categ, wc);
            Stat = DOW_CONFIRM;
            swprintf (wMsg, sizeof(wMsg) / sizeof(wchar_t), Get_transl(m_cdp, DELETE_CONFIRM_PATTERN, fmt),  obj_accuss, ToUnicode(m_cdp, ObjName, wName, sizeof(wName) / sizeof(wchar_t)));
            Msg = wMsg;
        }
        tiocresult Res = Warning(Stat, Msg, ObjName, CATEG_FOLDER);
        // IF not delete this object, skip object delete
        if (Res == iocNo)
            return(true);
        // IF Cancel, break delete process
        if (Res == iocCancel)
            return(false);
        // IF Yes To All, reset confirm flag
        if (Res == iocYesToAll || Stat == DOW_CONFIRMALL || (Res == iocYes && count <= 1))
            m_Flags |= DOPF_NOCONFIRM;
    }      

    if (Categ == CATEG_USER)  // check ADMIN groups
    {
        uns32 relation;
        if (!(m_Flags & DOPF_NOCONFADMIN) && !cd_GetSet_group_role(m_cdp, ol.GetObjnum(), CONF_ADM_GROUP, CATEG_GROUP, OPER_GET, &relation) && relation)
        {
            tiocresult Res = Warning(DOW_CONFADMIN, Get_transl(m_cdp, DELETING_CONF_ADMIN, fmt), ObjName, CATEG_USER);
            // IF not delete this object, skip object delete
            if (Res == iocNo)
                return(true);
            // IF Cancel, break delete process
            if (Res == iocCancel)
                return(false);
            // IF Yes To All, reset check ADMIN flag
            if (Res == iocYesToAll)
                m_Flags |= DOPF_NOCONFADMIN;
        }
        if (!(m_Flags & DOPF_NOSECURADMIN) && !cd_GetSet_group_role(m_cdp, ol.GetObjnum(), SECUR_ADM_GROUP, CATEG_GROUP, OPER_GET, &relation) && relation)
        {
            tiocresult Res = Warning(DOW_SECURADMIN, Get_transl(m_cdp, DELETING_SECUR_ADMIN, fmt), ObjName, CATEG_USER);
            if (Res == iocNo)
                return(true);
            if (Res == iocCancel)
                return(false);
            if (Res == iocYesToAll)
                m_Flags |= DOPF_NOSECURADMIN;
        }
    }

    bool OK = ::DeleteObject(m_cdp, ObjName, Categ, schema);
    
    if (OK)
    {
        // Pro pohled smazat graf, pro program smazat exe
        if (Categ == CATEG_VIEW || Categ == CATEG_PGMSRC)
        {
            tcateg auxcat = Categ == CATEG_VIEW ? CATEG_GRAPH : CATEG_PGMEXE;
            if (ol.Find(ObjName, Categ))
            {
                OK = ::DeleteObject(m_cdp, ObjName, auxcat, schema);
                if (!OK)
                    Categ = auxcat;
            }
        }
    }
    else
    {
        if ((Categ == CATEG_TABLE || Categ == CATEG_DOMAIN) && cd_Sz_error(m_cdp) == REFERENCED_BY_OTHER_OBJECT)
        {
            // table is referenced, ask if can DROP TABLE ... CASCADE:
            if (!(m_Flags & DOPF_NOCASCADEDEL))
            {
                wchar_t wc[64];
                swprintf (wMsg, sizeof(wMsg) / sizeof(wchar_t), Get_transl(m_cdp, CASCADE_TABLE_DEL, fmt), CCategList::CaptFirst(m_cdp, Categ, wc), ToUnicode(m_cdp, ObjName, wName, sizeof(wName) / sizeof(wchar_t)));
                tiocresult Res = Warning(DOW_CASCADEDEL, wMsg, ObjName, Categ); 
                if (Res == iocNo)
                    return(true);
                if (Res == iocCancel)
                    return(false);
                if (Res == iocYesToAll)
                    m_Flags |= DOPF_NOCASCADEDEL;
            }
            char cmd[30 + 2 * OBJ_NAME_LEN];
            if (schema != NULL && *schema != 0 && wb_stricmp(m_cdp, schema, m_cdp->sel_appl_name) != 0)
                sprintf(cmd, "DROP %s `%s`.`%s` CASCADE", CCategList::Original(Categ), schema, ObjName);
            else
                sprintf(cmd, "DROP %s `%s` CASCADE", CCategList::Original(Categ), ObjName);
            BOOL res = cd_SQL_execute(m_cdp, cmd, NULL);
            if (!res)
            {
                OK = true;
                update_cached_info(m_cdp);
            }
        }
    }
    if (OK)
        m_Deleted++;
    return(CallState(OK ? 0 : cd_Sz_error(m_cdp), NULL, ObjName, Categ));
}
//
// Deletes all objects of given category in given folder, returns false for break delete process
//
bool CDelObjCtx::DeleteCateg(const char *FolderName, tcateg Categ)
{
    bool Found;
    // Get object count of given category in given folder
    int Cnt = m_ol.GetCount(Categ, FolderName);
    if (Cnt)
    {
        // Create deleted objects name list
        CBuffer List;
        int Size = Cnt * (OBJ_NAME_LEN + 1);
        List.AllocX(Size);
        char *p;
        for (p = List, Found = m_ol.FindFirst(Categ, FolderName); Found; p += OBJ_NAME_LEN + 1, Found = m_ol.FindNext())
            strcpy(p, m_ol.GetName());
        // Delete objects in the list
        for (p = List; p < (char *)List + Size; p += OBJ_NAME_LEN + 1)
        {
            if (!DeleteObject(p, Categ))
                return(false);
        }
    }
    // FOR each subfolder in given folder, delete all objects of given category
    CObjectList fl(m_cdp, schema);
    for (Found = fl.FindFirst(CATEG_FOLDER, FolderName); Found; Found = fl.FindNext())
    {
        if (!DeleteCateg(fl.GetName(), Categ))
            return(false);
    }
    return(true);
}
//
// Deletes all objectsin given folder, returns false for break delete process
//
bool CDelObjCtx::DeleteFolder(const char *FolderName)
{
    // IF Folder is not empty AND ask flag set
    if (!(m_Flags & DOPF_NOEMPTYFLDRS) && !IsFolderEmpty(m_cdp, FolderName))
    { 
        wchar_t Fmt[128];
        wchar_t Msg[160];
        wchar_t wName[OBJ_NAME_LEN + 1];
        // Ask user
        swprintf (Msg, sizeof(Msg) / sizeof(wchar_t), Get_transl(m_cdp, FolderNotEmpty, Fmt), ToUnicode(m_cdp, FolderName, wName, sizeof(wName) / sizeof(wchar_t)), FolderName);
        tiocresult Res = Warning(DOW_FLDRNOTEMPTY, Msg, FolderName, CATEG_FOLDER);
        // IF not delete this folder, skip folder delete
        if (Res == iocNo)
            return(true);
        // IF Cancel, break delete process
        if (Res == iocCancel)
            return(false);
        // IF Yes To All, reset ask flag
        if (Res == iocYesToAll || (Res == iocYes && count <= 1))
            m_Flags |= DOPF_NOEMPTYFLDRS;
    }
    // FOR each subfolder in given folder, delete subfolder
    CObjectList fl(m_cdp, schema);
    for (bool Found = fl.FindFirst(CATEG_FOLDER, FolderName); Found; Found = fl.FindNext())
    {
        if (!DeleteFolder(fl.GetName()))
            return(false);
    }
    // FOR each category, delete all objects of given category
    for (tcateg Categ = 0; Categ < CATEG_COUNT; Categ++)
    {
        if (CCategList::Exportable(Categ))
            if (!DeleteCateg(FolderName, Categ))
                return(false);
    }
    // IF not root folder, delete folder object
    if (!IsRootFolder(FolderName) && fl.Find(FolderName, CATEG_FOLDER))
    {
        cd_Delete(m_cdp, OBJ_TABLENUM, fl.GetObjnum());
        if (!schema || !*schema || wb_stricmp(m_cdp, schema, m_cdp->sel_appl_name))
        {
            t_comp *comp = (t_comp *)fl.GetSource();
            *comp->name  = 0;
        }
        m_Deleted++;
    }
    return(true);
}
//
// Builds error message and calls State function
//
bool CDelObjCtx::CallState(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ)
{
    wchar_t wMsg[256];
    if (Stat && !Msg)
    { 
        Get_error_num_textW(m_cdp, Stat, wMsg, sizeof(wMsg) / sizeof(wchar_t));
        Msg = wMsg;
    }
    return(State(Stat, Msg, ObjName, Categ));
}
 
tiocresult CDelObjCtx::Warning(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ)
{
    if (warnfunc)
        return(warnfunc(Stat, Msg, ObjName, Categ, m_Single, param));
    return(iocNo);
}

bool CDelObjCtx::State(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ)
{
    if (statfunc)
        return(statfunc(Stat, Msg, ObjName, Categ, m_Single, param));
    return Stat == 0;
}

const wchar_t *GetErrMsg(cdp_t cdp, int Err, wchar_t *Buf)
{
    if (!Err)
        Err = cd_Sz_error(cdp);
    if (Err != OS_FILE_ERROR)
        Get_error_num_textW(cdp, Err, Buf, 256);
    else
    {
        char Msg[256];
        SysErrorMsg(Msg, sizeof(Msg));
        ToUnicode(Msg, Buf, 256);
    }
    return Buf;
}

bool rename_object_sql(cdp_t cdp, const char * categ_name, const char *OldName, const char *NewName)
// Renames a table in the current schema. Vraci true v pripade uspechu, false v pripade chyby a kod chyby v cdp.
{ 
    char cmd[100 + 3 * OBJ_NAME_LEN];
    sprintf(cmd, "RENAME %s `%s`.`%s` TO `%s`", categ_name, cdp->sel_appl_name, OldName, NewName);
    return !cd_SQL_execute(cdp, cmd, NULL);
}

bool rename_object_sql_nopref(cdp_t cdp, const char * categ_name, const char *OldName, const char *NewName)
// Renames a table in the current schema. Vraci true v pripade uspechu, false v pripade chyby a kod chyby v cdp.
{ char cmd[100+3*OBJ_NAME_LEN];
  sprintf(cmd, "RENAME %s `%s` TO `%s`", categ_name, OldName, NewName);
  return !cd_SQL_execute(cdp, cmd, NULL);
}

//
// Prejmenuje objekt OldName na NewName. Nema user interface. Vraci false v pripade uspechu, true v pripade chyby a kod chyby v cdp.
//
CFNC DllKernel BOOL WINAPI Rename_object(cdp_t cdp, const char *OldName, const char *NewName, tcateg Categ)
{
    CObjectList ol(cdp);
    if (!ol.Find(OldName, Categ))
    {
        client_error(cdp, OBJECT_NOT_FOUND);
        return true;
    }

    if (/*Categ == CATEG_TRIGGER || Categ == CATEG_DOMAIN ||*/ IS_ODBC_TABLEC(Categ, ol.GetObjnum()))
    { 
        client_error(cdp, OBJ_CANNOT_RENAME);  
        return true; 
    }
    if (!not_encrypted_object(cdp, Categ, ol.GetObjnum()))
    {
        client_error(cdp, APPL_IS_LOCKED);
        return true;  // otherwise the table would be decrypted by renaming
    }
    ttablenum tb    = (Categ==CATEG_SERVER) ? SRV_TABLENUM : CATEG2SYSTAB(Categ);
    tobjnum lobjnum = ol.GetLinkOrObjnum();
    if (Categ != CATEG_SERVER && !check_object_access(cdp, Categ, lobjnum, 1))
    {
        client_error(cdp, NO_RIGHT);
        return true;
    }
    uns16 flags;
    if (tb == TAB_TABLENUM || tb == OBJ_TABLENUM)  // must not read for server, user, group!
        cd_Read(cdp, tb, (trecnum)ol.GetObjnum(), OBJ_FLAGS_ATR, NULL, &flags);
    else
        flags = 0;  // may show the "link" icon if undefined

    tobjname UpName;
    strmaxcpy(UpName, NewName, OBJ_NAME_LEN);
    Upcase9(cdp, UpName);
    //cd_Start_transaction(cdp);
    // not using get_object_flags(my_cdp, CATEG_TABLE, objname), because it does not return the PROTECT flag
    /* table: change the name in the definition, disable in in ref. integr. */
    if (Categ == CATEG_TABLE) /* new intertable links must be crated */
    {
        if (!(flags & (CO_FLAG_LINK | CO_FLAG_ODBC)))
        {
#ifdef STOP  // better way on the new server
            tptr def = cd_Load_objdef(cdp, ol.GetObjnum(), CATEG_TABLE);
            if (!def)
                goto error;
            table_all ta;
            BOOL res = compile_table_to_all(cdp, def, &ta);
            corefree(def);
            if (res)
                goto error;
            /* check ref. integrity: */
            if (ta.refers.count())
            {
                client_error(cdp, REF_CANNOT_RENAME);
                goto error;
            }
            /* update and save the definition: */
            strmaxcpy(ta.selfname, NewName, OBJ_NAME_LEN);
            ta.schemaname[0] = 0;
            def = table_all_to_source(&ta, FALSE);
            if (!def)
            { 
                client_error(cdp, OUT_OF_APPL_MEMORY);
                goto error;
            }
            if (cd_Write_var(cdp, tb, ol.GetObjnum(), OBJ_DEF_ATR, NOINDEX, 0, strlen(def) + 1, def)) 
            { 
                corefree(def);
                goto error;
            }
            cd_Write_len(cdp, tb, ol.GetObjnum(), OBJ_DEF_ATR, NOINDEX, strlen(def) + 1); 
            corefree(def);
#else
            if (!rename_object_sql(cdp, "TABLE", OldName, NewName)) goto error;
            inval_table_d(cdp, NOOBJECT, CATEG_TABLE);  /* remove defs from the cache */
            inval_table_d(cdp, NOOBJECT, CATEG_CURSOR); // cursors may depend on the table
            goto endplay;
#endif
        }
        inval_table_d(cdp, NOOBJECT, CATEG_TABLE);  /* remove defs from the cache */
        inval_table_d(cdp, NOOBJECT, CATEG_CURSOR); // cursors may depend on the table
    }

#ifdef STOP  // better way on the new server
    /* write the new name: */
    if (cd_Write(cdp, tb, lobjnum, Categ == CATEG_SERVER ? SRV_ATR_NAME : OBJ_NAME_ATR, NULL, UpName, OBJ_NAME_LEN))
        goto error;

    if (Categ == CATEG_PGMSRC || Categ == CATEG_VIEW)
    {
        tobjnum object;
        if (!cd_Find_object(cdp, OldName, Categ == CATEG_PGMSRC ? CATEG_PGMEXE : CATEG_GRAPH, &object))
            if (cd_Write(cdp, OBJ_TABLENUM, (trecnum)object, OBJ_NAME_ATR, NULL, UpName, OBJ_NAME_LEN))
                goto error;
    }
    /* change the application description: */
    else if (Categ == CATEG_APPL)
    { 
        if (wb_stricmp(cdp, cdp->sel_appl_name, OldName) == 0) 
            cd_Set_application_ex(cdp, UpName, TRUE);
    }
    else if (Categ == CATEG_PROC || Categ == CATEG_TRIGGER || Categ == CATEG_SEQ || Categ == CATEG_DOMAIN)
    {
        if (!RenameDefin(cdp, ol.GetObjnum(), NewName, Categ))
            goto error;
    }
#else
    if (Categ == CATEG_PGMSRC || Categ == CATEG_VIEW || Categ == CATEG_DRAWING || Categ == CATEG_FOLDER || Categ == CATEG_XMLFORM)  // categories not recognized by the server
    { if (cd_Write(cdp, tb, lobjnum, OBJ_NAME_ATR, NULL, UpName, OBJ_NAME_LEN))
        goto error;
      uns32 stamp = stamp_now();
      cd_Write(cdp, tb, lobjnum, OBJ_MODIF_ATR,  NULL, &stamp, sizeof(uns32));
     // hidden objects:
      if (Categ == CATEG_PGMSRC || Categ == CATEG_VIEW)
      { tobjnum object;
        if (!cd_Find_object(cdp, OldName, Categ == CATEG_PGMSRC ? CATEG_PGMEXE : CATEG_GRAPH, &object))
          if (cd_Write(cdp, OBJ_TABLENUM, object, OBJ_NAME_ATR, NULL, UpName, OBJ_NAME_LEN))
            goto error;
      }
      else if (Categ == CATEG_FOLDER)
      {
          // Updatovat objekty v TABTAB a OBJTAB
          char Cmd[192];
          strcpy(Cmd, "UPDATE TABTAB SET Folder_name = '");
          strcat(Cmd, UpName);
          strcat(Cmd, "' WHERE Folder_name = '");
          strcat(Cmd, OldName);
          strcat(Cmd, "' AND Apl_uuid = X'");
          int sz = (int)strlen(Cmd);
          bin2hex(Cmd + sz, cdp->sel_appl_uuid, UUID_SIZE);
          *(uns16 *)(Cmd + sz + 2 * UUID_SIZE) = '\'';
          if (cd_SQL_execute(cdp, Cmd, NULL))
              goto error;
          *(uns32 *)(Cmd + 7) = 'O' + ('B' << 8) + ('J' << 16) + ('T' << 24);
          if (cd_SQL_execute(cdp, Cmd, NULL))
              goto error;
      }
    }
    else if (Categ == CATEG_APPL)
    { if (!rename_object_sql_nopref(cdp, "SCHEMA", OldName, NewName)) 
        goto error;
      if (wb_stricmp(cdp, cdp->sel_appl_name, OldName) == 0) 
        cd_Set_application_ex(cdp, UpName, TRUE);
    }
    else if (Categ == CATEG_USER || Categ == CATEG_GROUP)
    { if (!rename_object_sql_nopref(cdp, CCategList::Original(Categ), OldName, NewName)) 
        goto error;
    }
    else  // SQL objects
    { if (!rename_object_sql(cdp, 
            Categ==CATEG_CURSOR ? "VIEW" :
            Categ==CATEG_APPL   ? "SCHEMA" :
            Categ==CATEG_STSH   ? "STYLESHEET" :
            CCategList::Original(Categ), 
            OldName, NewName)) 
        goto error;
     // if the definition has not been modified (because it does not contain the name), update the timestamp:   
      if (Categ==CATEG_STSH || Categ==CATEG_CURSOR)  
      { uns32 stamp = stamp_now();
        cd_Write(cdp, tb, lobjnum, OBJ_MODIF_ATR,  NULL, &stamp, sizeof(uns32));
      }  
    }
#endif

   endplay:
    // Prejmenovat v cachi
    t_dynar *d_comp;
    d_comp = get_object_cache(cdp, Categ);
    if (d_comp)
    { 
        tobjname on;
        strcpy(on, OldName);
        Upcase9(cdp, on);
        if (Categ == CATEG_FOLDER)
        {
            for (int i = 0; i < d_comp->count(); i++)
            {
                t_comp *comp = (t_comp *)d_comp->acc0(i);
                if (strcmp(comp->name, on) == 0)
                {
                    strcpy(comp->name, UpName);
                    break;
                }
            }
        }
        else
        {// read timestamp and folder:
          unsigned pos;  t_folder_index fi;
          if (comp_dynar_search(d_comp, on, &pos))
          { t_comp * acomp = (t_comp*)d_comp->acc(pos);
            fi = acomp->folder;
          }
          else
            fi = FOLDER_ROOT;
         // remove and add the object:
          unsigned cache_index;
          Remove_from_cache(cdp, Categ, on);
          object_to_cache(cdp, d_comp, Categ, UpName, ol.GetObjnum(), flags, &cache_index); // does not define folder/timestamp
         // insert timestamp and restore the folder:
          t_comp * acomp    = (t_comp*)d_comp->acc0(cache_index);
          if (acomp)
          { acomp->folder = fi;
            if (tb==TAB_TABLENUM || tb==OBJ_TABLENUM)  // the objects have the timestamp column
              Get_obj_modif_time(cdp, lobjnum, Categ, &acomp->modif_timestamp);
          }
        }
    }

    if (Categ == CATEG_APPL || Categ == CATEG_USER || Categ == CATEG_GROUP)
        CPInterface->RefreshServer(cdp->locid_server_name);
    else
        CPInterface->RefreshSchema(cdp->locid_server_name, cdp->sel_appl_name);
    //cd_Commit(cdp);
    return false;

error:

    //cd_Roll_back(cdp);
    return true;
}
//
// Vytvoreni slozky
//
CFNC DllKernel BOOL WINAPI Create_folder(cdp_t cdp, const char *FolderName, const char *ParentName)
{
    tobjnum objnum;
    if (cd_Insert_object(cdp, FolderName, CATEG_FOLDER, &objnum))
        return true;
    if (cd_Write(cdp, OBJ_TABLENUM, objnum, OBJ_FOLDER_ATR, NULL, ParentName, OBJ_NAME_LEN))
        return true;
    AddObjectToCache(cdp, FolderName, objnum, CATEG_FOLDER, 0, ParentName);
    return false;
}
//
// Adds new object to object cache
//
CFNC DllKernel void WINAPI AddObjectToCache(cdp_t cdp, const char *ObjName, tobjnum objnum, tcateg Categ, uns16 Flags, const char *FolderName)
{
    t_dynar *d_comp  = get_object_cache(cdp, Categ);
    if (d_comp)  // cached category
    {
        unsigned Index;
        objnum = object_to_cache(cdp, d_comp, Categ, ObjName, objnum, Flags, &Index);
        t_comp *acomp = (t_comp*)d_comp->acc0(Index);
        acomp->folder = FOLDER_ROOT;
        if (CCategList::HasFolders(Categ) && FolderName && !IsRootFolder(FolderName))
        {
            t_folder_index fIndex = 0;
            CObjectList ol(cdp);
            if (ol.Find(FolderName, CATEG_FOLDER))
                fIndex = ol.GetIndex();
            else
            {
                tobjnum fObjNum = NOOBJECT;
                // IF Folder neni v cachi, ale je v databazi, zjistit jeho objnum a pridat do cache
                cd_Find_prefixed_object(cdp, cdp->sel_appl_name, FolderName, CATEG_FOLDER, &fObjNum);
                fIndex = search_or_add_folder(cdp, FolderName, fObjNum);
            }
            acomp->folder = fIndex;
        }
        acomp->modif_timestamp = stamp_now();
    }
}
//
// Refreshes control panel
//
CFNC DllKernel void WINAPI RefreshControlPanel(const char *ServerName, const char *SchemaName)
{
    if (CPInterface)
    {
        if (ServerName && *ServerName)
        {
            if (SchemaName && *SchemaName)
                CPInterface->RefreshSchema(ServerName, SchemaName);
            else 
                CPInterface->RefreshServer(ServerName);
        }
        else
        {
            CPInterface->RefreshPanel();
        }
    }
}
//
// Pomocne procedury pro import a export
//
//
// Checks if object name is not occupied by another object
// If object of given name allready exists, returns false,
// in pObjnum object number and in pCateg object category
//
CFNC DllKernel bool WINAPI IsObjNameFree(cdp_t cdp, const char *ObjName, tcateg Categ, tobjnum *pObjnum, tcateg *pCateg)
{
    tobjnum auxobj;
    tcateg  auxcat;
    if (pObjnum)
        *pObjnum = NOOBJECT;
    else
        pObjnum = &auxobj;
    char aName[OBJ_NAME_LEN + 3];
    *aName = '`';
    int len = (int)strlen(ObjName);
    if (len > OBJ_NAME_LEN)
        len = OBJ_NAME_LEN;
    memcpy(aName + 1, ObjName, len);
    *(short *)(aName + len + 1) = '`';
    // IF existuje objekt, ObjName neni free
    if (!cd_Find_object(cdp, aName, Categ, pObjnum))
    {
        if (pCateg)
            *pCateg = Categ;
        return(false);
    }
    // IF jiny kod nez OBJECT_NOT_FOUND, chyba
    if (cd_Sz_error(cdp) != OBJECT_NOT_FOUND)
        return(false);
    // IF CATEG_CURSOR, otestovat jeste tabulku a naopak
    if (Categ == CATEG_CURSOR || Categ == CATEG_TABLE)
    {
        auxcat = Categ == CATEG_CURSOR ? CATEG_TABLE : CATEG_CURSOR;
        if (!cd_Find_object(cdp, aName, auxcat, pObjnum))
        {
            if (pCateg)
                *pCateg = auxcat;
            return(false);
        }
        if (cd_Sz_error(cdp) != OBJECT_NOT_FOUND)
            return(false);
    }
    return(true);
}
//
// Returns sure free object name
//
CFNC DllKernel bool WINAPI GetFreeObjName(cdp_t cdp, char *ObjName, tcateg Categ)
{
    tobjnum objnum;
    char *p;
    if (IsObjNameFree(cdp, ObjName, Categ, &objnum))
        return true;
    for (p = ObjName + strlen(ObjName) - 1; p >= ObjName && *p >= '0' && *p <= '9'; p--);
    p++;
    int i = atoi(p);
    for (;;)
    {
        char Num[16];
        itoa(++i, Num, 10);
        if (p - ObjName + strlen(Num) > OBJ_NAME_LEN)
            p--;
        strcpy(p, Num);
        if (IsObjNameFree(cdp, ObjName, Categ, &objnum))
            break;
        if (objnum == NOOBJECT && Categ != CATEG_FOLDER)
            return(false);
    }
    return(true);
}
//
// Returns file name for object export
// Input: dst   - Buffer for file name
//        src   - Object name
//        app   - File name suffix
//
CFNC DllKernel const char * WINAPI object_fname(cdp_t cdp, char *dst, const char *src, const char *app)
{
    strcpy(dst, src);
    Upcase9(cdp, dst);
    ToASCII(dst, cdp->sys_charset);
    strcat(dst, app);
    return dst;
}
//
// Return file name suffix for given category
//
CFNC DllKernel const char * WINAPI GetSuffix(tcateg Categ)
{
    return(Categ < CATEG_COUNT ? CCategList::FileType(Categ) : "*.txt");
}

struct tpicttype
{
    const char *FileType;
    const char *FileTypeFilter;
};

tpicttype PictTypeTB[] =
{
    {"",     "All files (*.*)|*.*"},                         // OPEN_DEFTYPE
    {".bmp", "BMP Pictures (*.bmp)|*.bmp|"},                 // OPEN_BMP
    {".jpg", "JPEG Pictures (*.jpg;*.jpeg)|*.jpg; *.jpeg|"}, // OPEN_JPEG
    {".gif", "GIF Pictures (*.gif)|*.gif|"},                 // OPEN_GIF
    {".pcx", "PCX Pictures (*.pcx)|*.pcx|"},                 // OPEN_PCX
    {".tif", "TIFF Pictures (*.tif)|*.tif|"},                // OPEN_TIFF
    {".wpg", "WPG Pictures (*.wpg)|*.wpg|"},                 // OPEN_WPG
};

const char AllPictFilter[] = "All pictures (*.bmp; *.jpg; *.jpeg; *.gif; *.pcx; *.tif; *.wpg)|*.bmp;*.jpg;*.jpeg;*.gif;*.pcx;*.tif;*.wpg|";

#define     OPEN_BMP          1
#define     OPEN_WPG          6

#if 0

wxString GetPictFilter(int PictType)
{
    wxString Result;
    if (PictType >= OPEN_BMP && PictType <= OPEN_WPG)
    {
        Result.Append(wxString(PictTypeTB[PictType].FileTypeFilter, *wxConvCurrent));
        Result.Append(wxString(AllPictFilter, *wxConvCurrent));
    }
    Result.Append(FilterAll);
    return(Result);
}


const char *GetPictSuffix(int PictType)
{
    if (PictType >= OPEN_BMP && PictType <= OPEN_WPG)
        return(PictTypeTB[PictType].FileType);
    else
        return("");
}

#endif

int get_object_descriptor_data(cdp_t cdp, const char* buf, bool UTF8, tobjname folder_name, uns32 * stamp)
{ const char *ps = buf;
  uns32 BOM = *(uns32 *)buf & 0xFFFFFF;
  if (BOM == UTF_8_BOM)
      ps += 3;
  if (strlen(ps) >= 46 && ps[0]=='{' && ps[1]=='$' && ps[2]=='$' && ps[13]==' ')
  { if (stamp)
    { const char * p=ps+3;  *stamp=0;
      while (*p!=' ')
      { *stamp=10 * *stamp + (*p-'0');
        p++;
      } 
    }
    const char *pe;
    for (pe = ps + OBJECT_DESCRIPTOR_SIZE - 1; *pe && *pe != '}'; pe++);
    if (folder_name)
    { char *p;
      for (p = (char *)pe - 1; p >= ps + 14 && *p == ' '; p--);
      p++;
      char c = *p;
      *p = 0;
      if (ps[14])
        superconv(ATT_STRING, ATT_STRING, ps + 14, folder_name, t_specif(2 * OBJ_NAME_LEN, UTF8 ? 7 : 1, 0, 0), t_specif(OBJ_NAME_LEN, cdp->sys_charset, 0, 0), NULL);
      else
        *folder_name = 0;
      *p = c;
    }
    return (int)(pe - buf + 1);
  }
  else
  { if (folder_name) *folder_name=0;
    if (stamp) *stamp=0;
    return (int)(ps - buf);
  }
}
//
// Stores object to folder
//
CFNC DllKernel BOOL WINAPI Set_object_folder_name(cdp_t cdp, tobjnum objnum, tcateg categ, const char * folder_name)
{ tobjname ufolder;
// check the category:
  if (categ==CATEG_ROLE || categ==CATEG_CONNECTION || categ==CATEG_APPL || 
      categ==CATEG_PICT || categ==CATEG_GRAPH      || categ==CATEG_PGMEXE || !*cdp->sel_appl_name) 
  { client_error(cdp, ERROR_IN_FUNCTION_CALL);
    return TRUE;
  }
  strmaxcpy(ufolder, folder_name, sizeof(tobjname));
  Upcase9(cdp, ufolder);
  return cd_Write(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_FOLDER_ATR, NULL, ufolder, OBJ_NAME_LEN); 
}

CFNC DllKernel BOOL WINAPI Move_obj_to_folder(cdp_t cdp, const char *ObjName, tcateg Categ, const char *DestFolder)
// Not changing the object modificatrion time.
{// find the object: 
  tobjnum objnum;  
  if (cd_Find_object(cdp, ObjName, Categ, &objnum))
    return TRUE;
  return Set_object_folder_name(cdp, objnum, Categ, DestFolder);  
}

//
// Returns true if object is not encrypted
//
CFNC DllKernel bool WINAPI not_encrypted_object(cdp_t cdp, tcateg categ, tobjnum objnum)
{ if (!ENCRYPTED_CATEG(categ)) return true;
  uns16 flags;
  cd_Read(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_FLAGS_ATR, NULL, &flags);
  return !(flags & CO_FLAG_ENCRYPTED);
}

CFNC DllKernel bool WINAPI check_object_access(cdp_t cdp, tcateg categ, tobjnum objnum, int privtype)
// privtype==0: READ, 1: write, 2: delete
{ uns8 priv_val[PRIVIL_DESCR_SIZE];
  if (cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, CATEG2SYSTAB(categ), objnum, OPER_GETEFF, priv_val))
    return FALSE;
  if (privtype==0) return HAS_READ_PRIVIL(priv_val,OBJ_DEF_ATR) != 0;
  if (privtype==1) return HAS_WRITE_PRIVIL(priv_val,OBJ_DEF_ATR) != 0 && HAS_READ_PRIVIL(priv_val,OBJ_DEF_ATR) != 0;
    // most object editors need to read the current definition, so I am checking the READ and WRITE privils.
//if (privtype==2)
    return (*priv_val & RIGHT_DEL) != 0;  // READ privil not checked since 8.0.5.2, no reason
}

void update_cached_info(cdp_t cdp)
{ cd_Invalidate_cached_table_info(cdp);
  cd_Relist_objects_ex(cdp, TRUE);
//  UpdateDesktop();
}

CFNC DllKernel bool WINAPI Get_obj_modif_time(cdp_t cdp, tobjnum objnum, tcateg categ, uns32 * ts)
// Reads the time of the last change of the object.
{ return !cd_Read(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_MODIF_ATR, NULL, ts); }
//
// Returns true if Folder contains no objects of given category
//
bool IsFolderEmpty(cdp_t cdp, t_folder_index Folder, tcateg Categ)
{
    comp_dynar *Comp = (comp_dynar *)get_object_cache(cdp, Categ);
    if (!Comp)
        return true;
    for (int i = 0; i < Comp->count(); i++)
         if (Comp->acc(i)->folder == Folder)
             return false;
    return true;
}
//
// Returns true if Folder is empty
//
bool IsFolderEmpty(cdp_t cdp, const char *Folder)
{
    int FolderIndex;
    comp_dynar *Comp = (comp_dynar *)get_object_cache(cdp, CATEG_FOLDER);
    for (FolderIndex = 0; FolderIndex < Comp->count(); FolderIndex++)
        if (wb_stricmp(cdp, Comp->acc(FolderIndex)->name, Folder) == 0)
            break;
    if (FolderIndex >= Comp->count())
        return(true);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_TABLE))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_VIEW))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_CURSOR))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_PROC))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_TRIGGER))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_RELATION))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_SEQ))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_DOMAIN))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_FOLDER))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_PGMSRC))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_MENU))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_PICT))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_DRAWING))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_REPLREL))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_WWW))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_INFO))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_STSH))
        return(false);
    if (!IsFolderEmpty(cdp, FolderIndex, CATEG_XMLFORM))
        return(false);
    return(true);
}
//
// V UTF-8 podle RFC 3629 (www.faqs.org/rfcs/rfc3629.html) nasleduje za znakem
// 110xxxxx znak    10xxxxxx
// 1110xxxx 2 znaky 10xxxxxx 10xxxxxx
// 11110xxx 3 znaky 10xxxxxx 10xxxxxx 10xxxxxx
//
// Na zacatku UTF-8 streamu me bt BOM (byte order mark) sekvence EF BB BF
//
CFNC DllKernel bool WINAPI IsUTF8(const char *Text, int Size)
{
    uns32 BOM = *(uns32 *)Text & 0xFFFFFF;
    if (BOM == UTF_8_BOM)
        return true;

    bool UTF8   = false;
    int  Stat80 = 0;
    if (Size == -1)
        Size = (int)strlen(Text);
    const char *pe = Text + Size;
    for (const char *p = Text; p < pe;)
    {
        char c   = *p++;
        if (Stat80)
        {
            if ((c & 0xC0) != 0x80)
                return(false);
            Stat80--;
        }
        else if (c & 0x80)
        {
            if ((c & 0xE0) == 0xC0)
            {
                UTF8   = true;
                Stat80 = 1;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                UTF8   = true;
                Stat80 = 2;
            }
            else if ((c & 0xF8) == 0xE8)
            {
                UTF8   = true;
                Stat80 = 3;
            }
            else
            {
                return(false);
            }
        }
    }
    return(UTF8 && !Stat80);
}

#include "wbfile.cpp"
