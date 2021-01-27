/* @(#)parse.c	1.11 16/02/14 Copyright 2001-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)parse.c	1.11 16/02/14 Copyright 2001-2010 J. Schilling";
#endif
/*
 *	Interactive command parser for cdda2wav
 *
 *	Copyright (c) 2001-2010 J. Schilling
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

/*
 *	Commands:
 *		read	<start-sector>
 *
 *	Replies:
 *		200	OK
 *		400	Bad Request
 *		404	Not Found
 */

#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/ctype.h>
#include <schily/jmpdefs.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/varargs.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include "toc.h"

#define	E_ISOK		200	/* E_OK is used with euid access() */
#define	E_BAD		400
#define	E_NOTFOUND	404

#define	C_BAD		0
#define	C_STOP		1
#define	C_CONT		2
#define	C_READ		3
#define	C_EXIT		4
#define	C_HELP		5

typedef struct cmd {
	int	cmd;
	int	argtype;
	long	arg;
} cmd_t;

typedef struct keyw {
	char	*k_name;
	int	k_type;
} keyw_t;

LOCAL keyw_t	keywords[] = {
	{ "stop",	C_STOP },
	{ "cont",	C_CONT },
	{ "read",	C_READ },
	{ "exit",	C_EXIT },
	{ "quit",	C_EXIT },
	{ "help",	C_HELP },
	{ NULL,		0 },
};

typedef struct err {
	int	num;
	char	*name;
} err_t;

LOCAL err_t	errs[] = {
	{ E_ISOK,	"OK" },
	{ E_BAD,	"Bad Request" },
	{ E_NOTFOUND,	"Not Found" },
	{ -1,		NULL },
};

LOCAL	sigjmps_t	jmp;


EXPORT	int	parse		__PR((long *lp));
LOCAL	keyw_t	*lookup		__PR((char *word, keyw_t table[]));

LOCAL	FILE	*pfopen		__PR((char *name));
#ifdef	__needed__
LOCAL	char	*pfname		__PR((void));
#endif
LOCAL	char	*nextline	__PR((FILE *f));
#ifdef	__needed__
LOCAL	void	ungetline	__PR((void));
#endif
LOCAL	char	*skipwhite	__PR((const char *s));
LOCAL	char	*peekword	__PR((void));
LOCAL	char	*lineend	__PR((void));
LOCAL	char	*markword	__PR((char *delim));
#ifdef	__needed__
LOCAL	char	getworddelim	__PR((void));
#endif
LOCAL	char	*getnextitem	__PR((char *delim));
#ifdef	__needed__
LOCAL	char	*neednextitem	__PR((char *delim));
#endif
LOCAL	char	*nextword	__PR((void));
#ifdef	__needed__
LOCAL	char	*needword	__PR((void));
LOCAL	char	*curword	__PR((void));
LOCAL	char	*nextitem	__PR((void));
LOCAL	char	*needitem	__PR((void));
#endif
LOCAL	void	checkextra	__PR((void));
LOCAL	void	pabort		__PR((int errnum, const char *fmt, ...));
LOCAL	void	wok		__PR((void));
LOCAL	void	pusage		__PR((void));


EXPORT int
parse(lp)
	long	*lp;
{
		long	l;
	register keyw_t	*kp;
		char	*p;
		cmd_t	cmd;
static		FILE	*f;

	if (f == NULL)
		f = pfopen(NULL);
	if (f == NULL)
		return (-1);
again:
	if (sigsetjmp(jmp.jb, 1) != 0) {
		/*
		 * We come here from siglongjmp()
		 */
		;
	}
	if ((p = nextline(f)) == NULL)
		return (-1);

	p = nextword();
	kp = lookup(p, keywords);
	if (kp) {
		extern	void drop_all_buffers	__PR((void));

		cmd.cmd = kp->k_type;
		switch (kp->k_type) {

		case C_STOP:
			/* Flush buffers */
#ifdef	_is_working_
			drop_all_buffers();
#endif
			wok();
			goto again;
		case C_CONT:
			wok();
			return (0);
		case C_READ:
			p = nextword();
			if (streql(p, "sectors")) {
				p = nextword();
				if (*astol(p, &l) != '\0') {
					pabort(E_BAD, _("Not a number '%s'"), p);
				}
				*lp = l;
			} else if (streql(p, "tracks")) {
				p = nextword();
				if (*astol(p, &l) != '\0') {
					pabort(E_BAD, _("Not a number '%s'"), p);
				}
				if (l < FirstAudioTrack() || l > LastAudioTrack())
					pabort(E_BAD, _("Bad track number '%s'"), p);
				*lp = Get_StartSector(l);
			} else {
				pabort(E_BAD, _("Bad 'read' parameter '%s'"), p);
			}
			wok();
			break;
		case C_EXIT:
			wok();
			return (-1);
		case C_HELP:
			pusage();
			wok();
			goto again;
		default:
			pabort(E_NOTFOUND, _("Unknown command '%s'"), p);
			return (0);
		}
		checkextra();
		return (0);
	}
/*	checkextra();*/
	pabort(E_NOTFOUND, _("Unknown command '%s'"), p);
	return (0);
}

LOCAL keyw_t *
lookup(word, table)
	char	*word;
	keyw_t	table[];
{
	register keyw_t	*kp = table;

	while (kp->k_name) {
		if (streql(kp->k_name, word))
			return (kp);
		kp++;
	}
	return (NULL);
}

/*--------------------------------------------------------------------------*/
/*
 * Parser low level functions start here...
 */

LOCAL	char	linebuf[4096];
LOCAL	char	*fname;
LOCAL	char	*linep;
LOCAL	char	*wordendp;
LOCAL	char	wordendc;
LOCAL	int	olinelen;
LOCAL	int	linelen;
LOCAL	int	lineno;

LOCAL	char	worddelim[] = "=:,/";
#ifdef	__needed__
LOCAL	char	nulldelim[] = "";
#endif

#ifdef	DEBUG
LOCAL void
wdebug()
{
		printf("WORD: '%s' rest '%s'\n", linep, peekword());
		printf("linep %lX peekword %lX end %lX\n",
			(long)linep, (long)peekword(), (long)&linebuf[linelen]);
}
#endif

LOCAL FILE *
pfopen(name)
	char	*name;
{
	FILE	*f;

	if (name == NULL) {
		fname = "stdin";
		return (stdin);
	}
	f = fileopen(name, "r");
	if (f == NULL)
		comerr(_("Cannot open '%s'.\n"), name);

	fname = name;
	return (f);
}

#ifdef	__needed__
LOCAL char *
pfname()
{
	return (fname);
}
#endif

LOCAL char *
nextline(f)
	FILE	*f;
{
	do {
		register int	len;

		fillbytes(linebuf, sizeof (linebuf), '\0');
		len = fgetline(f, linebuf, sizeof (linebuf));
		if (len < 0)
			return (NULL);
		if (len > 0 && linebuf[len-1] == '\r') {
			linebuf[len-1] = '\0';
			len--;
		}
		linelen = len;
		lineno++;
	} while (linebuf[0] == '#');

	olinelen = linelen;
	linep = linebuf;
	wordendp = linep;
	wordendc = *linep;

	return (linep);
}

#ifdef	__needed__
LOCAL void
ungetline()
{
	linelen = olinelen;
	linep = linebuf;
	*wordendp = wordendc;
	wordendp = linep;
	wordendc = *linep;
}
#endif

LOCAL char *
skipwhite(s)
	const char	*s;
{
	register const Uchar	*p = (const Uchar *)s;

	while (*p) {
		if (!isspace(*p))
			break;
		p++;
	}
	return ((char *)p);
}

LOCAL char *
peekword()
{
	return (&wordendp[1]);
}

LOCAL char *
lineend()
{
	return (&linebuf[linelen]);
}

LOCAL char *
markword(delim)
	char	*delim;
{
	register	BOOL	quoted = FALSE;
	register	Uchar	c;
	register	Uchar	*s;
	register	Uchar	*from;
	register	Uchar	*to;

	for (s = (Uchar *)linep; (c = *s) != '\0'; s++) {
		if (c == '"') {
			quoted = !quoted;
			for (to = s, from = &s[1]; *from; ) {
				c = *from++;
				if (c == '\\' && quoted && (*from == '\\' || *from == '"'))
					c = *from++;
				*to++ = c;
			}
			*to = '\0';
			c = *s;
linelen--;
		}
		if (!quoted && isspace(c))
			break;
		if (!quoted && strchr(delim, c) && s > (Uchar *)linep)
			break;
	}
	wordendp = (char *)s;
	wordendc = (char)*s;
	*s = '\0';

	return (linep);
}

#ifdef	__needed__
LOCAL char
getworddelim()
{
	return (wordendc);
}
#endif

LOCAL char *
getnextitem(delim)
	char	*delim;
{
	*wordendp = wordendc;

	linep = skipwhite(wordendp);
	return (markword(delim));
}

#ifdef	__needed__
LOCAL char *
neednextitem(delim)
	char	*delim;
{
	char	*olinep = linep;
	char	*nlinep;

	nlinep = getnextitem(delim);

	if ((olinep == nlinep) || (*nlinep == '\0'))
		pabort(E_BAD, _("Missing text"));

	return (nlinep);
}
#endif

LOCAL char *
nextword()
{
	return (getnextitem(worddelim));
}

#ifdef	__needed__
LOCAL char *
needword()
{
	return (neednextitem(worddelim));
}

LOCAL char *
curword()
{
	return (linep);
}

LOCAL char *
nextitem()
{
	return (getnextitem(nulldelim));
}

LOCAL char *
needitem()
{
	return (neednextitem(nulldelim));
}
#endif

LOCAL void
checkextra()
{
	if (peekword() < lineend())
		pabort(E_BAD, _("Extra text '%s'"), peekword());
}

/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL void
pabort(int errnum, const char *fmt, ...)
#else
LOCAL void
pabort(errnum, fmt, va_alist)
	int	errnum;
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	err_t	*ep = errs;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	while (ep->num >= 0) {
		if (ep->num == errnum)
			break;
		ep++;
	}
	if (ep->num >= 0) {
		error("%d %s. ", ep->num, ep->name);
	}
	errmsgno(EX_BAD, _("%r on line %d in '%s'.\n"),
		fmt, args, lineno, fname);
	va_end(args);
	siglongjmp(jmp.jb, 1);
}

LOCAL void
wok()
{
	error("200 OK\n");
}

LOCAL void
pusage()
{
	error(_("Usage:\n"));
	error(_("	command	parameters		descriptionn\n"));
	error(_("	============================================\n"));
	error(_("	stop				stop processing and wait for new input.\n"));
	error(_("	cont				continue processing.\n"));
	error(_("	read sectors <sector number>	read sectors starting from sector number.\n"));
	error(_("	read tracks <track number>	read sectors starting from track number.\n"));
	error(_("	exit				exit processing.\n"));
	error(_("	quit				exit processing.\n"));
	error(_("	help				print this help and wait for new input.\n"));
}
