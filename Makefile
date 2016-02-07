#
# OTB-IOT - Out of The Box Internet Of Things
#
# Copyright (C) 2016 Piers Finlayson
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.
#

XTENSA_DIR = $(SDK_BASE)/xtensa-lx106-elf/bin
HTTPD_SRC_DIR = lib/httpd/src
HTTPD_OBJ_DIR = lib/httpd/obj
OBJDUMP = $(XTENSA_DIR)/xtensa-lx106-elf-objdump
NM = $(XTENSA_DIR)/xtensa-lx106-elf-nm
CC = $(XTENSA_DIR)/xtensa-lx106-elf-gcc
#CFLAGS = -Os -Iinclude -Isrc -Ilib/httpd/src -I$(SDK_BASE)/sdk/include -mlongcalls -std=c99 -c -ggdb -Werror -Wpointer-arith -Wundef -Wno-address -Wl,-El -fno-inline-functions -nostdlib -mtext-section-literals -DICACHE_FLASH
CFLAGS = -Os -Iinclude -Isrc -Ilib/httpd/src -I$(SDK_BASE)/sdk/include -mlongcalls -std=c99 -c -ggdb -Wpointer-arith -Wundef -Wno-address -Wl,-El -fno-inline-functions -nostdlib -mtext-section-literals -DICACHE_FLASH
HTTPD_CFLAGS = -D_STDINT_H -D__ets__ -DICACHE_FLASH
LDLIBS = -Wl,--start-group -lc -lcirom -lgcc -lhal -lphy -lpp -lnet80211 -lwpa -lmain -llwip -Wl,--end-group
LDFLAGS = -Teagle.app.v6.ld -nostdlib -Wl,--no-check-sections -Wl,-static -L$(SDK_BASE)/sdk/lib -L$(SDK_BASE)/esp_iot_sdk_v1.4.0/lib -u call_user_start 
otbObjects = obj/mqtt.o obj/otb_ds18b20.o obj/otb_mqtt.o obj/otb_wifi.o obj/proto.o obj/ringbuf.o obj/mqtt_msg.o obj/otb_main.o obj/otb_util.o obj/pin_map.o obj/queue.o obj/utils.o
httpdObjects = $(HTTPD_OBJ_DIR)/auth.o $(HTTPD_OBJ_DIR)/base64.o $(HTTPD_OBJ_DIR)/captdns.o $(HTTPD_OBJ_DIR)/cgiflash.o $(HTTPD_OBJ_DIR)/cgiwebsocket.o $(HTTPD_OBJ_DIR)/cgiwifi.o $(HTTPD_OBJ_DIR)/espfs.o $(HTTPD_OBJ_DIR)/heatshrink_decoder.o $(HTTPD_OBJ_DIR)/httpd.o $(HTTPD_OBJ_DIR)/httpdespfs.o $(HTTPD_OBJ_DIR)/sha1.o $(HTTPD_OBJ_DIR)/stdout.o

app_image-0x00000.bin: bin/app_image
	$(NM) -n $^ > bin/symbols
	$(OBJDUMP) -d $^ > bin/disassembly
	esptool.py elf2image $^

bin/app_image: otb_objects httpd_objects
	$(CC) $(LDFLAGS) $(LDLIBS) -o bin/app_image $(otbObjects) $(httpdObjects)

otb_objects: $(otbObjects)

httpd_objects: $(httpdObjects)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) $^ -o $@ 

$(HTTPD_OBJ_DIR)/%.o: $(HTTPD_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(HTTPD_CFLAGS) $^ -o $@ 

flash: app_image-0x00000.bin
	esptool.py write_flash 0x0 bin/app_image-0x00000.bin 0x40000 bin/app_image-0x40000.bin

connect:
	platformio serialports monitor -b 115200

clean: 
	@rm -f bin/* obj/* $(HTTPD_OBJ_DIR)/*

