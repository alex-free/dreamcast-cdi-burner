/* @(#)spawn.c	1.30 15/07/06 Copyright 1985, 1989, 1995-2015 J. Schilling */
/*
 *	Spawn another process/ wait for child process
 *
 *	Copyright (c) 1985, 1989, 1995-2015 J. Schilling
 *
 *	This is an interface that exists in the public since 1982.
 *	The POSIX.1-2008 standard did ignore POSIX rules not to
 *	redefine existing public interfaces and redefined the interfaces
 *	forcing us to add a js_*() prefix to the original functions.
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

#ifndef	__DO__FSPAWNL__
#define	fspawnv	__no__fspawnv__
#define	fspawnl	__no__fspawnl__
#define	fspawnv_nowait	__no__fspawnv_nowait__

#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/varargs.h>
#include <schily/wait.h>
#include <schily/errno.h>
#include <schily/schily.h>
#define	VMS_VFORK_OK
#include <schily/vfork.h>
#endif	/* __DO__FSPAWNL__ */

#define	MAX_F_ARGS	16

#ifndef	__DO__FSPAWNL__
#ifndef	NO_FSPAWN_COMPAT	/* Define to disable backward compatibility */
#undef	fspawnv
#undef	fspawnl
#undef	fspawnv_nowait

/*
 * The Cygwin compile environment incorrectly implements #pragma weak.
 * The weak symbols are only defined as local symbols making it impossible
 * to use them from outside the scope of this source file.
 * A platform that allows linking with global symbols has HAVE_LINK_WEAK
 * defined.
 */
#if defined(HAVE_PRAGMA_WEAK) && defined(HAVE_LINK_WEAK)
#pragma	weak fspawnv =	js_fspawnv
#pragma	weak fspawnl =	js_fspawnl
#pragma	weak fspawnv_nowait =	js_fspawnv_nowait
#else

EXPORT	int	fspawnv __PR((FILE *, FILE *, FILE *, int, char * const *));
EXPORT	int	fspawnl __PR((FILE *, FILE *, FILE *, const char *, ...));
EXPORT	int	fspawnv_nowait __PR((FILE *, FILE *, FILE *,
					const char *, int, char *const*));
EXPORT int
fspawnv(in, out, err, argc, argv)
	FILE	*in;
	FILE	*out;
	FILE	*err;
	int	argc;
	char	* const argv[];
{
	return (js_fspawnv(in, out, err, argc, argv));
}

EXPORT int
fspawnv_nowait(in, out, err, name, argc, argv)
	FILE		*in;
	FILE		*out;
	FILE		*err;
	const char	*name;
	int		argc;
	char		* const argv[];
{
	return (js_fspawnv_nowait(in, out, err, name, argc, argv));
}

#define	__DO__FSPAWNL__
#define	js_fspawnl	fspawnl
#include "spawn.c"
#undef	js_fspawnl
#undef	__DO__FSPAWNL__

#endif	/* HAVE_PRAGMA_WEAK && HAVE_LINK_WEAK */
#endif	/* NO_FSPAWN_COMPAT */

EXPORT int
js_fspawnv(in, out, err, argc, argv)
	FILE	*in;
	FILE	*out;
	FILE	*err;
	int	argc;
	char	* const argv[];
{
	int	pid;

	if ((pid = js_fspawnv_nowait(in, out, err, argv[0], argc, argv)) < 0)
		return (pid);

	return (wait_chld(pid));
}
#endif	/* __DO__FSPAWNL__ */

/* VARARGS3 */
#ifdef	PROTOTYPES
EXPORT int
js_fspawnl(FILE *in, FILE *out, FILE *err, const char *arg0, ...)
#else
EXPORT int
js_fspawnl(in, out, err, arg0, va_alist)
	FILE		*in;
	FILE		*out;
	FILE		*err;
	const char	*arg0;
	va_dcl
#endif
{
	va_list	args;
	int	ac = 0;
	char	*xav[MAX_F_ARGS+1];
	char	**av;
const	char	**pav;
	char	*p;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, arg0);
#else
	va_start(args);
#endif
	if (arg0) {
		ac++;
		while (va_arg(args, char *) != NULL)
			ac++;
	}
	va_end(args);

	if (ac <= MAX_F_ARGS) {
		av = xav;
	} else {
		av = (char **)malloc((ac+1)*sizeof (char *));
		if (av == 0)
			return (-1);
	}
	pav = (const char **)av;

#ifdef	PROTOTYPES
	va_start(args, arg0);
#else
	va_start(args);
#endif
	*pav++ = arg0;
	if (arg0) do {
		p = va_arg(args, char *);
		*pav++ = p;
	} while (p != NULL);
	va_end(args);

	ret =  js_fspawnv(in, out, err, ac, av);
	if (av != xav)
		free(av);
	return (ret);
}

#ifndef	__DO__FSPAWNL__
EXPORT int
js_fspawnv_nowait(in, out, err, name, argc, argv)
	FILE		*in;
	FILE		*out;
	FILE		*err;
	const char	*name;
	int		argc;
	char		* const argv[];
{
	int	pid = -1;	/* Initialization needed to make GCC happy */
	volatile int	i;

	for (i = 1; i < 64; i *= 2) {
		if ((pid = vfork()) >= 0)
			break;
		sleep(i);
	}
	if (pid != 0)
		return (pid);
				/*
				 * silly: fexecv must set av[ac] = NULL
				 * so we have to cast argv tp (char **)
				 */
	fexecv(name, in, out, err, argc, (char **)argv);
	_exit(geterrno());
	/* NOTREACHED */
#ifndef	lint
	return (0);		/* keep gnu compiler happy */
#endif
}

EXPORT int
wait_chld(pid)
	int	pid;
{
	int	died;
	WAIT_T	status;

	do {
		do {
			died = wait(&status);
		} while (died < 0 && geterrno() == EINTR);
		if (died < 0)
			return (died);
	} while (died != pid);

	if (WCOREDUMP(status))
		unlink("core");

	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	return (-WTERMSIG(status));
}
#endif	/* __DO__FSPAWNL__ */
