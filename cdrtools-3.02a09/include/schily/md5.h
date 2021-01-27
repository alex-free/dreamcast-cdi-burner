/* @(#)md5.h	1.11 10/08/27 2008-2010 J. Schilling */
/*
 * MD5 hash code taken from OpenBSD
 *
 * Portions Copyright (c) 2008-2010 J. Schilling
 */

/*	$OpenBSD: md5.h,v 1.16 2004/06/22 01:57:30 jfb Exp $	*/

/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 */

#ifndef	_SCHILY_MD5_H
#define	_SCHILY_MD5_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#include <schily/utypes.h>

#define	MD5_BLOCK_LENGTH		64
#define	MD5_DIGEST_LENGTH		16
#define	MD5_DIGEST_STRING_LENGTH	(MD5_DIGEST_LENGTH * 2 + 1)

typedef struct MD5Context {
	UInt32_t state[4];			/* state */
	UInt32_t count[2];			/* number of bits, mod 2^64 */
	UInt8_t	 buffer[MD5_BLOCK_LENGTH];	/* input buffer */
} MD5_CTX;

#ifdef	__cplusplus
extern "C" {
#endif

extern void	 MD5Init	__PR((MD5_CTX *));
extern void	 MD5Update	__PR((MD5_CTX *, const void *, size_t));
extern void	 MD5Pad		__PR((MD5_CTX *));
extern void	 MD5Final	__PR((UInt8_t [MD5_DIGEST_LENGTH], MD5_CTX *));
extern void	 MD5Transform	__PR((UInt32_t [4],
					const UInt8_t [MD5_BLOCK_LENGTH]));
extern char	*MD5End		__PR((MD5_CTX *, char *));
extern char	*MD5File	__PR((const char *, char *));
extern char	*MD5FileChunk	__PR((const char *, char *, off_t, off_t));
extern char	*MD5Data	__PR((const UInt8_t *, size_t, char *));

#ifdef	__cplusplus
}
#endif

#endif /* _SCHILY_MD5_H */
