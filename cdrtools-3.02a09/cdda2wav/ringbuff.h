/* @(#)ringbuff.h	1.8 16/02/14 Copyright 1998,1999,2000 Heiko Eissfeldt */
/*
 * This file contains data structures that reside in the shared memory
 * segment.
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

#ifndef	_RINGBUFF_H
#define	_RINGBUFF_H

/*
 * the linux semctl prototype is broken as is the definition
 * of union semun in sys/sem.h.
 */

#ifdef HAVE_UNION_SEMUN
#	define	my_semun	semun
#else
union my_semun {
	int		val;
	struct semid_ds	*pid;
	unsigned short	*array;
};
#endif

/* BEGIN CSTYLED */
/*
 * Ringbuffer structures.
 * Space for the ringbuffer is allocated page aligned
 *	 and contains the following
 *
 *	-------------------- start of page
 *	header (once for the ring buffer) \\
 *	space for page alignment	  ||+- HEADER_SIZE
 *	RB_BASE -+v			  ||
 *	myringbuffer.offset		  |/
 *	-------------------- start of page/-- pagesize
 *	myringbuffer.data (SEGMENT_SIZE)\
 *	space for page alignment	|+- ENTRY_SIZE_PAGE_AL
 *	myringbuffer.offset		/
 *	-------------------- start of page
 *	myringbuffer.data
 *	space for page alignment
 *	...
 */
/* END CSTYLED */

typedef struct {
	int	offset;
	UINT4	data[CD_FRAMESAMPLES];
} myringbuff;

struct ringbuffheader {
	myringbuff		*p1;
	myringbuff		*p2;
	volatile unsigned long	total_read;
	volatile unsigned long	total_written;
	volatile int		child_waitstate;
	volatile int		parent_waitstate;
	volatile int		input_littleendian;
	volatile int		end_is_reached;
	volatile unsigned long	nSamplesToDo;
	int			offset;
	UINT4			data[CD_FRAMESAMPLES];
};

extern	myringbuff	**he_fill_buffer;
extern	myringbuff	**last_buffer;
extern	volatile unsigned long	*total_segments_read;
extern	volatile unsigned long	*total_segments_written;
extern	volatile int	*child_waits;
extern	volatile int	*parent_waits;
extern	volatile int	*in_lendian;
extern	volatile int	*eorecording;

#define	palign(x, a)	(((char *)(x)) + ((a) - 1 - (((unsigned)((x)-1))%(a))))
#define	multpage(x, a)	((((x) + (a) - 1) / (a)) * (a))

#define	HEADER_SIZE	multpage(offsetof(struct ringbuffheader, data), global.pagesize)
#define	SEGMENT_SIZE	(global.nsectors*CD_FRAMESIZE_RAW)
#define	ENTRY_SIZE_PAGE_AL	multpage(SEGMENT_SIZE + \
				offsetof(myringbuff, data), global.pagesize)

#define	RB_BASE		((myringbuff *)(((unsigned char *)he_fill_buffer) + \
			HEADER_SIZE - offsetof(myringbuff, data)))

#define	INC(a)		(myringbuff *)(((char *)RB_BASE) + \
			(((((char *) (a))-((char *)RB_BASE))/ENTRY_SIZE_PAGE_AL + 1) % \
			total_buffers)*ENTRY_SIZE_PAGE_AL)


extern	void		set_total_buffers	__PR((unsigned int num_buffers,
								int mysem_id));
extern	const myringbuff *get_previous_read_buffer __PR((void));
extern	const myringbuff *get_he_fill_buffer	__PR((void));
extern	myringbuff	*get_next_buffer	__PR((void));
extern	myringbuff	*get_oldest_buffer	__PR((void));
extern	void		define_buffer		__PR((void));
extern	void		drop_buffer		__PR((void));
extern	void		drop_all_buffers	__PR((void));

#endif	/* _RINGBUFF_H */
