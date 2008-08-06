
/****************************************************************************/
/* wbkernel.h - definice pro komunikaci se serverem 602SQL                  */
/* (C) Janus Drozd, 1992-2008                                               */
/* verze: 0.0 (32-bit)                                                      */
/****************************************************************************/
#ifndef __WBKERNEL_H__
#define __WBKERNEL_H__

/* If we are on Windows or we have already included the winrepl definitions,
 * fine, otherwise we have to make some definitions.
 *
 * In fact, only few Windows symbols are used in this file. */
#if ! defined FALSE
typedef void * HWND;
typedef int BOOL;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef DWORD *LPDWORD;
typedef const char * LPCSTR;

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

#define __declspec(i)
#define WINAPI
#endif

#include "general.h"
#ifndef __CDP_H__
#include "cdp.h"
#endif

/* e-mail */
#define WBL_READRCPT 0x0001
#define WBL_DELAFTER 0x0002
#define WBL_PRILOW 0x0004
#define WBL_PRIHIGH 0x0008
#define WBL_SENSPERS 0x0010
#define WBL_SENSPRIV 0x0020
#define WBL_SENSCONF 0x0040
#define WBL_REMSENDNOW 0x0080
#define WBL_MSGINFILE 0x0100
#define WBL_VIEWED 0x0200
#define WBL_FAX 0x0400
#define WBL_REMOTE 0x0800
#define WBL_DELREQ 0x1000
#define WBMT_602 0x00000001
#define WBMT_MAPI 0x00000002
#define WBMT_SMTPOP3 0x00000004
#define WBMT_602REM 0x00010000
#define WBMT_602SP3 0x00020000
#define WBMT_DIALUP 0x00040000
#define ATML_ID 1
#define ATML_SUBJECT 2
#define ATML_SENDER 3
#define ATML_RECIPIENT 4
#define ATML_CREDATE 5
#define ATML_CRETIME 6
#define ATML_RCVDATE 7
#define ATML_RCVTIME 8
#define ATML_SIZE 9
#define ATML_FILECNT 10
#define ATML_FLAGS 11
#define ATML_STAT 12
#define ATML_MSGID 13
#define ATML_BODY 14
#define ATMF_ID 1
#define ATMF_NAME 2
#define ATMF_SIZE 3
#define ATMF_DATE 4
#define ATMF_TIME 5
#define MBL_BODY 1
#define MBL_FILES 2
#define MBS_NEW 0
#define MBS_OLD 1
#define MBS_DELETED 2
typedef BOOL (WINAPI enum_attr)( char * attrname, uns8 attrtype, uns8 attrmult, uns16 attrspecif );
typedef BOOL (WINAPI enum_attr_ex)( char * attrname, uns8 attrtype, uns8 attrmult, union t_specif attrspecif, void * user_data );

#ifdef __cplusplus
extern "C" {
#define DEFAULT(A) =A
#else
#define DEFAULT(A) /* */
#endif


#if WBVERS>=900

#ifdef __cplusplus
    extern "C" {
#endif
        /* priznaky pro pouziti ve funkci Import_appl_ex a v Import_appl_param */
#define IMPAPPL_NEW_INST 1
#define IMPAPPL_USE_ALT_NAME 2
#define IMPAPPL_REPLACE 4
#define IMPAPPL_NO_COMPILE 8
#define IMPAPPL_WITH_INFO 0x10
#define IMPEXP_DATA -1
#define IMPEXP_DATAPRIV -2
#define IMPEXP_ICON -3
#define IMPEXP_PROGMAX -100
#define IMPEXP_PROGSTEP -101
#define IMPEXP_PROGPOS -102
#define IMPEXP_FILENAME -103
#define IMPEXP_STATUS -104
#define IMPEXP_ERROR -105
#define IMPEXP_FINAL -106
        /* Oznaceni formatu dat */
#define IMPEXP_FORMAT_WINBASE 0
#define IMPEXP_FORMAT_TEXT_COLUMNS 1
#define IMPEXP_FORMAT_TEXT_CSV 2
#define IMPEXP_FORMAT_DBASE 3
#define IMPEXP_FORMAT_FOXPRO 4
#define IMPEXP_FORMAT_ODBC 5
#define IMPEXP_FORMAT_CURSOR 6
#if WBVERS>=950
#define IMPEXP_FORMAT_ODBCVIEW 7
#endif
#define IMPEXP_FORMAT_TABLE 10
#if WBVERS<=810
#define IMPEXP_FORMAT_TABLE_REIND 11
#endif
        typedef void (WINAPI IMPEXPAPPLCALLBACK)( int Categ, const void * Value, void * Param );
        typedef IMPEXPAPPLCALLBACK *LPIMPEXPAPPLCALLBACK;
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

#ifdef __cplusplus 
            // v C je treba inicializaci struktury provest rucne
            t_export_param(void)
            { cbSize=sizeof(t_export_param);
                hParent=0;  with_data=FALSE;  with_role_privils=TRUE;  with_usergrp=FALSE;
                exp_locked=FALSE;  back_end=FALSE;
                file_name=NULL;  date_limit=0;  long_names=TRUE;  report_progress=FALSE;
                callback=NULL;
                schema_name=NULL;
                overwrite=false;
            }
#endif
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

#ifdef __cplusplus
            // v C je treba inicializaci struktury provest rucne
            t_import_param(void)
            { memset(this, 0, sizeof(t_import_param));
                cbSize=sizeof(t_import_param);
            }
#endif
        };
        DllKernel BOOL WINAPI Move_data( cdp_t cdp, tobjnum move_descr_obj, const char * inpname, tobjnum inpobj, const char * outname, int inpformat, int outformat, int inpcode, int outcode, int silent );
#if WBVERS>=950
        DllKernel BOOL WINAPI cd_Data_transport( cdp_t cdp, tobjnum move_descr_obj, xcdp_t src_xcdp, xcdp_t trg_xcdp, const char * src_schema, const char * trg_schema, const char * src_name, const char * trg_name, int src_format, int trg_format, int src_code, int trg_code, unsigned flags );
#endif
        DllKernel BOOL WINAPI Import_appl_param( cdp_t cdp, const struct t_import_param * ip );
        DllKernel BOOL WINAPI Export_appl_param( cdp_t cdp, const struct t_export_param * ep );


#ifdef __cplusplus
    }
#endif

#endif

#ifdef NON_CD_FUNCTIONS       
    /* organizacni funkce */
    DllKernel void WINAPI start_package( void );
    DllKernel void WINAPI send_package( void );
    DllKernel void WINAPI concurrent( BOOL state );
    DllKernel BOOL WINAPI answered( void );
    DllKernel BOOL WINAPI waiting( sig32 timeout );
    /* chyby a varovani */
    DllKernel int WINAPI Sz_error( void );
    DllKernel int WINAPI Sz_warning( void );
    /* vyhledani objektu */
    DllKernel BOOL WINAPI Find2_object( const char * name, const uns8 * appl_id, tcateg category, tobjnum * position );
    DllKernel BOOL WINAPI Find_object( const char * name, tcateg category, tobjnum * position );
    DllKernel BOOL WINAPI Find_object_by_id( const WBUUID uuid, tcateg category, tobjnum * position );
    /* prace s kurzory */
    DllKernel BOOL WINAPI Open_cursor( tobjnum cursdef, tcursnum * curs );
    DllKernel BOOL WINAPI Open_cursor_direct( const char * query, tcursnum * curs );
    DllKernel BOOL WINAPI Close_cursor( tcursnum curs );
    DllKernel BOOL WINAPI Open_subcursor( tcursnum supercurs, const char * subcurdef, tcursnum * subcurs );
    DllKernel BOOL WINAPI Add_record( tcursnum curs, trecnum * recs, int numofrecs );
    DllKernel BOOL WINAPI Super_recnum( tcursnum subcursor, tcursnum supercursor, trecnum subrecnum, trecnum * superrecnum );
    DllKernel BOOL WINAPI Translate( tcursnum curs, trecnum crec, int tbord, trecnum * trec );
    /* prace se zaznamy */
    DllKernel trecnum WINAPI Append( tcurstab curs );
    DllKernel trecnum WINAPI Insert( tcurstab curs );
    DllKernel trecnum WINAPI Look_up( tcursnum curs, const char * attrname, void * res );
    DllKernel BOOL WINAPI Delete( tcurstab curs, trecnum position );
    DllKernel BOOL WINAPI Undelete( ttablenum table, trecnum position );
    DllKernel BOOL WINAPI Delete_all_records( tcurstab curs );
    DllKernel BOOL WINAPI Rec_cnt( tcurstab curs, trecnum * recnum );
    /* cteni a zapis dat */
    DllKernel BOOL WINAPI Read( tcurstab cursnum, trecnum position, tattrib attr, const modifrec * access, void * buffer );
    DllKernel BOOL WINAPI Write( tcurstab cursnum, trecnum position, tattrib attr, const modifrec * access, const void * buffer, uns32 datasize );
    DllKernel BOOL WINAPI Read_ind( tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, void * data );
    DllKernel BOOL WINAPI Write_ind( tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, void * data, uns32 datasize );
    DllKernel BOOL WINAPI Read_ind_cnt( tcurstab cursnum, trecnum position, tattrib attr, t_mult_size * count );
    DllKernel BOOL WINAPI Write_ind_cnt( tcurstab cursnum, trecnum position, tattrib attr, t_mult_size count );
    DllKernel BOOL WINAPI Read_var( tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size start, t_varcol_size size, void * buf, t_varcol_size * psize );
    DllKernel BOOL WINAPI Write_var( tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size start, t_varcol_size size, const void * buf );
    DllKernel BOOL WINAPI Read_len( tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size * size );
    DllKernel BOOL WINAPI Write_len( tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size size );
    DllKernel BOOL WINAPI Read_record( tcurstab cursnum, trecnum position, void * buf, uns32 datasize );
    DllKernel BOOL WINAPI Write_record( tcurstab cursnum, trecnum position, const void * buf, uns32 datasize );
    DllKernel BOOL WINAPI Write_record_ex( tcurstab cursnum, trecnum position, unsigned colcount, const struct t_column_val_descr * colvaldescr );
    DllKernel BOOL WINAPI Insert_record_ex( tcurstab curs, trecnum * position, unsigned colcount, const struct t_column_val_descr * colvaldescr );
    DllKernel BOOL WINAPI Append_record_ex( tcurstab curs, trecnum * position, unsigned colcount, const struct t_column_val_descr * colvaldescr );
    /* prava */
    DllKernel BOOL WINAPI GetSet_privils( tobjnum user_group_role, tcateg subject_categ, ttablenum table, trecnum recnum, t_oper operation, uns8 * privils );
    /* transakce */
    DllKernel BOOL WINAPI Start_transaction( void );
    DllKernel BOOL WINAPI Commit( void );
    DllKernel BOOL WINAPI Roll_back( void );
    DllKernel BOOL WINAPI Set_transaction_isolation_level( t_isolation level );
    /* zamky */
    DllKernel BOOL WINAPI Read_lock_record( tcurstab curs, trecnum position );
    DllKernel BOOL WINAPI Read_unlock_record( tcurstab curs, trecnum position );
    DllKernel BOOL WINAPI Write_lock_record( tcurstab curs, trecnum position );
    DllKernel BOOL WINAPI Write_unlock_record( tcurstab curs, trecnum position );
    DllKernel BOOL WINAPI Read_lock_table( tcurstab curs );
    DllKernel BOOL WINAPI Read_unlock_table( tcurstab curs );
    DllKernel BOOL WINAPI Write_lock_table( tcurstab curs );
    DllKernel BOOL WINAPI Write_unlock_table( tcurstab curs );
    /* prace s objekty */
    DllKernel BOOL WINAPI Insert_object( const char * name, tcateg category, tobjnum * objnum );
    DllKernel BOOL WINAPI Relist_objects( void );
    DllKernel BOOL WINAPI GetSet_group_role( tobjnum user_or_group, tobjnum group_or_role, tcateg subject2, t_oper operation, uns32 * relation );
    DllKernel BOOL WINAPI Create_user( const char * logname, const char * name1, const char * name2, const char * name3, const char * identif, const WBUUID homesrv, const char * password, tobjnum * objnum );
    DllKernel BOOL WINAPI Set_password( const char * username, const char * password );
    DllKernel BOOL WINAPI Create_link( const char * sourcename, const char * sourceappl, tcateg category, const char * linkname );
    DllKernel BOOL WINAPI Create2_link( const char * sourcename, const uns8 * sourceapplid, tcateg category, const char * linkname );
    DllKernel BOOL WINAPI Set_appl_starter( tcateg categ, const char * objname );
    DllKernel BOOL WINAPI Encrypt_object( tobjnum objnum, tcateg category );
    /* sledovani stavu */
#if WBVERS<=1000
    DllKernel sig32 WINAPI Available_memory( BOOL local );
#endif
    DllKernel sig32 WINAPI Used_memory( BOOL local );
    DllKernel int WINAPI Owned_cursors( void );
    DllKernel BOOL WINAPI Database_integrity( BOOL repair, uns32 * lost_blocks, uns32 * lost_dheap, uns32 * nonex_blocks, uns32 * cross_link, uns32 * damaged_tabdef );
#if WBVERS<=1000
    DllKernel BOOL WINAPI Get_info( kernel_info * kinf );
#endif
    DllKernel BOOL WINAPI GetSet_fil_size( t_oper operation, uns32 * size );
    DllKernel BOOL WINAPI GetSet_fil_blocks( t_oper operation, uns32 * size );
    DllKernel const char * WINAPI Who_am_I( void );
    DllKernel BOOL WINAPI Am_I_db_admin( void );
    DllKernel sig32 WINAPI WinBase602_version( void );
    DllKernel void WINAPI Enable_task_switch( BOOL enable );
    DllKernel BOOL WINAPI Compact_database( int margin );
    DllKernel BOOL WINAPI Log_write( const char * text );
    DllKernel BOOL WINAPI Connection_speed_test( int * requests, int * kbytes );
    DllKernel BOOL WINAPI Appl_inst_count( uns32 * count );
    DllKernel BOOL WINAPI Get_logged_user( int index, char * username, char * applname, int * state );
    DllKernel BOOL WINAPI Get_server_info( int info_type, void * buffer, unsigned buffer_size );
    DllKernel BOOL WINAPI Uninst_table( ttablenum table ); // POZOR: zastarala funkce
    /* obalka aplikace v externim jazyku */
    DllKernel int WINAPI link_kernel( const char * path, int display );
    DllKernel void WINAPI unlink_kernel( void );
    DllKernel int WINAPI interf_init( cdp_t cdp, int user );
    DllKernel void WINAPI interf_close( void );
    DllKernel void WINAPI core_release( void );
    DllKernel BOOL WINAPI Login( const char * username, const char * password );
    DllKernel BOOL WINAPI Logout( void );
    DllKernel BOOL WINAPI Set_application( const char * applname );
    /* jine funkce serveru */
    DllKernel BOOL WINAPI SQL_execute( const char * statement, uns32 * results );
    DllKernel BOOL WINAPI SQL_prepare( const char * statement, uns32 * handle );
    DllKernel BOOL WINAPI SQL_exec_prepared( uns32 handle, uns32 * results, int * count );
    DllKernel BOOL WINAPI SQL_drop( uns32 handle );
    DllKernel BOOL WINAPI Set_sql_option( uns32 optmask, uns32 optval );
    DllKernel BOOL WINAPI Get_sql_option( uns32 * optval );
    DllKernel BOOL WINAPI Message_to_clients( const char * msg );
    DllKernel BOOL WINAPI Set_progress_report_modulus( unsigned modulus );
    DllKernel BOOL WINAPI Get_property_value( const char * owner, const char * name, int num, char * buffer, unsigned buffer_size, int * valsize DEFAULT(NULL) );
    DllKernel BOOL WINAPI Set_property_value( const char * owner, const char * name, int num, const char * value, sig32 valsize DEFAULT(-1) );
    DllKernel void WINAPI Invalidate_cached_table_info( void );


    DllKernel BOOL WINAPI Break( void );


    /* manipulace s tabulkou */
    DllKernel BOOL WINAPI Enable_index( ttablenum table, int which, BOOL enable );
    DllKernel BOOL WINAPI Free_deleted( ttablenum table );
    DllKernel BOOL WINAPI Compact_table( ttablenum table );
    /* agregacni funkce */
    DllKernel BOOL WINAPI C_sum( tcursnum curs, const char * attrname, const char * condition, void * result );
    DllKernel BOOL WINAPI C_max( tcursnum curs, const char * attrname, const char * condition, void * result );
    DllKernel BOOL WINAPI C_min( tcursnum curs, const char * attrname, const char * condition, void * result );
    DllKernel BOOL WINAPI C_count( tcursnum curs, const char * attrname, const char * condition, trecnum * result );
    DllKernel BOOL WINAPI C_avg( tcursnum curs, const char * attrname, const char * condition, void * result );
    /* ODBC */
#if WBVERS<=810
    DllKernel BOOL WINAPI ODBC_open_cursor( uns32 connection, tcursnum * curs, const char * query );
#endif
#if WBVERS<=810
    DllKernel uns32 WINAPI ODBC_find_connection( const char * dsn_name );
#endif
#if WBVERS<=810
    DllKernel uns32 WINAPI ODBC_create_connection( const char * dsn_name );
#endif
#if WBVERS<=810
    DllKernel uns32 WINAPI ODBC_direct_connection( const char * conn_string );
#endif
    /* informace o sloupcich */
    DllKernel BOOL WINAPI Enum_attributes( ttablenum table, enum_attr * callback );
    DllKernel BOOL WINAPI Enum_attributes_ex( ttablenum table, enum_attr_ex * callback, void * user_data );
    DllKernel BOOL WINAPI Attribute_info( ttablenum table, const char * attrname, tattrib * attrnum, uns8 * attrtype, uns8 * attrmult, uns16 * attrspecif );
    DllKernel BOOL WINAPI Attribute_info_ex( ttablenum table, const char * attrname, tattrib * attrnum, uns8 * attrtype, uns8 * attrmult, union t_specif * attrspecif );
    /* podpisy a replikace */
#if WBVERS<=810
    DllKernel BOOL WINAPI Signature( HWND hParent, tcursnum cursor, trecnum recnum, tattrib attr, int create, void * param );
#endif
    DllKernel BOOL WINAPI Repl_control( int optype, int opparsize, void * opparam );
    DllKernel BOOL WINAPI Replicate( const char * ServerName, const char * ApplName, BOOL pull );
    DllKernel BOOL WINAPI Skip_repl( const char * ServerName, const char * ApplName );
    DllKernel BOOL WINAPI Reset_replication( void );
    DllKernel BOOL WINAPI GetSet_next_user( tcurstab curs, trecnum position, tattrib attr, t_oper operation, t_valtype valtype, void * value );
    /* prace s e-maily a e-mailovou schrankou */
    DllKernel DWORD WINAPI MailOpenInBox( HANDLE * lpMailBox );
    DllKernel DWORD WINAPI MailBoxLoad( HANDLE MailBox, UINT Flags );
    DllKernel DWORD WINAPI MailBoxGetMsg( HANDLE MailBox, DWORD MsgID );
    DllKernel DWORD WINAPI MailBoxGetMsgEx( HANDLE MailBox, DWORD MsgID, DWORD Flags );
    DllKernel DWORD WINAPI MailBoxGetFilInfo( HANDLE MailBox, DWORD MsgID );
    DllKernel DWORD WINAPI MailBoxSaveFileAs( HANDLE MailBox, DWORD MsgID, DWORD FilIdx, char * FilName, char * DstPath );
    DllKernel DWORD WINAPI MailBoxSaveFileDBs( HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, LPCSTR Table, LPCSTR Attr, LPCSTR Cond );
    DllKernel DWORD WINAPI MailBoxSaveFileDBr( HANDLE MailBox, DWORD MsgID, DWORD FilIdx, LPCSTR FilName, tcurstab Table, trecnum Pos, tattrib Attr, t_mult_size Index );
    DllKernel DWORD WINAPI MailBoxDeleteMsg( HANDLE MailBox, DWORD MsgID, BOOL RecToo );
    DllKernel DWORD WINAPI MailGetInBoxInfo( HANDLE MailBox, char * mTblName, ttablenum * mTblNum, char * fTblName, ttablenum * fTblNum );
    DllKernel void WINAPI MailCloseInBox( HANDLE MailBox );
    DllKernel DWORD WINAPI MailGetType( void );
    DllKernel DWORD WINAPI MailDial( char * PassWord );
    DllKernel DWORD WINAPI MailHangUp( void );
    /* prace s e-mailovym profilem */
    DllKernel DWORD WINAPI MailCreateProfile( const char * ProfileName, BOOL Temp );
    DllKernel DWORD WINAPI MailDeleteProfile( const char * ProfileName );
    DllKernel DWORD WINAPI MailSetProfileProp( const char * ProfileName, const char * PropName, const char * PropValue );
    DllKernel DWORD WINAPI MailGetProfileProp( const char * ProfileName, const char * PropName, char * PropValue, int ValSize );


#endif // NON_CD_FUNCTIONS      

    /* varianta cd_ */
    typedef uns32 t_docid32;
    typedef uns64 t_docid64;
    /* organizacni funkce */
    DllKernel void WINAPI cd_start_package( cdp_t cdp );
    DllKernel void WINAPI cd_send_package( cdp_t cdp );
    DllKernel BOOL WINAPI cd_concurrent( cdp_t cdp, BOOL state );
    DllKernel BOOL WINAPI cd_answered( cdp_t cdp );
    DllKernel void WINAPI cd_wait_for_answer( cdp_t cdp );
    DllKernel BOOL WINAPI cd_waiting( cdp_t cdp, sig32 timeout );
    /* chyby a varovani */
    DllKernel int WINAPI cd_Sz_error( cdp_t cdp );
    DllKernel int WINAPI cd_Sz_warning( cdp_t cdp );
    DllKernel BOOL WINAPI Get_error_num_text( xcdp_t xcdp, int err, char * buf, unsigned buflen );
    DllKernel BOOL WINAPI Get_server_error_suppl( cdp_t cdp, char * buf );
    DllKernel BOOL WINAPI Get_server_error_context( cdp_t cdp, int level, int * itype, int * par1, int * par2, int * par3, int * par4 );
    DllKernel BOOL WINAPI Get_server_error_context_text( cdp_t cdp, int level, char * buffer, int buflen );
    DllKernel BOOL WINAPI cd_Rolled_back( cdp_t cdp, BOOL * rolled_back );
    DllKernel BOOL WINAPI cd_Transaction_open( cdp_t cdp, BOOL * transaction_open );
    /* vyhledani objektu */
    DllKernel BOOL WINAPI cd_Find2_object( cdp_t cdp, const char * name, const uns8 * appl_id, tcateg category, tobjnum * position );
    DllKernel BOOL WINAPI cd_Find_object( cdp_t cdp, const char * name, tcateg category, tobjnum * position );
    DllKernel BOOL WINAPI cd_Find_object_by_id( cdp_t cdp, const WBUUID uuid, tcateg category, tobjnum * position );
    /* prace s kurzory */
    DllKernel BOOL WINAPI cd_Open_cursor( cdp_t cdp, tobjnum cursdef, tcursnum * curs );
    DllKernel BOOL WINAPI cd_Open_cursor_direct( cdp_t cdp, const char * query, tcursnum * curs );
    DllKernel BOOL WINAPI cd_Close_cursor( cdp_t cdp, tcursnum curs );
    DllKernel BOOL WINAPI cd_Open_subcursor( cdp_t cdp, tcursnum supercurs, const char * subcurdef, tcursnum * subcurs );
    DllKernel BOOL WINAPI cd_Add_record( cdp_t cdp, tcursnum curs, trecnum * recs, int numofrecs );
    DllKernel BOOL WINAPI cd_Super_recnum( cdp_t cdp, tcursnum subcursor, tcursnum supercursor, trecnum subrecnum, trecnum * superrecnum );
    DllKernel BOOL WINAPI cd_Translate( cdp_t cdp, tcursnum curs, trecnum crec, int tbord, trecnum * trec );
    /* prace se zaznamy */
    DllKernel trecnum WINAPI cd_Append( cdp_t cdp, tcurstab curs );
    DllKernel trecnum WINAPI cd_Insert( cdp_t cdp, tcurstab curs );
    DllKernel trecnum WINAPI cd_Look_up( cdp_t cdp, tcursnum curs, const char * attrname, void * res );
    DllKernel BOOL WINAPI cd_Delete( cdp_t cdp, tcurstab curs, trecnum position );
    DllKernel BOOL WINAPI cd_Undelete( cdp_t cdp, ttablenum table, trecnum position );
    DllKernel BOOL WINAPI cd_Delete_all_records( cdp_t cdp, tcurstab curs );
    DllKernel BOOL WINAPI cd_Rec_cnt( cdp_t cdp, tcurstab curs, trecnum * recnum );
    /* cteni a zapis dat */
    DllKernel BOOL WINAPI cd_Read( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, const modifrec * access, void * buffer );
    DllKernel BOOL WINAPI cd_Write( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, const modifrec * access, const void * buffer, uns32 datasize );
    DllKernel BOOL WINAPI cd_Read_ind( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, void * data );
    DllKernel BOOL WINAPI cd_Write_ind( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, void * data, uns32 datasize );
    DllKernel BOOL WINAPI cd_Read_ind_cnt( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, t_mult_size * count );
    DllKernel BOOL WINAPI cd_Write_ind_cnt( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, t_mult_size count );
    DllKernel BOOL WINAPI cd_Read_var( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size start, t_varcol_size size, void * buf, t_varcol_size * psize );
    DllKernel BOOL WINAPI cd_Write_var( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size start, t_varcol_size size, const void * buf );
    DllKernel BOOL WINAPI cd_Write_lob( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, t_varcol_size size, const void * buf );
    DllKernel BOOL WINAPI cd_Read_len( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size * size );
    DllKernel BOOL WINAPI cd_Write_len( cdp_t cdp, tcurstab cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size size );
    DllKernel BOOL WINAPI cd_Read_record( cdp_t cdp, tcurstab cursnum, trecnum position, void * buf, uns32 datasize );
    DllKernel BOOL WINAPI cd_Write_record( cdp_t cdp, tcurstab cursnum, trecnum position, const void * buf, uns32 datasize );
    DllKernel BOOL WINAPI cd_Write_record_ex( cdp_t cdp, tcurstab cursnum, trecnum position, unsigned colcount, const struct t_column_val_descr * colvaldescr );
    DllKernel BOOL WINAPI cd_Insert_record_ex( cdp_t cdp, tcurstab curs, trecnum * position, unsigned colcount, const struct t_column_val_descr * colvaldescr );
    DllKernel BOOL WINAPI cd_Append_record_ex( cdp_t cdp, tcurstab curs, trecnum * position, unsigned colcount, const struct t_column_val_descr * colvaldescr );
    /* prava */
    DllKernel BOOL WINAPI cd_GetSet_privils( cdp_t cdp, tobjnum user_group_role, tcateg subject_categ, ttablenum table, trecnum recnum, t_oper operation, uns8 * privils );
    /* transakce */
    DllKernel BOOL WINAPI cd_Start_transaction( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Commit( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Roll_back( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Set_transaction_isolation_level( cdp_t cdp, t_isolation level );
    /* zamky */
    DllKernel BOOL WINAPI cd_Read_lock_record( cdp_t cdp, tcurstab curs, trecnum position );
    DllKernel BOOL WINAPI cd_Read_unlock_record( cdp_t cdp, tcurstab curs, trecnum position );
    DllKernel BOOL WINAPI cd_Write_lock_record( cdp_t cdp, tcurstab curs, trecnum position );
    DllKernel BOOL WINAPI cd_Write_unlock_record( cdp_t cdp, tcurstab curs, trecnum position );
    DllKernel BOOL WINAPI cd_Read_lock_table( cdp_t cdp, tcurstab curs );
    DllKernel BOOL WINAPI cd_Read_unlock_table( cdp_t cdp, tcurstab curs );
    DllKernel BOOL WINAPI cd_Write_lock_table( cdp_t cdp, tcurstab curs );
    DllKernel BOOL WINAPI cd_Write_unlock_table( cdp_t cdp, tcurstab curs );
    DllKernel BOOL WINAPI cd_Who_prevents_locking( cdp_t cdp, ttablenum tabnum, trecnum position, BOOL write_lock, uns32 * client_number, char * user_name );
    /* prace s objekty */
    DllKernel BOOL WINAPI cd_Insert_object( cdp_t cdp, const char * name, tcateg category, tobjnum * objnum );
    DllKernel BOOL WINAPI cd_Insert_object_limited( cdp_t cdp, const char * name, tcateg category, tobjnum * objnum );
    DllKernel BOOL WINAPI cd_Relist_objects( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Relist_objects_ex( cdp_t cdp, BOOL extended );
    DllKernel BOOL WINAPI cd_GetSet_group_role( cdp_t cdp, tobjnum user_or_group, tobjnum group_or_role, tcateg subject2, t_oper operation, uns32 * relation );
    DllKernel BOOL WINAPI cd_Create_user( cdp_t cdp, const char * logname, const char * name1, const char * name2, const char * name3, const char * identif, const WBUUID homesrv, const char * password, tobjnum * objnum );
    DllKernel BOOL WINAPI cd_Set_password( cdp_t cdp, const char * username, const char * password );
#if WBVERS<=810
    DllKernel BOOL WINAPI cd_Create_link( cdp_t cdp, const char * sourcename, const char * sourceappl, tcateg category, const char * linkname );
#endif
#if WBVERS<=810
    DllKernel BOOL WINAPI cd_Create2_link( cdp_t cdp, const char * sourcename, const uns8 * sourceapplid, tcateg category, const char * linkname );
#endif
    DllKernel BOOL WINAPI cd_Set_appl_starter( cdp_t cdp, tcateg categ, const char * objname );
    DllKernel BOOL WINAPI cd_Encrypt_object( cdp_t cdp, tobjnum objnum, tcateg category );
    /* sledovani stavu */
#if WBVERS<=1000
    DllKernel sig32 WINAPI cd_Available_memory( cdp_t cdp, BOOL local );
#endif
    DllKernel sig32 WINAPI cd_Used_memory( cdp_t cdp, BOOL local );
    DllKernel int WINAPI cd_Owned_cursors( cdp_t cdp );
    DllKernel sig32 WINAPI cd_Client_number( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Database_integrity( cdp_t cdp, BOOL repair, uns32 * lost_blocks, uns32 * lost_dheap, uns32 * nonex_blocks, uns32 * cross_link, uns32 * damaged_tabdef );
#if WBVERS<=1000
    DllKernel BOOL WINAPI cd_Get_info( cdp_t cdp, kernel_info * kinf );
#endif
    DllKernel BOOL WINAPI cd_GetSet_fil_size( cdp_t cdp, t_oper operation, uns32 * size );
    DllKernel BOOL WINAPI cd_GetSet_fil_blocks( cdp_t cdp, t_oper operation, uns32 * size );
    DllKernel const char * WINAPI cd_Who_am_I( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Am_I_db_admin( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Am_I_appl_admin( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Am_I_appl_author( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Am_I_config_admin( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Am_I_security_admin( cdp_t cdp );
    DllKernel sig32 WINAPI cd_WinBase602_version( cdp_t cdp );
    DllKernel void WINAPI cd_Enable_task_switch( cdp_t cdp, BOOL enable );
    DllKernel BOOL WINAPI cd_Compact_database( cdp_t cdp, int margin );
    DllKernel BOOL WINAPI cd_Log_write( cdp_t cdp, const char * text );
    DllKernel BOOL WINAPI cd_Connection_speed_test( cdp_t cdp, int * requests, int * kbytes );
    DllKernel BOOL WINAPI cd_Appl_inst_count( cdp_t cdp, uns32 * count );
    DllKernel BOOL WINAPI cd_Get_logged_user( cdp_t cdp, int index, char * username, char * applname, int * state );
    DllKernel BOOL WINAPI cd_Get_server_info( cdp_t cdp, int info_type, void * buffer, unsigned buffer_size );
    DllKernel void WINAPI cd_Get_appl_info( cdp_t cdp, WBUUID appl_uuid, char * appl_name, char * server_name );
    DllKernel BOOL WINAPI cd_Check_indices( cdp_t cdp, ttablenum tbnum, sig32 * result, sig32 * index_number );
    DllKernel uns32 WINAPI Get_client_version( void );
    DllKernel uns32 WINAPI Get_client_version1( void );
    DllKernel uns32 WINAPI Get_client_version2( void );
    DllKernel uns32 WINAPI Get_client_version3( void );
    DllKernel uns32 WINAPI Get_client_version4( void );
    /* SQL server management */
    DllKernel int WINAPI srv_Get_state_local( const char * server_name );
    DllKernel int WINAPI srv_Start_server_local( const char * server_name, int mode, const char * password );
    DllKernel int WINAPI srv_Stop_server_local( const char * server_name, int wait_seconds, cdp_t cdp );
    DllKernel BOOL WINAPI srv_Get_service_name( const char * server_name, char * service_name );
    DllKernel BOOL WINAPI srv_Register_server_local( const char * server_name, const char * path, BOOL private_server );
    DllKernel BOOL WINAPI srv_Unregister_server( const char * server_name, BOOL private_server );
    DllKernel BOOL WINAPI srv_Register_service( const char * server_name, const char * service_name );
    DllKernel BOOL WINAPI srv_Unregister_service( const char * service_name );
    DllKernel int WINAPI srv_Restore( const char * server_name, const char * pathname_pattern, BOOL save_original_database );
    /* obalka aplikace v externim jazyku */
    DllKernel void WINAPI cdp_init( cdp_t cdp );
    DllKernel sig32 WINAPI cdp_size( void );
    DllKernel cdp_t WINAPI cdp_alloc( void );
    DllKernel void WINAPI cdp_free( cdp_t cdp );
    DllKernel void WINAPI cd_interf_close( cdp_t cdp );
    DllKernel int WINAPI cd_connect( cdp_t cdp, const char * server_name, int show_type );
    DllKernel void WINAPI cd_disconnect( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Login( cdp_t cdp, const char * username, const char * password );
    DllKernel BOOL WINAPI cd_Login_par( cdp_t cdp, const char * username, const char * password, unsigned flags );
    DllKernel BOOL WINAPI cd_Logout( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Set_application( cdp_t cdp, const char * applname );
    DllKernel BOOL WINAPI cd_Set_application_ex( cdp_t cdp, const char * applname, BOOL extended );
    DllKernel void WINAPI cd_assign_to_thread( cdp_t cdp );
    DllKernel void WINAPI cd_unassign( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Weak_link( cdp_t cdp );
    DllKernel void WINAPI Set_translation_callback( cdp_t cdp, t_translation_callback translation_callback );
    /* jine funkce serveru */
    DllKernel BOOL WINAPI cd_SQL_execute( cdp_t cdp, const char * statement, uns32 * results );
    DllKernel BOOL WINAPI cd_SQL_prepare( cdp_t cdp, const char * statement, uns32 * handle );
    DllKernel BOOL WINAPI cd_SQL_exec_prepared( cdp_t cdp, uns32 handle, uns32 * results, int * count );
    DllKernel BOOL WINAPI cd_SQL_drop( cdp_t cdp, uns32 handle );
    DllKernel BOOL WINAPI cd_Set_sql_option( cdp_t cdp, uns32 optmask, uns32 optval );
    DllKernel BOOL WINAPI cd_Get_sql_option( cdp_t cdp, uns32 * optval );
    DllKernel BOOL WINAPI cd_Break( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Break_user( cdp_t cdp, int client_number );
    DllKernel BOOL WINAPI cd_Kill_user( cdp_t cdp, int client_number );
    DllKernel BOOL WINAPI cd_Message_to_clients( cdp_t cdp, const char * msg );
    DllKernel BOOL WINAPI cd_Set_progress_report_modulus( cdp_t cdp, unsigned modulus );
    DllKernel BOOL WINAPI cd_SQL_host_execute( cdp_t cdp, const char * statement, uns32 * results, struct t_clivar * hostvars, unsigned hostvars_count );
    DllKernel BOOL WINAPI cd_SQL_host_prepare( cdp_t cdp, const char * statement, uns32 * handle, struct t_clivar * hostvars, unsigned hostvars_count );
    DllKernel BOOL WINAPI cd_SQL_value( cdp_t cdp, const char * sql_expr, char * outbuf, int out_bufsize );
    DllKernel BOOL WINAPI cd_Backup_database_file( cdp_t cdp, const char * file_name );
    DllKernel BOOL WINAPI cd_Enable_integrity( cdp_t cdp, BOOL enable );
    DllKernel BOOL WINAPI cd_Fulltext_index_doc( cdp_t cdp, const char * ft_label, t_docid32 docid, const char * filename, const char * format );
    DllKernel BOOL WINAPI cd_Fulltext_index_doc64( cdp_t cdp, const char * ft_label, t_docid64 docid, const char * filename, const char * format );
    DllKernel BOOL WINAPI cd_Get_property_value( cdp_t cdp, const char * owner, const char * name, int num, char * buffer, unsigned buffer_size, int * valsize DEFAULT(NULL) );
    DllKernel BOOL WINAPI cd_Set_property_value( cdp_t cdp, const char * owner, const char * name, int num, const char * value, sig32 valsize DEFAULT(-1) );
    DllKernel void WINAPI cd_Invalidate_cached_table_info( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Set_temporary_authoring_mode( cdp_t cdp, BOOL on_off );
    DllKernel BOOL WINAPI cd_Send_client_time_zone( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Lock_server( cdp_t cdp );
    DllKernel void WINAPI cd_Unlock_server( cdp_t cdp );
    DllKernel BOOL WINAPI cd_Park_server_settings( cdp_t cdp );

    DllKernel BOOL WINAPI cd_Operation_limits( cdp_t cdp, t_oper_limits data );
    /* manipulace s tabulkou */
    DllKernel BOOL WINAPI cd_Enable_index( cdp_t cdp, ttablenum table, int which, BOOL enable );
    DllKernel BOOL WINAPI cd_Free_deleted( cdp_t cdp, ttablenum table );
    DllKernel BOOL WINAPI cd_Compact_table( cdp_t cdp, ttablenum table );
    DllKernel BOOL WINAPI cd_Truncate_table( cdp_t cdp, ttablenum table );
    /* agregacni funkce */
    DllKernel BOOL WINAPI cd_C_sum( cdp_t cdp, tcursnum curs, const char * attrname, const char * condition, void * result );
    DllKernel BOOL WINAPI cd_C_max( cdp_t cdp, tcursnum curs, const char * attrname, const char * condition, void * result );
    DllKernel BOOL WINAPI cd_C_min( cdp_t cdp, tcursnum curs, const char * attrname, const char * condition, void * result );
    DllKernel BOOL WINAPI cd_C_count( cdp_t cdp, tcursnum curs, const char * attrname, const char * condition, trecnum * result );
    DllKernel BOOL WINAPI cd_C_avg( cdp_t cdp, tcursnum curs, const char * attrname, const char * condition, void * result );
    /* ODBC */
#if WBVERS<=810
    DllKernel BOOL WINAPI cd_ODBC_open_cursor( cdp_t cdp, uns32 connection, tcursnum * curs, const char * query );
#endif
#if WBVERS<=810
    DllKernel uns32 WINAPI cd_ODBC_find_connection( cdp_t cdp, const char * dsn_name );
#endif
#if WBVERS<=810
    DllKernel uns32 WINAPI cd_ODBC_create_connection( cdp_t cdp, const char * dsn_name );
#endif
#if WBVERS<=810
    DllKernel uns32 WINAPI cd_ODBC_direct_connection( cdp_t cdp, const char * conn_string );
#endif
#if WBVERS<=810
    DllKernel BOOL WINAPI cd_ODBC_exec_direct( cdp_t cdp, uns32 connection, const char * query );
#endif
#if WBVERS>=950
    DllKernel void WINAPI ODBC_release_env( void );
#endif
#if WBVERS>=950
    DllKernel t_pconnection WINAPI ODBC_connect( const char * dsn, const char * uid, const char * pwd, void * window_handle );
#endif
#if WBVERS>=950
    DllKernel void WINAPI ODBC_disconnect( t_pconnection conn );
#endif
    /* informace o sloupcich */
    DllKernel BOOL WINAPI cd_Enum_attributes( cdp_t cdp, ttablenum table, enum_attr * callback );
    DllKernel BOOL WINAPI cd_Enum_attributes_ex( cdp_t cdp, ttablenum table, enum_attr_ex * callback, void * user_data );
    DllKernel BOOL WINAPI cd_Attribute_info( cdp_t cdp, ttablenum table, const char * attrname, tattrib * attrnum, uns8 * attrtype, uns8 * attrmult, uns16 * attrspecif );
    DllKernel BOOL WINAPI cd_Attribute_info_ex( cdp_t cdp, ttablenum table, const char * attrname, tattrib * attrnum, uns8 * attrtype, uns8 * attrmult, union t_specif * attrspecif );
    /* podpisy a replikace */
#if WBVERS<=810
    DllKernel BOOL WINAPI cd_Signature( cdp_t cdp, HWND hParent, tcursnum cursor, trecnum recnum, tattrib attr, int create, void * param );
#endif
    DllKernel BOOL WINAPI cd_Repl_control( cdp_t cdp, int optype, int opparsize, void * opparam );
    DllKernel BOOL WINAPI cd_Replicate( cdp_t cdp, const char * ServerName, const char * ApplName, BOOL pull );
    DllKernel BOOL WINAPI cd_Skip_repl( cdp_t cdp, const char * ServerName, const char * ApplName );
    DllKernel BOOL WINAPI cd_Reset_replication( cdp_t cdp );
    DllKernel BOOL WINAPI cd_GetSet_next_user( cdp_t cdp, tcurstab curs, trecnum position, tattrib attr, t_oper operation, t_valtype valtype, void * value );

    enum t_rpap_oper { RPAP_GET_TOKENSRVR=1, RPAP_SET_TOKENSRVR, RPAP_GET_CONFLSRVR, RPAP_SET_CONFLSRVR } ;
    enum t_rpls_oper { RPLS_GET_ENABLED=1, RPLS_SET_ENABLED, RPLS_GET_ADDRESS1, RPLS_SET_ADDRESS1, RPLS_GET_ADDRTYPE1, RPLS_SET_ADDRTYPE1, RPLS_GET_ADDRESS2, RPLS_SET_ADDRESS2, RPLS_GET_ADDRTYPE2, RPLS_SET_ADDRTYPE2, RPLS_GET_MAILPROF, RPLS_SET_MAILPROF, RPLS_SET_MAILPWORD, RPLS_SET_MAILDPWORD, RPLS_GET_DIPDCONN, RPLS_SET_DIPDCONN, RPLS_GET_DIPDUSER, RPLS_SET_DIPDUSER, RPLS_SET_DIPDPWORD, RPLS_GET_INQUEUE, RPLS_SET_INQUEUE, RPLS_GET_OUTQUEUE, RPLS_SET_OUTQUEUE, RPLS_GET_THREADCNT, RPLS_SET_THREADCNT, RPLS_GET_REPLPERIOD, RPLS_SET_REPLPERIOD, RPLS_GET_LOGTABLES, RPLS_SET_LOGTABLES, RPLS_GET_REPLTRIGGERS, RPLS_SET_REPLTRIGGERS, RPLS_GET_ERRCAUSESNACK, RPLS_SET_ERRCAUSESNACK } ;
    enum t_rpsp_oper { RPSP_GET_ADDRESS1=1, RPSP_GET_ADDRTYPE1, RPSP_GET_ADDRESS2, RPSP_GET_ADDRTYPE2, RPSP_GET_USEALT, RPSP_SET_USEALT, RPSP_GET_LASTRCVD, RPSP_GET_LASTSNDD, RPSP_GET_LASTACKD } ;
    enum t_rpst_oper { RPST_STATE=1, RPST_RELNAME } ;
    DllKernel BOOL WINAPI Repl_GetSet_local_server_props( cdp_t cdp, enum t_rpls_oper Oper, void * Prop );
    DllKernel BOOL WINAPI Repl_GetSet_server_props( cdp_t cdp, const char * Srvr, enum t_rpsp_oper Oper, void * Prop );
    DllKernel BOOL WINAPI Repl_direct_register_server( cdp_t cdp, const char * Addr, const char * AType );
    DllKernel BOOL WINAPI Repl_refresh_server_info( cdp_t cdp, const char * Srvr );
    DllKernel BOOL WINAPI Repl_GetSet_appl_props( cdp_t cdp, const char * Appl, enum t_rpap_oper Oper, char * Prop );
    DllKernel BOOL WINAPI Repl_send_share_req( cdp_t cdp, const char * Srvr, const char * Appl, const char * RelName, BOOL Synch );
    DllKernel BOOL WINAPI Repl_accept_share_req( cdp_t cdp, const char * Srvr, const char * Appl, const char * RelName, BOOL Accept );
    DllKernel BOOL WINAPI Repl_abandon_sharing( cdp_t cdp, const char * Srvr, const char * Appl );
    DllKernel BOOL WINAPI Repl_Get_state( cdp_t cdp, const char * Srvr, const char * Appl, enum t_rpst_oper Oper, void * State );
    DllKernel BOOL WINAPI Repl_synchronize( cdp_t cdp, const char * Srvr );
    DllKernel BOOL WINAPI Repl_resend_packet( cdp_t cdp, const char * Srvr );
    DllKernel BOOL WINAPI Repl_cancel_packet( cdp_t cdp, const char * Srvr );
    DllKernel BOOL WINAPI Repl_appl_shared( cdp_t cdp, const char * Appl );

#if WBVERS<=810
    /* replication GUI */
    DllViewed BOOL WINAPI Repl_edit_local_server_options( cdp_t cdp, HWND hParent );
    DllViewed BOOL WINAPI Repl_register_new_remote_server( cdp_t cdp, HWND hParent );
    DllViewed BOOL WINAPI Repl_unregister_remote_server( cdp_t cdp, HWND hParent, const char * Srvr, BOOL confirm );
    DllViewed BOOL WINAPI Repl_edit_sharing_parameters( cdp_t cdp, HWND hParent, const char * Appl, BOOL complete );
    DllViewed BOOL WINAPI Repl_edit_application_properties( cdp_t cdp, HWND hParent, int pages );
    DllViewed void WINAPI Repl_edit_relation( cdp_t cdp, HWND hOwner, const char * RelName );

#endif
    /* translation */
    DllKernel BOOL WINAPI Select_language( cdp_t cdp, int language );
    DllKernel BOOL WINAPI Translate_to_language( cdp_t cdp, char * text, int space );
    DllKernel BOOL WINAPI Translate_and_alloc( cdp_t cdp, const char * input, char * * poutput );
    /* typove konverze */
    DllKernel uns32 WINAPI Make_date( int day, int month, int year );
    DllKernel int WINAPI Day( uns32 dt );
    DllKernel int WINAPI Month( uns32 dt );
    DllKernel int WINAPI Year( uns32 dt );
    DllKernel int WINAPI Quarter( uns32 dt );
    DllKernel uns32 WINAPI Today( void );
    DllKernel uns32 WINAPI Make_time( int hour, int minute, int second, int sec1000 );
    DllKernel int WINAPI Day_of_week( uns32 dt );
    DllKernel int WINAPI Hours( uns32 tm );
    DllKernel int WINAPI Minutes( uns32 tm );
    DllKernel int WINAPI Seconds( uns32 tm );
    DllKernel int WINAPI Sec1000( uns32 tm );
    DllKernel uns32 WINAPI Now( void );
    DllKernel uns32 WINAPI Current_timestamp( void );
    DllKernel BOOL WINAPI Like( const char * s1, const char * s2 );
    DllKernel BOOL WINAPI Pref( const char * s1, const char * s2 );
    DllKernel BOOL WINAPI Substr( const char * s1, const char * s2 );
    DllKernel void WINAPI Upcase( char * str );
    DllKernel double WINAPI money2real( monstr * m );
    DllKernel BOOL WINAPI real2money( double d, monstr * m );
    DllKernel uns32 WINAPI timestamp2date( uns32 dtm );
    DllKernel uns32 WINAPI timestamp2time( uns32 dtm );
    DllKernel uns32 WINAPI datetime2timestamp( uns32 dt, uns32 tm );
    DllKernel void WINAPI time2str( uns32 tm, char * txt, int param );
    DllKernel void WINAPI date2str( uns32 dt, char * txt, int param );
    DllKernel void WINAPI timestamp2str( uns32 dtm, char * txt, int param );
    DllKernel BOOL WINAPI str2time( const char * txt, uns32 * tm );
    DllKernel BOOL WINAPI str2date( const char * txt, uns32 * dt );
    DllKernel BOOL WINAPI str2timestamp( const char * txt, uns32 * dtm );
    DllKernel double WINAPI timestamp2TDateTime( uns32 dtm );
    DllKernel uns32 WINAPI TDateTime2timestamp( double tdt );
    DllKernel double WINAPI date2TDateTime( uns32 date );
    DllKernel double WINAPI time2TDateTime( uns32 time );
    DllKernel void WINAPI TDateTime2datetime( double tdt, uns32 * date, uns32 * time );
    /* mail */
    DllKernel DWORD WINAPI InitWBMail( char * Profile, char * PassWord );
    DllKernel DWORD WINAPI InitWBMailEx( char * Profile, char * RecvPassWord, char * SendPassWord );
    DllKernel DWORD WINAPI InitWBMail602( char * EmiPath, char * UserID, char * PassWord );
    DllKernel DWORD WINAPI InitWBMail602x( char * Profile );
    DllKernel void WINAPI CloseWBMail( void );
    DllKernel DWORD WINAPI LetterCreate( char * Subj, char * Msg, UINT Flags, HANDLE * lpLetter );
    DllKernel DWORD WINAPI LetterAddAddr( HANDLE Letter, char * Addr, char * AddrType, BOOL CC );
    DllKernel DWORD WINAPI LetterAddFile( HANDLE Letter, char * fName );
    DllKernel DWORD WINAPI LetterAddBLOBs( HANDLE Letter, char * fName, const char * Table, const char * Attr, const char * Cond );
    DllKernel DWORD WINAPI LetterAddBLOBr( HANDLE Letter, char * fName, tcurstab Table, trecnum Pos, tattrib Attr, t_mult_size Index );
    DllKernel DWORD WINAPI LetterSend( HANDLE Letter );
    DllKernel DWORD WINAPI TakeMailToRemOffice( void );
    DllKernel void WINAPI LetterCancel( HANDLE Letter );
    /* synchronizace pomoci udalosti */
    DllKernel BOOL WINAPI cd_Register_event( cdp_t cdp, const char * event_name, const char * param_str, BOOL param_exact, uns32 * event_handle );
    DllKernel BOOL WINAPI cd_Unregister_event( cdp_t cdp, uns32 event_handle );
    DllKernel BOOL WINAPI cd_Cancel_event_wait( cdp_t cdp, uns32 event_handle );
    DllKernel BOOL WINAPI cd_Wait_for_event( cdp_t cdp, sig32 timeout, uns32 * event_handle, uns32 * event_count, char * param_str, sig32 param_size, sig32 * result );
    /* XML rozsireni */
#if WBVERS<=810
    DllXMLExt int WINAPI Edit_XML_mapping( cdp_t cdp, tobjnum objnum, HWND hParentWnd, uns8 flags );
#endif
    DllXMLExt BOOL WINAPI Export_to_XML( cdp_t cdp, const char * dad_ref, const char * fname, tcursnum curs, struct t_clivar * hostvars, int hostvarscount );
    DllXMLExt BOOL WINAPI Import_from_XML( cdp_t cdp, const char * dad_ref, const char * fname, struct t_clivar * hostvars, int hostvarscount );
    DllXMLExt BOOL WINAPI Export_to_XML_buffer( cdp_t cdp, const char * dad_ref, char * buffer, int bufsize, int * xmlsize, tcursnum curs, struct t_clivar * hostvars, int hostvarscount );
    DllXMLExt BOOL WINAPI Export_to_XML_buffer_alloc( cdp_t cdp, const char * dad_ref, char * * buffer, tcursnum curs, struct t_clivar * hostvars, int hostvarscount );
    DllXMLExt BOOL WINAPI Import_from_XML_buffer( cdp_t cdp, const char * dad_ref, const char * buffer, int xmlsize, struct t_clivar * hostvars, int hostvarscount );
    DllXMLExt BOOL WINAPI Verify_DAD( cdp_t cdp, const char * dad, int * line, int * column );
    DllXMLExt char * WINAPI get_xml_form( cdp_t cdp, tobjnum objnum, BOOL translate, const char * url_prefix );
    /* zastarale funkce */
    DllKernel BOOL WINAPI cd_Uninst_table( cdp_t cdp, ttablenum table );
    DllKernel BOOL WINAPI Save_table( ttablenum table, const char * filename );
    DllKernel BOOL WINAPI cd_Save_table( cdp_t cdp, ttablenum table, const char * filename );
    DllKernel BOOL WINAPI Restore_table( ttablenum table, const char * filename );
    DllKernel BOOL WINAPI Get_object_rights( const char * objname, tcateg category, const char * username, tright * rights );
    DllKernel BOOL WINAPI Set_object_rights( const char * objname, tcateg category, const char * username, tright rights );
    DllKernel BOOL WINAPI Get_data_rights( ttablenum table, const char * username, tright * rights, tdright * rd_ri, tdright * wr_ri );
    DllKernel BOOL WINAPI Set_data_rights( ttablenum table, const char * username, tright rights, tdright rd_ri, tdright wr_ri );
    DllKernel BOOL WINAPI Create_group( const char * name );
    DllKernel BOOL WINAPI User_to_group( tobjnum user, tobjnum group, BOOL state );
    DllKernel BOOL WINAPI User_in_group( tobjnum user, tobjnum group, BOOL * state );


#if WBVERS>=900
    DllKernel BOOL WINAPI Chng_component_flag( cdp_t cdp, tcateg cat, const char * name, int mask, int setbit );
    DllKernel BOOL WINAPI Move_obj_to_folder( cdp_t cdp, const char * ObjName, tcateg Categ, const char * DestFolder );


#endif
    /* Typ callbacku pro enum_servers. */
    typedef int (server_callback)( const char * name, struct sockaddr_in * addr, void * data );
    /*
    Vysle zadost o serverove info broadcastem; ceka na odpovedi, pro kazdy
    server, ktery odpovi, provede callback. K navratu dojde, pokud nektery
    callback vrati nulu, nebo pokud po dobu timeout milisekund neprijde dalsi
    serverove info. Pokud je callback funkce NULL, provede se defaultni
    callback, ktery vypise na standartni vystup seznam nalezenych serveru.

    Vraci 0 (KSE_OK) nebo KSE_WINSOCK_ERROR.
         */
    DllKernel int enum_servers( int timeout, server_callback cbck, void * data );



#ifdef __cplusplus
} /* of extern "C" */
#endif

#endif  /* !def __WBKERNEL_H__ */

