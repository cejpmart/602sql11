/****************************************************************************/
/* Btree.h - operations on B-trees                                          */
/****************************************************************************/

#pragma pack(1)
struct nodebeg     // start of the btree index node
{ uns16 filled;    /* number of info bytes in the node, |0x8000 iff leaf */
  uns16 index_id;  /* identifier of the index node */
  tblocknum ptr1;
  char  key1[1];
  void init(void);
};
#pragma pack()

#define IBLOCKSIZE    BLOCKSIZE
#define LEAF_MARK     0x8000
#define SIZE_MASK     0x7fff
#define IHDRSIZE (2*sizeof(uns16))
#define INDEX_ID 0xd93a
//////////////////////// btree access structures ///////////////////////////
struct stkcell 
{ tblocknum nn;
  uns16 posit;
  uns16 nstate;   /* uns16 because of align, the real type is node_state */
};

#define BSTACK_LEN    15

typedef enum { BT_ERROR, BT_EXACT_REC, BT_EXACT_KEY, BT_CLOSEST, BT_NONEX } bt_result;

struct t_btree_acc
/* Normal state when the access is open:
stack[0] ... stack[sp] contains the path from the root to a leaf
Only the leaf page is fixed in [fcbn]. Nothing is fixed on entry to bt_remove2 (because I need a private page).
[skip_current] says that the positioned item should not be returned by btree_step()
[idx_descr] is the index desription specified in build_stack().
*/
{ stkcell stack[BSTACK_LEN];  // <0..sp> are valid items, 0 is the root
  int     sp;                 // stact top pointer, -1 iff stack empty
  tfcbnum fcbn;               // may be NOFCBNUM if nothing locked
  bool    skip_current;       // build_stack() sets this to false, passing any record sets true
  dd_index * idx_descr;       // non-owning pointer defined by build_stack() and used by btree_step()
  cdp_t   fix_cdp;
  inline t_btree_acc(void)
  { sp=-1;  fcbn=NOFCBNUM;
  }  // this is the closed state

  inline BOOL push(cdp_t cdp, tblocknum node_num)
  { 
    stack[++sp].nn=node_num;   /* new stack top, locked but not fixed yet */
    return FALSE;
  }

  inline bool drop(void)
  { 
    sp--;
    return sp>=0;
  }

  inline void unfix(cdp_t cdp)
  { if (fcbn!=NOFCBNUM)
      { UNFIX_PAGE(cdp, fcbn);  fcbn=NOFCBNUM; } // this allows re-calling
  }
  inline void close(cdp_t cdp)  // Unfixes fcbn. Is idempotent.
  { 
    unfix(cdp);  // must first unfix and then unlock because other process may immediately remove changes on the unlocked page
    sp=-1;  
  }
  ~t_btree_acc(void)
    { close(fix_cdp); }
 // passing the index:
  bt_result build_stack(cdp_t cdp, tblocknum root, dd_index * idx_descr, const char * key);
  BOOL btree_step(cdp_t cdp, tptr key);
  BOOL get_key_rec(cdp_t cdp, tptr key, trecnum * precnum) const;
 // removing the positioned item:
  BOOL bt_remove2(cdp_t cdp, tblocknum root, t_translog * translog);
};

// changing the index:
BOOL bt_insert(cdp_t cdp, tptr key, dd_index * idx_descr, tblocknum root, const char * tabname, t_translog * translog);
BOOL bt_remove(cdp_t cdp, tptr key, dd_index* idx_descr, tblocknum root, t_translog * translog);

// batch creation and destruction:
struct t_btree_stack
{ tfcbnum fcbn;
  tblocknum dadr;
};
void add_ordered(cdp_t cdp, tptr key, unsigned keysize, t_btree_stack * stack, int * top, t_translog * translog);
tblocknum close_creating(cdp_t cdp, t_btree_stack * stack, int top, t_translog * translog);
bool create_empty_index(cdp_t cdp, tblocknum & dadr, t_translog * translog);
void drop_index(cdp_t cdp, tblocknum root, unsigned bigstep, t_translog * translog);
void drop_index_idempotent(cdp_t cdp, tblocknum & root, unsigned bigstep, t_translog * translog);
void truncate_index(cdp_t cdp, tblocknum root, unsigned bigstep, t_translog * translog);
tblocknum create_btree(cdp_t cdp, table_descr * tbdf, dd_index * indx);
void sort_rec_in(cur_descr * cd);
void count_index_space(cdp_t cdp, tblocknum root, unsigned bigstep, unsigned & block_cnt, unsigned & used_space);

// key comparison:
sig32 cmp_keys(const char * key1, const char * key2, dd_index * idx_descr);
sig32 cmp_keys_partial(const char * key1, const char * key2, dd_index * idx_descr, int compare_parts);

// getting information from the index:
BOOL get_first_value(cdp_t cdp, tblocknum root, const dd_index * idx_descr, tptr key);
BOOL get_last_value (cdp_t cdp, tblocknum root, const dd_index * idx_descr, tptr key);
BOOL find_closest_key(cdp_t cdp, table_descr * td, int indnum, void * search_key, void * found_key);
bool check_all_indicies(cdp_t cdp, bool systab_only, int extent, bool repair, int & cache_error, int & index_error);
int  check_index(cdp_t cdp, table_descr * tbdf, int indnum);
int check_index_set(cdp_t cdp, uns8 * schema, ttablenum tabnum, int indnum);

class t_index_interval_itertor
{ tptr startkey, stopkey;
  dd_index * indx;  // not owning
  t_btree_acc bac;
  t_btree_read_lock btrl;
  BOOL eof;
  trecnum last_position;
  cdp_t cdp;
 public:
  table_descr * td;
  t_index_interval_itertor(cdp_t cdpIn) : cdp(cdpIn)
    { td=NULL; }
  ~t_index_interval_itertor(void)
    { close(); }
  BOOL open(ttablenum tbnum, int indnum, void * startkeyIn, void * stopkeyIn);
  trecnum next(void);
  void close_passing(void);
  void close(void);
};

bool update_private_changes_in_index(cdp_t cdp, t_index_uad * iuad);
