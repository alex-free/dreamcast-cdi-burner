#ident @(#)align_test.mk	1.2 06/10/31 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		include/schily/$(OARCH)
TARGET=		align.h
TARGETC=	align_test
CPPOPTS +=	-D__OPRINTF__
CFILES=		align_test.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.inc
###########################################################################
