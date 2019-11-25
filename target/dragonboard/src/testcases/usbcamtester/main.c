/*
 * Sunxi USB camera testcase
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <include.h>
#include "drv_display.h"
#include <v4l2_utils.h>
#include "dragonboard_inc.h"

#define DISP_BUFFER_CNT	(2)

extern int disp_buffer_alloc(struct disp_buffer *buf);
extern int disp_buffer_flush(void *address, int size);

extern int display_device_init(void);
extern void display_frame_init(int src_width, int src_height,
		int x, int y, int width, int height, int format, int zorder, int layer);
extern int display_update_frame(void *address);

int preview_loop(struct v4l2_device *device, int count);

struct ringbuf {
	unsigned char *base;
	unsigned int physical;
	unsigned int size;
	struct ringbuf *next;
};

struct disp_buffer disp_buffer;
struct ringbuf *dispframe;
struct ringbuf ringbuf_array[DISP_BUFFER_CNT];

struct options_t {
	char *devpath;
	int video_width;
	int video_height;
	int video_format;

	int debug;

	int x;
	int y;
	int screen_width;
	int screen_height;

	int zorder;
	int layer;
	int interval_time;
};

static struct options_t _opt = {
	.devpath      = "/dev/video0",
	.video_width  = 640,
	.video_height = 480,
	.video_format = DISP_FORMAT_YUV422_I_VYUY,
	.debug        = 0,

	.x = 1000,
	.y = 0,
	.screen_width  = 600,
	.screen_height = 480,

	.zorder = 12,
	.layer  = 0,
	.interval_time = 6,
};

static void usage(char *name)
{
	fprintf(stderr, "Usage: %s [-d device] [-r resolution] [-p position ] [-s size] [-f format] [-v] [-h]\n", name);
	fprintf(stderr, "   -o: Video input device, default: /dev/video0\n");
	fprintf(stderr, "   -r: Resolution of video capture, something like: 640x480\n");
	fprintf(stderr, "   -p: Position of the video, something like: [200:400]\n");
	fprintf(stderr, "   -s: Screen size of the video view, something like: 320x240\n");
	fprintf(stderr, "   -t: camera switch time,default 6 seconds");
	fprintf(stderr, "   -f: Image format\n");
	fprintf(stderr, "   -h: Print this message\n");
}

static void parse_opt(int argc, char **argv)
{
	int c;

	do {
		c = getopt(argc, argv, "d:r:f:p:s:z:l:t:vh");
		if (c == EOF)
			break;
		switch (c) {
		case 'd':
			_opt.devpath = optarg;
			break;
		case 'r':
			if (optarg)
				sscanf(optarg, "%dx%d\n",
					&_opt.video_width, &_opt.video_height);
			break;
		case 'p':
			if (optarg)
				sscanf(optarg, "[%d:%d]\n",
					&_opt.x, &_opt.y);
			break;
		case 's':
			if (optarg)
				sscanf(optarg, "%dx%d\n",
					&_opt.screen_width, &_opt.screen_height);
			break;

		case 'z':
			if (optarg)
				sscanf(optarg, "%d\n", &_opt.zorder);
			break;

		case 'l':
			if (optarg)
				sscanf(optarg, "%d\n", &_opt.layer);
			break;

		case 't':
			if (optarg)
				sscanf(optarg, "%d\n", &_opt.interval_time);
			break;

		case 'f':
			break;
		case 'v':
			_opt.debug = 1;
			break;
		case 'h':
		default:
			usage(argv[0]);
			exit(1);
			break;
		}
	} while (1);

	if (optind != argc) {
		usage(argv[0]);
		exit(1);
	}

	fprintf(stdout, "  devpth: %s\n", _opt.devpath);
	fprintf(stdout, "   width: %d\n", _opt.video_width);
	fprintf(stdout, "  height: %d\n", _opt.video_height);
	fprintf(stdout, "position: (%d, %d)\n", _opt.x, _opt.y);
	fprintf(stdout, "   frame: (%d, %d)\n", _opt.screen_width, _opt.screen_height);
	fprintf(stdout, "  format: %d\n", _opt.video_format);
}

int main(int argc, char **argv)
{
	int i;
	int retval;
	struct v4l2_device *dev;
	parse_opt(argc, argv);

	dev = v4l2_device_open(_opt.devpath, _opt.debug);
#if 0
	v4l2_device_enum_stds(dev);
	v4l2_device_enum_fmt(dev, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	v4l2_device_enum_input(dev);
#endif

	retval = v4l2_device_set_fmt(dev, &dev->format, _opt.video_width,
					_opt.video_height, V4L2_PIX_FMT_YUYV, V4L2_FIELD_ANY);
	if (retval)
		exit(1);
	display_device_init();
	display_frame_init(_opt.video_width, _opt.video_height, _opt.x, _opt.y,
			_opt.screen_width, _opt.screen_height, _opt.video_format,
			_opt.zorder, _opt.layer);

	int sizeimage = _opt.video_width * _opt.video_height * 2;
	memset(&disp_buffer, 0, sizeof(disp_buffer));
	disp_buffer.size = sizeimage * DISP_BUFFER_CNT;
	disp_buffer_init();
	disp_buffer_alloc(&disp_buffer);
	for (i = 0; i < DISP_BUFFER_CNT; i++) {
		ringbuf_array[i].base = disp_buffer.base + sizeimage * i;
		ringbuf_array[i].physical = disp_buffer.physical + sizeimage * i;
		ringbuf_array[i].size = sizeimage;

		if (i < DISP_BUFFER_CNT - 1)
			ringbuf_array[i].next = &ringbuf_array[i + 1];
		else
			ringbuf_array[i].next = &ringbuf_array[0];
	}
	dispframe = &ringbuf_array[0];

	v4l2_device_mmap_bufs(dev, 4);

	if (v4l2_device_start_streaming(dev) != 0)
		exit(1);

	preview_loop(dev, 10);
	v4l2_device_stop_streaming(dev);

	v4l2_device_free_bufs(dev);
	v4l2_device_close(dev);
	return 0;
}

int v4l2_buffer_recore(struct v4l2_buffer *v4l2_buf,
		       struct v4l2_buffer_mmap *buf)
{

#if 0
	static int index = 0;
	char fname[32] = { 0 };
	int fd;

	sprintf(fname, "%08x", index++);
	fd = open(fname, O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		perror("create file");
		return 0;
	}

	write(fd, buf->start, buf->length);
	close(fd);
#else
	memcpy(dispframe->base, buf->start, buf->length);
//	memset(dispframe->base, 0, buf->length);
	disp_buffer_flush(dispframe->base, dispframe->size);
	display_update_frame((void *)dispframe->physical);
	dispframe = dispframe->next;
#endif

	return 0;
}

int preview_loop(struct v4l2_device *device, int count)
{
	fd_set fds;
	struct timeval tv;
	int retval;
	struct  timeval    start_time;
    struct  timeval    end_time;
    
	gettimeofday(&start_time,NULL);
	while (1) {
		gettimeofday(&end_time,NULL);
		if(_opt.interval_time > 0 && end_time.tv_sec - start_time.tv_sec > _opt.interval_time)
  			break;
		FD_ZERO(&fds);
		FD_SET(device->fd, &fds);

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		retval = select(device->fd + 1, &fds, NULL, NULL, &tv);
		if (retval == -1) {
			if (errno == EINTR)
				continue;
			perror("select");
			return errno;
		}

		if (retval == 0) {
			fprintf(stderr, "select timeout\n");
			return errno;
		}

		v4l2_device_receive_buffer(device, v4l2_buffer_recore);
	}
	return 0;
}
