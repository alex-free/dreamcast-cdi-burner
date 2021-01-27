/* @(#)global.h	1.37 16/01/24 Copyright 1998-2004 Heiko Eissfeldt, Copyright 2004-2016 J. Schilling */
/*
 * Global Variables
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

#ifdef  MD5_SIGNATURES
#include <schily/md5.h>
#endif
#ifdef	USE_PARANOIA
#include "cdda_paranoia.h"
#endif

#define	outfp	global.out_fp

typedef struct index_list {
	struct index_list	*next;
	int			frameoffset;
} index_list;

typedef struct global {

	char			*dev_name;		/* device name */
	char			*dev_opts;		/* SCG device options */
	char			*aux_name;		/* auxiliary cdrom device name */
	char			fname_base[200];	/* current file name base */

	int			have_forked;		/* TRUE after we did fork */
	pid_t			child_pid;		/* return value from fork() */
	int			parent_died;		/* TRUE after we killed the parent */
	int			audio;			/* audio-out file desc */
	struct soundfile	*audio_out;		/* audio-out sound functions */
	int			cooked_fd;		/* cdrom-in file desc */
	int			no_file;		/* -N option */
	int			no_infofile;		/* -no-infofile option */
	int			no_textfile;		/* -no-textfile option */
	int			did_textfile;		/* flag: did create textfile */
	int			no_textdefaults;	/* -no-textdefaults option */
	int			no_cddbfile;		/* flag: do not create cddbfile */
	int			cuefile;		/* -cuefile option */
	int			no_hidden_track;	/* -no-hidden-track option */
	int			no_fork;		/* -no-fork option */
	int			interactive;		/* -interactive option */
	int			quiet;			/* -quiet option */
	int			verbose;		/* -v verbose level */
	int			scsi_silent;		/* SCSI silent flag */
	int			scsi_verbose;		/* SCSI verbose level */
	int			scsi_debug;		/* SCSI debug level */
	int			scsi_kdebug;		/* SCSI kernel debug level */
	int			scanbus;		/* -scanbus option */

	uid_t			uid;
	uid_t			euid;
	BOOL			issetuid;
	long			sector_offset;
	long			start_sector;
	unsigned long		endtrack;
	BOOL			alltracks;
	BOOL			maxtrack;
	int			cd_index;
	int			littleendian;
	double			rectime;
	double			int_part;
	char			*user_sound_device;
	int			moreargs;

	int			multiname;		/* multiple file names given */
	int			sh_bits;		/* sh_bits: sample bit shift */
	int			Remainder;
	int			SkippedSamples;
	int			OutSampleSize;
	int			need_big_endian;
	int			need_hostorder;
	int			channels;		/* output sound channels */
	unsigned long		iloop;			/* todo counter (frames) */
	unsigned long		nSamplesDoneInTrack;	/* written samples in current track */
	unsigned		overlap;		/* dynamic cdda2wav overlap */
	int			useroverlap;		/* -set-overlap # option */
	FILE			*out_fp;		/* -out-fd FILE * for messages */
	char			*buf;			/* The SCSI buffer */
	long			bufsize;		/* The size of the SCSI buffer */
	unsigned		nsectors;		/* -sectors-per-request option */
	unsigned		buffers;		/* -buffers-in-ring option */
	unsigned		shmsize;
	long			pagesize;
	int			in_lendian;
	int			outputendianess;
	int			findminmax;
	int			maxamp[2];
	int			minamp[2];
	unsigned		speed;
	int			userspeed;
	int			ismono;
	int			findmono;
	int			swapchannels;
	int			deemphasize;
	int			gui;
	long			playback_rate;
	int			target;			/* SCSI Id to be used */
	int			lun;			/* SCSI Lun to be used */
	UINT4			cddb_id;
	int			cddbp;
	char *			cddbp_server;
	char *			cddbp_port;
	unsigned		cddb_revision;
	int			cddb_year;
	char			cddb_genre[60];
	int			illleadout_cd;
	int			reads_illleadout;
	unsigned char		*cdindex_id;
	unsigned char		*copyright_message;	/* CD Extra specific */

	unsigned char		*disctitle;		/* 0x80 Album Ttitle */
	unsigned char		*performer;		/* 0x81 Album Performer */
	unsigned char		*songwriter;		/* 0x82 Album Songwriter */
	unsigned char		*composer;		/* 0x83 Album Composer */
	unsigned char		*arranger;		/* 0x84 Album Arranger */
	unsigned char		*message;		/* 0x85 Album Message */
	unsigned char		*closed_info;		/* 0x8d Album Closed Info */

	unsigned char		*tracktitle[100];	/* 0x80 Track Title */
	unsigned char		*trackperformer[100];	/* 0x81 Track Performer */
	unsigned char		*tracksongwriter[100];	/* 0x82 Track Songwriter */
	unsigned char		*trackcomposer[100];	/* 0x83 Track Composer */
	unsigned char		*trackarranger[100];	/* 0x84 Track Arranger */
	unsigned char		*trackmessage[100];	/* 0x85 Track Message */
	unsigned char		*trackclosed_info[100];	/* 0x8d Track Closed Info */

	index_list		*trackindexlist[100];

	int			paranoia_selected;
	long			paranoia_flags;
	int			paranoia_mode;
#ifdef	USE_PARANOIA
	cdrom_paranoia  	*cdp;

	struct paranoia_parms_t {
		Ucbit	disable_paranoia:1;
		Ucbit	disable_extra_paranoia:1;
		Ucbit	disable_scratch_detect:1;
		Ucbit	disable_scratch_repair:1;
		Ucbit	enable_c2_check:1;
		int	retries;
		int	readahead;
		int	overlap;
		int	mindynoverlap;
		int	maxdynoverlap;
	} paranoia_parms;
#endif

	int			md5offset;
	int			md5blocksize;
#ifdef	MD5_SIGNATURES
	int			md5count;
	int			md5size;
	MD5_CTX			*context;
	unsigned char		MD5_result[16];
#endif

#ifdef	ECHO_TO_SOUNDCARD
	int			soundcard_fd;
#endif
	int			echo;

	int			just_the_toc;
} global_t;

extern global_t global;
