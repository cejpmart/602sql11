// recwin9.h
#ifdef XMLEXT
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/util/XMLString.hpp>
#else
namespace XERCES_CPP_NAMESPACE
{ class DOMNode;
  class InputSource;
}
#endif

struct t_clivar;
class t_record_window;
class t_record_space;
struct dad_element_node;
struct t_xml_context;
struct t_output_cursor;
struct t_ucd;

struct t_col_info
{ unsigned offset;
  unsigned size;
  t_specif specif;
  uns8     type;
};

class t_dad_loader
{protected: 
  char * loaded_dad;     // dad loaded form database or created for the query
  const char * dad_ref;  // dad reference specified on input
  tcursnum curs;         // cursor opened from the [dad_ref] query specification
 public:
  t_dad_loader(void)
    { loaded_dad=NULL;  curs=NOCURSOR; }
  virtual ~t_dad_loader(void) 
    { }

  virtual bool put_dad_ref(t_ucd * cdIn, const char * dad_refIn) = 0;
  const char * dad_ptr(void) const
    { return loaded_dad ? loaded_dad : dad_ref; }
  tcursnum local_cursor(void) const
    { return curs; }
};

enum t_ucd_type { UCL_SERVER, UCL_CLI_602, UCL_CLI_ODBC };

struct t_ucd
{ int time_zone_info;
  bool same(const t_ucd * other) const;
  virtual t_record_window * create_record_window(void) = 0;
  virtual t_record_space * make_record_space(int table_index) = 0;

  virtual void raise_error(int errnum) = 0;
  virtual void raise_generic_error(int errnum, const wchar_t * errmsg) = 0;
  virtual const d_table * get_table_d(tcateg categ, const char * table_name, const char * schema_name, tcursnum cursnum) = 0;
  virtual void release_table_d(const d_table * td) = 0; 
  virtual bool _find_object(const char * name, const char * schema, tcateg categ, tobjnum * objnum) = 0;
  virtual bool open_cursor(const char * query, tcursnum * curs) = 0;
  virtual void close_cursor(tcursnum curs) = 0;
  virtual int  current_error(void) = 0;
  virtual void start_transaction(void) = 0;
  virtual bool commit(void) = 0;  // false on error
  virtual void roll_back(void) = 0;
  virtual bool has_xml_licence(void) = 0;
  virtual int  get_system_charset(void) = 0;
  virtual void * get_native_variable_val(const char * variable_name, int & type, t_specif & specif, int & vallen, bool importing) = 0;
  // For strings, vallen is the available space including the space for terminator (1 or 2 bytes)
  virtual void append_native_variable_val(const char * variable_name, const void * wrval, int bytes, bool appending) = 0;
  virtual void * client_cdp(void) const = 0;
  virtual bool translate(const char * funct_name, wuchar * str, int bufsize) = 0;
  virtual t_ucd_type ucd_type(void) const = 0;
  virtual const char * quote(void) const = 0;   
  virtual t_dad_loader * get_dad_loader(const char * dad_ref) = 0;
  virtual char * load_object_utf8(tobjnum objnum) = 0;
  virtual void release_memory(void * ptr) = 0;
  virtual char * get_web_access_params(void) const = 0;
  virtual int nothing(void)
    { return 0; }
};

class t_record_window
{public:
  virtual const void * get_native_column_val(int column_num, trecnum rec_offset, int & type, t_specif & specif, int & length) = 0;
  virtual bool more_records(void) = 0;
  virtual bool start(t_ucd * ucdpIn, HANDLE incurs, bool close_the_cursor) = 0;
  virtual bool to_next_record(void) = 0;
  virtual bool load_next_valid_record(trecnum cache_pos) = 0;
  virtual ~t_record_window(void) { }
  virtual bool is_null_record     (int table_index_in_cursor, t_output_cursor * oc, int table_index) = 0;
  virtual bool same_record_numbers(int table_index_in_cursor, t_output_cursor * oc, int table_index) = 0;

  bool same_values(int column_num);
};

//SPECIF_DYNAR(t_colnum_dynar2, tattrib, 10);

class t_key_evaluator
{protected:
  char * buffer;
  bool prepared_ok;
  bool use_result_set;
 public:
  const char * get_buffer(void) const
    { return buffer; }
  virtual bool eval(void) = 0;
  virtual bool prepare(t_ucd * ucdp, const dad_element_node * el, int type, t_specif specif) = 0;
  t_key_evaluator(void)
    { buffer=NULL;  prepared_ok=false; }
  virtual ~t_key_evaluator(void)
    { corefree(buffer); }
};

class t_trigger_starter
{
  bool prepared_ok;
 public:
  virtual bool prepare(t_ucd * ucdp, const dad_element_node * el, int type, t_specif specif) = 0;
};

class t_record_space  // cache for 1 record in a table, used in XML import
{protected: 
  enum { MAX_KEY_COLS = 8 };
  trecnum current_extrec;  // valid only if [current_is_inserted]
  tattrib update_key_column_list[MAX_KEY_COLS+1];  // +1 for the terminator (server side)
  int update_key_column_cnt;
 public: 
  const int table_index;
  bool temporary;
  t_key_evaluator * key_evaluator;
  t_record_space(int table_indexIn) : table_index(table_indexIn)
    { temporary=false;  update_key_column_cnt=0;  key_evaluator=NULL; }
  virtual ~t_record_space(void) 
    { if (key_evaluator) delete key_evaluator; }
  t_atrmap_flex val_defined_map;  // cannot be used before prepare2() inits this!
  bool current_is_inserted;
  virtual void get_column_descr(int column_num, t_col_info * ci) const = 0;
  virtual void column_changed(int column_num) = 0;
  virtual bool prepare(t_ucd * ucdp, const dad_element_node * el) = 0;
  virtual bool prepare2(t_ucd * ucdp, const d_table * td, const dad_element_node * el) = 0;
  void starting_new_record(int colcnt);
  virtual bool start_new_record(void) = 0;
  virtual char * get_native_column_value(int column_num) = 0;
  virtual bool write_native_column_value(int column_num, const void * val, int vallen, bool appending) = 0;
  void record_inserted(trecnum erec);
  virtual bool insert(void) = 0;
  virtual bool read_column_value(int column_num) = 0;
  void add_string_value(int column_num, t_specif specif, char * dest, const void * val, int vallen);
  void throw_database_exception(int errnum);
  void add_update_key_column(tattrib column);
};

#ifdef putc
#undef putc
#endif

class t_outbuf
{protected:
  bool expand16;
 public:
  t_outbuf(void) : expand16(false) { }
  virtual void putn(const char * str, int n) = 0;
  virtual void putndir(const char * str, int n) = 0;
  void putc(char c)
    { putn(&c, 1); }
  virtual void put(const char * str)
    { putn(str, (int)strlen(str)); }
  void putbase64(const unsigned char * str, int size);
  void puthex(const unsigned char * str, int size);
  void expand_to_16(void)
    { expand16=true; }
};

class t_file_outbuf : public t_outbuf
{ char buffer[4096];
  int bufcont;
  bool file_error;
  FHANDLE hnd;
  bool save_buf(void);
 public:
  virtual bool open(const char * fname);
  virtual void close(void);
  bool has_error(void) const
    { return file_error; }
  void putn(const char * str, int n);
  void putndir(const char * str, int n);
};

class t_mem_outbuf : public t_outbuf
{
  char * bufptr;
  int bufspace;
  int bytes_written;
  bool overflow;
  bool expandable;
  char * bufstart;   // used only if [expandable]
  int act_bufsize;   // used only if [expandable]
 public:
  t_mem_outbuf(char * bufferIn, int bufsizeIn)
  { bytes_written=0;  overflow=false;  
    if (bufsizeIn<0) { bufspace=0;          bufptr=NULL;      expandable=true;  bufstart=NULL;  act_bufsize=0;  }
    else             { bufspace=bufsizeIn;  bufptr=bufferIn;  expandable=false; }
  }
  void putn(const char * str, int n);
  void putndir(const char * str, int n);
  virtual bool close(int * size)
  { putn("", 1);  // terminator
    *size=bytes_written;  return overflow; 
  }
  char * get_alloc_buf(int * bufsize)
    { if (bufsize) *bufsize=act_bufsize;  return bufstart; }
};

#define MAX_PARAM_VAL_LEN 100

struct t_dad_param
{ tobjname name;
  int type;
  t_specif specif;
  wchar_t val[MAX_PARAM_VAL_LEN+1];
  void set_default(void)
    { *name=0;  type=ATT_INT32;  specif.set_null();  *val=0; }
};

SPECIF_DYNAR(t_dad_param_dynar, t_dad_param, 1);

enum dad_node_types { DAD_NODE_ELEMENT, DAD_NODE_ATTRIBUTE, DAD_NODE_COLUMN, DAD_NODE_TOP };

struct dad_node
{private: 
  const dad_node_types node_type;
 public:
  int node_num;  // root has number 0, text_column counted if non-empty, info-column not counted
  dad_node(dad_node_types node_typeIn) : node_type(node_typeIn) { }
  virtual ~dad_node(void) { }
  virtual dad_node_types type(void) const 
    { return node_type; }
  virtual void get_caption(char * buf, bool close, int space) const = 0;
};

struct t_dad_root : public dad_node
{ int node_counter;
  dad_element_node * root_element;
  char * sql_stmt;
  char * dtdid;
  char * prolog;
  char * doctype;
  char * stylesheet;    
  char * nmsp_prefix;
  char * nmsp_name;
  char * ok_response;
  char * fail_response;
  tobjname dad2;
  tobjname form2;
  bool ignore_elems, ignore_text, no_validate, emit_whitespaces, disable_cursor_opt;
  tgenobjname top_dsn;
  t_dad_param_dynar params;
  bool explicit_sql_statement;
  int sys_charset;  // for converting variable names to the upper case when parsing the DAD
  bool empty(void) const;
  t_dad_root(int sys_charsetIn);
  ~t_dad_root(void);
  void get_caption(char * buf, bool close, int space) const;
  virtual int get_encoding_from_prolog(void);
  bool needs_full_source(void) const;
};

#define MAX_COLS_IN_JOIN_KEY 4
#define XML_NAME_MAX_LEN 127
typedef char t_dad_attribute_name[XML_NAME_MAX_LEN+1];
typedef char t_dad_element_name[XML_NAME_MAX_LEN+1];
typedef tobjname dad_table_name;
typedef char t_dad_col_name[ATTRNAMELEN+1];
#define KEY_DEF_SIZE 80
enum t_link_types { LT_COLUMN=0, LT_VAR=1, LT_CONST=2 };

struct t_dad_column_node : public dad_node
// In fact, it is not necessarily a column, but a general link: to a column or a variable or a constant.
{ t_dad_col_name column_name;  // or var name or constant
  dad_table_name table_name;   // valid for link_type==LT_COLUMN, empty for synthetic DAD
  t_link_types link_type;
  int column_number, cursor_number, // column number in the cursor and the cursor index, used by export
      table_index, column_number_in_table;       // used by import
  char * format, * fixed_val;
  tobjname inconv_fnc, outconv_fnc;
  bool is_update_key;  // Used by import. When true, the value id the key (or its part) for searching the existing record instead of inserting a new one

  t_dad_column_node(void) : dad_node(DAD_NODE_COLUMN)
    { *table_name=*column_name=0;  link_type=LT_COLUMN;  column_number=-1;  format=fixed_val=NULL; 
      *inconv_fnc=*outconv_fnc=0;  is_update_key=false;
    }
  ~t_dad_column_node(void) 
  { if (format) delete [] format; 
    if (fixed_val) delete [] fixed_val; 
  }
  t_dad_column_node(t_dad_column_node & orig);  // copy constructor
  //virtual t_dad_column_node & operator =(t_dad_column_node & orig);  // assignment operator - problems in GCC 4.1 with vtable
  virtual void copy_to(t_dad_column_node * dest);  // replaces assignment
  void convert_column_supernode(XERCES_CPP_NAMESPACE::DOMNode *n, t_dad_root * dad_root);
  void convert_column_node(XERCES_CPP_NAMESPACE::DOMNode *n, t_link_types link_typeIn);
  void convert_fixed_node(XERCES_CPP_NAMESPACE::DOMNode *n);
  void convert_RDB_node(XERCES_CPP_NAMESPACE::DOMNode *n);
  bool valid(t_xml_context * xc);
  void get_tables_and_conditions(t_xml_context * xc, t_output_cursor * oc, bool main_column);
  void output(t_xml_context * xc, bool attribute_value, bool inclusive_content);
  void get_caption(char * buf, bool close, int space) const;
  void to_source(t_flstr & src, int indent) const;
  void name_format_to_source(t_flstr & src, const char * el_name) const;
};

struct dad_attribute_node : public dad_node
{ t_dad_attribute_name attr_name;
  t_dad_column_node column;
  dad_attribute_node * next;
  char * attrcond;
  int attrcond_column;
  dad_attribute_node(void) : dad_node(DAD_NODE_ATTRIBUTE)
    { *attr_name=0;  next=NULL;  attrcond=NULL;  attrcond_column=-1; }
  ~dad_attribute_node(void) { }
  void get_tables_and_conditions(t_xml_context * xc, t_output_cursor * oc);
  void output(t_xml_context * xc);
  void get_caption(char * buf, bool close, int space) const;
  void to_source(t_flstr & src, int indent) const;
  virtual bool is_linked_to_column(void) const  // not linked when disabled in the FO->XSD->DAD conversion
    { return *column.column_name!=0 || column.fixed_val!=NULL && *column.fixed_val!=0; }
  bool needs_full_source(void) const;
};

enum t_key_generator_type { KG_NONE, KG_PRE, KG_POST };

struct dad_element_node : public dad_node
{ t_dad_element_name el_name;
  dad_attribute_node * attributes;  // list of attributes
  bool multi;
  t_dad_column_node text_column;  // column specified for the direct text
  dad_element_node * sub_elements;
  t_dad_column_node info_column;  // used for finding the proper element for repetition
  dad_element_node * next;
 // join data:
  bool new_cursor_here;  // new table is joined here, but not to the last table in the chain
  dad_table_name upper_tab,   // if alias is specified for the upper table, then [upper_tab] is the alias
                 lower_tab;   // real table name, not alias
  dad_table_name lower_alias; // specified alias or ""
  char dsn[MAX_OBJECT_NAME_LEN+1];
  char lower_schema[MAX_OBJECT_NAME_LEN+1];
  virtual const char * lower_table_name(void) const
    { return *lower_alias ? lower_alias : lower_tab; }
  int columns_in_join_key;  // 0 when not joined to the upper table
  char upper_col[KEY_DEF_SIZE+1], lower_col[KEY_DEF_SIZE+1];
  char * order_by;
  char * condition;
  char * elemcond;
  int upper_table_index,
      lower_table_index, // index to [table_nums] in table-related nodes (incl. base), -1 otherwise
      upper_column_in_curs[MAX_COLS_IN_JOIN_KEY],
      lower_column_number_in_table[MAX_COLS_IN_JOIN_KEY],
      upper_column_number_in_table[MAX_COLS_IN_JOIN_KEY];
  int lower_cursor_num, upper_cursor_num;
  int lower_table_index_in_cursor;  // for output: number of table in its cursor, used for checing for OJ NULL
  int upper_table_index_in_cursor;  // defined in join nodes linked to a table in the same cursor
  const dad_element_node * prev_table_element;
  int elemcond_column;
 // auxuliary data:
  dad_element_node * multi_subel;  // not owning!
  t_atrmap_flex referenced_columns; // all referenced columns except for these in the subtree in the repetition direction
  // Valid for elements on the repetition path:
  // Contains references: 1. local, 2. above, 3. below except for the repetition path
  // Used only for output
  dad_element_node * multi_inserting_record;  // not owning, used only on input
  bool redirected;                            // used only on input
  bool recursive;
  bool has_recursive_subelement;
  bool inclusive;                             // all subcontents stored locally
 // data for generating the ID:
  char id_col[ATTRNAMELEN+1];
  int id_column_number_in_table;
  t_key_generator_type key_generator;
  char * key_generator_expression;

  dad_element_node(void) : dad_node(DAD_NODE_ELEMENT)
  { *el_name=0;
    attributes=NULL;  sub_elements=NULL;  next=NULL;  order_by=NULL;  condition=NULL;  elemcond=NULL;  key_generator_expression=NULL;
    multi=false;  multi_subel=NULL;
    *upper_tab=*lower_tab=*lower_alias=*upper_col=*lower_col=*dsn=*lower_schema=*id_col=0;  
    lower_table_index=-1;  elemcond_column=-1;
    new_cursor_here=false;  lower_cursor_num=upper_cursor_num=-1;
    lower_table_index_in_cursor=-1;  prev_table_element=NULL;
    upper_table_index_in_cursor=-1;
    multi_inserting_record=NULL;  redirected=recursive=has_recursive_subelement=inclusive=false;
    id_column_number_in_table=-1;
    key_generator=KG_NONE;
  }
  ~dad_element_node(void);
  void map_columns(t_xml_context * xc);
  void get_tables_and_conditions(t_xml_context * xc);
  void output(t_xml_context * xc);
  void output_sample(t_xml_context * xc);
  virtual bool is_related_to_table(void) const
    { return lower_table_index!=-1; }
  inline bool related_to_top_table(void) const
    { return lower_table_index==0; }
  void get_caption(char * buf, bool close, int space) const;
  void to_source(t_flstr & src, int indent, cdp_t object_cdp) const;
  virtual bool is_linked_to_column(void) const
    { return *text_column.column_name!=0 || text_column.fixed_val!=NULL && *text_column.fixed_val!=0; }
  virtual bool has_info_column(void) const
    { return *info_column.column_name!=0; }
  void find_recursion(t_xml_context * xc);
  bool needs_full_source(void) const;
};


void delete_dad_tree(t_dad_root * dad_tree);
char * make_dad_from_td(const d_table * td, int charset, const char * query, int style);
void release_hostvars(t_clivar * hostvars, int hostvars_count);

#if 0
CFNC DllXMLExt bool init_platform_utils(t_ucd * ucdp);
CFNC DllXMLExt void close_platform_utils(void);

CFNC DllXMLExt t_dad_root * parse(t_ucd * ucdp, const char * source, int * pline = NULL, int * pcolumn = NULL);
CFNC DllXMLExt BOOL export_to_file(t_ucd * ucdp, const char * dad, t_file_outbuf & fo, const char * fname, tcursnum curs, int * error_node);
CFNC DllXMLExt BOOL export_to_memory(t_ucd * ucdp, const char * dad, t_mem_outbuf & fo, tcursnum curs, int * error_node);
CFNC DllXMLExt BOOL import_from_file(t_ucd * ucdp, const char * dad, const char * fname, int * error_node, char ** out_buffer);
CFNC DllXMLExt BOOL import_from_memory(t_ucd * ucdp, const char * dad, const char * buffer, int xmlsize, int * error_node, char ** out_buffer);
DllXMLExt dad_element_node * new_dad_element_node(void);
DllXMLExt dad_attribute_node * new_dad_attribute_node(void);
DllXMLExt t_dad_root * new_t_dad_root(int sys_charsetIn);
DllXMLExt t_dad_column_node * new_t_dad_column_node(t_dad_column_node & orig);
#endif
//////////////////////////////// xsd... + dsparser /////////////////////////////////////
// tpes used in xmlext.h:
enum DSPProp 
{
    DSP_SCHEMANAME,
    DSP_FOLDERNAME,
    DSP_DADNAME,
    DSP_TABLEPREFIX,
    DSP_DEFSTRLENGTH,
    DSP_UNICODE,
    DSP_IDTYPE,
    DSP_ERRCODE,
    DSP_ERRSOURCE,
    DSP_ERRMSG,
    DSP_ERRTYPEXML,
    DSP_ERRFILE,
    DSP_ERRLINE,
    DSP_ERRCOL,
    DSP_XMLFORMSOURCE,
    DSP_REFINT,
    DSP_XSD,
    DSP_ROOTTABLE,
    DSP_ROOTTABLEKEY,
    DSP_ROOTTABLESEQ,
    DSP_FORMID,
    DSP_FORMDATAROOTNAME,
    DSP_FORMDATANSPREFIX,
    DSP_FORMDATANSURI,
    DSP_SPECROOTSEQ
};

enum DSPErrSource
{
    DSPE_NONE,
    DSPE_SYSTEM,
    DSPE_SQL,
    DSPE_XML,
    DSPE_DOM,
    DSPE_SAX,
    DSPE_XSD,
    DSPE_ZIP,
    DSPE_OTHER
};
//
// XML Form data model and DAD comparison result
//
enum MFDState
{
    MFD_AGR,            // Agrees 
    MFD_DISAGR,         // Disagrees
    MFD_FORMREADFAIL,   // Form read error
    MFD_FORMPARSEFAIL,  // Form parse error
    MFD_DADREADFAIL,    // DAD read error
    MFD_DADPARSEFAIL,   // DAD parse error
    MFD_OTHER,          // Other error
    MFD_DISAGRNAME      // Difference if the name
};

typedef void (*LPERRCALLBACK)(const void *Owner, int ErrCode, DSPErrSource ErrSource, const wchar_t *ErrMsg);  

