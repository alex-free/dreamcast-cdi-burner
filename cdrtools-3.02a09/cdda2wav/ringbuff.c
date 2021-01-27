/* @(#)ringbuff.c	1.24 10/12/19 Copyright 1998,1999,2000 Heiko Eissfeldt, Copyright 2004-2010 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)ringbuff.c	1.24 10/12/19 Copyright 1998,1999,2000 Heiko Eissfeldt, Copyright 2004-2010 J. Schilling";
#endif
/*
 * Ringbuffer handling
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

#include "config.h"

#include <schily/stdlib.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
#include <schily/ipc.h>
#include <schily/sem.h>
#endif

#include <scg/scsitransp.h>

#include "mytype.h"
#include "global.h"
#include "interface.h"
#include "ringbuff.h"
#include "semshm.h"
#include "exitcodes.h"

#undef WARN_INTERRUPT
#undef _DEBUG
#include <schily/assert.h>

static void occupy_buffer	__PR((void));

myringbuff		**he_fill_buffer;
myringbuff		**last_buffer;
volatile unsigned long	*total_segments_read;
volatile unsigned long	*total_segments_written;
volatile int		*child_waits;
volatile int		*parent_waits;
volatile int		*in_lendian;
volatile int		*eorecording;

static myringbuff	*previous_read_buffer;
static unsigned int	total_buffers;

#define	SEMS	2

#define	defined_buffers()	((*total_segments_read) - \
				(*total_segments_written))
#define	free_buffers()		(total_buffers - defined_buffers())
#define	occupied_buffers()	(defined_buffers())

/* ARGSUSED */
void
set_total_buffers(num_buffers, mysem_id)
	unsigned int	num_buffers;
	int		mysem_id;
{
#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
	union my_semun	mysemun;

	mysemun.val   = 0;
	if (semctl(mysem_id, (int) DEF_SEM, SETVAL, mysemun) < 0) {
		errmsg(_("Error in semctl DEF_SEM.\n"));
	}

	mysemun.val   = num_buffers;
	if (semctl(mysem_id, (int) FREE_SEM, SETVAL, mysemun) < 0) {
		errmsg(_("Error in semctl FREE_SEM.\n"));
	}
#endif

	total_buffers = num_buffers;

	/*
	 * initialize pointers
	 */
	*he_fill_buffer = *last_buffer = previous_read_buffer = NULL;
#ifdef DEBUG_SHM
	fprintf(stderr,
		"init: fill_b = %p,  last_b = %p\n",
		*he_fill_buffer, *last_buffer);
#endif
}

const myringbuff *
get_previous_read_buffer()
{
	assert(previous_read_buffer != NULL);
	assert(previous_read_buffer != *he_fill_buffer);
	return (previous_read_buffer);
}

const myringbuff *
get_he_fill_buffer()
{
	assert(*he_fill_buffer != NULL);
	assert(previous_read_buffer != *he_fill_buffer);
	return (*he_fill_buffer);
}

void
define_buffer()
{
	assert(defined_buffers() < total_buffers);

#ifdef _DEBUG
#if 0
	fprintf(stderr, "stop  reading  %p - %p\n",
		*he_fill_buffer, (char *)(*he_fill_buffer) + ENTRY_SIZE -1);
#endif
#endif

	if (*last_buffer == NULL)
		*last_buffer = *he_fill_buffer;
#ifdef DEBUG_SHM
	fprintf(stderr,
		"define: fill_b = %p,  last_b = %p\n",
		*he_fill_buffer, *last_buffer);
#endif

	(*total_segments_read)++;
	semrelease(sem_id, DEF_SEM, 1);
}

void
drop_buffer()
{
	assert(free_buffers() < total_buffers);
	assert(occupied_buffers() > 0);

#ifdef _DEBUG
#if 0
	fprintf(stderr, " stop  writing %p - %p ",
		*last_buffer, (char *)(*last_buffer) + ENTRY_SIZE -1);
#endif
#endif

	if (*last_buffer == NULL)
		*last_buffer = *he_fill_buffer;
	else
		*last_buffer = INC(*last_buffer);
#ifdef DEBUG_SHM
	fprintf(stderr,
		"drop: fill_b = %p,  last_b = %p\n",
		*he_fill_buffer, *last_buffer);
#endif
	(*total_segments_written)++;
	semrelease(sem_id, FREE_SEM, 1);
}

void
drop_all_buffers()
{
	(*total_segments_written) = (*total_segments_read);
	semrelease(sem_id, FREE_SEM, total_buffers);
}

static void
occupy_buffer()
{
	assert(occupied_buffers() <= total_buffers);

	previous_read_buffer = *he_fill_buffer;

	if (*he_fill_buffer == NULL) {
		*he_fill_buffer = RB_BASE;
	} else {
		*he_fill_buffer = INC(*he_fill_buffer);
	}
}

#if defined HAVE_FORK_AND_SHAREDMEM
myringbuff *
get_next_buffer()
{
	if (!global.have_forked) {
		occupy_buffer();
		return (*he_fill_buffer);
	}
#ifdef WARN_INTERRUPT
	if (free_buffers() <= 0) {
		fprintf(stderr,
		_("READER waits!! r=%lu, w=%lu\n"), *total_segments_read,
			*total_segments_written);
	}
#endif

	/*
	 * wait for a new buffer to become available
	 */
	if (semrequest(sem_id, FREE_SEM) != 0) {
		/*
		 * semaphore operation failed.
		 * try again...
		 */
		errmsgno(EX_BAD, _("Child reader sem request failed.\n"));
		exit(SEMAPHORE_ERROR);
	}
#if 0
	fprintf(stderr, "start reading  %p - %p\n",
		*he_fill_buffer, (char *)(*fill_buffer) + ENTRY_SIZE -1);
#endif

	occupy_buffer();

#ifdef DEBUG_SHM
	fprintf(stderr,
		"next: fill_b = %p,  last_b = %p, @last = %p\n",
		*he_fill_buffer, *last_buffer, last_buffer);
#endif
	return (*he_fill_buffer);
}

myringbuff *
get_oldest_buffer()
{
	myringbuff	*retval;

	if (!global.have_forked)
		return (*he_fill_buffer);

#ifdef WARN_INTERRUPT
	if (free_buffers() == total_buffers) {
		fprintf(stderr,
		_("WRITER waits!! r=%lu, w=%lu\n"), *total_segments_read,
			*total_segments_written);
	}
#endif
	/*
	 * wait for buffer to be defined
	 */
	seterrno(0);
	if (semrequest(sem_id, DEF_SEM) != 0) {
		if (geterrno() == 0)
			return ((myringbuff *)NULL);
		/*
		 * semaphore operation failed.
		 */
		errmsg(_("Parent writer sem request failed.\n"));
		return ((myringbuff *)NULL);
	}

	retval = *last_buffer;

#if 0
	fprintf(stderr, " begin writing %p - %p\n",
			retval, (char *)retval + ENTRY_SIZE -1);
#endif

	return (retval);
}
#else /* HAVE_FORK_AND_SHAREDMEM */
myringbuff *
get_next_buffer()
{
	occupy_buffer();
	return (*he_fill_buffer);
}

myringbuff *
get_oldest_buffer()
{
	return (*he_fill_buffer);
}
#endif /* HAVE_FORK_AND_SHAREDMEM */
