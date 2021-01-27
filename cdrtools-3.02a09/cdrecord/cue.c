/* @(#)cue.c	1.57 13/12/21 Copyright 2001-2013 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cue.c	1.57 13/12/21 Copyright 2001-2013 J. Schilling";
#endif
/*
 *	Cue sheet parser
 *
 *	Copyright (c) 2001-2013 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/fcntl.h>
#include <schily/stat.h>
#include <schily/varargs.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/ctype.h>
#include <schily/errno.h>

#include "xio.h"
#include "cdtext.h"
#include "cdrecord.h"
#include "auheader.h"
#include "schily/libport.h"

/*#define	PARSE_DEBUG*/

typedef struct state {
	char	*filename;	/* Name of file to open		*/
	void	*xfp;		/* Open file			*/
	Llong	trackoff;	/* Current offset in open file	*/
	Llong	filesize;	/* Size of current open file	*/
	int	filetype;	/* File type (e.g. K_WAVE)	*/
	int	tracktype;	/* Track type (e.g. TOC_DA)	*/
	int	sectype;	/* Sector type (e.g. SECT_AUDIO)*/
	int	dbtype;		/* Data block type (e.g. DB_RAW)*/
	int	secsize;	/* Sector size from TRACK type	*/
	int	dataoff;	/* Data offset from Track type	*/
	int	state;		/* Current state of the parser	*/
	int	prevstate;	/* Previous state of the parser	*/
	int	track;		/* Relative Track index		*/
	int	trackno;	/* Absolute Track number on disk*/
	int	index;		/* Last INDEX xx number parsed	*/
	long	index0;		/* Current INDEX 00 if found	*/
	long	index1;		/* Current INDEX 01 if found	*/
	long	secoff;		/* Last INDEX 01 value in file	*/
	long	pregapsize;	/* Pregap size from PREGAP	*/
	long	postgapsize;	/* Postgap size from POSTGAP	*/
	int	flags;		/* Track flags (e.g. TI_COPY)	*/
	int	pflags;		/* Parser flags			*/
} state_t;

/*
 * Values for "state" and "prevstate".
 */
#define	STATE_NONE	0	/* Initial state of parser	*/
#define	STATE_POSTGAP	1	/* Past INDEX before FILE/TRACK	*/
#define	STATE_FILE	2	/* FILE keyword found		*/
#define	STATE_TRACK	3	/* TRACK keyword found		*/
#define	STATE_FLAGS	4	/* FLAGS past TRACK before INDEX*/
#define	STATE_INDEX0	5	/* INDEX 00 found		*/
#define	STATE_INDEX1	6	/* INDEX 01 found		*/
#define	STATE_MAX	6	/* # of entries in states[]	*/

/*
 * Flag bits used in "pflags".
 */
#define	PF_CDRTOOLS_EXT	0x01	/* Cdrtools extensions allowed	*/
#define	PF_INDEX0_PREV	0x02	/* INDEX 00 belongs to prev FILE*/
#define	PF_FILE_FOUND	0x04	/* FILE command found for TRACK	*/

LOCAL char *states[] = {
	"NONE",
	"POSTGAP",
	"FILE",
	"TRACK",
	"FLAGS",
	"INDEX 00",
	"INDEX 01"
};

typedef struct keyw {
	char	*k_name;
	int	k_type;
} keyw_t;

/*
 *	Keywords (first word on line):
 *		CATALOG		- global	CATALOG		<MCN>
 *		CDTEXTFILE	- global	CDTEXTFILE	<fname>
 *		FILE		- track local	FILE		<fame> <type>
 *		FLAGS		- track local	FLAGS		<flag> ...
 *		INDEX		- track local	INDEX		<#> <mm:ss:ff>
 *		ISRC		- track local	ISRC		<ISRC>
 *		PERFORMER	- global/local	PERFORMER	<string>
 *		POSTGAP		- track locak	POSTGAP		<mm:ss:ff>
 *		PREGAP		- track local	PREGAP		<mm:ss:ff>
 *		REM		- anywhere	REM		<comment>
 *		SONGWRITER	- global/local	SONGWRITER	<string>
 *		TITLE		- global/local	TITLE		<string>
 *		TRACK		- track local	TRACK		<#> <datatype>
 *
 *	Order of keywords:
 *		CATALOG
 *		CDTEXTFILE
 *		PERFORMER | SONGWRITER | TITLE		Doc says past FILE...
 *		FILE					Must be past CATALOG
 *		------- Repeat the following:		mehrere FILE Commands?
 *		TRACK
 *		FLAGS | ISRC | PERFORMER | PREGAP | SONGWRITER | TITLE
 *		INDEX
 *		POSTGAP
 *
 *	Additional keyword rules:
 *		CATALOG		once
 *		CDTEXTFILE	once
 *		FILE		before "other command"
 *		FLAGS		one per TRACK, after TRACK before INDEX
 *		INDEX		>= 0, <= 99, first 0 or 1, sequential,
 *				first index of a FILE at 00:00:00
 *		ISRC		after TRACK before INDEX
 *		PERFORMER
 *		POSTGAP		one per TRACK, after all INDEX for current TRACK
 *		PREGAP		one per TRACK, after TRACK before INDEX
 *		REM
 *		SONGWRITER
 *		TITLE
 *		TRACK		>= 1, <= 99, sequential, >= 1 TRACK per FILE
 */

#define	K_G		0x10000		/* Global			*/
#define	K_T		0x20000		/* Track local			*/
#define	K_A		(K_T | K_G)	/* Global & Track local		*/

#define	K_ARRANGER	(0 | K_A)	/* CD-Text Arranger		*/
#define	K_MCN		(1 | K_G)	/* Media catalog number		*/
#define	K_TEXTFILE	(2 | K_G)	/* CD-Text binary file		*/
#define	K_COMPOSER	(3 | K_A)	/* CD-Text Composer		*/
#define	K_FILE		(4 | K_T)	/* Input data file		*/
#define	K_FLAGS		(5 | K_T)	/* Flags for ctrl nibble	*/
#define	K_INDEX		(6 | K_T)	/* Index marker for track	*/
#define	K_ISRC		(7 | K_T)	/* ISRC string for track	*/
#define	K_MESSAGE	(8 | K_A)	/* CD-Text Message		*/
#define	K_PERFORMER	(9 | K_A)	/* CD-Text Performer		*/
#define	K_POSTGAP	(10 | K_T)	/* Post gap for track (autogen)	*/
#define	K_PREGAP	(11 | K_T)	/* Pre gap for track (autogen)	*/
#define	K_REM		(12 | K_A)	/* Remark (Comment)		*/
#define	K_SONGWRITER	(13| K_A)	/* CD-Text Songwriter		*/
#define	K_TITLE		(14| K_A)	/* CD-Text Title		*/
#define	K_TRACK		(15| K_T)	/* Track marker			*/


LOCAL keyw_t	keywords[] = {
	{ "ARRANGER",	K_ARRANGER },	/* Not supported by CDR-WIN	*/
	{ "CATALOG",	K_MCN },
	{ "CDTEXTFILE",	K_TEXTFILE },
	{ "COMPOSER",	K_COMPOSER },	/* Not supported by CDR-WIN	*/
	{ "FILE",	K_FILE },
	{ "FLAGS",	K_FLAGS },
	{ "INDEX",	K_INDEX },
	{ "ISRC",	K_ISRC },
	{ "MESSAGE",	K_MESSAGE },	/* Not supported by CDR-WIN	*/
	{ "PERFORMER",	K_PERFORMER },
	{ "POSTGAP",	K_POSTGAP },
	{ "PREGAP",	K_PREGAP },
	{ "REM",	K_REM },
	{ "SONGWRITER",	K_SONGWRITER },
	{ "TITLE",	K_TITLE },
	{ "TRACK",	K_TRACK },
	{ NULL,		0 },
};


/*
 *	Filetypes - argument to FILE Keyword (one only):
 *
 *		BINARY		- Intel binary file (least significant byte first)
 *		MOTOTOLA	- Motorola binary file (most significant byte first)
 *		AIFF		- Audio AIFF file
 *		WAVE		- Audio WAVE file
 *		MP3		- Audio MP3 file
 *		AU		- Sun Audio file
 *		OGG		- Audio OGG file
 */
#define	K_BINARY	100
#define	K_MOTOROLA	101
#define	K_AIFF		102
#define	K_WAVE		103
#define	K_MP3		104
#define	K_FT_CDRWIN_MAX	104
#define	K_AU		105
#define	K_OGG		106

LOCAL keyw_t	filetypes[] = {
	{ "BINARY",	K_BINARY },
	{ "MOTOROLA",	K_MOTOROLA },
	{ "AIFF",	K_AIFF },
	{ "WAVE",	K_WAVE },
	{ "MP3",	K_MP3 },
	{ "AU",		K_AU },		/* Not supported by CDR-WIN	*/
	{ "OGG",	K_OGG },	/* Not supported by CDR-WIN	*/
	{ NULL,		0 },
};

/*
 *	Flags - argument to FLAGS Keyword (more than one allowed):
 *		DCP		- Digital copy permitted
 *		4CH		- Four channel audio
 *		PRE		- Pre-emphasis enabled (audio tracks only)
 *		SCMS		- Serial copy management system (not supported by all recorders)
 */
#define	K_DCP		1000
#define	K_4CH		1001
#define	K_PRE		1002
#define	K_SCMS		1003
#define	K_FL_CDRWIN_MAX	1003

LOCAL keyw_t	flags[] = {
	{ "DCP",	K_DCP },
	{ "4CH",	K_4CH },
	{ "PRE",	K_PRE },
	{ "SCMS",	K_SCMS },
	{ NULL,		0 },
};

/*
 *	Datatypes - argument to TRACK Keyword (one only):
 *		AUDIO		- Audio/Music (2352)
 *		CDG		- Karaoke CD+G (2448)
 *		MODE1/2048	- CDROM Mode1 Data (cooked)
 *		MODE1/2352	- CDROM Mode1 Data (raw)
 *		MODE2/2336	- CDROM-XA Mode2 Data
 *		MODE2/2352	- CDROM-XA Mode2 Data
 *		CDI/2336	- CDI Mode2 Data
 *		CDI/2352	- CDI Mode2 Data
 */
#define	K_AUDIO		10000
#define	K_CDG		10001
#define	K_MODE1		10002
#define	K_MODE2		10003
#define	K_CDI		10004
#define	K_DT_CDRWIN_MAX	10004

LOCAL keyw_t	dtypes[] = {
	{ "AUDIO",	K_AUDIO },
	{ "CDG",	K_CDG },
	{ "MODE1",	K_MODE1 },
	{ "MODE2",	K_MODE2 },
	{ "CDI",	K_CDI },
	{ NULL,		0 },
};


#ifdef	CUE_MAIN
EXPORT	int	main		__PR((int ac, char **av));
#endif
EXPORT	int	parsecue	__PR((char *cuefname, track_t trackp[]));
EXPORT	void	fparsecue	__PR((FILE *f, track_t trackp[]));
LOCAL	void	parse_arranger	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_mcn	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_textfile	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_composer	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_file	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_flags	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_index	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_isrc	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_message	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_performer	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_postgap	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_pregap	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_rem	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_songwriter __PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_title	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_track	__PR((track_t trackp[], state_t *sp));
LOCAL	void	parse_offset	__PR((long *lp));
LOCAL	void	newtrack	__PR((track_t trackp[], state_t *sp));

LOCAL	keyw_t	*lookup		__PR((char *word, keyw_t table[]));
LOCAL	char	*state_name	__PR((int st));
#ifdef	DEBUG
LOCAL	void	wdebug		__PR((void));
#endif
LOCAL	FILE	*cueopen	__PR((char *name));
LOCAL	char	*cuename	__PR((void));
LOCAL	char	*nextline	__PR((FILE *f));
#ifdef	__needed__
LOCAL	void	ungetline	__PR((void));
#endif
LOCAL	char	*skipwhite	__PR((const char *s));
LOCAL	char	*peekword	__PR((void));
LOCAL	char	*lineend	__PR((void));
LOCAL	char	*markword	__PR((char *delim));
LOCAL	char	getworddelim	__PR((void));
LOCAL	char	*getnextitem	__PR((char *delim));
LOCAL	char	*neednextitem	__PR((char *delim, char *type));
#ifdef	__needed__
LOCAL	char	*nextword	__PR((void));
#endif
LOCAL	char	*needword	__PR((char *type));
LOCAL	char	*curword	__PR((void));
LOCAL	char	*nextitem	__PR((void));
LOCAL	char	*needitem	__PR((char *type));
LOCAL	void	checkextra	__PR((void));
LOCAL	void	statewarn	__PR((state_t *sp, const char *fmt, ...));
#ifdef	__needed__
LOCAL	void	cuewarn		__PR((const char *fmt, ...));
#endif
LOCAL	void	cueabort	__PR((const char *fmt, ...));
LOCAL	void	extabort	__PR((const char *fmt, ...));

#ifdef	CUE_MAIN
int	debug;
int	xdebug = 1;

int write_secs	__PR((void));
int write_secs() { return (-1); }

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	i;
	track_t	track[MAX_TRACK+2];	/* Max tracks + track 0 + track AA */

	save_args(ac, av);

	fillbytes(track, sizeof (track), '\0');
	for (i = 0; i < MAX_TRACK+2; i++)
		track[i].track = track[i].trackno = i;
	track[0].tracktype = TOC_MASK;


	parsecue(av[1], track);
	return (0);
}
#else
extern	int	xdebug;
#endif

EXPORT int
parsecue(cuefname, trackp)
	char	*cuefname;
	track_t	trackp[];
{
	FILE	*f = cueopen(cuefname);

	fparsecue(f, trackp);
	return (0);
}

EXPORT void
fparsecue(f, trackp)
	FILE	*f;
	track_t	trackp[];
{
	char	*word;
	struct keyw *kp;
	BOOL	isglobal = TRUE;
	state_t	state;

	state.filename	= NULL;
	state.xfp	= NULL;
	state.trackoff	= 0;
	state.filesize	= 0;
	state.filetype	= 0;
	state.tracktype	= 0;
	state.sectype	= 0;
	state.dbtype	= 0;
	state.secsize	= 0;
	state.dataoff	= 0;
	state.state	= STATE_NONE;
	state.prevstate	= STATE_NONE;
	state.track	= 0;
	state.trackno	= 0;
	state.index	= -1;
	state.index0	= -1;
	state.index1	= -1;
	state.secoff	= 0;
	state.pregapsize = -1;
	state.postgapsize = -1;
	state.flags	= 0;
	state.pflags	= 0;

	if (xdebug > 1)
		printf(_("---> Entering CUE Parser...\n"));
	do {
		if (nextline(f) == NULL) {
			/*
			 * EOF on CUE File
			 * Do post processing here
			 */
			if (state.state < STATE_INDEX1 && state.state != STATE_POSTGAP) {
				statewarn(&state, _("INDEX 01 missing"));
				cueabort(_("Incomplete CUE file"));
			}
			if (state.xfp) {
				xclose(state.xfp);
				state.xfp = NULL;
			}
			if (xdebug > 1) {
				printf(_("---> CUE Parser got EOF, found %2.2d tracks.\n"),
								state.track);
			}
			return;
		}
		word = nextitem();
		if (*word == '\0')	/* empty line */
			continue;

		if (xdebug > 1)
			printf(_("\nKEY: '%s'     %s\n"), word, peekword());
		kp = lookup(word, keywords);
		if (kp == NULL)
			cueabort(_("Unknown CUE keyword '%s'"), word);

		if ((kp->k_type & K_G) == 0) {
			if (isglobal)
				isglobal = FALSE;
		}
		if ((kp->k_type & K_T) == 0) {
			if (!isglobal) {
				statewarn(&state,
					_("%s keyword must be before first TRACK"),
					word);
				cueabort(_("Badly placed CUE keyword '%s'"), word);
			}
		}
#ifdef	DEBUG
		printf("%s-", isglobal ? "G" : "T");
		wdebug();
#endif

		switch (kp->k_type) {

		case K_ARRANGER:   parse_arranger(trackp, &state);	break;
		case K_MCN:	   parse_mcn(trackp, &state);		break;
		case K_TEXTFILE:   parse_textfile(trackp, &state);	break;
		case K_COMPOSER:   parse_composer(trackp, &state);	break;
		case K_FILE:	   parse_file(trackp, &state);		break;
		case K_FLAGS:	   parse_flags(trackp, &state);		break;
		case K_INDEX:	   parse_index(trackp, &state);		break;
		case K_ISRC:	   parse_isrc(trackp, &state);		break;
		case K_MESSAGE:	   parse_message(trackp, &state);	break;
		case K_PERFORMER:  parse_performer(trackp, &state);	break;
		case K_POSTGAP:	   parse_postgap(trackp, &state);	break;
		case K_PREGAP:	   parse_pregap(trackp, &state);	break;
		case K_REM:	   parse_rem(trackp, &state);		break;
		case K_SONGWRITER: parse_songwriter(trackp, &state);	break;
		case K_TITLE:	   parse_title(trackp, &state);		break;
		case K_TRACK:	   parse_track(trackp, &state);		break;

		default:
			cueabort(_("Panic: unknown CUE command '%s'"), word);
		}
	} while (1);
}

LOCAL void
parse_arranger(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*word;
	textptr_t *txp;

	if ((sp->pflags & PF_CDRTOOLS_EXT) == 0)
		extabort("ARRANGER");

	if (sp->track > 0 && sp->state > STATE_INDEX0) {
		statewarn(sp, _("ARRANGER keyword cannot be after INDEX keyword"));
		cueabort(_("Badly placed ARRANGER keyword"));
	}

	word = needitem("arranger");
	txp = gettextptr(sp->track, trackp);
	txp->tc_arranger = strdup(word);

	checkextra();
}

LOCAL void
parse_mcn(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*word;
	textptr_t *txp;

	if (sp->track != 0)
		cueabort(_("CATALOG keyword must be before first TRACK"));

	word = needitem("MCN");
	setmcn(word, &trackp[0]);
	txp = gettextptr(0, trackp); /* MCN is isrc for trk 0 */
	txp->tc_isrc = strdup(word);

	checkextra();
}

LOCAL void
parse_textfile(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*word;

	if (sp->track != 0)
		cueabort(_("CDTEXTFILE keyword must be before first TRACK"));

	word = needitem("cdtextfile");

	if (trackp[MAX_TRACK+1].flags & TI_TEXT) {
		if (!checktextfile(word)) {
			comerrno(EX_BAD,
				_("Cannot use '%s' as CD-Text file.\n"),
				word);
		}
		trackp[0].flags |= TI_TEXT;
	} else {
		errmsgno(EX_BAD, _("Ignoring CDTEXTFILE '%s'.\n"), word);
		errmsgno(EX_BAD, _("If you like to write CD-Text, call cdrecord -text.\n"));
	}

	checkextra();
}

LOCAL void
parse_composer(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*word;
	textptr_t *txp;

	if ((sp->pflags & PF_CDRTOOLS_EXT) == 0)
		extabort("COMPOSER");

	if (sp->track > 0 && sp->state > STATE_INDEX0) {
		statewarn(sp, _("COMPOSER keyword cannot be after INDEX keyword"));
		cueabort(_("Badly placed COMPOSER keyword"));
	}

	word = needitem("composer");
	txp = gettextptr(sp->track, trackp);
	txp->tc_composer = strdup(word);

	checkextra();
}

LOCAL void
parse_file(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	cname[1024];
	char	newname[1024];
	struct keyw *kp;
	char	*word;
	char	*filetype;
	struct stat	st;
#ifdef	hint
	Llong		lsize;
#endif

	if ((sp->state <= STATE_TRACK && sp->state > STATE_POSTGAP) ||
	    (sp->state >= STATE_TRACK && sp->state < STATE_INDEX1)) {
		if (sp->state >= STATE_INDEX0 && sp->state < STATE_INDEX1) {
			if ((sp->pflags & PF_CDRTOOLS_EXT) == 0)
				extabort(_("FILE keyword after INDEX 00 and before INDEX 01"));
			sp->prevstate = sp->state;
			goto file_ok;
		}
		if (sp->state <= STATE_TRACK && sp->state > STATE_POSTGAP)
			statewarn(sp, _("FILE keyword only allowed once before TRACK keyword"));
		if (sp->state >= STATE_TRACK && sp->state < STATE_INDEX1)
			statewarn(sp, _("FILE keyword not allowed after TRACK and before INDEX 01"));
		cueabort(_("Badly placed FILE keyword"));
	}
file_ok:
	if (sp->state < STATE_INDEX1 && sp->pflags & PF_FILE_FOUND)
		cueabort(_("Only one FILE keyword allowed per TRACK"));

	sp->pflags |= PF_FILE_FOUND;
	sp->state = STATE_FILE;

	word = needitem("filename");
	if (sp->xfp) {
		xclose(sp->xfp);
		sp->xfp = NULL;
	}
	sp->xfp = xopen(word, O_RDONLY|O_BINARY, 0, X_NOREWIND);
	if (sp->xfp == NULL && geterrno() == ENOENT) {
		char	*p;

		if (strchr(word, '/') == 0 &&
		    strchr(cuename(), '/') != 0) {
			js_snprintf(cname, sizeof (cname),
				"%s", cuename());
			p = strrchr(cname, '/');
			if (p)
				*p = '\0';
			js_snprintf(newname, sizeof (newname),
				"%s/%s", cname, word);
			word = newname;
			sp->xfp = xopen(word, O_RDONLY|O_BINARY, 0, X_NOREWIND);
		}
	}
	if (sp->xfp == NULL) {
#ifdef	PARSE_DEBUG
		errmsg(_("Cannot open FILE '%s'.\n"), word);
#else
		comerr(_("Cannot open FILE '%s'.\n"), word);
#endif
	}

	sp->filename	 = strdup(word);
	sp->trackoff	 = 0;
	sp->secoff	 = 0;
	sp->filesize	 = 0;
	sp->flags	&= ~TI_SWAB;	/* Reset what we might set for FILE */

	filetype = needitem("filetype");
	kp = lookup(filetype, filetypes);
	if (kp == NULL)
		cueabort(_("Unknown filetype '%s'"), filetype);

	if ((sp->pflags & PF_CDRTOOLS_EXT) == 0 &&
	    kp->k_type > K_FT_CDRWIN_MAX)
		extabort(_("Filetype '%s'"), kp->k_name);

	switch (kp->k_type) {

	case K_BINARY:
	case K_MOTOROLA:
			if (fstat(xfileno(sp->xfp), &st) >= 0 &&
			    S_ISREG(st.st_mode)) {
				sp->filesize = st.st_size;
				if (kp->k_type == K_BINARY)
					sp->flags |= TI_SWAB;
			} else {
				cueabort(_("Unknown file size for FILE '%s'"),
								sp->filename);
			}
			break;
	case K_AIFF:
			cueabort(_("Unsupported filetype '%s'"), kp->k_name);
			break;
	case K_AU:
			sp->filesize = ausize(xfileno(sp->xfp));
			break;
	case K_WAVE:
#ifdef	PARSE_DEBUG
			sp->filesize = 1000000000;
#else
			sp->filesize = wavsize(xfileno(sp->xfp));
#endif
			sp->flags |= TI_SWAB;
			break;
	case K_MP3:
	case K_OGG:
			cueabort(_("Unsupported filetype '%s'"), kp->k_name);
			break;

	default:	cueabort(_("Panic: unknown filetype '%s'"), filetype);
	}

	if (sp->filesize == AU_BAD_CODING) {
		cueabort(_("Inappropriate audio coding in '%s'"),
							sp->filename);
	}
	if (xdebug > 0)
		printf(_("Track[%2.2d] %2.2d File '%s' Filesize %lld\n"),
			sp->track, sp->trackno, sp->filename, sp->filesize);

	sp->filetype = kp->k_type;

	checkextra();


#ifdef	hint
		trackp->itracksize = lsize;
		if (trackp->itracksize != lsize)
			comerrno(EX_BAD, _("This OS cannot handle large audio images.\n"));
#endif
}

LOCAL void
parse_flags(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	struct keyw *kp;
	char	*word;

	if ((sp->state < STATE_TRACK) ||
	    (sp->state >= STATE_INDEX0)) {
		statewarn(sp, _("FLAGS keyword must be after TRACK and before INDEX keyword"));
		cueabort(_("Badly placed FLAGS keyword"));
	}
	sp->state = STATE_FLAGS;

	do {
		word = needitem("flag");
		kp = lookup(word, flags);
		if (kp == NULL)
			cueabort(_("Unknown flag '%s'"), word);

		switch (kp->k_type) {

		case K_DCP:	sp->flags |= TI_COPY;	break;
		case K_4CH:	sp->flags |= TI_QUADRO;	break;
		case K_PRE:	sp->flags |= TI_PREEMP;	break;
		case K_SCMS:	sp->flags |= TI_SCMS;	break;
		default:	cueabort(_("Panic: unknown FLAG '%s'"), word);
		}

	} while (peekword() < lineend());

	if (xdebug > 0)
		printf(_("Track[%2.2d] %2.2d flags 0x%08X\n"), sp->track, sp->trackno, sp->flags);
}

LOCAL void
parse_index(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*word;
	long	l;
	int	track = sp->track;

	if (sp->state < STATE_TRACK) {
		if (sp->state == STATE_FILE &&
		    sp->prevstate >= STATE_TRACK &&
		    sp->prevstate <= STATE_INDEX1) {
			if ((sp->pflags & PF_CDRTOOLS_EXT) == 0)
				extabort(_("INDEX keyword after FILE keyword"));
			goto index_ok;
		}
		statewarn(sp, _("INDEX keyword must be after TRACK keyword"));
		cueabort(_("Badly placed INDEX keyword"));
	}
index_ok:

	word = needitem("index");
	if (*astolb(word, &l, 10) != '\0')
		cueabort(_("Not a number '%s'"), word);
	if (l < 0 || l > 99)
		cueabort(_("Illegal index '%s'"), word);

	if ((sp->index < l) &&
	    (((sp->index + 1) == l) || l == 1))
		sp->index = l;
	else
		cueabort(_("Badly placed INDEX %2.2ld number"), l);

	if (sp->state == STATE_FILE) {
		if (track == 1 || l > 1)
			cueabort(_("INDEX %2.2d not allowed after FILE"), l);
		if (l == 1)
			sp->pflags |= PF_INDEX0_PREV;
	}

	if (l > 0)
		sp->state = STATE_INDEX1;
	else
		sp->state = STATE_INDEX0;
	sp->prevstate = sp->state;

	parse_offset(&l);

	if (xdebug > 1)
		printf(_("Track[%2.2d] %2.2d Index %2.2d %ld\n"), sp->track, sp->trackno, sp->index, l);

	if (track == 1 ||
	    !streql(sp->filename, trackp[track-1].filename)) {
		/*
		 * Check for offset 0 when a new file begins.
		 */
		if (sp->index == 0 && l > 0)
			cueabort(_("Bad INDEX 00 offset in CUE file (must be 00:00:00 for new FILE)"));
		if (sp->index == 1 && sp->index0 < 0 && l > 0)
			cueabort(_("Bad INDEX 01 offset in CUE file (must be 00:00:00 for new FILE)"));
	}

	if (sp->index == 0) {
		sp->index0 = l;
	} else if (sp->index == 1) {
		sp->index1 = l;
		trackp[track].nindex = 1;
		newtrack(trackp, sp);

		if (xdebug > 1) {
			printf(_("Track[%2.2d] %2.2d pregapsize %ld\n"),
				sp->track, sp->trackno, trackp[track].pregapsize);
		}
	} else if (sp->index == 2) {
		trackp[track].tindex = malloc(100*sizeof (long));
		trackp[track].tindex[1] = 0;
		trackp[track].tindex[2] = l - sp->index1;
		trackp[track].nindex = 2;
	} else if (sp->index > 2) {
		trackp[track].tindex[sp->index] = l - sp->index1;
		trackp[track].nindex = sp->index;
	}

	checkextra();
}

LOCAL void
parse_isrc(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*word;
	textptr_t *txp;
	int	track = sp->track;

	if (track == 0)
		cueabort(_("ISRC keyword must be past first TRACK"));

	if ((sp->state < STATE_TRACK) ||
	    (sp->state >= STATE_INDEX0)) {
		statewarn(sp, _("ISRC keyword must be after TRACK and before INDEX keyword"));
		cueabort(_("Badly placed ISRC keyword"));
	}
	sp->state = STATE_FLAGS;

	word = needitem("ISRC");
	if ((sp->pflags & PF_CDRTOOLS_EXT) == 0 &&
	    strchr(word, '-')) {
		extabort(_("'-' in ISRC arg"));
	}
	setisrc(word, &trackp[track]);
	txp = gettextptr(track, trackp);
	txp->tc_isrc = strdup(word);

	checkextra();
}

LOCAL void
parse_message(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*word;
	textptr_t *txp;

	if ((sp->pflags & PF_CDRTOOLS_EXT) == 0)
		extabort("MESSAGE");

	if (sp->track > 0 && sp->state > STATE_INDEX0) {
		statewarn(sp, _("MESSAGE keyword cannot be after INDEX keyword"));
		cueabort(_("Badly placed MESSAGE keyword"));
	}

	word = needitem("message");
	txp = gettextptr(sp->track, trackp);
	txp->tc_message = strdup(word);

	checkextra();
}

LOCAL void
parse_performer(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*word;
	textptr_t *txp;

	if (sp->track > 0 && sp->state > STATE_INDEX0) {
		statewarn(sp, _("PERFORMER keyword cannot be after INDEX keyword"));
		cueabort(_("Badly placed PERFORMER keyword"));
	}

	word = needitem("performer");
	txp = gettextptr(sp->track, trackp);
	txp->tc_performer = strdup(word);

	checkextra();
}

LOCAL void
parse_postgap(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	long	l;

	if (sp->state < STATE_INDEX1) {
		statewarn(sp, _("POSTGAP keyword must be after INDEX 01"));
		cueabort(_("Badly placed POSTGAP keyword"));
	}
	sp->state = STATE_POSTGAP;

	parse_offset(&l);
	sp->postgapsize = l;
	trackp[sp->track].padsecs = l;
	/*
	 * Add to size of track.
	 * In non-CUE mode, this is done in opentracks().
	 */
	if (l > 0)
		trackp[sp->track].tracksecs += l;

	checkextra();
}

LOCAL void
parse_pregap(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	long	l;

	if ((sp->state < STATE_TRACK) ||
	    (sp->state >= STATE_INDEX0)) {
		statewarn(sp, _("PREGAP keyword must be after TRACK and before INDEX keyword"));
		cueabort(_("Badly placed PREGAP keyword"));
	}
	sp->state = STATE_FLAGS;

	parse_offset(&l);
	sp->pregapsize = l;

	checkextra();
}

LOCAL void
parse_rem(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*oword = curword();
	char	*word;

	word = nextitem();
	if ((oword == word) || (*word == '\0'))
		return;
	if ((sp->pflags & PF_CDRTOOLS_EXT) == 0 &&
	    streql(word, "CDRTOOLS")) {
		sp->pflags |= PF_CDRTOOLS_EXT;
		errmsgno(EX_BAD,
		_("Warning: Enabling cdrecord specific CUE extensions.\n"));
	}
	if ((sp->pflags & PF_CDRTOOLS_EXT) == 0 &&
	    streql(word, "COMMENT")) {
		oword = word;
		word = nextitem();
		if ((oword == word) || (*word == '\0'))
			return;
		if (strncmp(word, "ExactAudioCopy ", 15) == 0) {
			sp->pflags |= PF_CDRTOOLS_EXT;
			errmsgno(EX_BAD,
			_("Warning: Found ExactAudioCopy, enabling CUE extensions.\n"));
		}
	}
}

LOCAL void
parse_songwriter(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*word;
	textptr_t *txp;

	if (sp->track > 0 && sp->state > STATE_INDEX0) {
		statewarn(sp, _("SONGWRITER keyword cannot be after INDEX keyword"));
		cueabort(_("Badly placed SONGWRITER keyword"));
	}
	word = needitem("songwriter");
	txp = gettextptr(sp->track, trackp);
	txp->tc_songwriter = strdup(word);

	checkextra();
}

LOCAL void
parse_title(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	char	*word;
	textptr_t *txp;

	if (sp->track > 0 && sp->state > STATE_INDEX0) {
		statewarn(sp, _("TITLE keyword cannot be after INDEX keyword"));
		cueabort(_("Badly placed TITLE keyword"));
	}
	word = needitem("title");
	txp = gettextptr(sp->track, trackp);
	txp->tc_title = strdup(word);

	checkextra();
}

LOCAL void
parse_track(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	struct keyw *kp;
	char	*word;
	long	l;
	long	secsize = -1;

	if ((sp->state >= STATE_TRACK) &&
	    (sp->state < STATE_INDEX1)) {
		statewarn(sp, _("TRACK keyword must be after INDEX 01"));
		cueabort(_("Badly placed TRACK keyword"));
	}
	sp->pflags &= ~(PF_INDEX0_PREV|PF_FILE_FOUND);
	if (sp->state == STATE_FILE)
		sp->pflags |= PF_FILE_FOUND;
	sp->state = STATE_TRACK;
	sp->prevstate = STATE_TRACK;
	sp->track++;
	sp->index0 = -1;
	sp->index = -1;
	sp->pregapsize = -1;
	sp->postgapsize = -1;

	word = needitem("track number");
	if (*astolb(word, &l, 10) != '\0')
		cueabort(_("Not a number '%s'"), word);
	if (l <= 0 || l > 99)
		cueabort(_("Illegal TRACK number '%s'"), word);

	if ((sp->trackno < l) &&
	    (((sp->trackno + 1) == l) || sp->trackno == 0))
		sp->trackno = l;
	else
		cueabort(_("Badly placed TRACK %ld number"), l);

	word = needword("data type");
	kp = lookup(word, dtypes);
	if (kp == NULL)
		cueabort(_("Unknown data type '%s'"), word);

	if (getworddelim() == '/') {
		word = needitem("sector size");
		if (*astol(++word, &secsize) != '\0')
			cueabort(_("Not a number '%s'"), word);
	}

	/*
	 * Reset all flags that may be set in TRACK & FLAGS lines
	 */
	sp->flags &= ~(TI_AUDIO|TI_COPY|TI_QUADRO|TI_PREEMP|TI_SCMS);

	if (kp->k_type == K_AUDIO)
		sp->flags |= TI_AUDIO;

	switch (kp->k_type) {

	case K_CDG:
		if (secsize < 0)
			secsize = 2448;
	case K_AUDIO:
		if (secsize < 0)
			secsize = 2352;

		sp->tracktype = TOC_DA;
		sp->sectype = SECT_AUDIO;
		sp->dbtype = DB_RAW;
		sp->secsize = secsize;
		sp->dataoff = 0;
		if (secsize != 2352)
			cueabort(_("Unsupported sector size %ld for audio"), secsize);
		break;

	case K_MODE1:
		if (secsize < 0)
			secsize = 2048;

		sp->tracktype = TOC_ROM;
		sp->sectype = SECT_ROM;
		sp->dbtype = DB_ROM_MODE1;
		sp->secsize = secsize;
		sp->dataoff = 16;
		/*
		 * XXX Sector Size == 2352 ???
		 * XXX It seems that there exist bin/cue pairs with this value
		 */
		if (secsize != 2048)
			cueabort(_("Unsupported sector size %ld for data"), secsize);
		break;

	case K_MODE2:
	case K_CDI:
		sp->tracktype = TOC_ROM;
		sp->sectype = SECT_MODE_2;
		sp->dbtype = DB_ROM_MODE2;
		sp->secsize = secsize;
		sp->dataoff = 16;
		if (secsize == 2352) {
			sp->tracktype = TOC_XA2;
			sp->sectype = SECT_MODE_2_MIX;
			sp->sectype |= ST_MODE_RAW;
			sp->dbtype = DB_RAW;
			sp->dataoff = 0;
		} else if (secsize != 2336)
			cueabort(_("Unsupported sector size %ld for mode2"), secsize);
		if (kp->k_type == K_CDI)
			sp->tracktype = TOC_CDI;
		break;

	default:	cueabort(_("Panic: unknown datatype '%s'"), word);
	}

	if (sp->flags & TI_PREEMP)
		sp->sectype |= ST_PREEMPMASK;
	sp->secsize = secsize;

	if (xdebug > 1) {
		printf(_("Track[%2.2d] %2.2d Tracktype %s/%d\n"),
			sp->track, sp->trackno, kp->k_name, sp->secsize);
	}

	checkextra();
}

LOCAL void
parse_offset(lp)
	long	*lp;
{
	char	*word;
	char	*p;
	long	m = -1;
	long	s = -1;
	long	f = -1;

	word = needitem("time offset/length");

	if (strchr(word, ':') == NULL) {
		if (*astol(word, lp) != '\0')
			cueabort(_("Not a number '%s'"), word);
		return;
	}
	if (*(p = astolb(word, &m, 10)) != ':')
		cueabort(_("Not a number '%s'"), word);
	if (m < 0 || m >= 160)
		cueabort(_("Illegal minute value in '%s'"), word);
	p++;
	if (*(p = astolb(p, &s, 10)) != ':')
		cueabort(_("Not a number '%s'"), p);
	if (s < 0 || s >= 60)
		cueabort(_("Illegal second value in '%s'"), word);
	p++;
	if (*(p = astolb(p, &f, 10)) != '\0')
		cueabort(_("Not a number '%s'"), p);
	if (f < 0 || f >= 75)
		cueabort(_("Illegal frame value in '%s'"), word);

	m = m * 60 + s;
	m = m * 75 + f;
	*lp = m;
}

/*--------------------------------------------------------------------------*/
LOCAL void
newtrack(trackp, sp)
	track_t	trackp[];
	state_t	*sp;
{
	register int	i;
	register int	track = sp->track;
		Llong	tracksize;

	if (xdebug > 1)
		printf(_("-->Newtrack %2.2d Trackno %2.2d\n"), track, sp->trackno);
	if (track > 1 && streql(sp->filename, trackp[track-1].filename)) {
		tracksize = (sp->index1 - sp->secoff) * trackp[track-1].isecsize;

		if (xdebug > 1)
			printf("    trackoff %lld filesize %lld index1 %ld size %ld/%lld secsize/isecsize %d/%d\n",
				sp->trackoff, sp->filesize, sp->index1,
				sp->index1 - sp->secoff,
				tracksize,
				trackp[track-1].secsize,
				trackp[track-1].isecsize);

		trackp[track-1].itracksize = tracksize;
		trackp[track-1].tracksize = tracksize;
		if (trackp[track-1].secsize != trackp[track-1].isecsize) {
			/*
			 * In RAW mode, we need to recompute the track size.
			 */
			trackp[track-1].tracksize =
						(trackp[track-1].itracksize /
						trackp[track-1].isecsize) *
						trackp[track-1].secsize
						+ trackp[track-1].itracksize %
						trackp[track-1].isecsize;
		}
		trackp[track-1].tracksecs = sp->index1 - sp->secoff;
		/*
		 * Add to size of track.
		 * In non-CUE mode, this is done in opentracks().
		 */
		if (trackp[track-1].padsecs > 0)
			trackp[track-1].tracksecs += trackp[track-1].padsecs;

		sp->trackoff += tracksize;
		sp->secoff = sp->index1;
	}
	/*
	 * Make 'tracks' immediately usable in track structure.
	 */
	for (i = 0; i < MAX_TRACK+2; i++)
		trackp[i].tracks = track;

	trackp[track].filename = sp->filename;
	trackp[track].xfp = xopen(sp->filename, O_RDONLY|O_BINARY, 0, X_NOREWIND);
	trackp[track].trackstart = 0L;
/*
 * SEtzen wenn tracksecs bekannt sind
 * d.h. mit Index0 oder Index 1 vom nächsten track
 *
 *	trackp[track].itracksize = tracksize;
 *	trackp[track].tracksize = tracksize;
 *	trackp[track].tracksecs = -1L;
 */
	tracksize = sp->filesize - sp->trackoff;

	trackp[track].itracksize = tracksize;
	trackp[track].tracksize = tracksize;
	trackp[track].tracksecs = (tracksize + sp->secsize - 1) / sp->secsize;

	if (xdebug > 1)
		printf(_("    Remaining Filesize %lld (%lld secs)\n"),
			(sp->filesize-sp->trackoff),
			(sp->filesize-sp->trackoff +sp->secsize - 1) / sp->secsize);

	if (sp->pregapsize >= 0) {
/*		trackp[track].flags &= ~TI_PREGAP;*/
		sp->flags &= ~TI_PREGAP;
		trackp[track].pregapsize = sp->pregapsize;
	} else {
/*		trackp[track].flags |= TI_PREGAP;*/
		if (track > 1)
			sp->flags |= TI_PREGAP;
		if (track == 1)
			trackp[track].pregapsize = sp->index1 + 150;
		else if (sp->index0 < 0)
			trackp[track].pregapsize = 0;
		else if (sp->pflags & PF_INDEX0_PREV)	/* INDEX0 in prev FILE */
			trackp[track].pregapsize = trackp[track-1].tracksecs - sp->index0;
		else
			trackp[track].pregapsize = sp->index1 - sp->index0;
	}

	trackp[track].isecsize = sp->secsize;
	trackp[track].secsize = sp->secsize;
	trackp[track].flags = sp->flags |
		(trackp[0].flags & ~(TI_HIDDEN|TI_SWAB|TI_AUDIO|TI_COPY|TI_QUADRO|TI_PREEMP|TI_SCMS));
	if (trackp[0].flags & TI_RAW) {
		if (is_raw16(&trackp[track]))
			trackp[track].secsize = RAW16_SEC_SIZE;
		else
			trackp[track].secsize = RAW96_SEC_SIZE;
#ifndef	HAVE_LIB_EDC_ECC
		if ((sp->sectype & ST_MODE_MASK) != ST_MODE_AUDIO) {
			errmsgno(EX_BAD,
				_("EDC/ECC library not compiled in.\n"));
			comerrno(EX_BAD,
				_("Data sectors are not supported in RAW mode.\n"));
		}
#endif
	}
	/*
	 * In RAW mode, we need to recompute the track size.
	 */
	trackp[track].tracksize =
			(trackp[track].itracksize / trackp[track].isecsize) *
			trackp[track].secsize
			+ trackp[track].itracksize % trackp[track].isecsize;

	trackp[track].secspt = 0;	/* transfer size is set up in set_trsizes() */
/*	trackp[track].pktsize = pktsize; */
	trackp[track].pktsize = 0;
	trackp[track].trackno = sp->trackno;
	trackp[track].sectype = sp->sectype;

	trackp[track].dataoff = sp->dataoff;
	trackp[track].tracktype = sp->tracktype;
	trackp[track].dbtype = sp->dbtype;

	if (track == 1) {
		track_t	*tp0 = &trackp[0];
		track_t	*tp1 = &trackp[1];

		tp0->tracktype &= ~TOC_MASK;
		tp0->tracktype |= sp->tracktype;

		/*
		 * setleadinout() also sets: sectype dbtype dataoff
		 */
		tp0->sectype = tp1->sectype;
		tp0->dbtype  = tp1->dbtype;
		tp0->dataoff = tp1->dataoff;
		tp0->isecsize = tp1->isecsize;
		tp0->secsize = tp1->secsize;

		if (sp->index0 == 0 && sp->index1 > 0) {

			tp0->filename = tp1->filename;
			tp0->xfp = xopen(sp->filename, O_RDONLY|O_BINARY, 0, X_NOREWIND);
			tp0->trackstart = tp1->trackstart;
			tp0->itracksize = sp->index1 * tp1->isecsize;
			tp0->tracksize = sp->index1 * tp1->secsize;
			tp0->tracksecs = sp->index1;
			tp1->tracksecs -= sp->index1;
			sp->secoff += sp->index1;
			sp->trackoff += sp->index1 * tp1->isecsize;
			tp1->flags &= ~TI_PREGAP;
			tp0->flags |= tp1->flags &
					(TI_SWAB|TI_AUDIO|TI_COPY|TI_QUADRO|TI_PREEMP|TI_SCMS);
			tp0->flags |= TI_HIDDEN;
			tp1->flags |= TI_HIDDEN;
		}

		if (xdebug > 1) {
			printf(_("Track[%2.2d] %2.2d Tracktype %X\n"),
					0, 0, trackp[0].tracktype);
		}
	}
	if (xdebug > 1) {
		printf(_("Track[%2.2d] %2.2d Tracktype %X\n"),
				track, sp->trackno, trackp[track].tracktype);
	}
	trackp[track].nindex = 1;
	trackp[track].tindex = 0;

	if (xdebug > 1) {
		printf(_("Track[%2.2d] %2.2d flags 0x%08X\n"), 0, 0, trackp[0].flags);
		printf(_("Track[%2.2d] %2.2d flags 0x%08X\n"), track, sp->trackno, trackp[track].flags);
	}
}

/*--------------------------------------------------------------------------*/
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

LOCAL char *
state_name(st)
	int	st;
{
	if (st < STATE_NONE || st > STATE_MAX)
		return ("UNKNOWN");
	return (states[st]);
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
LOCAL	char	nulldelim[] = "";

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
cueopen(name)
	char	*name;
{
	FILE	*f;

	f = fileopen(name, "r");
	if (f == NULL)
		comerr(_("Cannot open '%s'.\n"), name);

	fname = name;
	return (f);
}

LOCAL char *
cuename()
{
	return (fname);
}

LOCAL char *
nextline(f)
	FILE	*f;
{
	register int	len;

	do {
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

LOCAL char
getworddelim()
{
	return (wordendc);
}

LOCAL char *
getnextitem(delim)
	char	*delim;
{
	*wordendp = wordendc;

	linep = skipwhite(wordendp);
	return (markword(delim));
}

LOCAL char *
neednextitem(delim, type)
	char	*delim;
	char	*type;
{
	char	*olinep = linep;
	char	*nlinep;

	nlinep = getnextitem(delim);

	if ((olinep == nlinep) || (*nlinep == '\0')) {
		if (type == NULL)
			cueabort(_("Missing text"));
		else
			cueabort(_("Missing '%s'"), type);
	}

	return (nlinep);
}

#ifdef	__needed__
LOCAL char *
nextword()
{
	return (getnextitem(worddelim));
}
#endif

LOCAL char *
needword(type)
	char	*type;
{
	return (neednextitem(worddelim, type));
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
needitem(type)
	char	*type;
{
	return (neednextitem(nulldelim, type));
}

LOCAL void
checkextra()
{
	if (peekword() < lineend())
		cueabort(_("Extra text '%s'"), peekword());
}

/* VARARGS2 */
#ifdef	PROTOTYPES
LOCAL void
statewarn(state_t *sp, const char *fmt, ...)
#else
LOCAL void
statewarn(sp, fmt, va_alist)
	state_t	*sp;
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	errmsgno(EX_BAD, _("%r. Current state is '%s'.\n"),
		fmt, args, state_name(sp->state));
	va_end(args);
}

#ifdef	__needed__
/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL void
cuewarn(const char *fmt, ...)
#else
LOCAL void
cuewarn(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	errmsgno(EX_BAD, _("%r on line %d col %d in '%s'.\n"),
		fmt, args, lineno, linep - linebuf, fname);
	va_end(args);
}
#endif

/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL void
cueabort(const char *fmt, ...)
#else
LOCAL void
cueabort(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
#ifdef	PARSE_DEBUG
	errmsgno(EX_BAD, _("%r on line %d col %d in '%s'.\n"),
#else
	comerrno(EX_BAD, _("%r on line %d col %d in '%s'.\n"),
#endif
		fmt, args, lineno, linep - linebuf, fname);
	va_end(args);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL void
extabort(const char *fmt, ...)
#else
LOCAL void
extabort(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	errmsgno(EX_BAD, _("Unsupported by CDRWIN: %r on line %d col %d in '%s'.\n"),
		fmt, args, lineno, linep - linebuf, fname);
	va_end(args);
	errmsgno(EX_BAD, _("Add 'REM CDRTOOLS' to enable cdrtools specific CUE extensions.\n"));
	comexit(EX_BAD);
}
