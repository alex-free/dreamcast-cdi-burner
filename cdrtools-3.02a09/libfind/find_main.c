/*#define	PLUS_DEBUG*/
/* @(#)find_main.c	1.69 15/09/12 Copyright 2004-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)find_main.c	1.69 15/09/12 Copyright 2004-2010 J. Schilling";
#endif
/*
 *	Another find implementation...
 *
 *	Copyright (c) 2004-2010 J. Schilling
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

#include <schily/stdio.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/stat.h>
#include <schily/time.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/jmpdefs.h>	/* To include __jmalloc() */
#include <schily/schily.h>

#include <schily/nlsdefs.h>

char	strvers[] = "1.5";	/* The pure version string	*/

#include <schily/walk.h>
#include <schily/find.h>

LOCAL	int	walkfunc	__PR((char *nm, struct stat *fs, int type, struct WALK *state));
LOCAL	int	getflg		__PR((char *optstr, long *argp));
EXPORT	int	find_main	__PR((int ac, char **av, char **ev, FILE *std[3], squit_t *quit));

LOCAL int
walkfunc(nm, fs, type, state)
	char		*nm;
	struct stat	*fs;
	int		type;
	struct WALK	*state;
{
	if (state->quitfun) {
		/*
		 * Check for shell builtin signal abort condition.
		 */
		if ((*state->quitfun)(state->qfarg)) {
			state->flags |= WALK_WF_QUIT;
			return (0);
		}
	}
	if (type == WALK_NS) {
		ferrmsg(state->std[2],
				_("Cannot stat '%s'.\n"), nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_SLN && (state->walkflags & WALK_PHYS) == 0) {
		ferrmsg(state->std[2],
				_("Cannot follow symlink '%s'.\n"), nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_DNR) {
		if (state->flags & WALK_WF_NOCHDIR) {
			ferrmsg(state->std[2],
				_("Cannot chdir to '%s'.\n"), nm);
		} else {
			ferrmsg(state->std[2],
				_("Cannot read '%s'.\n"), nm);
		}
		state->err = 1;
		return (0);
	}

	if (state->maxdepth >= 0 && state->level >= state->maxdepth)
		state->flags |= WALK_WF_PRUNE;
	if (state->mindepth >= 0 && state->level < state->mindepth)
		return (0);

	find_expr(nm, nm + state->base, fs, state, state->tree);
	return (0);
}



/* ARGSUSED */
LOCAL int
getflg(optstr, argp)
	char	*optstr;
	long	*argp;
{
#ifdef	GETFLG_DEBUG
	error("optstr: '%s'\n", optstr);
#endif

	if (optstr[1] != '\0')
		return (-1);

	switch (*optstr) {

	case 'H':
		*(int *)argp |= WALK_ARGFOLLOW;
		return (TRUE);
	case 'L':
		*(int *)argp |= WALK_ALLFOLLOW;
		return (TRUE);
	case 'P':
		*(int *)argp &= ~(WALK_ARGFOLLOW | WALK_ALLFOLLOW);
		return (TRUE);

	default:
		return (-1);
	}
}

EXPORT int
find_main(ac, av, ev, std, quit)
	int	ac;
	char	**av;
	char	**ev;
	FILE	*std[3];
	squit_t	*quit;
{
	int	cac  = ac;
	char	**cav = av;
	char	**firstpath;
	char	**firstprim;
	BOOL	help = FALSE;
	BOOL	prversion = FALSE;
	finda_t	fa;
	findn_t	*Tree;
	struct WALK	walkstate;
	int	oraise[3];
	int	ret = 0;
	int	i;

	save_args(ac, av);

#ifdef	USE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(TEXT_DOMAIN, "/opt/schily/lib/locale");
#endif
	find_argsinit(&fa);
	for (i = 0; i < 3; i++) {
		oraise[i] = file_getraise(std[i]);
		file_raise(std[i], FALSE);
		fa.std[i] = std[i];
	}
	fa.walkflags = WALK_CHDIR | WALK_PHYS;
	fa.walkflags |= WALK_NOSTAT;
	fa.walkflags |= WALK_NOEXIT;

	/*
	 * Do not check the return code for getargs() as we may get an error
	 * code from e.g. "find -print" and we do not like to handle this here.
	 */
	cac--, cav++;
	getargs(&cac, (char * const **)&cav, "help,version,&",
			&help, &prversion,
			getflg, (long *)&fa.walkflags);
	if (help) {
		find_usage(std[2]);
		goto out;
	}
	if (prversion) {
		fprintf(std[1],
		"sfind release %s (%s-%s-%s) Copyright (C) 2004-2010 Jörg Schilling\n",
				strvers,
				HOST_CPU, HOST_VENDOR, HOST_OS);
		goto out;
	}

	firstpath = cav;	/* Remember first file type arg */
	find_firstprim(&cac, (char *const **)&cav);
	firstprim = cav;	/* Remember first Primary type arg */
	fa.Argv = cav;
	fa.Argc = cac;

	if (cac) {
		Tree = find_parse(&fa);
		if (fa.primtype == FIND_ERRARG) {
			find_free(Tree, &fa);
			ret = fa.error;
			goto out;
		}
		if (fa.primtype != FIND_ENDARGS) {
			ferrmsgno(std[2], EX_BAD,
					_("Incomplete expression.\n"));
			find_free(Tree, &fa);
			ret = EX_BAD;
			goto out;
		}
		if (find_pname(Tree, "-chown") || find_pname(Tree, "-chgrp") ||
		    find_pname(Tree, "-chmod")) {
			ferrmsgno(std[2], EX_BAD,
				_("Unsupported primary -chown/-chgrp/-chmod.\n"));
			find_free(Tree, &fa);
			ret = EX_BAD;
			goto out;
		}
	} else {
		Tree = 0;
	}
	if (Tree == 0) {
		Tree = find_printnode();
	} else if (!fa.found_action) {
		Tree = find_addprint(Tree, &fa);
		if (Tree == (findn_t *)NULL) {
			ret = geterrno();
			goto out;
		}
	}
	walkinitstate(&walkstate);
	for (i = 0; i < 3; i++)
		walkstate.std[i] = std[i];
	if (ev)
		walkstate.env = ev;
	if (quit) {
		walkstate.quitfun = quit->quitfun;
		walkstate.qfarg   = quit->qfarg;
	}
	if (fa.patlen > 0) {
		walkstate.patstate = __fjmalloc(std[2],
					sizeof (int) * fa.patlen,
					"space for pattern state", JM_RETURN);
		if (walkstate.patstate == NULL) {
			ret = geterrno();
			goto out;
		}
	}

	find_timeinit(time(0));

	walkstate.walkflags	= fa.walkflags;
	walkstate.maxdepth	= fa.maxdepth;
	walkstate.mindepth	= fa.mindepth;
	walkstate.lname		= NULL;
	walkstate.tree		= Tree;
	walkstate.err		= 0;
	walkstate.pflags	= 0;

	if (firstpath == firstprim) {
		treewalk(".", walkfunc, &walkstate);
	} else {
		for (cav = firstpath; cav != firstprim; cav++) {
			treewalk(*cav, walkfunc, &walkstate);
			/*
			 * XXX hier break wenn treewalk() Fehler gemeldet
			 */
		}
	}
	/*
	 * Execute all unflushed '-exec .... {} +' expressions.
	 */
	find_plusflush(fa.plusp, &walkstate);
	find_free(Tree, &fa);
	if (walkstate.patstate != NULL)
		free(walkstate.patstate);
	ret = walkstate.err;

out:
	for (i = 0; i < 3; i++) {
		fflush(std[i]);
		if (ferror(std[i]))
			clearerr(std[i]);
		file_raise(std[i], oraise[i]);
	}
	return (ret);
}
