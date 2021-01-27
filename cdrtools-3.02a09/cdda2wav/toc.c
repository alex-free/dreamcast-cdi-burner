/* @(#)toc.c	1.104 17/07/19 Copyright 1998-2003,2017 Heiko Eissfeldt, Copyright 2004-2017 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)toc.c	1.104 17/07/19 Copyright 1998-2003,2017 Heiko Eissfeldt, Copyright 2004-2017 J. Schilling";
#endif
/*
 * CDDA2WAV (C) Heiko Eissfeldt heiko@hexco.de
 * Copyright (c) 2004-2017 J. Schilling, Heiko Eissfeldt
 *
 * The CDDB routines are compatible to cddbd (C) Ti Kan and Steve Scherf
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

#include "config.h"
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/intcvt.h>
#include <schily/unistd.h>		/* sleep */
#include <schily/ctype.h>
#include <schily/errno.h>
#include <schily/fcntl.h>
#include <schily/varargs.h>
#include <schily/nlsdefs.h>
#include <schily/hostname.h>
#include <schily/ioctl.h>
#include <schily/sha1.h>

#include <scg/scsitransp.h>

/* tcp stuff */
/* fix OS/2 compilation */
#ifdef	__EMX__
#define	gethostid	nogethostid
#endif

#include <schily/socket.h>
#undef gethostid
#include <schily/in.h>
#include <schily/netdb.h>

#include "mytype.h"
#include "byteorder.h"
#include "interface.h"
#include "cdda2wav.h"
#include "global.h"
#include "base64.h"
#include "toc.h"
#include "exitcodes.h"
#include "ringbuff.h"
#include "version.h"

#include <schily/schily.h>

#define	CD_TEXT
#define	CD_EXTRA
#undef	DEBUG_XTRA
#undef	DEBUG_CDTEXT
#undef	DEBUG_CDDBP


int have_CD_text;
int have_multisession;
int have_CD_extra;
int have_CDDB;

struct iterator;

int Get_Mins __PR((unsigned long p_track));
int Get_Secs __PR((unsigned long p_track));
int Get_Frames __PR((unsigned long p_track));
int Get_Flags __PR((unsigned long p_track));
int Get_SCMS __PR((unsigned long p_track));

LOCAL void UpdateTrackData	__PR((int p_num));
LOCAL void UpdateIndexData	__PR((int p_num));
LOCAL void UpdateTimeData	__PR((int p_min, int p_sec, int p_frm));
LOCAL unsigned int is_multisession	__PR((void));
LOCAL unsigned int get_end_of_last_audio_track	__PR((unsigned mult_off));
LOCAL void check_hidden		__PR((void));
LOCAL int cddb_sum		__PR((int n));
LOCAL void dump_extra_info	__PR((unsigned from));
LOCAL int GetIndexOfSector	__PR((unsigned sec, unsigned track));
LOCAL int patch_cd_extra	__PR((unsigned track, unsigned long sector));
LOCAL void patch_to_audio	__PR((unsigned long p_track));
LOCAL int restrict_tracks_illleadout __PR((void));

LOCAL void Set_MCN		__PR((unsigned char *MCN_arg));
LOCAL void Set_ISRC		__PR((unsigned track,
					const unsigned char *ISRC_arg));

LOCAL unsigned char	g_track = 0xff;	/* current track */
LOCAL unsigned char	g_index = 0xff;	/* current index */


LOCAL void InitIterator __PR((struct iterator *iter, unsigned long p_track));


/*
 * Conversion function: from logical block addresses  to minute,second,frame
 */
int
lba_2_msf(lba, m, s, f)
	long	lba;
	int	*m;
	int	*s;
	int	*f;
{
#ifdef  __follow_redbook__
	if (lba >= -150 && lba < 405000) {	/* lba <= 404849 */
#else
	if (lba >= -150) {
#endif
		lba += 150;
	} else if (lba >= -45150 && lba <= -151) {
		lba += 450150;
	} else
		return (1);

	*m = lba / 60 / 75;
	lba -= (*m)*60*75;
	*s = lba / 75;
	lba -= (*s)*75;
	*f = lba;

	return (0);
}

/*
 * print the track currently read
 */
LOCAL void
UpdateTrackData(p_num)
	int	p_num;
{
	if (global.quiet == 0) {
		fprintf(outfp, _("\ntrack: %.2d, "), p_num); fflush(outfp);
	}
	g_track = (unsigned char) p_num;
}


/*
 * print the index currently read
 */
LOCAL void
UpdateIndexData(p_num)
	int	p_num;
{
	if (global.quiet == 0) {
		fprintf(outfp, _("index: %.2d\n"), p_num); fflush(outfp);
	}
	g_index = (unsigned char) p_num;
}


/*
 * print the time of track currently read
 */
LOCAL void
UpdateTimeData(p_min, p_sec, p_frm)
	int	p_min;
	int	p_sec;
	int	p_frm;
{
	if (global.quiet == 0) {
		fprintf(outfp, _("time: %.2d:%.2d.%.2d\r"), p_min, p_sec, p_frm);
		fflush(outfp);
	}
}

void
AnalyzeQchannel(frame)
	unsigned	frame;
{
	if (trackindex_disp != 0) {
		subq_chnl	*sub_ch;

		sub_ch = ReadSubQ(get_scsi_p(), GET_POSITIONDATA, 0);

		/*
		 * analyze sub Q-channel data
		 */
		if (sub_ch->track != g_track ||
				sub_ch->index != g_index) {
			UpdateTrackData(sub_ch->track);
			UpdateIndexData(sub_ch->index);
		}
	}
	frame += 150;
	UpdateTimeData((unsigned char) (frame / (60*75)),
			(unsigned char) ((frame % (60*75)) / 75),
			(unsigned char) (frame % 75));
}

unsigned	cdtracks = 0;
LOCAL int	have_hiddenAudioTrack = 0;

int
no_disguised_audiotracks()
{
	/*
	 * we can assume no audio tracks according to toc here.
	 * read a data sector from the first data track
	 */
	unsigned char	p[3000];
	int		retval;

	get_scsi_p()->silent++;
	retval = 1 == ReadCdRomData(get_scsi_p(), p, Get_StartSector(1), 1);
	get_scsi_p()->silent--;
	if (retval == 0) {
		int	i;

		errmsgno(EX_BAD,
		_("Warning: wrong track types found: patching to audio...\n"));
		for (i = 0; i < cdtracks; i++)
			patch_to_audio(i);
	}
	return (retval);
}


#undef SIM_ILLLEADOUT
int
ReadToc()
{
	int	retval = (*doReadToc)(get_scsi_p());

#if	defined SIM_ILLLEADOUT
extern	TOC	g_toc[MAXTRK+1]; /* hidden track + 100 regular tracks */

	g_toc[cdtracks+1] = 20*75;
#endif
	return (retval);
}

LOCAL	int	can_read_illleadout	__PR((void));

LOCAL int
can_read_illleadout()
{
	SCSI	*scgp = get_scsi_p();

	UINT4 buffer [CD_FRAMESIZE_RAW/4];
	if (global.illleadout_cd == 0)
		return (0);

	scgp->silent++;
	global.reads_illleadout =
	    ReadCdRom(scgp, buffer, Get_AudioStartSector(CDROM_LEADOUT), 1);
	scgp->silent--;
	return (global.reads_illleadout);
}


unsigned	find_an_off_sector __PR((unsigned lSector,
						unsigned SectorBurstVal));

unsigned
find_an_off_sector(lSector, SectorBurstVal)
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	long	track_of_start = Get_Track(lSector);
	long	track_of_end = Get_Track(lSector + SectorBurstVal -1);
	long	start = Get_AudioStartSector(track_of_start);
	long	end = Get_EndSector(track_of_end);

	if (lSector - start > end - lSector + SectorBurstVal -1)
		return (start);
	else
		return (end);
}

#ifdef CD_TEXT
#include "scsi_cmds.h"
#endif


int	handle_cdtext	__PR((void));

int
handle_cdtext()
{
#ifdef CD_TEXT
	if (global.buf[0] == 0 && global.buf[1] == 0) {
		have_CD_text = 0;
		return (have_CD_text);
	}

	/*
	 * do a quick scan over all pack type indicators
	 */
	{
		int i;
		int count_fails = 0;
		int len = ((global.buf[0] & 0xFF) << 8) | (global.buf[1] & 0xFF);

		len += 2;
		len = min(len, global.bufsize);
		for (i = 0; i < len-4; i += 18) {
			if ((global.buf[4+i] & 0xFF) < 0x80 ||
			    (global.buf[4+i] & 0xFF) > 0x8f) {
				count_fails++;
			}
		}
		have_CD_text = len > 4 && count_fails < 3;
	}

#else
	have_CD_text = 0;
#endif
	return (have_CD_text);
}


#ifdef CD_TEXT
#include "cd_text.c"
#endif


#if defined CDROMMULTISESSION
LOCAL	int	tmp_fd;
#endif

#ifdef CD_EXTRA
#include "cd_extra.c"
#endif

LOCAL	unsigned	session_start;
/*
 * A Cd-Extra is detected, if it is a multisession CD with
 * only audio tracks in the first session and a data track
 * in the last session.
 */
LOCAL	unsigned
is_multisession()
{
	unsigned	mult_off;
#if defined CDROMMULTISESSION
	/*
	 * FIXME: we would have to do a ioctl (CDROMMULTISESSION)
	 *	    for the cdrom device associated with the generic device
	 *	    not just AUX_DEVICE
	 */
	struct cdrom_multisession	ms_str;

	if (interface == GENERIC_SCSI)
		tmp_fd = open(global.aux_name, O_RDONLY);
	else
		tmp_fd = global.cooked_fd;

	if (tmp_fd != -1) {
		int	result;

		ms_str.addr_format = CDROM_LBA;
		result = ioctl(tmp_fd, CDROMMULTISESSION, &ms_str);
		if (result == -1) {
			if (global.verbose != 0)
				errmsg(_("Multi session ioctl not supported.\n"));
		} else {
#ifdef DEBUG_XTRA
			fprintf(outfp,
				_("current ioctl multisession_offset = %u\n"),
				ms_str.addr.lba);
#endif
			if (interface == GENERIC_SCSI)
				close(tmp_fd);
			if (ms_str.addr.lba > 0)
				return (ms_str.addr.lba);
		}
	}
#endif
	mult_off = 0;
	if (LastAudioTrack() + 1 == FirstDataTrack()) {
		mult_off = Get_StartSector(FirstDataTrack());
	}

#ifdef DEBUG_XTRA
	fprintf(outfp,
		_("current guessed multisession_offset = %u\n"),
		mult_off);
#endif
	return (mult_off);
}

#define	SESSIONSECTORS	(152*75)
/*
 * The solution is to read the Table of Contents of the first
 * session only (if the drive permits that) and directly use
 * the start of the leadout. If this is not supported, we subtract
 * a constant of SESSIONSECTORS sectors (found heuristically).
 */
LOCAL unsigned
get_end_of_last_audio_track(mult_off)
	unsigned	mult_off;
{
	unsigned	retval;

	/*
	 * Try to read the first session table of contents.
	 * This works for Sony and mmc type drives.
	 */
	if (ReadLastAudio && (retval = ReadLastAudio(get_scsi_p())) != 0) {
		return (retval);
	} else {
		return (mult_off - SESSIONSECTORS);
	}
}

LOCAL void	dump_cdtext_info	__PR((void));

#if defined CDDB_SUPPORT
LOCAL void	emit_cddb_form		__PR((char *fname_baseval));
#endif

#if defined CDINDEX_SUPPORT
LOCAL void	emit_cdindex_form	__PR((char *fname_baseval));
#endif


typedef struct TOC {	/* structure of table of contents (cdrom) */
	unsigned char	reserved1;
	unsigned char	bFlags;
	unsigned char	bTrack;
	unsigned char	reserved2;
	unsigned int	dwStartSector;
	int		mins;
	int		secs;
	int	frms;
	unsigned char	ISRC[16];
	int		SCMS;
} TOC;


/*
 * Flags contains two fields:
 *  bits 7-4 (ADR)
 *	: 0 no sub-q-channel information
 *	: 1 sub-q-channel contains current position
 *	: 2 sub-q-channel contains media catalog number
 *	: 3 sub-q-channel contains International Standard
 *				   Recording Code ISRC
 *	: other values reserved
 *  bits 3-0 (Control) :
 *  bit 3 : when set indicates there are 4 audio channels else 2 channels
 *  bit 2 : when set indicates this is a data track else an audio track
 *  bit 1 : when set indicates digital copy is permitted else prohibited
 *  bit 0 : when set indicates pre-emphasis is present else not present
 */

#define	GETFLAGS(x)		((x)->bFlags)
#define	GETTRACK(x)		((x)->bTrack)
#define	GETSTART(x)		((x)->dwStartSector)
#define	GETMINS(x)		((x)->mins)
#define	GETSECS(x) 		((x)->secs)
#define	GETFRAMES(x)		((x)->frms)
#define	GETISRC(x)		((x)->ISRC)

#define	IS__PREEMPHASIZED(p)	((GETFLAGS(p) & 0x10) != 0)
#define	IS__INCREMENTAL(p)	((GETFLAGS(p) & 0x10) != 0)
#define	IS__COPYRESTRICTED(p)	(!(GETFLAGS(p) & 0x20) != 0)
#define	IS__COPYRIGHTED(p)	(!(GETFLAGS(p) & 0x20) != 0)
#define	IS__DATA(p)		((GETFLAGS(p) & 0x40) != 0)
#define	IS__AUDIO(p)		(!(GETFLAGS(p) & 0x40) != 0)
#define	IS__QUADRO(p)		((GETFLAGS(p) & 0x80) != 0)

/*
 * Iterator interface inspired from Java
 */
struct iterator {
	int	index;
	int		startindex;
	void		(*reset)	__PR((struct iterator *this));
	struct TOC	*(*getNextTrack) __PR((struct iterator *this));
	int		(*hasNextTrack)	__PR((struct iterator *this));
};


/*
 * The Table of Contents needs to be corrected if we
 * have a CD-Extra. In this case all audio tracks are
 * followed by a data track (in the second session).
 * Unlike for single session CDs the end of the last audio
 * track cannot be set to the start of the following
 * track, since the lead-out and lead-in would then
 * errenously be part of the audio track. This would
 * lead to read errors when trying to read into the
 * lead-out area.
 * So the length of the last track in case of Cd-Extra
 * has to be fixed.
 */
unsigned
FixupTOC(no_tracks)
	unsigned	no_tracks;
{
	unsigned	mult_off;
	unsigned	offset = 0;
	int		j = -1;
	unsigned	real_end = 2000000;

	/*
	 * get the multisession offset in sectors
	 */
	mult_off = is_multisession();

	/*
	 * if the first track address had been the victim of an underflow,
	 * set it to zero.
	 */
	if (Get_StartSector(1) > Get_StartSector(LastTrack())) {
		errmsgno(EX_BAD,
		_("Warning: first track has negative start sector! Setting to zero.\n"));
		toc_entry(1, Get_Flags(1), Get_Tracknumber(1),
		Get_ISRC(1), 0, 0, 2, 0);
	}

#ifdef DEBUG_XTRA
	fprintf(outfp, "current multisession_offset = %u\n", mult_off);
#endif
	dump_cdtext_info();

	if (mult_off > 100) { /* the offset has to have a minimum size */

		/*
		 * believe the multisession offset :-)
		 * adjust end of last audio track to be in the first session
		 */
		real_end = get_end_of_last_audio_track(mult_off);
#ifdef DEBUG_XTRA
		fprintf(outfp, "current end = %u\n", real_end);
#endif

		j = FirstDataTrack();
		if (LastAudioTrack() + 1 == j) {
			long	sj = Get_StartSector(j);

			if (sj > (long)real_end) {
				session_start = mult_off;
				have_multisession = sj;

#ifdef CD_EXTRA
				offset = Read_CD_Extra_Info(sj);

				if (offset != 0) {
					have_CD_extra = sj;
					dump_extra_info(offset);
				}
#endif
			}
		}
	}
	if (global.cddbp) {
#if	defined USE_REMOTE
		if (global.disctitle == NULL) {
			have_CDDB = !request_titles();
		}
#else
		errmsgno(EX_BAD,
			_("Cannot lookup titles: no cddbp support included!\n"));
#endif
	}
#if defined CDINDEX_SUPPORT || defined CDDB_SUPPORT
	if (have_CD_text || have_CD_extra || have_CDDB) {
		unsigned long		count_audio_tracks = 0;
		struct iterator	i;

		InitIterator(&i, 1);

		while (i.hasNextTrack(&i)) {
			struct TOC *p = i.getNextTrack(&i);
			if (IS__AUDIO(p))
				count_audio_tracks++;
		}

		if (count_audio_tracks > 0 && global.no_cddbfile == 0) {
#if defined CDINDEX_SUPPORT
			emit_cdindex_form(global.fname_base);
#endif
#if defined CDDB_SUPPORT
			emit_cddb_form(global.fname_base);
#endif
		}
	}
#endif
	if (have_multisession) {
		/*
		 * set start of track to beginning of lead-out
		 */
		patch_cd_extra(j, real_end);
#if	defined CD_EXTRA && defined DEBUG_XTRA
		fprintf(outfp,
			"setting end of session (track %d) to %u\n",
			j, real_end);
#endif
	}
	check_hidden();
	return (offset);
}

LOCAL void
check_hidden()
{
	long		sect;

	if (!global.no_hidden_track &&
	    (sect = Get_AudioStartSector(FirstAudioTrack())) > 0) {
		myringbuff	*p = RB_BASE;	/* Not yet initialized */
		int		i;
		int		n;
		BOOL		isdata = TRUE;

		get_scsi_p()->silent++;
		/*
		 * switch cdrom to data mode
		 */
		EnableCdda(get_scsi_p(), 0, 0);
		i = ReadCdRomData(get_scsi_p(), (Uchar *)p->data, 0, 1);
		if (i != 1) {
			have_hiddenAudioTrack = 1;
			isdata = FALSE;
		}
		if (global.quiet == 0) {
			fprintf(outfp,
			_("%ld sectors of %sdata before track #%ld"),
			sect, !isdata ? _("audio "):"", FirstAudioTrack());
		}
		/*
		 * switch cdrom to audio mode
		 */
		EnableCdda(get_scsi_p(), 1, CD_FRAMESIZE_RAW);
		i = ReadCdRom(get_scsi_p(), p->data, 0, 1);
		if (isdata && i != 1) {
			if (global.quiet == 0) {
				fprintf(outfp,
				_(", ignoring.\n"));
			}
		} else if (i != 1) {
			if (global.quiet == 0) {
				fprintf(outfp,
				_(", unreadable by this drive.\n"));
			}
			have_hiddenAudioTrack = 0;
		} else {
			for (n = 0; n < sect; n += global.nsectors) {
				int	o;

				fillbytes(p->data, CD_FRAMESIZE_RAW, '\0');
				i = ReadCdRom(get_scsi_p(), p->data, n, global.nsectors);
				if (i != global.nsectors)
					break;
				if ((o = cmpnullbytes(p->data, global.nsectors * CD_FRAMESIZE_RAW)) <
				    global.nsectors * CD_FRAMESIZE_RAW) {
					if (global.quiet == 0) {
						fprintf(outfp,
						_(", audible data at sector %d.\n"),
						n + o / CD_FRAMESIZE_RAW);
					}
					break;
				}
			}
			if (n >= sect) {
				have_hiddenAudioTrack = 0;
				if (global.quiet == 0)
					fprintf(outfp, "\n");
			} else {
				if (global.quiet == 0)
					fprintf(outfp,
					_("Hidden audio track with %ld sectors found.\n"), sect);
			}
		}
		get_scsi_p()->silent--;
	}
}

LOCAL int
cddb_sum(n)
	int	n;
{
	int	ret;

	for (ret = 0; n > 0; n /= 10) {
		ret += (n % 10);
	}

	return (ret);
}

void
calc_cddb_id()
{
	UINT4	i;
	UINT4	t = 0;
	UINT4	n = 0;

	for (i = 1; i <= cdtracks; i++) {
		n += cddb_sum(Get_StartSector(i)/75 + 2);
	}

	t = Get_StartSector(i)/75 - Get_StartSector(1)/75;

	global.cddb_id = (n % 0xff) << 24 | (t << 8) | cdtracks;
}


#undef TESTCDINDEX
#ifdef	TESTCDINDEX
void
TestGenerateId()
{
	SHA1_CTX	sha;
	unsigned char	digest[20], *base64;
	unsigned long	size;

	SHA1Init(&sha);
	SHA1Update(&sha, (unsigned char *)"0123456789", 10);
	SHA1Final(digest, &sha);

	base64 = rfc822_binary((char *)digest, 20, &size);
	if (strncmp((char *) base64, "h6zsF82dzSCnFsws9nQXtxyKcBY-", size)) {
		free(base64);

		fprintf(outfp,
		"The SHA-1 hash function failed to properly generate the\n");
		fprintf(outfp, "test key.\n");
		exit(INTERNAL_ERROR);
	}
	free(base64);
}
#endif

void
calc_cdindex_id()
{
	SHA1_CTX 	sha;
	unsigned char	digest[20], *base64;
	unsigned long	size;
	unsigned	i;
	char		temp[9];

#ifdef	TESTCDINDEX
extern	TOC	g_toc[MAXTRK+1]; /* hidden track + 100 regular tracks */

	TestGenerateId();
	g_toc[1].bTrack = 1;
	cdtracks = 15;
	g_toc[cdtracks].bTrack = 15;
	i = 1;
	g_toc[i++].dwStartSector = 0U;
	g_toc[i++].dwStartSector = 18641U;
	g_toc[i++].dwStartSector = 34667U;
	g_toc[i++].dwStartSector = 56350U;
	g_toc[i++].dwStartSector = 77006U;
	g_toc[i++].dwStartSector = 106094U;
	g_toc[i++].dwStartSector = 125729U;
	g_toc[i++].dwStartSector = 149785U;
	g_toc[i++].dwStartSector = 168885U;
	g_toc[i++].dwStartSector = 185910U;
	g_toc[i++].dwStartSector = 205829U;
	g_toc[i++].dwStartSector = 230142U;
	g_toc[i++].dwStartSector = 246659U;
	g_toc[i++].dwStartSector = 265614U;
	g_toc[i++].dwStartSector = 289479U;
	g_toc[i++].dwStartSector = 325732U;
#endif
	SHA1Init(&sha);
	sprintf(temp, "%02X", Get_Tracknumber(1));
	SHA1Update(&sha, (unsigned char *)temp, 2);
	sprintf(temp, "%02X", Get_Tracknumber(cdtracks));
	SHA1Update(&sha, (unsigned char *)temp, 2);

	/* the position of the leadout comes first. */
	sprintf(temp, "%08lX", 150 + Get_StartSector(CDROM_LEADOUT));
	SHA1Update(&sha, (unsigned char *)temp, 8);

	/* now 99 tracks follow with their positions. */
	for (i = 1; i <= cdtracks; i++) {
		sprintf(temp, "%08lX", 150+Get_StartSector(i));
		SHA1Update(&sha, (unsigned char *)temp, 8);
	}
	for (i++; i <= 100; i++) {
		SHA1Update(&sha, (unsigned char *)"00000000", 8);
	}
	SHA1Final(digest, &sha);

	base64 = rfc822_binary((char *)digest, 20, &size);
	global.cdindex_id = base64;
}


#if defined CDDB_SUPPORT

#ifdef	PROTOTYPES
LOCAL void
escape_and_split(FILE *channel, const char *args, ...)
#else
/*VARARGS3*/
LOCAL void
escape_and_split(channel, args, va_alist)
	FILE		*channel;
	const char	*args;
	va_dcl
#endif
{
	va_list	marker;

	int	prefixlen;
	int	len;
	char	*q;

#ifdef	PROTOTYPES
	va_start(marker, args);
#else
	va_start(marker);
#endif

	prefixlen = strlen(args);
	len = prefixlen;
	fputs(args, channel);

	q = va_arg(marker, char *);
	while (*q != '\0') {
		while (*q != '\0') {
			len += 2;
			if (*q == '\\')
				fputs("\\\\", channel);
			else if (*q == '\t')
				fputs("\\t", channel);
			else if (*q == '\n')
				fputs("\\n", channel);
			else {
				fputc(*q, channel);
				len--;
			}
			if (len > 78) {
				fputc('\n', channel);
				fputs(args, channel);
				len = prefixlen;
			}
			q++;
		}
		q = va_arg(marker, char *);
	}
	fputc('\n', channel);

	va_end(marker);
}

LOCAL void
emit_cddb_form(fname_baseval)
	char	*fname_baseval;
{
	struct iterator	i;
	unsigned	first_audio;
	FILE		*cddb_form;
	char		fname[200];
	char		*pp;

	if (fname_baseval == NULL || fname_baseval[0] == 0)
		return;

	if (strcmp(fname_baseval, "standard_output") == 0)
		return;
	InitIterator(&i, 1);

	strncpy(fname, fname_baseval, sizeof (fname) -1);
	fname[sizeof (fname) -1] = 0;
	pp = strrchr(fname, '.');
	if (pp == NULL) {
		pp = fname + strlen(fname);
	}
	strncpy(pp, ".cddb", sizeof (fname) - 1 - (pp - fname));

	cddb_form = fopen(fname, "w");
	if (cddb_form == NULL)
		return;

	first_audio = FirstAudioTrack();
	fprintf(cddb_form, "# xmcd\n#\n");
	fprintf(cddb_form, "# Track frame offsets:\n#\n");

	while (i.hasNextTrack(&i)) {
		struct TOC	*p = i.getNextTrack(&i);

		if (GETTRACK(p) == CDROM_LEADOUT)
			break;
		fprintf(cddb_form, "# %lu\n",
			150 + Get_AudioStartSector(GETTRACK(p)));
	}

	fprintf(cddb_form, "#\n# Disc length: %lu seconds\n#\n",
		(150 + Get_StartSector(CDROM_LEADOUT)) / 75);
	fprintf(cddb_form, "# Revision: %u\n", global.cddb_revision);
	fprintf(cddb_form, "# Submitted via: cdda2wav ");
	fprintf(cddb_form, VERSION);
	fprintf(cddb_form, VERSION_OS);
	fprintf(cddb_form, "\n");

	fprintf(cddb_form, "DISCID=%08lx\n", (unsigned long)global.cddb_id);

	if (global.disctitle == NULL && global.performer == NULL) {
		fprintf(cddb_form, "DTITLE=\n");
	} else {
		if (global.performer == NULL) {
			escape_and_split(cddb_form, "DTITLE=",
							global.disctitle, "");
		} else if (global.disctitle == NULL) {
			escape_and_split(cddb_form, "DTITLE=",
							global.performer, "");
		} else {
			escape_and_split(cddb_form, "DTITLE=",
							global.performer, " / ",
							global.disctitle, "");
		}
	}
	if (global.cddb_year != 0)
		fprintf(cddb_form, "DYEAR=%4u\n", global.cddb_year);
	else
		fprintf(cddb_form, "DYEAR=\n");
	fprintf(cddb_form, "DGENRE=%s\n", global.cddb_genre);

	InitIterator(&i, 1);
	while (i.hasNextTrack(&i)) {
		struct TOC	*p = i.getNextTrack(&i);
		int		ii;

		ii = GETTRACK(p);
		if (ii == CDROM_LEADOUT)
			break;

		if (global.tracktitle[ii] != NULL) {
			char	prefix[10];

			sprintf(prefix, "TTITLE%d=", ii-1);
			escape_and_split(cddb_form, prefix,
						global.tracktitle[ii], "");
		} else {
			fprintf(cddb_form, "TTITLE%d=\n", ii-1);
		}
	}

	if (global.copyright_message == NULL) {
		fprintf(cddb_form, "EXTD=\n");
	} else {
		escape_and_split(cddb_form, "EXTD=", "Copyright ",
						global.copyright_message, "");
	}

	InitIterator(&i, 1);
	while (i.hasNextTrack(&i)) {
		struct TOC	*p = i.getNextTrack(&i);
		int		ii;

		ii = GETTRACK(p);

		if (ii == CDROM_LEADOUT)
			break;

		fprintf(cddb_form, "EXTT%d=\n", ii-1);
	}
	fprintf(cddb_form, "PLAYORDER=\n");
	fclose(cddb_form);
}

#if	defined	USE_REMOTE
#include <schily/pwd.h>

LOCAL int	readn	__PR((register int fd, register char *ptr,
						register int nbytes));
LOCAL int	writen	__PR((register int fd, register char *ptr,
						register int nbytes));

LOCAL int
readn(fd, ptr, nbytes)
	register int	fd;
	register char	*ptr;
	register int	nbytes;
{
	int	nread;

	nread = _niread(fd, ptr, nbytes);
#ifdef	DEBUG_CDDBP
	if (nread > 0) {
		fprintf(outfp, "READ :(%d)", nread);
		write(2, ptr, nread);
	}
#endif
	if (nread < 0) {
		errmsg(_("Socket read error: fd=%d, ptr=%p, nbytes=%d.\n"),
			fd, ptr, nbytes);
		ptr[0] = '\0';
	}

	return (nread);
}

LOCAL int
writen(fd, ptr, nbytes)
	register int	fd;
	register char	*ptr;
	register int	nbytes;
{
	int	nwritten;

	nwritten = _nixwrite(fd, ptr, nbytes);
#ifdef	DEBUG_CDDBP
	if (nwritten <= 0)
		fprintf(outfp, "WRITE:%s\n", ptr);
#endif
	if (nwritten < 0) {
		errmsg(_("Socket write error: fd=%d, ptr=%p, nbytes=%d.\n"),
			fd, ptr, nbytes);
	}
	return (nwritten);
}

#define	SOCKBUFF	2048

LOCAL int	process_cddb_titles	__PR((int sock_fd, char *inbuff,
							int readbytes));
LOCAL int
process_cddb_titles(sock_fd, inbuff, readbytes)
	int	sock_fd;
	char	*inbuff;
	int	readbytes;
{
	int	finished = 0;
	char	*p = inbuff;
	int	ind = 0;
	char	**target = (char **)&global.performer;

	do {
		while (readbytes > 0) {
			/*
			 * do we have a complete line in the buffer?
			 */
			p = (char *)memchr(inbuff+ind, '\n', readbytes);
			if (p == NULL)
				break;

			/*
			 * look for the terminator first
			 */
			if (strncmp(".\r\n", inbuff+ind, 3) == 0) {
				finished = 1;
				break;
			}
			/*
			 * kill carriage return
			 */
			if (p > inbuff+ind && *(p-1) == '\r') {
				*(p-1) = '\0';
			}
			/*
			 * kill line feed
			 */
			*p = '\0';

			/*
			 * handle escaped characters
			 */
			{
				char	*q = inbuff+ind;

				while (*q) {
					if (*q++ == '\\' && *q != '\0') {
						if (*q == '\\') {
							readbytes--;
							p--;
							memmove(q, q+1, readbytes - (q-inbuff-ind));
						} else if (*q == 'n') {
							*(q-1) = '\n';
							readbytes--;
							p--;
							memmove(q, q+1, readbytes - (q-inbuff-ind));
						} else if (*q == 't') {
							*(q-1) = '\t';
							readbytes--;
							p--;
							memmove(q, q+1, readbytes - (q-inbuff-ind));
						}
					}
				}
			}

			/*
			 * handle multi line entries concatenate fields
			 *
			 * TODO if the delimiter is split into two lines,
			 * it is not recognized.
			 */
			if (strncmp(inbuff+ind, "DTITLE=", 7) == 0) {
				char *res = strstr(inbuff+ind+7, " / ");
				int clen;
				char *q;

				if (res == NULL) {
					/*
					 * no limiter found yet
					 * copy until the end
					 */
					q = p;
				} else {
					/*
					 * limiter found
					 * copy until the limiter
					 */
					q = res;
					*q = '\0';
				}

				clen = q - (inbuff+ind+7);
				if (*target == NULL) {
					*target = malloc(clen+1);
					if (*target != NULL)
						**target = '\0';
				} else {
					*target = realloc(*target,
						strlen(*target) + clen + 1);
				}
				if (*target != NULL) {
					strcat((char *)*target, inbuff+ind+7);
				}

				/*
				 * handle part after the delimiter, if present
				 */
				if (res != NULL) {
					target = (char **)&global.disctitle;
					/*
					 * skip the delimiter
					 */
					q += 3;
					clen = p - q;
					if (*target == NULL) {
						*target = malloc(clen+1);
						if (*target != NULL)
							**target = '\0';
					}
					if (*target != NULL) {
						strcat((char *)*target, q);
					}
				}
			} else if (strncmp(inbuff+ind, "TTITLE", 6) == 0) {
				char		*q = (char *)memchr(inbuff+ind, '=', readbytes);
				unsigned	tno;

				if (q != NULL) {
					*q = '\0';
					tno = (unsigned)atoi(inbuff+ind+6);
					tno++;
					if (tno < 100) {
						if (global.tracktitle[tno] == NULL) {
							global.tracktitle[tno] = malloc(p - q + 1);
							if (global.tracktitle[tno] != NULL)
								*(global.tracktitle[tno]) = '\0';
						} else {
							global.tracktitle[tno] =
								realloc(global.tracktitle[tno],
								strlen((char *)global.tracktitle[tno]) +
								p - q + 1);
						}
						if (global.tracktitle[tno] != NULL) {
							strcat((char *)global.tracktitle[tno], q+1);
						}
					}
				}
			} else if (strncmp(inbuff+ind, "DYEAR", 5) == 0) {
				char	*q = (char *)memchr(inbuff+ind, '=', readbytes);
				if (q++ != NULL) {
					sscanf(q, "%d", &global.cddb_year);
				}
			} else if (strncmp(inbuff+ind, "DGENRE", 6) == 0) {
				char	*q = (char *)memchr(inbuff+ind, '=', readbytes);
				if (q++ != NULL) {
					/*
					 * patch from Joe Nuzman, thanks
					 * might have significant whitespace
					 */
					strncpy(global.cddb_genre, q, sizeof (global.cddb_genre)-1);
					/*
					 * always have a terminator
					 */
					global.cddb_genre[sizeof (global.cddb_genre)-1] = '\0';
				}
			} else if (strncmp(inbuff+ind, "# Revision: ", 12) == 0) {
				char	*q = inbuff+ind+11;
				sscanf(q, "%d", &global.cddb_revision);
				global.cddb_revision++;
			}
			readbytes -= (p - inbuff -ind) + 1;
			ind = (p - inbuff) + 1;
		}
		if (!finished) {
			int	newbytes;
			memmove(inbuff, inbuff+ind, readbytes);
			newbytes = readn(sock_fd, inbuff+readbytes, SOCKBUFF-readbytes);
			if (newbytes <= 0)
				break;
			readbytes += newbytes;
			ind = 0;
		}
	} while (!(finished || readbytes == 0));
	return (finished);
}

LOCAL int	handle_userchoice	__PR((char *p, unsigned size));

LOCAL int
handle_userchoice(p, size)
	char		*p;
	unsigned	size;
{
	unsigned	nr = 0;
	unsigned	user_choice;
	int		i;
	char		*q;
	char		*o;

	/*
	 * count lines.
	 */
	q = p;
	while ((q = (char *)memchr(q, '\n', size - (q-p))) != NULL) {
		nr++;
		q++;
	}
	if (nr > 1)
		nr--;

	/*
	 * handle escaped characters
	 */
	{
		char	*r = p;

		while (*r) {
			if (*r++ == '\\' && *r != '\0') {
				if (*r == '\\') {
					size--;
					memmove(r, r+1, size - (r-p));
				} else if (*r == 'n') {
					*(r-1) = '\n';
					size--;
					memmove(r, r+1, size - (r-p));
				} else if (*r == 't') {
					*(r-1) = '\t';
					size--;
					memmove(r, r+1, size - (r-p));
				}
			}
		}
	}

	/*
	 * list entries.
	 */
	q = p;
	fprintf(outfp, _("%u entries found:\n"), nr);
	for (q = (char *)memchr(q, '\n', size - (q-p)), o = p, i = 0;
								i < nr; i++) {
		*q = '\0';
		fprintf(outfp, "%02u: %s\n", i, o);
		o = q+1;
		q = (char *)memchr(q, '\n', size - (q-p));
	}
	fprintf(outfp, _("%02u: ignore\n"), i);

	/*
	 * get user response.
	 * Some OS seem to misshandle stderr and make it buffered, so we
	 * call fflush(outfp) here.
	 */
	do {
		fprintf(outfp, _("please choose one (0-%u): "), nr);
		fflush(outfp);
		if (scanf("%u", &user_choice) != 1)
			user_choice = nr;
	} while (user_choice > nr);

	if (user_choice == nr)
		return (-1);

	/*
	 * skip to choice.
	 */
	q = p;
	for (i = 0; i <= (int)user_choice - 1; i++) {
		q = (char *)memchr(q, '\0', size - (q-p)) + 1;
	}
	return (q-p);
}

/*
 * request disc and track titles from a cddbp server.
 *
 * return values:
 *	0	titles have been found exactly (success)
 *	-1	some communication error happened.
 *	1	titles have not been found.
 *	2	multiple fuzzy matches have been found.
 */
EXPORT int
request_titles()
{
	int		retval = 0;
	int		sock_fd;
	struct sockaddr_in sa;
	struct hostent	*he;
	struct servent	*se;
	struct passwd	*pw = getpwuid(getuid());
	char		hostname[MAXHOSTNAMELEN+1];
	char		inbuff[SOCKBUFF];
	char		outbuff[SOCKBUFF];
	int		i;
	char		category[64];
	unsigned	cat_offset;
	unsigned	disc_id;
	ssize_t		readbytes;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		errmsg(_("Cddb socket failed.\n"));
		retval = -1;
		goto errout;
	}

	/*
	 * TODO fallbacks
	 * freedb.freedb.org
	 * de.freedb.org
	 * at.freedb.org
	 */
	if (global.cddbp_server != NULL)
		he = gethostbyname(global.cddbp_server);
	else
		he = gethostbyname(CDDBHOST /*"freedb.freedb.org"*/);

	/*
	 * save result data IMMEDIATELY!!
	 */
	memset(&sa, 0, sizeof (struct sockaddr_in));

	if (he != NULL) {
		sa.sin_family	   = he->h_addrtype;	/* AF_INET; */
		sa.sin_addr.s_addr = ((struct in_addr *)((he->h_addr_list)[0]))->s_addr;
	} else {
		errmsg(_("Cddb cannot resolve freedb host.\n"));
		sa.sin_family	   = AF_INET;
		sa.sin_addr.s_addr = htonl(UINT_C(0x526256aa)); /* freedb.freedb.de */
	}

	se = NULL;
	if (global.cddbp_port != NULL) {
		sa.sin_port = htons(atoi(global.cddbp_port));
	} else {
		se = getservbyname("cddbp-alt", "tcp");

		if (se == NULL) {
			if (global.cddbp_port == NULL) {
				se = getservbyname("cddbp", "tcp");
			}
		}
		if (se != NULL) {
			sa.sin_port = se->s_port;
		} else {
#if	0
			errmsg("Cddb cannot resolve cddbp or cddbp-alt port.\n");
#endif
			sa.sin_port = htons(CDDBPORT /*8880*/);
		}
	}

	/* TODO timeout */
	if (0 > connect(sock_fd, (struct sockaddr *)&sa,
			sizeof (struct sockaddr_in))) {
		errmsg(_("Cddb connect failed.\n"));
		retval = -1;
		goto errout;
	}

	/*
	 * read banner
	 */
	readbytes = readn(sock_fd, inbuff, sizeof (inbuff));
	if (readbytes < 0) {
		retval = -1;
		goto errout;
	}
	if (strncmp(inbuff, "200 ", 4) && strncmp(inbuff, "201 ", 4)) {
		errmsgno(EX_BAD,
		_("Bad status from freedb server during sign-on banner: %s.\n"), inbuff);
		retval = -1;
		goto errout;
	}

	/*
	 * say hello
	 */
	hostname[0] = '\0';
	if (0 > gethostname(hostname, sizeof (hostname)))
		strcpy(hostname, "unknown_host");
	hostname[sizeof (hostname)-1] = '\0';
	writen(sock_fd, "cddb hello ", 11);
	if (pw != NULL) {
		BOOL	space_err = FALSE;
		BOOL	ascii_err = FALSE;
		/*
		 * change spaces to underscores
		 */
		char	*q = pw->pw_name;
		while (*q != '\0') {
			if (*q == ' ') {
				if (!space_err) {
					space_err = TRUE;
					errmsgno(EX_BAD,
					_("Warning: Space in user name '%s'.\n"),
					pw->pw_name);
				}
				*q = '_';
			}
			if (*q < ' ' || *q > '~') {
				if (!ascii_err) {
					ascii_err = TRUE;
					errmsgno(EX_BAD,
					_("Warning: Nonascii character in user name '%s'.\n"),
					pw->pw_name);
				}
				*q = '_';
			}
			q++;
		}
		writen(sock_fd, pw->pw_name, strlen(pw->pw_name));
	} else {
		writen(sock_fd, "unknown", 7);
	}
	writen(sock_fd, " ", 1);

	/* change spaces to underscores */
	{
		char *q = hostname;
		BOOL	space_err = FALSE;
		BOOL	ascii_err = FALSE;

		while (*q != '\0') {
			if (*q == ' ') {
				if (!space_err) {
					space_err = TRUE;
					errmsgno(EX_BAD,
					_("Warning: Space in hostname '%s'.\n"),
					hostname);
				}
				*q = '_';
			}
			if (*q < ' ' || *q > '~') {
				if (!ascii_err) {
					ascii_err = TRUE;
					errmsgno(EX_BAD,
					_("Warning: Nonascii character in hostname '%s'.\n"),
					hostname);
				}
				*q = '_';
			}
			q++;
		}
	}

	writen(sock_fd, hostname, strlen(hostname));
	writen(sock_fd, " cdda2wav ", 10);
	writen(sock_fd, VERSION, strlen(VERSION));
	writen(sock_fd, VERSION_OS, strlen(VERSION_OS));
	writen(sock_fd, "\n", 1);

	readbytes = readn(sock_fd, inbuff, sizeof (inbuff));
	if (readbytes < 0) {
		retval = -1;
		goto signoff;
	}
	if (strncmp(inbuff, "200 ", 4)) {
		inbuff[readbytes] = '\0';
		errmsgno(EX_BAD,
		_("Bad status from freedb server during hello: %s.\n"), inbuff);
		retval = -1;
		goto signoff;
	}

	/*
	 * enable new protocol variant. Weird command here, no cddb prefix ?!?!
	 */
	writen(sock_fd, "proto\n", 6);
	readbytes = readn(sock_fd, inbuff, sizeof (inbuff));
	if (readbytes < 0) {
		retval = -1;
		goto signoff;
	}
	/*
	 * check for errors and maximum supported protocol level
	 */
	if (strncmp(inbuff, "201 ", 4) > 0) {
		inbuff[readbytes] = '\0';
		errmsgno(EX_BAD,
		_("Bad status from freedb server during proto command: %s.\n"),
			inbuff);
		retval = -1;
		goto signoff;
	}

	/*
	 * check the supported protocol level
	 */
	if (!memcmp(inbuff, "200 CDDB protocol level: current 1, supported ",
									46)) {
		char		*q = strstr(inbuff, " supported ");
		unsigned	pr_level;

		if (q != NULL) {
			q += 11;
			sscanf(q, "%u\n", &pr_level);
			if (pr_level > 1) {
				if (pr_level > 5)
					pr_level = 5;
				sprintf(inbuff, "proto %1u\n", pr_level);
				writen(sock_fd, inbuff, 8);
				readbytes = readn(sock_fd, inbuff,
							sizeof (inbuff));
				if (readbytes < 0) {
					retval = -1;
					goto signoff;
				}
				/*
				 * check for errors and maximum supported
				 * protocol level
				 */
				if (strncmp(inbuff, "201 ", 4) > 0) {
					inbuff[readbytes] = '\0';
					errmsgno(EX_BAD,
					_("Bad status from freedb server during proto x: %s.\n"),
						inbuff);
					retval = -1;
					goto signoff;
				}
			}
		}
	}

	/*
	 * format query string
	 */
	/*
	 * query
	 */
#define	CDDPB_INCLUDING_DATATRACKS
#ifdef	CDDPB_INCLUDING_DATATRACKS
	sprintf(outbuff, "cddb query %08lx %ld ",
		(unsigned long)global.cddb_id, LastTrack() - FirstTrack() + 1);
	/*
	 * first all leading datatracks
	 */
	{
		int j = FirstAudioTrack();
		if (j < 0)
			j = LastTrack() +1;
		for (i = FirstTrack(); i < j; i++) {
			sprintf(outbuff + strlen(outbuff), "%ld ",
					150 + Get_StartSector(i));
		}
	}
#else
	sprintf(outbuff, "cddb query %08lx %ld ",
			global.cddb_id,
			LastAudioTrack() - FirstAudioTrack() + 1);
#endif
	/*
	 * all audio tracks
	 */
	for (i = FirstAudioTrack(); i != -1 && i <= LastAudioTrack(); i++) {
		sprintf(outbuff + strlen(outbuff), "%ld ",
			150 + Get_AudioStartSector(i));
	}
#ifdef	CDDPB_INCLUDING_DATATRACKS
	/*
	 * now all trailing datatracks
	 */
	for (; i != -1 && i <= LastTrack(); i++) {
		sprintf(outbuff + strlen(outbuff), "%ld ",
			150 + Get_StartSector(i));
	}
	sprintf(outbuff + strlen(outbuff), "%lu\n",
		(150 + Get_StartSector(CDROM_LEADOUT)) / 75);
#else
	sprintf(outbuff + strlen(outbuff), "%lu\n",
		(150 + Get_LastSectorOnCd(FirstAudioTrack())) / 75);
#endif
/*	strcpy(outbuff, */
/*	"cddb query 9709210c 12 150 12010 33557 50765 65380 81467 93235 109115 124135 137732 152575 166742 2339\n"); */
/*	strcpy(outbuff, "cddb query 03015501 1 296 344\n"); */
/*
 * The next line is a CD with Tracktitle=     'Au fond d'un rêve doré'
 */
/*	sprintf(outbuff, "cddb query %s\n", "35069e05 5 150 12471 25528 35331 56166 1696");*/

	writen(sock_fd, outbuff, strlen(outbuff));

	readbytes = readn(sock_fd, inbuff, sizeof (inbuff));
	if (readbytes < 0) {
		retval = -1;
		goto signoff;
	}
	inbuff[readbytes] = '\0';
	cat_offset = 4;
	if (strncmp(inbuff, "210 ", 4) == 0 ||
	    strncmp(inbuff, "211 ", 4) == 0) {
		/*
		 * Check if there are really multiple entries.
		 */
		char *p = (char *)memchr(inbuff, '\n', readbytes-1);

		if (p != NULL)
			cat_offset = p+1 - inbuff;
		/*
		 * first entry
		 */
		if (p)
			p = (char *)memchr(p+1, '\n', inbuff+readbytes - p);
		/*
		 * second entry
		 */
		if (p)
			p = (char *)memchr(p+1, '\n', inbuff+readbytes - p);
		/*
		 * .
		 */
		if (p)
			p = (char *)memchr(p+1, '\n', inbuff+readbytes - p);
		if (p) {
			/* multiple entries */
			switch (global.cddbp) {
				case	2:	/* take the first entry */
				break;
				case	1:	/* ask user */
					if (!global.gui) {
						int userret = handle_userchoice(inbuff+cat_offset, readbytes - cat_offset);
						if (userret == -1) {
							/*
							 * ignore any selection
							 */
							retval = -1;
							goto signoff;
						}
						cat_offset += userret;
					}
				break;
				default:
					errmsgno(EX_BAD,
					_("Multiple entries found: %s.\n"), inbuff);
					retval = 2;
					goto signoff;
			}
		}

	} else if (strncmp(inbuff, "200 ", 4)) {
		if (strncmp(inbuff, "202 ", 4) == 0) {
			errmsgno(EX_BAD, _("No cddb entry found: %s.\n"), inbuff);
			retval = 1;
		} else {
			errmsgno(EX_BAD,
			_("Bad status from freedb server during query: %s.\n%s"),
				inbuff, outbuff);
			retval = -1;
		}
		goto signoff;
	}
	sscanf(inbuff + cat_offset, "%s %x", category, &disc_id);


	/*
	 * read
	 */
	sprintf(inbuff, "cddb read %s %08x\n", category, disc_id);
/*
 * The next line is a CD with Tracktitle=     'Au fond d'un rêve doré'
 */
/*	sprintf(inbuff, "cddb read %s %08x\n", category, 0x35069e05);*/
	writen(sock_fd, inbuff, strlen(inbuff));

	/*
	 * read status and first buffer size.
	 */
	readbytes = readn(sock_fd, inbuff, sizeof (inbuff));
	if (readbytes < 0) {
		retval = -1;
		goto signoff;
	}
	if (strncmp(inbuff, "210 ", 4)) {
		inbuff[readbytes] = '\0';
		errmsgno(EX_BAD,
			_("Bad status from freedb server during read: %s.\n"),
			inbuff);
		retval = -1;
		goto signoff;
	}

	if (1 != process_cddb_titles(sock_fd, inbuff, readbytes)) {
		errmsgno(EX_BAD, _("Cddb read finished not correctly!\n"));
	}

signoff:
	/*
	 * sign-off
	 */
	writen(sock_fd, "quit\n", 5);
	readbytes = readn(sock_fd, inbuff, sizeof (inbuff));
	if (readbytes < 0) {
		retval = -1;
		goto errout;
	}
	if (strncmp(inbuff, "230 ", 4)) {
		inbuff[readbytes] = '\0';
		errmsgno(EX_BAD,
			_("Bad status from freedb server during quit: %s.\n"),
			inbuff);
		goto errout;
	}

errout:
	close(sock_fd);
	return (retval);
}
#endif
#endif

#if	defined CDINDEX_SUPPORT

LOCAL int	IsSingleArtist	__PR((void));

/*
 * check, if there are more than one track performers
 */
LOCAL int
IsSingleArtist()
{
	struct iterator	i;

	InitIterator(&i, 1);

	while (i.hasNextTrack(&i)) {
		struct TOC *p = i.getNextTrack(&i);
		int ii;

		if (IS__DATA(p) || GETTRACK(p) == CDROM_LEADOUT)
			continue;

		ii = GETTRACK(p);
		if (global.performer && global.trackperformer[ii] &&
		    strcmp((char *) global.performer,
		    (char *) global.trackperformer[ii]) != 0)
			return (0);
	}
	return (1);
}

LOCAL const char *a2h[255-191] = {
"&Agrave;",
"&Aacute;",
"&Acirc;",
"&Atilde;",
"&Auml;",
"&Aring;",
"&AElig;",
"&Ccedil;",
"&Egrave;",
"&Eacute;",
"&Ecirc;",
"&Euml;",
"&Igrave;",
"&Iacute;",
"&Icirc;",
"&Iuml;",
"&ETH;",
"&Ntilde;",
"&Ograve;",
"&Oacute;",
"&Ocirc;",
"&Otilde;",
"&Ouml;",
"&times;",
"&Oslash;",
"&Ugrave;",
"&Uacute;",
"&Ucirc;",
"&Uuml;",
"&Yacute;",
"&THORN;",
"&szlig;",
"&agrave;",
"&aacute;",
"&acirc;",
"&atilde;",
"&auml;",
"&aring;",
"&aelig;",
"&ccedil;",
"&egrave;",
"&eacute;",
"&ecirc;",
"&euml;",
"&igrave;",
"&iacute;",
"&icirc;",
"&iuml;",
"&eth;",
"&ntilde;",
"&ograve;",
"&oacute;",
"&ocirc;",
"&otilde;",
"&ouml;",
"&divide;",
"&oslash;",
"&ugrave;",
"&uacute;",
"&ucirc;",
"&uuml;",
"&yacute;",
"&thorn;",
"&yuml;",
};

LOCAL char	*ascii2html	__PR((unsigned char *inp));

LOCAL char *
ascii2html(inp)
	unsigned char	*inp;
{
static unsigned char	outline[300];
	unsigned char	*outp = outline;

#define	copy_translation(a, b)	else if (*inp == (a)) \
				{	strcpy((char *)outp, (b)); \
					outp += sizeof ((b))-1; }

	while (*inp != '\0') {
		if (0);
		copy_translation('"', "&quot;")
		copy_translation('&', "&amp;")
		copy_translation('<', "&lt;")
		copy_translation('>', "&gt;")
		copy_translation(160, "&nbsp;")
		else if (*inp < 192) {
			*outp++ = *inp;
		} else {
			strcpy((char *)outp, a2h[*inp-192]);
			outp += strlen(a2h[*inp-192]);
		}
		inp++;
	}
	*outp = '\0';
	return ((char *) outline);
}
#undef copy_translation

LOCAL void
emit_cdindex_form(fname_baseval)
	char	*fname_baseval;
{
	FILE	*cdindex_form;
	char	fname[200];
	char	*pp;

	if (fname_baseval == NULL || fname_baseval[0] == 0)
		return;

	strncpy(fname, fname_baseval, sizeof (fname) -1);
	fname[sizeof (fname) -1] = 0;
	pp = strrchr(fname, '.');
	if (pp == NULL) {
		pp = fname + strlen(fname);
	}
	strncpy(pp, ".cdindex", sizeof (fname) - 1 - (pp - fname));

	cdindex_form = fopen(fname, "w");
	if (cdindex_form == NULL)
		return;

#define	CDINDEX_URL	"http://www.musicbrainz.org/dtd/CDInfo.dtd"

	/*
	 * format XML page according to cdindex DTD (see www.musicbrainz.org)
	 */
	fprintf(cdindex_form,
	"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n<!DOCTYPE CDInfo SYSTEM \"%s\">\n\n<CDInfo>\n",
		CDINDEX_URL);

	fprintf(cdindex_form, "   <Title>%s</Title>\n",
		global.disctitle ? ascii2html(global.disctitle) : "");
	/*
	 * In case of mixed mode and Extra-CD, nonaudio tracks are included!
	 */
	fprintf(cdindex_form,
		"   <NumTracks>%d</NumTracks>\n\n",
			cdtracks);
	fprintf(cdindex_form,
	"   <IdInfo>\n      <DiskId>\n         <Id>%s</Id>\n      </DiskId>\n",
			global.cdindex_id);
	fprintf(cdindex_form,
		"   </IdInfo>\n\n");

	if (IsSingleArtist()) {
		struct iterator	i;

		InitIterator(&i, 1);

		fprintf(cdindex_form,
			"   <SingleArtistCD>\n      <Artist>%s</Artist>\n",
			global.performer ? ascii2html(global.performer) : "");

		while (i.hasNextTrack(&i)) {
			struct TOC	*p = i.getNextTrack(&i);
			unsigned int	ii = GETTRACK(p);

			if (ii == CDROM_LEADOUT)
				break;
			if (IS__AUDIO(p)) {
				fprintf(cdindex_form,
				"      <Track Num=\"%u\">\n         <Name>%s</Name>\n      </Track>\n",
				    ii, global.tracktitle[ii] ? ascii2html(global.tracktitle[ii]) : "");
			} else {
				fprintf(cdindex_form,
				"      <Track Num=\"%u\">\n         <Name>data track</Name>\n      </Track>\n",
				    ii);
			}
		}
		fprintf(cdindex_form, "   </SingleArtistCD>\n");
	} else {
		struct iterator	i;

		InitIterator(&i, 1);

		fprintf(cdindex_form, "   <MultipleArtistCD>\n");

		while (i.hasNextTrack(&i)) {
			struct TOC	*p = i.getNextTrack(&i);
			unsigned int	ii = GETTRACK(p);

			if (ii == CDROM_LEADOUT)
				break;
			if (IS__AUDIO(p)) {
				fprintf(cdindex_form, "         <Artist>%s</Artist>\n",
					global.trackperformer[ii] ? ascii2html(global.trackperformer[ii]) : "");
				fprintf(cdindex_form,
				"         <Name>%s</Name>\n      </Track>\n",
					global.tracktitle[ii] ?
					ascii2html(global.tracktitle[ii]) : "");
			} else {
				fprintf(cdindex_form,
				"         <Artist>data track</Artist>\n         <Name>data track</Name>\n      </Track>\n");
			}
		}
		fprintf(cdindex_form, "   </MultipleArtistCD>\n");
	}
	fprintf(cdindex_form, "</CDInfo>\n");

	fclose(cdindex_form);
}
#endif

LOCAL void
dump_cdtext_info()
{
#ifdef CD_TEXT
	/*
	 * interpret the contents of CD Text information based on an early
	 * draft of SCSI-3 mmc version 2 from jan 2, 1998
	 * CD Text information consists of a text group containing up to
	 * 8 language blocks containing up to
	 * 255 Pack data chunks of
	 * 18 bytes each.
	 * So we have at most 36720 bytes to cope with.
	 */
	short int	datalength;
	int		fulllength;
	BOOL		text_ok = TRUE;
	unsigned char	*p = (unsigned char *)global.buf;
	unsigned char	lastline[255*12];
	int		lastitem = -1;
	int		itemcount = 0;
	int		inlinecount = 0;
	int		outlinecount = 0;

	lastline[0] = '\0';
	datalength = ((p[0] << 8) + p[1]) + 2;
	datalength = min(datalength, global.bufsize);
	fulllength = datalength;
	p += 4;
	datalength -= 4;
	for (; datalength > 0;
			datalength -= sizeof (cdtextpackdata),
			p += sizeof (cdtextpackdata)) {
		unsigned char	*zeroposition;

		/*
		 * handle one packet of CD Text Information Descriptor Pack Data
		 * this is raw R-W subchannel data converted to 8 bit values.
		 */
		cdtextpackdata	*c = (cdtextpackdata *)p;
		int		dbcc;
		int		crc_error;
		unsigned	tracknr;

#ifdef DEBUG_CDTEXT
		fprintf(outfp, "datalength =%d\n", datalength);
#endif
		crc_error = !cdtext_crc_ok(c);
		if (crc_error)
			text_ok = FALSE;

		if (lastitem != c->headerfield[0]) {
			itemcount = 0;
			lastitem = c->headerfield[0];
		}

		tracknr = c->headerfield[1] & 0x7f;
		dbcc = ((unsigned)(c->headerfield[3] & 0x80)) >> 7; /* double byte character code */

#if defined DEBUG_CDTEXT
		{
		int	extension_flag;
		int	sequence_number;
		int	block_number;
		int	character_position;

		extension_flag = ((unsigned)(c->headerfield[1] & 0x80)) >> 7;
		sequence_number = c->headerfield[2];
		block_number = ((unsigned)(c->headerfield[3] & 0x30)) >> 4; /* language */
		character_position = c->headerfield[3] & 0x0f;

		fprintf(outfp, _("CDText: ext_fl=%d, trnr=%u, seq_nr=%d, dbcc=%d, block_nr=%d, char_pos=%d\n"),
			extension_flag, tracknr, sequence_number, dbcc, block_number, character_position);
		}
#endif

		/*
		 * print ASCII information
		 */
		memcpy(lastline+inlinecount, c->textdatafield, 12);
		inlinecount += 12;
		zeroposition = (unsigned char *)memchr(lastline+outlinecount, '\0', inlinecount-outlinecount);
		while (zeroposition != NULL) {
			process_header(c, tracknr, dbcc, lastline+outlinecount);
			outlinecount += zeroposition - (lastline+outlinecount) + 1;

#if defined DEBUG_CDTEXT
			fprintf(outfp,
				"\tin=%d, out=%d, items=%d, trcknum=%u\n",
				inlinecount, outlinecount, itemcount, tracknr);
			{ int	q;

			for (q = outlinecount; q < inlinecount; q++)
				fprintf(outfp, "%c",
					lastline[q] ? lastline[q] : 'ß');
			fputs("\n", outfp);
			}
#else
			if (DETAILED) {
				if (crc_error)
					fputs(_(" ! uncorr. CRC-Error"), outfp);
				fputs("\n", outfp);
			}
#endif

			itemcount++;
			if (itemcount > (int)cdtracks ||
			    (c->headerfield[0] == 0x8f ||
			    (c->headerfield[0] <= 0x8d &&
			    c->headerfield[0] >= 0x86))) {
				outlinecount = inlinecount;
				break;
			}
			tracknr++;
			zeroposition = (unsigned char *)memchr(lastline+outlinecount, '\0', inlinecount-outlinecount);
		}
	}
	if (!global.no_textfile && text_ok && fulllength > 4) {
		char	fname[200];
		char	*pp;
		FILE	*f;

		strncpy(fname, global.fname_base, sizeof (fname) -1);
		fname[sizeof (fname) -1] = 0;
		pp = strrchr(fname, '.');
		if (pp == NULL)
			pp = fname + strlen(fname);
		strncpy(pp, ".cdtext", sizeof (fname) - 1 - (pp - fname));

		f = fileopen(fname, "wctb");
		if (f) {
			filewrite(f, global.buf, fulllength);
			fclose(f);
			global.did_textfile = 1;
		}
	}
#endif
}

LOCAL void
dump_extra_info(from)
	unsigned int	from;
{
#ifdef CD_EXTRA
	unsigned char	*p;

	if (from == 0)
		return;

	p = Extra_buffer + 48;
	while (*p != '\0') {
		unsigned	pos;
		unsigned	length;

		pos    = GET_BE_UINT_FROM_CHARP(p+2);
		length = GET_BE_UINT_FROM_CHARP(p+6);
		if (pos == (unsigned)-1) {
			pos = from+1;
		} else {
			pos += session_start;
		}

#ifdef DEBUG_XTRA
		if (global.gui == 0 && global.verbose != 0) {
			fprintf(outfp,
				"Language: %c%c (as defined by ISO 639)",
				*p, *(p+1));
			fprintf(outfp,
				" at sector %u, len=%u (sessionstart=%u)",
				pos, length, session_start);
			fputs("\n", outfp);
		}
#endif
		/*
		 * dump this entry
		 */
		Read_Subinfo(pos, length);
		p += 10;

		if (p + 9 > (Extra_buffer + CD_FRAMESIZE))
			break;
	}
#endif
}

LOCAL char	*quote	__PR((unsigned char *string));

LOCAL char *
quote(string)
	unsigned char	*string;
{
static char		result[200];
	unsigned char	*p = (unsigned char *)result;

	while (*string) {
		if (*string == '\'' || *string == '\\') {
			*p++ = '\\';
		}
		*p++ = *string++;
	}
	*p = '\0';

	return (result);
}



LOCAL void	DisplayToc_with_gui	__PR((unsigned long dw));

LOCAL void
DisplayToc_with_gui(dw)
	unsigned long	dw;
{
	unsigned	mins;
	unsigned	secnds;
	unsigned	frames;
	int		count_audio_trks;
	struct iterator	i;

	InitIterator(&i, 1);

	mins	=  dw / (60*75);
	secnds  = (dw % (60*75)) / 75;
	frames  = (dw % 75);

	/*
	 * summary
	 */
	count_audio_trks = 0;

	if ((global.verbose & SHOW_STARTPOSITIONS) != 0) {
		if (global.illleadout_cd != 0 && have_CD_extra == 0) {
			fprintf(outfp,
				_("Tracks:%u > %u:%02u.%02u\n"),
				cdtracks, mins, secnds, frames);
		} else {
			fprintf(outfp,
				_("Tracks:%u %u:%02u.%02u\n"),
				cdtracks, mins, secnds, frames);
		}
	}

	if (global.quiet == 0) {
		fprintf(outfp, _("CDINDEX discid: %s\n"), global.cdindex_id);
		fprintf(outfp, _("CDDB discid: 0x%08lx"),
				(unsigned long) global.cddb_id);

		if (have_CDDB != 0) {
			fprintf(outfp, _(" CDDBP titles: resolved\n"));
		} else {
			fprintf(outfp, "\n");
		}
		if (have_CD_text != 0) {
			fprintf(outfp, _("CD-Text: detected\n"));
			dump_cdtext_info();
		} else {
			fprintf(outfp, _("CD-Text: not detected\n"));
		}
		if (have_CD_extra != 0) {
			fprintf(outfp, _("CD-Extra: detected\n"));
			dump_extra_info(have_CD_extra);
		} else {
			fprintf(outfp, _("CD-Extra: not detected\n"));
		}

		fprintf(outfp,
			_("Album title: '%s'"), (void *)global.disctitle != NULL
			? quote(global.disctitle) : "");

		fprintf(outfp, _(" from '%s'\n"), (void *)global.performer != NULL
			? quote(global.performer) : "");
	}
	count_audio_trks = 0;


	if ((global.verbose &
	    (SHOW_TOC | SHOW_STARTPOSITIONS |
			SHOW_SUMMARY | SHOW_TITLES)) != 0 &&
	    i.hasNextTrack(&i)) {
		TOC *o = i.getNextTrack(&i);
		while (i.hasNextTrack(&i)) {
			TOC *p = i.getNextTrack(&i);
			int from;
			from = GETTRACK(o);

			fprintf(outfp,	"T%02d:", from);

			if (IS__DATA(o)) {
				/*
				 * Special case of cd extra
				 */
				unsigned int real_start = have_CD_extra
					? have_CD_extra	: GETSTART(o);


				dw = (unsigned long) (GETSTART(p) - real_start);

				mins   =  dw / (60*75);
				secnds = (dw % (60*75)) / 75;
				frames = (dw % 75);

				if (global.verbose & SHOW_STARTPOSITIONS)
					fprintf(outfp,
						" %7u",
						real_start);

				if (global.verbose & SHOW_TOC)
					fprintf(outfp,
						" %2u:%02u.%02u",
						mins, secnds, frames);

				if (global.verbose & SHOW_SUMMARY)
					fprintf(outfp,
						_(" data %s %s N/A"),

						/* how recorded */
						IS__INCREMENTAL(o)
						? _("incremental") :
						_("uninterrupted"),

						/* copy-permission */
						IS__COPYRIGHTED(o)
						? _("copydenied") :
						_("copyallowed"));
				fputs("\n", outfp);
			} else {
				dw = (unsigned long) (GETSTART(p) -
							GETSTART(o));
				mins   =  dw / (60*75);
				secnds = (dw % (60*75)) / 75;
				frames = (dw %  75);

				if (global.verbose & SHOW_STARTPOSITIONS)
					fprintf(outfp,
						" %7u",
						GETSTART(o));

				if (global.verbose & SHOW_TOC)
					fprintf(outfp,
						" %2u:%02u.%02u",
						mins, secnds, frames);

				if (global.verbose & SHOW_SUMMARY)
					fprintf(outfp,
						_(" audio %s %s %s"),

					/* how recorded */
					IS__PREEMPHASIZED(o)
					? _("pre-emphasized") : _("linear"),

					/* copy-permission */
					IS__COPYRIGHTED(o)
					? _("copydenied") : _("copyallowed"),

					/* channels */
					IS__QUADRO(o)
						? _("quadro") : _("stereo"));

				/* Title */
				if (global.verbose & SHOW_TITLES) {
					fprintf(outfp,
						_(" title '%s' from "),

						(void *) global.tracktitle[GETTRACK(o)] != NULL
						? quote(global.tracktitle[GETTRACK(o)]) : "");

					fprintf(outfp,
						"'%s'",

						(void *) global.trackperformer[GETTRACK(o)] != NULL
						? quote(global.trackperformer[GETTRACK(o)]) : "");
				}
				fputs("\n", outfp);
				count_audio_trks++;
			}
			o = p;
		} /* while */
		if (global.verbose & SHOW_STARTPOSITIONS)
			if (GETTRACK(o) == CDROM_LEADOUT) {
				fprintf(outfp, _("Leadout: %7u\n"), GETSTART(o));
			}
	} /* if */
}

LOCAL void DisplayToc_no_gui __PR((unsigned long dw));

LOCAL void
DisplayToc_no_gui(dw)
	unsigned long	dw;
{
	unsigned mins;
	unsigned secnds;
	unsigned frames;
	int count_audio_trks;
	unsigned ii = 0;
	struct iterator i;
	InitIterator(&i, 1);

	mins	=  dw / (60*75);
	secnds  = (dw % (60*75)) / 75;
	frames  = (dw % 75);

	/* summary */
	count_audio_trks = 0;

	if (i.hasNextTrack(&i)) {
		TOC *o = i.getNextTrack(&i);
		while (i.hasNextTrack(&i)) {
			TOC *p = i.getNextTrack(&i);
			int from;
			from = GETTRACK(o);


			while (p != NULL && GETTRACK(p) != CDROM_LEADOUT &&
				GETFLAGS(o) == GETFLAGS(p)) {
				o = p;
				p = i.getNextTrack(&i);
			}
			if ((global.verbose & SHOW_SUMMARY) == 0)
				continue;

			if (IS__DATA(o)) {
				fputs(_(" DATAtrack recorded      copy-permitted tracktype\n"), outfp);
				fprintf(outfp,
					_("     %2d-%2d %13.13s %14.14s      data\n"),
					from,
					GETTRACK(o),
					/* how recorded */
					IS__INCREMENTAL(o)
					? _("incremental") : _("uninterrupted"),

					/* copy-perm */
					IS__COPYRIGHTED(o) ? _("no") : _("yes"));
			} else {
				fputs(_("AUDIOtrack pre-emphasis  copy-permitted tracktype channels\n"), outfp);
				fprintf(outfp,
					_("     %2d-%2d %12.12s  %14.14s     audio    %1c\n"),
					from,
					GETTRACK(o),
					IS__PREEMPHASIZED(o)
					? _("yes") : _("no"),
					IS__COPYRIGHTED(o) ? _("no") : _("yes"),
					IS__QUADRO(o) ? '4' : '2');
				count_audio_trks++;
			}
			o = p;
		}
	}
	if ((global.verbose & SHOW_STARTPOSITIONS) != 0) {
		if (global.illleadout_cd != 0 && have_multisession == 0) {

			fprintf(outfp,
			_("Table of Contents: total tracks:%u, (total time more than %u:%02u.%02u)\n"),
				cdtracks, mins, secnds, frames);
		} else {
			fprintf(outfp,
			_("Table of Contents: total tracks:%u, (total time %u:%02u.%02u)\n"),
				cdtracks, mins, secnds, frames);
		}
	}

	InitIterator(&i, 1);
	if ((global.verbose & SHOW_TOC) != 0 &&
		i.hasNextTrack(&i)) {
		TOC *o = i.getNextTrack(&i);

		for (; i.hasNextTrack(&i); ) {
			TOC *p = i.getNextTrack(&i);

			if (GETTRACK(o) <= MAXTRK) {
				unsigned char brace1, brace2;
				unsigned trackbeg;
				trackbeg = have_multisession && IS__DATA(o) ?
					have_multisession : GETSTART(o);

				dw = (unsigned long) (GETSTART(p) - trackbeg);
				mins   =  dw / (60*75);
				secnds = (dw % (60*75)) / 75;
				frames = (dw % 75);

				if (IS__DATA(o)) {
					/*
					 * data track display
					 */
					brace1 = '[';
					brace2 = ']';
				} else if (have_multisession &&
					    GETTRACK(o) == LastAudioTrack()) {
					/*
					 * corrected length of
					 * last audio track in cd extra
					 */
					brace1 = '|';
					brace2 = '|';
				} else {
					/*
					 * audio track display
					 */
					brace1 = '(';
					brace2 = ')';
				}
				fprintf(outfp,
					" %2u.%c%2u:%02u.%02u%c",
					GETTRACK(o),
					brace1,
					mins, secnds, frames,
					brace2);
				ii++;

				if (ii % 5 == 0)
					fputs(",\n", outfp);
				else if (ii != cdtracks)
					fputc(',', outfp);
			}
			o = p;
		} /* for */
		if ((ii % 5) != 0)
			fputs("\n", outfp);
	} /* if */

	if ((global.verbose & SHOW_STARTPOSITIONS) != 0) {
		fputs(_("\nTable of Contents: starting sectors\n"), outfp);

		ii = 0;
		InitIterator(&i, 1);
		if (i.hasNextTrack(&i)) {
			TOC *o = i.getNextTrack(&i);
			for (; i.hasNextTrack(&i); ) {
				TOC *p = i.getNextTrack(&i);
				fprintf(outfp,
					" %2u.(%8u)",
					GETTRACK(o),
					have_multisession &&
					GETTRACK(o) == FirstDataTrack()
					? have_multisession
					: GETSTART(o)
#ifdef DEBUG_CDDB
					+150)
#else
					+0);
#endif

				ii++;
				if ((ii) % 5 == 0)
					fputs(",\n", outfp);
				else
					fputc(',', outfp);
				o = p;
			}
			fprintf(outfp, _(" lead-out(%8u)"), GETSTART(o));
			fputs("\n", outfp);
		}
	}
	if (global.quiet == 0) {
		fprintf(outfp, _("CDINDEX discid: %s\n"), global.cdindex_id);
		fprintf(outfp, _("CDDB discid: 0x%08lx"),
				(unsigned long) global.cddb_id);

		if (have_CDDB != 0) {
			fprintf(outfp, _(" CDDBP titles: resolved\n"));
		} else {
			fprintf(outfp, "\n");
		}
		if (have_CD_text != 0) {
			fprintf(outfp, _("CD-Text: detected\n"));
		} else {
			fprintf(outfp, _("CD-Text: not detected\n"));
		}
		if (have_CD_extra != 0) {
			fprintf(outfp, _("CD-Extra: detected\n"));
		} else {
			fprintf(outfp, _("CD-Extra: not detected\n"));
		}
	}
	if ((global.verbose & SHOW_TITLES) != 0) {
		unsigned int maxlen = 0;

		if (global.disctitle != NULL) {
			fprintf(outfp, _("Album title: '%s'"), global.disctitle);
			if (global.performer != NULL) {
				fprintf(outfp, _("\t[from %s]"), global.performer);
			}
			fputs("\n", outfp);
		}

		InitIterator(&i, 1);
		for (; i.hasNextTrack(&i); ) {
			TOC *p = i.getNextTrack(&i);
			unsigned int jj = GETTRACK(p);

			if (global.tracktitle[jj] != NULL) {
				unsigned int len;

				len = strlen((char *)global.tracktitle[jj]);
				maxlen = max(maxlen, len);
			}
		}
		maxlen = (maxlen + 12 + 8 + 7)/8;

		InitIterator(&i, 1);
		for (; i.hasNextTrack(&i); ) {
			TOC *p = i.getNextTrack(&i);
			unsigned int jj;

			if (IS__DATA(p))
				continue;

			jj = GETTRACK(p);

			if (jj == CDROM_LEADOUT)
				break;

			if (maxlen != 3) {
				if (global.tracktitle[jj] != NULL) {
					fprintf(outfp, _("Track %2u: '%s'"),
						jj, global.tracktitle[jj]);
				} else {
					fprintf(outfp, _("Track %2u: '%s'"),
						jj, "");
				}
				if (global.trackperformer[jj] != NULL &&
#if 1
				    global.trackperformer[jj][0] != '\0' &&
				    (global.performer == NULL ||
				    0 != strcmp((char *)global.performer, (char *)global.trackperformer[jj]))) {
#else
				    global.trackperformer[jj][0] != '\0') {

#endif
					int	j;
					char	*o = global.tracktitle[jj] != NULL
						? (char *)global.tracktitle[jj]
						: "";

					for (j = 0;
					    j < (maxlen - ((int)strlen(o) + 12)/8);
									j++) {
						fprintf(outfp, "\t");
					}
					fprintf(outfp,
						_("[from %s]"),
						global.trackperformer[jj]);
				}
				fputs("\n", outfp);
			}
		}
	}
}

void
DisplayToc()
{
	unsigned long	dw;

	/*
	 * special handling of pseudo-red-book-audio cds
	 */
	if (cdtracks > 1 &&
	    Get_StartSector(CDROM_LEADOUT) < Get_StartSector(cdtracks)) {
		global.illleadout_cd = 1;
		can_read_illleadout();
	}


	/*
	 * get total time
	 */
	if (global.illleadout_cd == 0)
		dw = (unsigned long) Get_StartSector(CDROM_LEADOUT) - Get_StartSector(1);
	else
		dw = (unsigned long) Get_StartSector(cdtracks) - Get_StartSector(1);

	if (global.gui == 0) {
		/*
		 * table formatting when in cmdline mode
		 */
		DisplayToc_no_gui(dw);
	} else if (global.gui == 1) {
		/*
		 * line formatting when in gui mode
		 */
		DisplayToc_with_gui(dw);
	}

	if (global.illleadout_cd != 0) {
		if (global.quiet == 0) {
			errmsgno(EX_BAD, _("CD with illegal leadout position detected!\n"));
		}

		if (global.reads_illleadout == 0) {
			/*
			 * limit accessible tracks
			 * to lowered leadout position
			 */
			restrict_tracks_illleadout();

			if (global.quiet == 0) {
				errmsgno(EX_BAD,
				_("The cdrom drive firmware does not permit access beyond the leadout position!\n"));
			}
			if (global.verbose & (SHOW_ISRC | SHOW_INDICES)) {
				global.verbose &= ~(SHOW_ISRC | SHOW_INDICES);
				fprintf(outfp, _("Switching index scan and ISRC scan off!\n"));
			}

			if (global.quiet == 0) {
				fprintf(outfp,
				_("Audio extraction will be limited to track %ld with maximal %ld sectors...\n"),
					LastTrack(),
					Get_EndSector(LastTrack())+1);
			}
		} else {
			/*
			 * The cdrom drive can read beyond the
			 * indicated leadout. We patch a new leadout
			 * position to the maximum:
			 *   99 minutes, 59 seconds, 74 frames
			 */
			patch_real_end(150 + (99*60+59)*75 + 74);
			if (global.quiet == 0) {
				fprintf(outfp,
				_("Restrictions apply, since the size of the last track is unknown!\n"));
			}
		}
	}
}

LOCAL void	Read_MCN_toshiba	__PR((subq_chnl **sub_ch));

LOCAL void
Read_MCN_toshiba(sub_ch)
	subq_chnl	**sub_ch;
{
	if (Toshiba3401() != 0 && global.quiet == 0 &&
	    ((*sub_ch) != 0 &&
	    (((subq_catalog *)(*sub_ch)->data)->mc_valid & 0x80))) {
		/*
		 * no valid MCN yet. do more searching
		 */
		long	h = Get_AudioStartSector(1);

		while (h <= Get_AudioStartSector(1) + 100) {
			if (Toshiba3401())
				ReadCdRom(get_scsi_p(), RB_BASE->data, h, global.nsectors);
			(*sub_ch) = ReadSubQ(get_scsi_p(), GET_CATALOGNUMBER, 0);
			if ((*sub_ch) != NULL) {
				subq_catalog *subq_cat;

				subq_cat = (subq_catalog *) (*sub_ch)->data;
				if ((subq_cat->mc_valid & 0x80) != 0) {
					break;
				}
			}
			h += global.nsectors;
		}
	}
}

LOCAL void	Get_Set_MCN	__PR((void));

LOCAL void
Get_Set_MCN()
{
	subq_chnl	*sub_ch;
	subq_catalog	*subq_cat = NULL;

	fprintf(outfp, _("scanning for MCN..."));
	fflush(outfp);

	sub_ch = ReadSubQ(get_scsi_p(), GET_CATALOGNUMBER, 0);

#define	EXPLICIT_READ_MCN_ISRC	1
#if EXPLICIT_READ_MCN_ISRC == 1 /* TOSHIBA HACK */
	Read_MCN_toshiba(&sub_ch);
#endif

	if (sub_ch != NULL)
		subq_cat = (subq_catalog *)sub_ch->data;

	if (sub_ch != NULL &&
	    (subq_cat->mc_valid & 0x80) != 0 &&
	    global.quiet == 0) {

		/*
		 * unified format guesser:
		 * format MCN all digits in bcd
		 *    01				  13
		 * A: ab cd ef gh ij kl m0  0  0  0  0  0  0  Plextor 6x Rel. 1.02
		 * B: 0a 0b 0c 0d 0e 0f 0g 0h 0i 0j 0k 0l 0m  Toshiba 3401
		 * C: AS AS AS AS AS AS AS AS AS AS AS AS AS  ASCII SCSI-2 Plextor 4.5x and 6x Rel. 1.06
		 */
		unsigned char *cp = subq_cat->media_catalog_number;
		if (!(cp[8] | cp[9] | cp[10] | cp[11] | cp[12]) &&
		    ((cp[0] & 0xf0) | (cp[1] & 0xf0)
			| (cp[2] & 0xf0) | (cp[3] & 0xf0)
			| (cp[4] & 0xf0) | (cp[5] & 0xf0)
			| (cp[6] & 0xf0))) {
			/* reformat A: to B: */
			cp[12] = cp[6] >> 4; cp[11] = cp[5] & 0xf;
			cp[10] = cp[5] >> 4; cp[ 9] = cp[4] & 0xf;
			cp[ 8] = cp[4] >> 4; cp[ 7] = cp[3] & 0xf;
			cp[ 6] = cp[3] >> 4; cp[ 5] = cp[2] & 0xf;
			cp[ 4] = cp[2] >> 4; cp[ 3] = cp[1] & 0xf;
			cp[ 2] = cp[1] >> 4; cp[ 1] = cp[0] & 0xf;
			cp[ 0] = cp[0] >> 4;
		}

		if (!isdigit(cp[0]) &&
		    (memcmp(subq_cat->media_catalog_number,
				"\0\0\0\0\0\0\0\0\0\0\0\0\0", 13) != 0)) {
			sprintf((char *)
				subq_cat->media_catalog_number,
				"%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X%1.1X",
				subq_cat->media_catalog_number [0],
				subq_cat->media_catalog_number [1],
				subq_cat->media_catalog_number [2],
				subq_cat->media_catalog_number [3],
				subq_cat->media_catalog_number [4],
				subq_cat->media_catalog_number [5],
				subq_cat->media_catalog_number [6],
				subq_cat->media_catalog_number [7],
				subq_cat->media_catalog_number [8],
				subq_cat->media_catalog_number [9],
				subq_cat->media_catalog_number [10],
				subq_cat->media_catalog_number [11],
				subq_cat->media_catalog_number [12]);
		}

		if (memcmp(subq_cat->media_catalog_number, "0000000000000", 13)
		    != 0) {
			Set_MCN(subq_cat->media_catalog_number);
		}
	}
}


LOCAL void	Read_ISRC_toshiba __PR((subq_chnl **sub_ch, unsigned tr));

LOCAL void
Read_ISRC_toshiba(sub_ch, tr)
	subq_chnl	**sub_ch;
	unsigned	tr;
{
	if (Toshiba3401() != 0) {
		int j;
		j = (Get_AudioStartSector(tr)/100 + 1) * 100;
		do {
			ReadCdRom(get_scsi_p(), RB_BASE->data, j,
							global.nsectors);
			*sub_ch = ReadSubQ(get_scsi_p(), GET_TRACK_ISRC,
							Get_Tracknumber(tr));
			if (*sub_ch != NULL) {
				subq_track_isrc * subq_tr;

				subq_tr = (subq_track_isrc *) (*sub_ch)->data;
				if (subq_tr != NULL && (subq_tr->tc_valid & 0x80) != 0)
					break;
			}
			j += global.nsectors;
		} while (j < (Get_AudioStartSector(tr)/100 + 1) * 100 + 100);
	}
}


LOCAL void	Get_Set_ISRC	__PR((unsigned tr));

LOCAL void
Get_Set_ISRC(tr)
	unsigned	tr;
{
	subq_chnl	*sub_ch;
	subq_track_isrc	*subq_tr;

	fprintf(outfp, _("\rscanning for ISRCs: %u ..."), tr);
	fflush(outfp);

	subq_tr = NULL;
	sub_ch = ReadSubQ(get_scsi_p(), GET_TRACK_ISRC, tr);

#if EXPLICIT_READ_MCN_ISRC == 1 /* TOSHIBA HACK */
	Read_ISRC_toshiba(&sub_ch, tr);
#endif

	if (sub_ch != NULL)
		subq_tr = (subq_track_isrc *)sub_ch->data;

	if (sub_ch != NULL && (subq_tr->tc_valid & 0x80) &&
	    global.quiet == 0) {
		unsigned char	p_start[16];
		unsigned char	*p = p_start;
		unsigned char	*cp = subq_tr->track_isrc;

		/*
		 * unified format guesser:
		 * there are 60 bits and 15 bytes available.
		 * 5 * 6bit-items + two zero fill bits + 7 * 4bit-items
		 *
		 * A: ab cd ef gh ij kl mn o0 0  0  0  0  0  0  0  Plextor 6x Rel. 1.02
		 * B: 0a 0b 0c 0d 0e 0f 0g 0h 0i 0j 0k 0l 0m 0n 0o Toshiba 3401
		 * C: AS AS AS AS AS AS AS AS AS AS AS AS AS AS AS ASCII SCSI-2
		 * eg 'G''B''-''A''0''7''-''6''8''-''0''0''2''7''0' makes most sense
		 * D: 'G''B''A''0''7''6''8''0''0''2''7''0'0  0  0  Plextor 6x Rel. 1.06 and 4.5x R. 1.01 and 1.04
		 */

		/* Check for format A: */
		if (!(cp[8] | cp[9] | cp[10] | cp[11] | cp[12] | cp[13] | cp[14]) &&
		    ((cp[0] & 0xf0) | (cp[1] & 0xf0) | (cp[2] & 0xf0) |
		    (cp[3]  & 0xf0) | (cp[4] & 0xf0) | (cp[5] & 0xf0) |
		    (cp[6]  & 0xf0) | (cp[7] & 0xf0))) {
#if DEBUG_ISRC
			fprintf(outfp, "a!\t");
#endif
			/* reformat A: to B: */
			cp[14] = cp[7] >> 4; cp[13] = cp[6] & 0xf;
			cp[12] = cp[6] >> 4; cp[11] = cp[5] & 0xf;
			cp[10] = cp[5] >> 4; cp[ 9] = cp[4] & 0xf;
			cp[ 8] = cp[4] >> 4; cp[ 7] = cp[3] & 0xf;
			cp[ 6] = cp[3] >> 4; cp[ 5] = cp[2] & 0xf;
			cp[ 4] = cp[2] >> 4; cp[ 3] = cp[1] & 0xf;
			cp[ 2] = cp[1] >> 4; cp[ 1] = cp[0] & 0xf;
			cp[ 0] = cp[0] >> 4;
#if DEBUG_ISRC
			fprintf(outfp, "a->b: %15.15s\n", cp);
#endif
		}

		/*
		 * Check for format B:
		 * If not yet in ASCII format, do the conversion
		 */
		if (cp[0] < '0' && cp[1] < '0') {
			/*
			 * coding table for International Standard
			 * Recording Code
			 */
			/* BEGIN CSTYLED */
			static char	bin2ISRC[] = {
			'0','1','2','3','4','5','6','7','8','9',	/* 10 */
			':',';','<','=','>','?','@',			/* 17 */
			'A','B','C','D','E','F','G','H','I','J','K',	/* 28 */
			'L','M','N','O','P','Q','R','S','T','U','V',	/* 39 */
			'W','X','Y','Z',				/* 43 */
#if 1
			'[','\\',']','^','_','`',			/* 49 */
			'a','b','c','d','e','f','g','h','i','j','k',	/* 60 */
			'l','m','n','o'					/* 64 */
#endif
			};
			/* END CSTYLED */

			/*
			 * build 6-bit vector of coded values
			 */
			unsigned	ind;
			int	bits;

#if DEBUG_ISRC
			fprintf(outfp, "b!\n");
#endif
			ind =   (cp[0] << 26) +
				(cp[1] << 22) +
				(cp[2] << 18) +
				(cp[3] << 14) +
				(cp[4] << 10) +
				(cp[5] << 6) +
				(cp[6] << 2) +
				(cp[7] >> 2);

			if ((cp[7] & 3) == 3) {
				if (global.verbose) {
					fprintf(outfp,
						_("Recorder-ID encountered: "));
					for (bits = 0; bits < 30; bits += 6) {
						unsigned binval = (ind & (ULONG_C(0x3f) << (24-bits)))
											>> (24-bits);
						if ((binval < sizeof (bin2ISRC)) &&
						    (binval <= 9 || binval >= 16)) {
							fprintf(outfp,
							"%X",
							bin2ISRC[binval]);
						}
					}

					fprintf(outfp,
					    "%.1X%.1X%.1X%.1X%.1X%.1X%.1X",
					    subq_tr->track_isrc [8] & 0x0f,
					    subq_tr->track_isrc [9] & 0x0f,
					    subq_tr->track_isrc [10] & 0x0f,
					    subq_tr->track_isrc [11] & 0x0f,
					    subq_tr->track_isrc [12] & 0x0f,
					    subq_tr->track_isrc [13] & 0x0f,
					    subq_tr->track_isrc [14] & 0x0f);
					fprintf(outfp, "\n");
				}
				return;
			}
			if ((cp[7] & 3) != 0) {
				fprintf(outfp,
				_("unknown mode 3 entry C1=0x%02x, C2=0x%02x\n"),
					(cp[7] >> 1) & 1, cp[7] & 1);
				return;
			}

			/*
			 * decode ISRC due to IEC 908
			 */
			for (bits = 0; bits < 30; bits += 6) {
				unsigned binval = (ind & ((unsigned long) 0x3fL << (24L-bits))) >> (24L-bits);
				if ((binval >= sizeof (bin2ISRC)) ||
				    (binval > 9 && binval < 16)) {
					/*
					 * Illegal ISRC, dump and skip
					 */
					int	y;

					Get_ISRC(tr)[0] = '\0';
					fprintf(outfp,
					_("\nIllegal ISRC for track %u, skipped: "),
						tr);
					for (y = 0; y < 15; y++) {
						fprintf(outfp, "%02x ",
								cp[y]);
					}
					fputs("\n", outfp);
					return;
				}
				*p++ = bin2ISRC[binval];

				/*
				 * insert a dash after two country
				 * characters for legibility
				 */
				if (bits == 6)
					*p++ = '-';
			}

			/*
			 * format year and serial number
			 */
			sprintf((char *)p, "-%.1X%.1X-%.1X%.1X%.1X%.1X%.1X",
				subq_tr->track_isrc [8] & 0x0f,
				subq_tr->track_isrc [9] & 0x0f,
				subq_tr->track_isrc [10] & 0x0f,
				subq_tr->track_isrc [11] & 0x0f,
				subq_tr->track_isrc [12] & 0x0f,
				subq_tr->track_isrc [13] & 0x0f,
				subq_tr->track_isrc [14] & 0x0f);
#if DEBUG_ISRC
			fprintf(outfp, "b: %15.15s!\n", p_start);
#endif
		} else {
			/*
			 * It might be in ASCII, surprise
			 */
			int ii;
			for (ii = 0; ii < 12; ii++) {
				if (cp[ii] < '0' || cp[ii] > 'Z') {
					break;
				}
			}
			if (ii != 12) {
				int y;

				Get_ISRC(ii)[0] = '\0';
				fprintf(outfp, _("\nIllegal ISRC for track %d, skipped: "), ii+1);
				for (y = 0; y < 15; y++) {
					fprintf(outfp, "%02x ", cp[y]);
				}
				fputs("\n", outfp);
				return;
			}

#if DEBUG_ISRC
			fprintf(outfp, "ascii: %15.15s!\n", cp);
#endif
			for (ii = 0; ii < 12; ii++) {
#if 1
				if ((ii == 2 || ii == 5 || ii == 7) &&
				    cp[ii] != ' ')
					*p++ = '-';
#endif
				*p++ = cp[ii];
			}
			if (p - p_start >= 16)
				*(p_start + 15) = '\0';
			else
				*p = '\0';
		}

		if (memcmp(p_start, "00-000-00-00000", 15) != 0) {
			Set_ISRC(tr, p_start);
		}
	}
}

/*
 * get and display Media Catalog Number (one per disc)
 *  and Track International Standard Recording Codes (for each track)
 */
void
Read_MCN_ISRC(startTrack, endTrack)
	unsigned	startTrack;
	unsigned	endTrack;
{
	int	old_hidden = have_hiddenAudioTrack;

	have_hiddenAudioTrack = 0;		/* Don'tcheck track #0 here */

	if ((global.verbose & SHOW_MCN) != 0) {

		if (Get_MCN()[0] == '\0') {
			Get_Set_MCN();
		}

		if (Get_MCN()[0] != '\0') {
			fprintf(outfp,
				_("\rMedia catalog number: %13.13s\n"),
				Get_MCN());
		} else {
			fprintf(outfp,
				_("\rNo media catalog number present.\n"));
		}
	}



	if ((global.verbose & SHOW_ISRC) != 0) {
		struct iterator i;

		InitIterator(&i, 1);

		while (i.hasNextTrack(&i)) {
			struct TOC *p = i.getNextTrack(&i);
			unsigned ii = GETTRACK(p);

			if (ii == CDROM_LEADOUT)
				break;

			if (ii < startTrack || ii > endTrack)
				continue;

			if (!IS__AUDIO(p))
				continue;

			if (GETISRC(p)[0] == '\0') {
				Get_Set_ISRC(ii);
			}

			if (GETISRC(p)[0] != '\0') {
				fprintf(outfp,
					"\rT: %2u ISRC: %15.15s\n",
					ii, GETISRC(p));
				fflush(outfp);
			}
		} /* for all tracks */

		fputs("\n", outfp);
	} /* if SHOW_ISRC */

	have_hiddenAudioTrack = old_hidden;	/* Restore old value */
}

LOCAL int playing = 0;

LOCAL subq_chnl *ReadSubChannel __PR((unsigned sec));

LOCAL subq_chnl *
ReadSubChannel(sec)
	unsigned	sec;
{
	subq_chnl	*sub_ch;

	/*
	 * For modern drives implement a direct method. If the drive supports
	 * reading of subchannel data, do direct reads.
	 */
	if (ReadSubChannels != NULL) {
		get_scsi_p()->silent++;
		sub_ch = ReadSubChannels(get_scsi_p(), sec);
		get_scsi_p()->silent--;
		if (sub_ch == NULL /*&& (scg_sense_key(get_scsi_p()) == 5)*/) {
			/*
			 * command is not implemented
			 */
			ReadSubChannels = NULL;
#if	defined DEBUG_SUB
			fprintf(outfp,
			"\nCommand not implemented: switching ReadSubChannels off !\n");
#endif
			goto fallback;
		}

		/*
		 * check the address mode field
		 */
		if ((sub_ch->control_adr & 0x0f) == 0) {
			/*
			 * no Q mode information present at all, weird
			 */
			sub_ch->control_adr = 0xAA;
		}

		if ((int)(sub_ch->control_adr & 0x0f) > 0x01) {
			/*
			 * this sector just has no position information.
			 * we try the one before and then the one after.
			 */
			if (sec > 1) {
				sec -= 1;
				sub_ch = ReadSubChannels(get_scsi_p(), sec);
				if (sub_ch == NULL)
					return (NULL);
				sec += 1;
			}
			if ((sub_ch->control_adr & 0x0f) != 0x01) {
				sec += 2;
				sub_ch = ReadSubChannels(get_scsi_p(), sec);
				if (sub_ch == NULL)
					return (NULL);
				sec -= 2;
			}
		}

		/*
		 * check address mode field for position information
		 */
		if ((sub_ch->control_adr & 0x0f) == 0x01) {
			return (sub_ch);
		}
		ReadSubChannels = NULL;
		fprintf(outfp,
		_("\nCould not get position information (%02x) for sectors %u, %u, %u: switching ReadSubChannels off !\n"),
		sub_ch->control_adr &0x0f, sec-1, sec, sec+2);
	}

	/*
	 * We rely on audio sectors here!!!
	 * The only method that worked even with my antique Toshiba 3401,
	 * is playing the sector and then request the subchannel afterwards.
	 */
fallback:
	/*
	 * We need a conformed audio track here!
	 *
	 * Fallback to ancient method
	 */
	if (-1 == Play_at(get_scsi_p(), sec, 1)) {
		return (NULL);
	}
	playing = 1;
	sub_ch = ReadSubQ(get_scsi_p(), GET_POSITIONDATA, 0);
	return (sub_ch);
}

LOCAL int	ReadSubControl	__PR((unsigned sec));
LOCAL int
ReadSubControl(sec)
	unsigned	sec;
{
	subq_chnl *sub_ch = ReadSubChannel(sec);
	if (sub_ch == NULL)
		return (-1);

	return (sub_ch->control_adr & 0xf0);
}

LOCAL int	HaveSCMS	__PR((unsigned StartSector));
LOCAL int
HaveSCMS(StartSector)
	unsigned	StartSector;
{
	int	i;
	int	copy_bits_set = 0;

	for (i = 0; i < 8; i++) {
		int	cr;

		cr = ReadSubControl(StartSector + i);
		if (cr == -1)
			continue;
		(cr & 0x20) ? copy_bits_set++ : 0;
	}
	return (copy_bits_set >= 1 && copy_bits_set < 8);
}

void
Check_Toc()
{
	/*
	 * detect layout
	 * detect tracks
	 */
}

LOCAL int
GetIndexOfSector(sec, track)
	unsigned	sec;
	unsigned	track;
{
	subq_chnl	*sub_ch = ReadSubChannel(sec);

	if (sub_ch == NULL) {
		if ((long)sec == Get_EndSector(track)) {
			errmsgno(EX_BAD,
			_("Driver and/or firmware bug detected! Drive cannot play the very last sector (%u)!\n"),
			sec);
		}
		return (-1);
	}

	/*
	 * can we trust that these values are hex and NOT bcd?
	 */
	if ((sub_ch->track >= 0x10) && (sub_ch->track - track > 5)) {
		/* change all values from bcd to hex */
		sub_ch->track = (sub_ch->track >> 4)*10 + (sub_ch->track & 0x0f);
		sub_ch->index = (sub_ch->index >> 4)*10 + (sub_ch->index & 0x0f);
	}

#if 1
	/*
	 * compare tracks
	 */
	if (sub_ch->index != 0 && track != sub_ch->track) {
		if (global.verbose)
			fprintf(outfp,
			_("\ntrack mismatch: %1u, in-track subchannel: %1u (index %1u, sector %1u)\n"),
			track, sub_ch->track, sub_ch->index, sec);
	}
#endif

	/*
	 * compare control field with the one from the TOC
	 */
	if ((Get_Flags(track) & 0xf0) != (sub_ch->control_adr & 0xf0)) {
		int	diffbits = (Get_Flags(track) & 0xf0) ^ (sub_ch->control_adr & 0xf0);

		if ((diffbits & 0x80) == 0x80) {
			/*
			 * broadcast difference
			 */
			if (global.verbose) {
				fprintf(outfp,
				_("broadcast type conflict detected -> TOC:%s, subchannel:%s\n"),
				(sub_ch->control_adr & 0x80) == 0 ? _("broadcast") : _("nonbroadcast"),
				(sub_ch->control_adr & 0x80) != 0 ? _("broadcast") : _("nonbroadcast"));
			}
		}
		if ((diffbits & 0x40) == 0x40) {
			/*
			 * track type difference
			 */
			if (global.verbose) {
				fprintf(outfp,
				_("track type conflict detected -> TOC:%s, subchannel:%s\n"),
				(sub_ch->control_adr & 0x40) == 0 ? _("data") : _("audio"),
				(sub_ch->control_adr & 0x40) != 0 ? _("data") : _("audio"));
			}
		}
		if ((diffbits & 0x20) == 0x20 && !Get_SCMS(track)) {
			/*
			 * copy permission difference is a sign for SCMS
			 * and is treated elsewhere.
			 */
			if (global.verbose) {
				fprintf(outfp,
				_("difference: TOC:%s, subchannel:%s\ncorrecting TOC...\n"),
				(sub_ch->control_adr & 0x20) == 0 ? _("unprotected") : _("copyright protected"),
				(sub_ch->control_adr & 0x20) != 0 ? _("unprotected") : _("copyright protected"));
			}

			toc_entry(track,
			    (Get_Flags(track) & 0xDF) | (sub_ch->control_adr & 0x20),
			    Get_Tracknumber(track),
			    Get_ISRC(track),
			    Get_AudioStartSector(track),
			    Get_Mins(track),
			    Get_Secs(track),
			    Get_Frames(track));
		}
		if ((diffbits & 0x10) == 0x10) {
			/*
			 * preemphasis difference
			 */
			if (global.verbose)
				fprintf(outfp,
				_("difference: TOC:%s, subchannel:%s preemphasis\ncorrecting TOC...\n"),
				(sub_ch->control_adr & 0x10) == 0 ? _("with") : _("without"),
				(sub_ch->control_adr & 0x10) != 0 ? _("with") : _("without"));

			toc_entry(track,
			    (Get_Flags(track) & 0xEF) | (sub_ch->control_adr & 0x10),
			    Get_Tracknumber(track),
			    Get_ISRC(track),
			    Get_AudioStartSector(track),
			    Get_Mins(track),
			    Get_Secs(track),
			    Get_Frames(track));
		}

	}

	return (sub_ch ? sub_ch->index == 244 ? 1 : sub_ch->index : -1);
}

LOCAL int ScanBackwardFrom	__PR((unsigned sec, unsigned limit,
						int *where, unsigned track));

LOCAL int
ScanBackwardFrom(sec, limit, where, track)
	unsigned	sec;
	unsigned	limit;
	int		*where;
	unsigned	track;
{
	unsigned	lastindex = 0;
	unsigned	mysec = sec;

	/*
	 * try to find the transition of index n to index 0,
	 * if the track ends with an index 0.
	 */
	while ((lastindex = GetIndexOfSector(mysec, track)) == 0) {
		if (mysec < limit+75) {
			break;
		}
		mysec -= 75;
	}
	if (mysec == sec) {
		/*
		 * there is no pre-gap in this track
		 */
		if (where != NULL)
			*where = -1;
	} else {
		/*
		 * we have a pre-gap in this track
		 */
		if (lastindex == 0) {
			/*
			 * we did not cross the transition yet -> search
			 * backward
			 */
			do {
				if (mysec < limit+1) {
					break;
				}
				mysec --;
			} while ((lastindex = GetIndexOfSector(mysec, track)) == 0);
			if (lastindex != 0) {
				/*
				 * successful
				 */
				mysec ++;
				/*
				 * register mysec as transition
				 */
				if (where != NULL)
					*where = (int) mysec;
			} else {
				/*
				 * could not find transition
				 */
				if (!global.quiet)
					errmsgno(EX_BAD,
					_("Could not find index transition for pre-gap.\n"));
				if (where != NULL)
					*where = -1;
			}
		} else {
			int myindex = -1;
			/*
			 * we have crossed the transition -> search forward
			 */
			do {
				if (mysec >= sec) {
					break;
				}
				mysec ++;
			} while ((myindex = GetIndexOfSector(mysec, track)) != 0);
			if (myindex == 0) {
				/*
				 * successful
				 * register mysec as transition
				 */
				if (where != NULL)
					*where = (int) mysec;
			} else {
				/*
				 * could not find transition
				 */
				if (!global.quiet)
					errmsgno(EX_BAD,
					_("Could not find index transition for pre-gap.\n"));
				if (where != NULL)
					*where = -1;
			}
		}
	}
	return (lastindex);
}

#ifdef	USE_LINEAR_SEARCH
LOCAL int	linear_search	__PR((int searchInd, unsigned int Start,
					unsigned int End, unsigned track));
LOCAL int
linear_search(searchInd, Start, End, track)
	int		searchInd;
	unsigned	Start;
	unsigned	End;
	unsigned	track;
{
	int	l = Start;
	int	r = End;

	for (; l <= r; l++) {
		int	ind;

		ind = GetIndexOfSector(l, track);
		if (searchInd == ind) {
			break;
		}
	}
	if (l <= r) {
		/*
		 * Index found.
		 */
		return (l);
	}
	return (-1);
}
#endif

#ifndef	USE_LINEAR_SEARCH
#undef DEBUG_BINSEARCH
LOCAL int binary_search	__PR((int searchInd, unsigned int Start,
					unsigned int End, unsigned track));
LOCAL int
binary_search(searchInd, Start, End, track)
	int		searchInd;
	unsigned	Start;
	unsigned	End;
	unsigned	track;
{
	int	l = Start;
	int	r = End;
	int	x = 0;
	int	ind;

	while (l <= r) {
		x = (l + r) / 2;
		/*
		 * try to avoid seeking
		 */
		ind = GetIndexOfSector(x, track);
		if (searchInd == ind) {
			break;
		} else {
			if (searchInd < ind)
				r = x - 1;
			else
				l = x + 1;
		}
	}
#ifdef DEBUG_BINSEARCH
	fprintf(outfp, "(%d,%d,%d > ", l, x, r);
#endif
	if (l <= r) {
		/*
		 * Index found. Now find the first position of this index
		 *
		 * l=LastPos	x=found		r=NextPos
		 */
		r = x;
		while (l < r-1) {
			x = (l + r) / 2;
			/*
			 * try to avoid seeking
			 */
			ind = GetIndexOfSector(x, track);
			if (searchInd == ind) {
				r = x;
			} else {
				l = x;
			}
#ifdef DEBUG_BINSEARCH
			fprintf(outfp, "%d -> ", x);
#endif
		}
#ifdef DEBUG_BINSEARCH
		fprintf(outfp, "%d,%d)\n", l, r);
#endif
		if (searchInd == GetIndexOfSector(l, track))
			return (l);
		else
			return (r);
	}
	return (-1);
}
#endif


LOCAL void	register_index_position	__PR((int IndexOffset,
					index_list **last_index_entry));

LOCAL void
register_index_position(IndexOffset, last_index_entry)
	int		IndexOffset;
	index_list	**last_index_entry;
{
	index_list	*indexentry;

	/*
	 * register higher index entries
	 */
	if (*last_index_entry != NULL) {
		indexentry = (index_list *) malloc(sizeof (index_list));
	} else {
		indexentry = NULL;
	}
	if (indexentry != NULL) {
		indexentry->next = NULL;
		(*last_index_entry)->next = indexentry;
		*last_index_entry = indexentry;
		indexentry->frameoffset = IndexOffset;
#if defined INFOFILES
	} else {
		fprintf(outfp,
		_("No memory for index lists. Index positions\nwill not be written in info file!\n"));
#endif
	}
}

LOCAL void	Set_SCMS	__PR((unsigned long p_track));

#undef DEBUG_INDLIST
/*
 * experimental code
 * search for indices (audio mode required)
 */
unsigned
ScanIndices(track, cd_index, bulk)
	unsigned	track;
	unsigned	cd_index;
	int		bulk;
{
	/*
	 * scan for indices.
	 * look at last sector of track.
	 * when the index is not equal 1 scan by bipartition
	 * for offsets of all indices
	 */
	unsigned	starttrack;
	unsigned	endtrack;
	unsigned	startindex;
	unsigned	endindex;

	unsigned	j;
	int		LastIndex = 0;
	int		n_0_transition;
	unsigned	StartSector;
	unsigned	retval = 0;

	index_list *baseindex_pool;
	index_list *last_index_entry;

	SCSI *scgp = get_scsi_p();

	int		old_hidden = have_hiddenAudioTrack;

	struct iterator	i;

	have_hiddenAudioTrack = 0;		/* Don'tcheck track #0 here */
	InitIterator(&i, 1);

	EnableCdda(scgp, 0, 0);
	EnableCdda(scgp, 1, CD_FRAMESIZE_RAW + 16);

	if (!global.quiet && !(global.verbose & SHOW_INDICES))
		fprintf(outfp, _("seeking index start ..."));

	starttrack = track;
	endtrack = global.endtrack;

	baseindex_pool = (index_list *) malloc(sizeof (index_list) * (endtrack - starttrack + 1));
#ifdef DEBUG_INDLIST
	fprintf(outfp, "index0-mem-pool %p\n", baseindex_pool);
#endif


	while (i.hasNextTrack(&i)) {
		struct TOC	*p = i.getNextTrack(&i);
		unsigned	ii = GETTRACK(p);

		if (ii < starttrack || IS__DATA(p))
			continue;	/* skip nonaudio tracks */

		if (ii > endtrack)
			break;

		if (global.verbose & SHOW_INDICES) {
			if (global.illleadout_cd && global.reads_illleadout &&
			    ii == endtrack) {
				fprintf(outfp,
				_("Analysis of track %u skipped due to unknown length\n"), ii);
			}
		}
		if (global.illleadout_cd && global.reads_illleadout &&
		    ii == endtrack)
			continue;

		StartSector = Get_AudioStartSector(ii);
		if (HaveSCMS(StartSector)) {
			Set_SCMS(ii);
		}
		if (global.verbose & SHOW_INDICES) {
			fprintf(outfp, _("\rindex scan: %u..."), ii);
			fflush(outfp);
		}
		LastIndex = ScanBackwardFrom(Get_EndSector(ii), StartSector,
							&n_0_transition, ii);
		if (LastIndex > 99)
			continue;

		if (baseindex_pool != NULL) {
#ifdef DEBUG_INDLIST
#endif
			/*
			 * register first index entry for this track
			 */
			baseindex_pool[ii - starttrack].next = NULL;
			baseindex_pool[ii - starttrack].frameoffset = StartSector;
			global.trackindexlist[ii] = &baseindex_pool[ii -
							starttrack];
#ifdef DEBUG_INDLIST
#endif
		} else {
			global.trackindexlist[ii] = NULL;
		}
		last_index_entry = global.trackindexlist[ii];

		if (LastIndex < 2) {
			register_index_position(n_0_transition,
							&last_index_entry);
			continue;
		}

		if ((global.verbose & SHOW_INDICES) && LastIndex > 1) {
			fprintf(outfp,
			_("\rtrack %2u has %d indices, index table (pairs of 'index: frame offset')\n"),
			ii, LastIndex);
		}
		startindex = 0;
		endindex = LastIndex;

		for (j = startindex; j <= endindex; j++) {
			int	IndexOffset;

			/*
			 * this track has indices
			 */

#ifdef	USE_LINEAR_SEARCH
			/*
			 * do a linear search
			 */
			IndexOffset = linear_search(j, StartSector,
							Get_EndSector(ii), ii);
#else
			/*
			 * do a binary search
			 */
			IndexOffset = binary_search(j, StartSector,
							Get_EndSector(ii), ii);
#endif

			if (IndexOffset != -1) {
				StartSector = IndexOffset;
			}

			if (j == 1)
				last_index_entry->frameoffset = IndexOffset;
			else if (j > 1)
				register_index_position(IndexOffset,
							&last_index_entry);

			if (IndexOffset == -1) {
				if (global.verbose & SHOW_INDICES) {
					if (global.gui == 0) {
						fprintf(outfp,
							"%2u: N/A   ", j);
						if (((j + 1) % 8) == 0)
							fputs("\n", outfp);
					} else {
						fprintf(outfp,
						"\rT%02u I%02u N/A\n",
						ii, j);
					}
				}
			} else {
				if (global.verbose & SHOW_INDICES) {
					if (global.gui == 0) {
						fprintf(outfp,
						"%2u:%6lu ",
						j,
						IndexOffset-
						Get_AudioStartSector(ii));
						if (((j + 1) % 8) == 0)
							fputs("\n", outfp);
					} else {
						fprintf(outfp,
						"\rT%02u I%02u %06lu\n",
						ii,
						j,
						IndexOffset-
						Get_AudioStartSector(ii));
					}
				}

				if (track == ii && cd_index == j) {
					retval = IndexOffset-
						    Get_AudioStartSector(ii);
				}
			} /* if IndexOffset */
		} /* for index */
		register_index_position(n_0_transition, &last_index_entry);

		/*
		 * sanity check. clear all consecutive nonindex
		 * entries (frameoffset -1) from the end.
		 */
		{
		index_list	*ip = global.trackindexlist[ii];
		index_list	*iq = NULL;
		index_list	*lastgood = iq;

		while (ip != NULL) {
			if (ip->frameoffset == -1) {
				/*
				 * no index available
				 */
				if (lastgood == NULL) {
					/*
					 * if this is the first one in a
					 * sequence, store predecessor
					 * unless it is the last entry and
					 * there is no index 0 transition
					 */
					if (!(ip->next == NULL &&
					    ip->frameoffset == n_0_transition))
						lastgood = iq;
				}
			} else {
				/*
				 * this is a valid index, reset marker
				 */
				lastgood = NULL;
			}

			iq = ip;
			ip = ip->next;
		}
		/*
		 * terminate chain at the last well defined entry.
		 */
		if (lastgood != NULL)
			lastgood->next = NULL;
		}

		if (global.gui == 0 && (global.verbose & SHOW_INDICES) &&
		    ii != endtrack)
			fputs("\n", outfp);
	} /* for tracks */

	if (global.gui == 0 && (global.verbose & SHOW_INDICES))
		fputs("\n", outfp);
	if (playing != 0)
		StopPlay(get_scsi_p());

	EnableCdda(scgp, 0, 0);
	EnableCdda(scgp, 1, CD_FRAMESIZE_RAW);

	have_hiddenAudioTrack = old_hidden;	/* Restore old value */

	return (retval);
}

LOCAL	unsigned char	MCN[14];

LOCAL void
Set_MCN(MCN_arg)
	unsigned char	*MCN_arg;
{
	memcpy(MCN, MCN_arg, 14);
	MCN[13] = '\0';
}

unsigned char *
Get_MCN()
{
	return (MCN);
}


LOCAL TOC	g_toc[MAXTRK+1]; /* hidden track + 100 regular tracks */

/*#define IS_AUDIO(i) (!(g_toc[i].bFlags & 0x40))*/

int
TOC_entries(tracks, a, b, binvalid)
	unsigned	tracks;
	unsigned char	*a;
	unsigned char	*b;
	int		binvalid;
{
	int	i;

	for (i = 1; i <= (int)tracks; i++) {
		unsigned char *p;

		if (binvalid) {
			unsigned long dwStartSector;

			p = a + 8*(i-1);

			g_toc[i].bFlags = p[1];
			g_toc[i].bTrack = p[2];
			g_toc[i].ISRC[0] = 0;
			dwStartSector = a_to_u_4_byte(p+4);
			g_toc[i].dwStartSector = dwStartSector;
			lba_2_msf((long)dwStartSector,
				&g_toc[i].mins,
				&g_toc[i].secs,
				&g_toc[i].frms);
		} else {
			p = b + 8*(i-1);
			g_toc[i].bFlags = p[1];
			g_toc[i].bTrack = p[2];
			g_toc[i].ISRC[0] = 0;
			if ((int)((p[5]*60 + p[6])*75 + p[7]) >= 150) {
				g_toc[i].dwStartSector = (p[5]*60 + p[6])*75 +
								p[7] -150;
			} else {
				g_toc[i].dwStartSector = 0;
			}
			g_toc[i].mins = p[5];
			g_toc[i].secs = p[6];
			g_toc[i].frms = p[7];
		}
	}
	return (0);
}

void
toc_entry(nr, flag, tr, ISRC, lba, m, s, f)
	unsigned	nr;
	unsigned	flag;
	unsigned	tr;
	unsigned char	*ISRC;
	unsigned long	lba;
	int		m;
	int		s;
	int		f;
{
	if (nr > MAXTRK)
		return;

	g_toc[nr].bFlags = flag;
	g_toc[nr].bTrack = tr;
	if (ISRC) {
		strncpy((char *)g_toc[nr].ISRC, (char *)ISRC,
			sizeof (g_toc[nr].ISRC) -1);
		g_toc[nr].ISRC[sizeof (g_toc[nr].ISRC) -1] = '\0';
	}
	g_toc[nr].dwStartSector = lba;
	g_toc[nr].mins = m;
	g_toc[nr].secs = s;
	g_toc[nr].frms = f;
}

int
patch_real_end(sector)
	unsigned long	sector;
{
	g_toc[cdtracks+1].dwStartSector = sector;
	return (0);
}

LOCAL int
patch_cd_extra(track, sector)
	unsigned	track;
	unsigned long	sector;
{
	if (track <= cdtracks)
		g_toc[track].dwStartSector = sector;
	return (0);
}

LOCAL int
restrict_tracks_illleadout()
{
	struct TOC	*o = &g_toc[cdtracks+1];
	int		i;

	for (i = cdtracks; i >= 0; i--) {
		struct TOC *p = &g_toc[i];
		if (GETSTART(o) > GETSTART(p))
			break;
	}
	patch_cd_extra(i+1, GETSTART(o));
	cdtracks = i;

	return (0);
}

LOCAL void
Set_ISRC(track, ISRC_arg)
	unsigned		track;
	const unsigned char	*ISRC_arg;
{
	if (track <= (int)cdtracks) {
		memcpy(Get_ISRC(track), ISRC_arg, 16);
	}
}


unsigned char *
Get_ISRC(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		return (g_toc[p_track].ISRC);
	return (NULL);
}

LOCAL void
patch_to_audio(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		g_toc[p_track].bFlags &= ~0x40;
}

int
Get_Flags(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		return (g_toc[p_track].bFlags);
	return (-1);
}

int
Get_Mins(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		return (g_toc[p_track].mins);
	return (-1);
}

int
Get_Secs(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		return (g_toc[p_track].secs);
	return (-1);
}

int
Get_Frames(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		return (g_toc[p_track].frms);
	return (-1);
}

int
Get_Preemphasis(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		return (g_toc[p_track].bFlags & 0x10);
	return (-1);
}

LOCAL void
Set_SCMS(p_track)
	unsigned long	p_track;
{
	g_toc[p_track].SCMS = 1;
}

int
Get_SCMS(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		return (g_toc[p_track].SCMS);
	return (-1);
}

int
Get_Copyright(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks) {
		if (g_toc[p_track].SCMS)
			return (1);
		return (((int)g_toc[p_track].bFlags & 0x20) >> 4);
	}
	return (-1);
}

int
Get_Datatrack(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		return (g_toc[p_track].bFlags & 0x40);
	return (-1);
}

int
Get_Channels(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		return (g_toc[p_track].bFlags & 0x80);
	return (-1);
}

int
Get_Tracknumber(p_track)
	unsigned long	p_track;
{
	if (p_track <= cdtracks)
		return (g_toc[p_track].bTrack);
	return (-1);
}

int	useHiddenTrack	__PR((void));

int
useHiddenTrack()
{
	if (global.no_hidden_track)
		return (0);
	return (have_hiddenAudioTrack);
}



LOCAL void	it_reset	__PR((struct iterator *this));

LOCAL void
it_reset(this)
	struct iterator	*this;
{
	this->index = this->startindex;
}


LOCAL int	it_hasNextTrack		__PR((struct iterator *this));
LOCAL struct TOC *it_getNextTrack	__PR((struct iterator *this));

LOCAL int
it_hasNextTrack(this)
	struct iterator	*this;
{
	return (this->index <= (int)cdtracks+1);
}



LOCAL struct TOC *
it_getNextTrack(this)
	struct iterator	*this;
{
	/* if ((*this->hasNextTrack)(this) == 0) return (NULL); */
	if (this->index > (int)cdtracks+1)
		return (NULL);

	return (&g_toc[this->index++]);
}


LOCAL void
InitIterator(iter, p_track)
	struct iterator	*iter;
	unsigned long	p_track;
{
	if (iter == NULL)
		return;

	iter->startindex = useHiddenTrack() ? 0 : p_track;
	iter->reset = it_reset;
	iter->getNextTrack = it_getNextTrack;
	iter->hasNextTrack = it_hasNextTrack;
	iter->reset(iter);
}

#if	0
LOCAL struct iterator *NewIterator __PR((void));

LOCAL struct iterator *
NewIterator()
{
	struct iterator	*retval;

	retval = malloc(sizeof (struct iterator));
	if (retval != NULL) {
		InitIterator(retval, 1);
	}
	return (retval);
}
#endif

long
Get_AudioStartSector(p_track)
	unsigned long	p_track;
{
#if	1
	if (p_track == CDROM_LEADOUT)
		p_track = cdtracks + 1;

	if (p_track <= cdtracks +1 &&
	    IS__AUDIO(&g_toc[p_track]))
		return (GETSTART(&g_toc[p_track]));
#else
	struct iterator	i;

	InitIterator(&i, p_track);

	if (p_track == cdtracks + 1)
		p_track = CDROM_LEADOUT;

	while (i.hasNextTrack(&i)) {
		TOC	*p = i.getNextTrack(&i);

		if (GETTRACK(p) == p_track) {
			if (IS__DATA(p)) {
				return (-1);
			}
			return (GETSTART(p));
		}
	}
#endif
	return (-1);
}


long
Get_StartSector(p_track)
	unsigned long	p_track;
{
#if	1
	if (p_track == CDROM_LEADOUT)
		p_track = cdtracks + 1;

	if (p_track <= cdtracks +1)
		return (GETSTART(&g_toc[p_track]));
#else
	struct iterator	i;

	InitIterator(&i, p_track);

	if (p_track == cdtracks + 1)
		p_track = CDROM_LEADOUT;

	while (i.hasNextTrack(&i)) {
		TOC *p = i.getNextTrack(&i);

		if (GETTRACK(p) == p_track) {
			return (GETSTART(p));
		}
	}
#endif
	return (-1);
}


long
Get_EndSector(p_track)
	unsigned long	p_track;
{
#if	1
	if (p_track <= cdtracks)
		return (GETSTART(&g_toc[p_track+1])-1);
#else
	struct iterator	i;

	InitIterator(&i, p_track);

	if (p_track == cdtracks + 1)
		p_track = CDROM_LEADOUT;

	while (i.hasNextTrack(&i)) {
		TOC *p = i.getNextTrack(&i);

		if (GETTRACK(p) == p_track) {
			p = i.getNextTrack(&i);
			if (p == NULL) {
				return (-1);
			}
			return (GETSTART(p)-1);
		}
	}
#endif
	return (-1);
}

long
FirstTrack()
{
	struct iterator	i;

	InitIterator(&i, 1);

	if (i.hasNextTrack(&i)) {
		return (GETTRACK(i.getNextTrack(&i)));
	}
	return (-1);
}

long
FirstAudioTrack()
{
	struct iterator	i;

	InitIterator(&i, 1);

	while (i.hasNextTrack(&i)) {
		TOC		*p = i.getNextTrack(&i);
		unsigned	ii = GETTRACK(p);

		if (ii == CDROM_LEADOUT)
			break;
		if (IS__AUDIO(p)) {
			return (ii);
		}
	}
	return (-1);
}

long
FirstDataTrack()
{
	struct iterator	i;

	InitIterator(&i, 1);

	while (i.hasNextTrack(&i)) {
		TOC	*p = i.getNextTrack(&i);
		if (IS__DATA(p)) {
			return (GETTRACK(p));
		}
	}
	return (-1);
}

long
LastTrack()
{
	return (g_toc[cdtracks].bTrack);
}

long
LastAudioTrack()
{
	long		j = -1;
	struct	iterator i;

	InitIterator(&i, 1);

	while (i.hasNextTrack(&i)) {
		TOC	*p = i.getNextTrack(&i);

		if (IS__AUDIO(p) && (GETTRACK(p) != CDROM_LEADOUT)) {
			j = GETTRACK(p);
		}
	}
	return (j);
}

long
Get_LastSectorOnCd(p_track)
	unsigned long p_track;
{
	long		LastSec = 0;
	struct iterator	i;

	if (global.illleadout_cd && global.reads_illleadout)
		return (150+(99*60+59)*75+74);

	InitIterator(&i, p_track);

	if (p_track == cdtracks + 1)
		p_track = CDROM_LEADOUT;

	while (i.hasNextTrack(&i)) {
		TOC	*p = i.getNextTrack(&i);

		if (GETTRACK(p) < p_track)
			continue;

		LastSec = GETSTART(p);

		if (IS__DATA(p))
			break;
	}
	return (LastSec);
}

int
Get_Track(sector)
	unsigned long	sector;
{
	struct iterator	i;

	InitIterator(&i, 1);

	if (i.hasNextTrack(&i)) {
		TOC	*o = i.getNextTrack(&i);

		while (i.hasNextTrack(&i)) {
			TOC	*p = i.getNextTrack(&i);

			if ((GETSTART(o) <= sector) && (GETSTART(p) > sector)) {
				if (IS__DATA(o)) {
					return (-1);
				} else {
					return (GETTRACK(o));
				}
			}
			o = p;
		}
	}
	return (-1);
}

int
CheckTrackrange(from, upto)
	unsigned long	from;
	unsigned long	upto;
{
	struct iterator	i;

	InitIterator(&i, from);

	while (i.hasNextTrack(&i)) {
		TOC *p = i.getNextTrack(&i);

		if (GETTRACK(p) < from)
			continue;

		if (GETTRACK(p) == upto)
			return (1);

		/*
		 * data tracks terminate the search
		 */
		if (IS__DATA(p))
			return (0);
	}
	/*
	 * track not found
	 */
	return (0);
}

#ifdef	USE_PARANOIA
long	cdda_disc_firstsector	__PR((void *d));

long
cdda_disc_firstsector(d)
	void	*d;
{
	return (Get_StartSector(FirstAudioTrack()));
}

int	cdda_tracks	__PR((void *d));

int
cdda_tracks(d)
	void	*d;
{
	return (LastAudioTrack() - FirstAudioTrack() +1);
}

int	cdda_track_audiop	__PR((void *d, int track));

int
cdda_track_audiop(d, track)
	void	*d;
	int	track;
{
	return (Get_Datatrack(track) == 0);
}

long	cdda_track_firstsector	__PR((void *d, int track));

long
cdda_track_firstsector(d, track)
	void	*d;
	int	track;
{
	return (Get_AudioStartSector(track));
}

long cdda_track_lastsector __PR((void *d, int track));

long
cdda_track_lastsector(d, track)
	void	*d;
	int	track;
{
	return (Get_EndSector(track));
}

long	cdda_disc_lastsector	__PR((void *d));

long
cdda_disc_lastsector(d)
	void	*d;
{
	return (Get_LastSectorOnCd(cdtracks) - 1);
}

int	cdda_sector_gettrack	__PR((void *d, long sector));

int
cdda_sector_gettrack(d, sector)
	void	*d;
	long	sector;
{
	return (Get_Track(sector));
}

#endif
