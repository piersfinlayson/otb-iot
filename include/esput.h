#ifndef ESPUT_H
#define ESPUT_H

#define ICACHE_FLASH_ATTR

#include "ctype.h"
#include "stdint.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef unsigned char bool;
typedef unsigned char BOOL;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed char sint8;
typedef signed short sint16;
typedef signed int sint32;

struct ip_addr {
    uint32 addr;
};

typedef struct ip_addr ip_addr_t;

typedef void ETSTimerFunc(void *timer_arg);

typedef struct _ETSTIMER_ {
    struct _ETSTIMER_    *timer_next;
    uint32_t              timer_expire;
    uint32_t              timer_period;
    ETSTimerFunc         *timer_func;
    void                 *timer_arg;
} ETSTimer;

/* probably should not put STATUS here */
typedef enum {
    OK = 0,
    FAIL,
    PENDING,
    BUSY,
    CANCEL,
} STATUS;

typedef struct {
        uint8 ssid[32];
        uint8 ssid_len;
        uint8 bssid[6];
        uint8 channel;
} Event_StaMode_Connected_t;

typedef struct {
        uint8 ssid[32];
        uint8 ssid_len;
        uint8 bssid[6];
        uint8 reason;
} Event_StaMode_Disconnected_t;

typedef struct {
        uint8 old_mode;
        uint8 new_mode;
} Event_StaMode_AuthMode_Change_t;

typedef struct {
        struct ip_addr ip;
        struct ip_addr mask;
        struct ip_addr gw;
} Event_StaMode_Got_IP_t;

typedef struct {
        uint8 mac[6];
        uint8 aid;
} Event_SoftAPMode_StaConnected_t;

typedef struct {
        uint8 mac[6];
        uint8 aid;
} Event_SoftAPMode_StaDisconnected_t;

typedef struct {
        int rssi;
        uint8 mac[6];
} Event_SoftAPMode_ProbeReqRecved_t;
typedef union {
        Event_StaMode_Connected_t                       connected;
        Event_StaMode_Disconnected_t            disconnected;
        Event_StaMode_AuthMode_Change_t         auth_change;
        Event_StaMode_Got_IP_t                          got_ip;
        Event_SoftAPMode_StaConnected_t         sta_connected;
        Event_SoftAPMode_StaDisconnected_t      sta_disconnected;
        Event_SoftAPMode_ProbeReqRecved_t   ap_probereqrecved;
} Event_Info_u;

typedef struct _esp_event {
    uint32 event;
    Event_Info_u event_info;
} System_Event_t;

#define FALSE 0
#define TRUE  1 

#define os_strstr strstr
#define os_strlen strlen
#define os_strnlen strnlen
#define os_strcmp strcmp
#define os_strncmp strncmp
#define os_memset memset
#define os_timer_t  ETSTimer
#define os_timer_func_t ETSTimerFunc

extern size_t strnlen(const char *s, size_t maxlen);
int esput_printf(const char *string, const char *format, ...);
extern void otb_util_assert(bool value, char *value_s);

extern bool esput_assertion_failed;
extern bool esput_debug;

#define ESPUT_TEST_START(X, Y)                                      \
void X(void);                                                       \
void X(void)                                                        \
{                                                                   \
  esput_assertion_failed = FALSE;                                   \
  bool passed = FALSE;                                              \
  esput_printf("", "UT: %s - %s", #X, Y);

#define ESPUT_TEST_FINISH                                           \
ESPUT_TEST_EXIT:                                                    \
  esput_printf("", "%s\n", (passed ? "PASSED" : "FAILED"));         \
}

#define ESPUT_TEST_FOR_ASSERTIONS()                                 \
if (esput_assertion_failed)  goto ESPUT_TEST_EXIT



#endif