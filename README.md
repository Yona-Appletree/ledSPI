Overview
========

LEDscape is a library for controlling 24 channels of WS2801 leds and 48 channels of WS2811/WS2812 LEDs from
a single Beagle Bone Black. It makes use of the two Programmable Realtime Units (PRUs),
each controlling 24 outputs. It can drive strings of 64 LEDS at 400 fps, 256 LEDs at around 120 fps,
and 512 LEDs at 50 fps, enabling a single BBB to control 24,576 LEDs at once.

The libary can be used directly from C or C++ or data can be sent using the
Open Pixel Control protocol (http://openpixelcontrol.org) from a variety of
sources (e.g. Processing, Java, Python, etc...)

Background
------
LEDscape was originally written by Trammell Hudson (http://trmm.net/Category:LEDscape) for
controlling WS2811-based LEDs. Since his original work, his version (https://github.com/osresearch/LEDscape)
has been repurposed to drive a different type of LED panel (e.g. http://www.adafruit.com/products/420).

This version of the library was forked from his original WS2811 work. Various
improvements have been made in the attempt to make an accessible and powerful
WS28xx driver based on the BBB. Many thanks to Trammell for his execellent work
in scaffolding the BBB and PRUs for driving LEDs.


WARNING
=======

This code works with the PRU units on the Beagle Bone and can easily
cause *hard crashes*.  It is still being debugged and developed.
Be careful hot-plugging things into the headers -- it is possible to
damage the pin drivers and cause problems in the ARM, especially if
there are +5V signals involved.


Installation and Usage - Command Line
=====================================
It is necessary to SSH onto the Beaglebone Black using serial, ethernet, or USB connections.  Examples on how to do this can be found at [BeagleBoard.org](http://beagleboard.org/getting-started) or at [Adafruit's Learning Site] (https://learn.adafruit.com/ssh-to-beaglebone-black-over-usb/ssh-on-mac-and-linux)


To use LEDscape, download it to your BeagleBone Black by connecting the BBB to the internet via ethernet and cloning this github repository. Before LEDscape will function, you will need to replace the device tree file, load the  uio\_pruss, and reboot by executing the commands listed below via the cmd line.

Angstrom - RevB

	git clone git://github.com/Yona-Appletree/LEDscape
	cd LEDscape
	cp /boot/am335x-boneblack.dtb{,.preledscape_bk}
	cp am335x-boneblack.dtb /boot/
	modprobe uio_pruss
	
	sudo sed -i 's/#optargs=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN/optargs=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN'/g /boot/uboot/uEnv.txt
	
	sudo sed -i 's/optargs=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN,BB-BONE-EMMC-2G/#optargs=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN,BB-BONE-EMMC-2G'/g /boot/uboot/uEnv.txt
	reboot

Debian - RevC

	git clone git://github.com/Yona-Appletree/LEDscape
	cd LEDscape
	cp /boot/uboot/dtbs/am335x-boneblack.dtb{,.preledscape_bk}
	cp am335x-boneblack.dtb /boot/uboot/dtbs/
	modprobe uio_pruss
	reboot    

After rebooting you will need to enter the LEDscape folder and compile the LEDscape code.

	cd LEDscape
	make
	
Note: Locating the am335x-boneblack.dtb file:
* Older BBB have the file in /boot;
* Some distros (e.g. Arch) keep these files in /boot/dtbs;
* The Debian distribution keeps the file in /boot/uboot/dtbs (when mounted
  over USB, the /boot/uboot directory is read-only from the BBB and you
  need to do the file operations from the host system.	
	
Disabling HDMI
--------------

If you need to use all 48 pins made available by LEDscape, you'll have
to disable the HDMI "cape" on the BeagleBone Black.

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

Installation and Usage - Using Bash Script
==========================================

	git clone git://github.com/Yona-Appletree/LEDscape
	cd LEDscape
	chmod u+x install_RevC
	sudo ./install_RevC

The BBB will automatically reboot after running this bash script.  It will now be necessary to "make" and compile LEDscape.

	cd LEDscape
	make


Test LEDscape
-------------
Connect a WS2811-based LED chain to the Beagle Bone. The strip must
be running at the same voltage as the data signal. If you are using
an external 5v supply for the LEDs, you'll need to use a level shifter
or other technique to bring the BBB's 3.3v signals up to 5v.

Once everything is connected, run the `rgb-test` program:

	sudo ./rgb-test

The LEDs should now be fading prettily. If not, go back and make
sure everything is setup correctly.

Open Pixel Control Server
=========================

Setup
-----

Once you have LEDscape sending data to your pixels, you will probably
want to use the `opc-server` server which accepts Open Pixel Control data
and passes it on to LEDscape. There is an systemd service
checked in, which can be installed like so:

	sudo systemctl enable /path/to/LEDscape/ledscape.service
	sudo systemctl start ledscape

Note that you must specify an absolute path. Relative paths will not
work with systemctl to enable services.

If you would prefer to run the receiver without adding it as a service:

	sudo run-ledscape
	
By default LEDscape is configured for strings of 256 pixels, accepting OPC
data on port 7890. You can adjust this by editing the run script and 
editing the parameters to opc-rx.

Data Format
-----------

The `opc-server` server accepts data on OPC channel 0. It expects the data for
each LED strip concatonated together. This is done because LEDscape requires
that data for all strips be present at once before flushing data data out to
the LEDs. 

Features and Options
--------------------

`opc-server` supports Fadecandy-inspired temporal dithering and interpolation
to enhance the smoothness of the output data. By default, it will apply a
luminance curve, interpolate and dither input data at the highest framerate
possible with the given number of LEDs.

These options can be configured by command-line swithes that are lightly
documented by `opc-server -h`. The most common setup will be:

	./opc-server --strip-count STRIP_COUNT --count LED_COUNT

An examples for 48 strips of 64 leds (WS2811 or WS2812) would look like:

	./opc-server -s48 -c64
	
Recently, support for WS2801 (5V, Data, Clock, Gnd) chips has been added.  
Support for 24 strips of 64 leds (WS2801) would look like:

	./opc-server -s48 -c64 -0ws2801 -1ws2801 (Pin map #1)
	./opc-server -s48 -c64 -0ws2801_newpins -1ws2801_newpins (Pin map #2)

Note that future versions of `opc-server` will make use of a JSON configuration
and the current flags will be deprecated or removed.

Processing Example
========

The easiest way to see that LEDscape can receive arbitrary data is to run
the included Processing sketch, based on the examples from 
FadeCandy (https://github.com/scanlime/fadecandy). There is a 16x16 panel
example in processing/grid16x16_clouds. Edit the example to point at your
beaglebone's hostname or IP and run 


Hardware Tips
========

Connecting the LEDs to the correct pins and level-shifting the voltages
to 5v can be quite complex when using many output ports of the BBB. 

While there may be others, RGB123 makes an excellent 24/48 pin cape designed 
specifically for this version of LEDscape: [24 pin](http://rgb-123.com/product/beaglebone-black-24-output-cape/) or [48 pin](http://rgb-123.com/product/beaglebone-black-48-output-cape/)

If you do not use a cape, refer to the pin mapping section below and remember
that the BBB outputs data at 3.3v. If you run your LEDs at 5v (which most are),
you will need to use a level-shifter of some sort. [Adafruit](http://www.adafruit.com/products/757) has a decent one which works well.  For custom circuit boards we recommend the [TI SN74LV245](http://octopart.com/partsearch#!?q=SN74LV245).

Pin Mapping
========

The mapping from LEDscape channel to BeagleBone GPIO pin can be generated by running the pinmap script:

	node pinmap.js

As of this writing, it generates the following:

		                       LEDscape Channel Index
	 Row  Pin#       P9        Pin#  |  Pin#       P8        Pin# Row
	  1    1                    2    |   1                    2    1
	  2    3                    4    |   3                    4    2
	  3    5                    6    |   5                    6    3
	  4    7                    8    |   7     25      26     8    4
	  5    9                    10   |   9     28      27     10   5
	  6    11    13      23     12   |   11    16      15     12   6
	  7    13    14      21     14   |   13    10      11     14   7
	  8    15    19      22     16   |   15    18      17     16   8
	  9    17                   18   |   17    12      24     18   9
	  10   19                   20   |   19     9             20   10
	  11   21     1       0     22   |   21                   22   11
	  12   23    20             24   |   23                   24   12
	  13   25             7     26   |   25                   26   13
	  14   27            47     28   |   27    41             28   14
	  15   29    45      46     30   |   29    42      43     30   15
	  16   31    44             32   |   31     5       6     32   16
	  17   33                   34   |   33     4      40     34   17
	  18   35                   36   |   35     3      39     36   18
	  19   37                   38   |   37    37      38     38   19
	  20   39                   40   |   39    35      36     40   20
	  21   41     8       2     42   |   41    33      34     42   21
	  22   43                   44   |   43    31      32     44   22
	  23   45                   46   |   45    29      30     46   23
	  
	              ^       ^                    ^       ^
	              |-------|--------------------|-------|
	                     LEDscape Channel Indexes

As of this writing, a secondary pin mapping is available:

		                       LEDscape Channel Index
	 Row  Pin#       P9        Pin#  |  Pin#       P8        Pin# Row
	  1    1                    2    |   1                    2    1
	  2    3                    4    |   3                    4    2
	  3    5                    6    |   5                    6    3
	  4    7                    8    |   7     24      07     8    4
	  5    9                    10   |   9     25      06     10   5
	  6    11    40      41     12   |   11    26      05     12   6
	  7    13    42      43     14   |   13    27      04     14   7
	  8    15    44      45     16   |   15    28      03     16   8
	  9    17                   18   |   17    29      02     18   9
	  10   19                   20   |   19    30             20   10
	  11   21    46      47     22   |   21                   22   11
	  12   23    32             24   |   23                   24   12
	  13   25            33     26   |   25                   26   13
	  14   27            34     28   |   27    31             28   14
	  15   29    35      36     30   |   29    16      01     30   15
	  16   31    37             32   |   31    17      00     32   16
	  17   33                   34   |   33    18      15     34   17
	  18   35                   36   |   35    19      14     36   18
	  19   37                   38   |   37    20      13     38   19
	  20   39                   40   |   39    21      12     40   20
	  21   41     38     39     42   |   41    22      11     42   21
	  22   43                   44   |   43    23      10     44   22
	  23   45                   46   |   45    08      09     46   23
	  
	              ^       ^                    ^       ^
	              |-------|--------------------|-------|
	                     LEDscape Channel Indexes

Implementation Notes
========

The WS281x LED chips are built like shift registers and make for very
easy LED strip construction.  The signals are duty-cycle modulated,
with a 0 measuring 250 ns long and a 1 being 600 ns long, and 1250 ns
between bits.  Since this doesn't map to normal SPI hardware and requires
an 800 KHz bit clock, it is typically handled with a dedicated microcontroller
or DMA hardware on something like the Teensy 3.

However, the TI AM335x ARM Cortex-A8 in the BeagleBone Black has two
programmable "microcontrollers" built into the CPU that can handle realtime
tasks and also access the ARM's memory.  This allows things that
might have been delegated to external devices to be handled without
any additional hardware, and without the overhead of clocking data out
the USB port.

The frames are stored in memory as a series of 4-byte pixels in the
order GRBA, packed in strip-major order.  This means that it looks
like this in RAM:

	S0P0 S1P0 S2P0 ... S31P0 S0P1 S1P1 ... S31P1 S0P2 S1P2 ... S31P2

This way length of the strip can be variable, although the memory used
will depend on the length of the longest strip.  4 * 32 * longest strip
bytes are required per frame buffer.  The maximum frame rate also depends
on the length of th elongest strip.


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
