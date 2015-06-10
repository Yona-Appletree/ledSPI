/** \file
*  OPC image packet receiver.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/mman.h>
#include "util.h"
#include "spio.h"

#include "lib/cesanta/net_skeleton.h"
#include "lib/cesanta/frozen.h"

#include <stdbool.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES, CONSTANTS and UTILS
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#define TRUE 1
#define FALSE 0

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define mint(t, a, b) ((t) (a) < (t) (b) ? (a) : (b))
#define maxt(t, a, b) ((t) (a) > (t) (b) ? (a) : (b))

static const int MAX_CONFIG_FILE_LENGTH_BYTES = 1024*1024*10;
static const uint32_t SPISCAPE_MAX_STRIPS = 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES

typedef enum {
	DEMO_MODE_NONE = 0,
	DEMO_MODE_FADE = 1,
	DEMO_MODE_IDENTIFY = 2,
	DEMO_MODE_BLACK = 3
} demo_mode_t;


typedef struct {
	char spi_dev_path[512];
	uint32_t spi_speed_hz;

	demo_mode_t demo_mode;

	uint16_t tcp_port;
	uint16_t udp_port;
	uint16_t e131_port;

	uint32_t leds_per_strip;
	uint32_t used_strip_count;

	color_channel_order_t color_channel_order;

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
} server_config_t;

char g_config_filename[4096] = {0};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Method declarations
void teardown_server();
void ensure_server_setup();

// Frame Manipulation
void ensure_frame_data();
void set_next_frame_data(uint8_t* frame_data, uint32_t data_size, uint8_t is_remote);
void rotate_frames(uint8_t lock_frame_data);

// Threads
void* render_thread(void* threadarg);
void* udp_server_thread(void* threadarg);
void* tcp_server_thread(void* threadarg);
void* e131_server_thread(void* threadarg);
void* demo_thread(void* threadarg);

// Config Methods
void build_lookup_tables();
int validate_server_config(
	server_config_t* input_config,
	char * result_json_buffer,
	size_t result_json_buffer_size
);

int server_config_from_json(
	const char* json,
	size_t json_size,
	server_config_t* output_config
) ;

void server_config_to_json(char* dest_string, size_t dest_string_size, server_config_t* input_config) ;

const char* demo_mode_to_string(demo_mode_t mode) {
	switch (mode) {
		case DEMO_MODE_NONE: return "none";
		case DEMO_MODE_FADE: return "fade";
		case DEMO_MODE_IDENTIFY: return "id";
		case DEMO_MODE_BLACK: return "black";
		default: return "<invalid demo_mode>";
	}
}

demo_mode_t demo_mode_from_string(const char* str) {
	if (strcasecmp(str, "none") == 0) {
		return DEMO_MODE_NONE;
	} else if (strcasecmp(str, "id") == 0) {
		return DEMO_MODE_IDENTIFY;
	} else if (strcasecmp(str, "fade") == 0) {
		return DEMO_MODE_FADE;
	} else if (strcasecmp(str, "black") == 0) {
		return DEMO_MODE_BLACK;
	} else {
		return -1;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Error Handling

typedef enum {
	OPC_SERVER_ERR_NONE,
	OPC_SERVER_ERR_NO_JSON,
	OPC_SERVER_ERR_INVALID_JSON,
	OPC_SERVER_ERR_FILE_READ_FAILED,
	OPC_SERVER_ERR_FILE_WRITE_FAILED,
	OPC_SERVER_ERR_FILE_TOO_LARGE,
	OPC_SERVER_ERR_SEEK_FAILED
} opc_error_code_t;

__thread opc_error_code_t g_error_code = 0;
__thread char g_error_info_str[4096];


const char* opc_server_strerr(
	opc_error_code_t error_code
) {
	switch (error_code) {
		case OPC_SERVER_ERR_NONE: return "No error";
		case OPC_SERVER_ERR_NO_JSON: return "No JSON document given";
		case OPC_SERVER_ERR_INVALID_JSON: return "Invalid JSON document given";
		default: return "Unkown Error";
	}
}

inline int opc_server_set_error(
	opc_error_code_t error_code,
	const char* extra_info,
	...
) {
	g_error_code = error_code;

	if (extra_info == NULL || strlen(extra_info) == 0) {
		strlcpy(
			g_error_info_str,
			opc_server_strerr(error_code),
			sizeof(g_error_info_str)
		);
	} else {
		char extra_info_out[2048];
		snprintf(
			extra_info_out,
			sizeof(extra_info_out),
			extra_info,
			__builtin_va_arg_pack()
		);
		snprintf(
			extra_info_out,
			sizeof(g_error_info_str),
			"%s: %s",
			opc_server_strerr(error_code),
			extra_info
		);
	}

	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global Data

server_config_t g_server_config = {
	.spi_dev_path = "/dev/spidev0.0",
	.spi_speed_hz = 8000000,

	.demo_mode = DEMO_MODE_FADE,

	.tcp_port = 7890,
	.udp_port = 7890,
	.e131_port = 5568,

	.leds_per_strip = 256,
	.used_strip_count = 1,
	.color_channel_order = COLOR_ORDER_BGR,

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

// Pixel Delta
typedef struct {
	int8_t r;
	int8_t g;
	int8_t b;

	int8_t last_effect_frame_r;
	int8_t last_effect_frame_g;
	int8_t last_effect_frame_b;
} __attribute__((__packed__)) pixel_delta_t;


// Global runtime data
static struct
{
	buffer_pixel_t* previous_frame_data;
	buffer_pixel_t* current_frame_data;
	buffer_pixel_t* next_frame_data;

	pixel_delta_t* frame_dithering_overflow;

	uint8_t* spi_buffer;

	uint8_t has_prev_frame;
	uint8_t has_current_frame;
	uint8_t has_next_frame;

	uint32_t frame_size;
	uint32_t leds_per_strip;

	volatile uint32_t frame_counter;

	struct timeval previous_frame_tv;
	struct timeval current_frame_tv;
	struct timeval next_frame_tv;

	struct timeval prev_current_delta_tv;

	spio_connection * spio_conn;

	uint32_t red_lookup[257];
	uint32_t green_lookup[257];
	uint32_t blue_lookup[257];

	struct timeval last_remote_data_tv;

	pthread_mutex_t mutex;
} g_runtime_state = {
	.previous_frame_data = (buffer_pixel_t*)NULL,
	.current_frame_data = (buffer_pixel_t*)NULL,
	.next_frame_data = (buffer_pixel_t*)NULL,
	.has_prev_frame = FALSE,
	.has_current_frame = FALSE,
	.has_next_frame = FALSE,
	.frame_dithering_overflow = (pixel_delta_t*)NULL,
	.frame_size = 0,
	.leds_per_strip = 0,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.last_remote_data_tv = {
		.tv_sec = 0,
		.tv_usec = 0
	},
	.spio_conn = NULL
};

// Global thread handles
typedef struct {
	pthread_t handle;
	bool enabled;
	bool running;
} thread_state_lt;

static struct
{
	thread_state_lt render_thread;
	thread_state_lt tcp_server_thread;
	thread_state_lt udp_server_thread;
	thread_state_lt e131_server_thread;
	thread_state_lt demo_thread;
} g_threads = {
	{NULL, false, false},
	{NULL, false, false},
	{NULL, false, false},
	{NULL, false, false},
	{NULL, false, false}
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// main()
static struct option long_options[] =
	{
		{"tcp-port", required_argument, NULL, 'p'},
		{"udp-port", required_argument, NULL, 'P'},

		{"e131-port", required_argument, NULL, 'e'},

		{"count", required_argument, NULL, 'c'},
		{"strip-count", required_argument, NULL, 's'},

		{"channel-order", required_argument, NULL, 'o'},

		{"demo-mode", required_argument, NULL, 'D'},

		{"no-interpolation", no_argument, NULL, 'i'},
		{"no-dithering", no_argument, NULL, 't'},
		{"no-lut", no_argument, NULL, 'l'},

		{"help", no_argument, NULL, 'h'},

		{"lum_power", required_argument, NULL, 'L'},

		{"red_bal", required_argument, NULL, 'r'},
		{"green_bal", required_argument, NULL, 'g'},
		{"blue_bal", required_argument, NULL, 'b'},

		{"spi-dev", required_argument, NULL, 'd'},
		{"spi-speed-hz", required_argument, NULL, 'S'},

		{"config", required_argument, NULL, 'C'},

		{NULL, 0, NULL, 0}
	};

void print_usage(char ** argv) {
	printf("Usage: %s ", argv[0]);

	int option_count = sizeof(long_options) / sizeof(struct option);
	for (int option_index = 0; option_index < option_count; option_index++) {
		struct option option_info = long_options[option_index];

		if (option_info.name != NULL) {
			if (option_info.has_arg == required_argument) {
				printf("[--%s <val> | -%c <val>] ", option_info.name, option_info.val);
			} else if (option_info.has_arg == optional_argument) {
				printf("[--%s[=<val>] | -%c[<val>] ", option_info.name, option_info.val);
			} else {
				printf("[--%s | -%c] ", option_info.name, option_info.val);
			}
		}
	}

	printf("\n");
}

int read_config_file(
	const char * config_filename,
	server_config_t* out_config
) {
	// Map the file for reading
	int fd = open(config_filename, O_RDONLY);
	if (fd < 0) {
		return opc_server_set_error(
			OPC_SERVER_ERR_FILE_READ_FAILED,
			"Failed to open config file %s for reading: %s\n",
			config_filename,
			strerror(errno)
		);
	}

	off_t file_end_offset = lseek(fd, 0, SEEK_END);

	if (file_end_offset < 0) {
		return opc_server_set_error(
			OPC_SERVER_ERR_SEEK_FAILED,
			"Failed to seek to end of %s.\n",
			config_filename
		);
	}

	if (file_end_offset > MAX_CONFIG_FILE_LENGTH_BYTES) {
		return opc_server_set_error(
			OPC_SERVER_ERR_FILE_TOO_LARGE,
			"Failed to open config file %s: file is larger than 10MB.\n",
			config_filename
		);
	}

	size_t file_length = (size_t) file_end_offset;

	void *data = mmap(0, file_length, PROT_READ, MAP_PRIVATE, fd, 0);

	// Read the config
	// TODO: Handle character encoding?
	char* str_data = malloc(file_length + 1);
	memcpy(str_data, data, file_length);
	str_data[file_length] = 0;
	server_config_from_json(str_data, strlen(str_data), out_config);
	free(str_data);

	// Unmap the data
	munmap(data, file_length);

	return close(fd);
}

int write_config_file(
	const char* config_filename,
	server_config_t* config
) {
	FILE* fd = fopen(config_filename, "w");
	if (fd == NULL) {
		return opc_server_set_error(
			OPC_SERVER_ERR_FILE_WRITE_FAILED,
			"Failed to open config file %s for reading: %s\n",
			config_filename,
			strerror(errno)
		);
	}

	char json_buffer[4096] = {0};
	server_config_to_json(json_buffer, sizeof(json_buffer), config);
	fputs(json_buffer, fd);

	return fclose(fd);
}

void handle_args(int argc, char ** argv) {
	extern char *optarg;

	int opt;
	while ((opt = getopt_long(argc, argv, "p:P:c:s:d:D:o:ithlL:r:g:b:0:1:m:M:S:", long_options, NULL)) != -1)
	{
		switch (opt)
		{
			case 'p': {
				g_server_config.tcp_port = (uint16_t) atoi(optarg);
			} break;

			case 'P': {
				g_server_config.udp_port = (uint16_t) atoi(optarg);
			} break;

			case 'e': {
				g_server_config.e131_port = (uint16_t) atoi(optarg);
			} break;

			case 'c': {
				g_server_config.leds_per_strip = (uint32_t) atoi(optarg);
			} break;

			case 's': {
				g_server_config.used_strip_count = (uint32_t) atoi(optarg);
			} break;

			case 'D': {
				g_server_config.demo_mode = demo_mode_from_string(optarg);
			} break;


			case 'o': {
				g_server_config.color_channel_order = color_channel_order_from_string(optarg);
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
				g_server_config.lum_power = (float) atof(optarg);
			} break;

			case 'r': {
				g_server_config.white_point.red = (float) atof(optarg);
			} break;

			case 'g': {
				g_server_config.white_point.green = (float) atof(optarg);
			} break;

			case 'b': {
				g_server_config.white_point.blue = (float) atof(optarg);
			} break;

			case 'd': {
				strlcpy(g_server_config.spi_dev_path, optarg, sizeof(g_server_config.spi_dev_path));
			} break;

			case 'S': {
				g_server_config.spi_speed_hz = (uint32_t) atoi(optarg);
			} break;

			case 'C': {
				strlcpy(g_config_filename, optarg, sizeof(g_config_filename));

				if (read_config_file(g_config_filename, &g_server_config) >= 0) {
					fprintf(stderr, "Loaded config file from %s.\n", g_config_filename);
				} else {
					fprintf(stderr, "Config file not loaded: %s\n", g_error_info_str);
				}
			} break;

			case 'h': {
				print_usage(argv);

				printf("\n");

				int option_count = sizeof(long_options) / sizeof(struct option);
				for (int option_index = 0; option_index < option_count; option_index++) {
					struct option option_info = long_options[option_index];
					if (option_info.name != NULL) {
						if (option_info.has_arg == required_argument) {
							printf("--%s <val>, -%c <val>\n\t", option_info.name, option_info.val);
						} else if (option_info.has_arg == optional_argument) {
							printf("--%s[=<val>], -%c[<val>]\n\t", option_info.name, option_info.val);
						} else {
							printf("--%s, -%c\n", option_info.name, option_info.val);
						}

						switch (option_info.val) {
							case 'p': printf("The TCP port to listen for OPC data on"); break;
							case 'P': printf("The UDP port to listen for OPC data on"); break;
							case 'e': printf("The UDP port to listen for e131 data on"); break;
							case 'c': printf("The number of pixels connected to each output channel"); break;
							case 's': printf("The number of used output channels (improves performance by not interpolating/dithering unused channels)"); break;
							case 'd': printf("The SPI device to connect to"); break;
							case 'S': printf("The speed of the SPI device, in hertz"); break;
							case 'D':
								printf("Configures the idle (demo) mode which activates when no data arrives for more than 5 seconds. Modes:\n");
						        printf("\t- none   Do nothing; leaving LED colors as they were\n");
						        printf("\t- black  Turn off all LEDs");
						        printf("\t- fade   Display a rainbow fade across all LEDs\n");
						        printf("\t- id     Send the channel index as all three color values or 0xAA (0b10101010) if channel and pixel index are equal");
						        break;
							case 'o':
								printf("Specifies the color channel output order (RGB, RBG, GRB, GBR, BGR or BRG); default is BRG.");
						        break;
							case 'i': printf("Disables interpolation between frames (choppier output but improves performance)"); break;
							case 't': printf("Disables dithering (choppier output but improves performance)"); break;
							case 'l': printf("Disables luminance correction (lower color values appear brighter than they should)"); break;
							case 'L': printf("Sets the exponent of the luminance power function to the given floating point value (default 2)"); break;
							case 'r': printf("Sets the red balance to the given floating point number (0-1, default .9)"); break;
							case 'g': printf("Sets the red balance to the given floating point number (0-1, default 1)"); break;
							case 'b': printf("Sets the red balance to the given floating point number (0-1, default 1)"); break;
							case '0': printf("[deprecated] Sets the PRU0 program. Use --mode and --mapping instead."); break;
							case '1': printf("[deprecated] Sets the PRU1 program. Use --mode and --mapping instead."); break;
							case 'm':
								printf("Sets the output mode:\n");
						        printf("\t- nop      Disable output; can be useful for debugging\n");
						        printf("\t- ws281x   WS2811/WS2812 output format\n");
						        printf("\t- ws2801   WS2801-compatible 8-bit SPI output. Supports 24 channels of output with pins in a DATA/CLOCK configuration.\n");
						        printf("\t- dmx      DMX compatible output (does not support RDM)\n");
						        break;
							case 'M':
								printf("Sets the pin mapping used:\n");
						        printf("\toriginal-ledspi: Original LedSPI pinmapping. Used on older RGB-123 capes.\n");
						        printf("\trgb-123-v2: RGB-123 mapping for new capes\n");
						        break;
							case 'C':
								printf("Specifies a configuration file to use and creates it if it does not already exist.\n");
						        printf("\tIf used with other options, options are parsed in order. Options before --config are overwritten\n");
						        printf("\tby the config file, and options afterwards will be saved to the config file.\n");
						        break;
							case 'h': printf("Displays this help message"); break;
							default: printf("Undocumented option: %c\n", option_info.val);
						}

						printf("\n");
					}
				}
				printf("\n");
				exit(EXIT_SUCCESS);
			}

			default:
				printf("Invalid option: %c\n\n", opt);
		        print_usage(argv);
		        printf("\nUse -h or --help for more information\n\n");
		        exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char ** argv)
{
	char validation_output_buffer[1024*1024];

	handle_args(argc, argv);

	// Validate the configuration
	if (validate_server_config(
		& g_server_config,
		validation_output_buffer,
		sizeof(validation_output_buffer)
	) != 0) {
		die("ERROR: Configuration failed validation:\n%s",
			validation_output_buffer
		);
	}

	// Save the config file if specified
	if (strlen(g_config_filename) > 0) {
		if (write_config_file(
			g_config_filename,
			&g_server_config
		) >= 0) {
			fprintf(stderr, "Config file written to %s\n", g_config_filename);
		} else {
			fprintf(stderr, "Failed to write to config file %s: %s\n", g_config_filename, g_error_info_str);
		}
	}

	fprintf(stderr,
		"[main] Starting server on ports (tcp=%d, udp=%d) for %d pixels on %d strips\n",
		g_server_config.tcp_port, g_server_config.udp_port, g_server_config.leds_per_strip, SPISCAPE_MAX_STRIPS
	);

	pthread_create(&g_threads.render_thread, NULL, render_thread, NULL);
	pthread_create(&g_threads.udp_server_thread, NULL, udp_server_thread, NULL);
	pthread_create(&g_threads.tcp_server_thread, NULL, tcp_server_thread, NULL);
	pthread_create(&g_threads.e131_server_thread, NULL, e131_server_thread, NULL);

	if (g_server_config.demo_mode != DEMO_MODE_NONE) {
		printf("[main] Demo Mode Enabled\n");
		pthread_create(&g_threads.demo_thread, NULL, demo_thread, NULL);
	} else {
		printf("[main] Demo Mode Disabled\n");
	}

	ensure_server_setup();

	pthread_exit(NULL);
}

const char* build_pruN_program_name(
	const char* output_mode_name,
	const char* output_mapping_name,
	uint8_t pruNum,
	char* out_pru_filename,
	int filename_len
) {
	snprintf(
		out_pru_filename,
		filename_len,
		"pru/bin/%s-%s-pru%d.bin",
		output_mode_name,
		output_mapping_name,
		(int) pruNum
	);

	return out_pru_filename;
}

void ensure_server_setup() {
	printf("[main] Initializing / Updating server...");

	// Setup tables
	build_lookup_tables();
	ensure_frame_data();

	pthread_mutex_lock(&g_runtime_state.mutex);
	pthread_mutex_lock(&g_server_config.mutex);

	// Determine if we need to [re]initialize LedSPI

	bool spi_init_needed = false;

	if (g_runtime_state.spio_conn == NULL) {
		spi_init_needed = true;
	}
	else if (strcasecmp(g_server_config.spi_dev_path, g_runtime_state.spio_conn->device_path) != 0) {
		spi_init_needed = true;
	}

	if (spi_init_needed) {
		if (g_runtime_state.spio_conn != NULL) {
			printf("[main] Closing SPI...");

			spio_close(g_runtime_state.spio_conn);
			g_runtime_state.spio_conn = NULL;
		}

		// Init SPI
		printf("[main] Connecting SPI...");
		g_runtime_state.spio_conn = spio_open(
			g_server_config.spi_dev_path,
		    g_server_config.spi_speed_hz
		);
		printf(" OK at %d hz\n", g_runtime_state.spio_conn->speed_hz);
		g_runtime_state.leds_per_strip = g_server_config.leds_per_strip;
	}

	pthread_mutex_unlock(&g_server_config.mutex);
	pthread_mutex_unlock(&g_runtime_state.mutex);

	// Display server config as JSON
	char json_buffer[4096] = { 0 };
	server_config_to_json(json_buffer, sizeof(json_buffer), &g_server_config);
	fputs(json_buffer, stderr);
}

int validate_server_config(
	server_config_t* input_config,
	char * result_json_buffer,
	size_t result_json_buffer_size
) {
	strlcpy(result_json_buffer, "{\n\t\"errors\": [", result_json_buffer_size);
	char path_temp[4096];

	int error_count = 0;

	inline void result_append(const char *format, ...) {
		snprintf(
			result_json_buffer + strlen(result_json_buffer),
			result_json_buffer_size - strlen(result_json_buffer) + 1,
			format,
			__builtin_va_arg_pack()
		);
	}

	inline void add_error(const char *format, ...) {
		// Can't call result_append here because it breaks gcc:
		// internal compiler error: in initialize_inlined_parameters, at tree-inline.c:2795
		snprintf(
			result_json_buffer + strlen(result_json_buffer),
			result_json_buffer_size - strlen(result_json_buffer) + 1,
			format,
			__builtin_va_arg_pack()
		);
		error_count ++;
	}

	inline void assert_enum_valid(const char *var_name, int value) {
		if (value < 0) {
			add_error(
				"\n\t\t\"" "Invalid %s" "\",",
				var_name
			);
		}
	}

	inline void assert_int_range_inclusive(const char *var_name, int min_val, int max_val, int value) {
		if (value < min_val || value > max_val) {
			add_error(
				"\n\t\t\"" "Given %s (%d) is outside of range %d-%d (inclusive)" "\",",
				var_name,
				value,
				min_val,
				max_val
			);
		}
	}

	inline void assert_double_range_inclusive(const char *var_name, double min_val, double max_val, double value) {
		if (value < min_val || value > max_val) {
			add_error(
				"\n\t\t\"" "Given %s (%f) is outside of range %f-%f (inclusive)" "\",",
				var_name,
				value,
				min_val,
				max_val
			);
		}
	}

	// TODO: Assert SPI dev and speed

	// demoMode
	assert_enum_valid("Demo Mode", input_config->demo_mode);

	// ledsPerStrip
	assert_int_range_inclusive("LED Count", 1, 8096, input_config->leds_per_strip);

	// usedStripCount
	assert_int_range_inclusive("Strip/Channel Count", 1, 1, input_config->used_strip_count);

	// colorChannelOrder
	assert_enum_valid("Color Channel Order", input_config->color_channel_order);

	// opcTcpPort
	assert_int_range_inclusive("OPC TCP Port", 1, 65535, input_config->tcp_port);

	// opcUdpPort
	assert_int_range_inclusive("OPC UDP Port", 1, 65535, input_config->udp_port);

	// e131Port
	assert_int_range_inclusive("e131 UDP Port", 1, 65535, input_config->e131_port);

	// lumCurvePower
	assert_double_range_inclusive("Luminance Curve Power", 0, 10, input_config->lum_power);

	// whitePoint.red
	assert_double_range_inclusive("Red White Point", 0, 1, input_config->white_point.red);

	// whitePoint.green
	assert_double_range_inclusive("Green White Point", 0, 1, input_config->white_point.green);

	// whitePoint.blue
	assert_double_range_inclusive("Blue White Point", 0, 1, input_config->white_point.blue);

	if (error_count > 0) {
		// Strip off trailing comma
		result_json_buffer[strlen(result_json_buffer)-1] = 0;
		result_append("\n\t],\n");
	} else {
		// Reset the output to not include the error messages
		if (result_json_buffer_size > 0) {
			result_json_buffer[0] = 0;
		}
		result_append("{\n");
	}

	// Add closing json
	result_append("\t\"valid\": %s\n", error_count == 0 ? "true" : "false");
	result_append("}");

	return error_count;
}

int server_config_from_json(
	const char* json,
	size_t json_size,
	server_config_t* output_config
) {
	struct json_token *json_tokens;
	const struct json_token *token;
	char token_value[4096];

	if (json_size < 2) {
		// No JSON data
		return opc_server_set_error(
			OPC_SERVER_ERR_NO_JSON,
			NULL
		);
	}

	// Tokenize json string, fill in tokens array
	json_tokens = parse_json2(json, json_size);

	printf("tokens: %d: %s\n", json_size, json);

	if (json_tokens == NULL) {
		// Invalid JSON...
		// TODO: Use parse_json with null token array to figure out where the problem is
		return opc_server_set_error(
			OPC_SERVER_ERR_INVALID_JSON,
			NULL
		);
	}

	// Search for parameter "bar" and print it's value
	if ((token = find_json_token(json_tokens, "spiDevPath"))) {
		strlcpy(output_config->spi_dev_path, token->ptr, mint(int32_t, sizeof(g_server_config.spi_dev_path), token->len + 1));
	}

	if ((token = find_json_token(json_tokens, "spiSpeedHz"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->spi_speed_hz = (uint32_t) atoi(token_value);
	}

	if ((token = find_json_token(json_tokens, "demoMode"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->demo_mode = demo_mode_from_string(token_value);
	}

	if ((token = find_json_token(json_tokens, "ledsPerStrip"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->leds_per_strip = (uint32_t) atoi(token_value);
	}

	if ((token = find_json_token(json_tokens, "usedStripCount"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->used_strip_count = (uint32_t) atoi(token_value);
	}

	if ((token = find_json_token(json_tokens, "colorChannelOrder"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->color_channel_order = color_channel_order_from_string(token_value);
	}

	if ((token = find_json_token(json_tokens, "opcTcpPort"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->tcp_port = (uint16_t) atoi(token_value);
	}

	if ((token = find_json_token(json_tokens, "opcUdpPort"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->udp_port = (uint16_t) atoi(token_value);
	}

	if ((token = find_json_token(json_tokens, "enableInterpolation"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->interpolation_enabled = strcasecmp(token_value, "true") == 0 ? TRUE : FALSE;
	}

	if ((token = find_json_token(json_tokens, "enableDithering"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->dithering_enabled = strcasecmp(token_value, "true") == 0 ? TRUE : FALSE;
	}

	if ((token = find_json_token(json_tokens, "enableLookupTable"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->lut_enabled = strcasecmp(token_value, "true") == 0 ? TRUE : FALSE;
	}

	if ((token = find_json_token(json_tokens, "lumCurvePower"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->lum_power = atof(token_value);
	}

	if ((token = find_json_token(json_tokens, "whitePoint.red"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->white_point.red = atof(token_value);
	}

	if ((token = find_json_token(json_tokens, "whitePoint.green"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->white_point.green = atof(token_value);
	}

	if ((token = find_json_token(json_tokens, "whitePoint.blue"))) {
		strlcpy(token_value, token->ptr, mint(int32_t, sizeof(token_value), token->len + 1));
		output_config->white_point.blue = atof(token_value);
	}

	// Do not forget to free allocated tokens array
	free(json_tokens);

	return 0;
}

void server_config_to_json(char* dest_string, size_t dest_string_size, server_config_t* input_config) {
	// Build config JSON
	snprintf(
		dest_string,
		dest_string_size,

		"{\n"
			"\t" "\"spiDevPath\": \"%s\"," "\n"
			"\t" "\"spiSpeedHz\": \"%d\"," "\n"
			"\t" "\"demoMode\": \"%s\"," "\n"

			"\t" "\"ledsPerStrip\": %d," "\n"
			"\t" "\"usedStripCount\": %d," "\n"
			"\t" "\"colorChannelOrder\": \"%s\"," "\n"

			"\t" "\"opcTcpPort\": %d," "\n"
			"\t" "\"opcUdpPort\": %d," "\n"

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

		input_config->spi_dev_path,
		input_config->spi_speed_hz,

		demo_mode_to_string(input_config->demo_mode),

		input_config->leds_per_strip,
		input_config->used_strip_count,

		color_channel_order_to_string(input_config->color_channel_order),

		input_config->tcp_port,
		input_config->udp_port,

		input_config->interpolation_enabled ? "true" : "false",
		input_config->dithering_enabled ? "true" : "false",
		input_config->lut_enabled ? "true" : "false",

		(double)input_config->lum_power,
		(double)input_config->white_point.red,
		(double)input_config->white_point.green,
		(double)input_config->white_point.blue
	);
}

void build_lookup_tables() {
	pthread_mutex_lock(&g_runtime_state.mutex);
	pthread_mutex_lock(&g_server_config.mutex);

	float white_points[] = {
		g_server_config.white_point.red,
		g_server_config.white_point.green,
		g_server_config.white_point.blue
	};

	uint32_t* lookup_tables[] = {
		g_runtime_state.red_lookup,
		g_runtime_state.green_lookup,
		g_runtime_state.blue_lookup
	};

	for (uint16_t c=0; c<3; c++) {
		for (uint16_t i=0; i<257; i++) {
			double normalI = (double)i / 256;
			normalI *= white_points[c];

			double output = pow(normalI, g_server_config.lum_power);
			int64_t longOutput = (int64_t) ((output * 0xFFFF) + 0.5);
			int32_t clampedOutput = (int32_t) max(0, min(0xFFFF, longOutput));

			lookup_tables[c][i] = (uint32_t) clampedOutput;
		}
	}

	pthread_mutex_unlock(&g_server_config.mutex);
	pthread_mutex_unlock(&g_runtime_state.mutex);
}

/**
* Ensure that the frame buffers are allocated to the correct values.
*/
void ensure_frame_data() {
	pthread_mutex_lock(&g_server_config.mutex);
	uint32_t led_count = (uint32_t)(g_server_config.leds_per_strip) * SPISCAPE_MAX_STRIPS;
	pthread_mutex_unlock(&g_server_config.mutex);

	pthread_mutex_lock(&g_runtime_state.mutex);
	if (g_runtime_state.frame_size != led_count) {
		fprintf(stderr, "Allocating buffers for %d pixels (%lu bytes)\n", led_count, led_count * 3 /*channels*/ * 4 /*buffers*/ * sizeof(uint16_t));

		if (g_runtime_state.previous_frame_data != NULL) {
			free(g_runtime_state.previous_frame_data);
			free(g_runtime_state.current_frame_data);
			free(g_runtime_state.next_frame_data);
			free(g_runtime_state.frame_dithering_overflow);
			free(g_runtime_state.spi_buffer);
		}

		g_runtime_state.frame_size = led_count;
		g_runtime_state.previous_frame_data = malloc(led_count * sizeof(buffer_pixel_t));
		g_runtime_state.current_frame_data = malloc(led_count * sizeof(buffer_pixel_t));
		g_runtime_state.next_frame_data = malloc(led_count * sizeof(buffer_pixel_t));
		g_runtime_state.spi_buffer = malloc(4 + led_count*4 + led_count / 16 + 1);
		g_runtime_state.frame_dithering_overflow = malloc(led_count * sizeof(pixel_delta_t));
		g_runtime_state.has_next_frame = FALSE;
		printf("frame_size1=%u\n", g_runtime_state.frame_size);

		// Init timestamps
		gettimeofday(&g_runtime_state.previous_frame_tv, NULL);
		gettimeofday(&g_runtime_state.current_frame_tv, NULL);
		gettimeofday(&g_runtime_state.next_frame_tv, NULL);
	}
	pthread_mutex_unlock(&g_runtime_state.mutex);
}

/**
* Set the next frame of data to the given 8-bit RGB buffer after rotating the buffers.
*/
void set_next_frame_data(
	uint8_t* frame_data,
	uint32_t data_size,
	uint8_t is_remote
) {
	pthread_mutex_lock(&g_runtime_state.mutex);

	rotate_frames(FALSE);

	// Prevent buffer overruns
	data_size = min(data_size, g_runtime_state.frame_size * 3);

	// Copy in new data
	memcpy(g_runtime_state.next_frame_data, frame_data, data_size);

	// Zero out any pixels not set by the new frame
	memset((uint8_t*) g_runtime_state.next_frame_data + data_size, 0, (g_runtime_state.frame_size*3 - data_size));

	// Update the timestamp & count
	gettimeofday(&g_runtime_state.next_frame_tv, NULL);

	// Update remote data timestamp if applicable
	if (is_remote) {
		gettimeofday(&g_runtime_state.last_remote_data_tv, NULL);
	}

	g_runtime_state.has_next_frame = TRUE;

	pthread_mutex_unlock(&g_runtime_state.mutex);
}

/**
* Rotate the buffers, dropping the previous frame and loading in the new one
*/
void rotate_frames(uint8_t lock_frame_data) {
	if (lock_frame_data) pthread_mutex_lock(&g_runtime_state.mutex);

	buffer_pixel_t* temp = NULL;

	g_runtime_state.has_prev_frame = FALSE;

	if (g_runtime_state.has_current_frame) {
		g_runtime_state.previous_frame_tv = g_runtime_state.current_frame_tv;

		temp = g_runtime_state.previous_frame_data;
		g_runtime_state.previous_frame_data = g_runtime_state.current_frame_data;
		g_runtime_state.current_frame_data = temp;

		g_runtime_state.has_prev_frame = TRUE;
		g_runtime_state.has_current_frame = FALSE;
	}

	if (g_runtime_state.has_next_frame) {
		g_runtime_state.current_frame_tv = g_runtime_state.next_frame_tv;

		temp = g_runtime_state.current_frame_data;
		g_runtime_state.current_frame_data = g_runtime_state.next_frame_data;
		g_runtime_state.next_frame_data = temp;

		g_runtime_state.has_current_frame = TRUE;
		g_runtime_state.has_next_frame = FALSE;
	}

	// Update the delta time stamp
	if (g_runtime_state.has_current_frame && g_runtime_state.has_prev_frame) {
		timersub(
			&g_runtime_state.current_frame_tv,
			&g_runtime_state.previous_frame_tv,
			&g_runtime_state.prev_current_delta_tv
		);
	}

	if (lock_frame_data) pthread_mutex_unlock(&g_runtime_state.mutex);
}

inline uint32_t lutInterpolate(uint32_t value, uint32_t* lut) {
	// Inspired by FadeCandy: https://github.com/scanlime/fadecandy/blob/master/firmware/fc_pixel_lut.cpp

	uint32_t index = value >> 8; // Range [0, 0xFF]
	uint32_t alpha = value & 0xFF; // Range [0, 0xFF]
	uint32_t invAlpha = 0x100 - alpha; // Range [1, 0x100]

	// Result in range [0, 0xFFFF]
	return (lut[index] * invAlpha + lut[index + 1] * alpha) >> 8;
}

void* render_thread(void* unused_data)
{
	unused_data=unused_data; // Suppress Warnings
	fprintf(stderr, "[render] Starting render thread for %u total pixels\n", g_server_config.leds_per_strip *
	                                                                         SPISCAPE_MAX_STRIPS
	);

	// Timing Variables
	struct timeval frame_progress_tv, now_tv;
	uint16_t frame_progress16, inv_frame_progress16;

	const unsigned fps_report_interval_seconds = 10;
	uint64_t last_report = 0;
	uint64_t frame_duration_sum_usec = 0;
	uint32_t frames_since_last_fps_report = 0;
	uint64_t frame_duration_avg_usec = 2000;

	uint8_t buffer_index = 0;
	int8_t ditheringFrame = 0;
	for(;;) {
		pthread_mutex_lock(&g_runtime_state.mutex);

		// Increment the frame counter
		g_runtime_state.frame_counter++;

		// Wait until LedSPI is initialized
		if (g_runtime_state.spio_conn == NULL) {
			printf("[render] Awaiting server initialization...\n");
			pthread_mutex_unlock(&g_runtime_state.mutex);
			usleep(1e6 /* 1s */);
			continue;
		}

		// Skip frames if there isn't enough data
		if (!g_runtime_state.has_prev_frame || !g_runtime_state.has_current_frame) {
			pthread_mutex_unlock(&g_runtime_state.mutex);
			usleep(10e3 /* 10ms */);
			continue;
		}

		// Calculate the time delta and current percentage (as a 16-bit value)
		gettimeofday(&now_tv, NULL);
		timersub(&now_tv, &g_runtime_state.next_frame_tv, &frame_progress_tv);

		// Calculate current frame and previous frame time
		uint64_t frame_progress_us = (uint64_t) (frame_progress_tv.tv_sec*1e6 + frame_progress_tv.tv_usec);
		uint64_t last_frame_time_us = (uint64_t) (g_runtime_state.prev_current_delta_tv.tv_sec*1e6 + g_runtime_state.prev_current_delta_tv.tv_usec);

		// Check for current frame exhaustion
		if (frame_progress_us > last_frame_time_us) {
			uint8_t has_next_frame = g_runtime_state.has_next_frame;
			pthread_mutex_unlock(&g_runtime_state.mutex);

			// This should only happen in a final frame case -- to avoid early switching (and some nasty resulting
			// artifacts) we only force frame rotation if the next frame is really late.
			if (has_next_frame && frame_progress_us > last_frame_time_us*2) {
				// If we have more data, rotate it in.
				//printf("Need data: rotating in; frame_progress_us=%llu; last_frame_time_us=%llu\n", frame_progress_us, last_frame_time_us);
				rotate_frames(TRUE);
			} else {
				// Otherwise sleep for a moment and wait for more data
				//printf("Need data: none available; frame_progress_us=%llu; last_frame_time_us=%llu\n", frame_progress_us, last_frame_time_us);
				usleep(1e3);
			}

			continue;
		}

		frame_progress16 = (uint16_t) ((frame_progress_us << 16) / last_frame_time_us);
		inv_frame_progress16 = (uint16_t) (0xFFFF - frame_progress16);

		if (frame_progress_tv.tv_sec > 5) {
			printf("[render] No data for 5 seconds; suspending render thread.\n");
			pthread_mutex_unlock(&g_runtime_state.mutex);
			usleep(100e3 /* 100ms */);
			continue;
		}

		// printf("%d of %d (%d)\n",
		// 	(frame_progress_tv.tv_sec*1000000 + frame_progress_tv.tv_usec) ,
		// 	(g_runtime_state.prev_current_delta_tv.tv_sec*1000000 + g_runtime_state.prev_current_delta_tv.tv_usec),
		// 	frame_progress16
		// );

		// Setup LedSPI for this frame
		buffer_index = (buffer_index+1)%2;

		// Build the render frame
		uint32_t led_count = g_runtime_state.frame_size;
		uint32_t leds_per_strip = led_count / SPISCAPE_MAX_STRIPS;
		uint32_t data_index = 0;

		// Update the dithering frame counter
		ditheringFrame ++;

		// Timing stuff
		struct timeval start_tv, stop_tv, delta_tv;
		gettimeofday(&start_tv, NULL);

		uint32_t used_strip_count;

		// Check the server config for dithering and interpolation options
		pthread_mutex_lock(&g_server_config.mutex);

		// Use the strip count from configs. This can save time that would be used dithering
		used_strip_count = min(g_server_config.used_strip_count, SPISCAPE_MAX_STRIPS);

		// Only enable dithering if we're better than 100fps
		bool dithering_enabled = (frame_duration_avg_usec < 10000) && g_server_config.dithering_enabled;
		bool interpolation_enabled = g_server_config.interpolation_enabled;
		bool lut_enabled = g_server_config.lut_enabled;

		color_channel_order_t color_channel_order = g_server_config.color_channel_order;

		pthread_mutex_unlock(&g_server_config.mutex);

		// Initialize SPI buffer with start frame (four 0s) and clock padding (pixels/2 or more bits)
		uint8_t* spi_buffer = g_runtime_state.spi_buffer;
		spi_buffer[0] = spi_buffer[1] = spi_buffer[2] = spi_buffer[3] = 0;
		for (int i=0; i<leds_per_strip/16+1; i++)
			spi_buffer[4 + leds_per_strip*4 + i] = 255;

		// Only allow dithering to take effect if it blinks faster than 60fps
		uint32_t maxDitherFrames = 16667 / frame_duration_avg_usec;

		for (uint32_t strip_index=0; strip_index<used_strip_count; strip_index++) {
			for (uint32_t led_index=0; led_index<leds_per_strip; led_index++, data_index++) {
				buffer_pixel_t* pixel_in_prev = &g_runtime_state.previous_frame_data[data_index];
				buffer_pixel_t* pixel_in_current = &g_runtime_state.current_frame_data[data_index];
				pixel_delta_t* pixel_in_overflow = &g_runtime_state.frame_dithering_overflow[data_index];

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
					interpolatedR = lutInterpolate((uint32_t) interpolatedR, g_runtime_state.red_lookup);
					interpolatedG = lutInterpolate((uint32_t) interpolatedG, g_runtime_state.green_lookup);
					interpolatedB = lutInterpolate((uint32_t) interpolatedB, g_runtime_state.blue_lookup);
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
				uint8_t r = (uint8_t) min((ditheredR+0x80) >> 8, 255);
				uint8_t g = (uint8_t) min((ditheredG+0x80) >> 8, 255);
				uint8_t b = (uint8_t) min((ditheredB+0x80) >> 8, 255);


				// TODO: Supprt color ordering properly
				uint8_t* pixel_out = &spi_buffer[4 + led_index*4];
				pixel_out[0] = 255;
				pixel_out[1] = b;
				pixel_out[2] = g;
				pixel_out[3] = r;

//				if (led_index == 0 && strip_index == 3) {
//					printf("channel %d: %03d %03d %03d\n", strip_index, r, g, b);
//				}

				// Check for interpolation effect
				if (r != (interpolatedR+0x80)>>8) pixel_in_overflow->last_effect_frame_r = ditheringFrame;
				if (g != (interpolatedG+0x80)>>8) pixel_in_overflow->last_effect_frame_g = ditheringFrame;
				if (b != (interpolatedB+0x80)>>8) pixel_in_overflow->last_effect_frame_b = ditheringFrame;

				// Recalculate Overflow
				// NOTE: For some strange reason, reading the values from pixel_out causes strange memory corruption. As such
				// we use temporary variables, r, g, and b. It probably has to do with things being loaded into the CPU cache
				// when read, as such, don't read pixel_out from here.
				if (dithering_enabled) {
					pixel_in_overflow->r = (uint8_t) ((int16_t)ditheredR - (r * 257));
					pixel_in_overflow->g = (uint8_t) ((int16_t)ditheredG - (g * 257));
					pixel_in_overflow->b = (uint8_t) ((int16_t)ditheredB - (b * 257));
				}
			}
		}

		// Render the frame
		spio_write(g_runtime_state.spio_conn, spi_buffer, 4 + leds_per_strip*4 + leds_per_strip/16 + 1);

		pthread_mutex_unlock(&g_runtime_state.mutex);

		// Give other threads time... this was found through expirmenting on an Raspberry Pi B+, where the animation
		// was quite choppy without it. Not totally sure of the reason.
		usleep(10);

		// Output Timing Info
		gettimeofday(&stop_tv, NULL);
		timersub(&stop_tv, &start_tv, &delta_tv);

		frames_since_last_fps_report++;
		frame_duration_sum_usec += delta_tv.tv_usec;
		if (stop_tv.tv_sec - last_report >= fps_report_interval_seconds) {
			last_report = stop_tv.tv_sec;

			frame_duration_avg_usec = frame_duration_sum_usec / frames_since_last_fps_report;
			printf("[render] fps_info={frame_avg_usec: %qu, possible_fps: %.2f, actual_fps: %.2f, sample_frames: %u}\n",
				frame_duration_avg_usec,
				(1.0e6 / frame_duration_avg_usec),
				frames_since_last_fps_report * 1.0 / fps_report_interval_seconds,
				frames_since_last_fps_report
			);


			frames_since_last_fps_report = 0;
			frame_duration_sum_usec = 0;
		}
	}

	spio_close(g_runtime_state.spio_conn);
	pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OPC Protocol Structures

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
		OPC_SYSID_LEDSPI = 2
} opc_system_id_t;

typedef enum
{
	OPC_LEDSPI_CMD_GET_CONFIG = 1
} opc_ledspi_cmd_id_t;

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

		out[0] = (uint8_t) r;
		out[1] = (uint8_t) g;
		out[2] = (uint8_t) b;
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

#pragma clang diagnostic ignored "-Wmissing-noreturn"
	for (uint16_t frame_index = 0; /*ever*/; frame_index +=3) {
		// Calculate time since last remote data
		pthread_mutex_lock(&g_runtime_state.mutex);
		gettimeofday(&now_tv, NULL);
		timersub(&now_tv, &g_runtime_state.last_remote_data_tv, &delta_tv);
		pthread_mutex_unlock(&g_runtime_state.mutex);

		pthread_mutex_lock(&g_server_config.mutex);
		uint32_t leds_per_strip = g_server_config.leds_per_strip;
		uint32_t channel_count = g_server_config.leds_per_strip*3* SPISCAPE_MAX_STRIPS;
		demo_mode_t demo_mode = g_server_config.demo_mode;
		pthread_mutex_unlock(&g_server_config.mutex);

		// Enable/disable demo mode and log
		if (delta_tv.tv_sec > 5) {
			if (! demo_enabled) {
				printf("[demo] Starting Demo: %s\n", demo_mode_to_string(demo_mode));
			}

			demo_enabled = TRUE;
		} else {
			if (demo_enabled) {
				printf("[demo] Stopping Demo: %s\n", demo_mode_to_string(demo_mode));
			}

			demo_enabled = FALSE;
		}

		if (demo_enabled) {
			if (buffer_size != channel_count) {
				if (buffer != NULL) free(buffer);
				buffer = malloc(buffer_size = channel_count);
				memset(buffer, 0, buffer_size);
			}

			for (uint32_t strip = 0, data_index = 0 ; strip < SPISCAPE_MAX_STRIPS; strip++)
			{
				for (uint16_t p = 0 ; p < leds_per_strip; p++, data_index+=3)
				{
					switch (demo_mode) {
						case DEMO_MODE_NONE: {
							buffer[data_index] =
								buffer[data_index+1] =
									buffer[data_index+2] = 0;
						} break;

						case DEMO_MODE_IDENTIFY: {
							// Set the pixel to the strip index unless the pixel has the same index as the strip, then
							// light it up grey with bit value: 1010 1010
							buffer[data_index] =
								buffer[data_index+1] =
									buffer[data_index+2] =
										(uint8_t) ((strip == p) ? 170 : strip);
						} break;

						case DEMO_MODE_FADE: {
							int baseBrightness = 196;
							int brightnessChange = 128;
							HSBtoRGB(
								((frame_index + ((p + strip*leds_per_strip)*360)/(leds_per_strip*10)) % 360),
								255,
								baseBrightness - (((frame_index /10) + (p*brightnessChange)/leds_per_strip + strip*10) % brightnessChange),
								&buffer[data_index]
							);
						} break;

						case DEMO_MODE_BLACK: {
							buffer[data_index] = buffer[data_index+1] = buffer[data_index+2] = 0;
						} break;
					}
				}
			}

			set_next_frame_data(buffer, buffer_size, FALSE);
		}

		usleep(1e6/30);
	}
#pragma clang diagnostic pop

	pthread_exit(NULL);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// e131 Server
//

// From http://atastypixel-blog-content.s3.amazonaws.com/blog/wp-content/uploads/2010/05/multicast_sample.c
int join_multicast_group_on_all_ifaces(
	const int sock_fd,
	const char* group_ip
) {
	// Obtain list of all network interfaces
	struct ifaddrs *addrs;

	if ( getifaddrs(&addrs) < 0 ) {
		// Error occurred
		return -1;
	}

	// Loop through interfaces, selecting those AF_INET devices that support multicast, but aren't loopback or point-to-point
	const struct ifaddrs *cursor = addrs;
	int joined_count = 0;
	while ( cursor != NULL ) {
		if ( cursor->ifa_addr->sa_family == AF_INET
			&& !(cursor->ifa_flags & IFF_LOOPBACK)
			&& !(cursor->ifa_flags & IFF_POINTOPOINT)
			&&	(cursor->ifa_flags & IFF_MULTICAST)
			) {
			// Prepare multicast group join request
			struct ip_mreq multicast_req;
			memset(&multicast_req, 0, sizeof(multicast_req));
			multicast_req.imr_multiaddr.s_addr = inet_addr(group_ip);
			multicast_req.imr_interface = ((struct sockaddr_in *)cursor->ifa_addr)->sin_addr;

			// Workaround for some odd join behaviour: It's perfectly legal to join the same group on more than one interface,
			// and up to 20 memberships may be added to the same socket (see ip(4)), but for some reason, OS X spews
			// 'Address already in use' errors when we actually attempt it.	As a workaround, we can 'drop' the membership
			// first, which would normally have no effect, as we have not yet joined on this interface.	However, it enables
			// us to perform the subsequent join, without dropping prior memberships.
			setsockopt(sock_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &multicast_req, sizeof(multicast_req));

			// Join multicast group on this interface
			if ( setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_req, sizeof(multicast_req)) >= 0 ) {
				printf("[e131] Joined multicast group %s on %s\n", group_ip, inet_ntoa(((struct sockaddr_in *)cursor->ifa_addr)->sin_addr));
				joined_count ++;
			} else {
				// Error occurred
				freeifaddrs(addrs);
				return -1;
			}
		}

		cursor = cursor->ifa_next;
	}

	freeifaddrs(addrs);

	return joined_count;
}

void* e131_server_thread(void* unused_data)
{
	unused_data=unused_data; // Suppress Warnings

	// Disable if given port 0
	if (g_server_config.e131_port == 0) {
		fprintf(stderr, "[e131] Not starting e131 server; Port is zero.\n");
		pthread_exit(NULL);
		return NULL;
	}

	fprintf(stderr, "[e131] Starting UDP server on port %d\n", g_server_config.e131_port);
	uint8_t packet_buffer[65536]; // e131 packet buffer

	const int sock = socket(AF_INET6, SOCK_DGRAM, 0);

	if (sock < 0)
		die("[e131] socket failed: %s\n", strerror(errno));

	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = in6addr_any,
		.sin6_port = htons(g_server_config.e131_port),
	};

	if (bind(sock, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
		fprintf(stderr, "[e131] bind port %d failed: %s\n", g_server_config.e131_port, strerror(errno));
		pthread_exit(NULL);
		return NULL;
	}

	int32_t last_seq_num = -1;

	// Bind to multicast
	if (join_multicast_group_on_all_ifaces(sock, "239.255.0.0") < 0) {
		fprintf(stderr, "[e131] failed to bind to multicast addresses\n");
	}

	uint8_t* dmx_buffer = NULL;
	uint32_t dmx_buffer_size = 0;

	uint32_t packets_since_update = 0;
	uint32_t frame_counter_at_last_update = g_runtime_state.frame_counter;

	while (1)
	{
		const ssize_t received_packet_size = recv(sock, packet_buffer, sizeof(packet_buffer), 0);
		if (received_packet_size < 0) {
			fprintf(stderr, "[e131] recv failed: %s\n", strerror(errno));
			continue;
		}

		// Ensure the buffer
		pthread_mutex_lock(&g_server_config.mutex);
		uint32_t leds_per_strip = g_server_config.leds_per_strip;
		uint32_t led_count = g_server_config.leds_per_strip * SPISCAPE_MAX_STRIPS;
		pthread_mutex_unlock(&g_server_config.mutex);

		if (dmx_buffer == NULL || dmx_buffer_size != led_count) {
			if (dmx_buffer != NULL) free(dmx_buffer);
			dmx_buffer_size = led_count * sizeof(buffer_pixel_t);
			dmx_buffer = malloc(dmx_buffer_size);
		}

		// Packet should be at least 126 bytes for the header
		if (received_packet_size >= 126) {
			int32_t current_seq_num = packet_buffer[111];

			if (last_seq_num == -1 || current_seq_num >= last_seq_num || (last_seq_num - current_seq_num) > 64) {
				last_seq_num = current_seq_num;

				// 1-based DMX universe
				uint16_t dmx_universe_num = ((uint16_t)packet_buffer[113] << 8) | packet_buffer[114];

				if (dmx_universe_num >= 1 && dmx_universe_num <= 48) {
					uint16_t ledspi_channel_num = dmx_universe_num - 1;
					// Data OK
//					set_next_frame_single_channel_data(
//						ledspi_channel_num,
//						packet_buffer + 126,
//						received_packet_size - 126,
//						TRUE
//					);

					memcpy(
						dmx_buffer + ledspi_channel_num * leds_per_strip * sizeof(buffer_pixel_t),
						packet_buffer + 126,
						min(received_packet_size - 126, led_count * sizeof(buffer_pixel_t))
					);

					set_next_frame_data(
						dmx_buffer,
						dmx_buffer_size * sizeof(buffer_pixel_t),
						TRUE
					);
				} else {
					fprintf(
						stderr,
						"[e131] DMX universe %d out of bounds [1,48] \n",
						dmx_universe_num
					);
				}
			} else {
				// Out of order sequence packet
				fprintf(stderr, "[e131] out of order packet; current %d, old %d \n", current_seq_num, last_seq_num);
			}
		} else {
			fprintf(stderr, "[e131] packet too small: %d < 126 \n", received_packet_size);
		}

		// Increment counter
		packets_since_update ++;
		if (g_runtime_state.frame_counter != frame_counter_at_last_update) {
			packets_since_update = 0;
			frame_counter_at_last_update = g_runtime_state.frame_counter;
		}

		if (packets_since_update >= SPISCAPE_MAX_STRIPS) {
			// Force an update here
			while (g_runtime_state.frame_counter == frame_counter_at_last_update)
				usleep(1e3 /* 1ms */);
		}
	}

	pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UDP Server
//

void* udp_server_thread(void* unused_data)
{
	unused_data=unused_data; // Suppress Warnings

	// Disable if given port 0
	if (g_server_config.udp_port == 0) {
		fprintf(stderr, "[udp] Not starting UDP server; Port is zero.\n");
		pthread_exit(NULL);
		return NULL;
	}

	uint32_t required_packet_size = g_server_config.used_strip_count * g_server_config.leds_per_strip * 3 + sizeof(opc_cmd_t);
	if (required_packet_size > 65507) {
		fprintf(stderr,
			"[udp] OPC command for %d LEDs cannot fit in UDP packet. Use --count or --strip-count to reduce the number of required LEDs, or disable UDP server with --udp-port 0\n",
			g_server_config.used_strip_count * g_server_config.leds_per_strip
		);
		pthread_exit(NULL);
		return NULL;
	}

	fprintf(stderr, "[udp] Starting UDP server on port %d\n", g_server_config.udp_port);
	uint8_t buf[65536];

	const int sock = socket(AF_INET6, SOCK_DGRAM, 0);

	if (sock < 0)
		die("[udp] socket failed: %s\n", strerror(errno));

	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = in6addr_any,
		.sin6_port = htons(g_server_config.udp_port),
	};

	if (bind(sock, (const struct sockaddr*) &addr, sizeof(addr)) < 0)
		die("[udp] bind port %d failed: %s\n", g_server_config.udp_port, strerror(errno));

	while (1)
	{
		const ssize_t rc = recv(sock, buf, sizeof(buf), 0);
		if (rc < 0) {
			fprintf(stderr, "[udp] recv failed: %s\n", strerror(errno));
			continue;
		}

		// Enough data for an OPC command header?
		if (rc >= sizeof(opc_cmd_t)) {
			opc_cmd_t* cmd = (opc_cmd_t*) buf;
			const size_t cmd_len = cmd->len_hi << 8 | cmd->len_lo;

			uint8_t* opc_cmd_payload = ((uint8_t*)buf) + sizeof(opc_cmd_t);

			// Enough data for the entire command?
			if (rc >= sizeof(opc_cmd_t) + cmd_len) {
				if (cmd->command == 0) {
					set_next_frame_data(opc_cmd_payload, cmd_len, TRUE);
				} else if (cmd->command == 255) {
					// System specific commands
					const uint16_t system_id = opc_cmd_payload[0] << 8 | opc_cmd_payload[1];

					if (system_id == OPC_SYSID_LEDSPI) {
						const opc_ledspi_cmd_id_t ledspi_cmd_id = opc_cmd_payload[2];

						if (ledspi_cmd_id == OPC_LEDSPI_CMD_GET_CONFIG) {
							warn("[udp] WARN: Config request request received but not supported on UDP.\n");
						} else {
							warn("[udp] WARN: Received command for unsupported LedSPI Command: %d\n", (int)ledspi_cmd_id);
						}
					} else {
						warn("[udp] WARN: Received command for unsupported system-id: %d\n", (int)system_id);
					}
				}
			}
		}
	}

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

						if (system_id == OPC_SYSID_LEDSPI) {
							const opc_ledspi_cmd_id_t ledspi_cmd_id = opc_cmd_payload[2];

							if (ledspi_cmd_id == OPC_LEDSPI_CMD_GET_CONFIG) {
								warn("[tcp] Responding to config request\n");
								ns_send(conn, g_server_config.json, strlen(g_server_config.json)+1);
							} else {
								warn("[tcp] WARN: Received command for unsupported LedSPI Command: %d\n", (int)ledspi_cmd_id);
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

	// Disable if given port 0
	if (g_server_config.tcp_port == 0) {
		fprintf(stderr, "[tcp] Not starting TCP server; Port is zero.\n");
		pthread_exit(NULL);
		return NULL;
	}

	struct ns_server server;
	char s_bind_addr[128];

	pthread_mutex_lock(&g_server_config.mutex);
	sprintf(s_bind_addr, "[::]:%d", g_server_config.tcp_port);
	pthread_mutex_unlock(&g_server_config.mutex);

	// Initialize server and open listening port
	ns_server_init(&server, NULL, event_handler);
	int port = ns_bind(&server, s_bind_addr);
	if (port < 0) {
		printf("[tcp] Failed to bind to port %s: %d\n", s_bind_addr, port);
		exit(-1);
	}

	printf("[tcp] Starting TCP server on %d\n", port);
	for (;;) {
		ns_server_poll(&server, 1000);
	}
	ns_server_free(&server);
	pthread_exit(NULL);
}

#pragma clang diagnostic pop
#pragma clang diagnostic pop