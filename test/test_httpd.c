#include "otb.h"

extern 
void test1(void)
{
  LOG("Test 1: Start HTTPD without captive DNS, and then stop");
  ESPUT_ASSERT(otb_httpd_start(FALSE));
  otb_httpd_stop;
}

void test2(void)
{
  LOG("Test 2: Start HTTPD with captive DNS, and then stop");
  ESPUT_ASSERT(otb_httpd_start(TRUE));
  otb_httpd_stop;
}

void test3(void)
{
  struct test3_data
  {
    char *data;
    uint16 method_len;
    uint8 method;
    uint16 status_code;
    char *status_str;
  };
  struct test3_data data[] =
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
  struct test3_data *tdata;
  uint16 len;
  uint16 rc;

  LOG("Test 3: Check correct method selected ...");

  for (tdata = data+0; tdata->data != NULL; tdata++)
  {
    LOG("HTTP message: %s", tdata->data);
    memset(&hconn, 0, sizeof(hconn));
    rc = otb_httpd_process_method(&hconn, tdata->data, strlen(tdata->data));
    ESPUT_ASSERT(rc == tdata->method_len);
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
}

int main(int argc, char *argv[])
{
  test1();
  test2();
  test3();

  LOG("Tests complete");

  return 0;
}