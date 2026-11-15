#include "dev/dev.h"

#include <errno.h>
#include <stddef.h>
#include <unistd.h>

/* Output is buffered and written when full or at serial_flush(). Trade-off: output of a guest killed by a signal or looping forever is lost until the buffer fills. */
static uint8_t buf[4096];
static size_t len;

void serial_flush(void)
{
	size_t off = 0;
	while (off < len) {
		ssize_t n = write(1, buf + off, len - off);
		if (n > 0)
			off += (size_t)n;
		else if (n < 0 && errno == EINTR)
			continue;
		else
			break;	/* e.g. closed stdout (EPIPE): drop the rest, the guest must not die for it */
	}
	len = 0;
}

enum dev_result serial_io(uint16_t port, bool is_write, uint8_t *data, uint32_t size)
{
	/* Only byte-wide accesses are modelled; wider ones report as unhandled rather than drop bytes. */
	if (size != 1)
		return DEV_NONE;
	if (port == 0x3F8 && is_write) {
		buf[len++] = data[0];
		if (len == sizeof buf)
			serial_flush();
		return DEV_OK;
	}
	if (port == 0x3FD && !is_write) {
		/* Line status: transmitter empty and holding register empty, so polling guests proceed. */
		data[0] = 0x60;
		return DEV_OK;
	}
	return DEV_NONE;
}
