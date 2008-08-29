#!/bin/bash

svnrev=$1
svnpath=$2
tempdir=kicad-$$

if [ -z "$svnrev" ]; then
  echo "usage: $0 <svnrevision> [svnrepo]"
  exit 1
fi

if [ -z "$svnpath" ]; then
  svnpath="https://kicad.svn.sourceforge.net/svnroot/kicad/trunk"
fi

mkdir ${tempdir}
cd ${tempdir}

# export requested revision
echo "Exporting..."
svn export -r ${svnrev} ${svnpath}/kicad
svn export -r ${svnrev} ${svnpath}/kicad-doc
svn export -r ${svnrev} ${svnpath}/kicad-library

# create "include/config.h" with svn date & revision in it
echo "Getting svn revision info..."
svndate=`svn info -r ${svnrev} ${svnpath}/kicad | grep "Last Changed Date: " | cut -f4 -d' ' | sed s/-//g`
cat <<EOF >kicad/include/config.h
#ifndef __KICAD_SVN_VERSION_H__
#define __KICAD_SVN_VERSION_H__

#define KICAD_ABOUT_VERSION "svn-r${svnrev} (${svndate})"

#endif  /* __KICAD_SVN_VERSION_H__ */
EOF

# get main program version from an include file
mainver=`cat kicad/include/build_version.h | grep 'main program version' | cut -d\( -f4 | cut -d\) -f1`

cd ..

# rename with proper version and tar it up
mv ${tempdir} kicad-${mainver}
tar -zcf kicad-${mainver}.tar.z kicad-${mainver}

