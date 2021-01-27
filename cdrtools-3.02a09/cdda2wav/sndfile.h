/* @(#)sndfile.h	1.9 10/01/12 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling */

/*
 * generic soundfile structure
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

#ifndef	_SNDFILE_H
#define	_SNDFILE_H

#include <schily/utypes.h>

struct soundfile {
	int	(* InitSound)		__PR((int audio, long channels,
						Ulong rate,
						long nBitsPerSample,
						Ulong expected_bytes));
	int	(* ExitSound)		__PR((int audio, Ulong nBytesDone));
	Ulong	(* GetHdrSize)		__PR((void));
	int	(* WriteSound)		__PR((int audio, Uchar *buf,
						size_t BytesToDo));
	Ulong	(* InSizeToOutSize)	__PR((Ulong BytesToDo));

	int	need_big_endian;
	char	*auf_cuename;
};

#endif	/* _SNDFILE_H */
