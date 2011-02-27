#!/bin/sh

BASEDIR=/tmp/eeschema-lib

CATEGORIES="lions tigers kitties"

PARTS="eyes ears feet"

REVS="rev1 rev5 rev10"

LINE="(line (pts (xy 12 13)(xy 12 20))(line_width 1.5))"
RECT="(rectangle (start 4 5)(end 6 8)(line_width 2.3)(fill transparent))"
CIRCLE="(circle (center 1 0)(radius 5)(line_width 2.1)(fill none))"
ARC="(arc (pos 22 33)(radius 12)(start 2 4)(end 13 33)(line_width 2.3)(fill filled))"


for C in ${CATEGORIES}; do

    mkdir -p $BASEDIR/$C

    for P in ${PARTS};  do
        for R in ${REVS}; do
            echo "(part $C/$P (value 22)(footprint SM0805)(model Airplane)$LINE$RECT$CIRCLE$ARC)" > $BASEDIR/$C/$P.part.$R
        done
        # also make the part without a rev:
        echo "(part $C/$P (value 22)(footprint SM0805)(model Airplane)$LINE$RECT$CIRCLE$ARC)" > $BASEDIR/$C/$P.part
    done
done

