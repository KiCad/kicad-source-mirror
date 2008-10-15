#!/bin/sh

# Copyright 2007 SoftPLC Corporation
# dick@softplc.com


# This script can operate on a *.brd file or a *.mod file and serves three purposes:
# 1) it resizes all the fonts to XSIZE and YSIZE
# 2) it sets the pen width of the fonts to PENWIDTH
# 3) it sets the "value" fields in all modules to "invisible"

# The two steps above correspond to the two regular expressions in the sed
# invocation below.


#----<configuration>----------------------------------------------------

# tenths of mils, e.g. 400 = .0400 inches
XSIZE=300
YSIZE=350

# tenths of mils
PENWIDTH=60

#----</configuration>--------------------------------------------------



if [ $# -ne 1 ]; then
    echo "usage: $0 <boardfile.brd>"
    exit 1
fi


sed \
-e "s/^\(T[0-9] -*[0-9]\{1,6\}\ -*[0-9]\{1,6\}\) -*[0-9]\{1,6\} -*[0-9]\{1,6\} \(-*[0-9]\{1,6\}\) [0-9]\{1,6\} \([NM] [IV] -*[0-9]\{1,6\} .*\)/\1 $YSIZE $XSIZE \2 $PENWIDTH \3/" \
-e "s/^\(T1 -*[0-9]\{1,6\}\ -*[0-9]\{1,6\} -*[0-9]\{1,6\} -*[0-9]\{1,6\} -*[0-9]\{1,6\} [0-9]\{1,6\} [NM]\) [IV] \(-*[0-9]\{1,6\} .*\)/\1 I \2/" \
$1 > out.tmp

# delete original and rename out.tmp to original
rm $1
mv out.tmp $1

