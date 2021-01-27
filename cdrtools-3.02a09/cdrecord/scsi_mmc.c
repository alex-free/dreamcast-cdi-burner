/* @(#)scsi_mmc.c	1.51 10/12/19 Copyright 2002-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scsi_mmc.c	1.51 10/12/19 Copyright 2002-2010 J. Schilling";
#endif
/*
 *	SCSI command functions for cdrecord
 *	covering MMC-3 level and above
 *
 *	Copyright (c) 2002-2010 J. Schilling
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

/*#define	DEBUG*/

#include <schily/mconfig.h>

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/time.h>

#include <schily/utypes.h>
#include <schily/btorder.h>
#include <schily/intcvt.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsimmc.h"
#include "cdrecord.h"

extern	int	xdebug;
extern	int	lverbose;

LOCAL struct features {
	UInt16_t	code;
	char		*name;
} fl[] = {
	{ 0x0000,	"Profile List", },
	{ 0x0001,	"Core", },
	{ 0x0002,	"Morphing", },
	{ 0x0003,	"Removable Medium", },
	{ 0x0004,	"Write Protect", },

	{ 0x0010,	"Random Readable", },

	{ 0x001D,	"Multi Read", },
	{ 0x001E,	"CD Read", },
	{ 0x001F,	"DVD Read", },

	{ 0x0020,	"Random Writable", },
	{ 0x0021,	"Incremental Streaming Writable", },
	{ 0x0022,	"Sector Erasable", },
	{ 0x0023,	"Formattable", },
	{ 0x0024,	"Defect Management", },
	{ 0x0025,	"Write Once", },
	{ 0x0026,	"Restricted Overwrite", },
	{ 0x0027,	"CD-RW CAV Write", },
	{ 0x0028,	"MRW", },
	{ 0x0029,	"Ehanced Defect Reporting", },
	{ 0x002A,	"DVD+RW", },
	{ 0x002B,	"DVD+R", },
	{ 0x002C,	"Rigid Restricted Overwrite", },
	{ 0x002D,	"CD Track at Once", },
	{ 0x002E,	"CD Mastering", },
	{ 0x002F,	"DVD-R/-RW Write", },

	{ 0x0030,	"DDCD Read", },
	{ 0x0031,	"DDCD-R Write", },
	{ 0x0032,	"DDCD-RW Write", },

	{ 0x0033,	"Layer Jump Recording", },

	{ 0x0037,	"CD-RW Write", },
	{ 0x0038,	"BD-R Pseudo-Overwrite (POW)", },

	{ 0x003A,	"DVD+RW/DL Read", },
	{ 0x003B,	"DVD+R/DL Read", },

	{ 0x0040,	"BD Read", },
	{ 0x0041,	"BD Write", },
	{ 0x0042,	"Time Safe Recording (TSR)", },

	{ 0x0050,	"HD-DVD Read", },
	{ 0x0051,	"HD-DVD Write", },

	{ 0x0080,	"Hybrid Disk Read", },

	{ 0x0100,	"Power Management", },
	{ 0x0101,	"S.M.A.R.T.", },
	{ 0x0102,	"Embedded Changer", },
	{ 0x0103,	"CD Audio analog play", },
	{ 0x0104,	"Microcode Upgrade", },
	{ 0x0105,	"Time-out", },
	{ 0x0106,	"DVD-CSS", },
	{ 0x0107,	"Real Time Streaming", },
	{ 0x0108,	"Logical Unit Serial Number", },
	{ 0x0109,	"Media Serial Number", },
	{ 0x010A,	"Disk Control Blocks", },
	{ 0x010B,	"DVD CPRM", },
	{ 0x010C,	"Microcode Information", },
	{ 0x010D,	"AACS", },

	{ 0x0110,	"VCPS", },
};

LOCAL struct profiles {
	UInt16_t	code;
	char		*name;
} pl[] = {
	{ 0x0000,	"Reserved", },
	{ 0x0001,	"Non -removable Disk", },
	{ 0x0002,	"Removable Disk", },
	{ 0x0003,	"MO Erasable", },
	{ 0x0004,	"MO Write Once", },
	{ 0x0005,	"AS-MO", },

	/* 0x06..0x07 is reserved */

	{ 0x0008,	"CD-ROM", },
	{ 0x0009,	"CD-R", },
	{ 0x000A,	"CD-RW", },

	/* 0x0B..0x0F is reserved */

	{ 0x0010,	"DVD-ROM", },
	{ 0x0011,	"DVD-R sequential recording", },
	{ 0x0012,	"DVD-RAM", },
	{ 0x0013,	"DVD-RW restricted overwrite", },
	{ 0x0014,	"DVD-RW sequential recording", },
	{ 0x0015,	"DVD-R/DL sequential recording", },
	{ 0x0016,	"DVD-R/DL layer jump recording", },
	{ 0x0017,	"DVD-RW/DL", },

	/* 0x18..0x19 is reserved */

	{ 0x001A,	"DVD+RW", },
	{ 0x001B,	"DVD+R", },

	{ 0x0020,	"DDCD-ROM", },
	{ 0x0021,	"DDCD-R", },
	{ 0x0022,	"DDCD-RW", },

	{ 0x002A,	"DVD+RW/DL", },
	{ 0x002B,	"DVD+R/DL", },

	{ 0x0040,	"BD-ROM", },
	{ 0x0041,	"BD-R sequential recording", },
	{ 0x0042,	"BD-R random recording", },
	{ 0x0043,	"BD-RE", },

	/* 0x44..0x4F is reserved */

	{ 0x0050,	"HD DVD-ROM", },
	{ 0x0051,	"HD DVD-R", },
	{ 0x0052,	"HD DVD-RAM", },
	{ 0x0053,	"HD DVD-RW", },

	/* 0x54..0x57 is reserved */

	{ 0x0058,	"HD DVD-R/DL", },

	{ 0x005A,	"HD DVD-RW/DL", },

	{ 0xFFFF,	"No standard Profile", },
};


EXPORT	int	get_configuration	__PR((SCSI *scgp, caddr_t bp, int cnt, int st_feature, int rt));
LOCAL	int	get_conflen		__PR((SCSI *scgp, int st_feature, int rt));
EXPORT	int	get_curprofile		__PR((SCSI *scgp));
LOCAL	int	get_profiles		__PR((SCSI *scgp, caddr_t bp, int cnt));
EXPORT	int	has_profile		__PR((SCSI *scgp, int profile));
EXPORT	int	print_profiles		__PR((SCSI *scgp));
EXPORT	int	get_proflist		__PR((SCSI *scgp, BOOL *wp, BOOL *cdp, BOOL *dvdp, BOOL *dvdplusp, BOOL *ddcdp));
EXPORT	int	get_wproflist		__PR((SCSI *scgp, BOOL *cdp, BOOL *dvdp,
							BOOL *dvdplusp, BOOL *ddcdp));
EXPORT	int	get_mediatype		__PR((SCSI *scgp));
EXPORT	int	get_singlespeed		__PR((int mt));
EXPORT	float	get_secsps		__PR((int mt));
EXPORT	char	*get_mclassname		__PR((int mt));
EXPORT	int	get_blf			__PR((int mt));

LOCAL	int	scsi_get_performance	__PR((SCSI *scgp, caddr_t bp, int cnd, int ndesc, int type, int datatype));
EXPORT	int	scsi_get_perf_maxspeed	__PR((SCSI *scgp, Ulong *readp, Ulong *writep, Ulong *endp));
EXPORT	int	scsi_get_perf_curspeed	__PR((SCSI *scgp, Ulong *readp, Ulong *writep, Ulong *endp));
LOCAL	int	scsi_set_streaming	__PR((SCSI *scgp, Ulong *readp, Ulong *writep, Ulong *endp));
EXPORT	int	speed_select_mdvd	__PR((SCSI *scgp, int readspeed, int writespeed));
LOCAL	char	*fname			__PR((Uint code));
LOCAL	char	*pname			__PR((Uint code));
LOCAL	BOOL	fname_known		__PR((Uint code));
LOCAL	BOOL	pname_known		__PR((Uint code));
EXPORT	int	print_features		__PR((SCSI *scgp));
EXPORT	void	print_format_capacities	__PR((SCSI *scgp));
EXPORT	int	get_format_capacities	__PR((SCSI *scgp, caddr_t bp, int cnt));
EXPORT	int	read_format_capacities	__PR((SCSI *scgp, caddr_t bp, int cnt));

EXPORT	void	przone			__PR((struct rzone_info *rp));
EXPORT	int	get_diskinfo		__PR((SCSI *scgp, struct disk_info *dip, int cnt));
EXPORT	char	*get_ses_type		__PR((int ses_type));
EXPORT	void	print_diskinfo		__PR((struct disk_info *dip, BOOL is_cd));
EXPORT	int	prdiskstatus		__PR((SCSI *scgp, cdr_t *dp, BOOL is_cd));
EXPORT	int	sessstatus		__PR((SCSI *scgp, BOOL is_cd, long *offp, long *nwap));
EXPORT	void	print_performance_mmc	__PR((SCSI *scgp));


/*
 * Get feature codes
 */
EXPORT int
get_configuration(scgp, bp, cnt, st_feature, rt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	st_feature;
	int	rt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x46;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	if (rt & 1)
		scmd->cdb.g1_cdb.reladr  = 1;
	if (rt & 2)
		scmd->cdb.g1_cdb.res  = 1;

	i_to_2_byte(scmd->cdb.g1_cdb.addr, st_feature);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "get_configuration";

	return (scg_cmd(scgp));
}

/*
 * Retrieve feature code list length
 */
LOCAL int
get_conflen(scgp, st_feature, rt)
	SCSI	*scgp;
	int	st_feature;
	int	rt;
{
	Uchar	cbuf[8];
	int	flen;
	int	i;

	fillbytes(cbuf, sizeof (cbuf), '\0');
	scgp->silent++;
	i = get_configuration(scgp, (char *)cbuf, sizeof (cbuf), st_feature, rt);
	scgp->silent--;
	if (i < 0)
		return (-1);
	i = sizeof (cbuf) - scg_getresid(scgp);
	if (i < 4)
		return (-1);

	flen = a_to_u_4_byte(cbuf);
	if (flen < 4)
		return (-1);
	return (flen);
}

EXPORT int
get_curprofile(scgp)
	SCSI	*scgp;
{
	Uchar	cbuf[8];
	int	amt;
	int	flen;
	int	profile;
	int	i;

	fillbytes(cbuf, sizeof (cbuf), '\0');
	scgp->silent++;
	i = get_configuration(scgp, (char *)cbuf, sizeof (cbuf), 0, 0);
	scgp->silent--;
	if (i < 0)
		return (-1);

	amt = sizeof (cbuf) - scg_getresid(scgp);
	if (amt < 8)
		return (-1);
	flen = a_to_u_4_byte(cbuf);
	if (flen < 4)
		return (-1);

	profile = a_to_u_2_byte(&cbuf[6]);

	if (xdebug > 1)
		scg_prbytes(_("Features: "), cbuf, amt);

	if (xdebug > 0)
		printf(_("feature len: %d current profile 0x%04X len %d\n"),
				flen, profile, amt);

	return (profile);
}

LOCAL int
get_profiles(scgp, bp, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
{
	int	amt;
	int	flen;
	int	i;

	flen = get_conflen(scgp, 0, 0);
	if (flen < 0)
		return (-1);
	if (cnt < flen)
		flen = cnt;

	fillbytes(bp, cnt, '\0');
	scgp->silent++;
	i = get_configuration(scgp, (char *)bp, flen, 0, 0);
	scgp->silent--;
	if (i < 0)
		return (-1);
	amt = flen - scg_getresid(scgp);

	flen = a_to_u_4_byte(bp);
	if ((flen+4) < amt)
		amt = flen+4;

	return (amt);
}

EXPORT int
has_profile(scgp, profile)
	SCSI	*scgp;
	int	profile;
{
	Uchar	cbuf[1024];
	Uchar	*p;
	int	flen;
	int	prf;
	int	i;
	int	n;

	flen = get_profiles(scgp, (caddr_t)cbuf, sizeof (cbuf));
	if (flen < 0)
		return (-1);

	p = cbuf;
	p += 8;		/* Skip feature header	*/
	n = p[3];	/* Additional length	*/
	n /= 4;
	p += 4;

	for (i = 0; i < n; i++) {
		prf = a_to_u_2_byte(p);
		if (xdebug > 0)
			printf(_("Profile: 0x%04X "), prf);
		if (profile == prf)
			return (1);
		p += 4;
	}
	return (0);
}

EXPORT int
print_profiles(scgp)
	SCSI	*scgp;
{
	Uchar	cbuf[1024];
	Uchar	*p;
	int	flen;
	int	curprofile;
	int	profile;
	int	i;
	int	n;

	flen = get_profiles(scgp, (caddr_t)cbuf, sizeof (cbuf));
	if (flen < 0)
		return (-1);

	p = cbuf;
	if (xdebug > 1)
		scg_prbytes(_("Features: "), cbuf, flen);

	curprofile = a_to_u_2_byte(&p[6]);
	if (xdebug > 0)
		printf(_("feature len: %d current profile 0x%04X\n"),
				flen, curprofile);

	if (pname_known(curprofile))
		printf(_("Current: %s\n"), curprofile == 0 ? _("none") : pname(curprofile));
	else
		printf(_("Current: 0x%04X unknown\n"), curprofile);

	p += 8;		/* Skip feature header	*/
	n = p[3];	/* Additional length	*/
	n /= 4;
	p += 4;

	for (i = 0; i < n; i++) {
		profile = a_to_u_2_byte(p);
		if (xdebug > 0)
			printf(_("Profile: 0x%04X "), profile);
		else
			printf(_("Profile: "));
		if (pname_known(profile))
			printf("%s %s\n", pname(profile), p[2] & 1 ? _("(current)"):"");
		else
			printf("0x%04X %s\n", profile, p[2] & 1 ? _("(current)"):"");
		p += 4;
	}
	return (curprofile);
}

EXPORT int
get_proflist(scgp, wp, cdp, dvdp, dvdplusp, ddcdp)
	SCSI	*scgp;
	BOOL	*wp;
	BOOL	*cdp;
	BOOL	*dvdp;
	BOOL	*dvdplusp;
	BOOL	*ddcdp;
{
	Uchar	cbuf[1024];
	Uchar	*p;
	int	flen;
	int	curprofile;
	int	profile;
	int	i;
	int	n;
	BOOL	wr	= FALSE;
	BOOL	cd	= FALSE;
	BOOL	dvd	= FALSE;
	BOOL	dvdplus	= FALSE;
	BOOL	ddcd	= FALSE;

	flen = get_profiles(scgp, (caddr_t)cbuf, sizeof (cbuf));
	if (flen < 0)
		return (-1);

	p = cbuf;
	if (xdebug > 1)
		scg_prbytes(_("Features: "), cbuf, flen);

	curprofile = a_to_u_2_byte(&p[6]);
	if (xdebug > 0)
		printf(_("feature len: %d current profile 0x%04X\n"),
				flen, curprofile);

	p += 8;		/* Skip feature header	*/
	n = p[3];	/* Additional length	*/
	n /= 4;
	p += 4;

	for (i = 0; i < n; i++) {
		profile = a_to_u_2_byte(p);
		p += 4;
		if (profile >= 0x0008 && profile < 0x0010)
			cd = TRUE;
		if (profile > 0x0008 && profile < 0x0010)
			wr = TRUE;

		if (profile >= 0x0010 && profile < 0x0018)
			dvd = TRUE;
		if (profile > 0x0010 && profile < 0x0018)
			wr = TRUE;

		if (profile >= 0x0018 && profile < 0x0020)
			dvdplus = TRUE;
		if (profile > 0x0018 && profile < 0x0020)
			wr = TRUE;

		if (profile >= 0x0020 && profile < 0x0028)
			ddcd = TRUE;
		if (profile > 0x0020 && profile < 0x0028)
			wr = TRUE;
	}
	if (wp)
		*wp	= wr;
	if (cdp)
		*cdp	= cd;
	if (dvdp)
		*dvdp	= dvd;
	if (dvdplusp)
		*dvdplusp = dvdplus;
	if (ddcdp)
		*ddcdp	= ddcd;

	return (curprofile);
}

EXPORT int
get_wproflist(scgp, cdp, dvdp, dvdplusp, ddcdp)
	SCSI	*scgp;
	BOOL	*cdp;
	BOOL	*dvdp;
	BOOL	*dvdplusp;
	BOOL	*ddcdp;
{
	Uchar	cbuf[1024];
	Uchar	*p;
	int	flen;
	int	curprofile;
	int	profile;
	int	i;
	int	n;
	BOOL	cd	= FALSE;
	BOOL	dvd	= FALSE;
	BOOL	dvdplus	= FALSE;
	BOOL	ddcd	= FALSE;

	flen = get_profiles(scgp, (caddr_t)cbuf, sizeof (cbuf));
	if (flen < 0)
		return (-1);
	p = cbuf;
	curprofile = a_to_u_2_byte(&p[6]);

	p += 8;		/* Skip feature header	*/
	n = p[3];	/* Additional length	*/
	n /= 4;
	p += 4;

	for (i = 0; i < n; i++) {
		profile = a_to_u_2_byte(p);
		p += 4;
		if (profile > 0x0008 && profile < 0x0010)
			cd = TRUE;
		if (profile > 0x0010 && profile < 0x0018)
			dvd = TRUE;
		if (profile > 0x0018 && profile < 0x0020)
			dvdplus = TRUE;
		if (profile > 0x0020 && profile < 0x0028)
			ddcd = TRUE;
	}
	if (cdp)
		*cdp	= cd;
	if (dvdp)
		*dvdp	= dvd;
	if (dvdplusp)
		*dvdplusp = dvdplus;
	if (ddcdp)
		*ddcdp	= ddcd;

	return (curprofile);
}

EXPORT int
get_mediatype(scgp)
	SCSI	*scgp;
{
	int	profile = get_curprofile(scgp);

	if (profile < 0x08)
		return (MT_NONE);
	if (profile >= 0x08 && profile < 0x10)
		return (MT_CD);
	if (profile >= 0x10 && profile < 0x40)
		return (MT_DVD);
	if (profile >= 0x40 && profile < 0x50)
		return (MT_BD);
	if (profile >= 0x50 && profile < 0x60)
		return (MT_HDDVD);

	return (MT_NONE);
}

EXPORT int
get_singlespeed(mt)
	int	mt;
{
	switch (mt) {

	case MT_CD:
		return (176);

	case MT_DVD:
		return (1385);

	case MT_BD:
		return (4495);

	case MT_HDDVD:
		return (4495);	/* XXX ??? */

	case MT_NONE:
	default:
		return (1);
	}
}

EXPORT float
get_secsps(mt)
	int	mt;
{
	switch (mt) {

	case MT_CD:
		return ((float)75.0);

	case MT_DVD:
		return ((float)676.27);

	case MT_BD:
		return ((float)2195.07);

	case MT_HDDVD:
		return ((float)2195.07);	/* XXX ??? */

	case MT_NONE:
	default:
		return ((float)75.0);
	}
}

EXPORT char *
get_mclassname(mt)
	int	mt;
{
	switch (mt) {

	case MT_CD:
		return ("CD");

	case MT_DVD:
		return ("DVD");

	case MT_BD:
		return ("BD");

	case MT_HDDVD:
		return ("HD-DVD");

	case MT_NONE:
	default:
		return ("NONE");
	}
}

/*
 * Guessed blocking factor based on media type
 */
EXPORT int
get_blf(mt)
	int	mt;
{
	switch (mt) {

	case MT_DVD:
		return (16);

	case MT_BD:
		return (32);

	case MT_HDDVD:
		return (32);	/* XXX ??? */

	default:
		return (1);
	}
}

LOCAL int
scsi_get_performance(scgp, bp, cnt, ndesc, type, datatype)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	ndesc;
	int	type;
	int	datatype;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t) scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA | SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xAC;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[1] |= datatype & 0x1F;
	scmd->cdb.cmd_cdb[9] = ndesc;
	scmd->cdb.cmd_cdb[10] = type;

	scgp->cmdname = "get performance";

	return (scg_cmd(scgp));
}


EXPORT int
scsi_get_perf_maxspeed(scgp, readp, writep, endp)
	SCSI	*scgp;
	Ulong	*readp;
	Ulong	*writep;
	Ulong	*endp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	struct mmc_performance_header	*ph;
	struct mmc_write_speed		*wsp;
#define	MAX_AMT	100
	char buffer[8 + MAX_AMT*16];
	Ulong	ul;
	int	amt;
	int	i;
	int	mt = 0;
	int	ssp = 1;
	char	*mname = NULL;

	if (xdebug != 0) {
		mt = get_mediatype(scgp);
		ssp = get_singlespeed(mt);
		mname = get_mclassname(mt);
	}
	fillbytes((caddr_t) buffer, sizeof (buffer), '\0');
	ph = (struct mmc_performance_header *)buffer;
	if (scsi_get_performance(scgp, buffer, 8+16, 1, 0x03, 0) < 0)
		return (-1);

	amt = (a_to_4_byte(ph->p_datalen) -4)/sizeof (struct mmc_write_speed);
	if (amt < 1)
		amt = 1;
	if (amt > MAX_AMT)
		amt = MAX_AMT;
	if (scsi_get_performance(scgp, buffer, 8+amt*16, amt, 0x03, 0) < 0)
		return (-1);

#ifdef	XDEBUG
	error(_("Bytes: %d\n"), scmd->size - scg_getresid(scgp));
	error(_("header: %ld\n"), a_to_4_byte(buffer) + 4);
#endif

	ph = (struct mmc_performance_header *)buffer;
	wsp = (struct mmc_write_speed *)(((char *)ph) +
				sizeof (struct mmc_performance_header));

	ul = a_to_u_4_byte(wsp->end_lba);
	if (endp)
		*endp = ul;

	ul = a_to_u_4_byte(wsp->read_speed);
	if (readp)
		*readp = ul;

	ul = a_to_u_4_byte(wsp->write_speed);
	if (writep)
		*writep = ul;

	wsp = (struct mmc_write_speed *)(((char *)ph) +
				sizeof (struct mmc_performance_header));

	i = (a_to_4_byte(buffer) -4)/sizeof (struct mmc_write_speed);
	if (i > scmd->cdb.cmd_cdb[9])
		i = scmd->cdb.cmd_cdb[9];
	if (xdebug > 0)
		error(_("MaxSpeed Nperf:   %d\n"), i);
	if (xdebug != 0) for (; --i >= 0; wsp++) {
		ul = a_to_u_4_byte(wsp->end_lba);
		error(_("End LBA:     %7lu\n"), ul);
		ul = a_to_u_4_byte(wsp->read_speed);
		error(_("Read Speed:  %7lu == %lux %s\n"), ul, ul/ssp, mname);
		ul = a_to_u_4_byte(wsp->write_speed);
		error(_("Write Speed: %7lu == %lux %s\n"), ul, ul/ssp, mname);
		error("\n");
	}
#ifdef	XDEBUG
	scg_prbytes(_("Performance data:"), (Uchar *)buffer, scmd->size - scg_getresid(scgp));
#endif

	return (0);
#undef	MAX_AMT
}

EXPORT int
scsi_get_perf_curspeed(scgp, readp, writep, endp)
	SCSI	*scgp;
	Ulong	*readp;
	Ulong	*writep;
	Ulong	*endp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	struct mmc_performance_header	*ph;
	struct mmc_performance		*perfp;
#define	MAX_AMT	100
	char buffer[8 + MAX_AMT*16];
	Ulong	ul;
	Ulong	end;
	Ulong	speed;
	int	amt;
	int	i;
	int	mt = 0;
	int	ssp = 1;
	char	*mname = NULL;

	if (xdebug != 0) {
		mt = get_mediatype(scgp);
		ssp = get_singlespeed(mt);
		mname = get_mclassname(mt);
	}

	if (endp || writep) {
		fillbytes((caddr_t) buffer, sizeof (buffer), '\0');
		scgp->silent++;
		if (scsi_get_performance(scgp, buffer, 8+16, 1, 0x00, 0x04) < 0) {
			scgp->silent--;
			goto doread;
		}
		scgp->silent--;

		ph = (struct mmc_performance_header *)buffer;
		amt = (a_to_4_byte(ph->p_datalen) -4)/sizeof (struct mmc_performance);
		if (amt < 1)
			amt = 1;
		if (amt > MAX_AMT)
			amt = MAX_AMT;

		if (scsi_get_performance(scgp, buffer, 8+16*amt, amt, 0x00, 0x04) < 0)
			return (-1);

#ifdef	XDEBUG
		error(_("Bytes: %d\n"), scmd->size - scg_getresid(scgp));
		error(_("header: %ld\n"), a_to_4_byte(buffer) + 4);
#endif

		ph = (struct mmc_performance_header *)buffer;
		perfp = (struct mmc_performance *)(((char *)ph) +
				sizeof (struct mmc_performance_header));

		i = (a_to_4_byte(buffer) -4)/sizeof (struct mmc_performance);
		if (i > amt)
			i = amt;
		end = 0;
		speed = 0;
		for (; --i >= 0; perfp++) {
			ul = a_to_u_4_byte(perfp->end_lba);
			if (ul > end) {
				end = ul;
				ul = a_to_u_4_byte(perfp->end_perf);
				speed = ul;
			}
		}

		if (endp)
			*endp = end;

		if (writep)
			*writep = speed;

		perfp = (struct mmc_performance *)(((char *)ph) +
				sizeof (struct mmc_performance_header));
		i = (a_to_4_byte(buffer) -4)/sizeof (struct mmc_performance);
		if (i > scmd->cdb.cmd_cdb[9])
			i = scmd->cdb.cmd_cdb[9];
		if (xdebug > 1)
			error(_("CurSpeed Writeperf: %d\n"), i);
		else if (xdebug < 0)
			error(_("Write Performance:\n"));
		if (xdebug != 0) for (; --i >= 0; perfp++) {
			ul = a_to_u_4_byte(perfp->start_lba);
			error(_("START LBA:   %7lu\n"), ul);
			ul = a_to_u_4_byte(perfp->end_lba);
			error(_("End LBA:     %7lu\n"), ul);
			ul = a_to_u_4_byte(perfp->start_perf);
			error(_("Start Perf:  %7lu == %lux %s\n"), ul, ul/ssp, mname);
			ul = a_to_u_4_byte(perfp->end_perf);
			error(_("END Perf:    %7lu == %lux %s\n"), ul, ul/ssp, mname);
			error("\n");
		}
#ifdef	XDEBUG
		scg_prbytes(_("Performance data:"), (Uchar *)buffer, scmd->size - scg_getresid(scgp));
#endif
	}
doread:
	if (readp) {
		fillbytes((caddr_t) buffer, sizeof (buffer), '\0');
		scgp->silent++;
		if (scsi_get_performance(scgp, buffer, 8+16, 1, 0x00, 0x00) < 0) {
			scgp->silent--;
			return (-1);
		}
		scgp->silent--;

		ph = (struct mmc_performance_header *)buffer;
		amt = (a_to_4_byte(ph->p_datalen) -4)/sizeof (struct mmc_performance);
		if (amt < 1)
			amt = 1;
		if (amt > MAX_AMT)
			amt = MAX_AMT;

		if (scsi_get_performance(scgp, buffer, 8+16*amt, amt, 0x00, 0x00) < 0)
			return (-1);

#ifdef	XDEBUG
		error(_("Bytes: %d\n"), scmd->size - scg_getresid(scgp));
		error(_("header: %ld\n"), a_to_4_byte(buffer) + 4);
#endif

		ph = (struct mmc_performance_header *)buffer;
		perfp = (struct mmc_performance *)(((char *)ph) +
				sizeof (struct mmc_performance_header));

		i = (a_to_4_byte(buffer) -4)/sizeof (struct mmc_performance);
		if (i > amt)
			i = amt;
		end = 0;
		speed = 0;
		for (; --i >= 0; perfp++) {
			ul = a_to_u_4_byte(perfp->end_lba);
			if (ul > end) {
				end = ul;
				ul = a_to_u_4_byte(perfp->end_perf);
				speed = ul;
			}
		}

		if (readp)
			*readp = speed;

		i = (a_to_4_byte(buffer) -4)/sizeof (struct mmc_performance);
		if (i > scmd->cdb.cmd_cdb[9])
			i = scmd->cdb.cmd_cdb[9];
		if (xdebug > 1)
			error(_("CurSpeed Readperf: %d\n"), i);
		else if (xdebug < 0)
			error(_("Read Performance:\n"));
		if (xdebug != 0) for (; --i >= 0; perfp++) {
			ul = a_to_u_4_byte(perfp->start_lba);
			error(_("START LBA:   %7lu\n"), ul);
			ul = a_to_u_4_byte(perfp->end_lba);
			error(_("End LBA:     %7lu\n"), ul);
			ul = a_to_u_4_byte(perfp->start_perf);
			error(_("Start Perf:  %7lu == %lux %s\n"), ul, ul/ssp, mname);
			ul = a_to_u_4_byte(perfp->end_perf);
			error(_("END Perf:    %7lu == %lux %s\n"), ul, ul/ssp, mname);
			error("\n");
		}
#ifdef	XDEBUG
		scg_prbytes(_("Performance data:"), (Uchar *)buffer, scmd->size - scg_getresid(scgp));
#endif
	}

	return (0);
}

LOCAL int
scsi_set_streaming(scgp, readp, writep, endp)
	SCSI	*scgp;
	Ulong	*readp;
	Ulong	*writep;
	Ulong	*endp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	struct mmc_streaming	str;
	struct mmc_streaming	*sp = &str;

	fillbytes((caddr_t) scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t) sp;
	scmd->size = sizeof (*sp);
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xB6;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	i_to_2_byte(&scmd->cdb.cmd_cdb[9], sizeof (*sp)); /* Sz not G5 alike */

	scgp->cmdname = "set streaming";

	fillbytes(sp, sizeof (*sp), '\0');
	if (endp)
		i_to_4_byte(sp->end_lba, *endp);
	else
		i_to_4_byte(sp->end_lba, 0x7FFFFFFF);

	if (readp)
		i_to_4_byte(sp->read_size, *readp);
	else
		i_to_4_byte(sp->read_size, 0x7FFFFFFF);

	if (writep)
		i_to_4_byte(sp->write_size, *writep);
	else
		i_to_4_byte(sp->write_size, 0x7FFFFFFF);

	i_to_4_byte(sp->read_time, 1000);
	i_to_4_byte(sp->write_time, 1000);

#ifdef	DEBUG
	scg_prbytes(_("Streaming data:"), (Uchar *)sp, sizeof (*sp));
#endif

	return (scg_cmd(scgp));
}

/*
 * set speed using the streaming descriptors
 */
EXPORT int
speed_select_mdvd(scgp, readspeed, writespeed)
	SCSI	*scgp;
	int	readspeed;
	int	writespeed;
{
	Ulong	end_lba = 0x7FFFFFFF;
	Ulong	wspeed = writespeed;

	if (scsi_get_perf_maxspeed(scgp, (Ulong *)NULL, (Ulong *)NULL, &end_lba) < 0)
		return (-1);

	if (scsi_set_streaming(scgp, (Ulong *)NULL, &wspeed, &end_lba) < 0)
		return (-1);

	return (0);
}


LOCAL char *
fname(code)
	Uint	code;
{
	UInt16_t	i;

	for (i = 0; i < sizeof (fl) / sizeof (fl[0]); i++) {
		if (code == fl[i].code)
			return (fl[i].name);
	}
	return ("Unknown");
}

LOCAL char *
pname(code)
	Uint	code;
{
	UInt16_t	i;

	for (i = 0; i < sizeof (pl) / sizeof (pl[0]); i++) {
		if (code == pl[i].code)
			return (pl[i].name);
	}
	return ("Unknown");
}

LOCAL BOOL
fname_known(code)
	Uint	code;
{
	UInt16_t	i;

	for (i = 0; i < sizeof (fl) / sizeof (fl[0]); i++) {
		if (code == fl[i].code)
			return (TRUE);
	}
	return (FALSE);
}

LOCAL BOOL
pname_known(code)
	Uint	code;
{
	UInt16_t	i;

	for (i = 0; i < sizeof (pl) / sizeof (pl[0]); i++) {
		if (code == pl[i].code)
			return (TRUE);
	}
	return (FALSE);
}

EXPORT int
print_features(scgp)
	SCSI	*scgp;
{
	Uchar	fbuf[32 * 1024];
	Uchar	*p;
	Uchar	*pend;
	int	amt;
	int	flen;
	int	feature;
	int	i;

	flen = get_conflen(scgp, 0, 0);
	if (flen < 0)
		return (-1);
	if (sizeof (fbuf) < flen)
		flen = sizeof (fbuf);

	fillbytes(fbuf, sizeof (fbuf), '\0');
	scgp->silent++;
	i = get_configuration(scgp, (char *)fbuf, sizeof (fbuf), 0, 0);
	scgp->silent--;
	if (i < 0)
		return (-1);
	amt = sizeof (fbuf) - scg_getresid(scgp);

	p = fbuf;
	pend = &p[sizeof (fbuf) - scg_getresid(scgp)];
	flen = a_to_u_4_byte(p);
	if ((flen+4) < amt)
		amt = flen+4;
	pend = &p[amt];
	if (xdebug > 1)
		scg_prbytes(_("Features: "), fbuf, amt);

	feature = a_to_u_2_byte(&p[6]);
	if (xdebug > 0)
		printf(_("feature len: %d current profile 0x%04X len %lld\n"),
				flen, feature, (Llong)(pend - p));

	p = fbuf + 8;	/* Skip feature header	*/
	while (p < pend) {
		int	col;

		col = 0;
		feature = a_to_u_2_byte(p);
		if (xdebug > 0)
			col += printf(_("Feature: 0x%04X "), feature);
		else
			col += printf(_("Feature: "));
		if (fname_known(feature))
			col += printf("'%s' ", fname(feature));
		else
			col += printf("0x%04X ", feature);
		col += printf("%s %s",
			p[2] & 1 ? _("(current)"):"",
			p[2] & 2 ? _("(persistent)"):"");

		if (feature == 0x108)
			col += printf(_("	Serial: '%.*s'"), p[3], &p[4]);
		if (xdebug > 1 && p[3]) {
			if (col < 50)
				printf("%*s", 50-col, "");
			scg_fprbytes(stdout, _(" Data: "), &p[4], p[3]);
		} else {
			printf("\n");
		}
		p += p[3];
		p += 4;
	}
	return (0);
}

LOCAL char *fdt[] = {
	"Reserved (0)",
	"Unformated or Blank Media",
	"Formatted Media",
	"No Media Present or Unknown Capacity"
};

EXPORT void
print_format_capacities(scgp)
	SCSI	*scgp;
{
	Uchar	b[1024];
	int	i;
	Uchar	*p;

	fillbytes(b, sizeof (b), '\0');
	scgp->silent++;
	i = read_format_capacities(scgp, (char *)b, sizeof (b));
	scgp->silent--;
	if (i < 0)
		return;

	i = b[3] + 4;
	fillbytes(b, sizeof (b), '\0');
	if (read_format_capacities(scgp, (char *)b, i) < 0)
		return;

	if (xdebug > 0) {
		i = b[3] + 4;
		scg_prbytes(_("Format cap: "), b, i);
	}
	i = b[3];
	if (i > 0) {
		int	cnt;
		UInt32_t n1;
		UInt32_t n2;
		printf(_("\n    Capacity  Blklen/Sparesz.  Format-type  Type\n"));
		for (p = &b[4]; i > 0; i -= 8, p += 8) {
			cnt = 0;
			n1 = a_to_u_4_byte(p);
			n2 = a_to_u_3_byte(&p[5]);
			printf("%12lu %16lu         0x%2.2X  %s\n",
				(Ulong)n1, (Ulong)n2,
				(p[4] >> 2) & 0x3F,
				fdt[p[4] & 0x03]);
		}
	}
}

EXPORT int
get_format_capacities(scgp, bp, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
{
	int			len = sizeof (struct scsi_format_cap_header);
	struct scsi_format_cap_header	*hp;

	fillbytes(bp, cnt, '\0');
	if (cnt < len)
		return (-1);
	scgp->silent++;
	if (read_format_capacities(scgp, bp, len) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	if (scg_getresid(scgp) > 0)
		return (-1);

	hp = (struct scsi_format_cap_header *)bp;
	len = hp->len;
	len += sizeof (struct scsi_format_cap_header);
	while (len > cnt)
		len -= sizeof (struct scsi_format_cap_desc);

	if (read_format_capacities(scgp, bp, len) < 0)
		return (-1);

	len -= scg_getresid(scgp);
	return (len);
}

EXPORT int
read_format_capacities(scgp, bp, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x23;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read_format_capacities";

	return (scg_cmd(scgp));
}

EXPORT void
przone(rp)
	struct rzone_info *rp;
{
	int	rsize = a_to_2_byte(rp->data_len)+2;

	if (rsize < 12)
		return;
	printf(_("rzone size:         %d\n"), rsize);
	printf(_("rzone number:       %d\n"), rp->rzone_num_msb * 256 + rp->rzone_num_lsb);
	printf(_("border number:      %d\n"), rp->border_num_msb * 256 + rp->border_num_lsb);
	printf(_("ljrs:               %d\n"), rp->ljrs);
	printf(_("track mode:         %d copy: %d\n"), rp->trackmode, rp->copy);
	printf(_("damage:             %d\n"), rp->damage);
	printf(_("reserved track:     %d blank: %d incremental: %d fp: %d\n"),
						rp->rt, rp->blank,
						rp->incremental, rp->fp);
	printf(_("data mode:          %d\n"), rp->datamode);
	printf(_("lra valid:          %d\n"), rp->lra_v);
	printf(_("nwa valid:          %d\n"), rp->nwa_v);
	printf(_("rzone start:        %ld\n"), a_to_4_byte(rp->rzone_start));
	printf(_("next wr addr:       %ld\n"), a_to_4_byte(rp->next_recordable_addr));
	printf(_("free blocks:        %ld\n"), a_to_4_byte(rp->free_blocks));
	printf(_("blocking factor:    %ld\n"), a_to_4_byte(rp->block_factor));
	printf(_("rzone size:         %ld\n"), a_to_4_byte(rp->rzone_size));
	printf(_("last recorded addr: %ld\n"), a_to_4_byte(rp->last_recorded_addr));
	if (rsize < 40)
		return;
	printf(_("read compat lba:    %ld\n"), a_to_4_byte(rp->read_compat_lba));
	if (rsize < 44)
		return;
	printf(_("next layerjmp addr: %ld\n"), a_to_4_byte(rp->next_layer_jump));
	if (rsize < 48)
		return;
	printf(_("last layerjmp addr: %ld\n"), a_to_4_byte(rp->last_layer_jump));
}

EXPORT int
get_diskinfo(scgp, dip, cnt)
	SCSI		*scgp;
	struct disk_info *dip;
	int		cnt;
{
	int	len;
	int	ret;

	fillbytes((caddr_t)dip, cnt, '\0');

	/*
	 * Used to be 2 instead of 4 (now). But some Y2k ATAPI drives as used
	 * by IOMEGA create a DMA overrun if we try to transfer only 2 bytes.
	 */
	if (read_disk_info(scgp, (caddr_t)dip, 4) < 0)
		return (-1);
	len = a_to_u_2_byte(dip->data_len);
	len += 2;
	if (len > cnt)
		len = cnt;
	ret = read_disk_info(scgp, (caddr_t)dip, len);

#ifdef	DEBUG
	if (lverbose > 1)
		scg_prbytes(_("Disk info:"), (Uchar *)dip,
				len-scg_getresid(scgp));
#endif
	return (ret);
}

#define	IS(what, flag)		printf(_("Disk Is %s%s\n"), flag?"":_("not "), what);

LOCAL	char	res[] = "reserved";

EXPORT char *
get_ses_type(ses_type)
	int	ses_type;
{
static	char	ret[16];

	switch (ses_type) {

	case SES_DA_ROM:	return ("CD-DA or CD-ROM");
	case SES_CDI:		return ("CDI");
	case SES_XA:		return ("CD-ROM XA");
	case SES_UNDEF:		return ("undefined");
	default:
				js_snprintf(ret, sizeof (ret), "%s: 0x%2.2X",
							res, ses_type);
				return (ret);
	}
}

EXPORT void
print_diskinfo(dip, is_cd)
	struct disk_info	*dip;
	BOOL			is_cd;
{
static	char *dt_name[] = { "standard", "track resources", "POW resources", res, res, res, res, res };
static	char *ds_name[] = { "empty", "incomplete/appendable", "complete", "illegal" };
static	char *ss_name[] = { "empty", "incomplete/appendable", "illegal", "complete", };
static	char *fd_name[] = { "none", "incomplete", "in progress", "completed", };

	IS(_("erasable"), dip->erasable);
	printf(_("data type:                %s\n"), dt_name[dip->dtype]);
	printf(_("disk status:              %s\n"), ds_name[dip->disk_status]);
	printf(_("session status:           %s\n"), ss_name[dip->sess_status]);
	printf(_("BG format status:         %s\n"), fd_name[dip->bg_format_stat]);
	printf(_("first track:              %d\n"),
		dip->first_track);
	printf(_("number of sessions:       %d\n"),
		dip->numsess + dip->numsess_msb * 256);
	printf(_("first track in last sess: %d\n"),
		dip->first_track_ls + dip->first_track_ls_msb * 256);
	printf(_("last track in last sess:  %d\n"),
		dip->last_track_ls + dip->last_track_ls_msb * 256);
	IS(_("unrestricted"), dip->uru);
	printf(_("Disk type: "));
	if (is_cd) {
		printf("%s", get_ses_type(dip->disk_type));
	} else {
		printf(_("DVD, HD-DVD or BD"));
	}
	printf("\n");
	if (dip->did_v)
		printf(_("Disk id: 0x%lX\n"), a_to_u_4_byte(dip->disk_id));

	if (is_cd) {
		printf(_("last start of lead in: %ld\n"),
			msf_to_lba(dip->last_lead_in[1],
			dip->last_lead_in[2],
			dip->last_lead_in[3], FALSE));
		printf(_("last start of lead out: %ld\n"),
			msf_to_lba(dip->last_lead_out[1],
			dip->last_lead_out[2],
			dip->last_lead_out[3], TRUE));
	}

	if (dip->dbc_v)
		printf(_("Disk bar code: 0x%lX%lX\n"),
			a_to_u_4_byte(dip->disk_barcode),
			a_to_u_4_byte(&dip->disk_barcode[4]));

	if (dip->dac_v)
		printf(_("Disk appl. code: %d\n"), dip->disk_appl_code);

	if (dip->num_opc_entries > 0) {
		printf(_("OPC table:\n"));
	}
}

EXPORT int
prdiskstatus(scgp, dp, is_cd)
	SCSI	*scgp;
	cdr_t	*dp;
	BOOL	is_cd;
{
	struct disk_info	di;
	struct rzone_info	rz;
	int			sessions;
	int			track;
	int			tracks;
	int			t;
	int			s;
	long			raddr;
	long			lastaddr = -1;
	long			lastsess = -1;
	long			leadout = -1;
	long			lo_sess = 0;
	long			nwa = -1;
	long			rsize = -1;
	long			border_size = -1;
	int			profile;

	profile = get_curprofile(scgp);
	if (profile > 0) {
		int mt = get_mediatype(scgp);

		printf(_("Mounted media class:      %s\n"),
				get_mclassname(mt));
		if (pname_known(profile)) {
			printf(_("Mounted media type:       %s\n"),
				pname(profile));
		}
	}
	get_diskinfo(scgp, &di, sizeof (di));
	print_diskinfo(&di, is_cd);

	sessions = di.numsess + di.numsess_msb * 256;
	tracks = di.last_track_ls + di.last_track_ls_msb * 256;

	printf(_("\nTrack  Sess Type   Start Addr End Addr   Size\n"));
	printf(_("==============================================\n"));
	fillbytes((caddr_t)&rz, sizeof (rz), '\0');
	for (t = di.first_track; t <= tracks; t++) {
		fillbytes((caddr_t)&rz, sizeof (rz), '\0');
		get_trackinfo(scgp, (caddr_t)&rz, TI_TYPE_TRACK, t, sizeof (rz));
		if (lverbose > 1)
			przone(&rz);
		track = rz.rzone_num_lsb + rz.rzone_num_msb * 256;
		s = rz.border_num_lsb + rz.border_num_msb * 256;
		raddr = a_to_4_byte(rz.rzone_start);
		if (rsize >= 0)
			border_size = raddr - (lastaddr+rsize);
		if (!rz.blank && s > lastsess) { /* First track in last sess ? */
			lastaddr = raddr;
			lastsess = s;
		}
		nwa = a_to_4_byte(rz.next_recordable_addr);
		rsize = a_to_4_byte(rz.rzone_size);
		if (!rz.blank) {
			leadout = raddr + rsize;
			lo_sess = s;
		}
		printf("%5d %5d %-6s %-10ld %-10ld %ld",
			track, s,
			rz.blank ? _("Blank") :
				rz.trackmode & 4 ? _("Data") : _("Audio"),
			raddr, raddr + rsize -1, rsize);
		if (lverbose > 0)
			printf(" %10ld", border_size);
		printf("\n");
	}
	printf("\n");
	if (lastaddr >= 0)
		printf(_("Last session start address:         %ld\n"), lastaddr);
	if (leadout >= 0)
		printf(_("Last session leadout start address: %ld\n"), leadout);
	if (rz.nwa_v) {
		printf(_("Next writable address:              %ld\n"), nwa);
		printf(_("Remaining writable size:            %ld\n"), rsize);
	}

	return (0);
}

EXPORT int
sessstatus(scgp, is_cd, offp, nwap)
	SCSI	*scgp;
	BOOL	is_cd;
	long	*offp;
	long	*nwap;
{
	struct disk_info	di;
	struct rzone_info	rz;
	int			sessions;
	int			track;
	int			tracks;
	int			t;
	int			s;
	long			raddr;
	long			lastaddr = -1;
	long			lastsess = -1;
	long			leadout = -1;
	long			lo_sess = 0;
	long			nwa = -1;
	long			rsize = -1;
	long			border_size = -1;


	if (get_diskinfo(scgp, &di, sizeof (di)) < 0)
		return (-1);

	sessions = di.numsess + di.numsess_msb * 256;
	tracks = di.last_track_ls + di.last_track_ls_msb * 256;

	fillbytes((caddr_t)&rz, sizeof (rz), '\0');
	for (t = di.first_track; t <= tracks; t++) {
		fillbytes((caddr_t)&rz, sizeof (rz), '\0');
		if (get_trackinfo(scgp, (caddr_t)&rz, TI_TYPE_TRACK, t, sizeof (rz)) < 0)
			return (-1);
		track = rz.rzone_num_lsb + rz.rzone_num_msb * 256;
		s = rz.border_num_lsb + rz.border_num_msb * 256;
		raddr = a_to_4_byte(rz.rzone_start);
		if (rsize >= 0)
			border_size = raddr - (lastaddr+rsize);
		if (!rz.blank && s > lastsess) { /* First track in last sess ? */
			lastaddr = raddr;
			lastsess = s;
		}
		nwa = a_to_4_byte(rz.next_recordable_addr);
		rsize = a_to_4_byte(rz.rzone_size);
		if (!rz.blank) {
			leadout = raddr + rsize;
			lo_sess = s;
		}
	}
	if (lastaddr >= 0 && offp != NULL)
		*offp = lastaddr;

	if (rz.nwa_v && nwap != NULL)
		*nwap = nwa;

	return (0);
}

EXPORT void
print_performance_mmc(scgp)
	SCSI	*scgp;
{
	Ulong	reads;
	Ulong	writes;
	Ulong	ends = 0x7FFFFFFF;
	int	oxdebug = xdebug;

	/*
	 * Do not try to fail with old drives...
	 */
	if (get_curprofile(scgp) < 0)
		return;

	if (xdebug == 0)
		xdebug = -1;

	printf(_("\nCurrent performance according to MMC get performance:\n"));
	scsi_get_perf_curspeed(scgp, &reads, &writes, &ends);

	printf(_("\nMaximum performance according to MMC get performance:\n"));
	scsi_get_perf_maxspeed(scgp, &reads, &writes, &ends);

	xdebug = oxdebug;
}
