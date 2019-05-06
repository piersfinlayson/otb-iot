#!/bin/bash
gpio mode 22 output
gpio write 22 0
./reboot.sh
