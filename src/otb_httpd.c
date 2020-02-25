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

MLOG("HTTPD");

bool ICACHE_FLASH_ATTR otb_httpd_start(bool dns)
{
  bool rc = TRUE;

  ENTRY;

  if (dns)
  {
    // Initialize the captive DNS service
    captdnsInit();
    otb_httpd_dns_inited = TRUE;
    MDETAIL("Started Captive DNS");
  }

  // Set up TCP connection handlers 
  os_memset(&otb_httpd_espconn, 0, sizeof(otb_httpd_espconn));
  os_memset(&otb_httpd_tcp, 0, sizeof(otb_httpd_tcp));
  os_memset(&otb_httpd_conn, 0, OTB_HTTPD_MAX_CONNS * sizeof(otb_httpd_connection));
  otb_httpd_espconn.type = ESPCONN_TCP;
  otb_httpd_espconn.state = ESPCONN_NONE;
  otb_httpd_espconn.proto.tcp = &otb_httpd_tcp;
  otb_httpd_tcp.local_port = 80;
  espconn_regist_connectcb(&otb_httpd_espconn, otb_httpd_connect_callback);
  espconn_tcp_set_max_con_allow(&otb_httpd_espconn, 1);
  espconn_accept(&otb_httpd_espconn);
  MDETAIL("Started listening on port %d", otb_httpd_tcp.local_port);

  otb_httpd_scratch = (char *)os_malloc(OTB_HTTP_SCRATCH_LEN);
  otb_httpd_scratch_match = (char *)os_malloc(OTB_HTTP_SCRATCH_MATCH_LEN);
  otb_httpd_msg = (char *)os_malloc(OTB_HTTP_MSG_LEN);

  if ((otb_httpd_scratch == NULL) ||
      (otb_httpd_scratch_match == NULL) ||
      (otb_httpd_msg == NULL))
  {
    MWARN("Failed to allocate scratch buffers");
    rc = FALSE;
    goto EXIT_LABEL;
  }

EXIT_LABEL:

  otb_httpd_started = TRUE;

  if (!rc)
  {
    otb_httpd_stop();
    otb_httpd_started = FALSE;
  }
  
  EXIT;

  return(rc);
}

void ICACHE_FLASH_ATTR otb_httpd_stop(void)
{
  bool rc = TRUE;

  ENTRY;

  if (otb_httpd_dns_inited)
  {
    // Terminate the captive DNS service
    captdnsTerm();
    otb_httpd_dns_inited = FALSE;
    MDETAIL("Terminated Captive DNS");
  }

  if (otb_httpd_started)
  {
    // Terminate HTTPD TCP connection
    espconn_disconnect(&otb_httpd_espconn);
    MDETAIL("Stopped listening");

    // Free up scratch buffers
    OTB_ASSERT(otb_httpd_scratch != NULL);
    OTB_ASSERT(otb_httpd_scratch_match != NULL);
    OTB_ASSERT(otb_httpd_msg != NULL);
    os_free(otb_httpd_scratch);
    os_free(otb_httpd_scratch_match);
    os_free(otb_httpd_msg);
    otb_httpd_scratch = NULL;
    otb_httpd_scratch_match = NULL;
    otb_httpd_msg = NULL;
    otb_httpd_started = FALSE;
  }
  else
  {
    MWARN("Can't terminate HTTP stack - not started");
  }

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_httpd_connect_callback(void *arg)
{
  int ii;
  otb_httpd_connection *hconn = NULL;
  struct espconn *conn = arg;

  ENTRY;

  // Get free otb_httpd_conn
  for (ii = 0; ii < OTB_HTTPD_MAX_CONNS; ii++)
  {
    hconn = otb_httpd_conn + ii;
    if (!hconn->active)
    {
      hconn = otb_httpd_conn + ii;
      break;
    }
  }

  if (hconn == NULL)
  {
    MDETAIL("Too many incoming connection requests - reject");
    espconn_disconnect(conn);
    goto EXIT_LABEL;
  }

  MDEBUG("Incoming connection from: %d.%d.%d.%d:%d",
        conn->proto.tcp->remote_ip[0],
        conn->proto.tcp->remote_ip[1],
        conn->proto.tcp->remote_ip[2],
        conn->proto.tcp->remote_ip[3],
        conn->proto.tcp->remote_port);
  MDEBUG("hconn: %p", hconn);

  conn->reverse = hconn;
  hconn->active = true;
  os_memcpy(hconn->remote_ip, conn->proto.tcp->remote_ip, 4);
  hconn->remote_port = conn->proto.tcp->remote_port;

  espconn_regist_recvcb(conn, otb_httpd_recv_callback);
  espconn_regist_reconcb(conn, otb_httpd_recon_callback);
  espconn_regist_disconcb(conn, otb_httpd_discon_callback);
  espconn_regist_sentcb(conn, otb_httpd_sent_callback);

EXIT_LABEL:

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_httpd_recon_callback(void *arg, sint8 err)
{
  struct espconn *conn = arg;
  otb_httpd_connection *hconn = conn->reverse;

  ENTRY;

  MDEBUG("hconn: %p", hconn);
  MDEBUG("Recon reason %d", err);
  otb_httpd_discon_callback(arg);

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_httpd_discon_callback(void *arg)
{
  struct espconn *conn = arg;
  otb_httpd_connection *hconn = conn->reverse;

  ENTRY;

  OTB_ASSERT(hconn != NULL);
  MDEBUG("hconn: %p", hconn);

  if (!hconn->active)
  {
    MWARN("Received disconnect when connection already disconnected");
  }

  // Clear hconn out
  os_memset(hconn, 0, sizeof(hconn));
  conn->reverse = NULL;

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_httpd_sent_callback(void *arg)
{
  struct espconn *conn = arg;
  otb_httpd_connection *hconn = conn->reverse;

  ENTRY;

  MDEBUG("hconn: %p", hconn);
  OTB_ASSERT(hconn != NULL);
  if (!hconn->active)
  {
    MWARN("Received sent callback when connection already disconnected");
  }

  EXIT;

  return;
}

uint16 ICACHE_FLASH_ATTR otb_httpd_process_method(otb_httpd_connection *hconn, char *data, uint16 len)
{
  uint16 cur = 0;

  ENTRY;

  // Process method
  if (os_strncmp(data, "HEAD ", OTB_MIN(5, len)) == 0)
  {
    MDEBUG("METHOD: HEAD");
    hconn->request.method = OTB_HTTPD_METHOD_HEAD;
    cur += 5;
  }
  else if (os_strncmp(data, "GET ", OTB_MIN(4, len)) == 0)
  {
    MDEBUG("METHOD: GET");
    hconn->request.method = OTB_HTTPD_METHOD_GET;
    cur += 4;
  }
  else if (os_strncmp(data, "POST ", OTB_MIN(5, len)) == 0)
  {
    MDEBUG("METHOD: POST");
    hconn->request.method = OTB_HTTPD_METHOD_POST;
    cur += 5;
  }
  else
  {
    MWARN("Unsupported method");
    hconn->request.status_code = 405;
    hconn->request.status_str = "Method Not Allowed";
  }

  EXIT;

  return cur;
}

uint16 ICACHE_FLASH_ATTR otb_httpd_process_url(otb_httpd_connection *hconn, char *data, uint16 len)
{
  uint16 cur = 0;
  int jj;

  ENTRY;

  // Process URL
  for (; cur < len; cur++)
  {
    if (data[cur] != ' ')
    {
      break;
    }
  }
  for (jj = 0; cur < len; cur++, jj++)
  {
    if (data[cur] == ' ')
    {
      break;
    }
    if (jj > (OTB_HTTPD_MAX_URL_LEN-1))
    {
      // Defensive branch
      jj = OTB_HTTPD_MAX_URL_LEN-1;
      break;
    }
    if (jj >= (OTB_HTTPD_MAX_URL_LEN-1))
    {
      break;
    }
    hconn->request.url[jj] = data[cur];
  }
  hconn->request.url[jj] = 0; // NULL terminate
  MDEBUG("URL: %s", hconn->request.url);

  EXIT;

  return cur;
}

uint16 ICACHE_FLASH_ATTR otb_httpd_process_http(otb_httpd_connection *hconn, char *data, uint16 len)
{
  uint16 cur = 0;
  int jj;

  ENTRY;

  // Process HTTP version
  for (; cur < len; cur++)
  {
    if (data[cur] != ' ')
    {
      break;
    }
  }
  for (jj = 0; cur < len; cur++, jj++)
  {
    if ((data[cur] == ' ') || 
        (data[cur] == '\r') ||
        (data[cur] == '\n')) 
    {
      break;
    }
    if (jj > (OTB_HTTPD_MAX_HTTP_LEN-1))
    {
      // Defensive branch
      jj = OTB_HTTPD_MAX_HTTP_LEN-1;
      break;
    }
    if (jj >= (OTB_HTTPD_MAX_HTTP_LEN-1))
    {
      break;
    }
    hconn->request.http[jj] = data[cur];
  }
  hconn->request.http[jj] = 0; // NULL terminate
  MDEBUG("HTTP: %s", hconn->request.http);

  if (os_strcmp(OTB_HTTPD_HTTP_1_0, hconn->request.http) &&
      os_strcmp(OTB_HTTPD_HTTP_1_1, hconn->request.http))
  {
    MWARN("Unsupported HTTP version: %s", hconn->request.http);
    hconn->request.status_code = 505;
    hconn->request.status_str = "HTTP Version Not Supported";
    os_snprintf(hconn->request.http, OTB_HTTPD_MAX_HTTP_LEN, OTB_HTTPD_HTTP_1_1); // Set version we'll use to respond
  }

  EXIT;

  return cur;
}

uint16 ICACHE_FLASH_ATTR otb_httpd_process_start_line(otb_httpd_connection *hconn, char *data, uint16 len)
{
  uint16 cur = 0;

  int jj;

  ENTRY;


  cur += otb_httpd_process_method(hconn, data+cur, len-cur);
  cur += otb_httpd_process_url(hconn, data+cur, len-cur);
  cur += otb_httpd_process_http(hconn, data+cur, len-cur);

  EXIT;

  return cur;
}

uint16 ICACHE_FLASH_ATTR otb_httpd_get_next_header(char *data,
                                                   uint16 len,
                                                   char *header_name,
                                                   uint16 header_name_len,
                                                   char *header_value,
                                                   uint16 header_value_len)
{
  uint16 cur = 0;
  int jj;

  ENTRY;

  // Skip whitespace
  for (cur = 0; cur < len; cur++)
  {
    if ((data[cur] != ' ') &&
        (data[cur] != '\r') &&
        (data[cur] != '\n'))
    {
      break;
    }
  }

  // Header name
  for (jj = 0; (cur < len); cur++, jj++)
  {
    if ((data[cur] == ' ') ||
        (data[cur] == '\r') ||
        (data[cur] == '\n') ||
        (data[cur] == ':'))
    {
      break;
    }
    // Check if have space to store!
    if (jj < (header_name_len-1))
    {
      header_name[jj] = data[cur];
    }
  }
  // NULL terminate
  if (jj < (header_name_len-1))
  {
    header_name[jj] = 0;
  }
  else
  {
    header_name[header_name_len-1] = 0;
  }
  
  
  // Skip whitespace
  for (; cur < len; cur++)
  {
    if ((data[cur] != ' ') &&
        (data[cur] != '\r') &&
        (data[cur] != '\n') &&
        (data[cur] != ':'))
    {
      break;
    }
  }

  // Header value
  for (jj = 0; (cur < len); cur++, jj++)
  {
    if ((data[cur] == '\r') ||
        (data[cur] == '\n'))
    {
      break;
    }
    // Check if have space to store!
    if (jj < (header_name_len-1))
    {
      header_value[jj] = data[cur];
    }
  }
  // NULL terminate
  if (jj < (header_value_len-1))
  {
    header_value[jj] = 0;
  }
  else
  {
    header_value[header_value_len-1] = 0;
  }

  if ((header_name[0] != 0) && (header_value[0] != 0))
  {
    MDEBUG("HEADER: Name: %s Value: %s", header_name, header_value);
  }
  else
  {
    MDEBUG("HEADER: None");
  }

  EXIT;

  return cur;
}

uint16 ICACHE_FLASH_ATTR otb_httpd_process_headers(otb_httpd_connection *hconn, char *data, uint16 len)
{
  uint16 cur = 0;
  char *end;
  uint16 header_len;
#define OTB_HTTPD_MAX_HEADER_LEN  32  
  char header_name[OTB_HTTPD_MAX_HEADER_LEN];
  char header_value[OTB_HTTPD_MAX_HEADER_LEN];

  ENTRY;

  // Figure out where the headers end
  end = strstr(data, "\r\n\r\n");
  if (end == NULL)
  {
    // Don't have entire string - bail out with 0 byte return which must
    // meant we don't have headers yet
    hconn->request.status_code = 400;
    hconn->request.status_str = "Bad Request";
    goto EXIT_LABEL;
  }
  end += 4;

  header_len = end - data;
  while (cur < header_len)
  {
    cur += otb_httpd_get_next_header(data+cur,
                                     header_len-cur,
                                     header_name,
                                     OTB_HTTPD_MAX_HEADER_LEN,
                                     header_value,
                                     OTB_HTTPD_MAX_HEADER_LEN);
    if (hconn->request.method == OTB_HTTPD_METHOD_POST)
    {
      if (!os_strncmp(OTB_HTTPD_HEADER_CONTENT_LEN,
                      header_name,
                      sizeof(OTB_HTTPD_HEADER_CONTENT_LEN)))
      {
        hconn->request.post.expected_data_len = atoi(header_value);
        MDEBUG("Expected POST data len: %d", hconn->request.post.expected_data_len);
      }
    }
    if (!os_strncmp(OTB_HTTPD_HEADER_CONNECTION,
                    header_name,
                    sizeof(OTB_HTTPD_HEADER_CONNECTION)))
    {
      if ((!os_strncmp(OTB_HTTPD_HEADER_KEEP_ALIVE,
                      header_value,
                      sizeof(OTB_HTTPD_HEADER_KEEP_ALIVE))))
#if 0
          (!os_strncmp(OTB_HTTPD_HEADER_KEEP_ALIVE_LOWER,
                      header_value,
                      sizeof(OTB_HTTPD_HEADER_KEEP_ALIVE_LOWER))))
#endif
      {
        MDEBUG("Keep-alive requested");
        hconn->request.keepalive = true;
      }
    }
  }

  cur = header_len;  // End of headers

EXIT_LABEL:

  EXIT;

  return cur;
}

void ICACHE_FLASH_ATTR otb_httpd_recv_callback(void *arg, char *data, unsigned short len)
{
  struct espconn *conn = arg;
  otb_httpd_connection *hconn = conn->reverse;
  uint16 rsp_len = 0;
  char *body = "";
  bool station_config = false;
  bool send_rsp = true;
  char *buf;
  char *buf2;
  uint16 rsp_len2;
  uint16 len2;
  uint16 body_len;
  uint16 cur = 0;
  otb_httpd_url_match *cand;
  int ii;
  int url_len, cand_len;
  bool handle = FALSE;
  bool match_rc;

  ENTRY;

  MDEBUG("hconn: %p", hconn);
  OTB_ASSERT(hconn != NULL);
  if (!hconn->active)
  {
    MWARN("Received data when connection disconnected");
    espconn_disconnect(conn);
    goto EXIT_LABEL;
  }

  MDEBUG("REQUEST: Len %d:\n---\n%s\n---", len, data);
  hconn->request.content_type = otb_httpd_content_type_text_html;

  // Assume it's a new request if we're not mid-way through processing a POST
  if (hconn->request.handling)
  {
    // Handling a POST already
    if ((hconn->request.post.current_data_len + len + 1) > hconn->request.post.data_capacity) // +1 for NULL term
    {
      MDETAIL("Don't have enough buffer to process entire message");
      hconn->request.status_code = 413;
      hconn->request.status_str = "Request Entity Too Large";
      goto EXIT_LABEL;
    }

    os_memcpy(hconn->request.post.data+hconn->request.post.current_data_len, data, len);
    hconn->request.post.current_data_len += len;
    hconn->request.post.data[hconn->request.post.current_data_len] = 0; // NULL terminate

    data = hconn->request.post.data;
    len = hconn->request.post.current_data_len;

    // No need to check statuses - we've processed these before
    cur += otb_httpd_process_start_line(hconn, data+cur, len-cur);
    cur += otb_httpd_process_headers(hconn, data+cur, len-cur);

    if (hconn->request.post.expected_data_len > len-cur)
    {
      MDEBUG("Waiting for %d bytes more data", hconn->request.post.expected_data_len-(len-cur));
      send_rsp = false;
      goto EXIT_LABEL;
    }

    // Set up post data to just point at the body
    MDEBUG("Have whole POST, so process");
    hconn->request.post.data += cur;
    hconn->request.post.current_data_len -= cur;
    hconn->request.post.data_capacity -= cur;
    hconn->request.status_code = 200;
    hconn->request.status_str = "OK";
    station_config = true;
  }
  else
  {
    hconn->request.status_code = 200;
    hconn->request.status_str = "OK";
    cur = 0;
    cur += otb_httpd_process_start_line(hconn, data+cur, len-cur);

    if (hconn->request.status_code != 200)
    {
      // Failed to process headers successfully
      goto EXIT_LABEL;
    }

    // Ran out of message before handling it all
    if (len <= cur)
    {
      hconn->request.status_code = 400;
      hconn->request.status_str = "Bad Request";
      goto EXIT_LABEL;
    }

    cur += otb_httpd_process_headers(hconn, data+cur, len-cur);
    
    // URLs to be handled are in otb_httpd_url_match_array which also
    // specifies match function, methods supported and any args
    url_len = os_strlen(hconn->request.url);
    hconn->request.match = NULL; // Should already be NULL, but belt and braces
    for (ii = 0; otb_httpd_url_match_array[ii].match_prefix != NULL; ii++)
    {
      cand = otb_httpd_url_match_array + ii;
      cand_len = os_strlen(cand->match_prefix);
      // Match if:
      // - The URL prefix is at least a match for the entire candidate
      // - and the method matches
      // - and either
      //   - it's a wildcard match (so any suffix matches)
      //   - or it's not a wildcard match and both the URL and candidate
      //     are the same length (so the same)
      if (!os_strncmp(hconn->request.url,
                      cand->match_prefix,
                      cand_len) &&
          (hconn->request.method & cand->methods) &&
          (cand->wildcard ||
           (cand_len == os_strlen(hconn->request.url))))
      {
        hconn->request.match = cand;
        MDETAIL("URL match: %s with: %s", hconn->request.url, cand->match_prefix);
        break;
      }
    }
    if (hconn->request.match == NULL)
    {
      hconn->request.status_code = 404;
      hconn->request.status_str = "Not Found";
      MDETAIL("URL not found: %s %d", hconn->request.url, hconn->request.status_code);
      goto EXIT_LABEL;
    }

    // If a POST check we have all of it before processing
    if (hconn->request.method == OTB_HTTPD_METHOD_POST)
    {
      if (hconn->request.post.expected_data_len > len-cur)
      {
        // Don't have the whole message yet
        MDEBUG("Haven't got whole POST, so don't process");
        if ((hconn->request.post.expected_data_len + 1 ) > OTB_HTTP_MSG_LEN)  // +1 for NULL term
        {
          MDEBUG("Don't have enough buffer to process entire message");
          hconn->request.status_code = 413;
          hconn->request.status_str = "Request Entity Too Large";
          body = "Can't handle a message this large";
        }
        else
        {
          hconn->request.status_code = 200;
          hconn->request.status_str = "OK";
          send_rsp = false;
          hconn->request.post.data = otb_httpd_msg;
          os_memcpy(hconn->request.post.data, data, len);
          hconn->request.post.data_capacity = OTB_HTTP_MSG_LEN;
          hconn->request.post.current_data_len = len;
          hconn->request.post.data[hconn->request.post.current_data_len] = 0; // NULL terminate
          hconn->request.handling = true;
        }
        goto EXIT_LABEL;
      }
      else
      {
        MDEBUG("Have whole POST, so process");
        hconn->request.post.data = data+cur;
        hconn->request.post.data_capacity = len-cur;
        hconn->request.post.current_data_len = len-cur;
        handle = TRUE;
      }
    }
    else
    {
      handle = TRUE;
    }
    
    // Waiting until after checking URL for this - as if a POST
    // to the wrong URL we can just return immediately whether
    // we have all this message or not
    if (hconn->request.status_code != 200)
    {
      // Failed to process headers successfully.
      // If a POST bail out and wait for another message
      // If any other messages just reject
      goto EXIT_LABEL;
    }
  }
  
EXIT_LABEL:

  if (send_rsp)
  {
    if (handle)
    {
      // See otb_httpd_url_handler_fn in otb_httpd.h for a description of what
      // the match function does (hint, handles a URL/method!)
      hconn->request.status_code = 200;
      hconn->request.status_str = "OK";
      body_len = hconn->request.match->handler_fn(&(hconn->request),
                                                  hconn->request.match->arg,
                                                  otb_httpd_scratch_match,
                                                  OTB_HTTP_SCRATCH_MATCH_LEN);
      MDETAIL("Match function status code: %d", hconn->request.status_code);
      body = otb_httpd_scratch_match;
    }
    else
    {
      body_len = os_strlen(body);
    }
   

    buf = otb_httpd_scratch;
    len = OTB_HTTP_SCRATCH_LEN;
    rsp_len = 0;
    rsp_len += otb_httpd_build_core_response(hconn,
                                             buf+rsp_len,
                                             len-rsp_len,
                                             body_len);
    if (hconn->request.method != OTB_HTTPD_METHOD_HEAD)
    {
      // Don't send the body if HEAD (but do send the body_len)
      rsp_len += OTB_HTTPD_ADD_BODY(buf+rsp_len,
                                    len-rsp_len,
                                    body);
    }

    MDEBUG("RESPONSE: Len %d:\n---\n%s\n---", rsp_len, otb_httpd_scratch);
    espconn_send(conn, otb_httpd_scratch, rsp_len);

    if (!hconn->request.keepalive)
    {
      MDEBUG("Disconnect TCP connection");
      // Disconnect the TCP connection - we said we would if we're HTTP/1.1
      // It's only fair as we only support a single connection simultaneously)
      espconn_disconnect(conn);
    }
    else
    {
      MDEBUG("Keepalive TCP connection");
    }

    // We've handled this request, so clear it out
    os_memset(&hconn->request, 0, sizeof(hconn->request));
  }

  EXIT;

  return;
}

uint16 ICACHE_FLASH_ATTR otb_httpd_build_core_response(otb_httpd_connection *hconn, char *buf, uint16 len, uint16 body_len)
{
  uint16 rsp_len = 0;
  char *content_type = otb_httpd_content_type_text_html;

  ENTRY;


  rsp_len += OTB_HTTPD_ADD_HEADER(buf+rsp_len,
                                  len-rsp_len,
                                  "%s %d %s",
                                  hconn->request.http,
                                  hconn->request.status_code,
                                  hconn->request.status_str);
  rsp_len += OTB_HTTPD_ADD_HEADER(buf+rsp_len,
                                  len-rsp_len,
                                  "Server: %s",
                                  otb_mqtt_root);
  if ((!os_strcmp(hconn->request.http, OTB_HTTPD_HTTP_1_1)) &&
      (!hconn->request.keepalive))
  {
    rsp_len += OTB_HTTPD_ADD_HEADER(buf+rsp_len,
                                    len-rsp_len,
                                    "Connection: close");
  }
  if (hconn->request.redirect)
  {
    rsp_len += OTB_HTTPD_ADD_HEADER(buf+rsp_len,
                                    len-rsp_len,
                                    "Location: %s",
                                    hconn->request.redirect_to);
  }
  if (body_len > 0)
  {
    rsp_len += OTB_HTTPD_ADD_HEADER(buf+rsp_len,
                                    len-rsp_len,
                                    "Content-Len: %d",
                                    body_len);
    if (hconn->request.content_type != NULL)
    {
      content_type = hconn->request.content_type;
    }
    rsp_len += OTB_HTTPD_ADD_HEADER(buf+rsp_len,
                                    len-rsp_len,
                                    "Content-Type: %s",
                                    hconn->request.content_type);
  }
  rsp_len += OTB_HTTPD_END_HEADERS(buf+rsp_len,
                                   len-rsp_len);

  EXIT;

  return rsp_len;
}

sint16 ICACHE_FLASH_ATTR otb_httpd_get_arg(char *data, char *find, char *found, uint16 found_len)
{
  sint16 len;
  int jj;
  uint16 find_len;
  uint16 data_len;

  ENTRY;

  find_len = os_strlen(find);
  data_len = os_strlen(data);
  jj = 0;
  len = -1;
  while ((jj+find_len) < data_len)
  {
    if (!os_strncmp(data+jj, find, find_len))
    {
      len = 0;
      jj += find_len;
      if (data[jj] == '=')
      {
        jj++;
        while ((jj < data_len) &&
              (len < (found_len-1)) &&
              (data[jj] != '&') &&
              (data[jj] != '\r') &&
              (data[jj] != '\n'))
        {
          found[len] = data[jj];
          len++;
          jj++;
        }
        OTB_ASSERT(len < found_len);
        found[len] = 0;
        break;
      }
    }
    else
    {
      while ((jj < data_len) &&
            (data[jj] != '&') &&
            (data[jj] != '\r') &&
            (data[jj] != '\n'))
      {
        jj++;
      }
      if (data[jj] != '&')
      {
        break;
      }
    }
    jj++;
  }

  if (len >= 0)
  {
    MDEBUG("FOUND: Arg: %s Value: %s Len: %d", find, found, len);
  }
  else
  {
    MDEBUG("NOT FOUND: Arg: %s", find);
  }

  EXIT;

  return len;
}

uint16_t ICACHE_FLASH_ATTR otb_httpd_mqtt_handler(otb_httpd_request *request,
                                                  void *arg,
                                                  char *buf,
                                                  uint16_t buf_len)
{
  uint16_t body_len = 0;
  char *msg_start;
  bool bad_url = FALSE;
  bool method_not_allowed = FALSE;
  bool forbidden = FALSE;
#define OTB_HTTPD_MQTT_FAKE_TOPIC_LEN 64 
  char fake_topic[OTB_HTTPD_MQTT_FAKE_TOPIC_LEN];

  ENTRY;

  //
  // Don't yet know how to deal with non-initiated messages, such as
  // - booted
  // - mbus data
  //
  // XXX AP won't be running unless set to keep alive - that needs to control
  // which URLs are available
  //

  // Only process these MQTT URLs if config setting is enabled (we must be at
  // a suitable point to process if can get to web server!)
  if (otb_conf->mqtt_httpd)
  {
    msg_start = os_strstr((request->url)+1, "/");
    if (msg_start != NULL)
    {
      msg_start++;
      // /.../get/...     - GET method supported
      // /.../set/...     - POST method supported (no post data - all data
      //                      should be in the URL as for MQTT
      // /.../trigger/... - as /.../set/...
      if (!os_strncmp(msg_start, "get", 3))
      {
        if (request-> method == OTB_HTTPD_METHOD_GET)
        {
          MDETAIL("Have GET request for get MQTT command");
        }
        else
        {
        }
      }
      else if (!os_strncmp(msg_start, "set", 3) ||
               !os_strncmp(msg_start, "trigger", 7))
      {
        if (request-> method == OTB_HTTPD_METHOD_POST)
        {
          MDETAIL("Have POST request for set or trigger MQTT command");
        }
        else
        {
          method_not_allowed = TRUE;
        }
      }
      else
      {
        bad_url = TRUE;
      }

      if (!bad_url && !method_not_allowed)
      {
        // Record via MQTT that this request came in
        otb_mqtt_publish(&otb_mqtt_client,
                        OTB_MQTT_STATUS_HTTP,
                        "",
                        msg_start,
                        "",
                        2,
                        0,
                        NULL,
                        0);  

        // Fake up the topic so MQTT stack deals properly with it
        os_snprintf(fake_topic, OTB_HTTPD_MQTT_FAKE_TOPIC_LEN-1, "/%s/%s", otb_mqtt_root, OTB_MAIN_CHIPID);

        // Give to the MQTT stack to process - passing in buf means as well
        // as sending a response via MQTT, it also fills buf with the response
        otb_cmd_mqtt_receive(NULL,
                             fake_topic,
                             os_strlen(fake_topic),
                             msg_start,
                             os_strlen(msg_start),
                             buf,
                             buf_len);
        body_len = os_strnlen(buf, buf_len-1);
      }
    }
    else
    {
      bad_url = TRUE;
    }
  }
  else
  {
    forbidden = TRUE;
    goto EXIT_LABEL;
  }

EXIT_LABEL:

  // Error handling
  if (forbidden)
  {
    request->status_code = 403;
    request->status_str = "Forbidden";
    body_len = os_snprintf(buf, buf_len, "MQTT HTTPD not enabled");
  }
  else if (method_not_allowed)
  {
    request->status_code = 405;
    request->status_str = "Method Not Allowed";
    body_len = os_snprintf(buf, buf_len, "Use GET for get, POST for set/trigger");
  }
  else if (bad_url)
  {
    request->status_code = 400;
    request->status_str = "Bad Request";
    body_len = os_snprintf(buf, buf_len, "Invalid MQTT URL: %s", request->url);
  }

  buf[buf_len - 1] = 0;

  request->content_type = otb_httpd_content_type_text_plain;

  EXIT;

  return(body_len);
}

uint16_t ICACHE_FLASH_ATTR otb_httpd_base_handler(otb_httpd_request *request,
                                                  void *arg,
                                                  char *buf,
                                                  uint16_t buf_len)
{
  uint16_t body_len = 0;

  ENTRY;

  // Expose base config pages if AP is supposed to be kept active, or if we
  // don't have MQTT running (yet)
  if (otb_conf->keep_ap_active || !otb_mqtt_connected)
  {
    body_len = otb_httpd_station_config(request,
                                        request->method,
                                        buf,
                                        buf_len);
    request->content_type = otb_httpd_content_type_text_html;
  }
  else
  {
    request->status_code = 403;
    request->status_str = "Forbidden";
    body_len = os_snprintf(buf, buf_len, "Keep AP alive not enabled");
    request->content_type = otb_httpd_content_type_text_plain;
  }

  buf[buf_len - 1] = 0;

  EXIT;

  return(body_len);
}

uint16_t ICACHE_FLASH_ATTR otb_httpd_not_found_handler(otb_httpd_request *request,
                                                       void *arg,
                                                       char *buf,
                                                       uint16_t buf_len)
{
  uint16_t body_len = 0;

  ENTRY;

  body_len = 0;
  request->status_code = 404;
  request->status_str = "Not Found";
  
  buf[buf_len - 1] = 0;

  EXIT;

  return(body_len);
}

uint16 ICACHE_FLASH_ATTR otb_httpd_station_config(otb_httpd_request *request, uint8 method, char *buf, uint16 len)
{
  bool conf_changed = FALSE;
  uint8_t ip[4];
  uint16 rsp_len = 0;
  uint8 wifi_rc;
  uint8 mqtt_svr_rc;
  uint8 mqtt_user_rc;
  bool conf_rc;
  sint16 ssid_len;
  sint16 password_len;
  char ssid[32];
  char password[64];
  char mqtt_svr[OTB_MQTT_MAX_SVR_LEN];
  char mqtt_port[OTB_MQTT_MAX_SVR_LEN];
  char mqtt_user[OTB_MQTT_MAX_USER_LEN];
  char mqtt_pass[OTB_MQTT_MAX_PASS_LEN];
  char domain_name[OTB_IP_MAX_DOMAIN_NAME_LEN];
  char ip_address[OTB_IP_MAX_IPV4_ADDR_LEN];
  char netmask[OTB_IP_MAX_IPV4_ADDR_LEN];
  char gateway[OTB_IP_MAX_IPV4_ADDR_LEN];
  char dns1[OTB_IP_MAX_IPV4_ADDR_LEN];
  char dns2[OTB_IP_MAX_IPV4_ADDR_LEN];
#define OTB_HTTPD_HTTP_SVR_MAX_LEN 10
  char http_svr[OTB_HTTPD_HTTP_SVR_MAX_LEN] ; //enabled or disabled
#define OTB_HTTPD_IP_CONFIG_MAX_LEN 8  
  char ip_config[OTB_HTTPD_IP_CONFIG_MAX_LEN]; // dhcp or manual
#define OTB_HTTPD_ENABLE_AP_MAX_LEN 4
  char enable_ap[OTB_HTTPD_ENABLE_AP_MAX_LEN]; // yes
  sint16 mqtt_svr_len;
  sint16 mqtt_port_len;
  sint16 mqtt_user_len;
  sint16 mqtt_pass_len;
  sint16 domain_name_len;
  sint16 ip_address_len;
  sint16 netmask_len;
  sint16 gateway_len;
  sint16 dns1_len;
  sint16 dns2_len;
  sint16 http_svr_len;
  sint16 ip_config_len;
  sint16 enable_ap_len;

  ENTRY;

  OTB_ASSERT(buf != NULL);

  switch (method)
  {
    // In the HEAD case still build the entire body - this won't be returned
    // but is necessary to return the correct Content-Length
    case OTB_HTTPD_METHOD_HEAD:
    case OTB_HTTPD_METHOD_GET:
      MDETAIL("GET station config");
      rsp_len += otb_httpd_wifi_form(buf, len);
      break;

    case OTB_HTTPD_METHOD_POST:
      MDETAIL("POST station config");
      OTB_ASSERT(request->post.data != NULL);
      MDEBUG("POST processing: %s", request->post.data);
      wifi_rc = OTB_CONF_RC_NOT_CHANGED;
      mqtt_svr_rc = OTB_CONF_RC_NOT_CHANGED;
      mqtt_user_rc = OTB_CONF_RC_NOT_CHANGED;

      ssid[0] = 0;
      password[0] = 0;
      mqtt_svr[0] = 0;
      mqtt_port[0] = 0;
      mqtt_user[0] = 0;
      mqtt_pass[0] = 0;  
      domain_name[0] = 0;
      ip_address[0] =0;
      netmask[0] = 0;
      gateway[0] = 0;
      dns1[0] = 0;
      dns2[0] = 0;
      enable_ap[0] = 0;
      http_svr[0] = 0;
      ip_config[0] = 0;
      ssid_len = otb_httpd_get_arg(request->post.data, "ssid", ssid, 32);
      password_len = otb_httpd_get_arg(request->post.data, "password", password, 64);
      mqtt_svr_len = otb_httpd_get_arg(request->post.data, "mqtt_svr", mqtt_svr, OTB_MQTT_MAX_SVR_LEN);
      mqtt_port_len = otb_httpd_get_arg(request->post.data, "mqtt_port", mqtt_port, OTB_MQTT_MAX_SVR_LEN);
      mqtt_user_len = otb_httpd_get_arg(request->post.data, "mqtt_user", mqtt_user, OTB_MQTT_MAX_USER_LEN);
      mqtt_pass_len = otb_httpd_get_arg(request->post.data, "mqtt_pass", mqtt_pass, OTB_MQTT_MAX_PASS_LEN);
      domain_name_len = otb_httpd_get_arg(request->post.data, "domain_name", domain_name, OTB_IP_MAX_DOMAIN_NAME_LEN);
      ip_address_len = otb_httpd_get_arg(request->post.data, "ip_addr", ip_address, OTB_IP_MAX_IPV4_ADDR_LEN);
      netmask_len = otb_httpd_get_arg(request->post.data, "subnet", netmask, OTB_IP_MAX_IPV4_ADDR_LEN);
      gateway_len = otb_httpd_get_arg(request->post.data, "gateway", gateway, OTB_IP_MAX_IPV4_ADDR_LEN);
      dns1_len = otb_httpd_get_arg(request->post.data, "dns1", dns1, OTB_IP_MAX_IPV4_ADDR_LEN);
      dns2_len = otb_httpd_get_arg(request->post.data, "dns2", dns2, OTB_IP_MAX_IPV4_ADDR_LEN);
      http_svr_len = otb_httpd_get_arg(request->post.data, "http_svr", http_svr, OTB_HTTPD_HTTP_SVR_MAX_LEN);
      ip_config_len = otb_httpd_get_arg(request->post.data, "ip_config", ip_config, OTB_HTTPD_IP_CONFIG_MAX_LEN);
      enable_ap_len = otb_httpd_get_arg(request->post.data, "enable_ap", enable_ap, OTB_HTTPD_ENABLE_AP_MAX_LEN);
      
      // NULL terminate everything just in case
      ssid[31] = 0;
      password[63] = 0;
      mqtt_svr[OTB_MQTT_MAX_SVR_LEN-1] = 0;
      mqtt_port[OTB_MQTT_MAX_SVR_LEN-1] = 0;
      mqtt_user[OTB_MQTT_MAX_USER_LEN-1] = 0;
      mqtt_pass[OTB_MQTT_MAX_PASS_LEN-1] = 0;
      domain_name[OTB_IP_MAX_DOMAIN_NAME_LEN-1] = 0;
      ip_address[OTB_IP_MAX_IPV4_ADDR_LEN-1] =0;
      netmask[OTB_IP_MAX_IPV4_ADDR_LEN-1] = 0;
      gateway[OTB_IP_MAX_IPV4_ADDR_LEN-1] = 0;
      dns1[OTB_IP_MAX_IPV4_ADDR_LEN-1] = 0;
      dns2[OTB_IP_MAX_IPV4_ADDR_LEN-1] = 0;
      ip_config[OTB_HTTPD_IP_CONFIG_MAX_LEN-1] = 0;
      http_svr[OTB_HTTPD_HTTP_SVR_MAX_LEN-1] = 0;
      enable_ap[OTB_HTTPD_ENABLE_AP_MAX_LEN-1] = 0;
      
      MDETAIL("ssid: %s", ssid);
      MDETAIL("password: %s", password);
      MDETAIL("mqtt_svr: %s", mqtt_svr);
      MDETAIL("mqtt_port: %s", mqtt_port);
      MDETAIL("mqtt_user: %s", mqtt_user);
      MDETAIL("mqtt_pass: %s", mqtt_pass);
      MDETAIL("domain_name: %s", domain_name);
      MDETAIL("ip_addr: %s", ip_address);
      MDETAIL("subnet: %s", netmask);
      MDETAIL("gateway: %s", gateway);
      MDETAIL("dns1: %s", dns1);
      MDETAIL("dns2: %s", dns2);
      MDETAIL("http_svr: %s", http_svr);
      MDETAIL("ip_config: %s", ip_config);
      MDETAIL("enable_ap: %s", enable_ap);

      MDEBUG("Add body");
        rsp_len += os_snprintf(buf + rsp_len,
                               len - rsp_len,
                               "<body>");


      if ((ssid_len > 0) && (password_len >= 0))
      {
        MDETAIL("Valid SSID and password");
        if (os_strcmp(password, "********"))
        {
          wifi_rc = otb_wifi_set_station_config(ssid, password, FALSE);
        }
        else
        {
          // Use old password
          wifi_rc = otb_wifi_set_station_config(ssid, otb_conf->password, FALSE);
        }
        if (wifi_rc == OTB_CONF_RC_CHANGED)
        {
          conf_changed = TRUE;
          rsp_len += os_snprintf(buf + rsp_len,
                                len - rsp_len,
                                "<p>Wifi credentials changed to SSID: %s Password: %s</p>",
                                ssid,
                                "********");
        }
        else
        {
          rsp_len += os_snprintf(buf + rsp_len,
                                len - rsp_len,
                                "<p>Wifi details not changed</p>");
        }
      }
      
      if ((mqtt_svr_len > 0) && (mqtt_port_len > 0))
      {
        mqtt_svr_rc = otb_mqtt_set_svr(mqtt_svr, mqtt_port, FALSE);
        if (mqtt_svr_rc == OTB_CONF_RC_CHANGED)
        {
          conf_changed = TRUE;
          rsp_len += os_snprintf(buf + rsp_len,
                                len - rsp_len,
                                "<p>MQTT server changed to: %s</p>",
                                mqtt_svr);
        }
        else
        {
          rsp_len += os_snprintf(buf + rsp_len,
                                len - rsp_len,
                                "<p>MQTT server not changed</p>");
        }
      }
      
      if ((mqtt_user_len > 0) && (mqtt_pass_len >= 0))
      {
        mqtt_user_rc = otb_mqtt_set_user(mqtt_user, mqtt_pass, FALSE);
        if (mqtt_user_rc == OTB_CONF_RC_CHANGED)
        {
          conf_changed = TRUE;
          rsp_len += os_snprintf(buf + rsp_len,
                                len - rsp_len,
                                "<p>MQTT username/password changed to "
                                "Username: %s Password: %s</p>",
                                mqtt_user,
                                "********");
        }
        else
        {
          rsp_len += os_snprintf(buf + rsp_len,
                                len - rsp_len,
                                "<p>MQTT username/password not changed</p>");
        }
      }
      
      if (domain_name_len > 0)
      {
        MDETAIL("Process domain name: %s", domain_name);
        if (os_strcmp(otb_conf->ip.domain_name, domain_name))
        {
          os_strcpy(otb_conf->ip.domain_name, domain_name);
          conf_changed = TRUE;
          rsp_len += os_snprintf(buf + rsp_len,
                                len - rsp_len,
                                "<p>Domain name changed to: %s</p>",
                                domain_name);
        }
      }

      if (ip_address_len > 0)
      {
        MDETAIL("Check IP address: %s", ip_address);
        if (otb_util_parse_ipv4_str(ip_address, ip) &&
            !otb_util_ip_is_all_val(ip, 0) &&
            !otb_util_ip_is_all_val(ip, 0xff))
        {
          if (os_memcmp(otb_conf->ip.ipv4, ip, 4))
          {
            os_memcpy(otb_conf->ip.ipv4, ip, 4);
            conf_changed = TRUE;
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>IP address changed to: %s</p>",
                                  ip_address);
          }
        }
        else
        {
          // Only log a problem if it's changed (i.e. not all zeros)
          if (os_memcmp(otb_conf->ip.ipv4, ip, 4))
          {
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>Invalid IP address: %s</p>",
                                  ip_address);
          }
        }
      }

      if (netmask_len > 0)
      {
        MDETAIL("Check netmask: %s", netmask);
        if (otb_util_parse_ipv4_str(netmask, ip) &&
            otb_util_ip_is_subnet_valid(ip))
        {
          if (os_memcmp(otb_conf->ip.ipv4_subnet, ip, 4))
          {
            os_memcpy(otb_conf->ip.ipv4_subnet, ip, 4);
            conf_changed = TRUE;
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>Netmask changed to: %s</p>",
                                  netmask);
          }
        }
        else
        {
          // Only log a problem if it's changed (i.e. not all zeros)
          if (os_memcmp(otb_conf->ip.ipv4_subnet, ip, 4))
          {
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>Invalid subnet mask: %s</p>",
                                  netmask);
          }
        }
      }

      if (gateway_len > 0)
      {
        MDETAIL("Check gateway: %s", gateway);
        // 0.0.0.0 is OK for default gateway
        if (otb_util_parse_ipv4_str(gateway, ip) &&
            !otb_util_ip_is_all_val(ip, 0xff))
        {
          if (os_memcmp(otb_conf->ip.gateway, ip, 4))
          {
            os_memcpy(otb_conf->ip.gateway, ip, 4);
            conf_changed = TRUE;
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>Gateway changed to: %s</p>",
                                  gateway);
          }
        }
        else
        {
          rsp_len += os_snprintf(buf + rsp_len,
                                len - rsp_len,
                                "<p>Invalid gateway: %s</p>",
                                gateway);
        }
      }

      if (dns1_len > 0)
      {
        MDETAIL("Check dns1: %s", dns1);
        // 0.0.0.0 is OK for DNS
        if (otb_util_parse_ipv4_str(dns1, ip) &&
            !otb_util_ip_is_all_val(ip, 0xff))
        {
          if (os_memcmp(otb_conf->ip.dns1, ip, 4))
          {
            os_memcpy(otb_conf->ip.dns1, ip, 4);
            conf_changed = TRUE;
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>DNS server 1 changed to: %s</p>",
                                  dns1);
          }
        }
        else
        {
          rsp_len += os_snprintf(buf + rsp_len,
                                len - rsp_len,
                                "<p>Invalid DNS server 1: %s</p>",
                                dns1);
        }
      }

      if (dns2_len > 0)
      {
        MDETAIL("Check dns2: %s", dns2);
        // 0.0.0.0 is OK for DNS
        if (otb_util_parse_ipv4_str(dns2, ip) &&
            !otb_util_ip_is_all_val(ip, 0xff))
        {
          if (os_memcmp(otb_conf->ip.dns2, ip, 4))
          {
            os_memcpy(otb_conf->ip.dns2, ip, 4);
            conf_changed = TRUE;
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>DNS server 2 changed to: %s</p>",
                                  dns2);
          }
        }
        else
        {
          rsp_len += os_snprintf(buf + rsp_len,
                                len - rsp_len,
                                "<p>Invalid DNS server 1: %s</p>",
                                dns2);
        }
      }

      if (enable_ap_len > 0)
      {
        MDETAIL("Enable AP: %s", enable_ap);
        if (!os_strcmp(enable_ap, "yes"))
        {
          if (!otb_conf->keep_ap_active)
          {
            otb_conf->keep_ap_active = 1;
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>Enabled AP when MQTT connected</p>");
            conf_changed = TRUE;
          }
        }
        else if (!os_strcmp(enable_ap, "no"))
        {
          if (otb_conf->keep_ap_active)
          {
            otb_conf->keep_ap_active = 0;
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>Disable AP when MQTT connected</p>");
            conf_changed = TRUE;
          }
        }
      }

      if (http_svr_len > 0)
      {
        MDETAIL("HTTP server: %s", http_svr);
        if (!os_strcmp(http_svr, "enabled"))
        {
          if (!otb_conf->mqtt_httpd)
          {
            otb_conf->mqtt_httpd = TRUE;
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>HTTP server to expose MQTT API enabled</p>");
            conf_changed = TRUE;
          }
        }
        else if (!os_strcmp(http_svr, "disabled"))
        {
          if (otb_conf->mqtt_httpd)
          {
            otb_conf->mqtt_httpd = FALSE;
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>HTTP server to expose MQTT API disabled</p>");
            conf_changed = TRUE;
          }
        }
      }

      if (ip_config_len > 0)
      {
        MDETAIL("Check ip config %s", ip_config);
        if (!os_strcmp(ip_config, "dhcp"))
        {
          if (otb_conf->ip.manual != OTB_IP_DHCP_DHCP)
          {
            otb_conf->ip.manual = OTB_IP_DHCP_DHCP;
            rsp_len += os_snprintf(buf + rsp_len,
                                  len - rsp_len,
                                  "<p>Using DHCP for IP addressing</p>");
            conf_changed = TRUE;
          }
        }
        else if (!os_strcmp(ip_config, "manual"))
        {
          if (otb_conf->ip.manual != OTB_IP_DHCP_MANUAL)
          {
            if (otb_conf_verify_manual_ip(otb_conf))
            {
              otb_conf->ip.manual = OTB_IP_DHCP_MANUAL;
              rsp_len += os_snprintf(buf + rsp_len,
                                    len - rsp_len,
                                    "<p>Using manual IP addressing</p>");
              conf_changed = TRUE;
            }
            else
            {
              rsp_len += os_snprintf(buf + rsp_len,
                                    len - rsp_len,
                                    "<p>Can't use manual IP addressing - no valid IP address information configurated</p>");
            }
          }
        }
      }

      MDEBUG("Update config");

      if (conf_changed)
      {
        conf_rc = otb_conf_update(otb_conf);
        if (!conf_rc)
        {
        rsp_len += os_snprintf(buf + rsp_len,
                               len - rsp_len,
                               "<p><b>Failed to commit new config to flash!</b></p>");
          MERROR("Failed to update config");
        }
        // Will cause device to reboot
        rsp_len += os_snprintf(buf + rsp_len,
                               len - rsp_len,
                               "<p>Rebooting shortly ... </p>");
                           
        MDETAIL("Terminate AP mode");
        otb_wifi_ap_mode_done_fn();          
      }
      else
      {
        rsp_len += os_snprintf(buf + rsp_len,
                               len - rsp_len,
                               "<p>No changes made</p>");
      }
      

      MDEBUG("Finish body");

      rsp_len += os_snprintf(buf + rsp_len,
                             len - rsp_len,
                             "</body>");

      break;

    default:
      MERROR("Invalid method passed to otb_httpd_station_config");
      OTB_ASSERT(false);
      break;
  }

EXIT_LABEL:
  
  EXIT;

  return(rsp_len);
}

int ICACHE_FLASH_ATTR otb_httpd_wifi_form(char *buffer, uint16_t buf_len)
{
  uint16_t output_len;
  unsigned char *not_checked = "";
  unsigned char *checked = "checked=\"true\"";
  unsigned char *form_str, *form_str_orig;
  char ip[OTB_IP_MAX_IPV4_ADDR_LEN];
  char netmask[OTB_IP_MAX_IPV4_ADDR_LEN];
  char gateway[OTB_IP_MAX_IPV4_ADDR_LEN];
  char dns1[OTB_IP_MAX_IPV4_ADDR_LEN];
  char dns2[OTB_IP_MAX_IPV4_ADDR_LEN];
  int str_ii;

  ENTRY;

  output_len = 0;

  OTB_IPV4_SNPRINTF(ip, otb_conf->ip.ipv4);
  OTB_IPV4_SNPRINTF(netmask, otb_conf->ip.ipv4_subnet);
  OTB_IPV4_SNPRINTF(gateway, otb_conf->ip.gateway);
  OTB_IPV4_SNPRINTF(dns1, otb_conf->ip.dns1);
  OTB_IPV4_SNPRINTF(dns2, otb_conf->ip.dns2);

  // Some complicated code to pull the form out of the eeprom and put it in
  // 4 byte aligned memory rather than doing operations on it that might not
  // be 4 byte aligned!
  size_t str_size;
  str_size = sizeof(otb_httpd_wifi_form_str)/sizeof(otb_httpd_wifi_form_str[0]);
  form_str_orig = (unsigned char *)os_malloc(str_size + 4);
  if (form_str_orig != NULL)
  {
    form_str = form_str_orig + (4 - (((uint32_t)form_str_orig)%4));
    for (str_ii = 0; str_ii < ((str_size/4)+1); str_ii++)
    {
      *(((uint32_t*)(form_str))+str_ii) = *(((uint32_t*)(&otb_httpd_wifi_form_str))+str_ii);
    }
    output_len += os_snprintf(buffer + output_len,
                              buf_len - output_len,
                              form_str,
                              otb_conf->ssid,
                              otb_conf->mqtt.svr,
                              otb_conf->mqtt.port,
                              otb_conf->mqtt.user,
                              otb_conf->mqtt.pass,
                              otb_conf->keep_ap_active ? checked : not_checked,
                              otb_conf->keep_ap_active ? not_checked : checked,
                              otb_conf->mqtt_httpd ? checked : not_checked,
                              otb_conf->mqtt_httpd ? not_checked : checked,
                              otb_conf->ip.domain_name,
                              otb_conf->ip.manual == OTB_IP_DHCP_DHCP ? checked : not_checked,
                              otb_conf->ip.manual == OTB_IP_DHCP_MANUAL ? checked : not_checked,
                              ip,
                              netmask,
                              gateway,
                              dns1,
                              dns2);
    os_free(form_str_orig);
  }
  else
  {
    output_len += os_snprintf(buffer + output_len,
                              buf_len - output_len,
                              "Internal error");
  }

  EXIT;

  return output_len;
}

int ICACHE_FLASH_ATTR otb_httpd_display_ap_list(char *buffer, uint16_t buf_len)
{
  uint16_t output_len;
  struct otb_wifi_ap *ap;
  
  ENTRY;

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
  
  EXIT;

  return(output_len);
}
