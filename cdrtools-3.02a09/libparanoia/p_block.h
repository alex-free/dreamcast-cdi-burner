/* @(#)p_block.h	1.21 15/09/20 J.^ Schilling from cdparanoia-III-alpha9.8 */
/*
 * CopyPolicy: GNU Lesser General Public License v2.1 applies
 * Copyright (C) 1997-2001 by Monty (xiphmont@mit.edu)
 * Copyright (C) 2002-2015 by J. Schilling
 */

#ifndef	_P_BLOCK_H
#define	_P_BLOCK_H

#define	MIN_WORDS_OVERLAP	  64	/* 16 bit words */
#define	MIN_WORDS_SEARCH	  64	/* 16 bit words */
#define	MIN_WORDS_RIFT		  16	/* 16 bit words */
#define	MAX_SECTOR_OVERLAP	  32	/* sectors */
#define	MIN_SECTOR_EPSILON	 128	/* words */
#define	MIN_SECTOR_BACKUP	  16	/* sectors */
#define	JIGGLE_MODULO		  15	/* sectors */
#define	MIN_SILENCE_BOUNDARY	1024	/* 16 bit words */

#undef	abs
#define	abs(x)		((x) < 0 ? -(x) : (x))
#undef	min
#define	min(x, y)	((x) < (y) ? (x) : (y))
#undef	max
#define	max(x, y)	((x) < (y) ? (y) : (x))

#include "isort.h"

typedef struct linked_list {
	/* linked list */
	struct linked_element	*head;
	struct linked_element	*tail;

	void			*(*new_poly)	__PR((void));
	void			(*free_poly)	__PR((void *poly));
	long			current;
	long			active;

} linked_list;

typedef struct linked_element {
	void			*ptr;
	struct linked_element	*prev;
	struct linked_element	*next;

	struct linked_list	*list;
	int			stamp;
} linked_element;

extern linked_list	*new_list	__PR((void *(*newp) (void),
						void (*freep) (void *)));
extern linked_element	*new_elem	__PR((linked_list * list));
extern linked_element	*add_elem	__PR((linked_list * list, void *elem));
extern void		free_list	__PR((linked_list * list, int free_ptr));	/* unlink or free */
extern void		free_elem	__PR((linked_element * e, int free_ptr));	/* unlink or free */
extern void		*get_elem	__PR((linked_element * e));
extern linked_list	*copy_list	__PR((linked_list * list));	/* shallow; doesn't copy */
									/* contained structures */

typedef struct c_block {
	/* The buffer */
	Int16_t		*vector;
	long		begin;	/* Begin diskoff, multiples of 16bit samples */
	long		size;	/* Size, multiples of 16bit samples */

	/* auxiliary support structures */
	unsigned char 	*flags;
				/*
				 * 1    known boundaries in read data
				 * 2    known blanked data
				 * 4    matched sample
				 * 8    reserved
				 * 16   reserved
				 * 32   reserved
				 * 64   reserved
				 * 128  reserved
				 */

	/* end of session cases */
	long		lastsector;
	struct cdrom_paranoia	*p;
	struct linked_element	*e;
} c_block;

extern void	free_c_block		__PR((c_block * c));
extern void	i_cblock_destructor	__PR((c_block * c));
extern c_block	*new_c_block		__PR((struct cdrom_paranoia * p));

typedef struct v_fragment {
	c_block		*one;

	long		begin;
	long		size;
	Int16_t		*vector;

	/* end of session cases */
	long		lastsector;

	/* linked list */
	struct cdrom_paranoia	*p;
	struct linked_element	*e;

} v_fragment;

extern void	free_v_fragment		__PR((v_fragment * c));
extern v_fragment *new_v_fragment	__PR((struct cdrom_paranoia * p,
						c_block * one,
						long begin, long end,
						int lastsector));
extern Int16_t	*v_buffer		__PR((v_fragment * v));

extern c_block	*c_first		__PR((struct cdrom_paranoia * p));
extern c_block	*c_last			__PR((struct cdrom_paranoia * p));
extern c_block	*c_next			__PR((c_block * c));
extern c_block	*c_prev			__PR((c_block * c));

extern v_fragment *v_first		__PR((struct cdrom_paranoia * p));
extern v_fragment *v_last		__PR((struct cdrom_paranoia * p));
extern v_fragment *v_next		__PR((v_fragment * v));
extern v_fragment *v_prev		__PR((v_fragment * v));

typedef struct root_block {
	long		returnedlimit;
	long		lastsector;
	struct cdrom_paranoia	*p;

	c_block		*vector;	/* doesn't use any sorting */
	int		silenceflag;
	long		silencebegin;
} root_block;

typedef struct offsets {
	long	offpoints;
	long	newpoints;
	long	offaccum;
	long	offdiff;
	long	offmin;
	long	offmax;

} offsets;

typedef struct cdrom_paranoia {
	void	*d;		/* A pointer to the driver interface */

	long	(*d_read) __PR((void *d, void *buffer,		/* -> long sectors */
			long beginsector, long sectors));
	long	(*d_disc_firstsector) __PR((void *d));		/* -> long sector */
	long	(*d_disc_lastsector) __PR((void *d));		/* -> long sector */
	int	(*d_tracks)	__PR((void *d));		/* -> int tracks */
	long	(*d_track_firstsector) __PR((void *d, int track)); /* -> long sector */
	long	(*d_track_lastsector) __PR((void *d, int track)); /* -> long sector */
	int	(*d_sector_gettrack) __PR((void *d, long sector)); /* -> int trackno */
	int	(*d_track_audiop) __PR((void *d, int track));	/* -> int Is audiotrack */

	int		nsectors;	/* # of sectors that fit into DMA buf */
	int		sectsize;	/* size of a sector, 2353 or 2646 bytes */
	int		sectwords;	/* words in a sector, 2353/2 or 2646/2 */

	root_block	root;		/* verified/reconstructed cached data */
	linked_list	*cache;		/* our data as read from the cdrom */
	long		cache_limit;
	linked_list	*fragments;	/* fragments of blocks that have been */
					/* 'verified' */
	sort_info	*sortcache;

	int		readahead;	/* sectors of readahead in each readop */
	int		jitter;
	long		lastread;

	int		enable;		/* modes from paranoia_modeset() */
	long		cursor;
	long		current_lastsector;
	long		current_firstsector;

	/* statistics for drift/overlap */
	struct offsets	stage1;
	struct offsets	stage2;

	long		mindynoverlap;
	long		maxdynoverlap;
	long		dynoverlap;
	long		dyndrift;

	/* statistics for verification */

} cdrom_paranoia;

extern c_block	*c_alloc		__PR((Int16_t * vector,
						long begin, long size));
extern void	c_set			__PR((c_block * v, long begin));
extern void	c_insert		__PR((c_block * v, long pos,
						Int16_t * b, long size));
extern void	c_remove		__PR((c_block * v, long cutpos,
						long cutsize));
extern void	c_overwrite		__PR((c_block * v, long pos,
						Int16_t * b, long size));
extern void	c_append		__PR((c_block * v, Int16_t * vector,
						long size));
extern void	c_removef		__PR((c_block * v, long cut));

#define	ce(v)	((v)->begin + (v)->size)
#define	cb(v)	((v)->begin)
#define	cs(v)	((v)->size)

/*
 * pos here is vector position from zero
 */
extern void	recover_cache		__PR((cdrom_paranoia * p));
extern void	i_paranoia_firstlast	__PR((cdrom_paranoia * p));

#define	cv(c)	((c)->vector)

#define	fe(f)	((f)->begin + (f)->size)
#define	fb(f)	((f)->begin)
#define	fs(f)	((f)->size)
#define	fv(f)	(v_buffer(f))

#define	CDP_COMPILE
#endif	/* _P_BLOCK_H */
