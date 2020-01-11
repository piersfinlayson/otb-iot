..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson

Raspberry Pi Build Server
=========================

A raspberry pi can be used as a headless build server for otb-iot.  This can be especially useful if you're writing hardware configuration into an I2C eeprom - as the raspberry pi and otb-iot include this programming function.

This section contains the steps to follow to set up the raspberry pi before building otb-iot.

1 Download and install raspbian lite to an sd card following the instructions `here`_.

.. _here: https://www.raspberrypi.org/documentation/installation/installing-images/

2 Before installing the sd card in the pi, insert in into a linux machine and identify the disk identifier the machine has allocated it.  We'll assume /dev/sde here.

3 On the linux machine, mount the boot parition /dev/sde1 and create a zero length file ssh

::

  sudo mount /dev/sde1 /mnt
  touch /mnt/ssh
  sudo umount /mnt

4 Now mount /dev/sde2 and set up the pi's networking.  For example, to set up wifi, edit the following files:

  * /etc/network/interfaces - change all instances of manual in this file to dhcp

  * /etc/wpa_supplicant/wpa_supplicant.conf - add the following at the end:

::

  network={
          ssid="your ssid"
          psk="your password"
  }

5 Unmount /dev/sde2 and run sync:

  sudo umount /dev/sde2
  sudo sync

6 Now insert the sd card into the pi and boot it up.

7 Once booted, ssh into the IP address of the pi (if it's been allocated via DHCP you may need to query your DHCP server to find its addres):

::

  ssh pi@ip_address # password is raspberry

8 At this point I like to install my public RSA key into ~/.ssh/authorized_keys to save entering the password again

9 Now run raspi-config to expand the filesystem, and enable I2C (optional)

::

  sudo raspi-config

10 After rebooting to allow the changes to take effect update your pi and install the necessary packages:

::

  sudo apt-get update
 
  sudo apt-get dist-upgrade

  sudo apt-get install make unrar-free autoconf automake libtool gcc g++ gperf flex bison texinfo gawk ncurses-dev libexpat-dev python-dev python python-serial sed git unzip bash help2man wget bzip2 libtool-bin hexedit

11 Install esp-open-sdk:

::

  cd ~/

  git clone --recursive https://github.com/pfalcon/esp-open-sdk.git

  cd esp-open-sdk

  make

  cd ..

You're now ready to :doc:`get started <building>` with otb-iot.




