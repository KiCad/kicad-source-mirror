#!/bin/sh

BASEDIR=/tmp/eeschema-lib

CATEGORIES="lions tigers kitties"

PARTS="eyes ears feet"

REVS="rev1 rev5 rev10"


REFERENCE="
    (reference U?
        (effects (at 12 13 180)(font (size .7 1))(visible yes))
    )"

LINE="
    (line
        (pts (xy 12 13)(xy 12 20))(line_width 1.5)
    )"

RECT="
    (rectangle
        (start 4 5)(end 6 8)(line_width 2.3)(fill transparent)
    )"

CIRCLE="
    (circle
        (center 1 0)(radius 5)(line_width 2.1)(fill none)
    )"

ARC="
    (arc
        (pos 22 33)(radius 12)(start 2 4)(end 13 33)(line_width 2.3)(fill filled)
    )"

BEZIER="
    (bezier
        (fill none)(line_width 2.0)(pts (xy 0 1)(xy 2 4))
    )"

TEXT="
    (text (at 23 23 90.0) \"This is some text\" (justify left bottom)(visible yes)(fill filled)
        (font arial (size .8 1.2))
    )"

PIN1="
    (pin output line (at 7 8 90)(length 2)(visible yes)
        (signal #WE  (font  (size 0.9 1.1) bold)(visible yes))
        (padname A23 (font (size 0.9 1.1) italic bold) (visible yes))
    )"

PIN2="
    (pin input line (at 8 8)(length 2)(visible yes)
        (signal #WAIT  (font  (size 0.9 1.1) bold)(visible yes))
        (padname A24 (font (size 0.9 1.1) italic bold) (visible yes))
    )"

PIN3="
    (pin (padname A25))"

PINS="
    (pin (padname Z12))(pin (padname Y14))(pin (padname Z13))(pin (padname Y15))"


PIN_SWAP="
    (pin_swap A23 A24)"

PIN_RENUM="
    (pin_renum A24 B24)"

PIN_RENAME="
    (pin_rename #WE LED)"

PIN_DELETE="
    (pin_del B24)"

PIN_MERGE="(pin_merge A23 (hide Z12 Y14))(pin_merge A25 (hide Z13 Y15))"


PROP1="
    (property mWatts 12
        (effects (at 1 34 270)(font (size .5 1) italic bold)(visible no))
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

