/** \file
 *  OPC image packet receiver.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include "util.h"
#include "ledscape.h"

#include "lib/cesanta/net_skeleton.h"
#include "lib/cesanta/frozen.h"

#include <pthread.h>

// TODO:
// Server:
// 	- ip-stack Agnostic socket stuff
//  - UDP receiver
// Config:
//  - White-balance, curve adjustment
//  - Respecting interpolation and dithering settings


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
#define TRUE 1
#define FALSE 0

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Method declarations
void teardown_server();
void start_server();

// Frame Manipulation
void ensure_frame_data();
void set_next_frame_data(uint8_t* frame_data, uint32_t data_size, uint8_t is_remote);
void rotate_frames();

// Threads
void* render_thread(void* threadarg);
void* udp_server_thread(void* threadarg);
void* tcp_server_thread(void* threadarg);
void* demo_thread(void* threadarg);

// Config Methods
void build_lookup_tables();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global Data
static struct {
	ledscape_output_mode_t pru0_mode;
	ledscape_output_mode_t pru1_mode;

	uint16_t tcp_port;
	uint16_t udp_port;
	uint16_t leds_per_strip;
	uint16_t used_strip_count;
	uint8_t demo_enabled;

	uint8_t interpolation_enabled;
	uint8_t dithering_enabled;
	uint8_t lut_enabled;

	struct {
		float red;
		float green;
		float blue;
	} white_point;

	float lum_power;

	pthread_mutex_t mutex;
	char json[4096];
} g_server_config = {
	.pru0_mode = WS281x,
	.pru1_mode = WS281x,
	.tcp_port = 7890,
	.udp_port = 7890,
	.leds_per_strip = 176,
	.used_strip_count = LEDSCAPE_NUM_STRIPS,
	.demo_enabled = TRUE,
	.interpolation_enabled = TRUE,
	.dithering_enabled = TRUE,
	.lut_enabled = TRUE,
	.white_point = { .9, 1, 1},
	.lum_power = 2,
	.mutex = PTHREAD_MUTEX_INITIALIZER
};

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} __attribute__((__packed__)) buffer_pixel_t;

typedef struct {
	int8_t r;
	int8_t g;
	int8_t b;

	int8_t last_effect_frame_r;
	int8_t last_effect_frame_g;
	int8_t last_effect_frame_b;
} __attribute__((__packed__)) pixel_delta_t;

static struct
{
	buffer_pixel_t* previous_frame_data;
	buffer_pixel_t* current_frame_data;
	buffer_pixel_t* next_frame_data;

	pixel_delta_t* frame_dithering_overflow;

	uint8_t has_prev_frame;
	uint8_t has_current_frame;
	uint8_t has_next_frame;

	uint32_t frame_size;

	struct timeval previous_frame_tv;
	struct timeval current_frame_tv;
	struct timeval next_frame_tv;

	struct timeval prev_current_delta_tv;

	ledscape_t * leds;

	uint32_t red_lookup[257];
	uint32_t green_lookup[257];
	uint32_t blue_lookup[257];

	struct timeval last_remote_data_tv;

	pthread_mutex_t mutex;
} g_frame_data = {
	.previous_frame_data = (buffer_pixel_t*)NULL,
	.current_frame_data = (buffer_pixel_t*)NULL,
	.next_frame_data = (buffer_pixel_t*)NULL,
	.has_prev_frame = FALSE,
	.has_current_frame = FALSE,
	.has_next_frame = FALSE,
	.frame_dithering_overflow = (pixel_delta_t*)NULL,
	.frame_size = 0,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.last_remote_data_tv = {
		.tv_sec = 0,
		.tv_usec = 0
	},
	.leds = NULL
};

static struct
{
	pthread_t render_thread;
	pthread_t tcp_server_thread;
	pthread_t udp_server_thread;
	pthread_t demo_thread;
} g_threads;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// main()
static struct option long_options[] =
{
    {"tcp-port", optional_argument, NULL, 'p'},
    {"udp-port", optional_argument, NULL, 'P'},
    {"count", optional_argument, NULL, 'c'},
    {"strip-count", optional_argument, NULL, 's'},
    {"dimensions", optional_argument, NULL, 'd'},

    {"no-demo", no_argument, NULL, 'D'},

    {"no-interpolation", no_argument, NULL, 'i'},
    {"no-dithering", no_argument, NULL, 't'},
    {"no-lut", no_argument, NULL, 'l'},

    {"lum_power", optional_argument, NULL, 'L'},

    {"red_bal", optional_argument, NULL, 'r'},
    {"green_bal", optional_argument, NULL, 'g'},
    {"blue_bal", optional_argument, NULL, 'b'},

    {"pru0_mode", optional_argument, NULL, '0'},
    {"pru1_mode", optional_argument, NULL, '1'},

    {NULL, 0, NULL, 0}
};

int main(int argc, char ** argv)
{
	extern char *optarg;
	int opt;
	while ((opt = getopt_long(argc, argv, "p:P:c:s:d:DitlL:r:g:b:0:1:", long_options, NULL)) != -1)
	{
		switch (opt)
		{
		case 'p': {
			g_server_config.tcp_port = atoi(optarg);
		} break;

		case 'P': {
			g_server_config.udp_port = atoi(optarg);
		} break;

		case 'c': {
			g_server_config.leds_per_strip = atoi(optarg);
		} break;

		case 's': {
			g_server_config.used_strip_count = atoi(optarg);
		} break;

		case 'd': {
			int width=0, height=0;

			if (sscanf(optarg,"%dx%d", &width, &height) == 2) {
				g_server_config.leds_per_strip = width * height;
			} else {
				printf("Invalid argument for -d; expected NxN; actual: %s", optarg);
				exit(EXIT_FAILURE);
			}
		} break;

		case 'D': {
			g_server_config.demo_enabled = FALSE;
		} break;

		case 'i': {
			g_server_config.interpolation_enabled = FALSE;
		} break;

		case 't': {
			g_server_config.dithering_enabled = FALSE;
		} break;

		case 'l': {
			g_server_config.lut_enabled = FALSE;
		} break;

		case 'L': {
			g_server_config.lum_power = atof(optarg);
		} break;

		case 'r': {
			g_server_config.white_point.red = atof(optarg);
		} break;

		case 'g': {
			g_server_config.white_point.green = atof(optarg);
		} break;

		case 'b': {
			g_server_config.white_point.blue = atof(optarg);
		} break;

		case '0': {
			g_server_config.pru0_mode = ledscape_output_mode_from_string(optarg);
		} break;

		case '1': {
			g_server_config.pru1_mode = ledscape_output_mode_from_string(optarg);
		} break;

		default:
			fprintf(stderr, "Usage: %s [-p <port>] [-c <led_count> | -d <width>x<height>] [-D | --no-demo]"
			    "[-i | --no-interpolation] [-t | --no-dithering] [-l | --no-lut] [-L | lum_power <lum_power>] "
				"[-r | -red_bal <red_bal>] [-g | -green_bal <green_bal>] [-b | -blue_bal <blue_bal>]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// largest possible UDP packet
	if (g_server_config.leds_per_strip*LEDSCAPE_NUM_STRIPS*3 >= 65536) {
		die("[main] %u pixels cannot fit in a UDP packet.\n", g_server_config.leds_per_strip);
	}

	fprintf(stderr,
		"[main] Starting server on ports (tcp=%d, udp=%d) for %d pixels on %d strips\n",
		g_server_config.tcp_port, g_server_config.udp_port, g_server_config.leds_per_strip, LEDSCAPE_NUM_STRIPS
	);
	fprintf(stderr, g_server_config.json);

	pthread_create(&g_threads.render_thread, NULL, render_thread, NULL);
	pthread_create(&g_threads.udp_server_thread, NULL, udp_server_thread, NULL);
	pthread_create(&g_threads.tcp_server_thread, NULL, tcp_server_thread, NULL);

	if (g_server_config.demo_enabled) {
		printf("Demo Mode Enabled\n");
		pthread_create(&g_threads.demo_thread, NULL, demo_thread, NULL);
	} else {
		printf("Demo Mode Disabled\n");
	}

	start_server();

	pthread_exit(NULL);
}

void teardown_server() {
	printf("Tearing down server...\n");

	pthread_mutex_lock(&g_frame_data.mutex);
	pthread_mutex_lock(&g_server_config.mutex);

	if (g_frame_data.leds) {
		ledscape_close(g_frame_data.leds);
		g_frame_data.leds = NULL;
	}

	pthread_mutex_unlock(&g_server_config.mutex);
	pthread_mutex_unlock(&g_frame_data.mutex);

	printf("Teardown Complete.\n");
}

void start_server() {
	printf("Starting server...");

	// Ensure we're not over the pixel limit
	if (g_server_config.leds_per_strip*LEDSCAPE_NUM_STRIPS*3 >= 65536) {
		fprintf(stderr, "%u pixels cannot fit in a UDP packet.\n", g_server_config.leds_per_strip);
		return;
	}

	// Setup tables
	build_lookup_tables();
	ensure_frame_data();

	// Init LEDscape
	g_frame_data.leds = ledscape_init_with_modes(
		g_server_config.leds_per_strip,
		g_server_config.pru0_mode,
		g_server_config.pru1_mode
	);
}

void load_config_file() {

}

void save_config_file() {
	// Build config JSON
	sprintf(
		g_server_config.json,
		"{\n"
			"\t" "\"pru0Mode\": \"%s\"," "\n"
			"\t" "\"pru1Mode\": \"%s\"," "\n"

			"\t" "\"ledsPerStrip\": %d," "\n"
			"\t" "\"usedStripCount\": %d," "\n"

			"\t" "\"tcpPort\": %d," "\n"
			"\t" "\"udpPort\": %d," "\n"

			"\t" "\"enableInterpolation\": %s," "\n"
			"\t" "\"enableDithering\": %s," "\n"
			"\t" "\"enableLookupTable\": %s," "\n"

			"\t" "\"lumCurvePower\": %.4f," "\n"
			"\t" "\"whitePoint\": {" "\n"
			"\t\t" "\"red\": %.4f," "\n"
			"\t\t" "\"green\": %.4f," "\n"
			"\t\t" "\"blue\": %.4f" "\n"
			"\t" "}" "\n"
		"}\n",

		ledscape_output_mode_to_string(g_frame_data.leds->pru0_mode),
		ledscape_output_mode_to_string(g_frame_data.leds->pru1_mode),

		g_server_config.leds_per_strip,
		g_server_config.used_strip_count,

		g_server_config.tcp_port,
		g_server_config.udp_port,

		g_server_config.interpolation_enabled ? "true" : "false",
		g_server_config.dithering_enabled ? "true" : "false",
		g_server_config.lut_enabled ? "true" : "false",

		(double)g_server_config.lum_power,
		(double)g_server_config.white_point.red,
		(double)g_server_config.white_point.green,
		(double)g_server_config.white_point.blue
	);

	fprintf(stderr,
		"[main] Starting server on ports (tcp=%d, udp=%d) for %d pixels on %d strips\n",
		g_server_config.tcp_port, g_server_config.udp_port, g_server_config.leds_per_strip, LEDSCAPE_NUM_STRIPS
	);
	fprintf(stderr, g_server_config.json);

	build_lookup_tables();
	ensure_frame_data();

	pthread_create(&g_threads.render_thread, NULL, render_thread, NULL);
	pthread_create(&g_threads.udp_server_thread, NULL, udp_server_thread, NULL);
	pthread_create(&g_threads.tcp_server_thread, NULL, tcp_server_thread, NULL);
	pthread_create(&g_threads.demo_thread, NULL, demo_thread, NULL);
	if (g_server_config.demo_enabled) {
		pthread_create(&g_threads.demo_thread, NULL, demo_thread, NULL);
	}

	pthread_exit(NULL);
}

void build_lookup_tables() {
	pthread_mutex_lock(&g_frame_data.mutex);
	pthread_mutex_lock(&g_server_config.mutex);

	float white_points[] = {
		g_server_config.white_point.red,
		g_server_config.white_point.green,
		g_server_config.white_point.blue
	};

	uint32_t* lookup_tables[] = {
		g_frame_data.red_lookup,
		g_frame_data.green_lookup,
		g_frame_data.blue_lookup
	};

	for (uint16_t c=0; c<3; c++) {
		for (uint16_t i=0; i<257; i++) {
			double normalI = (double)i / 256;
			normalI *= white_points[c];

			double output = pow(normalI, g_server_config.lum_power);
			int64_t longOutput = (output * 0xFFFF) + 0.5;
			int32_t clampedOutput = max(0, min(0xFFFF, longOutput));

			lookup_tables[c][i] = clampedOutput;
		}
	}

	pthread_mutex_unlock(&g_server_config.mutex);
	pthread_mutex_unlock(&g_frame_data.mutex);
}

/**
 * Ensure that the frame buffers are allocated to the correct values.
 */
void ensure_frame_data() {
	pthread_mutex_lock(&g_server_config.mutex);
	uint32_t led_count = g_server_config.leds_per_strip * LEDSCAPE_NUM_STRIPS;
	pthread_mutex_unlock(&g_server_config.mutex);

	pthread_mutex_lock(&g_frame_data.mutex);
	if (g_frame_data.frame_size != led_count) {
		fprintf(stderr, "Allocating buffers for %d pixels (%d bytes)\n", led_count, led_count * 3 /*channels*/ * 4 /*buffers*/ * sizeof(uint16_t));

		if (g_frame_data.previous_frame_data != NULL) {
			free(g_frame_data.previous_frame_data);
			free(g_frame_data.current_frame_data);
			free(g_frame_data.next_frame_data);
			free(g_frame_data.frame_dithering_overflow);
		}

		g_frame_data.frame_size = led_count;
		g_frame_data.previous_frame_data = malloc(led_count * sizeof(buffer_pixel_t));
		g_frame_data.current_frame_data = malloc(led_count * sizeof(buffer_pixel_t));
		g_frame_data.next_frame_data = malloc(led_count * sizeof(buffer_pixel_t));
		g_frame_data.frame_dithering_overflow = malloc(led_count * sizeof(pixel_delta_t));
		g_frame_data.has_next_frame = FALSE;
		printf("frame_size1=%u\n", g_frame_data.frame_size);

		// Init timestamps
		gettimeofday(&g_frame_data.previous_frame_tv, NULL);
		gettimeofday(&g_frame_data.current_frame_tv, NULL);
		gettimeofday(&g_frame_data.next_frame_tv, NULL);
	}
	pthread_mutex_unlock(&g_frame_data.mutex);
}

/**
 * Set the next frame of data to the given 8-bit RGB buffer after rotating the buffers.
 */
void set_next_frame_data(uint8_t* frame_data, uint32_t data_size, uint8_t is_remote) {
	rotate_frames();

	pthread_mutex_lock(&g_frame_data.mutex);

	// Prevent buffer overruns
	data_size = min(data_size, g_frame_data.frame_size * 3);

	// Copy in new data
	memcpy(g_frame_data.next_frame_data, frame_data, data_size);

	// Zero out any pixels not set by the new frame
	memset((uint8_t*)g_frame_data.next_frame_data + data_size, 0, (g_frame_data.frame_size*3 - data_size));

	// Update the timestamp & count
	gettimeofday(&g_frame_data.next_frame_tv, NULL);

	// Update remote data timestamp if applicable
	if (is_remote) {
		gettimeofday(&g_frame_data.last_remote_data_tv, NULL);
	}

	g_frame_data.has_next_frame = TRUE;

	pthread_mutex_unlock(&g_frame_data.mutex);
}

/**
 * Rotate the buffers, dropping the previous frame and loading in the new one
 */
void rotate_frames() {
	pthread_mutex_lock(&g_frame_data.mutex);

	buffer_pixel_t* temp = NULL;

	g_frame_data.has_prev_frame = FALSE;

	if (g_frame_data.has_current_frame) {
		g_frame_data.previous_frame_tv = g_frame_data.current_frame_tv;

		temp = g_frame_data.previous_frame_data;
		g_frame_data.previous_frame_data = g_frame_data.current_frame_data;
		g_frame_data.current_frame_data = temp;

		g_frame_data.has_prev_frame = TRUE;
		g_frame_data.has_current_frame = FALSE;
	}

	if (g_frame_data.has_next_frame) {
		g_frame_data.current_frame_tv = g_frame_data.next_frame_tv;

		temp = g_frame_data.current_frame_data;
		g_frame_data.current_frame_data = g_frame_data.next_frame_data;
		g_frame_data.next_frame_data = temp;

		g_frame_data.has_current_frame = TRUE;
		g_frame_data.has_next_frame = FALSE;
	}

	// Update the delta time stamp
	if (g_frame_data.has_current_frame && g_frame_data.has_prev_frame) {
		timersub(
			&g_frame_data.current_frame_tv,
			&g_frame_data.previous_frame_tv,
			&g_frame_data.prev_current_delta_tv
		);
	}

	pthread_mutex_unlock(&g_frame_data.mutex);
}

inline uint16_t lutInterpolate(uint16_t value, uint32_t* lut) {
	// Inspired by FadeCandy: https://github.com/scanlime/fadecandy/blob/master/firmware/fc_pixel_lut.cpp

	uint16_t index = value >> 8; // Range [0, 0xFF]
	uint16_t alpha = value & 0xFF; // Range [0, 0xFF]
	uint16_t invAlpha = 0x100 - alpha; // Range [1, 0x100]

	// Result in range [0, 0xFFFF]
	return (lut[index] * invAlpha + lut[index + 1] * alpha) >> 8;
}

void* render_thread(void* unused_data)
{
	unused_data=unused_data; // Suppress Warnings
	fprintf(stderr, "[render] Starting render thread for %u total pixels\n", g_server_config.leds_per_strip * LEDSCAPE_NUM_STRIPS);

	// Timing Variables
	struct timeval frame_progress_tv, now_tv;
	uint16_t frame_progress16, inv_frame_progress16;

	const unsigned report_interval = 10;
	unsigned last_report = 0;
	unsigned long delta_sum = 0;
	unsigned frames = 0;
	uint32_t delta_avg = 2000;

	uint8_t buffer_index = 0;
	int8_t ditheringFrame = 0;
	for(;;) {
		pthread_mutex_lock(&g_frame_data.mutex);

		// Wait until LEDscape is initialized
		if (g_frame_data.leds == NULL) {
			printf("[render] Awaiting server initialization...\n");
			pthread_mutex_unlock(&g_frame_data.mutex);
			usleep(1e6 /* 1s */);
			continue;
		}

		// Skip frames if there isn't enough data
		if (!g_frame_data.has_prev_frame || !g_frame_data.has_current_frame) {
			pthread_mutex_unlock(&g_frame_data.mutex);
			usleep(10e3 /* 10ms */);
			continue;
		}

		// Calculate the time delta and current percentage (as a 16-bit value)
		gettimeofday(&now_tv, NULL);
		timersub(&now_tv, &g_frame_data.next_frame_tv, &frame_progress_tv);

		// Calculate current frame and previous frame time
		uint64_t frame_progress_us = frame_progress_tv.tv_sec*1e6 + frame_progress_tv.tv_usec;
		uint64_t last_frame_time_us = g_frame_data.prev_current_delta_tv.tv_sec*1e6 + g_frame_data.prev_current_delta_tv.tv_usec;

		// Check for current frame exhaustion
		if (frame_progress_us > last_frame_time_us) {
			uint8_t has_next_frame = g_frame_data.has_next_frame;
			pthread_mutex_unlock(&g_frame_data.mutex);

			// This should only happen in a final frame case -- to avoid early switching (and some nasty resulting
			// artifacts) we only force frame rotation if the next frame is really late.
			if (has_next_frame && frame_progress_us > last_frame_time_us*2) {
				// If we have more data, rotate it in.
				//printf("Need data: rotating in; frame_progress_us=%llu; last_frame_time_us=%llu\n", frame_progress_us, last_frame_time_us);
				rotate_frames();
			} else {
				// Otherwise sleep for a moment and wait for more data
				//printf("Need data: none available; frame_progress_us=%llu; last_frame_time_us=%llu\n", frame_progress_us, last_frame_time_us);
				usleep(1e3);
			}

			continue;
		}

		frame_progress16 = (frame_progress_us << 16) / last_frame_time_us;
		inv_frame_progress16 = 0xFFFF - frame_progress16;

		if (frame_progress_tv.tv_sec > 5) {
			printf("[render] No data for 5 seconds; suspending render thread.\n");
			pthread_mutex_unlock(&g_frame_data.mutex);
			usleep(100e3 /* 100ms */);
			continue;
		}

		// printf("%d of %d (%d)\n",
		// 	(frame_progress_tv.tv_sec*1000000 + frame_progress_tv.tv_usec) ,
		// 	(g_frame_data.prev_current_delta_tv.tv_sec*1000000 + g_frame_data.prev_current_delta_tv.tv_usec),
		// 	frame_progress16
		// );

		// Setup LEDscape for this frame
		buffer_index = (buffer_index+1)%2;
		ledscape_frame_t * const frame = ledscape_frame(g_frame_data.leds, buffer_index);

		// Build the render frame
		uint16_t led_count = g_frame_data.frame_size;
		uint16_t leds_per_strip = led_count / LEDSCAPE_NUM_STRIPS;
		uint32_t data_index = 0;

		// Update the dithering frame counter
		ditheringFrame ++;

		// Timing stuff
		struct timeval start_tv, stop_tv, delta_tv;
		gettimeofday(&start_tv, NULL);

		uint16_t used_strip_count;

		// Check the server config for dithering and interpolation options
		pthread_mutex_lock(&g_server_config.mutex);

		// Use the strip count from configs. This can save time that would be used dithering
		used_strip_count = min(g_server_config.used_strip_count, LEDSCAPE_NUM_STRIPS);

		// Only enable dithering if we're better than 100fps
		uint8_t dithering_enabled = (delta_avg < 10000) && g_server_config.dithering_enabled;
		uint8_t interpolation_enabled = g_server_config.interpolation_enabled;
		uint8_t lut_enabled = g_server_config.lut_enabled;
		pthread_mutex_unlock(&g_server_config.mutex);

		// Only allow dithering to take effect if it blinks faster than 60fps
		uint32_t maxDitherFrames = 16667 / delta_avg;

		for (uint32_t strip_index=0; strip_index<used_strip_count; strip_index++) {
			for (uint32_t led_index=0; led_index<leds_per_strip; led_index++, data_index++) {
				buffer_pixel_t* pixel_in_prev = &g_frame_data.previous_frame_data[data_index];
				buffer_pixel_t* pixel_in_current = &g_frame_data.current_frame_data[data_index];
				pixel_delta_t* pixel_in_overflow = &g_frame_data.frame_dithering_overflow[data_index];

				ledscape_pixel_t* const pixel_out = & frame[led_index].strip[strip_index];

				int32_t interpolatedR;
				int32_t interpolatedG;
				int32_t interpolatedB;

				// Interpolate
				if (interpolation_enabled) {
					interpolatedR = (pixel_in_prev->r*inv_frame_progress16 + pixel_in_current->r*frame_progress16) >> 8;
					interpolatedG = (pixel_in_prev->g*inv_frame_progress16 + pixel_in_current->g*frame_progress16) >> 8;
					interpolatedB = (pixel_in_prev->b*inv_frame_progress16 + pixel_in_current->b*frame_progress16) >> 8;
				} else {
					interpolatedR = pixel_in_current->r << 8;
					interpolatedG = pixel_in_current->g << 8;
					interpolatedB = pixel_in_current->b << 8;
				}

				// Apply LUT
				if (lut_enabled) {
					interpolatedR = lutInterpolate(interpolatedR, g_frame_data.red_lookup);
					interpolatedG = lutInterpolate(interpolatedG, g_frame_data.green_lookup);
					interpolatedB = lutInterpolate(interpolatedB, g_frame_data.blue_lookup);
				}

				// Reset dithering for this pixel if it's been too long since it actually changed anything. This serves to prevent
				// visible blinking pixels.
				if (abs(abs(pixel_in_overflow->last_effect_frame_r) - abs(ditheringFrame)) > maxDitherFrames) {
					pixel_in_overflow->r = 0;
					pixel_in_overflow->last_effect_frame_r = ditheringFrame;
				}

				if (abs(abs(pixel_in_overflow->last_effect_frame_g) - abs(ditheringFrame)) > maxDitherFrames) {
					pixel_in_overflow->g = 0;
					pixel_in_overflow->last_effect_frame_g = ditheringFrame;
				}

				if (abs(abs(pixel_in_overflow->last_effect_frame_b) - abs(ditheringFrame)) > maxDitherFrames) {
					pixel_in_overflow->b = 0;
					pixel_in_overflow->last_effect_frame_b = ditheringFrame;
				}

				// Apply dithering overflow
				int32_t	ditheredR = interpolatedR;
				int32_t	ditheredG = interpolatedG;
				int32_t	ditheredB = interpolatedB;

				if (dithering_enabled) {
					ditheredR += pixel_in_overflow->r;
					ditheredG += pixel_in_overflow->g;
					ditheredB += pixel_in_overflow->b;
				}

				// Calculate and assign output values
				uint8_t r = pixel_out->r = min((ditheredR+0x80) >> 8, 255);
				uint8_t g = pixel_out->g = min((ditheredG+0x80) >> 8, 255);
				uint8_t b = pixel_out->b = min((ditheredB+0x80) >> 8, 255);

				// Check for interpolation effect
				if (r != (interpolatedR+0x80)>>8) pixel_in_overflow->last_effect_frame_r = ditheringFrame;
				if (g != (interpolatedG+0x80)>>8) pixel_in_overflow->last_effect_frame_g = ditheringFrame;
				if (b != (interpolatedB+0x80)>>8) pixel_in_overflow->last_effect_frame_b = ditheringFrame;

				// Recalculate Overflow
				// NOTE: For some strange reason, reading the values from pixel_out causes strange memory corruption. As such
				// we use temporary variables, r, g, and b. It probably has to do with things being loaded into the CPU cache
				// when read, as such, don't read pixel_out from here.
				if (dithering_enabled) {
					pixel_in_overflow->r = (int16_t)ditheredR - (r * 257);
					pixel_in_overflow->g = (int16_t)ditheredG - (g * 257);
					pixel_in_overflow->b = (int16_t)ditheredB - (b * 257);
				}
			}
		}

		// Render the frame
		ledscape_wait(g_frame_data.leds);
		ledscape_draw(g_frame_data.leds, buffer_index);

		pthread_mutex_unlock(&g_frame_data.mutex);

		// Output Timing Info
		gettimeofday(&stop_tv, NULL);
		timersub(&stop_tv, &start_tv, &delta_tv);

		frames++;
		delta_sum += delta_tv.tv_usec;
		if (stop_tv.tv_sec - last_report < report_interval)
			continue;
		last_report = stop_tv.tv_sec;

		delta_avg = delta_sum / frames;
		printf("[render] fps_info={frame_avg_usec: %6u, possible_fps: %.2f, actual_fps: %.2f, sample_frames: %u}\n",
			delta_avg,
			(1.0e6 / delta_avg),
			frames * 1.0 / report_interval,
			frames
		);

		frames = delta_sum = 0;
	}

	ledscape_close(g_frame_data.leds);
	pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Server Common

typedef struct
{
	uint8_t channel;
	uint8_t command;
	uint8_t len_hi;
	uint8_t len_lo;
} opc_cmd_t;

typedef enum
{
	OPC_SYSID_FADECANDY = 1,

	// Pending approval from the OPC folks
	OPC_SYSID_LEDSCAPE = 2
} opc_system_id_t;

typedef enum
{
	OPC_LEDSCAPE_CMD_GET_CONFIG = 1
} opc_ledscape_cmd_id_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Demo Data Thread
//

void HSBtoRGB(int32_t hue, int32_t sat, int32_t val, uint8_t out[]) {
	/* convert hue, saturation and brightness ( HSB/HSV ) to RGB
		 The dim_curve is used only on brightness/value and on saturation (inverted).
		 This looks the most natural.
	*/

	int r=0, g=0, b=0;
	int base;

	if (sat == 0) { // Achromatic color (gray). Hue doesn't mind.
		r = g = b = val;
	} else  {
		base = ((255 - sat) * val)>>8;

		switch((hue%360)/60) {
		case 0:
				r = val;
				g = (((val-base)*hue)/60)+base;
				b = base;
		break;

		case 1:
				r = (((val-base)*(60-(hue%60)))/60)+base;
				g = val;
				b = base;
		break;

		case 2:
				r = base;
				g = val;
				b = (((val-base)*(hue%60))/60)+base;
		break;

		case 3:
				r = base;
				g = (((val-base)*(60-(hue%60)))/60)+base;
				b = val;
		break;

		case 4:
				r = (((val-base)*(hue%60))/60)+base;
				g = base;
				b = val;
		break;

		case 5:
				r = val;
				g = base;
				b = (((val-base)*(60-(hue%60)))/60)+base;
		break;
		}

		out[0] = r;
		out[1] = g;
		out[2] = b;
	}
}

void* demo_thread(void* unused_data)
{
	unused_data=unused_data; // Suppress Warnings
	fprintf(stderr, "Starting demo data thread\n");

	uint8_t* buffer = NULL;
	uint32_t buffer_size = 0;

	struct timeval now_tv, delta_tv;
	uint8_t demo_enabled = FALSE;

	for (uint16_t i = 0; /*ever*/; i+=4) {
		// Calculate time since last remote data
		pthread_mutex_lock(&g_frame_data.mutex);
		gettimeofday(&now_tv, NULL);
		timersub(&now_tv, &g_frame_data.last_remote_data_tv, &delta_tv);
		pthread_mutex_unlock(&g_frame_data.mutex);

		// Enable/disable demo mode and log
		if (delta_tv.tv_sec > 5) {
			if (! demo_enabled) {
				printf("[demo] Starting Demo\n");
			}

			demo_enabled = TRUE;
		} else {
			if (demo_enabled) {
				printf("[demo] Stopping Demo\n");
			}

			demo_enabled = FALSE;
		}

		if (demo_enabled) {
			// Demo mode
			pthread_mutex_lock(&g_server_config.mutex);
			uint32_t leds_per_strip = g_server_config.leds_per_strip;
			uint32_t channel_count = g_server_config.leds_per_strip*3*LEDSCAPE_NUM_STRIPS;
			pthread_mutex_unlock(&g_server_config.mutex);

			if (buffer_size != channel_count) {
				if (buffer != NULL) free(buffer);
				buffer = malloc(buffer_size = channel_count);
				memset(buffer, 0, buffer_size);
			}

			for (uint32_t strip = 0, data_index = 0 ; strip < LEDSCAPE_NUM_STRIPS ; strip++)
			{
				for (uint16_t p = 0 ; p < leds_per_strip; p++, data_index+=3)
				{
					HSBtoRGB(
						((i + ((p + strip*leds_per_strip)*360)/(leds_per_strip*10)) % 360),
						200,
						128 - (((i/10) + (p*96)/leds_per_strip + strip*10) % 96),
						&buffer[data_index]
					);
//
//					buffer[data_index] = strip;
//					buffer[data_index+2] = strip;
//					buffer[data_index+1] = strip;
				}
			}

			set_next_frame_data(buffer, buffer_size, FALSE);
		}

		usleep(1e6 / 30);
	}

	pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UDP Server
//

void* udp_server_thread(void* unused_data)
{
	unused_data=unused_data; // Suppress Warnings
	fprintf(stderr, "Starting UDP server on port %d\n", g_server_config.udp_port);

	// Gen some fake data!
	uint32_t count = g_server_config.leds_per_strip*LEDSCAPE_NUM_STRIPS*3;
	uint8_t* data = malloc(count);

	uint8_t offset = 0;
	// for (;;) {
	// 	offset++;

	// 	for (uint32_t i=0; i<count; i+=3) {
	// 		// data[i] = (i + offset) % 64;
	// 		// data[i+1] = (i + offset + 256/3) % 64;
	// 		// data[i+2] = (i + offset + 512/3) % 64;
	// 		data[i] = data[i+1] = data[i+2] = (offset%2==0) ? 0 : 32;
	// 	}

	// 	set_next_frame_data(data, count);
	// 	usleep(1000000);
	// }

	pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TCP Server
static void event_handler(struct ns_connection *conn, enum ns_event ev, void *event_param) {
	struct iobuf *io = &conn->recv_iobuf; // IO buffer that holds received message

	switch (ev) {
		case NS_RECV: {
			// Enough data for an OPC command header?
			if (io->len >= sizeof(opc_cmd_t)) {
				opc_cmd_t* cmd = (opc_cmd_t*) io->buf;
				const size_t cmd_len = cmd->len_hi << 8 | cmd->len_lo;

				uint8_t* opc_cmd_payload = ((uint8_t*)io->buf) + sizeof(opc_cmd_t);

				// Enough data for the entire command?
				if (io->len >= sizeof(opc_cmd_t) + cmd_len) {
					if (cmd->command == 0) {
						set_next_frame_data(opc_cmd_payload, cmd_len, TRUE);
					} else if (cmd->command == 255) {
						// System specific commands
						const uint16_t system_id = opc_cmd_payload[0] << 8 | opc_cmd_payload[1];

						if (system_id == OPC_SYSID_LEDSCAPE) {
							const opc_ledscape_cmd_id_t ledscape_cmd_id = opc_cmd_payload[2];

							 if (ledscape_cmd_id == OPC_LEDSCAPE_CMD_GET_CONFIG) {
								warn("[tcp] Responding to config request\n");
								ns_send(conn, g_server_config.json, strlen(g_server_config.json)+1);
							} else {
								warn("[tcp] WARN: Received command for unsupported LEDscape Command: %d\n", (int)ledscape_cmd_id);
							}
						} else {
							warn("[tcp] WARN: Received command for unsupported system-id: %d\n", (int)system_id);
						}
					}

					// Removed the processed command from the buffer
					iobuf_remove(io, sizeof(opc_cmd_t) + cmd_len);
				}
			}

			// Fallback to handle misformed data. Clear the io buffer if we have more
			// than 100k waiting.
			if (io->len > 1e5) {
				iobuf_remove(io, io->len);
			}
		} break;

		case NS_ACCEPT: {
			char buffer[INET6_ADDRSTRLEN];
			ns_sock_to_str(conn->sock, buffer, sizeof(buffer), 1);
			printf("[tcp] Connection from %s\n", buffer);
		} break;

		default:
		break;    // We ignore all other events
	}
}

void* tcp_server_thread(void* unused_data)
{
	unused_data=unused_data; // Suppress Warnings

	struct ns_server server;
	char s_bind_addr[128];

	pthread_mutex_lock(&g_server_config.mutex);
	sprintf(s_bind_addr, "[::]:%d", g_server_config.tcp_port);
	pthread_mutex_unlock(&g_server_config.mutex);

	// Initialize server and open listening port
	ns_server_init(&server, NULL, event_handler);
	int port = ns_bind(&server, s_bind_addr);
	if (port < 0) {
		printf("Failed to bind to port %s: %d\n", s_bind_addr, port);
		exit(-1);
	}

	printf("[tcp] Starting TCP server on %d\n", port);
	for (;;) {
		ns_server_poll(&server, 1000);
	}
	ns_server_free(&server);
	pthread_exit(NULL);
}
