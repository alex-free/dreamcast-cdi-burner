/* @(#)sha2.h	1.5 10/08/27 2009-2010 J. Schilling */
/*
 * SHA2 hash code taken from OpenBSD
 *
 * Portions Copyright (c) 2009-2010 J. Schilling
 */

/*	$OpenBSD: sha2.h,v 1.7 2008/09/06 12:00:19 djm Exp $	*/

/*
 * FILE:	sha2.h
 * AUTHOR:	Aaron D. Gifford <me@aarongifford.com>
 *
 * Copyright (c) 2000-2001, Aaron D. Gifford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $From: sha2.h,v 1.1 2001/11/08 00:02:01 adg Exp adg $
 */

#ifndef	_SCHILY_SHA2_H
#define	_SCHILY_SHA2_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#include <schily/utypes.h>


/* ** SHA-256/384/512 Various Length Definitions ********************** */
#define	SHA256_BLOCK_LENGTH		64
#define	SHA256_DIGEST_LENGTH		32
#define	SHA256_DIGEST_STRING_LENGTH	(SHA256_DIGEST_LENGTH * 2 + 1)
#ifdef	HAVE_LONGLONG
#define	SHA384_BLOCK_LENGTH		128
#define	SHA384_DIGEST_LENGTH		48
#define	SHA384_DIGEST_STRING_LENGTH	(SHA384_DIGEST_LENGTH * 2 + 1)
#define	SHA512_BLOCK_LENGTH		128
#define	_SHA512_BLOCK_LENGTH		128
#define	SHA512_DIGEST_LENGTH		64
#define	SHA512_DIGEST_STRING_LENGTH	(SHA512_DIGEST_LENGTH * 2 + 1)
#else
#define	_SHA512_BLOCK_LENGTH		128
#endif


/* ** SHA-256/384/512 Context Structure ****************************** */
typedef struct _SHA2_CTX {
	union {
		UInt32_t	st32[8];
#ifdef	HAVE_LONGLONG
		UInt64_t	st64[8];
#else
		UInt32_t	st64[16];	/* Keep original size */
#endif
	} state;
#ifdef	HAVE_LONGLONG
	UInt64_t	bitcount[2];
#else
	UInt32_t	bitcount[4];
#endif
	UInt8_t	buffer[_SHA512_BLOCK_LENGTH];
} SHA2_CTX;


#ifdef	__cplusplus
extern "C" {
#endif

extern void SHA256Init		__PR((SHA2_CTX *));
extern void SHA256Transform	__PR((UInt32_t state[8],
					const UInt8_t [SHA256_BLOCK_LENGTH]));
extern void SHA256Update	__PR((SHA2_CTX *, const UInt8_t *, size_t));
extern void SHA256Pad		__PR((SHA2_CTX *));
extern void SHA256Final		__PR((UInt8_t [SHA256_DIGEST_LENGTH],
					SHA2_CTX *));
extern char *SHA256End		__PR((SHA2_CTX *, char *));
extern char *SHA256File		__PR((const char *, char *));
extern char *SHA256FileChunk	__PR((const char *, char *, off_t, off_t));
extern char *SHA256Data		__PR((const UInt8_t *, size_t, char *));

#ifdef	HAVE_LONGLONG
extern void SHA384Init		__PR((SHA2_CTX *));
extern void SHA384Transform	__PR((UInt64_t state[8],
					const UInt8_t [SHA384_BLOCK_LENGTH]));
extern void SHA384Update	__PR((SHA2_CTX *, const UInt8_t *, size_t));
extern void SHA384Pad		__PR((SHA2_CTX *));
extern void SHA384Final		__PR((UInt8_t [SHA384_DIGEST_LENGTH],
					SHA2_CTX *));
extern char *SHA384End		__PR((SHA2_CTX *, char *));
extern char *SHA384File		__PR((const char *, char *));
extern char *SHA384FileChunk	__PR((const char *, char *, off_t, off_t));
extern char *SHA384Data		__PR((const UInt8_t *, size_t, char *));

extern void SHA512Init		__PR((SHA2_CTX *));
extern void SHA512Transform	__PR((UInt64_t state[8],
					const UInt8_t [SHA512_BLOCK_LENGTH]));
extern void SHA512Update	__PR((SHA2_CTX *, const UInt8_t *, size_t));
extern void SHA512Pad		__PR((SHA2_CTX *));
extern void SHA512Final		__PR((UInt8_t [SHA512_DIGEST_LENGTH],
					SHA2_CTX *));
extern char *SHA512End		__PR((SHA2_CTX *, char *));
extern char *SHA512File		__PR((const char *, char *));
extern char *SHA512FileChunk	__PR((const char *, char *, off_t, off_t));
extern char *SHA512Data		__PR((const UInt8_t *, size_t, char *));
#endif

#ifdef	__cplusplus
}
#endif

#endif /* _SCHILY_SHA2_H */
