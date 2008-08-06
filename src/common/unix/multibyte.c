#include <iconv.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>  // GCC 4.0.3 nees it
#include <string.h>  // GCC 4.0.3 nees it

typedef unsigned short wuchar;  // same as in wbdefs.h
// iconv does something special when converting '\0'. Writing terminator explicitly.
/**
 *  WideCharToMultiByte - convert from UCS-2
 *  @charset: name of character set
 *  @wstr: text to convert
 *  @len: number of characters to convert (not bytes)
 *  @buffer: buffer to hold output
 *
 */
int WideCharToMultiByte(const char *charset, const char *wstr, size_t len, char *buffer)
// Returns 0 in case of failure, number of written bytes otherwise
{
  char *from=(char *)wstr;
  char *to=buffer;
  size_t tolen=len;
  size_t fromlen=2*tolen;
  iconv_t conv=iconv_open(charset, "UCS-2");
  if (conv==(iconv_t)-1) goto err;
  while (fromlen>0){
    int nconv=iconv(conv, &from, &fromlen, &to, &tolen);
    if (nconv==(size_t)-1){
      iconv_close(conv);
      goto err;
    }
  }
  *to='\0';  // adding a terminator - not counted
  iconv_close(conv);
  return to-buffer;
 err:
  if (len>=4) strcpy(buffer, "!cnv");
  else *buffer=0;
  return 0;
}

int MultiByteToWideChar(const char *charset, const char *str, size_t len, wuchar*buffer)
// Returns 0 in case of failure, number of written wide chars otherwise
{
  char *from=(char *)str;
  wuchar *to=buffer;
  size_t fromlen=len;
  size_t tolen=2*fromlen;
  iconv_t conv=iconv_open("UCS-2", charset);
  if (conv==(iconv_t)-1) goto err;
  while (fromlen>0){
    int nconv=iconv(conv, &from, &fromlen, (char **)&to, &tolen);
    if (nconv==(size_t)-1){
      iconv_close(conv);
      goto err;
    }
  }
  *to='\0';  // adding a wide terminator - not counted
  iconv_close(conv);
  return to-buffer;
 err:
  if (len>=4) memcpy(buffer, "!\000c\000n\000v\000\000\000", 10);
  else *buffer=0;  // wide
  return 0;

}
