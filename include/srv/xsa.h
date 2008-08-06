// xsa.h

#define XSA_VERSION 7

struct xsa
{ int (*get_xsa_version)(void);
  t_scope * (*make_scope_from_cursor)(cdp_t cdp, cur_descr * cd);
  bool (*add_cursor_structure_to_scope)(cdp_t cdp, CIp_t CI, t_scope & scope, db_kontext * kont, const char * name);
  bool (*load_cursor_record_to_rscope)(cdp_t cdp, cur_descr * cd, t_rscope * rscope);
  void (*release_cursor_rscope)(cdp_t cdp, cur_descr * cd, t_rscope * rscope);
  t_rscope * (*create_and_activate_rscope)(cdp_t cdp, t_scope & scope, int scope_level);
  void (*deactivate_and_drop_rscope)(cdp_t cdp);
  void (*make_complete)(cur_descr * cd);
  const trecnum * (*get_recnum_list)(cur_descr * cd, trecnum crec);
  void (*destruct_scope)(t_scope * scope);
  void (*access_locdecl_struct)(t_rscope * rscope, int column_num, t_locdecl *& locdecl, t_row_elem *& rel);
  BOOL (*find_obj)(cdp_t cdp, const char * name, const char * schema, tcateg categ, tobjnum * pobjnum);
  tptr (*ker_load_object)(cdp_t cdp, trecnum recnum);
  void (*ker_free)(void * ptr);
  BOOL (*commit)(cdp_t cdp);
  void (*roll_back)(cdp_t cdp);
  BOOL (WINAPI *request_error)(cdp_t cdp, int errnum);
  d_table * (*kernel_get_descr)(cdp_t cdp, tcateg categ, tobjnum tbnum, unsigned * size);
  tcursnum (*open_cursor_in_current_kontext)(cdp_t cdp, const char * query, cur_descr ** pcd);
  void (*close_working_cursor)(cdp_t cdp, tcursnum cursnum);
  cur_descr * (*get_cursor)(cdp_t cdp, tcursnum cursnum);
  void (*request_generic_error)(cdp_t cdp, int errnum, const char * msg);
  void * (*ker_alloc)(int size);

  t_ins_upd_ext_supp * (*eius_prepare)(cdp_t cdp, ttablenum tbnum);
  void (*eius_drop)(t_ins_upd_ext_supp * eius);
  void (*eius_clear)(t_ins_upd_ext_supp * eius);
  bool (*eius_store_data)(t_ins_upd_ext_supp * eius, int pos, char * dataptr, unsigned size, bool appending);  // appends data to a var-len buffer
  char * (*eius_get_dataptr)(t_ins_upd_ext_supp * eius, int pos);
  trecnum (*eius_insert)(cdp_t cdp, t_ins_upd_ext_supp * eius);
  bool (*eius_read_column_value)(cdp_t cdp, t_ins_upd_ext_supp * eius, trecnum recnum, int pos); 
  bool (*has_xml_licence)(cdp_t cdp);
  void * (*get_native_variable_val)(cdp_t cdp, const char * variable_name, int & type, t_specif & specif, int & vallen, bool importing);
  bool (*write_data)(cdp_t cdp, ttablenum tablenum, trecnum current_extrec, int column_num, const void * val, int vallen);
  BOOL (*request_error_with_msg)(cdp_t cdp, int errnum, const char * msg);
  void (*append_native_variable_val)(cdp_t cdp, const char * variable_name, const void * wrval, int bytes, bool appending);
  int  (*get_system_charset)(void);
  bool (*translate_by_function)(cdp_t cdp, const char * funct_name, wuchar * str, int bufsize);
  t_ins_upd_ext_supp * (*eius_prepare2)(cdp_t cdp, ttablenum tbnum, tattrib * attrs);
  char * (*get_web_access_params)(cdp_t cdp);
};

