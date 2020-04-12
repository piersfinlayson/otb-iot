/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-8 Piers Finlayson
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

#define OTB_MAIN_C
#include "otb.h"

MLOG("otb_main");

static void otb_main_init(void *arg);
static void otb_main_init_uart(void);
static void otb_main_check_log_level(void);
static void otb_main_process_log_level(char log_level);
uint8_t otb_util_log_level;

typedef struct {
  size_t buf_len;
  size_t chars;
  char *buf;
} otb_main_rx_buf;

void app_main(void)
{
  // According to docs required as first step, to enable us timer
  // Note that this means that maximum os_timer_arm (not us) value is about
  // 432s (0x689D0 in ms)
  // XXX RTOS change
  // prob not needed as will be using freertos timers
  //system_timer_reinit();

  // XXX RTOS change
  // Prob not needed as using IDF logging - but do need to choose log level
  //otb_util_init_logging();

  ENTRY;

  INFO("otb-iot application started");


  // See if user wants to override log level
  // XXX RTOS change
  xTaskCreate(&otb_main_init,
              "otb_main_init",
              2048,
              NULL,
              3,
              NULL);

    EXIT;

  return;
}

void otb_main_init(void *arg)
{
  ENTRY;

  MDETAIL("Check required log level");
  otb_main_check_log_level();

  MDETAIL("Initialize networking");
  tcpip_adapter_init();


  vTaskDelete(NULL);

  EXIT;

  return;
}

void otb_main_init_uart(void)
{
  uart_config_t uart_config;

  ENTRY;

  uart_config.baud_rate = 74880;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_param_config(UART_NUM_0, &uart_config);

  EXIT;
}

void otb_main_check_log_level(void)
{
  uint8_t *byte;
  int read;

  ENTRY;

  otb_main_init_uart();
  uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
  read = uart_read_bytes(UART_NUM_0, &byte, 1, 250/portTICK_RATE_MS);
  uart_driver_delete(UART_NUM_0);

  if (read > 0)
  {
    MDETAIL("Detected log level input");
    otb_main_process_log_level(byte);
  }
  else
  {
    MDETAIL("No log level input detected");
  }
 
  EXIT;

  return;
}

void otb_main_process_log_level(char log_level)
{
  ENTRY;

  switch (log_level)
  {
    case '0':
      otb_util_log_level = ESP_LOG_NONE;
      break;

    case '1':
      otb_util_log_level = ESP_LOG_ERROR;
      break;

    case '2':
      otb_util_log_level = ESP_LOG_WARN;
      break;

    case '3':
      otb_util_log_level = ESP_LOG_INFO;
      break;
      
    case '4':
      otb_util_log_level = ESP_LOG_DEBUG;
      break;
      
    case '5':
      // Note VERBOSE (OTB_DEBUG) may not be compiled in - log this later
      otb_util_log_level = ESP_LOG_VERBOSE;
      break;

    default:
      MWARN("Unexpected character received when checking log level 0x%02x, using default %d", log_level, otb_util_log_level);
  }

  esp_log_level_set("*", otb_util_log_level);

  switch (otb_util_log_level)
  {
    case ESP_LOG_VERBOSE:
      MINFO("Log level selected: VERBOSE");
#ifndef OTB_DEBUG
        MERROR("VERBOSE logging selected, but not compiled into firmware");
#endif // OTB_DEBUG          
      break;

    case ESP_LOG_DEBUG:
      MINFO("Log level selected: DETAIL");
      break;

    case ESP_LOG_INFO:
      MINFO("Log level selected: INFO");
      break;

    case ESP_LOG_WARN:
      MINFO("Log level selected: WARN");
      break;

    case ESP_LOG_ERROR:
      MINFO("Log level selected: ERROR");
      break;

    case ESP_LOG_NONE:
      MINFO("Log level selected: NONE");
      break;

    default:
      break;
  }

  EXIT;
}

bool otb_util_asserting = FALSE;

char ALIGN4 otb_util_assert_error_string[] = "UTIL: Assertion Failed";
void ICACHE_FLASH_ATTR otb_util_assert(bool value, char *value_s, char *file, uint32_t line)
{
  ENTRY;

  if(!value && !otb_util_asserting)
  {
    otb_util_asserting = TRUE;
    ERROR("------------- ASSERTION FAILED -------------");
    ERROR_VAR(value_s);
    ERROR("File: %s Line: %d", file, line);
    ERROR("Rebooting");
    ERROR("--------------------------------------------");
    // XXX RTOS Change
    DELAY(1000);
    esp_restart();
  }

  EXIT;
  
  return;
}

#if 0
// user_pre_init is required from SDK v3.0.0 onwards
// It is used to register the parition map with the SDK, primarily to allow
// the app to use the SDK's OTA capability.  We don't make use of that in 
// otb-iot and therefore the only info we provide is the mandatory stuff:
// - RF calibration data
// - Physical data
// - System parameter
// The location and length of these are from the 2A SDK getting started guide
void ICACHE_FLASH_ATTR user_pre_init(void)
{
  bool rc = false;
  static const partition_item_t part_table[] = 
  {
    {SYSTEM_PARTITION_RF_CAL,
     OTB_BOOT_RF_CAL,
     OTB_BOOT_RF_CAL_LEN},
    {SYSTEM_PARTITION_PHY_DATA,
     OTB_BOOT_PHY_DATA,
     OTB_BOOT_PHY_DATA_LEN},
    {SYSTEM_PARTITION_SYSTEM_PARAMETER,
     OTB_BOOT_SYS_PARAM,
     OTB_BOOT_SYS_PARAM_LEN},
  };

  // This isn't an ideal approach but there's not much point moving on unless
  // or until this has succeeded cos otherwise the SDK will just barf and 
  // refuse to call user_init()
  while (!rc)
  {
    rc = system_partition_table_regist(part_table,
				       sizeof(part_table)/sizeof(part_table[0]),
                                       4);
  }

  return;
}
#endif