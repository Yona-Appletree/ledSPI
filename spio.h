//
// Created by yona on 6/9/15.
//

#ifndef SPISCAPE_SPI_IO_H
#define SPISCAPE_SPI_IO_H

#endif //SPISCAPE_SPI_IO_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
	int fd;

	const char *device_path;

	uint32_t speed_hz;
	uint8_t spi_mode;
	uint8_t bits_per_word;
	uint16_t delay_usecs;
} spio_connection;

extern spio_connection* spio_open(
	const char *device,
	uint32_t speed
);

extern void spio_close(spio_connection* conn);

extern void spio_write(spio_connection* conn, const void* data, size_t len);