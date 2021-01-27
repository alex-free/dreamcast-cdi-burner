#                                               5 December 2006.  SMS.
#
#    CDRTOOLS 2.0 for VMS - MMS (or MMK) Source Description File.
#

# Default target identifier.

TARGET_LIBVMS = 1

# Common source description file.

.INCLUDE [-.VMS]DESCRIP_SRC_CMN.MMS

# C compiler defines.

CDEFS_SPEC =

CDEFS = $(CDEFS_CMN) $(CDEFS_FIND) $(CDEFS_LARGE) $(CDEFS_USR) $(CDEFS_SPEC)

# Other C compiler options.

CFLAGS_INCL = /include = ([])

CFLAGS_SPEC = /prefix_library_entries = all_entries

# Define CFLAGS and LINKFLAGS.

.INCLUDE [-.VMS]DESCRIP_SRC_FLAGS.MMS

# Object library modules.

.INCLUDE DESCRIP_MODS.MMS

