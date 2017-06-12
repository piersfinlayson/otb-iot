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

#define OTB_NIXIE_C
#include "otb.h"

#define OTB_NIXIE_INDEX_0    0
#define OTB_NIXIE_INDEX_1    1
#define OTB_NIXIE_INDEX_2    2
#define OTB_NIXIE_INDEX_3    3
#define OTB_NIXIE_INDEX_4    4
#define OTB_NIXIE_INDEX_5    5
#define OTB_NIXIE_INDEX_6    6
#define OTB_NIXIE_INDEX_7    7
#define OTB_NIXIE_INDEX_8    8
#define OTB_NIXIE_INDEX_9    9
#define OTB_NIXIE_INDEX_DP   10
#define OTB_NIXIE_INDEX_NUM  11

#define OTB_NIXIE_INVALID_VALUE  0xffffffff

uint8_t srck_pin = 4; 
uint8_t ser_pin = 5;
uint8_t rck_pin = 13;

char display_bytes[2] = {'0','0'};

typedef struct otb_nixie_index
{
  uint8_t chip;
  uint8_t pin;
} otb_nixie_index;

// First byte is TPIC6B595 instance, second byte is pin
otb_nixie_index otb_nixie_index_left[OTB_NIXIE_INDEX_NUM] =
{
  {1, 1}, // 0
  {0, 0}, // 1
  {0, 1}, // 2
  {0, 2}, // 3
  {0, 3}, // 4
  {0, 4}, // 5
  {0, 5}, // 6
  {0, 6}, // 7
  {0, 7}, // 8
  {1, 0}, // 9
  {1, 2}, // DP
};
otb_nixie_index otb_nixie_index_right[OTB_NIXIE_INDEX_NUM] =
{
  {2, 4}, // 0
  {1, 3}, // 1
  {1, 4}, // 2
  {1, 5}, // 3
  {1, 6}, // 4
  {1, 7}, // 5
  {2, 0}, // 6
  {2, 1}, // 7
  {2, 2}, // 8
  {2, 3}, // 9
  {2, 5}, // DP
};

otb_nixie_index *otb_nixie_indexes[2] =
{
  otb_nixie_index_left,
  otb_nixie_index_right,
};

// If only one byte passed in it's the right one.
uint32_t ICACHE_FLASH_ATTR otb_nixie_get_serial_data(char *bytes, uint8_t num_bytes)
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
    INFO("NIXIE: Invalid bytes passed");
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
        INFO("NIXIE: No dot at first or second");
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
        INFO("NIXIE: No dot at first");
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
      INFO("NIXIE: Invalid number of dots");
      value = OTB_NIXIE_INVALID_VALUE;
      goto EXIT_LABEL;
    }
  }

  which_nixie = num_bytes == 1 ? 1 : 0;

  for (ii = 0; ii < num_bytes; ii++)
  {
    chip = 0xff;
    pin = 0xff;
    temp_value = 0;

    OTB_ASSERT(((bytes[ii] >= '0') && (bytes[ii] <= '9')) ||
               (bytes[ii] == '_') ||
               (bytes[ii] == '.'));
    if (bytes[ii] == '_')
    {
      which_nixie++;
    }
    else
    {
      if (bytes[ii] == '.')
      {
        index = OTB_NIXIE_INDEX_DP;
      }
      else if (bytes[ii] != '_')
      {
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

    INFO("NIXIE: Byte: %d Value: %c Chip: %d Pin: %d Output Value: 0x%06x", ii, bytes[ii], chip, pin, temp_value);

    value |= temp_value;
  }

EXIT_LABEL:

  INFO("NIXIE: Combined Output Value: 0x%06x", value);

  DEBUG("NIXIE: otb_nixie_get_serial_data exit");

  return value;
}

bool ICACHE_FLASH_ATTR otb_nixie_show_value(unsigned char *to_show, uint8_t num_bytes)
{
  uint32_t wait_time = 100;
  uint32_t mask;
  uint32_t serial_data;
  uint32_t value;
  bool rc = TRUE;

  DEBUG("NIXIE: otb_nixie_show_value entry");

  otb_gpio_set(srck_pin, 1, FALSE);
  otb_gpio_set(ser_pin, 1, FALSE);
  otb_gpio_set(rck_pin, 1, FALSE);
  
  serial_data = otb_nixie_get_serial_data(to_show, num_bytes);
  if (serial_data == OTB_NIXIE_INVALID_VALUE)
  {
    rc = FALSE;
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
  
  display_bytes[0]++;
  display_bytes[1]++;
  if (display_bytes[0] > '9')
  {
    display_bytes[0] = '0';
    display_bytes[1] = '0';
  }

EXIT_LABEL:

  DEBUG("NIXIE: otb_nixie_show_value exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_nixie_init(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc;

  DEBUG("NIXIE: otb_nixie_init entry");

  otb_gpio_set(srck_pin, 1, FALSE);
  otb_gpio_set(ser_pin, 1, FALSE);
  otb_gpio_set(rck_pin, 1, FALSE);

  rc = otb_nixie_clear(NULL, NULL, NULL);

  DEBUG("NIXIE: otb_nixie_init exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_nixie_clear(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc;

  DEBUG("NIXIE: otb_nixie_clear entry");

  rc = otb_nixie_show_value("__", 2);

  DEBUG("NIXIE: otb_nixie_clear exit");

  return TRUE;
};

bool ICACHE_FLASH_ATTR otb_nixie_show(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc;
  int num_bytes;

  DEBUG("NIXIE: otb_nixie_show entry");

  if (next_cmd == NULL)
  {
    otb_cmd_rsp_append("no value");
    rc = FALSE;
    goto EXIT_LABEL;
  }

  rc = otb_nixie_show_value(next_cmd, os_strnlen(next_cmd, 5));

EXIT_LABEL:

  DEBUG("NIXIE: otb_nixie_show exit");

  return rc;
};

