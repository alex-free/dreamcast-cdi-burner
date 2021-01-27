#ident @(#)shlscg.mk	1.3 08/08/01 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	scg
CPPOPTS +=	-I.
CPPOPTS +=	-DUSE_PG
CPPOPTS +=	-DSCHILY_PRINT

include		Targets
LIBS=		$(LIB_VOLMGT) -lschily -lc

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
