#ident @(#)speed.mk	1.2 04/01/06 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		edcspeed
CPPOPTS +=	-DOLD_LIBEDC
CFILES=		edcspeed.c
HFILES=		
#LIBS=		-lschily $(LIB_SOCKET)
#LIBS=		-loedc -lschily
LIBS=		-ledc_ecc -lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
