#!/bin/sh

cd /home/mint/cuboulder/ecea3505/assignments-3-and-later-davidxvuong/server
make clean
make USE_AESD_CHAR_DEVICE=1
cd ../aesd-char-driver
make clean
make
cd ..
sudo ./aesd-char-driver/aesdchar_unload
sudo ./aesd-char-driver/aesdchar_load
./server/aesdsocket