// For compilers that support precompilation, includes "wx/wx.h".
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#include "wprez9.h"
#include "ierun9.h"
#include <stdlib.h>
#include <math.h>
#include <assert.h>

////////////////////////// DBF interface /////////////////////////////////////
#ifndef WINS
#define _ecvt ecvt
#define SQLExecDirectA SQLExecDirect
#endif

CFNC DllExport void WINAPI no_memory();

static void c4dtoa45(double doub_val, char *out_buffer, int len, int dec)
{  int         dec_val, sign_val ;
   int         pre_len, post_len, sign_pos ; /* pre and post decimal point lengths */
   char *result ;
   memset(out_buffer, (int) '0', (size_t) len) ;
   if (dec > 0)
   {  post_len =  dec ;
      if (post_len > 15)     post_len =  15 ;
      if (post_len > len-1)  post_len =  len-1 ;
      pre_len  =  len -post_len -1 ;
      out_buffer[ pre_len] = '.' ;
   }
   else  {  pre_len  =  len ;  post_len = 0 ;   }
   result =  fcvt( doub_val, post_len, &dec_val, &sign_val) ;
   if (dec_val > 0)
      sign_pos =   pre_len-dec_val -1 ;
   else
   {  sign_pos =   pre_len - 2 ;
		if ( pre_len == 1) sign_pos = 0 ;
   }
   if ( dec_val > pre_len ||  pre_len<0  ||  sign_pos< 0 && sign_val)
   {  memset( out_buffer, (int) '*', (size_t) len) ;
      return ;
   }
   if (dec_val > 0)
   {  memset( out_buffer, (int) ' ', (size_t) pre_len- dec_val) ;
      memmov( out_buffer+ pre_len- dec_val, result, (size_t) dec_val) ;
   }
   else
   {  if (pre_len> 0)  memset( out_buffer, (int) ' ', (size_t) (pre_len-1)) ;
   }
   if ( sign_val)  out_buffer[sign_pos] = '-' ;
   out_buffer += pre_len+1 ;
   if (dec_val >= 0)
     result+= dec_val ;
   else
   {  out_buffer    -= dec_val ;
      post_len += dec_val ;
   }
   if ( post_len > (int) strlen(result) )
      post_len =  (int) strlen( result) ;
   if (post_len > 0) memmov( out_buffer, result, (size_t) post_len) ;
}

static void c4ltoa45(sig32 l_val, char *ptr, int num)
{  int n, num_pos;   sig32  i_long;
   i_long =  (l_val>0) ? l_val : -l_val ;
   num_pos =  n =  (num > 0) ? num : -num ;
   while (n-- > 0)
   {  ptr[n] = (char) ('0'+ i_long%10) ;
      i_long = i_long/10 ;
   }
   if (i_long > 0)
     { memset(ptr, (int)'*', (size_t)num_pos);  return; }
   num--;
  // replace leading zeros by spaces:
   for (n=0;  n<num;  n++)
     if (ptr[n]=='0') ptr[n]= ' ';
     else break;
  // write minus sign:
   if (l_val < 0)
   { if (ptr[0] != ' ' )
       { memset(ptr, (int)'*', (size_t)num_pos);  return; }
     for (n=num;  n>=0;  n--)
       if (ptr[n]==' ')
         { ptr[n]= '-';  break; }
   }
}

static void c4ltoa45(sig64 l_val, char *ptr, int num)
{  int n, num_pos;   sig64  i_long;
   i_long =  (l_val>0) ? l_val : -l_val ;
   num_pos =  n =  (num > 0) ? num : -num ;
   while (n-- > 0)
   {  ptr[n] = (char) ('0'+ i_long%10) ;
      i_long = i_long/10 ;
   }
   if (i_long > 0)
     { memset(ptr, (int)'*', (size_t)num_pos);  return; }
   num--;
  // replace leading zeros by spaces:
   for (n=0;  n<num;  n++)
     if (ptr[n]=='0') ptr[n]= ' ';
     else break;
  // write minus sign:
   if (l_val < 0)
   { if (ptr[0] != ' ' )
       { memset(ptr, (int)'*', (size_t)num_pos);  return; }
     for (n=num;  n>=0;  n--)
       if (ptr[n]==' ')
         { ptr[n]= '-';  break; }
   }
}

static unsigned long rev4(unsigned long l)
{ return (l>>24) + ((l & 0x00ff0000L) >> 8) + ((l & 0x0000ff00L) << 8) +
         ((l & 0xff) << 24); }

static uns16 rev2(uns16 u)
{ return (uns16)((u >> 8) + ((u & 0xff) << 8)); }

static BOOL empty(tptr p, unsigned size)
{ while (size--) if (*p!=' ' && *p) return FALSE; else p++;
  return TRUE;
}

/////////////////////////////////// t_dbf_io ////////////////////////////////
#define MEMO_FIELD_LENGTH 10

bool t_dbf_io::rdinit(cdp_t cdp, char * pathname, trecnum * precnum)
{ DWORD rd;
 // read the header:
  if (!ReadFile(hnd, &hdr, sizeof(hdr), &rd, NULL) || rd!=sizeof(hdr))
    { client_error(cdp, OS_FILE_ERROR);  return false; }
  if (precnum!=NULL) *precnum=hdr.recnum;
 // open memo file:
  if (hdr.filetype & 0x80)
  { int len=(int)strlen(pathname);  char origsuff[3+1];
    if (pathname[--len]!='.') if (pathname[--len]!='.')
    if (pathname[--len]!='.') len--;  len++;
    strcpy(origsuff, pathname+len);
    strcpy(pathname+len, dbase ? "DBT" : "FPT");
    hnd2=CreateFileA(pathname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    strcpy(pathname+len, origsuff);
    if (hnd2==INVALID_FHANDLE_VALUE) 
      { client_error(cdp, MEMO_FILE_NOT_FOUND9);  return false; }
   /* find out the blocksize */
    if (!ReadFile(hnd2, rdbuf, 512, &rd, NULL) || 512 != rd)
      { client_error(cdp, OS_FILE_ERROR);  return false; }
    if (dbase) blocksize=((db_mm_hdr*)rdbuf)->block_size;
    else blocksize=rev2(((fox_mm_hdr*)rdbuf)->block_size);
  }
 // allocate recbuf & field descriptor:
  xattrs=(hdr.hdrsize-1) / 32 - 1;  /* it may not be exact, but shouldn't be less */
  xdef=(xadef*)sigalloc(xattrs*sizeof(xadef)+1, 99);  /* xbase fields definition */
  recbuf=sigalloc(hdr.recsize, 99);
  if (xdef==NULL || recbuf==NULL) 
    { client_error(cdp, OUT_OF_MEMORY);  return false; }
  if (!ReadFile(hnd, xdef, xattrs*sizeof(xadef)+1, &rd, NULL) || xattrs*sizeof(xadef)+1 != rd) 
    { client_error(cdp, OS_FILE_ERROR);  return false; }
 // calculate field offsets:
  int offset=1;
  for (int i=0;  i<xattrs;  i++)
  { xdef[i].offset=offset;
    offset+=xdef[i].size;
    xdef[i].type &= 0xdf;  // converted to uppercase
    // diacritics in the column name will be preserved, converting later on demand
  }
  SetFilePointer(hnd, hdr.hdrsize, NULL, FILE_BEGIN); // hdrsize may be slightly different!
  return true;
}

uns32 t_dbf_io::get_memo_size(tptr ptr)
// Positions the memo & returns its size
{ if (!empty(ptr, MEMO_FIELD_LENGTH) && hnd2!=INVALID_FHANDLE_VALUE)  /* no action if memo file not found */
  { uns32 lval=0;  DWORD rd;
    for (int i=0;  i<MEMO_FIELD_LENGTH;  i++)
      if (ptr[i]>='0' && ptr[i]<='9') lval=10*lval+ptr[i]-'0';
    if (SetFilePointer(hnd2, blocksize*lval, NULL, FILE_BEGIN) != 0xffffffff)
    { char mhdr[8];
      if (ReadFile(hnd2, mhdr, sizeof(mhdr), &rd, NULL) && rd==sizeof(mhdr))
        if (dbase)
          return ((db_m_hdr*)mhdr)->num_chars - ((db_m_hdr*)mhdr)->start_pos;
        else
          return rev4(((fox_m_hdr*)mhdr)->num_chars);
    }
  }
  return 0;
}

uns32 t_dbf_io::read_memo(tptr glob_atr_ptr, uns32 memo_size)
// Reads the positioned memo value
{ DWORD rd;
  if (!ReadFile(hnd2, glob_atr_ptr, memo_size, &rd, NULL)) rd=0;
  return rd;
}

BOOL t_dbf_io::wrinit(BOOL is_memo, int attrcnt, int recsizeIn, char * pathname)
{ DWORD wr;  uns32 dt;
  blocksize=0x200;
  if (recsizeIn>0xffff) return FALSE;  // DBF limit
 // create & write the header:
  memset(&hdr, 0, sizeof(hdr));
  hdr.filetype=(BYTE)(is_memo ? (dbase ? 0x83 : 0xf5) : 0x03);
  dt=Today();
  hdr.lastupdate[0]=Year(dt) % 100;
  hdr.lastupdate[1]=Month(dt);
  hdr.lastupdate[2]=Day(dt);
  hdr.hdrsize =(uns16)(sizeof(hdr) + attrcnt * sizeof(xadef)+1);
  hdr.recsize =recsizeIn;
  if (!WriteFile(hnd, &hdr, sizeof(dbf_hdr), &wr, NULL) || sizeof(dbf_hdr)!=wr)
    err=TRUE;
 // allocate recbuf:
  recbuf=sigalloc(recsizeIn, 99);
  if (recbuf==NULL) err=TRUE;
  else recbuf[0]=' ';  // constant
 // create & init the memo file:
  if (is_memo)
  { int len=(int)strlen(pathname);  char origsuff[3+1];
    if (pathname[--len]!='.') if (pathname[--len]!='.')
    if (pathname[--len]!='.') len--;  len++;
    strcpy(origsuff, pathname+len);
    strcpy(pathname+len, dbase ? "DBT" : "FPT");
    hnd2=CreateFileA(pathname, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    strcpy(pathname+len, origsuff);
    if (hnd2==INVALID_FHANDLE_VALUE) err=TRUE;
    else /* write the initial block */
    { memset(rdbuf, 0, blocksize);
      if (!WriteFile(hnd2, rdbuf, blocksize, &wr, NULL) || wr!=blocksize)
        err=TRUE;
      next_free_block=1;  /* npos, tpos remain valid!! */
    }
  }
 // store the basename:
  char shortname[MAX_PATH];
#ifdef WINS  
  GetShortPathNameA(pathname, shortname, sizeof(shortname));
#else
  strcpy(shortname, pathname);
#endif    
  int i,j;
  i=(int)strlen(shortname);
  while (i && shortname[i-1]!=PATH_SEPARATOR) i--;
  j=0;
  while (shortname[i+j] && shortname[i+j]!='.') j++;
  if (j>sizeof(basename)-1) j=sizeof(basename)-1;
  memcpy(basename, shortname+i, j);  basename[j]=0;
  return !err;
}

BOOL t_dbf_io::add_field(tptr name, char type, int size, int dec, int field_offset)
// [name] is in the encoding of the DBF file.
{ DWORD wr;
  if (name!=NULL)
  { xadef xdef;
    memset(&xdef, 0, sizeof(xdef));
    strmaxcpy(xdef.name, name, sizeof(xdef.name));
    xdef.type=type;
    xdef.res1=field_offset;  // only FoxPro requires this
    xdef.size=type=='M' ? MEMO_FIELD_LENGTH : size;
    xdef.dec =dec;
    return WriteFile(hnd, &xdef, sizeof(xdef), &wr, NULL) && wr==sizeof(xdef);
  }
  else // write delimiter
  { char delim = '\r';
    return WriteFile(hnd, &delim, 1,  &wr, NULL) && wr==1;
  }
}

BOOL t_dbf_io::write_memo(tptr satrval, unsigned satrvallen, tptr DBF_ptr, int is_text)
{ if (!satrvallen) memset(DBF_ptr, ' ', MEMO_FIELD_LENGTH);
  else
  { c4ltoa45((sig32)next_free_block, DBF_ptr, MEMO_FIELD_LENGTH);
    char mhdr[8];
    memset(mhdr, 0, sizeof(mhdr));
    if (dbase)
    { ((db_m_hdr *)mhdr)->minus_one=-1;
      ((db_m_hdr *)mhdr)->start_pos=8;
      ((db_m_hdr *)mhdr)->num_chars=8+satrvallen;
    }
    else
    { ((fox_m_hdr*)mhdr)->type     =rev4(is_text);
      ((fox_m_hdr*)mhdr)->num_chars=rev4(satrvallen);
    }
    DWORD wr;
    if (!WriteFile(hnd2, mhdr, sizeof(mhdr),  &wr, NULL) || wr!=sizeof(mhdr))
      err=TRUE;
    if (!WriteFile(hnd2, satrval, satrvallen, &wr, NULL) || wr!=satrvallen)
      err=TRUE;
    satrvallen+=sizeof(mhdr);  // written
    next_free_block += satrvallen / blocksize;
    if (satrvallen % blocksize)
    { unsigned padding = blocksize - satrvallen % blocksize;
      memset(rdbuf, 0, padding);
      if (!WriteFile(hnd2, rdbuf, padding, &wr, NULL) || wr!=padding) err=TRUE;
      next_free_block++;
    }
  }
  return !err;
}

BOOL t_dbf_io::wrclose(trecnum recnum)
{ DWORD wr;
 // write memo-file header:
  if (hnd2!=INVALID_FHANDLE_VALUE) /* update the memofile starblock */
  { memset(rdbuf, 0, blocksize);
    if (dbase)
    { ((db_mm_hdr *)rdbuf)->next_block=next_free_block;
      ((db_mm_hdr *)rdbuf)->x102=0x102;
	    ((db_mm_hdr *)rdbuf)->block_size=(short)blocksize;
      memcpy(((db_mm_hdr*)rdbuf)->file_name, basename, sizeof(((db_mm_hdr*)rdbuf)->file_name));
    }
    else
    { ((fox_mm_hdr*)rdbuf)->next_block=rev4(next_free_block);
	    ((fox_mm_hdr*)rdbuf)->block_size=rev2((uns16)blocksize);
    }
    SetFilePointer(hnd2, 0, NULL, FILE_BEGIN);
    if (!WriteFile(hnd2, rdbuf, blocksize, &wr, NULL) || wr!=blocksize)
      err=TRUE;
  }
 // write DBF file delimiter:
  char file_delim=0x1a;
  if (WriteFile(hnd, &file_delim, 1, &wr, NULL) || wr!=1) err=TRUE;
 // update the dbf-header (write total record number):
  hdr.recnum=recnum;  /* dbf-header to be updated */
  SetFilePointer(hnd, 0, NULL, FILE_BEGIN);
  if (!WriteFile(hnd, &hdr, sizeof(dbf_hdr), &wr, NULL) || wr!=sizeof(dbf_hdr)) err=TRUE;
  return !err;
}

BOOL t_dbf_io::read(void)
{ DWORD rd;
  if (!ReadFile(hnd, recbuf, hdr.recsize, &rd, NULL) || rd!=hdr.recsize)
    { err=TRUE;  return FALSE; }
  return TRUE;
}

BOOL t_dbf_io::write(void)
{ DWORD wr;
  if (!WriteFile(hnd, recbuf, hdr.recsize, &wr, NULL) || wr!=hdr.recsize)
    { err=TRUE;  return FALSE; }
  return TRUE;
}

///////////////////////////////// line input ////////////////////////////////
// get_line reads & returns lines from hnd. Returns NULL on EOF or on error.
// was_error informs about accumulated error.
// Errors: cannot allocate buffer as big as input line is, read error.
// Returns last line not delimited by \n if non-empty.

#ifdef STOP // does not recognize CR as the end of line
tptr t_lineinp::get_line(unsigned * plinelen)
{ if (err) return NULL;
  unsigned p=nextline;
  while (p<bufcont && buf[p]!='\n') p++;
  if (p==bufcont)  // eoln not found
  {// remove previous lines from the buffer:
    memmov(buf, buf+nextline, bufcont-nextline);
    bufcont-=nextline;  p-=nextline;  nextline=0;
    do
    {// read the rest of the buffer:
      DWORD rd;
      if (!ReadFile(hnd, buf+bufcont, bufsize-bufcont, &rd, NULL))
        { err=TRUE;  return NULL; }
      if (!rd)
      { if (p==nextline) return NULL;  // empty line
       // last line not delimited by \n:
        linestart=nextline;
        nextline=p;
        if (p && buf[p-1]=='\r') buf[--p]=0;
        if (p==linestart) return NULL;  // CR only on the line
        *plinelen=p-linestart;
        return buf+linestart;
      }
      bufcont+=rd;
      while (p<bufcont && buf[p]!='\n') p++;
      if (buf[p]=='\n') break; // eoln found
     // grow buffer:
      buf=(tptr)corerealloc(buf, bufsize+3000);
      if (buf==NULL) { err=TRUE;  return NULL; }
      bufsize+=3000;
    } while (TRUE);
  }
 // eoln at p:
  linestart=nextline;
  nextline=p+1;
  buf[p]=0;
  if (p && buf[p-1]=='\r') buf[--p]=0;
  *plinelen=p-linestart;
  return buf+linestart;
}
#endif

tptr t_lineinp::get_line(unsigned * plinelen)
// Returns line start pointer and *plinelen in chars. Returns NULL if input exhausted.
{ if (err) return NULL;
  unsigned p=nextline;
  bool was_cr = false;
  do
  { 
    switch (charsize)
    { case 1:
        if (lfonly)
        { while (p<bufcont && buf[p]!='\n') p++;
          if (p<bufcont) // eoln found
          { linestart=nextline;
            nextline=p+1;
            buf[p]=0;
            if (p && buf[p-1]=='\r') buf[--p]=0;
            goto done; 
          }
        }
        else // look for LF or 1st char following CR(s)
        { while (p<bufcont && buf[p]!='\n') 
          { if (buf[p]=='\r') was_cr=TRUE;
            else if (was_cr) break;
            p++;
          }
          if (p<bufcont) // eoln found
          { linestart=nextline;
            if (buf[p]=='\n') { nextline=p+1;  buf[p]=0; }
            else nextline=p;
            while (p && buf[p-1]=='\r') buf[--p]=0;
            goto done; 
          }
        }
        break;
     case 2:
        if (lfonly)
        { while (p<bufcont && buf2[p]!='\n') p++;
          if (p<bufcont) // eoln found
          { linestart=nextline;
            nextline=p+1;
            buf2[p]=0;
            if (p && buf2[p-1]=='\r') buf2[--p]=0;
            goto done; 
          }
        }
        else // look for LF or 1st char following CR(s)
        { while (p<bufcont && buf2[p]!='\n') 
          { if (buf2[p]=='\r') was_cr=TRUE;
            else if (was_cr) break;
            p++;
          }
          if (p<bufcont) // eoln found
          { linestart=nextline;
            if (buf2[p]=='\n') { nextline=p+1;  buf2[p]=0; }
            else nextline=p;
            while (p && buf2[p-1]=='\r') buf2[--p]=0;
            goto done; 
          }
        }
        break;
     case 4:
        if (lfonly)
        { while (p<bufcont && buf4[p]!='\n') p++;
          if (p<bufcont) // eoln found
          { linestart=nextline;
            nextline=p+1;
            buf4[p]=0;
            if (p && buf4[p-1]=='\r') buf4[--p]=0;
            goto done; 
          }
        }
        else // look for LF or 1st char following CR(s)
        { while (p<bufcont && buf4[p]!='\n') 
          { if (buf4[p]=='\r') was_cr=TRUE;
            else if (was_cr) break;
            p++;
          }
          if (p<bufcont) // eoln found
          { linestart=nextline;
            if (buf4[p]=='\n') { nextline=p+1;  buf4[p]=0; }
            else nextline=p;
            while (p && buf4[p-1]=='\r') buf4[--p]=0;
            goto done; 
          }
        }
        break;
    } // switch
   // make space for more input characters: 
    if (nextline) // remove previous lines from the buffer:
    { memmov(buf, buf+nextline, charsize*(bufcont-nextline));
      bufcont-=nextline;  p-=nextline;  nextline=0;
    }
    else // single line in the buffer, must expand it
    { buf=(tptr)corerealloc(buf, charsize*(2*bufsize));
      buf2=(uns16*)buf;  buf4=(uns32*)buf;
      if (buf==NULL) { err=true;  return NULL; }
      bufsize*=2;
    }
   // read the input upto the rest of the buffer:
    DWORD rd;
    if (!ReadFile(hnd, buf+charsize*bufcont, charsize*(bufsize-bufcont), &rd, NULL))
      { err=true;  return NULL; }
    bufcont+=rd/charsize;
    if (!rd) // end of input
    { if (p==nextline) return NULL;  // empty line
     // last line is not properly delimited:
      linestart=nextline;
      nextline=p;
      switch (charsize)
      { case 1:
          while (p && buf [p-1]=='\r') buf [--p]=0;  break;
        case 2:
          while (p && buf2[p-1]=='\r') buf2[--p]=0;  break;
        case 4:
          while (p && buf4[p-1]=='\r') buf4[--p]=0;  break;
      }
      if (p==linestart) return NULL;  // CR only on the line
      break;
    }
  } while (true);
 done:
 // eoln at p:
  *plinelen=p-linestart;
  return buf+charsize*linestart;
}

/////////////////////////////////// t_ie_run ////////////////////////////////
t_ie_run::t_ie_run(cdp_t cdpIn, int silentIn) : dd()
{ cdp=cdpIn;  
  silent = silentIn!=0;
  overwrite_existing_target_file = silentIn==2;
  source_type=dest_type=-1;  index_past=TRUE;
  csv_separ=',';  csv_quote='\"';  add_header=TRUE;  skip_lines=1;
  *inserver=*outserver=*inschema=*outschema=0;
  sxcdp=NULL;  txcdp=NULL;
  semilog=FALSE;  lfonly=TRUE;
  decim_separ='.';
  strcpy(date_form,      "DD.MM.CCYY");
  strcpy(time_form,      "h.mm.ss.fff");
  strcpy(timestamp_form, "DD.MM.CCYY h.mm.ss");
  strcpy(boolean_form,   "False,True");
  src_recode=dest_recode=  // the default encoding
#ifdef LINUX
    CHARSET_ISO88591;
#else
    CHARSET_CP1250;
#endif
  cond[0]=0;
  colmap=NULL;  src_colmap=NULL;
}

void t_ie_run::release_colmaps(void)
{ if (colmap)     safefree((void**)&colmap);
  if (src_colmap) safefree((void**)&src_colmap);
}

BOOL t_ie_run::wb2wb_trasport(void)
{ int i;  bool first;
  t_flstr stmt(1000, 1000);
  if (in_obj == out_obj)  // in-place operation
  { stmt.put("UPDATE ");
    make_full_table_name(sxcdp, stmt, inpath, inschema);
    stmt.put(" SET ");
    first=true;
    for (i=0;  i<dd.count();  i++)
    { tdetail * det=dd.acc0(i);
      if (*det->destin && *det->source) // unless line empty
      { if (first) first=false;
        else stmt.put(",");
        stmt.putqq(det->destin);
        stmt.put("=");
        stmt.put(det->source);  // [source] may be an expression: must not enclose it in `...`
      }
    }
  }
  else // moving data
  { stmt.put("INSERT INTO ");
    make_full_table_name(txcdp, stmt, outpath, outschema);
    stmt.put(" (");
    first=true;
    for (i=0;  i<dd.count();  i++)
    { tdetail * det=dd.acc0(i);
      if (*det->destin && *det->source) // unless line empty
      { if (first) first=false;
        else stmt.put(",");
        stmt.putqq(det->destin);
      }
    }
    stmt.put(") SELECT ");
    first=true;
    for (i=0;  i<dd.count();  i++)
    { tdetail * det=dd.acc0(i);
      if (*det->destin && *det->source)
      { if (first) first=false;
        else stmt.put(",");
        stmt.put(det->source);  // [source] may be an expression: must not enclose it in `...`
      }
    }
    stmt.put(" FROM ");
    make_full_table_name(sxcdp, stmt, inpath, inschema);
  }
  if (*cutspaces(cond))
  { stmt.put(" WHERE ");
    stmt.put(cond);
  }

 // transport it:
  uns32 count = 0;
  if (cd_SQL_execute(sxcdp->get_cdp(), stmt.get(), &count))
    return FALSE;
  client_status_nums(count, NORECNUM);
  return TRUE;
}

tobjnum create_table(xcdp_t xcdp, table_all * pta)
{ if (xcdp->connection_type==CONN_TP_602)
  { trecnum tablerec;
    tptr src=table_all_to_source(pta, FALSE);
    if (!src) return NOOBJECT;
    if (cd_SQL_execute(xcdp->get_cdp(), src, &tablerec))
      { corefree(src);  return NOOBJECT; }
    corefree(src);
    return tablerec;  // the caller must add the table to the control panel - when creates_target_table() returns true
  }
  else  // ODBC table: create_table for odbc tables not implemented
    return NOOBJECT;
}

void t_ie_run::run_clear(void)
// Frees all objects and handles allocated by do_transport.
// Preserve error number (Close_cursor, Enable_index may clear it)
{ int errnum=cd_Sz_error(cdp);
  if (inhnd  !=INVALID_FHANDLE_VALUE) CloseFile(inhnd);
  if (outhnd !=INVALID_FHANDLE_VALUE) CloseFile(outhnd);
  if (linp!=NULL) delete linp;
  if (dbf_inp!=NULL) delete dbf_inp;
  if (dbf_out!=NULL) delete dbf_out;
  if (source_dt!=NULL) { destroy_cached_access(source_dt);  source_dt=NULL; }
  if (dest_dt  !=NULL) { destroy_cached_access(dest_dt);    dest_dt  =NULL; }
  if (index_disabled)
    cd_Enable_index(txcdp->get_cdp(), out_obj, -1, TRUE);  // key duplicity error may be obscured by a previuos data error
  if (errnum) 
  { cdp->RV.report_number=errnum;  // restore the original error number (generic message are supposed to be intact)
    cdp->last_error_is_from_client=true;
  }
  release_colmaps();
}

#ifdef WINS
#define MAX_SIG64 0x7fffffffffffffffi64
#define MIN_SIG64 0x8000000000000000i64
#define MAX_SIG48 0x7fffffffffffi64
#define MIN_SIG48 0x800000000000i64
#else
#define MAX_SIG64 0x7fffffffffffffffll
#define MIN_SIG64 0x8000000000000000ll
#define MAX_SIG48 0x7fffffffffffll
#define MIN_SIG48 0x800000000000ll
#endif

static char DBF_type_char[6] = { 'C', 'N', 'F', 'L', 'D', 'M' };
static uns8 DBF_conv_type[6] =
 { ATT_STRING, ATT_FLOAT, ATT_FLOAT, ATT_BOOLEAN, ATT_DATE, ATT_STRING };

inline uns32 LINECHAR(const char * line, int index, int recode)
{ return recode==CHARSET_UCS4 ? ((uns32*)line)[index] : 
         recode==CHARSET_UCS2 ? ((uns16*)line)[index] : ((uns8*)line)[index]; 
}

inline uns32 * LINESET(const char * line, int index, int recode)
{ return recode==CHARSET_UCS4 ? (uns32*)(line+4*index) : 
         recode==CHARSET_UCS2 ? (uns32*)(line+2*index) : (uns32*)(line+index); 
}

void t_ie_run::analyse_cols_in_line(const char * srcline, int slinelen, int recode)
// Called for the column source format on the 1st line. Finds the positions and sizes of all columns 
// specified in [src_colmap] and writes it to [srcoff] and [srcsize].
// Not for CSV!
{ if (!srcline) return;  // input is empty
  int offset=0;
  for (int i=0;  i<=max_src_col;  i++)
  {// find the size of the next column: 
    int colsize=0;
    while (offset+colsize < slinelen && LINECHAR(srcline, offset+colsize, recode) > ' ') colsize++;
    while (offset+colsize < slinelen && LINECHAR(srcline, offset+colsize, recode) <=' ') colsize++;
    tattrib ind = src_colmap[i];
    if (ind!=NOATTRIB)
    { tdetail * det = dd.acc(i);  
      det->srcoff=offset;  det->srcsize=colsize;  // in chars!
    }
    offset+=colsize;
  }
}

void init_cache_dynars(ltable * dt)
// The main function is to set NULL to all values
{ int atr;  atr_info * catr;
  tptr p = dt->cache_ptr;
  for (atr=1, catr=dt->attrs+1;  atr<dt->attrcnt;  atr++, catr++)
    if (catr->flags & CA_MULTIATTR)
    { t_dynar * dy = (t_dynar*)(p+catr->offset);
      dy->init(catr->flags & CA_INDIRECT ? sizeof(indir_obj) : catr->size);
    }
    else if (!(catr->flags & CA_INDIRECT))
      catr_set_null(p+catr->offset, catr->type);
}

int t_ie_run::convert_column_names_to_numbers(cdp_t cdp, t_dbf_io * dbf_inp, t_specif s_specif)
// Searches for source column names [source] among the DBF column names.
// Returns dd index+1 when a column name is not found in [dbf_inp]. Returns 0 if OK.
{ int err_col = 0;
  for (int i=0;  i<dd.count();  i++)
  { tdetail * det = dd.acc0(i);  
    if (*det->source) // find and add the attribute name
    { int j;
      for (j=0;  j<dbf_inp->xattrs;  j++)
      { tobjname decoded_column_name;
        superconv(ATT_STRING, ATT_STRING, dbf_inp->xdef[j].name, decoded_column_name, s_specif, t_specif(0, cdp ? cdp->sys_charset : 0, 0, 0), NULLSTRING);
        if (!wb2_stricmp(cdp ? cdp->sys_charset : 0, det->source, decoded_column_name))
          break;
      }
      if (j==dbf_inp->xattrs) err_col=i+1; // not found -> error
      det->sattr = j<dbf_inp->xattrs ? j+1 : 0;  // 0 if not found
    }
  }
  return err_col;
}

DllExport t_specif WINAPI code2specif(int src_recode)
{ t_specif specif;
  switch (src_recode)
  { case CHARSET_UCS4: specif.wide_char=2;  break;
    case CHARSET_UCS2: specif.wide_char=1;  break;
    case CHARSET_CP1250:     specif.charset=1;      break;
    case CHARSET_CP1252:     specif.charset=3;      break;
    case CHARSET_ISO646:     specif.charset=0;      break;
    case CHARSET_ISO88592:   specif.charset=128+1;  break;
    case CHARSET_IBM852:     specif.charset=CHARSET_NUM_IBM852;  break;
    case CHARSET_ISO88591:   specif.charset=128+3;  break;
    case CHARSET_UTF8:       specif.charset=CHARSET_NUM_UTF8;    break;
  }
  return specif;
}

bool t_ie_run::analyse_dsgn(void)
// Defines [in_obj], [out_obj], [creating_target].
{ bool res=true;
 // find source: 
  in_obj=NOOBJECT;
  if (sxcdp && sxcdp->get_cdp() && source_type==IETYPE_WB)
  { if (cd_Find_prefixed_object(sxcdp->get_cdp(), inschema, inpath, CATEG_TABLE,  &in_obj))
      res=false;
  }
  else if (sxcdp && sxcdp->get_cdp() && source_type==IETYPE_CURSOR)
    if (cd_Find_prefixed_object(sxcdp->get_cdp(), inschema, inpath, CATEG_CURSOR, &in_obj))
      res=false; 
 // find target:
  out_obj=NOOBJECT;
  if (dest_type==IETYPE_WB)
  { if (txcdp && txcdp->get_cdp() && !cd_Find_prefixed_object(txcdp->get_cdp(), outschema, outpath, CATEG_TABLE, &out_obj))
      creating_target=false;  // existing target table
    else
      creating_target=true; 
  }
  else if (dest_type==IETYPE_ODBC)
    creating_target=false;   // createing not supported now
  else  // never append data to existing external files, always create
    creating_target=true;
  dest_exists=!creating_target;
  return res;
}

bool t_ie_run::allocate_colmaps(void)
// Computes max_src_col and colmapstop, allocates src_colmap and colmap.
{ int i;  tdetail * det;
  if (source_type==IETYPE_COLS || source_type==IETYPE_CSV)
  { max_src_col=0;
    int last_col=0;
    for (i=0;  i<dd.count();  i++)
    { det=dd.acc0(i);  sig32 val;
      if (!*det->source) 
      { if (*det->destin) // non-emty line
        { val=++last_col;  // compatibility with version 8.1
          int2str(val, det->source);  // source must not be empty in order the import to work
        }
        else val=-1;  
      }  
      else str2int(det->source, &val);
      if (val>0 && val<MAX_TABLE_COLUMNS)
        if (val-1 > max_src_col) max_src_col=val-1;
    }
    src_colmap=(tattrib*)corealloc(sizeof(tattrib)*(max_src_col+1), 45);
    if (!src_colmap) return false;
    memset(src_colmap, 0xff, sizeof(tattrib)*(max_src_col+1));
  }

 // find attribute transport order (if not above), compute colmapstop:
  colmapstop=0;
  if (dest_type==IETYPE_CSV || dest_type==IETYPE_COLS)
  { for (i=0;  i<dd.count();  i++)
    { det=dd.acc0(i);  sig32 val;
      str2int(det->destin, &val);
      if (val>0 && val<MAX_TABLE_COLUMNS)
        if (val > colmapstop) colmapstop=val;  // the colmap index is val-1, colmapstop is defined as max index + 1
    }
  }
  else  // just count the defined columns
  { for (i=0;  i<dd.count();  i++)
    { det=dd.acc0(i);
      if (*det->destin || *det->source)  // line non-empty
        colmapstop++;
    }
  }
  colmap=(tattrib*)corealloc(sizeof(tattrib)*colmapstop + 1, 45);  // +1 just prevents error when colmapstop is 0
  if (!colmap) return false;
  memset(colmap, 0xff, sizeof(tattrib)*colmapstop);
  return true;
}

int find_upcase_column_in_td(const d_table * td, const char * name)
{ int i;  const d_attr * pattr;
  
  if (strlen(name) > OBJ_NAME_LEN)  // ODBC
  { tptr src = (tptr)(td->attribs+td->attrcnt);
    if (!*src) return -1;  // extended names not specified
    for (i=1;  i < td->attrcnt;  src+=strlen(src)+1, i++)
      if (!stricmp(src, name))
        return i;
  }
  else   /* WB table or cursor: */
    for (i=0, pattr=FIRST_ATTR(td);  i<td->attrcnt;  i++, pattr=NEXT_ATTR(td,pattr))
      if (!strcmp(pattr->name, name))
        return i;
  return -1;
}

BOOL t_ie_run::do_transport(void)  
// May exit in any place without releasing objects. run_clear called after this.
// Supposes that in_obj, out_obj, dest_exists is defined!
// Supposes find_servers has been called before and returned true.
{ BOOL outmemo;  trecnum src_recnum; // WB, CURSOR, DBF
  unsigned slinelen;  tptr srcline;  bool have_line;  // necessary for analysing COLS header
  //t_connection * dest_conn;  
 // init handles:
  outhnd=inhnd=INVALID_FHANDLE_VALUE;  incurs=NOOBJECT;
  linp=NULL;  dbf_inp=dbf_out=NULL;  source_dt=dest_dt=NULL;
  index_disabled=FALSE;
 // check sxcdp, txcdp:
  if (!text_target() && !dbf_target() && dest_type!=IETYPE_TDT && !txcdp || !text_source() && !dbf_source() && source_type!=IETYPE_TDT && !sxcdp)
    return FALSE;
 // disable destination index, if required
  if (dest_exists && dest_type==IETYPE_WB && index_past)
  { cd_Enable_index(txcdp->get_cdp(), out_obj, -1, FALSE);
    index_disabled=TRUE;
  }

 /////////////////////// special cases: TDT format //////////////////////////
  if (source_type==IETYPE_TDT)
  { if (dest_type!=IETYPE_WB) return FALSE;
    if (!dest_exists)
      { SET_ERR_MSG(cdp, outpath);  client_error(cdp, OBJECT_NOT_FOUND);  return FALSE; }
    FHANDLE hFile=CreateFileA(inpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_FHANDLE_VALUE)
      { client_error(cdp, OS_FILE_ERROR);  return FALSE; }
    BOOL res=!cd_Restore_table(txcdp->get_cdp(), out_obj, hFile, FALSE);
    CloseFile(hFile);
    return res;
  }

  if (dest_type==IETYPE_TDT/* || dest_type==IETYPE_MAIL*/)  // !!! mail dodelat
  { if (source_type!=IETYPE_WB && source_type!=IETYPE_CURSOR) return FALSE;
   // create destination file: 
    outhnd=CreateFileA(outpath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
    if (outhnd==INVALID_FHANDLE_VALUE)
    { 
#ifdef STOP  // cannot ask here, must be outside
      if (!overwrite_existing_target_file && !silent)
        if (!can_overwrite_file(outpath, NULL))
          return FALSE;  // no additional error message is overwrite not confirmed
      if (overwrite_existing_target_file || !silent)
#endif
        outhnd=CreateFileA(outpath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
      if (outhnd==INVALID_FHANDLE_VALUE)
        { client_error(cdp, OS_FILE_ERROR);  return FALSE; }
    }

    BOOL res = TRUE;  tobjnum tbnum;
    if (source_type==IETYPE_CURSOR)
    { if (cd_Open_cursor(sxcdp->get_cdp(), in_obj, &incurs)) res=false;
      else tbnum=incurs;
    }
    else tbnum=in_obj;
    if (res) res=!cd_xSave_table(sxcdp->get_cdp(), tbnum, outhnd);
    CloseFile(outhnd);   outhnd=INVALID_FHANDLE_VALUE;
    return res;
  }
 ////////////////////////// analyse design ///////////////////////////////////
  int i;  tdetail * det;
  t_specif s_specif;  t_specif d_specif;  int src_charsize, dest_charsize;
  t_file_bufwrite fbw;
  uns32 csv_separ_wide=csv_separ, csv_quote_wide=csv_quote, space_char_wide=' ';

  if (!allocate_colmaps())
    { client_error(cdp, OS_FILE_ERROR);  return FALSE; }
 // create source column map:
  if (source_type==IETYPE_COLS || source_type==IETYPE_CSV)
  { int last_col=0;
    for (i=0;  i<dd.count();  i++)
    { det=dd.acc0(i);  sig32 val;
      if (!*det->source) 
        if (*det->destin) // non-emty line
          val=++last_col;  // compatibility with version 8.1
        else val=-1;  
      else str2int(det->source, &val);
      if (val>0 && val<MAX_TABLE_COLUMNS)
      { det->sattr=(tattrib)val;
        src_colmap[val-1]=(tattrib)i;
      }
    }
  }

 // find attribute transport order (if not above), compute colmapstop:
  if (dest_type==IETYPE_CSV || dest_type==IETYPE_COLS)
  { for (i=0;  i<dd.count();  i++)
    { det=dd.acc0(i);  sig32 val;
      str2int(det->destin, &val);
      if (val>0 && val<MAX_TABLE_COLUMNS)
      { det->dattr=(tattrib)val;
        colmap[val-1]=(tattrib)i;
      }
    }
  }
  else
  { int counter=0;
    for (i=0;  i<dd.count();  i++)
    { det=dd.acc0(i);
      if (*det->destin || *det->source)  // line non-empty
      { colmap[counter]=(tattrib)i;
        if (dbf_target()) det->dattr=counter;  // dest does not exist, will be created in this order
        counter++;
      }
    }
  }

 // find destination column positions and output record size:
  if (dest_type==IETYPE_COLS || dbf_target())
  { int offset=dest_type==IETYPE_COLS ? 0 : 1;
    for (i=0;  i<colmapstop;  i++)
      if (colmap[i]!=NOATTRIB)  // column is used
      { det=dd.acc(colmap[i]);
        det->destoff=offset;
        offset+=det->desttype==DBFTYPE_MEMO && dbf_target() ? MEMO_FIELD_LENGTH : det->destlen1;
      }
    destrecsize=offset;
  }

 ////////////////////////////////////// prepare source: //////////////////////////////////////
 // Creates cursor access for WB and ODBC, opend file and analyses the header for other formats.
 // for text source, stay on the 1st line - used when generating column names for a text target
 // Makes the full source type information available for creating the target table.
  if (db_source() || odbc_source())
  { bool input_cursor_opened = false;
    if (*inpath)
    { t_flstr cdef(1000, 1000);  
      cdef.put("SELECT ");
      int counter=0;
      for (i=0;  i<dd.count();  i++)
      { det=dd.acc0(i);
        if (*det->source)
        { if (counter) cdef.put(",");
          cdef.put(det->source);
          det->sattr=++counter;  // counter from 1
        }
      }
      cdef.put(" FROM ");
      make_full_table_name(sxcdp, cdef, inpath, inschema);
      if (*cutspaces(cond))
      { cdef.put(" WHERE ");
        cdef.put(cond);
      }
      if (db_source())
      { if (cd_Open_cursor_direct(sxcdp->get_cdp(), cdef.get(), &incurs))
          return FALSE;  
        input_cursor_opened=true;  // must give its ownership to source_dt or close it
      }
      else
      { HSTMT hStmt;  SQLRETURN result;  
        result = create_odbc_statement(sxcdp->get_odbcconn(), &hStmt);
        if (result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO) 
        { result=SQLExecDirectA(hStmt, (uns8*)cdef.get(), SQL_NTS);
          if (result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO || result==SQL_NO_DATA)
            incurs = (tcursnum)(size_t)hStmt;
          else 
          { odbc_error(sxcdp->get_odbcconn(), hStmt);
            pass_error_info(sxcdp->get_odbcconn(), cdp);
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false; 
          }
        }
        else 
        { odbc_error(sxcdp->get_odbcconn(), 0);  
          pass_error_info(sxcdp->get_odbcconn(), cdp);
          return FALSE; 
        }
        input_cursor_opened=true;  // must give its ownership to source_dt or close it
      }
    }
    else  // direct cursor
      incurs=in_obj;
    if (db_source())
    { if (cd_Rec_cnt(sxcdp->get_cdp(), incurs, &src_recnum))
      { if (input_cursor_opened) cd_Close_cursor(sxcdp->get_cdp(), incurs);
        return FALSE;
      }
      s_specif.charset = sxcdp->get_cdp()->sys_charset;  // used when creating the header line and converting the column names
      source_dt=create_cached_access9(sxcdp->get_cdp(), NULL, NULL, NULL, NULL, incurs);
      if (source_dt==NULL) 
      { if (input_cursor_opened) cd_Close_cursor(sxcdp->get_cdp(), incurs);
        return FALSE;
      }
      source_dt->close_cursor=TRUE;  // incurs owned by source_dt now, do not close it
    }
    else
    { const d_table * td = make_result_descriptor9(sxcdp->get_odbcconn(), (HSTMT)(size_t)incurs);
      source_dt=create_cached_access_for_stmt(sxcdp->get_odbcconn(), (HSTMT)(size_t)incurs, td);   // source_dt will co-own the [td]
      if (source_dt==NULL) 
      { if (input_cursor_opened) SQLFreeHandle(SQL_HANDLE_STMT, (HSTMT)(size_t)incurs);
        return FALSE;
      }
      // incurs owned by source_dt now, do not close it
      src_recnum = NORECNUM;  // must be a big value, NORECNUM prevents displaying the count in the status line
    }
  }
  else // source in a file
  { s_specif = code2specif(src_recode);
    src_charsize = src_recode==CHARSET_UCS4 ? 4 : src_recode==CHARSET_UCS2 ? 2 : 1;
    if (src_charsize==1 && !charset_available(s_specif.charset))
      { SET_ERR_MSG(cdp, "<source file charset>");  client_error(cdp, STRING_CONV_NOT_AVAIL);  return FALSE; }
    inhnd=CreateFileA(inpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (inhnd==INVALID_FHANDLE_VALUE)
      { client_error(cdp, OS_FILE_ERROR);  return FALSE; }
    if (text_source())
    { linp = new t_lineinp(inhnd, lfonly!=0, src_charsize);  
     // skipping byteorder mark on the beginning
      srcline=linp->get_line(&slinelen);
      if (src_recode==CHARSET_UTF8)
        if (srcline && !memcmp(srcline, "\xef\xbb\xbf", 3))
          { srcline+=3;  slinelen-=3; }
     // analyse the 1st line in the CSV format:
      if (source_type==IETYPE_COLS)
        analyse_cols_in_line(srcline, slinelen, src_recode);
      src_recnum=NORECNUM;
    }
    else if (dbf_source())
    {// read xbase header
      dbf_inp = new t_dbf_io(inhnd, source_type==IETYPE_DBASE);
      if (!dbf_inp->rdinit(cdp, inpath, &src_recnum))
        return FALSE;
     // convert column names to numbers:
      if (convert_column_names_to_numbers(cdp, dbf_inp, s_specif))
        { client_error(cdp, OS_FILE_ERROR);  return FALSE; }
    }
  }

 ////// prepare destination (creation, convtype, field offsets, headers): ////
 // When creating table, may use additional type info from the source (charset)!
  if (dest_exists)  
  {// find column names, map them to column numbers:
    const d_table * td;
    if (txcdp->get_cdp())
      td = cd_get_table_d(txcdp->get_cdp(), out_obj, CATEG_TABLE);
    else
      td = make_odbc_descriptor9(txcdp->get_odbcconn(), outpath, outschema, TRUE);
    if (!td) return FALSE;
    for (int i=0;  i<dd.count();  i++)
    { tdetail * det = dd.acc0(i);  d_attr info;
      if (*det->destin) // find and add the attribute name
      { if (txcdp->get_cdp()) Upcase9(txcdp->get_cdp(), det->destin);  else Upcase(det->destin);
        int col = find_upcase_column_in_td(td, det->destin);
        det->dattr = col==-1 ? 0 : col;  // 0 -> column not found, will not write into it
      }
    }
    release_table_d(td);
  }
  else  // !dest_exists: prepare the creating of the destination
    if (dest_type==IETYPE_WB || dest_type==IETYPE_ODBC)
    { table_all ta;
      strcpy(ta.selfname, outpath);  // length is limited in the dialog
      strcpy(ta.schemaname, *outschema ? outschema : txcdp->get_cdp()->sel_appl_name);   // using [sel_appl_name] is not necessary, the default schema is choosen by the server when not specified in the statement
      ta.tabdef_flags=0;

      int attrnum=1;
      for (i=0;  i<dd.count();  i++)
      { det=dd.acc0(i);
        if (*det->destin) // unless line or destination empty
        { atr_all * cur_atr=ta.attrs.next();
          cur_atr->state=ATR_NEW;
          cur_atr->nullable=wbtrue;  cur_atr->multi=1;  cur_atr->expl_val=wbfalse;
          cur_atr->specif.set_null();  // may contain info from older type, must null it!
          if (det->desttype==ATT_STRING || det->desttype==ATT_TEXT)
            if ((db_source() || odbc_source()) &&  // copy the source specif
                (source_dt->attrs[det->sattr].type==ATT_STRING || source_dt->attrs[det->sattr].type==ATT_TEXT))
              cur_atr->specif = source_dt->attrs[det->sattr].specif;  // length will be overwritten below
            else
            { cur_atr->specif.charset=code2specif(src_recode).charset;
#ifdef LINUX
              if (cur_atr->specif.charset && !(cur_atr->specif.charset & 0x80))  // usually not supported on Linux
                cur_atr->specif.charset|=0x80;
#endif
            }
          strcpy(cur_atr->name, det->destin);
          cur_atr->type=(uns8)/*enum_ind2val(get_control_def(inst, wP), */det->desttype;
          if (SPECIFLEN(cur_atr->type)) 
            cur_atr->specif.length=det->destlen1;
          else if (cur_atr->type==ATT_INT16 || cur_atr->type==ATT_INT32 || cur_atr->type==ATT_INT64 || cur_atr->type==ATT_INT8)
            cur_atr->specif.scale=(sig8)det->destlen1;
          det->dattr=attrnum++;
        }
      }
      out_obj=create_table(txcdp, &ta);
      if (out_obj==NOOBJECT) return FALSE;
    }
    else  
    { d_specif = code2specif(dest_recode);
      dest_charsize = dest_recode==CHARSET_UCS4 ? 4 : dest_recode==CHARSET_UCS2 ? 2 : 1;
      if (dest_charsize==1 && !charset_available(d_specif.charset))
        { SET_ERR_MSG(cdp, "<target file charset>");  client_error(cdp, STRING_CONV_NOT_AVAIL);  return FALSE; }
     // create destination file: 
      outhnd=CreateFileA(outpath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
      if (outhnd==INVALID_FHANDLE_VALUE)
      { 
#ifdef STOP  // cannot ask here, must be outside
        if (!overwrite_existing_target_file && !silent)
          if (!can_overwrite_file(outpath, NULL))
            return FALSE;  // no additional error message is overwrite not confirmed
        if (overwrite_existing_target_file || !silent)
#endif
          outhnd=CreateFileA(outpath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (outhnd==INVALID_FHANDLE_VALUE)
          { client_error(cdp, OS_FILE_ERROR);  return FALSE; }
      }
      fbw.start(outhnd);
     // prepare output:
      if (text_target())
      { for (int i=0;  i<dd.count();  i++)
          dd.acc0(i)->convtype=ATT_STRING;
      }
      else if (dbf_target())
      {// check for memo field (create memo file?):
        outmemo=FALSE;
        for (i=0;  i<dd.count();  i++)
        { det=dd.acc0(i);
          if (*det->destin) // unless line empty
            if (det->desttype==DBFTYPE_MEMO) { outmemo=TRUE;  break; }
        }
       // init DBF output:
        dbf_out = new t_dbf_io(outhnd, dest_type==IETYPE_DBASE);
        if (!dbf_out->wrinit(outmemo, colmapstop, destrecsize, outpath))
          { client_error(cdp, OS_FILE_ERROR);  return FALSE; }
       // write column descriptions:
        for (i=0;  i<dd.count();  i++)
        { det=dd.acc0(i);
          if (*det->destin) // unless line empty
          { tobjname encoded_column_name;
            superconv(ATT_STRING, ATT_STRING, det->destin, encoded_column_name, t_specif(0, cdp ? cdp->sys_charset : 0, 0, 0), d_specif, NULLSTRING);
            dbf_out->add_field(encoded_column_name, DBF_type_char[det->desttype-1], det->destlen1, det->destlen2, det->destoff);
            det->convtype=DBF_conv_type[det->desttype-1];
            if (det->convtype==ATT_FLOAT && !det->destlen2)
              det->convtype = det->destlen1<10 ? ATT_INT32 : ATT_INT64;
          }
        }
        dbf_out->add_field(NULL, 0, 0, 0, 0);  // writes field delimiter
      }
    }

  if (dest_type==IETYPE_WB && source_type==IETYPE_WB && sxcdp==txcdp)  // fast transport inside a single 602SQL server
    return wb2wb_trasport();

 // dest exists, open it (file already opened):
  if (dest_type==IETYPE_WB)
    dest_dt=create_cached_access9(txcdp->get_cdp(), NULL, NULL, NULL, NULL, out_obj);
#ifdef CLIENT_ODBC_9
  else if (dest_type==IETYPE_ODBC)
  { dest_dt=create_cached_access9(NULL, txcdp->get_odbcconn(), NULL, outpath, outschema, NOOBJECT);
    if (!dest_dt) pass_error_info(txcdp->get_odbcconn(), cdp);  // odbc_error called inside
  }
#endif
  if (dest_type==IETYPE_WB || dest_type==IETYPE_ODBC)
  { if (dest_dt==NULL) return FALSE;
   // prepare cache (allocated with the initial size 1):
    init_cache_dynars(dest_dt);
   // determine existing destination types & sizes:
    for (i=0;  i<dd.count();  i++)
    { det=dd.acc0(i);
      if (*det->destin && *det->source)  // line non-empty
      { det->convtype = det->dattr ? dest_dt->attrs[det->dattr].type : 0;  // no type when column not found
#ifdef STOP  // I believe that this is not used, but it overwrites the "scale" informtion for 
        det->destlen1=dest_dt->attrs[det->dattr].size;
        if (is_string(det->convtype)) det->destlen1--;
#endif
      }
    }
  }


 /////////////////////////// special cases of input/output pairing //////////
 // dBase numeric -> WB string: set number of decimal digits, no semilog
  int cind;
  for (cind=0;  cind<colmapstop;  cind++)
    if (colmap[cind]!=NOATTRIB)
    { det=dd.acc0(colmap[cind]);  // copy according to det
      if (dbf_source())
        if (dest_type==IETYPE_WB || dest_type==IETYPE_ODBC)
        { xadef * xatr = dbf_inp->xdef+(det->sattr-1);
          if (xatr->type=='N' || xatr->type=='F')
            if (is_string(det->convtype) || det->convtype==ATT_TEXT)
              { det->destlen2=xatr->dec;  semilog=FALSE; }
        }
    }
  if (dbf_target()) semilog=FALSE;

  char space_char = ' ';
  tptr satrval;  unsigned satrvallen;  char atrbuf[MAX_FIXED_STRING_LENGTH+2];
  char * resbuf;  void * bigbufin = NULL;  void * bigbufout = NULL;
  const d_table * dest_odbc_td = NULL;
  if (odbc_target())
  { dest_odbc_td = make_odbc_descriptor9(txcdp->get_odbcconn(), outpath, outschema, TRUE);
    prepare_insert_statement(dest_dt, dest_odbc_td);
  }
 ///////////// create output header (use source column names, target column names are not specified in the design) ///////
  if (add_header && text_target())
  { char column_name[MAX_SOURCE+1];
   // access the source:
    const d_table * odbc_td = NULL; 
    if (source_type==IETYPE_ODBC)
      odbc_td = make_odbc_descriptor9(sxcdp->get_odbcconn(), inpath, inschema, TRUE);
    if (source_type==IETYPE_CSV)
    { unsigned p=0, column=0;  bool quoted=false;
      while (column <= max_src_col)
      { tattrib ind = src_colmap[column];
        unsigned start=p;
        while (p<slinelen)
        { if (LINECHAR(srcline, p, src_recode)==csv_separ && !quoted) break;
          if (LINECHAR(srcline, p, src_recode)==csv_quote) quoted=!quoted;
          p++;
        }
        if (ind!=NOATTRIB)
        { det = dd.acc(ind);
          det->srcoff =start;  // offsets and sizes in chars!
          det->srcsize=p-start;
        }
        if (p<slinelen) p++;
        column++;
      }
    }
   // create target column names:
    for (cind=0;  cind<colmapstop;  cind++)
    { if (colmap[cind]!=NOATTRIB)
      { det=dd.acc(colmap[cind]);
       // get header field value:
        switch (source_type)
        { case IETYPE_COLS:   
            if (skip_lines && srcline!=NULL)
              if (det->srcoff >= slinelen)
                satrvallen=0;
              else
              { satrval = srcline + det->srcoff*src_charsize;  
                if (det->srcoff+det->srcsize > slinelen)
                  satrvallen=slinelen-det->srcoff;
                else
                  satrvallen=det->srcsize;
                while (satrvallen && LINECHAR(satrval, satrvallen-1, src_recode)==' ') satrvallen--;  // removing trailing spaces
                if (satrvallen+1>MAX_SOURCE/src_charsize) satrvallen = MAX_SOURCE/src_charsize-1;
                memcpy(column_name, satrval, satrvallen*src_charsize);
              }
            else
              { ((uns32*)column_name)[0]='?';  satrvallen=1; }  // valid for every char size
            *(uns32*)(column_name+satrvallen*src_charsize)=0;
            break;
          case IETYPE_CSV:
            if (skip_lines && srcline!=NULL)
            { satrval=srcline+det->srcoff*src_charsize;  satrvallen=det->srcsize;
              int i=0;
             // skip spaces:
              if (i<satrvallen && LINECHAR(satrval, i, src_recode)==' ') i++; // must un-quote
             // check for quote:
              if (i<satrvallen && LINECHAR(satrval, i, src_recode)==csv_quote)  // must un-quote
              { i++;
                int j=0;
                while (i<satrvallen)
                { if (LINECHAR(satrval, i, src_recode)==csv_quote)
                    if (i+1==satrvallen || !LINECHAR(satrval, i+1, src_recode)==csv_quote)
                      break;
                    else i++;
                  if (src_charsize==1)
                    satrval[j]=satrval[i];
                  else if (src_charsize==2)
                    ((uns16*)satrval)[j]=((uns16*)satrval)[i];
                  else
                    ((uns32*)satrval)[j]=((uns32*)satrval)[i];
                  i++;  j++;
                }
                satrvallen=j;
              }
              if (satrvallen+1>MAX_SOURCE/src_charsize) satrvallen = MAX_SOURCE/src_charsize-1;
              memcpy(column_name, satrval, satrvallen*src_charsize);
            }
            else
              { ((uns32*)column_name)[0]='?';  satrvallen=1; }  // valid for every char size
            *(uns32*)(column_name+satrvallen*src_charsize)=0;
            break;
          case IETYPE_DBASE:  case IETYPE_FOX:
            strcpy(column_name, dbf_inp->xdef[det->sattr-1].name);  // preserving the the diacritics is better, will be converted below
            break;
          case IETYPE_WB:     case IETYPE_CURSOR:
            strcpy(column_name, det->source);  // preserving the the diacritics is better, will be converted below
            break;
          case IETYPE_ODBC:
            if (odbc_td!=NULL)
              strcpy(column_name, ATTR_N(odbc_td, det->sattr)->name);
            else *column_name=0;
            break;
        }
      }
      else
        strcpy(column_name, "?");
     // remove `s and replace spaces:
      int x=0, y=0;
      while (column_name[x])
      { if (column_name[x]!='`')
        { if (column_name[x]==' ')
            column_name[y]='_';
          else
            column_name[y]=column_name[x];
          y++;
        }
        x++;
      }
      column_name[y]=0;
     // write it (s_specif contains the source encoding):
      int err=superconv(ATT_STRING, ATT_STRING, column_name, atrbuf, s_specif, d_specif, NULLSTRING);
      satrvallen = d_specif.charset==CHARSET_NUM_UTF8 ? (int)strlen(atrbuf) : (int)strlen(column_name)*dest_charsize;
      if (dest_type==IETYPE_COLS)
      { if (satrvallen > det->destlen1*dest_charsize) satrvallen = det->destlen1*dest_charsize;
        fbw.write(atrbuf, satrvallen); 
        satrvallen /= dest_charsize;  // bytes -> chars
        while (satrvallen++ < det->destlen1)
          fbw.write(&space_char_wide, dest_charsize);
      }
      else // CSV (full length here):
      { if (cind) fbw.write(&csv_separ_wide, dest_charsize);
        int charvallen = satrvallen / dest_charsize;  // bytes -> chars
        unsigned limlen = charvallen;  // not limited ->destlen1 && det->destlen1 < charvallen ? det->destlen1 : charvallen;
        bool quoted = false;
        for (int i=0;  i<limlen;  i++)
          if (LINECHAR(atrbuf, i, dest_recode)==csv_separ || LINECHAR(atrbuf, i, dest_recode)==csv_quote)
            { quoted=true;  break; }
        if (quoted)
        { fbw.write(&csv_quote_wide, dest_charsize);
          for (int i=0;  i<limlen;  i++)
          { unsigned linechar = LINECHAR(atrbuf, i, dest_recode);
            fbw.write(&linechar, dest_charsize);
            if (linechar==csv_quote)      // inside quote characters doubled
              fbw.write(&linechar, dest_charsize);
          }
          fbw.write(&csv_quote_wide, dest_charsize);
        }
        else fbw.write(atrbuf, dest_charsize*limlen);
      }
    } // cycle on fields
    if (odbc_td!=NULL) release_table_d(odbc_td);
    uns32 ch;
#ifdef WINS
    ch = '\r';
    fbw.write(&ch, dest_charsize);
#endif
    ch = '\n';
    fbw.write(&ch, dest_charsize);
  } // adding header

 // skip the input header lines (after creating the output header):
  if (text_source())
  { for (i=skip_lines;  i>0;  i--)
    { srcline=linp->get_line(&slinelen);
      if (srcline == NULL) break;
    }
    have_line = srcline != NULL;
  }
 // prepare formats for converting types:
  char numeric_form[2];  char real_form[1+9+1+1];   
  bool use_locale_decim_separ = (odbc_source() || odbc_target()) && !(text_source() || text_target());
  // Oracle driver needs sysapi separator DECIMAL a NUMERIC values
  numeric_form[0]=use_locale_decim_separ ? *get_separator(0) : decim_separ;  numeric_form[1]=0;

 ///////////////////////// transport it: ///////////////////////////////////
 //// multiattributes supported only by "from [0]" and "to [0]"
  trecnum srec=0, outrecnum=0, conversion_error_line=NORECNUM;
  int stype=ATT_STRING, dtype;    // used by CSV, COLS
  bool to_text_format = text_target();

  do
  {
   ///////////////////// get next input record: ////////////////////////////
   // source recode moved here because it is made in-place and must not be made twice if a source column is used twice
    switch (source_type)
    { case IETYPE_WB:  case IETYPE_CURSOR:  case IETYPE_ODBC:
        do
        { cache_free(source_dt, 0, 1);
          source_dt->cache_top=NORECNUM;
          if (srec >= src_recnum) goto source_eof;
          source_dt->cache_top=srec;  // without this cache_free will not release anything later
          cache_load(source_dt, NULL, 0, 1, srec);
          if (source_type==IETYPE_ODBC && (uns8)source_dt->cache_ptr[0]==0xff)
            goto source_eof;  // loading error -> end of ODBC table
          srec++;
        } while (source_dt->cache_ptr[0] != REC_EXISTS); // while deleted or WB loading error
        break;
      case IETYPE_COLS:
        if (have_line) have_line=false;
        else srcline=linp->get_line(&slinelen);
        if (srcline==NULL) goto source_eof;
        srec++;  // counter only
        break;
      case IETYPE_CSV:
        if (have_line) have_line=false;
        else srcline=linp->get_line(&slinelen);
        if (srcline==NULL) goto source_eof;
        srec++;  // counter only
       // divide it:
        { unsigned p=0, column=0;  bool quoted=false;
          while (column <= max_src_col)
          { tattrib ind = src_colmap[column];
            unsigned start=p;
            while (p<slinelen)
            { if (LINECHAR(srcline, p, src_recode)==csv_separ && !quoted) break;
              if (LINECHAR(srcline, p, src_recode)==csv_quote) quoted=!quoted;
              p++;
            }
            if (ind!=NOATTRIB)
            { det = dd.acc(ind);
              det->srcoff =start;  // offsets and sizes in chars!
              det->srcsize=p-start;
            }
            if (p<slinelen) p++;
            column++;
          }
        }
        break;
      case IETYPE_DBASE:   case IETYPE_FOX:
        do
        { if (srec >= src_recnum) goto source_eof;
          if (!dbf_inp->read()) goto source_eof;
          srec++;
        } while (dbf_inp->deleted());
        break;
    }

   /////////////////////// copy values of attributes: ///////////////////////
    for (cind=0;  cind<colmapstop;  cind++)
    { if (colmap[cind]==NOATTRIB)
      { if (dest_type==IETYPE_CSV) if (cind) fbw.write(&csv_separ_wide, dest_charsize);
        continue;  // no source for this destination
      }
      det=dd.acc0(colmap[cind]);  // copy according to det
     /////////////////////// get source value: //////////////////////////////
      switch (source_type)
      { case IETYPE_WB:  case IETYPE_CURSOR:  case IETYPE_ODBC:
        { atr_info * catr = source_dt->attrs+det->sattr;
          satrval    = (tptr)source_dt->cache_ptr + catr->offset;
          if (catr->multi>1)
          { t_dynar * d = (t_dynar*)satrval;
            if (!d->count()) continue;  // no transfer from this attribute
            satrval = (tptr)d->acc0(0);
          }

          stype    = catr->type;
          s_specif = catr->specif;
          if (catr->flags & CA_INDIRECT)
          { indir_obj * iobj = (indir_obj*)satrval;
            if (iobj->obj) { satrval=iobj->obj->data; satrvallen=(unsigned)iobj->actsize; }
            else           { satrval=NULLSTRING;      satrvallen=0; }
          }
          else if (stype == ATT_AUTOR)
          { User_name_by_id(sxcdp->get_cdp(), (uns8*)satrval, atrbuf);
            satrval=atrbuf;  satrvallen=(int)strlen(satrval);  stype=ATT_STRING;
          }
          else if (IS_STRING(stype) || stype==ATT_ODBC_NUMERIC || stype==ATT_ODBC_DECIMAL)
            if (s_specif.wide_char)
              satrvallen=2*(int)wuclen((wuchar*)satrval);
            else
              satrvallen=(int)strlen(satrval);
          else satrvallen = catr->size;
          break;
        }
        case IETYPE_COLS: // sets satrval & satrvallen
          if (det->srcoff >= slinelen)
            { satrval=NULLSTRING;  satrvallen=0; }
          else
          { satrval = srcline + det->srcoff*src_charsize;  
            if (det->srcoff+det->srcsize > slinelen)
              satrvallen=slinelen-det->srcoff;
            else
              satrvallen=det->srcsize;
            while (satrvallen && LINECHAR(satrval, satrvallen-1, src_recode)==' ') satrvallen--;  // removing trailing spaces
            satrvallen*=src_charsize;
          }
          break;
        case IETYPE_CSV:  // sets satrval & satrvallen
        { satrval=srcline+det->srcoff*src_charsize;  satrvallen=det->srcsize;
          int i=0;
         // skip spaces:
          if (i<satrvallen && LINECHAR(satrval, i, src_recode)==' ') i++; // must un-quote
         // check for quote:
          if (i<satrvallen && LINECHAR(satrval, i, src_recode)==csv_quote)  // must un-quote
          { i++;
            int j=0;
            while (i<satrvallen)
            { if (LINECHAR(satrval, i, src_recode)==csv_quote)
                if (i+1==satrvallen || !LINECHAR(satrval, i+1, src_recode)==csv_quote)
                  break;
                else i++;
              if (src_charsize==1)
                satrval[j]=satrval[i];
              else if (src_charsize==2)
                ((uns16*)satrval)[j]=((uns16*)satrval)[i];
              else
                ((uns32*)satrval)[j]=((uns32*)satrval)[i];
              i++;  j++;
            }
            satrvallen=j;
          }
          satrvallen*=src_charsize;
          break;
        }
        case IETYPE_DBASE:   case IETYPE_FOX:
        { xadef * xatr = dbf_inp->xdef+(det->sattr-1);
          switch (xatr->type)
          { case 'C':  // Characters
              satrval=dbf_inp->recbuf+xatr->offset;
              satrvallen=xatr->size;
              stype=ATT_STRING;
              while (satrvallen && satrval[satrvallen-1]==' ') satrvallen--;  // removing trailing spaces
              break;
            case 'L':  // Logical
            { char c=dbf_inp->recbuf[xatr->offset];
              if      (c=='T' || c=='t' || c=='Y' || c=='y') *atrbuf=wbtrue;
              else if (c=='F' || c=='f' || c=='N' || c=='n') *atrbuf=wbfalse;
              else *atrbuf=(char)0x80;
              satrval=atrbuf;  stype=ATT_BOOLEAN;
              break;
            }
            case 'D':  // Date
            { tptr ptr=dbf_inp->recbuf+xatr->offset;
              if (empty(ptr, xatr->size)) *(sig32*)atrbuf=NONEDATE;
              else  /* must check for ' ' replacing '0' (calc602) */
				      { int day, month, year;
                day  =ptr[7]-'0';
                if (ptr[6] && (ptr[6]!=' ')) day   +=10  *(ptr[6]-'0');
                month=ptr[5]-'0';
                if (ptr[4] && (ptr[4]!=' ')) month +=10  *(ptr[4]-'0');
                year =ptr[3]-'0';
                if (ptr[2] && (ptr[2]!=' ')) year  +=10  *(ptr[2]-'0');
                if (ptr[1] && (ptr[1]!=' ')) year  +=100 *(ptr[1]-'0');
                if (ptr[0] && (ptr[0]!=' ')) year  +=1000*(ptr[0]-'0');
                *(sig32*)atrbuf=Make_date(day, month, year);
              }
              satrval=atrbuf;  stype=ATT_DATE;
              break;
            }
            case 'N':  case 'F':  // Numeric
            { tptr ptr=dbf_inp->recbuf+xatr->offset;
              if (xatr->dec || xatr->size>18)
              { if (empty(ptr, xatr->size)) *(double*)atrbuf=NONEREAL;
                else
                { double dval=0.0;  BOOL decim, minus;  double koef=0.1f;
                  decim=minus=FALSE;
                  int i=0;  while (!ptr[i] && i<xatr->size) i++;
                  for (;  i<xatr->size;  i++)
                    if      (ptr[i]=='-') minus=TRUE;
                    else if (ptr[i]=='.') decim=TRUE;
                    else if (ptr[i]>='0' && ptr[i]<='9')
                      if (decim) { dval+=koef*(ptr[i]-'0');  koef*=0.1f; }
                      else dval=10*dval+(ptr[i]-'0');
                    else if (!ptr[i]) break;  // compatibility with some DBF libs
                  *(double*)atrbuf = minus ? -dval : dval;
                }
                stype=ATT_FLOAT;
              }
              else if (xatr->size>9) // no decimal part
              { if (empty(ptr, xatr->size)) *(sig64*)atrbuf=NONEINTEGER;
                else
                { sig64 lval=0;  BOOL minus=FALSE;
                  int i=0;  while (!ptr[i] && i<xatr->size) i++;
                  for (;  i<xatr->size;  i++)
                    if (ptr[i]=='-') minus=TRUE;
                    else if (ptr[i]>='0' && ptr[i]<='9')
                      lval=10*lval+ptr[i]-'0';
                    else if (!ptr[i]) break;  // compatibility with some DBF libs
                  *(sig64*)atrbuf = minus ? -lval : lval;
                }
                stype=ATT_INT64;
              }
              else  // no decimal part
              { if (empty(ptr, xatr->size)) *(sig32*)atrbuf=NONEINTEGER;
                else
                { sig32 lval=0;  BOOL minus=FALSE;
                  int i=0;  while (!ptr[i] && i<xatr->size) i++;
                  for (;  i<xatr->size;  i++)
                    if (ptr[i]=='-') minus=TRUE;
                    else if (ptr[i]>='0' && ptr[i]<='9')
                      lval=10*lval+ptr[i]-'0';
                    else if (!ptr[i]) break;  // compatibility with some DBF libs
                  *(sig32*)atrbuf = minus ? -lval : lval;
                }
                stype=ATT_INT32;
              }
              satrval=atrbuf;
              break;
            }
            case 'M':  // Memo
            { tptr ptr=dbf_inp->recbuf+xatr->offset;
              satrval=NULLSTRING;  satrvallen=0;
              uns32 memo_size = dbf_inp->get_memo_size(ptr);
              if (memo_size)
              { if (bigbufin) corefree(bigbufin);  
                bigbufin = corealloc(memo_size+4, 44);  // must have space for the (temporary) terminator
                if (bigbufin!=NULL)
                { satrvallen=dbf_inp->read_memo((char*)bigbufin, memo_size);
                  satrval   =(char*)bigbufin;
                }
              }
              stype=ATT_TEXT; // implies replacing CR, LF by space
              break;
            }
          } // switch on X-type
          break;
        } // DBF
      } // switch (source_type)

     ///////////////////// convert the value: ////////////////////////////
     // in satrval/satrvallen inplace:
      if (to_text_format && stype==ATT_TEXT) // replace CR, LF etc. by spaces
        if (s_specif.wide_char==0)
        { unsigned char * p = (unsigned char *)satrval;
          for (int i=0;  i<satrvallen;  i++, p++)
            if (*p && *p<' ') *p=' ';
        }
        else if (s_specif.wide_char==1)
        { uns16 * p = (uns16*)satrval;
          for (int i=0;  i<satrvallen;  i+=2, p++)
            if (*p && *p<' ') *p=' ';
        }

      dtype=det->convtype;
      {// find d_specif:
        if (db_target() || odbc_target())
        { if (!det->dattr) continue;  // column name from the design not found
          atr_info * catr = dest_dt->attrs+det->dattr;
          d_specif = catr->specif;
          dest_charsize = d_specif.wide_char==2 ? 4 : d_specif.wide_char==1 ? 2 : 1;
        }  // otherwise d_specif is default
       // prepare the string format:
        const char * string_format = NULL;  int string_format_type;
        if (stype==ATT_STRING || stype==ATT_TEXT || stype==ATT_ODBC_DECIMAL || stype==ATT_ODBC_NUMERIC)
          string_format_type=dtype;
        else if (dtype==ATT_STRING || dtype==ATT_TEXT || dtype==ATT_ODBC_DECIMAL || dtype==ATT_ODBC_NUMERIC)
          string_format_type=stype;
        else
          string_format_type=0;
        switch (string_format_type)
        { case ATT_INT8:  case ATT_INT16:  case ATT_INT32:   case ATT_INT64:
          case ATT_PTR:   case ATT_BIPTR:  case ATT_VARLEN:  case ATT_INDNUM:
            string_format=numeric_form;  break;
          case ATT_FLOAT:
            sprintf(real_form, "%c%u%c", use_locale_decim_separ ? *get_separator(0) : decim_separ, 
                                         det->destlen2, semilog ? 'e' : 'f');
            string_format=real_form;  
            break;
          case ATT_DATE:
            string_format=date_form;  break;
          case ATT_TIME:
            string_format=time_form;  break;
          case ATT_TIMESTAMP:  case ATT_DATIM:
            string_format=timestamp_form;  break; 
          case ATT_BOOLEAN:
            string_format=boolean_form;  break; 
        }
       // add temporary string terminator:
        uns32 origchar;
        if (stype==ATT_STRING || stype==ATT_TEXT || stype==ATT_CHAR)
        { switch (s_specif.wide_char)
          { case 0: origchar=         satrval [satrvallen];             satrval [satrvallen  ]=0;  break;
            case 1: origchar=((uns16*)satrval)[satrvallen/2];  ((uns16*)satrval)[satrvallen/2]=0;  break;
            case 2: origchar=((uns32*)satrval)[satrvallen/4];  ((uns32*)satrval)[satrvallen/4]=0;  break;
          }
        }
        int spaceest = space_estimate(stype, dtype, satrvallen, s_specif, d_specif);
        if (spaceest+4 > sizeof(atrbuf))
        { if (bigbufout) free(bigbufout);
          bigbufout=malloc(spaceest+4);
          if (!bigbufout) 
            { client_error(cdp, OUT_OF_MEMORY);  goto error_exit; }
          resbuf=(char*)bigbufout;
        }
        else 
          resbuf=atrbuf;
       // create special opqval for BLOBs:
        if (IS_HEAP_TYPE(stype) && stype!=ATT_TEXT)
          s_specif.opqval = satrvallen;
       // convert:
        int err=superconv(stype, dtype, satrval, resbuf, s_specif, d_specif, string_format);
       // restore the character overwritten by the terminator:
        if (stype==ATT_STRING || stype==ATT_TEXT || stype==ATT_CHAR)
        { switch (s_specif.wide_char)
          { case 0:          satrval [satrvallen  ]=origchar;  break;
            case 1: ((uns16*)satrval)[satrvallen/2]=origchar;  break;
            case 2: ((uns32*)satrval)[satrvallen/4]=origchar;  break;
          }
        }
        if (err<0) 
        { conversion_error_line=outrecnum;
          continue;
        }
        satrval=resbuf;
       // find the length of the converted value:
        if (dtype==ATT_STRING || dtype==ATT_TEXT)
        { if (dest_charsize==1)
            satrvallen=(int)strlen(satrval);
          else if (dest_charsize==sizeof(wchar_t))
            satrvallen=(int)wcslen((wchar_t*)satrval) * (int)sizeof(wchar_t);
          else
          { satrvallen=0;  // must not use the dest_record here, not defined for databases
            while (LINECHAR(satrval, satrvallen, dest_charsize==2 ? CHARSET_UCS2 : CHARSET_UCS4)) satrvallen++;
            satrvallen*=dest_charsize;
          }
        }
        else if (dtype==ATT_BINARY || IS_HEAP_TYPE(dtype))
          satrvallen=err;  
      }
#ifdef STOP
      if (dtype!=stype)
        if (IS_STRING(stype) || stype==ATT_TEXT || stype==ATT_ODBC_NUMERIC || stype==ATT_ODBC_DECIMAL)
        { if (stype==ATT_TEXT && (dest_type==IETYPE_COLS || dest_type==IETYPE_CSV))
            for (int i=0;  i<satrvallen;  i++)
              if (satrval[i]=='\r' || satrval[i]=='\n' || satrval[i]=='\0')  // this values damage the text file
                satrval[i]=' '; // replace them by space
          if (!(IS_STRING(dtype) || dtype==ATT_TEXT || dtype==ATT_ODBC_NUMERIC || dtype==ATT_ODBC_DECIMAL))
          { if (satrvallen >= sizeof(atrbuf)) satrvallen = sizeof(atrbuf)-1;
            memcpy(atrbuf, satrval, satrvallen);  atrbuf[satrvallen]=0;
            switch (dtype)
            { case ATT_DATE:
                f_str2date(atrbuf, (uns32*)atrbuf, date_form);
                break;
              case ATT_TIME:
                f_str2time(atrbuf, (uns32*)atrbuf, time_form);
                break;
              case ATT_TIMESTAMP:
              { int offset;  tptr p;  uns32 dt, tm;
                offset=f_str2date(atrbuf, &dt, date_form);
                p=atrbuf+offset;  while (*p==' ') p++;
                f_str2time(p, &tm, time_form);
                TIMESTAMP_STRUCT ts;
                if (dt==NONEDATE) ts.day=32;
                else
                { ts.day=Day(dt);  ts.month=Month(dt);  ts.year=Year(dt);
                  if (tm==NONEDATE) tm=0;
                  ts.hour=Hours(tm);  ts.minute=Minutes(tm);
                  ts.second=Seconds(tm);  ts.fraction=Sec1000(tm);
                }
                TIMESTAMP2datim(&ts, (uns32*)atrbuf);
                break;
              }
              default:
                if (!convert_from_string(dtype, atrbuf, atrbuf, t_specif()))  // scale and wide chars not supported here
                  catr_set_null(atrbuf, dtype);
                break;
            }
            satrval=atrbuf;
          }
        }
        else
        { switch (dtype)
          { case ATT_STRING:  case ATT_TEXT:
            { int prez=5;
              basicval bval, * theval = (basicval*)satrval;
              switch (stype)
              { case ATT_ODBC_DATE:  case ATT_ODBC_TIME:  case ATT_ODBC_TIMESTAMP:
                case ATT_AUTOR:
                  theval=&bval;  bval.ptr=atrbuf;  break;
                case ATT_BINARY:
                  theval=&bval;  bval.ptr=atrbuf;  prez=satrvallen;  break;
                case ATT_FLOAT:
                  if (semilog) prez=det->destlen2;
                  else if (det->destlen2) prez=-det->destlen2;
                  else /* integer */
                  { double val=floor(*(double*)satrval);
                    int dec, sign;
                    strcpy(atrbuf,_ecvt(val, 100, &dec, &sign));
                    if (dec>0) atrbuf[dec]=0;  else atrbuf[1]=0;
                    if (sign)
                    { memmov(atrbuf+1, atrbuf, strlen(atrbuf)+1);
                      *atrbuf='-';
                    }
                    goto converted;
                  }
                  break;
                case ATT_DATE:
                  f_date2str(*(uns32*)satrval, atrbuf, date_form);
                  goto converted;
                case ATT_TIME:
                  f_time2str(*(uns32*)satrval, atrbuf, time_form);
                  goto converted;
                case ATT_TIMESTAMP:  case ATT_DATIM:
                { TIMESTAMP_STRUCT ts;
                  datim2TIMESTAMP(*(uns32*)satrval, &ts);
                  if (ts.day==32) *atrbuf=0;
                  else
                  { f_date2str(Make_date(ts.day, ts.month, ts.year), atrbuf, date_form);
                    strcat(atrbuf, "  ");
                    f_time2str(Make_time(ts.hour, ts.minute, ts.second, ts.fraction),
                               atrbuf+strlen(atrbuf), time_form);
                  }
                  goto converted;
                }
                case ATT_MONEY:
                  prez=det->destlen2<=1 ? 0 : det->destlen2>2 ? 2 : 1;  break;
              }
              conv2str((basicval0*)theval, stype, atrbuf, prez, s_specif);
             converted:
              satrvallen=strlen(atrbuf);  break;
            }
            case ATT_INT8:
              switch (stype)
              { case ATT_INT32:
                  *(sig8*)atrbuf = *(sig32*)satrval>0x7f || *(sig32*)satrval<-0x7f ? NONETINY : *(sig8*)satrval;
                  break;
                case ATT_INT16:
                  *(sig8*)atrbuf = *(sig16*)satrval>0x7f || *(sig16*)satrval<-0x7f ? NONETINY : *(sig8*)satrval;
                  break;
                case ATT_INT64:
                  *(sig8*)atrbuf = *(sig64*)satrval>0x7f || *(sig64*)satrval<-0x7f ? NONETINY : *(sig8*)satrval;
                  break;
                case ATT_MONEY:
                  money2int((monstr*)satrval, (sig32*)atrbuf);
                  *(sig8*)atrbuf = *(sig32*)atrbuf >0x7f || *(sig32*)atrbuf <-0x7f ? NONETINY : *(sig8*)atrbuf;
                  break;
                case ATT_FLOAT:
                { double rval=*(double*)satrval;
                  if (rval>0) rval+=0.5;  else rval-=0.5;
                  *(sig8*)atrbuf = rval==NONEREAL || rval>0x7f || rval<-0x7f ? NONETINY : (sig8)rval;
                  break;
                }
                default:
                  *(sig8*)atrbuf=NONETINY;
                  break;
              }
              break;
            case ATT_INT16:
              switch (stype)
              { case ATT_INT8:
                  *(sig16*)atrbuf = *(sig8 *)satrval==NONETINY  ? NONESHORT : (sig16)*(sig8 *)satrval;
                  break;
                case ATT_INT32:
                  *(sig16*)atrbuf = *(sig32*)satrval>0x7fff || *(sig32*)satrval<-0x7fff ? NONESHORT : *(sig16*)satrval;
                  break;
                case ATT_INT64:
                  *(sig16*)atrbuf = *(sig64*)satrval>0x7fff || *(sig64*)satrval<-0x7fff ? NONESHORT : *(sig16*)satrval;
                  break;
                case ATT_MONEY:
                  money2int((monstr*)satrval, (sig32*)atrbuf);
                  *(sig16*)atrbuf = *(sig32*)atrbuf>0x7fff || *(sig32*)atrbuf<-0x7fff ? NONESHORT : *(sig16*)atrbuf;
                  break;
                case ATT_FLOAT:
                { double rval=*(double*)satrval;
                  if (rval>0) rval+=0.5;  else rval-=0.5;
                  *(sig16*)atrbuf = rval==NONEREAL || rval>0x7fff || rval<-0x7fff ? NONESHORT : (sig16)rval;
                  break;
                }
                default:
                  *(sig16*)atrbuf=NONESHORT;
                  break;
              }
              break;
            case ATT_INT32:
              switch (stype)
              { case ATT_INT8:
                  *(sig32*)atrbuf = *(sig8 *)satrval==NONETINY  ? NONEINTEGER : (sig32)*(sig8 *)satrval;
                  break;
                case ATT_INT16:
                  *(sig32*)atrbuf = *(sig16*)satrval==NONESHORT ? NONEINTEGER : (sig32)*(sig16*)satrval;
                  break;
                case ATT_INT64:
                  *(sig32*)atrbuf = *(sig64*)satrval>0x7fffffff || *(sig64*)satrval<-0x7fffffff ? NONEINTEGER : *(sig32*)satrval;
                  break;
                case ATT_MONEY:
                  money2int((monstr*)satrval, (sig32*)atrbuf);
                  break;
                case ATT_FLOAT:
                { double rval=*(double*)satrval;
                  if (rval>0) rval+=0.5;  else rval-=0.5;
                  *(sig32*)atrbuf = rval==NONEREAL || rval>0x7fffffff || rval<-0x7fffffff ? NONEINTEGER : (sig32)rval;
                  break;
                }
                default:
                  *(sig32*)atrbuf=NONEINTEGER;
                  break;
              }
              break;
            case ATT_INT64:
              switch (stype)
              { case ATT_INT8:
                  *(sig64*)atrbuf = *(sig8 *)satrval==NONETINY    ? NONEBIGINT : (sig64)*(sig8 *)satrval;
                  break;
                case ATT_INT16:
                  *(sig64*)atrbuf = *(sig16*)satrval==NONESHORT   ? NONEBIGINT : (sig64)*(sig16*)satrval;
                  break;
                case ATT_INT32:
                  *(sig64*)atrbuf = *(sig32*)satrval==NONEINTEGER ? NONEBIGINT : (sig64)*(sig32*)satrval;
                  break;
                case ATT_MONEY:
                  if (IS_NULLMONEY((monstr*)atrbuf))
                    *(sig64*)atrbuf = NONEBIGINT;
                  else
                  { memcpy(atrbuf, satrval, MONEY_SIZE);
                    ((uns16*)atrbuf)[3] = ((uns16*)atrbuf)[2] & 0x8000 ? 0xffff : 0;
                    *(sig64*)atrbuf /= 100;
                  }
                  break;
                case ATT_FLOAT:
                { double rval=*(double*)satrval;
                  if (rval>0) rval+=0.5;  else rval-=0.5;
                  *(sig64*)atrbuf = rval==NONEREAL || rval>MAX_SIG64 || rval<-MAX_SIG64 ? NONEBIGINT : (sig64)rval;
                  break;
                }
                default:
                  *(sig32*)atrbuf=NONEINTEGER;
                  break;
              }
              break;
            case ATT_MONEY:
              switch (stype)
              { case ATT_INT8:
                  int2money(*(sig8 *)satrval==NONETINY  ? NONEINTEGER : (sig32)*(sig8 *)satrval, (monstr*)atrbuf);
                  break;
                case ATT_INT16:
                  int2money(*(sig16*)satrval==NONESHORT ? NONEINTEGER : (sig32)*(sig16*)satrval, (monstr*)atrbuf);
                  break;
                case ATT_INT32:
                  int2money(*(sig32*)satrval, (monstr*)atrbuf);
                  break;
                case ATT_INT64:
                  *(sig64*)atrbuf = *(sig64*)satrval>MAX_SIG48/100 || *(sig64*)satrval<-MAX_SIG48/100 ? MIN_SIG48 : 100 * *(sig64*)satrval;
                  break;
                case ATT_FLOAT:
                  real2money(*(double*)satrval, (monstr*)atrbuf);
                  break;
                default:
                  ((monstr*)atrbuf)->money_hi4=NONEINTEGER;  ((monstr*)atrbuf)->money_lo2=0;
                  break;
              }
              break;
            case ATT_FLOAT:
              switch (stype)
              { case ATT_INT8:
                  *(double*)atrbuf=*(sig8 *)satrval==NONETINY    ? NONEREAL : (double)*(sig8 *)satrval;
                  break;
                case ATT_INT16:
                  *(double*)atrbuf=*(sig16*)satrval==NONESHORT   ? NONEREAL : (double)*(sig16*)satrval;
                  break;
                case ATT_INT32:
                  *(double*)atrbuf=*(sig32*)satrval==NONEINTEGER ? NONEREAL : (double)*(sig32*)satrval;
                  break;
                case ATT_INT64:
                  *(double*)atrbuf=*(sig64*)satrval==NONEBIGINT  ? NONEREAL : (double)*(sig64*)satrval;
                  break;
                case ATT_MONEY:
                  *(double*)atrbuf=money2real((monstr*)satrval);
                  break;
                default:
                  *(double*)atrbuf=NONEREAL;
                  break;
              }
              break;
            case ATT_DATE:
            case ATT_TIME:
            case ATT_TIMESTAMP:
            case ATT_DATIM:
            case ATT_ODBC_DATE:
            case ATT_ODBC_TIME:
            case ATT_ODBC_TIMESTAMP:
            { int day, month, year;  int hour, minute, second, fraction;
              BOOL is_null=FALSE;
              day=month=year=hour=minute=second=fraction=0;
             // analysis:
              switch (stype)
              { case ATT_DATIM:  case ATT_TIMESTAMP:
                { uns32 dtm = *(uns32*)satrval;
                  if (dtm==(uns32)NONEINTEGER) is_null=TRUE;
                  else
                  { dtm /= 86400L;
                    day   =(int)(dtm % 31 + 1);  dtm /= 31;
                    month =(int)(dtm % 12 + 1);
                    year  =(int)(dtm/12 + 1900);
                    dtm = *(uns32*)satrval % 86400L;
                    second=(int)(dtm % 60);  dtm /= 60;
                    minute=(int)(dtm % 60);  dtm /= 60;
                    hour  =(int)dtm;
                  }
                  break;
                }
                case ATT_DATE:
                { uns32 dt=*(uns32*)satrval;
                  if (dt==NONEDATE) is_null=TRUE;
                  else
                    { day=Day(dt);  month=Month(dt);  year=Year(dt); }
                  break;
                }
                case ATT_TIME:
                { uns32 tm=*(uns32*)satrval;
                  if (tm==NONETIME) is_null=TRUE;
                  else
                  { hour=Hours(tm);  minute=Minutes(tm);
                    second=Seconds(tm);  fraction=Sec1000(tm);
                  }
                  break;
                }
                case ATT_ODBC_DATE:
                { DATE_STRUCT * dtst = (DATE_STRUCT *)satrval;
                  if (dtst->day==32) is_null=TRUE;
                  else
                    { day=dtst->day;  month=dtst->month;  year=dtst->year; }
                  break;
                }
                case ATT_ODBC_TIME:
                { TIME_STRUCT * tmst = (TIME_STRUCT *)satrval;
                  if (tmst->hour>=24) is_null=TRUE;
                  else
                  { hour=tmst->hour;  minute=tmst->minute;
                    second=tmst->second;
                  }
                  break;
                }
                case ATT_ODBC_TIMESTAMP:
                { TIMESTAMP_STRUCT * dtmst = (TIMESTAMP_STRUCT *)satrval;
                  if (dtmst->day>=32) is_null=TRUE;
                  else
                  { day=dtmst->day;  month=dtmst->month;  year=dtmst->year;
                    hour=dtmst->hour;  minute=dtmst->minute;
                    second=dtmst->second;  fraction=dtmst->fraction;
                  }
                  break;
                }
                default: is_null=TRUE;
              } // analysis
             // synthesis:
              switch (dtype)
              { case ATT_DATE:
                  *(uns32*)atrbuf=is_null ? NONEDATE : Make_date(day, month, year);  break;
                case ATT_TIME:
                  *(uns32*)atrbuf=is_null ? NONETIME : Make_time(hour, minute, second, fraction);  break;
                case ATT_TIMESTAMP:
                case ATT_DATIM:
                  *(uns32*)atrbuf=is_null ? NONEINTEGER :
                    Make_timestamp(day, month, year, hour, minute, second);
                  break;
                case ATT_ODBC_DATE:
                { DATE_STRUCT * dtst = (DATE_STRUCT *)atrbuf;
                  dtst->day=is_null ? 32 : day;
                  dtst->month=month;
                  dtst->year=year;
                  break;
                }
                case ATT_ODBC_TIME:
                { TIME_STRUCT * tmst = (TIME_STRUCT *)atrbuf;
                  tmst->hour=is_null ? 25 : hour;
                  tmst->minute=minute;
                  tmst->second=second;
                  break;
                }
                case ATT_ODBC_TIMESTAMP:
                { TIMESTAMP_STRUCT * dtmst = (TIMESTAMP_STRUCT *)atrbuf;
                  dtmst->day=is_null ? 32 : day;
                  dtmst->month=month;
                  dtmst->year=year;
                  dtmst->hour=hour;
                  dtmst->minute=minute;
                  dtmst->second=second;
                  dtmst->fraction=fraction;
                  break;
                }
              }
              break;
            }
            default:
              *atrbuf=0;  satrvallen=0;
              break;
          } // switch
          satrval=atrbuf;
        } // not from string
#endif
     ////////////////////// write the destination value: ////////////////////
      switch (dest_type)
      { case IETYPE_WB:  case IETYPE_ODBC:  default:
        { atr_info * catr = dest_dt->attrs+det->dattr;
          if (!det->dattr) // descr. generated from file and attribute not found or dest. left empty
            break;
          tptr p = (tptr)dest_dt->cache_ptr + catr->offset;
          if (catr->multi>1)
          { t_dynar * d = (t_dynar*)p;
            p = (tptr)d->acc(0);
            if (p==NULL) continue;   // no transfer to this attribute
          }

          if (catr->flags & CA_INDIRECT)
          { indir_obj * iobj = (indir_obj*)p;
            if (iobj->obj)
              { HGLOBAL hnd=GlobalPtrHandle(iobj->obj);  GlobalUnlock(hnd);  GlobalFree(hnd); }
            if (!satrvallen) { iobj->obj=NULL;  iobj->actsize=0; }
            else
            { HGLOBAL hnd=GlobalAlloc(GMEM_MOVEABLE, sizeof(cached_longobj)+satrvallen);
              iobj->obj=(cached_longobj*)GlobalLock(hnd);
              if (iobj->obj!=NULL)
              { iobj->actsize=iobj->obj->maxsize=satrvallen;
                memcpy(iobj->obj->data, satrval, satrvallen);
              }
            }
          }
          else if (IS_STRING(catr->type))
          { if (satrvallen > catr->size-1) satrvallen=catr->size-1;
            memcpy(p, satrval, satrvallen);  p[satrvallen]=0;
            if (d_specif.wide_char) p[satrvallen+1]=0;
          }
          else memcpy(p, satrval, catr->size);
          catr->changed=wbtrue;
          break;
        }
        case IETYPE_COLS:
          if (satrvallen > det->destlen1*dest_charsize) satrvallen = det->destlen1*dest_charsize;
          fbw.write(satrval, satrvallen); 
          satrvallen /= dest_charsize;  // bytes -> chars
          while (satrvallen++ < det->destlen1)
            fbw.write(&space_char_wide, dest_charsize);
          break;
        case IETYPE_CSV:  // output with limit det->destlen1 if nonzero
        { if (cind) fbw.write(&csv_separ_wide, dest_charsize);
          int charvallen = satrvallen / dest_charsize;  // bytes -> chars
          unsigned limlen = det->destlen1 && det->destlen1 < charvallen ? det->destlen1 : charvallen;
          bool quoted = false;
          for (int i=0;  i<limlen;  i++)
            if (LINECHAR(satrval, i, dest_recode)==csv_separ || LINECHAR(satrval, i, dest_recode)==csv_quote)
              { quoted=true;  break; }
          if (quoted)
          { fbw.write(&csv_quote_wide, dest_charsize);
            for (int i=0;  i<limlen;  i++)
            { unsigned linechar = LINECHAR(satrval, i, dest_recode);
              fbw.write(&linechar, dest_charsize);
              if (linechar==csv_quote)      // inside quote characters doubled
                fbw.write(&linechar, dest_charsize);
            }
            fbw.write(&csv_quote_wide, dest_charsize);
          }
          else fbw.write(satrval, dest_charsize*limlen);
          break;
        }
        case IETYPE_DBASE:   case IETYPE_FOX:
        { tptr p=dbf_out->recbuf+det->destoff;
          if (det->desttype==DBFTYPE_MEMO)
          { if (!dbf_out->write_memo(satrval, satrvallen, p, TRUE))
              { client_error(cdp, OS_FILE_ERROR);  goto error_exit; }
          } // MEMO
          else switch (dtype)  // DBF type is determined by the convtype
          { case ATT_BOOLEAN:  // Logical
              *p=!*satrval ? 'F' : *(uns8*)satrval==0x80 ? '?' : 'T';  break;
            case ATT_DATE:     // Date
            { uns32 dt=*(uns32*)satrval;
              if (dt==(uns32)NONEDATE) memset(p, ' ', det->destlen1);
              else
              { p[0]=(char)('0' + Year(dt) / 1000);
                p[1]=(char)('0' + Year(dt) % 1000 / 100);
                p[2]=(char)('0' + Year(dt) % 100  / 10);
                p[3]=(char)('0' + Year(dt) % 10);
                p[4]=(char)('0' + Month(dt)/ 10);
                p[5]=(char)('0' + Month(dt)% 10);
                p[6]=(char)('0' + Day(dt)  / 10);
                p[7]=(char)('0' + Day(dt)  % 10);
                if (det->destlen1>8) memset(p+8, ' ', det->destlen1-8);
              }
              break;
            }
            case ATT_INT32:    // Numeric with DEC==0
              if (*(sig32*)satrval==NONEINTEGER) memset(p, ' ', det->destlen1);
              else c4ltoa45(*(sig32 *)satrval, p, det->destlen1);
              break;
            case ATT_INT64:    // Numeric with DEC==0
              if (*(sig64*)satrval==NONEBIGINT)  memset(p, ' ', det->destlen1);
              else c4ltoa45(*(sig64 *)satrval, p, det->destlen1);
              break;
            case ATT_FLOAT:    // Numeric with DEC>0
              if (*(double*)satrval==NONEREAL  ) memset(p, ' ', det->destlen1);
              else c4dtoa45(*(double*)satrval, p, det->destlen1, det->destlen2);
              break;
            case ATT_STRING:   // Characters
              if (satrvallen > det->destlen1) satrvallen = det->destlen1;
              memcpy(p, satrval, satrvallen);
              memset(p+satrvallen, ' ', det->destlen1-satrvallen);
              break;
          }
          break;
        } // DBF
      } // switch on convtype
    } // cycle on attributes
   // value is in satrval, if it is string or text, satrvallen is valid

   //////////////////////// write output record: ///////////////////////////
    switch (dest_type)
    { case IETYPE_WB:
      { BOOL err;  
        trecnum erec=(trecnum)-1;  // INSERT record
        err=wb_write_cache(txcdp->get_cdp(), dest_dt, out_obj, &erec, dest_dt->cache_ptr, FALSE);
        cache_free(dest_dt, 0, 1);
        init_cache_dynars(dest_dt);
        dest_dt->cache_top=NORECNUM;
        if (err) goto error_exit;
        break;
      }
#ifdef CLIENT_ODBC_9
      case IETYPE_ODBC:
        if (odbc_synchronize_view9(FALSE, txcdp->get_odbcconn(), dest_dt, NORECNUM, dest_dt->cache_ptr, NULL, FALSE))
        { pass_error_info(txcdp->get_odbcconn(), cdp);  // odbc_error called inside
          goto error_exit;
        }
        cache_free(dest_dt, 0, 1);
        init_cache_dynars(dest_dt);  // writes NULL values for the next record
        break;
#endif
      case IETYPE_COLS: // write the line separator
      case IETYPE_CSV:
      { uns32 ch;
#ifdef WINS
        ch = '\r';
        fbw.write(&ch, dest_charsize);
#endif
        ch = '\n';
        fbw.write(&ch, dest_charsize);
        break;
      }
      case IETYPE_DBASE:   case IETYPE_FOX: // write output record
        if (!dbf_out->write())
          { client_error(cdp, OS_FILE_ERROR);  goto error_exit; }
        break;
    }

   // inter-record actions:
    outrecnum++;
    if (!(outrecnum % 100))
    { client_status_nums(srec, src_recnum);
#if 0
      cdp->RV.holding=wbtrue;
      //deliver_messages(cdp, TRUE);
      cdp->RV.holding=wbfalse;
	    if (cdp->RV.break_request)
      { cdp->RV.break_request=wbfalse;
        client_error(cdp, REQUEST_BREAKED);
        goto error_exit;
      }
#endif
    }
  } while (true);  // cycle on source records

 source_eof:
  if (text_target())
  { fbw.flush();
    if (fbw.is_error())
      { client_error(cdp, OS_FILE_ERROR);  goto error_exit; }
  }
  if (bigbufin) { corefree(bigbufin);  bigbufin=NULL; }
  if (bigbufout) { free(bigbufout);  bigbufout=NULL; }
  if (dest_odbc_td) release_table_d(dest_odbc_td);
  if (dbf_target())
    dbf_out->wrclose(outrecnum);
  client_status_nums(outrecnum, NORECNUM);
  if (conversion_error_line!=NORECNUM)
  { char numbuf[20];
    int2str(conversion_error_line+1, numbuf);
    if (cdp) SET_ERR_MSG(cdp, numbuf);
    client_error(cdp, MOVE_DATA_ERROR);
    return FALSE;
  }
  return TRUE;
 error_exit:
  if (bigbufin) { free(bigbufin);  bigbufin=NULL; }
  if (bigbufout) { free(bigbufout);  bigbufout=NULL; }
  if (dest_odbc_td) release_table_d(dest_odbc_td);
  return FALSE;
}

bool t_ie_run::creates_target_file(void) const
{ return dest_type!=IETYPE_WB /*&& dest_type!=IETYPE_ODBC && dest_type<IETYPE_NEWODBC*/; }

bool t_ie_run::creates_target_table(void) const
{ return dest_type==IETYPE_WB && !dest_exists; }

static BOOL nice_type(int type)
{ return
    type==ATT_BOOLEAN || type==ATT_CHAR || type==ATT_INT16 || type==ATT_INT32 || type==ATT_INT8 || type==ATT_INT64 ||
    type==ATT_MONEY   || type==ATT_FLOAT|| type==ATT_BINARY ||
    type==ATT_DATE    || type==ATT_DATIM|| type==ATT_TIME || type==ATT_TIMESTAMP ||
    type==ATT_TEXT    || is_string(type);
}

void t_ie_run::OnCreImpl(void)
{ tdetail * det;  int counter=1; // couner of columns if the text output file
  dd.t_dynar::~t_dynar();
  const d_table * td = NULL;
 // check sxcdp, txcdp:
  if (!text_target() && !dbf_target() && !txcdp || !text_source() && !dbf_source() && !sxcdp)
    return;
 
  switch (source_type)
  { case IETYPE_WB:  case IETYPE_CURSOR:  
      td = cd_get_table_d(sxcdp->get_cdp(), in_obj, source_type==IETYPE_CURSOR ? CATEG_CURSOR : CATEG_TABLE);
      goto have_td;
    case IETYPE_ODBC:
      td = make_odbc_descriptor9(sxcdp->get_odbcconn(), inpath, inschema, TRUE);
     have_td:
      if (td!=NULL)
      { int i;  const d_attr * pattr;
        for (i=first_user_attr(td), pattr=ATTR_N(td,i);        i<td->attrcnt;
             i++,                   pattr=NEXT_ATTR(td,pattr))
          if (pattr->multi==1 && nice_type(pattr->type) || !dbf_target() && !text_target())
          { //if (pattr->type==ATT_TIME && dbf_target()) continue; -- removed, I do not know the reason
            det=dd.next();
            if (det!=NULL)
            { ident_to_sql(sxcdp, det->source, pattr->name);  wb_small_string(cdp, det->source, false);
              det->sattr=i;  // used by ODBC soubore
              if (text_target()) int2str(counter++, det->destin);
              else strcpy(det->destin, pattr->name);
              det->destlen2=0;
              if (dbf_target()) det->destin[10]=0;
              if (dbf_target() || text_target())
              { switch (pattr->type)
                { case ATT_BOOLEAN:
                    det->desttype=DBFTYPE_LOGICAL;  det->destlen1=dbf_target() ? 1 : 5;  break;
                  case ATT_CHAR:
                    det->desttype=DBFTYPE_CHAR;     det->destlen1=1;  break;
                  case ATT_INT8:
                    det->desttype=DBFTYPE_NUMERIC;  det->destlen1=3;  break;
                  case ATT_INT16:
                    det->desttype=DBFTYPE_NUMERIC;  det->destlen1=6;  break;
                  case ATT_INT32:
                    det->desttype=DBFTYPE_NUMERIC;  det->destlen1=11; break;
                  case ATT_INT64:
                    det->desttype=DBFTYPE_NUMERIC;  det->destlen1=20; break;
                  case ATT_ODBC_NUMERIC:
                  case ATT_ODBC_DECIMAL:
                    det->desttype=DBFTYPE_NUMERIC;  det->destlen1=pattr->specif.precision;  det->destlen2=pattr->specif.scale;  break;
                  case ATT_MONEY:
                    det->desttype=DBFTYPE_NUMERIC;  det->destlen1=16;  det->destlen2=2;  break;
                  case ATT_FLOAT:
                    det->desttype=DBFTYPE_NUMERIC;  det->destlen1=20;  det->destlen2=8;  break;
                  case ATT_DATIM:  case ATT_TIMESTAMP:
                    if (!dbf_target())            { det->destlen1=pattr->specif.with_time_zone ? 26:20;  break; }  // 2 spaces between date and time emitted
                    /* cont. */
                  case ATT_DATE:
                  case ATT_ODBC_DATE:
                    det->desttype=DBFTYPE_DATE;     det->destlen1=dbf_target() ? 8:10;   break;
                  case ATT_TEXT:
                    det->desttype=DBFTYPE_MEMO;  
                    if (dest_type==IETYPE_CSV)      det->destlen1=0; // do not limit the size of CSV text   
                    else                            det->destlen1=500; break;
                  case ATT_STRING:
                    det->desttype=DBFTYPE_CHAR;     det->destlen1=pattr->specif.length;  
                    if (dbf_target())
                      if (det->destlen1 > 255) det->destlen1 = 255;  // field length is in 1 byte
                    break;
                  case ATT_ODBC_TIMESTAMP:
                    det->desttype=DBFTYPE_CHAR;     det->destlen1=24;  break;
                  case ATT_ODBC_TIME:
                  case ATT_TIME:
                    det->desttype=DBFTYPE_CHAR;     det->destlen1=pattr->specif.with_time_zone ? 18:12;  break;
                  case ATT_BINARY:
                    det->desttype=DBFTYPE_CHAR;     det->destlen1=2*pattr->specif.length;  
                    if (det->destlen1 > 254) det->destlen1 = 254;  // field length is in 1 byte
                    break;  // converts every byte to 2 characters
                }
                if (text_target())
                { det->desttype=ATT_STRING;
                  if (dest_type==IETYPE_COLS)
                  { if      (pattr->type==ATT_BOOLEAN) det->destlen1=5; // "FALSE" needs 5
                    else if (pattr->type==ATT_DATE)    det->destlen1=11;
                    else det->destlen1++;
                    int hdrlen=(int)strlen(pattr->name)+1;
                    if (det->destlen1 < hdrlen) det->destlen1 = hdrlen;
                  }
                }
              }
              else
              { switch (pattr->type)
                { case ATT_ODBC_NUMERIC:
                  case ATT_ODBC_DECIMAL:   det->desttype=ATT_INT64;     break;
                  case ATT_ODBC_DATE:      det->desttype=ATT_DATE;      break;
                  case ATT_ODBC_TIMESTAMP: det->desttype=ATT_TIMESTAMP; break;
                  case ATT_ODBC_TIME:      det->desttype=ATT_TIME;      break;
                  default:                 det->desttype=pattr->type;   break;
                }
                det->destlen1=SPECIFLEN(det->desttype) ? pattr->specif.length : 
                  det->desttype==ATT_INT16 || det->desttype==ATT_INT32 || det->desttype==ATT_INT64 || det->desttype==ATT_INT8 ? 
                    pattr->specif.scale : 0;
               // translate desttype for ODBC table:
#ifdef CLIENT_ODBC_9x
                if (dest_type==IETYPE_ODBC)
                  if (ti==NULL) det->desttype=0; // dest type not known
                  else
                    det->desttype=type_WB_2_ODBC(ti, det->desttype);
                  { SWORD sqltype=type_WB_to_sql(det->desttype, TRUE);
                    det->desttype=0; // dest type not found (so far)
                    for (int i=0;  ti[i].strngs != NULL;  i++)
                      if (ti[i].sql_type==sqltype && !(ti[i].flags & TI_IS_AUTOINCR)) 
                        { det->desttype=i+1;  break; }
                  }
#endif
              }
            }
          }
        release_table_d(td);
      }
      break;

    case IETYPE_COLS:   case IETYPE_CSV:
    { FHANDLE inhnd=CreateFileA(inpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (inhnd!=INVALID_FHANDLE_VALUE)
      { t_lineinp * linp = new t_lineinp(inhnd, lfonly!=0, src_recode==CHARSET_UCS4 ? 4 : src_recode==CHARSET_UCS2 ? 2 : 1);
        tptr srcline;  unsigned slinelen;
        srcline=linp->get_line(&slinelen);
        if (srcline!=NULL)
        { if (src_recode==CHARSET_UTF8)
            if (srcline && !memcmp(srcline, "\xef\xbb\xbf", 3))
              { srcline+=3;  slinelen-=3; }
          unsigned p=0, i=0;
          if (source_type==IETYPE_COLS) // find column starts & sizes:
            while (p+i<slinelen && LINECHAR(srcline, p+i, src_recode)==' ') i++;  // skips margin (included in the 1st column)
          while (p<slinelen && dd.count()<MAX_TABLE_COLUMNS)
          { det = dd.next();
            det->srcoff=p;
            if (source_type==IETYPE_COLS) // find column starts & sizes:
            { while (p+i<slinelen && LINECHAR(srcline, p+i, src_recode)> ' ') i++;
              while (p+i<slinelen && LINECHAR(srcline, p+i, src_recode)<=' ') i++;
              det->srcsize=i;
            }
            else  // IETYPE_CSV
            { BOOL quoted=FALSE;
              while (p+i<slinelen)
              { if (LINECHAR(srcline, p+i, src_recode)==csv_separ && !quoted) break;
                if (LINECHAR(srcline, p+i, src_recode)==csv_quote) quoted=!quoted;
                i++;
              }
              det->srcsize=i;
              if (p+i<slinelen) i++;  // skip separator
            }
           // store source column:
            int2str(dd.count(), det->source);
           // store destin name:
            if (text_target())
              int2str(dd.count(), det->destin);
            else
            {// convert the column name into det->destin:
              char buf[(OBJ_NAME_LEN+1)*sizeof(uns32)];  t_specif s_specif, d_specif;
              unsigned len=det->srcsize;  if (len>OBJ_NAME_LEN) len=OBJ_NAME_LEN;
              if (src_recode==CHARSET_UCS4)
              { memcpy(buf, srcline+det->srcoff*sizeof(uns32), len*sizeof(uns32));
                ((uns32*)buf)[len]=0;  
              }
              else if (src_recode==CHARSET_UCS2)
              { memcpy(buf, srcline+det->srcoff*sizeof(uns16), len*sizeof(uns16));
                ((uns16*)buf)[len]=0;  
              }
              else // 8-bit
              { memcpy(buf, srcline+det->srcoff*sizeof(uns8 ), len*sizeof(uns8 ));
                ((uns8 *)buf)[len]=0;  
              }
             // remove quotes:
              if (source_type==IETYPE_CSV)
              { int j=0;
                while (LINECHAR(buf, j, src_recode)==' ') j++;
                if (LINECHAR(buf, j, src_recode)==csv_quote)
                { char buf2[(OBJ_NAME_LEN+1)*sizeof(uns32)];
                  memcpy(buf2, buf, sizeof(buf2));
                  int k=0;
                  j++;
                  while (LINECHAR(buf2, j, src_recode)) 
                  { if (LINECHAR(buf2, j, src_recode)==csv_quote)
                      if (LINECHAR(buf2, j+1, src_recode)!=csv_quote) break;
                      else j++;
                    *LINESET(buf, k, src_recode) = LINECHAR(buf2, j, src_recode);
                    k++;  j++;
                  }
                  *LINESET(buf, k, src_recode) = 0;
                }
              }
             // convert encoding:
              s_specif = code2specif(src_recode);
              if (text_target())  // never!
              { d_specif = code2specif(dest_recode);
                d_specif.wide_char = 0; // no space for the wide chars in det->destin
              }
              else d_specif.charset = cdp->sys_charset;
              superconv(ATT_STRING, ATT_STRING, buf, det->destin, s_specif, d_specif, NULLSTRING);
              cutspaces(det->destin);
              if (!is_AlphaA(*det->destin, cdp->sys_charset))
              { *det->destin='A';
                int2str(dd.count(), det->destin+1);
              }
            }
           // other destination parameters:
            det->destlen1=det->srcsize;  // if > 254 used by text output only
            if (dest_type==IETYPE_COLS && det->destlen1<3) 
              det->destlen1=3;  // usefull a bit for S1, S2,... columns
            if (source_type==IETYPE_CSV && !det->destlen1)
              det->destlen1=2;
            if (dbf_target())
              if (det->srcsize > 254)
                det->desttype=DBFTYPE_MEMO;
              else
                det->desttype=DBFTYPE_CHAR;
            else
              if (det->srcsize > MAX_FIXED_STRING_LENGTH)
                det->desttype=ATT_TEXT;
              else
                det->desttype=ATT_STRING;
           // continue:
            p+=i;  i=0;
          } // cycle on source fields
        }
        delete linp;
        CloseFile(inhnd);
      } // input file opened
      break;
    } // text input

    case IETYPE_DBASE:  case IETYPE_FOX:
    {// read xbase header
      FHANDLE hnd=CreateFileA(inpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (hnd!=INVALID_FHANDLE_VALUE)
      { t_dbf_io * dbf_inp = new t_dbf_io(hnd, source_type==IETYPE_DBASE);
        if (dbf_inp->rdinit(cdp, inpath, NULL))
        { for (int i=0;  i<dbf_inp->xattrs;  i++)
          { det=dd.next();
            if (det!=NULL)
            { det->sattr=i+1; // will not be used any more
              t_specif s_specif = code2specif(src_recode);
              superconv(ATT_STRING, ATT_STRING, dbf_inp->xdef[i].name, det->source, s_specif, t_specif(0, cdp->sys_charset, 0, 0), NULLSTRING);
              det->destlen2=0;
              if (text_target()) int2str(counter++, det->destin);
              else strcpy(det->destin, det->source);  // is already converted to sys_charset
              if (dbf_target())
              {  switch (dbf_inp->xdef[i].type)
                { case 'C':
                    det->desttype=DBFTYPE_CHAR;     break;
                  case 'F':
                    det->desttype=DBFTYPE_FLOAT;    break;
                  case 'N':
                    det->desttype=DBFTYPE_NUMERIC;  break;
                  case 'L':
                    det->desttype=DBFTYPE_LOGICAL;  break;
                  case 'D':
                    det->desttype=DBFTYPE_DATE;     break;
                  case 'M':
                    det->desttype=DBFTYPE_MEMO;     break;
                }
                det->destlen1=dbf_inp->xdef[i].size;
                det->destlen2=dbf_inp->xdef[i].dec;
              }
              else if (text_target())
              { det->desttype=ATT_STRING;
                switch (dbf_inp->xdef[i].type)
                { case 'C':
                    det->destlen1=dbf_inp->xdef[i].size+1;  break;
                  case 'F':  case 'N':
                    det->destlen1=dbf_inp->xdef[i].size+1;
                    det->destlen2=dbf_inp->xdef[i].dec;
                    break;
                  case 'L':
                    det->destlen1=4;   break;
                  case 'D':
                    det->destlen1=11;  break;
                  case 'M':
                    det->destlen1=500; break;
                }
                int hdrlen=(int)strlen(dbf_inp->xdef[i].name)+1;
                if (det->destlen1 < hdrlen) det->destlen1 = hdrlen;
              } // DBF->text
              else // WB dest
                switch (dbf_inp->xdef[i].type)
                { case 'C':
                    det->destlen1=dbf_inp->xdef[i].size;
                    det->desttype=det->destlen1==1 ? ATT_CHAR : ATT_STRING;
                    break;
                  case 'F':  
                    det->desttype=ATT_FLOAT;    break;
                  case 'N':
                    det->desttype=dbf_inp->xdef[i].size<=2 ? ATT_INT8 : dbf_inp->xdef[i].size<=4 ? ATT_INT16 : dbf_inp->xdef[i].size<=9 ? ATT_INT32 : ATT_INT64;
                    det->destlen1=dbf_inp->xdef[i].dec;
                    break;
                  case 'L':
                    det->desttype=ATT_BOOLEAN;   break;
                  case 'D':
                    det->desttype=ATT_DATE;      break;
                  case 'M':
                    det->desttype=ATT_TEXT;      break;
                }
            } // line added
          } // cycle on DBF fields
        } // DBF access opened
        delete dbf_inp;
        CloseFile(hnd);
      } // DBF file opened
      break;
    } // from DBF
  } // switch (source_type)

 // compare source-proposed attributes with destination attribtutes
  if (dest_exists)
  { int atrbase=0;
    mapped_input_columns=ignored_input_columns=0;
    const d_table * td;
    if (db_target())
      td = cd_get_table_d(txcdp->get_cdp(), out_obj, CATEG_TABLE);
    else
      td = make_odbc_descriptor9(txcdp->get_odbcconn(), outpath, outschema, TRUE);
    if (td!=NULL)
    { atrbase=first_user_attr(td);
      for (int i=0;  i<dd.count();  )
      { tdetail * det = dd.acc0(i);  d_attr info;
        char atrname[ATTRNAMELEN+1];  strmaxcpy(atrname, det->destin, sizeof(atrname));  
        if (txcdp->get_cdp()) Upcase9(txcdp->get_cdp(), atrname);  else Upcase(atrname);
        int col = find_upcase_column_in_td(td, atrname);
        if (col==-1)
        {
  //        *det->destin=0;  // clears if not an attribute name
          dd.delet(i);  ignored_input_columns++;
        }
        else
        { //det->dattr=info.get_num()-atrbase+1;
          i++;  mapped_input_columns++;
        }
      }
      release_table_d(td);
    }
  }
}

tptr t_ie_run::conv_ie_to_source(void)
{ t_flstr s;
  s.put("*");
  s.putint(source_type);  
#ifdef CLIENT_ODBC_9
  if (db_source())
  { if (stricmp(inserver, cdp->locid_server_name))
      { s.putss(inserver);  s.putc(':'); }
    if (stricmp(inserver, cdp->locid_server_name) || wb_stricmp(cdp, inschema, cdp->sel_appl_name))
      { s.putss(inschema);  s.putc(':'); }
  }
  else if (odbc_source())
      { s.putss(inserver); s.putc(':');  s.putss(inschema);  s.putc(':'); }
#endif
  s.putss(inpath);
  s.putint(dest_type);    
#ifdef CLIENT_ODBC_9
  if (db_target())
  { if (stricmp(outserver, cdp->locid_server_name))
      { s.putss(outserver);  s.putc(':'); }
    if (stricmp(outserver, cdp->locid_server_name) || wb_stricmp(cdp, outschema, cdp->sel_appl_name))
      { s.putss(outschema);  s.putc(':'); }
  }
  else if (odbc_target())
      { s.putss(outserver); s.putc(':');  s.putss(outschema);  s.putc(':'); }
#endif
  s.putss(outpath);
  s.putint(src_recode  < 0 ? 0 : src_recode);   
  s.putint(dest_recode < 0 ? 0 : dest_recode);
  s.putss(cond);
  s.put("\r\n");
 // details
  for (int i=0;  i<dd.count();  i++)
  { tdetail * det = dd.acc0(i);  
    if (*det->source)  // empty lines should not be usefull now (source column numbers can be skipped in the design)
    { s.put("* ");
     // save data:
      s.putss(det->source);  s.put(" 0 "); //s.putint(det->sattr);
      s.putint(det->srcsize);
      s.putss(det->destin);  s.put(" 0 "); //s.putint(det->dattr);
      s.putint(det->desttype);
      s.putint(det->destlen1);
      s.putint(det->destlen2);
      s.put("\r\n");
    }
  }
 // text and other parameters:
  s.put("+");

  s.putint(index_past);
  s.putint(csv_separ);  s.putint(csv_quote);
  s.putint(add_header);
  s.putint(skip_lines);
  s.putint(semilog);
  if (!lfonly) s.put(" * ");
  s.putss(date_form);  s.put(",");  s.putss(time_form);
  if (*timestamp_form || *boolean_form || decim_separ!='.')
    { s.put(",");  s.putss(timestamp_form);  s.put(",");  s.putss(boolean_form);  s.put(",");  s.putint(decim_separ); }
  s.put("\r\n");

  tptr src=s.get();  s.unbind();
  return src;
}

static sig32 get_int(CIp_t CI)
{ if (CI->cursym!=S_INTEGER) c_error(INTEGER_EXPECTED);
  sig32 val=(sig32)CI->curr_int;
  next_sym(CI);
  return val;
}
static void get_string(CIp_t CI, char * dest, unsigned maxsize)
{ if (CI->cursym!=S_STRING && CI->cursym!=S_CHAR) c_error(STRING_EXPECTED);
  strmaxcpy(dest, CI->curr_string(), maxsize);
  next_sym(CI);
}

void t_ie_run::analyse_trans(CIp_t CI)
{ if (CI->cursym!='*') c_error(GENERAL_SQL_SYNTAX); next_sym(CI);
  char name2[MAX_PATH+1];  // supposed that MAX_PATH >= MAX_ODBC_NAME_LEN
 // source:
  source_type = get_int(CI);  
  get_string(CI, inpath,  sizeof(inpath));
  if (CI->cursym==':')
  { next_sym(CI);
    get_string(CI, name2,  sizeof(name2));
    if (CI->cursym==':')
    { next_sym(CI);
      strmaxcpy(inserver, inpath, sizeof(inserver));
      strmaxcpy(inschema, name2, sizeof(inschema));
      get_string(CI, inpath,  sizeof(inpath));
    }
    else  // server not specified
    { strmaxcpy(inschema, inpath, sizeof(inschema));
      strmaxcpy(inpath, name2, sizeof(inpath));
    }
  }
 // target:
  dest_type = get_int(CI);  
  get_string(CI, outpath, sizeof(outpath));
  if (CI->cursym==':')
  { next_sym(CI);
    get_string(CI, name2,  sizeof(name2));
    if (CI->cursym==':')
    { next_sym(CI);
      strmaxcpy(outserver, outpath, sizeof(outserver));
      strmaxcpy(outschema, name2, sizeof(outschema));
      get_string(CI, outpath,  sizeof(outpath));
    }
    else  // server not specified
    { strmaxcpy(outschema, outpath, sizeof(outschema));
      strmaxcpy(outpath, name2, sizeof(outpath));
    }
  }
 // recode, text params etc.
  src_recode =get_int(CI);  dest_recode=get_int(CI);
  get_string(CI, cond, sizeof(cond));
  while (CI->cursym=='*')
  { tdetail * det = dd.next();
    if (det==NULL) c_error(OUT_OF_KERNEL_MEMORY);
    next_sym(CI);
    get_string(CI, det->source, sizeof(det->source));
    det->sattr=(uns8)get_int(CI);
    if (CI->cursym==S_INTEGER) det->srcsize=(uns16)get_int(CI);
    get_string(CI, det->destin, sizeof(det->destin));
    det->dattr=(uns8)get_int(CI);   // not used
    det->desttype=(uns16)get_int(CI);
    det->destlen1=(uns16)get_int(CI);  det->destlen2=(uns16)get_int(CI);
  }
  if (CI->cursym!='+') c_error(GENERAL_SQL_SYNTAX); next_sym(CI);
  index_past =(BOOL)get_int(CI);
  csv_separ  =(char)get_int(CI);
  csv_quote  =(char)get_int(CI);
  add_header =(BOOL)get_int(CI);
  skip_lines =      get_int(CI);
  semilog    =(BOOL)get_int(CI);
  if (CI->cursym=='*') { lfonly=FALSE;  next_sym(CI); }
  get_string(CI, date_form, sizeof(date_form));
  if (CI->cursym!=',') c_error(COMMA_EXPECTED);  next_sym(CI);
  get_string(CI, time_form, sizeof(time_form));
  if (CI->cursym==',')  // WB 9.0 extension
  { next_sym(CI);
    get_string(CI, timestamp_form, sizeof(timestamp_form));
    if (CI->cursym!=',') c_error(COMMA_EXPECTED);  next_sym(CI);
    get_string(CI, boolean_form, sizeof(boolean_form));
    if (CI->cursym!=',') c_error(COMMA_EXPECTED);  next_sym(CI);
    decim_separ = (char)get_int(CI);
  }
}

static void WINAPI analyse_transport(CIp_t CI)
{ ((t_ie_run*)CI->univ_ptr)->analyse_trans(CI); }

BOOL t_ie_run::load_design(tobjnum objnum)
{ if (!cdp)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return FALSE; }
  tptr src=cd_Load_objdef(cdp, objnum, CATEG_PGMSRC);
  if (src==NULL) return FALSE;
  int res = compile_design(src);
  corefree(src);
  if (res) { client_error(cdp, res);  return FALSE; }
  return TRUE;
}

int t_ie_run::compile_design(char * src)
{ compil_info xCI(cdp, src, DUMMY_OUT, analyse_transport);
  xCI.univ_ptr=this;
  return compile(&xCI);
}

void t_ie_run::copy_from(const t_ie_run & patt)
{
  source_type=patt.source_type;
  dest_type=patt.dest_type;
  strcpy(inserver,  patt.inserver);
  strcpy(outserver, patt.outserver);
  strcpy(inschema,  patt.inschema);
  strcpy(outschema, patt.outschema);
  strcpy(inpath,    patt.inpath);
  strcpy(outpath,   patt.outpath);
  strcpy(cond,      patt.cond);
  src_recode=patt.src_recode;
  dest_recode=patt.dest_recode;
  index_past=patt.index_past;
  csv_separ=patt.csv_separ; 
  csv_quote=patt.csv_quote;
  decim_separ=patt.decim_separ;
  add_header=patt.add_header;
  skip_lines=patt.skip_lines;
  strcpy(date_form, patt.date_form);
  strcpy(time_form, patt.time_form);
  strcpy(timestamp_form, patt.timestamp_form);
  strcpy(boolean_form, patt.boolean_form);
  semilog=patt.semilog;
  lfonly=patt.lfonly;
  creating_target=patt.creating_target;  // tested for a change
}

///////////////////////////// Import/Export API //////////////////////////////
bool make_connection(xcdp_t * xcdp, xcdp_t xcdp_inp, int format)
{
  switch (format)
  { case IETYPE_ODBC:  case IETYPE_ODBCVIEW:
      if (xcdp_inp && xcdp_inp->connection_type==CONN_TP_ODBC)
        { *xcdp = xcdp_inp;  return true; }
     // connect ODBC:
      break;
    case IETYPE_WB:  case IETYPE_CURSOR:
    { if (xcdp_inp && xcdp_inp->connection_type==CONN_TP_602)
        { *xcdp = xcdp_inp;  return true; }
     // connect 602:
      break;
    }
    default:
      *xcdp=NULL;
      return true;
  }
  return false;
}

CFNC DllExport BOOL WINAPI cd_Data_transport(cdp_t cdp, tobjnum move_descr_obj,
   xcdp_t src_xcdp,          xcdp_t trg_xcdp,
   const char * src_schema,  const char * trg_schema,
   const char * src_name,    const char * trg_name,
   int src_format, int trg_format, 
   int src_code, int trg_code,
   unsigned flags)
{ BOOL res;
 // create transport object,load transport definition:
  t_ie_run * ier = new t_ie_run(cdp, true);
  if (ier==NULL) { client_error(cdp, OUT_OF_MEMORY);  return FALSE; }
  if (move_descr_obj!=NOOBJECT)
    if (!ier->load_design(move_descr_obj)) goto err_exit;

 // analyse specific parameters:
  if (src_name   && *src_name)   strmaxcpy(ier->inpath,    src_name,   sizeof(ier->inpath));
  if (trg_name   && *trg_name)   strmaxcpy(ier->outpath,   trg_name,   sizeof(ier->outpath));
  if (src_schema && *src_schema) strmaxcpy(ier->inschema,  src_schema, sizeof(ier->inschema));
  if (trg_schema && *trg_schema) strmaxcpy(ier->outschema, trg_schema, sizeof(ier->outschema));
  if (src_format!=-1)
    if (src_format==10) ier->source_type=IETYPE_WB;
    else ier->source_type=src_format+1;
  if (trg_format!=-1)
    if (trg_format==10) ier->dest_type=IETYPE_WB;
    else ier->dest_type  =trg_format+1;
  ier->index_past = (flags & 1) != 0;
  if (src_code!=-1) ier->src_recode =src_code;
  if (trg_code!=-1) ier->dest_recode=trg_code;
  if (ier->source_type > IETYPE_LAST || ier->dest_type > IETYPE_LAST ||
      ier->source_type < 0           || ier->dest_type <  0)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  goto err_exit; }
  if (ier->src_recode >= CHARSET_CNT || ier->dest_recode >= CHARSET_CNT ||
      ier->src_recode < 0            || ier->dest_recode < 0)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  goto err_exit; }
  if (!make_connection(&ier->sxcdp, src_xcdp, ier->source_type))
    goto err_exit;
  if (!make_connection(&ier->txcdp, trg_xcdp, ier->dest_type))
    goto err_exit;
 // find objects:
  if (ier->source_type==IETYPE_WB)
  { if (cd_Find_prefixed_object(ier->sxcdp->get_cdp(), ier->inschema, ier->inpath, CATEG_TABLE,  &ier->in_obj))
      goto err_exit;
  }
  else if (ier->source_type==IETYPE_CURSOR)
    if (cd_Find_prefixed_object(ier->sxcdp->get_cdp(), ier->inschema, ier->inpath, CATEG_CURSOR, &ier->in_obj))
      goto err_exit;
  if (ier->dest_type==IETYPE_WB)
  { if (cd_Find_prefixed_object(ier->txcdp->get_cdp(), ier->outschema, ier->outpath, CATEG_TABLE,  &ier->out_obj))
      { ier->out_obj=NOOBJECT;  ier->dest_exists=FALSE; }
    else ier->dest_exists=TRUE;
  }
  else if (ier->dest_type==IETYPE_ODBC)
    ier->dest_exists=TRUE;   // creating not supported now
  else ier->dest_exists=FALSE;

 // default description:
  if (move_descr_obj==NOOBJECT)
    { ier->OnCreImpl();  ier->skip_lines=1;  ier->add_header=TRUE; }

  res=ier->do_transport();
  ier->run_clear();

  delete ier;
  return res;

 err_exit:
  delete ier;
  return FALSE;
}

CFNC DllExport BOOL WINAPI Move_data(cdp_t cdp, tobjnum move_descr_obj,
   const char * inpname, tobjnum inpobj, const char * outname,
   int inpformat, int outformat, int inpcode, int outcode, int silent)
{ BOOL res;
 // create transport object,load transport definition:
  t_ie_run * ier = new t_ie_run(cdp, silent);
  if (ier==NULL) { client_error(cdp, OUT_OF_MEMORY);  return FALSE; }
  if (move_descr_obj!=NOOBJECT)
    if (!ier->load_design(move_descr_obj)) goto err_exit;

 // analyse specific parameters:
  if (inpname && *inpname) strmaxcpy(ier->inpath,  inpname, sizeof(ier->inpath));
  if (outname && *outname) strmaxcpy(ier->outpath, outname, sizeof(ier->outpath));
  if (inpformat!=-1)
    if (inpformat>=10) ier->source_type=IETYPE_WB;
    else ier->source_type=inpformat+1;
  if (outformat!=-1)
    if (outformat>=10)
    { ier->dest_type=IETYPE_WB;
      ier->index_past=outformat>10;
    }
    else ier->dest_type  =outformat+1;
  if (inpcode!=-1) ier->src_recode =inpcode;
  if (outcode!=-1) ier->dest_recode=outcode;
  if (ier->source_type > IETYPE_LAST || ier->dest_type > IETYPE_LAST ||
      ier->source_type < 0           || ier->dest_type <  0)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  goto err_exit; }
  if (ier->src_recode >= CHARSET_CNT || ier->dest_recode >= CHARSET_CNT ||
      ier->src_recode < 0            || ier->dest_recode < 0)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  goto err_exit; }
  ier->sxcdp=cdp;
  ier->txcdp=cdp;
 // find objects:
  if (inpobj!=NOOBJECT)
  { ier->in_obj=inpobj;
    if (ier->source_type==IETYPE_WB)
      if (IS_CURSOR_NUM(inpobj)) *ier->inpath=0;
      else
        cd_Read(cdp, TAB_TABLENUM, inpobj, OBJ_NAME_ATR, NULL, ier->inpath);
    else if (ier->source_type==IETYPE_CURSOR)
      cd_Read(cdp, OBJ_TABLENUM, inpobj, OBJ_NAME_ATR, NULL, ier->inpath);
  }
  else if (ier->source_type==IETYPE_WB)
  { if (cd_Find_prefixed_object(ier->sxcdp->get_cdp(), ier->inschema, ier->inpath, CATEG_TABLE,  &ier->in_obj))
      goto err_exit;
  }
  else if (ier->source_type==IETYPE_CURSOR)
    if (cd_Find_prefixed_object(ier->sxcdp->get_cdp(), ier->inschema, ier->inpath, CATEG_CURSOR, &ier->in_obj))
      goto err_exit;
  if (ier->dest_type==IETYPE_WB)
  { if (cd_Find_prefixed_object(ier->txcdp->get_cdp(), ier->outschema, ier->outpath, CATEG_TABLE,  &ier->out_obj))
      { ier->out_obj=NOOBJECT;  ier->dest_exists=FALSE; }
    else ier->dest_exists=TRUE;
  }
  else if (ier->dest_type==IETYPE_ODBC)
    ier->dest_exists=TRUE;   // creating not supported now
  else ier->dest_exists=FALSE;

 // default description:
  if (move_descr_obj==NOOBJECT)
    { ier->OnCreImpl();  ier->skip_lines=1;  ier->add_header=TRUE; }

  res=ier->do_transport();
  ier->run_clear();

  delete ier;
  return res;

 err_exit:
  delete ier;
  return FALSE;
}

