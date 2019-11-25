/*
 * Sunxi V4L2 helper function
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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>

#include <v4l2_utils.h>
#include <config.h>

const char *v4l2_field_names[] = {
	[V4L2_FIELD_ANY]        = "any",
	[V4L2_FIELD_NONE]       = "none",
	[V4L2_FIELD_TOP]        = "top",
	[V4L2_FIELD_BOTTOM]     = "bottom",
	[V4L2_FIELD_INTERLACED] = "interlaced",
	[V4L2_FIELD_SEQ_TB]     = "seq-tb",
	[V4L2_FIELD_SEQ_BT]     = "seq-bt",
	[V4L2_FIELD_ALTERNATE]  = "alternate",
};

const char *v4l2_type_names[] = {
	[V4L2_BUF_TYPE_VIDEO_CAPTURE]      = "video-cap",
	[V4L2_BUF_TYPE_VIDEO_OVERLAY]      = "video-over",
	[V4L2_BUF_TYPE_VIDEO_OUTPUT]       = "video-out",
	[V4L2_BUF_TYPE_VBI_CAPTURE]        = "vbi-cap",
	[V4L2_BUF_TYPE_VBI_OUTPUT]         = "vbi-out",
	[V4L2_BUF_TYPE_SLICED_VBI_CAPTURE] = "sliced-vbi-cap",
	[V4L2_BUF_TYPE_SLICED_VBI_OUTPUT]  = "slicec-vbi-out",
};

const char *v4l2_memory_names[] = {
	[V4L2_MEMORY_MMAP]    = "mmap",
	[V4L2_MEMORY_USERPTR] = "userptr",
	[V4L2_MEMORY_OVERLAY] = "overlay",
};

static int __v4l2_ioctl(int fd, unsigned long request, void *arg)
{
	return ioctl(fd, request, arg);
}

struct v4l2_device *v4l2_device_open(const char *path, int debug)
{
	struct v4l2_device *device = 0;
	int retval;

	device = calloc(1, sizeof(*device));
	if (!device) {
		logerr("calloc failed, %s\n", strerror(errno));
		return NULL;
	}

	device->fd = open(path, O_RDWR);
	if (device->fd < 0) {
		logerr("open '%s' failed, %s\n", path, strerror(errno));
		goto _error_out;
	}

	device->debug = debug;
	retval = __v4l2_ioctl(device->fd, VIDIOC_QUERYCAP, (void *)&device->capability);
	if (!retval && device->debug) {
		char dumpstr[4096] = {0};
		char *p = dumpstr;

		p += sprintf(p, "DEV: %s\n", path);
		p += sprintf(p, "\tdriver=%s\n\tcard=%s\n\tbus=%s\n\tversion=%d.%d.%d\n"
				"\tcapabilities=%s\n\tdevice_caps=%s\n",
				device->capability.driver,
				device->capability.card,
				device->capability.bus_info,
				(device->capability.version >> 16) & 0xff,
				(device->capability.version >>  8) & 0xff,
				device->capability.version         & 0xff,
				v4l2_capability_format_string(device->capability.capabilities),
				(device->capability.capabilities & V4L2_CAP_DEVICE_CAPS) ?
				v4l2_capability_format_string(device->capability.device_caps) : "N/A");
		fprintf(stdout, "%s\n", dumpstr);
	}

	device->stds.next     = NULL;
	device->inputs.next   = NULL;
	device->fmt_caps.next = NULL;

	return device;

_error_out:
	free(device);
	return NULL;
}

static void __free_list(struct drv_list *list_head)
{
	struct drv_list *list = list_head->next;

	while (list) {
		list_head->next = list->next;
		free(list->node);
		free(list);
		list = list_head->next;
		printf("++ free\n");
	}
}

void v4l2_device_close(struct v4l2_device *device)
{
	if (!device)
		return;
	
	__free_list(&device->stds);
	__free_list(&device->inputs);
	__free_list(&device->fmt_caps);

	close(device->fd);
	free(device);
}

int v4l2_device_enum_stds(struct v4l2_device *device)
{
	struct v4l2_standard *std = NULL;
	struct drv_list *list;
	int index = 0;
	int retval;

	while (1) {
		std = calloc(1, sizeof(struct v4l2_standard));
		assert(std != NULL);

		std->index = index++;
		retval = __v4l2_ioctl(device->fd, VIDIOC_ENUMSTD, std);
		if (retval < 0) {
			free(std);
			break;
		}

		if (device->debug) {
			printf("\tSTANDARD: index=%d, id=%08x, name=%s, fps=%.3f, "
				"framelines=%d\n", std->index,
				(unsigned int)std->id, std->name,
				1. * std->frameperiod.denominator / std->frameperiod.numerator,
				std->framelines);
		}

		list = calloc(1, sizeof(*list));
		assert(list != NULL);
		list->node = std;
		list->next = device->stds.next;
		device->stds.next = list;
	}
	return 0;
}

static void __print_frmsize(struct v4l2_frmsizeenum *frmsize)
{
	const char *frmsize_type[] = {
		"Unknown",
		"Discrete",
		"Continuous",
		"Stepwise"
	};

	char dumpstr[512] = {0};
	char *p = dumpstr;

	if (frmsize->type > 3 || frmsize->type < 0)
		frmsize->type = 0;
	
	p += sprintf(p, "\t%s ", frmsize_type[frmsize->type]);
	if (frmsize->type == V4L2_FRMSIZE_TYPE_DISCRETE) {
		p += sprintf(p, "%dx%d\n",
			frmsize->discrete.width, frmsize->discrete.height);
	}
	printf("%s", dumpstr);
}

static void __print_frmival(struct v4l2_frmivalenum *frmival)
{
	struct v4l2_fract *f = &frmival->discrete;

	printf("\t\tInterval: %.3fs (%.3f fps)\n",
		(1.0 * f->numerator) / f->denominator,
		(1.0 * f->denominator) / f->numerator);
}

int v4l2_device_enum_fmt(struct v4l2_device *device, enum v4l2_buf_type type)
{
	struct v4l2_fmtdesc *fmt = NULL;
	struct drv_list *list;
	int index = 0;
	int retval;

	while (1) {
		fmt = calloc(1, sizeof(struct v4l2_fmtdesc));
		assert(fmt != NULL);

		fmt->index = index++;
		fmt->type = type;
		retval = __v4l2_ioctl(device->fd, VIDIOC_ENUM_FMT, fmt);
		if (retval < 0) {
			free(fmt);
			break;
		}

		if (device->debug) {
			printf("\tFORMAT: index=%d, type=%d, flags=%d, description='%s'\n\t "
				"fourcc=%c%c%c%c\n",
				fmt->index, fmt->type, fmt->flags, fmt->description,
				fmt->pixelformat & 0xff,
				(fmt->pixelformat >>  8) & 0xff,
				(fmt->pixelformat >> 16) & 0xff,
				(fmt->pixelformat >> 24) & 0xff);
		}

		struct v4l2_frmsizeenum frmsize;
		frmsize.pixel_format = fmt->pixelformat;
		frmsize.index = 0;
		while (__v4l2_ioctl(device->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
			__print_frmsize(&frmsize);

			if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
				struct v4l2_frmivalenum frmival;

				frmival.index = 0;
				frmival.pixel_format = fmt->pixelformat;
				frmival.width = frmsize.discrete.width;
				frmival.height = frmsize.discrete.height;
				while (__v4l2_ioctl(device->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0) {
					__print_frmival(&frmival);
					frmival.index++;
				}
			}
			frmsize.index++;
		}

		list = calloc(1, sizeof(*list));
		assert(list != NULL);
		list->node = fmt;
		list->next = device->fmt_caps.next;
		device->fmt_caps.next = list;
	}
	return 0;
}

int v4l2_device_enum_input(struct v4l2_device *device)
{
	struct v4l2_input *input = NULL;
	struct drv_list *list;
	int index = 0;
	int retval;

	while (1) {
		input = calloc(1, sizeof(struct v4l2_input));
		assert(input != NULL);

		input->index = index++;
		retval = __v4l2_ioctl(device->fd, VIDIOC_ENUMINPUT, input);
		if (retval < 0) {
			free(input);
			break;
		}

		if (device->debug) {
			printf("\tINPUT: index=%d, name=%s, type=%d, audioset=%d, "
				"tuner=%d, std=%08x, status=%d\n",
				input->index, input->name, input->type, input->audioset,
				input->tuner, (unsigned int)input->std, input->status);
		}

		list = calloc(1, sizeof(*list));
		assert(list != NULL);
		list->node = input;
		list->next = device->inputs.next;
		device->inputs.next = list;
	}
	return 0;
}

int v4l2_device_set_fmt(struct v4l2_device *device, struct v4l2_format *fmt,
	unsigned int width, unsigned int height, unsigned int pixelformat, enum v4l2_field field)
{
	int retval = 0;
	struct v4l2_pix_format *pix = &(fmt->fmt.pix);

	fmt->type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	pix->width       = width;
	pix->height      = height;
	pix->pixelformat = pixelformat;
	pix->field       = field;

	if (__v4l2_ioctl(device->fd, VIDIOC_S_FMT, fmt) < 0) {
		fprintf(stderr, "set video capture format failed, %s\n", strerror(errno));
		return -1;
	}
	device->sizeimage = pix->sizeimage;

#if 0
	if (pix->pixelformat != pixelformat ||
		pix->width != width || pix->height != height ||
		pix->bytesperline == 0 || pix->sizeimage == 0) {
#else
	if (pix->pixelformat != pixelformat ||
		pix->width != width || pix->height != height) {
#endif
		fprintf(stderr, "Error: asked pixelformat %d, size %dx%d\n"
				"       recev pixelformat %d, size %dx%d\n",
			pixelformat, width, height,
			pix->pixelformat, pix->width, pix->height);
		retval = -1;
	}

#if 0
	if (pix->bytesperline == 0 ) {
		fprintf(stderr, "Error: bytesperline = 0\n");
		retval = -1;
	}

	if (pix->sizeimage == 0 ) {
		fprintf(stderr, "Error: sizeimage = 0\n");
		retval = -1;
	}
#endif

	if (device->debug)
		printf( "FMT SET: %dx%d, fourcc=%c%c%c%c, %d bytes/line,"
			" %d bytes/frame, colorspace=0x%08x\n",
			pix->width,pix->height,
			pix->pixelformat & 0xff,
			(pix->pixelformat >>  8) & 0xff,
			(pix->pixelformat >> 16) & 0xff,
			(pix->pixelformat >> 24) & 0xff,
			pix->bytesperline,
			pix->sizeimage,
			pix->colorspace);
	return retval;
}

static void prt_buf_info(char *name,struct v4l2_buffer *p)
{
	struct v4l2_timecode *tc=&p->timecode;

	printf ("%s: %02ld:%02d:%02d.%08ld index=%d, type=%s, "
		"bytesused=%d, flags=0x%08x, "
		"field=%s, sequence=%d, memory=%s, offset=0x%08x, length=%d\n",
		name, (p->timestamp.tv_sec/3600),
		(int)(p->timestamp.tv_sec/60)%60,
		(int)(p->timestamp.tv_sec%60),
		p->timestamp.tv_usec,
		p->index,
		prt_names(p->type,v4l2_type_names),
		p->bytesused,p->flags,
		prt_names(p->field,v4l2_field_names),
		p->sequence,
		prt_names(p->memory,v4l2_memory_names),
		p->m.offset,
		p->length);
	tc=&p->timecode;
	printf ("\tTIMECODE: %02d:%02d:%02d type=%d, "
		"flags=0x%08x, frames=%d, userbits=0x%02x%02x%02x%02x\n",
		tc->hours,tc->minutes,tc->seconds,
		tc->type, tc->flags, tc->frames,
		tc->userbits[0],
		tc->userbits[1],
		tc->userbits[2],
		tc->userbits[3]);
}

int v4l2_device_mmap_bufs(struct v4l2_device *device, unsigned int num_buffers)
{
#if 0
	if (!device->sizeimage) {
		fprintf(stderr, "image size is zero!\n");
		return -1;
	}
#endif

	device->reqbuf.count = num_buffers;
	device->reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	device->reqbuf.memory = V4L2_MEMORY_MMAP;

	printf("%08x %08x\n", device->reqbuf.type, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	if (__v4l2_ioctl(device->fd, VIDIOC_REQBUFS, &device->reqbuf) < 0) {
		printf("VIDIOC_REQBUFS failed!\n");
		return -1;
	}

	if (device->debug) {
		printf("\tREQBUFS: count=%d, type=%s, memory=%s\n",
			device->reqbuf.count,
			prt_names(device->reqbuf.type, v4l2_type_names),
			prt_names(device->reqbuf.memory, v4l2_memory_names));
	}

	device->v4l2_bufs = calloc(device->reqbuf.count, sizeof(*device->v4l2_bufs));
	assert(device->v4l2_bufs != NULL);
	device->bufs = calloc(device->reqbuf.count, sizeof(*device->bufs));
	assert(device->bufs);

	for (device->n_bufs = 0; device->n_bufs < device->reqbuf.count; device->n_bufs++) {
		struct v4l2_buffer *p;

		p = calloc(1, sizeof(*p));
		assert(p != NULL);
		device->v4l2_bufs[device->n_bufs] = p;

		p->index  = device->n_bufs;
		p->type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		p->memory = V4L2_MEMORY_MMAP;
		if (__v4l2_ioctl(device->fd, VIDIOC_QUERYBUF, p) < 0) {
			free(p);
			fprintf(stderr, "querybuf failed, exit!\n");
			return -1;
		}

		if (device->debug)
			prt_buf_info("QUERYBUF", p);

		device->bufs[device->n_bufs].length = p->length;
		device->bufs[device->n_bufs].start = mmap(NULL,
				p->length,
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				device->fd, p->m.offset);

		if (device->bufs[device->n_bufs].start == MAP_FAILED) {
			free(p);
			fprintf(stderr, "mmap failed, exit!\n");
			return -1;
		}
	}
	return 0;
}

int v4l2_device_free_bufs(struct v4l2_device *device)
{
	unsigned int i;
	struct v4l2_buffer_mmap *buf;

	if (!device || !device->n_bufs)
		return 0;

	device->reqbuf.count  = 0;
	device->reqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	device->reqbuf.memory = V4L2_MEMORY_MMAP;

	if (__v4l2_ioctl(device->fd, VIDIOC_STREAMOFF, &device->reqbuf.type) < 0) {
		fprintf(stderr, "streamoff request failed!\n");
		return -1;
	}

	sleep(1);

	for (i=0; i<device->n_bufs; i++) {
		buf = &(device->bufs[i]);
		if (buf->length)
			munmap(buf->start, buf->length);
		if (device->v4l2_bufs[i])
			free(device->v4l2_bufs[i]);
	}

	free(device->v4l2_bufs);
	free(device->bufs);

	device->v4l2_bufs = NULL;
	device->bufs      = NULL;
	device->n_bufs    = 0;
	return 0;
}

int v4l2_device_start_streaming(struct v4l2_device *device)
{
	int i;
	int retval;
	struct v4l2_buffer buffer;

	for (i = 0; i < device->n_bufs; i++) {
		memset(&buffer, 0, sizeof(buffer));
		buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index  = i;

		retval = __v4l2_ioctl(device->fd, VIDIOC_QBUF, &buffer);
		if (!retval) {
			if (device->debug)
				prt_buf_info("QBUF", &buffer);
		} else {
			fprintf(stderr, "Error: queue buffer request\n");
			return -1;
		}
	}

	if (device->debug)
		printf("Enabling streaming\n");

	printf("%08x %08x\n", device->reqbuf.type, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	return __v4l2_ioctl(device->fd, VIDIOC_STREAMON, &device->reqbuf.type);
}

int v4l2_device_stop_streaming(struct v4l2_device *device)
{
	v4l2_device_free_bufs(device);
	return 0;
}

int v4l2_device_receive_buffer(struct v4l2_device *device, v4l2_receive_buffer_callback *callback)
{
	int retval = 0;
	struct v4l2_buffer buffer;

	memset(&buffer, 0, sizeof(buffer));
	buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.memory = V4L2_MEMORY_MMAP;

	if (__v4l2_ioctl(device->fd, VIDIOC_DQBUF, &buffer) == -1) {
		switch (errno) {
		case EAGAIN:
			return 0;
		case EIO:
		default:
			perror("dequeue buffer");
			return -errno;
		}
	}
	if (device->debug)
		prt_buf_info("DQBUF", &buffer);

	if (callback)
		retval = callback(&buffer, &device->bufs[buffer.index]);

	if (__v4l2_ioctl(device->fd, VIDIOC_QBUF, &buffer) < 0) {
		perror("queue buffer");
		return -errno;
	}

	return retval;
}
