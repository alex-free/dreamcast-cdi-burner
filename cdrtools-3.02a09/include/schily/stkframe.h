/* @(#)stkframe.h	1.13 10/08/27 Copyright 1995-2010 J. Schilling */
/*
 * Common definitions for routines that parse the stack frame.
 *
 * This file has to be fixed if you want to port routines which use getfp().
 * Have a look at struct frame below and use it as a sample,
 * the new struct frame must at least contain a member 'fr_savfp'.
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

#ifndef _SCHILY_STKFRAME_H
#define	_SCHILY_STKFRAME_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#if defined(sun) && (defined(SVR4) || defined(__SVR4) || defined(__SVR4__))
	/*
	 * Solaris 2.x aka SunOS 5.x
	 */
#	ifdef	i386
		/*
		 * On Solaris 2.1 x86 sys/frame.h is not useful at all
		 * On Solaris 2.4 x86 sys/frame.h is buggy (fr_savfp is int!!)
		 */
#		ifndef	_INCL_SYS_REG_H
#		include <sys/reg.h>
#		define	_INCL_SYS_REG_H
#		endif
#	endif
#	ifndef	_INCL_SYS_FRAME_H
#	include <sys/frame.h>
#	define	_INCL_SYS_FRAME_H
#	endif

#else
# if	defined(sun)
	/*
	 * SunOS 4.x
	 */
#	ifndef	_INCL_MACHINE_FRAME_H
#	include <machine/frame.h>
#	define	_INCL_MACHINE_FRAME_H
#	endif
# else
	/*
	 * Anything that is not SunOS
	 */

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	_SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

/*
 * XXX: I hope this will be useful on other machines (no guarantee)
 * XXX: It is taken from a sun Motorola system, but should also be useful
 * XXX: on a i386.
 * XXX: In general you have to write a sample program, set a breakpoint
 * XXX: on a small function and inspect the stackframe with adb.
 */

struct frame {
	struct frame	*fr_savfp;	/* saved frame pointer */
	Intptr_t	fr_savpc;	/* saved program counter */
	int		fr_arg[1];	/* array of arguments */
};

#ifdef	__cplusplus
}
#endif

# endif	/* ! defined (sun) */
#endif	/* ! defined (sun) && (defined(SVR4) || defined(__SVR4) || ... */

#endif	/* _SCHILY_STKFRAME_H */
