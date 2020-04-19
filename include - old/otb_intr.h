/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2018 Piers Finlayson
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
 * 
 */

#ifndef OTB_INTR_H_INCLUDED
#define OTB_INTR_H_INCLUDED

typedef void (otb_intr_handler_fn)(void *arg);

typedef struct otb_intr_reg
{
  otb_intr_handler_fn *fn;

  void *arg;

} otb_intr_reg;

extern otb_intr_reg otb_intr_reg_info[OTB_GPIO_ESP_GPIO_PINS];
#ifdef OTB_INTR_C
otb_intr_reg otb_intr_reg_info[OTB_GPIO_ESP_GPIO_PINS];
#endif // OTB_INTR_C

void ICACHE_FLASH_ATTR otb_intr_init(void);
void ICACHE_FLASH_ATTR otb_intr_clear(uint32_t gpio_status);
void ICACHE_FLASH_ATTR otb_intr_set();
bool ICACHE_FLASH_ATTR otb_intr_register(otb_intr_handler_fn *fn, void *arg, uint8_t pin);
void ICACHE_FLASH_ATTR otb_intr_unreg(uint8_t pin);
void ICACHE_FLASH_ATTR otb_intr_main_handler(void *arg);

#endif // OTB_INTR_H_INCLUDED
