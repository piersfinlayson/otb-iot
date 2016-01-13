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

typedef uint8_t DeviceAddress[8];
typedef char DeviceAddressString[MAX_ONEWIRE_ADDRESS_STRING_LENGTH];
#ifndef TEMP_MQTT_CPP
void initialize_temp(int bus, void *oneWire_handle, void *sensors_handle);
void request_temps(void *sensors_handle);
uint8_t get_device_count(void *sensors_handle);
bool get_device_address(void *sensors_handle, uint8_t index, DeviceAddressString *addressString);
float get_temp(void *sensors_handle, int index);
int16_t get_temp_raw(void *sensors_handle, int index);
void get_temp_string(void *sensors_handle, int index, char *temp);
#endif
