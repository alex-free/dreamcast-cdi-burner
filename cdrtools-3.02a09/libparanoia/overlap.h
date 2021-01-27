/* @(#)overlap.h	1.8 06/05/06 J. Schilling from cdparanoia-III-alpha9.8 */
/*
 * CopyPolicy: GNU Lesser General Public License v2.1 applies
 * Copyright (C) 1997-2001 by Monty (xiphmont@mit.edu)
 * Copyright (C) 2002-2006 by J. Schilling
 */

#ifndef	_OVERLAP_H
#define	_OVERLAP_H

extern	void	paranoia_resetcache	__PR((cdrom_paranoia * p));
extern	void	paranoia_resetall	__PR((cdrom_paranoia * p));
extern	void	i_paranoia_trim		__PR((cdrom_paranoia * p,
						long beginword, long endword));
extern	void	offset_adjust_settings	__PR((cdrom_paranoia * p,
						void (*callback) (long, int)));
extern	void	offset_add_value	__PR((cdrom_paranoia * p,
						offsets * o, long value,
						void (*callback) (long, int)));

#endif	/* _OVERLAP_H */
