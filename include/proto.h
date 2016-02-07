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

/* 
 * File:   proto.h
 * Author: ThuHien
 *
 * Created on November 23, 2012, 8:57 AM
 */

#ifndef _PROTO_H_
#define	_PROTO_H_
#ifdef OTBARDUINO
#include <Arduino.h>
#endif
#include <stdlib.h>
#include "mqtt_typedef.h"
#include "ringbuf.h"

typedef void(PROTO_PARSE_CALLBACK)();

typedef struct{
	U8 *buf;
	U16 bufSize;
	U16 dataLen;
	U8 isEsc;
	U8 isBegin;
	PROTO_PARSE_CALLBACK* callback;
}PROTO_PARSER;

I8 ICACHE_FLASH_ATTR PROTO_Init(PROTO_PARSER *parser, PROTO_PARSE_CALLBACK *completeCallback, U8 *buf, U16 bufSize);
I8 ICACHE_FLASH_ATTR PROTO_Parse(PROTO_PARSER *parser, U8 *buf, U16 len);
I16 ICACHE_FLASH_ATTR PROTO_Add(U8 *buf, const U8 *packet, I16 bufSize);
I16 ICACHE_FLASH_ATTR PROTO_AddRb(RINGBUF *rb, const U8 *packet, I16 len);
I8 ICACHE_FLASH_ATTR PROTO_ParseByte(PROTO_PARSER *parser, U8 value);
I16 ICACHE_FLASH_ATTR PROTO_ParseRb(RINGBUF *rb, U8 *bufOut, U16* len, U16 maxBufLen);
#endif

