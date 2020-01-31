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
# for otbiot_mqtt_influxdb.py.py
#

from otbiot_mqtt_influxdb_defines import *

# Address and port of MQTT broker otb-iot devices are attached to
mqtt_addr = "mqtt"
mqtt_port = 1883

# Influxdb configuration
influxdb_host = 'localhost'
influxdb_port = 8086
influxdb_user = 'admin'
influxdb_password = 'admin'
influxdb_dbname = 'mydb'

# DS18B20 temperature sensors - here we have 3 DS18B20 temperature sensors
# across 2 otb-iot devices
temp_sensors = [{CHIPID:"123456",
                 SENSORS:[{SENSORID:"28-1122334455ff",
                           LOCATION:"Location 1"},
                          {SENSORID:"28-2233445566ff",
                           LOCATION:"Location 2"}]},
                {CHIPID:"abcdef",
                 SENSORS:[{SENSORID:"28-3344556677ff",
                           LOCATION:"Location 3"}]},
               ]
               
# Power sensors - here, just one
power_sensors = [{CHIPID:"123456",
                  SENSORS:[{LOCATION:"Sample Location"}]},
                ]
