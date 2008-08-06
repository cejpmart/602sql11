#ifdef UNIX
#include <stdlib.h>
#endif

void free_cursor(tcursnum curs);  /* unregisters and deletes cursor */

#if defined(SRVR) || (WBVERS<900)
void *operator new(size_t size)
  { return corealloc(size, 97); }
void operator delete(void * ptr)
  { corefree(ptr); }
void *operator new[](size_t size)
  { return corealloc(size, 197); }
void operator delete[](void * ptr)
  { corefree(ptr); }
#endif

#ifndef SRVR
struct t_express
{ int dummy; };
#endif

/****************************** dynamic array *******************************/
BOOL index_dynar::delet(unsigned index)
{ if (index >= (unsigned)count()) return FALSE;
  acc(index)->ind_descr::~ind_descr();
  return t_dynar::delet(index);
}

index_dynar::~index_dynar(void)    /* t_dynar destructor called then */
{ for (int i=0;  i<count();  i++) acc(i)->ind_descr::~ind_descr(); }

BOOL check_dynar::delet(unsigned index)
{ if (index >= (unsigned)count()) return FALSE;
  acc(index)->check_constr::~check_constr();
  return t_dynar::delet(index);
}

check_dynar::~check_dynar(void)    /* t_dynar destructor called then */
{ for (int i=0;  i<count();  i++) acc(i)->check_constr::~check_constr(); }

BOOL refer_dynar::delet(unsigned index)
{ if (index >= (unsigned)count()) return FALSE;
  acc(index)->forkey_constr::~forkey_constr();
  return t_dynar::delet(index);
}

refer_dynar::~refer_dynar(void)    /* t_dynar destructor called then */
{ for (int i=0;  i<count();  i++) acc(i)->forkey_constr::~forkey_constr(); }

BOOL attr_dynar::delet(unsigned index)
{ if (index >= (unsigned)count()) return FALSE;
  acc(index)->atr_all::~atr_all();
  return t_dynar::delet(index);
}

attr_dynar::~attr_dynar(void)    /* t_dynar destructor called then */
{ for (int i=0;  i<count();  i++) acc(i)->atr_all::~atr_all(); }

ind_descr::~ind_descr(void)   /* destructor for reftables called automatically */
{ corefree(text);  text=NULL; }

check_constr::~check_constr(void)
{ corefree(text);  text=NULL; }


