/* @(#)cdtext.c	1.20 11/04/03 Copyright 1999-2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cdtext.c	1.20 11/04/03 Copyright 1999-2011 J. Schilling";
#endif
/*
 *	Generic CD-Text support functions
 *
 *	Copyright (c) 1999-2011 J. Schilling
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
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Include sys/types.h to make off_t available */
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include <scg/scsitransp.h>	/* For write_leadin() */

#include "cdtext.h"
#include "cdrecord.h"
#include "crc16.h"

#define	PTI_TITLE	0x80	/* Album name and Track titles */
#define	PTI_PERFORMER	0x81	/* Singer/player/conductor/orchestra */
#define	PTI_SONGWRITER	0x82	/* Name of the songwriter */
#define	PTI_COMPOSER	0x83	/* Name of the composer */
#define	PTI_ARRANGER	0x84	/* Name of the arranger */
#define	PTI_MESSAGE	0x85	/* Message from content provider or artist */
#define	PTI_DISK_ID	0x86	/* Disk identification information */
#define	PTI_GENRE	0x87	/* Genre identification / information */
#define	PTI_TOC		0x88	/* TOC information */
#define	PTI_TOC2	0x89	/* Second TOC */
#define	PTI_RES_8A	0x8A	/* Reserved 8A */
#define	PTI_RES_8B	0x8B	/* Reserved 8B */
#define	PTI_RES_8C	0x8C	/* Reserved 8C */
#define	PTI_CLOSED_INFO	0x8D	/* For internal use by content provider */
#define	PTI_ISRC	0x8E	/* UPC/EAN code of album and ISRC for tracks */
#define	PTI_SIZE	0x8F	/* Size information of the block */

extern	int	xdebug;

typedef struct textpack {
	Uchar	pack_type;	/* Pack Type indicator	*/
	char	track_no;	/* Track Number (0..99)	*/
	char	seq_number;	/* Sequence Number	*/
	char	block_number;	/* Block # / Char pos	*/
	char	text[12];	/* CD-Text Data field	*/
	char	crc[2];		/* CRC 16		*/
} txtpack_t;

#define	EXT_DATA 0x80		/* Extended data indicator in track_no */
#define	DBCC	 0x80		/* Double byte char indicator in block */

/*
 *	CD-Text size example:
 *
 *	0  1  2  3  00 01 02 03 04 05 06 07 08 09 10 11 CRC16
 *
 *	8F 00 2B 00 01 01 0D 03 0C 0C 00 00 00 00 01 00 7B 3D
 *	8F 01 2C 00 00 00 00 00 00 00 12 03 2D 00 00 00 DA B7
 *	8F 02 2D 00 00 00 00 00 09 00 00 00 00 00 00 00 6A 24
 *
 *	charcode 1
 *	first tr 1
 *	last tr  13
 *	Copyr	 3
 *	Pack Count 80= 12, 81 = 12, 86 = 1, 8e = 18, 8f = 3
 *	last seq   0 = 2d
 *	languages  0 = 9
 */

typedef struct textsizes {
	char	charcode;
	char	first_track;
	char	last_track;
	char	copyr_flags;
	char	pack_count[16];
	char	last_seqnum[8];
	char	language_codes[8];
} txtsize_t;

typedef struct textargs {
	txtpack_t	*tp;
	char		*p;
	txtsize_t	*tsize;
	txtpack_t	*endp;
	int		seqno;
} txtarg_t;


EXPORT	Uchar	*textsub;
EXPORT	int	textlen;

EXPORT	BOOL	checktextfile	__PR((char *fname));
LOCAL	void	setuptextdata	__PR((Uchar *bp, int len));
LOCAL	BOOL	cdtext_crc_ok	__PR((struct textpack *p));
EXPORT	void	packtext	__PR((int tracks, track_t *trackp));
LOCAL	BOOL	anytext		__PR((int pack_type, int tracks, track_t *trackp));
LOCAL	void	fillup_pack	__PR((txtarg_t *ap));
LOCAL	void	fillpacks	__PR((txtarg_t *ap, char *from, int len, int track_no, int pack_type));
EXPORT	int	write_cdtext	__PR((SCSI *scgp, cdr_t *dp, long startsec));
LOCAL	void	eight2six	__PR((Uchar *in, Uchar *out));
#ifdef	__needed__
LOCAL	void	six2eight	__PR((Uchar *in, Uchar *out));
#endif


EXPORT BOOL
checktextfile(fname)
	char	*fname;
{
	FILE	*f;
	Uchar	hbuf[4];
	Uchar	*bp;
	struct textpack *tp;
	int	len;
	int	crc;
	int	n;
	int	j;
	off_t	fs;

	if ((f = fileopen(fname, "rb")) == NULL) {
		errmsg(_("Cannot open '%s'.\n"), fname);
		return (FALSE);
	}
	fs = filesize(f);
	if (fs == (off_t)0) {
		errmsgno(EX_BAD, _("Empty CD-Text file.\n"));
		fclose(f);
		return (FALSE);
	}
	j = fs % sizeof (struct textpack);
	if (j == 4) {
		n = fileread(f, hbuf, 4);
		if (n != 4) {
			if (n < 0)
				errmsg(_("Cannot read '%s'.\n"), fname);
			else
				errmsgno(EX_BAD, _("File '%s' is too small for CD-Text.\n"), fname);
			fclose(f);
			return (FALSE);
		}
		len = hbuf[0] * 256 + hbuf[1];
		len -= 2;
		n = fs - 4;
		if (n != len) {
			errmsgno(EX_BAD, _("Inconsistent CD-Text file '%s' length should be %d but is %lld\n"),
				fname, len+4, (Llong)fs);
			fclose(f);
			return (FALSE);
		}
	} else if (j != 0) {
		errmsgno(EX_BAD, _("Inconsistent CD-Text file '%s' not a multiple of pack length\n"),
			fname);
		fclose(f);
		return (FALSE);
	} else {
		len = fs;
	}
	printf(_("Text len: %d\n"), len);
	bp = malloc(len);
	if (bp == NULL) {
		errmsg(_("Cannot malloc CD-Text read buffer.\n"));
		fclose(f);
		return (FALSE);
	}
	n = fileread(f, bp, len);

	tp = (struct textpack *)bp;
	for (n = 0; n < len; n += sizeof (struct textpack), tp++) {
		if (tp->pack_type < 0x80 || tp->pack_type > 0x8F) {
			errmsgno(EX_BAD, _("Illegal pack type 0x%02X pack #%ld in CD-Text file '%s'.\n"),
				tp->pack_type, (long)(n/sizeof (struct textpack)), fname);
			fclose(f);
			return (FALSE);
		}
		crc = (tp->crc[0] & 0xFF) << 8 | (tp->crc[1] & 0xFF);
		crc ^= 0xFFFF;
		if (crc != calcCRC((Uchar *)tp, sizeof (*tp)-2)) {
			if (cdtext_crc_ok(tp)) {
				errmsgno(EX_BAD,
				_("Corrected CRC ERROR in pack #%ld (offset %d-%ld) in CD-Text file '%s'.\n"),
				(long)(n/sizeof (struct textpack)),
				n+j, (long)(n+j+sizeof (struct textpack)),
				fname);
			} else {
			errmsgno(EX_BAD, _("CRC ERROR in pack #%ld (offset %d-%ld) in CD-Text file '%s'.\n"),
				(long)(n/sizeof (struct textpack)),
				n+j, (long)(n+j+sizeof (struct textpack)),
				fname);
			fclose(f);
			return (FALSE);
			}
		}
	}
	setuptextdata(bp, len);
	free(bp);
	fclose(f);

	if (textlen == 0 || textsub == NULL)
		return (FALSE);
	return (TRUE);
}

LOCAL void
setuptextdata(bp, len)
	Uchar	*bp;
	int	len;
{
	int	n;
	int	i;
	int	j;
	Uchar	*p;

	if (xdebug) {
		printf(_("%ld packs %% 4 = %ld\n"),
			(long)(len/sizeof (struct textpack)),
			(long)(len/sizeof (struct textpack)) % 4);
	}
	if (len == 0) {
		errmsgno(EX_BAD, _("No CD-Text data found.\n"));
		return;
	}
	i = (len/sizeof (struct textpack)) % 4;
	if (i == 0) {
		n = len;
	} else if (i == 2) {
		n = 2 * len;
	} else {
		n = 4 * len;
	}
	n = (n * 4) / 3;
	p = malloc(n);
	if (p == NULL) {
		errmsg(_("Cannot malloc CD-Text write buffer.\n"));
		return;
	}
	for (i = 0, j = 0; j < n; ) {
		eight2six(&bp[i%len], &p[j]);
		i += 3;
		j += 4;
	}
	textsub = p;
	textlen = n;

#ifdef	DEBUG
	{
	Uchar	sbuf[10000];
	struct textpack *tp;
	FILE		*f;
	int		crc;

	tp = (struct textpack *)bp;
	p = sbuf;
	for (n = 0; n < len; n += sizeof (struct textpack), tp++) {
		crc = (tp->crc[0] & 0xFF) << 8 | (tp->crc[1] & 0xFF);
		crc ^= 0xFFFF;

		printf("Pack:%3d ", n/ sizeof (struct textpack));
		printf("Pack type: %02X ", tp->pack_type & 0xFF);
		printf("Track #: %2d ", tp->track_no & 0xFF);
		printf("Sequence #:%3d ", tp->seq_number & 0xFF);
		printf("Block #:%3d ", tp->block_number & 0xFF);
		printf("CRC: %04X (%04X) ", crc, calcCRC((Uchar *)tp, sizeof (*tp)-2));
		printf("Text: '%.12s'\n", tp->text);
		movebytes(tp->text, p, 12);
		p += 12;
	}
	printf("len total: %d\n", n);
	f = fileopen("cdtext.out", "wctb");
	if (f) {
		filewrite(f, sbuf, p - sbuf);
		fflush(f);
		fclose(f);
	}
	}
#endif
}

LOCAL BOOL
cdtext_crc_ok(p)
	struct textpack *p;
{
	int		crc;
	struct textpack	new;

	movebytes(p, &new, sizeof (struct textpack));
	new.crc[0] ^= 0xFF;
	new.crc[1] ^= 0xFF;
	crc = calcCRC((Uchar *)&new, sizeof (struct textpack));
	crc = flip_crc_error_corr((Uchar *)&new, sizeof (struct textpack), crc);
	new.crc[0] ^= 0xFF;
	new.crc[1] ^= 0xFF;
	if (crc == 0)
		movebytes(&new, p, 18);

	return (crc == 0);
}


EXPORT void
packtext(tracks, trackp)
	int	tracks;
	track_t	*trackp;
{
	int	type;
	int	i;
	struct textpack *tp;
	struct textsizes tsize;
	txtarg_t targ;
	char	sbuf[256*18];	/* Sufficient for a single language block */
				/* Max 8 languages in total... */

	fillbytes(sbuf, sizeof (sbuf), 0);
	fillbytes(&tsize, sizeof (tsize), 0);

	tsize.charcode		= CC_8859_1;		/* ISO-8859-1	    */
	tsize.first_track	= trackp[1].trackno;
	tsize.last_track	= trackp[1].trackno + tracks - 1;
#ifdef	__FOUND_ON_COMMERCIAL_CD__
	tsize.copyr_flags	= 3;			/* for titles/names */
#else
	tsize.copyr_flags	= 0;			/* no Copyr. limitat. */
#endif
	tsize.pack_count[0x0F]	= 3;			/* 3 size packs	    */
	tsize.last_seqnum[0]	= 0;			/* Start value only */
	tsize.language_codes[0]	= LANG_ENGLISH;		/* English	    */

	tp = (struct textpack *)sbuf;

	targ.tp = tp;
	targ.p = NULL;
	targ.tsize = &tsize;
	targ.endp = (struct textpack *)&sbuf[256 * sizeof (struct textpack)];
	targ.seqno = 0;

	for (type = 0; type <= 0x0E; type++) {
		register int	maxtrk;
		register char	*s;

		if (!anytext(type, tracks, trackp))
			continue;
		maxtrk = tsize.last_track;
		if (type == 6) {
			maxtrk = 0;
		}
		for (i = 0; i <= maxtrk; i++) {
			s = trackp[i].text;
			if (s)
				s = ((textptr_t *)s)->textcodes[type];
			if (s)
				fillpacks(&targ, s, strlen(s)+1, i, 0x80| type);
			else
				fillpacks(&targ, "", 1, i, 0x80| type);

		}
		fillup_pack(&targ);
	}

	/*
	 * targ.seqno overshoots by one and we add 3 size packs...
	 */
	tsize.last_seqnum[0] = targ.seqno + 2;

	for (i = 0; i < 3; i++) {
		fillpacks(&targ, &((char *)(&tsize))[i*12], 12, i, 0x8f);
	}

	setuptextdata((Uchar *)sbuf, targ.seqno*18);

#ifdef	DEBUG
	{	FILE	*f;

	f = fileopen("cdtext.new", "wctb");
	if (f) {
		filewrite(f, sbuf, targ.seqno*18);
		fflush(f);
		fclose(f);
	}
	}
#endif
}

LOCAL BOOL
anytext(pack_type, tracks, trackp)
	int		pack_type;
	int	tracks;
	track_t	*trackp;
{
	register int	i;
	register char	*p;

	for (i = 0; i <= tracks; i++) {
		if (trackp[i].text == NULL)
			continue;
		p = ((textptr_t *)(trackp[i].text))->textcodes[pack_type];
		if (p != NULL && *p != '\0')
			return (TRUE);
	}
	return (FALSE);
}

LOCAL void
fillup_pack(ap)
	register txtarg_t *ap;
{
	if (ap->p) {
		fillbytes(ap->p, &ap->tp->text[12] - ap->p, '\0');
		fillcrc((Uchar *)ap->tp, sizeof (*ap->tp));
		ap->p  = 0;
		ap->tp++;
	}
}

LOCAL void
fillpacks(ap, from, len, track_no, pack_type)
	register txtarg_t	*ap;
	register char		*from;
	register int		len;
	register int		track_no;
	register int		pack_type;
{
	register int		charpos;
	register char		*p;
	register txtpack_t	*tp;

	tp = ap->tp;
	p  = ap->p;
	charpos = 0;
	do {
		if (tp >= ap->endp) {
			comerrno(EX_BAD,
				_("CD-Text size overflow in track %d.\n"),
				track_no);
		}
		if (p == 0) {
			p = tp->text;
			tp->pack_type = pack_type;
			if (pack_type != 0x8f)
				ap->tsize->pack_count[pack_type & 0x0F]++;
			tp->track_no = track_no;
			tp->seq_number = ap->seqno++;
			if (charpos < 15)
				tp->block_number = charpos;
			else
				tp->block_number = 15;
		}
		for (; --len >= 0 && p < &tp->text[12]; charpos++) {
			*p++ = *from++;
		}
		len++;	/* Overshoot compensation */

		if (p >= &tp->text[12]) {
			fillcrc((Uchar *)tp, sizeof (*tp));
			p = 0;
			tp++;
		}
	} while (len > 0);

	ap->tp = tp;
	ap->p = p;
}

EXPORT int
write_cdtext(scgp, dp, startsec)
	SCSI	*scgp;
	cdr_t	*dp;
	long	startsec;
{
	char	*bp = (char *)textsub;
	int	buflen = textlen;
	long	amount;
	long	bytes = 0;
	long	end = -150;
	int	secspt = textlen / 96;
	int	bytespt = textlen;
	long	maxdma = scgp->maxbuf;
	int	idx;
	int	secs;
	int	nbytes;

	if (textlen <= 0)
		return (-1);

/*maxdma = 4320;*/
	if (maxdma >= (2*textlen)) {
		/*
		 * Try to make each CD-Text transfer use as much data
		 * as possible.
		 */
		bp = scgp->bufptr;
		for (idx = 0; (idx + textlen) <= maxdma; idx += textlen)
			movebytes(textsub, &bp[idx], textlen);
		buflen = idx;
		secspt = buflen / 96;
		bytespt = buflen;
/*printf("textlen: %d buflen: %d secspt: %d\n", textlen, buflen, secspt);*/
	} else if (maxdma < buflen) {
		/*
		 * We have more CD-Text data than we may transfer at once.
		 */
		secspt = maxdma / 96;
		bytespt = secspt * 96;
	}
	while (startsec < end) {
		if ((end - startsec) < secspt) {
			secspt = end - startsec;
			bytespt = secspt * 96;
		}
		idx = 0;
		secs = secspt;
		nbytes = bytespt;
		do {			/* loop over CD-Text data buffer */

			if ((idx + nbytes) > buflen) {
				nbytes = buflen - idx;
				secs = nbytes / 96;
			}
/*printf("idx: %d nbytes: %d secs: %d startsec: %ld\n",*/
/*idx, nbytes, secs, startsec);*/
			amount = write_secs(scgp, dp,
				(char *)&bp[idx], startsec, nbytes, secs, FALSE);
			if (amount < 0) {
				printf(_("write CD-Text data: error after %ld bytes\n"),
						bytes);
				return (-1);
			}
			bytes += amount;
			idx += amount;
			startsec += secs;
		} while (idx < buflen && startsec < end);
	}
	return (0);
}


/*
 * 3 input bytes (8 bit based) are converted into 4 output bytes (6 bit based).
 */
LOCAL void
eight2six(in, out)
	register Uchar	*in;
	register Uchar	*out;
{
	register int	c;

	c = in[0];
	out[0]  = (c >> 2) & 0x3F;
	out[1]  = (c & 0x03) << 4;

	c = in[1];
	out[1] |= (c & 0xF0) >> 4;
	out[2]  = (c & 0x0F) << 2;

	c = in[2];
	out[2] |= (c & 0xC0) >> 6;
	out[3]  = c & 0x3F;
}

#ifdef	__needed__
/*
 * 4 input bytes (6 bit based) are converted into 3 output bytes (8 bit based).
 */
LOCAL void
six2eight(in, out)
	register Uchar	*in;
	register Uchar	*out;
{
	register int	c;

	c = in[0] & 0x3F;
	out[0]  = c << 2;

	c = in[1] & 0x3F;
	out[0] |= c >> 4;
	out[1]  = c << 4;

	c = in[2] & 0x3F;
	out[1] |= c >> 2;
	out[2]  = c << 6;

	c = in[3] & 0x3F;
	out[2] |= c;
}
#endif
