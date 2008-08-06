///////////////////////////////// t_flstr //////////////////////////////////
#ifndef _FLSTR_H
#define _FLSTR_H

#ifdef putc
#undef putc
#endif

class t_flstr
{ unsigned actsize, maxsize, step;
  tptr val;
  BOOL err;
 public:
  t_flstr(unsigned inisize=10, unsigned inistep=100)
  { val=(tptr)corealloc(inisize+1, 81);
    if (val==NULL) { err=TRUE;   maxsize=0; }
    else           { err=FALSE;  maxsize=inisize;  *val=0; }
    actsize=0;
    step=inistep;
  }
  inline ~t_flstr   (void)      { corefree(val); }
  inline tptr get   (void)      { if (err) return NULL;  val[actsize]=0;  return val; }
  inline unsigned getlen(void)  { return err ? 0 : actsize; }
  inline BOOL error (void)      { return err; }
  BOOL putn  (const char * str, unsigned size);
  BOOL putint(sig32 val);
  BOOL putint2(sig32 val);
  BOOL putint64(sig64 val);
  BOOL putss (const char * str);
  BOOL putssx(const char * str);
  void putproc(const char * str);
  void putqq(const char * str);

  BOOL put(const char * str)
    { return putn(str, (unsigned)strlen(str)); }
  inline BOOL putc(char c)
    { return putn(&c, 1); }
  inline BOOL putq(const char * name)
    { putc('`');  put(name);  return putc('`'); }
  inline BOOL putq_gen(const char * name, const char * quote)
    { put(quote);  put(name);  return put(quote); }
  inline void reset (void)
    { actsize=0;  if (maxsize) err=FALSE; } // partial work-around
  inline tptr unbind(void)
    { val[actsize]=0;  tptr p=val;  val=NULL;  return p; }
};

#include <wchar.h>

class t_flwstr
{ unsigned actsize, maxsize, step;  // all in characters
  wchar_t * val;
  bool err;
 public:
  t_flwstr(unsigned inisize=10, unsigned inistep=100)
  { val=(wchar_t*)corealloc((inisize+1)*sizeof(wchar_t), 81);
    if (val==NULL) { err=true;   maxsize=0; }
    else           { err=false;  maxsize=inisize;  *val=0; }
    actsize=0;
    step=inistep;
  }
  inline ~t_flwstr   (void)     { corefree(val); }
  inline wchar_t * get(void)    { if (err) return NULL;  val[actsize]=0;  return val; }
  inline unsigned getlen(void)  { return err ? 0 : actsize; }
  inline bool error (void)      { return err; }
  bool putn  (const wchar_t * str, unsigned size);
  bool putint(sig32 val);
  bool putint2(sig32 val);
  bool putint64(sig64 val);
  bool putss (const wchar_t * str);
  bool putssx(const wchar_t * str);
  void putproc(const wchar_t * str);
  void putqq(const wchar_t * str);
  wchar_t *getptr(unsigned size);

  bool put   (const wchar_t * str)
    { return putn(str, (unsigned)wcslen(str)); }
  inline bool putc  (wchar_t c)
    { return putn(&c, 1); }
  inline bool putq  (const wchar_t * name)
    { putc('`');  put(name);  return putc('`'); }
  inline BOOL putq_gen(const wchar_t * name, const wchar_t * quote)
    { put(quote);  put(name);  return put(quote); }
  inline void reset (void)
    { actsize=0;  if (maxsize) err=false; } // partial work-around
  inline wchar_t * unbind(void)
    { val[actsize]=0;  wchar_t * p=val;  val=NULL;  return p; }
};

#endif  // _FLSTR_H

