// sort.cpp - the new sort for big tables
// Combines quicksort and mergesort
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "sdefs.h"
#include "scomp.h"
#include "basic.h"
#include "frame.h"
#include "kurzor.h"
#include "dispatch.h"
#include "table.h"
#include "dheap.h"
#include "curcon.h"
#pragma hdrstop

/* Algoritmus: 
Projdu cely vstup a opakovane:
   - naplnim bigblock klici,
   - setridim je quicksortem
   - vysypu je do retezce
   - pripadne spojim retezce
Retezce se spojuji, kdyz jich je bbframes, abych mohl mit v bigblocku po jednom bloku z kazdeho spojovaneho retezce a jeden vystupni retezec.
Retezce eviduji hierarchicky podle urovne spojovani. Kdyz na jedne urovni mam plny pocet retezcu, vytvorim z nich retezec vyssi urovne.

Po projdeni vstupu se provede zaverecne spojovani. Spojuje se od spodnich urovni k vyssim, vysledek se zapise na nejblizsi vyssi uroven s volnym mistem.
Optimalizuje se tak, ze pokud pocet retezcu na vsech urovnich je nejvyse bbframes, spoji se vse najednou.
Posledni spojovani nevytvari vystupni retezec, ale rovnou pise vysledek na vystup, pomoci funkce write_result.
*/

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

unsigned used_sort_space = 0;

class t_sorter
{ enum { MAX_MERGED = 300 };
 protected:
  cdp_t cdp;
  unsigned  bbrecmax;          // max. count of sorting nodes in bigblock
  inline tptr node(unsigned index)
    { return bigblock+keysize*index; }
 private:
  tptr      bigblock;          // sortspace pointer
  void *    bigblock_handle;   // !=0 iff bigblock allocated
  unsigned  bbsize;            // bytes allocated for this sorting, valid iff bigblock_handle
  unsigned  bbframes;          // >=3, number of frames in bigblock - 1, max. number of merged chains

  tptr medval;       // space for a single node value

  t_chain_matrix chains;
  tblocknum merge_chains(unsigned level, BOOL final_merge);
  BOOL optimize_merge(unsigned level);

 protected:
  tptr     keyspace;
  dd_index * orderindx;     // sorting index
  unsigned keysize;         // node size, key with all record numbers
  unsigned recnums_offset;  // offset of record numbers in the key
  unsigned nodes_per_block; // max. number of nodes which can be stored in a BLOCKSIZE block
  trecnum recnum, out_recnum;

 private:
  void quick_sort_bigblock(int start, int stop);
  tblocknum bigblock_to_chain(unsigned nodes_in_bigblock);

 protected:
  t_sorter(cdp_t cdpIn, dd_index * orderindxIn, int tabcnt)
  {// init owners: 
    bigblock_handle=0;  medval=NULL;  keyspace=NULL;
   // copy input values:
    cdp=cdpIn;
    orderindx=orderindxIn;
   // calculate auxiliary values:
    keysize=orderindx->keysize + (tabcnt-1) * sizeof(trecnum);
    recnums_offset = keysize - tabcnt * sizeof(trecnum);
    nodes_per_block = (BLOCKSIZE-sizeof(t_chain_link)) / keysize;
   // other init:
    recnum=out_recnum=0;
  }
  virtual ~t_sorter(void)
  { if (bigblock_handle)
    { aligned_free(bigblock_handle);
      used_sort_space -= bbsize / 1024;
    }
    corefree(medval);  corefree(keyspace);
  }

  virtual void fill_bigblock(unsigned & nodes_in_bigblock) = 0;
  virtual void write_result(tptr result_block, unsigned count) = 0;

 public:
  BOOL alloc_Bigblock(cdp_t cdp, trecnum rec_estim);
  void sort_it(void);
};

BOOL t_sorter::alloc_Bigblock(cdp_t cdp, trecnum rec_estim)
// Allocates the main memory block for sorting 
// Returns TRUE if cannot alocate. 
// keysize must be defined on entry.
// "used_sort_space" is not protected by a critical section, errors are almost harmles.
{ unsigned size_llimit, estimated_space;
 // alocate working space for 2 nodes:
  medval  =(tptr)corealloc(keysize, 44);
  keyspace=(tptr)corealloc(keysize, 44);
  if (!medval || !keyspace)
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
 // calculate the bigblock size:
  if ((keysize > 0xffff || rec_estim > 0xffff) && (keysize > 0xffffffL || rec_estim > 0xff) && (keysize > 0xff || rec_estim > 0xffffffL)
      || !rec_estim)
		estimated_space=20000000; // 30 MB internal limit, used only when prifile limits are bigger
	else estimated_space = keysize*rec_estim;
  // allocation upper limit: minimum from sort space defined in the profile, rest of the total sort space, estimated necessary space:
  unsigned allocable_space = the_sp.total_sort_space.val() > used_sort_space ? 1024L*(the_sp.total_sort_space.val() - used_sort_space) : 0;
  unsigned profile_space   = 1024L * the_sp.profile_bigblock_size.val();
  bbsize = estimated_space;
  if (bbsize > allocable_space) bbsize = allocable_space;
  if (bbsize > profile_space)   bbsize = profile_space;
  // alocation lower limit: 4*BLOCKSIZE nesessary for mergesort (3*BLOCKSIZE is the real minimum, but it is horribly inefficiet)
  size_llimit=4*BLOCKSIZE;
  if (bbsize < size_llimit) bbsize = size_llimit;
 // allocating the bigblock:
	do
	{ bigblock_handle=aligned_alloc(bbsize, &bigblock);
		if (bigblock_handle) break;
		bbsize=bbsize/4*3;
		if (bbsize < size_llimit)
		  { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
	} while (TRUE);
 // register the allocated bigblock space:
  used_sort_space += bbsize / 1024;
 // compute space-related koefs:
  bbrecmax=bbsize / keysize;
  bbframes=bbsize / BLOCKSIZE - 1; // >=3
  if (bbframes > chains.MAX_EVID / 2 - 3) bbframes = chains.MAX_EVID / 2 - 3;
  if (bbframes > MAX_MERGED) bbframes = MAX_MERGED;
  chains.init(bbframes);
  return FALSE;
}
////////////////////////////////// quicksort /////////////////////////////////////////////
void t_sorter::quick_sort_bigblock(int start, int stop)
// start < stop supposed
{ int median;
 // find median:
  median=(start+stop)/2;
  if (start+7 < stop) // optimize the median
  { int ma=start+(stop-start+1)/4, mb=start+(stop-start+1)/4*3;
    short ram=(short)cmp_keys(node(ma),     node(median), orderindx);
    short rmb=(short)cmp_keys(node(median), node(mb),     orderindx);
    if (ram<0)
      if (rmb>0) // ma, mb less, take bigger of them
      { short rab=(short)cmp_keys(node(ma),     node(mb),     orderindx);
        if (rab<0) median=mb; else median=ma;
      }
      else
        ;
    else // ram>0
      if (rmb<0) // ma, mb bigger, take less of them
      { short rab=(short)cmp_keys(node(ma),     node(mb),     orderindx);
        if (rab<0) median=ma; else median=mb;
      }
      else
        ;
  }
 // dividing:
  int i=start, j=stop;
  memcpy(medval, node(median), keysize);
  do // INV: <start..i) values <= median value && (j..stop> values >= median value
  { while (i<=stop  && (short)cmp_keys(node(i), medval, orderindx) < 0) i++; 
    while (j>=start && (short)cmp_keys(medval, node(j), orderindx) < 0) j--; 
    if (i<=j)
    { if (i<j)
      { memcpy(keyspace, node(i),  keysize);
        memcpy(node(i),  node(j),  keysize);
        memcpy(node(j),  keyspace, keysize);
      }
      i++;  j--; 
    }
  } while (i<=j);
 // recursion:
  if (start < j) quick_sort_bigblock(start, j);
  if (i < stop)  quick_sort_bigblock(i, stop);
}

// Temporary blocks: every block is released just after being read.
tblocknum t_transactor::save_chain_block(tptr nodes, unsigned bytes, t_chain_link * chl)
{ tblocknum block = get_swap_block();
  ProtectedByCriticalSection cs(&cs_trans);
  DWORD pos=block*BLOCKSIZE, wr;
 	SetFilePointer(ffa.t_handle, pos, NULL, FILE_BEGIN);
  WriteFile(ffa.t_handle, (void*)nodes, bytes, &wr, NULL);
 	SetFilePointer(ffa.t_handle, pos+BLOCKSIZE-sizeof(t_chain_link), NULL, FILE_BEGIN);
  WriteFile(ffa.t_handle, (void*)chl, sizeof(t_chain_link), &wr, NULL);
  return block;
}

void t_transactor::save_a_block(tptr nodes, tblocknum block)
{ 
  ProtectedByCriticalSection cs(&cs_trans);
  DWORD pos=block*BLOCKSIZE, wr;
 	SetFilePointer(ffa.t_handle, pos, NULL, FILE_BEGIN);
  WriteFile(ffa.t_handle, (void*)nodes, BLOCKSIZE, &wr, NULL);
}

void t_transactor::load_chain_block(tptr nodes, tblocknum block)
{ 
  { ProtectedByCriticalSection cs(&cs_trans);
    DWORD pos=block*BLOCKSIZE, rd;
 	  SetFilePointer(ffa.t_handle, pos, NULL, FILE_BEGIN);
    ReadFile(ffa.t_handle, (void*)nodes, BLOCKSIZE, &rd, NULL);
  }
  free_swap_block(block);
}

tblocknum t_sorter::bigblock_to_chain(unsigned nodes_in_bigblock)
// nodes_in_bigblock>0. Creates a chain and returns its head
{ tblocknum current_head=0;
  t_chain_link chl;
  while (nodes_in_bigblock)
  { unsigned current_nodes = nodes_in_bigblock % nodes_per_block; // nodes to be saved in the current block
    if (!current_nodes) current_nodes=nodes_per_block;
    chl.local_nodes=current_nodes;  chl.next_block=current_head;
    current_head=transactor.save_chain_block(node(nodes_in_bigblock-current_nodes), current_nodes*keysize, &chl);
    nodes_in_bigblock-=current_nodes;
  }
  return current_head;
}

void t_sorter::sort_it(void)
{ unsigned level;
 // main cycle: fill, sort, output chain, merge if many chains
  do
  { unsigned nodes_in_bigblock;
    fill_bigblock(nodes_in_bigblock);
    if (!nodes_in_bigblock) break; // all input read
    quick_sort_bigblock(0, nodes_in_bigblock-1);
   // check for direct output of a single bigblock, wihout creating any chain
    if (chains.chain_cnt(0)==0 && nodes_in_bigblock<bbrecmax)
    { write_result(bigblock, nodes_in_bigblock);
      return;
    }
   // creating a chain from the bigblock:
    tblocknum ch = bigblock_to_chain(nodes_in_bigblock);
   // store the chain in the evid: merge chains if there is no space 
    level = 0; // the level where the chain ch should be stored
    while (chains.chain_cnt(level)==bbframes) // must merge, goes from the top
    { tblocknum next_ch = merge_chains(level, FALSE);
      chains.null_level(level);  chains.add_chain(level, ch);  
      ch=next_ch;  level++;
    }
    chains.add_chain(level, ch);  

  } while (TRUE);
 // merging every level except the last, storing result on upper level:
  for (level=0;  !optimize_merge(level);  level++)
  // INV: all levels below "level" are empty, some level above "level" is non-empty
  { tblocknum next_ch = merge_chains(level, FALSE);  chains.null_level(level);  
   // find a level above "level" which is not full for storing the result chain:
    unsigned result_level=level+1;
    while (chains.chain_cnt(result_level)==bbframes) result_level++;
    chains.add_chain(result_level, next_ch);  
  }
 // final merging and output:
  merge_chains(level, TRUE);
 // report final state:
  report_total(cdp, recnum, out_recnum);
}

//////////////////////////////////////////////////////////////////////////////////////////
BOOL t_sorter::optimize_merge(unsigned level)
// If total count of chains is <= bbframes then all chains can be merged in a single step.
// The proc puts all of them on the level "level" and returns TRUE (the final merging will follow)
// Otherwise returns FALSE.
{ unsigned total_chains = 0, level_chains, evid_level;
  for (evid_level=level;  TRUE;  evid_level++)
  { level_chains=chains.chain_cnt(evid_level);
    if (!level_chains) break;
    total_chains+=level_chains;
  }
  if (total_chains <= bbframes) // can merge all in a single step
  { for (evid_level=level+1;  TRUE;  evid_level++)
      if (!chains.chain_cnt(evid_level)) break;
      else // copy chains from upper evid_level to level:
        for (unsigned ch=0;  ch<chains.chain_cnt(evid_level);  ch++)
          chains.add_chain(level, chains.get_chain(evid_level, ch));
    return TRUE;
  }
  return FALSE;
}

tblocknum t_sorter::merge_chains(unsigned level, BOOL final_merge)
// merges all the chains on level "level".
{ struct t_toplist_entry
  { unsigned blocknum;
    unsigned pos_in_block;
  } toplist[MAX_MERGED]; // list of references to top nodes of the merged strings, sorted from MAX to MIN
  tptr merged_blocks[MAX_MERGED];  
  unsigned i;
 // initialise toplist:
  unsigned top_list_size=0; // number of the nen-exhausted merged chains (decreases during the process)
  for (i=0;  i<chains.chain_cnt(level);  i++)
  { merged_blocks[i]=bigblock+i*BLOCKSIZE;
    transactor.load_chain_block(merged_blocks[i], chains.get_chain(level, i));
    unsigned j;
    for (j=0;  j<top_list_size;  j++)
      if ((short)cmp_keys(merged_blocks[toplist[j].blocknum], merged_blocks[i], orderindx) < 0) break;
    // insert the i-th block into the [j] position in the toplist
    memmov(&toplist[j+1], &toplist[j], (top_list_size-j)*sizeof(t_toplist_entry));
    toplist[j].blocknum=i;  toplist[j].pos_in_block=0;  
    top_list_size++;
  }
 // init the result chain:
  tblocknum result_head, current_result_blocknum;  // the head and the current block of the 
  tptr result_block, result_pos;  // result block and the current position pointer
  if (final_merge)
    current_result_blocknum = 0; // not used
  else
    current_result_blocknum = transactor.get_swap_block();
  result_head = current_result_blocknum;
  unsigned result_cnt=0;                           // number of nodes in the current block of the result chain
  result_block=result_pos=bigblock+bbframes*BLOCKSIZE; // INV: result_pos==result_block+keysize*result_cnt
 // merge it:
  t_chain_link * chl;
  while (top_list_size) // while any of the input chains is non-empty
  {// make space in the result block:
    if (result_cnt==nodes_per_block)
    { if (final_merge)
        write_result(result_block, result_cnt);
      else
      { tblocknum next_result_blocknum = transactor.get_swap_block(); // allocate the next result block (its numbers willbe stored in the current block
        chl = (t_chain_link *)(result_block+BLOCKSIZE-sizeof(t_chain_link));
        chl->next_block=next_result_blocknum;  chl->local_nodes=result_cnt;
        transactor.save_a_block(result_block, current_result_blocknum);
        current_result_blocknum=next_result_blocknum;
      }
      result_pos=result_block;  result_cnt=0;
    }
   // copy a node to the result
    t_toplist_entry * top = &toplist[top_list_size-1];
    unsigned source_block=top->blocknum;
    memcpy(result_pos, merged_blocks[source_block]+keysize*top->pos_in_block, keysize);
    result_pos+=keysize;  result_cnt++;
   // get new source node from the same chain, if exists:
    chl = (t_chain_link *)(merged_blocks[source_block]+BLOCKSIZE-sizeof(t_chain_link));
    if (++top->pos_in_block >= chl->local_nodes)
      if (chl->next_block)
      { transactor.load_chain_block(merged_blocks[source_block], chl->next_block);
        top->pos_in_block=0;
      }
      else
      { top_list_size--;
        continue;  // no sorting in the toplist
      }
   // move the new source node to the correct place:
    for (i=0;  i<top_list_size-1;  i++)
    { tptr key = merged_blocks[top->blocknum]+top->pos_in_block*keysize;
      if ((short)cmp_keys(merged_blocks[toplist[i].blocknum]+toplist[i].pos_in_block*keysize, key, orderindx) < 0) 
      { t_toplist_entry top_copy=*top;
        memmov(&toplist[i+1], &toplist[i], (top_list_size-i-1)*sizeof(t_toplist_entry));
        toplist[i]=top_copy;
        break;
      }
    }
  }
 // write the last result block:
  if (final_merge)
    write_result(result_block, result_cnt);
  else
  { chl = (t_chain_link *)(result_block+BLOCKSIZE-sizeof(t_chain_link));
    chl->next_block=0;  chl->local_nodes=result_cnt;
    transactor.save_a_block(result_block, current_result_blocknum);
  }
  return result_head;
}

//////////////////////////////////////// sorter subclasses //////////////////////////////////////
// t_pmat_sorter: Stores sorted records to pmat

class t_pmat_sorter : public t_sorter
{ trecnum limit_offset, limit_count;
 protected:
  t_mater_ptrs * out_pmat;    
  t_optim * opt;
  virtual void fill_bigblock(unsigned & nodes_in_bigblock)=0;
  virtual void write_result(tptr result_block, unsigned count);

 public:
  bool distinct;
  t_pmat_sorter(cdp_t cdpIn, dd_index * orderindxIn, int tabcnt, t_mater_ptrs * out_pmatIn, t_optim * optIn, trecnum limit_offsetIn, trecnum limit_countIn) 
  : t_sorter(cdpIn, orderindxIn, tabcnt), limit_offset(limit_offsetIn), limit_count(limit_countIn)
    { out_pmat=out_pmatIn;  opt=optIn;  distinct=false; }
  virtual ~t_pmat_sorter(void)    {  }
};

void t_pmat_sorter::write_result(tptr result_block, unsigned count)
{// skipping records on the beginning ([limit_offset] more records to be skipped):
  if (limit_offset >= count)
    { limit_offset -= count;  return; }
  else
    { count -= limit_offset;
      result_block += keysize * limit_offset;
      limit_offset = 0;
    }
 // limiting the count of produced records ([limit_count] more records can be output):
  if (limit_count < count)
  { count = limit_count;
    limit_count = 0;
  }
  else if (limit_count!=NORECNUM) limit_count -= count;
 // outputting:
  while (count--)
  { if (distinct)
    { if (out_pmat->rec_cnt) // previous key is in keyval
        if (HIWORD(cmp_keys(keyspace, result_block, orderindx)))  // save key value as in the previous
          goto duplicate;  // do not insert
    }
    if (!out_pmat->put_recnums(cdp, (trecnum*)(result_block+recnums_offset)))
      return;
    out_recnum++;
    report_step_reversed(cdp, recnum, out_recnum);
   duplicate:
    if (distinct)
      memcpy(keyspace, result_block, keysize);
    result_block+=keysize;
  }
}

// All pmat_sorters: input data are obtained from p_get_next_record()

class t_pmat_sorter1 : public t_pmat_sorter
{ virtual void fill_bigblock(unsigned & nodes_in_bigblock);
 public:
  t_pmat_sorter1(cdp_t cdpIn, dd_index * orderindxIn, int tabcnt, t_mater_ptrs * out_pmatIn, t_optim * optIn, trecnum limit_offsetIn, trecnum limit_countIn) 
   : t_pmat_sorter(cdpIn, orderindxIn, tabcnt, out_pmatIn, optIn, limit_offsetIn, limit_countIn)    {  }
  virtual ~t_pmat_sorter1(void)    {  }
  inline BOOL alloc_Bigblock(cdp_t cdp, trecnum rec_estim)  { return t_pmat_sorter::alloc_Bigblock(cdp, rec_estim); }
  inline void sort_it(void)                                 {        t_pmat_sorter::sort_it(); }
};

class t_pmat_sorter2 : public t_pmat_sorter
{ t_query_expr * qe;  table_descr * mattabdescr;
  virtual void fill_bigblock(unsigned & nodes_in_bigblock);
 public:
  t_pmat_sorter2(cdp_t cdpIn, dd_index * orderindxIn, int tabcnt, t_mater_ptrs * out_pmatIn, t_optim * optIn, trecnum limit_offsetIn, trecnum limit_countIn, t_query_expr * qeIn, table_descr * mattabdescrIn) 
   : t_pmat_sorter(cdpIn, orderindxIn, tabcnt, out_pmatIn, optIn, limit_offsetIn, limit_countIn)
    { qe=qeIn;  mattabdescr=mattabdescrIn; }
  virtual ~t_pmat_sorter2(void)    {  }
  inline BOOL alloc_Bigblock(cdp_t cdp, trecnum rec_estim)  { return t_pmat_sorter::alloc_Bigblock(cdp, rec_estim); }
  inline void sort_it(void)                                 {        t_pmat_sorter::sort_it(); }
};

class t_pmat_sorter3 : public t_pmat_sorter
{ db_kontext * kont;
  virtual void fill_bigblock(unsigned & nodes_in_bigblock);
 public:
  t_pmat_sorter3(cdp_t cdpIn, dd_index * orderindxIn, int tabcnt, t_mater_ptrs * out_pmatIn, t_optim * optIn, trecnum limit_offsetIn, trecnum limit_countIn, db_kontext * kontIn) 
   : t_pmat_sorter(cdpIn, orderindxIn, tabcnt, out_pmatIn, optIn, limit_offsetIn, limit_countIn)
    { kont=kontIn; }
  virtual ~t_pmat_sorter3(void)    {  }
  inline BOOL alloc_Bigblock(cdp_t cdp, trecnum rec_estim)  { return t_pmat_sorter::alloc_Bigblock(cdp, rec_estim); }
  inline void sort_it(void)                                 {        t_pmat_sorter::sort_it(); }
};

void t_pmat_sorter1::fill_bigblock(unsigned & nodes_in_bigblock)
{ unsigned count=0;
  tptr anode = node(0);     /* node address */
  while (count < bbrecmax && !cdp->break_request && (opt->*opt->p_get_next_record)(cdp))
  { if (compute_key(cdp, NULL, NORECNUM, orderindx, anode, true))
    { 
#if SQL3
      int ordnum = 0;
      out_pmat->top_qe->get_position_indep((trecnum*)(anode+recnums_offset), ordnum);
#else
      out_pmat->top_qe->get_position((trecnum*)(anode+recnums_offset));
#endif
      count++;  anode+=keysize;  recnum++;
      report_step(cdp, recnum, out_recnum);
    }
  }
  nodes_in_bigblock=count;
}

void t_pmat_sorter2::fill_bigblock(unsigned & nodes_in_bigblock)
{ unsigned count=0;
  tptr anode = node(0);     /* node address */
  while (count < bbrecmax && !cdp->break_request && (opt->*opt->p_get_next_record)(cdp))
  {// materialization, if required:
    trecnum tab_recnum=tb_new(cdp, mattabdescr, TRUE, NULL);
    if (tab_recnum==NORECNUM) opt->error=TRUE;
    else
    { for (int i=1;  i<qe->kont.elems.count();  i++)
#if SQL3
        if (write_elem(cdp, qe, i, mattabdescr, tab_recnum, i))
#else
        if (write_elem(cdp, ((t_qe_ident*)qe)->op, i, mattabdescr, tab_recnum, i))
#endif
          opt->error=TRUE;
     // sorting:
      if (compute_key(cdp, NULL, NORECNUM, orderindx, anode, true))
      { *(trecnum*)(anode+recnums_offset) = tab_recnum;
        count++;  anode+=keysize;  recnum++;
        report_step(cdp, recnum, out_recnum);
      }
    }
  }
  nodes_in_bigblock=count;
}

void t_pmat_sorter3::fill_bigblock(unsigned & nodes_in_bigblock)
{ unsigned count=0;
  tptr anode = node(0);     /* node address */
  while (count < bbrecmax && !cdp->break_request && (opt->*opt->p_get_next_record)(cdp))
  { if (compute_key(cdp, NULL, NORECNUM, orderindx, anode, true))
    { *(trecnum*)(anode+recnums_offset) = kont->position;
      count++;  anode+=keysize;  recnum++;
      report_step(cdp, recnum, out_recnum);
    }
  }
  nodes_in_bigblock=count;
}

BOOL create_curs_sort(cdp_t cdp, dd_index * indx, t_query_expr * qe, t_optim * opt, trecnum limit_count, trecnum limit_offset, bool distinct)
// Sorts records whose numbers can be obtained by get_position().
// qe is supposed to contain pointer materialization.
// (opt->*opt->p_reset)(cdp);  called outside in SQL3.
{ 
#if SQL3
  t_pmat_sorter1 sr(cdp, indx, qe->kont.p_mat->kont_cnt, qe->kont.p_mat, opt, limit_offset, limit_count);  
#else
  t_pmat_sorter1 sr(cdp, indx, qe->kont.mat->kont_cnt, (t_mater_ptrs*)qe->kont.mat, opt, limit_offset, limit_count);  
#endif
  if (sr.alloc_Bigblock(cdp, 0)) return TRUE;
#if !SQL3
  (opt->*opt->p_reset)(cdp);
#endif
  sr.distinct=distinct;
  sr.sort_it();
  return FALSE;
}

BOOL curs_mater_sort(cdp_t cdp, dd_index * indx, t_query_expr * qe, t_optim * opt, ttablenum mattab, trecnum limit_count, trecnum limit_offset)
// Sorts records and creates a new (unsorted) table materialization of them.
// qe is supposed to contain pointer materialization.
{ 
#if SQL3
  t_pmat_sorter2 sr(cdp, indx, 1, qe->kont.p_mat, opt, limit_offset, limit_count, qe, tables[mattab]);  
#else
  t_pmat_sorter2 sr(cdp, indx, 1, (t_mater_ptrs*)qe->kont.mat, opt, limit_offset, limit_count, qe, tables[mattab]);  
#endif
  if (sr.alloc_Bigblock(cdp, 0)) return TRUE;
  //(opt->*opt->p_reset)(cdp); -- called outside
  sr.sort_it();
  return FALSE;
}

BOOL sort_cursor(cdp_t cdp, dd_index * indx, t_optim * opt, t_mater_ptrs * pmat, trecnum limit, db_kontext * kont, trecnum limit_count, trecnum limit_offset)
// Sorts records whose numbers can be found in [kont].
{ t_pmat_sorter3 sr(cdp, indx, 1, pmat, opt, limit_offset, limit_count, kont);  
  if (sr.alloc_Bigblock(cdp, limit)) return TRUE;
  sr.sort_it();
  return FALSE;
}

/////////////////////////////////////// btree sorter //////////////////////////////////////////////
class t_btree_sorter : public t_sorter
{ int btree_sp;
  t_btree_stack btree_stack[BSTACK_LEN];
  table_descr * tbdf;

 public:
  t_btree_sorter(cdp_t cdpIn, dd_index * orderindxIn, table_descr * tbdfIn) : t_sorter(cdpIn, orderindxIn, 1)
  // tbdfIn has to be secure but need not to be in tables[].
    { btree_sp=-1;  tbdf = tbdfIn; }
  void fill_bigblock(unsigned & nodes_in_bigblock);
  void write_result(tptr result_block, unsigned count);
  tblocknum close(void)
    { return close_creating(cdp, btree_stack, btree_sp, table_translog(cdp, tbdf)); }
};

void t_btree_sorter::fill_bigblock(unsigned & nodes_in_bigblock)
{ unsigned count=0;
  tptr anode = node(0);     /* node address */
  while (count < bbrecmax && !cdp->break_request && recnum < tbdf->Recnum())
  { report_step(cdp, recnum, out_recnum);
    if (!table_record_state(cdp, tbdf, recnum))
    { if (compute_key(cdp, tbdf, recnum, orderindx, anode, false))
      { *(trecnum*)(anode+recnums_offset) = recnum;
        if (orderindx->has_nulls || !is_null_key(anode, orderindx))
          { count++;  anode+=keysize; }
      }
    }
    recnum++;
  }
  nodes_in_bigblock=count;
}

void t_btree_sorter::write_result(tptr result_block, unsigned count)
{ tptr last_key_on_output=NULL;
  t_translog * translog = table_translog(cdp, tbdf);
  while (count--)
  {/* insert into the tree unless distinct && key is the same as the last one */
    if (!DISTINCT(orderindx) || !last_key_on_output ||
        !HIWORD(cmp_keys(last_key_on_output, result_block, orderindx)))
    { add_ordered(cdp, result_block, orderindx->keysize, btree_stack, &btree_sp, translog);
      out_recnum++;
      report_step_reversed(cdp, recnum, out_recnum);
    }
    else 
    { char buf[OBJ_NAME_LEN+45];
      sprintf(buf, "%s(%u,%u)", tbdf->selfname, *(trecnum*)(result_block+orderindx->keysize-sizeof(trecnum)), *(trecnum*)(last_key_on_output+orderindx->keysize-sizeof(trecnum)));
      buf[OBJ_NAME_LEN]=0; // must trim the length!
      SET_ERR_MSG(cdp, buf);
      request_error(cdp, KEY_DUPLICITY);  
    }
    last_key_on_output=result_block;
    result_block+=keysize;
  }
}

tblocknum create_btree(cdp_t cdp, table_descr * tbdf, dd_index * indx)
// tbdf is supposed to be secure
{ t_btree_sorter sr(cdp, indx, tbdf);  
  if (sr.alloc_Bigblock(cdp, tbdf->Recnum())) return 0;
 // sort the table:
  sr.sort_it();
  return sr.close();
}

