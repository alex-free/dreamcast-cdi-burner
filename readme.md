# Dreamcast CDI Burner (DCDIB) : Burn ALL Sega Dreamcast .CDI Files On Linux With Open Source Software

_by Alex Free_

 DCDIB is an open source toolkit for burning Sega Dreamcast `.cdi` files (used for self-booting homebrew programs and backups) to CD-R discs on Linux! CDI is the proprietary format of the Windows only software DiscJuggler. CDI is a very popular format for Sega Dreamcast software distribution. The CD-Rs you burn with DCDIB will _just work_ and boot like authentic Sega Dreamcast GD-ROM discs on revision 0 and revision 1 Sega Dreamcast consoles. **Revision 2 Sega Dreamcast consoles may or may not work with self-booting CD-Rs** due to the MIL-CD exploit being removed from the BIOS chip found on at least some revision 2 consoles. To find out what revision your Sega Dreamcast console is, flip the entire console upside-down and look for the number 0, 1, or 2 in a circle on the model sticker as displayed below:

![dreamcast rev 1 console](images/dreamcast-rev-1.jpg)

DCDIB supports burning **all 4 Sega Dreamcast CDI file format types**:

*   Data ISO (Session 1) + Data ISO (Session 2)
*   WAV/audio track (Session 1) + Data ISO (Session 2)
*   Multiple CDDA WAVs/audio tracks (Session 1) + Data ISO (Session 2)
*   Multiple CDDA WAVs/audio tracks in an ISO (Session 1) + Data ISO (Session 2)

## Links

*   [GitHub](https://github.com/alex-free/dreamcast-cdi-burner)
*   [Homepage](https://alex-free.github.io/dcdib)
*   [Dreamcast-Talk Thread](https://www.dreamcast-talk.com/forum/viewtopic.php?f=2&t=13974)

## Table Of Contents

*   [Downloads](#downloads)
*   [CD-R Media](#cd-r-media)
*   [Usage](#usage)
*   [Building From Source](build.md)
*   [License](#license)

## Downloads

### v1.0.9 (3/28/2024)

*   [DCDIB v1.0.9](https://github.com/alex-free/dreamcast-cdi-burner/releases/download/v1.0.9/dcdib-v1.0.9-x86_64.zip) _For x86\_64 Linux_.

Changes:

*   Added compressed archive support, you can give Dreamcast CDI Burner a supported archive file (7z/xz/cab/zip/gzip/bzip2/tar) containing a CDI file and it will find and burn the CDI file.

*  Updated [PLED](https://alex-free.github.io/pled) to v1.0.4, fixing building from source.

*   `dcdib` and `build` script optimizations/cleanup.

*   `build` script now installs build dependencies if the `apt` package manager is detected on your system.

[About previous versions](changelog.md).

## CD-R Media

It is recommend to only use high-quality CD-R media. I recommend [CMC Pro 80 Minute CD-Rs](https://alex-free.github.io/psx-cdr/#cmc-pro-powered-by-taiyo-yuden-technology-80-minute) with a [Good CD-R Burner](https://alex-free.github.io/psx-cdr/#recommended-cd-r-burners). While some CDI files can fit on lower capacity CD-Rs (i.e. 74 minutes), most CDI files target 80 minute CD-Rs. 

CDI files can however target CD-R capacities larger then 80 minutes, such as 99 or 100 minute CD-Rs. Not all CD-R 99 or 100 minute CD-Rs disc are compatible with the Dreamcast, and some burners don't even support writing that much data to a disc as it is way out of Red Book spec. Some specific brands/types of 99 and 100 minute CD-Rs DO however work, though I don't have any tests done myself yet.

You should [experiment](https://alex-free.github.io/psx-cdr/#cd-r-burn-speed) with different burn speeds if you encounter disc read issues as it can affect how well the Dreamcast can read the CD-R as well.

If you are still having issues booting even high quality CD-Rs on the Sega Dreamcast, consider wiping with a clean microfiber cloth from the inner ring to the outer edge of the CD-R in all directions and then trying to boot the disc again.

# Usage

1) Download and unzip the latest release.

2) After inserting a high-quality CD-R disc in your CD-R burner, **ignore any prompts about the new blank CD-R** that your OS may display such as:

![Fedora new CD prompt](images/fedora-new-cd.png)

3) **Using sudo execute the `dcdib` script in the extracted release directory with 1 or 2 arguments**. Root privileges are required to ensure that buffer under-runs do not occur during burning which would result in a CD-R coaster. Root privileges also ensure that `cdrecord` can access your CD burner hardware successfully to burn the CD-R.

The first argument is the filepath to either a CDI file OR a supported compressed archive (7z/xz/cab/zip/gzip/bzip2/tar) containing a CDI file.

![DCDIB usage 1](images/dcdib-usage-1.png)

![DCDIB usage 2](images/dcdib-usage-2.png)

![DCDIB usage 3](images/dcdib-usage-3.png)

The second argument is completely optional, it allows you to change the CD burner's device devname to a custom one. DCDIB already provides a default CD burner device devname to use which should work on most if not all Linux distributions (`/dev/sr0`). However if you find this to not work on your setup you can provide your own in the optional second argument to DCDIB. You will know if you need to specify this argument because something like below will be displayed:

![DCDIB devname](images/dcdib-devname.png)

## License

DCDIB itself is released into the public domain, see the file `dcdib.md` in the `licenses` directory.

DCDIB makes use of the following programs listed below, which have their own licenses/terms:

*   [PortableLinuxExecutableDirectory](https://alex-free.github.io/pled) (Public Domain, see the file `licenses/pled.txt`).

*   [CDIRip](https://github.com/jozip/cdirip) (GNU GPL v2, see the file `licenses/cdirip.txt`).

*   [CDRRecord (from CDRTools)](https://Distrotech/cdrtools) (CDDL v1.0 AND GPL v2, see the files `licenses/cdrecord-cddl.txt` and `licenses/cdrecord-gpl2.txt`).

*   [P7zip-zstd](https://github.com/p7zip-project/p7zip) (GNU LGPL with unRAR license restriction, BSD-3 Clause, and Public Domain), see `licenses/p7zip.txt`.