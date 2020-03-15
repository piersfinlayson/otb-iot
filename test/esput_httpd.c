#include "otb.h"

otb_conf_struct *otb_conf;
bool otb_mqtt_connected;
struct otb_wifi_ap_list otb_wifi_ap_list_struct;

static char *test_name = "unknown";

sint8 esput_espconn_tcp_set_max_con_allow(struct espconn *espconn, uint8 num)
{
  return 0;
}

sint8 esput_espconn_accept(struct espconn *espconn)
{
  return 0;
}

sint8 esput_espconn_disconnect(struct espconn *espconn)
{
  return 0;
}

sint8 esput_espconn_send(struct espconn *espconn, uint8 *psent, uint16 length)
{
  esput_sent_msg = malloc(length);
  assert(esput_sent_msg != NULL);
  memcpy(esput_sent_msg, psent, length);
  return 0;
}

sint8 esput_espconn_regist_connectcb(struct espconn *espconn, espconn_connect_callback connect_cb)
{
  otb_httpd_espconn = espconn;
  otb_httpd_connect_cb = connect_cb;
  return 0;
}

sint8 esput_espconn_regist_recvcb(struct espconn *espconn, espconn_recv_callback recv_cb)
{
  ESPUT_ASSERT(espconn == otb_httpd_espconn);
  otb_httpd_recv_cb = recv_cb;
  return 0;
}

sint8 esput_espconn_regist_reconcb(struct espconn *espconn, espconn_reconnect_callback recon_cb)
{
  ESPUT_ASSERT(espconn == otb_httpd_espconn);
  otb_httpd_recon_cb = recon_cb;
  return 0;
}

sint8 esput_espconn_regist_disconcb(struct espconn *espconn, espconn_connect_callback discon_cb)
{
  ESPUT_ASSERT(espconn == otb_httpd_espconn);
  otb_httpd_discon_cb = discon_cb;
  return 0;
}

sint8 esput_espconn_regist_sentcb(struct espconn *espconn, espconn_sent_callback sent_cb)
{
  ESPUT_ASSERT(espconn == otb_httpd_espconn);
  otb_httpd_sent_cb = sent_cb;
  return 0;
}

void esput_captdnsInit(void)
{
  return;
}

void esput_captdnsTerm(void)
{
  return;
}

void esput_otb_mqtt_publish(MQTT_Client *mqtt_client,
                            char *subtopic,
                            char *extra_subtopic,
                            char *message,
                            char *extra_message,
                            uint8_t qos,
                            bool retain,
                            char *buf,
                            uint16_t buf_len)
{
  return;
}

void esput_otb_cmd_mqtt_receive(uint32_t *client,
                                const char* topic,
                                uint32_t topic_len,
                                const char *msg,
                                uint32_t msg_len,
                                char *buf,
                                uint16_t buf_len)
{
  return;
}

uint8 esput_otb_wifi_set_station_config(char *ssid, char *password, bool commit)
{
  return 0;
}

void esput_otb_wifi_ap_mode_done_fn(void)
{
  return;
}

uint8 esput_otb_mqtt_set_svr(char *svr, char *port, bool commit)
{
  return 0;
}

uint8 esput_otb_mqtt_set_user(char *user, char *pass, bool commit)
{
  return 0;
}

bool esput_otb_conf_verify_manual_ip(otb_conf_struct *conf)
{
  return FALSE;
}

bool esput_otb_conf_update(otb_conf_struct *conf)
{
  return FALSE;
}

bool esput_otb_util_parse_ipv4_str(char *ip_in, uint8_t *ip_out)
{
  return FALSE;
}

bool esput_otb_util_ip_is_all_val(uint8_t *ip, uint8_t val)
{
  return FALSE;
}

bool esput_otb_util_ip_is_subnet_valid(uint8_t *subnet)
{
  return FALSE;
}
