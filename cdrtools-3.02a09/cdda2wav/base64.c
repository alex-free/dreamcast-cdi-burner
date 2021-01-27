/* @(#)base64.c	1.10 09/07/10 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2009 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)base64.c	1.10 09/07/10 Copyright 1998,1999 Heiko Eissfeldt, Copyright 2006-2009 J. Schilling";

#endif
/*
 * A hacked version of rfc822_binary() for the Internet CD Index
 *
 * This is not a true RFC822 rfc822_binary() anymore. The characters
 * '/', '+', and '=' do not work well as part of an URL.
 * '_', '.', and '-' are used as an replacement.
 * A final CRLF is not added.
 */
/*
 * Program:	RFC-822 routines (originally from SMTP)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	27 July 1988
 * Last Edited:	10 September 1998
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1998 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington or The
 * Leland Stanford Junior University not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written prior
 * permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON AND THE LELAND STANFORD JUNIOR UNIVERSITY
 * DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE UNIVERSITY OF
 * WASHINGTON OR THE LELAND STANFORD JUNIOR UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "config.h"
#include <schily/stdio.h>
#include <schily/stdlib.h>

#include "base64.h"

/*
 * Convert binary contents to BASE64
 * Accepts: source
 *	    length of source
 *	    pointer to return destination length
 * Returns: destination as BASE64
 */

unsigned char *
rfc822_binary(src, srcl, len)
	void		*src;
	unsigned long	srcl;
	unsigned long	*len;
{
	unsigned char	*ret;
	unsigned char	*d;
	unsigned char	*s = (unsigned char *) src;
/*	char		*v = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";*/
	char		*v = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._";
	unsigned long	i = ((srcl + 2) / 3) * 4;

	*len = i += 2 * ((i / 60) + 1);
	d = ret = malloc((size_t) ++i);
	for (i = 0; srcl; s += 3) {	/* process tuplets */
		*d++ = v[s[0] >> 2];	/* byte 1: high 6 bits (1) */
					/* byte 2: low 2 bits (1), high 4 bits (2) */
		*d++ = v[((s[0] << 4) + (--srcl ? (s[1] >> 4) : 0)) & 0x3f];
					/* byte 3: low 4 bits (2), high 2 bits (3) */
		*d++ = srcl ? v[((s[1] << 2) + (--srcl ? (s[2] >> 6) : 0)) & 0x3f] : '-';
					/* byte 4: low 6 bits (3) */
		*d++ = srcl ? v[s[2] & 0x3f] : '-';
		if (srcl) srcl--;	/* count third character if processed */
		if ((++i) == 15) {	/* output 60 characters? */
			i = 0;		/* restart line break count, insert CRLF */
			*d++ = '\015'; *d++ = '\012';
		}
	}
	*d = '\0';			/* tie off string */

	return (ret);			/* return the resulting string */
}
