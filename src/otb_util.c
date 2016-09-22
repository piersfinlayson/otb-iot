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

#define OTB_UTIL_C
#include "otb.h"
#include <stdarg.h>
#include <limits.h>
#include <errno.h>

#if 0

void ICACHE_FLASH_ATTR otb_util_factory_reset(void)
{
  rc = FALSE;

  DEBUG("UTIL: otb_util_factory_reset entry");
  
  // First of all reset config
  otb_led_wifi_blink();  // Will turn off
  otb_conf_init_config(otb_conf); 
  rc = otb_conf_save(otb_conf);
  if (!rc)
  {
    // But plow on regardless
    WARN("UTIL: Failed to reset config to factory");
  }
  otb_led_wifi_blink();  // Will turn back on
  
  // Now update boot slot
  rc = otb_rboot_update_slot(0);  // Updates slot to 0
  if (!rc)
  {
    // But plow on regardless
    WARN("UTIL: Failed to reset boot slot to 0");
  }

  // Now actually do the firmware update
  
  // check factory partition is valid
  if (!ota_rboot_check_factory_image())
  {
    // Can't recover from this
    ERROR("UTIL: No good factory image");
    goto EXIT_LABEL;
  }
  
  if (ota_rboot_use_factory_image())
  {
    rc = TRUE;
  }
  else
  {
    // Can't recover from this
    ERROR("UTIL: Failed to update boot image with factory");
    goto EXIT_LABEL;
  }
  
  // Get length of recovery partition
  // Copy a sector at a time from recovery parition to partition 0
  // Blinking between each sector

  // Cyan for good, blue for bad
  if (rc)
  {
    otb_led_wifi_update(OTB_LED_NEO_COLOUR_CYAN, TRUE);
  }
  else
  {
    otb_led_wifi_update(OTB_LED_NEO_COLOUR_BLUE, TRUE);
  }
  
  // Blink for 5s before reset to indicate completion
  otb_led_wifi_init_blink_timer();
  otb_reset_schedule(otb_util_factory_reset_reason, 5000, FALSE);
  
  DEBUG("UTIL: otb_util_factory_reset exit");
  
  return;
}
#endif

size_t ICACHE_FLASH_ATTR otb_util_copy_flash_to_ram(char *dst, const char *from_ptr_byte, int size)
{
        int from, to;
        uint32_t current32, byte;
        uint8_t current8;
        const uint32_t *from_ptr;

        from_ptr = (const uint32_t *)(const void *)from_ptr_byte;

        for(from = 0, to = 0; (int)(from * sizeof(*from_ptr)) < (size - 1); from++)
        {
                current32 = from_ptr[from];

                for(byte = 4; byte > 0; byte--)
                {
                        if((current8 = (current32 & 0x000000ff)) == '\0')
                                goto done;

                        if((to + 1) >= size)
                                goto done;

                        dst[to++] = (char)current8;
                        current32 = (current32 >> 8) & 0x00ffffff;
                }
        }

done:
        dst[to] = '\0';

        return(to);
}

void ICACHE_FLASH_ATTR otb_util_convert_ws_to_(char *text)
{
  char *ws;
  
  // DEBUG("UTIL: otb_util_convert_ws_to_ entry");
  
  ws = os_strstr(text, " ");
  while (ws)
  {
    *ws = '_';
    ws = os_strstr(ws, " ");
  }

  // DEBUG("UTIL: otb_util_convert_ws_to_ exit");

  return;  
}

void ICACHE_FLASH_ATTR otb_util_convert_colon_to_period(char *text)
{
  char *ws;
  
  // DEBUG("UTIL: otb_util_convert_ws_to_ entry");
  
  ws = os_strstr(text, ":");
  while (ws)
  {
    *ws = '.';
    ws = os_strstr(ws, ":");
  }

  // DEBUG("UTIL: otb_util_convert_ws_to_ exit");

  return;  
}

void ICACHE_FLASH_ATTR otb_util_log_useful_info(bool recovery)
{
  char recovery_str[16];

  if (recovery)
  {
    os_strncpy(recovery_str, "/!!!RECOVERY!!!", 16);
  }
  else
  {
    recovery_str[0] = 0;
  }

  // Set up and log some useful info
  os_sprintf(otb_compile_date, "%s", __DATE__);
  otb_util_convert_ws_to_(otb_compile_date);
  otb_util_convert_colon_to_period(otb_compile_date);
  os_sprintf(otb_compile_time, "%s", __TIME__);
  otb_util_convert_ws_to_(otb_compile_time);
  otb_util_convert_colon_to_period(otb_compile_time);
  os_snprintf(otb_version_id,
              OTB_MAIN_MAX_VERSION_LENGTH,
              "%s:%s:%s:%s",
              OTB_MAIN_OTB_IOT,
              OTB_MAIN_FW_VERSION,
              otb_compile_date, 
              otb_compile_time);
  // First log needs a line break!
  INFO("\nOTB: %s%s", otb_version_id, recovery_str);
  INFO("OTB: Boot slot: %d", otb_rboot_get_slot(FALSE));
  os_sprintf(OTB_MAIN_CHIPID, "%06x", system_get_chip_id());
  INFO("OTB: ESP device: %s", OTB_MAIN_CHIPID);
  os_sprintf(OTB_MAIN_DEVICE_ID, "OTB-IOT.%s", OTB_MAIN_CHIPID);
  INFO("OTB: Free heap size: %d bytes", system_get_free_heap_size());
  
  if (recovery)
  {
    INFO("");
    INFO("OTB: ---------------------");
    INFO("OTB: !!! Recovery Mode !!!");
    INFO("OTB: ---------------------");
    INFO("");
  }
  return;
}

void ICACHE_FLASH_ATTR otb_util_init_logging(void)
{
  // Set up serial logging
  uart_div_modify(0, UART_CLK_FREQ / OTB_MAIN_BAUD_RATE);
  uart_div_modify(1, UART_CLK_FREQ / OTB_MAIN_BAUD_RATE);
  
  otb_util_asserting = FALSE;
  
  // Configure ESP SDK logging (only)
  system_set_os_print(OTB_MAIN_SDK_LOGGING);

  // Initialize our log buffer
  otb_util_log_buf = (char *)otb_util_log_buf_int32;
  os_memset(otb_util_log_buf, 0, OTB_UTIL_LOG_BUFFER_LEN);
  otb_util_log_buffer_struct.buffer = otb_util_log_buf;
  otb_util_log_buffer_struct.current = otb_util_log_buf;
  otb_util_log_buffer_struct.len = OTB_UTIL_LOG_BUFFER_LEN;

  // Sanity check uint16 in otb_util_log_buffer is enough
  OTB_ASSERT(OTB_UTIL_LOG_BUFFER_LEN <= 2^16);
  
  // Check log location is on a sector boundary
  OTB_ASSERT(OTB_BOOT_LOG_LOCATION % 0x1000 == 0);
  
  // Check log buffer will fit in a sector
  OTB_ASSERT(OTB_UTIL_LOG_BUFFER_LEN <= 0x1000);
  
  OTB_ASSERT((uint32)otb_util_log_buf % 4 == 0);
  
  return;
}

void ICACHE_FLASH_ATTR otb_util_log_store(void)
{
  uint8 spi_rc = SPI_FLASH_RESULT_ERR;
  char scratch[4];
  uint8_t mod4;
  int16_t write_len;
  uint16_t rem_len;
  
  // This is fiddly cos:
  // - Need to write multiple of 4 bytes
  // - We are writing a circular buffer
  // Fixed the first of these by making sure each log is 4 byte aligned, so don't need
  // to worry here (still does, but buffer will always 4 byte align).
  
  // write_len must be signed as may go negative
  // (as may need to write up 3 bytes too many)
  write_len = OTB_UTIL_LOG_BUFFER_LEN;
  
  // Going to ignore spi_rc - not much we can do if these commands fail

  // Erase the sector
  spi_rc = spi_flash_erase_sector(OTB_BOOT_LOG_LOCATION / 0x1000);
  ets_printf("log flash erased\r\n");

  ets_printf("buffer 0x%08x\r\n", otb_util_log_buffer_struct.buffer);
  ets_printf("current 0x%08x\r\n", otb_util_log_buffer_struct.current);

  // Write from current position to whatever is closest to the end but a total length of
  // a multiple of 4 bytes
  rem_len = otb_util_log_buffer_struct.len -
            (int16_t)(otb_util_log_buffer_struct.current -
                      otb_util_log_buffer_struct.buffer);
  mod4 = rem_len % 4;
  rem_len -= mod4;
  ets_printf("mod4 %d\r\n", mod4);
  ets_printf("rem_len %d\r\n", rem_len);
    
  spi_rc = spi_flash_write(OTB_BOOT_LOG_LOCATION,
                           (uint32*)otb_util_log_buffer_struct.current,
                           (uint32)rem_len);
  write_len -= rem_len;

  // Now create 4 bytes from what was left over, and the first few bytes of the buffer
  if (write_len > 0)
  {
    os_memcpy(scratch, otb_util_log_buffer_struct.current + rem_len, mod4);
    os_memcpy(scratch+rem_len, otb_util_log_buffer_struct.buffer, (4-mod4));
    spi_rc = spi_flash_write(OTB_BOOT_LOG_LOCATION + rem_len,
                             (uint32*)scratch,
                             4);
    write_len -= 4;
  }
  
  if (write_len > 0)
  {
    // Ensure multiple of 4 bytes
    write_len = ((write_len/4) + 1) * 4;
    
    // XXX NULL out the extra bytes
    spi_rc = spi_flash_write(OTB_BOOT_LOG_LOCATION + rem_len + 4,
                             (uint32*)(otb_util_log_buffer_struct.buffer + (4-mod4)),
                             write_len);
  }

  return;
}

// XXX Should really break this out§
char ICACHE_FLASH_ATTR *otb_util_get_log_ram(uint8 index)
{
  uint16_t len_b4;
  int ii;
  sint16 times;
  bool non_null;
  bool first_loop;
  char *log_start;
  sint16 len;
  sint16 new_len;
  sint16 buffer_left;
  sint16 scratch_left;

  // Write from current position to whatever is closest to the end but a total length of
  // a multiple of 4 bytes
  len_b4 = (int16_t)(otb_util_log_buffer_struct.current -
                      otb_util_log_buffer_struct.buffer);

  // go backwards looking for null string terminator
  ii = len_b4;
  ii--;
  if (ii < 0)
  {
    ii = otb_util_log_buffer_struct.len-1;
    first_loop = FALSE;
  }
  else
  {
    first_loop = TRUE;
  }
  log_start = NULL;

  for (times = index; times >= 0; times--)
  {
    non_null = FALSE;
    while (TRUE)
    {
      // Need to skip over any trailing 0s (may be up to 4).
      if (otb_util_log_buffer_struct.buffer[ii] != 0)
      {
        non_null = TRUE;
      }
    
      if (first_loop)
      {
        if (ii < 0)
        {
          // Start from end
          ii = otb_util_log_buffer_struct.len - 1;
          first_loop = FALSE;
        }
      }
      else
      {
        if (ii <= len_b4)
        {
          // Gone all the way around with no match :-(
          break;
        }
      }

      if (non_null && otb_util_log_buffer_struct.buffer[ii] == 0)
      {
        if (times == 0)
        {
          // Have found first string.  Hurrah!
          ii++;
          if (ii > (otb_util_log_buffer_struct.len - 1))
          {
            ii = 0;
          }
          log_start = otb_util_log_buffer_struct.buffer + ii;
        }
        break;
      }
      ii--;
    }
  }
  
  // This is a bit of a hack - we'll basically copy the whole buffer into the log string
  // - well as much as there's space for.  However, "our" string will be NULL terminated
  // so it works
  if (log_start != NULL)
  {
    // Have a match, copy it into otb_log_s
    scratch_left = OTB_MAIN_MAX_LOG_LENGTH;
    buffer_left = otb_util_log_buffer_struct.len - ii;
    len = OTB_MIN(buffer_left, scratch_left);
    os_memcpy(otb_log_s, log_start, len);
    scratch_left -= len;
    buffer_left -= len;
    if (buffer_left > 0)
    {
      // done
      otb_log_s[len] = 0;
    }
    else
    {
      buffer_left = otb_util_log_buffer_struct.len - len;
      log_start = otb_util_log_buffer_struct.buffer;
      new_len = OTB_MIN(buffer_left, scratch_left);
      os_memcpy(otb_log_s+len, log_start, new_len);
      scratch_left -= new_len;
      buffer_left -= new_len;
      // done
    }
    otb_log_s[OTB_MAIN_MAX_LOG_LENGTH-1] = 0;
    log_start = otb_log_s;
  }

  return(log_start);
}

void ICACHE_FLASH_ATTR otb_util_log_save(char *text)
{
  uint16_t text_len;
  uint16_t overflow_len;
  int16_t rem_len;
  uint8_t mod4;
  
  // Get string length plus the NULL terminating bytes, as we'll log this too.
  text_len = os_strlen(text) + 1;

  rem_len = otb_util_log_buffer_struct.len -
            (uint16_t)(otb_util_log_buffer_struct.current -
                       otb_util_log_buffer_struct.buffer);
           
  if (rem_len < text_len)
  {
    overflow_len = text_len - rem_len;
    os_memcpy(otb_util_log_buffer_struct.current, text, rem_len);
    os_memcpy(otb_util_log_buffer_struct.buffer, text+rem_len, overflow_len);
    otb_util_log_buffer_struct.current = otb_util_log_buffer_struct.buffer + overflow_len;
    rem_len = otb_util_log_buffer_struct.len - overflow_len;
  }
  else if (rem_len >= text_len)
  {
    os_memcpy(otb_util_log_buffer_struct.current, text, text_len);
    otb_util_log_buffer_struct.current += text_len;
    rem_len = rem_len - text_len;
  }

  // 4 byte align each log  
  mod4 = ((int32)otb_util_log_buffer_struct.current) % 4;
  if (mod4 != 0)
  { 
    mod4 = 4 - mod4;
  }
  while (mod4 > 0)
  {
    if (rem_len > 0)
    {
      *otb_util_log_buffer_struct.current = 0;
      otb_util_log_buffer_struct.current++;
    }
    else
    {
      otb_util_log_buffer_struct.current = otb_util_log_buffer_struct.buffer;
      *otb_util_log_buffer_struct.current = 0;
      rem_len = otb_util_log_buffer_struct.len;
    }
    rem_len--;
    mod4--;
  }
  
  return;
}

void ICACHE_FLASH_ATTR otb_util_log_fn(char *text)
{
  if (!OTB_MAIN_DISABLE_OTB_LOGGING)
  {
    ets_printf(text);
    ets_printf("\n");
  }
  
  otb_util_log_save(text);
  
  return;
}

char ALIGN4 otb_util_assert_error_string[] = "UTIL: Assertion Failed";
void ICACHE_FLASH_ATTR otb_util_assert(bool value, char *value_s)
{
  DEBUG("UTIL: otb_util_assert entry");

  if(!value && !otb_util_asserting)
  {
    otb_util_asserting = TRUE;
    ERROR("------------- ASSERTION FAILED -------------");
    ERROR(value_s);
    ERROR("Rebooting");
    ERROR("--------------------------------------------");
    otb_reset_error(otb_util_assert_error_string);
    otb_util_delay_ms(1000);
  }

  DEBUG("UTIL: otb_util_assert exit");
  
  return;
}


void ICACHE_FLASH_ATTR otb_reset_schedule(uint32_t timeout,
                                          const char *reason,
                                          bool error)
{

  DEBUG("UTIL: otb_reset_schedule entry");

  os_memset(&otb_reset_reason_struct, 0, sizeof(otb_reset_reason_struct));
  otb_reset_reason_struct.reason = reason;  
  otb_reset_reason_struct.error = error;  

  otb_util_timer_set((os_timer_t *)&otb_util_reset_timer,
                     (os_timer_func_t *)otb_reset_timer,
                     (void *)&otb_reset_reason_struct,
                     timeout,
                     0);

  DEBUG("UTIL: otb_reset_schedule exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_reset_timer(void *arg)
{
  struct otb_reset_reason *reason_struct = (struct otb_reset_reason *)arg;

  DEBUG("UTIL: otb_reset_timer entry");
  
  otb_reset_internal((char *)reason_struct->reason, reason_struct->error);
  
  DEBUG("UTIL: otb_reset_timer exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_reset_error(char *text)
{
  DEBUG("OTB: otb_reset_error entry");
  
  otb_reset_schedule(1000, (const char *)text, TRUE);
  
  DEBUG("OTB: otb_reset_error exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_reset(char *text)
{
  DEBUG("OTB: otb_reset entry");
  
  otb_reset_internal(text, FALSE);
  
  DEBUG("OTB: otb_reset exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_reset_internal(char *text, bool error)
{
  bool rc;
  bool same;

  DEBUG("OTB: otb_reset_internal entry");
  
  otb_util_reset_store_reason(text, &same);
  
  if (error)
  {
    ERROR(OTB_UTIL_REBOOT_TEXT);
    if (text != NULL)
    {
      ERROR(text);
    }
    if (!same)
    {
      // Only store logs if not the same reset reason as last time (to avoid destroying
      // the flash)
      otb_util_log_store();
    }
  }
  else
  {
    INFO(OTB_UTIL_REBOOT_TEXT);
    if (text != NULL)
    {
      INFO(text);
    }    
  }
  
  // Delay to give any serial logs time to output.  Can't use arduino delay as
  // may be in wrong context
  otb_util_delay_ms(1000);

  #if 0
  // Reset by pulling reset GPIO (connected to reset) low
  // XXX Only works for GPIO 16
  WRITE_PERI_REG(RTC_GPIO_OUT,
                   (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(0));
  #else
  system_restart();
  #endif

  DEBUG("OTB: otb_reset_internal exit");

  return;
}

bool ICACHE_FLASH_ATTR otb_util_reset_store_reason(char *text, bool *same)
{
  bool rc = TRUE;
  uint8 tries = 0;
  uint16 len;
  uint8 mod4;
  uint8 spi_rc;
  char ALIGN4 stored[OTB_BOOT_LAST_REBOOT_LEN];

  DEBUG("UTIL: otb_util_reset_store_reason entry");
  
  *same = FALSE;
  
  // First read the current stored string - as only want to write if it differs (so we 
  // don't write the flash too many times
  
  rc = otb_util_flash_read((uint32)OTB_BOOT_LAST_REBOOT_REASON,
                           (uint32 *)stored,
                           OTB_BOOT_LAST_REBOOT_LEN);
  if (rc)
  {
    if (os_strncmp(text, stored, OTB_BOOT_LAST_REBOOT_LEN))
    {
      len = os_strlen(text);
      spi_rc = spi_flash_erase_sector(OTB_BOOT_LAST_REBOOT_REASON / 0x1000);
      rc = otb_util_flash_write_string((uint32)OTB_BOOT_LAST_REBOOT_REASON,
                                       (uint32 *)text,
                                       (uint32)len);
      if (!rc)
      {
        DEBUG("UTIL: Failed to actually store reset reason in flash");
      }
    }
    else
    {
      DEBUG("UTIL: New reset reason same as old one");
      *same = TRUE;
    }
  }
  
  if (!rc)
  {
    ERROR("UTIL: Failed to store reset reason");
  }
 
EXIT_LABEL:

  DEBUG("UTIL: otb_util_reset_store_reason exit");

  return rc;
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

bool ICACHE_FLASH_ATTR otb_util_flash_read(uint32 location,
                                           uint32 *dest,
                                           uint32 len)
{
  bool rc = TRUE;
  uint8 tries;
  uint8 spi_rc = SPI_FLASH_RESULT_ERR;

  DEBUG("UTIL: otb_util_flash_read_string entry");
  
  // Check everything is 4 byte aligned
  OTB_ASSERT((uint32)location % 4 == 0);
  OTB_ASSERT((uint32)dest % 4 == 0);
  OTB_ASSERT((uint32)len % 4 == 0);
  
  tries = 0;
  while ((tries < 2) && (spi_rc != SPI_FLASH_RESULT_OK))
  {
    spi_rc = spi_flash_read(location, dest, len);
  }

  if (tries == 2)
  {
    ERROR("UTIL: Failed to read at 0x%08x, error %d", location, spi_rc);
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
EXIT_LABEL:  
  
  DEBUG("UTIL: otb_util_flash_read_string exit");
  
  return rc;
}                                                  

bool ICACHE_FLASH_ATTR otb_util_flash_write_string(uint32 location,
                                                   uint32 *source,
                                                   uint32 len)
{
  bool rc;
  uint32 newlen;
  uint8 mod4;
  
  DEBUG("UTIL: otb_util_flash_write_string entry");
  
  // 4 byte align the string - this should be safeish - we're not writing past the end
  // of the passed in string, but we are ready past it so may log some rubbish
  mod4 = len % 4;
  newlen = len + (4-mod4);

  rc = otb_util_flash_write(location, source, newlen);
  
  DEBUG("UTIL: otb_util_flash_write_string exit");
  
  return(rc);
}

bool ICACHE_FLASH_ATTR otb_util_flash_write(uint32 location, uint32 *source, uint32 len)
{
  bool rc = TRUE;
  uint8 tries;
  uint8 spi_rc = SPI_FLASH_RESULT_ERR;
  
  DEBUG("UTIL: otb_util_flash_write entry");
  
  // Check everything is 4 byte aligned
  OTB_ASSERT((uint32)location % 4 == 0);
  OTB_ASSERT((uint32)source % 4 == 0);
  OTB_ASSERT((uint32)len % 4 == 0);

  tries = 0;
  while ((tries < 2) && (spi_rc != SPI_FLASH_RESULT_OK))
  {
    spi_rc = spi_flash_write(location, source, len);
    tries++;
  }

  if (tries == 2)
  {
    ERROR("UTIL: Failed to write at 0x%08x, error %d", location, spi_rc);
    rc = FALSE;
    goto EXIT_LABEL;
  }
  
EXIT_LABEL:  

  DEBUG("UTIL: otb_util_flash_write exit");
  
  return rc;
}
 
void ICACHE_FLASH_ATTR otb_util_get_heap_size(void)
{
  uint32 size;
  char size_s[8];

  DEBUG("UTIL: otb_util_get_heap_size entry");
  
  size = system_get_free_heap_size();  
  os_snprintf(size_s, 8, "%d", size);
  otb_mqtt_send_status(OTB_MQTT_STATUS_HEAP_SIZE, size_s, "", "");
  
  DEBUG("UTIL: otb_util_get_heap_size exit");

  return;
}  

bool otb_util_vdd33_flash_ok = FALSE;

void ICACHE_FLASH_ATTR otb_util_get_vdd33(void)
{
  uint16 vdd33;
  char output[16];
  double voltage;
  int voltage_int;
  int voltage_thou;
  unsigned char ALIGN4 flash[0x1000];
  bool rc;
  uint8 spi_rc = SPI_FLASH_RESULT_ERR;

  DEBUG("UTIL: otb_util_get_vdd33 entry");

  // To work byte 107 of ESP user.bin (0x3fc000 in 4MB flash) must be 0xff.  Really.
  // And you need to reboot afterwards.
  if (!otb_util_vdd33_flash_ok)
  {
    // Sigh.  Make it OK.
    DEBUG("UTIL: read flash 0x%p", flash);
    rc = otb_util_flash_read(OTB_BOOT_ESP_USER_BIN,
                             (uint32 *)flash,
                             0x1000);
    if (rc)
    {
      if (flash[107] != 0xff)
      {
        DEBUG("UTIL: write flash");
        flash[107] = 0xff;
        spi_rc = spi_flash_erase_sector(OTB_BOOT_ESP_USER_BIN / 0x1000);
        if (spi_rc != SPI_FLASH_RESULT_ERR)
        {
          rc = otb_util_flash_write(OTB_BOOT_ESP_USER_BIN,
                                    (uint32 *)flash,
                                    0x1000);
        }
        else
        {
          WARN("UTIL: Failed to erase flash");
          rc = FALSE;
        }
        if (rc)
        {
          otb_util_vdd33_flash_ok = TRUE;
          otb_mqtt_send_status(OTB_MQTT_STATUS_VDD33, OTB_MQTT_STATUS_ERROR, "reset first", "");
          goto EXIT_LABEL;
        }
        else
        {
          WARN("UTIL: Failed to update ESP user bin to flash 0x%x", OTB_BOOT_ESP_USER_BIN);
        }
      }
      else
      {
        DEBUG("UTIL: Byte 107 of ESP user bin 0x%x already 0xff", OTB_BOOT_ESP_USER_BIN);
        otb_util_vdd33_flash_ok = TRUE;
      }
    }
    else
    {
      WARN("UTIL: Failed to read ESP user bin from flash 0x%x", OTB_BOOT_ESP_USER_BIN);
    }
  }  


  if (otb_util_vdd33_flash_ok)  
  {
    vdd33 = system_get_vdd33();  
    //vdd33 = system_adc_read();
    voltage = vdd33/1024;
    voltage_int = voltage/1;
    voltage_thou = (int)(vdd33*1000/1024)%1000;
    os_snprintf(output, 16, "0x%04x/%d.%03dV", vdd33, voltage_int, voltage_thou);
    otb_mqtt_send_status(OTB_MQTT_STATUS_VDD33, output, "", "");
  }
  else
  {
    otb_mqtt_send_status(OTB_MQTT_STATUS_VDD33, OTB_MQTT_STATUS_ERROR, "", "");
  }

EXIT_LABEL:
  
  DEBUG("UTIL: otb_util_get_vdd33 exit");

  return;
}  

void ICACHE_FLASH_ATTR otb_util_timer_cancel(os_timer_t *timer)
{

  DEBUG("UTIL: otb_util_timer_cancel entry");
  
  // Set the timer function to NULL so we can test whether this is set
  os_timer_disarm(timer);
  os_timer_setfn(timer, (os_timer_func_t *)NULL, NULL);
  
  DEBUG("UTIL: otb_util_timer_cancel exit");
  
  return;
}

bool ICACHE_FLASH_ATTR otb_util_timer_is_set(os_timer_t *timer)
{
  bool rc = TRUE;
  
  DEBUG("UTIL: otb_util_timer_is_set entry");
  
  if (timer->timer_func == NULL)
  {
    DEBUG("UTIL: timer isn't set");
    rc = FALSE;
  }
  
  DEBUG("UTIL: otb_util_timer_is_set exit");

  return(rc);
}

void ICACHE_FLASH_ATTR otb_util_timer_set(os_timer_t *timer,
                                          os_timer_func_t *timerfunc,
                                          void *arg,
                                          uint32_t timeout,
                                          bool repeat)
{

  DEBUG("UTIL: otb_util_timer_set entry");

  otb_util_timer_cancel(timer);
  os_timer_setfn(timer, timerfunc, arg);
  os_timer_arm(timer, timeout, repeat);
  
  DEBUG("UTIL: otb_util_timer_set exit");
  
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
  otb_util_log_fn(otb_log_s);

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

void ICACHE_FLASH_ATTR otb_init_mqtt(void *arg)
{
  DEBUG("OTB: otb_init_mqtt entry");
  
  INFO("OTB: Set up MQTT stack");
  otb_mqtt_initialize(otb_conf->mqtt.svr,
                      otb_conf->mqtt.port,
                      0,
                      OTB_MAIN_DEVICE_ID,
                      otb_conf->mqtt.user,
                      otb_conf->mqtt.pass,
                      OTB_MQTT_KEEPALIVE);

  // Now set up DS18B20 init
  os_timer_disarm((os_timer_t*)&init_timer);
  os_timer_setfn((os_timer_t*)&init_timer, (os_timer_func_t *)otb_init_ds18b20, NULL);
  os_timer_arm((os_timer_t*)&init_timer, 500, 0);  

  DEBUG("OTB: otb_init_mqtt entry");

  return;
}

void ICACHE_FLASH_ATTR otb_init_ds18b20(void *arg)
{
  DEBUG("OTB: otb_init_ds18b20 entry");

  INFO("OTB: Set up One Wire bus");
  otb_ds18b20_initialize(OTB_DS18B20_DEFAULT_GPIO);

  // Now set up ADS init
  os_timer_disarm((os_timer_t*)&init_timer);
  os_timer_setfn((os_timer_t*)&init_timer, (os_timer_func_t *)otb_init_ads, NULL);
  os_timer_arm((os_timer_t*)&init_timer, 500, 0);  


  DEBUG("OTB: otb_init_ds18b20 exit");

  return;
}

void ICACHE_FLASH_ATTR otb_init_ads(void *arg)
{
  DEBUG("OTB: otb_init_ads entry");

  INFO("OTB: Set up ADS (+I2C bus)");
  otb_ads_initialize();

  // Now set up ADS init
  os_timer_disarm((os_timer_t*)&init_timer);

  DEBUG("OTB: otb_init_ads exit");

  return;
}

size_t ICACHE_FLASH_ATTR otb_util_strnlen(const char *s, size_t maxlen)
{
  size_t ii;
  for (ii = 0; ii < maxlen; ii++)
  {
    if (s[ii] == 0)
    {
      break;
    }
  }
  return ii;
}

char* ICACHE_FLASH_ATTR strcat(char * dest, const char * src) {
    return strncat(dest, src, strlen(src));
}

char* ICACHE_FLASH_ATTR strncat(char * dest, const char * src, size_t n) {
    size_t i;
    size_t offset = strlen(dest);
    for(i = 0; i < n && src[i]; i++) {
        dest[i + offset] = src[i];
    }
    dest[i + offset] = 0;
    return dest;
}

int ICACHE_FLASH_ATTR islower(int c) {
    if(c >= 'a' && c <= 'z') {
        return 1;
    }
    return 0;
}

int ICACHE_FLASH_ATTR isupper(int c) {
    if(c >= 'A' && c <= 'Z') {
        return 1;
    }
    return 0;
}

int ICACHE_FLASH_ATTR isalpha(int c) {
    if(islower(c) || isupper(c)) {
        return 1;
    }
    return 0;
}

int ICACHE_FLASH_ATTR isdigit(int c) {
    if(c >= '0' && c <= '9') {
        return 1;
    }
    return 0;
}

int ICACHE_FLASH_ATTR isalnum(int c) {
    if(isalpha(c) || isdigit(c)) {
        return 1;
    }
    return 0;
}

int ICACHE_FLASH_ATTR iscntrl(int c) {
    if(c <= 0x1F || c == 0x7F) {
        return 1;
    }
    return 0;
}

int ICACHE_FLASH_ATTR isprint(int c) {
    if(!iscntrl(c)) {
        return 1;
    }
    return 0;
}

int ICACHE_FLASH_ATTR isgraph(int c) {
    if(isprint(c) && c != ' ') {
        return 1;
    }
    return 0;
}

int ICACHE_FLASH_ATTR ispunct(int c) {
    if(isgraph(c) && !isalnum(c)) {
        return 1;
    }
    return 0;
}

int ICACHE_FLASH_ATTR isxdigit(int c) {
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

int ICACHE_FLASH_ATTR tolower(int c) {
    if(isupper(c)) {
        c += 0x20;
    }
    return c;
}

int ICACHE_FLASH_ATTR toupper(int c) {
    if(islower(c)) {
        c -= 0x20;
    }
    return c;
}

int ICACHE_FLASH_ATTR isblank(int c) {
    switch(c) {
        case 0x20: // ' '
        case 0x09: // '\t'
            return 1;
    }
    return 0;
}


int ICACHE_FLASH_ATTR isspace(int c) {
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

long ICACHE_FLASH_ATTR strtol(const char *nptr, char **endptr, int base) {
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

long ICACHE_FLASH_ATTR atol(const char* s) {
    char * tmp;
    return strtol(s, &tmp, 10);
}

int ICACHE_FLASH_ATTR atoi(const char* s) {
    return (int) atol(s);
}

static int errno_var = 0;

int* __errno(void) {
    // DEBUGV("__errno is called last error: %d (not current)\n", errno_var);
    return &errno_var;
}

