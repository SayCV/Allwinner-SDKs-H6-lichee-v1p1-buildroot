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
#include "include/videodev2.h"

char *v4l2_capability_format_string(unsigned int caps)
{
	static char caps_str[512] = {0};
	char *buf = caps_str;

	if (V4L2_CAP_VIDEO_CAPTURE & caps)
		buf += sprintf(buf, "CAPTURE ");
	if (V4L2_CAP_VIDEO_CAPTURE_MPLANE & caps)
		buf += sprintf(buf, "CAPTURE_MPLANE ");
	if (V4L2_CAP_VIDEO_OUTPUT & caps)
		buf += sprintf(buf, "OUTPUT ");
	if (V4L2_CAP_VIDEO_OUTPUT_MPLANE & caps)
		buf += sprintf(buf, "OUTPUT_MPLANE ");
	if (V4L2_CAP_VIDEO_M2M & caps)
		buf += sprintf(buf, "M2M ");
	if (V4L2_CAP_VIDEO_M2M_MPLANE & caps)
		buf += sprintf(buf, "M2M_MPLANE ");
	if (V4L2_CAP_VIDEO_OVERLAY & caps)
		buf += sprintf(buf, "OVERLAY ");
	if (V4L2_CAP_VBI_CAPTURE & caps)
		buf += sprintf(buf, "VBI_CAPTURE ");
	if (V4L2_CAP_VBI_OUTPUT & caps)
		buf += sprintf(buf, "VBI_OUTPUT ");
	if (V4L2_CAP_SLICED_VBI_CAPTURE & caps)
		buf += sprintf(buf, "SLICED_VBI_CAPTURE ");
	if (V4L2_CAP_SLICED_VBI_OUTPUT & caps)
		buf += sprintf(buf, "SLICED_VBI_OUTPUT ");
	if (V4L2_CAP_RDS_CAPTURE & caps)
		buf += sprintf(buf, "RDS_CAPTURE ");
	if (V4L2_CAP_RDS_OUTPUT & caps)
		buf += sprintf(buf, "RDS_OUTPUT ");
	if (V4L2_CAP_SDR_CAPTURE & caps)
		buf += sprintf(buf, "SDR_CAPTURE ");
	if (V4L2_CAP_TUNER & caps)
		buf += sprintf(buf, "TUNER ");
	if (V4L2_CAP_HW_FREQ_SEEK & caps)
		buf += sprintf(buf, "HW_FREQ_SEEK ");
	if (V4L2_CAP_MODULATOR & caps)
		buf += sprintf(buf, "MODULATOR ");
	if (V4L2_CAP_AUDIO & caps)
		buf += sprintf(buf, "AUDIO ");
	if (V4L2_CAP_RADIO & caps)
		buf += sprintf(buf, "RADIO ");
	if (V4L2_CAP_READWRITE & caps)
		buf += sprintf(buf, "READWRITE ");
	if (V4L2_CAP_ASYNCIO & caps)
		buf += sprintf(buf, "ASYNCIO ");
	if (V4L2_CAP_STREAMING & caps)
		buf += sprintf(buf, "STREAMING ");
	if (V4L2_CAP_EXT_PIX_FORMAT & caps)
		buf += sprintf(buf, "EXT_PIX_FORMAT ");
	if (V4L2_CAP_DEVICE_CAPS & caps)
		buf += sprintf(buf, "DEVICE_CAPS ");

	return caps_str;
}

