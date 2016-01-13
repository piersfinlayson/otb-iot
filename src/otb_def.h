/*
 *
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
 * 
 */

#define DEFAULT_WIFI_TIMEOUT 60 // 1 minute
//#define DEFAULT_WIFI_TIMEOUT 300 // 5 minutes
#define COMPILE_DATE compile_date
#define COMPILE_iTIME compile_time
#define OTB_FW_VERSION "v0.1b" //max 8 chars
#define OTB_VERSION_ID version_id
#define MAX_VERSION_ID_LENGTH 12 + 9 + 8 + 8 // date, time, version, root
#define MAX_DS18B20S 8
#define MAX_ONEWIRE_ADDRESS_STRING_LENGTH 16 // 7*2 +2
#define DEFAULT_WIFI_SSID_PREFIX "OTB-IOT"
#define ONE_WIRE_BUS 2
#define CHIPID_STR_LENGTH 7
#define REPORT_INTERVAL 60000 // 1 minute
#define DISCONNECTED_REBOOT_INTERVAL 180000 // 3 minutes
#define MQTT_QUEUE_BUFFER_SIZE 2048
#define OTB_ROOT "otb_iot" // max 7 chars
#define LOCATION_1 "home"
#define LOCATION_2 "office"
#define LOCATION_3 "rad"
#define TEMPERATURE "temp"
#define LOCATION_4_OPT "rad"
#define SYSTEM_CMD "system"
#define RESET_CMD "reset"
#define REBOOT_CMD "reboot"
#define UPGRADE_CMD "upgrade"
#define BOOTED_STATUS "booted"
#define OFFLINE "offline"
#define MQTT_SERVER "192.168.0.162" // DNS hostnames fine
#define MAX_LOG_LENGTH 1024
#define MAX_TOPIC_LENGTH 128
#define MAX_MSG_LENGTH 64
#define OTB_CHIPID chipid
#define GPIO_RESET 16
