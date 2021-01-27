/* @(#)aiff.c	1.15 16/02/14 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)aiff.c	1.15 16/02/14 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling";

#endif
/*
 * Copyright (C) by Heiko Eissfeldt
 * Copyright (c) 2006-2010 J. Schilling
 *
 * definitions for aiff pcm output
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
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "mytype.h"
#include "byteorder.h"
#include "sndfile.h"
#include "global.h"

typedef UINT4 FOURCC;    /* a four character code */
typedef struct CHUNKHDR {
	FOURCC	ckid;	/* chunk ID */
	UINT4	dwSize;	/* chunk size */
} CHUNKHDR;

#define	mmioFOURCC(ch0, ch1, ch2, ch3) \
		((UINT4)(unsigned char)(ch3) | \
		((UINT4)(unsigned char)(ch2) << 8) | \
		((UINT4)(unsigned char)(ch1) << 16) | \
		((UINT4)(unsigned char)(ch0) << 24))

#define	FOURCC_FORM	mmioFOURCC('F', 'O', 'R', 'M')
#define	FOURCC_AIFF	mmioFOURCC('A', 'I', 'F', 'F')
#define	FOURCC_COMM	mmioFOURCC('C', 'O', 'M', 'M')
#define	FOURCC_SSND	mmioFOURCC('S', 'S', 'N', 'D')

typedef struct AIFFHDR {
	CHUNKHDR	formChk;
	FOURCC		formType;

	CHUNKHDR	commChk;	/* Common chunk */
	/*
	 * from now on, alignment prevents us from using the original types :-(
	 */
	unsigned char	numChannels[2];		/* Audio Channels */
	unsigned char	numSampleFrames[4];	/* # of samples */
	unsigned char	samplesize[2];		/* bits per sample */
	unsigned char	sample_rate[10];	/* samplerate in extd. float */

	unsigned char	ssndChkid[4];	/* Sound data chunk */
	unsigned char	dwSize[4];	/* size of chunk */
	unsigned char	offset[4];	/* start of 1st sample */
	unsigned char	blocksize[4];	/* aligned sound data block size */
} AIFFHDR;

LOCAL AIFFHDR AiffHdr;

LOCAL int	Format_samplerate	__PR((Ulong rate,
						Uchar the_rate[10]));
LOCAL int	InitSound	__PR((int audio, long channels,
					Ulong rate,
					long nBitsPerSample,
					Ulong expected_bytes));
LOCAL int	ExitSound	__PR((int audio, Ulong nBytesDone));
LOCAL Ulong	GetHdrSize	__PR((void));
LOCAL Ulong	InSizeToOutSize	__PR((Ulong BytesToDo));


/*
 * format the sample rate into an
 * bigendian 10-byte IEEE-754 floating point number
 */
LOCAL int
Format_samplerate(rate, the_rate)
	Ulong	rate;
	Uchar	the_rate[10];
{
	int	i;

	/*
	 * normalize rate
	 */
	for (i = 0; (rate & 0xffff) != 0; rate <<= 1, i++) {
		if ((rate & 0x8000) != 0) {
			break;
		}
	}

	/*
	 * set exponent and sign
	 */
	the_rate[1] = 14-i;
	the_rate[0] = 0x40;	/* LSB = sign */

	/*
	 * 16-bit part of mantisse for sample rate
	 */
	the_rate[3] = rate & 0xff;
	the_rate[2] = (rate >> 8) & 0xff;

	/*
	 * initialize lower digits of mantisse
	 */
	the_rate[4] = the_rate[5] = the_rate[6] =
	the_rate[7] = the_rate[8] = the_rate[9] = 0;

	return (0);
}

LOCAL int
InitSound(audio, channels, rate, nBitsPerSample, expected_bytes)
	int	audio;
	long	channels;
	Ulong	rate;
	long	nBitsPerSample;
	Ulong	expected_bytes;
{
	UINT4	tmp;

	fillbytes(&AiffHdr, sizeof (AiffHdr), '\0');
	AiffHdr.formChk.ckid  = cpu_to_be32(FOURCC_FORM);
	AiffHdr.formChk.dwSize = cpu_to_be32(expected_bytes +
					offsetof(AIFFHDR, blocksize) +
					sizeof (AiffHdr.blocksize)
			- offsetof(AIFFHDR, formType));
	AiffHdr.formType	= cpu_to_be32(FOURCC_AIFF);

	AiffHdr.commChk.ckid	= cpu_to_be32(FOURCC_COMM);
	AiffHdr.commChk.dwSize	= cpu_to_be32(offsetof(AIFFHDR, ssndChkid)
					- offsetof(AIFFHDR, numChannels));

	AiffHdr.numChannels[1]	   = channels;
	tmp = cpu_to_be32(expected_bytes/(channels * (nBitsPerSample/8)));
	AiffHdr.numSampleFrames[0] = tmp >> 24;
	AiffHdr.numSampleFrames[1] = tmp >> 16;
	AiffHdr.numSampleFrames[2] = tmp >> 8;
	AiffHdr.numSampleFrames[3] = tmp >> 0;
	AiffHdr.samplesize[1] = nBitsPerSample;
	Format_samplerate(rate, AiffHdr.sample_rate);

	memcpy(AiffHdr.ssndChkid, "SSND", 4);
	tmp = cpu_to_be32(expected_bytes + offsetof(AIFFHDR, blocksize) +
		sizeof (AiffHdr.blocksize) - offsetof(AIFFHDR, offset));
	AiffHdr.dwSize[0] = tmp >> 24;
	AiffHdr.dwSize[1] = tmp >> 16;
	AiffHdr.dwSize[2] = tmp >> 8;
	AiffHdr.dwSize[3] = tmp >> 0;

	global.md5offset = sizeof (AiffHdr);

	return (write(audio, &AiffHdr, sizeof (AiffHdr)));
}

LOCAL int
ExitSound(audio, nBytesDone)
	int	audio;
	Ulong	nBytesDone;
{
	UINT4	tmp;

	AiffHdr.formChk.dwSize = cpu_to_be32(nBytesDone + sizeof (AIFFHDR)
					- offsetof(AIFFHDR, commChk));
	tmp = cpu_to_be32(nBytesDone/(
		AiffHdr.numChannels[1] * AiffHdr.samplesize[1]/ULONG_C(8)));
	AiffHdr.numSampleFrames[0] = tmp >> 24;
	AiffHdr.numSampleFrames[1] = tmp >> 16;
	AiffHdr.numSampleFrames[2] = tmp >> 8;
	AiffHdr.numSampleFrames[3] = tmp >> 0;

	/*
	 * If an odd number of bytes has been written,
	 * extend the chunk with one dummy byte.
	 * This is a requirement for AIFF.
	 */
	if ((nBytesDone & 1) && (lseek(audio, 1L, SEEK_CUR) == -1)) {
		return (0);
	}

	/*
	 * goto beginning
	 */
	if (lseek(audio, 0L, SEEK_SET) == -1) {
		return (0);
	}
	return (write(audio, &AiffHdr, sizeof (AiffHdr)));
}

LOCAL Ulong
GetHdrSize()
{
	return (sizeof (AiffHdr));
}

LOCAL Ulong
InSizeToOutSize(BytesToDo)
	Ulong	BytesToDo;
{
	return (BytesToDo);
}

struct soundfile aiffsound =
{
	InitSound,		/* init header method */
	ExitSound,		/* exit header method */
	GetHdrSize,		/* report header size method */
	(int (*) __PR((int audio,
		Uchar *buf,
		size_t BytesToDo))) write, /* get sound samples out */
	InSizeToOutSize,	/* compressed? output file size */
	1,			/* needs big endian samples */
	"AIFF"
};
