/****************************************************************************/
/* Dheap.h - disc heap & heap structures management                         */
/****************************************************************************/
#define DWORM_MARK 0xde5f
#define DWORM2_MARK 0x4f9c

extern wbbool check_dworms;

#define DWORM_PREF_SIZE (sizeof(uns16)+sizeof(uns16))
#define DWORM2_PREF_SIZE (2*sizeof(uns16)+sizeof(uns32))
#define SMALL_BLOCKS_PER_BIG_BLOCK 1024

struct dworm2_beg;

class t_dworm_access
{ dworm2_beg * bg;
  tfcbnum bg_fcbn; // valid iff opened
  unsigned piece_size;
  BOOL opened,  // bg, bg2 fixed, implies is_null==FALSE
       is_null, // piece not allocated, implies opened==FALSE
       is_error;  // bad data, masking, implies opened==FALSE
  unsigned big_blockcnt;    // !=0 iff bg->state==DWORM2_MARK (AXIOM)
  unsigned small_blockcnt;
  tblocknum * big_blocks;   // defined only when big_blockcnt!=0
  tblocknum * small_blocks;
  unsigned elemlen, perblock;
  t_translog * translog;
  cdp_t fix_cdp;
 public:
  t_dworm_access(cdp_t cdp, const tpiece * pc, unsigned elemlenIn, BOOL write_access, t_translog * translogIn);
  ~t_dworm_access(void);
  BOOL is_open(void) { return opened || is_null || is_error; }  // is_error says there is an error buf I'm masking it
  BOOL null(void)    { return is_null; }
  BOOL error(void)   { return is_error; }
  tfcbnum take_fcbn(void);
  tptr dirdata_pos(void)
    { return (tptr)(small_blocks+small_blockcnt); }
  unsigned dirdata_maxsize(void) const
    { return piece_size - (big_blockcnt ? DWORM2_PREF_SIZE+sizeof(tblocknum)*(small_blockcnt+big_blockcnt) : 
                                          DWORM_PREF_SIZE +sizeof(tblocknum)* small_blockcnt                ); }
  void piece_changed(cdp_t cdp, const tpiece * pc);
  tblocknum * get_small_block(unsigned ind);
  tblocknum get_big_block(unsigned ind);
  const char * pointer_rd(cdp_t cdp, unsigned start, tfcbnum * fcbn, unsigned * local);
        char * pointer_wr(cdp_t cdp, unsigned start, tfcbnum * fcbn, unsigned * local);
  BOOL read(cdp_t cdp, unsigned firstel, unsigned elemcnt, tptr data);
  BOOL expand(cdp_t cdp, tpiece * pc, unsigned elemcnt);
  BOOL shrink(cdp_t cdp, tpiece * pc, uns32 elemcnt);
  BOOL write(cdp_t cdp, unsigned firstel, unsigned elemcnt, const char * data, const tpiece * pc);
  BOOL search(cdp_t cdp, t_mult_size start, t_mult_size size, int flags, wbcharset_t charset, int pattsize, char * pattern, t_varcol_size * pos);
  uns64 allocated_space(void) const
    { return piece_size ? (uns64)SMALL_BLOCKS_PER_BIG_BLOCK*BLOCKSIZE*big_blockcnt + BLOCKSIZE*small_blockcnt + dirdata_maxsize() : 0; }
};

BOOL hp_read(cdp_t cdp, const tpiece * pc, uns16 elemlen, uns32 firstel, uns32 elemcnt, tptr data);
BOOL hp_write(cdp_t cdp, tpiece * pc, uns16 elemlen, uns32 firstel, unsigned elemcnt, const char * data, t_translog * translog);
BOOL hp_shrink_len(cdp_t cdp, tpiece * pc, uns16 elemlen, uns32 elemcnt, t_translog * translog);
tptr         hp_locate_write(cdp_t cdp, tpiece * pc, uns16 elemlen, uns32 index, tfcbnum * pfcbn, t_translog * translog);
const char * hp_locate_read (cdp_t cdp, const tpiece * pc, uns16 elemlen, uns32 index, tfcbnum * pfcbn);
BOOL hp_search(cdp_t cdp, const tpiece * pc, t_mult_size start, t_mult_size size, int flags, wbcharset_t charset, int pattsize, char * pattern, t_varcol_size * pos);
BOOL hp_free(cdp_t cdp, tpiece * pc, t_translog * translog);
BOOL hp_copy(cdp_t cdp, const tpiece * pcfrom, tpiece * pcto, uns32 size, ttablenum tb, trecnum recnum, tattrib attrib, uns16 index, t_translog * translog);
void hp_make_null_bg(void * buf);
sig64 get_alloc_len(cdp_t cdp, tpiece * pc);

void release_piece(cdp_t cdp, tpiece * pc, bool can_dealloc_direct);
void locked_release_piece(cdp_t cdp, tpiece * pc, bool can_dealloc_direct);
BOOL init_pieces(cdp_t cdp, BOOL load_stored_pieces);
void save_pieces(cdp_t cdp); /* save free pieces to the system heaps */
void reload_pieces(cdp_t cdp); /* load piece worms by free pieces stored in system */
int  mark_free_pieces(cdp_t cdp, bool repair);
void translate_free_pieces(cdp_t cdp);
void mark_core_pieces(void);
void trace_free_pieces(cdp_t cdp, const char * label);


