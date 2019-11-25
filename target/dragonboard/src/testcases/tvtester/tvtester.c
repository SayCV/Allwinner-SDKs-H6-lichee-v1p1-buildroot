/*
* \file        tvtester.c
* \brief
*
* \version     1.0.0
* \date        2014年04月15日
* \author      hezuyao <hezuyao@allwinnertech.com>
*
* Copyright (c) 2014 Allwinner Technology. All Rights Reserved.
*
*/

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <asoundlib.h>
#if defined(_SUN50IW1P1)||defined(_SUN50IW2P1)||defined(_SUN50IW6P1)
#include <tinyalsa/asoundlib.h>
#endif

#include "asm/arch/drv_display.h"
#include "dragonboard_inc.h"

static int disp;
static int sound_play_stop;

#define BUF_LEN                         4096
#define FB_DEV                          "/dev/graphics/fb1"
char *buf[BUF_LEN];

static int check_audio_route()
{
	int err, i;
	snd_ctl_t *handle;
	const char *route_ctrl_name = "Speaker Function";
	snd_ctl_elem_value_t * elem_value;
	snd_ctl_elem_info_t *info;

	snd_ctl_elem_list_t elist;
	snd_ctl_elem_id_t *eid;

	snd_ctl_elem_value_alloca(&elem_value);
	snd_ctl_elem_info_alloca(&info);
	//snd_ctl_elem_value_set_numid(elem_value, 54);

	if ((err = snd_ctl_open(&handle, "hw:0", 0)) < 0) {
		  db_msg("Open control error: %s\n", snd_strerror(err));
		  goto check_audio_route_err;
	}

	db_msg("card=%d\n", handle->card);

	memset(&elist, 0, sizeof(elist));
	if (snd_ctl_elem_list(handle, &elist) < 0) {
		db_msg("snd_ctl_elem_list 1 failed\n");
		goto check_audio_route_err;
	}

	eid = calloc(elist.count, sizeof(snd_ctl_elem_id_t));
	elist.space = elist.count;
	elist.pids = eid;
	if (snd_ctl_elem_list(handle, &elist) < 0) {
		db_msg("snd_ctl_elem_list 2 failed\n");
		goto check_audio_route_err;
	}

	for (i = 0; i < elist.count; ++i) {
		info->id.numid = eid[i].numid;
		if ((err = snd_ctl_elem_info(handle, info)) < 0) {
			db_msg("Cannot find the given element from control\n");
			goto check_audio_route_err;
		}
		//db_msg("name[%d]=%s\n", i, snd_ctl_elem_info_get_name(info));

		if (!strcmp(snd_ctl_elem_info_get_name(info), route_ctrl_name)) {
			db_msg("route ctrl found!!!\n");
			break;
		}
	}

	snd_ctl_elem_value_set_numid(elem_value, info->id.numid);
	if ((err = snd_ctl_elem_read(handle,elem_value)) < 0) {
		  db_msg("snd_ctl_elem_read error: %s\n", snd_strerror(err));
		  goto check_audio_route_err;
	}

	db_msg("numid=%d\n", snd_ctl_elem_value_get_numid(elem_value));
	db_msg("name=%s\n", snd_ctl_elem_value_get_name(elem_value));

	//to set the new value
	snd_ctl_elem_value_set_enumerated(elem_value,0, 1);
	if ((err = snd_ctl_elem_write(handle,elem_value)) < 0) {
		  db_msg("snd_ctl_elem_write error: %s\n", snd_strerror(err));
		  goto check_audio_route_err;
	}

	//read it out again to check if we did set the registers.
	if ((err = snd_ctl_elem_read(handle,elem_value)) < 0) {
		  db_msg("snd_ctl_elem_read error: %s\n", snd_strerror(err));
		  goto check_audio_route_err;
	}

	db_msg("after: %d\n", snd_ctl_elem_value_get_enumerated(elem_value, 0));

	snd_ctl_close(handle);
	return 0;

check_audio_route_err:
	snd_ctl_close(handle);
	return -1;
}

void enable_codec(int hub, int codec)
{
	struct mixer *mixer;
	mixer = mixer_open(hub);
    struct mixer_ctl *ctl;
    //enable dam0->i2s2
	ctl = mixer_get_ctl_by_name(mixer, "I2S3 Src Select");
	mixer_ctl_set_value(ctl, 0, 1);

	//enable i2s2 out
	ctl = mixer_get_ctl_by_name(mixer, "I2S3OUT Switch");
	mixer_ctl_set_value(ctl, 0, 1);
	mixer_close(mixer);

	
	mixer = mixer_open(codec);

	ctl = mixer_get_ctl_by_name(mixer, "I2S Mixer DAC Volume");
	mixer_ctl_set_value(ctl, 0, 1);

	ctl = mixer_get_ctl_by_name(mixer, "DAC Mxier DAC Volume");
	mixer_ctl_set_value(ctl, 0, 1);

	ctl = mixer_get_ctl_by_name(mixer, "Right I2S Mixer I2SDACR Switch");
	mixer_ctl_set_value(ctl, 0, 1);

	ctl = mixer_get_ctl_by_name(mixer, "Left I2S Mixer I2SDACL Switch ");
	mixer_ctl_set_value(ctl, 0, 1);

	ctl = mixer_get_ctl_by_name(mixer, "Right DAC Mixer I2SDACR Switch");
	mixer_ctl_set_value(ctl, 0, 1);

	ctl = mixer_get_ctl_by_name(mixer, "Left DAC Mixer I2SDACL Switch");
	mixer_ctl_set_value(ctl, 0, 1);

	ctl = mixer_get_ctl_by_name(mixer, "Left Output Mixer DACL Switch");
	mixer_ctl_set_value(ctl, 0, 1);

	mixer_close(mixer);
}
#if defined(_SUN50IW1P1)||defined(_SUN50IW2P1)||defined(_SUN50IW6P1)
static void *sound_play(void *args)
{
	char path[256];
	int samplerate;
	int err;
	struct pcm_config config;
	struct pcm *pcm;
	struct pcm *codecpcm;
	FILE *fp;

	db_msg("tv:prepare play sound...\n");
	if (script_fetch("tv", "sound_file", (int *)path, sizeof(path) / 4)) {
		db_warn("tv:unknown sound file, use default\n");
		strcpy(path, "/dragonboard/data/test48000.pcm");
	}
	if (script_fetch("tv", "samplerate", &samplerate, 1)) {
		db_warn("tv:unknown samplerate, use default #48000\n");
		samplerate = 48000;
	}
	db_msg("tv:samplerate #%d\n", samplerate);

	if (check_audio_route() < 0) {
		db_error("tv:check_audio_route failed\n");
		pthread_exit((void *)-1);
	}

    config.channels = 2;
    config.rate = samplerate;
    config.period_size = 1024;
    config.period_count = 4;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;
#if defined(_SUN50IW6P1)
	int device_id=get_audio_device_id("sndacx00codec",13);
	if(device_id<0)
	{
		db_error("tv:get audio id failed\n");
		pthread_exit((void *)-1);
	}
	pcm = pcm_open(0, 0, PCM_OUT, &config);
	codecpcm = pcm_open(device_id, 0, PCM_OUT, &config);
	enable_codec(0, 4);
#else
	pcm = pcm_open(0, 0, PCM_OUT, &config);
#endif
    if (!pcm || !pcm_is_ready(pcm)) {
        db_error("Unable to open PCM device %u (%s)\n",
                0, pcm_get_error(pcm));
        pthread_exit((void *)-1);
    }

	db_msg("tv:open test pcm file: %s\n", path);
	fp = fopen(path, "r");
	if (fp == NULL) {
		db_error("tv:cannot open test pcm file(%s)\n", strerror(errno));
		pthread_exit((void *)-1);
	}

	db_msg("tv:play it...\n");
	while (1) {
		while (!feof(fp)) {
			if (sound_play_stop) {
				goto out;
			}

			err = fread(buf, 1, BUF_LEN, fp);
			if (err < 0) {
				db_warn("tv:read test pcm failed(%s)\n", strerror(errno));
			}
            if (pcm_write(pcm, buf, BUF_LEN)) {
                db_warn("Error playing sample\n");
                //break;
            }
			if (feof(fp)) {
				fseek(fp, 0L, SEEK_SET);
			}
		}
	}

out:
	db_msg("tv:play end...\n");
	fclose(fp);
	pcm_close(pcm);
	pcm_close(codecpcm);
	pthread_exit(0);
}

#else

static int xrun_recovery(snd_pcm_t *handle, int err)
{
	if (err == -EPIPE) {
		err = snd_pcm_prepare(handle);
	}

	if (err < 0) {
		db_warn("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
	}
	else if (err == -ESTRPIPE) {
		while ((err = snd_pcm_resume(handle)) == -EAGAIN) {
			sleep(1);

			if (err < 0) {
				err = snd_pcm_prepare(handle);
			}
			if (err < 0) {
				db_warn("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
			}
		}

		return 0;
	}

	return err;
}

// A80 donot set route in prepare, so route it here.
static void *sound_play(void *args)
{
	char path[256];
	int samplerate;
	int err;
	snd_pcm_t *playback_handle;
	snd_pcm_hw_params_t *hw_params;
	FILE *fp;

	db_msg("tv:prepare play sound...\n");
	if (script_fetch("tv", "sound_file", (int *)path, sizeof(path) / 4)) {
		db_warn("tv:unknown sound file, use default\n");
		strcpy(path, "/dragonboard/data/test48000.pcm");
	}
	if (script_fetch("tv", "samplerate", &samplerate, 1)) {
		db_warn("tv:unknown samplerate, use default #48000\n");
		samplerate = 48000;
	}
	db_msg("tv:samplerate #%d\n", samplerate);

	if (check_audio_route() < 0) {
		db_error("tv:check_audio_route failed\n");
		pthread_exit((void *)-1);
	}

	err = snd_pcm_open(&playback_handle, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		db_error("tv:cannot open audio device (%s)\n", snd_strerror(err));
		pthread_exit((void *)-1);
	}

	err = snd_pcm_hw_params_malloc(&hw_params);
	if (err < 0) {
		db_error("tv:cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
		pthread_exit((void *)-1);
	}

	err = snd_pcm_hw_params_any(playback_handle, hw_params);
	if (err < 0) {
		db_error("tv:cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
		pthread_exit((void *)-1);
	}

	err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		db_error("tv:cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
		pthread_exit((void *)-1);
	}

	err = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE);
	if (err < 0) {
		db_error("tv:cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
		pthread_exit((void *)-1);
	}

	err = snd_pcm_hw_params_set_rate(playback_handle, hw_params, samplerate, 0);
	if (err < 0) {
		db_error("tv:cannot set sample rate (%s)\n", snd_strerror(err));
		pthread_exit((void *)-1);
	}

	err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, 2);
	if (err < 0) {
		db_error("tv:cannot set channel count (%s), err = %d\n", snd_strerror(err), err);
		pthread_exit((void *)-1);
	}

    err = snd_pcm_hw_params_set_raw_flag(playback_handle, hw_params, 1);
    if (err < 0) {
        db_error("tv:cannot set raw_flag (%s), err = %d\n", snd_strerror(err), err);
        pthread_exit((void *)-1);
    }

	err = snd_pcm_hw_params(playback_handle, hw_params);
	if (err < 0) {
		db_error("tv:cannot set parameters (%s)\n", snd_strerror(err));
		pthread_exit((void *)-1);
	}

	snd_pcm_hw_params_free(hw_params);

	db_msg("tv:open test pcm file: %s\n", path);
	fp = fopen(path, "r");
	if (fp == NULL) {
		db_error("tv:cannot open test pcm file(%s)\n", strerror(errno));
		pthread_exit((void *)-1);
	}

	db_msg("tv:play it...\n");
	while (1) {
		while (!feof(fp)) {
			if (sound_play_stop) {
				goto out;
			}

			err = fread(buf, 1, BUF_LEN, fp);
			if (err < 0) {
				db_warn("tv:read test pcm failed(%s)\n", strerror(errno));
			}

			err = snd_pcm_writei(playback_handle, buf, BUF_LEN/4);
			if (err < 0) {
				err = xrun_recovery(playback_handle, err);
				if (err < 0) {
					db_warn("tv:write error: %s\n", snd_strerror(err));
				}
			}

			if (err == -EBADFD) {
				db_warn("tv:PCM is not in the right state (SND_PCM_STATE_PREPARED or SND_PCM_STATE_RUNNING)\n");
			}
			if (err == -EPIPE) {
				db_warn("tv:an underrun occurred\n");
			}
			if (err == -ESTRPIPE) {
				db_warn("tv:a suspend event occurred (stream is suspended and waiting for an application recovery)\n");
			}

			if (feof(fp)) {
				fseek(fp, 0L, SEEK_SET);
			}
		}
	}

out:
	db_msg("tv:play end...\n");
	fclose(fp);
	snd_pcm_close(playback_handle);
	pthread_exit(0);
}
#endif


extern int getCurrentOutputType(void);
extern int isHdmiPluged(void);

int main(int argc, char *argv[])
{
	int tvout_status = 0; // 1: sucessed
	int retry = 0;
	int flags = 0;
	int ret;
	pthread_t tid;
	disp_output_type output_type;
	int hdmi_status;
	int mic_activated;
	INIT_CMD_PIPE();

	init_script(atoi(argv[2]));

	disp = open("/dev/disp", O_RDWR);
	if (disp == -1) {
		db_error("tvtester: open /dev/disp failed(%s)\n", strerror(errno));
		goto err;
	}

	if(script_fetch("mic", "activated", &mic_activated,1)) {
		mic_activated = 0;
	}

	/* test main loop */
	while (1) {
		output_type = getCurrentOutputType();
		hdmi_status = isHdmiPluged();

		if (hdmi_status != 1) {
			if (retry < 3) {
				retry++;
				sleep(1);
				continue;
			}
			
			if (tvout_status == 1) {
				sleep(1);
				tvout_status = 1;
				continue;
			}

			if (!mic_activated) {
				// create sound play thread
				sound_play_stop = 0;
				ret = pthread_create(&tid, NULL, sound_play, NULL);
				if (ret != 0)  {
					db_error("tvtester: create sound play thread failed\n");
					//ioctl(disp, DISP_CMD_TV_OFF, args);
					goto err;
				}
				mic_activated = 1;
			}

			db_warn("TV: wakeup core thread to swith display mode\n");
			tvout_status = 1;
			SEND_CMD_PIPE_OK();
		} else {
			void *retval;  

			retry = 0;
			if (flags < 3) {
				flags++;        
				sleep(1);        
				continue;        
			}
			
			if(tvout_status == 1) {
				db_warn("TV: switch to other display, type[%d]\n", output_type);
			}
			
			if(mic_activated) {
				// end sound play thread 
				sound_play_stop = 1;
				db_msg("tvtester: waiting for sound play thread finish...\n");
				if (pthread_join(tid, &retval)) {
					db_error("tvtester: can't join with sound play thread\n"); 
				}
				db_msg("tvtester: sound play thread exit code #%d\n", (int)retval);
				mic_activated = 0;
			}

			tvout_status = 0;
		}
		sleep(1);
	}

err:
	SEND_CMD_PIPE_FAIL();
	close(disp);
	deinit_script();
	return -1;
}
