#ifndef _API_H_
#define _API_H_

#include "exceptions.h"
#include "buffer.h"

#ifdef LINUX
#define DEF_SHOW_TYPE 0
#else
#define DEF_SHOW_TYPE SW_SHOWMINNOACTIVE
#endif


CFNC DllKernel BOOL WINAPI Move_data( cdp_t cdp, tobjnum move_descr_obj, const char * inpname, tobjnum inpobj, const char * outname, int inpformat, int outformat, int inpcode, int outcode, int silent );

class C602SQLApi
{
protected:

    cdp_t m_cdp;

public:

    C602SQLApi(){}
    C602SQLApi(cdp_t cdp){m_cdp = cdp;}

    bool Am_I_config_admin(){return cd_Am_I_config_admin(m_cdp) != 0;}
    bool Am_I_security_admin(){return cd_Am_I_security_admin(m_cdp) != 0;}
    void connect(const char *ServerName, int ShowType = DEF_SHOW_TYPE)
    {
        int Err = cd_connect(m_cdp, ServerName, ShowType);
        if (Err)
            throw new C602SQLException(m_cdp, L"cd_connect", Err);
    }
    void connect(const wchar_t *ServerName, int ShowType = DEF_SHOW_TYPE)
    {
        tobjname aName;
        int Err = cd_connect(m_cdp, FromUnicode(ServerName, aName, sizeof(aName)), ShowType);
        if (Err)
            throw new C602SQLException(m_cdp, L"cd_connect", Err);
    }
    void Close_cursor(tcursnum Curs)
    {
        cd_Close_cursor(m_cdp, Curs);
    }
    void Delete(tcurstab CursTab, trecnum Pos)
    {
        if (cd_Delete(m_cdp, CursTab, Pos))
            throw new C602SQLException(m_cdp, L"Delete");
    }
    tptr enc_read_from_file(FHANDLE hFile, WBUUID uuid, const char *objname, int *objsize)
    {
        tptr defin = ::enc_read_from_file(m_cdp, hFile, uuid, objname, objsize);
        if (!defin)
            throw new C602SQLException(m_cdp, L"enc_read_from_file");
        return(defin);
    }
    tobjnum Find_object(const char *Name, tcateg Categ)
    {
        tobjnum Result;
        if (cd_Find_prefixed_object(m_cdp, NULL, Name, Categ, &Result))
        {
            if (cd_Sz_error(m_cdp) != OBJECT_NOT_FOUND)
                throw new C602SQLException(m_cdp, L"Find_object");
        }
        return(Result);
    }
    tobjnum Find_object(const wchar_t *Name, tcateg Categ)
    {
        tobjname aName;
        return Find_object(FromUnicode(Name, aName, sizeof(aName)), Categ);
    }
    tobjnum Find_objectX(const char *Name, tcateg Categ)
    {
        tobjnum Result;
        if (cd_Find_prefixed_object(m_cdp, NULL, Name, Categ, &Result))
            throw new C602SQLException(m_cdp, L"Find_object");
        return(Result);
    }
    tobjnum Find_objectX(const wchar_t *Name, tcateg Categ)
    {
        tobjname aName;
        return Find_objectX(FromUnicode(Name, aName, sizeof(aName)), Categ);
    }
    tobjnum Find_object_by_id(WBUUID uuid, tcateg Categ)
    {
        tobjnum Result;
        if (cd_Find_object_by_id(m_cdp, uuid, Categ, &Result))
            throw new C602SQLException(m_cdp, L"Find_object_by_id");
        return(Result);
    }
    void Free_deleted(ttablenum Table)
    {
        if (cd_Free_deleted(m_cdp, Table))
            throw new C602SQLException(m_cdp, L"Free_deleted");
    }
    bool Get_group_role(tobjnum UserGroup, tobjnum GroupRole, tcateg Categ)
    {
        uns32 Result;
        if (cd_GetSet_group_role(m_cdp, UserGroup, GroupRole, Categ, OPER_GET, &Result))
            throw new C602SQLException(m_cdp, L"GetSet_group_role");
        return(Result != 0);
    }
    void Get_privils(tobjnum user_group_role, tcateg subject_categ, ttablenum table, trecnum recnum, t_oper operation, uns8 *privils)
    {
        if (cd_GetSet_privils(m_cdp, user_group_role, subject_categ, table, recnum, operation, privils))
            throw new C602SQLException(m_cdp, L"GetSet_privils");
    }
    int Get_property_value(const char *Owner, const char *Name, char *Buf, unsigned BufSize)
    {
        int  sz;
        if (cd_Get_property_value(m_cdp, Owner, Name, 0, Buf, BufSize, &sz))
            throw new C602SQLException(m_cdp, L"Get_property_value");
        return sz;
    }
    int Get_property_value(const wchar_t *Owner, const wchar_t *Name, char *Buf, unsigned BufSize)
    {
        int  sz;
        char aOwner[64];
        char aName[64];
        if (cd_Get_property_value(m_cdp, FromUnicode(Owner, aOwner, sizeof(aOwner)), FromUnicode(Name, aName, sizeof(aName)), 0, Buf, BufSize, &sz))
            throw new C602SQLException(m_cdp, L"Get_property_value");
        return sz;
    }
    const d_table *get_table_d(tobjnum CursTab, tcateg Categ)
    {
        const d_table *Result = cd_get_table_d(m_cdp, CursTab, Categ);
        if (!Result)
            throw new C602SQLException(m_cdp, L"cd_get_table_d");
        return(Result);
    }
    tobjnum Insert_object(const char *ObjName, tcateg Categ)
    {
        tobjnum Result;
        if (cd_Insert_object(m_cdp, ObjName, Categ, &Result))
            throw new C602SQLException(m_cdp, L"Insert_object");
        return(Result);
    }
    tobjnum Insert_object_limited(const char *ObjName, tcateg Categ)
    {
        tobjnum Result;
        if (cd_Insert_object_limited(m_cdp, ObjName, Categ, &Result))
            throw new C602SQLException(m_cdp, L"Insert_object_limited");
        return(Result);
    }
    tptr Load_objdef(tobjnum objnum, tcateg Categ)
    {
        tptr def = cd_Load_objdef(m_cdp, objnum, Categ, NULL);
        if (!def)
            throw new C602SQLException(m_cdp, L"Load_objdef");
        return(def);
    }
    bool Lock_server()
    {
        if (!cd_Lock_server(m_cdp))
            return true;
        if (cd_Sz_error(m_cdp) == CANNOT_LOCK_KERNEL)
            return false;
        throw new C602SQLException(m_cdp, L"Lock_server");
    }
    void Login(const char *UserName, const char *Password)
    {
        if (cd_Login(m_cdp, UserName, Password))
            throw new C602SQLException(m_cdp, L"Login");
    }
    void Login(const wchar_t *UserName, const wchar_t *Password)
    {
        tobjname aName;
        char aPword[64];
        if (cd_Login(m_cdp, FromUnicode(m_cdp, UserName, aName, sizeof(aName)), FromUnicode(Password, aPword, sizeof(aPword))))
            throw new C602SQLException(m_cdp, L"Login");
    }
    void Move_data(tobjnum move_descr_obj, const char *inpname, tobjnum inpobj, const char *outname, int inpformat, int outformat, int inpcode, int outcode, int silent)
    {
        if (!::Move_data(m_cdp, move_descr_obj, inpname, inpobj, outname, inpformat, outformat, inpcode, outcode, silent))
            throw new C602SQLException(m_cdp, L"Move_data");
    }
    tcursnum Open_cursor_direct(const char *Query)
    {
        tcursnum Result;
        if (cd_Open_cursor_direct(m_cdp, Query, &Result))
            throw new C602SQLException(m_cdp, L"Open_cursor_direct");
        return Result;
    }
    tcursnum Open_cursor_direct(const wchar_t *Query)
    {
        CBuffer aQuery;
        aQuery.AllocX((int)wcslen(Query) + 1);
        FromUnicode(m_cdp, Query, (char *)aQuery, aQuery.GetSize());
        tcursnum Result;
        if (cd_Open_cursor_direct(m_cdp, aQuery, &Result))
            throw new C602SQLException(m_cdp, L"Open_cursor_direct");
        return Result;
    }
    bool Read(tcurstab CursTab, trecnum Pos, tattrib Attr, void *Data)
    {
        if (cd_Read(m_cdp, CursTab, Pos, Attr, NULL, Data))
        {
            if (cd_Sz_error(m_cdp) == OUT_OF_TABLE)
                return false;
            throw new C602SQLException(m_cdp, L"Read");
        }
        return true;
    }
    bool Read_ind(tcurstab CursTab, trecnum Pos, tattrib Attr, void *Data)
    {
        if (cd_Read_ind(m_cdp, CursTab, Pos, Attr, NOINDEX, Data))
        {
            if (cd_Sz_error(m_cdp) == OUT_OF_TABLE)
                return false;
            throw C602SQLException(m_cdp, L"Read_ind");
        }
        return true;
    }
    t_varcol_size Read_len(tcurstab CursTab, trecnum Pos, tattrib Attr)
    {
        t_varcol_size Result;
        if (cd_Read_len(m_cdp, CursTab, Pos, Attr, NOINDEX, &Result))
            throw C602SQLException(m_cdp, L"Read_len");
        return(Result);
    }
    bool Read_record(tcurstab CursTab, trecnum Pos, void *Data, uns32 Size)
    {
        if (cd_Read_record(m_cdp, CursTab, Pos, Data, Size))
        {
            if (cd_Sz_error(m_cdp) == OUT_OF_TABLE)
                return false;
            throw C602SQLException(m_cdp, L"Read_record");
        }
        return true;
    }
    t_varcol_size Read_var(tcurstab CursTab, trecnum Pos, tattrib Attr, t_varcol_size offset, t_varcol_size size, void *Data)
    {
        t_varcol_size Result;
        if (cd_Read_var(m_cdp, CursTab, Pos, Attr, NOINDEX, offset, size, Data, &Result))
            throw new C602SQLException(m_cdp, L"Read_var");
        return(Result);
    }
    trecnum Rec_cnt(tcurstab CursTab)
    {
        trecnum Result;
        if (cd_Rec_cnt(m_cdp, CursTab, &Result))
            throw new C602SQLException(m_cdp, L"Rec_cnt");
        return(Result);
    }
    void Repl_control(int optype, int opparsize, void * opparam)
    {
        if (cd_Repl_control(m_cdp, optype, opparsize, opparam))
            throw new C602SQLException(m_cdp, L"Repl_control");
    }
    void Restore_table(ttablenum Table, FHANDLE hFile, bool Flag)
    {
        if (cd_Restore_table(m_cdp, Table, hFile, Flag))
            throw new C602SQLException(m_cdp, L"Restore_table");
    }
    void Set_application(const char *ApplName)
    {
        if (cd_Set_application(m_cdp, ApplName))
            throw new C602SQLException(m_cdp, L"Set_application");
    }
    void Set_application(const wchar_t *ApplName)
    {
        tobjname aName;
        Set_application(FromUnicode(m_cdp, ApplName, aName, sizeof(aName)));
    }
    void Set_application_ex(const char *ApplName, bool Ext)
    {
        if (cd_Set_application_ex(m_cdp, ApplName, Ext))
            throw new C602SQLException(m_cdp, L"Set_application_ex");
    }
    void Set_group_role(tobjnum UserGroup, tobjnum GroupRole, tcateg Categ, uns32 Val)
    {
        if (cd_GetSet_group_role(m_cdp, UserGroup, GroupRole, Categ, OPER_SET, &Val))
            throw new C602SQLException(m_cdp, L"GetSet_group_role");
    }
    void Set_privils(tobjnum user_group_role, tcateg subject_categ, ttablenum table, trecnum recnum, t_oper operation, uns8 *privils)
    {
        if (cd_GetSet_privils(m_cdp, user_group_role, subject_categ, table, recnum, operation, privils))
            throw new C602SQLException(m_cdp, L"GetSet_privils");
    }
    void Set_property_value(const char *Owner, const char *Name, const char *Value)
    {
        if (cd_Set_property_value(m_cdp, Owner, Name, 0, Value, -1))
            throw new C602SQLException(m_cdp, L"Set_property_value");
    }
    void Set_property_value(const wchar_t *Owner, const wchar_t *Name, const wchar_t *Value)
    {
        char aOwner[64];
        char aName[64];
        char aValue[256];
        if (cd_Set_property_value(m_cdp, FromUnicode(Owner, aOwner, sizeof(aOwner)), FromUnicode(Name, aName, sizeof(aName)), 0, FromUnicode(Value, aValue, sizeof(aValue)), -1))
            throw new C602SQLException(m_cdp, L"Set_property_value");
    }
    void Set_property_value(const char *Owner, const char *Name, bool Value)
    {
        if (cd_Set_property_value(m_cdp, Owner, Name, 0, Value ? "1" : "0", -1))
            throw new C602SQLException(m_cdp, L"Set_property_value");
    }
    void Set_property_value(const wchar_t *Owner, const wchar_t *Name, bool Value)
    {
        char aOwner[64];
        char aName[64];
        if (cd_Set_property_value(m_cdp, FromUnicode(Owner, aOwner, sizeof(aOwner)), FromUnicode(Name, aName, sizeof(aName)), 0, Value ? "1" : "0", -1))
            throw new C602SQLException(m_cdp, L"Set_property_value");
    }
    void Set_property_value(const char *Owner, const char *Name, int Value)
    {
        char Buf[16];
        sprintf(Buf, "%d", Value);
        if (cd_Set_property_value(m_cdp, Owner, Name, 0, Buf, -1))
            throw new C602SQLException(m_cdp, L"Set_property_value");
    }
    void Set_property_value(const wchar_t *Owner, const wchar_t *Name, int Value)
    {
        char aOwner[64];
        char aName[64];
        char Buf[16];
        sprintf(Buf, "%d", Value);
        if (cd_Set_property_value(m_cdp, FromUnicode(Owner, aOwner, sizeof(aOwner)), FromUnicode(Name, aName, sizeof(aName)), 0, Buf, -1))
            throw new C602SQLException(m_cdp, L"Set_property_value");
    }
    uns32 SQL_execute(const char *cmd)
    {
        uns32 Result;
        if (cd_SQL_execute(m_cdp, cmd, &Result))
            throw new C602SQLException(m_cdp, L"SQL_execute");
        return(Result);
    }
    uns32 SQL_execute(const wchar_t *cmd)
    {
        CBuffer aCmd;
        aCmd.AllocX((int)wcslen(cmd) + 1);
        FromUnicode(m_cdp, cmd, (char *)aCmd, aCmd.GetSize());
        return SQL_execute((const char *)aCmd);
    }
    int Sz_error(){return cd_Sz_error(m_cdp);}
    trecnum Translate(tcursnum Curs, trecnum Rec, int tbord = 0)
    {
        if (cd_Translate(m_cdp, Curs, Rec, tbord, &Rec))
            throw new C602SQLException(m_cdp, L"Translate");
        return(Rec);
    }
    void Unlock_server(){cd_Unlock_server(m_cdp);}
    void Write(tcurstab CursTab, trecnum Pos, tattrib Attr, const void *Data, uns32 Size)
    {
        if (cd_Write(m_cdp, CursTab, Pos, Attr, NULL, Data, Size))
            throw new C602SQLException(m_cdp, L"Write");
    }

    void Write_ind(tcursnum CursTab, trecnum Pos, tattrib Attr, void * Data, uns32 Size)
    {
        if (cd_Write_ind(m_cdp, CursTab, Pos, Attr, NOINDEX, Data, Size))
            throw new C602SQLException(m_cdp, L"Write_ind");
    }

    void Write_len(tcurstab CursTab, trecnum Pos, tattrib Attr, t_varcol_size Size)
    {
        if (cd_Write_len(m_cdp, CursTab, Pos, Attr, NOINDEX, Size))
            throw new C602SQLException(m_cdp, L"Write_len");
    }
	void Write_var(tcurstab CursTab, trecnum Pos, tattrib Attr, t_varcol_size Offset, t_varcol_size Size, const void *Data)
    {
	    if (cd_Write_var(m_cdp, CursTab, Pos, Attr, NOINDEX, Offset, Size, Data))
            throw new C602SQLException(m_cdp, L"Write_var");
    }
	void xSave_table(tcurstab CursTab, FHANDLE hFile)
    {
        if (cd_xSave_table(m_cdp, CursTab, hFile))
            throw new C602SQLException(m_cdp, L"xSave_table");
    }
};

#endif // _API_H_
