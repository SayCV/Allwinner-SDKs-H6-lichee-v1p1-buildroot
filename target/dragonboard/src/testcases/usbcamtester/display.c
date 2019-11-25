/*
 * Simple layer display helper function
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include "drv_display.h"

struct display_context {
	int dispfd;
	int screen_id;
	int width;
	int height;

	disp_layer_config layer;
};

static struct display_context *_disp_context = NULL;

void display_frame_init(int src_width, int src_height,
		int x, int y, int width, int height, int format, int zorder, int layer)
{
	disp_layer_config *config = &_disp_context->layer;

	/* set layer config */
	memset(config, 0, sizeof(disp_layer_config));

	config->channel  = 0;
	config->layer_id = layer;
	config->enable   = 1;

	config->info.fb.addr[0]    = 0;
	config->info.fb.format     = format;

	config->info.fb.size[0].width  = src_width;
	config->info.fb.size[0].height = src_height;

	config->info.mode   = LAYER_MODE_BUFFER;
	config->info.zorder = zorder;

	// image size
	config->info.fb.crop.width  = (unsigned long long)src_width  << 32;
	config->info.fb.crop.height = (unsigned long long)src_height << 32;

	// source window
	config->info.alpha_mode        = 1; //global alpha
	config->info.alpha_value       = 0xff;
	config->info.screen_win.x      = x;
	config->info.screen_win.y      = y;
	config->info.screen_win.width  = width;
	config->info.screen_win.height = height;
}

static void _close_all_layer(struct display_context *context)
{
	int i, j;
	unsigned int arg[4];
	disp_layer_config *config = &context->layer;

	/* set layer config */
	memset(config, 0, sizeof(disp_layer_config));

	config->channel = 0;
	config->layer_id = 0;
	config->enable = 0;

	for (j=0; j<4; j++) {
		config->channel = j;
		for (i = 0; i < 4; i++) {
			config->layer_id = i;
			arg[0] = _disp_context->screen_id;
			arg[1] = (unsigned int)config;
			arg[2] = 1; /* layer num */
			ioctl(_disp_context->dispfd, DISP_LAYER_SET_CONFIG, (void*)arg);
		}
	}
}

int display_device_init(void)
{
	unsigned int args[4];
	int screen_id = 0;
	int width, height;
	disp_output_type output_type;

	if (_disp_context)
		return 0;

	_disp_context = malloc(sizeof(*_disp_context));
	memset(_disp_context, 0, sizeof(*_disp_context));

	_disp_context->dispfd = open("/dev/disp", O_RDWR);
	if (_disp_context->dispfd < 0)
		goto _errout;

	for (screen_id = 0; screen_id < 2; screen_id++) {
		args[0] = screen_id;
		output_type = (disp_output_type)ioctl(_disp_context->dispfd,
							DISP_GET_OUTPUT_TYPE, (void *)args);

		if (output_type == DISP_OUTPUT_TYPE_LCD) {
#ifdef __DEBUG__
			printf("LCD screen id: %d\n", screen_id);
#endif
			_disp_context->screen_id = screen_id;
			break;
		}
	}

	if (screen_id >= 2)
		goto _errout;

	/* get screen size */
	args[0] = _disp_context->screen_id;
	width = ioctl(_disp_context->dispfd, DISP_GET_SCN_WIDTH, (void*)args);
	height = ioctl(_disp_context->dispfd, DISP_GET_SCN_HEIGHT, (void*)args);
#ifdef __DEBUG__
	printf("screen size=%d x %d \n", width, height);
#endif
	_disp_context->width = width;
	_disp_context->height = height;

//	_close_all_layer(_disp_context);
	return 0;

_errout:
	if (_disp_context->dispfd > 0)
		close(_disp_context->dispfd);
	free(_disp_context);
	_disp_context = NULL;
	return -1;
}

int display_device_deinit(void)
{
	if (_disp_context) {
		if (_disp_context->dispfd > 0)
			close(_disp_context->dispfd);

		free(_disp_context);
		_disp_context = NULL;
	}

	return 0;
}

int display_get_layer_config(int channel, int id, disp_layer_config *config)
{
	int ret;
	unsigned int arg[4];

	/* set layer config */
	memset(config, 0, sizeof(disp_layer_config));

	config->channel = channel;
	config->layer_id = id;

	arg[0] = _disp_context->screen_id;
	arg[1] = (unsigned int)config;
	arg[2] = 1; // layer num
	ret = ioctl(_disp_context->dispfd, DISP_LAYER_GET_CONFIG, (void*)arg);
	if (ret!=0)
		printf("fail to get layer config\n");

	printf("frame: %dx%d\n", config->info.screen_win.width, config->info.screen_win.height);
	return 0;
}

int display_update_frame(void *address)
{
	int ret;
	unsigned int arg[4];

	_disp_context->layer.info.fb.addr[0] = address;
	arg[0] = _disp_context->screen_id;
	arg[1] = (unsigned int)&_disp_context->layer;
	arg[2] = 1; // layer num
	ret = ioctl(_disp_context->dispfd, DISP_LAYER_SET_CONFIG, (void*)arg);
	if (ret!=0)
		printf("fail to set layer config\n");

#if 0
	disp_layer_config layer;
	memcpy(&layer, &_disp_context->layer, sizeof(disp_layer_config));
	layer.layer_id = 1;
	layer.info.screen_win.x      = 800;
	layer.info.screen_win.y      = 0;

	arg[0] = _disp_context->screen_id;
	arg[1] = (unsigned int)&layer;
	arg[2] = 1; // layer num
	ret = ioctl(_disp_context->dispfd, DISP_LAYER_SET_CONFIG, (void*)arg);
#endif

	return 0;
}

