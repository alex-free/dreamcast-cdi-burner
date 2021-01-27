/* @(#)sun.c	1.15 13/04/28 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2013 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)sun.c	1.15 13/04/28 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2013 J. Schilling";

#endif
/*
 * Copyright (C) by Heiko Eissfeldt
 * Copyright (c) 2006-2013 J. Schilling
 *
 *  definitions for sun pcm output
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

typedef struct SUNHDR {
	unsigned int	magic;		/* dns. a la .snd */
	unsigned int	data_location;	/* offset to data */
	unsigned int	size;		/* # of data bytes */
	unsigned int	format;		/* format code */
	unsigned int	sample_rate;	/* in Hertz */
	unsigned int	channelcount;	/* 1 for mono, 2 for stereo */
char info[8];				/* comments */
} SUNHDR;

LOCAL SUNHDR	sunHdr;

LOCAL int	InitSound	__PR((int audio, long channels,
					Ulong rate,
					long nBitsPerSample,
					Ulong expected_bytes));
LOCAL int	ExitSound	__PR((int audio, Ulong nBytesDone));
LOCAL Ulong	GetHdrSize	__PR((void));
LOCAL Ulong	InSizeToOutSize	__PR((Ulong BytesToDo));


LOCAL int
InitSound(audio, channels, rate, nBitsPerSample, expected_bytes)
	int	audio;
	long	channels;
	Ulong	rate;
	long	nBitsPerSample;
	Ulong	expected_bytes;
{
	Ulong	hdr_format = nBitsPerSample > 8 ? 0x03 : 0x02;

	sunHdr.magic		= cpu_to_le32(UINT4_C(0x646e732e));
	sunHdr.data_location	= cpu_to_be32(0x20);
	sunHdr.size		= cpu_to_be32(expected_bytes);
	sunHdr.format		= cpu_to_be32(hdr_format);
	sunHdr.sample_rate	= cpu_to_be32(rate);
	sunHdr.channelcount	= cpu_to_be32(channels);

	global.md5offset = sizeof (sunHdr);

	return (write(audio, &sunHdr, sizeof (sunHdr)));
}

LOCAL int
ExitSound(audio, nBytesDone)
	int	audio;
	Ulong	nBytesDone;
{
	sunHdr.size = cpu_to_be32(nBytesDone);

	/*
	 * goto beginning
	 */
	if (lseek(audio, 0L, SEEK_SET) == -1) {
		return (0);
	}
	return (write(audio, &sunHdr, sizeof (sunHdr)));
}

LOCAL Ulong
GetHdrSize()
{
	return (sizeof (sunHdr));
}

LOCAL Ulong
InSizeToOutSize(BytesToDo)
	Ulong	BytesToDo;
{
	return (BytesToDo);
}

struct soundfile sunsound =
{
	InitSound,		/* init header method */
	ExitSound,		/* exit header method */
	GetHdrSize,		/* report header size method */
	(int (*) __PR((int audio,
		Uchar *buf,
		size_t BytesToDo))) write, /* get sound samples out */
	InSizeToOutSize,	/* compressed? output file size */
	1,			/* needs big endian samples */
	"AU"
};
