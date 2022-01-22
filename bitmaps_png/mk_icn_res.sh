#!/bin/bash
#
# make icons using Inkscape

ICON="kicad
      pcbnew
      eeschema
      gerbview
      bitmap2component
      pcbcalculator"

SIZE="16x16
      24x24
      32x32
      48x48
      64x64
      128x128"

# create output directories
rm -r ../resources/linux/icons/hicolor/*
rm -r ../resources/linux/icons-nightly/hicolor/*

mkdir -p ../resources/linux/icons/hicolor/scalable/mimetypes ../resources/linux/icons/hicolor/scalable/apps
mkdir -p ../resources/linux/icons-nightly/hicolor/scalable/apps

echo -e '\E[0;34m'"Directory \"scalable\" for .svg icons was created."
tput sgr0

# copy sources to the scalable dir
cp ./sources/light/icon_kicad.svg ../resources/linux/icons/hicolor/scalable/mimetypes/application-x-kicad-project.svg
cp ./sources/light/icon_pcbnew.svg ../resources/linux/icons/hicolor/scalable/mimetypes/application-x-kicad-pcb.svg
cp ./sources/light/icon_eeschema.svg ../resources/linux/icons/hicolor/scalable/mimetypes/application-x-kicad-schematic.svg

for icon in $ICON
do
    cp ./sources/light/icon_$icon.svg ../resources/linux/icons/hicolor/scalable/apps/$icon.svg
done

cp ./sources/light/icon_kicad_nightly.svg ../resources/linux/icons-nightly/hicolor/scalable/apps/kicad.svg

echo -e '\E[0;34m'"Sources of icons were copied."
tput sgr0

# convert .svg files into .png files
for size in $SIZE
do
    sz=${size%x*}

    source_sz=""

    if [ $sz -le 32 ]
    then
        source_sz="_$sz"
    fi

	echo -e '\E[0;32m'"\nMaking icons with size $size from \"source$source_sz\"."
    tput sgr0

    mkdir -p ../resources/linux/icons/hicolor/$size/mimetypes ../resources/linux/icons/hicolor/$size/apps
    mkdir -p ../resources/linux/icons-nightly/hicolor/$size/apps

    inkscape -o ../resources/linux/icons/hicolor/$size/mimetypes/application-x-kicad-project.png -w $sz -h $sz --export-area-snap ./sources/light/icon_kicad${source_sz}.svg
    inkscape -o ../resources/linux/icons/hicolor/$size/mimetypes/application-x-kicad-pcb.png -w $sz -h $sz --export-area-snap ./sources/light/icon_pcbnew${source_sz}.svg
    inkscape -o ../resources/linux/icons/hicolor/$size/mimetypes/application-x-kicad-schematic.png -w $sz -h $sz --export-area-snap ./sources/light/icon_eeschema${source_sz}.svg

    for icon in $ICON
    do
        inkscape -o ../resources/linux/icons/hicolor/$size/apps/$icon.png -w $sz -h $sz --export-area-snap ./sources/light/icon_${icon}${source_sz}.svg
    done
    inkscape -o ../resources/linux/icons-nightly/hicolor/$size/apps/kicad.png -w $sz -h $sz --export-area-snap ./sources/light/icon_kicad_nightly${source_sz}.svg

done
