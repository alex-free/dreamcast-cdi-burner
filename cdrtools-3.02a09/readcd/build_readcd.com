$ defs = "/define=("USE_LARGEFILES","ABORT_DEEP_ISO_ONLY","APPLE_HYB","UDF","DVD_VIDEO","SORTING","USE_LIBSCHILY","USE_SCG","HAVE_DIRENT_H","HAVE_STRCASECMP")
$ incs = "/include=([-.include],[-.libhfs_iso],[-.cdrecord],[])"
$ opts = "/float=ieee/prefix=all/nowarn"
$ define/nolog scg [-.LIBSCG.scg]
$ copy [-.cdrecord]scsi_cdr.c *.*
$ copy [-.cdrecord]cd_misc.c *.*
$ copy [-.cdrecord]modes.c *.*
$ copy [-.cdrecord]SCSI_MMC.C *.*
$ copy [-.cdrecord]DEFAULTS.C *.*
$ copy [-.cdrecord]GETNUM.C *.*
$ copy [-.cdrecord]misc.C *.*
$ copy [-.cdrecord]MOVESECT.* *.*
$ copy [-.libdeflt]default.c *.*
$ cc 'defs' 'incs'  'opts' cd_misc.c
$ cc 'defs' 'incs'  'opts' DEFAULTS.C
$ cc 'defs' 'incs'  'opts' GETNUM.C
$ cc 'defs' 'incs'  'opts' IO.C
$ cc 'defs' 'incs'  'opts' MISC.C
$ cc 'defs' 'incs'  'opts' MODES.C
$ cc 'defs' 'incs'  'opts' MOVESECT.C
$ cc 'defs' 'incs'  'opts' READCD.C
$ cc 'defs' 'incs'  'opts' SCSI_CDR.C
$ cc 'defs' 'incs'  'opts' SCSI_MMC.C
$ cc 'defs' 'incs'  'opts' default.c
$ libr/crea readcd.olb
$ libr/ins readcd.olb *.obj
$ define/user sys$output nl:
$ link/exe=[-.bins]readcd.exe sys$input/option
cluster=myclu,,,readcd.obj,readcd.olb/libr, -
[-.libs]LIBHFS_Iso.olb/lib, -
LIBUNLS.olb/lib, -
libfile.olb/lib, -
libscg.olb/lib, -
STDIO.OLB/lib, -
libschily.olb/lib
sys$library:decc$shr/share/selective
sys$library:cma$tis_shr/share/selective
$ delete *.obj;*
$ purge/nolog *.olb
