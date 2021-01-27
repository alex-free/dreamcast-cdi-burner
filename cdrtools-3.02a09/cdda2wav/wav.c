/* @(#)wav.c	1.14 10/01/31 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)wav.c	1.14 10/01/31 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling";

#endif
/*
 * Copyright (C) by Heiko Eissfeldt
 * Copyright (c) 2006-2010 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include "config.h"
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/unistd.h>
#include "mytype.h"
#include "byteorder.h"
#include "sndfile.h"
#include "global.h"

/*
 * ---------------------------------------------------------------------
 *  definitions for RIFF-output (from windows MMSYSTEM)
 * ---------------------------------------------------------------------
 */

typedef unsigned int FOURCC;	/* a four character code */

typedef struct CHUNKHDR {
	FOURCC		ckid;		/* chunk ID */
	unsigned int	dwSize; 	/* chunk size */
} CHUNKHDR;

/*
 * flags for 'wFormatTag' field of WAVEFORMAT
 */
#define	WAVE_FORMAT_PCM	1

/*
 * specific waveform format structure for PCM data
 */
typedef struct pcmwaveformat_tag {
	unsigned short	wFormatTag;	/* format type */
	unsigned short	nChannels;	/* number of channels (i.e. mono, stereo, etc.) */
	unsigned int	nSamplesPerSec;	/* sample rate */
	unsigned int	nAvgBytesPerSec; /* for buffer size estimate */
	unsigned short	nBlockAlign;	/* block size of data */
	unsigned short	wBitsPerSample;
} PCMWAVEFORMAT;
typedef PCMWAVEFORMAT *PPCMWAVEFORMAT;


/*
 * MMIO macros
 */
#define	mmioFOURCC(ch0, ch1, ch2, ch3) \
	((unsigned int)(unsigned char)(ch0) | \
	((unsigned int)(unsigned char)(ch1) << 8) | \
	((unsigned int)(unsigned char)(ch2) << 16) | \
	((unsigned int)(unsigned char)(ch3) << 24))

#define	FOURCC_RIFF	mmioFOURCC('R', 'I', 'F', 'F')
#define	FOURCC_LIST	mmioFOURCC('L', 'I', 'S', 'T')
#define	FOURCC_WAVE	mmioFOURCC('W', 'A', 'V', 'E')
#define	FOURCC_FMT	mmioFOURCC('f', 'm', 't', ' ')
#define	FOURCC_DATA	mmioFOURCC('d', 'a', 't', 'a')


/*
 * simplified Header for standard WAV files
 */
typedef struct WAVEHDR {
	CHUNKHDR	chkRiff;
	FOURCC		fccWave;
	CHUNKHDR	chkFmt;
	unsigned short	wFormatTag;	/* format type */
	unsigned short	nChannels;	/* number of channels (i.e. mono, stereo, etc.) */
	unsigned int	nSamplesPerSec;	/* sample rate */
	unsigned int	nAvgBytesPerSec; /* for buffer estimation */
	unsigned short	nBlockAlign;	/* block size of data */
	unsigned short	wBitsPerSample;
	CHUNKHDR	chkData;
} WAVEHDR;

#define	IS_STD_WAV_HEADER(waveHdr)	\
	(waveHdr.chkRiff.ckid == FOURCC_RIFF && \
	waveHdr.fccWave == FOURCC_WAVE && \
	waveHdr.chkFmt.ckid == FOURCC_FMT && \
	waveHdr.chkData.ckid == FOURCC_DATA && \
	waveHdr.wFormatTag == WAVE_FORMAT_PCM)

LOCAL WAVEHDR	waveHdr;

LOCAL int	_InitSound	__PR((int audio, long channels, Ulong rate,
					long nBitsPerSample,
					Ulong expected_bytes));
LOCAL int	_ExitSound	__PR((int audio, Ulong nBytesDone));
LOCAL Ulong	_GetHdrSize	__PR((void));
LOCAL Ulong	InSizeToOutSize	__PR((Ulong BytesToDo));


LOCAL int
_InitSound(audio, channels, rate, nBitsPerSample, expected_bytes)
	int	audio;
	long	channels;
	Ulong	rate;
	long	nBitsPerSample;
	Ulong	expected_bytes;
{
	Ulong	nBlockAlign = channels * ((nBitsPerSample + 7) / 8);
	Ulong	nAvgBytesPerSec = nBlockAlign * rate;
	Ulong	temp = expected_bytes +
				sizeof (WAVEHDR) - sizeof (CHUNKHDR);

	waveHdr.chkRiff.ckid	= cpu_to_le32(FOURCC_RIFF);
	waveHdr.fccWave		= cpu_to_le32(FOURCC_WAVE);
	waveHdr.chkFmt.ckid	= cpu_to_le32(FOURCC_FMT);
	waveHdr.chkFmt.dwSize	= cpu_to_le32(sizeof (PCMWAVEFORMAT));
	waveHdr.wFormatTag	= cpu_to_le16(WAVE_FORMAT_PCM);
	waveHdr.nChannels	= cpu_to_le16(channels);
	waveHdr.nSamplesPerSec	= cpu_to_le32(rate);
	waveHdr.nBlockAlign	= cpu_to_le16(nBlockAlign);
	waveHdr.nAvgBytesPerSec	= cpu_to_le32(nAvgBytesPerSec);
	waveHdr.wBitsPerSample	= cpu_to_le16(nBitsPerSample);
	waveHdr.chkData.ckid	= cpu_to_le32(FOURCC_DATA);
	waveHdr.chkRiff.dwSize	= cpu_to_le32(temp);
	waveHdr.chkData.dwSize	= cpu_to_le32(expected_bytes);

	global.md5offset = sizeof (waveHdr);

	return (write(audio, &waveHdr, sizeof (waveHdr)));
}

LOCAL int
_ExitSound(audio, nBytesDone)
	int	audio;
	Ulong	nBytesDone;
{
	Ulong	temp = nBytesDone +
				sizeof (WAVEHDR) - sizeof (CHUNKHDR);

	waveHdr.chkRiff.dwSize = cpu_to_le32(temp);
	waveHdr.chkData.dwSize = cpu_to_le32(nBytesDone);

	/*
	 * goto beginning
	 */
	if (lseek(audio, 0L, SEEK_SET) == -1) {
		return (0);
	}
	return (write(audio, &waveHdr, sizeof (waveHdr)));
}

LOCAL Ulong
_GetHdrSize()
{
	return (sizeof (waveHdr));
}

LOCAL Ulong
InSizeToOutSize(BytesToDo)
	Ulong	BytesToDo;
{
	return (BytesToDo);
}

struct soundfile wavsound =
{
	_InitSound,		/* init header method */
	_ExitSound,		/* exit header method */
	_GetHdrSize,		/* report header size method */
	(int (*) __PR((int audio,
		Uchar *buf,
		size_t BytesToDo))) write, /* get sound samples out */
	InSizeToOutSize,	/* compressed? output file size */
	0,			/* needs big endian samples */
	"WAVE"
};
