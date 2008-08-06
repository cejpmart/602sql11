/****************************************************************************/
/* contr.cpp - container implementation                                     */
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

#ifndef WINS
#include <stdlib.h>
#endif
               
#define CONTR_ALLOC_STEP  3000

BOOL t_container::wopen(FHANDLE hFile)
{ if (hFile)
    { IsGlobStor=FALSE;  File=hFile; }
  else IsGlobStor=TRUE;
  opened=TRUE;
  return TRUE;
}

BOOL t_container::ropen(FHANDLE hFile)
{ if (hFile)
    { IsGlobStor=FALSE;  File=hFile; }
  else
    { IsGlobStor=TRUE;   rdpos=0; }
  opened=TRUE;
  return TRUE;
}

t_container::t_container(void)
{ hData=0;  buf=NULL;  datapos=rdpos=bufsize=0;  File=INVALID_FHANDLE_VALUE;  opened=FALSE; }

void t_container::put_handle(HGLOBAL hGlob)
{
#ifdef WINS   // not used on server side nor by NLM client!
  hData=hGlob;
  buf=(char *)GlobalLock(hData);
  datapos=(uns32)GlobalSize(hData);
#endif
}

t_container::~t_container(void)
{ if (opened)
  { if (IsGlobStor)
#ifdef WINS
      { if (hData) { GlobalUnlock(hData);  GlobalFree(hData); } }
#else
      { if (buf) free(buf); }
#endif
    opened=FALSE;
  }
}

HGLOBAL t_container::close(void)
{ if (!opened) return 0;
  if (IsGlobStor)
#ifdef WINS
    { if (hData) GlobalUnlock(hData); }
#endif
  opened=FALSE;
  return hData;
}

unsigned t_container::write(tptr data, unsigned size)
{ DWORD wr;
  if (!opened) return 0;
  if (IsGlobStor)
  { if (datapos+size > bufsize)  /* reallocate */
    { uns32 newsize = bufsize+CONTR_ALLOC_STEP;
      if (newsize < datapos+size) newsize = datapos+size;
#ifdef WINS
      HGLOBAL new_data;
      if (hData)
      { GlobalUnlock(hData);
        new_data = GlobalReAlloc(hData, newsize, GMEM_MOVEABLE);
        if (!new_data) { buf=(tptr)GlobalLock(hData);  return 0; }
      }
      else
      { new_data = GlobalAlloc(GMEM_MOVEABLE, newsize);
        if (!new_data) return 0;
      }
      hData=new_data;  buf=(tptr)GlobalLock(hData);
#else
      tptr newbuf = (tptr)malloc(newsize);
      if (!newbuf) return 0;
      if (buf) { memcpy(newbuf, buf, bufsize);  free(buf); }
      buf=newbuf;
#endif
      bufsize=newsize;
    }
    memcpy(buf+datapos, data, size);
    datapos+=size;
    return size;
  }
#ifndef SRVR  // not used by the server
  else return WriteFile(File, (tptr)data, size, &wr, NULL) ? wr : 0;
#else
  else return 0;
#endif
}

unsigned t_container::read(tptr dest, unsigned size)
{ DWORD rd;
  if (!opened) return 0;
  if (IsGlobStor)
  { if (rdpos+size > datapos)
      if (rdpos >= datapos) return 0;
      else size=(unsigned)(datapos-rdpos);
    memcpy(dest, buf+rdpos, size);
    rdpos+=size;
    return size;
  }
#ifndef SRVR  // not used by the server
  else return ReadFile(File, (tptr)dest, size, &rd, NULL) ? rd : 0;
#else
  else return 0;
#endif
}

