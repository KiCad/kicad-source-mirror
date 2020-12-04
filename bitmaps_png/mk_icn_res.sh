#!/bin/bash
#
# make icons using Inkscape

# create output directories
rm -r ../resources/linux/mime/icons/hicolor/*
mkdir -p ../resources/linux/mime/icons/hicolor/scalable
cd ../resources/linux/mime/icons/hicolor
echo -e '\E[0;34m'"Directory \"scalable\" for .svg icons was created."

# copy sources to the scalable dir
cd ../../../../../bitmaps_png/sources
cp icon_kicad.svg ../../resources/linux/mime/icons/hicolor/scalable/kicad.svg
cp icon_pcbnew.svg ../../resources/linux/mime/icons/hicolor/scalable/pcbnew.svg
cp icon_eeschema.svg ../../resources/linux/mime/icons/hicolor/scalable/eeschema.svg
echo -e '\E[0;34m'"Sources of icons was copied."

# convert .svg files into .png files
cd ../../resources/linux/mime/icons/hicolor/scalable

SIZES="16x16
       22x22
       24x24
       32x32
       48x48"

for size in $SIZES
do
    sz=${size%x*}
    echo -e '\E[0;32m'"\nMaking the mimetypes icons with size $size."
    mkdir ../$size
    echo -e '\E[0;34m'"Directory $size was created."
    mkdir ../$size/mimetypes
    echo -e '\E[0;34m'"Subdirectory \"mimetypes\" was created."
    tput sgr0

    inkscape -f kicad.svg -e ../$size/mimetypes/application-x-kicad-project.png -w $sz -h $sz --export-area-snap
    inkscape -f eeschema.svg -e ../$size/mimetypes/application-x-kicad-eeschema.png -w $sz -h $sz --export-area-snap
    inkscape -f pcbnew.svg -e ../$size/mimetypes/application-x-kicad-pcbnew.png -w $sz -h $sz --export-area-snap
    echo -e '\E[0;34m'"Icons with size $size was created."

    if [ $sz -eq 48 ]
    then
        echo -e '\E[0;32m'"\nMaking the applications icons with size $size."
        mkdir ../$size/apps
        echo -e '\E[0;34m'"Subdirectory \"apps\" was created."
        tput sgr0

        inkscape -f kicad.svg -e ../$size/apps/kicad.png -w $sz -h $sz --export-area-snap
        inkscape -f eeschema.svg -e ../$size/apps/eeschema.png -w $sz -h $sz --export-area-snap
        inkscape -f pcbnew.svg -e ../$size/apps/pcbnew.png -w $sz -h $sz --export-area-snap
        echo -e '\E[0;34m'"Icons with size $size was created."
        tput sgr0
    fi
done

