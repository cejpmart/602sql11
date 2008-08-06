// ftconv.cpp
#include <limits.h>

/* t_ftx_error_reporter {{{
 * common ancestor of t_input_stream and t_input_convertor classes
 * implements error reporting */
class t_ftx_error_reporter
{
 protected:
  bool any_error;
  cdp_t cdp;
  void set_any_error()
    { any_error=true; }
  void request_error(const char *errmsg);
  void request_errorf(const char *buf,...);
  void request_error_out_of_memory(const char *reason=NULL);
 public:
  t_ftx_error_reporter(cdp_t cdpIn):cdp(cdpIn),any_error(false) {}
  virtual ~t_ftx_error_reporter() {}
  bool was_any_error(void) const
    { return any_error; }
};
void t_ftx_error_reporter::request_error(const char *errmsg) /*{{{*/
{ set_any_error();  // common for all errors
#ifdef SRVR
  request_generic_error(cdp,GENERR_FULLTEXT_CONV,errmsg);  // must be warning, error is wrong when generating context etc.
#else
  client_error_generic(cdp,GENERR_FULLTEXT_CONV,errmsg);
#endif
} /*}}}*/
void t_ftx_error_reporter::request_errorf(const char *buf,...) /*{{{*/
{
  char outbuf[4096];
  va_list ap;
  va_start(ap,buf);
  vsnprintf(outbuf,sizeof(outbuf),buf,ap);
  outbuf[sizeof(outbuf)-1]=0;  // on overflow the terminator is NOT written!
  va_end(ap);
  request_error(outbuf);
} /*}}}*/
void t_ftx_error_reporter::request_error_out_of_memory(const char *reason) /*{{{*/
{
  if( reason!=NULL ) request_errorf("Fulltext: Out of memory (%s)",reason);
  else request_error("Fulltext: Out of memory");
} /*}}}*/
/*}}}*/

/* t_input_stream and descendants {{{*/
class t_input_stream:public t_ftx_error_reporter
{
 public:
  t_input_stream(cdp_t cdpIn) : t_ftx_error_reporter(cdpIn) { }
  virtual void reset(void) = 0;
  virtual void reset_to(int position) = 0;
  virtual int  read(char * buf, int size) = 0;
  virtual ~t_input_stream(void) { }
  /* returns filename that will be used in LIMITS evaluation or NULL */
  virtual const char *get_filename_for_limits() const { return NULL; }
  /* returns real filename of the file with document or NULL */
  virtual const char *get_document_filename() const { return get_filename_for_limits(); }
  virtual void request_error_ReadFile(const char *param_filename,const char *param_base_document_filename=NULL);
};
void t_input_stream::request_error_ReadFile(const char *param_filename,const char *param_base_document_filename)
{
#ifdef WINS
  char buf[4000];
  sprintf(buf,"failed to read document file, code=%d",GetLastError());
  if( param_filename!=NULL )
  {
    strcat(buf,", filename=");
    strcat(buf,param_filename);
  }
  if( param_base_document_filename!=NULL )
  {
    strcat(buf,", base filename=");
    strcat(buf,param_base_document_filename);
  }
  request_error(buf);
#else
  request_error("failed to read document file");
#endif
} 

class t_input_stream_direct : public t_input_stream
{ char * data;
  int length;
  int pos;
 public:
  virtual void reset(void)
    { pos=0; }
  virtual void reset_to(int position)
    { if( position<0 ) position=0;
      if( position>=length ) position=length;
      pos=position;
    }
  virtual int read(char * buf, int size)
    { int rest = length-pos;
      if (size>rest) size=rest;
      memcpy(buf, data+pos, size);
      pos+=size;
      return size;
    }
  t_input_stream_direct(cdp_t cdpIn, char * dataIn, int lengthIn):t_input_stream(cdpIn)
  { data=dataIn;  length=lengthIn;  
    reset();
  }
};

#ifdef SRVR
class t_input_stream_db : public t_input_stream
{ ttablenum tb;
  trecnum recnum;
  tattrib attrib;
  uns16 index;
  table_descr * tbdf;
  int pos;
  bool is_character_stream;
 public:
  virtual void reset(void)
    { pos=0; }
  virtual void reset_to(int position)
    { if( position<0 ) position=0;
      pos=position;
    }
  virtual int read(char * buf, int size)
  { if (index!=NOINDEX)
    { if (tb_read_ind_var(cdp, tbdf, recnum, attrib, index, pos, size, buf)) 
      return 0;
    }
    else
    { if (tb_read_var(cdp, tbdf, recnum, attrib, pos, size, buf)) 
        return 0;
    }
    unsigned rd = cdp->tb_read_var_result;
    pos+=rd;
    if (is_character_stream)  // stop on the 1st 0
    { void * endp = memchr(buf, 0, rd);
      if (endp)
        return (int)((char*)endp - buf);
    }
    return rd;
  }
  t_input_stream_db(cdp_t cdpIn, ttablenum tbIn, trecnum recIn, tattrib attrIn, uns16 indexIn):t_input_stream(cdpIn)
  { tb=tbIn;  recnum=recIn;  attrib=attrIn;  index=indexIn;
    tbdf=install_table(cdp, tb);
    is_character_stream = tbdf->attrs[attrib].attrtype==ATT_TEXT || tbdf->attrs[attrib].attrtype==ATT_STRING;
    reset();
  }
  ~t_input_stream_db(void)
    { unlock_tabdescr(tbdf); }
};
#else // !SRVR
class t_input_stream_db : public t_input_stream
{ ttablenum tb;
  trecnum recnum;
  tattrib attrib;
  uns16 index;
  int pos;
  bool is_character_stream;
 public:
  virtual void reset(void)
    { pos=0; }
  virtual void reset_to(int position)
    { if( position<0 ) position=0;
      pos=position;
    }
  virtual int read(char * buf, int size)
  { t_varcol_size rd;
    if (cd_Read_var(cdp, tb, recnum, attrib, index, pos, size, buf, &rd))
      return 0;
    pos+=rd;
    if (is_character_stream)  // stop on the 1st 0
    { void * endp = memchr(buf, 0, rd);
      if (endp)
        return (int)((char*)endp - buf);
    }
    return rd;
  }
  t_input_stream_db(cdp_t cdpIn, ttablenum tbIn, trecnum recIn, tattrib attrIn, uns16 indexIn):t_input_stream(cdpIn)
  { tb=tbIn;  recnum=recIn;  attrib=attrIn;  index=indexIn;
    const d_table * td = cd_get_table_d(cdp, tb, CATEG_TABLE);
    if (td)
    { int tp = ATTR_N(td, attrib)->type;
      is_character_stream = tp==ATT_TEXT || tp==ATT_STRING;
      release_table_d(td);
    }
    reset();
  }
};
#endif

class t_input_stream_file : public t_input_stream
{ FHANDLE fhandle;
  char *filename_for_limits,*filename,*base_document_filename;
  /* saves filename for LIMITS evaluation into filename_for_limits
   * stores just filename and extension part of (possibly full path name) param_filename_for_limits
   * returns true on success or false on error (out of memory) */
  bool store_filename_for_limits(const char *param_filename_for_limits);
 public:
  virtual void reset(void)
    { SetFilePointer(fhandle, 0, NULL, FILE_BEGIN); }
  virtual void reset_to(int position)
    { SetFilePointer(fhandle,position,NULL,FILE_BEGIN); }
  virtual int read(char * buf, int size)
  { DWORD rd;
    if (fhandle==INVALID_FHANDLE_VALUE) return 0;
    if (!ReadFile(fhandle, buf, size, &rd, NULL)) { request_error_ReadFile(filename,base_document_filename); return 0; }
    return rd;
  }
  t_input_stream_file(cdp_t cdpIn,const char *param_filename,const char *param_filename_for_limits=NULL,const char *param_base_document_filename=NULL):t_input_stream(cdpIn)
  { fhandle=INVALID_FHANDLE_VALUE; base_document_filename=NULL; filename_for_limits=NULL;
    filename=new char[strlen(param_filename)+1];
    if( filename==NULL ) { request_error_out_of_memory(); return; }
    strcpy(filename,param_filename);
    if( param_base_document_filename!=NULL )
    {
      base_document_filename=new char[strlen(param_base_document_filename)+1];
      if( base_document_filename==NULL ) { request_error_out_of_memory(); return; }
      strcpy(base_document_filename,param_base_document_filename);
    }
    fhandle = CreateFile(param_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (fhandle==INVALID_FHANDLE_VALUE) request_errorf("failed to open %s file",param_filename);
    filename_for_limits=NULL;
    if( param_filename_for_limits!=NULL ) store_filename_for_limits(param_filename_for_limits);
  }
  ~t_input_stream_file(void)
  {
    if (fhandle!=INVALID_FHANDLE_VALUE) CloseFile(fhandle);
    if( filename_for_limits!=NULL ) delete[] filename_for_limits;
    if( filename!=NULL ) delete[] filename;
  }
  virtual const char *get_filename_for_limits() const { return filename_for_limits; }
  virtual const char *get_document_filename() const { return (base_document_filename==NULL)?filename_for_limits:base_document_filename; }
};
bool t_input_stream_file::store_filename_for_limits(const char *param_filename_for_limits)
{
  if( filename_for_limits!=NULL ) { delete[] filename_for_limits; filename_for_limits=NULL; }
  if( param_filename_for_limits==NULL || *param_filename_for_limits=='\0' ) return true;
  const int path_delimiter=
#ifdef WINS
    '\\'
#else
    '/'
#endif
    ;
  const char *delim_ptr=strrchr(param_filename_for_limits,path_delimiter);
  if( delim_ptr==NULL ) delim_ptr=param_filename_for_limits;
  else delim_ptr++;
  // delim_ptr now points to the start of filename.ext
  filename_for_limits=new char[strlen(delim_ptr)+1];
  if( filename_for_limits==NULL ) return false;
  strcpy(filename_for_limits,delim_ptr);
  return true;
}

class t_input_stream_pipe:public t_input_stream
{ FILE *pipe;
 public:
  virtual void reset(void)
  {
    fseek(pipe,0,SEEK_SET);
  };
  virtual void reset_to(int position)
  {
    fseek(pipe,position,SEEK_SET);
  };
  virtual int  read(char * buf, int size)
  {
    return (int)fread(buf,(int)sizeof(char),size,pipe);
  };
  t_input_stream_pipe(cdp_t cdpIn,FILE *_pipe):t_input_stream(cdpIn) { pipe=_pipe; };
  virtual ~t_input_stream_pipe(void) { };
};

#ifdef WINS
class t_input_stream_HFILE:public t_input_stream
{ HANDLE hFile;
 public:
  virtual void reset(void)
  {
    SetFilePointer(hFile,0,NULL,FILE_BEGIN);
  };
  virtual void reset_to(int position)
  {
    SetFilePointer(hFile,position,NULL,FILE_BEGIN);
  };
  virtual int  read(char * buf, int size);
  t_input_stream_HFILE(cdp_t cdpIn,HANDLE _hFile):t_input_stream(cdpIn) { hFile=_hFile; };
  virtual ~t_input_stream_HFILE(void) { };
};
int t_input_stream_HFILE::read(char * buf, int size)
{
  DWORD dwRead;
  if( ReadFile(hFile,buf,size,&dwRead,NULL)==0 )
  {
    // if the error was ERROR_BROKEN_PIPE then it means that end of the pipe was encountered
    if( GetLastError()!=ERROR_BROKEN_PIPE )
    {
      request_error_ReadFile(NULL,NULL);
      return 0;
    }
  }
  return (int)dwRead;
}

#endif
/*}}}*/

/* #define USE_EML_CONVERTOR
 * Enables/disables t_input_convertor_eml and parts of t_format_guesser
 * which deal with EML document recognition. */
#ifdef WINS
#define USE_EML_CONVERTOR
#endif
/* $define USE_XSLFO_CONVERTOR
 * Enables/disables t_input_convertor_ext_xslfo and parts of t_format_guesser
 * that deal with XSL-FO document recognition. */
#ifdef WINS
#define USE_XSLFO_CONVERTOR
#endif

/* class t_input_convertor {{{*/
const uns8 max_depth_level=10;
const size_t max_nonword_length=100;
#ifdef WINS
// time period in milliseconds to test if the execution of the fulltext convertor was cancelled by the user - see t_input_convertor::was_breaked()
const DWORD ic_break_test_period_ms=1000;
#endif
class t_input_convertor:public t_ftx_error_reporter
{protected:
  t_input_stream * is;
  t_agent_base * agent_data;
  const char *subdocument_prefix;
  t_ft_limits_evaluator *limits_evaluator;
  const t_ft_kontext_base * working_for;  // access to word delimiter macroc etc
  uns8 depth_level; /* states how deep is nested this document, i.e.
                       original document which is being indexed has depth_level=1
                       if original document is for example zip file then zipped files have depth_level=2
                       if zipped file is zip file then its zipped files have depth_level=3
                       etc. */
  t_specif ft_specif;  // copied from [agent_data]
  void request_error_invalid_format(const char *name_of_format,const char *reason=NULL);
  /* stores relative path of this subdocument into subdocument_relpath
   * returns true on success, false on error
   * subdocument is relpath of this subdocument in parent document */
  bool make_subdocument_relpath(const char *subdocument,t_flstr &subdocument_relpath);
  /* tests if depth_level has reached max_depth_level
   * returns false if depth_level>max_depth_level and logs error (and caller should break document indexation with error)
   * otherwise returns true (and caller can continue with indexation */
  bool is_depth_level_allowed();
  /* tests LIMITS constraint for file whose filename and number of words are passed as parameters
   * returns true on success (and in result parameter returns result of test - true if document
   * have to be indexed or false if document doesn't have to be indexed)
   * returns false in case of error */
  bool test_limits_for_file(const char *filename,int wordcount,bool *result);
  /* returns true if the work was breaked by the client, otherwise returns false */
  inline bool was_breaked()
  {
#ifdef SRVR
    return (working_for==NULL)?false:working_for->breaked(cdp);
#else
    return false;
#endif
  }
 public:
  t_input_convertor(cdp_t cdp_param):t_ftx_error_reporter(cdp_param)
    { is=NULL; agent_data=NULL; subdocument_prefix=NULL; limits_evaluator=NULL; depth_level=0; };
  virtual ~t_input_convertor() {};
  void set_stream(t_input_stream * isIn)
    { is=isIn; }
  void set_owning_ft(const t_ft_kontext_base * working_forIn)
    { working_for=working_forIn; }
  void set_agent(t_agent_base * agent_dataIn)
    { agent_data=agent_dataIn;  
      ft_specif=agent_data->get_ft_specif();  ft_specif.length=0;  // must not limit the length when converting the input buffer!
    }
  virtual void set_specif(t_specif specif) { }  // stores the external information about the charset in the text
  void set_subdocument_prefix(const char *subdoc_prefix) { subdocument_prefix=subdoc_prefix; }
  void set_depth_level(uns8 _depth_level) { depth_level=_depth_level; }
  void set_limits_evaluator(t_ft_limits_evaluator *limits_evaluatorIn) { limits_evaluator=limits_evaluatorIn; }
  virtual bool convert(void) = 0;
   // Used when creating a sub-convertor with the same parameters as the owning convertor 
  void copy_parameters(const t_input_convertor * pattern)
    { set_agent         (pattern->agent_data);  // defines [ft_specif], too
      limits_evaluator = pattern->limits_evaluator;
      working_for      = pattern->working_for; 
      depth_level      = pattern->depth_level+1;
    }
};
void t_input_convertor::request_error_invalid_format(const char *name_of_format,const char *reason) /*{{{*/
{
  if( reason==NULL )
  {
    request_errorf("Fulltext: Invalid format of %s document.",name_of_format);
  }
  else
  {
    request_errorf("Fulltext: Invalid format of %s document, reason: %s.",name_of_format,reason);
  }
} /*}}}*/
bool t_input_convertor::make_subdocument_relpath(const char *subdocument,t_flstr &subdocument_relpath) /*{{{*/
{
  if( subdocument_prefix!=NULL )
  {
    subdocument_relpath.put(subdocument_prefix);
    subdocument_relpath.putc('/');
  }
  subdocument_relpath.put(subdocument);
  subdocument_relpath.putc('\0');
  return true;
} /*}}}*/
bool t_input_convertor::is_depth_level_allowed() /*{{{*/
{
  if( depth_level>max_depth_level )
  {
    request_error("602SQL: can't index this document because it's too deep nested");
    return false;
  }
  else return true;
} /*}}}*/
bool t_input_convertor::test_limits_for_file(const char *filename,int wordcount,bool *result) /*{{{*/
{
  if( limits_evaluator==NULL ) { *result=true; return true; } // empty LIMITS is satisfied by all files
#ifdef SRVR
  return limits_evaluator->test_file(filename,wordcount,sys_spec.charset,result);
#else
  return limits_evaluator->test_file(filename,wordcount,cdp->sys_charset,result);
#endif  
} /*}}}*/
/*}}}*/

/* definition of the class t_format_guesser {{{*/
typedef enum format_type_t { 
  FORMAT_TYPE_VOID, // undefined, not yet detected
  FORMAT_TYPE_BINARY,
  FORMAT_TYPE_TEXT_7BIT, // 7-bit ASCII
  FORMAT_TYPE_TEXT_8BIT_ISO, // ANY ISO-8859-* !!!
  FORMAT_TYPE_TEXT_8BIT_WIN1250,
  FORMAT_TYPE_TEXT_8BIT_WIN1252,
  FORMAT_TYPE_TEXT_UTF8,
  FORMAT_TYPE_TEXT_UCS2
} format_type_t;

class t_format_guesser
{
  cdp_t cdp;
  t_input_stream *is;
  format_type_t format_type;

  /* detects format type (i.e. whether document is binary or text)
   * returns true on success and sets format_type field
   *      or false on error
   * input stream read pointer of is will be located anywhere in stream
   * therefore caller should reset() input stream after the call of this method */
  bool internal_determine_format_type();
  /* performs tests based on content of the beginning of the document
   * assumes that buffer[] contains the beginning of the document
   * doesn't read any other part of the document
   * returns apropriate new-alloced t_input_convertor descendant on success
   *      or NULL when tests weren't successfull */
  t_input_convertor *guess_format_early();
  /* guesses format of OLE storage
   * returns apropriate new-alloced t_input_convertor descendant on success
   *      or NULL on failure (error or unknown/unsupported format)
   * assumes that 'is' input stream is at the beginning (more precisely
   * internal pointer of input stream is points after the beginning of the stream
   * which is already in buffer) and that buffer contains first part of stream
   * status of 'is' and 'buffer' is undeterminable, the caller have to execute
   *   is->reset();
   *   initialize_buffer();
   * before further usage of 'is' */
  t_input_convertor *guess_format_for_OLE_storage();

  /* helper methods that simplify input stream access */
  enum { buffer_size=2048 }; // have to be even number to ensure that whole UCS-2 characters are read
  char buffer[buffer_size+1];
  int current_index,
    buffer_length,
    offset;
  bool eof;
  void initialize_buffer();
  /* Reads next part of input stream contents into buffer. Ensures that
   * last overlapped_length chars from the end of previous buffer will be
   * copied into the start of the new buffer.
   * Returns true on success or false on error or when the whole document is already read. */
  bool populate_buffer_with_next_doc_part(int overlapped_length=0);
  int getc() const { return (eof)?EOF:(uns8)buffer[current_index]; };
  int fwd(int count=1);

  /* helper methods to detect character code pages */
  inline bool is_7bit_white_space(int c) const { return c==' ' || c=='\n' || c=='\r' || c=='\t' || c=='\v' || c=='\f'; }
  inline bool is_7bit_char(int c) const { return (c>=32 && c<=126) || is_7bit_white_space(c); }
  // invalid characters between 0x80 and 0xff in ISO-8859-* are from 0x80 to 0x9f
  inline bool is_8bit_iso_char(int c) const { return is_7bit_char(c) || c>=0xa0; };
  // these are invalid characters in Windows-1250 greater than 0x7f
  inline bool is_8bit_win1250_char(int c) const { return is_7bit_char(c) || (c>=0x80 && !(c==0x80 || c==0x81 || c==0x83 || c==0x88 || c==0x90 || c==0x98)); };
  // see http://www.microsoft.com/globaldev/reference/sbcs/1252.htm
  inline bool is_8bit_win1252_char(int c) const { return is_7bit_char(c) || (c>=0x80 && !(c==0x81 || c==0x8d || c==0x8f || c==0x90 || c==0x9d)); };

  /* helper methods for early tests */
  // returns true if pBuf points to OLE storage magic fingerprint
  // otherwise returns false
  bool is_OLE_storage_magic(const char *pBuf) const;
  // returns pointer to '<' char of first occurence of tag 'tag' in string 'str'
  // tag can be '<' white-space* tag-name white-space* attributes* '>'
  // without_attributes indicates whether we allow presence of attributes in tag
  // 'str' have to be null-terminated
  // returns NULL if 'tag' isn't present in 'str'
  const char *strtag(const char *str,const char *tag,const bool without_attributes);
#ifdef USE_EML_CONVERTOR
  // returns true if stream is EML otherwise returns false
  // uses content of buffer[] only
  bool is_eml_format();
  inline bool is_eml_header_id_char(int c) const { return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='-'||(c>='0'&&c<='9'); } // FIXME is it all?
#endif

  // ststr() for binary buffers
  const void *memmem(const void *s1,size_t s1len,const void *s2,size_t s2len) const;
public:
  t_format_guesser(cdp_t cdp_param,t_input_stream *_is);
  /* guesses format of input stream is
   * returns apropriate new-allocated input convertor or NULL */
  t_input_convertor *guess_format();
  /* determines format type of the input stream
   * returns FORMAT_TYPE_xxx constant or FORMAT_TYPE_VOID in case of error */
  format_type_t determine_format_type();

  /* Following static methods create new-allocated instances
   * of convertors for all supported document formats.
   * These methods decide which convertor will be used by examining
   * convertor_params object. */
  static t_input_convertor *new_get_text_convertor(cdp_t cdp_param);
  static t_input_convertor *new_get_html_convertor(cdp_t cdp_param);
  static t_input_convertor *new_get_xml_convertor(cdp_t cdp_param);
  static t_input_convertor *new_get_pdf_convertor(cdp_t cdp_param);
  static t_input_convertor *new_get_zip_convertor(cdp_t cdp_param);
  static t_input_convertor *new_get_ooo_convertor(cdp_t cdp_param);
  static t_input_convertor *new_get_rtf_convertor(cdp_t cdp_param);
  static t_input_convertor *new_get_doc_convertor(cdp_t cdp_param);
  static t_input_convertor *new_get_xls_convertor(cdp_t cdp_param);
  static t_input_convertor *new_get_ppt_convertor(cdp_t cdp_param);
#ifdef USE_EML_CONVERTOR
  static t_input_convertor *new_get_eml_convertor(cdp_t cdp_param);
#endif
#ifdef USE_XSLFO_CONVERTOR
  static t_input_convertor *new_get_xslfo_convertor(cdp_t cdp_param);
#endif
  // returns convertor that utilizes OOo
  static t_input_convertor *new_get_convertor_using_ooo(cdp_t cdp_param,const char *infile_ext);
};
/*}}}*/

static t_input_convertor * ic_from_text(cdp_t cdp_param,t_input_stream *is);

/* class t_convertor_params {{{*/
enum ftx_convertors_t {
  ANTIWORD,
  CATDOC,
  XLS2CSV,
  CATPPT,
  OPENOFFICE_ORG,
  RTF2HTML,
};

// include names of regdb keys and values
#include <ftxhlpkeys.h>

/* Contains parameters which affect document conversion.
 * Only one instance exists - convertor_params
 * Parameters are loaded in constructor. */
enum ooo_usage_t { OOO_NONE, OOO_STANDALONE, OOO_SERVER };
enum { COREALLOC_OWNER_ID=0xe5 }; 

class t_convertor_params
{
protected:
  ooo_usage_t use_ooo_convertor; // whether we should use OpenOffice.org for document conversion
  char *soffice_binary; // location of soffice[.exe] binary - valid for use_ooo_convertor==OOO_STANDALONE only
                        // points to malloc-ed buffer
  char *ooo_hostname,*ooo_port; // OOo hostname and port - valid for use_ooo_convertor==OOO_SERVER only
                                // points to malloc-ed buffer
  char *ooo_program_dir; // OOo 'program' directory - valid for use_ooo_convertor==OOO_SERVER only
                         // points to malloc-ed buffer
  ftx_convertors_t for_doc,for_xls,for_ppt,for_rtf;

#ifdef WINS
  char *GetmallocedRegDbValue(HKEY hKey,const char *valuename);
#else
  char *GetmallocedINIValue(const char *cfgfile,const char *section,const char *key);
#endif
public:
  t_convertor_params();
  ~t_convertor_params();
  // Get_xxx methods are thread-safe readers of parameters
  inline const bool Get_use_ooo_convertor() const { return use_ooo_convertor!=OOO_NONE; }
  inline const ooo_usage_t Get_ooo_usage_type() const { return use_ooo_convertor; }
  inline const char *Get_soffice_binary() const { return soffice_binary; }
  inline const char *Get_ooo_hostname() const { return (ooo_hostname!=NULL)?ooo_hostname:"localhost"; }
  inline const char *Get_ooo_port() const { return (ooo_port!=NULL)?ooo_port:"8100"; }
  inline const char *Get_ooo_program_dir() const { return ooo_program_dir; }
  inline const ftx_convertors_t Get_for_doc() const { return for_doc; }
  inline const ftx_convertors_t Get_for_xls() const { return for_xls; }
  inline const ftx_convertors_t Get_for_ppt() const { return for_ppt; }
  inline const ftx_convertors_t Get_for_rtf() const { return for_rtf; }
};
t_convertor_params::t_convertor_params() /*{{{*/
{ // set default values
  use_ooo_convertor=OOO_NONE;
  soffice_binary=ooo_hostname=ooo_port=ooo_program_dir=NULL;
  for_doc=CATDOC;
  for_xls=XLS2CSV;
  for_ppt=CATPPT;
  for_rtf=RTF2HTML;
#ifdef WINS
  char subkey[300];
  strcpy(subkey,WB_inst_key);
  strcat(subkey,"\\"); strcat(subkey,FTXHLP_KEY);
  HKEY hKey;
  if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,subkey,0,KEY_READ_EX,&hKey)==ERROR_SUCCESS )
  {
    DWORD value,valuesize=sizeof(DWORD);
    if( RegQueryValueEx(hKey,FTXHLP_VALNAME_USEOOO,NULL,NULL,(LPBYTE)&value,&valuesize)==ERROR_SUCCESS )
    {
      switch( value )
      {
        case 1:
          soffice_binary=GetmallocedRegDbValue(hKey,FTXHLP_VALNAME_OOOBINARY);
          if( soffice_binary!=NULL ) use_ooo_convertor=OOO_STANDALONE;
          break;
        case 2:
          ooo_hostname=GetmallocedRegDbValue(hKey,FTXHLP_VALNAME_OOOHOSTNAME);
          ooo_port=GetmallocedRegDbValue(hKey,FTXHLP_VALNAME_OOOPORT);
          ooo_program_dir=GetmallocedRegDbValue(hKey,FTXHLP_VALNAME_OOOPROGRAMDIR);
          if( ooo_program_dir!=NULL ) use_ooo_convertor=OOO_SERVER;
          break;
      }
    }
    char buffer[100]; valuesize=sizeof(buffer);
    if( RegQueryValueEx(hKey,FTXHLP_VALNAME_DOC,NULL,NULL,(LPBYTE)buffer,&valuesize)==ERROR_SUCCESS )
    {
      if( stricmp(buffer,FTXHLP_HELPER_ANTIWORD)==0 ) for_doc=ANTIWORD;
      else if( stricmp(buffer,FTXHLP_HELPER_CATDOC)==0 ) for_doc=CATDOC;
      else if( stricmp(buffer,FTXHLP_HELPER_OOO)==0 || stricmp(buffer,FTXHLP_HELPER_OOO_ALT)==0 ) for_doc=OPENOFFICE_ORG;
    }
    if( RegQueryValueEx(hKey,FTXHLP_VALNAME_XLS,NULL,NULL,(LPBYTE)buffer,&valuesize)==ERROR_SUCCESS )
    {
      if( stricmp(buffer,FTXHLP_HELPER_XLS2CSV)==0 ) for_xls=XLS2CSV;
      else if( stricmp(buffer,FTXHLP_HELPER_OOO)==0 || stricmp(buffer,FTXHLP_HELPER_OOO_ALT)==0 ) for_xls=OPENOFFICE_ORG;
    }
    if( RegQueryValueEx(hKey,FTXHLP_VALNAME_PPT,NULL,NULL,(LPBYTE)buffer,&valuesize)==ERROR_SUCCESS )
    {
      if( stricmp(buffer,FTXHLP_HELPER_CATPPT)==0 ) for_ppt=CATPPT;
      else if( stricmp(buffer,FTXHLP_HELPER_OOO)==0 || stricmp(buffer,FTXHLP_HELPER_OOO_ALT)==0 ) for_ppt=OPENOFFICE_ORG;
    }
    if( RegQueryValueEx(hKey,FTXHLP_VALNAME_RTF,NULL,NULL,(LPBYTE)buffer,&valuesize)==ERROR_SUCCESS )
    {
      if( stricmp(buffer,FTXHLP_HELPER_RTF2HTML)==0 ) for_rtf=RTF2HTML;
      else if( stricmp(buffer,FTXHLP_HELPER_OOO)==0 || stricmp(buffer,FTXHLP_HELPER_OOO_ALT)==0 ) for_rtf=OPENOFFICE_ORG;
    }
    RegCloseKey(hKey);
  }
#else
  // Linux: read from /etc/602sql
  const char cfgfile[]="/etc/602sql";
  const char invalidvalue[]=":";
  char buffer[300];
  switch( GetPrivateProfileInt(FTXHLP_SECTION,FTXHLP_VALNAME_USEOOO,0,cfgfile) )
  {
    case 1:
      soffice_binary=GetmallocedINIValue(cfgfile,FTXHLP_SECTION,FTXHLP_VALNAME_OOOBINARY);
      if( soffice_binary!=NULL ) use_ooo_convertor=OOO_STANDALONE;
      break;
    case 2:
      ooo_hostname=GetmallocedINIValue(cfgfile,FTXHLP_SECTION,FTXHLP_VALNAME_OOOHOSTNAME);
      ooo_port=GetmallocedINIValue(cfgfile,FTXHLP_SECTION,FTXHLP_VALNAME_OOOPORT);
      ooo_program_dir=GetmallocedINIValue(cfgfile,FTXHLP_SECTION,FTXHLP_VALNAME_OOOPROGRAMDIR);
      if( ooo_program_dir!=NULL ) use_ooo_convertor=OOO_SERVER;
      break;
  }
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_DOC,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    if( stricmp(buffer,FTXHLP_HELPER_ANTIWORD)==0 ) for_doc=ANTIWORD;
    else if( stricmp(buffer,FTXHLP_HELPER_CATDOC)==0 ) for_doc=CATDOC;
    else if( stricmp(buffer,FTXHLP_HELPER_OOO)==0 || stricmp(buffer,FTXHLP_HELPER_OOO_ALT)==0 ) for_doc=OPENOFFICE_ORG;
  }
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_XLS,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    if( stricmp(buffer,FTXHLP_HELPER_XLS2CSV)==0 ) for_xls=XLS2CSV;
    else if( stricmp(buffer,FTXHLP_HELPER_OOO)==0 || stricmp(buffer,FTXHLP_HELPER_OOO_ALT)==0 ) for_xls=OPENOFFICE_ORG;
  } 
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_PPT,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    if( stricmp(buffer,FTXHLP_HELPER_CATPPT)==0 ) for_ppt=CATPPT;
    else if( stricmp(buffer,FTXHLP_HELPER_OOO)==0 || stricmp(buffer,FTXHLP_HELPER_OOO_ALT)==0 ) for_ppt=OPENOFFICE_ORG;
  }
  GetPrivateProfileString(FTXHLP_SECTION,FTXHLP_VALNAME_RTF,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    if( stricmp(buffer,FTXHLP_HELPER_RTF2HTML)==0 ) for_rtf=RTF2HTML;
    else if( stricmp(buffer,FTXHLP_HELPER_OOO)==0 || stricmp(buffer,FTXHLP_HELPER_OOO_ALT)==0 ) for_rtf=OPENOFFICE_ORG;
  }
#endif
} /*}}}*/
t_convertor_params::~t_convertor_params() /*{{{*/
{
  if( soffice_binary!=NULL ) free(soffice_binary);
  if( ooo_program_dir!=NULL ) free(ooo_program_dir);
  if( ooo_hostname!=NULL ) free(ooo_hostname);
  if( ooo_port!=NULL ) free(ooo_port);
} /*}}}*/
#ifdef WINS
char * t_convertor_params::GetmallocedRegDbValue(HKEY hKey,const char *valuename) /*{{{*/
{
  DWORD valuesize;
  if( RegQueryValueEx(hKey,valuename,NULL,NULL,NULL,&valuesize)==ERROR_SUCCESS )
  {
    char *tmp=(char*)malloc(valuesize+1);
    if( tmp!=NULL )
    {
      if( RegQueryValueEx(hKey,valuename,NULL,NULL,(LPBYTE)tmp,&valuesize)==ERROR_SUCCESS )
      { // all succeeded
        tmp[valuesize]='\0';
        return tmp;
      }
      else free(tmp);
    }
  }
  return NULL;
} /*}}}*/
#else
char *t_convertor_params::GetmallocedINIValue(const char *cfgfile,const char *section,const char *key) /*{{{*/
{
  const char invalidvalue[]=":";
  char buffer[300];
  GetPrivateProfileString(section,key,invalidvalue,buffer,sizeof(buffer),cfgfile);
  if( strcmp(buffer,invalidvalue)!=0 )
  {
    char *tmp=(char*)malloc(strlen(buffer)+1);
    if( tmp!=NULL )
    {
      strcpy(tmp,buffer);
      return tmp;
    }
  }
  return NULL;
} /*}}}*/
#endif
/*}}}*/
static t_convertor_params convertor_params;

#ifdef WINS
/* class t_sem_for_convertor_childs {{{
 * Sets Error Mode that disables GPF dialog box and Abort, Retry, Fail dialog box
 * just before child convertor process is launched (see SetErrorMode() function from Windows API) */
class t_sem_for_convertor_childs
{
  unsigned long counter;
  UINT original_error_mode;
  CRITICAL_SECTION csSEM;
public:
  t_sem_for_convertor_childs();
  ~t_sem_for_convertor_childs();

  /* have to be called before child process is launched
   * returns true on success, false on error (when method failed to set desired error mode) */
  bool BeforeChildLaunch();
  /* have to be called after child process is launched
   * returns true on success, false on error (when method failed to set previous error mode) */
  bool AfterChildLaunch();
};
t_sem_for_convertor_childs::t_sem_for_convertor_childs() /*{{{*/
{
  counter=0;
  original_error_mode=0;
  InitializeCriticalSection(&csSEM);
} /*}}}*/
t_sem_for_convertor_childs::~t_sem_for_convertor_childs() /*{{{*/
{
  if( counter>0 )
  {
    counter=1;
    AfterChildLaunch();
  }
  DeleteCriticalSection(&csSEM);
} /*}}}*/
bool t_sem_for_convertor_childs::BeforeChildLaunch() /*{{{*/
{
  ProtectedByCriticalSection cs(&csSEM);
  counter++;
  if( counter>1 ) return true;
  original_error_mode=SetErrorMode(0); // get original error mode
  SetErrorMode(original_error_mode|SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX); // add desired bits to the original error mode
  return true;
} /*}}}*/
bool t_sem_for_convertor_childs::AfterChildLaunch() /*{{{*/
{
  ProtectedByCriticalSection cs(&csSEM);
  counter--;
  if( counter>0 ) return true;
  SetErrorMode(original_error_mode);
  return true;
} /*}}}*/
/*}}}*/
static t_sem_for_convertor_childs sem_for_convertor_childs;
#endif

/* abstract class t_input_convertor_with_specif {{{*/
class t_input_convertor_with_specif:public t_input_convertor
{
protected:
  t_specif specif; // specif of input stream -  the external information about the charset in the text
  /* analyses document and refines chaset and encoding settings in specif struct
   * this method is intended to be called by the descendants of this class just at the start
   * of the conversion process */
  void refine_charset_and_encoding_for_text_document();
public:
  t_input_convertor_with_specif(cdp_t cdp_param):t_input_convertor(cdp_param) {};
  virtual ~t_input_convertor_with_specif() {};
  virtual void set_specif(t_specif specifIn) { specif=specifIn; }  // stores the external information about the charset in the text
};
void t_input_convertor_with_specif::refine_charset_and_encoding_for_text_document() /*{{{*/
{
  if( is!=NULL && specif.charset==0 && specif.wide_char==0 )
  {
    is->reset();
    t_format_guesser fg(cdp,is);
    format_type_t ft=fg.determine_format_type();
    is->reset();
    switch( ft )
    {
      case FORMAT_TYPE_TEXT_7BIT:
        break;
      case FORMAT_TYPE_TEXT_8BIT_ISO:
        specif.charset=129; // ISO-8859-2 (Czech/Slovak)
        break;
      case FORMAT_TYPE_TEXT_8BIT_WIN1250:
        specif.charset=1; // Win1250 (Czech/Slovak)
        break;
      case FORMAT_TYPE_TEXT_8BIT_WIN1252:
        specif.charset=3; // Win1252 (French)
        break;
      case FORMAT_TYPE_TEXT_UTF8:
        specif.charset=CHARSET_NUM_UTF8;
        break;
      case FORMAT_TYPE_TEXT_UCS2:
        specif.wide_char=1;
        break;
    }
  }
} /*}}}*/
/*}}}*/
/* abstract class t_input_convertor_with_perform_action {{{*/
/* actions that can be performed by perform_action() method on documents */
typedef enum input_convertor_actions_t { ICA_INDEXDOCUMENT, ICA_COUNTWORDS } input_convertor_actions_t;
class t_input_convertor_with_perform_action:public t_input_convertor_with_specif
{protected:
  void ICA_COUNTWORDS_initialize_counter(void *dataptr) { *((int*)dataptr)=0; }
  void ICA_COUNTWORDS_inc_counter(void *dataptr) { if( *((int*)dataptr)<INT_MAX ) (*((int*)dataptr))++; }
 public:
  t_input_convertor_with_perform_action(cdp_t cdp_param):t_input_convertor_with_specif(cdp_param) {};
  virtual ~t_input_convertor_with_perform_action() {};
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods)
   * ICA_INDEXDOCUMENT: dataptr is unused
   * ICA_COUNTWORDS: dataptr points to word counter variable of int type */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr) = 0;
  virtual bool convert(void);
};
bool t_input_convertor_with_perform_action::convert() /*{{{*/
{
  if( is->get_filename_for_limits()!=NULL && limits_evaluator!=NULL )
  {
    const char *is_filename=is->get_filename_for_limits();
    int wordcount;
    if( !perform_action(ICA_COUNTWORDS,&wordcount) )
    { // error
      return false;
    }
    bool index_it;
    if( !test_limits_for_file(is_filename,wordcount,&index_it) )
    { // error
      return false;
    }
    if( !index_it ) return true; // document doesn't have to be indexed
  }
  // index document
  if( !perform_action(ICA_INDEXDOCUMENT,NULL) )
  { // error
    return false;
  }
  return true;
} /*}}}*/
/*}}}*/
#if 0
/* class t_input_convertor_plain_orig - not used {{{*/
class t_input_convertor_plain_orig : public t_input_convertor
{
 protected:
  t_specif specif;
  void next_part(void);
  char buf[4096];
  int bufcont, bufpos;  
 public:
   t_input_convertor_plain_orig(cdp_t cdp_param):t_input_convertor(cdp_param) { bufcont = bufpos = 0; };
  virtual bool convert(void);
  virtual void set_specif(t_specif specifIn)
    { specif=specifIn; }
};

void t_input_convertor_plain_orig::next_part(void)
{ if (specif.charset!=ft_specif.charset || specif.wide_char)
  { char buf2[4096];
    bufcont=is->read(buf2, sizeof(buf2));
    superconv(ATT_STRING, ATT_STRING, buf2, buf, specif, ft_specif, NULL);
    if (specif.wide_char) bufcont/=2;
  }
  else
    bufcont=is->read(buf, sizeof(buf));
  bufpos=0;
}

bool t_input_convertor_plain_orig::convert(void)
{ int wordpos;
  unsigned char c;  bool stop=false;
  do
  {// skip non-word characetrs:
    do
    { if (bufpos>=bufcont)
      { next_part();
        if (!bufcont) goto exit;
      }
      c=buf[bufpos++];
    } while (!working_for->FT9_WORD_STARTER(c, ft_specif.charset));
   // read the word:
    wordpos=0;
    do
    { agent_data->word[wordpos++]=c;
      if (bufpos>=bufcont)
      { next_part();
        if (!bufcont) { stop=true;  break; }
      }
      c=buf[bufpos++];
    } while (working_for->FT9_WORD_CONTIN(c, ft_specif.charset) && wordpos<MAX_FT_WORD_LEN /*t_agent_data::word[] is MAX_FT_WORD_LEN+1 long*/);
    agent_data->word[wordpos]=0;
   // return the word:
    agent_data->word_completed();
  } while (!stop);
 exit:
  return true;
}
/*}}}*/
#endif
/* abstract class t_cached_input_convertor {{{*/
class t_cached_input_convertor:public t_input_convertor_with_perform_action
{
protected:
  enum { BUFSIZE=1024*24 };
  char buf[BUFSIZE+1], // we need space for ending \0 char in case of using superconv() in next_part() method  - see next_part()
    incomplete_utf8_char[5];
  int buflen, // length of data that are stored in buffer buf
    idx, // index of current character within buf
    bufoffset, // offset of first character in buf from start of the stream
    incomplete_utf8_charlen;
  bool eof;

  // reads first part of document into buffer
  // have to be called before any operation
  void initialize_buffer();
  // reads next part of input data to buf buffer
  // converts text to current agent's charset when needed
  // returns number of readed chars
  // read_from_index is index in buf of first char that will be read
  // assumes that 0<=read_from_index<BUFSIZE
  int next_part(int read_from_index=0);
  // returns true if current position is at the end of stream
  const bool iseof() const { return eof; }

  /* support methods that aren't intended to call
   * by childs */
  // internal implementation of isstr() and isistr() methods
  const bool internal_isstr(const char *str,int len,const bool insensitive);
  
  /* methods for childs: */
  // returns current char or EOF
  const int getc() const { return (iseof())?EOF:buf[idx]; };
  // returns true if current char is whitespace (space, \t, \v, \r or \n)
  const bool cic_isspace() const;
  // returns true if string str of length len is at current position, otherwise returns false
  // when len is 0 then the whole string str have to be at current position
  // doesn't change current position
  const bool isstr(const char *str,int len=0) { return internal_isstr(str,len,false); };
  // case-insensitive version of isstr() method
  const bool isistr(const char *str,int len=0) { return internal_isstr(str,len,true); };
  // moves current position by 'count' chars forward
  // returns actual number of skipped chars
  // assumes that count<BUFSIZE
  int fwd(int count=1);
  // moves current position by 'count' chars backward
  // returns actual number of skipped chars
  // assumes that count<BUFSIZE
  int rew(int count=1);
  // moves current position to first non-whitespace char or to EOF
  void skipspace();
public:
  t_cached_input_convertor(cdp_t cdp_param):t_input_convertor_with_perform_action(cdp_param) {
    buflen=idx=bufoffset=0; eof=true;
    is=NULL; agent_data=NULL;
    incomplete_utf8_charlen=0; incomplete_utf8_char[0]=incomplete_utf8_char[1]=incomplete_utf8_char[2]=incomplete_utf8_char[3]=incomplete_utf8_char[4]='\0';
  };
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods) */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr) = 0;
};
void t_cached_input_convertor::initialize_buffer() /*{{{*/
{
  buflen=idx=bufoffset=incomplete_utf8_charlen=0;
  eof=false;
  buflen=next_part();
} /*}}}*/
int t_cached_input_convertor::next_part(int read_from_index) /*{{{*/
{
  if (specif.charset!=ft_specif.charset || specif.wide_char)
  {
    char buf2[BUFSIZE+2];
    int buf2len;
    // copy pre-read part of UTF-8 char to the start of buf2[]
    if( incomplete_utf8_charlen>0 ) memcpy(buf2,incomplete_utf8_char,incomplete_utf8_charlen);
    // read next part of stream
    buf2len=is->read(buf2+incomplete_utf8_charlen,BUFSIZE-read_from_index-incomplete_utf8_charlen)+incomplete_utf8_charlen;
    // test if nothing was read
    if( buf2len==incomplete_utf8_charlen ) { eof=true; return 0; }
    // if charset is UTF-8
    if( specif.charset==CHARSET_NUM_UTF8 )
    { /* While we read UTF-8 stream we have to detect last complete UTF-8 char in buffer.
       * UTF-8 char can be 1 to 6 bytes long.
       * We will store pre-readed part of following UTF-8 char into incomplete_utf8_char[].
       * Unused bytes of incomplete_utf8_char[] will be filled with '\0'. */
      // find rightmost byte which isn't 10xxxxxx - this is first byte of UTF-8 char
      char *ptr=buf2+buf2len-1;
      int utf8charlen=1;
      while( (*(unsigned char*)ptr&0xc0)==0x80 )
      {
        ptr--;
        utf8charlen++;
      }
      // compute length of UTF-8 char
      int utf8charlen_expected;
      if( (*(unsigned char*)ptr&0x80)==0 ) utf8charlen_expected=1;
      else if( (*(unsigned char*)ptr&0xe0)==0xc0 ) utf8charlen_expected=2;
      else if( (*(unsigned char*)ptr&0xf0)==0xe0 ) utf8charlen_expected=3;
      else if( (*(unsigned char*)ptr&0xf8)==0xf0 ) utf8charlen_expected=4;
      else if( (*(unsigned char*)ptr&0xfc)==0xf8 ) utf8charlen_expected=5;
      else if( (*(unsigned char*)ptr&0xfe)==0xfc ) utf8charlen_expected=6;
      else
      { // invalid first byte of UTF-8 char
        eof=true;
        return 0;
      }
      // zeroify incomplete_utf8_char[]
      memset(incomplete_utf8_char,0,sizeof(incomplete_utf8_char)); incomplete_utf8_charlen=0;
      // test if last UTF-8 char is incomplete
      if( utf8charlen_expected>utf8charlen )
      { // test if incomplete_utf8_char[] is long enough - fails only if stream isn't valid UTF-8 document
        if( utf8charlen>sizeof(incomplete_utf8_char) ) { eof=true; return 0; }
        // save it into incomplete_utf8_char[]
        memcpy(incomplete_utf8_char,ptr,utf8charlen);
        incomplete_utf8_charlen=utf8charlen;
        // move end of string mark to ensure that buf2[] willn't contain incomplete UTF-8 char
        *ptr='\0';
        // decrease length of readed data
        buf2len-=utf8charlen;
      }
    }
    buf2[buf2len]=buf2[buf2len+1]='\0'; // set end of string mark - we can do that because buf2 is BUFSIZE+2 long
//  { char msg [100];
//    sprintf(msg, "Converting from %d to %d", specif.charset, ft_specif.charset);
//    dbg_err(msg);
//  }
    superconv(ATT_STRING, ATT_STRING, buf2, buf+read_from_index, specif, ft_specif, NULL);
    if( specif.wide_char ) buf2len/=2;
    else if( specif.charset==CHARSET_NUM_UTF8 ) buf2len=(int)strlen(buf); /* 8-bit string could be shorter than its UTF-8 specimen
                                                                        we can call strlen() because buf is BUFSIZE+1 long
                                                                        i.e. it allways contains ending \0 char even when
                                                                        it holds BUFSIZE long string */
    idx=0; // next char to read is first char of buffer
    return buf2len;
  }
  else
  {
    int ret=is->read(buf+read_from_index, BUFSIZE-read_from_index);
    idx=0; // next char to read is first char of buffer
    return ret;
  } 
} /*}}}*/
const bool t_cached_input_convertor::cic_isspace() const /*{{{*/
{
  int c=getc();
  return c==' ' || c=='\t' || c=='\v' || c=='\r' || c=='\n';
} /*}}}*/
int t_cached_input_convertor::fwd(int count) /*{{{*/
{
  // exit if already at EOF
  if( eof ) return 0;
  // if next 'count' chars aren't in buf
  if( idx+count>=buflen )
  { // read them from input stream
    int ret=buflen-idx-1;
    // these chars are already in buf
    count-=ret;
    // empty buf buffer
    bufoffset+=buflen; idx=0;
    // read chars
    buflen=next_part();
    // check if chars were read
    if( count<=buflen )
    { // update idx to point to count-th char in buf
      idx=count-1;
      return ret+count;
    }
    else
    { // otherwise set current position to EOF
      idx=buflen;
      eof=true;
      return ret+buflen;
    }
  }
  // 'count' chars are in buf
  idx+=count;
  return count;
} /*}}}*/
int t_cached_input_convertor::rew(int count) /*{{{*/
{
  // exit if already at the start of the stream
  if( idx==0 && bufoffset==0 ) return 0;
  // if preceding 'count' chars aren't in buf
  if( count>idx )
  {
    if( bufoffset==0 )
    { // we already have beginning of the stream in buf
      int ret=idx;
      idx=0;
      return ret;
    }
    else
    { // these chars are already in buf
      int ret=idx;
      count-=idx;
      // read previous part of the stream
      int newoffset=(bufoffset>BUFSIZE)?bufoffset-BUFSIZE:0;
      is->reset_to(newoffset);
      buflen=next_part();
      // set idx
      idx=bufoffset-newoffset-count;
      bufoffset=newoffset;
      // if at least idx+1 chars weren't read
      if( buflen<=idx )
      { // set current position to EOF
        idx=buflen;
        eof=true;
        return ret;
      }
      else return ret+buflen-idx;
    }
  }
  // preceding 'count' chars are in buf
  idx-=count;
  return count;
} /*}}}*/ 
void t_cached_input_convertor::skipspace() /*{{{*/
{
  while( cic_isspace() ) fwd();
} /*}}}*/
const bool t_cached_input_convertor::internal_isstr(const char *str,int len,const bool insensitive) /*{{{*/
{
  if( str==NULL || *str=='\0' ) return true; // empty string is always present
  if( len<=0 ) len=(int)strlen(str);
  // ensure that buf contains at least len chars
  if( idx+len>buflen )
  { /* read next part of stream
     * char at current position will be moved to index 0 of buffer */
    int remaining=buflen-idx;
    // move chars that will remain in the buffer from current position to start of the buffer 
    memmove(buf,buf+idx,remaining*sizeof(char));
    // ensure consistency of counters and index
    bufoffset+=idx;
    idx=0;
    buflen=remaining;
    // read next part
    int readed=next_part(remaining);
    buflen+=readed;
  }
  return (insensitive)?strnicmp(buf+idx,str,len)==0:strncmp(buf+idx,str,len)==0;
} /*}}}*/
/*}}}*/
/* class t_input_convertor_plain {{{
 * converts plain text */
class t_input_convertor_plain:public t_cached_input_convertor
{
public:
  t_input_convertor_plain(cdp_t cdp_param):t_cached_input_convertor(cdp_param) {};
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods) */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr);
};

bool t_input_convertor_plain::perform_action(input_convertor_actions_t action,void *dataptr) /*{{{*/
{ int wordpos;
  int c;
  bool stop=false;
  refine_charset_and_encoding_for_text_document();
  // we have to reset input stream because previous action that was performed on the document could left input stream pointer in arbirary position
  is->reset();
  initialize_buffer();
  // initialize word counter
  if( action==ICA_COUNTWORDS ) ICA_COUNTWORDS_initialize_counter(dataptr);
  // main loop
  bool inside_long_word=false; // ==true when we're reading the long word (at least MAX_FT_WORD_LEN), otherwise false
  do
  {
    c=getc();
    // skip whitespaces
    while( FT_WORD_WHITESPACE(c) )
    {
      if( fwd()==0 )
        return true;
      c=getc();
      inside_long_word=false;
    }
    if (c==EOF) break;  // J.D added, otherwise inserts EOF as a word
    // c is non-whitespace
    if( working_for->FT9_WORD_STARTER(c, ft_specif.charset) )
    { // read the word
      wordpos=0;
      do
      {
        if( action==ICA_INDEXDOCUMENT ) agent_data->word[wordpos]=c;
        wordpos++;
        if( fwd()==0 ) { stop=true; break; }
        c=getc();
      } while( working_for->FT9_WORD_CONTIN(c, ft_specif.charset) && wordpos<MAX_FT_WORD_LEN /*t_agent_data::word[] is MAX_FT_WORD_LEN+1 long*/);
      if( action==ICA_INDEXDOCUMENT ) agent_data->word[wordpos]=0;
      // test if we've read the first part of the long word
      if( !inside_long_word && wordpos==MAX_FT_WORD_LEN ) inside_long_word=true;
      // return the word
      if( wordpos>0 )
      {
        if( action==ICA_INDEXDOCUMENT )
        {
          if( inside_long_word ) agent_data->word[0]='\0'; // clean the buffer
          else
          {
            if( !agent_data->word_completed() ) return true; // break the indexation
          }
        }
        else if( action==ICA_COUNTWORDS ) ICA_COUNTWORDS_inc_counter(dataptr);
      }
    }
    else
    { // read the non-word (puctuation etc.)
      inside_long_word=false; // clean the flag
      wordpos=0;
      while( !working_for->FT9_WORD_STARTER(c, ft_specif.charset) && !FT_WORD_WHITESPACE(c) && wordpos<MAX_FT_WORD_LEN )
      {
        if( action==ICA_INDEXDOCUMENT ) agent_data->word[wordpos]=c;
        wordpos++;
        if( fwd()==0 ) { stop=true; break; }
        c=getc();
      }
      if( action==ICA_INDEXDOCUMENT ) agent_data->word[wordpos]=0;
      // return the non-word
      if( wordpos>0 )
      {
        if( action==ICA_INDEXDOCUMENT )
        {
          if( !agent_data->word_completed(false) ) return true; // break the indexation
        }
      }
    }
    // test if the request was cancelled
    if( was_breaked() ) break;
#if 0
    // skip non-word characters
    while( !working_for->FT9_WORD_STARTER(c, ft_specif.charset) )
    {
      if( fwd()==0 )
        return true;
      c=getc();
    }
    // read the word
    wordpos=0;
    while( working_for->FT9_WORD_CONTIN(c, ft_specif.charset) && wordpos<MAX_FT_WORD_LEN /*t_agent_data::word[] is MAX_FT_WORD_LEN+1 long*/)
    {
      agent_data->word[wordpos++]=c;
      if( fwd()==0 ) { stop=true; break; }
      c=getc();
    }
    agent_data->word[wordpos]=0;
    // return the word
    if( wordpos>0 )
      agent_data->word_completed();
#endif
  } while( !stop );
  return true;
} /*}}}*/
/*}}}*/
/* struct html_character_entity_t    const html_character_entity_t html_character_entity {{{*/
const int character_entity_name_maxlen=8;
struct html_character_entity_t
{
  char name[character_entity_name_maxlen+1];
  int number;
};
// letter html entities sorted by number
const html_character_entity_t html_character_entity[] = {
  { "Agrave", 192 },
  { "Aacute", 193 },
  { "Acirc", 194 },
  { "Atilde", 195 },
  { "Auml", 196 },
  { "Aring", 197 },
  { "AElig", 198 },
  { "Ccedil", 199 },
  { "Egrave", 200 },
  { "Eacute", 201 },
  { "Ecirc", 202 },
  { "Euml", 203 },
  { "Igrave", 204 },
  { "Iacute", 205 },
  { "Icirc", 206 },
  { "Iuml", 207 },
  { "ETH", 208 },
  { "Ntilde", 209 },
  { "Ograve", 210 },
  { "Oacute", 211 },
  { "Ocirc", 212 },
  { "Otilde", 213 },
  { "Ouml", 214 },
  { "Oslash", 216 },
  { "Ugrave", 217 },
  { "Uacute", 218 },
  { "Ucirc", 219 },
  { "Uuml", 220 },
  { "Yacute", 221 },
  { "THORN", 222 },
  { "szlig", 223 },
  { "agrave", 224 },
  { "aacute", 225 },
  { "acirc", 226 },
  { "atilde", 227 },
  { "auml", 228 },
  { "aring", 229 },
  { "aelig", 230 },
  { "ccedil", 231 },
  { "egrave", 232 },
  { "eacute", 233 },
  { "ecirc", 234 },
  { "euml", 235 },
  { "igrave", 236 },
  { "iacute", 237 },
  { "icirc", 238 },
  { "iuml", 239 },
  { "eth", 240 },
  { "ntilde", 241 },
  { "ograve", 242 },
  { "oacute", 243 },
  { "ocirc", 244 },
  { "otilde", 245 },
  { "ouml", 246 },
  { "oslash", 248 },
  { "ugrave", 249 },
  { "uacute", 250 },
  { "ucirc", 251 },
  { "uuml", 252 },
  { "yacute", 253 },
  { "thorn", 254 },
  { "yuml", 255 },
  { "OElig", 338 },
  { "oelig", 339 },
  { "Scaron", 352 },
  { "scaron", 353 },
  { "Yuml", 376 },
  { "", 0 } // last item
}; /*}}}*/
/* abstract class t_input_convertor_ml_base {{{
 * base class for markup languages */
class t_input_convertor_ml_base:public t_cached_input_convertor
{
protected:
  // flag which affect functionality of methods
  bool convert_html;

  inline const char *get_format_name() const { return (convert_html)?"HTML":"XML"; };

  // skips HTML/XML comment <!-- ... -->
  // assumes that current position of stream is at '<' char of HTML comment
  // returns 1 on success, 0 on error (unexpected end of stream)
  int skip_comment();
  // skips HTML/XML tag
  // assumes that current position of stream is at '<' char
  // if convert_html is true and it's <script> or <style> tag reads up to ending </script> or </style> tag
  // returns 1 on success, 0 on error (unexpected end of stream)
  int skip_tag();
  // returns current char (char at current position) or EOF
  // decodes HTML-encoded chars like &gt; &quot; etc.
  // moves current position to next char
  const int html_readchar();
  // reads decimal number from current position
  // returns 1 on success, 0 on error (none number on current position)
  // moves current position behind number on success
  int read_number(int &number);
  // reads hexadecimal number from current position
  // returns 1 on success, 0 on error (none number on current position)
  // moves current position behind number on success
  int read_xnumber(int &number);

  // helper methods:
  // reads tag name (first buflen chars of tag name) into buf which length is buflen
  // returns 1 on success, 0 on error (unexpected end of stream)
  // assumes that starting '<' is at current position
  // when there is no tag at current position sets buf to empty string and returns 1
  // is_opening_tag is out parameter =true for opening <tag> or =false for closing </tag>
  // leaves current position on fist char behind tag name or on buflen+1 -th
  // char of tag name
  int read_tag_name(bool &is_opening_tag,char *buf,int buflen);
  // moves current pointer behind end of tag '>'
  // if convert_html is false then assumes that value of attributes can be enclosed in single quotes too
  // returns 1 on success, 0 on error (unexpected end of stream)
  int move_behind_end_of_tag();
  // moves current pointer behind text enclosed in double quotes "..."
  // (behind closing ")
  // returns 1 on success, 0 on error (unexpected end of stream)
  // assumes that current position is at starting "
  int move_behind_quoted_string();
  // moves current pointer behind text enclosed in single quotes '...'
  // (behind closing ')
  // returns 1 on success, 0 on error (unexpected end of stream)
  // assumes that current position is at starting '
  int move_behind_single_quoted_string();
  // moves current pointer behind end of current line
  // returns 1 on success, 0 on error (unexpected end of stream)
  int move_behind_end_of_line();
  // moves current pointer behind end of comment */
  // (behind closing / )
  // returns 1 on success, 0 on error (unexpected end of stream)
  int move_behind_end_of_script_comment();

  // sets specif.charset according to charset name
  // charset points to buffer that contains value XXXX of <meta http-equiv="Content-type" content="text/hml; charset=XXXX">
  // or <?xml encogind="XXXX"?>
  void determine_charset(const char *charset);
  // reads charset from metadata of the document
  // and updates specif
  virtual void update_charset_from_meta() = 0;
public:
  t_input_convertor_ml_base(cdp_t cdp_param,const bool _convert_html):t_cached_input_convertor(cdp_param),convert_html(_convert_html) {};
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods) */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr);
};
bool t_input_convertor_ml_base::perform_action(input_convertor_actions_t action,void *dataptr) /*{{{*/
{
  refine_charset_and_encoding_for_text_document();
  is->reset(); // input stream pointer could be left by previous action in arbitrary position
  initialize_buffer();
  update_charset_from_meta();
  // we have to reset input stream because update_charset_from_meta() left the stream in undetermined state
  is->reset();
  initialize_buffer();
  if( action==ICA_COUNTWORDS ) ICA_COUNTWORDS_initialize_counter(dataptr);
  bool inside_long_word=false; // ==true when we're reading the long word (at least MAX_FT_WORD_LEN), otherwise false
  int wordpos;
  int c;  bool stop=false;
  do
  {
    // is it start of the tag?
    if( (c=getc())=='<' )
    {
      inside_long_word=false;
      // test if this is a comment
      if( isstr("<!--") )
      {
        if( !skip_comment() ) { request_error_invalid_format(get_format_name(),"comment exceeds end of document"); return false; }
      }
      else if( !skip_tag() ) { 
        request_error_invalid_format(get_format_name(),"tag exceeds end of document"); return false; 
      }
    }
    else
    {
      c=getc();
      // skip whitespaces
      while( FT_WORD_WHITESPACE(c) )
      {
        if( fwd()==0 )
          return true;
        c=getc();
        inside_long_word=false;
      }
      if( c=='<' ) continue; // start of the tag
      // c is non-whitespace
      int htmlchar=html_readchar();
      if( htmlchar==EOF ) break;
      do {
        if( working_for->FT9_WORD_STARTER(htmlchar, ft_specif.charset) )
        { // read the word
          wordpos=0;
          do { 
            if( action==ICA_INDEXDOCUMENT ) agent_data->word[wordpos]=htmlchar; // we need HTML-decoded chars
            wordpos++;
            c=getc();
            if( c=='<' || FT_WORD_WHITESPACE(c) ) { htmlchar=EOF/* htmlchar is processed */; break; } // start of the tag or whitespace
            if( (htmlchar=html_readchar())==EOF ) { stop=true; break; }
          }
          while( working_for->FT9_WORD_CONTIN(htmlchar, ft_specif.charset) && wordpos<MAX_FT_WORD_LEN /*t_agent_data::word[] is MAX_FT_WORD_LEN+1 long*/);
          // !!! htmlchar is already read from the stream and stream pointer is moved behind htmlchar
          // so we have to process htmlchar during next step of the cycle
          if( action==ICA_INDEXDOCUMENT ) agent_data->word[wordpos]=0;
          // test if we've read the first part of the long word
          if( !inside_long_word && wordpos==MAX_FT_WORD_LEN ) inside_long_word=true;
          // return the word
          if( wordpos>0 )
          {
            if( action==ICA_INDEXDOCUMENT )
            {
              if( inside_long_word ) agent_data->word[0]='\0'; // clean the buffer
              else
              {
                if( !agent_data->word_completed() ) return true; // break the indexation
              }
            } 
            else if( action==ICA_COUNTWORDS ) ICA_COUNTWORDS_inc_counter(dataptr);
          }
        }
        else
        { // read the non-word (puctuation etc.)
          inside_long_word=false; // clean the flag
          wordpos=0;
          while( !working_for->FT9_WORD_STARTER(htmlchar, ft_specif.charset) && !FT_WORD_WHITESPACE(htmlchar) && wordpos<MAX_FT_WORD_LEN )
          {
            if( action==ICA_INDEXDOCUMENT ) agent_data->word[wordpos]=htmlchar;
            wordpos++;
            c=getc();
            if( c=='<' || FT_WORD_WHITESPACE(c) ) { htmlchar=EOF/* htmlchar is processed */; break; } // start of the tag or whitespace
            if( (htmlchar=html_readchar())==EOF ) { stop=true; break; }
          }
          // !!! htmlchar is already read from the stream and stream pointer is moved behind htmlchar
          // so we have to process htmlchar during next step of the cycle
          if( action==ICA_INDEXDOCUMENT ) agent_data->word[wordpos]=0;
          // return the non-word
          if( wordpos>0 )
          {
            if( action==ICA_INDEXDOCUMENT )
            {
              if( !agent_data->word_completed(false) ) return true; // break the indexation
            } 
          }
        }
        /* when htmlchar==EOF then htmlchar is processed
         * otherwise we have to process htmlchar */
      }
      while( htmlchar!=EOF && !FT_WORD_WHITESPACE(htmlchar)/* when &nbsp; is read from stream */ );
      // test if the request was cancelled
      if( was_breaked() ) break;
    }
  } while( !stop );
  return true;
} /*}}}*/
int t_input_convertor_ml_base::skip_comment() /*{{{*/
{ 
  if( !isstr("<!--") ) return 1;
  fwd(4); // skip <!--
  for( ;; )
  {
    if( getc()=='-' )
    {
      if( isstr("-->") )
      {
        fwd(3);
        return 1;
      }
    }
    if( !fwd() ) return 0;
  }
} /*}}}*/
int t_input_convertor_ml_base::skip_tag() /*{{{*/
{ 
  if( getc()!='<' ) return 0;
  // test if this is <script> or <style> tag
  const int MAXTAGNAMELEN=7;
  char tagname[MAXTAGNAMELEN+1];
  bool is_opening_tag;
  if( !read_tag_name(is_opening_tag,tagname,MAXTAGNAMELEN) ) return 0;
  if( !move_behind_end_of_tag() ) return 0;
  if( convert_html && is_opening_tag && (stricmp(tagname,"script")==0 || stricmp(tagname,"style")==0) )
  { // skip to ending </script> or </style> tag
    for( ;; )
    {
      int c=getc();
      switch( c )
      {
        case '<':
          if( isstr("<!--") )
          {
            if( !skip_comment() ) return 0;
          }
          else
          {
            char endtagname[MAXTAGNAMELEN+1];
            bool is_opening_tag;
            if( !read_tag_name(is_opening_tag,endtagname,MAXTAGNAMELEN) ) return 0;
            if( !move_behind_end_of_tag() ) return 0;
            if( !is_opening_tag && stricmp(tagname,endtagname)==0 ) return 1; // end tag was found
          }
          break;
        case '/':
          if( !fwd() ) return 0; // /
          c=getc();
          switch( c )
          {
            case '/':
              if( !fwd() ) return 0; // /
              if( !move_behind_end_of_line() ) return 0;
              break;
            case '*':
              if( !fwd() ) return 0; // *
              if( !move_behind_end_of_script_comment() ) return 0;
              break;
          }
          break;
        default:
          if( !fwd() ) return 0;
      }
    }
  }
  else return 1;
} /*}}}*/
const int t_input_convertor_ml_base::html_readchar() /*{{{*/
{ 
  int c=getc();
  if( !fwd() ) return (c!='&')?c:EOF; // skip &, however return c when c is last char of the document
  if( c!='&' ) return c;
  // read number or name of HTML entity into number or name
  int number=0;
  const int MAXNAMELEN=20;
  char name[MAXNAMELEN+1]; *name='\0';
  if( (c=getc())=='#' )
  { // numeric character reference
    if( !fwd() ) return EOF; // skip #
    c=getc();
    if( c=='x' )
    { // hexadecimal reference &#xH; H is case-insensitive!
      if( !fwd() ) return EOF; // skip x
      int number;
      if( !read_xnumber(number) ) return EOF;
      if( (c=getc())!=';' ) return EOF;
      fwd(); // skip ;
    }
    else if( c>='0' && c<='9' )
    { // decimal reference &#N;
      int number;
      if( !read_number(number) ) return EOF;
      if( (c=getc())!=';' ) return EOF;
      fwd(); // skip ;
    }
  }
  else
  { int i;
    // character entity - case-sensitive!
    for(i=0;i<MAXNAMELEN;i++ )
    {
      name[i]=c;
      if( !fwd() ) return EOF;
      if( (c=getc())==';' ) break;
    }
    name[i]='\0';
    if( c!=';' )
    {
      do
      {
        if( !fwd() ) return EOF;
      } while( (c=getc())!=';' );
    }
    fwd(); // skip ;
  }
  // search through html_character_entity
  // nb: we ignore non-letter entities as &quot; and &copy; 
  for( int i=0;html_character_entity[i].name[0]!='\0'&&html_character_entity[i].number!=0;i++ )
  {
    if( html_character_entity[i].number==number || strcmp(html_character_entity[i].name,name)==0 )
    { // html entity was found
      if( html_character_entity[i].number<256 ) return html_character_entity[i].number;
      // convert high numbers to character
      switch( html_character_entity[i].number )
      {
        case 338: return 140; // OElig
        case 339: return 156; // oelig
        case 352: return 138; // Scaron
        case 353: return 154; // scaron
        case 376: return 159; // Yuml
        default: return ' ';
      }
    }
  }
  // html entity wasn't found (i.e. this is non-letter entity)
  return ' ';
} /*}}}*/
int t_input_convertor_ml_base::read_tag_name(bool &is_opening_tag,char *buf,int buflen) /*{{{*/
{ 
  if( getc()!='<' ) { *buf='\0'; return 1; }
  if( !fwd() ) return 0;
  if( getc()=='/' )
  { 
    is_opening_tag=false;
    if( !fwd() ) return 0;
  }
  else is_opening_tag=true;
  skipspace();
  int i;
  for( i=0;i<buflen;i++ )
  {
    int c=getc();
    if( (c>='a' && c<='z') || (c>='A' && c<='Z') ) buf[i]=c;
    else break;
    if( !fwd() ) break;
  }
  buf[i]='\0';
  return 1;
} /*}}}*/
int t_input_convertor_ml_base::move_behind_end_of_tag() /*{{{*/
{ 
  for( ;; )
  { /* HTML/XML comment can't be placed inside tag */
    int c=getc();
    switch( c )
    {
      case '\"': // skip string enclosed in " "
        if( !move_behind_quoted_string() ) return 0;
        break;
      case '>': // this is end of tag
        fwd();
        return 1;
      case '\'':
        if( !convert_html )
        {
          if( !move_behind_single_quoted_string() ) return 0;
          break;
        }
        // else execute default: statements
      default:
        if( !fwd() ) return 0;
    }
  }
} /*}}}*/
int t_input_convertor_ml_base::move_behind_quoted_string() /*{{{*/
{ 
  if( getc()!='\"' ) return 1;
  do
  { if( !fwd() ) return 0;
  } while( getc()!='\"' );
  fwd(); // skip closing "
  return 1;
} /*}}}*/
int t_input_convertor_ml_base::move_behind_single_quoted_string() /*{{{*/
{ 
  if( getc()!='\'' ) return 1;
  do
  { if( !fwd() ) return 0;
  } while( getc()!='\'' );
  fwd(); // skip closing '
  return 1;
} /*}}}*/
int t_input_convertor_ml_base::read_number(int &number) /*{{{*/
{ 
  number=0;
  for( int c=getc();c>='0' && c<='9'; )
  {
    number=number*10+(c-'0');
    if( !fwd() ) break;
  }
  return 1;
} /*}}}*/
int t_input_convertor_ml_base::read_xnumber(int &number) /*{{{*/
{ 
  number=0;
  for( int c=getc();(c>='0' && c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'); )
  {
    number=number*16+(c>='0'&&c<='9')?c-'0':(c>='a'&&c>='f')?10+c-'a':10+c-'A';
    if( !fwd() ) break;
  }
  return 1;
} /*}}}*/
int t_input_convertor_ml_base::move_behind_end_of_line() /*{{{*/
{ 
  for( ;; )
  {
    int c=getc();
    if( !fwd() ) return 1; // c was last char of the document
    if( c=='\r' )
    {
      c=getc(); // get char after \r
      if( !fwd() ) return 1; // c was last char of the document
    }
    if( c=='\n' ) return 1;
  }
} /*}}}*/
int t_input_convertor_ml_base::move_behind_end_of_script_comment() /*{{{*/
{ 
  for( ;; )
  {
    int c=getc();
    if( !fwd() ) return 0;
    if( c=='*' )
    {
      if( (c=getc())=='/' )
      {
        fwd();
        return 1;
      }
    }
  }
} /*}}}*/
void t_input_convertor_ml_base::determine_charset(const char *charset) /*{{{*/
{
  // registered charset list is there http://www.iana.org/assignments/character-sets
  // we will detect just those charsets which are supported by 602SQL
  if( stricmp(charset,"ascii")==0 || stricmp(charset,"us-ascii")==0 || stricmp(charset,"us")==0
    || stricmp(charset,"ibm367")==0 || stricmp(charset,"cp367")==0 || stricmp(charset,"csascii")==0
    || stricmp(charset,"iso646-us")==0 || stricmp(charset,"iso-ir-6")==0
    || stricmp(charset,"ansi_x3.4-1986")==0 || stricmp(charset,"iso_646.irv:1991")==0 )
  { specif.charset=0; }
  else if( stricmp(charset,"windows-1250")==0 )
  { specif.charset=1; }
  else if( stricmp(charset,"windows-1252")==0 )
  { specif.charset=3; }
  else if( stricmp(charset,"iso-8859-1")==0 || stricmp(charset,"iso_8859-1")==0
    || stricmp(charset,"latin1")==0 || stricmp(charset,"l1")==0
    || stricmp(charset,"iso-ir-100")==0 || stricmp(charset,"ibm819")==0
    || stricmp(charset,"cp819")==0 || stricmp(charset,"csisolatin1")==0 )
  { specif.charset=131; }
  else if( stricmp(charset,"iso-8859-2")==0 || stricmp(charset,"iso_8859-2")==0
    || stricmp(charset,"latin2")==0 || stricmp(charset,"l2")==0
    || stricmp(charset,"iso-ir-101")==0 || stricmp(charset,"csisolatin2")==0 )
  { specif.charset=129; }
  else if( stricmp(charset,"utf-8")==0 )
  { specif.charset=CHARSET_NUM_UTF8; }
  // other charsets are ignored
} /*}}}*/
/*}}}*/
/* class t_input_convertor_html {{{*/
class t_input_convertor_html : public t_input_convertor_ml_base
{
protected:
  // reads charset from <meta httpequiv="content-type" value="...">
  // and updates specif
  virtual void update_charset_from_meta();
public:
  t_input_convertor_html(cdp_t cdp_param):t_input_convertor_ml_base(cdp_param,true/*we want detect HTML*/) {}
};
void t_input_convertor_html::update_charset_from_meta() /*{{{*/
{
  // test if the charset was already determined
  if( specif.charset!=0 || specif.wide_char!=0 ) return;
  bool head_found=false; // are we inside <head> tag?
  // search for <meta http-equiv="Content-Type" content="text/html; charset=XXXX">
  // this tag is inside <head> ... </head>
  for( ;; )
  {
    int c;
    if( (c=getc())==EOF ) break;
    // is it start of the tag?
    if( c=='<' )
    {
      // test if this is a comment
      if( isstr("<!--") ) skip_comment();
      else
      { // this is a tag
        const int TAGNAMELEN=30;
        char tagname[TAGNAMELEN+1];
        bool is_opening_tag;
        // get tag name and opening/closing flag
        if( !read_tag_name(is_opening_tag,tagname,TAGNAMELEN) ) goto leave_html_update_charset_from_meta;
        // when we find </head> we are done
        if( !is_opening_tag && stricmp(tagname,"HEAD")==0 ) goto leave_html_update_charset_from_meta;
        // when we find <body> we are done too
        if( is_opening_tag && stricmp(tagname,"BODY")==0 ) goto leave_html_update_charset_from_meta;
        if( is_opening_tag && stricmp(tagname,"HEAD")==0 )
        {
          head_found=true;
        }
        else if( head_found && is_opening_tag && stricmp(tagname,"META")==0 )
        { // this is <meta> tag
          bool http_equiv_found=false;
          for( ;; )
          {
            skipspace();
            if( isistr("http-equiv") )
            {
              if( http_equiv_found ) goto leave_html_update_charset_from_meta;
              if( !fwd(10) ) goto leave_html_update_charset_from_meta;
              skipspace();
              if( getc()!='=' ) goto leave_html_update_charset_from_meta;
              if( !fwd() ) goto leave_html_update_charset_from_meta;
              skipspace();
              if( getc()=='\"' )
              {
                if( !fwd() ) goto leave_html_update_charset_from_meta;
              }
              if( !isistr("content-type") ) goto leave_html_update_charset_from_meta;
              if( !fwd(12) ) goto leave_html_update_charset_from_meta;
              skipspace();
              if( getc()=='\"' )
              {
                if( !fwd() ) goto leave_html_update_charset_from_meta;
              }
              http_equiv_found=true;
            }
            else if( isistr("content") )
            {
              if( !fwd(7) ) goto leave_html_update_charset_from_meta;
              skipspace();
              if( getc()!='=' ) goto leave_html_update_charset_from_meta;
              if( !fwd() ) goto leave_html_update_charset_from_meta;
              skipspace();
              if( getc()!='\"' ) goto leave_html_update_charset_from_meta;
              if( !fwd() ) goto leave_html_update_charset_from_meta;
              if( !isistr("text/html;") ) goto leave_html_update_charset_from_meta;
              if( !fwd(10) ) goto leave_html_update_charset_from_meta;
              skipspace();
              if( !isistr("charset") ) goto leave_html_update_charset_from_meta;
              if( !fwd(7) ) goto leave_html_update_charset_from_meta;
              skipspace();
              if( getc()!='=' ) goto leave_html_update_charset_from_meta;
              if( !fwd() ) goto leave_html_update_charset_from_meta;
              skipspace();
              // read charset into charset var
              t_flstr charset(50,50);
              if( charset.error() ) goto leave_html_update_charset_from_meta;
              do
              { charset.putc(getc());
                if( !fwd() ) goto leave_html_update_charset_from_meta;
              } while( getc()!='\"' && !cic_isspace() );
              if( charset.error() || charset.getlen()==0 ) goto leave_html_update_charset_from_meta;
              charset.putc('\0');
              // determine charset
              determine_charset(charset.get());
              goto leave_html_update_charset_from_meta;
            }
            else break; // all other attributes of <meta> tag will lead to skip this <meta> tag
          }
        }
        if( !move_behind_end_of_tag() ) goto leave_html_update_charset_from_meta;
      }
    }
    else
    {
      // skip to next HTML char
      html_readchar();
    }
  }
leave_html_update_charset_from_meta:
  // we have to reset input stream before method exit
  is->reset();
  initialize_buffer();
} /*}}}*/
/*}}}*/
/* class t_input_convertor_xml {{{*/
class t_input_convertor_xml:public t_input_convertor_ml_base
{
protected:
  // reads charset from <?xml encoding="XXXX"?>
  // and updates specif
  virtual void update_charset_from_meta();
public:
  t_input_convertor_xml(cdp_t cdp_param):t_input_convertor_ml_base(cdp_param,false/*XML isn't HTML*/) {}
};
void t_input_convertor_xml::update_charset_from_meta() /*{{{*/
{
  // test if the charset was already determined
  if( specif.charset!=0 || specif.wide_char!=0 ) return;
  // otherwise search for the charset
  skipspace();
  if( !iseof() )
  {
    if( isstr("<?xml") )
    {
      if( !fwd(5) ) return;
      for( ;; )
      {
        skipspace();
        if( iseof() || isstr("?>") ) return;
        if( isstr("encoding") )
        {
          if( !fwd(8) ) return;
          skipspace();
          if( getc()=='=' )
          {
            if( !fwd() ) return;
            skipspace();
            int brace=getc();
            if( brace!='\"' && brace!='\'' ) return;
            if( !fwd() ) return;
            t_flstr charset(50,50);
            if( charset.error() ) return;
            while( getc()!=brace )
            {
              charset.putc(getc());
              if( !fwd() ) return;
            }
            if( charset.error() || charset.getlen()==0 ) return;
            charset.putc('\0');
            // determine charset
            determine_charset(charset.get());
            return;
          }
          // else this is attribute whose name starts with 'encoding'
          // => read it like any other attribute
        }
        /* We will detect
         *  attribute = "value"
         *  attribute = 'value'
         *  attribute
         * though XML specification is much more restrictive.
         * see http://www.w3.org/TR/REC-xml/#sec-prolog-dtd */
        int c=getc();
        while( (c>='a' && c<='z') || (c>='A' && c<='Z') )
        {
          if( !fwd() ) return;
          c=getc();
        }
        skipspace();
        if( getc()=='=' )
        {
          if( !fwd() ) return;
          skipspace();
          int brace=getc();
          if( brace!='\"' && brace!='\'' ) return;
          do {
            if( !fwd() ) return;
          } while( getc()!=brace );
          if( !fwd() ) return;
        }
      }
    }
  }
} /*}}}*/
/*}}}*/
/* abstract class t_input_convertor_ext_file_base {{{*/
class t_input_convertor_ext_file_base:public t_input_convertor_with_perform_action
{
protected:
  enum { COREALLOC_OWNER_ID=0xe4 };
  static unsigned long tmpname_index;

  void request_error_execution_failed(const char *command,const char *situation);

  bool exists_file(const char *filename);
  //  low-level helper methods:
  // creates temporary filename, saves it into corealloc-ed buffer and sets filename to point to this buffer
  // returns true on success, false on error and logs error with request_error()
  // default_ext is extension without dot which have to have temporary filename on NULL (none extension required)
  bool create_tmp_filename(char *&filename,const char *default_ext=NULL);
  // creates temporary dirname, saves it into corealloc-ed buffer and sets dirname to point to this buffer
  // creates temporary directory too
  // returns true on success, false on error and logs error with request_error()
  bool create_tmp_dirname_and_mkdir(char *&dirname);
  // returns true on success, false on error and logs error with request_error()
  virtual bool input_stream2file(t_input_stream &is,const char *filename);
  // tests if exists executable exe_name (on Windows with extension exe_ext)
  // in ftx_subdir subdirectory of "ftx" base directory
  // returns true if executable exists, false otherwise and raises specific error too
  // if outputbuf!=NULL then method stores there fully qualified name of executable
  bool exists_exe(t_flstr *outputbuf,const char *ftx_subdir,const char *exe_name,const char *exe_ext);
  // appends executable name with full path to command
  // returns true on success or false on error and logs error with request_error()
  // ftx_subdir is subdirectory of 'ftx' dir where executable resides
  // exe_name is name of executable (without extension on Windows)
  // exe_ext is extension of executable on Windows (without dot) - unused on Linux
  // doesn't append space after executable name
  // calls exists_exe() too
  bool append_exe(t_flstr &command,const char *ftx_subdir,const char *exe_name,const char *exe_ext);
  // deletes directory dirname
  // deletes all files in this directory too
  // returns true on success, false on error and logs error with request_error()
  bool delete_directory(const char *dirname);
  // returns corealloc()-ed buffer that contains double-quoted filename on NULL on error (out of memory) and logs this error
  // caller have to corefree() returned buffer!
  char *corestr_quote_filename(const char *filename);
  // tests if current transaction is empty, i.e. if none SQL statement modified data
  // in fact it tests if exists any read/write lock set by current transaction
  bool is_transaction_empty();
public:
  t_input_convertor_ext_file_base(cdp_t cdp_param):t_input_convertor_with_perform_action(cdp_param) {};
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods) */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr) = 0;
};
unsigned long t_input_convertor_ext_file_base::tmpname_index=0;
char *t_input_convertor_ext_file_base::corestr_quote_filename(const char *filename) /*{{{*/
{
  char *ret=(char *)corealloc(strlen(filename)+3,COREALLOC_OWNER_ID);
  if( ret==NULL ) { request_error_out_of_memory(); return NULL; }
  *ret='\"';
  strcpy(ret+1,filename);
  strcat(ret,"\"");
  return ret;
} /*}}}*/
void t_input_convertor_ext_file_base::request_error_execution_failed(const char *command,const char *situation) /*{{{*/
{
  if( situation==NULL ) request_errorf("Fulltext: Execution of external convertor \"%s\" failed.",command);
  else request_errorf("Fulltext: Execution of external convertor \"%s\" failed (%s).",command,situation);
} /*}}}*/
bool t_input_convertor_ext_file_base::create_tmp_filename(char *&filename,const char *default_ext) /*{{{*/
{
#if 0
#ifdef WINS
  // WIN: _tempnam
  char alt_temp_dir[MAX_PATH];
  DWORD len;
  if( (len=GetModuleFileName(NULL,alt_temp_dir,sizeof(alt_temp_dir)))==0 ) { request_error("Fulltext: Can't determine executable name [GetModuleFileName]."); return false; }
  do {
    len--;
  } while( len>=0 && alt_temp_dir[len]!='\\' );
  alt_temp_dir[len]='\0';
  char *buf=_tempnam(alt_temp_dir,"fx");
  if( buf==NULL ) { request_error("Fulltext: Can't get temporary file name [_tempnam]."); return false; }
#if 0
  char buf[L_tmpnam];
  if( tmpnam(buf)==NULL ) return false;
#endif
#else
  // LIN: mkstemp
  char buf[100];
  strcpy(buf,"/tmp/602ftxXXXXXX");
  int fd=mkstemp(buf);
  if( fd==-1 ) { request_error("Fulltext: Can't get temporary file descriptor [mkstemp]."); return false; }
  close(fd);
#endif
#endif

  char buf[MAX_PATH];
  unsigned long myindex;
#ifdef SRVR
  EnterCriticalSection(&cs_short_term);
#endif
  myindex=tmpname_index++;
  if( tmpname_index>=10000 ) tmpname_index=0;
#ifdef SRVR
  LeaveCriticalSection(&cs_short_term);
#endif

#ifdef WINS
  char *tmp_env=getenv("TMP");
  if( tmp_env!=NULL )
  {
    sprintf(buf,"%s\\ftx%x%04u",tmp_env,GetCurrentProcessId(),myindex);
  }
  else
  {
    char alt_temp_dir[MAX_PATH];
    DWORD len;
    if( (len=GetModuleFileName(NULL,alt_temp_dir,sizeof(alt_temp_dir)))==0 ) { request_error("Fulltext: Can't determine executable name [GetModuleFileName]."); return false; }
    do {
      len--;
    } while( len>=0 && alt_temp_dir[len]!='\\' );
    alt_temp_dir[len]='\0';
    sprintf(buf,"%s\\ft%x%04u",alt_temp_dir,GetCurrentProcessId(),myindex);
  }
#else
  sprintf(buf,"/tmp/602ftx%x%04u",(unsigned long)getpid(),myindex);
#endif
 
  int tmp=(default_ext==NULL)?0:(int)strlen(default_ext)+1;
  filename=(char*)corealloc(strlen(buf)+tmp+1,COREALLOC_OWNER_ID);
  if( filename==NULL )
  {
    request_error_out_of_memory();
    return false;
  }
  strcpy(filename,buf);
  if( default_ext!=NULL )
  {
    strcat(filename,".");
    strcat(filename,default_ext);
  }
  if( exists_file(filename) ) DeleteFile(filename);
  return true;
} /*}}}*/
bool t_input_convertor_ext_file_base::create_tmp_dirname_and_mkdir(char *&dirname) /*{{{*/
{
  if( !create_tmp_filename(dirname) ) return false;
#ifdef WINS
  if( _mkdir(dirname)!=0 )
#else
  if( mkdir(dirname,S_IRWXU)!=0 )
#endif
  { corefree(dirname); dirname=NULL; request_error("Fulltext: Can't create temporary directory [mkdir]."); return false; }
  // dirname is already corealloc-ed buffer
  return true;
} /*}}}*/
bool t_input_convertor_ext_file_base::input_stream2file(t_input_stream &is,const char *filename) /*{{{*/
{
  FILE *f=fopen(filename,"wb");
  if( f==NULL ) { request_errorf("Fulltext: Can't open temporary file \"%s\" for write.",filename); return false; }
  for( ;; )
  {
    char buf[1024];
    int readed;
    if( (readed=is.read(buf,sizeof(buf)))<=0 ) break;
    if( fwrite(buf,sizeof(char),readed,f)<readed )
    {
      request_error("Fulltext: Error while writing document into temporary file.");
      fclose(f);
      return false;
    }
    // test if the request was cancelled
    if( was_breaked() ) return false; // return error so the caller is forced to stop the execution
  }
  fclose(f);
  return true;
} /*}}}*/
bool t_input_convertor_ext_file_base::exists_file(const char *filename) /*{{{*/
{
#ifdef WINS
  struct _stat s;
  return ::_stat(filename,&s)==0;
#else
  struct stat s;
  return stat(filename,&s)==0;
#endif
} /*}}}*/
bool t_input_convertor_ext_file_base::exists_exe(t_flstr *outputbuf,const char *ftx_subdir,const char *exe_name,const char *exe_ext) /*{{{*/
{
  char tmp[MAX_PATH];
#ifdef WINS
  DWORD len;
  if( (len=GetModuleFileName(NULL,tmp,sizeof(tmp)))==0 ) { request_error("Fulltext: Can't determine executable name [GetModuleFileName]."); return false; }
  do {
    len--;
  } while( len>=0 && tmp[len]!='\\' );
  tmp[len+1]='\0';
#else
  // Linux - use WB_LINUX_SUBDIR_NAME which is defined in comm/wbvers.h
  strcpy(tmp,"/usr/lib/"); strcat(tmp,WB_LINUX_SUBDIR_NAME); strcat(tmp,"/");
#endif
  strcat(tmp,"ftx"); strcat(tmp,PATH_SEPARATOR_STR);
  strcat(tmp,ftx_subdir); strcat(tmp,PATH_SEPARATOR_STR);
  strcat(tmp,exe_name);
#ifdef WINS
  if( exe_ext!=NULL )
  {
    strcat(tmp,".");
    strcat(tmp,exe_ext);
  }
#endif
  bool ret=exists_file(tmp);
  if( ret )
  {
    if( outputbuf!=NULL ) outputbuf->put(tmp);
  }
  else
  {
    request_errorf("Fulltext: External convertor \"%s\" not found at \"%s\".",exe_name,tmp);
  }
  return ret;
} /*}}}*/
bool t_input_convertor_ext_file_base::append_exe(t_flstr &command,const char *ftx_subdir,const char *exe_name,const char *exe_ext) /*{{{*/
{
  return exists_exe(&command,ftx_subdir,exe_name,exe_ext);
} /*}}}*/
bool t_input_convertor_ext_file_base::delete_directory(const char *dirname) /*{{{*/
{
  bool result=false;
  char filespec[MAX_PATH];
  sprintf(filespec,"%s%c*",dirname,PATH_SEPARATOR);
  WIN32_FIND_DATA fd;
  HANDLE hFile=FindFirstFile(filespec,&fd);
  if( hFile!=INVALID_HANDLE_VALUE )
  {
    for( ;; )
    {
      if( strcmp(fd.cFileName,".")!=0 && strcmp(fd.cFileName,"..")!=0 )
      {
        char file[MAX_PATH];
        sprintf(file,"%s%c%s",dirname,PATH_SEPARATOR,fd.cFileName);
#ifdef WINS
        if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
#else
        struct stat st;
        // get stat of file
        if( stat(file,&st)!=0 ) break; //error
        // and test if file is directory
        if( S_ISDIR(st.st_mode) )
#endif
        { // file is directory
          if( !delete_directory(file) ) goto leave_delete_directory;
        }
        else
        { // file isn't directory
          DeleteFile(file);
        }
      }
      if( !FindNextFile(hFile,&fd) ) break;
    }
    if( hFile!=INVALID_HANDLE_VALUE ) { FindClose(hFile); hFile=INVALID_HANDLE_VALUE; }
    char cwd[MAX_PATH]; *cwd='\0';
    getcwd(cwd,MAX_PATH);
    if( stricmp(cwd,dirname)==0 )
    {
      size_t cwdlen=strlen(cwd);
      if( cwd[cwdlen-1]==PATH_SEPARATOR ) cwd[--cwdlen]='\0';
      do
      {
        cwdlen--;
      } while( cwdlen>0 && cwd[cwdlen]!=PATH_SEPARATOR );
      if( cwd[cwdlen]==PATH_SEPARATOR ) cwd[cwdlen]='\0';
      chdir(cwd);
    }
    rmdir(dirname);
    result=true;
  }
leave_delete_directory:
  if( hFile!=INVALID_HANDLE_VALUE ) FindClose(hFile);
  return result;
} /*}}}*/
bool t_input_convertor_ext_file_base::is_transaction_empty() /*{{{*/
{
#ifdef SRVR
  bool result=false;
  cur_descr *cd;
  // test if exists any lock set by this client
  tcursnum curs=open_working_cursor(cdp,
    "SELECT * FROM _iv_locks WHERE owner_usernum=Client_number AND "
    "NOT( " // read lock on TABTAB is fine
    "  table_name='TABTAB' AND schema_name IS NULL AND (lock_type=128 OR lock_type=16) "
    ") ",
    &cd);
  if( curs!=NOCURSOR )
  {
    result=(cd->recnum==0);
    close_working_cursor(cdp,curs);
  }
  return result;
#else
  // client-side
  return false;
#endif
} /*}}}*/
/*}}}*/
/* abstract class t_input_convertor_ext_file_with_execute {{{
 * base class for t_input_convertor_ext_file_to_file and t_input_convertor_ext_file_to_dir
 * contains execute_statement() method */
class t_input_convertor_ext_file_with_execute:public t_input_convertor_ext_file_base
{
protected:
  // command is command which will be executed
  // argv are parameters which will be passed to command
  // dll_path is fully qualified directory name with dll/so libraries that command have to (or should) use
  //   on Windows: this directory will be made current directory and command will be run
  //   on Linux: this directory will be added to LD_LIBRARY_PATH environment variable to the start of its value
  //   it can be NULL - in that case nothing special is performed
  // returns true on success, false on error and logs error with request_error()
  bool execute_statement(t_flstr &command,char *argv[],char *dll_path);
public:
  t_input_convertor_ext_file_with_execute(cdp_t cdp_param):t_input_convertor_ext_file_base(cdp_param) {}
  virtual ~t_input_convertor_ext_file_with_execute() {}
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods) */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr) = 0;

  friend THREAD_PROC(ext_file_with_execute_thread_proc);
};
struct ext_file_with_execute_thread_data /*{{{*/
{
  t_flstr *command;
  char **argv;
  t_input_convertor_ext_file_with_execute *ic;
  bool result; // false on error, true on success
  char *dll_path;
  ext_file_with_execute_thread_data(t_flstr *_command,char **_argv,char *_dll_path,t_input_convertor_ext_file_with_execute *_ic):
    command(_command),argv(_argv),dll_path(_dll_path),ic(_ic),result(false) {};
}; /*}}}*/
#if defined WINS && !defined BELOW_NORMAL_PRIORITY_CLASS
// taken from winbase.h from MS Visual Studio .NET 2003
#define BELOW_NORMAL_PRIORITY_CLASS       0x00004000
#endif
THREAD_PROC(ext_file_with_execute_thread_proc) /*{{{
 * executes external conversion utility
 * data is pointer to ext_file_with_execute_thread_data struct
 * returns 1 on success or 0 on error (and logs error through data->ic->request_error_XXX()) */
{
  ext_file_with_execute_thread_data *dataptr=(ext_file_with_execute_thread_data*)data;
#ifdef WINS
  // compose command
  t_flstr cmd;
  char **tmp;
  if( !cmd.putc('\"') ) goto file_to_file_execute_statement_error;
  if( !cmd.put(dataptr->command->get()) ) goto file_to_file_execute_statement_error;
  if( !cmd.putc('\"') ) goto file_to_file_execute_statement_error;
  for( tmp=dataptr->argv;*tmp!=NULL;tmp++ )
  {
    if( !cmd.putc(' ') ) goto file_to_file_execute_statement_error;
    if( !cmd.put(*tmp) ) goto file_to_file_execute_statement_error;
  }
  if( !cmd.putc('\0') ) goto file_to_file_execute_statement_error;
  // prepare structs
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  DWORD wait_result,creation_flags;
  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  ZeroMemory( &pi, sizeof(pi) );
  creation_flags=CREATE_NO_WINDOW|CREATE_NEW_CONSOLE;
  if( dataptr->ic->is_transaction_empty() ) creation_flags|=BELOW_NORMAL_PRIORITY_CLASS;
  // set error mode that will be inherited by child process
  sem_for_convertor_childs.BeforeChildLaunch();
  // execute command
  if( !CreateProcess(NULL,cmd.get(),NULL,NULL,FALSE,creation_flags,NULL,dataptr->dll_path,&si,&pi) )
  {
    sem_for_convertor_childs.AfterChildLaunch();
    dataptr->ic->request_error_execution_failed(cmd.get(),"CreateProcess() failed");
    return 0;
  }
  // revert to original error mode
  sem_for_convertor_childs.AfterChildLaunch();
  // wait for the end
  for(;;)
  {
    wait_result=WaitForSingleObject(pi.hProcess,ic_break_test_period_ms);
    if( wait_result==WAIT_TIMEOUT )
    {
      if( dataptr->ic->was_breaked() )
      {
        TerminateProcess(pi.hProcess,0);
        break;
      }
      // and continue to wait for the process
    }
    else break;
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  if( wait_result==WAIT_OBJECT_0 )
  {
    dataptr->result=true;
    return 1;
  }
  else
  {
    if( wait_result!=WAIT_TIMEOUT ) dataptr->ic->request_error_execution_failed(cmd.get(),"WaitForSingleObject() failed");
    return 0;
  }
file_to_file_execute_statement_error:
  dataptr->ic->request_error_out_of_memory();
  return 0;
#else
  const int error_in_execv=96;
  pid_t child_pid;
  if( (child_pid=fork())==0 )
  { // child
    // update LD_LIBRARY_PATH
    if( dataptr->dll_path!=NULL )
    {
      char *ld_library_path=getenv("LD_LIBRARY_PATH");
      char *tmp=(char*)corealloc(((ld_library_path!=NULL)?strlen(ld_library_path):0)+strlen(dataptr->dll_path)+1,COREALLOC_OWNER_ID);
      if( tmp==NULL )
      {
        exit(error_in_execv);
      }
      strcpy(tmp,dataptr->dll_path);
      if( ld_library_path!=NULL ) { strcat(tmp,":"); strcat(tmp,ld_library_path); }
      if( setenv("LD_LIBRARY_PATH",tmp,1)!=0 ) exit(error_in_execv);
    }
    // prepare arguments - argv[0] have to be executable filename
    int argcount=0;
    for( ;dataptr->argv[argcount]!=NULL;argcount++ ) {}
    char **argv=(char**)malloc(sizeof(char*)*(argcount+2));
    if( argv==NULL ) exit(error_in_execv);
    argv[0]=dataptr->command->get(); // executable filename
    for( int i=0;i<=argcount;i++ ) // copy parameters and ending NULL too
    {
      argv[i+1]=dataptr->argv[i];
    }
    // execute
    execv(dataptr->command->get(),argv);
    // this is executed when execv() encoutered an error
    free(argv);
    exit(error_in_execv);
  }
  // parent
  if( child_pid==-1 ) { dataptr->ic->request_error_execution_failed(dataptr->command->get(),"fork() failed"); return 0; } // fork() wasn't successfull
  // wait until child is terminated
  int child_status;
  if( waitpid(child_pid,&child_status,0)<0 ) { dataptr->ic->request_error_execution_failed(dataptr->command->get(),"waitpid() failed"); return 0; }
  if( WEXITSTATUS(child_status)==error_in_execv ) { dataptr->ic->request_error_execution_failed(dataptr->command->get(),"execv() failed"); return 0; }
  dataptr->result=true;
  return (void*)1;
#endif
} /*}}}*/
bool t_input_convertor_ext_file_with_execute::execute_statement(t_flstr &command,char *argv[],char *dll_path) /*{{{*/
{
  ext_file_with_execute_thread_data data(&command,argv,dll_path,this);
  THREAD_HANDLE hThread;
  // start thread
  if( THREAD_START_JOINABLE(ext_file_with_execute_thread_proc,10000,(void*)&data,&hThread) )
  {
    // wait till thread finishes
    THREAD_JOIN(hThread);
    return data.result;
  }
  else
  {
    request_error_execution_failed(command.get(),"THREAD_START_JOINABLE() failed");
    return false;
  }
} /*}}}*/
/*}}}*/
/* abstract class t_input_convertor_ext_file_to_file {{{*/
class t_input_convertor_ext_file_to_file:public t_input_convertor_ext_file_with_execute
{
protected:
  // fully qualified filenames of input and output files
  // point to corealloc-ed buffers or are NULL
  // have to be corefree-ed in destructor
  char *infile,*outfile;

  //  methods that children have to define:
  // composes cmd. line statement which has to be executed
  // command (output parameter) is command which will be executed, it's empty at start
  // argv (output parameter) are parameters which will be passed to command, it's empty at start
  //   caller have to set args to point to array of parameters finished by NULL array item
  // infile is fully qualified filename of input file which already exists
  // outfile is fully qualified filename of output file which doesn't exist
  // dll_path (output parameter) is fully qualified directory name with dll/so libraries that command have to (or should) use
  //   on Windows: this directory will be made current directory and command will be run
  //   on Linux: this directory will be added to LD_LIBRARY_PATH environment variable to the start of its value
  //   it can be NULL - in that case nothing special is performed
  // returns true on success (and sets command and params too), false on error and logs error with request_error()
  virtual bool compose_statement(t_flstr &command,char **(&argv),const char *infile,const char *outfile,char *&dll_path) = 0;
  // performs action on output file
  // returns true on success, false on error and logs error with request_error()
  // t_input_stream *is data member contains contents of output file during this method
  virtual bool perform_action_on_output_file(input_convertor_actions_t action,void *dataptr) = 0;

  // child can perform tests right after command execution there
  // have to return true to indicate success or false to indicate error and logs error with request_error()
  virtual bool test_after_execution() { return true; }

  // child can define which extension have to have infile and outfile
  // default is NULL i.e. no extension
  virtual const char *get_infile_extension() { return NULL; }
  virtual const char *get_outfile_extension() { return NULL; }

  //  helper methods:
  // saves input stream into file and writes its name into infile data member
  // returns true on success, false on error and logs error with request_error()
  bool is2infile();
public:
  t_input_convertor_ext_file_to_file(cdp_t cdp_param):t_input_convertor_ext_file_with_execute(cdp_param) { infile=outfile=NULL; };
  virtual ~t_input_convertor_ext_file_to_file();
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods) */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr);
};
t_input_convertor_ext_file_to_file::~t_input_convertor_ext_file_to_file() /*{{{*/
{
  if( infile!=NULL )
  {
    if( exists_file(infile) ) DeleteFile(infile);
    corefree(infile);
  }
  if( outfile!=NULL )
  {
    if( exists_file(outfile) ) DeleteFile(outfile);
    corefree(outfile);
  }
} /*}}}*/
bool t_input_convertor_ext_file_to_file::perform_action(input_convertor_actions_t action,void *dataptr) /*{{{*/
{ 
  if( outfile==NULL || !exists_file(outfile) )
  {
    // 1. save input stream into file and write its name into infile data member
    if( !is2infile() ) return false;
    // 2. create output filename and write it into outfile
    if( !create_tmp_filename(outfile,get_outfile_extension()) ) return false;
    // 3. compose cmd. line statement
    t_flstr command(200,100);
    char **argv,*dll_path;
    if( !compose_statement(command,argv,infile,outfile,dll_path) ) return false;
    // 4. execute statement
    command.putc('\0');
    if( !execute_statement(command,argv,dll_path) ) return false;
    // let child test various conditions right after execution
    if( !test_after_execution() ) return false;
    // test if exists outfile
    if( !exists_file(outfile) ) { request_error_execution_failed(command.get(),"temporary output file doesn't exist"); return false; }
  }
  // 5. create t_input_stream instance for outfile
  {
    t_input_stream_file isf(cdp,outfile,is->get_filename_for_limits()/* LIMITS has to be tested with filename of original input stream */);
    t_input_stream *prev_is=is;
    is=&isf;    
  // 6. call convert_output_file()
    if( !perform_action_on_output_file(action,dataptr) )
    {
      is=prev_is;
      return false;
    }
    is=prev_is;
  // 7. destroy t_input_stream instance for outfile
    // in its destructor
  }
  return true;
} /*}}}*/
bool t_input_convertor_ext_file_to_file::is2infile() /*{{{*/
{
  assert(infile==NULL);
  assert(is!=NULL);
  if( !create_tmp_filename(infile,get_infile_extension()) ) return false;
  return input_stream2file(*is,infile);
} /*}}}*/
/*}}}*/
/* class t_input_convertor_xpdf {{{*/
class t_input_convertor_xpdf:public t_input_convertor_ext_file_to_file
{
protected:
  char *myargv[5],*infile_quoted,*outfile_quoted;
  virtual bool compose_statement(t_flstr &command,char **&argv,const char *infile,const char *outfile,char *&dll_path);
  virtual bool perform_action_on_output_file(input_convertor_actions_t action,void *dataptr);
public:
  t_input_convertor_xpdf(cdp_t cdp_param):t_input_convertor_ext_file_to_file(cdp_param) { infile_quoted=outfile_quoted=NULL; };
  virtual ~t_input_convertor_xpdf()
  {
    if( infile_quoted!=NULL ) corefree(infile_quoted);
    if( outfile_quoted!=NULL ) corefree(outfile_quoted);
  };
  virtual const char *get_infile_extension() { return "pdf"; }
};
bool t_input_convertor_xpdf::compose_statement(t_flstr &command,char **&argv,const char *infile,const char *outfile,char *&dll_path) /*{{{*/
{
  if( (infile_quoted=corestr_quote_filename(infile))==NULL ) return false;
  if( (outfile_quoted=corestr_quote_filename(outfile))==NULL ) return false;
  if( !append_exe(command,"xpdf","pdftotext","exe") ) return false;
  // fill myargv array
  myargv[0]="-enc";
  myargv[1]="UTF-8";
  myargv[2]=infile_quoted;
  myargv[3]=outfile_quoted;
  myargv[4]=NULL; // last item have to be NULL!
  // set argv param
  argv=myargv;
  dll_path=NULL;
  return true;
} /*}}}*/
bool t_input_convertor_xpdf::perform_action_on_output_file(input_convertor_actions_t action,void *dataptr) /*{{{*/
{
  t_input_convertor_plain ic(cdp);
  ic.set_agent(agent_data);
  ic.set_stream(is);
  ic.set_specif(t_specif(0,CHARSET_NUM_UTF8,0,0));
  ic.set_depth_level(depth_level);
  ic.set_limits_evaluator(limits_evaluator);
  ic.set_owning_ft(working_for);
  return ic.perform_action(action,dataptr); // perform_action() logs possible error
} /*}}}*/
/*}}}*/
/* abstract class t_input_convertor_ext_file_to_stdout {{{*/
class t_input_convertor_ext_file_to_stdout:public t_input_convertor_ext_file_base
{
protected:
  // fully qualified filename of input file
  // points to corealloc-ed buffer or is NULL
  // have to be corefree-ed in destructor
  char *infile;
  // fully qualified filename of output file (i.e. result of conversion
  // points to corealloc-ed buffer or is NULL
  // have to be corefree-ed in destructor
  char *outfile;

  //  methods that children have to define:
  // composes cmd. line statement which has to be executed
  // command is command which will be executed along with its parameters
  //    which will be passed to command, it's empty at start
  // returns true on success (and sets command too), false on error
  virtual bool compose_statement(t_flstr &command,const char *infile) = 0;
  // performs action on output file
  // returns true on success, false on error
  // t_input_stream *is data member contains contents of output stream during this method
  virtual bool perform_action_on_output_file(input_convertor_actions_t action,void *dataptr) = 0;

  // helper methods:
  bool execute_statement(t_flstr &command);
public:
  t_input_convertor_ext_file_to_stdout(cdp_t cdp_param):t_input_convertor_ext_file_base(cdp_param) { infile=outfile=NULL; };
  virtual ~t_input_convertor_ext_file_to_stdout()
  {
    if( infile!=NULL ) { DeleteFile(infile); corefree(infile); }
    if( outfile!=NULL ) { DeleteFile(outfile); corefree(outfile); }
  }
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods)
   * ICA_INDEXDOCUMENT: dataptr is unused
   * ICA_COUNTWORDS: dataptr points to word counter variable of int type */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr);

  friend THREAD_PROC(ext_file_to_stdout_thread_proc);
};
struct ext_file_to_stdout_thread_data /*{{{*/
{
  t_flstr *command;
  t_input_convertor_ext_file_to_stdout *ic;
  bool result; // true on succes, false on error
  cdp_t cdp;
  ext_file_to_stdout_thread_data(t_flstr *_command,t_input_convertor_ext_file_to_stdout *_ic,cdp_t _cdp):
    command(_command),ic(_ic),result(false),cdp(_cdp) {};
}; /*}}}*/

THREAD_PROC(ext_file_to_stdout_thread_proc) /*{{{*/
{
  ext_file_to_stdout_thread_data *dataptr=(ext_file_to_stdout_thread_data*)data;
#ifdef WINS
  HANDLE hOutputReadTmp,hOutputRead,hOutputWrite;
  HANDLE hInputWriteTmp,hInputRead,hInputWrite;
  HANDLE hErrorWrite;
  SECURITY_ATTRIBUTES sa;
  HANDLE hStdIn = NULL; // Handle to parents std input.
  
  
  // Set up the security attributes struct.
  sa.nLength= sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;
  
  
  // Create the child output pipe.
  if (!CreatePipe(&hOutputReadTmp,&hOutputWrite,&sa,0))
  {
    dataptr->ic->request_error_execution_failed(dataptr->command->get(),"CreatePipe() for child output failed");
    return 0;
  }
  
  
  // Create a duplicate of the output write handle for the std error
  // write handle. This is necessary in case the child application
  // closes one of its std output handles.
  if (!DuplicateHandle(GetCurrentProcess(),hOutputWrite,
    GetCurrentProcess(),&hErrorWrite,0,
    TRUE,DUPLICATE_SAME_ACCESS))
  { 
    // child output pipe handles
    CloseHandle(hOutputReadTmp);
    CloseHandle(hOutputWrite);
    dataptr->ic->request_error_execution_failed(dataptr->command->get(),"DuplicateHandle() for output write handle failed");
    return 0;
  }
  
  
  // Create the child input pipe.
  if (!CreatePipe(&hInputRead,&hInputWriteTmp,&sa,0))
  { 
    // child output pipe handles
    CloseHandle(hOutputReadTmp);
    CloseHandle(hOutputWrite);
    // duplicate of the output write handle
    CloseHandle(hErrorWrite);
    dataptr->ic->request_error_execution_failed(dataptr->command->get(),"CreatePipe() for child input failed");
    return 0;
  }
  
  
  // Create new output read handle and the input write handles. Set
  // the Properties to FALSE. Otherwise, the child inherits the
  // properties and, as a result, non-closeable handles to the pipes
  // are created.
  if (!DuplicateHandle(GetCurrentProcess(),hOutputReadTmp,
    GetCurrentProcess(),
    &hOutputRead, // Address of new handle.
    0,FALSE, // Make it uninheritable.
    DUPLICATE_SAME_ACCESS))
  { 
    // child output pipe handles
    CloseHandle(hOutputReadTmp);
    CloseHandle(hOutputWrite);
    // duplicate of the output write handle
    CloseHandle(hErrorWrite);
    // child input pipe handles
    CloseHandle(hInputRead);
    CloseHandle(hInputWriteTmp);
    dataptr->ic->request_error_execution_failed(dataptr->command->get(),"DuplicateHandle() for output read handle failed");
    return 0;
  }
  
  if (!DuplicateHandle(GetCurrentProcess(),hInputWriteTmp,
    GetCurrentProcess(),
    &hInputWrite, // Address of new handle.
    0,FALSE, // Make it uninheritable.
    DUPLICATE_SAME_ACCESS))
  { 
    // child output pipe handles
    CloseHandle(hOutputReadTmp);
    CloseHandle(hOutputWrite);
    // duplicate of the output write handle
    CloseHandle(hErrorWrite);
    // child input pipe handles
    CloseHandle(hInputRead);
    CloseHandle(hInputWriteTmp);
    // duplicate of 
    CloseHandle(hOutputRead);
    dataptr->ic->request_error_execution_failed(dataptr->command->get(),"DuplicateHandle() for input write handle failed");
    return 0;
  }
  
  
  // Close inheritable copies of the handles you do not want to be
  // inherited.
  if (!CloseHandle(hOutputReadTmp))
  { 
    // child output pipe handles
    CloseHandle(hOutputWrite);
    // duplicate of the output write handle
    CloseHandle(hErrorWrite);
    // child input pipe handles
    CloseHandle(hInputRead);
    CloseHandle(hInputWriteTmp);
    // duplicate of 
    CloseHandle(hOutputRead);
    CloseHandle(hInputWrite);
    dataptr->ic->request_error_execution_failed(dataptr->command->get(),"CloseHandle() for output read handle failed");
    return 0;
  }
  if (!CloseHandle(hInputWriteTmp))
  { 
    // child output pipe handles
    CloseHandle(hOutputWrite);
    // duplicate of the output write handle
    CloseHandle(hErrorWrite);
    // child input pipe handles
    CloseHandle(hInputRead);
    // duplicate of 
    CloseHandle(hOutputRead);
    CloseHandle(hInputWrite);
    dataptr->ic->request_error_execution_failed(dataptr->command->get(),"CloseHandle() for input write handle failed");
    return 0;
  }
   
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  DWORD creation_flags;
  
  // Set up the start up info struct.
  ZeroMemory(&si,sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
  si.hStdOutput = hOutputWrite;
  si.hStdInput  = hInputRead;
  si.hStdError  = hErrorWrite;
  si.wShowWindow = SW_HIDE;
  creation_flags=CREATE_NO_WINDOW|CREATE_NEW_CONSOLE;
  if( dataptr->ic->is_transaction_empty() ) creation_flags|=BELOW_NORMAL_PRIORITY_CLASS;
  // Use this if you want to hide the child:
  //     si.wShowWindow = SW_HIDE;
  // Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
  // use the wShowWindow flags.
  
  
  // set error mode that will be inherited by child process
  sem_for_convertor_childs.BeforeChildLaunch();
  // Launch the process that you want to redirect (in this case,
  // Child.exe). Make sure Child.exe is in the same directory as
  // redirect.c launch redirect from a command line to prevent location
  // confusion.
  if (!CreateProcess(NULL,dataptr->command->get(),NULL,NULL,TRUE,creation_flags,NULL,NULL,&si,&pi))
  { 
    sem_for_convertor_childs.AfterChildLaunch();
    // child output pipe handles
    CloseHandle(hOutputWrite);
    // duplicate of the output write handle
    CloseHandle(hErrorWrite);
    // child input pipe handles
    CloseHandle(hInputRead);
    // duplicate of 
    CloseHandle(hOutputRead);
    CloseHandle(hInputWrite);
    dataptr->ic->request_error_execution_failed(dataptr->command->get(),"CreateProcess() failed");
    return 0;
  }
  // revert to original error mode
  sem_for_convertor_childs.AfterChildLaunch();
  
  
  
  // Close pipe handles (do not continue to modify the parent).
  // You need to make sure that no handles to the write end of the
  // output pipe are maintained in this process or else the pipe will
  // not close when the child process exits and the ReadFile will hang.
  CloseHandle(hOutputWrite);
  CloseHandle(hInputRead );
  CloseHandle(hErrorWrite);
  
  // Read the child's output.
  {
    t_input_stream_HFILE isHF(dataptr->cdp,hOutputRead);
    // save child's output into outfile temporary file
    if( !dataptr->ic->input_stream2file(isHF,dataptr->ic->outfile) )
    {
      // input_stream2file() can fail because the user has cancelled the execution - we have to test if the process is still active and terminate it
      DWORD dwProcessExitCode=0;
      if( GetExitCodeProcess(pi.hProcess,&dwProcessExitCode) )
      {
        if( dwProcessExitCode==STILL_ACTIVE )
        {
          TerminateProcess(pi.hProcess,0);
        }
      }
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
      CloseHandle(hOutputRead);
      CloseHandle(hInputWrite);
      return 0;
    }
  }

  if (!CloseHandle(pi.hProcess))
  { 
    // duplicate of 
    CloseHandle(hOutputRead);
    CloseHandle(hInputWrite);
    // process and thread handles
    CloseHandle(pi.hThread);
    dataptr->ic->request_error_execution_failed(dataptr->command->get(),"CloseHandle() for child process handle failed");
    return 0;
  } 
  if (!CloseHandle(pi.hThread))
  { 
    // duplicate of 
    CloseHandle(hOutputRead);
    CloseHandle(hInputWrite);
    dataptr->ic->request_error_execution_failed(dataptr->command->get(),"CloseHandle() for child thread handle failed");
    return 0;
  }
  
  
  // Redirection is complete
  CloseHandle(hOutputRead);
  CloseHandle(hInputWrite);
#else
  FILE *fpipe;
  fpipe=popen(dataptr->command->get(),"r");
  if( fpipe==NULL ) { dataptr->ic->request_error_execution_failed(dataptr->command->get(),"popen() failed"); return 0; }
  // create t_input_stream instance for pipe
  {
    t_input_stream_pipe isp(dataptr->cdp,fpipe);
    // save contents of pipe into outfile temporary file
    if( !dataptr->ic->input_stream2file(isp,dataptr->ic->outfile) )
    {
      pclose(fpipe);
      return 0;
    }
    // destroy t_input_stream instance for outfile
    // in its destructor
  }
  // close pipe
  pclose(fpipe);
#endif
  dataptr->result=true;
#ifdef WINS
  return 1;
#else
  return (void*)1; 
#endif
}/*}}}*/
bool t_input_convertor_ext_file_to_stdout::execute_statement(t_flstr &command) /*{{{*/
{
  ext_file_to_stdout_thread_data data(&command,this,cdp);
  THREAD_HANDLE hThread;
  // start thread
  if( THREAD_START_JOINABLE(ext_file_to_stdout_thread_proc,10000,(void*)&data,&hThread) )
  {
    // wait till thread finishes
    THREAD_JOIN(hThread);
    return data.result;
  }
  else
  {
    request_error_execution_failed(command.get(),"THREAD_START_JOINABLE() failed");
    return false;
  }
} /*}}}*/
bool t_input_convertor_ext_file_to_stdout::perform_action(input_convertor_actions_t action,void *dataptr) /*{{{*/
{
  if( outfile==NULL )
  {
    if( !create_tmp_filename(infile) || !create_tmp_filename(outfile) ) return false;
    // save input stream to file
    if( !input_stream2file(*is,infile) ) return false;
    // compose command
    t_flstr command(400,300);
    if( !compose_statement(command,infile) ) return false;
    command.putc('\0');
    // execute command
    if( !execute_statement(command) ) return false;
    /* outfile is result of conversion */
  }
  {
    t_input_stream_file tmpis(cdp,outfile,is->get_filename_for_limits()/* LIMITS has to be tested with filename of original input stream */);
    t_input_stream *previs=is;
    is=&tmpis;
    if( !perform_action_on_output_file(action,dataptr) )
    {
      is=previs;
      return false;
    }
    is=previs;
    return true;
  }
} /*}}}*/
/*}}}*/
/* class t_input_convertor_antiword {{{*/
class t_input_convertor_antiword:public t_input_convertor_ext_file_to_stdout
{
protected:
  virtual bool compose_statement(t_flstr &command,const char *infile);
  virtual bool perform_action_on_output_file(input_convertor_actions_t action,void *dataptr);
public:
  t_input_convertor_antiword(cdp_t cdp_param):t_input_convertor_ext_file_to_stdout(cdp_param) {};
  virtual ~t_input_convertor_antiword() {};
};
bool t_input_convertor_antiword::compose_statement(t_flstr &command,const char *infile) /*{{{*/
{
  if( !append_exe(command,"antiword","antiword","exe") ) return false;
  if( !command.put(" -m UTF-8.txt ") ) goto antiword_compose_statement_error;
  if( !command.putc('\"') ) goto antiword_compose_statement_error;
  if( !command.put(infile) ) goto antiword_compose_statement_error;
  if( !command.putc('\"') ) goto antiword_compose_statement_error;
  return true;
antiword_compose_statement_error:
  request_error_out_of_memory();
  return false;
} /*}}}*/
bool t_input_convertor_antiword::perform_action_on_output_file(input_convertor_actions_t action,void *dataptr) /*{{{*/
{
  t_input_convertor_plain ic(cdp);
  ic.set_agent(agent_data);
  ic.set_stream(is);
  ic.set_specif(t_specif(0,CHARSET_NUM_UTF8,0,0));
  ic.set_limits_evaluator(limits_evaluator);
  ic.set_depth_level(depth_level);
  ic.set_owning_ft(working_for);
  return ic.perform_action(action,dataptr);
} /*}}}*/
/*}}}*/
/* class t_input_convertor_zip {{{
 * This convertor indexes zip files and OOo documents.
 * OOo documents are zip files that contain 'mimetype' file whose content
 * is 'application/vnd.sun.xml.SOMETHING' or 'application/vnd.oasis.opendocument.SOMETHING'
 * and 'content.xml' file. */
class t_input_convertor_zip:public t_input_convertor_ext_file_base
{
protected:
  // fully qualified file name of zip file
  // points to corealloc-ed buffer or is NULL
  // have to be corefree-ed in destructor
  char *infile;
  bool tested_for_ooo, // =false when zip archive wasn't yet tested whether is OOo document, =true when it was tested
    is_ooo_document; // result of test, i.e. =false when this is normal zip archive, =true when this is OOo document

  void request_error_unzip(const char *situation);

  /* saves input stream into temporary file, stores its name into infile buffer
   * returns true on success, false on error */
  bool instream2infile();
public:
  t_input_convertor_zip(cdp_t cdp_param):t_input_convertor_ext_file_base(cdp_param) { infile=NULL; tested_for_ooo=false; is_ooo_document=false; };
  virtual ~t_input_convertor_zip();
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods)
   * For zip archive: ICA_COUNTWORDS returns 1 (one word in document) to ensure that zip archive will be indexed
   *                  ICA_INDEXDOCUMENT unzips files and indexes them
   * For OOo document: ICA_COUNTWORDS returns number of words in document
   *                   ICA_INDEXDOCUMENT indexes document */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr);
};
void t_input_convertor_zip::request_error_unzip(const char *situation) /*{{{*/
{
  request_errorf("Fulltext: Error during decompression of zip archive \"%s\" (%s).",infile,situation);
} /*}}}*/
t_input_convertor_zip::~t_input_convertor_zip() /*{{{*/
{
  if( infile!=NULL )
  {
    if( exists_file(infile) ) DeleteFile(infile);
    corefree(infile);
  }
} /*}}}*/ 
bool t_input_convertor_zip::perform_action(input_convertor_actions_t action,void *dataptr) /*{{{*/
{
  if( !tested_for_ooo )
  {
    // save zip stream into temporary file
    if( !instream2infile() ) return false;
    {
      char *filename;
      if( !create_tmp_filename(filename) ) return false;
      {
        wchar_t wname[MAX_PATH];
        INT_PTR hUnZip;
        superconv(ATT_STRING, ATT_STRING, infile, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), t_specif(0, 0, 0, 1), NULL);
        if ( UnZipOpen(wname, &hUnZip) ) { request_error_unzip("can't open zip archive"); corefree(filename); return false; }
        // have to corefree() filename
        INT_PTR hFind, hFile, hDoc;
        bool OOo_mimetype_found=false,OOo_content_xml_found=false;
        /* detect special zip files (i.e. OOo documents) 
         * OOo document is zip file which contains 'mimetype' file whose content is 'application/vnd.sun.xml.SOMETHING'
         * and 'content.xml' file */
        int Err;
        for( Err=UnZipFindFirst(hUnZip, NULL, true, &hFind, &hFile);Err==Z_OK;Err=UnZipFindNext(hFind, &hFile) )
        {
          if (!UnZipIsFolder(hUnZip, hFile))
          {
            UnZipGetFileName(hUnZip, hFile, wname, sizeof(wname) / sizeof(wchar_t));
            if( wcsicmp(wname,L"mimetype")==0 )
            {
              int unziperr;
              superconv(ATT_STRING, ATT_STRING, filename, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), t_specif(0, 0, 0, 1), NULL);
              if( (unziperr=UnZipFile(hUnZip, hFile, wname)) )
              {
                if( unziperr==Z_BADPASSWORD )
                {
                  corefree(filename);
                  UnZipClose(hUnZip);
                  return true;
                }
                request_error_unzip("can't decompress file from zip archive into temporary file");
                corefree(filename);
                UnZipClose(hUnZip);
                return false;
              }
              // have to delete filename
              char buf[101];
              FILE *f=fopen(filename,"rb");
              if( f==NULL ) { request_error_unzip("can't open decompressed file"); DeleteFile(filename); corefree(filename); UnZipClose(hUnZip); return false; }
              size_t len;
              if( (len=fread(buf,sizeof(char),100,f))==0 )
              {
                if( ferror(f) ) { fclose(f); DeleteFile(filename); corefree(filename); request_error_unzip("can't read from decompressed file"); UnZipClose(hUnZip); return false; }
              }
              else
              {
                buf[len]='\0';
                if( strnicmp(buf,"application/vnd.sun.xml.",24)==0 || strnicmp(buf,"application/vnd.oasis.opendocument.",35)==0 )
                {
                  OOo_mimetype_found=true;
                  if( OOo_content_xml_found ) { fclose(f); DeleteFile(filename); break; }
                }
              }
              fclose(f);
              DeleteFile(filename);
            }
            else if( wcsicmp(wname,L"content.xml")==0 )
            {
              hDoc = hFile;
              OOo_content_xml_found=true;
              if( OOo_mimetype_found ) break;
            }
          }
        }
        if (Err < 0)
        { request_error_unzip("can't decompress file from zip archive into temporary file"); corefree(filename); UnZipClose(hUnZip); return false; }
        is_ooo_document=(OOo_mimetype_found && OOo_content_xml_found);
        tested_for_ooo=true;
        if( is_ooo_document )
        {
          /* unzip content.xml into temporary file and its filename store into infile variable
           * then delete temporary zip file */
          superconv(ATT_STRING, ATT_STRING, filename, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), t_specif(0, 0, 0, 1), NULL);
          if( UnZipFile(hUnZip, hDoc, wname) ) { request_error_unzip("can't decompress content of the OOo document into temporary file"); corefree(filename); UnZipClose(hUnZip); return false; }
          // infile = zip filename
          // filename = content.xml filename
        }
        UnZipClose(hUnZip);
      }
      if( is_ooo_document )
      {
        DeleteFile(infile);
        corefree(infile);
        infile=filename; // set infile to filename of decompressed content.xml file
      }
      else corefree(filename);
    }
  }

  if( is_ooo_document )
  {
    t_input_stream_file tmpis(cdp,infile,is->get_filename_for_limits()/* LIMITS has to be tested with filename of original input stream */);
    t_input_convertor_xml ic(cdp);
    // set convertor data
    ic.set_stream(&tmpis);
    ic.set_agent(agent_data);
    ic.set_depth_level(depth_level);
    ic.set_limits_evaluator(limits_evaluator);
    ic.set_owning_ft(working_for);
    // perform action
    return ic.perform_action(action,dataptr);
  } 
  else
  {
    if( action==ICA_COUNTWORDS )
    {
      ICA_COUNTWORDS_initialize_counter(dataptr);
      ICA_COUNTWORDS_inc_counter(dataptr); // to 1
      return true;
    }
    else if( action==ICA_INDEXDOCUMENT )
    {
      INT_PTR hUnZip, hFind, hFile;
      wchar_t wname[MAX_PATH];
      superconv(ATT_STRING, ATT_STRING, infile, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), t_specif(0, 0, 0, 1), NULL);
      if( UnZipOpen(wname, &hUnZip) ) { request_error_unzip("can't open zip archive"); return false; }
      char *filename;
      if( !create_tmp_filename(filename) ) return false;
      // have to corefree() filename
      // test if we've reached highest allowed depth level
      if( !is_depth_level_allowed() )
      {
        corefree(filename);
        return false;
      }
      // for each file: unzip it, build fulltext index
      int Err;
      for( Err=UnZipFindFirst(hUnZip, NULL, true, &hFind, &hFile);Err==Z_OK;Err=UnZipFindNext(hFind, &hFile) )
      {
        if (!UnZipIsFolder(hUnZip, hFile))
        {
          int unziperr;
          char fname[MAX_PATH];
          *fname = '\0';
          UnZipGetFileName(hUnZip, hFile, wname, sizeof(wname) / sizeof(wchar_t));
          superconv(ATT_STRING, ATT_STRING, (char *)wname, fname, t_specif(0, 0, 0, 1), t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), NULL);
          superconv(ATT_STRING, ATT_STRING, filename, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), t_specif(0, 0, 0, 1), NULL);
          if( (unziperr=UnZipFile(hUnZip, hFile, wname)) )
          { // if the archive is password-protected skip it without reporting an error
            if( unziperr==Z_BADPASSWORD ) break;
            // report error
            request_error_unzip("can't decompress file from zip archive into temporary file");
            corefree(filename);
            UnZipClose(hUnZip);
            return false;
          }
          /* build ftx index for filename */
          // prepare input stream
          t_input_stream_file *tmpis=new t_input_stream_file(cdp,filename,fname/*LIMITS has to be tested with name of zipped file*/);
          if( tmpis==NULL ) { DeleteFile(filename); corefree(filename); return false; }
          // select convertor
          t_input_convertor *ic=ic_from_text(cdp,tmpis);
          if( ic==NULL ) { delete tmpis; DeleteFile(filename); corefree(filename); return false; } // TODO conversion not supported - hlasit tuto chybu?
#ifdef WINS
          // switch from backslashes to slashes on Windows
          for( char *tmp=fname;*tmp!='\0';tmp++ )
          {
            if( *tmp=='\\' ) *tmp='/';
          }
#endif
          /* we don't have to test if the user cancelled the request (see t_input_convertor::was_breaked())
           * because it's tested in ic->convert(), it returns false in this case */
          // prepare subdocument relative path
          t_flstr subdoc_relpath;
          if( !make_subdocument_relpath(fname,subdoc_relpath) ) { delete ic; delete tmpis; DeleteFile(filename); corefree(filename); return false; }
          // set convertor data
          ic->set_stream(tmpis);
          ic->copy_parameters(this);
          ic->set_subdocument_prefix(subdoc_relpath.get()/*keep this pointer valid until ic is destroyed*/);
          agent_data->starting_document_part(subdoc_relpath.get());
          // build index
          if( !ic->convert() ) { delete ic; delete tmpis; DeleteFile(filename); corefree(filename); return false; } // TODO hlasit tuto chybu?
          delete ic;
          delete tmpis;
          // delete tmp. file
          DeleteFile(filename);
        }
      }
      if (Err < 0)
      { request_error_unzip("can't decompress file from zip archive into temporary file"); corefree(filename); UnZipClose(hUnZip); return false; }
      corefree(filename);
      UnZipClose(hUnZip);
    } 
    else return false; // invalid action
    return true;
  }
} /*}}}*/
bool t_input_convertor_zip::instream2infile() /*{{{*/
{
  assert(infile==NULL);
  assert(is!=NULL);
  if( !create_tmp_filename(infile,"zip") ) return false;
  return input_stream2file(*is,infile);
} /*}}}*/
/*}}}*/
/* class t_input_convertor_rtf {{{*/
class t_input_convertor_rtf:public t_input_convertor_ext_file_to_file
{
protected:
  char *myargv[3],*infile_quoted,*outfile_quoted;
  virtual bool compose_statement(t_flstr &command,char **&argv,const char *infile,const char *outfile,char *&dll_path);
  virtual bool perform_action_on_output_file(input_convertor_actions_t action,void *dataptr);
  // returns true on success, false on error and logs error with request_error()
  // ensures that lines end with 0x0D0A
  virtual bool input_stream2file(t_input_stream &is,const char *filename);
public:
  t_input_convertor_rtf(cdp_t cdp_param):t_input_convertor_ext_file_to_file(cdp_param) { infile_quoted=outfile_quoted=NULL; };
  virtual ~t_input_convertor_rtf()
  {
    if( infile_quoted!=NULL ) corefree(infile_quoted);
    if( outfile_quoted!=NULL ) corefree(outfile_quoted);
  };
  virtual const char *get_infile_extension() { return "rtf"; }
};
bool t_input_convertor_rtf::compose_statement(t_flstr &command,char **&argv,const char *infile,const char *outfile,char *&dll_path) /*{{{*/
{
  if( (infile_quoted=corestr_quote_filename(infile))==NULL ) return false;
  if( (outfile_quoted=corestr_quote_filename(outfile))==NULL ) return false;
  if( !append_exe(command,"rtf2html","rtf2html","exe") ) return false;
  // fill myargv array
  myargv[0]=infile_quoted;
  myargv[1]=outfile_quoted;
  myargv[2]=NULL; // last item have to be NULL!
  // set argv param
  argv=myargv;
  dll_path=NULL;
  return true;
} /*}}}*/
bool t_input_convertor_rtf::perform_action_on_output_file(input_convertor_actions_t action,void *dataptr) /*{{{*/
{
  bool ret;
  {
    t_input_convertor_html ic(cdp);
    ic.set_agent(agent_data);
    ic.set_stream(is);
    ic.set_depth_level(depth_level);
    ic.set_limits_evaluator(limits_evaluator);
    ic.set_owning_ft(working_for);
    ret=ic.perform_action(action,dataptr);
  }
  // delete image directory
  if( outfile!=NULL )
  {
    char imagedir[MAX_PATH];
    strcpy(imagedir,outfile); strcat(imagedir,".files");
    delete_directory(imagedir);
  }
  return ret;
} /*}}}*/
bool t_input_convertor_rtf::input_stream2file(t_input_stream &is,const char *filename) /*{{{*/
{
  FILE *f=fopen(filename,"wb");
  if( f==NULL ) { request_errorf("Fulltext: Can't open temporary file \"%s\" for write.",filename); return false; }
  for( ;; )
  {
    char buf[1024];
    int readed,last_char_is_0D,first_idx_to_write,idx;
    if( (readed=is.read(buf,sizeof(buf)))<=0 ) break;
    last_char_is_0D=(buf[readed-1]==0x0d);
    first_idx_to_write=idx=0;
    for(;idx<readed;)
    {
      while( idx<readed && buf[idx]!=0x0a ) idx++;
      if( buf[idx]==0x0a )
      {
        if( (idx==0 && !last_char_is_0D) || (idx>0 && buf[idx-1]!=0x0d) )
        {
          // write chars before 0x0a
          if( idx>first_idx_to_write )
          {
            if( fwrite(buf+first_idx_to_write,sizeof(char),idx-first_idx_to_write,f)<idx-first_idx_to_write )
            {
              request_error("Fulltext: Error while writing document into temporary file.");
              fclose(f);
              return false;
            }
          }
          // write 0x0d
          if( fputc(0x0d,f)==EOF )
          {
            request_error("Fulltext: Error while writing document into temporary file.");
            fclose(f);
            return false;
          }
          // set first_idx_to_write
          first_idx_to_write=idx; // next time we will write 0x0a char
          // skip 0x0a
          idx++;
        }
        else idx++;
      }
    }
    // write the rest of the buffer
    if( idx-first_idx_to_write>0 )
    {
      if( fwrite(buf+first_idx_to_write,sizeof(char),idx-first_idx_to_write,f)<idx-first_idx_to_write )
      {
        request_error("Fulltext: Error while writing document into temporary file.");
        fclose(f);
        return false;
      }
    }
  }
  fclose(f);
  return true;
} /*}}}*/
/*}}}*/
/* abstract class t_input_convertor_ooo_base {{{*/
class t_input_convertor_ooo_base:public t_input_convertor_ext_file_to_file
{
protected:
#define INFILE_EXT_LENGTH 5
  char infile_ext[INFILE_EXT_LENGTH+1];
  char *ooo_infile;
  virtual const char *get_infile_extension() { return infile_ext; };
  virtual const char *get_outfile_extension() { return "zip"; };

  void request_error_unzip(const char *situation);

  virtual bool compose_statement(t_flstr &command,char **(&argv),const char *infile,const char *outfile,char *&dll_path) = 0;
  virtual bool perform_action_on_output_file(input_convertor_actions_t action,void *dataptr);
public:
  t_input_convertor_ooo_base(cdp_t cdp_param,const char *_infile_ext):t_input_convertor_ext_file_to_file(cdp_param) { strncpy(infile_ext,_infile_ext,INFILE_EXT_LENGTH); infile_ext[INFILE_EXT_LENGTH]='\0'; ooo_infile=NULL; };
  virtual ~t_input_convertor_ooo_base();
};
t_input_convertor_ooo_base::~t_input_convertor_ooo_base() /*{{{*/
{
  if( ooo_infile!=NULL )
  {
    if( exists_file(ooo_infile) ) DeleteFile(ooo_infile);
    corefree(ooo_infile);
  }
} /*}}}*/
void t_input_convertor_ooo_base::request_error_unzip(const char *situation) /*{{{*/
{
  request_errorf("Fulltext: Error during decompression of OOo document \"%s\" (%s).",infile,situation);
} /*}}}*/
bool t_input_convertor_ooo_base::perform_action_on_output_file(input_convertor_actions_t action,void *dataptr) /*{{{*/
{
  if( ooo_infile==NULL )
  {
    // save zip stream into temporary file ooo_infile
    if( !create_tmp_filename(ooo_infile,"zip") ) return false;
    if( !input_stream2file(*is,ooo_infile) ) return false;
    // open zip file and extract content.xml file
    {
      char *filename;
      if( !create_tmp_filename(filename) ) return false;
      // have to corefree() filename
      {
        wchar_t wname[MAX_PATH];
        INT_PTR hUnZip, hFind, hFile, hDoc;
        superconv(ATT_STRING, ATT_STRING, ooo_infile, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), t_specif(0, 0, 0, 1), NULL);
        if( UnZipOpen(wname, &hUnZip) ) { request_error_unzip("can't open zip archive"); corefree(filename); return false; }
        bool OOo_mimetype_found=false,OOo_content_xml_found=false;
        /* detect special zip files (i.e. OOo documents) 
         * OOo document is zip file which contains 'mimetype' file whose content is 'application/vnd.sun.xml.SOMETHING'
         * and 'content.xml' file */
        int Err;
        for( Err=UnZipFindFirst(hUnZip, NULL, true, &hFind, &hFile);Err==Z_OK;Err=UnZipFindNext(hFind, &hFile) )
        {
          if ( !UnZipIsFolder(hUnZip, hFile) )
          {
            UnZipGetFileName(hUnZip, hFile, wname, sizeof(wname) / sizeof(wchar_t));
            if( wcsicmp(wname,L"mimetype")==0 )
            {
              int unziperr;
              superconv(ATT_STRING, ATT_STRING, filename, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), t_specif(0, 0, 0, 1), NULL);
              if( (unziperr=UnZipFile(hUnZip, hFile, wname)) )
              { // if the archive is password-protected skip it without reporting an error
                if( unziperr==Z_BADPASSWORD )
                {
                  corefree(filename);
                  UnZipClose(hUnZip);
                  return true;
                }
                // report error
                request_error_unzip("can't decompress file from zip archive into temporary file");
                corefree(filename);
                UnZipClose(hUnZip);
                return false;
              }
              char buf[101];
              FILE *f=fopen(filename,"rb");
              if( f==NULL ) { request_error_unzip("can't open decompressed file"); corefree(filename); UnZipClose(hUnZip); return false; }
              size_t len;
              if( (len=fread(buf,sizeof(char),100,f))==0 )
              { 
                if( ferror(f) ) { fclose(f); DeleteFile(filename); corefree(filename); request_error_unzip("can't read from decompressed file"); UnZipClose(hUnZip); return false; }
              }
              else
              {
                buf[len]='\0';
                if( strnicmp(buf,"application/vnd.sun.xml.",24)==0 || strnicmp(buf,"application/vnd.oasis.opendocument.",35)==0 )
                {
                  OOo_mimetype_found=true;
                  if( OOo_content_xml_found ) { fclose(f); DeleteFile(filename); break; }
                }
              }
              fclose(f);
              DeleteFile(filename);
            }
            else if( wcsicmp(wname,L"content.xml")==0 )
            {
              hDoc = hFile;
              OOo_content_xml_found=true;
              if( OOo_mimetype_found ) break;
            }
          }
        }
        if (Err<0)
        { request_error_unzip("can't decompress file from zip archive into temporary file"); corefree(filename); UnZipClose(hUnZip); return false; }
        // test if this is an OOo document
        if( !OOo_mimetype_found && !OOo_content_xml_found )
        {
          request_error("Fulltext: Result of conversion isn't OOo document.");
          corefree(filename);
          return false;
        }
        superconv(ATT_STRING, ATT_STRING, filename, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), t_specif(0, 0, 0, 1), NULL);
        if( UnZipFile(hUnZip, hDoc, wname) ) { request_error_unzip("can't decompress file from zip archive into temporary file"); corefree(filename); UnZipClose(hUnZip); return false; }
        UnZipClose(hUnZip);
      }
      // filename is content.xml extracted file
      // ooo_infile is zip file (now closed)
      DeleteFile(ooo_infile); // delete temporary zip file
      corefree(ooo_infile);
      ooo_infile=filename; // set ooo_infile to extracted content.xml file - this temporary file will be deleted in destructor
    }
    /* ooo_infile is filename of extracted content.xml file */
  }
  // prepare input stream
  t_input_stream_file tmpis(cdp,ooo_infile,is->get_filename_for_limits()/* LIMITS has to be tested with filename of original input stream */);
  // use XML convertor
  t_input_convertor_xml ic(cdp);
  // set convertor data
  ic.set_stream(&tmpis);
  ic.set_agent(agent_data);
  ic.set_depth_level(depth_level);
  ic.set_limits_evaluator(limits_evaluator);
  ic.set_owning_ft(working_for);
  // perform action
  return ic.perform_action(action,dataptr);
}; /*}}}*/
/*}}}*/
/* class t_input_convertor_using_ooo {{{*/
class t_input_convertor_using_ooo:public t_input_convertor_ooo_base
{
protected:
  char *myargv[3]; // myargv[1] member can be corealloc()-ed => we have to corefree() it in destructor
  virtual bool compose_statement(t_flstr &command,char **(&argv),const char *infile,const char *outfile,char *&dll_path);
  // we will test exitence of <filename>.err
  virtual bool test_after_execution();
public:
  t_input_convertor_using_ooo(cdp_t cdp_param,const char *_infile_ext):t_input_convertor_ooo_base(cdp_param,_infile_ext) { myargv[1]=NULL; };
  virtual ~t_input_convertor_using_ooo()
  {
    if( myargv[1]!=NULL ) corefree(myargv[1]);
  };
};
bool t_input_convertor_using_ooo::compose_statement(t_flstr &command,char **(&argv),const char *infile,const char *outfile,char *&dll_path) /*{{{*/
{
  // test if is enabled to use OOo
  if( convertor_params.Get_ooo_usage_type()!=OOO_STANDALONE ) return false;
  // replace name of output file - OOo macro creates file with name <infile>.zip
  corefree(this->outfile);
  this->outfile=(char*)corealloc(strlen(infile)+5,COREALLOC_OWNER_ID);
  if( this->outfile==NULL ) goto using_ooo_compose_statement_error;
  strcpy(this->outfile,infile); strcat(this->outfile,".zip");
  outfile=this->outfile;
  // get name of soffice binary
  if( !command.put(convertor_params.Get_soffice_binary()) ) goto using_ooo_compose_statement_error;
  // prepare myargv array
  myargv[0]="-headless";
  myargv[1]=(char*)corealloc(strlen(infile)+50,COREALLOC_OWNER_ID);
  if( myargv[1]==NULL ) goto using_ooo_compose_statement_error;
  sprintf(myargv[1],"\"macro:///Tools.ftx602.ExportAsOOo(%s)\"",infile);
  myargv[2]=NULL; // last item have to be always NULL
  // set argv param
  argv=myargv;
  dll_path=NULL;
  return true;
using_ooo_compose_statement_error:
  request_error_out_of_memory();
  return false;
} /*}}}*/
bool t_input_convertor_using_ooo::test_after_execution() /*{{{*/
{
  t_flstr errfilename;
  /* If OOo macro encouters error it creates <filename>.err file with error message
   * instead of <filename>.zip regular output file. */
  errfilename.put(infile);
  errfilename.put(".err");
  errfilename.putc('\0');
  if( errfilename.error() ) { request_error_out_of_memory(); return false; }
  if( exists_file(errfilename.get()) )
  {
    char errmsg[4096];
    strcpy(errmsg,"OpenOffice.org failed to convert document"); // set default error message
    // get error message from <filename>.err
    FILE *f=fopen(errfilename.get(),"rb");
    if( f!=NULL )
    {
      fread(errmsg,sizeof(char),sizeof(errmsg),f);
      errmsg[sizeof(errmsg)-1]='\0';
      fclose(f);
    }
    // raise error
    request_error_execution_failed("OpenOffice.org",errmsg);
    // delete err file
    DeleteFile(errfilename.get());
    return false;
  }
  return true;
} /*}}}*/
/*}}}*/
/* class t_input_convertor_ooo_client {{{*/
class t_input_convertor_ooo_client:public t_input_convertor_ooo_base
{
protected:
  char *myargv[4]; // myargv[2] member can be corealloc()-ed => we have to corefree() it in destructor
  char *infile_quoted,*outfile_quoted;
  virtual bool compose_statement(t_flstr &command,char **&argv,const char *infile,const char *outfile,char *&dll_path);
  // we will test exitence of <filename>.err
  virtual bool test_after_execution();
public:
  t_input_convertor_ooo_client(cdp_t cdp_param,const char *_infile_ext):t_input_convertor_ooo_base(cdp_param,_infile_ext) { myargv[2]=infile_quoted=outfile_quoted=NULL; };
  virtual ~t_input_convertor_ooo_client()
  {
    if( myargv[2]!=NULL ) corefree(myargv[2]);
    if( infile_quoted!=NULL ) corefree(infile_quoted);
    if( outfile_quoted!=NULL ) corefree(outfile_quoted);
  };
};
bool t_input_convertor_ooo_client::compose_statement(t_flstr &command,char **&argv,const char *infile,const char *outfile,char *&dll_path) /*{{{*/
{
  // test if is enabled to use OOo
  if( convertor_params.Get_ooo_usage_type()!=OOO_SERVER ) return false;
  if( (infile_quoted=corestr_quote_filename(infile))==NULL ) return false;
  if( (outfile_quoted=corestr_quote_filename(outfile))==NULL ) return false;
  if( !append_exe(command,"ooo11cvt","ooo11cvt","exe") ) return false;
  // prepare myargv array
  myargv[0]=infile_quoted;
  myargv[1]=outfile_quoted;
  myargv[2]=(char*)corealloc(strlen(convertor_params.Get_ooo_hostname())+strlen(convertor_params.Get_ooo_port())+50,COREALLOC_OWNER_ID);
  if( myargv[2]==NULL ) goto ooo_client_compose_statement_error;
  sprintf(myargv[2],"\"uno:socket,host=%s,port=%s;urp;StarOffice.ServiceManager\"",convertor_params.Get_ooo_hostname(),convertor_params.Get_ooo_port());
  myargv[3]=NULL; // last item have to be always NULL
  // set argv param
  argv=myargv;
  // set dll_path
  dll_path=(char*)convertor_params.Get_ooo_program_dir();
  return true;
ooo_client_compose_statement_error:
  request_error_out_of_memory();
  return false;
} /*}}}*/
bool t_input_convertor_ooo_client::test_after_execution() /*{{{*/
{
  t_flstr errfilename;
  /* If OOo macro encouters error it creates <filename>.err file with error message
   * instead of <filename>.zip regular output file. */
  errfilename.put(infile);
  errfilename.put(".err");
  errfilename.putc('\0');
  if( errfilename.error() ) { request_error_out_of_memory(); return false; }
  if( exists_file(errfilename.get()) )
  {
    char errmsg[4096];
    strcpy(errmsg,"OpenOffice.org failed to convert document"); // set default error message
    // get error message from <filename>.err
    FILE *f=fopen(errfilename.get(),"rb");
    if( f!=NULL )
    {
      fread(errmsg,sizeof(char),sizeof(errmsg),f);
      errmsg[sizeof(errmsg)-1]='\0';
      fclose(f);
    }
    // raise error
    request_error_execution_failed("OpenOffice.org",errmsg);
    // delete err file
    DeleteFile(errfilename.get());
    return false;
  }
  return true;
} /*}}}*/
/*}}}*/
#ifdef USE_EML_CONVERTOR
/* class t_input_convertor_eml + msgsupp.dll loader {{{*/
/* MERR_xxx & fzXXX defines {{{*/
#define MERR_OK													0
#define MERR_BASE												0x20000000 //nastaveny 29 bit => user message
#define MERR_READ												MERR_BASE + 1
#define MERR_WRITE											MERR_BASE + 2
#define MERR_OPEN												MERR_BASE + 3
#define MERR_CREATE											MERR_BASE + 4
#define MERR_MEMORY											MERR_BASE + 5
#define MERR_TMPREAD										MERR_BASE + 6
#define MERR_TMPWRITE										MERR_BASE + 7
#define MERR_TMPCREATE									MERR_BASE + 8
#define MERR_TMPOPEN										MERR_BASE + 9
#define MERR_FILEMAPPING								MERR_BASE + 10
#define MERR_INVALID_TNEF_SIGNATURE			MERR_BASE + 300
#define MERR_UNKNOWN_INCOMING_CHARSET		MERR_BASE + 400
#define MERR_UNKNOWN_OUTGOING_CHARSET		MERR_BASE + 401
#define MERR_MULTIBYTE_TO_WIDECHAR			MERR_BASE + 402
#define MERR_WIDECHAR_TO_MULTIBYTE			MERR_BASE + 403
#define MERR_BODYPART_BODY_NOT_DEFINED	MERR_BASE + 500
#define MERR_BODYPART_BODY_FILENAME_EMPTY		MERR_BASE + 501
#define MERR_OPEN_BODYPART_BODY_FILE				MERR_BASE + 502
#define MERR_BODYPART_BODY_DATA_NULL				MERR_BASE + 503
#define MERR_INVALID_UUPART_INDEX						MERR_BASE + 504
#define MERR_UUSYNTAX												MERR_BASE + 600
#define MERR_STREAM_NOT_OPENED							MERR_BASE + 601
#define MERR_NOT_ENOUGH_MEMORY							MERR_BASE + 602
#define MERR_NO_BLANK_LINE_AT_END						MERR_BASE + 603
#define MERR_MESSAGE_HEADER_SYNTAX_ERROR		MERR_BASE + 604
#define MERR_MULTIPART_HAS_NO_BOUNDARY			MERR_BASE + 605
#define MERR_MULTIPART_BOUNDARY_NOT_FOUND		MERR_BASE + 605
#define	MERR_INVALID_STRUCTURE_STREAM_DATA	MERR_BASE + 606

#define MERR_SMIMEInitError									MERR_BASE + 700
#define MERR_SMIMECryptoDataError						MERR_BASE + 701
#define MERR_SMIMECertificateNotFound       MERR_BASE + 702
#define MERR_SMIMEBadUserData               MERR_BASE + 703
#define MERR_SMIMECertificateExpired        MERR_BASE + 704
#define MERR_SMIMECancel                    MERR_BASE + 705
#define MERR_SMIMEBadSenderData						  MERR_BASE + 706
#define MERR_SMIMEBadRecipientData					MERR_BASE + 707
#define MERR_SMIMEIncompleteData					  MERR_BASE + 708
#define MERR_SMIMESecurityWarning           MERR_BASE + 709
#define MERR_SMIMEUnableToDecodeData        MERR_BASE + 710
#define MERR_SMIMEEmailDoNotMatch           MERR_BASE + 711
#define MERR_SMIMEMessageTampered           MERR_BASE + 712
#define MERR_SMIMESignerNotFound            MERR_BASE + 713
#define MERR_SMIMEInternalError							MERR_BASE + 714
#define MERR_SMIMEVerificationOk            MERR_BASE + 715
#define MERR_SMIMEError                     MERR_BASE + 716
#define MERR_SMIMEBPMissing									MERR_BASE + 717
#define MERR_SMIMEBPParsingError            MERR_BASE + 718
#define MERR_SMIMENotAllPartsCouldBeDecrypted	MERR_BASE + 719
#define MERR_SMIMENoCertificateAvailable		MERR_BASE + 720
#define MERR_SMIMENoSigningNotSuccessfull		MERR_BASE + 721
#define MERR_SMIMEMissingKeyForDecryption		MERR_BASE + 722
#define MERR_SMIMENotSMIMEMessage				 		MERR_BASE + 723

#define MERR_SMIMEUnknownError							MERR_BASE + 799

/* fzXXX defines - message flags */
#define		fzCertified 1
#define		fzSigned    2
#define		fzEncrypted	4
/*}}}*/
struct t_msgsupp_loader /*{{{*/
{
  HMODULE hLibrary;
  t_msgsupp_loader();
  ~t_msgsupp_loader();
  bool load_library();
  void free_library();
protected:
  // retrieves msgsupp.dll full pathname
  // stores it in dest buffer which have to be at least MAX_PATH long
  // returns true on success or false on error
  bool get_library_filename(char *dest);
};
static t_msgsupp_loader msgsupp_loader;

t_msgsupp_loader::t_msgsupp_loader() /*{{{*/
{
  hLibrary=NULL;
}; /*}}}*/
t_msgsupp_loader::~t_msgsupp_loader() /*{{{*/
{
  free_library();
} /*}}}*/
bool t_msgsupp_loader::get_library_filename(char *dest) /*{{{*/
{
  /* msgsupp.dll will be in PATH */
  strcpy(dest,"msgsupp.dll");
  return true;
#if 0
  bool result=false;
  HKEY hKey;
  if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Software602\\",0,KEY_READ_EX,&hKey)==ERROR_SUCCESS )
  {
    DWORD valuesize=MAX_PATH;
    if( RegQueryValueEx(hKey,"Path",NULL,NULL,(LPBYTE)dest,&valuesize)==ERROR_SUCCESS )
    {
      dest[valuesize]='\0';
      result=true;
    }
    RegCloseKey(hKey);
  }
  return result;
#endif
} /*}}}*/ 
bool t_msgsupp_loader::load_library() /*{{{*/
{
#ifdef SRVR
  ProtectedByCriticalSection guard(&cs_ftx_msgsupp_load);
#endif
  if( hLibrary!=NULL ) return true;
  char libraryfilename[MAX_PATH];
  if( !get_library_filename(libraryfilename) )
  {
    return false;
  }
  if( (hLibrary=LoadLibrary(libraryfilename))==NULL )
  {
    return false;
  }
  return true;
} /*}}}*/
void t_msgsupp_loader::free_library() /*{{{*/
{
  if( hLibrary!=NULL ) { FreeLibrary(hLibrary); hLibrary=NULL; }
} /*}}}*/

typedef
  __int64 __stdcall generic_read(void *ctx, void *buf, __int64 bytes);
typedef
  __int64 __stdcall generic_write(void *ctx, void *buf, __int64 bytes);
typedef
  void __stdcall generic_seek(void *ctx, __int64 pos);
typedef
  __int64 __stdcall generic_tell(void *ctx);

typedef UINT WINAPI t_M_CreateMessage();
typedef t_M_CreateMessage * p_M_CreateMessage;

CFNC UINT WINAPI link_M_CreateMessage()
{ p_M_CreateMessage p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_CreateMessage)GetProcAddress(msgsupp_loader.hLibrary, "M_CreateMessage");
  if (!p) return (UINT)0;
  return (*p)();
}

typedef UINT WINAPI t_M_DestroyMessage(UINT mhandle);
typedef t_M_DestroyMessage * p_M_DestroyMessage;

CFNC UINT WINAPI link_M_DestroyMessage(UINT mhandle)
{ p_M_DestroyMessage p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_DestroyMessage)GetProcAddress(msgsupp_loader.hLibrary, "M_DestroyMessage");
  if (!p) return (UINT)0;
  return (*p)(mhandle);
}

typedef UINT WINAPI t_M_LoadMessage(UINT mhandle,  UINT shandle);
typedef t_M_LoadMessage * p_M_LoadMessage;

CFNC UINT WINAPI link_M_LoadMessage(UINT mhandle,  UINT shandle)
{ p_M_LoadMessage p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_LoadMessage)GetProcAddress(msgsupp_loader.hLibrary, "M_LoadMessage");
  if (!p) return (UINT)0;
  return (*p)(mhandle, shandle);
}

typedef UINT WINAPI t_M_SaveDecodedBodypartHeader(UINT mhandle,  UINT shandle,  char *Charset);
typedef t_M_SaveDecodedBodypartHeader * p_M_SaveDecodedBodypartHeader;

CFNC UINT WINAPI link_M_SaveDecodedBodypartHeader(UINT mhandle,  UINT shandle,  char *Charset)
{ p_M_SaveDecodedBodypartHeader p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_SaveDecodedBodypartHeader)GetProcAddress(msgsupp_loader.hLibrary, "M_SaveDecodedBodypartHeader");
  if (!p) return (UINT)0;
  return (*p)(mhandle, shandle, Charset);
}

typedef UINT WINAPI t_M_FindTextLetterBodypart(UINT mhandle);
typedef t_M_FindTextLetterBodypart * p_M_FindTextLetterBodypart;

CFNC UINT WINAPI link_M_FindTextLetterBodypart(UINT mhandle)
{ p_M_FindTextLetterBodypart p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_FindTextLetterBodypart)GetProcAddress(msgsupp_loader.hLibrary, "M_FindTextLetterBodypart");
  if (!p) return (UINT)0;
  return (*p)(mhandle);
}

typedef UINT WINAPI t_M_FindHTMLLetterBodypart(UINT mhandle,  UINT *prelatedbphandle);
typedef t_M_FindHTMLLetterBodypart * p_M_FindHTMLLetterBodypart;

CFNC UINT WINAPI link_M_FindHTMLLetterBodypart(UINT mhandle,  UINT *prelatedbphandle)
{ p_M_FindHTMLLetterBodypart p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_FindHTMLLetterBodypart)GetProcAddress(msgsupp_loader.hLibrary, "M_FindHTMLLetterBodypart");
  if (!p) return (UINT)0;
  return (*p)(mhandle, prelatedbphandle);
}

typedef UINT WINAPI t_M_SaveBodypartBody(UINT bphandle,  char *ContentTransferEncoding,  char *Charset,  UINT in_shandle,  UINT out_shandle);
typedef t_M_SaveBodypartBody * p_M_SaveBodypartBody;

CFNC UINT WINAPI link_M_SaveBodypartBody(UINT bphandle,  char *ContentTransferEncoding,  char *Charset,  UINT in_shandle,  UINT out_shandle)
{ p_M_SaveBodypartBody p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_SaveBodypartBody)GetProcAddress(msgsupp_loader.hLibrary, "M_SaveBodypartBody");
  if (!p) return (UINT)0;
  return (*p)(bphandle, ContentTransferEncoding, Charset, in_shandle, out_shandle);
}

typedef void WINAPI t_M_GetMessageAttachmentsList(UINT mhandle,  __int64 *List,  int *Count,  int MaxCount);
typedef t_M_GetMessageAttachmentsList * p_M_GetMessageAttachmentsList;

CFNC void WINAPI link_M_GetMessageAttachmentsList(UINT mhandle,  __int64 *List,  int *Count,  int MaxCount)
{ p_M_GetMessageAttachmentsList p;
  if (!msgsupp_loader.load_library()) return;
  p = (p_M_GetMessageAttachmentsList)GetProcAddress(msgsupp_loader.hLibrary, "M_GetMessageAttachmentsList");
  if (!p) return;
  (*p)(mhandle, List, Count, MaxCount);
}

typedef UINT WINAPI t_M_SaveAttachment(__int64 ID,  UINT in_shandle,  UINT out_shandle);
typedef t_M_SaveAttachment * p_M_SaveAttachment;

CFNC UINT WINAPI link_M_SaveAttachment(__int64 ID,  UINT in_shandle,  UINT out_shandle)
{ p_M_SaveAttachment p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_SaveAttachment)GetProcAddress(msgsupp_loader.hLibrary, "M_SaveAttachment");
  if (!p) return (UINT)0;
  return (*p)(ID, in_shandle, out_shandle);
}

typedef UINT WINAPI t_M_GetAttachmentName(__int64 ID,  char *Name,  int NameSize,  char *Charset);
typedef t_M_GetAttachmentName * p_M_GetAttachmentName;

CFNC UINT WINAPI link_M_GetAttachmentName(__int64 ID,  char *Name,  int NameSize,  char *Charset)
{ p_M_GetAttachmentName p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_GetAttachmentName)GetProcAddress(msgsupp_loader.hLibrary, "M_GetAttachmentName");
  if (!p) return (UINT)0;
  return (*p)(ID, Name, NameSize, Charset);
}

typedef UINT WINAPI t_M_GetAttachmentMimeType(__int64 ID,  char *MimeType,  int MimeTypeSize);
typedef t_M_GetAttachmentMimeType * p_M_GetAttachmentMimeType;

CFNC UINT WINAPI link_M_GetAttachmentMimeType(__int64 ID,  char *MimeType,  int MimeTypeSize)
{ p_M_GetAttachmentMimeType p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_GetAttachmentMimeType)GetProcAddress(msgsupp_loader.hLibrary, "M_GetAttachmentMimeType");
  if (!p) return (UINT)0;
  return (*p)(ID, MimeType, MimeTypeSize);
}

typedef UINT WINAPI t_S_CreateInStream(char *data,  int datasize,  void *ctx,  generic_read *_g_read,  generic_seek *_g_seek,  generic_tell *_g_tell,  int abufsize);
typedef t_S_CreateInStream * p_S_CreateInStream;

CFNC UINT WINAPI link_S_CreateInStream(char *data,  int datasize,  void *ctx,  generic_read *_g_read,  generic_seek *_g_seek,  generic_tell *_g_tell,  int abufsize)
{ p_S_CreateInStream p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_S_CreateInStream)GetProcAddress(msgsupp_loader.hLibrary, "S_CreateInStream");
  if (!p) return (UINT)0;
  return (*p)(data, datasize, ctx, _g_read, _g_seek, _g_tell, abufsize);
}

typedef void WINAPI t_S_DestroyInStream(UINT shandle);
typedef t_S_DestroyInStream * p_S_DestroyInStream;

CFNC void WINAPI link_S_DestroyInStream(UINT shandle)
{ p_S_DestroyInStream p;
  if (!msgsupp_loader.load_library()) return;
  p = (p_S_DestroyInStream)GetProcAddress(msgsupp_loader.hLibrary, "S_DestroyInStream");
  if (!p) return;
  (*p)(shandle);
}

typedef UINT WINAPI t_S_CreateOutStream(char *data,  void *ctx,  generic_write *_g_write,  generic_seek *_g_seek,  generic_tell *_g_tell,  int abufsize);
typedef t_S_CreateOutStream * p_S_CreateOutStream;

CFNC UINT WINAPI link_S_CreateOutStream(char *data,  void *ctx,  generic_write *_g_write,  generic_seek *_g_seek,  generic_tell *_g_tell,  int abufsize)
{ p_S_CreateOutStream p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_S_CreateOutStream)GetProcAddress(msgsupp_loader.hLibrary, "S_CreateOutStream");
  if (!p) return (UINT)0;
  return (*p)(data, ctx, _g_write, _g_seek, _g_tell, abufsize);
}

typedef void WINAPI t_S_DestroyOutStream(UINT shandle);
typedef t_S_DestroyOutStream * p_S_DestroyOutStream;

CFNC void WINAPI link_S_DestroyOutStream(UINT shandle)
{ p_S_DestroyOutStream p;
  if (!msgsupp_loader.load_library()) return;
  p = (p_S_DestroyOutStream)GetProcAddress(msgsupp_loader.hLibrary, "S_DestroyOutStream");
  if (!p) return;
  (*p)(shandle);
}

typedef UINT WINAPI t_S_GetOutStreamBuffer(UINT shandle,  char **data,  int *datasize);
typedef t_S_GetOutStreamBuffer * p_S_GetOutStreamBuffer;

CFNC UINT WINAPI link_S_GetOutStreamBuffer(UINT shandle,  char **data,  int *datasize)
{ p_S_GetOutStreamBuffer p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_S_GetOutStreamBuffer)GetProcAddress(msgsupp_loader.hLibrary, "S_GetOutStreamBuffer");
  if (!p) return (UINT)0;
  return (*p)(shandle, data, datasize);
}

typedef UINT WINAPI t_S_GetOutStreamFileName(UINT shandle,  char *name,  int namesize);
typedef t_S_GetOutStreamFileName * p_S_GetOutStreamFileName;

CFNC UINT WINAPI link_S_GetOutStreamFileName(UINT shandle,  char *name,  int namesize)
{ p_S_GetOutStreamFileName p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_S_GetOutStreamFileName)GetProcAddress(msgsupp_loader.hLibrary, "S_GetOutStreamFileName");
  if (!p) return (UINT)0;
  return (*p)(shandle, name, namesize);
}

typedef UINT WINAPI t_S_GetIOError(UINT shandle);
typedef t_S_GetIOError * p_S_GetIOError;

CFNC UINT WINAPI link_S_GetIOError(UINT shandle)
{ p_S_GetIOError p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_S_GetIOError)GetProcAddress(msgsupp_loader.hLibrary, "S_GetIOError");
  if (!p) return (UINT)0;
  return (*p)(shandle);
}

typedef UINT WINAPI t_S_FlushBuffers(UINT shandle);
typedef t_S_FlushBuffers * p_S_FlushBuffers;

CFNC UINT WINAPI link_S_FlushBuffers(UINT shandle)
{ p_S_FlushBuffers p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_S_FlushBuffers)GetProcAddress(msgsupp_loader.hLibrary, "S_FlushBuffers");
  if (!p) return (UINT)0;
  return (*p)(shandle);
}

typedef UINT WINAPI t_M_GetMessageFlags(UINT mhandle,  unsigned int *Flags);
typedef t_M_GetMessageFlags * p_M_GetMessageFlags;

CFNC UINT WINAPI link_M_GetMessageFlags(UINT mhandle,  unsigned int *Flags)
{ p_M_GetMessageFlags p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_GetMessageFlags)GetProcAddress(msgsupp_loader.hLibrary, "M_GetMessageFlags");
  if (!p) return (UINT)0;
  return (*p)(mhandle, Flags);
}

typedef UINT WINAPI t_M_SaveDecodedMessage(UINT shandle,  UINT in_shandle,  UINT out_shandle,  DWORD *VerificationFlags,  unsigned char *SignCertifData,  int *SignCertifDataSize,  unsigned char *CryptCertifData,  int *CryptCertifDataSize);
typedef t_M_SaveDecodedMessage * p_M_SaveDecodedMessage;

CFNC UINT WINAPI link_M_SaveDecodedMessage(UINT shandle,  UINT in_shandle,  UINT out_shandle,  DWORD *VerificationFlags,  unsigned char *SignCertifData,  int *SignCertifDataSize,  unsigned char *CryptCertifData,  int *CryptCertifDataSize)
{ p_M_SaveDecodedMessage p;
  if (!msgsupp_loader.load_library()) return (UINT)0;
  p = (p_M_SaveDecodedMessage)GetProcAddress(msgsupp_loader.hLibrary, "M_SaveDecodedMessage");
  if (!p) return (UINT)0;
  return (*p)(shandle, in_shandle, out_shandle, VerificationFlags, SignCertifData, SignCertifDataSize, CryptCertifData, CryptCertifDataSize);
}

typedef __int64 WINAPI t_M_GetAttachmentSize(__int64 ID);
typedef t_M_GetAttachmentSize * p_M_GetAttachmentSize;

CFNC __int64 WINAPI link_M_GetAttachmentSize(__int64 ID)
{ p_M_GetAttachmentSize p;
  if (!msgsupp_loader.load_library()) return (__int64)0;
  p = (p_M_GetAttachmentSize)GetProcAddress(msgsupp_loader.hLibrary, "M_GetAttachmentSize");
  if (!p) return (__int64)0;
  return (*p)(ID);
}

/*}}}*/
/* class t_input_convertor_eml {{{*/
class t_input_convertor_eml:public t_input_convertor_ext_file_base
{
protected:
  void request_error_msgsupp(UINT errnum,const char *msgsupp_function,const char *situation=NULL);
  const char *get_msgsupp_errmsg(UINT errnum);
public:
  t_input_convertor_eml(cdp_t cdp_param);
  virtual ~t_input_convertor_eml();
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods)
   * warning: this class uses its own convert() method */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr) { return false; };
  virtual bool convert(void);
};
t_input_convertor_eml::t_input_convertor_eml(cdp_t cdp_param):t_input_convertor_ext_file_base(cdp_param) /*{{{*/
{
} /*}}}*/
t_input_convertor_eml::~t_input_convertor_eml() /*{{{*/
{
} /*}}}*/
const char *t_input_convertor_eml::get_msgsupp_errmsg(UINT errnum) /*{{{*/
{
  switch( errnum )
  {
    case MERR_OK: return "MERR_OK";
    case MERR_BASE: return "MERR_BASE";
    case MERR_READ: return "MERR_";
    case MERR_WRITE: return "MERR_WRITE";
    case MERR_OPEN: return "MERR_OPEN";
    case MERR_CREATE: return "MERR_CREATE";
    case MERR_MEMORY: return "MERR_MEMORY";
    case MERR_TMPREAD: return "MERR_TMPREAD";
    case MERR_TMPWRITE: return "MERR_TMPWRITE";
    case MERR_TMPCREATE: return "MERR_TMPCREATE";
    case MERR_TMPOPEN: return "MERR_TMPOPEN";
    case MERR_FILEMAPPING: return "MERR_FILEMAPPING";
    
    case MERR_INVALID_TNEF_SIGNATURE: return "MERR_INVALID_TNEF_SIGNATURE";
    
    case MERR_UNKNOWN_INCOMING_CHARSET: return "MERR_UNKNOWN_INCOMING_CHARSET";
    case MERR_UNKNOWN_OUTGOING_CHARSET: return "MERR_UNKNOWN_OUTGOING_CHARSET";
    case MERR_MULTIBYTE_TO_WIDECHAR: return "MERR_MULTIBYTE_TO_WIDECHAR";
    case MERR_WIDECHAR_TO_MULTIBYTE: return "MERR_WIDECHAR_TO_MULTIBYTE";
    
    case MERR_BODYPART_BODY_NOT_DEFINED: return "MERR_BODYPART_BODY_NOT_DEFINED";
    case MERR_BODYPART_BODY_FILENAME_EMPTY: return "MERR_BODYPART_BODY_FILENAME_EMPTY";
    case MERR_OPEN_BODYPART_BODY_FILE: return "MERR_OPEN_BODYPART_BODY_FILE";
    case MERR_BODYPART_BODY_DATA_NULL: return "MERR_BODYPART_BODY_DATA_NULL";
    case MERR_INVALID_UUPART_INDEX: return "MERR_INVALID_UUPART_INDEX";
    
    case MERR_UUSYNTAX: return "MERR_UUSYNTAX";
    case MERR_STREAM_NOT_OPENED: return "MERR_STREAM_NOT_OPENED";
    case MERR_NOT_ENOUGH_MEMORY: return "MERR_NOT_ENOUGH_MEMORY";
    case MERR_NO_BLANK_LINE_AT_END: return "MERR_NO_BLANK_LINE_AT_END";
    case MERR_MESSAGE_HEADER_SYNTAX_ERROR: return "MERR_MESSAGE_HEADER_SYNTAX_ERROR";
    case MERR_MULTIPART_BOUNDARY_NOT_FOUND: return "MERR_MULTIPART_BOUNDARY_NOT_FOUND";
    case MERR_INVALID_STRUCTURE_STREAM_DATA: return "MERR_INVALID_STRUCTURE_STREAM_DATA";

    case MERR_SMIMEInitError: return "MERR_SMIMEInitError";
    case MERR_SMIMECryptoDataError: return "MERR_SMIMECryptoDataError";
    case MERR_SMIMECertificateNotFound: return "MERR_SMIMECertificateNotFound";
    case MERR_SMIMEBadUserData: return "MERR_SMIMEBadUserData";
    case MERR_SMIMECertificateExpired: return "MERR_SMIMECertificateExpired";
    case MERR_SMIMECancel: return "MERR_SMIMECancel";
    case MERR_SMIMEBadSenderData: return "MERR_SMIMEBadSenderData";
    case MERR_SMIMEBadRecipientData: return "MERR_SMIMEBadRecipientData";
    case MERR_SMIMEIncompleteData: return "MERR_SMIMEIncompleteData";
    case MERR_SMIMESecurityWarning: return "MERR_SMIMESecurityWarning";
    case MERR_SMIMEUnableToDecodeData: return "MERR_SMIMEUnableToDecodeData";
    case MERR_SMIMEEmailDoNotMatch: return "MERR_SMIMEEmailDoNotMatch";
    case MERR_SMIMEMessageTampered: return "MERR_SMIMEMessageTampered";
    case MERR_SMIMESignerNotFound: return "MERR_SMIMESignerNotFound";
    case MERR_SMIMEInternalError: return "MERR_SMIMEInternalError";
    case MERR_SMIMEVerificationOk: return "MERR_SMIMEVerificationOk";
    case MERR_SMIMEError: return "MERR_SMIMEError";
    case MERR_SMIMEBPMissing: return "MERR_SMIMEBPMissing";
    case MERR_SMIMEBPParsingError: return "MERR_SMIMEBPParsingError";
    case MERR_SMIMENotAllPartsCouldBeDecrypted: return "MERR_SMIMENotAllPartsCouldBeDecrypted";
    case MERR_SMIMENoCertificateAvailable: return "MERR_SMIMENoCertificateAvailable";
    case MERR_SMIMENoSigningNotSuccessfull: return "MERR_SMIMENoSigningNotSuccessfull";
    case MERR_SMIMEMissingKeyForDecryption: return "MERR_SMIMEMissingKeyForDecryption";
    case MERR_SMIMENotSMIMEMessage: return "MERR_SMIMENotSMIMEMessage";

    case MERR_SMIMEUnknownError: return "MERR_SMIMEUnknownError";

    default: return "unknown error or invalid error number";
  }
} /*}}}*/
void t_input_convertor_eml::request_error_msgsupp(UINT errnum,const char *msgsupp_function,const char *situation) /*{{{*/
{
  if( situation==NULL )
    request_errorf("Fulltext: Error %s in msgsupp.dll while calling %s function.",get_msgsupp_errmsg(errnum),msgsupp_function);
  else
    request_errorf("Fulltext: Error %s in msgsupp.dll while calling %s function [%s].",get_msgsupp_errmsg(errnum),msgsupp_function,situation);
} /*}}}*/
bool t_input_convertor_eml::convert() /*{{{*/
{
  if( !msgsupp_loader.load_library() )
  {
    request_error("Fulltext: Can't load msgsupp.dll library.");
    return false;
  }
  char *eml_filename=NULL, // for EML file
       *tmp_filename=NULL, // for letter and attachments
       *decoded_tmp_filename=NULL, // for decoded message
       *decoded2_tmp_filename=NULL; // for two-times decoded message
  if( !create_tmp_filename(eml_filename,"eml") ) return false;
  if( !create_tmp_filename(tmp_filename) ) { corefree(eml_filename); return false; }
  bool result=false;
  if( input_stream2file(*is,eml_filename) )
  {
    UINT res,in_shandle;
    in_shandle=link_S_CreateInStream(eml_filename,-1,NULL,NULL,NULL,NULL,10240);
    if( (res=link_S_GetIOError(in_shandle))!=MERR_OK )
    {
      request_error_msgsupp(res,"S_CreateInStream");
    }
    else
    {
      UINT mhandle=link_M_CreateMessage();
      if( (res=link_M_LoadMessage(mhandle,in_shandle))!=MERR_OK )
      {
        request_error_msgsupp(res,"M_LoadMessage");
      }
      else
      {
        UINT Flags=0;
        // test if this message have to be decoded
        if( (res=link_M_GetMessageFlags(mhandle,&Flags))!=MERR_OK )
        {
          request_error_msgsupp(res,"M_GetMessageFlags");
        }
        else
        {
          bool do_index=false;
          if( Flags & (fzSigned|fzEncrypted) )
          { // decode message into temporary file
            if( create_tmp_filename(decoded_tmp_filename,"eml") && create_tmp_filename(decoded2_tmp_filename,"eml") )
            {
              UINT temp_out_shandle=link_S_CreateOutStream(decoded_tmp_filename,NULL,NULL,NULL,NULL,10240);
              if( (res=link_S_GetIOError(temp_out_shandle))!=MERR_OK )
              {
                request_error_msgsupp(res,"S_CreateOutStream");
                corefree(decoded_tmp_filename); decoded_tmp_filename=NULL; // and decoded_tmp_filename will not be unlink-ed at the end of this method
              }
              else
              {
                res=link_M_SaveDecodedMessage(mhandle,in_shandle,temp_out_shandle,NULL,NULL,NULL,NULL,NULL);
                link_S_DestroyOutStream(temp_out_shandle);
                if( res!=MERR_OK )
                {
                  request_error_msgsupp(res,"M_SaveDecodedMessage");
                }
                else
                { //pokud se dekodovani podarilo, tak uzavru puvodni zasilku a otevru dekodovanou
                  link_M_DestroyMessage(mhandle); // close message
                  link_S_DestroyInStream(in_shandle); // close instream
                  in_shandle=link_S_CreateInStream(decoded_tmp_filename,-1,NULL,NULL,NULL,NULL,10240);
                  if( (res=link_S_GetIOError(in_shandle))!=MERR_OK )
                  {
                    request_error_msgsupp(res,"S_CreateInStream");
                  }
                  else
                  {
                    mhandle=link_M_CreateMessage();
                    if( (res=link_M_LoadMessage(mhandle,in_shandle))!=MERR_OK )
                    {
                      request_error_msgsupp(res,"M_LoadMessage");
                    }
                    else
                    {
                      //bohuzel i vyextrahovana zasilka muze byt zase zakodovana - bezne je zasilka signovana + encryptovana
                      if( (res=link_M_GetMessageFlags(mhandle,&Flags))!=MERR_OK )
                      {
                        request_error_msgsupp(res,"M_GetMessageFlags");
                      }
                      else
                      {
                        if( Flags & (fzSigned|fzEncrypted) )
                        {
                          //zasilku je treba dekodovat	do temp. souboru
                          temp_out_shandle=link_S_CreateOutStream(decoded2_tmp_filename,NULL,NULL,NULL,NULL,10240);
                          if( (res=link_S_GetIOError(temp_out_shandle))!=MERR_OK )
                          {
                            request_error_msgsupp(res,"S_CreateOutStream");
                          }
                          else
                          {
                            res=link_M_SaveDecodedMessage(mhandle,in_shandle,temp_out_shandle,NULL,NULL,NULL,NULL,NULL);
                            link_S_DestroyOutStream(temp_out_shandle);
                            if( res!=MERR_OK )
                            {
                              request_error_msgsupp(res,"M_SaveDecodedMessage");
                            }
                            else
                            {//pokud se dekodovani podarilo, tak uzavru puvodni zasilku a otevru dekodovanou
                              link_M_DestroyMessage(mhandle); // close message
                              link_S_DestroyInStream(in_shandle); // close instream
                              in_shandle=link_S_CreateInStream(decoded2_tmp_filename,-1,NULL,NULL,NULL,NULL,10240);
                              if( (res=link_S_GetIOError(in_shandle))!=MERR_OK )
                              {
                                request_error_msgsupp(res,"S_CreateInStream");
                              } 
                              else
                              {
                                mhandle=link_M_CreateMessage();
                                if( (res=link_M_LoadMessage(mhandle,in_shandle))!=MERR_OK )
                                {
                                  request_error_msgsupp(res,"M_LoadMessage");
                                }
                                else
                                { // message is decoded now
                                  do_index=true;
                                }
                              }
                            }
                          }
                        }
                        else
                        { // extracted message isn't encoded
                          do_index=true;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
          else
          { // message isn't encoded
            do_index=true;
          }
          // index letter (plain on HTML)
          if( do_index )
          {
            UINT html_bphandle,htmlrelated_bphandle;
            // index letter
            if( (html_bphandle=link_M_FindHTMLLetterBodypart(mhandle,&htmlrelated_bphandle))==0 )
            { // HTML letter not found
              UINT text_bphandle;
              if( (text_bphandle=link_M_FindTextLetterBodypart(mhandle))!=0 )
              {
                UINT out_shandle=link_S_CreateOutStream(tmp_filename,NULL,NULL,NULL,NULL,10240);
                if( (res=link_S_GetIOError(out_shandle))!=MERR_OK )
                {
                  request_error_msgsupp(res,"S_CreateOutStream","opening text letter");
                }
                else
                {
                  if( (res=link_M_SaveBodypartBody(text_bphandle,"","utf-8",in_shandle,out_shandle))!=MERR_OK )
                  {
                    request_error_msgsupp(res,"M_SaveBodypartBody","saving text letter to temporary file");
                  }
                  else do_index=true;
                }
                link_S_DestroyOutStream(out_shandle);
                if( do_index )
                { // index text letter
                  t_flstr subdoc_relpath;
                  if( make_subdocument_relpath(":letter.plain",subdoc_relpath) )
                  { 
                    t_input_stream_file letter_is(cdp,tmp_filename,"letter.txt"/*LIMITS constraint will be tested with this filename*/,is->get_document_filename()/*filename of the file with EML document*/);
                    t_input_convertor_plain ic(cdp);
                    ic.set_stream(&letter_is);
                    ic.copy_parameters(this);
                    ic.set_specif(t_specif(0,CHARSET_NUM_UTF8,0,0));  // must be after copy_parameters(this);
                    ic.set_subdocument_prefix(subdoc_relpath.get());
                    agent_data->starting_document_part(subdoc_relpath.get()/*keep this valid until ic is destroyed*/);
                    do_index=ic.convert();
                  }
                  else do_index=false;
                }
                DeleteFile(tmp_filename);
              }
            }
            else
            { // HTML letter found
              UINT out_shandle=link_S_CreateOutStream(tmp_filename,NULL,NULL,NULL,NULL,10240);
              if( (res=link_S_GetIOError(out_shandle))!=MERR_OK )
              {
                request_error_msgsupp(res,"S_CreateOutStream","opening HTML letter");
              }
              else
              {
                if( (res=link_M_SaveBodypartBody(html_bphandle,"","",in_shandle,out_shandle))!=MERR_OK )
                {
                  request_error_msgsupp(res,"M_SaveBodypartBody","saving HTML letter to temporary file");
                }
                else do_index=true;
              }
              link_S_DestroyOutStream(out_shandle);
              if( do_index )
              { // index HTML letter
                t_flstr subdoc_relpath;
                if( make_subdocument_relpath(":letter.html",subdoc_relpath) )
                {
                  t_input_stream_file letter_is(cdp,tmp_filename,"letter.htm"/*LIMITS constraint will be tested with this filename*/,is->get_document_filename()/*filename of the file with EML document*/);
                  t_input_convertor_html ic(cdp);
                  ic.set_stream(&letter_is);
                  ic.copy_parameters(this);
                  ic.set_subdocument_prefix(subdoc_relpath.get());
                  agent_data->starting_document_part(subdoc_relpath.get()/*keep this valid until ic is destroyed*/);
                  do_index=ic.convert();
                }
                else do_index=false;
              }
              DeleteFile(tmp_filename);
            }
          }
          // index attachments
          if( do_index ) // do_index is false there when an error was encoutered earlier
          {
            int att_count=0;
            link_M_GetMessageAttachmentsList(mhandle,NULL,&att_count,0);
            if( att_count>0 )
            {
              if( is_depth_level_allowed() )
              {
                __int64 *List = (__int64 *)malloc(att_count*sizeof(__int64));
                if( List==NULL ) { request_error_out_of_memory(); do_index=false; }
                else
                {
                  link_M_GetMessageAttachmentsList(mhandle, List, &att_count, att_count);
                  // we will set do_index to false on any error
                  for (int i=0;do_index&&i<att_count;i++)
                  {
                    char AttName[1024];
                    if( (res=link_M_GetAttachmentName(List[i], AttName, sizeof(AttName),"utf-8"))!=MERR_OK )
                    {
                      request_error_msgsupp(res,"M_GetAttachmentName"); do_index=false;
                    }
                    else
                    {
                      __int64 attsize=link_M_GetAttachmentSize(List[i]);
                      if( attsize>INT_MAX ) attsize=INT_MAX;
                      bool index_it=false;
                      if( !test_limits_for_file(AttName,(int)attsize,&index_it) )
                      {
                        do_index=false; // FIXME: request error
                      }
                      else
                      {
                        if( index_it )
                        {
                          // copy attachment to temp.file
                          UINT out_shandle = link_S_CreateOutStream(tmp_filename, NULL, NULL, NULL, NULL, 10240);
                          if( (res=link_S_GetIOError(out_shandle))!=MERR_OK )
                          {
                            request_error_msgsupp(res,"S_CreateOutStream","opening attachment"); do_index=false;
                          }
                          else
                          {
                            if( (res=link_M_SaveAttachment(List[i], in_shandle, out_shandle))!=MERR_OK )
                            {
                              request_error_msgsupp(res,"M_SaveAttachment","saving attachment to temporary file"); do_index=false;
                            }
                          }
                          link_S_DestroyOutStream(out_shandle);
                          // index attachment unless an error was encoutered
                          if( do_index )
                          {
                            t_flstr subdoc_relpath;
                            if( make_subdocument_relpath(AttName,subdoc_relpath) )
                            {
                              t_input_stream_file att_is(cdp,tmp_filename,AttName/*LIMITS has to be tested with filename of attachment*/,is->get_document_filename()/*filename of the file with EML document*/);
                              t_input_convertor *att_ic=ic_from_text(cdp,&att_is); // select apropriate convertor
                              if( att_ic!=NULL )
                              {
                                att_ic->set_stream(&att_is);
                                att_ic->copy_parameters(this);
                                att_ic->set_subdocument_prefix(subdoc_relpath.get());
                                agent_data->starting_document_part(subdoc_relpath.get()/*keep this valid until ic is destroyed*/);
                                do_index=att_ic->convert();
                                delete att_ic;
                              }
                            }
                            else do_index=false;
                          }
                          // delete temp.file
                          DeleteFile(tmp_filename);
                        }
                      }
                    }
                  }
                  free(List);
                }
              }
              else
              { // depth_level is too high
                do_index=false;
              }
            }
            result=do_index; // set if we were successfull
          }
        }
      }
      link_M_DestroyMessage(mhandle);
    }
    link_S_DestroyInStream(in_shandle);
    DeleteFile(eml_filename);
  }
  corefree(eml_filename);
  corefree(tmp_filename);
  if( decoded_tmp_filename!=NULL ) { DeleteFile(decoded_tmp_filename); corefree(decoded_tmp_filename); }
  if( decoded2_tmp_filename!=NULL ) { DeleteFile(decoded2_tmp_filename); corefree(decoded2_tmp_filename); }
  return result;
} /*}}}*/
/*}}}*/
/*}}}*/
#endif
/* class t_input_convertor_catdoc_generic {{{*/
enum catdoc_program_type_t {
  CATDOC_CATDOC,
  CATDOC_XLS2CSV,
  CATDOC_CATPPT,
};
class t_input_convertor_catdoc_generic:public t_input_convertor_ext_file_to_stdout
{
protected:
  catdoc_program_type_t program_type;
  virtual bool compose_statement(t_flstr &command,const char *infile);
  virtual bool perform_action_on_output_file(input_convertor_actions_t action,void *dataptr);
  const char *get_program_binary_name() const;
public:
  t_input_convertor_catdoc_generic(cdp_t cdp_param,const catdoc_program_type_t program_type_param):t_input_convertor_ext_file_to_stdout(cdp_param) { program_type=program_type_param; };
  virtual ~t_input_convertor_catdoc_generic() {};
};
const char *t_input_convertor_catdoc_generic::get_program_binary_name() const /*{{{*/
{
  switch( program_type )
  {
    case CATDOC_CATDOC: return "catdoc";
    case CATDOC_XLS2CSV: return "xls2csv";
    case CATDOC_CATPPT: return "catppt";
    default: return "invalid_catdoc_program";
  }
} /*}}}*/
bool t_input_convertor_catdoc_generic::compose_statement(t_flstr &command,const char *infile) /*{{{*/
{
  if( !append_exe(command,"catdoc",get_program_binary_name(),"exe") ) return false;
  if( !command.put(" -d utf-8 ") ) goto catdoc_compose_statement_error;
  if( !command.putc('\"') ) goto catdoc_compose_statement_error;
  if( !command.put(infile) ) goto catdoc_compose_statement_error;
  if( !command.putc('\"') ) goto catdoc_compose_statement_error;
  return true;
catdoc_compose_statement_error:
  request_error_out_of_memory();
  return false;
} /*}}}*/
bool t_input_convertor_catdoc_generic::perform_action_on_output_file(input_convertor_actions_t action,void *dataptr) /*{{{*/
{
  t_input_convertor_plain ic(cdp);
  ic.set_agent(agent_data);
  ic.set_stream(is);
  ic.set_specif(t_specif(0,CHARSET_NUM_UTF8,0,0));
  ic.set_depth_level(depth_level);
  ic.set_limits_evaluator(limits_evaluator);
  ic.set_owning_ft(working_for);
  return ic.perform_action(action,dataptr);
} /*}}}*/
/*}}}*/
/* abstract class t_input_convertor_ext_file_to_dir {{{*/
class t_input_convertor_ext_file_to_dir:public t_input_convertor_ext_file_with_execute
{
protected:
  // fully qualified filename of input file and output directory
  // point to corealloc-ed buffers or are NULL
  // have to be corefree-ed in destructor
  char *infile,*outdir;

  /* composes cmd. line statement which has to be executed
   * command (output parameter) is command which will be executed, it's empty at start
   * argv (output parameter) are parameters which will be passed to command, it's empty at start
   *   caller have to set args to point to array of parameters finished by NULL array item
   * infile is fully qualified filename of input file which already exists
   * outdir is fully qualified dirname of output directory
   * dll_path (output parameter) is fully qualified directory name with dll/so libraries that command have to (or should) use
   *   on Windows: this directory will be made current directory and command will be run
   *   on Linux: this directory will be added to LD_LIBRARY_PATH environment variable to the start of its value
   *   it can be NULL - in that case nothing special is performed
   * returns true on success (and sets command and params too), false on error and logs error with request_error() */
  virtual bool compose_statement(t_flstr &command,char **(&argv),const char *infile,const char *outdir,char *&dll_path) = 0;
  /* The method is called when external convertor has already written output files into output directory
   * but before the conversion of each output file. The descendant class can check whether external convertor
   * succeeded etc. 
   * Return value: true if the operation should continue with the conversion of each output file
   *               false if the operation have to stop now (i.e. it should be cancelled) */
  virtual bool perform_action_before_conversion() { return true; }
  /* Makes decision whether given output file have to be indexed or not.
   * Returns: false if the file doesn't have to be indexed
   *          true if the file have to be indexed and sets subdocument_prefix parameter to the subdocument prefix of given file
   * filename is file name and extension of given output file
   * subdirname is subdirectory name where given output file is located (relative to output directory)
   * subdocument_prefix is output parameter, have to be set to subdocument prefix
   * convertor is output parameter, have to be set to NULL or to pointer to new-allocated instance of t_input_convertor descendant
   *   that have to be used for conversion of given file; when this parameter is NULL then the convertor will be selected by
   *   the file content */
  virtual bool decide_about_file_indexation(const char *filename,const char *subdirname,t_flstr &subdocument_prefix,t_input_convertor_with_perform_action *&convertor) = 0;
  virtual const char *get_infile_extension() { return NULL; }

  // helper methods:
  /* Tests whether given directory is empty. Returns true if it is empty, otherwise returns false. */
  bool is_dir_empty(const char *dirname);
  // saves input stream into file and writes its name into infile data member
  // returns true on success, false on error and logs error with request_error()
  bool is2infile();
  /* performs given action (indexation etc.) on the files in given directory
   * action is the fulltext action that have to be performed
   * dataptr is pointer that was passed to perform_action() method
   * basedirname is fullname of the base output directory
   * subdirname is relative name of the subdirectory in basedirname directory where the file is located
   * returns true on success or false on error( and logs error with request_error_XXX() methods) */
  bool perform_action_on_directory(input_convertor_actions_t action,void *dataptr,const char *basedirname,const char *subdirname);
public:
  t_input_convertor_ext_file_to_dir(cdp_t cdp_param):t_input_convertor_ext_file_with_execute(cdp_param) { infile=outdir=NULL; }
  virtual ~t_input_convertor_ext_file_to_dir();
  /* makes action on document
   * returns true on success or false on error (and logs error with request_error_XXX() methods) */
  virtual bool perform_action(input_convertor_actions_t action,void *dataptr);
};
t_input_convertor_ext_file_to_dir::~t_input_convertor_ext_file_to_dir() /*{{{*/
{
  if( infile!=NULL )
  {
    if( exists_file(infile) ) DeleteFile(infile);
    corefree(infile);
  }
  if( outdir!=NULL )
  {
    delete_directory(outdir);
    corefree(outdir);
  }
} /*}}}*/
bool t_input_convertor_ext_file_to_dir::perform_action(input_convertor_actions_t action,void *dataptr) /*{{{*/
{
  if( outdir==NULL || is_dir_empty(outdir) )
  {
    if( outdir!=NULL )
    {
      delete_directory(outdir);
      corefree(outdir);
      outdir=NULL;
    }
    // 1. save input stream into file and write its name into infile data member
    if( !is2infile() ) return false;
    // 2. create output filename and write it into outfile
    if( !create_tmp_dirname_and_mkdir(outdir) ) return false;
    // 3. compose cmd. line statement
    t_flstr command(200,100);
    char **argv,*dll_path;
    if( !compose_statement(command,argv,infile,outdir,dll_path) ) return false;
    // 4. execute statement
    command.putc('\0');
    if( !execute_statement(command,argv,dll_path) ) return false;
    // let the child test various conditions right after execution
    if( !perform_action_before_conversion() ) return false;
  }
  // 5. perform action on output directory
  return perform_action_on_directory(action,dataptr,outdir,"");

#if 0
  // 5. create t_input_stream instance for outfile
  {
    t_input_stream_file isf(cdp,outfile,is->get_filename_for_limits()/* LIMITS has to be tested with filename of original input stream */);
    t_input_stream *prev_is=is;
    is=&isf;    
  // 6. call convert_output_file()
    if( !perform_action_on_output_file(action,dataptr) )
    {
      is=prev_is;
      return false;
    }
    is=prev_is;
  // 7. destroy t_input_stream instance for outfile
    // in its destructor
  }
  return true;
#endif
} /*}}}*/
bool t_input_convertor_ext_file_to_dir::perform_action_on_directory(input_convertor_actions_t action,void *dataptr,const char *basedirname,const char *subdirname) /*{{{*/
{
  if( action==ICA_COUNTWORDS )
  {
    ICA_COUNTWORDS_initialize_counter(dataptr);
    ICA_COUNTWORDS_inc_counter(dataptr); // to 1
    return true;
  }
  else if( action==ICA_INDEXDOCUMENT )
  {
    bool result=false;
    char filespec[MAX_PATH];
    if( strlen(subdirname)==0 || strcmp(subdirname,".")==0 ) sprintf(filespec,"%s%c*",basedirname,PATH_SEPARATOR);
    else sprintf(filespec,"%s%c%s%c*",basedirname,PATH_SEPARATOR,subdirname,PATH_SEPARATOR);
    WIN32_FIND_DATA fd;
    // 1. cycle through all files and subdirectories in output directory
    HANDLE hFile=FindFirstFile(filespec,&fd);
    if( hFile!=INVALID_HANDLE_VALUE )
    {
      for( ;; )
      {
        if( strcmp(fd.cFileName,".")!=0 && strcmp(fd.cFileName,"..")!=0 )
        {
          char file[MAX_PATH];
          if( strlen(subdirname)==0 || strcmp(subdirname,".")==0 ) sprintf(file,"%s%c%s",basedirname,PATH_SEPARATOR,fd.cFileName);
          else sprintf(file,"%s%c%s%c%s",basedirname,PATH_SEPARATOR,subdirname,PATH_SEPARATOR,fd.cFileName);
#ifdef WINS
          if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
#else
          struct stat st;
          // get stat of file
          if( stat(file,&st)!=0 ) break; //error
          // and test if file is directory
          if( S_ISDIR(st.st_mode) )
#endif
          { // file is directory - perform action on it
            char newsubdir[MAX_PATH];
            if( strlen(subdirname)==0 || strcmp(subdirname,".")==0 ) strcpy(newsubdir,fd.cFileName);
            else sprintf(newsubdir,"%s%c%s",subdirname,PATH_SEPARATOR,fd.cFileName);
            if( !perform_action_on_directory(action,dataptr,basedirname,newsubdir) ) goto leave_perform_action_on_directory;
          }
          else
          { // file isn't directory
            t_input_convertor_with_perform_action *ic=NULL;
            t_flstr subdocprefix;
            // 2. test if current file has to be indexed
            if( decide_about_file_indexation(fd.cFileName,subdirname,subdocprefix,ic) )
            {
              // 3. do action on the file, i.e.:
              // 3.1 create t_input_stream instance for the file
              t_input_stream_file *tmpis=new t_input_stream_file(cdp,file);
              if( tmpis==NULL ) { result=false; goto leave_perform_action_on_directory; }
              // select convertor if it isn't already selected by decide_about_file_indexation()
              if( ic==NULL )
              {
                /* every t_input_convertor descendant that is used for document conversions is descendant of t_input_convertor_with_perform_action too */
                ic=(t_input_convertor_with_perform_action*)ic_from_text(cdp,tmpis);
                if( ic==NULL ) { delete tmpis; result=false; goto leave_perform_action_on_directory; } // TODO conversion not supported - hlasit tuto chybu?
              }
              /* we don't have to test if the user cancelled the request (see t_input_convertor::was_breaked())
               * because it's tested in ic->convert(), it returns false in this case */
              // set convertor data
              ic->set_stream(tmpis);
              ic->copy_parameters(this);
              ic->set_subdocument_prefix(subdocprefix.get()/*keep this pointer valid until ic is destroyed*/);
              agent_data->starting_document_part(subdocprefix.get());
              // 3.2 do action
              // build index
              if( !ic->perform_action(action,dataptr) ) { delete ic; delete tmpis; result=false; goto leave_perform_action_on_directory; } // TODO hlasit tuto chybu?
              // 3.3 destroy convertor and t_input_stream instance
              delete ic;
              delete tmpis;
            }
          }
        }
        if( !FindNextFile(hFile,&fd) ) break;
      }
      result=true;
    }
leave_perform_action_on_directory:
    if( hFile!=INVALID_HANDLE_VALUE ) FindClose(hFile);
    return result;
  }
  else return false; // invalid action
} /*}}}*/
bool t_input_convertor_ext_file_to_dir::is2infile() /*{{{*/
{
  assert(infile==NULL);
  assert(is!=NULL);
  if( !create_tmp_filename(infile,get_infile_extension()) ) return false;
  return input_stream2file(*is,infile);
} /*}}}*/
bool t_input_convertor_ext_file_to_dir::is_dir_empty(const char *dirname) /*{{{*/
{
  char filespec[MAX_PATH];
  sprintf(filespec,"%s%c*",dirname,PATH_SEPARATOR);
  WIN32_FIND_DATA fd;
  HANDLE hFile=FindFirstFile(filespec,&fd);
  if( hFile==INVALID_HANDLE_VALUE ) return true;
  for(;;)
  {
    if( strcmp(fd.cFileName,".")!=0 && strcmp(fd.cFileName,"..")!=0 )
    {
      FindClose(hFile);
      return false;
    }
    if( !FindNextFile(hFile,&fd) ) break;
  }
  FindClose(hFile);
  return true;
} /*}}}*/
/*}}}*/
#ifdef USE_XSLFO_CONVERTOR
/* class t_input_convertor_ext_xslfo {{{*/
class t_input_convertor_ext_xslfo:public t_input_convertor_ext_file_to_dir
{
protected:
  char *myargv[5],*infile_quoted,*outdir_quoted;
  /* composes cmd. line statement which has to be executed
   * command (output parameter) is command which will be executed, it's empty at start
   * argv (output parameter) are parameters which will be passed to command, it's empty at start
   *   caller have to set args to point to array of parameters finished by NULL array item
   * infile is fully qualified filename of input file which already exists
   * outdir is fully qualified dirname of output directory
   * dll_path (output parameter) is fully qualified directory name with dll/so libraries that command have to (or should) use
   *   on Windows: this directory will be made current directory and command will be run
   *   on Linux: this directory will be added to LD_LIBRARY_PATH environment variable to the start of its value
   *   it can be NULL - in that case nothing special is performed
   * returns true on success (and sets command and params too), false on error and logs error with request_error() */
  virtual bool compose_statement(t_flstr &command,char **(&argv),const char *infile,const char *outdir,char *&dll_path);
  /* Makes decision whether given output file have to be indexed or not.
   * Returns: false if the file doesn't have to be indexed
   *          true if the file have to be indexed and sets subdocument_prefix parameter to the subdocument prefix of given file
   * filename is file name and extension of given output file
   * subdirname is subdirectory name where given output file is located (relative to output directory)
   * subdocument_prefix is output parameter, have to be set to subdocument prefix
   * convertor is output parameter, have to be set to NULL or to pointer to new-allocated instance of t_input_convertor descendant
   *   that have to be used for conversion of given file; when this parameter is NULL then the convertor will be selected by
   *   the file content */
  virtual bool decide_about_file_indexation(const char *filename,const char *subdirname,t_flstr &subdocument_prefix,t_input_convertor_with_perform_action *&convertor);
  virtual const char *get_infile_extension();
  /* The method is called when external convertor has already written output files into output directory
   * but before the conversion of each output file. The descendant class can check whether external convertor
   * succeeded etc. 
   * Return value: true if the operation should continue with the conversion of each output file
   *               false if the operation have to stop now (i.e. it should be cancelled) */
  virtual bool perform_action_before_conversion();
public:
  t_input_convertor_ext_xslfo(cdp_t cdp_param):t_input_convertor_ext_file_to_dir(cdp_param) { infile_quoted=outdir_quoted=NULL; }
  virtual ~t_input_convertor_ext_xslfo();
};
t_input_convertor_ext_xslfo::~t_input_convertor_ext_xslfo() /*{{{*/
{
  if( infile_quoted!=NULL ) corefree(infile_quoted);
  if( outdir_quoted!=NULL ) corefree(outdir_quoted);
} /*}}}*/
bool t_input_convertor_ext_xslfo::compose_statement(t_flstr &command,char **(&argv),const char *infile,const char *outdir,char *&dll_path) /*{{{*/
{
  // get long path names for infile and outdir
  char infile_long[1024],outdir_long[1024];
  *infile_long=*outdir_long='\0';
  GetLongPathName(infile,infile_long,1024);
  GetLongPathName(outdir,outdir_long,1024);
  // quote names
  if( (infile_quoted=corestr_quote_filename(infile_long))==NULL ) return false;
  if( (outdir_quoted=corestr_quote_filename(outdir_long))==NULL ) return false;
  if( !append_exe(command,"xslfo","FtxForms","exe") ) return false;
  // fill myargv array
  myargv[0]=infile_quoted;
  myargv[1]=outdir_quoted;
  myargv[2]=NULL; // last item have to be NULL!
  // set argv param
  argv=myargv;
  dll_path=NULL;
  return true;
} /*}}}*/
bool t_input_convertor_ext_xslfo::decide_about_file_indexation(const char *filename,const char *subdirname,t_flstr &subdocument_prefix,t_input_convertor_with_perform_action *&convertor) /*{{{*/
{
  subdocument_prefix.reset();
  if( subdirname==NULL || strlen(subdirname)==0 )
  { // base directory
    if( stricmp(filename,"cleanform.txt")==0 )
    { // cleanform.txt
      subdocument_prefix.put("form\0");
      convertor=new t_input_convertor_plain(cdp);
      convertor->set_specif(t_specif(0,CHARSET_NUM_UTF8,0,0));
      return true;
    }
    if( stricmp(filename,"data.xml")==0 )
    {
      subdocument_prefix.put("data\0");
      convertor=new t_input_convertor_xml(cdp);
      return true;
    }
  }
  if( stricmp(subdirname,"BinData")==0 )
  { // XSL-FO attachments
    subdocument_prefix.put("att/\0");
    subdocument_prefix.put(filename);
    convertor=NULL; // convertor will be selected by document content
    return true;
  }
  // default is to skip the file
  return false;
} /*}}}*/
const char *t_input_convertor_ext_xslfo::get_infile_extension() /*{{{*/
{
  bool is_zfo=false;
  if( is!=NULL )
  {
    char buf[10]; *buf='\0';
    is->read(buf,10);
    is->reset();
    is_zfo=(memcmp(buf,"PK\003\004",4)==0);
  }
  return (is_zfo)?"zfo":"fo";
} /*}}}*/
bool t_input_convertor_ext_xslfo::perform_action_before_conversion() /*{{{*/
{
  char result_filename[MAX_PATH];
  sprintf(result_filename,"%s%cresult.txt",outdir,PATH_SEPARATOR);
  // result.txt have to exist
  if( !exists_file(result_filename) )
  {
    request_error_execution_failed("FtxForms.exe","XSL-FO document parsing failed. The file result.txt with the result code of the conversion doesn\'t exist in output directory.");
    return false;
  }
  // read result.txt contents
  FILE *f=fopen(result_filename,"rt");
  char contents[101];
  if( f==NULL || fgets(contents,100,f)==NULL )
  {
    if( f!=NULL ) fclose(f);
    request_error("XSL-FO document parsing failed. The file result.txt with the result code of the conversion can\'t be opened or is empty.");
    return false;
  }
  fclose(f);
  // get result code
  char *ptr=strchr(contents,'=');
  if( ptr==NULL )
  {
    request_error("XSL-FO document parsing failed. The file result.txt with the result code of the conversion has invalid format (= sign is missing).");
    return false;
  }
  int result_code=0;
  sscanf(ptr+1,"%x",&result_code);
  // report error if the result code isn't 0
  if( result_code!=0 )
  {
    char buffer[2049];
    *buffer='\0';
    if( FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,NULL,(DWORD)result_code,0,(LPTSTR)buffer,2048,NULL)==0 )
    {
      request_errorf("XSL-FO document parsing failed. The XSL-FO convertor returned error number %d.",result_code);
      return false;
    }
    request_errorf("XSL-FO document parsing failed. The XSL-FO convertor returned error number %d. %s",result_code,buffer);
    return false;
  }
  return true;
} /*}}}*/
/*}}}*/
#endif
#if 0
/* class t_input_convertor_test - this class is for testing {{{*/
class t_input_convertor_test:public t_input_convertor
{
public:
  t_input_convertor_test(cdp_t cdp):t_input_convertor(cdp) {};
  virtual ~t_input_convertor_test() {};
  virtual bool convert(void); 
};
static unsigned WINAPI ic_test_thread_proc(void *data)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  ZeroMemory( &pi, sizeof(pi) );
  if( !CreateProcess(NULL,
    "\"c:\\Program Files\\OpenOffice.org 1.9.109\\program\\soffice.exe\" -headless \"macro:///Tools.ftx602.ExportAsOOo(C:\\Documents and Settings\\Administrator.VPECUCH\\Local Settings\\Temp\\ftx)\"",
//    "\"c:\\Program Files\\OpenOffice.org 1.9.109\\program\\soffice.exe\" -headless \"macro:///Tools.ftx602.ExportAsOOo(f:\\ftx.doc)\"",
//    "\"c:\\Program Files\\OpenOffice.org1.1.3\\program\\soffice.exe\" -headless \"macro:///Tools.ftx602.ExportAsOOo(f:\\ftx.doc)\"",
    NULL,
    NULL,
    FALSE,
    CREATE_NO_WINDOW|CREATE_NEW_CONSOLE,
    NULL,
    NULL,
    &si,
    &pi) )
  {
    return 0;
  }
  WaitForSingleObject(pi.hProcess,INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return 1;
} 
bool t_input_convertor_test::convert()
{
/*
  // prepare structs
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;
  ZeroMemory( &pi, sizeof(pi) );
  // execute command
  if( !CreateProcess(NULL,"F:\\projekty\\_testy\\spawnl\\Debug\\spawnl.exe ",NULL,NULL,FALSE,CREATE_NO_WINDOW|CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi) )
  {
    return false;
  }
  // wait for the end
  WaitForSingleObject(pi.hProcess,INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
*/
  THREAD_HANDLE hThread;
  if( THREAD_START_JOINABLE(ic_test_thread_proc,10000,NULL,&hThread) )
  {
    THREAD_JOIN(hThread);
    t_input_stream_file zis(cdp,"f:\\ftx.doc.zip");
    bool ret=false;
    {
      t_input_convertor_zip ic(cdp);
      ic.set_agent(agent_data);
      ic.set_stream(&zis);
      ic.set_depth_level(depth_level);
      ret=ic.convert();
    }
    return ret;
  }
  else return false;
}
/*}}}*/
#endif

/* methods of the class t_format_guesser {{{*/
t_format_guesser::t_format_guesser(cdp_t cdp_param,t_input_stream *_is) /*{{{*/
{
  cdp=cdp_param;
  is=_is;
  format_type=FORMAT_TYPE_VOID;
} /*}}}*/
void t_format_guesser::initialize_buffer() /*{{{*/
{
  buffer_length=is->read(buffer,buffer_size);
  eof=(buffer_length==0);
  offset=0;
  current_index=0;
  buffer[buffer_size]='\0'; // we want to use strstr() and other functions which assume that buffer will be ended with '\0';
} /*}}}*/
bool t_format_guesser::populate_buffer_with_next_doc_part(int overlapped_length) /*{{{*/
{
  if( overlapped_length<0 || overlapped_length>=buffer_length ) overlapped_length=0;
  else
  {
    memmove(buffer,buffer+buffer_length-overlapped_length,overlapped_length);
  }  
  int readed=is->read(buffer+overlapped_length,buffer_size-overlapped_length);
  eof=(readed==0);
  offset=offset+buffer_length;
  buffer_length=readed+overlapped_length;
  return !eof;
} /*}}}*/
int t_format_guesser::fwd(int count) /*{{{*/
{
  if( eof ) return 0;
  if( current_index+count<buffer_length )
  {
    current_index+=count;
    return count;
  }
  int ret=buffer_length-current_index-1; // these chars are already in buffer
  offset+=buffer_length;
  buffer_length=is->read(buffer,buffer_size);
  if( buffer_length==0 )
  {
    eof=true;
    return ret;
  }
  current_index=count-ret-1;
  return count;
} /*}}}*/
const void *t_format_guesser::memmem(const void *s1,size_t s1len,const void *s2,size_t s2len) const /*{{{*/
{
  if( s1==NULL || s1len<=0 || s1len<s2len ) return NULL;
  if( s2==NULL || s2len<=0 ) return s1;
  for( const char *ptr=(const char*)s1;s1len>=s2len;ptr++,s1len-- )
  {
    if( *ptr==*(const char*)s2 )
    {
      if( memcmp(ptr,s2,s2len)==0 ) return ptr;
    }
  }
  return NULL;
} /*}}}*/
t_input_convertor *t_format_guesser::guess_format() /*{{{*/
{
  // initialize buffer
  initialize_buffer();
  if( eof )
  { // empty document is ASCII plain text document
    format_type=FORMAT_TYPE_TEXT_7BIT;
  }
  else
  {
    /* test some well-known binary format characteristics */
    {
      t_input_convertor *early=guess_format_early();
      if( early!=NULL ) return early;
    }
    // determine format type - i.e. whether document is text or binary
    if( !internal_determine_format_type() ) return NULL;
    // reset 'is' and initialize buffer
    is->reset();
    initialize_buffer();
  }
  // perform final tests
  if( format_type==FORMAT_TYPE_BINARY )
  { // binary
  }
  else
  { // text
    // when format type is UCS2 we have to test for HTML and XML documents
    // tests on documents in other encoding have been executed in guess_format_early()
    if( format_type==FORMAT_TYPE_TEXT_UCS2 )
    {
      { 
        char *ptr=buffer;
        // skip ws
        while( is_7bit_white_space(*ptr) || *ptr=='\0' ) ptr++;
        // test for XML and HTML prologue
        if( memcmp(ptr,"<\0?\0x\0m\0l\0",10)==0 )
        {
          t_input_convertor *ret=NULL;
#ifdef USE_XSLFO_CONVERTOR
          do
          { // find <fo:root> tag in document
            if( memmem(buffer,buffer_length,"<\0f\0o\0:\0r\0o\0o\0t\0",16)!=NULL )
            {
              ret=new_get_xslfo_convertor(cdp); // -> XSL-FO
              break;
            }
          } while( populate_buffer_with_next_doc_part(100) );
          if( ret==NULL )
          { // use XML convertor
#endif
          ret=new_get_xml_convertor(cdp); // -> XML
#ifdef USE_XSLFO_CONVERTOR
          }
#endif
          if( ret!=NULL )
          {
            ret->set_specif(t_specif(0,0,0,1));
          }
          return ret;
        }
        if( memcmp(ptr,"<\0h\0t\0m\0l\0>\0",12)==0
          || memcmp(ptr,"<\0H\0T\0M\0L\0>\0",12)==0 )
        {
          t_input_convertor *ret=new_get_html_convertor(cdp); // -> HTML
          if( ret!=NULL )
          {
            ret->set_specif(t_specif(0,0,0,1));
          }
          return ret;
        }
      }
      // test for <html> && ( <head> || <body )
      if(
          (memmem(buffer,buffer_length,"<\0h\0t\0m\0l\0>\0",12)!=NULL || memmem(buffer,buffer_length,"<\0H\0T\0M\0L\0>\0",12)!=NULL)
          &&( (memmem(buffer,buffer_length,"<\0h\0e\0a\0d\0>\0",12)!=NULL || memmem(buffer,buffer_length,"<\0H\0E\0A\0D\0>\0",12)!=NULL)
            ||(memmem(buffer,buffer_length,"<\0b\0o\0d\0y\0",10)!=NULL || memmem(buffer,buffer_length,"<\0B\0O\0D\0Y\0",10)!=NULL)
            ) 
        )
      {
        t_input_convertor *ret=new_get_html_convertor(cdp); // -> HTML
        if( ret!=NULL )
        {
          ret->set_specif(t_specif(0,0,0,1));
        }
        return ret;
      }
    }
    // use plain convertor
    {
      t_input_convertor *plain=new_get_text_convertor(cdp);
      if( plain!=NULL )
      {
        if( format_type==FORMAT_TYPE_TEXT_UCS2 ) plain->set_specif(t_specif(0,0,0,1));
        else if( format_type==FORMAT_TYPE_TEXT_UTF8 ) plain->set_specif(t_specif(0,CHARSET_NUM_UTF8,0,0));
      }
      return plain;
    }
  }
  return NULL;
} /*}}}*/
bool t_format_guesser::internal_determine_format_type() /*{{{*/
{
  // test if document begins with UTF-16 BOM
  const unsigned char utf16le_bom[]="\xFF\xFE",
    utf16be_bom[]="\xFE\xFF";
  if( memcmp(buffer,utf16le_bom,2)==0 || memcmp(buffer,utf16be_bom,2)==0 ) { format_type=FORMAT_TYPE_TEXT_UCS2; return true; }
  bool is_7bit,is_8bit_iso,is_8bit_win1250,is_8bit_win1252,is_utf8,
    found_ucs2_basic_latin,found_ucs2_latin1_supplement,found_ucs2_latin_extA,found_ucs2_latin_extB,found_ucs2_diacritical_marks,found_ucs2_other,
    even_number_of_bytes;
  {
    int utf8charlen,utf8charlen_expected;
    unsigned char ucs2char_lowbyte;
    bool this_is_even_char;
    int max_char_count_to_test=512*1024; /* test at most this number of chars
                                            we assume that start of document of this length is significant enough to determine type of file
                                            and we don't need to test rest of document */
    is_7bit=is_8bit_iso=is_8bit_win1250=is_8bit_win1252=is_utf8=true;
    found_ucs2_basic_latin=found_ucs2_latin1_supplement=found_ucs2_latin_extA=found_ucs2_latin_extB=found_ucs2_diacritical_marks=found_ucs2_other=false;
    utf8charlen=utf8charlen_expected=0;
    this_is_even_char=false;
    do
    {
      int c=getc();
      // 7-bit text consist of chars between 32 and 126 and \n \r \t \v
      if( is_7bit && !is_7bit_char(c) )
        is_7bit=false;
      // 8-bit text
      if( is_8bit_iso && !is_8bit_iso_char(c) )
        is_8bit_iso=false;
      if( is_8bit_win1250 && !is_8bit_win1250_char(c) )
        is_8bit_win1250=false;
      if( is_8bit_win1252 && !is_8bit_win1252_char(c) )
        is_8bit_win1252=false;
      /* UTF-8 char can start with:
       *   0xxxxxxx - one byte long char
       *   110xxxxx - two bytes long char
       *   1110xxxx - three bytes long char
       *   11110xxx - four bytes long char
       *   111110xx - five bytes long char
       *   1111110x - six bytes long char
       * multibyte UTF-8 chars have to continue with 10xxxxxx bytes
       * and second byte have to have at least one 'x' bit nonzero
       * see http://en.wikipedia.org/wiki/UTF-8 */
      if( is_utf8 )
      {
        if( utf8charlen==0 )
        {
          if( (c&0x80)==0 )
          { // one byte UTF-8 char
            // nothing to do - utf8charlen is already 0 and value of utf8charlen_expected is unimportant
            // however NULL char can't be part of UTF-8 text document
            if( c==0 ) is_utf8=false;
          }
          else if( (c&0xe0)==0xc0 )
          { // two bytes UTF-8 char
            utf8charlen=1;
            utf8charlen_expected=2;
          }
          else if( (c&0xf0)==0xe0 )
          { // three bytes UTF-8 char
            utf8charlen=1;
            utf8charlen_expected=3;
          }
          else if( (c&0xf8)==0xf0 )
          { // four bytes UTF-8 char
            utf8charlen=1;
            utf8charlen_expected=4;
          }
          else if( (c&0xfc)==0xf8 )
          {
            utf8charlen=1;
            utf8charlen_expected=5;
          }
          else if( (c&0xfe)==0xfc )
          {
            utf8charlen=1;
            utf8charlen_expected=6;
          }
          else
          {
            is_utf8=false; // invalid first byte of UTF-8 char
          }
        }
        else
        { // test if byte is 10xxxxxx and for second byte if at least one 'x' bit is nonzero
          if( (c&0xc0)==0x80 /*&& ( utf8charlen>1 /*|| (c&0x3f)!=0)*/ )
          { // this can be byte of UTF-8 char
            if( ++utf8charlen>utf8charlen_expected )
            {
              is_utf8=false;
            }
            else
            {
              if( utf8charlen==utf8charlen_expected )
              { // we have read whole UTF-8 char
                utf8charlen=utf8charlen_expected=0;
              }
            }
          }
          else
          {
            is_utf8=false;
          }
        }
      }
      /* UCS-2:
       * We will detect which Unicode char groups are present in document.
       * Char groups are:
       *   Basic Latin (0000-007F)
       *   Latin-1 Supplement (0080-00FF)
       *   Latin Extended-A (0100-017F)
       *   Latin Extended-B (0180-024F)
       *   Combining Diacritical Marks (0300-036F)
       *   all other
       * Each group has its flag - found_ucs2_XXX var.
       * We then decide if it would be UCS-2 document according to presence of these groups' chars. */
      if( this_is_even_char )
      { // low byte of UCS-2 char is in ucs2char_lowbyte, high byte is in c
        if( c==0 )
        {
          if( ucs2char_lowbyte&0x80 )
          { // Latin-1 Supplement
            found_ucs2_latin1_supplement=true;
          }
          else
          { // Basic Latin
            found_ucs2_basic_latin=true;
          }
        }
        else if( c==0x01 && !(ucs2char_lowbyte&0x80) )
        { // Latin Extended-A
          found_ucs2_latin_extA=true;
        }
        else if( (c==0x01 && ucs2char_lowbyte&0x80)||(c==0x02 && ucs2char_lowbyte<=0x4f) )
        { // Latin Extended-B
          found_ucs2_latin_extB=true;
        }
        else if( c==0x03 && ucs2char_lowbyte<=0x6f )
        { // Combining Diacritical Marks
          found_ucs2_diacritical_marks=true;
        }
        else
          found_ucs2_other=true;
        // finally drop this_is_even_char flag
        this_is_even_char=false;
      }
      else
      { // save low byte of UCS-2 char
        ucs2char_lowbyte=(unsigned char)c;
        // mark that next char will be even
        this_is_even_char=true;
      }
      // move to next char
      fwd(); --max_char_count_to_test;
    } while( !eof && max_char_count_to_test>0 );
    even_number_of_bytes=!this_is_even_char;
  }
  // make decision
  if( is_7bit ) format_type=FORMAT_TYPE_TEXT_7BIT;
  else if( is_utf8 ) format_type=FORMAT_TYPE_TEXT_UTF8;
  else if( even_number_of_bytes && !found_ucs2_other ) format_type=FORMAT_TYPE_TEXT_UCS2;
  else if( is_8bit_iso ) format_type=FORMAT_TYPE_TEXT_8BIT_ISO;
  else if( is_8bit_win1250 ) format_type=FORMAT_TYPE_TEXT_8BIT_WIN1250;
  else if( is_8bit_win1252 ) format_type=FORMAT_TYPE_TEXT_8BIT_WIN1252;
  else format_type=FORMAT_TYPE_BINARY;
  return true;
} /*}}}*/
bool t_format_guesser::is_OLE_storage_magic(const char *pBuf) const /*{{{*/
{
  unsigned char magic[8]={0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
  return memcmp(pBuf,magic,sizeof(magic))==0;
} /*}}}*/
t_input_convertor *t_format_guesser::guess_format_for_OLE_storage() /*{{{*/
{
  /* ve strukture mit seznam retezcu, ktere chci hledat
   * zjistit nejdelsi -> jeho delka se pouzije pri postupnem cteni streamu do bufferu
   * postupne cist stream
   * pomoci memcmp() hledat retezce
   * do struct poznamenat nalezeni daneho retezce (mozna i pozici?? asi ne, k nicemu by to nebylo)
   * po precteni celeho streamu projit zaznamy o nalezeni
   * podle toho rozhodnout o typu OLE streamu */
  struct t_search_strip
  {
    char *strip; // can contain \0 and other non-printable chars
    int size; // byte-size
    bool found;  // flag indicating whether this strip was found in stream
    unsigned long position; // where is first occurence of strip in stream
  };
  t_search_strip ss[] = {
    // 0 - Root Entry
    { "R\0o\0o\0t\0 \0E\0n\0t\0r\0y\0",20,false,0 },
    // 1 - WordDocument
    { "W\0o\0r\0d\0D\0o\0c\0u\0m\0e\0n\0t\0",24,false,0 },
    // 2 - Workbook
    { "W\0o\0r\0k\0b\0o\0o\0k\0",16,false,0 },
    // 3 - PowerPoint Document
    { "P\0o\0w\0e\0r\0P\0o\0i\0n\0t\0 \0D\0o\0c\0u\0m\0e\0n\0t\0",38,false,0 },
    // 4 - CONTENTS
    { "C\0O\0N\0T\0E\0N\0T\0S\0",16,false,0 },
    // 5 - MagicTab
    { "M\0a\0g\0i\0c\0T\0a\0b\0",16,false,0 },
    // 6 - Book - for Excel 5.0/95
    { "B\0o\0o\0k\0",8,false,0 },
    // NEW STRIP: data write here
    // end element of array
    { NULL,0,false,0 }
  };
  const int longest_strip_size=38; // PowerPoint Document
  int buflen=buffer_length,global_idx=0;
  for( ;; )
  {
    // scan buffer for occurence of strips
    int idx=0;
    bool strip_found=false; /* When any strip after Root Entry is found set this variable to true,
                               exit testing part of this method and construct apropriate t_input_convertor descendant. */
    for( char *ptr=buffer;idx<buflen;ptr++,idx++,global_idx++ )
    {
      switch( *ptr )
      {
        // NEW STRIP: test here
#define TEST_STRIP(strip_index) if( ss[0].found && !ss[strip_index].found && memcmp(ptr,ss[strip_index].strip,ss[strip_index].size)==0 ) { ss[strip_index].found=true; ss[strip_index].position=global_idx; strip_found=true; }
        case 'R':
          if( !ss[0].found && memcmp(ptr,ss[0].strip,ss[0].size)==0 ) { ss[0].found=true; ss[0].position=global_idx; }
          break;
        case 'W':
          TEST_STRIP(1);
          TEST_STRIP(2);
          break;
        case 'P':
          TEST_STRIP(3);
          break;
        case 'C':
          TEST_STRIP(4);
          break;
        case 'M':
          TEST_STRIP(5);
          break;
        case 'B':
          TEST_STRIP(6);
          break;
      }
      if( strip_found ) break;
    }
    if( strip_found ) break;
    // read next part of stream into buffer
    memcpy(buffer,buffer+buffer_size-longest_strip_size,longest_strip_size);
    if( (buflen=is->read(buffer+longest_strip_size,buffer_size-longest_strip_size))<=0 ) break; // end of stream
  }
  // determine type of OLE stream
  if( ss[0].found ) // Root Entry have to be found otherwise this isn't OLE stream
  {
    int found_strip_idx=-1;
    // find non-'Root Entry' strip which was first found after 'Root Entry'
    // set its index to found_strip_idx
    for( int i=1;ss[i].strip!=NULL;i++ )
    {
      if( ss[i].found )
      {
        if( found_strip_idx<0 ) found_strip_idx=i;
        else if( ss[i].position<ss[found_strip_idx].position ) found_strip_idx=i;
      }
    }
    switch( found_strip_idx )
    {
      // NEW STRIP: construct t_input_convertor descendant here
      case 1: return new_get_doc_convertor(cdp);
      case 2: // Excel 97/2000
      case 6: // Excel 5.0/95
        return new_get_xls_convertor(cdp);
      case 3: return new_get_ppt_convertor(cdp);
      case 4: return NULL; // TODO 602Text
      case 5: return NULL; // TODO 602Tab
    }
  }
  return NULL;
} /*}}}*/
const char *t_format_guesser::strtag(const char *str,const char *tag,const bool without_attributes) /*{{{*/
{
  size_t taglen=strlen(tag);
  for( ;; )
  { // find '<' char
    const char *less_than_char=strchr(str,'<');
    if( less_than_char==NULL ) return NULL;
    // skip whitespaces
    str=less_than_char;
    do { str++; } while( is_7bit_white_space(*str) );
    if( *str=='\0' ) return NULL;
    // test if this tag is 'tag'
    if( strnicmp(str,tag,taglen)==0 )
    {
      str+=taglen;
      // skip whitespaces
      while( is_7bit_white_space(*str) ) { str++; }
      if( *str=='>' || !without_attributes ) return less_than_char; // found
    }
    // skip to '>'
    while( *str!='>' && *str!='\0' ) { str++; }
    // skip '>'
    str++;
  }
} /*}}}*/
t_input_convertor *t_format_guesser::guess_format_early() /*{{{*/
{
  if( is_OLE_storage_magic(buffer) )
  { // formats based on OLE storage: DOC, WPD, XLS, PPT etc.
    return guess_format_for_OLE_storage();
  }
  else
  { // isn't OLE storage file
    if( strncmp(buffer,"%PDF-",5)==0 ) return new_get_pdf_convertor(cdp); // -> PDF
    if( strncmp(buffer,"{\\rtf",5)==0 ) return new_get_rtf_convertor(cdp); // -> RTF
    if( memcmp(buffer,"PK\003\004",4)==0 )
    {
#ifdef USE_XSLFO_CONVERTOR
      if( memcmp(buffer+30,"form.fo",7)==0 ) return new_get_xslfo_convertor(cdp); // -> zipped XSL-FO
#endif
      return new_get_zip_convertor(cdp); // -> ZIP, OOo formats
    }
#ifdef USE_EML_CONVERTOR
    /* Test for EML format have to be done BEFORE tests for HTML and XML formats.
     * E-mail body can contain HTML letter and in this case HTML format test could succeed. */
    if( is_eml_format() ) return new_get_eml_convertor(cdp); // -> EML
#endif
    // perform tests on first non-white space chars
    { 
      char *ptr=buffer;
      // skip ws
      while( is_7bit_white_space(*ptr) ) ptr++;
      // test for XML and HTML prologue
      if( strncmp(ptr,"<?xml",5)==0 )
      {
#ifdef USE_XSLFO_CONVERTOR
        if( strtag(ptr,"fo:root",false)!=NULL )
        {
          return new_get_xslfo_convertor(cdp); // -> XSL-FO
        }
        if( strtag(ptr,"dsig:Signature",false)!=NULL )
        { // it could be XSL-FO document, <fo:root> tag isn't in buffer
          // we will incrementally read document and search for <fo:root> tag
          while( populate_buffer_with_next_doc_part(100) )
          {
            if( strtag(buffer,"fo:root",false)!=NULL ) return new_get_xslfo_convertor(cdp); // -> XSL-FO
          }
          // otherwise it isn't XSL-FO
        }
#endif
        if( strtag(ptr+5,"html",false/*XHTML <html> tag have attributes*/)!=NULL ) return new_get_html_convertor(cdp); // -> XHTML
        else return new_get_xml_convertor(cdp); // -> XML
      }
      if( strtag(ptr,"html",false)!=NULL ) return new_get_html_convertor(cdp); // -> HTML
    }
  }
  // unknown format
  return NULL;
} /*}}}*/
#ifdef USE_EML_CONVERTOR
bool t_format_guesser::is_eml_format() /*{{{*/
  /* EML format description from znovotny@software602.cz
K rozpoznani hlavicky zasilky:

Spravny header by mel vypadat takto:

A) Kazda radka by mela zacinat bud 1) identifikatorem oddelenym od zbytku 
radky dvojteckou,
nebo 2) mezerou ci tabem - to znaci pokracovaci radku.

B) Identifikator smi obsahovat pouze znaky ze spodni poloviny ASCII (7bit), 
krome dvojtecky, mezer, tabu a kontrolnich znaku, coz jsou znaky 0 - 31
a 127.

C) Header je od zbytku zasilky oddelen prazdnou radkou

Pozor: Na prvni radku zasilky nelze tato pravidla aplikovat - zejmena 
unixove maily si tam s oblibou zapisuji nejake informace typu "From ...." 
ktere nespl�ji A) a B). Uz jsem videl i zasilku se dvema pomrvenymi radky, 
ale to uz je vyjimka.

Typicke identifikatory jsou "Date" a "Message-Id", oba povinne, coz ale 
neznamena, ze je ma kazda zasilka, dale "From", "Subject", "To", "Cc", 
"Content-Type", "Content-Transfer-Encoding", "Mime-Version" - ty zpravidla 
generuje klient - a "Received" a "Return-path" - ty jsou od SMTP serveru. 
Identifikatory nejsou case sensitive. Identifikatory zacinajici "X-" jsou 
user-defined.
   */
{
  enum status_t {
    ST_ERROR,
    ST_START_OF_LINE,
    ST_HEADER_ID,
    ST_COLON,
    ST_REST_OF_LINE,
    ST_END_OF_LINE,
  } status=ST_START_OF_LINE;
  int line_counter=0,idx=0;
  bool significant_MIME_header_found=false;
  int start_of_MIME_header_idx=-1;
  for( ;; )
  { // test if we are at the end of the buffer
    if( idx>=buffer_size ) return status!=ST_ERROR && significant_MIME_header_found;
    // 
    switch( status )
    {
      case ST_START_OF_LINE:
        if( buffer[idx]==' ' || buffer[idx]=='\t' ) status=ST_REST_OF_LINE; // line can start with space or tab
        else if( is_eml_header_id_char(buffer[idx]) ) { status=ST_HEADER_ID; start_of_MIME_header_idx=idx; } // or with valid header ID char
        else if( (buffer[idx]=='\r' || buffer[idx]=='\n') && line_counter>=4 ) return significant_MIME_header_found; // this is empty line which divides headers from message body
          /* however we assume that message header could contain at least 4 lines - short text documents will be rejected */
        else status=ST_ERROR; // otherwise it's invalid
        break;
      case ST_HEADER_ID:
        if( is_eml_header_id_char(buffer[idx]) ) idx++; // valid char
        else if( buffer[idx]==':' ) // have to continue with colon
        {
          status=ST_COLON;
          if( !significant_MIME_header_found && start_of_MIME_header_idx>=0 && (
               strnicmp(buffer+start_of_MIME_header_idx,"Date",4)==0
            || strnicmp(buffer+start_of_MIME_header_idx,"Message-Id",10)==0
            || strnicmp(buffer+start_of_MIME_header_idx,"From",4)==0
            || strnicmp(buffer+start_of_MIME_header_idx,"Subject",7)==0
            || strnicmp(buffer+start_of_MIME_header_idx,"Received",8)==0
            || strnicmp(buffer+start_of_MIME_header_idx,"Return-path",11)==0
                ) )
          {
            significant_MIME_header_found=true;
          }
          start_of_MIME_header_idx=-1;
        }
        else if( line_counter<2 ) status=ST_REST_OF_LINE; // first two lines could be without colon
        else status=ST_ERROR; // otherwise it's error
        break;
      case ST_COLON:
        if( buffer[idx]==':' ) { idx++; status=ST_REST_OF_LINE; }
        else if( buffer[idx]==' ' || buffer[idx]=='\t' ) idx++; // there can be spaces or tabs between header ID and colon
        else status=ST_ERROR;
        break;
      case ST_REST_OF_LINE:
        if( buffer[idx]=='\r' || buffer[idx]=='\n' ) status=ST_END_OF_LINE;
        else idx++; // valid chars are any 8-bit ASCII ones
        break;
      case ST_END_OF_LINE:
        if( buffer[idx]=='\n' ) { line_counter++; idx++; status=ST_START_OF_LINE; } // line can end with \n
        else if( buffer[idx]=='\r' && buffer[idx+1]=='\n' ) { line_counter++; idx+=2; status=ST_START_OF_LINE; } // or with \r\n
        else status=ST_ERROR; // all other is invalid
        break;
      case ST_ERROR:
      default:
        return false;
    }
  }
} /*}}}*/
#endif
/* t_input_convertor *t_format_guesser::new_get_XXX_convertor() methods {{{*/
t_input_convertor *t_format_guesser::new_get_convertor_using_ooo(cdp_t cdp_param,const char *infile_ext)
{
  switch( convertor_params.Get_ooo_usage_type() )
  {
    case OOO_STANDALONE:
      return new t_input_convertor_using_ooo(cdp_param,infile_ext);
    case OOO_SERVER:
      return new t_input_convertor_ooo_client(cdp_param,infile_ext);
    default:
      return NULL;
  }
}
t_input_convertor *t_format_guesser::new_get_text_convertor(cdp_t cdp_param)
{
  return new t_input_convertor_plain(cdp_param);
}
t_input_convertor *t_format_guesser::new_get_html_convertor(cdp_t cdp_param)
{
  return new t_input_convertor_html(cdp_param);
}
t_input_convertor *t_format_guesser::new_get_xml_convertor(cdp_t cdp_param)
{
  return new t_input_convertor_xml(cdp_param);
}
t_input_convertor *t_format_guesser::new_get_pdf_convertor(cdp_t cdp_param)
{
  return new t_input_convertor_xpdf(cdp_param);
}
t_input_convertor *t_format_guesser::new_get_zip_convertor(cdp_t cdp_param)
{
  return new t_input_convertor_zip(cdp_param);
}
t_input_convertor *t_format_guesser::new_get_ooo_convertor(cdp_t cdp_param)
{
  return new t_input_convertor_zip(cdp_param);
}
t_input_convertor *t_format_guesser::new_get_rtf_convertor(cdp_t cdp_param)
{
  if( convertor_params.Get_use_ooo_convertor() && convertor_params.Get_for_rtf()==OPENOFFICE_ORG ) return new_get_convertor_using_ooo(cdp_param,"rtf");
  else return new t_input_convertor_rtf(cdp_param);
}
t_input_convertor *t_format_guesser::new_get_doc_convertor(cdp_t cdp_param)
{
  if( convertor_params.Get_use_ooo_convertor() && convertor_params.Get_for_doc()==OPENOFFICE_ORG ) return new_get_convertor_using_ooo(cdp_param,"doc");
  else if( convertor_params.Get_for_doc()==ANTIWORD ) return new t_input_convertor_antiword(cdp_param);
  else return new t_input_convertor_catdoc_generic(cdp_param,CATDOC_CATDOC);
}
t_input_convertor *t_format_guesser::new_get_xls_convertor(cdp_t cdp_param)
{
  if( convertor_params.Get_use_ooo_convertor() && convertor_params.Get_for_xls()==OPENOFFICE_ORG ) return new_get_convertor_using_ooo(cdp_param,"xls");
  else return new t_input_convertor_catdoc_generic(cdp_param,CATDOC_XLS2CSV);
} 
t_input_convertor *t_format_guesser::new_get_ppt_convertor(cdp_t cdp_param)
{
  if( convertor_params.Get_use_ooo_convertor() && convertor_params.Get_for_ppt()==OPENOFFICE_ORG ) return new_get_convertor_using_ooo(cdp_param,"ppt");
  else return new t_input_convertor_catdoc_generic(cdp_param,CATDOC_CATPPT);
} 
#ifdef USE_EML_CONVERTOR
t_input_convertor *t_format_guesser::new_get_eml_convertor(cdp_t cdp_param)
{
  return new t_input_convertor_eml(cdp_param);
}
#endif
#ifdef USE_XSLFO_CONVERTOR
t_input_convertor *t_format_guesser::new_get_xslfo_convertor(cdp_t cdp_param)
{
  return new t_input_convertor_ext_xslfo(cdp_param);
}
#endif
/*}}}*/
format_type_t t_format_guesser::determine_format_type() /*{{{*/
{
  // initialize buffer
  initialize_buffer();
  if( eof )
  { // empty document is ASCII plain text document
    return FORMAT_TYPE_TEXT_7BIT;
  }
  else
  {
    // determine format type - i.e. whether document is text or binary
    if( !internal_determine_format_type() ) return FORMAT_TYPE_VOID;
    // reset 'is'
    is->reset();
    return format_type;
  }
}
/*}}}*/
/*}}}*/

t_input_convertor * ic_from_format(cdp_t cdp,const char * format)
{ if (!stricmp(format, "PLAIN") || !stricmp(format, "TEXT") || !stricmp(format, "TXT"))
    return t_format_guesser::new_get_text_convertor(cdp);
  if( stricmp(format,"HTM")==0 || stricmp(format,"HTML")==0 || stricmp(format,"XHTML")==0 )
    return t_format_guesser::new_get_html_convertor(cdp);
  if( stricmp(format,"PDF")==0 ) return t_format_guesser::new_get_pdf_convertor(cdp);
  if( stricmp(format,"XML")==0 ) return t_format_guesser::new_get_xml_convertor(cdp);
  if( stricmp(format,"ZIP")==0 ) return t_format_guesser::new_get_zip_convertor(cdp);
  if( stricmp(format,"OOO")==0 ) return t_format_guesser::new_get_ooo_convertor(cdp);
  if( stricmp(format,"RTF")==0 ) return t_format_guesser::new_get_rtf_convertor(cdp);
  if( stricmp(format,"DOC")==0 ) return t_format_guesser::new_get_doc_convertor(cdp);
  if( stricmp(format,"XLS")==0 ) return t_format_guesser::new_get_xls_convertor(cdp);
  if( stricmp(format,"PPT")==0 ) return t_format_guesser::new_get_ppt_convertor(cdp);
#ifdef USE_EML_CONVERTOR
  if( stricmp(format,"EML")==0 ) return t_format_guesser::new_get_eml_convertor(cdp);
#endif
#ifdef USE_XSLFO_CONVERTOR
  if( stricmp(format,"FO")==0 || stricmp(format,"ZFO")==0 ) return t_format_guesser::new_get_xslfo_convertor(cdp);
#endif
  return NULL;
}

t_input_convertor * ic_from_text(cdp_t cdp,t_input_stream *is) {
  t_format_guesser fg(cdp,is);
  t_input_convertor *ret=fg.guess_format();
  is->reset(); // status of 'is' is undeterminable after guess_format() call
  return ret;
}

