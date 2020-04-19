/*
 *
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
 * 
 */

#ifndef OTB_HTTPD_H
#define OTB_HTTPD_H

#define OTB_HTTPD_METHOD_NONE  0
#define OTB_HTTPD_METHOD_HEAD  1
#define OTB_HTTPD_METHOD_GET   2
#define OTB_HTTPD_METHOD_POST  4

#define OTB_HTTPD_MAX_URL_LEN  64
#define OTB_HTTPD_MAX_HTTP_LEN 16

#define OTB_HTTPD_HTTP_1_0  "HTTP/1.0"
#define OTB_HTTPD_HTTP_1_1  "HTTP/1.1"

#define OTB_HTTPD_HEADER_CONTENT_LEN  "Content-Length"
#define OTB_HTTPD_HEADER_CONNECTION   "Connection"
#define OTB_HTTPD_HEADER_KEEP_ALIVE   "Keep-Alive"
#define OTB_HTTPD_HEADER_KEEP_ALIVE_LOWER   "keep-alive"

#define OTB_HTTPD_MAX_CONNS 4

#define OTB_HTTPD_ADD_HEADER(BUF, BUF_LEN, FMT, ...) \
                   os_snprintf(BUF,                  \
                               BUF_LEN,              \
                               FMT "\r\n",           \
                               ##__VA_ARGS__)
#define OTB_HTTPD_END_HEADERS(BUF, BUF_LEN)          \
                   os_snprintf(BUF,                  \
                               BUF_LEN,              \
                               "\r\n")

//
// Fn: otb_httpd_url_handler_fn
//
// Purpose: Used to handle HTTP method for a particular URL and method as
// configured in otb_httpd_url_match_array
//
// Returns:
// - Length of data written to buf (ignoring NULL terminating byte)
// - Status code (in request, if other than 200)
// - Status string (in request, if other than OK)
//
// Args:
// - request - request to be processed
// - arg - any arg specified in otb_httpd_url_match_array
// - buf - buffer to use for body response
// - buf_len - maximum size of buf (including NULL terminating byte)
//
struct otb_httpd_request; // Need to define as incomplete type now
typedef uint16_t otb_httpd_url_handler_fn(struct otb_httpd_request *request,
                                          void *arg,
                                          char *buf,
                                          uint16_t buf_len);

typedef struct otb_httpd_url_match
{
  // The URL prefix to match
  unsigned char *match_prefix;

  // Whether to match anything after this prefix
  bool wildcard;
  
  // If sub_cmd_control is NULL, call this handler function instead.  Function must
  // follow otb_cmd_handler prototype
  otb_httpd_url_handler_fn *handler_fn;
  
  // If handler_fn is to be invoked, the following value is passed into as arg
  void *arg;

  // Supported methods
  uint32_t methods;

} otb_httpd_url_match;

typedef struct otb_httpd_post
{
  char *data;
  uint16 data_capacity;
  uint16 expected_data_len;
  uint16 current_data_len;
} otb_httpd_post;

typedef struct otb_httpd_request
{
  bool handling;
  char url[OTB_HTTPD_MAX_URL_LEN];
  char http[OTB_HTTPD_MAX_HTTP_LEN];
  uint8 method;
  otb_httpd_post post;
  uint16 status_code;
  char *status_str;
  char *redirect_to;
  bool redirect;
  bool keepalive;
  struct otb_httpd_url_match *match;
  char *content_type;
} otb_httpd_request;

typedef struct otb_httpd_connection
{
  bool active;
  otb_httpd_request request;
  uint8 remote_ip[4];
  int remote_port;
} otb_httpd_connection;

bool otb_httpd_start(bool dns);
void otb_httpd_stop(void);
void otb_httpd_connect_callback(void *arg);
void otb_httpd_recon_callback(void *arg, sint8 err);
void otb_httpd_discon_callback(void *arg);
void otb_httpd_sent_callback(void *arg);
void otb_httpd_recv_callback(void *arg, char *data, unsigned short len);
uint16 otb_httpd_process_method(otb_httpd_connection *hconn, char *data, uint16 len);
uint16 otb_httpd_process_url(otb_httpd_connection *hconn, char *data, uint16 len);
uint16 otb_httpd_process_http(otb_httpd_connection *hconn, char *data, uint16 len);
uint16 otb_httpd_process_start_line(otb_httpd_connection *hconn, char *data, uint16 len);
uint16 otb_httpd_get_next_header(char *data,
                                 uint16 len,
                                 char *header_name,
                                 uint16 header_name_len,
                                 char *header_value,
                                 uint16 header_value_len);
uint16 otb_httpd_process_headers(otb_httpd_connection *hconn, char *data, uint16 len);
uint16 otb_httpd_build_core_response(otb_httpd_connection *hconn, char *buf, uint16 len, uint16 body_len);
sint16 otb_httpd_get_arg(char *data, char *find, char *found, uint16 found_len);
uint16_t otb_httpd_mqtt_handler(otb_httpd_request *request,
                                void *arg,
                                char *buf,
                                uint16_t buf_len);
uint16_t otb_httpd_not_found_handler(otb_httpd_request *request,
                                     void *arg,
                                     char *buf,
                                     uint16_t buf_len);
uint16_t otb_httpd_base_handler(otb_httpd_request *request,
                                void *arg,
                                char *buf,
                                uint16_t buf_len);
uint16 otb_httpd_station_config(otb_httpd_request *request, uint8 method, char *buf, uint16 len);
int otb_httpd_wifi_form(char *buffer, uint16_t buf_len);
int otb_httpd_display_ap_list(char *buffer, uint16_t buf_len);

#ifdef OTB_HTTPD_C
bool otb_httpd_started;
bool otb_httpd_dns_inited;

#define OTB_HTTP_SCRATCH_LEN 2048
char *otb_httpd_scratch;
#define OTB_HTTP_SCRATCH_MATCH_LEN 2048
char *otb_httpd_scratch_match;
#define OTB_HTTP_MSG_LEN 2048
char *otb_httpd_msg;

static otb_httpd_connection otb_httpd_conn[OTB_HTTPD_MAX_CONNS];
static struct espconn otb_httpd_espconn;
static esp_tcp otb_httpd_tcp;

char otb_httpd_content_type_text_html[]  = "text/html";
char otb_httpd_content_type_text_plain[] = "text/plain";

#define OTB_HTTPD_DEFAULT_BASE_URL "/"
otb_httpd_url_match otb_httpd_url_match_array[] = 
{
  {"/otb-iot", 
   TRUE,
   otb_httpd_mqtt_handler,
   NULL,
   OTB_HTTPD_METHOD_GET | OTB_HTTPD_METHOD_POST},
  {"/espi",
   TRUE,
   otb_httpd_mqtt_handler,
   NULL,
   OTB_HTTPD_METHOD_GET | OTB_HTTPD_METHOD_POST},
  {"/mqtt",
   TRUE,
   otb_httpd_mqtt_handler,
   NULL,
   OTB_HTTPD_METHOD_GET | OTB_HTTPD_METHOD_POST},
  {"/base",
   FALSE,
   otb_httpd_base_handler,
   NULL,
   OTB_HTTPD_METHOD_HEAD | OTB_HTTPD_METHOD_GET | OTB_HTTPD_METHOD_POST},
  {"/favicon.ico",
   FALSE,
   otb_httpd_not_found_handler,
   NULL,
   OTB_HTTPD_METHOD_GET},
  {OTB_HTTPD_DEFAULT_BASE_URL,
   TRUE,
   otb_httpd_base_handler,
   NULL,
   OTB_HTTPD_METHOD_HEAD | OTB_HTTPD_METHOD_GET | OTB_HTTPD_METHOD_POST},
  {NULL, FALSE, NULL, NULL, OTB_HTTPD_METHOD_NONE}, // Must terminate with this
};

static const char ALIGN4 ICACHE_RODATA_ATTR otb_httpd_wifi_form_str[] =
"<body><form name=\"otb-iot\" action=\"/\" method=\"post\">"
  "<p/>WiFi SSID<br/>"
    "<input type=\"text\" name=\"ssid\" value=\"%s\"/>"
  "<p/>WiFi Password<br/>"
    "<input type=\"password\" name=\"password\" value=\"********\"/>"
  "<p/>MQTT Server (IP address or fully qualiied domain name)<br/>"
    "<input type=\"text\" name=\"mqtt_svr\" value=\"%s\" />"
  "<p/>MQTT Port (default 1883)<br/>"
    "<input type=\"number\" name=\"mqtt_port\" value=\"%d\" />"
  "<p/>MQTT Username<br/>"
    "<input type=\"text\" name=\"mqtt_user\" value=\"%s\" />"
  "<p/>MQTT Password<br/>"
    "<input type=\"password\" name=\"mqtt_pass\" value=\"%s\" />"
  "<p>Keep AP enabled when station connected:<br/>"
    "<input type=\"radio\" name=\"enable_ap\" id=\"yes\" value=\"yes\" %s />"
      "<label for=\"yes\">Yes</label><br/>"
    "<input type=\"radio\" name=\"enable_ap\" id=\"no\" value=\"no\" %s />"
      "<label for=\"no\">No</label><br/>"
  "<p/>Run HTTP server to expose MQTT API:<br/>"
    "<input type=\"radio\" name=\"http_svr\" id=\"enabled\" value=\"enabled\" %s />"
      "<label for=\"enabled\">Enabled</label><br/>"
    "<input type=\"radio\" name=\"http_svr\" id=\"disabled\" value=\"disabled\" %s />"
      "<label for=\"disabled\">Disabled</label><br/>"
  "<p/>Domain name suffix (optional)<br/>"
    "<input type=\"text\" name=\"domain_name\" value=\"%s\" />"
  "<p>IP address configuration:</p>"
    "<input type=\"radio\" name=\"ip_config\" id=\"dhcp\" value=\"dhcp\" %s />"
      "<label for=\"dhcp\">DHCP</label><br/>"
    "<input type=\"radio\" name=\"ip_config\" id=\"manual\" value=\"manual\" %s />"
      "<label for=\"manual\">Manual</label><br/>"
  "<p/>IP address<br/>"
    "<input type=\"text\" name=\"ip_addr\" value=\"%s\" />"
  "<p/>Subnet mask<br/>"
    "<input type=\"text\" name=\"subnet\" value=\"%s\" />"
  "<p/>Default gateway<br/>"
    "<input type=\"text\" name=\"gateway\" value=\"%s\" />"
  "<p/>DNS server 1<br/>"
    "<input type=\"text\" name=\"dns1\" value=\"%s\" />"
  "<p/>DNS server 2<br/>"
    "<input type=\"text\" name=\"dns2\" value=\"%s\" />"
  "<p/>"
  "<input type=\"submit\" value=\"Store\">"
"</form></body>";

#endif // OTB_HTTPD_C
#endif // OTB_HTTPD_H
