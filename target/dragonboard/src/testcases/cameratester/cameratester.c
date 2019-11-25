/*
 * \file        cameratest.c
 * \brief       
 *
 * \version     1.0.0
 * \date        2012年06月26日
 * \author      James Deng <csjamesdeng@allwinnertech.com>
 * 
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 */

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "asm/arch/drv_display.h"
#include "dragonboard_inc.h"
#include "videodev2.h"
#define ALIGN_4K(x) (((x) + (4095)) & ~(4095))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
#define DEBUG_CAMERA
struct test_layer_info
{
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
};
static struct test_layer_info test_info;

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
	int fps;
};
static struct options_t _opt = {
	.devpath      = "/dev/video1",
	.video_width  = 640,
	.video_height = 480,
	.video_format = DISP_FORMAT_YUV420_SP_UVUV,
	.debug        = 0,

	.x = 1000,
	.y = 0,
	.screen_width  = 600,
	.screen_height = 480,

	.zorder = 1,
	.layer  = 0,
	.interval_time = 6,
	.fps = 30,
};

struct buffer
{
    void   *start;
    size_t length;
};

static int csi_format;
static int fps;
static int req_frame_num;

static struct buffer *buffers = NULL;
static int nbuffers = 0;

static disp_rectsz input_size;
static disp_rectsz display_size;

static int screen_id = 0;
static pthread_t video_tid;
static int sensor_type = 0;

int disp_set_addr(int width, int height, unsigned int *addr);

static int getSensorType(int fd)
{
	struct v4l2_control ctrl;
	struct v4l2_queryctrl qc_ctrl;

	if (fd == NULL)
	{
		return 0xFF000000;
	}

	ctrl.id = V4L2_CID_SENSOR_TYPE;
	qc_ctrl.id = V4L2_CID_SENSOR_TYPE;

	if (-1 == ioctl (fd, VIDIOC_QUERYCTRL, &qc_ctrl))
	{
		db_error("query sensor type ctrl failed");
		return -1;
	}
	ioctl(fd, VIDIOC_G_CTRL, &ctrl);
	return ctrl.value;
}

static int read_frame(int fd)
{
    struct v4l2_buffer buf;
	unsigned int phyaddr;
    memset(&buf, 0, sizeof(struct v4l2_buffer));
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    ioctl(fd, VIDIOC_DQBUF, &buf);
    
	if(sensor_type == V4L2_SENSOR_TYPE_YUV){
		phyaddr = buf.m.offset;
	}
	else if(sensor_type == V4L2_SENSOR_TYPE_RAW){
		phyaddr = buf.m.offset + ALIGN_4K(ALIGN_16B(input_size.width) * input_size.height * 3 >> 1);
	}    
    disp_set_addr(display_size.width, display_size.height, &phyaddr);
	if(_opt.debug)
	{
	 	static int a =0;
	 	int ret;
	 	if(a == 50){
	 		FILE *frame_file = NULL;
	 		frame_file = fopen("/data/frame1.yuv","wb");
	 		if (frame_file == NULL) 
	 			db_error("open /data/frame1.yuv err\n");
	 		int frameszie = display_size.width*display_size.height*1.5;
	 		void *vaddr;
	 		vaddr = buffers[buf.index].start + phyaddr - buf.m.offset; 
	 
	 		db_debug("buf.length %d, vaddr: %x\n",buf.length,(unsigned int)vaddr);		
	 		db_debug("buf.length: %d\n,vaddr: %x\n",frameszie,(unsigned int)vaddr);
	 		ret = fwrite((void*)vaddr,1,frameszie,frame_file);
	 		db_debug("fwrite ret is %d\n",ret);
	 		fclose(frame_file);
	 		a++;
	 	}
 		else a++;
    }
    ioctl(fd, VIDIOC_QBUF, &buf);
    return 1;
}

static int disp_init(int x,int y,int width,int height)

{
	unsigned int arg[6];
	memset(&test_info, 0, sizeof(struct test_layer_info));
	if((test_info.dispfh = open("/dev/disp",O_RDWR)) == -1) {
		db_error("open display device fail!\n");
		return -1;
	}
	//get current output type
	disp_output_type output_type;
	for (screen_id = 0;screen_id < 3;screen_id++){
		arg[0] = screen_id;
		output_type = (disp_output_type)ioctl(test_info.dispfh, DISP_GET_OUTPUT_TYPE, (void*)arg);
		if(output_type != DISP_OUTPUT_TYPE_NONE){
			db_debug("the output type: %d\n",screen_id);
			break;
		}
	}
	test_info.screen_id = 0; //0 for lcd ,1 for hdmi, 2 for edp
	test_info.layer_config.channel = 0;
	test_info.layer_config.layer_id = _opt.layer;
	test_info.layer_config.info.zorder = _opt.zorder;
	test_info.layer_config.info.alpha_mode       = 1; //global alpha
	test_info.layer_config.info.alpha_value      = 0xff;

	test_info.layer_config.info.screen_win.x = x;
	test_info.layer_config.info.screen_win.y = y;
	test_info.layer_config.info.screen_win.width    = width;
	test_info.layer_config.info.screen_win.height   = height;

	//mode
	test_info.layer_config.info.mode = LAYER_MODE_BUFFER;

	//data format
	test_info.layer_config.info.fb.format = _opt.video_format;
	return 0;
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
int disp_set_addr(int width, int height, unsigned int *addr)

{
	unsigned int arg[6];
	int ret;

	//source frame size
	test_info.layer_config.info.fb.size[0].width = width;
	test_info.layer_config.info.fb.size[0].height = height;
	test_info.layer_config.info.fb.size[1].width = width/2;
	test_info.layer_config.info.fb.size[1].height = height/2;
	
	// src 
	test_info.layer_config.info.fb.crop.width = (unsigned long long)width << 32;
	test_info.layer_config.info.fb.crop.height= (unsigned long long)height << 32;

	test_info.layer_config.info.fb.addr[0] = (*addr);
	test_info.layer_config.info.fb.addr[1] = (test_info.layer_config.info.fb.addr[0] + width*height);

	test_info.layer_config.enable = 1;
	arg[0] = test_info.screen_id;
	arg[1] = (int)&test_info.layer_config;
	arg[2] = 1;
	ret = ioctl(test_info.dispfh, DISP_LAYER_SET_CONFIG, (void*)arg);
	if(0 != ret)
		db_error("disp_set_addr fail to set layer info\n");
	return 0;
}

static void *video_mainloop(void)
{
    int fd;
    fd_set fds;
    struct timeval tv;
    int r;
    char dev_name[32];
    struct v4l2_input inp;
    struct v4l2_format fmt;
    struct v4l2_format sub_fmt;
    struct v4l2_streamparm parms;
    struct v4l2_requestbuffers req;
    int i;
    enum v4l2_buf_type type;

    struct  timeval    start_time;
    struct  timeval    end_time;
	sleep(1);

    snprintf(dev_name, sizeof(dev_name), _opt.devpath);
    db_debug("open %s\n", dev_name);
    if ((fd = open(dev_name, O_RDWR,S_IRWXU)) < 0) {
        db_error("can't open %s(%s)\n", dev_name, strerror(errno));
        goto open_err;
    }
    
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    
    inp.index = 0;
    db_debug("inp.index: %d\n",inp.index);
    inp.type = V4L2_INPUT_TYPE_CAMERA;

    /* set input index */
    if (ioctl(fd, VIDIOC_S_INPUT, &inp) == -1) {
        db_error("VIDIOC_S_INPUT error\n");
        goto err;
    }
    
    parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parms.parm.capture.timeperframe.numerator = 1;
    parms.parm.capture.timeperframe.denominator = fps;
    if (ioctl(fd, VIDIOC_S_PARM, &parms) == -1) {
        db_error("set frequence failed\n");
        //goto err;
    }
    /* set image format */
    memset(&fmt, 0, sizeof(struct v4l2_format));
    memset(&sub_fmt, 0, sizeof(struct v4l2_format));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = input_size.width;
    fmt.fmt.pix.height      = input_size.height;
    fmt.fmt.pix.pixelformat = csi_format;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    
 	//get sensor type
    sensor_type = getSensorType(fd);
	sensor_type = 0;
    if(sensor_type == V4L2_SENSOR_TYPE_RAW) {
        fmt.fmt.pix.subchannel = &sub_fmt;
	    fmt.fmt.pix.subchannel->width = 640;
	    fmt.fmt.pix.subchannel->height = 480;
	    fmt.fmt.pix.subchannel->pixelformat = csi_format;
	    fmt.fmt.pix.subchannel->field = V4L2_FIELD_NONE;
    }
	
    if (ioctl(fd, VIDIOC_S_FMT, &fmt)<0) {
        db_error("set image format failed\n");
        goto err;
    }
    
    db_debug("image input width #%d height #%d, diplay width #%d height %d\n", 
	input_size.width, input_size.height, display_size.width, display_size.height);
			
    input_size.width = fmt.fmt.pix.width;
	input_size.height = fmt.fmt.pix.height;
	if(sensor_type == V4L2_SENSOR_TYPE_YUV){
		display_size.width = fmt.fmt.pix.width;
		display_size.height = fmt.fmt.pix.height;
	}else if(sensor_type == V4L2_SENSOR_TYPE_RAW){
		display_size.width = fmt.fmt.pix.subchannel->width;
		display_size.height = fmt.fmt.pix.subchannel->height;
	}
			
	db_msg("image input width #%d height #%d, diplay width #%d height %d\n", 
			input_size.width, input_size.height, display_size.width, display_size.height);

    /* request buffer */
    memset(&req, 0, sizeof(struct v4l2_requestbuffers));
    req.count  = req_frame_num;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
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

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        db_error("VIDIOC_STREAMON error\n");
        goto disp_exit;
    }
    gettimeofday(&start_time,NULL);
    while (1) {
        gettimeofday(&end_time,NULL);   
        if(_opt.interval_time > 0 && end_time.tv_sec - start_time.tv_sec > _opt.interval_time)
        	break;
    	FD_ZERO(&fds);
     	FD_SET(fd, &fds);

    	// timeout 
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
     		db_error("select timeout\n");
        	goto stream_off;
    	}
            
        if (read_frame(fd)) {
            // break;
        }
    }
    FD_ZERO(&fds);
    
    if(-1==ioctl(fd, VIDIOC_STREAMOFF, &type))
    {
        db_error("vidioc_streamoff error!\n");
		return -1;
    }
    //disp_stop();
    for (i = 0; i < nbuffers; i++) {
		if(-1==munmap(buffers[i].start, buffers[i].length))
        {
			db_error("munmap error!\n");
            return -1;
        }
    }
	free(buffers);
    db_msg("close fd\n");
	if(0!= close(fd))
	{    
		db_error("close video fd error!\n");
		return -1;
	}
   return 0;
stream_off:
    ioctl(fd, VIDIOC_STREAMOFF, &type);
disp_exit:
    //disp_stop();
unmap:
    for (i = 0; i < nbuffers; i++) {
        munmap(buffers[i].start, buffers[i].length);
    }
buffer_rel:
    free(buffers);
err:
    close(fd);
open_err:
    return -1;
}

static void usage(char *name)
{
	fprintf(stderr, "	Usage: %s [-d device] [-r resolution] [-p position ] [-s size] [-f format] [-v] [-h]\n", name);
	fprintf(stderr, "   -d: Video input device, default: /dev/video0\n");
	fprintf(stderr, "   -r: Resolution of video capture, something like: 640x480\n");
	fprintf(stderr, "   -p: Position of the video, something like: [200:400]\n");
	fprintf(stderr, "   -F: set the fps of camera\n");
	fprintf(stderr, "   -s: Screen size of the video view, something like: 320x240\n");
	fprintf(stderr, "   -t: camera switch time,default 6 seconds\n");
	fprintf(stderr, "   -f: Image format\n");
	fprintf(stderr, "   -h: Print this message\n");
}

static void parse_opt(int argc, char **argv)
{
	int c;

	do {
		c = getopt(argc, argv, "d:r:f:p:s:z:l:t:F:vh");
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
			
		case 'F':
			if (optarg)
				sscanf(optarg, "%d\n", &_opt.fps);
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
	fprintf(stdout, "  width: %d\n", _opt.video_width);
	fprintf(stdout, "  height: %d\n", _opt.video_height);
	fprintf(stdout, "  position: (%d, %d)\n", _opt.x, _opt.y);
	fprintf(stdout, "  frame: (%d, %d)\n", _opt.screen_width, _opt.screen_height);
	fprintf(stdout, "  format: %d\n", _opt.video_format);
	fprintf(stdout, "  fps: %d\n", _opt.fps);
}

int main(int argc,char *argv[])
{
	parse_opt(argc,argv);
	db_debug("the window: x: %d,y: %d,width: %d,height: %d\n",_opt.x,_opt.y,_opt.video_width,_opt.video_height);

    csi_format = V4L2_PIX_FMT_NV12;
    req_frame_num = 10;

    /* 受限于带宽，默认使用480p */
    fps = _opt.fps;
    
    input_size.width = _opt.video_width;
    input_size.height = _opt.video_height;

    display_size.width = _opt.screen_width;
    display_size.height = _opt.screen_height;

    if (disp_init(_opt.x,_opt.y,_opt.screen_width,_opt.screen_height) < 0) {
        db_error("camera: disp init failed\n");
        return -1;
    }
    video_mainloop();

    video_tid = 0;
    return 0;
}
