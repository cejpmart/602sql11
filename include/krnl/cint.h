/****************************************************************************/
/* CINT.H - header & definition file for Sezam - C language interface       */
/****************************************************************************/
#ifndef CINT_INCLUDED
#define CINT_INCLUDED
#ifndef GENERAL_DEF
#include "defs.h"
#endif

#include "wbkernel.h"

extern int task_switch_enabled;
extern int client_tzd;  // time zone displacement in seconds
/******************************* internal services **************************/
extern "C" {
DllKernel int  WINAPI program_flags(cdp_t cdp, tptr src);

DllKernel void WINAPI inval_table_d(cdp_t cdp, tobjnum tb, tcateg cat);
DllKernel BOOL WINAPI Lock_kernel    (void);
DllKernel void WINAPI Unlock_kernel  (void);
DllKernel BOOL WINAPI Unlock_user(BOOL objects);
#ifdef STOP  // removed from WB 8.0
DllKernel BOOL WINAPI Get_users(uns16 maxusers, tobjnum * userbuf);
DllKernel BOOL WINAPI Get_locks(uns16 maxlocks, lockinfo * lockbuf);
#endif
DllKernel BOOL WINAPI Remove_lock(lockinfo * llock);
DllKernel BOOL WINAPI cd_Remove_lock(cdp_t cdp, lockinfo * llock);
DllKernel BOOL WINAPI cd_Db_copy(cdp_t cdp, ttablenum t1, trecnum r1, tattrib a1, uns16 ind1,
             ttablenum t2, trecnum r2, tattrib a2, uns16 ind2);
DllKernel BOOL WINAPI PtrSteps(tcursnum cursor, trecnum   position, tattrib attr,
                            uns16 index, void * buffer);
DllKernel BOOL WINAPI Delete_history(ttablenum tb, BOOL datim_type, timestamp dtm);
DllKernel BOOL WINAPI Recstate(cdp_t cdp, tcursnum cursor, trecnum rec, uns8 * state);
DllKernel BOOL WINAPI cd_Table_stat(cdp_t cdp, ttablenum table, trecnum * stat);
DllKernel BOOL WINAPI Add_value(tcursnum cursor, trecnum position, tattrib attr, void * buf, uns16 datasize);
DllKernel BOOL WINAPI Del_value(tcursnum cursor, trecnum position, tattrib attr, void * buf, uns16 datasize);
DllKernel BOOL WINAPI Remove_invalid(tcursnum cursor, trecnum position, tattrib attr);
DllKernel BOOL WINAPI cd_xSave_table(cdp_t cdp, ttablenum table, FHANDLE hFile);
DllKernel BOOL WINAPI cd_Restore_table(cdp_t cdp, ttablenum table, FHANDLE hFile, BOOL flag);
DllKernel BOOL WINAPI cd_Get_valid_recs(cdp_t cdp, tcursnum cursnum, trecnum startrec,
    trecnum limit, void * map, trecnum * totrec, trecnum * valrec);
DllKernel BOOL WINAPI cd_Text_substring(cdp_t cdp, tcursnum cursor, trecnum position, tattrib attr, uns16 index,
   tptr pattern, uns16 pattlen, uns32 start, uns16 flags, uns32 * stringpos);
DllKernel BOOL WINAPI cd_Get_descriptor(cdp_t cdp, tobjnum curs, tcateg cat, d_table ** td);
DllKernel BOOL WINAPI cd_Apl_id2name(cdp_t cdp, const uns8 * appl_id, char * name);
DllKernel BOOL WINAPI cd_Apl_name2id(cdp_t cdp, const char * name, uns8 * appl_id);
DllKernel BOOL WINAPI cd_Bck_replay(cdp_t cdp, tobjnum bad_user, timestamp dttm, uns32 * stat);
DllKernel BOOL WINAPI Bck_get_time(timestamp * dttm);
DllKernel BOOL WINAPI cd_Send_params(cdp_t cdp, uns32 stmt, DWORD len, void * value);
DllKernel BOOL WINAPI cd_Send_parameter(cdp_t cdp, uns32 stmt, sig16 parnum, uns32 len, uns32 offset, void * value);
DllKernel BOOL WINAPI cd_Pull_params(cdp_t cdp, uns32 handle, sig16 code, void ** answer);
DllKernel BOOL WINAPI cd_Pull_parameter(cdp_t cdp, uns32 handle, int parnum, int start, int length, char * buf);
DllKernel BOOL WINAPI Get_error_info(uns32 * line, uns16 * column, tptr buf);
DllKernel BOOL WINAPI cd_Get_error_info(cdp_t cdp, uns32 * line, uns16 * column, tptr buf);
DllKernel BOOL WINAPI cd_SQL_catalog(cdp_t cdp, BOOL metadata, unsigned ctlg_type,
                             unsigned parsize, tptr inpars, uns32 * result);
DllKernel BOOL WINAPI Server_access(cdp_t cdp, char * buf);
DllKernel BOOL WINAPI Cursor_quality(tcursnum curs, trecnum * recnum, trecnum * falsecond, trecnum * truecond);
DllKernel BOOL WINAPI Back_count     (uns32 * count);
DllKernel BOOL WINAPI cd_Get_base_tables(cdp_t cdp, tcursnum curs, uns16 * cnt, ttablenum * tabs);
DllKernel BOOL WINAPI BrowseStart(HWND hServTree);
DllKernel void WINAPI BrowseStop(HWND hServTree);
DllKernel BOOL WINAPI DetectLocalServer(const char * loc_server_name);  // obsolete, replaced by srv_Get_state_local()

DllKernel BOOL WINAPI cd_Export_user(cdp_t cdp, tobjnum userobj, FHANDLE hFile);
DllKernel BOOL WINAPI cd_Import_user(cdp_t cdp, FHANDLE hFile);
DllKernel BOOL WINAPI SQL_get_result_set_info(uns32 handle, unsigned rs_order, d_table ** td);
DllKernel BOOL WINAPI SQL_get_param_info(uns32 handle, void ** pars);
DllKernel BOOL WINAPI cd_SQL_get_result_set_info(cdp_t cdp, uns32 handle, unsigned rs_order, d_table ** td);
DllKernel BOOL WINAPI cd_SQL_get_param_info(cdp_t cdp, uns32 handle, void ** pars);
DllKernel BOOL WINAPI cd_CD_ROM(cdp_t cdp);
DllKernel BOOL WINAPI cd_Enable_refint(cdp_t cdp, BOOL enable);
#ifdef STOP  // not used any more
DllKernel BOOL WINAPI cd_Clear_passwords(cdp_t cdp);
#endif
DllKernel int  WINAPI tcpip_echo(char * server_name);
DllKernel int  WINAPI tcpip_echo_addr(const char * ip_addr);
DllKernel BOOL WINAPI cd_Server_debug(cdp_t cdp, int oper, tobjnum objnum, int line, int * state, tobjnum * pobjnum, int * pline);
DllKernel BOOL WINAPI cd_Server_eval(cdp_t cdp, int frame, const char * expr, void * value);
DllKernel BOOL WINAPI cd_Server_stack(cdp_t cdp, t_stack_info ** value);
DllKernel BOOL WINAPI cd_Server_stack9(cdp_t cdp, t_stack_info9 ** value);
DllKernel BOOL WINAPI cd_Server_assign(cdp_t cdp, int frame, const char * dest, const char * value);
DllExport BOOL WINAPI erase_replica(cdp_t cdp);
void scan_objects(cdp_t cdp, tcateg cat, BOOL extended);
DllKernel BOOL WINAPI cd_Trigger_changed(cdp_t cdp, tobjnum trigobj, BOOL after);
DllKernel BOOL WINAPI cd_Procedure_changed(cdp_t cdp);
struct t_trig_info
{ uns32 trigger_type;
  tobjname name;
};
DllKernel BOOL WINAPI cd_List_triggers(cdp_t cdp, ttablenum tb, t_trig_info ** trgs);
DllKernel BOOL WINAPI cd_Send_fulltext_words(cdp_t cdp, const char * ft_label, t_docid32 docid, unsigned startpos, const char * buf, unsigned bufsize);
DllKernel BOOL WINAPI cd_Send_fulltext_words64(cdp_t cdp, const char * ft_label, t_docid64 docid, unsigned startpos, const char * buf, unsigned bufsize);
DllKernel BOOL WINAPI cd_Make_persistent(cdp_t cdp, tcursnum cursnum);
DllKernel BOOL WINAPI cd_Create3_link(cdp_t cdp, const char * sourcename, const uns8 * sourceapplid, tcateg category, const char * linkname, BOOL limited);
DllKernel BOOL WINAPI cd_Query_optimization(cdp_t cdp, const char * query, BOOL evaluate, tptr * result);
DllKernel BOOL WINAPI cd_Find_prefixed_object(cdp_t cdp, const char * schema, const char * objname, tcateg category, tobjnum * position);
DllKernel BOOL WINAPI cd_Crash_server(cdp_t cdp);

typedef void CALLBACK t_client_error_callback(const char * text);
extern t_client_error_callback * client_error_callback;
DllKernel void WINAPI set_client_error_callback(t_client_error_callback * client_error_callbackIn);
DllKernel void WINAPI set_client_error_callback2(cdp_t cdp, t_client_error_callback2 * client_error_callbackIn);
DllKernel void WINAPI set_client_error_callbackW(cdp_t cdp, t_client_error_callbackW * client_error_callbackWIn);
DllKernel void WINAPI client_error(xcd_t * xcdp, int num);
DllKernel void client_error_param(cdp_t cdp, int num, ...);
DllKernel void client_error_message(cdp_t cdp, const char * msg, ...);
DllKernel void WINAPI client_error_generic(xcd_t * xcdp, int errnum, const char * msg);
DllKernel void WINAPI client_error_genericW(xcd_t * xcdp, int errnum, const wchar_t * msg);
#if WBVERS >= 900
DllKernel BOOL WINAPI Get_error_num_textW(xcd_t * xcdp, int err, wchar_t * buf, unsigned buflen);
DllKernel BOOL WINAPI Get_warning_num_textW(cdp_t cdp, int wrnnum, wchar_t * buf, unsigned buflen);
#endif
CFNC DllKernel BOOL WINAPI Get_warning_num_text(cdp_t cdp, int wrnnum, char * buf, unsigned buflen);

DllKernel BOOL WINAPI propose_relation(cdp_t cdp, WBUUID dest_srv, WBUUID appl_id, const char * relname, BOOL get_def, tobjnum aplobj);
DllKernel trecnum WINAPI find_repl_record(cdp_t cdp, WBUUID src_srv, WBUUID dest_srv, WBUUID appl_id, tptr tabname);
DllKernel void WINAPI UnPackAddrType(const char *TypeAddr, char *Addr, char *Type);
DllKernel void WINAPI PackAddrType(char *TypeAddr, const char *Addr, const char *Type);
DllKernel BOOL WINAPI cd_Logout_par(cdp_t cdp, unsigned flags);
#define LOGIN_FLAG_LIMITED   1
#define LOGIN_FLAG_SIMULATED 2
#define LOGIN_FLAG_HTTP      4
#if WBVERS<900
DllKernel BOOL WINAPI cd_Stop_server(cdp_t cdp);
#endif
#define INTEGR_CHECK_INDEXES                   0x20
#define INTEGR_CHECK_CACHE                     0x40
DllKernel BOOL WINAPI cd_Check_indexes(cdp_t cdp, int extent, BOOL repair, uns32 * index_err, uns32 * cache_err);

typedef void CALLBACK t_server_scan_callback(const char * server_name, uns32 ip, uns16 port);
DllKernel void WINAPI set_server_scan_callback(t_server_scan_callback * new_server_scan_callback);

#if WBVERS>=900
typedef void (CALLBACK t_notification_callback)(cdp_t cdp, int notification_type, void * parameter);
extern t_notification_callback * notification_callback;
DllKernel void WINAPI Set_notification_callback(t_notification_callback *CallBack);
DllKernel void WINAPI process_notifications(cdp_t cdp);
#endif
DllKernel BOOL WINAPI cd_Store_lic_num(cdp_t cdp, const char * lic, const char * company, const char * computer, char ** request);
DllKernel BOOL WINAPI cd_Store_activation(cdp_t cdp, char * response);

#if WBVERS>=1000
DllKernel int WINAPI RebuildDb_export(cdp_t cdp, const char * destdir, LPIMPEXPAPPLCALLBACK callback, void * callback_param, HANDLE * handle);
DllKernel int WINAPI RebuildDb_import(HANDLE handle);
DllKernel int WINAPI RebuildDb_move(HANDLE handle, const char * destdir);
DllKernel void WINAPI RebuildDb_close(HANDLE handle, BOOL erase_export);
#endif
DllKernel BOOL WINAPI cd_Set_lobrefs_export(cdp_t cdp, BOOL refs_only);

#define MAX_SERVICE_NAME 60 
///////////////////////// functions from other modules //////////////////////
#define SEARCH_CASE_SENSITIVE   1
#define SEARCH_WHOLE_WORDS      2
#define SEARCH_BACKWARDS        4
#define SEARCH_IN_BLOCK         0x100
#define SEARCH_FROM_CURSOR      0x200
#define SEARCH_AUTOREPEAT       0x400
#define SEARCH_DOREPLACE        0x4000
#define SEARCH_NOCONT           0x8000
DllKernel BOOL WINAPI Search_in_block(const char * block, uns32 blocksize, const char * next, const char * patt, 
                      uns16 pattsize, uns16 flags, uns32 * respos);
DllKernel bool WINAPI Search_in_blockA(const char * block, uns32 blocksize, const char * next, const char * patt, 
                      int pattsize, unsigned flags, int charset, uns32 * respos);
DllKernel bool WINAPI Search_in_blockW(const wchar_t * block, uns32 blocksize, const wchar_t * next, const wchar_t * patt, 
                      int pattsize, unsigned flags, int charset, uns32 * respos);
DllKernel void WINAPI wininichange(char * section);
DllKernel BOOL WINAPI get_server_path(char * pathname);
DllKernel unsigned char * WINAPI load_certificate_data(const tobjname server_name, unsigned * length);
DllKernel BOOL WINAPI erase_certificate(const tobjname server_name);
DllKernel bool WINAPI convert_to_SQL_literal(tptr dest, tptr source, int type, t_specif specif, bool add_eq);
DllKernel bool WINAPI cd_convert_to_SQL_literal(cdp_t cdp, tptr dest, tptr source, int type, t_specif specif, bool add_eq);
DllKernel void * WINAPI get_native_variable(cdp_t cdp, const char * variable_name, int & type, t_specif & specif, int & vallen);
DllKernel BOOL WINAPI cd_is_dead_connection(cdp_t cdp);
DllKernel BOOL WINAPI is_null(tptr val, uns8 type);
/******************************** odbc ***************************************/
#if WBVERS>=900
#define CLIENT_ODBC_9
#endif

#ifdef CLIENT_ODBC_9
#define MAX_ODBC_NAME_LEN 255
DllKernel d_table * WINAPI make_odbc_descriptor9(t_connection * conn, const char * tabname, const char * prefix, BOOL WB_types);
DllKernel d_table * WINAPI make_result_descriptor9(t_connection * conn, HSTMT hStmt);
DllKernel ltable * WINAPI create_cached_access9(cdp_t cdp, t_connection *	conn, const char * source, const char * tablename, const char * odbc_prefix, tcursnum cursnum);
DllKernel unsigned WINAPI odbc_tpsize(int type, t_specif specif);
DllKernel RETCODE WINAPI create_odbc_statement(t_connection *	conn, HSTMT * hStmt);
DllKernel ltable * WINAPI create_cached_access_for_stmt(t_connection * conn, HSTMT hStmt, const d_table * td);
//DllKernel void WINAPI log_odbc_stmt_error(HSTMT hStmt, cdp_t cdp = NULL);
//DllKernel void WINAPI log_odbc_dbc_error(t_connection * conn);
DllKernel bool WINAPI odbc_error(t_connection * conn, HSTMT hStmt);
DllKernel void WINAPI pass_error_info(t_connection * conn, cdp_t cdp);
DllKernel BOOL WINAPI odbc_synchronize_view9(BOOL update, t_connection * conn,
  ltable * dt, trecnum extrec, tptr new_vals, tptr old_vals, BOOL on_current);
DllExport void WINAPI prepare_insert_statement(ltable * dt, const d_table * dest_odbc_td);
//DllKernel void WINAPI ODBC9_release_env(void);
DllKernel BOOL WINAPI ODBC_data_sources(bool first, const char * dsn, int dsn_space, const char * descr, int descr_space);
//DllKernel t_connection * WINAPI ODBC9_connect(const char * dsn, const char * uid, const char * pwd, void * window_handle);
//DllKernel void WINAPI ODBC9_disconnect(t_connection * conn);
DllKernel xcd_t * WINAPI find_connection(const char * dsn);
DllKernel int  WINAPI odbc_delete_all9(t_connection * conn, HSTMT hStmt, tptr select_st);
DllKernel void WINAPI make_full_table_name(xcdp_t xcdp, t_flstr & dest, const char * table_name, const char * schema_name);
#else
DllKernel void WINAPI odbc_stmt_error(cdp_t cdp, HSTMT hStmt);
DllKernel void WINAPI odbc_dbc_error(cdp_t cdp, HDBC hDbc);
DllKernel void WINAPI odbc_env_error(cdp_t cdp);
DllKernel int  WINAPI odbc_delete_all(cdp_t cdp, HSTMT hStmt, tptr select_st);
#endif

#if 1
DllKernel BOOL WINAPI Unlink_odbc_table(cdp_t cdp, ttablenum tb);
DllKernel t_connection * WINAPI connect_data_source(cdp_t cdp,
  const char * dsn, BOOL use_old_access, BOOL temporary);
DllKernel t_connection * WINAPI find_data_source(cdp_t cdp, const char * dsn);
DllKernel ltable * WINAPI get_ltable(cdp_t cdp, tcursnum tabnum);
DllKernel unsigned WINAPI get_object_flags(cdp_t cdp, tcateg category, ctptr name);
DllKernel BOOL WINAPI comp_dynar_search(t_dynar * d_comp, ctptr name, unsigned * pos);
DllKernel t_dynar * WINAPI get_object_cache(cdp_t cdp, tcateg category);
DllKernel tobjnum WINAPI object_to_cache(cdp_t cdp, t_dynar * d_comp,
   tcateg cat, ctptr name, tobjnum objnum, uns16 flags, unsigned * cache_index = NULL);
void sort_comp_dynar(t_dynar * d_comp);
DllKernel d_table * WINAPI make_odbc_descriptor(t_connection * conn, t_odbc_link * odbc_link, BOOL WB_types);
DllKernel ltable * WINAPI create_cached_access(cdp_t cdp, t_connection * conn,
                   tptr source, BOOL source_is_table, tcursnum cursnum);
DllKernel void WINAPI destroy_cached_access(ltable * dt);
DllKernel tcursnum WINAPI get_new_odbc_tabnum(cdp_t cdp);
DllKernel odbc_tabcurs * WINAPI get_odbc_tc(cdp_t cdp, tcursnum curs);
DllKernel tptr WINAPI srch(tptr source, tptr patt);
DllKernel void WINAPI merge_condition(tptr dest, tptr source, tptr cond);
DllKernel void WINAPI free_connection(cdp_t cdp, t_connection * conn);
DllKernel void WINAPI remove_temp_conns(cdp_t cdp);

DllKernel tptr WINAPI odbc_table_to_source(table_all * ta, BOOL altering, t_connection * conn);
DllKernel BOOL WINAPI map_tables(cdp_t cdp, t_connection * conn);
DllKernel void WINAPI start_mapping(cdp_t cdp);
DllKernel void WINAPI make_apl_query(char * query, const char * tabname, const uns8 * uuid);
d_table * make_result_descriptor(ltable * ltab);
DllKernel int WINAPI get_time_zone_info(void);
DllKernel BOOL WINAPI cd_Admin_mode(cdp_t cdp, tobjnum objnum, BOOL mode);
DllKernel t_folder_index WINAPI search_or_add_folder(cdp_t cdp, const char * fldname, tobjnum objnum);
DllKernel int TcpIpQueryForServer(void);
DllExport BOOL TcpIpQueryInit(void);
DllExport BOOL TcpIpQueryDeinit(void);
DllKernel void WINAPI Upcase9(cdp_t cdp, char * txt);
DllKernel BOOL WINAPI Get_server_error_context_text(cdp_t cdp, int level, char * buffer, int buflen);
DllKernel BOOL WINAPI cd_Server_eval9(cdp_t cdp, int frame, const char * expr, wuchar * value);
DllKernel void WINAPI client_status_nums(trecnum num1, trecnum num2);
DllKernel const wchar_t *Get_transl(cdp_t cdp, const wchar_t *msg, wchar_t *buf);
DllKernel void WINAPI ident_to_sql(xcdp_t xcdp, char * dest, const char * name);
DllKernel void WINAPI make_full_table_name(xcdp_t xcdp, t_flstr & dest, const char * table_name, const char * schema_name);
DllKernel void WINAPI Enable_server_search(BOOL enable);
DllKernel BOOL WINAPI cd_Mask_duplicity_error(cdp_t cdp, BOOL mask);
DllKernel void WINAPI Convert_error_to_generic(cdp_t cdp);

#endif /* WINS */
DllKernel int GetHostCharSet();
#if WBVERS>=900
DllKernel void WINAPI catr_set_null(tptr pp, int type);
#endif

 }      /* of extern "C" */

void cd_unlink_kernel(cdp_t cdp);

const char *FromUnicode(const wchar_t *Src, char *Buf, int Size);
const char *FromUnicode(cdp_t cdp, const wchar_t *Src, char *Buf, int Size);
const wchar_t *ToUnicode(const char *Src, wchar_t *Buf, int Size);
const wchar_t *ToUnicode(cdp_t cdp, const char *Src, wchar_t *Buf, int Size);

extern t_translation_callback * global_translation_callback;
extern CRITICAL_SECTION cs_short_term;

CFNC DllKernel uns32  WINAPI free_sum(void);  // obsolete
char * append(char * dest, const char * src);  // appends with single PATH_SEPARATOR

#endif  // CINT_INCLUDED


