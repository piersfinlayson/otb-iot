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

#include "otb_cpp.h"
 
char otb_compile_date[12];
char otb_compile_time[9];
char otb_version_id[OTB_MAIN_MAX_VERSION_LENGTH];

// Logging function, needs to be extern "C" as accessed from C as well as CPP
extern "C" void otb_main_log_fn(char *text)
{
  Serial.println(text);
  //delay(500);
}

void configModeCallback();
char ssid[32];
char OTB_MAIN_CHIPID[OTB_MAIN_CHIPID_STR_LENGTH];
char OTB_MAIN_DEVICE_ID[20];

extern "C" void otb_reset()
{
  DEBUG("OTB: otb_reset entry");

  // Log resetting and include \n so this log isn't overwritten
  INFO("OTB: Resetting ...");
  // Delay to give any serial logs time to output.  Can't use arduino delay as
  // may be in wrong context
  otb_util_delay_ms(1000);

  // Reset by pulling reset GPIO (connected to reset) low
  digitalWrite(OTB_MAIN_GPIO_RESET, LOW);
  // ESP.reset();

  DEBUG("OTB: otb_reset exit");
 
  return;
}

void setup(void)
{
  // Set up serial port for logging
  Serial.begin(115200);

  DEBUG("OTB: setup entry");

  // Set up the reset CPIO to be high
  pinMode(OTB_MAIN_GPIO_RESET, OUTPUT);
  digitalWrite(OTB_MAIN_GPIO_RESET, HIGH);

  // Set up and log some useful info
  sprintf(otb_compile_date, "%s", __DATE__);
  sprintf(otb_compile_time, "%s", __TIME__);
  snprintf(otb_version_id,
           OTB_MAIN_MAX_VERSION_LENGTH,
           "%s/%s/%s/%s",
           OTB_MAIN_OTB_IOT,
           OTB_MAIN_FW_VERSION,
           otb_compile_date, 
           otb_compile_time);
  // First log needs a line break!
  INFO("\nOTB: %s", otb_version_id);
  INFO("OTB: Arduino setup function");
  sprintf(OTB_MAIN_CHIPID, "%06x", ESP.getChipId());
  INFO("OTB: ESP device %s", OTB_MAIN_CHIPID);
  sprintf(OTB_MAIN_DEVICE_ID, "OTB-IOT.%s", OTB_MAIN_CHIPID);
  
  // Call C setup routine
  c_setup();
  
  DEBUG("OTB: setup exit");
  
  return;
}
 
void loop(void)
{
  // DEBUG("OTB: loop entry");

  // Call the scheduler
  //otb_sched_execute();

  // DEBUG("OTB: loop exit");
  
  return;
}
