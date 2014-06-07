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


/** LEDscape pixel format is BRGA.
 *
 * data is laid out with BRGA format, since that is how it will
 * be translated during the clock out from the PRU.
 */
typedef struct {
	uint8_t b;
	uint8_t r;
	uint8_t g;
	uint8_t a;
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

typedef enum {
	WS281x = 0,
	DMX = 1,
	WS2801 = 2,
	WS2801_NEWPINS = 3,
} ledscape_output_mode_t;

typedef struct ws281x_command ws281x_command_t;

typedef struct {
	ws281x_command_t * ws281x_0;
	ws281x_command_t * ws281x_1;
	pru_t * pru0;
	pru_t * pru1;
	ledscape_output_mode_t pru0_mode;
	ledscape_output_mode_t pru1_mode;
	unsigned num_pixels;
	size_t frame_size;
} ledscape_t;

extern ledscape_t *
ledscape_init(
	unsigned num_pixels
);

extern ledscape_t *
ledscape_init_with_modes(
	unsigned num_pixels,
	ledscape_output_mode_t pru0_mode,
	ledscape_output_mode_t pru1_mode
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


extern void
ledscape_set_color(
	ledscape_frame_t * const frame,
	uint8_t strip,
	uint16_t pixel,
	uint8_t r,
	uint8_t g,
	uint8_t b
);


extern uint32_t
ledscape_wait(
	ledscape_t * const leds
);


extern void
ledscape_close(
	ledscape_t * const leds
);


extern const char* ledscape_output_mode_to_string(ledscape_output_mode_t mode);

#endif
