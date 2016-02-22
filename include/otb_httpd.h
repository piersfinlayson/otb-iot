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

#ifndef OTB_HTTPD_H
#define OTB_HTTPD_H

extern HttpdBuiltInUrl otb_httpd_ap_urls[];

void otb_httpd_start(void);
void otb_httpd_station_config_callback(void *arg);
int otb_httpd_station_config(HttpdConnData *connData);
int otb_httpd_wifi_form(char *buffer, uint16_t buf_len);
int otb_httpd_display_ap_list(char *buffer, uint16_t buf_len);

#ifdef OTB_HTTPD_C

#define OTB_HTTP_SCRATCH_LEN 8192
char otb_httpd_scratch[OTB_HTTP_SCRATCH_LEN];
HttpdBuiltInUrl otb_httpd_ap_urls[] =
{
//  {"*", cgiRedirectApClientToHostname, OTB_MAIN_DEVICE_ID},
  {"*", cgiRedirectApClientToHostname, "192.168.4.1"},
  {"/", otb_httpd_station_config, NULL},
  {"/*", cgiRedirect, "/"},
  {NULL, NULL, NULL}
};





#endif // OTB_HTTPD_C
#endif // OTB_HTTPD_H
