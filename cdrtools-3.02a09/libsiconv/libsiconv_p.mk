#ident @(#)libsiconv_p.mk	1.3 08/01/11 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/profiled
SUBINSDIR=	/profiled
INSDIR=		lib
TARGETLIB=	siconv
#CPPOPTS +=	-Istdio
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_ICONV
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
COPTS +=	$(COPTGPROF)

include		Targets
LIBS=

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
