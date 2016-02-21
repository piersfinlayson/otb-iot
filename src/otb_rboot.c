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

#include "otb.h"

volatile rboot_ota otb_rboot_ota;
static bool otb_rboot_update_in_progress = FALSE;
static char otb_update_request[512];
static const uint8 ota_ip[4] = {192, 168, 0, 162};
#define HTTP_HEADER "Connection: keep-alive\r\nCache-Control: no-cache\r\nUser-Agent: rBoot-Sample/1.0\r\nAccept: */*\r\n\r\n"

char ALIGN4 otb_rboot_update_callback_error_string[] = "RBOOT: Update successful";
void ICACHE_FLASH_ATTR otb_rboot_update_callback(void *arg, bool result)
{
  bool rc;

  DEBUG("RBOOT: otb_rboot_update_callback entry");
  
  OTB_ASSERT(arg == (void *)&otb_rboot_ota);
  
  otb_rboot_update_in_progress = FALSE;
  
  if (result)
  {
    INFO("RBOOT: Update succeeded");
    rc = rboot_set_current_rom(otb_rboot_ota.rom_slot);
    if (rc)
    {
      INFO("RBOOT: Set slot to %d", otb_rboot_ota.rom_slot);
      // XXX Doesn't give system time to actually send the status
      otb_mqtt_send_status(OTB_MQTT_SYSTEM_UPDATE,
                           OTB_MQTT_STATUS_OK,
                           "",
                           "");
      otb_reset(otb_rboot_update_callback_error_string);
    }
    else
    {
      ERROR("RBOOT: Failed to set slot to %d", otb_rboot_ota.rom_slot);
      otb_mqtt_send_status(OTB_MQTT_SYSTEM_UPDATE,
                           OTB_MQTT_STATUS_ERROR,
                           "Failed to set boot slot after update",
                           "");
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
    ERROR("RBOOT: Update failed");
    goto EXIT_LABEL;
  }
  
EXIT_LABEL:
  
  DEBUG("RBOOT: otb_rboot_update_callback exit");
  
  return;
}

bool ICACHE_FLASH_ATTR otb_rboot_update(char *ip, char *path)
{
  uint8_t current_slot;
  uint8_t update_slot;
  bool rc = FALSE;
  char slot[8];
  
  DEBUG("RBOOT: otb_rboot_update entry");
  
  if (otb_rboot_update_in_progress)
  {
    INFO("RBOOT: Can't update as already in progress");
    goto EXIT_LABEL;
  }
  otb_rboot_update_in_progress = TRUE;
  memset((void *)&otb_rboot_ota, 0, sizeof(otb_rboot_ota));
  memcpy((void *)otb_rboot_ota.ip, ota_ip, 4);
  
  otb_rboot_ota.port = 8080;
  otb_rboot_ota.callback = (ota_callback)otb_rboot_update_callback;
  otb_rboot_ota.request = otb_update_request;
  
  current_slot = rboot_get_current_rom();
  if (current_slot == 0)
  {
    update_slot = 1;
  }
  else
  {
    update_slot = 0;
  }
  INFO("RBOOT: Update slot %d", update_slot);
  otb_rboot_ota.rom_slot = update_slot;
  
  os_snprintf((char *)otb_rboot_ota.request,
              512,
              "GET /app_image.bin HTTP/1.1\r\nHost: 192.168.0.162\r\n"HTTP_HEADER);
  
  rc = rboot_ota_start((rboot_ota *)&otb_rboot_ota);
  if (!rc)
  {
    // Log this as an error so gets sent over MQTT
    ERROR("RBOOT: Update failed");
  }
  
EXIT_LABEL:  

  if (rc)
  {
    os_snprintf(slot, 8, "%d", update_slot);
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_UPDATE,
                         OTB_MQTT_STATUS_OK,
                         "slot",
                         slot);
  }
  else
  {
    otb_rboot_update_in_progress = FALSE;
  }
  
  DEBUG("RBOOT: otb_rboot_update exit");
  
  return(rc);
}

bool ICACHE_FLASH_ATTR otb_rboot_update_slot(char *msg)
{
  bool rc = TRUE;
  int slot;
  char *response;

  DEBUG("RBOOT: otb_rboot_update_slot entry");
  
  if (msg == NULL)
  {
    INFO("RBOOT: No slot number");
    rc = FALSE;
    response = "No slot number";
    goto EXIT_LABEL;
  }

  slot = atoi(msg);
  if ((slot < 0) || (slot > 1))
  {
    INFO("RBOOT: Invalid slot number");
    rc = FALSE;
    response = "Invalid slot number";
    goto EXIT_LABEL;
  }
  
  rc = rboot_set_current_rom(slot);
  if (rc)
  {
    response = OTB_MQTT_EMPTY;
  }
  else
  {
    response = "Failed to set slot";
  }
  
EXIT_LABEL:

  if (rc)
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_BOOT_SLOT,
                         OTB_MQTT_STATUS_OK,
                         response,
                         "");
  }
  else
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_BOOT_SLOT,
                         OTB_MQTT_STATUS_ERROR,
                         response,
                         "");
  }
  
  DEBUG("RBOOT: otb_rboot_update_slot exit");
  
  return(rc);
}

uint8_t ICACHE_FLASH_ATTR otb_rboot_get_slot(bool publish)
{
  uint8_t slot;
  char slot_s[4];

  DEBUG("RBOOT: otb_rboot_get_slot entry");

  slot = rboot_get_current_rom();  
  if (publish)
  {
    os_snprintf(slot_s, 4, "%d", slot);
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_BOOT_SLOT,
                         OTB_MQTT_STATUS_OK,
                         slot_s,
                         "");
  }
  
  DEBUG("RBOOT: otb_rboot_get_slot exit");  
  
  return(slot);
}