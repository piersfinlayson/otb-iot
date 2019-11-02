/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2019 Piers Finlayson
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

void ICACHE_FLASH_ATTR otb_break_start(void)
{
  DEBUG("BREAK: otb_break_start entry");

  otb_break_rx_buf_len = 0;
  otb_break_state = OTB_BREAK_STATE_MAIN;
  INFO("Entered otb-iot break processing");
  INFO("Enable watchdog");
  otb_util_break_enable_timer(OTB_BREAK_WATCHDOG_TIMER);
  INFO(" Watchdog enabled: %dms", OTB_BREAK_WATCHDOG_TIMER)
  otb_util_uart0_rx_en();
  INFO("Press h or ? for help");

  DEBUG("BREAK: otb_break_start exit");

  return;
}

void ICACHE_FLASH_ATTR otb_break_options_output(void)
{

  DEBUG("BREAK: otb_break_options_output entry");

  INFO("otb-iot break options:")
  INFO("  c - change config (changes do not persist)");
  INFO("  x - resumes booting (with changed config is applicable");
  INFO("  q - reboot");
  INFO("  f - factory reset");
  INFO("  g - run gpio test");
  INFO("  r - waits for soft reset button to be pressed");
  INFO("  i - outputs device info");
  INFO("  w - wipe stored config");
  INFO("  d - disable watchdog (device will not reboot automatically after 5 minutes)");
  INFO("  e - enable watchdog (device will rebot after 5 minutes)");
  INFO("  h - output this list of options");

  DEBUG("BREAK: otb_break_options_output exit");

}  

void ICACHE_FLASH_ATTR otb_break_process_char_timerfunc(void *arg)
{
  char rx_char;

  DEBUG("BREAK: otb_break_process_char_timerfunc entry");

  otb_util_timer_cancel((os_timer_t*)&otb_break_process_char_timer);
  OTB_ASSERT(otb_break_rx_buf > 0);
  otb_break_options_fan_out(otb_break_rx_buf[0]);

  DEBUG("BREAK: otb_break_process_char_timerfunc exit");

  return;
}

void ICACHE_FLASH_ATTR otb_break_options_fan_out(char input)
{
  bool rc = TRUE;

  DEBUG("BREAK: otb_break_options_fan_out entry");

  switch (otb_break_state)
  {
    case OTB_BREAK_STATE_MAIN:
      rc = otb_break_options_select(input);
      break;

    case OTB_BREAK_STATE_CONFIG:
      break;

    case OTB_BREAK_STATE_GPIO:
      otb_break_gpio_input(input);
      break;

    case OTB_BREAK_STATE_SOFT_RESET:
      break;

    default:
      OTB_ASSERT(FALSE);
      break;
  }

  if (rc)
  {
    otb_break_rx_buf_len = 0;
  }

  DEBUG("BREAK: otb_break_options_fan_out exit");

  return;
}

char ALIGN4 otb_break_user_reboot_string[] = "BREAK: User triggered reboot";
char ALIGN4 otb_break_config_reboot_string[] = "BREAK: User triggered config wipe";
bool ICACHE_FLASH_ATTR otb_break_options_select(char option)
{
  bool output_options = FALSE;
  bool clear_buf = TRUE;

  DEBUG("BREAK: otb_break_options_select entry");

  switch (option)
  {
    case 'c':
    case 'C':
      INFO("Change config");
      INFO(" Not yet implemented");
      break;

    case 'x':
    case 'X':
      INFO("Resume booting");
      otb_util_uart0_rx_dis();
      otb_util_break_disable_timer();
      otb_wifi_kick_off();
      output_options = FALSE;
      break;

    case 'q':
    case 'Q':
      INFO("Reboot");
      otb_reset(otb_break_user_reboot_string);
      output_options = FALSE;
      break;

    case 'f':
    case 'F':
      INFO("Factory reset");
      INFO(" Not yet implemented");
      //output_options = FALSE;
      break;

    case 'g':
    case 'G':
      INFO("Run GPIO test");
      otb_break_state = OTB_BREAK_STATE_GPIO;
      INFO(" GPIO test running (test not yet implemented)");
      break;

    case 'r':
    case 'R':
      INFO("Soft reset button test");
      INFO(" Not yet implemented");
      break;

    case 'i':
    case 'I':
      INFO("Output device info");
      INFO(" Not yet implemented");
      break;

    case 'w':
    case 'W':
      INFO("Wipe stored config");
      uint8 spi_rc = spi_flash_erase_sector(OTB_BOOT_CONF_LOCATION / 0x1000);
      INFO(" Wiped");
      otb_reset(otb_break_config_reboot_string);
      output_options = FALSE;
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

  DEBUG("BREAK: otb_break_options_select exit");

  return(clear_buf);
}

// Called from within interrupts
bool ICACHE_FLASH_ATTR otb_break_gpio_input(char input)
{

  DEBUG("BREAK: otb_break_gpio_input entry");

  switch (input)
  {
    case 'x':
    case 'X':
      otb_break_state = OTB_BREAK_STATE_MAIN;
      INFO(" Terminated")
      break;

    case 'h':
    case 'H':
    case '?':
      INFO("  x - Terminate");
      break;

    default:
      break;
  }

  DEBUG("BREAK: otb_break_gpio_input exit");

  return(TRUE);
}

void otb_break_process_char(void)
{
  otb_util_timer_set((os_timer_t*)&otb_break_process_char_timer, 
                    (os_timer_func_t *)otb_break_process_char_timerfunc,
                    NULL,
                    0,
                    0);

  return;
}

