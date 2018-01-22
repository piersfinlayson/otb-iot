/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2017 Piers Finlayson
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

#ifndef OTB_CMD_H_INCLUDED
#define OTB_CMD_H_INCLUDED

// otb_cmd_rsp - used to store command responses from otb_cmd
#define OTB_CMD_RSP_MAX_LEN   256
#ifndef OTB_CMD_C
extern uint16_t otb_cmd_rsp_next;
extern unsigned char otb_cmd_rsp[OTB_CMD_RSP_MAX_LEN];
#else
uint16_t otb_cmd_rsp_next;
unsigned char otb_cmd_rsp[OTB_CMD_RSP_MAX_LEN];
#endif

// Arrays used to decode incoming MQTT cmds and topics into.  As all commands are handled
// serially, only one such structure is required.  If the processing of a command is
// asynchronous, it's the responsibility of the command handler to store off any info
// about the command which kicked off the asynchronous programming.

// Note that TOPIC_LEN length includes the string NULL terminator
#define OTB_CMD_MAX_CMD_LEN  16
#define OTB_CMD_MAX_CMDS     16
unsigned char otb_cmd_incoming_cmd[OTB_CMD_MAX_CMDS][OTB_CMD_MAX_CMD_LEN];

//
// otb_cmd_match_fn function prototype
//
// To be used for matches other than simple string compares
// - IN  - topic/cmd to match
//
// Return val:
// - TRUE if match
// - FALSE otherwise
//
typedef bool otb_cmd_match_fn(unsigned char *to_match);

// Match functions
extern otb_cmd_match_fn otb_cmd_match_chipid;

// Also defined in otb_ds18b20.h
extern otb_cmd_match_fn otb_ds18b20_valid_addr;
extern otb_cmd_match_fn otb_ds18b20_configured_addr;

// Also defined in otb_i2c.h
extern otb_cmd_match_fn otb_i2c_ads_valid_addr;
extern otb_cmd_match_fn otb_i2c_ads_configured_addr;

// Also defined in otb_gpio.h
extern otb_cmd_match_fn otb_gpio_valid_pin;

// Also defined in otb_relay.h
extern otb_cmd_match_fn otb_relay_valid_id;

// Also defined in otb_i2c_pca9695.h
extern otb_cmd_match_fn otb_i2c_pca9685_valid_pin;

//
// otb_cmd_handler_fn functon protoype
// 
// To be used to handling incoming MQTT commands
//
// Arguments:
// - IN  - next_cmd - the next unmatched MQTT cmd
// - IN  - arg - argument stored in otb_cmd_control to be passed in
//
// Return val:
// - TRUE if OK
// - FALSE otherwise
//
typedef bool otb_cmd_handler_fn(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd);

// Handler functions

extern otb_cmd_handler_fn otb_cmd_get_string;
extern otb_cmd_handler_fn otb_cmd_get_boot_slot;
extern otb_cmd_handler_fn otb_cmd_get_rssi;
extern otb_cmd_handler_fn otb_cmd_get_heap_size;
extern otb_cmd_handler_fn otb_cmd_get_config_all;
extern otb_cmd_handler_fn otb_cmd_get_vdd33;
extern otb_cmd_handler_fn otb_cmd_get_logs_ram;
extern otb_cmd_handler_fn otb_cmd_get_reason_reboot;
extern otb_cmd_handler_fn otb_cmd_trigger_test_led_fn;
extern otb_cmd_handler_fn otb_cmd_set_boot_slot;
extern otb_cmd_handler_fn otb_cmd_trigger_update;
extern otb_cmd_handler_fn otb_cmd_control_get_sensor_temp_ds18b20_num;
extern otb_cmd_handler_fn otb_cmd_control_get_sensor_temp_ds18b20_addr;
// extern otb_cmd_handler_fn otb_cmd_control_get_sensor_temp_ds18b20_value;
extern otb_cmd_handler_fn otb_cmd_trigger_reset;
extern otb_cmd_handler_fn otb_cmd_trigger_ping;
extern otb_cmd_handler_fn otb_led_trigger_sf;
extern otb_cmd_handler_fn otb_cmd_get_config_all;
extern otb_cmd_handler_fn otb_cmd_get_sensor_adc_ads;
extern otb_cmd_handler_fn otb_conf_set_keep_ap_active;
extern otb_cmd_handler_fn otb_conf_set_status_led;
extern otb_cmd_handler_fn otb_conf_set_loc;
//extern otb_cmd_handler_fn otb_ds18b20_conf_set;
//extern otb_cmd_handler_fn otb_i2c_ads_conf_set;
extern otb_cmd_handler_fn otb_conf_delete_loc;
extern otb_cmd_handler_fn otb_i2c_ads_conf_delete;
extern otb_cmd_handler_fn otb_ds18b20_conf_delete;
extern otb_cmd_handler_fn otb_gpio_cmd_get;
extern otb_cmd_handler_fn otb_gpio_cmd_get_config;
extern otb_cmd_handler_fn otb_gpio_cmd_set;
extern otb_cmd_handler_fn otb_gpio_cmd_set_config;
extern otb_cmd_handler_fn otb_relay_conf_set;
extern otb_cmd_handler_fn otb_relay_mezz_trigger;
extern otb_cmd_handler_fn otb_relay_trigger;
extern otb_cmd_handler_fn otb_serial_config_handler;
extern otb_cmd_handler_fn otb_nixie_init;
extern otb_cmd_handler_fn otb_nixie_clear;
extern otb_cmd_handler_fn otb_nixie_show;
extern otb_cmd_handler_fn otb_nixie_cycle;
extern otb_cmd_handler_fn otb_nixie_power;
extern otb_cmd_handler_fn otb_i2c_pca_gpio_cmd;

#define OTB_CMD_GPIO_MIN         0
#define OTB_CMD_GPIO_GET         0
#define OTB_CMD_GPIO_GET_CONFIG  1
#define OTB_CMD_GPIO_TRIGGER     2
#define OTB_CMD_GPIO_SET_CONFIG  3
#define OTB_CMD_GPIO_NUM         4

#define OTB_CMD_GPIO_PCA_SDA    0x100
#define OTB_CMD_GPIO_PCA_SCL    0x200
#define OTB_CMD_GPIO_PCA_ADDR   0x400
#define OTB_CMD_GPIO_PCA_STORE_CONF 0x800
#define OTB_CMD_GPIO_PCA_INIT   0x1000
#define OTB_CMD_GPIO_PCA_GPIO   0x2000

#define OTB_CMD_RELAY_MIN      0
#define OTB_CMD_RELAY_LOC      0
#define OTB_CMD_RELAY_TYPE     1
#define OTB_CMD_RELAY_ADDR     2
#define OTB_CMD_RELAY_NUM      3
#define OTB_CMD_RELAY_STATUS   4
#define OTB_CMD_RELAY_PWR_ON   5
#define OTB_CMD_RELAY_TOTAL    6

// otb_cmd_control struct
// 
// Used to decide handler function for a particular MQTT command

typedef struct otb_cmd_control
{
  // The command to match
  unsigned char *match_cmd;
  
  // Function to use to test match instead of using match_cmd (if match_cmd NULL)
  otb_cmd_match_fn *match_fn;
  
  // Control structure to use to match the next sub_cmd
  struct otb_cmd_control *sub_cmd_control;
  
  // If sub_cmd_control is NULL, call this handler function instead.  Function must
  // follow otb_cmd_handler prototype
  otb_cmd_handler_fn *handler_fn;
  
  // If otb_cmd_handler is invoked, the following value is passed into argument arg
  void *arg;
  
} otb_cmd_control;

//
// Supported commands
//
// It's encouraged to put _all_ commands in here, and let the otb_cmd routines police
// the defined structure - rather than write custom code for each command use.
//
// Note all otb_cmd_control arrays must end with {OTB_CMD_FINISH} which essentially
// null terminates
//
// To add a new command, add to the comment below, and then to the appropriate point in 
// the arrays which follow - adding additional arrays if there's a new sub-structure, or
// just calling out to a new function to implement the new command
//

//
// This is the command structure:
//
// get
//   sensor
//     temp
//       ds18b20
//         num     
//         value   // Index 0 to 7  !! Unimplemented
//         addr    // Index 0 to 7 
//     adc
//       ads     // ADS1115 family
//   gpio
//     pin // Must be unreserved
//     pcf     // PCF8574  family - need to figure out how to implement - perhaps not a "GPIO"
//     mcp     // MCP23017 family - need to figure out how to implement - perhaps not a "GPIO"
//     pca     // PCA9685  family - need to figure out how to implement - perhaps not a "GPIO"
//   config
//     all
//     ??
//     gpio
//       pin  // Must be an unreserved GPIO
//         state  // Must be 0 or 1
//     serial
//       enable
//       rx|rx_pin|rxpin
//       tx|tx_pin|txpin
//       baud|baudrate|baud_rate|bit_rate|bitrate|speed
//       stopbit|stop_bit
//       parity
//   info
//     version
//     compile_date
//     compile_time
//     boot_slot
//     logs
//       flash   // Unimplemented
//       ram
//     rssi
//     heap_size
//     reason
//       reboot
//     chip_id
//     hw_info
//     vdd33
// set
//   config
//     status_led
//       <on|off|warn>
//     keep_ap_active
//       yes|true|no|false
//     loc
//       1|2|3
//         <location>
//     ds18b20
//       <addr>  // xx-yyyyyyyyyyyy format
//         <name>
//     ads
//       <addr>
//         add - used to initialize this one
//         mux
//           <value>
//         rate
//           <value>
//         gain
//           <value>
//         cont
//           <value>
//         rms
//           <value>
//         period
//           <value>
//         samples
//           <value>
//         loc
//           <value>
//     relay (external relay module)
//       <id> (1-8)
//         loc (location, 31 chars max)
//         type (otb, pca, pcf, mcp - only otb = otb-relay v0.4 and pca currently supported = pca9685)
//         addr (address, 2 hex digits if pca, pcf or mcp, 3 binary digits if otb, defaults 000 for otb, 40 for pca)
//         num (defaults to 8 for otb, 8 for pca)
//         status (invalid for otb, status led, pin num of driver connected to status led, -1 means no status led)
//         pwr_on
//           <num> // not yet implemented
//             <state> (0 or 1) // not yet implemented
//           all // not yet implemented
//             <state> (string of 0s and 1s - lowest numbered pin last) // not yet implemented
//           current (stores current state)
//     gpio
//       pin  // Must be an unreserved GPIO
//         state  // Must be 0 or 1
//       pca 
//         sda
//           pin
//         scl
//           pin
//         addr
//           <addr>
//         store_conf
//     serial
//       enable                                          // no, yes, false, true, or no value   (no = default)
//       disable                                         // no value
//       rx|rx_pin|rxpin                                 // GPIO pin to use for RX (no default, cannot be enabled until set)
//       tx|tx_pin|txpin                                 // GPIO pin to use for TX (no default, cannot be enabled until set)
//       baud|baudrate|baud_rate|bit_rate|bitrate|speed  // standard baudrate from 300 to 57600 (9600 = default)
//       stopbit|stop_bit                                // 1, one, 2, two, 0, none (0 = default)
//       parity                                          // none, even or odd      (none = default
//       commit                                          // no argument required
//   boot_slot
// delete
//   config
//     loc
//       all
//       1|2|3
//     ds18b20
//       all
//       <addr>
//     ads
//       all
//       <addr>
// trigger
//   update
//   upgrade
//   reset
//   reboot
//   ping
//   ow
//     ??
//   i2c
//     ??
//   test
//     led    
//       once   // led name    
//       go     // led name
//       stop   // led name
//   gpio
//     pin  // Must be an unreserved GPIO
//       state  // Must be 0 or 1
//     pca  
//       pin
//         state // Must be 0 or 1
//       init
//   relay
//     <id>
//       <num>
//         <state> (0 or 1)
//       all
//         <state> (string of 0s and 1s - lowest numbered pin last)
//     mezz
//       0 (only 1 relay mezz board supported)
//         <state> (0 or 1)
//   serial
//     send|transmit|tx
//     buffer
//       dump <empty>|all|<bytes>
//       clear|empty
//   nixie
//     init
//     clear
//     show
//       ._._ // .s optional, _ = _ for space, or digit
//     cycle
//     power
//       on
//       off
//   sf // snowflake - temporary
//   neo
//     off
//       [num of neos]
//     solid
//       [num of neos]
//         [colour in hex]
//     bounce
//       [num of neos]
//         [main colour in hex]
//           [bounce colour in hex]
//             [speed (in ms between movements)]
//     round
//       [num of neos]
//         [main colour in hex]
//           [bounce colour in hex]
//             [speed (in ms between movements)]
//     rainbow
//       [num of neos]
//         [start colour in hex]
//           [end colour in hex]
//     bouncer // rainow bounce
//       [num of neos]
//         [main colour in hex]
//           [bounce colour in hex]
//             [speed (in ms between movements)]
//               [end colour in hex]
//     rounder // rainow bounce
//       [num of neos]
//         [main colour in hex]
//           [bounce colour in hex]
//             [speed (in ms between movements)]
//               [end colour in hex]
//     rotate
//       [num of neos]
//         [start colour in hex]
//           [end colour in hex]
//             [speed (in ms between movements)]
//     rotateb
//       [num of neos]
//         [start colour in hex]
//           [end colour in hex]
//             [speed (in ms between movements)]
//     message
//       [size of display - only 8x8 supported]
//         [background colour in hex]
//           [message colour in hex]
//             [speed (in ms between movements)]
//               message
//  

// Some macros to simplify command structure definition
#define OTB_CMD_NO_FN   NULL, NULL
#define OTB_CMD_FINISH  NULL, NULL, NULL, NULL, NULL

extern otb_cmd_control otb_cmd_control_topic_top[];
extern otb_cmd_control otb_cmd_control_topic_2nd[];
extern otb_cmd_control otb_cmd_control_cmd_top_level[];
extern otb_cmd_control otb_cmd_control_get[];
extern otb_cmd_control otb_cmd_control_get_sensor[];
extern otb_cmd_control otb_cmd_control_get_sensor_temp[];
extern otb_cmd_control otb_cmd_control_get_sensor_temp_ds18b20[];
extern otb_cmd_control otb_cmd_control_get_sensor_adc[];
extern otb_cmd_control otb_cmd_control_get_sensor_adc_ads[];
// extern otb_cmd_control otb_cmd_control_get_sensor_gpio[];
// extern otb_cmd_control otb_cmd_control_get_gpio_native[];
// extern otb_cmd_control otb_cmd_control_get_gpio_pcf[];
// extern otb_cmd_control otb_cmd_control_get_gpio_mcp[];
// extern otb_cmd_control otb_cmd_control_get_gpio_pca[];
extern otb_cmd_control otb_cmd_control_get_config[];
extern otb_cmd_control otb_cmd_control_get_info[];
extern otb_cmd_control otb_cmd_control_get_reason[];
extern otb_cmd_control otb_cmd_control_get_info_logs[];
extern otb_cmd_control otb_cmd_control_set[];
extern otb_cmd_control otb_cmd_control_set_config[];
extern otb_cmd_control otb_cmd_control_set_config_status_led[];
extern otb_cmd_control otb_cmd_control_set_config_keep_ap_active[];
extern otb_cmd_control otb_cmd_control_set_config_loc[];
extern otb_cmd_control otb_cmd_control_set_config_ds18b20[];
extern otb_cmd_control otb_cmd_control_set_config_ads[];
extern otb_cmd_control otb_cmd_control_set_config_ads_valid[];
extern otb_cmd_control otb_cmd_control_delete[];
extern otb_cmd_control otb_cmd_control_delete_config[];
extern otb_cmd_control otb_cmd_control_delete_config_loc[];
extern otb_cmd_control otb_cmd_control_delete_config_ds18b20[];
extern otb_cmd_control otb_cmd_control_delete_config_ads[];
extern otb_cmd_control otb_cmd_control_trigger[];
extern otb_cmd_control otb_cmd_control_trigger_ow[];
extern otb_cmd_control otb_cmd_control_trigger_i2c[];
extern otb_cmd_control otb_cmd_control_trigger_test[];
extern otb_cmd_control otb_cmd_control_trigger_test_led[];
extern otb_cmd_control otb_cmd_control_get_gpio[];
extern otb_cmd_control otb_cmd_control_get_config_gpio[];
extern otb_cmd_control otb_cmd_control_set_config_gpio[];
extern otb_cmd_control otb_cmd_control_set_config_gpio_pca[];
extern otb_cmd_control otb_cmd_control_set_config_gpio_pca_sda[];
extern otb_cmd_control otb_cmd_control_set_config_gpio_pca_scl[];
extern otb_cmd_control otb_cmd_control_trigger_gpio[];
extern otb_cmd_control otb_cmd_control_trigger_gpio_pca[];
extern otb_cmd_control otb_cmd_control_set_config_relay[];
extern otb_cmd_control otb_cmd_control_set_config_relay_valid[];
extern otb_cmd_control otb_cmd_control_trigger_relay[];
extern otb_cmd_control otb_cmd_control_get_config_serial[];
extern otb_cmd_control otb_cmd_control_set_config_serial[];
extern otb_cmd_control otb_cmd_control_trigger_serial[];
extern otb_cmd_control otb_cmd_control_trigger_nixie[];
extern otb_cmd_control otb_cmd_control_trigger_nixie_power[];
extern otb_cmd_control otb_cmd_control_trigger_neo[];

#ifdef OTB_CMD_C

otb_cmd_control otb_cmd_control_topic_top[] = 
{
  {OTB_MQTT_OTBIOT_TOPIC, NULL, otb_cmd_control_topic_2nd,      OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}
};

// Note deliberately not checking any sub-topics beyond chipid until the bottom.  Note
// this won't work where location is set and used within the topic
// XX fixup
otb_cmd_control otb_cmd_control_topic_2nd[] = 
{
  {NULL, otb_cmd_match_chipid, otb_cmd_control_cmd_top_level,  OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}
};

// First level of OTB-IOT commands
otb_cmd_control otb_cmd_control_cmd_top_level[] =
{
  {"get",              NULL, otb_cmd_control_get,            OTB_CMD_NO_FN},
  {"set",              NULL, otb_cmd_control_set,            OTB_CMD_NO_FN},
  {"delete",           NULL, otb_cmd_control_delete,         OTB_CMD_NO_FN},
  {"trigger",          NULL, otb_cmd_control_trigger,        OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}
};

// get commands
otb_cmd_control otb_cmd_control_get[] = 
{
  {"sensor",           NULL, otb_cmd_control_get_sensor,     OTB_CMD_NO_FN},
  {"config",           NULL, otb_cmd_control_get_config,     OTB_CMD_NO_FN},
  {"info",             NULL, otb_cmd_control_get_info,       OTB_CMD_NO_FN},
  {"gpio",             otb_gpio_valid_pin, NULL, otb_gpio_cmd, (void *)OTB_CMD_GPIO_GET_CONFIG},
  {OTB_CMD_FINISH}    
};

// get->sensor commands
otb_cmd_control otb_cmd_control_get_sensor[] = 
{
  {"temp",             NULL, otb_cmd_control_get_sensor_temp,       OTB_CMD_NO_FN},
  {"adc",              NULL, otb_cmd_control_get_sensor_adc,        OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}    
};

// get->sensor->temp commands
otb_cmd_control otb_cmd_control_get_sensor_temp[] = 
{
  {"ds18b20",          NULL, otb_cmd_control_get_sensor_temp_ds18b20, OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}    
};

// get->sensor->temp->ds18b20 commands
otb_cmd_control otb_cmd_control_get_sensor_temp_ds18b20[] =
{
  {"num",              NULL, NULL,      otb_cmd_control_get_sensor_temp_ds18b20_num, NULL},
//  {"value",            NULL, NULL,      otb_cmd_control_get_sensor_temp_ds18b20_value, NULL},
  {"addr",             NULL, NULL,      otb_cmd_control_get_sensor_temp_ds18b20_addr, NULL},
  {OTB_CMD_FINISH}
};

// get->sensor->adc commands
otb_cmd_control otb_cmd_control_get_sensor_adc[] = 
{
  {"ads",              NULL, otb_cmd_control_get_sensor_adc_ads,    OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}    
};

// get->sensor->adc->ads commands
otb_cmd_control otb_cmd_control_get_sensor_adc_ads[] = 
{
  {"ads",              NULL, NULL,      otb_cmd_get_sensor_adc_ads, NULL},
  {OTB_CMD_FINISH}    
};

// get->sensor->gpio commands
otb_cmd_control otb_cmd_control_get_gpio[] = 
{
//  {"native",           NULL, otb_cmd_control_get_gpio_native,  OTB_CMD_NO_FN},
//  {"pcf",              NULL, otb_cmd_control_get_gpio_pcf,     OTB_CMD_NO_FN},
//  {"mcp",              NULL, otb_cmd_control_get__mcp,     OTB_CMD_NO_FN},
//  {"pca",              NULL, otb_cmd_control_get_gpio_pca,     OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}    
};

#if 0
// get->sensor->gpio->pcf commands
otb_cmd_control otb_cmd_control_get_gpio_pcf[] =
{
  // XXX TBC
  {OTB_CMD_FINISH}    
};

// get->sensor->gpio->mcp commands
otb_cmd_control otb_cmd_control_get_gpio_mcp[] =
{
  // XXX TBC
  {OTB_CMD_FINISH}    
};

// get->sensor->gpio->pca commands
otb_cmd_control otb_cmd_control_get_gpio_pca[] =
{
  // XXX TBC
  {OTB_CMD_FINISH}    
};
#endif

// get->config commands
otb_cmd_control otb_cmd_control_get_config[] =
{
  {"all",              NULL, NULL,      otb_cmd_get_config_all,    NULL},
  {"gpio",             NULL, otb_cmd_control_get_config_gpio,    OTB_CMD_NO_FN},
  {"serial",           NULL, otb_cmd_control_get_config_serial,    OTB_CMD_NO_FN},
  // XXX TBC
  {OTB_CMD_FINISH}    
};

// get->config->gpio
otb_cmd_control otb_cmd_control_get_config_gpio[] =
{
  {NULL, otb_gpio_valid_pin, NULL, otb_gpio_cmd, (void *)OTB_CMD_GPIO_GET_CONFIG},
  {OTB_CMD_FINISH}
};

// get->config->Serial
otb_cmd_control otb_cmd_control_get_config_serial[] =
{
  {"enable",    NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_ENABLE  | OTB_SERIAL_CMD_GET)},
  {"rx",        NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_RX      | OTB_SERIAL_CMD_GET)},
  {"rx_pin",    NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_RX      | OTB_SERIAL_CMD_GET)},
  {"rxpin",     NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_RX      | OTB_SERIAL_CMD_GET)},
  {"tx",        NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_TX      | OTB_SERIAL_CMD_GET)},
  {"tx_pin",    NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_TX      | OTB_SERIAL_CMD_GET)},
  {"txpin",     NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_TX      | OTB_SERIAL_CMD_GET)},
  {"baud",      NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_GET)},
  {"baudrate",  NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_GET)},
  {"baud_rate", NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_GET)},
  {"bit_rate",  NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_GET)},
  {"bitrate",   NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_GET)},
  {"speed",     NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_GET)},
  {"stopbit",   NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_STOPBIT | OTB_SERIAL_CMD_GET)},
  {"stop_bit",  NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_STOPBIT | OTB_SERIAL_CMD_GET)},
  {"parity",    NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_PARITY  | OTB_SERIAL_CMD_GET)},
  {OTB_CMD_FINISH}
};

// get->info commands
otb_cmd_control otb_cmd_control_get_info[] =
{
  {"version",           NULL, NULL,     otb_cmd_get_string,        otb_version_id},
  {"compile_date",      NULL, NULL,     otb_cmd_get_string,        otb_compile_date},
  {"compile_time",      NULL, NULL,     otb_cmd_get_string,        otb_compile_time},
  {"boot_slot",         NULL, NULL,     otb_cmd_get_boot_slot,     NULL},
  {"logs",              NULL, otb_cmd_control_get_info_logs,     OTB_CMD_NO_FN},
  {"rssi",              NULL, NULL,     otb_cmd_get_rssi,          NULL},
  {"heap_size",         NULL, NULL,     otb_cmd_get_heap_size,     NULL},
  {"reason",            NULL, otb_cmd_control_get_reason,         OTB_CMD_NO_FN},
  {"chip_id",           NULL, NULL,     otb_cmd_get_string,        OTB_MAIN_CHIPID},
  {"hw_info",           NULL, NULL,     otb_cmd_get_string,        otb_hw_info},
  {"vdd33",             NULL, NULL,     otb_cmd_get_vdd33,         NULL},
  {OTB_CMD_FINISH}    
};

// get->info->reason commands
otb_cmd_control otb_cmd_control_get_reason[] =
{
  {"reason",            NULL, NULL,     otb_cmd_get_reason_reboot, NULL},
  {OTB_CMD_FINISH}    
};

// get->info->logs commands
otb_cmd_control otb_cmd_control_get_info_logs[] =
{
  {"ram",               NULL, NULL,     otb_cmd_get_logs_ram,      NULL},
  {OTB_CMD_FINISH}    
};

// set commands
otb_cmd_control otb_cmd_control_set[] =
{
  {"config",           NULL, otb_cmd_control_set_config,        OTB_CMD_NO_FN},
  {"boot_slot",        NULL, NULL,      otb_cmd_set_boot_slot,     NULL},
  {OTB_CMD_FINISH}    
};

// set->config commands
otb_cmd_control otb_cmd_control_set_config[] =
{
  {"status_led",       NULL, otb_cmd_control_set_config_status_led,  OTB_CMD_NO_FN},
  {"keep_ap_active",   NULL, otb_cmd_control_set_config_keep_ap_active,  OTB_CMD_NO_FN},
  {"loc",              NULL, otb_cmd_control_set_config_loc,             OTB_CMD_NO_FN},
  {"ds18b20",          NULL, otb_cmd_control_set_config_ds18b20,         OTB_CMD_NO_FN},
  {"ads",              NULL, otb_cmd_control_set_config_ads,             OTB_CMD_NO_FN},
  {"gpio",             NULL, otb_cmd_control_set_config_gpio,            OTB_CMD_NO_FN},
  {"relay",            NULL, otb_cmd_control_set_config_relay,           OTB_CMD_NO_FN},
  {"serial",           NULL, otb_cmd_control_set_config_serial,          OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}    
};

// set->config->status_led commands
otb_cmd_control otb_cmd_control_set_config_status_led[] =
{
  {"on",                NULL, NULL, otb_conf_set_status_led, (void *)OTB_CONF_STATUS_LED_BEHAVIOUR_NORMAL},
  {"normal",            NULL, NULL, otb_conf_set_status_led, (void *)OTB_CONF_STATUS_LED_BEHAVIOUR_NORMAL},
  {"off",               NULL, NULL, otb_conf_set_status_led, (void *)OTB_CONF_STATUS_LED_BEHAVIOUR_OFF},
  {"warn",              NULL, NULL, otb_conf_set_status_led, (void *)OTB_CONF_STATUS_LED_BEHAVIOUR_WARN},
  {OTB_CMD_FINISH}    
};
// set->config->keep_ap_active commands
otb_cmd_control otb_cmd_control_set_config_keep_ap_active[] =
{
  {"yes",               NULL, NULL, otb_conf_set_keep_ap_active, (void *)TRUE},
  {"true",              NULL, NULL, otb_conf_set_keep_ap_active, (void *)TRUE},
  {"no",                NULL, NULL, otb_conf_set_keep_ap_active, (void *)FALSE},
  {"false",             NULL, NULL, otb_conf_set_keep_ap_active, (void *)FALSE},
  {OTB_CMD_FINISH}    
};

// set->config->loc commands
otb_cmd_control otb_cmd_control_set_config_loc[] =
{
  {"1",                 NULL, NULL, otb_conf_set_loc, (void *)1},
  {"2",                 NULL, NULL, otb_conf_set_loc, (void *)2},
  {"3",                 NULL, NULL, otb_conf_set_loc, (void *)3},
  {OTB_CMD_FINISH}
};

// set->config->ds18b20
otb_cmd_control otb_cmd_control_set_config_ds18b20[] =
{
  {NULL, otb_ds18b20_valid_addr, NULL, otb_ds18b20_conf_set, NULL}, 
  {OTB_CMD_FINISH}    
};

// set->config->ads
otb_cmd_control otb_cmd_control_set_config_ads[] =
{
  {NULL, otb_i2c_ads_valid_addr, otb_cmd_control_set_config_ads_valid, OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}
};

// set->config->ads-><addr>
otb_cmd_control otb_cmd_control_set_config_ads_valid[] = 
{
  {"add",      NULL, NULL, otb_i2c_ads_conf_set, (void *)OTB_CMD_ADS_ADD}, 
  {"mux",      NULL, NULL, otb_i2c_ads_conf_set, (void *)OTB_CMD_ADS_MUX}, 
  {"rate",     NULL, NULL, otb_i2c_ads_conf_set, (void *)OTB_CMD_ADS_RATE}, 
  {"gain",     NULL, NULL, otb_i2c_ads_conf_set, (void *)OTB_CMD_ADS_GAIN}, 
  {"cont",     NULL, NULL, otb_i2c_ads_conf_set, (void *)OTB_CMD_ADS_CONT}, 
  {"rms",      NULL, NULL, otb_i2c_ads_conf_set, (void *)OTB_CMD_ADS_RMS}, 
  {"period",   NULL, NULL, otb_i2c_ads_conf_set, (void *)OTB_CMD_ADS_PERIOD}, 
  {"samples",  NULL, NULL, otb_i2c_ads_conf_set, (void *)OTB_CMD_ADS_SAMPLES}, 
  {"loc",      NULL, NULL, otb_i2c_ads_conf_set, (void *)OTB_CMD_ADS_LOC}, 
  {OTB_CMD_FINISH}
};

// set->config->gpio
otb_cmd_control otb_cmd_control_set_config_gpio[] =
{
  {"pca", NULL, otb_cmd_control_set_config_gpio_pca, OTB_CMD_NO_FN},
  {NULL, otb_gpio_valid_pin, NULL, otb_gpio_cmd, (void *)OTB_CMD_GPIO_SET_CONFIG},
  {OTB_CMD_FINISH}
};

// set->config->gpio->pca
otb_cmd_control otb_cmd_control_set_config_gpio_pca[] =
{
  {"sda",   NULL, otb_cmd_control_set_config_gpio_pca_sda, OTB_CMD_NO_FN},
  {"scl",   NULL, otb_cmd_control_set_config_gpio_pca_scl, OTB_CMD_NO_FN},
  {"addr",  NULL, NULL, otb_i2c_pca_gpio_cmd, (void *)(OTB_CMD_GPIO_SET_CONFIG|OTB_CMD_GPIO_PCA_ADDR)},
  {"store_conf", NULL, NULL, otb_i2c_pca_gpio_cmd, (void *)(OTB_CMD_GPIO_SET_CONFIG|OTB_CMD_GPIO_PCA_STORE_CONF)},
  {OTB_CMD_FINISH}
};

// set->config->gpio->pca->sda
otb_cmd_control otb_cmd_control_set_config_gpio_pca_sda[] =
{
  {NULL,   otb_gpio_valid_pin, NULL, otb_i2c_pca_gpio_cmd, (void *)(OTB_CMD_GPIO_SET_CONFIG|OTB_CMD_GPIO_PCA_SDA)},
  {OTB_CMD_FINISH}
};

// set->config->gpio->pca->scl
otb_cmd_control otb_cmd_control_set_config_gpio_pca_scl[] =
{
  {NULL,   otb_gpio_valid_pin, NULL, otb_i2c_pca_gpio_cmd, (void *)(OTB_CMD_GPIO_SET_CONFIG|OTB_CMD_GPIO_PCA_SCL)},
  {OTB_CMD_FINISH}
};

// set->config->relay
//     relay (external relay module)
//       <id> (1-8)
//         loc (location, 31 chars max)
//         type (otb, pca, pcf, mcp - only otb = otb-relay v0.4 and pca currently supported = pca9685)
//         addr (address, 2 hex digits if pca, pcf or mcp, 3 binary digits if otb, defaults 000 for otb, 40 for pca)
//         num (defaults to 8 for otb, 8 for pca)
//         status (invalid for otb, status led, pin num of driver connected to status led, -1 means no status led)
//         pwr_on
//           <num>
//             <state> (0 or 1)
//           all
//             <state> (string of 0s and 1s - lowest numbered pin last)
otb_cmd_control otb_cmd_control_set_config_relay[] = 
{
  {NULL, otb_relay_valid_id, otb_cmd_control_set_config_relay_valid, OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}
};

otb_cmd_control otb_cmd_control_set_config_relay_valid[] =
{
  {"loc",          NULL, NULL, otb_relay_conf_set, (void *)OTB_CMD_RELAY_LOC},
  {"type",         NULL, NULL, otb_relay_conf_set, (void *)OTB_CMD_RELAY_TYPE},
  {"addr",         NULL, NULL, otb_relay_conf_set, (void *)OTB_CMD_RELAY_ADDR},
  {"num",          NULL, NULL, otb_relay_conf_set, (void *)OTB_CMD_RELAY_NUM},
  {"status",       NULL, NULL, otb_relay_conf_set, (void *)OTB_CMD_RELAY_STATUS},
  {"pwr_on",       NULL, NULL, otb_relay_conf_set, (void *)OTB_CMD_RELAY_PWR_ON},
  {OTB_CMD_FINISH}
};

// get->config->Serial
otb_cmd_control otb_cmd_control_set_config_serial[] =
{
  {"enable",    NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_ENABLE  | OTB_SERIAL_CMD_SET)},
  {"disable",   NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_DISABLE | OTB_SERIAL_CMD_SET)},
  {"rx",        NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_RX      | OTB_SERIAL_CMD_SET)},
  {"rx_pin",    NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_RX      | OTB_SERIAL_CMD_SET)},
  {"rxpin",     NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_RX      | OTB_SERIAL_CMD_SET)},
  {"tx",        NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_TX      | OTB_SERIAL_CMD_SET)},
  {"tx_pin",    NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_TX      | OTB_SERIAL_CMD_SET)},
  {"txpin",     NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_TX      | OTB_SERIAL_CMD_SET)},
  {"baud",      NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_SET)},
  {"baudrate",  NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_SET)},
  {"baud_rate", NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_SET)},
  {"bit_rate",  NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_SET)},
  {"bitrate",   NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_SET)},
  {"speed",     NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BAUD    | OTB_SERIAL_CMD_SET)},
  {"stopbit",   NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_STOPBIT | OTB_SERIAL_CMD_SET)},
  {"stop_bit",  NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_STOPBIT | OTB_SERIAL_CMD_SET)},
  {"parity",    NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_PARITY  | OTB_SERIAL_CMD_SET)},
  {"commit",    NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_COMMIT  | OTB_SERIAL_CMD_SET)},
  {OTB_CMD_FINISH}
};

// delete commands
otb_cmd_control otb_cmd_control_delete[] = 
{
  {"config",            NULL, otb_cmd_control_delete_config,   OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}    
};

// delete->config commands
otb_cmd_control otb_cmd_control_delete_config[] = 
{
  {"loc",               NULL, otb_cmd_control_delete_config_loc,     OTB_CMD_NO_FN},
  {"ds18b20",           NULL, otb_cmd_control_delete_config_ds18b20, OTB_CMD_NO_FN},
  {"ads",               NULL, otb_cmd_control_delete_config_ads,     OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}    
};

// delete->config->loc commands
otb_cmd_control otb_cmd_control_delete_config_loc[] = 
{
  {"all",               NULL, NULL, otb_conf_delete_loc, (void *)NULL},
  {"1",                 NULL, NULL, otb_conf_delete_loc, (void *)1},
  {"2",                 NULL, NULL, otb_conf_delete_loc, (void *)2},
  {"3",                 NULL, NULL, otb_conf_delete_loc, (void *)3},
  {OTB_CMD_FINISH}    
};

// delete->config->ds18b20 commands
#endif // OTB_CMD_C
#define OTB_CMD_DS18B20_ALL   0
#define OTB_CMD_DS18B20_ADDR  1
#ifdef OTB_CMD_C
otb_cmd_control otb_cmd_control_delete_config_ds18b20[] = 
{

  {"all",               NULL, NULL, otb_ds18b20_conf_delete, (void *)OTB_CMD_DS18B20_ALL},
  {NULL, otb_ds18b20_configured_addr,  NULL, otb_ds18b20_conf_delete, (void *)OTB_CMD_DS18B20_ADDR},
  {OTB_CMD_FINISH}    
};

// delete->config->ads commands
#endif // OTB_CMD_C
#define OTB_CMD_ADS_ALL   0
#define OTB_CMD_ADS_ADDR  1
#ifdef OTB_CMD_C
otb_cmd_control otb_cmd_control_delete_config_ads[] = 
{

  {"all",               NULL, NULL,           otb_i2c_ads_conf_delete, (void *)OTB_CMD_ADS_ALL},
  {NULL, otb_i2c_ads_configured_addr,  NULL,  otb_i2c_ads_conf_delete, (void *)OTB_CMD_ADS_ADDR},
  {OTB_CMD_FINISH}    
};

// trigger commands
otb_cmd_control otb_cmd_control_trigger[] =
{
  {"update",            NULL, NULL,     otb_cmd_trigger_update,    NULL},
  {"upgrade",           NULL, NULL,     otb_cmd_trigger_update,    NULL},
  {"reset",             NULL, NULL,     otb_cmd_trigger_reset,     NULL},
  {"reboot",            NULL, NULL,     otb_cmd_trigger_reset,     NULL},
  {"ping",              NULL, NULL,     otb_cmd_trigger_ping,      NULL},
  {"ow",                NULL, otb_cmd_control_trigger_ow,      OTB_CMD_NO_FN},
  {"i2c",               NULL, otb_cmd_control_trigger_i2c,     OTB_CMD_NO_FN},
  {"test",              NULL, otb_cmd_control_trigger_test,    OTB_CMD_NO_FN},
  {"gpio",              NULL, otb_cmd_control_trigger_gpio,    OTB_CMD_NO_FN},
  {"relay",             NULL, otb_cmd_control_trigger_relay,   OTB_CMD_NO_FN},
  {"serial",            NULL, otb_cmd_control_trigger_serial,  OTB_CMD_NO_FN},
  {"nixie",             NULL, otb_cmd_control_trigger_nixie,   OTB_CMD_NO_FN},
  {"sf",                NULL, NULL,     otb_led_trigger_sf,      NULL},
  {"neo",               NULL, otb_cmd_control_trigger_neo,     OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}    
};

// trigger->ow commands
otb_cmd_control otb_cmd_control_trigger_ow[] =
{
  // XXX TBC
  {OTB_CMD_FINISH}    
};

// trigger->i2c commands
otb_cmd_control otb_cmd_control_trigger_i2c[] =
{
  // XXX TBC
  {OTB_CMD_FINISH}    
};

// trigger->test commands
otb_cmd_control otb_cmd_control_trigger_test[] =
{
  {"led",               NULL, otb_cmd_control_trigger_test_led, OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}
};

// trigger->test->led commands
otb_cmd_control otb_cmd_control_trigger_test_led[] =
{
#define OTB_CMD_TRIGGER_TEST_LED_ONCE   0 
#define OTB_CMD_TRIGGER_TEST_LED_GO     1
#define OTB_CMD_TRIGGER_TEST_LED_STOP   2
#define OTB_CMD_TRIGGER_TEST_LED_TYPES  3
  {"once",              NULL, NULL,     otb_cmd_trigger_test_led_fn,  (void *)OTB_CMD_TRIGGER_TEST_LED_ONCE},
  {"go",                NULL, NULL,     otb_cmd_trigger_test_led_fn,  (void *)OTB_CMD_TRIGGER_TEST_LED_GO},
  {"stop",              NULL, NULL,     otb_cmd_trigger_test_led_fn,  (void *)OTB_CMD_TRIGGER_TEST_LED_STOP},
  {OTB_CMD_FINISH}
};

// trigger->gpio
otb_cmd_control otb_cmd_control_trigger_gpio[] =
{
  {"pca",    NULL, otb_cmd_control_trigger_gpio_pca, OTB_CMD_NO_FN},
  {NULL, otb_gpio_valid_pin, NULL, otb_gpio_cmd, (void *)OTB_CMD_GPIO_TRIGGER},
  {OTB_CMD_FINISH}
};

// trigger->gpio->pca
otb_cmd_control otb_cmd_control_trigger_gpio_pca[] =
{
  {"init", NULL, NULL, otb_i2c_pca_gpio_cmd, (void *)(OTB_CMD_GPIO_TRIGGER|OTB_CMD_GPIO_PCA_INIT)},
  {NULL, otb_i2c_pca9685_valid_pin, NULL, otb_i2c_pca_gpio_cmd, (void *)(OTB_CMD_GPIO_TRIGGER|OTB_CMD_GPIO_PCA_GPIO)},
  {OTB_CMD_FINISH}
};

// trigger->relay
otb_cmd_control otb_cmd_control_trigger_relay[] =
{
  {"mezz", NULL, NULL, otb_relay_mezz_trigger, NULL},
  {NULL, otb_relay_valid_id, NULL, otb_relay_trigger, NULL},
  {OTB_CMD_FINISH}
};

// trigger->serial
otb_cmd_control otb_cmd_control_trigger_serial[] =
{
  {"send",      NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_TRANSMIT | OTB_SERIAL_CMD_TRIGGER)},
  {"trasmit",   NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_TRANSMIT | OTB_SERIAL_CMD_TRIGGER)},
  {"tx",        NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_TRANSMIT | OTB_SERIAL_CMD_TRIGGER)},
  {"buffer",    NULL, NULL, otb_serial_config_handler, (void *)(OTB_SERIAL_CMD_BUFFER   | OTB_SERIAL_CMD_TRIGGER)},
  {OTB_CMD_FINISH}
};

// trigger->nixie commands
otb_cmd_control otb_cmd_control_trigger_nixie[] =
{
  {"init",             NULL, NULL, otb_nixie_init,   NULL},
  {"clear",            NULL, NULL, otb_nixie_clear,   NULL},
  {"show",             NULL, NULL, otb_nixie_show,   NULL},
  {"cycle",            NULL, NULL, otb_nixie_cycle,   NULL},
  {"power",            NULL, otb_cmd_control_trigger_nixie_power,   OTB_CMD_NO_FN},
  {OTB_CMD_FINISH}    
};

// trigger->nixie->power commands
otb_cmd_control otb_cmd_control_trigger_nixie_power[] =
{
  {"on",             NULL, NULL, otb_nixie_power,   (void *)1},
  {"off",            NULL, NULL, otb_nixie_power,   (void *)0},
  {OTB_CMD_FINISH}    
};

// trigger->neo commands
otb_cmd_control otb_cmd_control_trigger_neo[] =
{
  {"off",              NULL, NULL, otb_led_trigger_neo,   (void *)OTB_CMD_LED_NEO_OFF},
  {"solid",            NULL, NULL, otb_led_trigger_neo,   (void *)OTB_CMD_LED_NEO_SOLID},
  {"bounce",           NULL, NULL, otb_led_trigger_neo,   (void *)OTB_CMD_LED_NEO_BOUNCE},
  {"round",            NULL, NULL, otb_led_trigger_neo,   (void *)OTB_CMD_LED_NEO_ROUND},
  {"rainbow",          NULL, NULL, otb_led_trigger_neo,   (void *)OTB_CMD_LED_NEO_RAINBOW},
  {"bouncer",          NULL, NULL, otb_led_trigger_neo,   (void *)OTB_CMD_LED_NEO_BOUNCER},
  {"rounder",          NULL, NULL, otb_led_trigger_neo,   (void *)OTB_CMD_LED_NEO_ROUNDER},
  {"rotate",           NULL, NULL, otb_led_trigger_neo,   (void *)OTB_CMD_LED_NEO_ROTATE},
  {"rotateb",          NULL, NULL, otb_led_trigger_neo,   (void *)OTB_CMD_LED_NEO_ROTATEB},
  {"message",          NULL, NULL, otb_led_trigger_neo,   (void *)OTB_CMD_LED_NEO_MESSAGE},
  {OTB_CMD_FINISH}    
};

#endif // OTB_CMD_C

// Generic functions
void otb_cmd_mqtt_receive(uint32_t *client,
                          const char* topic,
                          uint32_t topic_len,
                          const char *msg,
                          uint32_t msg_len);
void otb_cmd_rsp_clear(void);
void otb_cmd_rsp_append(char *format, ...);
unsigned char *otb_cmd_rsp_get(void);
bool otb_cmd_populate_all(const char *topic,
                          uint32_t topic_len,
                          const char *msg,
                          uint32_t msg_len);
bool otb_cmd_populate_one(unsigned char store[][OTB_CMD_MAX_CMD_LEN],
                          uint8_t store_num,
                          uint8_t store_str_len,
                          const char *input_str,
                          uint32_t input_str_len,
                          uint8_t *written);
unsigned char *otb_cmd_get_next_cmd(unsigned char *cmd);

// Match functions
bool otb_cmd_match_chipid(unsigned char *to_match);

#endif // OTB_CMD_H_INCLUDED
