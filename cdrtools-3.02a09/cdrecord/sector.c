/* @(#)sector.c	1.18 10/12/19 Copyright 2001-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)sector.c	1.18 10/12/19 Copyright 2001-2010 J. Schilling";
#endif
/*
 *	Functions needed to use libedc_ecc from cdrecord
 *
 *	Copyright (c) 2001-2010 J. Schilling
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
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/time.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include "cdrecord.h"
#include "movesect.h"

#ifdef	HAVE_LIB_EDC_ECC


#define	LAYER2
#define	EDC_LAYER2
#define	ENCODER
#define	EDC_ENCODER
#include <ecc.h>

#ifdef	DO8
#define	HAVE_NEW_LIB_EDC
#endif

EXPORT	int	encspeed	__PR((BOOL be_verbose));
EXPORT	void	encsectors	__PR((track_t *trackp, Uchar *bp, int address, int nsecs));
EXPORT	void	scrsectors	__PR((track_t *trackp, Uchar *bp, int address, int nsecs));
EXPORT	void	encodesector	__PR((Uchar *sp, int sectype, int address));
EXPORT	void	fillsector	__PR((Uchar *sp, int sectype, int address));

/*
 * Sector types known by lib libedc_ecc:
 */
#ifdef	__comment__
				/*   MMC					*/
#define	MODE_0		0	/* -> XX  12+4+2336	(12+4uuu von libedc)	*/
#define	MODE_1		1	/* -> 8   12+4+2048+288 (124+4uuu+288 von libedc)*/
#define	MODE_2		2	/* -> 9	  12+4+2336	(12+4uuu von libedc)	*/
#define	MODE_2_FORM_1	3	/* -> 10/11 12+4+8+2048+280 (12+4hhhuuu+280 von libedc)*/
#define	MODE_2_FORM_2	4	/* -> 12 (eher 13!) 12+4+8+2324+4 (12+4hhhuuu+4 von libedc)*/
#define	AUDIO		5
#define	UNKNOWN		6
#endif

/*
 * known sector types
 */
#ifndef	EDC_MODE_0
#define	EDC_MODE_0	MODE_0
#endif
#ifndef	EDC_MODE_1
#define	EDC_MODE_1	MODE_1
#endif
#ifndef	EDC_MODE_2
#define	EDC_MODE_2	MODE_2
#endif
#ifndef	EDC_MODE_2_FORM_1
#define	EDC_MODE_2_FORM_1	MODE_2_FORM_1
#endif
#ifndef	EDC_MODE_2_FORM_2
#define	EDC_MODE_2_FORM_2	MODE_2_FORM_2
#endif
#ifndef	EDC_AUDIO
#define	EDC_AUDIO	AUDIO
#endif
#ifndef	EDC_UNKNOWN
#define	EDC_UNKNOWN	UNKNOWN
#endif

/*
 * Compute max sector encoding speed
 */
EXPORT int
encspeed(be_verbose)
	BOOL	be_verbose;
{
	track_t	t[1];
	Uchar	sect[2352];
	int	i;
	struct	timeval tv;
	struct	timeval tv2;

	t[0].sectype = ST_MODE_1;

	/*
	 * Encoding speed is content dependant.
	 * Set up a known non-null pattern in the sector before; to make
	 * the result of this test independant of the current stack content.
	 */
	for (i = 0; i < 2352; ) {
		sect[i++] = 'J';
		sect[i++] = 'S';
	}
	gettimeofday(&tv, (struct timezone *)0);
	for (i = 0; i < 75000; i++) {		/* Goes up to 1000x */
		encsectors(t, sect, 12345, 1);
		gettimeofday(&tv2, (struct timezone *)0);
		if (tv2.tv_sec >= (tv.tv_sec+1) &&
		    tv2.tv_usec >= tv.tv_usec)
			break;
	}
	if (be_verbose) {
		printf(_("Encoding speed : %dx (%d sectors/s) for libedc from %s\n"),
				(i+74)/75, i,
				_("Heiko Eissfeldt"));
	}
	return ((i+74)/75);
}

/*
 * Encode sectors according to trackp->sectype
 */
EXPORT void
encsectors(trackp, bp, address, nsecs)
	track_t	*trackp;
	Uchar	*bp;
	int	address;
	int	nsecs;
{
	int	sectype = trackp->sectype;

	if ((sectype & ST_MODE_MASK) == ST_MODE_AUDIO)
		return;

	while (--nsecs >= 0) {
		encodesector(bp, sectype, address);
		address++;
		bp += trackp->secsize;
	}
}


#ifdef	CLONE_WRITE

#define	IS_SECHDR(p)	((p)[0] == 0 &&				\
			(p)[1] == 0xFF && (p)[2] == 0xFF &&	\
			(p)[3] == 0xFF && (p)[4] == 0xFF &&	\
			(p)[5] == 0xFF && (p)[6] == 0xFF &&	\
			(p)[7] == 0xFF && (p)[8] == 0xFF &&	\
			(p)[9] == 0xFF && (p)[10] == 0xFF &&	\
			(p)[11] == 0)
/*
 * Scramble data sectors without coding (needed for clone writing)
 */
EXPORT void
scrsectors(trackp, bp, address, nsecs)
	track_t	*trackp;
	Uchar	*bp;
	int	address;
	int	nsecs;
{
	/*
	 * In Clone write mode, we cannot expect that the sector type
	 * of a "track" which really is a file holding the whole disk
	 * is flagged with something that makes sense.
	 *
	 * For this reason, we check each sector if it's a data sector
	 * and needs scrambling.
	 */
	while (--nsecs >= 0) {
		if (IS_SECHDR(bp))
			scramble_L2(bp);
		bp += trackp->secsize;
	}
}
#else
EXPORT void
scrsectors(trackp, bp, address, nsecs)
	track_t	*trackp;
	Uchar	*bp;
	int	address;
	int	nsecs;
{
	comerrno(EX_BAD, _("Cannot write in clone RAW mode.\n"));
}
#endif

/*
 * Encode one sector according to trackp->sectype
 */
EXPORT void
encodesector(sp, sectype, address)
	Uchar	*sp;
	int	sectype;
	int	address;
{
	if (address < -150)
		address += 450150;
	else
		address += 150;
#define	_address address


	switch (sectype & ST_MODE_MASK) {

	case	ST_MODE_0:
		do_encode_L2(sp, EDC_MODE_0, _address);
		break;

	case	ST_MODE_1:
		do_encode_L2(sp, EDC_MODE_1, _address);
		break;

	case	ST_MODE_2:
		do_encode_L2(sp, EDC_MODE_2, _address);
		break;

	case	ST_MODE_2_FORM_1:
		sp[16+2]   &= ~0x20;	/* Form 1 sector */
		sp[16+4+2] &= ~0x20;	/* Form 1 sector 2nd copy */
		/* FALLTHROUGH */

	case	ST_MODE_2_MIXED:
		do_encode_L2(sp, EDC_MODE_2_FORM_1, _address);
		break;

	case	ST_MODE_2_FORM_2:
		sp[16+2]   |= 0x20;	/* Form 2 sector */
		sp[16+4+2] |= 0x20;	/* Form 2 sector 2nd copy */

		do_encode_L2(sp, EDC_MODE_2_FORM_2, _address);
		break;

	case	ST_MODE_AUDIO:
		return;
	default:
		fill2352(sp, '\0');
		return;
	}
	if ((sectype & ST_NOSCRAMBLE) == 0) {
		scramble_L2(sp);
#ifndef	EDC_SCRAMBLE_NOSWAP
		swabbytes(sp, 2352);
#endif
	}
}

/*
 * Create one zero filles encoded sector (according to trackp->sectype)
 */
EXPORT void
fillsector(sp, sectype, address)
	Uchar	*sp;
	int	sectype;
	int	address;
{
	fill2352(sp, '\0');
	encodesector(sp, sectype, address);
}

#endif	/* HAVE_LIB_EDC_ECC */
