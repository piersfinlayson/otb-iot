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
 
 #ifndef OTB_RBOOT_H
 #define OTB_RBOOT_H

void otb_rboot_update_callback(void *arg, bool result);
bool otb_rboot_update(char *ip, char *port, char *path);
bool otb_rboot_update_slot(char *msg);
uint8_t otb_rboot_get_slot(bool publish);
bool ota_rboot_check_factory_image(void);
bool ota_rboot_use_factory_image(void);

#endif // OTB_RBOOT_H
