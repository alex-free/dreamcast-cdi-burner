/*#define	PLUS_DEBUG*/
/* @(#)find.c	1.101 15/09/12 Copyright 2004-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)find.c	1.101 15/09/12 Copyright 2004-2015 J. Schilling";
#endif
/*
 *	Another find implementation...
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

#ifdef	__FIND__
#define	FIND_MAIN
#endif

#include <schily/stdio.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/fcntl.h>
#include <schily/stat.h>
#include <schily/dirent.h>
#include <schily/time.h>
#include <schily/wait.h>
#include <schily/string.h>
#include <schily/utypes.h>	/* incl. limits.h (_POSIX_ARG_MAX/ARG_MAX) */
#include <schily/param.h>	/* #defines NCARGS on old systems */
#include <schily/btorder.h>
#include <schily/patmatch.h>
#include <schily/fnmatch.h>
#include <schily/standard.h>
#include <schily/jmpdefs.h>
#include <schily/schily.h>
#include <schily/pwd.h>
#include <schily/grp.h>
#define	VMS_VFORK_OK
#include <schily/vfork.h>

#include <schily/nlsdefs.h>

#if	defined(_ARG_MAX32) && defined(_ARG_MAX64)
#define	MULTI_ARG_MAX		/* SunOS only ?			*/
#endif

#ifdef	__FIND__
char	strvers[] = "1.4";	/* The pure version string	*/
#endif

typedef struct {
	char	*left;
	char	*right;
	char	*this;
	int	op;
	union {
		int	i;
		long	l;
		dev_t	dev;
		ino_t	ino;
		mode_t	mode;
		nlink_t	nlink;
		uid_t	uid;
		gid_t	gid;
		size_t	size;
		time_t	time;
		FILE	*fp;
	} val, val2;
} findn_t;

#include <schily/walk.h>
#define	FIND_NODE
#include <schily/find.h>
#include "find_list.h"
#include "find_misc.h"
#define	TOKEN_NAMES
#include "find_tok.h"

/*
 *	The struct plusargs and the adjacent space that holds the
 *	arg vector and the string table. The struct plusargs member "av"
 *	is also part of the ARG_MAX sized space that follows.
 *
 *	---------------------------------
 *	| Other struct plusargs fields	|	Don't count against ARG_MAX
 *	---------------------------------
 *	---------------------------------
 *	| 	New Arg vector[0]	|	Space for ARG_MAX starts here
 *	---------------------------------
 *	|		.		|
 *	|		.		|	Arg space grows upwards
 *	|		V		|
 *	---------------------------------
 *	|	 Arg vector end		|	"nextargp" points here
 *	---------------------------------
 *	---------------------------------
 *	| Space for first arg string	|
 *	---------------------------------	"laststr" points here
 *	|		^		|
 *	|		.		|	String space "grows" downwards
 *	|		.		|
 *	---------------------------------
 *	| Space for first arg string	|	Space for ARG_MAX ends here
 *	---------------------------------	"endp" points here
 *
 *	The Arg vector in struct plusargs uses the native pointer size
 *	for libfind. ARG_MAX however is based on the pointer size in the
 *	called program.
 *
 *	If a 32 bit libfind calls a 64 bit program, the arg vector and the
 *	environment array in the called program needs more space than in the
 *	calling libfind code.
 *
 *	If a 64 bit libfind calls a 32 bit program, the arg vector and the
 *	environment array in the called program needs less space than in the
 *	calling libfind code.
 */
struct plusargs {
	struct plusargs	*next;		/* Next in list for flushing	*/
	char		*endp;		/* Points to end of data block	*/
	char		**nextargp;	/* Points to next av[] entry	*/
	char		*laststr;	/* points to last used string	*/
	int		nenv;		/* Number of entries in env	*/
	int		ac;		/* The argc for our command	*/
	char		*av[1];		/* The argv for our command	*/
};

#ifdef	PLUS_DEBUG			/* We are no longer reentrant	*/
LOCAL struct plusargs *plusp;		/* Avoid PLUS_DEBUG if possible	*/
#endif

#define	MINSECS		(60)
#define	HOURSECS	(60 * MINSECS)
#define	DAYSECS		(24 * HOURSECS)
#define	YEARSECS	(365 * DAYSECS)

extern	time_t	find_sixmonth;		/* 6 months before limit (ls)	*/
extern	time_t	find_now;		/* now limit (ls)		*/

LOCAL	findn_t	Printnode = { 0, 0, 0, PRINT };

EXPORT	void	find_argsinit	__PR((finda_t *fap));
EXPORT	void	find_timeinit	__PR((time_t now));
EXPORT	findn_t	*find_printnode	__PR((void));
EXPORT	findn_t	*find_addprint	__PR((findn_t *np, finda_t *fap));
LOCAL	findn_t	*allocnode	__PR((finda_t *fap));
EXPORT	void	find_free	__PR((findn_t *t, finda_t *fap));
LOCAL	void	find_freenode	__PR((findn_t *t));
LOCAL	void	nexttoken	__PR((finda_t *fap));
LOCAL	BOOL	_nexttoken	__PR((finda_t *fap));
LOCAL	void	errjmp		__PR((finda_t *fap, int err));
EXPORT	int	find_token	__PR((char *word));
EXPORT	char	*find_tname	__PR((int op));
LOCAL	char	*nextarg	__PR((finda_t *fap, findn_t *t));
EXPORT	findn_t	*find_parse	__PR((finda_t *fap));
LOCAL	findn_t	*parse		__PR((finda_t *fap));
LOCAL	findn_t	*parseand	__PR((finda_t *fap));
LOCAL	findn_t	*parseprim	__PR((finda_t *fap));
EXPORT	void	find_firstprim	__PR((int *pac, char *const **pav));
EXPORT	BOOL	find_primary	__PR((findn_t *t, int op));
EXPORT	BOOL	find_pname	__PR((findn_t *t, char *word));
EXPORT	BOOL	find_hasprint	__PR((findn_t *t));
EXPORT	BOOL	find_hasexec	__PR((findn_t *t));
#ifdef	FIND_MAIN
LOCAL	int	walkfunc	__PR((char *nm, struct stat *fs, int type, struct WALK *state));
#endif
#ifdef	__FIND__
LOCAL	inline BOOL find_expr	__PR((char *f, char *ff, struct stat *fs, struct WALK *state, findn_t *t));
#else
EXPORT	BOOL	find_expr	__PR((char *f, char *ff, struct stat *fs, struct WALK *state, findn_t *t));
#endif
#ifdef	HAVE_FORK
LOCAL	BOOL	doexec		__PR((char *f, findn_t *t, int ac, char **av, struct WALK *state));
#endif
LOCAL	int	countenv	__PR((void));
LOCAL	int	argsize		__PR((int xtype));
LOCAL	int	extype		__PR((char *name));
#ifdef	MULTI_ARG_MAX
LOCAL	int	xargsize	__PR((int xtype, int maxarg));
#endif
LOCAL	BOOL	pluscreate	__PR((FILE *f, int ac, char **av, finda_t *fap));
#ifdef	HAVE_FORK
LOCAL	BOOL	plusexec	__PR((char *f, findn_t *t, struct WALK *state));
#endif
EXPORT	int	find_plusflush	__PR((void *p, struct WALK *state));
EXPORT	void	find_usage	__PR((FILE *f));
#ifdef	FIND_MAIN
LOCAL	int	getflg		__PR((char *optstr, long *argp));
EXPORT	int	main		__PR((int ac, char **av));
#endif


EXPORT void
find_argsinit(fap)
	finda_t	*fap;
{
	fap->Argc = 0;
	fap->Argv = (char **)NULL;
	fap->std[0] = stdin;
	fap->std[1] = stdout;
	fap->std[2] = stderr;
	fap->primtype = 0;
	fap->found_action = FALSE;
	fap->patlen = 0;
	fap->walkflags = 0;
	fap->maxdepth = -1;
	fap->mindepth = -1;
	fap->plusp = (struct plusargs *)NULL;
	fap->jmp = NULL;
	fap->error = 0;
}

EXPORT void
find_timeinit(now)
	time_t	now;
{
	find_now	= now + 60;
	find_sixmonth	= now - 6L*30L*24L*60L*60L;
}

EXPORT findn_t *
find_printnode()
{
	return (&Printnode);
}

/*
 * Add a -print node to the parsed tree if there is no action already.
 */
EXPORT findn_t *
find_addprint(np, fap)
	findn_t	*np;
	finda_t	*fap;
{
	findn_t	*n;

	n = allocnode(fap);
	if (n == NULL) {
		find_freenode(np);
		return ((void *)NULL);
	}
	n->op = AND;
	n->left = (char *)np;
	n->right = (char *)&Printnode;
	return (n);
}

/*
 * allocnode is currently called by:
 *	find_addprint(), parse(), parseand(), parseprim()
 */
LOCAL findn_t *
allocnode(fap)
	finda_t	*fap;
{
	findn_t *n;

	n = __fjmalloc(fap->std[2], sizeof (findn_t), "allocnode", JM_RETURN);
	if (n == NULL)
		return (n);
	n->left = 0;
	n->right = 0;
	n->this = 0;
	n->op = 0;
	n->val.l = 0;
	n->val2.l = 0;
	return (n);
}

EXPORT void
find_free(t, fap)
	findn_t	*t;
	finda_t	*fap;
{
	if (fap != NULL) {
		struct plusargs *p;
		struct plusargs *np = NULL;

		for (p = fap->plusp; p != NULL; p = np) {
			np = p->next;
			free(p);
		}
	}

	find_freenode(t);
}

LOCAL void
find_freenode(t)
	findn_t	*t;
{
	if (t == (findn_t *)NULL || t == &Printnode)
		return;

	switch (t->op) {

	case OPEN:
	case LNOT:
		find_freenode((findn_t *)t->this);
		break;
	case AND:
	case LOR:
		find_freenode((findn_t *)t->left);
		find_freenode((findn_t *)t->right);
		break;
	case PAT:
	case PPAT:
	case LPAT:
		if (t->right != NULL)
			free(t->right);	/* aux array for patcompile() */
		break;
	case FLS:
	case FPRINT:
	case FPRINT0:
	case FPRINTNNL:
		fclose(t->val.fp);
	default:
		;
	}
	free(t);
}

LOCAL void
nexttoken(fap)
	register finda_t	*fap;
{
	if (!_nexttoken(fap)) {
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
	}
}

/*
 * No errjmp() variant of nexttoken(), returns FALSE on error.
 */
LOCAL BOOL
_nexttoken(fap)
	register finda_t	*fap;
{
	register char	*word;
	register char	*tail;

	if (fap->Argc <= 0) {
		fap->primtype = FIND_ENDARGS;
		return (TRUE);
	}
	word = *fap->Argv;
	if ((tail = strchr(word, '=')) != NULL) {
#ifdef	XXX
		if (*tail == '\0') {
			fap->Argv++; fap->Argc--;
		} else
#endif
			*fap->Argv = ++tail;
	} else {
		fap->Argv++; fap->Argc--;
	}
	if ((fap->primtype = find_token(word)) >= 0)
		return (TRUE);

	ferrmsgno(fap->std[2], EX_BAD, _("Bad Option: '%s'.\n"), word);
	find_usage(fap->std[2]);
	fap->primtype = FIND_ERRARG;	/* Mark as "parse aborted"	*/
	return (FALSE);
}

LOCAL void
errjmp(fap, err)
	register finda_t	*fap;
		int		err;
{
	fap->primtype	= FIND_ERRARG;	/* Mark as "parse aborted"	*/
	fap->error	= err;		/* Set error return		*/

	siglongjmp(((sigjmps_t *)fap->jmp)->jb, 1);
	/* NOTREACHED */
}

EXPORT int
find_token(word)
	register char	*word;
{
	char	**tp;
	char	*tn;
	char	*equalp;
	int	type;

	if ((equalp = strchr(word, '=')) != NULL)
		*equalp = '\0';

	if (*word == '-') {
		/*
		 * Do not allow -(, -), -!
		 */
		if (word[1] == '\0' || !strchr("()!", word[1]))
			word++;
	} else if (!strchr("()!", word[0]) && (!equalp || equalp[1] == '\0')) {
		goto bad;
	}
	for (type = 0, tp = tokennames; *tp; tp++, type++) {
		tn = *tp;
		if (*tn != *word)
			continue;
		if (streql(tn, word)) {
			if (equalp)
				*equalp = '=';
			return (type);
		}
	}
bad:
	if (equalp)
		*equalp = '=';

	return (-1);
}

EXPORT char *
find_tname(op)
	int	op;
{
	if (op >= 0 && op < NTOK)
		return (tokennames[op]);
	return ("unknown");
}

LOCAL char *
nextarg(fap, t)
	finda_t	*fap;
	findn_t	*t;
{
	if (fap->Argc-- <= 0) {
		char	*prim	= NULL;
		int	pt	= t->op;

		if (pt >= 0 && pt < NTOK)
			prim = tokennames[pt];
		if (prim) {
			ferrmsgno(fap->std[2], EX_BAD,
				_("Missing arg for '%s%s'.\n"),
				pt > LNOT ? "-":"", prim);
		} else {
			ferrmsgno(fap->std[2], EX_BAD,
				_("Missing arg.\n"));
		}
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
		return ((char *)0);
	} else {
		return (*fap->Argv++);
	}
}

EXPORT findn_t *
find_parse(fap)
	finda_t	*fap;
{
	findn_t		*ret;

	if (!_nexttoken(fap))
		return ((findn_t *)NULL);	/* Immediate parse error */
	if (fap->primtype == FIND_ENDARGS)
		return ((findn_t *)NULL);	/* Empty command	 */

	ret = parse(fap);
	if (ret)
		return (ret);

	if (fap->primtype == HELP) {
		fap->primtype = FIND_ERRARG;
	} else if (fap->error == 0) {
		fap->primtype = FIND_ERRARG;
		fap->error = geterrno();
		if (fap->error == 0)
			fap->error = EX_BAD;
	}
	return (ret);
}

LOCAL findn_t *
parse(fap)
	finda_t	*fap;
{
	findn_t	*n;

	n = parseand(fap);
	if (n == NULL)
		return (n);
	if (fap->primtype == LOR) {
		findn_t	*l = allocnode(fap);

		if (l == NULL)
			goto err;
		l->left = (char *)n;
		l->op = fap->primtype;
		if (_nexttoken(fap))
			l->right = (char *)parse(fap);
		if (l->right == NULL) {
			find_freenode(l);
			n = NULL;		/* Do not free twice		*/
			goto err;
		}
		return (l);
	}
	return (n);
err:
	find_freenode(n);
	fap->primtype = FIND_ERRARG;		/* Mark as "parse aborted"	*/
	return ((findn_t *)NULL);
}

LOCAL findn_t *
parseand(fap)
	finda_t	*fap;
{
	findn_t	*n;

	n = parseprim(fap);
	if (n == NULL)
		return (n);

	if ((fap->primtype == AND) ||
	    (fap->primtype != LOR && fap->primtype != CLOSE &&
	    fap->primtype != FIND_ENDARGS)) {
		findn_t	*l = allocnode(fap);
		BOOL	ok = TRUE;

		if (l == NULL)
			goto err;
		l->left = (char *)n;
		l->op = AND;		/* If no Operator, default to AND -a */
		if (fap->primtype == AND) /* Fetch Operator for next node */
			ok = _nexttoken(fap);
		if (ok)
			l->right = (char *)parseand(fap);
		if (l->right == NULL) {
			find_freenode(l);
			n = NULL;		/* Do not free twice		*/
			goto err;
		}
		return (l);
	}
	return (n);
err:
	find_freenode(n);
	fap->primtype = FIND_ERRARG;		/* Mark as "parse aborted"	*/
	return ((findn_t *)NULL);
}

LOCAL findn_t *
parseprim(fap)
	finda_t	*fap;
{
	sigjmps_t	jmp;
	sigjmps_t	*ojmp = fap->jmp;
	register findn_t *n;
	register char	*p;
		Llong	ll;

	n = allocnode(fap);
	if (n == (findn_t *)NULL) {
		fap->primtype = FIND_ERRARG;	/* Mark as "parse aborted"	*/
		return ((findn_t *)NULL);
	}

	fap->jmp = &jmp;
	if (sigsetjmp(jmp.jb, 1) != 0) {
		/*
		 * We come here from siglongjmp()
		 */
		find_freenode(n);
		fap->jmp = ojmp;		/* Restore old jump target */
		return ((findn_t *)NULL);
	}
	switch (n->op = fap->primtype) {

	/*
	 * Use simple to old (historic) shell globbing.
	 */
	case INAME:
	case ILNAME:
	case IPATH:
#ifndef	FNM_IGNORECASE
		ferrmsgno(fap->std[2],
			EX_BAD, _("The primary '-%s' is unsupported on this OS.\n"),
					tokennames[n->op]);
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
#endif
	case NAME:
	case PATH:
	case LNAME:
#ifndef	HAVE_FNMATCH
#define	HAVE_FNMATCH				/* We have fnmatch() in libschily */
#endif
#if	defined(HAVE_FNMATCH)
		n->this = nextarg(fap, n);
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
#endif
		/* FALLTHRU */
		/* Implement "fallback" to patmatch() if we have no fnmatch() */

	/*
	 * Use patmatch() which is a regular expression matcher that implements
	 * extensions that are compatible to old (historic) shell globbing.
	 */
	case IPAT:
	case IPPAT:
	case ILPAT:
		ferrmsgno(fap->std[2],
			EX_BAD, _("The primary '-%s' is currently unsupported.\n"),
					tokennames[n->op]);
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
	case PAT:
	case PPAT:
	case LPAT: {
		int	plen;

		plen = strlen(n->this = nextarg(fap, n));
		if (plen > fap->patlen)
			fap->patlen = plen;
		n->right = __fjmalloc(fap->std[2], sizeof (int)*plen,
						"space for pattern", fap->jmp);

		if ((n->val.i = patcompile((Uchar *)n->this, plen, (int *)n->right)) == 0) {
			ferrmsgno(fap->std[2],
				EX_BAD, _("Bad pattern in '-%s %s'.\n"),
						tokennames[n->op], n->this);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case SIZE: {
		char	*numarg;

		fap->walkflags &= ~WALK_NOSTAT;

		p = n->left = nextarg(fap, n);
		numarg = p;
		if (p[0] == '-' || p[0] == '+')
			numarg = ++p;
		p = astoll(p, &ll);
		if (p[0] == '\0') {
			/* EMPTY */
			;
		} else if (p[0] == 'c' && p[1] == '\0') {
			n->this = p;
		} else if (getllnum(numarg, &ll) == 1) {
			n->this = p;
		} else if (*p) {
			ferrmsgno(fap->std[2], EX_BAD,
			_("Non numeric character '%c' in '-size %s'.\n"),
				*p, n->left);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		n->val.size = ll;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case EMPTY:
		fap->walkflags &= ~WALK_NOSTAT;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case LINKS:
		fap->walkflags &= ~WALK_NOSTAT;

		p = n->left = nextarg(fap, n);
		if (p[0] == '-' || p[0] == '+')
			p++;
		p = astoll(p, &ll);
		if (*p) {
			ferrmsgno(fap->std[2], EX_BAD,
			_("Non numeric character '%c' in '-links %s'.\n"),
				*p, n->left);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		n->val.nlink = ll;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case INUM:
		fap->walkflags &= ~WALK_NOSTAT;

		p = n->left = nextarg(fap, n);
		if (p[0] == '-' || p[0] == '+')
			p++;
		p = astoll(p, &ll);
		if (*p) {
			ferrmsgno(fap->std[2], EX_BAD,
			_("Non numeric character '%c' in '-inum %s'.\n"),
				*p, n->left);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		n->val.ino = ll;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case LINKEDTO: {
		struct stat ns;

		fap->walkflags &= ~WALK_NOSTAT;

		if (stat(n->left = nextarg(fap, n), &ns) < 0) {
			ferrmsg(fap->std[2],
				_("Cannot stat '%s'.\n"), n->left);
			errjmp(fap, geterrno());
			/* NOTREACHED */
		}
		n->val.ino = ns.st_ino;
		n->val2.dev = ns.st_dev;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case AMIN:
	case CMIN:
	case MMIN:
		n->val2.i = 1;
	case TIME:
	case ATIME:
	case CTIME:
	case MTIME: {
		int	len;

		fap->walkflags &= ~WALK_NOSTAT;

		p = n->left = nextarg(fap, n);
		if (p[0] == '-' || p[0] == '+')
			p++;
		if (gettnum(p, &n->val.time) != 1) {
			ferrmsgno(fap->std[2], EX_BAD,
				_("Bad timespec in '-%s %s'.\n"),
				tokennames[n->op], n->left);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		if (n->val2.i)
			n->val.time *= 60;

		len = strlen(p);
		if (len > 0) {
			len = (Uchar)p[len-1];
			if (!(len >= '0' && len <= '9')) {
				if (n->val2.i) { /* -mmin, No ext. time spec */
					ferrmsgno(fap->std[2], EX_BAD,
					_("Unsupported timespec in '-%s %s'.\n"),
						tokennames[n->op], n->left);
					errjmp(fap, geterrno());
					/* NOTREACHED */
				}
				n->val2.i = 1;	/* Ext. time spec permitted */
			}
		}
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case NEWERAA:
	case NEWERCA:
	case NEWERMA: {
		struct stat ns;

		fap->walkflags &= ~WALK_NOSTAT;

		if (stat(n->left = nextarg(fap, n), &ns) < 0) {
			ferrmsg(fap->std[2],
				_("Cannot stat '%s'.\n"), n->left);
			errjmp(fap, geterrno());
			/* NOTREACHED */
		}
		n->val.time = ns.st_atime;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case NEWERAC:
	case NEWERCC:
	case NEWERMC: {
		struct stat ns;

		fap->walkflags &= ~WALK_NOSTAT;

		if (stat(n->left = nextarg(fap, n), &ns) < 0) {
			ferrmsg(fap->std[2],
				_("Cannot stat '%s'.\n"), n->left);
			errjmp(fap, geterrno());
			/* NOTREACHED */
		}
		n->val.time = ns.st_ctime;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case NEWERAM:
	case NEWERCM:
	case NEWERMM:
	case NEWER: {
		struct stat ns;

		fap->walkflags &= ~WALK_NOSTAT;

		if (stat(n->left = nextarg(fap, n), &ns) < 0) {
			ferrmsg(fap->std[2],
				_("Cannot stat '%s'.\n"), n->left);
			errjmp(fap, geterrno());
			/* NOTREACHED */
		}
		n->val.time = ns.st_mtime;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case TYPE:
		fap->walkflags &= ~WALK_NOSTAT;

		n->this = (char *)nextarg(fap, n);
		switch (*(n->this)) {

		/*
		 * 'b'lock, 'c'har, 'd'ir, 'D'oor,
		 * 'e'ventcount, 'f'ile, 'l'ink, 'p'ipe,
		 * 'P'ort event, 's'ocket
		 */
		case 'b': case 'c': case 'd': case 'D':
		case 'e': case 'f': case 'l': case 'p':
		case 'P': case 's':
			if ((n->this)[1] == '\0') {
				nexttoken(fap);
				fap->jmp = ojmp; /* Restore old jump target */
				return (n);
			}
		}
		ferrmsgno(fap->std[2], EX_BAD,
			_("Bad type '%c' in '-type %s'.\n"),
			*n->this, n->this);
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
		break;

	case FSTYPE:
		fap->walkflags &= ~WALK_NOSTAT;

#ifdef	HAVE_ST_FSTYPE
		n->this = (char *)nextarg(fap, n);
#else
		ferrmsgno(fap->std[2], EX_BAD,
			_("-fstype not supported by this OS.\n"));
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
#endif
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case LOCL:
		fap->walkflags &= ~WALK_NOSTAT;

#ifndef	HAVE_ST_FSTYPE
		ferrmsgno(fap->std[2], EX_BAD,
			_("-local not supported by this OS.\n"));
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
#endif
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

#ifdef	CHOWN
	case CHOWN:
#endif
	case USER: {
		struct  passwd  *pw;
		char		*u;

		fap->walkflags &= ~WALK_NOSTAT;

		u = n->left = nextarg(fap, n);
		if (u[0] == '-' || u[0] == '+')
			u++;
		if ((pw = getpwnam(u)) != NULL) {
			n->val.uid = pw->pw_uid;
		} else {
			if (*astoll(n->left, &ll)) {
				ferrmsgno(fap->std[2], EX_BAD,
				_("User '%s' not in passwd database.\n"),
				n->left);
				errjmp(fap, EX_BAD);
				/* NOTREACHED */
			}
			n->val.uid = ll;
		}
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

#ifdef	CHGRP
	case CHGRP:
#endif
	case GROUP: {
		struct  group	*gr;
		char		*g;

		fap->walkflags &= ~WALK_NOSTAT;

		g = n->left = nextarg(fap, n);
		if (g[0] == '-' || g[0] == '+')
			g++;
		if ((gr = getgrnam(g)) != NULL) {
			n->val.gid = gr->gr_gid;
		} else {
			if (*astoll(n->left, &ll)) {
				ferrmsgno(fap->std[2], EX_BAD,
				_("Group '%s' not in group database.\n"),
				n->left);
				errjmp(fap, EX_BAD);
				/* NOTREACHED */
			}
			n->val.gid = ll;
		}
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

#ifdef	CHMOD
	case CHMOD:
#endif
	case PERM:
		fap->walkflags &= ~WALK_NOSTAT;

		p = n->left = nextarg(fap, n);
		if (p[0] == '+') {
			if (p[1] == '0' ||
			    p[1] == 'u' || p[1] == 'g' || p[1] == 'o' || p[1] == 'a')
				p++;
			else
				n->left = "";
		}
		if (getperm(fap->std[2], p, tokennames[n->op],
				&n->val.mode, (mode_t)0,
				n->op == PERM ? GP_FPERM|GP_XERR:GP_NOX) < 0) {
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case MODE:
		fap->walkflags &= ~WALK_NOSTAT;

		ferrmsgno(fap->std[2], EX_BAD,
				_("-mode not yet implemented.\n"));
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case XDEV:
	case MOUNT:
		fap->walkflags &= ~WALK_NOSTAT;
		fap->walkflags |= WALK_MOUNT;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	case DEPTH:
		fap->walkflags |= WALK_DEPTH;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	case FOLLOW:
		fap->walkflags &= ~WALK_PHYS;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case MAXDEPTH:
	case MINDEPTH:
		p = n->left = nextarg(fap, n);
		p = astoll(p, &ll);
		if (*p) {
			ferrmsgno(fap->std[2], EX_BAD,
			_("Non numeric character '%c' in '-%s %s'.\n"),
				*p, tokennames[n->op], n->left);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		n->val.l = ll;
		if (n->op == MAXDEPTH)
			fap->maxdepth = ll;
		else
			fap->mindepth = ll;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case NOUSER:
	case NOGRP:
	case PACL:
	case XATTR:
	case SPARSE:
	case DOSTAT:
		fap->walkflags &= ~WALK_NOSTAT;
		/* FALLTHRU */
	case PRUNE:
	case LTRUE:
	case LFALSE:
	case READABLE:
	case WRITABLE:
	case EXECUTABLE:
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case OK_EXEC:
	case OK_EXECDIR:
	case EXEC:
	case EXECDIR: {
		int	i = 1;

		n->this = (char *)fap->Argv;	/* Cheat: Pointer is pointer */
		nextarg(fap, n);		/* Eat up cmd name	    */
		while ((p = nextarg(fap, n)) != NULL) {
			if (streql(p, ";"))
				break;
			else if (streql(p, "+") && streql(fap->Argv[-2], "{}")) {
				if (n->op == OK_EXECDIR || n->op == EXECDIR) {
					ferrmsgno(fap->std[2], EX_BAD,
					_("'-%s' does not yet work with '+'.\n"),
						tokennames[n->op]);
					errjmp(fap, EX_BAD);
					/* NOTREACHED */
				}
				n->op = fap->primtype = EXECPLUS;
				if (!pluscreate(fap->std[2], --i, (char **)n->this, fap)) {
					errjmp(fap, EX_BAD);
					/* NOTREACHED */
				}
				n->this = (char *)fap->plusp;
				break;
			}
			i++;
		}
		n->val.i = i;
#ifdef	PLUS_DEBUG
		if (0) {
			char **pp = (char **)n->this;
			for (i = 0; i < n->val.i; i++, pp++)
				printf("ARG %d '%s'\n", i, *pp);
		}
#endif
	}
	/* FALLTHRU */

	case LS:
		fap->walkflags &= ~WALK_NOSTAT;
		goto found_action;
	case FLS:
		fap->walkflags &= ~WALK_NOSTAT;
	case FPRINT:
	case FPRINT0:
	case FPRINTNNL:
		p = nextarg(fap, n);
		n->val.fp = fileopen(p, "wcta");
		if (n->val.fp == NULL) {
			ferrmsg(fap->std[2],
			_("Cannot open '%s' for '-%s'.\n"),
				p, tokennames[n->op]);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
	case PRINT:
	case PRINT0:
	case PRINTNNL:
	found_action:
		fap->found_action = TRUE;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case FIND_ENDARGS:
#ifdef	DEBUG
		ferrmsgno(fap->std[2], EX_BAD,
				_("ENDARGS in parseprim()\n"));
#endif
		ferrmsgno(fap->std[2], EX_BAD,
				_("Incomplete expression.\n"));
		find_freenode(n);
		fap->jmp = ojmp;		/* Restore old jump target */
		return ((findn_t *)NULL);

	case OPEN:
		nexttoken(fap);
		n->this = (char *)parse(fap);
		if (n->this == NULL) {
			find_freenode(n);
			fap->jmp = ojmp;	/* Restore old jump target */
			return ((findn_t *)NULL);
		}
		if (fap->primtype != CLOSE) {
			ferrmsgno(fap->std[2], EX_BAD,
				_("Found '%s', but ')' expected.\n"),
				fap->Argv[-1]);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		} else {
			nexttoken(fap);
			fap->jmp = ojmp;	/* Restore old jump target */
			return (n);
		}
		break;

	case CLOSE:
		/*
		 * The triggering arg is at fap->Argv[-2].
		 */
		ferrmsgno(fap->std[2], EX_BAD, _("Missing '('.\n"));
		errjmp(fap, EX_BAD);
		/* NOTREACHED */

	case LNOT:
		nexttoken(fap);
		n->this = (char *)parseprim(fap);
		if (n->this == NULL) {
			find_freenode(n);
			n = (findn_t *)NULL;
		}
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case AND:
	case LOR:
		ferrmsgno(fap->std[2], EX_BAD,
		_("Invalid expression with '-%s'.\n"), tokennames[n->op]);
		errjmp(fap, EX_BAD);
		/* NOTREACHED */

	case HELP:
		find_usage(fap->std[2]);
		find_freenode(n);
		fap->jmp = ojmp;		/* Restore old jump target */
		return ((findn_t *)NULL);

	default:
		ferrmsgno(fap->std[2], EX_BAD,
		_("Internal malfunction, found unknown primary '-%s' (%d).\n"),
			find_tname(n->op), n->op);
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
	}
	fap->jmp = ojmp;			/* Restore old jump target */
	fap->primtype = FIND_ERRARG;		/* Mark as "parse aborted" */
	return (0);
}

#define	S_ALLPERM	(S_IRWXU|S_IRWXG|S_IRWXO)
#define	S_ALLFLAGS	(S_ISUID|S_ISGID|S_ISVTX)
#define	S_ALLMODES	(S_ALLFLAGS | S_ALLPERM)

EXPORT void
find_firstprim(pac, pav)
	int	*pac;
	char    *const *pav[];
{
	register int	cac  = *pac;
	register char *const *cav = *pav;
	register char	c;

	while (cac > 0 &&
		(c = **cav) != '-' && c != '(' && c != ')' && c != '!') {
		cav++;
		cac--;
	}
	*pac = cac;
	*pav = cav;
}

EXPORT BOOL
find_primary(t, op)
	findn_t	*t;
	int	op;
{
	BOOL	ret = FALSE;

	if (t->op == op) {
		return (TRUE);
	}
	switch (t->op) {

	case OPEN:
		ret = find_primary((findn_t *)t->this, op);
		break;
	case LNOT:
		ret = find_primary((findn_t *)t->this, op);
		break;
	case AND:
		ret = find_primary((findn_t *)t->left, op);
		if (ret)
			return (ret);
		ret = find_primary((findn_t *)t->right, op);
		break;
	case LOR:
		ret = find_primary((findn_t *)t->left, op);
		if (ret)
			return (ret);
		ret = find_primary((findn_t *)t->right, op);
		break;

	default:
		;
	}
	return (ret);
}

EXPORT BOOL
find_pname(t, word)
	findn_t	*t;
	char	*word;
{
	if (streql(word, "-exec+"))
		return (find_primary(t, EXECPLUS));
	if (streql(word, "-execdir+"))
		return (find_primary(t, EXECDIRPLUS));
	return (find_primary(t, find_token(word)));
}

EXPORT BOOL
find_hasprint(t)
	findn_t	*t;
{
	if (t == NULL)
		return (FALSE);

	if (find_primary(t, PRINT) || find_primary(t, PRINTNNL) ||
	    find_primary(t, PRINT0))
		return (TRUE);
	if (find_primary(t, LS))
		return (TRUE);
	return (FALSE);
}

EXPORT BOOL
find_hasexec(t)
	findn_t	*t;
{
	if (t == NULL)
		return (FALSE);

	if (find_primary(t, EXEC) || find_primary(t, EXECPLUS))
		return (TRUE);
	if (find_primary(t, EXECDIR) || find_primary(t, EXECDIRPLUS))
		return (TRUE);
	if (find_primary(t, OK_EXEC) || find_primary(t, OK_EXECDIR))
		return (TRUE);
	return (FALSE);
}

#ifdef	FIND_MAIN
LOCAL int
walkfunc(nm, fs, type, state)
	char		*nm;
	struct stat	*fs;
	int		type;
	struct WALK	*state;
{
	if (type == WALK_NS) {
		ferrmsg(state->std[2], _("Cannot stat '%s'.\n"), nm);
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
#endif

#ifdef	__FIND__
LOCAL inline BOOL
#else
EXPORT BOOL
#endif
find_expr(f, ff, fs, state, t)
	char		*f;	/* path name */
	char		*ff;	/* file name */
	struct stat	*fs;
	struct WALK	*state;
	findn_t		*t;
{
	time_t	xtime;
	FILE	*fp = state->std[1];
	char	*p;
	char	lname[8192];
	int	fnflags = 0;

	switch (t->op) {

	case ILNAME:
#ifdef	FNM_IGNORECASE
		fnflags = FNM_IGNORECASE;
#endif
	case LNAME: {
		int	lsize;

		if (!S_ISLNK(fs->st_mode))
			return (FALSE);

		if (state->lname != NULL) {
			p = state->lname;
			goto nmatch;
		}
		lname[0] = '\0';
		/*
		 * For file names from the command line, we did not perform
		 * a chdir() before, so we need to use the full path name.
		 */
		lsize = readlink(state->level ? ff : f, lname, sizeof (lname));
		if (lsize < 0) {
			ferrmsg(state->std[2],
				_("Cannot read link '%s'.\n"), ff);
			return (FALSE);
		}
		lname[sizeof (lname)-1] = '\0';
		if (lsize >= 0)
			lname[lsize] = '\0';
		p = lname;
		goto nmatch;
	}
	case IPATH:
#ifdef	FNM_IGNORECASE
		fnflags = FNM_IGNORECASE;
#endif
	case PATH:
		p = f;
		goto nmatch;
	case INAME:
#ifdef	FNM_IGNORECASE
		fnflags = FNM_IGNORECASE;
#endif
	case NAME:
		p = ff;
	nmatch:
#if	defined(HAVE_FNMATCH)
		return (!fnmatch(t->this, p, fnflags));
#else
		goto pattern;		/* Use patmatch() as "fallback" */
#endif

	case LPAT: {
		int	lsize;

		if (!S_ISLNK(fs->st_mode))
			return (FALSE);

		if (state->lname != NULL) {
			p = state->lname;
			goto pattern;
		}
		lname[0] = '\0';
		/*
		 * For file names from the command line, we did not perform
		 * a chdir() before, so we need to use the full path name.
		 */
		lsize = readlink(state->level ? ff : f, lname, sizeof (lname));
		if (lsize < 0) {
			ferrmsg(state->std[2],
				_("Cannot read link '%s'.\n"), ff);
			return (FALSE);
		}
		lname[sizeof (lname)-1] = '\0';
		if (lsize >= 0)
			lname[lsize] = '\0';
		p = lname;
		goto pattern;
	}
	case PPAT:
		p = f;
		goto pattern;
	case PAT:
		p = ff;
	pattern: {
		Uchar	*pr;		/* patmatch() return */

		pr = patmatch((Uchar *)t->this, (int *)t->right,
			(Uchar *)p, 0, strlen(p), t->val.i, state->patstate);
		return (*p && pr && (*pr == '\0'));
	}

	case SIZE:
		switch (*(t->left)) {
		case '+':
			if (t->this)
				return (fs->st_size    > t->val.size);
			return ((fs->st_size+511)/512  > t->val.size);
		case '-':
			if (t->this)
				return (fs->st_size   <  t->val.size);
			return ((fs->st_size+511)/512 <  t->val.size);
		default:
			if (t->this)
				return (fs->st_size   == t->val.size);
			return ((fs->st_size+511)/512 == t->val.size);
		}

	case EMPTY:
		if (S_ISREG(fs->st_mode) && fs->st_size == 0)
			return (TRUE);
		/*
		 * For file names from the command line, we did not perform
		 * a chdir() before, so we need to use the full path name.
		 */
		if (S_ISDIR(fs->st_mode)) {
			struct dirent	*dp;
			DIR		*d = opendir(state->level ? ff : f);

			if (d == NULL) {
				ferrmsg(state->std[2],
					_("Cannot open directory '%s'.\n"),
					ff);
				return (FALSE);
			}
			while ((dp = readdir(d)) != NULL) {
				register char *name = dp->d_name;
				/*
				 * Skip the following names: "", ".", "..".
				 */
				if (name[name[0] != '.' ? 0 :
				    name[1] != '.' ? 1 : 2] == '\0')
					continue;
				closedir(d);
				return (FALSE);
			}
			closedir(d);
			return (TRUE);
		}
		return (FALSE);

	case LINKS:
		switch (*(t->left)) {
		case '+':
			return (fs->st_nlink  > t->val.nlink);
		case '-':
			return (fs->st_nlink <  t->val.nlink);
		default:
			return (fs->st_nlink == t->val.nlink);
		}

	case INUM:
		switch (*(t->left)) {
		case '+':
			return (fs->st_ino  > t->val.ino);
		case '-':
			return (fs->st_ino <  t->val.ino);
		default:
			return (fs->st_ino == t->val.ino);
		}

	case LINKEDTO:
			return ((fs->st_ino == t->val.ino) &&
				(fs->st_dev == t->val2.dev));

	case READABLE:
		t->val.i = R_OK;
		goto check_access;
	case WRITABLE:
		t->val.i = W_OK;
		goto check_access;
	case EXECUTABLE:
		t->val.i = X_OK;
	check_access:
		/*
		 * For file names from the command line, we did not perform
		 * a chdir() before, so we need to use the full path name.
		 */
		if (access(state->level ? ff : f, t->val.i) < 0)
			return (FALSE);
		return (TRUE);

	case AMIN:
	case ATIME:
		xtime = fs->st_atime;
		goto times;
	case CMIN:
	case CTIME:
		xtime = fs->st_ctime;
		goto times;
	case MMIN:
	case MTIME:
	case TIME:
		xtime = fs->st_mtime;
	times:
		if (t->val2.i != 0)
			goto timex;

		switch (*(t->left)) {
		case '+':
			return ((find_now-xtime)/DAYSECS >  t->val.time);
		case '-':
			return ((find_now-xtime)/DAYSECS <  t->val.time);
		default:
			return ((find_now-xtime)/DAYSECS == t->val.time);
		}
	timex:
		switch (*(t->left)) {
		case '+':
			return ((find_now-xtime) >  t->val.time);
		case '-':
			return ((find_now-xtime) <  t->val.time);
		default:
			return ((find_now-xtime) == t->val.time);
		}

	case NEWERAA:
	case NEWERAC:
	case NEWERAM:
		return (t->val.time < fs->st_atime);

	case NEWERCA:
	case NEWERCC:
	case NEWERCM:
		return (t->val.time < fs->st_ctime);

	case NEWER:
	case NEWERMA:
	case NEWERMC:
	case NEWERMM:
		return (t->val.time < fs->st_mtime);

	case TYPE:
		switch (*(t->this)) {
		case 'b':
			return (S_ISBLK(fs->st_mode));
		case 'c':
			return (S_ISCHR(fs->st_mode));
		case 'd':
			return (S_ISDIR(fs->st_mode));
		case 'D':
			return (S_ISDOOR(fs->st_mode));
		case 'e':
			return (S_ISEVC(fs->st_mode));
		case 'f':
			return (S_ISREG(fs->st_mode));
		case 'l':
			return (S_ISLNK(fs->st_mode));
		case 'p':
			return (S_ISFIFO(fs->st_mode));
		case 'P':
			return (S_ISPORT(fs->st_mode));
		case 's':
			return (S_ISSOCK(fs->st_mode));
		default:
			return (FALSE);
		}

	case FSTYPE:
#ifdef	HAVE_ST_FSTYPE
		return (streql(t->this, fs->st_fstype));
#else
		return (TRUE);
#endif

	case LOCL:
#ifdef	HAVE_ST_FSTYPE
		if (streql("nfs", fs->st_fstype) ||
		    streql("autofs", fs->st_fstype) ||
		    streql("cachefs", fs->st_fstype))
			return (FALSE);
#endif
		return (TRUE);

#ifdef	CHOWN
	case CHOWN:
		fs->st_uid = t->val.uid;
		return (TRUE);
#endif

	case USER:
		switch (*(t->left)) {
		case '+':
			return (fs->st_uid  > t->val.uid);
		case '-':
			return (fs->st_uid <  t->val.uid);
		default:
			return (fs->st_uid == t->val.uid);
		}

#ifdef	CHGRP
	case CHGRP:
		fs->st_gid = t->val.gid;
		return (TRUE);
#endif

	case GROUP:
		switch (*(t->left)) {
		case '+':
			return (fs->st_gid  > t->val.gid);
		case '-':
			return (fs->st_gid <  t->val.gid);
		default:
			return (fs->st_gid == t->val.gid);
		}

#ifdef	CHMOD
	case CHMOD:
		getperm(state->std[2], t->left, tokennames[t->op],
			&t->val.mode, fs->st_mode & S_ALLMODES,
			(S_ISDIR(fs->st_mode) ||
			(fs->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) != 0) ?
								GP_DOX:GP_NOX);
		fs->st_mode &= ~S_ALLMODES;
		fs->st_mode |= t->val.mode;
		return (TRUE);
#endif

	case PERM:
		if (t->left[0] == '+')
			return ((fs->st_mode & t->val.mode) != 0);
		else if (t->left[0] == '-')
			return ((fs->st_mode & t->val.mode) == t->val.mode);
		else
			return ((fs->st_mode & S_ALLMODES) == t->val.mode);

	case MODE:
		return (TRUE);

	case XDEV:
	case MOUNT:
	case DEPTH:
	case FOLLOW:
	case DOSTAT:
		return (TRUE);

	case NOUSER:
		return (getpwuid(fs->st_uid) == NULL);

	case NOGRP:
		return (getgrgid(fs->st_gid) == NULL);

	case PRUNE:
		state->flags |= WALK_WF_PRUNE;
		return (TRUE);

	case MAXDEPTH:
	case MINDEPTH:
		return (TRUE);

	case PACL:
		if (state->pflags & PF_ACL) {
			return ((state->pflags & PF_HAS_ACL) != 0);
		}
		return (has_acl(state->std[2], f, ff, fs));

	case XATTR:
		if (state->pflags & PF_XATTR) {
			return ((state->pflags & PF_HAS_XATTR) != 0);
		}
		return (has_xattr(state->std[2], ff));

	case SPARSE:
		if (!S_ISREG(fs->st_mode))
			return (FALSE);
#ifdef	HAVE_ST_BLOCKS
		return (fs->st_size > (fs->st_blocks * DEV_BSIZE + DEV_BSIZE));
#else
		return (FALSE);
#endif

	case OK_EXEC:
	case OK_EXECDIR: {
		char qbuf[32];

		fflush(state->std[1]);
		fprintf(state->std[2], "< %s ... %s > ? ", ((char **)t->this)[0], f);
		fflush(state->std[2]);
		fgetline(state->std[0], qbuf, sizeof (qbuf) - 1);

		switch (qbuf[0]) {
		case 'y':
			if (qbuf[1] == '\0' || streql(qbuf, "yes")) break;
		default:
			return (FALSE);
		}
	}
	/* FALLTHRU */

	case EXEC:
	case EXECDIR:
#ifdef	HAVE_FORK
		return (doexec(
			state->level && (t->op == OK_EXECDIR || t->op == EXECDIR)?
			ff:f,
			t, t->val.i, (char **)t->this, state));
#else
		ferrmsgno(state->std[2], EX_BAD,
		_("'-%s' is unsupported on this platform, returning FALSE.\n"),
				find_tname(t->op));
		return (FALSE);
#endif

	case EXECPLUS:
#ifdef	HAVE_FORK
		return (plusexec(f, t, state));
#else
		ferrmsgno(state->std[2], EX_BAD,
		_("'-%s' is unsupported on this platform, returning FALSE.\n"),
				find_tname(t->op));
		return (FALSE);
#endif

	case FPRINT:
		fp = t->val.fp;
		/* FALLTHRU */
	case PRINT:
		filewrite(fp, f, strlen(f));
		putc('\n', fp);
		return (TRUE);

	case FPRINT0:
		fp = t->val.fp;
		/* FALLTHRU */
	case PRINT0:
		filewrite(fp, f, strlen(f));
		putc('\0', fp);
		return (TRUE);

	case FPRINTNNL:
		fp = t->val.fp;
		/* FALLTHRU */
	case PRINTNNL:
		filewrite(fp, f, strlen(f));
		putc(' ', fp);
		return (TRUE);

	case FLS:
		fp = t->val.fp;
		/* FALLTHRU */
	case LS: {
		FILE	*std[3];
		/*
		 * The third parameter is the file name used for readlink()
		 * (inside find_list()) relatively to the current working
		 * directory. For file names from the command line, we did not
		 * perform a chdir() before, so we need to use the full path
		 * name.
		 */
		std[0] = state->std[0];
		std[1] = fp;
		std[2] = state->std[2];
		find_list(std, fs, f, state->level ? ff : f, state);
		return (TRUE);
	}
	case LTRUE:
		return (TRUE);

	case LFALSE:
		return (FALSE);

	case OPEN:
		return (find_expr(f, ff, fs, state, (findn_t *)t->this));
	case LNOT:
		return (!find_expr(f, ff, fs, state, (findn_t *)t->this));
	case AND:
		return (find_expr(f, ff, fs, state, (findn_t *)t->left) ?
			find_expr(f, ff, fs, state, (findn_t *)t->right) : 0);
	case LOR:
		return (find_expr(f, ff, fs, state, (findn_t *)t->left) ?
			1 : find_expr(f, ff, fs, state, (findn_t *)t->right));
	}
	if (!(state->pflags & 0x80000000)) {

		ferrmsgno(state->std[2], EX_BAD,
		_("Internal malfunction, found unknown primary '-%s' (%d).\n"),
				find_tname(t->op), t->op);
		state->pflags |= 0x80000000;
	}
	return (FALSE);		/* Unknown operator ??? */
}

#ifdef	HAVE_FORK
LOCAL BOOL
doexec(f, t, ac, av, state)
	char	*f;
	findn_t	*t;
	int	ac;
	char	**av;
	struct WALK *state;
{
#ifdef	HAVE_VFORK
	char	** volatile aav = NULL;
#endif
	pid_t	pid;
	int	retval;

#ifdef	HAVE_VFORK
	if (f && ac >= 32) {
		aav = malloc((ac+1) * sizeof (char **));
		if (aav == NULL) {
			ferrmsg(state->std[2], _("Cannot malloc arg vector for -exec.\n"));
			return (FALSE);
		}
	}
#endif
	if ((pid = vfork()) < 0) {
#ifdef	HAVE_VFORK
		ferrmsg(state->std[2], _("Cannot vfork child.\n"));
#else
		ferrmsg(state->std[2], _("Cannot fork child.\n"));
#endif
#ifdef	HAVE_VFORK
		/*
		 * Ugly code as a workaround for broken Linux include files
		 * that do not specify vfork() as a problem. This may
		 * cause aav to be != NULL even when malloc() above was never
		 * called. Freeing a random address on some platforms causes a
		 * coredump.
		 * As similar problems may exist on other platforms, where the
		 * correct fix to mark aav volatile does not work, we keep the
		 * workaround to check f and ac as well.
		 */
		if (aav && f && ac >= 32)
			free(aav);
#endif
		return (FALSE);
	}
	if (pid) {
		while (wait(&retval) != pid)
			/* LINTED */
			;
#ifdef	HAVE_VFORK
		/*
		 * Ugly code as a workaround for broken Linux include files
		 * that do not specify vfork() as a problem. This may
		 * cause aav to be != NULL even when malloc() above was never
		 * called. Freeing a random address on some platforms causes a
		 * coredump.
		 * As similar problems may exist on other platforms, where the
		 * correct fix to mark aav volatile does not work, we keep the
		 * workaround to check f and ac as well.
		 */
		if (aav && f && ac >= 32)
			free(aav);
#endif
		return (retval == 0);
	} else {
#ifdef	HAVE_VFORK
			char	*xav[32];
		register char	**pp2 = xav;
#endif
		register int	i;
		register char	**pp = av;
			int	err;

		/*
		 * This is the forked process and for this reason, we may
		 * call fcomerr() here without problems.
		 */
		if (t != NULL &&	/* Not called from find_plusflush() */
		    t->op != OK_EXECDIR && t->op != EXECDIR &&
		    walkhome(state) < 0) {
			fcomerr(state->std[2],
					_("Cannot chdir to '.'.\n"));
		}
#ifndef	F_SETFD
		walkclose(state);
#endif

#define	iscurlypair(p)	((p)[0] == '{' && (p)[1] == '}' && (p)[2] == '\0')

#ifdef	HAVE_VFORK
		if (aav)
			pp2 = aav;
#endif
		if (f) {				/* NULL for -exec+ */
			for (i = 0; i < ac; i++, pp++) {
				register char	*p = *pp;

#ifdef	HAVE_VFORK
				if (iscurlypair(p))	/* streql(p, "{}") */
					*pp2++ = f;
				else
					*pp2++ = p;
#else
				if (iscurlypair(p))	/* streql(p, "{}") */
					*pp = f;
#endif
			}
#ifdef	HAVE_VFORK
			if (aav)
				pp = aav;
			else
				pp = xav;
#endif
		} else {
			pp = av;
		}
#ifndef	HAVE_VFORK
		pp = av;
#endif
#ifdef	PLUS_DEBUG
		error("argsize %d\n",
			(plusp->endp - (char *)&plusp->nextargp[0]) -
			(plusp->laststr - (char *)&plusp->nextargp[1]));
#endif
		pp[ac] = NULL;	/* -exec {} \; is not NULL terminated */

		fexecve(av[0], state->std[0], state->std[1], state->std[2],
							pp, state->env);
		err = geterrno();
#ifdef	PLUS_DEBUG
		error("argsize %d\n",
			(plusp->endp - (char *)&plusp->nextargp[0]) -
			(plusp->laststr - (char *)&plusp->nextargp[1]));
#endif
		/*
		 * This is the forked process and for this reason, we may
		 * call _exit() here without problems.
		 */
		ferrmsgno(state->std[2], err,
			_("Cannot execute '%s'.\n"), av[0]);
		_exit(err);
		/* NOTREACHED */
		return (-1);
	}
}
#endif	/* HAVE FORK */

#ifndef	LINE_MAX
#define	LINE_MAX	1024
#endif

#if	defined(IS_MACOS_X) && defined(HAVE_CRT_EXTERNS_H)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * We need to fetch the address of "environ" using a hack.
 */
#include <crt_externs.h>
#define	environ	*_NSGetEnviron()
#else
extern	char **environ;
#endif

/*
 * Return the number of environment entries including the final NULL pointer.
 */
LOCAL int
countenv()
{
	register int	evs = 0;
	register char	**ep;

	for (ep = environ; *ep; ep++) {
		evs++;
	}
	evs++;			/* The environ NULL ptr at the end */
	return (evs);
}

/*
 * Return ARG_MAX - LINE_MAX - size of current environment.
 *
 * The return value is reduced by LINE_MAX to allow the called
 * program to do own exec(2) calls with slightly increased arg size.
 */
LOCAL int
argsize(xtype)
	int	xtype;
{
	static int	ret = 0;

	if (ret == 0) {
		register int	evs = 0;
		register char	**ep;

		for (ep = environ; *ep; ep++) {
			evs += strlen(*ep) + 1 + sizeof (ep);
		}
		evs += sizeof (char **); /* The environ NULL ptr at the end */

#ifdef	_SC_ARG_MAX
		ret = sysconf(_SC_ARG_MAX);
		if (ret < 0)
#ifdef	_POSIX_ARG_MAX
			ret = _POSIX_ARG_MAX;
#else
			ret = ARG_MAX;
#endif
#else	/* VV NO _SC_ARG_MAX VV */
#ifdef	ARG_MAX
		ret = ARG_MAX;
#else
#ifdef	NCARGS
		/*
		 * On Solaris: ARG_MAX = NCARGS - space for other stuff on
		 * initial stack. This size is 256 for 32 bit and 512 for
		 * 64 bit programs.
		 */
		ret = NCARGS - 256;	/* Let us do the same */
#endif
#endif
#endif

#ifdef	MULTI_ARG_MAX
		ret = xargsize(xtype, ret);
#endif

#ifdef	PLUS_DEBUG
		ret = 3000;
#define		LINE_MAX	100
		error("evs %d\n", evs);
#endif
		if (ret <= 0)
			ret = 10000;	/* Just a guess */

		ret -= evs;		/* Subtract current env size */
		if ((ret - LINE_MAX) > 0)
			ret -= LINE_MAX;
		else
			ret -= 100;
	}
	return (ret);
}

/*
 * Return the executable type:
 *
 *	0	unknown type -> default
 *	32	a 32 bit binary
 *	64	a 64 bit binary
 */
LOCAL int
extype(name)
	char	*name;
{
	int	f;
	char	*xname;
	char	elfbuf[8];

	xname = findinpath(name, X_OK, TRUE, NULL);
	if (name == NULL)
		xname = name;

	if ((f = open(xname, O_RDONLY|O_BINARY)) < 0) {
		if (xname != name)
			free(xname);
		return (0);
	}
	if (xname != name)
		free(xname);
	if (read(f, elfbuf, sizeof (elfbuf)) < sizeof (elfbuf)) {
		close(f);
		return (0);
	}
	close(f);

	/*
	 * We only support ELF binaries
	 */
	if (elfbuf[0] != 0x7F ||
	    elfbuf[1] != 'E' || elfbuf[2] != 'L' || elfbuf[3] != 'F')
		return (0);

	switch (elfbuf[4] & 0xFF) {

	case 1:	/* ELFCLASS32 */
		return (32);

	case 2:	/* ELFCLASS64 */
		return (64);
	}
	return (0);
}

#ifdef	MULTI_ARG_MAX
/*
 * If we have both _ARG_MAX32 and _ARG_MAX64 (currently on SunOS)
 * correct maxarg based on the target binary type.
 */
LOCAL int
xargsize(xtype, maxarg)
	int	xtype;
	int	maxarg;
{
	/*
	 * Set up a safe fallback in case we are not able to determine the
	 * binary type.
	 */
	if (maxarg > _ARG_MAX32)
		maxarg = _ARG_MAX32;

	switch (xtype) {

	case 32:
		maxarg = _ARG_MAX32;
		break;

	case 64:
		maxarg = _ARG_MAX64;
		break;
	}

	return (maxarg);
}
#endif

LOCAL BOOL
pluscreate(f, ac, av, fap)
	FILE	*f;
	int	ac;
	char	**av;
	finda_t	*fap;
{
	struct plusargs	*pp;
	register char	**ap = av;
	register int	i;
		int	mxtype;
		int	xtype;
		int	maxarg;
		int	nenv;

	xtype  = extype(av[0]);		/* Get -exec executable type	*/
	maxarg = argsize(xtype);	/* Get ARG_MAX for executable	*/
	nenv   = countenv();		/* # of ents in current env	*/

	mxtype = sizeof (char *) * CHAR_BIT;
	if (xtype == 0)
		mxtype = 0;

	if (xtype == mxtype)
		nenv = 0;		/* Easy case			*/
	else if (xtype > mxtype)
		nenv = -nenv;		/* Need to reduce arg size	*/

	maxarg += nenv * (32 / CHAR_BIT); /* Correct maxarg by ptr size	*/

#ifdef	PLUS_DEBUG
	printf("Argc %d\n", ac);
	ap = av;
	for (i = 0; i < ac; i++, ap++)
		printf("ARG %d '%s'\n", i, *ap);
#endif

	pp = __fjmalloc(fap->std[2], maxarg + sizeof (struct plusargs),
						"-exec args", fap->jmp);
	pp->laststr = pp->endp = (char *)(&pp->av[0]) + maxarg;
	pp->nenv = nenv;
	pp->ac = 0;
	pp->nextargp = &pp->av[0];

#ifdef	PLUS_DEBUG
	printf("pp          %d\n", pp);
	printf("pp->laststr %d\n", pp->laststr);
	printf("argsize()   %d\n", maxarg);
#endif

	/*
	 * Copy args from command line.
	 */
	ap = av;
	for (i = 0; i < ac; i++, ap++) {
#ifdef	PLUS_DEBUG
		printf("ARG %d '%s'\n", i, *ap);
#endif
		*(pp->nextargp++) = *ap;
		pp->laststr -= strlen(*ap) + 1;
		pp->ac++;
		if (pp->laststr <= (char *)pp->nextargp) {
			ferrmsgno(f, EX_BAD,
				_("No space to copy -exec args.\n"));
			free(pp);		/* The exec plusargs struct */
			return (FALSE);
		}
	}
#ifdef	PLUS_DEBUG
	error("lastr %d endp %d diff %d\n",
		pp->laststr, pp->endp, pp->endp - pp->laststr);
#endif
	pp->endp = pp->laststr;	/* Reduce endp by the size of cmdline args */

#ifdef	PLUS_DEBUG
	ap = &pp->av[0];
	for (i = 0; i < pp->ac; i++, ap++) {
		printf("ARG %d '%s'\n", i, *ap);
	}
#endif
#ifdef	PLUS_DEBUG
	printf("pp          %d\n", pp);
	printf("pp->laststr %d\n", pp->laststr);
#endif
	pp->next = fap->plusp;
	fap->plusp = pp;
#ifdef	PLUS_DEBUG
	plusp = fap->plusp;	/* Makes libfind not MT safe */
#endif
	return (TRUE);
}

#ifdef	HAVE_FORK
LOCAL BOOL
plusexec(f, t, state)
	char	*f;
	findn_t	*t;
	struct WALK *state;
{
	register struct plusargs *pp = (struct plusargs *)t->this;
#ifdef	PLUS_DEBUG
	register char	**ap;
	register int	i;
#endif
	size_t	size;
	size_t	slen = strlen(f) + 1;
	char	*nargp = (char *)&pp->nextargp[2];
	char	*cp;
	int	ret = TRUE;

	size = pp->laststr - (char *)&pp->nextargp[2];	/* Remaining strlen */

	if (pp->nenv < 0)				/* Called cmd has   */
		nargp += pp->ac * (32 / CHAR_BIT);	/* larger ptr size  */

	if (pp->laststr < nargp ||			/* Already full	    */
	    slen > size) {				/* str does not fit */
		pp->nextargp[0] = NULL;
		ret = doexec(NULL, t, pp->ac, pp->av, state);
		pp->laststr = pp->endp;
		pp->ac = t->val.i;
		pp->nextargp = &pp->av[t->val.i];
		size = pp->laststr - (char *)&pp->nextargp[2];
	}
	if (pp->laststr < nargp ||
	    slen > size) {
		ferrmsgno(state->std[2], EX_BAD,
			_("No space for arg '%s'.\n"), f);
		return (FALSE);
	}
	cp = pp->laststr - slen;
	strcpy(cp, f);
	pp->nextargp[0] = cp;
	pp->ac++;
	pp->nextargp++;
	pp->laststr -= slen;

#ifdef	PLUS_DEBUG
	ap = &plusp->av[0];
	for (i = 0; i < plusp->ac; i++, ap++) {
		printf("ARG %d '%s'\n", i, *ap);
	}
	error("EXECPLUS '%s'\n", f);
#endif
	return (ret);
}
#endif	/* HAVE_FORK */

EXPORT int
find_plusflush(p, state)
	void	*p;
	struct WALK *state;
{
	struct plusargs	*plusp = p;
	BOOL		ret = TRUE;

	/*
	 * Execute all unflushed '-exec .... {} +' expressions.
	 */
	while (plusp) {
#ifdef	PLUS_DEBUG
		error("lastr %p endp %p\n", plusp->laststr, plusp->endp);
#endif
		if (plusp->laststr != plusp->endp) {
			plusp->nextargp[0] = NULL;
#ifdef	HAVE_FORK
			if (!doexec(NULL, NULL, plusp->ac, plusp->av, state))
				ret = FALSE;
#endif
		}
		plusp = plusp->next;
	}
	return (ret);
}

EXPORT void
find_usage(f)
	FILE	*f;
{
	fprintf(f, _("Usage:	%s [options] [path_1 ... path_n] [expression]\n"), get_progname());
	fprintf(f, _("Options:\n"));
	fprintf(f, _("	-H	follow symbolic links encountered on command line\n"));
	fprintf(f, _("	-L	follow all symbolic links\n"));
	fprintf(f, _("*	-P	do not follow symbolic links (default)\n"));
	fprintf(f, _("*	-help	Print this help.\n"));
	fprintf(f, _("*	-version Print version number.\n"));
	fprintf(f, _("Operators in decreasing precedence:\n"));
	fprintf(f, _("	( )	group an expression\n"));
	fprintf(f, _("	!, -a, -o negate a primary (unary NOT), logical AND, logical OR\n"));
	fprintf(f, _("Primaries:\n"));
	fprintf(f, _("*	-acl	      TRUE if the file has additional ACLs defined\n"));
	fprintf(f, _("	-atime #      TRUE if st_atime is in specified range\n"));
#ifdef	CHGRP
	fprintf(f, _("*	-chgrp gname/gid always TRUE, sets st_gid to gname/gid\n"));
#endif
#ifdef	CHMOD
	fprintf(f, _("*	-chmod mode/onum always TRUE, sets permissions to mode/onum\n"));
#endif
#ifdef	CHOWN
	fprintf(f, _("*	-chown uname/uid always TRUE, sets st_uid to uname/uid\n"));
#endif
	fprintf(f, _("	-ctime #      TRUE if st_ctime is in specified range\n"));
	fprintf(f, _("	-depth	      evaluate directory content before directory (always TRUE)\n"));
	fprintf(f, _("*	-dostat	      Do not do stat optimization (always TRUE)\n"));
	fprintf(f, _("*	-empty	      TRUE zero sized plain file or empty directory\n"));
	fprintf(f, _("	-exec program [argument ...] \\;\n"));
	fprintf(f, _("	-exec program [argument ...] {} +\n"));
	fprintf(f, _("*	-execdir program [argument ...] \\;\n"));
	fprintf(f, _("*	-executable   TRUE if file is executable by real user id\n"));
	fprintf(f, _("*	-false	      always FALSE\n"));
	fprintf(f, _("*	-fls file     list files similar to 'ls -ilds' into 'file' (always TRUE)\n"));
	fprintf(f, _("*	-follow	      outdated: follow all symbolic links (always TRUE)\n"));
	fprintf(f, _("*	-fprint file  print file names line separated into 'file' (always TRUE)\n"));
	fprintf(f, _("*	-fprint0 file print file names nul separated into 'file' (always TRUE)\n"));
	fprintf(f, _("*	-fprintnnl file print file names space separated into 'file' (always TRUE)\n"));
	fprintf(f, _("*	-fstype type  TRUE if st_fstype matches type\n"));
	fprintf(f, _("	-group gname/gid TRUE if st_gid matches gname/gid\n"));
	fprintf(f, _("*	-ilname glob  TRUE if symlink name matches shell glob\n"));
	fprintf(f, _("*	-ilpat pattern TRUE if symlink name matches pattern\n"));
	fprintf(f, _("*	-iname glob   TRUE if path component matches shell glob\n"));
	fprintf(f, _("*	-inum #	      TRUE if st_ino is in specified range\n"));
	fprintf(f, _("*	-ipat pattern TRUE if path component matches pattern\n"));
	fprintf(f, _("*	-ipath glob   TRUE if full path matches shell glob\n"));
	fprintf(f, _("*	-ippat pattern TRUE if full path matches pattern\n"));
	fprintf(f, _("*	-linkedto path TRUE if the file is linked to path\n"));
	fprintf(f, _("	-links #      TRUE if st_nlink is in specified range\n"));
	fprintf(f, _("*	-lname glob   TRUE if symlink name matches shell glob\n"));
	fprintf(f, _("*	-local	      TRUE if st_fstype does not match remote fs types\n"));
	fprintf(f, _("*	-lpat pattern TRUE if symlink name matches pattern\n"));
	fprintf(f, _("*	-ls	      list files similar to 'ls -ilds' (always TRUE)\n"));
	fprintf(f, _("*	-maxdepth #   descend at most # directory levels (always TRUE)\n"));
	fprintf(f, _("*	-mindepth #   start tests at directory level # (always TRUE)\n"));
	fprintf(f, _("	-mtime #      TRUE if st_mtime is in specified range\n"));
	fprintf(f, _("	-name glob    TRUE if path component matches shell glob\n"));
	fprintf(f, _("	-newer file   TRUE if st_mtime newer then mtime of file\n"));
	fprintf(f, _("*	-newerXY file TRUE if [acm]time (X) newer then [acm]time (Y) of file\n"));
	fprintf(f, _("	-nogroup      TRUE if not in group database\n"));
	fprintf(f, _("	-nouser       TRUE if not in user database\n"));
	fprintf(f, _("	-ok program [argument ...] \\;\n"));
	fprintf(f, _("*	-okdir program [argument ...] \\;\n"));
	fprintf(f, _("*	-pat pattern  TRUE if path component matches pattern\n"));
	fprintf(f, _("*	-path glob    TRUE if full path matches shell glob\n"));
	fprintf(f, _("	-perm mode/onum TRUE if symbolic/octal permission matches\n"));
	fprintf(f, _("*	-ppat pattern TRUE if full path matches pattern\n"));
	fprintf(f, _("	-print	      print file names line separated to stdout (always TRUE)\n"));
	fprintf(f, _("*	-print0	      print file names nul separated to stdout (always TRUE)\n"));
	fprintf(f, _("*	-printnnl     print file names space separated to stdout (always TRUE)\n"));
	fprintf(f, _("	-prune	      do not descent current directory (always TRUE)\n"));
	fprintf(f, _("*	-readable     TRUE if file is readable by real user id\n"));
	fprintf(f, _("	-size #	      TRUE if st_size is in specified range\n"));
	fprintf(f, _("*	-sparse	      TRUE if file appears to be sparse\n"));
	fprintf(f, _("*	-true	      always TRUE\n"));
	fprintf(f, _("	-type c	      TRUE if file type matches, c is from (b c d D e f l p P s)\n"));
	fprintf(f, _("	-user uname/uid TRUE if st_uid matches uname/uid\n"));
	fprintf(f, _("*	-writable     TRUE if file is writable by real user id\n"));
	fprintf(f, _("*	-xattr	      TRUE if the file has extended attributes\n"));
	fprintf(f, _("	-xdev, -mount restrict search to current filesystem (always TRUE)\n"));
	fprintf(f, _("Primaries marked with '*' are POSIX extensions, avoid them in portable scripts.\n"));
	fprintf(f, _("If path is omitted, '.' is used. If expression is omitted, -print is used.\n"));
}

#ifdef FIND_MAIN

/* ARGSUSED */
LOCAL int
getflg(optstr, argp)
	char	*optstr;
	long	*argp;
{
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
main(ac, av)
	int	ac;
	char	**av;
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

	save_args(ac, av);

#ifdef	USE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain("SCHILY_FIND", "/opt/schily/lib/locale");
	textdomain("SCHILY_FIND");
#endif
	find_argsinit(&fa);
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
		find_usage(stderr);
		return (0);
	}
	if (prversion) {
		printf("sfind release %s (%s-%s-%s) Copyright (C) 2004-2008 Jörg Schilling\n",
				strvers,
				HOST_CPU, HOST_VENDOR, HOST_OS);
		return (0);
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
			return (fa.error);
		}
		if (fa.primtype != FIND_ENDARGS) {
			ferrmsgno(stderr, EX_BAD,
				_("Incomplete expression.\n"));
			find_free(Tree, &fa);
			return (EX_BAD);
		}
		if (find_pname(Tree, "-chown") || find_pname(Tree, "-chgrp") ||
		    find_pname(Tree, "-chmod")) {
			ferrmsgno(stderr, EX_BAD,
				_("Unsupported primary -chown/-chgrp/-chmod.\n"));
			find_free(Tree, &fa);
			return (EX_BAD);
		}
	} else {
		Tree = 0;
	}
	if (Tree == 0) {
		Tree = find_printnode();
	} else if (!fa.found_action) {
		Tree = find_addprint(Tree, &fa);
		if (Tree == (findn_t *)NULL)
			return (geterrno());
	}
	walkinitstate(&walkstate);
	if (fa.patlen > 0) {
		walkstate.patstate = __jmalloc(sizeof (int) * fa.patlen,
					"space for pattern state", JM_RETURN);
		if (walkstate.patstate == NULL)
			return (geterrno());
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
	return (walkstate.err);
}

#endif /* FIND_MAIN */
