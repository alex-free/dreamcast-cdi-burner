/* @(#)xmconfig.h	1.45 10/08/27 Copyright 1995-2010 J. Schilling */
/*
 *	This file either includes the manual generated
 *	static definitions for a machine configuration.
 *
 *	Copyright (c) 1995-2010 J. Schilling
 *
 *	Use only cpp instructions.
 *
 *	NOTE: SING: (Schily Is Not Gnu)
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

#ifndef _SCHILY_XMCONFIG_H
#define	_SCHILY_XMCONFIG_H

/*
 * This is the only static (hand crafted) set of machine configuration
 * left over. We are trying to avoid even this in future.
 */


#ifdef __cplusplus
extern "C" {
#endif

#if defined(VMS)
#	define	PROTOTYPES
#	define	HAVE_OSDEF	/* prevent later defines to overwrite current */
#	define	__NOT_SVR4__	/* Not a real SVR4 implementation */

#	define	HAVE_UNISTD_H
#	define	HAVE_SYS_TYPES_H
#	define	HAVE_CTYPE_H
/*  #	define	HAVE_SYS_TIME_H	*/
#	define	HAVE_FCNTL_H
#	define	HAVE_USLEEP
#	if __DECC_VER >= 60000000
#		define HAVE_INTTYPES_H
#	endif /* __DECC_VER >= 60000000 */
#	define SIZEOF_CHAR 1
#	define SIZEOF_UNSIGNED_CHAR SIZEOF_CHAR
#	define SIZEOF_SHORT_INT 2
#	define SIZEOF_UNSIGNED_SHORT_INT SIZEOF_SHORT_INT
#	define SIZEOF_INT 4
#	define SIZEOF_UNSIGNED_INT SIZEOF_INT
#	define SIZEOF_LONG_INT 4
#	define SIZEOF_UNSIGNED_LONG_INT SIZEOF_LONG_INT
#	ifdef __VAX
#		define SIZEOF_LONG_LONG 4
#	else /* def __VAX */
#	endif /* def __VAX [else] */
#	define SIZEOF_UNSIGNED_LONG_LONG SIZEOF_LONG_LONG
#	define SIZEOF_CHAR_P 4

#	include <types.h>
#	ifndef __OFF_T
		typedef int off_t;
#	endif /* ndef __OFF_T */

#	if defined(__alpha)
#		define HOST_CPU	"Alpha"
#	elif defined(__ia64)
#		define HOST_CPU	"IA64"
#	elif defined(__vax)
#		define HOST_CPU	"VAX"
#	else
#		define HOST_CPU	"VAX(?)"
#	endif
#	define HOST_VENDOR "HP"
#	define HOST_OS "VMS/OpenVMS"
#	define RETSIGTYPE   void
#	define	HAVE_STDARG_H
#	define	HAVE_STDLIB_H
#	define	HAVE_STRING_H
#	define	HAVE_STDC_HEADERS
#	define	STDC_HEADERS
#	define	HAVE_UNISTD_H
#	define	HAVE_FCNTL_H
#	define	HAVE_DIRENT_H
#	define	HAVE_WAIT_H
#	define	HAVE_SYS_UTSNAME_H
#	define	HAVE_GETCWD
#	define	HAVE_STRERROR
#	define	HAVE_MEMMOVE
#	define	HAVE_MMAP
#	define	HAVE_FLOCK
#	define	HAVE_GETHOSTNAME
#	define	HAVE_SELECT
#	define	USLEEPRETURN_T	unsigned int

#	define	HAVE_STRUCT_TIMEVAL
#	define	HAVE_UTSNAME_ARCH	/* uname -p */

#	define HAVE_MALLOC
#	define HAVE_CALLOC
#	define HAVE_REALLOC

#	define	HAVE_UNAME
#	define	HAVE_RENAME
#	define	HAVE_PUTENV
#	define	HAVE_STRNCPY

/* 2005-11-22 SMS.  Enabled some above.  Added some below. */
#	define	HAVE_ECVT
#	define	HAVE_FCVT
#	define	HAVE_GCVT
#	define	HAVE_NICE 1
#	define	HAVE_SELECT
#	define	HAVE_STRCASECMP
#	if __CRTL_VER >= 70312000
#		define HAVE_SNPRINTF
#	endif /* __CRTL_VER >= 70312000 */
#	define	NICE_DECR -8

/* 2005-03-14 SMS.  Need VMS-specific open() parameters. */
#ifndef	NO_OPENFD_SRC
#	define _OPENFD_SRC	/* Use VMS-specific _openfd() function. */
#	define _openfd openfd_vms	/* This one. */
#	define O_BINARY 0x0004	/* DOS-like value.  Implies "ctx=bin". */
#endif

#	define HAVE_C_BIGENDIAN
#	define HAVE_C_BITFIELDS
#	define BITFIELDS_LTOH

/* 2006-09-14 SMS.  Various things for libfind. */
#	define fork fork_dummy
#	ifndef _POSIX_ARG_MAX
#		define _POSIX_ARG_MAX 4096	/* Probably needed. */
#	endif /* ndef _POSIX_ARG_MAX */
#	define HAVE_DECL_STAT
#	define HAVE_DECL_LSTAT

/* 2006-09-17 SMS. */
#	if __CRTL_VER >= 70000000
#		define HAVE_GETPAGESIZE
#	endif /* __CRTL_VER >= 70000000 */

	extern pid_t fork_dummy(void);

#endif /* defined(VMS) */


#ifdef __cplusplus
}
#endif

#endif /* _SCHILY_XMCONFIG_H */
