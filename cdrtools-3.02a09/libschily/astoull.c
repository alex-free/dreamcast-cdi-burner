/* @(#)astoull.c	1.2 09/07/26 Copyright 1985, 2000-2006 J. Schilling */
/*
 *	astoll() converts a string to long long
 *
 *	Leading tabs and spaces are ignored.
 *	Both return pointer to the first char that has not been used.
 *	Caller must check if this means a bad conversion.
 *
 *	leading "+" is ignored
 *	leading "0"  makes conversion octal (base 8)
 *	leading "0x" makes conversion hex   (base 16)
 *
 *	Llong is silently reverted to long if the compiler does not
 *	support long long.
 *
 *	Copyright (c) 1985, 2000-2006 J. Schilling
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
#include <schily/utypes.h>
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



EXPORT	char *astoull	__PR((const char *s, Ullong *l));
EXPORT	char *astoullb	__PR((const char *s, Ullong *l, int base));

EXPORT char *
astoull(s, l)
	register const char *s;
	Ullong *l;
{
	return (astoullb(s, l, 0));
}

EXPORT char *
astoullb(s, l, base)
	register const char *s;
	Ullong *l;
	register int base;
{
#ifdef	DO_SIGNED
	int neg = 0;
#endif
	register Ullong ret = (Ullong)0;
		Ullong maxmult;
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
#ifndef	DO_SIGNED
		seterrno(EINVAL);
		return ((char *)s);
#else
		s++;
		neg++;
#endif
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
	maxmult = TYPE_MAXVAL(Ullong) / base;
	for (; (c = *s) != 0; s++) {

		if (is_digit(c)) {
			digit = c - '0';
#ifdef	OLD
		} else if (is_hex(c)) {
			digit = to_lower(c) - 'a' + 10;
#else
		} else if (is_lower(c)) {
			digit = c - 'a' + 10;
		} else if (is_upper(c)) {
			digit = c - 'A' + 10;
#endif
		} else {
			break;
		}

		if (digit < base) {
			if (ret > maxmult)
				goto overflow;
			ret *= base;
			if (TYPE_MAXVAL(Ullong) - ret < digit)
				goto overflow;
			ret += digit;
		} else {
			break;
		}
	}
#ifdef	DO_SIGNED
	if (neg)
		ret = -ret;
#endif
	*l = ret;
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
	*l = TYPE_MAXVAL(Ullong);
	seterrno(ERANGE);
	return ((char *)s);
}
