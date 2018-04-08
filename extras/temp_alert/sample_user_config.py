#
# OTB-IOT - Out of The Box Internet Of Things
#
# Copyright (C) 2018 Piers Finlayson
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
# for temp_alert.py
#

from temp_alert_defines import *

# Influxdb configuration
influxdb_host = 'influxdb'
influxdb_port = 8086
influxdb_user = 'admin'
influxdb_password = 'admin'
influxdb_dbname = 'mydb'

# Thresholds
thresholds = [
              {NAME: "No values",
               TYPE: EMPTY,
               PERIOD: "60m",
               ALERT: EMAIL,
               INFLUX_LOC: "Freezer",
               FRIENDLY_LOC: "Freezer",
               EMAIL_FROM: "freezer@example.com",
               EMAIL_TO: ["warn@example.com", ],
               EMAIL_SMTP: "smtp.example.com"},
              {NAME: "Warn",
               TYPE: HIGH,
               PERIOD: "60m",
               VALUE: -16,
               ALERT: EMAIL,
               INFLUX_LOC: "Freezer",
               FRIENDLY_LOC: "Freezer",
               EMAIL_FROM: "freezer@example.com",
               EMAIL_TO: ["warn@example.com", ],
               EMAIL_SMTP: "smtp.example.com"},
              {NAME: "Error",
               TYPE: HIGH,
               PERIOD: "60m",
               VALUE: -8,
               ALERT: SMS,
               INFLUX_LOC: "Freezer",
               FRIENDLY_LOC: "Freezer",
               SMS_URL: "https://www.voipcheap.co.uk/myaccount/sendsms.php?username=username&password=password&from=+441234567890&to=+441234567890&text=%s"},
             ]
