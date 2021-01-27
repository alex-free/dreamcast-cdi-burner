/* @(#)repair.c	1.4 10/05/24 Copyright 2001-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)repair.c	1.4 10/05/24 Copyright 2001-2010 J. Schilling";
#endif
/*
 * Code to check libedc_ecc_dec
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

#include <schily/stdio.h>
#include <schily/fcntl.h>
#include <schily/unistd.h>

#define	LAYER2
#define	ENCODER
#define	EDC_LAYER2
#define	EDC_ENCODER

#define	EDC_DECODER
#define	EDC_SUBCHANNEL

#include "ecc.h"
#include "edc.h"

#ifndef	EDC_MODE_1
#define	EDC_MODE_1		MODE_1
#endif

char	ooo[2352];
char	xxx[2352];

int
main()
{
	int	f = open("OOO", 0);
	int	f2;
	int	f3;
	int	f4;
	int	i;

	read(f, ooo, 16);
	read(f, &xxx[16], 2048);

	do_encode_L2((unsigned char *)xxx, EDC_MODE_1, 0 + 150);

	f2 = creat("out", 0666);
	write(f2, xxx, 2352);

printf("crc_check %d\n", crc_check((unsigned char *)xxx, EDC_MODE_1));

	xxx[123] = 123;

	f3 = creat("out-def", 0666);
	write(f3, xxx, 2352);

printf("crc_check %d\n", crc_check((unsigned char *)xxx, EDC_MODE_1));

#define	FALSE	0
	do_decode_L2((unsigned char *)xxx, EDC_MODE_1, FALSE, 0);

printf("crc_check %d\n", crc_check((unsigned char *)xxx, EDC_MODE_1));

	f4 = creat("out-rep", 0666);
	write(f4, xxx, 2352);

	return (0);
}
