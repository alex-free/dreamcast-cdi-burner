$!                                              6 June 2006.  SMS.
$!
$! Cdrtools for VMS accessory procedure.
$!
$! VMSTAR converts symbolic links into text files which say:
$!    *** This file is a link to <path_to_actual_file>
$!
$! This bewilders the VMS builders, which expects to find such files in
$! their original (and only) locations.
$!
$! To solve this problem, this procedure seeks and renames the ".h"
$! files among these from <original_name> to <original_name>_LINK.
$!
$! Note that this procedure acts on the directory tree in which is it
$! situated (normally in the [.vms] subdirectory), not necessarily on
$! any tree associated with the user's default directory.
$!
$!
$ test_string = "*** This file is a link to"
$ test_string_len = f$length( test_string)
$!
$ def_dev_dir = f$environment( "default")
$ in_file_open = 0
$!
$ on error then goto clean_up
$ on control_y then goto clean_up
$!
$ proc_name = f$environment( "procedure")
$ proc_dir = f$parse( proc_name, , , "device")+ -
   f$parse( proc_name, , , "directory")
$!
$ set default 'proc_dir'
$ set default [-]
$ loop_top:
$!
$    in_file_name = f$search( "[...]*.h")
$    if (in_file_name .eqs. "") then goto loop_end
$!
$    open /read /error = clean_up in_file 'in_file_name'
$    in_file_open = 1
$    read /error = clean_up in_file line
$    close in_file
$    in_file_open = 0
$    if (f$extract( 0, test_string_len, line) .eqs. test_string)
$    then
$       new_name = f$parse( in_file_name, , , "name")+ -
         f$parse( in_file_name, , , "type")+ -
         "_LINK"+ -
         f$parse( in_file_name, , , "version")
$       rename /log 'in_file_name' 'new_name'
$    endif
$!
$ goto loop_top
$ loop_end:
$!
$ clean_up:
$!
$ if (in_file_open)
$ then
$    close in_file
$ endif
$!
$ set default 'def_dev_dir'
$!
