/*
 * file        tvd_camera.c
 * brief       this is only used in v40 platform  
 *
 * version     1.0.0
 * date        2016.2.26
 * author      liubaihao <liubaihao@allwinnertech.com>
 * 
 * Copyright (c) 2016 Allwinner Technology. All Rights Reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <time.h>
#include <linux/fb.h>
#include <linux/input.h>
#include "drv_display.h"
#include "dragonboard_inc.h"

#define TVD_HEIGHT_NTSC			480             /*30fps*/
#define TVD_HEIGHT_PAL			576             /*25fps*/
#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer {
    void   *start;
    size_t length;
};

struct size {
	int width;
	int height;
};

struct test_layer_info {
	int screen_id;
	int layer_id;
	int mem_id;
	disp_layer_config layer_config;
	int addr_map;
	int width,height;//screen size
	int dispfh;//device node handle
	int fh;//picture resource file handle
	int mem;
	int clear;//is clear layer
	char filename[32];
    int full_screen;
};

struct disp_screen {
	int x;
	int y;
	int w;
	int h;
};

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

//default paras for display
static struct options_t option = {
	.devpath      = "/dev/video4",      
	.video_width  = 640,
	.video_height = 480,
	.video_format = DISP_FORMAT_YUV420_SP_VUVU,
	.debug        = 0,

	.x = 0,
	.y = 0,
	.screen_width  = 400,
	.screen_height = 576,

	.zorder = 12,
	.layer  = 0,
	.interval_time = 6,
};

static int fd;
static struct buffer *buffers = NULL;
static int nbuffers = 0;
//static struct size input_size;
static struct size disp_size;
struct test_layer_info test_info;

static struct disp_screen get_disp_screen(int w1, int h1, int w2, int h2)
{
	struct disp_screen screen;
	float r1,r2;
	r1 = (float)w1/(float)w2;
	r2 = (float)h1/(float)h2;
	if(r1 < r2){
		screen.w = w2*r1;
		screen.h = h2*r1;
	}else{
		screen.w = w2*r2;
		screen.h = h2*r2;
	}

	screen.x = (w1 - screen.w)/2;
	screen.y = (h1 - screen.h)/2;

	return screen;
}

static void  disp_init(void)
{
	unsigned int arg[6];
	int screen_id = 0;
	disp_output_type output_type;

	memset(&test_info, 0, sizeof(struct test_layer_info));

	if((test_info.dispfh = open("/dev/disp",O_RDWR)) == -1) {
		db_error("open display device fail!\n");
		return -1;
	}

	for (screen_id = 0;screen_id < 3;screen_id++){
		arg[0] = screen_id;
		output_type = (disp_output_type)ioctl(test_info.dispfh, DISP_GET_OUTPUT_TYPE, (void*)arg);
		if(output_type != DISP_OUTPUT_TYPE_NONE){
			db_debug("the output type: %d\n",screen_id);
			break;
		}
	}

	test_info.full_screen = 0;
	test_info.screen_id = screen_id; //0 for lcd ,1 for hdmi
	test_info.layer_config.channel = 0;
	test_info.layer_config.layer_id = 0;
	test_info.layer_config.info.zorder = 1;
	test_info.layer_config.info.alpha_mode       = 1; //global alpha
	test_info.layer_config.info.alpha_value      = 0xff;
	test_info.width = ioctl(test_info.dispfh,DISP_GET_SCN_WIDTH,(void*)arg);	//get screen width and height
	test_info.height = ioctl(test_info.dispfh,DISP_GET_SCN_HEIGHT,(void*)arg);
	test_info.layer_config.info.mode = LAYER_MODE_BUFFER;
	test_info.layer_config.info.fb.format = option.video_format;//DISP_FORMAT_YUV420_SP_UVUV;
	
	db_debug("screen width: %d, height: %d\n",test_info.width,test_info.height);
}

static int disp_quit(void)
{
	int ret;
	unsigned int arg[6];
	test_info.layer_config.enable = 0;
	arg[0] = test_info.screen_id;
	arg[1] = (int)&test_info.layer_config;
	arg[2] = 0;
	ret = ioctl(test_info.dispfh, DISP_LAYER_SET_CONFIG, (void*)arg);
	if(0 != ret)
		db_error("fail to set layer info\n");

	close(test_info.dispfh);
	memset(&test_info, 0, sizeof(struct test_layer_info));
	
	return 0;
}

static int disp_set_addr(int width, int height, unsigned int *addr)

{
	unsigned int arg[6];
	int ret;


	if (test_info.full_screen == 0){
		test_info.layer_config.info.screen_win.x = option.x;
		test_info.layer_config.info.screen_win.y = option.y;
		test_info.layer_config.info.screen_win.width    = option.screen_width;
		test_info.layer_config.info.screen_win.height   = option.screen_height;
	}else{
		struct disp_screen screen;
		screen = get_disp_screen(test_info.width,test_info.height,width,height);
		test_info.layer_config.info.screen_win.x = 0;//screen.x;
		test_info.layer_config.info.screen_win.y =0; //screen.y;
		test_info.layer_config.info.screen_win.width    = 1280;//screen.w;
		test_info.layer_config.info.screen_win.height   = 800;//screen.h;
		db_debug("x: %d, y: %d, w: %d, h: %d\n",screen.x,screen.y,screen.w,screen.h);
	}

	test_info.layer_config.info.fb.size[0].width  = width;
	test_info.layer_config.info.fb.size[0].height = height;

	test_info.layer_config.info.fb.size[1].width  = width/2;
	test_info.layer_config.info.fb.size[1].height = height/2;

	test_info.layer_config.info.fb.size[2].width  = 0;
	test_info.layer_config.info.fb.size[2].height = 0;

	test_info.layer_config.info.fb.crop.width  = (unsigned long long)width  << 32;
	test_info.layer_config.info.fb.crop.height = (unsigned long long)height << 32;

	test_info.layer_config.info.fb.addr[0] = (*addr);
	test_info.layer_config.info.fb.addr[1] = (test_info.layer_config.info.fb.addr[0] + width*height);
	test_info.layer_config.info.fb.addr[2] = 0;

	test_info.layer_config.enable = 1;

	arg[0] = test_info.screen_id;
	arg[1] = (int)&test_info.layer_config;
	arg[2] = 1;
	ret = ioctl(test_info.dispfh, DISP_LAYER_SET_CONFIG, (void*)arg);
	if(0 != ret)
		db_error("disp_set_addr fail to set layer info\n");
	return 0;
}

static int read_frame(void)
{
	struct v4l2_buffer buf;
	static int count = 0;
	int buf_size[3]={0};
	char fdstr[30];
	void *bfstart = NULL;
	FILE *file_fd = NULL;
	static char path_name[] = "/mnt";
	int i, num;
	int mode = 0;
	buf_size[0] = 720*TVD_HEIGHT_PAL*3/2;

	memset(&buf, 0, sizeof(struct v4l2_buffer));

	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	ioctl(fd, VIDIOC_DQBUF, &buf);

    if(option.debug)
	{
		if (count != 40)
			count++;
		
		if (count == 30) {
			
			db_debug("file length = %d\n", buffers[buf.index].length);
			db_debug("file start = %p\n", buffers[buf.index].start);
		
			num = (mode > 2) ? 2 : mode;
			bfstart = buffers[buf.index].start;
		
			for (i = 0; i <= num; i++) {
				db_debug("file %d start = %p\n", i, bfstart);
		
				sprintf(fdstr, "%s/fb%d_y%d.bin", path_name, i + 1, mode);
				file_fd = fopen(fdstr, "w");
				fwrite(bfstart, buf_size[i] * 2 / 3, 1, file_fd);
				fclose(file_fd);
		
				sprintf(fdstr, "%s/fb%d_u%d.bin", path_name, i + 1, mode);
				file_fd = fopen(fdstr, "w");
				fwrite(bfstart + buf_size[i] * 2 / 3, buf_size[i] / 6, 1, file_fd);
				fclose(file_fd);
		
				sprintf(fdstr, "%s/fb%d_v%d.bin", path_name, i + 1, mode);
				file_fd = fopen(fdstr, "w");
				fwrite(bfstart + buf_size[i] * 2 / 3 + buf_size[i] / 6, buf_size[i] / 6, 1, file_fd);
				fclose(file_fd);
		
				bfstart +=buf_size[i];
			}
		}
    }
	disp_set_addr(disp_size.width, disp_size.height, &buf.m.offset);
	ioctl(fd, VIDIOC_QBUF, &buf);
	return 1;
}

int video_mainloop(void)
{
	char dev_name[32];
	struct v4l2_format fmt;
	int i = 0;
	enum v4l2_buf_type type;
	struct  timeval    start_time;
    struct  timeval    end_time;
	fd_set fds;

	strncpy(dev_name, option.devpath, 32);
	if ((fd = open(dev_name, O_RDWR | O_NONBLOCK, 0)) < 0) {
		db_error("can't open %s(%s)\n", dev_name, strerror(errno));
		goto open_err;
	}
	
	memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;
	while (ioctl(fd, VIDIOC_G_FMT, &fmt)) {
		db_error("get signal failed.\n");
	}	

	usleep(400000);
 
	db_debug("*********image source width = %d, height = %d********\n",fmt.fmt.pix.width, fmt.fmt.pix.height);
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;
    db_debug("set fmt again!!!\n");
	if (ioctl(fd, VIDIOC_S_FMT, &fmt))
	{
		db_error("set image format failed\n");
	}

	usleep(200000);
	disp_size.width = fmt.fmt.pix.width;
	disp_size.height = fmt.fmt.pix.height;

	struct v4l2_requestbuffers req;
	CLEAR (req);
	req.count               = 5;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	 //申请缓冲，count是申请的数量，注意，释放缓冲实际在VIDIOC_STREAMOFF内完成了。
	ioctl(fd, VIDIOC_REQBUFS, &req);
	buffers = calloc(req.count, sizeof(struct buffer));
	for (nbuffers = 0; nbuffers < req.count; nbuffers++) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(struct v4l2_buffer));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = nbuffers;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            db_error("VIDIOC_QUERYBUF error\n");
            goto buffer_rel;
        }

        buffers[nbuffers].start  = mmap(NULL, buf.length,
                PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        buffers[nbuffers].length = buf.length;
        if (buffers[nbuffers].start == MAP_FAILED) {
            db_error("mmap failed\n");
            goto buffer_rel;
        }
    }

	for (i = 0; i < nbuffers; i++) {
		struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(struct v4l2_buffer));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;
		if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
			db_error("VIDIOC_QBUF error\n");
			goto unmap;
		}
	}

	disp_init();  
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
		db_error("VIDIOC_STREAMON error\n");
		goto disp_exit;
	}
    gettimeofday(&start_time,NULL);
	while (1) {
		struct timeval tv;
        int r;

		gettimeofday(&end_time,NULL);   
		if(option.interval_time > 0 && end_time.tv_sec - start_time.tv_sec > option.interval_time)
			break;
	
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		/* timeout */
		tv.tv_sec  = 2;
		tv.tv_usec = 0;

		r = select(fd + 1, &fds, NULL, NULL, &tv);
		if (r == -1) {
			if (errno == EINTR) {
	    		continue;
			}	
		db_error("select error\n");
		}

		if (r == 0) {
			db_debug("select timeout\n");
			goto stream_off;
		}

		if (read_frame()) {
		//break;
		}
	}
    FD_ZERO(&fds);
stream_off:
	db_debug("something is err, streamoff.\n");
	ioctl(fd, VIDIOC_STREAMOFF, &type);
disp_exit:
    disp_quit();
unmap:
    for (i = 0; i < nbuffers; i++) {
        munmap(buffers[i].start, buffers[i].length);
    }
buffer_rel:
    free(buffers);
    close(fd);
open_err:
    return -1;
}

static void usage(char *name)
{
	fprintf(stderr, "   Usage: %s [-d device] [-r resolution] [-p position ] [-s size] [-f format] [-v] [-h]\n", name);
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
			option.devpath = optarg;
			break;
		case 'r':
			if (optarg)
				sscanf(optarg, "%dx%d\n",
					&option.video_width, &option.video_height);
			break;
		case 'p':
			if (optarg)
				sscanf(optarg, "[%d:%d]\n",
					&option.x, &option.y);
			break;
		case 's':
			if (optarg)
				sscanf(optarg, "%dx%d\n",
					&option.screen_width, &option.screen_height);
			break;

		case 'z':
			if (optarg)
				sscanf(optarg, "%d\n", &option.zorder);
			break;

		case 'l':
			if (optarg)
				sscanf(optarg, "%d\n", &option.layer);
			break;

		case 't':
			if (optarg)
				sscanf(optarg, "%d\n", &option.interval_time);
			break;

		case 'f':
			break;
		case 'v':
			option.debug = 1;
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

	fprintf(stdout, "  devpth: %s\n", option.devpath);
	fprintf(stdout, "  width: %d\n", option.video_width);
	fprintf(stdout, "  height: %d\n", option.video_height);
	fprintf(stdout, "  position: (%d, %d)\n", option.x, option.y);
	fprintf(stdout, "  frame: (%d, %d)\n", option.screen_width, option.screen_height);
	fprintf(stdout, "  format: %d\n", option.video_format);
}

int main(int argc, char *argv[])
{	
	parse_opt(argc, argv);
	db_debug("v40 tvin test v1 version 2016.1.20\n");
    video_mainloop();
    return 0;
}

