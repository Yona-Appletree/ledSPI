![Testing LEDscape](http://farm4.staticflickr.com/3834/9378678019_b706c55635_z.jpg)

DANGER!
=======

This code works with the PRU units on the Beagle Bone and can easily
cause *hard crashes*.  It is still being debugged and developed.
Be careful hot-plugging things into the headers -- it is possible to
damage the pin drivers and cause problems in the ARM, especially if
there are +5V signals involved.


Overview
========

This is a modified version of the LEDscape library designed to control
up to 32 chains of WS2811-based LED modules from a Beagle Bone Black. The 
timing has been updated and verified to work with both WS2812 and WS2812b 
chips. This version of the library uses both PRUs on the Beagle Bone Black
and has approximately 2x the framerate of the original library.

Installation and Usage
========

To use LEDscape, download it to your BeagleBone Black. 

First, make sure that LEDscape compiles:

	make

Before LEDscape will function, you will need to replace the device tree
file and reboot.

	cp /boot/am335x-boneblack.dtb{,.preledscape_bk}
	cp am335x-boneblack.dtb /boot/
	reboot

You can now test LEDscape, run the following to display a map of how
the LEDscape strip ordering coreesponds to the GPIO pins on the BBB:

	node pinmap.js

Connect a WS2811-based LED chain to the Beagle Bone. The strip must
be running at the same voltage as the data signal. If you are using
an external 5v supply for the LEDs, you'll need to use a level shifter
or other technique to bring the BBB's 3.3v signals up to 5v.

Once everything is connected, run the `rgb-test` program:

	./rgb-test

The LEDs should now be fading prettily. If not, go back and make
sure everything is setup correctly.

At this point, you will probably want to install the ledscape service
which will run the UDP->LEDscape bridge on port 9999. You can then
send data to LEDscape from other programs or computers.

To install the service:

	systemctl enable /path/to/LEDscape/ledscape.service

To start or stop the service:

	systemctl start ledscape

Note that you must specify an absolute path. Relative paths will not
work with systemctl to enable services.

You can now send data to UDP port 9999. The format is:

	Strip 0     Strip 1   Strip 2
	RGBRGB...RGBRGBRGB....RGB


Disabling HDMI
========

If you need to use all 48 pins made available by LEDscape, you'll have
to disable the HDMI "cape" on the BeagleBone Black.

Mount the FAT32 partition, either through linux on the BeagleBone or
by plugging the USB into a computer, and add the following to the
first line of `uEnv.txt'

	capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN

It should read something like

	optargs=quiet drm.debug=7 capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN

Then reboot the BeagleBone Black.

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

<ledscape.h> defines the API. The key components are:

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

* 
* http://www.adafruit.com/products/1138
* http://www.adafruit.com/datasheets/WS2811.pdf
