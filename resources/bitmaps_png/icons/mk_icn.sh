#!/bin/bash
#
# make icons using Inkscape
# inkscape (1.0.2 minimum) and icotool are necessary


# create output directories
rm -r tmp
rm icon_*.png
mkdir tmp
mkdir tmp/16 tmp/24 tmp/32 tmp/48 tmp/64 tmp/256 tmp/ico
echo "all directories created."
cd ../sources/light

ICON_FILES="icon_3d.svg
            icon_gerbview.svg
            icon_kicad.svg
            icon_kicad_nightly.svg
            icon_bitmap2component.svg
            icon_pcbcalculator.svg
            icon_pagelayout_editor.svg
            icon_cvpcb.svg
            icon_pcbnew.svg
            icon_eeschema.svg
            icon_pcm.svg"

# convert .svg files into .png files
for fl in $ICON_FILES
do
   NAME=${fl%.*} # strip the file extension
	inkscape -o ../../icons/tmp/16/$NAME.png -w 16 -h 16 --export-area-snap $fl
	inkscape -o ../../icons/tmp/24/$NAME.png -w 24 -h 24 --export-area-snap $fl
	inkscape -o ../../icons/tmp/32/$NAME.png -w 32 -h 32 --export-area-snap $fl
	inkscape -o ../../icons/tmp/48/$NAME.png -w 48 -h 48 --export-area-snap $fl
	inkscape -o ../../icons/tmp/64/$NAME.png -w 64 -h 64 --export-area-snap $fl
	inkscape -o ../../icons/tmp/256/$NAME.png -w 256 -h 256 --export-area-snap $fl
	echo "file $fl converted."
done

# convert .png files into .ico files using "icotool" from icoutils
# (see http://www.nongnu.org/icoutils/)
cd ../../icons
for fl in $ICON_FILES
do
    NAME=${fl%.*} # strip the file extension
    icotool -c tmp/16/$NAME.png tmp/24/$NAME.png tmp/32/$NAME.png tmp/48/$NAME.png \
	tmp/64/$NAME.png tmp/256/$NAME.png -o tmp/ico/$NAME.ico
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

