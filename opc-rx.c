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
#include <pthread.h>
#include "util.h"
#include "ledscape.h"

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
	OPC_LEDSCAPE_CMD_SET_CONFIG = 0,
	OPC_LEDSCAPE_CMD_GET_CONFIG = 1
} opc_ledscape_cmd_id_t;

typedef struct
{
	uint8_t len_hi;
	uint8_t len_lo;

	uint8_t pru0_mode;
	uint8_t pru1_mode;
} opc_ledscape_config_t;


static int
tcp_socket(
	const int port
)
{
	const int sock = socket(AF_INET6, SOCK_STREAM, 0);
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(port),
		.sin6_addr = in6addr_any,
	};

	if (sock < 0)
		return -1;
	if (bind(sock, (const struct sockaddr*) &addr, sizeof(addr)) < 0)
		return -1;
	if (listen(sock, 5) == -1)
		return -1;

	return sock;
}

int
main(
	int argc,
	char ** argv
)
{
	uint16_t port = 7890;
	uint16_t led_count = 256;

	extern char *optarg;
	int opt;
	while ((opt = getopt(argc, argv, "p:c:d:")) != -1)
	{
		switch (opt)
		{
		case 'p':
			port = atoi(optarg);
			break;

		case 'c':
			led_count = atoi(optarg);
			break;

		case 'd': {
			int width=0, height=0;

			if (sscanf(optarg,"%dx%d", &width, &height) == 2) {
				led_count = width * height;
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

	const int sock = tcp_socket(port);
	if (sock < 0)
		die("socket port %d failed: %s\n", port, strerror(errno));

	const size_t image_size = led_count * 3;

	// largest possible UDP packet
	uint8_t buf[65536];
	if (sizeof(buf) < image_size + 1)
		die("%u too large for UDP\n", image_size);

	fprintf(stderr, "OpenPixelControl LEDScape Receiver started on TCP port %d for %d pixels on %d strips totalling %d pixels.\n", port, led_count, LEDSCAPE_NUM_STRIPS, led_count*LEDSCAPE_NUM_STRIPS);
	fprintf(stderr, "NOTE: This server can only accept a single concurrent OPC client connection.\n");

	ledscape_t * const leds = ledscape_init(led_count);

	const uint32_t report_interval = 1;
	uint32_t last_report = 0;
	uint64_t delta_sum = 0;
	uint32_t frames = 0;

	int fd;
	uint32_t frame_num = 0;
	while ((fd = accept(sock, NULL, NULL)) >= 0)
	{
		printf("Client connected!");
		while(1)
		{
			opc_cmd_t cmd;
			ssize_t rlen = read(fd, &cmd, sizeof(cmd));
			if (rlen < 0)
				die("recv failed: %s\n", strerror(errno));
			if (rlen == 0)
			{
				close(fd);
				break;
			}

			const size_t cmd_len = cmd.len_hi << 8 | cmd.len_lo;

			//warn("cmd=%d; size=%zu\n", cmd.command, cmd_len);

			size_t offset = 0;
			while (offset < cmd_len)
			{
				rlen = read(fd, buf + offset, cmd_len - offset);
				if (rlen < 0)
					die("recv failed: %s\n", strerror(errno));
				if (rlen == 0)
					break;
				offset += rlen;
			}

			if (cmd.command == 0) {
				// Standard display data command
				struct timeval start_tv, stop_tv, delta_tv;
				gettimeofday(&start_tv, NULL);

				frame_num = frame_num == 0 ? 1 : 0;
				ledscape_frame_t * const frame = ledscape_frame(leds, frame_num);

				uint16_t given_pixels = cmd_len / 3;
				for (uint16_t i=0, s=0, p=0; i<given_pixels; i++, p++) {
					if (p >= led_count) {
						s ++;
						p = 0;

						if (s >= LEDSCAPE_NUM_STRIPS) break;
					}
					const uint8_t * const in = &buf[3 * i];
					ledscape_pixel_t * const px = &frame[p].strip[s];
					px->r = in[0];
					px->g = in[1];
					px->b = in[2];
				}

				ledscape_draw(leds, frame_num);
				const uint32_t response = ledscape_wait(leds);

				gettimeofday(&stop_tv, NULL);
				timersub(&stop_tv, &start_tv, &delta_tv);

				frames++;
				delta_sum += delta_tv.tv_usec;
				if (stop_tv.tv_sec - last_report < report_interval)
					continue;
				last_report = stop_tv.tv_sec;

				const unsigned delta_avg = delta_sum / frames;
				printf("%6u usec avg, max %.2f fps, actual %.2f fps (over %u frames)  res=%"PRIx32"\n",
					delta_avg,
					report_interval * 1.0e6 / delta_avg,
					frames * 1.0 / report_interval,
					frames,
					response
				);

				frames = delta_sum = 0;
			} else if (cmd.command == 255) {
				// System specific commands
				const uint16_t system_id = buf[0] << 8 | buf[1];

				if (system_id == OPC_SYSID_LEDSCAPE) {
					const opc_ledscape_cmd_id_t ledscape_cmd_id = buf[2];

					if (ledscape_cmd_id == OPC_LEDSCAPE_CMD_SET_CONFIG) {
						opc_ledscape_config_t* config_cmd = &buf[3];
						led_count = config_cmd->len_hi << 8 | config_cmd->len_lo;

						warn("Received config update request: (led_count=%d, pru0_mode=%d, pru1_mode=%d)\n", led_count, config_cmd->pru0_mode, config_cmd->pru1_mode);

						// TODO: Implement configuration updating
					} else if (ledscape_cmd_id == OPC_LEDSCAPE_CMD_GET_CONFIG) {
						opc_ledscape_config_t config;
						config.len_hi = (leds->num_pixels >> 8) & 0xFF;
						config.len_lo = leds->num_pixels & 0xFF;
						config.pru0_mode = leds->pru0_mode;
						config.pru1_mode = leds->pru1_mode;

						warn("Responding to config request\n");
						write(fd, &config, sizeof(config));
					} else {
						warn("WARN: Received command for unsupported LEDscape Command: %d\n", (int)ledscape_cmd_id);
					}
				} else {
					warn("WARN: Received command for unsupported system-id: %d\n", (int)system_id);
				}
			}
		}
	}

  ledscape_close(leds);

  return EXIT_SUCCESS;
}
