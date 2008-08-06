// httpsupp.c - support for tunnelling via http

bool t_buffered_socket_reader::read_next_part(void)
{ 
  if (terminated) return false;
 // remove the read part of the buffer:
  if (bufpos<bufcont)
  { memmove(buf, buf+bufpos, bufcont-bufpos);
    bufcont=bufcont-bufpos;
  }
  else bufcont=0;
  bufpos=0;
 // read from bufpos till the end of the buffer:
#if defined(SRVR) && !defined(LINUX)
  HANDLE events[2];  events[0] = cdp->hSlaveSem;  events[1] = adr->tcpip_event;
  DWORD rd;
	memset(&adr->ovl, 0, sizeof(OVERLAPPED));	adr->ovl.hEvent = events[1];
	ResetEvent(adr->ovl.hEvent);
	if (ReadFile((FHANDLE)adr->sock, buf+bufcont, buflen-bufcont, &rd, &adr->ovl))  // read OK
  { bufcont+=rd;
    return true;
  }
  if (GetLastError()==ERROR_IO_PENDING)
	{ int who = WaitForMultipleObjects(2, events, FALSE, INFINITE);
	  if (who == WAIT_OBJECT_0+1)     // doslo k udalosti prijmu znaku
	  { if (GetOverlappedResult((FHANDLE)adr->sock, &adr->ovl, &rd, FALSE) && rd != 0) // on W95 sometimes returns len==0
      { bufcont+=rd;    // something received
        return true;
      }
	  }
   // error or hSlaveSem used to stop the slave
    if (who==WAIT_FAILED) GetLastError();
  }
#else  // client or LINUX
  int rd = recv(adr->sock, buf+bufcont, buflen-bufcont, 0);  // rd==0 when connection has been closed 
  if (rd && rd!=SOCKET_ERROR) 
  { bufcont+=rd;    // something received
    return true;
  }
#ifdef WINS  
  WSAGetLastError();
#endif
#endif
 // terminating:
  terminated = true;
  return false;
}

char t_buffered_socket_reader::foll_char(void)
{
  if (bufpos>=bufcont)
    if (!read_next_part()) return 0;
  return buf[bufpos];
}

char t_buffered_socket_reader::next_char(void)
{
  if (bufpos>=bufcont)
    if (!read_next_part()) return 0;
  return buf[bufpos++];
}

char * t_buffered_socket_reader::read_line(void)
// Allocates and return the next input line.  Removes the CRLF and termibates it with null character.
// Returns NULL on alloction error or when the input is empty.
{ enum { step=500 };
  int allocated=0, read=0;
  char * line=NULL;
  do
  {// get next char
    char c;
    c=next_char();
    if (!c) break;  // no more input
   // make space for continuatuion: 
    if (read==allocated)
    { char * newline = (char*)corerealloc(line, allocated+step+1);
      if (!newline) { corefree(line);  return NULL; }
      line=newline;
      allocated+=step;
    }
   // read next part:
    if (c=='\n' && read>0 && line[read-1]=='\r') // LF, CR is the previous char
    { read--;
      if (!read) break;  // must not check for continuation when the line is empty, this may be the terminating line.
      c=foll_char();
      if (c!=' ' && c!='\t')  // unless line continues
        break;
    }
    else
      line[read++]=c;
  } while (true);
  if (line)  // unless input is empty
    line[read]=0;  // terminator
  return line;
}

int t_buffered_socket_reader::read_from_buffer(char * dest, int toread)
{ int rd=0;
  while (toread)
  {// copy available data from the buffer:
    int frombuf = bufcont-bufpos;
    if (frombuf>toread) frombuf=toread;
    memcpy(dest, buf+bufpos, frombuf);
    dest+=frombuf;  rd+=frombuf;  bufpos+=frombuf;  toread-=frombuf;
   // read next data iff needs more:
    if (toread)
      if (!read_next_part())
        break;
  }
  return rd;
}

void send_part(SOCKET sock, const char * data, int length)
{ int bRetCode;  
	unsigned sended = 0;
	do
	{ bRetCode = send(sock, data+sended, length-sended, 0);
    if (bRetCode>0) sended = sended + bRetCode;
    else break;
	} while (sended!=length);
}

char * chunked_trans_enc = "Transfer-Encoding: chunked\r\n";
char * crlf = "\r\n";  

void send_chunked(SOCKET sock, char * body, int len)
{ int sizelen=2, len2=len;
  char sizebuf[10], *bufptr;
  bufptr=sizebuf+sizeof(sizebuf)-2;
  *bufptr='\r';  bufptr[1]='\n';
  do
  { int rest = len % 16;
    len /= 16;
    bufptr--;  sizelen++;
    if (rest<10) *bufptr = '0'+rest;
    else *bufptr = 'A'+(rest-10);
  } while (len);
  send_part(sock, bufptr, sizelen);
  send_part(sock, body, len2);
  send_part(sock, "\r\n0\r\n\r\n", 7);  // data terminator, last chunk and body end
}

void send_direct(SOCKET sock, char * body, int len, int message_type)
// When [message_type] is 0 then no message type is sent (e.g. in the response to the GET request)
{ char buf[30];
  sprintf(buf, "Content-Length: %u\r\n\r\n%c", message_type ? len+1 : len, message_type);
  send_part(sock, buf, (int)strlen(buf));
  send_part(sock, body, len);
}

void send_response(cAddress * pRemAddr, char * startline, char * body, int bodysize, int message_type)
{ SOCKET sock = ((cTcpIpAddress*)pRemAddr)->sock;
  send_part(sock, startline, (int)strlen(startline));
  if (body)
    send_direct(sock, body, bodysize, message_type);
}

extern const char * content_type_apl;
char * content_disp_save = "Content-Disposition: attachment\r\n";

void send_response_text(cAddress * pRemAddr, char * startline, char * body, int bodysize, const char * content_type)
{ SOCKET sock = ((cTcpIpAddress*)pRemAddr)->sock;
  send_part(sock, startline, (int)strlen(startline));
  if (body)
  { if (content_type) 
    { send_part(sock, content_type, (int)strlen(content_type));
      if (!strcmp(content_type, content_type_apl))
        send_part(sock, content_disp_save, (int)strlen(content_disp_save));  // otherwise the form is not opened in the filler
    }
    if (!strcmp(content_type, content_type_apl))
      send_part(sock, "Cache-Control: private\r\n", 24);
    send_direct(sock, body, bodysize, 0);
  }
}

#include "httpsupp2.c"

void free_options(char ** options)
{ for (int i=0; i<HTTP_REQOPT_COUNT; i++) 
    if (options[i])
      { corefree(options[i]);  options[i]=NULL; }
}

int http_receive(cdp_t cdp, t_buffered_socket_reader & bsr, char * & body, int & bodysize, char ** options)
// Returns method number when request received, HTTP_METHOD_NONE when connection closed or error.
// [body] and [bodysize] may be returned. When [body]!=NULL, it must be deallocated.
{ int bodyalloc=0; 
  body = NULL;  bodysize=0;  
  bool chunked_body=false;
 // skip CRLFs:
  char c;
  do c=bsr.foll_char();
  while (c=='\r' || c=='\n');
  if (!c) return HTTP_METHOD_NONE;
 // get start line:
  char * start = bsr.read_line();
  if (!start) return HTTP_METHOD_NONE;
 // analyse the method
  int pos=0;
  char method[30+1];
  while (start[pos] && start[pos]!=' ' && pos<sizeof(method)-1)
    { method[pos]=start[pos];  pos++; }
  method[pos]=0;
  while (start[pos]==' ') pos++;
 // read the URI:
  const char * uri = start+pos;
  while (start[pos] && start[pos]!=' ')
    pos++;
  if (start[pos])
  { start[pos]=0;  // terminate the URI
    pos++;
    while (start[pos]==' ') pos++;
   // now start+pos should be HTTP/1.1
  }
 // read the header:
  bool has_body = false;
  do
  { char * header = bsr.read_line();
    if (!header) break;   // conn closed
    if (!*header) 
      { corefree(header);  break; }  // empty line terminating header
   // find ':':
    char * field_name_end=header;
    while (*field_name_end && *field_name_end!=':') field_name_end++;
    if (*field_name_end)
    { *field_name_end=0;
      field_name_end++;
      while (*field_name_end==' ') field_name_end++;
     // process some header fields:
      if (!stricmp(header, "Content-length"))
      { has_body=true;
        bodysize=atol(field_name_end);
      }
      else if (!stricmp(header, "Transfer-encoding"))
      { has_body=true;
        chunked_body=true;  // supposed
      }
      else if (!stricmp(header, "Content-Type"))
        { if (options) options[HTTP_REQOPT_CONTTP] = alloc(field_name_end); }
      else if (!stricmp(header, "Accept"))
        { if (options) options[HTTP_REQOPT_ACCEPT] = alloc(field_name_end); }
      else if (!stricmp(header, "Authorization"))
        //{ if (options) options[HTTP_REQOPT_AUTH]   = alloc(field_name_end); }
      {
        if (!strnicmp(field_name_end, "Basic ", 6)) 
        {// decode the credentials:
          char * auth_string = field_name_end+6;
          while (*auth_string==' ') auth_string++; 
          t_base64 base64;   base64.reset();
          int len = base64.decode((unsigned char*)auth_string);
          if (len>0) 
          { auth_string[len]=0;
            int pass = 0;
            while (auth_string[pass] && auth_string[pass]!=':') pass++;
            if (pass && pass<len)  // valid username and password
            { auth_string[pass]=0;
              options[HTTP_REQOPT_USERNAME] = alloc(auth_string);
              options[HTTP_REQOPT_PASSWORD] = alloc(auth_string+pass+1);
            }
          }
        }
      }
      else if (!stricmp(header, "Host"))
      { 
#ifdef SRVR  // HTTP_REQOPT_HOST is used and http_web_port_num is available only on the server
        if (options) 
        { options[HTTP_REQOPT_HOST] = (char*)corealloc(strlen(field_name_end)+20, 51); 
          if (options[HTTP_REQOPT_HOST])
            //if (http_web_port_num==80) -- no, host field contains port number
              sprintf(options[HTTP_REQOPT_HOST], "http://%s", field_name_end);
            //else
            //  sprintf(options[HTTP_REQOPT_HOST], "http://%s:%d", field_name_end, http_web_port_num);
        }
#endif
      }
    } 
    // else error in header: does not start with "field:"
    corefree(header);
  } while (true);
 // read the body:
  if (has_body)
  { if (chunked_body) // read chunked body:
    { do
      {// get chunk size:
        char * chunk_size = bsr.read_line();
        if (*chunk_size) break;
        uns32 chsize = 0;  int i=0;
        do
        { if (chunk_size[i]>='0' && chunk_size[i]>='9')
            chsize=16*chsize+chunk_size[i]-'0';
          else if (chunk_size[i]>='A' && chunk_size[i]>='F')
            chsize=16*chsize+chunk_size[i]-'A'+10;
          else if (chunk_size[i]>='a' && chunk_size[i]>='f')
            chsize=16*chsize+chunk_size[i]-'a'+10;
          else break;  // extension ignored
        } while (true);
        corefree(chunk_size);
        if (!chsize) break;  // this was the last chunk
       // realloc the body:
        char * newbody = (char*)corerealloc(body, bodyalloc+chsize);
        if (!newbody) break;
        body=newbody;  bodyalloc+=chsize;
       // read the chunk data:
        bodysize+=bsr.read_from_buffer(body+bodysize, chsize);
        corefree(bsr.read_line());  // drop the CRLF terminator of the chunk data
      } while (true);
     // skip the trailer and the FINAL CRLF:
      do
      { char * trailer_line = bsr.read_line();
        if (!trailer_line) break;
        if (!*trailer_line)  // empty line terminating the chunked body
          { corefree(trailer_line);  break; }
        corefree(trailer_line);
      } while (true);
    }
    else  // read a direct body
    { body = (char*)corealloc(bodysize, 77);
      if (!body) { corefree(start);  return HTTP_METHOD_NONE; }
      bsr.read_from_buffer(body, bodysize);
    }
  }
 // analyse the method:
  int imethod;
  if      (!strcmp(method, "POST"))      imethod=HTTP_METHOD_POST;
  else if (!strcmp(method, "HTTP/1.1"))  imethod=HTTP_METHOD_RESP;
  else if (!strcmp(method, "GET"))       imethod=HTTP_METHOD_GET;
  else                                   imethod=HTTP_METHOD_OTHER;
 // copy the path from the request-URI (uses imethod from the above):
  if (options!=NULL)
  { if (imethod!=HTTP_METHOD_RESP)  // for HTTP_METHOD_RESP URI contains the status-code
      if (*uri!='/')  // when it does not start by '/' then it is the complete URL
        for (int i=0;  uri && i<3;  i++)
          uri = strchr(uri+1, '/');
    if (uri)  // found 3rd '/', [uri] point to it
    { char * dest = options[HTTP_REQOPT_PATH] = (char*)corealloc(strlen(uri)+1, 77);
      if (!dest) { corefree(body);  body=NULL;  corefree(start);  return HTTP_METHOD_NONE; }
#ifdef STOP  // the original form of the URI is needed
      int i=0, o=0;
      while (uri[i])
      { if (uri[i]=='%' && uri[i+1] && uri[i+2])
        { dest[o]= 16*hexdig(uri[i+1]) + hexdig(uri[i+2]);
          i+=3;
        }
        else dest[o] = uri[i++];
        o++;
      }
      dest[o] = 0;
#else
      strcpy(dest, uri);
#endif
    }
  }
  corefree(start);
  return imethod;
}

#ifndef SRVR

char * post_head = "POST / HTTP/1.1\r\n";

BOOL send_http(cAddress *pRemAddr, t_fragmented_buffer * frbu, int message_type)
{// prepare the complete message:
  unsigned total_length = frbu->total_length();
  frbu->reset_pointer();
  BYTE * pBuffSend;
	if (total_length>TCPIP_BUFFSIZE)
       pBuffSend = new BYTE[total_length];  // creates the extended send buffer
  else pBuffSend = ((cTcpIpAddress*)pRemAddr)->send_buffer;
  frbu->copy((char*)pBuffSend, total_length);
 // send the POST request:
  SOCKET sock = ((cTcpIpAddress*)pRemAddr)->sock;
  send_part(sock, post_head, (int)strlen(post_head));
  char cont_len[50];
  sprintf(cont_len, "Content-Length: %u\r\n\r\n%c", message_type ? total_length+1 : total_length, message_type);
  send_part(sock, cont_len, (int)strlen(cont_len));
  send_part(sock, (char*)pBuffSend, total_length);
 // delete the extended send buffer:
  if (total_length>TCPIP_BUFFSIZE) delete [] pBuffSend; 
  return TRUE;
}

#endif


#ifdef SRVR  // XML support

void write_params_into_scope(const t_url_param * params, t_rscope * rscope_par)
{ if (!params) return;
 // convert DAD params to upper case:
  for (int i = 0;  i<rscope_par->sscope->locdecls.count();  i++)
  { t_locdecl * locdecl = rscope_par->sscope->locdecls.acc0(i);
    sys_Upcase(locdecl->name);
  }
  while (params->name[0])
  { t_locdecl * locdecl = rscope_par->sscope->find_name(params->name, LC_VAR);
    if (locdecl)
    { char * dataptr = rscope_par->data + locdecl->var.offset;
      superconv(ATT_STRING, locdecl->var.type, params->value, dataptr, t_specif(0, CHARSET_NUM_UTF8, 0, 0), locdecl->var.specif, "");
    }
    params++;
  }
}

t_rscope * make_scope_from_params(const t_url_param * params)
{ if (!params) return NULL;
 // get count and size of parameters:
  int count=0, extent=0;
  while (params[count].name[0])
  { extent += (int)strlen(params[count].value)+1;
    count++;
  }
  t_scope * scope = new t_scope(FALSE);
  t_rscope * rscope = new(extent) t_rscope(scope);
  if (!scope || !rscope) return NULL;
  int pos=0;
  for (int i = 0;  i<count;  i++)
  { t_locdecl * locdecl = scope->locdecls.next();
    if (locdecl==NULL) break;
   // create the variable:
    strmaxcpy(locdecl->name, params[i].name, sizeof(locdecl->name));
    sys_Upcase(locdecl->name);
    locdecl->loccat=LC_VAR;
    locdecl->var.type  = ATT_STRING;
    int parlen = (int)strlen(params[i].value);
    locdecl->var.specif=t_specif(parlen,CHARSET_NUM_UTF8,0,0).opqval;
    locdecl->var.offset=pos;
    locdecl->var.defval=NULL;
    locdecl->var.structured_type=false;
   // copy the value:
    strcpy(rscope->data + pos, params[i].value);
    //superconv(ATT_STRING, ATT_STRING, params[i].value, rscope->data + pos, t_specif(0, CHARSET_NUM_UTF8, 0, 0), locdecl->var.specif, "");
    pos += parlen+1;
  }
  scope->extent = extent;
  rscope->level = 0;
  return rscope;
}

void capitalize_scope_names(t_scope * scope)
{ 
  for (int i = 0;  i<scope->locdecls.count();  i++)
  { t_locdecl * locdecl = scope->locdecls.acc0(i);
    sys_Upcase(locdecl->name);
  }
}

typedef BOOL WINAPI t_srv_Export_to_XML_CLOB(cdp_t cdp, const char * dad_ref, t_value * clob);
typedef BOOL WINAPI t_srv_Import_from_XML_CLOB(cdp_t cdp, const char * dad_ref, t_value * clob);
typedef BOOL WINAPI t_srv_ImpExp_from_XML_CLOB(cdp_t cdp, const char * dad_ref, t_value * clob, t_value * outxml);
typedef BOOL WINAPI t_srv_Export_to_XML_CLOB(cdp_t cdp, const char * dad_ref, t_value * clob);
typedef t_rscope * WINAPI t_srv_Get_dad_rscope(cdp_t cdp, char * dad_ref);
typedef void       WINAPI t_srv_Free_dad_rscope(cdp_t cdp, t_rscope * rscope);
typedef int WINAPI t_ZipData(const void *Data, int DataSize, void **Zip, int *ZipSize);

#define ZIP602_STATIC
#include "Zip602.h"

int ZipData(const void *Data, int DataSize, void **Zip, int *ZipSize)
{   *Zip     = NULL;
    *ZipSize = 0;
    INT_PTR hZip;
    int Err = ZipCreate(NULL, NULL, &hZip);
    if (Err != Z_OK)
        return Z_OK;
    Err = ZipAddM(hZip, Data, DataSize, L"form.fo");
    if (Err == Z_OK)
    {   QWORD Size;
        Err = ZipGetSize(hZip, &Size);
        if (Err == Z_OK)
        {   void *z = corealloc((int)Size, 111);
            if (!z)
                Err = Z_MEM_ERROR;
            else
            {   Err = ZipCloseM(hZip, z, (int)Size);
                if (Err != Z_OK)
                    corefree(z);
                else
                {   *Zip     = z;
                    *ZipSize = (int)Size;
                }
            }
        }
    }
    return Err;
}

char * compress_data(char * input, int inputlen, int & outputlen)
// Allocates the output buffer, deallocates the input buffer. Writes [outputlen].
{ void * zip;
  int res = ZipData(input, inputlen, &zip, &outputlen);
  if (res!=0/*Z_OK*/)
    return input;  
  corefree(input);
  return (char*)zip;
}

BOOL export_from_dad(cdp_t cdp, HINSTANCE hInst, const char * dadname, t_url_param * params, char **resp)
{ BOOL res;  t_value outxml;
  char dadref[1+OBJ_NAME_LEN+1];  *dadref='*';  strcpy(dadref+1, dadname);
  t_srv_Export_to_XML_CLOB * p_srv_Export_to_XML_CLOB = (t_srv_Export_to_XML_CLOB*)GetProcAddress(hInst, "srv_Export_to_XML_CLOB");
  t_srv_Get_dad_rscope  * p_srv_Get_dad_rscope  = (t_srv_Get_dad_rscope  *)GetProcAddress(hInst, "srv_Get_dad_rscope");
  t_srv_Free_dad_rscope * p_srv_Free_dad_rscope = (t_srv_Free_dad_rscope *)GetProcAddress(hInst, "srv_Free_dad_rscope");
  t_rscope * rscope_par = (*p_srv_Get_dad_rscope)(cdp, dadref);
  if (rscope_par)  // unless error
  { capitalize_scope_names((t_scope *)rscope_par->sscope);
    write_params_into_scope(params, rscope_par);    
    rscope_par->next_rscope=cdp->rscope;  cdp->rscope=rscope_par;
  }
  res = (*p_srv_Export_to_XML_CLOB)(cdp, dadref, &outxml);
  if (rscope_par)  // unless error
  { cdp->rscope=rscope_par->next_rscope;
    (*p_srv_Free_dad_rscope)(cdp, rscope_par);
  }
  if (res)
    *resp = alloc(outxml.valptr());
  return res;
}
                           
char * export_error = "XML export error.\r\n\r\n";
//"<html><body>XML export error.</body></html>";

int get_xml_response(cdp_t cdp, int method, char ** options, const char * body, int bodysize, 
                      char ** resp, const char ** content_type, char * realm, int & resplen)
{ 
  tobjname server, schema, objname;  t_url_param * params = NULL;  char * query = NULL;  t_webxml_req category;
  char msg[100+2*OBJ_NAME_LEN];  bool specobj;  tobjnum objnum;  t_auth_req req_auth;
  *content_type = content_type_html;
  *resp=NULL;  resplen=0;  // to be safe

  if (!analyse_uri(options[HTTP_REQOPT_PATH], sys_spec.charset, server, schema, &category, objname, &params, &query, &req_auth))
    { *resp = alloc("This is a 602SQL server emulating a web server.<p>URL format error.");  goto reterr; }
 // authenticate:
  if (realm) strcpy(realm, server_name);
  if (category!=WXR_STATUS || *server)
    if (!options[HTTP_REQOPT_USERNAME] || !options[HTTP_REQOPT_PASSWORD]) 
    { if (req_auth==AR_ALWAYS || req_auth==AR_DEFAULT && the_sp.DisableHTTPAnonymous.val()) 
        return 401; 
    }
    else
    { if (!web_login(cdp, options[HTTP_REQOPT_USERNAME], options[HTTP_REQOPT_PASSWORD]))  // authentification failed
        return 401;
    }
 // trace (after login):
  trace_msg_general(cdp, TRACE_WEB_REQUEST, options[HTTP_REQOPT_PATH], NOOBJECT);
 // find and set the schema:
  if (category!=WXR_STATUS)
  { tobjnum aplobj;
    ker_set_application(cdp, schema, aplobj);
    if (aplobj==NOOBJECT)
    { sprintf(msg, "Schema %s not found on the server.", schema);
      *resp = alloc(msg);  goto reterr;
    }
  }
  //bool needs_login = request_needs_login(cdp, 
 // find the object:
  specobj = !strcmp(objname, "*");
  
  switch (category)
  { case WXR_STATUS:
      *resp = *server ? alloc("The web XML interface to the 602SQL server is running.") : 
                        alloc("This is a 602SQL server emulating a web server.");
      break;
    case WXR_XMLFORM:    // returns the XML form 
    case WXR_XMLFORM_Z:  // returns the compressed XML form
    {// check privils & load the object:
      objnum = find2_object(cdp, CATEG_XMLFORM, NULL, objname);
      if (objnum==NOOBJECT)
      { sprintf(msg, "Object %s not found in schema %s.", objname, schema);
        *resp = alloc(msg);  goto reterr;
      }
      WBUUID query_schema_uuid;
      fast_table_read(cdp, objtab_descr, objnum, APPL_ID_ATR, query_schema_uuid); // same as child
      if (!cdp->has_full_access(query_schema_uuid))
        if (!can_read_objdef(cdp, objtab_descr, objnum))
          { request_error(cdp, NO_RIGHT);  goto err; }
      tptr buf=ker_load_objdef(cdp, objtab_descr, objnum);
      if (buf==NULL) goto err;  /* request_error called inside */
     // translate XML form (using own server name, not from the URI):
      char * params = make_web_access_params(options[HTTP_REQOPT_HOST], the_sp.HostPathPrefix.val(), schema, server_name);
      *resp = merge_form_and_data(buf, NULL, params);
      corefree(params);
      corefree(buf);
      *content_type = content_type_apl;  // this instructs FireFox to open the Filler, but does not help in IE.
      if (category==WXR_XMLFORM_Z)  // compress the result
        *resp=compress_data(*resp, (int)strlen(*resp), resplen);
      break;
    }
    case WXR_STYLESHEET:  // returns the stylesheet
      if (specobj)
        *resp=alloc(ssh2);
      else
      {// check privils & load the object:
        objnum = find2_object(cdp, CATEG_STSH, NULL, objname);
        if (objnum==NOOBJECT)
        { sprintf(msg, "Object %s not found in schema %s.", objname, schema);
          *resp = alloc(msg);  goto reterr;
        }
        WBUUID query_schema_uuid;
        fast_table_read(cdp, objtab_descr, objnum, APPL_ID_ATR, query_schema_uuid); // same as child
        if (!cdp->has_full_access(query_schema_uuid))
          if (!can_read_objdef(cdp, objtab_descr, objnum))
            { request_error(cdp, NO_RIGHT);  goto err; }
        tptr buf=ker_load_objdef(cdp, objtab_descr, objnum);
        if (buf==NULL) goto err;  /* request_error called inside */
        *resp=buf;
      }
      *content_type = content_type_xml;  // for stylesheets, IE accepts content_type_xsl too but FireFox does not!
      break;
    case WXR_SHOWQUERY:
    case WXR_QUERYDATA:
    { HINSTANCE hInst;
      extension_object * exobj = search_obj_in_ext(cdp, "EXPORT_TO_XML_CLOB", XML_EXT_NAME, &hInst);
      if (!exobj)
        { *resp = alloc("The SQL server XML extension not loaded.");  goto reterr; }
      t_srv_Export_to_XML_CLOB * p_srv_Export_to_XML_CLOB = (t_srv_Export_to_XML_CLOB*)GetProcAddress(hInst, exobj->expname);
      t_value outxml;  
      { t_scope scope(0);
        t_rscope * rscope_fict = create_and_activate_rscope(cdp, scope, 0);
        t_rscope * rscope_par = make_scope_from_params(params);
        if (rscope_par)
          { rscope_par->next_rscope=cdp->rscope;  cdp->rscope=rscope_par; }
        BOOL res = (*p_srv_Export_to_XML_CLOB)(cdp, query, &outxml);
        if (rscope_par)
          { cdp->rscope=rscope_par->next_rscope;  delete rscope_par->sscope;  delete rscope_par; }
        deactivate_and_drop_rscope(cdp);
        if (!res)
          { *resp = alloc(export_error);  goto reterr; }
      }
      *resp = alloc(outxml.valptr());
      *content_type = content_type_xml;
      break;
    }
    case WXR_DAD:
    case WXR_DADX:
    {// compile the DAD:
      BOOL res;  
      HINSTANCE hInst;
      extension_object * exobj = search_obj_in_ext(cdp, method==HTTP_METHOD_GET ? "EXPORT_TO_XML_CLOB" : "IMPEXP_FROM_XML_CLOB", XML_EXT_NAME, &hInst);
      if (!exobj)
        { *resp = alloc("The SQL server XML extension not loaded.");  goto reterr; }
      if (method==HTTP_METHOD_GET)
      { res=export_from_dad(cdp, hInst, objname, params, resp);
        if (!res)
          { *resp = alloc(export_error);  goto reterr; }
        *content_type = content_type_xml;
      }
      else  // POST methods
      { char dadref[1+OBJ_NAME_LEN+1];  *dadref='*';  strcpy(dadref+1, objname);
        t_srv_Get_dad_rscope  * p_srv_Get_dad_rscope  = (t_srv_Get_dad_rscope  *)GetProcAddress(hInst, "srv_Get_dad_rscope");
        t_srv_Free_dad_rscope * p_srv_Free_dad_rscope = (t_srv_Free_dad_rscope *)GetProcAddress(hInst, "srv_Free_dad_rscope");
        t_srv_ImpExp_from_XML_CLOB * p_srv_ImpExp_from_XML_CLOB = (t_srv_ImpExp_from_XML_CLOB*)GetProcAddress(hInst, exobj->expname);
        t_value outxml, inxml;  //t_scope scope(0);
        inxml.allocate(1+bodysize+1);
        char * p = inxml.valptr();
        if (category==WXR_DADX)
        { *(p++)='!';  // signal for the import procedure
          memcpy(p, body, bodysize);
          p+=bodysize;
        }
        else
          for (int i=0;  i<bodysize;  i++, p++)
            if (body[i]=='\r' || body[i]=='\n') 
              *p=' ';
            else
              *p=body[i];
        *p=0;  inxml.length=bodysize;
        //t_rscope * rscope_fict = create_and_activate_rscope(cdp, scope, 0);
        t_rscope * rscope_par = (*p_srv_Get_dad_rscope)(cdp, dadref);
        if (rscope_par)  // is NULL on error in dad name
        { capitalize_scope_names((t_scope *)rscope_par->sscope);
          write_params_into_scope(params, rscope_par);    
          rscope_par->next_rscope=cdp->rscope;  cdp->rscope=rscope_par;
        }
        res = (*p_srv_ImpExp_from_XML_CLOB)(cdp, dadref, &inxml, &outxml);
        //deactivate_and_drop_rscope(cdp);
        if (rscope_par)  // is NULL on error in dad name
        { cdp->rscope=rscope_par->next_rscope;
          (*p_srv_Free_dad_rscope)(cdp, rscope_par);
        }
        if (!outxml.is_null())
        { *resp = alloc(outxml.valptr());
          *content_type = !memcmp(*resp, "<?xml", 5) ? content_type_xml : content_type_html;
        }
        else if (!res)
          goto err;  // return standard error report
      }
      break;
    }
    case WXR_COMPOSE:
    {// make temp. copy of the beginning of the absolute path from the URL:
      const char * bsl = strrchr(options[HTTP_REQOPT_PATH], '/');
      bsl--;
      while (*bsl!='/') bsl--;
      int len=(int)(bsl-options[HTTP_REQOPT_PATH]);
      char * path = (char*)corealloc(len+1, 77);
      if (!path) goto reterr;
      strmaxcpy(path, options[HTTP_REQOPT_PATH], len+1);
     // compose the new URL:
      char * dadname = query+strlen(query)+1;
      if (*dadname)
      { const char * patt = acx2;
        const char * last_bsl = strrchr(options[HTTP_REQOPT_PATH], '/');
        const char * parptr = strchr(last_bsl+1, '?');
        if (!parptr) parptr="";
        *resp = (char*)corealloc(strlen(patt)+2*strlen(options[HTTP_REQOPT_HOST])+2*strlen(path)+strlen(query)+strlen(dadname)+strlen(parptr)+1, 88);
        sprintf(*resp, patt, options[HTTP_REQOPT_HOST], path, query, options[HTTP_REQOPT_HOST], path, dadname, parptr);
      }
      else
      { const char * patt = acx1;
        *resp = (char*)corealloc(strlen(patt)+strlen(options[HTTP_REQOPT_HOST])+strlen(path)+strlen(query)+1, 88);
        sprintf(*resp, patt, options[HTTP_REQOPT_HOST], path, query);
      }
      corefree(path);
      break;
    }
    case WXT_FORMANDDATA:
    {// export the XML:
      char * dadname = query+strlen(query)+1;  bool compress = false;
      HINSTANCE hInst;
      extension_object * exobj = search_obj_in_ext(cdp, "EXPORT_TO_XML_CLOB", XML_EXT_NAME, &hInst);
      char * data = NULL;
      if (*dadname)
      { compress = trim_and_test(dadname);
        export_from_dad(cdp, hInst, dadname, params, &data);
      }
      else  // no data, just the form
        compress = trim_and_test(query);
     // load the form:
      WBUUID schema_uuid;  
      objnum = find2_object(cdp, CATEG_XMLFORM, NULL, query);
      fast_table_read(cdp, objtab_descr, objnum, APPL_ID_ATR, schema_uuid); // same as child
      if (!cdp->has_full_access(schema_uuid))
        if (!can_read_objdef(cdp, objtab_descr, objnum))
          { request_error(cdp, NO_RIGHT);  goto err; }
      tptr buf=ker_load_objdef(cdp, objtab_descr, objnum);
      if (buf==NULL) goto err;  /* request_error called inside */
     // translate XML form (using own server name, not from the URI):
      char * params = make_web_access_params(options[HTTP_REQOPT_HOST], the_sp.HostPathPrefix.val(), schema, server_name);
      *resp = merge_form_and_data(buf, data, params);
      corefree(params);
      corefree(buf);
      corefree(data);
      *content_type = content_type_apl;  // this instructs FireFox to open the Filler, but does not help in IE.
      if (compress)
        *resp=compress_data(*resp, (int)strlen(*resp), resplen);
      break;
    }
  }
  corefree(query);
  dealloc_params(params);  
  if (*resp && !resplen) resplen = (int)strlen(*resp); // if there is a response and its length has not been calculated above
  return 200;

 err:  // create error message from the current error:
  { char xbuf[350];
    Get_server_error_text(cdp, cdp->get_return_code(), xbuf, 350);  // returns the message in the system charset
    *resp = alloc(xbuf);
  }
 reterr:
  corefree(query);
  dealloc_params(params);  
  if (*resp && !resplen) resplen = (int)strlen(*resp); // if there is a response and its length has not been calculated above
  return 400;
}

char * get_web_access_params(cdp_t cdp) 
{ char url[90];
 // find the host name and port
  bool emulation = ((wProtocol & TCPIP_WEB) != 0) && the_sp.using_emulation.val();
  int port = emulation ? the_sp.web_port.val() : the_sp.ext_web_server_port.val();
  if (port==80)
    sprintf(url, "http://%s", the_sp.HostName.val());
  else
    sprintf(url, "http://%s:%d", the_sp.HostName.val(), port);
  return make_web_access_params(url, the_sp.HostPathPrefix.val(), cdp->sel_appl_name, server_name);
}

#else  // !SRVR

#endif

