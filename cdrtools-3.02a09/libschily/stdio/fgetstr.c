/* @(#)fgetstr.c	1.12 16/11/07 Copyright 1986, 1996-2016 J. Schilling */
/*
 *	Copyright (c) 1986, 1996-2016 J. Schilling
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

#define	FAST_GETC_PUTC		/* Will be reset if not possible */
#include "schilyio.h"

#if !defined(getc) && defined(USE_FGETS_FOR_FGETSTR)
#include <schily/string.h>

/*
 * Warning: this prevents us from being able to have embedded null chars.
 */
EXPORT int
fgetstr(f, buf, len)
	register	FILE	*f;
			char	*buf;
	register	int	len;
{
	char	*bp = fgets(buf, len, f);

	if (bp) {
		return (strlen(bp));
	}
	buf[0] = '\0';
	return (-1);
}

#else

EXPORT int
fgetstr(f, buf, len)
	register	FILE	*f;
			char	*buf;
	register	int	len;
{
	register char	*bp	= buf;
#if	defined(HAVE_USG_STDIO) || defined(FAST_GETC_PUTC)
	register char	*p;
#else
	register int	c	= '\0';
	register int	nl	= '\n';
#endif

	down2(f, _IOREAD, _IORW);

	if (len <= 0)
		return (0);

	*bp = '\0';
	for (;;) {
#if	defined(HAVE_USG_STDIO) || defined(FAST_GETC_PUTC)
		size_t	n;

		if ((__c f)->_cnt <= 0) {
			if (usg_filbuf(f) == EOF) {
				/*
				 * If buffer is empty and we hit EOF, return EOF
				 */
				if (bp == buf)
					return (EOF);
				break;
			}
			(__c f)->_cnt++;
			(__c f)->_ptr--;
		}

		n = len;
		if (n > (__c f)->_cnt)
			n = (__c f)->_cnt;
		p = movecbytes((__c f)->_ptr, bp, '\n', n);
		if (p) {
			n = p - bp;
		}
		(__c f)->_ptr += n;
		(__c f)->_cnt -= n;
		bp += n;
		len -= n;
		if (p != NULL)
			break;
#else
		if ((c = getc(f)) < 0) {
			/*
			 * If buffer is empty and we hit EOF, return EOF
			 */
			if (bp == buf)
				return (c);
			break;
		}
		if (--len > 0)
			*bp++ = (char)c;
		if (c == nl)
			break;
#endif
	}
	*bp = '\0';

	return (bp - buf);
}

#endif

EXPORT int
getstr(buf, len)
	char	*buf;
	int	len;
{
	return (fgetstr(stdin, buf, len));
}
