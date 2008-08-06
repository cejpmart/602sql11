// trctbl.cpp

CTrcTblVal *CTrcTblRow :: AddVal(int Type, LPCVOID Val, int Size)
{
    CTrcTblVal *Item = (CTrcTblVal *)corealloc(sizeof(CTrcTblVal) + m_MaxValSz, 13);
    if (!Item)
        return(FALSE);
 
    Item->m_Next = NULL;
    Item->SetVal(Type, Val, Size, m_MaxValSz);
    
   *m_Tail =  Item;
    m_Tail = &Item->m_Next;
    return(Item);
}

CTrcTblVal *CTrcTblRow :: InsertVal(CTrcTblVal *After, int Type, LPCVOID Val, int Size)
{
    CTrcTblVal *Item = (CTrcTblVal *)corealloc(sizeof(CTrcTblVal) + m_MaxValSz, 13);
    if (!Item)
        return(FALSE);
 
    Item->SetVal(Type, Val, Size, m_MaxValSz);
    
    if (!After)
    {
        if (m_Hdr)
        {
            Item->m_Next = m_Hdr;
            m_Hdr        = Item;
        }
        else
        {
           *m_Tail       =  Item;
            m_Tail       = &Item->m_Next;
            Item->m_Next = NULL;
        }
    }
    else
    {
        Item->m_Next  = After->m_Next;
        After->m_Next = Item;
        if (m_Tail == &After->m_Next)
        {
            *m_Tail =  Item;
             m_Tail = &Item->m_Next;
        }
    }
    return(Item);
}

void CTrcTblVal :: SetVal(int Type, LPCVOID Val, int Size, int MaxSize)
{
    switch (Type)
    {
    case ATT_BOOLEAN:
        Bool2a(*(bool *)Val);
        break;
    case ATT_CHAR:
        Char2a(*(char *)Val);
        break;
    case ATT_INT8:
        Tiny2a(*(sig8 *)Val);
        break;
    case ATT_INT16:
        Short2a(*(sig16 *)Val);
        break;
    case ATT_INT32:
        Int2a(*(sig32 *)Val);
        break;
    case ATT_PTR:
    case ATT_BIPTR:
        Uns2a(*(uns32 *)Val);
        break;
    case ATT_INT64:
        Bigint2a(*(sig64 *)Val);
        break;
    case ATT_MONEY:
        Mon2a((monstr *)Val);
        break;
    case ATT_FLOAT:
        Real2a((double *)Val);
        break;
    case ATT_STRING:
    case ATT_TEXT:
        Str2a((char *)Val, Size, MaxSize);
        break;
    case ATT_BINARY:
    case ATT_AUTOR:
        Bin2a((char *)Val, Size, MaxSize);
        break;
    case ATT_DATE:
        Date2a(*(DWORD *)Val);
        break;
    case ATT_TIME:
        Time2a(*(DWORD *)Val);
        break;
    case ATT_TIMESTAMP:
    case ATT_DATIM:
        Tstmp2a(*(DWORD *)Val);
        break;
    default:
        m_Val[0] = 0;
        break;
    }
}


#define MAX_TRACE_ROW_LEN 120

void CTrcTbl :: Trace(cdp_t cdp, unsigned trace_type)
{
    tptr xbuf = NULL;  // allocate when used for the 1st time

    ProtectedByCriticalSection cs(&cs_trace_logs, cdp, WAIT_CS_TRACE_LOGS);
    // write the message to log(s):
    for (t_log_request * req = pending_log_request_list;  req;  req=req->next)
    {
        if (req->situation==trace_type && req->user_ok(cdp->prvs.luser()))
        { 
            // find the log description:
            if (!xbuf)
            {
                xbuf = (tptr)corealloc(MAX_TRACE_MSG_LEN, 67);
                if (!xbuf)
                    return;
            }
            t_trace_log * alog = find_trace_log(req->log_name);
            if (!alog)
            { 
                alog = find_trace_log("");
                if (alog)
                {
                    char buf[30 + OBJ_NAME_LEN];  
                    sprintf(buf, TRACELOG_NOTFOUND, req->log_name); 
                    alog->get_error_envelope(cdp, xbuf, MAX_TRACE_MSG_LEN, trace_type, buf);
                    alog->output(xbuf);
                    alog->message_cache.add_message(xbuf);
                    write_to_server_console(cdp, xbuf);
                }
                continue;
            }

            alog->get_error_envelope(cdp, xbuf, MAX_TRACE_MSG_LEN, trace_type, "");
            char *ph = xbuf + strlen(xbuf);
            int Col;
            for (int NextCol = 0; NextCol < GetColCnt(); NextCol = Col)
            {
                for (int r = 0; r < m_RowCnt; r++)
                {
                    Col = BuildFPRow(xbuf, r, NextCol);
                    alog->output(xbuf);
                    alog->message_cache.add_message(xbuf);
                    if (!*req->log_name)
                    {
#ifdef ANY_SERVER_GUI
                        *ph = 0;
                        BuildVPRow(xbuf, r, NextCol, Col);
#endif
                      write_to_server_console(cdp, xbuf);
                    }
                    ph    = xbuf;
                    *xbuf = 0;
                }
            }
        }
    }
    if (xbuf)
        corefree(xbuf);
}

int CTrcTbl :: BuildFPRow(char *MsgHdr, int Rw, int Col0)
{
    CTrcTblRow *Row;
    CTrcTblVal *Item;
    int        *pSize;
    int         sz;

    if (!m_FPSize)
    {
        m_FPSize = (int *)corealloc((GetColCnt() + 1) * sizeof(int), 14);
        if (!m_FPSize)
            return(m_ColCnt);
        memset(m_FPSize, 0, (m_ColCnt + 1) * sizeof(int));
        m_FPSize[0] = (int)strlen(MsgHdr);
        for (Row = m_Row; Row < m_Row + m_RowCnt; Row++)
        {
            for (Item = Row->m_Hdr, pSize = m_FPSize + 1; Item; Item = Item->m_Next, pSize++)
            {
                sz = (int)strlen(Item->m_Val) + 2;
                if (*pSize < sz)
                    *pSize = sz;
            }
        }
    }

    int Col;
    Row = m_Row + Rw;
    for (Item = Row->m_Hdr, Col = 0; Col < Col0; Item = Item->m_Next, Col++);
    sz = (int)strlen(MsgHdr);
    char *Buf = MsgHdr + sz;
    for (pSize = m_FPSize + Col; Item ; Item = Item->m_Next, pSize++, Col ++)
    {
        sz = *pSize - sz;
        memset(Buf, ' ', sz);
        Buf += sz;
        strcpy(Buf, Item->m_Val);
        sz = (int)strlen(Item->m_Val);
        Buf += sz;
        if (Buf > MsgHdr + m_FPSize[0] + MAX_TRACE_ROW_LEN)
            break;
    }
    return(Col + 1);
}

#ifdef ANY_SERVER_GUI

extern HWND hInfoWnd;
#include "server.h"

void CTrcTbl :: BuildVPRow(char *MsgHdr, int Rw, int Col0, int ColMax)
{
    CTrcTblRow *Row;
    CTrcTblVal *Item;
    SIZE        sz;
    int        *pSize;

    HWND hList = GetDlgItem(hInfoWnd, CD_SI_LOG);
    HDC  hDC   = GetDC(hList);
    if (!hDC)
        return;

    HGDIOBJ hFont = SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));

    if (!m_VPSize)
    {
        SIZE SPSz;
        m_VPSize = (int *)corealloc((m_ColCnt + 1) * sizeof(int), 15);
        if (!m_VPSize)
            goto exit;
        GetTextExtentPoint32(hDC, "  ", 2, &SPSz);
        memset(m_VPSize, 0, (m_ColCnt + 1) * sizeof(int));
        GetTextExtentPoint32(hDC, MsgHdr, (int)strlen(MsgHdr), &sz);
        m_VPSize[0] = sz.cx;
        for (Row = m_Row; Row < m_Row + m_RowCnt; Row++)
        {
            for (Item = Row->m_Hdr, pSize = m_VPSize + 1; Item; Item = Item->m_Next, pSize++)
            {
                GetTextExtentPoint32(hDC, Item->m_Val, (int)strlen(Item->m_Val), &sz);
                if (*pSize < sz.cx)
                    *pSize = sz.cx;
            }
        }
        for (int i = 1; i <= m_ColCnt; i++)
            m_VPSize[i] += m_VPSize[i - 1] + SPSz.cx;
    }

    int Col;
    Row = m_Row + Rw;
    for (Item = Row->m_Hdr, Col = 0; Col < Col0; Item = Item->m_Next, Col++);
    int Len;
    Len = (int)strlen(MsgHdr);
    GetTextExtentPoint32(hDC, MsgHdr, Len, &sz);
    int Pos;
    Pos = sz.cx;
    char *Buf;
    Buf = MsgHdr + Len;
    for (pSize = m_VPSize + Col; Item && Col < ColMax ; Item = Item->m_Next, pSize++, Col++)
    {
        while (sz.cx < *pSize)
        {
            *Buf++ = ' ';
            Len++;
            GetTextExtentPoint32(hDC, MsgHdr, Len, &sz);
        }
        strcpy(Buf, Item->m_Val);
        int s = (int)strlen(Item->m_Val); 
        Len += s;
        Buf += s;
    }
    if (Col == ColMax && Rw == m_RowCnt - 1)
    {
        for (Len = m_VPSize[Col] - m_VPSize[0]; Col < m_ColCnt; Col++)
            m_VPSize[Col + 1] -= Len;
    }

exit:

    SelectObject(hDC, hFont);
    ReleaseDC(hList, hDC);
}

#endif // ANY_SERVER_GUI

