/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2017-8 Piers Finlayson
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

// XXX To do
// - Turn otb_nixie_show_value into generate 8 bit shift register control function
//   - Send value
//   - Apply value
// - Support arbitrary numbers of digits

#define OTB_NIXIE_C
#include "otb.h"

bool otb_nixie_depoison_timer_armed = FALSE;

void ICACHE_FLASH_ATTR otb_nixie_module_init(void)
{

  DEBUG("NIXIE: otb_nixie_module_init entry");

  otb_nixie_info.inited = FALSE;
  otb_nixie_info.depoisoning_wait_s = 0;
  otb_nixie_info.depoisoning = FALSE;
  otb_nixie_info.current.digits[0] = '_';
  otb_nixie_info.current.digits[1] = '_';
  otb_nixie_info.target.digits[0] = '_';
  otb_nixie_info.target.digits[1] = '_';
  otb_nixie_info.pins.srck = OTB_NIXIE_PIN_DEFAULT_SRCK;
  otb_nixie_info.pins.ser = OTB_NIXIE_PIN_DEFAULT_SER;
  otb_nixie_info.pins.rck = OTB_NIXIE_PIN_DEFAULT_RCK;
  otb_nixie_info.depoison_cycle = 0;
  otb_nixie_info.power = FALSE;
  
  os_timer_disarm((os_timer_t*)&(otb_nixie_info.depoisoning_timer));
  otb_nixie_depoison_timer_armed = FALSE;
  os_timer_disarm((os_timer_t*)&(otb_nixie_info.display_timer));
  os_timer_setfn((os_timer_t*)&(otb_nixie_info.depoisoning_timer), (os_timer_func_t *)otb_nixie_depoison, NULL);

  otb_nixie_indexes = otb_nixie_indexes_v0_2;
  otb_nixie_index_power = &otb_nixie_index_power_v0_2;

  DEBUG("NIXIE: otb_nixie_module_init exit");

  return;
}

void ICACHE_FLASH_ATTR otb_nixie_depoison(void *arg)
{
  bool rc;

  DEBUG("NIXIE: otb_nixie_depoison entry");

  if (!otb_nixie_info.depoisoning)
  {
    otb_nixie_info.depoisoning_wait_s++;
    if (otb_nixie_info.depoisoning_wait_s < OTB_NIXIE_DEPOISONING_TIMER_S)
    {
      // No need to rearm as set as repeating
      goto EXIT_LABEL;
    }
  }

  otb_nixie_info.depoisoning = TRUE;
  otb_nixie_info.depoisoning_wait_s = 0;
  os_timer_disarm((os_timer_t*)&(otb_nixie_info.depoisoning_timer));
  otb_nixie_depoison_timer_armed = FALSE;
  os_timer_disarm((os_timer_t*)&(otb_nixie_info.display_timer));

  if (otb_nixie_info.depoison_cycle >= OTB_NIXIE_CYCLE_LEN)
  {
    // Show current target
    otb_nixie_info.depoisoning = FALSE;
    otb_nixie_info.depoison_cycle = 0;
    rc = otb_nixie_display_update(&(otb_nixie_info.target));
    otb_nixie_depoison_timer_arm();
    goto EXIT_LABEL;
  }
  else
  {
    if (otb_nixie_info.depoison_cycle == 0)
    {
      // Store off current as target
      os_memcpy(&otb_nixie_info.target, &otb_nixie_info.current, sizeof(otb_nixie_info.current));
    }

    rc = otb_nixie_show_value(otb_nixie_depoison_cycle_display[otb_nixie_info.depoison_cycle], 2, TRUE);

    otb_nixie_info.depoison_cycle++;

    os_timer_setfn((os_timer_t*)&(otb_nixie_info.display_timer), (os_timer_func_t *)otb_nixie_depoison, NULL);
    os_timer_arm((os_timer_t*)&(otb_nixie_info.display_timer), OTB_NIXIE_DEPOSION_CYCLE_TIMER_MS, 0);  
  }

EXIT_LABEL:  

  DEBUG("NIXIE: otb_nixie_depoison exit");
  
  return;
}

// If only one byte passed in it's the right one.
uint32_t ICACHE_FLASH_ATTR otb_nixie_get_serial_data(char *bytes, uint8_t num_bytes, bool power)
{
  int ii;
  int which_nixie;
  int index;
  uint8_t chip;
  uint8_t pin;
  uint8_t mult;
  uint32_t value = 0;
  uint32_t temp_value;
  uint32_t dots, max_dots, min_dots;

  DEBUG("NIXIE: otb_nixie_get_serial_data entry");

  OTB_ASSERT((num_bytes <= 4) && (num_bytes > 0));

  if ((num_bytes == 0) || (num_bytes > 4))
  {
    WARN("NIXIE: Invalid bytes passed");
    value = OTB_NIXIE_INVALID_VALUE;
    goto EXIT_LABEL;
  }
  else if ((num_bytes == 3) || (num_bytes == 4))
  {
    if (num_bytes == 3)
    {
      // Must be 1 and only 1 '.' as 1st or 3rd
      max_dots = 1;
      min_dots = 1;
      if ((bytes[0] != '.') && (bytes[1] != '.'))
      {
        WARN("NIXIE: No dot at first or second");
        value = OTB_NIXIE_INVALID_VALUE;
        goto EXIT_LABEL;
      }
    }
    else
    {
      // Must be 2 '.'s and first must be dot
      max_dots = 2;
      min_dots = 2;
      if (bytes[0] != '.')
      {
        WARN("NIXIE: No dot at first");
        value = OTB_NIXIE_INVALID_VALUE;
        goto EXIT_LABEL;
      }
    }
    dots = 0;
    for (ii = 0; ii < 3; ii++)
    {
      if (bytes[ii] == '.')
      {
        dots++;
      }
    }
    if ((dots > max_dots) || (dots < min_dots))
    {
      WARN("NIXIE: Invalid number of dots");
      value = OTB_NIXIE_INVALID_VALUE;
      goto EXIT_LABEL;
    }
  }

  which_nixie = num_bytes == 1 ? 1 : 0;
  otb_nixie_info.current.dots[0] = '_';
  otb_nixie_info.current.dots[1] = '_';
  otb_nixie_info.current.digits[0] = '_';
  otb_nixie_info.current.digits[1] = '_';

  for (ii = 0; ii < num_bytes; ii++)
  {
    chip = 0xff;
    pin = 0xff;
    temp_value = 0;

    if ((bytes[ii] != '_') &&
        (bytes[ii] != '.') &&
        ((bytes[ii] < '0') || (bytes[ii] > '9')))
    {
      WARN("NIXIE: Invalid character: %d", bytes[ii]);
      value = OTB_NIXIE_INVALID_VALUE;
      goto EXIT_LABEL;
    }

    if (bytes[ii] == '_')
    {
      which_nixie++;
    }
    else
    {
      if (bytes[ii] == '.')
      {
        if ((which_nixie == 0) && (ii > 0))
        {
          which_nixie++;
        }
        index = OTB_NIXIE_INDEX_DP;
        otb_nixie_info.current.dots[which_nixie] = bytes[ii];
      }
      else if (bytes[ii] != '_')
      {
        otb_nixie_info.current.digits[which_nixie] = bytes[ii];
        index = bytes[ii] - '0';
      }
      chip = otb_nixie_indexes[which_nixie][index].chip;
      pin = otb_nixie_indexes[which_nixie][index].pin;

      mult = 2 - chip;

      OTB_ASSERT(pin <= 7);
      temp_value = 1 << (7 - pin);
      temp_value <<= 8 * mult;

      if (bytes[ii] != '.')
      {
        which_nixie++;
      }
    }

    DEBUG("NIXIE: Byte: %d Value: %c Chip: %d Pin: %d Output Value: 0x%06x", ii, bytes[ii], chip, pin, temp_value);

    value |= temp_value;
  }

  if (power)
  {
    // Do power
    chip = otb_nixie_index_power->chip;
    pin = otb_nixie_index_power->pin;
    mult = 2 - chip;
    OTB_ASSERT(pin <= 7);
    temp_value = 1 << (7 - pin);
    temp_value <<= 8 * mult;
    value |= temp_value;
  }

EXIT_LABEL:

  DEBUG("NIXIE: Combined Output Value: 0x%06x", value);

  DEBUG("NIXIE: otb_nixie_get_serial_data exit");

  return value;
}

bool ICACHE_FLASH_ATTR otb_nixie_show_value(unsigned char *to_show, uint8_t num_bytes, bool commit)
{
  uint32_t wait_time = OTB_NXIE_SERIAL_TIMER_US;
  uint32_t mask;
  uint32_t serial_data;
  uint32_t value;
  bool rc = TRUE;

  DEBUG("NIXIE: otb_nixie_show_value entry");

  otb_gpio_set(srck_pin, 1, FALSE);
  otb_gpio_set(ser_pin, 1, FALSE);
  otb_gpio_set(rck_pin, 1, FALSE);
  
  serial_data = otb_nixie_get_serial_data(to_show, num_bytes, otb_nixie_info.power);
  if (serial_data == OTB_NIXIE_INVALID_VALUE)
  {
    rc = FALSE;
    goto EXIT_LABEL;
  }

  if (!commit)
  {
    goto EXIT_LABEL;
  }

  for (mask = 1; mask < (1 << 24); mask <<= 1)
  {
    // Flip value
    value = serial_data & mask ? 0 : 1;

    // Set value
    otb_gpio_set(ser_pin, value, FALSE);
    os_delay_us(wait_time);

    // Clock
    otb_gpio_set(srck_pin, 0, FALSE);
    os_delay_us(wait_time);
    otb_gpio_set(srck_pin, 1, FALSE);
    os_delay_us(wait_time);
  }

  // Transfer data to output
  otb_gpio_set(rck_pin, 0, FALSE);
  os_delay_us(wait_time);
  otb_gpio_set(rck_pin, 1, FALSE);

EXIT_LABEL:

  DEBUG("NIXIE: otb_nixie_show_value exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_nixie_init(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc;

  DEBUG("NIXIE: otb_nixie_init entry");

  INFO("NIXIE: init");

  // Set pins to 1 (and using NOT gate - so is 0)
  otb_gpio_set(srck_pin, 1, FALSE);
  otb_gpio_set(ser_pin, 1, FALSE);
  otb_gpio_set(rck_pin, 1, FALSE);

  rc = otb_nixie_cycle(NULL, NULL, NULL);

  DEBUG("NIXIE: otb_nixie_init exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_nixie_clear(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc;

  DEBUG("NIXIE: otb_nixie_clear entry");

  INFO("NIXIE: clear");

  otb_nixie_info.target.digits[0] = '_';
  otb_nixie_info.target.digits[1] = '_';

  if (!otb_nixie_info.depoisoning)
  {
    rc = otb_nixie_show_value("__", 2, TRUE);
  }

  DEBUG("NIXIE: otb_nixie_clear exit");

  return TRUE;
};

bool ICACHE_FLASH_ATTR otb_nixie_show(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc;
  int num_bytes;
  bool commit;

  DEBUG("NIXIE: otb_nixie_show entry");
  
  if (next_cmd == NULL)
  {
    INFO("NIXIE: show");
    otb_cmd_rsp_append("no value");
    rc = FALSE;
    goto EXIT_LABEL;
  }

  INFO("NIXIE: show %s", next_cmd);

  commit = otb_nixie_info.depoisoning ? FALSE : TRUE;
  rc = otb_nixie_show_value(next_cmd, os_strnlen(next_cmd, 5), commit);
  if (!commit)
  {
    os_memcpy(&otb_nixie_info.target, &otb_nixie_info.current, sizeof(otb_nixie_info.current));
  }

EXIT_LABEL:

  DEBUG("NIXIE: otb_nixie_show exit");

  return rc;
};

bool ICACHE_FLASH_ATTR otb_nixie_cycle(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = TRUE;
  int num_bytes;
  int ii;

  DEBUG("NIXIE: otb_nixie_cycle entry");

  INFO("NIXIE: cycle");

  if (otb_nixie_info.depoisoning)
  {
    rc = FALSE;
    otb_cmd_rsp_append("depoisoning");
    goto EXIT_LABEL;
  }

  for (ii = 0; (ii < OTB_NIXIE_CYCLE_LEN) && rc; ii++)
  {
    rc = otb_nixie_show_value(otb_nixie_cycle_display[ii], 2, TRUE);
    os_delay_us(OTB_NIXIE_CYCLE_TIMER_US);
  }

EXIT_LABEL:

  DEBUG("NIXIE: otb_nixie_cycle exit");

  return rc;
};

void ICACHE_FLASH_ATTR otb_nixie_depoison_timer_arm()
{
  DEBUG("NIXIE: otb_nixie_depoison_timer_arm entry");

  otb_nixie_depoison_timer_disarm();
  os_timer_arm((os_timer_t*)&(otb_nixie_info.depoisoning_timer), 1000, 1);  
  otb_nixie_info.depoisoning_wait_s = 0;
  otb_nixie_depoison_timer_armed = TRUE;

  DEBUG("NIXIE: otb_nixie_depoison_timer_arm exit");

  return;
}

void ICACHE_FLASH_ATTR otb_nixie_depoison_timer_disarm()
{
  DEBUG("NIXIE: otb_nixie_depoison_timer_disarm entry");

  os_timer_disarm((os_timer_t*)&(otb_nixie_info.depoisoning_timer));
  otb_nixie_depoison_timer_armed = FALSE;

  DEBUG("NIXIE: otb_nixie_depoison_timer_disarm exit");

  return;
}

bool ICACHE_FLASH_ATTR otb_nixie_power(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool on = (bool)arg;
  bool rc = TRUE;
  int num_bytes;
  int ii;

  DEBUG("NIXIE: otb_nixie_power entry");

  INFO("NIXIE: power %s", on ? "on" : "off");

  if (on)
  {
    // Kick off depoison routine
    if (!otb_nixie_depoison_timer_armed)
    {
      otb_nixie_depoison_timer_arm();
    }
  }
  // Keep depoison timer running if we've ever been turned on so short on/off
  // periods don't keep from depoisoning

  otb_nixie_info.power = on ? TRUE : FALSE;
  
  if (!otb_nixie_info.depoisoning)
  {
    // If depoisoning will update the power soon anyway!
    otb_nixie_display_update(&(otb_nixie_info.current));
  }

EXIT_LABEL:

  DEBUG("NIXIE: otb_nixie_power exit");

  return rc;
};

bool ICACHE_FLASH_ATTR otb_nixie_display_update(otb_nixie_display *display)
{
  bool rc;
  char disp[2 * OTB_NIXIE_DIGITS + 1];
  int ii, jj;

  DEBUG("NIXIE: otb_nixie_display_update entry");

  ii = 0;
  for (jj = 0; jj < OTB_NIXIE_DIGITS; jj++)
  {
    if (display->dots[jj] == '.')
    {
      disp[ii] = '.';
      ii++;
    }
    disp[ii] = display->digits[jj];
    ii++;
  }
  disp[ii] = 0;
  rc = otb_nixie_show_value(disp, ii, TRUE);
 
  DEBUG("NIXIE: otb_nixie_display_update exit");

  return rc;
}
