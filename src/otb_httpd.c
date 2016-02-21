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
  int len;
  int ssid_len;
  int password_len;
  char ssid[32];
  char password[64];
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
      ssid_len = httpdFindArg(connData->post->buff, "ssid", ssid, 32);
      password_len = httpdFindArg(connData->post->buff, "password", password, 33);
      if ((ssid_len >= 0) && (password_len >= 0))
      {
        DEBUG("HTTPD: Valid SSID and password");
        wifi_rc = otb_wifi_set_station_config(ssid, password);
        httpdStartResponse(connData, 200);
        httpdHeader(connData, "Content-Type", "text/html");
        httpdEndHeaders(connData);
        if (wifi_rc)
        {
          len = os_snprintf(otb_httpd_scratch,
                            OTB_HTTP_SCRATCH_LEN,
                            "<body>"
                            "<p>Wifi credentials set ... restarting shortly</p>"
                            "</body>");
          otb_wifi_ap_mode_done_fn();          
        }
        else
        {
          len = os_snprintf(otb_httpd_scratch,
                            OTB_HTTP_SCRATCH_LEN,
                            "<body>"
                            "<p>Failed to set wifi credentials</p>"
                            "<p>Please <a href=\"wifi.html\">try again</a></p>"
                            "</body>");
        }
        httpdSend(connData, otb_httpd_scratch, len);
        rc = HTTPD_CGI_DONE;
        goto EXIT_LABEL;
      }
      else
      {
        INFO("HTTPD: Invalid ssid or password entered");
      }
    
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
                            "<form name=\"wifi_ap\" action=\"wifi.html\" method=\"post\" >"
                            "<p>SSID</p>"
                            "<input type=\"text\" name=\"ssid\" />"
                            "<p>Password</p>"
                            "<input type=\"password\" name=\"password\" />"
                            "<p/>"
                            "<p>Disable AP when station connected "
                            "<input type=\"checkbox\" name=\"disable_ap\" value=\"yes\" checked></p>"
                            "<input type=\"submit\" value=\"Store\">"
                            "</form>");
  
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
 