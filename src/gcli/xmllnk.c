// Generated stubs for lazy dynamic linking
static HINSTANCE hLibInst = 0;

bool load_lib(void)
{
  if (hLibInst) return true;
  hLibInst=LoadLibraryA(XML_EXT_NAME);
  return hLibInst!=0;
}

typedef   BOOL t_export_to_file(t_ucd * ucdp,  const char * dad,  t_file_outbuf & fo,  const char * fname,  tcursnum curs,  int * error_node);
typedef t_export_to_file * p_export_to_file;

CFNC   BOOL link_export_to_file(t_ucd * ucdp,  const char * dad,  t_file_outbuf & fo,  const char * fname,  tcursnum curs,  int * error_node)
{ p_export_to_file p;
  if (!load_lib()) return (BOOL)0;
  p = (p_export_to_file)GetProcAddress(hLibInst, "export_to_file");
  if (!p) return (BOOL)0;
  return (*p)(ucdp, dad, fo, fname, curs, error_node);
}

typedef   BOOL t_export_to_memory(t_ucd * ucdp,  const char * dad,  t_mem_outbuf & fo,  tcursnum curs,  int * error_node);
typedef t_export_to_memory * p_export_to_memory;

CFNC   BOOL link_export_to_memory(t_ucd * ucdp,  const char * dad,  t_mem_outbuf & fo,  tcursnum curs,  int * error_node)
{ p_export_to_memory p;
  if (!load_lib()) return (BOOL)0;
  p = (p_export_to_memory)GetProcAddress(hLibInst, "export_to_memory");
  if (!p) return (BOOL)0;
  return (*p)(ucdp, dad, fo, curs, error_node);
}

typedef   BOOL t_import_from_file(t_ucd * ucdp,  const char * dad,  const char * fname,  int * error_node,  char ** out_buffer);
typedef t_import_from_file * p_import_from_file;

CFNC   BOOL link_import_from_file(t_ucd * ucdp,  const char * dad,  const char * fname,  int * error_node,  char ** out_buffer)
{ p_import_from_file p;
  if (!load_lib()) return (BOOL)0;
  p = (p_import_from_file)GetProcAddress(hLibInst, "import_from_file");
  if (!p) return (BOOL)0;
  return (*p)(ucdp, dad, fname, error_node, out_buffer);
}

typedef   BOOL t_import_from_memory(t_ucd * ucdp,  const char * dad,  const char * buffer,  int xmlsize,  int * error_node,  char ** out_buffer);
typedef t_import_from_memory * p_import_from_memory;

CFNC   BOOL link_import_from_memory(t_ucd * ucdp,  const char * dad,  const char * buffer,  int xmlsize,  int * error_node,  char ** out_buffer)
{ p_import_from_memory p;
  if (!load_lib()) return (BOOL)0;
  p = (p_import_from_memory)GetProcAddress(hLibInst, "import_from_memory");
  if (!p) return (BOOL)0;
  return (*p)(ucdp, dad, buffer, xmlsize, error_node, out_buffer);
}

typedef   BOOL WINAPI t_Export_to_XML( cdp_t cdp,  const char * dad_ref,  const char * fname,  tcursnum curs,  struct t_clivar * hostvars,  int hostvarscount );
typedef t_Export_to_XML * p_Export_to_XML;

CFNC   BOOL WINAPI link_Export_to_XML( cdp_t cdp,  const char * dad_ref,  const char * fname,  tcursnum curs,  struct t_clivar * hostvars,  int hostvarscount )
{ p_Export_to_XML p;
  if (!load_lib()) return (BOOL)0;
  p = (p_Export_to_XML)GetProcAddress(hLibInst, "Export_to_XML");
  if (!p) return (BOOL)0;
  return (*p)(cdp, dad_ref, fname, curs, hostvars, hostvarscount);
}

typedef   BOOL WINAPI t_Import_from_XML( cdp_t cdp,  const char * dad_ref,  const char * fname,  struct t_clivar * hostvars,  int hostvarscount );
typedef t_Import_from_XML * p_Import_from_XML;

CFNC   BOOL WINAPI link_Import_from_XML( cdp_t cdp,  const char * dad_ref,  const char * fname,  struct t_clivar * hostvars,  int hostvarscount )
{ p_Import_from_XML p;
  if (!load_lib()) return (BOOL)0;
  p = (p_Import_from_XML)GetProcAddress(hLibInst, "Import_from_XML");
  if (!p) return (BOOL)0;
  return (*p)(cdp, dad_ref, fname, hostvars, hostvarscount);
}

typedef   BOOL WINAPI t_Export_to_XML_buffer( cdp_t cdp,  const char * dad_ref,  char * buffer,  int bufsize,  int * xmlsize,  tcursnum curs,  struct t_clivar * hostvars,  int hostvarscount );
typedef t_Export_to_XML_buffer * p_Export_to_XML_buffer;

CFNC   BOOL WINAPI link_Export_to_XML_buffer( cdp_t cdp,  const char * dad_ref,  char * buffer,  int bufsize,  int * xmlsize,  tcursnum curs,  struct t_clivar * hostvars,  int hostvarscount )
{ p_Export_to_XML_buffer p;
  if (!load_lib()) return (BOOL)0;
  p = (p_Export_to_XML_buffer)GetProcAddress(hLibInst, "Export_to_XML_buffer");
  if (!p) return (BOOL)0;
  return (*p)(cdp, dad_ref, buffer, bufsize, xmlsize, curs, hostvars, hostvarscount);
}

typedef   BOOL WINAPI t_Export_to_XML_buffer_alloc( cdp_t cdp,  const char * dad_ref,  char * * buffer,  tcursnum curs,  struct t_clivar * hostvars,  int hostvarscount );
typedef t_Export_to_XML_buffer_alloc * p_Export_to_XML_buffer_alloc;

CFNC   BOOL WINAPI link_Export_to_XML_buffer_alloc( cdp_t cdp,  const char * dad_ref,  char * * buffer,  tcursnum curs,  struct t_clivar * hostvars,  int hostvarscount )
{ p_Export_to_XML_buffer_alloc p;
  if (!load_lib()) return (BOOL)0;
  p = (p_Export_to_XML_buffer_alloc)GetProcAddress(hLibInst, "Export_to_XML_buffer_alloc");
  if (!p) return (BOOL)0;
  return (*p)(cdp, dad_ref, buffer, curs, hostvars, hostvarscount);
}

typedef   BOOL WINAPI t_Import_from_XML_buffer( cdp_t cdp,  const char * dad_ref,  const char * buffer,  int xmlsize,  struct t_clivar * hostvars,  int hostvarscount );
typedef t_Import_from_XML_buffer * p_Import_from_XML_buffer;

CFNC   BOOL WINAPI link_Import_from_XML_buffer( cdp_t cdp,  const char * dad_ref,  const char * buffer,  int xmlsize,  struct t_clivar * hostvars,  int hostvarscount )
{ p_Import_from_XML_buffer p;
  if (!load_lib()) return (BOOL)0;
  p = (p_Import_from_XML_buffer)GetProcAddress(hLibInst, "Import_from_XML_buffer");
  if (!p) return (BOOL)0;
  return (*p)(cdp, dad_ref, buffer, xmlsize, hostvars, hostvarscount);
}

typedef   BOOL WINAPI t_Verify_DAD( cdp_t cdp,  const char * dad,  int * line,  int * column );
typedef t_Verify_DAD * p_Verify_DAD;

CFNC   BOOL WINAPI link_Verify_DAD( cdp_t cdp,  const char * dad,  int * line,  int * column )
{ p_Verify_DAD p;
  if (!load_lib()) return (BOOL)0;
  p = (p_Verify_DAD)GetProcAddress(hLibInst, "Verify_DAD");
  if (!p) return (BOOL)0;
  return (*p)(cdp, dad, line, column);
}

typedef   char * WINAPI t_get_xml_form( cdp_t cdp,  tobjnum objnum,  BOOL translate,  const char * url_prefix );
typedef t_get_xml_form * p_get_xml_form;

CFNC   char * WINAPI link_get_xml_form( cdp_t cdp,  tobjnum objnum,  BOOL translate,  const char * url_prefix )
{ p_get_xml_form p;
  if (!load_lib()) return (char *)0;
  p = (p_get_xml_form)GetProcAddress(hLibInst, "get_xml_form");
  if (!p) return (char *)0;
  return (*p)(cdp, objnum, translate, url_prefix);
}

typedef   BOOL WINAPI t_Get_DAD_params(cdp_t cdp,  const char * dad,  t_dad_param ** pars,  int * count);
typedef t_Get_DAD_params * p_Get_DAD_params;

CFNC   BOOL WINAPI link_Get_DAD_params(cdp_t cdp,  const char * dad,  t_dad_param ** pars,  int * count)
{ p_Get_DAD_params p;
  if (!load_lib()) return (BOOL)0;
  p = (p_Get_DAD_params)GetProcAddress(hLibInst, "Get_DAD_params");
  if (!p) return (BOOL)0;
  return (*p)(cdp, dad, pars, count);
}

typedef   BOOL WINAPI t_GetDadFromXMLForm(cdp_t cdp,  tobjnum objnum,  char *DADName,  bool Inp );
typedef t_GetDadFromXMLForm * p_GetDadFromXMLForm;

CFNC   BOOL WINAPI link_GetDadFromXMLForm(cdp_t cdp,  tobjnum objnum,  char *DADName,  bool Inp )
{ p_GetDadFromXMLForm p;
  if (!load_lib()) return (BOOL)0;
  p = (p_GetDadFromXMLForm)GetProcAddress(hLibInst, "GetDadFromXMLForm");
  if (!p) return (BOOL)0;
  return (*p)(cdp, objnum, DADName, Inp);
}

typedef   char * WINAPI t_Merge_XML_form(cdp_t cdp, const char *xml_form_ref, const char *data);
typedef t_Merge_XML_form * p_Merge_XML_form;

CFNC   char * WINAPI link_Merge_XML_form(cdp_t cdp, const char *xml_form_ref, const char *data)
{ p_Merge_XML_form p;
  if (!load_lib()) return (char *)0;
  p = (p_Merge_XML_form)GetProcAddress(hLibInst, "Merge_XML_form");
  if (!p) return (char *)0;
  return (*p)(cdp, xml_form_ref, data);
}

typedef   char * WINAPI t_Merge_XML_formM(cdp_t cdp, const char *xml_form_ref, int xml_form_ref_size, const char *data, int *result_size, char **error_message);
typedef t_Merge_XML_formM * p_Merge_XML_formM;

CFNC   char * WINAPI link_Merge_XML_formM(cdp_t cdp, const char *xml_form_ref, int xml_form_ref_size, const char *data, int *result_size, char **error_message)
{ p_Merge_XML_formM p;
  if (!load_lib()) return (char *)0;
  p = (p_Merge_XML_formM)GetProcAddress(hLibInst, "Merge_XML_formM");
  if (!p) return (char *)0;
  return (*p)(cdp, xml_form_ref, xml_form_ref_size, data, result_size, error_message);
}

typedef   char * t_make_source(t_dad_root * dad_tree,  bool alter,  cdp_t object_cdp);
typedef t_make_source * p_make_source;

CFNC   char * link_make_source(t_dad_root * dad_tree,  bool alter,  cdp_t object_cdp)
{ p_make_source p;
  if (!load_lib()) return (char *)0;
  p = (p_make_source)GetProcAddress(hLibInst, "make_source");
  if (!p) return (char *)0;
  return (*p)(dad_tree, alter, object_cdp);
}

typedef   t_ucd_cl_602 * t_new_t_ucd_cl_602(cdp_t cdpIn);
typedef t_new_t_ucd_cl_602 * p_new_t_ucd_cl_602;

CFNC   t_ucd_cl_602 * link_new_t_ucd_cl_602(cdp_t cdpIn)
{ p_new_t_ucd_cl_602 p;
  if (!load_lib()) return (t_ucd_cl_602 *)0;
  p = (p_new_t_ucd_cl_602)GetProcAddress(hLibInst, "new_t_ucd_cl_602");
  if (!p) return (t_ucd_cl_602 *)0;
  return (*p)(cdpIn);
}

typedef   t_ucd_odbc * t_new_t_ucd_cl_odbc(t_pconnection connIn,  cdp_t reporting_cdpIn);
typedef t_new_t_ucd_cl_odbc * p_new_t_ucd_cl_odbc;

CFNC   t_ucd_odbc * link_new_t_ucd_cl_odbc(t_pconnection connIn,  cdp_t reporting_cdpIn)
{ p_new_t_ucd_cl_odbc p;
  if (!load_lib()) return (t_ucd_odbc *)0;
  p = (p_new_t_ucd_cl_odbc)GetProcAddress(hLibInst, "new_t_ucd_cl_odbc");
  if (!p) return (t_ucd_odbc *)0;
  return (*p)(connIn, reporting_cdpIn);
}

typedef   void t_delete_t_ucd_cl(t_ucd_cl * cdcl);
typedef t_delete_t_ucd_cl * p_delete_t_ucd_cl;

CFNC   void link_delete_t_ucd_cl(t_ucd_cl * cdcl)
{ p_delete_t_ucd_cl p;
  if (!load_lib()) return;
  p = (p_delete_t_ucd_cl)GetProcAddress(hLibInst, "delete_t_ucd_cl");
  if (!p) return;
  (*p)(cdcl);
}

typedef   char * t_new_chars(int len);
typedef t_new_chars * p_new_chars;

CFNC   char * link_new_chars(int len)
{ p_new_chars p;
  if (!load_lib()) return (char *)0;
  p = (p_new_chars)GetProcAddress(hLibInst, "new_chars");
  if (!p) return (char *)0;
  return (*p)(len);
}

typedef   void t_delete_chars(char * str);
typedef t_delete_chars * p_delete_chars;

CFNC   void link_delete_chars(char * str)
{ p_delete_chars p;
  if (!load_lib()) return;
  p = (p_delete_chars)GetProcAddress(hLibInst, "delete_chars");
  if (!p) return;
  (*p)(str);
}

typedef   char * WINAPI t_convert2utf8(char * system_def);
typedef t_convert2utf8 * p_convert2utf8;

CFNC   char * WINAPI link_convert2utf8(char * system_def)
{ p_convert2utf8 p;
  if (!load_lib()) return (char *)0;
  p = (p_convert2utf8)GetProcAddress(hLibInst, "convert2utf8");
  if (!p) return (char *)0;
  return (*p)(system_def);
}

typedef   BOOL WINAPI t_NewXMLForm(cdp_t cdp,  const CXMLFormStruct *Struct,  const char * fname,  tcursnum curs,  t_clivar * hostvars,  int hostvars_count);
typedef t_NewXMLForm * p_NewXMLForm;

CFNC   BOOL WINAPI link_NewXMLForm(cdp_t cdp,  const CXMLFormStruct *Struct,  const char * fname,  tcursnum curs,  t_clivar * hostvars,  int hostvars_count)
{ p_NewXMLForm p;
  if (!load_lib()) return (BOOL)0;
  p = (p_NewXMLForm)GetProcAddress(hLibInst, "NewXMLForm");
  if (!p) return (BOOL)0;
  return (*p)(cdp, Struct, fname, curs, hostvars, hostvars_count);
}

typedef   bool t_init_platform_utils(t_ucd * ucdp);
typedef t_init_platform_utils * p_init_platform_utils;

CFNC   bool link_init_platform_utils(t_ucd * ucdp)
{ p_init_platform_utils p;
  if (!load_lib()) return (bool)0;
  p = (p_init_platform_utils)GetProcAddress(hLibInst, "init_platform_utils");
  if (!p) return (bool)0;
  return (*p)(ucdp);
}

typedef   void t_close_platform_utils(void);
typedef t_close_platform_utils * p_close_platform_utils;

CFNC   void link_close_platform_utils(void)
{ p_close_platform_utils p;
  if (!load_lib()) return;
  p = (p_close_platform_utils)GetProcAddress(hLibInst, "close_platform_utils");
  if (!p) return;
  (*p)();
}

typedef   bool WINAPI t_create_default_schema_design(cdp_t cdp,  void * owner,  const char * schema_file,  const char *schema_name,  const char *folder_name,  t_dad_root * dad_tree,  t_xsd_gen_params * gen_params,  LPERRCALLBACK err_callback);
typedef t_create_default_schema_design * p_create_default_schema_design;

CFNC   bool WINAPI link_create_default_schema_design(cdp_t cdp,  void * owner,  const char * schema_file,  const char *schema_name,  const char *folder_name,  t_dad_root * dad_tree,  t_xsd_gen_params * gen_params,  LPERRCALLBACK err_callback)
{ p_create_default_schema_design p;
  if (!load_lib()) return (bool)0;
  p = (p_create_default_schema_design)GetProcAddress(hLibInst, "create_default_schema_design");
  if (!p) return (bool)0;
  return (*p)(cdp, owner, schema_file, schema_name, folder_name, dad_tree, gen_params, err_callback);
}

typedef   t_dad_root * t_parse(t_ucd * ucdp,  const char * source,  int * pline ,  int * pcolumn );
typedef t_parse * p_parse;

CFNC   t_dad_root * link_parse(t_ucd * ucdp,  const char * source,  int * pline ,  int * pcolumn )
{ p_parse p;
  if (!load_lib()) return (t_dad_root *)0;
  p = (p_parse)GetProcAddress(hLibInst, "parse");
  if (!p) return (t_dad_root *)0;
  return (*p)(ucdp, source, pline, pcolumn);
}

typedef   dad_element_node * t_new_dad_element_node(void);
typedef t_new_dad_element_node * p_new_dad_element_node;

CFNC   dad_element_node * link_new_dad_element_node(void)
{ p_new_dad_element_node p;
  if (!load_lib()) return (dad_element_node *)0;
  p = (p_new_dad_element_node)GetProcAddress(hLibInst, "new_dad_element_node");
  if (!p) return (dad_element_node *)0;
  return (*p)();
}

typedef   dad_attribute_node * t_new_dad_attribute_node(void);
typedef t_new_dad_attribute_node * p_new_dad_attribute_node;

CFNC   dad_attribute_node * link_new_dad_attribute_node(void)
{ p_new_dad_attribute_node p;
  if (!load_lib()) return (dad_attribute_node *)0;
  p = (p_new_dad_attribute_node)GetProcAddress(hLibInst, "new_dad_attribute_node");
  if (!p) return (dad_attribute_node *)0;
  return (*p)();
}

typedef   t_dad_root * t_new_t_dad_root(int sys_charsetIn);
typedef t_new_t_dad_root * p_new_t_dad_root;

CFNC   t_dad_root * link_new_t_dad_root(int sys_charsetIn)
{ p_new_t_dad_root p;
  if (!load_lib()) return (t_dad_root *)0;
  p = (p_new_t_dad_root)GetProcAddress(hLibInst, "new_t_dad_root");
  if (!p) return (t_dad_root *)0;
  return (*p)(sys_charsetIn);
}

typedef   t_dad_column_node * t_new_t_dad_column_node(t_dad_column_node & orig);
typedef t_new_t_dad_column_node * p_new_t_dad_column_node;

CFNC   t_dad_column_node * link_new_t_dad_column_node(t_dad_column_node & orig)
{ p_new_t_dad_column_node p;
  if (!load_lib()) return (t_dad_column_node *)0;
  p = (p_new_t_dad_column_node)GetProcAddress(hLibInst, "new_t_dad_column_node");
  if (!p) return (t_dad_column_node *)0;
  return (*p)(orig);
}

typedef   t_xml_context * t_new_xml_context(t_dad_root * dad_treeIn);
typedef t_new_xml_context * p_new_xml_context;

CFNC   t_xml_context * link_new_xml_context(t_dad_root * dad_treeIn)
{ p_new_xml_context p;
  if (!load_lib()) return (t_xml_context *)0;
  p = (p_new_xml_context)GetProcAddress(hLibInst, "new_xml_context");
  if (!p) return (t_xml_context *)0;
  return (*p)(dad_treeIn);
}

typedef   const char * t_get_encoding_name(int num);
typedef t_get_encoding_name * p_get_encoding_name;

CFNC   const char * link_get_encoding_name(int num)
{ p_get_encoding_name p;
  if (!load_lib()) return (const char *)0;
  p = (p_get_encoding_name)GetProcAddress(hLibInst, "get_encoding_name");
  if (!p) return (const char *)0;
  return (*p)(num);
}

typedef   BOOL WINAPI t_DSParserOpen(cdp_t cdp,  HANDLE *pHandle);
typedef t_DSParserOpen * p_DSParserOpen;

CFNC   BOOL WINAPI link_DSParserOpen(cdp_t cdp,  HANDLE *pHandle)
{ p_DSParserOpen p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserOpen)GetProcAddress(hLibInst, "DSParserOpen");
  if (!p) return (BOOL)0;
  return (*p)(cdp, pHandle);
}

typedef   void WINAPI t_DSParserClose(HANDLE Handle);
typedef t_DSParserClose * p_DSParserClose;

CFNC   void WINAPI link_DSParserClose(HANDLE Handle)
{ p_DSParserClose p;
  if (!load_lib()) return;
  p = (p_DSParserClose)GetProcAddress(hLibInst, "DSParserClose");
  if (!p) return;
  (*p)(Handle);
}

typedef   BOOL WINAPI t_DSParserSetStrProp(HANDLE Handle,  DSPProp Prop,  const char *Value);
typedef t_DSParserSetStrProp * p_DSParserSetStrProp;

CFNC   BOOL WINAPI link_DSParserSetStrProp(HANDLE Handle,  DSPProp Prop,  const char *Value)
{ p_DSParserSetStrProp p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserSetStrProp)GetProcAddress(hLibInst, "DSParserSetStrProp");
  if (!p) return (BOOL)0;
  return (*p)(Handle, Prop, Value);
}

typedef   BOOL WINAPI t_DSParserSetIntProp(HANDLE Handle,  DSPProp Prop,  int Value);
typedef t_DSParserSetIntProp * p_DSParserSetIntProp;

CFNC   BOOL WINAPI link_DSParserSetIntProp(HANDLE Handle,  DSPProp Prop,  int Value)
{ p_DSParserSetIntProp p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserSetIntProp)GetProcAddress(hLibInst, "DSParserSetIntProp");
  if (!p) return (BOOL)0;
  return (*p)(Handle, Prop, Value);
}

typedef   BOOL WINAPI t_DSParserGetStrProp(HANDLE Handle,  DSPProp Prop,  const char **Value);
typedef t_DSParserGetStrProp * p_DSParserGetStrProp;

CFNC   BOOL WINAPI link_DSParserGetStrProp(HANDLE Handle,  DSPProp Prop,  const char **Value)
{ p_DSParserGetStrProp p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserGetStrProp)GetProcAddress(hLibInst, "DSParserGetStrProp");
  if (!p) return (BOOL)0;
  return (*p)(Handle, Prop, Value);
}

typedef   BOOL WINAPI t_DSParserGetIntProp(HANDLE Handle,  DSPProp Prop,  int *Value);
typedef t_DSParserGetIntProp * p_DSParserGetIntProp;

CFNC   BOOL WINAPI link_DSParserGetIntProp(HANDLE Handle,  DSPProp Prop,  int *Value)
{ p_DSParserGetIntProp p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserGetIntProp)GetProcAddress(hLibInst, "DSParserGetIntProp");
  if (!p) return (BOOL)0;
  return (*p)(Handle, Prop, Value);
}

typedef   BOOL WINAPI t_DSParserParse(HANDLE Handle,  const char *Source);
typedef t_DSParserParse * p_DSParserParse;

CFNC   BOOL WINAPI link_DSParserParse(HANDLE Handle,  const char *Source)
{ p_DSParserParse p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserParse)GetProcAddress(hLibInst, "DSParserParse");
  if (!p) return (BOOL)0;
  return (*p)(Handle, Source);
}

typedef   BOOL WINAPI t_DSParserGetObjectName(HANDLE Handle,  int Index,  char *Name,  tcateg *Categ);
typedef t_DSParserGetObjectName * p_DSParserGetObjectName;

CFNC   BOOL WINAPI link_DSParserGetObjectName(HANDLE Handle,  int Index,  char *Name,  tcateg *Categ)
{ p_DSParserGetObjectName p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserGetObjectName)GetProcAddress(hLibInst, "DSParserGetObjectName");
  if (!p) return (BOOL)0;
  return (*p)(Handle, Index, Name, Categ);
}

typedef   BOOL WINAPI t_DSParserGetColumnProps(HANDLE Handle,  const char *TableName,  int ColumnIndex,  char *ColumnName,  int *Type,  t_specif *Specif,  int *Exclude);
typedef t_DSParserGetColumnProps * p_DSParserGetColumnProps;

CFNC   BOOL WINAPI link_DSParserGetColumnProps(HANDLE Handle,  const char *TableName,  int ColumnIndex,  char *ColumnName,  int *Type,  t_specif *Specif,  int *Exclude)
{ p_DSParserGetColumnProps p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserGetColumnProps)GetProcAddress(hLibInst, "DSParserGetColumnProps");
  if (!p) return (BOOL)0;
  return (*p)(Handle, TableName, ColumnIndex, ColumnName, Type, Specif, Exclude);
}

typedef   BOOL WINAPI t_DSParserSetColumnProps(HANDLE Handle,  const char *TableName,  const char *ColumnName,  int Type,  t_specif Specif,  bool Exclude);
typedef t_DSParserSetColumnProps * p_DSParserSetColumnProps;

CFNC   BOOL WINAPI link_DSParserSetColumnProps(HANDLE Handle,  const char *TableName,  const char *ColumnName,  int Type,  t_specif Specif,  bool Exclude)
{ p_DSParserSetColumnProps p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserSetColumnProps)GetProcAddress(hLibInst, "DSParserSetColumnProps");
  if (!p) return (BOOL)0;
  return (*p)(Handle, TableName, ColumnName, Type, Specif, Exclude);
}

typedef   BOOL WINAPI t_DSParserGetTableExcludeFlag(HANDLE Handle,  const char *TableName,  bool *Exclude);
typedef t_DSParserGetTableExcludeFlag * p_DSParserGetTableExcludeFlag;

CFNC   BOOL WINAPI link_DSParserGetTableExcludeFlag(HANDLE Handle,  const char *TableName,  bool *Exclude)
{ p_DSParserGetTableExcludeFlag p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserGetTableExcludeFlag)GetProcAddress(hLibInst, "DSParserGetTableExcludeFlag");
  if (!p) return (BOOL)0;
  return (*p)(Handle, TableName, Exclude);
}

typedef   BOOL WINAPI t_DSParserSetTableExcludeFlag(HANDLE Handle,  const char *TableName,  bool Exclude);
typedef t_DSParserSetTableExcludeFlag * p_DSParserSetTableExcludeFlag;

CFNC   BOOL WINAPI link_DSParserSetTableExcludeFlag(HANDLE Handle,  const char *TableName,  bool Exclude)
{ p_DSParserSetTableExcludeFlag p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserSetTableExcludeFlag)GetProcAddress(hLibInst, "DSParserSetTableExcludeFlag");
  if (!p) return (BOOL)0;
  return (*p)(Handle, TableName, Exclude);
}

typedef   BOOL WINAPI t_DSParserCreateObjects(HANDLE Handle);
typedef t_DSParserCreateObjects * p_DSParserCreateObjects;

CFNC   BOOL WINAPI link_DSParserCreateObjects(HANDLE Handle)
{ p_DSParserCreateObjects p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserCreateObjects)GetProcAddress(hLibInst, "DSParserCreateObjects");
  if (!p) return (BOOL)0;
  return (*p)(Handle);
}

typedef   BOOL WINAPI t_DSParserGetLastError(HANDLE Handle,  int *ErrCode,  const char **ErrMsg);
typedef t_DSParserGetLastError * p_DSParserGetLastError;

CFNC   BOOL WINAPI link_DSParserGetLastError(HANDLE Handle,  int *ErrCode,  const char **ErrMsg)
{ p_DSParserGetLastError p;
  if (!load_lib()) return (BOOL)0;
  p = (p_DSParserGetLastError)GetProcAddress(hLibInst, "DSParserGetLastError");
  if (!p) return (BOOL)0;
  return (*p)(Handle, ErrCode, ErrMsg);
}

typedef   MFDState WINAPI t_MatchFormAndDAD(cdp_t cdp,  const char *Form,  const char *DAD);
typedef t_MatchFormAndDAD * p_MatchFormAndDAD;

CFNC   MFDState WINAPI link_MatchFormAndDAD(cdp_t cdp,  const char *Form,  const char *DAD)
{ p_MatchFormAndDAD p;
  if (!load_lib()) return (MFDState)0;
  p = (p_MatchFormAndDAD)GetProcAddress(hLibInst, "MatchFormAndDAD");
  if (!p) return (MFDState)0;
  return (*p)(cdp, Form, DAD);
}

typedef   t_mem_outbuf * t_new_t_mem_outbuf(char * bufferIn,  int bufsizeIn);
typedef t_new_t_mem_outbuf * p_new_t_mem_outbuf;

CFNC   t_mem_outbuf * link_new_t_mem_outbuf(char * bufferIn,  int bufsizeIn)
{ p_new_t_mem_outbuf p;
  if (!load_lib()) return (t_mem_outbuf *)0;
  p = (p_new_t_mem_outbuf)GetProcAddress(hLibInst, "new_t_mem_outbuf");
  if (!p) return (t_mem_outbuf *)0;
  return (*p)(bufferIn, bufsizeIn);
}

typedef   t_file_outbuf * t_new_t_file_outbuf(void);
typedef t_new_t_file_outbuf * p_new_t_file_outbuf;

CFNC   t_file_outbuf * link_new_t_file_outbuf(void)
{ p_new_t_file_outbuf p;
  if (!load_lib()) return (t_file_outbuf *)0;
  p = (p_new_t_file_outbuf)GetProcAddress(hLibInst, "new_t_file_outbuf");
  if (!p) return (t_file_outbuf *)0;
  return (*p)();
}

typedef   void t_delete_t_outbuf(t_outbuf * ob);
typedef t_delete_t_outbuf * p_delete_t_outbuf;

CFNC   void link_delete_t_outbuf(t_outbuf * ob)
{ p_delete_t_outbuf p;
  if (!load_lib()) return;
  p = (p_delete_t_outbuf)GetProcAddress(hLibInst, "delete_t_outbuf");
  if (!p) return;
  (*p)(ob);
}

typedef   uns32 WINAPI t_XMLGetVersion(void);
typedef t_XMLGetVersion * p_XMLGetVersion;

CFNC   uns32 WINAPI link_XMLGetVersion(void)
{ p_XMLGetVersion p;
  if (!load_lib()) return (uns32)0;
  p = (p_XMLGetVersion)GetProcAddress(hLibInst, "XMLGetVersion");
  if (!p) return (uns32)0;
  return (*p)();
}

