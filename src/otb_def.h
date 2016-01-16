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

// Configurable stuff
#define OTB_MAIN_FW_VERSION "v0.1c" //max 8 chars
#define OTB_MAIN_MAX_VERSION_LENGTH 12 + 9 + 8 + 8 + 1// date, time, version, root
#define OTB_MAIN_GPIO_RESET 16
#define OTB_MAIN_MAX_LOG_LENGTH 1024
#define OTB_WIFI_DEFAULT_STA_TIMEOUT 30000 // 30 seconds
#define OTB_WIFI_DEFAULT_AP_TIMEOUT 300000 // 5 minutes
#define OTB_WIFI_DEFAULT_SSID_PREFIX "OTB-IOT"
#define OTB_WIFI_DEFAULT_DUMMY_SSID "otb-iot-dummy"
#define OTB_WIFI_MAC_ADDRESS_STRING_LENGTH 18 // 6 * 2 + 5 + 1
#define OTB_OW_MAX_DS18B20S 8
#define OTB_OW_DEFAULT_GPIO 2
#define OTB_OW_MAX_TEMP_LEN 7 
#define OTB_OW_REPORT_INTERVAL 60000 // 1 minute
#define OTB_MQTT_HEARTBEAT_INTERVAL 60000 // 1 minute
#define OTB_MQTT_DISCONNECTED_REBOOT_INTERVAL 180000 // 3 minutes
#define OTB_MQTT_QUEUE_BUFFER_SIZE 2048
#define OTB_MQTT_MAX_TOPIC_LENGTH 128
#define OTB_MQTT_MAX_MSG_LENGTH 64
#define OTB_MAIN_OTB_IOT "otb_iot" // max 8 chars
#define OTB_MQTT_KEEPALIVE 120 // seconds
#define OTB_MQTT_ROOT OTB_MAIN_OTB_IOT // max 8 chars
#define OTB_MQTT_SERVER "192.168.0.162" // DNS hostnames are fine
#define OTB_MQTT_PORT 1880
#define OTB_MQTT_ALL "all"
#define OTB_MQTT_LOCATION_1 "home"
#define OTB_MQTT_LOCATION_2 "office"
#define OTB_MQTT_LOCATION_3 "rad"
#define OTB_MQTT_TEMPERATURE "temp"
#define OTB_MQTT_LOCATION_4_OPT "rad"
#define OTB_MQTT_CMD_SYSTEM "system"
#define OTB_MQTT_CMD_RESET "reset"
#define OTB_MQTT_CMD_REBOOT "reboot"
#define OTB_MQTT_CMD_UPGRADE "upgrade"
#define OTB_MQTT_PUB_STATUS "status"
#define OTB_MQTT_STATUS_BOOTED "booted"
#define OTB_MQTT_STATUS_VERSION "version"
#define OTB_MQTT_STATUS_CHIPID "chipid"
#define OTB_MQTT_STATUS_OFFLINE "offline"

// Fixed stuff
#define OTB_MAIN_COMPILE_DATE otb_compile_date
#define OTB_MAIN_COMPILE_TIME otb_compile_time
#define OTB_MAIN_VERSION_ID otb_version_id
#define OTB_MAIN_CHIPID_STR_LENGTH 7
#define OTB_MAIN_CHIPID otb_chipid
#define OTB_WIFI_MAX_IPV4_STRING_LEN 16 // 3+1+3+1+3+1+3+1
#define OTB_OW_MAX_ADDRESS_STRING_LENGTH 16 // 7*2 +2
#define OTB_ESP_MAX_DELAY_MS 65 // os_delay_us can delay max 65535 us
#define OTB_UTIL_DELAY_WAIT_MS 30 
#define OTB_UTIL_MAX_DELAY_MS 0xffffffff / 1000 // max 32 bit int in us
#define OTB_SCHED_MQTT_TASK 1
#define OTB_SCHED_OW_TASK 2
