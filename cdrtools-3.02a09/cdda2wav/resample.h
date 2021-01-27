/* @(#)resample.h	1.5 06/05/13 Copyright 1998,1999 Heiko Eissfeldt */

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

#define	SYNC_SIZE	600	/* has to be smaller than CD_FRAMESAMPLES */

extern	int	waitforsignal;	/* flag: wait for any audio response */
extern	int	any_signal;

extern	short	undersampling;	/* conversion factor */
extern	short	samples_to_do;	/* loop variable for conversion */
extern	int	Halved;		/* interpolate due to non integral divider */
extern	int	jitterShift;	/* track accumulated jitter */

extern	long		SaveBuffer	__PR((UINT4 *p,
						unsigned long SecsToDo,
						unsigned long *BytesDone));
extern	unsigned char	*synchronize	__PR((UINT4 *p,
						unsigned SamplesToDo,
						unsigned TotSamplesDone));
extern	void		handle_inputendianess __PR((UINT4 *p,
						unsigned SamplesToDo));
