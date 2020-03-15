void esput_otb_mqtt_publish(MQTT_Client *mqtt_client,
                            char *subtopic,
                            char *extra_subtopic,
                            char *message,
                            char *extra_message,
                            uint8_t qos,
                            bool retain,
                            char *buf,
                            uint16_t buf_len);
void esput_otb_cmd_mqtt_receive(uint32_t *client,
                                const char* topic,
                                uint32_t topic_len,
                                const char *msg,
                                uint32_t msg_len,
                                char *buf,
                                uint16_t buf_len);
uint8 esput_otb_wifi_set_station_config(char *ssid, char *password, bool commit);
void esput_otb_wifi_ap_mode_done_fn(void);
uint8 esput_otb_mqtt_set_svr(char *svr, char *port, bool commit);
uint8 esput_otb_mqtt_set_user(char *user, char *pass, bool commit);
bool esput_otb_conf_verify_manual_ip(otb_conf_struct *conf);
bool esput_otb_conf_update(otb_conf_struct *conf);
bool esput_otb_util_parse_ipv4_str(char *ip_in, uint8_t *ip_out);
bool esput_otb_util_ip_is_all_val(uint8_t *ip, uint8_t val);
bool esput_otb_util_ip_is_subnet_valid(uint8_t *subnet);
#define otb_mqtt_publish(...) esput_otb_mqtt_publish(__VA_ARGS__)
#define otb_cmd_mqtt_receive(...) esput_otb_cmd_mqtt_receive(__VA_ARGS__)
#define otb_wifi_set_station_config(...) esput_otb_wifi_set_station_config(__VA_ARGS__)
#define otb_mqtt_set_svr(...) esput_otb_mqtt_set_svr(__VA_ARGS__)
#define otb_mqtt_set_user(...) esput_otb_mqtt_set_user(__VA_ARGS__)
#define otb_util_parse_ipv4_str(...) esput_otb_util_parse_ipv4_str(__VA_ARGS__)
#define otb_util_ip_is_all_val(...) esput_otb_util_ip_is_all_val(__VA_ARGS__)
#define otb_util_ip_is_subnet_valid(...) esput_otb_util_ip_is_subnet_valid(__VA_ARGS__)
#define otb_conf_verify_manual_ip(...) esput_otb_conf_verify_manual_ip(__VA_ARGS__)
#define otb_conf_update(...) esput_otb_conf_update(__VA_ARGS__)
#define otb_wifi_ap_mode_done_fn(...) esput_otb_wifi_ap_mode_done_fn(__VA_ARGS__)

char *otb_mqtt_root;

void *otb_mqtt_client;

#define OTB_MAIN_CHIPID "chipid"
#define OTB_MAIN_DEVICE_ID "deviceid"

#define OTB_MQTT_STATUS_HTTP "status"

extern bool otb_mqtt_connected;

#ifndef OTB_HTTPD_C
extern struct espconn *otb_httpd_espconn;
extern espconn_connect_callback otb_httpd_connect_cb;
extern espconn_recv_callback otb_httpd_recv_cb;
extern espconn_reconnect_callback otb_httpd_recon_cb;
extern espconn_connect_callback otb_httpd_discon_cb;
extern espconn_sent_callback otb_httpd_sent_cb;
extern uint8 *esput_sent_msg;
#endif // OTB_HTTPD_C

typedef struct test_httpd_data
{
  char *data;
  uint16 len;
  uint8 method;
  uint16 status_code;
  char *status_str;
} test_httpd_data;
