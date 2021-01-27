$ defs = "/define=("USE_LARGEFILES","ABORT_DEEP_ISO_ONLY","APPLE_HYB","UDF","DVD_VIDEO","SORTING","USE_LIBSCHILY","USE_SCG","HAVE_DIRENT_H","HAVE_STRCASECMP")
$ incs = "/include=([-.include],[-.libhfs_iso],[-.cdrecord],[])"
$ opts = "/float=ieee/prefix=all"
$ define/nolog scg [-.LIBSCG.scg]
$ copy [-.cdrecord]scsi_cdr.c *.*
$ copy [-.cdrecord]cd_misc.c *.*
$ copy [-.cdrecord]modes.c *.*
$ cc 'defs' 'incs'  'opts' mkisofs.c
$ cc 'defs' 'incs'  'opts' tree.c
$ cc 'defs' 'incs'  'opts' write.c
$ cc 'defs' 'incs'  'opts' hash.c
$ cc 'defs' 'incs'  'opts' rock.c
$ cc 'defs' 'incs'  'opts' udf.c
$ cc 'defs' 'incs'  'opts' multi.c
$ cc 'defs' 'incs'  'opts' joliet.c
$ cc 'defs' 'incs'  'opts' match.c
$ cc 'defs' 'incs'  'opts' name.c
$ cc 'defs' 'incs'  'opts' fnmatch.c
$ cc 'defs' 'incs'  'opts' eltorito.c
$ cc 'defs' 'incs'  'opts' boot.c
$ cc 'defs' 'incs'  'opts' getopt.c
$ cc 'defs' 'incs'  'opts' getopt1.c
$ cc 'defs' 'incs'  'opts' scsi.c
$ cc 'defs' 'incs'  'opts' scsi_cdr.c
$ cc 'defs' 'incs'  'opts' cd_misc.c
$ cc 'defs' 'incs'  'opts' modes.c
$ cc 'defs' 'incs'  'opts' apple.c
$ cc 'defs' 'incs'  'opts' volume.c
$ cc 'defs' 'incs'  'opts' desktop.c
$ cc 'defs' 'incs'  'opts' mac_label.c
$ cc 'defs' 'incs'  'opts' stream.c
$ cc 'defs' 'incs'  'opts' ifo_read.c
$ cc 'defs' 'incs'  'opts' dvd_file.c
$ cc 'defs' 'incs'  'opts' dvd_reader.c
$ cc 'defs' 'incs'  'opts' vms.c
$ libr/crea mkisofs.olb
$ libr/ins mkisofs.olb *.obj
$ link/nosysshr/exe=[-.bins]mkisofs.exe sys$input/option
cluster=myclu,,,mkisofs.obj,mkisofs.olb/libr, -
[-.libs]LIBHFS_Iso.olb/lib, -
LIBUNLS.olb/lib, -
libfile.olb/lib, -
libscg.olb/lib, -
libschily.olb/lib, -
STDIO.OLB/lib
sys$library:decc$shr/share/selective
sys$library:cma$tis_shr/share/selective
$ delete *.obj;*
$ purge/nolog *.olb
