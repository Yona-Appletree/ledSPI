/** \file
 * LEDscape for the BeagleBone Black.
 *
 * Drives up to 32 ws281x LED strips using the PRU to have no CPU overhead.
 * Allows easy double buffering of frames.
 */

#ifndef _ledscape_h_
#define _ledscape_h_

#include <stdint.h>
#include "pru.h"

/** The number of strips supported.
 *
 * Changing this also requires changes in ws281x.p to stride the
 * correct number of bytes per row..
 */
#define LEDSCAPE_NUM_STRIPS 48


/**
 * An LEDscape "pixel" consists of three channels of output and an unused fourth channel. The color mapping of these
 * channels is not defined by the pixel construct, but is specified by color_channel_order_t. Use ledscape_pixel_set_color
 * to assign color values to a pixel.
 */
typedef struct {
	uint8_t a;// was blue
	uint8_t b;// was red
	uint8_t c;// was green
	uint8_t unused;
} __attribute__((__packed__)) ledscape_pixel_t;


/** LEDscape frame buffer is "strip-major".
 *
 * All 32 strips worth of data for each pixel are stored adjacent.
 * This makes it easier to clock out while reading from the DDR
 * in a burst mode.
 */
typedef struct {
	ledscape_pixel_t strip[LEDSCAPE_NUM_STRIPS];
} __attribute__((__packed__)) ledscape_frame_t;

typedef struct ws281x_command ws281x_command_t;

typedef struct {
	ws281x_command_t * ws281x_0;
	ws281x_command_t * ws281x_1;
	pru_t * pru0;
	pru_t * pru1;
	const char* pru0_program_filename;
	const char* pru1_program_filename;
	unsigned num_pixels;
	size_t frame_size;
} ledscape_t;


typedef enum {
	COLOR_ORDER_RGB,
	COLOR_ORDER_RBG,
	COLOR_ORDER_GRB,
	COLOR_ORDER_GBR,
	COLOR_ORDER_BGR,
	COLOR_ORDER_BRG // Old LEDscape default
} color_channel_order_t;


extern ledscape_t * ledscape_init(
unsigned num_pixels
);

extern ledscape_t * ledscape_init_with_programs(
	unsigned num_pixels,
	const char* pru0_program_filename,
	const char* pru1_program_filename
);


extern ledscape_frame_t *
ledscape_frame(
	ledscape_t * const leds,
	unsigned frame
);

extern void
ledscape_draw(
	ledscape_t * const leds,
	unsigned frame
);

extern inline void ledscape_set_color(
	ledscape_frame_t * const frame,
	color_channel_order_t color_channel_order,
	uint8_t strip,
	uint16_t pixel,
	uint8_t r,
	uint8_t g,
	uint8_t b
);

extern inline void ledscape_pixel_set_color(
	ledscape_pixel_t * const out_pixel,
	color_channel_order_t color_channel_order,
	uint8_t r,
	uint8_t g,
	uint8_t b
);

extern void
ledscape_wait(
	ledscape_t * const leds
);


extern void
ledscape_close(
	ledscape_t * const leds
);


extern const char* color_channel_order_to_string(
	color_channel_order_t color_channel_order
);

extern color_channel_order_t color_channel_order_from_string(
	const char* str
);

#endif
