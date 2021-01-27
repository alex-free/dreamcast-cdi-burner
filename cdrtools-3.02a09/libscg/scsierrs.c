/* @(#)scsierrs.c	2.34 10/05/24 Copyright 1987-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scsierrs.c	2.34 10/05/24 Copyright 1987-2010 J. Schilling";
#endif
/*
 *	Error printing for scsitransp.c
 *
 *	Copyright (c) 1987-2010 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * The following exceptions apply:
 * CDDL §3.6 needs to be replaced by: "You may create a Larger Work by
 * combining Covered Software with other code if all other code is governed by
 * the terms of a license that is OSI approved (see www.opensource.org) and
 * you may distribute the Larger Work as a single product. In such a case,
 * You must make sure the requirements of this License are fulfilled for
 * the Covered Software."
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/stdio.h>
#include <schily/unistd.h> /* for sys/types.h needed in schily.h for sprintf() */
#include <schily/standard.h>
#include <schily/schily.h>

#include <scg/scsireg.h>
#include <scg/scsidefs.h>
#include <scg/scgcmd.h>	/*XXX JS wird eigentlich nicht benoetigt!!	*/
			/*XXX JS kommt weg, wenn struct sense und status */
			/*XXX JS von scgio.h nach scsireg.h kommen	*/
#include <scg/scsitransp.h>

#define	CTYPE_CCS	0
#define	CTYPE_MD21	1
#define	CTYPE_ACB4000	2
#define	CTYPE_SMO_C501	3

#define	SMO_C501

EXPORT	const char	*scg_sensemsg	__PR((int, int, int,
						const char **, char *, int maxcnt));
EXPORT	int		scg__errmsg	__PR((SCSI *scgp, char *obuf, int maxcnt,
						struct scsi_sense *,
						struct scsi_status *,
						int));
#ifdef	KEY_FROM_OLD_SENSE
/*
 * Map old non extended sense to sense key.
 */
static Uchar sd_adaptec_keys[] = {
	0, 4, 4, 4,  2, 2, 4, 4,		/* 0x00-0x07 */
	4, 4, 4, 4,  4, 4, 4, 4,		/* 0x08-0x0f */
	4, 3, 3, 3,  3, 4, 3, 1,		/* 0x10-0x17 */
	1, 1, 3, 4,  3, 4, 3, 3,		/* 0x18-0x1f */
	5, 5, 5, 5,  5, 5, 5, 7,		/* 0x20-0x27 */
	6, 6, 6, 5,  4, 11, 11, 11		/* 0x28-0x2f */
};
#define	MAX_ADAPTEC_KEYS (sizeof (sd_adaptec_keys))
#endif

/*
 * Deviations to CCS found on old pre CCS devices
 */
static const char *sd_adaptec_error_str[] = {
	"\031\000ECC error during verify",		/* 0x19 */
	"\032\000interleave error",			/* 0x1a */
	"\034\000bad format on drive",			/* 0x1c */
	"\035\000self test failed",			/* 0x1d */
	"\036\000defective track",			/* 0x1e */
	"\043\000volume overflow",			/* 0x23 */
	"\053\000set limit violation",			/* 0x2b */
	"\054\000error counter overflow",		/* 0x2c */
	"\055\000initiator detected error",		/* 0x2d */
	"\056\000scsi parity error",			/* 0x2e */
	"\057\000adapter parity error",			/* 0x2f */
	NULL
};

/*
 * The sense codes of SCSI-1/CCS, SCSI-2 and SCSI-3 devices.
 */
static const char *sd_ccs_error_str[] = {
	"\000\000no additional sense information",		/* 00 00 */
	"\000\001filemark detected",				/* 00 01 */
	"\000\002end-of-partition/medium detected",		/* 00 02 */
	"\000\003setmark detected",				/* 00 03 */
	"\000\004beginning-of-partition/medium detected",	/* 00 04 */
	"\000\005end-of-data detected",				/* 00 05 */
	"\000\006i/o process terminated",			/* 00 06 */
	"\000\021audio play operation in progress",		/* 00 11 */
	"\000\022audio play operation paused",			/* 00 12 */
	"\000\023audio play operation successfully completed",	/* 00 13 */
	"\000\024audio play operation stopped due to error",	/* 00 14 */
	"\000\025no current audio status to return",		/* 00 15 */
	"\000\026operation in progress",			/* 00 16 */
	"\000\027cleaning requested",				/* 00 17 */
	"\001\000no index/sector signal",			/* 01 00 */
	"\002\000no seek complete",				/* 02 00 */
	"\003\000peripheral device write fault",		/* 03 00 */
	"\003\001no write current",				/* 03 01 */
	"\003\002excessive write errors",			/* 03 02 */
	"\004\000logical unit not ready, cause not reportable",	/* 04 00 */
	"\004\001logical unit is in process of becoming ready",	/* 04 01 */
	"\004\002logical unit not ready, initializing cmd. required",	/* 04 02 */
	"\004\003logical unit not ready, manual intervention required",	/* 04 03 */
	"\004\004logical unit not ready, format in progress",	/* 04 04 */
	"\004\005logical unit not ready, rebuild in progress",	/* 04 05 */
	"\004\006logical unit not ready, recalculation in progress",	/* 04 06 */
	"\004\007logical unit not ready, operation in progress", /* 04 07 */
	"\004\010logical unit not ready, long write in progress",	/* 04 08 */
	"\004\011logical unit not ready, self-test in progress", /* 04 09 */
	"\004\012asymmetric access code 3 (00-232) [proposed]",	/* 04 0A */
	"\004\013asymmetric access code 1 (00-232) [proposed]",	/* 04 0B */
	"\004\014asymmetric access code 2 (00-232) [proposed]",	/* 04 0C */
	"\004\020auxiliary memory code 2 (99-148) [proposed]",	/* 04 10 */
	"\005\000logical unit does not respond to selection",	/* 05 00 */
	"\006\000no reference position found",			/* 06 00 */
	"\007\000multiple peripheral devices selected",		/* 07 00 */
	"\010\000logical unit communication failure",		/* 08 00 */
	"\010\001logical unit communication time-out",		/* 08 01 */
	"\010\002logical unit communication parity error",	/* 08 02 */
	"\010\003logical unit communication crc error (ultra-dma/32)",	/* 08 03 */
	"\010\004unreachable copy target",			/* 08 04 */
	"\011\000track following error",			/* 09 00 */
	"\011\001tracking servo failure",			/* 09 01 */
	"\011\002focus servo failure",				/* 09 02 */
	"\011\003spindle servo failure",			/* 09 03 */
	"\011\004head select fault",				/* 09 04 */
	"\012\000error log overflow",				/* 0A 00 */
	"\013\000warning",					/* 0B 00 */
	"\013\001warning - specified temperature exceeded",	/* 0B 01 */
	"\013\002warning - enclosure degraded",			/* 0B 02 */
	"\014\000write error",					/* 0C 00 */
	"\014\001write error - recovered with auto reallocation",	/* 0C 01 */
	"\014\002write error - auto reallocation failed",	/* 0C 02 */
	"\014\003write error - recommend reassignment",		/* 0C 03 */
	"\014\004compression check miscompare error",		/* 0C 04 */
	"\014\005data expansion occurred during compression",	/* 0C 05 */
	"\014\006block not compressible",			/* 0C 06 */
	"\014\007write error - recovery needed",		/* 0C 07 */
	"\014\010write error - recovery failed",		/* 0C 08 */
	"\014\011write error - loss of streaming",		/* 0C 09 */
	"\014\012write error - padding blocks added",		/* 0C 0A */
	"\014\013auxiliary memory code 4 (99-148) [proposed]",	/* 0C 0B */
	"\015\000error detected by third party temporary initiator",	/* 0D 00 */
	"\015\001third party device failure",			/* 0D 01 */
	"\015\002copy target device not reachable",		/* 0D 02 */
	"\015\003incorrect copy target device type",		/* 0D 03 */
	"\015\004copy target device data underrun",		/* 0D 04 */
	"\015\005copy target device data overrun",		/* 0D 05 */
#ifdef	__used__
	"\016\000",						/* 0E 00 */
	"\017\000",						/* 0F 00 */
#endif
	"\020\000id crc or ecc error",				/* 10 00 */
	"\021\000unrecovered read error",			/* 11 00 */
	"\021\001read retries exhausted",			/* 11 01 */
	"\021\002error too long to correct",			/* 11 02 */
	"\021\003multiple read errors",				/* 11 03 */
	"\021\004unrecovered read error - auto reallocate failed",	/* 11 04 */
	"\021\005l-ec uncorrectable error",			/* 11 05 */
	"\021\006circ unrecovered error",			/* 11 06 */
	"\021\007data re-synchronization error",		/* 11 07 */
	"\021\010incomplete block read",			/* 11 08 */
	"\021\011no gap found",					/* 11 09 */
	"\021\012miscorrected error",				/* 11 0A */
	"\021\013unrecovered read error - recommend reassignment",	/* 11 0B */
	"\021\014unrecovered read error - recommend rewrite the data",	/* 11 0C */
	"\021\015de-compression crc error",			/* 11 0D */
	"\021\016cannot decompress using declared algorithm",	/* 11 0E */
	"\021\017error reading upc/ean number",			/* 11 0F */
	"\021\020error reading isrc number",			/* 11 10 */
	"\021\021read error - loss of streaming",		/* 11 11 */
	"\021\022auxiliary memory code 3 (99-148) [proposed]",	/* 11 12 */
	"\022\000address mark not found for id field",		/* 12 00 */
	"\023\000address mark not found for data field",	/* 13 00 */
	"\024\000recorded entity not found",			/* 14 00 */
	"\024\001record not found",				/* 14 01 */
	"\024\002filemark or setmark not found",		/* 14 02 */
	"\024\003end-of-data not found",			/* 14 03 */
	"\024\004block sequence error",				/* 14 04 */
	"\024\005record not found - recommend reassignment",	/* 14 05 */
	"\024\006record not found - data auto-reallocated",	/* 14 06 */
	"\025\000random positioning error",			/* 15 00 */
	"\025\001mechanical positioning error",			/* 15 01 */
	"\025\002positioning error detected by read of medium",	/* 15 02 */
	"\026\000data synchronization mark error",		/* 16 00 */
	"\026\001data sync error - data rewritten",		/* 16 01 */
	"\026\002data sync error - recommend rewrite",		/* 16 02 */
	"\026\003data sync error - data auto-reallocated",	/* 16 03 */
	"\026\004data sync error - recommend reassignment",	/* 16 04 */
	"\027\000recovered data with no error correction applied",	/* 17 00 */
	"\027\001recovered data with retries",			/* 17 01 */
	"\027\002recovered data with positive head offset",	/* 17 02 */
	"\027\003recovered data with negative head offset",	/* 17 03 */
	"\027\004recovered data with retries and/or circ applied",	/* 17 04 */
	"\027\005recovered data using previous sector id",	/* 17 05 */
	"\027\006recovered data without ecc - data auto-reallocated",	/* 17 06 */
	"\027\007recovered data without ecc - recommend reassignment",	/* 17 07 */
	"\027\010recovered data without ecc - recommend rewrite",	/* 17 08 */
	"\027\011recovered data without ecc - data rewritten",	/* 17 09 */
	"\030\000recovered data with error correction applied",	/* 18 00 */
	"\030\001recovered data with error corr. & retries applied",	/* 18 01 */
	"\030\002recovered data - data auto-reallocated",	/* 18 02 */
	"\030\003recovered data with circ",			/* 18 03 */
	"\030\004recovered data with l-ec",			/* 18 04 */
	"\030\005recovered data - recommend reassignment",	/* 18 05 */
	"\030\006recovered data - recommend rewrite",		/* 18 06 */
	"\030\007recovered data with ecc - data rewritten",	/* 18 07 */
	"\030\010recovered data with linking",			/* 18 08 */
	"\031\000defect list error",				/* 19 00 */
	"\031\001defect list not available",			/* 19 01 */
	"\031\002defect list error in primary list",		/* 19 02 */
	"\031\003defect list error in grown list",		/* 19 03 */
	"\032\000parameter list length error",			/* 1A 00 */
	"\033\000synchronous data transfer error",		/* 1B 00 */
	"\034\000defect list not found",			/* 1C 00 */
	"\034\001primary defect list not found",		/* 1C 01 */
	"\034\002grown defect list not found",			/* 1C 02 */
	"\035\000miscompare during verify operation",		/* 1D 00 */
	"\036\000recovered id with ecc correction",		/* 1E 00 */
	"\037\000partial defect list transfer",			/* 1F 00 */
	"\040\000invalid command operation code",		/* 20 00 */
	"\040\001access controls code 1 (99-314) [proposed]",	/* 20 01 */
	"\040\002access controls code 2 (99-314) [proposed]",	/* 20 02 */
	"\040\003access controls code 3 (99-314) [proposed]",	/* 20 03 */
	"\040\004read type operation while in write capable state",	/* 20 04 */
	"\040\005write type operation while in read capable state",	/* 20 05 */
	"\040\006illegal command while in explicit address model",	/* 20 06 */
	"\040\007illegal command while in implicit address model",	/* 20 07 */
	"\040\010access controls code 5 (99-245) [proposed]",	/* 20 08 */
	"\040\011access controls code 6 (99-245) [proposed]",	/* 20 09 */
	"\040\012access controls code 7 (99-245) [proposed]",	/* 20 0A */
	"\040\013access controls code 8 (99-245) [proposed]",	/* 20 0B */
	"\041\000logical block address out of range",		/* 21 00 */
	"\041\001invalid element address",			/* 21 01 */
	"\041\002invalid address for write",			/* 21 02 */
	"\042\000illegal function (use 20 00, 24 00, or 26 00)", /* 22 00 */
#ifdef	__used__
	"\043\000",						/* 23 00 */
#endif
	"\044\000invalid field in cdb",				/* 24 00 */
	"\044\001cdb decryption error",				/* 24 01 */
	"\044\002invalid cdb field while in explicit block address model",	/* 24 02 */
	"\044\003invalid cdb field while in implicit block address model",	/* 24 03 */
	"\045\000logical unit not supported",			/* 25 00 */
	"\046\000invalid field in parameter list",		/* 26 00 */
	"\046\001parameter not supported",			/* 26 01 */
	"\046\002parameter value invalid",			/* 26 02 */
	"\046\003threshold parameters not supported",		/* 26 03 */
	"\046\004invalid release of persistent reservation",	/* 26 04 */
	"\046\005data decryption error",			/* 26 05 */
	"\046\006too many target descriptors",			/* 26 06 */
	"\046\007unsupported target descriptor type code",	/* 26 07 */
	"\046\010too many segment descriptors",			/* 26 08 */
	"\046\011unsupported segment descriptor type code",	/* 26 09 */
	"\046\012unexpected inexact segment",			/* 26 0A */
	"\046\013inline data length exceeded",			/* 26 0B */
	"\046\014invalid operation for copy source or destination",	/* 26 0C */
	"\046\015copy segment granularity violation",		/* 26 0D */
	"\047\000write protected",				/* 27 00 */
	"\047\001hardware write protected",			/* 27 01 */
	"\047\002logical unit software write protected",	/* 27 02 */
	"\047\003associated write protect",			/* 27 03 */
	"\047\004persistent write protect",			/* 27 04 */
	"\047\005permanent write protect",			/* 27 05 */
	"\047\006conditional write protect",			/* 27 06 */
	"\050\000not ready to ready change, medium may have changed",	/* 28 00 */
	"\050\001import or export element accessed",		/* 28 01 */
	"\051\000power on, reset, or bus device reset occurred", /* 29 00 */
	"\051\001power on occurred",				/* 29 01 */
	"\051\002scsi bus reset occurred",			/* 29 02 */
	"\051\003bus device reset function occurred",		/* 29 03 */
	"\051\004device internal reset",			/* 29 04 */
	"\051\005transceiver mode changed to single-ended",	/* 29 05 */
	"\051\006transceiver mode changed to lvd",		/* 29 06 */
	"\052\000parameters changed",				/* 2A 00 */
	"\052\001mode parameters changed",			/* 2A 01 */
	"\052\002log parameters changed",			/* 2A 02 */
	"\052\003reservations preempted",			/* 2A 03 */
	"\052\004reservations released",			/* 2A 04 */
	"\052\005registrations preempted",			/* 2A 05 */
	"\052\006asymmetric access code 6 (00-232) [proposed]",	/* 2A 06 */
	"\052\007asymmetric access code 7 (00-232) [proposed]",	/* 2A 07 */
	"\053\000copy cannot execute since host cannot disconnect",	/* 2B 00 */
	"\054\000command sequence error",			/* 2C 00 */
	"\054\001too many windows specified",			/* 2C 01 */
	"\054\002invalid combination of windows specified",	/* 2C 02 */
	"\054\003current program area is not empty",		/* 2C 03 */
	"\054\004current program area is empty",		/* 2C 04 */
	"\054\005illegal power condition request",		/* 2C 05 */
	"\054\006persistent prevent conflict",			/* 2C 06 */
	"\055\000overwrite error on update in place",		/* 2D 00 */
	"\056\000insufficient time for operation",		/* 2E 00 */
	"\057\000commands cleared by another initiator",	/* 2F 00 */
	"\060\000incompatible medium installed",		/* 30 00 */
	"\060\001cannot read medium - unknown format",		/* 30 01 */
	"\060\002cannot read medium - incompatible format",	/* 30 02 */
	"\060\003cleaning cartridge installed",			/* 30 03 */
	"\060\004cannot write medium - unknown format",		/* 30 04 */
	"\060\005cannot write medium - incompatible format",	/* 30 05 */
	"\060\006cannot format medium - incompatible medium",	/* 30 06 */
	"\060\007cleaning failure",				/* 30 07 */
	"\060\010cannot write - application code mismatch",	/* 30 08 */
	"\060\011current session not fixated for append",	/* 30 09 */
	"\060\020medium not formatted",				/* 30 10 */
	"\061\000medium format corrupted",			/* 31 00 */
	"\061\001format command failed",			/* 31 01 */
	"\061\002zoned formatting failed due to spare linking",	/* 31 02 */
	"\062\000no defect spare location available",		/* 32 00 */
	"\062\001defect list update failure",			/* 32 01 */
	"\063\000tape length error",				/* 33 00 */
	"\064\000enclosure failure",				/* 34 00 */
	"\065\000enclosure services failure",			/* 35 00 */
	"\065\001unsupported enclosure function",		/* 35 01 */
	"\065\002enclosure services unavailable",		/* 35 02 */
	"\065\003enclosure services transfer failure",		/* 35 03 */
	"\065\004enclosure services transfer refused",		/* 35 04 */
	"\066\000ribbon, ink, or toner failure",		/* 36 00 */
	"\067\000rounded parameter",				/* 37 00 */
	"\070\000event status notification",			/* 38 00 */
	"\070\002esn - power management class event",		/* 38 02 */
	"\070\004esn - media class event",			/* 38 04 */
	"\070\006esn - device busy class event",		/* 38 06 */
	"\071\000saving parameters not supported",		/* 39 00 */
	"\072\000medium not present",				/* 3A 00 */
	"\072\001medium not present - tray closed",		/* 3A 01 */
	"\072\002medium not present - tray open",		/* 3A 02 */
	"\072\003medium not present - loadable",		/* 3A 03 */
	"\072\004medium not present - medium auxiliary memory accessible",	/* 3A 04 */
	"\073\000sequential positioning error",			/* 3B 00 */
	"\073\001tape position error at beginning-of-medium",	/* 3B 01 */
	"\073\002tape position error at end-of-medium",		/* 3B 02 */
	"\073\003tape or electronic vertical forms unit not ready",	/* 3B 03 */
	"\073\004slew failure",					/* 3B 04 */
	"\073\005paper jam",					/* 3B 05 */
	"\073\006failed to sense top-of-form",			/* 3B 06 */
	"\073\007failed to sense bottom-of-form",		/* 3B 07 */
	"\073\010reposition error",				/* 3B 08 */
	"\073\011read past end of medium",			/* 3B 09 */
	"\073\012read past beginning of medium",		/* 3B 0A */
	"\073\013position past end of medium",			/* 3B 0B */
	"\073\014position past beginning of medium",		/* 3B 0C */
	"\073\015medium destination element full",		/* 3B 0D */
	"\073\016medium source element empty",			/* 3B 0E */
	"\073\017end of medium reached",			/* 3B 0F */
	"\073\021medium magazine not accessible",		/* 3B 11 */
	"\073\022medium magazine removed",			/* 3B 12 */
	"\073\023medium magazine inserted",			/* 3B 13 */
	"\073\024medium magazine locked",			/* 3B 14 */
	"\073\025medium magazine unlocked",			/* 3B 15 */
	"\073\026mechanical positioning or changer error",	/* 3B 16 */
#ifdef	__used__
	"\074\000",						/* 3C 00 */
#endif
	"\075\000invalid bits in identify message",		/* 3D 00 */
	"\076\000logical unit has not self-configured yet",	/* 3E 00 */
	"\076\001logical unit failure",				/* 3E 01 */
	"\076\002timeout on logical unit",			/* 3E 02 */
	"\076\003logical unit failed self-test",		/* 3E 03 */
	"\076\004logical unit unable to update self-test log",	/* 3E 04 */
	"\077\000target operating conditions have changed",	/* 3F 00 */
	"\077\001microcode has been changed",			/* 3F 01 */
	"\077\002changed operating definition",			/* 3F 02 */
	"\077\003inquiry data has changed",			/* 3F 03 */
	"\077\004component device attached",			/* 3F 04 */
	"\077\005device identifier changed",			/* 3F 05 */
	"\077\006redundancy group created or modified",		/* 3F 06 */
	"\077\007redundancy group deleted",			/* 3F 07 */
	"\077\010spare created or modified",			/* 3F 08 */
	"\077\011spare deleted",				/* 3F 09 */
	"\077\012volume set created or modified",		/* 3F 0A */
	"\077\013volume set deleted",				/* 3F 0B */
	"\077\014volume set deassigned",			/* 3F 0C */
	"\077\015volume set reassigned",			/* 3F 0D */
	"\077\016reported luns data has changed",		/* 3F 0E */
	"\077\017echo buffer overwritten",			/* 3F 0F */
	"\077\020medium loadable",				/* 3F 10 */
	"\077\021medium auxiliary memory accessible",		/* 3F 11 */
	"\100\000ram failure (should use 40 nn)",		/* 40 00 */
#ifdef	XXX
	"\100\000nn diagnostic failure on component nn (80h-ffh)",	/* 40 00 */
#endif
	"\100\000diagnostic failure on component nn (80h-ffh)",	/* 40 00 */
	"\101\000data path failure (should use 40 nn)",		/* 41 00 */
	"\102\000power-on or self-test failure (should use 40 nn)",	/* 42 00 */
	"\103\000message error",				/* 43 00 */
	"\104\000internal target failure",			/* 44 00 */
	"\105\000select or reselect failure",			/* 45 00 */
	"\106\000unsuccessful soft reset",			/* 46 00 */
	"\107\000scsi parity error",				/* 47 00 */
	"\107\001data phase crc error detected",		/* 47 01 */
	"\107\002scsi parity error detected during st data phase",	/* 47 02 */
	"\107\003information unit crc error detected",		/* 47 03 */
	"\107\004asynchronous information protection error detected",	/* 47 04 */
	"\110\000initiator detected error message received",	/* 48 00 */
	"\111\000invalid message error",			/* 49 00 */
	"\112\000command phase error",				/* 4A 00 */
	"\113\000data phase error",				/* 4B 00 */
	"\114\000logical unit failed self-configuration",	/* 4C 00 */
#ifdef	XXX
	"\115\000nn tagged overlapped commands (nn = queue tag)",	/* 4D 00 */
#endif
	"\115\000tagged overlapped commands (nn = queue tag)",	/* 4D 00 */
	"\116\000overlapped commands attempted",		/* 4E 00 */
#ifdef	__used__
	"\117\000",						/* 4F 00 */
#endif
	"\120\000write append error",				/* 50 00 */
	"\120\001write append position error",			/* 50 01 */
	"\120\002position error related to timing",		/* 50 02 */
	"\121\000erase failure",				/* 51 00 */
	"\121\001erase failure - incomplete erase operation detected",	/* 51 01 */
	"\122\000cartridge fault",				/* 52 00 */
	"\123\000media load or eject failed",			/* 53 00 */
	"\123\001unload tape failure",				/* 53 01 */
	"\123\002medium removal prevented",			/* 53 02 */
	"\124\000scsi to host system interface failure",	/* 54 00 */
	"\125\000system resource failure",			/* 55 00 */
	"\125\001system buffer full",				/* 55 01 */
	"\125\002insufficient reservation resources",		/* 55 02 */
	"\125\003insufficient resources",			/* 55 03 */
	"\125\004insufficient registration resources",		/* 55 04 */
	"\125\005access controls code 4 (99-314) [proposed]",	/* 55 05 */
	"\125\006auxiliary memory code 1 (99-148) [proposed]",	/* 55 06 */
#ifdef	__used__
	"\126\000",						/* 56 00 */
#endif
	"\127\000unable to recover table-of-contents",		/* 57 00 */
	"\130\000generation does not exist",			/* 58 00 */
	"\131\000updated block read",				/* 59 00 */
	"\132\000operator request or state change input",	/* 5A 00 */
	"\132\001operator medium removal request",		/* 5A 01 */
	"\132\002operator selected write protect",		/* 5A 02 */
	"\132\003operator selected write permit",		/* 5A 03 */
	"\133\000log exception",				/* 5B 00 */
	"\133\001threshold condition met",			/* 5B 01 */
	"\133\002log counter at maximum",			/* 5B 02 */
	"\133\003log list codes exhausted",			/* 5B 03 */
	"\134\000rpl status change",				/* 5C 00 */
	"\134\001spindles synchronized",			/* 5C 01 */
	"\134\002spindles not synchronized",			/* 5C 02 */
	"\135\000failure prediction threshold exceeded",	/* 5D 00 */
	"\135\001media failure prediction threshold exceeded",	/* 5D 01 */
	"\135\002logical unit failure prediction threshold exceeded",	/* 5D 02 */
	"\135\003spare area exhaustion prediction threshold exceeded",	/* 5D 03 */
	"\135\020hardware impending failure general hard drive failure", /* 5D 10 */
	"\135\021hardware impending failure drive error rate too high",	/* 5D 11 */
	"\135\022hardware impending failure data error rate too high",	/* 5D 12 */
	"\135\023hardware impending failure seek error rate too high",	/* 5D 13 */
	"\135\024hardware impending failure too many block reassigns",	/* 5D 14 */
	"\135\025hardware impending failure access times too high",	/* 5D 15 */
	"\135\026hardware impending failure start unit times too high",	/* 5D 16 */
	"\135\027hardware impending failure channel parametrics",	/* 5D 17 */
	"\135\030hardware impending failure controller detected",	/* 5D 18 */
	"\135\031hardware impending failure throughput performance",	/* 5D 19 */
	"\135\032hardware impending failure seek time performance",	/* 5D 1A */
	"\135\033hardware impending failure spin-up retry count",	/* 5D 1B */
	"\135\034hardware impending failure drive calibration retry count",	/* 5D 1C */
	"\135\040controller impending failure general hard drive failure",	/* 5D 20 */
	"\135\041controller impending failure drive error rate too high",	/* 5D 21 */
	"\135\042controller impending failure data error rate too high", /* 5D 22 */
	"\135\043controller impending failure seek error rate too high", /* 5D 23 */
	"\135\044controller impending failure too many block reassigns", /* 5D 24 */
	"\135\045controller impending failure access times too high",	/* 5D 25 */
	"\135\046controller impending failure start unit times too high",	/* 5D 26 */
	"\135\047controller impending failure channel parametrics",	/* 5D 27 */
	"\135\050controller impending failure controller detected",	/* 5D 28 */
	"\135\051controller impending failure throughput performance",	/* 5D 29 */
	"\135\052controller impending failure seek time performance",	/* 5D 2A */
	"\135\053controller impending failure spin-up retry count",	/* 5D 2B */
	"\135\054controller impending failure drive calibration retry count",	/* 5D 2C */
	"\135\060data channel impending failure general hard drive failure",	/* 5D 30 */
	"\135\061data channel impending failure drive error rate too high",	/* 5D 31 */
	"\135\062data channel impending failure data error rate too high",	/* 5D 32 */
	"\135\063data channel impending failure seek error rate too high",	/* 5D 33 */
	"\135\064data channel impending failure too many block reassigns",	/* 5D 34 */
	"\135\065data channel impending failure access times too high",	/* 5D 35 */
	"\135\066data channel impending failure start unit times too high",	/* 5D 36 */
	"\135\067data channel impending failure channel parametrics",	/* 5D 37 */
	"\135\070data channel impending failure controller detected",	/* 5D 38 */
	"\135\071data channel impending failure throughput performance", /* 5D 39 */
	"\135\072data channel impending failure seek time performance",	/* 5D 3A */
	"\135\073data channel impending failure spin-up retry count",	/* 5D 3B */
	"\135\074data channel impending failure drive calibration retry count",	/* 5D 3C */
	"\135\100servo impending failure general hard drive failure",	/* 5D 40 */
	"\135\101servo impending failure drive error rate too high",	/* 5D 41 */
	"\135\102servo impending failure data error rate too high",	/* 5D 42 */
	"\135\103servo impending failure seek error rate too high",	/* 5D 43 */
	"\135\104servo impending failure too many block reassigns",	/* 5D 44 */
	"\135\105servo impending failure access times too high",	/* 5D 45 */
	"\135\106servo impending failure start unit times too high",	/* 5D 46 */
	"\135\107servo impending failure channel parametrics",		/* 5D 47 */
	"\135\110servo impending failure controller detected",		/* 5D 48 */
	"\135\111servo impending failure throughput performance",	/* 5D 49 */
	"\135\112servo impending failure seek time performance",	/* 5D 4A */
	"\135\113servo impending failure spin-up retry count",		/* 5D 4B */
	"\135\114servo impending failure drive calibration retry count", /* 5D 4C */
	"\135\120spindle impending failure general hard drive failure",	/* 5D 50 */
	"\135\121spindle impending failure drive error rate too high",	/* 5D 51 */
	"\135\122spindle impending failure data error rate too high",	/* 5D 52 */
	"\135\123spindle impending failure seek error rate too high",	/* 5D 53 */
	"\135\124spindle impending failure too many block reassigns",	/* 5D 54 */
	"\135\125spindle impending failure access times too high",	/* 5D 55 */
	"\135\126spindle impending failure start unit times too high",	/* 5D 56 */
	"\135\127spindle impending failure channel parametrics",	/* 5D 57 */
	"\135\130spindle impending failure controller detected",	/* 5D 58 */
	"\135\131spindle impending failure throughput performance",	/* 5D 59 */
	"\135\132spindle impending failure seek time performance",	/* 5D 5A */
	"\135\133spindle impending failure spin-up retry count",	/* 5D 5B */
	"\135\134spindle impending failure drive calibration retry count",	/* 5D 5C */
	"\135\140firmware impending failure general hard drive failure", /* 5D 60 */
	"\135\141firmware impending failure drive error rate too high",	/* 5D 61 */
	"\135\142firmware impending failure data error rate too high",	/* 5D 62 */
	"\135\143firmware impending failure seek error rate too high",	/* 5D 63 */
	"\135\144firmware impending failure too many block reassigns",	/* 5D 64 */
	"\135\145firmware impending failure access times too high",	/* 5D 65 */
	"\135\146firmware impending failure start unit times too high",	/* 5D 66 */
	"\135\147firmware impending failure channel parametrics",	/* 5D 67 */
	"\135\150firmware impending failure controller detected",	/* 5D 68 */
	"\135\151firmware impending failure throughput performance",	/* 5D 69 */
	"\135\152firmware impending failure seek time performance",	/* 5D 6A */
	"\135\153firmware impending failure spin-up retry count",	/* 5D 6B */
	"\135\154firmware impending failure drive calibration retry count",	/* 5D 6C */
	"\135\377failure prediction threshold exceeded (false)", /* 5D FF */
	"\136\000low power condition on",			/* 5E 00 */
	"\136\001idle condition activated by timer",		/* 5E 01 */
	"\136\002standby condition activated by timer",		/* 5E 02 */
	"\136\003idle condition activated by command",		/* 5E 03 */
	"\136\004standby condition activated by command",	/* 5E 04 */
	"\136\101power state change to active",			/* 5E 41 */
	"\136\102power state change to idle",			/* 5E 42 */
	"\136\103power state change to standby",		/* 5E 43 */
	"\136\105power state change to sleep",			/* 5E 45 */
	"\136\107power state change to device control",		/* 5E 47 */
#ifdef	__used__
	"\137\000",						/* 5F 00 */
#endif
	"\140\000lamp failure",					/* 60 00 */
	"\141\000video acquisition error",			/* 61 00 */
	"\141\001unable to acquire video",			/* 61 01 */
	"\141\002out of focus",					/* 61 02 */
	"\142\000scan head positioning error",			/* 62 00 */
	"\143\000end of user area encountered on this track",	/* 63 00 */
	"\143\001packet does not fit in available space",	/* 63 01 */
	"\144\000illegal mode for this track",			/* 64 00 */
	"\144\001invalid packet size",				/* 64 01 */
	"\145\000voltage fault",				/* 65 00 */
	"\146\000automatic document feeder cover up",		/* 66 00 */
	"\146\001automatic document feeder lift up",		/* 66 01 */
	"\146\002document jam in automatic document feeder",	/* 66 02 */
	"\146\003document miss feed automatic in document feeder",	/* 66 03 */
	"\147\000configuration failure",			/* 67 00 */
	"\147\001configuration of incapable logical units failed",	/* 67 01 */
	"\147\002add logical unit failed",			/* 67 02 */
	"\147\003modification of logical unit failed",		/* 67 03 */
	"\147\004exchange of logical unit failed",		/* 67 04 */
	"\147\005remove of logical unit failed",		/* 67 05 */
	"\147\006attachment of logical unit failed",		/* 67 06 */
	"\147\007creation of logical unit failed",		/* 67 07 */
	"\147\010assign failure occurred",			/* 67 08 */
	"\147\011multiply assigned logical unit",		/* 67 09 */
	"\147\012asymmetric access code 4 (00-232) [proposed]",	/* 67 0A */
	"\147\013asymmetric access code 5 (00-232) [proposed]",	/* 67 0B */
	"\150\000logical unit not configured",			/* 68 00 */
	"\151\000data loss on logical unit",			/* 69 00 */
	"\151\001multiple logical unit failures",		/* 69 01 */
	"\151\002parity/data mismatch",				/* 69 02 */
	"\152\000informational, refer to log",			/* 6A 00 */
	"\153\000state change has occurred",			/* 6B 00 */
	"\153\001redundancy level got better",			/* 6B 01 */
	"\153\002redundancy level got worse",			/* 6B 02 */
	"\154\000rebuild failure occurred",			/* 6C 00 */
	"\155\000recalculate failure occurred",			/* 6D 00 */
	"\156\000command to logical unit failed",		/* 6E 00 */
	"\157\000copy protection key exchange failure - authentication failure", /* 6F 00 */
	"\157\001copy protection key exchange failure - key not present",	/* 6F 01 */
	"\157\002copy protection key exchange failure - key not established",	/* 6F 02 */
	"\157\003read of scrambled sector without authentication",	/* 6F 03 */
	"\157\004media region code is mismatched to logical unit region",	/* 6F 04 */
	"\157\005drive region must be permanent/region reset count error",	/* 6F 05 */
#ifdef	XXX
	"\160\000nn decompression exception short algorithm id of nn",	/* 70 00 */
#endif
	"\160\000decompression exception short algorithm id of nn",	/* 70 00 */
	"\161\000decompression exception long algorithm id",	/* 71 00 */
	"\162\000session fixation error",			/* 72 00 */
	"\162\001session fixation error writing lead-in",	/* 72 01 */
	"\162\002session fixation error writing lead-out",	/* 72 02 */
	"\162\003session fixation error - incomplete track in session",	/* 72 03 */
	"\162\004empty or partially written reserved track",	/* 72 04 */
	"\162\005no more track reservations allowed",		/* 72 05 */
	"\163\000cd control error",				/* 73 00 */
	"\163\001power calibration area almost full",		/* 73 01 */
	"\163\002power calibration area is full",		/* 73 02 */
	"\163\003power calibration area error",			/* 73 03 */
	"\163\004program memory area update failure",		/* 73 04 */
	"\163\005program memory area is full",			/* 73 05 */
	"\163\006rma/pma is almost full",			/* 73 06 */
#ifdef	__used__
	"\164\000",						/* 74 00 */
	"\165\000",						/* 75 00 */
	"\166\000",						/* 76 00 */
	"\167\000",						/* 77 00 */
	"\170\000",						/* 78 00 */
	"\171\000",						/* 79 00 */
	"\172\000",						/* 7A 00 */
	"\173\000",						/* 7B 00 */
	"\174\000",						/* 7C 00 */
	"\175\000",						/* 7D 00 */
	"\176\000",						/* 7E 00 */
	"\177\000",						/* 7F 00 */
#endif
#ifdef	XXX
	"\200\000start vendor unique",				/* 80 00 */
#endif
	NULL
};

#ifdef SMO_C501
static const char *sd_smo_c501_error_str[] = {
	"\012\000disk not inserted",			/* 0x0a */
	"\013\000load/unload failure",			/* 0x0b */
	"\014\000spindle failure",			/* 0x0c */
	"\015\000focus failure",			/* 0x0d */
	"\016\000tracking failure",			/* 0x0e */
	"\017\000bias magnet failure",			/* 0x0f */
	"\043\000illegal function for medium type",	/* 0x23 */
/*XXX*/	"\070\000recoverable write error",		/* 0x38 */
/*XXX*/	"\071\000write error recovery failed",		/* 0x39 */
	"\072\000defect list update failed",		/* 0x3a */
	"\075\000defect list not available",		/* 0x3d */
	"\200\000limited laser life",			/* 0x80 */
	"\201\000laser focus coil over-current",	/* 0x81 */
	"\202\000laser tracking coil over-current",	/* 0x82 */
	"\203\000temperature alarm",			/* 0x83 */
	NULL
};
#endif

static char *sd_sense_keys[] = {
	"No Additional Sense",		/* 0x00 */
	"Recovered Error",		/* 0x01 */
	"Not Ready",			/* 0x02 */
	"Medium Error",			/* 0x03 */
	"Hardware Error",		/* 0x04 */
	"Illegal Request",		/* 0x05 */
	"Unit Attention",		/* 0x06 */
	"Data Protect",			/* 0x07 */
	"Blank Check",			/* 0x08 */
	"Vendor Unique",		/* 0x09 */
	"Copy Aborted",			/* 0x0a */
	"Aborted Command",		/* 0x0b */
	"Equal",			/* 0x0c */
	"Volume Overflow",		/* 0x0d */
	"Miscompare",			/* 0x0e */
	"Reserved"			/* 0x0f */
};

#ifdef	MAP_SCSI_CMDS
static char *sd_cmds[] = {
	"\000test unit ready",		/* 0x00 */
	"\001rezero",			/* 0x01 */
	"\003request sense",		/* 0x03 */
	"\004format",			/* 0x04 */
	"\007reassign",			/* 0x07 */
	"\010read",			/* 0x08 */
	"\012write",			/* 0x0a */
	"\013seek",			/* 0x0b */
	"\022inquiry",			/* 0x12 */
	"\025mode select",		/* 0x15 */
	"\026reserve",			/* 0x16 */
	"\027release",			/* 0x17 */
	"\030copy",			/* 0x18 */
	"\032mode sense",		/* 0x1a */
	"\033start/stop",		/* 0x1b */
	"\036door lock",		/* 0x1e */
	"\067read defect data",		/* 0x37 */
	NULL
};
#endif

EXPORT
const char *
scg_sensemsg(ctype, code, qual, vec, sbuf, maxcnt)
	register 	int	ctype;
	register 	int	code;
	register 	int	qual;
	register const char	**vec;
			char	*sbuf;
			int	maxcnt;
{
	register int i;

	/*
	 * Ignore controller type if error vec is supplied.
	 */
	if (vec == (const char **)NULL) switch (ctype) {
	case DEV_MD21:
		vec = sd_ccs_error_str;
		break;

	case DEV_ACB40X0:
	case DEV_ACB4000:
	case DEV_ACB4010:
	case DEV_ACB4070:
	case DEV_ACB5500:
		vec = sd_adaptec_error_str;
		break;

#ifdef	SMO_C501
	case DEV_SONY_SMO:
		vec = sd_smo_c501_error_str;
		break;
#endif

	default:
		vec = sd_ccs_error_str;
	}
	if (vec == (const char **)NULL)
		return ("");

	for (i = 0; i < 2; i++) {
		while (*vec != (char *) NULL) {
			if (code == (Uchar)(*vec)[0] &&
					qual == (Uchar)(*vec)[1]) {
				return (&(*vec)[2]);
			} else {
				vec++;		/* Next entry */
			}
		}
		if (*vec == (char *) NULL)	/* End of List: switch over */
			vec = sd_ccs_error_str;
	}
	if (code == 0x40) {
		js_snprintf(sbuf, maxcnt,
			"diagnostic failure on component 0x%X", qual);
		return (sbuf);
	}
	if (code == 0x4D) {
		js_snprintf(sbuf, maxcnt,
			"tagged overlapped commands, queue tag is 0x%X",
									qual);
		return (sbuf);
	}
	if (code == 0x70) {
		js_snprintf(sbuf, maxcnt,
			"decompression exception short algorithm id of 0x%X",
									qual);
		return (sbuf);
	}
	if (qual != 0)
		return ((char *)NULL);

	if (code < 0x80) {
		js_snprintf(sbuf, maxcnt, "invalid sense code 0x%X", code);
		return (sbuf);
	}
	js_snprintf(sbuf, maxcnt, "vendor unique sense code 0x%X", code);
	return (sbuf);
}

#undef	sense	/*XXX JS Hack, solange scgio.h noch nicht fertig ist */
EXPORT int
scg__errmsg(scgp, obuf, maxcnt, sense, status, sense_code)
	SCSI	*scgp;
	char	*obuf;
	int	maxcnt;
	register struct scsi_sense	*sense;
	register struct scsi_status	*status;
	int				sense_code;
{
	char	sbuf[80];
	const char *sensemsg, *cmdname, *sensekey;
#define	ext_sense	((struct scsi_ext_sense *) sense)
	register int blkno = 0;
	register int code;
	int	badqual = 0;
	int	qual = 0;
	int	fru = 0;
	int	key = 0;
	int	segment = 0;
	int	blkvalid = 0;
	int	fm = 0;
	int	eom = 0;
	int	ili = 0;
	int	sksv = 0;
	int	n;
	int	sizeleft = maxcnt;

	sensekey = sensemsg = "[]";
	if (sense->code >= 0x70) {
		if (sense_code >= 0)
			code = sense_code;
		else
			code = ext_sense->sense_code;
		segment = ext_sense->seg_num;
		qual = ext_sense->qual_code;
		fru = ext_sense->fru_code;
		sksv = ext_sense->sksv;
	} else {
		code = sense->code;
	}
	if (status->chk == 0) {
		sensemsg = "no sense";
	} else {
		if (sense->code >= 0x70) {
			key = ext_sense->key;
			if (key < 0x10)
				sensekey = sd_sense_keys[ext_sense->key];
			else
				sensekey = "invalid sensekey";
			blkno = (ext_sense->info_1 << 24) |
				(ext_sense->info_2 << 16) |
				(ext_sense->info_3 << 8) |
				ext_sense->info_4;
			fm = ext_sense->fil_mk;
			eom = ext_sense->eom;
			ili = ext_sense->ili;
		} else {
			key = -1;
			sensekey = "[]";
			blkno = (sense->high_addr << 16) |
				(sense->mid_addr << 8) |
				sense->low_addr;
			fm = eom = 0;
		}
		blkvalid = sense->adr_val;

		sensemsg = scg_sensemsg(scgp->dev, code, qual, scgp->nonstderrs, sbuf, sizeof (sbuf));
		if (sensemsg == NULL) {
			sensemsg = scg_sensemsg(scgp->dev, code, 0, scgp->nonstderrs, sbuf, sizeof (sbuf));
			badqual = 1;
		}
	}
#ifdef	nonono
	if (un->un_cmd < sizeof (scsi_cmds)) {
		cmdname = scsi_cmds[un->un_cmd];
	} else {
		cmdname = "unknown cmd";
	}
#endif
	cmdname = "";
	n = js_snprintf(obuf, sizeleft, "%sSense Key: 0x%X %s%s, Segment %d\n",
		cmdname, key, sensekey,
		(sense->code == 0x71)?", deferred error":"",
		segment);
	if (n <= 0) {
		obuf[0] = '\0';
		return (maxcnt - sizeleft);
	}
	obuf += n;
	sizeleft -= n;
	n = js_snprintf(obuf, sizeleft, "Sense Code: 0x%02X Qual 0x%02X %s%s%s%s Fru 0x%X\n",
		code, qual, *sensemsg?"(":"", sensemsg, *sensemsg?")":"",
		badqual? " [No matching qualifier]":"",
		fru);
	if (n <= 0) {
		obuf[0] = '\0';
		return (maxcnt - sizeleft);
	}
	obuf += n;
	sizeleft -= n;
	n = js_snprintf(obuf, sizeleft, "Sense flags: Blk %d %s%s%s%s",
		blkno, blkvalid?"(valid) ":"(not valid) ",
		fm?"file mark detected ":"",
		eom?"end of medium ":"",
		ili?"illegal block length ":"");
	if (n <= 0) {
		obuf[0] = '\0';
		return (maxcnt - sizeleft);
	}
	obuf += n;
	sizeleft -= n;
	if (!sksv) {
		n = js_snprintf(obuf, sizeleft, "\n");
		if (n <= 0) {
			obuf[0] = '\0';
			return (maxcnt - sizeleft);
		}
		obuf += n;
		sizeleft -= n;
		return (maxcnt - sizeleft);
	}
	switch (key) {

	case SC_ILLEGAL_REQUEST:
		n = js_snprintf(obuf, sizeleft, "error refers to %s part, bit ptr %d %s field ptr %d",
			ext_sense->cd? "command":"data",
			(int)ext_sense->bptr,
			ext_sense->bpv? "(valid)":"(not valid)",
			ext_sense->field_ptr[0] << 8 |
			ext_sense->field_ptr[1]);
		if (n <= 0) {
			obuf[0] = '\0';
			return (maxcnt - sizeleft);
		}
		obuf += n;
		sizeleft -= n;
		break;

	case SC_RECOVERABLE_ERROR:
	case SC_HARDWARE_ERROR:
	case SC_MEDIUM_ERROR:
		n = js_snprintf(obuf, sizeleft, "actual retry count %d",
			ext_sense->field_ptr[0] << 8 |
			ext_sense->field_ptr[1]);
		if (n <= 0) {
			obuf[0] = '\0';
			return (maxcnt - sizeleft);
		}
		obuf += n;
		sizeleft -= n;
		break;

	case SC_NOT_READY:
		n = js_snprintf(obuf, sizeleft, "operation %d%% done",
			(100*(ext_sense->field_ptr[0] << 8 |
				ext_sense->field_ptr[1]))/(unsigned)65536);
		if (n < 0) {
			obuf[0] = '\0';
			return (maxcnt - sizeleft);
		}
		obuf += n;
		sizeleft -= n;
		break;
	}
	n = js_snprintf(obuf, sizeleft, "\n");
	if (n <= 0) {
		obuf[0] = '\0';
		return (maxcnt - sizeleft);
	}
	obuf += n;
	sizeleft -= n;
	return (maxcnt - sizeleft);
}

#ifdef	DEBUG
print_err(code, ctype)
{
	register int i;
	register char **vec  = (char **)NULL;

	switch (ctype) {
	case CTYPE_MD21:
	case CTYPE_CCS:
		vec = sd_ccs_error_str;
		break;

	case CTYPE_ACB4000:
		vec = sd_adaptec_error_str;
		break;

#ifdef	SMO_C501
	case CTYPE_SMO_C501:
		vec = sd_smo_c501_error_str;
		break;
#endif

#ifdef	CDD_521
	case DEV_CDD_521:
		vec = sd_cdd_521_error_str;
		break;
#endif
	}
	js_printf("error code: 0x%x", code);
	if (vec == (char **)NULL)
		return;

	for (i = 0; i < 2; i++) {
		while (*vec != (char *) NULL) {
			if (code == (Uchar)*vec[0]) {
				js_printf(" (%s)", (char *)((int)(*vec)+1));
				return;
			} else
				vec++;
		}
		if (*vec == (char *) NULL)
			vec = sd_ccs_error_str;
	}
}


main()
{
	int	i;

#ifdef ACB
	for (i = 0; i < 0x30; i++) {
/*		js_printf("Code: 0x%x	Key: 0x%x	", i, sd_adaptec_keys[i]);*/
		js_printf("Key: 0x%x %-16s ", sd_adaptec_keys[i],
					sd_sense_keys[sd_adaptec_keys[i]]);
		js_print_err(i, CTYPE_ACB4000);
		js_printf("\n");
	}
#else
/*	for (i = 0; i < 0x84; i++) {*/
	for (i = 0; i < 0xd8; i++) {
/*		print_err(i, CTYPE_SMO_C501);*/
		print_err(i, DEV_CDD_521);
		js_printf("\n");
	}
#endif
}
#endif
