//
// Copy, Paste, Drag&Drop
// 
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

#include "wx/clipbrd.h"
#include "wx/file.h"
#include "wprez9.h"
#include "controlpanel.h"
#include "impexpui.h"
#include "impobjconfl.h"
#include "impexpprog.h"
#include "expschemadlg.h"

#include "impexpprog.cpp"
#include "impobjconfl.cpp"
#include "expschemadlg.cpp"

#ifdef WINS   
//#include <shlobj.h>  // this was necessary with the older Platform SDK
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#define EnterAnotherName     _("%s named \"%s\" already exists. Please use another name.")
#define CannotForODBCTab     _("This action cannot be completed \nsince the table is linked via ODBC")
#define PasteObjFail         _("Failed to paste %s \"%s\"")
#define ImportNoRight        _("You do not have the right to insert objects")
#define EncNoExportMsg       _("Cannot export a protected object without encryption")
#define FileExistsMsg        _("The file %s already exists")
#define FileExistsRewr       _("The file %s already exists, overwrite?")
#define OpenFileErr          _("Failed to open the file %s\n%ErrMsg")
#define ReadFileErr          _("Failed to read the file %s\n%ErrMsg")
#define SchemaExists         _("The imported application already exists")
#define ExportFailed         _("Failed to export %s \"%s\"")

int      LastRecode = 0;
//
// t_impobjctx::warnfunc implementation - Shows import object conflict dialog
//
tiocresult CALLBACK ImportObjectsWarning(unsigned Stat, const wchar_t *Msg, char *ObjName, tcateg Categ, bool Single, void *Param)
{
    if (Stat == IOW_REPLACE)
    {
        CImpObjConflDlg Dlg((cdp_t)Param, Msg, ObjName, Categ, Single);
        return (tiocresult)Dlg.ShowModal();
    }
    return iocNo;
}
//
// Imports object from file, returns true on success
//
bool ImportSelObject(cdp_t cdp, tcateg Categ, const char *FileName, const char *FolderName)
{
    // check the privilege to insert records to system tables:
    uns8 Privils[PRIVIL_DESCR_SIZE];
    if (cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, CATEG2SYSTAB(Categ), NORECNUM, OPER_GETEFF, Privils) || !(*Privils & RIGHT_INSERT))
    {
        error_box(ImportNoRight);
        return(0);
    }

    else
    {
        // Fill import parameters
        char fName[MAX_PATH + 1];
        int  sz = (int)strlen(FileName) + 1;
        memcpy(fName, FileName, sz);
        fName[sz] = 0;
        t_impobjctx imp;
        imp.strsize    = sizeof(t_impobjctx);
        imp.count      = 1;
        imp.categ      = Categ;
        imp.file_names = fName;
        imp.flags      = 0;
        imp.param      = cdp;
        imp.nextfunc   = NULL;
        imp.warnfunc   = ImportObjectsWarning;
        imp.readfunc   = NULL;
        imp.statfunc   = NULL;
        // Import object
        return !Import_objects_ex(cdp, &imp, FolderName);
    }
}
//
// t_impobjctx.statfunc implemantation - Shows error message
//
bool CALLBACK ImportStat(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param)
{
    if (!Stat)
        return true;
    if (Single)
    {
        info_box(wxString(Msg), wxTheApp->GetTopWindow());
        return false;
    }
    else
    {
        return yesno_box(wxString(Msg) + wxT("\n\n") + _("Continue importing?"), wxTheApp->GetTopWindow());
    }
}
//
// Imports set of objects from user selected files, returns number of imported objects
//
unsigned ImportSelObjects(cdp_t cdp, tcateg Categ, const char *Folder)
{
    // check the privilege to insert records to system tables:
    uns8 Privils[PRIVIL_DESCR_SIZE];
    if (cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, CATEG2SYSTAB(Categ), NORECNUM, OPER_GETEFF, Privils) || !(*Privils & RIGHT_INSERT))
    {
        error_box(ImportNoRight);
        return 0;
    }
    // Select files
    wxArrayString FileNames = GetImportFileNames(GetFileTypeFilter(Categ));
    if (FileNames.IsEmpty())
        return 0;
    CBuffer aNames;
    int     sz = 0;
    // Convert UNICODE wxArrayString to ASCII string sequence
    for (int i = 0; i < FileNames.Count(); i++)
    {
        int ssz = (int)FileNames[i].Length() + 1;
        int asz = aNames.GetSize(); 
        if (sz + ssz >= asz)
        {
            if (!aNames.Alloc(asz ? asz * 2 : 512))
            {
                no_memory2(wxGetApp().frame);
                return 0;
            }
        }
        strcpy((char *)aNames + sz, ToChar(FileNames[i], wxConvCurrent));
        sz += ssz;
    }
    aNames[sz] = 0;
    // Fill import parameters
    t_impobjctx imp;
    imp.strsize    = sizeof(t_impobjctx);
    imp.count      = (int)FileNames.Count();
    imp.categ      = Categ;
    imp.file_names = aNames;
    imp.flags      = 0;
    imp.param      = cdp;
    imp.nextfunc   = NULL;
    imp.warnfunc   = ImportObjectsWarning;
    imp.readfunc   = NULL;
    imp.statfunc   = ImportStat;
    // Import objects
    return !Import_objects_ex(cdp, &imp, Folder);
}
//
// t_expobjctx::warnfunc implementation
//
tiocresult CALLBACK FileExistsWarning(char *FileName, const char *ObjName, tcateg Categ, bool Single, void *Param)
{
    wxFileName fName(wxString(FileName, *wxConvCurrent));
    do
    {
        // IF single object, ask user  
        if (Single)
        {
            // IF rewrite, return iocYes
            if (yesno_boxp(FileExistsRewr, NULL, fName.GetFullPath().c_str()))
                return iocYes;
        }
        // ELSE multiple objects, ask user
        else
        {
            // IF rewrite all, return iocYesToAll
            int Res = yesnoall_boxp(_("Yes to all"), FileExistsRewr, NULL, fName.GetFullPath().c_str());
            if (Res == wxID_YESTOALL)
                return iocYesToAll;
            // IF rewrite current file, return iocYes
            if (Res != wxID_NO)
                return iocYes;
        }
        // Answer is not rewrite
        wxString nm = fName.GetName();
        wxChar c;
        // Truncate number at end of file name
        int i = (int)nm.Length();
        do
            c = nm.GetChar(--i); 
        while (i > 0 && c >= '0' && c <= '9');
        nm.Truncate(i +  1);
        // Find first not existing file name with name nm + i
        for (i = 1; ; i++)
        {
            fName = fName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR ) + nm + wxString::Format(wxT("%d"), i) + wxString(wxT('.')) + fName.GetExt();
            if (!fName.FileExists())
                break;
        }

        wxString Filter;
        // Show get file name dialog
        #if 0
        if (Categ == CATEG_PICT)
            Filter = GetPictFilter(PictType);
        else
        #endif
            Filter = GetFileTypeFilter(Categ);
        wxString fn = GetExportFileName(fName.GetFullPath(), Filter, wxGetApp().frame);
        // IF dialog canceled, return iocCancel
        if (fn.IsEmpty())
            return iocCancel;
        fName = fn;
    }
    // BREAK if selected file not exist
    while (fName.FileExists());
    strcpy(FileName, fName.GetFullPath().mb_str(*wxConvCurrent));
    return iocYes;
}
//
// t_expobjctx::statfunc implementation for export objects from control panel
//
bool CALLBACK ExpState(unsigned Stat, const wxChar *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param)
{
    return ImpExpState(((selection_iterator *)Param)->info.cdp, Stat == 0, ExportFailed, Msg, ObjName, Categ, Single);
}
//
// t_expobjctx::statfunc implementation for export ExportSelObject
//
bool CALLBACK ExpSelState(unsigned Stat, const wxChar *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param)
{
    return ImpExpState((cdp_t)Param, Stat == 0, ExportFailed, Msg, ObjName, Categ, Single);
}
//
// Shows error message for export object status
//
bool ImpExpState(cdp_t cdp, bool OK, const wxChar *Fmt, const wxChar *Msg, const char *ObjName, tcateg Categ, bool Single)
{
    if (OK)
        return(true);
    wchar_t wc[64];
    wxString Text = wxString::Format(Fmt, CCategList::Lower(cdp, Categ, wc), (const wxChar *)TowxChar(ObjName, cdp->sys_conv));
    Text.Append(wxT("\n\n"));
    Text.Append(Msg);
    if (Single)
    {
        error_box(Text);
        return(false);
    }
    Text.Append(wxT("\n\n"));
    Text.Append(ContinueMsg);
    return(yesno_box(Text));
}
//
// Exports single objekt to user selected file, returns true on success
//
bool ExportSelObject(cdp_t cdp, const char *objname, tcateg Categ, const char *Schema)
{
    tobjnum objnum;
    // IF schema not specified, use current schema
    if (!Schema)
        Schema = cdp->sel_appl_name;
    // Get exported object objnum
    if (cd_Find_prefixed_object(cdp, Schema, objname, Categ, &objnum))
    {
        cd_Signalize(cdp);
        return(false);
    }
    // exporting ODBC tables is possible but they cannot be imported directly
    if (Categ == CATEG_TABLE && IS_ODBC_TABLE(objnum))
    {
        unpos_box(CannotForODBCTab);
        return(false);
    }
    // Check encrypted object
    if (!not_encrypted_object(cdp, Categ, objnum))
    {
        unpos_box(EncNoExportMsg);
        return(false);
    }
    wxString Filter;
    const char *Suffix;
#if 0
    // IF picture, set file type and file type filter, by picture type
    if (Categ == CATEG_PICT)
    {
        int PictType = GetPictureType(OBJ_TABLENUM, objnum, OBJ_DEF_ATR, NOINDEX);
        Filter = GetPictFilter(PictType);
        Suffix = GetPictSuffix(PictType);
    }
    else
#endif    
    // set file type and file type filter by object category
    {
        Filter = GetFileTypeFilter(Categ);
        Suffix = GetSuffix(Categ);
    }
    char aName[MAX_PATH];
    wxString filepatt(object_fname(cdp, aName, objname, Suffix), *wxConvCurrent);
    wxString fname;
    // Open get file name dialog
    fname = GetExportFileName(filepatt, Filter, wxGetApp().frame);
    if (fname.IsEmpty())
        return(false);
    // Fill export parameters
    tccharptr FileName = fname.mb_str(*wxConvCurrent); 
    t_expobjctx exp;
    exp.strsize   = sizeof(t_expobjctx);
    exp.count     = 1;
    exp.obj_name  = objname;
    exp.categ     = Categ;
    exp.file_name = FileName;
    exp.flags     = EOF_WITHDESCRIPTOR;
    exp.warnfunc  = FileExistsWarning;
    exp.statfunc  = ExpSelState;
    exp.param     = cdp;
    strcpy(exp.schema, Schema);
    // Export object
    return Export_objects(cdp, &exp) != 0;
}
//
// Schema export, returns true on success
//
bool ExportSchema(cdp_t cdp, const char *SchemaName)
{ 
    apx_header apx;
    apx.appl_locked = FALSE;
    tobjnum ApplObj;
    // Get schema objnum
    if (cd_Find2_object(cdp, SchemaName, NULL, CATEG_APPL, &ApplObj))
    {
        cd_Signalize(cdp);
        return false;
    }
    // Read schema header
    if (cd_Read_var(cdp, OBJ_TABLENUM, (trecnum)ApplObj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &apx, NULL))
    {
        cd_Signalize(cdp);
        return false;
    }
    // Show schema export dialog
    char aName[MAX_PATH];
    CExpSchemaDlg Dlg(wxString(SchemaName, *cdp->sys_conv), wxString(object_fname(cdp, aName, SchemaName, ".apl"), *wxConvCurrent), apx.appl_locked != FALSE);
    if (Dlg.ShowModal() != wxID_OK)
        return false;
    tccharptr afName = ToChar(Dlg.GetPath(), wxConvCurrent);
    // Create export progress dialog
    CImpExpProgDlg ProgDlg(cdp, wxGetApp().frame, _("Exporting application"), false);
    // Fill export parameters
    t_export_param ep;
    ep.overwrite = true;
    ep.date_limit        = Dlg.m_ChangedSince;
    ep.with_data         = Dlg.m_Data;
    ep.with_role_privils = Dlg.m_RolePrivils;
    ep.with_usergrp      = Dlg.m_UserPrivils;
    ep.exp_locked        = Dlg.m_Encrypt;
    ep.file_name         = afName;
    ep.report_progress   = true;
    ep.callback          = &CImpExpProgDlg::Progress;
    ep.param             = &ProgDlg;
    ep.schema_name       = SchemaName;
    // Export schema
    return !Export_appl_param(cdp, &ep);
}
//
// Schema import, returns true on success
//
bool ImportSchema(cdp_t cdp)
{ 
    // Show get file name dialog
    wxString fName = GetImportFileName(wxEmptyString, GetFileTypeFilter(CATEG_APPL), wxGetApp().frame);
    if (fName.IsEmpty())
        return false;
    // Open schema file
    wxFile fl;
    if (!fl.Open(fName))
    {
        error_box(SysErrorMsg(OpenFileErr, fName.c_str()), wxGetApp().frame);
        return false;
    }
    // Read first 80 chars from file
    char Buf[80];
    size_t sz = fl.Read(Buf, sizeof(Buf) - 1);
    if (sz == wxInvalidOffset)
    {
        error_box(SysErrorMsg(ReadFileErr, fName.c_str()), wxGetApp().frame);
        return false;
    }
    fl.Close();
    Buf[sz] = 0;
    char *p = Buf;
    // IF text is in UTF8
    if (IsUTF8(Buf, (int)sz))
    {
        // IF file starts with BOM, skip BOM
        uns32 BOM = *(uns32 *)p & 0xFFFFFF;
        if (BOM == UTF_8_BOM)
            p += UTF_8_BOMSZ;
        // Convert text to system charset
        superconv(ATT_STRING, ATT_STRING, p, p, t_specif((uns16)sz, 7, 0, 0), t_specif((uns16)sz, cdp->sys_charset, 0, 0), NULL);
    }
    tobjname  ApplName;
    WBUUID    ApplUUID;
    bool      UUIDExists = false;
    tccharptr aName  = ToChar(fName, wxConvCurrent);
    char *u;
    char *d;
    // IF text starts with "Application", get schema name and schema UUID from text
    if (strnicmp(p, "Application", 11) == 0)
    {
        for (p += 11; *p != 0 && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'); p++);
        if (*p != 0)
        {
            d = ApplName;
            if (*p == '`')
            {
                for (p++; *p != 0 && *p != '`';)
                    *d++ = *p++;
            }
            else
            {
                for (p++; *p != 0 && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n';)
                    *d++ = *p++;
            }
            *d = 0;
            for (; *p != 0 && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'); p++);
            if (*p == 'X')
            {
                u = ++p;
                for (; *p != 0 && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'); p++);
                for (; *p != 0 && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n'; p++);
                *p = 0;
                superconv(ATT_STRING, ATT_BINARY, u, (char *)ApplUUID, t_specif(), t_specif(), NULL);
                UUIDExists = true;
            }
        }
    }
    // ELSE get schema name from file name
    else
    {
        fname2objname(aName, ApplName);
    }

    t_import_param ip;
    tobjnum ApplObj;
    // Check if schema name or UUID exists
    bool NameExists = cd_Find2_object(cdp, ApplName, NULL, CATEG_APPL, &ApplObj) == false;
    if (UUIDExists)
    {
        C602SQLQuery Query(cdp);
        Query.Open("SELECT * FROM OBJTAB WHERE Apl_UUID=X\'%s\'", u);
        UUIDExists = Query.Rec_cnt() > 0;
    }

    // IF schema name or UUID exists, ask user
    if (NameExists || UUIDExists)
    {
        CImpObjConflDlg Dlg(cdp, SchemaExists, ApplName, CATEG_APPL, true, true);
        tiocresult res = (tiocresult)Dlg.ShowModal();
        // IF import canceled, exit
        if (res == iocCancel)
            return false;
        // IF replace, set flag for replace
        if (res == iocYes)
            ip.flags |= IMPAPPL_REPLACE;
        // ELSE set flag and name for new instance
        else
        {
            ip.flags |= IMPAPPL_NEW_INST | IMPAPPL_USE_ALT_NAME;
            ip.alternate_name = ApplName;
        }
    }
    // Create progress dialog
    CImpExpProgDlg ProgDlg(cdp, wxGetApp().frame, _("Importing application"), false);
    // Fill import parameters
    ip.file_name         = aName;
    ip.report_progress   = TRUE;
    ip.callback          = &CImpExpProgDlg::Progress;
    ip.param             = &ProgDlg;
    // Import schema
    return Import_appl_param(cdp, &ip) == false;
}
//
// t_delobjctx::warnfunc implementation - not used
//
tiocresult DelObjWarning(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param)
{
    if (Stat == DOW_CONFIRM || Single)
    {
        if (!yesno_box(Msg))
            return iocCancel;
        return iocYes;
    }
    else 
    {
        int Res = yesnoall_box(_("Delete &All"), Msg);
        if (Res == wxID_YES)
            return iocYes;
        if (Res == wxID_YESTOALL)
            return iocYesToAll;
        return iocNo;
    }
}
//
// Asks user for new name and renames object, on success returns true and new name in Name
//
bool RenameObject(cdp_t cdp, char *Name, tcateg Categ)
{
    tobjnum Obj;
    // Get object objnum
    if (cd_Find_prefixed_object(cdp, cdp->sel_appl_name, Name, Categ, &Obj))
    {
        cd_Signalize(cdp);
        return(false);
    }
    // Get object flags
    unsigned Flags = get_object_flags(cdp, Categ, Name);
    // IF table, check system or ODBC table
    if (Categ == CATEG_TABLE)
    {
        if (Obj <= REL_TABLENUM)
        {
            unpos_box(_("System table cannot be renamed"));
            return(false);
        }
        if (Flags & CO_FLAG_ODBC)
        {
            unpos_box(_("ODBC table cannot be renamed"));
            return(false);
        }
    }
    // IF User, check name for Anonymous
    else if (Categ == CATEG_USER && Obj < FIXED_USER_TABLE_OBJECTS)
    {
        unpos_box(_("System user cannot be renamed"));
        return(false);
    }
    // IF group, check system groups
    else if (Categ == CATEG_GROUP && Obj < FIXED_USER_TABLE_OBJECTS)
    {
        unpos_box(_("System group cannot be renamed"));
        return(false);
    }
    // ODBC connection, replication server cannot be renamed
    else if (Categ == CATEG_CONNECTION)
    {
        unpos_box(_("ODBC connection cannot be renamed"));
        return(false);
    }
    else if (Categ == CATEG_SERVER)
    {
        unpos_box(wxT("Replication server cannot be renamed"));
        return(false);
    }
#if 0
    else if (Categ == CATEG_TRIGGER)
    {
        unpos_box(_("Trigger cannot be renamed"));
        return(false);
    }
    else if (Categ == CATEG_DOMAIN)
    {
        unpos_box(_("Domain cannot be renamed"));
        return(false);
    }
#endif
    // Encrypted object cannot be renamed
    if (Flags & CO_FLAG_ENCRYPTED)
    {
        unpos_box(_("Object is encrypted. It cannot be renamed."));
        return(false);
    }
    
    // IF link, get original object
    tobjnum lObj = Obj;
    if (Flags & CO_FLAG_LINK)
    {
        if (cd_Find_prefixed_object(cdp, cdp->sel_appl_name, Name, Categ | IS_LINK, &lObj))
        {
            cd_Signalize(cdp);
            return(false);
        }
    }
    // Check access rights
    if (!check_object_access(cdp, Categ, lObj, 1))
    {
        unpos_box(Get_error_num_textWX(cdp, NO_RIGHT));
        return(false);
    }

    // Get new name
    tobjname NewName;
    strcpy(NewName, Name);
    if (!get_name_for_new_object(cdp, wxGetApp().frame, cdp->sel_appl_name, Categ, _("Enter new name:"), NewName, true))
        return(false);
    // Rename object
    if (Rename_object(cdp, Name, NewName, Categ))
    {
        cd_Signalize(cdp);
        return(false);
    }
    if (cd_Sz_warning(cdp) == RENAME_IN_DEFIN)
        info_box(_("The object must be renamed in the definition as well"));
    // Return new name
    strcpy(Name, NewName);
    return(true);
}

wxDataFormat * DF_602SQLObject = NULL;
//
// wxDataObject::GetPreferredFormat implementation - returns DF_602SQLObject
//
wxDataFormat C602SQLDataObject::GetPreferredFormat(Direction Dir) const
{
    return *DF_602SQLObject;
}
//
// wxDataObject::GetFormatCount implementation - returns 1, DF_602SQLObject is single supported format
//
size_t C602SQLDataObject::GetFormatCount(Direction Dir) const
{
    return(1);
}
//
// wxDataObject::GetAllFormats implementation - returns DF_602SQLObject, it is single supported format
//
void C602SQLDataObject::GetAllFormats(wxDataFormat *Formats, Direction Dir) const
{
    Formats[0] = *DF_602SQLObject;
}
//
// wxDataObject::GetDataSize implementation
//
size_t C602SQLDataObject::GetDataSize(const wxDataFormat &Format) const
{
    if (Format == *DF_602SQLObject)
        return(m_ObjSize);
    return(0);
}

#ifdef WINS
//
// Corrects error in wxDataObject::GetSizeFromBuffer (msw\dataobj.cpp), it calls HeapSize for buffer allocated
// by GlobalAlloc, this craches on Win98.
//
const void* C602SQLDataObject::GetSizeFromBuffer( const void* buffer, size_t* size, const wxDataFormat& format)
{
    *size = GlobalSize(GlobalHandle(buffer));
    return buffer;
}

#endif
//
// wxDataObject::GetDataHere implementation
//
bool C602SQLDataObject::GetDataHere(const wxDataFormat &Format, void *Buf) const
{
    if (Format == *DF_602SQLObject)
    {
        if (!m_ObjData)
            return(false);
        memcpy(Buf, m_ObjData, m_ObjSize);
        return(true);
    }
    return(false);
}
//
// wxDataObject::SetData implementation
//
bool C602SQLDataObject::SetData(const wxDataFormat& Format, size_t Len, const void *Buf)
{
    if (Format == *DF_602SQLObject)
    {
        if (m_ObjData)
            m_ObjData->Free();
        m_ObjData = C602SQLObjects::Allocs((unsigned)Len);
        if (!m_ObjData)
            return(false);
        memcpy(m_ObjData, Buf, Len);
        m_ObjSize = Len;
        return(true);
    }
    return(false);
}
//
// Reads C602SQLObjects from clipboard
//
C602SQLObjects *C602SQLDataObject::GetDataFromClipboard()
{
    C602SQLObjects *Result = NULL;
    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->GetData(*this))
            Result = m_ObjData;
        wxTheClipboard->Close();
    }
    return(Result);
}
//
// Initializes list of copied object
//
void C602SQLObjects::Init(cdp_t cdp, const char *Schema, const char *Folder, bool Cut)
{
    m_CharSet = cdp->sys_charset;                   // Charset
    strcpy(m_Server, cdp->locid_server_name);       // Server
    if (Schema && *Schema)                          // Schema
        strcpy(m_Schema, Schema);
    else
        strcpy(m_Schema, cdp->sel_appl_name);
    if (Folder && *Folder)                          // Folder
        strcpy(m_Folder, Folder);
    else
        *m_Folder = 0;
    m_Flags = 0;                                    // Flags
    if (Cut)
        m_Flags |= CBO_CUT;
    m_SrcThread = wxThread::GetCurrentId();         // SrcThread
}
//
// Returns true if C602SQLObjects contains only objects that could be placed only in root folder
//
bool C602SQLObjects::RootOnlyObjects() const
{
    int co = 0;
    for (unsigned i = 0; i < m_Count; i++)
    {
        if (m_Obj[i].m_Categ != CATEG_FOLDER)
        {
            if (CCategList::HasFolders(m_Obj[i].m_Categ))
                return(false);
            co++;
        }
    }
    return(co != 0);
}
//
// Returns true if C602SQLObjects contains objetts of given category
//
bool C602SQLObjects::Contains(CCateg Categ) const
{
    for (unsigned i = 0; i < m_Count; i++)
    {
        if (m_Obj[i].m_Categ == Categ)
            return(true);
    }
    return(false);
}
//
// Returns true if C602SQLObjects contains only objetts of given category
//
bool C602SQLObjects::ContainsOnly(CCateg Categ) const
{
    for (unsigned i = 0; i < m_Count; i++)
    {
        if (m_Obj[i].m_Categ != Categ && m_Obj[i].m_Categ != CATEG_FOLDER)
            return false;
    }
    return true;
}
//
//
//
bool C602SQLObjects::ContainsFolder(cdp_t cdp, const char *Folder, bool DestToo) const
{
    const char *ParFolder;
    CObjectList ol(cdp);
    if (DestToo && wb_stricmp(cdp, Folder, m_Folder) == 0)
        return(true);
    for (unsigned i = 0; i < m_Count; i++)
    {
        if (m_Obj[i].m_Categ != CATEG_FOLDER)
            continue;
        if (!ol.Find(m_Obj[i].m_ObjName, CATEG_FOLDER))
            continue;
        ParFolder = ol.GetFolderName();
        if (wb_stricmp(cdp, Folder, ParFolder) == 0 || ol.IsSubFolder(Folder, m_Obj[i].m_ObjName))
            return(true);
    }
    return(false);
}
//
// Copies single object to clipboard
//
bool CopySelObject(cdp_t cdp, const char *ObjName, CCateg Categ, const char *FolderName, const char *SchemaName, bool Cut)
{
    // Create copy context
    CCopyObjCtx Ctx(cdp, SchemaName);
    // Store object to DataObject
    C602SQLDataObject *dobj = Ctx.Copy(ObjName, Categ, FolderName, SchemaName, Cut);
    if (!dobj)
        return(false);
    // Save DataObject to clipboard
    if (!wxTheClipboard->Open())
    {
        delete dobj;
        return(false);
    }
    wxTheClipboard->SetData(dobj);
    wxTheClipboard->Close();
    return(true);
}
//
// Copies set of objects to clipboard
//
bool CopySelObjects(cdp_t cdp, const t_copyobjctx *CopyCtx)
{
    // Create copy context
    CCopyObjCtx Ctx(cdp, CopyCtx->schema);
    // Store object set to DataObject
    C602SQLDataObject *dobj = Ctx.Copy(CopyCtx);
    if (!dobj)
        return(false);
    // Save DataObject to clipboard
    if (!wxTheClipboard->Open())
    {
        delete dobj;
        return(false);
    }
    wxTheClipboard->SetData(dobj);
    wxTheClipboard->Close();
    return(true);
}
//
// Creates DataObject from list of copied objets
//
C602SQLDataObject *CCopyObjCtx::Copy(const t_copyobjctx *CopyCtx)
{
    char    ObjName[256];
    // Allocate memory for object list
    m_Objs = C602SQLObjects::Allocc(CopyCtx->count);
    if (!m_Objs)
    {
        client_error(m_cdp, OUT_OF_APPL_MEMORY);
        return(NULL);
    }
    // Initialize object list
    m_Objs->Init(m_cdp, CopyCtx->schema, CopyCtx->folder, (CopyCtx->flags & COF_CUT));
    unsigned i;
    CCateg Categ;
    for (i = 0; CopyCtx->nextfunc(ObjName, &Categ, CopyCtx->param); i++)
    {
        // IF Whole folder, write folder content to list
        if (Categ.m_Categ == CATEG_FOLDER || Categ.m_Categ == NONECATEG)
        {
            if (!CopyFolder(ObjName))
                return(NULL);
        }
        // IF all objects of given category in the folder
        else if (Categ.m_Opts & CO_ALLINFOLDER)
        {
            // IF folder too
            if ((Categ.m_Opts & CO_FOLDERTOO) && *ObjName)
            {
                // Get folder objnum
                if (!m_ol.Find(ObjName, CATEG_FOLDER))
                    return(NULL);
                // Write folder to list
                if (!CopyObject(ObjName, m_ol.GetObjnum(), CATEG_FOLDER))
                    return(NULL);
            }
            // Write objects to list
            if (!CopyCateg(ObjName, Categ))
                return(NULL);
        }
        // ELSE object
        else
        {
            // Get object objnum
            if (!m_ol.Find(ObjName, Categ))
                return(NULL);
            // Write object to list
            CopyObject(ObjName, m_ol.GetObjnum(), Categ.m_Categ);
        }
    }
    // Create new data object
    C602SQLDataObject *dobj = new C602SQLDataObject(m_Objs, m_Objs->GetSize());
    if (!dobj)
    {
        client_error(m_cdp, OUT_OF_APPL_MEMORY);
        return(NULL);
    }
    // Clear object list pointer to not be released in C602SQLDataObject destructor
    m_Objs = NULL; 
    return(dobj);
}
//
// Creates DataObject from single copied objet
//
C602SQLDataObject *CCopyObjCtx::Copy(const char *ObjName, CCateg Categ, const char *FolderName, const char *SchemaName, bool Cut)
{
    // Allocate memory for object list
    m_Objs = C602SQLObjects::Allocc(1);
    if (!m_Objs)
    {
        client_error(m_cdp, OUT_OF_APPL_MEMORY);
        return(NULL);
    }
    // Get object objnum
    if (!m_ol.Find(ObjName, (Categ.m_Categ == NONECATEG || Categ.m_Opts & CO_ALLINFOLDER) ? CATEG_FOLDER : Categ.m_Categ))
         return(NULL);
    // Initialize object list
    m_Objs->Init(m_cdp, SchemaName, FolderName, Cut);
    // IF whole folder
    if (Categ.m_Categ == CATEG_FOLDER || Categ.m_Categ == NONECATEG)
    {
        // Write all objects in folder to list
        if (!CopyFolder(ObjName))
            return(NULL);
    }
    // IF category or folder below category
    else if (Categ.m_Opts & CO_ALLINFOLDER)
    {
        // IF folder too
        if ((Categ.m_Opts & CO_FOLDERTOO) && *ObjName)
        {
            // Write folder to list
            if (!CopyObject(ObjName, m_ol.GetObjnum(), CATEG_FOLDER))
                return(NULL);
        }
        // Write objects to list
        if (!CopyCateg(ObjName, Categ))
            return(NULL);
    }
    // ELSE object
    else
    {
        // Write object to list
        CopyObject(ObjName, m_ol.GetObjnum(), Categ.m_Categ);
    }
    // Create DataObject
    C602SQLDataObject *dobj = new C602SQLDataObject(m_Objs, m_Objs->GetSize());
    if (!dobj)
    {
        client_error(m_cdp, OUT_OF_APPL_MEMORY);
        return(NULL);
    }
    // Clear object list pointer to not be released in C602SQLDataObject destructor
    m_Objs = NULL;
    return(dobj);
}
//
// Writes single object description to DataObject object list
//
bool CCopyObjCtx::CopyObject(const char *ObjName, tobjnum ObjNum, tcateg Categ)
{
    C602SQLObjName *on = C602SQLObjects::GetItem(&m_Objs, m_Objs->m_Count++);   // Get descriptor pointer
    if (!on)
    {
        client_error(m_cdp, OUT_OF_APPL_MEMORY);
        return(false);
    }
    strncpy(on->m_ObjName, ObjName, OBJ_NAME_LEN);                              // Name
    on->m_ObjName[OBJ_NAME_LEN] = 0;                                
    on->m_Categ  = Categ;                                                       // Category
    on->m_ObjNum = ObjNum;                                                      // objnum
    return(true);
}
//
// Writes all objects of given category from given folder description to DataObject object list
//
bool CCopyObjCtx::CopyCateg(const char *FolderName, CCateg Categ)
{
    bool Found;
    // FOR each object of given from given folder, write object description
    for (Found = m_ol.FindFirst(Categ, FolderName); Found; Found = m_ol.FindNext())
    {
        if (!CopyObject(m_ol.GetName(), m_ol.GetObjnum(), Categ.m_Categ))
            return(false);
    }
    // FOR each subfolder in given folder
    CObjectList fl(m_cdp, m_Objs->m_Schema);
    for (Found = fl.FindFirst(CATEG_FOLDER, FolderName); Found; Found = fl.FindNext())
    {
        unsigned Index = m_Objs->m_Count;
        // Write folder desription to list
        if (!CopyObject(fl.GetName(), fl.GetObjnum(), CATEG_FOLDER))
            return(false);
        // Write objects from subfolder to list
        if (!CopyCateg(fl.GetName(), Categ))
            return(false);
        // IF folder is empty, remove folder description from list
        if (m_Objs->m_Count <= Index + 1)
            m_Objs->m_Count = Index;
    }
    return(true);
}
//
// Writes all objects of all categories from given folder description to DataObject object list
//
bool CCopyObjCtx::CopyFolder(const char *FolderName)
{
    // Get folder objnum
    if (!m_ol.Find(FolderName, CATEG_FOLDER))
        return(false);
    // Write folder desription to list
    if (!CopyObject(FolderName, m_ol.GetObjnum(), CATEG_FOLDER))
        return(false);
    // FOR each category
    for (tcateg Categ = 0; Categ < CATEG_COUNT; Categ++)
    {
        // IF objects of given category can be copied
        if (CCategList::Exportable(Categ))
            // Write all objects of given category from given folder to list
            if (!CopyCateg(FolderName, Categ))
                return(false);
    }
    return(true);
}
//
// Pastes objects DataObject object list to database
//
void CPasteObjCtx::PasteObjects(C602SQLObjects *objs)
{
    m_Objs = objs;
    // IF schema not specified, set current schema
    if (!m_dSchema || !*m_dSchema)
        m_dSchema = m_cdp->sel_appl_name;
    // IF copy from different folder
    if (stricmp(m_Objs->m_Server, m_cdp->locid_server_name) != 0)
    {
        // Copy from different process not supported
        if (m_Objs->m_SrcThread != wxThread::GetCurrentId())
            return;
        CObjectList ol;
        // EXIT IF source server not found or not active
        if (!ol.Find(m_Objs->m_Server, XCATEG_SERVER))
            return;
        // Get source cdp
        m_scdp = ((t_avail_server *)ol.GetSource())->cdp;
        if (!m_scdp)
            return;
    }
    // ELSE same server
    else
    {
        // Source cdp is my cdp
        m_scdp = m_cdp;
        // IF copy from same schema
        if (wb_stricmp(m_cdp, m_Objs->m_Schema, m_dSchema) == 0)
        {
            // IF copy from same folder, duplicate objects
            if (wb_stricmp(m_cdp, m_Objs->m_Folder, m_dFolder) == 0)
                m_Duplicate = true;
            // ELSE move objects to current folder
            else
            {
                m_dfl.SetCdp(m_cdp);
                MoveToFolder();
                    ControlPanel()->RefreshSchema(m_cdp->locid_server_name, m_dSchema);
                return;
            }
        }
    }

    // IF destination schema is not current schema, set temporarily the destination schema
    unsigned Pasted;
    { t_temp_appl_context tac(m_cdp, m_dSchema, 1);
      if (tac.is_error())
        { cd_Signalize(m_cdp);  return; }
      // Reset source and destination folder ObjectList
      m_sfl.SetCdp(m_scdp, m_Objs->m_Schema);
      m_dfl.SetCdp(m_cdp);
      // Import objects from list
      Pasted = Import();
    }  
    // IF some object copied AND cut operation, delete source objects
    if (Pasted && (m_Objs->m_Flags & CBO_CUT))
    {
        t_delobjctx ctx;
        memset(&ctx, 0, sizeof(t_delobjctx));
        ctx.strsize  = sizeof(t_delobjctx);
        ctx.count    = m_Objs->m_Count;  
        ctx.flags    = DOF_NOCONFIRM;
        strcpy(ctx.schema, m_Objs->m_Schema);
        ctx.param    = this;  
        ctx.nextfunc = Next;
        m_Index      = m_Objs->m_Count;
        Delete_objects(m_scdp, &ctx);
    }
    // IF some object copied, refresh control panel
    if (Pasted)
        ControlPanel()->RefreshSchema(m_cdp->locid_server_name, m_dSchema);
}
//
// Returns next object description from copied object list for paste
//
tnextimpres CPasteObjCtx::Next(char *ObjName, tcateg *Categ, char *FolderName, uns32 *Modif, uns32 *Size)
{
    tobjname FolderBuf;
    for (; m_Index < m_Objs->m_Count; m_Index++)
    {
        // Get next object descriptor
        C602SQLObjName *on = &m_Objs->m_Obj[m_Index];
        // IF folder
        if (on->m_Categ == CATEG_FOLDER)
        {
            // Find or create destination folder
            const char *DestFolder = GetFolder(on->m_ObjName, FolderBuf);
            if (!DestFolder)
            {
                if (!CallState(cd_Sz_error(m_scdp), NULL, on->m_ObjName, CATEG_FOLDER))
                    return NEXT_END;
            }
            // Continue with next object
            continue;
        }
        // Fulltext cannot by copied from different schema, continue with next object
        if (on->m_Categ == CATEG_INFO)
            continue;
        // Output object name
        Copy(ObjName, on->m_ObjName);
        // Output object category
        *Categ = on->m_Categ;
        // Get current object definition
        char *Defin = cd_Load_objdef(m_scdp, on->m_ObjNum, on->m_Categ, NULL);
        if (!Defin)
        {
            if (!CallState(cd_Sz_error(m_scdp), NULL, on->m_ObjName, on->m_Categ))
                return NEXT_END;
            continue;
        }
        try
        {
            int sz = (int)strlen(Defin);
            // IF text definition, convert to UTF8
            if (CCategList::HasTextDefin(on->m_Categ))
            {
                m_Defin.AllocX(sz * 2);
                ToUTF8(m_scdp, Defin, m_Defin, sz * 2);
            }
            else
            {
                m_Defin.AllocX(sz + 1);
                strcpy(m_Defin, Defin);
            }
            corefree(Defin);
        }
        catch (...)
        {
            corefree(Defin);
            if (!CallState(SQ_INV_CHAR_VAL_FOR_CAST, NULL, on->m_ObjName, on->m_Categ))
                return NEXT_END;
            continue;
        }
        // Output object definition size
        *Size = (uns32)strlen(m_Defin);
        tobjname Folder;
        // Get source object folder name
        if (cd_Read_ind(m_scdp, CATEG2SYSTAB(on->m_Categ), on->m_ObjNum, OBJ_FOLDER_ATR, NOINDEX, Folder))
        {
            if (!CallState(cd_Sz_error(m_scdp), NULL, on->m_ObjName, on->m_Categ))
                return NEXT_END;
            continue;
        }
        // Find or create destination folder
        const char *DestFolder = GetFolder(Folder, FolderBuf);
        if (!DestFolder)
        {
            if (!CallState(cd_Sz_error(m_scdp), NULL, Folder, CATEG_FOLDER))
                return NEXT_END;
            continue;
        }
        // Output folder name
        Copy(FolderName, DestFolder);
        m_Index++;
        return NEXT_CONTINUE;
    }
    return NEXT_END;
}
//
// Finds or creates destination folder
// Input:   SrcFolder   - Source object folder name
//          DstFolder   - Buffer for destination object folder name
// Returns: Destination object folder name
//
const char *CPasteObjCtx::GetFolder(const char *SrcFolder, char *DstFolder)
{
    tobjname    dParent;
    const char *Parent;
    // IF source folder is root OR source object belongs to source folder, object will be copied to destination folder
    if (!SrcFolder || !*SrcFolder || wb_stricmp(m_scdp, SrcFolder, m_Objs->m_Folder) == 0)
        return(m_dFolder);
    // ELSE object will be copied to folder of same name
    Copy(DstFolder, SrcFolder);
    // IF this folder exists, done
    if (m_dfl.Find(DstFolder, CATEG_FOLDER))
        return(DstFolder);
    // IF source object folder in source schema not found, parent of resulting folder will be destination folder
    if (!m_sfl.Find(SrcFolder, CATEG_FOLDER))
        Parent = m_dFolder;
    else 
    {
        // Find parent folder in source schema
        const char *sParent = m_sfl.GetFolderName();
        Parent = GetFolder(sParent, dParent);
        if (!Parent)
            return(NULL);
    }
    // Create new folder
    if (Create_folder(m_cdp, DstFolder, Parent))
        return(NULL);
    return(DstFolder);
}
//
// Reads copied object definition
//
unsigned CPasteObjCtx::Read(char *Defin, unsigned Size)
{
    strcpy(Defin, m_Defin);
    return(Size);
}
//
// Shows paste object error message
//
bool CPasteObjCtx::State(unsigned Stat, const wxChar *Msg, const char *ObjName, tcateg Categ)
{
    if (Stat == 0)
        return(true);
    wchar_t wc[64];
    wxString Text = wxString::Format(PasteObjFail, CCategList::Lower(m_cdp, Categ, wc), (const wxChar *)TowxChar(ObjName, wxConvCurrent));
    Text.Append(wxT("\n\n"));
    Text.Append(Msg);
    // IF single object, show message 
    if (m_Single)
    {
        error_box(Text);
        return(false);
    }
    // ELSE multiple objects, show message and ask user if to continue
    Text.Append(wxT("\n\n"));
    Text.Append(ContinueMsg);
    return(yesno_box(Text));
}
//
// Move from folder to folder within same schema
//
void CPasteObjCtx::MoveToFolder()
{
    CObjectList    ol(m_cdp);
    // dfi = Destination folder index
    if (!m_dfl.Find(m_dFolder, CATEG_FOLDER))
        return;
    t_folder_index dfi = m_dfl.GetIndex();
    // sfi = Source folder index
    if (!m_dfl.Find(m_Objs->m_Folder, CATEG_FOLDER))
        return;
    t_folder_index sfi = m_dfl.GetIndex();

    // FOR each object in list
    for (unsigned i = 0; i < m_Objs->m_Count; i++)
    {
        // Get object descriptor
        C602SQLObjName *on = &m_Objs->m_Obj[i];
        // IF object allready exists, continue
        if (!ol.Find(on->m_ObjName, on->m_Categ))
            continue;
        // IF object not belongs to souce folder, nothing to do
        if (ol.GetFolderIndex() != sfi)
            break;
        // IF Object
        if (on->m_Categ != CATEG_FOLDER)
        {
            // IF object category cannot have folders, continue
            if (!CCategList::HasFolders(on->m_Categ))
                continue;
            // Write new folder name to object definition
            if (Set_object_folder_name(m_cdp, on->m_ObjNum, on->m_Categ, m_dFolder))
            {
                if (!CallState(cd_Sz_error(m_cdp), NULL, on->m_ObjName, on->m_Categ))
                    return;
                continue;
            }
            // Write new folder index to object cache
            t_comp *comp = (t_comp *)ol.GetSource();
            comp->folder = dfi;
        }
        // ELSE Folder
        else
        {
            // IF folder object not exists in database, create it
            if (on->m_ObjNum == NOOBJECT)
            {
                if (Create_folder(m_cdp, on->m_ObjName, m_dFolder))                       
                   if (!CallState(cd_Sz_error(m_cdp), NULL, on->m_ObjName, on->m_Categ))
                        return;
            }
            else
            {
                // Write new parent folder name to folder definition
                if (Set_object_folder_name(m_cdp, on->m_ObjNum, on->m_Categ, m_dFolder))
                {
                    if (!CallState(cd_Sz_error(m_cdp), NULL, on->m_ObjName, on->m_Categ))
                        return;
                    continue;
                }
                // Write new parent folder index to object cache
                t_comp *comp = (t_comp *)ol.GetSource();
                comp->folder = dfi;
            }
        }
    }
}
//
// Returns next object description from copied object list for delete
//
bool wxCALLBACK CPasteObjCtx::Next(char *ObjName, CCateg *Categ, void *Param)
{
    CPasteObjCtx   *ctx = (CPasteObjCtx *)Param;
    while (ctx->m_Index--)
    {
        // Get next object descriptor
        C602SQLObjName *on  = &ctx->m_Objs->m_Obj[ctx->m_Index];
        // IF folder AND folder is not empty, continue
        if (on->m_Categ == CATEG_FOLDER && GetFolderState(ctx->m_scdp, ctx->m_sSchema, on->m_ObjName, true, true) != FST_EMPTY && GetFolderState(ctx->m_scdp, ctx->m_sSchema, on->m_ObjName, CATEG_FOLDER, true, false) != FST_EMPTY)
            continue;
        // Output object name and category
        strcpy(ObjName, on->m_ObjName);
        *Categ = on->m_Categ;
        return(true);
    }
    return(false);
}
//
// Pastes objects from clipboard to Folder in Schema
//
void PasteObjects(cdp_t cdp, const char *Folder, const char *Schema)
{
    C602SQLDataObject Data;
    C602SQLObjects *objs = Data.GetDataFromClipboard();
    if (objs)
    {
        CPasteObjCtx Ctx(cdp, Schema, Folder);
        Ctx.PasteObjects(objs);
    }
}

#ifdef LINUX
//
// Checks if user has acces rights to selected files
//
bool wbFileDialog::Validate()
{
	if (!wxFileDialog::Validate())
		return(false);
    int Style = GetStyle();
    if (Style & wxMULTIPLE)
    {
        wxArrayString fl;
        GetPaths(fl);
        for (int i = 0; i < fl.Count(); i++)
        {
            if (access(fl[i].mb_str(*wxConvCurrent), R_OK))
            {
                error_boxp(_("You do not have read access to the file %s"), this, fl[i].c_str());
                return(false);
            }
        }
    }
    else
    {
        wxString fl = GetPath();
        if (Style & wxSAVE)
        {
            if (wxFileExists(fl))
            {
                if (access(fl.mb_str(*wxConvCurrent), W_OK))
                {
                    error_boxp(_("You do not have write access to the file %s"), this, fl.c_str());
                    return(false);
                }
            }
            else
            {
                if (access(GetDirectory().mb_str(*wxConvCurrent), W_OK))
                {
                    error_boxp(_("You do not have write access to the directory %s"), this, GetDirectory().c_str());
                    return(false);
                }
            }
        }
		else
		{
            if (access(fl.mb_str(*wxConvCurrent), R_OK))
            {
                error_boxp(_("You do not have read access to the file %s"), this, fl.c_str());
                return(false);
            }
		}
    }
	return(true);
}
//
// Checks if user has access right to selected folder
//
bool wbDirDialog::Validate()
{
	if (!wxDirDialog::Validate())
		return(false);
    if (access(GetPath().mb_str(*wxConvCurrent), W_OK))
    {
        error_boxp(_("You do not have write access to the directory %s"), this, GetPath().c_str());
        return(false);
    }
	return(true);
}

#endif
//
// Procedures for file name select
//
// SelectFileNames enambles set of file names selection
// Input:   Path    - Initial path
//          parent  - Parent window
// Returns: Selected file names list, selected folder in Path
//
wxArrayString SelectFileNames(wxString &Path, wxString Filter, wxWindow * parent)
{
    // IF parent not specified, parent = application frame
    if (!parent)
        parent = wxGetApp().frame;
    wxArrayString Result;
    // Show dialog
#if WXWIN_COMPATIBILITY_2_4  // wxHIDE_READONLY is still necessary in the 2.4 compatibility mode
    wbFileDialog fd(NULL, _("Select file"), Path, wxEmptyString, Filter, wxOPEN | wxMULTIPLE | wxHIDE_READONLY);
#else                        // READONLY is hidden by default
    wbFileDialog fd(NULL, _("Select file"), Path, wxEmptyString, Filter, wxOPEN | wxMULTIPLE);
#endif
    
    if (fd.ShowModal() == wxID_OK)
    {
        Path = fd.GetDirectory();
        fd.GetPaths(Result);
    }
    return(Result);
}
//
// Enables single file name selection
// Input:   Path    - Initial path
//          Name    - Initial name
//          Filter  - File type filter
//          ForSave - true - open file for write, false - open file for read
//          parent  - Parent window
//          title   - Dialog title
// Returns: Selected file name, selected folder in Path
//
wxString SelectFileName(wxString &Path, wxString Name, wxString Filter, bool ForSave, wxWindow * parent, wxString title)
{
    // IF parent not specified, parent = application frame
    if (!parent)
        parent = wxGetApp().frame;
    // When [Name] contains path, use this path instead of [Path]:
    wxString StartPath;
    int ind = Name.Find(PATH_SEPARATOR, true);
    if (ind==-1)
      StartPath=Path;
    else 
    { StartPath=Name.Mid(0, ind+1);  // including the separator, importatnt when the path is / only
      Name=Name.Mid(ind+1);
    }
   // run the dialog:     
#if WXWIN_COMPATIBILITY_2_4  // wxHIDE_READONLY is still necessary in the 2.4 compatibility mode
    wbFileDialog fd(parent, title.IsEmpty() ? wxString(_("Select file")) : title, StartPath, Name, Filter, ForSave ? wxSAVE | wxHIDE_READONLY : wxOPEN | wxHIDE_READONLY);
#else                        // READONLY is hidden by default
    wbFileDialog fd(parent, title.IsEmpty() ? wxString(_("Select file")) : title, StartPath, Name, Filter, ForSave ? wxSAVE : wxOPEN);
#endif
    if (fd.ShowModal() == wxID_OK)
    {
        Path = fd.GetDirectory();
        return(fd.GetPath());
    }
    else
    {
        return wxEmptyString;
    }
}

wxString LastImportPath;

//
// Enables selection of single file name for reading, initial folder gets and selected folder stores to LastImportPath
// Input:   InitName - Initial name
//          Filter   - File type filter
//          parent   - Parent window
//          title    - Dialog title
// Returns: Selected file name
//
wxString GetImportFileName(wxString InitName, wxString Filter, wxWindow * parent, wxString title)
{
    // IF LastImportPath not defined, read value from ini file
    if (LastImportPath.IsEmpty())
        LastImportPath = read_profile_string("Directories", "Import Path");
    // Select file name
    wxString FileName = SelectFileName(LastImportPath, InitName, Filter, false, parent, title);
    // Store selected folder to ini file
    if (!FileName.IsEmpty())
        write_profile_string("Directories", "Import Path", LastImportPath.mb_str(*wxConvCurrent));
    return(FileName);
}
//
// Enables selection of set of file names for reading, initial folder gets and selected folder stores to LastImportPath
// Input:   Filter   - File type filter
//          parent   - Parent window
// Returns: Selected file names list
//
wxArrayString GetImportFileNames(wxString Filter, wxWindow * parent)
{
    // IF LastImportPath not defined, read value from ini file
    if (LastImportPath.IsEmpty())
        LastImportPath = read_profile_string("Directories", "Import Path");
    // Select file names
    wxArrayString FileNames = SelectFileNames(LastImportPath, Filter);
    // Store selected folder to ini file
    if (!FileNames.IsEmpty())
        write_profile_string("Directories", "Import Path", LastImportPath.mb_str(*wxConvCurrent));
    return(FileNames);
}

wxString LastExportPath;

//
// Enables selection of single file name for wtiting, initial folder gets and selected folder stores to LastExportPath
// Input:   InitName - Initial name
//          Filter   - File type filter
//          parent   - Parent window
//          title    - Dialog title
// Returns: Selected file name
//
wxString GetExportFileName(wxString InitName, wxString Filter, wxWindow * parent, wxString title)
{
    // IF LastExportPath not defined, read value from ini file
    GetLastExportPath();
    // Select file name
    wxString fname = SelectFileName(LastExportPath, InitName, Filter, true, parent, title);
    // Store selected folder to ini file
    if (!fname.IsEmpty())
        write_profile_string("Directories", "Export Path", LastExportPath.mb_str(*wxConvCurrent));
    return(fname);
}
//
// Enables selection of folder for exporting objects, initial folder gets and selected folder stores to LastExportPath
// Input:   parent   - Parent window
// Returns: Selected file names list
//
wxString GetExportPath(wxWindow *parent)
{
    // IF parent not specified, parent = application frame
    if (!parent)
        parent = wxGetApp().frame;
    // IF LastExportPath not defined, read value from ini file
    GetLastExportPath();
    wxString Path;
    // Show select folder dialog
    wbDirDialog dd(parent, _("Select directory"), LastExportPath, wxDD_NEW_DIR_BUTTON);
    if (dd.ShowModal() == wxID_OK)
    {
        // Store selected folder to ini file
        Path = LastExportPath = dd.GetPath();
        write_profile_string("Directories", "Export Path", LastExportPath.mb_str(*wxConvCurrent));
    }
    return(Path);
}
//
// Returns last folder used for export files
//
wxString GetLastExportPath()
{
    if (LastExportPath.IsEmpty())
        LastExportPath = read_profile_string("Directories", "Export Path");
    return(LastExportPath);
}
//
// IF object exists, shows prompt to enter new name and returns true
//
bool ObjNameExists(cdp_t cdp, const char *ObjName, tcateg Categ)
{
    tobjnum objnum;
    tcateg  cat;
    // IF object not exists, return false
    if (IsObjNameFree(cdp, ObjName, Categ, &objnum, &cat))
        return(false);
    // IF error, show error message
    if (objnum == NOOBJECT && Categ != CATEG_FOLDER)
    {
        cd_Signalize(cdp);
        return(true);
    }
    wchar_t wc[64];
    // S
    error_box(wxString::Format(EnterAnotherName, CCategList::CaptFirst(cdp, cat, wc), (const wxChar *)TowxChar(ObjName, wxConvCurrent)));
    return(true);
}
//
// Returns file type filter for given category
//
wxString GetFileTypeFilter(tcateg Categ)
{
    return(Categ < CATEG_COUNT ? wxString(CCategList::FileTypeFilter(Categ)) + FilterAll : wxString(FilterAll));
}

