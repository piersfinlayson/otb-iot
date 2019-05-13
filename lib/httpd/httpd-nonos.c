/*
ESP8266 web server - platform-dependent routines, nonos version
*/

#include <esp8266.h>
#include "httpd.h"
#include "platform.h"
#include "httpd-platform.h"

#ifndef FREERTOS

//Listening connection data
static struct espconn httpdConn;
static esp_tcp httpdTcp;

//Set/clear global httpd lock.
//Not needed on nonoos.
void ICACHE_FLASH_ATTR httpdPlatLock() {
}
void ICACHE_FLASH_ATTR httpdPlatUnlock() {
}


static void ICACHE_FLASH_ATTR platReconCb(void *arg, sint8 err) {
	//From ESP8266 SDK
	//If still no response, considers it as TCP connection broke, goes into espconn_reconnect_callback.

	ConnTypePtr conn=arg;
	//Just call disconnect to clean up pool and close connection.
	httpdDisconCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port);
}

static void ICACHE_FLASH_ATTR platDisconCb(void *arg) {
	ConnTypePtr conn=arg;
	httpdDisconCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port);
}

static void ICACHE_FLASH_ATTR platRecvCb(void *arg, char *data, unsigned short len) {
	ConnTypePtr conn=arg;
	httpd_printf("platRecvCb entry\n");
	httpd_printf("  stack %p\n", &conn);
	httpd_printf("  conn %p\n", conn);
	httpd_printf("  data %p\n", data);
	httpd_printf("  len %d\n", len);
	httpd_printf("  data is:\n");
	for (int ii; ii < len; ii++)
	{
		httpd_printf("%c", data[ii]);
	}
	httpd_printf("\n");
	httpdRecvCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port, data, len);
	httpd_printf("platRecvCb exit\n");
}

static void ICACHE_FLASH_ATTR platSentCb(void *arg) {
	ConnTypePtr conn=arg;
	httpdSentCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port);
}

static void ICACHE_FLASH_ATTR platConnCb(void *arg) {
	ConnTypePtr conn=arg;
	if (httpdConnectCb(conn, (char*)conn->proto.tcp->remote_ip, conn->proto.tcp->remote_port)) {
		espconn_regist_recvcb(conn, platRecvCb);
		espconn_regist_reconcb(conn, platReconCb);
		espconn_regist_disconcb(conn, platDisconCb);
		espconn_regist_sentcb(conn, platSentCb);
	} else {
		espconn_disconnect(conn);
	}
}


int ICACHE_FLASH_ATTR httpdPlatSendData(ConnTypePtr conn, char *buff, int len) {
	int r;
	httpd_printf("httdPlatSendData %d\n", len);
	r=espconn_sent(conn, (uint8_t*)buff, len);
	httpd_printf("httdPlatSendData %d\n", r>=0);
	return (r>=0);
}

void ICACHE_FLASH_ATTR httpdPlatDisconnect(ConnTypePtr conn) {
	espconn_disconnect(conn);
}

void ICACHE_FLASH_ATTR httpdPlatDisableTimeout(ConnTypePtr conn) {
	//Can't disable timeout; set to 2 hours instead.
	espconn_regist_time(conn, 7199, 1);
}

//Initialize listening socket, do general initialization
void ICACHE_FLASH_ATTR httpdPlatInit(int port, int maxConnCt) {
	httpdConn.type=ESPCONN_TCP;
	httpdConn.state=ESPCONN_NONE;
	httpdTcp.local_port=port;
	httpdConn.proto.tcp=&httpdTcp;
	espconn_regist_connectcb(&httpdConn, platConnCb);
	espconn_accept(&httpdConn);
	espconn_tcp_set_max_con_allow(&httpdConn, maxConnCt);
}


#endif