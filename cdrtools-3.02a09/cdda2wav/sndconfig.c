/* @(#)sndconfig.c	1.44 15/10/14 Copyright 1998-2004,2015 Heiko Eissfeldt, Copyright 2004-2015 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)sndconfig.c	1.44 15/10/14 Copyright 1998-2004,2015 Heiko Eissfeldt, Copyright 2004-2015 J. Schilling";
#endif

/*
 * os dependent functions
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include "config.h"
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/fcntl.h>
#include <schily/unistd.h>
#include <schily/ioctl.h>
#include <schily/select.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>


#if	defined HAVE_SOUNDCARD_H || defined HAVE_SYS_SOUNDCARD_H || \
	defined HAVE_LINUX_SOUNDCARD_H || defined HAVE_MACHINE_SOUNDCARD_H
#define	HAVE_OSS	1
#endif

#if	defined HAVE_ALSA_ASOUNDLIB_H || defined HAVE_SYS_ASOUNDLIB_H
#define	HAVE_ALSA	1
#undef	HAVE_OSS
#endif

/*
 * OpenSolaris switched to OSS in 2008, so is it really wise to
 * prefer the old sound system?
 */
#if	defined HAVE_SYS_AUDIOIO_H || defined HAVE_SUN_AUDIOIO_H
#define	HAVE_SUNSOUND	1
#undef	HAVE_ALSA
#undef	HAVE_OSS
#endif

#if	defined HAVE_WINDOWS_H && defined HAVE_MMSYSTEM_H
#undef	HAVE_WINSOUND
#endif

#if	defined HAVE_OS2_H && defined HAVE_OS2ME_H
#define	HAVE_OS2SOUND	1
#undef	HAVE_OSS
#endif

/* soundcard setup */
#if !defined HAVE_SUNSOUND
# if defined(HAVE_SOUNDCARD_H) || defined(HAVE_LINUX_SOUNDCARD_H) || \
	defined(HAVE_SYS_SOUNDCARD_H) || defined(HAVE_MACHINE_SOUNDCARD_H)
#  if defined(HAVE_SOUNDCARD_H)
#   include <soundcard.h>
#  else
#   if defined(HAVE_MACHINE_SOUNDCARD_H)
#    include <machine/soundcard.h>
#   else
#    if defined(HAVE_SYS_SOUNDCARD_H)
#	include <sys/soundcard.h>
#    else
#	if defined(HAVE_LINUX_SOUNDCARD_H)
#		include <linux/soundcard.h>
#	endif
#    endif
#   endif
#  endif
# endif
#endif

#if defined HAVE_SNDIO_H
# include <sndio.h>
#undef	SOUND_DEV
#define	SOUND_DEV SIO_DEVANY
#endif

#include "mytype.h"
#include "byteorder.h"
#include "lowlevel.h"
#include "global.h"
#include "sndconfig.h"

#ifdef	ECHO_TO_SOUNDCARD
#   if defined(HAVE_WINSOUND)
#	include <schily/windows.h>
#	include "mmsystem.h"
#   endif

#   if	defined(HAVE_OS2SOUND)
#	define	INCL_DOS
#	define	INCL_OS2MM
#	include	<os2.h>
#	define	PPFN	_PPFN
#	include	<os2me.h>
#	undef	PPFN
static unsigned long	DeviceID;

#	define	FRAGMENTS	2
/* playlist-structure */
typedef struct {
	ULONG	ulCommand;
	ULONG	ulOperand1;
	ULONG	ulOperand2;
	ULONG	ulOperand3;
} PLAYLISTSTRUCTURE;

static PLAYLISTSTRUCTURE PlayList[FRAGMENTS + 1];
static unsigned BufferInd;
#   endif /* defined __EMX__ */

static char snd_device[200] = SOUND_DEV;

int
set_snd_device(devicename)
	const char	*devicename;
{
	strncpy(snd_device, devicename, sizeof (snd_device));
	return (0);
}

#   if	defined(HAVE_WINSOUND)
static HWAVEOUT	DeviceID;
#	define	WAVEHDRS	3
static WAVEHDR	wavehdr[WAVEHDRS];
static unsigned	lastwav = 0;
static unsigned	wavehdrinuse = 0;
static HANDLE	waveOutEvent;

static int check_winsound_caps __PR((int bits, double rate, int channels));

static int
check_winsound_caps(bits, rate, channels)
	int	bits;
	double	rate;
	int	channels;
{
	int		result = 0;
	WAVEOUTCAPS	caps;

	/*
	 * get caps
	 */
	if (waveOutGetDevCaps(0, &caps, sizeof (caps))) {
		errmsgno(EX_BAD, _("Cannot get soundcard capabilities!\n"));
		return (1);
	}

	/*
	 * check caps
	 */
	if ((bits == 8 && !(caps.dwFormats & 0x333)) ||
	    (bits == 16 && !(caps.dwFormats & 0xccc))) {
		errmsgno(EX_BAD, _("%d bits sound are not supported.\n"), bits);
		result = 2;
	}

	if ((channels == 1 && !(caps.dwFormats & 0x555)) ||
	    (channels == 2 && !(caps.dwFormats & 0xaaa))) {
		errmsgno(EX_BAD,
			_("%d sound channels are not supported.\n"), channels);
		result = 3;
	}

	if ((rate == 44100.0 && !(caps.dwFormats & 0xf00)) ||
	    (rate == 22050.0 && !(caps.dwFormats & 0xf0)) ||
	    (rate == 11025.0 && !(caps.dwFormats & 0xf))) {
		errmsgno(EX_BAD,
			_("%d sample rate is not supported.\n"), (int)rate);
		result = 4;
	}

	return (result);
}

static void CALLBACK waveOutProc __PR((HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2));

static void CALLBACK waveOutProc(hwo, uMsg, dwInstance, dwParam1, dwParam2)
	HWAVEOUT hwo;
	UINT	uMsg;
	DWORD	dwInstance;
	DWORD	dwParam1;
	DWORD	dwParam2;
{
	if (uMsg == WOM_DONE) {
		if (wavehdrinuse) {
			wavehdrinuse--;
			SetEvent(waveOutEvent);
		}
	}
}

#   endif /* defined HAVE_WINSOUND */
#endif /* defined ECHO_TO_SOUNDCARD */

#ifdef	HAVE_SUN_AUDIOIO_H
# include <sun/audioio.h>
#endif
#ifdef	HAVE_SYS_AUDIOIO_H
# include <sys/audioio.h>
#endif

#ifdef	HAVE_ALSA
#ifdef	HAVE_SYS_ASOUNDLIB_H
# include <sys/asoundlib.h>
#endif
#ifdef	HAVE_ALSA_ASOUNDLIB_H
# include <alsa/asoundlib.h>
#endif
snd_pcm_t	*pcm_handle;
unsigned frame_factor;
#endif

#if	defined	HAVE_SNDIO_H
struct	sio_hdl *hdl;
#endif

#if	defined	HAVE_OSS && defined SNDCTL_DSP_GETOSPACE
audio_buf_info abinfo;
#endif

#if	defined HAVE_PULSE_PULSEAUDIO_H
# include	<pulse/pulseaudio.h>

#define	PULSEAUDIO_SIMPLE_API	1
/*#undef	PULSEAUDIO_SIMPLE_API*/

#ifdef	PULSEAUDIO_SIMPLE_API
# include	<pulse/simple.h>
pa_simple	*ptr_pa_simple = NULL;
#else
pa_mainloop	*ptr_pa_mainloop = NULL;
pa_mainloop_api	*ptr_pa_mainloop_api = NULL;
pa_context	*ptr_pa_context = NULL;
pa_stream	*ptr_pa_stream = NULL;
#endif

struct pa_sample_spec	_pa_sample_spec;

#endif



int
init_soundcard(rate, bits)
	double	rate;
	int	bits;
{
#ifdef	ECHO_TO_SOUNDCARD
	if (global.echo) {
#if	defined HAVE_PULSE_PULSEAUDIO_H
		int ret = 1;
		if (bits != 16 && bits != 8) {
			error("Cannot use pulseaudio sound device with %d bits per sample\n",
				bits);
			return (1);
		}
		/* setup format */
		_pa_sample_spec.format = bits == 16 ? PA_SAMPLE_S16LE
				: PA_SAMPLE_U8;
		_pa_sample_spec.channels = 2;
		_pa_sample_spec.rate = rate;

#ifdef	PULSEAUDIO_SIMPLE_API
		ptr_pa_simple = pa_simple_new(
			NULL,				/* default server */
			pa_locale_to_utf8("Cdda2wav"),	/* application name */
			PA_STREAM_PLAYBACK,		/* sound transfer direction */
			NULL,				/* default device */
			pa_locale_to_utf8("Music"),	/* stream description */
			&_pa_sample_spec,		/* sample format */
			NULL,				/* default channel map */
			NULL,				/* default buffering attributes */
			&ret);				/* error code */

		if (ptr_pa_simple == NULL) {
#ifdef	SND_DEBUG
			error("Cannot use pulseaudio sound daemon (no connected object): %d. Trying native sound device...\n",
				ret);
#endif
			goto pa_outsimple;
		}
#else
		/* get a mainloop */
		ptr_pa_mainloop = pa_mainloop_new();
		if (ptr_pa_mainloop == NULL) {
			error("Cannot use pulseaudio sound daemon (no mainloop). Trying native sound device...\n");
			goto pa_out0;
		}

		ptr_pa_mainloop_api = pa_mainloop_get_api(ptr_pa_mainloop);


		/* get a context */
		ptr_pa_context = pa_context_new(
			ptr_pa_mainloop_api,
			pa_locale_to_utf8("Cdda2wav_ctx"));

		if (ptr_pa_context == NULL) {
			error("Cannot use pulseaudio sound daemon (no context). Trying native sound device...\n");
			goto pa_out1;
		}

		/* connect a context */
		if (pa_context_connect(
		    ptr_pa_context,
		    NULL,		/* default server */
		    0,			/* flags */
		    NULL) < 0 {		/* spawn API */
#ifdef	SND_DEBUG
			error("Cannot open pulseaudio sound device (no context connect). Trying native sound device...\n");
#endif
			goto pa_out2;
		}

		int state = 0;
		do {
			if (pa_mainloop_iterate(ptr_pa_mainloop, 1, NULL) < 0) {
				error(
				"Cannot open pulseaudio sound device (mainloop_iterate failed). Trying native sound device...\n");
				goto pa_out2;
			}
			state = pa_context_get_state(ptr_pa_context);
			if (state != PA_CONTEXT_CONNECTING &&
			    state != PA_CONTEXT_AUTHORIZING &&
			    state != PA_CONTEXT_SETTING_NAME &&
			    state != PA_CONTEXT_READY) {
				error(
				"Cannot open pulseaudio sound device (bad context state %d). Trying native sound device...\n",
				state);
				goto pa_out2;
			}
		} while (state != PA_CONTEXT_READY);

		pa_channel_map pacmap;
		pa_channel_map_init_auto(&pacmap, _pa_sample_spec.channels,
					PA_CHANNEL_MAP_WAVEEX);

		ptr_pa_stream = pa_stream_new(
			ptr_pa_context,
			pa_locale_to_utf8("Cdda2wav"),
			&_pa_sample_spec,
			&pacmap);

		if (ptr_pa_stream == NULL) {
			error("Cannot use pulseaudio sound daemon (no stream). Trying native sound device...\n");
			goto pa_out2;
		}

		if (pa_stream_connect_playback(
		    ptr_pa_stream,
		    NULL,
		    NULL,
		    0,
		    NULL,
		    NULL) < 0) {
			error("Cannot use pulseaudio sound daemon (no connect). Trying native sound device...\n");
			goto pa_out2;
		}

		state = 0;
		do {
			if (pa_mainloop_iterate(ptr_pa_mainloop, 1, NULL) < 0) {
				error(
				"Cannot open pulseaudio sound device (mainloop_iterate failed). Trying native sound device...\n");
				goto pa_out2;
			}
			state = pa_stream_get_state(ptr_pa_stream);
			if (state != PA_STREAM_CREATING &&
			    state != PA_STREAM_READY) {
				error(
				"Cannot open pulseaudio sound device (bad stream state %d). Trying native sound device...\n",
				state);
				goto pa_out2;
			}
		} while (state != PA_STREAM_READY);

		return (0);
		/* if unsuccessful, fallback to native sound API */

		/* clean up */
pa_out2:
		if (ptr_pa_context)
			pa_context_unref(ptr_pa_context);
pa_out1:
		if (ptr_pa_mainloop)
			pa_mainloop_free(ptr_pa_mainloop);
pa_out0:
		ptr_pa_stream = NULL;
		ptr_pa_context = NULL;
		ptr_pa_mainloop = NULL;
		ptr_pa_mainloop_api = NULL;
#endif
pa_outsimple:
		;
#endif
# if	defined(HAVE_SNDIO_H)
		struct	sio_par par;
		hdl = sio_open(snd_device, SIO_PLAY, 0);
		if (hdl == NULL) {
			errmsg("Cannot open sndio sound device '%s'.\n", snd_device);
			return (1);
		}
		sio_initpar(&par);
		par.bits = bits;
		par.sig = 1;
		par.le = SIO_LE_NATIVE;
		par.pchan = 2;
		par.rate = rate;
		par.appbufsz = 44100 / 4; /* 61152 */
		if (!sio_setpar(hdl, &par) || !sio_getpar(hdl, &par)) {
			errmsg("Cannot set sound parameters for '%s'.\n", snd_device);
			sio_close(hdl);
			hdl = NULL;
			return (1);
		}
		if (par.bits != bits || par.sig != 1 || par.le != SIO_LE_NATIVE ||
		    par.pchan != 2 || par.rate != (int)rate) {
			errmsg("Unsupported sound parameters for '%s'.\n", snd_device);
			sio_close(hdl);
			hdl = NULL;
			return (1);
		}
		if (!sio_start(hdl)) {
			errmsg("Couldn't start sound device '%s'.\n", snd_device);
			sio_close(hdl);
			hdl = NULL;
			return (1);
		}
# else
#  if	defined(HAVE_OSS) && HAVE_OSS == 1
		if (open_snd_device() != 0) {
			errmsg(_("Cannot open oss sound device '%s'.\n"), snd_device);
			global.echo = 0;
		} else {
			/*
			 * This the sound device initialisation for 4front Open sound drivers
			 */
			int	dummy;
			int	garbled_rate = rate;
			int	stereo = (global.channels == 2);
			int	myformat = bits == 8 ? AFMT_U8 :
					(MY_LITTLE_ENDIAN ?
					AFMT_S16_LE : AFMT_S16_BE);
			int	mask;

			if (ioctl(global.soundcard_fd,
			    SNDCTL_DSP_GETBLKSIZE, &dummy) == -1) {
				errmsg(_("Cannot get blocksize for %s.\n"),
					snd_device);
				global.echo = 0;
			}
			if (ioctl(global.soundcard_fd,
			    SNDCTL_DSP_SYNC, NULL) == -1) {
				errmsg(_("Cannot sync for %s.\n"),
					snd_device);
				global.echo = 0;
			}

#if	defined SNDCTL_DSP_GETOSPACE
			if (ioctl(global.soundcard_fd,
			    SNDCTL_DSP_GETOSPACE, &abinfo) == -1) {
				errmsg(_("Cannot get input buffersize for %s.\n"),
					snd_device);
				abinfo.fragments  = 0;
			}
#endif

			/*
			 * check, if the sound device can do the
			 * requested format
			 */
			if (ioctl(global.soundcard_fd,
			    SNDCTL_DSP_GETFMTS, &mask) == -1) {
				errmsg(_("Fatal error in ioctl(SNDCTL_DSP_GETFMTS).\n"));
				return (-1);
			}
			if ((mask & myformat) == 0) {
				errmsgno(EX_BAD,
				_("Sound format (%d bits signed) is not available.\n"),
				bits);
				if ((mask & AFMT_U8) != 0) {
					bits = 8;
					myformat = AFMT_U8;
				}
			}
			if (ioctl(global.soundcard_fd,
			    SNDCTL_DSP_SETFMT, &myformat) == -1) {
				errmsg(_("Cannot set %d bits/sample for %s.\n"),
					bits, snd_device);
			    global.echo = 0;
			}

			/*
			 * limited sound devices may not support stereo
			 */
			if (stereo &&
			    ioctl(global.soundcard_fd,
			    SNDCTL_DSP_STEREO, &stereo) == -1) {
				errmsg(_("Cannot set stereo mode for %s.\n"),
					snd_device);
				stereo = 0;
			}
			if (!stereo &&
			    ioctl(global.soundcard_fd,
			    SNDCTL_DSP_STEREO, &stereo) == -1) {
				errmsg(_("Cannot set mono mode for %s.\n"),
					snd_device);
				global.echo = 0;
			}

			/*
			 * set the sample rate
			 */
			if (ioctl(global.soundcard_fd,
			    SNDCTL_DSP_SPEED, &garbled_rate) == -1) {
				errmsg(_("Cannot set rate %d.%2d Hz for %s.\n"),
					(int)rate, (int)(rate*100)%100,
					snd_device);
				global.echo = 0;
			}
			if (abs((long)rate - garbled_rate) > rate / 20) {
				errmsgno(EX_BAD,
				_("Sound device: next best sample rate is %d.\n"),
				garbled_rate);
			}
		}

#  else /* HAVE_OSS */

#   if defined	HAVE_SYS_AUDIOIO_H || defined HAVE_SUN_AUDIOIO_H
		/*
		 * This is the SunOS / Solaris / NetBSD
		 * sound initialisation
		 */
		if ((global.soundcard_fd = open(snd_device, O_WRONLY, 0)) ==
		    EOF) {
			errmsg(_("Cannot open '%s'.\n"), snd_device);
			global.echo = 0;
		} else {
#    if	defined(AUDIO_INITINFO) && defined(AUDIO_ENCODING_LINEAR)
			audio_info_t	info;

			AUDIO_INITINFO(&info);
			info.play.sample_rate = rate;
			info.play.channels = global.channels;
			info.play.precision = bits;
			info.play.encoding = AUDIO_ENCODING_LINEAR;
			info.play.pause = 0;
			info.record.pause = 0;
			info.monitor_gain = 0;
			if (ioctl(global.soundcard_fd, AUDIO_SETINFO, &info) <
			    0) {
				errmsg(_("Cannot init %s (sun).\n"),
					snd_device);
				global.echo = 0;
			}
#    else
			errmsgno(EX_BAD,
			_("Cannot init sound device with %u.%u kHz sample rate on %s (sun compatible).\n"),
			rate / 1000, (rate % 1000) / 100,
			snd_device);
			global.echo = 0;
#    endif
		}
#   else /* SUN audio */
#    if defined(HAVE_WINSOUND)
		/*
		 * Windows sound info
		 */
		MMRESULT	mmres;
		WAVEFORMATEX	wavform;

		if (waveOutGetNumDevs() < 1) {
			errmsgno(EX_BAD, _("No sound devices available!\n"));
			global.echo = 0;
			return (1);
		}

		/*
		 * check capabilities
		 */
		if (check_winsound_caps(bits, rate, global.channels) != 0) {
			errmsgno(EX_BAD,
			_("Soundcard capabilities are not sufficient!\n"));
			global.echo = 0;
			return (1);
		}

		wavform.wFormatTag = WAVE_FORMAT_PCM;
		wavform.nChannels = global.channels;
		wavform.nSamplesPerSec = (int)rate;
		wavform.wBitsPerSample = bits;
		wavform.cbSize = sizeof (wavform);
		wavform.nAvgBytesPerSec = (int)rate * global.channels *
						(wavform.wBitsPerSample / 8);
		wavform.nBlockAlign = global.channels * (wavform.wBitsPerSample / 8);

		waveOutEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		DeviceID = 0;
		mmres = waveOutOpen(&DeviceID, WAVE_MAPPER, &wavform,
			(uint32_t)waveOutProc, 0, CALLBACK_FUNCTION);
		if (mmres) {
			char	erstr[329];

			waveOutGetErrorText(mmres, erstr, sizeof (erstr));
			errmsgno(EX_BAD,
				_("Soundcard open error: %s!\n"), erstr);
			global.echo = 0;
			return (1);
		}

		global.soundcard_fd = 0;

		/*
		 * init all wavehdrs
		 */
		{ int	i;

			for (i = 0; i < WAVEHDRS; i++) {
				wavehdr[i].dwBufferLength = (global.channels*(bits/ 8)*(int)rate*
							global.nsectors)/75;
				wavehdr[i].lpData = malloc(wavehdr[i].dwBufferLength);
				if (wavehdr[i].lpData == NULL) {
					errmsg(
					_("No memory for sound buffers available.\n"));
					waveOutReset(0);
					CloseHandle(waveOutEvent);
					waveOutClose(DeviceID);
					return (1);
				}
				mmres = waveOutPrepareHeader(DeviceID,
						&wavehdr[i], sizeof (WAVEHDR));
				if (mmres) {
					char	erstr[129];

					waveOutGetErrorText(mmres, erstr,
							sizeof (erstr));
					errmsgno(EX_BAD,
					_("soundcard prepare error: %s!\n"),
						erstr);
					return (1);
				}

				wavehdr[i].dwLoops = 0;
				wavehdr[i].dwFlags = WHDR_DONE;
				wavehdr[i].dwBufferLength = 0;
			}
		}

#    else
#	if defined(HAVE_OS2SOUND)
#	if defined(HAVE_MMPM)
		/*
		 * OS/2 MMPM/2 MCI sound info
		 */
		MCI_OPEN_PARMS	mciOpenParms;
		int		i;

		/*
		 * create playlist
		 */
		for (i = 0; i < FRAGMENTS; i++) {
			PlayList[i].ulCommand = DATA_OPERATION;	/* play data */
			PlayList[i].ulOperand1 = 0;		/* address */
			PlayList[i].ulOperand2 = 0;		/* size */
			PlayList[i].ulOperand3 = 0;		/* offset */
		}
		PlayList[FRAGMENTS].ulCommand = BRANCH_OPERATION; /* jump */
		PlayList[FRAGMENTS].ulOperand1 = 0;
		PlayList[FRAGMENTS].ulOperand2 = 0;		/* destination */
		PlayList[FRAGMENTS].ulOperand3 = 0;

		memset(&mciOpenParms, 0, sizeof (mciOpenParms));
		mciOpenParms.pszDeviceType = (PSZ) (((unsigned long) MCI_DEVTYPE_WAVEFORM_AUDIO << 16) | \
						(unsigned short) DeviceIndex);
		mciOpenParms.pszElementName = (PSZ) & PlayList;

		/*
		 * try to open the sound device
		 */
		if (mciSendCommand(0, MCI_OPEN,
			MCI_WAIT | MCI_OPEN_SHAREABLE | MCIOPEN_Type_ID,
							&mciOpenParms, 0)
				!= MCIERR_SUCCESS) {
			/*
			 * no sound
			 */
			errmsgno(EX_BAD, _("No sound devices available!\n"));
			global.echo = 0;
			return (1);
		}
		/*
		 * try to set the parameters
		 */
		DeviceID = mciOpenParms.usDeviceID;

		{
			MCI_WAVE_SET_PARMS	mciWaveSetParms;

			memset(&mciWaveSetParms, 0, sizeof (mciWaveSetParms));
			mciWaveSetParms.ulSamplesPerSec = rate;
			mciWaveSetParms.usBitsPerSample = bits;
			mciWaveSetParms.usChannels = global.channels;
			mciWaveSetParms.ulAudio = MCI_SET_AUDIO_ALL;

			/*
			 * set play-parameters
			 */
			if (mciSendCommand(DeviceID, MCI_SET,
					MCI_WAIT | MCI_WAVE_SET_SAMPLESPERSEC |
					MCI_WAVE_SET_BITSPERSAMPLE |
					MCI_WAVE_SET_CHANNELS,
					(PVOID) & mciWaveSetParms, 0)) {
				MCI_GENERIC_PARMS	mciGenericParms;

				errmsgno(EX_BAD,
				_("Soundcard capabilities are not sufficient!\n"));
				global.echo = 0;
				/*
				 * close
				 */
				mciSendCommand(DeviceID, MCI_CLOSE, MCI_WAIT,
							&mciGenericParms, 0);
				return (1);
			}
		}

#	endif /* EMX MMPM OS2 sound */
#	else
#	if defined(HAVE_ALSA)
		int		card = -1;
		int		rtn;

/*error("setting ALSA device: '%s'.\n", snd_device);*/
		rtn = snd_pcm_open(&pcm_handle,
			snd_device, SND_PCM_STREAM_PLAYBACK, 0);
		if (rtn < 0) {
			errmsg(_("Error opening ALSA sound device (%s).\n"), snd_strerror(rtn));
			return (1);
		}

		if ((rtn = snd_pcm_set_params(
			pcm_handle,
			bits == 8 ? SND_PCM_FORMAT_U8 : SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			global.channels,
			rate,
			1,
			500000)) < 0) {
			error("Error setting ALSA parameters: %s.\n",
				snd_strerror(rtn));
			return (1);
		}

		frame_factor = ((bits == 8 ? 1 : 2) * global.channels);


#	endif /* QNX sound */
#	endif /* EMX OS2 sound */
#	endif /* CYGWIN Windows sound */
#   endif /* else SUN audio */
#  endif /* else HAVE_OSS */
# endif /* else HAVE_SNDIO_H */
	}
#endif /* ifdef ECHO_TO_SOUNDCARD */
	return (0);
}

int
open_snd_device()
{
#if	defined ECHO_TO_SOUNDCARD && !defined HAVE_WINSOUND && !defined HAVE_OS2SOUND
#if	defined(F_GETFL) && defined(F_SETFL) && defined(O_NONBLOCK)
	int	fl;
#endif

	global.soundcard_fd = open(snd_device,
#ifdef	linux
		/*
		 * Linux BUG: the sound driver open() blocks,
		 * if the device is in use.
		 */
		O_NONBLOCK |
#endif
		O_WRONLY, 0);

#if	defined(F_GETFL) && defined(F_SETFL) && defined(O_NONBLOCK)
	fl = fcntl(global.soundcard_fd, F_GETFL, 0);
	fl &= ~O_NONBLOCK;
	fcntl(global.soundcard_fd, F_SETFL, fl);
#endif

	return (global.soundcard_fd < 0);

#else	/* def ECHO_TO_SOUNDCARD && !def __CYGWIN32__ && !def __CYGWIN32__ && !def __MINGW32__ && !def __EMX__ */
	return (0);
#endif
}

int
close_snd_device()
{
#if	!defined ECHO_TO_SOUNDCARD
	return (0);
#else

#if	defined HAVE_PULSE_PULSEAUDIO_H
#ifdef	PULSEAUDIO_SIMPLE_API
	if (ptr_pa_simple) {
		int ret = 0;
		if (pa_simple_drain(ptr_pa_simple, &ret) < 0) {
			errmsg(_("Soundcard pulse audio drain error: %d!\n"), ret);
		}
		pa_simple_free(ptr_pa_simple);
		ptr_pa_simple = NULL;
		return (0);
	}
#else
	if (ptr_pa_stream) {
		pa_operation * ptr_op =
			pa_stream_drain(
				ptr_pa_stream,		/* stream */
				NULL,			/* callback */
				NULL);			/* user data */
		if (ptr_op == NULL) {
			errmsg(_("Soundcard pulse audio drain error!\n"));
		}
		while (pa_operation_get_state(ptr_op) != PA_OPERATION_DONE) {
			if (pa_context_get_state(ptr_pa_context) != PA_CONTEXT_READY ||
			    pa_stream_get_state(ptr_pa_stream) != PA_STREAM_READY ||
			    pa_mainloop_iterate(ptr_pa_mainloop, 1, NULL) < 0) {
				pa_operation_cancel(ptr_op);
				break;
			}
		}
		pa_operation_unref(ptr_op);

		pa_stream_disconnect(ptr_pa_stream);
		pa_stream_unref(ptr_pa_stream);

		if (ptr_pa_context)
			pa_context_unref(ptr_pa_context);
		if (ptr_pa_mainloop) {
			pa_signal_done();
			pa_mainloop_free(ptr_pa_mainloop);
		}
		ptr_pa_stream = NULL;
		ptr_pa_context = NULL;
		ptr_pa_mainloop = NULL;
		ptr_pa_mainloop_api = NULL;
		return (0);
	}
#endif
	/* FALLTHROUGH to native API */
#endif /* !HAVE_PULSE_PULSEAUDIO_H */

# if	defined(HAVE_WINSOUND)
	waveOutReset(0);
	CloseHandle(waveOutEvent);
	return (waveOutClose(DeviceID));
# else /* !Cygwin32 */

#  if	defined HAVE_OS2SOUND
#   if	defined HAVE_MMPM
	/*
	 * close the sound device
	 */
	MCI_GENERIC_PARMS mciGenericParms;
	mciSendCommand(DeviceID, MCI_CLOSE, MCI_WAIT, &mciGenericParms, 0);
#   else /* HAVE_MMPM */
	return (0);
#   endif /* HAVE_MMPM */
#  else /* !EMX */
#   if	defined	HAVE_ALSA
	snd_pcm_drain(pcm_handle);
	return (snd_pcm_close(pcm_handle));
#   else /* !ALSA */
#    if defined HAVE_SNDIO_H
	if (hdl != NULL) {
		sio_close(hdl);
		hdl = NULL;
	}
	return (0);
#    else
	return (close(global.soundcard_fd));
#    endif /* !HAVE_SNDIO_H */
#   endif /* !QNX */
#  endif /* !EMX */
# endif /* !Cygwin32 */
#endif /* ifdef ECHO_TO_SOUNDCARD */
}

int
write_snd_device(buffer, todo)
	char		*buffer;
	unsigned	todo;
{
	int	result = 0;
#ifdef	ECHO_TO_SOUNDCARD

#if	defined HAVE_PULSE_PULSEAUDIO_H
#ifdef	PULSEAUDIO_SIMPLE_API
	if (ptr_pa_simple) {
		int ret = pa_simple_write(
			ptr_pa_simple,		/* object */
			buffer,			/* sample data */
			todo,			/* size in bytes */
			&result);		/* error code */
		if (ret < 0) {
			errmsgno(EX_BAD,
				_("Soundcard write error (pulseaudio): %d, %s!\n"), result, pa_strerror(result));
		}
		return (0);
	}
#else
	if (ptr_pa_stream) {
		result = pa_stream_write(
			ptr_pa_stream,		/* stream */
			buffer,			/* sample data */
			todo,			/* size in bytes */
			NULL,			/* callback to free() data */
			0LL,			/* seek offset */
			PA_SEEK_RELATIVE);	/* seek mode */
		if (result < 0) {
			errmsgno(EX_BAD,
				_("Soundcard write error (pulseaudio): %d, %s!\n"), result, pa_strerror(result));
		}
		return (result);
	}
#endif
	/* FALLTHROUGH to native transport */
#endif /* defined HAVE_PULSE_PULSEAUDIO_H */

#if	defined HAVE_SNDIO_H
	if (hdl == NULL || !sio_write(hdl, buffer, todo)) {
		errmsgno(EX_BAD,
			_("Soundcard write error (sndio)!\n"));
		return (1);
	}
#else
#if	defined(HAVE_WINSOUND)
	MMRESULT	mmres;

	wavehdr[lastwav].dwBufferLength = todo;
	memcpy(wavehdr[lastwav].lpData, buffer, todo);

	while (wavehdrinuse >= WAVEHDRS) {
		WaitForSingleObject(waveOutEvent, INFINITE);
		ResetEvent(waveOutEvent);
	}
	wavehdrinuse++;
	mmres = waveOutWrite(DeviceID, &wavehdr[lastwav], sizeof (WAVEHDR));
	if (mmres) {
		char erstr[129];

		waveOutGetErrorText(mmres, erstr, sizeof (erstr));
		errmsgno(EX_BAD, _("Soundcard write error (cygwin): %s!\n"), erstr);
		return (1);
	}
	if (++lastwav >= WAVEHDRS)
		lastwav -= WAVEHDRS;
	result = mmres;
#else
#if	defined HAVE_OS2SOUND
	Playlist[BufferInd].ulOperand1 = buffer;
	Playlist[BufferInd].ulOperand2 = todo;
	Playlist[BufferInd].ulOperand3 = 0;
	if (++BufferInd >= FRAGMENTS)
		BufferInd -= FRAGMENTS;

	/*
	 * no MCI_WAIT here, because application program has to continue
	 */
	memset(&mciPlayParms, 0, sizeof (mciPlayParms));
	if (mciSendCommand(DeviceID, MCI_PLAY, MCI_FROM, &mciPlayParms, 0)) {
		errmsgno(EX_BAD, _("Soundcard write error (os/2): %s!\n"), erstr);
		return (1);
	}
	result = 0;
#else
	int retval2;
	int towrite;

#if	defined	HAVE_OSS && defined SNDCTL_DSP_GETOSPACE
	towrite = abinfo.fragments * abinfo.fragsize;
	if (towrite == 0)
#endif
		towrite = todo;
	do {
		int		wrote;
#ifdef	HAVE_SELECT
		fd_set		writefds[1];
		struct timeval	timeout2;

		timeout2.tv_sec = 0;
		timeout2.tv_usec = 4*120000;

		FD_ZERO(writefds);
		FD_SET(global.soundcard_fd, writefds);
		retval2 = select(global.soundcard_fd + 1,
				NULL, writefds, NULL, &timeout2);
		switch (retval2) {

		default:
		case -1: errmsg(_("Select failed.\n"));
			/* FALLTHROUGH */
		case 0: /* timeout */
			result = 2;
			goto outside_loop;
		case 1: break;
		}
#endif	/* HAVE_SELECT */
		if (towrite > todo) {
			towrite = todo;
		}
#if		defined HAVE_ALSA
		{
			snd_pcm_sframes_t frames = towrite / frame_factor;
			wrote = snd_pcm_writei(pcm_handle, buffer, frames);
			if (wrote < 0) {
				wrote = snd_pcm_recover(pcm_handle, wrote, 0);
			}
			wrote *= frame_factor;
		}
#else
		wrote = write(global.soundcard_fd, buffer, towrite);
#endif
		if (wrote <= 0) {
#if		defined HAVE_ALSA
			errmsg(_("Can't write audio (alsa).\n"));
#else
			errmsg(_("Can't write audio (oss).\n"));
#endif
			result = 1;
			goto outside_loop;
		} else {
			todo -= wrote;
			buffer += wrote;
		}
	} while (todo > 0);
outside_loop:
	;
#endif	/* !defined __EMX__ */
#endif	/* !defined __CYGWIN32__ */
#endif	/* !defined HAVE_SNDIO_H */
#endif	/* ECHO_TO_SOUNDCARD */
	return (result);
}
