/* @(#)find.h	1.24 15/07/10 Copyright 2005-2015 J. Schilling */
/*
 *	Definitions for libfind users.
 *
 *	Copyright (c) 2004-2015 J. Schilling
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

#ifndef	_SCHILY_FIND_H
#define	_SCHILY_FIND_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef _SCHILY_STDIO_H
#include <schily/stdio.h>
#endif

#ifndef _SCHILY_STANDARD_H
#include <schily/standard.h>
#endif
#ifndef _SCHILY_STAT_H
#include <schily/stat.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct find_args {
	int	Argc;		/* A copy of argc		*/
	char	**Argv;		/* A copy of argv		*/
	FILE	*std[3];	/* To redirect stdin/stdout/err	*/
	int	primtype;	/* The type of the last primary	*/
	BOOL	found_action;	/* -print/-ls/-exec found	*/
	int	patlen;		/* strlen() for longest pattern	*/
	int	walkflags;	/* Walkflags modifed by parser	*/
	int	maxdepth;	/* -mindepth arg		*/
	int	mindepth;	/* -maxdepth arg		*/
	struct plusargs *plusp;	/* List of -exec {} + commands	*/
	void	*jmp;		/* Used internally by parser	*/
	int	error;		/* Error code from find_parse()	*/
} finda_t;

/*
 * finda_t->primtype is set to ENDARGS by find_parse() if a complete expression
 * could be parsed.
 */
#define	FIND_ENDARGS	1000	/* Found End of Arg Vector		*/
#define	FIND_ERRARG	1001	/* Parser abort by -help or error	*/

/*
 * Flags used for struct WALK->pflags:
 */
#define	PF_ACL		0x00001	/* Check ACL from struct WALK->pflags	*/
#define	PF_HAS_ACL	0x10000	/* This file has ACL			*/
#define	PF_XATTR	0x00002	/* Check XATTR from struct WALK->pflags	*/
#define	PF_HAS_XATTR	0x20000	/* This file has XATTR			*/

#ifndef	FIND_NODE
#define	findn_t	void
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


extern	void	find_argsinit	__PR((finda_t *fap));
extern	void	find_timeinit	__PR((time_t __now));
extern	findn_t	*find_printnode	__PR((void));
extern	findn_t	*find_addprint	__PR((findn_t *np, finda_t *fap));
extern	void	find_free	__PR((findn_t *t, finda_t *fap));
extern	int	find_token	__PR((char *__word));
extern	char	*find_tname	__PR((int op));
extern	findn_t	*find_parse	__PR((finda_t *fap));
extern	void	find_firstprim	__PR((int *pac, char *const **pav));

extern	BOOL	find_primary	__PR((findn_t *t, int op));
extern	BOOL	find_pname	__PR((findn_t *t, char *__word));
extern	BOOL	find_hasprint	__PR((findn_t *t));
extern	BOOL	find_hasexec	__PR((findn_t *t));
extern	BOOL	find_expr	__PR((char *f, char *ff, struct stat *fs,
					struct WALK *state, findn_t *t));

extern	int	find_plusflush	__PR((void *p, struct WALK *state));
extern	void	find_usage	__PR((FILE *f));
extern	int	find_main	__PR((int ac, char **av, char **ev,
					FILE *std[3], squit_t *__quit));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_FIND_H */
