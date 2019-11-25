/*
 * Sunxi ion buffer helper function
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <ZengYajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <include.h>
#include "libion/ion_alloc.h"

#define log_error(fmt, args...)		printf("[ION] "fmt, ##args)
#define log_warning(fmt, args...)	printf("[ION] "fmt, ##args)

#ifdef __DEBUG__
#define log_debug(fmt, args...)		printf("[ION] "fmt, ##args)
#else
#define log_debug(fmt, args...)
#endif

int disp_buffer_init(void)
{
	return ion_alloc_open();
}

int disp_buffer_exit(void)
{
	return ion_alloc_close();
}

int disp_buffer_alloc(struct disp_buffer *buf)
{
	if (!buf || (buf->size <= 0))
		return -1;

	buf->base = ion_alloc_alloc(buf->size);
	if (!buf->base) {
		log_error("ion alloc failed, size %d\n", buf->size);
		return -1;
	}

	buf->physical = (unsigned int)ion_alloc_vir2phy(buf->base);
	log_error("map success: base %p, physical %08x\n",
		  buf->base, buf->physical);
	return 0;
}

int disp_buffer_flush(void *address, int size)
{
	ion_flush_cache(address, size);
	return 0;
}

int disp_buffer_free(struct disp_buffer *buf)
{
	return 0;
}
