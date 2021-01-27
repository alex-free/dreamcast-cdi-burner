/* @(#)astoi.c	1.11 15/12/10 Copyright 1985, 1995-2015 J. Schilling */
/*
 *	astoi() converts a string to int
 *	astol() converts a string to long
 *
 *	Leading tabs and spaces are ignored.
 *	Both return pointer to the first char that has not been used.
 *	Caller must check if this means a bad conversion.
 *
 *	leading "+" is ignored
 *	leading "0"  makes conversion octal (base 8)
 *	leading "0x" makes conversion hex   (base 16)
 *
 *	Copyright (c) 1985, 1995-2015 J. Schilling
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

#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/errno.h>

#define	is_space(c)	 ((c) == ' ' || (c) == '\t')
#define	is_digit(c)	 ((c) >= '0' && (c) <= '9')
#define	is_hex(c)	(\
			((c) >= 'a' && (c) <= 'f') || \
			((c) >= 'A' && (c) <= 'F'))

#define	is_lower(c)	((c) >= 'a' && (c) <= 'z')
#define	is_upper(c)	((c) >= 'A' && (c) <= 'Z')
#define	to_lower(c)	(((c) >= 'A' && (c) <= 'Z') ? (c) - 'A'+'a' : (c))

#if	('i' + 1) < 'j'
#define	BASE_MAX	('i' - 'a' + 10 + 1)	/* This is EBCDIC */
#else
#define	BASE_MAX	('z' - 'a' + 10 + 1)	/* This is ASCII */
#endif

#ifdef	notdef
EXPORT int
atoi(s)
	char	*s;
{
	long	l;

	(void) astol(s, &l);
	return ((int)l);
}

EXPORT long
atol(s)
	char	*s;
{
	long	l;

	(void) astol(s, &l);
	return (l);
}
#endif

EXPORT char *
astoi(s, i)
	const char *s;
	int *i;
{
	long l;
	char *ret;

	ret = astol(s, &l);
	*i = l;
#if	SIZEOF_INT != SIZEOF_LONG_INT
	if (*i != l) {
		if (l < 0) {
			*i = TYPE_MINVAL(int);
		} else {
			*i = TYPE_MAXVAL(int);
		}
		seterrno(ERANGE);
	}
#endif
	return (ret);
}

EXPORT char *
astol(s, l)
	register const char *s;
	long *l;
{
	return (astolb(s, l, 0));
}

EXPORT char *
astolb(s, l, base)
	register const char *s;
	long *l;
	register int base;
{
	int neg = 0;
	register unsigned long ret = 0L;
		unsigned long maxmult;
		unsigned long maxval;
	register int digit;
	register char c;

	if (base > BASE_MAX || base == 1 || base < 0) {
		seterrno(EINVAL);
		return ((char *)s);
	}

	while (is_space(*s))
		s++;

	if (*s == '+') {
		s++;
	} else if (*s == '-') {
		s++;
		neg++;
	}

	if (base == 0) {
		if (*s == '0') {
			base = 8;
			s++;
			if (*s == 'x' || *s == 'X') {
				s++;
				base = 16;
			}
		} else {
			base = 10;
		}
	}
	if (neg) {
		/*
		 * Portable way to compute the positive value of "min-long"
		 * as -TYPE_MINVAL(long) does not work.
		 */
		maxval = ((unsigned long)(-1 * (TYPE_MINVAL(long)+1))) + 1;
	} else {
		maxval = TYPE_MAXVAL(long);
	}
	maxmult = maxval / base;
	for (; (c = *s) != 0; s++) {

		if (is_digit(c)) {
			digit = c - '0';
		} else if (is_lower(c)) {
			digit = c - 'a' + 10;
		} else if (is_upper(c)) {
			digit = c - 'A' + 10;
		} else {
			break;
		}

		if (digit < base) {
			if (ret > maxmult)
				goto overflow;
			ret *= base;
			if (maxval - ret < digit)
				goto overflow;
			ret += digit;
		} else {
			break;
		}
	}
	if (neg) {
		*l = (Llong)-1 * ret;
	} else {
		*l = (Llong)ret;
	}
	return ((char *)s);
overflow:
	for (; (c = *s) != 0; s++) {

		if (is_digit(c)) {
			digit = c - '0';
		} else if (is_lower(c)) {
			digit = c - 'a' + 10;
		} else if (is_upper(c)) {
			digit = c - 'A' + 10;
		} else {
			break;
		}
		if (digit >= base)
			break;
	}
	if (neg) {
		*l = TYPE_MINVAL(long);
	} else {
		*l = TYPE_MAXVAL(long);
	}
	seterrno(ERANGE);
	return ((char *)s);
}
