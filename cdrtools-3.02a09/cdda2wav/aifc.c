/* @(#)aifc.c	1.16 16/02/14 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)aifc.c	1.16 16/02/14 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling";

#endif
/*
 * Copyright (C) by Heiko Eissfeldt
 * Copyright (c) 2006-2010 J. Schilling
 *
 * definitions for aifc pcm output
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
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/schily.h>
#include "mytype.h"
#include "byteorder.h"
#include "sndfile.h"
#include "global.h"

typedef UINT4	FOURCC;    /* a four character code */
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
#define	FOURCC_AIFC	mmioFOURCC('A', 'I', 'F', 'C')
#define	FOURCC_FVER	mmioFOURCC('F', 'V', 'E', 'R')
#define	FOURCC_COMM	mmioFOURCC('C', 'O', 'M', 'M')
#define	FOURCC_NONE	mmioFOURCC('N', 'O', 'N', 'E')
#define	FOURCC_SSND	mmioFOURCC('S', 'S', 'N', 'D')

#define	NO_COMPRESSION	"not compressed"

/*
 * brain dead construction from apple involving bigendian 80-bit doubles.
 * Definitely designed not to be portable. Alignment is a nightmare too.
 */
typedef struct AIFCHDR {
	CHUNKHDR	formChk;
	FOURCC		formType;

	CHUNKHDR	fverChk;	/* Version chunk */
	UINT4		timestamp;	/* timestamp identifies version */

	CHUNKHDR	commChk;	/* Common chunk */
	/*
	 * from now on, alignment prevents us from using the original types :-(
	 */
	unsigned char	numChannels[2];		/* Audio Channels */
	unsigned char	numSampleFrames[4];	/* # of samples */
	unsigned char	samplesize[2];		/* bits per sample */
	unsigned char	sample_rate[10];	/* samplerate in extd. float */
	unsigned char	compressionType[4];	/* AIFC extension */
	unsigned char	compressionNameLen;	/* AIFC extension */
	char		compressionName[sizeof (NO_COMPRESSION)]; /* AIFC e. */

	unsigned char	ssndChkid[4];	/* Sound data chunk */
	unsigned char	dwSize[4];	/* size of chunk */
	unsigned char	offset[4];	/* start of 1st sample */
	unsigned char	blocksize[4];	/* aligned sound data block size */

} AIFCHDR;

LOCAL	AIFCHDR	AifcHdr;

LOCAL int	Format_samplerate __PR((Ulong rate, Uchar the_rate[10]));
LOCAL int	InitSound	__PR((int audio, long channels, Ulong rate,
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
	the_rate[0] = 0x40;		/* LSB = sign */

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

	fillbytes(&AifcHdr, sizeof (AifcHdr), '\0');
	AifcHdr.formChk.ckid	= cpu_to_be32(FOURCC_FORM);
	AifcHdr.formChk.dwSize	= cpu_to_be32(expected_bytes +
					offsetof(AIFCHDR, blocksize) +
					sizeof (AifcHdr.blocksize) -
					offsetof(AIFCHDR, commChk));
	AifcHdr.formType	= cpu_to_be32(FOURCC_AIFC);

	AifcHdr.fverChk.ckid	= cpu_to_be32(FOURCC_FVER);
	AifcHdr.fverChk.dwSize	= cpu_to_be32(offsetof(AIFCHDR, commChk)
					- offsetof(AIFCHDR, timestamp));

	AifcHdr.compressionType[0] = 'N';
	AifcHdr.compressionType[1] = 'O';
	AifcHdr.compressionType[2] = 'N';
	AifcHdr.compressionType[3] = 'E';
	AifcHdr.compressionNameLen = sizeof (NO_COMPRESSION)-1;
	strcpy(AifcHdr.compressionName, NO_COMPRESSION);
	/*
	 * AIFC Version 1
	 */
	AifcHdr.timestamp	= cpu_to_be32(UINT4_C(0xA2805140));

	AifcHdr.commChk.ckid	= cpu_to_be32(FOURCC_COMM);
	AifcHdr.commChk.dwSize	= cpu_to_be32(offsetof(AIFCHDR, ssndChkid)
					- offsetof(AIFCHDR, numChannels));

	AifcHdr.numChannels[1] = channels;

	tmp = cpu_to_be32(expected_bytes/(channels * (nBitsPerSample/8)));
	AifcHdr.numSampleFrames[0] = tmp >> 24;
	AifcHdr.numSampleFrames[1] = tmp >> 16;
	AifcHdr.numSampleFrames[2] = tmp >> 8;
	AifcHdr.numSampleFrames[3] = tmp >> 0;
	AifcHdr.samplesize[1]	= nBitsPerSample;
	Format_samplerate(rate, AifcHdr.sample_rate);

	memcpy(AifcHdr.ssndChkid, "SSND", 4);
	tmp = cpu_to_be32(expected_bytes + offsetof(AIFCHDR, blocksize) +
		sizeof (AifcHdr.blocksize) - offsetof(AIFCHDR, offset));
	AifcHdr.dwSize[0] = tmp >> 24;
	AifcHdr.dwSize[1] = tmp >> 16;
	AifcHdr.dwSize[2] = tmp >> 8;
	AifcHdr.dwSize[3] = tmp >> 0;

	global.md5offset = sizeof (AifcHdr);

	return (write(audio, &AifcHdr, sizeof (AifcHdr)));
}

LOCAL int
ExitSound(audio, nBytesDone)
	int	audio;
	Ulong	nBytesDone;
{
	UINT4	tmp;

	AifcHdr.formChk.dwSize = cpu_to_be32(nBytesDone + sizeof (AIFCHDR)
					- offsetof(AIFCHDR, commChk));
	tmp = cpu_to_be32(nBytesDone/(AifcHdr.numChannels[1] *
			    AifcHdr.samplesize[1]/ULONG_C(8)));
	AifcHdr.numSampleFrames[0] = tmp >> 24;
	AifcHdr.numSampleFrames[1] = tmp >> 16;
	AifcHdr.numSampleFrames[2] = tmp >> 8;
	AifcHdr.numSampleFrames[3] = tmp >> 0;

	/*
	 * If an odd number of bytes has been written,
	 * extend the chunk with one dummy byte.
	 * This is a requirement for AIFC.
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
	return (write(audio, &AifcHdr, sizeof (AifcHdr)));
}

LOCAL Ulong
GetHdrSize()
{
	return (sizeof (AifcHdr));
}

LOCAL Ulong
InSizeToOutSize(BytesToDo)
	Ulong	BytesToDo;
{
	return (BytesToDo);
}

struct soundfile aifcsound = {
	InitSound,		/* init header method */
	ExitSound,		/* exit header method */
	GetHdrSize,		/* report header size method */
	(int (*) __PR((int audio,
		Uchar *buf,
		size_t BytesToDo))) write, /* get sound samples out */
	InSizeToOutSize,	/* compressed? output file size */
	1,			/* needs big endian samples */
	"AIFC"
};
