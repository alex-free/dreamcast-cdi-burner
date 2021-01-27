$ defs = "/define=("USE_LARGEFILES","ABORT_DEEP_ISO_ONLY","APPLE_HYB","UDF","DVD_VIDEO","SORTING","USE_LIBSCHILY","USE_SCG","HAVE_DIRENT_H","HAVE_STRCASECMP")
$ incs = "/include=([-.include],[-.libhfs_iso],[-.cdrecord],[])"
$ opts = "/float=ieee/prefix=all"
$ define/nolog scg [-.LIBSCG.scg]
$ cc 'defs' 'incs'  'opts' APPRENTICE.C
$ cc 'defs' 'incs'  'opts' FILE.C
$ cc 'defs' 'incs'  'opts' SOFTMAGIC.C
$ libr/cre [-.libs]LIBfile.olb
$ libr/ins [-.libs]LIBfile.olb *.obj         
$ delete *.obj;*
$ purge/nolog [-.libs]*.olb
