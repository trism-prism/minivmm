#ifndef UTIL_H
#define UTIL_H

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

static inline _Noreturn void die(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static inline _Noreturn void die(const char *fmt, ...)
{
	va_list ap;
	fputs("minivmm: ", stderr);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
}

/* ioctl that dies on failure, naming the request; evaluates to the return value. */
#define KIOCTL(fd, req, arg) \
	({ int r_ = ioctl((fd), (req), (void *)(uintptr_t)(arg)); \
	   if (r_ < 0) die(#req " failed: %s", strerror(errno)); \
	   r_; })

#endif
