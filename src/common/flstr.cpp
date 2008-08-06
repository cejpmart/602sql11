/////////////////////////// flexible string /////////////////////////////////
// Accumulated error if cannot allocate enough memory
// Contains space for terminating null, added on "get"

BOOL t_flstr::putn(const char * str, unsigned size)
{ if (err) return FALSE;
  if (actsize+size > maxsize)
  { unsigned newmax = actsize+size > maxsize+step ? actsize+size : maxsize+step;
    tptr re=(tptr)corerealloc(val, newmax+1);
    if (re==NULL) { err=TRUE;  return FALSE; }
    val=re;  maxsize=newmax;
  }
  memcpy(val+actsize, str, size);
  actsize+=size;
  return TRUE;
}
BOOL t_flstr::putint(sig32 val)
{ char buf[16];  buf[0]=' ';
  int2str(val, buf+1);
  return put(buf);
}
BOOL t_flstr::putint2(sig32 val)
{ char buf[16];  
  int2str(val, buf);
  return put(buf);
}
BOOL t_flstr::putint64(sig64 val)
{ char buf[30];  buf[0]=' ';
  int64tostr(val, buf+1);
  return put(buf);
}
BOOL t_flstr::putss(const char * str)
{ char q='\"';
  putn(&q, 1);
  int l = (int)strlen(str);
  for (int i=0;  i<l;  i++)
  { putn(str+i, 1);
    if (str[i]=='\'' || str[i]=='\"') putn(str+i, 1);
  }
  return putn(&q, 1);
}
BOOL t_flstr::putssx(const char * str)
{ char q='\'';
  putn(&q, 1);
  int l = (int)strlen(str);
  for (int i=0;  i<l;  i++)
  { putn(str+i, 1);
    if (str[i]=='\'' || str[i]=='\"') putn(str+i, 1);
  }
  return putn(&q, 1);
}
void t_flstr::putproc(const char * str)
{ while (*str)
  { putc(*str);
    if (*str=='%') putc(*str);
    str++;
  }
}

void t_flstr::putqq(const char * str)
// puts str enclosed in `...` only if str does not have own `...`
{ while (*str==' ') str++;
  if (*str!='`') putc('`');
  int len = (int)strlen(str);
  while (len && str[len-1]==' ') len--;
  putn(str, len);
  if (len && str[len-1]!='`') putc('`');
}

/////////////////////////
bool t_flwstr::putn(const wchar_t * str, unsigned size)
{ if (err) return false;
  wchar_t *ptr = getptr(size);
  if (!ptr) return false;
  memcpy(ptr, str, size*sizeof(wchar_t));
  actsize+=size;
  return true;
}

wchar_t *t_flwstr::getptr(unsigned size)
{ if (actsize+size > maxsize)
  { unsigned newmax = actsize+size > maxsize+step ? actsize+size : maxsize+step;
    wchar_t * re = (wchar_t*)corerealloc(val, (newmax+1)*sizeof(wchar_t));
    if (re==NULL) { err=true;  return NULL; }
    val=re;  maxsize=newmax;
  }
  return val + actsize;
}

bool t_flwstr::putint(sig32 val)
{ wchar_t buf[16];  buf[0]=' ';
  swprintf(buf+1, 
#ifdef LINUX
           16,
#endif			    	
           L"%d", val);
  return put(buf);
}
bool t_flwstr::putint2(sig32 val)
{ wchar_t buf[16];  
  swprintf(buf, 
#ifdef LINUX
           16,
#endif			    	
           L"%d", val);
  return put(buf);
}
bool t_flwstr::putint64(sig64 val)
{ wchar_t buf[30];  buf[0]=' ';
  swprintf(buf+1, 
#ifdef LINUX
           30,
#endif			    	
           L"%I64d", val);
  return put(buf);
}
bool t_flwstr::putss(const wchar_t * str)
{ wchar_t q='\"';
  putn(&q, 1);
  int l = (int)wcslen(str);
  for (int i=0;  i<l;  i++)
  { putn(str+i, 1);
    if (str[i]=='\'' || str[i]=='\"') putn(str+i, 1);
  }
  return putn(&q, 1);
}
bool t_flwstr::putssx(const wchar_t * str)
{ wchar_t q='\'';
  putn(&q, 1);
  int l = (int)wcslen(str);
  for (int i=0;  i<l;  i++)
  { putn(str+i, 1);
    if (str[i]=='\'' || str[i]=='\"') putn(str+i, 1);
  }
  return putn(&q, 1);
}
void t_flwstr::putproc(const wchar_t * str)
{ while (*str)
  { putc(*str);
    if (*str=='%') putc(*str);
    str++;
  }
}

void t_flwstr::putqq(const wchar_t * str)
// puts str enclosed in `...` only if str does not have own `...`
{ while (*str==' ') str++;
  if (*str!='`') putc('`');
  int len = (int)wcslen(str);
  while (len && str[len-1]==' ') len--;
  putn(str, len);
  if (len && str[len-1]!='`') putc('`');
}
