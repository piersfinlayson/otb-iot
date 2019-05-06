#!/bin/bash
gpio mode 22 output
gpio write 22 1
./reboot.sh
