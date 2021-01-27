#ident @(#)paranoia.mk	1.2 07/06/13 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	paranoia
#CPPOPTS +=	-Ispecincl
#CPPOPTS +=	-DUSE_PG
include		Targets
LIBS=		

XMK_FILE=	Rinterface.mk Rparanoia.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
