#!/bin/sh

#
# Note: The adapter shell var must updated in order to reflect your actual hardware.
#	The current value "friq" is the right value for the FreeCom PP adapter.
#	See paride documentation for the list of other drivers.
#
adapter=friq

case "$1" in

'start')
	echo "Starting Parallel Port IDE/ATAPI"
	modprobe paride
	modprobe "$adapter"
#	modprobe pg
#	modprobe pg verbose=2 drive0=0x378,0,0,-1,-1,0
	modprobe pg verbose=0 drive0=0x378,0,0,-1,-1,0
	;;

'stop')
	echo "Stopping Parallel Port IDE/ATAPI"
	rmmod pg
	rmmod "$adapter"
	rmmod paride
	;;

*)
	echo "Usage: rc.pp start|stop"
esac

exit

###############################################################################
When you load pg, you need to specify some parameters like:

        drive0=0x378,0,0,4,0,0

The parameters are:

 <prt>,<pro>,<uni>,<mod>,<slv>,<dly>

                        Where,

                <prt>   is the base of the parallel port address for
                        the corresponding drive.  (required)

                <pro>   is the protocol number for the adapter that
                        supports this drive.  These numbers are
                        logged by 'paride' when the protocol modules
                        are initialised.  (0 if not given)

                <uni>   for those adapters that support chained
                        devices, this is the unit selector for the
                        chain of devices on the given port.  It should
                        be zero for devices that don't support chaining.
                        (0 if not given)

                <mod>   this can be -1 to choose the best mode, or one
                        of the mode numbers supported by the adapter.
                        (-1 if not given)

                <slv>   ATAPI devices can be jumpered to master or slave.
                        Set this to 0 to choose the master drive, 1 to
                        choose the slave, -1 (the default) to choose the
                        first drive found.

                <dly>   some parallel ports require the driver to 
                        go more slowly.  -1 sets a default value that
                        should work with the chosen protocol.  Otherwise,
                        set this to a small integer, the larger it is
                        the slower the port i/o.  In some cases, setting
                        this to zero will speed up the device. (default -1)

EPP mode is best.  Your BIOS may not give you that option, unfortunately.
What options does it support ?
