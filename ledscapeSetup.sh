#!/usr/bin/env bash

#Copy DTBS File
echo "Installing DTBS..."
cp /boot/uboot/dtbs/am335x-boneblack.dtb{,.preledscape_bk}
cp am335x-boneblack.dtb /boot/uboot/dtbs/

#Modify uio_pruss
echo "Modprobing uio_pruss..."
modprobe uio_pruss

#Replace uEnv.txt file that has HDMI disabled
echo "Disabling HDMI..."
cp --backup /root/LEDscape/uEnv.txt /boot/uboot/uEnv.txt

#Set Static IP on BBB
echo "Setting Static IP..."
cp --backup /root/LEDscape/interfaces /etc/network/interfaces

#Inform User of Final Steps
echo "Reboot BBB for settings to take effect..."
echo "Make Files after reboot..."