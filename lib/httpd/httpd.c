/*
Esp8266 http server - core routines
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <esp8266.h>
#include "httpd.h"
#include "httpd-platform.h"

//This gets set at init time.
static HttpdBuiltInUrl *builtInUrls;

typedef struct HttpSendBacklogItem HttpSendBacklogItem;

struct HttpSendBacklogItem {
	int len;
	HttpSendBacklogItem *next;
	char data[];
};

//Flags
#define HFL_HTTP11 (1<<0)
#define HFL_CHUNKED (1<<1)
#define HFL_SENDINGBODY (1<<2)
#define HFL_DISCONAFTERSENT (1<<3)
#define HFL_NOCONNECTIONSTR (1<<4)

//Private data for http connection
struct HttpdPriv {
	char head[HTTPD_MAX_HEAD_LEN];
	int headPos;
	char *sendBuff;
	int sendBuffLen;
	char *chunkHdr;
	HttpSendBacklogItem *sendBacklog;
	int sendBacklogSize;
	int flags;
};


//Connection pool
static HttpdConnData *connData[HTTPD_MAX_CONNECTIONS];

//Struct to keep extension->mime data in
typedef struct {
	const char *ext;
	const char *mimetype;
} MimeMap;


//#define RSTR(a) ((const char)(a))

//The mappings from file extensions to mime types. If you need an extra mime type,
//add it here.
static const ICACHE_RODATA_ATTR MimeMap mimeTypes[]={
	{"htm", "text/htm"},
	{"html", "text/html"},
	{"css", "text/css"},
	{"js", "text/javascript"},
	{"txt", "text/plain"},
	{"jpg", "image/jpeg"},
	{"jpeg", "image/jpeg"},
	{"png", "image/png"},
	{"svg", "image/svg+xml"},
	{"xml", "text/xml"},
	{"json", "application/json"},
	{NULL, "text/html"}, //default value
};

void dump_stack(void)
{
	void *p = (void *)0x40000000;  // 0x3ffffffc is top of stack.  &p is bottom

	ets_printf("STACK DUMP");

  for (; p > (void *)&p; p = (void *)((uint32)p - 4))
	{
		if (!((int)p % 0x10))
		{
			ets_printf("\r\n%p: ", ((uint32)p)-4);
		}
		ets_printf("%p ", *((uint32*)p-4));
	}
	ets_printf("\r\n");
}

//Returns a static char* to a mime type for a given url to a file.
const char ICACHE_FLASH_ATTR *httpdGetMimetype(char *url) {
	httpd_printf("ENTRY: httpdGetMimetype\n");
	int i=0;
	//Go find the extension
	char *ext=url+(strlen(url)-1);
	while (ext!=url && *ext!='.') ext--;
	if (*ext=='.') ext++;
	
	//ToDo: strcmp is case sensitive; we may want to do case-intensive matching here...
	while (mimeTypes[i].ext!=NULL && strcmp(ext, mimeTypes[i].ext)!=0) i++;
	httpd_printf("EXIT: httpdGetMimetype\n");
	return mimeTypes[i].mimetype;
}

//Looks up the connData info for a specific connection
static HttpdConnData ICACHE_FLASH_ATTR *httpdFindConnData(ConnTypePtr conn, char *remIp, int remPort) {
	httpd_printf("ENTRY: httpdFindConnData\n");
	for (int i=0; i<HTTPD_MAX_CONNECTIONS; i++) {
		if (connData[i] && connData[i]->remote_port == remPort &&
						memcmp(connData[i]->remote_ip, remIp, 4) == 0) {
			connData[i]->conn=conn;
			httpd_printf("EXIT: httpdFindConnData\n");
			return connData[i];
		}
	}
	//Shouldn't happen.
	httpd_printf("*** Unknown connection %d.%d.%d.%d:%d\n", remIp[0]&0xff, remIp[1]&0xff, remIp[2]&0xff, remIp[3]&0xff, remPort);
	httpdPlatDisconnect(conn);
	httpd_printf("EXIT: httpdFindConnData\n");
	return NULL;
}

//Retires a connection for re-use
void ICACHE_FLASH_ATTR httpdRetireConn(HttpdConnData *conn) {
	httpd_printf("ENTRY: httpdRetireConn\n");
	conn->cgi = NULL;
	if (!conn->reserved)
	{
		if (conn->priv->sendBacklog!=NULL) {
			HttpSendBacklogItem *i, *j;
			i=conn->priv->sendBacklog;
			do {
				j=i;
				i=i->next;
				free(j);
				httpd_printf("FREE: httpsendbacklogitem\n");
			} while (i!=NULL);
		}
		if (conn->post->buff!=NULL) {
			free(conn->post->buff);
			httpd_printf("FREE: conn->post->buff\n");
		}
		if (conn->post!=NULL) {
			free(conn->post);
			httpd_printf("FREE: conn->post\n");
		}
		if (conn->priv!=NULL) {
			free(conn->priv);
			httpd_printf("FREE: conn->priv\n");
		}
		if (conn) {
			free(conn);
			httpd_printf("FREE: conn\n");
		}
		for (int i=0; i<HTTPD_MAX_CONNECTIONS; i++) {
			if (connData[i]==conn) connData[i]=NULL;
		}
	}
	httpd_printf("EXIT: httpdRetireConn\n");
}

//Stupid li'l helper function that returns the value of a hex char.
static int ICACHE_FLASH_ATTR  httpdHexVal(char c) {
	httpd_printf("ENTRY: httpdHexVal\n");
	if (c>='0' && c<='9') {
		httpd_printf("EXIT: httpdHexVal\n");
		return c-'0';
	}
	if (c>='A' && c<='F') {
		httpd_printf("EXIT: httpdHexVal\n");
		return c-'A'+10;
	}
	if (c>='a' && c<='f') {
		httpd_printf("EXIT: httpdHexVal\n");
		return c-'a'+10;
	}
	httpd_printf("EXIT: httpdHexVal\n");
	return 0;
}

//Decode a percent-encoded value.
//Takes the valLen bytes stored in val, and converts it into at most retLen bytes that
//are stored in the ret buffer. Returns the actual amount of bytes used in ret. Also
//zero-terminates the ret buffer.
int ICACHE_FLASH_ATTR httpdUrlDecode(char *val, int valLen, char *ret, int retLen) {
	httpd_printf("ENTRY: httpdUrlDecode\n");
	int s=0, d=0;
	int esced=0, escVal=0;
	while (s<valLen && d<retLen) {
		if (esced==1)  {
			escVal=httpdHexVal(val[s])<<4;
			esced=2;
		} else if (esced==2) {
			escVal+=httpdHexVal(val[s]);
			ret[d++]=escVal;
			esced=0;
		} else if (val[s]=='%') {
			esced=1;
		} else if (val[s]=='+') {
			ret[d++]=' ';
		} else {
			ret[d++]=val[s];
		}
		s++;
	}
	if (d<retLen) ret[d]=0;
	httpd_printf("EXIT: httpdUrlDecode\n");
	return d;
}

//Find a specific arg in a string of get- or post-data.
//Line is the string of post/get-data, arg is the name of the value to find. The
//zero-terminated result is written in buff, with at most buffLen bytes used. The
//function returns the length of the result, or -1 if the value wasn't found. The 
//returned string will be urldecoded already.
int ICACHE_FLASH_ATTR httpdFindArg(char *line, char *arg, char *buff, int buffLen) {
	httpd_printf("ENTRY: httpdFindArg\n");
	char *p, *e;
	if (line==NULL) {
		httpd_printf("EXIT: httpdFindArg\n");
		return -1;
	}
	p=line;
	while(p!=NULL && *p!='\n' && *p!='\r' && *p!=0) {
//		httpd_printf("findArg: %s\n", p);
		if (strncmp(p, arg, strlen(arg))==0 && p[strlen(arg)]=='=') {
			p+=strlen(arg)+1; //move p to start of value
			e=(char*)strstr(p, "&");
			if (e==NULL) e=p+strlen(p);
//			httpd_printf("findArg: val %s len %d\n", p, (e-p));
			httpd_printf("EXIT: httpdFindArg\n");
			return httpdUrlDecode(p, (e-p), buff, buffLen);
		}
		p=(char*)strstr(p, "&");
		if (p!=NULL) p+=1;
	}
	httpd_printf("Finding %s in %s: Not found :/\n", arg, line);
	httpd_printf("EXIT: httpdFindArg\n");
	return -1; //not found
}

//Get the value of a certain header in the HTTP client head
//Returns true when found, false when not found.
int ICACHE_FLASH_ATTR httpdGetHeader(HttpdConnData *conn, char *header, char *ret, int retLen) {
	httpd_printf("ENTRY: httpdGetHeader\n");
	char *p=conn->priv->head;
	p=p+strlen(p)+1; //skip GET/POST part
	p=p+strlen(p)+1; //skip HTTP part
	while (p<(conn->priv->head+conn->priv->headPos)) {
		while(*p<=32 && *p!=0) p++; //skip crap at start
		//See if this is the header
		if (strncmp(p, header, strlen(header))==0 && p[strlen(header)]==':') {
			//Skip 'key:' bit of header line
			p=p+strlen(header)+1;
			//Skip past spaces after the colon
			while(*p==' ') p++;
			//Copy from p to end
			while (*p!=0 && *p!='\r' && *p!='\n' && retLen>1) {
				*ret++=*p++;
				retLen--;
			}
			//Zero-terminate string
			*ret=0;
			//All done :)
			httpd_printf("EXIT: httpdGetHeader\n");
			return 1;
		}
		p+=strlen(p)+1; //Skip past end of string and \0 terminator
	}
	httpd_printf("EXIT: httpdGetHeader\n");
	return 0;
}

void ICACHE_FLASH_ATTR httdSetTransferMode(HttpdConnData *conn, int mode) {
	httpd_printf("ENTRY: httdSetTransferMode\n");
	if (mode==HTTPD_TRANSFER_CLOSE) {
		conn->priv->flags&=~HFL_CHUNKED;
		conn->priv->flags&=~HFL_NOCONNECTIONSTR;
	} else if (mode==HTTPD_TRANSFER_CHUNKED) {
		conn->priv->flags|=HFL_CHUNKED;
		conn->priv->flags&=~HFL_NOCONNECTIONSTR;
	} else if (mode==HTTPD_TRANSFER_NONE) {
		conn->priv->flags&=~HFL_CHUNKED;
		conn->priv->flags|=HFL_NOCONNECTIONSTR;
	}
	httpd_printf("EXIT: httdSetTransferMode\n");
}

//Start the response headers.
void ICACHE_FLASH_ATTR httpdStartResponse(HttpdConnData *conn, int code) {
	httpd_printf("ENTRY: httpdStartResponse\n");
	char buff[256];
	int l;
	const char *connStr="Connection: close\r\n";
	if (conn->priv->flags&HFL_CHUNKED) connStr="Transfer-Encoding: chunked\r\n";
	if (conn->priv->flags&HFL_NOCONNECTIONSTR) connStr="";
	l=sprintf(buff, "HTTP/1.%d %d OK\r\nServer: esp8266-httpd/"HTTPDVER"\r\n%s", 
			(conn->priv->flags&HFL_HTTP11)?1:0, 
			code, 
			connStr);
	httpdSend(conn, buff, l);
	httpd_printf("EXIT: httpdStartResponse\n");
}

//Send a http header.
void ICACHE_FLASH_ATTR httpdHeader(HttpdConnData *conn, const char *field, const char *val) {
	httpd_printf("ENTRY: httpdHeader\n");
	httpdSend(conn, field, -1);
	httpdSend(conn, ": ", -1);
	httpdSend(conn, val, -1);
	httpdSend(conn, "\r\n", -1);
	httpd_printf("EXIT: httpdHeader\n");
}

//Finish the headers.
void ICACHE_FLASH_ATTR httpdEndHeaders(HttpdConnData *conn) {
	httpd_printf("ENTRY: httpdEndHeaders\n");
	httpdSend(conn, "\r\n", -1);
	conn->priv->flags|=HFL_SENDINGBODY;
	httpd_printf("EXIT: httpdEndHeaders\n");
}

//Redirect to the given URL.
void ICACHE_FLASH_ATTR httpdRedirect(HttpdConnData *conn, char *newUrl) {
	httpd_printf("ENTRY: httpdRedirect\n");
	httpdStartResponse(conn, 302);
	httpdHeader(conn, "Location", newUrl);
	httpdEndHeaders(conn);
	httpdSend(conn, "Moved to ", -1);
	httpdSend(conn, newUrl, -1);
	httpd_printf("EXIT: httpdRedirect\n");
}

//Use this as a cgi function to redirect one url to another.
int ICACHE_FLASH_ATTR cgiRedirect(HttpdConnData *connData) {
	httpd_printf("ENTRY: cgiRedirect\n");
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		httpd_printf("EXIT: cgiRedirect\n");
		return HTTPD_CGI_DONE;
	}
	httpdRedirect(connData, (char*)connData->cgiArg);
	httpd_printf("EXIT: cgiRedirect\n");
	return HTTPD_CGI_DONE;
}

//Used to spit out a 404 error
static int ICACHE_FLASH_ATTR cgiNotFound(HttpdConnData *connData) {
	httpd_printf("ENTRY: cgiNotFound\n");
	if (connData->conn==NULL) {
		httpd_printf("EXIT: cgiNotFound\n");
		return HTTPD_CGI_DONE;
	}
	httpdStartResponse(connData, 404);
	httpdEndHeaders(connData);
	httpdSend(connData, "404 File not found.", -1);
	httpd_printf("EXIT: cgiNotFound\n");
	return HTTPD_CGI_DONE;
}

//This CGI function redirects to a fixed url of http://[hostname]/ if hostname field of request isn't
//already that hostname. Use this in combination with a DNS server that redirects everything to the
//ESP in order to load a HTML page as soon as a phone, tablet etc connects to the ESP. Watch out:
//this will also redirect connections when the ESP is in STA mode, potentially to a hostname that is not
//in the 'official' DNS and so will fail.
int ICACHE_FLASH_ATTR cgiRedirectToHostname(HttpdConnData *connData) {
	httpd_printf("ENTRY: cgiRedirectToHostname\n");
	static const char hostFmt[]="http://%s/";
	char *buff;
	int isIP=0;
	int x;
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		httpd_printf("EXIT: cgiRedirectToHostname\n");
		return HTTPD_CGI_DONE;
	}
	if (connData->hostName==NULL) {
		httpd_printf("Huh? No hostname.\n");
		httpd_printf("EXIT: cgiRedirectToHostname\n");
		return HTTPD_CGI_NOTFOUND;
	}

	//Quick and dirty code to see if host is an IP
	if (strlen(connData->hostName)>8) {
		isIP=1;
		for (x=0; x<strlen(connData->hostName); x++) {
			if (connData->hostName[x]!='.' && (connData->hostName[x]<'0' || connData->hostName[x]>'9')) isIP=0;
		}
	}
	if (isIP) {
		httpd_printf("EXIT: cgiRedirectToHostname\n");
		return HTTPD_CGI_NOTFOUND;
	}
	//Check hostname; pass on if the same
	if (strcmp(connData->hostName, (char*)connData->cgiArg)==0) {
		httpd_printf("EXIT: cgiRedirectToHostname\n");
		return HTTPD_CGI_NOTFOUND;
	}
	//Not the same. Redirect to real hostname.
	buff=malloc(strlen((char*)connData->cgiArg)+sizeof(hostFmt));
	httpd_printf("MALLOC: redirect buff %d\r\n", strlen((char*)connData->cgiArg)+sizeof(hostFmt));
	if (buff==NULL) {
		//Bail out
		httpd_printf("MALLOC: failed\n");
		httpd_printf("EXIT: cgiRedirectToHostname\n");
		return HTTPD_CGI_DONE;
	}
	sprintf(buff, hostFmt, (char*)connData->cgiArg);
	httpd_printf("REDIRECT: to hostname url %s\n", buff);
	httpdRedirect(connData, buff);
	free(buff);
	httpd_printf("FREE: buff\n");
	httpd_printf("EXIT: cgiRedirectToHostname\n");
	return HTTPD_CGI_DONE;
}


//Same as above, but will only redirect clients with an IP that is in the range of
//the SoftAP interface. This should preclude clients connected to the STA interface
//to be redirected to nowhere.
int ICACHE_FLASH_ATTR cgiRedirectApClientToHostname(HttpdConnData *connData) {
	httpd_printf("ENTRY: cgiRedirectApClientToHostname\n");
#ifndef FREERTOS
	uint32 *remadr;
	struct ip_info apip;
	int x=wifi_get_opmode();
	//Check if we have an softap interface; bail out if not
	if (x!=2 && x!=3) return HTTPD_CGI_NOTFOUND;
	remadr=(uint32 *)connData->remote_ip;
	wifi_get_ip_info(SOFTAP_IF, &apip);
	if ((*remadr & apip.netmask.addr) == (apip.ip.addr & apip.netmask.addr)) {
		httpd_printf("EXIT: cgiRedirectApClientToHostname\n");
		return cgiRedirectToHostname(connData);
	} else {
		httpd_printf("EXIT: cgiRedirectApClientToHostname\n");
		return HTTPD_CGI_NOTFOUND;
	}
#else
	return HTTPD_CGI_NOTFOUND;
#endif
	httpd_printf("EXIT: cgiRedirectApClientToHostname\n");
}


//Add data to the send buffer. len is the length of the data. If len is -1
//the data is seen as a C-string.
//Returns 1 for success, 0 for out-of-memory.
int ICACHE_FLASH_ATTR httpdSend(HttpdConnData *conn, const char *data, int len) {
	httpd_printf("ENTRY: httpdSend\n");
	int stack;
#if 0	
	dump_stack();
	ets_printf("  conn %x\r\n", conn);
	ets_printf("  priv %x\r\n", conn->priv);
	ets_printf("  sendBuff %x\r\n", conn->priv->sendBuff);
	ets_printf("  sendBuffLen %d\r\n", conn->priv->sendBuffLen);
	//ets_printf("  data %x\r\n", data);
	ets_printf("  this len %x\r\n", len);
	//ets_printf("  %s\r\n", data);
  ets_printf("  free heap: %d\r\n", system_get_free_heap_size());
	ets_printf("  send data: %s\r\n", data);
	//ets_printf("  %s\r\n", data);
#endif	
	if (conn->conn==NULL) {
		httpd_printf("EXIT: httpdSend\n");
		return 0;
	}
	if (len<0)
	{
		len=strlen(data);
#if 0		
		ets_printf("  strlen: %d\r\n", len);
#endif
	}
	if (len==0) {
		httpd_printf("EXIT: httpdSend\n");
		return 0;
	}
	if (conn->priv->flags&HFL_CHUNKED && conn->priv->flags&HFL_SENDINGBODY && conn->priv->chunkHdr==NULL) {
		if (conn->priv->sendBuffLen+len+6>HTTPD_MAX_SENDBUFF_LEN) {
  		httpd_printf("EXIT: httpdSend\n");
			return 0;
		}
		//Establish start of chunk
		conn->priv->chunkHdr=&conn->priv->sendBuff[conn->priv->sendBuffLen];
		strcpy(conn->priv->chunkHdr, "0000\r\n");
		conn->priv->sendBuffLen+=6;
	}
	if (conn->priv->sendBuffLen+len>HTTPD_MAX_SENDBUFF_LEN) {
		httpd_printf("EXIT: httpdSend\n");
		return 0;
	}
	httpd_printf("  Adding to sendBuff\n");
	httpd_printf("    sendBuffLen %d\n", conn->priv->sendBuffLen);
	httpd_printf("    &data %p\n", data);
	httpd_printf("    data %s\n", data);
	httpd_printf("    data len %d\n", len);
	memcpy(conn->priv->sendBuff+conn->priv->sendBuffLen, data, len);
	conn->priv->sendBuffLen+=len;
#if 0	
	ets_printf("  memcpy done\r\n");
#endif
	httpd_printf("EXIT: httpdSend\n");
	return 1;
}

static char ICACHE_FLASH_ATTR httpdHexNibble(int val) {
	httpd_printf("ENTRY: httpdHexNibble\n");
	val&=0xf;
	if (val<10) {
		httpd_printf("EXIT: httpdHexNibble\n");
		return '0'+val;
	}
	httpd_printf("EXIT: httpdHexNibble\n");
	return 'A'+(val-10);
}

//Function to send any data in conn->priv->sendBuff. Do not use in CGIs unless you know what you
//are doing! Also, if you do set conn->cgi to NULL to indicate the connection is closed, do it BEFORE
//calling this.
void ICACHE_FLASH_ATTR httpdFlushSendBuffer(HttpdConnData *conn) {
	httpd_printf("ENTRY: httpdFlushSendBuffer\n");
	int r, len;
#if 0	
	httpd_printf("  conn %p\n", conn);
	httpd_printf("  conn->conn %p\n", conn->conn);
	httpd_printf("  conn->priv %p\n", conn->priv);
	httpd_printf("  conn->priv->chunkHdr %p\n", conn->priv->chunkHdr);
	httpd_printf("  conn->priv->flags %x\n", conn->priv->flags);
	httpd_printf("  conn->cgi %p\n", conn->cgi);
#endif 	
	if (conn->conn==NULL) {
	  httpd_printf("EXIT: httpdFlushSendBuffer\n");
		return;
	}
#if 0	
	httpd_printf("  conn->priv->sendBuffLen %d\n", conn->priv->sendBuffLen);
#endif	
	if (conn->priv->chunkHdr!=NULL) {
		//We're sending chunked data, and the chunk needs fixing up.
		//Finish chunk with cr/lf
		httpdSend(conn, "\r\n", 2);
		//Calculate length of chunk
		len=((&conn->priv->sendBuff[conn->priv->sendBuffLen])-conn->priv->chunkHdr)-8;
		//Fix up chunk header to correct value
		conn->priv->chunkHdr[0]=httpdHexNibble(len>>12);
		conn->priv->chunkHdr[1]=httpdHexNibble(len>>8);
		conn->priv->chunkHdr[2]=httpdHexNibble(len>>4);
		conn->priv->chunkHdr[3]=httpdHexNibble(len>>0);
		//Reset chunk hdr for next call
		conn->priv->chunkHdr=NULL;
	}
	if (conn->priv->flags&HFL_CHUNKED && conn->priv->flags&HFL_SENDINGBODY && conn->cgi==NULL) {
		//Connection finished sending whatever needs to be sent. Add NULL chunk to indicate this.
		strcpy(&conn->priv->sendBuff[conn->priv->sendBuffLen], "0\r\n\r\n");
		conn->priv->sendBuffLen+=5;
	}
	if (conn->priv->sendBuffLen!=0) {
		r=httpdPlatSendData(conn->conn, conn->priv->sendBuff, conn->priv->sendBuffLen);
		if (!r) {
			//Can't send this for some reason. Dump packet in backlog, we can send it later.
			if (conn->priv->sendBacklogSize+conn->priv->sendBuffLen>HTTPD_MAX_BACKLOG_SIZE) {
				httpd_printf("Httpd: Backlog: Exceeded max backlog size, dropped %d bytes instead of sending them.\n", conn->priv->sendBuffLen);
				conn->priv->sendBuffLen=0;
				httpd_printf("EXIT: httpdFlushSendBuffer\n");
				return;
			}
			HttpSendBacklogItem *i=malloc(sizeof(HttpSendBacklogItem)+conn->priv->sendBuffLen);
			if (i==NULL) {
				httpd_printf("Httpd: Backlog: malloc failed, out of memory!\n");
				httpd_printf("EXIT: httpdFlushSendBuffer\n");
				return;
			}
			httpd_printf("MALLOC: httpsendbacklogitem %d\n", sizeof(HttpSendBacklogItem)+conn->priv->sendBuffLen);
			memcpy(i->data, conn->priv->sendBuff, conn->priv->sendBuffLen);
			i->len=conn->priv->sendBuffLen;
			i->next=NULL;
			if (conn->priv->sendBacklog==NULL) {
				conn->priv->sendBacklog=i;
			} else {
				HttpSendBacklogItem *e=conn->priv->sendBacklog;
				while (e->next!=NULL) e=e->next;
				e->next=i;
			}
			conn->priv->sendBacklogSize+=conn->priv->sendBuffLen;
		}
		conn->priv->sendBuffLen=0;
	}
	httpd_printf("EXIT: httpdFlushSendBuffer\n");
}

void ICACHE_FLASH_ATTR httpdCgiIsDone(HttpdConnData *conn) {
	httpd_printf("ENTRY: httpdCgiIsDone\n");
	if (conn == NULL)
	{
		httpd_printf("Eeep!\n");
	}
	conn->cgi=NULL; //no need to call this anymore
	if (conn->priv->flags&HFL_CHUNKED) {
		httpd_printf("Pool slot %d is done. Cleaning up for next req\n", conn->slot);
		httpdFlushSendBuffer(conn);
		//Note: Do not clean up sendBacklog, it may still contain data at this point.
		conn->priv->headPos=0;
		conn->post->len=-1;
		conn->priv->flags=0;
		if (conn->post->buff) {
			free(conn->post->buff);
			httpd_printf("FREE: conn->post->buff\n");
		}
		conn->post->buff=NULL;
		conn->post->buffLen=0;
		conn->post->received=0;
		conn->hostName=NULL;
		conn->reserved=0;
	} else {
		//Cannot re-use this connection. Mark to get it killed after all data is sent.
		conn->priv->flags|=HFL_DISCONAFTERSENT;
	}
	httpd_printf("EXIT: httpdCgiIsDone\n");
}

//Callback called when the data on a socket has been successfully
//sent.
void ICACHE_FLASH_ATTR httpdSentCb(ConnTypePtr rconn, char *remIp, int remPort) {
	httpd_printf("ENTRY: httpdSentCb\n");
	HttpdConnData *conn=httpdFindConnData(rconn, remIp, remPort);
	httpdContinue(conn);
	httpd_printf("EXIT: httpdSentCb\n");
}

//Can be called after a CGI function has returned HTTPD_CGI_MORE to
//resume handling an open connection asynchronously
void ICACHE_FLASH_ATTR httpdContinue(HttpdConnData * conn) {
	httpd_printf("ENTRY: httpdContinue\n");
	int r;
	httpdPlatLock();

	char *sendBuff;

	if (conn==NULL) {
  	httpd_printf("EXIT: httpdContinue\n");
		return;
	}

	if (conn->priv->sendBacklog!=NULL) {
		//We have some backlog to send first.
		HttpSendBacklogItem *next=conn->priv->sendBacklog->next;
		httpdPlatSendData(conn->conn, conn->priv->sendBacklog->data, conn->priv->sendBacklog->len);
		conn->priv->sendBacklogSize-=conn->priv->sendBacklog->len;
		free(conn->priv->sendBacklog);
		httpd_printf("FREE: conn->priv->sendBacklog\n");
		conn->priv->sendBacklog=next;
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdContinue\n");
		return;
	}

	if (conn->priv->flags&HFL_DISCONAFTERSENT) { //Marked for destruction?
		httpd_printf("Pool slot %d is done. Closing.\n", conn->slot);
		httpdPlatDisconnect(conn->conn);
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdContinue\n");
		return; //No need to call httpdFlushSendBuffer.
	}

	//If we don't have a CGI function, there's nothing to do but wait for something from the client.
	if (conn->cgi==NULL) {
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdContinue\n");
		return;
	}

	sendBuff=malloc(HTTPD_MAX_SENDBUFF_LEN);
	if (sendBuff==NULL) {
		httpd_printf("Malloc of sendBuff failed!\n");
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdContinue\n");
		return;
	}
	httpd_printf("MALLOC: sendBuff %d\n", HTTPD_MAX_SENDBUFF_LEN);
	conn->priv->sendBuff=sendBuff;
	conn->priv->sendBuffLen=0;
	r=conn->cgi(conn); //Execute cgi fn.
	if (r==HTTPD_CGI_DONE) {
		httpdCgiIsDone(conn);
	}
	if (r==HTTPD_CGI_NOTFOUND || r==HTTPD_CGI_AUTHENTICATED) {
		httpd_printf("ERROR! CGI fn returns code %d after sending data! Bad CGI!\n", r);
		httpdCgiIsDone(conn);
	}
	httpdFlushSendBuffer(conn);
	free(sendBuff);
	httpd_printf("FREE: sendBuff\n");
	httpdPlatUnlock();
	httpd_printf("EXIT: httpdContinue\n");
}

//This is called when the headers have been received and the connection is ready to send
//the result headers and data.
//We need to find the CGI function to call, call it, and dependent on what it returns either
//find the next cgi function, wait till the cgi data is sent or close up the connection.
static void ICACHE_FLASH_ATTR httpdProcessRequest(HttpdConnData *conn) {
	httpd_printf("ENTRY: httpdProcessRequest\n");
	int r;
	int i=0;
	ets_printf("httpdProcessRequest stack %p\r\n", &r);
	if (conn->url==NULL) {
		httpd_printf("WtF? url = NULL\n");
		httpd_printf("EXIT: httpdProcessRequest\n");
		return; //Shouldn't happen
	}
	ets_printf("httpdProcesRequest for URL: %s\r\n", conn->url);

	//See if we can find a CGI that's happy to handle the request.
	while (1) {
		//Look up URL in the built-in URL table.
		while (builtInUrls[i].url!=NULL) {
			int match=0;
			//See if there's a literal match
			if (strcmp(builtInUrls[i].url, conn->url)==0) match=1;
			//See if there's a wildcard match
			if (builtInUrls[i].url[strlen(builtInUrls[i].url)-1]=='*' &&
					strncmp(builtInUrls[i].url, conn->url, strlen(builtInUrls[i].url)-1)==0) match=1;
			if (match) {
				httpd_printf("Is url index %d\n", i);
				conn->cgiData=NULL;
				conn->cgi=builtInUrls[i].cgiCb;
				conn->cgiArg=builtInUrls[i].cgiArg;
				break;
			}
			i++;
		}
		if (builtInUrls[i].url==NULL) {
			//Drat, we're at the end of the URL table. This usually shouldn't happen. Well, just
			//generate a built-in 404 to handle this.
			httpd_printf("%s not found. 404!\n", conn->url);
			conn->cgi=cgiNotFound;
		}
		
		//Okay, we have a CGI function that matches the URL. See if it wants to handle the
		//particular URL we're supposed to handle.
		r=conn->cgi(conn);
		if (r==HTTPD_CGI_MORE) {
			//Yep, it's happy to do so and has more data to send.
			if (conn->recvHdl) {
				//Seems the CGI is planning to do some long-term communications with the socket.
				//Disable the timeout on it, so we won't run into that.
				httpdPlatDisableTimeout(conn->conn);
			}
			httpdFlushSendBuffer(conn);
			httpd_printf("EXIT: httpdProcessRequest\n");
			return;
		} else if (r==HTTPD_CGI_DONE) {
			//Yep, it's happy to do so and already is done sending data.
			httpdCgiIsDone(conn);
			httpd_printf("EXIT: httpdProcessRequest\n");
			return;
		} else if (r==HTTPD_CGI_NOTFOUND || r==HTTPD_CGI_AUTHENTICATED) {
			//URL doesn't want to handle the request: either the data isn't found or there's no
			//need to generate a login screen.
			i++; //look at next url the next iteration of the loop.
		}
	}
	httpd_printf("EXIT: httpdProcessRequest\n");
}

//Parse a line of header data and modify the connection data accordingly.
static void ICACHE_FLASH_ATTR httpdParseHeader(char *h, HttpdConnData *conn) {
	httpd_printf("ENTRY: httpdParseHeader\n");
	int i;
	char firstLine=0;
	httpd_printf("httpdParseHeader %s\n", h);
	
	if (strncmp(h, "GET ", 4)==0) {
		conn->requestType = HTTPD_METHOD_GET;
		firstLine=1;
	} else if (strncmp(h, "HEAD ", 4)==0) {
		conn->requestType = HTTPD_METHOD_HEAD;
		firstLine=1;
	} else if (strncmp(h, "Host:", 5)==0) {
		i=5;
		while (h[i]==' ') i++;
		conn->hostName=&h[i];
	} else if (strncmp(h, "POST ", 5)==0) {
		conn->requestType = HTTPD_METHOD_POST;
		firstLine=1;
	}

	if (firstLine) {
		char *e;
		
		//Skip past the space after POST/GET/HEAD
		i=0;
		while (h[i]!=' ') i++;
		conn->url=h+i+1;

		//Figure out end of url.
		e=(char*)strstr(conn->url, " ");
		if (e==NULL) {
			httpd_printf("EXIT: httpdParseHeader\n");
			return; //wtf?
		}
		*e=0; //terminate url part
		e++; //Skip to protocol indicator
		while (*e==' ') e++; //Skip spaces.
		//If HTTP/1.1, note that and set chunked encoding
		//if (strcasecmp(e, "HTTP/1.1")==0) conn->priv->flags|=HFL_HTTP11|HFL_CHUNKED;  // XXX PDF Was
		if (strcasecmp(e, "HTTP/1.1")==0) conn->priv->flags|=HFL_HTTP11;  // XXX PDF change

		httpd_printf("Conn ptr = %p\n", conn);
		httpd_printf("URL ptr = %x\n", conn->url);
		httpd_printf("URL len = %d\n", strlen(conn->url));
		httpd_printf("URL = %s\n", conn->url);
		//Parse out the URL part before the GET parameters.
		conn->getArgs=(char*)strstr(conn->url, "?");
		if (conn->getArgs!=0) {
			*conn->getArgs=0;
			conn->getArgs++;
			httpd_printf("GET args = %s\n", conn->getArgs);
		} else {
			conn->getArgs=NULL;
		}
	} else if (strncmp(h, "Connection:", 11)==0) {
		i=11;
		//Skip trailing spaces
		while (h[i]==' ') i++;
		if (strncmp(&h[i], "close", 5)==0) conn->priv->flags&=~HFL_CHUNKED; //Don't use chunked conn
	} else if (strncmp(h, "Content-Length:", 15)==0) {
		i=15;
		//Skip trailing spaces
		while (h[i]==' ') i++;
		//Get POST data length
		conn->post->len=atoi(h+i);

		// Allocate the buffer
		if (conn->post->len > HTTPD_MAX_POST_LEN) {
			// we'll stream this in in chunks
			conn->post->buffSize = HTTPD_MAX_POST_LEN;
		} else {
			conn->post->buffSize = conn->post->len;
		}
		conn->post->buff=(char*)malloc(conn->post->buffSize + 1);
		if (conn->post->buff==NULL) {
			printf("...failed!\n");
			httpd_printf("EXIT: httpdParseHeader\n");
			return;
		}
		httpd_printf("MALLOC: post->buff %d\n", conn->post->buffSize + 1);
		conn->post->buffLen=0;
	} else if (strncmp(h, "Content-Type: ", 14)==0) {
		if (strstr(h, "multipart/form-data")) {
			// It's multipart form data so let's pull out the boundary for future use
			char *b;
			if ((b = strstr(h, "boundary=")) != NULL) {
				conn->post->multipartBoundary = b + 7; // move the pointer 2 chars before boundary then fill them with dashes
				conn->post->multipartBoundary[0] = '-';
				conn->post->multipartBoundary[1] = '-';
				httpd_printf("boundary = %s\n", conn->post->multipartBoundary);
			}
		}
	}
	httpd_printf("EXIT: httpdParseHeader\n");
}

//Make a connection 'live' so we can do all the things a cgi can do to it.
//ToDo: Also make httpdRecvCb/httpdContinue use these?
//ToDo: Fail if malloc fails?
void ICACHE_FLASH_ATTR httpdConnSendStart(HttpdConnData *conn) {
	httpd_printf("ENTRY: httpdConnSendStart\n");
	httpdPlatLock();
	char *sendBuff=malloc(HTTPD_MAX_SENDBUFF_LEN);
	if (sendBuff==NULL) {
		printf("Malloc sendBuff failed!\n");
		httpd_printf("EXIT: httpdConnSendStart\n");
		return;
	}
	httpd_printf("MALLOC: sendBuff %d\n", HTTPD_MAX_SENDBUFF_LEN);
	conn->priv->sendBuff=sendBuff;
	conn->priv->sendBuffLen=0;
	httpd_printf("EXIT: httpdConnSendStart\n");
}

//Finish the live-ness of a connection. Always call this after httpdConnStart
void ICACHE_FLASH_ATTR httpdConnSendFinish(HttpdConnData *conn) {
	httpd_printf("ENTRY: httpdConnSendFinish\n");
	if (conn->conn) httpdFlushSendBuffer(conn);
	free(conn->priv->sendBuff);
	httpd_printf("FREE: conn->priv->sendBuff\n");
	httpdPlatUnlock();
	httpd_printf("EXIT: httpdConnSendFinish\n");
}

//Callback called when there's data available on a socket.
void ICACHE_FLASH_ATTR httpdRecvCb(ConnTypePtr rconn, char *remIp, int remPort, char *data, unsigned short len) {
	httpd_printf("ENTRY: httpdRecvCb\n");
	int x, r;
	char *p, *e;

	ets_printf("  stack %p\r\n", &x);
	
	httpdPlatLock();

	HttpdConnData *conn=httpdFindConnData(rconn, remIp, remPort);
	if (conn==NULL) {
		ets_printf("couldn't find connection\r\n");
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdRecvCb\n");
		return;
	}
	ets_printf("  conn %p\r\n", conn);
	ets_printf("  conn->priv %p\r\n", conn->priv);

	char *sendBuff=malloc(HTTPD_MAX_SENDBUFF_LEN);
	if (sendBuff==NULL) {
		printf("Malloc sendBuff failed!\n");
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdRecvCb\n");
		return;
	}
	httpd_printf("MALLOC: sendBuff %d\n", HTTPD_MAX_SENDBUFF_LEN);
	conn->priv->sendBuff=sendBuff;
	conn->priv->sendBuffLen=0;
	ets_printf("  conn->priv->SendBuff %p\r\n", conn->priv->sendBuff);

	//This is slightly evil/dirty: we abuse conn->post->len as a state variable for where in the http communications we are:
	//<0 (-1): Post len unknown because we're still receiving headers
	//==0: No post data
	//>0: Need to receive post data
	//ToDo: See if we can use something more elegant for this.

	for (x=0; x<len; x++) {
		if (conn->post->len<0) {
			//This byte is a header byte.
			if (data[x]=='\n') {
				//Compatibility with clients that send \n only: fake a \r in front of this.
				if (conn->priv->headPos!=0 && conn->priv->head[conn->priv->headPos-1]!='\r') {
					conn->priv->head[conn->priv->headPos++]='\r';
				}
			}
			//ToDo: return http error code 431 (request header too long) if this happens
			if (conn->priv->headPos<HTTPD_MAX_HEAD_LEN-1) conn->priv->head[conn->priv->headPos++]=data[x];
			conn->priv->head[conn->priv->headPos]=0;
			//Scan for /r/n/r/n. Receiving this indicate the headers end.
			if (data[x]=='\n' && (char *)strstr(conn->priv->head, "\r\n\r\n")!=NULL) {
				//Indicate we're done with the headers.
				conn->post->len=0;
				//Reset url data
				conn->url=NULL;
				//Iterate over all received headers and parse them.
				p=conn->priv->head;
				while(p<(&conn->priv->head[conn->priv->headPos-4])) {
					e=(char *)strstr(p, "\r\n"); //Find end of header line
					if (e==NULL) break;			//Shouldn't happen.
					e[0]=0;						//Zero-terminate header
					httpdParseHeader(p, conn);	//and parse it.
					p=e+2;						//Skip /r/n (now /0/n)
				}
				//If we don't need to receive post data, we can send the response now.
				if (conn->post->len==0) {
					httpdProcessRequest(conn);
				}
			}
		} else if (conn->post->len!=0) {
			//This byte is a POST byte.
			conn->post->buff[conn->post->buffLen++]=data[x];
			conn->post->received++;
			conn->hostName=NULL;
			if (conn->post->buffLen >= conn->post->buffSize || conn->post->received == conn->post->len) {
				//Received a chunk of post data
				conn->post->buff[conn->post->buffLen]=0; //zero-terminate, in case the cgi handler knows it can use strings
				//Process the data
				if (conn->cgi) {
					r=conn->cgi(conn);
					if (r==HTTPD_CGI_DONE) {
						httpdCgiIsDone(conn);
					}
				} else {
					//No CGI fn set yet: probably first call. Allow httpdProcessRequest to choose CGI and
					//call it the first time.
					httpdProcessRequest(conn);
				}
				conn->post->buffLen = 0;
			}
		} else {
			//Let cgi handle data if it registered a recvHdl callback. If not, ignore.
			if (conn->recvHdl) {
				r=conn->recvHdl(conn, data+x, len-x);
				if (r==HTTPD_CGI_DONE) {
					httpd_printf("Recvhdl returned DONE\n");
					httpdCgiIsDone(conn);
					//We assume the recvhdlr has sent something; we'll kill the sock in the sent callback.
				}
				break; //ignore rest of data, recvhdl has parsed it.
			} else {
				httpd_printf("Eh? Got unexpected data from client. %s\n", data);
			}
		}
	}
	if (conn->conn) httpdFlushSendBuffer(conn);
	free(sendBuff);
	httpd_printf("FREE: sendBuff\n");
	conn->priv->sendBuff = NULL; // XXX PDF New
	httpdPlatUnlock();
	httpd_printf("EXIT: httpdRecvCb\n");
}

//The platform layer should ALWAYS call this function, regardless if the connection is closed by the server
//or by the client.
void ICACHE_FLASH_ATTR httpdDisconCb(ConnTypePtr rconn, char *remIp, int remPort) {
	httpd_printf("ENTRY: httpdDisconCb\n");
	httpdPlatLock();
	HttpdConnData *hconn=httpdFindConnData(rconn, remIp, remPort);
	if (hconn==NULL) {
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdDisconCb\n");
		return;
	}
	httpd_printf("Pool slot %d: socket closed.\n", hconn->slot);
	hconn->conn=NULL; //indicate cgi the connection is gone
	if (hconn->cgi) hconn->cgi(hconn); //Execute cgi fn if needed
	httpdRetireConn(hconn);
	httpdPlatUnlock();
	httpd_printf("EXIT: httpdDisconCb\n");
}


int ICACHE_FLASH_ATTR httpdConnectCb(ConnTypePtr conn, char *remIp, int remPort) {
	httpd_printf("ENTRY: httpdConnectCb\n");
	int i;
	httpdPlatLock();
	//Find empty conndata in pool
	for (i=0; i<HTTPD_MAX_CONNECTIONS; i++) if (connData[i]==NULL) break;
	httpd_printf("Conn req from  %d.%d.%d.%d:%d, using pool slot %d\n", remIp[0]&0xff, remIp[1]&0xff, remIp[2]&0xff, remIp[3]&0xff, remPort, i);
	if (i==HTTPD_MAX_CONNECTIONS) {
		httpd_printf("Aiee, conn pool overflow!\n");
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdConnectCb\n");
		return 0;
	}
	connData[i]=malloc(sizeof(HttpdConnData));
	if (connData[i]==NULL) {
		printf("Out of memory allocating connData!\n");
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdConnectCb\n");
		return 0;
	}
	httpd_printf("MALLOC: connData %d\n", sizeof(HttpdConnData));
	memset(connData[i], 0, sizeof(HttpdConnData));
	connData[i]->priv=malloc(sizeof(HttpdPriv));
	if (connData[i]->priv == NULL)
	{
		printf("Out of memory allocating connData priv struct!\n");
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdConnectCb\n");
		return 0;
	}
	httpd_printf("MALLOC: priv %d\n", sizeof(HttpdPriv));
	memset(connData[i]->priv, 0, sizeof(HttpdPriv));
	connData[i]->conn=conn;
	connData[i]->slot=i;
	connData[i]->priv->headPos=0;
	connData[i]->post=malloc(sizeof(HttpdPostData));
	if (connData[i]->post==NULL) {
		printf("Out of memory allocating connData post struct!\n");
		httpdPlatUnlock();
		httpd_printf("EXIT: httpdConnectCb\n");
		return 0;
	}
	httpd_printf("MALLOC: post %d\n", sizeof(HttpdPostData));
	memset(connData[i]->post, 0, sizeof(HttpdPostData));
	connData[i]->post->buff=NULL;
	connData[i]->post->buffLen=0;
	connData[i]->post->received=0;
	connData[i]->post->len=-1;
	connData[i]->hostName=NULL;
	connData[i]->remote_port=remPort;
	connData[i]->priv->sendBacklog=NULL;
	connData[i]->priv->sendBacklogSize=0;
	memcpy(connData[i]->remote_ip, remIp, 4);
	connData[i]->reserved = FALSE;

	httpdPlatUnlock();
	httpd_printf("EXIT: httpdConnectCb\n");
	return 1;
}

//Httpd initialization routine. Call this to kick off webserver functionality.
void ICACHE_FLASH_ATTR httpdInit(HttpdBuiltInUrl *fixedUrls, int port) {
	httpd_printf("ENTRY: httpdInit\n");
	int i;

	for (i=0; i<HTTPD_MAX_CONNECTIONS; i++) {
		connData[i]=NULL;
	}
	builtInUrls=fixedUrls;

	httpdPlatInit(port, HTTPD_MAX_CONNECTIONS);
	httpd_printf("EXIT: httpdInit\n");
}
