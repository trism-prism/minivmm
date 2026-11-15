#include "dev/dev.h"

#include <unistd.h>

enum dev_result serial_io(uint16_t port, bool is_write, uint8_t *data, uint32_t size)
{
	/* Only byte-wide accesses are modelled; wider ones report as unhandled rather than drop bytes. */
	if (size != 1)
		return DEV_NONE;
	if (port == 0x3F8 && is_write) {
		/* One write(2) per byte on purpose: the naive baseline for a later buffering comparison. */
		ssize_t n = write(1, data, 1);
		(void)n;	/* a closed stdout must not kill the guest */
		return DEV_OK;
	}
	if (port == 0x3FD && !is_write) {
		/* Line status: transmitter empty and holding register empty, so polling guests proceed. */
		data[0] = 0x60;
		return DEV_OK;
	}
	return DEV_NONE;
}
