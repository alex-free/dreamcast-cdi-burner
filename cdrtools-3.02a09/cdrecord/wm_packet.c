/* @(#)wm_packet.c	1.31 10/12/19 Copyright 1995, 1997, 2001-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)wm_packet.c	1.31 10/12/19 Copyright 1995, 1997, 2001-2010 J. Schilling";
#endif
/*
 *	CDR write method abtraction layer
 *	packet writing intercace routines
 *
 *	Copyright (c) 1995, 1997, 2001-2010 J. Schilling
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
#include <schily/unistd.h>
#include <schily/time.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include <scg/scsitransp.h>
#include "cdrecord.h"
#include "xio.h"

extern	int	debug;
extern	int	verbose;
extern	int	lverbose;

extern	char	*buf;			/* The transfer buffer */

EXPORT	int	write_packet_data __PR((SCSI *scgp, cdr_t *dp, track_t *trackp));

EXPORT int
write_packet_data(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	int	track = trackp->trackno;
	int	f = -1;
	int	isaudio;
	long	startsec;
	Llong	bytes_read = 0;
	Llong	bytes	= 0;
	Llong	savbytes = 0;
	int	count;
	Llong	tracksize;
	int	secsize;
	int	secspt;
	int	bytespt;
	int	bytes_to_read;
	long	amount;
	int	pad;
	int	retried;
	long	nextblock;
	BOOL	neednl	= FALSE;
	BOOL	islast	= FALSE;
	char	*bp	= buf;
	struct timeval tlast;
	struct timeval tcur;
	float	secsps = 75.0;
long bsize;
long bfree;
#define	BCAP
#ifdef	BCAP
int per = 0;
#ifdef	XBCAP
int oper = -1;
#endif
#endif

	if (dp->cdr_dstat->ds_flags & DSF_DVD)
		secsps = 676.27;
	if (dp->cdr_dstat->ds_flags & DSF_BD)
		secsps = 2195.07;

	scgp->silent++;
	if ((*dp->cdr_buffer_cap)(scgp, &bsize, &bfree) < 0)
		bsize = -1L;
	if (bsize == 0)		/* If we have no (known) buffer, we cannot */
		bsize = -1L;	/* retrieve the buffer fill ratio	   */
	else
		dp->cdr_dstat->ds_buflow = 0;
	scgp->silent--;

	if (trackp->xfp != NULL)
		f = xfileno(trackp->xfp);

	isaudio = is_audio(trackp);
	tracksize = trackp->tracksize;
	startsec = trackp->trackstart;

	secsize = trackp->secsize;
	secspt = trackp->secspt;
	bytespt = secsize * secspt;

	pad = !isaudio && is_pad(trackp);	/* Pad only data tracks */

	if (debug) {
		printf(_("secsize:%d secspt:%d bytespt:%d audio:%d pad:%d\n"),
			secsize, secspt, bytespt, isaudio, pad);
	}

	if (lverbose) {
		if (tracksize > 0)
			printf(_("\rTrack %02d:    0 of %4lld MB written."),
				track, tracksize >> 20);
		else
			printf(_("\rTrack %02d:    0 MB written."), track);
		flush();
		neednl = TRUE;
	}

	gettimeofday(&tlast, (struct timezone *)0);
	do {
		bytes_to_read = bytespt;
		if (tracksize > 0) {
			if ((tracksize - bytes_read) > bytespt)
				bytes_to_read = bytespt;
			else
				bytes_to_read = tracksize - bytes_read;
		}
					/* XXX next wr addr ??? */
		count = get_buf(f, trackp, startsec, &bp, bytes_to_read);
		if (count < 0)
			comerr(_("read error on input file\n"));
		if (count == 0)
			break;
		bytes_read += count;
		if (tracksize >= 0 && bytes_read >= tracksize) {
			count -= bytes_read - tracksize;
			if (trackp->padsecs == 0 || (bytes_read/secsize) >= 300)
				islast = TRUE;
		}

		if (count < bytespt) {
			if (debug) {
				printf(_("\nNOTICE: reducing block size for last record.\n"));
				neednl = FALSE;
			}

			if ((amount = count % secsize) != 0) {
				amount = secsize - amount;
				fillbytes(&bp[count], amount, '\0');
				count += amount;
				printf(_("\nWARNING: padding up to secsize.\n"));
				neednl = FALSE;
			}
			if (is_packet(trackp) && trackp->pktsize > 0) {
				if (count < bytespt) {
					amount = bytespt - count;
					count += amount;
					printf(_("\nWARNING: padding remainder of packet.\n"));
					neednl = FALSE;
				}
			}
			bytespt = count;
			secspt = count / secsize;
			if (trackp->padsecs == 0 || (bytes_read/secsize) >= 300)
				islast = TRUE;
		}

		retried = 0;
		retry:
		/* XXX Fixed-packet writes can be very slow*/
		if (is_packet(trackp) && trackp->pktsize > 0)
			scg_settimeout(scgp, 100);
		/* XXX */
		if (is_packet(trackp) && trackp->pktsize == 0) {
			if ((*dp->cdr_next_wr_address)(scgp, trackp, &nextblock) == 0) {
				/*
				 * Some drives (e.g. Ricoh MPS6201S) do not
				 * increment the Next Writable Address value to
				 * point to the beginning of a new packet if
				 * their write buffer has underflowed.
				 */
				if (retried && nextblock == startsec) {
					startsec += 7;
				} else {
					startsec = nextblock;
				}
			}
		}

		amount = write_secs(scgp, dp, bp, startsec, bytespt, secspt, islast);
		if (amount < 0) {
			if (is_packet(trackp) && trackp->pktsize == 0 && !retried) {
				printf(_("%swrite track data: error after %lld bytes, retry with new packet\n"),
					neednl?"\n":"", bytes);
				retried = 1;
				neednl = FALSE;
				goto retry;
			}
			printf(_("%swrite track data: error after %lld bytes\n"),
							neednl?"\n":"", bytes);
			return (-1);
		}
		bytes += amount;
		startsec += amount / secsize;

		if (lverbose && (bytes >= (savbytes + 0x100000))) {
			int	fper;
			int	nsecs = (bytes - savbytes) / secsize;
			float	fspeed;

			gettimeofday(&tcur, (struct timezone *)0);
			printf(_("\rTrack %02d: %4lld"), track, bytes >> 20);
			if (tracksize > 0)
				printf(_(" of %4lld MB"), tracksize >> 20);
			else
				printf(" MB");
			printf(_(" written"));
			fper = fifo_percent(TRUE);
			if (fper >= 0)
				printf(" (fifo %3d%%)", fper);
#ifdef	BCAP
			if (bsize > 0) {			/* buffer size known */
				scgp->silent++;
				per = (*dp->cdr_buffer_cap)(scgp, (long *)0, &bfree);
				scgp->silent--;
				if (per >= 0) {
					per = 100*(bsize - bfree) / bsize;
					if (per < 5)
						dp->cdr_dstat->ds_buflow++;
					if (per < (int)dp->cdr_dstat->ds_minbuf &&
					    (startsec*secsize) > bsize) {
						dp->cdr_dstat->ds_minbuf = per;
					}
					printf(" [buf %3d%%]", per);
#ifdef	BCAPDBG
					printf(" %3ld %3ld", bsize >> 10, bfree >> 10);
#endif
				}
			}
#endif

			tlast.tv_sec = tcur.tv_sec - tlast.tv_sec;
			tlast.tv_usec = tcur.tv_usec - tlast.tv_usec;
			while (tlast.tv_usec < 0) {
				tlast.tv_usec += 1000000;
				tlast.tv_sec -= 1;
			}
			fspeed = (nsecs / secsps) /
				(tlast.tv_sec * 1.0 + tlast.tv_usec * 0.000001);
			if (fspeed > 999.0)
				fspeed = 999.0;
			printf(" %5.1fx", fspeed);
			printf(".");
			savbytes = (bytes >> 20) << 20;
			flush();
			neednl = TRUE;
			tlast = tcur;
		}
	} while (tracksize < 0 || bytes_read < tracksize);

	if ((bytes / secsize) < 300) {
		if ((trackp->padsecs + (bytes / secsize)) < 300)
			trackp->padsecs = 300 - (bytes / secsize);
	}
	if (trackp->padsecs > 0) {
		Llong	padbytes;

		/*
		 * pad_track() is based on secsize. Compute the amount of bytes
		 * assumed by pad_track().
		 */
		padbytes = (Llong)trackp->padsecs * secsize;

		if (neednl) {
			printf("\n");
			neednl = FALSE;
		}
		if ((padbytes >> 20) > 0) {
			neednl = TRUE;
		} else if (lverbose) {
			printf(_("Track %02d: writing %3lld KB of pad data.\n"),
					track, (Llong)(padbytes >> 10));
			neednl = FALSE;
		}
		pad_track(scgp, dp, trackp, startsec, padbytes,
					TRUE, &savbytes);
		bytes += savbytes;
		startsec += savbytes / secsize;
	}
	printf(_("%sTrack %02d: Total bytes read/written: %lld/%lld (%lld sectors).\n"),
		neednl?"\n":"", track, bytes_read, bytes, bytes/secsize);
	return (0);
}
