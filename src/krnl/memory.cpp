/****************************************************************************/
/* memory.cpp - the client memory management                                */
/****************************************************************************/
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#include <stddef.h>
#ifdef UNIX
#include <stdlib.h>
#endif

static void WINAPI fatal_error(cdp_t cdp, const char * txt) 
// Called when a fatal error like a memory crash is detected (from inside of cs_memory). 
// Aborts the process.
{ 
#ifndef ODBCSTP
  if (!cdp) cdp = GetCurrTaskPtr();
  if (cdp) client_error(cdp, MEMORY_CRASH);  // displays the error to the monitor
#endif
  client_error_message(NULL, txt);
#ifdef WINS
  MessageBeep(-1);  Sleep(500);  MessageBeep(-1);  Sleep(500);  MessageBeep(-1);  Sleep(1000);
  FatalExit(10); 
#else
  abort(); 
#endif
}

static void memory_not_extended(const char * msg)
{
#ifndef ODBCSTP
  cdp_t cdp = GetCurrTaskPtr();
  if (cdp) client_error(cdp, MEMORY_NOT_EXTENDED);
#endif    
  client_error_message(NULL, msg);
}

#define STD_CORE_ALLOC   5    // initial allocation in 64KB units
// using the same value as in link_kernel_ex in cint.cpp

#include "membase.cpp"

#ifdef STOP  // the old memory management

static void WINAPI fatal_error(void) 
// Called when a fatal error like a memory crash is detected (from inside of cs_memory). 
// Aborts the process.
{ LeaveCriticalSection(&cs_memory);
#ifndef ODBCSTP
  cdp_t cdp = GetCurrTaskPtr();
  if (cdp) client_error(cdp, MEMORY_CRASH);  // displays the error to the monitor
#endif
#ifdef WINS
  MessageBeep(-1);  Sleep(500);  MessageBeep(-1);  Sleep(500);  MessageBeep(-1);  Sleep(1000);
  FatalExit(10); 
#else
  abort(); 
#endif
}

HGLOBAL wb_system_alloc(uns32 size, BOOL shared, tptr * ptr);
void    wb_system_free(void * ptr, HGLOBAL handle);
void    wb_system_freeptr(void * ptr);


/* Core is allocated from system in blocks whose size is a multiply of 64 KB - heap parts.
   Parts are subdivided into blocks starting with struct memblock.
   Free blocks are stored in a linked list ordered by address.
   Common pointer to an allocated memory block points MEM_PREFIX_SIZE bytes after its beginning.
*/

#define MAX_HEAP_PARTS  25    // maximal number of allocated heap parts (continous blocks)
#define MAX_HEAP_64    200    // maximal total allocation in 64KB units
#define ALLOC_SIZE_64   15    // maximal heap part size in 64KB units, must be <16
#define INTERNAL_ALLOCATION_LIMIT 150000 // allocations above this limit taken directly from the operating system

#define BLOCK_FROM_PTR(x) ((memblock*)(((tptr)x)-MEM_PREFIX_SIZE)) // converts common pointer to block ptr
#define NEXT_BLOCK(bl)    ((memblock*)((tptr)bl+16*bl->size)) // the next memory block in the linked list
#define USABLE_BLOCK_SIZE(bl) 16*(unsigned)bl->size-MEM_PREFIX_SIZE // bytes in a block available to the user
#define STD_CORE_ALLOC 6
 
#define DCMARK         0xdc   // value identifying a non-corrupted memory block
#define BIG_MARK       0xde   // alloated directly from the system

struct memblock  // memory block prefix
{ uns8  dcmark;     // magic number == DCMARK, used in corruption tests
  uns8  owner;      // 0 iff the memory block is free, block owner otherwise
  uns16 size;       // total block size (including this prefix) in 16 byte units
  memblock * next;  // defined iff the memory block is free, the beginning of block contents otherwise
};

#define MEM_PREFIX_SIZE   (offsetof(struct memblock, next)) // offset of common pointer to block start

static CRITICAL_SECTION cs_memory;            // protects access to memory management structures
static memblock * freelist = NULL,            // linked list of free memory blocks
                * origlist[MAX_HEAP_PARTS],   // pointers to heap parts
                * heap_limit[MAX_HEAP_PARTS]; // pointers after the end of each heap part
static HGLOBAL orighnd[MAX_HEAP_PARTS];       // handles of heap parts 
static int allocated_heap_parts = 0,          // number of allocated heap parts, < MAX_HEAP_PARTS
           allocated_size_64 = 0;             // size of allocated memory in 64KB units

CFNC DllKernel void WINAPI corefree(const void * x)
// Releases a memory block. 
// The size member must remain valid even if the block is linked with the previous block (used by total_free).
{ memblock *prev, *bl, *fr;
  if (x==NULL) return;
  fr=BLOCK_FROM_PTR(x);
  if (fr->dcmark==BIG_MARK) // block allocated directly from the system
    { free(fr);  return; }
  if (!allocated_heap_parts) return;  // prevents crash when destroying global instances of various classes
#ifdef HEAP_TESTS
  heap_test();
#endif
  EnterCriticalSection(&cs_memory);
  if (!fr->owner || fr->dcmark != DCMARK || !fr->size) fatal_error();
  fr->owner=0;  // mark the memory block as "free"
 // find the position of fr in the linked list and insert it:
  prev=NULL;  bl=freelist;
  while (bl)
  { if (fr < bl) // fr is before bl
    { if      (NEXT_BLOCK(fr) < bl) // link fr to bl
        fr->next=bl;
      else if (NEXT_BLOCK(fr) > bl) // fr block overlaps with bl
        fatal_error();    
      else  // NEXT_BLOCK(fr) ==bl     append bl to fr
      { fr->size+=bl->size;  fr->next=bl->next; } 
      if (prev) prev->next=fr; else freelist=fr;
      LeaveCriticalSection(&cs_memory);
      return;
    }
    if (fr <= NEXT_BLOCK(bl))  // append fr to bl, unless overlapping
    { if (fr < NEXT_BLOCK(bl)) fatal_error(); // fr block overlaps with bl
      bl->size += fr->size;
      if (NEXT_BLOCK(bl) == bl->next) /* filled a hole */
      { bl->size+=bl->next->size;
        bl->next =bl->next->next;
      }
      LeaveCriticalSection(&cs_memory);
      return;
    }
    prev=bl;  bl=bl->next;  // go to the next block in the list
  }
 // add fr to the end of the list of free blocks:
  fr->next=0;
  if (prev) prev->next=fr; else freelist=fr;
  LeaveCriticalSection(&cs_memory);
}

CFNC DllKernel void * WINAPI corealloc(unsigned size, uns8 owner)
// Allocates a block of memory. Tries to extend memory if cannot allocate.
{ memblock *prev, *bl, *nbl;
  if (!size) return NULL;
  size+=MEM_PREFIX_SIZE;                      // block size including the prefix
  if (size > INTERNAL_ALLOCATION_LIMIT || !allocated_heap_parts) // allocate directly from the system
  { size = ((size-1) & 0xffffc000) + 0x4000;
    bl=(memblock*)malloc(size);
    if (!bl) return NULL;
    bl->owner=owner;  bl->dcmark=BIG_MARK;  bl->size=size>>14;
    return &(bl->next);
  }
#ifdef HEAP_TESTS
  heap_test();
#endif
  size = (size & 0xf) ? size/16+1 : size/16;  // convert size to 16 byte untis
  if (size==1) size=2;
  EnterCriticalSection(&cs_memory);
  do
  {// find first free block big enough:
    prev=NULL;  bl=freelist;
    while (bl)
    { if (bl->dcmark != DCMARK || !bl->size || bl->next && bl->next < bl)
        fatal_error();
      if (bl->size>=size) // block found
      { if (bl->size<size+1) nbl=bl->next;
        else /* divide */
        { nbl=(memblock*)((tptr)bl+16*size);
          nbl->dcmark=DCMARK;  nbl->owner=0;
          nbl->size=(uns16)(bl->size-size);
          nbl->next=bl->next;  bl->size=(uns16)size;
        }
        if (prev) prev->next=nbl; else freelist=nbl;
        bl->owner=owner;
        LeaveCriticalSection(&cs_memory);
        return &(bl->next);
      }
      prev=bl;  bl=bl->next;  // go to the next block in the list
    }
   // memory block not found, try to allocate more memory from the system:
  } while (core_init(allocated_size_64 + ALLOC_SIZE_64));
  LeaveCriticalSection(&cs_memory);
  return NULL;  // cannot allocate the memory
}
                 
CFNC DllKernel BOOL WINAPI core_init(int num)
// Extends the memory allocation to "num" 64KB blocks.
// Called from inside of cs_memory iff extending, initializes cs_memory otherwise.
// Returns TRUE if anything allocated (FALSE is not an error when num <= allocated_size_64).
{ BOOL initial = allocated_heap_parts==0, anything_allocated=FALSE;
  if (num <= 0)          num=STD_CORE_ALLOC;  // minimal allocation
  if (num > MAX_HEAP_64) num = MAX_HEAP_64;   // maximal allocation
  if (num <= allocated_size_64) return FALSE;       // already allocated
  num -= allocated_size_64;                   // 64KB units to be allocated now
  while (num)  /* cycle on heap parts to be allocated */
  { if (allocated_heap_parts >= MAX_HEAP_PARTS) return anything_allocated;
    int this_alloc = num <= ALLOC_SIZE_64 ? num : ALLOC_SIZE_64;
    int size = this_alloc==1 ? 0xffe0 : this_alloc * 0xfff0;
   // allocate the heap part, maky it smaller if cannot allocate:
    do  /* cycle up to successfull allocation */
    { orighnd[allocated_heap_parts]=wb_system_alloc(size, TRUE, (tptr*)&origlist[allocated_heap_parts]);
      if (orighnd[allocated_heap_parts]) break;
      size=size/4*3;
      if (size<4096) goto report_and_exit; // cannot allocate (more) memory
    } while (TRUE);
   // initialize the allocated heap part:
    memblock * bl=origlist[allocated_heap_parts];
    bl->dcmark= DCMARK;
    bl->owner = 1;  /* will be changed to 0 in corefree */
    bl->size  = (uns16)(size/16);
    bl->next  = NULL;
    heap_limit[allocated_heap_parts]=NEXT_BLOCK(bl);  // bl->size must be defined!
    allocated_heap_parts++;  // must be before corefree() 
    allocated_size_64+=this_alloc;
    if (initial && !anything_allocated)
    { InitializeCriticalSection(&cs_memory); // must be before corefree()
    }
    anything_allocated=TRUE;
    corefree(&bl->next);
    num-=this_alloc;
  }
 report_and_exit:
#ifndef ODBCSTP
  cdp_t cdp = GetCurrTaskPtr();
  if (cdp)
  { if (!anything_allocated)
      client_error(cdp, MEMORY_NOT_EXTENDED);  // reporting memory failure to the client log
    //else if (!initial)  -- no, Signalize would report error!!
    //  client_error(cdp, MEMORY_EXTENDED);  // reporting memory extension to the client log
  }
#endif
  return anything_allocated;
}

CFNC DllKernel void WINAPI core_release(void)
// Releases all allocated memory and deinitializes the core management.
{ 
  if (allocated_heap_parts) 
  { for (int i=0;  i<allocated_heap_parts;  i++)
      if (origlist[i] && orighnd[i])
       wb_system_free(origlist[i], orighnd[i]);
    DeleteCriticalSection(&cs_memory);
  }
  allocated_heap_parts=allocated_size_64=0;
  freelist=NULL;
}

CFNC DllKernel void WINAPI heap_test(void)
// Checks the integrity of memory blocks
{ memblock * bl;  int i;
  EnterCriticalSection(&cs_memory);
 // check the free list:
  bl=freelist;
  if (bl)
     while (bl->next)  /* both bl & bl->next must be valid */
     { if (bl->dcmark!=DCMARK || !bl->size || bl->next <= bl ||
           bl->next < NEXT_BLOCK(bl)) fatal_error();
       bl=bl->next;
     }
 // check the heap parts:
  for (i=0;  i<allocated_heap_parts;  i++)
  { bl=origlist[i];
    do
    { if (bl->dcmark!=DCMARK || !bl->size) fatal_error();
      bl=NEXT_BLOCK(bl);
    } while (bl < heap_limit[i]);
  }
  LeaveCriticalSection(&cs_memory);
}

CFNC DllKernel int WINAPI total_used_memory(void)
// Returns the size of used memory
{ int sum=0;
  EnterCriticalSection(&cs_memory);
  for (int i=0;  i<allocated_heap_parts;  i++)
  { memblock * bl=origlist[i];  
    do
    { if (bl->owner) sum += USABLE_BLOCK_SIZE(bl);
      bl=NEXT_BLOCK(bl);
    } while (bl < heap_limit[i]);
  }
  LeaveCriticalSection(&cs_memory);
  return sum;
}

CFNC DllKernel void WINAPI PrintHeapState(t_print_info_line * print_line)
// Logs the memory allocations grouped by owners
#define OWNERS_ARRAY_SIZE 256
{ int i;  unsigned j;
  if (!client_error_callback) return;
 // count the allocations by owners:
  unsigned aMemOwners[OWNERS_ARRAY_SIZE];
  memset(aMemOwners, 0, sizeof(aMemOwners));
  bool error = false;
  { EnterCriticalSection(&cs_memory);
    for (i=0;  i<allocated_heap_parts;  i++)
    { memblock * bl = origlist[i];  j=0;
      do
      { if ((bl->dcmark!=DCMARK) || !bl->size) 
          { error = true;  break; }
        aMemOwners[bl->owner] += bl->size;
        bl=NEXT_BLOCK(bl);
      } while (bl < heap_limit[i]);
    }
    LeaveCriticalSection(&cs_memory);
  }
 // print the allocations (must be outside the cs_memory):
  if (error)
    (*client_error_callback)("Memory error");
  else
  { char szLine[81], szPattern[21];
    *szLine = 0;
    for (i = 0; i < OWNERS_ARRAY_SIZE; ++i)
      if (aMemOwners[i])
      { sprintf(szPattern, "%u:%u ", i, aMemOwners[i]);
        if (strlen(szLine) + strlen(szPattern) >= 80-23)  // user space on the line
        { (*client_error_callback)(szLine);
          *szLine = 0;
        }
        strcat(szLine, szPattern);
      }
    if (*szLine) (*client_error_callback)(szLine);
  }
}


/******************** support functions *********************/
CFNC DllKernel void * WINAPI corerealloc(void * x, unsigned y)
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
  memcpy(p, x, y < origsize ? y : origsize);
  corefree(x);
  return p;
}

CFNC DllKernel void WINAPI safefree(void * * px)
{ corefree(*px);  *px=NULL; }

CFNC DllKernel void WINAPI memmov(void * destin, const void * source, unsigned size)
{ if (size) memmove(destin, source, size); }


/***************************** system memory allocs ************************/
HGLOBAL wb_system_alloc(uns32 size, BOOL shared, tptr * ptr)
// Allocates memory from the system, return its handle and memory pointer in ptr.
// Returns 0 iff cannot alloc.
{ void * mem;
#ifdef WINS
  HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, size);
  if (!handle) goto cannot_alloc;
  mem=GlobalLock(handle);
  if (!mem) { GlobalFree(handle);  goto cannot_alloc; }
  *ptr=(tptr)mem;
  return handle;
#else
#ifdef UNIX
  mem=malloc((unsigned)size);
  if (!mem) goto cannot_alloc; 
#else  // NLM
  LONG allocated;  static LONG res_tag=0;
  if (!res_tag)
  { res_tag=AllocateResourceTag(GetNLMHandle(), (unsigned char*)"WB602: Memory Allocation", CacheNonMovableMemorySignature);
    if (!res_tag) goto cannot_alloc; 
  }
  mem=AllocNonMovableCacheMemory(size, &allocated, res_tag);
  if (!mem) goto cannot_alloc; 
  if (allocated < size)
    { FreeNonMovableCacheMemory(mem);  goto cannot_alloc; }
#endif
  *ptr=(tptr)mem;
  return ptr;  // non-zero value replacing the handle
#endif
 cannot_alloc:
  *ptr=NULL;
  return 0;
}

#ifdef WINS
void wb_system_free(void * ptr, HGLOBAL handle)
  { GlobalUnlock(handle); GlobalFree(handle); }
void wb_system_freeptr(void * ptr)
  { wb_system_free(ptr, GlobalPtrHandle(ptr)); }
#else
#ifdef UNIX
void wb_system_free(void * ptr, HGLOBAL handle)
  { ::free(ptr); }
void wb_system_freeptr(void * ptr)
  { ::free(ptr); }
#else // NLM
void wb_system_free(void * ptr, HGLOBAL handle)
  { FreeNonMovableCacheMemory(ptr); }
void wb_system_freeptr(void * ptr)
  { FreeNonMovableCacheMemory(ptr); }
#endif
#endif

#endif
/////////////////////////////// client specifics //////////////////////////////
BOOL total_free(uns8 tp)
// Releases all memory allocated by owner tp.
{ memblock * bl;  
#ifdef HEAP_TESTS
  heap_test();
#endif
  EnterCriticalSection(&cs_memory);
  for (int i=0;  i<allocated_core_blocks;  i++)
  { bl=origlist[i];  int j=0;
    do
    { if (bl->owner==tp) corefree((tptr)bl+MEM_PREFIX_SIZE);
      bl=NEXT_BLOCK(bl);
      if (j++ > 50000) fatal_memory_error("Crash in total_free()");
    } while (bl < heap_limit[i]);
  }
  LeaveCriticalSection(&cs_memory);
  return TRUE;
}

CFNC DllKernel uns32 WINAPI coresize(const void * x)
{
  memblock *bl = BLOCK_FROM_PTR(x);
  if (bl->dcmark==BIG_MARK)
    return (bl->size<<14)-MEM_PREFIX_SIZE;
  return USABLE_BLOCK_SIZE(bl);
}

CFNC DllKernel uns32 WINAPI free_sum(void)
// Checks the memory integrity and returns the size of the free memory. Obsolete function, only for compatibility.
{ uns32 sum=0;
#ifdef HEAP_TESTS
  heap_test();
#endif
  EnterCriticalSection(&cs_memory);
  for (memblock * bl=freelist;  bl->next!=freelist;  bl=bl->next) /* both bl & bl->next must be valid */
    sum += USABLE_BLOCK_SIZE(bl);
  LeaveCriticalSection(&cs_memory);
  return sum;
}

