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

DOCS="project
      schematic
      pcb
      footprint
      symbol
      worksheet"

for doc in $DOCS
do
    output=""

    for size in $SIZES
    do
        sz=${size%x*}

        echo -e '\E[0;32m'"\nMaking the documents icons with size $size."

        # MacOS wants icons with 10% clearance on each side
        let "sub_sz = $sz * 8 / 10"

        sigma=3
        if [ $sz -eq 16 ]
        then
            sigma=2
        elif [ $sz -ge 512 ]
        then
            sigma=5
        fi

        # Offset the drop shadow by one smaller than the kernel
        let "off = $sigma - 1"

        # Use specialized icons for smaller sizes to keep pixel alignment
        if [ $sz -le 32 ]
        then
            inkscape linux/icons/hicolor/scalable/mimetypes/application-x-kicad-${doc}-${sz}.svg -o macos_tmp/${doc}_small_${sz}px.png -w ${sub_sz} -h ${sub_sz} --export-area-snap
        else
            inkscape linux/icons/hicolor/scalable/mimetypes/application-x-kicad-${doc}.svg -o macos_tmp/${doc}_small_${sz}px.png -w ${sub_sz} -h ${sub_sz} --export-area-snap
        fi

        convert macos_tmp/${doc}_small_${sz}px.png \( +clone -background black -shadow 80x${sigma}+${off}+${off} \) +swap -background transparent -layers merge +repage -size $size xc:transparent +swap -gravity center -composite macos_tmp/${doc}_${sz}px.png

        output+="macos_tmp/${doc}_${sz}px.png "
    done

    png2icns macos_tmp/${doc}.icns ${output}
done


for pgm in $ICONS
do
    output=""

    for size in $SIZES
    do
        sz=${size%x*}

        echo -e '\E[0;32m'"\nMaking the applications icons with size $size."

        # MacOS wants icons with 10% clearance on each side
        let "sub_sz = $sz * 8 / 10"

        sigma=3
        if [ $sz -eq 16 ]
        then
            sigma=2
        elif [ $sz -ge 512 ]
        then
            sigma=5
        fi

        # Offset the drop shadow by one smaller than the kernel
        let "off = $sigma - 1"

        # Use specialized icons for smaller sizes to keep pixel alignment
        if [ $sz -le 32 ]
        then
            inkscape bitmaps_png/sources/light/icon_${pgm}_${sz}.svg -o macos_tmp/${pgm}_small_${sz}px.png -w ${sub_sz} -h ${sub_sz} --export-area-snap
        else
            inkscape bitmaps_png/sources/light/icon_${pgm}.svg -o macos_tmp/${pgm}_small_${sz}px.png -w ${sub_sz} -h ${sub_sz} --export-area-snap
        fi

        convert macos_tmp/${pgm}_small_${sz}px.png \( +clone -background black -shadow 80x${sigma}+${off}+${off} \) +swap -background transparent -layers merge +repage -size $size xc:transparent +swap -gravity center -composite macos_tmp/${pgm}_${sz}px.png

        output+="macos_tmp/${pgm}_${sz}px.png "
    done

    case ${pgm} in
        pcbcalculator)
            # mismatch in the pcbcalculator icon names
            png2icns ../pcb_calculator/pcb_calculator.icns ${output}
            ;;
        bitmap2component)
            # bitmap2component does not have associated documents
            png2icns ../${pgm}/${pgm}.icns ${output}
            ;;
        eeschema)
            png2icns ../${pgm}/${pgm}.icns ${output}
            cp macos_tmp/schematic.icns ../${pgm}/${pgm}_doc.icns
            cp macos_tmp/symbol.icns ../${pgm}/libedit_doc.icns
            ;;
        pcbnew)
            png2icns ../${pgm}/${pgm}.icns ${output}
            cp macos_tmp/pcb.icns ../${pgm}/${pgm}_doc.icns
            cp macos_tmp/footprint.icns ../${pgm}/fpedit_doc.icns
            ;;
        kicad)
            png2icns ../${pgm}/${pgm}.icns ${output}
            cp macos_tmp/project.icns ../${pgm}/${pgm}_doc.icns
            ;;
        pagelayout_editor)
            png2icns ../${pgm}/${pgm}.icns ${output}
            cp macos_tmp/worksheet.icns ../${pgm}/${pgm}_doc.icns
            ;;
        *)
            png2icns ../${pgm}/${pgm}.icns ${output}
            cp ../${pgm}/${pgm}.icns ../${pgm}/${pgm}_doc.icns
            ;;
    esac
done

# rm -rf macos_tmp