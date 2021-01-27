#!/bin/sh

# exit on all errors
set -e

if [ ! -f "$1" ];then
	echo "Usage: `basename $0` [imagefile.ext] [-o|-i]"
	echo "-o will make the bottom of the image be on the outside of the disk"
	echo "-i will make the bottom of the image be on the inside of the disk"
	echo
	echo "Image must be greyscale"
	echo
	echo "make sure the imagefile has the right x-y format"
	echo "run cdrecord driveropts=tattooinfo -checkdrive"
	echo "to find out"
	exit 1
fi


case "$2" in 
	-i)
		convopts="-flip"
	;;
	-o)
		convopts="-flop"
	;;
	*)
		echo "error: arg2 must be -o or -i"
		exit 1
	;;
esac

echo "Make sure the imagefile has the right size"
echo "run cdrecord dev=a,b,c driveropts=tattooinfo -checkdrive"
echo "to find out"
echo
echo -n "Converting $1 to disktattoo_image format: "

## convert it tp ppm format
## note.. -flop = flip horizontally
convert $convopts "$1" "$1-TMP-$$.ppm"

## We only need the data, which is the last line in a ppm file
## tail works mighty fine for this (insert evil dirty-coder-laughter here)
tail -n+4 "$1-TMP-$$.ppm" > "$1.disktattoo_image"

## clean up like a good boy
rm "$1-TMP-$$.ppm"

## notify user that we're done converting
echo "done"
echo

## stop with stupid code commenting
echo "You may now burn '$1.disktattoo_image' to your cd with the command:"
echo "cdrecord driveropts=tattoofile=\"$1.disktattoo_image\" -checkdrive"
