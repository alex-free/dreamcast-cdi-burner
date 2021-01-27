/* @(#)scsi_cmds.h	1.19 15/10/19 Copyright 1998,1999,2015 Heiko Eissfeldt, Copyright 2004-2013 J. Schilling */
/*
 * header file for scsi_cmds.c
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

#ifndef	_SCSI_CMDS_H
#define	_SCSI_CMDS_H

extern	int		accepts_fua_bit;
extern	unsigned char	density;
extern	unsigned char	orgmode4;
extern	unsigned char	orgmode10;
extern	unsigned char	orgmode11;

extern	int	SCSI_emulated_ATAPI_on	__PR((SCSI *scgp));
extern	unsigned char *ScsiInquiry	__PR((SCSI *scgp));
extern	int	TestForMedium		__PR((SCSI *scgp));
extern	void	SpeedSelectSCSIMMC	__PR((SCSI *scgp, unsigned speed));
extern	void	SpeedSelectSCSIYamaha	__PR((SCSI *scgp, unsigned speed));
extern	void	SpeedSelectSCSISony	__PR((SCSI *scgp, unsigned speed));
extern	void	SpeedSelectSCSIPhilipsCDD2600 __PR((SCSI *scgp,
						unsigned speed));
extern	void	SpeedSelectSCSINEC	__PR((SCSI *scgp, unsigned speed));
extern	void	SpeedSelectSCSIToshiba	__PR((SCSI *scgp, unsigned speed));
extern	subq_chnl *ReadSubQSCSI		__PR((SCSI *scgp,
						unsigned char sq_format,
						unsigned char ltrack));
extern	subq_chnl *ReadSubChannelsSony __PR((SCSI *scgp, unsigned lSector));
extern	subq_chnl *ReadSubChannelsFallbackMMC __PR((SCSI *scgp,
						unsigned lSector));
extern	subq_chnl *ReadStandardSub	__PR((SCSI *scgp, unsigned lSector));
extern	int	ReadCddaMMC12		__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SectorBurstVal));
extern	int	ReadCddaMMC12_C2	__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SectorBurstVal));
extern	int	ReadCdda12Matsushita	__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SectorBurstVal));
extern	int	ReadCdda12		__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SecorBurstVal));
extern	int	ReadCdda12_C2		__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SecorBurstVal));
extern	int	ReadCdda10		__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SecorBurstVal));
extern	int	ReadStandard		__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SctorBurstVal));
extern	int	ReadStandardData	__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SctorBurstVal));
extern	int	ReadCddaFallbackMMC	__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SctorBurstVal));
extern	int	ReadCddaFallbackMMC_C2	__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SctorBurstVal));
extern	int	ReadCddaNoFallback_C2	__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SctorBurstVal));
extern	int	ReadCddaSubSony		__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SectorBurstVal));
extern	int	ReadCddaSub96Sony	__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SectorBurstVal));
extern	int	ReadCddaSubMMC12	__PR((SCSI *scgp, UINT4 *p,
						unsigned lSector,
						unsigned SectorBurstVal));
extern	unsigned ReadTocSony		__PR((SCSI *scgp));
extern	unsigned ReadTocMMC		__PR((SCSI *scgp));
extern	unsigned ReadTocSCSI		__PR((SCSI *scgp));
extern	unsigned ReadFirstSessionTOCSony __PR((SCSI *scgp));
extern	unsigned ReadFirstSessionTOCMMC	__PR((SCSI *scgp));
extern	void	ReadTocTextSCSIMMC	__PR((SCSI *scgp));
extern	int	Play_atSCSI		__PR((SCSI *scgp,
						unsigned int from_sector,
						unsigned int sectors));
extern	int	StopPlaySCSI		__PR((SCSI *scgp));
extern	void	EnableCddaModeSelect	__PR((SCSI *scgp, int fAudioMode,
						unsigned uSectorsize));
extern	int	set_sectorsize		__PR((SCSI *scgp,
						unsigned int secsize));
extern	unsigned int get_orig_sectorsize __PR((SCSI *scgp, unsigned char *m4,
						unsigned char *m10,
						unsigned char *m11));
extern	int	heiko_mmc		__PR((SCSI *scgp));
extern	void	init_scsibuf		__PR((SCSI *scgp, long amt));
extern	int	myscsierr		__PR((SCSI *scgp));

#endif	/* _SCSI_CMDS_H */
