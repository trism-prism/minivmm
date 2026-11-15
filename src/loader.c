#include "loader.h"
#include "util.h"

#include <stdio.h>

void loader_load(struct vm *v, const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		die("cannot open %s: %s", path, strerror(errno));
	size_t max = RAM_SIZE - LOAD_ADDR;
	size_t n = fread(v->ram + LOAD_ADDR, 1, max, f);
	/* A full buffer plus one more byte means the image does not fit. */
	int extra = n == max ? fgetc(f) : EOF;
	if (ferror(f))
		die("read error on %s", path);
	fclose(f);
	if (n == 0)
		die("%s is empty", path);
	if (extra != EOF)
		die("%s is larger than guest RAM (max %zu bytes)", path, max);
}
