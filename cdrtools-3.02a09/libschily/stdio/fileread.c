/* @(#)fileread.c	1.17 12/02/26 Copyright 1986, 1995-2012 J. Schilling */
/*
 *	Copyright (c) 1986, 1995-2012 J. Schilling
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

#include "schilyio.h"

static	char	_readerr[]	= "file_read_err";

#ifdef	HAVE_USG_STDIO

EXPORT ssize_t
fileread(f, buf, len)
	register FILE	*f;
	void	*buf;
	size_t	len;
{
	ssize_t	cnt;
	register int	n;

	down2(f, _IOREAD, _IORW);

	if (f->_flag & _IONBF) {
		cnt = _niread(fileno(f), buf, len);
		if (cnt < 0) {
			f->_flag |= _IOERR;
			if (!(my_flag(f) & _JS_IONORAISE))
				raisecond(_readerr, 0L);
		}
		if (cnt == 0 && len)
			f->_flag |= _IOEOF;
		return (cnt);
	}
	cnt = 0;
	while (len > 0) {
		if (f->_cnt <= 0) {
			if (usg_filbuf(f) == EOF)
				break;
			f->_cnt++;
			f->_ptr--;
		}
		n = f->_cnt >= len ? len : f->_cnt;
		buf = (void *)movebytes(f->_ptr, buf, n);
		f->_ptr += n;
		f->_cnt -= n;
		cnt += n;
		len -= n;
	}
	if (!ferror(f))
		return (cnt);
	if (!(my_flag(f) & _JS_IONORAISE) && !(_io_glflag & _JS_IONORAISE))
		raisecond(_readerr, 0L);
	return (-1);
}

#else

EXPORT ssize_t
fileread(f, buf, len)
	register FILE	*f;
	void	*buf;
	size_t	len;
{
	ssize_t	cnt;

	down2(f, _IOREAD, _IORW);

	if (my_flag(f) & _JS_IOUNBUF)
		return (_niread(fileno(f), buf, len));
	cnt = fread(buf, 1, len, f);

	if (!ferror(f))
		return (cnt);
	if (!(my_flag(f) & _JS_IONORAISE) && !(_io_glflag & _JS_IONORAISE))
		raisecond(_readerr, 0L);
	return (-1);
}

#endif	/* HAVE_USG_STDIO */
