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

include hardware

# SDK versions, etc
SDK_BASE = /opt/esp-open-sdk
ESP_SDK = sdk

# Build tools
XTENSA_DIR = $(SDK_BASE)/xtensa-lx106-elf/bin
OBJDUMP = $(XTENSA_DIR)/xtensa-lx106-elf-objdump
OBJCOPY = $(XTENSA_DIR)/xtensa-lx106-elf-objcopy
NM = $(XTENSA_DIR)/xtensa-lx106-elf-nm
CC = $(XTENSA_DIR)/xtensa-lx106-elf-gcc
LD = $(XTENSA_DIR)/xtensa-lx106-elf-gcc
AR = $(XTENSA_DIR)/xtensa-lx106-elf-ar
ESPTOOL2 = /usr/bin/esptool2
ESPTOOL_PY = $(XTENSA_DIR)/esptool.py

# Compile options
CFLAGS = -Os -Iinclude -I$(SDK_BASE)/sdk/include -mlongcalls -c -ggdb -Wpointer-arith -Wundef -Wno-address -Wl,-El -fno-inline-functions -nostdlib -mtext-section-literals -DICACHE_FLASH -Werror -D__ets__ -Ilib/rboot $(HW_DEFINES)
HTTPD_CFLAGS = -Ilib/httpd -DHTTPD_MAX_CONNECTIONS=5 -std=c99 
RBOOT_CFLAGS = -Ilib/rboot -Ilib/rboot/appcode -DBOOT_BIG_FLASH -DBOOT_CONFIG_CHKSUM -DBOOT_IROM_CHKSUM 
MQTT_CFLAGS = -Ilib/mqtt -Ilib/httpd -std=c99 
OTB_CFLAGS = -Ilib/httpd -Ilib/mqtt -Ilib/rboot -Ilib/rboot/appcode -Ilib/brzo_i2c -std=c99 
I2C_CFLAGS = -Ilib/i2c

# esptool.py options
ESPBAUD = 230400
ESPPORT = /dev/ttyUSB0
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

# Link options
LD_DIR = ld
LDLIBS = -Wl,--start-group -lc -lcirom -lgcc -lhal -lphy -lpp -lnet80211 -lwpa -lmain2 -llwip -lssl -Wl,--end-group
LDFLAGS = -T$(LD_DIR)/eagle.app.v6.ld -nostdlib -Wl,--no-check-sections -Wl,-static -L$(SDK_BASE)/$(ESP_SDK)/lib -u call_user_start -u Cache_Read_Enable_New -Lbin
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

# Object files
otbObjects = $(OTB_OBJ_DIR)/otb_ds18b20.o \
             $(OTB_OBJ_DIR)/otb_mqtt.o \
             $(OTB_OBJ_DIR)/otb_i2c.o \
             $(OTB_OBJ_DIR)/otb_i2c_pca9685.o \
             $(OTB_OBJ_DIR)/otb_i2c_mcp23017.o \
             $(OTB_OBJ_DIR)/otb_i2c_pcf8574.o \
             $(OTB_OBJ_DIR)/otb_i2c_24xxyy.o \
             $(OTB_OBJ_DIR)/otb_led.o \
             $(OTB_OBJ_DIR)/otb_wifi.o \
             $(OTB_OBJ_DIR)/otb_main.o \
             $(OTB_OBJ_DIR)/otb_util.o \
             $(OTB_OBJ_DIR)/otb_rboot.o \
             $(OTB_OBJ_DIR)/otb_gpio.o \
             $(OTB_OBJ_DIR)/otb_conf.o \
             $(OTB_OBJ_DIR)/otb_httpd.o \
             $(RBOOT_OBJ_DIR)/rboot_ota.o \
             $(RBOOT_OBJ_DIR)/rboot-api.o \
             $(RBOOT_OBJ_DIR)/rboot-bigflash.o \
             $(OTB_OBJ_DIR)/strcasecmp.o \
             $(OTB_OBJ_DIR)/pin_map.o 

otbRecoveryObjects = $(OTB_OBJ_DIR)/otb_ds18b20.o \
             $(OTB_OBJ_DIR)/otb_mqtt.o \
             $(OTB_OBJ_DIR)/otb_i2c.o \
             $(OTB_OBJ_DIR)/otb_i2c_pca9685.o \
             $(OTB_OBJ_DIR)/otb_i2c_mcp23017.o \
             $(OTB_OBJ_DIR)/otb_i2c_24xxyy.o \
             $(OTB_OBJ_DIR)/otb_led.o \
             $(OTB_OBJ_DIR)/otb_wifi.o \
             $(OTB_OBJ_DIR)/otb_main.o \
             $(OTB_OBJ_DIR)/otb_util.o \
             $(OTB_OBJ_DIR)/otb_rboot.o \
             $(OTB_OBJ_DIR)/otb_gpio.o \
             $(OTB_OBJ_DIR)/otb_conf.o \
             $(OTB_OBJ_DIR)/otb_httpd.o \
             $(RBOOT_OBJ_DIR)/rboot_ota.o \
             $(RBOOT_OBJ_DIR)/rboot-api.o \
             $(RBOOT_OBJ_DIR)/rboot-bigflash.o \
             $(OTB_OBJ_DIR)/strcasecmp.o \
             $(OTB_OBJ_DIR)/pin_map.o 

mqttObjects = $(MQTT_OBJ_DIR)/mqtt.o \
              $(MQTT_OBJ_DIR)/proto.o \
              $(MQTT_OBJ_DIR)/ringbuf.o \
              $(MQTT_OBJ_DIR)/mqtt_msg.o \
              $(MQTT_OBJ_DIR)/queue.o \
              $(MQTT_OBJ_DIR)/utils.o

httpdObjects = $(HTTPD_OBJ_DIR)/auth.o \
               $(HTTPD_OBJ_DIR)/captdns.o \
               $(HTTPD_OBJ_DIR)/espfs.o \
               $(HTTPD_OBJ_DIR)/heatshrink_decoder.o \
               $(HTTPD_OBJ_DIR)/httpd.o \
               $(HTTPD_OBJ_DIR)/httpd-nonos.o \
               $(HTTPD_OBJ_DIR)/httpdespfs.o \
               $(HTTPD_OBJ_DIR)/sha1.o \
               $(HTTPD_OBJ_DIR)/stdout.o

rbootObjects = $(RBOOT_OBJ_DIR)/rboot.o \
               $(RBOOT_OBJ_DIR)/rboot-stage2a.o \
               $(RBOOT_OBJ_DIR)/appcode/rboot-api.o \
               $(RBOOT_OBJ_DIR)/appcode/rboot-bigflash.o

i2cObjects = $(I2C_OBJ_DIR)/brzo_i2c.o

all: directories bin/app_image.bin bin/rboot.bin

bin/app_image.bin: bin/app_image.elf
	$(NM) -n $^ > bin/symbols
	$(OBJDUMP) -d $^ > bin/disassembly
	$(ESPTOOL2) -bin -iromchksum -boot2 -1024 $^ $@ .text .data .rodata 

bin/app_image.elf: libmain2 otb_objects httpd_objects mqtt_objects i2c_objects obj/html/libwebpages-espfs.a
	$(LD) $(LDFLAGS) -o bin/app_image.elf $(otbObjects) $(httpdObjects) $(mqttObjects) $(i2cObjects) $(LDLIBS) obj/html/libwebpages-espfs.a

# can replace with our own version (from rboot-bigflash.c)
libmain2:
	$(OBJCOPY) -W Cache_Read_Enable_New $(SDK_BASE)/$(ESP_SDK)/lib/libmain.a bin/libmain2.a

otb_objects: clean_otb_util_o $(otbObjects)

httpd_objects: $(httpdObjects)

mqtt_objects: $(mqttObjects)

i2c_objects: $(i2cObjects)

$(OTB_OBJ_DIR)/%.o: $(OTB_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(OTB_CFLAGS) $^ -o $@ 

$(HTTPD_OBJ_DIR)/%.o: $(HTTPD_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(HTTPD_CFLAGS) $^ -o $@ 

$(MQTT_OBJ_DIR)/%.o: $(MQTT_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(MQTT_CFLAGS) $^ -o $@ 

$(I2C_OBJ_DIR)/%.o: $(I2C_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(I2C_CFLAGS) $^ -o $@ 

$(RBOOT_OBJ_DIR)/%.o: $(RBOOT_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(RBOOT_CFLAGS) $(OTB_CFLAGS) $^ -o $@

$(RBOOT_OBJ_DIR)/rboot-stage2a.o: $(RBOOT_SRC_DIR)/rboot-stage2a.c $(RBOOT_SRC_DIR)/rboot-private.h $(RBOOT_SRC_DIR)/rboot.h
	$(CC) $(CFLAGS) $(RBOOT_CFLAGS) -c $< -o $@

bin/rboot-stage2a.elf: $(RBOOT_OBJ_DIR)/rboot-stage2a.o
	$(LD) -T$(LD_DIR)/rboot-stage2a.ld $(RBOOT_LDFLAGS) -Wl,--start-group $^ -Wl,--end-group -o $@

$(RBOOT_OBJ_DIR)/rboot-hex2a.h: bin/rboot-stage2a.elf
	$(ESPTOOL2) -quiet -header $< $@ .text

$(RBOOT_OBJ_DIR)/rboot.o: $(RBOOT_SRC_DIR)/rboot.c $(RBOOT_SRC_DIR)/rboot-private.h $(RBOOT_SRC_DIR)/rboot.h $(RBOOT_OBJ_DIR)/rboot-hex2a.h 
	$(CC) $(CFLAGS) $(RBOOT_CFLAGS) -I$(RBOOT_OBJ_DIR) -c $< -o $@

bin/rboot.elf: $(RBOOT_OBJ_DIR)/rboot.o
	$(LD) -T$(LD_SCRIPT) $(RBOOT_LDFLAGS) -Wl,--start-group $^ -Wl,--end-group -o $@

bin/rboot.bin: bin/rboot.elf
	$(ESPTOOL2) $(E2_OPTS) $< $@ .text .rodata

webpages.espfs:
	cd html; find . | ../tools/mkespfsimage > ../obj/html/webpages.espfs; cd ..

obj/html/libwebpages-espfs.a: webpages.espfs
	$(OBJCOPY) -I binary -O elf32-xtensa-le -B xtensa --rename-section .data=.irom0.literal \
                obj/html/webpages.espfs obj/html/webpages.espfs.o.tmp
	$(LD) -nostdlib -Wl,-r obj/html/webpages.espfs.o.tmp -o obj/html/webpages.espfs.o -Wl,-T ld/webpages.espfs.ld
	$(AR) cru $@ obj/html/webpages.espfs.o

flash_boot: bin/rboot.bin
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash -ff 40m -fs 32m 0x0 bin/rboot.bin

flash_app: bin/app_image.bin
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash 0x2000 bin/app_image.bin

flash_app2: bin/app_image.bin
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash 0x202000 bin/app_image.bin

flash_factory: bin/app_image.bin
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash 0x302000 bin/app_image.bin

flash: flash_boot flash_app

flash_initial: erase_flash flash_boot flash_app flash_factory

connect:
	platformio serialports monitor -b 115200

clean_otb_util_o:
	@rm -f $(OTB_OBJ_DIR)/otb_util.o

clean: 
	@rm -f bin/* $(OTB_OBJ_DIR)/*.o $(HTTPD_OBJ_DIR)/*.o $(RBOOT_OBJ_DIR)/appcode/*.o $(RBOOT_OBJ_DIR)/*.o $(RBOOT_OBJ_DIR)/*.h $(MQTT_OBJ_DIR)/*.o $(I2C_OBJ_DIR)/*.o obj/html/*

erase_flash:
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) erase_flash

flash_40mhz:
	$(ESPTOOL_PY) $(ESPTOOL_PY_OPTS) write_flash 0x3fc000 flash/esp_init_data_40mhz_xtal.hex

directories:
	mkdir -p bin $(OTB_OBJ_DIR) $(HTTPD_OBJ_DIR) $(RBOOT_OBJ_DIR) $(RBOOT_OBJ_DIR) $(RBOOT_OBJ_DIR) $(MQTT_OBJ_DIR) $(I2C_OBJ_DIR) obj/html
