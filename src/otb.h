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

#include <Arduino.h>
//#include "user_interface.h"
#include "osapi.h"
#include <lwip/ip.h>
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "mqtt_msg.h"
#include "mqtt_user_config.h"
#include "mqtt.h"
#include "queue.h"
#include "otb_fns.h"
#include "otb_def.h"


// Globals
extern char chipid[CHIPID_STR_LENGTH];
extern char log_s[MAX_LOG_LENGTH];
void (*log_fn)(char *);
extern MQTT_Client mqttClient;
void reset(void);
extern char topic_s[MAX_TOPIC_LENGTH];
extern char compile_date[12];
extern char compile_time[9];
extern char version_id[MAX_VERSION_ID_LENGTH];

// Macros
#define LOG(args...) sprintf(log_s, args); log_fn(log_s)
#define INFO LOG

