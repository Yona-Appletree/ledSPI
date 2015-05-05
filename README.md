Overview
========
LEDscape is a library and service for controlling individually addressable LEDs from a 
Beagle Bone Black using the onboard PRUs. It currently supports WS281x (WS2811, WS2812, WS2812b), WS2801 and initial 
support for DMX. It is designed to be used with level-shifter capes like those produced by RGB-123.


Background
------
LEDscape was originally written by Trammell Hudson (http://trmm.net/Category:LEDscape) for
controlling WS2811-based LEDs. Since his original work, his version (https://github.com/osresearch/LEDscape)
has been repurposed to drive a different type of LED panel (e.g. http://www.adafruit.com/products/420).

This version of the library was forked from his original WS2811 work. Various improvements have been made in the 
attempt to make an accessible and powerful LED driver based on the BBB. Many thanks to Trammell for his excellent work
in scaffolding the BBB and PRUs for driving LEDs.


WARNING
=======

This code works with the PRU units on the Beagle Bone and can easily cause *hard crashes*.  It is still being debugged 
and developed. Be careful hot-plugging things into the headers -- it is possible to damage the pin drivers and cause 
problems in the ARM, especially if there are +5V signals involved.


Installation and Usage - Command Line
=====================================
It is necessary to have access to a shell onto the Beaglebone Black using serial, ethernet, or USB connections.  
Examples on how to do this can be found at [BeagleBoard.org](http://beagleboard.org/getting-started) or at
[Adafruit's Learning Site](https://learn.adafruit.com/ssh-to-beaglebone-black-over-usb/ssh-on-mac-and-linux).


To use LEDscape, download it to your BeagleBone Black by connecting the BBB to the internet via ethernet and cloning 
this github repository. Before LEDscape will function, you will need to replace the device tree file, load the  
uio\_pruss, and reboot by executing the commands listed below via the cmd line.

Angstrom - RevB

	git clone git://github.com/Yona-Appletree/LEDscape
	cd LEDscape
	cp /boot/am335x-boneblack.dtb{,.preledscape_bk}
	cp am335x-boneblack.dtb /boot/
	modprobe uio_pruss
	vi /boot/uboot/uEnv.txt
	reboot
	sudo ./install-service.sh

Debian - RevC (2014-04-23)

	git clone git://github.com/Yona-Appletree/LEDscape
	cd LEDscape
	cp /boot/uboot/dtbs/am335x-boneblack.dtb{,.preledscape_bk}
	cp am335x-boneblack.dtb /boot/uboot/dtbs/
	modprobe uio_pruss
	vi /boot/uboot/uEnv.txt 
	reboot

Debian - RevC (2015-03-01)

	git clone git://github.com/Yona-Appletree/LEDscape
	cd LEDscape
	cp /boot/dtbs/3.8.13-bone70/am335x-boneblack.dtb{,.preledscape_bk}
	cp am335x-boneblack.dtb /boot/dtbs/3.8.13-bone70/
	modprobe uio_pruss	
	vi /boot/uEnv.txt
	reboot

After rebooting you will need to enter the LEDscape folder and compile the LEDscape code.

	cd LEDscape
	make
	sudo ./install-service.sh
	
Note: Locating the am335x-boneblack.dtb file:
* Older BBB have the file in /boot;
* Some distros (e.g. Arch) keep these files in /boot/dtbs;
* The Debian distribution keeps the file in /boot/uboot/dtbs (when mounted
  over USB, the /boot/uboot directory is read-only from the BBB and you
  need to do the file operations from the host system.	
	
Disabling HDMI
--------------

For LEDscape to run properly you'll have to disable the HDMI "cape" on the BeagleBone Black.

Mount the FAT32 partition, either through linux on the BeagleBone or
by plugging the USB into a computer, modify 'uEnv.txt' by changing:

Using vi
	
	vi /boot/uboot/uEnv.txt

Angstrom - RevB
    
	capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN

It should read something like

	optargs=quiet drm.debug=7 capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN

Debian - RevB

	##Disable HDMI
	#cape_disable=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN
	
Change to
	
	##Disable HDMI
	cape_disable=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN
    
Save and Reboot the BeagleBone Black.


Open Pixel Control Server
=========================

Setup
-----

Once you have LEDscape sending data to your pixels, you will probably
want to use the `opc-server` server which accepts Open Pixel Control data
and passes it on to LEDscape. There is an systemd service file built to run
LEDscape from it's home directory. Simple install/uninstall scripts are provided:

	sudo ./install-service.sh

If you would prefer to run the receiver startup script without adding it as a service:

	sudo ./run-ledscape

-------------
Configuration
	
By default LEDscape is configured for strings of 256 WS2811 pixels, accepting OPC
data on port 7890. You can adjust this by editing `run-ledscape` and 
editing the parameters to `opc-server`


Data Format
-----------

The `opc-server` server accepts data on OPC channel 0. It expects the data for
each LED strip concatonated together. This is done because LEDscape requires
that data for all strips be present at once before flushing data data out to
the LEDs. 

`opc-server` supports both TCP and UDP data packets. The TCP port is specified with `--tcp-port <port>` and the UDP port
with `--udp-port <port>`. Entering `0` for a port number will disable that server.
 
Note that if using the UDP server, `opc-server` will limit the number of pixels to 21835, or 454 pixels per port if
using all 48 ports.

Output Modes
------------
LEDscape is capable of outputting several types of signal. By default, a ws2811-compatible signal is generated. The
output mode can be specified with the `--mode <mode-id>` parameter. A list of available modes and their descriptions
can be obtained by running `opc-server -h`. 

Frame Rates for WS2812 Leds
-----------
	512 per channel ~= 060 fps
	256 per channel ~= 120 fps
	128 per channel ~= 240 fps
	064 per channel ~= 400 fps

Pin Mappings
------------
Each output mode of LEDscape is compatible with several different pin mappings. These pin-mappings are declared in
`pru/mappings` as json files and each contain information about the mapping. They can be provided to LEDscape with the
`--mapping <mapping-id>` parameter, where `<mapping-id>` is the filename of the json file without it's extension.

The mappings are designed for use with various different cape configurations to simplify the hardware designed.
Additional mappings can be created by adding new `json` files to the `pru/mappings` directory and rebuilding.

A human-readable pinout for a mapping can be generated by running

    node pru/pinmap.js --mapping <mapping-id>

Multi-Pin Channels
------------------
Some mappings, such as ws2801, use multiple pins to output each channel. In these cases, fewer than the full 48 channels
are available. In the case of ws2801, each channel uses two pins, DATA on the first pin and CLOCK on the next. Only 24
channels of output are available and to reduce CPU usage, `opc-server` should be called with `--strip-count 24` or
lower.


Output Features
---------------
`opc-server` supports Fadecandy-inspired temporal dithering and interpolation
to enhance the smoothness of the output data. By default, it will apply a
luminance curve, interpolate and dither input data at the highest framerate
possible with the given number of LEDs.

These options can be configured by command-line switches that are documented in the help output from `opc-server -h`.

To disable all signal enhancements, use `opc-server -lut`


Invocation Examples
-------------------

| Configuration                 | `opc-server` Invocation
| ----------------------------- | -------------
| 32 strips, 64 pixels, ws2811  | ./opc-server --strip-count 32 --count 64 --mode ws281x 
| 24 strips, 512 pixels, ws2801 | ./opc-server --strip-count 24 --count 512 --mode ws2801
| 8 outputs, 170 pixels, dmx    | ./opc-server --strip-count 8 --count 170 --mode dmx


JSON Configuration
------------------

Use the command below to create and execute the JSON configuration

	./opc-server --config ws281x-config.json --mapping rgb-123-v2 --mode ws281x --count 64 --strip-count 48

With this JSON configured it can be called again by issuing the command

	./opc-server --config ws281x-config.json

Processing Example
========

LEDscape provides versions of the FadeCandy processing examples modified to work better with LEDscape in the
`processing` directory. Clone this repo on a computer and run these sketches, edited to point at your BBB hostname or
ip address after starting `opc-server` or installing the system service.


Hardware Tips
========

Connecting the LEDs to the correct pins and level-shifting the voltages
to 5v can be quite complex when using many output ports of the BBB. 

While there may be others, RGB123 makes an excellent 24/48 pin cape designed 
specifically for this version of LEDscape: [24 pin](http://rgb-123.com/product/beaglebone-black-24-output-cape/) or [48 pin](http://rgb-123.com/product/beaglebone-black-48-output-cape/)

If you do not use a cape, refer to the pin mapping section below and remember
that the BBB outputs data at 3.3v. If you run your LEDs at 5v (which most are),
you will need to use a level-shifter of some sort. [Adafruit](http://www.adafruit.com/products/757) has a decent one which works well.  For custom circuit boards we recommend the [TI SN74LV245](http://octopart.com/partsearch#!?q=SN74LV245).


API
===

`ledscape.h` defines the API. The key components are:

	ledscape_t * ledscape_init(unsigned num_pixels)
	ledscape_frame_t * ledscape_frame(ledscape_t*, unsigned frame_num);
	ledscape_draw(ledscape_t*, unsigned frame_num);
	unsigned ledscape_wait(ledscape_t*)

You can double buffer like this:

	const int num_pixels = 256;
	ledscape_t * const leds = ledscape_init(num_pixels);

	unsigned i = 0;
	while (1)
	{
		// Alternate frame buffers on each draw command
		const unsigned frame_num = i++ % 2;
		ledscape_frame_t * const frame
			= ledscape_frame(leds, frame_num);

		render(frame);

		// wait for the previous frame to finish;
		ledscape_wait(leds);
		ledscape_draw(leds, frame_num);
	}

	ledscape_close(leds);

The 24-bit RGB data to be displayed is laid out with BRGA format,
since that is how it will be translated during the clock out from the PRU.
The frame buffer is stored as a "strip-major" array of pixels.

	typedef struct {
		uint8_t b;
		uint8_t r;
		uint8_t g;
		uint8_t a;
	} __attribute__((__packed__)) ledscape_pixel_t;

	typedef struct {
		ledscape_pixel_t strip[32];
	} __attribute__((__packed__)) ledscape_frame_t;


Low level API
=============

If you want to poke at the PRU directly, there is a command structure
shared in PRU DRAM that holds a pointer to the current frame buffer,
the length in pixels, a command byte and a response byte.
Once the PRU has cleared the command byte you are free to re-write the
dma address or number of pixels.

	typedef struct
	{
		// in the DDR shared with the PRU
		const uintptr_t pixels_dma;

		// Length in pixels of the longest LED strip.
		unsigned num_pixels;

		// write 1 to start, 0xFF to abort. will be cleared when started
		volatile unsigned command;

		// will have a non-zero response written when done
		volatile unsigned response;
	} __attribute__((__packed__)) ws281x_command_t;

Reference
==========
* http://www.adafruit.com/products/1138
* http://www.adafruit.com/datasheets/WS2811.pdf
* http://processors.wiki.ti.com/index.php/PRU_Assembly_Instructions
