/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2020 Piers Finlayson
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
 */

#define OTB_BREAK_C
#include "otb.h"
#include "brzo_i2c.h"

MLOG("BREAK");

void ICACHE_FLASH_ATTR otb_break_start(void)
{
  ENTRY;

  otb_break_rx_buf_len = 0;
  otb_break_state = OTB_BREAK_STATE_MAIN;
  INFO("Entered otb-iot break processing");
  INFO("Enable watchdog");
  otb_util_break_enable_timer(OTB_BREAK_WATCHDOG_TIMER);
  INFO(" Watchdog enabled: %dms", OTB_BREAK_WATCHDOG_TIMER)
  otb_util_uart0_rx_en();
  INFO("Press h or ? for help");
  // Make sure our INFO logs get output
  otb_break_old_log_level = otb_util_log_level;
  if (otb_util_log_level > OTB_LOG_LEVEL_INFO)
  {
    otb_util_log_level = OTB_LOG_LEVEL_INFO;
  }

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_break_options_output(void)
{
  ENTRY;

  INFO("otb-iot break options:")
  INFO("  c - change config (changes do not persist)");
  INFO("  C - change config (changes persist)");
  INFO("  x - resumes booting (with changed config as applicable)");
  INFO("  q - reboot");
  INFO("  f - factory reset");
  INFO("  g - run gpio test");
  INFO("  r - waits for soft reset button to be pressed");
  INFO("  i - outputs device info");
  INFO("  w - wipe stored config (auto-reboots)");
  INFO("  d - disable watchdog (device will not reboot automatically after 5 minutes)");
  INFO("  e - enable watchdog (device will rebot after 5 minutes)");
  INFO("  h - output this list of options");

  EXIT;

  return;
}  

void ICACHE_FLASH_ATTR otb_break_process_char_timerfunc(void *arg)
{
  char rx_char;

  ENTRY;

  otb_util_timer_cancel((os_timer_t*)&otb_break_process_char_timer);
  OTB_ASSERT(otb_break_rx_buf > 0);
  otb_break_options_fan_out(otb_break_rx_buf[0]);

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_break_options_fan_out(char input)
{
  bool rc = TRUE;

  ENTRY;

  switch (otb_break_state)
  {
    case OTB_BREAK_STATE_MAIN:
      rc = otb_break_options_select(input);
      break;

    case OTB_BREAK_STATE_CONFIG:
      otb_break_config_input(input);
      break;

    case OTB_BREAK_STATE_GPIO:
      otb_break_gpio_input(input);
      break;

    case OTB_BREAK_STATE_SOFT_RESET:
      otb_break_soft_reset_input(input);
      break;

    default:
      OTB_ASSERT(FALSE);
      break;
  }

  if (rc)
  {
    otb_break_rx_buf_len = 0;
  }

  EXIT;

  return;
}

char ALIGN4 otb_break_user_reboot_string[] = "BREAK: User triggered reboot";
char ALIGN4 otb_break_config_reboot_string[] = "BREAK: User triggered config wipe";
bool ICACHE_FLASH_ATTR otb_break_options_select(char option)
{
  bool output_options = FALSE;
  bool clear_buf = TRUE;
  unsigned char mac1[6], mac2[6];
  bool rc;
  uint32_t chipid;

  ENTRY;

  switch (option)
  {
    case 'c':
    case 'C':
      INFO("Change config");
      otb_break_state = OTB_BREAK_STATE_CONFIG;
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
      if (option == 'c')
      {
        INFO(" Changes DO NOT persist");
        otb_break_config_persist = FALSE;
      }
      else
      {
        INFO(" Changes persist");
        otb_break_config_persist = TRUE;
      }
      INFO(" Select config to change");
      break;

    case 'x':
    case 'X':
      INFO("Resume booting");
      otb_util_log_level = otb_break_old_log_level;
      otb_util_uart0_rx_dis();
      otb_util_break_disable_timer();
      otb_wifi_kick_off();
      break;

    case 'q':
    case 'Q':
      INFO("Reboot");
      otb_reset(otb_break_user_reboot_string);
      break;

    case 'f':
    case 'F':
      INFO("Factory reset");
      INFO(" For factory reset reboot and hold down soft reset (GPIO 14) for > 15 seconds");
      break;

    case 'g':
    case 'G':
      INFO("Run GPIO test");
      otb_break_state = OTB_BREAK_STATE_GPIO;
      otb_break_start_gpio_test();
      break;

    case 'r':
    case 'R':
      INFO("Soft reset button test");
      otb_break_state = OTB_BREAK_STATE_SOFT_RESET;
      if (otb_gpio_pins.soft_reset != OTB_GPIO_INVALID_PIN)
      {
        otb_intr_unreg(otb_gpio_pins.soft_reset);
        otb_gpio_set(otb_gpio_pins.soft_reset, 1, TRUE);
        GPIO_DIS_OUTPUT(otb_gpio_pins.soft_reset);
        otb_intr_register(otb_break_reset_button_interrupt, NULL, otb_gpio_pins.soft_reset);
        INFO(" Waiting for soft reset button to be pressed");
        otb_break_state = OTB_BREAK_STATE_SOFT_RESET;
      }
      else
      {
        INFO(" No soft reset pin configured - exiting");
      }
      break;

    case 'i':
    case 'I':
      INFO("Output device info");
      INFO(" SDK Version:    %s", otb_sdk_version_id);
      chipid = system_get_chip_id();
      INFO(" Actual Chip ID: %06x", chipid);
      os_memset(mac1, 0, 6);
      os_memset(mac2, 0, 6);
      if (wifi_get_macaddr(STATION_IF, mac1))
      {
        INFO(" Station MAC:    %02x%02x%02x", mac1[0], mac1[1], mac1[2]);
      }
      if (wifi_get_macaddr(SOFTAP_IF, mac2))
      {
        INFO(" AP MAC:         %02x%02x%02x", mac2[0], mac2[1], mac2[2]);
      }
      INFO(" Eeprom args:    -i %06x -1 %02x%02x%02x -2 %02x%02x%02x", chipid, mac1[0], mac1[1], mac1[2], mac2[0], mac2[1], mac2[2]);
      INFO(" Hardware info:  %s", otb_hw_info);
      INFO(" Boot slot:      %d", otb_rboot_get_slot(FALSE));
      INFO(" Free heap size:  %d bytes", system_get_free_heap_size());
      INFO(" SW Chip ID:      %s", OTB_MAIN_CHIPID);
      INFO(" Device ID:       %s", OTB_MAIN_DEVICE_ID);
      INFO(" Wifi SSID:       %s", otb_conf->ssid);
      INFO(" Wifi Password:   %s", otb_conf->password);
      INFO(" MQTT Server:     %s", otb_conf->mqtt.svr);
      INFO(" MQTT Port:       %d", otb_conf->mqtt.port);
      INFO(" MQTT Username:   %s", otb_conf->mqtt.user);
      INFO(" MQTT Password:   %s", otb_conf->mqtt.pass);
      INFO(" IP domain name:  %s", otb_conf->ip.domain_name);
      if (otb_conf->ip.manual == OTB_IP_DHCP_DHCP)
      {
        INFO(" IP addressing:   DHCP");
      }
      else if (otb_conf->ip.manual == OTB_IP_DHCP_MANUAL)
      {
        INFO(" IP addressing:   Manual");
      }
      INFO(" IP IPv4 address: %d.%d.%d.%d",
              otb_conf->ip.ipv4[0],
              otb_conf->ip.ipv4[1],
              otb_conf->ip.ipv4[2],
              otb_conf->ip.ipv4[3]);
      INFO(" IP IPv4 subnet:  %d.%d.%d.%d",
              otb_conf->ip.ipv4_subnet[0],
              otb_conf->ip.ipv4_subnet[1],
              otb_conf->ip.ipv4_subnet[2],
              otb_conf->ip.ipv4_subnet[3]);
      INFO(" IP IPv4 gateway: %d.%d.%d.%d",
              otb_conf->ip.gateway[0],
              otb_conf->ip.gateway[1],
              otb_conf->ip.gateway[2],
              otb_conf->ip.gateway[3]);
      INFO(" IP DNS server 1: %d.%d.%d.%d",
              otb_conf->ip.dns1[0],
              otb_conf->ip.dns1[1],
              otb_conf->ip.dns1[2],
              otb_conf->ip.dns1[3]);
      INFO(" IP DNS server 2: %d.%d.%d.%d",
              otb_conf->ip.dns2[0],
              otb_conf->ip.dns2[1],
              otb_conf->ip.dns2[2],
              otb_conf->ip.dns2[3]);
      break;

    case 'w':
    case 'W':
      INFO("Wipe stored config");
      uint8 spi_rc = spi_flash_erase_sector(OTB_BOOT_CONF_LOCATION / 0x1000);
      INFO(" Wiped");
      otb_reset(otb_break_config_reboot_string);
      break;

    case 'd':
    case 'D':
      INFO("Disable watchdog");
      otb_util_break_disable_timer();
      INFO(" Watchdog disabled");
      break;

    case 'e':
    case 'E':
      INFO("Enable watchdog");
      otb_util_break_disable_timer();
      otb_util_break_enable_timer(OTB_BREAK_WATCHDOG_TIMER);
      INFO(" Watchdog enabled: %dms", OTB_BREAK_WATCHDOG_TIMER)
      break;

    case 'h':
    case 'H':
    case '?':
      output_options = TRUE;
      break;

    default:
      INFO("Invalid option selected");
      break;
  }

  if (output_options)
  {
      otb_break_options_output();
  }

  EXIT;

  return(clear_buf);
}

ICACHE_FLASH_ATTR void otb_break_start_gpio_test(void)
{
  bool rc;
  uint8_t write[2];
  uint8_t val;
  uint8_t reg;
  uint8_t regs[6];
  int ii;
  uint8_t brzo_rc;

  ENTRY;

  INFO(" GPIO test running");

  otb_break_gpio_test_init(0x20, &otb_i2c_bus_internal);

  otb_break_gpio_next_led = 0;

  otb_util_timer_set((os_timer_t*)&otb_break_gpio_timer, 
                    (os_timer_func_t *)otb_break_gpio_timerfunc,
                    NULL,
                    250,
                    1);

  // Now create timer

EXIT_LABEL:

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_break_gpio_test_init(uint8_t addr, brzo_i2c_info *info)
{
  ENTRY;

  DEBUG(" Init MCP23017");
  DEBUG(" SDA pin:  %d", info->sda_pin);
  DEBUG(" SCL pin:  %d", info->scl_pin);
  DEBUG(" MCP addr: 0x%02x", addr);

  // Need to reinitialise bus.
  // Did this in otb_main.c but something we did since has screwed it up
  // No harm in redoing repeatedly (except for undoing whatever screwed it up
  // in the first place!)
  otb_i2c_initialize_bus_internal();
  if (otb_gpio_pins.soft_reset != OTB_GPIO_INVALID_PIN)
  {
    INFO(" Disable soft reset pin ...");
    otb_intr_unreg(otb_gpio_pins.soft_reset);
  }
  otb_i2c_mcp23017_init(addr, info);

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_break_gpio_timerfunc(void *arg)
{
  brzo_i2c_info *info;
  uint8_t addr;
  int ii;
  uint8_t type;
  uint8_t num;
  uint8_t gpa;
  uint8_t gpb;
  uint8_t *gp;

  ENTRY;

  info = &otb_i2c_bus_internal;
  addr = 0x20;

  if (otb_break_gpio_next_led >= OTB_BREAK_GPIO_TEST_LED_NUM)
  {
    otb_break_gpio_next_led = 0;
  }

  type = otb_break_gpio_test_led_seq[otb_break_gpio_next_led].type;
  num = otb_break_gpio_test_led_seq[otb_break_gpio_next_led].num;

  otb_gpio_set(12, 0, TRUE);
  otb_gpio_set(13, 0, TRUE);
  otb_gpio_set(14, 0, TRUE);
  gpa = 0;
  gpb = 0;

  MDEBUG("type %d num %d", type, num);

  if (type == OTB_BREAK_GPIO_LED_TYPE_GPIO)
  {
    OTB_ASSERT((num > 11) && (num < 15));
    otb_gpio_set(num, 1, TRUE);
  }
  else
  {
    OTB_ASSERT((type == OTB_BREAK_GPIO_LED_TYPE_GPA) ||
               (type == OTB_BREAK_GPIO_LED_TYPE_GPB));
    OTB_ASSERT(num < 8);
    if (type == OTB_BREAK_GPIO_LED_TYPE_GPA)
    {
      gp = &gpa;
    }
    else
    {
      gp = &gpb;
    }
    *gp = (1 << num);

  }
  
  MDEBUG("Write GPA 0x%02x GPB 0x%02x", gpa, gpb);
  otb_i2c_mcp23017_write_gpios(gpa, gpb, addr, info);
  otb_i2c_mcp23017_read_gpios(&gpa, &gpb, addr, info);
  MDEBUG("Read  GPA 0x%02x GPB 0x%02x", gpa, gpb);

  otb_break_gpio_next_led++;

  EXIT;

  return;
}

void ICACHE_FLASH_ATTR otb_break_gpio_test_cancel(void)
{
  brzo_i2c_info *info;
  uint8_t addr;

  ENTRY;

  otb_gpio_set(12, 1, TRUE);
  otb_gpio_set(13, 1, TRUE);
  otb_gpio_set(14, 1, TRUE);
  if (otb_gpio_pins.soft_reset != OTB_GPIO_INVALID_PIN)
  {
    INFO(" Re-enable soft reset pin ...");
    otb_gpio_set(otb_gpio_pins.soft_reset, 1, TRUE);
    GPIO_DIS_OUTPUT(otb_gpio_pins.soft_reset);
    otb_intr_register(otb_gpio_reset_button_interrupt, NULL, otb_gpio_pins.soft_reset);
  }
  info = &otb_i2c_bus_internal;
  addr = 0x20;
  otb_i2c_mcp23017_write_gpios(0, 0, addr, info);
  otb_gpio_set(12, 0, TRUE);
  otb_util_timer_cancel((os_timer_t*)&otb_break_gpio_timer);

  EXIT;

  return;
}

bool ICACHE_FLASH_ATTR otb_break_gpio_input(char input)
{

  ENTRY;

  switch (input)
  {
    case 'x':
    case 'X':
      otb_break_state = OTB_BREAK_STATE_MAIN;
      otb_break_gpio_test_cancel();
      INFO(" Exiting GPIO test")
      break;

    case 'h':
    case 'H':
    case '?':
      INFO("  x - Terminate");
      break;

    default:
      INFO(" Invalid option selected");
      break;
  }

  EXIT;

  return(TRUE);
}

bool ICACHE_FLASH_ATTR otb_break_config_input(char input)
{
  bool rc = TRUE;
  bool persist = FALSE;
  uint8_t ip[4];
  bool fn_rc;

  ENTRY;

  switch (otb_break_config_state)
  {
    case OTB_BREAK_CONFIG_STATE_MAIN:
      rc = otb_break_config_input_main(input);
      break;

    case OTB_BREAK_CONFIG_STATE_SSID:
      if (otb_break_collect_string(input))
      {
        os_strncpy(otb_conf->ssid, otb_break_string, OTB_CONF_WIFI_SSID_MAX_LEN);
        otb_conf->ssid[OTB_CONF_WIFI_SSID_MAX_LEN-1] = 0;
        INFO("  Set SSID to: %s", otb_conf->ssid);
        otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
        persist = otb_break_config_persist ? TRUE : FALSE;
      }
      break;
      
    case OTB_BREAK_CONFIG_STATE_PASSWORD:
      if (otb_break_collect_string(input))
      {
        os_strncpy(otb_conf->password, otb_break_string, OTB_CONF_WIFI_PASSWORD_MAX_LEN);
        otb_conf->password[OTB_CONF_WIFI_PASSWORD_MAX_LEN-1] = 0;
        INFO("  Set WiFi password to: %s", otb_conf->password);
        otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
        persist = otb_break_config_persist ? TRUE : FALSE;
      }
      break;
      
    case OTB_BREAK_CONFIG_STATE_MQTT_SVR:
      if (otb_break_collect_string(input))
      {
        os_strncpy(otb_conf->mqtt.svr, otb_break_string, OTB_MQTT_MAX_SVR_LEN);
        otb_conf->mqtt.svr[OTB_MQTT_MAX_SVR_LEN-1] = 0;
        INFO("  Set MQTT server to: %s", otb_conf->mqtt.svr);
        otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
        persist = otb_break_config_persist ? TRUE : FALSE;
      }
      break;
      
    case OTB_BREAK_CONFIG_STATE_MQTT_PORT:
      if (otb_break_collect_string(input))
      {
        otb_conf->mqtt.port = atoi(otb_break_string);
        INFO("  Set MQTT port to: %d", otb_conf->mqtt.port);
        otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
        persist = otb_break_config_persist ? TRUE : FALSE;
      }
      break;
      
    case OTB_BREAK_CONFIG_STATE_CHIP_ID:
      if (otb_break_collect_string(input))
      {
        os_strncpy(OTB_MAIN_CHIPID, otb_break_string, OTB_MAIN_CHIPID_STR_LENGTH);
        OTB_MAIN_CHIPID[OTB_MAIN_CHIPID_STR_LENGTH-1] = 0;
        INFO("  Set Chip ID to: %s", OTB_MAIN_CHIPID);
        otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
      }
      break;

    case OTB_BREAK_CONFIG_STATE_DEVICE_ID:
      if (otb_break_collect_string(input))
      {
        os_strncpy(OTB_MAIN_DEVICE_ID, otb_break_string, OTB_MAIN_DEVICE_ID_STR_LENGTH);
        OTB_MAIN_DEVICE_ID[OTB_MAIN_DEVICE_ID_STR_LENGTH-1] = 0;
        INFO("  Set Device ID to: %s", OTB_MAIN_DEVICE_ID);
        otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
      }
      break;

    case OTB_BREAK_CONFIG_STATE_IP_IPV4_ADDRESS:
      if (otb_break_collect_string(input))
      {
        if (otb_util_parse_ipv4_str(otb_break_string, ip) &&
            !otb_util_ip_is_all_val(ip, 0) &&
            !otb_util_ip_is_all_val(ip, 0xff))
        {
          os_memcpy(otb_conf->ip.ipv4, ip, 4);
          INFO("  Set IPv4 address to: %d.%d.%d.%d",
               ip[0],
               ip[1],
               ip[2],
               ip[3]);
          otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
          persist = otb_break_config_persist ? TRUE : FALSE;
        }
        else
        {
          INFO("  Invalid IPv4 address entered");
          otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
        }
      }
      break;

    case OTB_BREAK_CONFIG_STATE_IP_IPV4_SUBNET:
      if (otb_break_collect_string(input))
      {
        if (otb_util_parse_ipv4_str(otb_break_string, ip) &&
            otb_util_ip_is_subnet_valid(ip))
        {
          os_memcpy(otb_conf->ip.ipv4_subnet, ip, 4);
          INFO("  Set IPv4 subnet mask to: %d.%d.%d.%d",
               ip[0],
               ip[1],
               ip[2],
               ip[3]);
          otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
          persist = otb_break_config_persist ? TRUE : FALSE;
        }
        else
        {
          INFO("  Invalid IPv4 subnet mask entered");
          otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
        }
      }
      break;

    case OTB_BREAK_CONFIG_STATE_IP_IPV4_GATEWAY:
      if (otb_break_collect_string(input))
      {
        if (otb_util_parse_ipv4_str(otb_break_string, ip) &&
            !otb_util_ip_is_all_val(ip, 0xff))
        {
          INFO("  Set gateway to: %d.%d.%d.%d",
               ip[0],
               ip[1],
               ip[2],
               ip[3]);
          os_memcpy(otb_conf->ip.gateway, ip, 4);
          otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
          persist = otb_break_config_persist ? TRUE : FALSE;
        }
        else
        {
          INFO("  Invalid IPv4 address entered");
          otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
        }
      }
      break;

    case OTB_BREAK_CONFIG_STATE_IP_IPV4_DNS1:
    case OTB_BREAK_CONFIG_STATE_IP_IPV4_DNS2:
      if (otb_break_collect_string(input))
      {
        if (otb_util_parse_ipv4_str(otb_break_string, ip) &&
            !otb_util_ip_is_all_val(ip, 0xff))
        {
          int svr;
          if (otb_break_config_state == OTB_BREAK_CONFIG_STATE_IP_IPV4_DNS1)
          {
            os_memcpy(otb_conf->ip.dns1, ip, 4);
            svr = 1;
          }
          else
          {
            os_memcpy(otb_conf->ip.dns2, ip, 4);
            svr = 2;
          }
          INFO("  Set DNS server %d to: %d.%d.%d.%d",
               svr, 
               ip[0],
               ip[1],
               ip[2],
               ip[3]);
          otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
          persist = otb_break_config_persist ? TRUE : FALSE;
        }
        else
        {
          INFO("  Invalid IPv4 address entered");
          otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
        }
      }
      break;

    case OTB_BREAK_CONFIG_STATE_IP_IPV4_DOMAIN_NAME:
      if (otb_break_collect_string(input))
      {
        os_strncpy(otb_conf->ip.domain_name, otb_break_string, OTB_IP_MAX_DOMAIN_NAME_LEN);
        otb_conf->ip.domain_name[OTB_IP_MAX_DOMAIN_NAME_LEN-1] = 0;
        INFO("  Set domain name to: %s", otb_conf->ip.domain_name);
        otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
        persist = otb_break_config_persist ? TRUE : FALSE;
      }
      break;
      
    default:
      OTB_ASSERT(FALSE);
      break;
  }

  if (persist)
  {
    rc = otb_conf_update(otb_conf);
    if (rc)
    {
      INFO("  Change stored");
    }
    else
    {
      WARN("  Failed to store change");
    }
  }

  EXIT;

  return(rc);
}

void ICACHE_FLASH_ATTR otb_break_clear_string(void)
{
  ENTRY;

  os_memset(otb_break_string, 0, OTB_BREAK_CONFIG_STRING_LEN);

  EXIT;

  return;
}

// returns TRUE if string is finished (got \r or \n)
bool ICACHE_FLASH_ATTR otb_break_collect_string(char input)
{
  bool rc = FALSE;
  size_t len;

  ENTRY;

  if ((input == '\r') || (input == '\n'))
  {
    rc = TRUE;
  }
  else
  {
    len = os_strnlen(otb_break_string, OTB_BREAK_CONFIG_STRING_LEN);
    if (len < (OTB_BREAK_CONFIG_STRING_LEN - 1))
    {
      otb_break_string[len] = input;
      ets_printf("%c", input);
    }
    else
    {
      INFO("  Hit max config string length %d", OTB_BREAK_CONFIG_STRING_LEN-1);
      rc = TRUE;
      otb_break_string[OTB_BREAK_CONFIG_STRING_LEN-1] = 0;
    }
  }

  if (rc)
  {
    ets_printf("\r\n");
  }

  EXIT;

  return(rc);
}

bool ICACHE_FLASH_ATTR otb_break_config_input_main(char input)
{
  bool rc;

  ENTRY;

  switch (input)
  {
    case 'x':
    case 'X':
      otb_break_state = OTB_BREAK_STATE_MAIN;
      INFO(" Exiting config change")
      break;

    case 's':
    case 'S':
      INFO(" SSID");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_SSID;
      otb_break_clear_string();
      break;

    case 'p':
    case 'P':
      INFO(" WiFi Password");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_PASSWORD;
      otb_break_clear_string();
      break;

    case 'm':
    case 'M':
      INFO(" MQTT Server");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_MQTT_SVR;
      otb_break_clear_string();
      break;

    case 'r':
    case 'R':
      INFO(" MQTT Port");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_MQTT_PORT;
      otb_break_clear_string();
      break;

    case 'c':
    case 'C':
      INFO(" Chip ID");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_CHIP_ID;
      otb_break_clear_string();
      break;

    case 'd':
    case 'D':
      INFO(" Device ID");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_DEVICE_ID;
      otb_break_clear_string();
      break;

    case 'i':
    case 'I':
      INFO(" Toggle Manual/DHCP IP addressing");
      if (otb_conf->ip.manual == OTB_IP_DHCP_DHCP)
      {
        otb_conf->ip.manual = OTB_IP_DHCP_MANUAL;
        INFO(" Set IP addressing to: Manual");
      }
      else
      {
        otb_conf->ip.manual = OTB_IP_DHCP_DHCP;
        INFO(" Set IP addressing: DHCP");
      }
      if (otb_break_config_persist)
      {
        rc = otb_conf_update(otb_conf);
        if (rc)
        {
          INFO("  Change stored");
        }
        else
        {
          WARN("  Failed to store change");
        }
      }
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_MAIN;
      break;

    case 'a':
    case 'A':
      INFO(" IPv4 address");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_IP_IPV4_ADDRESS;
      otb_break_clear_string();
      break;

    case 'n':
    case 'N':
      INFO(" IPv4 subnet");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_IP_IPV4_SUBNET;
      otb_break_clear_string();
      break;

    case 'g':
    case 'G':
      INFO(" IPv4 gateway");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_IP_IPV4_GATEWAY;
      otb_break_clear_string();
      break;

    case '1':
      INFO(" DNS server 1");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_IP_IPV4_DNS1;
      otb_break_clear_string();
      break;

    case '2':
      INFO(" DNS server 2");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_IP_IPV4_DNS2;
      otb_break_clear_string();
      break;

    case 'o':
    case 'O':
      INFO(" Domain name");
      otb_break_config_state = OTB_BREAK_CONFIG_STATE_IP_IPV4_DOMAIN_NAME;
      otb_break_clear_string();
      break;

    case 'h':
    case 'H':
    case '?':
      INFO(" Config change options:")
      INFO("  s - SSID");
      INFO("  p - WiFi Password");
      INFO("  m - MQTT Server");
      INFO("  r - MQTT Port");
      INFO("  c - Chip ID");
      INFO("  d - Device ID");
      INFO("  i - Toggle Manual/DHCP IP addressing");
      INFO("  a - IPv4 address");
      INFO("  n - IPv4 subnet");
      INFO("  g - IPv4 gateway");
      INFO("  1 - DNS server 1");
      INFO("  2 - DNS server 2");
      INFO("  o - Domain name suffix (not hostname)")
      INFO("  x - Exit");
      break;

    default:
      INFO("Invalid option selected");
      break;
  }

  EXIT;

  return(TRUE);
}

void ICACHE_FLASH_ATTR otb_break_reset_button_interrupt(void *arg)
{
  sint8 get;

  // Get and act on interrupt
  get = otb_gpio_get(otb_gpio_pins.soft_reset, TRUE);

  if (!get)
  {
    INFO(" Button pressed");
  }
  else
  {
    INFO(" Button released");
  }
  
  return;
}

bool ICACHE_FLASH_ATTR otb_break_soft_reset_input(char input)
{

  ENTRY;

  switch (input)
  {
    case 'x':
    case 'X':
      otb_break_state = OTB_BREAK_STATE_MAIN;
      INFO(" Exiting soft reset test")
      otb_intr_unreg(otb_gpio_pins.soft_reset);
      otb_gpio_set(otb_gpio_pins.soft_reset, 1, TRUE);
      GPIO_DIS_OUTPUT(otb_gpio_pins.soft_reset);
      otb_intr_register(otb_gpio_reset_button_interrupt, NULL, otb_gpio_pins.soft_reset);
      break;

    case 'h':
    case 'H':
    case '?':
      INFO("  x - Terminate");
      break;

    default:
      INFO("Invalid option selected");
      break;
  }

  EXIT;

  return(TRUE);
}

// Called from within interrupts
void otb_break_process_char(void)
{

  //ENTRY;

  os_timer_disarm(&otb_break_process_char_timer);
  os_timer_setfn(&otb_break_process_char_timer,
                 otb_break_process_char_timerfunc,
                 NULL);
  os_timer_arm(&otb_break_process_char_timer, 0, 0);

  //EXIT;

  return;
}

