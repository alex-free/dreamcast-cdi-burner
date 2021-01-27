/* @(#)libscgcmd.h	1.1 08/10/26 Copyright 1986-2008 J. Schilling */

#ifndef	_LIBSCGCMD_H
#define	_LIBSCGCMD_H

/*
 * buffer.c
 */
extern	int	read_buffer	__PR((SCSI *scgp, caddr_t bp, int cnt, int mode));
extern	int	write_buffer	__PR((SCSI *scgp, char *buffer, long length, int mode, int bufferid, long offset));

/*
 * inquiry.c
 */
extern	int	inquiry		__PR((SCSI *scgp, caddr_t, int));

/*
 * modes.c
 */
extern	BOOL	get_mode_params	__PR((SCSI *scgp, int page, char *pagename,
					Uchar *modep, Uchar *cmodep,
					Uchar *dmodep, Uchar *smodep,
					int *lenp));
extern	BOOL	set_mode_params	__PR((SCSI *scgp, char *pagename, Uchar *modep,
					int len, int save, int secsize));

/*
 * modesense.c
 */
extern	BOOL	__is_atapi	__PR((void));
extern	BOOL	allow_atapi	__PR((SCSI *scgp, BOOL new));
extern	int	mode_select	__PR((SCSI *scgp, Uchar *, int, int, int));
extern	int	mode_sense	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
extern	int	mode_select_sg0	__PR((SCSI *scgp, Uchar *, int, int, int));
extern	int	mode_sense_sg0	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
extern	int	mode_select_g0	__PR((SCSI *scgp, Uchar *, int, int, int));
extern	int	mode_select_g1	__PR((SCSI *scgp, Uchar *, int, int, int));
extern	int	mode_sense_g0	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
extern	int	mode_sense_g1	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));

/*
 * read.c
 */
extern	int	read_scsi	__PR((SCSI *scgp, caddr_t, long, int));
extern	int	read_g0		__PR((SCSI *scgp, caddr_t, long, int));
extern	int	read_g1		__PR((SCSI *scgp, caddr_t, long, int));

/*
 * readcap.c
 */
extern	int	read_capacity	__PR((SCSI *scgp));
#ifdef	EOF	/* stdio.h has been included */
extern	void	print_capacity	__PR((SCSI *scgp, FILE *f));
#endif

/*
 * ready.c
 */
extern	BOOL	unit_ready	__PR((SCSI *scgp));
extern	BOOL	wait_unit_ready	__PR((SCSI *scgp, int secs));
extern	int	test_unit_ready	__PR((SCSI *scgp));

#endif	/* _LIBSCGCMD_H */
