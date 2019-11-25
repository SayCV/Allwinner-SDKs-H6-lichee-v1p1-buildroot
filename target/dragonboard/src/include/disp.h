/*
 * \file        df_view.h
 * \brief       
 *
 * \version     1.0.0
 * \date        2012年05月31日
 * \author      James Deng <csjamesdeng@allwinnertech.com>
 *
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 */

#ifndef __DISP_H__
#define __DISP_H__


typedef enum
{
	HDMI_MOD_480P,
	HDMI_MOD_720P_60HZ,
	HDMI_MOD_1080P_60HZ,
	CVBS_MOD_PAL,
	CVBS_MOD_NTSC,
};

extern int isHdmiPluged(void);
extern int getCurrentOutputType(void);
extern int setHdmiMode(int mode, int wPercent, int hPercent);
extern int setCvbsMode(int mode, int wPercent, int hPercent);

#endif /* __DISP_H__ */
