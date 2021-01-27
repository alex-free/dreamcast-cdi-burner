/* @(#)rmd160.h	1.5 10/08/27 2009-2010 J. Schilling */
/*
 * RMD160 hash code taken from OpenBSD
 *
 * Portions Copyright (c) 2009-2010 J. Schilling
 */
/*	$OpenBSD: rmd160.h,v 1.16 2004/06/22 01:57:30 jfb Exp $	*/
/*
 * Copyright (c) 2001 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_SCHILY_RMD160_H
#define	_SCHILY_RMD160_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#include <schily/utypes.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define	RMD160_BLOCK_LENGTH		64
#define	RMD160_DIGEST_LENGTH		20
#define	RMD160_DIGEST_STRING_LENGTH	(RMD160_DIGEST_LENGTH * 2 + 1)

/* RMD160 context. */
typedef struct RMD160Context {
	UInt32_t state[5];			/* state */
	UInt32_t count[2];			/* number of bits, mod 2^64 */
	UInt8_t  buffer[RMD160_BLOCK_LENGTH];	/* input buffer */
} RMD160_CTX;

extern void	 RMD160Init	__PR((RMD160_CTX *));
extern void	 RMD160Transform __PR((UInt32_t [5],
					const UInt8_t [RMD160_BLOCK_LENGTH]));
extern void	 RMD160Update	__PR((RMD160_CTX *, const UInt8_t *, size_t));
extern void	 RMD160Pad	__PR((RMD160_CTX *));
extern void	 RMD160Final	__PR((UInt8_t [RMD160_DIGEST_LENGTH],
					RMD160_CTX *));
extern char	*RMD160End	__PR((RMD160_CTX *, char *));
extern char	*RMD160File	__PR((const char *, char *));
extern char	*RMD160FileChunk __PR((const char *, char *, off_t, off_t));
extern char	*RMD160Data	__PR((const UInt8_t *, size_t, char *));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_RMD160_H */
