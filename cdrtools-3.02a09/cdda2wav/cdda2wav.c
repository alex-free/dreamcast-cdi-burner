/* @(#)cdda2wav.c	1.170 17/12/07 Copyright 1993-2004,2015,2017 Heiko Eissfeldt, Copyright 2004-2017 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)cdda2wav.c	1.170 17/12/07 Copyright 1993-2004,2015,2017 Heiko Eissfeldt, Copyright 2004-2017 J. Schilling";

#endif
#undef	DEBUG_BUFFER_ADDRESSES
#undef	GPROF
#undef	DEBUG_FORKED
#undef	DEBUG_CLEANUP
#undef	DEBUG_DYN_OVERLAP
#undef	DEBUG_READS

#define	DEBUG_ILLLEADOUT	0	/* 0 disables, 1 enables */
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
/*
 * Copright 1993-2004,2015,2017	(C) Heiko Eissfeldt
 * Copright 2004-2017		(C) J. Schilling
 *
 * last changes:
 *   18.12.93 - first version,	OK
 *   01.01.94 - generalized & clean up HE
 *   10.06.94 - first linux version HE
 *   12.06.94 - wav header alignment problem fixed HE
 *   12.08.94 - open the cdrom device O_RDONLY makes more sense :-)
 *		no more floating point math
 *		change to sector size 2352 which is more common
 *		sub-q-channel information per kernel ioctl requested
 *		doesn't work as well as before
 *		some new options (-max -i)
 *   01.02.95 - async i/o via semaphores and shared memory
 *   03.02.95 - overlapped reading on sectors
 *   03.02.95 - generalized sample rates. all integral divisors are legal
 *   04.02.95 - sun format added
 *              more divisors: all integral halves >= 1 allowed
 *		floating point math needed again
 *   06.02.95 - bugfix for last track and not d0
 *              tested with photo-cd with audio tracks
 *		tested with xa disk
 *   29.01.96 - new options for bulk transfer
 *   01.06.96 - tested with enhanced cd
 *   01.06.96 - tested with cd-plus
 *   02.06.96 - support pipes
 *   02.06.96 - support raw format
 *   04.02.96 - security hole fixed
 *   22.04.97 - large parts rewritten
 *   28.04.97 - make file names DOS compatible
 *   01.09.97 - add speed control
 *   20.10.97 - add find mono option
 *   Jan/Feb 98 - conversion to use Joerg Schillings SCSI library
 *   see ChangeLog
 */

#include "config.h"

#include <schily/unistd.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/nlsdefs.h>
#include <schily/signal.h>
#include <schily/math.h>
#include <schily/fcntl.h>
#include <schily/time.h>
#include <schily/limits.h>
#include <schily/ioctl.h>
#include <schily/errno.h>
#include <schily/stat.h>
#include <schily/wait.h>
#include <schily/resource.h>
#include <schily/varargs.h>
#include <schily/maxpath.h>
#include <schily/btorder.h>
#include <schily/io.h>		/* for setmode() prototype */
#include <schily/priv.h>	/* To get PRIV_PFEXEC definition */
#include <schily/schily.h>

#include <scg/scsitransp.h>

#ifdef	HAVE_AREAS
#ifdef	HAVE_OS_H
#include <OS.h>
#else
#include <be/kernel/OS.h>
#endif
#endif

#include "mytype.h"
#include "sndconfig.h"

#include "semshm.h"	/* semaphore functions */
#include "sndfile.h"
#include "wav.h"	/* wav file header structures */
#include "sun.h"	/* sun audio file header structures */
#include "raw.h"	/* raw file handling */
#include "aiff.h"	/* aiff file handling */
#include "aifc.h"	/* aifc file handling */
#ifdef	USE_LAME
#include "mp3.h"	/* mp3 file handling */
#endif
#include "interface.h"  /* low level cdrom interfacing */
#include "cdda2wav.h"
#include "cdrecord.h"	/* Only for getnum() */
#include "resample.h"
#include "toc.h"
#include "setuid.h"
#include "ringbuff.h"
#include "global.h"
#include "exitcodes.h"
#ifdef	USE_PARANOIA
#include "cdda_paranoia.h"
#endif
#include "parse.h"
#include "cdrdeflt.h"
#include "version.h"

#ifdef	VMS
#include <vms_init.h>
#define	open(n, p, m)	(open)((n), (p), (m), \
				"ctx=bin", "rfm=fix", "mrs=512", \
				"acc", acc_cb, &open_id)
#endif

EXPORT	int	main			__PR((int argc, char **argv));
#ifdef	ECHO_TO_SOUNDCARD
LOCAL	void	RestrictPlaybackRate	__PR((long newrate));
#endif
LOCAL	void	output_indices		__PR((FILE *fp, index_list *p,
						unsigned trackstart));
LOCAL	FILE	*info_file_open		__PR((char *fname_baseval,
						unsigned int track,
						BOOL doappend,
						BOOL numbered));
LOCAL	FILE	*cue_file_open		__PR((char *fname_baseval));
LOCAL	void	get_datetime		__PR((char *datetime, int size));
LOCAL	int	write_info_file		__PR((char *fname_baseval,
						unsigned int track,
						unsigned long SamplesDone,
						int numbered));
LOCAL	int	write_md5_info		__PR((char *fname_baseval,
						unsigned int track,
						BOOL numbered));
LOCAL	void	write_cue_global	__PR((FILE *cuef, char *fname_baseval));
LOCAL	char	*index_str		__PR((long sector));
LOCAL	void	write_cue_track		__PR((FILE *cuef, char *fname_baseval,
						unsigned int track));
LOCAL	void	CloseAudio		__PR((int channels_val,
						unsigned long nSamples,
						struct soundfile *audio_out));
LOCAL	void	CloseAll		__PR((void));
LOCAL	void	OpenAudio		__PR((char *fname, double rate,
						long nBitsPerSample,
						long channels_val,
						unsigned long expected_bytes,
						struct soundfile *audio_out));
LOCAL	void	set_offset		__PR((myringbuff *p, int offset));
LOCAL	int	get_offset		__PR((myringbuff *p));
LOCAL	void	usage			__PR((void));
LOCAL	void	prdefaults		__PR((FILE *f));
LOCAL	void	init_globals		__PR((void));
LOCAL	int	is_fifo			__PR((char * filename));
LOCAL	const char *get_audiotype	__PR((void));
LOCAL	void	priv_warn		__PR((const char *what, const char *msg));
LOCAL	void	gargs			__PR((int argc, char *argv[]));


/*
 * Rules:
 * unique parameterless options first,
 * unique parametrized option names next,
 * ambigious parameterless option names next,
 * ambigious string parametrized option names last
 */
/* BEGIN CSTYLED */
LOCAL const char *opts = "paranoia,paraopts&,version,help,h,\
no-write,N,dump-rates,R,bulk,B,alltracks,verbose-scsi+,V+,\
find-extremes,F,find-mono,G,no-infofile,H,no-textdefaults,\
no-textfile,cuefile,no-hidden-track,\
deemphasize,T,info-only,J,silent-scsi,Q,\
cddbp-server*,cddbp-port*,\
scanbus,device*,dev*,D*,scgopts*,debug#,debug-scsi#,kdebug#,kd#,kdebug-scsi#,ts&,\
auxdevice*,A*,interface*,I*,output-format*,O*,\
output-endianess*,E*,cdrom-endianess*,C*,speed#,S#,\
playback-realtime#L,p#L,md5,M,set-overlap#,P#,sound-device*,K*,\
cddb#,L#,channels*,c*,bits-per-sample#,b#,rate#,r#,gui,g,\
divider*,a*,track*,t*,index#,i#,duration*,d*,offset#L,o#L,start-sector#L,\
sectors-per-request#,n#,verbose-level&,v&,buffers-in-ring#,l#,\
stereo,s,mono,m,wait,w,echo,e,quiet,q,max,x,out-fd#,audio-fd#,no-fork,interactive\
";
/* END CSTYLED */


/*
 * Global variables
 */
EXPORT global_t	global;

/*
 * Local variables
 */
LOCAL unsigned long	nSamplesDone = 0;
LOCAL unsigned long	*nSamplesToDo;
LOCAL unsigned int	current_track_reading;
LOCAL unsigned int	current_track_writing;
LOCAL int		bulk = 0;

unsigned int get_current_track_writing __PR((void));

unsigned int
get_current_track_writing()
{
	return (current_track_writing);
}

#ifdef	ECHO_TO_SOUNDCARD
LOCAL void
RestrictPlaybackRate(newrate)
	long	newrate;
{
	global.playback_rate = newrate;

	/*
	 * filter out insane values
	 */
	if (global.playback_rate < 25)
		global.playback_rate = 25;
	if (global.playback_rate > 250)
		global.playback_rate = 250;

	if (global.playback_rate < 100)
		global.nsectors = (global.nsectors*global.playback_rate)/100;
}
#endif

long
SamplesNeeded(amount, undersampling_val)
	long	amount;
	long	undersampling_val;
{
	long	retval = ((undersampling_val * 2 + Halved) * amount) / 2;

	if (Halved && (*nSamplesToDo & 1))
		retval += 2;
	return (retval);
}

LOCAL int	argc2;
LOCAL int	argc3;
LOCAL char	**argv2;

LOCAL void reset_name_iterator __PR((void));
LOCAL void
reset_name_iterator()
{
	argv2 -= argc3 - argc2;
	argc2 = argc3;
}

LOCAL char *get_next_name __PR((void));
LOCAL char
*get_next_name()
{
	if (argc2 > 0) {
		argc2--;
		return (*argv2++);
	} else {
		return (NULL);
	}
}

LOCAL char *cut_extension __PR((char * fname));

LOCAL char
*cut_extension(fname)
	char	*fname;
{
	char	*pp;

	pp = strrchr(fname, '.');

	if (pp == NULL) {
		pp = fname + strlen(fname);
	}
	*pp = '\0';

	return (pp);
}

#ifdef INFOFILES
LOCAL void
output_indices(fp, p, trackstart)
	FILE		*fp;
	index_list	*p;
	unsigned	trackstart;
{
	int	ci;

	fprintf(fp, "Index=\t\t");

	if (p == NULL) {
		fprintf(fp, "0\n");
		return;
	}

	for (ci = 1; p != NULL; ci++, p = p->next) {
		int	frameoff = p->frameoffset;

		if (p->next == NULL)
			fputs("\nIndex0=\t\t", fp);
#if 0
		else if (ci > 8 && (ci % 8) == 1)
			fputs("\nIndex =\t\t", fp);
#endif
		if (frameoff != -1)
			fprintf(fp, "%d ", frameoff - trackstart);
		else
			fprintf(fp, "-1 ");
	}
	fputs("\n", fp);
}

LOCAL FILE *
info_file_open(fname_baseval, track, doappend, numbered)
	char		*fname_baseval;
	unsigned int	track;
	BOOL		doappend;
	BOOL		numbered;
{
	char	fname[PATH_MAX+1];

	/*
	 * write info file
	 */
	if (strcmp(fname_baseval, "-") == 0)
		return ((FILE *)0);

	strncpy(fname, fname_baseval, sizeof (fname) -1);
	fname[sizeof (fname) -1] = 0;
	if (numbered)
		sprintf(cut_extension(fname), "_%02u.inf", track);
	else
		strcpy(cut_extension(fname), ".inf");

	return (fopen(fname, doappend ? "a" : "w"));
}

LOCAL FILE *
cue_file_open(fname_baseval)
	char		*fname_baseval;
{
	char	fname[PATH_MAX+1];

	/*
	 * write info file
	 */
	if (strcmp(fname_baseval, "-") == 0)
		return ((FILE *)0);

	strncpy(fname, fname_baseval, sizeof (fname) -1);
	fname[sizeof (fname) -1] = 0;
	strcpy(cut_extension(fname), ".cue");

	return (fopen(fname, "w"));
}

LOCAL void
get_datetime(datetime, size)
	char	*datetime;
	int	size;
{
	time_t	utc_time;
	struct tm *tmptr;

	utc_time = time(NULL);
	tmptr = localtime(&utc_time);
	if (tmptr) {
		strftime(datetime, size, "%x %X", tmptr);
	} else {
		strlcpy(datetime, "unknown", size);
	}
}

/*
 * write information before the start of the sampling process
 *
 *
 * uglyfied for Joerg Schillings ultra dumb line parser
 */
LOCAL int
write_info_file(fname_baseval, track, SamplesDone, numbered)
	char		*fname_baseval;
	unsigned int	track;
	unsigned long	SamplesDone;
	int		numbered;
{
	FILE	*info_fp;
	char	datetime[30];
	time_t	utc_time;
	struct tm *tmptr;

	/*
	 * write info file
	 */
	if (strcmp(fname_baseval, "-") == 0)
		return (0);

	info_fp = info_file_open(fname_baseval, track, FALSE, numbered);
	if (!info_fp)
		return (-1);

	utc_time = time(NULL);
	tmptr = localtime(&utc_time);
	if (tmptr) {
		strftime(datetime, sizeof (datetime), "%x %X", tmptr);
	} else {
		strncpy(datetime, "unknown", sizeof (datetime));
	}
	fprintf(info_fp, "#created by cdda2wav %s%s %s\n#\n",
								VERSION,
								VERSION_OS,
								datetime);
	fprintf(info_fp, "CDINDEX_DISCID=\t'%s'\n", global.cdindex_id);
	/* BEGIN CSTYLED */
	fprintf(info_fp,
"CDDB_DISCID=\t0x%08lx\n\
MCN=\t\t%s\n\
ISRC=\t\t%15.15s\n\
#\n",
		(unsigned long) global.cddb_id,
		Get_MCN(),
		Get_ISRC(track));
	/* END CSTYLED */

	fprintf(info_fp, "Albumperformer=\t'%s'\n",
		global.performer != NULL ?
			global.performer : (const unsigned char *)"");
	fprintf(info_fp, "Performer=\t'%s'\n",
		global.trackperformer[track] != NULL ?
			global.trackperformer[track] :
		(global.no_textdefaults == 0 && global.performer != NULL ?
			global.performer : (const unsigned char *)""));

	fprintf(info_fp, "Albumsongwriter='%s'\n",
		global.songwriter != NULL ?
			global.songwriter : (const unsigned char *)"");
	fprintf(info_fp, "Songwriter=\t'%s'\n",
		global.tracksongwriter[track] != NULL ?
			global.tracksongwriter[track] :
		(global.no_textdefaults == 0 && global.songwriter != NULL ?
			global.songwriter : (const unsigned char *)""));

	fprintf(info_fp, "Albumcomposer=\t'%s'\n",
		global.composer != NULL ?
			global.composer : (const unsigned char *)"");
	fprintf(info_fp, "Composer=\t'%s'\n",
		global.trackcomposer[track] != NULL ?
			global.trackcomposer[track] :
		(global.no_textdefaults == 0 && global.composer != NULL ?
			global.composer : (const unsigned char *)""));

	fprintf(info_fp, "Albumarranger=\t'%s'\n",
		global.arranger != NULL ?
			global.arranger : (const unsigned char *)"");
	fprintf(info_fp, "Arranger=\t'%s'\n",
		global.trackarranger[track] != NULL ?
			global.trackarranger[track] :
		(global.no_textdefaults == 0 && global.arranger != NULL ?
			global.arranger : (const unsigned char *)""));

	fprintf(info_fp, "Albummessage=\t'%s'\n",
		global.message != NULL ?
			global.message : (const unsigned char *)"");
	fprintf(info_fp, "Message=\t'%s'\n",
		global.trackmessage[track] != NULL ?
			global.trackmessage[track] :
		(global.no_textdefaults == 0 && global.message != NULL ?
			global.message : (const unsigned char *)""));

	fprintf(info_fp, "Albumclosed_info='%s'\n",
		global.closed_info != NULL ?
			global.closed_info : (const unsigned char *)"");
	fprintf(info_fp, "Closed_info=\t'%s'\n",
		global.trackclosed_info[track] != NULL ?
			global.trackclosed_info[track] :
		(global.no_textdefaults == 0 && global.closed_info != NULL ?
			global.closed_info : (const unsigned char *)""));

	fprintf(info_fp, "Albumtitle=\t'%s'\n",
		global.disctitle != NULL ?
			global.disctitle : (const unsigned char *)"");
	fprintf(info_fp,  "Tracktitle=\t'%s'\n",
		global.tracktitle[track] ?
			global.tracktitle[track] : (const unsigned char *)"");

	fprintf(info_fp, "Track=\t\t%u\n", track);
	fprintf(info_fp, "Tracknumber=\t%u\n", Get_Tracknumber(track));
	fprintf(info_fp, "Trackstart=\t%ld\n", Get_AudioStartSector(track));
	fprintf(info_fp,
	"# track length in sectors (1/75 seconds each), rest samples\nTracklength=\t%ld, %d\n",
		SamplesDone/588L, (int)(SamplesDone%588));
	fprintf(info_fp, "Pre-emphasis=\t%s\n",
		Get_Preemphasis(track) && (global.deemphasize == 0) ?
						"yes" : "no");
	fprintf(info_fp, "Channels=\t%d\n",
		Get_Channels(track) ? 4 : global.channels == 2 ? 2 : 1);

	{  int cr = Get_Copyright(track);

		fputs("Copy_permitted=\t", info_fp);
		switch (cr) {
			case 0:
				fputs("once (copyright protected)\n", info_fp);
			break;
			case 1:
				fputs("no (SCMS first copy, not made by copyright holder)\n", info_fp);
			break;
			case 2:
				fputs("yes (not copyright protected)\n",
					info_fp);
			break;
			default:
				fputs("unknown\n", info_fp);
		}
	}
	fprintf(info_fp, "Endianess=\t%s\n",
		global.need_big_endian ? "big" : "little");
	fprintf(info_fp, "# index list\n");
	output_indices(info_fp, global.trackindexlist[track],
		Get_AudioStartSector(track));

	fclose(info_fp);
	return (0);
}

LOCAL int
write_md5_info(fname_baseval, track, numbered)
	char		*fname_baseval;
	unsigned int	track;
	BOOL		numbered;
{
#ifdef MD5_SIGNATURES
	FILE	*info_fp;

	/*
	 * write info file
	 */
	if (strcmp(fname_baseval, "-") == 0)
		return (0);

	info_fp = info_file_open(fname_baseval, track, TRUE, numbered);
	if (!info_fp)
		return (-1);

	if (global.md5blocksize)
		MD5Final(global.MD5_result, global.context);

	fprintf(info_fp,
		"# md5 sum\nMD5-offset=\t%d\n", global.md5offset);
	if (global.md5blocksize) {
		fprintf(info_fp,
			"MD5-size=\t%d\n", global.md5size);
		fprintf(info_fp,
			"MD5-sum=\t%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			global.MD5_result[0],
			global.MD5_result[1],
			global.MD5_result[2],
			global.MD5_result[3],
			global.MD5_result[4],
			global.MD5_result[5],
			global.MD5_result[6],
			global.MD5_result[7],
			global.MD5_result[8],
			global.MD5_result[9],
			global.MD5_result[10],
			global.MD5_result[11],
			global.MD5_result[12],
			global.MD5_result[13],
			global.MD5_result[14],
			global.MD5_result[15]);
	}
	fclose(info_fp);
#endif
	return (0);
}
#endif	/* INFOFILES */

LOCAL void
write_cue_global(cuef, fname_baseval)
	FILE	*cuef;
	char	*fname_baseval;
{
	char	datetime[30];
	char	fname[200];

	if (cuef == NULL)
		return;

	get_datetime(datetime, sizeof (datetime));

	fprintf(cuef, "REM created by cdda2wav %s%s %s\n",
								VERSION,
								VERSION_OS,
								datetime);
	fprintf(cuef, "REM CDRTOOLS\n");
	fprintf(cuef, "REM\n");

	if (Get_MCN()[0] != '\0')
		fprintf(cuef, "CATALOG %s\n", Get_MCN());
	if (global.did_textfile) {
		char	*pp;

		strncpy(fname, fname_baseval, sizeof (fname) -1);
		fname[sizeof (fname) -1] = 0;
		pp = strrchr(fname, '.');
		if (pp == NULL)
			pp = fname + strlen(fname);
		strncpy(pp, ".cdtext", sizeof (fname) - 1 - (pp - fname));
		fprintf(cuef, "CDTEXTFILE \"%s\"\n", fname);
	}
	if (global.performer)
		fprintf(cuef, "PERFORMER \"%s\"\n", global.performer);
	if (global.songwriter)
		fprintf(cuef, "SONGWRITER \"%s\"\n", global.songwriter);
	if (global.disctitle)
		fprintf(cuef, "TITLE \"%s\"\n",  global.disctitle);

	/*
	 * If we allow to wite cue files with -B, we need to remove the FILE
	 * entry here.
	 */
	snprintf(fname, sizeof (fname),
		"%s.%s",
		global.fname_base,
		get_audiotype());
	fprintf(cuef, "FILE \"%s\" %s\n",  fname, global.audio_out->auf_cuename);
}

LOCAL char *
index_str(sector)
	long	sector;
{
static	char	strbuf[10];
	long	val;

	val = sector / (60 * 75);
	if (val > 99)
		return ("err");
	strbuf[0] = (val / 10) + '0';
	strbuf[1] = (val % 10) + '0';
	strbuf[2] = ':';

	sector = sector % (60 * 75);
	val = sector / 75;
	strbuf[3] = (val / 10) + '0';
	strbuf[4] = (val % 10) + '0';
	strbuf[5] = ':';

	val = sector % 75;
	strbuf[6] = (val / 10) + '0';
	strbuf[7] = (val % 10) + '0';
	strbuf[8] = '\0';

	return (strbuf);
}

LOCAL void
write_cue_track(cuef, fname_baseval, track)
	FILE	*cuef;
	char	*fname_baseval;
	unsigned int track;
{
	long		off_01;
	int		i;
	index_list	*ip;

	if (cuef == NULL)
		return;
	if (track == 0)
		return;

	/*
	 * If we allow to write cue files with -B, we need to add the FILE
	 * entry here.
	 */

	fprintf(cuef, "  TRACK %2.2d AUDIO\n", Get_Tracknumber(track));
	if (global.tracktitle[track])
		fprintf(cuef, "    TITLE \"%s\"\n", global.tracktitle[track]);
	if (global.trackperformer[track] ||
	    (global.no_textdefaults == 0 && global.performer != NULL)) {
		fprintf(cuef, "    PERFORMER \"%s\"\n",
				global.trackperformer[track] != NULL ?
				global.trackperformer[track] :
				global.performer);
	}
	if (global.tracksongwriter[track] ||
	    (global.no_textdefaults == 0 && global.songwriter != NULL)) {
		fprintf(cuef, "    SONGWRITER \"%s\"\n",
				global.tracksongwriter[track] != NULL ?
				global.tracksongwriter[track] :
				global.songwriter);
	}
	if (Get_ISRC(track)[0]) {
		char	*p = (char *)Get_ISRC(track);
		fprintf(cuef, "    ISRC ");
		while (*p) {
			if (*p == '-')
				p++;
			putc(*p++, cuef);
		}
		fprintf(cuef, "\n");
	}
	if ((Get_Preemphasis(track) && (global.deemphasize == 0)) ||
	    Get_Copyright(track) != 0 ||
	    Get_Channels(track)) {
		fprintf(cuef, "    FLAGS");
		switch (Get_Copyright(track)) {

		case 1:	fprintf(cuef, " SCMS"); break;
		case 2:	fprintf(cuef, " DCP"); break;
		}
		if (Get_Channels(track))
			fprintf(cuef, " 4CH");
		if (Get_Preemphasis(track) && (global.deemphasize == 0))
			fprintf(cuef, " PRE");
		fprintf(cuef, "\n");
	}
	for (ip = global.trackindexlist[track]; ip; ip = ip->next) {
		if (ip->next == NULL)
			break;
	}
	if (useHiddenTrack())
		off_01 = 0L;
	else
		off_01 = Get_AudioStartSector(FirstAudioTrack());
	if (global.trackindexlist[track] == NULL) {
		if (track == 1) {
			if (useHiddenTrack())
				fprintf(cuef, "    INDEX 00 %s\n", "00:00:00");
			else
				fprintf(cuef, "    PREGAP %s\n", index_str(off_01));
		}
		fprintf(cuef, "    INDEX 01 %s\n",
			index_str(Get_AudioStartSector(track) - off_01));
	} else if (track == 1 &&
	    global.trackindexlist[track] &&
	    global.trackindexlist[track]->frameoffset != 0) {
		if (useHiddenTrack())
			fprintf(cuef, "    INDEX 00 %s\n", "00:00:00");
		else
			fprintf(cuef, "    PREGAP %s\n",  index_str(off_01));
	} else if (track > 1) {
		for (ip = global.trackindexlist[track-1]; ip; ip = ip->next) {
			if (ip->next == NULL)
				break;
		}
		if (ip && ip->frameoffset != -1)
			fprintf(cuef, "    INDEX 00 %s\n",
				index_str((long)ip->frameoffset - off_01));
	}
	for (i = 1, ip = global.trackindexlist[track]; ip; i++, ip = ip->next) {
		if (ip->next == NULL)
			break;
		if (ip->frameoffset == -1)
			continue;
		fprintf(cuef, "    INDEX %2.2d %s\n", i,
			index_str((long)ip->frameoffset - off_01));
	}
}

LOCAL void
CloseAudio(channels_val, nSamples, audio_out)
	int channels_val;
	unsigned long nSamples;
	struct soundfile *audio_out;
{
	/*
	 * define length
	 */
	audio_out->ExitSound(global.audio,
			(nSamples-global.SkippedSamples) *
			global.OutSampleSize*channels_val);

	close(global.audio);
	global.audio = -1;
}

LOCAL	unsigned int	track = (unsigned int)-1;

/*
 * On terminating:
 * define size-related entries in audio file header, update and close file
 */
LOCAL void
CloseAll()
{
	int	amiparent;

	/*
	 * terminate child process first
	 */
	amiparent = global.child_pid > 0;

	if (global.iloop > 0) {
		/* set to zero */
		global.iloop = 0;
	}

#if	defined	HAVE_FORK_AND_SHAREDMEM
#ifdef DEBUG_CLEANUP
	fprintf(stderr, _("%s terminating, \n"), amiparent ?
		_("Parent (READER)") : _("Child (WRITER)"));
#endif
#else
#ifdef DEBUG_CLEANUP
	fprintf(stderr, _("Cdda2wav single process terminating, \n"));
#endif
#endif

	if (amiparent || global.child_pid < 0) {
		/* switch to original mode and close device */
		EnableCdda(get_scsi_p(), 0, 0);
	}

	if (!amiparent) {
		/* do general clean up */

		if (global.audio >= 0) {
			if (bulk) {
				/* finish sample file for this track */
				CloseAudio(global.channels,
					global.nSamplesDoneInTrack,
					global.audio_out);
			} else {
				/* finish sample file for this track */
				CloseAudio(global.channels,
					(unsigned int) *nSamplesToDo,
					global.audio_out);
			}
		}

		/* tell minimum and maximum amplitudes, if required */
		if (global.findminmax) {
			fprintf(outfp,
			_("Right channel: minimum amplitude :%d/-32768, maximum amplitude :%d/32767\n"),
			global.minamp[0], global.maxamp[0]);
			fprintf(outfp,
			_("Left  channel: minimum amplitude :%d/-32768, maximum amplitude :%d/32767\n"),
			global.minamp[1], global.maxamp[1]);
		}

		/* tell mono or stereo recording, if required */
		if (global.findmono) {
			fprintf(outfp,
				_("Audio samples are originally %s.\n"),
				global.ismono ? _("mono") : _("stereo"));
		}

		return;	/* end of child or single process */
	}


	if (global.have_forked == 1) {
#ifdef	HAVE_FORK
		WAIT_T	chld_return_status;
#endif
#ifdef DEBUG_CLEANUP
		fprintf(stderr, _("Parent wait for child death, \n"));
#endif
		semdestroy();

#ifdef	HAVE_FORK
		/*
		 * If we don't have fork don't wait() either,
		 * else wait for child to terminate.
		 */
		if (0 > wait(&chld_return_status)) {
			errmsg(_("Error waiting for child.\n"));
		} else {
			if (WIFEXITED(chld_return_status)) {
				if (WEXITSTATUS(chld_return_status)) {
					fprintf(stderr,
					_("\nW Child exited with %d\n"),
					WEXITSTATUS(chld_return_status));
				}
			}
			if (WIFSIGNALED(chld_return_status)) {
				fprintf(stderr,
				_("\nW Child exited due to signal %d\n"),
				WTERMSIG(chld_return_status));
			}
			if (WIFSTOPPED(chld_return_status)) {
				fprintf(stderr,
				_("\nW Child is stopped due to signal %d\n"),
				WSTOPSIG(chld_return_status));
			}
		}

#ifdef DEBUG_CLEANUP
		fprintf(stderr,
			_("\nW Parent child death, state:%d\n"),
			chld_return_status);
#endif
#endif
	}

#ifdef GPROF
	rename("gmon.out", "gmon.child");
#endif
}


/*
 * report a usage error and exit
 */
#ifdef  PROTOTYPES
LOCAL void
usage2(const char *szMessage, ...)
#else
LOCAL void
usage2(szMessage, va_alist)
	const char *szMessage;
	va_dcl
#endif
{
	va_list	marker;

#ifdef  PROTOTYPES
	va_start(marker, szMessage);
#else
	va_start(marker);
#endif

	errmsgno(EX_BAD, "%r", szMessage, marker);

	va_end(marker);
	fprintf(stderr,
		_("Please use -help or consult the man page for help.\n"));

	exit(1);
}


/*
 * report a fatal error, clean up and exit
 */
#ifdef  PROTOTYPES
void
FatalError(int err, const char *szMessage, ...)
#else
void
FatalError(err, szMessage, va_alist)
	int		err;
	const char	*szMessage;
	va_dcl
#endif
{
	va_list marker;

#ifdef  PROTOTYPES
	va_start(marker, szMessage);
#else
	va_start(marker);
#endif

	errmsgno(err, "%r", szMessage, marker);

	va_end(marker);

#ifdef	HAVE_KILL
	if (global.child_pid >= 0) {
		if (global.child_pid == 0) {
			pid_t	ppid;
			/*
			 * Kill the parent too if we are not orphaned.
			 */
			ppid = getppid();
			if (ppid > 1)
				kill(ppid, SIGINT);
		} else {
			kill(global.child_pid, SIGINT);
		}
	}
#endif
	exit(1);
}


/*
 * open the audio output file and prepare the header.
 * the header will be defined on terminating (when the size
 * is known). So hitting the interrupt key leaves an intact
 * file.
 */
LOCAL void
OpenAudio(fname, rate, nBitsPerSample, channels_val, expected_bytes, audio_out)
	char *fname;
	double rate;
	long nBitsPerSample;
	long channels_val;
	unsigned long expected_bytes;
	struct soundfile * audio_out;
{
	if (global.audio == -1) {
#ifdef	VMS
		static int	open_id = 3;
#endif
		global.audio = open(fname,
#ifdef SYNCHRONOUS_WRITE
				O_SYNC |
#endif
				O_CREAT | O_WRONLY | O_TRUNC | O_BINARY,
								(mode_t)0666);
		if (global.audio == -1) {
			if (errno == EAGAIN && is_fifo(fname)) {
				FatalError(errno,
				_("Could not open fifo %s. Probably no fifo reader present.\n"),
				fname);
			}
			FatalError(errno,
				_("Could not open audio sample file %s.\n"),
				fname);
		}
	}
	global.SkippedSamples = 0;
	any_signal = 0;
	audio_out->InitSound(global.audio, channels_val, (unsigned long)rate,
					nBitsPerSample, expected_bytes);

#ifdef MD5_SIGNATURES
	global.md5size = 0;
	if (global.md5blocksize)
		MD5Init(global.context);
	global.md5count = global.md5blocksize;
#endif
}

#include "scsi_cmds.h"

LOCAL int	RealEnd __PR((SCSI *scgp, UINT4 *buff));

LOCAL int
RealEnd(scgp, buff)
	SCSI	*scgp;
	UINT4	*buff;
{
	if (scg_cmd_err(scgp) != 0) {
		int c, k, q;

		k = scg_sense_key(scgp);
		c = scg_sense_code(scgp);
		q = scg_sense_qual(scgp);
		if ((k == 0x05	/* ILLEGAL_REQUEST */ &&
		    c  == 0x21	/* lba out of range */ &&
		    q  == 0x00) ||
		    (k == 0x05	/* ILLEGAL_REQUEST */ &&
		    c  == 0x63	/* end of user area encount. on this tr. */ &&
		    q  == 0x00) ||
		    (k == 0x08	/* BLANK_CHECK */ &&
		    c  == 0x64	/* illegal mode for this track */ &&
		    q  == 0x00)) {
			return (1);
		}
	}

	if (scg_getresid(scgp) > 16)
		return (1);

	{
		unsigned char *p;

		/* Look into the subchannel data */
		buff += CD_FRAMESAMPLES;
		p = (unsigned char *)buff;
		if (p[0] == 0x21 && p[1] == 0xaa) {
			return (1);
		}
	}
	return (0);
}

LOCAL void
set_offset(p, offset)
	myringbuff	*p;
	int		offset;
{
#ifdef DEBUG_SHM
	fprintf(stderr, _("Write offset %d at %p\n"), offset, &p->offset);
#endif
	p->offset = offset;
}


LOCAL int
get_offset(p)
	myringbuff	*p;
{
#ifdef DEBUG_SHM
	fprintf(stderr, _("Read offset %d from %p\n"), p->offset, &p->offset);
#endif
	return (p->offset);
}


LOCAL void
usage()
{
	/* BEGIN CSTYLED */
	fputs(_("Usage: cdda2wav [OPTIONS ...] [trackfilenames ...]\n\
OPTIONS:\n\
        [-c chans] [-s] [-m] [-b bits] [-r rate] [-a divider] [-S speed] [-x]\n\
        [-t track[+endtrack]] [-i index] [-o offset] [-d duration] [-F] [-G]\n\
        [-q] [-w] [-v vopts] [-R] [-P overlap] [-B] [-T] [-C input-endianess]\n\
        [-e] [-n sectors] [-N] [-J] [-L cddbp-mode] [-H] [-g] [-l buffers] [-D cd-device]\n\
        [-I interface] [-K sound-device] [-O audiotype] [-E output-endianess]\n\
        [-A auxdevice] [-paranoia] [-cddbp-server=name] [-cddbp-port=port] [-version]\n"),
		stderr);
	/* END CSTYLED */

	/* BEGIN CSTYLED */
	fputs(_("\
  (-D) dev=device		set the cdrom or scsi device (as Bus,Id,Lun).\n\
       scgopts=spec		SCSI options for libscg\n\
       ts=#			set maximum transfer size for a single SCSI command\n\
  (-A) auxdevice=device		set the aux device (typically /dev/cdrom).\n\
  (-K) sound-device=device	set the sound device to use for -e (typically /dev/dsp).\n\
       out-fd=descriptor	set the file descriptor for general output to descriptor.\n\
       audio-fd=descriptor	set the file descriptor for '-' audio output.\n\
  (-I) interface=interface	specify the interface for cdrom access.\n\
        (generic_scsi or cooked_ioctl).\n\
  (-c) channels=channels	set 1 for mono, 2 or s for stereo (s: channels swapped).\n\
  (-s) -stereo			select stereo recording.\n\
  (-m) -mono			select mono recording.\n\
  (-x) -max			select maximum quality (stereo/16-bit/44.1 kHz).\n\
  (-b) bits=bits		set bits per sample per channel (8, 12 or 16 bits).\n\
  (-r) rate=rate		set rate in samples per second. -R gives all rates\n\
  (-a) divider=divider		set rate to 44100Hz / divider. -R gives all rates\n\
  (-R) -dump-rates		dump a table with all available sample rates\n\
  (-S) speed=speedfactor	set the cdrom drive to a given speed during reading\n\
  (-P) set-overlap=sectors	set amount of overlap sampling (default is 0)\n\
  (-n) sectors-per-request=secs	read 'sectors' sectors per request.\n\
  (-l) buffers-in-ring=buffers	use a ring buffer with 'buffers' elements.\n\
  (-t) track=track[+end track]	select start track (option. end track).\n\
  (-i) index=index		select start index.\n\
  (-o) offset=offset		start at 'offset' sectors behind start track/index.\n\
        one sector equivalents 1/75 second.\n\
       start-sector=sector	set absolute start sector.\n\
  (-O) output-format=audiotype	set to wav, au (sun), cdr (raw), aiff or aifc format.\n\
  (-C) cdrom-endianess=endian	set little, big or guess input sample endianess.\n\
  (-E) output-endianess=endian	set little or big output sample endianess.\n\
  (-d) duration=seconds		set recording time in seconds or 0 for whole track.\n\
  (-w) -wait			wait for audio signal, then start recording.\n\
  (-F) -find-extremes		find extrem amplitudes in samples.\n\
  (-G) -find-mono		find if input samples are mono.\n\
  (-T) -deemphasize		undo pre-emphasis in input samples.\n\
  (-e) -echo			echo audio data to sound device (see -K) SOUND_DEV.\n\
  (-v) verbose-level=optlist	controls verbosity (for a list use -vhelp).\n\
  (-N) -no-write		do not create audio sample files.\n\
  (-J) -info-only		give disc information only.\n\
  (-L) cddb=cddbpmode		do cddbp title lookups.\n\
        resolve multiple entries according to cddbpmode: 0=interactive, 1=first entry\n\
       -cuefile			create a CDRWIN CUE file instead of info files.\n\
  (-H) -no-infofile		no info file generation.\n\
       -no-textdefaults		do not fill missing track CD-Text from album CD-Text.\n\
       -no-textfile		no binary cdtext file generation.\n\
       -no-hidden-track		do not scan for hidden audio track (before track #1).\n\
       -no-fork			do not fork for better buffering.\n\
  (-g) -gui			generate special output suitable for gui frontends.\n\
  (-Q) -silent-scsi		do not print status of erreneous scsi-commands.\n\
       -scanbus			scan the SCSI bus and exit.\n\
  (-M) -md5			calculate MD-5 checksum for audio data.\n\
  (-q) -quiet			quiet operation, no screen output.\n\
  (-p) playback-realtime=perc	play (echo) audio pitched at perc percent (50%-200%).\n\
  (-V) -verbose-scsi		each option increases verbosity for SCSI commands.\n\
  (-h) -help			show this help screen.\n\
  (-B) -alltracks, -bulk	record each track into a separate file.\n\
       -paranoia		use the lib paranoia for reading.\n\
       -paraopts=opts		set options for lib paranoia (see -paraopts=help).\n\
       -cddbp-server=servername	set the cddbp server to use for title lookups.\n\
       -cddbp-port=portnumber	set the cddbp port to use for title lookups.\n\
       -interactive		select interactive mode (used by gstreamer plugin).\n\
       -version			print version information.\n\
\n\
Please note: some short options will be phased out soon (disappear)!\n\
\n\
parameters: (optional) one or more file names or - for standard output.\n\
"),
	stderr);
	/* END CSTYLED */

	fputs(_("Version "), stderr);
	fputs(VERSION, stderr);
	fputs(VERSION_OS, stderr);
	prdefaults(stderr);
	exit(SYNTAX_ERROR);
}

LOCAL void
prdefaults(f)
	FILE	*f;
{
	/* BEGIN CSTYLED */
	fprintf(f, _("\n\
Defaults: %s, %d bit, %d.%02d Hz, track 1, no offset, one track,\n"),
		  (CHANNELS-1) ? _("stereo") : _("mono"), BITS_P_S,
		 44100 / UNDERSAMPLING,
		 (4410000 / UNDERSAMPLING) % 100);

	fprintf(f, _("\
	  type: %s filename: '%s', don't wait for signal, not quiet,\n"),
		AUDIOTYPE, FILENAME);
	fprintf(f, _("\
	  use: '%s', device: '%s', aux: '%s'\n"),
		DEF_INTERFACE, CD_DEVICE, AUX_DEVICE);
	/* END CSTYLED */
}

LOCAL void
init_globals()
{
#ifdef	HISTORICAL_JUNK
	global.dev_name = CD_DEVICE;	/* device name */
#endif
	global.dev_opts = NULL;		/* scg device options */
	global.aux_name = AUX_DEVICE;	/* auxiliary cdrom device name */
	global.out_fp = stderr;		/* -out-fd FILE * for messages */
	strncpy(global.fname_base, FILENAME,
		sizeof (global.fname_base)); /* current file name base */
	global.have_forked = 0;		/* state variable for clean up */
	global.child_pid = -2;		/* state variable for clean up */
	global.parent_died = 0;		/* state variable for clean up */
	global.audio    = -1;		/* audio-out file desc */
	global.cooked_fd  = -1;		/* cdrom-in file desc */
	global.no_file  =  0;		/* -N option */
	global.no_infofile  =  0;	/* -no-infofile option */
	global.no_textfile  =  0;	/* -no-textfile option */
	global.did_textfile  =  0;	/* flag: did create textfile */
	global.no_textdefaults = 0;	/* -no-textdefaults option */
	global.no_cddbfile  =  0;	/* flag: do not create cddbfile */
	global.cuefile    =  0;		/* -cuefile option */
	global.no_hidden_track = 0;	/* -no-hidden-track option */
	global.no_fork	  =  0;		/* -no-fork option */
	global.interactive = 0;		/* -interactive option */
	global.quiet	  =  0;		/* -quiet option */
	global.verbose  =  SHOW_TOC + SHOW_SUMMARY +
				SHOW_STARTPOSITIONS +
				SHOW_TITLES;	/* -v verbose level */
	global.scsi_silent = 0;		/* SCSI silent flag */
	global.scsi_verbose = 0;	/* SCSI verbose level */
	global.scsi_debug = 0;		/* SCSI debug level */
	global.scsi_kdebug = 0;		/* SCSI kernel debug level */
	global.scanbus = 0;		/* -scanbus option */

	global.uid = 0;
	global.euid = 0;
	global.issetuid = FALSE;

	global.sector_offset = 0;
	global.start_sector = -1;
	global.endtrack = (unsigned int)-1;
	global.alltracks = FALSE;
	global.maxtrack = FALSE;
	global.cd_index = -1;
	global.littleendian = -1;
	global.rectime = DURATION;
	global.int_part = 0;
	global.user_sound_device = "";
	global.moreargs = 0;

	global.multiname = 0;		/* multiple file names given */
	global.sh_bits  =  0;		/* sh_bits: sample bit shift */
	global.Remainder =  0;		/* remainder */
	global.iloop    =  0;		/* todo counter (frames) */
	global.SkippedSamples =  0;	/* skipped samples */
	global.OutSampleSize  =  0;	/* output sample size */
	global.channels = CHANNELS;	/* output sound channels */
	global.nSamplesDoneInTrack = 0;	/* written samples in current track */
	global.buffers = 4;		/* buffers to use */
	global.bufsize = -1L;		/* The SCSI buffer size */
	global.nsectors = NSECTORS;	/* sectors to read in one request */
	global.overlap = 1;		/* amount of overlapping sectors */
	global.useroverlap = -1;	/* amt of overl. sect. user override */
	global.need_hostorder = 0;	/* processing needs samples in host endianess */
	global.in_lendian = -1;		/* input endianess from SetupSCSI() */
	global.outputendianess = NONE;	/* user specified output endianess */
	global.findminmax  =  0;	/* flag find extrem amplitudes */
#ifdef HAVE_LIMITS_H
	global.maxamp[0] = INT_MIN;	/* maximum amplitude */
	global.maxamp[1] = INT_MIN;	/* maximum amplitude */
	global.minamp[0] = INT_MAX;	/* minimum amplitude */
	global.minamp[1] = INT_MAX;	/* minimum amplitude */
#else
	global.maxamp[0] = -32768;	/* maximum amplitude */
	global.maxamp[1] = -32768;	/* maximum amplitude */
	global.minamp[0] = 32767;	/* minimum amplitude */
	global.minamp[1] = 32767;	/* minimum amplitude */
#endif
	global.speed = DEFAULT_SPEED;	/* use default */
	global.userspeed = -1;		/* speed user override */
	global.findmono  =  0;		/* flag find if samples are mono */
	global.ismono  =  1;		/* flag if samples are mono */
	global.swapchannels  =  0;	/* flag if channels shall be swapped */
	global.deemphasize  =  0;	/* flag undo pre-emphasis in samples */
	global.playback_rate = 100;	/* new fancy selectable sound output rate */
	global.gui  =  0;		/* flag plain formatting for guis */
	global.cddb_id = 0;		/* disc identifying id for CDDB database */
	global.cddb_revision = 0;	/* entry revision for CDDB database */
	global.cddb_year = 0;		/* disc identifying year for CDDB database */
	global.cddb_genre[0] = '\0';	/* disc identifying genre for CDDB database */
	global.cddbp = 0;		/* flag if titles shall be looked up from CDDBP */
	global.cddbp_server = 0;	/* user supplied CDDBP server */
	global.cddbp_port = 0;		/* user supplied CDDBP port */
	global.illleadout_cd = 0;	/* flag if illegal leadout is present */
	global.reads_illleadout = 0;	/* flag if cdrom drive reads cds with illegal leadouts */
	global.copyright_message = NULL;
	global.disctitle = NULL;
	global.performer = NULL;
	global.songwriter = NULL;
	global.composer = NULL;
	global.arranger = NULL;
	global.message = NULL;
	global.closed_info = NULL;
	memset(global.tracktitle, 0, sizeof (global.tracktitle));
	memset(global.trackperformer, 0, sizeof (global.trackperformer));
	memset(global.tracksongwriter, 0, sizeof (global.tracksongwriter));
	memset(global.trackcomposer, 0, sizeof (global.trackcomposer));
	memset(global.trackarranger, 0, sizeof (global.trackarranger));
	memset(global.trackmessage, 0, sizeof (global.trackmessage));
	memset(global.trackclosed_info, 0, sizeof (global.trackclosed_info));
	memset(global.trackindexlist, 0, sizeof (global.trackindexlist));

	global.just_the_toc = 0;
	global.paranoia_selected = 0;
	global.paranoia_flags = 0;
	global.paranoia_mode = 0;
#ifdef	USE_PARANOIA
	global.paranoia_parms.disable_paranoia =
	global.paranoia_parms.disable_extra_paranoia =
	global.paranoia_parms.disable_scratch_detect =
	global.paranoia_parms.disable_scratch_repair =
	global.paranoia_parms.enable_c2_check = 0;
	global.paranoia_parms.retries = 20;
	global.paranoia_parms.readahead = -1;
	global.paranoia_parms.overlap = -1;
	global.paranoia_parms.mindynoverlap = -1;
	global.paranoia_parms.maxdynoverlap = -1;
#endif
	global.md5offset = 0;
	global.md5blocksize = 0;
#ifdef	MD5_SIGNATURES
	global.md5count = 0;
	global.md5size = 0;
#endif
}

#if !defined(HAVE_STRCASECMP) || (HAVE_STRCASECMP != 1)
#include <schily/ctype.h>
LOCAL int strcasecmp __PR((const char *s1, const char *s2));
LOCAL int
strcasecmp(s1, s2)
	const char	*s1;
	const char	*s2;
{
	if (s1 && s2) {
		while (*s1 && *s2 && (tolower(*s1) - tolower(*s2) == 0)) {
			s1++;
			s2++;
		}
		if (*s1 == '\0' && *s2 == '\0')
			return (0);
		if (*s1 == '\0')
			return (-1);
		if (*s2 == '\0')
			return (+1);
		return (tolower(*s1) - tolower(*s2));
	}
	return (-1);
}
#endif

LOCAL int
is_fifo(filename)
	char	*filename;
{
#if	defined S_ISFIFO
	struct stat	statstruct;

	if (stat(filename, &statstruct)) {
		/*
		 * maybe the output file does not exist.
		 */
		if (errno == ENOENT)
			return (0);
		else
			comerr(_("Error during stat for output file\n"));
	} else {
		if (S_ISFIFO(statstruct.st_mode)) {
			return (1);
		}
	}
	return (0);
#else
	return (0);
#endif
}


#if !defined(HAVE_STRTOUL) || (HAVE_STRTOUL != 1)
LOCAL unsigned int strtoul __PR((const char *s1, char **s2, int base));
LOCAL unsigned int
strtoul(s1, s2, base)
	const char	*s1;
	char		**s2;
	int		base;
{
	long retval;

	if (base == 10) {
		/* strip zeros in front */
		while (*s1 == '0')
			s1++;
	}
	if (s2 != NULL) {
		*s2 = astol(s1, &retval);
	} else {
		(void) astol(s1, &retval);
	}

	return ((unsigned long) retval);
}
#endif

LOCAL unsigned long SectorBurst;
#if (SENTINEL > CD_FRAMESIZE_RAW)
error block size for overlap check has to be < sector size
#endif


LOCAL void
switch_to_realtime_priority __PR((void));

#ifdef  HAVE_SYS_PRIOCNTL_H

#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
LOCAL void
switch_to_realtime_priority()
{
	pcinfo_t	info;
	pcparms_t	param;
	rtinfo_t	rtinfo;
	rtparms_t	rtparam;
	int		pid;

	pid = getpid();

	/*
	 * get info
	 */
	strcpy(info.pc_clname, "RT");
	if (-1 == priocntl(P_PID, pid, PC_GETCID, (void *)&info)) {
		errmsg(_("Cannot get priority class id priocntl(PC_GETCID)\n"));
		goto prio_done;
	}

	memmove(&rtinfo, info.pc_clinfo, sizeof (rtinfo_t));

	/*
	 * set priority not to the max
	 */
	rtparam.rt_pri = rtinfo.rt_maxpri - 2;
	rtparam.rt_tqsecs = 0;
	rtparam.rt_tqnsecs = RT_TQDEF;
	param.pc_cid = info.pc_cid;
	memmove(param.pc_clparms, &rtparam, sizeof (rtparms_t));
	if (global.issetuid || global.uid != 0)
		priv_on();
	needroot(0);
	if (-1 == priocntl(P_PID, pid, PC_SETPARMS, (void *)&param))
		errmsg(
		_("Cannot set priority class parameters priocntl(PC_SETPARMS)\n"));
prio_done:
	if (global.issetuid || global.uid != 0)
		priv_off();
	dontneedroot();
}
#else
#if defined(_POSIX_PRIORITY_SCHEDULING) && _POSIX_PRIORITY_SCHEDULING -0 >= 0 && \
    defined(HAVE_SCHED_SETSCHEDULER)
#define	USE_POSIX_PRIORITY_SCHEDULING
#endif

#ifdef	USE_POSIX_PRIORITY_SCHEDULING
#include <sched.h>

LOCAL void
switch_to_realtime_priority()
{
#ifdef  _SC_PRIORITY_SCHEDULING
	if (sysconf(_SC_PRIORITY_SCHEDULING) == -1) {
		errmsg(_("WARNING: RR-scheduler not available, disabling.\n"));
	} else
#endif
	{
	int sched_fifo_min, sched_fifo_max;
	struct sched_param sched_parms;

	sched_fifo_min = sched_get_priority_min(SCHED_FIFO);
	sched_fifo_max = sched_get_priority_max(SCHED_FIFO);
	sched_parms.sched_priority = sched_fifo_max - 1;
	if (global.issetuid || global.uid != 0)
		priv_on();
	needroot(0);
	if (-1 == sched_setscheduler(getpid(), SCHED_FIFO, &sched_parms) &&
	    global.quiet != 1)
		errmsg(_("Cannot set posix realtime scheduling policy.\n"));
	if (global.issetuid || global.uid != 0)
		priv_off();
	dontneedroot();
	}
}
#else
#if defined(__CYGWIN32__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(_MSC_VER)

/*
 * NOTE: Base.h from Cygwin-B20 has a second typedef for BOOL.
 *	 We define BOOL to make all local code use BOOL
 *	 from Windows.h and use the hidden __SBOOL for
 *	 our global interfaces.
 *
 * NOTE: windows.h from Cygwin-1.x includes a structure field named sample,
 *	 so me may not define our own 'sample' or need to #undef it now.
 *	 With a few nasty exceptions, Microsoft assumes that any global
 *	 defines or identifiers will begin with an Uppercase letter, so
 *	 there may be more of these problems in the future.
 *
 * NOTE: windows.h defines interface as an alias for struct, this
 *	 is used by COM/OLE2, I guess it is class on C++
 *	 We man need to #undef 'interface'
 *
 *	 These workarounds are now applied in schily/windows.h
 */
#include <schily/windows.h>
#undef interface

LOCAL void
switch_to_realtime_priority()
{
	/*
	 * set priority class
	 */
	if (FALSE == SetPriorityClass(GetCurrentProcess(),
						REALTIME_PRIORITY_CLASS)) {
		/*
		 * XXX No errno?
		 */
		errmsgno(EX_BAD, _("No realtime priority possible.\n"));
		return;
	}

	/*
	 * set thread priority
	 */
	if (FALSE == SetThreadPriority(GetCurrentThread(),
						THREAD_PRIORITY_HIGHEST)) {
		/*
		 * XXX No errno?
		 */
		errmsgno(EX_BAD, _("Could not set realtime priority.\n"));
	}
}
#else
LOCAL void
switch_to_realtime_priority()
{
}
#endif
#endif
#endif

/* SCSI cleanup */
int on_exitscsi __PR((void *status));

int
on_exitscsi(status)
	void *status;
{
	/*
	 * The double cast is only needed for GCC in LP64 mode.
	 */
	exit((int)(Intptr_t)status);
	return (0);
}

/* wrapper for signal handler exit needed for Mac-OS-X */
LOCAL void exit_wrapper __PR((int status));

LOCAL void
exit_wrapper(status)
	int	status;
{
#if defined DEBUG_CLEANUP
	fprintf(stderr, _("Exit(%d) for %s\n"),
			status, global.child_pid == 0 ? _("Child") : _("Parent"));
	fflush(stderr);
#endif

	if (global.child_pid != 0) {
		SCSI *scgp = get_scsi_p();

		/*
		 * The double cast is only needed for GCC in LP64 mode.
		 */
		if (scgp != NULL && scgp->running) {
			scgp->cb_fun = on_exitscsi;
			scgp->cb_arg = (void *)(Intptr_t)status;
		} else {
			on_exitscsi((void *)(Intptr_t)status);
		}
	} else {
		exit(status);
	}
}

/* signal handler for process communication */
LOCAL void set_nonforked __PR((int status));

/* ARGSUSED */
LOCAL void
set_nonforked(status)
	int	status;
{
	global.parent_died = 1;
#if defined DEBUG_CLEANUP
	fprintf(stderr, _("SIGPIPE received from %s.\n"),
			global.child_pid == 0 ? _("Child") : _("Parent"));
#endif
#ifdef	HAVE_KILL
	if (global.child_pid == 0) {
		pid_t	ppid;
		/*
		 * Kill the parent too if we are not orphaned.
		 */
		ppid = getppid();
		if (ppid > 1)
			kill(ppid, SIGINT);
	} else {
		kill(global.child_pid, SIGINT);
	}
#endif
	exit(SIGPIPE_ERROR);
}



#ifdef	USE_PARANOIA
LOCAL struct paranoia_statistics
{
	long	c_sector;
	long	v_sector;
	int	last_heartbeatstate;
	long	lasttime;
	char	heartbeat;
	int	minoverlap;
	int	curoverlap;
	int	maxoverlap;
	int	slevel;
	int	slastlevel;
	int	stimeout;
	int	rip_smile_level;
	unsigned verifies;
	unsigned reads;
	unsigned sectors;
	unsigned fixup_edges;
	unsigned fixup_atoms;
	unsigned readerrs;
	unsigned c2errs;
	unsigned c2bytes;
	unsigned c2secs;
	unsigned c2maxerrs;
	unsigned c2badsecs;
	unsigned skips;
	unsigned overlaps;
	unsigned scratchs;
	unsigned drifts;
	unsigned fixup_droppeds;
	unsigned fixup_dupeds;
}	*para_stat;


LOCAL void paranoia_statreset __PR((void));
LOCAL void
paranoia_statreset()
{
	para_stat->c_sector = 0;
	para_stat->v_sector = 0;
	para_stat->last_heartbeatstate = 0;
	para_stat->lasttime = 0;
	para_stat->heartbeat = ' ';
	para_stat->minoverlap = 0x7FFFFFFF;
	para_stat->curoverlap = 0;
	para_stat->maxoverlap = 0;
	para_stat->slevel = 0;
	para_stat->slastlevel = 0;
	para_stat->stimeout = 0;
	para_stat->rip_smile_level = 0;
	para_stat->verifies = 0;
	para_stat->reads = 0;
	para_stat->sectors = 0;
	para_stat->readerrs = 0;
	para_stat->c2errs = 0;
	para_stat->c2bytes = 0;
	para_stat->c2secs = 0;
	para_stat->c2maxerrs = 0;
	para_stat->c2badsecs = 0;
	para_stat->fixup_edges = 0;
	para_stat->fixup_atoms = 0;
	para_stat->fixup_droppeds = 0;
	para_stat->fixup_dupeds = 0;
	para_stat->drifts = 0;
	para_stat->scratchs = 0;
	para_stat->overlaps = 0;
	para_stat->skips = 0;
}

LOCAL void paranoia_callback __PR((long inpos, int function));

LOCAL void
paranoia_callback(inpos, function)
	long	inpos;
	int	function;
{
	struct timeval thistime;
	long	test;

	switch (function) {
		case	-2:
			para_stat->v_sector = inpos / CD_FRAMEWORDS;
			return;

		case	-1:
			para_stat->last_heartbeatstate = 8;
			para_stat->heartbeat = '*';
			para_stat->slevel = 0;
			para_stat->v_sector = inpos / CD_FRAMEWORDS;
			break;

		case	PARANOIA_CB_VERIFY:
			if (para_stat->stimeout >= 30) {
				if (para_stat->curoverlap > CD_FRAMEWORDS) {
					para_stat->slevel = 2;
				} else {
					para_stat->slevel = 1;
				}
			}
			para_stat->verifies++;
			break;

		case	PARANOIA_CB_READ:
			if (inpos / CD_FRAMEWORDS > para_stat->c_sector) {
				para_stat->c_sector = inpos / CD_FRAMEWORDS;
			}
			para_stat->reads++;
			break;

		case	PARANOIA_CB_SECS:
			para_stat->sectors += inpos;
			break;

		case	PARANOIA_CB_FIXUP_EDGE:
			if (para_stat->stimeout >= 5) {
				if (para_stat->curoverlap > CD_FRAMEWORDS) {
					para_stat->slevel = 2;
				} else {
					para_stat->slevel = 1;
				}
			}
			para_stat->fixup_edges++;
			break;

		case	PARANOIA_CB_FIXUP_ATOM:
			if (para_stat->slevel < 3 || para_stat->stimeout > 5) {
				para_stat->slevel = 3;
			}
			para_stat->fixup_atoms++;
			break;

		case	PARANOIA_CB_READERR:
			para_stat->slevel = 6;
			para_stat->readerrs++;
			break;

		case	PARANOIA_CB_C2ERR:
			para_stat->slevel = 3;
			para_stat->c2errs++;
			break;

		case	PARANOIA_CB_C2BYTES:
			para_stat->c2bytes += inpos;
			break;

		case	PARANOIA_CB_C2SECS:
			para_stat->c2secs += inpos;
			break;

		case	PARANOIA_CB_C2MAXERRS:
			if (inpos > para_stat->c2maxerrs)
				para_stat->c2maxerrs = inpos;
			if (inpos > 100)
				para_stat->c2badsecs++;
			break;

		case	PARANOIA_CB_SKIP:
			para_stat->slevel = 8;
			para_stat->skips++;
			break;

		case	PARANOIA_CB_OVERLAP:
			para_stat->curoverlap = inpos;
			if (inpos > para_stat->maxoverlap)
				para_stat->maxoverlap = inpos;
			if (inpos < para_stat->minoverlap)
				para_stat->minoverlap = inpos;
			para_stat->overlaps++;
			break;

		case	PARANOIA_CB_SCRATCH:
			para_stat->slevel = 7;
			para_stat->scratchs++;
			break;

		case	PARANOIA_CB_DRIFT:
			if (para_stat->slevel < 4 || para_stat->stimeout > 5) {
				para_stat->slevel = 4;
			}
			para_stat->drifts++;
			break;

		case	PARANOIA_CB_FIXUP_DROPPED:
			para_stat->slevel = 5;
			para_stat->fixup_droppeds++;
			break;

		case	PARANOIA_CB_FIXUP_DUPED:
			para_stat->slevel = 5;
			para_stat->fixup_dupeds++;
			break;
	}

	gettimeofday(&thistime, NULL);
	/* now in tenth of seconds. */
	test = thistime.tv_sec * 10 + thistime.tv_usec / 100000;

	if (para_stat->lasttime != test ||
	    function == -1 ||
	    para_stat->slastlevel != para_stat->slevel) {

		if (function == -1 ||
		    para_stat->slastlevel != para_stat->slevel) {

			static const char hstates[] = " .o0O0o.";

			para_stat->lasttime = test;
			para_stat->stimeout++;

			para_stat->last_heartbeatstate++;
			if (para_stat->last_heartbeatstate > 7) {
				para_stat->last_heartbeatstate = 0;
			}
			para_stat->heartbeat =
				hstates[para_stat->last_heartbeatstate];

			if (function == -1) {
				para_stat->heartbeat = '*';
			}
		}

		if (para_stat->slastlevel != para_stat->slevel) {
			para_stat->stimeout = 0;
		}
		para_stat->slastlevel = para_stat->slevel;
	}

	if (para_stat->slevel < 8) {
		para_stat->rip_smile_level = para_stat->slevel;
	} else {
		para_stat->rip_smile_level = 0;
	}
}
#endif

LOCAL long		lSector;			/* Current sector # */
LOCAL long		lSector_p2;			/* Last sector to read */
LOCAL double		rate = 44100.0 / UNDERSAMPLING;
LOCAL int		bits = BITS_P_S;
LOCAL char		fname[200];
LOCAL const char	*audio_type;
LOCAL long		BeginAtSample;
LOCAL unsigned long	SamplesToWrite;
LOCAL unsigned		minover;
LOCAL unsigned		maxover;

LOCAL unsigned long	calc_SectorBurst __PR((void));
LOCAL void		set_newstart	__PR((long newstart));

LOCAL const char *
get_audiotype()
{
	return (audio_type);
}

LOCAL unsigned long
calc_SectorBurst()
{
	unsigned long	SectorBurstVal;

	SectorBurstVal = min(global.nsectors,
			(global.iloop + CD_FRAMESAMPLES-1) / CD_FRAMESAMPLES);
	if (lSector+(int)SectorBurst-1 >= lSector_p2)
		SectorBurstVal = lSector_p2 - lSector;
	return (SectorBurstVal);
}

LOCAL void
set_newstart(newstart)
	long	newstart;
{
	lSector = newstart;
	global.iloop = (lSector_p2 - lSector) * CD_FRAMESAMPLES;
	*nSamplesToDo = global.iloop;
	nSamplesDone = 0;
	if (global.paranoia_selected)
		paranoia_seek(global.cdp, lSector, SEEK_SET);

#ifdef	DEBUG_INTERACTIVE
	error("iloop %lu (%lu sects == %lu s) lSector %ld -> %ld\n",
		global.iloop, global.iloop/588, global.iloop/588/75,
		lSector, lSector_p2);
	error("*nSamplesToDo %ld nSamplesDone %ld\n",
		*nSamplesToDo, nSamplesDone);
#endif
}

/*
 * if PERCENTAGE_PER_TRACK is defined, the percentage message will reach
 * 100% every time a track end is reached or the time limit is reached.
 *
 * Otherwise if PERCENTAGE_PER_TRACK is not defined, the percentage message
 * will reach 100% once at the very end of the last track.
 */
#define	PERCENTAGE_PER_TRACK

LOCAL int do_read __PR((myringbuff *p, unsigned *total_unsuccessful_retries));
LOCAL int
do_read(p, total_unsuccessful_retries)
	myringbuff	*p;
	unsigned	*total_unsuccessful_retries;
{
	unsigned char *newbuf;
	int offset;
	unsigned int added_size;

	/* how many sectors should be read */
	SectorBurst =  calc_SectorBurst();

#ifdef	USE_PARANOIA
	if (global.paranoia_selected) {
		int i;

		for (i = 0; i < SectorBurst; i++) {
			void *dp;

			dp = paranoia_read_limited(global.cdp,
				paranoia_callback,
				global.paranoia_parms.retries);
#ifdef	nonono
			{
				char *err;
				char *msg;
				err = cdda_errors(global.cdp);
				msg = cdda_messages(global.cdp);
				if (err) {
					fputs(err, stderr);
					free(err);
				}
				if (msg) {
					fputs(msg, stderr);
					free(msg);
				}
			}
#endif	/* nonono */
			if (dp != NULL)	{
				memcpy(p->data + i*CD_FRAMESAMPLES, dp,
					CD_FRAMESIZE_RAW);
			} else {
				errmsgno(EX_BAD,
					_("E unrecoverable error!\n"));
				exit(READ_ERROR);
			}
		}
		newbuf = (unsigned char *)p->data;
		offset = 0;
		set_offset(p, offset);
		added_size = SectorBurst * CD_FRAMESAMPLES;
		global.overlap = 0;
		handle_inputendianess(p->data, added_size);
	} else
#endif
	{
		unsigned int retry_count;
#define	MAX_READRETRY 12

		retry_count = 0;
		do {
			SCSI *scgp = get_scsi_p();
#ifdef DEBUG_READS
			fprintf(stderr,
				"reading from %lu to %lu, overlap %u\n",
				lSector, lSector + SectorBurst -1,
				global.overlap);
#endif

#ifdef DEBUG_BUFFER_ADDRESSES
			fprintf(stderr, "%p %l\n", p->data, global.pagesize);
			if (((unsigned)p->data) & (global.pagesize -1) != 0) {
				fprintf(stderr,
					"Address %p is NOT page aligned!!\n",
					p->data);
			}
#endif

			if (global.reads_illleadout != 0 &&
			    lSector > Get_StartSector(LastTrack())) {
				int	singles = 0;
				UINT4	bufferSub[CD_FRAMESAMPLES + 24];

				/*
				 * we switch to single sector reads,
				 * in order to handle the remaining sectors.
				 */
				scgp->silent++;
				do {
					ReadCdRomSub(scgp, bufferSub,
							lSector+singles, 1);
					*eorecording = RealEnd(scgp,
							bufferSub);
					if (*eorecording) {
						break;
					}
					memcpy(p->data+singles*CD_FRAMESAMPLES,
						bufferSub, CD_FRAMESIZE_RAW);
					singles++;
				} while (singles < SectorBurst);
				scgp->silent--;

				if (*eorecording) {
					patch_real_end(lSector+singles);
					SectorBurst = singles;
#if	DEBUG_ILLLEADOUT
				fprintf(stderr,
				"iloop=%11lu, nSamplesToDo=%11lu, end=%lu -->\n",
					global.iloop,
					*nSamplesToDo,
					lSector+singles);
#endif

					*nSamplesToDo -= global.iloop -
							SectorBurst *
							CD_FRAMESAMPLES;
					global.iloop = SectorBurst *
							CD_FRAMESAMPLES;
#if	DEBUG_ILLLEADOUT
				fprintf(stderr,
				"iloop=%11lu, nSamplesToDo=%11lu\n\n",
					global.iloop, *nSamplesToDo);
#endif

				}
			} else {
				ReadCdRom(scgp, p->data, lSector, SectorBurst);
			}
			handle_inputendianess(p->data,
						SectorBurst * CD_FRAMESAMPLES);
			if (NULL ==
				(newbuf = synchronize(p->data,
						SectorBurst*CD_FRAMESAMPLES,
						*nSamplesToDo-global.iloop))) {
				/*
				 * could not synchronize!
				 * Try to invalidate the cdrom cache.
				 * Increase overlap setting, if possible.
				 */
#ifdef				nonono
				trash_cache(p->data, lSector, SectorBurst);
#endif
				if (global.overlap < global.nsectors - 1) {
					global.overlap++;
					lSector--;
					SectorBurst = calc_SectorBurst();
#ifdef DEBUG_DYN_OVERLAP
					fprintf(stderr,
					"using increased overlap of %u\n",
						global.overlap);
#endif
				} else {
					lSector += global.overlap - 1;
					global.overlap = 1;
					SectorBurst =  calc_SectorBurst();
				}
			} else
				break;
		} while (++retry_count < MAX_READRETRY);

		if (retry_count == MAX_READRETRY && newbuf == NULL &&
		    global.verbose != 0) {
			(*total_unsuccessful_retries)++;
		}

		if (newbuf) {
			offset = newbuf - ((unsigned char *)p->data);
		} else {
			offset = global.overlap * CD_FRAMESIZE_RAW;
		}
		set_offset(p, offset);

		/* how much has been added? */
		added_size = SectorBurst * CD_FRAMESAMPLES - offset/4;

		if (newbuf && *nSamplesToDo != global.iloop) {
			minover = min(global.overlap, minover);
			maxover = max(global.overlap, maxover);


			/* should we reduce the overlap setting ? */
			if (offset > CD_FRAMESIZE_RAW && global.overlap > 1) {
#ifdef DEBUG_DYN_OVERLAP
				fprintf(stderr,
				"decreasing overlap from %u to %u (jitter %d)\n",
				global.overlap, global.overlap-1,
				offset - (global.overlap)*CD_FRAMESIZE_RAW);
#endif
				global.overlap--;
				SectorBurst =  calc_SectorBurst();
			}
		}
	}
	if (global.iloop >= added_size) {
		global.iloop -= added_size;
	} else {
		global.iloop = 0;
	}

	lSector += SectorBurst - global.overlap;

#if	defined	PERCENTAGE_PER_TRACK && defined HAVE_FORK_AND_SHAREDMEM
	if (global.iloop > 0) {
		int as;
		while ((as = Get_StartSector(current_track_reading+1)) != -1 &&
							lSector >= as) {
			current_track_reading++;
		}
	}
#endif

	return (offset);
}

LOCAL void
print_percentage __PR((unsigned *poper, int c_offset));

LOCAL void
print_percentage(poper, c_offset)
	unsigned *poper;
	int	c_offset;
{
	unsigned per;
#ifdef	PERCENTAGE_PER_TRACK
	/* Thomas Niederreiter wants percentage per track */
	unsigned start_in_track = max(BeginAtSample,
		Get_AudioStartSector(current_track_writing)*CD_FRAMESAMPLES);

	per = min(BeginAtSample + (long)*nSamplesToDo,
		Get_StartSector(current_track_writing+1)*CD_FRAMESAMPLES)
		- (long)start_in_track;

	if (per > 0)
		per = (BeginAtSample+nSamplesDone - start_in_track)/(per/100);
	else
		per = 0;

#else
	per = global.iloop ? (nSamplesDone)/(*nSamplesToDo/100) : 100;
#endif

	if (global.overlap > 0) {
		fprintf(outfp, "\r%2u/%2u/%2u/%7d %3u%%",
			minover, maxover, global.overlap,
			c_offset - global.overlap*CD_FRAMESIZE_RAW,
			per);
	} else if (*poper != per) {
		fprintf(outfp, "\r%3u%%", per);
	}
	*poper = per;
	fflush(outfp);
}

LOCAL unsigned long do_write __PR((myringbuff *p));
LOCAL unsigned long
do_write(p)
	myringbuff	*p;
{
	int current_offset;
	unsigned int InSamples;
	static unsigned oper = 200;

	current_offset = get_offset(p);

	/* how many bytes are available? */
	InSamples = global.nsectors*CD_FRAMESAMPLES - current_offset/4;
	/* how many samples are wanted? */
	InSamples = min((*nSamplesToDo-nSamplesDone), InSamples);

	while ((nSamplesDone < *nSamplesToDo) && (InSamples != 0)) {
		long unsigned int how_much = InSamples;

		long int left_in_track;
		left_in_track  = Get_StartSector(current_track_writing+1) *
					CD_FRAMESAMPLES
					- (int)(BeginAtSample+nSamplesDone);

		if (*eorecording != 0 && current_track_writing == cdtracks+1 &&
		    (*total_segments_read) == (*total_segments_written)+1) {
			/*
			 * limit, if the actual end of the last track is
			 * not known from the toc.
			 */
			left_in_track = InSamples;
		}

		if (left_in_track < 0) {
			errmsgno(EX_BAD,
			_("internal error: negative left_in_track:%ld, current_track_writing=%d\n"),
				left_in_track, current_track_writing);
		}

		if (bulk) {
			how_much = min(how_much,
						(unsigned long) left_in_track);
		}

		if (SaveBuffer(p->data + current_offset/4,
						how_much,
						&nSamplesDone)) {
#ifdef	HAVE_KILL
			if (global.have_forked == 1) {
				pid_t	ppid;
				/*
				 * Kill the parent too if we are not orphaned.
				 */
				ppid = getppid();
				if (ppid > 1)
					kill(ppid, SIGINT);
			}
#endif
			exit(WRITE_ERROR);
		}

		global.nSamplesDoneInTrack += how_much;
		SamplesToWrite -= how_much;

		/* move residual samples upto buffer start */
		if (how_much < InSamples) {
			memmove(
				(char *)(p->data) + current_offset,
				(char *)(p->data) + current_offset +
				how_much * 4,
				(InSamples - how_much) * 4);
		}

		/* when track end is reached, close current file and start a new one */
		if ((unsigned long) left_in_track <= InSamples ||
		    SamplesToWrite == 0) {
			/*
			 * the current portion to be handled is
			 * the end of a track
			 */

			if (bulk) {
				/* finish sample file for this track */
				CloseAudio(global.channels,
					global.nSamplesDoneInTrack,
					global.audio_out);
			} else if (SamplesToWrite == 0) {
				/* finish sample file for this track */
				CloseAudio(global.channels,
					(unsigned int) *nSamplesToDo,
					global.audio_out);
			}
#ifdef INFOFILES
			if (global.no_infofile == 0) {
				write_md5_info(global.fname_base,
						current_track_writing,
						bulk && global.multiname == 0);
			}
#endif
			if (global.verbose) {
#ifdef	USE_PARANOIA
				double	f;
				double fc2 = 0.0;

				if (global.paranoia_mode & PARANOIA_MODE_C2CHECK)
					fc2 =  para_stat->c2secs * 100.0 /
						(para_stat->sectors * 1.0);
#endif
				print_percentage(&oper, current_offset);
				fputc(' ', outfp);

#ifndef	THOMAS_SCHAU_MAL
				if ((unsigned long)left_in_track > InSamples) {
					fputs(_(" incomplete"), outfp);
				}
#endif
				if (global.tracktitle[current_track_writing] != NULL) {
					fprintf(outfp,
					_(" track %2u '%s' recorded"),
					current_track_writing,
					global.tracktitle[current_track_writing]);
				} else {
					fprintf(outfp,
						_(" track %2u recorded"),
						current_track_writing);
				}
#ifdef	USE_PARANOIA
				oper = para_stat->readerrs + para_stat->skips +
						para_stat->fixup_edges +
						para_stat->fixup_atoms +
						para_stat->fixup_droppeds +
						para_stat->fixup_dupeds +
						para_stat->drifts;
				f = (100.0 * oper) /
				(para_stat->sectors * 1.0);

				if (para_stat->readerrs || para_stat->c2badsecs) {
					fprintf(outfp,
						_(" with audible hard errors"));
					fprintf(outfp,
						_(" %u c2badsecs"), para_stat->c2badsecs);
				} else if ((para_stat->skips) > 0) {
					fprintf(outfp,
						_(" with %sretry/skip errors"),
						f < 2.0 ? "":_("audible "));
				} else if (oper > 0) {
					oper = f;

					fprintf(outfp, _(" with "));
					if (oper < 4)
						fprintf(outfp, _("minor"));
					else if (oper < 16)
						fprintf(outfp, _("medium"));
					else if (oper < 67)
						fprintf(outfp,
							_("noticeable audible"));
					else if (oper < 100)
						fprintf(outfp,
							_("major audible"));
					else
						fprintf(outfp,
							_("extreme audible"));
					fprintf(outfp, _(" problems"));
				} else {
					fprintf(outfp, _(" successfully"));
				}
				if (f >= 0.1 || fc2 > 0.1) {
					fprintf(outfp, " (");
				}
				if (f >= 0.1) {
					fprintf(outfp,
						_("%.1f%% problem sectors"), f);
				}
				if (fc2 >= 0.1) {
					if (f >= 0.1)
						fprintf(outfp, ", ");
					fprintf(outfp,
						_("%.2f%% c2 sectors"), fc2);
				}
				if (f >= 0.1 || fc2 > 0.1) {
					fprintf(outfp, ")");
				}
#else
				fprintf(outfp, _(" successfully"));
#endif

				if (waitforsignal == 1) {
					fprintf(outfp,
					_(". %d silent samples omitted"),
						global.SkippedSamples);
				}
				fputs("\n", outfp);

				if (global.reads_illleadout &&
				    *eorecording == 1) {
					fprintf(outfp,
					_("Real lead out at: %ld sectors\n"),
					(*nSamplesToDo+BeginAtSample)/CD_FRAMESAMPLES);
				}
#ifdef	USE_PARANOIA
				if (global.paranoia_selected) {
					oper = 200;	/* force new output */
					print_percentage(&oper, current_offset);
					if (para_stat->minoverlap == 0x7FFFFFFF)
						para_stat->minoverlap = 0;
					fprintf(outfp,
						_("  %u rderr, %u skip, %u atom, %u edge, %u drop, %u dup, %u drift"),
						para_stat->readerrs,
						para_stat->skips,
						para_stat->fixup_atoms,
						para_stat->fixup_edges,
						para_stat->fixup_droppeds,
						para_stat->fixup_dupeds,
						para_stat->drifts);
					if (global.paranoia_mode & PARANOIA_MODE_C2CHECK) {
						fprintf(outfp,
							_(", %u %u c2\n"), para_stat->c2errs, para_stat->c2secs);
#ifdef	PARANOIA_DEBUG
						fprintf(outfp,
							", %u c2b", para_stat->c2bytes);
						fprintf(outfp,
							", %u c2s", para_stat->c2secs);
						fprintf(outfp,
							", %u c2m", para_stat->c2maxerrs);
						fprintf(outfp,
							", %u c2B\n", para_stat->c2badsecs);
#endif
					} else {
						fprintf(outfp, "\n");
					}
					oper = 200;	/* force new output */
					print_percentage(&oper, current_offset);
					fprintf(outfp,
						_("  %u reads(%.1f%%) %u overlap(%.4g .. %.4g)\n"),
						para_stat->reads,
						para_stat->sectors*1.0 /
						(global.nSamplesDoneInTrack/588.0/100.0),
						para_stat->overlaps,
						(float)para_stat->minoverlap /
						    (2352.0/2.0),
						(float)para_stat->maxoverlap /
						    (2352.0/2.0));
					paranoia_statreset();
				}
#endif
			}

			global.nSamplesDoneInTrack = 0;
			if (bulk && SamplesToWrite > 0) {
				if (!global.no_file) {
					char *tmp_fname;

					/* build next filename */
					tmp_fname = get_next_name();
					if (tmp_fname != NULL) {
						strncpy(global.fname_base,
							tmp_fname,
							sizeof (global.fname_base));
						global.fname_base[
							sizeof (global.fname_base)-1] =
							'\0';
					}

					cut_extension(global.fname_base);

					if (global.multiname == 0) {
						snprintf(fname, sizeof (fname),
							"%s_%02u.%s",
							global.fname_base,
							current_track_writing+1,
							audio_type);
					} else {
						snprintf(fname, sizeof (fname),
							"%s.%s",
							global.fname_base,
							audio_type);
					}

					OpenAudio(fname, rate, bits,
						global.channels,
						(Get_AudioStartSector(current_track_writing+1) -
						Get_AudioStartSector(current_track_writing))
						*CD_FRAMESIZE_RAW,
						global.audio_out);
				} /* global.nofile */
			} /* if (bulk && SamplesToWrite > 0) */
			current_track_writing++;

		} /* left_in_track <= InSamples */
		InSamples -= how_much;

	}  /* end while */
	if (!global.quiet && *nSamplesToDo != nSamplesDone) {
		print_percentage(&oper, current_offset);
	}
	return (nSamplesDone);
}

#define	PRINT_OVERLAP_INIT \
	if (global.verbose) { \
		if (global.overlap > 0) \
			fprintf(outfp, _("overlap:min/max/cur, jitter, percent_done:\n  /  /  /          0%%")); \
		else \
			fputs(_("percent_done:\n  0%"), outfp); \
	}

#if defined HAVE_FORK_AND_SHAREDMEM
LOCAL void forked_read __PR((void));

/*
 * This function does all audio cdrom reads
 * until there is nothing more to do
 */
LOCAL void
forked_read()
{
	unsigned	total_unsuccessful_retries = 0;

#if !defined(HAVE_SEMGET) || !defined(USE_SEMAPHORES)
	init_child();
#endif

	minover = global.nsectors;

	if (global.interactive) {
		int	r;

#ifdef	DEBUG_INTERACTIVE
		error("iloop %lu (%lu sects == %lu s) lSector %ld -> %ld\n",
			global.iloop, global.iloop/588, global.iloop/588/75,
			lSector, lSector_p2);
		error("*nSamplesToDo %ld nSamplesDone %ld\n",
			*nSamplesToDo, nSamplesDone);
#endif
		lSector = Get_StartSector(FirstAudioTrack());
		lSector_p2 = Get_LastSectorOnCd(cdtracks);
		set_newstart(lSector);

		r = parse(&lSector);
		if (r < 0) {
			set_nonforked(-1);
			/* NOTREACHED */
		}
		set_newstart(lSector);
	}

	PRINT_OVERLAP_INIT
	while (global.iloop) {
/*	while (global.interactive || global.iloop) {*/
/*
 * So blockiert es mit get_next_buffer() + get_oldest_buffer()
 * mit -interactive nach Ablauf des Auslesens des aktuellen Auftrags.
 */

		if (global.interactive && poll_in() > 0) {
			int	r = parse(&lSector);
			if (r < 0) {
				set_nonforked(-1);
				/* NOTREACHED */
			}
			set_newstart(lSector);
		}

		do_read(get_next_buffer(), &total_unsuccessful_retries);

		define_buffer();
	}

	if (total_unsuccessful_retries) {
		fprintf(stderr,
			_("%u unsuccessful matches while reading\n"),
			total_unsuccessful_retries);
	}
}

LOCAL void forked_write __PR((void));

LOCAL void
forked_write()
{

	/*
	 * don't need these anymore.  Good security policy says we get rid
	 * of them ASAP
	 */
	if (global.issetuid || global.uid != 0)
		priv_off();
	neverneedroot();
	neverneedgroup();

#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
#else
	init_parent();
#endif

	for (; global.interactive || nSamplesDone < *nSamplesToDo; ) {
		myringbuff	*oldest_buffer;

		if (*eorecording == 1 &&
		    (*total_segments_read) == (*total_segments_written))
			break;

		/*
		 * get oldest buffers
		 */
		oldest_buffer = get_oldest_buffer();
		if (oldest_buffer == NULL)
			break;
		nSamplesDone = do_write(oldest_buffer);

		drop_buffer();
	}
}
#endif

/*
 * This function implements the read and write calls in one loop (in case
 * there is no fork/thread_create system call).
 * This means reads and writes have to wait for each other to complete.
 */
LOCAL void nonforked_loop __PR((void));

LOCAL void
nonforked_loop()
{
	unsigned	total_unsuccessful_retries = 0;

	minover = global.nsectors;

	if (global.interactive) {
		int	r;

		lSector = Get_StartSector(FirstAudioTrack());
		lSector_p2 = Get_LastSectorOnCd(cdtracks);
		set_newstart(lSector);

		r = parse(&lSector);
		if (r < 0) {
			return;
		}
		set_newstart(lSector);
	}

	PRINT_OVERLAP_INIT
	while (global.iloop) {
		myringbuff	*oldest_buffer;

		if (global.interactive && poll_in() > 0) {
			int	r = parse(&lSector);
			if (r < 0) {
				break;
			}
			set_newstart(lSector);
		}

		do_read(get_next_buffer(), &total_unsuccessful_retries);

		oldest_buffer = get_oldest_buffer();
		if (oldest_buffer == NULL)
			break;
		do_write(oldest_buffer);
	}

	if (total_unsuccessful_retries) {
		fprintf(stderr, _("%u unsuccessful matches while reading\n"), total_unsuccessful_retries);
	}
}

LOCAL void verbose_usage __PR((void));

LOCAL void
verbose_usage()
{
	fputs(_("\
	help		lists all verbose options.\n\
	disable		disables verbose mode.\n\
	all		enables all verbose options.\n\
	toc		display the table of contents.\n\
	summary		display a summary of track parameters.\n\
	indices		retrieve/display index positions.\n\
	catalog		retrieve/display media catalog number.\n\
	mcn		retrieve/display media catalog number.\n\
	trackid		retrieve/display international standard recording code.\n\
	isrc		retrieve/display international standard recording code.\n\
	sectors		display the start sectors of each track.\n\
	titles		display any known track titles.\n\
	audio-tracks	list the audio tracks and their start sectors.\n\
"), stderr);
}

#ifdef	USE_PARANOIA
LOCAL void paranoia_usage __PR((void));

LOCAL void
paranoia_usage()
{
	/* BEGIN CSTYLED */
	fputs(_("\
	help		lists all paranoia options.\n\
	disable		disables paranoia mode. Paranoia is still being used.\n\
	no-verify	switches verify off, and overlap on.\n\
	retries=amount	set the number of maximum retries per sector.\n\
	readahead=amount set the number of sectors to use for the read ahead buffer.\n\
	overlap=amount	set the number of sectors used for statical overlap.\n\
	minoverlap=amt	set the min. number of sectors used for dynamic overlap.\n\
	maxoverlap=amt	set the max. number of sectors used for dynamic overlap.\n\
	c2check		check C2 pointers from drive to rate quality.\n\
	proof		alias: minoverlap=20,retries=200,readahead=600,c2check.\n\
"),
		stderr);
	/* END CSTYLED */
}
#endif

LOCAL int
handle_verbose_opts __PR((char *optstr, long *flagp));

LOCAL int
handle_verbose_opts(optstr, flagp)
	char	*optstr;
	long	*flagp;
{
	char	*ep;
	char	*np;
	int	optlen;
	BOOL	not = FALSE;

	*flagp = 0;
	while (*optstr) {
		if ((ep = strchr(optstr, ',')) != NULL) {
			optlen = ep - optstr;
			np = ep + 1;
		} else {
			optlen = strlen(optstr);
			np = optstr + optlen;
		}
		if (optstr[0] == '!') {
			optstr++;
			optlen--;
			not = TRUE;
		}
		if (strncmp(optstr, "not", optlen) == 0 ||
				strncmp(optstr, "!", optlen) == 0) {
			not = TRUE;
		} else if (strncmp(optstr, "toc", optlen) == 0) {
			*flagp |= SHOW_TOC;
		} else if (strncmp(optstr, "summary", optlen) == 0) {
			*flagp |= SHOW_SUMMARY;
		} else if (strncmp(optstr, "indices", optlen) == 0) {
			*flagp |= SHOW_INDICES;
		} else if (strncmp(optstr, "catalog", optlen) == 0) {
			*flagp |= SHOW_MCN;
		} else if (strncmp(optstr, "MCN", optlen) == 0) {
			*flagp |= SHOW_MCN;
		} else if (strncmp(optstr, "mcn", optlen) == 0) {
			*flagp |= SHOW_MCN;
		} else if (strncmp(optstr, "trackid", optlen) == 0) {
			*flagp |= SHOW_ISRC;
		} else if (strncmp(optstr, "ISRC", optlen) == 0) {
			*flagp |= SHOW_ISRC;
		} else if (strncmp(optstr, "isrc", optlen) == 0) {
			*flagp |= SHOW_ISRC;
		} else if (strncmp(optstr, "sectors", optlen) == 0) {
			*flagp |= SHOW_STARTPOSITIONS;
		} else if (strncmp(optstr, "titles", optlen) == 0) {
			*flagp |= SHOW_TITLES;
		} else if (strncmp(optstr, "audio-tracks", optlen) == 0) {
			*flagp |= SHOW_JUSTAUDIOTRACKS;
		} else if (strncmp(optstr, "all", optlen) == 0) {
			*flagp |= SHOW_MAX;
		} else if (strncmp(optstr, "disable", optlen) == 0) {
			*flagp = 0;
		} else if (strncmp(optstr, "help", optlen) == 0) {
			verbose_usage();
			exit(NO_ERROR);
		} else {
			char	*endptr;
			unsigned arg = strtoul(optstr, &endptr, 10);
			if (optstr != endptr &&
			    arg <= SHOW_MAX) {
				*flagp |= arg;
				errmsgno(EX_BAD,
					_("Warning: numerical parameters for -v are no more supported in the next releases!\n"));
			} else {
				errmsgno(EX_BAD,
					_("Unknown option '%s'.\n"), optstr);
				verbose_usage();
				exit(SYNTAX_ERROR);
			}
		}
		optstr = np;
	}
	if (not)
		*flagp = (~ *flagp) & SHOW_MAX;
	return (1);
}


LOCAL int
handle_paranoia_opts __PR((char *optstr, long *flagp));

LOCAL int
handle_paranoia_opts(optstr, flagp)
	char *optstr;
	long *flagp;
{
#ifdef	USE_PARANOIA
	char	*ep;
	char	*np;
	int	optlen;

	while (*optstr) {
		if ((ep = strchr(optstr, ',')) != NULL) {
			optlen = ep - optstr;
			np = ep + 1;
		} else {
			optlen = strlen(optstr);
			np = optstr + optlen;
		}
		if (strncmp(optstr, "retries=", min(8, optlen)) == 0) {
			char *eqp = strchr(optstr, '=');
			int   rets;

			astoi(eqp+1, &rets);
			if (rets >= 0) {
				global.paranoia_parms.retries = rets;
			}
		} else if (strncmp(optstr, "readahead=", min(10, optlen)) == 0) {
			char *eqp = strchr(optstr, '=');
			int   readahead;

			astoi(eqp+1, &readahead);
			if (readahead >= 0) {
				global.paranoia_parms.readahead = readahead;
			}
		} else if (strncmp(optstr, "overlap=", min(8, optlen)) == 0) {
			char *eqp = strchr(optstr, '=');
			int   rets;

			astoi(eqp+1, &rets);
			if (rets >= 0) {
				global.paranoia_parms.overlap = rets;
			}
		} else if (strncmp(optstr, "minoverlap=",
						min(11, optlen)) == 0) {
			char *eqp = strchr(optstr, '=');
			int   rets;

			astoi(eqp+1, &rets);
			if (rets >= 0) {
				global.paranoia_parms.mindynoverlap = rets;
			}
		} else if (strncmp(optstr, "maxoverlap=",
						min(11, optlen)) == 0) {
			char *eqp = strchr(optstr, '=');
			int   rets;

			astoi(eqp+1, &rets);
			if (rets >= 0) {
				global.paranoia_parms.maxdynoverlap = rets;
			}
		} else if (strncmp(optstr, "no-verify", optlen) == 0) {
			global.paranoia_parms.disable_extra_paranoia = 1;
		} else if (strncmp(optstr, "disable", optlen) == 0) {
			global.paranoia_parms.disable_paranoia = 1;
		} else if (strncmp(optstr, "c2check", optlen) == 0) {
			global.paranoia_parms.enable_c2_check = 1;
		} else if (strncmp(optstr, "help", optlen) == 0) {
			paranoia_usage();
			exit(NO_ERROR);
		} else if (strncmp(optstr, "proof", optlen) == 0) {
			*flagp = 1;
			global.paranoia_parms.mindynoverlap = -1;
			global.paranoia_parms.retries = 200;
			global.paranoia_parms.readahead = 600;
#define	__should_we__
#ifdef	__should_we__
			/*
			 * c2check may cause some drives to become unable
			 * to read hidden tracks.
			 */
			global.paranoia_parms.enable_c2_check = 1;
#endif
		} else {
			errmsgno(EX_BAD, _("Unknown option '%s'.\n"), optstr);
			paranoia_usage();
			exit(SYNTAX_ERROR);
		}
		optstr = np;
	}
	global.paranoia_selected = TRUE;
	return (1);
#else
	errmsgno(EX_BAD, _("lib paranoia support is not configured!\n"));
	return (0);
#endif
}


/*
 * and finally: the MAIN program
 */
EXPORT int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	long		lSector_p1;
	char		*env_p;
#if	defined(USE_NLS)
	char		*dir;
#endif
	int		tracks_included;

	audio_type = AUDIOTYPE;

#ifdef	HAVE_SOLARIS_PPRIV
	/*
	 * Try to gain additional privs on Solaris
	 */
	do_pfexec(argc, argv,
		PRIV_FILE_DAC_READ,
		PRIV_SYS_DEVICES,
		PRIV_PROC_PRIOCNTL,
		PRIV_NET_PRIVADDR,
		NULL);
#endif
	save_args(argc, argv);

#if	defined(USE_NLS)
	(void) setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "cdda2wav"	/* Use this only if it weren't */
#endif
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#if defined(PROTOTYPES) && defined(INS_BASE)
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
#endif

	/*
	 * init global variables
	 */
	init_globals();
	{
		int	am_i_cdda2wav;

		/*
		 * When being invoked as list_audio_tracks, just dump a list of
		 * audio tracks.
		 */
		am_i_cdda2wav = !(strlen(argv[0]) >= sizeof ("list_audio_tracks")-1 &&
				strcmp(argv[0]+strlen(argv[0])+1-sizeof ("list_audio_tracks"), "list_audio_tracks") == 0);
		if (!am_i_cdda2wav)
			global.verbose = SHOW_JUSTAUDIOTRACKS;
	}

	/*
	 * Control those set-id and privileges...
	 *
	 * At this point, we should have the needed privileges, either because:
	 *
	 *	1)	We have been called by a privileged user (eg. root)
	 *	2)	This is a suid-root process
	 *	3)	This is a process that did call pfexec to gain privs
	 *	4)	This is a process that has been called via pfexec
	 *	5)	This is a process that gained privs via fcaps
	 *
	 * Case (1) is the only case where whe should not give up privileges
	 * because people would not expect it and because there will be no
	 * privilege escalation in this process.
	 */
	global.uid = getuid();
	global.euid = geteuid();
#ifdef	HAVE_ISSETUGID
	global.issetuid = issetugid();
#else
	global.issetuid = global.uid != global.euid;
#endif
	if (global.issetuid || global.uid != 0) {
		/*
		 * If this is a suid-root process or if the real uid of
		 * this process is not root, we may have gained privileges
		 * from suid-root or pfexec and need to manage privileges in
		 * order to prevent privilege escalations for the user.
		 */
		priv_init();
	}
	initsecurity();

	env_p = getenv("CDDA_DEVICE");
	if (env_p != NULL) {
		global.dev_name = env_p;
	}

	env_p = getenv("CDDBP_SERVER");
	if (env_p != NULL) {
		global.cddbp_server = env_p;
	}

	env_p = getenv("CDDBP_PORT");
	if (env_p != NULL) {
		global.cddbp_port = env_p;
	}

	gargs(argc, argv);

	/*
	 * The check has been introduced as some Linux distributions miss the
	 * skills to perceive the necessity for the needed privileges. So we
	 * warn which features are impaired by actually missing privileges.
	 */
	if (global.issetuid || global.uid != 0)
		priv_on();
	needroot(0);
	if (!priv_eff_priv(SCHILY_PRIV_FILE_DAC_READ))
		priv_warn("file read", "You will not be able to open all needed devices.");
#ifndef	__SUNOS5
	/*
	 * Due to a design bug in the Solaris USCSI ioctl, we don't need
	 * PRIV_FILE_DAC_WRITE to send SCSI commands and most installations
	 * probably don't grant PRIV_FILE_DAC_WRITE. Once we need /dev/scg*,
	 * we would need to test for PRIV_FILE_DAC_WRITE also.
	 */
	if (!priv_eff_priv(SCHILY_PRIV_FILE_DAC_WRITE))
		priv_warn("file write", "You will not be able to open all needed devices.");
#endif
	if (!priv_eff_priv(SCHILY_PRIV_SYS_DEVICES))
		priv_warn("device",
		    "You may not be able to send all needed SCSI commands, this my cause various unexplainable problems.");
	if (!priv_eff_priv(SCHILY_PRIV_PROC_PRIOCNTL))
		priv_warn("priocntl", "You may get jitter.");
	if (!priv_eff_priv(SCHILY_PRIV_NET_PRIVADDR))
		priv_warn("network", "You will not be able to do remote SCSI.");
	if (global.issetuid || global.uid != 0)
		priv_off();
	dontneedroot();

#define	SETSIGHAND(PROC, SIG, SIGNAME) if (signal(SIG, PROC) == SIG_ERR) \
	{ errmsg(_("Cannot set signal %s handler.\n"), SIGNAME); exit(SETSIG_ERROR); }
#ifdef	SIGINT
	SETSIGHAND(exit_wrapper, SIGINT, "SIGINT")
#endif
#ifdef	SIGQUIT
	SETSIGHAND(exit_wrapper, SIGQUIT, "SIGQUIT")
#endif
#ifdef	SIGTERM
	SETSIGHAND(exit_wrapper, SIGTERM, "SIGTERM")
#endif
#ifdef	SIGHUP
	SETSIGHAND(exit_wrapper, SIGHUP, "SIGHUP")
#endif
#ifdef	SIGPIPE
	SETSIGHAND(set_nonforked, SIGPIPE, "SIGPIPE")
#endif

	/* setup interface and open cdrom device */
	/*
	 * request sychronization facilities and shared memory
	 */
	SetupInterface();

	/*
	 * use global.useroverlap to set our overlap
	 */
	if (global.useroverlap != -1)
		global.overlap = global.useroverlap;

	/*
	 * check for more valid option combinations
	 */
	if (global.nsectors < 1+global.overlap) {
		errmsgno(EX_BAD,
		_("Warning: Setting #nsectors to minimum of %d, due to jitter correction!\n"),
			global.overlap+1);
		global.nsectors = global.overlap+1;
	}

	if (global.overlap > 0 && global.buffers < 2) {
		errmsgno(EX_BAD,
		_("Warning: Setting #buffers to minimum of 2, due to jitter correction!\n"));
		global.buffers = 2;
	}

	/*
	 * Value of 'nsectors' must be defined here
	 */
	global.shmsize = 0;
#ifdef	USE_PARANOIA
	while (global.shmsize < sizeof (struct paranoia_statistics))
		global.shmsize += global.pagesize;
#endif
#ifdef	MD5_SIGNATURES
	{ int	i = 0;
		while (i < sizeof (MD5_CTX))
			i += global.pagesize;
		global.shmsize += i;
	}
#endif
	global.shmsize += 10*global.pagesize;	/* XXX Der Speicherfehler ist nicht in libparanoia sondern in cdda2wav :-( */
	global.shmsize += HEADER_SIZE + ENTRY_SIZE_PAGE_AL * global.buffers;

#if	defined(HAVE_FORK_AND_SHAREDMEM)
	/*
	 * The (void *) cast is to avoid a GCC warning like:
	 * warning: dereferencing type-punned pointer will break
	 * strict-aliasing rules
	 * which does not apply to this code. (void *) introduces a compatible
	 * intermediate type in the cast list.
	 */
	he_fill_buffer = request_shm_sem(global.shmsize,
				(unsigned char **)(void *)&he_fill_buffer);
	if (he_fill_buffer == NULL) {
		errmsgno(EX_BAD, _("No shared memory available!\n"));
		exit(SHMMEM_ERROR);
	}
#else /* do not have fork() and shared memory */
	he_fill_buffer = malloc(global.shmsize);
	if (he_fill_buffer == NULL) {
		errmsg(_("No buffer memory available!\n"));
		exit(NOMEM_ERROR);
	}
#endif
#ifdef	USE_PARANOIA
	{
		int	i = 0;
		char	*ptr = (char *)he_fill_buffer;

		para_stat = (struct paranoia_statistics *)he_fill_buffer;
		while (i < sizeof (struct paranoia_statistics)) {
			i		+= global.pagesize;
			ptr		+= global.pagesize;
			global.shmsize	-= global.pagesize;
		}
		he_fill_buffer = (myringbuff **)ptr;
	}
#endif
#ifdef	MD5_SIGNATURES
	{
		int	i = 0;
		char	*ptr = (char *)he_fill_buffer;

		global.context = (MD5_CTX*)he_fill_buffer;
		while (i < sizeof (MD5_CTX))
			i += global.pagesize;
		ptr		+= i;
		global.shmsize	-= i;

		he_fill_buffer = (myringbuff **)ptr;
	}
#endif

	if (global.verbose != 0) {
		fprintf(outfp,
		_("%u bytes buffer memory requested, transfer size %ld bytes, %u buffers, %u sectors\n"),
			global.shmsize, global.bufsize, global.buffers, global.nsectors);
	}

	/*
	 * initialize pointers into shared memory segment
	 */
	last_buffer = he_fill_buffer + 1;
	total_segments_read = (unsigned long *) (last_buffer + 1);
	total_segments_written = total_segments_read + 1;
	child_waits = (int *) (total_segments_written + 1);
	parent_waits = child_waits + 1;
	in_lendian = parent_waits + 1;
	eorecording = in_lendian + 1;
	*total_segments_read = *total_segments_written = 0;
	nSamplesToDo = (unsigned long *)(eorecording + 1);
	*eorecording = 0;
	*in_lendian = global.in_lendian;

	set_total_buffers(global.buffers, sem_id);


#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
	atexit(free_sem);
#endif

	/*
	 * set input endian default
	 */
	if (global.littleendian != -1)
		*in_lendian = global.littleendian;

	/*
	 * get table of contents
	 */
	cdtracks = ReadToc();
	if (cdtracks == 0) {
		errmsgno(EX_BAD,
			_("No track in table of contents! Aborting...\n"));
		exit(MEDIA_ERROR);
	} else if (global.maxtrack) {
		global.endtrack = cdtracks;
	}

	calc_cddb_id();
	calc_cdindex_id();

#if	1
	Check_Toc();
#endif

	if (ReadTocText != NULL && FirstAudioTrack() != -1) {
		ReadTocText(get_scsi_p());
		handle_cdtext();
		/*
		 * Starting from here, we cannot issue any SCSI command that
		 * overwrites the buffer used to read on the CD-Text data until
		 * FixupTOC() has been called.
		 */
	}
	if (global.verbose == SHOW_JUSTAUDIOTRACKS) {
		unsigned int z;

		/*
		 * XXX We did not check for hidden Track here
		 */
		for (z = 0; z <= cdtracks; z++) {
			if (Get_Datatrack(z) == 0) {
				printf("%02d\t%06ld\n",
					Get_Tracknumber(z),
					Get_AudioStartSector(z));
			}
		}
		exit(NO_ERROR);
	}

	if (global.verbose != 0) {
		fputs(_("#Cdda2wav version "), outfp);
		fputs(VERSION, outfp);
		fputs(VERSION_OS, outfp);
#if defined USE_POSIX_PRIORITY_SCHEDULING || defined HAVE_SYS_PRIOCNTL_H
		fputs(_(", real time sched."), outfp);
#endif
#if defined ECHO_TO_SOUNDCARD
		fputs(_(", soundcard"), outfp);
#endif
#if defined USE_PARANOIA
		fputs(_(", libparanoia"), outfp);
#endif
		fputs(_(" support\n"), outfp);
	}

	/*
	 * Also handles the CD-Text or CD-Extra information that has been read
	 * avove with ReadTocText().
	 */
	FixupTOC(cdtracks + 1);

	if (track == (unsigned int)-1) {
		if (useHiddenTrack() && (bulk || global.alltracks))
			global.endtrack = track = 0;
		else
			global.endtrack = track = 1;
		if (bulk || global.alltracks)
			global.endtrack = cdtracks;
	}

#if	0
	if (!global.paranoia_selected) {
		error("NICE\n");
		/*
		 * try to get some extra kicks
		 */
		if (global.issetuid || global.uid != 0)
			priv_on();
		needroot(0);
#if defined HAVE_SETPRIORITY
		setpriority(PRIO_PROCESS, 0, -20);
#else
#if defined(HAVE_NICE) && (HAVE_NICE == 1)
		nice(-NZERO);
#endif
#endif
		if (global.issetuid || global.uid != 0)
			priv_off();
		dontneedroot();
	}
#endif

	/*
	 * switch cdrom to audio mode
	 */
	EnableCdda(get_scsi_p(), 1, CD_FRAMESIZE_RAW);

	atexit(CloseAll);

	DisplayToc();
	if (FirstAudioTrack() == -1) {
		if (no_disguised_audiotracks()) {
			FatalError(EX_BAD,
				_("This disk has no audio tracks.\n"));
		}
	}

	/*
	 * check if start track is in range
	 */
	if (track < 1 || track > cdtracks) {
		if (!(track == 0 && useHiddenTrack()))
			usage2(_("Incorrect start track setting: %d\n"), track);
	}

	/*
	 * check if end track is in range
	 */
	if (global.endtrack < track || global.endtrack > cdtracks) {
		usage2(_("Incorrect end track setting: %ld\n"), global.endtrack);
	}

	/*
	 * Find track that is related to the absolute sector offset.
	 */
	if (global.start_sector != -1) {
		int	t;

		for (t = 1; t <= cdtracks; t++) {
			lSector = Get_AudioStartSector(t);
			lSector_p1 = Get_EndSector(t) + 1;
			if (lSector < 0)
				continue;
			if (global.start_sector >= lSector &&
			    global.start_sector < lSector_p1) {
				track = t;
				global.sector_offset = global.start_sector - lSector;
			} else if (t == 1 && useHiddenTrack() &&
				global.start_sector >= 0 &&
				global.start_sector < lSector) {
				lSector = global.start_sector;
				track = 0;
			}
			if (bulk || global.alltracks)
				global.endtrack = t;
		}
	}

	do {
		if (track == 0) {
			lSector = 0;
			lSector_p1 = Get_AudioStartSector(1);
		} else {
			lSector = Get_AudioStartSector(track);
			lSector_p1 = Get_EndSector(track) + 1;
		}
		if (lSector < 0) {
			if (bulk == 0 && !global.alltracks) {
				FatalError(EX_BAD,
					_("Track %u not found.\n"), track);
			} else {
				fprintf(outfp,
					_("Skipping data track %u...\n"), track);
				if (global.endtrack == track)
					global.endtrack++;
				track++;
			}
		}
	} while ((bulk != 0 || global.alltracks) && track <= cdtracks && lSector < 0);

	Read_MCN_ISRC(track, global.endtrack);

	if ((global.illleadout_cd == 0 || global.reads_illleadout != 0) &&
	    global.cd_index != -1) {
		if (global.verbose && !global.quiet) {
			global.verbose |= SHOW_INDICES;
		}
		global.sector_offset += ScanIndices(track, global.cd_index, bulk || global.alltracks);
	} else {
		global.cd_index = 1;
		if (global.deemphasize || (global.verbose & SHOW_INDICES)) {
			ScanIndices(track, global.cd_index, bulk || global.alltracks);
		}
	}

	lSector += global.sector_offset;
	/*
	 * check against end sector of track
	 */
	if (lSector >= lSector_p1) {
		fprintf(stderr,
		_("W Sector offset %ld exceeds track size (ignored)\n"),
			global.sector_offset);
		lSector -= global.sector_offset;
	}

	if (lSector < 0L) {
		fputs(_("Negative start sector! Set to zero.\n"), stderr);
		lSector = 0L;
	}

	lSector_p2 = Get_LastSectorOnCd(track);
	if ((bulk == 1 || global.alltracks) && track == global.endtrack && global.rectime == 0.0)
		global.rectime = 99999.0;
	if (global.rectime == 0.0) {
		/*
		 * set time to track time
		 */
		 *nSamplesToDo = (lSector_p1 - lSector) * CD_FRAMESAMPLES;
		global.rectime = (lSector_p1 - lSector) / 75.0;
		if (CheckTrackrange(track, global.endtrack) == 1) {
			lSector_p2 = Get_EndSector(global.endtrack) + 1;

			if (lSector_p2 >= 0) {
				global.rectime = (lSector_p2 - lSector) / 75.0;
				*nSamplesToDo = (long)(global.rectime*44100.0 + 0.5);
			} else {
				fputs(
				_("End track is no valid audio track (ignored)\n"),
					stderr);
			}
		} else {
			fputs(
			_("Track range does not consist of audio tracks only (ignored)\n"),
				stderr);
		}
	} else {
		/*
		 * Prepare the maximum recording duration.
		 * It is defined as the biggest amount of
		 * adjacent audio sectors beginning with the
		 * specified track/index/offset.
		 */
		if (global.rectime > (lSector_p2 - lSector) / 75.0) {
			global.rectime = (lSector_p2 - lSector) / 75.0;
			lSector_p1 = lSector_p2;
		}

		/* calculate # of samples to read */
		*nSamplesToDo = (long)(global.rectime*44100.0 + 0.5);
	}

	global.OutSampleSize = (1+bits/12);
	if (*nSamplesToDo/undersampling == 0L) {
		usage2(_("Time interval is too short. Choose a duration greater than %d.%02d secs!\n"),
			undersampling/44100, (int)(undersampling/44100) % 100);
	}
	if (global.moreargs < argc) {
		if (strcmp(argv[global.moreargs], "-") == 0 ||
		    is_fifo(argv[global.moreargs])) {
			/*
			 * pipe mode
			 */
			if (bulk == 1) {
				fprintf(stderr,
				_("W Bulk mode is disabled while outputting to a %spipe\n"),
					is_fifo(argv[global.moreargs]) ?
							"named " : "");
				bulk = 0;
			}
			global.no_cddbfile = 1;
		}
	}
	if (global.no_infofile == 0) {
		global.no_infofile = 1;
		if (global.channels == 1 || bits != 16 || rate != 44100) {
			fprintf(stderr,
			_("W Sample conversions disable generation of info files!\n"));
		} else if (waitforsignal == 1) {
			fprintf(stderr,
			_("W Option -w 'wait for signal' disables generation of info files!\n"));
		} else if (global.alltracks) {
			fprintf(stderr,
			_("W Option -tall 'all tracks into one file' disables generation of info files!\n"));
		} else if (global.sector_offset != 0) {
			fprintf(stderr,
			_("W Using an start offset (option -o) disables generation of info files!\n"));
		} else if (!bulk && !global.alltracks &&
			    !((lSector == Get_AudioStartSector(track)) &&
			    ((long)(lSector + global.rectime*75.0 + 0.5) ==
			    Get_EndSector(global.endtrack) + 1))) {
			fprintf(stderr,
			_("W Duration is not set for complete tracks (option -d), this disables generation\n  of info files!\n"));
		} else {
			global.no_infofile = 0;
		}
	}
	if (global.cuefile) {
		if (global.outputendianess != NONE) {
			fprintf(stderr,
			_("W Option -E 'outout endianess' disables generation of cue file!\n"));
			global.cuefile = 0;
		} else if (!global.alltracks) {
			fprintf(stderr,
			_("W Not selecting all tracks disables generation of cue file!\n"));
			global.cuefile = 0;
		}
	}

	SamplesToWrite = *nSamplesToDo*2/(int)global.int_part;

	{
		int first = FirstAudioTrack();

		tracks_included = Get_Track(
				(unsigned) (lSector +
					    *nSamplesToDo/CD_FRAMESAMPLES -1))
					    - max((int)track, first) +1;
	}

	if (global.multiname != 0 && global.moreargs + tracks_included > argc) {
		global.multiname = 0;
	}

	if (!waitforsignal) {

#ifdef INFOFILES
		if (!global.no_infofile) {
			int i;

			for (i = track; i < (int)track + tracks_included; i++) {
				unsigned minsec, maxsec;
				char *tmp_fname;

				/*
				 * build next filename
				 */
				tmp_fname = get_next_name();
				if (tmp_fname != NULL)
					strncpy(global.fname_base, tmp_fname,
						sizeof (global.fname_base)-8);
				global.fname_base[sizeof (global.fname_base)-1] = 0;
				minsec = max(lSector, Get_AudioStartSector(i));
				maxsec = min(lSector + global.rectime*75.0 + 0.5,
							1+Get_EndSector(i));
				if ((int)minsec == Get_AudioStartSector(i) &&
				    (int)maxsec == 1+Get_EndSector(i)) {
					write_info_file(global.fname_base, i,
						(maxsec-minsec)*CD_FRAMESAMPLES,
						bulk && global.multiname == 0);
				} else {
					fprintf(stderr,
					_("Partial length copy for track %d, no info file will be generated for this track!\n"), i);
				}
				if (!bulk)
					break;
			}
			reset_name_iterator();
		}
#endif
		if (global.cuefile) {
			int i;
			char *tmp_fname;
			FILE	*cuef;

			/*
			 * build next filename
			 */
			tmp_fname = get_next_name();
			if (tmp_fname != NULL)
				strncpy(global.fname_base, tmp_fname,
					sizeof (global.fname_base)-8);
			global.fname_base[sizeof (global.fname_base)-1] = 0;
			cut_extension(global.fname_base);

			cuef = cue_file_open(global.fname_base);
			write_cue_global(cuef, global.fname_base);

			for (i = track; i < (int)track + tracks_included; i++) {
				unsigned minsec, maxsec;

				minsec = max(lSector, Get_AudioStartSector(i));
				maxsec = min(lSector + global.rectime*75.0 + 0.5,
							1+Get_EndSector(i));
				if ((int)minsec == Get_AudioStartSector(i) &&
				    (int)maxsec == 1+Get_EndSector(i)) {
					write_cue_track(cuef, global.fname_base, i);
				}

				/*
				 * build next filename
				 */
				tmp_fname = get_next_name();
				if (tmp_fname != NULL)
					strncpy(global.fname_base, tmp_fname,
						sizeof (global.fname_base)-8);
				global.fname_base[sizeof (global.fname_base)-1] = 0;
				cut_extension(global.fname_base);
			}
			reset_name_iterator();
			if (cuef)
				fclose(cuef);
		}
	}

	if (global.just_the_toc)
		exit(NO_ERROR);

#ifdef  ECHO_TO_SOUNDCARD
	if (global.user_sound_device[0] != '\0') {
		set_snd_device(global.user_sound_device);
	}
	if (global.no_fork)
		init_soundcard(rate, bits);
#endif /* ECHO_TO_SOUNDCARD */

	if (global.userspeed > -1)
		global.speed = global.userspeed;

	if (global.speed != 0 && SelectSpeed != NULL) {
		SelectSpeed(get_scsi_p(), global.speed);
	}

	current_track_reading = current_track_writing = track;

	if (!global.no_file) {
		{
			char *myfname;

			myfname = get_next_name();

			if (myfname != NULL) {
				strncpy(global.fname_base, myfname,
						sizeof (global.fname_base)-8);
				global.fname_base[sizeof (global.fname_base)-1] = 0;
			}
		}

		/* strip audio_type extension */
		cut_extension(global.fname_base);
		if (bulk && global.multiname == 0) {
			sprintf(fname, "%s_%02u.%s",
				global.fname_base, current_track_writing, audio_type);
		} else {
			sprintf(fname, "%s.%s", global.fname_base, audio_type);
		}

		OpenAudio(fname, rate, bits, global.channels,
			(unsigned)(SamplesToWrite*global.OutSampleSize*global.channels),
			global.audio_out);
	}

	global.Remainder = (75 % global.nsectors)+1;

	global.sh_bits = 16 - bits;		/* shift counter */

	global.iloop = *nSamplesToDo;
	if (Halved && (global.iloop&1))
		global.iloop += 2;

	BeginAtSample = lSector * CD_FRAMESAMPLES;

#if	1
	if ((global.verbose & SHOW_SUMMARY) && !global.just_the_toc &&
	    (global.reads_illleadout == 0 ||
	    lSector+*nSamplesToDo/CD_FRAMESAMPLES
	    <= (unsigned) Get_AudioStartSector(cdtracks-1))) {

		fprintf(outfp, _("samplefile size will be %lu bytes.\n"),
			global.audio_out->GetHdrSize() +
			global.audio_out->InSizeToOutSize(SamplesToWrite*global.OutSampleSize*global.channels));
		fprintf(outfp,
		_("recording %d.%04d seconds %s with %d bits @ %5d.%01d Hz"),
			(int)global.rectime, (int)(global.rectime * 10000) % 10000,
			global.channels == 1 ? _("mono"):_("stereo"),
			bits, (int)rate, (int)(rate*10)%10);
		if (!global.no_file && *global.fname_base)
			fprintf(outfp, " ->'%s'...", global.fname_base);
		fputs("\n", outfp);
	}
#endif

#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
#else
	init_pipes();
#endif

#ifdef	USE_PARANOIA
	if (global.paranoia_selected) {
		long paranoia_mode;

		paranoia_mode = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;

		if (global.paranoia_parms.disable_paranoia) {
			paranoia_mode = PARANOIA_MODE_DISABLE;
		}
		if (global.paranoia_parms.disable_extra_paranoia) {
			paranoia_mode |= PARANOIA_MODE_OVERLAP;
			paranoia_mode &= ~PARANOIA_MODE_VERIFY;
		}
		/* not yet implemented */
		if (global.paranoia_parms.disable_scratch_detect) {
			paranoia_mode &= ~(PARANOIA_MODE_SCRATCH|PARANOIA_MODE_REPAIR);
		}
		/* not yet implemented */
		if (global.paranoia_parms.disable_scratch_repair) {
			paranoia_mode &= ~PARANOIA_MODE_REPAIR;
		}
		if (global.paranoia_parms.enable_c2_check) {
			/* test if we can read C2 with ReadCdRom_C2() */
			char buffer[3000];
			cdda_read_c2(get_scsi_p(), buffer, Get_StartSector(1), 1);
			if (ReadCdRom_C2 == NULL) {
				if (global.verbose)
					fprintf(outfp, _("c2check not supported by drive.\n"));
			} else {
				if (global.verbose)
					fprintf(outfp, _("using c2check to verify reads.\n"));
				paranoia_mode |= PARANOIA_MODE_C2CHECK;
			}
		}

		global.cdp = paranoia_init(get_scsi_p(), global.nsectors,
				(paranoia_mode & PARANOIA_MODE_C2CHECK) ?
				cdda_read_c2 : cdda_read,
				cdda_disc_firstsector, cdda_disc_lastsector,
				cdda_tracks,
				cdda_track_firstsector, cdda_track_lastsector,
				cdda_sector_gettrack, cdda_track_audiop);

		if (global.paranoia_parms.overlap >= 0) {
			int	overlap = global.paranoia_parms.overlap;

			if (overlap > global.nsectors - 1)
				overlap = global.nsectors - 1;
			paranoia_overlapset(global.cdp, overlap);
		}
		/*
		 * Implement the shortcut "paraopts=proof"
		 */
		if ((global.paranoia_flags & 1) &&
		    global.paranoia_parms.mindynoverlap < 0) {
			if (global.nsectors > 20)
				global.paranoia_parms.mindynoverlap = 20;
			else
				global.paranoia_parms.mindynoverlap = global.nsectors - 1;
		}
		/*
		 * Default to a  minimum of dynamic overlapping == 0.5 sectors.
		 * If we don't do this, we get the default from libparanoia
		 * which is approx. 0.1.
		 */
		if (global.paranoia_parms.mindynoverlap < 0)
			paranoia_dynoverlapset(global.cdp, CD_FRAMEWORDS/2, -1);
		paranoia_dynoverlapset(global.cdp,
			global.paranoia_parms.mindynoverlap * CD_FRAMEWORDS,
			global.paranoia_parms.maxdynoverlap * CD_FRAMEWORDS);

		paranoia_modeset(global.cdp, paranoia_mode);
		global.paranoia_mode = paranoia_mode;

		if (global.paranoia_parms.readahead < 0) {
			global.paranoia_parms.readahead = paranoia_get_readahead(global.cdp);
			if (global.paranoia_parms.readahead < 400) {
				global.paranoia_parms.readahead = 400;
				paranoia_set_readahead(global.cdp, 400);
			}
		} else {
			paranoia_set_readahead(global.cdp, global.paranoia_parms.readahead);
		}
		if (global.verbose)
			fprintf(outfp, _("using lib paranoia for reading.\n"));
		paranoia_seek(global.cdp, lSector, SEEK_SET);
		paranoia_statreset();
	}
#endif
#if defined(HAVE_FORK_AND_SHAREDMEM)

	/*
	 * Linux comes with a broken libc that makes "stderr" buffered even
	 * though POSIX requires "stderr" to be never "fully buffered".
	 * As a result, we would get garbled output once our fork()d child
	 * calls exit(). We work around the Linux bug by calling fflush()
	 * before fork()ing.
	 */
	fflush(outfp);

	/*
	 * Everything is set up. Now fork and let one process read cdda sectors
	 * and let the other one store them in a wav file
	 */

	/* forking */
	if (!global.no_fork)
		global.child_pid = fork();

	if (global.child_pid > 0 && global.gui > 0 && global.verbose > 0)
		fprintf(stderr, _("child pid is %lld\n"), (Llong)global.child_pid);

	/* ********************** fork ************************************* */
	if (global.child_pid == 0) {
		/* child WRITER section */

#ifdef  ECHO_TO_SOUNDCARD
		init_soundcard(rate, bits);
#endif /* ECHO_TO_SOUNDCARD */
#ifdef	HAVE_AREAS
		/*
		 * Under BeOS a fork() with shared memory does not work as
		 * it does under System V Rel. 4. The mapping of the child
		 * works with copy on write semantics, so changes do not
		 * propagate back and forth. The existing mapping has to be
		 * deleted and replaced by an clone without copy on write
		 * semantics.
		 * This is done with clone_area(..., B_ANY_ADDRESS,...).
		 * Thanks to file support.c from the postgreSQL project.
		 */
		area_info inf;
		int32 cook = 0;
		/*
		 * iterate over all mappings to find our shared memory mapping.
		 */
		while (get_next_area_info(0, &cook, &inf) == B_OK) {
			/* check the name of the mapping. */
			if (strcmp(inf.name, AREA_NAME) == 0) {
				void *area_address;
				area_id area_parent;

				/* kill the cow mapping. */
				area_address = inf.address;
				if (B_OK != delete_area(inf.area)) {
					errmsgno(EX_BAD,
					_("delete_area: no valid area.\n"));
					exit(SHMMEM_ERROR);
				}
				/* get the parent mapping. */
				area_parent = find_area(inf.name);
				if (area_parent == B_NAME_NOT_FOUND) {
					errmsgno(EX_BAD,
					_("find_area: no such area name.\n"));
					exit(SHMMEM_ERROR);
				}
				/*
				 * clone the parent mapping without cow.
				 * The original implementaion used
				 * B_ANY_ADDRESS, but newer Haiku versions
				 * implement address randomization that
				 * prevents us from using the pointer in the
				 * child. So we now use B_EXACT_ADDRESS.
				 */
				if (B_OK > clone_area("shm_child",
				    &area_address, B_EXACT_ADDRESS,
				    B_READ_AREA | B_WRITE_AREA, area_parent)) {
					errmsgno(EX_BAD,
						_("clone_area failed\n"));
					exit(SHMMEM_ERROR);
				}
			}
		}
#endif
#ifdef	__EMX__
		if (DosGetSharedMem(he_fill_buffer, 3)) {
			comerr(_("DosGetSharedMem() failed.\n"));
		}
#endif
		global.have_forked = 1;
		forked_write();
#ifdef	__EMX__
		DosFreeMem(he_fill_buffer);
		_exit(NO_ERROR);
		/* NOTREACHED */
#endif
		exit_wrapper(NO_ERROR);
		/* NOTREACHED */
	} else if (global.child_pid > 0) {
		/* parent READER section */

		global.have_forked = 1;
		switch_to_realtime_priority();

		forked_read();
#ifdef	HAVE_AREAS
		{
			area_id aid;
			aid = find_area(AREA_NAME);
			if (aid < B_OK) {
				comerrno(aid, _("find_area() failed.\n"));
			}
			delete_area(aid);
		}
#endif
#ifdef	__EMX__
		DosFreeMem(he_fill_buffer);
#endif
		exit_wrapper(NO_ERROR);
		/* NOTREACHED */
	} else {
		if (global.child_pid != -2)
			errmsg(_("Cannot fork.\n"));
	}

#endif	/* defined(HAVE_FORK_AND_SHAREDMEM) */

	/* version without fork */
	{
		global.have_forked = 0;
#if	0
		if (!global.paranoia_selected) {
			error("REAL\n");
			switch_to_realtime_priority();
		}
#endif
		if (!global.quiet)
			fprintf(stderr, _("a nonforking version is running...\n"));
		nonforked_loop();
		exit_wrapper(NO_ERROR);
		/* NOTREACHED */
	}
#ifdef	USE_PARANOIA
	if (global.paranoia_selected)
		paranoia_free(global.cdp);
#endif

	return (0);
}

LOCAL void
priv_warn(what, msg)
	const char	*what;
	const char	*msg;
{
	errmsgno(EX_BAD, "Insufficient '%s' privileges. %s\n", what, msg);
}

LOCAL void
gargs(argc, argv)
	int	argc;
	char	*argv[];
{
	int	cac;
	char	*const*cav;

	BOOL	version = FALSE;
	BOOL	help = FALSE;
	char	*channels = NULL;
	int	irate = -1;
	char	*divider = NULL;
	char	*trackspec = NULL;
	char	*duration = NULL;
	char	*int_name = DEF_INTERFACE;

	char	*oendianess = NULL;
	char	*cendianess = NULL;
	int	cddbp = -1;
	BOOL	stereo = FALSE;
	BOOL	mono = FALSE;
	BOOL	domax = FALSE;
	BOOL	dump_rates = FALSE;
	BOOL	md5blocksize = FALSE;
	long	userverbose = -1;
	int	outfd = -1;
	int	audiofd = -1;

	cac = argc;
	cav = argv;
	cac--;
	cav++;
	if (getargs(&cac, &cav, opts,
			&global.paranoia_selected,
			handle_paranoia_opts, &global.paranoia_flags,
			&version,
			&help, &help,

			&global.no_file, &global.no_file,

			&dump_rates, &dump_rates,
			&bulk, &bulk, &bulk,
			&global.scsi_verbose, &global.scsi_verbose,

			&global.findminmax, &global.findminmax,
			&global.findmono, &global.findmono,
			&global.no_infofile, &global.no_infofile,
			&global.no_textdefaults,
			&global.no_textfile,
			&global.cuefile,
			&global.no_hidden_track,

			&global.deemphasize, &global.deemphasize,
			&global.just_the_toc, &global.just_the_toc,
			&global.scsi_silent, &global.scsi_silent,

			&global.cddbp_server, &global.cddbp_port,
			&global.scanbus,
			&global.dev_name, &global.dev_name, &global.dev_name,
			&global.dev_opts,
			&global.scsi_debug, &global.scsi_debug,
			&global.scsi_kdebug, &global.scsi_kdebug, &global.scsi_kdebug,
			getnum, &global.bufsize,
			&global.aux_name, &global.aux_name,
			&int_name, &int_name,
			&audio_type, &audio_type,

			&oendianess, &oendianess,
			&cendianess, &cendianess,
			&global.userspeed, &global.userspeed,

			&global.playback_rate, &global.playback_rate,
			&md5blocksize, &md5blocksize,
			&global.useroverlap, &global.useroverlap,
			&global.user_sound_device, &global.user_sound_device,

			&cddbp, &cddbp,
			&channels, &channels,
			&bits, &bits,
			&irate, &irate,
			&global.gui, &global.gui,

			&divider, &divider,
			&trackspec, &trackspec,
			&global.cd_index, &global.cd_index,
			&duration, &duration,
			&global.sector_offset, &global.sector_offset,
			&global.start_sector,

			&global.nsectors, &global.nsectors,
			handle_verbose_opts, &userverbose,
			handle_verbose_opts, &userverbose,
			&global.buffers, &global.buffers,

			&stereo, &stereo,
			&mono, &mono,
			&waitforsignal, &waitforsignal,
			&global.echo, &global.echo,
			&global.quiet, &global.quiet,
			&domax, &domax, &outfd, &audiofd,
			&global.no_fork, &global.interactive) < 0) {
		errmsgno(EX_BAD, _("Bad Option: %s.\n"), cav[0]);
		fputs(_("Use 'cdda2wav -help' to get more information.\n"),
				stderr);
		exit(SYNTAX_ERROR);
	}
	if (getfiles(&cac, &cav, opts) == 0)
		/* No more file type arguments */;
	global.moreargs = cav - argv;
	if (version) {
#ifdef	OLD_VERSION_PRINT
		fputs(_("cdda2wav version "), outfp);
		fputs(VERSION, outfp);
		fputs(VERSION_OS, outfp);
		fputs("\n", outfp);
#else
		/*
		 * Make the version string similar for all cdrtools programs.
		 */
		printf(_("cdda2wav %s (%s-%s-%s) Copyright (C) 1993-2004,2015,2017 %s (C) 2004-2017 %s\n"),
					VERSION,
					HOST_CPU, HOST_VENDOR, HOST_OS,
					_("Heiko Eissfeldt"),
					_("Joerg Schilling"));
		prdefaults(stdout);
#endif
		exit(NO_ERROR);
	}
	if (help) {
		usage();
	}
	if (outfd >= 0) {
#ifdef	F_GETFD
		if (fcntl(outfd, F_GETFD, 0) < 0)
			comerr(_("Cannot redirect output to fd %d.\n"), outfd);
#endif
		global.out_fp = fdopen(outfd, "wa");
		if (global.out_fp == NULL)
			comerr(_("Cannot open output fd %d.\n"), outfd);
#ifdef	HAVE_SETVBUF
		setvbuf(global.out_fp, NULL, _IONBF, 0);
#else
#ifdef	HAVE_SETVBUF
		setbuf(global.out_fp, NULL);
#endif
#endif
	}
	if (!global.scanbus)
		cdr_defaults(&global.dev_name, NULL, NULL, &global.bufsize, NULL);
	if (global.bufsize < 0L)
		global.bufsize = DEF_BUFSIZE;	/* The SCSI buffer size */

	if (dump_rates) {	/* list available rates */
		int	ii;

		/* BEGIN CSTYLED */
		fputs(_("\
Available rates are:\n\
Rate   Divider      Rate   Divider      Rate   Divider      Rate   Divider\n\
"),
			outfp);
		/* END CSTYLED */
		for (ii = 1; ii <= 44100 / 880 / 2; ii++) {
			long i2 = ii;
			fprintf(outfp, "%7.1f  %2ld         %7.1f  %2ld.5       ",
				44100.0/i2, i2, 44100.0/(i2+0.5), i2);
			i2 += 25;
			fprintf(outfp, "%7.1f  %2ld         %7.1f  %2ld.5\n",
				44100.0/i2, i2, 44100.0/(i2+0.5), i2);
			i2 -= 25;
		}
		exit(NO_ERROR);
	}
	if (channels) {
		if (*channels == 's') {
			global.channels = 2;
			global.swapchannels = 1;
		} else {
			global.channels = strtol(channels, NULL, 10);
		}
	}
	if (irate >= 0) {
		rate = irate;
	}
	if (divider) {
		double divider_d;
		divider_d = strtod(divider, NULL);
		if (divider_d > 0.0) {
			rate = 44100.0 / divider_d;
		} else {
			errmsgno(EX_BAD,
			_("E option -divider requires a nonzero, positive argument.\nSee -dump-rates.\n"));
			exit(SYNTAX_ERROR);
		}
	}
	if (global.sector_offset != 0 && global.start_sector != -1) {
		errmsgno(EX_BAD, _("-offset and -start-sector are mutual exclusive.\n"));
		exit(SYNTAX_ERROR);
	}
	if (trackspec) {
		char * endptr;
		char * endptr2;

		if (streql(trackspec, "all")) {
			global.alltracks = TRUE;
		} else {
			track = strtoul(trackspec, &endptr, 10);
			if (trackspec == endptr)
				comerrno(EX_BAD,
					_("Invalid track specification '%s'.\n"),
					trackspec);
			if (streql(endptr, "+max")) {
				if (track <= 1) {
					global.alltracks = TRUE;
					/*
					 * Hack for write_cue_track() to
					 * use correct INDEX offsets.
					 */
					if (track == 1)
						global.no_hidden_track = TRUE;
				}
				global.maxtrack = TRUE;
				endptr2 = endptr;
			} else {
				global.endtrack = strtoul(endptr, &endptr2, 10);
			}
			if (endptr2 == endptr) {		/* endtrack empty */
				global.endtrack = track;
			} else if (track == global.endtrack) {	/* manually -tn+n */
				bulk = -1;
			}
		}
		if (global.start_sector != -1 && !global.alltracks) {
			errmsgno(EX_BAD, _("-t and -start-sector are mutual exclusive.\n"));
			exit(SYNTAX_ERROR);
		}
	}
	if (duration) {
		char *end_ptr = NULL;
		global.rectime = strtod(duration, &end_ptr);
		if (*end_ptr == 'f') {
			global.rectime = global.rectime / 75.0;
			/* TODO: add an absolute end of recording. */
#if	0
		} else if (*end_ptr == 'F') {
			global.rectime = global.rectime / 75.0;
#endif
		} else if (*end_ptr != '\0') {
			global.rectime = -1.0;
		}
	}
	if (oendianess) {
		if (strcasecmp(oendianess, "little") == 0) {
			global.outputendianess = LITTLE;
		} else if (strcasecmp(oendianess, "big") == 0) {
			global.outputendianess = BIG;
		} else if (strcasecmp(oendianess, "machine") == 0 ||
			    strcasecmp(oendianess, "host") == 0) {
#ifdef	WORDS_BIGENDIAN
			global.outputendianess = BIG;
#else
			global.outputendianess = LITTLE;
#endif
		} else {
			usage2(_("Wrong parameter '%s' for option -E\n"), oendianess);
		}
	}
	if (cendianess) {
		if (strcasecmp(cendianess, "little") == 0) {
			global.littleendian = 1;
		} else if (strcasecmp(cendianess, "big") == 0) {
			global.littleendian = 0;
		} else if (strcasecmp(cendianess, "machine") == 0 ||
			    strcasecmp(cendianess, "host") == 0) {
#ifdef	WORDS_BIGENDIAN
			global.littleendian = 0;
#else
			global.littleendian = 1;
#endif
		} else if (strcasecmp(cendianess, "guess") == 0) {
			global.littleendian = -2;
		} else {
			usage2(_("Wrong parameter '%s' for option -C\n"), cendianess);
		}
	}
	if (cddbp >= 0) {
		global.cddbp = 1 + cddbp;
	}
	if (stereo) {
		global.channels = 2;
	}
	if (mono) {
		global.channels = 1;
		global.need_hostorder = 1;
	}
	if (global.echo) {
#ifdef	ECHO_TO_SOUNDCARD
		if (global.playback_rate != 100) {
			RestrictPlaybackRate(global.playback_rate);
		}
		global.need_hostorder = 1;
#else
		errmsgno(EX_BAD,
			_("There is no sound support compiled into %s.\n"),
			argv[0]);
		global.echo = 0;
#endif
	}
	if (global.quiet) {
		global.verbose = 0;
	}
	if (domax) {
		global.channels = 2; bits = 16; rate = 44100;
	}
	if (global.findminmax) {
		global.need_hostorder = 1;
	}
	if (global.deemphasize) {
		global.need_hostorder = 1;
	}
	if (global.just_the_toc) {
		global.verbose = SHOW_MAX;
		bulk = 1;
	}
	if (global.gui) {
#ifdef	Thomas_will_es
		global.no_file = 1;
		global.no_infofile = 1;
		global.verbose = SHOW_MAX;
#endif
		global.no_cddbfile = 1;
	}
	if (global.no_file) {
		global.no_infofile = 1;
		global.no_cddbfile = 1;
		global.no_textfile = 1;
		global.cuefile = 0;
	}
	if (global.no_infofile) {
		global.no_cddbfile = 1;
		global.no_textfile = 1;
		global.cuefile = 0;
	}
	if (global.cuefile) {
		global.no_infofile = 1;
	}
	if (md5blocksize)
		global.md5blocksize = -1;
	if (global.md5blocksize) {
#ifndef	MD5_SIGNATURES
		errmsgno(EX_BAD,
			_("The option MD5 signatures is not configured!\n"));
#endif
	}
	if (global.user_sound_device[0] != '\0') {
#ifndef	ECHO_TO_SOUNDCARD
		errmsgno(EX_BAD, _("There is no sound support configured!\n"));
#else
		global.echo = TRUE;
#endif
	}
	if (global.paranoia_selected) {
		global.useroverlap = 0;
	}
	if (userverbose >= 0) {
		global.verbose = userverbose;
	}
	/*
	 * check all parameters
	 */
	if (global.bufsize < CD_FRAMESIZE_RAW) {
		usage2(_("Incorrect transfer size setting: %d\n"), global.bufsize);
	}

	if (global.buffers < 1) {
		usage2(_("Incorrect buffer setting: %d\n"), global.buffers);
	}

	if (global.nsectors < 1) {
		usage2(_("Incorrect nsectors setting: %d\n"), global.nsectors);
	}

	if (global.verbose < 0 || global.verbose > SHOW_MAX) {
		usage2(_("Incorrect verbose level setting: %d\n"), global.verbose);
	}
	if (global.verbose == 0)
		global.quiet = 1;

	if (global.rectime < 0.0) {
		usage2(_("Incorrect recording time setting: %d.%02d\n"),
			(int)global.rectime, (int)(global.rectime*100+0.5) % 100);
	}

	if (global.channels != 1 && global.channels != 2) {
		usage2(_("Incorrect channel setting: %d\n"), global.channels);
	}

	if (bits != 8 && bits != 12 && bits != 16) {
		usage2(_("Incorrect bits_per_sample setting: %d\n"), bits);
	}

	if (rate < 827.0 || rate > 44100.0) {
		usage2(_("Incorrect sample rate setting: %d.%02d\n"),
			(int)rate, ((int)rate*100) % 100);
	}

	global.int_part = (double)(long) (2*44100.0 / rate);

	if (2*44100.0 / rate - global.int_part >= 0.5) {
		global.int_part += 1.0;
		fprintf(outfp,
			_("Nearest available sample rate is %d.%02d Hertz\n"),
			2*44100 / (int)global.int_part,
			(2*4410000 / (int)global.int_part) % 100);
	}
	Halved = ((int) global.int_part) & 1;
	rate = 2*44100.0 / global.int_part;
	undersampling = (int) global.int_part / 2.0;
	samples_to_do = undersampling;

	if (strcmp((char *)int_name, "generic_scsi") == 0) {
		interface = GENERIC_SCSI;
	} else if (strcmp((char *)int_name, "cooked_ioctl") == 0) {
		interface = COOKED_IOCTL;
	} else  {
		usage2(_("Incorrect interface setting: %s\n"), int_name);
	}

	/*
	 * check * init audio file
	 */
	if (strncmp(audio_type, "wav", 3) == 0) {
		global.audio_out = &wavsound;
	} else if (strncmp(audio_type, "sun", 3) == 0 ||
		    strncmp(audio_type, "au", 2) == 0) {
		/*
		 * Enhanced compatibility
		 */
		audio_type = "au";
		global.audio_out = &sunsound;
	} else if (strncmp(audio_type, "cdr", 3) == 0||
		    strncmp(audio_type, "raw", 3) == 0) {
		global.audio_out = &rawsound;
	} else if (strncmp(audio_type, "aiff", 4) == 0) {
		global.audio_out = &aiffsound;
	} else if (strncmp(audio_type, "aifc", 4) == 0) {
		global.audio_out = &aifcsound;
#ifdef USE_LAME
	} else if (strncmp(audio_type, "mp3", 3) == 0) {
		global.audio_out = &mp3sound;
		if (!global.quiet) {
			unsigned char Lame_version[20];

			fetch_lame_version(Lame_version);
			fprintf(outfp,
				_("Using LAME version %s.\n"), Lame_version);
		}
		if (bits < 9) {
			bits = 16;
			fprintf(outfp,
			_("Warning: sample size forced to 16 bit for MP3 format.\n"));
		}
#endif /* USE_LAME */
	} else {
		usage2(_("Incorrect audio type setting: %3s\n"), audio_type);
	}

	if (bulk == -1)
		bulk = 0;

	global.need_big_endian = global.audio_out->need_big_endian;
	if (global.outputendianess != NONE)
		global.need_big_endian = global.outputendianess == BIG;

	if (global.no_file)
		global.fname_base[0] = '\0';

	if (!bulk) {
		strcat(global.fname_base, ".");
		strcat(global.fname_base, audio_type);
	}

	/*
	 * If we need to calculate with samples or write them to a soundcard,
	 * we need a conversion to host byte order.
	 */
	if (global.channels != 2 ||
	    bits != 16 ||
	    rate != 44100) {
		global.need_hostorder = 1;
	}

	/*
	 * Bad hack!!
	 * Remove for release 2.0
	 * this is a bug compatibility feature.
	 */
	if (global.gui && global.verbose == SHOW_TOC)
		global.verbose |= SHOW_STARTPOSITIONS | SHOW_SUMMARY |
					SHOW_TITLES;

	/*
	 * all options processed.
	 * Now a file name per track may follow
	 */
	argc2 = argc3 = argc - global.moreargs;
	argv2 = argv + global.moreargs;
	if (global.moreargs < argc) {
		if (strcmp(argv[global.moreargs], "-") == 0) {
			if (audiofd >= 0) {
#ifdef	F_GETFD
				if (fcntl(audiofd, F_GETFD, 0) < 0) {
					comerr(
					_("Cannot redirect audio data to fd %d.\n"),
					audiofd);
				}
#endif
				global.audio = audiofd;
			} else {
				global.audio = dup(fileno(stdout));
			}
			setmode(global.audio, O_BINARY);
			strncpy(global.fname_base, "standard_output",
						sizeof (global.fname_base));
			global.fname_base[sizeof (global.fname_base)-1] = 0;
		} else if (!is_fifo(argv[global.moreargs])) {
			/*
			 * we do have at least one argument
			 */
			global.multiname = 1;
			strlcpy(global.fname_base,
				argv[global.moreargs],
				sizeof (global.fname_base));
		}
	}
}
