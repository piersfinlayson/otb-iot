#
# OTB-IOT - Out of The Box Internet Of Things
#
# Copyright (C) 2016-8 Piers Finlayson
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

include hardware

# SDK versions, etc
SDK_BASE ?= /opt/esp-open-sdk
ESP_SDK = sdk

# Build tools
XTENSA_DIR = $(SDK_BASE)/xtensa-lx106-elf/bin
OBJDUMP = $(XTENSA_DIR)/xtensa-lx106-elf-objdump
OBJCOPY = $(XTENSA_DIR)/xtensa-lx106-elf-objcopy
NM = $(XTENSA_DIR)/xtensa-lx106-elf-nm
CC = $(XTENSA_DIR)/xtensa-lx106-elf-gcc
LD = $(XTENSA_DIR)/xtensa-lx106-elf-gcc
AR = $(XTENSA_DIR)/xtensa-lx106-elf-ar
ESPTOOL2 = bin/esptool2
ESPTOOL_PY = python2 $(XTENSA_DIR)/esptool.py
MAKE = make

# Serial connection information
SERIAL_PORT ?= /dev/ttyUSB0
SERIAL_BAUD ?= 115200
SERIAL_BAUD_230 = 230400
SERIAL_BAUD_74 = 74880
SERIAL = miniterm.py $(SERIAL_PORT) $(SERIAL_BAUD) --raw

# Compile options
CFLAGS = -Os -Iinclude -Iinclude/boards -I$(SDK_BASE)/sdk/include -mlongcalls -c -ggdb -Wpointer-arith -Wundef -Wno-address -Wl,-El -fno-inline-functions -nostdlib -mtext-section-literals -DICACHE_FLASH -Werror -D__ets__ -Ilib/rboot $(HW_DEFINES) -Ilib/esp8266-software-uart/softuart/include
HTTPD_CFLAGS = -Ilib/httpd -DHTTPD_MAX_CONNECTIONS=5 -std=c99 
RBOOT_CFLAGS = -Ilib/rboot -Ilib/rboot/appcode -Ilib/esp8266-software-uart/softuart/include -Ilib/brzo_i2c -DBOOT_BIG_FLASH -DBOOT_CONFIG_CHKSUM -DBOOT_IROM_CHKSUM -DOTB_RBOOT_BOOTLOADER
MQTT_CFLAGS = -Ilib/mqtt -Ilib/httpd -std=c99 
OTB_CFLAGS = -Ilib/httpd -Ilib/mqtt -Ilib/rboot -Ilib/rboot/appcode -Ilib/brzo_i2c -std=c99 -DOTB_IOT_V0_3
I2C_CFLAGS = -Ilib/i2c -DOTB_TEST
RBOOT_OTHER_CFLAGS = -Os -Iinclude -Iinclude/boards -I$(SDK_BASE)/sdk/include -mlongcalls
HWINFO_CFLAGS = -Iinclude -Iinclude/boards -Iobj/hwinfo -c
STAGE_CFLAGS = -Itools/stage -c
SOFTUART_CFLAGS = $(OTB_CFLAGS) -Ilib/esp8266-software-uart/softuart/include
LIBB64_CFLAGS = $(OTB_CFLAGS) -I lib/libb64/include

# esptool.py options
ESPBAUD = 115200
ESPPORT = $(SERIAL_PORT)
ESPTOOL_PY_OPTS=--port $(ESPPORT) --baud $(ESPBAUD)

# esptool2 options
E2_OPTS = -quiet -bin -boot0
SPI_SIZE = 4M
SPI_SPEED = 40

ifeq ($(SPI_SIZE), 256K)
        E2_OPTS += -256
else ifeq ($(SPI_SIZE), 512K)
        E2_OPTS += -512
else ifeq ($(SPI_SIZE), 1M)
        E2_OPTS += -1024
else ifeq ($(SPI_SIZE), 2M)
        E2_OPTS += -2048
else ifeq ($(SPI_SIZE), 4M)
        E2_OPTS += -4096
else ifeq ($(SPI_SIZE), 8M)
        E2_OPTS += -8192
else ifeq ($(SPI_SIZE), 16M)
        E2_OPTS += -16384
endif
ifeq ($(SPI_MODE), qio)
        E2_OPTS += -qio
else ifeq ($(SPI_MODE), dio)
        E2_OPTS += -dio
else ifeq ($(SPI_MODE), qout)
        E2_OPTS += -qout
else ifeq ($(SPI_MODE), dout)
        E2_OPTS += -dout
endif
ifeq ($(SPI_SPEED), 20)
        E2_OPTS += -20
else ifeq ($(SPI_SPEED), 26)
        E2_OPTS += -26.7
else ifeq ($(SPI_SPEED), 40)
        E2_OPTS += -40
else ifeq ($(SPI_SPEED), 80)
        E2_OPTS += -80
endif

# Check image size
# 983040 is 0xf0000. If factory image is written to 0x305000 and the SDK writes
# at 0x3fc000 then we have a bit over 0xf0000 for the app image - so leave a
# bit of contingency
CHECK_APP_IMAGE_FILE_SIZE = \
        if [ ! -f "bin/app_image.bin" ]; then \
                echo "bin/app_image.bin does not exist" ; exit 1 ; \
        fi; \
        FILE_SIZE=$$(du -b "bin/app_image.bin" | cut -f 1) ; \
        if [ $$FILE_SIZE -gt 983040 ]; then \
                echo "bin/app_image.bin too large" ; exit 1 ; \
        fi

CHECK_STAGE_APP_IMAGE_FILE_SIZE = \
        if [ ! -f "bin/stage_app_image.bin" ]; then \
                echo "bin/stage_app_image.bin does not exist" ; exit 1 ; \
        fi; \
        FILE_SIZE=$$(du -b "bin/stage_app_image.bin" | cut -f 1) ; \
        if [ $$FILE_SIZE -gt 983040 ]; then \
                echo "bin/stage_app_image.bin too large" ; exit 1 ; \
        fi

# Link options
LD_DIR = ld
LDLIBS = -Wl,--start-group -lc -lcirom -lgcc -lhal -lphy -lpp -lnet80211 -lwpa -lmain2 -llwip -Wl,--end-group
BUILD_NUM_FILE = include/otb_build_num.txt
LDFLAGS = -T$(LD_DIR)/eagle.app.v6.ld -nostdlib -Wl,--no-check-sections -Wl,-static -L$(SDK_BASE)/$(ESP_SDK)/lib -u call_user_start -u Cache_Read_Enable_New -Lbin
LDFLAGS += -Xlinker --defsym -Xlinker otb_build_num=$$(cat $(BUILD_NUM_FILE))
RBOOT_LDFLAGS = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static
LD_SCRIPT = $(LD_DIR)/eagle.app.v6.ld

# Source and object directories
OTB_SRC_DIR = src
OTB_OBJ_DIR = obj/otb
HTTPD_SRC_DIR = lib/httpd
HTTPD_OBJ_DIR = obj/httpd
RBOOT_SRC_DIR = lib/rboot
RBOOT_OBJ_DIR = obj/rboot
MQTT_SRC_DIR = lib/mqtt
MQTT_OBJ_DIR = obj/mqtt
I2C_SRC_DIR = lib/brzo_i2c
I2C_OBJ_DIR = obj/brzo_i2c
HWINFO_SRC_DIR = tools/hwinfo
HWINFO_OBJ_DIR = obj/hwinfo
STAGE_SRC_DIR = tools/stage
STAGE_OBJ_DIR = obj/stage
SOFTUART_SRC_DIR = lib/esp8266-software-uart/softuart
SOFTUART_OBJ_DIR = obj/softuart
LIBB64_SRC_DIR = lib/libb64/src
LIBB64_OBJ_DIR = obj/libb64

# Object files
otbObjects = $(OTB_OBJ_DIR)/otb_ds18b20.o \
             $(OTB_OBJ_DIR)/otb_mqtt.o \
             $(OTB_OBJ_DIR)/otb_i2c.o \
             $(OTB_OBJ_DIR)/otb_i2c_pca9685.o \
             $(OTB_OBJ_DIR)/otb_i2c_mcp23017.o \
             $(OTB_OBJ_DIR)/otb_i2c_pcf8574.o \
             $(OTB_OBJ_DIR)/otb_i2c_24xxyy.o \
             $(OTB_OBJ_DIR)/otb_brzo_i2c.o \
             $(OTB_OBJ_DIR)/otb_font.o \
             $(OTB_OBJ_DIR)/otb_led.o \
             $(OTB_OBJ_DIR)/otb_wifi.o \
             $(OTB_OBJ_DIR)/otb_main.o \
             $(OTB_OBJ_DIR)/otb_util.o \
             $(OTB_OBJ_DIR)/otb_rboot.o \
             $(OTB_OBJ_DIR)/otb_gpio.o \
             $(OTB_OBJ_DIR)/otb_conf.o \
             $(OTB_OBJ_DIR)/otb_httpd.o \
             $(OTB_OBJ_DIR)/otb_cmd.o \
             $(OTB_OBJ_DIR)/otb_flash.o \
             $(OTB_OBJ_DIR)/otb_relay.o \
             $(OTB_OBJ_DIR)/otb_eeprom.o \
             $(OTB_OBJ_DIR)/otb_serial.o \
             $(OTB_OBJ_DIR)/otb_nixie.o \
             $(RBOOT_OBJ_DIR)/rboot_ota.o \
             $(RBOOT_OBJ_DIR)/rboot-api.o \
             $(RBOOT_OBJ_DIR)/rboot-bigflash.o \
             $(OTB_OBJ_DIR)/strcasecmp.o \
             $(OTB_OBJ_DIR)/pin_map.o 
otbDep = $(otbObjects:%.o=%.d)

softuartObjects = $(SOFTUART_OBJ_DIR)/softuart.o 
softuartDep = $(softuartObjects:%.o=%.d)

mqttObjects = $(MQTT_OBJ_DIR)/mqtt.o \
              $(MQTT_OBJ_DIR)/proto.o \
              $(MQTT_OBJ_DIR)/ringbuf.o \
              $(MQTT_OBJ_DIR)/mqtt_msg.o \
              $(MQTT_OBJ_DIR)/queue.o \
              $(MQTT_OBJ_DIR)/utils.o
mqttDep = $(mqttObjects:%.o=%.d)

httpdObjects = $(HTTPD_OBJ_DIR)/auth.o \
               $(HTTPD_OBJ_DIR)/captdns.o \
               $(HTTPD_OBJ_DIR)/espfs.o \
               $(HTTPD_OBJ_DIR)/heatshrink_decoder.o \
               $(HTTPD_OBJ_DIR)/httpd.o \
               $(HTTPD_OBJ_DIR)/httpd-nonos.o \
               $(HTTPD_OBJ_DIR)/httpdespfs.o \
               $(HTTPD_OBJ_DIR)/sha1.o \
               $(HTTPD_OBJ_DIR)/stdout.o
httpdDep = $(httpdObjects:%.o=%.d)

rbootObjects = $(RBOOT_OBJ_DIR)/rboot.o \
               $(RBOOT_OBJ_DIR)/rboot-stage2a.o \
               $(RBOOT_OBJ_DIR)/otb_i2c_24xxyy.o \
               $(RBOOT_OBJ_DIR)/brzo_i2c.o \
               $(RBOOT_OBJ_DIR)/otb_brzo_i2c.o \
               $(RBOOT_OBJ_DIR)/otb_eeprom.o \
               $(RBOOT_OBJ_DIR)/otb_i2c.o \
               $(RBOOT_OBJ_DIR)/appcode/rboot-api.o \
               $(RBOOT_OBJ_DIR)/appcode/rboot-bigflash.o \
               $(RBOOT_OBJ_DIR)/pin_map.o
rbootDep = $(rbootObjects:%.o=%.d)

libb64Objects = $(LIBB64_OBJ_DIR)/cdecode.o \
                $(LIBB64_OBJ_DIR)/cencode.o
libb64Dep = $(libb64Objects:%.o=%.d)

i2cObjects = $(I2C_OBJ_DIR)/brzo_i2c.o
i2cDep = $(i2cObjects:%.o=%.d)

hwinfoObjects = $(HWINFO_OBJ_DIR)/otb_hwinfo.o
hwinfoDep = $(hwinfoObjects:%.o=%.d)

stageObjects = $(STAGE_OBJ_DIR)/otb_stage.o
stageDep = $(stageObjects:%.o=%.d)

all: directories bin/app_image.bin bin/rboot.bin docs

bin/app_image.bin: bin/app_image.elf $(ESPTOOL2)
	$(NM) -n bin/app_image.elf > bin/app_image.sym
	$(OBJDUMP) -d bin/app_image.elf > bin/app_image.dis
	$(ESPTOOL2) -bin -iromchksum -boot2 -1024 bin/app_image.elf $@ .text .data .rodata 
	@$(CHECK_APP_IMAGE_FILE_SIZE)

bin/stage_app_image.bin: bin/stage_app_image.elf $(ESPTOOL2)
	$(NM) -n bin/stage_app_image.elf > bin/stage_app_image.sym
	$(OBJDUMP) -d bin/stage_app_image.elf > bin/stage_app_image.dis
	$(ESPTOOL2) -bin -iromchksum -boot2 -1024 bin/stage_app_image.elf $@ .text .data .rodata 
	@$(CHECK_STAGE_APP_IMAGE_FILE_SIZE)

# Increment build number
build_num: libmain2 otb_objects httpd_objects mqtt_objects i2c_objects softuart_objects libb64_objects obj/html/libwebpages-espfs.a
	@if ! test -f $(BUILD_NUM_FILE); then echo 0 > $(BUILD_NUM_FILE); fi
	@echo $$(($$(cat $(BUILD_NUM_FILE))+1)) > $(BUILD_NUM_FILE)

bin/app_image.elf: build_num libmain2 otb_objects httpd_objects mqtt_objects i2c_objects softuart_objects libb64_objects obj/html/libwebpages-espfs.a
	$(LD) $(LDFLAGS) -o bin/app_image.elf $(otbObjects) $(httpdObjects) $(mqttObjects) $(i2cObjects) $(softuartObjects) $(LDLIBS) obj/html/libwebpages-espfs.a

bin/stage_app_image.elf: build_num libmain2 stage_objects
	$(LD) $(LDFLAGS) -o bin/stage_app_image.elf $(stageObjects) $(LDLIBS) 

-include $(otbDep) $(mqttDep) $(httpdDep) $(rbootDep) $(i2cDep) $(softuartDep) $(libb64Dep) $(hwinfoDep)

sdk_init_data.bin: directories
	@rm -f $(HWINFO_OBJ_DIR)/sdk_init_data.bin
	ln -s $(SDK_BASE)/$(ESP_SDK)/bin/esp_init_data_default.bin $(HWINFO_OBJ_DIR)/sdk_init_data.bin

otb_hwinfo_sdk_init_data.h: sdk_init_data.bin
	xxd -i $(HWINFO_OBJ_DIR)/sdk_init_data.bin $(HWINFO_OBJ_DIR)/otb_hwinfo_sdk_init_data.h

hwinfo: otb_hwinfo_sdk_init_data.h $(hwinfoObjects)
	gcc $(hwinfoObjects) -lc -o bin/$@

stage: $(stageObjects)
	$(LD) $(LDFLAGS) -o bin/stage_image.elf $(stageObjects) $(LDLIBS)

# can replace with our own version (from rboot-bigflash.c)
libmain2: directories
	$(OBJCOPY) -W Cache_Read_Enable_New $(SDK_BASE)/$(ESP_SDK)/lib/libmain.a bin/libmain2.a

#otb_objects: clean_otb_util_o $(otbObjects)
otb_objects: $(otbObjects)

httpd_objects: $(httpdObjects)

mqtt_objects: $(mqttObjects)

i2c_objects: $(i2cObjects)

hwinfo_objects: $(hwinfoObjects)

stage_objects: $(stageObjects)

softuart_objects: $(softuartObjects)

libb64_objects: $(libb64Objects)

$(HWINFO_OBJ_DIR)/%.o: $(HWINFO_SRC_DIR)/%.c
	gcc $(HWINFO_CFLAGS) -MMD -c $< -o $@

$(STAGE_OBJ_DIR)/%.o: $(STAGE_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(STAGE_CFLAGS) -MMD -c $< -o $@ 

$(OTB_OBJ_DIR)/%.o: $(OTB_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(OTB_CFLAGS) -MMD -c $< -o $@ 

$(HTTPD_OBJ_DIR)/%.o: $(HTTPD_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(HTTPD_CFLAGS) -MMD -c $< -o $@ 

$(MQTT_OBJ_DIR)/%.o: $(MQTT_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(OTB_CFLAGS) -MMD -c $< -o $@ 

$(I2C_OBJ_DIR)/%.o: $(I2C_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(I2C_CFLAGS) -MMD -c $< -o $@ 

$(SOFTUART_OBJ_DIR)/%.o: $(SOFTUART_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(SOFTUART_CFLAGS) -MMD -c $< -o $@ 

$(LIBB64_OBJ_DIR)/%.o: $(LIBB64_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(LIBB64_CFLAGS) -MMD -c $< -o $@ 

$(RBOOT_OBJ_DIR)/%.o: $(RBOOT_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(RBOOT_CFLAGS) $(OTB_CFLAGS) -MMD -c $< -o $@

$(RBOOT_OBJ_DIR)/pin_map.o: $(OTB_SRC_DIR)/pin_map.c
	$(CC) $(RBOOT_OTHER_CFLAGS) $(RBOOT_CFLAGS) -Iinclude -I$(SDK_BASE)/sdk/include -Ilib/httpd -Ilib/mqtt -Ilib/rboot -Ilib/rboot/appcode -Ilib/i2c -Ilib/mqtt -Ilib/httpd -Ilib/brzo_i2c $(RBOOT_CFLAGS) -c $< -o $@

$(RBOOT_OBJ_DIR)/otb_i2c_24xxyy.o: $(OTB_SRC_DIR)/otb_i2c_24xxyy.c
	$(CC) $(RBOOT_OTHER_CFLAGS) $(RBOOT_CFLAGS) -Iinclude -I$(SDK_BASE)/sdk/include -Ilib/httpd -Ilib/mqtt -Ilib/rboot -Ilib/rboot/appcode -Ilib/i2c -Ilib/mqtt -Ilib/httpd -Ilib/brzo_i2c $(RBOOT_CFLAGS) -c $< -o $@

$(RBOOT_OBJ_DIR)/brzo_i2c.o: $(I2C_SRC_DIR)/brzo_i2c.c
	$(CC) $(RBOOT_OTHER_CFLAGS) $(I2C_CFLAGS) $(RBOOT_CFLAGS) -MMD -c $< -o $@

$(RBOOT_OBJ_DIR)/otb_brzo_i2c.o: $(OTB_SRC_DIR)/otb_brzo_i2c.c
	$(CC) $(RBOOT_OTHER_CFLAGS) $(RBOOT_CFLAGS) -Iinclude -I$(SDK_BASE)/sdk/include -Ilib/httpd -Ilib/mqtt -Ilib/rboot -Ilib/rboot/appcode -Ilib/i2c -Ilib/mqtt -Ilib/httpd -Ilib/brzo_i2c $(RBOOT_CFLAGS) -c $< -o $@

$(RBOOT_OBJ_DIR)/otb_i2c.o: $(OTB_SRC_DIR)/otb_i2c.c
	$(CC) $(RBOOT_OTHER_CFLAGS) $(RBOOT_CFLAGS) -Iinclude -I$(SDK_BASE)/sdk/include -Ilib/httpd -Ilib/mqtt -Ilib/rboot -Ilib/rboot/appcode -Ilib/i2c -Ilib/mqtt -Ilib/httpd -Ilib/brzo_i2c $(RBOOT_CFLAGS) -c $< -o $@

$(RBOOT_OBJ_DIR)/otb_eeprom.o: $(OTB_SRC_DIR)/otb_eeprom.c
	$(CC) $(RBOOT_OTHER_CFLAGS) $(RBOOT_CFLAGS) -Iinclude -I$(SDK_BASE)/sdk/include -Ilib/httpd -Ilib/mqtt -Ilib/rboot -Ilib/rboot/appcode -Ilib/i2c -Ilib/mqtt -Ilib/httpd -Ilib/brzo_i2c $(RBOOT_CFLAGS) -c $< -o $@

$(RBOOT_OBJ_DIR)/rboot-stage2a.o: $(RBOOT_SRC_DIR)/rboot-stage2a.c $(RBOOT_SRC_DIR)/rboot-private.h $(RBOOT_SRC_DIR)/rboot.h
	$(CC) $(CFLAGS) $(RBOOT_CFLAGS) -c $< -o $@

bin/rboot-stage2a.elf: $(RBOOT_OBJ_DIR)/rboot-stage2a.o
	$(LD) -T$(LD_DIR)/rboot-stage2a.ld $(RBOOT_LDFLAGS) -Wl,--start-group $^ -Wl,--end-group -o $@

$(RBOOT_OBJ_DIR)/rboot-hex2a.h: bin/rboot-stage2a.elf $(ESPTOOL2)
	$(ESPTOOL2) -quiet -header bin/rboot-stage2a.elf $@ .text

$(RBOOT_OBJ_DIR)/rboot.o: $(RBOOT_SRC_DIR)/rboot.c $(RBOOT_SRC_DIR)/rboot-private.h $(RBOOT_SRC_DIR)/rboot.h $(RBOOT_OBJ_DIR)/rboot-hex2a.h 
	$(CC) $(CFLAGS) $(RBOOT_CFLAGS) -I$(RBOOT_OBJ_DIR) -c $< -o $@

bin/rboot.elf: $(RBOOT_OBJ_DIR)/rboot.o $(RBOOT_OBJ_DIR)/pin_map.o $(RBOOT_OBJ_DIR)/otb_i2c.o $(RBOOT_OBJ_DIR)/otb_eeprom.o $(RBOOT_OBJ_DIR)/otb_brzo_i2c.o $(RBOOT_OBJ_DIR)/brzo_i2c.o $(RBOOT_OBJ_DIR)/otb_i2c_24xxyy.o
	$(LD) -T$(LD_SCRIPT) $(RBOOT_LDFLAGS) -Wl,--start-group $^ -Wl,--end-group -o $@

bin/rboot.bin: bin/rboot.elf $(ESPTOOL2)
	$(NM) -n bin/rboot.elf > bin/rboot.sym
	$(OBJDUMP) -d bin/rboot.elf > bin/rboot.dis
	$(ESPTOOL2) $(E2_OPTS) bin/rboot.elf $@ .text .rodata .bss .data

$(ESPTOOL2):
	$(MAKE) -C external/esptool2; cp external/esptool2/esptool2 $(ESPTOOL2)

webpages.espfs: mkespfsimage
	cd html; find . | ../bin/mkespfsimage > ../obj/html/webpages.espfs; cd ..

mkespfsimage:
	$(MAKE) -C external/libesphttpd/espfs/mkespfsimage; cp external/libesphttpd/espfs/mkespfsimage/mkespfsimage bin/

obj/html/libwebpages-espfs.a: webpages.espfs
	$(OBJCOPY) -I binary -O elf32-xtensa-le -B xtensa --rename-section .data=.irom0.literal \
                obj/html/webpages.espfs obj/html/webpages.espfs.o.tmp
	$(LD) -nostdlib -Wl,-r obj/html/webpages.espfs.o.tmp -o obj/html/webpages.espfs.o -Wl,-T ld/webpages.espfs.ld
	$(AR) cru $@ obj/html/webpages.espfs.o

flash_boot: bin/rboot.bin
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash -fm dio -ff 40m -fs 32m 0x0 bin/rboot.bin

flash_app: bin/app_image.bin
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash 0x8000 bin/app_image.bin

flash_app2: bin/app_image.bin
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash 0x208000 bin/app_image.bin

flash_factory: bin/app_image.bin
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash 0x308000 bin/app_image.bin

flash_stage_app: bin/stage_app_image.bin
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash 0x8000 bin/stage_app_image.bin

flash: flash_boot flash_app

flash_sdk:
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash 0x3fc000 $(SDK_BASE)/$(ESP_SDK)/bin/esp_init_data_default.bin

flash_initial: flash_otbiot

flash_otbiot: directories erase_flash flash_sdk flash_boot flash_app flash_factory

flash_stage: directories erase_flash flash_sdk flash_boot flash_stage_app

flash_initial_40mhz: directories erase_flash flash_boot flash_app flash_factory flash_40mhz

con: connect

connect:
	miniterm.py $(SERIAL_PORT) $(SERIAL_BAUD) --raw

con230:
	miniterm.py $(SERIAL_PORT) $(SERIAL_BAUD_230) --raw

con74:
	miniterm.py $(SERIAL_PORT) $(SERIAL_BAUD_74) --raw

clean_otb_util_o: FORCE
	@rm -f $(OTB_OBJ_DIR)/otb_util.o

clean: clean_esptool2 clean_mkespfsimage clean_i2c-tools clean_hwinfo clean_docs clean_stage
	@rm -fr bin obj 

clean_hwinfo: FORCE
	@rm -f $(HWINFO_OBJ_DIR)/*

clean_stage: FORCE
	@rm -f $(STAGE_OBJ_DIR)/*

clean_esptool2: FORCE
	@rm -f external/esptool2/*.o esptool2

clean_mkespfsimage: FORCE
	@rm -f external/libesphttpd/espfs/mkespfsimage/*.o external/libesphttpd/espfs/mkespfsimage/mkespfsimage

clean_i2c-tools: FORCE
	$(MAKE) -C external/i2c-tools EXTRA=eeprog clean
	@rm -f bin/eeprog

i2c-tools: directories
	$(MAKE) -C external/i2c-tools EXTRA=eeprog USE_STATIC_LIB=1
	cp external/i2c-tools/eeprog/eeprog bin/

erase_flash:
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) erase_flash

flash_40mhz:
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash 0x3fc000 flash/esp_init_data_40mhz_xtal.hex

directories:
	mkdir -p bin $(OTB_OBJ_DIR) $(HTTPD_OBJ_DIR) $(RBOOT_OBJ_DIR) $(RBOOT_OBJ_DIR) $(RBOOT_OBJ_DIR) $(MQTT_OBJ_DIR) $(I2C_OBJ_DIR) $(HWINFO_OBJ_DIR) $(STAGE_OBJ_DIR) $(SOFTUART_OBJ_DIR) $(LIBB64_OBJ_DIR) obj/html

docs: FORCE
	-$(MAKE) -C docs html

clean_docs: FORCE
	-$(MAKE) -C docs clean

clean_nweb: FORCE
	@rm -f bin/nweb

nweb: FORCE directories
	gcc external/nweb/nweb23.c -o bin/nweb

mbus_tools: FORCE
	cd extras/mbus_tools;. ./build.sh

FORCE:

