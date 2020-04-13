#!/bin/bash
echo "Cleaning up any old directories ..."
echo "  xtensa-lx106-elf"
rm -fr xtensa-lx106-elf
echo "  ESP8266_RTOS_SDK"
rm -fr ESP8266_RTOS_SDK
echo "  build"
rm -fr build
echo "Install python-pip"
sudo apt update
sudo apt install -y python-pip
echo "Get xtensa toolset"
wget https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz
tar zxf xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz
rm -f xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz
echo "Get IDF"
git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
cd ESP8266_RTOS_SDK
git checkout remotes/origin/release/v3.3
cd ..
echo "Install IDF Python pre-requisities"
/usr/bin/python -m pip install --user -r ./ESP8266_RTOS_SDK/requirements.txt
echo "Done - now run:"
echo "export IDF_PATH=`pwd`/ESP8266_RTOS_SDK"
echo "export PATH=$PATH:`pwd`/xtensa-lx106-elf/bin:`pwd`/ESP8266_RTOS_SDK/tools"

