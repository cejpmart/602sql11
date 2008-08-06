// cnetapi.h

#ifdef LINUX
typedef int SOCKET;
#endif

class cTcpIpAddress;

class t_buffered_socket_reader
{ enum { buflen=1024};
  char buf[buflen];
  int bufcont, bufpos;
  cdp_t cdp;
 public:
  cTcpIpAddress * adr;
  bool terminated;
  bool read_next_part(void);
  char foll_char(void);
  char next_char(void);
  char * read_line(void);
  t_buffered_socket_reader(cdp_t cdpIn)
    { cdp=cdpIn;  bufcont=bufpos=0;  terminated=false;  adr = (cTcpIpAddress*)cdp->pRemAddr; }
  t_buffered_socket_reader(cTcpIpAddress * adrIn)
    { cdp=NULL;  bufcont=bufpos=0;  terminated=false;  adr = adrIn; }
  int read_from_buffer(char * dest, int toread);
};

extern char * ok_response;
extern const char * content_type_xml;
extern const char * content_type_html;

int http_receive(cdp_t cdp, t_buffered_socket_reader & bsr, char * & body, int & bodysize, char ** options = NULL);
void send_direct(SOCKET sock, char * body, int len, int message_type);
void send_part(SOCKET sock, const char * data, int length);
void send_response(cAddress * pRemAddr, char * startline, char * body, int bodysize, int message_type);
#ifdef SRVR
int get_xml_response(cdp_t cdp, int method, char ** options, const char * body, int bodysize, 
                      char ** resp, const char ** content_type, char * realm, int & resplen);
#endif
void send_response_text(cAddress * pRemAddr, char * startline, char * body, int bodysize, const char * content_type);
void free_options(char ** options);

