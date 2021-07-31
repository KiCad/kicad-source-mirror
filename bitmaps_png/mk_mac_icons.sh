#!/bin/bash
#
# make macos icons using Inkscape

mkdir macos_tmp
# convert .svg files into .png files

SIZES="16x16
       32x32
       128x128
       256x256
       512x512
       1024x1024"

ICONS="bitmap2component
       eeschema
       gerbview
       kicad
       pagelayout_editor
       pcbcalculator
       pcbnew"


for pgm in $ICONS
do
    output=""

    for size in $SIZES
    do
        sz=${size%x*}

        echo -e '\E[0;32m'"\nMaking the applications icons with size $size."
        
        # MacOS wants icons with 10% clearance on each side
        let "sub_sz = $sz * 8 / 10"

        # Use specialized icons for smaller sizes to keep pixel alignment
        if [ $sz -le 32 ]
        then
            inkscape sources/light/icon_${pgm}_${sz}.svg -o macos_tmp/${pgm}_small_${sz}px.png -w ${sub_sz} -h ${sub_sz} --export-area-snap
        else
            inkscape sources/light/icon_${pgm}.svg -o macos_tmp/${pgm}_small_${sz}px.png -w ${sub_sz} -h ${sub_sz} --export-area-snap
        fi

        convert macos_tmp/${pgm}_small_${sz}px.png -size $size xc:transparent +swap -gravity center -composite macos_tmp/${pgm}_${sz}px.png

        output+="macos_tmp/${pgm}_${sz}px.png "
    done

    if [ ${pgm} == "pcbcalculator" ]
    then
        # mismatch in the pcbcalculator icon names
        png2icns ../pcb_calculator/pcb_calculator.icns ${output}
    elif [ ${pgm} == "bitmap2component" ]
    then
        #bitmap2component does not have associated documents
        png2icns ../${pgm}/${pgm}.icns ${output}
    else
        png2icns ../${pgm}/${pgm}.icns ${output}
        cp ../${pgm}/${pgm}.icns ../${pgm}/${pgm}_doc.icns
    fi
done

rm -rf macos_tmp