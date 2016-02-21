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

#ifndef OTB_MQTT_H
#define OTB_MQTT_H

#define OTB_MQTT_MAX_CMDS  5

#define OTB_MQTT_EMPTY   otb_mqtt_string_empty
#define OTB_MQTT_SLASH   otb_mqtt_string_slash
#define OTB_MQTT_COLON   otb_mqtt_string_colon
#define OTB_MQTT_TRUE    otb_mqtt_string_true
#define OTB_MQTT_FALSE   otb_mqtt_string_false

#define OTB_MQTT_TOPIC_INVALID_ 0
#define OTB_MQTT_TOPIC_SYSTEM   "system"
#define OTB_MQTT_TOPIC_SYSTEM_  1
#define OTB_MQTT_TOPIC_STATUS   "status"
#define OTB_MQTT_TOPIC_STATUS_  2
#define OTB_MQTT_TOPIC_ERROR    "error"
#define OTB_MQTT_TOPIC_LOG      "log"

#define OTB_MQTT_SYSTEM_INVALID_   255
#define OTB_MQTT_SYSTEM_CMD_FIRST_ 0
#define OTB_MQTT_SYSTEM_CONFIG     "config"
#define OTB_MQTT_SYSTEM_CONFIG_    0
#define OTB_MQTT_SYSTEM_GPIO       "gpio"
#define OTB_MQTT_SYSTEM_GPIO_      1
#define OTB_MQTT_SYSTEM_BOOT_SLOT  "boot_slot"
#define OTB_MQTT_SYSTEM_BOOT_SLOT_ 2
#define OTB_MQTT_SYSTEM_HEAP_SIZE  "heap_size"
#define OTB_MQTT_SYSTEM_HEAP_SIZE_ 3
#define OTB_MQTT_SYSTEM_DS18B20    "ds18b20"
#define OTB_MQTT_SYSTEM_DS18B20_   4
#define OTB_MQTT_SYSTEM_UPDATE     "update"
#define OTB_MQTT_SYSTEM_UPDATE_    5
#define OTB_MQTT_SYSTEM_UPGRADE    "upgrade"
#define OTB_MQTT_SYSTEM_UPGRADE_   6
#define OTB_MQTT_SYSTEM_RESET      "reset"
#define OTB_MQTT_SYSTEM_RESET_     7
#define OTB_MQTT_SYSTEM_REBOOT     "reboot"
#define OTB_MQTT_SYSTEM_REBOOT_    8 
#define OTB_MQTT_SYSTEM_PING       "ping"
#define OTB_MQTT_SYSTEM_PING_      9
#define OTB_MQTT_SYSTEM_RSSI       "rssi"
#define OTB_MQTT_SYSTEM_RSSI_      10
#define OTB_MQTT_SYSTEM_RSSI       "reason"
#define OTB_MQTT_SYSTEM_RSSI_      11
#define OTB_MQTT_SYSTEM_CMD_LAST_  11

extern char *otb_mqtt_system_cmds[];
#ifdef OTB_MQTT_C
char *otb_mqtt_system_cmds[OTB_MQTT_SYSTEM_CMD_LAST_ + 1] =
{
  OTB_MQTT_SYSTEM_CONFIG,
  OTB_MQTT_SYSTEM_GPIO,
  OTB_MQTT_SYSTEM_BOOT_SLOT,
  OTB_MQTT_SYSTEM_HEAP_SIZE,
  OTB_MQTT_SYSTEM_DS18B20,
  OTB_MQTT_SYSTEM_UPDATE,
  OTB_MQTT_SYSTEM_UPGRADE,
  OTB_MQTT_SYSTEM_RESET,
  OTB_MQTT_SYSTEM_REBOOT,
  OTB_MQTT_SYSTEM_PING, 
  OTB_MQTT_SYSTEM_RSSI,
  OTB_MQTT_SYSTEM_REASON
};
#endif // OTB_MQTT_C

#define OTB_MQTT_CMD_INVALID_      255
#define OTB_MQTT_CMD_FIRST_        0
#define OTB_MQTT_CMD_GET           "get"
#define OTB_MQTT_CMD_GET_          0
#define OTB_MQTT_CMD_GET_NUM       "get_num"
#define OTB_MQTT_CMD_GET_NUM_      1
#define OTB_MQTT_CMD_SET           "set"
#define OTB_MQTT_CMD_SET_          2
#define OTB_MQTT_CMD_LAST_         2

extern char *otb_mqtt_cmds[];
#ifdef OTB_MQTT_C
char *otb_mqtt_cmds[OTB_MQTT_CMD_LAST_ + 1] = 
{
  OTB_MQTT_CMD_GET,
  OTB_MQTT_CMD_GET_NUM,
  OTB_MQTT_CMD_SET
};
#endif

#define OTB_MQTT_CMD_GET_INDEX     "index"
#define OTB_MQTT_CMD_SET_CLEAR     "clear"

#define OTB_MQTT_STATUS_GPIO       OTB_MQTT_SYSTEM_GPIO
#define OTB_MQTT_STATUS_BOOT_SLOT  OTB_MQTT_SYSTEM_BOOT_SLOT
#define OTB_MQTT_STATUS_HEAP_SIZE  OTB_MQTT_SYSTEM_HEAP_SIZE
#define OTB_MQTT_STATUS_PONG       "pong"
#define OTB_MQTT_STATUS_OK         "ok"
#define OTB_MQTT_STATUS_ERROR      "error"
#define OTB_MQTT_STATUS_BOOTED "booted"
#define OTB_MQTT_STATUS_VERSION "version"
#define OTB_MQTT_STATUS_CHIPID "chipid"

#define OTB_MQTT_CONFIG_TRUE            "true"
#define OTB_MQTT_CONFIG_FALSE           "false"
#define OTB_MQTT_CONFIG_YES             "yes"
#define OTB_MQTT_CONFIG_NO              "no"
#define OTB_MQTT_CONFIG_INVALID_        255
#define OTB_MQTT_CONFIG_FIRST_          0
#define OTB_MQTT_CONFIG_KEEP_AP_ACTIVE  "keep_ap_active"
#define OTB_MQTT_CONFIG_KEEP_AP_ACTIVE_ 0
#define OTB_MQTT_CONFIG_LOC1            "loc1"
#define OTB_MQTT_CONFIG_LOC1_           1
#define OTB_MQTT_CONFIG_LOC2            "loc2"
#define OTB_MQTT_CONFIG_LOC2_           2
#define OTB_MQTT_CONFIG_LOC3            "loc3"
#define OTB_MQTT_CONFIG_LOC3_           3
#define OTB_MQTT_CONFIG_DS18B20S        "ds18b20s"
#define OTB_MQTT_CONFIG_DS18B20S_       4
#define OTB_MQTT_CONFIG_DS18B20	        "ds18b20"
#define OTB_MQTT_CONFIG_DS18B20_        5
#define OTB_MQTT_CONFIG_LAST_           5

extern char *otb_mqtt_config_fields[];
#ifdef OTB_MQTT_C
char *otb_mqtt_config_fields[OTB_MQTT_CONFIG_LAST_ + 1] =
{
  OTB_MQTT_CONFIG_KEEP_AP_ACTIVE,
  OTB_MQTT_CONFIG_LOC1,
  OTB_MQTT_CONFIG_LOC2,
  OTB_MQTT_CONFIG_LOC3,
  OTB_MQTT_CONFIG_DS18B20S,
  OTB_MQTT_CONFIG_DS18B20
};
#endif // OTB_MQTT_C


extern void otb_mqtt_publish(MQTT_Client *mqtt_client,
                             char *subtopic,
                             char *extra_subtopic,
                             char *message,
                             char *extra_message,
                             uint8_t qos,
                             bool retain);
void otb_mqtt_handle_loc(char **loc1,
                         char **loc1_,
                         char **loc2,
                         char **loc2_,
                         char **loc3,
                         char **loc3_);
extern void otb_mqtt_subscribe(MQTT_Client *mqtt_client,
                               char *subtopic,
                               char *extra_subtopic,
                               uint8_t qos);
extern void otb_mqtt_on_connected(uint32_t *client);
extern void otb_mqtt_on_disconnected(uint32_t *client);
extern void otb_mqtt_on_published(uint32_t *client);
extern void otb_mqtt_initialize(char *host,
                                int port,
                                int security,
                                char *device_id,
                                char *mqtt_username,
                                char *mqtt_password,
                                uint16_t keepalive);
void otb_mqtt_report_error(char *cmd, char *error);
void otb_mqtt_send_status(char *val1,
                          char *val2,
                          char *val3,
                          char *val4);
extern void otb_mqtt_on_receive_publish(uint32_t *client,
                                        const char* topic,
                                        uint32_t topic_len,
                                        const char *msg,
                                        uint32_t msg_len);
void otb_mqtt_reason(char *what);
uint8 otb_mqtt_pub_get_topic(char *topic);
int otb_mqtt_get_cmd_len(char *cmd);
bool otb_mqtt_match(char *msg, char *cmd);
uint8 otb_mqtt_pub_get_command(char *msg, char *val[]);

extern char otb_mqtt_string_empty[];
extern char otb_mqtt_string_slash[];
extern char otb_mqtt_string_slash[];
extern char otb_mqtt_string_colon[];
extern char otb_mqtt_string_true[];
extern char otb_mqtt_string_false[];
extern char otb_mqtt_scratch[OTB_MQTT_MAX_MSG_LENGTH];

#ifdef OTB_MQTT_C

// Need to statically assign, rather than from stack
char otb_mqtt_string_empty[] = "";
char otb_mqtt_string_slash[] = "/";
char otb_mqtt_string_colon[] = ":";
char otb_mqtt_string_true[] = "true";
char otb_mqtt_string_false[] = "false";

MQTT_Client otb_mqtt_client;

char otb_mqtt_topic_s[OTB_MQTT_MAX_TOPIC_LENGTH];
char otb_mqtt_msg_s[OTB_MQTT_MAX_MSG_LENGTH];
char otb_mqtt_scratch[OTB_MQTT_MAX_MSG_LENGTH];

#endif // OTB_MQTT_C
#endif // OTB_MQTT_H
