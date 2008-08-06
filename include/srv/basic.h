/****************************************************************************/
/* Basic.h - Header and reading/writing blocks                              */
/****************************************************************************/
/* functions return TRUE iff failed! */
void write_gcr(cdp_t cdp, tptr dest);
void update_gcr(cdp_t cdp, const char * zcr_val);
SPECIF_DYNAR(t_blocknum_dynar, tblocknum, 50);

#define XOR_SIZE 32
#define DES_SIZE 128    // >= XOR_SIZE

typedef class t_crypt_1
{ unsigned crypt_step;
  uns8 crypt_xor[DES_SIZE];
  t_fil_crypt crypt_type;

 public:
  BOOL crypting(void) { return crypt_type!=fil_crypt_no; }
  void set_crypt(t_fil_crypt acrypt, const char * password);
  void i_fil_crypt(uns8 * block);
  void o_fil_crypt(uns8 * block);
  t_fil_crypt current_crypt_type(void) { return crypt_type; }

  t_crypt_1(void) { crypt_type=fil_crypt_no; }
} t_crypt_1;

typedef struct t_crypt_2
{ t_crypt_1 i, o;
  uns8 cbuf[0x4000];  //  protected by fil_sem
} t_crypt_2;

extern t_crypt_2 acrypt;
void crypt_changed(cdp_t cdp);
extern char def_password[];

 /* level 2 */
extern uns8 * corepool;  /* used in catch_free & WBRest_ services too */

class fil_backup
{ enum { READ_PARAM_INTERVAL=300, BACKUP_TIME_COUNT=4 };
  unsigned backup_interval;       // in seconds
  unsigned backup_time[BACKUP_TIME_COUNT]; // seconds since midnight
  unsigned backup_day [BACKUP_TIME_COUNT]; // 0-no backup, 1-daily, 2-Monday etc.
  BOOL     backup_done[BACKUP_TIME_COUNT];
  timestamp last_backup;          // last periodical backup or server startup timestamp
  void backup_now(cdp_t cdp);
 public:
  void read_backup_params(void);
  fil_backup(void);
  void check_for_fil_backup(cdp_t cdp);
};

void backup_count_limit(const char * direc, const char * name_suffix);
class t_backup_support_fil;

#define BLOCK_BUFFER_SIZE 200 /* number of allocated blocks stored in memory */
class t_ffa
{ friend class ProtectedByCriticalSection;
  friend class t_disk_space_management;
  friend int _sqp_backup(cdp_t cdp, const char * pathname_pattern, bool nonblocking, int extent);
  friend int _sqp_backup_zip(const char * pathname_pattern, const char * dest_dir, const char * move_dir);

 // FIL location: 
  char  fil_path[MAX_FIL_PARTS][MAX_PATH];  // paths to database file parts, '\' on the end
  uns32 fil_offset[MAX_FIL_PARTS];          // blocknum offset of the fil part 
  int   fil_parts;                          // number of file parts
  BOOL  GetServerPaths(const char * server_name);
 // handles & size:
  FHANDLE handle[MAX_FIL_PARTS];   /* application handles to database file parts */
  uns32  db_file_size;            // length of FIL in blocks
  tblocknum part_blocks[MAX_FIL_PARTS]; // sizes of FIL parts, in blocks
  int fil_open_counter;           // number of threads accessing the files, protected by fil_sem
  tblocknum header_blocknum;      // 0 unless header displaced by CD ROM 
  BOOL first_part_read_only;      // cannot write the first part, will not backup it
  t_backup_support_fil * backup_support;  // not NULL only during the hot backup
 // remapped access:
 public:
  CRITICAL_SECTION fil_sem;
  CRITICAL_SECTION remap_sem;
 private:
  unsigned  remapspace;       /* number of cells in core remap arrays */
  tblocknum remap_noinit;
  tblocknum * remap_from;    /* core remap arrays */
  tblocknum * remap_to;
  int  add_to_remap(cdp_t cdp, tblocknum from_place, tblocknum to_place);
  tblocknum translate_blocknum(tblocknum logblocknum);
#ifndef FLUSH_OPTIZATION        
 // block allocation:
  tblocknum blockbuf[BLOCK_BUFFER_SIZE]; /* accesible for maintan */
  unsigned  blocks_in_buf;  /* accesible for maintan */
  unsigned  next_pool; /* the pool I'll allocate from */
  void insert_block(cdp_t cdp, tblocknum dadr);
#endif
 // communication log: 
  FHANDLE c_handle;                        // handle to client-server communication log
  CRITICAL_SECTION c_log_cs;              // critical section protecting writing to the communications log
  int do_backup_inner(cdp_t cdp, const char * fname);
  bool protocol2;
 public:
  FHANDLE j_handle;                /* application handle to JOURNAL.FIL */
  FHANDLE t_handle;                /* application handle to TRANSACT.FIL */
  FHANDLE hProtocol;

  void preinit(const char * name_or_path, char * server_name);
  void init  (void);
  void deinit(void);
  int  check_fil(void);
  int  restore(void);
  BOOL open_fil_parts(void);
  void close_fil_parts(void);
  int  open_fil_access(cdp_t cdp, char * server_name);
  void close_fil_access(cdp_t cdp);
  const char * first_fil_path(void) const // used when looking for other files
    { return fil_path[0]; }
  const char *  last_fil_path(void) const // used when creating queues & determining free space
    { return fil_path[fil_parts-1]; }
  int  do_backup(cdp_t cdp, const char * fname);
  int  do_backup2(cdp_t cdp, const char * fname);
  void complete_broken_backup(void);
  void fil_consistency(void);
  void get_database_file_size(void);

  BOOL WINAPI w_raw_read_block (uns32 blocknum, void * kam);
  BOOL WINAPI w_raw_write_block(uns32 blocknum, const void * adr);
  BOOL WINAPI w_enc_write_block(uns32 blocknum, const void * adr);
  void flush(void);
  int  read_header (void);
  void write_header(void);
  
  tblocknum get_fil_size(void) const   // returns total FIL size in blocks!
    { return db_file_size; }
  bool set_fil_size(cdp_t cdp, tblocknum newblocks);   // LCK2
  void truncate_database_file(tblocknum dadr_count);
  void cdrom(cdp_t cdp);
  tblocknum first_allocable_block(void)
    { return header.cd_rom_offset*8*BLOCKSIZE; }
  unsigned backupable_fil_parts(void)
    { return first_part_read_only ? fil_parts-1 : fil_parts; }

 // remapped access:
  int remap_init(void);
  void get_remap_to(tblocknum ** to_remap,  unsigned * remap_count);
  BOOL resave_remap(void);
  inline BOOL sys_read_block(tblocknum from_where, void * where_to)
    { return w_raw_read_block(translate_blocknum(from_where), where_to); }
  BOOL sys_write_block_noredir(const void * from_where, tblocknum where_to);
  BOOL sys_write_block(cdp_t cdp, const void * from_where, tblocknum where_to);
 // displaced FIL access //////////////////////////////////
  inline bool is_valid_block(tblocknum block)
    { return block + header.bpool_first < db_file_size; }
  BOOL read_block(cdp_t cdp, tblocknum from_where, tptr where_to);
  BOOL write_block(cdp_t cdp, tptr from_where, tblocknum where_to);
#ifndef FLUSH_OPTIZATION        
 // allocating blocks:
  BOOL write_bpool(uns8 * bpool, int poolnum);
  BOOL read_bpool(int poolnum, uns8 * bpool);
  void save_blocks(cdp_t cdp, unsigned pool); // almost private
  tblocknum get_block(cdp_t cdp);  /* returns a new block, 0 if cannot */       // LCK2
  void release_block(cdp_t cdp, tblocknum dadr);   /* frees a disc block */     // LCK2
  void save_working_blocks(cdp_t cdp);
  tblocknum free_blocks(cdp_t cdp);                                             // LCK2
  void get_block_buf(tblocknum ** buf,  unsigned * count);
  tblocknum get_big_block(cdp_t cdp);/* returns a new big block, 0 if cannot */ // LCK2
#endif
  int  disc_open (cdp_t cdp, BOOL & fil_closed_properly);
  void disc_close(cdp_t cdp);
 // communication log: 
  inline BOOL logging_communication(void)
    { return FILE_HANDLE_VALID(c_handle); }
  void log_client_restart(void);          // log the server start, initialize the log if empty
  void log_client_request(cdp_t cdp);     // log the request
  void log_client_termination(cdp_t cdp); // log client termination
  BOOL c_log_read(void * buf, DWORD req_length); // reads from the log when replaying
  BOOL c_log_prepare(void);                      // prepares the log for replaying
  void to_protocol(char * buf, int len);
 // other:
  void mark_core(void);
};

extern t_ffa ffa;

class t_disk_space_management
{ friend class t_ffa;
  friend void save_pieces(cdp_t cdp);
  enum { FREE_LIMIT_UP = 800, FREE_LIMIT_DOWN = 150, BLOCK_FILL_LIMIT = 400 };
  t_blocknum_dynar free_blocks;      // valid data are in <0, free_blocks_cnt)
  t_blocknum_dynar pre_free_blocks;  // valid data are in <0, predeallocated_blocks_cnt)
  piece_dynar      pre_free_pieces;  // valid data are in <0, pre_free_pieces_cnt)
  unsigned predeallocated_blocks_cnt, free_blocks_cnt, pre_free_pieces_cnt;
  unsigned next_pool;
  void save_to_pool(unsigned poolnum, unsigned margin);
 // allocation:
  bool taken_from_pool;  // temporary store, must flush, a block has been taken fron a pool or from predeallocated records
  tblocknum max_phys_block;  // temporary store
  t_client_number locked_by;
  bool flush_requested;  // user of the allocated block(s) requests a flush
  bool add_new_pool(void);
  bool extend_database_if_outside(cdp_t cdp, tblocknum max_phys_block);
 public:
  CRITICAL_SECTION bpools_sem;
  bool lock_and_alloc(cdp_t cdp, unsigned count);
  void unlock_allocation(cdp_t cdp);
  void flush_request(void)
    { flush_requested=true; }
  void deallocation_move(cdp_t cdp);

 private: 
  tblocknum take_block_from_cache(void);
 // flushing:
  uns32 last_fil_flush;  // protected by commit semaphore
  int stop_flushing;
 public: 
  t_disk_space_management(void);
  ~t_disk_space_management(void);
  BOOL write_bpool(uns8 * bpool, int poolnum);
  BOOL read_bpool(int poolnum, uns8 * bpool);
  tblocknum get_block(cdp_t cdp);  /* returns a new block, 0 if cannot */  
  tblocknum get_big_block(cdp_t cdp);  /* returns a new big block, 0 if cannot */ 
  void release_block_safe(cdp_t cdp, tblocknum dadr);
  void release_block_direct(cdp_t cdp, tblocknum dadr);
  void release_piece_safe(cdp_t cdp, tpiece * pc);
  void reduce_free_blocks(void);
  void close_block_cache(cdp_t cdp);
  void get_block_buf(tblocknum ** buf,  unsigned * count);
  tblocknum free_blocks_count(cdp_t cdp);
  void extflush(cdp_t cdp, bool unconditional);
  void flush_and_time(cdp_t cdp);
  void enable_flush(void)
    { if (stop_flushing) stop_flushing--; }
  void disable_flush(void)
    { stop_flushing++; }
};

extern t_disk_space_management disksp;
void make_directory_path(tptr direc);

void check_for_fil_backup(cdp_t cdp);

inline bool position2(FHANDLE hnd, uns64 off64)
{
#ifdef WINS
  return SetFilePointer(hnd, (DWORD)off64, ((long*)&off64)+1, FILE_BEGIN) != (DWORD)-1 || GetLastError()==NO_ERROR;
#elif defined(LINUX)
  return SetFilePointer64(hnd, off64, FILE_BEGIN) != (off_t)-1;
#else
  return SetFilePointer(hnd, off64, NULL, FILE_BEGIN) != (DWORD)-1;
#endif
}

#ifdef IMAGE_WRITER
///////////////////////////////////// image writer //////////////////////////////
struct t_image_writer_item
// Rules: if [is_in_core] then [fbcn] is fixed (get_frame() cannot take it)
{ tblocknum dadr;
  bool is_in_core;
  tfcbnum fcbn;
};

class t_image_writer
{ enum { MAX_WRITE_BLOCK_COUNT = 20 };
 // synchronization objects:
  CRITICAL_SECTION writer_cs;
  LocEvent new_write_request_event;
  LocEvent block_written_event;
  THREAD_HANDLE thread_handle;
 // variables protected by [writer_cs]:
  t_image_writer_item items[MAX_WRITE_BLOCK_COUNT];
  int head, tail;  // the first and last valid item, both are -1 when empty
 // private variables used only by the image_writer_thread() and init/deinit
  bool deinit_request;  // signal fot the writer thread to terminate
  uns8 blockbuf[BLOCKSIZE];
 // private methods:
  void write_item(tfcbnum fcbn, bool is_in_core);
  void write_until_queue_empty(void);
 public:
  t_image_writer(void)
    { }
  void image_writer_thread(void);
  void init(void);
  void deinit(void);
  void add_item(tfcbnum fcbn, bool is_in_core);
  bool wait_for_releasing_frame(void);
  bool read_from_queue(tblocknum dadr, tptr frame);
  void write_to_queue(tblocknum dadr, tptr frame);
};

extern t_image_writer the_image_writer;
#endif
 /////////////////////////////////////* maintan */
bool xmark_piece(cdp_t cdp, const tpiece * pc);
void mark_used_block(cdp_t cdp, tblocknum dadr);
bool mark_index(cdp_t cdp, const dd_index * cc);
BOOL translate(tblocknum & dadr);

 // profile

class t_server_profile;

BOOL return_str(cdp_t cdp, unsigned buffer_length, const char * strval);  // auxiliary

class t_property_descr  // base class impements the persitence of a string value 
{ friend int  set_property_value(cdp_t cdp, const char * owner, const char * name, uns32 num, const char * value, uns32 valsize);
  friend void get_property_value(cdp_t cdp, const char * owner, const char * name, uns32 num, char * value, unsigned buffer_length);
  friend void get_nad_return_property_value(cdp_t cdp, const char * owner, const char * name, uns32 num, unsigned buffer_length);
 public:
  tobjname name;   // name used only in derived classes
 protected: 
  BOOL security_related;
  virtual void change_notification(cdp_t cdp)
    { }
 public:
  inline BOOL is_named(const char * name_srch) const
    { return !stricmp(name, name_srch); }
  virtual BOOL set_and_save  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual BOOL get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length);
  virtual void get_and_copy  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length);
  BOOL load_to_memory(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buf);
  virtual void set_default(void)
    { }
  virtual BOOL string_to_mem(const char * strval, uns32 num)  // used during the initial loading only
    { return FALSE; }  // cannot store to memory
  t_property_descr(const char * nameIn, BOOL sec_relIn) : security_related(sec_relIn)
    { strcpy(name, nameIn); }
  virtual ~t_property_descr(void) { }  // just to prevent warning
};

class t_property_descr_int : public t_property_descr
{protected:
  int ivalue;
  const int default_value;
 public:
  virtual BOOL set_and_save  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual BOOL get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length);
  virtual void get_and_copy  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length);
  virtual void set_default(void)
    { ivalue=default_value; }
  virtual BOOL string_to_mem(const char * strval, uns32 num);
  t_property_descr_int(const char * nameIn, BOOL sec_relIn, int default_valueIn) : 
    t_property_descr(nameIn, sec_relIn), default_value(default_valueIn) { }
  inline int val(void) const { return ivalue; }
  operator int() { return ivalue; }
  inline void friendly_save(int val)
    { ivalue=val; }
};

class t_property_descr_int_ext : public t_property_descr_int
// Like t_property_descr_int but values are stored in the registry and loaded explicitky in load_all
{ 
 public:
  virtual BOOL set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  t_property_descr_int_ext(const char * nameIn, BOOL sec_relIn, int default_valueIn) : 
    t_property_descr_int(nameIn, sec_relIn, default_valueIn) { }
};

class t_property_descr_str : public t_property_descr
{protected: 
  const char * const default_value;
  const int length;
  char * strvalue;
 public:
  virtual BOOL set_and_save  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual BOOL get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length);
  virtual void get_and_copy  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length);
  virtual void set_default(void)
    { strcpy(strvalue, default_value); }
  virtual BOOL string_to_mem(const char * strval, uns32 num)
    { strmaxcpy(strvalue, strval, length);  return TRUE; }  // cannot store to memory
  t_property_descr_str(const char * nameIn, BOOL sec_relIn, const char * default_valueIn, int lengthIn, char * strvalueIn) : 
    t_property_descr(nameIn, sec_relIn), default_value(default_valueIn), length(lengthIn), strvalue(strvalueIn) { }
  inline const char * val(void) { return strvalue; }
  inline void friendly_save(const char * strval)
    { strmaxcpy(strvalue, strval, length); }
};

class t_property_descr_str_hdn : public t_property_descr_str
{public:
  virtual BOOL set_and_save  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual BOOL get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length);
  virtual void get_and_copy  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length);
  virtual BOOL string_to_mem(const char * strval, uns32 num)
    { strmaxcpy(strvalue, strval, length);  return TRUE; }  // cannot store to memory
  t_property_descr_str_hdn(const char * nameIn, BOOL sec_relIn, const char * default_valueIn, int lengthIn, char * strvalueIn) : 
    t_property_descr_str(nameIn, sec_relIn, default_valueIn, lengthIn, strvalueIn) 
    { }
};

class t_property_descr_str_srvname : public t_property_descr_str
{public:
  virtual BOOL set_and_save  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  t_property_descr_str_srvname(const char * nameIn, BOOL sec_relIn, const char * default_valueIn, int lengthIn, char * strvalueIn) : 
    t_property_descr_str(nameIn, sec_relIn, default_valueIn, lengthIn, strvalueIn) { }
};

class t_property_descr_str_multi : public t_property_descr  // directory list
{ t_dynar list;
 public:
  virtual BOOL set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual void get_and_copy(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length);
  virtual void set_default(void)
    { list.clear(); }
  virtual BOOL string_to_mem(const char * strval, uns32 num);
  t_property_descr_str_multi(const char * nameIn, BOOL sec_relIn) : 
    t_property_descr(nameIn, sec_relIn), list(1,0,50) { }
  void add_to_list(const char * str);
  BOOL is_in_list(const char * str) const;
  inline void mark_core(void)
    { list.mark_core(); }
  const char * get_next_string(const char *old);
};

class t_property_descr_filcrypt : public t_property_descr  // database file encryption and password
{public:
  virtual BOOL set_and_save  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual BOOL get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length);
  virtual void get_and_copy  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, char * buffer, unsigned buffer_length);
  t_property_descr_filcrypt(const char * nameIn) : t_property_descr(nameIn, TRUE) { }
};

class t_property_descr_srvkey : public t_property_descr  // server certificate
{public:
  virtual BOOL set_and_save  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual BOOL get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length);
  t_property_descr_srvkey(const char * nameIn) : t_property_descr(nameIn, TRUE) { }
};

class t_property_descr_srvuuid : public t_property_descr  // server UUID
{public:
  virtual BOOL set_and_save  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual BOOL get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length);
  t_property_descr_srvuuid(const char * nameIn) : t_property_descr(nameIn, TRUE) { }
};

class t_property_descr_str_ips : public t_property_descr  // IP address and mask list
{ const BOOL is_ena, is_addr;
  const int set_num;
 public:
  virtual BOOL set_and_save(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual BOOL string_to_mem(const char * strval, uns32 num);
  t_property_descr_str_ips(const char * nameIn, BOOL sec_relIn, BOOL is_enaIn, BOOL is_addrIn, int set_numIn) : 
    t_property_descr(nameIn, sec_relIn), is_ena(is_enaIn), is_addr(is_addrIn), set_num(set_numIn) { }
};

class t_property_descr_licnum : public t_property_descr  // add-on licence number - storing only
{public:
  virtual BOOL set_and_save  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual BOOL get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length);
  t_property_descr_licnum(const char * nameIn) : t_property_descr(nameIn, FALSE) { }
};

#if WBVERS<900
class t_property_descr_gcr : public t_property_descr  // GCR - just for updating the GCR by the client - not used any more
{public:
  virtual BOOL set_and_save  (cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, const char * value, unsigned vallen);
  virtual BOOL get_and_return(cdp_t cdp, t_server_profile * sp, const char * owner, uns32 num, unsigned buffer_length);
  t_property_descr_gcr(const char * nameIn) : t_property_descr(nameIn, FALSE) { }
};
#endif

struct t_trace_property_assoc  // Association of persistent server properties and trace situations
{ uns32 situaton;
  t_property_descr_int * prop;
};

enum { TRACE_PROPERTY_ASSOC_COUNT=24 };

class t_server_profile
{ friend class t_property_descr;
  friend class t_property_descr_int;
  friend class t_property_descr_int_ext;
  friend class t_property_descr_str;
#if WBVERS<900
  enum { SERVER_PROFILE_ITEM_COUNT = 135 };  // 800
#else
  enum { SERVER_PROFILE_ITEM_COUNT = 131 };  // >=900
#endif
  static t_property_descr * server_profile_items[SERVER_PROFILE_ITEM_COUNT];
#ifdef WINS
  HKEY hKeyInst, hKeyDb;
#else
  char pathname[MAX_PATH];
#endif
#if WBVERS<900
  static char reqProtocolStr[40];
#endif
  static char thrusted_domain[MAX_LOGIN_SERVER+1];
  static char top_CA_str[2*UUID_SIZE+1];
  static char backup_directory[MAX_PATH+1];
  static char backup_zip_move_to[MAX_PATH+1];
  static char backup_net_username[100+1];
  static char backup_net_password[100+1];
  static char kerberos_name_str[50];
  static char kerberos_cache_str[MAX_PATH+1];
  static char server_ip_addr_str[24];
  static tobjname http_user_name;
  static char ext_fulltext_dir[254+1];
  static char host_name[70+1];
  static char host_path_prefix[70+1];
  static char extlob_directory_str[100+1];

 public:
  static t_trace_property_assoc trace_property_assocs[TRACE_PROPERTY_ASSOC_COUNT];
  t_property_descr * find(const char * name_srch) const;
  BOOL open(BOOL write_access);
  void close(void);
  void get_top_CA_uuid(WBUUID uuid);
//  inline const char * get_backup_directory(void) const { return backup_directory; }

  void load_all(cdp_t cdp);
  void set_all_to_defaut(void);
  void add_to_dir_list(const char * direc);
  static t_property_descr_int ipc_comm;
  static t_property_descr_int net_comm;
  static t_property_descr_int wb_tcpip_port;
  static t_property_descr_int http_tunel;
  static t_property_descr_int_ext http_tunnel_port;
  static t_property_descr_int web_emul;
  static t_property_descr_int_ext web_port;
  static t_property_descr_int ext_web_server_port;
  static t_property_descr_int using_emulation;

  static t_property_descr_int DisableAnonymous;
  static t_property_descr_int DisableHTTPAnonymous;
  static t_property_descr_int MinPasswordLen;
  static t_property_descr_int FullProtocol;
  static t_property_descr_int PasswordExpiration;
  static t_property_descr_int ConfAdmsOwnGroup;
  static t_property_descr_int ConfAdmsUserGroups;
  static t_property_descr_str http_user;
  static t_property_descr_str server_ip_addr;
  static t_property_descr_int kill_time_notlogged, kill_time_logged;
  static t_property_descr_int UnlimitedPasswords;
  static t_property_descr_int trace_log_cache_size;
  static t_property_descr_int show_as_tray_icon;
  //static t_property_descr_int ascii_console_output;
  static t_property_descr_int integr_check_interv;
  static t_property_descr_int integr_check_extent;
  static t_property_descr_int IntegrTimeDay1;
  static t_property_descr_int IntegrTimeHour1;
  static t_property_descr_int IntegrTimeMin1;
  static t_property_descr_int IntegrTimeDay2;
  static t_property_descr_int IntegrTimeHour2;
  static t_property_descr_int IntegrTimeMin2;
  static t_property_descr_int IntegrTimeDay3;
  static t_property_descr_int IntegrTimeHour3;
  static t_property_descr_int IntegrTimeMin3;
  static t_property_descr_int IntegrTimeDay4;
  static t_property_descr_int IntegrTimeHour4;
  static t_property_descr_int IntegrTimeMin4;
  static t_property_descr_int integr_check_priority;
  static t_property_descr_int default_SQL_options;

  static t_property_descr_int kernel_cache_size;
  static t_property_descr_int core_alloc_size;          // default is zero, the real default is defined in smemory.cpp
  static t_property_descr_int DisableFastLogin;
  static t_property_descr_int CloseFileWhenIdle;
  static t_property_descr_int profile_bigblock_size;    // max. space used by a single sorting (in KB)
  static t_property_descr_int total_sort_space;         // max. space used al all concurrent sortings (in KB)
  static t_property_descr_str_multi dll_directory_list; // list of directory paths separated by 0s, without path separators on their ends
  static t_property_descr_str ThrustedDomain;
  static t_property_descr_int CreateDomainUsers;
  unsigned reqProtocol;
  static t_property_descr_filcrypt FilEncrypt;
  static t_property_descr_int WriteJournal;
  static t_property_descr_int SecureTransactions;
  static t_property_descr_int FlushOnCommit;
  static t_property_descr_int FlushPeriod;
  static t_property_descr_int down_conditions;
  static t_property_descr_str_ips IP_enabled_addr;
  static t_property_descr_str_ips IP_enabled_mask;
  static t_property_descr_str_ips IP_disabled_addr;
  static t_property_descr_str_ips IP_disabled_mask;
  static t_property_descr_str_ips IP1_enabled_addr;
  static t_property_descr_str_ips IP1_enabled_mask;
  static t_property_descr_str_ips IP1_disabled_addr;
  static t_property_descr_str_ips IP1_disabled_mask;
  static t_property_descr_str_ips IP2_enabled_addr;
  static t_property_descr_str_ips IP2_enabled_mask;
  static t_property_descr_str_ips IP2_disabled_addr;
  static t_property_descr_str_ips IP2_disabled_mask;
  static t_property_descr_srvkey  SrvKey;
  static t_property_descr_srvuuid SrvUUID;
  static t_property_descr_int required_comm_enc;
  static t_property_descr_str top_CA_uuid;              
  static t_property_descr_licnum AddOnLicence;

  static t_property_descr_str BackupDirectory;
  static t_property_descr_int BackupFilesLimit;
  static t_property_descr_int BackupIntervalHours;
  static t_property_descr_int BackupIntervalMinutes;
  static t_property_descr_int BackupTimeDay1;
  static t_property_descr_int BackupTimeHour1;
  static t_property_descr_int BackupTimeMin1;
  static t_property_descr_int BackupTimeDay2;
  static t_property_descr_int BackupTimeHour2;
  static t_property_descr_int BackupTimeMin2;
  static t_property_descr_int BackupTimeDay3;
  static t_property_descr_int BackupTimeHour3;
  static t_property_descr_int BackupTimeMin3;
  static t_property_descr_int BackupTimeDay4;
  static t_property_descr_int BackupTimeHour4;
  static t_property_descr_int BackupTimeMin4;
  static t_property_descr_int BackupZip;
  static t_property_descr_str BackupZipMoveTo;
  static t_property_descr_int NonblockingBackups;
  static t_property_descr_str BackupNetUsername;
  static t_property_descr_str_hdn BackupNetPassword;

  static t_property_descr_int trace_user_error;
  static t_property_descr_int trace_network_global;
  static t_property_descr_int trace_replication;
  static t_property_descr_int trace_direct_ip;
  static t_property_descr_int trace_user_mail;
  static t_property_descr_int trace_sql;
  static t_property_descr_int trace_login;
  static t_property_descr_int trace_cursor;
  static t_property_descr_int trace_impl_rollback;
  static t_property_descr_int trace_network_error;
  static t_property_descr_int trace_replic_mail;
  static t_property_descr_int trace_replic_copy;
  static t_property_descr_int trace_repl_conflict;
  static t_property_descr_int trace_bck_obj_error;
  static t_property_descr_int trace_procedure_call;
  static t_property_descr_int trace_trigger_exec;
  static t_property_descr_int trace_user_warning;
  static t_property_descr_int trace_server_info;
  static t_property_descr_int trace_server_failure;
  static t_property_descr_int trace_log_write;
  static t_property_descr_int trace_start_stop;
  static t_property_descr_int trace_lock_error;
  static t_property_descr_int trace_web_request;
  static t_property_descr_int trace_convertor_error;

  static t_property_descr_str kerberos_name;
  static t_property_descr_str kerberos_cache;
  static t_property_descr_str decimal_separator;
  static t_property_descr_int DailyBasicLog;
  static t_property_descr_int DailyUserLogs;
  static t_property_descr_int LockWaitingTimeout;
#if WBVERS<900
  static t_property_descr_int ReportLowLicences;
  static t_property_descr_gcr gcr;
  static t_property_descr_int EnableConsoleButton;
  static t_property_descr_str reqProtocolProp;          // the requested network protocol 
#endif
  static t_property_descr_str ExtFulltextDir;
  static t_property_descr_str HostName;
  static t_property_descr_str HostPathPrefix;
  static t_property_descr_int report_slow;
  static t_property_descr_int max_routine_stack;
  static t_property_descr_int max_client_cursors;
  static t_property_descr_int DiskSpaceLimit;
  static t_property_descr_str extlob_directory;

  static t_property_descr_str_srvname ServerName;
};

extern t_server_profile the_sp;
void get_property_value(cdp_t cdp, const char * owner, const char * name, uns32 num, char * value, unsigned buffer_length);
int  set_property_value(cdp_t cdp, const char * owner, const char * name, uns32 num, const char * value, uns32 valsize);
void get_nad_return_property_value(cdp_t cdp, const char * owner, const char * name, uns32 num, unsigned buffer_length);
int  backup_entry(cdp_t cdp, const char * expl_fname);
unsigned get_last_separator_pos(const char * pathname);
BOOL WritePrivateProfileInt(const char * section, const char * key, int value, const char * pathname);

void park_server_settings(cdp_t cdp);
int hex2bin_str(const char * str, uns8 * bin, int maxsize);

// bits in down_conditions:
#define REL_NONEX_BLOCK 1  // releasing a bad block
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SPECIF_DYNAR(t_thread_handle_dynar, THREAD_HANDLE, 2);

class t_offline_threads
{ t_thread_handle_dynar thread_handle_dynar;  // free cells have the INVALID_THANDLE_VALUE value
  bool joining_on_exit;
 public:
  bool add(THREAD_HANDLE thread_handle);
  void remove(THREAD_HANDLE thread_handle);
  void join_all(void);
  void mark_core(void)
    { thread_handle_dynar.mark_core(); }
  t_offline_threads(void) : joining_on_exit(false) { }
};

extern t_offline_threads offline_threads;
extern BOOL bCancelCopying;
///////////////////////////////////////////// hot backup support //////////////////////////////////////////////////

class t_backup_support
// Access is protected by [file_cs] shared with the backupped file 
{protected:
  char auxi_file_name[MAX_PATH];  
  FHANDLE hnd;
  t_blocknum_dynar list;  // valid part of the list starts at [start]
  int start;  // 0 in phase 1, the first block not copied from the auxiliary file in phase 2
  bool phase2;
  bool rd(unsigned pos, void * ptr);
  unsigned total_writes, invals;
  CRITICAL_SECTION * file_cs;
  virtual const char * get_auxi_file_name(void) const = 0;
  virtual tblocknum get_file_block_count(void) const = 0;
  virtual void write_buf_to_file(tblocknum blk, char * buf) = 0;
  virtual void expand_file_size(void) = 0;
  virtual unsigned block_size(void) const = 0;
 public:
  t_backup_support(CRITICAL_SECTION * file_csIn) : file_cs(file_csIn)
    { }
  tblocknum new_db_file_size;
  bool prepare(bool restoring);
  void unprepare(void ** p_backup_support);
  bool is_phase1(void) const
    { return !phase2; }
  bool write_block(tblocknum blocknum, const void * ptr);
  bool read_block(tblocknum blocknum, void * ptr);
  void invalidate_block(tblocknum blocknum);
  void synchronize(bool restoring, void ** p_backup_support);
  const char * my_auxi_file_name(void) const  // just for reporting the error
    { return auxi_file_name; }
};

class t_backup_support_fil : public t_backup_support
// Hot backup for the database file
{
  const char * get_auxi_file_name(void) const
    { return "bckpsupp.fil"; }
  tblocknum get_file_block_count(void) const
    { return ffa.get_fil_size(); }
  void write_buf_to_file(tblocknum blk, char * buf)
    { ffa.w_enc_write_block(blk, buf); }
  void expand_file_size(void)
    { ffa.w_enc_write_block(new_db_file_size-1, (tptr)corepool);  // this updates [db_file_size], too
      ffa.close_fil_parts();  // increates the file length
      ffa.open_fil_parts();
      ffa. get_database_file_size();
    }
  unsigned block_size(void) const
    { return BLOCKSIZE; }
 public:
  t_backup_support_fil(CRITICAL_SECTION * file_csIn) : t_backup_support(file_csIn) { }
};

class ext_index_file;

class t_backup_support_ftx : public t_backup_support
// Hot backup for an external fulltext system
{ ext_index_file * eif;
  char auxi_name[2*OBJ_NAME_LEN+9+1];
  const char * get_auxi_file_name(void) const
  // Every fulltext system must have a different name for its auxiliary file. If the backup is broken the name
  //  identifies the FT system the file belongs to.
    { return auxi_name; }
  void write_buf_to_file(tblocknum blk, char * buf);
  tblocknum get_file_block_count(void) const;
  void expand_file_size(void);
  unsigned block_size(void) const;
 public:
  t_backup_support_ftx(CRITICAL_SECTION * file_csIn, ext_index_file * eifIn, const char * schema_name, const char * ft_name)
    : t_backup_support(file_csIn), eif(eifIn)
  { sprintf(auxi_name, "%s.%s.bckftx", schema_name, ft_name);
  }
};

class t_temp_priority_change 
{ int old_priority;
 public:
  t_temp_priority_change(bool up)
  { 
#ifdef WINS
    old_priority = GetThreadPriority(GetCurrentThread());
    SetThreadPriority(GetCurrentThread(), up ? THREAD_PRIORITY_ABOVE_NORMAL : THREAD_PRIORITY_BELOW_NORMAL);
#endif    
  }  
  ~t_temp_priority_change(void)
  { 
#ifdef WINS
    SetThreadPriority(GetCurrentThread(), old_priority); 
#endif    
  }
};

void propagate_system_error(const char * pattern, const char * parameter, char * errmsg = NULL);
void complete_broken_ft_backup(cdp_t cdp);

void _sqp_backup_get_pathname_pattern(const char * directory_path, char * pathname_pattern);
int _sqp_backup(cdp_t cdp, const char * pathname_pattern, bool nonblocking, int extent);
int _sqp_backup_reduce(const char * directory_path, int max_count);
int _sqp_backup_zip(const char * pathname_pattern, const char * dest_dir, const char * move_dir);
int _sqp_connect_disk(const char * path, const char * username, const char * password);
void _sqp_disconnect_disk(const char * path);
extern CRITICAL_SECTION cs_gcr;
