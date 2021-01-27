/* @(#)scgio.h	2.17 07/11/21 Copyright 1986 J. Schilling */
/*
 *	Definitions for the SCSI general driver 'scg'
 *
 *	Copyright (c) 1986 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * The following exceptions apply:
 * CDDL §3.6 needs to be replaced by: "You may create a Larger Work by
 * combining Covered Software with other code if all other code is governed by
 * the terms of a license that is OSI approved (see www.opensource.org) and
 * you may distribute the Larger Work as a single product. In such a case,
 * You must make sure the requirements of this License are fulfilled for
 * the Covered Software."
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	_SCG_SCGIO_H
#define	_SCG_SCGIO_H

#ifndef	_SCG_SCGCMD_H
#include <scg/scgcmd.h>
#endif

#if	defined(SVR4)
#include <sys/ioccom.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(__STDC__) || defined(SVR4)
#define	SCGIOCMD	_IOWR('G', 1, struct scg_cmd)	/* do a SCSI cmd   */
#define	SCGIOCMD32	_IOWR('G', 1, struct scg_cmd32)	/* do a SCSI cmd   */
#define	SCGIORESET	_IO('G', 2)			/* reset SCSI bus  */
#define	SCGIOGDISRE	_IOR('G', 4, int)		/* get sc disre Val*/
#define	SCGIOSDISRE	_IOW('G', 5, int)		/* set sc disre Val*/
#define	SCGIOIDBG	_IO('G', 100)			/* Inc Debug Val   */
#define	SCGIODDBG	_IO('G', 101)			/* Dec Debug Val   */
#define	SCGIOGDBG	_IOR('G', 102, int)		/* get Debug Val   */
#define	SCGIOSDBG	_IOW('G', 103, int)		/* set Debug Val   */
#define	SCIOGDBG	_IOR('G', 104, int)		/* get sc Debug Val*/
#define	SCIOSDBG	_IOW('G', 105, int)		/* set sc Debug Val*/
#else
#define	SCGIOCMD	_IOWR(G, 1, struct scg_cmd)	/* do a SCSI cmd   */
#define	SCGIOCMD32	_IOWR(G, 1, struct scg_cmd32)	/* do a SCSI cmd   */
#define	SCGIORESET	_IO(G, 2)			/* reset SCSI bus  */
#define	SCGIOGDISRE	_IOR(G, 4, int)			/* get sc disre Val*/
#define	SCGIOSDISRE	_IOW(G, 5, int)			/* set sc disre Val*/
#define	SCGIOIDBG	_IO(G, 100)			/* Inc Debug Val   */
#define	SCGIODDBG	_IO(G, 101)			/* Dec Debug Val   */
#define	SCGIOGDBG	_IOR(G, 102, int)		/* get Debug Val   */
#define	SCGIOSDBG	_IOW(G, 103, int)		/* set Debug Val   */
#define	SCIOGDBG	_IOR(G, 104, int)		/* get sc Debug Val*/
#define	SCIOSDBG	_IOW(G, 105, int)		/* set sc Debug Val*/
#endif

#define	SCGIO_CMD	SCGIOCMD	/* backward ccompatibility */

#ifdef	__cplusplus
}
#endif

#endif	/* _SCG_SCGIO_H */
