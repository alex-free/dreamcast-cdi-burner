/* @(#)sunos4_proto.h	1.3 13/09/14 Copyright 2013 J. Schilling */
/*
 *	Prototypes for POSIX standard functions that are missing on SunOS-4.x.
 *
 *	Copyright (c) 2013 J. Schilling
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


#ifndef _SCHILY_SUNOS4_PROTO_H
#define	_SCHILY_SUNOS4_PROTO_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * ctype.h
 */
#if	defined(_SCHILY_CTYPE_H) || defined(_SCHILY_WCTYPE_H)
extern	int		tolower __PR((int __c));
extern	int		toupper __PR((int __c));
#endif

/*
 * pwd.h
 */
#ifdef	_SCHILY_PWD_H
extern	void		endpwent __PR((void));
#endif

/*
 * poll.h
 */
/*#ifdef	_sys_poll_h*/
#ifdef	_SCHILY_POLL_H
extern	int		poll __PR((struct pollfd __fds[], unsigned long __nfds, int __timeout));
#endif

/*
 * schily/hostname.h
 */
#ifdef	_SCHILY_HOSTNAME_H
extern	int		gethostname	__PR((char *name, int namelen));
extern	int		getdomainname	__PR((char *name, int namelen));
#endif

/*
 * signal.h
 */
#ifdef	_SCHILY_SIGNAL_H
extern	int		killpg __PR((pid_t __pgrp, int __sig));
extern	int		sigvec __PR((int __sig, struct sigvec *__nvec, struct sigvec *__ovec));
#endif

/*
 * stdio.h
 */
#ifdef	_INCL_STDIO_H
extern	int		fclose __PR((FILE *__stream));
/*
 * XXX libshedit contains #define fflush(a) (0) that would cause a syntax error
 * XXX on a K&R compiler.
 */
#ifndef	fflush
extern	int		fflush __PR((FILE *__stream));
#endif
extern	int		fgetc __PR((FILE *__stream));
extern	int		_filbuf __PR((FILE *__stream));
extern	int		_flsbuf __PR((int __c, FILE *__stream));
extern	int		fputc __PR((int __c, FILE *__stream));
extern	int		fputs __PR((const char *__s, FILE *__stream));
extern	int		fprintf __PR((FILE *__stream, const char * __format, ...));
extern	size_t		fread __PR((void *__ptr, size_t __size, size_t __nitems, FILE *__stream));
extern	int		fseek __PR((FILE *__stream, long __offset, int __whence));
extern	size_t		fwrite __PR((void *__ptr, size_t __size, size_t __nitems, FILE *__stream));
extern	int		pclose __PR((FILE *__stream));
extern	void		rewind __PR((FILE *__stream));
extern	void		setbuf __PR((FILE *__stream, char *__buf));
extern	int		setvbuf __PR((FILE *__stream, char *__buf, int __type, size_t __size));
extern	int		scanf __PR((const char * __format, ...));
extern	int		sscanf __PR((const char * __s, const char * __format, ...));
extern	int		ungetc __PR((int __c, FILE *__stream));
#ifdef	_SCHILY_VARARGS_H
extern	int		vfprintf __PR((FILE *__stream, const char *__format, va_list __ap));
#endif
extern	void		perror __PR((const char *__s));
extern	int		printf __PR((const char * __format, ...));

/* Kommt von libschily */
/*PRINTFLIKE3*/
extern	int		snprintf __PR((char * __s, size_t __n,
					const char * __format, /* args*/ ...))
							__printflike__(3, 4);
#endif

/*
 * stdlib.h
 */
#ifdef	_INCL_STDLIB_H
/*
 * XXX cdda2wav includes a #define atexit(f) on_exit(f, 0) that
 * XXX would cause syntax errors with the next prototype on a K&R compiler.
 */
#ifndef	atexit
extern	int		atexit __PR((void (*__func)(void)));
#endif
extern	int		on_exit __PR((void (*__procp)(void), caddr_t __arg));
extern	char *		ecvt __PR((double __value, int __ndigit, int * __decpt, int * __sign));
/*
 * XXX Sun has extern int free(), but GCC has extern void free() in stdlib.h
 */
/*extern	void		free __PR((void *__ptr));*/
extern	char *		fcvt __PR((double __value, int __ndigit, int * __decpt, int * __sign));
extern	char *		gcvt __PR((double __value, int __ndigit, char * __buf));
extern	int		mkstemp __PR((char *__template));
extern	char *		mktemp __PR((char *__template));
extern	int		putenv __PR((char *__string));
extern	long		strtol __PR((const char * __str, char ** __endptr, int __base));
extern	int		system __PR((const char *_string));
extern	void *		valloc __PR((size_t __size));
#endif

/*
 * string.h
 */
#ifdef	_SCHILY_STRING_H
extern	void *		memchr __PR((const void *__s, int __c, size_t __n));
extern	int		strcoll __PR((const char *__s1, const char *__s2));
extern	int		strncasecmp __PR((const char *__s1, const char *__s2, size_t __n));

/*
 * strings.h
 */
extern	void		bcopy __PR((const void *__s1, void *__s2, size_t __n));
extern	void		bzero __PR((void *__s, size_t __n));
#endif

/*
 * sys/file.h
 */
#ifdef	_SCHILY_FCNTL_H
extern	int		flock __PR((int __fd, int __operation));
#endif

/*
 * sys/mman.h
 */
#ifdef	_SCHILY_MMAN_H
extern	int		mlock __PR((caddr_t __addr, size_t __len));
extern	int		mlockall __PR((int __flags));
#endif

/*
 * sys/resource.h
 */
#ifdef	_SCHILY_RESOURCE_H
extern	int		getrlimit __PR((int __resource, struct rlimit *__rlp));
extern	int		setrlimit __PR((int __resource, const struct rlimit *__rlp));
extern	int		getrusage __PR((int __who, struct rusage *__r_usage));
extern	int		getpriority __PR((int __which, int __who));
extern	int		setpriority __PR((int _which, int __who, int __value));

extern	pid_t		wait3 __PR((int *__statusp, int __options, struct rusage *__rusage));
#endif

/*
 * sys/shm.h
 */
#ifdef	_SCHILY_SHM_H
extern	void *		shmat __PR((int __shmid, const void *__shmaddr, int __shmflg));
extern	int		shmctl __PR((int __shmid, int __cmd, struct shmid_ds *__buf));
extern	int		shmget __PR((key_t __key, size_t __size, int __shmflg));
#endif

/*
 * sys/socket.h
 */
#ifdef	_SCHILY_SOCKET_H
extern	int		connect __PR((int __s, const struct sockaddr *__name, int __namelen));
extern	int		socket __PR((int __domain, int __type, int __protocol));
extern	int		getsockopt __PR((int __s, int __level, int __optname, void *__optval,
					int *__optlen));
extern	int		setsockopt __PR((int __s, int __level, int __optname, const void *__optval,
					int __optlen));
extern	int		getpeername __PR((int __s, struct sockaddr *__name, socklen_t *__namelen));
extern	int		socketpair __PR((int __domain, int __type, int __protocol, int __sv[2]));
#endif

/*
 * sys/stat.h
 */
#ifdef	_INCL_SYS_STAT_H
extern	int		stat __PR((const char * __path, struct stat * __buf));
extern	int		lstat __PR((const char * __path, struct stat * __buf));
extern	int		fchmod __PR((int __fildes, mode_t __mode));
extern	int		fstat __PR((int __filedes, struct stat * __buf));
extern	int		mknod __PR((const char *__path, mode_t __mode, dev_t __dev));
#endif

/*
 * sys/time.h
 */
#ifdef	_INCL_SYS_TIME_H
extern	int		gettimeofday __PR((struct timeval *__tp, void *__tzp));
extern	int		settimeofday __PR((struct timeval *__tp, void *__tzp));
extern	int		utimes __PR((const char *__path, const struct timeval __times[2]));
extern	int		select __PR((int __nfds, fd_set * __readfds, fd_set * __writefds,
						fd_set * __errorfds,
						struct timeval * __timeout));

#endif

/*
 * sys/timeb.h
 */
#ifdef	_sys_timeb_h
extern	int		ftime __PR((struct timeb *__tp));
#endif

/*
 * time.h
 */
#ifdef	_SCHILY_TIME_H
extern	clock_t		clock  __PR((void));
extern	time_t		mktime __PR((struct tm *__timeptr));
extern	size_t		strftime __PR((char * _s, size_t _maxsize,
						const char * __format,
						const struct tm * __timeptr));
extern	time_t		time __PR((time_t *__tloc));
#endif

/*
 * unistd.h
 */
#ifdef	_SCHILY_UNISTD_H
/*extern	int		chdir __PR((const char *__path));*/
extern	int		fchdir __PR((int __fildes));
extern	int		fsync __PR((int __fildes));
extern	int		getdtablesize __PR((void));
extern	long		gethostid __PR((void));
extern	int		getopt __PR((int __argc, char * const __argv[], const char *__optstring));
extern	int		ioctl __PR((int __fildes, int __request, /* arg */ ...));
extern	int		lockf __PR((int __fildes, int __function, off_t __size));
extern	int		rcmd __PR((char **__ahost, unsigned short __inport, const char *__luser,
					const char *__ruser, const char *__cmd, int *__fd2p));
extern	ssize_t		readlink __PR((const char * __path, char * __buf, size_t __bufsiz));
extern	int		rename __PR((const char *__old, const char *__new));
extern	int		setreuid __PR((uid_t __ruid, uid_t __euid));
extern	int		seteuid __PR((uid_t __euid));
extern	int		setegid __PR((gid_t __egid));

extern	int		symlink __PR((const char *__name1, const char *__name2));
extern	void		sync __PR((void));

extern	int		truncate __PR((const char *__path, off_t __length));
extern	int		ftruncate __PR((int __fildes, off_t __length));

extern	int		brk __PR((void *__endds));
/*extern	void *		sbrk __PR((intptr_t __incr));*/
extern	void *		sbrk __PR((Intptr_t __incr));

/*extern	int		usleep __PR((useconds_t __useconds));*/
extern	int		usleep __PR((unsigned __useconds));
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_SUNOS4_PROTO_H */
