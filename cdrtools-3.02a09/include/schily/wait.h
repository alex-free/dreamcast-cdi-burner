/* @(#)wait.h	1.26 16/04/02 Copyright 1995-2016 J. Schilling */
/*
 *	Definitions to deal with various kinds of wait flavour
 *
 *	Copyright (c) 1995-2016 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	_SCHILY_WAIT_H
#define	_SCHILY_WAIT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * Needed for SysVr4
 */
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

#if	defined(HAVE_WAIT_H)
#	ifndef	_INCL_WAIT_H
#	include <wait.h>
#	define	_INCL_WAIT_H
#	endif
#else
/*
 * K&R Compiler doesn't like #elif
 */
#	if	defined(HAVE_SYS_WAIT_H)	/* POSIX.1 compl. sys/wait.h */
#	undef	HAVE_UNION_WAIT			/* POSIX.1 doesn't use U_W   */
#		ifndef	_INCL_SYS_WAIT_H
#		include <sys/wait.h>
#		define	_INCL_SYS_WAIT_H
#		endif
#	else
#	if	defined(HAVE_UNION_WAIT)	/* Pure BSD U_W / sys/wait.h */
#		ifndef	_INCL_SYS_WAIT_H
#		include <sys/wait.h>
#		define	_INCL_SYS_WAIT_H
#		endif
#	endif
#	endif
#endif
#ifdef	JOS
#	ifndef	_INCL_SYS_EXIT_H
#	include <sys/exit.h>
#	define	_INCL_SYS_EXIT_H
#	endif
#endif
#if defined(__EMX__) || defined(__DJGPP__)
#	ifndef	_INCL_PROCESS_H
#	include <process.h>
#	define	_INCL_PROCESS_H
#	endif
#endif

#if !defined(HAVE_TYPE_SIGINFO_T) && defined(HAVE_SIGINFO_T)
#ifndef	_SCHILY_SIGNAL_H
#include <schily/signal.h>
#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * waitid() idtype_t definition, define when missing or broken.
 * NetBSD-5 has an idtype_t that is in conflict with POSIX.
 */
#ifndef	HAVE_TYPE_IDTYPE_T

#undef  HAVE_WAITID	/* Can't have waitid() with broken idtype_t */

#undef	P_PID
#undef	P_PPID
#undef	P_PGID
#undef	P_SID
#undef	P_CID
#undef	P_UID
#undef	P_GID
#undef	P_ALL

#define	P_PID	MY_P_PID
#define	P_PPID	MY_P_PPID
#define	P_PGID	MY_P_PGID
#define	P_SID	MY_P_SID
#define	P_CID	MY_P_CID
#define	P_UID	MY_P_UID
#define	P_GID	MY_P_GID
#define	P_ALL	MY_P_ALL

typedef enum {
	P_PID,		/* A process identifier.		*/
	P_PPID,		/* A parent process identifier.		*/
	P_PGID,		/* A process group (job control group)	*/
			/* identifier.				*/
	P_SID,		/* A session identifier.		*/
	P_CID,		/* A scheduling class identifier.	*/
	P_UID,		/* A user identifier.			*/
	P_GID,		/* A group identifier.			*/
	P_ALL,		/* All processes.			*/
} my_idtype_t;

#undef	idtype_t
#define	idtype_t	my_idtype_t

#endif	/* HAVE_TYPE_IDTYPE_T */

#ifndef	WCOREFLG
#ifdef	WCOREFLAG
#define	WCOREFLG	WCOREFLAG
#else
#define	WCOREFLG	0x80
#endif
#define	NO_WCOREFLG
#endif

#ifndef	WSTOPFLG
#ifdef	_WSTOPPED
#define	WSTOPFLG	_WSTOPPED
#else
#define	WSTOPFLG	0x7F
#endif
#define	NO_WSTOPFLG
#endif

#ifndef	WCONTFLG
#define	WCONTFLG	0xFFFF
#define	NO_WCONTFLG
#endif

/*
 * Work around a NetBSD bug that has been imported from BSD.
 * There is a #define WSTOPPED _WSTOPPED that is in conflict with the
 * AT&T waitid() flags definition that is part of the POSIX standard.
 */
#if	defined(WSTOPPED) && defined(_WSTOPPED) && WSTOPPED == _WSTOPPED
#undef	WSTOPPED
#define	WSTOPPED	WUNTRACED
#endif

/*
 * waitid() option flags:
 */
#ifndef	WCONTINUED
#define	WCONTINUED	0
#define	NO_WCONTINUED
#endif
#ifndef	WEXITED
#define	WEXITED		0
#define	NO_WEXITED
#endif
#ifndef	WNOHANG
#define	WNOHANG		0
#define	NO_WNOHANG
#endif
#ifndef	WNOWAIT
#define	WNOWAIT		0
#define	NO_WNOWAIT
#endif
#ifndef	WSTOPPED
#ifdef	WUNTRACED
#define	WSTOPPED	WUNTRACED
#else
#define	WSTOPPED	0
#endif
#define	NO_WSTOPPED
#endif
#ifndef	WTRAPPED
#define	WTRAPPED	0
#define	NO_WTRAPPED
#endif

/*
 * waitid() code values, #define them when they are missing:
 */
#ifndef	CLD_EXITED		/* Assume all is missing then */
#define	CLD_EXITED	1	/* child has exited */
#define	CLD_KILLED	2	/* child was killed */
#define	CLD_DUMPED	3	/* child has coredumped */
#define	CLD_TRAPPED	4	/* traced child has stopped */
#define	CLD_STOPPED	5	/* child has stopped on signal */
#define	CLD_CONTINUED	6	/* stopped child has continued */
#define	NO_CLD_EXITED
#endif	/* CLD_EXITED */

#if defined(HAVE_UNION_WAIT) && defined(USE_UNION_WAIT)
#	define WAIT_T union wait
#	define	_W_U(w)	((union wait *)&(w))
#	ifndef WTERMSIG
#		define WTERMSIG(status)		(_W_U(status)->w_termsig)
#	endif
#	ifndef WCOREDUMP
#		define WCOREDUMP(status)	(_W_U(status)->w_coredump)
#	endif
#	ifndef WEXITSTATUS
#		define WEXITSTATUS(status)	(_W_U(status)->w_retcode)
#	endif
#	ifndef WSTOPSIG
#		define WSTOPSIG(status)		(_W_U(status)->w_stopsig)
#	endif
#	ifndef WIFCONTINUED
#		define	WIFCONTINUED(status)	(0)
#	endif
#	ifndef WIFSTOPPED
#		define WIFSTOPPED(status)	(_W_U(status)->w_stopval == \
								WSTOPFLG)
#	endif
#	ifndef WIFSIGNALED
#		define WIFSIGNALED(status) 	(_W_U(status)->w_stopval != \
						WSTOPFLG && \
						_W_U(status)->w_termsig != 0)
#	endif
#	ifndef WIFEXITED
#		define WIFEXITED(status)	(_W_U(status)->w_stopval != \
						WSTOPFLG && \
						_W_U(status)->w_termsig == 0)
#	endif
#else
#	define WAIT_T int
#	define	_W_I(w)	(*(int *)&(w))
#	ifndef WTERMSIG
#		define WTERMSIG(status)		(_W_I(status) & 0x7F)
#	endif
#	ifndef WCOREDUMP
#	ifdef WIFCORED				/* Haiku */
#		define WCOREDUMP(status)	(WIFCORED(_W_I(status)))
#	else
#		define WCOREDUMP(status)	(_W_I(status) & 0x80)
#	endif
#	endif
#	ifndef WEXITSTATUS
#		define WEXITSTATUS(status)	((_W_I(status) >> 8) & 0xFF)
#	endif
#	ifndef WSTOPSIG
#		define WSTOPSIG(status)		((_W_I(status) >> 8) & 0xFF)
#	endif
/*
 * WIFSTOPPED and WIFSIGNALED match the definitions on older UNIX versions
 * e.g. SunOS-4.x or HP-UX
 */
#	ifndef WIFCONTINUED
#	ifdef	NO_WCONTINUED
#		define	WIFCONTINUED(status)	(0)
#	else
#		define	WIFCONTINUED(status)	((_W_I(status) & 0xFFFF) == \
								WCONTFLG)
#	endif
#	endif
#	ifndef WIFSTOPPED
#		define	WIFSTOPPED(status)	((_W_I(status) & 0xFF) == 0x7F)
#	endif
#	ifndef WIFSIGNALED
#		define	WIFSIGNALED(status)	((_W_I(status) & 0xFF) != 0x7F && \
						WTERMSIG(status) != 0)
#	endif
#	ifndef WIFEXITED
#		define	WIFEXITED(status)	((_W_I(status) & 0xFF) == 0)
#	endif
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_WAIT_H */
