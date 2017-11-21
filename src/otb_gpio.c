/*
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
 */

#define OTB_GPIO_C
#include "otb.h"

const otb_eeprom_pin_info ICACHE_FLASH_ATTR *otb_gpio_get_pin_info_det(uint32_t pin_num, uint32_t num_pins, const otb_eeprom_pin_info *pin_info)
{
  int ii;
  const otb_eeprom_pin_info *rc_pin_info = NULL;

  DEBUG("GPIO: otb_gpio_get_pin_info_det entry")

  for (ii = 0; ii < num_pins; ii++)
  {
    DEBUG("GPIO: ii and pin_num: %d %d", ii, pin_num);
    if (pin_info[ii].num == pin_num)
    {
      rc_pin_info = pin_info + ii;
      DEBUG("GPIO: match: 0x%p use: %d", rc_pin_info, rc_pin_info->use);
      break;
    }
  }

EXIT_LABEL:

  DEBUG("GPIO: otb_gpio_get_pin_info_det exit")

  return rc_pin_info;
}

const otb_eeprom_pin_info ICACHE_FLASH_ATTR *otb_gpio_get_pin_info(uint32_t pin_num)
{
  const otb_eeprom_pin_info *pin_info = NULL;

  DEBUG("GPIO: otb_gpio_get_pin_info entry")

  if (otb_eeprom_main_board_gpio_pins_g != NULL)
  {
    DEBUG("GPIO: Search in eeprom: %d %p", otb_eeprom_main_board_gpio_pins_g->num_pins, otb_eeprom_main_board_gpio_pins_g->pin_info);
    pin_info = otb_gpio_get_pin_info_det(pin_num, otb_eeprom_main_board_gpio_pins_g->num_pins, otb_eeprom_main_board_gpio_pins_g->pin_info);
  }

  if (pin_info == NULL)
  {
    DEBUG("GPIO: Search in pin defaults: %d %p", otb_eeprom_def_board_info->pin_count, *(otb_eeprom_def_board_info->pin_info));
    pin_info = otb_gpio_get_pin_info_det(pin_num, otb_eeprom_def_board_info->pin_count, *(otb_eeprom_def_board_info->pin_info));
  }

  DEBUG("GPIO: otb_gpio_get_pin_info exit")

  return pin_info;
}

void ICACHE_FLASH_ATTR otb_gpio_init(void)
{
  int ii, jj;
  const otb_eeprom_pin_info *pin_info;
  bool set_ok;

  DEBUG("GPIO: otb_gpio_init entry");

  memset(otb_gpio_pin_io_status, 0, OTB_GPIO_ESP_GPIO_PINS);
  gpio_init();

  // Initialize all the pins we might use as GPIO.  Pins not included:
  // 1/3 (serial)
  // 6-11 (SPI flash)
  // 16 (reset)
  for (ii = 0; ii < OTB_GPIO_ESP_GPIO_PINS; ii++)
  {
    DEBUG("GPIO: Init pin %d", ii);
  
    // For each pin check whether have directions on eeprom about how to use
    // it.  If not, use default pin information.
    pin_info = otb_gpio_get_pin_info(ii);
    OTB_ASSERT(pin_info != NULL);

    // Do special processing
    DEBUG("GPIO: pin use: 0x%x", pin_info->use);
    set_ok = TRUE; 
    switch (pin_info->use)
    {
      case OTB_EEPROM_PIN_USE_RESERVED:
      case OTB_EEPROM_PIN_USE_INT_SDA:
      case OTB_EEPROM_PIN_USE_INT_SCL:
        INFO("GPIO: Reserved pin: %d", ii);
        set_ok = FALSE; 
        break;

      case OTB_EEPROM_PIN_USE_RESET_HARD:
        INFO("GPIO: Hard reset pin: %d", ii);
        set_ok = FALSE; 
        break;

      case OTB_EEPROM_PIN_USE_RESET_SOFT:
        INFO("GPIO: Soft reset pin: %d", ii);
        otb_gpio_pins.soft_reset = pin_info->num;
        ETS_GPIO_INTR_DISABLE();
        ETS_GPIO_INTR_ATTACH(otb_gpio_reset_button_interrupt, otb_gpio_pins.soft_reset);
        GPIO_DIS_OUTPUT(otb_gpio_pins.soft_reset);
        gpio_pin_intr_state_set(GPIO_ID_PIN(otb_gpio_pins.soft_reset), 1);
        ETS_GPIO_INTR_ENABLE();
        break;

      case OTB_EEPROM_PIN_USE_STATUS_LED:
        INFO("GPIO: Status LED pin: %d", ii);
        otb_gpio_pins.status = pin_info->num;
        otb_gpio_pins.status_type = pin_info->further_info;
        set_ok = otb_gpio_set(ii, 1, TRUE);
        break;

      default:
        switch (ii)
        {
          case 4:
          case 5:
          case 12:
          case 13:
            // These pins are held up on otbiot hardware, so initialise to 1
            // On the Wemos D1 mini these are floating.  Might as well therefore be
            // consistent with otbiot hardware, but may say these pins switching
            // at boot as may well be floating low before we set them.
            DEBUG("GPIO: Set pin: %d: 1", ii);
            set_ok = otb_gpio_set(ii, 1, FALSE);
            break;

          default:
            DEBUG("GPIO: Set pin: %d: 0", ii);
            set_ok = otb_gpio_set(ii, 0, FALSE);
            break;
        }
        break;
    }
    if (set_ok)
    {
      PIN_FUNC_SELECT(pin_mux[ii], pin_func[ii]);
    }
  }

  DEBUG("GPIO: otb_gpio_init exit");
  
  return;
}

void ICACHE_FLASH_ATTR otb_gpio_reset_button_interrupt(void)
{
  sint8 get;
  uint32_t gpio_status;

  // Disable interrupts
  ETS_GPIO_INTR_DISABLE();
  
  // Get and act on interrupt
  get = otb_gpio_get(otb_gpio_pins.soft_reset, TRUE);
  if (get)
  {
    WARN("GPIO: Reset button pressed");
    otb_reset_schedule(1000, otb_gpio_reset_reason_reset, FALSE);
    otb_led_wifi_update(OTB_LED_NEO_COLOUR_BLUE, TRUE);
    otb_led_wifi_blink(5);
  }
  else
  {
    DEBUG("GPIO: Reset button released");
  }
  
  // Clear interrupt
  gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
  
  // Re-enable interrupts
  ETS_GPIO_INTR_ENABLE();
  
  return;
}

#if 0
void ICACHE_FLASH_ATTR otb_gpio_reset_kick_off(void)
{
  DEBUG("GPIO: otb_gpio_init_reset_kick_off entry");

  otb_led_wifi_update(OTB_LED_NEO_COLOUR_WHITE, TRUE);
  otb_gpio_init_reset_timer();
  otb_gpio_reset_count = 0;

  DEBUG("GPIO: otb_gpio_init_reset_kick_off exit");
}

void ICACHE_FLASH_ATTR otb_gpio_init_reset_timer(void)
{
  DEBUG("GPIO: otb_gpio_init_reset_timer entry");
  
  otb_util_timer_set((os_timer_t*)&otb_gpio_reset_timer, 
                     (os_timer_func_t *)otb_gpio_reset_timerfunc,
                     NULL,
                     1000,
                     1);

  DEBUG("GPIO: otb_gpio_init_reset_timer exit");
}

void ICACHE_FLASH_ATTR otb_gpio_reset_timerfunc(void *arg)
{
  DEBUG("GPIO: otb_gpio_reset_timerfunc entry");
  
  otb_gpio_reset_count++;
  
  if (!otb_gpio_get(OTB_GPIO_RESET_PIN, TRUE))
  {
    if (otb_gpio_reset_count >= (OTB_GPIO_RESET_COUNT_MAX-1))
    {
      WARN("GPIO: Reset the device - not implemented!!!");
      otb_util_factory_reset();
      otb_util_timer_cancel((os_timer_t*)&otb_gpio_reset_timer);
    }
  }
  else
  {
    INFO("GPIO: Reset cancelled");
    otb_gpio_reset_count = 0;
    otb_util_timer_cancel((os_timer_t*)&otb_gpio_reset_timer);
    
    // Re-instate usual processing
    otb_wifi_kick_off();
  }
  
  DEBUG("GPIO: otb_gpio_reset_timerfunc exit");
}
#endif

void ICACHE_FLASH_ATTR otb_gpio_apply_boot_state(void)
{
  char *dummy;
  int ii;
  bool rc;

  DEBUG("GPIO: gpio_apply_boot_state entry");

  if (otb_eeprom_module_present())
  {
    INFO("GPIO: Not applying boot state as have modules present");
    goto EXIT_LABEL;
  }

  for (ii = 0; ii < 17; ii++)
  {
    if (!otb_gpio_is_reserved(ii, &dummy))
    {
      rc = otb_gpio_set(ii, otb_conf->gpio_boot_state[ii], FALSE);
      if (!rc)
      {
        WARN("GPIO: failed to set boot state for pin %d", ii);
      }
    }
    else
    {
      WARN("GPIO: not applying boot state for reserved pin %d", ii);
    }
  }

EXIT_LABEL:  
  
  DEBUG("GPIO: gpio_apply_boot_state exit");
  
  return;
}

int8_t ICACHE_FLASH_ATTR otb_gpio_get_pin(unsigned char *to_match)
{
  int8_t pin = -1;

  DEBUG("GPIO: otb_gpio_get_pin entry");
  
  pin = atoi(to_match);
  DEBUG("GPIO: Pin %d", pin);
  if ((pin < 0) ||
      (pin >= GPIO_PIN_NUM) ||
      ((pin == 0) && (to_match[0] != '0')))
  {
    pin = -1;
  }

  DEBUG("GPIO: otb_gpio_get_pin exit");

  return pin;
}

// Unlike "otb_gpio_is_valid" this method is called from the cmd handling function to 
// check whether this gpio can be get or set - so both whether a valid pin and whether
// reserved.
bool ICACHE_FLASH_ATTR otb_gpio_valid_pin(unsigned char *to_match)
{
  bool rc = FALSE;
  int8_t pin;
  char *text;
  
  DEBUG("GPIO: otb_gpio_valid_pin entry");

  pin = otb_gpio_get_pin(to_match);
  
  // (Note passing in int8_t into uint8_t method - this is OK as only have 0-16 GPIOs,
  // and error value of -1 is 255 when unsigned, so will return invalid
  rc = otb_gpio_is_valid(pin);
  
  if (rc)
  {
    rc = otb_gpio_is_reserved(pin, &text);
    // Flip sense of rc
    rc = rc ? FALSE : TRUE;
  }

  DEBUG("GPIO: otb_gpio_valid_pin exit");

 return rc;
}

bool ICACHE_FLASH_ATTR otb_gpio_is_valid(uint8_t pin)
{
  bool rc = TRUE;
  
  DEBUG("GPIO: otb_gpio_is_valid entry");
  
  if (pin >= OTB_GPIO_ESP_GPIO_PINS)
  {
    DEBUG("GPIO: Pin is invalid");
    rc = FALSE;
  }

  DEBUG("GPIO: otb_gpio_is_special exit");
  
  return rc;
}

bool ICACHE_FLASH_ATTR otb_gpio_is_reserved(uint8_t pin, char **reserved_text)
{
  bool rc = TRUE;
  const otb_eeprom_pin_info *pin_info;
  
  DEBUG("GPIO: otb_gpio_is_reserved entry");

  *reserved_text = "";

  pin_info = otb_gpio_get_pin_info(pin);
  OTB_ASSERT(pin_info != NULL);

  switch (pin_info->use)
  {
    case OTB_EEPROM_PIN_USE_GPIO: 
      rc = FALSE;
      break;

    case OTB_EEPROM_PIN_USE_STATUS_LED:
      *reserved_text = "status led";
      break;

    case OTB_EEPROM_PIN_USE_RESET_HARD:
      *reserved_text = "hard reset";
      break;

    case OTB_EEPROM_PIN_USE_RESET_SOFT:
      *reserved_text = "soft reset";
      break;

    case OTB_EEPROM_PIN_USE_INT_SDA:
      *reserved_text = "internal SDA";
      break;

    case OTB_EEPROM_PIN_USE_INT_SCL:
      *reserved_text = "internal SCL";
      break;

    case OTB_EEPROM_PIN_USE_TX:
      *reserved_text = "TX";
      break;

    case OTB_EEPROM_PIN_USE_RX:
      *reserved_text = "RX";
      break;

    default:
      *reserved_text = "reserved other";
      break;
  }

  DEBUG("GPIO: otb_gpio_is_reserved exit");

  return rc;
}  

bool ICACHE_FLASH_ATTR otb_gpio_cmd(unsigned char *next_cmd,
                                    void *arg,
                                    unsigned char *prev_cmd)
{
  bool rc = FALSE;
  int cmd;
  otb_conf_ads *ads = NULL;
  char *pin_t;
  int8_t pin;
  int8_t value;
  bool store_conf = FALSE;
    
  DEBUG("GPIO: otb_gpio_cmd entry");
  
  // Double check what we're being asked to do is valid
  cmd = (int)arg;
  OTB_ASSERT((cmd >= OTB_CMD_GPIO_MIN) && (cmd < OTB_CMD_GPIO_NUM));
  
  // No need to check whether the pin is valid or reserved - this has already been done
  pin = otb_gpio_get_pin(prev_cmd);
  
  // If a set command, get value to set
  if ((cmd == OTB_CMD_GPIO_TRIGGER) || (cmd == OTB_CMD_GPIO_SET_CONFIG))
  {
    if (next_cmd == NULL)
    {
      rc = FALSE;
      otb_cmd_rsp_append("No value provided");
      goto EXIT_LABEL;
    }
    value = atoi(next_cmd);
    if ((value < 0) ||
        (value > 1) ||
        ((value == 0) && (next_cmd[0] != '0')))
    {
      rc = FALSE;
      otb_cmd_rsp_append("Invalid value provided");
      goto EXIT_LABEL;
    }
  }
  
  switch(cmd)
  {
    case OTB_CMD_GPIO_GET:
      value = otb_gpio_get(pin, FALSE);
      otb_cmd_rsp_append("%d", value);
      break;
      
    case OTB_CMD_GPIO_GET_CONFIG:
      value = otb_conf->gpio_boot_state[pin];
      otb_cmd_rsp_append("%d", value);
      break;
    
    case OTB_CMD_GPIO_TRIGGER:
      rc = otb_gpio_set(pin, value, FALSE);
      break;
      
    case OTB_CMD_GPIO_SET_CONFIG:
      if (otb_conf->gpio_boot_state[pin] != value)
      {
        otb_conf->gpio_boot_state[pin] = value;
        store_conf = TRUE;
      }
      break;
      
    default:
      OTB_ASSERT(FALSE);
      break;
  }

EXIT_LABEL:

  // If successful store off new config
  if (store_conf)
  {
    rc = otb_conf_update(otb_conf);
    if (!rc)
    {
      otb_cmd_rsp_append("failed to store new config");
    }
  }

  DEBUG("GPIO: otb_gpio_cmd exit");
  
  return rc;
}

sint8 ICACHE_FLASH_ATTR otb_gpio_get(int pin, bool override_reserved)
{
  bool special;
  sint8 input = -1;
  char *error_text;
  
  DEBUG("GPIO: otb_gpio_get entry");

  if (!otb_gpio_is_valid(pin))
  {
    ERROR("GPIO: Can't get pin %d - invalid", pin);
    goto EXIT_LABEL;
  }
  
  if (!override_reserved && otb_gpio_is_reserved(pin, &error_text))
  {
    ERROR("GPIO: Can't get pin %d - %s", error_text);
    goto EXIT_LABEL;
  }
  
  // Should this be INPUT or OUTPUT?
  input = GPIO_INPUT_GET(pin);
  
  DEBUG("GPIO: Pin %d state %d", pin, input);
  
EXIT_LABEL:
  
  DEBUG("GPIO: otb_gpio_get exit");
  
  return input;
}

bool ICACHE_FLASH_ATTR otb_gpio_set(int pin, int value, bool override_reserved)
{
  bool rc = FALSE;
  bool special;
  uint8 input;
  char *error_text;
  
  DEBUG("GPIO: otb_gpio_set entry");

  if (!otb_gpio_is_valid(pin))
  {
    ERROR("GPIO: Can't set pin %d - invalid", pin);
    goto EXIT_LABEL;
  }
  
  if (!override_reserved && otb_gpio_is_reserved(pin, &error_text))
  {
    ERROR("GPIO: Can't set pin %d - %s", pin, error_text);
    goto EXIT_LABEL;
  }
  
  DEBUG("GPIO: Set pin %d value %d", pin, value);
  // Code used to say pin - 1.  Why???
  otb_gpio_pin_io_status[pin] = OTB_GPIO_PIN_IO_STATUS_OUTPUT;
  GPIO_OUTPUT_SET(pin, value);
  rc = TRUE;
  
EXIT_LABEL:
  
  DEBUG("GPIO: otb_gpio_set exit");
  
  return rc;
}

void ICACHE_FLASH_ATTR otb_gpio_mqtt(char *cmd1, char *cmd2, char *cmd3)
{
  int pin;
  int value = -1;
  uint8 cmd_id = OTB_MQTT_CMD_INVALID_;
  bool rc;
  char response[8];
  bool save = FALSE;
  char *error_text;
  char *cmd = "";

  DEBUG("GPIO: otb_gpio_cmd entry");

  // Note that cmd1 and cmd2 are colon terminated
  // Supported commands:
  // cmd1 = get/set
  // cmd2 = field
  // cmd3 = value (set only)
  
  if ((cmd1 == NULL) || (cmd2 == NULL))
  {
    INFO("GPIO: Invalid GPIO command");
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                         OTB_MQTT_STATUS_ERROR,
                         "Invalid GPIO command",
                         "");
    goto EXIT_LABEL;
  }
  
  if (otb_mqtt_match(cmd1, OTB_MQTT_CMD_SET))
  {
    cmd = OTB_MQTT_CMD_SET;
    cmd_id = OTB_MQTT_CMD_SET_;
  }
  else if (otb_mqtt_match(cmd1, OTB_MQTT_CMD_GET))
  {
    cmd = OTB_MQTT_CMD_GET;
    cmd_id = OTB_MQTT_CMD_GET_;
  }
  else if (otb_mqtt_match(cmd1, OTB_MQTT_CMD_SAVE))
  {
    cmd = OTB_MQTT_CMD_SAVE;
    cmd_id = OTB_MQTT_CMD_SAVE_;
  }
  
  pin = atoi(cmd2);
  DEBUG("GPIO: Pin %d", pin);
  if ((pin < 0) || (pin >= GPIO_PIN_NUM))
  {
    otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                         cmd,
                         OTB_MQTT_STATUS_ERROR,
                         "invalid Pin");
  }

  switch (cmd_id)
  {
    case OTB_MQTT_CMD_SAVE_:
      // Deliberately fall through to set.  Will store once we've decided if this GPIO
      // works
      DEBUG("GPIO: Save command");
      save = TRUE;
      if (cmd3 == NULL)
      {
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                             cmd,
                             OTB_MQTT_STATUS_ERROR,
                             "no state");
        goto EXIT_LABEL;
        break;
      }
      else
      {
        value = atoi(cmd3);
        if ((value >= 0) && (value <= 1))
        {
          if (!otb_gpio_is_reserved(pin, &error_text))
          {
            if (otb_conf->gpio_boot_state[pin] != value)
            {
              otb_conf->gpio_boot_state[pin] = value;
              rc = otb_conf_update(otb_conf);
              if (!rc)
              {
                ERROR("CONF: Failed to save new boot state");
                otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                                     cmd,
                                     OTB_MQTT_STATUS_ERROR,
                                     "failed to store off new boot state");
                goto EXIT_LABEL;
                break;
              }
            }
          }
          else
          {
            otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                                 cmd,
                                 OTB_MQTT_STATUS_ERROR,
                                 error_text);
            goto EXIT_LABEL;
            break;
          }
        }
        else
        {
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_ERROR,
                               "invalid state");
          goto EXIT_LABEL;
          break;
        }
      }
      // Fall through to actually set GPIO ...
    case OTB_MQTT_CMD_SET_:
      DEBUG("GPIO: Set command");
      if ((value != -1) || (cmd3 != NULL))
      {
        if (value == -1)
        {
          value = atoi(cmd3);
        }
        DEBUG("GPIO: Set - new value %d", value);
        rc = otb_gpio_set(pin, value, FALSE); 
        if (rc)
        {
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_OK,
                               "");
        }
        else
        {
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_ERROR,
                               "");
        }
      }
      else
      {
        INFO("GPIO: no value");
        otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                             cmd,
                             OTB_MQTT_STATUS_ERROR,
                             "No value");
        goto EXIT_LABEL;                         
      }

      break;
      
    case OTB_MQTT_CMD_GET_:
      DEBUG("GPIO: Get command");
      if (cmd3 != NULL)
      {
        if (otb_mqtt_match(cmd3, OTB_MQTT_BOOT_STATE))
        {
          value = otb_conf->gpio_boot_state[pin];
          os_snprintf(response, 8, "%d", value);
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_OK,
                               response);
        }
        else
        {
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_ERROR,
                               "Unknown get command");
          goto EXIT_LABEL;                         
          break;
        }
      }
      else
      {
        value = otb_gpio_get(pin, FALSE);
        if (rc >= 0)
        {
          os_snprintf(response, 8, "%d", value);
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_OK,
                               response);
        }
        else
        {
          INFO("MQTT: Failed to get GPIO");
          otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                               cmd,
                               OTB_MQTT_STATUS_ERROR,
                               "");
        }
      }
      break;
    
    default:
      INFO("GPIO: Unsupported command");
      otb_mqtt_send_status(OTB_MQTT_SYSTEM_GPIO,
                           OTB_MQTT_STATUS_ERROR,
                           "Unsupported command",
                           "");
      goto EXIT_LABEL;    
      break;
  }  

EXIT_LABEL:
  
  DEBUG("GPIO: otb_gpio_cmd exit");
  
  return;
}
