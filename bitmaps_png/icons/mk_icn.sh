#!/bin/bash
#
# make icons using Inkscape
# inkscape and icotool are necessary


# create output directories
rm -r tmp
rm icon_*.png
mkdir tmp
mkdir tmp/22 tmp/26 tmp/32 tmp/48 tmp/64 tmp/128 tmp/256 tmp/ico
echo "all directories created."
cd ../sources

ICON_FILES="icon_3d.svg
            icon_gerbview.svg
            icon_kicad.svg
            icon_bitmap2component.svg
            icon_pcbcalculator.svg
            icon_pl_editor.svg
            icon_cvpcb.svg
            icon_pcbnew.svg
            icon_eeschema.svg"

# convert .svg files into .png files
for fl in $ICON_FILES
do
   NAME=${fl%.*} # strip the file extension
	inkscape -f $fl -e ../icons/tmp/22/$NAME.png -w 22 -h 22 --export-area-snap
	inkscape -f $fl -e ../icons/tmp/26/$NAME.png -w 26 -h 26 --export-area-snap
	inkscape -f $fl -e ../icons/tmp/32/$NAME.png -w 32 -h 32 --export-area-snap
	inkscape -f $fl -e ../icons/tmp/48/$NAME.png -w 48 -h 48 --export-area-snap
	inkscape -f $fl -e ../icons/tmp/64/$NAME.png -w 64 -h 64 --export-area-snap
	inkscape -f $fl -e ../icons/tmp/128/$NAME.png -w 128 -h 128 --export-area-snap
	inkscape -f $fl -e ../icons/tmp/256/$NAME.png -w 256 -h 256 --export-area-snap
	echo "file $fl converted."
done

# convert .png files into .ico files using "icotool" from icoutils
# (see http://www.nongnu.org/icoutils/)
cd ../icons
for fl in $ICON_FILES
do
    NAME=${fl%.*} # strip the file extension
    icotool -c tmp/22/$NAME.png tmp/26/$NAME.png tmp/32/$NAME.png tmp/48/$NAME.png \
	tmp/64/$NAME.png tmp/128/$NAME.png -o tmp/ico/$NAME.ico
	echo "file $fl converted in .ico file."
done

# make mosaic images locally
montage tmp/64/*.png -geometry +6+6 -tile x1 all.png
echo "mosaic images created"

# create png and xpm 64 px from icon_kicad
cp tmp/64/icon_kicad.png ./icon_kicad_64.png
convert icon_kicad_64.png icon_kicad_64.xpm

# create png and xpm 128 px from icon_kicad
cp tmp/128/icon_kicad.png ./
convert icon_kicad.png icon_kicad.xpm

# delete what is not needed
cp tmp/ico/* ./
rm -R tmp

