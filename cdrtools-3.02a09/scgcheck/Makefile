#ident @(#)Makefile	1.15 13/04/21 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		scgcheck
CPPOPTS +=	-I../libscg
CPPOPTS +=	-I../cdrecord
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_cdrtools\"

CFILES=		scgcheck.c sense.c dmaresid.c \
		scsi_scan.c scsi_cdr.c cd_misc.c

HFILES=		scgcheck.h version.h
#LIBS=		-lschily $(LIB_SOCKET)
#
# LIB_CAP is needed for Linux capability support in librscg.
#
LIBS=		-lscgcmd -lrscg -lscg $(LIB_VOLMGT) -lschily $(SCSILIB) $(LIB_SOCKET) $(LIB_INTL) $(LIB_CAP)
XMK_FILE=	Makefile.man

scsi_scan.c scsi_cdr.c cd_misc.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): scsi_scan.c scsi_cdr.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

