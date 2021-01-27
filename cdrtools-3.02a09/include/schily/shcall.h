/* @(#)shcall.h	1.3 10/08/27 Copyright 2009-2010 J. Schilling */
/*
 *	Abstraction from shcall.h
 *
 *	Copyright (c) 2009-2010 J. Schilling
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

#ifndef _SCHILY_SHCALL_H
#define	_SCHILY_SHCALL_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	__sqfun_t_defined
typedef	int	(*sqfun_t)	__PR((void *arg));
#define	__sqfun_t_defined
#endif

#ifndef	__squit_t_defined

typedef struct {
	sqfun_t	quitfun;	/* Function to query for shell signal quit   */
	void	*qfarg;		/* Generic arg for shell builtin quit fun    */
} squit_t;

#define	__squit_t_defined
#endif

typedef	int	(*shcall)	__PR((int ac, char **av, char **ev,
					FILE *std[3], squit_t *__quit));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_SHCALL_H */
