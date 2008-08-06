#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

uns8 _csconv[256] = {
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
 144,145,146,147,148,149,150,151,152,153,138,155,140,141,142,143,
 160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
 176,177,178,163,180,181,182,183,184,165,170,187,188,189,188,175,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,247,216,217,218,219,220,221,222,255};

void _strmaxcpy(char * destin, const char * source, unsigned maxsize)
{ unsigned len;
  if (!maxsize) return;
  maxsize--;
  len=strlen(source);
  if (len > maxsize) len=maxsize;
  memcpy(destin, source, len);
  destin[len]=0;
}

char * _cutspaces(char * txt)
{ int i;
  i=0; while (txt[i]==' ') i++;
  if (i) strcpy(txt,txt+i);
  i=strlen(txt);
  while (i && (txt[i-1]==' ')) i--;
  txt[i]=0;
  return txt;
}

/*
 * int GetPrivateProfileString() Windows API function
 */

/* read a line from input
 * strip spaces
 * cut it at first non-printable character (usu. \r or \n) or at ;, which is
 * considered to be the comment separator.
 * Return number of chars if line read succesfully, -1 otherwise.
 * (19. 3. 2001) Treat escaped newline as two spaces.
 */
inline signed int get_clear_line(char *line, int l, FILE *f)
{
    size_t len=0;
  for(;;) {
	  char * scan;
	  if (NULL==fgets(line, l-len, f)) return -1;
	  _cutspaces(line);
	  for (scan=line; (unsigned char)*scan>=' '; scan++) 
#ifdef WINS    
    if (*scan==';') break;
#else  // ; appears in font description on GTK
    ;
    if (*line==';') scan=line;
#endif
	  len+=(scan-line);
	  if (scan[-1]!='\\' || len==(size_t)l){
		  *scan='\0';
		  return len;
	  }
	  scan[-1]='\n';
	  line=scan;
  }
}

/* Checks if given string conforms to a section definition.
 * If so, also removes final ']' */
inline static bool issection(char *line, int l)
{
	if (*line!='['||line[l-1]!=']') return false;
	line[l-1]='\0';
	return true;
}

/* Returns 0 if line does not contains valid key=value pair.
 * Otherwise, return length of the key and modifie line to contain only that
 * key. */
inline int line2key(char *line)
{
	char *loc=strchr(line, '=');
	if (!loc) return 0;
	*loc='\0';
	return loc-line;
}

/* Move source_length, but at most free_cells characters to the target.
 * If there is not enough room, indicate it by adding two '\0' at the end of
 * the target. Return true if success, false if no memory.
 */
static bool add_to_list(char * target, const char * source, int free_cells,
			int source_length)
{
    if (source_length<free_cells-2){
	    memcpy(target, source, source_length);
	    target[source_length]='\0';  // fullstop, may be overwritten by the next item
	    return true;
    }
    memcpy(target, source, free_cells-2);
    target[free_cells-1]='\0';
    target[free_cells-2]='\0';
    return false;
}

/* Implementation:
 *   - copy the default - must not be NULL!
 *   - if the incoming text is section name,
 *      - if Section is null, add it to the buffer
 *      - else check if it matches needed section
 *   - otherwise, if we are not in right section, ignore the row
 *   - if the incoming text is a (key, value) pair, check if key matches
 */

int GetPrivateProfileString(const char * lpszSection,  const char * lpszEntry,  const char * lpszDefault,
            char * lpszReturnBuffer, int cbReturnBuffer, const char * lpszFilename)
{
    char line[1024];  FILE * f;  BOOL my_section;
    int passed_chars=0; /* used when or section or entry is NULL */
    int l;
    char *p; 
  assert(lpszDefault!=NULL);
  _strmaxcpy(lpszReturnBuffer, lpszDefault, cbReturnBuffer);
  f=fopen(lpszFilename, "rb");
  if (!f) return strlen(lpszReturnBuffer);
  my_section=FALSE;
  while (-1!=(l=get_clear_line(line, sizeof(line), f)))
  {
    if (issection(line, l)){
	    if (lpszSection) my_section=!strcasecmp(line+1, lpszSection);
	    else if (!add_to_list(lpszReturnBuffer+passed_chars,
				 line+1, cbReturnBuffer-passed_chars, --l)){
		    fclose(f);
		    return cbReturnBuffer-2;
	    	}
	    	else passed_chars+=l;
	    continue;
    }
    if (!my_section) continue;
    if (lpszEntry==NULL){
	    if (0==(l=line2key(line))) continue;
	    if (!add_to_list(lpszReturnBuffer+passed_chars, line,
			    cbReturnBuffer-passed_chars, ++l)){
		    fclose(f);
		    return cbReturnBuffer-2;
	    }
	    passed_chars+=l;
	    continue;
    }
    if (strncasecmp(line, lpszEntry, l=strlen(lpszEntry))) continue;
    p=line+l;
    while (*p==' ') p++;
    if (*p!='=') continue;  p++;
    while (*p==' ') p++;
    _strmaxcpy(lpszReturnBuffer, p, cbReturnBuffer);
    fclose(f);
    return strlen(lpszReturnBuffer);
  }
  fclose(f);
  if (NULL==lpszSection||NULL==lpszEntry) return passed_chars;
  return 0;
}

BOOL _str2uns(const char ** ptxt, uns32 * val)  /* without end-check */
{ uns8 c;  uns32 v=0;
  while (**ptxt==' ') (*ptxt)++;
  c=**ptxt;
  if ((c<'0')||(c>'9')) return FALSE;
  do
  { (*ptxt)++;
    if ((v==429496729) && (c>'5') || v>429496729)
      return FALSE;  /* cannot be converted to sig32 */
    v=10*v+(c-'0');
    c=**ptxt;
  } while ((c>='0') && (c<='9'));
  *val=v;
  while (**ptxt==' ') (*ptxt)++;
  return TRUE;
}


BOOL _str2int(const char * txt, sig32 * val)
{ BOOL neg;  uns32 v;
  while (*txt==' ') txt++;
  if (!*txt) { *val=NONEINTEGER; return TRUE; }
  neg=*txt=='-';
  if ((*txt=='-')||(*txt=='+')) txt++;
  if (!_str2uns(&txt, &v)) return FALSE;
  if (v>2147483647) return FALSE;  // outside the signed type range
  if (*txt) return FALSE;  /* end check */
  else { *val=neg ? -(sig32)v : (sig32)v;  return TRUE; }
}


unsigned int GetPrivateProfileInt(const char * lpszSection,  const char * lpszEntry,  int defaultval, const char * lpszFilename)
{ char buf[100];  int val;
  if (!GetPrivateProfileString(lpszSection, lpszEntry, "", buf, sizeof(buf), lpszFilename))
    return defaultval;
  if (_str2int(buf, &val))
      return val;
  return defaultval;
}

BOOL WritePrivateProfileString(const char * lpszSection,  const char * lpszEntry, const char * lpszString, const char * lpszFilename)
// Deletes the entry iff lpszString==NULL
{ char line[1024];  FILE * fi, * fo;  BOOL my_section=FALSE, added=FALSE;
  char newname[160];  int hnd;
#ifdef IN_PLACE_TEMP  // problems when user has no rights to create files in the directory
  strcpy(newname, lpszFilename);
  strcpy(newname+strlen(newname)-3, "NEW");
#else
  strcpy(newname, "/tmp/602sqlXXXXXX");
  hnd=mkstemp(newname);
  if (hnd==-1) { perror(newname); return FALSE; }
  close(hnd);
#endif
  fi=fopen(lpszFilename, "rb");
  if (!fi && errno!=ENOENT)
    { perror(lpszFilename);  return FALSE; }
  fo=fopen(newname, "wb");
  if (!fo) { perror(newname); if (fi) fclose(fi);  return FALSE; }
  my_section=FALSE;
  if (fi)
  { while (fgets(line, sizeof(line), fi))
    { int l;
      _cutspaces(line);
      l=strlen(line);
      while (l && ((line[l-1]=='\r') || (line[l-1]=='\n'))) l--;
      line[l]=0;
    // analyse the clear line:
      if (line[0]=='[')
      { if (my_section && !added && lpszEntry)    // end of my section, add it  // V.B. Ruseni cele sekce, kdyz lpszEntry == NULL
        { if (lpszString)
            { fputs(lpszEntry, fo);  fputs("=", fo);  fputs(lpszString, fo);  fputs("\n", fo); }
          added=TRUE;
        }
        if ((l>2) && (line[l-1]==']'))
        { line[l-1]=0;
          my_section=!strcasecmp(line+1, lpszSection);
          line[l-1]=']';
        }
        else my_section=FALSE;
        if (!my_section || lpszEntry)             // V.B.
          fputs(line, fo);  fputs("\n", fo);
      }
      else if (my_section)           // V.B.
      { if (lpszEntry) 
        { char * p=line;  const char * q=lpszEntry;
          while (*p && (_csconv[*p] == _csconv[*q])) { p++; q++; }
          while (*p==' ') p++;
          if (!*q && *p=='=')  // replace the line
          { if (lpszString)
            { fputs(lpszEntry, fo);  fputs("=", fo);  fputs(lpszString, fo);  fputs("\n", fo); }
            added=TRUE;
          }
          else // other entry from my section
          { fputs(line, fo);  fputs("\n", fo); }
        }
      }
      else  // entry from other section
        { fputs(line, fo);  fputs("\n", fo); }
    }
    fclose(fi);
  }
  if (!added && lpszEntry && lpszString)                     // V.B.
  { if (!my_section)
   	  { fputs("[", fo);   fputs(lpszSection, fo);  fputs("]\n", fo); }
    fputs(lpszEntry, fo);  fputs("=", fo);  fputs(lpszString, fo);
    fputs("\n", fo);
  }
  fclose(fo);
#ifdef IN_PLACE_TEMP
  if (unlink(lpszFilename)) return FALSE;
  if (rename(newname, lpszFilename)) return FALSE;
  return TRUE;
#else
  // works for different filesystems, too
  { int hFrom, hTo;  BOOL ok=TRUE;
    hFrom=open(newname, O_RDONLY);
    if (hFrom==-1) ok=FALSE;
    else
    { hTo=open(lpszFilename, O_WRONLY|O_TRUNC|O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
      if (hTo==-1) { perror(lpszFilename);  ok=FALSE; }
      else
      { char buf[512];  int rd;
        do
        { rd=read(hFrom, buf, sizeof(buf));
          if (rd==-1) { ok=FALSE;  break; }
          if (!rd) break;
          if (write(hTo, buf, rd) != rd) { ok=FALSE;  break; }
        } while (true);
        close(hTo);
      }
      close(hFrom);
      if (ok) unlink(newname);
    }
    return ok;
  }
#endif
}

BOOL RenameSectionInPrivateProfile(const char * lpszSection,  const char * lpszNewSection, const char * lpszFilename)
// Deletes the entry iff lpszString==NULL
{ char line[1024];  FILE * fi, * fo;  BOOL my_section;
  char newname[160];  int hnd;
#ifdef IN_PLACE_TEMP  // problems when user has no rights to create files in the directory
  strcpy(newname, lpszFilename);
  strcpy(newname+strlen(newname)-3, "NEW");
#else
  strcpy(newname, "/tmp/602sqlXXXXXX");
  hnd=mkstemp(newname);
  if (hnd==-1) { perror(newname); return FALSE; }
  close(hnd);
#endif
  fi=fopen(lpszFilename, "rb");
  if (!fi){ perror(lpszFilename); return FALSE;}
  fo=fopen(newname, "wb");
  if (!fo) { perror(newname); fclose(fi);  return FALSE; }
  while (fgets(line, sizeof(line), fi))
  { int l;
    _cutspaces(line);
    l=strlen(line);
    while (l && ((line[l-1]=='\r') || (line[l-1]=='\n'))) l--;
    line[l]=0;
   // analyse the clear line:
    if (line[0]=='[')
    { if ((l>2) && (line[l-1]==']'))
      { line[l-1]=0;
	my_section=!strcasecmp(line+1, lpszSection);
        line[l-1]=']';
        if (my_section)
          { fputs("[", fo);  fputs(lpszNewSection, fo);  fputs("]\n", fo);  continue; }
      }
    }
    fputs(line, fo);  fputs("\n", fo);
  }
  fclose(fo);
  fclose(fi);
#ifdef IN_PLACE_TEMP
  if (unlink(lpszFilename)) return FALSE;
  if (rename(newname, lpszFilename)) return FALSE;
  return TRUE;
#else
  // works for different filesystems, too
  { int hFrom, hTo;  BOOL ok=TRUE;
    hFrom=open(newname, O_RDONLY);
    if (hFrom==-1) ok=FALSE;
    else
    { hTo=open(lpszFilename, O_WRONLY|O_TRUNC|O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
      if (hTo==-1) ok=FALSE;
      else
      { char buf[512];  int rd;
        do
        { rd=read(hFrom, buf, sizeof(buf));
          if (rd==-1) { ok=FALSE;  break; }
          if (!rd) break;
          if (write(hTo, buf, rd) != rd) { ok=FALSE;  break; }
        } while (true);
        close(hTo);
      }
      close(hFrom);
      if (ok) unlink(newname);
    }
    return ok;
  }
#endif
}

