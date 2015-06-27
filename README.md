Overview
========
ledSPI is a simple opc-compatible server for driving APA102 leds from spidev-enabled linux machines. 
It is derived from LEDscape and contains the same interpolation and dithering support, which itself was inspired by Fadecandy.

It was created specifically to drive APA102 LEDs from a RaspberryPi from data over the OPC protocol. It supports TCP and UDP OPC data, as well as expiermental support for e131 for DMX compatibility.


Building
=========================

To build, simply check out the project to a spidev-enabled machine, and run `make`. There is currently no install script.

ledSPI Server
=========================

*IMPORTANT:* You must have IPv6 enabled. If you get an error like `Address family not supported by protocol`, load the ipv6 module: `sudo modprobe ipv6`

ledSPI can be run using `./ledspi-server`, and can also be enabled using the ledspi-service.sh and ledspi.service files
for sysV and systemd, respectively. For options about the configuration, use `./ledspi-server -h`.

Configuration / Startup
=========================

Systemd (`ledspi-service`) and sysv (`ledspi.service`) startup scripts are included. They each run the run-ledspi script in the project directory, which
can be modified for custom options.

SPI Speed
=========================

APA102s work well up to about 11mhz, but some kernal drivers only allow setting the SPI speed in powers of two, with a gap between 8 and 16mhz. More information about this on the Raspberry Pi can be found here: https://www.raspberrypi.org/forums/viewtopic.php?f=44&t=43442
