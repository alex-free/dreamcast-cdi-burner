#ident "@(#)repair.mk	1.1 06/05/15 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		edcrepair
#CPPOPTS +=	-DOLD_LIBEDC
CFILES=		repair.c
HFILES=		
#LIBS=		-lschily $(LIB_SOCKET)
#LIBS=		-loedc -lschily
LIBS=		-ledc_ecc_dec -ledc_ecc -lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
