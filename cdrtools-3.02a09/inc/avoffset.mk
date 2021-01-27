#ident @(#)avoffset.mk	1.2 06/10/31 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		include/schily/$(OARCH)
TARGET=		avoffset.h
TARGETC=	avoffset
CPPOPTS +=	-DUSE_SCANSTACK
CPPOPTS +=	-D__OPRINTF__
CFILES=		avoffset.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.inc
###########################################################################
