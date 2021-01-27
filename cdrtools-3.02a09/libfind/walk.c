/* @(#)walk.c	1.46 16/03/10 Copyright 2004-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)walk.c	1.46 16/03/10 Copyright 2004-2016 J. Schilling";
#endif
/*
 *	Walk a directory tree
 *
 *	Copyright (c) 2004-2016 J. Schilling
 *
 *	In order to make treewalk() thread safe, we need to make it to not use
 *	chdir(2)/fchdir(2) which is process global.
 *
 *	chdir(newdir)	->	old = dfd;
 *				dfd = openat(old, newdir, O_RDONLY);
 *				close(old)
 *	fchdir(dd)	->	close(dfd); dfd = dd;
 *	stat(name)	->	fstatat(dfd, name, statb, 0)
 *	lstat(name)	-> 	fstatat(dfd, name, statb, AT_SYMLINK_NOFOLLOW)
 *	opendir(dname)	->	dd = openat(dfd, dname, O_RDONLY);
 *				dir = fdopendir(dd);
 *
 *	Similar changes need to be introduced in fetchdir().
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
#ifdef	HAVE_FCHDIR
#include <schily/fcntl.h>
#else
#include <schily/maxpath.h>
#endif
#include <schily/param.h>	/* NODEV */
#include <schily/stat.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/getcwd.h>
#include <schily/jmpdefs.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>
#include <schily/walk.h>
#include <schily/fetchdir.h>

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

#ifndef	HAVE_LSTAT
#define	lstat	stat
#endif
#ifndef	HAVE_DECL_STAT
extern int stat	__PR((const char *, struct stat *));
#endif
#ifndef	HAVE_DECL_LSTAT
extern int lstat __PR((const char *, struct stat *));
#endif

#define	IS_UFS(p)	((p)[0] == 'u' && \
			(p)[1] == 'f' && \
			(p)[2] == 's' && \
			(p)[3] == '\0')
#define	IS_ZFS(p)	((p)[0] == 'z' && \
			(p)[1] == 'f' && \
			(p)[2] == 's' && \
			(p)[3] == '\0')

#define	DIR_INCR	1024		/* How to increment Curdir size */
#define	TW_MALLOC	0x01		/* Struct was allocated		*/
struct twvars {
	char		*Curdir;	/* The current path name	*/
	int		Curdtail;	/* Where to append to Curdir	*/
	int		Curdlen;	/* Current size of 'Curdir'	*/
	int		Flags;		/* Flags related to this struct	*/
	struct WALK	*Walk;		/* Backpointer to struct WALK	*/
	struct stat	Sb;		/* stat(2) buffer for start dir	*/
#ifdef	HAVE_FCHDIR
	int		Home;		/* open fd to start CWD		*/
#else
	char		Home[MAXPATHNAME+1];	/* Abspath to start CWD	*/
#endif
};

struct pdirs {
	struct pdirs	*p_last; /* Previous node in list	*/
	dev_t		p_dev;	/* st_dev for this dir		*/
	ino_t		p_ino;	/* st_ino for this dir		*/
	BOOL		p_stat;	/* Need to call stat always	*/
	nlink_t		p_nlink; /* Number of subdirs to stat	*/
};


typedef	int	(*statfun)	__PR((const char *_nm, struct stat *_fs));

EXPORT	int	treewalk	__PR((char *nm, walkfun fn,
						struct WALK *_state));
LOCAL	int	walk		__PR((char *nm, statfun sf, walkfun fn,
						struct WALK *state,
						struct pdirs *last));
LOCAL	int	incr_dspace	__PR((FILE *f, struct twvars *varp, int amt));
EXPORT	void	walkinitstate	__PR((struct WALK *_state));
EXPORT	void	*walkopen	__PR((struct WALK *_state));
EXPORT	int	walkgethome	__PR((struct WALK *_state));
EXPORT	int	walkhome	__PR((struct WALK *_state));
EXPORT	int	walkclose	__PR((struct WALK *_state));
LOCAL	int	xchdotdot	__PR((struct pdirs *last, int tail,
						struct WALK *_state));
LOCAL	int	xchdir		__PR((char *p));

EXPORT int
treewalk(nm, fn, state)
	char		*nm;	/* The name to start walking		*/
	walkfun		fn;	/* The function to call for each node	*/
	struct WALK	*state; /* Walk state				*/
{
	struct twvars	vars;
	statfun		statf = stat;
	int		nlen;

	if ((state->walkflags & WALK_CHDIR) == 0) {
		seterrno(EINVAL);
		return (-1);
	}

	vars.Curdir  = NULL;
	vars.Curdlen = 0;
	vars.Curdtail = 0;
	vars.Flags = 0;
#ifdef	HAVE_FCHDIR
	vars.Home = -1;
#endif
	state->twprivate = &vars;
	vars.Walk = state;
	if (walkgethome(state) < 0) {
		state->twprivate = NULL;
		return (-1);
	}

	if (nm == NULL || nm[0] == '\0') {
		nm = ".";
	} else if (state->walkflags & WALK_STRIPLDOT) {
		if (nm[0] == '.' && nm[1] == '/') {
			for (nm++; nm[0] == '/'; nm++)
				/* LINTED */
				;
		}
	}

	vars.Curdir = __fjmalloc(state->std[2],
					DIR_INCR, "path buffer", JM_RETURN);
	if (vars.Curdir == NULL)
		return (-1);
	vars.Curdir[0] = 0;
	vars.Curdlen = DIR_INCR;
	/*
	 * If initial Curdir space is not sufficient, expand it.
	 */
	nlen = strlen(nm);
	if ((vars.Curdlen - 2) < nlen)
		if (incr_dspace(state->std[2], &vars, nlen + 2) < 0)
			return (-1);

	while (lstat(nm, &vars.Sb) < 0 && geterrno() == EINTR)
		;

	state->flags = 0;
	state->base = 0;
	state->level = 0;

	if (state->walkflags & WALK_PHYS)
		statf = lstat;

	if (state->walkflags & (WALK_ARGFOLLOW|WALK_ALLFOLLOW))
		statf = stat;

	nlen = walk(nm, statf, fn, state, (struct pdirs *)0);
	walkhome(state);
	walkclose(state);

	free(vars.Curdir);
	state->twprivate = NULL;
	return (nlen);
}

LOCAL int
walk(nm, sf, fn, state, last)
	char		*nm;	/* The current name for the walk	*/
	statfun		sf;	/* stat() or lstat()			*/
	walkfun		fn;	/* The function to call for each node	*/
	struct WALK	*state;	/* For communication with (*fn)()	*/
	struct pdirs	*last;	/* This helps to avoid loops		*/
{
	struct pdirs	thisd;
	struct stat	fs;
	int		type;
	int		ret;
	int		otail;
	char		*p;
	struct twvars	*varp = state->twprivate;

	otail = varp->Curdtail;
	state->base = otail;
	if (varp->Curdtail == 0 || varp->Curdir[varp->Curdtail-1] == '/') {
		if (varp->Curdtail == 0 &&
		    (state->walkflags & WALK_STRIPLDOT) &&
		    (nm[0] == '.' && nm[1] == '\0')) {
			varp->Curdir[0] = '.';
			varp->Curdir[1] = '\0';
		} else {
			p = strcatl(&varp->Curdir[varp->Curdtail], nm,
								(char *)0);
			varp->Curdtail = p - varp->Curdir;
		}
	} else {
		p = strcatl(&varp->Curdir[varp->Curdtail], "/", nm, (char *)0);
		varp->Curdtail = p - varp->Curdir;
		state->base++;
	}

	if ((state->walkflags & WALK_NOSTAT) &&
	    (
#ifdef	HAVE_DIRENT_D_TYPE
	    (state->flags & WALK_WF_NOTDIR) ||
#endif
	    (last != NULL && !last->p_stat && last->p_nlink <= 0))) {
		/*
		 * Set important fields to useful values to make sure that
		 * no wrong information is evaluated in the no stat(2) case.
		 */
		fs.st_mode = 0;
		fs.st_ino = 0;
		fs.st_dev = NODEV;
		fs.st_nlink = 0;
		fs.st_size = 0;

		type = WALK_F;
		state->flags = 0;

		goto type_known;
	} else {
		while ((ret = (*sf)(nm, &fs)) < 0 && geterrno() == EINTR)
			;
	}
	state->flags = 0;
	if (ret >= 0) {
#ifdef	HAVE_ST_FSTYPE
		/*
		 * Check for autofs mount points...
		 */
		if (fs.st_fstype[0] == 'a' &&
		    strcmp(fs.st_fstype, "autofs") == 0) {
			int	f = open(nm, O_RDONLY|O_NDELAY);
			if (f < 0) {
				type = WALK_DNR;
			} else {
				if (fstat(f, &fs) < 0)
					type = WALK_NS;
				close(f);
			}
		}
#endif
		if (S_ISDIR(fs.st_mode)) {
			type = WALK_D;
			if (last != NULL && !last->p_stat && last->p_nlink > 0)
				last->p_nlink--;
		} else if (S_ISLNK(fs.st_mode))
			type = WALK_SL;
		else
			type = WALK_F;
	} else {
		int	err = geterrno();
		while ((ret = lstat(nm, &fs)) < 0 && geterrno() == EINTR)
			;
		if (ret >= 0 &&
		    S_ISLNK(fs.st_mode)) {
			seterrno(err);
			/*
			 * Found symbolic link that points to nonexistent file.
			 */
			ret = (*fn)(varp->Curdir, &fs, WALK_SLN, state);
			goto out;
		} else {
			/*
			 * Found unknown file type because lstat() failed.
			 */
			ret = (*fn)(varp->Curdir, &fs, WALK_NS, state);
			goto out;
		}
	}
	if ((state->walkflags & WALK_MOUNT) != 0 &&
	    varp->Sb.st_dev != fs.st_dev) {
		ret = 0;
		goto out;
	}

type_known:
	if (type == WALK_D) {
		BOOL		isdot = (nm[0] == '.' && nm[1] == '\0');
		struct pdirs	*pd = last;

		ret = 0;
		if ((state->walkflags & (WALK_PHYS|WALK_ALLFOLLOW)) == WALK_PHYS)
			sf = lstat;

		/*
		 * Search parent dir structure for possible loops.
		 * If a loop is found, do not descend.
		 */
		thisd.p_last = last;
		thisd.p_dev  = fs.st_dev;
		thisd.p_ino  = fs.st_ino;
		if (state->walkflags & WALK_NOSTAT && fs.st_nlink >= 2) {
			thisd.p_stat  = FALSE;
			thisd.p_nlink  = fs.st_nlink - 2;
#ifdef	HAVE_ST_FSTYPE
			if (!IS_UFS(fs.st_fstype) &&
			    !IS_ZFS(fs.st_fstype))
				thisd.p_stat  = TRUE;
#else
			thisd.p_stat  = TRUE;	/* Safe fallback */
#endif
		} else {
			thisd.p_stat  = TRUE;
			thisd.p_nlink  = 1;
		}

		while (pd) {
			if (pd->p_dev == fs.st_dev &&
			    pd->p_ino == fs.st_ino) {
				/*
				 * Found a directory that has been previously
				 * visited already. This may happen with hard
				 * linked directories. We found a loop, so do
				 * not descend this directory.
				 */
				ret = (*fn)(varp->Curdir, &fs, WALK_DP, state);
				goto out;
			}
			pd = pd->p_last;
		}

		if ((state->walkflags & WALK_DEPTH) == 0) {
			/*
			 * Found a directory, check the content past this dir.
			 */
			ret = (*fn)(varp->Curdir, &fs, type, state);

			if (state->flags & WALK_WF_PRUNE)
				goto out;
		}

		if (!isdot && chdir(nm) < 0) {
			state->flags |= WALK_WF_NOCHDIR;
			/*
			 * Found a directory that does not allow chdir() into.
			 */
			ret = (*fn)(varp->Curdir, &fs, WALK_DNR, state);
			state->flags &= ~WALK_WF_NOCHDIR;
			goto out;
		} else {
			char	*dp;
			char	*odp;
			int	nents;
			int	Dspace;

			/*
			 * Add space for '/' & '\0'
			 */
			Dspace = varp->Curdlen - varp->Curdtail - 2;

			if ((dp = fetchdir(".", &nents, NULL, NULL)) == NULL) {
				/*
				 * Found a directory that cannot be read.
				 */
				ret = (*fn)(varp->Curdir, &fs, WALK_DNR, state);
				goto skip;
			}

			odp = dp;
			while (nents > 0 && ret == 0) {
				register char	*name;
				register int	nlen;

				name = &dp[1];
				nlen = strlen(name);

				if (Dspace < nlen) {
					int incr = incr_dspace(state->std[2],
								varp, nlen + 2);
					if (incr < 0) {
						ret = -1;
						break;
					}
					Dspace += incr;
				}
#ifdef	HAVE_DIRENT_D_TYPE
				if (dp[0] != FDT_DIR && dp[0] != FDT_UNKN)
					state->flags |= WALK_WF_NOTDIR;
#endif
				state->level++;
				ret = walk(name, sf, fn, state, &thisd);
				state->level--;

				if (state->flags & WALK_WF_QUIT)
					break;
				nents--;
				dp += nlen +2;
			}
			free(odp);
		skip:
			if (!isdot && state->level > 0 && xchdotdot(last,
							otail, state) < 0) {
				ret = geterrno();
				state->flags |= WALK_WF_NOHOME;
				if ((state->walkflags & WALK_NOMSG) == 0) {
					ferrmsg(state->std[2],
					_(
					"Cannot chdir to '..' from '%s/'.\n"),
						varp->Curdir);
				}
				if ((state->walkflags & WALK_NOEXIT) == 0)
					comexit(ret);
				ret = -1;
				goto out;
			}
			if (ret < 0)		/* Do not call callback	    */
				goto out;	/* func past fatal errors   */
		}
		if ((state->walkflags & WALK_DEPTH) != 0) {
			if (varp->Curdtail == 0 &&
			    (state->walkflags & WALK_STRIPLDOT) &&
			    (nm[0] == '.' && nm[1] == '\0')) {
				varp->Curdir[0] = '.';
				varp->Curdir[1] = '\0';
			}
			ret = (*fn)(varp->Curdir, &fs, type, state);
		}
	} else {
		/*
		 * Any other non-directory and non-symlink file type.
		 */
		ret = (*fn)(varp->Curdir, &fs, type, state);
	}
out:
	varp->Curdir[otail] = '\0';
	varp->Curdtail = otail;
	return (ret);
}

LOCAL int
incr_dspace(f, varp, amt)
	FILE		*f;
	struct twvars	*varp;
	int		amt;
{
	int	incr = DIR_INCR;
	char	*new;

	if (amt < 0)
		amt = 0;
	while (incr < amt)
		incr += DIR_INCR;
	new = __fjrealloc(f, varp->Curdir, varp->Curdlen + incr,
						"path buffer", JM_RETURN);
	if (new == NULL)
		return (-1);
	varp->Curdir = new;
	varp->Curdlen += incr;
	return (incr);
}

/*
 * Call first to create a useful WALK state default.
 */
EXPORT void
walkinitstate(state)
	struct WALK	*state;
{
	state->flags = state->base = state->level = state->walkflags = 0;
	state->twprivate = NULL;
	state->std[0] = stdin;
	state->std[1] = stdout;
	state->std[2] = stderr;
	state->env = environ;
	state->quitfun = NULL;
	state->qfarg = NULL;
	state->maxdepth = state->mindepth = 0;
	state->lname = state->tree = state->patstate = NULL;
	state->err = state->pflags = state->auxi = 0;
	state->auxp = NULL;
}

/*
 * For users that do not call treewalk(), e.g. star in extract mode.
 */
EXPORT void *
walkopen(state)
	struct WALK	*state;
{
	struct twvars	*varp = __fjmalloc(state->std[2],
					sizeof (struct twvars), "walk vars",
								JM_RETURN);

	if (varp == NULL)
		return (NULL);
	varp->Curdir  = NULL;
	varp->Curdlen = 0;
	varp->Curdtail = 0;
	varp->Flags = TW_MALLOC;
#ifdef	HAVE_FCHDIR
	varp->Home = -1;
#else
	varp->Home[0] = '\0';
#endif
	state->twprivate = varp;
	varp->Walk = state;

	return ((void *)varp);
}

EXPORT int
walkgethome(state)
	struct WALK	*state;
{
	struct twvars	*varp = state->twprivate;
	int		err = EX_BAD;

	if (varp == NULL) {
		if ((state->walkflags & WALK_NOMSG) == 0)
			ferrmsg(state->std[2],
				_("walkgethome: NULL twprivate\n"));
		if ((state->walkflags & WALK_NOEXIT) == 0)
			comexit(err);
		return (-1);
	}
#ifdef	HAVE_FCHDIR
	if (varp->Home >= 0)
		close(varp->Home);
	if ((varp->Home = open(".", O_SEARCH|O_NDELAY)) < 0) {
		err = geterrno();
		state->flags |= WALK_WF_NOCWD;
		if ((state->walkflags & WALK_NOMSG) == 0)
			ferrmsg(state->std[2],
				_("Cannot get working directory.\n"));
		if ((state->walkflags & WALK_NOEXIT) == 0)
			comexit(err);
		return (-1);
	}
#ifdef	F_SETFD
	fcntl(varp->Home, F_SETFD, FD_CLOEXEC);
#endif
#else
	if (getcwd(varp->Home, sizeof (varp->Home)) == NULL) {
		err = geterrno();
		state->flags |= WALK_WF_NOCWD;
		if ((state->walkflags & WALK_NOMSG) == 0)
			ferrmsg(state->std[2],
				_("Cannot get working directory.\n"));
		if ((state->walkflags & WALK_NOEXIT) == 0)
			comexit(err);
		return (-1);
	}
#endif
	return (0);
}

EXPORT int
walkhome(state)
	struct WALK	*state;
{
	struct twvars	*varp = state->twprivate;

	if (varp == NULL)
		return (0);
#ifdef	HAVE_FCHDIR
	if (varp->Home >= 0)
		return (fchdir(varp->Home));
#else
	if (varp->Home[0] != '\0')
		return (chdir(varp->Home));
#endif
	return (0);
}

EXPORT int
walkclose(state)
	struct WALK	*state;
{
	int		ret = 0;
	struct twvars	*varp = state->twprivate;

	if (varp == NULL)
		return (0);
#ifdef	HAVE_FCHDIR
	if (varp->Home >= 0)
		ret = close(varp->Home);
	varp->Home = -1;
#else
	varp->Home[0] = '\0';
#endif
	if (varp->Flags & TW_MALLOC) {
		free(varp);
		state->twprivate = NULL;
	}
	return (ret);
}

LOCAL int
xchdotdot(last, tail, state)
	struct pdirs	*last;
	int		tail;
	struct WALK	*state;
{
	struct twvars	*varp = state->twprivate;
	char	c;
	struct stat sb;

	if (chdir("..") >= 0) {
		seterrno(0);
		if (stat(".", &sb) >= 0) {
			if (sb.st_dev == last->p_dev &&
			    sb.st_ino == last->p_ino)
				return (0);
		}
	}

	if (walkhome(state) < 0)
		return (-1);

	c = varp->Curdir[tail];
	varp->Curdir[tail] = '\0';
	if (chdir(varp->Curdir) < 0) {
		if (geterrno() != ENAMETOOLONG) {
			varp->Curdir[tail] = c;
			return (-1);
		}
		if (xchdir(varp->Curdir) < 0) {
			varp->Curdir[tail] = c;
			return (-1);
		}
	}
	varp->Curdir[tail] = c;
	seterrno(0);
	if (stat(".", &sb) >= 0) {
		if (sb.st_dev == last->p_dev &&
		    sb.st_ino == last->p_ino)
			return (0);
	}
	return (-1);
}

LOCAL int
xchdir(p)
	char	*p;
{
	char	*p2;

	while (*p) {
		if ((p2 = strchr(p, '/')) != NULL)
			*p2 = '\0';
		if (chdir(p) < 0)
			return (-1);
		if (p2 == NULL)
			break;
		*p2 = '/';
		p = &p2[1];
	}
	return (0);
}
