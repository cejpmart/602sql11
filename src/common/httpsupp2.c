// httpsupp2.c - analysis of the URL for client and server
#if WBVERS<900 
#include "flstr.h"
#include "../xmlext/dad.h"
#endif

int hexdig(char c)
{ return c>='0' && c<='9' ? c-'0' : c>='A' && c<='F' ? c-'A'+10 : c>='a' && c<='f' ? c-'a'+10 : 0; }

char * alloc(const char * str)
{ char * all = (char*)corealloc(strlen(str)+1, 71);
  if (all) strcpy(all, str);
  return all;
}

/* Supported methods and URLs:
GET  .../server/schema/DAD/dadname     - exports XML data from the DAD, returns XML content
POST .../server/schema/DAD/dadname     - imports XML data from the body using the DAD
GET  .../server/schema/FORM/formname   - returns the XML form design stored in the database as XML content
GET  .../server/schema/STYLESHEET/formname   - returns the stylesheet stored in the database as XML content
GET  .../server/schema/STYLESHEET/*          - returns the default stylesheet as XML content
GET  .../server/schema/SHOWQUERY/sqlquery    - returns the answer to the query as XML content with the stylesheet reference ../FORM/*
GET  .../server/schema/QUERYDATA/sqlquery    - returns the answer to the query as the default XML data with column names, XML content, no stylesheet 
GET  .../server/schema/COMPOSE/formname dadname   - returns a HTML page with ActiveX object referencing the form design stored in the database and the DAD
GET  .../server/schema/STATUS/*              - returns the server status
*/

bool convert_procent(const char * src, int ilen, char * dest, int dest_size)
// Copies [ilen] bytes of [src] string to [dest]. Replaces %-notation.
// Returns false when the result is longer than [dest_size].
{
  int iind=0, oind=0;
  while (iind<ilen)
  { if (oind>=dest_size-1) return false;
    if (src[iind]=='%' && iind+2<ilen)
    { dest[oind]= 16*hexdig(src[iind+1]) + hexdig(src[iind+2]);
      iind+=3;
    }
    else 
      dest[oind] = src[iind++];
    oind++;
  }
  dest[oind]=0;
  return true;
}

struct t_url_param
{ tobjname name;
  char * value;  // may be NULL when parameter name is specified but the value is not
};

bool analyse_params(t_url_param ** params, const char * parptr, int sys_charset)
// Creates and returns array of parameters, terminated by the parameter with an empty name.
// On error the function returns false and the array may be created partially, but it is terminated.
{
 // count the parameters:
  int count=0, pos=0;
  while (parptr[pos])
  { count++;
   // go to the next parameter:
    while (parptr[pos] && parptr[pos]!='&') pos++;
    if (parptr[pos]) pos++;  // skips the & delimiter
  }
 // allocate the array for the paramaters:
  *params = (t_url_param*)corealloc(sizeof(t_url_param)*(count+1), 51);
  if (!*params) return false;  // error
 // copy parameters into the array:
  t_url_param * par;
  count=pos=0;
  while (parptr[pos])
  { par = (*params) + count;
    count++;
    par[1].name[0]=0;    // the terminator of the array (moves instantly, makes the deallocation possible in any momment)
    par->value=NULL;
   // analyse and copy the parameter:
    // name:
    int start = pos;
    while (parptr[pos] && parptr[pos]!='&' && parptr[pos]!='=') pos++;
    if (parptr[pos]!='=' || pos==start) return false;  // parameter syntax error (missing name or '=')
    char parname[2*OBJ_NAME_LEN+1];  // is in UTF-8
    if (!convert_procent(parptr+start, pos-start, parname, sizeof(parname))) 
      return false;  // name too long
    if (superconv(ATT_STRING, ATT_STRING, parname, par->name, t_specif(0, CHARSET_NUM_UTF8, 0, 0), t_specif(OBJ_NAME_LEN, sys_charset, 0, 0), NULL))
      return false;  // unconvertible or too long after convertion
    convert_to_uppercaseA(par->name, par->name, sys_charset);
    // value:
    pos++;
    start=pos;  // start of the value
    while (parptr[pos] && parptr[pos]!='&') pos++;
    if (pos>start)
    { par->value = (char*)corealloc(pos-start+1, 51);
      if (!par->value) return false;
      convert_procent(parptr+start, pos-start, par->value, pos-start+1);
    }
    if (parptr[pos]) pos++;  // skips the & delimiter
  }
  return true;
}

void dealloc_params(t_url_param * params)
{ 
  if (params==NULL) return;
  for (t_url_param * par = params;  par->name[0];  par++)
    corefree(par->value);
  corefree(params);
}

inline int _max(int a, int b) 
{ return a<b ? b : a; }

#ifdef STOP  // not used any more, forms cannot contain placeholders

const char * HOST_NAME_PLHLD        = "%host_name%";
const char * HOST_PATH_PREFIX_PLHLD = "%host_path_prefix%";
const char * HOST_SQL_SCHEMA_PLHLD  = "%host_sql_schema%";
const char * HOST_SQL_SERVER_PLHLD  = "%host_sql_server%";

int count_occs(const char * buf, const char * patt)
{ int cnt=0;
  do
  { char * occ = strstr(buf, patt);
    if (!occ) return cnt;
    cnt++;
    buf = occ+1;
  } while (true);
}

void replace_occs(char * buf, const char * repl, const char * patt)
{ 
  int len1=strlen(patt), len2 = strlen(repl);
  do
  { char * occ = strstr(buf, patt);
    if (!occ) break;
   // replacing:
    memmov(occ+len2, occ+len1, strlen(occ)-len1+1);
    memcpy(occ, repl, len2);
    buf=occ+len2;
  } while (true);
}

char * translate_form(char * buf, const char * host_name, const char * host_path_prefix, const char * host_sql_schema, 
                      const char * host_sql_server)
// Replaces the placeholders in [buf] and returns the new [buf].
{
  int cnt1, cnt2, cnt3, cnt4;
  cnt1 = count_occs(buf, HOST_NAME_PLHLD);
  cnt2 = count_occs(buf, HOST_PATH_PREFIX_PLHLD);
  cnt3 = count_occs(buf, HOST_SQL_SCHEMA_PLHLD);
  cnt4 = count_occs(buf, HOST_SQL_SERVER_PLHLD);
  if (!cnt1 && !cnt2 && !cnt3 || !cnt4) return buf;
  int new_length = strlen(buf) + cnt1*(_max(strlen(host_name),       strlen(HOST_NAME_PLHLD)))
                               + cnt2*(_max(strlen(host_path_prefix),strlen(HOST_PATH_PREFIX_PLHLD)))
                               + cnt3*(_max(strlen(host_sql_schema), strlen(HOST_SQL_SCHEMA_PLHLD)))
                               + cnt4*(_max(strlen(host_sql_server), strlen(HOST_SQL_SERVER_PLHLD))) + 1;
  char * new_buf = (char*)corealloc(new_length, 71);
  if (!new_buf) return buf; // error->not translating
  strcpy(new_buf, buf);
  corefree(buf);
  replace_occs(new_buf, host_name,        HOST_NAME_PLHLD);
  replace_occs(new_buf, host_path_prefix, HOST_PATH_PREFIX_PLHLD);
  replace_occs(new_buf, host_sql_schema,  HOST_SQL_SCHEMA_PLHLD);
  replace_occs(new_buf, host_sql_server,  HOST_SQL_SERVER_PLHLD);
  return new_buf;
}

#endif

enum t_webxml_req { WXR_STATUS, WXR_DAD, WXR_DADX, WXR_STYLESHEET, WXR_XMLFORM, WXR_XMLFORM_Z, 
                    WXR_SHOWQUERY, WXR_QUERYDATA, WXR_COMPOSE, WXT_FORMANDDATA }; 

bool trim_and_test(char * p)
// Removes any suffix, returns true if the suffix was zfo.
{ while (*p) 
    if (*p=='.') 
    { *p=0;  
      return !strcmp(p+1, "zfo");
    }
    else p++;
  return false;
}

enum t_auth_req { AR_DEFAULT, AR_ALWAYS, AR_NEVER };

bool analyse_uri(const char * url, int sys_charset, char * server_name, char * schema_name, t_webxml_req * category, 
                 char * object_name, t_url_param **params, char **query, t_auth_req * req_auth)
// URL may have the %-notation, is supposed to be in UTF-8.
// Writes [server_name], [schema_name], [category], may write [object_name].
// May allocate [params], [query].
{ *req_auth=AR_DEFAULT;
 // divide the URL into main parts:
  const char * last_bsl = strrchr(url, '/');
  if (!last_bsl) return false;
  if (url==last_bsl)  // URL without any / inside -> produce a standard message
  { strcpy(object_name, "*");  *category=WXR_STATUS;  *server_name=0;  // this combination produces a special message
    return true;
  }
  const char * object_ptr = last_bsl+1;
  const char * parptr = strchr(object_ptr, '?');
  const char * p = last_bsl;
  do if (p==url) return false;  else p--;  while (*p!='/');
  const char * categ_name=p+1;
  do if (p==url) return false;  else p--;  while (*p!='/');
  const char * schema_ptr=p+1;
  if (p==url) return false;
  do p--;  while (p>url && *p!='/');
  const char * server_ptr = *p=='/' ? p+1 : p;
  if (server_ptr>=schema_ptr-1) return false;  // server name empty
 // copy and convert parts:
  if (!convert_procent(server_ptr, (int)(schema_ptr-1-server_ptr), server_name, sizeof(tobjname)))
    return false;  // server name too long
  char schema_utf8[2*sizeof(tobjname)+1];
  if (!convert_procent(schema_ptr, (int)(categ_name-1-schema_ptr), schema_utf8, sizeof(schema_utf8)))
    return false;  // schema name too long in UTF-8
  if (superconv(ATT_STRING, ATT_STRING, schema_utf8, schema_name, t_specif(0, CHARSET_NUM_UTF8, 0, 0), t_specif(OBJ_NAME_LEN, sys_charset, 0, 0), NULL))
    return false;  // unconvertible or too long after convertion
  convert_to_uppercaseA(schema_name, schema_name, sys_charset);
  char cat_name[20];
  if (!convert_procent(categ_name, (int)(last_bsl-categ_name), cat_name, sizeof(cat_name)))
    return false;  // categ name too long
  if (cat_name[0]=='!') 
    { *req_auth=AR_ALWAYS;  memmove(cat_name, cat_name+1, strlen(cat_name)); }
  else if (cat_name[0]=='-') 
    { *req_auth=AR_NEVER;   memmove(cat_name, cat_name+1, strlen(cat_name)); }
 // analyse the category:
  if (!stricmp(cat_name, "DAD"))         *category=WXR_DAD;         else
  if (!stricmp(cat_name, "DADX"))        *category=WXR_DADX;        else
  if (!stricmp(cat_name, "STYLESHEET"))  *category=WXR_STYLESHEET;  else
  if (!stricmp(cat_name, "FORM"))        *category=WXR_XMLFORM;     else
  if (!stricmp(cat_name, "SHOWQUERY"))   *category=WXR_SHOWQUERY;   else
  if (!stricmp(cat_name, "QUERYDATA"))   *category=WXR_QUERYDATA;   else
  if (!stricmp(cat_name, "COMPOSE"))     *category=WXR_COMPOSE;     else
  if (!stricmp(cat_name, "FORMANDDATA")) *category=WXT_FORMANDDATA; else
  if (!stricmp(cat_name, "STATUS"))      *category=WXR_STATUS;      else
    return false;  // undefined category
 // analyse object:
  int qlen = (int)(parptr ? parptr - object_ptr : strlen(object_ptr));
  if (*category==WXR_SHOWQUERY || *category==WXR_QUERYDATA)  // query source is on input, add + or .
  { *object_name=0;  // safety
    char *q1 = (char*)corealloc(qlen+1, 77);
    if (!q1) return false;
    if (!convert_procent(object_ptr, qlen, q1, qlen + 1)) return false;
    char *q2 = (char*)corealloc(1+qlen+1, 77);
    if (!q2) return false;
    *q2 = *category==WXR_SHOWQUERY ? '.' : '+';
    if (superconv(ATT_STRING, ATT_STRING, q1, q2+1, t_specif(0, CHARSET_NUM_UTF8, 0, 0), t_specif(0, sys_charset, 0, 0), NULL))
      { corefree(q1);  return false; }  // unconvertible 
    corefree(q1);
    *query=q2;
  }
  else if (*category==WXR_COMPOSE || *category==WXT_FORMANDDATA)
  { *object_name=0;  // safety
    const char * delim; // = strstr(object_ptr, "%20");  -- delimiter changed to !
    int delimlen;
    //if (delim && (!parptr || delim<parptr))
    //  delimlen=3;
    //else
    { delim = strstr(object_ptr, "!");
      if (delim && (!parptr || delim<parptr)) 
        delimlen=1;
      else
      { delim=object_ptr+qlen;  // DAD name is empty
        delimlen=0;
      }
    }
#if 0  // do not convert, copying this into the new URL
   // convert part 1:
    char objname_utf8[2*sizeof(tobjname)+1];
    if (!convert_procent(object_ptr, delim-object_ptr, objname_utf8, sizeof(objname_utf8)))
      return false;  // object name too long in UTF-8
    if (superconv(ATT_STRING, ATT_STRING, objname_utf8, object_name, t_specif(0, CHARSET_NUM_UTF8, 0, 0), t_specif(OBJ_NAME_LEN, sys_charset, 0, 0), NULL))
      return false;  // unconvertible or too long after convertion
    convert_to_uppercaseA(object_name, object_name, sys_charset);
   // convert part 2:
    int q2len = parptr ? parptr - (delim+3) : strlen(delim+3);
    if (!convert_procent(delim+3, q2len, objname_utf8, sizeof(objname_utf8)))
      return false;  // object name too long in UTF-8
    char *q2 = (char*)corealloc(sizeof(tobjname), 77);
    if (!q2) return false;
    if (superconv(ATT_STRING, ATT_STRING, objname_utf8, q2, t_specif(0, CHARSET_NUM_UTF8, 0, 0), t_specif(OBJ_NAME_LEN, sys_charset, 0, 0), NULL))
      return false;  // unconvertible or too long after convertion
    convert_to_uppercaseA(q2, q2, sys_charset);
    *query=q2;
#endif
    char *q = (char*)corealloc(qlen+1, 77);
    if (!q) return false;
    int len1 = (int)(delim-object_ptr);
    memcpy(q, object_ptr, len1);  q[len1]=0;
    if (delimlen)
      { memcpy(q+len1+1, delim+delimlen, qlen-len1-delimlen);  q[qlen-delimlen+1]=0; }
    else q[len1+1]=0;
    *query=q;
  }
  else
  { char objname_utf8[2*(sizeof(tobjname)+4)+1];
    if (!convert_procent(object_ptr, qlen, objname_utf8, sizeof(objname_utf8)))
      return false;  // object name too long in UTF-8
    if (*category==WXR_XMLFORM)
    { int ulen = (int)strlen(objname_utf8);
      if (ulen>3 && !memcmp(objname_utf8+ulen-3, ".fo", 3))
        objname_utf8[ulen-3] = 0;  // remove the suffix
      if (ulen>4 && !memcmp(objname_utf8+ulen-4, ".zfo", 4))
      { objname_utf8[ulen-4] = 0;  // remove the suffix
        *category=WXR_XMLFORM_Z;
      }
    }
    if (superconv(ATT_STRING, ATT_STRING, objname_utf8, object_name, t_specif(0, CHARSET_NUM_UTF8, 0, 0), t_specif(OBJ_NAME_LEN, sys_charset, 0, 0), NULL))
      return false;  // unconvertible or too long after convertion
    convert_to_uppercaseA(object_name, object_name, sys_charset);
  }
  if (parptr)
    return analyse_params(params, parptr+1, sys_charset);
  return true;
}

const char * content_type_xml = "Content-Type: text/xml\r\n";
const char * content_type_html = "Content-Type: text/html\r\n";
const char * content_type_xsl = "Content-Type: text/xsl\r\n";
const char * content_type_apl = "Content-Type: application/force-download\r\n";

#define acx1 "<html>"\
    "<body id=\"MyBody\">"\
            "<OBJECT CLASSID=\"clsid:672EE252-D813-4F5E-81BB-5DD163DD4FA5\" ALIGN=\"CENTER\" WIDTH=\"100%%\""\
                " HEIGHT=\"100%%\" NAME=\"Formular\" ID=\"Formular\" VIEWASTEXT codeBase=\"http://www.602.cz/602xml/filleractivex.cab#version=-1,-1,-1,-1\">"\
                "<PARAM NAME=\"FormFileName\" VALUE=\"%s%s/FORM/%s\">"\
            "</OBJECT>"\
    "</body>"\
"</html>"

#define acx2 "<html>"\
    "<body id=\"MyBody\">"\
            "<OBJECT CLASSID=\"clsid:672EE252-D813-4F5E-81BB-5DD163DD4FA5\" ALIGN=\"CENTER\" WIDTH=\"100%%\""\
                " HEIGHT=\"100%%\" NAME=\"Formular\" ID=\"Formular\" VIEWASTEXT codeBase=\"http://www.602.cz/602xml/filleractivex.cab#version=-1,-1,-1,-1\">"\
                "<PARAM NAME=\"FormFileName\" VALUE=\"%s%s/FORM/%s\">"\
                "<PARAM NAME=\"DataFileName\" VALUE=\"%s%s/DAD/%s%s\">"\
            "</OBJECT>"\
    "</body>"\
"</html>"

#define ssh "<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
"<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">"\
"<xsl:output method=\"html\" encoding=\"windows-1250\"/>"\
"<xsl:template match=\"/\">"\
  "<html><head><title>602tabulka</title></head>"\
    "<body><h2>602tabulka</h2><xsl:apply-templates/></body>"\
  "</html>"\
"</xsl:template>"\
"<xsl:template match=\"root\">"\
  "<table border=\"1\"><xsl:apply-templates/></table>"\
  "<hr/>"\
"</xsl:template>"\
"<xsl:template match=\"row\">"\
  "<tr><xsl:apply-templates/></tr>"\
        "</xsl:template>"\
        "<xsl:template match=\"cell\">"\
        "<td><xsl:choose>"\
        "<xsl:when test=\"string(.)=''\">&#160;</xsl:when><xsl:otherwise><xsl:value-of select=\".\"/></xsl:otherwise>"\
        "</xsl:choose></td>"\
        "</xsl:template>"\
        "</xsl:stylesheet>"

#define ssh2 "<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
"<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">"\
"<xsl:output method=\"html\" encoding=\"utf-8\"/>"\
"<xsl:template match=\"/\">"\
  "<html>"\
    "<head><title>602tabulka</title></head>"\
    "<body>"\
      "<h2>Data from <xsl:value-of select=\"//@Query\"/></h2>"\
      "<table border=\"1\">"\
      "<xsl:for-each select=\"/*/*[1]\">"\
        "<tr>"\
        "<xsl:for-each select=\"*\">"\
          "<td><strong><xsl:value-of select=\"name()\"/></strong></td>"\
        "</xsl:for-each>"\
         "</tr>"\
      "</xsl:for-each>"\
      "<xsl:for-each select=\"/*/*\">"\
        "<tr>"\
        "<xsl:for-each select=\"*\">"\
          "<td>"\
          "<xsl:choose>"\
            "<xsl:when test=\"string(.)=''\">&#160;</xsl:when>"\
            "<xsl:otherwise><xsl:value-of select=\".\"/></xsl:otherwise>"\
          "</xsl:choose>"\
          "</td>"\
        "</xsl:for-each>"\
        "</tr>"\
      "</xsl:for-each>"\
      "</table>"\
    "</body>"\
  "</html>"\
"</xsl:template>"\
"</xsl:stylesheet>"
/////////////////////////////////////// base 64 /////////////////////////////////////////////
inline int d64(char c)
{ return c>='a' ? c-'a'+26 : c>='A' ? c-'A' : c>='0' ? c-'0'+52 : c=='+' ? 62 : 63; }

int t_base64::decode(unsigned char * tx)
// Returns binary length or -1 on error
{ int bytecnt = 0, allbytes = 0;  unsigned char bytes[3];  unsigned char * byteptr = tx;
  while (*tx)
  { if (*tx>' ')  // whitespaces ignored!
    { ext_chars[ext_cnt++]=*tx;
      if (ext_cnt==4)
      {// save previous 3 bytes:
        memcpy(byteptr, bytes, bytecnt);  byteptr+=bytecnt;  allbytes+=bytecnt;
       // decode 4 chars from base64:
        bytes[0] = (d64(ext_chars[0])<<2) + (d64(ext_chars[1])>>4);
        if (ext_chars[2]=='=')
          bytecnt=1;  // double '=' on the end
        else 
        { bytes[1] = (d64(ext_chars[1])<<4) + (d64(ext_chars[2])>>2);
          if (ext_chars[3]=='=')
            bytecnt=2;  // single '=' on the end
          else
          { bytes[2] = (d64(ext_chars[2])<<6) + d64(ext_chars[3]);
            bytecnt=3;  // full count of chars
          }
        }
        ext_cnt=0;
      }
    }
    tx++;
  }
  memcpy(byteptr, bytes, bytecnt);  byteptr+=bytecnt;  allbytes+=bytecnt;
  return allbytes;
}

/////////////////////////////////////////////////////////////////////////////
char * make_web_access_params(const char * url_pref, const char * host_path_prefix, const char * schema_name, const char * server_name)
{
  int len = (int)strlen(url_pref) + (int)strlen(host_path_prefix) + (int)strlen(schema_name) + (int)strlen(server_name) + 200;
  char * buf = (char*)corealloc(len, 88);
  if (buf)
    sprintf(buf, "<host_name>%s</host_name><host_path_prefix>%s</host_path_prefix><host_sql_schema>%s</host_sql_schema><host_sql_server>%s</host_sql_server>",
            url_pref, host_path_prefix, schema_name, server_name);
  return buf;
}

//#include <fstream.h>

char * merge_form_and_data(const char * formdef, const char * data, const char * params)
// [formdef] may containf params, but is supposed not to contain data.
{ char * result;
  const char * pos = strstr(formdef, "</dsig:Signature>");
  if (!pos) 
  { //  return NULL;
    result = (char*)corealloc(100, 88);
    strcpy(result, "<html><body>The form is not signed.\n</body></html>");
    return result;
  }
 // search existing params:
  const char * oldpars_start = strstr(formdef, "<dsig:Object "), * oldpars_end;
  while (oldpars_start)
  { const char * p = oldpars_start+13;
    bool found = false; 
    while (true)
    { // Find next attribute
      while (*p==' ') p++;
      if (!strnicmp(p, "Id='form-params'", (size_t)16) || !strnicmp(p, "Id=\"form-params\"", (size_t)16))
      { found = true;
        break;  // existing params found!
      }
      char c = 0;
      while (true) 
      { p++;
        c = *p;
        if (c == ' ' || c == '>')
          break;
        if (c == '"' || c == '\'')
        { do
            p++;
          while (*p != c);
        }
      }
      if (c == '>')
        break;
    }
    if (found)
      break;
    oldpars_start = strstr(p, "<dsig:Object ");
  }
  if (oldpars_start)
  { oldpars_end=strstr(oldpars_start, "</dsig:Object>");
    if (!oldpars_end) oldpars_start=NULL;  // internal error, ignoring oldpars
  }
 // skip the header of [data]:
  if (data)
  { while (!memcmp(data, "<?", 2))
      data=strstr(data, "?>")+2;
    if (data==(const char*)2) return NULL;  // data forma error
  }
 //  allocate output buffer:
  int buflen = (int)strlen(formdef)+1;
  if (data)   buflen+=(int)strlen(data)  +40;
  if (params) buflen+=(int)strlen(params)+60;
  result = (char*)corealloc(buflen, 88);
  if (!result) return NULL;
 // make the result - take the beginning of the form:
  int len = (int)((oldpars_start ? oldpars_start : pos) - formdef);
  memcpy(result, formdef, len);
 // append data:
  if (data)
  { memcpy(result+len, "<dsig:Object>",  13);  len+=13;
    strcpy(result+len, data);  len+=(int)strlen(data);
    memcpy(result+len, "</dsig:Object>", 14);  len+=14;
  }
 // append params and the end of the form definition:
  if (params)
  { memcpy(result+len, "<dsig:Object Id='form-params'>", 30);  len+=30;
    strcpy(result+len, params);  len+=(int)strlen(params);
    memcpy(result+len, "</dsig:Object>",                 14);  len+=14;
    strcpy(result+len, pos);
  }
  else  // use the old params, when new params not specified
    if (oldpars_start)
      strcpy(result+len, oldpars_start);
    else
      strcpy(result+len, pos);
#ifdef STOP  // just for debugging
  ofstream ofs("d:\\temp\\exp.fo", ios::out|ios::binary);
  ofs << result;
  ofs.close();
#endif
  return result;
}

