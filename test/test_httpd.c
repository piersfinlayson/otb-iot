#include "otb.h"

int main(int argc, char *argv[])
{
  LOG("Test 1: Start HTTPD without captive DNS, and then stop");
  ESPUT_ASSERT(otb_httpd_start(FALSE));
  ESPUT_ASSERT(otb_httpd_started);
  otb_httpd_stop;

  LOG("Test 2: Start HTTPD with captive DNS, and then stop");
  ESPUT_ASSERT(otb_httpd_start(TRUE));
  ESPUT_ASSERT(otb_httpd_started);
  otb_httpd_stop;

  LOG("Tests complete");

  return 0;
}