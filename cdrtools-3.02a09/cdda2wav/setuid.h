/* @(#)setuid.h	1.4 06/05/13 Copyright 1998,1999 Heiko Eissfeldt */
/*
 * Security functions
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

#ifndef	_SETUID_H
#define	_SETUID_H

extern	void initsecurity __PR((void));

extern	void	needroot	__PR((int necessary));
extern	void	dontneedroot	__PR((void));
extern	void	neverneedroot	__PR((void));

extern	void	needgroup	__PR((int necessary));
extern	void	dontneedgroup	__PR((void));
extern	void	neverneedgroup	__PR((void));

#if defined(HPUX)

#define	HAVE_SETREUID
#define	HAVE_SETREGID

extern	int	seteuid		__PR((uid_t uid));
extern	int	setreuid	__PR((uid_t uid1, uid_t uid2));
extern	int	setregid	__PR((gid_t gid1, gid_t gid2));

#endif

#endif	/* _SETUID_H */
