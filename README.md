Overview
========
ledSPI is a simple opc-compatible server for driving APA102 leds from spidev-enabled linux machines. It is derived from
LEDscape and contains the same interpolation and dithering support, which itself was inspired by Fadecandy. 


Building
=========================

To build, simply check out the project to a spidev-enabled machine, and run `make`. There is currently no install script.

ledSPI Server
=========================

ledSPI can be run using `./ledspi-server`, and can also be enabled using the ledspi-service.sh and ledspi.service files
for sysV and systemd, respectively. For options about the configuration, use `./ledspi-server -h`.

Configuration / Startup
=========================

Systemd (`ledspi-service`) and sysv (`ledspi.service`) startup scripts are included. They each run the run-ledspi script in the project directory, which
can be modified for custom options.