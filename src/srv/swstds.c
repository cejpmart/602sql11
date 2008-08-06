/*************************** standard objects *******************************/

constobj
   false_cdesc = { OBT_CONST, {  0 }, ATT_BOOLEAN },
    true_cdesc = { OBT_CONST, {   1 }, ATT_BOOLEAN },
 unknown_cdesc = { OBT_CONST, {0x80 }, ATT_BOOLEAN },
     nil_cdesc = { OBT_CONST, {   0 }, ATT_NIL     },

 nullchar_cdesc= { OBT_CONST, {  0 }, ATT_CHAR    },
 nullbool_cdesc= { OBT_CONST, {128 }, ATT_BOOLEAN },
 nullsig16_cdesc={ OBT_CONST, {  0 }, ATT_INT16   },
 nullsig32_cdesc={ OBT_CONST, {  0 }, ATT_INT32   },
 nullmoney_cdesc={ OBT_CONST, {  0 }, ATT_MONEY   },
 nulldate_cdesc= { OBT_CONST, {  0 }, ATT_DATE    },
 nulltime_cdesc= { OBT_CONST, {  0 }, ATT_TIME    },
 nullts_cdesc  = { OBT_CONST, {  0 }, ATT_TIMESTAMP},
 nullreal_cdesc= { OBT_CONST, {  0 }, ATT_FLOAT   },
 nullptr_cdesc = { OBT_CONST, { -1 }, ATT_PTR     },

 ct_appl_cdesc = { OBT_CONST, {  CATEG_APPL },     ATT_INT16   },
 ct_curs_cdesc = { OBT_CONST, {  CATEG_CURSOR },   ATT_INT16   },
 ct_menu_cdesc = { OBT_CONST, {  CATEG_MENU },     ATT_INT16   },
 ct_pgms_cdesc = { OBT_CONST, {  CATEG_PGMSRC },   ATT_INT16   },
 ct_pgme_cdesc = { OBT_CONST, {  CATEG_PGMEXE },   ATT_INT16   },
 ct_tabl_cdesc = { OBT_CONST, {  CATEG_TABLE },    ATT_INT16   },
 ct_user_cdesc = { OBT_CONST, {  CATEG_USER },     ATT_INT16   },
 ct_view_cdesc = { OBT_CONST, {  CATEG_VIEW },     ATT_INT16   },
 ct_pict_cdesc = { OBT_CONST, {  CATEG_PICT },     ATT_INT16   },
 ct_link_cdesc = { OBT_CONST, {  IS_LINK },        ATT_INT16   },
 ct_dirc_cdesc = { OBT_CONST, {  CATEG_DIRCUR },   ATT_INT16   },
 ct_grup_cdesc = { OBT_CONST, {  CATEG_GROUP },    ATT_INT16   },
 ct_role_cdesc = { OBT_CONST, {  CATEG_ROLE },     ATT_INT16   },
 ct_conn_cdesc = { OBT_CONST, {  CATEG_CONNECTION },ATT_INT16  },
 ct_rela_cdesc = { OBT_CONST, {  CATEG_RELATION }, ATT_INT16   },
 ct_graf_cdesc = { OBT_CONST, {  CATEG_GRAPH },    ATT_INT16   },
 ct_schm_cdesc = { OBT_CONST, {  CATEG_DRAWING },  ATT_INT16   },
 ct_proc_cdesc = { OBT_CONST, {  CATEG_PROC },     ATT_INT16   },
 ct_trig_cdesc = { OBT_CONST, {  CATEG_TRIGGER },  ATT_INT16   },
 ct_www_cdesc  = { OBT_CONST, {  CATEG_WWW },      ATT_INT16   },
 ct_seq_cdesc  = { OBT_CONST, {  CATEG_SEQ },      ATT_INT16   },
 ct_serv_cdesc = { OBT_CONST, {  CATEG_SERVER },   ATT_INT16   },
 ct_repl_cdesc = { OBT_CONST, {  CATEG_REPLREL },  ATT_INT16   },
 ct_fold_cdesc = { OBT_CONST, {  CATEG_FOLDER },   ATT_INT16   },
 ct_info_cdesc = { OBT_CONST, {  CATEG_INFO },     ATT_INT16   },
 ct_dom_cdesc  = { OBT_CONST, {  CATEG_DOMAIN },   ATT_INT16   },
 ct_key_cdesc  = { OBT_CONST, {  CATEG_KEY },      ATT_INT16   },
 ct_xml_cdesc  = { OBT_CONST, {  CATEG_XMLFORM },  ATT_INT16   },

 ri_del        = { OBT_CONST, {  RIGHT_DEL },      ATT_INT16   },
 ri_grant      = { OBT_CONST, {  RIGHT_GRANT },    ATT_INT16   },
 ri_insert     = { OBT_CONST, {  RIGHT_INSERT },   ATT_INT16   },
 ri_read       = { OBT_CONST, {  RIGHT_READ },     ATT_INT16   },
 ri_write      = { OBT_CONST, {  RIGHT_WRITE },    ATT_INT16   },

  tr_ue_cdesc= { OBT_CONST, {  TRACE_USER_ERROR },     ATT_INT32   },
  tr_sf_cdesc= { OBT_CONST, {  TRACE_SERVER_FAILURE }, ATT_INT32   },
  tr_ng_cdesc= { OBT_CONST, {  TRACE_NETWORK_GLOBAL }, ATT_INT32   },
  tr_rp_cdesc= { OBT_CONST, {  TRACE_REPLICATION },    ATT_INT32   },
  tr_ip_cdesc= { OBT_CONST, {  TRACE_DIRECT_IP },      ATT_INT32   },
  tr_um_cdesc= { OBT_CONST, {  TRACE_USER_MAIL },      ATT_INT32   },
  tr_sq_cdesc= { OBT_CONST, {  TRACE_SQL },            ATT_INT32   },
  tr_lg_cdesc= { OBT_CONST, {  TRACE_LOGIN },          ATT_INT32   },
  tr_lw_cdesc= { OBT_CONST, {  TRACE_LOG_WRITE },      ATT_INT32   },
  tr_ir_cdesc= { OBT_CONST, {  TRACE_IMPL_ROLLBACK },  ATT_INT32   },
  tr_ne_cdesc= { OBT_CONST, {  TRACE_NETWORK_ERROR },  ATT_INT32   },
  tr_rm_cdesc= { OBT_CONST, {  TRACE_REPLIC_MAIL },    ATT_INT32   },
  tr_rc_cdesc= { OBT_CONST, {  TRACE_REPLIC_COPY },    ATT_INT32   },
  tr_ss_cdesc= { OBT_CONST, {  TRACE_START_STOP },     ATT_INT32   },
  tr_rf_cdesc= { OBT_CONST, {  TRACE_REPL_CONFLICT },  ATT_INT32   },
  tr_si_cdesc= { OBT_CONST, {  TRACE_SERVER_INFO },    ATT_INT32   },
  tr_rd_cdesc= { OBT_CONST, {  TRACE_READ },           ATT_INT32   },
  tr_wr_cdesc= { OBT_CONST, {  TRACE_WRITE },          ATT_INT32   },
  tr_in_cdesc= { OBT_CONST, {  TRACE_INSERT },         ATT_INT32   },
  tr_de_cdesc= { OBT_CONST, {  TRACE_DELETE },         ATT_INT32   },
  tr_cu_cdesc= { OBT_CONST, {  TRACE_CURSOR },         ATT_INT32   },
  tr_bc_cdesc= { OBT_CONST, {  TRACE_BCK_OBJ_ERROR },  ATT_INT32   },
  tr_pc_cdesc= { OBT_CONST, {  TRACE_PROCEDURE_CALL }, ATT_INT32   },
  tr_te_cdesc= { OBT_CONST, {  TRACE_TRIGGER_EXEC },   ATT_INT32   },
  tr_uw_cdesc= { OBT_CONST, {  TRACE_USER_WARNING },   ATT_INT32   },
  tr_ps_cdesc= { OBT_CONST, {  TRACE_PIECES},          ATT_INT32   },
  tr_lo_cdesc= { OBT_CONST, {  TRACE_LOCK_ERROR},      ATT_INT32   },
  tr_ww_cdesc= { OBT_CONST, {  TRACE_WEB_REQUEST},     ATT_INT32   },
  tr_ce_cdesc= { OBT_CONST, {  TRACE_CONVERTOR_ERROR}, ATT_INT32   }
;


sspar p_vint4[]  =  { { O_VALPAR, ATT_INT32},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
#define p_vint3 (p_vint4 + 1)

sspar p_date[]    =  { { O_VALPAR, ATT_DATE  },
                      { 0, 0 } };
#define p_none   (p_date + 1)

sspar p_time[]    =  { { O_VALPAR, ATT_TIME  },
                      { 0, 0 } };
sspar p_real[]    =  { { O_VALPAR, ATT_FLOAT },
                      { 0, 0 } };
sspar p_char[]    =  { { O_VALPAR, ATT_CHAR  },
                      { 0, 0 } };
sspar p_vint32[]  =  { { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_str1[]    =  { { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
sspar p_memcpy[]  =  { { O_REFPAR, ATT_NIL   },
                      { O_REFPAR, ATT_NIL   },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_monint[]  =  { { O_VALPAR, ATT_MONEY },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_relint[]  =  { { O_VALPAR, ATT_FLOAT },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_str2[]    =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
sspar p_sii[]     =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_vsii[]    =  { { O_VALPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_ssi[]     =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_datei[]   =  { { O_VALPAR, ATT_DATE  },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_timei[]   =  { { O_VALPAR, ATT_TIME  },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_docflow[] =  { { O_VALPAR, ATT_BINARY},
                      { 0, 0 } };
sspar p_dattim[]  =  { { O_VALPAR, ATT_DATE  },
                      { O_VALPAR, ATT_TIME  },
                      { 0, 0 } };
sspar p_timest[]  =  { { O_VALPAR, ATT_TIMESTAMP},
                      { 0, 0 } };
sspar p_timesti[] =  { { O_VALPAR, ATT_TIMESTAMP},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_binary[]  =  { { O_VALPAR, ATT_BINARY},
                      { 0, 0 } };
sspar p_vi322[]   =  { { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_hi[]     =  { { O_VALPAR, ATT_HANDLE},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_hii[]   =   { { O_VALPAR, ATT_HANDLE},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_ssb[]     = { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_BOOLEAN },
                      { 0, 0 } };
sspar p_hs[]      = { { O_VALPAR, ATT_HANDLE },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
sspar p_crelet[]  = { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_TEXT  },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_HANDLE },
                      { 0, 0 } };
sspar p_adrlet[]  = { { O_VALPAR, ATT_HANDLE },
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
sspar p_str3[]    = { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
sspar p_h[]       = { { O_REFPAR, ATT_HANDLE},
                      { 0, 0 } };
sspar p_mbsfas[]  = { { O_VALPAR, ATT_HANDLE },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
sspar p_mbdelm[]  = { { O_VALPAR, ATT_HANDLE },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
sspar p_mbinfo[]  = { { O_VALPAR, ATT_HANDLE },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_objnum },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_objnum },
                      { 0, 0 } };
sspar p_si[]      = { { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { 0, 0 } };
sspar p_trace[]   = { { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { 0, 0 } };
sspar p_ft_index[]= { { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_TEXT   },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { 0, 0 } };
sspar p_text[]   =  { { O_REFPAR, ATT_TEXT   },
                      { 0, 0 } };
sspar p_vint64[] =  { { O_VALPAR, ATT_INT64  },
                      { 0, 0 } };
sspar p_handle[] =  { { O_VALPAR, ATT_HANDLE },
                      { 0, 0 } };
sspar p_memb[]   =  { { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
sspar p_ft_ct[]  =  { { O_REFPAR, ATT_TEXT   },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { 0, 0 } };
sspar p_ft_ct2[] =  { { O_REFPAR, ATT_TEXT   },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_TEXT   },
                      { 0, 0 } };
sspar p_ft_ct9[] =  { { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_TEXT   },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { 0, 0 } };
sspar p_blobr[] =   { { O_VALPAR, ATT_HANDLE  },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT16  },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT8   },
                      { O_VALPAR, ATT_INT16  },
                      { 0, 0 } };
sspar p_blobs[] =   { { O_VALPAR, ATT_HANDLE },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { 0, 0 } };
sspar p_sfdbr[] =   { { O_VALPAR, ATT_HANDLE },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { O_VALPAR, ATT_INT16  },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT8   },
                      { O_VALPAR, ATT_INT16  },
                      { 0, 0 } };
sspar p_sfdbs[] =   { { O_VALPAR, ATT_HANDLE  },
                      { O_VALPAR, ATT_INT32  },
                      { O_VALPAR, ATT_INT32  },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { O_REFPAR, ATT_STRING },
                      { 0, 0 } };
sspar p_sssi[]   =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_stbl[]   =  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_BOOLEAN},
                      { 0, 0 } };
sspar p_inv_ev[] =  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_TEXT  },
                      { 0, 0 } };
sspar p_setprop[]=  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
sspar p_getprop[]=  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_loadext[]=  { { O_REFPAR, ATT_STRING},
                      { 0, 0 } };
sspar p_srverrcnt[]={ { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_INT32 },
                      { O_REFPAR, ATT_STRING},
                      { 0, 0 } };

sspar p_comptab[]=  { { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_STRING},
                      { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_enaind []=  { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_BOOLEAN },
                      { 0, 0 } };
sspar p_bool[]    = { { O_VALPAR, ATT_BOOLEAN },
                      { 0, 0 } };
sspar p_ib[]      = { { O_VALPAR, ATT_INT32 },
                      { O_VALPAR, ATT_BOOLEAN },
                      { 0, 0 } };
sspar p_strb2[]   = { { O_REFPAR, ATT_STRING  },
                      { O_VALPAR, ATT_BOOLEAN },
                      { O_VALPAR, ATT_BOOLEAN },
                      { 0, 0 } };
sspar p_bi5[]     = { { O_VALPAR, ATT_BOOLEAN },
                      { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { O_REFPAR, ATT_INT32 },
                      { 0, 0 } };
sspar p_sbi[]     = { { O_REFPAR, ATT_STRING},
                      { O_VALPAR, ATT_BOOLEAN},
                      { O_VALPAR, ATT_INT32 },
                      { 0, 0 } };

sprocobj  d_position   ={OBT_SPROC, 0, NULL     ,ATT_INT32   };
sprocobj  d_charlen    ={OBT_SPROC, 1, NULL     ,ATT_INT32   };
sprocobj  d_bitlen     ={OBT_SPROC, 2, NULL     ,ATT_INT32   };
sprocobj  d_extract    ={OBT_SPROC, 3, NULL     ,ATT_INT32   }; // may be changed by program
sprocobj  d_substring  ={OBT_SPROC, 9, NULL     ,0           }; // defined by program
sprocobj  d_lower      ={OBT_SPROC,10, NULL     ,ATT_STRING  }; // may be changed by program
sprocobj  d_upper      ={OBT_SPROC,11, NULL     ,ATT_STRING  }; // may be changed by program
sprocobj  d_fulltext   ={OBT_SPROC, FCNUM_FULLTEXT, NULL ,ATT_BOOLEAN };
sprocobj  d_selpriv    ={OBT_SPROC, FCNUM_SELPRIV,  NULL ,ATT_BOOLEAN };
sprocobj  d_updpriv    ={OBT_SPROC, FCNUM_UPDPRIV,  NULL ,ATT_BOOLEAN };
sprocobj  d_delpriv    ={OBT_SPROC, FCNUM_DELPRIV,  NULL ,ATT_BOOLEAN };
sprocobj  d_inspriv    ={OBT_SPROC, FCNUM_INSPRIV,  NULL ,ATT_BOOLEAN };
sprocobj  d_grtpriv    ={OBT_SPROC, FCNUM_GRTPRIV,  NULL ,ATT_BOOLEAN };
sprocobj  d_admin_mode ={OBT_SPROC, FCNUM_ADMMODE,  NULL ,ATT_BOOLEAN };
sprocobj  d_ft_ct      ={OBT_SPROC, FCNUM_FULLTEXT_CONTEXT, p_ft_ct,ATT_TEXT };
sprocobj  d_octlen     ={OBT_SPROC, FCNUM_OCTETLEN, NULL ,ATT_INT32   };
sprocobj  d_constr_ok  ={OBT_SPROC, FCNUM_CONSTRAINS_OK, NULL ,ATT_STRING };
sprocobj  d_ft_ct2     ={OBT_SPROC, FCNUM_FULLTEXT_CONTEXT2, p_ft_ct2,ATT_TEXT };
sprocobj  d_ft_ct9     ={OBT_SPROC, FCNUM_GET_FULLTEXT_CONTEXT, p_ft_ct9,ATT_TEXT };

sprocobj  d_create_sem ={OBT_SPROC,72, p_si     ,ATT_HANDLE  };  //4
sprocobj  d_close_sem  ={OBT_SPROC,73, p_h      ,0           };  //4
sprocobj  d_release_sem={OBT_SPROC,74, p_h      ,0           };  //4
sprocobj  d_wait_sem   ={OBT_SPROC,75, p_hi     ,ATT_INT32   };  //4
sprocobj  d_sleep      ={OBT_SPROC,76, p_vint32 ,ATT_BOOLEAN };  //4
sprocobj  d_curr_date  ={OBT_SPROC,77, p_none   ,ATT_DATE    };  //4
sprocobj  d_curr_time  ={OBT_SPROC,78, p_none   ,ATT_TIME    };  //4
sprocobj  d_curr_ts    ={OBT_SPROC,79, p_none   ,ATT_TIMESTAMP}; //4
sprocobj  d_make_date  ={OBT_SPROC,80, p_vint3  ,ATT_DATE    };  //4
sprocobj  d_day        ={OBT_SPROC,81, p_date   ,ATT_INT32   };  //4
sprocobj  d_month      ={OBT_SPROC,82, p_date   ,ATT_INT32   };  //4
sprocobj  d_year       ={OBT_SPROC,83, p_date   ,ATT_INT32   };  //4
sprocobj  d_today      ={OBT_SPROC,84, p_none   ,ATT_DATE    };  //4
sprocobj  d_make_time  ={OBT_SPROC,85, p_vint4  ,ATT_TIME    };  //4
sprocobj  d_hours      ={OBT_SPROC,86, p_time   ,ATT_INT32   };  //4
sprocobj  d_minutes    ={OBT_SPROC,87, p_time   ,ATT_INT32   };  //4
sprocobj  d_seconds    ={OBT_SPROC,88, p_time   ,ATT_INT32   };  //4
sprocobj  d_sec1000    ={OBT_SPROC,89, p_time   ,ATT_INT32   };  //4
sprocobj  d_now        ={OBT_SPROC,90, p_none   ,ATT_TIME    };  //4
sprocobj  d_ord        ={OBT_SPROC,91, p_char   ,ATT_INT32   };  //4
sprocobj  d_chr        ={OBT_SPROC,92, p_vint32 ,ATT_CHAR    };  //4
sprocobj  d_odd        ={OBT_SPROC,93, p_vint32 ,ATT_BOOLEAN };  //4
sprocobj  d_abs        ={OBT_SPROC,94, p_real   ,ATT_FLOAT   };  //4
sprocobj  d_iabs       ={OBT_SPROC,95, p_vint32 ,ATT_INT32   };  //4
sprocobj  d_sqrt       ={OBT_SPROC,96, p_real   ,ATT_FLOAT   };  //4
sprocobj  d_round      ={OBT_SPROC,97, p_real   ,ATT_INT32   };  //4
sprocobj  d_trunc      ={OBT_SPROC,98, p_real   ,ATT_INT32   };  //4
sprocobj  d_sqr        ={OBT_SPROC,99, p_real   ,ATT_FLOAT   };  //4
sprocobj  d_isqr       ={OBT_SPROC,100, p_vint32 ,ATT_INT32   }; //4
sprocobj  d_sin        ={OBT_SPROC,101, p_real   ,ATT_FLOAT   }; //4
sprocobj  d_cos        ={OBT_SPROC,102, p_real   ,ATT_FLOAT   }; //4
sprocobj  d_arctan     ={OBT_SPROC,103, p_real   ,ATT_FLOAT   }; //4
sprocobj  d_ln         ={OBT_SPROC,104, p_real   ,ATT_FLOAT   }; //4
sprocobj  d_exp        ={OBT_SPROC,105, p_real   ,ATT_FLOAT   }; //4
sprocobj  d_like       ={OBT_SPROC,106, p_str2   ,ATT_BOOLEAN }; //4
sprocobj  d_pref       ={OBT_SPROC,107, p_str2   ,ATT_BOOLEAN }; //4
sprocobj  d_substr     ={OBT_SPROC,108, p_str2   ,ATT_BOOLEAN }; //4
sprocobj  d_strcat     ={OBT_SPROC,109, p_str2   ,ATT_STRING  }; //4
sprocobj  d_upcase     ={OBT_SPROC,110, p_str1   ,0           }; //4
sprocobj  d_memcpy     ={OBT_SPROC,111, p_memcpy ,0           }; //4
sprocobj  d_str2int    ={OBT_SPROC,112, p_str1   ,ATT_INT32   }; //4
sprocobj  d_str2money  ={OBT_SPROC,113, p_str1   ,ATT_MONEY   }; //4
sprocobj  d_str2real   ={OBT_SPROC,114, p_str1   ,ATT_FLOAT   }; //4
sprocobj  d_int2str    ={OBT_SPROC,115, p_vint32 ,ATT_STRING  }; //4
sprocobj  d_money2str  ={OBT_SPROC,116, p_monint ,ATT_STRING  }; //4
sprocobj  d_real2str   ={OBT_SPROC,117, p_relint ,ATT_STRING  }; //4
sprocobj  d_strinsert  ={OBT_SPROC,124, p_ssi    ,ATT_STRING  }; //4
sprocobj  d_strdelete  ={OBT_SPROC,125, p_sii    ,ATT_STRING  }; //4
sprocobj  d_strcopy    ={OBT_SPROC,126, p_vsii   ,ATT_STRING  }; //4
sprocobj  d_strpos     ={OBT_SPROC,127, p_str2   ,ATT_INT32   }; //4
sprocobj  d_strlenght  ={OBT_SPROC,128, p_str1   ,ATT_INT32   }; //4
sprocobj  d_str2date   ={OBT_SPROC,129, p_str1   ,ATT_DATE    }; //4
sprocobj  d_str2time   ={OBT_SPROC,130, p_str1   ,ATT_TIME    }; //4
sprocobj  d_date2str   ={OBT_SPROC,131, p_datei  ,ATT_STRING  }; //4
sprocobj  d_time2str   ={OBT_SPROC,132, p_timei  ,ATT_STRING  }; //4
sprocobj  d_dayofweek  ={OBT_SPROC,133, p_date   ,ATT_INT32   }; //4
sprocobj  d_strtrim    ={OBT_SPROC,134, p_str1   ,ATT_STRING  }; //4
sprocobj  d_WB_version ={OBT_SPROC,137, p_none   ,ATT_INT32   }; //4
sprocobj  d_quarter    ={OBT_SPROC,138, p_date   ,ATT_INT32   }; //4
sprocobj  d_waits_fm   ={OBT_SPROC,139, p_docflow,ATT_BOOLEAN};//4
sprocobj  d_datetime2  ={OBT_SPROC,145, p_dattim ,ATT_TIMESTAMP};// 4
sprocobj  d_dtm2dt     ={OBT_SPROC,146, p_timest ,ATT_DATE    }; // 4
sprocobj  d_dtm2tm     ={OBT_SPROC,147, p_timest ,ATT_TIME    }; // 4
sprocobj  d_is_repl_d  ={OBT_SPROC,148, p_binary ,ATT_BOOLEAN }; // 4
sprocobj  d_log_write  ={OBT_SPROC,149, p_str1   ,0           }; // 4
sprocobj  d_free_del   ={OBT_SPROC,150, p_str1   ,ATT_BOOLEAN }; // 4
sprocobj  d_currentapl ={OBT_SPROC,151, p_none   ,ATT_STRING  }; // 4
sprocobj  d_clientnum  ={OBT_SPROC,152, p_none   ,ATT_INT32   }; // 4
sprocobj  d_str2timest ={OBT_SPROC,153, p_str1   ,ATT_TIMESTAMP }; //4
sprocobj  d_timest2str ={OBT_SPROC,154, p_timesti,ATT_STRING  }; // 4
sprocobj  d_setsqlopt  ={OBT_SPROC,155, p_vi322  ,ATT_BOOLEAN }; // 4
sprocobj  d_exec       ={OBT_SPROC,156, p_ssb    ,ATT_BOOLEAN }; // 4
sprocobj  d_ml_init    ={OBT_SPROC,157, p_str2   ,ATT_INT32    }; //4
sprocobj  d_ml_initex  ={OBT_SPROC,FCNUM_INITWBMAILEX, p_str3   ,ATT_INT32    }; //4
sprocobj  d_ml_init602 ={OBT_SPROC,158, p_str3   ,ATT_INT32    }; //4
sprocobj  d_ml_initx   ={OBT_SPROC,166, p_str1   ,ATT_INT32    }; //4
sprocobj  d_ml_close   ={OBT_SPROC,159, p_none   ,0            }; //4
sprocobj  d_ml_cre     ={OBT_SPROC,160, p_crelet ,ATT_INT32    }; //4
sprocobj  d_ml_crew    ={OBT_SPROC,FCNUM_LETTERCREW,p_crelet ,ATT_INT32    }; //4
sprocobj  d_ml_addr    ={OBT_SPROC,161,p_adrlet ,ATT_INT32    }; //4
sprocobj  d_ml_addrw   ={OBT_SPROC,FCNUM_LETTERADDADRW,p_adrlet ,ATT_INT32    }; //4
sprocobj  d_ml_file    ={OBT_SPROC,162, p_hs     ,ATT_INT32    }; //4
sprocobj  d_ml_filew   ={OBT_SPROC,FCNUM_LETTERADDFILEW, p_hs     ,ATT_INT32    }; //4
sprocobj  d_ml_send    ={OBT_SPROC,163, p_handle, ATT_INT32    }; //4
sprocobj  d_ml_take    ={OBT_SPROC,164, p_none   ,ATT_INT32    }; //4
sprocobj  d_ml_cancel  ={OBT_SPROC,165, p_handle ,0            }; //4
sprocobj  d_mb_open    ={OBT_SPROC,167, p_h      ,ATT_INT32    }; //4
sprocobj  d_mb_load    ={OBT_SPROC,168, p_hi     ,ATT_INT32    }; //4
sprocobj  d_mb_getmsg  ={OBT_SPROC,169, p_hi     ,ATT_INT32    }; //4
sprocobj  d_mb_getfli  ={OBT_SPROC,170, p_hi  ,   ATT_INT32    }; //4
sprocobj  d_mb_saveas  ={OBT_SPROC,171, p_mbsfas ,ATT_INT32    }; //4
sprocobj  d_mb_delmsg  ={OBT_SPROC,172, p_mbdelm ,ATT_INT32    }; //4
sprocobj  d_mb_getinfo ={OBT_SPROC,173, p_mbinfo ,ATT_INT32    }; //4
sprocobj  d_mb_gettype ={OBT_SPROC,174, p_none   ,ATT_INT32    }; //4
sprocobj  d_mb_close   ={OBT_SPROC,175, p_handle ,0            }; //4
sprocobj  d_mb_dial    ={OBT_SPROC,176, p_str1   ,ATT_INT32    }; //4
sprocobj  d_mb_hangup  ={OBT_SPROC,177, p_none   ,ATT_INT32    }; //4

sprocobj  d_define_log ={OBT_SPROC, FCNUM_DEFINE_LOG,p_str3,    ATT_BOOLEAN };
sprocobj  d_ft_create  ={OBT_SPROC, FCNUM_FT_CREATE, p_str2    ,ATT_BOOLEAN };
sprocobj  d_ft_index   ={OBT_SPROC, FCNUM_FT_INDEX,  p_ft_index,ATT_BOOLEAN };
sprocobj  d_ft_remove  ={OBT_SPROC, FCNUM_FT_REMOVE, p_si      ,0 }; // 183
sprocobj  d_trace      ={OBT_SPROC, FCNUM_TRACE,     p_trace   ,ATT_BOOLEAN };
sprocobj  d_sql_exec   ={OBT_SPROC, FCNUM_SQL_EXECUTE,p_text    ,ATT_INT32   };
sprocobj  d_ft_destroy ={OBT_SPROC, FCNUM_FT_DESTROY, p_str1   ,ATT_BOOLEAN }; // 186

sprocobj  d_user_name  ={OBT_SPROC,198, p_none   ,ATT_STRING   };

sprocobj  d_bigint2str ={OBT_SPROC,FCNUM_BIGINT2STR,  p_vint64, ATT_STRING  };
sprocobj  d_str2bigint ={OBT_SPROC,FCNUM_STR2BIGINT,  p_str1   ,ATT_INT64   };
sprocobj  d_mb_cretbl  ={OBT_SPROC,FCNUM_MBCRETBL,    p_str1   ,ATT_INT32   }; 
sprocobj  d_WB_build   ={OBT_SPROC,FCNUM_WB_BUILD,    p_none   ,ATT_INT32   }; 
sprocobj  d_enum_rec_pr={OBT_SPROC,FCNUM_ENUMRECPR,   NULL     ,ATT_BINARY  };
sprocobj  d_enum_tab_pr={OBT_SPROC,FCNUM_ENUMTABPR,   NULL     ,ATT_BINARY  };
sprocobj  d_setpass    ={OBT_SPROC,FCNUM_SETPASS,     p_str2   ,ATT_BOOLEAN };
sprocobj  d_membership ={OBT_SPROC,FCNUM_MEMBERSHIP,  p_memb   ,ATT_BOOLEAN };
sprocobj  d_log_wrex   ={OBT_SPROC,FCNUM_LOGWRITEEX,  p_ssi    ,0           }; 
sprocobj  d_mb_getmsgx ={OBT_SPROC,FCNUM_MBGETMSGEX,  p_hii    ,ATT_INT32   }; 
sprocobj  d_ml_blobr   ={OBT_SPROC,FCNUM_MLADDBLOBR,  p_blobr  ,ATT_INT32   }; 
sprocobj  d_ml_blobrw  ={OBT_SPROC,FCNUM_MLADDBLOBRW, p_blobr ,ATT_INT32   }; 
sprocobj  d_ml_blobs   ={OBT_SPROC,FCNUM_MLADDBLOBS,  p_blobs  ,ATT_INT32   }; 
sprocobj  d_ml_blobsw  ={OBT_SPROC,FCNUM_MLADDBLOBSW, p_blobs  ,ATT_INT32   }; 
sprocobj  d_mb_sfdbr   ={OBT_SPROC,FCNUM_MBSAVETODBR, p_sfdbr  ,ATT_INT32   }; 
sprocobj  d_mb_sfdbs   ={OBT_SPROC,FCNUM_MBSAVETODBS, p_sfdbs  ,ATT_INT32   }; 
sprocobj  d_appl_shrd  ={OBT_SPROC,FCNUM_APPLSHARED,  p_str1   ,ATT_BOOLEAN }; 
sprocobj  d_gmembership={OBT_SPROC,FCNUM_MEMBERSHIP_GET, p_memb,ATT_BOOLEAN };
sprocobj  d_round64    ={OBT_SPROC,FCNUM_ROUND64,     p_real   ,ATT_INT64   };  
sprocobj  d_getsqlopt  ={OBT_SPROC,FCNUM_GETSQLOPT,   p_none   ,ATT_INT32   };  
sprocobj  d_mp_crea    ={OBT_SPROC,FCNUM_MCREATEPROF, p_stbl   ,ATT_INT32   };
sprocobj  d_mp_dele    ={OBT_SPROC,FCNUM_MDELETEPROF, p_str1   ,ATT_INT32   };
sprocobj  d_mp_set     ={OBT_SPROC,FCNUM_MSETPROF,    p_str3   ,ATT_INT32   };
sprocobj  d_mp_get     ={OBT_SPROC,FCNUM_MGETPROF,    p_sssi   ,ATT_INT32   };
sprocobj  d_getsrvinfo ={OBT_SPROC,FCNUM_GETSERVERINFO,p_vint32, ATT_INT32   };
sprocobj  d_active_rout={OBT_SPROC,FCNUM_ACTIVE_ROUT, p_vint32 ,ATT_STRING  };
sprocobj  d_inv_event  ={OBT_SPROC,FCNUM_INVOKE_EVENT,p_inv_ev ,0           }; 
sprocobj  d_lic_cnt    ={OBT_SPROC,FCNUM_GET_LIC_CNT, p_str1   ,ATT_INT32   }; 
sprocobj  d_getprop    ={OBT_SPROC,FCNUM_GET_PROP,    p_getprop,ATT_STRING  };
sprocobj  d_setprop    ={OBT_SPROC,FCNUM_SET_PROP,    p_setprop,ATT_BOOLEAN }; 
sprocobj  d_loadext    ={OBT_SPROC,FCNUM_LOAD_EXT,    p_loadext,ATT_BOOLEAN }; 
sprocobj  d_trunc_tab  ={OBT_SPROC,FCNUM_TRUCT_TABLE, p_str1   ,ATT_BOOLEAN }; 
sprocobj  d_getsrverrcnt={OBT_SPROC,FCNUM_GETSRVERRCNT,p_srverrcnt ,0      }; 
sprocobj  d_schema_id  ={OBT_SPROC,FCNUM_SCHEMA_ID,   p_str1   ,ATT_BINARY  };  // length added in call_gen
sprocobj  d_local_schema={OBT_SPROC,FCNUM_LOCAL_SCHEMA,p_none   ,ATT_STRING  };  // length added in call_gen
sprocobj  d_lock_key   ={OBT_SPROC,FCNUM_LOCK_BY_KEY, p_ssi    ,ATT_BOOLEAN };
sprocobj  d_getsrverrcnttx={OBT_SPROC,FCNUM_GETSRVERRCNTTX, p_vint32,ATT_TEXT}; 
sprocobj  d_compare_tables={OBT_SPROC,FCNUM_COMP_TAB, p_comptab,ATT_INT32   }; 
sprocobj  d_enaind     ={OBT_SPROC,FCNUM_ENABLE_INDEX,p_enaind ,0           }; 
sprocobj  d_regex      ={OBT_SPROC,FCNUM_REGEXPR,     NULL,     ATT_BOOLEAN }; 
sprocobj  d_prof_all   ={OBT_SPROC,FCNUM_PROF_ALL,    p_bool,   0           }; 
sprocobj  d_prof_thr   ={OBT_SPROC,FCNUM_PROF_THR,    p_ib,     0           }; 
sprocobj  d_prof_reset ={OBT_SPROC,FCNUM_PROF_RESET,  p_none,   0           }; 
sprocobj  d_prof_lines ={OBT_SPROC,FCNUM_PROF_LINES,  p_bool,   0           }; 
sprocobj  d_set_th_name={OBT_SPROC,FCNUM_SET_TH_NAME, p_str1,   0           }; 
sprocobj  d_message    ={OBT_SPROC,FCNUM_MESSAGE_TO_CLI, p_str1,0           }; 
sprocobj  d_check_ft   ={OBT_SPROC,FCNUM_CHECK_FT,    p_str1,   0           }; 
sprocobj  d_ft_locking ={OBT_SPROC,FCNUM_FT_LOCKING,  p_strb2,  0           }; 
sprocobj  d_create_sysext={OBT_SPROC,FCNUM_CRE_SYSEXT,p_str1,   0           }; 
sprocobj  d_backup_fil ={OBT_SPROC,FCNUM_BACKUP_FIL,  p_str1,   ATT_INT32   }; 
sprocobj  d_replicate  ={OBT_SPROC,FCNUM_REPLICATE,   p_ssb,    ATT_BOOLEAN }; 
sprocobj  d_fatal_error={OBT_SPROC,FCNUM_FATAL_ERR,   p_none,   0           }; 
sprocobj  d_internal   ={OBT_SPROC,FCNUM_INTERNAL,    p_vint32, ATT_INT32   }; 
sprocobj  d_check_index={OBT_SPROC,FCNUM_CHECK_INDEX, p_ssi,    ATT_INT32   }; 
sprocobj  d_database_integrity={OBT_SPROC,FCNUM_DB_INT,p_bi5,   ATT_INT32   }; 
sprocobj  d_bck         ={OBT_SPROC,FCNUM_BCK,          p_sbi,   ATT_INT32   };
sprocobj  d_bck_get_patt={OBT_SPROC,FCNUM_BCK_GET_PATT, p_str1,  ATT_STRING   };
sprocobj  d_bck_reduce  ={OBT_SPROC,FCNUM_BCK_REDUCE,   p_si,    ATT_INT32   };
sprocobj  d_bck_zip     ={OBT_SPROC,FCNUM_BCK_ZIP,      p_str3,  ATT_INT32   };
sprocobj  d_conn_disk   ={OBT_SPROC,FCNUM_CONN_DISK,    p_str3,  ATT_INT32   };
sprocobj  d_disconn_disk={OBT_SPROC,FCNUM_DISCONN_DISK, p_str1,   0};
sprocobj  d_extlob_file ={OBT_SPROC,FCNUM_EXTLOB_FILE,  NULL,    ATT_STRING   };

////////////////////////////////////// system information views ///////////////////////////
static const t_ivcols ivc_lusers[] = {
 { "LOGIN_NAME",      ATT_STRING,     OBJ_NAME_LEN, 0 },
 { "CLIENT_NUMBER",   ATT_INT32,      0,  0 },
 { "SEL_SCHEMA_NAME", ATT_STRING,     OBJ_NAME_LEN, 0 },
 { "STATE",           ATT_INT32,      0, 0 },
 { "CONNECTION",      ATT_INT32,      0, 0 },
 { "NET_ADDRESS",     ATT_STRING,     30,0 },
 { "DETACHED",        ATT_BOOLEAN,    0, 0 },
 { "WORKER_THREAD",   ATT_BOOLEAN,    0, 0 },
 { "OWN_CONNECTION",  ATT_BOOLEAN,    0, 0 },
 { "TRANSACTION_OPEN",ATT_BOOLEAN,    0, 0 },
 { "ISOLATION_LEVEL", ATT_INT32,      0, 0 },
 { "SQL_OPTIONS",     ATT_INT32,      0, 0 },
 { "LOCK_WAITING_TIMEOUT", ATT_INT32, 0, 0 },
 { "COMM_ENCRYPTION", ATT_INT32,      0, 0 },
 { "SESSION_NUMBER",  ATT_INT32,      0, 0 },
 { "PROFILED_THREAD", ATT_BOOLEAN,    0, 0 },
 { "THREAD_NAME",     ATT_STRING,     OBJ_NAME_LEN, 0 },
 { "REQ_PROC_TIME",   ATT_INT32,      0, 0 },
 { "PROC_SQL_STMT",   ATT_STRING,     255, 0 },
 { "OPEN_CURSORS",    ATT_INT32,      0, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_client_activity[] = {
 { "CLIENT_NUMBER",   ATT_INT32, 0, 0 },
 { "REQUESTS",        ATT_INT32, 0, 0 },
 { "SQL_STATEMENTS",  ATT_INT32, 0, 0 },
 { "OPENED_CURSORS",  ATT_INT32, 0, 0 },
 { "PAGES_READ",      ATT_INT32, 0, 0 },
 { "PAGES_WRITTEN",   ATT_INT32, 0, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_tabcols[] = {
 { "SCHEMA_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "TABLE_NAME",       ATT_STRING, OBJ_NAME_LEN, 0 },
 { "COLUMN_NAME",      ATT_STRING, ATTRNAMELEN,  0 },
 { "ORDINAL_POSITION", ATT_INT32,  0, 0 },
 { "DATA_TYPE",        ATT_INT32,  0, 0 },
 { "LENGTH",           ATT_INT32,  0, 0 },
 { "PRECISION",        ATT_INT32,  0, 0 },  // scale, but cannot rename it - compatibility
 { "DEFAULT_VALUE",    ATT_TEXT,   0, 0 },
 { "IS_NULLABLE",      ATT_BOOLEAN,0, 0 },
 { "VALUE_COUNT",      ATT_INT32,  0, 0 },
 { "EXPANDABLE",       ATT_BOOLEAN,0, 0 },
 { "WIDE_CHAR",        ATT_BOOLEAN,0, 0 },
 { "IGNORE_CASE",      ATT_BOOLEAN,0, 0 },
 { "COLLATION_NAME",   ATT_STRING, 20, 0 },
 { "WITH_TIME_ZONE",   ATT_BOOLEAN,0, 0 },
 { "DOMAIN_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "HINT_CAPTION",     ATT_STRING, ATTRNAMELEN,  0 },
 { "HINT_HELPTEXT",    ATT_TEXT,   0,  0 },
 { "HINT_CODEBOOK",    ATT_TEXT,   0,  0 },
 { "HINT_CODETEXT",    ATT_STRING, ATTRNAMELEN,  0 },
 { "HINT_CODEVAL",     ATT_STRING, ATTRNAMELEN,  0 },
 { "HINT_OLESERVER",   ATT_TEXT,   0,  0 },
 { "SPECIF_OPAQUE",    ATT_INT32,  0,  0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_tabstat[] = {
 { "SCHEMA_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "TABLE_NAME",       ATT_STRING, OBJ_NAME_LEN, 0 },
 { "MAIN_SPACE",       ATT_INT64,  0, 0 },
 { "VALID_QUOTIENT",   ATT_FLOAT,  0, 0 },
 { "VALID_OR_DELETED_QUOTIENT", ATT_FLOAT,  0, 0 },
 { "LOB_SPACE",            ATT_INT64,  0, 0 },
 { "LOB_VALID_QUOTIENT",   ATT_FLOAT,  0, 0 },
 { "LOB_USAGE_QUOTIENT",   ATT_FLOAT,  0, 0 },
 { "INDEX_SPACE",          ATT_INT64,  0, 0 },
 { "INDEX_USAGE_QUOTIENT", ATT_FLOAT,  0, 0 },
 { "VALID_RECORDS",    ATT_INT32,  0, 0 },
 { "DELETED_RECORDS",  ATT_INT32,  0, 0 },
 { "FREE_RECORDS",     ATT_INT32,  0, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_object_state[] = {
 { "SCHEMA_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "OBJECT_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "CATEGORY",         ATT_INT16,  0, 0 },
 { "ERROR_CODE",       ATT_INT32,  0, 0 },
 { "ERROR_CONTEXT",    ATT_STRING, PRE_KONTEXT+1+POST_KONTEXT+1, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_mailprofs[] = {
 { "PROFILE_NAME",     ATT_STRING,  63, 0 },
 { "MAIL_TYPE",        ATT_INT32,   0, 0 },
 { "SMTP_SERVER",      ATT_STRING,  63, 0 },
 { "MY_ADDRESS",       ATT_STRING,  63, 0 },
 { "POP3_SERVER",      ATT_STRING,  63, 0 },
 { "USER_NAME",        ATT_STRING,  63, 0 },
 { "DIAL_CONNECION",   ATT_STRING, 254, 0 },
 { "DIAL_USER",        ATT_STRING, 254, 0 },
 { "PROF_602_MAPI",    ATT_STRING,  63, 0 },
 { "PATH",             ATT_STRING, 254, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_recentlog[] = {
 { "LOG_NAME",      ATT_STRING,    OBJ_NAME_LEN, 0 },
 { "SEQ_NUM",       ATT_INT32,     0, 0 },
 { "LOG_MESSAGE",   ATT_TEXT,      0, 0 },
 { "LOG_MESSAGE_W", ATT_TEXT,      0, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_logreqs[] = {
 { "LOG_NAME",      ATT_STRING,    OBJ_NAME_LEN, 0 },
 { "SITUATION",     ATT_INT32,     0, 0 },
 { "USERNUM",       ATT_objnum,    0, 0 },
 { "TABNUM",        ATT_objnum,    0, 0 },
 { "CONTEXT",       ATT_INT32,     0, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_procpars[] = {
 { "SCHEMA_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "PROCEDURE_NAME",   ATT_STRING, OBJ_NAME_LEN, 0 },
 { "PARAMETER_NAME",   ATT_STRING, ATTRNAMELEN,  0 },
 { "ORDINAL_POSITION", ATT_INT32,  0, 0 },
 { "DATA_TYPE",        ATT_INT32,  0, 0 },
 { "LENGTH",           ATT_INT32,  0, 0 },
 { "PRECISION",        ATT_INT32,  0, 0 },
 { "DIRECTION",        ATT_INT32,  0, 0 },
 { "SPECIF",           ATT_INT32,  0, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_profile[] = {
 { "CATEGORY_NAME",    ATT_STRING, OBJ_NAME_LEN, 0 },
 { "OBJECT_NUMBER",    ATT_INT32,  0, 0 },
 { "OBJECT_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "LINE_NUMBER",      ATT_INT32,  0, 0 },
 { "SQL",              ATT_TEXT,   0, 0 },
 { "HIT_COUNT",        ATT_INT32,  0, 0 },
 { "BRUTTO_TIME",      ATT_INT32,  3, 0 },
 { "NETTO_TIME",       ATT_INT32,  3, 0 },
 { "ACTIVE_TIME",      ATT_INT32,  3, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_locks[] = {
 { "SCHEMA_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "TABLE_NAME",       ATT_STRING, OBJ_NAME_LEN, 0 },
 { "RECORD_NUMBER",    ATT_INT32,  0, 0 },
 { "LOCK_TYPE",        ATT_INT32,  0, 0 },
 { "OWNER_USERNUM",    ATT_INT32,  0, 0 },
 { "OWNER_NAME",       ATT_STRING, OBJ_NAME_LEN, 0 },
 { "OBJECT_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_subjmemb[] = {
 { "SUBJECT_NAME",     ATT_STRING, OBJ_NAME_LEN, 0 },
 { "SUBJECT_CATEG",    ATT_INT16,  0, 0 },
 { "SUBJECT_UUID",     ATT_BINARY, UUID_SIZE, 0 },
 { "SUBJECT_OBJNUM",   ATT_objnum, 0, 0 },
 { "CONTAINER_NAME",   ATT_STRING, OBJ_NAME_LEN, 0 },
 { "CONTAINER_CATEG",  ATT_INT16,  0, 0 },
 { "CONTAINER_UUID",   ATT_BINARY, UUID_SIZE, 0 },
 { "CONTAINER_OBJNUM", ATT_objnum, 0, 0 },
 { "CONTAINER_SCHEMA", ATT_STRING, OBJ_NAME_LEN, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_privileges[] = {
 { "SCHEMA_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "TABLE_NAME",       ATT_STRING, OBJ_NAME_LEN, 0 },
 { "RECORD_NUMBER",    ATT_INT32,  0, 0 },
 { "COLUMN_NAME",      ATT_STRING, ATTRNAMELEN,  0 },
 { "SUBJECT_NAME",     ATT_STRING, OBJ_NAME_LEN, 0 },
 { "SUBJECT_CATEG",    ATT_INT16,  0, 0 },
 { "SUBJECT_UUID",     ATT_BINARY, UUID_SIZE, 0 },
 { "PRIVILEGE",        ATT_STRING, 10,  0 },
 { "SUBJECT_SCHEMA_UUID", ATT_BINARY, UUID_SIZE, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_check_cons[] = {
 { "SCHEMA_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "TABLE_NAME",       ATT_STRING, OBJ_NAME_LEN, 0 },
 { "CONSTRAINT_NAME",  ATT_STRING, OBJ_NAME_LEN, 0 },
 { "DEFINITION",       ATT_TEXT,   0, 0 },
 { "DEFERRED",         ATT_INT16,  0, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_forkeys[] = {
 { "SCHEMA_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "TABLE_NAME",       ATT_STRING, OBJ_NAME_LEN, 0 },
 { "CONSTRAINT_NAME",  ATT_STRING, OBJ_NAME_LEN, 0 },
 { "DEFINITION",       ATT_TEXT,   0, 0 },
 { "FOR_SCHEMA_NAME",  ATT_STRING, OBJ_NAME_LEN, 0 },
 { "FOR_TABLE_NAME",   ATT_STRING, OBJ_NAME_LEN, 0 },
 { "FOR_DEFINITION",   ATT_TEXT,   0, 0 },
 { "DEFERRED",         ATT_INT16,  0, 0 },
 { "UPDATE_RULE",      ATT_STRING, 12, 0 },
 { "DELETE_RULE",      ATT_STRING, 12, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_indexes[] = {
 { "SCHEMA_NAME",      ATT_STRING, OBJ_NAME_LEN, 0 },
 { "TABLE_NAME",       ATT_STRING, OBJ_NAME_LEN, 0 },
 { "CONSTRAINT_NAME",  ATT_STRING, OBJ_NAME_LEN, 0 },
 { "INDEX_TYPE",       ATT_INT16,  0, 0 },
 { "DEFINITION",       ATT_TEXT,   0, 0 },
 { "HAS_NULLS",        ATT_BOOLEAN,0, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_certificates[] = {
 { "CERT_UUID",        ATT_BINARY,   UUID_SIZE, 0 },
 { "OWNER_UUID",       ATT_BINARY,   UUID_SIZE, 0 },
 { "VALID_FROM",       ATT_TIMESTAMP,0, 0 },
 { "VALID_TO"  ,       ATT_TIMESTAMP,0, 0 },
 { "STATE",            ATT_INT32,    0, 0 },
 { "VALUE",            ATT_NOSPEC,   0, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_ip_addresses[] = {
 { "IP_STRING",        ATT_STRING,   3*IP_length+IP_length-1, 0 },
 { "IP_BIN",           ATT_BINARY,   IP_length, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_memory_usage[] = {
 { "OWNER_CODE",       ATT_INT32,    0, 0 },
 { "BYTES",            ATT_INT32,    0, 0 },
 { "ITEMS",            ATT_INT32,    0, 0 },
 { "", 0, 0, 0 } };

static const infoviewobj
  iv_certificates={ OBT_INFOVIEW, IV_CERTIFICATES,     ivc_certificates, NULL},
  iv_check_cons = { OBT_INFOVIEW, IV_CHECK_CONS,       ivc_check_cons, NULL},
  iv_forkeys    = { OBT_INFOVIEW, IV_FORKEYS,          ivc_forkeys  , NULL},
  iv_indexes    = { OBT_INFOVIEW, IV_INDICIES,         ivc_indexes ,  NULL},
  iv_locks      = { OBT_INFOVIEW, IV_LOCKS,            ivc_locks    , NULL},
  iv_lusers     = { OBT_INFOVIEW, IV_NUM_LOGGED_USERS, ivc_lusers   , NULL},
  iv_tabcols    = { OBT_INFOVIEW, IV_TABLE_COLUMNS,    ivc_tabcols  , NULL},
  iv_mailprofs  = { OBT_INFOVIEW, IV_MAIL_PROFS,       ivc_mailprofs, NULL},
  iv_recentlog  = { OBT_INFOVIEW, IV_RECENT_LOG,       ivc_recentlog, NULL},
  iv_logresq    = { OBT_INFOVIEW, IV_LOG_REQS,         ivc_logreqs  , NULL},
  iv_procpars   = { OBT_INFOVIEW, IV_PROC_PARS,        ivc_procpars , NULL},
  iv_profile    = { OBT_INFOVIEW, IV_PROFILE,          ivc_profile,   NULL},
  iv_subjmemb   = { OBT_INFOVIEW, IV_SUBJ_MEMB,        ivc_subjmemb , NULL},
  iv_privileges = { OBT_INFOVIEW, IV_PRIVILEGES,       ivc_privileges, NULL},
  iv_ip_addresses={ OBT_INFOVIEW, IV_IP_ADDRESSES,     ivc_ip_addresses, NULL},
  iv_viewed_cols= { OBT_INFOVIEW, IV_VIEWED_COLUMNS,   ivc_tabcols  , NULL},
  iv_tabstat    = { OBT_INFOVIEW, IV_TABLE_STAT,       ivc_tabstat  , NULL},
  iv_object_st  = { OBT_INFOVIEW, IV_OBJECT_ST,        ivc_object_state, NULL},
  iv_memory_usage={ OBT_INFOVIEW, IV_MEMORY_USAGE,     ivc_memory_usage, NULL},
  iv_client_activ={ OBT_INFOVIEW, IV_CLIENT_ACTIV,     ivc_client_activity, NULL},

  iv_odbc_types             = { OBT_INFOVIEW, IV_ODBC_TYPES,             ctlg_type_info , NULL},
  iv_odbc_columns           = { OBT_INFOVIEW, IV_ODBC_COLUMNS,           ctlg_columns , NULL},
  iv_odbc_column_privs      = { OBT_INFOVIEW, IV_ODBC_COLUMN_PRIVS,      ctlg_column_privileges , NULL},
  iv_odbc_foreign_keys      = { OBT_INFOVIEW, IV_ODBC_FOREIGN_KEYS,      ctlg_foreign_keys , NULL},
  iv_odbc_primary_keys      = { OBT_INFOVIEW, IV_ODBC_PRIMARY_KEYS,      ctlg_primary_keys , NULL},
  iv_odbc_procedures        = { OBT_INFOVIEW, IV_ODBC_PROCEDURES,        ctlg_procedures , NULL},
  iv_odbc_procedure_columns = { OBT_INFOVIEW, IV_ODBC_PROCEDURE_COLUMNS, ctlg_procedure_columns , NULL},
  iv_odbc_special_columns   = { OBT_INFOVIEW, IV_ODBC_SPECIAL_COLUMNS,   ctlg_special_columns, NULL },
  iv_odbc_statistics        = { OBT_INFOVIEW, IV_ODBC_STATISTICS,        ctlg_statistics, NULL },
  iv_odbc_tables            = { OBT_INFOVIEW, IV_ODBC_TABLES,            ctlg_tables, NULL },
  iv_odbc_table_privs       = { OBT_INFOVIEW, IV_ODBC_TABLE_PRIVS,       ctlg_table_privileges, NULL };
  

static const t_ivcols ivc_wordtab[] = {
 { "WORD",        ATT_STRING,  MAX_FT_WORD_LEN, 0 },
 { "WORDID",      ATT_INT32,   0,               0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_reftab[] = {
 { "WORDID",     ATT_INT32,   0, 0 },
 { "DOCID",      ATT_INT32,   0, 0 },
 { "POSITION",   ATT_INT32,   0, 0 },
 { "FLAGS",      ATT_INT8,    0, 0 },
 { "", 0, 0, 0 } };

static const t_ivcols ivc_reftab2[] = {
 { "DOCID",      ATT_INT32,   0, 0 },
 { "WORDID",     ATT_INT32,   0, 0 },
 { "", 0, 0, 0 } };

static const infoviewobj // must not have more than MAX_INFOVIEW_PARAMS parameters!!
  ivp_wordtab = { OBT_INFOVIEW, IVP_FT_WORDTAB, ivc_wordtab, p_str2 },
  ivp_reftab  = { OBT_INFOVIEW, IVP_FT_REFTAB , ivc_reftab,  p_str2 },
  ivp_reftab2 = { OBT_INFOVIEW, IVP_FT_REFTAB2, ivc_reftab2, p_str2 };

#if WBVERS>=950
#define STD_NUM 304
#else
#define STD_NUM 299
#endif

const sobjdef standard_table[STD_NUM] = {  /* an array of standard object definitions */
{ "ABS"           , (object*)&d_abs        },
{ "ACTIVE_ROUTINE_NAME", (object*)&d_active_rout},
{ "ADMIN_MODE"    , (object*)&d_admin_mode },
{ "ANO"           , (object*)&true_cdesc   }, // compatibility with <=5.0
{ "ARCTAN"        , (object*)&d_arctan     },
{ "BACKUP_DATABASE_FILE",(object*)&d_backup_fil },
{ "BIGINT2STR"    , (object*)&d_bigint2str },
{ "BIT_LENGTH"    , (object*)&d_bitlen     },
{ "CATEG_APPL"    , (object*)&ct_appl_cdesc},
{ "CATEG_CONNECTION", (object*)&ct_conn_cdesc},
{ "CATEG_CURSOR"  , (object*)&ct_curs_cdesc},
{ "CATEG_DIRCUR"  , (object*)&ct_dirc_cdesc},
{ "CATEG_DOMAIN"  , (object*)&ct_dom_cdesc },
{ "CATEG_DRAWING" , (object*)&ct_schm_cdesc},
{ "CATEG_FOLDER"  , (object*)&ct_fold_cdesc },
{ "CATEG_GRAPH"   , (object*)&ct_graf_cdesc},
{ "CATEG_GROUP"   , (object*)&ct_grup_cdesc},
{ "CATEG_INFO"    , (object*)&ct_info_cdesc },
{ "CATEG_KEY"     , (object*)&ct_key_cdesc },
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
{ "CATEG_XMLFORM" , (object*)&ct_xml_cdesc },
{ "CHARACTER_LENGTH",(object*)&d_charlen   },
{ "CHAR_LENGTH"   , (object*)&d_charlen    },
{ "CHR"           , (object*)&d_chr        },
{ "CLIENT_NUMBER" , (object*)&d_clientnum  },
{ "CLOSEWBMAIL"   , (object*)&d_ml_close   },
{ "CLOSE_SEMAPHORE",(object*)&d_close_sem   },
{ "COS"           , (object*)&d_cos        },
{ "CREATE_SEMAPHORE", (object*)&d_create_sem},
{ "CURRENT_APPLICATION", (object*)&d_currentapl },
{ "CURRENT_DATE",   (object*)&d_curr_date  },
{ "CURRENT_TIME",   (object*)&d_curr_time  },
{ "CURRENT_TIMESTAMP", (object*)&d_curr_ts },
{ "DATE2STR"      , (object*)&d_date2str   },
{ "DATETIME2TIMESTAMP", (object*)&d_datetime2  },
{ "DAY"           , (object*)&d_day        },
{ "DAY_OF_WEEK"   , (object*)&d_dayofweek  },
{ "ENABLE_INDEX"  , (object*)&d_enaind     },
{ "ENUM_REC_PRIV_SUBJECTS", (object*)&d_enum_rec_pr}, 
{ "ENUM_TAB_PRIV_SUBJECTS", (object*)&d_enum_tab_pr}, 
{ "EXEC"          , (object*)&d_exec       },
{ "EXP"           , (object*)&d_exp        },
{ "EXTRACT"       , (object*)&d_extract    },
{ "FALSE"         , (object*)&false_cdesc  },
{ "FREE_DELETED"  , (object*)&d_free_del   },
{ "FULLTEXT"      , (object*)&d_fulltext   },
{ "FULLTEXT_CONTEXT"  ,(object*)&d_ft_ct },
{ "FULLTEXT_CONTEXT2" ,(object*)&d_ft_ct2},
{ "FULLTEXT_CREATE"   ,(object*)&d_ft_create},
{ "FULLTEXT_DESTROY"  ,(object*)&d_ft_destroy},
#if WBVERS>=950
{ "FULLTEXT_GET_CONTEXT" ,(object*)&d_ft_ct9},
#endif
{ "FULLTEXT_INDEX_DOC",(object*)&d_ft_index },
{ "FULLTEXT_REMOVE_DOC",(object*)&d_ft_remove},
{ "GET_LICENCE_COUNT", (object*)&d_lic_cnt },
{ "GET_MEMBERSHIP",    (object*)&d_gmembership },
{ "GET_PROPERTY_VALUE",(object*)&d_getprop },
{ "GET_SERVER_ERROR_CONTEXT",(object*)&d_getsrverrcnt },
{ "GET_SERVER_ERROR_CONTEXT_TEXT",(object*)&d_getsrverrcnttx },
{ "GET_SERVER_INFO",   (object*)&d_getsrvinfo },
{ "GET_SQL_OPTION",    (object*)&d_getsqlopt  },
{ "HAS_DELETE_PRIVIL", (object*)&d_delpriv },
{ "HAS_GRANT_PRIVIL",  (object*)&d_grtpriv },
{ "HAS_INSERT_PRIVIL", (object*)&d_inspriv },
{ "HAS_SELECT_PRIVIL", (object*)&d_selpriv },
{ "HAS_UPDATE_PRIVIL", (object*)&d_updpriv },
{ "HOURS"         , (object*)&d_hours      },
{ "IABS"          , (object*)&d_iabs       },
{ "INITWBMAIL"    , (object*)&d_ml_init    },
{ "INITWBMAIL602" , (object*)&d_ml_init602 },
{ "INITWBMAIL602X", (object*)&d_ml_initx   },
{ "INITWBMAILEX",   (object*)&d_ml_initex  },
{ "INT2STR"       , (object*)&d_int2str    },
{ "INVOKE_EVENT"  , (object*)&d_inv_event  },
{ "ISQR"          , (object*)&d_isqr       },
{ "IS_LINK"       , (object*)&ct_link_cdesc},
{ "IS_REPL_DESTIN", (object*)&d_is_repl_d  },
{ "LETTERADDADDR" , (object*)&d_ml_addr    },
{ "LETTERADDADDRW", (object*)&d_ml_addrw   },
{ "LETTERADDBLOBR", (object*)&d_ml_blobr   },
{ "LETTERADDBLOBRW",(object*)&d_ml_blobrw  },
{ "LETTERADDBLOBS", (object*)&d_ml_blobs   },
{ "LETTERADDBLOBSW",(object*)&d_ml_blobsw  },
{ "LETTERADDFILE" , (object*)&d_ml_file    },
{ "LETTERADDFILEW", (object*)&d_ml_filew   },
{ "LETTERCANCEL"  , (object*)&d_ml_cancel  },
{ "LETTERCREATE"  , (object*)&d_ml_cre     },
{ "LETTERCREATEW" , (object*)&d_ml_crew    },
{ "LETTERSEND"    , (object*)&d_ml_send    },
{ "LIKE"          , (object*)&d_like       },
{ "LN"            , (object*)&d_ln         },
{ "LOAD_SERVER_EXTENSION", (object*)&d_loadext },
{ "LOCAL_SCHEMA_NAME", (object*)&d_local_schema },
{ "LOG_WRITE"     , (object*)&d_log_write  },
{ "LOWER"         , (object*)&d_lower      },
{ "MAILBOXDELETEMSG", (object*)&d_mb_delmsg },
{ "MAILBOXGETFILINFO", (object*)&d_mb_getfli},
{ "MAILBOXGETMSG" , (object*)&d_mb_getmsg   },
{ "MAILBOXGETMSGEX", (object*)&d_mb_getmsgx },
{ "MAILBOXLOAD"   , (object*)&d_mb_load     },
{ "MAILBOXSAVEFILEAS", (object*)&d_mb_saveas},
{ "MAILBOXSAVEFILEDBR", (object*)&d_mb_sfdbr},
{ "MAILBOXSAVEFILEDBS", (object*)&d_mb_sfdbs},
{ "MAILCLOSEINBOX", (object*)&d_mb_close    },
{ "MAILCREATEPROFILE",  (object*)&d_mp_crea },
{ "MAILCREINBOXTABLES",(object*)&d_mb_cretbl}, 
{ "MAILDELETEPROFILE",  (object*)&d_mp_dele },
{ "MAILDIAL"      , (object*)&d_mb_dial     },
{ "MAILGETINBOXINFO", (object*)&d_mb_getinfo},
{ "MAILGETPROFILEPROP", (object*)&d_mp_get  },
{ "MAILGETTYPE"   , (object*)&d_mb_gettype  },
{ "MAILHANGUP"    , (object*)&d_mb_hangup   },
{ "MAILOPENINBOX" , (object*)&d_mb_open     },
{ "MAILSETPROFILEPROP", (object*)&d_mp_set  },
{ "MAKE_DATE"     , (object*)&d_make_date  },
{ "MAKE_TIME"     , (object*)&d_make_time  },
{ "MEMCPY"        , (object*)&d_memcpy     },
{ "MESSAGE_TO_CLIENTS", (object*)&d_message },
{ "MINUTES"       , (object*)&d_minutes    },
{ "MONEY2STR"     , (object*)&d_money2str  },
{ "MONTH"         , (object*)&d_month      },
{ "NE"            , (object*)&false_cdesc  },  // compatibility with <=5.0
{ "NIL"           , (object*)&nil_cdesc    },
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
{ "NOW"           , (object*)&d_now        },
{ "OCTET_LENGTH"  , (object*)&d_octlen     },
{ "ODD"           , (object*)&d_odd        },
{ "ORD"           , (object*)&d_ord        },
{ "POSITION"      , (object*)&d_position   },
{ "PREF"          , (object*)&d_pref       },
{ "QUARTER"       , (object*)&d_quarter    },
{ "REAL2STR"      , (object*)&d_real2str   },
#if WBVERS>=950
{ "REGEXP_LIKE"   , (object*)&d_regex      },
#endif
{ "RELEASE_SEMAPHORE",(object*)&d_release_sem},
{ "REPLICATE",      (object*)&d_replicate  }, 
{ "REPL_APPL_SHARED",(object*)&d_appl_shrd },
{ "RIGHT_DEL"     , (object*)&ri_del       },
{ "RIGHT_GRANT"   , (object*)&ri_grant     },
{ "RIGHT_INSERT"  , (object*)&ri_insert    },
{ "RIGHT_READ"    , (object*)&ri_read      },
{ "RIGHT_WRITE"   , (object*)&ri_write     },
{ "ROUND"         , (object*)&d_round      },
{ "ROUND64"       , (object*)&d_round64    },
{ "SCHEMA_UUID"   , (object*)&d_schema_id  },
{ "SEC1000"       , (object*)&d_sec1000    },
{ "SECONDS"       , (object*)&d_seconds    },
{ "SET_MEMBERSHIP", (object*)&d_membership },
{ "SET_PASSWORD"  , (object*)&d_setpass    },
{ "SET_PROPERTY_VALUE",(object*)&d_setprop },
{ "SET_SQL_OPTION", (object*)&d_setsqlopt  },
{ "SIN"           , (object*)&d_sin        },
{ "SLEEP"         , (object*)&d_sleep      },
{ "SQL602_BUILD",   (object*)&d_WB_build   },
{ "SQL602_VERSION", (object*)&d_WB_version },
{ "SQL_EXECUTE"   , (object*)&d_sql_exec   },
{ "SQR"           , (object*)&d_sqr        },
{ "SQRT"          , (object*)&d_sqrt       },
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
{ "SUBSTRING"     , (object*)&d_substring  },
{ "TAKEMAILTOREMOFFICE",(object*)&d_ml_take },
{ "TIME2STR"      , (object*)&d_time2str   },
{ "TIMESTAMP2DATE", (object*)&d_dtm2dt     },
{ "TIMESTAMP2STR" , (object*)&d_timest2str },
{ "TIMESTAMP2TIME", (object*)&d_dtm2tm     },
{ "TODAY"         , (object*)&d_today      },
{ "TRACE_BCK_OBJ_ERROR", (object*)&tr_bc_cdesc },
{ "TRACE_CONVERTOR_ERROR",(object*)&tr_ce_cdesc },
{ "TRACE_CURSOR"      ,  (object*)&tr_cu_cdesc },
{ "TRACE_DELETE"      ,  (object*)&tr_de_cdesc },
{ "TRACE_DIRECT_IP"   ,  (object*)&tr_ip_cdesc },
{ "TRACE_IMPL_ROLLBACK", (object*)&tr_ir_cdesc },
{ "TRACE_INSERT"      ,  (object*)&tr_in_cdesc },
{ "TRACE_LOCK_ERROR"  ,  (object*)&tr_lo_cdesc },
{ "TRACE_LOGIN"       ,  (object*)&tr_lg_cdesc },
{ "TRACE_LOG_WRITE"   ,  (object*)&tr_lw_cdesc },
{ "TRACE_NETWORK_ERRORS",(object*)&tr_ne_cdesc },
{ "TRACE_NETWORK_GLOBAL",(object*)&tr_ng_cdesc },
{ "TRACE_PIECES",        (object*)&tr_ps_cdesc },
{ "TRACE_PROCEDURE_CALL",(object*)&tr_pc_cdesc },
{ "TRACE_READ"        ,  (object*)&tr_rd_cdesc },
{ "TRACE_REPLICATION" ,  (object*)&tr_rp_cdesc },
{ "TRACE_REPLIC_COPY" ,  (object*)&tr_rc_cdesc },
{ "TRACE_REPLIC_MAIL" ,  (object*)&tr_rm_cdesc },
{ "TRACE_REPL_CONFLICT", (object*)&tr_rf_cdesc },
{ "TRACE_SERVER_FAILURE",(object*)&tr_sf_cdesc },
{ "TRACE_SERVER_INFO" ,  (object*)&tr_si_cdesc },
{ "TRACE_SQL"         ,  (object*)&tr_sq_cdesc },
{ "TRACE_START_STOP"  ,  (object*)&tr_ss_cdesc },
{ "TRACE_TRIGGER_EXEC",  (object*)&tr_te_cdesc },
{ "TRACE_USER_ERROR"  ,  (object*)&tr_ue_cdesc },
{ "TRACE_USER_MAIL"   ,  (object*)&tr_um_cdesc },
{ "TRACE_USER_WARNING",  (object*)&tr_uw_cdesc },
{ "TRACE_WEB_REQUEST" ,  (object*)&tr_ww_cdesc },
{ "TRACE_WRITE"       ,  (object*)&tr_wr_cdesc },
{ "TRUE"          , (object*)&true_cdesc   },
{ "TRUNC"         , (object*)&d_trunc      },
{ "TRUNCATE_TABLE", (object*)&d_trunc_tab  },
{ "UNKNOWN"       , (object*)&unknown_cdesc},
{ "UPCASE"        , (object*)&d_upcase     },
{ "UPPER"         , (object*)&d_upper      },
{ "VIOLATED_CONSTRAIN",(object*)&d_constr_ok},
{ "WAITS_FOR_ME"  , (object*)&d_waits_fm   },
{ "WAIT_FOR_SEMAPHORE",(object*)&d_wait_sem },
{ "WHO_AM_I"      , (object*)&d_user_name  },
{ "WINBASE602_BUILD",   (object*)&d_WB_build },
{ "WINBASE602_VERSION", (object*)&d_WB_version },
{ "YEAR"          , (object*)&d_year       },
{ "_IV_CERTIFICATES", (object*)&iv_certificates}, 
{ "_IV_CHECK_CONSTRAINS", (object*)&iv_check_cons}, 
{ "_IV_CHECK_CONSTRAINTS",(object*)&iv_check_cons}, 
{ "_IV_CLIENT_ACTIVITY",  (object*)&iv_client_activ},
{ "_IV_FOREIGN_KEYS",   (object*)&iv_forkeys}, 
#if WBVERS>=950
{ "_IV_FULLTEXT_REFTAB",  (object*)&ivp_reftab  },
{ "_IV_FULLTEXT_REFTAB2", (object*)&ivp_reftab2  },
{ "_IV_FULLTEXT_WORDTAB", (object*)&ivp_wordtab },
#endif
{ "_IV_INDEXES",        (object*)&iv_indexes}, 
{ "_IV_INDICIES",       (object*)&iv_indexes}, 
{ "_IV_IP_ADDRESSES", (object*)&iv_ip_addresses},
{ "_IV_LOCKS",      (object*) &iv_locks    },
{ "_IV_LOGGED_USERS",(object*)&iv_lusers   },
{ "_IV_MAIL_PROFS", (object*)&iv_mailprofs },
{ "_IV_MEMORY_USAGE",(object*)&iv_memory_usage },
{ "_IV_OBJECT_STATE", (object*)&iv_object_st   }, 
{ "_IV_ODBC_COLUMNS",           (object*)&iv_odbc_columns },
{ "_IV_ODBC_COLUMN_PRIVS",      (object*)&iv_odbc_column_privs},
{ "_IV_ODBC_FOREIGN_KEYS",      (object*)&iv_odbc_foreign_keys},
{ "_IV_ODBC_PRIMARY_KEYS",      (object*)&iv_odbc_primary_keys},
{ "_IV_ODBC_PROCEDURES",        (object*)&iv_odbc_procedures},
{ "_IV_ODBC_PROCEDURE_COLUMNS", (object*)&iv_odbc_procedure_columns},
{ "_IV_ODBC_SPECIAL_COLUMNS",   (object*)&iv_odbc_special_columns},
{ "_IV_ODBC_STATISTICS",        (object*)&iv_odbc_statistics},
{ "_IV_ODBC_TABLES",            (object*)&iv_odbc_tables},
{ "_IV_ODBC_TABLE_PRIVS",       (object*)&iv_odbc_table_privs},
{ "_IV_ODBC_TYPE_INFO",         (object*)&iv_odbc_types },
{ "_IV_PENDING_LOG_REQS", (object*)&iv_logresq},
{ "_IV_PRIVILEGES", (object*)&iv_privileges },
{ "_IV_PROCEDURE_PARAMETERS",   (object*)&iv_procpars},
{ "_IV_PROFILE",                (object*)&iv_profile},
{ "_IV_RECENT_LOG", (object*)&iv_recentlog },
{ "_IV_SUBJECT_MEMBERSHIP", (object*)&iv_subjmemb },
{ "_IV_TABLE_COLUMNS",(object*)&iv_tabcols },
{ "_IV_TABLE_SPACE",     (object*)&iv_tabstat },
{ "_IV_VIEWED_TABLE_COLUMNS",(object*)&iv_viewed_cols },
{ "_SQP_BACKUP",          (object*)&d_bck },
{ "_SQP_BACKUP_GET_PATHNAME_PATTER", (object*)&d_bck_get_patt },
{ "_SQP_BACKUP_REDUCE",   (object*)&d_bck_reduce },
{ "_SQP_BACKUP_ZIP",      (object*)&d_bck_zip },
{ "_SQP_CHECK_INDEX",    (object*)&d_check_index },
{ "_SQP_COMPARE_TABLES", (object*)&d_compare_tables },
{ "_SQP_CONNECT_DISK",    (object*)&d_conn_disk },
{ "_SQP_CREATE_SYSEXT_OBJECT",  (object*)&d_create_sysext },
{ "_SQP_DATABASE_INTEGRITY",    (object*)&d_database_integrity },
{ "_SQP_DEFINE_LOG",     (object*)&d_define_log },
{ "_SQP_DISCONNECT_DISK", (object*)&d_disconn_disk },
{ "_SQP_EXTLOB_FILE",    (object*)&d_extlob_file },
{ "_SQP_FATAL_ERROR",    (object*)&d_fatal_error },
{ "_SQP_FULLTEXT_LOCKING", (object*)&d_ft_locking },
{ "_SQP_INTERNAL",       (object*)&d_internal }, 
{ "_SQP_INT_CHECK_FT",   (object*)&d_check_ft },
{ "_SQP_LOCK_BY_KEYVAL", (object*)&d_lock_key   },
{ "_SQP_LOG_WRITE", (object*)&d_log_wrex   },
{ "_SQP_PROFILE_ALL",    (object*)&d_prof_all },
{ "_SQP_PROFILE_LINES",  (object*)&d_prof_lines },
{ "_SQP_PROFILE_RESET",  (object*)&d_prof_reset },
{ "_SQP_PROFILE_THREAD", (object*)&d_prof_thr },
{ "_SQP_SET_THREAD_NAME",(object*)&d_set_th_name},
{ "_SQP_TRACE" ,         (object*)&d_trace      }
};

const char * get_function_name(int fcnum)
{ int i=0;
  do
  { if (standard_table[i].descr->any.categ==OBT_SPROC)
      if (standard_table[i].descr->proc.sprocnum==fcnum)
        return standard_table[i].name;
  } while (++i<STD_NUM);  
  return NULLSTRING;
}

///////////////////////// objects on the system scope ////////////////////////
t_scope system_scope;

