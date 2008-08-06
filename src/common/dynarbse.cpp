// dynarbse.cpp - dynar base class implementation

void * t_dynar::acc2(unsigned index)
{ if (index >= arrsize)
  { unsigned newarrsize=(index/step+1)*step;
    tptr newels = (tptr)corerealloc(elems, newarrsize*elsize);
    if (!newels) return NULL;
    for (unsigned i=arrsize;  i<newarrsize;  i++) init_elem(newels+i*elsize);
//    memset(newels+arrsize*elsize, 0, (newarrsize-arrsize)*elsize);
    elems=newels;  arrsize=newarrsize;
  }
  if (index>=elcnt) elcnt=index+1;
  return elems+(index*elsize);
}

void t_dynar::minimize(void)
{ if (elcnt < arrsize)
  { tptr newels = (tptr)corerealloc(elems, elcnt*elsize);
    if (!newels) return;   /* no change if cannot minimize */
    corefree(elems);  elems=newels;  arrsize=elcnt;
  }
}

int t_dynar::find(void * pattern)
{ unsigned i;  tptr p;
  for (i=0, p=elems;  i<elcnt;  i++, p+=elsize)
    if (!memcmp(pattern, p, elsize)) return i;
  return -1;
}

void t_dynar::init(unsigned c_elsize, unsigned c_arrsize, unsigned c_step)
{ elcnt = 0;  elsize=c_elsize;  step = c_step;
  elems = (tptr)corealloc(elsize * c_arrsize, 93);
  if (elems)
  { arrsize = c_arrsize;
    for (unsigned i=0;  i<arrsize;  i++) init_elem(elems+i*elsize);
  }
  else arrsize=0;
}

BOOL t_dynar::delet(unsigned index)
{ if (index >= elcnt) return FALSE;
  memmov(elems+index*elsize, elems+(index+1)*elsize, (elcnt-index-1)*elsize);
  elcnt--;
  init_elem(elems+elcnt*elsize);
  return TRUE;
}

void * t_dynar::insert(unsigned index)
{ if (index < elcnt)
  { if (elcnt==arrsize) { if (!acc(elcnt)) return NULL; }
    else elcnt++;
    memmov(elems+(index+1)*elsize, elems+index*elsize, (elcnt-1-index)*elsize);
    init_elem(elems+index*elsize);
  }
  return acc(index);
}

void t_dynar::drop(void)
{ elcnt=arrsize=0;  elems=NULL; }  /* no free(elems)! */

void t_dynar::init_elem(void * elptr)
{ memset(elptr, 0, elsize); }

#ifdef SRVR
void t_dynar::mark_core(void)
{ if (elems) mark(elems); }
#endif



