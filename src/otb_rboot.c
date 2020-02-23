/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-2020 Piers Finlayson
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

#include "otb.h"

MLOG("RBOOT");

volatile rboot_ota otb_rboot_ota;
static bool otb_rboot_update_in_progress = FALSE;
static char otb_update_request[512];
#define HTTP_HEADER "Connection: keep-alive\r\nCache-Control: no-cache\r\nUser-Agent: rBoot-Sample/1.0\r\nAccept: */*\r\n\r\n"

const char ALIGN4 otb_rboot_update_callback_error_string[] = "RBOOT: Update successful";
void ICACHE_FLASH_ATTR otb_rboot_update_callback(void *arg, bool result)
{
  bool rc;

  ENTRY;
  
  OTB_ASSERT(arg == (void *)&otb_rboot_ota);
  
  otb_rboot_update_in_progress = FALSE;
  
  if (result)
  {
    MDETAIL("Update succeeded");
    rc = rboot_set_current_rom(otb_rboot_ota.rom_slot);
    if (rc)
    {
      MDETAIL("Set slot to %d", otb_rboot_ota.rom_slot);
      // XXX Doesn't give system time to actually send the status
      otb_mqtt_send_status(OTB_MQTT_SYSTEM_UPDATE,
                           OTB_MQTT_STATUS_OK,
                           "resetting",
                           "");
      otb_reset_schedule(1000, otb_rboot_update_callback_error_string, FALSE);
    }
    else
    {
      MERROR("Failed to set slot to %d", otb_rboot_ota.rom_slot);
      otb_mqtt_send_status(OTB_MQTT_SYSTEM_UPDATE,
                           OTB_MQTT_STATUS_ERROR,
                           "Failed to set boot slot after update",
                           "");
      //otb_ads_initialize();
      goto EXIT_LABEL;
    }
  }
  else
  {
    // Log this as an error so gets sent over MQTT
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_UPDATE,
                         OTB_MQTT_STATUS_ERROR,
                         "Update failed",
                         "");
    MDETAIL("Update failed");
    //otb_ads_initialize();
    goto EXIT_LABEL;
  }
  
EXIT_LABEL:
  
  EXIT;
  
  return;
}

bool ICACHE_FLASH_ATTR otb_rboot_update(char *ip, char *port, char *path, unsigned char **error)
{
  uint8_t current_slot;
  uint8_t update_slot;
  bool rc = FALSE;
  char slot[8];
  uint8 ota_ip[4];
  char *pos;
  //char *pos2;
  int ii;
  uint32 port_int;
  int len;
  
  ENTRY;

  // Won't update if already updtating.
  if (otb_rboot_update_in_progress)
  {
    MDEBUG("Can't update as already in progress");
    rc = FALSE;
    *error = "update already in progress";
    goto EXIT_LABEL;
  }

  // Sanity check args
  if ((ip == NULL) ||
      (ip[0] == 0) ||
      (port == NULL) ||
      (port[0] == 0) ||
      (path == NULL) ||
      (path[0] == 0))
  {
    MDEBUG("Duff input vars");
    rc = FALSE;
    *error = "invalid arguments";
    goto EXIT_LABEL;
  }
  
  // Process IP address arg
  for (ii = 0, pos = ip; (pos != NULL) && (ii < 4); ii++)
  {
    // Get next byte of IP address
    ota_ip[ii] = (uint8)atoi(pos);
    
    // Move past the dot
    //pos2 = os_strstr(pos, OTB_MQTT_COLON);
    pos = os_strstr(pos, OTB_MQTT_PERIOD);
    //if ((pos2 != NULL) && (pos < pos2))
    if (pos != NULL)
    {
      pos++;
    }
  }
  
  // Null terminate the IP address string, as we'll need it later.  Don't care if this
  // fails - means was already null terminated
  pos = os_strstr(ip, OTB_MQTT_COLON);
  if (pos != NULL)
  {
    *pos = 0;
  }

  // Process port argument
  port_int = atoi(port);
  
  // Process path argument - see if there's a colon, and if so strip
  pos = os_strstr(path, OTB_MQTT_COLON);
  if (pos != NULL)
  {
    *pos = 0;
  }
  // If starts with / remove - we'll add back in later
  if (path[0] == '/')
  {
    path++;
  }

  // Build update request struct
  otb_rboot_update_in_progress = TRUE;
  memset((void *)&otb_rboot_ota, 0, sizeof(otb_rboot_ota));
  memcpy((void *)otb_rboot_ota.ip, ota_ip, 4);
  otb_rboot_ota.port = port_int;
  otb_rboot_ota.callback = (ota_callback)otb_rboot_update_callback;
  otb_rboot_ota.request = otb_update_request;
  
  // Figure out which slot to update
  current_slot = rboot_get_current_rom();
  if (current_slot == 0)
  {
    update_slot = 1;
  }
  else
  {
    update_slot = 0;
  }
  otb_rboot_ota.rom_slot = update_slot;

  // Build the HTTP request
  len = os_snprintf((char *)otb_rboot_ota.request,
                    512,
                    "GET /%s HTTP/1.1\r\nHost: %s\r\n" HTTP_HEADER,
                    path,
                    ip);
  if (len >= 512)
  {
    rc = FALSE;
    *error = "path too long";
    goto EXIT_LABEL;
  }
    
  MDETAIL("Update slot: %d from host: %d.%d.%d.%d port: %d path: /%s",
      update_slot,
      ota_ip[0], ota_ip[1], ota_ip[2], ota_ip[3],
      port_int,
      path);
  
  // Kill all timers cos they'll interfere
  //otb_i2c_ads_disable_all_timers();
  //otb_led_wifi_disable_blink_timer();
  
  rc = rboot_ota_start((rboot_ota *)&otb_rboot_ota);
  if (!rc)
  {
    *error = "internal error";
    goto EXIT_LABEL;
  }
  else
  {
    *error = "ok";
  }
  
EXIT_LABEL:  

  EXIT;
  
  return(rc);
}

bool ICACHE_FLASH_ATTR otb_rboot_update_slot(char *msg, unsigned char **response)
{
  bool rc = TRUE;
  int slot;

  ENTRY;
  
  if (msg == NULL)
  {
    MDETAIL("No slot number");
    rc = FALSE;
    *response = "No slot number";
    goto EXIT_LABEL;
  }

  slot = atoi(msg);
  if ((slot < 0) || (slot > 1))
  {
    MDETAIL("Invalid slot number");
    rc = FALSE;
    *response = "Invalid slot number";
    goto EXIT_LABEL;
  }
  
  rc = rboot_set_current_rom(slot);
  if (rc)
  {
    *response = "ok";
  }
  else
  {
    *response = "Failed to set slot";
  }
  
EXIT_LABEL:

  EXIT;
  
  return(rc);
}

uint8_t ICACHE_FLASH_ATTR otb_rboot_get_slot(bool publish)
{
  uint8_t slot;
  char slot_s[4];

  ENTRY;

  slot = rboot_get_current_rom();  
  if (publish)
  {
    os_snprintf(slot_s, 4, "%d", slot);
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_BOOT_SLOT,
                         OTB_MQTT_STATUS_OK,
                         slot_s,
                         "");
  }
  
  EXIT;  
  
  return(slot);
}

#if 0

bool ICACHE_FLASH_ATTR ota_rboot_check_factory_image(void)
{
  bool rc = FALSE;
  unsigned char buffer[0x1000]; // Shouldn't really allocate 4k on stack, but should be a clean boot
  uint16_t buf_len;
  
  ENTRY;
  
  rboot_check_image();
  
  EXIT;
  
  return rc;
}

bool ICACHE_FLASH_ATTR ota_rboot_use_factory_image(void)
{
  bool rc = FALSE;
  
  ENTRY;
  
  EXIT;
  
  return rc;
}
#endif
