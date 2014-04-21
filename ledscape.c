/** \file
 * Userspace interface to the WS281x LED strip driver.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include "ledscape.h"


/** GPIO pins used by the LEDscape.
 *
 * The device tree should handle this configuration for us, but it
 * seems horribly broken and won't configure these pins as outputs.
 * So instead we have to repeat them here as well.
 *
 * If these are changed, be sure to check the mappings in
 * ws281x.p!
 *
 * See https://github.com/ehayon/BeagleBone-GPIO/blob/master/src/am335x.h
 * for a complete list of pins.
 *
 * TODO: Find a way to unify this with the defines in the .p file
 */
static const uint8_t gpios0[] = {
	2, 3, 7, 8, 9, 10, 11, 14, 20, 22, 23, 26, 27, 30, 31
};

static const uint8_t gpios1[] = {
	12, 13, 14, 15, 16, 17, 18, 19, 28, 29
};

static const uint8_t gpios2[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 22, 23, 24, 25,
};

static const uint8_t gpios3[] = {
	14, 15, 16, 17, 19, 21
};

#define ARRAY_COUNT(a) ((sizeof(a) / sizeof(*a)))


/** Command structure shared with the PRU.
 *
 * This is mapped into the PRU data RAM and points to the
 * frame buffer in the shared DDR segment.
 *
 * Changing this requires changes in ws281x.p
 */
typedef struct ws281x_command
{
	// in the DDR shared with the PRU
	uintptr_t pixels_dma;

	// Length in pixels of the longest LED strip.
	unsigned num_pixels;

	// write 1 to start, 0xFF to abort. will be cleared when started
	volatile unsigned command;

	// will have a non-zero response written when done
	volatile unsigned response;
} __attribute__((__packed__)) ws281x_command_t;


/** Retrieve one of the two frame buffers. */
ledscape_frame_t *
ledscape_frame(
	ledscape_t * const leds,
	unsigned int frame
)
{
	if (frame >= 2)
		return NULL;

	return (ledscape_frame_t*)((uint8_t*) leds->pru0->ddr + leds->frame_size * frame);
}


/** Initiate the transfer of a frame to the LED strips */
void
ledscape_draw(
	ledscape_t * const leds,
	unsigned int frame
)
{

	leds->ws281x_0->pixels_dma = leds->pru0->ddr_addr + leds->frame_size * frame;
	leds->ws281x_1->pixels_dma = leds->pru0->ddr_addr + leds->frame_size * frame;

	// Wait for any current command to have been acknowledged
	while (leds->ws281x_0->command || leds->ws281x_1->command);

	// Zero the responses so we can wait for them
	leds->ws281x_0->response = leds->ws281x_1->response = 0;

	// Send the start command
	leds->ws281x_0->command = 1;
	leds->ws281x_1->command = 1;
}


/** Wait for the current frame to finish transfering to the strips.
 * \returns a token indicating the response code.
 */
uint32_t
ledscape_wait(
	ledscape_t * const leds
)
{
	while (1)
	{
		uint32_t response0 = leds->ws281x_0->response;
		uint32_t response1 = leds->ws281x_1->response;

		// printf("pru0: (%d,%d), pru1: (%d,%d)\n",
		// 	leds->ws281x_0->command, leds->ws281x_0->response,
		// 	leds->ws281x_1->command, leds->ws281x_1->response
		// );

		if (response0 && response1) {
			leds->ws281x_0->response = leds->ws281x_1->response = 0;
			// TODO: How to handle both return values?
			return response0;
		}
	}
}


ledscape_t *
ledscape_init(
	unsigned num_pixels
)
{
	return ledscape_init_with_modes(num_pixels, WS281x, WS281x);
}

ledscape_t *
ledscape_init_with_modes(
	unsigned num_pixels,
	ledscape_output_mode_t pru0_mode,
	ledscape_output_mode_t pru1_mode
)
{
	pru_t * const pru0 = pru_init(0);
	pru_t * const pru1 = pru_init(1);

	const size_t frame_size = num_pixels * LEDSCAPE_NUM_STRIPS * 4;

	if (2 *frame_size > pru0->ddr_size)
		die("Pixel data needs at least 2 * %zu, only %zu in DDR\n",
			frame_size,
			pru0->ddr_size
		);

	ledscape_t * const leds = calloc(1, sizeof(*leds));

	*leds = (ledscape_t) {
		.pru0		= pru0,
		.pru1		= pru1,
		.num_pixels	= num_pixels,
		.frame_size	= frame_size,
		.pru0_mode  = pru0_mode,
		.pru1_mode  = pru1_mode,
		.ws281x_0	= pru0->data_ram,
		.ws281x_1	= pru1->data_ram
	};

	*(leds->ws281x_0) = *(leds->ws281x_1) = (ws281x_command_t) {
		.pixels_dma	= 0, // will be set in draw routine
		.command	= 0,
		.response	= 0,
		.num_pixels	= leds->num_pixels,
	};

	// Configure all of our output pins.
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios0) ; i++)
		pru_gpio(0, gpios0[i], 1, 0);
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios1) ; i++)
		pru_gpio(1, gpios1[i], 1, 0);
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios2) ; i++)
		pru_gpio(2, gpios2[i], 1, 0);
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios3) ; i++)
		pru_gpio(3, gpios3[i], 1, 0);

	// Initiate the PRU0 program
	const char* pru0_program_filename;
	switch (pru0_mode) {
		case WS281x: pru0_program_filename = "./ws281x_0.bin"; break;
		case DMX: pru0_program_filename = "./dmx_0.bin"; break;
		default:
			warn("Invalid PRU0 Mode.");
			exit(-1);
	}
	pru_exec(pru0, pru0_program_filename);

	// Watch for a done response that indicates a proper startup
	// \todo timeout if it fails
	fprintf(stdout, "String PRU0 with %s... ", pru0_program_filename);
	while (!leds->ws281x_0->response);
	printf("OK\n");


	// Initiate the PRU1 program
	const char* pru1_program_filename;
	switch (pru1_mode) {
		case WS281x: pru1_program_filename = "./ws281x_1.bin"; break;
		case DMX:
			warn("PRU1 does not currently support DMX.");
			exit(-1);
			//pru1_program_filename = "./dmx_1.bin";
		break;
		default:
			warn("Invalid PRU1 Mode.");
			exit(-1);
	}
	pru_exec(pru1, pru1_program_filename);

	// Watch for a done response that indicates a proper startup
	// \todo timeout if it fails
	fprintf(stdout, "String PRU1 with %s... ", pru1_program_filename);
	while (!leds->ws281x_1->response);
	printf("OK\n");

	return leds;
}


void
ledscape_close(
	ledscape_t * const leds
)
{
	// Signal a halt command
	leds->ws281x_0->command = 0xFF;
	leds->ws281x_1->command = 0xFF;
	pru_close(leds->pru0);
	pru_close(leds->pru1);
}


void
ledscape_set_color(
	ledscape_frame_t * const frame,
	uint8_t strip,
	uint8_t pixel,
	uint8_t r,
	uint8_t g,
	uint8_t b
)
{
	ledscape_pixel_t * const p = &frame[pixel].strip[strip];
	p->r = r;
	p->g = g;
	p->b = b;
}
