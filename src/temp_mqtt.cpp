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

extern "C" void ICACHE_FLASH_ATTR initialize_temp(int bus, void **oneWire_handle, void **sensors_handle)
{
  OneWire *oneWire;
  DallasTemperature *sensors;

  oneWire = new OneWire(bus);
  sensors = new DallasTemperature(oneWire);
 
  *oneWire_handle = (void *)oneWire;
  *sensors_handle = (void *)sensors;
}

extern "C" void ICACHE_FLASH_ATTR request_temps(void *sensors_handle)
{
  DallasTemperature *sensors;
  sensors = (DallasTemperature *)sensors_handle;
  sensors->requestTemperatures();
}

extern "C" float ICACHE_FLASH_ATTR get_temp(void *sensors_handle, int index)
{
  DallasTemperature *sensors;
  float temp;
  sensors = (DallasTemperature *)sensors_handle;
  temp = sensors->getTempCByIndex(index);
  return(temp);
}
