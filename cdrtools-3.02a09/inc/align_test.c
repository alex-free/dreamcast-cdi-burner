/* @(#)align_test.c	1.31 15/11/30 Copyright 1995-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef	lint
static	UConst char sccsid[] =
	"@(#)align_test.c	1.31 15/11/30 Copyright 1995-2015 J. Schilling";
#endif
/*
 *	Generate machine dependant align.h
 *
 *	Copyright (c) 1995-2015 J. Schilling
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

#include <schily/stdio.h>
#include <schily/standard.h>
#undef	NO_LONGLONG	/* Make sure that Llong will be long long */
#include <schily/utypes.h>
#include <schily/libport.h>	/* Define missing prototypes */

/*
 * Be very careful here as MSVC does not implement long long but rather __int64
 * and once someone makes 'long long' 128 bits on a 64 bit machine, we need to
 * check for a MSVC __int128 type.
 */

/*
 * CHECK_ALIGN needs SIGBUS, but DJGPP has no SIGBUS
 */
/*#define	FORCE_ALIGN*/
/*#define	OFF_ALIGN*/
/*#define	CHECK_ALIGN*/

EXPORT	int	main	__PR((int ac, char **av));

#if	!defined(FORCE_ALIGN) && !defined(OFF_ALIGN) &&	!defined(CHECK_ALIGN)
#define	OFF_ALIGN
#endif

char	buf[8192+1024];
char	*buf_aligned;

#ifdef	FORCE_ALIGN
#	undef	CHECK_ALIGN
#	undef	OFF_ALIGN
#endif

#ifdef	CHECK_ALIGN
#	undef	FORCE_ALIGN
#	undef	OFF_ALIGN
#endif

#ifdef	OFF_ALIGN
#	undef	FORCE_ALIGN
#	undef	CHECK_ALIGN
#endif


#ifdef	FORCE_ALIGN

#define	ALIGN_short	sizeof (short)
#define	ALIGN_int	sizeof (int)
#define	ALIGN_long	sizeof (long)
#define	ALIGN_longlong	sizeof (Llong)
#define	ALIGN_float	sizeof (float)
#define	ALIGN_double	sizeof (double)
#define	ALIGN_ldouble	sizeof (long double)
#define	ALIGN_ptr	sizeof (char *)

#endif

#ifdef	CHECK_ALIGN

#include <schily/signal.h>
#include <schily/setjmp.h>
LOCAL	jmp_buf	jb;

LOCAL	int	check_align	__PR((int (*)(char *, int),
					void (*)(char *, int), int));
LOCAL	int	check_short	__PR((char *, int));
LOCAL	int	check_int	__PR((char *, int));
LOCAL	int	check_long	__PR((char *, int));
LOCAL	int	check_longlong	__PR((char *, int));
LOCAL	int	check_float	__PR((char *, int));
LOCAL	int	check_double	__PR((char *, int));
#ifdef	HAVE_LONGDOUBLE
LOCAL	int	check_ldouble	__PR((char *, int));
#endif
LOCAL	int	check_ptr	__PR((char *, int));

LOCAL	int	speed_check	__PR((char *,
					void (*)(char *, int), int));
LOCAL	void	speed_short	__PR((char *, int));
LOCAL	void	speed_int	__PR((char *, int));
LOCAL	void	speed_long	__PR((char *, int));
LOCAL	void	speed_longlong	__PR((char *, int));
LOCAL	void	speed_float	__PR((char *, int));
LOCAL	void	speed_double	__PR((char *, int));
#ifdef	HAVE_LONGDOUBLE
LOCAL	void	speed_ldouble	__PR((char *, int));
#endif
LOCAL	void	speed_ptr	__PR((char *, int));

#define	ALIGN_short	check_align(check_short, speed_short, sizeof (short))
#define	ALIGN_int	check_align(check_int, speed_int, sizeof (int))
#define	ALIGN_long	check_align(check_long, speed_long, sizeof (long))
#define	ALIGN_longlong	check_align(check_longlong, speed_longlong, sizeof (Llong))
#define	ALIGN_float	check_align(check_float, speed_float, sizeof (float))
#define	ALIGN_double	check_align(check_double, speed_double, sizeof (double))
#define	ALIGN_ldouble	check_align(check_ldouble, speed_ldouble, sizeof (long double))
#define	ALIGN_ptr	check_align(check_ptr, speed_ptr, sizeof (char *))

#endif

#ifdef	OFF_ALIGN

#define	sm_off(s, m)	((size_t)&((s)0)->m)

LOCAL	void	used		__PR((int i));
LOCAL	int	off_short	__PR((void));
LOCAL	int	off_int		__PR((void));
LOCAL	int	off_long	__PR((void));
LOCAL	int	off_longlong	__PR((void));
LOCAL	int	off_float	__PR((void));
LOCAL	int	off_double	__PR((void));
#ifdef	HAVE_LONGDOUBLE
LOCAL	int	off_ldouble	__PR((void));
#endif
LOCAL	int	off_ptr		__PR((void));

#define	ALIGN_short	off_short()
#define	ALIGN_int	off_int()
#define	ALIGN_long	off_long()
#define	ALIGN_longlong	off_longlong()
#define	ALIGN_float	off_float()
#define	ALIGN_double	off_double()
#define	ALIGN_ldouble	off_ldouble()
#define	ALIGN_ptr	off_ptr()

#endif

LOCAL	void	printmacs	__PR((void));

#ifdef	CHECK_ALIGN
LOCAL	void	sig		__PR((int));

LOCAL void
sig(signo)
	int	signo;
{
	signal(signo, sig);
	longjmp(jb, 1);
}
#endif

#if defined(mc68000) || defined(mc68020)
#define	MIN_ALIGN	2
#else
#define	MIN_ALIGN	2
#endif


#define	min_align(i)	(((i) < MIN_ALIGN) ? MIN_ALIGN : (i))

/*
 * Make it LOCAL MacOS-X by default links against libcurses and
 * so we totherwise have a double defined "al".
 */
LOCAL	char	al[] = "alignment value for ";
LOCAL	char	ms[] = "alignment mask  for ";
LOCAL	char	so[] = "sizeof ";
LOCAL	char	sh[] = "short";
LOCAL	char	in[] = "int";
LOCAL	char	lo[] = "long";
LOCAL	char	ll[] = "long long";
LOCAL	char	fl[] = "float";
LOCAL	char	db[] = "double";
LOCAL	char	ld[] = "long double";
LOCAL	char	pt[] = "pointer";
LOCAL	char	mt[] = "max type";

#define	xalign(x, a, m)		(((char *)(x)) + ((a) - (((UIntptr_t)(x))&(m))))

EXPORT int
main(ac, av)
	int	ac;
	char	**av;
{
	char	*p;
	int	i;
	int	s;
	int	amax = 0;
	int	smax = 0;

#ifdef	CHECK_ALIGN
#ifdef	SIGBUS
	signal(SIGBUS, sig);
#endif
#endif

	i = ((size_t)buf) % 1024;
	i = 1024 - i;
	p = &buf[i];
	buf_aligned = p;

#ifdef	DEBUG
	fprintf(stderr, "buf: 0x%lX 0x%lX\n",
		(unsigned long)buf, (unsigned long)xalign(buf, 1024, 1023));
#endif

	printf("/*\n");
	printf(" * This file has been generated automatically\n");
	printf(" * by %s\n", sccsid);
	printf(" * do not edit by hand.\n");
	printf(" */\n");
	printf("#ifndef	__ALIGN_H\n");
	printf("#define	__ALIGN_H\n\n");

	s = sizeof (short);
	i  = ALIGN_short;
	i = min_align(i);
	if (i > amax)
		amax = i;
	if (s > smax)
		smax = s;
	printf("\n");
	printf("#define	ALIGN_SHORT	%d\t/* %s(%s *)\t*/\n", i, al, sh);
	printf("#define	ALIGN_SMASK	%d\t/* %s(%s *)\t*/\n", i-1, ms, sh);
	printf("#define	SIZE_SHORT	%d\t/* %s(%s)\t\t\t*/\n", s, so, sh);

	s = sizeof (int);
	i  = ALIGN_int;
	i = min_align(i);
	if (i > amax)
		amax = i;
	if (s > smax)
		smax = s;
	printf("\n");
	printf("#define	ALIGN_INT	%d\t/* %s(%s *)\t\t*/\n", i, al, in);
	printf("#define	ALIGN_IMASK	%d\t/* %s(%s *)\t\t*/\n", i-1, ms, in);
	printf("#define	SIZE_INT	%d\t/* %s(%s)\t\t\t\t*/\n", s, so, in);

	s = sizeof (long);
	i  = ALIGN_long;
	i = min_align(i);
	if (i > amax)
		amax = i;
	if (s > smax)
		smax = s;
	printf("\n");
	printf("#define	ALIGN_LONG	%d\t/* %s(%s *)\t\t*/\n", i, al, lo);
	printf("#define	ALIGN_LMASK	%d\t/* %s(%s *)\t\t*/\n", i-1, ms, lo);
	printf("#define	SIZE_LONG	%d\t/* %s(%s)\t\t\t*/\n", s, so, lo);

#ifdef	HAVE_LONGLONG
	s = sizeof (Llong);
	i  = ALIGN_longlong;
	i = min_align(i);
	if (i > amax)
		amax = i;
	if (s > smax)
		smax = s;
#endif
	printf("\n");
	printf("#define	ALIGN_LLONG	%d\t/* %s(%s *)\t*/\n", i, al, ll);
	printf("#define	ALIGN_LLMASK	%d\t/* %s(%s *)\t*/\n", i-1, ms, ll);
	printf("#define	SIZE_LLONG	%d\t/* %s(%s)\t\t\t*/\n", s, so, ll);

	s = sizeof (float);
	i  = ALIGN_float;
	i = min_align(i);
	if (i > amax)
		amax = i;
	if (s > smax)
		smax = s;
	printf("\n");
	printf("#define	ALIGN_FLOAT	%d\t/* %s(%s *)\t*/\n", i, al, fl);
	printf("#define	ALIGN_FMASK	%d\t/* %s(%s *)\t*/\n", i-1, ms, fl);
	printf("#define	SIZE_FLOAT	%d\t/* %s(%s)\t\t\t*/\n", s, so, fl);

	s = sizeof (double);
	i  = ALIGN_double;
	i = min_align(i);
	if (i > amax)
		amax = i;
	if (s > smax)
		smax = s;
	printf("\n");
	printf("#define	ALIGN_DOUBLE	%d\t/* %s(%s *)\t*/\n", i, al, db);
	printf("#define	ALIGN_DMASK	%d\t/* %s(%s *)\t*/\n", i-1, ms, db);
	printf("#define	SIZE_DOUBLE	%d\t/* %s(%s)\t\t\t*/\n", s, so, db);

#ifdef	HAVE_LONGDOUBLE
	s = sizeof (long double);
	i  = ALIGN_ldouble;
	i = min_align(i);
	if (i > amax)
		amax = i;
	if (s > smax)
		smax = s;
#endif
	printf("\n");
	printf("#define	ALIGN_LDOUBLE	%d\t/* %s(%s *)\t*/\n", i, al, ld);
	printf("#define	ALIGN_LDMASK	%d\t/* %s(%s *)\t*/\n", i-1, ms, ld);
	printf("#define	SIZE_LDOUBLE	%d\t/* %s(%s)\t\t\t*/\n", s, so, ld);

	s = sizeof (char *);
	i  = ALIGN_ptr;
	i = min_align(i);
	if (i > amax)
		amax = i;
	if (s > smax)
		smax = s;
	printf("\n");
	printf("#define	ALIGN_PTR	%d\t/* %s(%s *)\t*/\n", i, al, pt);
	printf("#define	ALIGN_PMASK	%d\t/* %s(%s *)\t*/\n", i-1, ms, pt);
	printf("#define	SIZE_PTR	%d\t/* %s(%s)\t\t\t*/\n", s, so, pt);

	printf("\n");
	printf("#define	ALIGN_TMAX	%d\t/* %s(%s *)\t*/\n", amax, al, mt);
	printf("#define	ALIGN_TMMASK	%d\t/* %s(%s *)\t*/\n", amax-1, ms, mt);
	printf("#define	SIZE_TMAX	%d\t/* %s(%s)\t\t\t*/\n", smax, so, mt);

	printmacs();
	printf("\n#endif	/* __ALIGN_H */\n");
	fflush(stdout);
	return (0);
}

LOCAL void
printmacs()
{
printf("\n\n");
printf("/*\n * There used to be a cast to an int but we get a warning from GCC.\n");
printf(" * This warning message from GCC is wrong.\n");
printf(" * Believe me that this macro would even be usable if I would cast to short.\n");
printf(" * In order to avoid this warning, we are now using UIntptr_t\n */\n");
/*printf("\n");*/
/*printf("\n");*/
printf("#define	xaligned(a, s)		((((UIntptr_t)(a)) & (s)) == 0)\n");
printf("#define	x2aligned(a, b, s)	(((((UIntptr_t)(a)) | ((UIntptr_t)(b))) & (s)) == 0)\n");
printf("\n");
printf("#define	saligned(a)		xaligned(a, ALIGN_SMASK)\n");
printf("#define	s2aligned(a, b)		x2aligned(a, b, ALIGN_SMASK)\n");
printf("\n");
printf("#define	ialigned(a)		xaligned(a, ALIGN_IMASK)\n");
printf("#define	i2aligned(a, b)		x2aligned(a, b, ALIGN_IMASK)\n");
printf("\n");
printf("#define	laligned(a)		xaligned(a, ALIGN_LMASK)\n");
printf("#define	l2aligned(a, b)		x2aligned(a, b, ALIGN_LMASK)\n");
printf("\n");
printf("#define	llaligned(a)		xaligned(a, ALIGN_LLMASK)\n");
printf("#define	ll2aligned(a, b)	x2aligned(a, b, ALIGN_LLMASK)\n");
printf("\n");
printf("#define	faligned(a)		xaligned(a, ALIGN_FMASK)\n");
printf("#define	f2aligned(a, b)		x2aligned(a, b, ALIGN_FMASK)\n");
printf("\n");
printf("#define	daligned(a)		xaligned(a, ALIGN_DMASK)\n");
printf("#define	d2aligned(a, b)		x2aligned(a, b, ALIGN_DMASK)\n");
printf("\n");
printf("#define	ldaligned(a)		xaligned(a, ALIGN_LDMASK)\n");
printf("#define	ld2aligned(a, b)	x2aligned(a, b, ALIGN_LDMASK)\n");
printf("\n");
printf("#define	paligned(a)		xaligned(a, ALIGN_PMASK)\n");
printf("#define	p2aligned(a, b)		x2aligned(a, b, ALIGN_PMASK)\n");
printf("\n");
printf("#define	maligned(a)		xaligned(a, ALIGN_TMMASK)\n");
printf("#define	m2aligned(a, b)		x2aligned(a, b, ALIGN_TMMASK)\n");

printf("\n\n");
printf("/*\n * There used to be a cast to an int but we get a warning from GCC.\n");
printf(" * This warning message from GCC is wrong.\n");
printf(" * Believe me that this macro would even be usable if I would cast to short.\n");
printf(" * In order to avoid this warning, we are now using UIntptr_t\n */\n");
printf("#define	xalign(x, a, m)		(((char *)(x)) + ((a) - 1 - ((((UIntptr_t)(x))-1)&(m))))\n");
printf("\n");
printf("#define	salign(x)		xalign((x), ALIGN_SHORT, ALIGN_SMASK)\n");
printf("#define	ialign(x)		xalign((x), ALIGN_INT, ALIGN_IMASK)\n");
printf("#define	lalign(x)		xalign((x), ALIGN_LONG, ALIGN_LMASK)\n");
printf("#define	llalign(x)		xalign((x), ALIGN_LLONG, ALIGN_LLMASK)\n");
printf("#define	falign(x)		xalign((x), ALIGN_FLOAT, ALIGN_FMASK)\n");
printf("#define	dalign(x)		xalign((x), ALIGN_DOUBLE, ALIGN_DMASK)\n");
printf("#define	ldalign(x)		xalign((x), ALIGN_LDOUBLE, ALIGN_LDMASK)\n");
printf("#define	palign(x)		xalign((x), ALIGN_PTR, ALIGN_PMASK)\n");
printf("#define	malign(x)		xalign((x), ALIGN_TMAX, ALIGN_TMMASK)\n");
}

#ifdef	CHECK_ALIGN
/*
 * Routines to compute the alignement by checking if the assignement
 * causes a bus error.
 * Some systems (e.g. Linux on DEC Aplha) will allow to fetch any
 * type from any address. On these systems we must check the speed
 * because unaligned fetches will take more time.
 */
LOCAL int
check_align(cfunc, sfunc, tsize)
	int	(*cfunc)();
	void	(*sfunc)();
	int	tsize;
{
		int	calign;
		int	align;
		int	tcheck;
		int	t;
	register int	i;
	register char	*p = buf_aligned;

	for (i = 1; i < 128; i++) {
		if (!setjmp(jb)) {
			(cfunc)(p, i);
			break;
		}
	}
#ifdef	DEBUG
	fprintf(stderr, "i: %d tsize: %d\n", i, tsize);
#endif
	if (i == tsize)
		return (i);

	align = calign = i;
	tcheck = speed_check(p, sfunc, i);
#ifdef	DEBUG
	fprintf(stderr, "tcheck: %d\n", tcheck);
#endif

	for (i = calign*2; i <= tsize; i *= 2) {
		t = speed_check(p, sfunc, i);
#ifdef	DEBUG
		fprintf(stderr, "tcheck: %d t: %d i: %d\n", tcheck, t, i);
		fprintf(stderr, "tcheck - t: %d ... * 3: %d\n",  (tcheck - t), (tcheck - t) * 3);
#endif
		if (((tcheck - t) > 0) && ((tcheck - t) * 3) > tcheck) {
#ifdef	DEBUG
			fprintf(stderr, "kleiner\n");
#endif
			align = i;
			tcheck = t;
		}
	}
	return (align);
}

LOCAL int
check_short(p, i)
	char	*p;
	int	i;
{
	short	*sp;

	sp = (short *)&p[i];
	*sp = 1;
	return (0);
}

LOCAL int
check_int(p, i)
	char	*p;
	int	i;
{
	int	*ip;

	ip = (int *)&p[i];
	*ip = 1;
	return (0);
}

LOCAL int
check_long(p, i)
	char	*p;
	int	i;
{
	long	*lp;

	lp = (long *)&p[i];
	*lp = 1;
	return (0);
}

#ifdef	HAVE_LONGLONG
LOCAL int
check_longlong(p, i)
	char	*p;
	int	i;
{
	Llong	*llp;

	llp = (Llong *)&p[i];
	*llp = 1;
	return (0);
}
#endif

LOCAL int
check_float(p, i)
	char	*p;
	int	i;
{
	float	*fp;

	fp = (float *)&p[i];
	*fp = 1.0;
	return (0);
}

LOCAL int
check_double(p, i)
	char	*p;
	int	i;
{
	double	*dp;

	dp = (double *)&p[i];
	*dp = 1.0;
	return (0);
}

#ifdef	HAVE_LONGDOUBLE
LOCAL int
check_ldouble(p, i)
	char	*p;
	int	i;
{
	long double	*dp;

	dp = (long double *)&p[i];
	*dp = 1.0;
	return (0);
}
#endif

LOCAL int
check_ptr(p, i)
	char	*p;
	int	i;
{
	char	**pp;

	pp = (char  **)&p[i];
	*pp = (char *)1;
	return (0);
}

/*
 * Routines to compute the alignement by checking the speed of the
 * assignement.
 * Some systems (e.g. Linux on DEC Aplha) will allow to fetch any
 * type from any address. On these systems we must check the speed
 * because unaligned fetches will take more time.
 */
LOCAL void
speed_short(p, n)
	char	*p;
	int	n;
{
	short	*sp;
	int	i;

	sp = (short *)&p[n];

	for (i = 1000000; --i >= 0; )
		*sp = i;
}

LOCAL void
speed_int(p, n)
	char	*p;
	int	n;
{
	int	*ip;
	int	i;

	ip = (int *)&p[n];

	for (i = 1000000; --i >= 0; )
		*ip = i;
}

LOCAL void
speed_long(p, n)
	char	*p;
	int	n;
{
	long	*lp;
	int	i;

	lp = (long *)&p[n];

	for (i = 1000000; --i >= 0; )
		*lp = i;
}

#ifdef	HAVE_LONGLONG
LOCAL void
speed_longlong(p, n)
	char	*p;
	int	n;
{
	Llong *llp;
	int	i;

	llp = (Llong *)&p[n];

	for (i = 1000000; --i >= 0; )
		*llp = i;
}
#endif

LOCAL void
speed_float(p, n)
	char	*p;
	int	n;
{
	float	*fp;
	int	i;

	fp = (float *)&p[n];

	for (i = 1000000; --i >= 0; )
		*fp = i;
}

LOCAL void
speed_double(p, n)
	char	*p;
	int	n;
{
	double	*dp;
	int	i;

	dp = (double *)&p[n];

	for (i = 1000000; --i >= 0; )
		*dp = i;
}

#ifdef	HAVE_LONGDOUBLE
LOCAL void
speed_ldouble(p, n)
	char	*p;
	int	n;
{
	long double	*dp;
	int	i;

	dp = (long double *)&p[n];

	for (i = 1000000; --i >= 0; )
		*dp = i;
}
#endif

LOCAL void
speed_ptr(p, n)
	char	*p;
	int	n;
{
	char	**pp;
	int	i;

	pp = (char **)&p[n];

	for (i = 1000000; --i >= 0; )
		*pp = (char *)i;
}

#include <schily/time.h>
#include <schily/times.h>
LOCAL int
speed_check(p, sfunc, n)
	char	*p;
	void	(*sfunc)();
	int	n;
{
	struct tms tm1;
	struct tms tm2;

	times(&tm1);
	(*sfunc)(p, n);
	times(&tm2);

#ifdef	DEBUG
	fprintf(stderr, "t1: %ld\n", (long)tm2.tms_utime-tm1.tms_utime);
#endif

	return ((int)tm2.tms_utime-tm1.tms_utime);
}

#endif	/* CHECK_ALIGN */

#ifdef	OFF_ALIGN
/*
 * Routines to compute the alignement by using the knowledge
 * of the C-compiler.
 * We define a structure and check the padding that has been inserted
 * by the compiler to keep the apropriate type on a properly aligned
 * address.
 */
LOCAL	int	used_var;
LOCAL void
used(i)
	int	i;
{
	used_var = i;
}

LOCAL int
off_short()
{
	struct ss {
		char	c;
		short	s;
	} ss;
	ss.c = 0;		/* fool C-compiler */
	used(ss.c);

	return (sm_off(struct ss *, s));
}

LOCAL int
off_int()
{
	struct si {
		char	c;
		int	i;
	} si;
	si.c = 0;		/* fool C-compiler */
	used(si.c);

	return (sm_off(struct si *, i));
}

LOCAL int
off_long()
{
	struct sl {
		char	c;
		long	l;
	} sl;
	sl.c = 0;		/* fool C-compiler */
	used(sl.c);

	return (sm_off(struct sl *, l));
}

#ifdef	HAVE_LONGLONG
LOCAL int
off_longlong()
{
	struct sll {
		char	c;
		Llong	ll;
	} sll;
	sll.c = 0;		/* fool C-compiler */
	used(sll.c);

	return (sm_off(struct sll *, ll));
}
#endif

LOCAL int
off_float()
{
	struct sf {
		char	c;
		float	f;
	} sf;
	sf.c = 0;		/* fool C-compiler */
	used(sf.c);

	return (sm_off(struct sf *, f));
}

LOCAL int
off_double()
{
	struct sd {
		char	c;
		double	d;
	} sd;
	sd.c = 0;		/* fool C-compiler */
	used(sd.c);

	return (sm_off(struct sd *, d));
}

#ifdef	HAVE_LONGDOUBLE
LOCAL int
off_ldouble()
{
	struct sd {
		char		c;
		long double	ld;
	} sd;
	sd.c = 0;		/* fool C-compiler */
	used(sd.c);

	return (sm_off(struct sd *, ld));
}
#endif

LOCAL int
off_ptr()
{
	struct sp {
		char	c;
		char	*p;
	} sp;
	sp.c = 0;		/* fool C-compiler */
	used(sp.c);

	return (sm_off(struct sp *, p));
}

#endif	/* OFF_ALIGN */
