#!/bin/sh

rrdtool graph /tmp/temp_graph.png \
-w 1024 -h 400 -a PNG --slope-mode \
--start -7200 --end -3600 \
--title "Gegin Fedw: Power usage an hour ago" \
--watermark "(c) $(date +%Y) Piers Finlayson" \
--font WATERMARK:8 \
-l "0" \
DEF:power=/data/house/power/main.rrd:power:AVERAGE \
VDEF:totalenergy=power,AVERAGE \
CDEF:powerkw=power,1000,/ \
VDEF:totalenergykwh=powerkw,AVERAGE \
AREA:power#0000ff:"Instaneous power (kW)\\n" \
LINE2:totalenergy#ff0000:"Average power over period\:" \
CDEF:powerkwtotal=powerkw,1,* \
VDEF:totalenergykwhtotal=powerkwtotal,AVERAGE \
GPRINT:totalenergykwh:"%.2lf kW\\n" \
GPRINT:totalenergykwhtotal:"  Total energy used during period\:  %.2lf kWh\\n"


