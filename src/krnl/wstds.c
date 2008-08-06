#if WBVERS<=810
#include "wprez.h"  // load cache adr etc.
#else
#include "wprez9.h"  // load cache adr etc.
#endif
#define ATT_WINID  ATT_INT32
#define ATT_objnum ATT_INT32
/*************************** standard objects *******************************/
typeobj 
  att_null,
  att_boolean,
  att_char,
  att_int16,
  att_int32,
  att_money,
  att_float,
  att_string,
  att_binary,
  att_date,
  att_time,
  att_timestamp,
  att_ptr,
  att_biptr,
  att_autor,
  att_datim,
  att_hist,
  att_raster,
  att_text,
  att_nospec,
  att_signat,
  att_untyped,
  att_marker,
  att_indnum,
  att_interval,
  att_nil,
  att_file,
  att_table,
  att_cursor,
  att_dbrec,
  att_varlen,
  att_statcurs,
  att_odbc_numeric,
  att_odbc_decimal,
  att_odbc_time,
  att_odbc_date,
  att_odbc_timestamp,
  att_int8,
  att_int64,
  att_domain;

typeobj * simple_types[ATT_AFTERLAST] = 
{ &att_null,
  &att_boolean,
  &att_char,
  &att_int16,
  &att_int32,
  &att_money,
  &att_float,
  &att_string, NULL, NULL,
  &att_binary,
  &att_date,
  &att_time,
  &att_timestamp,
  &att_ptr,
  &att_biptr,
  &att_autor,
  &att_datim,
  &att_hist,
  &att_raster,
  &att_text,
  &att_nospec,
  &att_signat,
  &att_untyped,
  &att_marker,
  &att_indnum,
  &att_interval,
  &att_nil,
  &att_file,
  &att_table,
  &att_cursor,
  &att_dbrec,
  &att_varlen,
  &att_statcurs,
  NULL, NULL, NULL, NULL, NULL, NULL, 
  &att_odbc_numeric,
  &att_odbc_decimal,
  &att_odbc_time,
  &att_odbc_date,
  &att_odbc_timestamp,
  &att_int8,
  &att_int64,
  &att_domain };

constobj
   false_cdesc = { O_CONST,  0, &att_boolean },
    true_cdesc = { O_CONST,  1, &att_boolean },
     nil_cdesc = { O_CONST,  0, &att_nil     },
 black_cdesc   = { O_CONST,  0, &att_int16   },
 blue_cdesc    = { O_CONST, 64, &att_int16   },
 green_cdesc   = { O_CONST, 32, &att_int16   },
 cyan_cdesc    = { O_CONST, 96, &att_int16   },
 red_cdesc     = { O_CONST,  4, &att_int16   },
 magneta_cdesc = { O_CONST, 68, &att_int16   },
 brown_cdesc   = { O_CONST, 36, &att_int16   },
 l_gray_cdesc  = { O_CONST,173, &att_int16   },
 dark_gray_cdesc={ O_CONST, 82, &att_int16   },
 l_blue_cdesc  = { O_CONST,192, &att_int16   },
 l_green_cdesc = { O_CONST, 56, &att_int16   },
 l_cyan_cdesc  = { O_CONST,248, &att_int16   },
 l_red_cdesc   = { O_CONST,  7, &att_int16   },
 l_magenta_cdesc={ O_CONST,199, &att_int16   },
 yellow_cdesc  = { O_CONST, 63, &att_int16   },
 l_white_cdesc = { O_CONST,255, &att_int16   },

 nullchar_cdesc= { O_CONST,  0, &att_char    },
 nullbool_cdesc= { O_CONST,128, &att_boolean },
 nullsig8_cdesc= { O_CONST,  0, &att_int8    },
 nullsig16_cdesc={ O_CONST,  0, &att_int16   },
 nullsig32_cdesc={ O_CONST,  0, &att_int32   },
 nullsig64_cdesc={ O_CONST,  0, &att_int64   },
 nullmoney_cdesc={ O_CONST,  0, &att_money   },
 nulldate_cdesc= { O_CONST,  0, &att_date    },
 nulltime_cdesc= { O_CONST,  0, &att_time    },
 nullts_cdesc  = { O_CONST,  0, &att_timestamp},
 nullreal_cdesc= { O_CONST,  0, &att_float   },
 nullptr_cdesc = { O_CONST, -1, &att_ptr     },

#if WBVERS<900
 nodelete_cd   = { O_CONST,  NO_DELETE,   &att_int16   },
 noedit_cd     = { O_CONST,  NO_EDIT,     &att_int16   },
 noinsert_cd   = { O_CONST,  NO_INSERT,   &att_int16   },
 nomove_cd     = { O_CONST,  NO_MOVE,     &att_int16   },
 acc_cdesc     = { O_CONST,  AUTO_CURSOR, &att_int16   },
 count_cdesc   = { O_CONST,  COUNT_RECS,  &att_int16   },
 delrecs_cdesc = { O_CONST,  DEL_RECS,    &att_int16   },
 no_redir      = { O_CONST,  NOCURSOR,    &att_cursor  },
#endif

 ct_appl_cdesc = { O_CONST,  CATEG_APPL,      &att_int32   },
 ct_curs_cdesc = { O_CONST,  CATEG_CURSOR,    &att_int32   },
 ct_menu_cdesc = { O_CONST,  CATEG_MENU,      &att_int32   },
 ct_pgms_cdesc = { O_CONST,  CATEG_PGMSRC,    &att_int32   },
 ct_pgme_cdesc = { O_CONST,  CATEG_PGMEXE,    &att_int32   },
 ct_tabl_cdesc = { O_CONST,  CATEG_TABLE,     &att_int32   },
 ct_user_cdesc = { O_CONST,  CATEG_USER,      &att_int32   },
 ct_view_cdesc = { O_CONST,  CATEG_VIEW,      &att_int32   },
 ct_pict_cdesc = { O_CONST,  CATEG_PICT,      &att_int32   },
 ct_link_cdesc = { O_CONST,  IS_LINK,         &att_int32   },
 ct_dirc_cdesc = { O_CONST,  CATEG_DIRCUR,    &att_int32   },
 ct_grup_cdesc = { O_CONST,  CATEG_GROUP,     &att_int32   },
 ct_role_cdesc = { O_CONST,  CATEG_ROLE,      &att_int32   },
 ct_conn_cdesc = { O_CONST,  CATEG_CONNECTION,&att_int32   },
 ct_rela_cdesc = { O_CONST,  CATEG_RELATION,  &att_int32   },
 ct_graf_cdesc = { O_CONST,  CATEG_GRAPH,     &att_int32   },
 ct_schm_cdesc = { O_CONST,  CATEG_DRAWING,   &att_int32   },
 ct_proc_cdesc = { O_CONST,  CATEG_PROC,      &att_int32   },
 ct_trig_cdesc = { O_CONST,  CATEG_TRIGGER,   &att_int32   },
 ct_www_cdesc  = { O_CONST,  CATEG_WWW,       &att_int32   },
 ct_seq_cdesc  = { O_CONST,  CATEG_SEQ,       &att_int32   },
 ct_serv_cdesc = { O_CONST,  CATEG_SERVER,    &att_int32   },
 ct_repl_cdesc = { O_CONST,  CATEG_REPLREL,   &att_int32   },
 ct_dom_cdesc  = { O_CONST,  CATEG_DOMAIN,    &att_int32   },

 ri_del        = { O_CONST,  RIGHT_DEL,      &att_int16   },
 ri_grant      = { O_CONST,  RIGHT_GRANT,    &att_int16   },
 ri_insert     = { O_CONST,  RIGHT_INSERT,   &att_int16   },
 ri_read       = { O_CONST,  RIGHT_READ,     &att_int16   },
 ri_write      = { O_CONST,  RIGHT_WRITE,    &att_int16   },

 tab_tablenum  = { O_CONST,  TAB_TABLENUM,   &att_int32   },
 obj_tablenum  = { O_CONST,  OBJ_TABLENUM,   &att_int32   },
 user_tablenum = { O_CONST,  USER_TABLENUM,  &att_int32   },
 repl_tablenum = { O_CONST,  REPL_TABLENUM,  &att_int32   },
 srv_tablenum  = { O_CONST,  SRV_TABLENUM,   &att_int32   },
 key_tablenum  = { O_CONST,  KEY_TABLENUM,   &att_int32   },

#if WBVERS<900
 modal_v_cdesc = { O_CONST,  MODAL_VIEW,     &att_int32   },
 model_v_cdesc = { O_CONST,  MODELESS_VIEW,  &att_int32   },
 par_c_cdesc   = { O_CONST,  PARENT_CURSOR,  &att_int32   },
 query_cdesc   = { O_CONST,  QUERY_VIEW,     &att_int32   },

 r_visi_cdesc  = { O_CONST,  RESET_NONE   ,  &att_int32   },
 r_cntr_cdesc  = { O_CONST,  RESET_ALL    ,  &att_int32   },
 r_dels_cdesc  = { O_CONST,  RESET_ALL_REMAP,&att_int32   },
 r_cache_cdesc = { O_CONST,  RESET_CACHE  ,  &att_int32   },
 r_curs_cdesc  = { O_CONST,  RESET_CURSOR ,  &att_int32   },
 r_syn_cdesc   = { O_CONST,  RESET_SYNCHRO,  &att_int32   },
 r_combo_cdesc = { O_CONST,  RESET_COMBOS,   &att_int32   },
#endif

 oper_set_cdesc   = { O_CONST,  0,   &att_int32   },
 oper_get_cdesc   = { O_CONST,  1,   &att_int32   },
 oper_geteff_cdesc= { O_CONST,  2,   &att_int32   },

 vt_objnum_cdesc  = { O_CONST,  0,   &att_int32   },
 vt_name_cdesc    = { O_CONST,  1,   &att_int32   },
 vt_uuid_cdesc    = { O_CONST,  2,   &att_int32   }

;


spar p_vint_4[]  =  { { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
#define p_vint_3 (p_vint_4 + 1)
#define p_vint_2 (p_vint_4 + 2)
#define p_vint_1 (p_vint_4 + 3)
#define p_none   (p_vint_4 + 4)
#define p_vint32 (p_vint_4 + 3)
#define p_vi322  (p_vint_4 + 2)

spar p_open_v[]  =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_WINID },
                      { O_REFPAR, ATT_WINID },
                      { 0, 0 } };
spar p_open_q[]  =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_WINID },
                      { 0, 0 } };
spar p_print_v[] =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
#define p_vlng_2 (p_print_v + 2)
spar p_selrecs[] =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_NIL   },
                      { O_VALPAR, ATT_WINID },
                      { O_REFPAR, ATT_WINID },
                      { 0, 0 } };
spar p_set_pri[] =  { { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_date[]    =  { { O_VALPAR, ATT_DATE  },
                      { 0, 0 } };
spar p_time[]    =  { { O_VALPAR, ATT_TIME  },
                      { 0, 0 } };
spar p_bool[]    =  { { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_real[]    =  { { O_VALPAR, ATT_FLOAT },
                      { 0, 0 } };
spar p_char[]    =  { { O_VALPAR, ATT_CHAR  },
                      { 0, 0 } };
spar p_r4[]      =  { { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_h[]       =  { { O_VALPAR, ATT_HANDLE },
                      { 0, 0 } };
spar p_str1[]    =  { { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_stcurs[]  =  { { O_REFPAR, ATT_STATCURS},
                      { 0, 0 } };
spar p_stcustr[] =  { { O_REFPAR, ATT_STATCURS},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_custr[]   =  { { O_REFPAR, ATT_CURSOR},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_custr4[]  =  { { O_REFPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_STRING},
                      { O_VALPAR, ATT_STRING},
                      { O_VALPAR, ATT_STRING},
                      { O_VALPAR, ATT_STRING},
                      { 0, 0 } };
spar p_refcurs[] =  { { O_REFPAR, ATT_CURSOR},
                      { 0, 0 } };
spar p_curs[]    =  { { O_VALPAR, ATT_CURSOR},
                      { 0, 0 } };
spar p_reccnt[]  =  { { O_VALPAR, ATT_CURSOR},
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_currec[]  =  { { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_table[]   =  { { O_VALPAR, ATT_TABLE },
                      { 0, 0 } };
spar p_iotable[] =  { { O_VALPAR, ATT_TABLE },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_filnm[]   =  { { O_REFPAR, ATT_FILE  },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_file[]    =  { { O_VALPAR, ATT_FILE  },
                      { 0, 0 } };
spar p_filepos[] =  { { O_VALPAR, ATT_FILE  },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_memcpy[]  =  { { O_REFPAR, ATT_NIL   },
                      { O_REFPAR, ATT_NIL   },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_monint[]  =  { { O_VALPAR, ATT_MONEY },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_relint[]  =  { { O_VALPAR, ATT_FLOAT },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_c2t[]     =  { { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_str2[]    =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_sii[]     =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_vsii[]    =  { { O_VALPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_ssi[]     =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_gvp[]     =  { { O_VALPAR, ATT_WINID },
                      { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_wbb[]     =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_ww[]      =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_WINID },
                      { 0, 0 } };
spar p_ifxb[]    =  { { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_iw44[]    =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_w2s[]     =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_edittx[]  =  { { O_REFPAR, ATT_DBREC },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_bindrec[] =  { { O_REFPAR, ATT_DBREC },
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_gfc[]     =  { { O_VALPAR, ATT_WINID },
                      { O_REFPAR, ATT_CURSOR},
                      { O_REFPAR, ATT_INT16 }, // Short ref flags
                      { 0, 0 } };
spar p_sfc[]     =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_findobj[] =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_objnum}, 
                      { 0, 0 } };
spar p_gobjri[]  =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_INT16 }, // Short ref param
                      { 0, 0 } };
spar p_sobjri[]  =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT16 }, // Short val param
                      { 0, 0 } };
spar p_gdatari[] =  { { O_VALPAR, ATT_TABLE },
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_INT16 }, // Short ref param
                      { O_REFPAR, ATT_INT16 }, // Short ref param
                      { O_REFPAR, ATT_INT16 }, // Short ref param
                      { 0, 0 } };
spar p_sdatari[] =  { { O_VALPAR, ATT_TABLE },
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT16 }, // Short val param
                      { O_VALPAR, ATT_INT16 }, // Short val param
                      { O_VALPAR, ATT_INT16 }, // Short val param
                      { 0, 0 } };
spar p_addrec[]  =  { { O_VALPAR, ATT_CURSOR},
                      { O_REFPAR, ATT_NIL   },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_superr[]  =  { { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_textsub[] =  { { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT16 }, // mindex
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_xbase[]   =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_datei[]   =  { { O_VALPAR, ATT_DATE  },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_timei[]   =  { { O_VALPAR, ATT_TIME  },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_usering[] =  { { O_VALPAR, ATT_objnum },
                      { O_VALPAR, ATT_objnum },
                      { O_REFPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_iib[]     =  { { O_VALPAR, ATT_objnum },
                      { O_VALPAR, ATT_objnum },
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_enind[]   =  { { O_VALPAR, ATT_TABLE  },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_send[]    =  { { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_wind[]    =  { { O_VALPAR, ATT_WINID },
                      { 0, 0 } };
spar p_wind_6[]  =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_wind_4[]  =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_wind_2[]  =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_action[]  =  { { O_VALPAR, ATT_INT32},
                      { O_VALPAR, ATT_STRING},
                      { O_VALPAR, ATT_STRING},
                      { O_VALPAR, ATT_STRING},
                      { 0, 0 } };
spar p_pr_sel[]  =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_aggr[]    =  { { O_VALPAR, ATT_CURSOR},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_UNTYPED },
                      { 0, 0 } };
spar p_count[]   =  { { O_VALPAR, ATT_CURSOR},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_lookup[]  =  { { O_VALPAR, ATT_CURSOR},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_UNTYPED },
                      { 0, 0 } };
spar p_ws[]      =  { { O_VALPAR, ATT_WINID },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_relrec[]  =  { { O_VALPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_WINID },
                      { O_VALPAR, ATT_STRING},
                      { 0, 0 } };
spar p_gsival[]  =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_UNTYPED },
                      { 0, 0 } };
spar p_reg_key[] =  { { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_get_ext[] =  { { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_WINID },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_dataexp[] =  { { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_dataimp[] =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_dbint[]   =  { { O_VALPAR, ATT_BOOLEAN},
                      { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_crelink[] =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_cre2lnk[] =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_BINARY},
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_o_curs[]  =  { { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_CURSOR},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_ex_dir[]  =  { { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_privils[] =  { { O_VALPAR, ATT_objnum},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_TABLE },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_NIL   },
                      { 0, 0 } };
spar p_grprole[] =  { { O_VALPAR, ATT_objnum},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_wi     [] =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_wi16   [] =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_objnum},
                      { 0, 0 } };
spar p_objnum [] =  { { O_VALPAR, ATT_objnum},
                      { 0, 0 } };
spar p_nextuser[] = { { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_NIL   },
                      { 0, 0 } };
spar p_sigstat[] =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_docflow[] =  { { O_VALPAR, ATT_BINARY},
                      { 0, 0 } };
spar p_fobjbi[]  =  { { O_REFPAR, ATT_NIL   },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_objnum},
                      { 0, 0 } };
spar p_movedt[]  =  { { O_VALPAR, ATT_objnum },
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_objnum },
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_filsize[] =  { { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_nil[]     =  { { O_VALPAR, ATT_NIL   },
                      { 0, 0 } };
spar p_refnil[]  =  { { O_REFPAR, ATT_NIL   },
                      { 0, 0 } };
spar p_new[]     =  { { O_REFPAR, ATT_NIL   },
//                    { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_dattim[]  =  { { O_VALPAR, ATT_DATE  },
                      { O_VALPAR, ATT_TIME  },
                      { 0, 0 } };
spar p_timest[]  =  { { O_VALPAR, ATT_TIMESTAMP},
                      { 0, 0 } };
spar p_timesti[] =  { { O_VALPAR, ATT_TIMESTAMP},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_creuser[] =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_BINARY},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_objnum},
                      { 0, 0 } };
spar p_chngcf[]  =  { { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_binary[]  =  { { O_VALPAR, ATT_BINARY},
                      { 0, 0 } };
spar p_token[]   =  { { O_VALPAR, ATT_CURSOR},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_edrel[]   =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_objnum},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_edpriv[]  =  { { O_VALPAR, ATT_WINID },
                      { O_REFPAR, ATT_TABLE },
                      { O_REFPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_prepare[] =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_rwnd[]    =  { { O_REFPAR, ATT_WINID },
                      { 0, 0 } };
spar p_is[]      =  { { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_hs[]      =  { { O_VALPAR, ATT_HANDLE},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_bbb[]     =  { { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_gluser[]  =  { { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_ri2[]     =  { { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_str3[]    =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_hi[]      =  { { O_VALPAR, ATT_HANDLE},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_hii[]    =   { { O_VALPAR, ATT_HANDLE},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
spar p_crelet[]  =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_HANDLE},
                      { 0, 0 } };
spar p_adrlet[]  =  { { O_VALPAR, ATT_HANDLE},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_expapl[]  =  { { O_VALPAR, ATT_WINID },
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_iivx[]    =  { { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32},
                      { O_REFPAR, ATT_NIL },
                      { 0, 0 } };
spar p_ssb[]     =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_mbsvas[]  =  { { O_VALPAR, ATT_HANDLE},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_mbdelm[]  =  { { O_VALPAR, ATT_HANDLE },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_mbinfo[]  =  { { O_VALPAR, ATT_HANDLE },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_INT16  },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_INT16  },
                      { 0, 0 } };
spar p_ft_index[]=  { { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { 0, 0 } };
spar p_irint5[]  =  { { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_INT32  },
                      { O_REFPAR, ATT_INT32  },
                      { O_REFPAR, ATT_INT32  },
                      { O_REFPAR, ATT_INT32  },
                      { O_REFPAR, ATT_INT32  },
                      { 0, 0 } };
spar p_isi[]     =  { { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { 0, 0 } };

spar p_ini[]     =  { { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_NIL },
                      { O_VALPAR, ATT_INT32},
                      { 0, 0 } };

spar p_db2file[] =  { { O_REFPAR, ATT_DBREC },
                      { O_REFPAR, ATT_STRING },
                      { 0, 0 } };

spar p_file2db3[] = { { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_DBREC },
                      { O_REFPAR, ATT_DBREC },
                      { O_REFPAR, ATT_DBREC },
                      { 0, 0 } };
spar p_bigint[]   = { { O_VALPAR, ATT_INT64 },
                      { 0, 0 } };
spar p_objtofolder[]={{ O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { 0, 0 } };

spar p_selrec[]  =  { { O_VALPAR, ATT_WINID  },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_issel[]  =   { { O_VALPAR, ATT_WINID  },
                      { O_VALPAR, ATT_INT32  },
                      { 0, 0 } };
spar p_blobr[] =    { { O_VALPAR, ATT_HANDLE  },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_CURSOR },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT8   },
                      { O_VALPAR, ATT_INT16  },
                      { 0, 0 } };
spar p_blobs[] =    { { O_VALPAR, ATT_HANDLE },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { 0, 0 } };
spar p_sfdbr[] =    { { O_VALPAR, ATT_HANDLE },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_CURSOR },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT8   },
                      { O_VALPAR, ATT_INT16  },
                      { 0, 0 } };
spar p_sfdbs[] =    { { O_VALPAR, ATT_HANDLE },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { 0, 0 } };
spar p_gp[]      =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32  },
                      { 0, 0 } };
spar p_cp[]      =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_gprop[]   =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32  },
                      { 0, 0 } };
spar p_sprop[]   =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_mbopen[]  =  { { O_REFPAR, ATT_HANDLE},
                      { 0, 0 } };
spar p_refbool[] =  { { O_REFPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_impxml[]  =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
spar p_impxmlb[] =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32  },
                      { 0, 0 } };
spar p_expxml[]  =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_CURSOR},
                      { 0, 0 } };
spar p_expxmlb[] =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_INT32  },
                      { O_VALPAR, ATT_CURSOR},
                      { 0, 0 } };
spar p_valstr[]  =  { { O_VALPAR, ATT_STRING},
                      { 0, 0 } };

procobj  d_getitem    ={O_SPROC, 0, 8, p_wind_2 ,ATT_STRING   };//4 
procobj  d_setitem    ={O_SPROC, 1,12, p_w2s    ,0            };//4
procobj  d_resetview  ={O_SPROC, 2,12, p_iw44   ,0            };//4
procobj  d_main_menu  ={O_SPROC, 3, 4, p_str1   ,ATT_BOOLEAN  };//4
procobj  d_open_view  ={O_SPROC, 4,24, p_open_v ,ATT_WINID    };//4
procobj  d_print_view ={O_SPROC, 5,16, p_print_v,ATT_BOOLEAN  };//4
procobj  d_close_view ={O_SPROC, 6, 4, p_wind   ,0            };//4
procobj  d_bind_recs  ={O_SPROC, 7,36, p_bindrec,ATT_WINID    };//4
procobj  d_select_recs={O_SPROC, 8,28, p_selrecs,ATT_WINID    };//4
procobj  d_set_ipos   ={O_SPROC, 9,12, p_iw44   ,ATT_BOOLEAN  };//4
procobj  d_set_epos   ={O_SPROC,10,12, p_iw44   ,ATT_BOOLEAN  };//4
procobj  d_get_view   ={O_SPROC,11,12, p_gvp    ,ATT_BOOLEAN  };//4
procobj  d_edit_text  ={O_SPROC,12,40, p_edittx ,ATT_BOOLEAN  };//4
procobj  d_pick       ={O_SPROC,13, 4, p_wind   ,0            };//4
procobj  d_put_pixel  ={O_SPROC,14,16, p_wind_4 ,0            };//4
procobj  d_draw_line  ={O_SPROC,15,24, p_wind_6 ,0            };//4
procobj  d_commit_view={O_SPROC,16,12, p_wbb    ,ATT_BOOLEAN  };//4
procobj  d_alogin     ={O_SPROC,17, 4, p_wind   ,ATT_BOOLEAN  };//4
procobj  d_print_opt  ={O_SPROC,18, 4, p_wind   ,ATT_BOOLEAN  };//4
procobj  d_roll_back_v={O_SPROC,19, 4, p_wind   ,0            };//4
procobj  d_set_printe ={O_SPROC,20,28, p_set_pri,0            };//4
procobj  d_exec       ={O_SPROC,21, 8, p_str2   ,ATT_INT32    };//4
procobj  d_get_messg  ={O_SPROC,22, 4, p_r4     ,ATT_BOOLEAN  };//4
procobj  d_infobox    ={O_SPROC,23, 8, p_str2   ,0            };//4
procobj  d_yesnobox   ={O_SPROC,24, 8, p_str2   ,ATT_BOOLEAN  };//4
procobj  d_areplicate ={O_SPROC,25, 4, p_bool   ,ATT_BOOLEAN  };//4
procobj  d_get_fcurs  ={O_SPROC,26,12, p_gfc    ,ATT_BOOLEAN  };//4
procobj  d_set_fcurs  ={O_SPROC,27,12, p_sfc    ,ATT_BOOLEAN  };//4
procobj  d_viewclose  ={O_SPROC,28, 4, p_str1   ,0            };//4
procobj  d_viewopen   ={O_SPROC,29, 4, p_str1   ,0            };//4
procobj  d_viewprint  ={O_SPROC,30, 4, p_str1   ,ATT_BOOLEAN  };//4
procobj  d_viewstate  ={O_SPROC,31, 4, p_str1   ,ATT_BOOLEAN  };//4
procobj  d_viewtoggle ={O_SPROC,32, 4, p_str1   ,0            };//4
procobj  d_peekmess   ={O_SPROC,33, 0, p_none   ,ATT_BOOLEAN  };//4
procobj  d_printer_d  ={O_SPROC,34, 4, p_wind   ,0            };//4
procobj  d_set_cursor ={O_SPROC,35, 4, p_vint_1 ,ATT_INT32    };//4
procobj  d_set_st_nums={O_SPROC,36, 8, p_vlng_2 ,0            };//4
procobj  d_set_st_text={O_SPROC,37, 4, p_str1   ,0            };//4
procobj  d_fromxbase  ={O_SPROC,38,16, p_xbase  ,ATT_BOOLEAN  };//4
procobj  d_toxbase    ={O_SPROC,39,16, p_xbase  ,ATT_BOOLEAN  };//4
procobj  d_print_marg ={O_SPROC,40,16, p_vint_4 ,0            };//4
procobj  d_curr_item  ={O_SPROC,41, 4, p_wind   ,ATT_INT32    };//4
procobj  d_curr_view  ={O_SPROC,42, 0, p_none   ,ATT_WINID    };//4
procobj  d_exp_apl_ex ={O_SPROC,43,24, p_expapl ,ATT_BOOLEAN  };//4
procobj  d_closeallv  ={O_SPROC,44, 0, p_none   ,0            };//4
procobj  d_inputbox   ={O_SPROC,45,12, p_ssi    ,ATT_BOOLEAN  };//4
procobj  d_print_cpy  ={O_SPROC,46, 8, p_ifxb   ,ATT_BOOLEAN  };//4
procobj  d_show_help  ={O_SPROC,47, 4, p_vint32 ,0            };//4
procobj  d_help_file  ={O_SPROC,48, 4, p_str1   ,0            };//4
procobj  d_send_mess  ={O_SPROC,49,16, p_send   ,ATT_INT32    };//4
procobj  d_action     ={O_SPROC,50,772,p_action ,0            };//4
procobj  d_printer_sel={O_SPROC,51,36, p_pr_sel ,ATT_BOOLEAN  };//4
procobj  d_sel_file   ={O_SPROC,52, 8 ,p_ws     ,ATT_BOOLEAN  };//4
procobj  d_relrec     ={O_SPROC,53,528,p_relrec ,ATT_WINID    };//4
procobj  d_get_i_val  ={O_SPROC,54,16 ,p_gsival ,ATT_BOOLEAN  };//4
procobj  d_set_i_val  ={O_SPROC,55,16 ,p_gsival ,ATT_BOOLEAN  };//4
procobj  d_reg_key    ={O_SPROC,56,20 ,p_reg_key,0            };//4
procobj  d_get_ext_msg={O_SPROC,57,12 ,p_get_ext,ATT_BOOLEAN  };//4
procobj  d_data_exp   ={O_SPROC,58,20 ,p_dataexp,ATT_BOOLEAN  };//4
procobj  d_data_imp   ={O_SPROC,59,20 ,p_dataimp,ATT_BOOLEAN  };//4
procobj  d_edit_table ={O_SPROC,60, 4, p_str1,   ATT_BOOLEAN  };//4
procobj  d_reg_synchr ={O_SPROC,61, 8, p_ww,     ATT_BOOLEAN  };//4
procobj  d_sel_dir    ={O_SPROC,62, 8 ,p_ws     ,ATT_BOOLEAN  };//4
procobj  d_qbe_state  ={O_SPROC,63, 4, p_wind   ,ATT_INT32    };//4
procobj  d_acre_user  ={O_SPROC,64, 4 ,p_wind   ,ATT_BOOLEAN  };//4
procobj  d_next_uname ={O_SPROC,65, 8 ,p_wi     ,ATT_STRING   };//4
procobj  d_sigstate   ={O_SPROC,66,12 ,p_sigstat,ATT_INT32    };//4
procobj  d_move_data  ={O_SPROC,67,36 ,p_movedt, ATT_BOOLEAN  }; //4
procobj  d_tab_page   ={O_SPROC,68, 8 ,p_wi     ,ATT_INT32    }; //4
procobj  d_pagesetup  ={O_SPROC,69, 4 ,p_wind   ,ATT_BOOLEAN  }; //4
procobj  d_show_helpp ={O_SPROC,70, 4, p_vint32 ,0            }; //4
procobj  d_edit_view  ={O_SPROC,71, 4, p_str1,   ATT_BOOLEAN  }; //4
procobj  d_chngcomp   ={O_SPROC,72,16, p_chngcf, ATT_BOOLEAN  }; //4
procobj  d_atoken     ={O_SPROC,73,12, p_sfc   , ATT_BOOLEAN  }; //4
procobj  d_edit_rel   ={O_SPROC,74,16, p_edrel , ATT_BOOLEAN  }; //4
procobj  d_edit_privs ={O_SPROC,75,16, p_edpriv, ATT_BOOLEAN  }; //4
procobj  d_asetpass   ={O_SPROC,76, 4, p_wind,   ATT_BOOLEAN  }; //4
procobj  d_amod_user  ={O_SPROC,77, 8, p_wi16,   ATT_BOOLEAN  }; //4
procobj  d_edit_query ={O_SPROC,78, 4, p_str1,   ATT_BOOLEAN  }; //4
procobj  d_edit_ie    ={O_SPROC,79, 4, p_str1,   0            }; //4

procobj  d_make_date  ={O_SPROC,80,12, p_vint_3 ,ATT_DATE    };  //4
procobj  d_day        ={O_SPROC,81, 4, p_date   ,ATT_INT32   };  //4
procobj  d_month      ={O_SPROC,82, 4, p_date   ,ATT_INT32   };  //4
procobj  d_year       ={O_SPROC,83, 4, p_date   ,ATT_INT32   };  //4
procobj  d_today      ={O_SPROC,84, 0, p_none   ,ATT_DATE    };  //4
procobj  d_make_time  ={O_SPROC,85,16, p_vint_4 ,ATT_TIME    };  //4
procobj  d_hours      ={O_SPROC,86, 4, p_time   ,ATT_INT32   };  //4
procobj  d_minutes    ={O_SPROC,87, 4, p_time   ,ATT_INT32   };  //4
procobj  d_seconds    ={O_SPROC,88, 4, p_time   ,ATT_INT32   };  //4
procobj  d_sec1000    ={O_SPROC,89, 4, p_time   ,ATT_INT32   };  //4
procobj  d_now        ={O_SPROC,90, 0, p_none   ,ATT_TIME    };  //4
procobj  d_ord        ={O_SPROC,91, 4, p_char   ,ATT_INT32   };  //4
procobj  d_chr        ={O_SPROC,92, 4, p_vint_1 ,ATT_CHAR    };  //4
procobj  d_odd        ={O_SPROC,93, 4, p_vint32 ,ATT_BOOLEAN };  //4
procobj  d_abs        ={O_SPROC,94, 8, p_real   ,ATT_FLOAT   };  //4
procobj  d_iabs       ={O_SPROC,95, 4, p_vint32 ,ATT_INT32   };  //4
procobj  d_sqrt       ={O_SPROC,96, 8, p_real   ,ATT_FLOAT   };  //4
procobj  d_round      ={O_SPROC,97, 8, p_real   ,ATT_INT32   };  //4
procobj  d_trunc      ={O_SPROC,98, 8, p_real   ,ATT_INT32   };  //4
procobj  d_sqr        ={O_SPROC,99, 8, p_real   ,ATT_FLOAT   };  //4
procobj  d_isqr       ={O_SPROC,100, 4, p_vint32 ,ATT_INT32   }; //4
procobj  d_sin        ={O_SPROC,101, 8, p_real   ,ATT_FLOAT   }; //4
procobj  d_cos        ={O_SPROC,102, 8, p_real   ,ATT_FLOAT   }; //4
procobj  d_arctan     ={O_SPROC,103, 8, p_real   ,ATT_FLOAT   }; //4
procobj  d_ln         ={O_SPROC,104, 8, p_real   ,ATT_FLOAT   }; //4
procobj  d_exp        ={O_SPROC,105, 8, p_real   ,ATT_FLOAT   }; //4
procobj  d_like       ={O_SPROC,106, 8, p_str2   ,ATT_BOOLEAN }; //4
procobj  d_pref       ={O_SPROC,107, 8, p_str2   ,ATT_BOOLEAN }; //4
procobj  d_substr     ={O_SPROC,108, 8, p_str2   ,ATT_BOOLEAN }; //4
procobj  d_strcat     ={O_SPROC,109, 8, p_str2   ,ATT_STRING  }; //4
procobj  d_upcase     ={O_SPROC,110, 4, p_str1   ,0           }; //4
procobj  d_memcpy     ={O_SPROC,111,12, p_memcpy ,0           }; //4
procobj  d_str2int    ={O_SPROC,112, 4, p_str1   ,ATT_INT32   }; //4
procobj  d_str2money  ={O_SPROC,113, 4, p_str1   ,ATT_MONEY   }; //4
procobj  d_str2real   ={O_SPROC,114, 4, p_str1   ,ATT_FLOAT   }; //4
procobj  d_int2str    ={O_SPROC,115, 4, p_vint32 ,ATT_STRING  }; //4
procobj  d_money2str  ={O_SPROC,116,12, p_monint ,ATT_STRING  }; //4
procobj  d_real2str   ={O_SPROC,117,12, p_relint ,ATT_STRING  }; //4
procobj  d_reset      ={O_SPROC,118, 8, p_filnm  ,ATT_BOOLEAN }; //4
procobj  d_rewrite    ={O_SPROC,119, 8, p_filnm  ,ATT_BOOLEAN }; //4
procobj  d_close      ={O_SPROC,120, 4, p_file   ,0           }; //4
procobj  d_eof        ={O_SPROC,121, 4, p_file   ,ATT_BOOLEAN }; //4
procobj  d_seek       ={O_SPROC,122, 8, p_filepos,ATT_BOOLEAN }; //4
procobj  d_filelength ={O_SPROC,123, 4, p_file   ,ATT_INT32   }; //4
procobj  d_strinsert  ={O_SPROC,124,12, p_ssi    ,ATT_STRING  }; //4
procobj  d_strdelete  ={O_SPROC,125,12, p_sii    ,ATT_STRING  }; //4
procobj  d_strcopy    ={O_SPROC,126,264,p_vsii   ,ATT_STRING  }; //4
procobj  d_strpos     ={O_SPROC,127, 8, p_str2   ,ATT_INT32   }; //4
procobj  d_strlenght  ={O_SPROC,128, 4, p_str1   ,ATT_INT32   }; //4
procobj  d_str2date   ={O_SPROC,129, 4, p_str1   ,ATT_DATE    }; //4
procobj  d_str2time   ={O_SPROC,130, 4, p_str1   ,ATT_TIME    }; //4
procobj  d_date2str   ={O_SPROC,131, 8, p_datei  ,ATT_STRING  }; //4
procobj  d_time2str   ={O_SPROC,132, 8, p_timei  ,ATT_STRING  }; //4
procobj  d_dayofweek  ={O_SPROC,133, 4, p_date   ,ATT_INT32   }; //4
procobj  d_strtrim    ={O_SPROC,134, 4, p_str1   ,ATT_STRING  }; //4
procobj  d_delfile    ={O_SPROC,135, 4, p_str1   ,ATT_BOOLEAN }; //4
procobj  d_mkdir      ={O_SPROC,136, 4, p_str1   ,ATT_BOOLEAN }; //4
procobj  d_WB_version ={O_SPROC,137, 0, p_none   ,ATT_INT32   }; //4
procobj  d_quarter    ={O_SPROC,138, 4, p_date   ,ATT_INT32   }; //4
procobj  d_waits_fm   ={O_SPROC,139, 2*UUID_SIZE,p_docflow,ATT_BOOLEAN};//4
procobj  d_viewpatt   ={O_SPROC,140, 4, p_objnum ,0           }; // 4
procobj  d_firstlab   ={O_SPROC,141, 8, p_vi322  ,0           }; // 4
procobj  d_dispose    ={O_SPROC,142, 4, p_nil    ,0           }; // 4
procobj  d_new        ={O_SPROC,143, 8, p_new    ,0           }; // 4
procobj  d_addr       ={O_SPROC,144, 4, p_refnil ,ATT_NIL     }; // 4
procobj  d_datetime2  ={O_SPROC,145, 8, p_dattim ,ATT_TIMESTAMP};// 4
procobj  d_dtm2dt     ={O_SPROC,146, 4, p_timest ,ATT_DATE    }; // 4
procobj  d_dtm2tm     ={O_SPROC,147, 4, p_timest ,ATT_TIME    }; // 4
procobj  d_is_repl_d  ={O_SPROC,148,16, p_binary ,ATT_BOOLEAN }; // 4
procobj  d_set_timer  ={O_SPROC,149, 8, p_vi322  ,0           }; // 4
procobj  d_str2timest ={O_SPROC,150, 4, p_str1   ,ATT_TIMESTAMP }; //4
procobj  d_timest2str ={O_SPROC,151, 8, p_timesti,ATT_STRING  }; //4

procobj  d_sz_error   ={O_SPROC,152, 0, p_none   ,ATT_INT32    };//4
procobj  d_sz_warning ={O_SPROC,153, 0, p_none   ,ATT_INT32    };//4
procobj  d_err_mask   ={O_SPROC,154, 4, p_bool   ,0            };//4
procobj  d_login      ={O_SPROC,155, 8, p_str2   ,ATT_BOOLEAN  };//4
procobj  d_logout     ={O_SPROC,156, 0, p_none   ,ATT_BOOLEAN  };//4
procobj  d_start_tra  ={O_SPROC,157, 0, p_none   ,ATT_BOOLEAN  };//4
procobj  d_commit     ={O_SPROC,158, 0, p_none   ,ATT_BOOLEAN  };//4
procobj  d_roll_back  ={O_SPROC,159, 0, p_none   ,ATT_BOOLEAN  };//4
procobj  d_open_cursor={O_SPROC,160, 4, p_stcurs ,ATT_BOOLEAN  };//4
procobj  d_close_curs ={O_SPROC,161, 4, p_refcurs,ATT_BOOLEAN  };//4
procobj  d_open_sql_c ={O_SPROC,162, 8, p_custr  ,ATT_BOOLEAN  };//4
procobj  d_open_sql_p ={O_SPROC,163,1028,p_custr4,ATT_BOOLEAN  };//4
procobj  d_restrict   ={O_SPROC,164, 8, p_stcustr,ATT_BOOLEAN  };//4
procobj  d_restore    ={O_SPROC,165, 4, p_stcurs ,ATT_BOOLEAN  };//4
procobj  d_rec_cnt    ={O_SPROC,166, 8, p_reccnt ,ATT_BOOLEAN  };//4
procobj  d_delete     ={O_SPROC,167, 8, p_currec ,ATT_BOOLEAN  };//4
procobj  d_undelete   ={O_SPROC,168, 8, p_currec ,ATT_BOOLEAN  };//4
procobj  d_free_del   ={O_SPROC,169, 4, p_table  ,ATT_BOOLEAN  };//4
procobj  d_append     ={O_SPROC,170, 4, p_curs   ,ATT_INT32    };//4
procobj  d_insert     ={O_SPROC,171, 4, p_curs   ,ATT_INT32    };//4
procobj  d_translate  ={O_SPROC,172,16, p_c2t    ,ATT_BOOLEAN  };//4
procobj  d_deftab     ={O_SPROC,173, 8, p_str2   ,ATT_BOOLEAN  };//4
procobj  d_findobj    ={O_SPROC,174,12, p_findobj,ATT_BOOLEAN  };//4
procobj  d_savetab    ={O_SPROC,175, 8, p_iotable,ATT_BOOLEAN  };//4
procobj  d_resttab    ={O_SPROC,176, 8, p_iotable,ATT_BOOLEAN  };//4
procobj  d_gdatari    ={O_SPROC,177,20, p_gdatari,ATT_BOOLEAN  };//4
procobj  d_gobjri     ={O_SPROC,178,16, p_gobjri ,ATT_BOOLEAN  };//4
procobj  d_sdatari    ={O_SPROC,179,20, p_sdatari,ATT_BOOLEAN  };//4
procobj  d_sobjri     ={O_SPROC,180,16, p_sobjri ,ATT_BOOLEAN  };//4
procobj  d_rlockt     ={O_SPROC,181, 4, p_curs   ,ATT_BOOLEAN  };//4
procobj  d_wlockt     ={O_SPROC,182, 4, p_curs   ,ATT_BOOLEAN  };//4
procobj  d_runlockt   ={O_SPROC,183, 4, p_curs   ,ATT_BOOLEAN  };//4
procobj  d_wunlockt   ={O_SPROC,184, 4, p_curs   ,ATT_BOOLEAN  };//4
procobj  d_rlockr     ={O_SPROC,185, 8, p_currec ,ATT_BOOLEAN  };//4
procobj  d_wlockr     ={O_SPROC,186, 8, p_currec ,ATT_BOOLEAN  };//4
procobj  d_runlockr   ={O_SPROC,187, 8, p_currec ,ATT_BOOLEAN  };//4
procobj  d_wunlockr   ={O_SPROC,188, 8, p_currec ,ATT_BOOLEAN  };//4
procobj  d_addrec     ={O_SPROC,189,12, p_addrec ,ATT_BOOLEAN  };//4
procobj  d_superrec   ={O_SPROC,190,16, p_superr ,ATT_BOOLEAN  };//4
procobj  d_textsubstr ={O_SPROC,191,36, p_textsub,ATT_BOOLEAN  };//4
procobj  d_uninst     ={O_SPROC,192, 4, p_table  ,ATT_BOOLEAN  };//4
procobj  d_set_pass   ={O_SPROC,193, 8, p_str2   ,ATT_BOOLEAN  };//4
procobj  d_insert_obj ={O_SPROC,194,12, p_findobj,ATT_BOOLEAN  };//4
procobj  d_signalize  ={O_SPROC,195, 0, p_none   ,ATT_BOOLEAN  };//4
procobj  d_createlink ={O_SPROC,196,16, p_crelink,ATT_BOOLEAN  };//4
procobj  d_relist     ={O_SPROC,197, 0, p_none   ,ATT_BOOLEAN  };//4
procobj  d_user_name  ={O_SPROC,198, 0, p_none   ,ATT_STRING   };//4
procobj  d_delallrec  ={O_SPROC,199, 4, p_curs   ,ATT_BOOLEAN  };//4
procobj  d_useringrp  ={O_SPROC,200,12, p_usering,ATT_BOOLEAN  };//4
procobj  d_usertogrp  ={O_SPROC,201,12, p_iib,    ATT_BOOLEAN  };//4
procobj  d_enableind  ={O_SPROC,202,12, p_enind,  ATT_BOOLEAN  };//4
procobj  d_creategrp  ={O_SPROC,203, 4, p_str1,   ATT_BOOLEAN  };//4
procobj  d_sqlexec    ={O_SPROC,NUM_SQLEXECUTE, 4, p_str1,   ATT_BOOLEAN  };//4
procobj  d_c_sum      ={O_SPROC,205,16, p_aggr,   ATT_BOOLEAN  };//4
procobj  d_c_avg      ={O_SPROC,206,16, p_aggr,   ATT_BOOLEAN  };//4
procobj  d_c_max      ={O_SPROC,207,16, p_aggr,   ATT_BOOLEAN  };//4
procobj  d_c_min      ={O_SPROC,208,16, p_aggr,   ATT_BOOLEAN  };//4
procobj  d_c_count    ={O_SPROC,209,16, p_count,  ATT_BOOLEAN  };//4
procobj  d_lookup     ={O_SPROC,210,12, p_lookup, ATT_INT32    };//4
procobj  d_availmem   ={O_SPROC,211, 4, p_bool,   ATT_INT32    };//4
procobj  d_owncurs    ={O_SPROC,212, 0, p_none,   ATT_INT32    };//4
procobj  d_db_integr  ={O_SPROC,213,24, p_dbint,  ATT_BOOLEAN  };//4
procobj  d_o_create   ={O_SPROC,214, 4, p_str1,   ATT_INT32    };//4
procobj  d_o_find     ={O_SPROC,215, 4, p_str1,   ATT_INT32    };//4
procobj  d_o_direct   ={O_SPROC,216, 4, p_str1,   ATT_INT32    };//4
procobj  d_o_cursor   ={O_SPROC,217,12, p_o_curs, ATT_BOOLEAN  };//4
procobj  d_privils    ={O_SPROC,218,24, p_privils,ATT_BOOLEAN  };//4
procobj  d_grprole    ={O_SPROC,219,20, p_grprole,ATT_BOOLEAN  };//4
procobj  d_next_user  ={O_SPROC,220,24, p_nextuser,ATT_BOOLEAN };//4
procobj  d_amidbadmin ={O_SPROC,221, 0, p_none,   ATT_BOOLEAN  };//4
procobj  d_findobjbyid={O_SPROC,222,12, p_fobjbi, ATT_BOOLEAN  };//4
procobj  d_currentapl ={O_SPROC,223, 0, p_none   ,ATT_STRING   };//4
procobj  d_filsize    ={O_SPROC,224, 8, p_filsize,ATT_BOOLEAN  }; //4
procobj  d_waiting    ={O_SPROC,225, 4, p_vint32, ATT_BOOLEAN  }; //4
procobj  d_createuser ={O_SPROC,226,32, p_creuser,ATT_BOOLEAN  }; //4
procobj  d_enablets   ={O_SPROC,227, 4, p_bool   ,0            }; //4
procobj  d_tokenctrl  ={O_SPROC,228,12, p_token  ,ATT_BOOLEAN  }; //4
procobj  d_create2link={O_SPROC,229,20, p_cre2lnk,ATT_BOOLEAN  }; //4
procobj  d_sqlprepare ={O_SPROC,230, 8, p_prepare,ATT_BOOLEAN  }; //4
procobj  d_sqlexecprep={O_SPROC,231, 4, p_vint32 ,ATT_BOOLEAN  }; //4
procobj  d_sqldrop    ={O_SPROC,232, 4, p_vint32 ,ATT_BOOLEAN  }; //4
procobj  d_sqlopt     ={O_SPROC,233, 8, p_vi322  ,ATT_BOOLEAN  }; //4
procobj  d_compactab  ={O_SPROC,234, 4, p_table  ,ATT_BOOLEAN  }; //4
procobj  d_compacall  ={O_SPROC,235, 4, p_vint32 ,ATT_BOOLEAN  }; //4
procobj  d_log_write  ={O_SPROC,236, 4, p_str1   ,0            }; //4
procobj  d_applinstcnt={O_SPROC,237, 4, p_r4     ,ATT_BOOLEAN  }; //4
procobj  d_set_starter={O_SPROC,238, 8, p_is     ,ATT_BOOLEAN  }; //4
procobj  d_set_progres={O_SPROC,239, 4, p_vint32 ,ATT_BOOLEAN  }; //4
procobj  d_get_l_user ={O_SPROC,240,16, p_gluser ,ATT_BOOLEAN  }; //4
procobj  d_conn_spd   ={O_SPROC,241, 8, p_ri2    ,ATT_BOOLEAN  }; //4
procobj  d_mess2cli   ={O_SPROC,242, 4, p_str1   ,ATT_BOOLEAN  }; //4
procobj  d_filblocks  ={O_SPROC,243, 8, p_filsize,ATT_BOOLEAN  }; //4
procobj  d_ml_init    ={O_SPROC,244, 8, p_str2   ,ATT_INT32    }; //4
procobj  d_ml_init602 ={O_SPROC,245,12, p_str3   ,ATT_INT32    }; //4
procobj  d_ml_close   ={O_SPROC,246, 0, p_none   ,0            }; //4
procobj  d_ml_cre     ={O_SPROC,247,16, p_crelet ,ATT_INT32    }; //4
procobj  d_ml_addr    ={O_SPROC,248,16, p_adrlet ,ATT_INT32    }; //4
procobj  d_ml_file    ={O_SPROC,249, 8, p_hs     ,ATT_INT32    }; //4
procobj  d_ml_send    ={O_SPROC,250, 4, p_h      ,ATT_INT32    }; //4
procobj  d_ml_take    ={O_SPROC,251, 0, p_none   ,ATT_INT32    }; //4
procobj  d_ml_cancel  ={O_SPROC,252, 4, p_h      ,0            }; //4
procobj  d_used_mem   ={O_SPROC,253, 4, p_bool   ,ATT_INT32    }; //4
procobj  d_replicate  ={O_SPROC,254,12, p_ssb    ,ATT_BOOLEAN  }; //4
procobj  d_repl_ctl   ={O_SPROC,255,12, p_iivx   ,ATT_BOOLEAN  }; //4
procobj  d_set_tr_iso ={O_SPROC,256, 4, p_vint32 ,ATT_BOOLEAN  }; //4
procobj  d_backbmp    ={O_SPROC,257, 4, p_str1   ,ATT_BOOLEAN  }; //4
procobj  d_mb_open    ={O_SPROC,258, 4, p_mbopen ,ATT_INT32    }; //4
procobj  d_mb_load    ={O_SPROC,259, 8, p_hi     ,ATT_INT32    }; //4
procobj  d_mb_getmsg  ={O_SPROC,260, 8, p_hi     ,ATT_INT32    }; //4
procobj  d_mb_getfli  ={O_SPROC,261, 8, p_hi     ,ATT_INT32    }; //4
procobj  d_mb_saveas  ={O_SPROC,262,20, p_mbsvas ,ATT_INT32    }; //4
procobj  d_mb_delmsg  ={O_SPROC,263,12, p_mbdelm ,ATT_INT32    }; //4
procobj  d_mb_getinfo ={O_SPROC,264,20, p_mbinfo ,ATT_INT32    }; //4
procobj  d_mb_gettype ={O_SPROC,265, 0, p_none   ,ATT_INT32    }; //4
procobj  d_mb_close   ={O_SPROC,266, 4, p_h      ,0            }; //4
procobj  d_mb_dial    ={O_SPROC,267, 4, p_str1   ,ATT_INT32    }; //4
procobj  d_mb_hangup  ={O_SPROC,268, 0, p_none   ,ATT_INT32    }; //4
procobj  d_ft_index   ={O_SPROC,269, 16,p_ft_index,ATT_BOOLEAN };
procobj  d_err_supp   ={O_SPROC,270, 4, p_str1   ,ATT_BOOLEAN  };
procobj  d_err_cont   ={O_SPROC,271, 24,p_irint5 ,ATT_BOOLEAN  };
procobj  d_get_errtxt ={O_SPROC,272, 12,p_isi,    ATT_BOOLEAN  };
procobj  d_srv_info   ={O_SPROC,273, 12,p_ini,    ATT_BOOLEAN  };
procobj  d_db2file    ={O_SPROC,274, 28,p_db2file,ATT_BOOLEAN  };
procobj  d_file2db3   ={O_SPROC,275, 76,p_file2db3,ATT_BOOLEAN };
procobj  d_isviewchng ={O_SPROC,276, 4, p_wind   ,ATT_BOOLEAN  };
procobj  d_bigint2str ={O_SPROC,277, 8, p_bigint ,ATT_STRING   };
procobj  d_str2bigint ={O_SPROC,278, 4, p_str1   ,ATT_INT64    };
procobj  d_objtofolder={O_SPROC,279, 16,p_objtofolder,ATT_BOOLEAN  };
procobj  d_curr_ts    ={O_SPROC,280, 0, p_none   ,ATT_TIMESTAMP}; 
procobj  d_selintrec  ={O_SPROC,281,16, p_selrec ,ATT_BOOLEAN}; 
procobj  d_selextrec  ={O_SPROC,282,16, p_selrec ,ATT_BOOLEAN}; 
procobj  d_isintsel   ={O_SPROC,283, 8, p_issel  ,ATT_BOOLEAN}; 
procobj  d_isextsel   ={O_SPROC,284, 8, p_issel  ,ATT_BOOLEAN}; 
procobj  d_viewreccnt ={O_SPROC,285, 8, p_issel  ,ATT_INT32}; 
procobj  d_unload     ={O_SPROC,286, 4, p_str1   ,ATT_BOOLEAN}; 
procobj  d_mb_getmsgx ={O_SPROC,287,12, p_hii    ,ATT_INT32};
procobj  d_ml_blobr   ={O_SPROC,288,24, p_blobr  ,ATT_INT32};
procobj  d_ml_blobs   ={O_SPROC,289,20, p_blobs  ,ATT_INT32};
procobj  d_mb_sfdbr   ={O_SPROC,290,32, p_sfdbr  ,ATT_INT32};
procobj  d_mb_sfdbs   ={O_SPROC,291,28, p_sfdbs  ,ATT_INT32};
procobj  d_appl_shrd  ={O_SPROC,292, 4, p_str1   ,ATT_BOOLEAN }; 
procobj  d_get_prop   ={O_SPROC,293,20, p_gprop  ,ATT_BOOLEAN }; 
procobj  d_set_prop   ={O_SPROC,294,16, p_sprop  ,ATT_BOOLEAN }; 
procobj  d_sqloptget  ={O_SPROC,295, 4, p_r4     ,ATT_BOOLEAN }; 
procobj  d_mp_crea    ={O_SPROC,296, 8, p_cp     ,ATT_INT32   };
procobj  d_mp_dele    ={O_SPROC,297, 4, p_str1   ,ATT_INT32   };
procobj  d_mp_set     ={O_SPROC,298,12, p_str3   ,ATT_INT32   };
procobj  d_mp_get     ={O_SPROC,299,16, p_gp     ,ATT_INT32   };
procobj  d_inv_cache  ={O_SPROC,300, 0, p_none   ,0           };  
procobj  d_am_i_config={O_SPROC,301, 0, p_none   ,ATT_BOOLEAN };  
procobj  d_askiprepl  ={O_SPROC,302, 4, p_wind,   ATT_BOOLEAN }; 
procobj  d_skiprepl   ={O_SPROC,303, 8, p_str2,   ATT_BOOLEAN }; 
procobj  d_set_cli_prop={O_SPROC,304,8, p_is,     0           }; 
procobj  d_rolledback ={O_SPROC,305, 4, p_refbool,ATT_BOOLEAN }; 
procobj  d_transopen  ={O_SPROC,306, 4, p_refbool,ATT_BOOLEAN }; 
procobj  d_get_cdp    ={O_SPROC,307, 4, p_none   ,ATT_HANDLE  };  // integer type convertible to pointer and vice versa
procobj  d_impxml     ={O_SPROC,308, 8, p_impxml ,ATT_BOOLEAN }; 
procobj  d_impxmlb    ={O_SPROC,309,12, p_impxmlb,ATT_BOOLEAN }; 
procobj  d_expxml     ={O_SPROC,310,12, p_expxml ,ATT_BOOLEAN }; 
procobj  d_expxmlb    ={O_SPROC,311,20, p_expxmlb,ATT_BOOLEAN }; 
procobj  d_trunc_tab  ={O_SPROC,312, 4, p_table  ,ATT_BOOLEAN  };
procobj  d_sql_value  ={O_SPROC,313,256,p_valstr ,ATT_STRING   };
procobj  d_exec_dir   ={O_SPROC,314, 8, p_ex_dir, ATT_BOOLEAN  };
procobj  d_ml_crew    ={O_SPROC,315,16, p_crelet ,ATT_INT32    }; //4
procobj  d_ml_addrw   ={O_SPROC,316,16, p_adrlet ,ATT_INT32    }; //4
procobj  d_ml_blobrw  ={O_SPROC,317,24, p_blobr  ,ATT_INT32};
procobj  d_ml_blobsw  ={O_SPROC,318,20, p_blobs  ,ATT_INT32};
procobj  d_ml_filew   ={O_SPROC,319, 8, p_hs     ,ATT_INT32    }; //4
procobj  d_ml_initex  ={O_SPROC,320,12, p_str3   ,ATT_INT32    }; //4

aggrobj  d_min        ={O_AGGREG, AG_TYPE_MIN };
aggrobj  d_max        ={O_AGGREG, AG_TYPE_MAX };
aggrobj  d_sum        ={O_AGGREG, AG_TYPE_SUM };
aggrobj  d_avg        ={O_AGGREG, AG_TYPE_LCN };
aggrobj  d_count      ={O_AGGREG, AG_TYPE_CNT };

#if WBVERS<900
#define STD_NUM 444
#else
#define STD_NUM 425
#endif

struct std_table 
{ objtable * next_table; /* ptr to the table on the higher level */
  unsigned objnum;
  unsigned tabsize;
  objdef objects[STD_NUM];            /* an array of object definitions */
};  

CFNC DllKernel std_table standard_table;  // on Linux must not be extern "C" and initialized in the same decl.

std_table standard_table = { NULL, STD_NUM, STD_NUM, {
{ "ABS"           , (object*)&d_abs        },
{ "ACREATE_USER"  , (object*)&d_acre_user  },
{ "ACTION_"       , (object*)&d_action     },
{ "ACTIVE_VIEW"   , (object*)&d_curr_view  },
{ "ADDR"          , (object*)&d_addr       },
{ "ADD_RECORD"    , (object*)&d_addrec     },
{ "ALOGIN"        , (object*)&d_alogin     },
{ "AMODIFY_USER"  , (object*)&d_amod_user  },
{ "AM_I_CONFIG_ADMIN",(object*)&d_am_i_config},
{ "AM_I_DB_ADMIN" , (object*)&d_amidbadmin },
{ "ANO"           , (object*)&true_cdesc   },
{ "APPEND"        , (object*)&d_append     },
{ "APPL_INST_COUNT",(object*)&d_applinstcnt},
{ "ARCTAN"        , (object*)&d_arctan     },
{ "AREPLICATE"    , (object*)&d_areplicate },
{ "ASET_PASSWORD" , (object*)&d_asetpass   },
{ "ASKIP_REPL"    , (object*)&d_askiprepl  },
{ "ATOKEN_CONTROL", (object*)&d_atoken     },
#if WBVERS<900
{ "AUTO_CURSOR"   , (object*)&acc_cdesc    },
#endif
{ "AVAILABLE_MEMORY",(object*)&d_availmem  },
{ "AVG"           , (object*)&d_avg        },
{ "BACKGROUND_BITMAP",(object*)&d_backbmp  },
{ "BIGINT"        , (object*)&att_int64    },
{ "BIGINT2STR"    , (object*)&d_bigint2str },
{ "BIND_RECORDS"  , (object*)&d_bind_recs  },
{ "BLACK"         , (object*)&black_cdesc  },
{ "BLUE"          , (object*)&blue_cdesc   },
{ "BOOLEAN"       , (object*)&att_boolean  },
{ "BROWN"         , (object*)&brown_cdesc  },
{ "CATEG_APPL"    , (object*)&ct_appl_cdesc},
{ "CATEG_CONNECTION", (object*)&ct_conn_cdesc},
{ "CATEG_CURSOR"  , (object*)&ct_curs_cdesc},
{ "CATEG_DIRCUR"  , (object*)&ct_dirc_cdesc},
{ "CATEG_DOMAIN"  , (object*)&ct_dom_cdesc },
{ "CATEG_DRAWING" , (object*)&ct_schm_cdesc},
{ "CATEG_GRAPH"   , (object*)&ct_graf_cdesc},
{ "CATEG_GROUP"   , (object*)&ct_grup_cdesc},
{ "CATEG_MENU"    , (object*)&ct_menu_cdesc},
{ "CATEG_PGMEXE"  , (object*)&ct_pgme_cdesc},
{ "CATEG_PGMSRC"  , (object*)&ct_pgms_cdesc},
{ "CATEG_PICT"    , (object*)&ct_pict_cdesc},
{ "CATEG_PROC"    , (object*)&ct_proc_cdesc},
{ "CATEG_RELATION", (object*)&ct_rela_cdesc},
{ "CATEG_REPLREL" , (object*)&ct_repl_cdesc},
{ "CATEG_ROLE"    , (object*)&ct_role_cdesc},
{ "CATEG_SEQ"     , (object*)&ct_seq_cdesc },
{ "CATEG_SERVER"  , (object*)&ct_serv_cdesc},
{ "CATEG_TABLE"   , (object*)&ct_tabl_cdesc},
{ "CATEG_TRIGGER" , (object*)&ct_trig_cdesc},
{ "CATEG_USER"    , (object*)&ct_user_cdesc},
{ "CATEG_VIEW"    , (object*)&ct_view_cdesc},
{ "CATEG_WWW"     , (object*)&ct_www_cdesc },
{ "CHAR"          , (object*)&att_char     },
{ "CHNG_COMPONENT_FLA", (object*)&d_chngcomp   },
{ "CHR"           , (object*)&d_chr        },
{ "CLOSE"         , (object*)&d_close      },
{ "CLOSEWBMAIL"   , (object*)&d_ml_close   },
{ "CLOSE_ALL_VIEWS", (object*)&d_closeallv },
{ "CLOSE_CURSOR"  , (object*)&d_close_curs },
{ "CLOSE_VIEW"    , (object*)&d_close_view },
{ "COMMIT"        , (object*)&d_commit     },
{ "COMMIT_VIEW"   , (object*)&d_commit_view},
{ "COMPACT_DATABASE",(object*)&d_compacall },
{ "COMPACT_TABLE" , (object*)&d_compactab  },
{ "CONNECTION_SPEED_T",(object*)&d_conn_spd},
{ "COS"           , (object*)&d_cos        },
{ "COUNT"         , (object*)&d_count      },
#if WBVERS<900
{ "COUNT_RECS"    , (object*)&count_cdesc  },
#endif
{ "CREATE2_LINK"  , (object*)&d_create2link},
{ "CREATE_GROUP"  , (object*)&d_creategrp  },
{ "CREATE_LINK"   , (object*)&d_createlink },
{ "CREATE_USER"   , (object*)&d_createuser },
{ "CURRENT_APPLICATIO", (object*)&d_currentapl },
{ "CURRENT_ITEM"  , (object*)&d_curr_item  },
{ "CURRENT_TIMESTAMP",(object*)&d_curr_ts  },
{ "CYAN"          , (object*)&cyan_cdesc   },
{ "C_AVG"         , (object*)&d_c_avg      },
{ "C_COUNT"       , (object*)&d_c_count    },
{ "C_MAX"         , (object*)&d_c_max      },
{ "C_MIN"         , (object*)&d_c_min      },
{ "C_SUM"         , (object*)&d_c_sum      },
{ "DARK_GRAY"     , (object*)&dark_gray_cdesc},
{ "DATABASE_INTEGRITY", (object*)&d_db_integr  },
{ "DATA_EXPORT"   , (object*)&d_data_exp   },
{ "DATA_IMPORT"   , (object*)&d_data_imp   },
{ "DATE"          , (object*)&att_date     },
{ "DATE2STR"      , (object*)&d_date2str   },
{ "DATETIME2TIMESTAMP", (object*)&d_datetime2  },
{ "DATUMOVKA"     , (object*)&att_datim    },
{ "DAY"           , (object*)&d_day        },
{ "DAY_OF_WEEK"   , (object*)&d_dayofweek  },
{ "DB2FILE"       , (object*)&d_db2file    },
{ "DEFINE_TABLE"  , (object*)&d_deftab     },
{ "DELETE"        , (object*)&d_delete     },
{ "DELETE_ALL_RECORDS", (object*)&d_delallrec  },
{ "DELETE_FILE"   , (object*)&d_delfile    },
#if WBVERS<900
{ "DEL_RECS"      , (object*)&delrecs_cdesc},
#endif
{ "DISPOSE"       , (object*)&d_dispose    },
{ "DRAW_LINE"     , (object*)&d_draw_line  },
{ "EDIT_IMPEXP"   , (object*)&d_edit_ie    },
{ "EDIT_PRIVILS"  , (object*)&d_edit_privs },
{ "EDIT_QUERY"    , (object*)&d_edit_query },
{ "EDIT_RELATION" , (object*)&d_edit_rel   },
{ "EDIT_TABLE"    , (object*)&d_edit_table },
{ "EDIT_TEXT"     , (object*)&d_edit_text  },
{ "EDIT_VIEW"     , (object*)&d_edit_view  },
{ "ENABLE_INDEX"  , (object*)&d_enableind  },
{ "ENABLE_TASK_SWITCH", (object*)&d_enablets   },
{ "EOF"           , (object*)&d_eof        },
{ "ERR_MASK"      , (object*)&d_err_mask   },
{ "EXEC"          , (object*)&d_exec       },
{ "EXP"           , (object*)&d_exp        },
{ "EXPORT_APPL_EX", (object*)&d_exp_apl_ex },
{ "EXPORT_TO_XML",(object*)&d_expxml     },
{ "EXPORT_TO_XML_BUFF",(object*)&d_expxmlb },
{ "FALSE"         , (object*)&false_cdesc  },
{ "FILE2DB3"      , (object*)&d_file2db3   },
{ "FILELENGTH"    , (object*)&d_filelength },
{ "FIND_OBJECT"   , (object*)&d_findobj    },
{ "FIND_OBJECT_BY_ID", (object*)&d_findobjbyid},
{ "FLEXINT"       , (object*)&att_int32    },
{ "FREE_DELETED"  , (object*)&d_free_del   },
{ "FROM_XBASE"    , (object*)&d_fromxbase  },
{ "FULLTEXT_INDEX_DOC",(object*)&d_ft_index },
{ "GETSET_FIL_BLOCKS",(object*)&d_filblocks},
{ "GETSET_FIL_SIZE", (object*)&d_filsize   },
{ "GETSET_GROUP_ROLE", (object*)&d_grprole },
{ "GETSET_NEXT_USER", (object*)&d_next_user},
{ "GETSET_PRIVILS", (object*)&d_privils    },
{ "GET_CURR_TASK_PTR", (object*)&d_get_cdp },
{ "GET_DATA_RIGHTS", (object*)&d_gdatari   },
{ "GET_ERROR_NUM_TEXT",(object*)&d_get_errtxt},
{ "GET_EXT_MESSAGE", (object*)&d_get_ext_msg},
{ "GET_FCURSOR"   , (object*)&d_get_fcurs  },
{ "GET_ITEM_VALUE", (object*)&d_get_i_val  },
{ "GET_LOGGED_USER",(object*)&d_get_l_user },
{ "GET_MESSAGE"   , (object*)&d_get_messg  },
{ "GET_OBJECT_RIGHTS", (object*)&d_gobjri  },
{ "GET_PROPERTY_VALUE",(object*)&d_get_prop},
{ "GET_SERVER_ERROR_C",(object*)&d_err_cont},
{ "GET_SERVER_ERROR_S",(object*)&d_err_supp}, 
{ "GET_SERVER_INFO",(object*)&d_srv_info   },
{ "GET_SQL_OPTION", (object*)&d_sqloptget  },
{ "GET_VIEW_ITEM" , (object*)&d_getitem    },
{ "GET_VIEW_POS"  , (object*)&d_get_view   },
{ "GRAY"          , (object*)&l_gray_cdesc },
{ "GREEN"         , (object*)&green_cdesc  },
{ "HELP_FILE"     , (object*)&d_help_file  },
{ "HOURS"         , (object*)&d_hours      },
{ "IABS"          , (object*)&d_iabs       },
{ "IMPORT_FROM_XML",(object*)&d_impxml     },
{ "IMPORT_FROM_XML_BU",(object*)&d_impxmlb },
{ "INFO_BOX"      , (object*)&d_infobox    },
{ "INITWBMAIL"    , (object*)&d_ml_init    },
{ "INITWBMAIL602" , (object*)&d_ml_init602 },
{ "INITWBMAILEX"  , (object*)&d_ml_initex  },
{ "INPUT_BOX"     , (object*)&d_inputbox   },
{ "INSERT"        , (object*)&d_insert     },
{ "INSERT_OBJECT" , (object*)&d_insert_obj },
{ "INT2STR"       , (object*)&d_int2str    },
{ "INTEGER"       , (object*)&att_int32    },
{ "INVALIDATE_CACHED_",(object*)&d_inv_cache},
{ "ISEXTRECSELECTED", (object*)&d_isextsel },
{ "ISINTRECSELECTED", (object*)&d_isintsel },
{ "ISQR"          , (object*)&d_isqr       },
{ "ISVIEWCHANGED",  (object*)&d_isviewchng },
{ "IS_LINK"       , (object*)&ct_link_cdesc},
{ "IS_REPL_DESTIN", (object*)&d_is_repl_d  },
{ "KEY_TABLENUM"  , (object*)&key_tablenum },
{ "LETTERADDADDR" , (object*)&d_ml_addr    },
{ "LETTERADDADDRW", (object*)&d_ml_addrw   },
{ "LETTERADDBLOBR", (object*)&d_ml_blobr   },
{ "LETTERADDBLOBRW", (object*)&d_ml_blobrw },
{ "LETTERADDBLOBS", (object*)&d_ml_blobs   },
{ "LETTERADDBLOBSW", (object*)&d_ml_blobsw },
{ "LETTERADDFILE" , (object*)&d_ml_file    },
{ "LETTERADDFILEW" , (object*)&d_ml_filew  },
{ "LETTERCANCEL"  , (object*)&d_ml_cancel  },
{ "LETTERCREATE"  , (object*)&d_ml_cre     },
{ "LETTERCREATEW" , (object*)&d_ml_crew    },
{ "LETTERSEND"    , (object*)&d_ml_send    },
{ "LIKE"          , (object*)&d_like       },
{ "LN"            , (object*)&d_ln         },
{ "LOGIN"         , (object*)&d_login      },
{ "LOGOUT"        , (object*)&d_logout     },
{ "LOG_WRITE"     , (object*)&d_log_write  },
{ "LOOK_UP"       , (object*)&d_lookup     },
{ "L_BLUE"        , (object*)&l_blue_cdesc },
{ "L_CYAN"        , (object*)&l_cyan_cdesc },
{ "L_GREEN"       , (object*)&l_green_cdesc},
{ "L_MAGENTA"     , (object*)&l_magenta_cdesc},
{ "L_RED"         , (object*)&l_red_cdesc  },
{ "L_WHITE"       , (object*)&l_white_cdesc},
{ "MAGNETA"       , (object*)&magneta_cdesc},
{ "MAILBOXDELETEMSG", (object*)&d_mb_delmsg },
{ "MAILBOXGETFILINFO", (object*)&d_mb_getfli},
{ "MAILBOXGETMSG" , (object*)&d_mb_getmsg   },
{ "MAILBOXGETMSGEX" , (object*)&d_mb_getmsgx},
{ "MAILBOXLOAD"   , (object*)&d_mb_load     },
{ "MAILBOXSAVEFILEAS", (object*)&d_mb_saveas},
{ "MAILBOXSAVEFILEDBR", (object*)&d_mb_sfdbr},
{ "MAILBOXSAVEFILEDBS", (object*)&d_mb_sfdbs},
{ "MAILCLOSEINBOX", (object*)&d_mb_close    },
{ "MAILCREATEPROFILE", (object*)&d_mp_crea  },
{ "MAILDELETEPROFILE", (object*)&d_mp_dele  },
{ "MAILDIAL"      , (object*)&d_mb_dial     },
{ "MAILGETINBOXINFO", (object*)&d_mb_getinfo},
{ "MAILGETPROFILEPROP", (object*)&d_mp_get  },
{ "MAILGETTYPE"   , (object*)&d_mb_gettype  },
{ "MAILHANGUP"    , (object*)&d_mb_hangup   },
{ "MAILOPENINBOX" , (object*)&d_mb_open     },
{ "MAILSETPROFILEPROP", (object*)&d_mp_set  },
{ "MAIN_MENU"     , (object*)&d_main_menu  },
{ "MAKE_DATE"     , (object*)&d_make_date  },
{ "MAKE_DIRECTORY", (object*)&d_mkdir      },
{ "MAKE_TIME"     , (object*)&d_make_time  },
{ "MAX"           , (object*)&d_max        },
{ "MEMCPY"        , (object*)&d_memcpy     },
{ "MESSAGE_TO_CLIENTS",(object*)&d_mess2cli},
{ "MIN"           , (object*)&d_min        },
{ "MINUTES"       , (object*)&d_minutes    },
#if WBVERS<900
{ "MODAL_VIEW"    , (object*)&modal_v_cdesc},
{ "MODELESS_VIEW" , (object*)&model_v_cdesc},
#endif
{ "MONEY"         , (object*)&att_money    },
{ "MONEY2STR"     , (object*)&d_money2str  },
{ "MONTH"         , (object*)&d_month      },
{ "MOVE_DATA"     , (object*)&d_move_data  },
{ "MOVE_OBJ_TO_FOLDER",(object*)&d_objtofolder},
{ "NE"            , (object*)&false_cdesc  },
{ "NEW"           , (object*)&d_new        },
{ "NEXT_USER_NAME", (object*)&d_next_uname },
{ "NIL"           , (object*)&nil_cdesc    },
{ "NONEBIGINT"    , (object*)&nullsig64_cdesc},
{ "NONEBOOLEAN"   , (object*)&nullbool_cdesc },
{ "NONECHAR"      , (object*)&nullchar_cdesc },
{ "NONEDATE"      , (object*)&nulldate_cdesc },
{ "NONEINTEGER"   , (object*)&nullsig32_cdesc},
{ "NONEMONEY"     , (object*)&nullmoney_cdesc},
{ "NONEPTR"       , (object*)&nullptr_cdesc  },
{ "NONEREAL"      , (object*)&nullreal_cdesc },
{ "NONESHORT"     , (object*)&nullsig16_cdesc},
{ "NONETIME"      , (object*)&nulltime_cdesc },
{ "NONETIMESTAMP" , (object*)&nullts_cdesc },
{ "NONETINY"      , (object*)&nullsig8_cdesc},
{ "NOW"           , (object*)&d_now        },
#if WBVERS<900
{ "NO_DELETE"     , (object*)&nodelete_cd  },
{ "NO_EDIT"       , (object*)&noedit_cd    },
{ "NO_INSERT"     , (object*)&noinsert_cd  },
{ "NO_MOVE"       , (object*)&nomove_cd    },
{ "NO_REDIR"      , (object*)&no_redir     },
#endif
{ "OBJ_TABLENUM"  , (object*)&obj_tablenum },
{ "ODBC_CREATE_CONNEC", (object*)&d_o_create   },
{ "ODBC_DIRECT_CONNEC", (object*)&d_o_direct   },
{ "ODBC_EXEC_DIRECT", (object*)&d_exec_dir   },
{ "ODBC_FIND_CONNECTI", (object*)&d_o_find     },
{ "ODBC_OPEN_CURSOR",(object*)&d_o_cursor   },
{ "ODD"           , (object*)&d_odd        },
{ "OPEN_CURSOR"   , (object*)&d_open_cursor},
{ "OPEN_SQL_CURSOR", (object*)&d_open_sql_c },
{ "OPEN_SQL_PARTS", (object*)&d_open_sql_p },
{ "OPEN_VIEW"     , (object*)&d_open_view  },
{ "OPER_GET"      , (object*)&oper_get_cdesc},
{ "OPER_GETEFF"   , (object*)&oper_geteff_cdesc},
{ "OPER_SET"      , (object*)&oper_set_cdesc},
{ "ORD"           , (object*)&d_ord        },
{ "OWNED_CURSORS" , (object*)&d_owncurs    },
{ "PAGE_SETUP"    , (object*)&d_pagesetup  },
#if WBVERS<900
{ "PARENT_CURSOR" , (object*)&par_c_cdesc  },
#endif
{ "PEEK_MESSAGE"  , (object*)&d_peekmess   },
{ "PICK_WINDOW"   , (object*)&d_pick       },
{ "POINTER"       , (object*)&att_nil      },
{ "PREF"          , (object*)&d_pref       },
{ "PRINTER_DIALOG", (object*)&d_printer_d  },
{ "PRINTER_SELECT", (object*)&d_printer_sel},
{ "PRINT_COPIES"  , (object*)&d_print_cpy  },
{ "PRINT_MARGINS" , (object*)&d_print_marg },
{ "PRINT_OPT"     , (object*)&d_print_opt  },
{ "PRINT_VIEW"    , (object*)&d_print_view },
{ "PUT_PIXEL"     , (object*)&d_put_pixel  },
{ "QBE_STATE"     , (object*)&d_qbe_state  },
{ "QUARTER"       , (object*)&d_quarter    },
#if WBVERS<900
{ "QUERY_VIEW"    , (object*)&query_cdesc  },
#endif
{ "READ_LOCK_RECORD", (object*)&d_rlockr     },
{ "READ_LOCK_TABLE", (object*)&d_rlockt     },
{ "READ_UNLOCK_RECORD", (object*)&d_runlockr   },
{ "READ_UNLOCK_TABLE", (object*)&d_runlockt   },
{ "REAL"          , (object*)&att_float    },
{ "REAL2STR"      , (object*)&d_real2str   },
{ "REC_CNT"       , (object*)&d_rec_cnt    },
{ "RED"           , (object*)&red_cdesc    },
{ "REGISTER_KEY"  , (object*)&d_reg_key    },
{ "REGISTER_REC_SYN", (object*)&d_reg_synchr },
{ "RELATE_RECORD" , (object*)&d_relrec     },
{ "RELIST_OBJECTS", (object*)&d_relist     },
{ "REPLICATE",      (object*)&d_replicate  },
{ "REPL_APPL_SHARED", (object*)&d_appl_shrd},
{ "REPL_CONTROL",   (object*)&d_repl_ctl   },
{ "REPL_TABLENUM" , (object*)&repl_tablenum },
{ "RESET"         , (object*)&d_reset      },
#if WBVERS<900
{ "RESET_CACHE"   , (object*)&r_cache_cdesc},
{ "RESET_COMBOS"  , (object*)&r_combo_cdesc},
{ "RESET_CONTROLS", (object*)&r_cntr_cdesc },
{ "RESET_CURSOR"  , (object*)&r_curs_cdesc },
{ "RESET_DELETIONS", (object*)&r_dels_cdesc },
{ "RESET_SYNCHRO" , (object*)&r_syn_cdesc  },
{ "RESET_VIEW"    , (object*)&d_resetview  },
{ "RESET_VISIBILITY", (object*)&r_visi_cdesc },
#endif
{ "RESTORE_CURSOR", (object*)&d_restore    },
{ "RESTORE_TABLE" , (object*)&d_resttab    },
{ "RESTRICT_CURSOR", (object*)&d_restrict   },
{ "REWRITE"       , (object*)&d_rewrite    },
{ "RIGHT_DEL"     , (object*)&ri_del       },
{ "RIGHT_GRANT"   , (object*)&ri_grant     },
{ "RIGHT_INSERT"  , (object*)&ri_insert    },
{ "RIGHT_READ"    , (object*)&ri_read      },
{ "RIGHT_WRITE"   , (object*)&ri_write     },
{ "ROLLED_BACK"   , (object*)&d_rolledback },
{ "ROLL_BACK"     , (object*)&d_roll_back  },
{ "ROLL_BACK_VIEW", (object*)&d_roll_back_v},
{ "ROUND"         , (object*)&d_round      },
{ "SAVE_TABLE"    , (object*)&d_savetab    },
{ "SEC1000"       , (object*)&d_sec1000    },
{ "SECONDS"       , (object*)&d_seconds    },
{ "SEEK"          , (object*)&d_seek       },
{ "SELECTEXTREC"  , (object*)&d_selextrec   }, 
{ "SELECTINTREC"  , (object*)&d_selintrec   },
{ "SELECT_DIRECTORY", (object*)&d_sel_dir    },
{ "SELECT_FILE"   , (object*)&d_sel_file   },
{ "SELECT_RECORDS", (object*)&d_select_recs},
{ "SEND_MESSAGE"  , (object*)&d_send_mess  },
{ "SET_APPL_STARTER", (object*)&d_set_starter },
{ "SET_CLIENT_PROPERT", (object*)&d_set_cli_prop },
{ "SET_CURSOR"    , (object*)&d_set_cursor },
{ "SET_DATA_RIGHTS", (object*)&d_sdatari    },
{ "SET_EXT_POS"   , (object*)&d_set_epos   },
{ "SET_FCURSOR"   , (object*)&d_set_fcurs  },
{ "SET_FIRST_LABEL", (object*)&d_firstlab   },
{ "SET_INT_POS"   , (object*)&d_set_ipos   },
{ "SET_ITEM_VALUE", (object*)&d_set_i_val  },
{ "SET_OBJECT_RIGHTS", (object*)&d_sobjri     },
{ "SET_PASSWORD"  , (object*)&d_set_pass   },
{ "SET_PRINTER"   , (object*)&d_set_printe },
{ "SET_PROGRESS_REPOR",(object*)&d_set_progres },
{ "SET_PROPERTY_VALUE",(object*)&d_set_prop },
{ "SET_SQL_OPTION", (object*)&d_sqlopt     },
{ "SET_STATUS_NUMS",(object*)&d_set_st_nums},
{ "SET_STATUS_TEXT",(object*)&d_set_st_text},
{ "SET_TIMER"     , (object*)&d_set_timer  },
{ "SET_TRANSACTION_IS", (object*)&d_set_tr_iso},
{ "SET_VIEW_ITEM" , (object*)&d_setitem    },
{ "SHORT"         , (object*)&att_int16    },
{ "SHOW_HELP"     , (object*)&d_show_help  },
{ "SHOW_HELP_POPUP",(object*)&d_show_helpp },
{ "SIG16"         , (object*)&att_int16    },
{ "SIG32"         , (object*)&att_int32    },
{ "SIGNALIZE"     , (object*)&d_signalize  },
{ "SIGNATURE_STATE",(object*)&d_sigstate   },
{ "SIN"           , (object*)&d_sin        },
{ "SKIP_REPL"     , (object*)&d_skiprepl   },
{ "SQL_DROP"      , (object*)&d_sqldrop    },
{ "SQL_EXECUTE"   , (object*)&d_sqlexec    },
{ "SQL_EXEC_PREPARED", (object*)&d_sqlexecprep},
{ "SQL_PREPARE"   , (object*)&d_sqlprepare },
{ "SQL_VALUE"     , (object*)&d_sql_value  },
{ "SQR"           , (object*)&d_sqr        },
{ "SQRT"          , (object*)&d_sqrt       },
{ "SRV_TABLENUM"  , (object*)&srv_tablenum },
{ "START_TRANSACTION", (object*)&d_start_tra  },
{ "STR2BIGINT"    , (object*)&d_str2bigint },
{ "STR2DATE"      , (object*)&d_str2date   },
{ "STR2INT"       , (object*)&d_str2int    },
{ "STR2MONEY"     , (object*)&d_str2money  },
{ "STR2REAL"      , (object*)&d_str2real   },
{ "STR2TIME"      , (object*)&d_str2time   },
{ "STR2TIMESTAMP" , (object*)&d_str2timest },
{ "STRCAT"        , (object*)&d_strcat     },
{ "STRCOPY"       , (object*)&d_strcopy    },
{ "STRDELETE"     , (object*)&d_strdelete  },
{ "STRINSERT"     , (object*)&d_strinsert  },
{ "STRLENGHT"     , (object*)&d_strlenght  },
{ "STRLENGTH"     , (object*)&d_strlenght  },
{ "STRPOS"        , (object*)&d_strpos     },
{ "STRTRIM"       , (object*)&d_strtrim    },
{ "SUBSTR"        , (object*)&d_substr     },
{ "SUM"           , (object*)&d_sum        },
{ "SUPER_RECNUM"  , (object*)&d_superrec   },
{ "SZ_ERROR"      , (object*)&d_sz_error   },
{ "SZ_WARNING"    , (object*)&d_sz_warning },
{ "TAB_PAGE"      , (object*)&d_tab_page   },
{ "TAB_TABLENUM"  , (object*)&tab_tablenum },
{ "TAKEMAILTOREMOFFIC",(object*)&d_ml_take },
{ "TCATEG"        , (object*)&att_int16    },
{ "TCURSNUM"      , (object*)&att_int32    },
{ "TCURSTAB"      , (object*)&att_int32    },
{ "TEXT_SUBSTRING", (object*)&d_textsubstr },
{ "TIME"          , (object*)&att_time     },
{ "TIME2STR"      , (object*)&d_time2str   },
{ "TIMESTAMP"     , (object*)&att_timestamp},
{ "TIMESTAMP2DATE", (object*)&d_dtm2dt     },
{ "TIMESTAMP2STR" , (object*)&d_timest2str },
{ "TIMESTAMP2TIME", (object*)&d_dtm2tm     },
{ "TINYINT"       , (object*)&att_int8     },
{ "TOBJNUM"       , (object*)&att_int32    },
{ "TODAY"         , (object*)&d_today      },
{ "TOKEN_CONTROL" , (object*)&d_tokenctrl  },
{ "TO_XBASE"      , (object*)&d_toxbase    },
{ "TRANSACTION_OPEN",(object*)&d_transopen },
{ "TRANSLATE"     , (object*)&d_translate  },
{ "TRECNUM"       , (object*)&att_int32    },
{ "TRUE"          , (object*)&true_cdesc   },
{ "TRUNC"         , (object*)&d_trunc      },
{ "TRUNCATE_TABLE", (object*)&d_trunc_tab  },
{ "TTABLENUM"     , (object*)&att_int32    },
{ "UNDELETE"      , (object*)&d_undelete   },
{ "UNINST_TABLE"  , (object*)&d_uninst     },
{ "UNLOAD_LIBRARY", (object*)&d_unload     },
{ "UNTYPED"       , (object*)&att_untyped  },
{ "UPCASE"        , (object*)&d_upcase     },
{ "USED_MEMORY"   , (object*)&d_used_mem   },
{ "USER_IN_GROUP" , (object*)&d_useringrp  },
{ "USER_TABLENUM" , (object*)&user_tablenum},
{ "USER_TO_GROUP" , (object*)&d_usertogrp  },
{ "VIEWRECCOUNT"  , (object*)&d_viewreccnt },
{ "VIEW_CLOSE"    , (object*)&d_viewclose  },
{ "VIEW_OPEN"     , (object*)&d_viewopen   },
{ "VIEW_PATTERN"  , (object*)&d_viewpatt   },
{ "VIEW_PRINT"    , (object*)&d_viewprint  },
{ "VIEW_STATE"    , (object*)&d_viewstate  },
{ "VIEW_TOGGLE"   , (object*)&d_viewtoggle },
{ "VT_NAME"       , (object*)&vt_name_cdesc},
{ "VT_OBJNUM"     , (object*)&vt_objnum_cdesc},
{ "VT_UUID"       , (object*)&vt_uuid_cdesc},
{ "WAITING"       , (object*)&d_waiting    },
{ "WAITS_FOR_ME"  , (object*)&d_waits_fm   },
{ "WHO_AM_I"      , (object*)&d_user_name  },
{ "WINBASE602_VERSION", (object*)&d_WB_version },
{ "WINDOW_ID"     , (object*)&att_int32    },
{ "WRITE_LOCK_RECORD", (object*)&d_wlockr  },
{ "WRITE_LOCK_TABLE", (object*)&d_wlockt   },
{ "WRITE_UNLOCK_RECOR", (object*)&d_wunlockr   },
{ "WRITE_UNLOCK_TABLE", (object*)&d_wunlockt   },
{ "YEAR"          , (object*)&d_year       },
{ "YELLOW"        , (object*)&yellow_cdesc },
{ "YESNO_BOX"     , (object*)&d_yesnobox   }
} };

/////////////////////////////// view object ////////////////////////////////////
spar p_font[]    =  { { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };

spar p_srec[]    =  { { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
spar p_isel[]    =  { { O_VALPAR, ATT_INT32  },
                      { 0, 0 } };
spar p_rcnt[]    =  { { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };

static const propobj
  vp_caption     = {O_PROPERTY, 0, ATT_STRING  },
  vp_x           = {O_PROPERTY, 2, ATT_INT32   }, 
  vp_y           = {O_PROPERTY, 4, ATT_INT32   }, 
  vp_width       = {O_PROPERTY, 6, ATT_INT32   }, 
  vp_height      = {O_PROPERTY, 8, ATT_INT32   }, 
  vp_visible     = {O_PROPERTY,10, ATT_BOOLEAN },
  vp_curpos      = {O_PROPERTY,38, ATT_INT32   },
  vp_curextrec   = {O_PROPERTY,40, ATT_INT32   },
  vp_curitem     = {O_PROPERTY,42, ATT_INT32   },
  vp_cursnum     = {O_PROPERTY,44, ATT_CURSOR  },
  vp_editable    = {O_PROPERTY,46, ATT_BOOLEAN },
  vp_movable     = {O_PROPERTY,48, ATT_BOOLEAN },
  vp_deletable   = {O_PROPERTY,50, ATT_INT32   },
  vp_insertable  = {O_PROPERTY,52, ATT_BOOLEAN },
  vp_backcolor   = {O_PROPERTY,99, ATT_INT32   };

static const methobj
  vm_pick        = {O_METHOD,  12, 0, p_none,  0           , NULLSTRING}, 
  vm_is_open     = {O_METHOD,  13, 0, p_none,  ATT_BOOLEAN , NULLSTRING}, // number used in interp.cpp
  vm_commit      = {O_METHOD,  14, 8, p_wbb+1, ATT_BOOLEAN , "CanAsk\0ReportError" }, 
  vm_rollback    = {O_METHOD,  15, 0, p_none,  0           , NULLSTRING}, 
  vm_firstrec    = {O_METHOD,  16, 0, p_none,  0           , NULLSTRING}, 
  vm_lastrec     = {O_METHOD,  17, 0, p_none,  0           , NULLSTRING}, 
  vm_nextrec     = {O_METHOD,  18, 0, p_none,  0           , NULLSTRING}, 
  vm_prevrec     = {O_METHOD,  19, 0, p_none,  0           , NULLSTRING}, 
  vm_nextpage    = {O_METHOD,  20, 0, p_none,  0           , NULLSTRING}, 
  vm_prevpage    = {O_METHOD,  21, 0, p_none,  0           , NULLSTRING}, 
  vm_firstitem   = {O_METHOD,  22, 0, p_none,  0           , NULLSTRING}, 
  vm_lastitem    = {O_METHOD,  23, 0, p_none,  0           , NULLSTRING}, 
  vm_nexttab     = {O_METHOD,  24, 0, p_none,  0           , NULLSTRING}, 
  vm_prevtab     = {O_METHOD,  25, 0, p_none,  0           , NULLSTRING}, 
  vm_upitem      = {O_METHOD,  26, 0, p_none,  0           , NULLSTRING}, 
  vm_downitem    = {O_METHOD,  27, 0, p_none,  0           , NULLSTRING}, 
  vm_open_qbe    = {O_METHOD,  28, 0, p_none,  0           , NULLSTRING}, 
  vm_open_sort   = {O_METHOD,  29, 0, p_none,  0           , NULLSTRING}, 
  vm_accept_query= {O_METHOD,  30, 0, p_none,  0           , NULLSTRING}, 
  vm_cancel_query= {O_METHOD,  31, 0, p_none,  0           , NULLSTRING}, 
  vm_qbe_state   = {O_METHOD,  32, 0, p_none,  ATT_INT32   , NULLSTRING}, 
  vm_insert      = {O_METHOD,  33, 0, p_none,  0           , NULLSTRING}, 
  vm_del_rec     = {O_METHOD,  34, 0, p_none,  0           , NULLSTRING}, 
  vm_del_ask     = {O_METHOD,  35, 0, p_none,  0           , NULLSTRING}, 
  vm_print       = {O_METHOD,  36, 0, p_none,  0           , NULLSTRING}, 
  vm_help        = {O_METHOD,  37, 0, p_none,  0           , NULLSTRING},
  vm_parinst     = {O_METHOD,PARINST_METHOD_NUMBER,0,p_none,ATT_INT32, NULLSTRING},
  vm_reset       = {O_METHOD,  58, 8, p_iw44+1,0           , "RecordNumber\0Extent"},
  vm_open        = {O_METHOD,  61, 4, p_rwnd,  ATT_BOOLEAN , "OutFormId"},
  vm_close       = {O_METHOD,  62, 0, p_none,  0           , NULLSTRING},
  vm_set_source  = {O_METHOD,  84, 4, p_str1,  ATT_BOOLEAN , "SourceText"},
  vm_handle      = {O_METHOD,  86, 0, p_none,  ATT_INT32   , NULLSTRING},
  vm_attach_tb   = {O_METHOD,  94, 4, p_str1,  ATT_BOOLEAN , "FormName"},
  vm_setfont     = {O_METHOD, 102,28, p_font,  ATT_BOOLEAN,  "FaceName\0Height\0CharSet\0Bold\0Italic\0Underline\0Strikeout"},
  vm_isviewchng  = {O_METHOD, 103, 0, p_none,  ATT_BOOLEAN , NULLSTRING},
  vm_selintrec   = {O_METHOD, 104,12, p_srec,  ATT_BOOLEAN , "RecordNumber\0Select\0Redraw"},
  vm_selextrec   = {O_METHOD, 105,12, p_srec,  ATT_BOOLEAN , "RecordNumber\0Select\0Redraw"},
  vm_isintsel    = {O_METHOD, 106, 4, p_isel,  ATT_BOOLEAN , "RecordNumber"},
  vm_isextsel    = {O_METHOD, 107, 4, p_isel,  ATT_BOOLEAN , "RecordNumber"},
  vm_reccount    = {O_METHOD, 108, 4, p_rcnt,  ATT_INT32   , "WithFictiveRecord"};

#define VIEW_STD_NUM 56

struct view_std_table 
{ objtable * next_table; /* ptr to the table on the higher level */
  unsigned objnum;
  unsigned tabsize;
  objdef objects[VIEW_STD_NUM];   /* an array of object definitions */
};

#ifdef CLIENT_GUI
  CFNC DllKernel 
#else
  static 
#endif
    view_std_table view_standard_table = { NULL, VIEW_STD_NUM, VIEW_STD_NUM, {
{ "ACCEPTQUERY"        , (object*)&vm_accept_query},
{ "ATTACH_TOOLBAR"     , (object*)&vm_attach_tb   },
{ "BACKCOLOR"          , (object*)&vp_backcolor   },
{ "CANCELQUERY"        , (object*)&vm_cancel_query},
{ "CAPTION"            , (object*)&vp_caption     },
{ "CLOSE"              , (object*)&vm_close       },
{ "COMMIT_VIEW"        , (object*)&vm_commit      },
{ "CUREXTREC"          , (object*)&vp_curextrec   },
{ "CURITEM"            , (object*)&vp_curitem     },
{ "CURPOS"             , (object*)&vp_curpos      },
{ "CURSNUM"            , (object*)&vp_cursnum     },
{ "DELASK"             , (object*)&vm_del_ask     },
{ "DELETABLE"          , (object*)&vp_deletable   },
{ "DELREC"             , (object*)&vm_del_rec     },
{ "DOWNITEM"           , (object*)&vm_downitem    },
{ "EDITABLE"           , (object*)&vp_editable    },
{ "FIRSTITEM"          , (object*)&vm_firstitem   },
{ "FIRSTREC"           , (object*)&vm_firstrec    },
{ "HANDLE"             , (object*)&vm_handle      },
{ "HEIGHT"             , (object*)&vp_height      },
{ "HELP"               , (object*)&vm_help        },
{ "INSERT"             , (object*)&vm_insert      },
{ "INSERTABLE"         , (object*)&vp_insertable  },
{ "ISEXTRECSELECTED"   , (object*)&vm_isextsel    },
{ "ISINTRECSELECTED"   , (object*)&vm_isintsel    },
{ "ISVIEWCHANGED"      , (object*)&vm_isviewchng  },
{ "IS_OPEN"            , (object*)&vm_is_open     },
{ "LASTITEM"           , (object*)&vm_lastitem    },
{ "LASTREC"            , (object*)&vm_lastrec     },
{ "MOVABLE"            , (object*)&vp_movable     },
{ "NEXTPAGE"           , (object*)&vm_nextpage    },
{ "NEXTREC"            , (object*)&vm_nextrec     },
{ "NEXTTAB"            , (object*)&vm_nexttab     },
{ "OPEN"               , (object*)&vm_open        },
{ "OPENQBE"            , (object*)&vm_open_qbe    },
{ "OPENSORT"           , (object*)&vm_open_sort   },
{ "PARENT"             , (object*)&vm_parinst     },
{ "PARINST"            , (object*)&vm_parinst     },  // old name used by factor designer
{ "PICK_WINDOW"        , (object*)&vm_pick        },
{ "PREVPAGE"           , (object*)&vm_prevpage    },
{ "PREVREC"            , (object*)&vm_prevrec     },
{ "PREVTAB"            , (object*)&vm_prevtab     },
{ "PRINT"              , (object*)&vm_print       },
{ "QBE_STATE"          , (object*)&vm_qbe_state   },
{ "RECCOUNT"           , (object*)&vm_reccount    },
{ "RESET_VIEW"         , (object*)&vm_reset       },
{ "ROLL_BACK_VIEW"     , (object*)&vm_rollback    },
{ "SELECTEXTREC"       , (object*)&vm_selextrec   }, 
{ "SELECTINTREC"       , (object*)&vm_selintrec   },
{ "SET_FONT"           , (object*)&vm_setfont     },
{ "SET_SOURCE"         , (object*)&vm_set_source  },
{ "UPITEM"             , (object*)&vm_upitem      },
{ "VISIBLE"            , (object*)&vp_visible     },
{ "WIDTH"              , (object*)&vp_width       },
{ "X"                  , (object*)&vp_x           },
{ "Y"                  , (object*)&vp_y           }
} };

///////////////////////////// control methods ////////////////////////////////
spar p_strb[]    =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };

static const propobj 
  vcp_text       = {O_PROPERTY, 54, ATT_STRING  },
  vcp_value      = {O_PROPERTY, CONTROL_VALUE_PROPNUM, ATT_NIL },
  vcp_precision  = {O_PROPERTY, 64, ATT_INT32   },
  vcp_checked    = {O_PROPERTY, 66, ATT_BOOLEAN },
  vcp_format     = {O_PROPERTY, 68, ATT_INT32   },
  vcp_page       = {O_PROPERTY, 70, ATT_INT32   },
  vcp_length     = {O_PROPERTY, VALUE_LENGTH_PROPNUM, ATT_INT32 },
  vcp_selindex   = {O_PROPERTY, 82, ATT_INT32   },
  vcp_backcolor  = {O_PROPERTY, 95, ATT_INT32   },
  vcp_textcolor  = {O_PROPERTY, 97, ATT_INT32   };
static const methobj
  vcm_subinst    = {O_METHOD, SUBINST_METHOD_NUMBER, 
                                  0, p_none,  ATT_INT32, NULLSTRING },
  vcm_isnull     = {O_METHOD, 72, 0, p_none,  ATT_BOOLEAN, NULLSTRING},
  vcm_open       = {O_METHOD, 73, 0, p_none,  ATT_BOOLEAN, NULLSTRING},
  vcm_readfile   = {O_METHOD, 74, 4, p_str1,  ATT_BOOLEAN, "FileName"},
  vcm_resetcombo = {O_METHOD, 75, 4, p_str1,  ATT_BOOLEAN, "SQLQuery"},
  vcm_setnull    = {O_METHOD, 76, 0, p_none,  0          , NULLSTRING},
  vcm_writefile  = {O_METHOD, 77, 8, p_strb,  ATT_BOOLEAN, "FileName\0Append" },
  vcm_calendar   = {O_METHOD, 85, 12,p_bbb,   0          , "ShowToday\0CircleToday\0WeekNumbers"},
  vcm_handle     = {O_METHOD, 87, 0, p_none,  ATT_INT32  , NULLSTRING},
  vcm_setfocus   = {O_METHOD, 88, 0, p_none,  0          , NULLSTRING},
  vcm_selection  = {O_METHOD, 89, 8, p_vlng_2,0          , "StartPos\0StopPos"},
  vcm_sigcheck   = {O_METHOD, 92, 0, p_none,  0          , NULLSTRING},
  vcm_sigsign    = {O_METHOD, 93, 0, p_none,  0          , NULLSTRING},
  vcm_sigstate   = {O_METHOD, 91, 0, p_none,  ATT_INT32  , NULLSTRING},
  vcm_refresh    = {O_METHOD, 90, 0, p_none,  0          , NULLSTRING},
  vcm_setfont    = {O_METHOD,101,28, p_font,  ATT_BOOLEAN, "FaceName\0Height\0CharSet\0Bold\0Italic\0Underline\0Strikeout"};


#define CONTROL_STD_NUM 25

struct control_std_table 
{ objtable * next_table; /* ptr to the table on the higher level */
  unsigned objnum;
  unsigned tabsize;
  objdef   objects[CONTROL_STD_NUM];   /* an array of object definitions */
};

#ifdef CLIENT_GUI
  CFNC DllKernel 
#else
  static 
#endif
    control_std_table control_standard_table = { NULL, CONTROL_STD_NUM, CONTROL_STD_NUM, {
{ "BACKCOLOR"          , (object*)&vcp_backcolor  },
{ "CALENDAR"           , (object*)&vcm_calendar   },
{ "CHECKED"            , (object*)&vcp_checked    }, // check & radio buttons
{ "FORMAT"             , (object*)&vcp_format     },
{ "HANDLE"             , (object*)&vcm_handle     },
{ "ISNULL"             , (object*)&vcm_isnull     },
{ "LENGTH"             , (object*)&vcp_length     },
{ "OPEN"               , (object*)&vcm_open       },
{ "PAGE"               , (object*)&vcp_page       }, // tab control
{ "PRECISION"          , (object*)&vcp_precision  },
{ "READFILE"           , (object*)&vcm_readfile   },
{ "REFRESH"            , (object*)&vcm_refresh    },
{ "RESETCOMBO"         , (object*)&vcm_resetcombo }, // combo
{ "SELECTION"          , (object*)&vcm_selection  },
{ "SELINDEX"           , (object*)&vcp_selindex   }, // list box
{ "SETFOCUS"           , (object*)&vcm_setfocus   },
{ "SETNULL"            , (object*)&vcm_setnull    },
{ "SET_FONT"           , (object*)&vcm_setfont    },
{ "SIGNATURE_CHECK"    , (object*)&vcm_sigcheck   }, //sig
{ "SIGNATURE_SIGN"     , (object*)&vcm_sigsign    }, //sig
{ "SIGNATURE_STATE"    , (object*)&vcm_sigstate   }, //sig
{ "TEXT"               , (object*)&vcp_text       },
{ "TEXTCOLOR"          , (object*)&vcp_textcolor  },
{ "VALUE"              , (object*)&vcp_value      },
{ "WRITEFILE"          , (object*)&vcm_writefile  }
} };

