/* @(#)sndconfig.h	1.4 06/05/13 Copyright 1998,1999 Heiko Eissfeldt */

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

#ifndef	_SNDCONFIG_H
#define	_SNDCONFIG_H

#define	NONBLOCKING_AUDIO

extern	int	set_snd_device	__PR((const char *devicename));
extern	int	init_soundcard	__PR((double rate, int bits));

extern	int	open_snd_device	__PR((void));
extern	int	close_snd_device __PR((void));
extern	int	write_snd_device __PR((char *buffer, unsigned length));

#endif	/* _SNDCONFIG_H */
