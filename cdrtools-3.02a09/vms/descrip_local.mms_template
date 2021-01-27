#                                               7 June 2006.  SMS.
#
#    CDRTOOLS 2.0 for VMS - MMS (or MMK) Local Options File.
#
# Usage:
#
#    Before building cdrtools, edit this file, and then copy it to
#    [.VMS]DESCRIP_LOCAL.MMS.
#
########################################################################

# This description file is included by other description files.  It is
# not intended to be used alone.  Verify proper inclusion.

.IFDEF INCL_DESCRIP_SRC
.ELSE
$$$$ THIS DESCRIPTION FILE IS NOT INTENDED TO BE USED THIS WAY.
.ENDIF # INCL_DESCRIP_SRC

########################################################################

#    CDDA2WAV local options.

# The following options are enabled by default.  Comment out ("#") the
# appropriate macro definition to disable a particular feature.
#
# For example, to disable MD5 signatures, change the line:
#
#       WANT_MD5 = 1
# to:
#       # WANT_MD5 = 1
#

# MD5 signatures in info files.
#
WANT_MD5 = 1

# Disc description file for the cdindex server.
#
WANT_CDINDEX_SUPPORT = 1

# Network access to a CDDB server.
#
WANT_CDDB_SUPPORT = 1

# The following CDDB server parameters may remain defined, even if
# WANT_CDDB_SUPPORT is commented out.

# Default CDDB server IP name or address.
# (Use no quotation marks here.)
#
CDDB_SERVERHOST = freedb.freedb.org

# Default CDDB IP port munber.
#
CDDB_SERVERPORT = 8880

########################################################################

# End of Local Options file.

