/* @(#)vms.h	1.3 04/03/01 eric */
/*
 * Header file mkisofs.h - assorted structure definitions and typecasts.
 *
 *   Written by Eric Youngdale (1993).
 */

#ifdef VMS
#define	stat(X, Y)	VMS_stat(X, Y)
#define	lstat		VMS_stat

/* gmtime not available under VMS - make it look like we are in Greenwich */
#define	gmtime	localtime

extern int	vms_write_one_file(char *filename, off_t size, FILE * outfile);

#endif
