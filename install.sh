#!/bin/bash

sudo apt install -y cmake libusb-1.0-0-dev

if [ ! -e /usr/include/libusb.h ]; then
    sudo ln -s /usr/include/libusb-1.0/libusb.h /usr/include/libusb.h
fi
