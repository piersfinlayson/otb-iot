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
    delay(1000);
  }

  DEBUG("UTIL: otb_util_assert exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_util_delay_ms(uint32_t value)
{
  DEBUG("UTIL: otb_util_delay_ms entry");
  
#ifdef OTB_ARDUINO
  delay(value);
#else
  // os_delay_us takes a uint16_t so can only delay up to 65535 microseconds
  // the code below needs to be checked that it will work OK across wrapping (a uint32_t
  // containing microseconds will wrap every 71 minutes, so there's a decent chance of
  // this happening
  uint32_t wait_length = value;
  uint32_t last_time;
  uint32_t current_time;
  uint32_t end_time;
  uint32_t working;
  bool going_to_wrap = FALSE;
    
  DEBUG("UTIL: wait for %d ms" value);

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
      ]
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
#endif  
  
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

void ICACHE_FLASH_ATTR otb_util_log(char *log_string,
                                    uint16_t max_log_string_len,
                                    char *format,
                                    ...)
{
  va_list args;
  
  va_start(args, format);
  snprintf(log_string, max_log_string_len, format, args);
  va_end(args);
  otb_main_log_fn(otb_log_s);
  
  return;
}