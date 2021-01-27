/* @(#)crc16.h	1.3 02/03/04 Copyright 1998-2002 J. Schilling */
/*
 *	Q-subchannel CRC definitions
 *
 *	Copyright (c) 1998-2002 J. Schilling
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

#ifndef	_CRC16_H
#define	_CRC16_H

extern	UInt16_t	calcCRC		__PR((Uchar *buf, Uint bsize));
extern	UInt16_t	fillcrc		__PR((Uchar *buf, Uint bsize));
extern	UInt16_t	flip_crc_error_corr __PR((Uchar *b, Uint bsize, Uint p_crc));

#endif
