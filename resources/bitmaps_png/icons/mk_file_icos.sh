#!/bin/bash
#
# make icons using Inkscape
# inkscape (1.0.2 minimum) and icotool are necessary


# create output directories
rm -r tmp
mkdir tmp
mkdir tmp/16 tmp/24 tmp/32 tmp/48 tmp/64 tmp/256 tmp/ico
echo "all directories created."
cd ../../linux/icons/hicolor/scalable/mimetypes

ICON_FILES="worksheet
            symbol
            schematic
            project
            pcb
            footprint"

# convert .svg files into .png files
for fl in $ICON_FILES
do
    NAME=$fl
    SOURCE_NAME="application-x-kicad-${fl}.svg"
	inkscape -o ../../../../../bitmaps_png/icons/tmp/16/$NAME.png -w 16 -h 16 --export-area-snap $SOURCE_NAME
	inkscape -o ../../../../../bitmaps_png/icons/tmp/24/$NAME.png -w 24 -h 24 --export-area-snap $SOURCE_NAME
	inkscape -o ../../../../../bitmaps_png/icons/tmp/32/$NAME.png -w 32 -h 32 --export-area-snap $SOURCE_NAME
	inkscape -o ../../../../../bitmaps_png/icons/tmp/48/$NAME.png -w 48 -h 48 --export-area-snap $SOURCE_NAME
	inkscape -o ../../../../../bitmaps_png/icons/tmp/64/$NAME.png -w 64 -h 64 --export-area-snap $SOURCE_NAME
	inkscape -o ../../../../../bitmaps_png/icons/tmp/256/$NAME.png -w 256 -h 256 --export-area-snap $SOURCE_NAME
	echo "file $SOURCE_NAME converted."
done

# convert .png files into .ico files using "icotool" from icoutils
# (see http://www.nongnu.org/icoutils/)
cd ../../../../../bitmaps_png/icons
for fl in $ICON_FILES
do
    NAME=${fl%.*} # strip the file extension
    icotool -c tmp/16/$NAME.png tmp/24/$NAME.png tmp/32/$NAME.png tmp/48/$NAME.png \
	tmp/64/$NAME.png tmp/256/$NAME.png -o tmp/ico/fileicon_$NAME.ico
	echo "file $fl converted in .ico file."
done

# make mosaic images locally
montage tmp/64/*.png -geometry +6+6 -tile x1 all.png
echo "mosaic images created"

# delete what is not needed
cp tmp/ico/* ./
rm -R tmp

