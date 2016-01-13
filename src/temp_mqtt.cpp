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

#include "OneWire.h"
#include "DallasTemperature.h"
#include "otbCpp.h"
#include "otb_def.h"
#define TEMP_MQTT_CPP
#include "temp_mqtt.h"

extern "C" void ICACHE_FLASH_ATTR initialize_temp(int bus, void **oneWire_handle, void **sensors_handle)
{
  OneWire *oneWire;
  DallasTemperature *sensors;

  oneWire = new OneWire(bus);
  sensors = new DallasTemperature(oneWire);
  // Allow some time for sensors to power on 
  delay(1000);
  sensors->begin();
 
  *oneWire_handle = (void *)oneWire;
  *sensors_handle = (void *)sensors;
}

extern "C" void ICACHE_FLASH_ATTR request_temps(void *sensors_handle)
{
  DallasTemperature *sensors;
  sensors = (DallasTemperature *)sensors_handle;
  sensors->requestTemperatures();
}

extern "C" uint8_t ICACHE_FLASH_ATTR get_device_count(void *sensors_handle)
{ 
  uint8_t count;
  DallasTemperature *sensors;
  sensors = (DallasTemperature *)sensors_handle;
  count = sensors->getDeviceCount();
  return count;
}

extern "C" bool ICACHE_FLASH_ATTR get_device_address(void *sensors_handle, uint8_t index, DeviceAddressString *addressString)
{ 
  bool rc;
  DallasTemperature *sensors;
  DeviceAddress ds18b20;

  sensors = (DallasTemperature *)sensors_handle;
  rc = sensors->getAddress((uint8_t*)ds18b20, index);
  if (rc)
  {
    // I want the address format in the same format as debian/raspbian
    // Which reverses the order of all but the first byte, and drops the CRC8
    // byte at the end (which we'll check).
    sprintf((char*)addressString, "%02x-%02x%02x%02x%02x%02x%02x",
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
      LOG("DS18B20: Read device %s", addressString);
    }
    else
    {
      LOG("DS18B20: Invalid address received: %s CRC: %02x", addressString, ds18b20[7]);
    }
  }
  else
  {
    LOG("DS18B20: Failed to get device address");
  }
  
  return rc;
}

extern "C" float ICACHE_FLASH_ATTR get_temp(void *sensors_handle, int index)
{
  DallasTemperature *sensors;
  float temp;
  sensors = (DallasTemperature *)sensors_handle;
  temp = sensors->getTempCByIndex(index);
  return(temp);
}
