#ident @(#)Makefile	1.53 13/04/21 Copyr 1995-2013 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		cdrecord
INSMODE=	4711
INSUSR=		root
CPPOPTS +=	-I../libscg
CPPOPTS +=	-I../libedc
CPPOPTS +=	-I../libcdrdeflt
CPPOPTS +=	-DCDRECORD	# Signal that we compile cdrecord
CPPOPTS +=	-DDVD_MULTI
CPPOPTS +=	-DDRV_DVD
CPPOPTS +=	-DDRV_BD
CPPOPTS +=	-DFIFO
CPPOPTS +=	-DAUINFO
CPPOPTS +=	-DHAVE_LIB_EDC_ECC
CPPOPTS +=	-DCLONE_WRITE
CPPOPTS +=	-DVENDOR_CMDS
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_cdrtools\"

CFILES=		cdrecord.c \
		cd_misc.c fifo.c isosize.c audiosize.c auinfo.c \
		cue.c \
		diskid.c \
		wm_session.c wm_track.c wm_packet.c \
		sector.c clone.c \
		movesect.c subchan.c cdtext.c \
		cdr_drv.c \
		drv_philips.c drv_sony.c drv_jvc.c drv_7501.c drv_mmc.c \
		drv_dvd.c drv_dvdplus.c \
		drv_bd.c \
		drv_simul.c \
		vendor.c \
		scsi_scan.c scsi_cdr.c scsi_cdr_mmc4.c scsi_mmc.c scsi_mmc4.c \
		scsilog.c \
		misc.c crc16.c xio.c priv.c
HFILES=		cdrecord.h iso9660.h auheader.h cdtext.h \
		crc16.h movesect.h mmcvendor.h scsi_scan.h \
		scsimmc.h \
		scsilog.h adip.h \
		xio.h \
		version.h
HFILES_TAG=	../include/scg/scgio.h ../include/scg/scsidefs.h \
		../include/scg/scsireg.h ../include/scg/scsitransp.h
#LIBS=		-lschily $(LIB_SOCKET)
#
# LIB_CAP is needed for Linux capability support in librscg.
#
LIBS=		-lscgcmd -lrscg -lscg $(LIB_VOLMGT) -ledc_ecc -lcdrdeflt -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET) $(LIB_INTL) $(LIB_CAP)
XMK_FILE=	Makefile.man Makefile.dfl Makefile.doc


###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

