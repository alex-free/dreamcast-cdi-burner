#ident @(#)shlfind.mk	1.5 17/07/06 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	find
#CPPOPTS +=	-Istdio
#CPPOPTS +=	-DUSE_SCANSTACK

#CPPOPTS +=	-D__FIND__
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_ACL
CPPOPTS +=	-DUSE_XATTR
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DUSE_DGETTEXT			# _() -> dgettext()
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_FIND\"
CPPOPTS +=	-DSCHILY_PRINT

include		Targets
LIBS=		-lschily $(LIB_ACL_TEST) $(LIB_INTL) -lc

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
