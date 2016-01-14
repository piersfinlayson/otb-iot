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

#include <OneWire.h>
#include <DallasTemperature.h>
#include "otbCpp.h"
#include <WiFiManager.h>
#include "fake_scheduler.h"
extern "C" {
#include "c_types.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "smartconfig.h"
#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/dns.h"
}

 
char compile_date[12];
char compile_time[9];
char version_id[MAX_VERSION_ID_LENGTH];

// WiFiManager object to hande wifi
WiFiManager wifiManager;

// Logging function, needs to be extern "C" as accessed from C as well as CPP
extern "C" void log_otb(char *text)
{
  Serial.println(text);
  //delay(500);
}

// Must extern "C" scheduler, as accessed from C code as well as CPP
extern "C" Scheduler scheduler;
void configModeCallback();
char ssid[32];
char OTB_CHIPID[CHIPID_STR_LENGTH];

extern "C" void reset()
{
  // Delay to give any serial logs time to output
  delay(500);
  //pinMode(GPIO_RESET, OUTPUT);
  //digitalWrite(GPIO_RESET, LOW);
  ESP.reset();
}

void setup(void)
{
  sprintf(compile_date, "%s", __DATE__);
  sprintf(compile_time, "%s", __TIME__);
  sprintf(version_id, "%s/%s/%s/%s", OTB_ROOT, OTB_FW_VERSION, compile_date, compile_time);
  // Set up serial port for logging
  Serial.begin(115200);
  LOG("\nARDUNIO: %s", version_id);
  LOG("ARDUINO: Arduino setup function");
  sprintf(OTB_CHIPID, "%06x", ESP.getChipId());

  pinMode(GPIO_RESET, OUTPUT);
  digitalWrite(GPIO_RESET, HIGH);
  LOG("ARDUINO: Set up Wifi");

  // Initialize WiFiManager
  WiFi.mode(WIFI_STA); // First thing we'll do is try and connect as a station
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(DEFAULT_WIFI_TIMEOUT);
  sprintf(ssid, "%s %s", DEFAULT_WIFI_SSID_PREFIX, OTB_CHIPID);

  // Wifi doesn't work at all well if no SSID set - just hangs trying to 
  // connect.  So let's reset it to something (doesn't matter what) if it's
  // blank.
  String current_ssid = wifiManager.getSSID();
  if (!strcmp(current_ssid.c_str(), ""))
  {
    LOG("ARDUINO: Set SSID to %s", DEFAULT_DUMMY_SSID);
    struct station_config wifi_conf;
    strcpy((char *)wifi_conf.ssid, DEFAULT_DUMMY_SSID); 
    strcpy((char *)wifi_conf.password, "");
    wifi_station_set_config(&wifi_conf);
  }

  // Actually connect
  boolean success = wifiManager.autoConnect(ssid, NULL);
  if (!success)
  {
    // This means couldn't connect to any configured wifi, and no other wifi
    // was configured within DEFAULT_WIFI_TIMEOUT, so reset to have another go
    LOG("ARDUNIO: WiFiManager autoConnect failed");
    reset();
  }

  // Call C setup routine
  c_setup(log_otb);
}
 
void loop(void)
{
  //LOG("ARDUINO: Arduino loop function\n");

  // Call C loop routine
  scheduler.execute();
}

void configModeCallback()
{
  // Nothing to do - let auto config program kick in
  LOG("ARDUINO: In configModeCallback");
}

