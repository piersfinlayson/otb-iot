#!/usr/bin/python
#
# OTB-IOT - Out of The Box Internet Of Things
#
# Copyright (C) 2016 Piers Finlayson
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
# - query mbus meter readings from otb-iot attached mbus capable meters
# - query once an hour
# - process interesting information from the meter and upload to influxdb
#
##############################################################################

##############################################################################
#
# To install mosquitto module:
#
#   pip install paho-mqtt
#
# To install InfluxDBClient:
#
#   pip install influxdb
#
##############################################################################

##############################################################################
#
# This script is designed to be run from cron - hourly
#
##############################################################################

# This script supports just new format (otb-iot) MQTT topics

import paho.mqtt.client as mqtt
import time, syslog, datetime
import subprocess
import xml.etree.ElementTree as ET
from influxdb import InfluxDBClient
from time import gmtime, strftime
from otbiot_mbus_query_defines import *

# If this throws an error create your own user_config.py from sample_user_config.py
from user_config import *

# Globals
connected = False
was_once_connected = False
influxdb_client = None
exit = False
status = {}

# timer for general alive loop in main method
sleep_time = 5

def create_mbus_set_config(type, value):
  ret = "set/config/serial/%s" % type
  if value != None:
    ret += "/%s" % value
  return ret

def create_mbus_trigger_send(value):
  ret = "trigger/serial/send/%s" % value
  return ret
  
def create_mbus_trigger_dump(value):
  ret = "trigger/serial/buffer/dump"
  if value != None:
    ret = +"/%s" % value
  return ret
  
def create_mbus_query_string(chipId, addr, location):
  return "105b%02s5c16" % addr

def create_mbus_topic_send(chipId):
  syslog.syslog(syslog.LOG_DEBUG, "Create mbus topic send for chipId: %s" % (chipId))
  topic = "/otb-iot/%s" % chipId
  syslog.syslog(syslog.LOG_DEBUG, "Topic: %s" % topic)
  return topic

def create_mbus_topic_rsp(chipId, addr, location):
  syslog.syslog(syslog.LOG_DEBUG, "Create temp topic rsp for chipId: %s, addr: %s, location: %s" % (chipId, addr, location))
  topic = "/otb-iot/%s/status" % chipId
  syslog.syslog(syslog.LOG_DEBUG, "Topic: %s" % topic)
  return topic
  
# returns False if no more to do for now, True if something else to do
def do_addr_action(status):
  global tx_pin, rx_pin, baud_rate, exit, client
  sta = status[STATUS]
  addr = status[ADDR]
  location = status[LOCATION]
  chipId = status[CHIPID]
  syslog.syslog(syslog.LOG_DEBUG, "do_addr_action: %d chipId: %s addr: %s location: %s" % (sta, chipId, addr, location))
  rc = False
  send_mbus = False
  if (sta == ADDR_STATUS_INIT):
    rc = True
  elif (sta == ADDR_STATUS_QUERY_MBUS):
    topic = create_mbus_topic_send(chipId)
    message = create_mbus_trigger_send(create_mbus_query_string(chipId, addr, location))
    send_mbus = True
  elif (sta == ADDR_STATUS_CLEAR_BUF):
    topic = create_mbus_topic_send(chipId)
    message = create_mbus_trigger_dump(None)
    send_mbus = True
  elif (sta == ADDR_STATUS_DUMP_DATA):
    topic = create_mbus_topic_send(chipId)
    time.sleep(2)
    message = create_mbus_trigger_dump(None)
    send_mbus = True
  elif (sta == ADDR_STATUS_PROCESS_DATA):
    rc = process_dump(status)
    if (rc):
      status[STATUS] += 1
  else:
    syslog.syslog(syslog.LOG_ERR, "Unexpected status: %d" % sta)
    assert(False)
  if send_mbus:
    syslog.syslog(syslog.LOG_DEBUG, "Send MQTT %s %s" % (topic, message))
    client.publish(topic, message)
  return rc
  
def process_dump(status):
  global glob_message, glob_topic
  rc = False
  hex = glob_message[3:].split('/')
  for bit in hex:
    if bit != "xx":
      status[DUMP] += bit
    else:
      rc = True
      break
  return rc

def do_addr(status):
  go = True
  rc = False
  while go:
    go = do_addr_action(status)
    if go and status[STATUS] == ADDR_STATUS_DONE:
      rc = True
      break
  return rc
    
def get_chip_id(topic):
  chipId = topic.split("/")[2]
  return chipId
  
def process_response(topic, message):
  global glob_topic, glob_message
  glob_topic = topic
  glob_message = message
  chipId = get_chip_id(topic)
  ok = False
  if message[:2] == "ok":
    ok = True
  return chipId, ok

def on_message(client, userdata, msg):
  global status, exit
  syslog.syslog(syslog.LOG_DEBUG, "Received MQTT message: %s %s" % (msg.topic, msg.payload))
  chipId, ok = process_response(msg.topic, msg.payload)
  sta = status[chipId]
  if not ok:
    # Only failure we accept is disabling at start (in case already disabled)
    if (sta[STATUS] != CHIP_STATUS_DISABLE):
      syslog.syslog(syslog.LOG_ERR, "Failed message - exiting")
      exit = True
      return
  if sta[STATUS] < CHIP_STATUS_DONE:
      sta[STATUS] += 1
  if sta[STATUS] == CHIP_STATUS_DONE:
    # Should be related to an addr - can only handle addrs serially
    done = False
    for addr in sta[ADDRS]:
      if sta[ADDRS][addr][STATUS] < ADDR_STATUS_DONE:
        if sta[ADDRS][addr][STATUS] != ADDR_STATUS_PROCESS_DATA:
          sta[ADDRS][addr][STATUS] += 1
        rc = do_addr(sta[ADDRS][addr])
        if rc:
          # Need to move onto next addr
          done = False
        else:
          done = True
          break
    if not done:
      exit = 1
  else:
    do_chip(sta)

def do_subscribe(chipId, addr, location):
  global client, last_mid
  return client.subscribe(create_mbus_topic_rsp(chipId, addr, location))

def do_chip_action(status):
  global tx_pin, rx_pin, baud_rate, exit, client
  chipId = status[CHIPID]
  sta = status[STATUS]
  syslog.syslog(syslog.LOG_DEBUG, "do_chip_action: %d chipId: %s" % (sta, chipId))
  rc = False
  send_mbus = False
  if (sta == CHIP_STATUS_INIT):
    status[STATUS] += 1
    rc = True
  elif (sta == CHIP_STATUS_DISABLE):
    topic = create_mbus_topic_send(chipId)
    message = create_mbus_set_config(DISABLE, None)
    send_mbus = True
  elif (sta == CHIP_STATUS_SET_TX_PIN):
    topic = create_mbus_topic_send(chipId)
    message = create_mbus_set_config(TX, tx_pin)
    send_mbus = True
  elif (sta == CHIP_STATUS_SET_RX_PIN):
    topic = create_mbus_topic_send(chipId)
    message = create_mbus_set_config(RX, rx_pin)
    send_mbus = True
  elif (sta == CHIP_STATUS_SET_BAUD_RATE):
    topic = create_mbus_topic_send(chipId)
    message = create_mbus_set_config(BAUD_RATE, baud_rate)
    send_mbus = True
  elif (sta == CHIP_STATUS_ENABLE):
    topic = create_mbus_topic_send(chipId)
    message = create_mbus_set_config(ENABLE, None)
    send_mbus = True
  elif (sta == CHIP_STATUS_RESET):
    status = STATUS_INIT
    exit = True
  else:
    syslog.syslog(syslog.LOG_ERR, "Unexpected status: %d", sta)
    assert(False)
  if send_mbus:
    syslog.syslog(syslog.LOG_DEBUG, "Send MQTT %s %s" % (topic, message))
    client.publish(topic, message)
  return rc

def do_chip(status):
  go = True
  while go:
    go = do_chip_action(status)

def on_subscribe(client, userdata, mid, qos):
  global status
  for chipId in status:
    if status[chipId][MID] == mid:
      do_chip(status[chipId])

def on_connect(client, userdata, flags, rc):
  global mbus_sensors, connected, status
  syslog.syslog(syslog.LOG_INFO, "Connected to broker with result code: " + str(rc))
  for chip in mbus_sensors:
    chipId = chip[CHIPID]
    status[chipId] = {CHIPID:chipId, ADDRS:{}, STATUS:CHIP_STATUS_INIT, MID:None}
    done_chip = None
    for address in chip[ADDRS]:
      addr = address[ADDR]
      location = address[LOCATION]
      status[chipId][ADDRS][addr] = {ADDR:addr, STATUS:ADDR_STATUS_INIT, LOCATION:location, CHIPID:chipId, DUMP:""}
      if done_chip == None:
        # Only subscribe once per chip
        rc, mid = do_subscribe(chipId, addr, location)
        status[chipId][MID] = mid
      done_chip = chipId
  connected = True
  was_once_connected = True

def on_disconnect(client, userdata, rc):
  syslog.syslog(syslog.LOG_ERR, "Disconnected from broker!!!")
  run = False
  
def get_time():
  time_now = strftime("%Y-%m-%dT%H:%M:%SZ", gmtime()) 
  return time_now

def send_to_influx(chipId, addr, location, xml):
  global influxdb_client
  energy = xml[2][3].text
  volume = xml[3][3].text
  power = xml[4][3].text
  flow = xml[5][3].text
  flow_temp = xml[6][3].text
  return_temp = xml[7][3].text
  venergy = int(energy) * 10
  uenergy = "kWh"
  vvolume = int(volume) / 10.0
  uvolume = "m^3"
  if power != "999999":
    vpower = int(power) / 10.0
  else:
    vpower = 0
  upower = "kW"
  vflow = int(flow)
  uflow = "m m^3/h"
  vflow_temp = int(flow_temp) / 10.0
  uflow_temp = "C"
  vreturn_temp = int(return_temp) / 10.0
  ureturn_temp = "C"
  syslog.syslog(syslog.LOG_INFO, "Output for chipId: %s addr: %s location: %s" % (chipId, addr, location))
  syslog.syslog(syslog.LOG_INFO, "  %s      %s" % ("Energy (10kWh)",     xml[2][3].text))
  syslog.syslog(syslog.LOG_INFO, "  %s     %s" % ("Volume (0.1m^3)",    xml[3][3].text))
  syslog.syslog(syslog.LOG_INFO, "  %s        %s" % ("Power (100W)",       xml[4][3].text))
  syslog.syslog(syslog.LOG_INFO, "  %s      %s" % ("Flow (m m^3/h)",     xml[5][3].text))
  syslog.syslog(syslog.LOG_INFO, "  %s    %s" % ("Flow temp (0.1C)",   xml[6][3].text))
  syslog.syslog(syslog.LOG_INFO, "  %s  %s" % ("Return temp (0.1C)", xml[7][3].text))
  try:
    cur_time = get_time()
    json_body = [{'measurement':'energy',
                  'tags':{'chipId':chipId,
                          'address':addr,
                          'location':location,
                          'unit':uenergy},
                  'time':cur_time,
                  'fields':{'value':"%d" % venergy}},
                 {'measurement':'volume',
                  'tags':{'chipId':chipId,
                          'address':addr,
                          'location':location,
                          'unit':uvolume},
                  'time':cur_time,
                  'fields':{'value':"%.1f" % vvolume}},
                 {'measurement':'power',
                  'tags':{'chipId':chipId,
                          'address':addr,
                          'location':location,
                          'unit':upower},
                  'time':cur_time,
                  'fields':{'value':"%.2f" % vpower}},
                 {'measurement':'flow',
                  'tags':{'chipId':chipId,
                          'address':addr,
                          'location':location,
                          'unit':uflow},
                  'time':cur_time,
                  'fields':{'value':"%d" % vflow}},
                 {'measurement':'flow_temp',
                  'tags':{'chipId':chipId,
                          'address':addr,
                          'location':location,
                          'unit':uflow_temp},
                  'time':cur_time,
                  'fields':{'value':"%.1f" % vflow_temp}},
                 {'measurement':'return_temp',
                  'tags':{'chipId':chipId,
                          'address':addr,
                          'location':location,
                          'unit':ureturn_temp},
                  'time':cur_time,
                  'fields':{'value':"%.1f" % vreturn_temp}},
                ]
    influxdb_client.write_points(json_body)
    syslog.syslog(syslog.LOG_INFO, "Written data to influxdb: " + str(json_body))
  except:
    syslog.syslog(syslog.LOG_ERR, "EXCEPTION: Failed to write info to influxdb")
  

def main():  
  global client, connected, mqtt_addr, mqtt_port, sleep_time, was_once_connected, status
  global influxdb_client, influxdb_host, influxdb_port, influxdb_user, influxdb_password, influxdb_dbname

  syslog.syslog(syslog.LOG_INFO, "Create connection to MQTT broker: %s:%d" % (mqtt_addr, mqtt_port))

  client = mqtt.Client(protocol=mqtt.MQTTv31)
  client.on_connect = on_connect
  client.on_message = on_message
  client.on_disconnect = on_disconnect
  client.on_subscribe = on_subscribe

  connected = False
  was_once_connected = False
  temp_updates_received = 0
  client.connect_async(mqtt_addr, mqtt_port, 60)
  client.loop_start()
  
  influxdb_client = InfluxDBClient(influxdb_host, influxdb_port, influxdb_user, influxdb_password, influxdb_dbname)

  run = True
  graceful = False
  while run:
    time.sleep(sleep_time)
    if not connected and was_once_connected:
      run = False
      syslog.syslog(syslog.LOG_ERR, "Exiting as disconnected")
      break
    if exit:
      syslog.syslog(syslog.LOG_INFO, "Exiting as done")
      graceful = True
      break
  if graceful:
    for chipId in status:
      for addr in status[chipId][ADDRS]:
        location = status[chipId][ADDRS][addr][LOCATION]
        output = subprocess.check_output([mbus_process_hex_dump_cmd, status[chipId][ADDRS][addr][DUMP]])
        output = "\n".join(output.split("\n")[1:])
        xml = ET.fromstring(output)
        send_to_influx(chipId, addr, location, xml) 
  
if __name__ == "__main__":
  main()

