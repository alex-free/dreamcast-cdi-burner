/* @(#)md4.h	1.4 10/08/27 2009-2010 J. Schilling */
/*
 * MD4 hash code taken from OpenBSD
 *
 * Portions Copyright (c) 2009-2010 J. Schilling
 */
/*	$OpenBSD: md4.h,v 1.15 2004/06/22 01:57:30 jfb Exp $	*/

/*
 * This code implements the MD4 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 * Todd C. Miller modified the MD5 code to do MD4 based on RFC 1186.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 */

#ifndef	_SCHILY_MD4_H
#define	_SCHILY_MD4_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#include <schily/utypes.h>

#define	MD4_BLOCK_LENGTH		64
#define	MD4_DIGEST_LENGTH		16
#define	MD4_DIGEST_STRING_LENGTH	(MD4_DIGEST_LENGTH * 2 + 1)

typedef struct MD4Context {
	UInt32_t state[4];			/* state */
	UInt32_t count[2];			/* number of bits, mod 2^64 */
	UInt8_t	 buffer[MD4_BLOCK_LENGTH];	/* input buffer */
} MD4_CTX;

#ifdef	__cplusplus
extern "C" {
#endif

extern void	 MD4Init	__PR((MD4_CTX *));
extern void	 MD4Update	__PR((MD4_CTX *, const void *, size_t));
extern void	 MD4Pad		__PR((MD4_CTX *));
extern void	 MD4Final	__PR((UInt8_t [MD4_DIGEST_LENGTH], MD4_CTX *));
extern void	 MD4Transform	__PR((UInt32_t [4],
					const UInt8_t [MD4_BLOCK_LENGTH]));
extern char	*MD4End		__PR((MD4_CTX *, char *));
extern char	*MD4File	__PR((const char *, char *));
extern char	*MD4FileChunk	__PR((const char *, char *, off_t, off_t));
extern char	*MD4Data	__PR((const UInt8_t *, size_t, char *));

#ifdef	__cplusplus
}
#endif

#endif /* _SCHILY_MD4_H */
