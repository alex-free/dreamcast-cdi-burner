/* @(#)clone.c	1.12 10/12/19 Copyright 2001-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)clone.c	1.12 10/12/19 Copyright 2001-2010 J. Schilling";
#endif
/*
 *	Clone Subchannel processing
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
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/btorder.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include <scg/scgcmd.h>
#include <scg/scsitransp.h>

#include "cdrecord.h"
#include "crc16.h"

#include <scg/scsireg.h>
#include "scsimmc.h"

/*#define	SAO_RAW*/

EXPORT	void	clone_toc	__PR((track_t *trackp));
EXPORT	void	clone_tracktype	__PR((track_t *trackp));

extern	int	lverbose;
extern	int	xdebug;

extern	Uchar	_subq[110][12];
extern	int	_nsubh;


LOCAL	int	ctrl_first;
LOCAL	int	ctrl_last;
LOCAL	int	sectype_first;
LOCAL	int	sectype_last;
LOCAL	int	disktype;
LOCAL	long	loutstart;

/*
 * Read Clone TOC description from full toc file.
 */
EXPORT void
clone_toc(trackp)
	track_t	*trackp;
{
	char	filename[1024];
	msf_t	m;
	msf_t	mr;
	struct	tocheader *tp;
	struct	ftrackdesc *fp;
	int	f;
	char	buf[2048];
	int	amt;
	int	len;
	int	i;
	int	j;
	int	ctrladr;
	Uint	first = 100;
	Uint	last = 0;

	len = strlen(trackp[1].filename);
	if (len > (sizeof (filename)-5)) {
		len = sizeof (filename)-5;
	}
	js_snprintf(filename, sizeof (filename), "%.*s.toc", len, trackp[1].filename);

	f = open(filename, O_RDONLY|O_BINARY);
	if (f < 0)
		comerr(_("Cannot open '%s'.\n"), filename);
	amt = read(f, buf, sizeof (buf));

	if (amt == sizeof (buf))
		comerrno(EX_BAD, _("TOC too large.\n"));
	close(f);
	tp = (struct tocheader *)buf;
	len = a_to_u_2_byte(tp->len) + sizeof (struct tocheader)-2;

	if (xdebug) {
		printf(_("Read %d bytes TOC len: %d first session: %d last session: %d\n"),
			amt, len, tp->first, tp->last);
	}

	fp = (struct ftrackdesc *)&buf[4];

	for (i = 4, j = 0; i < len; i += 11) {
		fp = (struct ftrackdesc *)&buf[i];
		if (xdebug)
			scg_prbytes("FT", (Uchar *)&buf[i], 11);
		if (fp->sess_number != 1)
			comerrno(EX_BAD, _("Can only copy session # 1.\n"));

		if (fp->adr == 1) {
			if (fp->point < first) {
				first = fp->point;
				ctrl_first = fp->control;
			}
			if (fp->point <= 99 && fp->point > last) {
				last = fp->point;
				ctrl_last = fp->control;
			}
		}
		if (fp->adr != 1) {
			switch (fp->point) {

			case 0xB0:
			case 0xC0:
			case 0xC1:
				break;
			default:
				continue;
			}
		}
		m.msf_min    = fp->amin;
		m.msf_sec    = fp->asec;
		m.msf_frame  = fp->aframe;

		mr.msf_min   = fp->pmin;
		mr.msf_sec   = fp->psec;
		mr.msf_frame = fp->pframe;

		if (fp->point == 0xA0) {
			disktype = mr.msf_sec;
			mr.msf_sec = from_bcd(mr.msf_sec);		/* convert to BCD */
		}

		if (fp->point == 0xA2)
			loutstart = msf_to_lba(fp->pmin, fp->psec, fp->pframe, TRUE);
		ctrladr = fp->control << 4;
		ctrladr |= fp->adr;

		filltpoint(_subq[j], ctrladr, fp->point, &mr);
		fillttime(_subq[j], &m);
		_subq[j][6] = fp->res7;
		if (fp->point == 0xC0 || fp->point == 0xC1) {
			_subq[j][3] = m.msf_min;
			_subq[j][4] = m.msf_sec;
			_subq[j][5] = m.msf_frame;
		}
		if (fp->point == 0xC1) {
			_subq[j][7] = mr.msf_min;
			_subq[j][8] = mr.msf_sec;
			_subq[j][9] = mr.msf_frame;
		}
		if (xdebug)
			scg_prbytes("TOC  ", _subq[j], 12);
		j++;
	}
	_nsubh = j;
	if (xdebug) {
		printf("nsubheader %d lout: %ld track 1 secs: %ld\n", j, loutstart, trackp[1].tracksecs);
		printf("first %u last %u ctrl first: %X ctrl last %X\n", first, last, ctrl_first, ctrl_last);
	}
	if (trackp->tracks != 1)
		comerrno(EX_BAD, _("Clone writing currently supports only one file argument.\n"));
	if (loutstart > trackp[1].tracksecs)
		comerrno(EX_BAD, _("Clone writing TOC length %ld does not match track length %ld\n"),
			loutstart, trackp[1].tracksecs);

	if (amt > len) {
		sectype_first = buf[len];
		sectype_last = buf[len+1];
		if (xdebug) {
			printf(_("sectype first: %X sectype last %X\n"),
				sectype_first, sectype_last);
		}
	}
}


/*
 * Set tracktypes for track 0 (lead-in) & track AA (lead-out)
 *
 * Control 0 = audio
 * Control 1 = audio preemp
 * Control 2 = audio copy
 * Control 3 = audio copy preemp
 * Control 4 = data
 * Control 5 = packet data
 */
EXPORT void
clone_tracktype(trackp)
	track_t	*trackp;
{
	int	tracks = trackp->tracks;
	int	sectype;

	sectype = SECT_ROM;
	if ((ctrl_first & TM_DATA) == 0) {
		sectype = SECT_AUDIO;

		if ((ctrl_first & TM_PREEM) != 0) {
			trackp[0].flags |= TI_PREEMP;
		} else {
			trackp[0].flags &= ~TI_PREEMP;
			sectype |= ST_PREEMPMASK;
		}
		if ((ctrl_first & TM_ALLOW_COPY) != 0) {
			trackp[0].flags |= TI_COPY;
		} else {
			trackp[0].flags &= ~TI_COPY;
		}
/* XXX ???	flags |= TI_SCMS; */
	} else {
		if ((ctrl_first & TM_INCREMENTAL) != 0) {
			trackp[0].flags |= TI_PACKET;
		} else {
			trackp[0].flags &= ~TI_PACKET;
		}
		if (sectype_first != 0)
			sectype = sectype_first;
	}
	trackp[0].sectype = sectype;

	sectype = SECT_ROM;

	if ((ctrl_last & TM_DATA) == 0) {
		sectype = SECT_AUDIO;

		if ((ctrl_last & TM_PREEM) != 0) {
			trackp[tracks+1].flags |= TI_PREEMP;
		} else {
			trackp[tracks+1].flags &= ~TI_PREEMP;
			sectype |= ST_PREEMPMASK;
		}
		if ((ctrl_last & TM_ALLOW_COPY) != 0) {
			trackp[tracks+1].flags |= TI_COPY;
		} else {
			trackp[tracks+1].flags &= ~TI_COPY;
		}
/* XXX ???	flags |= TI_SCMS; */
	} else {
		if ((ctrl_first & TM_INCREMENTAL) != 0) {
			trackp[0].flags |= TI_PACKET;
		} else {
			trackp[0].flags &= ~TI_PACKET;
			if (sectype_last != 0)
				sectype = sectype_last;
		}
	}
	trackp[tracks+1].sectype = sectype;
}
