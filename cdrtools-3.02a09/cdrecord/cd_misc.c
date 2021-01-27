/* @(#)cd_misc.c	1.18 10/12/19 Copyright 1997-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cd_misc.c	1.18 10/12/19 Copyright 1997-2010 J. Schilling";
#endif
/*
 *	Misc CD support routines
 *
 *	Copyright (c) 1997-2010 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/utypes.h>	/* Includes <sys/types.h> for caddr_t */
#include <schily/stdio.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include "cdrecord.h"

EXPORT	int	from_bcd		__PR((int b));
EXPORT	int	to_bcd			__PR((int i));
EXPORT	long	msf_to_lba		__PR((int m, int s, int f, BOOL force_positive));
EXPORT	BOOL	lba_to_msf		__PR((long lba, msf_t *mp));
EXPORT	void	sec_to_msf		__PR((long sec, msf_t *mp));
EXPORT	void	print_min_atip		__PR((long li, long lo));
EXPORT	BOOL	is_cdspeed		__PR((int speed));

EXPORT int
from_bcd(b)
	int	b;
{
	return ((b & 0x0F) + 10 * (((b)>> 4) & 0x0F));
}

EXPORT int
to_bcd(i)
	int	i;
{
	return (i % 10 | ((i / 10) % 10) << 4);
}

EXPORT long
msf_to_lba(m, s, f, force_positive)
	int	m;
	int	s;
	int	f;
	BOOL	force_positive;
{
	long	ret = m * 60 + s;

	ret *= 75;
	ret += f;
	if (m < 90 || force_positive)
		ret -= 150;
	else
		ret -= 450150;
	return (ret);
}

EXPORT BOOL
lba_to_msf(lba, mp)
	long	lba;
	msf_t	*mp;
{
	int	m;
	int	s;
	int	f;

#ifdef	__follow_redbook__
	if (lba >= -150 && lba < 405000) {	/* lba <= 404849 */
#else
	if (lba >= -150) {
#endif
		m = (lba + 150) / 60 / 75;
		s = (lba + 150 - m*60*75)  / 75;
		f = (lba + 150 - m*60*75 - s*75);

	} else if (lba >= -45150 && lba <= -151) {
		m = (lba + 450150) / 60 / 75;
		s = (lba + 450150 - m*60*75)  / 75;
		f = (lba + 450150 - m*60*75 - s*75);

	} else {
		mp->msf_min   = -1;
		mp->msf_sec   = -1;
		mp->msf_frame = -1;

		return (FALSE);
	}
	mp->msf_min   = m;
	mp->msf_sec   = s;
	mp->msf_frame = f;

	if (lba > 404849)			/* 404850 -> 404999: lead out */
		return (FALSE);
	return (TRUE);
}

EXPORT void
sec_to_msf(sec, mp)
	long	sec;
	msf_t	*mp;
{
	int	m;
	int	s;
	int	f;

	m = (sec) / 60 / 75;
	s = (sec - m*60*75)  / 75;
	f = (sec - m*60*75 - s*75);

	mp->msf_min   = m;
	mp->msf_sec   = s;
	mp->msf_frame = f;
}

EXPORT void
print_min_atip(li, lo)
	long	li;
	long	lo;
{
	msf_t	msf;

	if (li < 0) {
		lba_to_msf(li, &msf);

		printf(_("  ATIP start of lead in:  %ld (%02d:%02d/%02d)\n"),
			li, msf.msf_min, msf.msf_sec, msf.msf_frame);
	}
	if (lo > 0) {
		lba_to_msf(lo, &msf);
		printf(_("  ATIP start of lead out: %ld (%02d:%02d/%02d)\n"),
			lo, msf.msf_min, msf.msf_sec, msf.msf_frame);
	}
}

/*
 * Make a probability guess whether the speed is related to CD or DVD speed.
 */
EXPORT BOOL
is_cdspeed(speed)
	int	speed;
{
	int	cdspeed;
	long	cdrest;
	int	dvdspeed;
	long	dvdrest;

	cdspeed = speed / 176;
	cdrest = cdspeed * 1764;
	cdrest = speed * 10 - cdrest;
	if (cdrest < 0)
		cdrest *= -1;
	if (cdrest > 1764/2)
		cdrest = 1764 - cdrest;

	/*
	 * 3324 = 1382 * 2.4
	 * speed > 0.9 * 3324 && speed < 1.1 * 3324
	 */
	if (speed >= 2990 && speed <= 3656) {
		dvdrest = 33240;
		dvdrest = speed * 10 - dvdrest;
	} else {
		dvdspeed = speed / 1385;
		dvdrest = dvdspeed * 13850;
		dvdrest = speed * 10 - dvdrest;
	}
	if (dvdrest < 0)
		dvdrest *= -1;
	if (dvdrest > 13850/2)
		dvdrest = 13850 - dvdrest;

	/*
	 * 1385 / 176.4 = 7.85, so we need to multiply the CD rest by 7.85
	 */
	return (dvdrest > (cdrest*8 - cdrest/6));
}
