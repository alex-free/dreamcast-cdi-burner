/* @(#)scsi_cmds.c	1.56 16/02/14 Copyright 1998-2002,2015 Heiko Eissfeldt, Copyright 2004-2015 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)scsi_cmds.c	1.56 16/02/14 Copyright 1998-2002,2015 Heiko Eissfeldt, Copyright 2004-2015 J. Schilling";
#endif
/*
 * file for all SCSI commands
 * FUA (Force Unit Access) bit handling copied from Monty's cdparanoia.
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

#undef	DEBUG_FULLTOC
#undef	WARN_FULLTOC
#define	TESTSUBQFALLBACK	0

#include "config.h"
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/types.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include <schily/btorder.h>

#define	g5x_cdblen(cdb, len)	((cdb)->count[0] = ((len) >> 16L)& 0xFF,\
				(cdb)->count[1] = ((len) >> 8L) & 0xFF,\
				(cdb)->count[2] = (len) & 0xFF)


#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>

#include <scg/scsitransp.h>

#include "mytype.h"
#include "cdda2wav.h"
#include "interface.h"
#include "byteorder.h"
#include "global.h"
#include "cdrecord.h"
#include "toc.h"
#include "scsi_cmds.h"
#include "exitcodes.h"

unsigned char	*bufferTOC;	/* Global TOC data buffer	*/
int		bufTOCsize;	/* Size of global TOC buffer	*/
subq_chnl	*SubQbuffer;	/* Global Sub-channel buffer	*/

static unsigned	ReadFullTOCSony	__PR((SCSI *scgp));
static unsigned	ReadFullTOCMMC	__PR((SCSI *scgp));


int
SCSI_emulated_ATAPI_on(scgp)
	SCSI	*scgp;
{
/*	return (is_atapi);*/
	if (scg_isatapi(scgp) > 0)
		return (TRUE);

	(void) allow_atapi(scgp, TRUE);
	return (allow_atapi(scgp, TRUE));
}

int
heiko_mmc(scgp)
	SCSI	*scgp;
{
	unsigned char	mode[0x100];
	int		was_atapi;
	struct cd_mode_page_2A	*mp;
	int		retval;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	was_atapi = allow_atapi(scgp, 1);
	scgp->silent++;
	mp = mmc_cap(scgp, mode);
	scgp->silent--;
	allow_atapi(scgp, was_atapi);
	if (mp == NULL)
		return (0);

	/*
	 * have a look at the capabilities
	 */
	if (mp->cd_da_supported == 0) {
		retval = -1;
	} else {
		retval = 1 + mp->cd_da_accurate;
	}
	return (retval);
}


int		accepts_fua_bit;
unsigned char	density = 0;
unsigned char	orgmode4 = 0;
unsigned char	orgmode10, orgmode11;

/*
 * get current sector size from SCSI cdrom drive
 */
unsigned int
get_orig_sectorsize(scgp, m4, m10, m11)
	SCSI		*scgp;
	unsigned char	*m4;
	unsigned char	*m10;
	unsigned char	*m11;
{
	/*
	 * first get current values for density, etc.
	 */
static unsigned char	*modesense = NULL;

	if (modesense == NULL) {
		modesense = malloc(12);
		if (modesense == NULL) {
			errmsg(
			_("Cannot allocate memory for mode sense command in line %d.\n"),
				__LINE__);
			return (0);
		}
	}

	/*
	 * do the scsi cmd
	 */
	if (scgp->verbose)
		fprintf(stderr, _("\nget density and sector size..."));
	if (mode_sense(scgp, modesense, 12, 0x01, 0) < 0)
		fprintf(stderr, _("get_orig_sectorsize mode sense failed\n"));

	/*
	 * FIXME: some drives dont deliver block descriptors !!!
	 */
	if (modesense[3] == 0)
		return (0);

#if	0
	modesense[4] = 0x81;
	modesense[10] = 0x08;
	modesense[11] = 0x00;
#endif

	if (m4 != NULL)				/* density */
		*m4 = modesense[4];
	if (m10 != NULL)			/* MSB sector size */
		*m10 = modesense[10];
	if (m11 != NULL)			/* LSB sector size */
		*m11 = modesense[11];

	return ((modesense[10] << 8) + modesense[11]);
}


/*
 * switch CDROM scsi drives to given sector size
 */
int
set_sectorsize(scgp, secsize)
	SCSI		*scgp;
	unsigned int	secsize;
{
static unsigned char	mode [4 + 8];
	int		retval;

	if (orgmode4 == 0xff) {
		get_orig_sectorsize(scgp, &orgmode4, &orgmode10, &orgmode11);
	}
	if (orgmode4 == 0x82 && secsize == 2048)
		orgmode4 = 0x81;

	/*
	 * prepare to read cds in the previous mode
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	mode[ 3] = 8;			/* Block Descriptor Length */
	mode[ 4] = orgmode4;		/* normal density */
	mode[10] =  secsize >> 8;	/* block length "msb" */
	mode[11] =  secsize & 0xFF;	/* block length lsb */

	if (scgp->verbose)
		fprintf(stderr, _("\nset density and sector size..."));
	/*
	 * do the scsi cmd
	 */
	if ((retval = mode_select(scgp, mode, 12, 0,
					scgp->inq->data_format >= 2)) < 0)
		errmsgno(EX_BAD, _("Setting sector size failed.\n"));

	return (retval);
}


/*
 * switch Toshiba/DEC and HP drives from/to cdda density
 */
void
EnableCddaModeSelect(scgp, fAudioMode, uSectorsize)
	SCSI *scgp;
	int fAudioMode;
	unsigned uSectorsize;
{
	/*
	 * reserved, Medium type=0, Dev spec Parm = 0,
	 * block descriptor len 0 oder 8,
	 * Density (cd format)
	 * (0=YellowBook, XA Mode 2=81h,
	 * XA Mode1=83h and raw audio via SCSI=82h),
	 * # blks msb, #blks, #blks lsb, reserved,
	 * blocksize, blocklen msb, blocklen lsb,
	 */

	/*
	 * MODE_SELECT, page = SCSI-2 save page disabled, reserved, reserved,
	 * parm list len, flags
	 */
static unsigned char mode [4 + 8] = {
	/*
	 * mode section
	 */
	0,
	0, 0,
	8,		/* Block Descriptor Length */
	/*
	 * block descriptor
	 */
	0,		/* Density Code */
	0, 0, 0,	/* # of Blocks */
	0,		/* reserved */
	0, 0, 0};	/* Blocklen */

	if (orgmode4 == 0 && fAudioMode) {
		if (0 == get_orig_sectorsize(scgp,
					&orgmode4, &orgmode10, &orgmode11)) {
			/*
			 * cannot retrieve density, sectorsize
			 */
			orgmode10 = (CD_FRAMESIZE >> 8L);
			orgmode11 = (CD_FRAMESIZE & 0xFF);
		}
	}

	if (fAudioMode) {
		/*
		 * prepare to read audio cdda
		 */
		mode [4] = density;		 /* cdda density	*/
		mode [10] = (uSectorsize >> 8L); /* block length "msb"	*/
		mode [11] = (uSectorsize & 0xFF); /* block length "lsb"	*/
	} else {
		/*
		 * prepare to read cds in the previous mode
		 */
		mode [4] = orgmode4;	/* 0x00;	    normal density */
		mode [10] = orgmode10;	/* (CD_FRAMESIZE >> 8L); block length "msb" */
		mode [11] = orgmode11;	/* (CD_FRAMESIZE & 0xFF); block length lsb */
	}

	if (scgp->verbose) {
		fprintf(stderr,
		_("\nset density/sector size (EnableCddaModeSelect)...\n"));
	}
	/*
	 * do the scsi cmd
	 */
	if (mode_select(scgp, mode, 12, 0, scgp->inq->data_format >= 2) < 0)
		errmsgno(EX_BAD, _("Audio mode switch failed.\n"));
}


/*
 * read CD Text information from the table of contents
 */
void
ReadTocTextSCSIMMC(scgp)
	SCSI	*scgp;
{
	short	datalength;

#if 1
	/*
	 * READTOC, MSF, format, res, res, res, Start track/session, len msb,
	 * len lsb, control
	 */
		unsigned char	*p = (unsigned char *)global.buf;
	register struct	scg_cmd	*scmd = scgp->scmd;

	scgp->silent++;
	unit_ready(scgp);
	scgp->silent--;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)global.buf;
	scmd->size = 4;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x43;		/* Read TOC command */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = 5;		/* format field */
	scmd->cdb.g1_cdb.res6 = 0;	/* track/session is reserved */
	g1_cdblen(&scmd->cdb.g1_cdb, 4);

	scgp->silent++;
	if (scgp->verbose)
		fprintf(stderr, _("\nRead TOC CD Text size ..."));

	scgp->cmdname = "read toc size (text)";

	if (scg_cmd(scgp) < 0) {
		scgp->silent--;
		if (global.quiet != 1) {
			errmsgno(EX_BAD,
			_("Read TOC CD Text failed (probably not supported).\n"));
		}
		p[0] = p[1] = '\0';
		return;
	}
	scgp->silent--;

	datalength  = (p[0] << 8) | (p[1]);
	if (datalength <= 2)
		return;
	datalength += 2;
	if ((datalength) > global.bufsize)
		datalength = global.bufsize;

	scgp->silent++;
	unit_ready(scgp);
	scgp->silent--;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)global.buf;
	scmd->size = datalength;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x43;		/* Read TOC command */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = 5;		/* format field */
	scmd->cdb.g1_cdb.res6 = 0;	/* track/session is reserved */
	g1_cdblen(&scmd->cdb.g1_cdb, datalength);

	scgp->silent++;
	if (scgp->verbose) {
		fprintf(stderr,
		_("\nRead TOC CD Text data (length %d)..."), (int)datalength);
	}
	scgp->cmdname = "read toc data (text)";

	if (scg_cmd(scgp) < 0) {
		if (global.quiet != 1) {
			errmsgno(EX_BAD,
			_("Read TOC CD Text data failed (probably not supported).\n"));
		}
		p[0] = p[1] = '\0';
		unit_ready(scgp);
		scgp->silent--;
		return;
	}
	scgp->silent--;
#else
	{
	FILE	*fp;
	int	read_;

	/*fp = fopen("PearlJam.cdtext", "rb");*/
	/*fp = fopen("celine.cdtext", "rb");*/
	fp = fopen("japan.cdtext", "rb");
	if (fp == NULL) {
		errmsg(_("Cannot open '%s'.\n"), "japan.cdtext");
		return;
	}
	fillbytes(global.buf, global.bufsize, '\0');
	read_ = fread(global.buf, 1, global.bufsize, fp);
	fprintf(stderr, _("read %d bytes. sizeof(buffer)=%u\n"),
			read_, global.bufsize);
	datalength  = ((global.buf[0] & 0xFF) << 8) | (global.buf[1] & 0xFF) + 2;
	fclose(fp);
	}
#endif
}

/*
 * read the full TOC
 */
static unsigned
ReadFullTOCSony(scgp)
	SCSI	*scgp;
{
	/*
	 * READTOC, MSF, format, res, res, res, Start track/session, len msb,
	 * len lsb, control
	 */
	register struct	scg_cmd	*scmd = scgp->scmd;
		unsigned	tracks = 99;

	scgp->silent++;
	unit_ready(scgp);
	scgp->silent--;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)bufferTOC;
	scmd->size = 4 + (3 + tracks + 6) * 11;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x43;		/* Read TOC command */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.res6 = 1;		/* session */
	g1_cdblen(&scmd->cdb.g1_cdb, 4 + (3 + tracks + 6) * 11);
	scmd->cdb.g1_cdb.vu_97 = 1;		/* format */

	scgp->silent++;
	if (scgp->verbose)
		fprintf(stderr, _("\nRead Full TOC Sony ..."));

	scgp->cmdname = "read full toc sony";

	if (scg_cmd(scgp) < 0) {
		if (global.quiet != 1) {
			errmsgno(EX_BAD,
			_("Read Full TOC Sony failed (probably not supported).\n"));
		}
		unit_ready(scgp);
		scgp->silent--;
		return (0);
	}
	scgp->silent--;

	return ((unsigned)((bufferTOC[0] << 8) | bufferTOC[1]));
}

struct msf_address {
	unsigned char	mins;
	unsigned char	secs;
	unsigned char	frame;
};

struct zmsf_address {
	unsigned char	zero;
	unsigned char	mins;
	unsigned char	secs;
	unsigned char	frame;
};

#ifdef	WARN_FULLTOC
static unsigned	lba __PR((struct msf_address *ad));

static unsigned
lba(ad)
	struct msf_address	*ad;
{
	return (ad->mins*60*75 + ad->secs*75 + ad->frame);
}
#endif

static unsigned	dvd_lba __PR((struct zmsf_address *ad));

static unsigned
dvd_lba(ad)
	struct zmsf_address	*ad;
{
	return (ad->zero*1053696 + ad->mins*60*75 + ad->secs*75 + ad->frame);
}

struct tocdesc_old {
	unsigned char	session;
	unsigned char	adrctl;
	unsigned char	tno;
	unsigned char	point;
	struct msf_address	adr1;
	struct zmsf_address	padr2;
};

struct tocdesc {
	unsigned char	session;
	unsigned char	adrctl;
	unsigned char	tno;
	unsigned char	point;

	unsigned char	mins;
	unsigned char	secs;
	unsigned char	frame;
	unsigned char	zero;
	unsigned char	pmins;
	unsigned char	psecs;
	unsigned char	pframe;
};

#if 0
struct outer_old {
	unsigned char	len_msb;
	unsigned char	len_lsb;
	unsigned char	first_track;
	unsigned char	last_track;
	struct tocdesc_old	ent[1];
};
#endif

struct outer {
	unsigned char	len_msb;
	unsigned char	len_lsb;
	unsigned char	first_track;
	unsigned char	last_track;
	struct tocdesc	ent[1];
};

/*
 * Do address computation and return the address of "struct tocdesc" for
 * track #i.
 *
 * As struct tocdesc is 11 bytes long, this structure is tail padded to
 * 12 bytes by MC-680x0 compilers, so we cannot give this task to the compiler.
 */
#define	toc_addr(op, i)	((struct tocdesc *)(((char *)&((op)->ent[0])) + (i)*11))

static unsigned long	first_session_leadout = 0;

static unsigned	collect_tracks __PR((struct outer *po, unsigned entries,
							BOOL bcd_flag));

static unsigned
collect_tracks(po, entries, bcd_flag)
	struct outer	*po;
	unsigned	entries;
	BOOL		bcd_flag;
{
	unsigned	tracks = 0;
	int		i;
	unsigned	session;
	unsigned	last_start;
	unsigned	leadout_start_orig;
	unsigned	leadout_start;
	unsigned	max_leadout = 0;
	struct tocdesc	*ep;

#ifdef	DEBUG_FULLTOC
	for (i = 0; i < entries; i++) {
		fprintf(stderr,
		"%3d: %d %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			i,
			bufferTOC[4+0 + (i * 11)],
			bufferTOC[4+1 + (i * 11)],
			bufferTOC[4+2 + (i * 11)],
			bufferTOC[4+3 + (i * 11)],
			bufferTOC[4+4 + (i * 11)],
			bufferTOC[4+5 + (i * 11)],
			bufferTOC[4+6 + (i * 11)],
			bufferTOC[4+7 + (i * 11)],
			bufferTOC[4+8 + (i * 11)],
			bufferTOC[4+9 + (i * 11)],
			bufferTOC[4+10 + (i * 11)]);
	}
#endif
	/*
	 * reformat to standard toc format
	 */
	bufferTOC[2] = 0;
	bufferTOC[3] = 0;
	session = 0;
	last_start = 0;
	leadout_start_orig = 0;
	leadout_start = 0;

	for (i = 0; i < entries; i++) {
		ep = toc_addr(po, i);	/* Get struct tocdesc * for Track #i */

#ifdef	WARN_FULLTOC
		if (ep->tno != 0) {
			fprintf(stderr, _("entry %d, tno is not 0: %d!\n"),
				i, ep->tno);
		}
#endif
		if (bcd_flag) {
			ep->session	= from_bcd(ep->session);
			ep->mins	= from_bcd(ep->mins);
			ep->secs	= from_bcd(ep->secs);
			ep->frame	= from_bcd(ep->frame);
			ep->pmins	= from_bcd(ep->pmins);
			ep->psecs	= from_bcd(ep->psecs);
			ep->pframe	= from_bcd(ep->pframe);
		}
		switch (ep->point) {
		case	0xa0:

			/*
			 * check if session is monotonous increasing
			 */
			if (session+1 == ep->session) {
				session = ep->session;
			}
#ifdef	WARN_FULLTOC
			else {
				fprintf(stderr,
				_("entry %d, session anomaly %d != %d!\n"),
					i, session+1, ep->session);
			}
			/*
			 * check the adrctl field
			 */
			if (0x10 != (ep->adrctl & 0x10)) {
				fprintf(stderr,
				_("entry %d, incorrect adrctl field %x!\n"),
					i, ep->adrctl);
			}
#endif
			/*
			 * first track number
			 */
			if (bufferTOC[2] < ep->pmins &&
			    bufferTOC[3] < ep->pmins) {
				bufferTOC[2] = ep->pmins;
			}
#ifdef	WARN_FULLTOC
			else
				fprintf(stderr,
_("entry %d, session %d: start tracknumber anomaly: %d <= %d,%d(last)!\n"),
					i, session, ep->pmins,
					bufferTOC[2], bufferTOC[3]);
#endif
			break;

		case	0xa1:
#ifdef	WARN_FULLTOC
			/*
			 * check if session is constant
			 */
			if (session != ep->session) {
				fprintf(stderr,
				_("entry %d, session anomaly %d != %d!\n"),
					i, session, ep->session);
			}

			/*
			 * check the adrctl field
			 */
			if (0x10 != (ep->adrctl & 0x10)) {
				fprintf(stderr,
				_("entry %d, incorrect adrctl field %x!\n"),
					i, ep->adrctl);
			}
#endif
			/*
			 * last track number
			 */
			if (bufferTOC[2] <= ep->pmins &&
			    bufferTOC[3] < ep->pmins) {
				bufferTOC[3] = ep->pmins;
			}
#ifdef	WARN_FULLTOC
			else
				fprintf(stderr,
_("entry %d, session %d: end tracknumber anomaly: %d <= %d,%d(last)!\n"),
					i, session, ep->pmins,
					bufferTOC[2], bufferTOC[3]);
#endif
			break;

		case	0xa2:
#ifdef	WARN_FULLTOC
			/*
			 * check if session is constant
			 */
			if (session != ep->session) {
				fprintf(stderr,
				_("entry %d, session anomaly %d != %d!\n"),
					i, session, ep->session);
			}

			/*
			 * check the adrctl field
			 */
			if (0x10 != (ep->adrctl & 0x10)) {
				fprintf(stderr,
				_("entry %d, incorrect adrctl field %x!\n"),
				i, ep->adrctl);
			}
#endif
			/*
			 * register leadout position
			 */
			{
			unsigned	leadout_start_tmp =
						dvd_lba((struct zmsf_address *)&ep->zero);

			if (first_session_leadout == 0) {
				first_session_leadout = leadout_start_tmp
							- 150;
			}
			if (leadout_start_tmp > leadout_start) {
				leadout_start_orig = leadout_start_tmp;
				leadout_start = leadout_start_tmp;
			}
#ifdef	WARN_FULLTOC
			else
				fprintf(stderr,
				_("entry %d, leadout position anomaly %u!\n"),
					i, leadout_start_tmp);
#endif
		}
			break;

		case	0xb0:
#ifdef	WARN_FULLTOC
			/*
			 * check if session is constant
			 */
			if (session != ep->session) {
				fprintf(stderr,
				_("entry %d, session anomaly %d != %d!\n"),
					i, session, ep->session);
			}

			/*
			 * check the adrctl field
			 */
			if (0x50 != (ep->adrctl & 0x50)) {
				fprintf(stderr,
				_("entry %d, incorrect adrctl field %x!\n"),
					i, ep->adrctl);
			}

			/*
			 * check the next program area
			 */
			if (lba((struct msf_address *)&ep->mins) < 6750 + leadout_start) {
				fprintf(stderr,
_("entry %d, next program area %u < leadout_start + 6750 = %u!\n"),
					i, lba((struct msf_address *)&ep->mins),
					6750 + leadout_start);
			}

			/*
			 * check the maximum leadout_start
			 */
			if (max_leadout != 0 && dvd_lba((struct zmsf_address *)&ep->zero) !=
								max_leadout) {
				fprintf(stderr,
_("entry %d, max leadout_start %u != last max_leadout_start %u!\n"),
					i, dvd_lba((struct zmsf_address *)&ep->zero),
					max_leadout);
			}
#endif
			if (max_leadout == 0)
				max_leadout = dvd_lba((struct zmsf_address *)&ep->zero);

			break;
		case	0xb1:
		case	0xb2:
		case	0xb3:
		case	0xb4:
		case	0xb5:
		case	0xb6:
			break;
		case	0xc0:
		case	0xc1:
			break;
		default:
			/*
			 * check if session is constant
			 */
			if (session != ep->session) {
#ifdef	WARN_FULLTOC
				fprintf(stderr,
				_("entry %d, session anomaly %d != %d!\n"),
					i, session, ep->session);
#endif
				continue;
			}

			/*
			 * check tno
			 */
			if (bcd_flag)
				ep->point = from_bcd(ep->point);

			if (ep->point < bufferTOC[2] ||
			    ep->point > bufferTOC[3]) {
#ifdef	WARN_FULLTOC
				fprintf(stderr,
				_("entry %d, track number anomaly %d - %d - %d!\n"),
					i, bufferTOC[2], ep->point,
					bufferTOC[3]);
#endif
			} else {
				/*
				 * check start position
				 */
				unsigned trackstart = dvd_lba((struct zmsf_address *)&ep->zero);

				/*
				 * correct illegal leadouts
				 */
				if (leadout_start < trackstart) {
					leadout_start = trackstart+1;
				}
				if (trackstart < last_start ||
				    trackstart >= leadout_start) {
#ifdef	WARN_FULLTOC
					fprintf(stderr,
_("entry %d, track %d start position anomaly %d - %d - %d!\n"),
						i, ep->point,
						last_start,
						trackstart, leadout_start);
#endif
				} else {
					last_start = trackstart;
					memcpy(toc_addr(po, tracks), ep, 11);
					tracks++;
				}
			}
		}	/* switch */
	}	/* for */

	/*
	 * patch leadout track
	 */
	ep = toc_addr(po, tracks);	/* Get struct tocdesc * for lead-out */
	ep->session = session;
	ep->adrctl = 0x10;
	ep->tno = 0;
	ep->point = 0xAA;
	ep->mins = 0;
	ep->secs = 0;
	ep->frame = 0;
	ep->zero = leadout_start_orig / (1053696);
	ep->pmins = (leadout_start_orig / (60*75)) % 100;
	ep->psecs = (leadout_start_orig / 75) % 60;
	ep->pframe = leadout_start_orig % 75;
	tracks++;

	/*
	 * length
	 */
	bufferTOC[0] = ((tracks * 8) + 2) >> 8;
	bufferTOC[1] = ((tracks * 8) + 2) & 0xff;


	/*
	 * reformat 11 byte blocks to 8 byte entries
	 */

	/*
	 * 1: Session	\	/	reserved
	 * 2: adr ctrl	|	|	adr ctrl
	 * 3: TNO	|	|	track number
	 * 4: Point	|	|	reserved
	 * 5: Min	+-->----+	0
	 * 6: Sec	|	|	Min
	 * 7: Frame	|	|	Sec
	 * 8: Zero	|	\	Frame
	 * 9: PMin	|
	 * 10: PSec	|
	 * 11: PFrame	/
	 */
	for (i = 0; i < tracks; i++) {
		bufferTOC[4+0 + (i << 3)] = 0;
		bufferTOC[4+1 + (i << 3)] = bufferTOC[4+1 + (i*11)];
		bufferTOC[4+1 + (i << 3)] = (bufferTOC[4+1 + (i << 3)] >> 4) |
					    (bufferTOC[4+1 + (i << 3)] << 4);
		bufferTOC[4+2 + (i << 3)] = bufferTOC[4+3 + (i*11)];
		bufferTOC[4+3 + (i << 3)] = 0;
		bufferTOC[4+4 + (i << 3)] = bufferTOC[4+7 + (i*11)];
		bufferTOC[4+5 + (i << 3)] = bufferTOC[4+8 + (i*11)];
		bufferTOC[4+6 + (i << 3)] = bufferTOC[4+9 + (i*11)];
		bufferTOC[4+7 + (i << 3)] = bufferTOC[4+10 + (i*11)];
#ifdef	DEBUG_FULLTOC
		fprintf(stderr, "%02x %02x %02x %02x %02x %02x\n",
			bufferTOC[4+ 1 + i*8],
			bufferTOC[4+ 2 + i*8],
			bufferTOC[4+ 4 + i*8],
			bufferTOC[4+ 5 + i*8],
			bufferTOC[4+ 6 + i*8],
			bufferTOC[4+ 7 + i*8]);
#endif
	}

	TOC_entries(tracks, NULL, bufferTOC+4, 0);
	return (tracks);
}

/*
 * read the table of contents from the cd and fill the TOC array
 */
unsigned
ReadTocSony(scgp)
	SCSI	*scgp;
{
	unsigned	tracks = 0;
	unsigned	return_length;

	struct outer	*po = (struct outer *)bufferTOC;

	return_length = ReadFullTOCSony(scgp);

	/*
	 * Check if the format was understood
	 */
	if ((return_length & 7) == 2 &&
	    (bufferTOC[3] - bufferTOC[2]) == (return_length >> 3)) {
		/*
		 * The extended format seems not be understood, fallback to
		 * the classical format.
		 */
		return (ReadTocSCSI(scgp));
	}

	tracks = collect_tracks(po, ((return_length - 2) / 11), TRUE);

	return (--tracks);	/* without lead-out */
}

/*
 * read the start of the lead-out from the first session TOC
 */
unsigned
ReadFirstSessionTOCSony(scgp)
	SCSI	*scgp;
{
	unsigned	return_length;

	if (first_session_leadout != 0)
		return (first_session_leadout);

	return_length = ReadFullTOCSony(scgp);
	if (return_length >= 4 + (3 * 11) -2) {
		unsigned	off;

		/*
		 * We want the entry with POINT = 0xA2,
		 * which has the start position
		 * of the first session lead out
		 */
		off = 4 + 2 * 11 + 3;
		if (bufferTOC[off-3] == 1 && bufferTOC[off] == 0xA2) {
			unsigned	retval;

			off = 4 + 2 * 11 + 8;
			retval = bufferTOC[off] >> 4;
			retval *= 10; retval += bufferTOC[off] & 0xf;
			retval *= 60;
			off++;
			retval += 10 * (bufferTOC[off] >> 4) +
					(bufferTOC[off] & 0xf);
			retval *= 75;
			off++;
			retval += 10 * (bufferTOC[off] >> 4) +
					(bufferTOC[off] & 0xf);
			retval -= 150;

			return (retval);
		}
	}
	return (0);
}

/*
 * read the full TOC
 */
static unsigned
ReadFullTOCMMC(scgp)
	SCSI	*scgp;
{
	/*
	 * READTOC, MSF, format, res, res, res, Start track/session, len msb,
	 * len lsb, control
	 */
	register struct	scg_cmd	*scmd = scgp->scmd;
	int	len;

	scgp->silent++;
	unit_ready(scgp);
	scgp->silent--;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)bufferTOC;
	scmd->size = 0;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x43;		/* Read TOC command */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = 2;		/* format */
	scmd->cdb.g1_cdb.res6 = 1;		/* session */
	g1_cdblen(&scmd->cdb.g1_cdb, 0);

	scgp->silent++;
	if (scgp->verbose)
		fprintf(stderr, _("\nRead Full TOC MMC..."));

	scgp->cmdname = "read full toc mmc";

	scmd->size = 4;
	g1_cdblen(&scmd->cdb.g1_cdb, 4);
	if (scg_cmd(scgp) < 0) {
		if (global.quiet != 1) {
			errmsgno(EX_BAD,
			_("Read Full TOC MMC failed (probably not supported).\n"));
		}
	/* XXX was ist das ??? */
#ifdef	B_BEOS_VERSION
#else
		scgp->silent--;
		return (0);
#endif
	}
	len = (unsigned)((bufferTOC[0] << 8) | bufferTOC[1]) + 2;
	/*
	 * XXX there is a bug in some ASPI versions that
	 * XXX cause a hang with odd transfer lengths.
	 * XXX We should workaround the problem where it exists
	 * XXX but the problem may exist elsewhere too.
	 */
	if (len & 1)
		len++;
	scmd->size = len;
	g1_cdblen(&scmd->cdb.g1_cdb, len);

	if (scg_cmd(scgp) < 0) {
		unit_ready(scgp);
		scgp->silent--;
		return (0);
	}

	scgp->silent--;

	return ((unsigned)((bufferTOC[0] << 8) | bufferTOC[1]));
}

/*
 * read the start of the lead-out from the first session TOC
 */
unsigned
ReadFirstSessionTOCMMC(scgp)
	SCSI		*scgp;
{
	unsigned	off;
	unsigned	return_length;

	if (first_session_leadout != 0)
		return (first_session_leadout);

	return_length = ReadFullTOCMMC(scgp);

	/*
	 * We want the entry with POINT = 0xA2, which has the start position
	 * of the first session lead out
	 */
	off = 4 + 3;
	while (off < return_length && bufferTOC[off] != 0xA2) {
		off += 11;
	}
	if (off < return_length) {
		off += 5;
		return ((bufferTOC[off]*60 + bufferTOC[off+1])*75 +
						bufferTOC[off+2] - 150);
	}
	return (0);
}

/*
 * read the table of contents from the cd and fill the TOC array
 */
unsigned
ReadTocMMC(scgp)
	SCSI		*scgp;
{
	unsigned	tracks = 0;
	unsigned	return_length;

	struct outer *po = (struct outer *)bufferTOC;

	return_length = ReadFullTOCMMC(scgp);
	if (return_length - 2 < 4*11 || ((return_length - 2) % 11) != 0)
		return (ReadTocSCSI(scgp));

	tracks = collect_tracks(po, ((return_length - 2) / 11), FALSE);
	return (--tracks);		/* without lead-out */
}

/*
 * read the table of contents from the cd and fill the TOC array
 */
unsigned
ReadTocSCSI(scgp)
	SCSI		*scgp;
{
	unsigned	tracks;
	int		result;
	unsigned char	bufferTOCMSF[CD_FRAMESIZE];

	/*
	 * first read the first and last track number
	 */
	/*
	 * READTOC, MSF format flag, res, res, res, res, Start track, len msb,
	 * len lsb, flags
	 */
	register struct	scg_cmd	*scmd = scgp->scmd;

	scgp->silent++;
	unit_ready(scgp);
	scgp->silent--;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)bufferTOC;
	scmd->size = 4;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x43;		/* read TOC command */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.res6 = 1;		/* start track */
	g1_cdblen(&scmd->cdb.g1_cdb, 4);

	if (scgp->verbose)
	fprintf(stderr, _("\nRead TOC size (standard)..."));

	/*
	 * do the scsi cmd (read table of contents)
	 */

	scgp->cmdname = "read toc size";
	if (scg_cmd(scgp) < 0)
		FatalError(EX_BAD, _("Read TOC size failed.\n"));

	scgp->silent++;
	unit_ready(scgp);
	scgp->silent--;

	tracks = ((bufferTOC [3]) - bufferTOC [2] + 2);
	if (tracks > MAXTRK)
		return (0);
	if (tracks == 0)
		return (0);

	memset(bufferTOCMSF, 0, sizeof (bufferTOCMSF));
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)bufferTOCMSF;
	scmd->size = 4 + tracks * 8;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x43;		/* read TOC command */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.res = 1;		/* MSF format */
	scmd->cdb.g1_cdb.res6 = 1;		/* start track */
	g1_cdblen(&scmd->cdb.g1_cdb, 4 + tracks * 8);

	if (scgp->verbose)
		fprintf(stderr, _("\nRead TOC tracks (standard MSF)..."));
	/*
	 * do the scsi cmd (read table of contents)
	 */
	scgp->cmdname = "read toc tracks ";
	result = scg_cmd(scgp);

	if (result < 0) {
		/*
		 * MSF format did not succeeded
		 */
		memset(bufferTOCMSF, 0, sizeof (bufferTOCMSF));

		scgp->silent++;
		unit_ready(scgp);
		scgp->silent--;
	} else {
		int	i;

		for (i = 0; i < tracks; i++) {
			bufferTOCMSF[4+1 + (i << 3)] = (bufferTOCMSF[4+1 +
							(i << 3)] >> 4) |
					(bufferTOCMSF[4+1 + (i << 3)] << 4);
#if	0
			fprintf(stderr,
			"MSF %d %02x %02x %02x %02x %02x %02x %02x %02x\n",
				i,
				bufferTOCMSF[4+0 + (i * 8)],
				bufferTOCMSF[4+1 + (i * 8)],
				bufferTOCMSF[4+2 + (i * 8)],
				bufferTOCMSF[4+3 + (i * 8)],
				bufferTOCMSF[4+4 + (i * 8)],
				bufferTOCMSF[4+5 + (i * 8)],
				bufferTOCMSF[4+6 + (i * 8)],
				bufferTOCMSF[4+7 + (i * 8)]);
#endif
		}
	}

	/*
	 * LBA format for cd burners like Philips CD-522
	 */
	scgp->silent++;
	unit_ready(scgp);
	scgp->silent--;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)bufferTOC;
	scmd->size = 4 + tracks * 8;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x43;		/* read TOC command */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.res = 0;		/* LBA format */
	scmd->cdb.g1_cdb.res6 = 1;		/* start track */
	g1_cdblen(&scmd->cdb.g1_cdb, 4 + tracks * 8);

	if (scgp->verbose)
		fprintf(stderr, _("\nRead TOC tracks (standard LBA)..."));
	/*
	 * do the scsi cmd (read table of contents)
	 */

	scgp->cmdname = "read toc tracks ";
	if (scg_cmd(scgp) < 0) {
		FatalError(EX_BAD, _("Read TOC tracks (lba) failed.\n"));
	}
	scgp->silent++;
	unit_ready(scgp);
	scgp->silent--;

	{
	int	i;

	for (i = 0; i < tracks; i++) {
		bufferTOC[4+1 + (i << 3)] = (bufferTOC[4+1 + (i << 3)] >> 4) |
					    (bufferTOC[4+1 + (i << 3)] << 4);
#if	0
		fprintf(stderr,
			"LBA %d %02x %02x %02x %02x %02x %02x %02x %02x\n",
			i,
			bufferTOC[4+0 + (i * 8)],
			bufferTOC[4+1 + (i * 8)],
			bufferTOC[4+2 + (i * 8)],
			bufferTOC[4+3 + (i * 8)],
			bufferTOC[4+4 + (i * 8)],
			bufferTOC[4+5 + (i * 8)],
			bufferTOC[4+6 + (i * 8)],
			bufferTOC[4+7 + (i * 8)]);
#endif
	}
	}
	TOC_entries(tracks, bufferTOC+4, bufferTOCMSF+4, result);
	return (--tracks);		/* without lead-out */
}

/* ---------------- Read methods ------------------------------ */

/*
 * Read max. SectorBurst of cdda sectors to buffer
 * via standard SCSI-2 Read(10) command
 */
static int ReadStandardLowlevel __PR((SCSI *scgp, UINT4 *p, unsigned lSector,
				unsigned SectorBurstVal, unsigned secsize));

static int
ReadStandardLowlevel(scgp, p, lSector, SectorBurstVal, secsize)
	SCSI	*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
	unsigned	secsize;
{
	/*
	 * READ10, flags, block1 msb, block2, block3, block4 lsb, reserved,
	 * transfer len msb, transfer len lsb, block addressing mode
	 */
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)p;
	scmd->size = SectorBurstVal * secsize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x28;		/* read 10 command */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
	g1_cdbaddr(&scmd->cdb.g1_cdb, lSector);
	g1_cdblen(&scmd->cdb.g1_cdb, SectorBurstVal);

	if (scgp->verbose) {
		fprintf(stderr, _("\nReadStandard10 %s (%u)..."),
			secsize > 2048 ? "CDDA" : "CD_DATA", secsize);
	}
	scgp->cmdname = "ReadStandard10";

	if (scg_cmd(scgp))
		return (0);

	/*
	 * has all or something been read?
	 */
	return (SectorBurstVal - scg_getresid(scgp)/secsize);
}


int
ReadStandard(scgp, p, lSector, SectorBurstVal)
	SCSI	*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	return (ReadStandardLowlevel(scgp, p, lSector, SectorBurstVal,
							CD_FRAMESIZE_RAW));
}

int
ReadStandardData(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	return (ReadStandardLowlevel(scgp, p, lSector, SectorBurstVal,
							CD_FRAMESIZE));
}

/*
 * Read max. SectorBurst of cdda sectors to buffer
 * via vendor-specific ReadCdda(10) command
 */
int
ReadCdda10(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	/*
	 * READ10, flags, block1 msb, block2, block3, block4 lsb, reserved,
	 * transfer len msb, transfer len lsb, block addressing mode
	 */
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)p;
	scmd->size = SectorBurstVal*CD_FRAMESIZE_RAW;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xd4;		/* Read audio command */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
	g1_cdbaddr(&scmd->cdb.g1_cdb, lSector);
	g1_cdblen(&scmd->cdb.g1_cdb, SectorBurstVal);
	if (scgp->verbose)
		fprintf(stderr, _("\nReadNEC10 CDDA..."));

	scgp->cmdname = "Read10 NEC";

	if (scg_cmd(scgp))
		return (0);

	/*
	 * has all or something been read?
	 */
	return (SectorBurstVal - scg_getresid(scgp)/CD_FRAMESIZE_RAW);
}


/*
 * Read max. SectorBurst of cdda sectors to buffer
 * via vendor-specific ReadCdda(12) command
 */
int
ReadCdda12(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)p;
	scmd->size = SectorBurstVal*CD_FRAMESIZE_RAW;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xd8;		/* read audio command */
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g5_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
	g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
	g5_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);

	if (scgp->verbose)
		fprintf(stderr, _("\nReadSony12 CDDA..."));

	scgp->cmdname = "Read12";

	if (scg_cmd(scgp) < 0) {
		scgp->silent++;
		unit_ready(scgp);
		scgp->silent--;
		return (0);
	}

	/*
	 * has all or something been read?
	 */
	return (SectorBurstVal - scg_getresid(scgp)/CD_FRAMESIZE_RAW);
}

/*
 * Read max. SectorBurst of cdda sectors to buffer
 * via vendor-specific ReadCdda(12) command
 */
int
ReadCdda12_C2(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)p;
	scmd->size = SectorBurstVal*CD_FRAMESIZE_RAWER;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xd8;		/* read audio command */
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g5_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
	scmd->cdb.g5_cdb.res10 = 0x04;		/* With C2 errors */
	g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
	g5_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);

	if (scgp->verbose)
		fprintf(stderr, _("\nReadSony12 CDDA C2..."));

	scgp->cmdname = "Read12 C2";

	if (scg_cmd(scgp) < 0) {
		scgp->silent++;
		unit_ready(scgp);
		scgp->silent--;
		return (0);
	}

	/*
	 * has all or something been read?
	 */
	return (SectorBurstVal - scg_getresid(scgp)/CD_FRAMESIZE_RAWER);
}

/*
 * Read max. SectorBurst of cdda sectors to buffer
 * via vendor-specific ReadCdda(12) command
 */
/*
 * It uses a 12 Byte CDB with 0xd4 as opcode, the start sector is coded as
 * normal and the number of sectors is coded in Byte 8 and 9 (begining with 0).
 */
int
ReadCdda12Matsushita(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)p;
	scmd->size = SectorBurstVal*CD_FRAMESIZE_RAW;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xd4;		/* read audio command */
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g5_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
	g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
	g5_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);

	if (scgp->verbose)
			fprintf(stderr, _("\nReadMatsushita12 CDDA..."));

	scgp->cmdname = "Read12Matsushita";

	if (scg_cmd(scgp))
		return (0);

	/*
	 * has all or something been read?
	 */
	return (SectorBurstVal - scg_getresid(scgp)/CD_FRAMESIZE_RAW);
}

/*
 * Read max. SectorBurst of cdda sectors to buffer
 * via MMC standard READ CD command
 */
int
ReadCddaMMC12(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	register struct	scg_cmd	*scmd;

	scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)p;
	scmd->size = SectorBurstVal*CD_FRAMESIZE_RAW;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xbe;		/* read cd command */
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g5_cdb.res = 1 << 1; /* expected sector type field CDDA */
	g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
	g5x_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);
	scmd->cdb.g5_cdb.count[3] = 1 << 4;	/* User data */

	if (scgp->verbose)
		fprintf(stderr, _("\nReadMMC12 CDDA..."));

	scgp->cmdname = "ReadCD MMC 12";

	if (scg_cmd(scgp) < 0) {
		scgp->silent++;
		unit_ready(scgp);
		scgp->silent--;
		return (0);
	}

	/*
	 * has all or something been read?
	 */
	return (SectorBurstVal - scg_getresid(scgp)/CD_FRAMESIZE_RAW);
}

/*
 * Read max. SectorBurst of cdda sectors to buffer
 * via MMC standard READ CD command
 */
int
ReadCddaMMC12_C2(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	register struct	scg_cmd	*scmd;

	scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)p;
	scmd->size = SectorBurstVal*CD_FRAMESIZE_RAWER;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xbe;		/* read cd command */
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g5_cdb.res = 1 << 1; /* expected sector type field CDDA */
	g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
	g5x_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);
	scmd->cdb.g5_cdb.count[3] = 1 << 4;	/* User data */
	scmd->cdb.g5_cdb.count[3] |= 1 << 1;	/* C2 */

	if (scgp->verbose)
		fprintf(stderr, _("\nReadMMC12 CDDA C2..."));

	scgp->cmdname = "ReadCD MMC 12 C2";

	scgp->silent++;
	if (scg_cmd(scgp) < 0) {
		/*
		 * if the command is not available, disable this method
		 * by setting the ReadCdRom_C2 pointer to NULL.
		 */
		if (scg_sense_key(scgp) == 0x05 &&
			scg_sense_code(scgp) == 0x24 &&
			scg_sense_qual(scgp) == 0x00) {
			ReadCdRom_C2 = NULL;
		}
		unit_ready(scgp);

		scgp->silent--;
		return (0);
	}
	scgp->silent--;

	/*
	 * has all or something been read?
	 */
	return (SectorBurstVal - scg_getresid(scgp)/CD_FRAMESIZE_RAWER);
}

int
ReadCddaFallbackMMC(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
static int	ReadCdda12_unknown = 0;
	int	retval = -999;

	scgp->silent++;
	if (ReadCdda12_unknown ||
	    ((retval = ReadCdda12(scgp, p, lSector, SectorBurstVal)) <= 0)) {
		/*
		 * if the command is not available, use the regular
		 * MMC ReadCd
		 */
		if (retval <= 0 && scg_sense_key(scgp) == 0x05) {
			ReadCdda12_unknown = 1;
		}
		scgp->silent--;
		ReadCdRom = ReadCddaMMC12;
		ReadCdRomSub = ReadCddaSubMMC12;
		return (ReadCddaMMC12(scgp, p, lSector, SectorBurstVal));
	}
	scgp->silent--;
	return (retval);
}

int
ReadCddaFallbackMMC_C2(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
static int	ReadCdda12_C2_unknown = 0;
	int	retval = -999;

	scgp->silent++;
	if (ReadCdda12_C2_unknown ||
	    ((retval = ReadCdda12_C2(scgp, p, lSector, SectorBurstVal)) <= 0)) {
		/*
		 * if the command is not available, use the regular
		 * MMC ReadCd
		 */
		if (retval <= 0 && scg_sense_key(scgp) == 0x05) {
			ReadCdda12_C2_unknown = 1;
		}
		scgp->silent--;
		ReadCdRom_C2 = ReadCddaMMC12_C2;
		return (ReadCddaMMC12_C2(scgp, p, lSector, SectorBurstVal));
	}
	scgp->silent--;
	return (retval);
}

int
ReadCddaNoFallback_C2(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
static int	ReadCdda12_C2_unknown = 0;
	int	retval = -999;

	scgp->silent++;
	if (ReadCdda12_C2_unknown ||
	    ((retval = ReadCdda12_C2(scgp, p, lSector, SectorBurstVal)) <= 0)) {
		/*
		 * if the command is not available, disable this method
		 * by setting the ReadCdRom_C2 pointer to NULL.
		 */
		if (retval <= 0 && scg_sense_key(scgp) == 0x05 &&
			scg_sense_code(scgp) == 0x24 &&
			scg_sense_qual(scgp) == 0x00) {
			ReadCdda12_C2_unknown = 1;
		}
		ReadCdRom_C2 = NULL;
	}
	scgp->silent--;
	return (retval);
}

/*
 * Read the Sub-Q-Channel to SubQbuffer. This is the method for
 * drives thp->sectsizeat do not support subchannel parameters.
 */
#ifdef	PROTOTYPES
static subq_chnl *
ReadSubQFallback(SCSI *scgp, unsigned char sq_format, unsigned char track)
#else
static subq_chnl *
ReadSubQFallback(scgp, sq_format, track)
	SCSI		*scgp;
	unsigned char	sq_format;
	unsigned char	track;
#endif
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)SubQbuffer;
	scmd->size = 24;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x42;		/* Read SubQChannel */
						/* use LBA */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = 0x40;	/* SubQ info */
	scmd->cdb.g1_cdb.addr[1] = 0;		/* parameter list: all */
	scmd->cdb.g1_cdb.res6 = track;		/* track number */
	g1_cdblen(&scmd->cdb.g1_cdb, 24);

	if (scgp->verbose)
		fprintf(stderr, _("\nRead Subchannel_dumb..."));

	scgp->cmdname = "Read Subchannel_dumb";

	if (scg_cmd(scgp) < 0) {
		errmsgno(EX_BAD, _("Read SubQ failed.\n"));
	}

	/*
	 * check, if the requested format is delivered
	 */
	{ unsigned char	*p = (unsigned char *) SubQbuffer;

		if ((((unsigned)p[2] << 8) | p[3]) /* LENGTH */ > ULONG_C(11) &&
		    (p[5] >> 4) /* ADR */ == sq_format) {
			if (sq_format == GET_POSITIONDATA)
				p[5] = (p[5] << 4) | (p[5] >> 4);
			return (SubQbuffer);
		}
	}

	/*
	 * FIXME: we might actively search for the requested info ...
	 */
	return (NULL);
}

/*
 * Read the Sub-Q-Channel to SubQbuffer
 */
#ifdef	PROTOTYPES
subq_chnl *
ReadSubQSCSI(SCSI *scgp, unsigned char sq_format, unsigned char track)
#else
subq_chnl *
ReadSubQSCSI(scgp, sq_format, track)
	SCSI		*scgp;
	unsigned char	sq_format;
	unsigned char	track;
#endif
{
	int		resp_size;
	register struct	scg_cmd	*scmd = scgp->scmd;

	switch (sq_format) {
	case GET_POSITIONDATA:
		resp_size = 16;
		track = 0;
		break;

	case GET_CATALOGNUMBER:
		resp_size = 24;
		track = 0;
		break;

	case GET_TRACK_ISRC:
		resp_size = 24;
		break;

	default:
		fprintf(stderr,
			_("ReadSubQSCSI: unknown format %d\n"), sq_format);
		return (NULL);
	}

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)SubQbuffer;
	scmd->size = resp_size;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x42;
						/* use LBA */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = 0x40; 	/* SubQ info */
	scmd->cdb.g1_cdb.addr[1] = sq_format;	/* parameter list: all */
	scmd->cdb.g1_cdb.res6 = track;		/* track number */
	g1_cdblen(&scmd->cdb.g1_cdb, resp_size);

	if (scgp->verbose)
		fprintf(stderr, _("\nRead Subchannel..."));

	scgp->cmdname = "Read Subchannel";

	if (scg_cmd(scgp) < 0) {
		/*
		 * in case of error do a fallback for dumb firmwares
		 */
		return (ReadSubQFallback(scgp, sq_format, track));
	}

	if (sq_format == GET_POSITIONDATA)
		SubQbuffer->control_adr = (SubQbuffer->control_adr << 4) |
					    (SubQbuffer->control_adr >> 4);
	return (SubQbuffer);
}

static subq_chnl sc;

static subq_chnl* fill_subchannel __PR((unsigned char bufferwithQ[]));
static subq_chnl* fill_subchannel(bufferwithQ)
	unsigned char bufferwithQ[];
{
	sc.subq_length = 0;
	sc.control_adr = bufferwithQ[CD_FRAMESIZE_RAW + 0];
	sc.track = bufferwithQ[CD_FRAMESIZE_RAW + 1];
	sc.index = bufferwithQ[CD_FRAMESIZE_RAW + 2];
	return (&sc);
}

int
ReadCddaSubSony(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)p;
	scmd->size = SectorBurstVal*(CD_FRAMESIZE_RAW + 16);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xd8;		/* read audio command */
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g5_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
	scmd->cdb.g5_cdb.res10 = 0x01;	/* subcode 1 -> cdda + 16 * q sub */
	g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
	g5_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);

	if (scgp->verbose)
		fprintf(stderr, _("\nReadSony12 CDDA + SubChannels..."));

	scgp->cmdname = "Read12SubChannelsSony";

	if (scg_cmd(scgp) < 0) {
		scgp->silent++;
		unit_ready(scgp);
		scgp->silent--;
		return (-1);
	}

	/*
	 * has all or something been read?
	 */
	return (scg_getresid(scgp) != 0);
}

int ReadCddaSub96Sony __PR((SCSI *scgp, UINT4 *p, unsigned lSector,
					unsigned SectorBurstVal));

int
ReadCddaSub96Sony(scgp, p, lSector, SectorBurstVal)
	SCSI	*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)p;
	scmd->size = SectorBurstVal*(CD_FRAMESIZE_RAW + 96);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xd8;		/* read audio command */
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g5_cdb.res |= (accepts_fua_bit == 1 ? 1 << 2 : 0);
	scmd->cdb.g5_cdb.res10 = 0x02;	/* subcode 2 -> cdda + 96 * q sub */
	g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
	g5_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);

	if (scgp->verbose)
		fprintf(stderr, _("\nReadSony12 CDDA + 96 byte SubChannels..."));

	scgp->cmdname = "Read12SubChannelsSony";

	if (scg_cmd(scgp) < 0) {
		scgp->silent++;
		unit_ready(scgp);
		scgp->silent--;
		return (-1);
	}

	/*
	 * has all or something been read?
	 */
	return (scg_getresid(scgp) != 0);
}

subq_chnl *
ReadSubChannelsSony(scgp, lSector)
	SCSI		*scgp;
	unsigned	lSector;
{
	/*int	retval = ReadCddaSub96Sony(scgp, (UINT4 *)bufferTOC, lSector, 1);*/
	int	retval = ReadCddaSubSony(scgp, (UINT4 *)bufferTOC, lSector, 1);

	if (retval != 0)
		return (NULL);

	return (fill_subchannel(bufferTOC));
}

/*
 * Read max. SectorBurst of cdda sectors to buffer
 * via MMC standard READ CD command
 */
int
ReadCddaSubMMC12(scgp, p, lSector, SectorBurstVal)
	SCSI		*scgp;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	register struct	scg_cmd	*scmd;

	scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)p;
	scmd->size = SectorBurstVal*(CD_FRAMESIZE_RAW + 16);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xbe;		/* read cd command */
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g5_cdb.res = 1 << 1; /* expected sector type field CDDA */
	g5_cdbaddr(&scmd->cdb.g5_cdb, lSector);
	g5x_cdblen(&scmd->cdb.g5_cdb, SectorBurstVal);
	scmd->cdb.g5_cdb.count[3] = 1 << 4;	/* User data */
	scmd->cdb.g5_cdb.res10 = 0x02;	/* subcode 2 -> cdda + 16 * q sub */

	if (scgp->verbose)
		fprintf(stderr, _("\nReadMMC12 CDDA + SUB..."));

	scgp->cmdname = "ReadCD Sub MMC 12";

	if (scg_cmd(scgp) < 0) {
		scgp->silent++;
		unit_ready(scgp);
		scgp->silent--;
		return (-1);
	}

	/*
	 * has all or something been read?
	 */
	return (scg_getresid(scgp) != 0);
}

static subq_chnl *ReadSubChannelsMMC __PR((SCSI *scgp, unsigned lSector));
static subq_chnl *
ReadSubChannelsMMC(scgp, lSector)
	SCSI		*scgp;
	unsigned	lSector;
{
	int	retval = ReadCddaSubMMC12(scgp, (UINT4 *)bufferTOC, lSector, 1);

	if (retval != 0)
		return (NULL);

	return (fill_subchannel(bufferTOC));
}

subq_chnl *
ReadSubChannelsFallbackMMC(scgp, lSector)
	SCSI		*scgp;
	unsigned	lSector;
{
static int		ReadSubSony_unknown = 0;
	subq_chnl	*retval = NULL;

	scgp->silent++;
	if (ReadSubSony_unknown ||
	    ((retval = ReadSubChannelsSony(scgp, lSector)) == NULL)) {
		/*
		 * if the command is not available, use the regular
		 * MMC ReadCd
		 */
		if (retval == NULL && scg_sense_key(scgp) == 0x05) {
			ReadSubSony_unknown = 1;
		}
		scgp->silent--;
		return (ReadSubChannelsMMC(scgp, lSector));
	}
	scgp->silent--;
	return (retval);
}

subq_chnl *
ReadStandardSub(scgp, lSector)
	SCSI		*scgp;
	unsigned	lSector;
{
	if (0 == ReadStandardLowlevel(scgp, (UINT4 *)bufferTOC, lSector, 1,
						CD_FRAMESIZE_RAW + 16)) {
		return (NULL);
	}
#if	0
	fprintf(stderr, "Subchannel Sec %x: %02x %02x %02x %02x\n",
		lSector,
		bufferTOC[CD_FRAMESIZE_RAW + 0],
		bufferTOC[CD_FRAMESIZE_RAW + 1],
		bufferTOC[CD_FRAMESIZE_RAW + 2],
		bufferTOC[CD_FRAMESIZE_RAW + 3]);
#endif
	sc.control_adr = (bufferTOC[CD_FRAMESIZE_RAW + 0] << 4)
		| bufferTOC[CD_FRAMESIZE_RAW + 1];
	sc.track = from_bcd(bufferTOC[CD_FRAMESIZE_RAW + 2]);
	sc.index = from_bcd(bufferTOC[CD_FRAMESIZE_RAW + 3]);
	return (&sc);
}
/* ******** non standardized speed selects ********************** */

void
SpeedSelectSCSIToshiba(scgp, speed)
	SCSI		*scgp;
	unsigned	speed;
{
static unsigned char	mode [4 + 3];
	unsigned char	*page = mode + 4;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	/*
	 * the first 4 mode bytes are zero.
	 */
	page[0] = 0x20;
	page[1] = 1;
	page[2] = speed; /* 0 for single speed, 1 for double speed (3401) */

	if (scgp->verbose)
		fprintf(stderr, _("\nspeed select Toshiba..."));

	scgp->silent++;
	/*
	 * do the scsi cmd
	 */
	if (mode_select(scgp, mode, 7, 0, scgp->inq->data_format >= 2) < 0)
		fprintf(stderr, _("speed select Toshiba failed\n"));
	scgp->silent--;
}

void
SpeedSelectSCSINEC(scgp, speed)
	SCSI		*scgp;
	unsigned	speed;
{
static unsigned char	mode [4 + 8];
	unsigned char	*page = mode + 4;
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	/*
	 * the first 4 mode bytes are zero.
	 */
	page [0] = 0x0f; /* page code */
	page [1] = 6;    /* parameter length */
	/*
	 * bit 5 == 1 for single speed, otherwise double speed
	 */
	page [2] = speed == 1 ? 1 << 5 : 0;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)mode;
	scmd->size = 12;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xC5;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = 0 ? 1 : 0 | 1 ? 0x10 : 0;
	g1_cdblen(&scmd->cdb.g1_cdb, 12);

	if (scgp->verbose)
		fprintf(stderr, _("\nspeed select NEC..."));

	/*
	 * do the scsi cmd
	 */
	scgp->cmdname = "speed select NEC";

	if (scg_cmd(scgp) < 0)
		errmsgno(EX_BAD, _("Speed select NEC failed.\n"));
}

void
SpeedSelectSCSIPhilipsCDD2600(scgp, speed)
	SCSI		*scgp;
	unsigned	speed;
{
	/*
	 * MODE_SELECT, page = SCSI-2 save page disabled, reserved, reserved,
	 * parm list len, flags
	 */
static unsigned char	mode [4 + 8];
	unsigned char	*page = mode + 4;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	/*
	 * the first 4 mode bytes are zero.
	 */
	page[0] = 0x23;
	page[1] = 6;
	page[2] = page [4] = speed;
	page[3] = 1;

	if (scgp->verbose)
		fprintf(stderr, _("\nspeed select Philips..."));
	/*
	 * do the scsi cmd
	 */
	if (mode_select(scgp, mode, 12, 0, scgp->inq->data_format >= 2) < 0)
		errmsgno(EX_BAD, _("Speed select PhilipsCDD2600 failed.\n"));
}

void
SpeedSelectSCSISony(scgp, speed)
	SCSI		*scgp;
	unsigned	speed;
{
static unsigned char	mode [4 + 4];
	unsigned char	*page = mode + 4;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	/*
	 * the first 4 mode bytes are zero.
	 */
	page[0] = 0x31;
	page[1] = 2;
	page[2] = speed;

	if (scgp->verbose)
		fprintf(stderr, _("\nspeed select Sony..."));
	/*
	 * do the scsi cmd
	 */
	scgp->silent++;
	if (mode_select(scgp, mode, 8, 0, scgp->inq->data_format >= 2) < 0)
		errmsgno(EX_BAD, _("Speed select Sony failed.\n"));
	scgp->silent--;
}

void
SpeedSelectSCSIYamaha(scgp, speed)
	SCSI		*scgp;
	unsigned	speed;
{
static unsigned char	mode [4 + 4];
	unsigned char	*page = mode + 4;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	/*
	 * the first 4 mode bytes are zero.
	 */
	page[0] = 0x31;
	page[1] = 2;
	page[2] = speed;

	if (scgp->verbose)
		fprintf(stderr, _("\nspeed select Yamaha..."));
	/*
	 * do the scsi cmd
	 */
	if (mode_select(scgp, mode, 8, 0, scgp->inq->data_format >= 2) < 0)
		errmsgno(EX_BAD, _("Speed select Yamaha failed.\n"));
}

void
SpeedSelectSCSIMMC(scgp, speed)
	SCSI		*scgp;
	unsigned	speed;
{
		int		spd;
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (speed == 0 || speed == 0xFFFF) {
		spd = 0xFFFF;
	} else {
		spd = (1764 * speed) / 10;
	}
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xBB;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	i_to_2_byte(&scmd->cdb.g5_cdb.addr[0], spd);
	i_to_2_byte(&scmd->cdb.g5_cdb.addr[2], 0xffff);

	if (scgp->verbose)
		fprintf(stderr, _("\nspeed select MMC..."));

	scgp->cmdname = "set cd speed";

	scgp->silent++;
	if (scg_cmd(scgp) < 0) {
		if (scg_sense_key(scgp) == 0x05 &&
		    scg_sense_code(scgp) == 0x20 &&
		    scg_sense_qual(scgp) == 0x00) {
			/*
			 * this optional command is not implemented
			 */
		} else {
			scg_printerr(scgp);
			errmsgno(EX_BAD, _("Speed select MMC failed.\n"));
		}
	}
	scgp->silent--;
}

/*
 * request vendor brand and model
 */
unsigned char *
ScsiInquiry(scgp)
	SCSI	*scgp;
{
static unsigned char 		*Inqbuffer = NULL;
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (Inqbuffer == NULL) {
		Inqbuffer = malloc(36);
		if (Inqbuffer == NULL) {
			errmsg(
			_("Cannot allocate memory for inquiry command in line %d\n"),
			__LINE__);
			return (NULL);
		}
	}

	fillbytes(Inqbuffer, 36, '\0');
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)Inqbuffer;
	scmd->size = 36;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_INQUIRY;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.count = 36;

	scgp->cmdname = "inquiry";

	if (scg_cmd(scgp) < 0)
		return (NULL);

	/*
	 * define structure with inquiry data
	 */
	memcpy(scgp->inq, Inqbuffer, sizeof (*scgp->inq));

	if (scgp->verbose) {
		scg_prbytes(_("Inquiry Data   :"),
			(Uchar *)Inqbuffer, 22 - scmd->resid);
	}
	return (Inqbuffer);
}

#define	SC_CLASS_EXTENDED_SENSE 0x07
#define	TESTUNITREADY_CMD 0
#define	TESTUNITREADY_CMDLEN 6

#define	ADD_SENSECODE 12
#define	ADD_SC_QUALIFIER 13
#define	NO_MEDIA_SC 0x3a
#define	NO_MEDIA_SCQ 0x00

int
TestForMedium(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (interface != GENERIC_SCSI) {
		return (1);
	}

	/*
	 * request READY status
	 */
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA | (1 ? SCG_SILENT:0);
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_TEST_UNIT_READY;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);

	if (scgp->verbose)
		fprintf(stderr, _("\ntest unit ready..."));
	scgp->silent++;

	scgp->cmdname = "test unit ready";

	if (scg_cmd(scgp) >= 0) {
		scgp->silent--;
		return (1);
	}
	scgp->silent--;

	if (scmd->sense.code >= SC_CLASS_EXTENDED_SENSE) {
		return (scmd->u_sense.cmd_sense[ADD_SENSECODE] !=
							NO_MEDIA_SC ||
		scmd->u_sense.cmd_sense[ADD_SC_QUALIFIER] != NO_MEDIA_SCQ);
	} else {
		/*
		 * analyse status.
		 * 'check condition' is interpreted as not ready.
		 */
		return ((scmd->u_scb.cmd_scb[0] & 0x1e) != 0x02);
	}
}

int
StopPlaySCSI(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = NULL;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x1b;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);

	if (scgp->verbose)
		fprintf(stderr, _("\nstop audio play"));

	/*
	 * do the scsi cmd
	 */
	scgp->cmdname = "stop audio play";

	return (scg_cmd(scgp) >= 0 ? 0 : -1);
}

int
Play_atSCSI(scgp, from_sector, sectors)
	SCSI		*scgp;
	unsigned int	from_sector;
	unsigned int	sectors;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = NULL;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x47;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[1] = (from_sector + 150) / (60*75);
	scmd->cdb.g1_cdb.addr[2] = ((from_sector + 150) / 75) % 60;
	scmd->cdb.g1_cdb.addr[3] = (from_sector + 150) % 75;
	scmd->cdb.g1_cdb.res6 = (from_sector + 150 + sectors) / (60*75);
	scmd->cdb.g1_cdb.count[0] = ((from_sector + 150 + sectors) / 75) % 60;
	scmd->cdb.g1_cdb.count[1] = (from_sector + 150 + sectors) % 75;

	if (scgp->verbose)
		fprintf(stderr, _("\nplay sectors..."));

	/*
	 * do the scsi cmd
	 */
	scgp->cmdname = "play sectors";

return (scg_cmd(scgp) >= 0 ? 0 : -1);
}

static caddr_t scsibuffer;	/* page aligned scsi transfer buffer */

EXPORT	void	init_scsibuf	__PR((SCSI *, long));

void
init_scsibuf(scgp, amt)
	SCSI		*scgp;
	long		amt;
{
	if (scsibuffer != NULL) {
		errmsgno(EX_BAD,
		_("The SCSI transfer buffer has already been allocated!\n"));
		exit(SETUPSCSI_ERROR);
	}
	scsibuffer = scg_getbuf(scgp, amt);
	if (scsibuffer == NULL) {
		errmsg(_("Could not get SCSI transfer buffer!\n"));
		exit(SETUPSCSI_ERROR);
	}
	global.buf = scsibuffer;
}
