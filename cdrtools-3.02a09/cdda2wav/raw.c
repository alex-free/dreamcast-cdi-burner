/* @(#)raw.c	1.13 10/01/12 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)raw.c	1.13 10/01/12 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling";

#endif
/*
 * RAW sound handling
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
#include "sndfile.h"
#include "global.h"

LOCAL int		InitSound	__PR((int audio, long channels,
						Ulong rate,
						long nBitsPerSample,
						Ulong expected_bytes));
LOCAL int		ExitSound	__PR((int audio, Ulong nBytesDone));
LOCAL unsigned long	GetHdrSize	__PR((void));
LOCAL unsigned long	InSizeToOutSize	__PR((Ulong BytesToDo));

/* ARGSUSED */
LOCAL int
InitSound(audio, channels, rate, nBitsPerSample, expected_bytes)
	int	audio;
	long	channels;
	Ulong	rate;
	long	nBitsPerSample;
	Ulong	expected_bytes;
{
	global.md5offset = 0;
	return (0);
}

LOCAL int
ExitSound(audio, nBytesDone)
	int	audio;
	Ulong	nBytesDone;
{
	return (0);
}

LOCAL unsigned long
GetHdrSize()
{
	return (0L);
}

LOCAL unsigned long
InSizeToOutSize(BytesToDo)
	Ulong	BytesToDo;
{
	return (BytesToDo);
}

struct soundfile rawsound = {
	InitSound,
	ExitSound,
	GetHdrSize,
	(int (*) __PR((int audio,
		Uchar *buf,
		size_t BytesToDo))) write, /* get sound samples out */

	InSizeToOutSize,	/* compressed? output file size */
	1,			/* needs big endian samples */
	"MOTOROLA"
};
