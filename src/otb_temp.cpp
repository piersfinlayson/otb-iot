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

#define OTB_TEMP_CPP
#include "OneWire.h"
#include "DallasTemperature.h"
#include "otb_cpp.h"

extern "C" void ICACHE_FLASH_ATTR otb_temp_initialize_temp(int bus, 
                                                           void **oneWire_handle,
                                                           void **sensors_handle)
{
  OneWire *oneWire;
  DallasTemperature *sensors;

  DEBUG("TEMP: otb_temp_initialize_temp entry");  

  oneWire = new OneWire(bus);
  sensors = new DallasTemperature(oneWire);
  // Allow some time for sensors to power on 
  otb_util_delay_ms(1000);
  sensors->begin();
 
  *oneWire_handle = (void *)oneWire;
  *sensors_handle = (void *)sensors;

  DEBUG("TEMP: otb_temp_initialize_temp exit");  

  return;
}

extern "C" void ICACHE_FLASH_ATTR otb_temp_request_temps(void *sensors_handle)
{
  DallasTemperature *sensors;
  
  DEBUG("TEMP: otb_temp_request_temps entry");  
  
  sensors = (DallasTemperature *)sensors_handle;
  sensors->requestTemperatures();

  DEBUG("TEMP: otb_temp_request_temps exit");  

  return;
}

extern "C" uint8_t ICACHE_FLASH_ATTR otb_temp_get_device_count(void *sensors_handle)
{ 
  uint8_t count;
  DallasTemperature *sensors;
  sensors = (DallasTemperature *)sensors_handle;
  
  DEBUG("TEMP: otb_temp_get_device_count entry");  

  count = sensors->getDeviceCount();

  DEBUG("TEMP: otb_temp_get_device_count exit");  

  return count;
}

extern "C" bool ICACHE_FLASH_ATTR otb_temp_get_device_address(void *sensors_handle,
                                                              uint8_t index,
                                            OTB_TEMP_DEVICE_ADDRESS_STRING *addressString)
{ 
  bool rc;
  DallasTemperature *sensors;
  OTB_TEMP_DEVICE_ADDRESS ds18b20;

  DEBUG("TEMP: otb_temp_get_device_address entry");  

  sensors = (DallasTemperature *)sensors_handle;
  rc = sensors->getAddress((uint8_t*)ds18b20, index);
  if (rc)
  {
    // I want the address format in the same format as debian/raspbian
    // Which reverses the order of all but the first byte, and drops the CRC8
    // byte at the end (which we'll check).
    snprintf((char*)addressString,
             OTB_OW_MAX_ADDRESS_STRING_LENGTH,
             "%02x-%02x%02x%02x%02x%02x%02x",
             ds18b20[0],
             ds18b20[6],
             ds18b20[5],
             ds18b20[4],
             ds18b20[3],
             ds18b20[2],
             ds18b20[1]);
    rc = sensors->validAddress((uint8_t*)ds18b20);
    if (rc)
    {
      INFO("DS18B20: Read device %s", addressString);
    }
    else
    {
      WARN("DS18B20: Invalid address received: %s CRC: %02x", addressString, ds18b20[7]);
    }
  }
  else
  {
    WARN("DS18B20: Failed to get device address");
  }
  
  DEBUG("TEMP: otb_temp_get_device_address exit");  
  
  return rc;
}

extern "C" float ICACHE_FLASH_ATTR otb_temp_get_temp_float(void *sensors_handle, int index)
{
  DallasTemperature *sensors;
  float temp;

  DEBUG("TEMP: otb_temp_get_temp_float entry");  

  sensors = (DallasTemperature *)sensors_handle;
  temp = sensors->getTempCByIndex(index);

  DEBUG("TEMP: otb_temp_get_temp_float exit");  

  return(temp);
}

extern "C" uint16_t otb_temp_get_temp_raw(void *sensors_handle, int index)
{
  DallasTemperature *sensors;
  uint16_t rc;

  DEBUG("TEMP: otb_temp_get_temp_raw entry");  

  DeviceAddress deviceAddress;
  sensors = (DallasTemperature *)sensors_handle;
  if (sensors->getAddress(deviceAddress, index))
  {
    rc = sensors->getTemp(deviceAddress);
  }
  else
  {
    WARN("TEMP: Failed to get device address");
    rc = DEVICE_DISCONNECTED_C;
  }

  DEBUG("TEMP: otb_temp_get_temp_raw exit");  

  return rc;
}

extern "C" void otb_temp_get_temp_string(void *sensors_handle, int index, char *temp)
{
  uint16_t raw;
  uint8_t x, y;
  uint8_t mod;

  DEBUG("TEMP: otb_temp_get_temp_string entry");  

  raw = otb_temp_get_temp_raw(sensors_handle, index);
  x = raw/128;
  mod = raw%128;
  y = mod * 10 / 128; 
  if (((mod*10)%128) > 64)
  {
    y++;
  }
  if (y >= 10)
  {
    y = 0;
    x++;
  }
  snprintf(temp, OTB_OW_MAX_TEMP_LEN, "%d.%d", x, y);

  DEBUG("TEMP: otb_temp_get_temp_string exit");  

  return;
}

