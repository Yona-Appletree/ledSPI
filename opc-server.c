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
#include "util.h"
#include "ledscape.h"

#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
#define TRUE 1
#define FALSE 0

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Method declarations

// Frame Manipulation
void ensure_frame_data();
void set_next_frame_data(uint8_t* frame_data, uint32_t data_size);
void rotate_frames();

// Threads
void* render_thread(void* threadarg);
void* udp_server_thread(void* threadarg);
void* tcp_server_thread(void* threadarg);

// Helper methods
void init_lookup_tables();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global Data
static struct {
	uint16_t port;
	uint16_t leds_per_strip;

	uint8_t interpolation_enabled;
	uint8_t dithering_enabled;

	uint16_t redLookup[257];

	uint16_t greenLookup[257];

	uint16_t blueLookup[257];

	pthread_mutex_t mutex;
} g_server_config = {
	7890,
	176,
	TRUE,
	TRUE,
	{},
	{},
	{},
	PTHREAD_MUTEX_INITIALIZER
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

	int8_t lastEffectFrame;
} __attribute__((__packed__)) pixel_delta_t;

static struct
{
	buffer_pixel_t* previous_frame_data;
	buffer_pixel_t* current_frame_data;
	buffer_pixel_t* next_frame_data;

	pixel_delta_t* frame_dithering_overflow;

	uint32_t frame_size;

	uint64_t frame_count;

	struct timeval previous_frame_tv;
	struct timeval current_frame_tv;
	struct timeval next_frame_tv;

	struct timeval prev_current_delta_tv;

	pthread_mutex_t mutex;
} g_frame_data = {
	.previous_frame_data = (buffer_pixel_t*)NULL,
	.current_frame_data = (buffer_pixel_t*)NULL,
	.next_frame_data = (buffer_pixel_t*)NULL,
	.frame_dithering_overflow = (buffer_pixel_t*)NULL,
	.frame_size = 0,
	.frame_count = 0,
	.mutex = PTHREAD_MUTEX_INITIALIZER
};

static struct
{
	pthread_t render_thread;
	pthread_t tcp_server_thread;
	pthread_t udp_server_thread;
} g_threads;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// main()
int main(int argc, char ** argv)
{
	extern char *optarg;
	int opt;
	while ((opt = getopt(argc, argv, "p:c:d:")) != -1)
	{
		switch (opt)
		{
		case 'p':
			g_server_config.port = atoi(optarg);
			break;
		case 'c':
			g_server_config.leds_per_strip = atoi(optarg);
			break;
		case 'd': {
			int width=0, height=0;

			if (sscanf(optarg,"%dx%d", &width, &height) == 2) {
				g_server_config.leds_per_strip = width * height;
			} else {
				printf("Invalid argument for -d; expected NxN; actual: %s", optarg);
				exit(EXIT_FAILURE);
			}
		}
		break;
		default:
			fprintf(stderr, "Usage: %s [-p <port>] [-c <led_count> | -d <width>x<height>]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// largest possible UDP packet
	if (g_server_config.leds_per_strip*LEDSCAPE_NUM_STRIPS*3 >= 65536) {
		die("%u pixels cannot fit in a UDP packet.\n", g_server_config.leds_per_strip);
	}

	fprintf(stderr, "Starting server on port %d for %d pixels on %d strips\n", g_server_config.port, g_server_config.leds_per_strip, LEDSCAPE_NUM_STRIPS);

	init_lookup_tables();

	ensure_frame_data();

	pthread_create(&g_threads.render_thread, NULL, render_thread, NULL);
	pthread_create(&g_threads.udp_server_thread, NULL, udp_server_thread, NULL);
	pthread_create(&g_threads.tcp_server_thread, NULL, tcp_server_thread, NULL);

	pthread_exit(NULL);
}

void init_lookup_tables() {
	for (uint16_t i=0; i<256; i++) {
		uint16_t value = (uint16_t) ((double)65536*pow((double)i/256, 2));
		//printf("%u = %u\n", i, value);
		g_server_config.redLookup[i] = g_server_config.greenLookup[i] = g_server_config.blueLookup[i] = value;
	}

	g_server_config.redLookup[256] = g_server_config.greenLookup[256] = g_server_config.blueLookup[256] = 0xFFFF;
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
		g_frame_data.frame_count = 0;
		printf("frame_size1=%u\n", g_frame_data.frame_size);
	}
	pthread_mutex_unlock(&g_frame_data.mutex);
}

/**
 * Set the next frame of data to the given 8-bit RGB buffer after rotating the buffers.
 */
void set_next_frame_data(uint8_t* frame_data, uint32_t data_size) {
	rotate_frames();

	pthread_mutex_lock(&g_frame_data.mutex);

	// Prevent buffer overruns
	data_size = min(data_size, g_frame_data.frame_size * 3);

	memcpy(g_frame_data.next_frame_data, frame_data, data_size);
	memset((uint8_t*)g_frame_data.next_frame_data + data_size, 0, (g_frame_data.frame_size*3 - data_size));

	// Update the timestamp & count
	gettimeofday(&g_frame_data.next_frame_tv, NULL);
	g_frame_data.frame_count ++;

	pthread_mutex_unlock(&g_frame_data.mutex);
}

/**
 * Rotate the buffers, dropping the previous frame and loading in the new one
 */
void rotate_frames() {
	pthread_mutex_lock(&g_frame_data.mutex);

	// Update timestamps
	g_frame_data.previous_frame_tv = g_frame_data.current_frame_tv;
	g_frame_data.current_frame_tv = g_frame_data.next_frame_tv;

	// Copy data
	buffer_pixel_t* temp = g_frame_data.previous_frame_data;
	g_frame_data.previous_frame_data = g_frame_data.current_frame_data;
	g_frame_data.current_frame_data = g_frame_data.next_frame_data;
	g_frame_data.next_frame_data = temp;

	// Update the delta time stamp
	timersub(&g_frame_data.current_frame_tv, &g_frame_data.previous_frame_tv, &g_frame_data.prev_current_delta_tv);

	pthread_mutex_unlock(&g_frame_data.mutex);
}

inline uint16_t lutInterpolate(uint16_t value, uint16_t* lut) {
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
	fprintf(stderr, "Starting render thread for %u total pixels\n", g_server_config.leds_per_strip * LEDSCAPE_NUM_STRIPS);

	// Timing Variables
	struct timeval frame_progress_tv, now_tv;
	uint16_t frame_progress16, inv_frame_progress16;

	// Init LEDscape
	ledscape_t * const leds = ledscape_init(g_server_config.leds_per_strip);

	const unsigned report_interval = 1;
	unsigned last_report = 0;
	unsigned long delta_sum = 0;
	unsigned frames = 0;
	uint32_t delta_avg = 2000;


	uint8_t buffer_index = 0;
	int8_t ditheringFrame = 0;
	for(;;) {
		// Skip frames if there isn't enough data
		if (g_frame_data.frame_count < 3) {
			usleep(10000);
			continue;
		}

		pthread_mutex_lock(&g_frame_data.mutex);

		// Calculate the time delta and current percentage (as a 16-bit value)
		gettimeofday(&now_tv, NULL);
		timersub(&now_tv, &g_frame_data.next_frame_tv, &frame_progress_tv);

		frame_progress16 = (((uint64_t)frame_progress_tv.tv_sec*1000000 + frame_progress_tv.tv_usec) << 16) / (g_frame_data.prev_current_delta_tv.tv_sec*1000000 + g_frame_data.prev_current_delta_tv.tv_usec);
		inv_frame_progress16 = 0x10000 - frame_progress16;

		// printf("%d of %d (%d)\n",
		// 	(frame_progress_tv.tv_sec*1000000 + frame_progress_tv.tv_usec) ,
		// 	(g_frame_data.prev_current_delta_tv.tv_sec*1000000 + g_frame_data.prev_current_delta_tv.tv_usec),
		// 	frame_progress16
		// );

		// Setup LEDscape for this frame
		buffer_index = (buffer_index+1)%2;
		ledscape_frame_t * const frame = ledscape_frame(leds, buffer_index);

		// Build the render frame
		uint16_t led_count = g_frame_data.frame_size;
		uint16_t leds_per_strip = led_count / LEDSCAPE_NUM_STRIPS;
		uint32_t data_index = 0;

		// Update the dithering frame counter
		ditheringFrame ++;

		// Timing stuff
		struct timeval start_tv, stop_tv, delta_tv;
		gettimeofday(&start_tv, NULL);

		// Only enable dithering if we're better than 100fps
		uint8_t ditheringEnabled = delta_avg < 10000;

		// Only allow dithering to take effect if it blinks faster than 60fps
		uint32_t maxDitherFrames = 16666 / delta_avg;

		for (uint32_t strip_index=0; strip_index<LEDSCAPE_NUM_STRIPS; strip_index++) {
			for (uint32_t led_index=0; led_index<leds_per_strip; led_index++, data_index++) {
				buffer_pixel_t* pixel_in_prev = &g_frame_data.previous_frame_data[data_index];
				buffer_pixel_t* pixel_in_current = &g_frame_data.current_frame_data[data_index];
				pixel_delta_t* pixel_in_overflow = &g_frame_data.frame_dithering_overflow[data_index];

				ledscape_pixel_t* const pixel_out = & frame[led_index].strip[strip_index];

				// Interpolate
				int32_t interpolatedR = (pixel_in_prev->r*inv_frame_progress16 + pixel_in_current->r*frame_progress16) >> 8;
				int32_t interpolatedG = (pixel_in_prev->g*inv_frame_progress16 + pixel_in_current->g*frame_progress16) >> 8;
				int32_t interpolatedB = (pixel_in_prev->b*inv_frame_progress16 + pixel_in_current->b*frame_progress16) >> 8;

				// Apply LUT
				interpolatedR = lutInterpolate(interpolatedR, g_server_config.redLookup);
				interpolatedG = lutInterpolate(interpolatedG, g_server_config.greenLookup);
				interpolatedB = lutInterpolate(interpolatedB, g_server_config.blueLookup);

				// if (data_index == 0) {
				// 	printf("%d\n", abs(abs(pixel_in_overflow->lastEffectFrame) - abs(ditheringFrame)));
				// }

				// Reset the dithering if it's been too long
				if (abs(abs(pixel_in_overflow->lastEffectFrame) - abs(ditheringFrame)) > maxDitherFrames) {
					pixel_in_overflow->r = pixel_in_overflow->g = pixel_in_overflow->b = 0;
					pixel_in_overflow->lastEffectFrame = ditheringFrame;
				}

				// Apply dithering overflow
				int32_t	ditheredR = interpolatedR;
				int32_t	ditheredG = interpolatedG;
				int32_t	ditheredB = interpolatedB;

				if (ditheringEnabled) {
					ditheredR += pixel_in_overflow->r;
					ditheredG += pixel_in_overflow->g;
					ditheredB += pixel_in_overflow->b;
				}

				// Calculate and assign output values
				uint8_t r = pixel_out->r = min((ditheredR+0x80) >> 8, 255);
				uint8_t g = pixel_out->g = min((ditheredG+0x80) >> 8, 255);
				uint8_t b = pixel_out->b = min((ditheredB+0x80) >> 8, 255);

				// Check for interpolation effect
				if (r != (interpolatedR+0x80)>>8 || g != (interpolatedG+0x80)>>8 || b != (interpolatedB+0x80)>>8) {
						pixel_in_overflow->lastEffectFrame = ditheringFrame;
						//if (data_index == 0) printf("overflow used\n");
				}

				// Recalculate Overflow
				// NOTE: For some strange reason, reading the values from pixel_out causes strange memory corruption. As such
				// we use temporary variables, r, g, and b. It probably has to do with things being loaded into the CPU cache
				// when read, as such, don't read pixel_out from here.
				pixel_in_overflow->r = (int16_t)interpolatedR - (r * 257);
				pixel_in_overflow->g = (int16_t)interpolatedG - (g * 257);
				pixel_in_overflow->b = (int16_t)interpolatedB - (b * 257);
			}
		}

		pthread_mutex_unlock(&g_frame_data.mutex);

		// Render the frame
		ledscape_wait(leds);
		ledscape_draw(leds, buffer_index);

		// Output Timing Info
		gettimeofday(&stop_tv, NULL);
		timersub(&stop_tv, &start_tv, &delta_tv);

		frames++;
		delta_sum += delta_tv.tv_usec;
		if (stop_tv.tv_sec - last_report < report_interval)
			continue;
		last_report = stop_tv.tv_sec;

		delta_avg = delta_sum / frames;
		printf("%6u usec avg, max %.2f fps, actual %.2f fps (over %u frames)\n",
			delta_avg,
			report_interval * 1.0e6 / delta_avg,
			frames * 1.0 / report_interval,
			frames
		);

		frames = delta_sum = 0;
	}

	ledscape_close(leds);
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
	OPC_LEDSCAPE_CMD_SET_CONFIG = 0
} opc_ledscape_cmd_id_t;

typedef struct
{
	uint8_t len_hi;
	uint8_t len_lo;

	uint8_t pru0_mode;
	uint8_t pru1_mode;
} opc_ledscape_set_config_t;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UDP Server

void* udp_server_thread(void* unused_data)
{
	unused_data=unused_data; // Suppress Warnings
	fprintf(stderr, "Starting UDP server on port %d\n", g_server_config.port);

	// Gen some fake data!
	uint32_t count = g_server_config.leds_per_strip*LEDSCAPE_NUM_STRIPS*3;
	uint8_t* data = malloc(count);

	uint8_t offset = 0;
	for (;;) {
		offset++;

		for (uint32_t i=0; i<count; i+=3) {
			// data[i] = (i + offset) % 64;
			// data[i+1] = (i + offset + 256/3) % 64;
			// data[i+2] = (i + offset + 512/3) % 64;
			data[i] = data[i+1] = data[i+2] = (offset%2==0) ? 0 : 32;
		}

		set_next_frame_data(data, count);
		usleep(1000000);
	}

	pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TCP Server
static int
tcp_socket(
	const int port
)
{
	const int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY,
	};

	if (sock < 0)
		return -1;
	if (bind(sock, (const struct sockaddr*) &addr, sizeof(addr)) < 0)
		return -1;
	if (listen(sock, 5) == -1)
		return -1;

	return sock;
}

void* tcp_server_thread(void* unused_data)
{
	// unused_data=unused_data; // Suppress Warnings
	// fprintf(stderr, "Starting TCP server on port %d\n", g_server_config.port);

	// const int sock = tcp_socket(g_server_config.port);
	// if (sock < 0)
	// 	die("socket port %d failed: %s\n", port, strerror(errno));

	// uint8_t buf[65536];

	// while ((fd = accept(sock, NULL, NULL)) >= 0)
	// {
	// 	printf("Client connected!");

	// 	while(1)
	// 	{
	// 		opc_cmd_t cmd;
	// 		ssize_t rlen = read(fd, &cmd, sizeof(cmd));
	// 		if (rlen < 0)
	// 			die("recv failed: %s\n", strerror(errno));
	// 		if (rlen == 0)
	// 		{
	// 			close(fd);
	// 			break;
	// 		}

	// 		const size_t cmd_len = cmd.len_hi << 8 | cmd.len_lo;

	// 		//warn("cmd=%d; size=%zu\n", cmd.command, cmd_len);

	// 		size_t offset = 0;
	// 		while (offset < cmd_len)
	// 		{
	// 			rlen = read(fd, buf + offset, cmd_len - offset);
	// 			if (rlen < 0)
	// 				die("recv failed: %s\n", strerror(errno));
	// 			if (rlen == 0)
	// 				break;
	// 			offset += rlen;
	// 		}

	// 		if (cmd.command == 0) {
	// 			set_next_frame_data(buf, cmd_len);
	// 		} else if (cmd.command == 255) {
	// 			// System specific commands
	// 			const uint16_t system_id = buf[0] << 8 | buf[1];

	// 			if (system_id == OPC_SYSID_LEDSCAPE) {
	// 				const opc_ledscape_cmd_id_t ledscape_cmd_id = buf[2];

	// 				if (ledscape_cmd_id == OPC_LEDSCAPE_CMD_SET_CONFIG) {
	// 					opc_ledscape_set_config_t* config_cmd = &buf[3];
	// 					led_count = config_cmd->len_hi << 8 | config_cmd->len_lo;

	// 					warn("Received config update request: (led_count=%d, pru0_mode=%d, pru1_mode=%d)", led_count, config_cmd->pru0_mode, config_cmd->pru1_mode);

	// 					// TODO: Implement configuration updating
	// 				} else {
	// 					warn("WARN: Received command for unsupported LEDscape Command: %d", (int)ledscape_cmd_id);
	// 				}
	// 			} else {
	// 				warn("WARN: Received command for unsupported system-id: %d", (int)system_id);
	// 			}
		// 	}
		// }
//	}

	pthread_exit(NULL);
}
