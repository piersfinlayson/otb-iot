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

import os, time, datetime, rrdtool, tempfile, astral
from flask import Flask
from flask import Response
from flask import request
from collections import OrderedDict

os.environ['TZ'] = "Europe/London"
time.tzset()

TITLE_POWER = "Power Usage"
LOCATION = "Gegin Fedw"
SCALES = ('hour', '2 hours', '4 hours', '8 hours', '12 hours', 'day', '3 days', 'week', 'month', 'quarter', 'six months', 'year')
SCALES_HOURS = {'hour':1,
                '2 hours':2,
                '4 hours':4,
                '8 hours':8,
                '12 hours':12,
                'day':24,
                '3 days':72,
                'week':192,
                'month':840,
                'quarter':2160,
                'six months':4380,
                'year':8760}
RESOLUTIONS = {'hour': '-1h', '2 hours': '-2h', '4 hours': '-4h', '8 hours': '-8h', '12 hours': '-12h', 'day': '-24hours', '3 days': '-72hours', 'week':'-8d', 'month':'-35d', 'quarter':'-90d', 'six months':'-6months', 'year':'-1y'}
GRAPH_NAMES = ('graphPower',)
GRAPHS = {'graphPower':TITLE_POWER}
RRD_FILE = "/data/house/power/main.rrd"

RRD_POWER = 'power'
RRD_CURRENT = 'current'
RRD_VOLTAGE = 'voltage'


def outputGraph(graph, title, scale):
  output = ""
  output += '<h3>%s</h3>' % title
  output += '<p><img src="./%s.png?scale=%s"></p>\n' % (graph, scale)
  output += '<p>Other periods: \n'
  for scale in SCALES:
    output += '<a href="./%s?scale=%s">%s</a> \n' % (graph, scale, scale)
  output += '</p>\n'
  return output

def readRrdData(rrdFile=RRD_FILE, rrdField=RRD_POWER, type='AVERAGE', range='5s'):
  time_span, names, values = rrdtool.fetch(rrdFile, type, '-s -%s' % range, '-e now')
  # ts_start, ts_end, ts_res = time_span
  # times = range(ts_start, ts_end, ts_res)
  ii = 0
  found = False
  for name in names:
    if name == rrdField:
      found = True
      break
    ii += 1
  if found:
    value = values[0][ii]
  return value

app = Flask(__name__)
@app.route("/")
def power():
  output = "<title>%s %s</title>\n<body>\n" % (LOCATION, TITLE_POWER)
  output += "<h2>%s %s</h2>\n" % (LOCATION, TITLE_POWER)
  output += "<h3>Current Instantaneous Values</h3>"
  output += "<p>Power:   %dW</p>" % readRrdData(RRD_FILE, RRD_POWER)
  output += "<p>Current: %.2fA</p>" % readRrdData(RRD_FILE, RRD_CURRENT)
  output += "<p>Voltage: %.1fV</p>" % readRrdData(RRD_FILE, RRD_VOLTAGE)
  output += "<h3>Graphs</h3>"
  for graphName in GRAPH_NAMES:
    output += '<p><iframe src="./%s" height="680" width="920"></iframe></p>\n' % graphName
  output += "<p><i>Page generated at %s local time (%s)</i></p>" % (str(datetime.datetime.now()).split('.')[0], os.environ['TZ']) 
  output += "</body>"
  return output

def graphFrame(request, name):
  scale = getScale(request)
  output = outputGraph(name, GRAPHS[name], scale)
  return output

@app.route("/graphPower")
def graphPowerFrame():
  return graphFrame(request, 'graphPower')

def getSunRiseSet():
  a = astral.Astral()
  brum = a['Birmingham']
  now = datetime.datetime.now()
  sun = brum.sun(date=now)
  if sun['sunrise'].replace(tzinfo=None) < now:
    sunrise = sun['sunrise']
    if sun['sunset'].replace(tzinfo=None) < now:
      sunset = sun['sunset']
    else:
      sun = brum.sun(date=(now-datetime.timedelta(hours=24)))
      sunset = sun['sunset']
  else:
    sun = brum.sun(date=(now-datetime.timedelta(hours=24)))
    sunrise = sun['sunrise']
    sunset = sun['sunset']
  return time.mktime(sunrise.timetuple()), time.mktime(sunset.timetuple())

def getScale(request, default='hour'):
  scale = request.args.get('scale')
  if scale not in SCALES:
    scale = default
  return scale

def prepGraph(request):
  scale = getScale(request)
  fd, path = tempfile.mkstemp('.png')
  if isinstance(path,unicode) :
    path = path.decode('utf8').encode('ascii')
  return scale, fd, path

def doGraph(path, scale, verticals, name, details):
  title = '%s %s over last %s' % (LOCATION, GRAPHS[name], str(scale))
  rrdtool.graph(path,
                '-w', '800', '-h', '400', '-a', 'PNG', '--slope-mode',
                '--start', '%s' % RESOLUTIONS[scale], '--end', 'now',
                '--vertical-label', 'Power (kW)',
                '--title', title,
                '-l', '0', 
                '--font', 'WATERMARK:8', 
                '--watermark', "(c) Piers Finlayson generated at " + str(datetime.datetime.now()).split('.')[0] + " " + os.environ['TZ'],
                details,
                verticals)
  data = open(path, 'r').read()
  os.unlink(path)
  return data
  
@app.route("/graphPower.png")
def graphPower():
  name = 'graphPower'
  scale, fd, path = prepGraph(request)
  verticals = []
  details = ['DEF:power=%s:%s:AVERAGE' % (RRD_FILE, RRD_POWER),
             'DEF:current=%s:%s:AVERAGE' % (RRD_FILE, RRD_CURRENT),
             'DEF:voltage=%s:%s:AVERAGE' % (RRD_FILE, RRD_VOLTAGE),
             'VDEF:totalenergy=%s,AVERAGE' % (RRD_POWER),
             'CDEF:powerkw=%s,1000,/' % (RRD_POWER),
             'CDEF:powerkwtotal=powerkw,%d,*' % (SCALES_HOURS[scale]), 
             'VDEF:totalenergykwh=powerkw,AVERAGE',
             'VDEF:totalenergykwhtotal=powerkwtotal,AVERAGE',
             'AREA:%s#0000ff:Instantaneous power (kW)' % (RRD_POWER),
             'COMMENT:\s',
             'COMMENT:\s',
             'COMMENT:\s',
             'LINE2:totalenergy#ff0000:Average power over period\:   ',
             'GPRINT:totalenergykwh:%2.2lfkW',
             'COMMENT:\s',
             'COMMENT:\s',
             'COMMENT:\s',
             'GPRINT:totalenergykwhtotal:Total energy used during period\: %2.2lfkWh',
             'COMMENT:\s',
             'COMMENT:\s',
             'COMMENT:\s',
             'VDEF:maxpower=power,MAXIMUM',
             'VDEF:minpower=power,MINIMUM',
             'GPRINT:maxpower:Maximum power over period\: %7.0lfW',
             'GPRINT:minpower:Minimum power over period\: %7.0lfW',
             'COMMENT:\s',
             'COMMENT:\s',
             'COMMENT:\s',
             'VDEF:maxcurrent=current,MAXIMUM',
             'VDEF:mincurrent=current,MINIMUM',
             'GPRINT:maxcurrent:Maximum current over period\: %2.2lfA',
             'GPRINT:mincurrent:Minimum current over period\: %2.2lfA',
             'COMMENT:\s',
             'COMMENT:\s',
             'COMMENT:\s',
             'VDEF:maxvoltage=voltage,MAXIMUM',
             'VDEF:minvoltage=voltage,MINIMUM',
             'GPRINT:maxvoltage:Maximum voltage over period\: %3.1lfV',
             'GPRINT:minvoltage:Minimum voltage over period\: %3.1lfV',
             'COMMENT:\s',
             'COMMENT:\s',
             'COMMENT:\s',
             ]
  data = doGraph(path, scale, verticals, name, details)
  return Response(data, mimetype='image/png')

if __name__ == "__main__":
  app.run(host="0.0.0.0", port=80, debug=True)