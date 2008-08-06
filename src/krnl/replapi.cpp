#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#include "replic.h"

static tcursnum make_repl_cursor(cdp_t cdp, WBUUID src_srv, WBUUID dest_srv, WBUUID appl_id, tptr tabname)
{ char query[120+6*UUID_SIZE+OBJ_NAME_LEN];
  strcpy(query, "SELECT TABNAME FROM REPLTAB WHERE DEST_UUID=X\'");
  int                                           len =(int)strlen(query);
  bin2hex(query+len, dest_srv, UUID_SIZE);
  len+=2*UUID_SIZE;
  strcpy(query+len, "\' AND SOURCE_UUID=X\'");  len+=(int)strlen(query+len);
  bin2hex(query+len, src_srv,  UUID_SIZE);
  len+=2*UUID_SIZE;
  strcpy(query+len, "\' AND APPL_UUID=X\'");    len+=(int)strlen(query+len);
  bin2hex(query+len, appl_id,  UUID_SIZE);
  len+=2*UUID_SIZE;
  if (tabname!=NULL)
  { strcpy(query+len, "\' AND TABNAME=\'");     len+=(int)strlen(query+len);
    strcpy(query+len, tabname);
    len+=(int)strlen(tabname);
  }
  strcpy(query+len, "\'");
  tcursnum cursnum;
  if (cd_Open_cursor_direct(cdp, query, &cursnum))
    return NOOBJECT; 
  return cursnum;
}

CFNC DllKernel trecnum WINAPI find_repl_record(cdp_t cdp, WBUUID src_srv, WBUUID dest_srv, WBUUID appl_id, tptr tabname)
{ trecnum trec=NORECNUM, cnt;
  tcursnum cursnum = make_repl_cursor(cdp, src_srv, dest_srv, appl_id, tabname);
  if (cursnum!=NOOBJECT)
  { if (!cd_Rec_cnt(cdp, cursnum, &cnt))
      if (cnt)
        cd_Translate(cdp, cursnum, 0, 0, &trec);
    cd_Close_cursor(cdp, cursnum);
  }
  return trec;  // NORECNUM on any error
}

static trecnum get_repl_record(cdp_t cdp, WBUUID src_srv, WBUUID dest_srv, WBUUID appl_id, tptr tabname)
{ trecnum trec=find_repl_record(cdp, src_srv, dest_srv, appl_id, tabname);
  if (trec!=NORECNUM) return trec;  // this need not be an error
  cd_Start_transaction(cdp);
  trec=cd_Insert(cdp, REPL_TABLENUM);
  if (trec==NORECNUM) { cd_Roll_back(cdp);  return NORECNUM; }
  cd_Write(cdp, REPL_TABLENUM, trec, REPL_ATR_SOURCE, NULL, src_srv,  UUID_SIZE);
  cd_Write(cdp, REPL_TABLENUM, trec, REPL_ATR_DEST,   NULL, dest_srv, UUID_SIZE);
  cd_Write(cdp, REPL_TABLENUM, trec, REPL_ATR_APPL,   NULL, appl_id,  UUID_SIZE);
  if (tabname!=NULL)
    cd_Write(cdp, REPL_TABLENUM, trec, REPL_ATR_TAB,  NULL, tabname,  UUID_SIZE);
  cd_Commit(cdp);
  return trec;
}

CFNC DllKernel BOOL WINAPI propose_relation(cdp_t cdp, WBUUID dest_srv, WBUUID appl_id, const char * relname, BOOL get_def, tobjnum aplobj)
// Sends relation request, stores the REPLAPL_REQPOSTED and blocked state
{ t_relreq_pars rrp;  apx_header ap;
 // find & store application parameters if application exists locally:
  memset (&ap, 0, sizeof(apx_header)); // used if appl doesn't exist or servers not specified in it
  if (aplobj!=NOOBJECT)
    cd_Read_var(cdp, OBJ_TABLENUM, aplobj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &ap, NULL);
  memcpy(rrp.dp_uuid, ap.dp_uuid, UUID_SIZE);
  memcpy(rrp.os_uuid, ap.os_uuid, UUID_SIZE);
  memcpy(rrp.vs_uuid, ap.vs_uuid, UUID_SIZE);
 // store other parametres of the relation request:
  memcpy(rrp.srv_uuid, dest_srv, UUID_SIZE);
  memcpy(rrp.apl_uuid, appl_id,  UUID_SIZE);
  strcpy(rrp.relname,  relname);
         rrp.get_def=  get_def;
  if (cd_Repl_control(cdp, PKTTP_REL_REQ, sizeof(t_relreq_pars), (tptr)&rrp) && cd_Sz_error(cdp)!=NOT_ANSWERED)
    return FALSE;
 // make local record: state REPLAPL_REQPOSTED, replicating blocked
  WBUUID src_srv;  trecnum rec;
  t_zcr gcr_val;  memset(gcr_val, 0xff, ZCR_SIZE);
  t_replapl ra(REPLAPL_REQPOSTED, relname, get_def);
  cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_UUID, NULL, src_srv);
  rec=get_repl_record(cdp, src_srv, dest_srv, appl_id, NULL);
  if (rec==NORECNUM) return FALSE;
  if (cd_Write_var(cdp, REPL_TABLENUM, rec, REPL_ATR_REPLCOND, NOINDEX, 0, sizeof(t_replapl), &ra)) return FALSE;
  if (cd_Write(    cdp, REPL_TABLENUM, rec, REPL_ATR_GCRLIMIT, NULL, (tptr)gcr_val, ZCR_SIZE)) return FALSE;
 // copy for the other direction:
  rec=get_repl_record(cdp, dest_srv, src_srv, appl_id, NULL);
  if (rec!=NORECNUM)
  { if (cd_Write_var(cdp, REPL_TABLENUM, rec, REPL_ATR_REPLCOND, NOINDEX, 0, sizeof(t_replapl), &ra)) return FALSE;
    if (cd_Write(    cdp, REPL_TABLENUM, rec, REPL_ATR_GCRLIMIT, NULL, (tptr)gcr_val, ZCR_SIZE)) return FALSE;
  }
  return TRUE;
}

char SMTP[]       = "SMTP";
char _MAIL602[]    = "Mail602";
char INPUTDIR[]   = "Input directory";

CFNC DllKernel void WINAPI PackAddrType(char *TypeAddr, const char *Addr, const char *Type)
{
    if (!*Addr)
        *TypeAddr = 0;
    else if (*Type == '>' || !lstrcmpi(Type, INPUTDIR))
        wsprintf(TypeAddr, ">%s", Addr);
    else
        wsprintf(TypeAddr, "{%s}%s", *Type ? Type : _MAIL602, Addr);
}

CFNC DllKernel void WINAPI UnPackAddrType(const char *TypeAddr, char *Addr, char *Type)
{
    const char *p = TypeAddr;
    if (!*TypeAddr)
    {
        *Type = *Addr = 0;
    }
    else if (*TypeAddr == '>')
    {
        lstrcpy(Type, INPUTDIR);
        while (*++p == ' ');
        lstrcpy(Addr, p);
    }
    else if (*TypeAddr == '{')
    {
        while (*++p != '}');
        strncpy(Type, TypeAddr + 1, p - TypeAddr - 1);
        if (lstrcmpi(Type, "Internet") == 0)
            lstrcpy(Type, SMTP);
        lstrcpy(Addr, p + 1);
    }
    else 
    {
        lstrcpy(Addr, TypeAddr);
        while (*p && *p != ',' && *p != '@') 
            p++;
        if (*p == '@')  // internet address
            p = SMTP;
        else 
            p = _MAIL602;
        lstrcpy(Type, p);
    }
}


static BOOL SrvrAppl2ObjUUID(cdp_t cdp, const char *Srvr, const char *Appl, tobjnum *SrvrObj, char *SrvrUUID, tobjnum *ApplObj, char *ApplUUID)
{
    if (cd_Find_object(cdp, Srvr, CATEG_SERVER, SrvrObj)) return TRUE;
    if (   cd_Read(cdp, SRV_TABLENUM, *SrvrObj, SRV_ATR_UUID, NULL, SrvrUUID)) return TRUE;
    if (cd_Find_object(cdp, Appl, CATEG_APPL,   ApplObj)) return TRUE;
    return cd_Read(cdp, OBJ_TABLENUM, *ApplObj, APPL_ID_ATR,  NULL, ApplUUID);
}

CFNC DllKernel BOOL WINAPI Repl_GetSet_local_server_props(cdp_t cdp, t_rpls_oper Oper, void *Prop)
{
    BOOL Get;
    tattrib Attr;
    t_my_server_info si;
    char Buf[256];
    char Addr[256];
    char Type[18];
    WORD  pSize;
    t_varcol_size sisz;
    void *pProp;
    pProp = Prop;
    switch (Oper)
    {
    case RPLS_GET_ENABLED:
        *(char *)Prop = NONEBOOLEAN;
        Attr  = SRV_ATR_ACKN;
        Get   = TRUE;
        break;
    case RPLS_SET_ENABLED:
        Attr  = SRV_ATR_ACKN;
        Get   = FALSE;
       *Buf   = *(char *)Prop ? TRUE : FALSE;
        pProp = Buf;
        pSize = 1;
        break;
    case RPLS_GET_ADDRESS1:
    case RPLS_GET_ADDRTYPE1:
        *(char *)Prop = 0;
        Attr  = SRV_ATR_ADDR1;
        Get   = TRUE;
        pProp = Buf;
        break;
    case RPLS_GET_ADDRESS2:
    case RPLS_GET_ADDRTYPE2:
        *(char *)Prop = 0;
        Attr  = SRV_ATR_ADDR2;
        Get   = TRUE;
        pProp = Buf;
        break;
    case RPLS_SET_ADDRESS1:
    case RPLS_SET_ADDRTYPE1:
        if (cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_ADDR1, NULL, Buf)) return TRUE; 
        UnPackAddrType(Buf, Addr, Type);
        PackAddrType(Buf, Oper == RPLS_SET_ADDRESS1 ? (char *)Prop : Addr,  Oper == RPLS_SET_ADDRTYPE1 ? (char *)Prop : Type); 
        Attr  = SRV_ATR_ADDR1;
        Get   = FALSE;
        pProp = Buf;
        pSize = 254;
        break;
    case RPLS_SET_ADDRESS2:
    case RPLS_SET_ADDRTYPE2:
        if (cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_ADDR2, NULL, Buf)) return TRUE; 
        UnPackAddrType(Buf, Addr, Type);
        PackAddrType(Buf, Oper == RPLS_SET_ADDRESS2 ? (char *)Prop : Addr,  Oper == RPLS_SET_ADDRTYPE2 ? (char *)Prop : Type); 
        Attr  = SRV_ATR_ADDR2;
        Get   = FALSE;
        pProp = Buf;
        pSize = 254;
        break;
    case RPLS_GET_MAILPROF:
    case RPLS_GET_DIPDCONN:
    case RPLS_GET_DIPDUSER:
    case RPLS_GET_INQUEUE:
    case RPLS_GET_OUTQUEUE:
        *(char *)Prop = 0;
        Get   = TRUE;
        pProp = NULL;
        break;
    case RPLS_GET_THREADCNT:
    case RPLS_GET_REPLPERIOD:
        *(DWORD *)Prop = NONEINTEGER;
        Get   = TRUE;
        pProp = NULL;
        break;
    case RPLS_GET_LOGTABLES:
        *(wbbool *)Prop = FALSE;
        Get   = TRUE;
        pProp = NULL;
        break;
    case RPLS_GET_REPLTRIGGERS:
        *(wbbool *)Prop = FALSE;
        Get   = TRUE;
        pProp = NULL;
        break;
    case RPLS_SET_MAILPROF:
    case RPLS_SET_MAILPWORD:
    case RPLS_SET_MAILDPWORD:
    case RPLS_SET_DIPDCONN:
    case RPLS_SET_DIPDUSER:
    case RPLS_SET_DIPDPWORD:
    case RPLS_SET_INQUEUE:
    case RPLS_SET_OUTQUEUE:
    case RPLS_SET_THREADCNT:
    case RPLS_SET_REPLPERIOD:
    case RPLS_SET_LOGTABLES:
    case RPLS_SET_REPLTRIGGERS:
    case RPLS_SET_ERRCAUSESNACK:
        memset(&si, 0, sizeof(t_my_server_info));
        if (cd_Read_var(cdp, SRV_TABLENUM, 0, SRV_ATR_APPLS, NOINDEX, 0, sizeof(t_my_server_info), &si, &sisz)) return TRUE; 
        if (Oper == RPLS_SET_THREADCNT)
            si.input_threads = *(DWORD *)Prop;
        else if (Oper == RPLS_SET_REPLPERIOD)
            si.scanning_period = *(DWORD *)Prop;
        else if (Oper == RPLS_SET_LOGTABLES)
            si.log_tables = *(wbbool *)Prop;
        else if (Oper == RPLS_SET_REPLTRIGGERS)
            si.repl_triggers = *(wbbool *)Prop;
        else if (Oper == RPLS_SET_ERRCAUSESNACK)
            si.err_causes_nack = *(wbbool *)Prop;
        else
        {
            if (Oper == RPLS_SET_MAILPROF)
                pProp = si.profile;
            else if (Oper == RPLS_SET_MAILPWORD)
                pProp = si.mailpword;
            else if (Oper == RPLS_SET_MAILDPWORD)
                pProp = si.dialpword;
            else if (Oper == RPLS_SET_DIPDCONN)
                pProp = si.dipdconn;
            else if (Oper == RPLS_SET_DIPDUSER)
                pProp = si.dipduser;
            else if (Oper == RPLS_SET_DIPDPWORD)
                pProp = si.dipdpword;
            else if (Oper == RPLS_SET_INQUEUE)
            {
                if (!Prop || !*(char *)Prop)
                {
                    client_error(cdp, ERROR_IN_FUNCTION_ARG);
                    return TRUE;
                }
                pProp = si.input_dir;
            }
            else if (Oper == RPLS_SET_OUTQUEUE)
            {
                if (!Prop || !*(char *)Prop)
                {
                    client_error(cdp, ERROR_IN_FUNCTION_ARG);
                    return TRUE;
                }
                pProp = si.output_dir;
            }
            lstrcpy((char *)pProp, (char *)Prop);
        }
        Get   = FALSE;
        pProp = NULL;
        pSize = sizeof(t_my_server_info);
        break;
    default:
        client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE;
    }

    if (Get)
    {
        if (pProp)
        {
            if (cd_Read(cdp, SRV_TABLENUM, 0, Attr, NULL, pProp)) return TRUE; 
        }
        else
        {
            memset(&si, 0, sizeof(t_my_server_info));
            if (cd_Read_var(cdp, SRV_TABLENUM, 0, SRV_ATR_APPLS, NOINDEX, 0, sizeof(t_my_server_info), &si, &sisz)) return TRUE; 
        }
    }
    else
    {
        if (!cd_Am_I_db_admin(cdp) && !cd_Am_I_appl_admin(cdp))
          { client_error(cdp, NO_RIGHT);  return TRUE; }
        if (pProp)
        {
            if (cd_Write(cdp, SRV_TABLENUM, 0, Attr, NULL, pProp, pSize)) return TRUE; 
        }
        else
        {
            if (cd_Write_var(cdp, SRV_TABLENUM, 0, SRV_ATR_APPLS, NOINDEX, 0, sizeof(t_my_server_info), &si)) return TRUE; 
        }
    }

    switch (Oper)
    {
    case RPLS_GET_ADDRESS1:
    case RPLS_GET_ADDRESS2:
        UnPackAddrType(Buf, (char *)Prop, Type);
        break;
    case RPLS_GET_ADDRTYPE1:
    case RPLS_GET_ADDRTYPE2:
        UnPackAddrType(Buf, Addr, (char *)Prop);
        break;
    case RPLS_GET_MAILPROF:
        lstrcpy((char *)Prop, si.profile);
        break;
    case RPLS_GET_DIPDCONN:
        lstrcpy((char *)Prop, si.dipdconn);
        break;
    case RPLS_GET_DIPDUSER:
        lstrcpy((char *)Prop, si.dipduser);
        break;
    case RPLS_GET_INQUEUE:
        lstrcpy((char *)Prop, si.input_dir);
        break;
    case RPLS_GET_OUTQUEUE:
        lstrcpy((char *)Prop, si.output_dir);
        break;
    case RPLS_GET_THREADCNT:
        *(DWORD *)Prop = si.input_threads;
        break;
    case RPLS_GET_REPLPERIOD:
         *(DWORD *)Prop = si.scanning_period;
         break;
    case RPLS_GET_LOGTABLES:
         *(wbbool *)Prop = si.log_tables;
         break;
    case RPLS_GET_REPLTRIGGERS:
         *(wbbool *)Prop = si.repl_triggers;
         break;
    case RPLS_GET_ERRCAUSESNACK:
        *(wbbool *)Prop = si.err_causes_nack;
         break;
    }

    return(FALSE);
}

CFNC DllKernel BOOL WINAPI Repl_GetSet_server_props(cdp_t cdp, const char *Srvr, t_rpsp_oper Oper, void *Prop)
{
    char    buf[256];
    char    dummy[256];
    tobjnum SrvrObj;
    tattrib Attr;
    void   *pProp = Prop;
    
    switch (Oper)
    {
    case RPSP_GET_ADDRESS1:
    case RPSP_GET_ADDRTYPE1:
        *(char *)Prop = 0;
        Attr  = SRV_ATR_ADDR1;
        pProp = buf;
        break;
    case RPSP_GET_ADDRESS2:
    case RPSP_GET_ADDRTYPE2:
        *(char *)Prop = 0;
        Attr  = SRV_ATR_ADDR2;
        pProp = buf;
        break;        
    case RPSP_GET_USEALT:
        *(char *)Prop = NONEBOOLEAN;
        Attr  = SRV_ATR_USEALT;
        break;
    case RPSP_SET_USEALT:
        if (cd_Find_object(cdp, Srvr, CATEG_SERVER, &SrvrObj)) return TRUE;
        return cd_Write(cdp, SRV_TABLENUM, SrvrObj, SRV_ATR_USEALT, NULL, Prop, 1);
    case RPSP_GET_LASTRCVD:
        *(int *)Prop = NONEINTEGER;
        Attr  = SRV_ATR_RECVNUM;
        break;
    case RPSP_GET_LASTSNDD:
        *(int *)Prop = NONEINTEGER;
        Attr = SRV_ATR_SENDNUM;
        break;
    case RPSP_GET_LASTACKD:
        *(int *)Prop = TRUE;
        Attr = SRV_ATR_ACKN;
        break;
    default:
        client_error(cdp, ERROR_IN_FUNCTION_CALL);
        return TRUE;
    }
     
    if (cd_Find_object(cdp, Srvr, CATEG_SERVER, &SrvrObj)) return TRUE;
    if (cd_Read(cdp, SRV_TABLENUM, SrvrObj, Attr, NULL, pProp)) return TRUE;
    if (Oper == RPSP_GET_ADDRESS1 || Oper == RPSP_GET_ADDRESS2)
        UnPackAddrType(buf, (char *)Prop, dummy);
    else if (Oper == RPSP_GET_ADDRTYPE1 || Oper == RPSP_GET_ADDRTYPE2)
        UnPackAddrType(buf, dummy, (char *)Prop);
    return FALSE;
}

CFNC DllKernel BOOL WINAPI Repl_direct_register_server(cdp_t cdp, const char *Addr, const char *Type)
{   char AddrType[256];
    PackAddrType(AddrType, Addr, Type);
    return cd_Repl_control(cdp, PKTTP_SERVER_REQ, lstrlen(AddrType) + 1, AddrType);
}

CFNC DllKernel BOOL WINAPI Repl_GetSet_appl_props(cdp_t cdp, const char * Appl, t_rpap_oper Oper, char *Prop)
{   tobjnum ApplObj, SrvrObj;
    if (Oper == RPAP_GET_TOKENSRVR || Oper == RPAP_GET_CONFLSRVR)
      *(char *)Prop = 0;
    if (cd_Find_object(cdp, Appl, CATEG_APPL, &ApplObj)) return TRUE;
    apx_header ap;
    memset(&ap, 0, sizeof(apx_header));
    if (cd_Read_var(cdp, OBJ_TABLENUM, ApplObj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &ap, NULL)) return TRUE;

    uns8 *puuid;
    BOOL Get;
    switch (Oper)
    {
    case RPAP_GET_TOKENSRVR:
        puuid = ap.dp_uuid;
        Get   = TRUE;
        break;
    case RPAP_SET_TOKENSRVR:
        puuid = ap.dp_uuid;
        Get   = FALSE;
        break;
    case RPAP_GET_CONFLSRVR:
        puuid = ap.os_uuid;
        Get   = TRUE;
        break;
    case RPAP_SET_CONFLSRVR:
        puuid = ap.os_uuid;
        Get   = FALSE;
        break;
    default:
        client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE;
    }

    if (Get)
    {
        if (cd_Find_object_by_id(cdp, puuid, CATEG_SERVER, &SrvrObj)) return TRUE;
        if (cd_Read(cdp, SRV_TABLENUM, SrvrObj, SRV_ATR_NAME, NULL, Prop)) return TRUE;
    }
    else
    {
        if (!cd_Am_I_db_admin(cdp) && !cd_Am_I_appl_admin(cdp))
          { client_error(cdp, NO_RIGHT);  return TRUE; }
        if (cd_Find_object(cdp, Prop, CATEG_SERVER, &SrvrObj))
            return TRUE;
        if (cd_Read(cdp, SRV_TABLENUM, SrvrObj, SRV_ATR_UUID, NULL, puuid))
            return TRUE;
        if (cd_Write_var(cdp, OBJ_TABLENUM, ApplObj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &ap))
            return TRUE;
    }
    return FALSE;
}

CFNC DllKernel BOOL WINAPI Repl_send_share_req(cdp_t cdp, const char *Srvr, const char *Appl, const char *RelName, BOOL Synch)
{
    tobjnum SrvrObj, ApplObj;
    WBUUID  SrvrUUID, ApplUUID;
    if (SrvrAppl2ObjUUID(cdp, Srvr, Appl, &SrvrObj, (char *)SrvrUUID, &ApplObj, (char *)ApplUUID)) return TRUE;
    return !propose_relation(cdp, SrvrUUID, ApplUUID, RelName, Synch ? 2 : 0, ApplObj);
}

CFNC DllKernel BOOL WINAPI Repl_accept_share_req(cdp_t cdp, const char *Srvr, const char *Appl, const char *RelName, BOOL Accept)
{
    tobjnum SrvrObj, ApplObj;
    char buf[2 * UUID_SIZE + OBJ_NAME_LEN + 1];
    if (SrvrAppl2ObjUUID(cdp, Srvr, Appl, &SrvrObj, buf, &ApplObj, buf + UUID_SIZE))
      return TRUE;
    WBUUID MyUUID;
    if (cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_UUID, NULL, MyUUID))
      return TRUE;
    trecnum rec;
    rec = find_repl_record(cdp, MyUUID, *(WBUUID *)buf, *(WBUUID *)(buf + UUID_SIZE), "");
    if (rec == NORECNUM)
      return TRUE;
   // send packet:
    strcpy(buf + 2 * UUID_SIZE, RelName);
    if (cd_Repl_control(cdp, Accept ? PKTTP_REL_ACK : PKTTP_REL_NOACK, sizeof(buf), buf))
      return TRUE;
   // save local info:
    if (Accept)
    {
        t_varcol_size sz;
        t_replapl replapl;
        if (cd_Read_var(cdp, REPL_TABLENUM, rec, REPL_ATR_REPLCOND, NOINDEX, 0, sizeof(replapl), &replapl, &sz))
            return TRUE;
        replapl.state = REPLAPL_BLOCKED;
        if (cd_Write_var(cdp, REPL_TABLENUM, rec, REPL_ATR_REPLCOND, NOINDEX, 0, sizeof(replapl), &replapl))
            return TRUE;
        t_zcr gcr_val;
        memset(gcr_val, 0xff, ZCR_SIZE);
        if (cd_Write(cdp, REPL_TABLENUM, rec, REPL_ATR_GCRLIMIT, NULL, (tptr)&gcr_val, ZCR_SIZE))
            return TRUE;
    }
    else
    {
        cd_Delete(cdp, REPL_TABLENUM, rec);
    }
    return FALSE;
}

CFNC DllKernel BOOL WINAPI Repl_abandon_sharing(cdp_t cdp, const char *Srvr, const char *Appl)
{   tobjnum SrvrObj, ApplObj;
    char    buf[2*UUID_SIZE];
    if (SrvrAppl2ObjUUID(cdp, Srvr, Appl, &SrvrObj, buf, &ApplObj, buf + UUID_SIZE)) return TRUE;
    return cd_Repl_control(cdp, PKTTP_REPL_STOP, sizeof(buf), buf);
}

CFNC DllKernel BOOL WINAPI Repl_Get_state(cdp_t cdp, const char *Srvr, const char *Appl, t_rpst_oper Oper, void *State)
{   WBUUID    SrvrUUID, ApplUUID, MyUUID;
    tobjnum   SrvrObj, ApplObj;

    switch (Oper)  // set the error return values and "NOT FOUND" return value
    { case RPST_STATE:
        *(int *)State = REPLAPL_NOTHING;
        break;
      case RPST_RELNAME:
        *(char *)State = 0;
        break;
      default:
        client_error(cdp, ERROR_IN_FUNCTION_CALL);
        return TRUE;
    }

    if (cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_UUID, NULL, MyUUID))
      return TRUE;
    if (SrvrAppl2ObjUUID(cdp, Srvr, Appl, &SrvrObj, (char *)SrvrUUID, &ApplObj, (char *)ApplUUID))
      return cd_Sz_error(cdp) != OBJECT_NOT_FOUND;
    
    trecnum ApplRec = find_repl_record(cdp, MyUUID, SrvrUUID, ApplUUID, "");
    if (ApplRec == NORECNUM) // not sharing
    { if (Oper==RPST_STATE) *(int *)State = REPLAPL_SRVRREG;
      return FALSE;  
    }
    t_varcol_size sz;  t_replapl replapl;
    if (cd_Read_var(cdp, REPL_TABLENUM, ApplRec, REPL_ATR_REPLCOND, NOINDEX, 0, sizeof(replapl), &replapl, &sz))
      return TRUE;

    switch (Oper)
    { case RPST_STATE:
        *(int *)State = replapl.state;
        break;
    case RPST_RELNAME:
        lstrcpy((char *)State, replapl.relname);
        break;
    }
    return FALSE;
}

CFNC DllKernel BOOL WINAPI Repl_refresh_server_info(cdp_t cdp, const char *Srvr)
{
    tobjnum SrvrObj;
    bool    UseAlt;
    char    Addr[256];
    if (cd_Find_object(cdp, Srvr, CATEG_SERVER, &SrvrObj))
        return TRUE;
    if (cd_Read(cdp, SRV_TABLENUM, SrvrObj, SRV_ATR_USEALT, NULL, (tptr)&UseAlt))
        return TRUE;
    if (cd_Read(cdp, SRV_TABLENUM, SrvrObj, UseAlt ? SRV_ATR_ADDR2 : SRV_ATR_ADDR1, NULL, Addr))
        return TRUE;
    return cd_Repl_control(cdp, PKTTP_SERVER_REQ, lstrlen(Addr) + 1, Addr);
}

CFNC DllKernel BOOL WINAPI Repl_synchronize(cdp_t cdp, const char *Srvr)
{   tobjnum SrvrObj;
    if (cd_Find_object(cdp, Srvr, CATEG_SERVER, &SrvrObj))
      return TRUE;
    return cd_Repl_control(cdp, OPER_RESET_COMM, sizeof(tobjnum), (tptr)&SrvrObj);
}

CFNC DllKernel BOOL WINAPI Repl_resend_packet(cdp_t cdp, const char *Srvr)
{   tobjnum SrvrObj;
    if (cd_Find_object(cdp, Srvr, CATEG_SERVER, &SrvrObj))
      return TRUE;
    return cd_Repl_control(cdp, OPER_RESEND_PACKET, sizeof(tobjnum), (tptr)&SrvrObj);
}

CFNC DllKernel BOOL WINAPI Repl_cancel_packet(cdp_t cdp, const char *Srvr)
{   tobjnum SrvrObj;
    if (cd_Find_object(cdp, Srvr, CATEG_SERVER, &SrvrObj))
      return TRUE;
    return cd_Repl_control(cdp, OPER_CANCEL_PACKET, sizeof(tobjnum), (tptr)&SrvrObj);
}

CFNC DllKernel BOOL WINAPI Repl_appl_shared(cdp_t cdp, const char *Appl)
{
    WBUUID ApplUUID;

    if (!Appl || !*Appl)
    {
        memcpy(ApplUUID, cdp->sel_appl_name, UUID_SIZE);
    }
    else
    {
        tobjnum ApplObj;
        if (cd_Find_object(cdp, Appl, CATEG_APPL, &ApplObj))
            return(NONEBOOLEAN);
        if (cd_Read(cdp, OBJ_TABLENUM, ApplObj, APPL_ID_ATR, NULL, ApplUUID))
            return(NONEBOOLEAN);
    }
    WBUUID my_server_id;
    if (cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_UUID, NULL, my_server_id))
        return(NONEBOOLEAN);
    char Query[128];  int len;
    lstrcpy(Query, "SELECT REPLCOND FROM REPLTAB WHERE SOURCE_UUID=X\'");  len=(int)strlen(Query);
    bin2hex(Query + len, my_server_id, UUID_SIZE);                         len+=2*UUID_SIZE;
    lstrcpy(Query + len, "\' AND APPL_UUID=X\'");                          len=(int)strlen(Query);
    bin2hex(Query + len, ApplUUID, UUID_SIZE);                             len+=2*UUID_SIZE;
    lstrcpy(Query + len, "\' AND TABNAME=\'\'");
    tcursnum Curs;
    if (cd_Open_cursor_direct(cdp, Query, &Curs))
        return(NONEBOOLEAN);
    BOOL Err = TRUE;
    t_replapl replapl;
    replapl.state = REPLAPL_NOTHING;
    trecnum rCnt;
    if (!cd_Rec_cnt(cdp, Curs, &rCnt))
    {
         for (trecnum r = 0; r < rCnt; r++)
         {
             t_varcol_size sz;
             if (cd_Read_var(cdp, Curs, r, 1, NOINDEX, 0, sizeof(replapl), &replapl, &sz))
                 goto exit;
             if (replapl.state == REPLAPL_IS)
                 break;
         }
         Err = FALSE;
    }

exit:

    cd_Close_cursor(cdp, Curs);
    if (Err)
        return(NONEBOOLEAN);
    return(replapl.state == REPLAPL_IS);
}

