/* @(#)cd_extra.c	1.20 16/02/14 Copyright 2000-2001 Heiko Eissfeldt, Copyright 2004-2010 J. Schilling */
#ifndef lint
static	const char _sccsid[] =
"@(#)cd_extra.c	1.20 16/02/14 Copyright 2000-2001 Heiko Eissfeldt, Copyright 2004-2010 J. Schilling";
#endif

/*
 * This is an include file!
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

/**************** CD-Extra special treatment *********************************/

#include <schily/ctype.h>

static unsigned long Read_CD_Extra_File __PR((unsigned char *Extra_buf,
						unsigned long sector));

static unsigned long
Read_CD_Extra_File(Extra_buf, sector)
	unsigned char	*Extra_buf;
	unsigned long	sector;
{
	unsigned long	mysec;

	/* read PVD */
	ReadCdRomData(get_scsi_p(), Extra_buf, sector+16, 1);

	/* check ISO signature */
	if (memcmp(Extra_buf, "\001CD001", 6) != 0)
		return (0);

	/* get path_table */
	mysec = Extra_buf[148] << 24;
	mysec |= Extra_buf[149] << 16;
	mysec |= Extra_buf[150] << 8;
	mysec |= Extra_buf[151];

	if (mysec <= sector)
		return (0);

	/* read path table */
	ReadCdRomData(get_scsi_p(), Extra_buf, mysec, 1);

	/* find cdplus subdirectory */
	{ unsigned char * p = Extra_buf;
		while (p+8 < Extra_buf + CD_FRAMESIZE_RAW) {
			int namelength;

			namelength = p[0] | (p[1] << 8);
			if (namelength == 6 &&
			    !memcmp(p+8, "CDPLUS", 6)) break;

			p += 8 + namelength + (namelength & 1);
		}
		if (p+8 >= Extra_buf + CD_FRAMESIZE_RAW)
			return (0);

		/* get extent */
		mysec = p[2] << 24;
		mysec |= p[3] << 16;
		mysec |= p[4] << 8;
		mysec |= p[5];
	}

	if (mysec <= sector)
		return (0);

	ReadCdRomData(get_scsi_p(), Extra_buf, mysec, 1);

	/* find file info.cdp */
	{ unsigned char * p = Extra_buf;
		while (p+33 < Extra_buf + CD_FRAMESIZE_RAW) {
			int namelength;

			namelength = p[32];
			if (namelength < 1)	/* Not a valid filename	*/
				return (0);
			if (namelength == 10 &&
			    !memcmp(p+33, "INFO.CDP;1", 10)) break;

			p += p[0];
			if (p[0] == 0)		/* Avoid endless loop	*/
				return (0);	/* with bad dir content	*/
		}
		if (p+33 >= Extra_buf + CD_FRAMESIZE_RAW)
			return (0);

		/* get extent */
		mysec = p[6] << 24;
		mysec |= p[7] << 16;
		mysec |= p[8] << 8;
		mysec |= p[9];
	}

	if (mysec <= sector)
		return (0);

	/* read file info.cdp */
	ReadCdRomData(get_scsi_p(), Extra_buf, mysec, 1);

	return (mysec - sector);
}

static unsigned char Extra_buffer[CD_FRAMESIZE_RAW];

/*
 * Read the file cdplus/info.cdp from the cd extra disc.
 * This file has to reside at exactly 75 sectors after start of
 * the last session (according to Blue Book).
 * Of course, there are a lot dubious cd extras, which don't care :-(((
 * As an alternative method, we try reading through the iso9660 file system...
 */
static int Read_CD_Extra_Info __PR((unsigned long sector));
static int Read_CD_Extra_Info(sector)
	unsigned long sector;
{
	unsigned	i;
static int		offsets[] = {
		75		/* this is what blue book says */
	};

	for (i = 0; i < sizeof (offsets)/sizeof (int); i++) {
#ifdef DEBUG_XTRA
		fprintf(stderr,
			"debug: Read_CD_Extra_Info at sector %lu\n",
			sector+offsets[i]);
#endif
		ReadCdRomData(get_scsi_p(), Extra_buffer,
							sector+offsets[i], 1);

		/*
		 * If we are unlucky the drive cannot handle XA sectors by
		 * default.
		 * We try to compensate by ignoring the first eight bytes.
		 * Of course then we lack the last 8 bytes of the sector...
		 */
		if (Extra_buffer[0] == 0)
			memmove(Extra_buffer, Extra_buffer +8,
							CD_FRAMESIZE - 8);

		/*
		 * check for cd extra
		 */
		if (Extra_buffer[0] == 'C' && Extra_buffer[1] == 'D')
			return (sector+offsets[i]);

	/*
	 * CD is not conforming to BlueBook!
	 * Read the file through ISO9660 file system.
	 */
	{
		unsigned long	offset = Read_CD_Extra_File(Extra_buffer,
								sector);

		if (offset == 0)
			return (0);

		if (Extra_buffer[0] == 0)
			memmove(Extra_buffer, Extra_buffer +8,
							CD_FRAMESIZE - 8);

		/*
		 * check for cd extra
		 */
		if (Extra_buffer[0] == 'C' && Extra_buffer[1] == 'D')
			return (sector+offset);
		}
	}

	return (0);
}

static void Read_Subinfo __PR((unsigned pos, unsigned length));
static void
Read_Subinfo(pos, length)
	unsigned	pos;
	unsigned	length;
{
	unsigned	num_infos, num;
	unsigned char	*Subp, *orgSubp;
	unsigned	this_track = 0xff;
#ifdef DEBUG_XTRA
	unsigned char	*up;
	unsigned char	*sp;
	unsigned	u;
	unsigned short	s;
#endif

	length += 8;
	length = (length + CD_FRAMESIZE_RAW-1) / CD_FRAMESIZE_RAW;
	length *= CD_FRAMESIZE_RAW;
	orgSubp = Subp = malloc(length);

	if (Subp == NULL) {
		errmsg(_("Read_Subinfo no memory(%d).\n"), length);
		goto errorout;
	}

	ReadCdRomData(get_scsi_p(), Subp, pos, 1);

	num_infos = Subp[45]+(Subp[44] << 8);
#ifdef DEBUG_XTRA
	fprintf(stderr, "subinfo version %c%c.%c%c, %u info packets\n",
			Subp[8],
			Subp[9],
			Subp[10],
			Subp[11],
			num_infos);
#endif
	length -= 46;
	Subp += 46;

	for (num = 0; num < num_infos && length > 0; num++) {
		unsigned	id = *Subp;
		unsigned	len = *(Subp +1);
#define	INFOPACKETTYPES	0x44
#ifdef	INFOPACKETSTRINGS
		static const char *infopacketID[INFOPACKETTYPES] = { "0",
			"track identifier",
			"album title",
			"universal product code",
			"international standard book number",
			"copyright",
			"track title",
			"notes",
			"main interpret",
			"secondary interpret",
			"composer",
			"original composer",
			"creation date",
			"release  date",
			"publisher",
			"0f",
			"isrc audio track",
			"isrc lyrics",
			"isrc pictures",
			"isrc MIDI data",
			"14", "15", "16", "17", "18", "19",
			"copyright state SUB_INFO",
			"copyright state intro lyrics",
			"copyright state lyrics",
			"copyright state MIDI data",
			"1e", "1f",
			"intro lyrics",
			"pointer to lyrics text file and length",
			"22", "23", "24", "25", "26", "27", "28",
			"29", "2a", "2b", "2c", "2d", "2e", "2f",
			"still picture descriptor",
			"31",
			"32", "33", "34", "35", "36", "37", "38",
			"39", "3a", "3b", "3c", "3d", "3e", "3f",
			"MIDI file descriptor",
			"genre code",
			"tempo",
			"key"
		};
#endif

		if (id >= INFOPACKETTYPES) {
			fprintf(stderr,
			"Off=%4d, ind=%2u/%2u, unknown Id=%2u, len=%2u ",
				/*
				 * this pointer difference is assumed to be
				 * small enough for an int.
				 */
				(int)(Subp - orgSubp),
				num, num_infos, id, len);
			Subp += 2 + 1;
			length -= 2 + 1;
			break;
		}
#ifdef DEBUG_XTRA
		fprintf(stderr, "info packet %u\n", id);
#endif

		switch (id) {

		case 1:    /* track nummer or 0 */
			this_track = 10 * (*(Subp + 2) - '0') +
					(*(Subp + 3) - '0');
			break;

		case 0x02: /* album title */
			if (global.disctitle == NULL) {
				global.disctitle = malloc(len + 1);
				if (global.disctitle != NULL) {
					memcpy(global.disctitle, Subp + 2,
									len);
					global.disctitle[len] = '\0';
				}
			}
			break;

		case 0x03: /* media catalog number */
			if (Get_MCN()[0] == '\0' && Subp[2] != '\0' &&
			    len >= 13) {
				Set_MCN(Subp + 2);
			}
			break;

		case 0x06: /* track title */
			if (this_track > 0 && this_track < 100 &&
			    global.tracktitle[this_track] == NULL) {
				global.tracktitle[this_track] = malloc(len+1);
				if (global.tracktitle[this_track] != NULL) {
					memcpy(global.tracktitle[this_track],
								Subp + 2, len);
					global.tracktitle[this_track][len] = '\0';
				}
			}
			break;

		case 0x05: /* copyright message */
			if (global.copyright_message == NULL) {
				global.copyright_message = malloc(len + 1);
				if (global.copyright_message != NULL) {
					memcpy(global.copyright_message,
								Subp + 2, len);
					global.copyright_message[len] = '\0';
				}
			}
			break;

		case 0x08: /* creator -> CD-Text performer */
			if (global.performer == NULL) {
				global.performer = malloc(len + 1);
				if (global.performer != NULL) {
					memcpy(global.performer, Subp + 2, len);
					global.performer[len] = '\0';
				}
			}
			break;

		case 0x10: /* isrc */
			if (this_track > 0 && this_track < 100 &&
			    Get_ISRC(this_track)[0] == '\0' &&
			    Subp[2] != '\0' &&
			    len >= 15) {
				Set_ISRC(this_track, Subp + 2);
			}
			break;

#if 0
		case 0x04:
		case 0x07:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
#ifdef	INFOPACKETSTRINGS
			fprintf(outfp, "%s: %*.*s\n",
				infopacketID[id],
				(int) len, (int) len, (Subp +2));
#endif
			break;

#ifdef DEBUG_XTRA
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
#ifdef	INFOPACKETSTRINGS
			fprintf(outfp, _("%s %scopyrighted\n"),
				infopacketID[id], *(Subp + 2) == 0 ?
								_("not ") : "");
#endif
			break;

	case 0x21:
			fprintf(outfp, _("lyrics file beginning at sector %u"),
				(unsigned) GET_BE_UINT_FROM_CHARP(Subp + 2));
			if (len == 8)
				fprintf(outfp, _(", having length: %u\n"),
				(unsigned) GET_BE_UINT_FROM_CHARP(Subp + 6));
			else
				fputs("\n", outfp);
			break;

		case 0x30:
			sp = Subp + 2;
			while (sp < Subp + 2 + len) {
			/*while (len >= 10) {*/
				s = be16_to_cpu((*(sp)) | (*(sp) << 8));
				fprintf(outfp, "%04x, ", s);
				sp += 2;
				up = sp;
				switch (s) {
				case 0:
					break;
				case 4:
					break;
				case 5:
					break;
				case 6:
					break;
				}
				u = GET_BE_UINT_FROM_CHARP(up);
				fprintf(outfp, "%04lx, ", (long) u);
				up += 4;
				u = GET_BE_UINT_FROM_CHARP(up);
				fprintf(outfp, "%04lx, ", (long) u);
				up += 4;
				sp += 8;
			}
			fputs("\n", outfp);
			break;

		case 0x40:
			fprintf(outfp, _("MIDI file beginning at sector %u"),
				(unsigned) GET_BE_UINT_FROM_CHARP(Subp + 2));
			if (len == 8)
				fprintf(outfp, _(", having length: %u\n"),
				(unsigned) GET_BE_UINT_FROM_CHARP(Subp + 6));
			else
				fputs("\n", outfp);
			break;

#ifdef	INFOPACKETSTRINGS
		case 0x42:
			fprintf(outfp, _("%s: %d beats per minute\n"),
					infopacketID[id], *(Subp + 2));
			break;

		case 0x41:
			if (len == 8) {
				fprintf(outfp,
					"%s: %x, %x, %x, %x, %x, %x, %x, %x\n",
					infopacketID[id],
					*(Subp + 2),
					*(Subp + 3),
					*(Subp + 4),
					*(Subp + 5),
					*(Subp + 6),
					*(Subp + 7),
					*(Subp + 8),
					*(Subp + 9));
			} else {
				fprintf(outfp, "%s:\n", infopacketID[id]);
			}
			break;

		case 0x43:
			fprintf(outfp,
				"%s: %x\n", infopacketID[id], *(Subp + 2));
			break;

		default:
			fprintf(outfp,
				"%s: %*.*s\n", infopacketID[id],
				(int)len, (int)len, (Subp +2));
#endif
#endif
#endif
		}

		if (len & 1)
			len++;
		Subp += 2 + len;
		length -= 2 + len;
	}

	/*
	 * cleanup
	 */
	free(orgSubp);

errorout:
	;
}
