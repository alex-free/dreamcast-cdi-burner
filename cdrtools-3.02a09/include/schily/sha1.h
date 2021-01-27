/* @(#)sha1.h	1.4 10/08/27 2010 J. Schilling */
/*
 * SHA1 hash code taken from OpenBSD
 *
 * Portions Copyright (c) 2010 J. Schilling
 */

/*	$OpenBSD: sha1.h,v 1.23 2004/06/22 01:57:30 jfb Exp $	*/

/*
 * SHA-1 in C
 * By Steve Reid <steve@edmweb.com>
 * 100% Public Domain
 */

#ifndef	_SCHILY_SHA1_H
#define	_SCHILY_SHA1_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#define	SHA1_BLOCK_LENGTH		64
#define	SHA1_DIGEST_LENGTH		20
#define	SHA1_DIGEST_STRING_LENGTH	(SHA1_DIGEST_LENGTH * 2 + 1)

typedef struct {
	UInt32_t state[5];
	UInt32_t count[2];
	UInt8_t	 buffer[SHA1_BLOCK_LENGTH];
} SHA1_CTX;

#ifdef	__cplusplus
extern "C" {
#endif

extern	void	SHA1Init	__PR((SHA1_CTX *));
extern	void	SHA1Pad		__PR((SHA1_CTX *));
extern	void	SHA1Transform	__PR((UInt32_t [5],
					const UInt8_t [SHA1_BLOCK_LENGTH]));
extern	void	SHA1Update	__PR((SHA1_CTX *, const UInt8_t *, size_t));
extern	void	SHA1Final	__PR((UInt8_t [SHA1_DIGEST_LENGTH],
					SHA1_CTX *));
extern	char	*SHA1End	__PR((SHA1_CTX *, char *));
extern	char	*SHA1File	__PR((const char *, char *));
extern	char	*SHA1FileChunk	__PR((const char *, char *, off_t, off_t));
extern	char	*SHA1Data	__PR((const UInt8_t *, size_t, char *));

#ifdef	__cplusplus
}
#endif

#define	HTONDIGEST(x) do {                                              \
	x[0] = htonl(x[0]);                                             \
	x[1] = htonl(x[1]);                                             \
	x[2] = htonl(x[2]);                                             \
	x[3] = htonl(x[3]);                                             \
	x[4] = htonl(x[4]); } while (0)

#define	NTOHDIGEST(x) do {                                              \
	x[0] = ntohl(x[0]);                                             \
	x[1] = ntohl(x[1]);                                             \
	x[2] = ntohl(x[2]);                                             \
	x[3] = ntohl(x[3]);                                             \
	x[4] = ntohl(x[4]); } while (0)

#endif /* _SCHILY_SHA1_H */
