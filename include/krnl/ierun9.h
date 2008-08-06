// ierun9.h

#include "flstr.h"
/////////////////////////////////// t_dbf_io ////////////////////////////////

typedef struct
{ char name[11];
  char type;
  WORD res1;    // must be field offset on FoxPro
  WORD offset;  /* used for conversion only, reserved in Xbase */
  BYTE size;
  BYTE dec;
  WORD res3;
  BYTE res2[12];
} xadef;
typedef struct
{ BYTE filetype;
  char lastupdate[3];
  long recnum;
  uns16 hdrsize;
  uns16 recsize;
  uns16 res1;
  BYTE trans;
  BYTE crypt;
  BYTE res2[12];
  BYTE ismdx;
  BYTE res3[3];
} dbf_hdr;
typedef struct
{ long type;       /* reversed, 1 for text, 0 for picture */
  long num_chars;  /* reversed, text size, not incl. this header */
} fox_m_hdr;
typedef struct
{ short minus_one;  /* -1 */
  short start_pos;  /* 8 */
  long  num_chars;  /* text size + 8 */
} db_m_hdr;
typedef struct
{ long next_block;  /* reversed, next free block number */
  BYTE res[2];
  short block_size; /* reversed, 0x200 */
} fox_mm_hdr;
typedef struct
{ long next_block;  /* next free block number */
  long zero;
  char file_name[8];
  short zero2;
  short x102;
  short block_size; /* 0x200 */
  short zero3;
} db_mm_hdr;

class DllKernel t_dbf_io
{ BOOL     err;
  dbf_hdr  hdr;
  BOOL     dbase;
  FHANDLE   hnd, hnd2;
  char     rdbuf[512];
  uns32    next_free_block;
  char     basename[8+1];
  unsigned blocksize;
 public:
  tptr     recbuf;
  unsigned xattrs;
  xadef *  xdef;    // xdef & xattrs used only when reading a DBF file

  t_dbf_io(FHANDLE hndIn, BOOL dbaseIn)
  { hnd=hndIn;  hnd2=INVALID_FHANDLE_VALUE;  recbuf=NULL;  xdef=NULL;
    dbase=dbaseIn;  err=FALSE;
  }
  ~t_dbf_io(void)
  { corefree(recbuf);  corefree(xdef);
    if (hnd2!=INVALID_FHANDLE_VALUE) CloseHandle(hnd2);
  }
  bool  rdinit(cdp_t cdp, char * pathname, trecnum * precnum);
  BOOL  read(void);
  uns32 get_memo_size(tptr ptr);
  uns32 read_memo(tptr glob_atr_ptr, uns32 memo_size);
  BOOL  wrinit(BOOL is_memo, int attrcnt, int recsize, char * fname);
  BOOL  add_field(tptr name, char type, int size, int dec, int field_offset);
  BOOL  write_memo(tptr satrval, unsigned satrvallen, tptr DBF_ptr, int is_text);
  BOOL  wrclose(trecnum recnum);
  BOOL  write(void);
  BOOL  deleted(void)   { return *recbuf=='*'; }
};

//////////////////////////////// line input ///////////////////////////////
class t_lineinp
{ FHANDLE hnd;
  char * buf;
  uns16 * buf2;  uns32 * buf4;
  unsigned bufsize, linestart, nextline, bufcont;
  bool err, lfonly;
  int charsize;
 public:
  t_lineinp(FHANDLE hndIn, bool lfonlyIn, int charsizeIn)
  { hnd=hndIn;  buf=NULL;  err=FALSE;  lfonly=lfonlyIn;  charsize=charsizeIn;
    bufsize=linestart=nextline=bufcont=0;  // all in characters, not bytes
    buf=(tptr)corealloc(charsize*3000, 81);
    buf2=(uns16*)buf;  buf4=(uns32*)buf;
    if (buf==NULL) err=true;  else bufsize=3000;
  }
  ~t_lineinp(void)
    { corefree(buf); }
  tptr get_line(unsigned * plinelen);
  bool was_error(void) { return err; }
};
////////////////////////////////// buffered file output ///////////////////////////
class t_file_bufwrite
{ FHANDLE hnd;
  char buf[1024];
  int bufcont;
  bool error;
 public:
  t_file_bufwrite(void)
    { hnd=INVALID_FHANDLE_VALUE; }
  void start(FHANDLE hndIn)
    { hnd=hndIn;  bufcont=0;  error=false; } 
  bool is_error(void) const 
    { return error; }
  void flush(void)
  { DWORD wr;
    if (!WriteFile(hnd, buf, bufcont, &wr, NULL) || wr!=bufcont)
      error=true;
    bufcont=0;
  };
  void write(void * data, int size)
  { if (size<sizeof(buf)-bufcont)
    { memcpy(buf+bufcont, data, size);
      bufcont+=size;
    }
    else
    { flush();
      if (size<sizeof(buf))
      { memcpy(buf, data, size);
        bufcont=size;
      }
      else
      { DWORD wr;
        if (!WriteFile(hnd, data, size, &wr, NULL) || wr!=size)
          error=true;
      }
    }
  }
};

//////////////////////////////// t_ie_dsgn /////////////////////////////////
#define IETYPE_WB     0  // includes open cursor
#define IETYPE_TDT    1
#define IETYPE_COLS   2
#define IETYPE_CSV    3
#define IETYPE_DBASE  4
#define IETYPE_FOX    5
#define IETYPE_ODBC   6
#define IETYPE_CURSOR 7   // input only
#define IETYPE_ODBCVIEW 8 // input only
#define IETYPE_LAST   IETYPE_ODBCVIEW

#define DBFTYPE_CHAR    1
#define DBFTYPE_NUMERIC 2
#define DBFTYPE_FLOAT   3
#define DBFTYPE_LOGICAL 4
#define DBFTYPE_DATE    5
#define DBFTYPE_MEMO    6  // "desttype" value for MEMO

#define CHARSET_CP1250    0  // original Move_data code
#define CHARSET_CP1252    1
#define CHARSET_ISO646    2
#define CHARSET_ISO88592  3  // original Move_data code
#define CHARSET_IBM852    4  // original Move_data code
#define CHARSET_ISO88591  5
#define CHARSET_UTF8      6
#define CHARSET_UCS2      7
#define CHARSET_UCS4      8
#define CHARSET_CNT      9

#define MAX_SOURCE 79
typedef struct
{ char    source[MAX_SOURCE+1];    // is always in the system encoding!
  tattrib sattr;    // the source column number, not stored any more, used only in run-time
  uns16   srcoff;   // for COLS, DBF (in chars)
  uns16   srcsize;  // for COLS (defined by user), CSV (searched for each line) - in chars
  tattrib dattr;    // used during transport only, destination column number in database, text, DBF
  tobjname destin;  // is always in the system encoding!
  uns16   desttype; // user defined type for created destination
  uns16   convtype; // data are converted to this type
  uns16   destlen1, destlen2;
  uns16   destoff;  // for COLS, DBF
} tdetail;

SPECIF_DYNAR(detail_dynar,tdetail,4);

#define MAX_COND_SIZE  100
#define FORMAT_STRING_SIZE 25 // reasonable format size for the timestamp is 19

class DllKernel t_ie_run
{public: 
  cdp_t    cdp;
  bool     silent;
  tattrib  * colmap;       // ordering of destination columns (NOATTRIB if column not written)
  unsigned colmapstop;     // index after the last valid item in the colmap
  tattrib  * src_colmap;   // ordering of source columns in text sources, NOATTRIB if column not used. Indexed by 0-based column number, gives dd index
  unsigned max_src_col;    // max number of column in the design (0-based here)

 // run data, deallocated by run_clear:
  FHANDLE outhnd, inhnd;
  t_lineinp * linp;
  t_dbf_io * dbf_inp, * dbf_out;
  tcursnum incurs;
  ltable * source_dt, * dest_dt;
  BOOL index_disabled;  
  xcdp_t sxcdp, txcdp;

  void destname2num(tobjnum objnum);
  BOOL wb2wb_trasport(void);

 public:
 // data from the definition:
  int     source_type, dest_type;  // types of data source and data target
  char    inserver[MAX_ODBC_NAME_LEN+1], outserver[MAX_ODBC_NAME_LEN+1];
  char    inschema[MAX_ODBC_NAME_LEN+1], outschema[MAX_ODBC_NAME_LEN+1];  // only for 602SQL schemata, ODBC schema is stored in inpath/outpath  
  char    inpath[MAX_PATH], outpath[MAX_PATH];  // source and target object/file names
  char    cond[MAX_COND_SIZE+1];   // source record condition
  tobjnum in_obj, out_obj;         // source and target object numbers
  int     src_recode, dest_recode;
 // special parameters:
  BOOL    index_past;
  char    csv_separ, csv_quote, decim_separ;
  BOOL    add_header;
  unsigned skip_lines;
  char    date_form[FORMAT_STRING_SIZE+1], time_form[FORMAT_STRING_SIZE+1], 
          timestamp_form[FORMAT_STRING_SIZE+1], boolean_form[FORMAT_STRING_SIZE+1];
  BOOL    semilog, lfonly;

  BOOL    dest_exists;
  bool    creating_target;
  unsigned destrecsize;  // used by destination COLS, DBF
  detail_dynar dd;
  bool    overwrite_existing_target_file;
  int     mapped_input_columns;
  int     ignored_input_columns;

  t_ie_run(cdp_t cdpIn, BOOL silentIn);
  BOOL do_transport(void);
  bool analyse_dsgn(void);
  void analyse_cols_in_line(const char * srcline, int slinelen, int recode);
  void OnCreImpl(void);
  void run_clear(void);
  int  convert_column_names_to_numbers(cdp_t cdp, t_dbf_io * dbf_inp, t_specif s_specif);

  tptr conv_ie_to_source(void);     // creates transport description
  BOOL load_design(tobjnum objnum); // loads & compiles transport description
  int  compile_design(char * src);
  void analyse_trans(CIp_t CI);     // compiler proc
  bool creates_target_file(void) const;
  bool creates_target_table(void) const;
  bool allocate_colmaps(void);
  void release_colmaps(void);

  inline bool text_source(void) const
    { return source_type==IETYPE_CSV || source_type==IETYPE_COLS; }
  inline bool text_target(void) const
    { return dest_type  ==IETYPE_CSV || dest_type  ==IETYPE_COLS; }
  inline bool dbf_source(void) const
    { return source_type==IETYPE_DBASE || source_type==IETYPE_FOX; }
  inline bool dbf_target(void) const
    { return dest_type  ==IETYPE_DBASE || dest_type  ==IETYPE_FOX; }
  inline bool db_source(void) const
    { return source_type==IETYPE_WB || source_type==IETYPE_CURSOR; }
  inline bool db_target(void) const
    { return dest_type  ==IETYPE_WB; }
  inline bool odbc_source(void) const
    { return source_type==IETYPE_ODBC; }
  inline bool odbc_target(void) const
    { return dest_type  ==IETYPE_ODBC; }
  void copy_from(const t_ie_run & patt);
};

DllExport t_specif WINAPI code2specif(int src_recode);

struct t_xsd_gen_params
{
  tobjname tabname_prefix;
  int id_type;
  int string_length;
  bool unicode_strings;
  bool special_top_seq;    // use a separate sequence for the ID column in the top-level table
  int refint;
  tobjname dad_name;
  t_xsd_gen_params(void)
  { strcpy(tabname_prefix, "xml_");
    id_type=ATT_INT32;
    string_length=255;
    unicode_strings=true;
    special_top_seq=false;
    refint=1;
    *dad_name=0;
  }
};




