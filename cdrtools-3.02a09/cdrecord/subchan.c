/* @(#)subchan.c	1.29 17/07/17 Copyright 2000-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)subchan.c	1.29 17/07/17 Copyright 2000-2017 J. Schilling";
#endif
/*
 *	Subchannel processing
 *
 *	Copyright (c) 2000-2017 J. Schilling
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
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include <scg/scsitransp.h>

#include "cdrecord.h"
#include "crc16.h"

EXPORT	int	do_leadin	__PR((track_t *trackp));
EXPORT	int	write_leadin	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp, int leadinstart));
EXPORT	int	write_leadout	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
EXPORT	void	fillsubch	__PR((track_t *trackp, Uchar *sp, int secno, int nsecs));
EXPORT	void	filltpoint	__PR((Uchar *sub, int ctrl_adr, int point, msf_t *mp));
EXPORT	void	fillttime	__PR((Uchar *sub, msf_t *mp));
LOCAL	void	filldsubq	__PR((Uchar *sub, int ca, int t, int i, msf_t *mrp, msf_t *mp));
LOCAL	void	fillmcn		__PR((Uchar *sub, Uchar *mcn));
LOCAL	void	fillisrc	__PR((Uchar *sub, Uchar *isrc));
LOCAL	int	ascii2q		__PR((int c));
LOCAL	void	qpto16		__PR((Uchar *sub, Uchar *subq, int dop));
EXPORT	void	qpto96		__PR((Uchar *sub, Uchar *subq, int dop));
EXPORT	void	addrw		__PR((Uchar *sub, Uchar	*subrwptr));
EXPORT	void	qwto16		__PR((Uchar *subq, Uchar *subptr));
EXPORT	void	subrecodesecs	__PR((track_t *trackp, Uchar *bp, int address, int nsecs));
#ifdef	DO_SUBINTERLEAVE
LOCAL	void	subinterleave	__PR((Uchar *sub));
#endif

/*#define	TEST_CRC*/
#ifdef	TEST_CRC
LOCAL	void	testcrc		__PR((void));
#endif

/*Die 96 Bits == 12 Bytes haben folgendes Aussehen:*/

struct q {
	Uchar ctrl_adr;	/*  0 (ctrl << 4) | adr		 */
	Uchar track;	/*  1 current track #		 */
	Uchar index;	/*  2 current index #		 */
	Uchar pmin;	/*  3 Relative time minutes part */
	Uchar psec;	/*  4 Relative time seconds part */
	Uchar pframe;	/*  5 Relative time frames part  */
	Uchar zero;	/*  6 */
	Uchar amin;	/*  7 Absolute time minutes part */
	Uchar asec;	/*  8 Absolute time seconds part */
	Uchar aframe;	/*  9 Absolute time frames part  */
	Uchar crc1;	/* 10	all bits inverted. Polynom is	*/
	Uchar crc2;	/* 11	X^16 + X^12 + X^5 + 1		*/
};

EXPORT	Uchar	_subq[110][12];
EXPORT	int	_nsubh;

extern	int	lverbose;
extern	int	xdebug;

/*
 * Prepare master sunchannel data for RAW TOC.
 */
EXPORT int
do_leadin(trackp)
	track_t	*trackp;
{
	int	tracks = trackp->tracks;
	msf_t	m;
	int	ctrl;
	int	i;
	int	toctype = trackp[0].tracktype & TOC_MASK;

	if (_nsubh) {
		if (xdebug)
			printf(_("Using CLONE TOC....\n"));
		return (0);
	}
	if (xdebug)
		printf(_("Leadin TOC Type: %d\n"), trackp[0].tracktype & TOC_MASK);
	if (lverbose > 1) {
		for (i = 1; i <= tracks+1; i++)
			printf(_("Track %d start %ld\n"), i, trackp[i].trackstart);
	}

#ifdef	TEST_CRC
	testcrc();
/*	exit(1);*/
#endif

	fillbytes(_subq, sizeof (_subq), '\0');

	/*
	 * Fill in point 0xA0 for first track # on disk
	 */
	ctrl = (st2mode[trackp[0].sectype & ST_MASK]) << 4;
	if (is_copy(&trackp[0]))
		ctrl |= TM_ALLOW_COPY << 4;
	m.msf_min = trackp[1].trackno;
	/*
	 * Disk Type: 0 = AUDIO/DATA, 0x10 = CDI, 0x20 = XA mode 2
	 */
	m.msf_sec = toc2sess[toctype & TOC_MASK];
	m.msf_sec = from_bcd(m.msf_sec);		/* convert to BCD */
	m.msf_frame = 0;
	filltpoint(_subq[0], ctrl|0x01, 0xA0, &m);
	if (lverbose > 1)
		scg_prbytes("", _subq[0], 12);

	/*
	 * Fill in point 0xA1 for last track # on disk
	 */
	ctrl = (st2mode[trackp[tracks].sectype & ST_MASK]) << 4;
	if (is_copy(&trackp[tracks]))
		ctrl |= TM_ALLOW_COPY << 4;
	m.msf_min = trackp[tracks].trackno;
	m.msf_sec = 0;
	m.msf_frame = 0;
	filltpoint(_subq[1], ctrl|0x01, 0xA1, &m);
	if (lverbose > 1)
		scg_prbytes("", _subq[1], 12);

	/*
	 * Fill in point 0xA2 for lead out start time on disk
	 */
	lba_to_msf(trackp[tracks+1].trackstart, &m);
	ctrl = (st2mode[trackp[tracks].sectype & ST_MASK]) << 4;
	if (is_copy(&trackp[tracks]))
		ctrl |= TM_ALLOW_COPY << 4;
	filltpoint(_subq[2], ctrl|0x01, 0xA2, &m);
	if (lverbose > 1)
		scg_prbytes("", _subq[2], 12);

	/*
	 * Fill in track start times.
	 */
	for (i = 1; i <= tracks; i++) {
		lba_to_msf(trackp[i].trackstart, &m);
		ctrl = (st2mode[trackp[i].sectype & ST_MASK]) << 4;
		if (is_copy(&trackp[i]))
			ctrl |= TM_ALLOW_COPY << 4;
		filltpoint(_subq[i-1+3], ctrl|0x01, to_bcd(trackp[i].trackno), &m);	/* track n */
		if (lverbose > 1)
			scg_prbytes("", _subq[i-1+3], 12);
	}
	return (0);
}

/*
 * Write TOC (lead-in)
 *
 * Use previously prepared master subchannel data to create the
 * subchannel frames for the lead-in.
 */
EXPORT int
write_leadin(scgp, dp, trackp, leadinstart)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
	int	leadinstart;
{
	msf_t	m;
	int	i;
	Uint	j;
	Uchar	*bp = scgp->bufptr;
	Uchar	*subp;
	Uchar	*sp;
	int	secsize;
	int	secspt;
	int	bytespt;
	long	amount;
	int	startsec;
	long	bytes = 0L;
	int	textoff = 0;
	msf_t	msf;

	secsize = trackp[0].secsize;
	secspt = trackp[0].secspt;
	bytespt = secspt * secsize;

	lba_to_msf(leadinstart, &msf);

	fillbytes(bp, bytespt, '\0');

	if (_nsubh) {
		if (xdebug)
			printf(_("Using CLONE LEADIN\n"));
	}
	if (xdebug) {
		printf(_("Leadinstart: %d %d:%d/%d"),
			leadinstart,
			msf.msf_min, msf.msf_sec, msf.msf_frame);
		printf(_(" FLAGS: 0x%X sect: %X RAW16:%d secs: %d spt: %d\n"),
			trackp[0].flags, trackp[0].sectype,
			is_raw16(&trackp[0]), secsize, secspt);
	}

	startsec = leadinstart;
	sp = bp;
	subp = bp + 2352;
	for (i = leadinstart, j = 0; i < -150; i++, j++) {
		/*
		 * TOC hat folgende unterschiedliche Sub Q Frames:
		 * A0		First Track
		 * A1		Last Track
		 * A3		Lead out start
		 * 1..99	Tracks
		 * ==	3 + N* tracks
		 * Jeder Frame wird 3x wiederholt.
		 */
		if (_nsubh) {
			if (j >= (3*_nsubh))
				j = 0;
		} else {
			if (j >= (3*3 + 3*trackp->tracks))
				j = 0;
		}
		lba_to_msf((long)i, &m);
		fillttime(_subq[j/3], &m);
		fillcrc(_subq[j/3], 12);
		if (xdebug > 2)
			scg_prbytes("", _subq[j/3], 12);
		if (is_raw16(&trackp[0])) {
			qpto16(subp, _subq[j/3], 0);
		} else {
			extern Uchar *textsub;
			extern int  textlen;

			qpto96(subp, _subq[j/3], 0);
			if (textsub) {
				addrw(subp, &textsub[textoff]);
				textoff += 96;
				if (textoff >= textlen)
					textoff = 0;
			}
#ifdef	DO_SUBINTERLEAVE
/*			if (is_raw96p(&trackp[0]))*/
/*				subinterleave(subp);*/
#endif
		}
		if ((startsec+secspt-1) == i || i == -151) {
			if ((i-startsec+1) < secspt) {
				secspt = i-startsec+1;
				bytespt = secspt * secsize;
			}
			encsectors(trackp, bp, startsec, secspt);

			amount = write_secs(scgp, dp,
					(char *)bp, startsec, bytespt, secspt, FALSE);
			if (amount < 0) {
				printf(_("write leadin data: error after %ld bytes\n"),
							bytes);
				return (-1);
			}
			bytes += amount;
			startsec = i+1;
			sp = bp;
			subp = bp + 2352;
			continue;
		}
		sp += secsize;
		subp += secsize;
	}
	return (0);
}

/*
 * Write Track 0xAA (lead-out)
 */
EXPORT int
write_leadout(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	int	tracks = trackp->tracks;
	msf_t	m;
	msf_t	mr;
	int	ctrl;
	int	i;
	int	j;
	Uchar	*bp = scgp->bufptr;
	Uchar	*subp;
	Uchar	*sp;
	int	secsize;
	int	secspt;
	int	bytespt;
	long	amount;
	long	startsec;
	long	endsec;
	long	bytes = 0L;
	int	leadoutstart;
	Uchar	sub[12];
	BOOL	p;
	msf_t	msf;

	fillbytes(sub, 12, '\0');

	secsize = trackp[tracks+1].secsize;
	secspt = trackp[tracks+1].secspt;
	bytespt = secspt * secsize;

	leadoutstart = trackp[tracks+1].trackstart;
	lba_to_msf(leadoutstart, &msf);

	fillbytes(bp, bytespt, '\0');

	if (xdebug) {
		printf(_("Leadoutstart: %d %d:%d/%d amt %ld"),
			leadoutstart,
			msf.msf_min, msf.msf_sec, msf.msf_frame,
			trackp[tracks+1].tracksecs);
		printf(_(" FLAGS: 0x%X sect: %X RAW16:%d secs: %d spt: %d\n"),
			trackp[tracks+1].flags, trackp[tracks+1].sectype,
			is_raw16(&trackp[tracks+1]), secsize, secspt);
	}

	startsec = leadoutstart;
	endsec = startsec + trackp[tracks+1].tracksecs;
	sp = bp;
	subp = bp + 2352;
	ctrl = (st2mode[trackp->sectype & ST_MASK]) << 4;
	if (is_copy(trackp))
		ctrl |= TM_ALLOW_COPY << 4;

	for (i = leadoutstart, j = 0; i < endsec; i++, j++) {

		lba_to_msf((long)i, &m);
		sec_to_msf((long)j, &mr);
		filldsubq(sub, ctrl|0x01, 0xAA, 1, &mr, &m);
		sub[1] = 0xAA;
		fillcrc(sub, 12);
		p = (j % 150) < 75;
		if (j < 150)
			p = FALSE;
		if (xdebug > 2)
			scg_prbytes(p?"P":" ", sub, 12);

		if (is_raw16(&trackp[0])) {
			qpto16(subp, sub, p);
		} else {
			qpto96(subp, sub, p);
#ifdef	DO_SUBINTERLEAVE
/*			if (is_raw96p(&trackp[0]))*/
/*				subinterleave(subp);*/
#endif
		}
		if ((startsec+secspt-1) == i || i == (endsec-1)) {
			if ((i-startsec+1) < secspt) {
				secspt = i-startsec+1;
				bytespt = secspt * secsize;
			}
			encsectors(trackp, bp, startsec, secspt);

			amount = write_secs(scgp, dp,
					(char *)bp, startsec, bytespt, secspt, FALSE);
			if (amount < 0) {
				printf(_("write leadout data: error after %ld bytes\n"),
							bytes);
				return (-1);
			}
			bytes += amount;
			startsec = i+1;
			sp = bp;
			subp = bp + 2352;
			continue;
		}
		sp += secsize;
		subp += secsize;
	}
	return (0);
}

/*
 * Fill in subchannel data.
 *
 * This function is used to prepare the sub channels when writing
 * the data part of a CD (bewteen lead-in and lead-out).
 */
EXPORT void
fillsubch(trackp, sp, secno, nsecs)
	track_t	*trackp;
	Uchar	*sp;	/* Sector data pointer	*/
	int	secno;	/* Starting sector #	*/
	int	nsecs;	/* # of sectors to fill	*/
{
	msf_t	m;
	msf_t	mr;
	int	ctrl;
	int	i;
	int	rsecno;
	int	end;
	int	secsize = trackp->secsize;
	int	trackno = trackp->trackno;
	int	nindex = trackp->nindex;
	int	curindex;
	long	*tindex = NULL;
	long	nextindex = 0L;
	Uchar	sub[12];
	Uchar	*sup;
	char	*mcn;
	/*
	 * In theory this should make fillsubch() non-reenrtrant but it seems
	 * that the probability check at the beginning of the function is
	 * sufficient to make it work as expected.
	 */
static	long	nextmcn = -1000000L;
static	long	nextisrc = -1000000L;
static	Uchar	lastindex = 255;

	if (trackno == 0 && is_hidden(trackp))
		trackno = trackp[1].trackno;

	fillbytes(sub, 12, '\0');

	mcn = track_base(trackp)->isrc;
	rsecno = secno - trackp->trackstart;
	if ((secno + nsecs) > (trackp->trackstart + trackp->tracksecs)) {
		comerrno(EX_BAD,
			_("Implementation botch: track boundary in buffer.\n"));
	}
	sup = sp + 2352;
	if (mcn && (nextmcn < secno || nextmcn > (secno+100))) {
		nextmcn = secno/100*100 + 99;
	}
	if (trackp->isrc && (nextisrc < secno || nextisrc > (secno+100))) {
		nextisrc = secno/100*100 + 49;
	}
	ctrl = (st2mode[trackp->sectype & ST_MASK]) << 4;
	if (is_copy(trackp))
		ctrl |= TM_ALLOW_COPY << 4;

#ifdef	SUB_DEBUG
	error("Tracknl %d nindex %d trackstart %ld rsecno %d index0start %ld nsecs %d\n",
	trackno, nindex, trackp->trackstart, rsecno, trackp->index0start, nsecs);
#endif

	if (rsecno < 0) {
		/*
		 * Called to manually write pregap null data into the pregap
		 * while 'trackp' points to the curent track. For this reason,
		 * the sectors are before the start of track 'n' index 0.
		 */
		curindex = 0;
		end = trackp->trackstart;

	} else if (rsecno > trackp->index0start) {
		/*
		 * This track contains pregap of next track.
		 * We currently are inside this pregap.
		 */
		trackno++;
		curindex = 0;
		end = trackp->trackstart + trackp->tracksecs;
	} else {
		/*
		 * We are inside the normal data part of this track.
		 * This is index_1...index_m for track n.
		 * Set 'end' to 0 in this case although it is only used
		 * if 'index' is 0. But GCC gives a warning that 'end'
		 * might be uninitialized.
		 */
		end = 0;
		curindex = 1;
		if (nindex > 1) {
			tindex = trackp->tindex;
			nextindex = trackp->tracksecs;
			/*
			 * find current index in list
			 */
			for (curindex = nindex; curindex >= 1; curindex--) {
				if (rsecno >= tindex[curindex]) {
					if (curindex < nindex)
						nextindex = tindex[curindex+1];
					break;
				}
			}
		}
	}

	for (i = 0; i < nsecs; i++) {

		if (tindex != NULL && rsecno >= nextindex) {
			/*
			 * Skip to next index in list.
			 */
			if (curindex < nindex) {
				curindex++;
				nextindex = tindex[curindex+1];
			}
		}
		if (rsecno == trackp->index0start) {
			/*
			 * This track contains pregap of next track.
			 */
			trackno++;
			curindex = 0;
			end = trackp->trackstart + trackp->tracksecs;
		}
		lba_to_msf((long)secno, &m);
		if (curindex == 0)
			sec_to_msf((long)end-1 - secno, &mr);
		else
			sec_to_msf((long)rsecno, &mr);
		if (is_scms(trackp)) {
			if ((secno % 8) <= 3) {
				ctrl &= ~(TM_ALLOW_COPY << 4);
			} else {
				ctrl |= TM_ALLOW_COPY << 4;
			}
		}
		filldsubq(sub, ctrl|0x01, trackno, curindex, &mr, &m);
		if (mcn && (secno == nextmcn)) {
			if (curindex == lastindex) {
				fillmcn(sub, (Uchar *)mcn);
				nextmcn = (secno+1)/100*100 + 99;
			} else {
				nextmcn++;
			}
		}
		if (trackp->isrc && (secno == nextisrc)) {
			if (curindex == lastindex) {
				fillisrc(sub, (Uchar *)trackp->isrc);
				nextisrc = (secno+1)/100*100 + 49;
			} else {
				nextisrc++;
			}
		}
		fillcrc(sub, 12);
		if (xdebug > 2)
			scg_prbytes(curindex == 0 ? "P":" ", sub, 12);
		if (is_raw16(trackp)) {
			qpto16(sup, sub, curindex == 0);
		} else {
			qpto96(sup, sub, curindex == 0);
#ifdef	DO_SUBINTERLEAVE
/*			if (is_raw96p(trackp))*/
/*				subinterleave(sup);*/
#endif
		}
		lastindex = curindex;
		secno++;
		rsecno++;
		sup += secsize;
	}
}


/*
 * Fill TOC Point
 * Ax Werte einfüllen.
 */
EXPORT void
filltpoint(sub, ctrl_adr, point, mp)
	Uchar	*sub;
	int	ctrl_adr;
	int	point;
	msf_t	*mp;
{
	sub[0] = ctrl_adr;
	sub[2] = point;
	sub[7] = to_bcd(mp->msf_min);
	sub[8] = to_bcd(mp->msf_sec);
	sub[9] = to_bcd(mp->msf_frame);
}

/*
 * Fill TOC time
 * Aktuelle Zeit in TOC Sub-Q einfüllen.
 */
EXPORT void
fillttime(sub, mp)
	Uchar	*sub;
	msf_t	*mp;
{
	sub[3] = to_bcd(mp->msf_min);
	sub[4] = to_bcd(mp->msf_sec);
	sub[5] = to_bcd(mp->msf_frame);
}

/*
 * Q-Sub in Datenbereich füllen.
 */
LOCAL void
filldsubq(sub, ca, t, i, mrp, mp)
	Uchar	*sub;
	int	ca;	/* Control/Addr	*/
	int	t;	/* Track	*/
	int	i;	/* Index	*/
	msf_t	*mrp;	/* Relative time*/
	msf_t	*mp;	/* Absolute time*/
{
	sub[0] = ca;
	sub[1] = to_bcd(t);
	sub[2] = to_bcd(i);
	sub[3] = to_bcd(mrp->msf_min);
	sub[4] = to_bcd(mrp->msf_sec);
	sub[5] = to_bcd(mrp->msf_frame);

	sub[7] = to_bcd(mp->msf_min);
	sub[8] = to_bcd(mp->msf_sec);
	sub[9] = to_bcd(mp->msf_frame);
}

/*
 * Fill MCN
 * MCN konvertieren und in Sub-Q einfüllen.
 */
LOCAL void
fillmcn(sub, mcn)
	Uchar	*sub;
	Uchar	*mcn;
{
	register int	i;
	register int	c;

	sub[0] = ADR_MCN;
	for (i = 1; i <= 8; i++) {
		c = *mcn++;
		if (c >= '0' && c <= '9')
			sub[i] = (c - '0') << 4;
		else
			sub[i] = 0;

		if (c != '\0')
			c = *mcn++;
		if (c >= '0' && c <= '9')
			sub[i] |= (c - '0');

		if (c == '\0') {
			i++;
			break;
		}
	}
	for (; i <= 8; i++)
		sub[i] = '\0';
}

/*
 * Fill ISRC
 * ISRC konvertieren und in Sub-Q einfüllen.
 */
LOCAL void
fillisrc(sub, isrc)
	Uchar	*sub;
	Uchar	*isrc;
{
	register int	i;
	register int	j;
	Uchar		tmp[13];
	Uchar		*sp;

	sub[0] = ADR_ISRC;
	sp = &sub[1];

	/*
	 * Convert into Sub-Q character coding
	 */
	for (i = 0, j = 0; i < 12; i++) {
		if (isrc[i+j] == '-')
			j++;
		if (isrc[i+j] == '\0')
			break;
		tmp[i] = ascii2q(isrc[i+j]);
	}
	for (; i < 13; i++)
		tmp[i] = '\0';

	/*
	 * The first 5 chars from e.g. "FI-BAR-99-00312"
	 */
	sp[0]  = tmp[0] << 2;
	sp[0] |= (tmp[1] >> 4) & 0x03;
	sp[1]  = (tmp[1] << 4) & 0xF0;
	sp[1] |= (tmp[2] >> 2) & 0x0F;
	sp[2]  = (tmp[2] << 6) & 0xC0;
	sp[2] |= tmp[3] & 0x3F;
	sp[3]  = tmp[4] << 2;

	/*
	 * Now 7 digits from e.g. "FI-BAR-99-00312"
	 */
	for (i = 4, j = 5; i < 8; i++) {
		sp[i]  = tmp[j++] << 4;
		sp[i] |= tmp[j++];
	}
}

/*
 * ASCII -> Sub-Q ISRC code conversion
 */
LOCAL int
ascii2q(c)
	int	c;
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	else if (c >= '@' && c <= 'o')
		return (0x10 + c - '@');
	return (0);
}

/*
 * Q-Sub auf 16 Bytes blähen und P-Sub addieren
 *
 * OUT: sub, IN: subqptr
 */
LOCAL void
qpto16(sub, subqptr, dop)
	Uchar	*sub;
	Uchar	*subqptr;
	int	dop;
{
	if (sub != subqptr)
		movebytes(subqptr, sub, 12);
	sub[12] = '\0';
	sub[13] = '\0';
	sub[14] = '\0';
	sub[15] = '\0';
	if (dop)
		sub[15] |= 0x80;

}

/*
 * Q-Sub auf 96 Bytes blähen und P-Sub addieren
 *
 * OUT: sub, IN: subqptr
 */
EXPORT void
qpto96(sub, subqptr, dop)
	Uchar	*sub;
	Uchar	*subqptr;
	int	dop;
{
	Uchar	tmp[16];
	Uchar	*p;
	int	c;
	int	i;

	if (subqptr == sub) {
		/*
		 * Remember 12 byte subchannel data if subchannel
		 * is overlapping.
		 */
		movebytes(subqptr, tmp, 12);
		subqptr = tmp;
	}
	/*
	 * Clear all subchannel bits in the 96 byte target space.
	 */
	fillbytes(sub, 96, '\0');

	/* BEGIN CSTYLED */
	if (dop) for (i = 0, p = sub; i < 96; i++) {
		*p++ |= 0x80;
	}
	for (i = 0, p = sub; i < 12; i++) {
		c = subqptr[i] & 0xFF;
/*printf("%02X\n", c);*/
		if (c & 0x80)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x40)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x20)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x10)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x08)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x04)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x02)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x01)
			*p++ |= 0x40;
		else
			p++;
	}
}

/*
 * Add R-W-Sub (96 Bytes) to P-Q-Sub (96 Bytes)
 *
 * OUT: sub, IN: subrwptr
 */
EXPORT void
addrw(sub, subrwptr)
	register Uchar	*sub;
	register Uchar	*subrwptr;
{
	register int	i;

#define	DO8(a)	a; a; a; a; a; a; a; a;

	for (i = 0; i < 12; i++) {
		DO8(*sub++ |= *subrwptr++ & 0x3F);
	}
}

/*
 * Q-W-Sub (96 Bytes) auf 16 Bytes schrumpfen
 *
 * OUT: subq, IN: subptr
 */
EXPORT void
qwto16(subq, subptr)
	Uchar	*subq;
	Uchar	*subptr;
{
	register int	i;
	register int	np = 0;
	register Uchar	*p;
		Uchar	tmp[96];

	p = subptr;
	for (i = 0; i < 96; i++)
		if (*p++ & 0x80)
			np++;
	p = subptr;
	if (subptr == subq) {
		/*
		 * Remember 96 byte subchannel data if subchannel
		 * is overlapping.
		 */
		movebytes(subptr, tmp, 96);
		p = tmp;
	}

	for (i = 0; i < 12; i++) {
		subq[i] = 0;
		if (*p++ & 0x40)
			subq[i] |= 0x80;
		if (*p++ & 0x40)
			subq[i] |= 0x40;
		if (*p++ & 0x40)
			subq[i] |= 0x20;
		if (*p++ & 0x40)
			subq[i] |= 0x10;
		if (*p++ & 0x40)
			subq[i] |= 0x08;
		if (*p++ & 0x40)
			subq[i] |= 0x04;
		if (*p++ & 0x40)
			subq[i] |= 0x02;
		if (*p++ & 0x40)
			subq[i] |= 0x01;
	}
	subq[12] = 0;
	subq[13] = 0;
	subq[14] = 0;
	if (np > (96/2))
		subq[15] = 0x80;
}

/*
 * Recode subchannels of sectors from 2352 + 96 bytes to 2352 + 16 bytes
 */
EXPORT void
subrecodesecs(trackp, bp, address, nsecs)
	track_t	*trackp;
	Uchar	*bp;
	int	address;
	int	nsecs;
{
	bp += 2352;
	while (--nsecs >= 0) {
		qwto16(bp, bp);
		bp += trackp->isecsize;
	}
}

#ifndef	HAVE_LIB_EDC_ECC
EXPORT void
encsectors(trackp, bp, address, nsecs)
	track_t	*trackp;
	Uchar	*bp;
	int	address;
	int	nsecs;
{
	int	sectype = trackp->sectype;

	if ((sectype & ST_MODE_MASK) == ST_MODE_AUDIO)
		return;

	comerrno(EX_BAD, _("Can only write audio sectors in RAW mode.\n"));
}

EXPORT void
scrsectors(trackp, bp, address, nsecs)
	track_t	*trackp;
	Uchar	*bp;
	int	address;
	int	nsecs;
{
	comerrno(EX_BAD, _("Cannot write in clone RAW mode.\n"));
}
#endif

/*--------------------------------------------------------------------------*/
#ifdef	TEST_CRC

Uchar	tq[12] = { 0x01, 0x00, 0xA0, 0x98, 0x06, 0x12, 0x00, 0x01, 0x00, 0x00, 0xE3, 0x74 };

/*
01 00 A0 98 06 12 00 01 00 00 E3 74
01 00 A0 98 06 13 00 01 00 00 49 25
01 00 A1 98 06 14 00 13 00 00 44 21
01 00 A1 98 06 15 00 13 00 00 EE 70
01 00 A1 98 06 16 00 13 00 00 00 A2
01 00 A2 98 06 17 00 70 40 73 E3 85
01 00 A2 98 06 18 00 70 40 73 86 7C
01 00 A2 98 06 19 00 70 40 73 2C 2D
01 00 01 98 06 20 00 00 02 00 3B 71
01 00 01 98 06 21 00 00 02 00 91 20
01 00 01 98 06 22 00 00 02 00 7F F2
01 00 02 98 06 23 00 03 48 45 BE E0
01 00 02 98 06 24 00 03 48 45 D9 34

*/

LOCAL	int	b	__PR((int bcd));


LOCAL int
b(bcd)
	int	bcd;
{
	return ((bcd & 0x0F) + 10 * (((bcd)>> 4) & 0x0F));
}

LOCAL void
testcrc()
{
	struct q q;
	int	ocrc;
	int	crc1;
	int	crc2;

	movebytes(&tq, &q, 12);
	crc1 = q.crc1 & 0xFF;
	crc2 = q.crc2 & 0xFF;

	/*
	 * Per RED Book, CRC Bits on disk are inverted.
	 * Invert them again to make calcCRC() work.
	 */
	q.crc1 ^= 0xFF;
	q.crc2 ^= 0xFF;

	ocrc = calcCRC((Uchar *)&q, 12);
	printf("AC: %02X t: %3d (%02X) i: %3d (%02X) %d:%d:%d %d:%d:%d %02X%02X %X (%X)\n",
		q.ctrl_adr,
		b(q.track),
		b(q.track) & 0xFF,
		b(q.index),
		q.index & 0xFF,
		b(q.pmin),
		b(q.psec),
		b(q.pframe),
		b(q.amin),
		b(q.asec),
		b(q.aframe),
		crc1, crc2,
		ocrc,
		fillcrc((Uchar *)&q, 12) & 0xFFFF);
}
#endif	/* TEST_CRC */

#if	0
96 / 24 = 4

index 1 < - > 18
index 2 < - > 5
index 3 < - > 23

delay index mod 8
#endif

#ifdef	DO_SUBINTERLEAVE
/*
 * Sub 96 Bytes Interleave
 */
LOCAL void
subinterleave(sub)
	Uchar	*sub;
{
	Uchar	*p;
	int	i;

	for (i = 0, p = sub; i < 4; i++) {
		Uchar	save;

		/*
		 * index 1 < - > 18
		 * index 2 < - > 5
		 * index 3 < - > 23
		 */
		save  = p[1];
		p[1]  = p[18];
		p[18] = save;

		save  = p[2];
		p[2]  = p[5];
		p[5]  = save;

		save  = p[3];
		p[3]  = p[23];
		p[23] = save;

		p += 24;
	}
}
#endif	/* DO_SUBINTERLEAVE */
