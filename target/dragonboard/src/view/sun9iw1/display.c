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

#define HDMI_USED 1
#define CVBS_USED 0
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

int getCurrentOutputType(void){
	unsigned int args[4] = {0};
	int dispFd;

	if((dispFd = open("/dev/disp", O_RDWR)) == -1){
		return -1;
	}
	args[0] = HDMI_USED;
	int type = ioctl(dispFd, DISP_CMD_GET_OUTPUT_TYPE, args);
	if(type == DISP_OUTPUT_TYPE_NONE){
		args[0] = CVBS_USED;
		type = ioctl(dispFd, DISP_CMD_GET_OUTPUT_TYPE, args);
		close(dispFd);
		return type;
	}
	close(dispFd);
	return type;
}

int getCurrentOutputMode(void){
	unsigned int args[4] = {0};
	int dispFd;
	if((dispFd = open("/dev/disp", O_RDWR)) == -1){
		return -1;
	}
	int type = getCurrentOutputType();
	switch(type){
	case DISP_OUTPUT_TYPE_HDMI:
		args[0] = HDMI_USED;
		return ioctl(dispFd, DISP_CMD_HDMI_GET_MODE, args);
	case DISP_OUTPUT_TYPE_TV:
		args[0] = CVBS_USED;
		return ioctl(dispFd, DISP_CMD_TV_GET_MODE, args);
	default:
		return -1;
	}
}

int scaleScreen(int wPercent, int hPercent, disp_rect *srcScreen, disp_rect *dstScreen){
	dstScreen->width = (unsigned int)(srcScreen->width * wPercent * 0.01);
	dstScreen->height = (unsigned int)(srcScreen->height * hPercent * 0.01);
	dstScreen->x = (unsigned int)(srcScreen->width - dstScreen->width) >> 1;
	dstScreen->y = (unsigned int)(srcScreen->height - dstScreen->height) >> 1;
	return 0;
}

int setTvMode(int type, int mode, int wPercent, int hPercent){
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
	int dispFd;
	if((dispFd = open("/dev/disp", O_RDWR)) == -1){
		return -1;
	}
	//get layer info
	int preType = getCurrentOutputType();
	unsigned int args[4] = {0};
	disp_layer_info layer_para;
	disp_layer_info disp_layer_para;
	switch(preType){
	case DISP_OUTPUT_TYPE_HDMI:
		args[0] = HDMI_USED;
		break;
	case DISP_OUTPUT_TYPE_TV:
		args[0] = CVBS_USED;
		break;
	}
	args[1] = 0;
	args[2] = &layer_para;
	ioctl(dispFd,DISP_CMD_LAYER_GET_INFO, (void*)args);

	//switch
	switch(type){
	case DISP_OUTPUT_TYPE_TV:
		/*disable hdmi first*/
		args[0] = HDMI_USED;
		args[1] = 0;
		args[2] = 0;
		ioctl(dispFd, DISP_CMD_LAYER_DISABLE, args);
		ioctl(dispFd, DISP_CMD_HDMI_DISABLE, args);
		//enable layer0 for cvbs
		args[0] = CVBS_USED;
		args[1] = 0;
		ioctl(dispFd, DISP_CMD_LAYER_DISABLE,args);
		scaleScreen(wPercent, hPercent, &srcScreen, &(layer_para.screen_win));
		layer_para.mode = DISP_LAYER_WORK_MODE_SCALER;
		args[2] = &layer_para;
		ioctl(dispFd, DISP_CMD_LAYER_SET_INFO, (void*)args);
		ioctl(dispFd, DISP_CMD_LAYER_ENABLE, (void*)args);
		//check whether we set successfully
		args[1] = 0;
		args[2] = &disp_layer_para;
		ioctl(dispFd, DISP_CMD_LAYER_GET_INFO, (void*)args);
		db_msg("disp%d\n", args[0]);
		db_msg("scn_win.x=%d,scn_win.y=%d\n",disp_layer_para.screen_win.x, disp_layer_para.screen_win.y);
		db_msg("scn_win.width=%d,scn_win.height=%d\n", disp_layer_para.screen_win.width, disp_layer_para.screen_win.height);
		//enable the cvbs mode
		args[0] = 0;
		ioctl(dispFd, DISP_CMD_TV_OFF,args);
		args[1] = DISP_TV_MOD_PAL;
		ioctl(dispFd, DISP_CMD_TV_SET_MODE, (void*)args);
		ioctl(dispFd, DISP_CMD_TV_ON, args);
		break;
	case DISP_OUTPUT_TYPE_HDMI:
		//disable cvbs first
		args[0] = CVBS_USED;
		args[1] = 0;
		ioctl(dispFd, DISP_CMD_LAYER_DISABLE, args);
		ioctl(dispFd, DISP_CMD_TV_OFF, args);
		//enable layer0 for hdmi
		args[0] = HDMI_USED;
		args[1] = 0;
		ioctl(dispFd, DISP_CMD_LAYER_DISABLE, args);
		scaleScreen(wPercent, hPercent, &srcScreen, &(layer_para.screen_win));
		layer_para.mode = DISP_LAYER_WORK_MODE_SCALER;
		args[2] = &layer_para;
		ioctl(dispFd, DISP_CMD_LAYER_SET_INFO, (void*)args);
		ioctl(dispFd, DISP_CMD_LAYER_ENABLE, (void*)args);
		//check whether we set successfully
		args[1] = 0;
		args[2] = &disp_layer_para;
		ioctl(dispFd, DISP_CMD_LAYER_GET_INFO, (void*)args);
		db_msg("disp%d\n", args[0]);
		db_msg("scn_win.x=%d,scn_win.y=%d\n",disp_layer_para.screen_win.x, disp_layer_para.screen_win.y);
		db_msg("scn_win.width=%d,scn_win.height=%d\n", disp_layer_para.screen_win.width, disp_layer_para.screen_win.height);
		//enable the hdmi mode
		args[0] = 1;
		ioctl(dispFd, DISP_CMD_HDMI_DISABLE, args);
		args[1] = DISP_TV_MOD_720P_60HZ;
		ioctl(dispFd, DISP_CMD_HDMI_SET_MODE, (void*)args);
		ioctl(dispFd, DISP_CMD_HDMI_ENABLE, args);
		break;
	}
	//check if we set successfully
   args[0] = 1;
   disp_tv_mode nowMode = (disp_tv_mode)ioctl(dispFd, DISP_CMD_HDMI_GET_MODE, args);
   db_msg("disp 1 output mode %d\n", nowMode);
   args[0] = 0;
   nowMode = (disp_tv_mode)ioctl(dispFd, DISP_CMD_TV_GET_MODE, args);
   db_msg("disp 0 output mode %d\n", nowMode);
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
