/* @(#)semshm.c 1.33 16/02/14 Copyright 1998-2002 Heiko Eissfeldt, Copyright 2004-2013 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)semshm.c	1.33 16/02/14 Copyright 1998-2002 Heiko Eissfeldt, Copyright 2004-2013 J. Schilling";
#endif

#define	IPCTST
#undef IPCTST
/* -------------------------------------------------------------------- */
/*	semshm.c							*/
/* -------------------------------------------------------------------- */
/*		int seminstall(key,amount)				*/
/*		int semrequest(semid,semnum)				*/
/*		int semrelease(semid,semnum)				*/
/* -------------------------------------------------------------------- */
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

#if	!defined(HAVE_SMMAP) && !defined(HAVE_USGSHM) && \
	!defined(HAVE_DOSALLOCSHAREDMEM) && !defined(HAVE_AREAS)
#undef  FIFO			/* We cannot have a FIFO on this platform */
#endif

#if !defined(USE_MMAP) && !defined(USE_USGSHM)
#define	USE_MMAP
#endif

#if	!defined HAVE_SMMAP && defined FIFO
#	undef   USE_MMAP
#	define  USE_USGSHM	/* SYSV shared memory is the default	*/
#endif

#ifdef  USE_MMAP		/* Only want to have one implementation	*/
#	undef   USE_USGSHM	/* mmap() is preferred			*/
#endif

#ifdef	HAVE_DOSALLOCSHAREDMEM
#	undef   USE_MMAP
#	undef   USE_USGSHM
#	define	USE_OS2SHM
#	undef	USE_BEOS_AREAS
#endif

#ifdef	HAVE_AREAS
#	undef   USE_MMAP
#	undef   USE_USGSHM
#	undef	USE_OS2SHM
#	define	USE_BEOS_AREAS
#endif

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/nlsdefs.h>

#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
#include <schily/types.h>
#include <schily/ipc.h>
#include <schily/sem.h>
#endif

#if defined(HAVE_SHMAT) && (HAVE_SHMAT == 1)
#include <schily/types.h>
#include <schily/ipc.h>
#include <schily/shm.h>
#endif

#ifdef  USE_MMAP
#if defined(HAVE_SMMAP) && defined(USE_MMAP)
#include <schily/mman.h>
#endif
#endif
#include <schily/schily.h>

#include <scg/scsitransp.h>

#ifdef	USE_BEOS_AREAS
#ifdef	HAVE_OS_H
#include	<OS.h>
#else
#include	<be/kernel/OS.h>
#endif
#endif

#include "mytype.h"
#include "interface.h"
#include "ringbuff.h"
#include "global.h"
#include "exitcodes.h"
#include "semshm.h"

#ifdef DEBUG_SHM
char	*start_of_shm;
char	*end_of_shm;
#endif

int	flush_buffers	__PR((void));


/* ------ Semaphore interfacing (for special cases only)	---------- */
/* ------ Synchronization with pipes is preferred	---------- */

#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)

int sem_id;
static int	seminstall	__PR((key_t key, int amount));

static int
seminstall(key, amount)
	key_t	key;
	int	amount;
{
	int	ret_val;
	int	semflag;

	semflag = IPC_CREAT | 0600;
#ifdef IPCTST
	fprintf(stderr,
		"seminstall: key: %d, #sems %d, flags %4x\n",
		key, amount, semflag);
#endif
	ret_val = semget(key, amount, semflag);
	if (ret_val == -1) {
		errmsg(_("Semget: (Key %lx, #%d) failed.\n"),
			(long)key, amount);
	}
	return (ret_val);
}

/* ----------------------------------------------------------------- */
int
semrequest(semid, semnum)
	int	semid;
	int	semnum;
{
	struct sembuf	sops[1];
	int		ret_val;

#ifdef IPCTST
	fprintf(stderr, "pid %d, ReQuest id:num %d:%d\n",
			getpid(), semid, semnum);
#endif
	if (!global.have_forked)
		return (0);
	sops[0].sem_op  = -1;
	sops[0].sem_num = (short) semnum;
	sops[0].sem_flg = 0;

	do {
		errno = 0;
		ret_val = semop(semid, sops, 1);
		if (ret_val == -1 && errno != EAGAIN && errno != EINTR) {
			errmsg(_("Request Sema %d(%d) failed.\n"),
				semid, semnum);
		}
	} while (errno == EAGAIN || errno == EINTR);
	return (ret_val);
}

/* ----------------------------------------------------------------- */
int
semrelease(semid, semnum, amount)
	int	semid;
	int	semnum;
	int	amount;
{
	struct sembuf	sops[1];
	int		ret_val;

#ifdef IPCTST
	fprintf(stderr, "%d RL %d:%d\n", getpid(), semid, semnum);
#endif
	if (!global.have_forked)
		return (0);
	sops[0].sem_op  = amount;
	sops[0].sem_num = (short) semnum;
	sops[0].sem_flg = 0;
	ret_val = semop(semid, sops, 1);
	if (ret_val == -1 && errno != EAGAIN) {
		errmsg(_("Release Sema %d(%d) failed.\n"),
			semid, semnum);
	}
	return (ret_val);
}

void
semdestroy()
{
	/*
	 * How do we stop the other process from waiting?
	 */
	free_sem();
}

int
flush_buffers()
{
	return (0);
}
#else
/* ------ Synchronization with pipes ---------- */
int	pipefdp2c[2];
int	pipefdc2p[2];

void
init_pipes()
{
	if (pipe(pipefdp2c) < 0) {
		errmsg(_("Cannot create pipe parent to child.\n"));
		exit(PIPE_ERROR);
	}
	if (pipe(pipefdc2p) < 0) {
		errmsg(_("Cannot create pipe child to parent.\n"));
		exit(PIPE_ERROR);
	}
}

void
init_parent()
{
	close(pipefdp2c[0]);
	close(pipefdc2p[1]);
}

void
init_child()
{
	close(pipefdp2c[1]);
	close(pipefdc2p[0]);
}

int
semrequest(dummy, semnum)
	int	dummy;
	int	semnum;
{
	if (!global.have_forked)
		return (0);
	if (semnum == FREE_SEM /* 0 */)  {
		if ((*total_segments_read) - (*total_segments_written) >=
		    global.buffers) {
			int	retval;

			/*
			 * parent/reader waits for freed buffers from the
			 * child/writer
			 */
			*parent_waits = 1;
			retval = read(pipefdp2c[0], &dummy, 1) != 1;
			return (retval);
		}
	} else {
		if ((*total_segments_read) == (*total_segments_written)) {
			int	retval;

			/*
			 * child/writer waits for defined buffers from the
			 * parent/reader
			 */
			*child_waits = 1;
			retval = read(pipefdc2p[0], &dummy, 1) != 1;
			return (retval);
		}
	}
	return (0);
}

/* ARGSUSED */
int
semrelease(dummy, semnum, amount)
	int	dummy;
	int	semnum;
	int	amount;
{
	if (!global.have_forked)
		return (0);
	if (semnum == FREE_SEM /* 0 */)  {
		if (*parent_waits == 1) {
			int	retval;

			/*
			 * child/writer signals freed buffer to the
			 * parent/reader
			 */
			*parent_waits = 0;
			retval = write(pipefdp2c[1],
					"12345678901234567890",
					amount) != amount;
			return (retval);
		}
	} else {
		if (*child_waits == 1) {
			int	retval;

			/*
			 * parent/reader signals defined buffers to the
			 * child/writer
			 */
			*child_waits = 0;
			retval = write(pipefdc2p[1],
					"12345678901234567890",
					amount) != amount;
			return (retval);
		}
	}
	return (0);
}

void
semdestroy()
{
	if (global.child_pid == 0) {	/* Child */
		close(pipefdp2c[1]);
		close(pipefdc2p[0]);
	} else if (global.child_pid != -1) {
		close(pipefdp2c[0]);
		close(pipefdc2p[1]);
	}
}


int
flush_buffers()
{
	if ((*total_segments_read) > (*total_segments_written)) {
		return (write(pipefdc2p[1], "1", 1) != 1);
	}
	return (0);
}

#endif

/* ------------------- Shared memory interfacing ----------------------- */



#if defined(HAVE_SHMAT) && (HAVE_SHMAT == 1)
static int shm_request_nommap	__PR((int size, unsigned char **memptr));

/* request a shared memory block */
static int
shm_request_nommap(size, memptr)
	int		size;
	unsigned char	**memptr;
{
	int	ret_val;
	int	shmflag;
	int	SHMEM_ID;
	int	cmd;
	struct shmid_ds	buf;
	key_t		key = IPC_PRIVATE;

	shmflag = IPC_CREAT | 0600;
	ret_val = shmget(key, size, shmflag);
	if (ret_val == -1) {
		errmsg(_("Shmget failed.\n"));
		return (-1);
	}

	SHMEM_ID = ret_val;
	cmd = IPC_STAT;
	ret_val = shmctl(SHMEM_ID, cmd, &buf);
#ifdef IPCTST
	fprintf(stderr,
	"%d: shmctl STAT= %d, SHM_ID: %d, key %ld cuid %d cgid %d mode %3o size %d\n",
		getpid(), ret_val, SHMEM_ID,
		(long) buf.shm_perm.key,
		buf.shm_perm.cuid, buf.shm_perm.cgid,
		buf.shm_perm.mode, buf.shm_segsz);
#endif
	if (ret_val == -1) {
		errmsg(_("Shmctl failed.\n"));
		return (-1);
	}

	*memptr = (unsigned char *) shmat(SHMEM_ID, NULL, 0);
	if (*memptr == (unsigned char *) -1) {
		*memptr = NULL;
		errmsg(_("Shmat failed for %d bytes.\n"), size);
		return (-1);
	}

	if (shmctl(SHMEM_ID, IPC_RMID, 0) < 0) {
		errmsg(_("Shmctl failed to detach shared memory segment.\n"));
		return (-1);
	}


#ifdef DEBUG_SHM
	start_of_shm = *memptr;
	end_of_shm = (char *)(*memptr) + size;

	fprintf(stderr,
	"Shared memory from %p to %p (%d bytes)\n", start_of_shm, end_of_shm, size);
#endif
	return (0);
}


#endif	/* #if defined(HAVE_SHMAT) && (HAVE_SHMAT == 1) */


static int shm_request	__PR((int size, unsigned char **memptr));

#ifdef  USE_USGSHM
/*
 * request a shared memory block
 */
static int
shm_request(size, memptr)
	int		size;
	unsigned char	**memptr;
{
	return (shm_request_nommap(size, memptr));
}
#endif

/*
 * release semaphores
 */
void	free_sem	__PR((void));
void
free_sem()
{
#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
	int		mycmd;
	union my_semun	unused_arg;

	mycmd = IPC_RMID;

	/*
	 * HP-UX warns here, but 'unused_arg' is not used for this operation
	 * This warning is difficult to avoid, since the structure of the union
	 * generally is not known (os dependent). So we cannot initialize it
	 * properly.
	 */
	semctl(sem_id, 0, mycmd, unused_arg);
#endif

}

#ifdef  USE_MMAP
#if defined(HAVE_SMMAP)

int shm_id;
/*
 * request a shared memory block
 */
static int
shm_request(size, memptr)
	int		size;
	unsigned char	**memptr;
{
	int	f;
	char	*addr;

#ifdef  MAP_ANONYMOUS   /* HP/UX */
	f = -1;
	addr = mmap(0, mmap_sizeparm(size),
					PROT_READ|PROT_WRITE,
					MAP_SHARED|MAP_ANONYMOUS, f, 0);
#else
	if ((f = open("/dev/zero", O_RDWR)) < 0)
		comerr(_("Cannot open '/dev/zero'.\n"));
	addr = mmap(0, mmap_sizeparm(size),
					PROT_READ|PROT_WRITE,
					MAP_SHARED, f, 0);
#endif

	if (addr == (char *)-1) {
#if	defined HAVE_SHMAT && (HAVE_SHMAT == 1)
		unsigned char *address;
		/* fallback to alternate method */
		if (0 != shm_request_nommap(size, &address) ||
		    (addr = (char *)address) == NULL)
#endif
			comerr(_("Cannot get mmap for %d Bytes on /dev/zero.\n"),
				size);
	}
	close(f);

	if (memptr != NULL)
		*memptr = (unsigned char *)addr;

	return (0);
}
#endif	/* HAVE_SMMAP */
#endif	/* USE_MMAP */

#ifdef	USE_OS2SHM

/*
 * request a shared memory block
 */
static int
shm_request(size, memptr)
	int		size;
	unsigned char	**memptr;
{
	char	*addr;

	/*
	 * The OS/2 implementation of shm (using shm.dll) limits the size
	 * of one memory segment to 0x3fa000 (aprox. 4MBytes).
	 * Using OS/2 native API we no such restriction so I decided to use
	 * it allowing fifos of arbitrary size
	 */
	if (DosAllocSharedMem(&addr, NULL, size, 0X100L | 0x1L | 0x2L | 0x10L))
		comerr(_("DosAllocSharedMem() failed\n"));

	if (memptr != NULL)
		*memptr = (unsigned char *)addr;

	return (0);
}
#endif

#ifdef	USE_BEOS_AREAS

/*
 * request a shared memory block
 */
static int
shm_request(size, memptr)
	int		size;
	unsigned char	**memptr;
{
	char	*addr;
	area_id	aid;	/* positive id of the mapping */

	/*
	 * round up to a multiple of pagesize.
	 */
	size = ((size - 1) | (B_PAGE_SIZE - 1)) + 1;
	/*
	 * request a shared memory area in user space.
	 */
	aid = create_area(AREA_NAME,	/* name of the mapping */
		(void *)&addr,		/* address of shared memory */
		B_ANY_ADDRESS,		/* type of address constraint */
		size,			/* size in bytes (multiple of pagesize) */
		B_NO_LOCK, /* B_FULL_LOCK, */ /* memory locking */
		B_READ_AREA | B_WRITE_AREA);	/* read and write permissions */

	if (aid < B_OK)
		comerrno(aid, _("create_area() failed\n"));

	if (memptr != NULL)
		*memptr = (unsigned char *)addr;

	return (0);
}
#endif

void *
request_shm_sem(amount_of_sh_mem, pointer)
	unsigned	amount_of_sh_mem;
	unsigned char	**pointer;
{
#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
	/*
	 * install semaphores for double buffer usage
	 */
	sem_id = seminstall(IPC_PRIVATE, 2);
	if (sem_id == -1) {
		errmsg(_("Seminstall failed.\n"));
		exit(SEMAPHORE_ERROR);
	}

#endif

#if defined(FIFO)
	if (-1 == shm_request(amount_of_sh_mem, pointer)) {
		errmsg(_("Shm_request failed.\n"));
		exit(SHMMEM_ERROR);
	}

	return (*pointer);
#else
	return (NULL);
#endif
}
