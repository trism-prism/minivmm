#include "dev/dev.h"

enum dev_result debug_exit_io(uint16_t port, bool is_write, uint8_t *data, uint32_t size)
{
	(void)data;
	/* Write-only, byte-wide; the dispatcher takes the status from data[0]. */
	return port == 0xF4 && is_write && size == 1 ? DEV_EXIT : DEV_NONE;
}
