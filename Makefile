#
# IOT - Out of The Box Internet Of Things
#
# Copyright (C) 2016-2020 Piers Finlayson
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

MAKEFLAGS += --no-print-directory

IDF_PATH ?= /home/build/ESP8266_RTOS_SDK
IDF_TOOLS ?= $(IDF_PATH)/tools
IDF_PY = $(IDF_TOOLS)/idf.py

# Serial connection information
SERIAL_PORT ?= /dev/ttyUSB0
SERIAL_BAUD ?= 115200
SERIAL_BAUD_230 = 230400
SERIAL_BAUD_74 = 74880
SERIAL_CMD = python2.7 /usr/lib/python2.7/dist-packages/serial/tools/miniterm.py
SERIAL = $(SERIAL_CMD) $(SERIAL_PORT) $(SERIAL_BAUD) --raw

all:
	$(IDF_PY) all

build:
	$(IDF_PY) build

flash_app: app-flash

flash-app: app-flash

app_flash: app-flash

app-flash:
	$(IDF_PY) app-flash

flash_erase: erase_flash

flash-erase: erase_flash

erase-flash: erase_flash

erase_flash:
	$(IDF_PY) erase_flash

flash:
	$(IDF_PY) flash

clean:
	$(IDF_PY) clean

con: con74

connect:
	$(SERIAL_CMD) $(SERIAL_PORT) $(SERIAL_BAUD) --raw

con230:
	$(SERIAL_CMD) $(SERIAL_PORT) $(SERIAL_BAUD_230) --raw

con74:
	$(SERIAL_CMD) $(SERIAL_PORT) $(SERIAL_BAUD_74) --raw

