
#ifndef __V4L2_UTILS_H__
#define __V4L2_UTILS_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>
#include "videodev2.h"

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*arr))
#define prt_names(a,arr) (((a)<ARRAY_SIZE(arr))?arr[a]:"unknown")

struct drv_list {
	void *node;
	struct drv_list *next;
};

struct v4l2_buffer_mmap {
	void *start;
	size_t length;
};

typedef int v4l2_receive_buffer_callback (struct v4l2_buffer *v4l2_buf, struct v4l2_buffer_mmap *buf);

struct v4l2_device {
	int fd;

	int debug;

	/* V4L2 structs */
	struct v4l2_capability capability;
	struct v4l2_streamparm parm;

	/* lists to be used to store enumbered values */
	struct drv_list stds;
	struct drv_list inputs;
	struct drv_list fmt_caps;

	/* Stream control */
	struct v4l2_format         format;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer         **v4l2_bufs;
	struct v4l2_buffer_mmap    *bufs;
	uint32_t                   sizeimage;
	uint32_t                   n_bufs;

	/* Queue control */
	uint32_t waitq;
	uint32_t currq;
};

char *v4l2_capability_format_string(uint32_t caps);
struct v4l2_device *v4l2_device_open(const char *path, int debug);
void v4l2_device_close(struct v4l2_device *device);
int v4l2_device_enum_stds(struct v4l2_device *device);
int v4l2_device_enum_fmt(struct v4l2_device *device, enum v4l2_buf_type type);
int v4l2_device_enum_input(struct v4l2_device *device);

int v4l2_device_mmap_bufs(struct v4l2_device *device, unsigned int num_buffers);
int v4l2_device_free_bufs(struct v4l2_device *device);

int v4l2_device_set_fmt(struct v4l2_device *device, struct v4l2_format *fmt,
	unsigned int width, unsigned int height, unsigned int pixelformat, enum v4l2_field field);

int v4l2_device_start_streaming(struct v4l2_device *device);
int v4l2_device_stop_streaming(struct v4l2_device *device);
int v4l2_device_receive_buffer(struct v4l2_device *device, v4l2_receive_buffer_callback *callback);

#endif
