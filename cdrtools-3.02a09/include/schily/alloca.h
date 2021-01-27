/* @(#)alloca.h	1.7 10/08/24 Copyright 2002-2010 J. Schilling */
/*
 *	Definitions for users of alloca()
 *
 *	Important: #include this directly after <schily/mconfig.h>
 *	and before any other include file.
 *	See comment in _AIX part below.
 *
 *	Copyright (c) 2002-2010 J. Schilling
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


#ifndef	_SCHILY_ALLOCA_H
#define	_SCHILY_ALLOCA_H

#ifdef __GNUC__
#	ifndef	alloca
#		define	alloca(s)	__builtin_alloca(s)
#	endif
#else
#	ifdef _MSC_VER
#		include <malloc.h>
#		define alloca _alloca
#	else
#		if HAVE_ALLOCA_H
#			include <alloca.h>
#		else
#			ifdef _AIX
				/*
				 * Indent so pre-ANSI compilers will ignore it
				 *
				 * Some versions of AIX may require this to be
				 * first in the file and only preceded by
				 * comments and preprocessor directives/
				 */
				/* CSTYLED */
				#pragma alloca
#			else
#				ifndef alloca
#ifdef	__cplusplus
extern "C" {
#endif
					/*
					 * predefined by HP cc +Olibcalls
					 */
#					ifdef	PROTOTYPES
						extern void *alloca();
#					else
						extern char *alloca();
#					endif
#ifdef	__cplusplus
}
#endif
#				endif
#			endif
#		endif
#	endif
#endif

#endif	/* _SCHILY_ALLOCA_H */
