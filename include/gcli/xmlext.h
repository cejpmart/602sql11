// XML_EXT_NAME DllXMLExt

struct CXMLFormStruct;
struct t_xsd_gen_params;

CFNC DllXMLExt BOOL export_to_file(t_ucd * ucdp, const char * dad, t_file_outbuf & fo, const char * fname, tcursnum curs, int * error_node);
CFNC DllXMLExt BOOL export_to_memory(t_ucd * ucdp, const char * dad, t_mem_outbuf & fo, tcursnum curs, int * error_node);
CFNC DllXMLExt BOOL import_from_file(t_ucd * ucdp, const char * dad, const char * fname, int * error_node, char ** out_buffer);
CFNC DllXMLExt BOOL import_from_memory(t_ucd * ucdp, const char * dad, const char * buffer, int xmlsize, int * error_node, char ** out_buffer);

CFNC DllXMLExt BOOL WINAPI Export_to_XML( cdp_t cdp, const char * dad_ref, const char * fname, tcursnum curs, struct t_clivar * hostvars, int hostvarscount );
CFNC DllXMLExt BOOL WINAPI Import_from_XML( cdp_t cdp, const char * dad_ref, const char * fname, struct t_clivar * hostvars, int hostvarscount );
CFNC DllXMLExt BOOL WINAPI Export_to_XML_buffer( cdp_t cdp, const char * dad_ref, char * buffer, int bufsize, int * xmlsize, tcursnum curs, struct t_clivar * hostvars, int hostvarscount );
CFNC DllXMLExt BOOL WINAPI Export_to_XML_buffer_alloc( cdp_t cdp, const char * dad_ref, char * * buffer, tcursnum curs, struct t_clivar * hostvars, int hostvarscount );
CFNC DllXMLExt BOOL WINAPI Import_from_XML_buffer( cdp_t cdp, const char * dad_ref, const char * buffer, int xmlsize, struct t_clivar * hostvars, int hostvarscount );
CFNC DllXMLExt BOOL WINAPI Verify_DAD( cdp_t cdp, const char * dad, int * line, int * column );
CFNC DllXMLExt char * WINAPI get_xml_form( cdp_t cdp, tobjnum objnum, BOOL translate, const char * url_prefix );

CFNC DllXMLExt BOOL WINAPI Get_DAD_params(cdp_t cdp, const char * dad, t_dad_param ** pars, int * count);
CFNC DllXMLExt BOOL WINAPI GetDadFromXMLForm(cdp_t cdp, tobjnum objnum, char *DADName, bool Inp = false);
CFNC DllXMLExt char * WINAPI Merge_XML_form(cdp_t cdp,const char *xml_form_ref,const char *data);
CFNC DllXMLExt char * WINAPI Merge_XML_formM(cdp_t cdp,const char *xml_form_ref,int xml_form_ref_size,const char *data,int *result_size,char **error_message);
CFNC DllXMLExt char * make_source(t_dad_root * dad_tree, bool alter, cdp_t object_cdp);
CFNC DllXMLExt t_ucd_cl_602 * new_t_ucd_cl_602(cdp_t cdpIn);
CFNC DllXMLExt t_ucd_odbc * new_t_ucd_cl_odbc(t_pconnection connIn, cdp_t reporting_cdpIn);
CFNC DllXMLExt void delete_t_ucd_cl(t_ucd_cl * cdcl);
CFNC DllXMLExt char * new_chars(int len);
CFNC DllXMLExt void delete_chars(char * str);
CFNC DllXMLExt char * WINAPI convert2utf8(char * system_def);
CFNC DllXMLExt BOOL WINAPI NewXMLForm(cdp_t cdp, const CXMLFormStruct *Struct, const char * fname, tcursnum curs, t_clivar * hostvars, int hostvars_count);
CFNC DllXMLExt bool init_platform_utils(t_ucd * ucdp);
CFNC DllXMLExt void close_platform_utils(void);
CFNC DllXMLExt bool WINAPI create_default_schema_design(cdp_t cdp, void * owner, const char * schema_file, const char *schema_name, const char *folder_name, t_dad_root * dad_tree, t_xsd_gen_params * gen_params, LPERRCALLBACK err_callback);
CFNC DllXMLExt t_dad_root * parse(t_ucd * ucdp, const char * source, int * pline = NULL, int * pcolumn = NULL);

CFNC DllXMLExt dad_element_node * new_dad_element_node(void);
CFNC DllXMLExt dad_attribute_node * new_dad_attribute_node(void);
CFNC DllXMLExt t_dad_root * new_t_dad_root(int sys_charsetIn);
CFNC DllXMLExt t_dad_column_node * new_t_dad_column_node(t_dad_column_node & orig);
CFNC DllXMLExt t_xml_context * new_xml_context(t_dad_root * dad_treeIn);
CFNC DllXMLExt const char * get_encoding_name(int num);

CFNC DllXMLExt BOOL WINAPI DSParserOpen(cdp_t cdp, HANDLE *pHandle);
CFNC DllXMLExt void WINAPI DSParserClose(HANDLE Handle);
CFNC DllXMLExt BOOL WINAPI DSParserSetStrProp(HANDLE Handle, DSPProp Prop, const char *Value);
CFNC DllXMLExt BOOL WINAPI DSParserSetIntProp(HANDLE Handle, DSPProp Prop, int Value);
CFNC DllXMLExt BOOL WINAPI DSParserGetStrProp(HANDLE Handle, DSPProp Prop, const char **Value);
CFNC DllXMLExt BOOL WINAPI DSParserGetIntProp(HANDLE Handle, DSPProp Prop, int *Value);
CFNC DllXMLExt BOOL WINAPI DSParserParse(HANDLE Handle, const char *Source);
CFNC DllXMLExt BOOL WINAPI DSParserGetObjectName(HANDLE Handle, int Index, char *Name, tcateg *Categ);
CFNC DllXMLExt BOOL WINAPI DSParserGetColumnProps(HANDLE Handle, const char *TableName, int ColumnIndex, char *ColumnName, int *Type, t_specif *Specif, int *Exclude);
CFNC DllXMLExt BOOL WINAPI DSParserSetColumnProps(HANDLE Handle, const char *TableName, const char *ColumnName, int Type, t_specif Specif, bool Exclude);
CFNC DllXMLExt BOOL WINAPI DSParserGetTableExcludeFlag(HANDLE Handle, const char *TableName, bool *Exclude);
CFNC DllXMLExt BOOL WINAPI DSParserSetTableExcludeFlag(HANDLE Handle, const char *TableName, bool Exclude);
CFNC DllXMLExt BOOL WINAPI DSParserCreateObjects(HANDLE Handle);
CFNC DllXMLExt BOOL WINAPI DSParserGetLastError(HANDLE Handle, int *ErrCode, const char **ErrMsg);
CFNC DllXMLExt MFDState WINAPI MatchFormAndDAD(cdp_t cdp, const char *Form, const char *DAD);

CFNC DllXMLExt t_mem_outbuf * new_t_mem_outbuf(char * bufferIn, int bufsizeIn);
CFNC DllXMLExt t_file_outbuf * new_t_file_outbuf(void);
CFNC DllXMLExt void delete_t_outbuf(t_outbuf * ob);

CFNC DllXMLExt uns32 WINAPI XMLGetVersion(void);
