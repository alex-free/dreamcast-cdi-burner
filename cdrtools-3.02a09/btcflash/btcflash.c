/* @(#)btcflash.c	1.18 17/07/15 2004-2017 J. Schilling */
#ifndef lint
static	const char _sccsid[] =
	"@(#)btcflash.c	1.18 17/07/15 2004-2017 J. Schilling";
#endif
/*--------------------------------------------------------------------------*/
/*
 * Firmware flash utility for BTC DRW1008 DVD+/-RW recorder
 * Version 2004/04/29
 * By David Huang <khym@azeotrope.org>
 * This work is dedicated to the public domain
 *
 * This utility may also work with other BTC DVD recorders, such as
 * the DRW1004 and DRW1108, but they have not been tested.
 *
 * USE AT YOUR OWN RISK!
 * btcflash is provided AS IS, with NO WARRANTY, either expressed or implied.
 *
 * Firmware files may be obtained by running BTC's Windows flash
 * utility, then searching in the WINDOWS\TEMP or WINNT\TEMP directory
 * for a *.HEX file. It will probably be in a subdirectory named
 * PAC*.tmp.DIR, and the HEX file will be named Vnnnn.HEX, where nnnn
 * is the firmware version number. You'll also find IDEFLASH.EXE or
 * BTCFLASH.EXE in the same directory.
 *
 * This utility will also accept firmware files in ".BIN" format.
 */

#ifdef	DO_INCLUDE
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/unistd.h>
#endif

#define	FLASHSIZE	0x100000	/* BTC flash is 1MB */

EXPORT	unsigned char 	*loadfirmware __PR((const char *));
LOCAL	int		getbyte		__PR((char **));
LOCAL	unsigned short	calcsum		__PR((unsigned char *));
LOCAL	int		btcmain		__PR((SCSI *scgp, const char *fwfile));


EXPORT unsigned char *
loadfirmware(firmware)
	const char *	firmware;
{
	FILE		*f;
	char		line[80];
	char		*p;
	unsigned char	*fwbuf;
	int		bank;
	int		length;
	int		offset;
	int		type;
	int		hexsum;
	int		i;
	int		b;

	fwbuf = malloc(FLASHSIZE);
	if (!fwbuf) {
		fprintf(stderr, _("Could not allocate memory for firmware\n"));
		return (NULL);
	}

	f = fopen(firmware, "r");
	if (!f) {
		fprintf(stderr, _("%s: Unable to open: "), firmware);
		perror(NULL);
		free(fwbuf);
		return (NULL);
	}

	/*
	 * Get length of file. If it's exactly FLASHSIZE, assume it's a
	 * .bin file. Otherwise, try to read it as a .hex file.
	 */
	fseek(f, 0, SEEK_END);
	if (ftell(f) == FLASHSIZE) {
		rewind(f);
		if (fread(fwbuf, 1, FLASHSIZE, f) != FLASHSIZE) {
			fprintf(stderr, _("%s: Short read\n"), firmware);
			fclose(f);
			free(fwbuf);
			return (NULL);
		}
		fclose(f);
		return (fwbuf);
	}

	rewind(f);
	memset(fwbuf, 0xff, FLASHSIZE);

	bank = 0;
	while (fgets(line, sizeof (line), f)) {
		if (line[0] != ':')
			continue;

		p = line + 1;

		length = getbyte(&p);
		offset = getbyte(&p) << 8 | getbyte(&p);
		type = getbyte(&p);
		if (length < 0 || offset < 0 || type < 0 ||
		    (type != 0 && length != 0)) {
			errmsgno(EX_BAD, _("Malformed line: %.79s\n"), line);
			fclose(f);
			free(fwbuf);
			return (NULL);
		} else if (length == 0) {
			if (strncmp(line, ":00000155AA", 11) == 0) {
				if (++bank >= 16) {
					errmsgno(EX_BAD,
					    _("Firmware file larger than 1MB\n"));
					fclose(f);
					free(fwbuf);
					return (NULL);
				}
				continue;
			} else if (strncmp(line, ":00000001FF", 11) == 0) {
				break;
			} else {
				errmsgno(EX_BAD, _("Malformed line: %.79s\n"), line);
				fclose(f);
				free(fwbuf);
				return (NULL);
			}
		}

		hexsum = (length + (offset >> 8) + (offset & 0xff)) & 0xff;
		for (i = 0; i < length; i++, offset++) {
			b = getbyte(&p);
			hexsum = (hexsum + b) & 0xff;
			if (b < 0) {
				errmsgno(EX_BAD, _("Short line: %.79s\n"), line);
				fclose(f);
				free(fwbuf);
				return (NULL);
			}
			fwbuf[(bank << 16) | offset] = (char)b;
		}
		hexsum = (0x100 - hexsum) & 0xff;
		if (hexsum != getbyte(&p)) {
			errmsgno(EX_BAD, _("Checksum mismatch: %.79s\n"), line);
			fclose(f);
			free(fwbuf);
			return (NULL);
		}
	}

	fclose(f);

	if (bank != 15) {
		errmsgno(EX_BAD, _("Firmware file too small\n"));
		free(fwbuf);
		return (NULL);
	}

	return (fwbuf);
}

LOCAL int
getbyte(pp)
	char	**pp;
{
	int	h;
	int	l;

	h = **pp;
	if (h >= '0' && h <= '9')
		h -= '0';
	else if (h >= 'A' && h <= 'F')
		h -= 'A' - 10;
	else if (h >= 'a' && h <= 'f')
		h -= 'a' - 10;
	else
		return (-1);

	l = *(*pp+1);
	if (l >= '0' && l <= '9')
		l -= '0';
	else if (l >= 'A' && l <= 'F')
		l -= 'A' - 10;
	else if (l >= 'a' && l <= 'f')
		l -= 'a' - 10;
	else
		return (-1);

	*pp += 2;
	return ((h << 4) | l);
}

LOCAL unsigned short
calcsum(fwbuf)
	unsigned char	*fwbuf;
{
	unsigned int	flashsum;
	unsigned int	i;

	for (flashsum = 0, i = 0; i < FLASHSIZE; i++)
		flashsum += fwbuf[i];

	return ((flashsum & 0xffff));
}

LOCAL int
btcmain(scgp, fwfile)
	SCSI		*scgp;
	const char	*fwfile;
{
	char		confirm[5];
	unsigned char	*fwbuf;
	unsigned char	inq[128];
	unsigned char	csbuf[32];
	unsigned short	checksum;
	unsigned int	offset;

	printf(_("BTC DVD+/-RW firmware flash utility release %s %s\n"), "1.18", "17/07/15");
	printf(_("USE AT YOUR OWN RISK!\n\n"));

	if (!(fwbuf = loadfirmware(fwfile)))
		return (1);

	checksum = calcsum(fwbuf);

	printf(_("Loaded firmware from %s\nFirmware checksum is %04X\n"),
	    fwfile, checksum);

	if (inquiry(scgp, (char *)inq, 36) < 0)
		return (1);

	printf(_("Drive is currently:     [%.8s][%.16s][%.4s]\n"),
	    inq+8, inq+16, inq+32);
	printf(_("Firmware appears to be: [%.8s][%.16s][%.4s]\n\n"),
	    fwbuf+0x40bc, fwbuf+0x40c4, fwbuf+0x40d4);

	if (strncmp((char *)inq + 8, (char *)fwbuf + 0x40bc, 24) != 0)
		printf(
		    "**********************************************************\n");
		printf(
		    _("WARNING! THIS FIRMWARE DOES NOT SEEM TO BE FOR THIS DRIVE!\n"));
		printf(
		    "**********************************************************\n");

	printf(_("Type \"YES\" to proceed with flash: "));
	fflush(stdout);
	fgets(confirm, sizeof (confirm), stdin);
	if (strcmp(confirm, "YES\n") != 0) {
		printf(_("\nFlash canceled.\n"));
		return (0);
	}

	printf(_("\nUploading firmware...\n"));

	/* Upload firmware */
	for (offset = 0; offset < FLASHSIZE; offset += 0x1000) {

		if (write_buffer(scgp,	(char *)fwbuf + offset, 0x1000,
					6 /* Download Microcode with Offsets */,
					0 /* Buffer ID 0 */,
					offset + 0x20) < 0) {
			errmsgno(EX_BAD, _("Cannot write microcode\n"));
			return (1);
		}
	}

	/* Upload checksum */
	memset(csbuf, 0, 32);
	csbuf[30] = (checksum >> 8);
	csbuf[31] = (checksum & 0xff);

	if (write_buffer(scgp,	(char *)csbuf, 0x20,
				6 /* Download Microcode with Offsets */,
				0 /* Buffer ID 0 */,
				0) /* Offset 0 */
				< 0) {
		errmsgno(EX_BAD, _("Cannot write microcode checksum\n"));
		return (1);
	}

	printf(_("Flashing drive...\n"));

	/* Firmware uploaded; now flash it! */
	if (write_buffer(scgp,	NULL, 0,
				7 /* Download Microcode with Offsets and Save */,
				0 /* Buffer ID 0 */,
				0) /* Offset 0 */
				< 0) {
		errmsgno(EX_BAD, _("Cannot save microcode\n"));
		return (1);
	}

	sleep(50);	/* Let drive sit for a while before bothering it */

	scgp->silent++;
	wait_unit_ready(scgp, 300);
	scgp->silent--;

	if (inquiry(scgp, (char *)inq, 36) < 0)
		return (1);

	printf(_("Drive is now:           [%.8s][%.16s][%.4s]\n\n"),
	    inq+8, inq+16, inq+32);
	printf(_("Please reboot before using the drive.\n"));

	return (0);
}
