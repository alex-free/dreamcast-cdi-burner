/* @(#)semshm.h	1.6 08/08/03 Copyright 1998,1999 Heiko Eissfeldt */

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

#ifndef	_SEMSHM_H
#define	_SEMSHM_H

#undef DEBUG_SHM
#ifdef DEBUG_SHM
extern	char	*start_of_shm;
extern	char	*end_of_shm;
#endif

#define	FREE_SEM	0
#define	DEF_SEM		1

#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
extern	int	sem_id;
#else

#define	sem_id		42	/* nearly any other number would do it too */

extern	void	init_pipes	__PR((void));
extern	void	init_parent	__PR((void));
extern	void	init_child	__PR((void));
#endif


#ifdef	HAVE_AREAS
/*
 * The name of the shared memory mapping for the FIFO under BeOS.
 */
#define	AREA_NAME	"shmfifo"
#endif

extern	void	free_sem	__PR((void));
extern	int	semrequest	__PR((int semid, int semnum));
extern	int	semrelease	__PR((int semid, int semnum, int amount));
extern	void	semdestroy	__PR((void));
extern	int	flush_buffers	__PR((void));
extern	void	* request_shm_sem __PR((unsigned amount_of_sh_mem, unsigned char **pointer));

#endif	/* _SEMSHM_H */
