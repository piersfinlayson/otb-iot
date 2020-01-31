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
# - go through all temp and power sensors, pinging them
# - go through all temp sensors, making querying the number of sensors
#
##############################################################################

##############################################################################
#
# To install mosquitto module:
#
#   pip install paho-mqtt
#
##############################################################################

import paho.mqtt.client as mqtt
import time, datetime
from time import gmtime, strftime
from otbiot_mqtt_influxdb_defines import *
from user_config import *
import smtplib, email.message, email.utils

sleep_time = 10
PING_RSP = "ping_rsp"
STATUS_SUB = "status_sub_%s"
QUERY_RSP = "query_rsp"

otb_new = "otb-iot"
otb_old = "otb_iot"
espi = "espi"
otb_types = (otb_new, otb_old, espi)

def create_status_topic(chipId, otb_type):
  topic = "/%s/%s/status" % (otb_type, chipId)
  return topic

def create_ping_topic_message(chipId, otb_type):
  if otb_type == "otb_iot":
    # old style
    topic = "/%s/%s/system" % (otb_type, chipId)
    message = "ping"
  else:
    # new style
    topic = "/%s/%s" % (otb_type, chipId)
    message = "trigger/ping"
  return (topic, message)

def create_ping_topic_rsp(chipId, otb_type):
  if otb_type == "otb_iot":
    # old style
    topic = "/%s/%s/status" % (otb_type, chipId)
  else:
    # new style
    topic = "/%s/%s" % (otb_type, chipId)
  return (topic, message)

def create_query_temp_sensors(chipId, otb_type):
  topic = "/%s/%s" % (otb_type, chipId)
  message = "get/sensor/temp/ds18b20/num"
  return topic, message

# Subscribe to all sensor topics
def subscribe(client, temp_sensors, power_sensors):
  for tsensor in temp_sensors:
    for otb_type in otb_types:
      topic = create_status_topic(tsensor[CHIPID], otb_type)
      client.subscribe(topic)
      tsensor[STATUS_SUB % otb_type] = topic
      print("Subscribed to topic: %s" % topic)
  for psensor in power_sensors:
    for otb_type in otb_types:
      topic = create_status_topic(psensor[CHIPID], otb_type)
      client.subscribe(topic)
      psensor[STATUS_SUB % otb_type] = topic
      print("Subscribed to topic: %s" % topic)
  return

def ping(client, temp_sensors, power_sensors):
  for tsensor in temp_sensors:
    for otb_type in otb_types:
      topic, message = create_ping_topic_message(tsensor[CHIPID], otb_type)
      client.publish(topic, message)
      print("Sent: %s %s" % (topic, message))
    tsensor[PING_RSP] = False
  for psensor in power_sensors:
    for otb_type in otb_types:
      topic, message = create_ping_topic_message(psensor[CHIPID], otb_type)
      client.publish(topic, message)
      print("Sent: %s %s" % (topic, message))
    psensor[PING_RSP] = False

def query_temp_sensors(client, temp_sensors):
  for tsensor in temp_sensors:
    for otb_type in otb_types:
      topic, message = create_query_temp_sensors(tsensor[CHIPID], otb_type)
      client.publish(topic, message)
      print("Sent: %s %s" % (topic, message))
    tsensor[QUERY_RSP] = None

def process_rsp(client, temp_sensors, power_sensors, topic, message):
  rc = None
  for tsensor in temp_sensors:
    topic_match = False
    for otb_type in otb_types:
      if tsensor[STATUS_SUB % otb_type] == topic:
        topic_match = True  
    if topic_match:
      # Match
      if (message == "ok:pong"):
        tsensor[PING_RSP] = "OK"
        return True
      elif (message == "pong"):
        tsensor[PING_RSP] = "OK"
        return True
      elif (message.startswith("ok:")):
        num = message.split(':')[1]
        num = int(num)
        correctnum = len(tsensor[SENSORS])
        if (correctnum == num):
          tsensor[QUERY_RSP] = "OK"
        else:
          tsensor[QUERY_RSP] = "BAD %d vs %d" % (num, correctnum)
        return True
  for psensor in power_sensors:
    topic_match = False
    for otb_type in otb_types:
      if psensor[STATUS_SUB % otb_type] == topic:
        topic_match = True  
      # Match
    if topic_match:
      if (message == "pong"):
        psensor[PING_RSP] = "OK"
        return True
  return rc

def check_rsps(client):
  global temp_sensors, power_sensors
  results = ""
  send_email = False
  results += "--- Results ---\n"
  for tsensor in temp_sensors:
    results += "Temp sensor  %6.6s  Ping:    %s" % (tsensor[CHIPID], str(tsensor[PING_RSP]))
    results += "  Sensors: %s\n" % str(tsensor[QUERY_RSP])
    if tsensor[PING_RSP] != "OK" or tsensor[QUERY_RSP] != "OK":
      send_email = True
  for psensor in power_sensors:
    results += "Power sensor %6.6s  Ping:    %s\n" % (psensor[CHIPID], str(psensor[PING_RSP]))
    if psensor[PING_RSP] != "OK":
      send_email = True
  results += "---------------"
  return results, send_email

def on_message(client, userdata, msg):
  global temp_sensors, power_sensors
  print("Received MQTT message: %s %s" % (msg.topic, msg.payload))
  rc = process_rsp(client, temp_sensors, power_sensors, msg.topic, msg.payload)
  if rc == None:
    print("No matching sensors (or unexpected message) - ignoring: %s %s" % (msg.topic, msg.payload))
  return

def on_connect(client, userdata, flags, rc):
  global temp_sensors, connected
  print("Connected to broker with result code: " + str(rc))
  subscribe(client, temp_sensors, power_sensors)
  connected = True
  was_once_connected = True
  ping(client, temp_sensors, power_sensors)
  query_temp_sensors(client, temp_sensors)

def on_disconnect(client, userdata, rc):
  print("Disconnected from broker!!!")
  run = False

def send_results_email(results):
  email_to = ["piers@piersandkatie.com",]
  email_from = "cont1@piersandkatie.com"
  smtp = smtplib.SMTP("smtp.zen.co.uk")
  msg = email.message.Message()
  msg['From'] = email_from
  msg['To'] = ', '.join(email_to)
  msg['Subject'] = "otbiot sensor report on %s" % (strftime("%Y_%m_%d at %H:%M:%S_UTC", gmtime()))
  msg.add_header('Content-Type', 'text')
  msg.set_payload(results)
  smtp.sendmail(msg['From'], email_to, msg.as_string())
  smtp.quit()

def main():
  global client, connected, mqtt_addr, mqtt_port, sleep_time, was_once_connected
  global influxdb_client, influxdb_host, influxdb_port, influxdb_user, influxdb_password, influxdb_dbname

  print("Create connection to MQTT broker: %s:%d" % (mqtt_addr, mqtt_port))

  client = mqtt.Client(protocol=mqtt.MQTTv31)
  client.on_connect = on_connect
  client.on_message = on_message
  client.on_disconnect = on_disconnect

  connected = False
  was_once_connected = False
  temp_updates_received = 0
  client.connect_async(mqtt_addr, mqtt_port, 60)
  client.loop_start()

  run = True
  while run:
    time.sleep(sleep_time)
    if not connected and was_once_connected:
      run = False
      break
    results, send_email = check_rsps(client)
    print(results)
    if send_email:
      send_results_email(results)
      print("Sent results email")
    run = False

  print("Exiting as done")

if __name__ == "__main__":
  main()

