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

#define OTB_HTTPD_METHOD_NONE  0
#define OTB_HTTPD_METHOD_HEAD  1
#define OTB_HTTPD_METHOD_GET   2
#define OTB_HTTPD_METHOD_POST  3

#define OTB_HTTPD_MAX_URL_LEN  64
#define OTB_HTTPD_MAX_HTTP_LEN 16

#define OTB_HTTPD_HTTP_1_0  "HTTP/1.0"
#define OTB_HTTPD_HTTP_1_1  "HTTP/1.1"

#define OTB_HTTPD_HEADER_CONTENT_LEN  "Content-Length"
#define OTB_HTTPD_HEADER_CONNECTION   "Connection"
#define OTB_HTTPD_HEADER_KEEP_ALIVE   "Keep-Alive"
#define OTB_HTTPD_HEADER_KEEP_ALIVE_LOWER   "keep-alive"

#define OTB_HTTPD_ADD_HEADER(BUF, BUF_LEN, FMT, ...) \
                   os_snprintf(BUF,                  \
                               BUF_LEN,              \
                               FMT "\r\n",           \
                               ##__VA_ARGS__)
#define OTB_HTTPD_END_HEADERS(BUF, BUF_LEN)          \
                   os_snprintf(BUF,                  \
                               BUF_LEN,              \
                               "\r\n")
#define OTB_HTTPD_ADD_BODY(BUF, BUF_LEN, BODY)       \
                   os_snprintf(BUF,                  \
                           BUF_LEN,                  \
                           BODY)

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
} otb_httpd_request;

typedef struct otb_httpd_connection
{
  bool active;
  otb_httpd_request request;
} otb_httpd_connection;

void otb_httpd_start(void);
void otb_httpd_connect_callback(void *arg);
void otb_httpd_recon_callback(void *arg, sint8 err);
void otb_httpd_discon_callback(void *arg);
void otb_httpd_sent_callback(void *arg);
void otb_httpd_recv_callback(void *arg, char *data, unsigned short len);
uint16 otb_httpd_process_method(char *data, uint16 len);
uint16 otb_httpd_process_url(char *data, uint16 len);
uint16 otb_httpd_process_http(char *data, uint16 len);
uint16 otb_httpd_process_start_line(char *data, uint16 len);
uint16 otb_httpd_get_next_header(char *data,
                                 uint16 len,
                                 char *header_name,
                                 uint16 header_name_len,
                                 char *header_value,
                                 uint16 header_value_len);
uint16 otb_httpd_process_headers(char *data, uint16 len);
uint16 otb_httpd_build_core_response(char *buf, uint16 len, uint16 body_len);
sint16 otb_httpd_get_arg(char *data, char *find, char *found, uint16 found_len);
uint16 otb_httpd_station_config(uint8 method, char *buf, uint16 len);
int otb_httpd_wifi_form(char *buffer, uint16_t buf_len);
int otb_httpd_display_ap_list(char *buffer, uint16_t buf_len);

#ifdef OTB_HTTPD_C

#define OTB_HTTP_SCRATCH_LEN 2048
char otb_httpd_scratch[OTB_HTTP_SCRATCH_LEN];
#define OTB_HTTP_SCRATCH2_LEN 1024
char otb_httpd_scratch2[OTB_HTTP_SCRATCH2_LEN];
#define OTB_HTTP_MSG_LEN 2048
char otb_httpd_msg[OTB_HTTP_MSG_LEN];

static otb_httpd_connection otb_httpd_conn;
static struct espconn otb_httpd_espconn;
static esp_tcp otb_httpd_tcp;

#endif // OTB_HTTPD_C
#endif // OTB_HTTPD_H
