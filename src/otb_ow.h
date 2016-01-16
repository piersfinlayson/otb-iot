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

extern void otb_ow_initialize(uint8_t bus);
extern void otb_ow_ds18b20_callback(os_event_t event);

#ifdef OTB_OW_C
// OW Specific module globals
void *otb_ow_onewire_handle;
void *otb_ow_sensors_handle;
uint8_t otb_ow_ds18b20_reads = 2;
uint8_t otb_ow_mqtt_disconnected_counter = 0;
OTB_TEMP_DEVICE_ADDRESS_STRING otb_ow_ds18b20_addresses[OTB_OW_MAX_DS18B20S];
uint8_t otb_ow_ds18b20_count = 0;
char otb_ow_last_temp_s[OTB_OW_MAX_DS18B20S][OTB_OW_MAX_TEMP_LEN];
#endif
