// extbtree.h
#include "profiler.h"
/////////////////////////// mrsw lock ///////////////////////////////////////
#ifdef LINUX

class t_mrsw_lock
{ 
  int readers_inside;
  bool writer_inside;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;

 public:
  t_mrsw_lock(void)
  { readers_inside=0;  writer_inside=false;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init (&cond, NULL);
  }
  ~t_mrsw_lock(void)
  {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
  }
  void reader_entrance(cdp_t cdp, t_wait wait_type)
  { prof_time start;
    if (PROFILING_ON(cdp)) start = get_prof_time();
    pthread_mutex_lock(&mutex);
    while (writer_inside)
      pthread_cond_wait(&cond, &mutex);
    readers_inside++;
    pthread_mutex_unlock(&mutex);
    if (PROFILING_ON(cdp)) add_hit_and_time(get_prof_time()-start, PROFCLASS_CS, wait_type-100, 0, NULL);
  }

  void reader_exit(void)
  {
    pthread_mutex_lock(&mutex);
    readers_inside--;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
  }

  void writer_entrance(cdp_t cdp, t_wait wait_type)
  { prof_time start;
    if (PROFILING_ON(cdp)) start = get_prof_time();
    pthread_mutex_lock(&mutex);
    while (readers_inside || writer_inside)
      pthread_cond_wait(&cond, &mutex);
    writer_inside=true;
    pthread_mutex_unlock(&mutex);
    if (PROFILING_ON(cdp)) add_hit_and_time(get_prof_time()-start, PROFCLASS_CS, wait_type-100, 0, NULL);
  }

  void writer_exit(void)
  {
    pthread_mutex_lock(&mutex);
    writer_inside=false;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
  }
};


#elif WINS

class t_mrsw_lock
{ int readers_inside;
  bool writer_inside;
  CRITICAL_SECTION acs;
  HANDLE hEvent;

 public:
  t_mrsw_lock(void);
  ~t_mrsw_lock(void);
  void reader_entrance(cdp_t cdp, t_wait wait_type);
  void reader_exit(void);
  void writer_entrance(cdp_t cdp, t_wait wait_type);
  void writer_exit(void);
};

#else
class t_mrsw_lock
{ int waiting;
  int readers_inside;
  bool writer_inside;
  CRITICAL_SECTION acs;
  Semaph entry_semaphore;

 public:
  t_mrsw_lock(void)
  { waiting=0;  readers_inside=0;  writer_inside=false;
    InitializeCriticalSection(&acs);
    CreateSemaph(1, 100, &entry_semaphore);
  }
  ~t_mrsw_lock(void)
  {
    DeleteCriticalSection(&acs);
    CloseSemaph(&entry_semaphore);
  }
  void reader_entrance(void)
  {// increase the waiting] counter:
    EnterCriticalSection(&acs);
    waiting++;
    LeaveCriticalSection(&acs);
    bool can_enter;
    do
    {// check the conditions:
      EnterCriticalSection(&acs);
      if (writer_inside)
        { can_enter=false;   }
      else
        { can_enter=true;  waiting--;  readers_inside++; }
      LeaveCriticalSection(&acs);
      if (!can_enter)  // wait on the semaphore
        WaitSemaph(&entry_semaphore, INFINITE);
      else break;
    } while (true);
  }

  void reader_exit(void)
  {
    EnterCriticalSection(&acs);
    readers_inside--;
    ReleaseSemaph(&entry_semaphore, waiting);
    LeaveCriticalSection(&acs);
  }

  void writer_entrance(void)
  {// increase the waiting] counter:
    EnterCriticalSection(&acs);
    waiting++;
    LeaveCriticalSection(&acs);
   // wait on the semaphore
    bool can_enter;
    do
    {// check the conditions:
      EnterCriticalSection(&acs);
      if (writer_inside || readers_inside)
          can_enter=false;
      else
        { can_enter=true;  waiting--;  writer_inside=true; }
      LeaveCriticalSection(&acs);
      if (!can_enter)  // wait on the semaphore
        WaitSemaph(&entry_semaphore, INFINITE);
      else break;
    } while (true);
  }

  void writer_exit(void)
  {
    EnterCriticalSection(&acs);
    writer_inside=false;
    ReleaseSemaph(&entry_semaphore, waiting);
    LeaveCriticalSection(&acs);
  }
};
#endif

class t_reader
{ t_mrsw_lock * mswr;
 public:
  t_reader(t_mrsw_lock * mswrIn, cdp_t cdp) : mswr(mswrIn)
    { mswr->reader_entrance(cdp, WAIT_EXTFT); }
  ~t_reader(void)
    { mswr->reader_exit(); }
};

class t_writer
{ t_mrsw_lock * mswr;
 public:
  t_writer(t_mrsw_lock * mswrIn, cdp_t cdp) : mswr(mswrIn)
    { mswr->writer_entrance(cdp, WAIT_EXTFT); }
  ~t_writer(void)
    { mswr->writer_exit(); }
};



#if WBVERS>=950

bool get_fulltext_file_name(cdp_t cdp, char * fname, const char * schemaname, const char *suffix);

typedef uns32  cbnode_number;
typedef int    cbnode_pos;
enum { cbnode_inner_magic = 0x715c, cbnode_leaf_magic = 0x3f68 };

struct cb_node
{ uns16 magic;
  uns16 used_space;  // includes the cb_node header
  cbnode_number leftmost_son;
  inline bool is_leaf(void) const
    { return magic == cbnode_leaf_magic; }
  inline uns8 * key(int pos, int step)
    { return (uns8*)this + sizeof(cb_node) + pos*step; }
  inline cbnode_number * son(int pos, int step)   // son pointers numbered from 0 to count-1.
    { return pos==-1 ? &leftmost_son : (cbnode_number*)((uns8*)this + sizeof(cb_node) + (pos+1)*step - sizeof(cbnode_number)); }
  inline void * end_pos(void)
    { return (uns8*)this + used_space; }
  void init_root(bool leaf)
    { magic=leaf ? cbnode_leaf_magic : cbnode_inner_magic;  used_space=sizeof(cb_node);  leftmost_son=(cbnode_number)-1; }
};

typedef cb_node * cbnode_ptr;
enum cb_result { CB_ERROR, CB_FOUND_EXACT, CB_FOUND_CLOSEST, CB_NOT_FOUND, CB_SPLIT };
enum { cb_cluster_size = 4096 };   
/////////////////// frame allocation ////////////////////////////////
cbnode_ptr alloc_cb_frame(void);
void free_cb_frame(cbnode_ptr frame);


/////////////////////////////////////////////////////
/* Leave structure:
  leave header: contains magic number and position after the last value
  sequence of full values (2nd and the following may be shortened)
   Inner node structure
  inner node header: contains magic number, cbnode_number of the leftmost son and position after the last value
  sequence of key values (2nd and the following may be shortened)
  Logic: values in left son(s) < key value <= values in right son(s), but the key value may not exist in leaves
*/

class cbnode_oper
// Abstract class implementing the basic operation on the cluster
{ //virtual bool is_end_pos(cbnode_ptr cbnode, cbnode_pos & pos);
  //virtual void to_next_pos(cbnode_ptr cbnode, cbnode_pos & pos);
   // supposes is_end_pos()==false on entry
 protected:
  void define_steps(void);
 public:
  unsigned key_value_size, clustered_value_size;
  unsigned leaf_step, inner_step;
  cb_result search_node(const void * key, cbnode_ptr cbnode, cbnode_pos & pos);
  virtual int cmp_keys(const void * key1, const void * key2) = 0;
  void insert_direct(cbnode_ptr ptr, cbnode_pos pos, void * key_value, void * clustered_value, unsigned step);
};

class cbnode_oper_wi : public cbnode_oper
{public: 
  int cmp_keys(const void * key1, const void * key2);
  cbnode_oper_wi(void);
};

class cbnode_oper_ri1_32 : public cbnode_oper
{public: 
  int cmp_keys(const void * key1, const void * key2);
  cbnode_oper_ri1_32(void);
};

class cbnode_oper_ri2_32 : public cbnode_oper
{public: 
  int cmp_keys(const void * key1, const void * key2);
  cbnode_oper_ri2_32(void);
};

class cbnode_oper_ri1_64 : public cbnode_oper
{public: 
  int cmp_keys(const void * key1, const void * key2);
  cbnode_oper_ri1_64(void);
};

class cbnode_oper_ri2_64 : public cbnode_oper
{public: 
  int cmp_keys(const void * key1, const void * key2);
  cbnode_oper_ri2_64(void);
};

/////////////////////////// path ///////////////////////////////
#define MAX_CBKEY_LENGTH  50

struct cbnode_path_item
{ cbnode_ptr ptr;
  cbnode_pos pos;
  cbnode_number num;
};

class ext_index_file;

struct cbnode_path
{ enum { MAX_CBNODE_PATH = 15 };
  int top;  // -1 if the path is empty
  cbnode_path_item items[MAX_CBNODE_PATH];
  void close(void);
  cbnode_path_item * push(ext_index_file & eif, cbnode_number num, cbnode_oper & oper);
  void pop(void);
  cbnode_path(void)
    { top=-1; }
  ~cbnode_path(void)
    { close(); }
  void * clustered_value_ptr(cbnode_oper & oper);
  void check(void);
  bool start_ordered_creating(ext_index_file * eif, cbnode_oper * oper);
  bool cb_add_ordered(ext_index_file * eif, cbnode_oper * oper, const char * keyandval);
  bool close_ordered_creating(ext_index_file * eif, cbnode_oper * oper, cbnode_number root);
};


///////////////////// index file access ////////////////////////////////
// File structure: cluster 0 is the header, cluster 1 is the 1st pool, every cluster 1+8*cb_cluster_size is the pool.
typedef unsigned counter_type;
#define CB_NODE_CACHE_SIZE 40

class ext_index_file
{ friend class ProtectedByCriticalSection;
  friend class t_backup_support_ftx;
  FHANDLE handle;             // INVALID_FHANDLE_VALUE when file is not opened
  cbnode_number block_count;  // total count of blocks in the file (incl. header block), defined when the file is open
  bool position(cbnode_number number) const;
  uns32 pool[cb_cluster_size/4];
  int loaded_pool_number;    // valid when file opened, -1 when no pool loaded
  unsigned pool_search_position;  // byte to be checked when allocating, valid when a pool is loaded
  bool read_pool(int pool_number);
  bool write_pool(void);
  counter_type next_counter_value;
  unsigned allocated_counter_values;
  cbnode_number node_cache[CB_NODE_CACHE_SIZE];
  unsigned nodes_in_cache;
 public:
  t_backup_support_ftx * backup_support;  // not NULL during a non-blocking backup of the fulltext system
  CRITICAL_SECTION cs_file;  // protects positioning of the file pointer until the end of the read/write/setend operation
  t_mrsw_lock mrsw_lock;
  unsigned err_magic, err_used_space, err_read, err_alloc, err_stack, err_order;
  ext_index_file(void);
  ~ext_index_file(void);
  bool create(const char * fname);
  bool open(const char * fname);
  void close(void);
  bool init_ext_ft_index(void);
  bool read0(cbnode_number number, cbnode_ptr ptr);
  bool read(cbnode_number number, cbnode_ptr ptr, cbnode_oper & oper);
  bool write(cbnode_number number, cbnode_ptr ptr);
  cbnode_number alloc_cbnode(void);
  void free_cbnode(cbnode_number);
  void save_to_current_pool(void);
  counter_type get_counter_value(void);
  void contain_block(cbnode_number number);
  void get_errors(unsigned & error_magic, unsigned & error_used_space, unsigned & error_read, unsigned & error_alloc, 
                  unsigned & error_stack, unsigned & error_order) const
  { error_magic=err_magic; error_used_space=err_used_space;
    error_read=err_read;   error_alloc=err_alloc;
    error_stack=err_stack; error_order=err_order;
  }
  int backup_it(cdp_t cdp, const char * fname, bool nonblocking, const char * schema_name, const char * ft_name, bool reduce);
  void complete_broken_backup(const char * schema_name, const char * ft_name);
};

#define WORD_INDEX_ROOT_NUM 1
#define REF1_INDEX_ROOT_NUM 2
#define REF2_INDEX_ROOT_NUM 3
#define COUNTER_NODE_NUM    4

bool cb_insert_val(ext_index_file & eif, cbnode_oper & oper, cbnode_number root, void * key_value, void * clustered_value);
cb_result cb_build_stack(ext_index_file & eif, cbnode_oper & oper, cbnode_number root, void * key_value, cbnode_path & path);
cb_result cb_build_stack_find(ext_index_file & eif, cbnode_oper & oper, cbnode_number root, void * key_value, cbnode_path & path);
bool cb_step(ext_index_file & eif, cbnode_oper & oper, void * key_value, cbnode_path & path);
bool _cb_remove_val(ext_index_file & eif, cbnode_oper & oper, cbnode_path & path);
bool cb_remove_val(ext_index_file & eif, cbnode_oper & oper, cbnode_number root, void * key_value);
void cb_pass_all(ext_index_file & eif, cbnode_oper & oper, cbnode_number root);

/////////////////////////////////// sort from the scratch /////////////////////////////////////
extern unsigned used_sort_space;

struct t_chain_link // stored on the end of each block of a chain
{ tblocknum next_block; // the number of the next block in the chain or 0 if the chain ends here
  unsigned local_nodes; // number of nodes in the block, >0, <=nodes_per_block
};

class t_chain_matrix
// Hierarchicka evidence retezcu, pro kazde "level" obsahuje posloupnost retezcu
// Pokud je posloupnost pro nejakou level prazdna, je prazdna i pro vsechny vyssi level.
{public:
  enum { MAX_EVID = 1000, MAX_HIERAR = 20 };
 private:
  tblocknum evid[MAX_EVID];    // internal matrix of chain heads
  unsigned chain_count[MAX_HIERAR];
  unsigned loc_bbframes;
 public:
  inline void init(unsigned bbframesIn)  // must be called before any other operation
    { loc_bbframes=bbframesIn; }
  inline void add_chain(int level, tblocknum block)
    { evid[level*loc_bbframes+chain_count[level]]=block;  chain_count[level]++; }
  inline tblocknum get_chain(int level, int num)
    { return evid[level*loc_bbframes+num]; }
  inline unsigned chain_cnt(int level)
    { return chain_count[level]; }
  inline void null_level(int level)
    { chain_count[level]=0; }
  t_chain_matrix(void)
  { for (unsigned evid_level=0;  evid_level<=MAX_HIERAR;  evid_level++)
      chain_count[evid_level]=0;
  }
};

class t_scratch_sorter
{ enum { MAX_MERGED = 300 };
  unsigned nodes_in_bigblock;
  void * bigblock_handle;   // !=0 iff bigblock allocated
  char * bigblock, * medval, * keyspace, * last_key_on_output;
  unsigned keysize, bbrecmax, bbsize, bbframes;
  cbnode_oper * oper;
  ext_index_file * eif;
  inline tptr node(unsigned index)
    { return bigblock+keysize*index; }
  void quick_sort_bigblock(int start, int stop);
  t_chain_matrix chains;
  unsigned nodes_per_block; // max. number of nodes which can be stored in a BLOCKSIZE block
  char * merged_blocks[MAX_MERGED];  
  cbnode_path output_path;
  int cbnodes_in_chains;
  bool distinct;
 public:
  void add_key(const void * key_value, const void * clustered_value);
  t_scratch_sorter(cbnode_oper * operIn, ext_index_file * eifIn, bool distinctIn) : oper(operIn), eif(eifIn), distinct(distinctIn)
  { nodes_in_bigblock=0;  oper=operIn;  keysize=oper->leaf_step;  
    nodes_per_block = (BLOCKSIZE-sizeof(t_chain_link)) / keysize;
    cbnodes_in_chains=0;
  }
  ~t_scratch_sorter(void)
  { corefree(medval);  corefree(keyspace); 
    if (bigblock_handle)
    { aligned_free(bigblock_handle);
      used_sort_space -= bbsize / 1024;
    }
    if (cbnodes_in_chains)
      dbg_err("scratch sorter error");
  }
  bool prepare(cdp_t cdp);
  tblocknum bigblock_to_chain(void);
  void bigblock_completed(void);
  bool optimize_merge(unsigned level);
  void write_result_chain(bool final_merge, tptr result_block, unsigned result_cnt, cbnode_number & current_result_blocknum, bool is_last_block);
  tblocknum merge_chains(unsigned level, bool final_merge);
  void write_result(tptr result_block, unsigned count);
  void complete_sort(cbnode_number root);
};


#endif
