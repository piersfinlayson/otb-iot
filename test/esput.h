
#define OTB_DEBUG 1
#define ICACHE_FLASH_ATTR
#define ICACHE_RODATA_ATTR
#define ALIGN4

typedef unsigned char bool;
#define TRUE 1
#define true 1
#define FALSE 0
#define false 0
typedef unsigned char uint8;
typedef unsigned char uint8_t;
typedef char int8;
typedef char sint8;
typedef unsigned char uint8;
typedef unsigned char uint8_t;
typedef unsigned short uint16;
typedef unsigned short uint16_t;
typedef short sint16;
typedef unsigned long uint32;
typedef unsigned long uint32_t;

#define OTB_MIN(A, B) (A < B) ? A : B

extern char *mlog;
#define MLOG(X) char *mlog = X

extern bool esput_debug;

#define MDETAIL(X, ...)  printf("  %s: " X "\n", mlog, ##__VA_ARGS__)
#define MDEBUG(X, ...)   esput_debug ? printf("  %s: " X "\n", mlog, ##__VA_ARGS__) : 0
#define MINFO(X, ...)    printf("  %s: " X "\n", mlog, ##__VA_ARGS__)
#define MWARN(X, ...)    printf("  %s: " X "\n", mlog, ##__VA_ARGS__)
#define MERROR(X, ...)   printf("  %s: " X "\n", mlog, ##__VA_ARGS__)
#define DETAIL(X, ...)   printf("  %s: " X "\n", mlog, ##__VA_ARGS__)
#define DEBUG(X, ...)    esput_debug ? printf("  %s: " X "\n", mlog, ##__VA_ARGS__) : 0
#define INFO(X, ...)     printf("  %s: " X "\n", mlog, ##__VA_ARGS__)
#define WARN(X, ...)     printf("  %s: " X "\n", mlog, ##__VA_ARGS__)
#define ERROR(X, ...)    printf("  %s: " X "\n", mlog, ##__VA_ARGS__)
#define LOG(X, ...)      printf("ESPUT: " X "\n", ##__VA_ARGS__)
#define ENTRY
#define EXIT

#define os_memset memset
#define os_memcpy memcpy
#define os_memcmp memcmp
#define os_printf printf
#define os_malloc malloc
#define os_free free
#define os_strcpy strcpy
#define os_strncpy strncpy
#define os_strcmp strcmp
#define os_strncmp strncmp
#define os_strlen strlen
#define os_strnlen strnlen
#define os_strstr strstr
#define os_snprintf snprintf
#define os_sprintf sprintf

#define OTB_ASSERT assert

typedef void System_Event_t;
typedef unsigned char STATUS;
typedef void (* espconn_recv_callback)(void *arg, char *pdata, unsigned short len);
typedef void (* espconn_sent_callback)(void *arg);

enum espconn_type
{
  ESPCONN_INVALID = 0,
  ESPCONN_TCP     = 0x10,
  ESPCONN_UDP     = 0x20,
};

enum espconn_state {
  ESPCONN_NONE,
  ESPCONN_WAIT,
  ESPCONN_LISTEN,
  ESPCONN_CONNECT,
  ESPCONN_WRITE,
  ESPCONN_READ,
  ESPCONN_CLOSE
};

typedef void (* espconn_connect_callback)(void *arg);
typedef void (* espconn_reconnect_callback)(void *arg, sint8 err);

typedef struct esput_esp_tcp
{
  int remote_port;
  int local_port;
  uint8 local_ip[4];
  uint8 remote_ip[4];
  espconn_connect_callback connect_callback;
  espconn_reconnect_callback reconnect_callback;
  espconn_connect_callback disconnect_callback;
  espconn_connect_callback write_finish_fn;
} esp_tcp;

typedef struct esput_esp_udp
{
  int remote_port;
  int local_port;
  uint8 local_ip[4];
  uint8 remote_ip[4];
} esp_udp;

struct espconn
{
  enum espconn_type type;
  enum espconn_state state;
  union
  {
    esp_tcp *tcp;
    esp_udp *udp;
  } proto;
  espconn_recv_callback recv_callback;
  espconn_sent_callback sent_callback;
  uint8 link_cnt;
  void *reverse;
};

#define espconn_tcp_set_max_con_allow(...) esput_espconn_tcp_set_max_con_allow(__VA_ARGS__)
#define espconn_accept(...) esput_espconn_accept(__VA_ARGS__)
#define espconn_disconnect(...) esput_espconn_disconnect(__VA_ARGS__)
#define espconn_regist_connectcb(...) esput_espconn_regist_connectcb(__VA_ARGS__)
#define espconn_regist_recvcb(...) esput_espconn_regist_recvcb(__VA_ARGS__)
#define espconn_regist_reconcb(...) esput_espconn_regist_reconcb(__VA_ARGS__)
#define espconn_regist_disconcb(...) esput_espconn_regist_disconcb(__VA_ARGS__)
#define espconn_regist_sentcb(...) esput_espconn_regist_sentcb(__VA_ARGS__)
#define espconn_send(...) esput_espconn_send(__VA_ARGS__)
#define captdnsInit(...) esput_captdnsInit(__VA_ARGS__)
#define captdnsTerm(...) esput_captdnsTerm(__VA_ARGS__)

extern sint8 esput_espconn_tcp_set_max_con_allow(struct espconn *espconn, uint8 num);
extern sint8 esput_espconn_accept(struct espconn *espconn);
extern sint8 esput_espconn_disconnect(struct espconn *espconn);
extern sint8 esput_espconn_send(struct espconn *espconn, uint8 *psent, uint16 length);
extern sint8 esput_espconn_regist_connectcb(struct espconn *espconn, espconn_connect_callback connect_cb);
extern sint8 esput_espconn_regist_recvcb(struct espconn *espconn, espconn_recv_callback recv_cb);
extern sint8 esput_espconn_regist_reconcb(struct espconn *espconn, espconn_reconnect_callback recon_cb);
extern sint8 esput_espconn_regist_disconcb(struct espconn *espconn, espconn_connect_callback discon_cb);
extern sint8 esput_espconn_regist_sentcb(struct espconn *espconn, espconn_sent_callback sent_cb);
extern void esput_captdnsInit(void);
extern void esput_captdnsTerm(void);

typedef void MQTT_Client;

#define OTB_MQTT_MAX_SVR_LEN            32
#define OTB_MQTT_MAX_USER_LEN           32
#define OTB_MQTT_MAX_PASS_LEN           32
#define OTB_DS18B20_MAX_DS18B20S 8

#define OTB_IPV4_SNPRINTF(STR, IP)                                   \
        os_snprintf(STR, OTB_IP_MAX_IPV4_ADDR_LEN, "%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]); \
        STR[OTB_IP_MAX_IPV4_ADDR_LEN-1] = 0

#define ESPUT_ASSERT(X) if (!(X)) { printf("%s: %s failed\n", test_name, #X); return(FALSE); }

typedef bool esput_test_fn(char *test_name);

typedef struct esput_test
{
  esput_test_fn *fn;
  char *name;
  char *descr;
} esput_test;

extern esput_test esput_tests[];
