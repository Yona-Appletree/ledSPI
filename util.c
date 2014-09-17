/** \file
 * Various utility functions
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

/** Write all the bytes to a fd, even if there is a brief interruption.
 * \return number of bytes written or -1 on any fatal error.
 */
ssize_t
write_all(
	const int fd,
	const void * const buf_ptr,
	const size_t len
)
{
	const uint8_t * const buf = buf_ptr;
	size_t offset = 0;

	while (offset < len)
	{
		const ssize_t rc = write(fd, buf + offset, len - offset);
		if (rc < 0)
		{
			if (errno == EAGAIN)
				continue;
			return -1;
		}

		if (rc == 0)
			return -1;

		offset += rc;
	}

	return len;
}
		

int
serial_open(
	const char * const dev
)
{
	const int fd = open(dev, O_RDWR | O_NONBLOCK | O_NOCTTY, 0666);
	if (fd < 0)
		return -1;

	// Disable modem control signals
	struct termios attr;
	tcgetattr(fd, &attr);
	attr.c_cflag |= CLOCAL | CREAD;
	attr.c_oflag &= ~OPOST;
	tcsetattr(fd, TCSANOW, &attr);

	return fd;
}


void
hexdump(
	FILE * const outfile,
	const void * const buf,
	const size_t len
)
{
	const uint8_t * const p = buf;

	for(size_t i = 0 ; i < len ; i++)
	{
		if (i % 8 == 0)
			fprintf(outfile, "%s%04zu:", i == 0 ? "": "\n", i);
		fprintf(outfile, " %02x", p[i]);
	}

	fprintf(outfile, "\n");
}

#ifndef HAVE_STRLCAT
/*
 * '_cups_strlcat()' - Safely concatenate two strings.
 */

size_t                  /* O - Length of string */
strlcat(char       *dst,        /* O - Destination string */
              const char *src,      /* I - Source string */
          size_t     size)      /* I - Size of destination string buffer */
{
  size_t    srclen;         /* Length of source string */
  size_t    dstlen;         /* Length of destination string */


 /*
  * Figure out how much room is left...
  */

  dstlen = strlen(dst);
  size   -= dstlen + 1;

  if (!size)
    return (dstlen);        /* No room, return immediately... */

 /*
  * Figure out how much room is needed...
  */

  srclen = strlen(src);

 /*
  * Copy the appropriate amount...
  */

  if (srclen > size)
    srclen = size;

  memcpy(dst + dstlen, src, srclen);
  dst[dstlen + srclen] = '\0';

  return (dstlen + srclen);
}
#endif /* !HAVE_STRLCAT */

#ifndef HAVE_STRLCPY
/*
* '_cups_strlcpy()' - Safely copy two strings.
*/

size_t /* O - Length of string */
strlcpy(
	char       *dst, /* O - Destination string */
	const char *src, /* I - Source string */
	size_t      size /* I - Size of destination string buffer */
) {
	size_t srclen; /* Length of source string */

	/*
	* Figure out how much room is needed...
	*/

	size --;

	srclen = strlen(src);

	/*
	* Copy the appropriate amount...
	*/

	if (srclen > size)
	srclen = size;

	memcpy(dst, src, srclen);
	dst[srclen] = '\0';

	return (srclen);
}
#endif /* !HAVE_STRLCPY */