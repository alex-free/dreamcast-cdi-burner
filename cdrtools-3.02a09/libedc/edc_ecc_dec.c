/* @(#)edc_ecc_dec.c	1.14 16/04/01 Copyright 1998-2001 Heiko Eissfeldt, Copyright 2006-2013 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)edc_ecc_dec.c	1.14 16/04/01 Copyright 1998-2001 Heiko Eissfeldt, Copyright 2006-2013 J. Schilling";
#endif

/*
 * This file contains protected intellectual property.
 *
 * reed-solomon encoder / decoder for compact discs.
 *
 * Copyright 1998-2001 by Heiko Eissfeldt
 * Copyright 2006-2013 by Joerg Schilling
 */
/*@@C@@*/

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <assert.h>
#include <schily/utypes.h>

#define	EDC_DECODER_HACK	/* Hack to allow using edc.h with ecc.h */
#define	EDC_DECODER
#define	EDC_SUBCHANNEL
#define	EDC_LAYER2
#include "edc.h"

#ifndef	HAVE_MEMMOVE
#include <schily/libport.h>	/* Define missing prototypes */
/*#define	memmove(dst, src, size)		movebytes((src), (dst), (size))*/
#define	memmove(d, s, n)	bcopy((s), (d), (n))
#endif

#include "edc_code_tables"

#define	DEBUG	-2
#define	TELLME	0

#if	DEBUG < 0
#define	NDEBUG	1	/* turn assertions off */
#endif

#ifdef	EDC_DECODER

/* macros for high level functionality */

/* initialize the Berlekamp-Massey decoder */
#define	INIT_BMD \
	memset(err_locations, 0, sizeof (err_locations));			\
	memset(err_values, 0, sizeof (err_values));				\
	memset(tmp_loc, 0, sizeof (tmp_loc));					\
	err_locations[0] = 1;							\
	err_values[0] = 1;

/* create an error location polynomial from given erasures. */
#define	INIT_ERASURES(ERAS, ERRORS, MODUL, LOG, ALOG)				\
		err_locations[1] = ALOG[MODUL - (ERAS[0]+1)];			\
		for (i = 1; i < min(erasures, ERRORS*2); i++) {			\
			for (j = i+1; j > 0; j--) { 				\
				if (err_locations[j - 1] != 0) {		\
					err_locations[j] ^= ALOG[		\
						LOG[err_locations[j - 1]]	\
						+ MODUL - (ERAS[i]+1)];		\
				}						\
			}							\
		}

/* the condition for ignoring this root */
#define	FIND_ROOTS_TABOO(SECTORMACRO, UNIT, SKIPPED) \
			((SECTORMACRO(UNIT, (i-SKIPPED))) >= low_taboo &&	\
			(SECTORMACRO(UNIT, (i-SKIPPED))) <= high_taboo)

/* debugging code for the error location polynomial. */
#define	DEBUG_ERASURE_POLY(LOG, ALOG, MODUL, ERRORS, SKIPPED, TABOO) \
		if (DEBUG > 4) { \
			fprintf(stderr, " Errloc coeff.: "); \
			for (i = 0; i <= min(erasures, ERRORS); i++) { \
				fprintf(stderr, "%d (%d), ", err_locations[i], LOG[err_locations[i]]); \
			} \
			fprintf(stderr, "\n"); \
		} \
		for (roots_found = 0, i = SKIPPED; i < MODUL; i++) { \
			int pval = 0; \
			if (TABOO) { \
				fprintf(stderr, "QLOCK(%u)\t", i-SKIPPED); \
				continue; \
			} \
			for (j = min(erasures, ERRORS*2); ; j--) { \
				if (err_locations[j] != 0) \
					pval ^= ALOG[LOG[err_locations[j]] + j*(i+1) % MODUL ]; \
				if (j == 0) \
					break; \
			} \
			if (pval != 0) continue; \
			roots_found++; \
			if (DEBUG > 4) fprintf(stderr, "root at %d\n", i - SKIPPED); \
		} \
		if (min(erasures, ERRORS*2) != roots_found) \
			fprintf(stderr, "error location poly is wrong erasures=%d, roots=%d\n", erasures, roots_found);

/* calculate the 'discrepancy' in the Berlekmap-Massey decoder. */
#define	CALC_DISCREPANCY(LOG, ALOG) \
		discrepancy = SYND(j); \
		for (i = 1; i <= j; i++) { \
			if (err_locations[i] == 0 || SYND(j-i) == 0) \
				continue; \
			discrepancy ^= \
				ALOG[LOG[err_locations[i]]+ \
					    LOG[SYND(j-i)]]; \
		}

/* build a stage of the shift register. */
#define	BUILD_SHIFT_REGISTER(LOG, ALOG, MODUL, ERRORS) \
			int	ld = LOG[discrepancy]; \
			if (l+l <= j + erasures) { \
				/*						\
				 * complex case			\
				 * err_locations and tmp_loc needs to be changed \
				 */						\
				unsigned char	tmp_errl[2*ERRORS+1];		\
				int		lnd = MODUL - ld;		\
				/*						\
				 * change shift register length			\
				 */						\
				memcpy(tmp_errl, err_locations+k, l);		\
				for (i = 0; i < k + l; i++) {			\
					int	c = err_locations[i];		\
					if (i < l) {				\
						int	e = tmp_loc[i];		\
						if (e != 0) \
							tmp_errl[i] ^= ALOG[ LOG[e] + ld]; \
					}					\
					if (c != 0)				\
						tmp_loc[i] = ALOG[LOG[c] + lnd]; \
					else					\
						tmp_loc[i] = 0;			\
				}						\
				memcpy(err_locations+k, tmp_errl, l);		\
				l = j - l + erasures + 1;			\
				k = 0;						\
			} else {						\
				/*						\
				 * increase order of shift register		\
				 */						\
				for (i = k; i < k + l; i++) {			\
					if (tmp_loc[i-k] != 0)			\
						err_locations[i] ^= ALOG[ LOG[tmp_loc[i-k]] + ld]; \
				}						\
			}

/* do a chien search to find roots of the error location polynomial. */
#define	FIND_ROOTS(LOG, ALOG, MODUL, ERRORS, SKIPPED, TABOOMACRO) \
	for (roots_found = 0, i = SKIPPED; i < MODUL; i++) { \
		unsigned pval; \
		if (TABOOMACRO) { \
			continue; \
		} \
		pval = 0; \
		for (j = l; ; j--) { \
			if (err_locations[j] != 0) { \
				pval ^= ALOG[LOG[err_locations[j]] + j*(i+1) % MODUL ]; \
			} \
			if (j == 0) \
				break; \
		} \
		if (pval == 0) { \
			if (roots_found < 2*ERRORS) { \
				roots[roots_found] = (unsigned char)i; \
			} \
			roots_found++; \
if (DEBUG > 3) fprintf(stderr, "%d. root=%d\t", roots_found, i - SKIPPED); \
		} \
	}

/* calculate the error valuator. */
#define	CALC_ERROR_VALUATOR(LOG, ALOG) \
	for (j = 0; j < l; j++) { \
		err_values[j] = 0; \
		for (i = 0; i <= j; i++) { \
			if (err_locations[i] != 0 && SYND(j-i) != 0) \
				err_values[j] ^= ALOG[ LOG[err_locations[i]] + LOG[SYND(j-i)]]; \
		} \
	}

/*
 * do the correction in the 'msb' part of the p layer
 * at column 'column' and row 'i'
 * with an error of 'ERROR'
 */
#define	DO_L2_P_CORRECTION(ERROR, SKIPPED) \
		{ \
			unsigned position; \
			unsigned dia; \
			position = SECBPL(column, i-SKIPPED); \
			assert(position < 2352-12); \
			if (NO_DECODING_FAILURES) { \
				if (gdp->global_erasures != NULL &&\
				    gdp->global_erasures[position+msb] == 0) return (1); \
			} \
			dia = DIALO(position/2); \
			if	(STOP_FAILURE) { \
				if (gdp->Qcorrcount[msb][dia] == 0 && \
				    gdp->Pcorrcount[msb][dia] == 0 && \
					gdp->corrcount[position+msb] == 0) { \
					calc_L2_Q_syndrome(inout-msb, dia, &gdp->Qsyndromes[2*dia]); \
					if (gdp->Qsyndromes[2*dia+msb] == 0 &&\
					    gdp->Qsyndromes[2*dia+msb+L2_Q/2] == 0) { \
						/* probably a decoding failure */ \
						if (TELLME)	{ \
							fprintf(stderr, "FAILURE (@%d) Dia %d is clean\n", position+12+msb, dia); \
						} \
						return (1); \
					} \
				} \
			} \
			if (gdp->corrcount[position+msb] < MAX_TOGGLE) { \
				inout[position] ^= (unsigned char)ERROR; \
				if (DEBUG > 1) { \
					fprintf(stderr, "\t-%d-", 12+position + msb); \
				} \
				gdp->corrcount[position+msb]++; \
				gdp->Pcorrcount[msb][column]++; \
				kill_erasure(position, msb, gdp); \
				if (loops != 0) check_P_for_decoding_failures(gdp->Qsyndromes, msb, position/2, gdp); \
			} else return (1); \
		}

/*
 * do the correction in the 'msb' part of the q layer
 * at diagonal 'diagonal' and offset 'i'
 * with an error of 'ERROR'
 */
#define	DO_L2_Q_CORRECTION(ERROR, SKIPPED) \
		{ \
			unsigned position; \
			position = SECBQL(diagonal, i-SKIPPED); \
			assert(position < 2352-12); \
			if (NO_DECODING_FAILURES) { \
				if (gdp->global_erasures != NULL &&\
				    gdp->global_erasures[position+msb] == 0) return (1); \
			} \
			if	(STOP_FAILURE2) { \
				if (position < 2236) { \
					unsigned col = COL(position/2); \
					if ((gdp->Pcorrcount[msb][col] > 0 && \
						gdp->corrcount[position+msb] > 0) || \
					    (gdp->Pcorrcount[msb][col] == 0 && \
						gdp->corrcount[position+msb] > 0)) { \
						calc_L2_P_syndrome(inout-msb, col, &gdp->Psyndromes[2*col]); \
						if (gdp->Psyndromes[2*col+msb] == 0 &&\
						    gdp->Psyndromes[2*col+msb+L2_P/2] == 0) { \
							/* probably a decoding failure */	\
							if (TELLME)	{			\
								fprintf(stderr,			\
									"FAILURE (@%d) col %d is clean\n", \
									position+12+msb, col);	\
							}					\
							return (1);				\
						} \
					} \
				} \
			} \
			if (gdp->corrcount[position+msb] < MAX_TOGGLE) { \
				inout[position] ^= (unsigned char)ERROR; \
				gdp->corrcount[position+msb]++; \
				gdp->Qcorrcount[msb][diagonal]++; \
				if (DEBUG > 1) { \
					fprintf(stderr, "\t-%d-", 12+position + msb); \
				} \
				kill_erasure(position, msb, gdp); \
				if (i-SKIPPED < L12_P_LENGTH) *q_changed_p = 1; \
				check_Q_for_decoding_failures(gdp->Psyndromes, msb, position/2, gdp); \
			} else return (1); \
		}


/*
 * do the correction in one of the subchannel layers
 * at offset 'i'
 * with an error of 'ERROR'
 */
#define	DO_SUB_CORRECTION(ERROR, SKIPPED) \
					{ \
						unsigned position; \
						position = (i-SKIPPED); \
						if (DEBUG > 1) \
								fprintf(stderr, \
									"Position %d=0x%x has error 0x%x giving 0x%x\n", \
									position, inout[position], ERROR, \
									inout[position] ^ ERROR); \
						inout[position] ^= (unsigned char)ERROR; \
					}

/*
 * apply the forney algorithm on error valuator and error location
 * polynomial in order to do the corrections.
 */
#define	APPLY_FORNEY_ALGORITHM(LOG, ALOG, MODUL, SKIPPED, DO_CORRECTION) \
	for (j = 0; j < roots_found; j++) { \
			/* \
			 * root in error location polynomial is found! \
			 * apply Forney algorithm to get error value \
			 */ \
			int	numerator; \
			i = roots[j]; \
			numerator = 0; \
			for (k = l-1; ; k--) { \
				if (err_values[k] != 0) { \
					numerator ^= ALOG[LOG[err_values[k]] + (i+1)*k]; \
				} \
				if (k == 0) \
					break; \
			} \
			if (l > 1) { \
					int	error; \
					int	denominator; \
					denominator = 0; \
					for (k = (l-1) & ~1; ; k -= 2) { \
						if (err_locations[k+1] != 0) { \
							denominator ^= ALOG[LOG[err_locations[k+1]] + (k+1)*(i+1)]; \
						} \
						if (k == 0) \
							break; \
					} \
					assert(denominator != 0); \
					error = ALOG[(MODUL - LOG[denominator] + LOG[numerator]) % MODUL]; \
					DO_CORRECTION(error, SKIPPED) \
			} else { \
					DO_CORRECTION(numerator, SKIPPED) \
			} \
	}

#endif

#ifdef	EDC_LAYER2
#ifdef	EDC_DECODER

/*
 * set THOROUGH to 1 for maximum correction capabilities.
 * set THOROUGH to 0 for maximum speed.
 *
 * maximum speed: P and Q corrections are done once
 *
 * maximum capabilities: iteration between layers are used with several
 *		  heuristics. Error positions are deduced and supplied to
 *		  the next layer.
 *
 */
#define	THOROUGH	1

#if	THOROUGH
#ifndef	MAXLOOPS
#define	MAXLOOPS	15
#endif
#else
#define	MAXLOOPS	1
#endif

#define	MAX_TOGGLE	((MAXLOOPS + 1) & ~1)
#define	STOP_FAILURE	1
#define	STOP_FAILURE2	0

#define	NO_DECODING_FAILURES	0


typedef struct globdata {
	unsigned char Qsyndromes[L2_Q];	/* all syndromes of Q */
	unsigned char Psyndromes[L2_P];	/* all syndromes of P */

	/******************** erasure related stuff *********************/

	unsigned char *global_erasures;	/* pointer to the erasure array */
	unsigned char Perapos[2][L2_P/4][L12_Q_LENGTH]; /* erasures in columns */
	unsigned char Qerapos[2][L2_Q/4][L12_P_LENGTH+2]; /* erasures in diagonals */
	unsigned char Pcount[2][L2_P/4]; /* count for corrections */
	unsigned char Qcount[2][L2_Q/4];
	unsigned char Pfailed[2][L2_P/4]; /* count for uncorrectables */
	unsigned char Qfailed[2][L2_Q/4];

	unsigned char corrcount[2352]; /* count for corrections */
	unsigned char Pcorrcount[2][L2_P/4]; /* count for P corrections */
	unsigned char Qcorrcount[2][L2_Q/4]; /* count for Q corrections */
} gdat, *gdatp;


static void kill_erasure __PR((unsigned position, unsigned msb, gdatp gdp));


/***************** cyclic redundancy check function **************/


/*
 * calculate the crc checksum depending on the sector type.
 *
 * parameters:
 *   input:
 *		inout is the data sector as a byte array at offset 0
 *		type is the sector type (MODE_1, MODE_2_FORM_1 or MODE_2_FORM_2)
 *		loops is a flag indicating erasure initialization, when 0.
 *			      Should be set to 0 at the very first time.
 *		gdp is a pointer to auxiliary book keeping data.
 *   output: none
 *
 * return value:	1, if cyclic redundancy checksum is 0 (no errors)
 *			0, if errors are present.
 *
 */

static int do_crc_check __PR((unsigned char inout[(L2_RAW + L2_Q + L2_P)], int type, int loops, gdatp gdp));

static int do_crc_check(inout, type, loops, gdp)
	unsigned char inout[(L2_RAW + L2_Q + L2_P)];
	int type;
	int loops;
	gdatp	gdp;
{
	unsigned result;
	unsigned low;
	unsigned high;

	/*
	 * Verify the checksum.
	 * This is dependent on the sector type being used!
	 */
	switch (type) {
		case EDC_MODE_1:
			low = 0;
			high = 16 + 2048 + 4 - 1;
		break;
		case EDC_MODE_2_FORM_1:
			low = 16;
			high = 16 + 8 + 2048 + 4 - 1;
		break;
		case EDC_MODE_2_FORM_2:
			low = 16;
			high = 16 + 8 + 2324 + 4 - 1;
			loops = 1;
		break;
		default:
			return (0);
	}
	result = build_edc(inout, low, high);
	if (loops == 0 && result == 0 && gdp->global_erasures != NULL) {
		unsigned i;
		/* kill all erasures for this range */
		for (i = max(low, 12)-12; i <= high-12; i++) {
			if (gdp->global_erasures[i] == 1)
				kill_erasure(i, i & 1, gdp);
		}
	}
	return (result == 0);	/* failed, if result != 0 */
}



/*
 * calculate the crc checksum depending on the sector type.
 *
 * parameters:
 *   input:
 *		inout is the data sector as a byte array at offset 0
 *		type is the sector type (MODE_1 or MODE_2_FORM_1)
 *
 * return value:	1, if cyclic redundancy checksum is 0 (no errors)
 *			0, if errors are present.
 *
 */
int crc_check __PR((unsigned char inout[(L2_RAW + L2_Q + L2_P)], int type));
int
crc_check(inout, type)
	unsigned char inout[(L2_RAW + L2_Q + L2_P)];
	int type;
{
	return (do_crc_check(inout, type, 1, NULL));
}

/******************* SYNDROM FUNCTIONS ************************/


/*
 * calculate the syndrome for one diagonal of the Q parity layer of data sectors
 * One diagonal consists of words (least significant byte first and most
 * significant byte last).
 *
 * parameters:
 *   input:
 *	inout		the content of the data sector as a byte array
 *			at offset 12.
 *	diagonal	0..25, the diagonal to calculate the syndrome for.
 *   output:
 *	syndrome	the byte array beginning at the syndrome position for
 *			that diagonal.
 * return value:	1, if the syndrome is 0 (the diagonal shows no errors)
 *			0, if errors are present.
 */
static int
calc_L2_Q_syndrome __PR((unsigned char inout[4 + L2_RAW + 4 + 8 + L2_P + L2_Q],
	unsigned diagonal, unsigned char syndrome[L2_Q/2+2]));

static int
calc_L2_Q_syndrome(inout, diagonal, syndrome)
	unsigned char inout[4 + L2_RAW + 4 + 8 + L2_P + L2_Q];
	unsigned diagonal;
	unsigned char syndrome[L2_Q/2+2];
{
	register unsigned		dp0, dp1;
	register const	unsigned	short	*qa;
	register unsigned char		*Q;

	qa = &qacc[diagonal][44];
	Q = syndrome;
	Q[0] = Q[2*L12_Q_LENGTH] = 0;
	Q[1] = Q[2*L12_Q_LENGTH+1] = 0;

#define	QBODYEND(i) \
		dp0 = inout[*qa];			\
		dp1 = inout[*qa+1]; 			\
							\
		*Q++ ^= (unsigned char)dp0;		\
		*Q   ^= (unsigned char)dp1;		\
		Q += 26*2-1;				\
		*Q++ ^= L2syn[i][dp0];			\
		*Q   ^= L2syn[i][dp1];

#define	QBODY(i)   \
		QBODYEND(i) \
		qa--;       \
		Q -= 26*2+1;

		QBODY(0);  QBODY(1);  QBODY(2);  QBODY(3);  QBODY(4);
		QBODY(5);  QBODY(6);  QBODY(7);  QBODY(8);  QBODY(9);
		QBODY(10); QBODY(11); QBODY(12); QBODY(13); QBODY(14);
		QBODY(15); QBODY(16); QBODY(17); QBODY(18); QBODY(19);
		QBODY(20); QBODY(21); QBODY(22); QBODY(23); QBODY(24);
		QBODY(25); QBODY(26); QBODY(27); QBODY(28); QBODY(29);
		QBODY(30); QBODY(31); QBODY(32); QBODY(33); QBODY(34);
		QBODY(35); QBODY(36); QBODY(37); QBODY(38); QBODY(39);
		QBODY(40); QBODY(41); QBODY(42); QBODY(43); QBODYEND(44);
#undef  QBODY
#undef  QBODYEND
	return (0 == syndrome[0] &&
	    0 == syndrome[1] &&
	    0 == syndrome[0 + L12_Q_LENGTH * 2] &&
	    0 == syndrome[1 + L12_Q_LENGTH * 2]);
}

/*
 * calculate the syndrome for one column of the P parity layer of data sectors
 * One column consists of words (least significant byte first and most
 * significant byte last).
 *
 * parameters:
 *   input:
 *	inout		the content of the data sector as a byte array
 *			at offset 12.
 *	column		0..43, the column to calculate the syndrome for.
 *   output:
 *	syndrome	the byte array beginning at the syndrome position for
 *			that column.
 * return value:	1, if the syndrome is 0 (the column shows no errors)
 *			0, if errors are present.
 */
static int
calc_L2_P_syndrome __PR((unsigned char inout[4 + L2_RAW + 4 + 8 + L2_P],
	unsigned column, unsigned char syndrome[L2_P/2+2]));

static int
calc_L2_P_syndrome(inout, column, syndrome)
	unsigned char inout[4 + L2_RAW + 4 + 8 + L2_P];
	unsigned column;
	unsigned char syndrome[L2_P/2+2];
{
	register unsigned char  *P;
	register unsigned dp0, dp1;

	P = syndrome;
	P[0] = P[1] = P[L12_P_LENGTH*2] = P[1+L12_P_LENGTH*2] = 0;
	inout += 2*column + (L12_Q_LENGTH-1)*L12_P_LENGTH*2 + 1;

#define	PBODYEND(i)					\
	dp1 = *inout--;					\
	dp0 = *inout;					\
							\
	*P++ ^= (unsigned char)dp0;			\
	*P   ^= (unsigned char)dp1;			\
	P   += L12_P_LENGTH*2-1;			\
	*P++ ^= L2syn[i][dp0];				\
	*P   ^= L2syn[i][dp1];

#define	PBODY(i)					\
			PBODYEND(i)			\
			inout -= L12_P_LENGTH*2-1;	\
			P   -= L12_P_LENGTH*2+1;

    PBODY(0)   PBODY(1)   PBODY(2)   PBODY(3)   PBODY(4)
    PBODY(5)   PBODY(6)   PBODY(7)   PBODY(8)   PBODY(9)
    PBODY(10)  PBODY(11)  PBODY(12)  PBODY(13)  PBODY(14)
    PBODY(15)  PBODY(16)  PBODY(17)  PBODY(18)  PBODY(19)
    PBODY(20)  PBODY(21)  PBODY(22)  PBODY(23)  PBODY(24)
    PBODYEND(25)
#undef	PBODY
#undef	PBODYEND
	return (0 == syndrome[0] &&
	    0 == syndrome[1] &&
	    0 == syndrome[0 + L12_P_LENGTH * 2] &&
	    0 == syndrome[1 + L12_P_LENGTH * 2]);
}


/*
 * set an erasure at the given position.
 *
 * Input parameters:
 *  position: even offset in the sector after the sync header.
 *  msb:		1 for the most significant byte.
 *				0 for the least significant byte.
 *  gdp:	  pointer to auxiliary structure.
 */
static void set_erasure __PR((unsigned position, unsigned msb, gdatp gdp));

static void set_erasure(position, msb, gdp)
	unsigned position;
	unsigned	msb;
	gdatp	gdp;
{
	unsigned	dia;
	unsigned	cpos;
	unsigned	dpos;
	unsigned	k;

	dia = DIA(position/2);
	dpos = POSQ(position/2) + L12_Q_SKIPPED;

	if (position < 2*L12_P_LENGTH*L12_Q_LENGTH) {
		unsigned col;
		col = COL(position >> 1);
		cpos = POSP(position >> 1) + L12_P_SKIPPED;

		/* look if erasure exists. */
		for (k = 0; k < gdp->Pcount[msb][col]; k++) {
			if (cpos == gdp->Perapos[msb][col][k]) break;
		}

		if (k == gdp->Pcount[msb][col])
			gdp->Perapos[msb][col][gdp->Pcount[msb][col]++] = (unsigned char)cpos;

#if	DEBUG >= 0
	fprintf(stderr, "\t+%d(c%u,d%u)", position+12+msb, col, dia);
	if (gdp->global_erasures[position+msb] == 0)	fprintf(stderr, "??");
	} else {
	fprintf(stderr, "\t+%d(c??,d%u)", position+12+msb, dia);
	if (gdp->global_erasures[position+msb] == 0)	fprintf(stderr, "??");
#endif
	}

	for (k = 0; k < gdp->Qcount[msb][dia]; k++) {
		if (dpos == gdp->Qerapos[msb][dia][k]) break;
	}

	if (k == gdp->Qcount[msb][dia])
		gdp->Qerapos[msb][dia][gdp->Qcount[msb][dia]++] = (unsigned char)dpos;

	if (gdp->global_erasures == NULL)
		return;

	/* set the erasure at the given position. */
	gdp->global_erasures[position+msb] = 1;
}


/*
 * delete the erasure at the given position.
 *
 * Input parameters:
 *  position: even offset in the sector after the sync header.
 *  msb:		1 for the most significant byte.
 *				0 for the least significant byte.
 *		gdp is a pointer to auxiliary book keeping data.
 */

static void
kill_erasure(position, msb, gdp)
	unsigned position;
	unsigned msb;
	gdatp gdp;
{
	unsigned i, j;
	unsigned dia;
	unsigned pos;
	unsigned col;
	unsigned char *p, *q;

	if (gdp->global_erasures == NULL)
		return;

#if	DEBUG >= 7
	if (global_erasures[position + msb] == 0) {
		fprintf(stderr, "<WRONG(c%d,d%d)\t",
		position < 2*L12_P_LENGTH*L12_Q_LENGTH ? COL(position/2) : -1, DIA(position/2));
	}
#endif

#if	DEBUG >= 0
	/* is the erasure really defined once? */
	if (position < 2*L12_P_LENGTH*L12_Q_LENGTH) {
		col = COL(position/2);
		pos = POSP(position/2);
		p =  gdp->Perapos[msb][col];
		for (dia = 0, j = gdp->Pcount[msb][col]; j > 0; j--, p++)
			if (*p == pos + L12_P_SKIPPED) {
				dia++;
			}
		if (dia > 1)
			fprintf(stderr, "\nkill:P position %d %d times, count=%d!\n", position+12+msb, dia, gdp->Pcount[msb][col]);
	}
	dia = DIA(position/2);
	pos = POSQ(position/2);
	q =  gdp->Qerapos[msb][dia];
	for (col = 0, i = gdp->Qcount[msb][dia]; i > 0; i--, q++)
		if (*q == pos + L12_Q_SKIPPED) {
			col++;
		}
	if (col > 1)
		fprintf(stderr, "\nkill:Q position %d %d times!\n", position+12+msb, col);
#endif

	gdp->global_erasures[position+msb] ^= 1;


	if (position < 2*L12_P_LENGTH*L12_Q_LENGTH) {
		col = COL(position/2);
		pos = POSP(position/2) + L12_P_SKIPPED;
		p =  gdp->Perapos[msb][col];

		for (j = gdp->Pcount[msb][col]; j > 0 && *p != pos; j--, p++)
			;

		if (j > 1)
			memmove(p, p+1, j-1);
		else if (j == 1)
			*p = 0;

		if (gdp->Pcount[msb][col] > 0)
			gdp->Pcount[msb][col]--;
	}
	dia = DIA(position/2);
	q =  gdp->Qerapos[msb][dia];
	pos = POSQ(position/2) + L12_Q_SKIPPED;

	for (i = gdp->Qcount[msb][dia]; i > 0 && *q != pos; i--, q++)
		;

	if (i > 1)
		memmove(q, q+1, i-1);
	else if (i == 1)
		*q = 0;

	if (gdp->Qcount[msb][dia] > 0)
		gdp->Qcount[msb][dia]--;

#if	DEBUG >= 0
	/* has the erasure really been deleted? */
	pos = POSP(position/2);
	p =  gdp->Perapos[msb][col];
	for (j = gdp->Pcount[msb][col]; j > 0; j--, p++)
		if (*p == pos + L12_P_SKIPPED) {
			fprintf(stderr, "\nkill_era: error in p (pos=%d,dia=%d,col=%d)!\n", position+12+msb, dia, col);
		}
	pos = POSQ(position/2);
	q =  gdp->Qerapos[msb][dia];
	for (i = gdp->Qcount[msb][dia]; i > 0; i--, q++)
		if (*q == pos + L12_Q_SKIPPED) {
			fprintf(stderr, "\nkill_era: error in q (pos=%d,dia=%d,col=%d)!\n", position+12+msb, dia, col);
		}
#endif
}


/*
 * translate the erasures in p and q coordinates.
 *
 * Input parameters:
 *  have_erasures:	the number of erasures
 *					set in the erasures array.
 *	inout:			the content of the data sector as a byte array
 *					at offset 12.
 *	gdp:			is a pointer to auxiliary book keeping data.
 *
 * Return
 */

static unsigned	unify_erasures __PR((int have_erasures, unsigned char *inout, gdatp gdp));
static unsigned
unify_erasures(have_erasures, inout, gdp)
	int	have_erasures;
	unsigned char	*inout;
	gdatp gdp;
{
	unsigned	i;
	unsigned	msb;
	unsigned	retval = 0;

	if (have_erasures != 0)
		return (0);

	/*
	 * We assume the q layer has just been done. This possibly changed
	 * something, so we update the status for the p layer.
	 */
	for (i = 0; i < L2_P/4; i++) {
		unsigned char	synd[L2_P];

		calc_L2_P_syndrome(inout, i, synd);
		if (synd[0] == 0 && synd[L2_P/2] == 0) {
			gdp->Pfailed[0][i] = 0;
		}

		if (synd[1] == 0 && synd[L2_P/2+1] == 0) {
			gdp->Pfailed[1][i] = 0;
		}
	}

	/*
	 * loop for both byte layers (most and least significant)
	 */
	for (msb = 0; msb < 2; msb++) {
		int	pcounter = 0;
		int	qcounter = 0;
		int	j;

		/*
		 * count columns in P with failures
		 */
		for (j = 0; j < L2_P/4; j++) {
			if (gdp->Pfailed[msb][j] != 0) {
				pcounter++;
#if	DEBUG	>= 0
	fprintf(stderr, " p%2d ", j);
#endif
			}
		}

		/*
		 * count diagonals in Q with failures
		 */
		for (j = 0; j < L2_Q/4; j++) {
			if (gdp->Qfailed[msb][j] != 0) {
				qcounter++;
#if	DEBUG	>= 0
	fprintf(stderr, " q%2d ", j);
#endif
			}
		}
#if	DEBUG	>= 0
	fprintf(stderr, " pc%d qc%d\n", pcounter, qcounter);
#endif

		/*
		 * If Q is clean, but P is not, this is
		 * considered as decoding failures introduced from Q
		 */
		if (pcounter > 0 && qcounter == 0) {
			/* decoding failure */
#if	DEBUG	>= 0
			fprintf(stderr, "\n");
#endif
			return (1);
		}

		/*
		 * if P is clean (and the CRC too), we consider all
		 * remaining errors located in the Q parity part.
		 */
		if (pcounter == 0) {
			for (i = 0; i < L2_Q/4; i++) {
				if (gdp->Qfailed[msb][i] != 0) {
					if (gdp->global_erasures != NULL &&
					    (gdp->global_erasures[SECBQL(i, L2_P/4)+msb] == 0 ||
					    gdp->global_erasures[SECBQL(i, L2_P/4 + 1)+msb] == 0)) {
						set_erasure(SECBQL(i, L2_P/4), msb, gdp);
						set_erasure(SECBQL(i, L2_P/4 + 1), msb, gdp);
						retval = 1;
					}
				}
			}
#if	0
		} else if (pcounter  <= 2) {
			/* P has one or two errors */
			for (i = 0; i < L2_Q/4; i++) {
				if (gdp->Qfailed[msb][i] != 0) {
					for (j = 0; j < L2_P/4; j++) {
						if (gdp->Pfailed[msb][j] != 0) {
							set_erasure(SECBQL(i, j)+msb, gdp);
							retval = 1;
#if	DEBUG	>= 0
	fprintf(stderr, " 2P%d\n", j);
#endif
						}
					}
				}
			}
#endif
		}

		/*
		 * If max 2 diagonals have errors, we assume
		 * the errors are located in the common part with P.
		 */
		if (qcounter <= 2) {
			for (i = 0; i < L2_P/4; i++) {
				if (gdp->Pfailed[msb][i] != 0) {
					for (j = 0; j < L2_Q/4; j++) {
						if (gdp->Qfailed[msb][j] != 0) {
							set_erasure(SECBQL(j, i), msb, gdp);
							retval = 1;
#if	DEBUG	>= 0
	fprintf(stderr, " 2Q%d\n", j);
#endif
						}
					}
				}
			}
		}
		if (0) {
		    for (i = 0; i < L2_Q/4; i++) {
				if (gdp->Qfailed[msb][i] != 0) {
					/* look for P status */
					for (j = 0; j < L2_P/4; j++) {
						if (pcounter == 0) {
							set_erasure(SECBQL(i, L2_P/4), msb, gdp);
							set_erasure(SECBQL(i, L2_P/4 + 1), msb, gdp);
							break;
#if 0
						} else if (gdp->Pcount[msb][j] > 0 &&
							    gdp->Pcount[msb][j] <= 2) {
							int	k;
							for (k = 0; k < gdp->Pcount[msb][j]; k++) {
								set_erasure(SECBQL(i,
										gdp->Perapos[msb][j][k] - P_SKIPPED)+msb,
										gdp);
#if	DEBUG	>= 0
		fprintf(stderr, " P%d,%d ", j, k);
#endif
							}
#endif
						}
					}
				}
		    }
		}
	}
#if	DEBUG	>= 0
	fprintf(stderr, " ->%d\n", retval);
#endif
	return (retval);
}

/*
 * Initialize our separated p and q erasure arrays.
 *
 * Input parameters:
 *  have_erasures:	the number of erasures
 *					set in the erasures array.
 *  erasures:		the byte array for the erasures. Each byte
 *					corresponds to a data byte in the L2 sector.
 *	gdp:			is a pointer to auxiliary book keeping data.
 */
static void
init_erasures __PR((int have_erasures, unsigned char erasures[(L2_RAW + L2_Q + L2_P)], gdatp gdp));

static void
init_erasures(have_erasures, erasures, gdp)
	int	have_erasures;
	unsigned char	erasures[(L2_RAW + L2_Q + L2_P)];
	gdatp gdp;
{
	int	i;

	memset(gdp->Perapos, 0, sizeof (gdp->Perapos));
	memset(gdp->Qerapos, 0, sizeof (gdp->Qerapos));
	memset(gdp->Pcount, 0, sizeof (gdp->Pcount));
	memset(gdp->Qcount, 0, sizeof (gdp->Qcount));

	if (erasures == NULL) {
		have_erasures = 0;
		gdp->global_erasures = NULL;
	} else {
		gdp->global_erasures = erasures+12;
	}
#if DEBUG > 2
	if (gdp->global_erasures != NULL) {
#else
	if (have_erasures != 0) {
#endif
		memset(erasures, 0, 12);

		/*
		 * We can only use erasures at offset 12 and above.
		 *  Further we need the number/positions of erasures separated
		 *  for LSB and MSB. So count them.
		 */


		for (i = 0; i < 2*L12_P_LENGTH*L12_Q_LENGTH; i += 2) {
			if (gdp->global_erasures[i  ] != 0) {
				gdp->Perapos[0][COL(i/2)]
					[gdp->Pcount[0][COL(i/2)]]
						= (unsigned char)(POSP(i/2) + L12_P_SKIPPED);
#if DEBUG > 2
	fprintf(stderr, "off %d -> col %d pos %d   dia %d pos %d\n", i+12, COL(i/2), POSP(i/2), DIA(i/2), POSQ(i/2));
	if (have_erasures != 0)
#endif
				gdp->Pcount[0][COL(i/2)]++;
			}
			if (gdp->global_erasures[i+1] != 0) {
				gdp->Perapos[1][COL(i/2)]
					[gdp->Pcount[1][COL(i/2)]]
						= (unsigned char)(POSP(i/2) + L12_P_SKIPPED);
#if DEBUG > 2
	fprintf(stderr, "off %d -> col %d pos %d   dia %d pos %d\n", i+12+1, COL(i/2), POSP(i/2), DIA(i/2), POSQ(i/2));
	if (have_erasures != 0)
#endif
				gdp->Pcount[1][COL(i/2)]++;
			}
		}
		for (i = 0; i < L12_Q_LENGTH; i++) {
			int j;
			for (j = 0; j < L12_P_LENGTH; j++) {
				int ind = SECBQLLO(i, j);

				if (gdp->global_erasures[ind  ] != 0) {
					gdp->Qerapos[0][i][gdp->Qcount[0][i]]
							= (unsigned char)(j + L12_Q_SKIPPED);
#if DEBUG > 2
	fprintf(stderr, "off %d -> dia %d pos %d   col %d pos %d\n", ind+12, i, j, COL(ind/2), POSP(ind/2));
	if (have_erasures != 0)
#endif
					gdp->Qcount[0][i]++;
				}
				if (gdp->global_erasures[ind+1] != 0) {
					gdp->Qerapos[1][i][gdp->Qcount[1][i]]
							= (unsigned char)(j + L12_Q_SKIPPED);
#if DEBUG > 2
	fprintf(stderr, "off %d -> dia %d pos %d   col %d pos %d\n", ind+12+1, i, j, COL(ind/2), POSP(ind/2));
	if (have_erasures != 0)
#endif
					gdp->Qcount[1][i]++;
				}
			}
		}
		for (i = 0; i < L12_Q_LENGTH; i++) {
			int j;
			for (j = L12_P_LENGTH; j < L12_P_LENGTH + 2; j++) {
				int ind = SECBQLHI(i, j);

				if (gdp->global_erasures[ind  ] != 0) {
					gdp->Qerapos[0][i][gdp->Qcount[0][i]]
							= (unsigned char)(j + L12_Q_SKIPPED);
#if DEBUG > 2
	fprintf(stderr, "off %d -> dia %d pos %d   col -- pos --\n", ind+12, i, j);
	if (have_erasures != 0)
#endif
					gdp->Qcount[0][i]++;
				}
				if (gdp->global_erasures[ind+1] != 0) {
					gdp->Qerapos[1][i][gdp->Qcount[1][i]]
							= (unsigned char)(j + L12_Q_SKIPPED);
#if DEBUG > 2
	fprintf(stderr, "off %d -> dia %d pos %d   col -- pos --\n", ind+12+1, i, j);
	if (have_erasures != 0)
#endif
					gdp->Qcount[1][i]++;
				}
			}
		}
	}
}

/*
 *	Check the P layer for inconsistencies at a given position.
 *
 * Input parameters:
 *  syndrome:   a byte array with the precalculated syndrome.
 *  msb:        true, for the most significant bytes
 *              false for the least significant bytes
 *  position:   the offset in the sector (after byte 12)
 *	gdp:		is a pointer to auxiliary book keeping data.
 */

static
void check_P_for_decoding_failures __PR((unsigned char syndrome[],
	unsigned msb, unsigned position, gdatp gdp));

static
void
check_P_for_decoding_failures(syndrome, msb, position, gdp)
	unsigned char syndrome[];
	unsigned	msb;
	unsigned	position;
	gdatp gdp;
{
	unsigned	pos = msb+2*DIALO(position);
	/* check Q syndrome at that position */
	if (syndrome[pos] == 0 && syndrome[pos+L2_Q/2] == 0) {
#if	DEBUG > 1
fprintf(stderr, " inval Q d %d", DIALO(position));
#endif
		syndrome[pos] = 1;
		gdp->Pfailed[msb][COL(position)] = 1;
		/* mark_erasure too */
		/*set_erasure(2*position+msb);*/
	}
}

/*
 *	Check the Q layer for inconsistencies at a given position.
 *
 * Input parameters:
 *  syndrome:   a byte array with the precalculated syndrome.
 *  msb:        true, for the most significant bytes
 *              false for the least significant bytes
 *  position:   the offset in the sector (after byte 12)
 *	gdp:		is a pointer to auxiliary book keeping data.
 */

static
void check_Q_for_decoding_failures __PR((unsigned char syndrome[],
	unsigned msb, unsigned position, gdatp gdp));

static
void
check_Q_for_decoding_failures(syndrome, msb, position, gdp)
	unsigned char syndrome[];
	unsigned	msb;
	unsigned	position;
	gdatp gdp;
{
	unsigned	pos;

	if (position >= L12_P_LENGTH*L12_Q_LENGTH)
		return;

	pos = msb+2*COL(position);
	/* check P syndrome at that position */
	if (syndrome[pos] == 0 && syndrome[pos+L2_P/2] == 0) {
#if	DEBUG > 1
fprintf(stderr, " inval P c %d", COL(position));
#endif
		syndrome[pos] = 1;
		gdp->Qfailed[msb][DIALO(position)] = 1;
		/* mark_erasure too */
		/*if (loops) set_erasure(2*position+msb);*/
	}
}



/*
 * implements Berlekamp-Massey-Decoders.
 */

/* get the a'th syndrome byte. We translate to the sector structure */
#define	SYND(a)		syndrome[(a)*L12_P_LENGTH*2]

/*
 * Correct one column in the p parity layer.
 *
 * Input/Output parameters:
 *  inout:      the byte array holding the sector after the sync header.
 *              A correction will change the array.
 *
 * Input parameters:
 *  msb:        true, for the most significant bytes
 *              false for the least significant bytes
 *  column:     the column number (0 - 42)
 *  syndrome:   a byte array with the precalculated syndrome.
 *  erasures:   the number of given error positions for this column.
 *  Perasures:  the byte array holding the error positions
 *              for this column.
 *  low_taboo:  the lower limit of a range left to be unchanged.
 *  high_taboo: the upper limit of a range left to be unchanged.
 *  loops:		>= 0, the loop count of the iteration
 *	gdp:		is a pointer to auxiliary book keeping data.
 */
static int
correct_P __PR((unsigned char inout[4 + L2_RAW + 4 + 8 + L2_P],
	unsigned msb,
	unsigned column, unsigned char syndrome[L2_P/2+2],
	unsigned erasures, unsigned char Perasures[L12_Q_LENGTH],
	unsigned low_taboo, unsigned high_taboo, int loops, gdatp gdp));

static int
correct_P(inout, msb, column, syndrome, erasures, Perasures, low_taboo, high_taboo, loops, gdp)
	unsigned char inout[4 + L2_RAW + 4 + 8 + L2_P];
	unsigned msb;
	unsigned column;
	unsigned char syndrome[L2_P/2+2];
	unsigned erasures;
	unsigned char Perasures[L12_Q_LENGTH];
	unsigned low_taboo;
	unsigned high_taboo;
	int loops;
	gdatp gdp;
{
	unsigned	i;
	unsigned	j;
	unsigned	l;
	unsigned	k;
	unsigned	roots_found;
	unsigned char	err_locations[2*L12_P_ERRORS+1];
	unsigned char	err_values[2*L12_P_ERRORS+1];
	unsigned char	tmp_loc[2*L12_P_ERRORS+1];
	unsigned char	roots[2*L12_P_ERRORS];
	unsigned char	discrepancy;

	INIT_BMD

#if	DEBUG > 0
fprintf(stderr, "tab(%u-%u), era(%u)", low_taboo, high_taboo, erasures);
#endif

	/*
	 * initialize error location polynomial with roots from given
	 * erasure locations.
	 */
	if (erasures > 0) {

		INIT_ERASURES(Perasures, L12_P_ERRORS, L12_MODUL,
						rs_l12_log, rs_l12_alog)

		if (DEBUG > 2) { DEBUG_ERASURE_POLY(rs_l12_log, rs_l12_alog,
				L12_MODUL, L12_P_ERRORS, L12_P_SKIPPED, 0) }
	}

	/*
	 * calculate the final error location polynomial.
	 */
	for (j = l = erasures, k = 1; j < 2*L12_P_ERRORS; j++, k++) {

		/* calculate the 'discrepancy' */
		CALC_DISCREPANCY(rs_l12_log, rs_l12_alog)

		/* test discrepancy for zero */
		if (discrepancy != 0) {
			BUILD_SHIFT_REGISTER(rs_l12_log, rs_l12_alog,
					L12_MODUL, L12_P_ERRORS)
		}
#if	DEBUG > 4
		fprintf(stderr, " Pdisc=%d, ", discrepancy);
		fprintf(stderr, "j=%d, ", j);
		fprintf(stderr, "k=%d, l(errors)=%d, e0=%d, e1=%d, B0=%d, B1=%d\n",
				k, l, err_locations[0], err_locations[1], tmp_loc[0], tmp_loc[1]);
#endif
	}

#if	DEBUG > 0
	fprintf(stderr, ",l(%d)   ", l);
	if (gdp->global_erasures != NULL) {
		int b;
		for (b = 0; b < L12_Q_LENGTH; b++) {
			if (gdp->global_erasures[SECBPL(column, b)+ msb] == 1) {
				fprintf(stderr, "%4d, ", 12+SECBPL(column, b) + msb);
			}
		}
		for (b = 0; b < gdp->Pcount[msb][column]; b++) {
			if (gdp->global_erasures[SECBPL(column, gdp->Perapos[msb][column][b] - L12_P_SKIPPED) + msb] == 0) {
				fprintf(stderr, " :%4d: ", 12+SECBPL(column, gdp->Perapos[msb][column][b] - L12_P_SKIPPED) + msb);
			}
		}
	}
#endif
	if (l > L12_P_ERRORS+erasures/2) {
#if	DEBUG > 0
		fprintf(stderr, "\tl > 2+era/2  ");
#endif
		return (1);
	}

	/* find roots of err_locations */
	FIND_ROOTS(rs_l12_log, rs_l12_alog, L12_MODUL, L12_P_ERRORS,
		L12_P_SKIPPED, (FIND_ROOTS_TABOO(SECWP, column, L12_P_SKIPPED)))

	/* too many errors if the number of roots is not what we expected */
	if (roots_found != l) {
#if	DEBUG > 0
		fprintf(stderr, "roots(%d)!=l(%d)\t", roots_found, l);
#endif
		return (1);
	}

	/*
	 * l holds the number of errors
	 *
	 * now calculate the error valuator
	 */
	CALC_ERROR_VALUATOR(rs_l12_log, rs_l12_alog)

	/* use roots of err_locations */
	APPLY_FORNEY_ALGORITHM(rs_l12_log, rs_l12_alog, L12_MODUL,
			L12_P_SKIPPED, DO_L2_P_CORRECTION)

	return (0);
}
#undef	SYND




/*
 * Correct one diagonal in the q parity layer.
 *
 * Input/Output parameters:
 *  inout:      the byte array holding the sector after the sync header.
 *              A correction will change the array.
 *
 * Input parameters:
 *  msb:        true, for the most significant bytes
 *              false for the least significant bytes
 *  diagonal:   the diagonal number (0 - 25)
 *  syndrome:   a byte array with the precalculated syndrome.
 *  erasures:   the number of given error positions for this column.
 *  Qerasures:  the byte array holding the error positions
 *              for this column.
 *  low_taboo:  the lower limit of a range left to be unchanged.
 *  high_taboo: the upper limit of a range left to be unchanged.
 *  q_changed_p: pointer to int; the integer is set true, if a
 *				correction affects the part covered by the
 *				p parity layer.
 *  loops:		>= 0, the loop count of the iteration
 *	gdp:		is a pointer to auxiliary book keeping data.
 */

#define	SYND(a)		syndrome[(a)*L12_Q_LENGTH*2]
static int
correct_Q __PR((unsigned char inout[4 + L2_RAW + 4 + 8 + L2_P + L2_Q],
	unsigned msb,
	unsigned diagonal, unsigned char syndrome[L2_Q/2+2],
	unsigned erasures, unsigned char Qerasures[L12_P_LENGTH],
	unsigned low_taboo, unsigned high_taboo, unsigned *q_changed_p, gdatp gdp));

static int
correct_Q(inout, msb, diagonal, syndrome, erasures, Qerasures, low_taboo, high_taboo,
	q_changed_p, gdp)
	unsigned char inout[4 + L2_RAW + 4 + 8 + L2_P + L2_Q];
	unsigned msb;
	unsigned diagonal;
	unsigned char syndrome[L2_Q/2+2];
	unsigned erasures;
	unsigned char Qerasures[L12_P_LENGTH];
	unsigned low_taboo;
	unsigned high_taboo;
	unsigned *q_changed_p;
	gdatp gdp;
{
	unsigned	i;
	unsigned	j;
	unsigned	l;
	unsigned	k;
	unsigned	roots_found;
	unsigned char	err_locations[2*L12_Q_ERRORS+1];
	unsigned char	err_values[2*L12_Q_ERRORS+1];
	unsigned char	tmp_loc[2*L12_Q_ERRORS+1];
	unsigned char	roots[2*L12_Q_ERRORS];
	unsigned char	discrepancy;

	INIT_BMD

#if	DEBUG > 0
fprintf(stderr, "tab(%u-%u), era(%u)", low_taboo, high_taboo, erasures);
#endif

	/*
	 * initialize error location polynomial with roots from given
	 * erasure locations.
	 */
	if (erasures > 0) {

		INIT_ERASURES(Qerasures, L12_Q_ERRORS, L12_MODUL,
						rs_l12_log, rs_l12_alog)

		if (DEBUG > 2) { DEBUG_ERASURE_POLY(rs_l12_log, rs_l12_alog,
				L12_MODUL, L12_Q_ERRORS, L12_Q_SKIPPED,
				(FIND_ROOTS_TABOO(SECWQ, diagonal, L12_Q_SKIPPED))) }
	}

	/*
	 * calculate the final error location polynomial.
	 */
	for (j = l = erasures, k = 1; j < 2*L12_Q_ERRORS; j++, k++) {

		/* calculate the 'discrepancy' */
		CALC_DISCREPANCY(rs_l12_log, rs_l12_alog)

		/* test discrepancy for zero */
		if (discrepancy != 0) {
			BUILD_SHIFT_REGISTER(rs_l12_log, rs_l12_alog,
					L12_MODUL, L12_Q_ERRORS)
		}
#if	DEBUG > 4
		fprintf(stderr, " Qdisc=%d, ", discrepancy);
		fprintf(stderr, "j=%d, ", j);
		fprintf(stderr, "k=%d, l(errors)=%d, e0=%d, e1=%d, B0=%d, B1=%d\n",
				k, l, err_locations[0], err_locations[1], tmp_loc[0], tmp_loc[1]);
#endif
	}

#if	DEBUG > 0
	fprintf(stderr, ",l(%d)   ", l);
	if (gdp->global_erasures != NULL) {
		int b;
		for (b = 0; b < L12_P_LENGTH+2; b++) {
			if (gdp->global_erasures[SECBQL(diagonal, b)+ msb] == 1) {
				fprintf(stderr, "%4d, ", 12+SECBQL(diagonal, b) + msb);
			}
		}
		for (b = 0; b < gdp->Qcount[msb][diagonal]; b++) {
			if (gdp->global_erasures[SECBQL(diagonal, gdp->Qerapos[msb][diagonal][b] - L12_Q_SKIPPED) + msb] == 0) {
				fprintf(stderr, " :%4d: ",
						12+SECBQL(diagonal, gdp->Qerapos[msb][diagonal][b] - L12_Q_SKIPPED) + msb);
			}
		}
	}
#endif
	if (l > L12_Q_ERRORS+erasures/2) {
#if	DEBUG > 0
		fprintf(stderr, "l > 2+era/2  ");
#endif
		return (1);
	}

	/* find roots of err_locations */
	FIND_ROOTS(rs_l12_log, rs_l12_alog, L12_MODUL, L12_Q_ERRORS,
		L12_Q_SKIPPED, (FIND_ROOTS_TABOO(SECWQ, diagonal, L12_Q_SKIPPED)))

	/* too many errors if the number of roots are not what we expected */
	if (roots_found != l) {
#if	DEBUG > 0
		fprintf(stderr, "roots(%d)!=l(%d)\t", roots_found, l);
#endif
		return (1);
	}

	/*
	 * l holds the number of errors
	 *
	 * now calculate the error valuator
	 */
	CALC_ERROR_VALUATOR(rs_l12_log, rs_l12_alog)

	/* use roots of err_locations */
	APPLY_FORNEY_ALGORITHM(rs_l12_log, rs_l12_alog, L12_MODUL,
			L12_Q_SKIPPED, DO_L2_Q_CORRECTION)

	return (0);
}
#undef	SYND


/*
 * p_correct_all: do the correction for all columns
 * in the p parity layer.
 *
 * Input/output parameters:
 *  inout_pq:	the byte array with the sector data
 *				after the sync header to be corrected.
 *  p_changed:	pointer to int; set true, if a
 *				correction (change) has been done.
 *
 * Input parameters:
 *  low_taboo:  the lower limit of a range left to be unchanged.
 *  high_taboo: the upper limit of a range left to be unchanged.
 *  loops:		>= 0, the loop count of the iteration
 *	gdp:		is a pointer to auxiliary book keeping data.
 *
 */

static int
p_correct_all __PR((unsigned char inout_pq[], unsigned *p_changed,
	unsigned low_taboo, unsigned high_taboo, int loops, gdatp gdp));

static int
p_correct_all(inout_pq, p_changed, low_taboo, high_taboo, loops, gdp)
	unsigned char	inout_pq[];
	unsigned	*p_changed;
	unsigned	low_taboo;
	unsigned	high_taboo;
	int	loops;
	gdatp gdp;
{
	int	have_error[2];
	unsigned	i;
	unsigned	lowcol = COL(low_taboo);
	unsigned	higcol = COL(high_taboo);
	unsigned	lowpos = POSP(low_taboo);
	unsigned	higpos = POSP(high_taboo);

	have_error[0] = have_error[1] = 0;
	for (*p_changed = i = 0; i < L2_P/4; i++) {

		gdp->Pfailed[0][i] = gdp->Pfailed[1][i] = 0;

		if (loops > 0 &&
		    gdp->Psyndromes[2*i] == 0 &&
		    gdp->Psyndromes[2*i+1] == 0 &&
		    gdp->Psyndromes[2*i+L2_P/2] == 0 &&
		    gdp->Psyndromes[2*i+L2_P/2+1] == 0) continue;

		if (!calc_L2_P_syndrome(inout_pq, i, &gdp->Psyndromes[2*i])) {
			unsigned	msb;
			/*
			 * error(s) in column i in layer of p (LSB and MSB)
			 */
			for (msb = 0; msb < 2; msb++) {
				int	retval;

				if (gdp->Psyndromes[2*i+msb] != 0 ||
				    gdp->Psyndromes[2*i+msb+L2_P/2] != 0) {
					if (TELLME) fprintf(stderr, "%2d p %s c %2d: ", loops, msb ? "MSB" : "LSB", i);
if (0) fprintf(stderr, "\nP taboo:%u-%u, c/p:%u/%u - %u/%u\n",
low_taboo, high_taboo, lowcol, lowpos, higcol, higpos);
					retval = correct_P(inout_pq+msb, msb, i,
							&gdp->Psyndromes[2*i+msb], gdp->Pcount[msb][i],
							gdp->Perapos[msb][i],
							i >= lowcol ? lowpos : lowpos+1,
							i <= higcol || higpos == 0 ? higpos : higpos-1,
							loops, gdp);
					*p_changed |= !retval;
					have_error[msb] |= retval;
					if (retval == 0) {
#if DEBUG >= 0
						/* recheck */
						calc_L2_P_syndrome(inout_pq, i, &gdp->Psyndromes[2*i]);
						if (gdp->Psyndromes[2*i+msb] != 0 ||
							gdp->Psyndromes[2*i+msb+L2_P/2] != 0) {
fprintf(stderr, "\n P %s internal error in %s, %d\n", msb ? "MSB" : "LSB", __FILE__, __LINE__);
						}
#endif
						gdp->Psyndromes[2*i+msb] = 0;
						gdp->Psyndromes[2*i+msb+L2_P/2] = 0;
					} else {
						gdp->Pfailed[msb][i] = 1;
					}
					if (TELLME) fputs("\n", stderr);
				} /* error in byte layer */
#if	DEBUG >= 0
				else {
					/*
					 * if this column is clean,
					 * there should be no erasures left
					 */
					if (gdp->Pcount[msb][i] != 0) {
fprintf(stderr, "p: %d stale erasures in column %d, MSB=%d\n", gdp->Pcount[msb][i], i, msb);
					}
				}
#endif
			} /* for LSB and MSB */
		} /* error in column */
	} /* for all columns */
	return (have_error[0] == 0 && have_error[1] == 0);
}


/*
 * q_correct_all: do the correction for all diagonals
 * in the q parity layer.
 *
 * Input/output parameters:
 *  inout_pq:	the byte array with the sector data
 *				after the sync header to be corrected.
 *  q_changed:	pointer to int; set true, if a
 *				correction (change) has been done.
 *  q_changed_p: pointer to int; set true, if a
 *				correction (change) has been done that
 *				affects the range of the p parity layer.
 *
 * Input parameters:
 *  low_taboo:  the lower limit of a range left to be unchanged.
 *  high_taboo: the upper limit of a range left to be unchanged.
 *  loops:		>= 0, the loop count of the iteration
 *	gdp:		is a pointer to auxiliary book keeping data.
 *
 */

static int
q_correct_all __PR((unsigned char inout_pq[], unsigned *q_changed, unsigned *q_changed_p,
	unsigned low_taboo, unsigned high_taboo, int loops, gdatp gdp));

static int
q_correct_all(inout_pq, q_changed, q_changed_p, low_taboo, high_taboo, loops, gdp)
	unsigned char	inout_pq[];
	unsigned *q_changed;
	unsigned *q_changed_p;
	unsigned	low_taboo;
	unsigned	high_taboo;
	int	loops;
	gdatp gdp;
{
	int	have_error[2];
	unsigned	i;

	have_error[0] = have_error[1] = 0;
	for (*q_changed = *q_changed_p = i = 0; i < L2_Q/4; i++) {

		gdp->Qfailed[0][i] = gdp->Qfailed[1][i] = 0;

		if (loops > 0 &&
		    gdp->Qsyndromes[2*i] == 0 &&
		    gdp->Qsyndromes[2*i+1] == 0 &&
		    gdp->Qsyndromes[2*i+L2_Q/2] == 0 &&
		    gdp->Qsyndromes[2*i+L2_Q/2+1] == 0) continue;

		if (!calc_L2_Q_syndrome(inout_pq, i, &gdp->Qsyndromes[2*i])) {
			unsigned msb;
			/*
			 * error(s) in diagonal i in layer of q (LSB and MSB)
			 */
			for (msb = 0; msb < 2; msb++) {
				if (gdp->Qsyndromes[2*i+msb] != 0 ||
				    gdp->Qsyndromes[2*i+msb+L2_Q/2] != 0) {
					int	retval;

					if (TELLME) fprintf(stderr, "%2d q %s d %2d: ", loops, msb ? "MSB" : "LSB", i);

					retval = correct_Q(inout_pq+msb, msb, i,
							&gdp->Qsyndromes[2*i+msb], gdp->Qcount[msb][i],
							gdp->Qerapos[msb][i],
							low_taboo,
							high_taboo,
							q_changed_p,
							gdp);
					*q_changed |= !retval;
					have_error[msb] |= retval;
					if (retval == 0) {
#if DEBUG >= 0
						/* recheck */
						calc_L2_Q_syndrome(inout_pq, i, &gdp->Qsyndromes[2*i]);
						if (gdp->Qsyndromes[2*i+msb] != 0 ||
						    gdp->Qsyndromes[2*i+msb+L2_Q/2] != 0) {
fprintf(stderr, "\n Q %s internal error in %s, %d\n", msb ? "MSB" : "LSB", __FILE__, __LINE__);
						}
#endif
						gdp->Qsyndromes[2*i+msb] = 0;
						gdp->Qsyndromes[2*i+msb+L2_Q/2] = 0;
					} else {
						gdp->Qfailed[msb][i] = 1;
					}
					if (TELLME) fputs("\n", stderr);
				} /* error in byte layer */
#if	DEBUG >= 0
				else {
					/*
					 * if this column is clean,
					 * there should be no erasures left
					 */
					if (gdp->Qcount[msb][i] != 0) {
fprintf(stderr, "q: %d stale erasures in diagonal %d, MSB=%d\n", gdp->Qcount[msb][i], i, msb);
					}
				}
#endif
			} /* for LSB and MSB */
		} /* error in column */
	} /* for all columns */
	return (have_error[0] == 0 && have_error[1] == 0);
}


/*
 * do_decode_L2: main entry point for correction of one
 *		Layer 2 data sector.
 *
 * Input/output parameters:
 *  inout: the byte array holding the raw data sector.
 *			The corrections modify the array.
 *
 * Input parameters:
 *  sectortype: one of MODE_1, MODE_2_FORM_1
 *  have_erasures: the number of given error positions in the
 *				'erasures' byte array.
 *  erasures: a byte array with the size of a raw data sector. Each
 *			byte unequal 0 indicates a position of an error in the
 *			data sector. The number of nonzero bytes should equal
 *			the value of the 'have_erasures' parameter.
 *
 */

int
do_decode_L2 __PR((unsigned char inout[(L2_RAW + L2_Q + L2_P)],
			int sectortype,
			int have_erasures,
			unsigned char erasures[(L2_RAW + L2_Q + L2_P)]));

int
do_decode_L2(inout, sectortype, have_erasures, erasures)
	unsigned char	inout[(L2_RAW + L2_Q + L2_P)];
	int		sectortype;
	int		have_erasures;
	unsigned char	erasures[(L2_RAW + L2_Q + L2_P)];
{
	int	crc_ok;
	int	q_ok = -1;
	int	p_ok = -1;
	unsigned p_changed = 0;
	unsigned q_changed = 0;
	unsigned q_changed_p = 0;
	int	loops;
	unsigned 	low_taboo;
	unsigned 	high_taboo;
	gdat	gdata;
	gdatp	gdp = &gdata;

	if (sectortype != EDC_MODE_1 && sectortype != EDC_MODE_2_FORM_1)
		return (WRONG_TYPE);

	memcpy(inout, "\000\377\377\377\377\377\377\377\377\377\377\000", 12);
	if (sectortype == EDC_MODE_1) {
		low_taboo  = 1;
		high_taboo = 0;
	} else {
		memcpy(inout+12, "\000\000\000\000", 4);
		low_taboo  = 0;
		high_taboo = (4-1)/2;
	}


	init_erasures(have_erasures, erasures, gdp);
	memset(gdp->corrcount, 0, sizeof (gdp->corrcount));
	memset(gdp->Pcorrcount, 0, sizeof (gdp->Pcorrcount));
	memset(gdp->Qcorrcount, 0, sizeof (gdp->Qcorrcount));

	loops = 0;
	do {

		crc_ok = do_crc_check(inout, sectortype, loops, gdp);

		/* we are satisfied, when the crc is correct. */
		if (crc_ok) {
#if	DEBUG >= 3
fprintf(stderr, "loop1:%d crc=%d, p=%d, p_chan=%d, q=%d, q_chan=%d, q_chan_p=%d lo=%d, hi=%d\n",
	loops, crc_ok, p_ok, p_changed, q_ok, q_changed, q_changed_p, low_taboo, high_taboo);
#endif
			break;
		}


		p_ok = p_correct_all(inout+12, &p_changed, low_taboo, high_taboo, loops, gdp);

		if (p_ok && p_changed) {
			crc_ok = do_crc_check(inout, sectortype, loops, gdp);
			/* we are satisfied, when the crc is correct. */
			if (crc_ok) {
#if	DEBUG >= 3
fprintf(stderr, "loop2:%d crc=%d, p=%d, p_chan=%d, q=%d, q_chan=%d, q_chan_p=%d lo=%d, hi=%d\n",
	loops, crc_ok, p_ok, p_changed, q_ok, q_changed, q_changed_p, low_taboo, high_taboo);
#endif
				break;
			}
		}

		if (loops > 0 && !crc_ok && q_ok && p_ok) {
			memset(gdp->Psyndromes, 1, sizeof (gdp->Psyndromes));
			memset(gdp->Qsyndromes, 1, sizeof (gdp->Qsyndromes));
		}
		q_ok = q_correct_all(inout+12, &q_changed, &q_changed_p,
					low_taboo, high_taboo, loops, gdp);

#if	DEBUG >= 2
fprintf(stderr, "loop :%d crc=%d, p=%d, p_chan=%d, q=%d, q_chan=%d, q_chan_p=%d lo=%d, hi=%d\n",
	loops, crc_ok, p_ok, p_changed, q_ok, q_changed, q_changed_p, low_taboo, high_taboo);
#endif
		if (!q_changed_p && crc_ok && q_ok) {
			if (loops == 0 && !q_changed && !p_changed && p_ok)
				return (NO_ERRORS);

			if (p_ok)
				return (FULLY_CORRECTED);
		}
		if ((!q_ok) && !q_changed_p)
			q_changed_p |= unify_erasures(have_erasures, inout+12, gdp);

		loops++;
	} while (q_changed_p && loops < MAXLOOPS);

	if (crc_ok) {
		unsigned	i;

#if	1
		if (!q_changed && !p_changed && loops == 0) {
			for (i = 0, p_ok = 1; i < L2_P/4; i++) {
				p_ok &= calc_L2_P_syndrome(inout+12, i, &gdp->Psyndromes[2*i]);
			}
			for (i = 0, q_ok = 1; i < L2_Q/4; i++) {
				q_ok &= calc_L2_Q_syndrome(inout+12, i, &gdp->Qsyndromes[2*i]);
			}
#if	DEBUG >= 2
fprintf(stderr, "loop :%d crc=%d, p=%d, p_chan=%d, q=%d, q_chan=%d, q_chan_p=%d lo=%d, hi=%d\n",
	-1, crc_ok, p_ok, p_changed, q_ok, q_changed, q_changed_p, low_taboo, high_taboo);
#endif
			if (p_ok && q_ok)
				return (NO_ERRORS);
		}
#endif

		if (p_ok && q_ok && !q_changed_p)
			return (FULLY_CORRECTED);

		/* restore unused, p and q portions */
		if (sectortype == EDC_MODE_1)
			memset(inout + 2064 + 4, 0, 8);
		encode_L2_P(inout + 12);
		encode_L2_Q(inout + 12);

		return (FULLY_CORRECTED);
	}

	crc_ok = do_crc_check(inout, sectortype, 1, gdp);

#if	DEBUG >= 2
fprintf(stderr, "loop:%d crc=%d, p=%d, p_chan=%d, q=%d, q_chan=%d, q_chan_p=%d lo=%d, hi=%d\n",
	99, crc_ok, p_ok, p_changed, q_ok, q_changed, q_changed_p, low_taboo, high_taboo);
#endif
	if (crc_ok) {
		if (p_ok && q_ok)
			return (FULLY_CORRECTED);

		/* restore unused, p and q portions */
		if (sectortype == EDC_MODE_1) memset(inout + 2064 + 4, 0, 8);
		encode_L2_P(inout + 12);
		encode_L2_Q(inout + 12);

		return (FULLY_CORRECTED);
	}
	return (UNCORRECTABLE);
}
#endif
#endif

#ifdef	EDC_SUBCHANNEL
/************************** SUBCHANNEL *********************************/
#ifdef	EDC_DECODER

#undef L12_MODUL
#undef	L12_P_ERRORS
#undef	L12_Q_ERRORS
#undef	P_LENGTH
#undef	Q_LENGTH
#undef	P_SKIPPED
#undef	Q_SKIPPED

#define	LSUB_MODUL	63
#define	LSUB_P_ERRORS	2
#define	LSUB_Q_ERRORS	1
#define	LSUB_P_LENGTH	20
#define	LSUB_Q_LENGTH	2
#define	LSUB_P_SKIPPED	(LSUB_MODUL-LSUB_P_LENGTH-2*LSUB_P_ERRORS)
#define	LSUB_Q_SKIPPED	(LSUB_MODUL-LSUB_Q_LENGTH-2*LSUB_Q_ERRORS)


static int
calc_LSUB_Q_syndrome __PR((unsigned char inout[LSUB_QRAW + LSUB_Q],
unsigned char syndrome[LSUB_Q]));

static int
calc_LSUB_Q_syndrome(inout, syndrome)
	unsigned char inout[LSUB_QRAW + LSUB_Q];
	unsigned char syndrome[LSUB_Q];
{
	unsigned    data;
    unsigned char *Q = syndrome;

	syndrome[0] = syndrome[1] = 0;
	inout += 4-1;

#define	QBODYEND(i)   					\
		data = *inout & (unsigned)0x3f;		\
							\
		*Q++ ^= (unsigned char)data;		\
		*Q   ^= SUBsyn[i][0][data];

#define	QBODY(i)					\
		QBODYEND(i)				\
		inout--; 				\
		Q--;

	QBODY(0)
	QBODY(1)
	QBODY(2)
	QBODYEND(3)
	return ((syndrome[0] | syndrome[1]) != 0);
}

static int
calc_LSUB_P_syndrome __PR((unsigned char inout[LSUB_RAW + LSUB_Q + LSUB_P],
unsigned char syndrome[LSUB_P]));

static int
calc_LSUB_P_syndrome(inout, syndrome)
	unsigned char inout[LSUB_RAW + LSUB_Q + LSUB_P];
	unsigned char syndrome[LSUB_P];
{
	unsigned    data;
    unsigned char *P = syndrome;

	syndrome[0] = syndrome[1] = syndrome[2] = syndrome[3] = 0;
	inout += 24-1;

#define	PBODYEND(i)   					\
		data = *inout & (unsigned)0x3f;		\
							\
		*P++ ^= (unsigned char)data;		\
		*P++ ^= SUBsyn[i][0][data]; 		\
		*P++ ^= SUBsyn[i][1][data]; 		\
		*P   ^= SUBsyn[i][2][data];

#define	PBODY(i)   	\
		PBODYEND(i) \
		inout--; \
		P -= 3;

	PBODY(0)  PBODY(1)  PBODY(2)  PBODY(3)  PBODY(4)
	PBODY(5)  PBODY(6)  PBODY(7)  PBODY(8)  PBODY(9)
	PBODY(10) PBODY(11) PBODY(12) PBODY(13) PBODY(14)
	PBODY(15) PBODY(16) PBODY(17) PBODY(18) PBODY(19)
	PBODY(20) PBODY(21) PBODY(22) PBODYEND(23)

	return ((syndrome[0] | syndrome[1] |
		    syndrome[2] | syndrome[3]) != 0);
}

#ifdef	__needed__
#define	SYND(a)		syndrome[a]
static int
correct_QSUB __PR((unsigned char inout[LSUB_RAW + LSUB_P + LSUB_Q],
	unsigned char syndrome[LSUB_Q],
	unsigned erasures, unsigned char Qerasures[LSUB_Q_LENGTH]));

static int
correct_QSUB(inout, syndrome, erasures, Qerasures)
	unsigned char inout[LSUB_RAW + LSUB_P + LSUB_Q];
	unsigned char syndrome[LSUB_Q];
	unsigned erasures;
	unsigned char Qerasures[LSUB_Q_LENGTH];
{
	unsigned	i;
	unsigned	j;
	unsigned	l;
	unsigned	k;
	unsigned	roots_found;
	unsigned char	err_locations[2*LSUB_Q_ERRORS+1];
	unsigned char	err_values[2*LSUB_Q_ERRORS+1];
	unsigned char	tmp_loc[2*LSUB_Q_ERRORS+1];
	unsigned char	roots[2*LSUB_Q_ERRORS];
	unsigned char	discrepancy;

	INIT_BMD

	if (erasures > 2*LSUB_Q_ERRORS) erasures = 0;
	/*
	 * initialize error location polynomial with roots from given
	 * erasure locations.
	 */
	if (erasures > 0) {

		INIT_ERASURES(Qerasures, LSUB_Q_ERRORS, LSUB_MODUL,
						rs_sub_rw_log, rs_sub_rw_alog)

		if (DEBUG > 2) { DEBUG_ERASURE_POLY(rs_sub_rw_log, rs_sub_rw_alog,
				LSUB_MODUL, LSUB_Q_ERRORS, LSUB_Q_SKIPPED, 0) }
	} /* erasure initialisation */

	/*
	 * calculate the final error location polynomial.
	 */
	for (j = l = erasures, k = 1; j < 2*LSUB_Q_ERRORS; j++, k++) {

		/* calculate the 'discrepancy' */
		CALC_DISCREPANCY(rs_sub_rw_log, rs_sub_rw_alog)

		/* test discrepancy for zero */
		if (discrepancy != 0) {
			BUILD_SHIFT_REGISTER(rs_sub_rw_log, rs_sub_rw_alog,
					LSUB_MODUL, LSUB_Q_ERRORS)
		}

#if	DEBUG > 2
		fprintf(stderr, "Qdisc=%d, ", discrepancy);
		fprintf(stderr, "j=%d, ", j);
		fprintf(stderr, "k=%d, l(errors)=%d, e0=%d, e1=%d, B0=%d, B1=%d\n",
				k, l, err_locations[0], err_locations[1], tmp_loc[0], tmp_loc[1]);
#endif
	}

	if (l > LSUB_Q_ERRORS+erasures/2)
		return (1);

	/* find roots of err_locations */
	FIND_ROOTS(rs_sub_rw_log, rs_sub_rw_alog, LSUB_MODUL, LSUB_Q_ERRORS,
		LSUB_Q_SKIPPED, 0)

	if (roots_found != l) {
		return (1);
	}

	/*
	 * l holds the number of errors
	 *
	 * now calculate the error valuator
	 */
	CALC_ERROR_VALUATOR(rs_sub_rw_log, rs_sub_rw_alog)

	APPLY_FORNEY_ALGORITHM(rs_sub_rw_log, rs_sub_rw_alog, LSUB_MODUL,
		LSUB_Q_SKIPPED, DO_SUB_CORRECTION)

	return (roots_found ? 0 : 1);
}
#undef	SYND
#endif	/* __needed__ */

#ifdef	__needed__
#define	SYND(a)		syndrome[a]
static int
correct_PSUB __PR((unsigned char inout[LSUB_RAW + LSUB_P + LSUB_Q],
	unsigned char syndrome[LSUB_P],
	unsigned erasures, unsigned char Perasures[LSUB_P_LENGTH]));

static int
correct_PSUB(inout, syndrome, erasures, Perasures)
	unsigned char inout[LSUB_RAW + LSUB_P + LSUB_Q];
	unsigned char syndrome[LSUB_P];
	unsigned erasures;
	unsigned char Perasures[LSUB_P_LENGTH];
{
	unsigned	i;
	unsigned	j;
	unsigned	l;
	unsigned	k;
	unsigned	roots_found;
	unsigned char	err_locations[2*LSUB_P_ERRORS+1];
	unsigned char	err_values[2*LSUB_P_ERRORS+1];
	unsigned char	tmp_loc[2*LSUB_P_ERRORS+1];
	unsigned char	roots[2*LSUB_P_ERRORS];
	unsigned char	discrepancy;

	INIT_BMD

	if (erasures > 2*LSUB_P_ERRORS) erasures = 0;
	/*
	 * initialize error location polynomial with roots from given
	 * erasure locations.
	 */
	if (erasures > 0) {

		INIT_ERASURES(Perasures, LSUB_P_ERRORS, LSUB_MODUL,
						rs_sub_rw_log, rs_sub_rw_alog)

		if (DEBUG > 2) { DEBUG_ERASURE_POLY(rs_sub_rw_log, rs_sub_rw_alog,
				LSUB_MODUL, LSUB_P_ERRORS, LSUB_P_SKIPPED, 0) }
	} /* erasure initialisation */

	/*
	 * calculate the final error location polynomial.
	 */
	for (j = l = erasures, k = 1; j < 2*LSUB_P_ERRORS; j++, k++) {

		/* calculate the 'discrepancy' */
		CALC_DISCREPANCY(rs_sub_rw_log, rs_sub_rw_alog)

		/* test discrepancy for zero */
		if (discrepancy != 0) {
			BUILD_SHIFT_REGISTER(rs_sub_rw_log, rs_sub_rw_alog,
					LSUB_MODUL, LSUB_P_ERRORS)
		}
#if	DEBUG > 2
fprintf(stderr, "Pdisc=%d, ", discrepancy);
fprintf(stderr, "j=%d, ", j);
fprintf(stderr, "k=%d, l(errors)=%d, e0=%d, e1=%d, B0=%d, B1=%d\n",
	k, l, err_locations[0], err_locations[1], tmp_loc[0], tmp_loc[1]);
#endif
	}

	if (l > LSUB_P_ERRORS+erasures/2)
		return (1);

	/* find roots of err_locations */
	FIND_ROOTS(rs_sub_rw_log, rs_sub_rw_alog, LSUB_MODUL, LSUB_P_ERRORS,
		LSUB_P_SKIPPED, 0)

	if (roots_found != l) {
		return (1);
	}

	/*
	 * l holds the number of errors
	 *
	 * now calculate the error valuator
	 */
	CALC_ERROR_VALUATOR(rs_sub_rw_log, rs_sub_rw_alog)

	/*
	 * apply the forney algorithm
	 */
	APPLY_FORNEY_ALGORITHM(rs_sub_rw_log, rs_sub_rw_alog, LSUB_MODUL,
		LSUB_P_SKIPPED, DO_SUB_CORRECTION)

	return (roots_found ? 0 : 1);
}
#undef	SYND
#endif	/* __needed__ */


#ifdef	DECODE_SUB_IN_ENCODER	/* XXX we need to fix this in edc_ecc.c */
int
do_decode_sub __PR((
	unsigned char inout[(LSUB_RAW + LSUB_Q + LSUB_P)*PACKETS_PER_SUBCHANNELFRAME],
	int have_erasures,
	unsigned char erasures[(LSUB_RAW +LSUB_Q +LSUB_P)*PACKETS_PER_SUBCHANNELFRAME],
	int results[PACKETS_PER_SUBCHANNELFRAME]
));

int
do_decode_sub(inout, have_erasures, erasures, results)
	unsigned char inout[(LSUB_RAW + LSUB_Q + LSUB_P) * PACKETS_PER_SUBCHANNELFRAME];
	int have_erasures;
	unsigned char erasures[(LSUB_RAW + LSUB_Q + LSUB_P) * PACKETS_PER_SUBCHANNELFRAME];
	int results[PACKETS_PER_SUBCHANNELFRAME];
{
	static unsigned char Psyndromes[LSUB_P * PACKETS_PER_SUBCHANNELFRAME];
	static unsigned char Qsyndromes[LSUB_Q * PACKETS_PER_SUBCHANNELFRAME];
	static unsigned char Perapos[(LSUB_RAW+LSUB_Q+LSUB_P) * PACKETS_PER_SUBCHANNELFRAME];
	static unsigned char Qerapos[(LSUB_QRAW+LSUB_Q) * PACKETS_PER_SUBCHANNELFRAME];
	static unsigned char Pcount[PACKETS_PER_SUBCHANNELFRAME];
	static unsigned char Qcount[PACKETS_PER_SUBCHANNELFRAME];
	unsigned char *Psyndromesp = Psyndromes;
	unsigned char *Qsyndromesp = Qsyndromes;
	unsigned char *Peraposp = Perapos;
	unsigned char *Qeraposp = Qerapos;
	unsigned char *Pcountp = Pcount;
	unsigned char *Qcountp = Qcount;
	int	packet;
	int	have_error = 1;
	int 	error = 0;
	int 	changed_Q;
	int 	changed_P;
	int 	retval = -12;

	memset(Perapos, 0, sizeof (Perapos));
	memset(Qerapos, 0, sizeof (Qerapos));
	memset(Pcount, 0, sizeof (Pcount));
	memset(Qcount, 0, sizeof (Qcount));
	if (have_erasures != 0) {
		unsigned char i;
		/*
		 * separate erasures for Q and P level.
		 */
		for (i = 0; i < (LSUB_QRAW + LSUB_Q)* PACKETS_PER_SUBCHANNELFRAME; i++) {
			if (erasures[i] != 0) {
				Qerapos[i] = (unsigned char)(i + LSUB_Q_SKIPPED);
				Qcount[i/(LSUB_QRAW + LSUB_Q)]++;
			}
			if ((i % (LSUB_QRAW + LSUB_Q)) == LSUB_QRAW + LSUB_Q - 1) {
				erasures += LSUB_RAW + LSUB_P + LSUB_Q;
			}
		}
		erasures -= (LSUB_RAW + LSUB_P + LSUB_Q)* PACKETS_PER_SUBCHANNELFRAME;
		for (i = 0; i < (LSUB_RAW + LSUB_P + LSUB_Q)* PACKETS_PER_SUBCHANNELFRAME; i++) {
			if (erasures[i] != 0) {
				Perapos[i] = (unsigned char)(i + LSUB_P_SKIPPED);
				Pcount[i/(LSUB_QRAW + LSUB_Q)]++;
			}
		}
	}
	for (have_error = 1, packet = 0;
		packet < PACKETS_PER_SUBCHANNELFRAME;
		packet++,
		Qsyndromesp += LSUB_Q, Psyndromesp += LSUB_P,
		Qcountp++, Pcountp++,
		Qeraposp += LSUB_QRAW + LSUB_Q,
		Peraposp += LSUB_RAW + LSUB_Q + LSUB_P,
		inout += 24) {
		int iCnt;

		for (iCnt = 0; have_error && iCnt < 2; iCnt++) {
			unsigned i = 0;
			changed_Q = have_error = 0;

			if (calc_LSUB_P_syndrome(inout, Psyndromesp)) {
				error |= 2 << (2*i);

				retval = correct_PSUB(inout, Psyndromesp, 0, NULL);
				have_error |= retval;
				changed_P = !retval;
				if (retval == 0) {
					Pcountp[0] = 0;
#if DEBUG >= 0
					calc_LSUB_P_syndrome(inout, Psyndromesp);
					if (Psyndromesp[0] != 0 ||
					    Psyndromesp[1] != 0 ||
					    Psyndromesp[2] != 0 ||
					    Psyndromesp[3] != 0) {
fprintf(stderr, "\nSubP internal error in %s, %d\n", __FILE__, __LINE__);
					}
#endif
				}
			}
			if (calc_LSUB_Q_syndrome(inout, Qsyndromesp)) {
				error |= 1 << (2*i);
				retval = correct_QSUB(inout, Qsyndromesp, 0, NULL);
				have_error |= retval;
				changed_Q = !retval;
				if (retval == 0) {
					Qcountp[0] = 0;
#if DEBUG >= 0
					calc_LSUB_Q_syndrome(inout, Qsyndromesp);
					if (Qsyndromesp[0] != 0 ||
					    Qsyndromesp[1] != 0) {
fprintf(stderr, "\nSubQ internal error in %s, %d\n", __FILE__, __LINE__);
					}
#endif
				}
			}
			if (results != NULL) {
				results[packet] = (error & (3 << (2*i))) != 0;
			}
		}
	}
	return (have_error);
}
#endif	/* DECODE_SUB_IN_ENCODER	XXX we need to fix this in edc_ecc.c */

int check_sub __PR((unsigned char input[]));

int
check_sub(input)
	unsigned char input[];
{
	int error = 0;
	int packet;

	for (packet = 0;
		!error && packet < PACKETS_PER_SUBCHANNELFRAME;
		packet++, input += 24) {
		unsigned char syndromes[4];

		error |= calc_LSUB_P_syndrome(input, syndromes);
		error |= calc_LSUB_Q_syndrome(input, syndromes);
	}
	return (error);
}
#endif
#endif
