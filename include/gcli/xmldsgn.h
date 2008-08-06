// xmldsgn.h
#include "recwin9.h"

#define XMLCODE_UCS2  0
#define XMLCODE_UCS4  1
#define XMLCODE_ASCII 2
#define XMLCODE_UTF8  3
#define XMLCODE_UTF7  4
#define XMLCODE_852   5
#define XMLCODE_1252  6
#define XMLCODE_850   7
#define XMLCODE_1250  8
#define XMLCODE_88592 9
#define XMLCODE_88591 10
#ifdef WINS
#define XMLCODE_PLATFORM_WIDE XMLCODE_UCS2
#define XMLCODE_API_WIDE      XMLCODE_UCS2
#else
#define XMLCODE_PLATFORM_WIDE XMLCODE_UCS2   // XMLCh is 2-byte on Linux
#define XMLCODE_API_WIDE      XMLCODE_UCS4   // wchar_t is 4-byte on Linux
#endif

extern int xmlcode_system;
int charset2xmlcode(int charset);
int xml_convert(int from_code, int to_code, const char * source, size_t srcsize, char * dest, size_t destsize);
char * get_web_access_params(cdp_t cdp);
extern char sys_decim_separ;
bool prepare_conversions(void);

struct t_enc_table
{ char * enc_name;
  int enc_code;
};

extern const t_enc_table enc_table[];

class t_ucd_cl : public t_ucd
{
  friend t_ucd * get_ucdp_for_name(const char * dsn, t_xml_context * xc);
 protected:
  t_clivar * hostvars;
  int hostvars_count;
 public:
  virtual void supply_temp_hostvars(t_clivar * hostvarsIn, int hostvars_countIn);
  t_ucd_cl(t_clivar * hostvarsIn, int hostvars_countIn);
  virtual void dematerialize_params(void);
  t_dad_loader * get_dad_loader(const char * dad_ref);
  void release_memory(void * ptr)
    { corefree(ptr); }
};

class t_ucd_cl_602 : public t_ucd_cl
{public:  
  cdp_t cdp;
 public:
  t_ucd_cl_602(cdp_t cdpIn, t_clivar * hostvarsIn=NULL, int hostvars_countIn=0);
  t_record_window * create_record_window(void);
  t_record_space * make_record_space(int table_index);
  void raise_error(int errnum);
  void raise_generic_error(int errnum, const wchar_t * errmsg);
  const d_table * get_table_d(tcateg categ, const char * table_name, const char * schema_name, tcursnum cursnum);
  void release_table_d(const d_table * td);
  bool _find_object(const char * name, const char * schema, tcateg categ, tobjnum * objnum);
  bool open_cursor(const char * query, tcursnum * curs);
  void close_cursor(tcursnum curs);
  int  current_error(void);
  void start_transaction(void);
  bool commit(void);
  void roll_back(void);
  bool has_xml_licence(void)
  { 
#if (BLD_FLAGS & 1)  // unblocked version
    return true;
#else    
    uns32 buf;
    return !cd_Get_server_info(cdp, OP_GI_LICS_XML, &buf, sizeof(buf)) && buf!=0; 
#endif    
  }
  int  get_system_charset(void);
  void * get_native_variable_val(const char * variable_name, int & type, t_specif & specif, int & vallen, bool importing);
  void append_native_variable_val(const char * variable_name, const void * wrval, int bytes, bool appending);
  bool translate(const char * funct_name, wuchar * str, int bufsize);
  void * client_cdp(void) const;
  t_ucd_type ucd_type(void) const
    { return UCL_CLI_602; }
  const char * quote(void) const
    { return "`"; }
  char * load_object_utf8(tobjnum objnum);
  char * get_web_access_params(void) const
    { return ::get_web_access_params(cdp); };
  int nothing(void)
    { return 1; }
};

class t_ucd_odbc : public t_ucd_cl
{ t_pconnection conn;
  int last_error;
  char quote_string[2];
 public:
  cdp_t reporting_cdp;  // just for error messages
  t_ucd_odbc(t_pconnection connIn, cdp_t reporting_cdpIn, t_clivar * hostvarsIn=NULL, int hostvars_countIn=0);
  t_record_window * create_record_window(void);
  t_record_space * make_record_space(int table_index);
  void raise_error(int errnum);
  void raise_generic_error(int errnum, const wchar_t * errmsg);
  const d_table * get_table_d(tcateg categ, const char * table_name, const char * schema_name, tcursnum cursnum);
  void release_table_d(const d_table * td);
  bool _find_object(const char * name, const char * schema, tcateg categ, tobjnum * objnum);
  bool open_cursor(const char * query, tcursnum * curs);
  void close_cursor(tcursnum curs);
  int  current_error(void);
  void start_transaction(void);
  bool commit(void);
  void roll_back(void);
  bool has_xml_licence(void)
    { return true; }
  int  get_system_charset(void);
  void * get_native_variable_val(const char * variable_name, int & type, t_specif & specif, int & vallen, bool importing);
  void append_native_variable_val(const char * variable_name, const void * wrval, int bytes, bool appending);
  bool translate(const char * funct_name, wuchar * str, int bufsize);
  void * client_cdp(void) const;
  t_ucd_type ucd_type(void) const
    { return UCL_CLI_ODBC; }
  const char * quote(void) const
    { return quote_string; }
  char * load_object_utf8(tobjnum objnum)
    { return NULL; }
  char * get_web_access_params(void) const
    { return NULL; };
  int nothing(void)
    { return 1; }
};

t_ucd * clone_ucd_sr(t_ucd * orig);

#ifndef SRVR
class t_record_window_cl : public t_record_window
{protected: 
  trecnum ext_recnums[2];  // used only in the t_record_window_cl_602 class
  tobjname user_name;      // used only in the t_record_window_cl_602 class
  ltable * dt;
  int count_of_valid_recs;
  trecnum srec, src_recnum;
 public:
  const void * get_native_column_val(int column_num, trecnum rec_offset, int & type, t_specif & specif, int & length);
  bool more_records(void)
    { return count_of_valid_recs>=2; }
  bool start(t_ucd * ucdpIn, HANDLE incurs, bool close_the_cursor);
  bool to_next_record(void);
  t_record_window_cl(void)
    { dt=NULL; }
  ~t_record_window_cl(void);
};

class t_record_window_cl_602 : public t_record_window_cl
{
 public:
  bool load_next_valid_record(trecnum cache_pos);
  bool start(t_ucd * ucdpIn, HANDLE incurs, bool close_the_cursor);
  bool is_null_record     (int table_index_in_cursor, t_output_cursor * oc, int table_index);
  bool same_record_numbers(int table_index_in_cursor, t_output_cursor * oc, int table_index);
};

class t_record_window_cl_odbc : public t_record_window_cl
{
 public:
  bool load_next_valid_record(trecnum cache_pos);
  bool start(t_ucd * ucdpIn, HANDLE incurs, bool close_the_cursor);
  bool is_null_record     (int table_index_in_cursor, t_output_cursor * oc, int table_index);
  bool same_record_numbers(int table_index_in_cursor, t_output_cursor * oc, int table_index);
};
#endif

extern uns8 _tpsize[ATT_AFTERLAST];

enum t_xml_error { XMLE_OK, XMLE_COLUMN_NAME_NOT_IN_DATABASE, XMLE_TABLE_NAME_NOT_FOUND, 
  XMLE_BAD_JOIN, XMLE_BAD_UPPER_TABLE, XMLE_QUERY_ERROR, XMLE_TABLE_NAME_MISSING, XMLE_CANNOT_IMPORT_SYNT,
  XMLE_TOO_COMPLEX_QUERY, XMLE_UPPER_ELEMENT_NOT_FOUND, XMLE_UPPER_ELEMENT_LINKED_TO_DIFF_TAB, XMLE_DATA_SOURCE_NOT_CONN };

extern const char * error_messages[];  // indexed by error number

enum t_s6_ex_type // import errors
{ EX_PARSER, EX_DATABASE, EX_CONVERSION, EX_UNDEF_PCDATA, EX_UNDEF_ELEMENT, 
  EX_STACK_OVERFLOW, EX_INTERNAL, EX_VARIABLE, EX_VAR_TYPE, EX_CONST_DATA, 
  EX_ODBC, 
  EX_XMLCORE_NOT_AVAIL, EX_XMLCORE_FAILED
};

extern const char * import_error_messages[]; // indexed by t_s6_ex_type

struct t_dad_root;

#ifndef SRVR

class t_trigger_starter_odbc : public t_trigger_starter
{ SQLHSTMT handle;

};

class t_trigger_starter_cli602 : public t_trigger_starter
{

  virtual bool prepare(t_ucd * ucdp, const dad_element_node * el, int type, t_specif specif);
};

class t_key_evaluator_odbc : public t_key_evaluator
{ t_connection * conn;
  char * expr;
  void * handle;
  SQLLEN rdsize;
  int bufsize;
  int ctype;
  SQLSMALLINT DataType;  
  SQLULEN ParameterSize;  
 public:
  virtual bool eval(void);
  virtual bool prepare(t_ucd * ucdp, const dad_element_node * el, int type, t_specif specif);
  t_key_evaluator_odbc(void);
  virtual ~t_key_evaluator_odbc(void);
  bool prepare2(void);

};

class t_record_space_cl : public t_record_space 
{
 protected:
  char _tablename[MAX_OBJECT_NAME_LEN+1];
  ltable * dt;
  t_clivar * hostvars;
 public:
  t_record_space_cl(int table_indexIn) : t_record_space(table_indexIn)
    { dt=NULL;  hostvars=NULL; }
  ~t_record_space_cl(void);
  char * get_native_column_value(int column_num);
  void get_column_descr(int column_num, t_col_info * ci) const;
  void column_changed(int column_num);
  bool start_new_record(void);
  bool write_native_column_value(int column_num, const void * val, int vallen, bool appending);
};

class t_record_space_cl_602 : public t_record_space_cl 
{
  uns32 handle;
 public:
  t_record_space_cl_602(int table_indexIn) : t_record_space_cl(table_indexIn)
    { handle=0; }
  ~t_record_space_cl_602(void);
  bool prepare(t_ucd * ucdp, const dad_element_node * el);
  bool prepare2(t_ucd * ucdp, const d_table * td, const dad_element_node * el);
  bool write_native_column_value(int column_num, const void * val, int vallen, bool appending);
  bool read_column_value(int column_num);
  bool insert(void);
};

class t_record_space_cl_odbc : public t_record_space_cl 
{ char _prefix[MAX_OBJECT_NAME_LEN+1];
  t_ucd * _ucdp;
 public:
  t_record_space_cl_odbc(int table_indexIn) : t_record_space_cl(table_indexIn)
   { }
  ~t_record_space_cl_odbc(void);
  bool prepare(t_ucd * ucdp, const dad_element_node * el);
  bool prepare2(t_ucd * ucdp, const d_table * td, const dad_element_node * el);
  bool write_native_column_value(int column_num, const void * val, int vallen, bool appending);
  bool read_column_value(int column_num);
  bool insert(void);
};

#endif

struct t_column_map
{ uns8    table_index;
  tattrib table_colnum;
};

SPECIF_DYNAR(t_column_map_dynar, t_column_map, 3);

struct t_output_cursor
{// recorw window and destructor support:
  ::t_ucd * ucdp;
 // cursor construction:
  const bool cursor_is_external;  // query specified in the DAD or cursor passed as a parameter
  t_flstr select, condition, from, orderby;
  char * sql_stmt;
  int table_count_in_cursor;
 // columns of the analytical tables included in the cursor:
  t_column_map_dynar col_map; // index is column number in the cursor - 1
 // the data access:
  t_record_window * rcw;

  t_output_cursor(::t_ucd * ucdpIn, bool cursor_is_externalIn) : ucdp(ucdpIn), cursor_is_external(cursor_is_externalIn)
  { sql_stmt = NULL;  table_count_in_cursor=0;
    rcw=NULL;
  }
  ~t_output_cursor(void)
  { if (sql_stmt) delete [] sql_stmt;
  }

  void make_query_pattern(t_flstr * pselect);
  int  add_occ_condition(const char * occ_cond, t_xml_context * xc);
  bool open_cursor(t_xml_context * xc, dad_element_node * el, tcursnum & curs);
  bool occurrence_cond_satisfied(int cond_column);
};

struct t_table_info
{ ::t_ucd * ucdp;
  bool free_ucdp;
  bool is_base;
  const d_table * td;    // owning, must call release_table_d (for non-ODBC) or corefree (for ODBC)
  void destruct(void)
    { ucdp->release_table_d(td); }
};

SPECIF_DYNAR(tabinfo_dynar, t_table_info, 2);
SPECIF_DYNAR(t_record_space_dynar, t_record_space *, 4);
SPECIF_DYNAR(t_output_cursor_dynar, t_output_cursor *, 4);
SPECIF_DYNAR(dad_element_node_dynar, dad_element_node *, 10);

struct t_xml_context
{ enum { ELEM_STACK_SIZE=70 };
  t_xml_error err;
  tobjname error_message_param;
  void set_error_param(const char * param)
    { strmaxcpy(error_message_param, param, sizeof(error_message_param)); }
  int err_node_num;
  bool for_output;
  t_output_cursor_dynar output_cursor;
  int cnt_output_cursors;
  t_output_cursor * current_oc;  // not owning, used during export run-time
  t_dad_root * dad_tree;
  int table_count;                              // number of added tables in the tree
  const dad_element_node * last_table_element;
  dad_element_node_dynar element_stack;
  int element_stack_top;                        // points to the top element in the [element_stack], -1 if the stack is empty 
 // arrays indexd by the [table_index] (see the explatation of the "table_index" concept):
  tabinfo_dynar table_nums;   // NOOBJECT for the base cursor, then all table numbers as added in the elements 
  t_record_space_dynar rsp;   // used for inserting into the table on import (on recursion other temporary rsps are being created)
  t_base64 base64;
  int open_upper_table;
  ::t_ucd * ucdp;  // DAD object source
  int indent;
  t_outbuf * outbuf;  // not owning!
  int enc_code;       // XMLCODE_...
  bool sample;
  bool owning_top_cursor;
  tcursnum top_cursor;
  const d_table * top_td;
  ::t_ucd * top_ucdp;  // data source for the explicit SQL statement
  bool free_top_ucdp;
  const char * full_input;  // not owning
  unsigned full_input_length;

  t_xml_context(t_dad_root * dad_treeIn) : err(XMLE_OK) 
  { dad_tree = dad_treeIn;  open_upper_table=-1;  
    outbuf=NULL;  cnt_output_cursors=0;   last_table_element=NULL;
    sample=false;  owning_top_cursor=false;  top_cursor=NOOBJECT;  top_td=NULL;  
    top_ucdp=NULL;  free_top_ucdp=false;  full_input=NULL;
  }
  virtual ~t_xml_context(void);
  void map_oj(void);
  void output(void);
  void newline(void);
  virtual void analyze_tree(tcursnum incurs);
  bool export_to_xml(void);
  bool import_from_xml(const XERCES_CPP_NAMESPACE::InputSource & is);
  void close_top_cursor(void)
  { if (owning_top_cursor) if (top_cursor!=NOOBJECT) 
      { top_ucdp->close_cursor(top_cursor);  top_cursor=NOOBJECT;  owning_top_cursor=false; }
  }
};

enum t_insert_mode { IM_FIRSTCHILD, IM_LASTCHILD, IM_AFTER_SPECIF };


#define base_cursor_name wxT(" - base cursor - ")
#define base_cursor_name8 " - base cursor - "


//////////////////////////// internal functions ////////////////////////////////
// dadedit:
bool edit_dad(cdp_t cdp, HINSTANCE hInstance, HWND hParent, t_dad_root * dad, tobjnum objnum);
// dada:
void create_join_condition(t_flstr & cond, const char * lower_tab, const char * lower_cols, const char * upper_tab, const char * upper_cols);
char * LF2CRLF(char * inp);
void raise_error_message(t_ucd * ucdp, int num, const char * pattern, const char * param, const char * param2 = NULL, int line=-1, int col=-1);

char * duplicate_chars(const char * val);
void hostvars_from_params(t_dad_param * params, int parcnt, t_clivar *& hostvars, int & hc);
int wb2c_type(int type, t_specif specif);

#if 0
CFNC DllXMLExt char * WINAPI convert2utf8(char * system_def);
CFNC DllXMLExt char * WINAPI Merge_XML_form(cdp_t cdp,const char *xml_form_ref,const char *data);
CFNC DllXMLExt char * WINAPI Merge_XML_formM(cdp_t cdp,const char *xml_form_ref,int xml_form_ref_size,const char *data,int *result_size,char **error_message);
DllXMLExt t_xml_context * new_xml_context(t_dad_root * dad_treeIn);
CFNC DllXMLExt char * make_source(t_dad_root * dad_tree, bool alter, cdp_t object_cdp);
CFNC DllXMLExt t_ucd_cl_602 * new_t_ucd_cl_602(cdp_t cdpIn);
CFNC DllXMLExt t_ucd_odbc * new_t_ucd_cl_odbc(t_pconnection connIn, cdp_t reporting_cdpIn);
CFNC DllXMLExt void delete_t_ucd_cl(t_ucd_cl * cdcl);

CFNC DllXMLExt char * new_chars(int len);
CFNC DllXMLExt void delete_chars(char * str);
CFNC DllXMLExt BOOL WINAPI Get_DAD_params(cdp_t cdp, const char * dad, t_dad_param ** pars, int * count);
CFNC DllXMLExt BOOL WINAPI GetDadFromXMLForm(cdp_t cdp, tobjnum objnum, char *DADName, bool Inp = false);
#endif
          
#ifdef XMLEXT

bool XMLChcmp8(const XMLCh * str1, const char * str2);

class StrX
{
public :
    StrX(const XMLCh* const toTranscode)
    {   // Call the private transcoding method
        fLocalForm = XERCES_CPP_NAMESPACE::XMLString::transcode(toTranscode);
    }
    ~StrX()
      { XERCES_CPP_NAMESPACE::XMLString::release(&fLocalForm); }
    const char* localForm() const
      { return fLocalForm; }
private :
    char*   fLocalForm;  // This is the local code page form of the string.
};

int XMLChlen(const XMLCh * str);

class conv2to4
{ wchar_t * buf;
 public:
  conv2to4(const XMLCh * inp)
  { int len=0;
    while (inp[len]) len++;
    buf = new wchar_t[len+1];
    int i=0;
    while (inp[i])
      { buf[i]=inp[i];  i++; }
    buf[i]=0;  
  }
  operator const wchar_t *(void)
  { return buf; }
  ~conv2to4(void)
  { delete [] buf; }
};  

#ifdef LINUX

class conv4to2
{ XMLCh * buf;
 public:
  conv4to2(const wchar_t * inp)
  { int len=wcslen(inp);
    buf = new XMLCh[len+1];
    int i=0;
    while (inp[i])
      { buf[i]=inp[i];  i++; }
    buf[i]=0;  
  }
  operator const XMLCh *(void)
  { return buf; }
  ~conv4to2(void)
  { delete [] buf; }
};  

#undef  XMLStrL
#define XMLStrL(inp) conv4to2(L##inp)
#define XMLStrW(inp) conv4to2(inp)

#else

#define XMLStrW(inp) inp

#endif

#include "xmlext.h"

#else  // external users

#include "xmllnk.h"

#endif


