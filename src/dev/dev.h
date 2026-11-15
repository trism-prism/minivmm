#ifndef DEV_H
#define DEV_H

#include <stdbool.h>
#include <stdint.h>

/* Result of a device access. */
enum dev_result { DEV_NONE, DEV_OK, DEV_EXIT };	/* not mine, handled, handled and end the run (status = data[0]) */

/* Port I/O: data points at `size` bytes, read from or written to the guest. */
enum dev_result serial_io(uint16_t port, bool is_write, uint8_t *data, uint32_t size);
enum dev_result debug_exit_io(uint16_t port, bool is_write, uint8_t *data, uint32_t size);

/* MMIO: data points at `len` bytes. */
enum dev_result mmio_probe(uint64_t addr, bool is_write, uint8_t *data, uint32_t len);

#endif
