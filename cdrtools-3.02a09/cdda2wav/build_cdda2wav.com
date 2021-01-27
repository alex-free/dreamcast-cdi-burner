$ define/nolog scg [-.LIBSCG.SCG]
$ incs = "/INCL=([-.INCLUDE],[-.CDRECORD])"
$! defs = "/debug/noopt "
$ defs = "/nowarn"
$ copy [-.libparanoia]cdda_paranoia.h *.*
$ copy [-.cdrecord]CD_MISC.C *.*
$ copy [-.cdrecord]MODES.C *.*
$ copy [-.cdrecord]SCSI_CDR.C *.*
$ copy lconfig.vms lconfig.h
$ cc/prefix=all'incs''defs' AIFC.C
$ cc/prefix=all'incs''defs' AIFF.C
$ cc/prefix=all'incs''defs' BASE64.C
$ cc/prefix=all'incs''defs' CDDA2WAV.C
$ cc/prefix=all'incs''defs' CD_MISC.C
$ cc/prefix=all'incs''defs' INTERFACE.C
$ cc/prefix=all'incs''defs' IOCTL.C
$ cc/prefix=all'incs''defs' MD5C.C
$ cc/prefix=all'incs''defs' MODES.C
$ cc/prefix=all'incs''defs' RAW.C
$ cc/prefix=all'incs''defs' RESAMPLE.C
$ cc/prefix=all'incs''defs' RINGBUFF.C
$ cc/prefix=all'incs''defs' SCSI_CDR.C
$ cc/prefix=all'incs''defs' SCSI_CMDS.C
$ cc/prefix=all'incs''defs' SEMSHM.C
$ cc/prefix=all'incs''defs' SETUID.C
$ cc/prefix=all'incs''defs' SHA_FUNC.C
$ cc/prefix=all'incs''defs' SNDCONFIG.C
$ cc/prefix=all'incs''defs' SUN.C
$ cc/prefix=all'incs''defs' TOC.C
$ cc/prefix=all'incs''defs' WAV.C
$! defs = "/debug"
$ defs = " "
$ link/noinform/exe=[-.bins]CDDA2WAV.exe 'defs' sys$input/option
cluster=myclu,,,CDDA2WAV.OBJ,AIFC.OBJ,AIFF.OBJ,BASE64.OBJ, -
CD_MISC.OBJ,INTERFACE.OBJ,IOCTL.OBJ, - 
MD5C.OBJ,MODES.OBJ,RAW.OBJ,RESAMPLE.OBJ, -
RINGBUFF.OBJ,SCSI_CMDS.OBJ, -
SEMSHM.OBJ,SETUID.OBJ,SHA_FUNC.OBJ, -
SNDCONFIG.OBJ,SUN.OBJ,TOC.OBJ,WAV.OBJ, -
CD_MISC.obj/selective,MODES.obj/selective,SCSI_CDR.obj/selective, -
[-.libs]libscg.OLB/libr,LIBfile.OLB/libr,libschily.olb/lib,STDIO.OLB/libr
sys$library:decc$shr/share/selective
sys$library:cma$tis_shr/share/selective
$ delete *.obj;*
