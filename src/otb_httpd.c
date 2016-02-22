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
 * This program is distributed in the hope that it will be useful, but WITfHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#define OTB_HTTPD_C
#include "otb.h"

void ICACHE_FLASH_ATTR otb_httpd_start(void)
{
  // Initialize the captive DNS service
  //stdoutInit();
  captdnsInit();

  // Start up the file system to serve files
  espFsInit((void*)(webpages_espfs_start));
  
  // Kick of the HTTP server - otb_wifi_ap_mode_done will signal when done
  httpdInit(otb_httpd_ap_urls, OTB_HTTP_SERVER_PORT);
  
  return;
}

void otb_httpd_station_config_callback(void *arg)
{
  HttpdConnData *connData;
  int len;

  DEBUG("HTTPD: otb_httpd_station_config_callback entry");

  connData = (HttpdConnData *)arg;

  // We set reserved flag on connData when scheduling this callback
  OTB_ASSERT(connData->reserved);
  connData->reserved = FALSE;
  
  if (connData->cgi != NULL)
  {
    // Send HTTPD data on this connection.  It won't be disconnected, as cgi is set to 
    // NULL in this case.
    httpdStartResponse(connData, 200);
    httpdHeader(connData, "Content-Type", "text/html");
    httpdEndHeaders(connData);
    len = 0;
    len += otb_httpd_wifi_form(otb_httpd_scratch+len, OTB_HTTP_SCRATCH_LEN-len);
    len += otb_httpd_display_ap_list(otb_httpd_scratch+len, OTB_HTTP_SCRATCH_LEN-len);
    DEBUG("HTTPD: output %d bytes", len);
    if (len > 1024)
    {
      DEBUG("HTTPD: Send first chunk");
      httpdSend(connData, otb_httpd_scratch, 1024);
      connData->cgiData = (void *)(1024);
      httpdFlushSendBuffer(connData);
    }
    else
    {
      DEBUG("HTTPD: Send whole message");
      httpdSend(connData, otb_httpd_scratch, len);
      httpdFlushSendBuffer(connData);
      // Now mark this connection as done (this is what happens when return HTTPD_CGI_DONE)
      connData->cgiData = NULL;
      connData->cgi = NULL;
    }

  }
  else
  {
    // Connection has been disconnected
    WARN("HTTPD: Scan didn't complete until after session disconnected");
    httpdRetireConn(connData);
  }
  
  DEBUG("HTTPD: otb_httpd_station_config_callback exit");

  return;
}

int ICACHE_FLASH_ATTR otb_httpd_station_config(HttpdConnData *connData)
{
  int rc = HTTPD_CGI_DONE;
  bool wifi_rc;
  bool mqtt_svr_rc;
  bool mqtt_user_rc;
  bool conf_rc;
  int len;
  int ssid_len;
  int password_len;
  char ssid[32];
  char password[64];
  char mqtt_svr[OTB_MQTT_MAX_SVR_LEN];
  char mqtt_port[OTB_MQTT_MAX_SVR_LEN];
  char mqtt_user[OTB_MQTT_MAX_USER_LEN];
  char mqtt_pass[OTB_MQTT_MAX_PASS_LEN];
  int mqtt_svr_len;
  int mqtt_port_len;
  int mqtt_user_len;
  int mqtt_pass_len;
  char buf[512];

  DEBUG("HTTPD: otb_httpd_station_config entry");

  if (connData->conn == NULL)
  {
    DEBUG("HTTPD: Connection aborted");
    goto EXIT_LABEL;
  }

  if (connData->cgiData == NULL)
    if (connData->requestType == HTTPD_METHOD_GET)
    {
      // XXX Handle providing list of APs
      if (otb_wifi_ap_list_struct.searched == OTB_WIFI_AP_SEARCH_NOT_STARTED)
      {
        wifi_rc = otb_wifi_station_scan(otb_httpd_station_config_callback,
                                        (void *)connData);
        if (wifi_rc)
        {
          DEBUG("HTTPD: Station search started");
          rc = HTTPD_CGI_MORE;
          connData->reserved = TRUE;
          goto EXIT_LABEL;
        }
        else
        {
          ERROR("HTTPD: Station search failed");
          len = os_snprintf(otb_httpd_scratch,
                            OTB_HTTP_SCRATCH_LEN,
                            "<body>"
                            "<h2>Detected WiFi Networks</h2>"
                            "<p>Unable to scan at present</p>"
                            "<p>Please retry or restart device</p></body>");
          httpdStartResponse(connData, 200);
          httpdHeader(connData, "Content-Type", "text/html");
          httpdEndHeaders(connData);
          DEBUG("HTTPD: output %d bytes", len);
          if (len > 1024)
          {
            DEBUG("HTTPD: Send first chunk");
            httpdSend(connData, otb_httpd_scratch, 1024);
            connData->cgiData = (void *)(1024);
            rc = HTTPD_CGI_MORE;
          }
          else
          {
            DEBUG("HTTPD: Send whole message");
            httpdSend(connData, otb_httpd_scratch, len);
            connData->cgiData = NULL;
            rc = HTTPD_CGI_DONE;
          }
          goto EXIT_LABEL;
        }
      }
      else if (otb_wifi_ap_list_struct.searched == OTB_WIFI_AP_SEARCH_STARTED)
      {
        DEBUG("HTTPD: Station search not completed yet");
        rc = HTTPD_CGI_DONE;
        goto EXIT_LABEL;
      }
      else
      {
        DEBUG("HTTPD: Station search complete");
        OTB_ASSERT(otb_wifi_ap_list_struct.searched == OTB_WIFI_AP_SEARCH_DONE);
        httpdStartResponse(connData, 200);
        httpdHeader(connData, "Content-Type", "text/html");
        httpdEndHeaders(connData);

        len = 0;
        len += otb_httpd_wifi_form(otb_httpd_scratch+len, OTB_HTTP_SCRATCH_LEN-len);
        len += otb_httpd_display_ap_list(otb_httpd_scratch+len, OTB_HTTP_SCRATCH_LEN-len);
        DEBUG("HTTPD: output %d bytes", len);
        if (len > 1024)
        {
          DEBUG("HTTPD: Send first chunk");
          httpdSend(connData, otb_httpd_scratch, 1024);
          connData->cgiData = (void *)(1024);
          rc = HTTPD_CGI_MORE;
        }
        else
        {
          DEBUG("HTTPD: Send whole message");
          httpdSend(connData, otb_httpd_scratch, len);
          connData->cgiData = NULL;
          rc = HTTPD_CGI_DONE;
        }
        goto EXIT_LABEL;
      }
    }
    else if (connData->requestType == HTTPD_METHOD_POST)
    {
      DEBUG("HTTPD: Handle POST");
      
      wifi_rc = FALSE;
      mqtt_svr_rc = FALSE;
      mqtt_user_rc = FALSE;

      ssid_len = httpdFindArg(connData->post->buff, "ssid", ssid, 32);
      password_len = httpdFindArg(connData->post->buff, "password", password, 64);
      mqtt_svr_len = httpdFindArg(connData->post->buff, "mqtt_svr", mqtt_svr, OTB_MQTT_MAX_SVR_LEN);
      mqtt_port_len = httpdFindArg(connData->post->buff, "mqtt_port", mqtt_port, OTB_MQTT_MAX_SVR_LEN);
      mqtt_user_len = httpdFindArg(connData->post->buff, "mqtt_user", mqtt_user, OTB_MQTT_MAX_USER_LEN);
      mqtt_pass_len = httpdFindArg(connData->post->buff, "mqtt_pass", mqtt_pass, OTB_MQTT_MAX_PASS_LEN);

      // Just in case
      ssid[31] = 0;
      password[63] = 0;
      mqtt_svr[OTB_MQTT_MAX_SVR_LEN-1] = 0;
      mqtt_user[OTB_MQTT_MAX_USER_LEN-1] = 0;
      mqtt_pass[OTB_MQTT_MAX_PASS_LEN-1] = 0;
      
      if ((ssid_len > 0) && (password_len >= 0))
      {
        DEBUG("HTTPD: Valid SSID and password");
        if (os_strcmp(password, "********"))
        {
          wifi_rc = otb_wifi_set_station_config(ssid, password, FALSE);
        }
        else
        {
          // Use old password
          wifi_rc = otb_wifi_set_station_config(ssid, otb_conf->password, FALSE);
        }
        httpdStartResponse(connData, 200);
        httpdHeader(connData, "Content-Type", "text/html");
        httpdEndHeaders(connData);
      }
      
      if ((mqtt_svr_len > 0) && (mqtt_port_len > 0))
      {
        mqtt_svr_rc = otb_mqtt_set_svr(mqtt_svr, mqtt_port, FALSE);
      }
      
      if ((mqtt_user_len > 0) && (mqtt_pass_len >= 0))
      {
        mqtt_user_rc = otb_mqtt_set_user(mqtt_user, mqtt_pass, FALSE);
      }
      
      len = os_snprintf(otb_httpd_scratch,
                          OTB_HTTP_SCRATCH_LEN,
                          "<body>");
      
      if (wifi_rc == OTB_CONF_RC_CHANGED)
      {
        len = os_snprintf(otb_httpd_scratch,
                          OTB_HTTP_SCRATCH_LEN,
                          "<p>Wifi credentials changed to SSID: %s Password: %s</p>",
                          ssid,
                          "********");
      }
      else
      {
        len = os_snprintf(otb_httpd_scratch,
                          OTB_HTTP_SCRATCH_LEN,
                          "<p>Wifi details not changed</p>");
      }
      
      if (mqtt_svr_rc == OTB_CONF_RC_CHANGED)
      {
        len = os_snprintf(otb_httpd_scratch,
                          OTB_HTTP_SCRATCH_LEN,
                          "<p>MQTT server changed to: %s</p>",
                          mqtt_svr);
      }
      else
      {
        len = os_snprintf(otb_httpd_scratch,
                          OTB_HTTP_SCRATCH_LEN,
                          "<p>MQTT server not changed</p>");
      }
      
      if (mqtt_svr_rc == OTB_CONF_RC_CHANGED)
      {
        len = os_snprintf(otb_httpd_scratch,
                          OTB_HTTP_SCRATCH_LEN,
                          "<p>MQTT username/password changed to "
                          "Username: %s Password: %s</p>",
                          mqtt_user,
                          "********");
      }
      else
      {
        len = os_snprintf(otb_httpd_scratch,
                          OTB_HTTP_SCRATCH_LEN,
                          "<p>MQTT username/password not changed</p>");
      }
      
      if ((wifi_rc == OTB_CONF_RC_CHANGED) ||
          (mqtt_svr_rc == OTB_CONF_RC_CHANGED) ||
          (mqtt_user_rc == OTB_CONF_RC_CHANGED))
      {
        conf_rc = otb_conf_update(otb_conf);
        if (!conf_rc)
        {
          len = os_snprintf(otb_httpd_scratch,
                            OTB_HTTP_SCRATCH_LEN,
                            "<p><b>Failed to commit new config to flash!</b></p>");
          ERROR("CONF: Failed to update config");
        }
        // Will cause device to reboot
        len = os_snprintf(otb_httpd_scratch,
                          OTB_HTTP_SCRATCH_LEN,
                          "<p>Rebooting shortly ... </p>");
        otb_wifi_ap_mode_done_fn();          
      }


      len = os_snprintf(otb_httpd_scratch,
                          OTB_HTTP_SCRATCH_LEN,
                          "</body>");

      httpdSend(connData, otb_httpd_scratch, len);
      rc = HTTPD_CGI_DONE;
      
      goto EXIT_LABEL;

      // XXX Handle configuring AP details
    }
    else
    {
      INFO("HTTPD: Got non GET or POST request %d", connData->requestType);
      // 406 is "unacceptable"
      httpdStartResponse(connData, 406);
      httpdEndHeaders(connData);
      goto EXIT_LABEL;
    }
  else
  {
    // XXX Not safe for more than 1 connection
    len = strlen(otb_httpd_scratch + (int)connData->cgiData);
    if (len > 1024)
    {
      INFO("HTTPD: Send another chunk");
      httpdSend(connData, otb_httpd_scratch + (int)(connData->cgiData), 1024);
      connData->cgiData = (void *)((int)connData->cgiData + 1024);
      rc = HTTPD_CGI_MORE;
    }
    else
    {
      DEBUG("HTTPD: Send rest of message");
      httpdSend(connData, otb_httpd_scratch + (int)(connData->cgiData), len);
      connData->cgiData = NULL;
      rc = HTTPD_CGI_DONE;
    }
    
  }
  
EXIT_LABEL:  
  
  DEBUG("HTTPD: otb_httpd_station_config exit");
  
  return(rc);
}

int ICACHE_FLASH_ATTR otb_httpd_wifi_form(char *buffer, uint16_t buf_len)
{
  uint16_t output_len;

  DEBUG("HTTPD: otb_httpd_wifi_form entry");
  
  output_len = 0;
  output_len += os_snprintf(buffer + output_len,
                            buf_len - output_len,
                            "<form name=\"otb-iot\" action=\"/\" method=\"post\">"
                            "<p>WiFi SSID</p>"
                            "<input type=\"text\" name=\"ssid\" value=\"%s\"/>"
                            "<p>WiFi Password</p>"
                            "<input type=\"password\" name=\"password\" value=\"********\"/>"
                            "<p>Disable AP when station connected "
                            "<input type=\"checkbox\" name=\"disable_ap\" value=\"yes\" checked></p>"
                            "<p>MQTT Server (IP address only)</p>"
                            "<input type=\"text\" name=\"mqtt_svr\" value=\"%s\" />"
                            "<p>MQTT Port (default 1883)</p>"
                            "<input type=\"number\" name=\"mqtt_port\" value=\"%d\" />"
                            "<p>MQTT Username</p>"
                            "<input type=\"text\" name=\"mqtt_user\" value=\"%s\" />"
                            "<p>MQTT Password</p>"
                            "<input type=\"password\" name=\"mqtt_pass\" value=\"%s\" />"
                            "<p/>"
                            "<input type=\"submit\" value=\"Store\">"
                            "</form>",
                            otb_conf->ssid,
                            otb_conf->mqtt.svr,
                            otb_conf->mqtt.port,
                            otb_conf->mqtt.user,
                            otb_conf->mqtt.pass);
  
  DEBUG("HTTPD: otb_httpd_wifi_form exit");

  return output_len;
}

int ICACHE_FLASH_ATTR otb_httpd_display_ap_list(char *buffer, uint16_t buf_len)
{
  uint16_t output_len;
  struct otb_wifi_ap *ap;
  
  DEBUG("HTTPD: otb_httpd_display_ap_list entry");

  output_len = 0;
  output_len += os_snprintf(buffer + output_len,
                            buf_len - output_len,
                            "<!DOCTYPE html><html><head><title>%s</title></head>",
                            OTB_MAIN_DEVICE_ID);
  
  output_len += os_snprintf(buffer + output_len,
                            buf_len - output_len,
                            "<body><h2>%s</h2><ul>",
                            OTB_MAIN_DEVICE_ID);
  output_len += os_snprintf(buffer + output_len,
                            buf_len - output_len,
                            "<h3>Detected WiFi Networks</h3><ul>");
  ap = otb_wifi_ap_list_struct.first;
  while (ap != NULL)
  {
    output_len += os_snprintf(buffer + output_len,
                              buf_len - output_len,
                              "<li>Name: <a href=\"#\" onclick=\"document.wifi_ap.ssid.value='%s';return false;\">%s</a> Signal strenth %d</li>",
                              ap->ssid,
                              ap->ssid,
                              ap->rssi);
    ap = ap->next;
  }
  
  output_len += os_snprintf(buffer + output_len,
                            buf_len - output_len,
                            "</ul></body></html>");
  
  DEBUG("HTTPD: otb_httpd_display_ap_list exit");

  return(output_len);
}
 