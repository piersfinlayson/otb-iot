#include "esput.h"

char otb_mqtt_string_slash[] = "/";
bool otb_cmd_ut_cmd_handler_rc = FALSE;
bool esput_assertion_failed = FALSE;
bool esput_debug = FALSE;

void otb_cmd_mqtt_receive(uint32_t *client,
                          const char* topic,
                          uint32_t topic_len,
                          const char *msg,
                          uint32_t msg_len);

unsigned char *otb_mqtt_send_status_val1;
unsigned char *otb_mqtt_send_status_val2;
unsigned char *otb_mqtt_send_status_val3;
unsigned char *otb_mqtt_send_status_val4;
void otb_mqtt_send_status(char *val1,
                          char *val2,
                          char *val3,
                          char *val4)
{
  esput_printf("Received function call", "otb_mqtt_send_status\n");
  esput_printf("                    ", "%s\n", val1);
  esput_printf("                    ", "%s\n", val2);
  esput_printf("                    ", "%s\n", val3);
  esput_printf("                    ", "%s\n", val4);
  otb_mqtt_send_status_val1 = malloc(strlen(val1)+1);
  strcpy(otb_mqtt_send_status_val1, val1);
  otb_mqtt_send_status_val2 = malloc(strlen(val2)+1);
  strcpy(otb_mqtt_send_status_val2, val2);
  otb_mqtt_send_status_val3 = malloc(strlen(val3)+1);
  strcpy(otb_mqtt_send_status_val3, val3);
  otb_mqtt_send_status_val4 = malloc(strlen(val4)+1);
  strcpy(otb_mqtt_send_status_val4, val4);
}

ESPUT_TEST_START(test1, "trigger->ping")
{
  char topic[] = "/otb-iot/abcdef";
  char msg[] = "/trigger/ping";
  esput_printf("Making function call", "otb_cmd_mqtt_receive\n");
  otb_cmd_mqtt_receive(NULL, topic, strlen(topic), msg, strlen(msg));
  ESPUT_TEST_FOR_ASSERTIONS();
  if (!os_strcmp(otb_mqtt_send_status_val1, "ok")) passed = TRUE;
}
ESPUT_TEST_FINISH

ESPUT_TEST_START(test2, "Call with invalid message and topic")
{
  char topic[] = "/invalid";
  char msg[] = "/invalid";
  esput_printf("Making function call", "otb_cmd_mqtt_receive\n");
  otb_cmd_mqtt_receive(NULL, topic, strlen(topic), msg, strlen(msg));
  ESPUT_TEST_FOR_ASSERTIONS();
  if (!os_strcmp(otb_mqtt_send_status_val1, "error")) passed = TRUE;
}
ESPUT_TEST_FINISH

ESPUT_TEST_START(test3, "Call with too long a topic segment")
{
  char topic[] = "/topic1/verylongtopicmorethan16chars";
  char msg[] = "/message";
  esput_printf("Making function call", "otb_cmd_mqtt_receive\n");
  otb_cmd_mqtt_receive(NULL, topic, strlen(topic), msg, strlen(msg)); 
  ESPUT_TEST_FOR_ASSERTIONS();
  if (!os_strcmp(otb_mqtt_send_status_val1, "error")) passed = TRUE;
}
ESPUT_TEST_FINISH

ESPUT_TEST_START(test4, "Call with too long a message segment")
{
  char topic[] = "/topic1/topic2";
  char msg[] = "/message16charsxx";
  esput_printf("Making function call", "otb_cmd_mqtt_receive\n");
  ESPUT_TEST_FOR_ASSERTIONS();
  otb_cmd_mqtt_receive(NULL, topic, strlen(topic), msg, strlen(msg)); 
  if (!os_strcmp(otb_mqtt_send_status_val1, "error")) passed = TRUE;
}
ESPUT_TEST_FINISH

  
// Stub out the various calls made by otb_cmd
#define OTB_CMD_UT_CMD_HANDLER_IMP(OTB_CMD_UT_CMD_HANDLER_FN)       \
bool OTB_CMD_UT_CMD_HANDLER_FN(unsigned char *next_cmd, void *arg)  \
{                                                                   \
  printf("Function: %s\n", "OTB_CMD_UT_CMD_HANDLER_FN");            \
  printf("next_cmd: %s\n", next_cmd);                               \
  printf("     arg: 0x%p\n", arg);                                  \
  return(otb_cmd_ut_cmd_handler_rc);                                \
}
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_trigger_update)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_trigger_reset)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_trigger_ping)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_set_boot_slot)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_get_logs_flash)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_get_logs_ram)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_get_reason_reboot)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_get_version)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_get_rssi)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_get_heap_size)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_get_config_all)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_get_sensor_adc_ads)
OTB_CMD_UT_CMD_HANDLER_IMP(otb_cmd_get_temp_ds1b20)
// XXX Instead of get_chip_id we need to set up OTB_MAIN_CHIPID, version_id, provide boot_slot get function, compile_date, compile_time, version_id, hw_info

int esput_printf(const char *string, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  if (strlen(string) > 0)
  {
    printf("    %s: ", string);
  }
  return vprintf(format, args);
}

void main(int argc, char *argv[])
{
  esput_printf("", "otb_cmd UTs\n");
  esput_printf("", "-----------\n");

  test1();
  test2();
  test3();
  test4();

}

