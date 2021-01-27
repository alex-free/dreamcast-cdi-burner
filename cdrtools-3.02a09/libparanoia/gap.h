/* @(#)gap.h	1.11 06/05/06 J. Schilling from cdparanoia-III-alpha9.8 */
/*
 * CopyPolicy: GNU Lesser General Public License v2.1 applies
 * Copyright (C) 1997-2001 by Monty (xiphmont@mit.edu)
 * Copyright (C) 2002-2006 by J. Schilling
 */

#ifndef	_GAP_H
#define	_GAP_H

extern long	i_paranoia_overlap_r	__PR((Int16_t * buffA, Int16_t * buffB,
						long offsetA, long offsetB));
extern long	i_paranoia_overlap_f	__PR((Int16_t * buffA, Int16_t * buffB,
						long offsetA, long offsetB,
						long sizeA, long sizeB));
extern int	i_stutter_or_gap	__PR((Int16_t * A, Int16_t * B,
						long offA, long offB,
						long gap));
extern void	i_analyze_rift_f	__PR((Int16_t * A, Int16_t * B,
						long sizeA, long sizeB,
						long aoffset, long boffset,
						long *matchA, long *matchB,
						long *matchC));
extern void	i_analyze_rift_r	__PR((Int16_t * A, Int16_t * B,
						long sizeA, long sizeB,
						long aoffset, long boffset,
						long *matchA, long *matchB,
						long *matchC));

extern void	analyze_rift_silence_f	__PR((Int16_t * A, Int16_t * B,
						long sizeA, long sizeB,
						long aoffset, long boffset,
						long *matchA, long *matchB));

#endif	/* _GAP_H */
