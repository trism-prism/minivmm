#include "dev/dev.h"

enum dev_result debug_exit_io(uint16_t port, bool is_write, uint8_t *data, uint32_t size)
{
	(void)data;
	/* Bench sink: byte writes accepted and dropped, so pio_loop measures the exit, not a device. QEMU's debugcon port is also 0xE9. */
	if (port == 0xE9 && is_write && size == 1)
		return DEV_OK;
	/* Write-only, byte-wide; the dispatcher takes the status from data[0]. */
	return port == 0xF4 && is_write && size == 1 ? DEV_EXIT : DEV_NONE;
}
