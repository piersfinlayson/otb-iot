#include "otb.h"

bool test1(char *test_name)
{
  ESPUT_ASSERT(otb_httpd_start(FALSE));
  otb_httpd_stop;
  return TRUE;
}

bool test2(char *test_name)
{
  ESPUT_ASSERT(otb_httpd_start(TRUE));
  otb_httpd_stop;
  return TRUE;
}

bool test3(char *test_name)
{
  test_httpd_data data[] =
  {
    {"HEAD blah", 5, OTB_HTTPD_METHOD_HEAD, 0, NULL},
    {"GET blah", 4, OTB_HTTPD_METHOD_GET, 0, NULL},
    {"POST blah", 5, OTB_HTTPD_METHOD_POST, 0, NULL},
    {"PUT blah", 0, OTB_HTTPD_METHOD_NONE, 405, "Method Not Allowed"},
    {"HEAD ", 5, OTB_HTTPD_METHOD_HEAD, 0, NULL},
    {"HEAD", 5, OTB_HTTPD_METHOD_HEAD, 0, NULL},
    {NULL, 0, 0, 0},
  };
  otb_httpd_connection hconn;
  struct test_httpd_data *tdata;
  uint16 len;
  uint16 rc;

  for (tdata = data; tdata->data != NULL; tdata++)
  {
    LOG("HTTP message: %s", tdata->data);
    memset(&hconn, 0, sizeof(hconn));
    rc = otb_httpd_process_method(&hconn, tdata->data, strlen(tdata->data));
    ESPUT_ASSERT(rc == tdata->len);
    ESPUT_ASSERT(hconn.request.method == tdata->method);
    ESPUT_ASSERT(hconn.request.status_code == tdata->status_code);
    if (tdata->status_str == NULL)
    {
      ESPUT_ASSERT(hconn.request.status_str == NULL)
    }
    else
    {
      ESPUT_ASSERT(!strcmp(hconn.request.status_str, tdata->status_str));
    }
  }
  return TRUE;
}

struct espconn *otb_httpd_espconn;
espconn_connect_callback otb_httpd_connect_cb;
espconn_recv_callback otb_httpd_recv_cb;
espconn_reconnect_callback otb_httpd_recon_cb;
espconn_connect_callback otb_httpd_discon_cb;
espconn_sent_callback otb_httpd_sent_cb;
uint8 *esput_sent_msg;
bool test4(char *test_name)
{
  struct otb_httpd_connection *hconn;
  test_httpd_data *tdata;
  otb_conf_struct conf;
  
  // Set up otb_conf (is used in otb_httpd_base_handler)
  memset(&conf, 0, sizeof(conf));
  otb_conf = &conf;
  otb_mqtt_root = "espi";

  LOG("Start HTTPD");
  // Start HTTPD
  otb_httpd_espconn = NULL;
  otb_httpd_connect_cb = NULL;
  ESPUT_ASSERT(otb_httpd_start(FALSE));
  ESPUT_ASSERT(otb_httpd_espconn != NULL);
  ESPUT_ASSERT(otb_httpd_connect_cb != NULL);

  test_httpd_data data[] =
  {
    {"GET / HTTP/1.0", 3, OTB_HTTPD_METHOD_GET, 200, "HTTP/1.0 200 OK"},
    {"GET / HTTP/1.0 ", 3, OTB_HTTPD_METHOD_GET, 200, "HTTP/1.0 200 OK"},
    {"GET / HTTP/1.0 \r\n\r\n", 3, OTB_HTTPD_METHOD_GET, 200, "HTTP/1.0 200 OK"},
    {"GET / HTTP/1.0 \r\n\r\nServer: esput", 3, OTB_HTTPD_METHOD_GET, 200, "HTTP/1.0 200 OK"},
    {"GET / HTTP/1.0 \r\n\r\nServer: esput\r\n\r\n", 3, OTB_HTTPD_METHOD_GET, 200, "HTTP/1.0 200 OK"},
    {"HEAD / HTTP/1.0", 3, OTB_HTTPD_METHOD_GET, 200, "HTTP/1.0 200 OK"},
    {NULL, 0, 0, 0},
  };
  
  for (tdata = data; tdata->data != NULL; tdata++)
  {
    // Create a connection
    otb_httpd_recv_cb = NULL;
    otb_httpd_recon_cb = NULL;
    otb_httpd_discon_cb = NULL;
    otb_httpd_sent_cb = NULL;
    otb_httpd_espconn->proto.tcp->remote_ip[0] = 192;
    otb_httpd_espconn->proto.tcp->remote_ip[1] = 168;
    otb_httpd_espconn->proto.tcp->remote_ip[2] = 0;
    otb_httpd_espconn->proto.tcp->remote_ip[3] = 1;
    otb_httpd_espconn->proto.tcp->remote_port = 8080;
    otb_httpd_connect_callback(otb_httpd_espconn);
    ESPUT_ASSERT(otb_httpd_recv_cb != NULL);
    ESPUT_ASSERT(otb_httpd_recon_cb != NULL);
    ESPUT_ASSERT(otb_httpd_discon_cb != NULL);
    ESPUT_ASSERT(otb_httpd_sent_cb != NULL);
    hconn = (otb_httpd_connection *)(otb_httpd_espconn->reverse);
    ESPUT_ASSERT(hconn != NULL);
    ESPUT_ASSERT(hconn->remote_ip[0] == 192);
    ESPUT_ASSERT(hconn->remote_ip[1] == 168);
    ESPUT_ASSERT(hconn->remote_ip[2] == 0);
    ESPUT_ASSERT(hconn->remote_ip[3] == 1);
    ESPUT_ASSERT(hconn->remote_port == 8080);

    esput_sent_msg = NULL;
    otb_httpd_recv_cb(otb_httpd_espconn, tdata->data, strlen(tdata->data));
    // We can't check the hconn->request as the recv_cb function clears it
    // after sending
    ESPUT_ASSERT(esput_sent_msg != NULL);
    ESPUT_ASSERT(!memcmp(tdata->status_str, esput_sent_msg, strlen(tdata->status_str)));
    free(esput_sent_msg);
  }

  LOG("Stop HTTPD");
  otb_httpd_stop();
  
  return TRUE;
}

esput_test esput_tests[] =
{
  {test1, "Test 1", "Start HTTPD without captive DNS, and then stop"},
  {test2, "Test 2", "Start HTTPD with captive DNS, and then stopp"},
  {test3, "Test 3", "Check correct method selected"},
  {test4, "Test 4", "Test / URL handling, with base handling enabled"},
  {NULL, NULL, NULL},
};
