#!/bin/bash
#####################################
#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2015 Marco Ciampa <ciampix@libero.it>
# Copyright (C) 2015 KiCAd Developers
#
# License GNU GPL Version 3 or any later version.
#
#####################################

SOURCEDIR=../kicad-source-mirror #Set this first!!!

#Autovars
LOCALDIR=$PWD
LINGUAS=`cat LINGUAS|grep -v '^#'|grep -v '^\s*$'` #Read file without comment and empty lines
POTDIRS=`cat POTDIRS|grep -v '^#'|grep -v '^\s*$'` #Read file without comment and empty lines

cd $SOURCEDIR

#Generate source file list
for f in $POTDIRS
do
  find $f -name "*.cpp" >>$LOCALDIR/POTFILES #List files
  find $f -name "*.h"   >>$LOCALDIR/POTFILES #List files
done

#Generate/update template pot file
xgettext -f $LOCALDIR/POTFILES -k_ -k_HKI --force-po --from-code utf-8 -o $LOCALDIR/kicad.pot

rm $LOCALDIR/POTFILES

for i in $LINGUAS
do
  msgmerge --force-po $LOCALDIR/$i/kicad.po $LOCALDIR/kicad.pot -o $LOCALDIR/$i/kicad.po 2>&1 # >> /dev/null
#  msgfmt --statistics $LOCALDIR/$i/kicad.po 2>&1 >>/dev/null
done

#Comment this to create new language template
rm $LOCALDIR/kicad.pot
