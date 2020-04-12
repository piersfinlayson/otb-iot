/*
 *
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-2020 Piers Finlayson
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
 * 
 */

#ifndef OTB_H_INCLUDED
#define OTB_H_INCLUDED

//#define OTB_FLASH_ATTR __attribute__((section(".otb-iot.rodata"))) __attribute__((aligned(sizeof(char*))))
#define OTB_FLASH_ATTR

#define USE_US_TIMER

// #define OTB_DEBUG 1
// #define OTB_ARDUINO 1

#ifdef OTB_ARDUINO
#include <Arduino.h>
#endif

// Standard libraries
#include "stdio.h"
#include "stdarg.h"

#ifndef ESPUT
// RTOS SDK include
#include "esp_types.h"
#include "esp_attr.h"
#include "esp_clk.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_idf_version.h"
#include "esp_interface.h"
#include "esp_libc.h"
#include "esp_log.h"
// #include "esp_now.h"
#include "esp_phy_init.h"
#include "esp_sleep.h"
#include "esp_smartconfig.h"
// #include "esp_ssc.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_wifi.h"
// #include "esp_wifi_crypto_types.h"
// #include "esp_wifi_osi.h"
#include "esp_wifi_types.h"
#include "esp_wpa2.h"
#include "esp_wps.h"
#include "os.h"
#include "gpio.h"
#include "spi_flash.h"
#include "uart.h"
//#include "uart_select.h"
//#include "esp8266/uart_register.h"
//#include "esp8266/uart_struct.h"
//#include "rom_functions.h"
//#include "tcpip_adapter.h"
//#include "lwip/err.h"
//#include "lwip/dns.h"

typedef uint8_t BOOL;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int8_t sint8;
typedef int16_t sint16;
typedef int32_t sint32;
typedef int8_t sint8_t;
typedef int16_t sint16_t;
typedef int32_t sint32_t;
typedef os_timer_t ETSTimer;
typedef os_timer_func_t ETSTimerFunc;
typedef system_event_t System_Event_t;
typedef tcpip_adapter_ip_info_t ip_info;
#define FALSE 0
#define TRUE 1
#undef os_snprintf
#define os_snprintf snprintf
#ifndef os_strcpy
#define os_strcpy strcpy
#endif // os_strcpy
#define GPIO_DIS_OUTPUT(gpio_no)        gpio_output_set(0,0,0, 1<<gpio_no)
#define GPIO_OUTPUT_SET(gpio_no, bit_value) \
    gpio_output_set((bit_value)<<gpio_no, ((~(bit_value))&0x01)<<gpio_no, 1<<gpio_no,0)
extern uint32_t esp_get_time(void);
#define system_get_time esp_get_time
extern void os_timer_arm_us ( os_timer_t * ptimer, uint32 usec, bool repeat_flag );
extern uint32 system_get_chip_id ( void );
#define STATION_IF WIFI_IF_STA
#define SOFTAP_IF WIFI_IF_AP
#define wifi_get_macaddr esp_wifi_get_mac
#define system_get_free_heap_size esp_get_free_heap_size
#define ETS_INTR_LOCK ets_intr_lock
#define ETS_INTR_UNLOCK ets_intr_lock
#define wifi_station_set_auto_connect esp_wifi_set_auto_connect
#define os_sprintf sprintf
#define espconn_dns_getserver dns_getserver
#define wifi_get_ip_info tcpip_adapter_get_ip_info 
#define gpio_pin_intr_state_set gpio_set_intr_type

// otb-iot includes
#include "esp_systemapi.h"
#include "rom/ets_sys.h"
#include "sdkconfig.h"
#else
#include "esput.h"
#endif // ESPUT

// MQTT
#ifndef ESPUT
#else
// XXX
#endif // ESPUT
#include "mqtt_msg.h"
#include "mqtt_user_config.h"
#include "mqtt.h"
#include "queue.h"
#include "utils.h"
#include "proto.h"
#include "ringbuf.h"

// HTTPD
#include "captdns.h"

// DS18B20
#include "pin_map.h"

// RBOOT
#include "rboot_ota.h"

// Softuart
#include "softuart.h"

// brzo_i2c
#include "brzo_i2c.h"

// OTB
#include "otb_def.h"
#include "otb_globals.h"
#include "otb_macros.h"
#include "otb_font.h"
#include "otb_util.h"
#include "otb_main.h"
#include "otb_wifi.h"
#include "otb_mqtt.h"
#include "otb_conf.h"
#include "otb_i2c.h"
#include "otb_i2c_pca9685.h"
#include "otb_i2c_mcp23017.h"
#include "otb_i2c_pcf8574.h"
#include "otb_i2c_24xxyy.h"
#include "otb_ds18b20.h"
#include "otb_rboot.h"
#include "otb_gpio.h"
#include "otb_httpd.h"
#include "otb_led.h"
#include "otb_serial.h"
#include "otb_mbus.h"
#include "otb_eeprom.h"
#include "otb_flash.h"
#include "otb_relay.h"
#include "otb_brzo_i2c.h"
#include "otb_board.h"
#include "otb_nixie.h"
#include "otb_intr.h"
#include "otb_break.h"
#include "otb_cmd.h"

#endif
