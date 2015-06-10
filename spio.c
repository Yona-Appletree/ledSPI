//
// Created by yona on 6/9/15.
//

#include "spio.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define SPI_BUFFER_SIZE 4096

static void pabort(const char *s)
{
	perror(s);
	abort();
}

spio_connection* spio_open(
	const char *device,
	uint32_t speed
) {
	spio_connection* conn = malloc(sizeof(spio_connection));

	conn->device_path = device;
	conn->speed_hz = speed;
	conn->spi_mode = 0;
	conn->bits_per_word = 8;
	conn->delay_usecs = 0;

	conn->fd = open(device, O_WRONLY);

	/*
	 * spi mode
	 */
	if (ioctl(conn->fd, SPI_IOC_RD_MODE, &conn->spi_mode) == -1)
		pabort("can't set spi mode");

	if (ioctl(conn->fd, SPI_IOC_RD_MODE, &conn->spi_mode) == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	if (ioctl(conn->fd, SPI_IOC_WR_BITS_PER_WORD, &conn->bits_per_word) == -1)
		pabort("can't set bits per word");

	if (ioctl(conn->fd, SPI_IOC_RD_BITS_PER_WORD, &conn->bits_per_word) == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	if (ioctl(conn->fd, SPI_IOC_WR_MAX_SPEED_HZ, &conn->speed_hz) == -1)
		pabort("can't set max speed hz");

	if (ioctl(conn->fd, SPI_IOC_RD_MAX_SPEED_HZ, &conn->speed_hz) == -1)
		pabort("can't get max speed hz");

	return conn;
}

void spio_close(spio_connection* conn) {
	if (conn->fd >= 0) {
		close(conn->fd);
		conn->fd = -1;
	}
}

void spio_write(spio_connection* conn, const void* data, size_t len) {
	for (size_t i=0; i<len; i += SPI_BUFFER_SIZE) {
		size_t packet_size = i + SPI_BUFFER_SIZE > len
		                     ? len - i
		                     : SPI_BUFFER_SIZE;

		if (packet_size > 0) {
//			int ret;
//			struct spi_ioc_transfer tr = {
//				.tx_buf = (unsigned long)data + i,
//				.rx_buf = (unsigned long)data + i,
//				.len = packet_size,
//				.delay_usecs = conn->delay_usecs,
//				.speed_hz = conn->speed_hz,
//				.bits_per_word = conn->bits_per_word,
//			};
//
//			ret = ioctl(conn->fd, SPI_IOC_MESSAGE(1), &tr);
//			if (ret < 1)
//				pabort("can't send spi message");

			write(conn->fd, data + i, packet_size);
		}
	}
}

