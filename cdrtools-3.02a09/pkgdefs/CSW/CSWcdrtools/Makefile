#ident @(#)Makefile	1.2 09/04/04 
###########################################################################
# Sample makefile for creating SVr4 packages
###########################################################################
SRCROOT=	../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

include		$(SRCROOT)/$(RULESDIR)/rules.csw
###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.pkg
###########################################################################

VERSION :sh=	../../../proto/`../../../conf/oarch.sh`/opt/csw/bin/cdrecord -version | awk '{ print $2}'
