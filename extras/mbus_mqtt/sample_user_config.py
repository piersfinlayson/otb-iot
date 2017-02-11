#
# OTB-IOT - Out of The Box Internet Of Things
#
# Copyright (C) 2017 Piers Finlayson
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.
#

#
# sample_user_config.py
#
# for otbiot_mbus_query.py
#

from otbiot_mbus_query_defines import *

# Address and port of MQTT broker otb-iot devices are attached to
mqtt_addr = "mqtt"
mqtt_port = 1883

# Influxdb configuration
influxdb_host = 'localhost'
influxdb_port = 8086
influxdb_user = 'admin'
influxdb_password = 'admin'
influxdb_dbname = 'mydb'

# Pin to use for TX and RX communication to mbus meter
tx_pin = "15"
rx_pin    = "13"
baud_rate = "2400"

# ADDR must be 2 hex digits, making up byte address
mbus_sensors = [{CHIPID:"123456",
                 ADDRS:[{ADDR:"01",
                         LOCATION:"Wood Boiler"}]},
               ]

# location of mbus_process_hex_dump
mbus_process_hex_dump_cmd = './mbus_process_hex_dump'

