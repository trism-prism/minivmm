#include "dev/dev.h"

#include <string.h>

enum dev_result mmio_probe(uint64_t addr, bool is_write, uint8_t *data, uint32_t len)
{
	/* VGA window: no RAM behind it, so it exercises the MMIO exit path. */
	if (addr < 0xA0000 || addr + len > 0xC0000)
		return DEV_NONE;
	if (!is_write)
		memset(data, 0, len);
	return DEV_OK;
}
