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

typedef uint8_t OTB_TEMP_DEVICE_ADDRESS[OTB_OW_MAX_DS18B20S];
typedef char OTB_TEMP_DEVICE_ADDRESS_STRING[OTB_OW_MAX_ADDRESS_STRING_LENGTH];

#ifdef OTB_TEMP_CPP
extern "C" void otb_temp_initialize(int bus, void *oneWire_handle, void *sensors_handle);
extern "C" void otb_temp_request_temps(void *sensors_handle);
extern "C" uint8_t otb_temp_get_device_count(void *sensors_handle);
extern "C" bool otb_temp_get_device_address(void *sensors_handle,
                                            uint8_t index,
                                            OTB_TEMP_DEVICE_ADDRESS_STRING *addressString);
extern "C" float otb_temp_get_temp_float(void *sensors_handle, int index);
extern "C" uint16_t otb_temp_get_temp_raw(void *sensors_handle, int index);
extern "C" void otb_temp_get_temp_string(void *sensors_handle, int index, char *temp);
#else
extern void otb_temp_initialize(int bus, void *oneWire_handle, void *sensors_handle);
extern void otb_temp_request_temps(void *sensors_handle);
extern uint8_t otb_temp_get_device_count(void *sensors_handle);
extern bool otb_temp_get_device_address(void *sensors_handle,
                                        uint8_t index,
                                        OTB_TEMP_DEVICE_ADDRESS_STRING *addressString);
extern float otb_temp_get_temp_float(void *sensors_handle, int index);
extern int16_t otb_temp_get_temp_raw(void *sensors_handle, int index);
extern void otb_temp_get_temp_string(void *sensors_handle, int index, char *temp);
#endif
