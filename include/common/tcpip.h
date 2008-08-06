//
//  tcpip.h - header of tcpip.cpp
// 	=============================
//

BOOL TcpIpPresent(void);

BYTE TcpIpProtocolStart(void);
BYTE TcpIpProtocolStop(cdp_t cdp);

BYTE TcpIpAdvertiseStart(bool force_net_interface);
BYTE TcpIpAdvertiseStop(void);

#ifndef SRVR
BOOL TcpIpQueryInit(void);
BOOL TcpIpQueryDeinit(void);
int  TcpIpQueryForServer(void);

BOOL CreateTcpIpAddress(char * ip_addr, unsigned port, cAddress **ppAddr, bool is_http);
#endif

BOOL TcpIpReceiveStart(void);
BOOL TcpIpReceiveStop(void);
void prepare_socket_on_port_80(const char * server_name);

#define TCPIP_BUFFSIZE  16384

#define SQP_FOR_SERVER		0x01
#define SQP_BREAK					0x02
#define SQP_SERVER_NAME		0x04
#define SQP_KEEP_ALIVE		0x08
#define SQP_SERVER_NAME8	0x11
#define SQP_SERVER_NAME95	0x12
#if WBVERS<950
#define SQP_SERVER_NAME_CURRENT  SQP_SERVER_NAME8
#else
#define SQP_SERVER_NAME_CURRENT  SQP_SERVER_NAME95
#endif
