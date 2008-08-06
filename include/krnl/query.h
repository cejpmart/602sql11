#ifndef _QUERY_H_
#define _QUERY_H_

#include "api.h"

class C602SQLQuery : public C602SQLApi
{
protected:

    tcurstab m_Handle;

public:

    C602SQLQuery(cdp_t cdp) : C602SQLApi(cdp){m_Handle = NOCURSOR;}
   ~C602SQLQuery()
    {
        if ((m_Handle != NOCURSOR) && IS_CURSOR_NUM(m_Handle))
            cd_Close_cursor(m_cdp, m_Handle);
    }
    void Close_cursor()
    {
        if ((m_Handle != NOCURSOR) && IS_CURSOR_NUM(m_Handle))
        {
            cd_Close_cursor(m_cdp, m_Handle);
            m_Handle = NOCURSOR;
        }
    }
    trecnum Rec_cnt(){return(C602SQLApi::Rec_cnt(m_Handle));}
    bool Read(trecnum Pos, tattrib Attr, void *Data){return C602SQLApi::Read(m_Handle, Pos, Attr, Data);}
    bool Read_ind(trecnum Pos, tattrib Attr, void *Data){return C602SQLApi::Read_ind(m_Handle, Pos, Attr, Data);}
    bool Read_record(trecnum Pos, void *Data, int Size){return C602SQLApi::Read_record(m_Handle, Pos, Data, Size);}
    trecnum Translate(trecnum Rec, int tbord = 0){return(C602SQLApi::Translate(m_Handle, Rec, tbord));}
    void Write_ind(trecnum Pos, tattrib Attr, void * Data, uns32 Size){C602SQLApi::Write_ind(m_Handle, Pos, Attr, Data, Size);}


    void GetSchemaTables(const char *Cond = NULL)
    { 
        static char Sel[] = "SELECT Tab_name,Category,Apl_UUID,Defin,Flags,Folder_name,Last_modif FROM TABTAB WHERE Apl_UUID=X'";
        int    sz = sizeof(Sel) - 1;
        int   csz = Cond ? (int)strlen(Cond) + 1 : 0;
        CBuffer aQuery;
        aQuery.AllocX(sz + UUID_SIZE * 2 + csz + 8);
        memcpy((char *)aQuery, Sel, sz);
        bin2hex((char *)aQuery + sz, m_cdp->sel_appl_uuid, UUID_SIZE);
        sz += 2 * UUID_SIZE;
        aQuery[sz++] = '\'';
        aQuery[sz]   = 0;
        if (Cond)
        {
            aQuery[sz++] = ' ';
            memcpy((char *)aQuery + sz, Cond, csz);
        }
        Close_cursor();
        m_Handle = Open_cursor_direct((const char *)aQuery);
    }

    void GetSchemaObjects(const char *Cond = NULL)
    { 
        static char Sel[] = "SELECT Obj_name,Category,Apl_UUID,Defin,Flags,Folder_name,Last_modif FROM OBJTAB WHERE Apl_UUID=X'";
        int    sz = sizeof(Sel) - 1;
        int   csz = Cond ? (int)strlen(Cond) + 1 : 0;
        CBuffer aQuery;
        aQuery.AllocX(sz + UUID_SIZE * 2 + csz + 8);
        memcpy((char *)aQuery, Sel, sz);
        bin2hex((char *)aQuery + sz, m_cdp->sel_appl_uuid, UUID_SIZE);
        sz += 2 * UUID_SIZE;
        aQuery[sz++] = '\'';
        aQuery[sz]   = 0;
        if (Cond)
        {
            aQuery[sz++] = ' ';
            memcpy((char *)aQuery + sz, Cond, csz);
        }
        Close_cursor();
        m_Handle = Open_cursor_direct((char *)aQuery);
    }
    void Open(const char *Cmd, ...)
    {
        CBuffer Query;
        va_list va;
        va_start(va, Cmd);
        Query.FormatV(Cmd, va);
        m_Handle = Open_cursor_direct((const char *)Query);
    }
    operator tcurstab(){return(m_Handle);}
};

#endif // _QUERY_H_

