/* @(#)resample.c	1.36 16/02/14 Copyright 1998-2000,2015 Heiko Eissfeldt, Copyright 2004-2010 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)resample.c	1.36 16/02/14 Copyright 1998-2000,2015 Heiko Eissfeldt, Copyright 2004-2010 J. Schilling";
#endif
/*
 * resampling module
 *
 * The audio data has been read. Here are the
 * functions to ensure a correct continuation
 * of the output stream and to convert to a
 * lower sample rate.
 *
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

#undef DEBUG_VOTE_ENDIANESS
#undef DEBUG_SHIFTS		/* simulate bad cdrom drives */
#undef DEBUG_MATCHING
#undef SHOW_JITTER
#undef CHECK_MEM

#include "config.h"
#include <schily/time.h>
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/utypes.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/limits.h>
#include <schily/assert.h>
#include <schily/math.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include <scg/scsitransp.h>

#include "mytype.h"
#include "cdda2wav.h"
#include "interface.h"
#include "byteorder.h"
#include "ringbuff.h"
#include "resample.h"
#include "toc.h"
#include "sndfile.h"
#include "sndconfig.h"
#include "global.h"
#include "exitcodes.h"


int waitforsignal = 0;	/* flag: wait for any audio response */
int any_signal = 0;

short undersampling;	/* conversion factor */
short samples_to_do;	/* loop variable for conversion */
int Halved;		/* interpolate due to non integral divider */

static long lsum = 0, rsum = 0;		/* accumulator for left/right channel */
static long ls2 = 0, rs2 = 0, ls3 = 0, rs3 = 0, auxl = 0, auxr = 0;

static const unsigned char *my_symmemmem __PR((const unsigned char *HAYSTACK,
					const size_t HAYSTACK_LEN,
					const unsigned char *const NEEDLE,
					const size_t NEEDLE_LEN));
static const unsigned char *my_memmem	__PR((const unsigned char *HAYSTACK,
					const size_t HAYSTACK_LEN,
					const unsigned char *const NEEDLE,
					const size_t NEEDLE_LEN));
static const unsigned char *my_memrmem	__PR((const unsigned char *HAYSTACK,
					const size_t HAYSTACK_LEN,
					const unsigned char *const NEEDLE,
					const size_t NEEDLE_LEN));
static const unsigned char *sync_buffers __PR((const unsigned char *const newbuf));
static long interpolate			__PR((long p1, long p2, long p3));
static void emit_sample			__PR((long lsumval, long rsumval,
						long channels));
static void change_endianness		__PR((UINT4 *pSam,
						unsigned int Samples));
static void swap_channels		__PR((UINT4 *pSam,
						unsigned int Samples));
static int guess_endianess		__PR((UINT4 *p, Int16_t *p2,
						unsigned int SamplesToDo));


#ifdef CHECK_MEM
static void
check_mem __PR((const unsigned char *p, unsigned long amount,
				const unsigned char *q,
				unsigned line, char *file));

static void
check_mem(p, amount, q, line, file)
	const unsigned char	*p;
	unsigned long		amount;
	const unsigned char	*q;
	unsigned		line;
	char			*file;
{
	if (p < q || p+amount > q + ENTRY_SIZE) {
		errmsgno(EX_BAD,
		_("File %s, line %u: invalid buffer range (%p - %p), allowed is (%p - %p).\n"),
			file, line, p, p+amount-1, q, q + ENTRY_SIZE-1);
		exit(INTERNAL_ERROR);
	}
}
#endif


#ifdef DEBUG_MATCHING
int
memcmp(const void * a, const void * b, size_t c)
{
	return (1);
}
#endif

static const unsigned char *
my_symmemmem(HAYSTACK, HAYSTACK_LEN, NEEDLE, NEEDLE_LEN)
	const unsigned char	*HAYSTACK;
	const size_t		HAYSTACK_LEN;
	const unsigned char	*const NEEDLE;
	const size_t		NEEDLE_LEN;
{
	const unsigned char	*const UPPER_LIMIT = HAYSTACK +
							HAYSTACK_LEN -
							NEEDLE_LEN - 1;
	const unsigned char	*HAYSTACK2 = HAYSTACK-1;

	while (HAYSTACK <= UPPER_LIMIT) {
		if (memcmp(NEEDLE, HAYSTACK, NEEDLE_LEN) == 0) {
			return (HAYSTACK);
		} else {
			if (memcmp(NEEDLE, HAYSTACK2, NEEDLE_LEN) == 0) {
				return (HAYSTACK2);
			}
			HAYSTACK2--;
			HAYSTACK++;
		}
	}
#ifdef DEBUG_MATCHING
	HAYSTACK2++;
	HAYSTACK--;
	fprintf(stderr, "scompared %p-%p with %p-%p (%p)\n",
			NEEDLE, NEEDLE + NEEDLE_LEN-1,
			HAYSTACK2, HAYSTACK + NEEDLE_LEN-1, HAYSTACK);
#endif
	return (NULL);
}

static const unsigned char *
my_memmem(HAYSTACK, HAYSTACK_LEN, NEEDLE, NEEDLE_LEN)
	const unsigned char	*HAYSTACK;
	const size_t		HAYSTACK_LEN;
	const unsigned char	*const NEEDLE;
	const size_t		NEEDLE_LEN;
{
	const unsigned char	*const UPPER_LIMIT = HAYSTACK +
							HAYSTACK_LEN -
							NEEDLE_LEN;

	while (HAYSTACK <= UPPER_LIMIT) {
		if (memcmp(NEEDLE, HAYSTACK, NEEDLE_LEN) == 0) {
			return (HAYSTACK);
		} else {
			HAYSTACK++;
		}
	}
#ifdef DEBUG_MATCHING
	HAYSTACK--;
	fprintf(stderr, "fcompared %p-%p with %p-%p (%p)\n",
		NEEDLE, NEEDLE + NEEDLE_LEN-1,
		HAYSTACK - HAYSTACK_LEN + NEEDLE_LEN,
		HAYSTACK + NEEDLE_LEN-1,
		HAYSTACK);
#endif
	return (NULL);
}

static const unsigned char *
my_memrmem(HAYSTACK, HAYSTACK_LEN, NEEDLE, NEEDLE_LEN)
	const unsigned char	*HAYSTACK;
	const size_t		HAYSTACK_LEN;
	const unsigned char	*const NEEDLE;
	const size_t		NEEDLE_LEN;
{
	const unsigned char	*const LOWER_LIMIT = HAYSTACK -
							(HAYSTACK_LEN - 1);

	while (HAYSTACK >= LOWER_LIMIT) {
		if (memcmp(NEEDLE, HAYSTACK, NEEDLE_LEN) == 0) {
			return (HAYSTACK);
		} else {
			HAYSTACK--;
		}
	}
#ifdef DEBUG_MATCHING
	HAYSTACK++;
	fprintf(stderr, "bcompared %p-%p with %p-%p (%p)\n",
		NEEDLE, NEEDLE + NEEDLE_LEN-1,
		HAYSTACK, HAYSTACK + (HAYSTACK_LEN - 1),
		HAYSTACK + (HAYSTACK_LEN - 1) - NEEDLE_LEN - 1);
#endif
	return (NULL);
}

/* find continuation in new buffer */
static const unsigned char *
sync_buffers(newbuf)
	const unsigned char	*const newbuf;
{
	const unsigned char *retval = newbuf;

	if (global.overlap != 0) {
		/*
		 * find position of SYNC_SIZE bytes
		 * of the old buffer in the new buffer
		 */
		size_t			haystack_len;
		const size_t		needle_len = SYNC_SIZE;
		const unsigned char	*const oldbuf =
					    (const unsigned char *)
					    (get_previous_read_buffer()->data);
		const unsigned char	*haystack;
		const unsigned char	*needle;

		/*
		 * compare the previous buffer with the new one
		 *
		 * 1. symmetrical search:
		 *   look for the last SYNC_SIZE bytes of the previous buffer
		 *   in the new buffer (from the optimum to the outer
		 *   positions).
		 *
		 * 2. if the first approach did not find anything do forward
		 *   search look for the last SYNC_SIZE bytes of the previous
		 *   buffer in the new buffer (from behind the overlap to the
		 *   end).
		 */
		haystack_len = min((global.nsectors - global.overlap) *
					CD_FRAMESIZE_RAW
					+SYNC_SIZE+1,
					global.overlap*CD_FRAMESIZE_RAW);
		/*
		 * expected here
		 */
		haystack = newbuf + CD_FRAMESIZE_RAW *
					global.overlap - SYNC_SIZE;
		needle = oldbuf + CD_FRAMESIZE_RAW*global.nsectors - SYNC_SIZE;

#ifdef DEBUG_MATCHING
		fprintf(stderr, "oldbuf    %p-%p  new %p-%p %u %u %u\n",
			oldbuf, oldbuf + CD_FRAMESIZE_RAW*global.nsectors - 1,
			newbuf, newbuf + CD_FRAMESIZE_RAW*global.nsectors - 1,
			CD_FRAMESIZE_RAW*global.nsectors, global.nsectors,
			global.overlap);
#endif

		retval = my_symmemmem(haystack, haystack_len, needle,
								needle_len);
		if (retval != NULL) {
			retval += SYNC_SIZE;
		} else {
			/*
			 * fallback to asymmetrical search
			 *
			 * if there is no asymmetrical part left,
			 * return with 'not found'
			 */
			if (2*global.overlap == global.nsectors) {
				retval = NULL;
			} else if (2*global.overlap > global.nsectors) {
				/*
				 * the asymmetrical part is in front,
				 * search backwards
				 */
				haystack_len = (2*global.overlap-global.nsectors)*CD_FRAMESIZE_RAW;
				haystack = newbuf + haystack_len - 1;
				retval = my_memrmem(haystack, haystack_len, needle, needle_len);
			} else {
				/*
				 * the asymmetrical part is at the end,
				 * search forward
				 */
				haystack = newbuf + 2*(global.overlap*CD_FRAMESIZE_RAW - SYNC_SIZE);
				haystack_len = (global.nsectors-2*global.overlap)*CD_FRAMESIZE_RAW + 2*SYNC_SIZE;
				retval = my_memmem(haystack, haystack_len, needle, needle_len);
			}
			if (retval != NULL)
				retval += SYNC_SIZE;
		}

#ifdef SHOW_JITTER
		if (retval) {
			fprintf(stderr, "%d\n",
			retval-(newbuf+global.overlap*CD_FRAMESIZE_RAW));
		} else {
			fprintf(stderr, _("no match\n"));
		}
#endif
	}
	return (retval);
}

/*
 * quadratic interpolation
 * p1, p3 span the interval 0 - 2. give interpolated value for 1/2
 */
static long int
interpolate(p1, p2, p3)
	long int	p1;
	long int	p2;
	long int	p3;
{
	return ((3L*p1 + 6L*p2 - p3)/8L);
}

static unsigned char *pStart;	/* running ptr defining end of output buffer */
static unsigned char *pDst;	/* start of output buffer */
/*
 * Write the filtered sample into the output buffer.
 */
static void
emit_sample(lsumval, rsumval, channels)
	long	lsumval;
	long	rsumval;
	long	channels;
{
	if (global.findminmax) {
		if (rsumval > global.maxamp[0])
			global.maxamp[0] = rsumval;
		if (rsumval < global.minamp[0])
			global.minamp[0] = rsumval;
		if (lsumval < global.minamp[1])
			global.minamp[1] = lsumval;
		if (lsumval > global.maxamp[1])
			global.maxamp[1] = lsumval;
	}
	/*
	 * convert to output format
	 */
	if (channels == 1) {
		Int16_t	sum;

		/*
		 * mono section
		 */
		sum = (lsumval + rsumval) >> (global.sh_bits + 1);
		if (global.sh_bits == 8) {
			if (waitforsignal == 1) {
				if (any_signal == 0) {
					if (((char) sum) != '\0') {
						pStart = (unsigned char *) pDst;
						any_signal = 1;
						*pDst++ = (unsigned char) sum + (1 << 7);
					} else
						global.SkippedSamples++;
				} else
					*pDst++ = (unsigned char) sum + (1 << 7);
			} else
				*pDst++ = (unsigned char) sum + (1 << 7);
		} else {
			Int16_t	*myptr = (Int16_t *) pDst;

			if (waitforsignal == 1) {
				if (any_signal == 0) {
					if (sum != 0) {
						pStart = (unsigned char *) pDst;
						any_signal = 1;
						*myptr = sum; pDst += sizeof (Int16_t);
					} else
						global.SkippedSamples++;
				} else {
					*myptr = sum; pDst += sizeof (Int16_t);
				}
			} else {
				*myptr = sum; pDst += sizeof (Int16_t);
			}
		}

	} else {
		/*
		 * stereo section
		 */
		lsumval >>= global.sh_bits;
		rsumval >>= global.sh_bits;
		if (global.sh_bits == 8) {
			if (waitforsignal == 1) {
				if (any_signal == 0) {
					if ((((char) lsumval != '\0') || ((char) rsumval != '\0'))) {
						pStart = (unsigned char *) pDst;
						any_signal = 1;
						*pDst++ = (unsigned char)(short) lsumval + (1 << 7);
						*pDst++ = (unsigned char)(short) rsumval + (1 << 7);
					} else
						global.SkippedSamples++;
				} else {
					*pDst++ = (unsigned char)(short) lsumval + (1 << 7);
					*pDst++ = (unsigned char)(short) rsumval + (1 << 7);
				}
			} else {
				*pDst++ = (unsigned char)(short) lsumval + (1 << 7);
				*pDst++ = (unsigned char)(short) rsumval + (1 << 7);
			}
		} else {
			Int16_t	*myptr = (Int16_t *) pDst;

			if (waitforsignal == 1) {
				if (any_signal == 0) {
					if ((((Int16_t) lsumval != 0) || ((Int16_t) rsumval != 0))) {
						pStart = (unsigned char *) pDst;
						any_signal = 1;
						*myptr++ = (Int16_t) lsumval;
						*myptr   = (Int16_t) rsumval;
						pDst += 2*sizeof (Int16_t);
					} else
						global.SkippedSamples++;
				} else {
					*myptr++ = (Int16_t) lsumval;
					*myptr   = (Int16_t) rsumval;
					pDst += 2*sizeof (Int16_t);
				}
			} else {
				*myptr++ = (Int16_t) lsumval;
				*myptr   = (Int16_t) rsumval;
				pDst += 2*sizeof (Int16_t);
			}
		}
	}
}

static void
change_endianness(pSam, Samples)
	UINT4		*pSam;
	unsigned int	Samples;
{
	UINT4	*pend = (pSam + Samples);

	/*
	 * type UINT4 may not be greater than the assumed biggest type
	 */
#if (SIZEOF_LONG_INT < 4)
error type unsigned long is too small
#endif

#if (SIZEOF_LONG_INT == 4)

	unsigned long	*plong = (unsigned long *)pSam;

	for (; plong < pend; ) {
		*plong = ((*plong >> 8L) & UINT_C(0x00ff00ff)) |
			((*plong << 8L) & UINT_C(0xff00ff00));
		plong++;
	}
#else  /* sizeof long unsigned > 4 bytes */
#if (SIZEOF_LONG_INT == 8)
#define	INTEGRAL_LONGS	(SIZEOF_LONG_INT-1UL)
	register unsigned long	*plong;
	unsigned long		*pend0 = (unsigned long *) (((unsigned long) pend) & ~ INTEGRAL_LONGS);

	if (((unsigned long) pSam) & INTEGRAL_LONGS) {
		*pSam = ((*pSam >> 8L) & UINT_C(0x00ff00ff)) |
			((*pSam << 8L) & UINT_C(0xff00ff00));
		pSam++;
	}

	plong = (unsigned long *)pSam;

	for (; plong < pend0; ) {
		*plong = ((*plong >> 8L) & ULONG_C(0x00ff00ff00ff00ff)) |
			((*plong << 8L) & ULONG_C(0xff00ff00ff00ff00));
		plong++;
	}

	if (((unsigned long *) pend) != pend0) {
		UINT4	*pint = (UINT4 *) pend0;

		for (; pint < pend; ) {
			*pint = ((*pint >> 8) & UINT_C(0x00ff00ff)) |
				((*pint << 8) & UINT_C(0xff00ff00));
			pint++;
		}
	}
#else  /* sizeof long unsigned > 4 bytes but not 8 */
	{
	UINT4	*pint = pSam;

	for (; pint < pend; ) {
		*pint = ((*pint >> 8) & UINT_C(0x00ff00ff)) |
			((*pint << 8) & UINT_C(0xff00ff00));
		pint++;
	}
	}
#endif
#endif
}

static void
swap_channels(pSam, Samples)
	UINT4		*pSam;
	unsigned int	Samples;
{
	UINT4	*pend = (pSam + Samples);

	/*
	 * type UINT4 may not be greater than the assumed biggest type
	 */
#if (SIZEOF_LONG_INT < 4)
error type unsigned long is too small
#endif

#if (SIZEOF_LONG_INT == 4)

	unsigned long	*plong = (unsigned long *)pSam;

	for (; plong < pend; ) {
		*plong = ((*plong >> 16L) & UINT_C(0x0000ffff)) |
			((*plong << 16L) & UINT_C(0xffff0000));
		plong++;
	}
#else  /* sizeof long unsigned > 4 bytes */
#if (SIZEOF_LONG_INT == 8)
#define	INTEGRAL_LONGS	(SIZEOF_LONG_INT-1UL)
	register unsigned long	*plong;
	unsigned long		*pend0 = (unsigned long *) (((unsigned long) pend) & ~ INTEGRAL_LONGS);

	if (((unsigned long) pSam) & INTEGRAL_LONGS) {
		*pSam = ((*pSam >> 16L) & UINT_C(0x0000ffff)) |
			((*pSam << 16L) & UINT_C(0xffff0000));
		pSam++;
	}

	plong = (unsigned long *)pSam;

	for (; plong < pend0; ) {
		*plong = ((*plong >> 16L) & ULONG_C(0x0000ffff0000ffff)) |
			((*plong << 16L) & ULONG_C(0xffff0000ffff0000));
		plong++;
	}

	if (((unsigned long *) pend) != pend0) {
		UINT4	*pint = (UINT4 *) pend0;

		for (; pint < pend; ) {
			*pint = ((*pint >> 16L) & UINT_C(0x0000ffff)) |
				((*pint << 16L) & UINT_C(0xffff0000));
			pint++;
		}
	}
#else  /* sizeof long unsigned > 4 bytes but not 8 */
	{
	UINT4	*pint = pSam;

	for (; pint < pend; ) {
		*pint = ((*pint >> 16L) & UINT_C(0x0000ffff)) |
			((*pint << 16L) & UINT_C(0xffff0000));
		pint++;
	}
	}
#endif
#endif
}

#ifdef	ECHO_TO_SOUNDCARD
static long ReSampleBuffer __PR((unsigned char *p, unsigned char *newp,
						long samples, int samplesize));
static long
ReSampleBuffer(p, newp, samples, samplesize)
	unsigned char	*p;
	unsigned char	*newp;
	long		samples;
	int		samplesize;
{
	UINT4	di = 0;

	if (global.playback_rate == 100.0) {
		memcpy(newp, p, samplesize* samples);
		di = samples;
	} else {
		UINT4	si = 0;
		double	idx = 0.0;

		while (si < (UINT4)samples) {
			memcpy(newp+(di*samplesize), p+(si*samplesize),
								samplesize);
			idx += (double)(global.playback_rate/100.0);
			si = (UINT4)idx;
			di++;
		}
	}
	return ((long)di*samplesize);
}
#endif

static int
guess_endianess(p, p2, SamplesToDo)
	UINT4		*p;
	Int16_t		*p2;
	unsigned	SamplesToDo;
{
	/*
	 * analyse samples
	 */
	int	vote_for_little = 0;
	int	vote_for_big = 0;
	int	total_votes;

	while (((UINT4 *)p2 - p) + (unsigned) 1 < SamplesToDo) {
		unsigned char	*p3 = (unsigned char *)p2;
#if MY_LITTLE_ENDIAN == 1
		int diff_lowl = *(p2+0) - *(p2+2);
		int diff_lowr = *(p2+1) - *(p2+3);
		int diff_bigl = ((*(p3)   << 8) + *(p3+1)) - ((*(p3+4) << 8) + *(p3+5));
		int diff_bigr = ((*(p3+2) << 8) + *(p3+3)) - ((*(p3+6) << 8) + *(p3+7));
#else
		int diff_lowl = ((*(p3+1) << 8) + *(p3))   - ((*(p3+5) << 8) + *(p3+4));
		int diff_lowr = ((*(p3+3) << 8) + *(p3+2)) - ((*(p3+7) << 8) + *(p3+6));
		int diff_bigl = *(p2+0) - *(p2+2);
		int diff_bigr = *(p2+1) - *(p2+3);
#endif

		if ((abs(diff_lowl) + abs(diff_lowr)) <
		    (abs(diff_bigl) + abs(diff_bigr))) {
			vote_for_little++;
		} else {
			if ((abs(diff_lowl) + abs(diff_lowr)) >
			    (abs(diff_bigl) + abs(diff_bigr))) {
				vote_for_big++;
			}
		}
		p2 += 2;
	}
#ifdef DEBUG_VOTE_ENDIANESS
	if (global.quiet != 1) {
		fprintf(stderr,
		"votes for little: %4d,  votes for big: %4d\n",
			vote_for_little, vote_for_big);
	}
#endif
	total_votes = vote_for_big + vote_for_little;
	if (total_votes < 3 ||
	    abs(vote_for_big - vote_for_little) < total_votes/3) {
		return (-1);
	} else {
		if (vote_for_big > vote_for_little)
			return (1);
		else
			return (0);
	}
}

int	jitterShift = 0;

void
handle_inputendianess(p, SamplesToDo)
	UINT4		*p;
	unsigned	SamplesToDo;
{
	/*
	 * if endianess is unknown, guess endianess based on
	 * differences between succesive samples. If endianess
	 * is correct, the differences are smaller than with the
	 * opposite byte order.
	 */
	if ((*in_lendian) < 0) {
		Int16_t	*p2 = (Int16_t *)p;

		/*
		 * skip constant samples
		 */
		while ((((UINT4 *)p2 - p) + (unsigned) 1 < SamplesToDo) &&
						*p2 == *(p2+2))
			p2++;

		if (((UINT4 *)p2 - p) + (unsigned) 1 < SamplesToDo) {
			switch (guess_endianess(p, p2, SamplesToDo)) {
			case -1: break;
			case  1: (*in_lendian) = 0;
#if 0
				if (global.quiet != 1) {
					fprintf(stderr,
						"big endian detected\n");
				}
#endif
				break;
			case  0: (*in_lendian) = 1;
#if 0
				if (global.quiet != 1) {
					fprintf(stderr,
						"little endian detected\n");
				}
#endif
				break;
			}
		}
	}

	/*
	 * ENDIAN ISSUES:
	 * the individual endianess of cdrom/cd-writer, cpu,
	 * sound card and audio output format need a careful treatment.
	 *
	 * For possible sample processing (rate conversion) we need
	 * the samples in cpu byte order. This is the first conversion.
	 *
	 * After processing it depends on the endianness of the output
	 * format, whether a second conversion is needed.
	 *
	 */

	if (global.need_hostorder && (*in_lendian) != MY_LITTLE_ENDIAN) {
		/*
		 * change endianess of delivered samples to native cpu order
		 */
		change_endianness(p, SamplesToDo);
	}
}

unsigned char *
synchronize(p, SamplesToDo, TotSamplesDone)
	UINT4		*p;
	unsigned	SamplesToDo;
	unsigned	TotSamplesDone;
{
	char	*pSrc;		/* start of cdrom buffer */

	/*
	 * synchronisation code
	 */
	if (TotSamplesDone != 0 && global.overlap != 0 &&
	    SamplesToDo > CD_FRAMESAMPLES) {

		pSrc = (char *) sync_buffers((unsigned char *)p);
		if (!pSrc) {
			return (NULL);
		}
		if (pSrc) {
			int	jitter;

			jitter = ((unsigned char *)pSrc -
				(((unsigned char *)p) +
				global.overlap*CD_FRAMESIZE_RAW))/4;
			jitterShift += jitter;
			SamplesToDo -= jitter + global.overlap*CD_FRAMESAMPLES;
#if 0
			fprintf(stderr,
			"Length: pre %d, diff1 %ld, diff2 %ld, min %ld\n",
			SamplesToDo,
			(TotSamplesWanted - TotSamplesDone),
			SamplesNeeded((TotSamplesWanted - TotSamplesDone),
			undersampling),
			min(SamplesToDo,
			    SamplesNeeded((TotSamplesWanted - TotSamplesDone),
			    undersampling)));
#endif
		}
	} else {
		pSrc = (char *) p;
	}
	return ((unsigned char *) pSrc);
}

/*
 * convert cdda data to required output format
 * sync code for unreliable cdroms included
 */
long
SaveBuffer(p, SamplesToDo, TotSamplesDone)
	UINT4		*p;
	unsigned long	SamplesToDo;
	unsigned long	*TotSamplesDone;
{
	UINT4	*pSrc;		/* start of cdrom buffer */
	UINT4	*pSrcStop;	/* end of cdrom buffer */

	/*
	 * in case of different endianness between host and output format,
	 * or channel swaps, or deemphasizing
	 * copy in a separate buffer and modify the local copy
	 */
	if (((((!global.need_hostorder &&
	    global.need_big_endian == (*in_lendian)) ||
	    (global.need_hostorder && global.need_big_endian != MY_BIG_ENDIAN)) ||
	    (global.deemphasize != 0)) && (global.OutSampleSize > 1)) ||
	    global.swapchannels != 0) {
		static UINT4	*localoutputbuffer;

		if (localoutputbuffer == NULL) {
			localoutputbuffer = malloc(global.nsectors*CD_FRAMESIZE_RAW);
			if (localoutputbuffer == NULL) {
				errmsg(_("Cannot allocate local buffer.\n"));
				return (1);
			}
		}
		memcpy(localoutputbuffer, p, SamplesToDo*4);
		p = localoutputbuffer;
	}

	pSrc = p;
	pDst = (unsigned char *) p;
	pStart = (unsigned char *) pSrc;
	pSrcStop = pSrc + SamplesToDo;

	/*
	 * code for subsampling and output stage
	 */
	if (global.ismono && global.findmono) {
		Int16_t	*pmm;

		for (pmm = (Int16_t *)pStart;
				(UINT4 *) pmm < pSrcStop; pmm += 2) {
			if (*pmm != *(pmm+1)) {
				global.ismono = 0;
				break;
			}
		}
	}

	/*
	 * optimize the case of no conversion
	 */
	if (1 && undersampling == 1 && samples_to_do == 1 &&
	    global.channels == 2 && global.OutSampleSize == 2 && Halved == 0) {
		/*
		 * output format is the original cdda format ->
		 * just forward the buffer
		 */
		if (waitforsignal != 0 && any_signal == 0) {
			UINT4	*myptr = (UINT4 *)pStart;

			while (myptr < pSrcStop && *myptr == 0)
				myptr++;
			pStart = (unsigned char *) myptr;
			/*
			 * scan for first signal
			 */
			if ((UINT4 *)pStart != pSrcStop) {
				/*
				 * first non null amplitude is found in buffer
				 */
				any_signal = 1;
				global.SkippedSamples += ((UINT4 *)pStart - p);
			} else {
				global.SkippedSamples += (pSrcStop - p);
			}
		}
		pDst = (unsigned char *) pSrcStop;	/* set pDst to end */

		if (global.deemphasize &&
		    (Get_Preemphasis(get_current_track_writing()))) {
			/*
			 * this implements an attenuation treble shelving
			 * filter to undo the effect of pre-emphasis.
			 * The filter is of a recursive first order
			 */
			static Int16_t	lastin[2] = { 0, 0 };
			static double	lastout[2] = { 0.0, 0.0 };
			Int16_t		*pmm;

			/*
			 * Here is the gnuplot file for the frequency response
			 * of the deemphasis. The error is below +-0.1dB
			 */
#ifdef	GNU_PLOT_PROGRAM
/* BEGIN CSTYLED */
/*
# first define the ideal filter. We use the tenfold sampling frequency.
T=1./441000.
OmegaU=1./15E-6
OmegaL=15./50.*OmegaU
V0=OmegaL/OmegaU
H0=V0-1.
B=V0*tan(OmegaU*T/2.)
# the coefficients follow
a1=(B - 1.)/(B + 1.)
b0=(1.0 + (1.0 - a1) * H0/2.)
b1=(a1 + (a1 - 1.0) * H0/2.)
# helper variables
D=b1/b0
o=2*pi*T
H2(f)=b0*sqrt((1+2*cos(f*o)*D+D*D)/(1+2*cos(f*o)*a1+a1*a1))
# now approximate the ideal curve with a fitted one for sampling frequency
# of 44100 Hz.
T2=1./44100.
V02=0.3365
OmegaU2=1./19E-6
B2=V02*tan(OmegaU2*T2/2.)
# the coefficients follow
a12=(B2 - 1.)/(B2 + 1.)
b02=(1.0 + (1.0 - a12) * (V02-1.)/2.)
b12=(a12 + (a12 - 1.0) * (V02-1.)/2.)
# helper variables
D2=b12/b02
o2=2*pi*T2
H(f)=b02*sqrt((1+2*cos(f*o2)*D2+D2*D2)/(1+2*cos(f*o2)*a12+a12*a12))
# plot best, real, ideal, level with halved attenuation,
# level at full attentuation, 10fold magnified error
set logscale x
set grid xtics ytics mxtics mytics
plot [f=1000:20000] [-12:2] 20*log10(H(f)),20*log10(H2(f)),  20*log10(OmegaL/(2*pi*f)), 0.5*20*log10(V0), 20*log10(V0), 200*log10(H(f)/H2(f))
pause -1 "Hit return to continue"
*/
/* END CSTYLED */
#endif

#ifdef TEST
#define	V0	0.3365
#define	OMEGAG	(1./19e-6)
#define	T	(1./44100.)
#define	H0	(V0-1.)
#define	B	(V0*tan((OMEGAG * T)/2.0))
#define	a1	((B - 1.)/(B + 1.))
#define	b0 	(1.0 + (1.0 - a1) * H0/2.0)
#define	b1 	(a1 + (a1 - 1.0) * H0/2.0)
#undef	V0
#undef	OMEGAG
#undef	T
#undef	H0
#undef	B
#else
#define	a1	-0.62786881719628784282
#define	b0 	0.45995451989513153057
#define	b1 	-0.08782333709141937339
#endif

			for (pmm = (Int16_t *)pStart; pmm < (Int16_t *)pDst; ) {
				lastout[0] = *pmm * b0 + lastin[0] * b1 - lastout[0] * a1;
				lastin[0] = *pmm;
				*pmm++ = lastout[0] > 0.0 ? lastout[0] + 0.5 : lastout[0] - 0.5;
				lastout[1] = *pmm * b0 + lastin[1] * b1 - lastout[1] * a1;
				lastin[1] = *pmm;
				*pmm++ = lastout[1] > 0.0 ? lastout[1] + 0.5 : lastout[1] - 0.5;
			}
#undef	a1
#undef	b0
#undef	b1
		}

		if (global.swapchannels == 1) {
			swap_channels((UINT4 *)pStart, SamplesToDo);
		}

		if (global.findminmax) {
			Int16_t	*pmm;

			for (pmm = (Int16_t *)pStart;
						pmm < (Int16_t *)pDst; pmm++) {
				if (*pmm < global.minamp[1])
					global.minamp[1] = *pmm;
				if (*pmm > global.maxamp[1])
					global.maxamp[1] = *pmm;
				pmm++;
				if (*pmm < global.minamp[0])
					global.minamp[0] = *pmm;
				if (*pmm > global.maxamp[0])
					global.maxamp[0] = *pmm;
			}
		}
	} else {

#define	none_missing	0
#define	one_missing	1
#define	two_missing	2
#define	collecting	3

		static int	sample_state = collecting;
		static int	Toggle_on = 0;

		if (global.channels == 2 && global.swapchannels == 1) {
			swap_channels((UINT4 *)pStart, SamplesToDo);
		}

		/*
		 * conversion required
		 */
		while (pSrc < pSrcStop) {
			long	l;
			long	r;

			long iSamples_left = (pSrcStop - pSrc) / sizeof (Int16_t) / 2;
			Int16_t *myptr = (Int16_t *) pSrc;

			/*
			 * LSB l, MSB l
			 */
			l = *myptr++;	/* left channel */
			r = *myptr++;	/* right channel */
			pSrc = (UINT4 *) myptr;

			switch (sample_state) {
			case two_missing:
two__missing:
				ls2 += l; rs2 += r;
				if (undersampling > 1) {
					ls3 += l; rs3 += r;
				}
				sample_state = one_missing;
				break;
			case one_missing:
				auxl = l; auxr = r;

				ls3 += l; rs3 += r;
				sample_state = none_missing;

				/* FALLTHROUGH */
none__missing:
			case none_missing:
				/*
				 * Filtered samples are complete.
				 * Now interpolate and scale.
				 */
				if (Halved != 0 && Toggle_on == 0) {
					lsum = interpolate(lsum, ls2, ls3)/(int) undersampling;
					rsum = interpolate(rsum, rs2, rs3)/(int) undersampling;
				} else {
					lsum /= (int) undersampling;
					rsum /= (int) undersampling;
				}
				emit_sample(lsum, rsum, global.channels);
				/*
				 * reload counter
				 */
				samples_to_do = undersampling - 1;
				lsum = auxl;
				rsum = auxr;
				/*
				 * reset sample register
				 */
				auxl = ls2 = ls3 = 0;
				auxr = rs2 = rs3 = 0;
				Toggle_on ^= 1;
				sample_state = collecting;
				break;

			case collecting:
				if (samples_to_do > 0) {
					samples_to_do--;
					if (Halved != 0 && Toggle_on == 0) {
						/*
						 * Divider x.5 : we need data
						 * for quadratic interpolation
						 */
						iSamples_left--;

						lsum += l; rsum += r;
						if (samples_to_do < undersampling - 1) {
							ls2 += l; rs2 += r;
						}
						if (samples_to_do < undersampling - 2) {
							ls3 += l; rs3 += r;
						}
					} else {
						/*
						 * integral divider
						 */
						lsum += l;
						rsum += r;
						iSamples_left--;
					}
				} else {
					if (Halved != 0 && Toggle_on == 0) {
						sample_state = two_missing;
						goto two__missing;
					} else {
						auxl = l;
						auxr = r;
						sample_state = none_missing;
						goto none__missing;
					}
				}
				break;
			} /* switch state */
		} /* while */

		/*
		 * flush_buffer
		 */
		if ((samples_to_do == 0 && Halved == 0)) {
			if (Halved != 0 && Toggle_on == 0) {
				lsum = interpolate(lsum, ls2, ls3)/(int) undersampling;
				rsum = interpolate(rsum, rs2, rs3)/(int) undersampling;
			} else {
				lsum /= (int) undersampling;
				rsum /= (int) undersampling;
			}
			emit_sample(lsum, rsum, global.channels);

			/*
			 * reload counter
			 */
			samples_to_do = undersampling;

			/*
			 * reset sample register
			 */
			lsum = auxl = ls2 = ls3 = 0;
			rsum = auxr = rs2 = rs3 = 0;
			Toggle_on ^= 1;
			sample_state = collecting;
		}

	} /* if optimize else */

	if (waitforsignal == 0)
		pStart = (unsigned char *)p;

	if (waitforsignal == 0 || any_signal != 0) {
		int		retval = 0;
		unsigned	outlen;
		unsigned	todo;

		assert(pDst >= pStart);
		outlen = (size_t) (pDst - pStart);

		if (outlen == 0)
			return (0);

#ifdef	ECHO_TO_SOUNDCARD
		/*
		 * this assumes the soundcard needs samples in native
		 * cpu byte order
		 */
		if (global.echo != 0) {
			static unsigned char	*newp;
				unsigned	newlen;

			newlen = (100*(outlen/4))/global.playback_rate;
			newlen = (newlen*4);
			if ((newp != NULL) || (newp = malloc(2*global.nsectors*CD_FRAMESIZE_RAW+32))) {
				newlen = ReSampleBuffer(pStart, newp, outlen/4, global.OutSampleSize*global.channels);
				write_snd_device((char *)newp, newlen);
			}
		}
#endif
		if (global.no_file != 0) {
			*TotSamplesDone += SamplesToDo;
			return (0);
		}
		if ((!global.need_hostorder && global.need_big_endian == (*in_lendian)) ||
		    (global.need_hostorder && global.need_big_endian != MY_BIG_ENDIAN)) {
			if (global.OutSampleSize > 1) {
				/*
				 * change endianness from input sample or native cpu order
				 * to required output endianness
				 */
				change_endianness((UINT4 *)pStart, outlen/4);
			}
		}
		{
			unsigned char	*p2 = pStart;

			todo = outlen;
			while (todo != 0) {
#ifdef	MD5_SIGNATURES
				if (global.md5count) {
					global.md5size += todo;
					MD5Update(global.context, p2, todo);
					if (global.md5blocksize > 0)
						global.md5count -= todo;
				}
#endif
				retval = global.audio_out->WriteSound(global.audio, p2, todo);
				if (retval < 0)
					break;
				p2 += retval;
				todo -= retval;
			}
		}
		if (todo == 0) {
			*TotSamplesDone += SamplesToDo;
			return (0);
		} else {
			errmsg(
			_("Error in write(audio, 0x%p, %u) = %d\nProbably disk space exhausted.\n"),
				pStart, outlen, retval);
			return (1);
		}
	} else {
		*TotSamplesDone += SamplesToDo;
		return (0);
	}
}
