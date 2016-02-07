/*
 *
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
 * 
 */

#ifndef _RING_BUF_H_
#define _RING_BUF_H_

#ifdef OTB_ARDUINO
#include <Arduino.h>
#endif
#include "osapi.h"
#include <os_type.h>
#include <stdlib.h>
#include "mqtt_typedef.h"

typedef struct{
	U8* p_o;				/**< Original pointer */
	U8* volatile p_r;		/**< Read pointer */
	U8* volatile p_w;		/**< Write pointer */
	volatile I32 fill_cnt;	/**< Number of filled slots */
	I32 size;				/**< Buffer size */
}RINGBUF;

I16 ICACHE_FLASH_ATTR RINGBUF_Init(RINGBUF *r, U8* buf, I32 size);
I16 ICACHE_FLASH_ATTR RINGBUF_Put(RINGBUF *r, U8 c);
I16 ICACHE_FLASH_ATTR RINGBUF_Get(RINGBUF *r, U8* c);
#endif
