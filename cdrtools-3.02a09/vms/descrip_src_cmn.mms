#                                               5 December 2006.  SMS.
#
#    CDRTOOLS 2.0 for VMS - MMS (or MMK) Common Source Description File.
#

# This description file is included by other description files.  It is
# not intended to be used alone.  Verify proper inclusion.

.IFDEF INCL_DESCRIP_SRC
.ELSE
$$$$ THIS DESCRIPTION FILE IS NOT INTENDED TO BE USED THIS WAY.
.ENDIF # INCL_DESCRIP_SRC


# Define MMK architecture macros when using MMS.

.IFDEF __MMK__                  # __MMK__
.ELSE                           # __MMK__
ALPHA_X_ALPHA = 1
IA64_X_IA64 = 1
VAX_X_VAX = 1
.IFDEF $(MMS$ARCH_NAME)_X_ALPHA     # $(MMS$ARCH_NAME)_X_ALPHA
__ALPHA__ = 1
.ENDIF                              # $(MMS$ARCH_NAME)_X_ALPHA
.IFDEF $(MMS$ARCH_NAME)_X_IA64      # $(MMS$ARCH_NAME)_X_IA64
__IA64__ = 1
.ENDIF                              # $(MMS$ARCH_NAME)_X_IA64
.IFDEF $(MMS$ARCH_NAME)_X_VAX       # $(MMS$ARCH_NAME)_X_VAX
__VAX__ = 1
.ENDIF                              # $(MMS$ARCH_NAME)_X_VAX
.ENDIF                          # __MMK__ [else]

# Analyze architecture-related and option macros.

.IFDEF __ALPHA__                # __ALPHA__
DESTM = ALPHA
.ELSE                           # __ALPHA__
.IFDEF __IA64__                     # __IA64__
DESTM = IA64
.ELSE                               # __IA64__
.IFDEF __VAX__                          # __VAX__
DESTM = VAX
.ELSE                                   # __VAX__
DESTM = UNK
UNK_DEST = 1
.ENDIF                                  # __VAX__ [else]
.ENDIF                              # __IA64__ [else]
.ENDIF                          # __ALPHA__ [else]

.IFDEF LARGE                    # LARGE
.IFDEF __VAX__                      # __VAX__
DESTL =
.ELSE                               # __VAX__
DESTL = L
.ENDIF                              # __VAX__ [else]
.ELSE                           # LARGE
DESTL =
.ENDIF                          # LARGE [else]

DEST = $(DESTM)$(DESTL)

# Check for option problems.

.IFDEF __VAX__                  # __VAX__
.IFDEF FIND                         # FIND
FIND_VAX = 1
.ENDIF                              # FIND
.IFDEF LARGE                        # LARGE
LARGE_VAX = 1
.ENDIF                              # LARGE
.ENDIF                          # __VAX__

# DBG options.

.IFDEF DBG                      # DBG
CFLAGS_DBG = /debug /nooptimize
LINKFLAGS_DBG = /debug /traceback
.ELSE                           # DBG
CFLAGS_DBG =
LINKFLAGS_DBG = /traceback  ##### SMSd  /notraceback
.ENDIF                          # DBG [else]

# Large-file options.

.IFDEF LARGE                    # LARGE
CDEFS_LARGE = , HAVE_LARGEFILES, USE_LONGLONG, _LARGEFILE
.ELSE                           # LARGE
CDEFS_LARGE =
.ENDIF                          # LARGE [else]

# User-specified options.

.IFDEF CDEFS_USER               # CDEFS_USER
CDEFS_USR = , $(CDEFS_USER)
.ELSE                           # CDEFS_USER
CDEFS_USR =
.ENDIF                          # CDEFS_USER [else]

# Absence of MMSDESCRIPTION_FILE.
.IFDEF MMSDESCRIPTION_FILE      # MMSDESCRIPTION_FILE
.ELSE                           # MMSDESCRIPTION_FILE
NO_MMSDESCRIPTION_FILE = 1
.ENDIF                          # MMSDESCRIPTION_FILE [else]


# Subsidiary directory names.  (Note: DEST must be defined first.)

DIR_CDDA2WAV      = cdda2wav
DIR_CDRECORD      = cdrecord
DIR_DEFLT         = libdeflt
DIR_EDC           = libedc
DIR_FILE          = libfile
DIR_FIND          = libfind
DIR_HFS_ISO       = libhfs_iso
DIR_INC           = inc
DIR_INC_DEST      = [-.$(DIR_INC).$(DEST)]
DIR_INC_DEST_FILE = [-.$(DIR_INC)]$(DEST).DIR;1
DIR_MKISOFS       = mkisofs
DIR_MKISOFS_DIAG  = mkisofs.diag
DIR_PARANOIA      = libparanoia
DIR_READCD        = readcd
DIR_SCG           = libscg
DIR_SCGCHECK      = scgcheck
DIR_SCHILY        = libschily
DIR_UNLS          = libunls
DIR_VMS           = libvms

# Object library names.

LIB_CDDA2WAV      = [-.$(DIR_CDDA2WAV).$(DEST)]LIBCDDA2WAV.OLB
LIB_CDRECORD      = [-.$(DIR_CDRECORD).$(DEST)]LIBCDRECORD.OLB
LIB_DEFLT         = [-.$(DIR_DEFLT).$(DEST)]LIBDEFLT.OLB
LIB_EDC           = [-.$(DIR_EDC).$(DEST)]LIBEDC.OLB
LIB_FILE          = [-.$(DIR_FILE).$(DEST)]LIBFILE.OLB
LIB_FIND          = [-.$(DIR_FIND).$(DEST)]LIBFIND.OLB
LIB_HFS_ISO       = [-.$(DIR_HFS_ISO).$(DEST)]LIBHFS_ISO.OLB
LIB_MKISOFS       = [-.$(DIR_MKISOFS).$(DEST)]LIBMKISOFS.OLB
LIB_MKISOFS_DIAG  = [-.$(DIR_MKISOFS).$(DEST)]LIBMKISOFS_DIAG.OLB
LIB_PARANOIA      = [-.$(DIR_PARANOIA).$(DEST)]LIBPARANOIA.OLB
LIB_READCD        = [-.$(DIR_READCD).$(DEST)]LIBREADCD.OLB
LIB_SCG           = [-.$(DIR_SCG).$(DEST)]LIBSCG.OLB
LIB_SCGCHECK      = [-.$(DIR_SCGCHECK).$(DEST)]LIBSCGCHECK.OLB
LIB_SCHILY        = [-.$(DIR_SCHILY).$(DEST)]LIBSCHILY.OLB
LIB_UNLS          = [-.$(DIR_UNLS).$(DEST)]LIBUNLS.OLB
LIB_VMS           = [-.$(DIR_VMS).$(DEST)]LIBVMS.OLB

# Executable names.

CDDA2WAV_EXE = [-.$(DIR_CDDA2WAV).$(DEST)]CDDA2WAV.EXE
CDRECORD_EXE = [-.$(DIR_CDRECORD).$(DEST)]CDRECORD.EXE
DECC_VER_EXE = [-.$(DIR_VMS).$(DEST)]DECC_VER.EXE
ISODEBUG_EXE = [-.$(DIR_MKISOFS).$(DEST)]ISODEBUG.EXE
ISOINFO_EXE  = [-.$(DIR_MKISOFS).$(DEST)]ISOINFO.EXE
ISOVFY_EXE   = [-.$(DIR_MKISOFS).$(DEST)]ISOVFY.EXE
MKISOFS_EXE  = [-.$(DIR_MKISOFS).$(DEST)]MKISOFS.EXE
READCD_EXE   = [-.$(DIR_READCD).$(DEST)]READCD.EXE
SCGCHECK_EXE = [-.$(DIR_SCGCHECK).$(DEST)]SCGCHECK.EXE

EXES = $(CDDA2WAV_EXE) \
       $(CDRECORD_EXE) \
       $(DECC_VER_EXE) \
       $(ISODEBUG_EXE) \
       $(ISOINFO_EXE) \
       $(ISOVFY_EXE) \
       $(MKISOFS_EXE) \
       $(READCD_EXE) \
       $(SCGCHECK_EXE)


# Find options.

.IFDEF FIND                     # FIND
.IFDEF STD_STAT                     # STD_STAT
CDEFS_FIND = , _USE_STD_STAT, USE_FIND
.ELSE                               # STD_STAT
CDEFS_FIND = , VMS_OLD_STAT, USE_FIND
.ENDIF                              # STD_STAT [else]
LIB_FIND_DEP = $(LIB_FIND)
LIB_FIND_OPTS = $(LIB_FIND) /library,
.ELSE                           # FIND
CDEFS_FIND =
LIB_FIND_DEP =
LIB_FIND_OPTS =
.ENDIF                          # FIND [else]


# Complain if warranted.  Otherwise, show destination directory.
# Make the destination directories, if necessary.
				
.IFDEF UNK_DEST                 # UNK_DEST
.FIRST
	@ write sys$output -
 "   Unknown system architecture."
.IFDEF __MMK__                      # __MMK__
	@ write sys$output -
 "   MMK on IA64?  Try adding ""/MACRO = __IA64__""."
.ELSE                               # __MMK__
	@ write sys$output -
 "   MMS too old?  Try adding ""/MACRO = MMS$ARCH_NAME=ALPHA"","
	@ write sys$output -
 "   or ""/MACRO = MMS$ARCH_NAME=IA64"", or ""/MACRO = MMS$ARCH_NAME=VAX"","
	@ write sys$output -
 "   as appropriate.  (Or try a newer version of MMS.)"
.ENDIF                              # __MMK__ [else]
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                           # UNK_DEST
.IFDEF LARGE_VAX                    # LARGE_VAX
.FIRST
	@ write sys$output -
 "   Macro ""LARGE"" is invalid on VAX."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                               # LARGE_VAX
.IFDEF FIND_VAX                         # FIND_VAX
	@ write sys$output -
 "   Macro ""FIND"" is invalid on VAX."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                   # FIND_VAX

.IFDEF NO_MMSDESCRIPTION_FILE               # NO_MMSDESCRIPTION_FILE
.FIRST
	@ write sys$output -
 "   Macro ""MMSDESCRIPTION_FILE"" is not defined as required."
	@ write sys$output -
 "   MMK, or MMS too old?  Try adding:"
	@ write sys$output -
 "   ""/MACRO = MMSDESCRIPTION_FILE=dev:[dir]description_file"","
	@ write sys$output -
 "   as appropriate, where ""dev:[dir]description_file"" is the full path"
	@ write sys$output -
 "   to the MMS/MMK description file being used (typically ""DESCRIP.MMS"")."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                       # NO_MMSDESCRIPTION_FILE
.FIRST
	@ show time
	@ write sys$output "   Destination: [.$(DEST)]"
	@ write sys$output ""
.IFDEF ALL_DEST_DIRS_NEEDED                 # ALL_DEST_DIRS_NEEDED
	@ ! Create all destination directories first.
	if (f$search( "[-.$(DIR_CDDA2WAV)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_CDDA2WAV).$(DEST)]
	if (f$search( "[-.$(DIR_CDRECORD)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_CDRECORD).$(DEST)]
	if (f$search( "[-.$(DIR_DEFLT)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_DEFLT).$(DEST)]
	if (f$search( "[-.$(DIR_EDC)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_EDC).$(DEST)]
	if (f$search( "[-.$(DIR_FILE)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_FILE).$(DEST)]
	if (f$search( "[-.$(DIR_FIND)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_FIND).$(DEST)]
	if (f$search( "[-.$(DIR_HFS_ISO)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_HFS_ISO).$(DEST)]
	if (f$search( "[-.$(DIR_INC)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_INC).$(DEST)]
	if (f$search( "[-.$(DIR_MKISOFS)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_MKISOFS).$(DEST)]
	if (f$search( "[-.$(DIR_PARANOIA)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_PARANOIA).$(DEST)]
	if (f$search( "[-.$(DIR_READCD)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_READCD).$(DEST)]
	if (f$search( "[-.$(DIR_SCG)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_SCG).$(DEST)]
	if (f$search( "[-.$(DIR_SCGCHECK)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_SCGCHECK).$(DEST)]
	if (f$search( "[-.$(DIR_SCHILY)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_SCHILY).$(DEST)]
	if (f$search( "[-.$(DIR_UNLS)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_UNLS).$(DEST)]
	if (f$search( "[-.$(DIR_VMS)]$(DEST).DIR;1") .eqs. "") then -
	 create /directory [-.$(DIR_VMS).$(DEST)]
.ELSE                                           # ALL_DEST_DIRS_NEEDED
	@ ! Create this specific destination directory first.
	if (f$search( "$(DEST).DIR;1") .eqs. "") then -
	 create /directory [.$(DEST)]
.ENDIF                                          # ALL_DEST_DIRS_NEEDED [else]
	@ ! Define SCHILY and SCG logical names for "#include <schily/X.h>"
	@ ! and "#include <scg/X.h>" directives.
	def_dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-]
	schily_dev_dir = f$environment( "DEFAULT")- "]"+ ".include.schily]"
	if (f$trnlnm( "schily", "LNM$PROCESS_TABLE") .eqs. "") then -
	 define schily 'schily_dev_dir'
	scg_dev_dir = f$environment( "DEFAULT")- "]"+ ".libscg.scg]"
	if (f$trnlnm( "scg", "LNM$PROCESS_TABLE") .eqs. "") then -
	 define scg 'scg_dev_dir'
	set default 'def_dev_dir'
	@ ! Define "backport" C RTL logical names as needed, that is,
	@ ! VMS version < 7.0, and logical name not already defined.
	v_v = f$getsyi( "VERSION")
	v_v = f$extract( 1, (f$locate( ".", v_v)- 1), v_v)
	if ((v_v .lt. 7) .and. (f$trnlnm( "DECC$CRTLMAP") .eqs. "")) then -
	 define DECC$CRTLMAP SYS$LIBRARY:DECC$CRTL.EXE
	if ((v_v .lt. 7) .and. (f$trnlnm( "LNK$LIBRARY") .eqs. "")) then -
	 define LNK$LIBRARY SYS$LIBRARY:DECC$CRTL.OLB
.IFDEF TARGET_LIBEDC                            # TARGET_LIBEDC
	@ ! Define logical names for dotless file names.  Sigh.
	define edc_code_tables edc_code_tables.
	define encoder_tables encoder_tables.
	define l2sq_table l2sq_table.
	define scramble_table scramble_table.
.ENDIF                                          # TARGET_LIBEDC
.ENDIF                                      # NO_MMSDESCRIPTION_FILE [else]
.ENDIF                                  # FIND_VAX [else]
.ENDIF                              # LARGE_VAX [else]
.ENDIF                          # UNK_DEST [else]


# Specific DESCRIP_SRC.MMS defines:
#    CDEFS
#    CFLAGS_SPEC
#    CFLAGS_INCL

CDEFS_CMN = VMS, USE_STATIC_CONF

# Architecture-specific CC and LINK flags.

.IFDEF __VAX__                  # __VAX__
CFLAGS_ARCH = /decc
.ELSE                           # __VAX__
CFLAGS_ARCH = /float = ieee_float
.ENDIF                          # __VAX__ [else]

LFLAGS_ARCH =

# LIST options.

.IFDEF LIST                     # LIST
# Note: Before DEC C V6.0, "/show = [no]messages" will cause trouble.
CFLAGS_LIST = /list = $*.LIS /show = (all, nomessages)
LINKFLAGS_LIST = /map = $*.MAP /cross_reference /full
.ELSE                           # LIST
CFLAGS_LIST =
LINKFLAGS_LIST =
.ENDIF                          # LIST [else]

