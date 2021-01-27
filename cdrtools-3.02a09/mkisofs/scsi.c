/* @(#)scsi.c	1.35 12/12/02 Copyright 1997-2012 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scsi.c	1.35 12/12/02 Copyright 1997-2012 J. Schilling";
#endif
/*
 *	Copyright (c) 1997-2012 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef	USE_SCG

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/schily.h>

#include "mkisofs.h"
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "libscgcmd.h"
#include "cdrdeflt.h"

/*
 * NOTICE:	You should not make BUF_SIZE more than
 *		the buffer size of the CD-Recorder.
 *
 * Do not set BUF_SIZE to be more than 126 KBytes
 * if you are running cdrecord on a sun4c machine.
 *
 * WARNING:	Philips CDD 521 dies if BUF_SIZE is to big.
 */
#define	BUF_SIZE	(62*1024)	/* Must be a multiple of 2048	   */

LOCAL	SCSI	*scgp;
LOCAL	long	bufsize;		/* The size of the transfer buffer */

EXPORT	int	readsecs	__PR((UInt32_t startsecno, void *buffer, int sectorcount));
EXPORT	int	scsidev_open	__PR((char *path));
EXPORT	int	scsidev_close	__PR((void));

EXPORT int
readsecs(startsecno, buffer, sectorcount)
	UInt32_t startsecno;
	void	*buffer;
	int	sectorcount;
{
	int	f;
	int	secsize;	/* The drive's SCSI sector size		*/
	long	amount;		/* The number of bytes to be transfered	*/
	long	secno;		/* The sector number to read from	*/
	long	secnum;		/* The number of sectors to read	*/
	char	*bp;
	long	amt;

	if (in_image == NULL) {
		/*
		 * We are using the standard CD-ROM sectorsize of 2048 bytes
		 * while the drive may be switched to 512 bytes per sector.
		 *
		 * XXX We assume that secsize is no more than SECTOR_SIZE
		 * XXX and that SECTOR_SIZE / secsize is not a fraction.
		 */
		secsize = scgp->cap->c_bsize;
		amount = sectorcount * SECTOR_SIZE;
		secno = startsecno * (SECTOR_SIZE / secsize);
		bp = buffer;

		while (amount > 0) {
			amt = amount;
			if (amount > bufsize)
				amt = bufsize;
			secnum = amt / secsize;

			if (read_scsi(scgp, bp, secno, secnum) < 0 ||
						scg_getresid(scgp) != 0) {
#ifdef	OLD
				return (-1);
#else
				comerr(_("Read error on old image\n"));
#endif
			}

			amount	-= secnum * secsize;
			bp	+= secnum * secsize;
			secno	+= secnum;
		}
		return (SECTOR_SIZE * sectorcount);
	}

	f = fileno(in_image);

	if (lseek(f, (off_t)startsecno * SECTOR_SIZE, SEEK_SET) == (off_t)-1) {
		comerr(_("Seek error on old image\n"));
	}
	if ((amt = read(f, buffer, (sectorcount * SECTOR_SIZE)))
			!= (sectorcount * SECTOR_SIZE)) {
		if (ignerr) {
			if (amt < 0)
				amt = 0;
			fillbytes(&((char *)buffer)[amt],
					(sectorcount * SECTOR_SIZE) - amt, '\0');
			return (sectorcount * SECTOR_SIZE);	/* Should we cheat here too? */
		}
		if (amt < 0)
			comerr(_("Read error on old image\n"));
		comerrno(EX_BAD, _("Short read on old image\n")); /* < secnt aber > 0 */
	}
	return (sectorcount * SECTOR_SIZE);
}

EXPORT int
scsidev_open(path)
	char	*path;
{
	char	errstr[80];
	char	*buf;	/* ignored, bit OS/2 ASPI layer needs memory which */
			/* has been allocated by scsi_getbuf()		   */

	/*
	 * Call scg_remote() to force loading the remote SCSI transport library
	 * code that is located in librscg instead of the dummy remote routines
	 * that are located inside libscg.
	 */
	scg_remote();

	cdr_defaults(&path, NULL, NULL, NULL, NULL);
			/* path, debug, verboseopen */
	scgp = scg_open(path, errstr, sizeof (errstr), 0, 0);
	if (scgp == 0) {
		errmsg(_("%s%sCannot open SCSI driver.\n"), errstr, errstr[0]?". ":"");
		return (-1);
	}

	bufsize = scg_bufsize(scgp, BUF_SIZE);
	if ((buf = scg_getbuf(scgp, bufsize)) == NULL) {
		errmsg(_("Cannot get SCSI I/O buffer.\n"));
		scg_close(scgp);
		return (-1);
	}

	bufsize = (bufsize / SECTOR_SIZE) * SECTOR_SIZE;

	allow_atapi(scgp, TRUE);

	if (!wait_unit_ready(scgp, 60)) { /* Eat Unit att / Wait for drive */
		return (-1);
	}

	scgp->silent++;
	read_capacity(scgp);	/* Set Capacity/Sectorsize for I/O */
	scgp->silent--;

	return (1);
}

EXPORT int
scsidev_close()
{
	if (in_image == NULL) {
		return (scg_close(scgp));
	} else {
		return (fclose(in_image));
	}
}

#endif	/* USE_SCG */
