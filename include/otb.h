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

// Standard libraries
#include "stdio.h"
#include "stdarg.h"

// RTOS SDK includes
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
#include "esp_phy_init.h"
#include "esp_sleep.h"
#include "esp_smartconfig.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_wpa2.h"
#include "esp_wps.h"
#include "os.h"
#include "driver/gpio.h"
#include "spi_flash.h"
#include "driver/uart.h"
#include "driver/i2c.h"

typedef uint8_t BOOL;
#define TRUE  1
#define FALSE 0
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

#ifdef CONFIG_LOG_DEFAULT_LEVEL_VERBOSE
#define OTB_DEBUG
#endif 

// otb-iot includes
#include "otb_def.h"
#include "otb_macros.h"
#include "otb_main.h"
#include "otb_util.h"
#include "otb_eeprom.h"
#include "otb_i2c.h"
#include "otb_i2c_24xxyy.h"
#include "otb_globals.h"

#endif // OTB_H_INCLUDED
