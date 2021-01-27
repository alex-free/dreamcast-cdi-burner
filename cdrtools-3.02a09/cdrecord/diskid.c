/* @(#)diskid.c	1.44 10/12/19 Copyright 1998-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)diskid.c	1.44 10/12/19 Copyright 1998-2010 J. Schilling";
#endif
/*
 *	Disk Idientification Method
 *
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

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include "cdrecord.h"

EXPORT	void	pr_manufacturer		__PR((msf_t *mp, BOOL rw, BOOL audio));
LOCAL	struct disk_man * man_ptr	__PR((msf_t *mp));
EXPORT	int	manufacturer_id		__PR((msf_t *mp));
EXPORT	long	disk_rcap		__PR((msf_t *mp, long maxblock, BOOL rw, BOOL audio));

struct disk_man {
	msf_t	mi_msf;
	char	mi_num;
	char	*mi_name;
};

/*
 * Illegal (old) Manufacturer.
 */
LOCAL	char	m_ill[]   = "Unknown old Manufacturer code";
LOCAL	char	m_illrw[] = "Illegal Manufacturer code";

/*
 * Permanent codes.
 */
LOCAL	char	m_kingpro[]	= "King Pro Mediatek Inc.";
LOCAL	char	m_custpo[]	= "Customer Pressing Oosterhout";
LOCAL	char	m_taeil[]	= "Taeil Media Co.,Ltd.";
LOCAL	char	m_doremi[]	= "Doremi Media Co., Ltd.";
LOCAL	char	m_xcitec[]	= "Xcitec Inc.";
LOCAL	char	m_leaddata[]	= "Lead Data Inc.";
LOCAL	char	m_fuji[]	= "FUJI Photo Film Co., Ltd.";
LOCAL	char	m_hitachi[]	= "Hitachi Maxell, Ltd.";
LOCAL	char	m_kodakjp[]	= "Kodak Japan Limited";
LOCAL	char	m_mitsui[]	= "Mitsui Chemicals, Inc.";
LOCAL	char	m_pioneer[]	= "Pioneer Video Corporation";
LOCAL	char	m_plasmon[]	= "Plasmon Data systems Ltd.";
LOCAL	char	m_princo[]	= "Princo Corporation";
LOCAL	char	m_ricoh[]	= "Ricoh Company Limited";
LOCAL	char	m_skc[]		= "SKC Co., Ltd.";
LOCAL	char	m_tyuden[]	= "Taiyo Yuden Company Limited";
LOCAL	char	m_tdk[]		= "TDK Corporation";
LOCAL	char	m_mitsubishi[]	= "Mitsubishi Chemical Corporation";
LOCAL	char	m_auvistar[]	= "Auvistar Industry Co.,Ltd.";
LOCAL	char	m_gigastore[]	= "GIGASTORAGE CORPORATION";
LOCAL	char	m_fornet[]	= "FORNET INTERNATIONAL PTE LTD.";
LOCAL	char	m_cmc[]		= "CMC Magnetics Corporation";
LOCAL	char	m_odm[]		= "Optical Disc Manufacturing Equipment";
LOCAL	char	m_ritek[]	= "Ritek Co.";

/*
 * Tentative codes.
 */
LOCAL	char	m_bestdisk[]	= "Bestdisc Technology Corporation";
LOCAL	char	m_wealth_fair[]	= "WEALTH FAIR INVESTMENT LIMITED";
LOCAL	char	m_general_mag[]	= "General Magnetics Ld";
LOCAL	char	m_mpo[]		= "MPO";
LOCAL	char	m_jvc[]		= "VICTOR COMPANY OF JAPAN, LIMITED";
LOCAL	char	m_vivistar[]	= "VIVASTAR AG";
LOCAL	char	m_taroko[]	= "TAROKO INTERNATIONAL CO.,LTD.";
LOCAL	char	m_unidisc[]	= "UNIDISC TECHNOLOGY CO.,LTD";
LOCAL	char	m_hokodig[]	= "Hong Kong Digital Technology Co., Ltd.";
LOCAL	char	m_viva[]	= "VIVA MAGNETICS LIMITED";
LOCAL	char	m_hile[]	= "Hile Optical Disc Technology Corp.";
LOCAL	char	m_friendly[]	= "Friendly CD-Tek Co.";
LOCAL	char	m_soundsound[]	= "Sound Sound Multi-Media Development Limited";
LOCAL	char	m_kdg[]		= "kdg mediatech AG";
LOCAL	char	m_seantram[]	= "Seantram Technology Inc.";
LOCAL	char	m_eximpo[]	= "EXIMPO";
LOCAL	char	m_delphi[]	= "DELPHI TECHNOLOGY INC.";
LOCAL	char	m_harmonic[]	= "Harmonic Hall Optical Disc Ltd.";
LOCAL	char	m_guannyinn[]	= "Guann Yinn Co.,Ltd.";
LOCAL	char	m_optime[]	= "Opti.Me.S. S.p.A.";
LOCAL	char	m_nacar[]	= "Nacar Media srl";
LOCAL	char	m_optrom[]	= "OPTROM.INC.";
LOCAL	char	m_audiodis[]	= "AUDIO DISTRIBUTORS CO., LTD.";
LOCAL	char	m_acer[]	= "Acer Media Technology, Inc.";
LOCAL	char	m_woongjin[]	= "Woongjin Media corp";
LOCAL	char	m_infodisk[]	= "INFODISC Technology Co., Ltd.";
LOCAL	char	m_unitech[]	= "UNITECH JAPAN INC.";
LOCAL	char	m_ams[]		= "AMS Technology Inc.";
LOCAL	char	m_vanguard[]	= "Vanguard Disc Inc.";
LOCAL	char	m_grandadv[]	= "Grand Advance Technology Ltd.";
LOCAL	char	m_digitalstor[]	= "DIGITAL STORAGE TECHNOLOGY CO.,LTD";
LOCAL	char	m_matsushita[]	= "Matsushita Electric Industrial Co.,Ltd.";
LOCAL	char	m_albrechts[]	= "CDA Datenträger Albrechts GmbH.";
LOCAL	char	m_xalbrechts[]	= "??? CDA Datenträger Albrechts GmbH.";

LOCAL	char	m_prodisc[]	= "Prodisc Technology Inc.";
LOCAL	char	m_postech[]	= "POSTECH Corporation";
#ifdef	used
LOCAL	char	m_ncolumbia[]	= "NIPPON COLUMBIA CO.,LTD.";
#endif
LOCAL	char	m_odc[]		= "OPTICAL DISC CORPRATION";
LOCAL	char	m_sony[]	= "SONY Corporation";
LOCAL	char	m_cis[]		= "CIS Technology Inc.";
LOCAL	char	m_csitaly[]	= "Computer Support Italy s.r.l.";
LOCAL	char	m_mmmm[]	= "Multi Media Masters & Machinary SA";

/*
 * Guessed codes.
 */
/*LOCAL	char	m_seantram[]	= "Seantram Technology Inc.";*/
LOCAL	char	m_advanced[]	= "Advanced Digital Media";
LOCAL	char	m_moser[]	= "Moser Baer India Limited";
LOCAL	char	m_nanya[]	= "NAN-YA Plastics Corporation";
LOCAL	char	m_shenzen[]	= "SHENZEN SG&GAST DIGITAL OPTICAL DISCS";

LOCAL	struct disk_man notable =
	{{00, 00, 00},  -1, "unknown (not in table)" };

/*
 * Old (illegal) code table. It lists single specific codes (97:xx:yy).
 */
LOCAL	struct disk_man odman[] = {
	/*
	 * Illegal (old) codes.
	 */
	{{97, 25, 00}, 80, "ILLEGAL OLD CODE: TDK ???" },
	{{97, 25, 15},  0, m_ill },
	{{97, 27, 00}, 81, "ILLEGAL OLD CODE: Old Ritek Co.???" },
	{{97, 27, 25},  0, m_ill },
	{{97, 30, 00},  0, m_ill },
	{{97, 33, 00}, 82, "ILLEGAL OLD CODE: Old CDA Datenträger Albrechts GmbH." },
	{{97, 35, 44},  0, m_ill },
	{{97, 39, 00},  0, m_ill },
	{{97, 45, 36}, 83, "ILLEGAL OLD CODE: Old Kodak Photo CD" },
	{{97, 47, 00},  0, m_ill },
	{{97, 47, 30},  0, m_ill },
	{{97, 48, 14},  0, m_ill },
	{{97, 48, 33},  0, m_ill },
	{{97, 49, 00},  0, m_ill },
	{{97, 54, 00},  0, m_ill },
	{{97, 55, 06},  0, m_ill },
	{{97, 57, 00},  0, m_ill },
	/*
	 * List end marker
	 */
	{{00, 00, 00}, 0, NULL },
};

#define	noman	(sizeof (oman)/sizeof (oman[0]))

/*
 * Actual code table. It lists code ranges (97:xx:y0 - 97:xx:y9).
 *
 * Note that dp->mi_msf.msf_frame needs to be always rounded down
 * to 0 even for media that has e.g. 97:27/01 in the official table.
 */
LOCAL	struct disk_man dman[] = {
	/*
	 * Permanent codes.
	 */

	{{97, 22, 10}, 53, m_seantram },
	{{97, 15, 00}, 26, m_tdk },
	{{97, 49, 30}, 47, m_optime },
	{{97, 28, 00}, 47, m_optime },
	{{97, 28, 40}, 36, m_kingpro },
	{{97, 23, 60}, 49, m_custpo },
	{{97, 29, 00}, 37, m_taeil },
	{{97, 26, 10}, 19, m_postech },
	{{97, 47, 40}, 19, m_postech },
	{{97, 24, 10}, 24, m_sony },
/*	{{97, 46, 10}, 24, m_sony },*/
	{{97, 23, 10}, 33, m_doremi },
	{{97, 25, 60}, 30, m_xcitec },
	{{97, 45, 60}, 30, m_xcitec },
	{{97, 26, 50}, 10, m_leaddata },
	{{97, 48, 60}, 10, m_leaddata },
	{{97, 26, 40},  6, m_fuji },
	{{97, 46, 40},  6, m_fuji },
	{{97, 25, 20},  8, m_hitachi },
	{{97, 47, 10},  8, m_hitachi },
	{{97, 27, 40},  9, m_kodakjp },
	{{97, 48, 10},  9, m_kodakjp },
	{{97, 27, 50}, 12, m_mitsui },
	{{97, 48, 50}, 12, m_mitsui },
	{{97, 27, 30}, 17, m_pioneer },
	{{97, 48, 30}, 17, m_pioneer },
	{{97, 27, 10}, 18, m_plasmon },
	{{97, 48, 20}, 18, m_plasmon },
	{{97, 27, 20}, 20, m_princo },
	{{97, 47, 20}, 20, m_princo },
	{{97, 27, 60}, 21, m_ricoh },
	{{97, 48, 00}, 21, m_ricoh },
	{{97, 26, 20}, 23, m_skc },
	{{97, 24, 00}, 25, m_tyuden },
	{{97, 46, 00}, 25, m_tyuden },
	{{97, 32, 00}, 26, m_tdk },
	{{97, 49, 00}, 26, m_tdk },
	{{97, 34, 20}, 11, m_mitsubishi },
	{{97, 50, 20}, 11, m_mitsubishi },
	{{97, 28, 30},  1, m_auvistar },
	{{97, 46, 50},  1, m_auvistar },
	{{97, 28, 10},  7, m_gigastore },
	{{97, 49, 10},  7, m_gigastore },
	{{97, 26, 00},  5, m_fornet },
	{{97, 45, 00},  5, m_fornet },
	{{97, 26, 60},  3, m_cmc },
	{{97, 46, 60},  3, m_cmc },
	{{97, 21, 40}, 16, m_odm },
	{{97, 31, 00}, 22, m_ritek },
	{{97, 47, 50}, 22, m_ritek },
	{{97, 28, 20}, 13, m_mmmm },
	{{97, 46, 20}, 13, m_mmmm },
	{{97, 32, 10}, 27, m_prodisc },

	/*
	 * Tentative codes.
	 */
	{{97, 21, 30}, 67, m_bestdisk },
	{{97, 18, 10}, 66, m_wealth_fair },
	{{97, 29, 50}, 65, m_general_mag },
	{{97, 25, 00}, 64, m_mpo },		/* in reality 25/01    */
	{{97, 49, 40}, 63, m_jvc },
	{{97, 23, 40}, 63, m_jvc },
	{{97, 25, 40}, 62, m_vivistar },
	{{97, 18, 60}, 61, m_taroko },
	{{97, 29, 20}, 60, m_unidisc },
	{{97, 46, 10}, 59, m_hokodig },		/* XXX was m_sony */
	{{97, 22, 50}, 59, m_hokodig },
	{{97, 29, 40}, 58, m_viva },
	{{97, 29, 30}, 57, m_hile },
	{{97, 51, 50}, 57, m_hile },
	{{97, 28, 60}, 56, m_friendly },
	{{97, 21, 50}, 55, m_soundsound },
	{{97, 24, 40}, 54, m_kdg },
	{{97, 22, 30}, 52, m_eximpo },
	{{97, 28, 50}, 51, m_delphi },
	{{97, 29, 00}, 50, m_harmonic },
	{{97, 15, 10}, 22, m_ritek },
	{{97, 45, 50}, 48, m_guannyinn },
	{{97, 24, 50}, 48, m_guannyinn },
	{{97, 23, 20}, 46, m_nacar },
	{{97, 23, 50}, 45, m_optrom },
	{{97, 23, 30}, 44, m_audiodis },
	{{97, 22, 60}, 43, m_acer },
	{{97, 45, 20}, 43, m_acer },
	{{97, 15, 20}, 11, m_mitsubishi },
	{{97, 22, 00}, 39, m_woongjin },
	{{97, 25, 30}, 40, m_infodisk },
	{{97, 51, 20}, 40, m_infodisk },
	{{97, 24, 30}, 41, m_unitech },
	{{97, 25, 50}, 42, m_ams },
	{{97, 29, 10}, 38, m_vanguard },
	{{97, 50, 10}, 38, m_vanguard },
	{{97, 16, 30}, 35, m_grandadv },
	{{97, 31, 30}, 35, m_grandadv },
	{{97, 51, 10}, 35, m_grandadv },
	{{97, 49, 20}, 36, m_kingpro },
	{{97, 27, 00}, 34, m_digitalstor },	/* in reality 27/01    */
	{{97, 48, 40}, 34, m_digitalstor },	/* XXX was m_ncolumbia */
	{{97, 23, 00}, 31, m_matsushita },
	{{97, 49, 60}, 31, m_matsushita },
	{{97, 30, 10}, 32, m_albrechts },	/* XXX was m_ncolumbia */
	{{97, 50, 30}, 32, m_albrechts },
	{{97, 47, 60}, 27, m_prodisc },
/*	{{97, 30, 10}, 14, m_ncolumbia },*/
/*	{{97, 48, 40}, 14, m_ncolumbia },*/
	{{97, 26, 30}, 15, m_odc },
	{{97, 22, 40},  2, m_cis },
	{{97, 45, 40},  2, m_cis },
	{{97, 24, 20},  4, m_csitaly },
	{{97, 46, 30},  4, m_csitaly },

	/*
	 * Guessed codes.
	 */
	{{97, 20, 10}, 32, m_xalbrechts },			/* XXX guess */
/*	{{97, 23, 40}, 32, m_xalbrechts },*/			/* Really is JVC */

	/*
	 * New guessed codes (2002 ff.).
	 * Id code >= 68 referres to a new manufacturer.
	 */
#define	I_GUESS	105
	{{97, 22, 20}, 68, m_advanced },
	{{97, 42, 20}, 68, m_advanced },
	{{97, 24, 60}, 50, m_harmonic },
	{{97, 17, 00}, 69, m_moser },
	{{97, 15, 30}, 70, m_nanya },
	{{97, 16, 20}, 71, m_shenzen },
	{{97, 45, 10}, 41, m_unitech },

	/*
	 * List end marker
	 */
	{{00, 00, 00},  0, NULL },
};

#define	ndman	(sizeof (dman)/sizeof (dman[0]))

LOCAL struct disk_man *
man_ptr(mp)
	msf_t	*mp;
{
	struct disk_man * dp;
	int	frame;
	int	type;

	type = mp->msf_frame % 10;
	frame = mp->msf_frame - type;

	dp = odman;
	while (dp->mi_msf.msf_min != 0) {
		if (mp->msf_min == dp->mi_msf.msf_min &&
				mp->msf_sec == dp->mi_msf.msf_sec &&
				mp->msf_frame == dp->mi_msf.msf_frame) {
			return (dp);
		}
		dp++;
	}
	dp = dman;
	while (dp->mi_msf.msf_min != 0) {
		if (mp->msf_min == dp->mi_msf.msf_min &&
				mp->msf_sec == dp->mi_msf.msf_sec &&
				frame == dp->mi_msf.msf_frame) {
			/*
			 * Note that dp->mi_msf.msf_frame is always rounded
			 * down to 0 even for media that has 97:27/01 in the
			 * official table.
			 */
			return (dp);
		}
		dp++;
	}
	return (NULL);
}

EXPORT void
pr_manufacturer(mp, rw, audio)
	msf_t	*mp;
	BOOL	rw;
	BOOL	audio;
{
	struct disk_man * dp;
	struct disk_man xdman;
	int	type;
	char	*tname;

/*	printf("pr_manufacturer rw: %d audio: %d\n", rw, audio);*/

	type = mp->msf_frame % 10;
	if (type < 5) {
		tname = _("Long strategy type (Cyanine, AZO or similar)");
	} else {
		tname = _("Short strategy type (Phthalocyanine or similar)");
	}
	if (rw) {
		tname = _("Phase change");
	}

	dp = man_ptr(mp);
	if (dp != NULL) {
		if (dp->mi_num == 0 || dp->mi_num >= 80) {
			if (!rw) {
				tname = _("unknown dye (old id code)");
			} else {
				xdman = *dp;
				dp = &xdman;
				dp->mi_num = 0;
				dp->mi_name = m_illrw;
			}
		}
	} else {
		tname = _("unknown dye (reserved id code)");
		dp = &notable;
	}
	printf(_("Disk type:    %s\n"), tname);
	printf(_("Manuf. index: %d\n"), dp->mi_num);
	printf(_("Manufacturer: %s\n"), dp->mi_name);

	if (mp->msf_min != 97)	/* This may be garbage ATIP from a DVD */
		return;

	if (dp >= &dman[I_GUESS] && dp < &dman[ndman]) {
		printf(_("Manufacturer is guessed because of the orange forum embargo.\n"));
		printf(_("The orange forum likes to get money for recent information.\n"));
		printf(_("The information for this media may not be correct.\n"));
	}
	if (dp == &notable) {
		printf(_("Manufacturer is unknown because of the orange forum embargo.\n"));
		printf(_("As the orange forum likes to get money for recent information,\n"));
		printf(_("it may be that this media does not use illegal manufacturer coding.\n"));
	}
}

EXPORT int
manufacturer_id(mp)
	msf_t	*mp;
{
	struct disk_man * dp;

	dp = man_ptr(mp);
	if (dp != NULL)
		return (dp->mi_num);
	return (-1);
}

struct disk_rcap {
	msf_t	ci_msf;				/* Lead in start time	    */
	long	ci_cap;				/* Lead out start time	    */
	long	ci_rcap;			/* Abs max lead out start   */
};

LOCAL	struct disk_rcap rcap[] = {

#ifdef	__redbook_only__
	{{97, 35, 44}, 359849, 404700 },	/*! Unknown 99 min (89:58/00)*/
#endif
	{{97, 35, 44}, 359849, 449700 },	/*! Unknown 99 min (99:58/00) */
	{{97, 31, 00}, 359849, 368923 },	/*! Arita CD-R 80	    */
	{{97, 26, 50}, 359849, 369096 },	/*! Lead Data CD-R 80	    */
	{{97, 26, 12}, 359849, 368000 },	/*X POSTECH 80 Min	    */
	{{97, 25, 00}, 359849, 374002 },	/* TDK 80 Minuten	    */
	{{97, 20, 14}, 359700, 376386 },	/*! Albrechts DataFile Plus */
	{{97, 35, 44}, 359100, 368791 },	/*! NoName BC-1 700 Mb/80 Min */

	{{97, 26, 60}, 337350, 349030 },	/* Koch grün CD-R74PRO	    */
	{{97, 26, 50}, 337050, 351205 },	/* Saba			    */
	{{97, 26, 00}, 337050, 351411 },	/*!DGN (FORNET)		    */
	{{97, 22, 40}, 336631, 349971 },	/* Targa grün CD-R74	    */
	{{97, 26, 50}, 336631, 351727 },	/*! Sunstar (Lead Data)	    */
	{{97, 26, 55}, 336631, 350474 },	/*! NoName ZAP (Lead Data)  */

	{{97, 27, 28}, 336601, 346489 },	/*! BTC CD-R (Princo)	    */
	{{97, 27, 30}, 336601, 351646 },	/*! Pioneer blau CDM-W74S   */
	{{97, 27, 31}, 336601, 351379 },	/* Pioneer blau CDM-W74S    */
	{{97, 27, 33}, 336601, 347029 },	/*! Pioneer braun CDM-V74S  */
	{{97, 26, 40}, 336225, 346210 },	/* Fuji Silver Disk	    */
	{{97, 28, 10}, 336225, 348757 },	/*!GigaStorage Cursor CD-R  */
	{{97, 31, 00}, 336225, 345460 },	/* Arita grün		    */
	{{97, 25, 28}, 336075, 352879 },	/* Maxell gold CD-R74G	    */
	{{97, 24, 01}, 336075, 346856 },	/*!Philips Premium Silver   */
	{{97, 24, 00}, 336075, 346741 },	/* Philips grün CD-R74	    */

	{{97, 22, 41}, 335206, 349385 },	/* Octek grün		    */
	{{97, 34, 20}, 335100, 342460 },	/* Verbatim DataLifePlus    */
	{{97, 33, 00}, 335100, 344634 },	/*!ITS Singapore (braun/grün) */
	{{97, 32, 19}, 335100, 343921 },	/*!Prodisc silber/silber    */
	{{97, 25, 21}, 335100, 346013 },	/* Maxell grün CD-R74XL	    */
	{{97, 27, 00}, 335100, 353448 },	/* TDK grün CD-RXG74	    */
	{{97, 27, 31}, 335100, 351862 },	/*!Maxell CD-R74MU (Musik)  */
	{{97, 27, 33}, 335100, 351336 },	/* Pioneer RDD-74A	    */

	{{97, 26, 60}, 334259, 349036 },	/* BASF grün		    */
	{{97, 28, 21}, 333976, 348217 },	/*! Noname-B (MMMM)	    */
	{{97, 28, 20}, 333976, 346485 },	/* Koch  grün  CD-R74 PRO   */
	{{97, 32, 00}, 333975, 345736 },	/* Imation 3M		    */
	{{97, 32, 00}, 333975, 348835 },	/* TDK Reflex X	    CD-R74  */
	{{97, 30, 18}, 333899, 344857 },	/* HiSpace  grün	    */
	{{97, 27, 66}, 333750, 352726 },	/*!Philips Megalife (Musik) */
	{{97, 28, 43}, 333750, 345344 },	/*!MMore CD-R		    */
	{{97, 27, 65}, 333750, 348343 },	/* Ricoh gold		    */

	{{97, 27, 00}, 333750, 336246 },	/* BestMedia grün   CD-R74  */
	{{97, 27, 28}, 333491, 347473 },	/* Fuji grün (alt)	    */
	{{97, 24, 48}, 333491, 343519 },	/* BASF (alt)		    */
	{{97, 27, 55}, 333235, 343270 },	/* Teac gold CD-R74	    */
	{{97, 27, 45}, 333226, 343358 },	/* Kodak gold		    */
	{{97, 28, 20}, 333226, 346483 },	/* SAST grün		    */
	{{97, 27, 45}, 333226, 343357 },	/* Mitsumi gold		    */
	{{97, 28, 25}, 333226, 346481 },	/* Cedar Grün		    */
	{{97, 23, 00}, 333226, 346206 },	/* Fuji grün (alt)	    */
	{{97, 33, 00}, 333225, 349623 },	/* DataFile Albrechts	    */
	{{97, 24, 24}, 333198, 342536 },	/*!SUN CD Recordable	    */

	{{97, 27, 19}, 332850, 348442 },	/* Plasmon gold PCD-R74	    */
	{{97, 32, 00}, 96600,  106502 },	/* TDK 80mm (for music only) */

	/*
	 * List end marker
	 */
	{{00, 00, 00}, 0L, 0L },
};

EXPORT long
disk_rcap(mp, maxblock, rw, audio)
	msf_t	*mp;
	long	maxblock;
	BOOL	rw;
	BOOL	audio;
{
	struct disk_rcap * dp;

	dp = rcap;
	while (dp->ci_msf.msf_min != 0) {
		if (mp->msf_min == dp->ci_msf.msf_min &&
				mp->msf_sec == dp->ci_msf.msf_sec &&
				mp->msf_frame == dp->ci_msf.msf_frame &&
				maxblock == dp->ci_cap)
			return (dp->ci_rcap);
		dp++;
	}
	return (0L);
}
