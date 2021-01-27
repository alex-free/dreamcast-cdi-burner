/* @(#)auinfo.c	1.33 10/12/19 Copyright 1998-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)auinfo.c	1.33 10/12/19 Copyright 1998-2010 J. Schilling";
#endif
/*
 *	Copyright (c) 1998-2010 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/stat.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/deflts.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include "cdtext.h"
#include "cdrecord.h"

extern	int	debug;
extern	int	xdebug;

LOCAL	int	auinfopen	__PR((char *name));
EXPORT	BOOL	auinfosize	__PR((char *name, track_t *trackp));
EXPORT	BOOL	auinfhidden	__PR((char *name, int track, track_t *trackp));
EXPORT	void	auinfo		__PR((char *name, int track, track_t *trackp));
EXPORT	textptr_t *gettextptr	__PR((int track, track_t *trackp));
LOCAL	char 	*savestr	__PR((char *name));
LOCAL	char 	*readtag	__PR((char *name));
LOCAL	char 	*readtstr	__PR((char *name));
EXPORT	void	setmcn		__PR((char *mcn, track_t *trackp));
LOCAL	void	isrc_illchar	__PR((char *isrc, int c));
EXPORT	void	setisrc		__PR((char *isrc, track_t *trackp));
EXPORT	void	setindex	__PR((char *tindex, track_t *trackp));

#ifdef	XXX
main(ac, av)
	int	ac;
	char	*av[];
{
/*	auinfo("/etc/default/cdrecord");*/
/*	auinfo("/mnt2/CD3/audio_01.inf");*/
	auinfo("/mnt2/CD3/audio_01.wav");
}
#endif

LOCAL int
auinfopen(name)
	char	*name;
{
	char	infname[1024];
	char	*p;

	strncpy(infname, name, sizeof (infname)-1);
	infname[sizeof (infname)-1] = '\0';
	p = strrchr(infname, '.');
	if (p != 0 && &p[4] < &name[sizeof (infname)]) {
		strcpy(&p[1], "inf");
	}

	return (defltopen(infname));
}

EXPORT BOOL
auinfosize(name, trackp)
	char	*name;
	track_t	*trackp;
{
	const	char	*p;
	const	char	*tlp;
	struct stat	sb;
	long		secs;
	long		nsamples;
	Llong		tracksize;

	if (!is_audio(trackp))
		return (FALSE);

	if ((trackp->flags & TI_USEINFO) == 0)
		return (FALSE);

	if ((p = strrchr(name, '.')) == NULL)
		return (FALSE);
	if (!streql(p, ".inf") && !streql(p, ".INF"))
		return (FALSE);

	/*
	 * First check if a bad guy tries to call auinfosize()
	 * while STDIN_FILENO is a TTY.
	 */
	if (isatty(STDIN_FILENO)) {
		errmsgno(EX_BAD,
			_("WARNING: Stdin is connected to a terminal.\n"));
		return (FALSE);
	}

	if (stat(name, &sb) < 0)	/* *.inf file not found		*/
		return (FALSE);

	if (sb.st_size > 10000)		/* Too large for a *.inf file	*/
		return (FALSE);

	if (defltopen(name) < 0)	/* Cannot open *.inf file	*/
		return (FALSE);

	tlp = p = readtag("Tracklength=");
	if (p == NULL) {		/* Tracklength= Tag not found	*/
		errmsgno(EX_BAD,
			_("WARNING: %s does not contain a 'Tracklength=' tag.\n"),
			name);
		defltclose();
		return (FALSE);
	}

	p = astol(p, &secs);
	if (*p != '\0' && *p != ',') {
		errmsgno(EX_BAD,
			_("WARNING: %s: 'Tracklength=' contains illegal parameter '%s'.\n"),
			name, tlp);
		defltclose();
		return (FALSE);
	}
	if (*p == ',')
		p++;
	p = astol(p, &nsamples);
	if (*p != '\0') {
		errmsgno(EX_BAD,
			_("WARNING: %s: 'Tracklength=' contains illegal parameter '%s'.\n"),
			name, tlp);
		defltclose();
		return (FALSE);
	}
	tracksize = (secs * 2352) + (nsamples * 4);
	if (xdebug > 0) {
		error(_("%s: Tracksize %lld bytes (%ld sectors, %ld samples)\n"),
			name, tracksize, secs, nsamples);
	}
	trackp->itracksize = tracksize;
	defltclose();
	return (TRUE);
}

EXPORT BOOL
auinfhidden(name, track, trackp)
	char	*name;
	int	track;
	track_t	*trackp;
{
	char	*p;
	long	l;
	long	tr = -1;
	long	tno = -1;
	BOOL	isdao = !is_tao(&trackp[0]);

	if (auinfopen(name) != 0)
		return (FALSE);

	p = readtag("Track=");		/* Track no, first track is 1 */
	if (p && isdao)
		astol(p, &tr);
	if (tr != 0)
		return (FALSE);
	p = readtag("Tracknumber=");	/* Track no, first track >= 1 */
	if (p && isdao)
		astol(p, &tno);

	p = readtag("Trackstart=");
	if (p && isdao) {
		l = -1L;
		astol(p, &l);
		if (track == 1 && tno == 0 && l == 0)
			return (TRUE);
	}
	return (FALSE);
}

EXPORT void
auinfo(name, track, trackp)
	char	*name;
	int	track;
	track_t	*trackp;
{
	char	*p;
	track_t	*tp = &trackp[track];
	textptr_t *txp;
	long	l;
	long	tr = -1;
	long	tno = -1;
	BOOL	isdao = !is_tao(&trackp[0]);

	if (auinfopen(name) != 0)
		return;

	p = readtstr("CDINDEX_DISCID=");
	p = readtag("CDDB_DISKID=");

	p = readtag("MCN=");
	if (p && *p) {
		setmcn(p, &trackp[0]);
		txp = gettextptr(0, trackp); /* MCN is isrc for trk 0*/
		txp->tc_isrc = savestr(p);
	}

	p = readtag("ISRC=");
	if (p && *p && track > 0) {
		setisrc(p, &trackp[track]);
		txp = gettextptr(track, trackp);
		txp->tc_isrc = savestr(p);
	}

	p = readtstr("Albumperformer=");
	if (p && *p) {
		txp = gettextptr(0, trackp); /* Album perf. in trk 0*/
		txp->tc_performer = savestr(p);
	}
	p = readtstr("Performer=");
	if (p && *p && track > 0) {
		txp = gettextptr(track, trackp);
		txp->tc_performer = savestr(p);
	}
	p = readtstr("Albumtitle=");
	if (p && *p) {
		txp = gettextptr(0, trackp); /* Album title in trk 0*/
		txp->tc_title = savestr(p);
	}
	p = readtstr("Tracktitle=");
	if (p && *p && track > 0) {
		txp = gettextptr(track, trackp);
		txp->tc_title = savestr(p);
	}
	p = readtstr("Albumsongwriter=");
	if (p && *p) {
		txp = gettextptr(0, trackp);
		txp->tc_songwriter = savestr(p);
	}
	p = readtstr("Songwriter=");
	if (p && *p && track > 0) {
		txp = gettextptr(track, trackp);
		txp->tc_songwriter = savestr(p);
	}
	p = readtstr("Albumcomposer=");
	if (p && *p) {
		txp = gettextptr(0, trackp);
		txp->tc_composer = savestr(p);
	}
	p = readtstr("Composer=");
	if (p && *p && track > 0) {
		txp = gettextptr(track, trackp);
		txp->tc_composer = savestr(p);
	}
	p = readtstr("Albumarranger=");
	if (p && *p) {
		txp = gettextptr(0, trackp);
		txp->tc_arranger = savestr(p);
	}
	p = readtstr("Arranger=");
	if (p && *p && track > 0) {
		txp = gettextptr(track, trackp);
		txp->tc_arranger = savestr(p);
	}
	p = readtstr("Albummessage=");
	if (p && *p) {
		txp = gettextptr(0, trackp);
		txp->tc_message = savestr(p);
	}
	p = readtstr("Message=");
	if (p && *p && track > 0) {
		txp = gettextptr(track, trackp);
		txp->tc_message = savestr(p);
	}
	p = readtstr("Diskid=");
	if (p && *p) {
		txp = gettextptr(0, trackp); /* Disk id is in trk 0*/
		txp->tc_diskid = savestr(p);
	}
	p = readtstr("Albumclosed_info=");
	if (p && *p) {
		txp = gettextptr(0, trackp);
		txp->tc_closed_info = savestr(p);
	}
	p = readtstr("Closed_info=");
	if (p && *p && track > 0) {
		txp = gettextptr(track, trackp);
		txp->tc_closed_info = savestr(p);
	}

	p = readtag("Track=");		/* Track no, first track is 1 */
	if (p && isdao)
		astol(p, &tr);
	p = readtag("Tracknumber=");	/* Track no, first track >= 1 */
	if (p && isdao)
		astol(p, &tno);
	if (tr < 0 && tno > 0)		/* Support old inf files	*/
		tr = tno;

	p = readtag("Trackstart=");
	if (p && isdao) {
		l = -1L;
		astol(p, &l);
		if (track == 1 && tr == 1 && l > 0) {
			trackp[1].pregapsize = 150 + l;
			printf(_("Track1 Start: '%s' (%ld)\n"), p, l);
		}
	}

	p = readtag("Tracklength=");

	p = readtag("Pre-emphasis=");
	if (p && *p) {
		if (strncmp(p, "yes", 3) == 0) {
			tp->flags |= TI_PREEMP;
			if ((tp->tracktype & TOC_MASK) == TOC_DA)
				tp->sectype = SECT_AUDIO_PRE;

		} else if (strncmp(p, "no", 2) == 0) {
			tp->flags &= ~TI_PREEMP;
			if ((tp->tracktype & TOC_MASK) == TOC_DA)
				tp->sectype = SECT_AUDIO_NOPRE;
		}
	}

	p = readtag("Channels=");
	p = readtag("Copy_permitted=");
	if (p && *p) {
		/*
		 * -useinfo always wins
		 */
		tp->flags &= ~(TI_COPY|TI_SCMS);

		if (strncmp(p, "yes", 3) == 0)
			tp->flags |= TI_COPY;
		else if (strncmp(p, "no", 2) == 0)
			tp->flags |= TI_SCMS;
		else if (strncmp(p, "once", 2) == 0)
			tp->flags &= ~(TI_COPY|TI_SCMS);
	}
	p = readtag("Endianess=");
	p = readtag("Index=");
	if (p && *p && isdao)
		setindex(p, &trackp[track]);

	p = readtag("Index0=");
	if (p && isdao) {
		Llong ts;
		Llong ps;

		l = -2L;
		astol(p, &l);
		if (l == -1) {
			trackp[track+1].pregapsize = 0;
		} else if (l > 0) {
			ts = tp->itracksize / tp->isecsize;
			ps = ts - l;
			if (ps > 0)
				trackp[track+1].pregapsize = ps;
		}
	}
}

EXPORT textptr_t *
gettextptr(track, trackp)
	int	track;
	track_t	*trackp;
{
	register textptr_t *txp;

	txp = (textptr_t *)trackp[track].text;
	if (txp == NULL) {
		txp = malloc(sizeof (textptr_t));
		if (txp == NULL)
			comerr(_("Cannot malloc CD-Text structure.\n"));
		fillbytes(txp, sizeof (textptr_t), '\0');
		trackp[track].text = txp;
	}
	return (txp);
}

LOCAL char *
savestr(str)
	char	*str;
{
	char	*ret;

	ret = malloc(strlen(str)+1);
	if (ret)
		strcpy(ret, str);
	else
		comerr(_("Cannot malloc auinfo string.\n"));
	return (ret);
}

LOCAL char *
readtag(name)
	char	*name;
{
	register char	*p;

	p = defltread(name);
	if (p) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (debug)
			printf("%s	'%s'\n", name, p);
	}
	return (p);
}

LOCAL char *
readtstr(name)
	char	*name;
{
	register char	*p;
	register char	*p2;

	p = readtag(name);
	if (p && *p == '\'') {
		p2 = ++p;
		while (*p2 != '\0')
			p2++;
		while (p2 > p && *p2 != '\'')
			p2--;
		*p2 = '\0';
		if (debug)
			printf("%s	'%s'\n", name, p);
	}
	return (p);
}

/*
 * Media catalog number is a 13 digit number.
 */
EXPORT void
setmcn(mcn, trackp)
	char	*mcn;
	track_t	*trackp;
{
	register char	*p;

	if (strlen(mcn) != 13)
		comerrno(EX_BAD, _("MCN '%s' has illegal length.\n"), mcn);

	for (p = mcn; *p; p++) {
		if (*p < '0' || *p > '9')
			comerrno(EX_BAD, _("MCN '%s' contains illegal character '%c'.\n"), mcn, *p);
	}
	p = malloc(14);
	strcpy(p, mcn);
	trackp->isrc = p;

	if (debug)
		printf(_("Track %d MCN: '%s'\n"), (int)trackp->trackno, trackp->isrc);
}

LOCAL	char	upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

LOCAL void
isrc_illchar(isrc, c)
	char	*isrc;
	int	c;
{
	errmsgno(EX_BAD, _("ISRC '%s' contains illegal character '%c'.\n"), isrc, c);
}

/*
 * ISRC is 12 Byte:
 *
 *	Country code   'C' (alpha)	  2 Bytes
 *	Owner code     'O' (alphanumeric) 3 Bytes
 *	Year of record 'Y' (numeric)	  2 Bytes
 *	Serial number  'S' (numeric)	  5 Bytes
 *
 *	CC-OOO-YY-SSSSS
 */
EXPORT void
setisrc(isrc, trackp)
	char	*isrc;
	track_t	*trackp;
{
	char	ibuf[13];
	char	*ip;
	char	*p;
	int	i;
	int	len;

	if ((len = strlen(isrc)) != 12) {
		for (p = isrc, i = 0; *p; p++) {
			if (*p == '-')
				i++;
		}
		if (((len - i) != 12) || i > 3)
			comerrno(EX_BAD, _("ISRC '%s' has illegal length.\n"), isrc);
	}

	/*
	 * The country code.
	 */
	for (p = isrc, ip = ibuf, i = 0; i < 2; p++, i++) {
		*ip++ = *p;
		if (!strchr(upper, *p)) {
/*			goto illchar;*/
			/*
			 * Flag numbers but accept them.
			 */
			isrc_illchar(isrc, *p);
			if (*p >= '0' && *p <= '9')
				continue;
			exit(EX_BAD);
		}
	}
	if (*p == '-')
		p++;

	/*
	 * The owner code.
	 */
	for (i = 0; i < 3; p++, i++) {
		*ip++ = *p;
		if (strchr(upper, *p))
			continue;
		if (*p >= '0' && *p <= '9')
			continue;
		goto illchar;
	}
	if (*p == '-')
		p++;

	/*
	 * The Year and the recording number (2 + 5 numbers).
	 */
	for (i = 0; i < 7; p++, i++) {
		*ip++ = *p;
		if (*p >= '0' && *p <= '9')
			continue;
		if (*p == '-' && i == 2) {
			ip--;
			i--;
			continue;
		}
		goto illchar;
	}
	*ip = '\0';
	p = malloc(13);
	strcpy(p, ibuf);
	trackp->isrc = p;

	if (debug)
		printf(_("Track %d ISRC: '%s'\n"), (int)trackp->trackno, trackp->isrc);
	return;
illchar:
	isrc_illchar(isrc, *p);
	exit(EX_BAD);
}

EXPORT void
setindex(tindex, trackp)
	char	*tindex;
	track_t	*trackp;
{
	char	*p;
	int	i;
	int	nindex;
	long	idx;
	long	*idxlist;

	idxlist = malloc(100*sizeof (long));
	p = tindex;
	idxlist[0] = 0;
	i = 0;
	while (*p) {
		p = astol(p, &idx);
		if (*p != '\0' && *p != ' ' && *p != '\t' && *p != ',')
			goto illchar;
		i++;
		if (i > 99)
			comerrno(EX_BAD, _("Too many indices for track %d\n"), (int)trackp->trackno);
		idxlist[i] = idx;
		if (*p == ',')
			p++;
		while (*p == ' ' || *p == '\t')
			p++;
	}
	nindex = i;

	if (debug)
		printf(_("Track %d %d Index: '%s'\n"), (int)trackp->trackno, i, tindex);

	if (debug) {
		for (i = 0; i <= nindex; i++)
			printf("%d: %ld\n", i, idxlist[i]);
	}

	trackp->nindex = nindex;
	trackp->tindex = idxlist;
	return;
illchar:
	comerrno(EX_BAD, _("Index '%s' contains illegal character '%c'.\n"), tindex, *p);
}
