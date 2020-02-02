/*
 *
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-2018 Piers Finlayson
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
// ESP SDK
#include "osapi.h"
#include "c_types.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "smartconfig.h"
#include "esp_systemapi.h"
#include "uart.h"
#include "esp_sdk_ver.h"
#include "espconn.h"
#else
#include "esput.h"
#endif // ESPUT

// MQTT
#ifndef ESPUT
#include "espconn.h"
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
#include "gpio.h"
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
