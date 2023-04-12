#!/bin/sh

mkdir -p 1sectorpercluster
mkdir -p 4sectorpercluster

insmod fat16.ko

mount -t fat16 fat16_1sectorpercluster.img 1sectorpercluster
# mount -t fat16 fat16_4sectorspercluster.img 4sectorpercluster

dmesg | grep fat16
