#!/usr/bin/python
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

##############################################################################
#
# Craft your own user_config.py based on sample_user_config.py
#
# This script will
# - connect to influxdb
# - attempt to read temperature values from influx for the configured location
#   and time interval
# - if _all_ values for that interval are above or below configured threshold
#   raise an alert
# - if no values were read, raise an alert
#
##############################################################################

##############################################################################
#
# To install InfluxDBClient:
#
#   pip install influxdb
#
##############################################################################

from influxdb import InfluxDBClient
import urllib, urllib2, smtplib, email.message, email.utils
import xml.etree.ElementTree as ET
from user_config import *

# Defines
ALERT_TEXT  = "alert_text"
ALERTED = "alerted"
BEYOND = "beyond"
VALUES = "values"

# location something like 'Big Freezer'
# period something like '15m' (15 minutes)
def get_temp_reading(threshold):
  global influxdb_client
  query = influxdb_client.query("SELECT * from temperature where location='%s' and time > now() - %s" % (threshold[INFLUX_LOC], threshold[PERIOD]))
  results = list(query.get_points())
  return results

def do_alert(threshold, message):
  threshold[ALERT_TEXT] = "ALERT: %s - %s for %s" % (threshold[FRIENDLY_LOC], message, threshold[PERIOD])
  if threshold[ALERT] == SMS:
    encoded_alert_text = urllib.quote(threshold[ALERT_TEXT])
    url = threshold[SMS_URL] % (encoded_alert_text)
    result = urllib2.urlopen(url).read()
    root = ET.fromstring(result)
    success = root.find('resultstring').text
    if success != "success":
      print("ERROR: Failed to send alert SMS - response follows")
      print result
  elif threshold[ALERT] == EMAIL:
    msg = email.message.Message()
    msg['From'] = threshold[EMAIL_FROM]
    msg['To'] = ', '.join(threshold[EMAIL_TO])
    msg['Subject'] = threshold[ALERT_TEXT]
    msg.add_header('Content-Type', 'text')
    msg.set_payload(build_results(threshold))
    smtp = smtplib.SMTP(threshold[EMAIL_SMTP])
    smtp.sendmail(msg['From'], threshold[EMAIL_TO], msg.as_string())
    smtp.quit()

def build_results(threshold):
  results = ""
  results += "Result for location: %s/%s\n" % (threshold[INFLUX_LOC], threshold[FRIENDLY_LOC])
  if threshold[ALERTED]:
    results += "              Alert: %s\n" % (threshold[ALERT_TEXT])
  elif threshold[BEYOND]:
    if threshold[TYPE] == HIGH:
      beyond = "above"
    else:
      beyond = "below"
    results += "              Alert: None, but some values %s threshold\n" % (beyond)
  else:
    results += "              Alert: No\n"
  results += "     Threshold type: %s\n" % (threshold[TYPE])
  results += "             Period: %s\n" % (threshold[PERIOD])
  if threshold[TYPE] != EMPTY:
    results += "              Value: %d\n" % threshold[VALUE]
  results += "               Name: %s\n" % threshold[NAME]
  results += "      Actual values: %s" % threshold[VALUES]
  return results

# Open connection to influxdb and then loop through all configured thresholds
def main():
  global influxdb_client
  influxdb_client = InfluxDBClient(influxdb_host, influxdb_port, influxdb_user, influxdb_password, influxdb_dbname)
  for threshold in thresholds:
    threshold[ALERTED] = False
    threshold[BEYOND] = False
    threshold[VALUES] = ""
    results = get_temp_reading(threshold)
    if threshold[TYPE] == EMPTY:
      if len(results) == 0:
        threshold[ALERTED] = True
        do_alert(threshold, "No readings")
      else:
        threshold[VALUES] += "%d" % len(results)
    else:
      threshold[ALERTED] = True
      threshold[BEYOND] = False
      if len(results) == 0:
        threshold[ALERTED] = False
      for val in results:
        temp = float(val['value'])
        threshold[VALUES] += val['value'] + " "
        if threshold[TYPE] == HIGH:
          if temp <= threshold[VALUE]:
            # Have at least one value below threshold
            threshold[ALERTED] = False
          else:
            threshold[BEYOND] = True
        if threshold[TYPE] == LOW:
          if temp >= threshold[VALUE]:
            # Have a least one value above threshold
            threshold[ALERTED] = False
          else:
            threshold[BEYOND] = True
      if threshold[ALERTED]:
        if threshold[TYPE] == HIGH:
          beyond = "above"
        else:
          beyond = "below"
        do_alert(threshold, "All readings %s threshold %dC" % (beyond, threshold[VALUE], threshold[PERIOD]))
    print(build_results(threshold))
    print("")

if __name__ == "__main__":
  main()