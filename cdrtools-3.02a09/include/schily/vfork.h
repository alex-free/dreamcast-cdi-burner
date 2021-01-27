/* @(#)vfork.h	1.4 09/11/15 Copyright 2009 J. Schilling */
/*
 *	Vfork abstraction
 *
 *	Copyright (c) 2009 J. Schilling
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

#ifndef	_SCHILY_VFORK_H
#define	_SCHILY_VFORK_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef	_SCHILY_UNISTD_H
#include <schily/unistd.h>	/* vfork() definitions are here on Solaris */
#endif

#ifdef	HAVE_VFORK_H
#ifndef	_INCL_VFORK_H
#define	_INCL_VFORK_H
#include <vfork.h>
#endif
#endif
#ifdef	HAVE_SYS_FORK_H
#ifndef	_INCL_SYS_FORK_H
#define	_INCL_SYS_FORK_H
#include <sys/fork.h>
#endif
#endif

#ifdef	VMS
#ifndef	VMS_VFORK_OK
/*
 * vfork() on VMS implements strange deviations from the expected behavior.
 * The child does not run correctly unless it calls exec*().
 * The file descriptors are not separated from the parent.
 *
 * decc$set_child_standard_streams() allows to work around the most important
 * problem of having no separate space for file descriptors if the code is
 * prepared for the deviations.
 *
 * As VMS silently overwrites the "vfork" #definition from mconfig.h in
 * unistd.h, we need to reset to the definition in mconfig.h for all programs
 * that are not prepared for the deviations.
 */
#ifndef	HAVE_VFORK	/* Paranoia in case VMS implements a working vfork() */
#undef	vfork
#define	vfork	fork
#endif	/* HAVE_VFORK */

#else	/* VMS_VFORK_OK */

#define	set_child_standard_fds(in, out, err)		\
			decc$set_child_standard_streams((in), (out), (err))
#endif	/* VMS_VFORK_OK */
#endif	/* VMS */

#endif	/* _SCHILY_VFORK_H */
