DCDIB

Dreamcast CDI Burner
====================

by Alex Free

Dreamcast CDI Burner is a collection of software (CDIrip and CDRtools, along with my own cdib bash script) that can burn a Dreamcast CDI file to a blank CD-ROM using your computer's CD burner. The resulting CD-ROM is bootable on a Sega Dreamcast console, making use of the MIL-CD exploit. This exploit does not require any console modifications, as long as it is exploitable. CDI is the proprietary format of the Windows only software DiscJuggler. It is a very popular format for Sega Dreamcast software distribution.

Revision 0 and 1 consoles can all be exploited. If you have a revision 2 console it may not be able to use the MIL-CD exploit to boot burned CD-ROMs. To find out the revision of a console, flip it upside down and look for the number 0, 1, or 2 circled on the bottom label.

Dreamcast CDI Burner supports all 4 Sega Dreamcast CDI File format types.

*   Data ISO+data ISO
*   WAV+data ISO
*   Multiple CDDA WAVs+data ISO
*   Multiple CDDA WAVs in an ISO+data ISO

[GitHub](https://github.com/alex-free/dreamcast-cdi-burner) || [HomePage](https://alex-free.github.io/dcdib) || [Macintosh Garden](https://macintoshgarden.org/apps/dreamcast-cdi-burner-mac-os-x)

Table Of Contents
-----------------

*   [Downloads](#downloads)
*   [Installation](#install)
*   [Install From Source](#install_source)
*   [How To Uninstall](#uninstall)
*   [Important Info For Systems With Multiple CD Burners](#optical_drive_note)
*   [Important Info About CD-ROMs](#cdr_note)
*   [Usage](#usage)
*   [Example Burns (Images)](#images)
*   [Changlog](#changelog)
*   [Bundled Software & Licensing](#bundled_software)

Downloads
---------

### Version 1.0.7 - (8/19/2021) - [Changes In This Version](#changelog)

[Mac OS X 10.3.9-10.6.x (PowerPC)](https://github.com/alex-free/dreamcast-cdi-burner/releases/download/1.0.7/dreamcast_cdi_burner_1.0.7_mac_powerpc.zip)

[Mac OS X 10.6.x-10.14.x (Intel)](https://github.com/alex-free/dreamcast-cdi-burner/releases/download/1.0.7/dreamcast_cdi_burner_1.0.7_mac_i386.zip)

[Source Code](https://github.com/alex-free/dreamcast-cdi-burner/archive/1.0.7.zip)

View [all releases](https://github.com/alex-free/dreamcast-cdi-burner/releases/).

Installation
------------

### Mac OS X

Download the PowerPC or Intel release, and unzip it. Copy the `DCDIB.app` to `/Applications/DCDIB.app`. If `DCDIB.app` is ran anywhere else or named differently you will be prompted to copy it to `/Applications/DCDIB.app` before you can continue.

### Linux

A pre-built release is not yet available. Please install Dreamcast CDI
Burner from [source](#install_source).

Note that the heart of this program is just a bash shell script,
[cdib](raw/master/cdib) which calls two other programs: cdrecorder and cdirip. 
By default, those programs are compiled and installed with this package.

If you already have cdrecorder installed (very likely) and
[cdirip](https://github.com/jozip/cdirip) (less likely), then
all you need to install is the shell script:

```bash
 wget https://github.com/alex-free/dreamcast-cdi-burner/raw/master/cdib		# Download the cdib script.
 chmod +x cdib				# Make it executable.
 mv cdib /usr/local/bin/		# Put it in your PATH.
```


Install From Source
-------------------

### Mac OS X Requirements

Mac OS X 10.3.9 or newer is required to build for Mac OS X. PowerPC and Intel are supported. To target Mac OS X 10.3.9-10.6.8 PowerPC build on Mac OS X 10.3.9. To target Mac OS X 10.4-10.14 Intel, build on Mac OS X 10.4. Xcode and or Xcode Command Line Tools (depending on your OS version) is required.

### Linux Requirements

Compiling for Linux requires gcc and GNUmake installed.

### Building

Compiling Dreamcast CDI Burner is done by executing one command found in the [source code](https://github.com/alex-free/dreamcast-cdi-burner/archive/1.0.6.zip) download, the `build-all` script with sudo privilages or as root. The `build-all` script does not need to be executed from the same directory to work. It will compile and install Dreamcast CDI Burner, as well as create a release in the `build` directory.

Important Info About CD-ROMs
----------------------------

Some people swear by [Verbatim](https://www.verbatim.com/home?con=696) CD-ROMs, however this has not been my experience. I've been able to grab any CD-ROM brand (such as Maxell) and they work perfectly on my Model 1 Sega Dreamcast.

Also, I'd just like to say here that playing burned games and software is _no different then playing burned audio CDs in your Dreamcast as far as the laser is concerned_.

Usage
-----

### Mac

Insert a blank CD-ROM into the internal burner of your Mac. **When Finder asks what to do with this disc, click `ignore`**.

![Ignore blank CD-ROM on Mac](https://alex-free.github.io/dcdib/blank_cd_finder.png)

Double click `/Applications/DCDIB.app`. A window will open allowing you to select a CDI file. After selecting a file you may be prompted for your account password. This is required to give CDRtools the privileges required for successful burning. After entering your password if prompted, simply wait for the burn to complete, you may monitor the progress made in the Terminal window.

![Selecting a .CDI file in Finder on Mac](https://alex-free.github.io/dcdib/dcdib_mac_finder_select_cdi.png)

Note that Dreamcast CDI Burner will eject all removable storage as a side effect of enforcing a successful burn on Mac OS X.

### Linux

Insert a blank CD-ROM into the internal burner of your computer. Don't let any software/your OS format or otherwise 'touch' the CD.

![Ignore blank CD-ROM on Linux](https://alex-free.github.io/dcdib/blank_cd_fedora.png)

In the terminal execute `sudo cdib /path/to/your.cdi` (replace `/path/to/your.cdi` with your actual desired .CDI file). Then, just wait for your burn to complete.

Example Burns (Images)
----------------------

![DOA2 burn on Mac image 1](https://alex-free.github.io/dcdib/doa2_burn_on_mac_1.png)

![DOA2 burn on Mac image 2](https://alex-free.github.io/dcdib/doa2_burn_on_mac_2.png)

![DOA2 burn on Mac image 3](https://alex-free.github.io/dcdib/doa2_burn_on_mac_3.png)

![DOA2 burn on Mac image 4](https://alex-free.github.io/dcdib/doa2_burn_on_mac_4.png)

![HT burn on Linux image 1](https://alex-free.github.io/dcdib/ht_burn_on_linux_1.png)

![HT burn on Linux image 2](https://alex-free.github.io/dcdib/ht_burn_on_linux_2.png)

![HT burn on Linux image 3](https://alex-free.github.io/dcdib/ht_burn_on_linux_3.png)

![HT burn on Linux image 4](https://alex-free.github.io/dcdib/ht_burn_on_linux_4.png)

![HT burn on Linux image 5](https://alex-free.github.io/dcdib/ht_burn_on_linux_5.png)

![HT burn on Linux image 6](https://alex-free.github.io/dcdib/ht_burn_on_linux_6.png)

![HT burn on Linux image 7](https://alex-free.github.io/dcdib/ht_burn_on_linux_7.png)

![HT burn on Linux image 8](https://alex-free.github.io/dcdib/ht_burn_on_linux_8.png)

![HT burn on Linux image 9](https://alex-free.github.io/dcdib/ht_burn_on_linux_9.png)

![RECVCD1 burn on Linux image 1](https://alex-free.github.io/dcdib/recv_cd1_burn_on_linux_1.png)

![RECVCD1 burn on Linux image 2](https://alex-free.github.io/dcdib/recv_cd1_burn_on_linux_2.png)

![RECVCD1 burn on Linux image 3](https://alex-free.github.io/dcdib/recv_cd1_burn_on_linux_3.png)

![RECVCD1 burn on Linux image 4](https://alex-free.github.io/dcdib/recv_cd1_burn_on_linux_4.png)

![RECVCD1 burn on Linux image 5](https://alex-free.github.io/dcdib/recv_cd1_burn_on_linux_5.png)

![RECVCD1 burn on Linux image 6](https://alex-free.github.io/dcdib/recv_cd1_burn_on_linux_6.png)

![RECVCD2 burn on Linux image 1](https://alex-free.github.io/dcdib/recv_cd2_burn_on_linux_1.png)

![RECVCD2 burn on Linux image 2](https://alex-free.github.io/dcdib/recv_cd2_burn_on_linux_2.png)

![RECVCD2 burn on Linux image 3](https://alex-free.github.io/dcdib/recv_cd2_burn_on_linux_3.png)

![RECVCD2 burn on Linux image 4](https://alex-free.github.io/dcdib/recv_cd2_burn_on_linux_4.png)

![RECVCD2 burn on Linux image 5](https://alex-free.github.io/dcdib/recv_cd2_burn_on_linux_5.png)

Changelog
---------

### Version 1.0.7 (8/19/2021)

*   Removed build select, 10.3.9 is now required to build targeting Panther PowerPC.
*   Added Intel Mac support, and Mac OS 10.11+ compatibility by unloading diskarbitrationd and not loading it until burning is complete.
*   Always specify IODVDServices.

### Version 1.0.6 (2/1/2021)

*   Fixed the CDRtools compatibility regression on Mac OS X 10.3.9
*   The `build-all` script should now work on Mac OS X 10.7 and above. (untested as I do not have an Intell Mac).
*   The `build-all` script should now compile native Intel builds when ran on Intel Macs (untested as I do not have an Intel Mac).
*   Switched to uname based OS detection in the cdib script.
*   On Linux the CD-ROM will be ejected after a successful burn (the Mac OS X version already did this).

### Version 1.0.5 (1/27/2021)

*   Linux support.
*   CDIrip 0.6.3 is now used for Linux builds.
*   Mac OS X build now uses Build Select v1.0.1 (included in source).
*   The build script can now be executed from any directory, not just the current one.

How To Uninstall
----------------

### On Mac OS X

Delete `/Applications/DCDIB.app`.

### On Linux

Execute the `uninstall` script (found in each linux release, if you built from source a release was created in the `build` directory) with sudo privilages or as root. You can type `sudo` in the Terminal application, add a space, and then simply drag the `uninstall` script as well, it does not need to be executed from the same directory.

Bundled Software & Licensing
----------------------------

*   [CDRtools v3.02a09](http://cdrtools.sourceforge.net/private/cdrecord.html) (CDDL, BSDL 2 clause, LGPL version 2.1, GPL version 2, Public Domain)
*   CDIrip ([0.6.3](https://sourceforge.net/projects/cdimagetools/files/CDIRip) - GNU GPL version 2), ([0.6.2](https://web.archive.org/web/20060213170824/http://cdirip.cjb.net) - as is, no license, source code available).
*   [Cdib](https://github.com/alex-free/dreamcast-cdi-burner/blob/master/cdib) (my own script) along with everything else that is not the above software, but is included in the source and releases, is released into the Public Domain (unlicense).

For more information, see the 'licenses' directory in each release.