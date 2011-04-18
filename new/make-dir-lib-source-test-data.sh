#!/bin/sh

BASEDIR=/tmp/eeschema-lib

CATEGORIES="lions tigers kitties"

PARTS="eyes ears feet"

REVS="rev1 rev5 rev10"


REFERENCE="
    (reference U?
        (effects (at 12 13 180)(font (size 7 10))(visible yes))
    )"

LINE="
    (line
        (pts (xy 12 13)(xy 12 20))(stroke 1.5)
    )"

RECT="
    (rectangle
        (start 4 5)(end 6 8)(stroke 2.3)(fill transparent)
    )"

CIRCLE="
    (circle
        (center 1 0)(radius 5)(stroke 2.1)(fill none)
    )"

ARC="
    (arc
        (pos 22 33)(radius 12)(start 2 4)(end 13 33)(stroke 2.3)(fill filled)
    )"

BEZIER="
    (bezier
        (fill none)(stroke 2.0)(pts (xy 0 1)(xy 2 4))
    )"

TEXT="
    (text (at 23 23 90.0) \"This is some text\" (justify left bottom)(visible yes)(fill filled)
        (font arial (size 8 12))
    )"

PIN1="
    (pin out line (at 7 8 90)
        (signal #WE  (font  (size 8 10) bold)(visible no))
        (pad A23 (font (size 9 11) italic bold))
    )"

PIN2="
    (pin in line (at 8 8)(visible yes)
        (signal #WAIT  (visible yes))
        (pad A24 (visible yes))
    )"

PIN3="
    (pin (pad A25))"

PINS="
    (pin (pad Z12))(pin (pad Y14))(pin (pad Z13))(pin (pad Y15))"


PIN_SWAP="
    (pin_swap A23 A24)"

PIN_RENUM="
    (pin_renum A24 B24)"

PIN_RENAME="
    (pin_rename B24 LED)"

PIN_DELETE="
    (pin_del B24)"

PIN_MERGE="(pin_merge A23 (pads Z12 Y14))(pin_merge A25 (pads Z13 Y15))"


PROP1="
    (property mWatts 12
        (effects (at 1 34 270)(font (size 5 9) italic bold)(visible no))
    )"

KEYWORDS="
    (keywords varistor batcave einstein)"

ALTERNATES="
    (alternates 7400/7400_b 7400/7400_c)"



for C in ${CATEGORIES}; do

    mkdir -p $BASEDIR/$C

    for P in ${PARTS};  do
        for R in ${REVS}; do
            echo "(part $C/$P (value 22)(footprint SM0805)(model Airplane)(datasheet http://favorite.pdf)
                $REFERENCE
                $LINE
                $RECT
                $CIRCLE
                $ARC
                $BEZIER
                $TEXT
                $PIN1
                $PIN2
                $PIN3
                $PINS
                $PROP1
                $KEYWORDS
                $ALTERNATES
                $PIN_SWAP
                $PIN_RENUM
                $PIN_RENAME
                $PIN_DELETE
                $PIN_MERGE
                )" > $BASEDIR/$C/$P.part.$R
        done
        # also make the part without a rev:
        echo "(part $C/$P (value 22)(footprint SM0805)(model Airplane)(datasheet http://favorite.pdf)
            $REFERENCE
            $LINE
            $RECT
            $CIRCLE
            $ARC
            $BEZIER
            $TEXT
            $PIN1
            $PIN2
            $PIN3
            $PINS
            $PROP1
            $KEYWORDS
            $ALTERNATES
            $PIN_SWAP
            $PIN_RENUM
            $PIN_RENAME
            $PIN_DELETE
            $PIN_MERGE
            )" > $BASEDIR/$C/$P.part
    done
done

