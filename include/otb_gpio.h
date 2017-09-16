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
 
#ifndef OTB_GPIO_H
#define OTB_GPIO_H

#define OTB_GPIO_PIN_IO_STATUS_UNKNOWN  0
#define OTB_GPIO_PIN_IO_STATUS_INPUT    1
#define OTB_GPIO_PIN_IO_STATUS_OUTPUT   2

#define OTB_GPIO_RESET_PIN              14
#define OTB_GPIO_RESET_COUNT_MAX        15

#define OTB_GPIO_INVALID_PIN  0xffffffff

typedef struct otb_gpio_pin_purpose
{
  // OTB_GPIO_INVALID_PIN/0xffffffff means unset

  uint32_t soft_reset;

  uint32_t status;

  uint32_t status_type;

} otb_gpio_pin_purpose;

#ifndef OTB_GPIO_C
extern otb_gpio_pin_purpose otb_gpio_pins;
extern uint8_t otb_gpio_pin_io_status[OTB_GPIO_ESP_GPIO_PINS];
#else // OTB_GPIO_C
otb_gpio_pin_purpose otb_gpio_pins =
{
  OTB_GPIO_INVALID_PIN,
  OTB_GPIO_INVALID_PIN,
  OTB_GPIO_INVALID_PIN,
};
uint8_t otb_gpio_pin_io_status[OTB_GPIO_ESP_GPIO_PINS];
uint8_t otb_gpio_reset_count;
os_timer_t otb_gpio_reset_timer;
unsigned char ALIGN4 otb_gpio_reset_string[] = "Resetting to factory settings";
unsigned char ALIGN4 otb_gpio_reset_reason_reset[] = "Reset button pressed";
#endif // OTB_GPIO_C

const struct otb_eeprom_pin_info *otb_gpio_get_pin_info_det(uint32_t pin_num, uint32_t num_pins, const struct otb_eeprom_pin_info *pin_info);
const struct otb_eeprom_pin_info *otb_gpio_get_pin_info(uint32_t pin_num);
void otb_gpio_init(void);
void otb_gpio_reset_button_interrupt(void);
void otb_gpio_reset_kick_off(void);
void otb_gpio_init_reset_timer(void);
void otb_gpio_reset_timerfunc(void *arg);
void otb_gpio_apply_boot_state(void);
int8_t otb_gpio_get_pin(unsigned char *);
bool otb_gpio_valid_pin(unsigned char *);
bool otb_gpio_valid(unsigned char *);
bool otb_gpio_is_valid(uint8_t pin);
bool otb_gpio_is_reserved(uint8_t pin, char **reserved_text);
bool otb_gpio_cmd(unsigned char *, void *, unsigned char *);
sint8 otb_gpio_get(int pin, bool override_reserved);
bool otb_gpio_set(int pin, int value, bool override_reserved);
void otb_gpio_mqtt(char *cmd1, char *cmd2, char *cmd3);

#endif // OTB_GPIO_H
