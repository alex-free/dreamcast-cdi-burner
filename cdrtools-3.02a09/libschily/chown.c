/* @(#)chown.c	1.1 11/08/14 Copyright 2011 J. Schilling */
/*
 *	Copyright (c) 2011 J. Schilling
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
#include <schily/types.h>

#ifndef	HAVE_CHOWN
EXPORT int
chown(path, owner, group)
	const char	*path;
		uid_t	owner;
		gid_t	group;
{
	return (0);	/* Fake success */
}
#endif
