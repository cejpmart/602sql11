/*
------------------------------------------------------------------------------
Standard definitions and types, Bob Jenkins
------------------------------------------------------------------------------
*/
typedef  unsigned       char ub1;

#define bis(target,mask)  ((target) |=  (mask))
#define bic(target,mask)  ((target) &= ~(mask))
#define bit(target,mask)  ((target) &   (mask))
#ifndef min
# define min(a,b) (((a)<(b)) ? (a) : (b))
#endif /* min */
#ifndef max
# define max(a,b) (((a)<(b)) ? (b) : (a))
#endif /* max */
#ifndef align
# define align(a) (((ub4)a+(sizeof(void *)-1))&(~(sizeof(void *)-1)))
#endif /* align */
#ifndef abs
# define abs(a)   (((a)>0) ? (a) : -(a))
#endif

/*
------------------------------------------------------------------------------
rand.h: definitions for a random number generator
MODIFIED:
  960327: Creation (addition of randinit, really)
  970719: use context, not global variables, for internal state
  980324: renamed seed to flag
  980605: recommend RANDSIZL=4 for noncryptography.
------------------------------------------------------------------------------
*/

/*------------------------------------------------------------------------------
 If (flag==TRUE), then use the contents of randrsl[0..RANDSIZ-1] as the seed.
------------------------------------------------------------------------------*/
void randinit(randctx *r, int flag);
void isaac(randctx *r);
/*------------------------------------------------------------------------------
rand.c: By Bob Jenkins.  My random number generator, ISAAC.
MODIFIED:
  960327: Creation (addition of randinit, really)
  970719: use context, not global variables, for internal state
  980324: added main (ifdef'ed out), also rearranged randinit()
------------------------------------------------------------------------------
*/
#define ind(mm,x)  (*(ub4 *)((ub1 *)(mm) + ((x) & ((RANDSIZ-1)<<2))))
#define rngstep(mix,a,b,mm,m,m2,r,x) \
{ \
  x = *m;  \
  a = (a^(mix)) + *(m2++); \
  *(m++) = y = ind(mm,x) + a + b; \
  *(r++) = b = ind(mm,y>>RANDSIZL) + x; \
}

void isaac(randctx *ctx)
{  register ub4 a,b,x,y,*m,*mm,*m2,*r,*mend;
   mm=ctx->randmem; r=ctx->randrsl;
   a = ctx->randa; b = ctx->randb + (++ctx->randc);
   for (m = mm, mend = m2 = m+(RANDSIZ/2); m<mend; )
   {  rngstep( a<<13, a, b, mm, m, m2, r, x);
      rngstep( a>>6 , a, b, mm, m, m2, r, x);
      rngstep( a<<2 , a, b, mm, m, m2, r, x);
      rngstep( a>>16, a, b, mm, m, m2, r, x);
   }
   for (m2 = mm; m2<mend; )
   {  rngstep( a<<13, a, b, mm, m, m2, r, x);
      rngstep( a>>6 , a, b, mm, m, m2, r, x);
      rngstep( a<<2 , a, b, mm, m, m2, r, x);
      rngstep( a>>16, a, b, mm, m, m2, r, x);
   }
   ctx->randb = b; ctx->randa = a;
}


#define mix(a,b,c,d,e,f,g,h) \
{ \
   a^=b<<11; d+=a; b+=c; \
   b^=c>>2;  e+=b; c+=d; \
   c^=d<<8;  f+=c; d+=e; \
   d^=e>>16; g+=d; e+=f; \
   e^=f<<10; h+=e; f+=g; \
   f^=g>>4;  a+=f; g+=h; \
   g^=h<<8;  b+=g; h+=a; \
   h^=a>>9;  c+=h; a+=b; \
}

/* if (flag==TRUE), then use the contents of randrsl[] to initialize mm[]. */
void randinit(randctx *ctx, int flag)
{
   int i;
   ub4 a,b,c,d,e,f,g,h;
   ub4 *m,*r;
   ctx->randa = ctx->randb = ctx->randc = 0;
   m=ctx->randmem;
   r=ctx->randrsl;
   a=b=c=d=e=f=g=h=0x9e3779b9;  /* the golden ratio */

   for (i=0; i<4; ++i)          /* scramble it */
   {
     mix(a,b,c,d,e,f,g,h);
   }

   if (flag) 
   {
     /* initialize using the contents of r[] as the seed */
     for (i=0; i<RANDSIZ; i+=8)
     {
       a+=r[i  ]; b+=r[i+1]; c+=r[i+2]; d+=r[i+3];
       e+=r[i+4]; f+=r[i+5]; g+=r[i+6]; h+=r[i+7];
       mix(a,b,c,d,e,f,g,h);
       m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
       m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
     }
     /* do a second pass to make all of the seed affect all of m */
     for (i=0; i<RANDSIZ; i+=8)
     {
       a+=m[i  ]; b+=m[i+1]; c+=m[i+2]; d+=m[i+3];
       e+=m[i+4]; f+=m[i+5]; g+=m[i+6]; h+=m[i+7];
       mix(a,b,c,d,e,f,g,h);
       m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
       m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
     }
   }
   else
   {
     /* fill in mm[] with messy stuff */
     for (i=0; i<RANDSIZ; i+=8)
     {
       mix(a,b,c,d,e,f,g,h);
       m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
       m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
     }
   }
}

/////////////////////////////////////////////////////////////////////////////
void randctx::init(tptr keyval, int keylen)
{// fill ctx->randrsl with sel_appl_uuid
  int space = sizeof(randrsl);
  tptr dest = (tptr)randrsl;
  while (space)
  { int mv = space < keylen ? space : keylen;
    memcpy(dest, keyval, mv);
    dest+=mv;  space-=mv;
  }
 // init the generator:
  randinit(this, 1);
  bytecnt=0;
}

void randctx::rand_encode(tptr buf, int size)
{ while (size--)
  { if (!bytecnt) { isaac(this);  bytecnt=sizeof(randrsl); }
    *buf ^= ((tptr)randrsl)[--bytecnt];
    buf++;
  }
}

struct t_uuid_num_key
{ WBUUID uuid;
  tobjnum objnum;
  t_uuid_num_key(WBUUID uuidIn, tobjnum objnumIn)
  { memcpy(uuid, uuidIn, UUID_SIZE);
    objnum=objnumIn;
  }
  int length(void) { return UUID_SIZE+sizeof(tobjnum); }
  tptr value(void) { return (tptr)uuid; }
};

CFNC DllKernel void WINAPI enc_buffer_total(cdp_t cdp, tptr buf, int size, tobjnum objnum)
{ randctx ctx;
#ifdef SRVR
  t_uuid_num_key key(header.server_uuid, objnum);
#else
  t_uuid_num_key key(cdp->server_uuid, objnum);
#endif  
  ctx.init(key.value(), key.length());
  ctx.rand_encode(buf, size);
}

#ifndef SRVR
#if !defined ODBCDRV || ! defined WINS
#define COPY_BUF_SIZE 4096

struct t_uuid_name_key
{ WBUUID uuid;
  tobjname name;
  t_uuid_name_key(WBUUID uuidIn, const char * nameIn)
  { memcpy(uuid, uuidIn, UUID_SIZE);
    strncpy(name, nameIn, OBJ_NAME_LEN);
    name[OBJ_NAME_LEN] = 0;
  }
  int length(void) { return UUID_SIZE+(int)strlen(name); }
  tptr value(void) { return (tptr)uuid; }
};

CFNC DllKernel BOOL WINAPI enc_copy_to_file(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr,
                           uns16 index, FHANDLE hFile, const char * objname)
// Encryption using sel_appl_uuid and objname, used when exporting appl in an encrypted way
/* copies var-len attribute & closes the file, returns FALSE on error */
{ tptr buf;  uns32 off, size32;  BOOL err=FALSE;  randctx ctx;  DWORD wr;
 // prepare the key
  t_uuid_name_key key(cdp->sel_appl_uuid, objname);
  ctx.init(key.value(), key.length());
  if (!(buf=sigalloc(COPY_BUF_SIZE, 99))) return FALSE;
#ifdef WINS
  HCURSOR oldcurs=SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif
  off=0;
  do
  { if (cd_Read_var(cdp, tb, recnum, attr, index, off, COPY_BUF_SIZE, buf, &size32))
    { 
#if WBVERS<900
      cd_Signalize(cdp);  
#endif
      err=TRUE;  break; 
    }
    ctx.rand_encode(buf, size32);
    if (!WriteFile(hFile, buf, size32, &wr, NULL))
      { /*syserr(FILE_WRITE_ERROR);*/  err=TRUE;  break; }
    off+=COPY_BUF_SIZE;
  } while (size32==COPY_BUF_SIZE);
  corefree(buf);
  CloseFile(hFile);
#ifdef WINS
  SetCursor(oldcurs);
#endif
  return !err;
}

CFNC DllKernel BOOL WINAPI enc2_copy_to_file(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr,
                           uns16 index, FHANDLE hFile, const char * objname)
// Encryption using sel_appl_uuid and objname, used when exporting ENCRYPTED appl in an encrypted way
/* copies var-len attribute & closes the file, returns FALSE on error */
{ tptr buf;  uns32 size32;  BOOL err=FALSE;  randctx ctx;  DWORD wr;
 // load and decrypt the object:
  cd_Read_len(cdp, tb, recnum, OBJ_DEF_ATR, NOINDEX, &size32);
  buf = cd_Load_objdef(cdp, recnum, tb==TAB_TABLENUM ? CATEG_TABLE : CATEG_PGMSRC, NULL);
  if (buf==NULL) 
  { 
#if WBVERS<900
    cd_Signalize(cdp);  
#endif
    return FALSE; 
  }
 // prepare the key
  t_uuid_name_key key(cdp->sel_appl_uuid, objname);
  ctx.init(key.value(), key.length());
#ifdef WINS
  HCURSOR oldcurs=SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif
  ctx.rand_encode(buf, size32);
  if (!WriteFile(hFile, buf, size32, &wr, NULL))
    { /*syserr(FILE_WRITE_ERROR);*/  err=TRUE; }
  corefree(buf);
  CloseFile(hFile);
#ifdef WINS
  SetCursor(oldcurs);
#endif
  return !err;
}

CFNC DllKernel void WINAPI enc_buf(WBUUID uuid, const char * objname, char *buf, int len)
// Encryption / decryption using appl_uuid and objname, used when exporting / importing appl in an encrypted way
{ randctx ctx;
  // prepare the key
  t_uuid_name_key key(uuid, objname);
  ctx.init(key.value(), key.length());
#ifdef WINS
  HCURSOR oldcurs=SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif
  ctx.rand_encode(buf, len);
#ifdef WINS
  SetCursor(oldcurs);
#endif
}

CFNC DllKernel tptr WINAPI enc_read_from_file(cdp_t cdp, FHANDLE hFile, WBUUID uuid, const char * objname, int * objsize)
{ tptr defin;  DWORD rd;
 // loading
  DWORD start = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);  // save pos
  DWORD flen  = SetFilePointer(hFile, 0, NULL, FILE_END);      // find end
  SetFilePointer(hFile, start, NULL, FILE_BEGIN);              // restore original pos
  if (flen==0xffffffff)  // size limit removed, big pictures possible
    { CloseFile(hFile);  return NULL; }
  flen-=start;
  defin=(tptr)corealloc(flen+1, 255);
  if (!defin)
    { client_error(cdp, OUT_OF_MEMORY);  CloseFile(hFile);  return NULL; }
  //defin=sigalloc(flen+1, 96);
  //if (!defin) { CloseFile(hFile);  return NULL; }
  if (!ReadFile(hFile, defin, flen, &rd, NULL) || rd!=(DWORD)flen)
    { CloseFile(hFile);  corefree(defin);  return NULL; }
  CloseFile(hFile);
  defin[flen]=0;
 // decrypting:
  if (objname)
  { t_uuid_name_key key(uuid, objname);  randctx ctx;  
    ctx.init(key.value(), key.length());
    ctx.rand_encode(defin, flen);
  }
  *objsize=flen;
  return defin;
}
// #endif
#endif
#endif
