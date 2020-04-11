// Based on mziwisky espmissingincludes.h && ESP8266_IoT_SDK_Programming Guide_v0.9.1.pdf && ESP SDK defines

#ifndef __ESP_SYSTEM_API_H__
#define __ESP_SYSTEM_API_H__

#include <rom/ets_sys.h>
#include <sdkconfig.h>

#include <stdarg.h>

#include <user_config.h>

#define __ESP8266_EX__ // System definition ESP8266 SOC

#define __forceinline __attribute__((always_inline)) inline
#define STORE_TYPEDEF_ATTR __attribute__((aligned(4),packed))
#define STORE_ATTR __attribute__((aligned(4)))

#undef assert
#define debugf(fmt, ...) m_printf(fmt"\r\n", ##__VA_ARGS__)
#define assert(condition) if (!(condition)) SYSTEM_ERROR("ASSERT: %s %d", __FUNCTION__, __LINE__)
#define SYSTEM_ERROR(fmt, ...) m_printf("ERROR: " fmt "\r\n", ##__VA_ARGS__)

extern void ets_timer_arm_new(os_timer_t *ptimer, uint32_t time, bool repeat_flag, bool ms_flag);
//extern void ets_timer_disarm(ETSTimer *a);
//extern void ets_timer_setfn(ETSTimer *t, ETSTimerFunc *pfunction, void *parg);

extern void ets_wdt_enable(void);
extern void ets_wdt_disable(void);
extern void wdt_feed(void);

extern void ets_isr_mask(unsigned intr);
extern void ets_isr_unmask(unsigned intr);
typedef void (* ets_isr_t)(void *);
extern void ets_isr_attach(int i, ets_isr_t func, void *arg);

extern int ets_memcmp(const void *s1, const void *s2, size_t n);
extern void *ets_memcpy(void *dest, const void *src, size_t n);
extern void *ets_memset(void *s, int c, size_t n);

extern void ets_install_putc1(void (*p)(char c));
extern int ets_sprintf(char *str, const char *format, ...)  __attribute__ ((format (printf, 2, 3)));
extern int ets_str2macaddr(void *, void *);
extern int ets_strcmp(const char *s1, const char *s2);
extern char *ets_strcpy(char *dest, const char *src);
extern const char * ets_strrchr(const char *str, int character);
extern int ets_strlen(const char *s);
extern size_t ets_strnlen(const char *s, size_t maxlen);
#define os_strnlen(A, B) otb_util_strnlen(A, B)
extern int ets_strncmp(const char *s1, const char *s2, unsigned int n);
extern char *ets_strncpy(char *dest, const char *src, size_t n);
extern char *ets_strstr(const char *haystack, const char *needle);
extern int os_printf_plus(const char *format, ...)  __attribute__ ((format (printf, 1, 2)));
//extern int os_snprintf(char *str, size_t size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
extern int ets_snprintf(char *str, size_t size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
extern int ets_vsnprintf(char * s, size_t n, const char * format, va_list arg) __attribute__ ((format (printf, 3, 0)));

//extern void *pvPortZalloc(size_t xWantedSize, const char *file, uint32_t line);
//extern void pvPortFree(void *ptr);
//extern void vPortFree(void *ptr, const char *file, uint32_t line);
//extern void *vPortMalloc(size_t xWantedSize);

extern void uart_div_modify(uint8_t uart_no, uint32_t DivLatchValue);
extern int ets_uart_printf(const char *fmt, ...);
extern int ets_printf(const char *fmt, ...);
extern void uart_tx_one_char(char ch);

extern void ets_intr_lock();
extern void ets_intr_unlock();
extern int tolower(int c);
extern int isspace(int c);
extern int strcasecmp(const char *a, const char *b);

// CPU Frequency
extern void ets_update_cpu_frequency(uint32_t frq);
extern uint32_t ets_get_cpu_frequency();

typedef signed short file_t;

#endif
