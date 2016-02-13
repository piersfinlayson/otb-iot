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

#define OTB_UTIL_LOG_BUFFER_LEN  512
 
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

extern void otb_util_convert_ws_to_(char *text);
extern void otb_util_log_useful_info(bool recovery);
extern void otb_util_init_logging(void);
extern void otb_util_log_fn(char *text);
extern void otb_util_log_store(void);
extern void otb_util_log_save(char *text);
extern void otb_util_log_fn(char *text);
extern void otb_util_assert(bool value, char *value_s);
extern void otb_error_reset(void);
extern void otb_reset(void);
extern void otb_util_clear_reset(void);
extern bool otb_util_timer_finished(otb_util_timeout *timeout);
extern void otb_util_delay_ms(uint32_t value);
extern void otb_util_check_defs(void);
extern void otb_util_log_snprintf(char *log_string,
                                  uint16_t max_log_string_len,
                                  char *format,
                                  va_list args);
extern void otb_util_log(bool error,
                         char *log_string,
                         uint16_t max_log_string_len,
                         char *format,
                         ...);
extern void otb_util_log_error_via_mqtt(char *);
extern size_t otb_util_strnlen(const char *s, size_t maxlen);
extern void otb_init_wifi(void *arg);
extern void otb_init_mqtt(void *arg);
extern void otb_init_ds18b20(void *arg);

