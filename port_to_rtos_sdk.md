USER ROOT:
sudo apt update
sudo apt install python-pip



USER build:
cd ~/
wget https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz
tar xvf xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar
export PATH=$PATH:~/xtensa-lx106-elf/bin
git clone https://github.com/espressif/ESP8266_RTOS_SDK.git
export IDF_PATH=~/ESP8266_RTOS_SDK
cd $IDF_PATH
git checkout remotes/origin/release/v3.3
/usr/bin/python -m pip install --user -r /home/build/ESP8266_RTOS_SDK/requirements.txt
cd $IDF_PATH/examples/get-started/hello_world
make menuconfig
make
