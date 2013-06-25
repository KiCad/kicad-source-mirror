#!/bin/sh
# Generate a footprint library table which can serve as a sensible initial
# personal/global table assuming you have all the footprint libraries installed
# from the bazaar library repo.
# Copyright (C) 2007-2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
# License GPLv2

# This program makes the table either in s-expression or CSV format.
# The CSV format can be loaded into a spreadsheet and then from there rows
# can be copied into either of the fp_table_dialog's table editors:
# a) personal/global or b) project


# Usage:
# $ make_global_table.sh [--csv] > outputfile

# Library Type:  Legacy for now, Kicad in future
libtype="Legacy"


# Get a list of modules currently in the launchpad repo:
mods=`bzr ls -d lp:~kicad-lib-committers/kicad/library modules 2>/dev/null`
#echo "$mods"

mods=`echo "$mods" | egrep '\.mod$' | sort`
#echo "$mods"

csv=0

if [ $# -gt 0  -a "$1" == "--csv" ]; then
    csv=1
fi

today=`date --rfc-3339=date`

if [ $csv -eq 1 ]; then
    echo "NICKNAME,TYPE,URI,OPTIONS,DESCR,,FP LIB TABLE: made from KiCad's Bazaar 'library' repository on $today"
    echo

    echo "$mods" | while read mod;
    do
        # base filename w/o extension
        bfn=`basename $mod .mod`

        printf '%s,%s,${KISYSMOD}/%s,"",""\n' \
            "$bfn" \
            "$libtype" \
            "$bfn.mod"
    done

else

    echo "# FP LIB TABLE: made from KiCad's Bazaar 'library' repository on $today"
    echo "(fp_lib_table"

    echo "$mods" | while read mod;
    do
        # base filename w/o extension
        bfn=`basename $mod .mod`

        printf ' (lib (name %s)(type %s)(uri ${KISYSMOD}/%s)(options "")(descr ""))\n' \
            "$bfn" \
            "$libtype" \
            "$bfn.mod"
    done

    echo ")"
fi
