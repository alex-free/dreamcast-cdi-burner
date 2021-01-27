#ifndef __VMS_INIT_H
#define	__VMS_INIT_H

/*
 * 2005-03-06 SMS.
 *
 * Header file for general VMS-specific functions:
 *    Exit handler
 *    LIB$INITIALIZE DECC features
 *
 * User must establish:
 *
 *    int vms_init_done;
 *    decc_feat_t decc_feat_array[];
 *    int vms_init_diag();
 *
 */

/* Common test for DECC$* initialization availability. */

#if !defined(__VAX) && (__CRTL_VER >= 70301000)

#	define DECC_INIT

#endif /* !defined(__VAX) && (__CRTL_VER >= 70301000) */

/*--------------------------------------------------------------------*/

#include <unixlib.h>

/* DECC$* feature name and value structure. */

#ifdef DECC_INIT

/* Structure to hold a DECC$* feature name and its desired value. */

typedef struct
	{
	char *name;
	int value;
	} decc_feat_t;

#endif /* def DECC_INIT */

/*--------------------------------------------------------------------*/

/* Global storage. */

/*	Flag to sense if vms_init() was called. */

extern int vms_init_done;

#  ifdef DECC_INIT

/* Array of DECC$* feature names and their desired values. */

extern decc_feat_t decc_feat_array[];

#  endif /* def DECC_INIT */

/*--------------------------------------------------------------------*/

/* Function prototypes. */

extern int acc_cb();

extern int vms_init_diag(void);

extern int vms_set_prio(int prio);

/*--------------------------------------------------------------------*/

#endif /* ndef __VMS_INIT_H */
