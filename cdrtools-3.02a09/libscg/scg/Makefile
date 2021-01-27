#ident @(#)Makefile	1.1 08/07/07 
###########################################################################
# Sample makefile for installing non-localized auxiliary files
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		include/scg
TARGET=		scgcmd.h
XMK_FILE=	\
		aspi-dos.mk aspi-win32.mk \
		scgcmd.mk scgio.mk scgops.mk \
		scsicdb.mk scsidefs.mk scsireg.mk scsisense.mk scsitransp.mk \
		spti-wnt.mk \
		srb_os2.mk


###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.aux
###########################################################################
