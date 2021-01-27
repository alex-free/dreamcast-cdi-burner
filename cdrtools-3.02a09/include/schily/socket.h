/* @(#)socket.h	1.3 12/11/14 Copyright 2009-2012 J. Schilling */
/*
 *	Socket abstraction
 *
 *	Copyright (c) 2009-2012 J. Schilling
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

#ifndef	_SCHILY_SOCKET_H
#define	_SCHILY_SOCKET_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_SYS_SOCKET_H

#ifndef	_INCL_SYS_SOCKET_H
#include <sys/socket.h>
#define	_INCL_SYS_SOCKET_H
#endif

/*
 * Compatibility defines for UNIX/POSIX:
 *
 * Win32 defines a socket layer in winsock.h that is not POSIX compliant.
 * The functions socket() and accept() return an "unsigned int" instead of just
 * an "int". As a result, an error return from socket() and accept() cannot be
 * -1 but is INVALID_SOCKET. All functions from the Win32 socket layer except
 * socket() and accept() return -1 on error.
 * Since a socket is not a file descriptor on Win32, we cannot call close()
 * but need to call closesocket().
 * If we like to write software that compiles on a Win32 system without a
 * POSIX layer, we need use the following definitions as a workaround even
 * for UNIX/POSIX systems.
 */

#define	SOCKET		int	/* The socket type on UNIX/POSIX */
#define	INVALID_SOCKET	(-1)	/* Error return code for socket()/accept() */
#define	closesocket	close	/* Use instead of close(s) for Win32 compat */

#else	/* On a non-POSIX system: */
/*
 * If we are on a Win32 system without a POSIX layer, we would need to include
 * winsock.h but this includes definitions that cause compatibility problems.
 * For this reason, we instead include our windows.h that contains the needed
 * workaround.
 */
#ifdef	HAVE_WINDOWS_H
#include <schily/windows.h>
#endif	/* HAVE_WINDOWS_H */
#endif

#endif	/* _SCHILY_SOCKET_H */
