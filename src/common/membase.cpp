// membase.cpp - Memory management base module common for client and server
/* Calls external functions: fatal_error, memory_not_extended. */
// Uses define: STD_CORE_ALLOC
/*
 *		 Sprava pameti
 *
 * Zmeny s cilem urychlit dealokaci: dcmark by mel davat starou hodnotu po
 * XORovani 0x80. Prvni bit nyni uschovava informaci o tom, je-li predchozi
 * blok volny (0x80) nebo ne (0x00). Pokud je dany blok volny, je v jeho
 * poslednich 4 bajtech pointer na jeho zacatek. Krome toho je rozsiren i o
 * pointer na predchozi blok (viz strukturu memblock). Toto vse se vejde i do
 * nejmensiho volneho bloku (=16 bajtu), ale uz nic vic...
 *
 * Existuje systemovy memblock, ktery je ekvivalentem NULL pro pouzivani v
 * cyklech, plus zacykluje list volnych pametovych bloku.
 *
 * Zabira se 16 bajtu na alokovany part navic, z toho je 12 bajtu volnych.
 * Mozna by se do nich mohly dat nejake smysluplne informace...
 *
 * Bloky nemusi byt razeny jak jdou za sebou v pameti.
 *
 * Volne podle Knuth, TAOCP, kap. 2.5, system s tagy.
 
*/

//#define HEAP_TEST  // activates very extensive integrity tests
#ifdef _DEBUG
#define MEM_CHECK    // activates debugging tests
#endif
//////////////////////////////// alloaction of aligned memory from the system /////////////////
#define ALIGNMENT sizeof(void*)    // every allocated memory block starts on address which is a multiple of ALIGNMENT

static void * special_aligned_alloc(size_t size, void ** ptr, int off)
// Allocates memory block from the system, returns pointer to it 
//   and stores aligned pointer to *ptr such that (*ptr+off) % ALIGNMENT == 0.
// Returns NULL if cannot allocate.
{ void * alloc;
  *ptr=NULL;
 // allocate with a reserve for alignment:
#ifdef WINS
  HGLOBAL handle=GlobalAlloc(GMEM_MOVEABLE, size+(ALIGNMENT-1));
  if (!handle) return NULL; 
  alloc = GlobalLock(handle);
  if (!alloc) { GlobalFree(handle);  return NULL; }
#else
  alloc = malloc(size+(ALIGNMENT-1));
  if (!alloc) {   return NULL; }
#endif
 // store the aligned beginning of the block to *ptr:
  size_t offset = ((size_t)alloc + off) % ALIGNMENT;
  if (offset) offset = ALIGNMENT - offset;
  *ptr = (void*)((size_t)alloc + offset);
  return alloc;
}
CFNC DllKernel void * WINAPI aligned_alloc(size_t size, char ** ptr)
// Allocates memory block from the system, returns pointer to it 
//   and stores aligned pointer to *ptr. 
{ return special_aligned_alloc(size, (void**)ptr, 0); }

CFNC DllKernel void WINAPI aligned_free(void * alloc)
// Deallocates memory ablock allocated from the system, using its (unaligned) pointer.
#ifdef WINS
{ HGLOBAL handle = GlobalPtrHandle(alloc);
  GlobalUnlock(handle);  GlobalFree(handle); 
}
#else
{ ::free(alloc); }
#endif

//////////////////     // The list of free memory blocks /////////////////////
struct memblock  // beginning of each memory block, update MEM_PREFIX_SIZE if changed!
{ uns8  prevfree:1; // is  previous block free?
  uns8  dcmark:7;   // magic number DCMARK (DD_MARK when searching for leaks)
  uns8  owner;
  uns16 size;       // total size of the block in 16-byte units
 // if memory block is in use, here starts the user part
  memblock * next; // pointer to the next block in the list of free blocks (used only in free blocks!!)
  memblock * prev; // previous free block, usage ditto 
};

#define BLOCK_END_PTR(block) ((tptr)(block)+16*(unsigned)(block)->size)
#define MEM_PREFIX_SIZE (offsetof(struct memblock, next)) // size of the system part of the memory block
#define DCMARK   0x5c
#define DD_MARK  0x5d
#define BIG_MARK 0x5e  // alloated directly from the system

#define BLOCK_FROM_PTR(x) ((memblock*)(((char *)x)-MEM_PREFIX_SIZE)) // converts common pointer to block ptr
#define NEXT_BLOCK(bl)    ((memblock*)((tptr)bl+16*bl->size)) // the next memory block in the linked list
#define USABLE_BLOCK_SIZE(bl) 16*(unsigned)bl->size-MEM_PREFIX_SIZE // bytes in a block available to the user

static memblock sys_memblock=
{ 0, /* whatever */
	DCMARK,
	0xff, /* random owner */
	1, /* small size */
	&sys_memblock, /* next is self at the beginning */
	&sys_memblock  /* ditto  for prev */
};
	
static memblock * const freelist = &sys_memblock;  // list of all free memory blocks

inline static void unchain(memblock *bl)
{
    bl->next->prev=bl->prev;
    bl->prev->next=bl->next; 
}

inline static void chain(memblock * bl)
{
    bl->next=freelist->next;
    freelist->next=bl;
    bl->next->prev=bl;
    bl->prev=freelist;
}

//////////////////////////////// memory management envelope ////////////////////////////////////////
CRITICAL_SECTION cs_memory;  // DDLK1: Inside this CS no other CS is entered and no locks used (leak_test is an exception)
// Exists iff [allocated_core_blocks]!=0. Created and destroyed here.

#define MAX_HEAP_PARTS  500    // max. count of blocks allocated from the system
static memblock * origlist  [MAX_HEAP_PARTS],  // original blocks allocated from the operating system
                * heap_limit[MAX_HEAP_PARTS];  // ptrs after ends of the blocks from "origlist"
static void     * orig_blocks[MAX_HEAP_PARTS];     // original blocks allocated from the operating system, not aligned
static int    allocated_core_blocks = 0;
static size_t total_allocated_size = 0;
static bool   memory_allocation_enabled = false;  // disabled when the system is not initialized and on fatal memory error

/* Got new part of memory, need to create a allocated cells system on it.
 * That is: assign it (all but final padding) as a free block, with the
 * previous one (in memory) not free, and include it to the free block chain.
 */
static inline void init_new_block(memblock *bl, size_t size)
{
    size/=16;
    bl->prevfree=0;
    bl->dcmark=DCMARK;
    bl->owner=0;
    bl->size=(uns16)size-1; /* padding for prevfree needed */
    origlist  [allocated_core_blocks]=bl;
    heap_limit[allocated_core_blocks]=(memblock*)BLOCK_END_PTR(bl);
    memblock *padd=(memblock *)BLOCK_END_PTR(bl);
    padd->prevfree=1;
    padd->dcmark=DCMARK;
    padd->owner=0xff; /* Dont let it be joined */
    padd->size=1;
    *(((memblock **)padd)-1)=bl;
    chain(bl);
}

CFNC DllKernel BOOL WINAPI core_init(int num)
// Allocates "num" 64KB blocks (when positive) or extends the allocation by "-num" (when negative)
// core_init(0) allocates the initial amount of memory
{ size_t size;
 // find the size of the new memory block:
  if (!allocated_core_blocks)
    InitializeCriticalSection(&cs_memory);
  if (num<0)  // adding (-num) 
    size = -num * 0x10000;
  else        // increasing to (num) size
  { if (num==0) num=STD_CORE_ALLOC;  // standard initial allocation
    size = num * 0x10000;
    if (size <= total_allocated_size) return FALSE;
  }
 // allocate it and add to the list:
  bool memory_extended = false;
  { ProtectedByCriticalSection cs(&cs_memory);
    while (size>1000 && allocated_core_blocks < MAX_HEAP_PARTS)
    { size_t part_size = size;
     // a memory block must not be bigger than 0xfffe0 (size if 16-bit units is stored in 16-bit integer):
      if (part_size>0xfffe0) part_size=0xfffe0;
      memblock * bl;
      orig_blocks[allocated_core_blocks]=special_aligned_alloc(part_size, (void**)&bl, sizeof(memblock));
      if (!orig_blocks[allocated_core_blocks]) 
        break;
	    init_new_block(bl, part_size);
	    allocated_core_blocks++;  // must be after init_new_block()
	    total_allocated_size += part_size;  size -= part_size;
	    memory_extended=true;
	  }  
  }
  memory_allocation_enabled=true;
  return memory_extended;
}

CFNC DllKernel void WINAPI core_disable(void)
{
  memory_allocation_enabled=false;
}

CFNC DllKernel void WINAPI core_release(void)
// Returns all memory of the memory management system to the operating system
{ int i;
  memory_allocation_enabled=false;
  for (i=0;  i<allocated_core_blocks;  i++)
    if (orig_blocks[i])
      aligned_free(orig_blocks[i]);
  if (allocated_core_blocks)
    DeleteCriticalSection(&cs_memory);
  allocated_core_blocks=0;  total_allocated_size=0;
  freelist->next=freelist->prev=freelist;  // freelist points always to sys_memblock
}

void fatal_memory_error(const char * msg)
{
  memory_allocation_enabled=false;  // use the system memory management when own management is corrupt
  fatal_error(NULL, msg);
}

#if WBVERS<=810
CFNC DllKernel BOOL WINAPI is_core_management_open(void)
{ return memory_allocation_enabled; }
#endif

///////////////////////////////////// allocating and deallocating ////////////////////////////
#define INTERNAL_ALLOCATION_LIMIT 150000 // allocations above this limit taken directly from the operating system

CFNC DllKernel void WINAPI corefree(const void * x)
// Releases the memory block [x]. 
// Does nothing if called with x==NULL. 
// Does nothing if called when memory management is not initialised or has been closed.
{ 
  if (x==NULL) return;
  memblock * fr = BLOCK_FROM_PTR(x);
  if (fr->dcmark==BIG_MARK) // block allocated directly from the system
  {
#ifdef MEM_CHECK
    if (((tptr)fr)[(fr->size<<14)-1]!=0x6e)
      { fatal_memory_error("Fatal memory management error 2b");  return; }
#endif
    free(fr);  
    return; 
  }
  if (!memory_allocation_enabled) return;  // called in the destructor of a static class like dynar or after a fatal error
  if (!fr->owner || fr->dcmark != DCMARK || !fr->size)
    { fatal_memory_error("Fatal memory management error 1");  return; }
#ifdef HEAP_TEST
  heap_test();
#endif
#ifdef MEM_CHECK
  if (((tptr)fr)[16*fr->size-1]!=0x6e)
    { fatal_memory_error("Fatal memory management error 2a");  return; }
  memset(&fr->next, 0xe3, USABLE_BLOCK_SIZE(fr));  // debugging support
#endif
  { ProtectedByCriticalSection cs(&cs_memory);
	  if (fr->prevfree)  /* join with previous */
		{ memblock *prev=*(((memblock **)fr)-1); /* previous block in
							memory*/
		  prev->size+=fr->size;	/* cummulate sizes */
		  fr=prev; /* pretend that we want to free prev */
		  unchain(fr);
	  }
	  memblock* next=(memblock *)BLOCK_END_PTR(fr);
	  if (next->owner==0){ /* join */
		  fr->size+=next->size;
		  unchain(next);
		  next=(memblock *)BLOCK_END_PTR(fr);
	  } else /* tell him he follows free block */
		  next->prevfree=1;
	  chain(fr);
	  fr->owner=0;
	  *(((void **)next)-1)=fr; /* pointer at the end of the block */
  } // end of critical section
}

#define EXTENSION_SIZE_64 15

CFNC DllKernel void * WINAPI corealloc(size_t size, uns8 owner)
// Allocates a block of memory with at least the given size.
// Must be able to alloc block bigger than 64KB, e.g. t_log neess them.
{ memblock *prev, *bl;
  if (!size) return NULL;
  if (size<2*sizeof(void*)) size=3*sizeof(void*);  // minimal space for [next] and [prev] pointers and end ptr
#ifdef MEM_CHECK
  size+=MEM_PREFIX_SIZE+1;  // +1 for debugging
#else
  size+=MEM_PREFIX_SIZE;
#endif
  if (size > INTERNAL_ALLOCATION_LIMIT // allocate directly from the system all the big blocks and ...
      || !memory_allocation_enabled)   // when static class constructor alocates memory or after a fatal error
  { size = ((size-1) & 0xffffc000) + 0x4000;  
    bl=(memblock*)malloc(size);
    if (!bl) return NULL;
    bl->owner=owner;  bl->dcmark=BIG_MARK;  bl->size=(uns16)(size>>14);
#ifdef MEM_CHECK
    ((tptr)bl)[size-1]=0x6e;
#endif
    return &(bl->next);
  }
#ifdef HEAP_TEST
  heap_test();
#endif
  size = (size & 0xf) ? size/16+1 : size/16; // allocation granularity
  if (size==1) size=2;
  bool memory_extended = false;
  do // this cycle allows to re-try allocation after extending the memory used by the memory management system
  {
    { ProtectedByCriticalSection cs(&cs_memory);
      prev=NULL;  bl=freelist->next;
      while (bl!=freelist) // cycle on the free list, find 1st block big enough
      { if (bl->dcmark != DCMARK || !bl->size) goto fatal;
        if (bl->size>=size)
        { unchain(bl);
          if (sizeof(void*)>4 ? bl->size<=2+size : bl->size==size)
		  {	size=bl->size;
            ((memblock *)BLOCK_END_PTR(bl))->prevfree=0;
		  }
		  else 
		  { /* divide */
			  memblock *nbl=(memblock*)((tptr)bl+16*size);
			  nbl->dcmark=DCMARK;  nbl->owner=0;
			  nbl->size=(uns16)(bl->size-size);
			  bl->size=(uns16)size;
			  chain(nbl);
			  nbl->prevfree=0;
			  *(((memblock **)BLOCK_END_PTR(nbl))-1)=nbl;
		  }
		  bl->owner=owner;
#ifdef MEM_CHECK
          memset(&bl->next, 0xc9, USABLE_BLOCK_SIZE(bl));  // debugging support
          ((tptr)bl)[16*size-1]=0x6e;
#endif
		      return &(bl->next);
	      }
	      prev=bl; bl=bl->next;
      }
    } // end of critical section
   // cannot alloc, can I extend the memory?
    if (memory_extended || !core_init(-EXTENSION_SIZE_64)) // memory has been or cannot be extended, cannot alloc
    { char msg[200];
      sprintf(msg, "Failed allocation of %u bytes of memory, managing %u blocks of total size %u.", 16*size, allocated_core_blocks, total_allocated_size);
      memory_not_extended(msg);  // may cause fatal exit of the program
      return NULL;
    }  
    memory_extended=true;
  } while (true);
 fatal: // fatal memory error processing must be outside the CS!
  fatal_memory_error("Fatal memory management error 3");  
  return NULL; 
}

////////////////////////////// verifying the integrity of the memory /////////////////////////////////
#define MAX_MEMORY_FRAGMENTS 5000000

CFNC DllKernel void WINAPI heap_test(void)
// Checks the integrity of the memory management
{
  { memblock * bl;  unsigned j;
    ProtectedByCriticalSection cs(&cs_memory);
   // check the list of free blocks:
    for (bl=freelist;  bl->next!=freelist;  bl=bl->next) /* both bl & bl->next must be valid */
    { if (bl->dcmark!=DCMARK || !bl->size) 
        goto heaperr;
      memblock * sptr = *(memblock**)(BLOCK_END_PTR(bl) - sizeof(memblock*));
      if (sptr!=bl && bl->owner!=255)
        goto heaperr;
    }    
    for (bl=freelist;  bl->prev!=freelist;  bl=bl->prev) /* both bl & bl->prev must be valid */
      if (bl->dcmark!=DCMARK || !bl->size) 
        goto heaperr;
   // check all blocks allocated from the system:
    for (int i=0;  i<allocated_core_blocks;  i++)
    { bl=origlist[i];  j=0;
      do
      { if (bl->dcmark!=DCMARK || !bl->size) goto heaperr;
         bl=(memblock*)BLOCK_END_PTR(bl);
         if (j++ > MAX_MEMORY_FRAGMENTS) goto heaperr;
      } while (bl < heap_limit[i]);
    }
    return;
  } // end of CS
 heaperr:   // fatal memory error processing must be outside the CS!
  fatal_memory_error("Fatal memory management error 4");
}

CFNC DllKernel int WINAPI total_used_memory(void)
// Returns the amount of memory in bytes allocated by the users of the memory management.
{ int sum=0;
  ProtectedByCriticalSection cs(&cs_memory);
 // cycle on the blocks orginally allocated from the system:
  for (int i=0;  i<allocated_core_blocks && origlist[i];  i++)
  { memblock * bl=origlist[i];  int j=0;
    do
    { if (bl->owner) sum += USABLE_BLOCK_SIZE(bl);  // block is in use: add its size then
      bl=(memblock*)BLOCK_END_PTR(bl); // goes to the next block of memory
      if (j++ > MAX_MEMORY_FRAGMENTS) break; // prevents inficite cycling on a crashed memory pool
    } while (bl < heap_limit[i]);
  }
  return sum;
}

bool create_memory_stats(t_mem_info mi[256])
{ memset(mi, 0, sizeof(t_mem_info[256]));
  bool error = false;
  ProtectedByCriticalSection cs(&cs_memory);
  for (int i=0;  i<allocated_core_blocks && !error;  i++)
  { memblock * bl = origlist[i];  int j=0;
    do
    { if ((bl->dcmark!=DCMARK) || !bl->size) { error = true;  break; }
      mi[bl->owner].size += bl->size;  mi[bl->owner].items++;
      bl=(memblock*)BLOCK_END_PTR(bl); // goes to the next block of memory
      if (j++ > MAX_MEMORY_FRAGMENTS)
        { error = true;  break; }
    } while (bl < heap_limit[i]);
  }
  return error;
}


CFNC DllKernel void WINAPI PrintHeapState(t_print_info_line * print_line)
// Logs the memory allocations grouped by owners
#define OWNERS_ARRAY_SIZE 256
{ int i;
 // count the allocations by owners:
  t_mem_info aMemOwners[OWNERS_ARRAY_SIZE];
  if (create_memory_stats(aMemOwners))
    (*print_line)("!!! Memory error");  
  else  // print the allocations (must be outside the cs_memory):
  { char szLine[81], szPattern[21];
    *szLine = 0;
    for (i = 0; i < OWNERS_ARRAY_SIZE; ++i)
      if (aMemOwners[i].size)
      { sprintf(szPattern, "%u:%u ", i, aMemOwners[i].size);
        if (strlen(szLine) + strlen(szPattern) >= 80-23)  // user space on the line
        { (*print_line)(szLine);
          *szLine = 0;
        }
        strcat(szLine, szPattern);
      }
    if (*szLine) (*print_line)(szLine);
  }
}

/******************** memory management utilities *********************/
CFNC DllKernel void * WINAPI corerealloc(void * x, unsigned y)
// Changes the size of the memory block.
// Preserves the owner (if reallocating the existing block) or defines the owner as 1 (for non-existing blocks)
// Preserves as much as possible from the contents.
{ unsigned origsize;
  if (x==NULL) return corealloc(y, 1);
  memblock * fr = BLOCK_FROM_PTR(x);
  void * p = corealloc(y, fr->owner);
  if (fr->dcmark==BIG_MARK)
    origsize = ((unsigned)fr->size<<14) - MEM_PREFIX_SIZE;
  else
    origsize = USABLE_BLOCK_SIZE(fr);
  if (p==NULL)  /* cannot alloc */
    return y<=origsize ? x : NULL;
  if (fr->size) memcpy(p, x, y < origsize ? y : origsize);
  corefree(x);
  return p;
}

CFNC DllKernel void WINAPI safefree(void * * px)
// Deallocates the memory block and sets its pointer to NULL
{ corefree(*px);  *px=NULL; }

CFNC DllKernel void WINAPI memmov(void * destin, const void * source, size_t size)
// Patch for platforms which do not support memove(.., .., 0)
{ if (size) memmove(destin, source, size); }

