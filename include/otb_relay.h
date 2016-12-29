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

#ifndef OTB_RLEAY_H_INCLUDED
#define OTB_RELAY_H_INCLUDED

typedef struct otb_relay
{
  sint8_t index; // Index into otb_conf->relay[x]
  
  // Whether connected to this module
  bool connected; 
  
  // Timer used to connect to relay module, and then apply any updates made via MQTT - 
  // these are only done periodically to avoid too many operations
  os_timer_t timer;
  
  // Expected state of the relays on this module
  sint8_t known_state[16];

} otb_relay;
 
#ifdef OTB_RELAY_C
uint8_t otb_relay_num;
otb_relay otb_relay_status[OTB_CONF_RELAY_MAX_MODULES];
#endif // OTB_RELAY_C

#define OTB_RELAY_STATUS_LED_OTB_0_4  15

bool otb_relay_valid_id(unsigned char *to_match);  
int8_t otb_relay_get_index(unsigned char *text);
bool otb_relay_check_index(int8_t index); 
otb_relay *otb_relay_find_status(uint8_t id);
bool otb_relay_trigger(unsigned char *next_cmd,
                       void *arg,
                       unsigned char *prev_cmd);
bool otb_relay_trigger_relay(otb_relay *relay_status, uint8_t num, uint8_t state);
bool otb_relay_conf_set(unsigned char *next_cmd,
                        void *arg,
                        unsigned char *prev_cmd);
bool otb_relay_configured(void);
void otb_relay_init(void);
void otb_relay_on_timer(void *arg);

#endif // OTB_RELAY_H_INCLUDED