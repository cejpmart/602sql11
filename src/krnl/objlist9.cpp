// objlist.cpp
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#include "enumsrv.h"

#ifdef WINS
#include "regstr.h"
#endif
#include "objlist9.h"

#include "bmps/diDiagramms.xpm"
#include "bmps/diDomains.xpm"
#include "bmps/diFolders.xpm"
#include "bmps/diForms.xpm"
#include "bmps/diProcedures.xpm"
#include "bmps/diProgramms.xpm"
#include "bmps/diQueries.xpm"
#include "bmps/diRoles.xpm"
#include "bmps/diSequences.xpm"
#include "bmps/diTables.xpm"
#include "bmps/diTriggers.xpm"
#include "bmps/_full.xpm"
#include "bmps/xmlform.xpm"
//#include "../mpsql/bmps/stylesh.xpm"

/* The object list logic:
Every object list is created for a given xcdp and schema. Every engine has a fixed xcdp, schema and category. 
When xcdp or schema is changed in the object list by SetCdp(), the engine is dropped.
When starting any object list operation and the engine exists, the categories are compared and if they are equal,
  the engine us re-used by Reset(). Otherwise new engine is created by Init().
*/
#ifdef LINUX
#define SQLTablesA SQLTables
#endif

#ifdef NEVER

t_comp SysTable [] =
{
    {"TabTab",  TAB_TABLENUM,  0, FOLDER_ROOT, 0},
    {"ObjTab",  OBJ_TABLENUM,  0, FOLDER_ROOT, 0},
    {"UserTab", USER_TABLENUM, 0, FOLDER_ROOT, 0},
    {"SrvTab",  SRV_TABLENUM,  0, FOLDER_ROOT, 0},
    {"ReplTab", REPL_TABLENUM, 0, FOLDER_ROOT, 0},
    {"KeyTab",  KEY_TABLENUM,  0, FOLDER_ROOT, 0},
//    {"__RelTab", REL_TABLENUM,  0, FOLDER_ROOT, 0}, -- hidden
//    {"__PropTab",PROP_TABLENUM, 0, FOLDER_ROOT, 0}
};

#endif

LPGETSERVERLIST CObjectList::GetServerList;
//
// Constructor
//
CObjectList::CObjectList(const t_avail_server * as, const char *Schema)
{
    m_cdp      = as->cdp;
    m_conn     = as->conn;
    m_eng      = NULL;
    m_Schema   = Schema;
    m_MySchema = !Schema || !*Schema || m_cdp && wb_stricmp(m_cdp, Schema, m_cdp->sel_appl_name) == 0;
   *m_SelFolderName = 0;
}
//
// Sets cdp, schema name and releases old search engine
//
void CObjectList::SetCdp(const t_avail_server * as, const char *Schema)
{
    m_cdp      = as->cdp;
    m_conn     = as->conn;
    m_Schema   = Schema;
    m_MySchema = !Schema || m_cdp && wb_stricmp(m_cdp, Schema, m_cdp->sel_appl_name) == 0;
    Release();
}
//
// Initializes object, sets search engine
//
bool CObjectList::Init(CCateg Categ)
{
    comp_dynar *Fld = NULL;
    Release();          // Releases old serch engine
    Reset();            // Resets current object descriptor
    m_SelFolder = FOLDER_ANY;
    // IF Categ is server or ODBC data source, set engine for servers
    if (Categ.m_Categ == XCATEG_SERVER || Categ.m_Categ == XCATEG_ODBCDS)
    {
        m_SelFolder = FOLDER_ROOT;
        m_eng       = new COLServ(Categ);
    }
    // IF ODBC data source scan, set engine for ODBC data source objects
    else if (m_conn)
    { 
        m_SelFolder = FOLDER_ROOT;
        if (Categ == CATEG_FOLDER)
            return true;
        COLODBC * eng = new COLODBC(Categ);
        if (Categ.m_Categ != CATEG_APPL) //(m_Schema && *m_Schema)  -- m_Schema may be empty in some data sources
        {
            bool use_qualif, use_schema;
            if (m_conn->special_flags & 2)
            {
                use_qualif = false;
                use_schema = true;
            }
            else
            {
                use_qualif = m_conn->qualifier_usage && !m_conn->owner_usage;  
                use_schema = m_conn->owner_usage != 0;
            }
            eng->fill(m_conn, 3, use_qualif ? (char*)m_Schema : NULL, use_schema ? (char*)m_Schema : NULL, Categ);
        }
        else
	    { 
#if 0  // MS SQL uses both, but qualifier is fixed in a data source
            if (m_conn->qualifier_usage)
                eng->fill(m_conn, 1,  SQL_ALL_CATALOGS, NULLSTRING, Categ);  // Categ ignore here
            else
                eng->fill(m_conn, 2,  NULLSTRING, SQL_ALL_SCHEMAS, Categ);  // Categ ignore here
#else
            if (m_conn->owner_usage)
                eng->fill(m_conn, 2,  NULLSTRING, SQL_ALL_SCHEMAS, Categ);  // Categ ignore here
            else
                eng->fill(m_conn, 1,  SQL_ALL_CATALOGS, NULLSTRING, Categ);  // Categ ignore here
#endif
        }
        m_eng = eng;
    }
    // IF current schema
    else if (m_MySchema)
    {
        // Get folder cache and selected folder INDEX
        Fld = (comp_dynar *)get_object_cache(m_cdp, CATEG_FOLDER);
        if (!AnyFolder())
        {
            unsigned int fi;
            if (!*m_SelFolderName)
                m_SelFolder = FOLDER_ROOT;
            else if (comp_dynar_search(Fld, m_SelFolderName, &fi))
                m_SelFolder = fi;
        }
        // IF cached category, set engine for cached objects
        comp_dynar *Comp = (comp_dynar *)get_object_cache(m_cdp, Categ.m_Categ);
        if (Comp)
            m_eng = new COLCache(m_cdp, Comp, Fld, Categ);
    }
    // IF engine not set
    if (!m_eng)
    {
        WBUUID appl_id;
        if (!m_MySchema)
            cd_Apl_name2id(m_cdp, m_Schema, appl_id);
        char *Info;
        bool Extended = !(Categ == CATEG_USER || Categ == CATEG_GROUP || Categ == CATEG_SERVER || Categ == CATEG_ROLE);
        // IF Categ is folder, create folder list
        if (Categ == CATEG_FOLDER)
        {
            char Query[1024];
            char sUUID[2 * UUID_SIZE + 1];
            bin2hex(sUUID, appl_id, UUID_SIZE);
            sUUID[2 * UUID_SIZE] = 0;
            strcpy(Query, "SELECT ObjName, Folder_name FROM ");
            strcat(Query, "(SELECT Obj_name AS ObjName FROM Objtab WHERE Category=chr(19) AND Obj_name<>'.' AND Apl_uuid=X'");
            strcat(Query, sUUID);
            strcat(Query, "' UNION SELECT DISTINCT Folder_name AS ObjName FROM Objtab WHERE Folder_name<>'' AND Folder_name<>'.' AND Apl_uuid=X'");
            strcat(Query, sUUID);
            strcat(Query, "' UNION SELECT DISTINCT Folder_name AS ObjName FROM Tabtab WHERE Folder_name<>'' AND Folder_name<>'.' AND Apl_uuid=X'");
            strcat(Query, sUUID);
            strcat(Query, "') LEFT JOIN "); 
            strcat(Query, "(SELECT obj_name, Folder_name FROM Objtab WHERE Category = Chr(19) AND folder_name<>'.' AND Apl_uuid=X'");
            strcat(Query, sUUID);
            strcat(Query, "') ON ObjName=obj_name ");
            strcat(Query, "ORDER BY Folder_name, Objname");

            tcursnum Curs;
            if (cd_Open_cursor_direct(m_cdp, Query, &Curs))
                return false;
            trecnum rCnt;
            cd_Rec_cnt(m_cdp, Curs, &rCnt);
            Info = (char *)corealloc(rCnt * (sizeof(tobjname) + sizeof(tobjname) + sizeof(uns32) + sizeof(tobjnum) + sizeof(uns16)) + 1, 87);
            if (!Info)
            {
                client_error(m_cdp, OUT_OF_APPL_MEMORY);
                cd_Close_cursor(m_cdp, Curs);
                return(false);
            }
            char *p = Info;
            for (trecnum Pos = 0; Pos < rCnt; Pos++)
            {
                cd_Read_ind(m_cdp, Curs, Pos, 1, NOINDEX, p);
                p += strlen(p) + 1;
                cd_Read_ind(m_cdp, Curs, Pos, 2, NOINDEX, p);
                p += strlen(p) + 1;
                *(uns32 *)p = 0;
                p += sizeof(uns32);
                *(tobjnum *)p = NOOBJECT;
                p += sizeof(tobjnum);
                *(uns16 *)p = 0;
                p += sizeof(uns16);
            }
            *p = 0;
            cd_Close_cursor(m_cdp, Curs);
        }
        // ELSE all other objects, get object list from server
        else
        {
            if (cd_List_objects(m_cdp, !Extended ? Categ.m_Categ : Categ.m_Categ | IS_LINK, m_MySchema ? NULL : appl_id, &Info))
                return false;
        }
        // Create object list engine
        m_eng = new COLDB(m_cdp, Info, Fld, Extended, Categ);
    }
    return true;
}
//
// Finds first object of given Category in given foder
// Returns true if found
//
bool CObjectList::FindFirst(CCateg Categ, const char *Folder)
{
    // Reset folder
    ChangeFolder(Folder);
    // IF category not changed, reset current object index
    if (m_eng && Categ == m_eng->GetMyCateg())
        m_eng->Reset();
    // ELSE reset search engine
    else if (!Init(Categ))
         return(false);
    // Find object
    return(FindNext());
}
//
// Finds object
// Returns true if found
//
bool CObjectList::FindNext()
{
    // Get next object in the list until selected object is in given folder
    do 
    {
        if (!m_eng->Next(this))
            return(false);
    }
    while (!IsFromSelFolder());
    return(true);
}
//
// Finds given object is in given folder
// Returns true if found
//
bool CObjectList::Find(const char *Name, CCateg Categ, const char *Folder)
{
    // Reset folder
    ChangeFolder(Folder);
    // IF category changed, reset search engine
    if (!(m_eng && Categ == m_eng->GetMyCateg()))
        if (!Init(Categ))
            return(false);
    // Find object in the list
    m_ObjName = Name;
    if (!m_eng->Find(this))
        return(false);
    // Return true, if found object is in given folder
    return(IsFromSelFolder());
}
//
// Finds object specified by objnum
// Returns true if found
//
bool CObjectList::FindByObjnum(tobjnum Objnum, CCateg Categ)
{
    // IF category changed, reset search engine
    if (!(m_eng && Categ == m_eng->GetMyCateg()))
        if (!Init(Categ))
            return(false);
    // Find object
    m_ObjNum = Objnum;
    return(m_eng->FindByObjnum(this));
}
//
// Selects object specified by index in the list
// Returns true if found
//
bool CObjectList::FindByIndex(int Index, CCateg Categ)
{
    if (!(m_eng && Categ == m_eng->GetMyCateg()))
        if (!Init(Categ))
            return(false);
    m_Index = Index;
    return(m_eng->FindByIndex(this));
}
//
// If object is a link, returns objnum as destination object
//
tobjnum CObjectList::GetLinkOrObjnum() const
{
#if WBVERS<900
    if (m_Flags & CO_FLAG_LINK)  // links are not used since version 9.0
    { tobjnum obj;
      cd_Find_object(m_cdp, m_ObjName, m_Categ | IS_LINK, &obj);
      return(obj);
    }
#endif      
    return m_ObjNum;
}
//
// Returns current object folder index
//
t_folder_index CObjectList::GetFolderIndex()
{
    // IF folder index not known, try to ask engine
    if (m_FolderIndex == FOLDER_ANY)
    {
        tobjname Name;
        strcpy(Name, m_ObjName);
        Upcase9(m_cdp, Name);
        m_FolderIndex = m_eng->GetFolderIndex(Name);
    }
    return m_FolderIndex;
}
//
// Returns true if objects is in selected folder
//
bool CObjectList::IsFromSelFolder()
{
    // IF search through all folders, return true
    if (m_AnyFolder)
        return true;
    // IF object folder index AND selected folder index known, compare folder index
    if (m_SelFolder != FOLDER_ANY && m_FolderIndex != FOLDER_ANY)
        return m_FolderIndex == m_SelFolder; 
#if WBVERS<900
    // IF Tables and selected table is ODBC table and selected folder is root, return true
    if (m_Categ == CATEG_TABLE && (m_Flags & CO_FLAG_ODBC))
        return *m_SelFolderName == 0;
#endif        
    // compare folder names
    return wb_stricmp(m_cdp, m_FolderName, m_SelFolderName) == 0;
}
//
// Vraci pocet objektu kategorie Categ, ve slozce Folder, NONECATEG = vsechny objekty, slozky se nepocitaji
//
unsigned CObjectList::GetCount(CCateg Categ, const char *Folder)
{
    if (Categ.m_Categ != NONECATEG)
        return(GetCategCount(Categ, Folder));
    unsigned Result = 0;
    CCategList cl;
    do
        Result += GetCategCount(cl.GetCateg(), Folder);
    while (cl.FindNext());
    return(Result);
}
//
// Returns object count of given category in given folder
//
unsigned CObjectList::GetCategCount(CCateg Categ, const char *Folder)
{
    // Reset folder
    ChangeFolder(Folder);
    // IF folder and category not changed, reset current object index
    if (m_eng && Categ == m_eng->GetMyCateg())
        m_eng->Reset();
    // ELSE reset search engine
    else if (!Init(Categ))
        return(0);
    // IF root folder, return all object count
    if (IsRootFolder(Folder))
        return(m_eng->GetCount());
    // Scan given folder
    unsigned Result = 0;
    while (m_eng->Next(this))
    {
         if (IsFromSelFolder())
             Result++;
    }
    // Scan subfolders (seems to be very uneffective for COLDB with folders)
    CObjectList fl(m_cdp, m_Schema);
    for (bool Found = fl.FindFirst(CATEG_FOLDER, Folder); Found; Found = fl.FindNext())
        Result += GetCategCount(Categ, fl.GetName());
    return(Result);
}
//
// Returns true if SubFolder is subfolder of Folder
//
bool CObjectList::IsSubFolder(const char *SubFolder, const char *Folder)
{
    while (wb_stricmp(m_cdp, SubFolder, Folder) != 0)
    {
        if (!*SubFolder)
            return(false);
        if (!Find(SubFolder, CATEG_FOLDER))
            return(false);
        SubFolder = GetFolderName();
    }
    return(true);
}
//
// Finds next object
// Returns true if found
//
bool COLCache::Next(CObjDescr *od)
{
    for (;;)
    {
        // IF Index >= object count, return false
        if (m_Index >= m_Comp->count())
            return(false);
        // Skip . folder
        if (m_MyCateg != CATEG_FOLDER || (m_Index != 0 && *m_Comp->acc0(m_Index)->name != 0))
            break;
        // Increment index
        m_Index++;
    }
    // Get cache content
    t_comp *comp      = m_Comp->acc0(m_Index);
    // Fill object description
    Fill(od, comp);
    return(true);
}
//
// Finds object specified by name in od->m_ObjName
// Returns true if found
//
bool COLCache::Find(CObjDescr *od)
{
    t_comp *comp;
    // IF Categ is folder
    if (m_MyCateg == CATEG_FOLDER)    // Foldery nejsou podle abecedy
    {
        // IF . folder, return first chache item
        if (!od->m_ObjName || !*od->m_ObjName)
        {
            m_Index = 0;
            comp    = m_Comp->acc0(0);
        }
        // ELSE traverse folder cache
        else
        {
            for (m_Index = 0; ; m_Index++)
            {
                if (m_Index >= m_Comp->count())
                    return(false);
                comp = m_Comp->acc0(m_Index);
                if (wb_stricmp(m_cdp, comp->name, od->m_ObjName) == 0)
                    break;
            }
        }
    }
    // ELSE not folder
    else
    {
        // Do quick cache search
        tobjname Name;
        strcpy(Name, od->m_ObjName);
        Upcase9(m_cdp, Name);
        if (!comp_dynar_search(m_Comp, Name, (unsigned *)&m_Index))
            return(false);
        comp = m_Comp->acc0(m_Index);
    }
    Fill(od, comp);
    return(true);
}
//
// Finds object specified by object number in od->m_ObjNum
// Returns true if found
//
bool COLCache::FindByObjnum(CObjDescr *od)
{
    for (m_Index = 0; m_Index < m_Comp->count(); m_Index++)
    {
        t_comp *comp = m_Comp->acc0(m_Index);
        if (comp->objnum == od->m_ObjNum)
        {
            Fill(od, comp);
            return(true);
        }
    }
    return(false);
}
//
// Selects object specified by object index in od->m_Index
// Returns true if found
//
bool COLCache::FindByIndex(CObjDescr *od)
{
    if (od->m_Index >= m_Comp->count())
        return(false);
    Fill(od, m_Comp->acc0(od->m_Index));
    return(true);
}
//
// Fills object description
//
void COLCache::Fill(CObjDescr *od, t_comp *comp)
{
    od->m_ObjName     = comp->name;
    if (m_MyCateg == CATEG_FOLDER && *(WORD *)od->m_ObjName == '.')
        od->m_ObjName = "";
    od->m_FolderName  = GetFolderName(comp->folder);
    od->m_FolderIndex = comp->folder;
    od->m_ObjNum      = comp->objnum;
    od->m_Flags       = comp->flags;
    od->m_Modif       = comp->modif_timestamp;
    od->m_Index       = m_Index++;
    od->m_Source      = comp;
}
//
// Returns folder name
//
const char *COLCache::GetFolderName(t_folder_index Index)
{
    if (Index)
        return(((t_comp *)m_FolderCache->acc0(Index))->name);
    else
        return("");
}
//
// Returns object count
//
unsigned COLCache::GetCount(void)
{
    unsigned Result = m_Comp->count();
    // Remove . folder
    if (m_MyCateg == CATEG_FOLDER)
    {
        for (int i = 0; i < m_Comp->count(); i++)
        {
            if (!*((t_comp *)m_Comp->acc0(i))->name || *((t_comp *)m_Comp->acc0(i))->name == '.')
                Result--;
        }
    }
    return(Result);
}
//
// Finds next object
// Returns true if found
//
bool COLDB::Next(CObjDescr *od)
{
    do
    {
        // IF no source list, return false
        if (!m_Src || !*m_Src)
            return(false);
        // Fill object descriptor
        od->m_Source      = (void *)m_Src;
        od->m_ObjName     = m_Src;
        m_Src            += strlen(m_Src) + 1;
        od->m_FolderName  = "";
        if (m_Extended)
        {
            if (*m_Src && *(WORD *)m_Src != '.')
                od->m_FolderName = m_Src;
            m_Src      += strlen(m_Src) + 1;
            od->m_Modif = *(uns32*)m_Src;
            m_Src      += sizeof(uns32);
        }
        od->m_FolderIndex = *od->m_FolderName ? FOLDER_ANY : FOLDER_ROOT;
        od->m_ObjNum = *(tobjnum *)m_Src;
        m_Src       += sizeof(tobjnum);
        od->m_Flags  = *(WORD *)m_Src;
        m_Src       += sizeof(WORD);
        // Skip not valid objects
    }
    while (m_MyCateg == CATEG_ROLE && *od->m_ObjName >= '0' && *od->m_ObjName <= '9');
    return(true);
}
//
// Finds object specified by name in od->m_ObjName
// Returns true if found
//
bool COLDB::Find(CObjDescr *od)
{
    const char *Name = od->m_ObjName;
    m_Src = m_Info;
    do
    {
        if (!Next(od))
            return(false);
    }
    while (wb_stricmp(m_cdp, Name, od->m_ObjName) != 0);
    return(true);
}
//
// Finds object specified by object number in od->m_ObjNum
// Returns true if found
//
bool COLDB::FindByObjnum(CObjDescr *od)
{
    tobjnum ObjNum = od->m_ObjNum;
    m_Src = m_Info;
    do
    {
        if (!Next(od))
            return(false);
    }
    while (od->m_ObjNum != ObjNum);
    return(true);
}
//
// Returns index of folder specified by od->m_FolderName 
//
t_folder_index COLDB::GetFolderIndex(const char *Name)
{
    if (!*Name || *(WORD *)Name == '.')
        return FOLDER_ROOT;
    else if (!m_FolderCache)
        return FOLDER_ANY;
    else
    {
        unsigned Result;
        if (!comp_dynar_search(m_FolderCache, Name, &Result))
            Result = FOLDER_ANY;
        return (t_folder_index)Result;
    }
}
//
// Returns object count
//
unsigned COLDB::GetCount(void)
{
    unsigned Result = 0;
    const char *Name;
    for (const char *Src = m_Info; Src && *Src;)
    {
        Name = Src;
        Src += strlen(Src) + 1;
        if (m_Extended)
        {
            Src += strlen(Src) + 1;
            Src += sizeof(uns32);
        }
        Src += sizeof(tobjnum);
        Src += sizeof(WORD);
        if (m_MyCateg != CATEG_ROLE || *Name < '0' || *Name > '9')
            Result++;
    }
    return(Result);
}
/////////////////////////////////////////////// COLODBC ///////////////////////////////////////////////////////
void COLODBC::fill(t_connection * conn, int column, char * catalog, char * schema, tcateg categ)
{ RETCODE retcode;  char buf[MAX_OBJECT_NAME_LEN+1];
  //fprintf(stderr, "\n%s %s %d %d\n", catalog ? catalog : "NULL", schema ? schema : "NULL", column, categ);
  retcode=SQLTablesA(conn->hStmt, 
    (UCHAR*)catalog, catalog ? SQL_NTS : 0, 
    (UCHAR*)schema,  schema ? SQL_NTS : 0,
    (UCHAR*)(column==3 ? "%" : NULLSTRING), SQL_NTS, 
    (UCHAR*)(categ==CATEG_TABLE ? "TABLE" : categ==CATEG_CURSOR ? "VIEW" : ""), SQL_NTS);  // Excel driver has system tables, but they are not accessible
  if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
  { SQLBindCol(conn->hStmt, column, SQL_C_CHAR, buf, sizeof(buf), NULL);
    do
    { retcode=SQLFetch(conn->hStmt);
	  //fprintf(stderr, "retcode=%d\n", retcode);
      if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO) break;
      int pos = names.count();
      if (names.acc(pos+(int)strlen(buf)))
        strcpy(names.acc0(pos), buf);
    } while (true);
    SQLFreeStmt(conn->hStmt, SQL_CLOSE);  
    SQLFreeStmt(conn->hStmt, SQL_UNBIND);
  }
  // When asking for catalogs:
   // Visual FoxPro driver: nothing returned when schema==NULL
   // Visual FoxPro driver: error returned when schema==NULLSTRING
   // Access: needs schema==NULLSTRING
  if (!names.count())  
    if (column==1)
      if (names.acc((int)strlen("<Default>")))
        strcpy(names.acc0(0), "<Default>");
  *names.acc(names.count()) = 0;  // terminator
}

bool COLODBC::Next(CObjDescr *od)
{ const char * p = names.acc(curr_pos);
  if (!*p) return false;
  od->m_Source = (void*)p;
  od->m_ObjName = p;
  curr_pos += (int)strlen(p) + 1;
  return true;
}

bool COLODBC::Find(CObjDescr *od)
{
  const char *Name = od->m_ObjName;
  Reset();
  do
  { if (!Next(od))
      return(false);
  } while (stricmp(Name, od->m_ObjName));   // should be case-sensitive, case of names not changed any more
  return true;
}

unsigned COLODBC::GetCount(void)
{ int i = 0, cnt = 0;
  while (*names.acc(i))
  { i+=(int)strlen(names.acc0(i))+1;
    cnt++;
  }
  return cnt;
}

//
// Finds next object
// Returns true if found
//
bool COLServ::Next(CObjDescr *od)
{
    while (m_Server && m_Server->odbc != m_ODBC)
      m_Server  = m_Server->next;
    if (!m_Server)
      return false;
    od->m_ObjName = m_Server->locid_server_name;
    od->m_Source  = m_Server;
    m_Server  = m_Server->next;
    return true;
}
//
// Finds object specified by name in od->m_ObjName
// Returns true if found
//
bool COLServ::Find(CObjDescr *od)
{
    for (m_Server = GetServerList(); m_Server; m_Server = m_Server->next)
    {
        if (stricmp(m_Server->locid_server_name, od->m_ObjName) == 0 && m_Server->odbc == m_ODBC)
        {
            od->m_Source  = m_Server;
            return(true);
        }
    }
    return(false);
}
//
// Finds object specified by objnum
// Returns true if found
//
bool COLServ::FindByObjnum(CObjDescr *od)
{
    tobjnum ObjNum;
    for (m_Server = GetServerList(), ObjNum = od->m_ObjNum; m_Server; m_Server = m_Server->next, ObjNum--)
    {
         if (!ObjNum)
         {
            od->m_ObjName = m_Server->locid_server_name;
            od->m_Source  = m_Server;
            return(true);
         }
    }
    return(false);
}
//
// Returns object count
//
unsigned COLServ::GetCount(void)
{
    unsigned Result = 0;
    for (t_avail_server *Server = GetServerList(); Server; Server = Server->next)
        if (Server->odbc == m_ODBC)
            Result++;
    return(Result);
}
//
// CATEG_TABLE      0   Cache   Folder
// CATEG_USER       1   
// CATEG_VIEW       2   Cache   Folder
// CATEG_CURSOR     3   Cache   Folder
// CATEG_PGMSRC     4   Cache   Folder
// CATEG_PGMEXE     5   
// CATEG_MENU       6   Cache   Folder
// CATEG_APPL       7   
// CATEG_PICT       8   Cache
// CATEG_GROUP      9   
// CATEG_ROLE       10  
// CATEG_CONNECTION 11  Cache?
// CATEG_RELATION   12  Cache   Folder
// CATEG_DRAWING    13  Cache   Folder
// CATEG_GRAPH      14
// CATEG_REPLREL    15  Cache   Folder
// CATEG_PROC       16  Cache   Folder
// CATEG_TRIGGER    17  Cache   Folder
// CATEG_WWW        18  Cache   Folder
// CATEG_FOLDER     19  Cache   Folder
// CATEG_SEQ        20  Cache   Folder
// CATEG_INFO       21          
// CATEG_DOMAIN     22  Cache   Fodler
// CATEG_KEY        25
// CATEG_SERVER     26
// 
//
// Popisy kategorii
//
#define gettext_noopL(string) L##string
#define gettext_noop(string) string

#define FilterTDF  gettext_noop(L"Tables (*.tdf)|*.tdf|")
#define FilterUSR  gettext_noop(L"Users (*.usr)|*.usr|")
#define FilterVWD  (L"Views (*.vwd)|*.vwd|")
#define FilterSSH  gettext_noopL("Style sheets (*.sts)|*.sts|")
#define FilterCRS  gettext_noopL("Queries (*.crs)|*.crs|")
#define FilterPGM  gettext_noopL("Transports (*.pgm)|*.pgm|")
#define FilterEXX  (L"Executables (*.exx)|*.exx|")
#define FilterMNU  (L"Menus (*.mnu)|*.mnu|")
#define FilterAPL  gettext_noopL("Applications (*.apl)|*.apl|")
#define FilterPCT  (L"Pictures (*.bmp;*.jpg;*.jpeg;*.gif;*.pcx;*.tif;*.wpg;*.wbp)|*.bmp;*.jpg;*.jpeg;*.gif;*.pcx;*.tif;*.wpg;*.wbp|bmp (*.bmp)|*.bmp|jpeg (*.jpg;*.jpeg)|*.jpg;*.jpeg|gif (*.gif)|*.gif|pcx (*.pcx)|*.pcx|tiff (*.tif)|*.tif|wpg (*.wpg)|*.wpg|wbp (*.wbp)|*.wbp|")
#define FilterDRW  gettext_noopL("Diagrams (*.drw)|*.drw|")
#define FilterRRL  (L"Replication relations (*.rrl)|*.rrl|")
#define FilterPSM  gettext_noopL("Procedures (*.psm)|*.psm|")
#define FilterTRG  gettext_noopL("Triggers (*.trg)|*.trg|")
#define FilterWWW  (L"WWW objects (*.www)|*.www|")
#define FilterSEQ  gettext_noopL("Sequences (*.seq)|*.seq|")
#define FilterDOM  gettext_noopL("Domains (*.dom)|*.dom|")
#define FilterFTX  gettext_noopL("Fulltext systems (*.inf)|*.inf|")
#define FilterPRK  (L"Secure keys (*.prk)|*.prk|")
#define FilterFO   (L"XML Forms (*.fo)|*.fo|")                   
//
// Category descryption table
//
// Unused categories are usefull when exporting/importing old applications.
//
tcategdescr CategNameTB[] =
{
    {FF_FOLDERS | FF_EXPORT | FF_ENCRYPT | FF_BACKEND | FF_TEXTDEF,            "Table",               gettext_noopL("Table"),               gettext_noopL("table"),               gettext_noopL("tables"),                "Table   ", ".tdf", FilterTDF,   diTables_xpm,      CATEG_CURSOR},       // CATEG_TABLE
    {0,                                                                        "User",                gettext_noopL("User"),                gettext_noopL("user"),                gettext_noopL("users"),                 "User    ", ".usr", FilterUSR,   NULL,              CATEG_GROUP},        // CATEG_USER
    {FF_HIDEN,                                                                 "",                    L"",                                  L"",                                  L"",                                    "",         ".vwd", FilterVWD,   NULL,              NONECATEG},         // CATEG_VIEW
    {FF_FOLDERS | FF_EXPORT | FF_ENCRYPT | FF_BACKEND | FF_TEXTDEF,            "Query",               gettext_noopL("Query"),               gettext_noopL("query"),               gettext_noopL("queries"),               "Cursor  ", ".crs", FilterCRS,   diQueries_xpm,     CATEG_PGMSRC},       // CATEG_CURSOR
    {FF_FOLDERS | FF_EXPORT | FF_ENCRYPT | FF_TEXTDEF,                         "Transport",           gettext_noopL("Transport"),           gettext_noopL("transport"),           gettext_noopL("transports"),            "Program ", ".pgm", FilterPGM,   diProgramms_xpm,   CATEG_DRAWING},      // CATEG_PGMSRC
    {FF_HIDEN,                                                                 "",                    L"",                                  L"",                                  L"",                                    "",         ".exx", FilterEXX,   NULL,              NONECATEG},          // CATEG_PGMEXE
    {FF_HIDEN,                                                                 "",                    L"",                                  L"",                                  L"",                                    "",         ".mnu", FilterMNU,   NULL,              CATEG_CURSOR},       // CATEG_MENU
    {FF_HIDEN,                                                                 "Application",         gettext_noopL("Application"),         gettext_noopL("application"),         gettext_noopL("applications"),          "",         ".apl", FilterAPL,   NULL,              NONECATEG},          // CATEG_APPL
    {FF_HIDEN,                                                                 "",                    L"",                                  L"",                                  L"",                                    "",         ".wbp", FilterPCT,   NULL,              CATEG_DRAWING},      // CATEG_PICT
    {0,                                                                        "Group",               gettext_noopL("Group"),               gettext_noopL("group"),               gettext_noopL("groups"),                "Group   ","",     L"",         NULL,              CATEG_SERVER},       // CATEG_GROUP
    {0,                                                                        "Role",                gettext_noopL("Role"),                gettext_noopL("role"),                gettext_noopL("roles"),                 "Role    ", "",     L"",         diRoles_xpm,       NONECATEG},          // CATEG_ROLE  (last category)
    {FF_HIDEN,                                                                 "",                    L"",                                  L"",                                  L"",                                    "",         "",     L"",         NULL,              CATEG_REPLREL},      // CATEG_CONNECTION
    {FF_HIDEN | FF_EXPORT | FF_TEXTDEF,                                        "",                    L"",                                  L"",                                  L"",                                    "Relat   ",    ".rel", L"",         NULL,              CATEG_CONNECTION},   // CATEG_RELATION
    {FF_FOLDERS | FF_EXPORT | FF_ENCRYPT | FF_TEXTDEF,                         "Diagram",             gettext_noopL("Diagram"),             gettext_noopL("diagram"),             gettext_noopL("diagrams"),              "Drawing ", ".drw", FilterDRW,   diDiagramms_xpm,   CATEG_PROC},         // CATEG_DRAWING
    {FF_HIDEN,                                                                 "",                    L"",                                  L"",                                  L"",                                    "",         ".grf", L"",         NULL,              NONECATEG},          // CATEG_GRAPH
    {FF_HIDEN | FF_EXPORT | FF_TEXTDEF,                                        "",                    L"",                                  L"",                                  L"",                                    "ReplRel ",  ".rrl", FilterRRL,   NULL,              CATEG_PROC},         // CATEG_REPLREL
    {FF_FOLDERS | FF_EXPORT | FF_ENCRYPT | FF_BACKEND | FF_TEXTDEF,            "Procedure",           gettext_noopL("Procedure"),           gettext_noopL("procedure"),           gettext_noopL("procedures"),            "Proc    ", ".psm", FilterPSM,   diProcedures_xpm,  CATEG_TRIGGER},      // CATEG_PROC
    {FF_FOLDERS | FF_EXPORT | FF_ENCRYPT | FF_BACKEND | FF_TEXTDEF,            "Trigger",             gettext_noopL("Trigger"),             gettext_noopL("trigger"),             gettext_noopL("triggers"),              "Trigger ", ".trg", FilterTRG,   diTriggers_xpm,    CATEG_SEQ},          // CATEG_TRIGGER
    {FF_HIDEN,                                                                 "",                    L"",                                  L"",                                  L"",                                    "",         ".www", FilterWWW,   NULL,              CATEG_VIEW},         // CATEG_WWW
    {FF_HIDEN | FF_FOLDERS,                                                    "Folder",              gettext_noopL("Folder"),              gettext_noopL("folder"),              gettext_noopL("folders"),               "Folder  ", ".fld", L"",         diFolders_xpm,     NONECATEG},          // CATEG_FOLDER
    {FF_FOLDERS | FF_EXPORT | FF_BACKEND | FF_TEXTDEF,                         "Sequence",            gettext_noopL("Sequence"),            gettext_noopL("sequence"),            gettext_noopL("sequences"),             "Sequen  ", ".seq", FilterSEQ,   diSequences_xpm,   CATEG_DOMAIN},       // CATEG_SEQ
#if WBVERS>=950   // The fulltext system object is on the control panel, if defined
    {FF_FOLDERS | FF_EXPORT | FF_BACKEND | FF_TEXTDEF,                         "Fulltext",            gettext_noopL("Fulltext"),            gettext_noopL("fulltext"),            gettext_noopL("fulltexts"),             "Info    ", ".inf", FilterFTX,   _full_xpm,         CATEG_XMLFORM},      // CATEG_INFO
    {FF_FOLDERS | FF_EXPORT | FF_BACKEND | FF_TEXTDEF,                         "Domain",              gettext_noopL("Domain"),              gettext_noopL("domain"),              gettext_noopL("domains"),               "Domain  ", ".dom", FilterDOM,   diDomains_xpm,     CATEG_INFO},         // CATEG_DOMAIN
#else
    {FF_HIDEN,                                                                 "Fulltext",            gettext_noopL("Fulltext"),            gettext_noopL("fulltext"),            gettext_noopL("fulltexts"),             "Info    ", ".inf", FilterFTX,   _full_xpm,         CATEG_INFO},         // CATEG_INFO
    {FF_FOLDERS | FF_EXPORT | FF_BACKEND | FF_TEXTDEF,                         "Domain",              gettext_noopL("Domain"),              gettext_noopL("domain"),              gettext_noopL("domains"),               "Domain  ", ".dom", FilterDOM,   diDomains_xpm,     CATEG_XMLFORM},      // CATEG_DOMAIN
#endif
#ifdef XMLFORMS
    {           FF_EXPORT | FF_ENCRYPT,                                        "Style sheet",         gettext_noopL("Style sheet"),         gettext_noopL("style sheet"),         gettext_noopL("style sheets"),          "StyleSh ", ".sts", FilterSSH,   diForms_xpm,       CATEG_ROLE},          // CATEG_STSH
#ifdef WINS 
    {           FF_EXPORT | FF_ENCRYPT,                                        "XML Form",            gettext_noopL("XML Form"),            gettext_noopL("XML Form"),            gettext_noopL("XML Forms"),             "Form    ", ".fo",  FilterFO,    xmlform_xpm,       CATEG_STSH},          // CATEG_XMLFORM
#else
    {FF_HIDEN | FF_EXPORT | FF_ENCRYPT,                                        "XML Form",            gettext_noopL("XML Form"),            gettext_noopL("XML Form"),            gettext_noopL("XML Forms"),             "Form    ", ".fo",  FilterFO,    xmlform_xpm,       CATEG_STSH},          // CATEG_XMLFORM
#endif
#else
    {FF_HIDEN,                                                                 "",                    L"",                                  L"",                                  L"",                                    "",         ".prk", L"",         NULL,              CATEG_ROLE},          // CATEG_STSH
    {FF_HIDEN | FF_EXPORT | FF_ENCRYPT,                                        "",                    L"",                                  L"",                                  L"",                                    "",         ".fo",  L"",         NULL,              CATEG_STSH},         // CATEG_XMLFORM
#endif
    {FF_HIDEN,                                                                 "",                    L"",                                  L"",                                  L"",                                    "",         "",     L"",         NULL,              CATEG_TABLE}         // CATEG_SERVER
};

#ifdef GETTEXT_STRING  // mark these strings for translation
wxTRANSLATE("XML Form")
wxTRANSLATE("XML Forms")
wxTRANSLATE("Style Sheet")
wxTRANSLATE("Style Sheets")
#endif
//
// Constructor
//
CCategList::CCategList()
{
    m_Descr = CategNameTB;
    Count   = sizeof(CategNameTB) / sizeof(tcategdescr);
}
//
// Selects first category in category list
//
void CCategList::FindFirst()
{
    m_Descr = CategNameTB;
}
//
// Selects next category in category list
// Returns false if no next category exists
//
bool CCategList::FindNext()
{
    while (m_Descr->NextInPanel != NONECATEG)
    {
        m_Descr = CategNameTB + m_Descr->NextInPanel;
        if (!(m_Descr->Flags & FF_HIDEN))
            return(true);
    }
    return(false);
}
//
// Returns category ID of selected category
//
tcateg CCategList::GetCateg() const
{
    return((tcateg)(m_Descr - CategNameTB));
}
//
// Returns flags of selected category
//
unsigned CCategList::GetFlags(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? CategNameTB[Categ].Flags : 0);
}
//
// Returns true if selected category can have folders
//
bool CCategList::HasFolders(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? (CategNameTB[Categ].Flags & FF_FOLDERS) != 0 : false);
}
//
// Returns true if selected category has text definitions
//
bool CCategList::HasTextDefin(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? (CategNameTB[Categ].Flags & FF_TEXTDEF) != 0 : false);
}
//
// Returns true if objects of selected category can be exported
//
bool CCategList::Exportable(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? (CategNameTB[Categ].Flags & FF_EXPORT) != 0 : false);
}
//
// Returns true if objects of selected category can be encrypted
//
bool CCategList::Encryptable(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? (CategNameTB[Categ].Flags & FF_ENCRYPT) != 0 : false);
}
//
// Returns true if objects of selected category can be encrypted
//
bool CCategList::IsBackendCateg(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? (CategNameTB[Categ].Flags & FF_BACKEND) != 0 : false);
}
//
// Returns true if objects of selected category fall in database backend
//
const char *CCategList::Original(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? CategNameTB[Categ].Original : "");
}
//
// Returns given category name with first character capital
//
const wchar_t *CCategList::CaptFirst(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? CategNameTB[Categ].CaptFirst : L"");
}
//
// Returns given category name in UNICODE with first caracter capital
// Input:   Categ - Requested category
//          Val   - Buffer for UNICODE conversion
//
const wchar_t *CCategList::CaptFirst(cdp_t cdp, CCateg Categ, wchar_t *Val)
{
    return(Categ < CATEG_COUNT ? Get_transl(cdp, CategNameTB[Categ].CaptFirst, Val) : L"");
}
//
// Returns given category name in lower case
//
const wchar_t *CCategList::Lower(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? CategNameTB[Categ].Lower : L"");
}
//
// Returns given category name in UNICODE lower case
// Input:   Categ - Requested category
//          Val   - Buffer for UNICODE conversion
//
const wchar_t *CCategList::Lower(cdp_t cdp, CCateg Categ, wchar_t *Val)
{
    return(Categ < CATEG_COUNT ? Get_transl(cdp, CategNameTB[Categ].Lower, Val) : L"");
}
//
// Returns plural of given category name
//
const wchar_t *CCategList::Plural(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? CategNameTB[Categ].Plural : L"");
}
//
// Returns plural of given category name in UNICODE
// Input:   Categ - Requested category
//          Val   - Buffer for UNICODE conversion
//
const wchar_t *CCategList::Plural(cdp_t cdp, CCateg Categ, wchar_t *Val, bool CaptFirst)
{
    if (Categ >= CATEG_COUNT)
        return L"";
    Get_transl(cdp, CategNameTB[Categ].Plural, Val);
    if (CaptFirst)
        *Val = upcase_charW(*Val, cdp ? cdp->sys_charset : 0);
    return Val;
}
//
// Returns given category name for .APL file
//
const char *CCategList::ShortName(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? CategNameTB[Categ].ExportName : "");
}
//
// Returns file extension for objects of given category export
//
const char *CCategList::FileType(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? CategNameTB[Categ].FileType : "");
}
//
// Returns file extension filter for select file dialog for objects of given category export
//
const wchar_t *CCategList::FileTypeFilter(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? CategNameTB[Categ].FileTypeFilter : L"");
}
//
// Returns image of more selected objects of given category
//
const char **CCategList::MultiSelImage(CCateg Categ)
{
    return(Categ < CATEG_COUNT ? (const char **)CategNameTB[Categ].DragImages : NULL);
}

#ifdef WINS
//
// Finds first profile in profile list
// Returns true if profile found
//
bool CMailProfList::FindFirst()
{
    if (!GetRootKey())
        return(false);
    m_Index = -1;
    return(FindNext());
}
//
// Finds next profile in profile list
// Returns true if profile found
//
bool CMailProfList::FindNext()
{
    CloseKey();
    DWORD Size = sizeof(m_Name) / sizeof(wchar_t);
    return(RegEnumKeyExW(m_hRootKey, (DWORD)++m_Index, m_Name, &Size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS);
}
//
// Opens mail profile
//
bool CMailProfList::Open(const wchar_t *ProfileName, bool WriteAccess)
{
    if (!GetRootKey())
        return(false);
    CloseKey();
    if (WriteAccess)
        return(RegCreateKeyExW(m_hRootKey, ProfileName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS_EX, NULL, &m_hKey, NULL) == ERROR_SUCCESS);
    else
        return(RegOpenKeyExW(m_hRootKey, ProfileName, 0, KEY_READ_EX, &m_hKey) == ERROR_SUCCESS);
}
//
// Returns value of profile property
//
const wchar_t *CMailProfList::GetStrValue(const wchar_t *ValName)
{
    if (!m_hKey)
        return NULL;
    DWORD Size = sizeof(m_Value);
    m_Value[0] = 0;
    RegQueryValueExW(m_hKey, ValName, NULL, NULL, (LPBYTE)m_Value, &Size);
    return m_Value;
}
//
// Sets value of profile property
//
void CMailProfList::SetStrValue(const wchar_t *ValName, const wchar_t *Val)
{
    if (m_hKey)
    {
        if (Val && *Val)
        {
            DWORD Size = lstrlenW(Val) * sizeof(wchar_t);
            RegSetValueExW(m_hKey, ValName, 0, REG_SZ, (LPBYTE)Val, Size);
        }
        else
        {
            RegDeleteValueW(m_hKey, ValName);
        }
    }
}
//
// Sets binary value of profile property
// Input:   ValName - Property name
//          Val     - Value 
//          Size    - Value size
//
void CMailProfList::SetBinValue(const wchar_t *ValName, const char *Val, unsigned Size)
{
    if (m_hKey)
    {
        if (Size)
            RegSetValueExW(m_hKey, ValName, 0, REG_BINARY, (LPBYTE)Val, Size);
        else
            RegDeleteValueW(m_hKey, ValName);
    }
}
//
// Returns binary value of profile property
// Input:   ValName - Property name
//          Buf     - Buffer for value 
//          Size    - Buffer size
// Returns: Real value size
//
int CMailProfList::GetBinValue(const wchar_t *ValName, char *Buf, unsigned long Size)
{
    if (!m_hKey)
        return(0);
    if (RegQueryValueExW(m_hKey, ValName, NULL, NULL, (LPBYTE)Buf, &Size) != ERROR_SUCCESS)
        return(0);
    return(Size);
}
//
// Returns key of profile list root
// 
bool CMailProfList::GetRootKey()
{
    if (!m_hRootKey)
    {
        char  Key[MAX_PATH];

        strcpy(Key, WB_inst_key);  
        strcat(Key, "\\Mail");
        if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, Key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS_EX, NULL, &m_hRootKey, NULL))
            return(false);
    }
    return(true);
}
//
// Finds first Mail602 profile
// Returns true if found
//
wchar_t Mail602INI[] = L"Mail602.INI";

bool CMailProfList::FindFirst602()
{
    // Read section list from Mail602.INI 
    for (unsigned Size = 512; ; Size += 256)
    {
        m_Profs602.AllocX(Size * sizeof(wchar_t));
        unsigned rd = GetPrivateProfileStringW(L"PROFILES", NULL, NULL, m_Profs602, Size, Mail602INI);
        if (rd < Size - 2)
            break;
    }
    // Set first profile
    m_Prof602 = m_Profs602;
    return(*m_Prof602 != 0);
}
//
// Finds next Mail602 profile
// Returns true if found
//
bool CMailProfList::FindNext602()
{
    if (*m_Prof602)
        m_Prof602 += lstrlenW(m_Prof602) + 1;
    return(*m_Prof602 != 0);
}
//
// Finds first MAPI profile
// Returns true if found
//
HINSTANCE           hMapi;    
LPMAPIADMINPROFILES lpMAPIAdminProfiles;
LPMAPIFREEBUFFER    lpMAPIFreeBuffer;

bool CMailProfList::FindFirstMAPI()
{
    // Load MAPI dll and necessary functions
    if (!hMapi)
    {
        hMapi = LoadLibraryA("mapi32.dll");
        if (!hMapi)
            return(false);
    }
    if (!lpMAPIAdminProfiles)
    {
        lpMAPIAdminProfiles = (LPMAPIADMINPROFILES)GetProcAddress(hMapi, "MAPIAdminProfiles");
        if (!lpMAPIAdminProfiles)
            return(false);
    }
    if (!lpMAPIFreeBuffer)
    {
        lpMAPIFreeBuffer = (LPMAPIFREEBUFFER)GetProcAddress(hMapi, "MAPIFreeBuffer");
        if (!lpMAPIFreeBuffer)
            return(false);
    }
    // Release old list
    if (m_RowSet)
        lpMAPIFreeBuffer(m_RowSet);
    LPPROFADMIN lpProfAdmin = NULL;
    LPMAPITABLE lpProfTable = NULL;
    DWORD cRows;

    // Get profile admin
    bool    Found = false;
    HRESULT hrErr = lpMAPIAdminProfiles(0, &lpProfAdmin);
    if (hrErr == NOERROR)
    {
        // Get profile table
        hrErr = lpProfAdmin->GetProfileTable(0, &lpProfTable);
        if (hrErr == NOERROR)
        {
            // Z atributu profilu me zajima jen jejich DISPLAY_NAME
            // Set column list
            SPropTagArray ColDesc;
            ColDesc.cValues       = 1;
            ColDesc.aulPropTag[0] = PR_DISPLAY_NAME_W;
            hrErr = lpProfTable->SetColumns(&ColDesc, 0);
            // Load profile names list
            if (hrErr == NOERROR)
            {

                hrErr = lpProfTable->GetRowCount(0, &cRows);
                if (hrErr == NOERROR)
                {
                    if (cRows)
                    {
                        hrErr = lpProfTable->QueryRows(cRows, 0, &m_RowSet);
                        if (hrErr == NOERROR)
                        {
                            m_IndexMAPI = 0;
                            Found       = true;
                        }
                    }
                }
            }
            //lpProfTable->Release();
        }
        //lpProfAdmin->Release();
    }
    return(Found);
}
//
// Destructor
//
CMailProfList::~CMailProfList()
{
    if (m_hKey)
        RegCloseKey(m_hKey);
    if (m_hRootKey)
        RegCloseKey(m_hRootKey);
    if (m_RowSet)
        lpMAPIFreeBuffer(m_RowSet);
}
//
// Deletes all profile properties
//
void CMailProfList::DeleteAllValues()
{ 
    char  Buf[64];
    for (DWORD i = 0; ; i++)
    {
        DWORD Size = sizeof(Buf);
        if (RegEnumValueA(m_hKey, i, Buf, &Size, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;
        RegDeleteValueA(m_hKey, Buf);
    }
}
//
// Deletes profile
//
bool CMailProfList::Delete(const wchar_t *ProfileName)
{
    if (!GetRootKey())
        return(false);
    return(RegDeleteKeyW(m_hRootKey, ProfileName) == ERROR_SUCCESS);
}

#else

//
// Finds first profile in profile list
// Returns true if profile found
//
#define configfile  "/etc/602sql"

bool CMailProfList::FindFirst()
{
    // Load 602sql ini file section names list
    if (!GetProfList())
        return(false);
    m_Index = 0;
    // Find first mail profile section name
    for (m_ProfPtr = m_ProfList; *m_ProfPtr && strnicmp(m_ProfPtr, ".MAIL_", 6) != 0; m_ProfPtr += strlen(m_ProfPtr) + 1, m_Index++);
    // Set profile name
    strmaxcpy(m_Profile, m_ProfPtr, sizeof(m_Profile));
	ToUnicode(m_Profile + 6, m_Name, sizeof(m_Name) / sizeof(wchar_t));
    return(*m_Profile != 0);
}
//
// Finds next profile in profile list
// Returns true if profile found
//
bool CMailProfList::FindNext()
{
    // Find next mail profile section name
    while (*m_ProfPtr != 0)
    {
        m_ProfPtr += strlen(m_ProfPtr) + 1;
        m_Index++;
        if (strnicmp(m_ProfPtr, ".MAIL_", 6) == 0)
        {
            // Set profile name
            strmaxcpy(m_Profile, m_ProfPtr,  sizeof(m_Profile));
			ToUnicode(m_Profile + 6, m_Name, sizeof(m_Name) / sizeof(wchar_t));
            break;
        }
    }
    return(*m_ProfPtr != 0);
}
//
// Opens mail profile
//
bool CMailProfList::Open(const wchar_t *ProfileName, bool WriteAccess)
{
    strcpy(m_Profile, ".MAIL_");
    FromUnicode(ProfileName, m_Profile + 6, sizeof(m_Profile) - 6);
    char buf[4];
    return WriteAccess || GetPrivateProfileString(m_Profile, NULL, "", buf, sizeof(buf), configfile) > 0;
}
//
// Returns true if profile property exists
//
bool CMailProfList::Exists(const wchar_t *ValName)
{
    char Name[64];
    char Buf[256] = "";
    if (*m_Profile)
        GetPrivateProfileString(m_Profile, FromUnicode(ValName, Name, sizeof(Name)), "__$$NotExists$$__", Buf, sizeof(Buf), configfile);
    return strcmp(Buf, "__$$NotExists$$__") != 0;
}
//
// Returns value of profile property
//
const wchar_t *CMailProfList::GetStrValue(const wchar_t *ValName)
{
    char Name[64];
    char Buf[256] = "";
    if (*m_Profile)
        GetPrivateProfileString(m_Profile, FromUnicode(ValName, Name, sizeof(Name)), "", Buf, sizeof(Buf), configfile);
    return ToUnicode(Buf, m_Value, sizeof(m_Value) / sizeof(wchar_t));
}
//
// Sets value of profile property
//
void CMailProfList::SetStrValue(const wchar_t *ValName, const wchar_t *Val)
{
    char Name[64];
    char Buf[256];
    if (*m_Profile)
        WritePrivateProfileString(m_Profile, FromUnicode(ValName, Name, sizeof(Name)), Val && *Val ? FromUnicode(Val, Buf, sizeof(Buf)) : NULL, configfile);
}
//
// Loads 602sql ini file section names list
//
bool CMailProfList::GetProfList()
{
    if (m_ProfList)
        return(true);
    for (int  Size = 256; ; Size *= 2)
    {
        m_ProfList = (const char *)corealloc(Size, 0x88);
        if (!m_ProfList)
            return(false);
        if (GetPrivateProfileString(NULL, NULL, "", (char *)m_ProfList, Size, configfile) < Size - 2)
            break;
    }
    return(true);
}
//
// Destructor
//
CMailProfList::~CMailProfList()
{
    if (m_ProfList)
        corefree(m_ProfList);
}
//
// Deletes all profile properties
//
void CMailProfList::DeleteAllValues()
{ 
    WritePrivateProfileString(m_Profile, NULL, NULL, configfile);
}
//
// Deletes profile
//
bool CMailProfList::Delete(const wchar_t *ProfileName)
{ 
    char Profile[65] = ".MAIL_";
    FromUnicode(ProfileName, Profile + 6, sizeof(Profile) - 6);
    WritePrivateProfileString(Profile, NULL, NULL, configfile);
    return true;
}

#endif

