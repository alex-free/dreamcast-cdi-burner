/* @(#)edcspeed.c	1.7 17/07/15 Copyright 2002-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)edcspeed.c	1.7 17/07/15 Copyright 2002-2017 J. Schilling";
#endif
/*
 *	Copyright (c) 2002-2017 J. Schilling
 */
/*@@C@@*/

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/time.h>
#include <schily/string.h>

#define	EXPORT
#define	Uchar	unsigned char

LOCAL	int	encspeed	__PR((void));
EXPORT	int	main		__PR(());

LOCAL int
encspeed()
{
		Uchar	sbuf[2352];
	register Uchar	*sect = sbuf;
	register int	i;
	register int	end;
	register int	secs;
	struct	timeval tv;
	struct	timeval tv2;

	secs = 10;
	end = 75*1000000 * secs;

	memset(sect, 0, sizeof (sect));
	for (i = 0; i < 2352; ) {
		sect[i++] = 'J';
		sect[i++] = 'S';
	}

	gettimeofday(&tv, (struct timezone *)0);
	for (i = 0; i < end; i++) {
#ifdef	OLD_LIBEDC
		do_encode_L2(sect, 1, 1);
		scramble_L2(sect);
#else

/*		lec_encode_mode1_sector(12345, sect);*/
/*		lec_scramble(sect);*/
#endif
/*		if ((i & 31) == 0) {*/
		if (1) {
			gettimeofday(&tv2, (struct timezone *)0);
			if (tv2.tv_sec >= (tv.tv_sec+secs) &&
			    tv2.tv_usec >= tv.tv_usec)
				break;
		}
	}
	printf("%d sectors/%ds\n", i, secs);
	printf("%d sectors/s\n", i/secs);
	printf("speed: %5.2fx\n", (1.0*i)/750.0);
	return (((i+74)/75) / secs);
}

int
main()
{
/*	lec_init();*/

/*	printf("speed: %d\n",  encspeed());*/
	encspeed();
	return (0);
}
