$! build_all.com Version 22.10.2002 for cdrecord 1.11alpha38
$ create/dir [.vms]
$ copy *.c [.vms]
$ copy [-.libscg]*.c [.vms]
$ copy [-.libscg]*.h [.vms]
$ copy [-.libdeflt]*.c [.vms]
$ copy [-.libedc]*.c [.vms]
$ copy [-.libedc]encoder_tables. [.vms]encoder.tables
$ copy [-.libedc]scramble_table. [.vms]scramble.table
$ copy [-.libedc]L2SQ_TABLE. [.vms]L2SQ.table
$ set default [.vms]
$ define scg [--.libscg.scg]
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/def=("VMS") CDRECORD.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include],[--.libedc])/def=("VMS") sector.c
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") CDR_DRV.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") DRV_JVC.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") DRV_MMC.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") DRV_PHILIPS.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") DRV_SONY.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") FIFO.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") ISOSIZE.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") MODES.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSIERRS.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSITRANSP.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSIOPEN.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSIHACK.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSIHELP.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") RDUMMY.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCGSETTARGET.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCGTIMES.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSI_CDR.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSI_SCAN.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") WM_PACKET.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") WM_SESSION.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") WM_TRACK.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") AUDIOSIZE.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") DRV_SIMUL.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") DISKID.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") CD_MISC.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") MISC.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") DEFAULTS.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") DEFAULT.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") cdtext.c
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") crc16.c
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") SUBCHAN.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") GETNUM.C
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include])/define=("VMS") MOVESECT.C
$ crea/dir [.inc]
$ copy [--.inc]*.* [.inc]
$ set default [.inc]
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") ALIGN_TEST.C
$ link ALIGN_TEST
$ delete ALIGN_TEST.obj;*
$ def/user sys$output ALIGN.H
$ r ALIGN_TEST
$ cc/float=ieee/pref=all/obj/incl=([],[--],[---.include])/define=("VMS") AVOFFSET.C
$ link AVOFFSET
$ delete AVOFFSET.obj;*
$ def/user sys$output AVOFFSET.H
$ r AVOFFSET
$ set default [-]
$ crea/dir [.lib]
$ copy [--.lib]*.* [.lib]
$ copy [--.libschily]*.* [.lib]
$ set default [.lib]
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") ASTOI.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") ASTOLL.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") COMERR.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") ERROR.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") FCONV.C
$ cc/float=ieee/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") FILLBYTES.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") FORMAT.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") GETARGS.C
$ cc/float=ieee/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") GETAV0.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") GETERRNO.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") GETFP.C
$ cc/float=ieee/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") MOVEBYTES.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") PRINTF.C
$ cc/float=ieee/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") RAISECOND.C
$ cc/float=ieee/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") SAVEARGS.C
$ cc/float=ieee/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") jsprintf.C
$ cc/float=ieee/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") jssnprintf.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") STREQL.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") SWABBYTES.C
$ cc/float=ieee/pref=all/obj/incl=([--],[---.include])/define=("VMS") SERRMSG.C
$ libr/crea [-]lib.olb
$ libr/ins [-]lib.olb *.obj
$ delete *.obj;*
$ set default [-]
$ create/dir [.stdio]
$ copy [--.libschily.stdio]*.* [.stdio]
$ set default [.stdio]
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") CVMOD.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") DAT.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FCONS.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FGETLINE.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FILEOPEN.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FILEREAD.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FILEWRITE.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FLAG.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FLUSH.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") NIREAD.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FILELUOPEN.C
$ cc/float=ieee/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FILESIZE.C
$ libr/cre [-]stdio.olb
$ libr/ins [-]stdio.olb *.obj
$ delete *.obj;*
$ set default [-]
$ define/user encoder_tables encoder.tables
$ define/user scramble_table scramble.table
$ define/user l2sq_table L2SQ.TABLE
$ cc/float=ieee/pref=all/obj/incl=([-],[--.include],[--.libedc],[.inc])/def=("VMS") edc_ecc.c
$ libr/cre cdr.olb
$ libr/ins cdr.olb *.obj
$ link /nosysshr/exe=cdrecord sys$input/option
  cluster=myclu,,,cdrecord.obj,cdr.olb/lib,lib.olb/lib,stdio.olb/lib
  sys$library:decc$shr/share/selective
  sys$library:cma$tis_shr/share/selective
$ dir *.exe
