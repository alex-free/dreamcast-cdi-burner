$ defs = "/define=("USE_LARGEFILES","ABORT_DEEP_ISO_ONLY","APPLE_HYB","UDF","DVD_VIDEO","SORTING","USE_LIBSCHILY","USE_SCG","HAVE_DIRENT_H","HAVE_STRCASECMP")
$ incs = "/include=([-.include],[-.libhfs_iso],[-.cdrecord],[])"
$ opts = "/float=ieee/prefix=all"
$ define/nolog scg [-.LIBSCG.scg]
$ cc 'defs' 'incs'  'opts' BLOCK.C
$ cc 'defs' 'incs'  'opts' BTREE.C
$ cc 'defs' 'incs'  'opts' DATA.C
$ cc 'defs' 'incs'  'opts' FILE.C           
$ cc 'defs' 'incs'  'opts' GDATA.C
$ cc 'defs' 'incs'  'opts' HFS.C
$ cc 'defs' 'incs'  'opts' LOW.C
$ cc 'defs' 'incs'  'opts' NODE.C           
$ cc 'defs' 'incs'  'opts' RECORD.C
$ cc 'defs' 'incs'  'opts' VOLUME.C
$ libr/cre [-.libs]LIBHFS_ISO.olb
$ libr/ins [-.libs]LIBHFS_ISO.olb *.obj         
$ delete *.obj;*
$ purge/nolog [-.libs]*.olb
