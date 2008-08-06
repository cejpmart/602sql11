/****************************************************************************/
/* smemory.c - the memory management                                        */
/****************************************************************************/
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
#include "nlmtexts.h"
#include "netapi.h"
#ifndef WINS
#include <stdlib.h>
#endif
#include <stddef.h>
#include "wbsecur.h"
/*
Cile: WB ma vlastni spravu pameti, aby bylo mono snadneji detekovat chyby ve vyuiti pameti, a to:
- ztr�y pam�i (nevracen�alokovanch blok),
- z�is za hranice alokovan�o bloku pam�i,
- pr�i s d�e uvoln�m blokem pam�i.
Spr�a pam�i si alokuje pam� od syst�u. Pro inicializaci spr�y a zv�en�alokace slou�funkce core_init.
Pro vr�en�veker�pam�i syst�u slou�core_release.

Po�t blok alokovanch od syst�u je v allocated_core_blocks, ukazatele na za�tky blok jsou v origlist[],
ukazatele na za�tky blok po zaukrouhlen�jsou v origlist_aligned[],
ukazatele za konce blok jsou v heap_limit[]. V orighnd[] jsou handles t�hto blok.

P�nakem, e spr�a pam�i je inicializovan� je allocated_core_blocks!=0. Jeliko destruktury statickch t� 
mohou volat spr�u pam�i po skon�n�programu, je nutno inicializaci testovat v API spr�y pam�i.

Spr�a pam�i m�spojov seznam blok pam�i nealokovanch uivateli spr�y. Na za�tku jsou v n� bloky pam�i 
od syst�u, ty se pak mohou d�it na men�bloky. Z�ladn�granularita pam�ovch alokac�je 16 bajt. 
Bloky ve spojov� seznamu jsou uspo��y vzestupn�podle adres, aby se uvol�van�bloky daly snadno zcelovat.

Na za�tku kad�o bloku je struktura memblok vyu�an�spr�ou pam�i. Uivatele spr�y pam�i pi odkazech na blok 
pracuj�s ukazatelem, kter odkazuje bezprostedn�poloku next v t�o struktue. Voln�bloky jsou spojen�pomoc�poloek next,
zat�co v alokovanch bloc�h pat�poloka next ji do uivatelsk�oblasti bloku.

Memblock obsahuje identifika��zna�u bloku (dcmark) rovnou DCMARK, vlastn�a bloku (owner) a velikost bloku (size) v jednotk�h 
o velikosti 16 bajt. Je-li owner==0, je blok voln, jinak owner dovoluje identifikovat majitele ztracench blok 
a m�it rozsah alokac�pro rzn��ly.

Alokaci a dealokaci pameti provadeji funkce corealloc a corefree. Pokud je pozadovana velkost bloku vetsi 
nez INTERNAL_ALLOCATION_LIMIT, pak se blok alokuje od systemu a jemu se i vrati. Pro bloky alokovan�t�to zpsobem plat�
- dcmark je BIG_MARK,
- velikost bloku je v jednotk�h 16KB (maxim�n�alokace 1 BG), granularita je tak�16 KB.
Kdy alokace pam�i nen�mon�kvli nedostatku volnch blok ve spr��pam�i, pak se od syst�u alokuje dal�blok pam�i
o velikosti ALLOC_SIZE_64 * 64KB. Pak se zkus�alokace opakovan�

P�tup ke spr��pam�i je chr�� kritickou sekc�cs_memory.

P�p�tupu k API spr�y pam�i se kontroluje integrita pam�i. Pokud je poruena, proces skon�, co je lep�alternativa,
ne riskovat pepis obsahu datab�e.

K vy�rp��intern�pam�i me doj� po pekro�n�po�u MAX_HEAP_PARTS blok pam�i alokovanch spr�ou od syst�u.
Pokud se alokace od syst�u da� pak je to pes 20 MB pam�i. Velikost alokovan�pam�i lze tak�omezit konstantou MAX_HEAP_64,
co m�smysl na NLM.

  Nedostatky: 
Maxim�n�velikost bloku pam�i ve spr��je men�ne 16 kr� 64KB, protoe v��velikost 
nelze zaznamenat do deskriptoru pam�ov�o bloku. Proto v��mnostv�pam�i se od syst�u mus�alokovat
po men�h kusech.
*/

/***************************** fatal error **********************************/
static wbbool processing_fatal_error = wbfalse;  // prevent re-entering the fatal error processing

void fatal_error(cdp_t cdp, const char * txt)
// Terminates the WB server due to a fatal error.
{ if (processing_fatal_error) return;
  processing_fatal_error=wbtrue;
  dump_thread_overview(cdp);
  cd_dbg_err(cdp, txt);
  trace_msg_general(0, TRACE_START_STOP, server_term_error, NOOBJECT);  // not translating the message, it needs the memory allocation
  PrintHeapState(direct_to_log);
#ifdef WINS
  *(char*)0=1;  // crash
  FatalExit(10); 
#else           
  abort(); 
#endif
}

void fatal_memory_error(const char * msg);  // defined in membase.cpp

static void memory_not_extended(const char * msg)
{ char amsg[81];
  display_server_info(form_message(amsg, sizeof(amsg), memoryNotExtended));
  fatal_memory_error(msg);
}

#define STD_CORE_ALLOC   6    // initial allocation in 64KB units

#include "membase.cpp"

///////////////////////////////////////// searching the lost memory /////////////////////////
/*                                 Hled��ztr� pam�i
Hled��ztracench blok funguje tak, e se naped ozna� vechny bloky pam�i pou�an�serverem
a pak se projdou vechny bloky ve spr��pam�i a hledaj�se ty, kter�nejsou voln�(owner!=0),
ale z�ove�nejsou ozna�n�jako vyuit� Z hodnoty "owner" lze pak odhadnout vin�a.

Velikost ztracen�pam�i vrac�funkce leak_test(). Pomoc�BP lze v n�vid� tak�vin�y.

Ozna�v��prob��tak, e se "dcmark" v kad� bloku pam�i zm��na hodnotu DD_MARK. Prov��to funkce 
mark(), kterou vol�ada member funkc�v rznch t��h. Pi s�t��ztr� se dcmar mus�peklopit zp�.

Hled��ztr� pam�i nen�pesn� kdy server zpracov��n�ak�poadavky, protoe do�sn�alokovan�pam�
nemus�bt vdy ozna�na. Kritick�sekce br��tomu, aby kvli dcmark==DD_MARK dolo k ukon�n�serveru.

Takto nelze hledat ztr�y pam�i alokovan�p�o od syst�u kvli velikosti pekra�j��INTERNAL_ALLOCATION_LIMIT.
*/

void WINAPI mark(const void * block)
// Mark the memory block as "not lost".
{ memblock * bl = (memblock*)(((tptr)block)-MEM_PREFIX_SIZE);
  if (bl->dcmark==DCMARK)  // dcmark may be BIG_MARK (big block), DD_MARK (already marked) or anything else on error
    bl->dcmark=DD_MARK;
}

void WINAPI mark_array(const void * block)
// Mark the memory block allocated by new [] as "not lost".
{ memblock * bla = (memblock*)(((tptr)block)-4-MEM_PREFIX_SIZE);
  memblock * blb = (memblock*)(((tptr)block)-MEM_PREFIX_SIZE);
  if (bla->dcmark==DCMARK || bla->dcmark==DD_MARK || bla->dcmark==BIG_MARK)
    if (blb->dcmark==DCMARK || blb->dcmark==DD_MARK || blb->dcmark==BIG_MARK)
      return;  // cannot decide, which is the right one, must not change any of them!!
  if (bla->dcmark==DCMARK)  // dcmark may be BIG_MARK (big block), DD_MARK (already marked) or anything else on error
    bla->dcmark=DD_MARK;
  else if (blb->dcmark==DCMARK)
    blb->dcmark=DD_MARK;
}

#if WBVERS>=1100
void mark_callback(const void * ptr)
{ 
  if (ptr) mark(ptr);
}

void mark_wcs(t_mark_callback * mark_callback);
#endif

unsigned WINAPI leak_test(void)
{ int i;
  ProtectedByCriticalSection cs8(&cs_tables);      // deadlock prevention: cs_tables is entered in some mark() functions
  ProtectedByCriticalSection cs7(&cs_sequences);   // deadlock prevention: cs_sequences is entered in some mark() functions
  ProtectedByCriticalSection cs6(&cs_frame);       // deadlock prevention: cs_frame is entered in some mark() functions
  ProtectedByCriticalSection cs5(&cs_client_list); // [cds] accessed below, must enter before cs_memory
  ProtectedByCriticalSection cs4(&cs_trace_logs);  // deadlock prevention: cs_trace_logs is entered in some mark() functions
  ProtectedByCriticalSection cs3(&crs_sem);        // [crs] accessed below, must enter before cs_memory
  ProtectedByCriticalSection cs2(&cs_short_term);  // deadlock prevention: cs_short_term is entered in some mark() functions
  ProtectedByCriticalSection cs1(&cs_memory);
 // mark all used memory blocks
  ffa.mark_core();
  // owned by all cds:
  for (i=0;  i<=max_task_index;  i++) if (cds[i])
    cds[i]->mark_core();
  // owned by installed tables
  mark_installed_tables();
  // owned by installed cursors
  for (tcursnum crnm=0;  crnm<crs.count();  crnm++)  /* autom. closing of cursors */
  { cur_descr * cd = *crs.acc(crnm);
    if (cd) cd->mark_core();
  }
  crs.mark_core();
  // cached routines:
  psm_cache.mark_core();
  mark_sequence_cache();  
  mark_core_pieces();
  mark_replication();
  if (jourbuf) mark(jourbuf);
  system_scope.mark_core();
  frame_mark_core();
  the_sp.dll_directory_list.mark_core();
  mark_ip_filter();
  mark_fulltext_kontext_list();
  mark_fulltext_list();
#ifdef WINS
  extlibs_mark_core();
#endif
  mark_trace_logs();
  mark_admin_mode_list();
  mark_global_sscopes();
  mark_extensions();
  offline_threads.mark_core();
  prof_mark_core();
#if WBVERS>=1100
  mark_allocations(&mark_callback);
  mark_wcs(&mark_callback);
#endif
  mark_schema_cache_dynar();
  mark(corepool);

 // look for unmarked blocks, restore the DCMARK value:
  unsigned lost=0, not_joined=0;
  for (i=0;  i<allocated_core_blocks && origlist[i];  i++)
  { memblock * bl = origlist[i]; 
    bool prev_is_free = false;
    do
    {// count lost space:
      if (bl->dcmark!=DD_MARK && bl->owner) 
        lost+=bl->size;
      // count not joined parts:  
       if (!bl->owner)
       { if (prev_is_free) not_joined++;
         else prev_is_free=true;
       }
       else prev_is_free=false;  
       bl->dcmark=DCMARK; // restore the dcmark value
       bl=(memblock*)((tptr)bl+16*bl->size);
    } while (bl < heap_limit[i]);
  }
  if (not_joined)
  { char msg[100];
    sprintf(msg, "%u free memory parts not joined", not_joined);
    dbg_err(msg);
  }  
  return lost;
}

