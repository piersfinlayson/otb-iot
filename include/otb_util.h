/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016 Piers Finlayson
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version. 
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef OTB_UTIL_H_INCLUDED
#define OTB_UTIL_H_INCLUDED

// This is a linker symbol, the address of which is actually the build number.
// As a result don't attempt to access the contents of this address!
extern char otb_build_num;
#define OTB_BUILD_NUM (unsigned long)&otb_build_num

#define OTB_UTIL_LOG_BUFFER_LEN 1024 // 1024
#define OTB_UTIL_NS_PER_CYCLE 12.5

void ICACHE_FLASH_ATTR otb_util_factory_reset(void);

typedef struct otb_util_log_buffer
{
  // Pointer to buffer (which is otb_util_log_buf)
  char *buffer;
  
  // Pointer to next bit of buffer to write o
  char *current;
  
  // Total length of buffer - so max len 64KB
  uint16_t len;

} otb_util_log_buffer;

typedef struct otb_util_timeout
{
  uint32_t start_time;
  uint32_t end_time;
} otb_util_timeout;

void otb_util_booted(void);
extern size_t otb_util_copy_flash_to_ram(char *dst, const char *from_ptr_byte, int size);
void otb_util_convert_char_to_char(char *text, int from, int to);
int isdigit(int c);
int isalpha(int c);
int isalnum(int c);
void otb_util_get_chip_id(void);
void otb_util_convert_colon_to_period(char *text);
extern void otb_util_log_useful_info(bool recovery);
extern void otb_util_init_logging(void);
void otb_util_resume_init(void);
extern void otb_util_log_fn(char *text);
extern void otb_util_log_store(void);
char *otb_util_get_log_ram(uint8 index);
extern void otb_util_log_save(char *text);
void otb_util_assert(bool value, char *value_s, char *file, uint32_t line);
void otb_reset_schedule(uint32_t timeout,
                        const char *reason,
                        bool error);
void otb_reset_timer(void *arg);
extern void otb_reset_error(char *text);
extern void otb_reset(char *text);
void otb_reset_internal(char *text, bool error);
bool otb_util_reset_store_reason(char *text, bool *same);
extern void otb_util_clear_reset(void);
bool otb_util_flash_read(uint32 location,
                         uint32 *dest,
                         uint32 len);
bool otb_util_flash_write_string(uint32 location,
                                 uint32 *source,
                                 uint32 len);
bool otb_util_flash_write(uint32 location, uint32 *source, uint32 len);
void otb_util_log_heap_size_start_timer(void);
void otb_util_log_heap_size_timer(void *arg);
void otb_util_get_heap_size(void);
bool otb_util_get_vdd33(uint16 *vdd33);
void otb_util_timer_cancel(os_timer_t *timer);
bool otb_util_timer_is_set(os_timer_t *timer);
void otb_util_timer_set(os_timer_t *timer,
                        os_timer_func_t *timerfunc,
                        void *arg,
                        uint32_t timeout,
                        bool repeat);
extern bool otb_util_timer_finished(otb_util_timeout *timeout);
extern void otb_util_delay_ms(uint32_t value);
extern void otb_util_check_defs(void);
extern void otb_util_log_snprintf(char *log_string,
                                  uint16_t max_log_string_len,
                                  const char *format,
                                  va_list args);
extern void otb_util_log(uint8_t error,
                         char *log_string,
                         uint16_t max_log_string_len,
                         const char *format,
                         ...);
void otb_util_disable_logging(void);
void otb_util_enable_logging(void);
extern void otb_util_log_error_via_mqtt(char *);
extern size_t otb_util_strnlen(const char *s, size_t maxlen);
bool otb_util_is_fqdn(unsigned char *dn);
bool otb_util_is_ip(unsigned char *ip);
extern void otb_init_mqtt(void *arg);
extern void otb_init_ds18b20(void *arg);
void otb_init_ads(void *arg);
bool otb_util_parse_ipv4_str(char *ip_in, uint8_t *ip_out);
bool otb_util_ip_is_all_val(uint8_t *ip, uint8_t val);
bool otb_util_ip_is_subnet_valid(uint8_t *subnet);
void otb_util_uart0_rx_en(void);
void otb_util_uart0_rx_dis(void);
void otb_util_check_for_log_level(void);
void otb_util_check_for_break(void);
void otb_util_break_disable_timer(void);
void otb_util_break_enable_timer(uint32_t period);
void otb_util_break_timerfunc(void *arg);
void otb_util_uart0_rx_intr_handler(void *para);

int toupper(int c);
int isxdigit(int c);
int iscntrl(int c);
char *os_strdup(const char *s);

static inline uint32_t otb_util_get_cycle_count(void) __attribute__((always_inline));
static inline uint32_t otb_util_get_cycle_count(void)
{
  uint32_t ccount;
  __asm__ volatile ("rsr %0,ccount"
                    :
                    "=a" (ccount));
  return ccount;
}

struct otb_reset_reason
{
  const char *reason;
  
  bool error;
} otb_reset_reason;

extern uint8_t otb_util_log_level;

#ifdef OTB_UTIL_C

bool otb_util_booted_g;
uint8_t otb_util_log_level;
uint8_t otb_util_log_level_stored;
os_timer_t otb_util_reset_timer;
unsigned char otb_util_factory_reset_reason[] = "Factory reset completed";

struct otb_reset_reason otb_reset_reason_struct;

os_timer_t otb_util_heap_timer;

char otb_compile_date[12];
char otb_compile_time[9];
char OTB_MAIN_CHIPID[OTB_MAIN_CHIPID_STR_LENGTH];
char OTB_MAIN_DEVICE_ID[OTB_MAIN_DEVICE_ID_STR_LENGTH];
char otb_hw_info[10];

char otb_log_s[OTB_MAIN_MAX_LOG_LENGTH];

char ALIGN4 otb_util_log_flash_buffer[OTB_UTIL_LOG_FLASH_BUFFER_LEN];
// Force log buffer to be 4 byte aligned
uint32 otb_util_log_buf_int32[OTB_UTIL_LOG_BUFFER_LEN/4];
char *otb_util_log_buf;
otb_util_log_buffer otb_util_log_buffer_struct;

bool otb_util_asserting;
bool otb_util_break_enabled;
bool otb_util_break_checking;
bool otb_util_log_level_checking;
int otb_util_uart_rx_bytes;
char otb_util_uart_rx_buf[16];

os_timer_t otb_util_break_timer;

extern UartDevice    UartDev;

#endif // OTB_UTIL_C

#endif // OTB_UTIL_H_INCLUDED
