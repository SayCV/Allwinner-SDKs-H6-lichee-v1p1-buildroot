/*
 * \file        display.c
 * \brief
 *
 * \version		1.0.0
 * \date 		2014Äê10ÔÂ21ÈÕ
 * \author 		hezuyao <hezuyao@allwinnertech.com>
 *
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 * \Descriptions:
 *      create the inital version
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "drv_display.h"
#include "disp.h"
#include "dragonboard.h"

int getCurrentOutputType(void){
	unsigned int args[4] = {0};
	disp_output output;
	int dispFd;

	if((dispFd = open("/dev/disp", O_RDWR)) == -1){
		return -1;
	}
	args[0] = 0;
	args[1] = (int)&output;
	ioctl(dispFd, DISP_GET_OUTPUT, args);
	close(dispFd);
	return output.type;
}

int getCurrentOutputMode(void){
	unsigned int args[4] = {0};
	disp_output output;
	int dispFd;

	if((dispFd = open("/dev/disp", O_RDWR)) == -1){
		return -1;
	}
	args[0] = 0;
	args[1] = (int)&output;
	ioctl(dispFd, DISP_GET_OUTPUT, args);
	close(dispFd);
	return output.mode;
}

//return 1 when hdmi is pluged in, otherwise 0 when hdmi is pluged out
int isHdmiPluged(void)
{
	unsigned int args[4];
	args[0] = 0;
	int hdmiPlug = 0;
	int hdmiFd = open("/sys/class/switch/hdmi/state", O_RDONLY);
	if(hdmiFd){
		char val;
		if(read(hdmiFd, &val, 1) == 1 && val == '1'){
			hdmiPlug = 1;
		}
		close(hdmiFd);
	}
	return hdmiPlug;
}

#define TOTAL_DISP_NUM 2
int getFbInfo(int dispFd, disp_layer_config *config){
	unsigned int args[4];
	disp_output output;
	int i;

	config->channel = 1;
	config->layer_id = 0;
	for(i = 0; i < TOTAL_DISP_NUM; i++){
		args[0] = i;
		args[1] = (int)&output;
		ioctl(dispFd, DISP_GET_OUTPUT, args);
		switch(output.type){
		case DISP_OUTPUT_TYPE_LCD:
		case DISP_OUTPUT_TYPE_HDMI:
		case DISP_OUTPUT_TYPE_TV:
		case DISP_OUTPUT_TYPE_VGA:
			args[1] = (int)config;
			args[2] = 1;
			ioctl(dispFd, DISP_LAYER_GET_CONFIG, args);

			if (config->enable != 0) {
				db_msg("display: find framebuffer layer: disp %d-%d-%d\n"
						"display: framebuffer %016lx\n",
						i, config->channel, config->layer_id,
						config->info.fb.addr[0]);

				/* if bootup with cvbs, then: DE[1] <--> cvbs,
				 * we must close DE[1] first, or we can not switch
				 * cvbs to DE[0].
				 */
				args[0] = 1;
				args[1] = 0;
				args[2] = 0;
				ioctl(dispFd, DISP_DEVICE_SWITCH, args);
				break;
			}
		}
	}
}

int scaleScreen(int wPercent, int hPercent, disp_rect *srcScreen, disp_rect *dstScreen){
	dstScreen->width = (unsigned int)(srcScreen->width * wPercent * 0.01);
	dstScreen->height = (unsigned int)(srcScreen->height * hPercent * 0.01);
	dstScreen->x = (unsigned int)(srcScreen->width - dstScreen->width) >> 1;
	dstScreen->y = (unsigned int)(srcScreen->height - dstScreen->height) >> 1;
}

int setTvMode(int type, int mode, int wPercent, int hPercent){
	static disp_layer_config framebuffer;
	int drvMode = -1;
	disp_rect srcScreen;

	srcScreen.x = 0;
	srcScreen.y = 0;

	//get message from param 'mode'
	switch(mode){
	case HDMI_MOD_480P:
		drvMode = DISP_TV_MOD_480P;
		srcScreen.width = 720;
		srcScreen.height = 480;
		break;
	case HDMI_MOD_720P_60HZ:
		drvMode = DISP_TV_MOD_720P_60HZ;
		srcScreen.width = 1280;
		srcScreen.height = 720;
		break;
	case HDMI_MOD_1080P_60HZ:
		drvMode = DISP_TV_MOD_1080P_60HZ;
		srcScreen.width = 1920;
		srcScreen.height = 1080;
		break;
	case CVBS_MOD_PAL:
		drvMode = DISP_TV_MOD_PAL;
		srcScreen.width = 720;
		srcScreen.height = 576;
		break;
	case CVBS_MOD_NTSC:
		drvMode = DISP_TV_MOD_NTSC;
		srcScreen.width = 720;
		srcScreen.height = 480;
		break;
	default:
		return -1;
	}

	disp_layer_config *config = NULL;
	unsigned int args[4] = {0};
	int dispFd;
	if((dispFd = open("/dev/disp", O_RDWR)) == -1){
		return -1;
	}

	if (framebuffer.enable == 0)
		getFbInfo(dispFd, &framebuffer);

	config = &framebuffer;

	//disable other first
	args[1] = 0;
	ioctl(dispFd, DISP_BLANK, args);
	//enable channel[1]layer[0] for hdmi
	scaleScreen(wPercent, hPercent, &srcScreen, &(config->info.screen_win));
	args[1] = (int)config;
	args[2] = 1;
	ioctl(dispFd, DISP_LAYER_SET_CONFIG, args);
	//check whether we set successfully
	ioctl(dispFd, DISP_LAYER_GET_CONFIG, args);
	db_msg("disp%d\n", args[0]);
	db_msg("scn_win=(%d, %d, %d, %d)\n",config->info.screen_win.x, config->info.screen_win.y,
		config->info.screen_win.width, config->info.screen_win.height);
	//enable the mode
	args[1] = type;
	args[2] = drvMode;
	if(getCurrentOutputMode() != drvMode){

		ioctl(dispFd, DISP_DEVICE_SWITCH, args);
	}
	close(dispFd);
	return 0;
}

//set hdmi mode
//return -1 when fail, otherwise 0 when success.
int setHdmiMode(int mode, int wPercent, int hPercent){
	return setTvMode(DISP_OUTPUT_TYPE_HDMI, mode, wPercent, hPercent);
}

int setCvbsMode(int mode, int wPercent, int hPercent){
	return setTvMode(DISP_OUTPUT_TYPE_TV, mode, wPercent, hPercent);
}
