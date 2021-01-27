/* @(#)cd_text.c	1.11 10/12/19 Copyright 2000-2001 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling */

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
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

/* *************** CD-Text special treatment ******************************* */

typedef struct {
	unsigned char headerfield[4];
	unsigned char textdatafield[12];
	unsigned char crcfield[2];
} cdtextpackdata;

static unsigned short crctab[1<<8] = { /* as calculated by initcrctab() */
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0,
};

#define	SUBSIZE	18*8

static unsigned short	updcrc	__PR((unsigned int p_crc,
					unsigned char   *cp,
					size_t  cnt));

static unsigned short
updcrc(p_crc, cp, cnt)
	unsigned int		p_crc;
	register unsigned char	*cp;
	register size_t		cnt;
{
	register unsigned short	crc = (unsigned short)p_crc;

	while (cnt--) {
		crc = (crc<<8) ^ crctab[(crc>>(16-8)) ^ (*cp++)];
	}
	return (crc);
}

static unsigned short	calcCRC	__PR((unsigned char *buf, unsigned bsize));

static unsigned short
calcCRC(buf, bsize)
	unsigned char	*buf;
	unsigned	bsize;
{
	return (updcrc(0x0, (unsigned char *)buf, bsize));
}

static unsigned char    fliptab[8] = {
				0x01,
				0x02,
				0x04,
				0x08,
				0x10,
				0x20,
				0x40,
				0x80,
};

static int	flip_error_corr	__PR((unsigned char *b, int crc));

static int
flip_error_corr(b, crc)
	unsigned char	*b;
	int		crc;
{
	if (crc != 0) {
		int	i;

		for (i = 0; i < SUBSIZE; i++) {
			char	c;

			c = fliptab[i%8];
			b[i / 8] ^= c;
			if ((crc = calcCRC(b, SUBSIZE/8)) == 0) {
				return (crc);
			}
			b[i / 8] ^= c;
		}
	}
	return (crc & 0xffff);
}


static int	cdtext_crc_ok	__PR((cdtextpackdata *c));

static int
cdtext_crc_ok(c)
	cdtextpackdata	*c;
{
	int	crc;
	int	retval;

	c->crcfield[0] ^= 0xff;
	c->crcfield[1] ^= 0xff;
	crc = calcCRC(((unsigned char *)c), 18);
	retval = (0 == flip_error_corr((unsigned char *)c, crc));
	c->crcfield[0] ^= 0xff;
	c->crcfield[1] ^= 0xff;
#if	0
	fprintf(stderr, "%02x %02x %02x %02x  ",
		c->headerfield[0], c->headerfield[1],
		c->headerfield[2], c->headerfield[3]);
	fprintf(stderr,
		"%c %c %c %c %c %c %c %c %c %c %c %c  ",
		c->textdatafield[0],
		c->textdatafield[1],
		c->textdatafield[2],
		c->textdatafield[3],
		c->textdatafield[4],
		c->textdatafield[5],
		c->textdatafield[6],
		c->textdatafield[7],
		c->textdatafield[8],
		c->textdatafield[9],
		c->textdatafield[10],
		c->textdatafield[11]);
	fprintf(stderr, "%02x %02x \n",
		c->crcfield[0],
		c->crcfield[1]);
#endif
	return (retval);
}

#define	DETAILED	0

#if	DETAILED
static void	dump_binary	__PR((cdtextpackdata *c));

static void
dump_binary(c)
	cdtextpackdata	*c;
{
	fprintf(stderr, _(": header fields %02x %02x %02x  "),
		c->headerfield[1], c->headerfield[2], c->headerfield[3]);
	fprintf(stderr,
		"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		c->textdatafield[0],
		c->textdatafield[1],
		c->textdatafield[2],
		c->textdatafield[3],
		c->textdatafield[4],
		c->textdatafield[5],
		c->textdatafield[6],
		c->textdatafield[7],
		c->textdatafield[8],
		c->textdatafield[9],
		c->textdatafield[10],
		c->textdatafield[11]);
}
#endif

static int	process_header	__PR((cdtextpackdata *c, unsigned tracknr,
					int dbcc, unsigned char *line));

static int
process_header(c, tracknr, dbcc, line)
	cdtextpackdata	*c;
	unsigned	tracknr;
	int		dbcc;
	unsigned char	*line;
{
	switch ((int)c->headerfield[0]) {

	case 0x80:	/* Title of album or track */
#if	DETAILED
		fprintf(stderr, _("Title"));
#endif
		if (tracknr > 0 && tracknr < 100 &&
		    global.tracktitle[tracknr] == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.tracktitle[tracknr] = malloc(len + 1);
			if (global.tracktitle[tracknr] != NULL) {
				memcpy(global.tracktitle[tracknr], line, len);
				global.tracktitle[tracknr][len] = '\0';
			}
		} else if (tracknr == 0 &&
			    global.disctitle == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.disctitle = malloc(len + 1);
			if (global.disctitle != NULL) {
				memcpy(global.disctitle, line, len);
				global.disctitle[len] = '\0';
			}
		}
		break;

	case 0x81:	/* Name(s) of the performer(s) */
#if	DETAILED
		fprintf(stderr, _("Performer(s)"));
#endif
		if (tracknr > 0 && tracknr < 100 &&
		    global.trackperformer[tracknr] == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.trackperformer[tracknr] = malloc(len + 1);

			if (global.trackperformer[tracknr] != NULL) {
				memcpy(global.trackperformer[tracknr], line,
									len);
				global.trackperformer[tracknr][len] = '\0';
			}
		} else if (tracknr == 0 &&
			    global.performer == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.performer = malloc(len + 1);
			if (global.performer != NULL) {
				memcpy(global.performer, line, len);
				global.performer[len] = '\0';
			}
		}
		break;

	case 0x82:	/* Name(s) of the songwriter(s) */
#if	DETAILED
		fprintf(stderr, _("Songwriter(s)"));
#endif
		if (tracknr > 0 && tracknr < 100 &&
		    global.tracksongwriter[tracknr] == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.tracksongwriter[tracknr] = malloc(len + 1);

			if (global.tracksongwriter[tracknr] != NULL) {
				memcpy(global.tracksongwriter[tracknr], line,
									len);
				global.tracksongwriter[tracknr][len] = '\0';
			}
		} else if (tracknr == 0 &&
			    global.songwriter == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.songwriter = malloc(len + 1);
			if (global.songwriter != NULL) {
				memcpy(global.songwriter, line, len);
				global.songwriter[len] = '\0';
			}
		}
		break;

	case 0x83:	/* Name(s) of the composer(s) */
#if	DETAILED
		fprintf(stderr, _("Composer(s)"));
#endif
		if (tracknr > 0 && tracknr < 100 &&
		    global.trackcomposer[tracknr] == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.trackcomposer[tracknr] = malloc(len + 1);

			if (global.trackcomposer[tracknr] != NULL) {
				memcpy(global.trackcomposer[tracknr], line,
									len);
				global.trackcomposer[tracknr][len] = '\0';
			}
		} else if (tracknr == 0 &&
			    global.composer == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.composer = malloc(len + 1);
			if (global.composer != NULL) {
				memcpy(global.composer, line, len);
				global.composer[len] = '\0';
			}
		}
		break;

	case 0x84:	/* Name(s) of the arranger(s) */
#if	DETAILED
		fprintf(stderr, _("Arranger(s)"));
#endif
		if (tracknr > 0 && tracknr < 100 &&
		    global.trackarranger[tracknr] == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.trackarranger[tracknr] = malloc(len + 1);

			if (global.trackarranger[tracknr] != NULL) {
				memcpy(global.trackarranger[tracknr], line,
									len);
				global.trackarranger[tracknr][len] = '\0';
			}
		} else if (tracknr == 0 &&
			    global.arranger == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.arranger = malloc(len + 1);
			if (global.arranger != NULL) {
				memcpy(global.arranger, line, len);
				global.arranger[len] = '\0';
			}
		}
		break;

	case 0x85:	/* Message from content provider and/or artist */
#if	DETAILED
		fprintf(stderr, _("Message"));
#endif
		if (tracknr > 0 && tracknr < 100 &&
		    global.trackmessage[tracknr] == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.trackmessage[tracknr] = malloc(len + 1);

			if (global.trackmessage[tracknr] != NULL) {
				memcpy(global.trackmessage[tracknr], line,
									len);
				global.trackmessage[tracknr][len] = '\0';
			}
		} else if (tracknr == 0 &&
			    global.message == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.message = malloc(len + 1);
			if (global.message != NULL) {
				memcpy(global.message, line, len);
				global.message[len] = '\0';
			}
		}
		break;

	case 0x86:	/* Disc Identification and information */
#if	DETAILED
		fprintf(stderr, _("Disc identification"));
#endif
		if (tracknr == 0 && line[0] != '\0') {
			fprintf(stderr, _("Disc identification: %s\n"), line);
		}
		break;

	case 0x87:	/* Genre Identification and information */
#if	DETAILED
		fprintf(stderr, _("Genre identification"));
#endif
		break;

	case 0x8d:	/* Closed information */
#if	DETAILED
		fprintf(stderr, _("Closed information"));
#endif
		if (tracknr > 0 && tracknr < 100 &&
		    global.trackclosed_info[tracknr] == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.trackclosed_info[tracknr] = malloc(len + 1);

			if (global.trackclosed_info[tracknr] != NULL) {
				memcpy(global.trackclosed_info[tracknr], line,
									len);
				global.trackclosed_info[tracknr][len] = '\0';
			}
		} else if (tracknr == 0 &&
			    global.closed_info == NULL) {
			unsigned	len;

			len = strlen((char *)line);

			if (len > 0)
				global.closed_info = malloc(len + 1);
			if (global.closed_info != NULL) {
				memcpy(global.closed_info, line, len);
				global.closed_info[len] = '\0';
			}
		}
		break;

	case 0x8e:	/* UPC/EAN code or ISRC code */
#if	DETAILED
		fprintf(stderr, _("UPC or ISRC"));
#endif
		if (tracknr > 0 && tracknr < 100) {
			Set_ISRC(tracknr, line);
		} else if (tracknr == 0 && line[0] != '\0') {
			Set_MCN(line);
		}
		break;

	case 0x88:	/* Table of Content information */
#if	DETAILED
		fprintf(stderr, _("Table of Content identification"));
		dump_binary(c);
#endif
		return (0);

	case 0x89:	/* Second Table of Content information */
#if	DETAILED
		fprintf(stderr, _("Second Table of Content identification"));
		dump_binary(c);
#endif
		return (0);

	case 0x8f:	/* Size information of the block */
#if	DETAILED == 0
		break;
#else
		switch (tracknr) {

		case 0:
			fprintf(outfp,
				_("first track is %d, last track is %d\n"),
				c->textdatafield[1],
				c->textdatafield[2]);
			if (c->textdatafield[3] & 0x80) {
				fprintf(outfp,
				_("Program Area CD Text information available\n"));
				if (c->textdatafield[3] & 0x40) {
					fprintf(outfp, _("Program Area copy protection available\n"));
				}
			}
			if (c->textdatafield[3] & 0x07) {
				fprintf(outfp,
				_("message information is %scopyrighted\n"),
					c->textdatafield[3] & 0x04 ?
							"": _("not "));
				fprintf(outfp,
				_("Names of performer/songwriter/composer/arranger(s) are %scopyrighted\n"),
					c->textdatafield[3] & 0x02 ?
							"": _("not "));
				fprintf(outfp,
				_("album and track names are %scopyrighted\n"),
					c->textdatafield[3] & 0x01 ?
							"": _("not "));
			}
			fprintf(outfp,
				_("%d packs with album/track names\n"),
				c->textdatafield[4]);
			fprintf(outfp,
				_("%d packs with performer names\n"),
				c->textdatafield[5]);
			fprintf(outfp,
				_("%d packs with songwriter names\n"),
				c->textdatafield[6]);
			fprintf(outfp,
				_("%d packs with composer names\n"),
				c->textdatafield[7]);
			fprintf(outfp,
				_("%d packs with arranger names\n"),
				c->textdatafield[8]);
			fprintf(outfp,
				_("%d packs with artist or content provider messages\n"),
				c->textdatafield[9]);
			fprintf(outfp,
				_("%d packs with disc identification information\n"),
				c->textdatafield[10]);
			fprintf(outfp,
			_("%d packs with genre identification/information\n"),
				c->textdatafield[11]);
			break;

		case 1:
			fprintf(outfp,
			_("%d packs with table of contents information\n"),
				c->textdatafield[0]);
			fprintf(outfp,
			_("%d packs with second table of contents information\n"),
				c->textdatafield[1]);
			fprintf(outfp,
				_("%d packs with reserved information\n"),
				c->textdatafield[2]);
			fprintf(outfp,
				_("%d packs with reserved information\n"),
				c->textdatafield[3]);
			fprintf(outfp,
				_("%d packs with reserved information\n"),
				c->textdatafield[4]);
			fprintf(outfp,
				_("%d packs with closed information\n"),
				c->textdatafield[5]);
			fprintf(outfp,
				_("%d packs with UPC/EAN ISRC information\n"),
				c->textdatafield[6]);
			fprintf(outfp,
				_("%d packs with size information\n"),
				c->textdatafield[7]);
			fprintf(outfp,
			_("last sequence numbers for blocks 1-8: %d %d %d %d "),
				c->textdatafield[8],
				c->textdatafield[9],
				c->textdatafield[10],
				c->textdatafield[11]);
			break;

		case 2:
			fprintf(outfp, "%d %d %d %d\n",
				c->textdatafield[0],
				c->textdatafield[1],
				c->textdatafield[2],
				c->textdatafield[3]);
			fprintf(outfp,
			_("Language codes for blocks 1-8: %d %d %d %d %d %d %d %d\n"),
				c->textdatafield[4],
				c->textdatafield[5],
				c->textdatafield[6],
				c->textdatafield[7],
				c->textdatafield[8],
				c->textdatafield[9],
				c->textdatafield[10],
				c->textdatafield[11]);
			break;
		}

		fprintf(stderr, _("Blocksize"));
		dump_binary(c);
		return (0);

#if !defined DEBUG_CDTEXT
	default:
#else
	}
#endif

		fprintf(stderr, _(": header fields %02x %02x %02x  "),
			c->headerfield[1], c->headerfield[2],
			c->headerfield[3]);
#endif /* DETAILED */

#if !defined DEBUG_CDTEXT
	}
#if	DETAILED
	if (tracknr == 0) {
		fprintf(stderr, _(" for album   : ->"));
	} else {
		fprintf(stderr, _(" for track %2u: ->"), tracknr);
	}
	fputs((char *)line, stderr);
	fputs("<-", stderr);
#endif

	if (dbcc != 0) {
#else
	{
#endif
		/* EMPTY */
#if	DETAILED
		fprintf(stderr,
		"  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
			c->textdatafield[0],
			c->textdatafield[1],
			c->textdatafield[2],
			c->textdatafield[3],
			c->textdatafield[4],
			c->textdatafield[5],
			c->textdatafield[6],
			c->textdatafield[7],
			c->textdatafield[8],
			c->textdatafield[9],
			c->textdatafield[10],
			c->textdatafield[11]);
#endif
	}
	return (0);
}
