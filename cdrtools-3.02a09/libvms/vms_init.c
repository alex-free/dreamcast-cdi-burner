/*
 * 2005-03-06 SMS.
 *
 * General VMS-specific functions:
 *    vms_set_prio() - Set process base priority.
 *    vms_exit_handler() - Exit handler.
 *    vms_init() - LIB$INITIALIZE DECC features.
 */

#include <schily/mconfig.h>		/* For NICE. */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#include <jpidef.h>
#include <rms.h>
#include <ssdef.h>
#include <starlet.h>
#include <stsdef.h>

#include "vms_init.h"


/* GETxxx item descriptor structure. */
typedef struct
	{
	short buf_len;
	short itm_cod;
	void *buf;
	int *ret_len;
	} xxx_item_t;


/* Set process base priority.  (To arg-1.  0 -> restore original value.) */

int
vms_set_prio(int prio)
{
	int sts;			/* System service status. */

	static int prio_base;		/* Current process base priority. */
	static int prio_prev;		/* Previous process base priority. */

	/* Item list structure to get proc base prio using SYS$GETJPI(). */
	struct {
		xxx_item_t jpi_prib_itm;
		int term;
	} jpi_itm_lst =
	    {	{ sizeof (prio_base), JPI$_PRIB, &prio_base, 0 },
		0
	    };

	static int prio_base_orig = -1;	/* Original process base priority. */

	if (prio == 0) {
		/* Reset process base priority to its original value. */
		if (prio_base_orig >= 0) {
			sts = sys$setpri(0, 0, prio_base_orig,
			    &prio_prev, 0, 0);
			if ((sts& STS$M_SEVERITY) != STS$K_SUCCESS) {
				return (-1);
			}
		}
	} else if ((prio < 0) || (prio > 65)) {
		/* Invalid priority value. */
		return (-2);
	} else {
		/* Get and save the original process base priority. */
		if (prio_base_orig < 0) {
			sts = sys$getjpiw(0,	/* Event flag. */
			    0,			/* Process ID. */
			    0,			/* Process name. */
			    &jpi_itm_lst,	/* Item list. */
			    0,			/* IOSB. */
			    0,			/* AST address. */
			    0);			/* AST parameter. */

			if ((sts& STS$M_SEVERITY) == STS$K_SUCCESS) {
				prio_base_orig = prio_base;
			}
		}

		/* Set process priority as requested. */
		sts = sys$setpri(0, 0, (prio- 1), &prio_prev, 0, 0);

		if ((sts& STS$M_SEVERITY) != STS$K_SUCCESS) {
			return (-3);
		} else {
			sts = sys$getjpiw(0,	/* Event flag. */
			    0,			/* Process ID. */
			    0,			/* Process name. */
			    &jpi_itm_lst,	/* Item list. */
			    0,			/* IOSB. */
			    0,			/* AST address. */
			    0);			/* AST parameter. */

			if ((sts& STS$M_SEVERITY) != STS$K_SUCCESS) {
				return (-4);
			} else if (prio_base != (prio- 1)) {
				return (-5);
			}
		}
	}
	/* Success. */
	return (0);
}


/*
 *    Exit handler to restore base process priority as CDRECORD exits.
 *
 * Unlike UNIX, VMS does not create a new process to run CDRECORD, so
 * the increased process priority (nice( -x)) persists after CDRECORD
 * exits, unless some steps are taken to restore it.
 */

/* VMS exit handler. */

int
vms_exit_handler(int *condition)
{
	/* Restore process base priority. */
	NICE(0);
	return (SS$_NORMAL);
}

/* Establish the VMS exit handler. */

void
establish_vms_exit_handler(void)
{
/* Required first argument for exit handler. */

	static unsigned int condition;

/* Exit handler control block. */

	static struct ECB {
		unsigned int *reserved;
		int (*handler)(int *);
		unsigned int arg_count;
		unsigned int *condition;
	} vms_exit_handler_control_block;

	int sts;			/* System service status. */

/*
 * Fill in the exit handler control block.
 * The exotic calculation for .arg_count accomodates possible future
 * additional user arguments.
 */
	vms_exit_handler_control_block.reserved = NULL;
	vms_exit_handler_control_block.handler = vms_exit_handler;
	vms_exit_handler_control_block.arg_count =
	    ((sizeof (vms_exit_handler_control_block)-
	    offsetof(struct ECB, condition))/ sizeof (int));

	vms_exit_handler_control_block.condition = &condition;

	/* Declare the exit handler. */

	sts = sys$dclexh(&vms_exit_handler_control_block);

	if ((sts& STS$M_SEVERITY) != STS$K_SUCCESS) {
		printf(" Error establishing VMS exit handler.  Status = %%x%08x.\n",
		sts);
	}
}

/*
 * 2004-10-06 SMS.
 *    LIB$INITIALIZE initialization.
 *
 * On all systems, establish the exit handler.
 * On sufficiently recent non-VAX systems, set a collection of C RTL
 * features without using the DECC$* logical name method.
 *
 *
 * Note: Old VAX VMS versions may suffer from a linker complaint like
 * this:
 *
 * %LINK-W-MULPSC, conflicting attributes for psect LIB$INITIALIZE
 *      in module LIB$INITIALIZE file SYS$COMMON:[SYSLIB]STARLET.OLB;1
 *
 * Using a LINK options file which includes a line like this one should
 * stop this complaint:
 *
 * PSECT_ATTR=LIB$INITIALIZE,NOPIC
 *
 */

#include <unixlib.h>

/*--------------------------------------------------------------------*/

/*
 * vms_init()
 *
 *    Uses LIB$INITIALIZE to set a collection of C RTL features without
 *    requiring the user to define the corresponding logical names.
 */

/* LIB$INITIALIZE initialization function. */

static void vms_init(void) {
/* Set the global flag to indicate that LIB$INITIALIZE worked. */

	vms_init_done = 1;

#ifdef DECC_INIT

	{ /* Begin decc$feature block. */

	int feat_index;
	int feat_value;
	int feat_value_max;
	int feat_value_min;
	int i;
	int sts;

	/* Loop through all items in the decc_feat_array[]. */

	for (i = 0; decc_feat_array[ i].name != NULL; i++) {
		/* Get the feature index. */
		feat_index = decc$feature_get_index(decc_feat_array[ i].name);
		if (feat_index >= 0) {
			/* Valid item.  Collect its properties. */
			feat_value = decc$feature_get_value(feat_index, 1);
			feat_value_min = decc$feature_get_value(feat_index, 2);
			feat_value_max = decc$feature_get_value(feat_index, 3);

			if ((decc_feat_array[ i].value >= feat_value_min) &&
			    (decc_feat_array[ i].value <= feat_value_max)) {
				/* Valid value.  Set it if necessary. */
				if (feat_value != decc_feat_array[ i].value) {
					sts = decc$feature_set_value(feat_index, 1,
					    decc_feat_array[ i].value);
				}
			} else {
				/* Invalid DECC feature value. */
				printf(
				    " INVALID DECC FEATURE VALUE, %d: %d <= %s <= %d.\n",
				    feat_value,
				    feat_value_min, decc_feat_array[ i].name,
				    feat_value_max);
			}
		} else {
			/* Invalid DECC feature name. */
			printf(" UNKNOWN DECC FEATURE: %s.\n",
			    decc_feat_array[ i].name);
			}
		}
	} /* End decc$feature block. */

#endif /* def DECC_INIT */

	/* Establish the exit handler. */
	establish_vms_exit_handler();
}

/* Get "vms_init()" into a valid, loaded LIB$INITIALIZE PSECT. */

#pragma nostandard

/*
 * Establish the LIB$INITIALIZE PSECTs, with proper alignment and
 * other attributes.  Note that "nopic" is significant only on VAX.
 */
#pragma extern_model save

#pragma extern_model strict_refdef "LIB$INITIALIZE" 2, nopic, nowrt
void (*const x_vms_init)() = vms_init;

#pragma extern_model strict_refdef "LIB$INITIALIZ" 2, nopic, nowrt
const int spare[ 8] = { 0 };

#pragma extern_model restore

/* Fake reference to ensure loading the LIB$INITIALIZE PSECT. */

#pragma extern_model save
	int lib$initialize(void);
#pragma extern_model strict_refdef
	int dmy_lib$initialize = (int) lib$initialize;
#pragma extern_model restore

#pragma standard


/*
 * 2004-11-23 SMS.
 *
 *       get_rms_defaults().
 *
 *    Get user-specified values from (DCL) SET RMS_DEFAULT.  FAB/RAB
 *    items of particular interest are:
 *
 *       fab$w_deq         default extension quantity (blocks) (write).
 *       rab$b_mbc         multi-block count.
 *       rab$b_mbf         multi-buffer count (used with rah and wbh).
 */

/* Default RMS parameter values. */

#define	RMS_DEQ_DEFAULT	16384	/* About 1/4 the max (65535 blocks). */
#define	RMS_MBC_DEFAULT	127	/* The max, */
#define	RMS_MBF_DEFAULT	2	/* Enough to enable rah and wbh. */

/* Durable storage */

static int rms_defaults_known = 0;

/* JPI item buffers. */
static unsigned short rms_ext;
static char rms_mbc;
static unsigned char rms_mbf;

/* Active RMS item values. */
unsigned short rms_ext_active;
char rms_mbc_active;
unsigned char rms_mbf_active;

/* GETJPI item lengths. */
static int rms_ext_len;		/* Should come back 2. */
static int rms_mbc_len;		/* Should come back 1. */
static int rms_mbf_len;		/* Should come back 1. */

/*
 * Desperation attempts to define unknown macros.  Probably doomed.
 * If these get used, expect sys$getjpiw() to return %x00000014 =
 * %SYSTEM-F-BADPARAM, bad parameter value.
 * They keep compilers with old header files quiet, though.
 */
#ifndef JPI$_RMS_EXTEND_SIZE
#  define JPI$_RMS_EXTEND_SIZE 542
#endif /* ndef JPI$_RMS_EXTEND_SIZE */

#ifndef JPI$_RMS_DFMBC
#  define JPI$_RMS_DFMBC 535
#endif /* ndef JPI$_RMS_DFMBC */

#ifndef JPI$_RMS_DFMBFSDK
#  define JPI$_RMS_DFMBFSDK 536
#endif /* ndef JPI$_RMS_DFMBFSDK */

/* GETJPI item descriptor set. */

struct
	{
	xxx_item_t rms_ext_itm;
	xxx_item_t rms_mbc_itm;
	xxx_item_t rms_mbf_itm;
	int term;
	} jpi_itm_lst =
	{ { 2, JPI$_RMS_EXTEND_SIZE, &rms_ext, &rms_ext_len },
	{ 1, JPI$_RMS_DFMBC, &rms_mbc, &rms_mbc_len },
	{ 1, JPI$_RMS_DFMBFSDK, &rms_mbf, &rms_mbf_len },
	0
	};

int
get_rms_defaults(void)
{
	int sts;

	/* Get process RMS_DEFAULT values. */

	sts = sys$getjpiw(0, 0, 0, &jpi_itm_lst, 0, 0, 0);
	if ((sts& STS$M_SEVERITY) != STS$K_SUCCESS) {
		/* Failed.  Don't try again. */
		rms_defaults_known = -1;
	} else {
		/* Fine, but don't come back. */
		rms_defaults_known = 1;
	}

	/* Limit the active values according to the RMS_DEFAULT values. */

	if (rms_defaults_known > 0) {
		/* Set the default values. */

		rms_ext_active = RMS_DEQ_DEFAULT;
		rms_mbc_active = RMS_MBC_DEFAULT;
		rms_mbf_active = RMS_MBF_DEFAULT;

		/* Default extend quantity.  Use the user value, if set. */
		if (rms_ext > 0) {
			rms_ext_active = rms_ext;
		}

		/* Default multi-block count.  Use the user value, if set. */
		if (rms_mbc > 0) {
			rms_mbc_active = rms_mbc;
		}

		/* Default multi-buffer count.  Use the user value, if set. */
		if (rms_mbf > 0) {
			rms_mbf_active = rms_mbf;
		}
	}

	if (vms_init_diag() > 0) {
		fprintf(stderr,
		    "Get RMS defaults.  getjpi sts = %%x%08x.\n",
		    sts);

		if (rms_defaults_known > 0) {
			fprintf(stderr,
			    "               Default: deq = %6d, mbc = %3d, mbf = %3d.\n",
			    rms_ext, rms_mbc, rms_mbf);
		}
	}
return (sts);
}

#ifdef __DECC

/*
 * 2004-11-23 SMS.
 *
 *       acc_cb(), access callback function for DEC C [f]open().
 *
 *    Set some RMS FAB/RAB items, with consideration of user-specified
 * values from (DCL) SET RMS_DEFAULT.  Items of particular interest are:
 *
 *       fab$w_deq         default extension quantity (blocks).
 *       rab$b_mbc         multi-block count.
 *       rab$b_mbf         multi-buffer count (used with rah and wbh).
 *
 *    See also the FOP* macros in OSDEP.H.  Currently, no notice is
 * taken of the caller-ID value, but options could be set differently
 * for read versus write access.  (I assume that specifying fab$w_deq,
 * for example, for a read-only file has no ill effects.)
 */

/* acc_cb() */

int
acc_cb(int *id_arg, struct FAB *fab, struct RAB *rab)
{
	int sts;

	/* Get process RMS_DEFAULT values, if not already done. */
	if (rms_defaults_known == 0) {
		get_rms_defaults();
	}

	/*
	 * If RMS_DEFAULT (and adjusted active) values are available, then set
	 * the FAB/RAB parameters.  If RMS_DEFAULT values are not available,
	 * suffer with the default parameters.
	 */
	if (rms_defaults_known > 0) {
		/* Set the FAB/RAB parameters accordingly. */
		fab-> fab$w_deq = rms_ext_active;
		rab-> rab$b_mbc = rms_mbc_active;
		rab-> rab$b_mbf = rms_mbf_active;

		/* Truncate at EOF on close, as we'll probably over-extend. */
		fab-> fab$v_tef = 1;

		/* If using multiple buffers, enable read-ahead and write-behind. */
		if (rms_mbf_active > 1)	{
		rab-> rab$v_rah = 1;
		rab-> rab$v_wbh = 1;
		}

		if (vms_init_diag() > 0) {
			fprintf(stderr,
			    "Open callback.  ID = %d, deq = %6d, mbc = %3d, mbf = %3d.\n",
			    *id_arg, fab-> fab$w_deq, rab-> rab$b_mbc, rab-> rab$b_mbf);
		}
	}

	/* Declare success. */
	return (0);
}

#endif /* def __DECC */
