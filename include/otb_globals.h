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

extern char OTB_MAIN_CHIPID[OTB_MAIN_CHIPID_STR_LENGTH];
extern char otb_log_s[OTB_MAIN_MAX_LOG_LENGTH];
extern MQTT_Client otb_mqtt_client;
extern char otb_mqtt_topic_s[OTB_MQTT_MAX_TOPIC_LENGTH];
extern char otb_mqtt_msg_s[OTB_MQTT_MAX_MSG_LENGTH];
extern char otb_compile_date[12];
extern char otb_compile_time[9];
extern char otb_version_id[OTB_MAIN_MAX_VERSION_LENGTH];
extern bool otb_wifi_ap_mode_done;
extern char OTB_MAIN_DEVICE_ID[20];
