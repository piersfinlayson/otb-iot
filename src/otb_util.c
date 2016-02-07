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

#include "otb.h"
#include <stdarg.h>
#include <limits.h>
#include <errno.h>

void ICACHE_FLASH_ATTR otb_util_assert(bool value, char *value_s)
{
  DEBUG("UTIL: otb_util_assert entry");

  if(!value)
  {
    ERROR("------------- ASSERTION FAILED -------------");
    ERROR(value_s);
    ERROR("Rebooting");
    ERROR("--------------------------------------------");
    otb_reset();
    otb_util_delay_ms(1000);
  }

  DEBUG("UTIL: otb_util_assert exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_util_clear_reset(void)
{
  DEBUG("UTIL: otb_util_clear_reset entry");

  // Set GPIO 16 (reset GPIO) to high 
  // XXX Only works for GPIO16
  WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                 (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1);      // mux configuration for XPD_DCDC to output rtc_gpio0

  WRITE_PERI_REG(RTC_GPIO_CONF,
                 (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);  //mux configuration for out enable

  WRITE_PERI_REG(RTC_GPIO_ENABLE,
                 (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);        //out enable
  WRITE_PERI_REG(RTC_GPIO_OUT,
                 (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(1));

  DEBUG("UTIL: otb_util_clear_reset exit");

  return;
}

bool ICACHE_FLASH_ATTR otb_util_timer_finished(otb_util_timeout *timeout)
{
  bool rc = FALSE;
  uint32_t current_time; 
  
  DEBUG("UTIL: otb_util_timer_finished entry");

  current_time = system_get_time();
  if (timeout->start_time < timeout->end_time)
  {
    if (current_time > timeout->end_time)
    {
      DEBUG("UTIL: Timer finished", timeout->end_time);
      rc = TRUE;
    }
  }
  else
  {
    if ((current_time < timeout->start_time) && (current_time > timeout->end_time))
    {
      DEBUG("UTIL: Timer finished", timeout->end_time);
      rc = TRUE;
    }
  }

  DEBUG("UTIL: otb_util_timer_finished exit");
  
  return rc;  
}
 
void ICACHE_FLASH_ATTR otb_util_delay_ms(uint32_t value)
{
  // Can't just use Ardunio delay, cos may not be in the right context.
  // os_delay_us takes a uint16_t so can only delay up to 65535 microseconds
  // the code below needs to be checked that it will work OK across wrapping (a uint32_t
  // containing microseconds will wrap every 71 minutes, so there's a decent chance of
  // this happening
  // XXX Should test context to see whether do delay (if in sketch) or call
  // ESP function otherwise
  uint32_t wait_length = value;
  uint32_t start_time;
  uint32_t current_time;
  uint32_t end_time;
  bool going_to_wrap = FALSE;
    
  DEBUG("UTIL: otb_util_delay_ms entry");
  
  DEBUG("UTIL: wait for %d ms", value);

  OTB_ASSERT(value <= OTB_UTIL_MAX_DELAY_MS);
  
  current_time = system_get_time();
  start_time = current_time;
  end_time = current_time + (value * 1000);
  if (end_time < current_time)
  {
    going_to_wrap = TRUE;
  }
  
  while (TRUE)
  {
    if (value <= OTB_ESP_MAX_DELAY_MS)
    {
      DEBUG("UTIL: Wait for %d ms", value); 
      os_delay_us(value*1000);
    }
    else
    {
      DEBUG("UTIL: Wait for %d ms", OTB_UTIL_DELAY_WAIT_MS);
      os_delay_us(OTB_UTIL_DELAY_WAIT_MS*1000);
    }
    system_soft_wdt_feed();
    
    current_time = system_get_time();
    
    if (!going_to_wrap)
    {
      if (current_time >= end_time)
      {
        DEBUG("UTIL: Done");
        break;
      }
      else
      {
        value = (end_time - current_time) / 1000;
      }
    }
    else
    {
      if ((current_time < start_time) && (current_time >= end_time))
      {
        DEBUG("UTIL: Done");
        break;
      }
      else if (current_time < end_time)
      {
        value = (end_time - current_time) / 1000;
      }
      else
      {
        value = (end_time + (0xffffffff - current_time)) / 1000;
      }
    }
  }
  
  DEBUG("UTIL: otb_util_delay_ms exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_util_check_defs(void)
{
  DEBUG("UTIL: otb_util_check_defs entry");
  
  OTB_ASSERT(OTB_UTIL_DELAY_WAIT_MS <= OTB_ESP_MAX_DELAY_MS);
  OTB_ASSERT(strlen(OTB_MAIN_OTB_IOT) <= 8);
  OTB_ASSERT(strlen(OTB_MQTT_ROOT) <= 8);
  OTB_ASSERT(strlen(OTB_MAIN_FW_VERSION) <= 8);
  
  DEBUG("UTIL: otb_util_check_defs exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_util_log_snprintf(char *log_string,
                                             uint16_t max_log_string_len,
                                             char *format,
                                             va_list args)
{
  // DEBUG("UTIL: otb_util_log_snprintf entry");

  // Need to call vsnprintf not snprintf, as passing va_list
  os_vsnprintf(log_string, max_log_string_len, format, args);

  return;

  // DEBUG("UTIL: otb_util_log_snprintf exit");
}

void ICACHE_FLASH_ATTR otb_util_log(bool error,
                                    char *log_string,
                                    uint16_t max_log_string_len,
                                    char *format,
                                    ...)
{
  va_list args;

  // DEBUG("UTIL: otb_util_log entry");
  
  // Bit of messing around to deal with var args, but basically snprintf log
  // into log buffer and then log it
  va_start(args, format);
  otb_util_log_snprintf(log_string, max_log_string_len, format, args);
  va_end(args);
  otb_main_log_fn(otb_log_s);

  // Log if MQTT if an error and we're connected
  if (error && (otb_mqtt_client.connState == MQTT_DATA))                       \
  {                                                                            \
    otb_util_log_error_via_mqtt(otb_log_s);                                    \
  }

  // DEBUG("UTIL: otb_util_log exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_util_log_error_via_mqtt(char *text)
{
  // DEBUG("UTIL: otb_util_log_error_via_mqtt entry");

  os_snprintf(otb_mqtt_topic_s,
              OTB_MQTT_MAX_TOPIC_LENGTH,
              "/%s/%s/%s/%s/%s/%s",
              OTB_MQTT_ROOT,
              OTB_MQTT_LOCATION_1,
              OTB_MQTT_LOCATION_2,
              OTB_MQTT_LOCATION_3,
              OTB_MAIN_CHIPID,
              OTB_MQTT_PUB_LOG);

  // QOS=1 so gets through at least once, retain=1 so last log retained
  MQTT_Publish(&otb_mqtt_client, otb_mqtt_topic_s, text, strlen(text),  1, 1);

  // DEBUG("UTIL: otb_util_log_error_via_mqtt exit");

  return;
}

char* strcat(char * dest, const char * src) {
    return strncat(dest, src, strlen(src));
}

char* strncat(char * dest, const char * src, size_t n) {
    size_t i;
    size_t offset = strlen(dest);
    for(i = 0; i < n && src[i]; i++) {
        dest[i + offset] = src[i];
    }
    dest[i + offset] = 0;
    return dest;
}

int islower(int c) {
    if(c >= 'a' && c <= 'z') {
        return 1;
    }
    return 0;
}

int isupper(int c) {
    if(c >= 'A' && c <= 'Z') {
        return 1;
    }
    return 0;
}

int isalpha(int c) {
    if(islower(c) || isupper(c)) {
        return 1;
    }
    return 0;
}

int isdigit(int c) {
    if(c >= '0' && c <= '9') {
        return 1;
    }
    return 0;
}

int isalnum(int c) {
    if(isalpha(c) || isdigit(c)) {
        return 1;
    }
    return 0;
}

int iscntrl(int c) {
    if(c <= 0x1F || c == 0x7F) {
        return 1;
    }
    return 0;
}

int isprint(int c) {
    if(!iscntrl(c)) {
        return 1;
    }
    return 0;
}

int isgraph(int c) {
    if(isprint(c) && c != ' ') {
        return 1;
    }
    return 0;
}

int ispunct(int c) {
    if(isgraph(c) && !isalnum(c)) {
        return 1;
    }
    return 0;
}

int isxdigit(int c) {
    if(c >= 'A' && c <= 'F') {
        return 1;
    }
    if(c >= 'a' && c <= 'f') {
        return 1;
    }
    if(isdigit(c)) {
        return 1;
    }
    return 0;
}

int tolower(int c) {
    if(isupper(c)) {
        c += 0x20;
    }
    return c;
}

int toupper(int c) {
    if(islower(c)) {
        c -= 0x20;
    }
    return c;
}

int isblank(int c) {
    switch(c) {
        case 0x20: // ' '
        case 0x09: // '\t'
            return 1;
    }
    return 0;
}


int isspace(int c) {
    switch(c) {
        case 0x20: // ' '
        case 0x09: // '\t'
        case 0x0a: // '\n'
        case 0x0b: // '\v'
        case 0x0c: // '\f'
        case 0x0d: // '\r'
            return 1;
    }
    return 0;
}

/**
 * strtol() and strtoul() implementations borrowed from newlib:
 * http://www.sourceware.org/newlib/
 *      newlib/libc/stdlib/strtol.c
 *      newlib/libc/stdlib/strtoul.c
 *
 * Adapted for ESP8266 by Kiril Zyapkov <kiril.zyapkov@gmail.com>
 *
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the University of
 *    California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

long strtol(const char *nptr, char **endptr, int base) {
    const unsigned char *s = (const unsigned char *)nptr;
    unsigned long acc;
    int c;
    unsigned long cutoff;
    int neg = 0, any, cutlim;

    /*
     * Skip white space and pick up leading +/- sign if any.
     * If base is 0, allow 0x for hex and 0 for octal, else
     * assume decimal; if base is already 16, allow 0x.
     */
    do {
        c = *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+')
        c = *s++;
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;

    /*
     * Compute the cutoff value between legal numbers and illegal
     * numbers.  That is the largest legal value, divided by the
     * base.  An input number that is greater than this value, if
     * followed by a legal input character, is too big.  One that
     * is equal to this value may be valid or not; the limit
     * between valid and invalid numbers is then based on the last
     * digit.  For instance, if the range for longs is
     * [-2147483648..2147483647] and the input base is 10,
     * cutoff will be set to 214748364 and cutlim to either
     * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
     * a value > 214748364, or equal but the next digit is > 7 (or 8),
     * the number is too big, and we will return a range error.
     *
     * Set any if any `digits' consumed; make it negative to indicate
     * overflow.
     */
    cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
    cutlim = cutoff % (unsigned long)base;
    cutoff /= (unsigned long)base;
    for (acc = 0, any = 0;; c = *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
               if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = neg ? LONG_MIN : LONG_MAX;
        errno = ERANGE;
    } else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? (char *)s - 1 : nptr);
    return (acc);
}

long atol(const char* s) {
    char * tmp;
    return strtol(s, &tmp, 10);
}

int atoi(const char* s) {
    return (int) atol(s);
}

static int errno_var = 0;

int* __errno(void) {
    // DEBUGV("__errno is called last error: %d (not current)\n", errno_var);
    return &errno_var;
}

