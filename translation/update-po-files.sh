#!/bin/bash -e
#####################################
#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2015 Marco Ciampa <ciampix@libero.it>
# Copyright (C) 2015-2016 KiCad Developers
#
# License GNU GPL Version 3 or any later version.
#
#####################################

export LC_ALL=C

display_help() {
  echo "Usage: $0 [-k] [-p] [-s=<path>] <locale|all>"
  echo "  -k keep pot template and not delete it"
  echo "  -p plot the translation statistics [requires python with matplotlib]"
  echo "  -s=<path> path to kicad source code"
  exit
}

# Handle command line arguments
for i in "$@"; do
case $i in
  -h|--help)
  display_help
  shift
  ;;
  -k)
  KEEP=1
  shift
  ;;
  -p)
  PLOT=1
  shift
  ;;
  -s=*)
  SOURCEDIR="${i#*=}"
  shift
  ;;
  *)
  SINGLE_LANG=$i
  ;;
esac
done

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ -z ${SOURCEDIR} ]; then
  SOURCEDIR=${DIR}/..
  echo "Using default SOURCEDIR=${SOURCEDIR}"
fi

#Autovars
cd $(dirname ${BASH_SOURCE[0]})
LOCALDIR=$PWD
CSVFILE=${PWD}/i18n_status.csv
POTDIRS=`cat $LOCALDIR/POTDIRS|grep -v '^#'|grep -v '^\s*$'` #Read file without comment and empty lines

cd $SOURCEDIR

#Generate/update template pot file
find $POTDIRS -name '*.cpp' -or -name '*.h' |
  sort |
  xgettext -f- -k_ -k_HKI -kwxPLURAL:1,2 --force-po --from-code utf-8 -o $LOCALDIR/kicad.pot

LINGUAS=`cat $LOCALDIR/pofiles/LINGUAS|grep -v '^#'|grep -v '^\s*$'` #Read file without comment and empty lines

#check if present in locale list
validate() { echo $LINGUAS | grep -F -q -w "$1"; }

if [ "$SINGLE_LANG" != "all" ] ; then
#If supplied, update only the specified locale
  if [ "$SINGLE_LANG" = "" ] ; then
    display_help
  elif validate "$SINGLE_LANG"; then
    LINGUAS="$SINGLE_LANG"
  else
    echo "Error!"
    echo "Locale argument \"$1\" not present in current locale list:"
    for i in $LINGUAS; do echo -n "$i "; done
    echo # newline
    exit 1
  fi
fi


echo "Writing summary to ${CSVFILE}"
echo "LANG;TRANSLATED;FUZZY;UNTRANSLATED" > "${CSVFILE}"

for i in $LINGUAS
do
  echo "## $i"
  if [ "$i" = "en" ] ; then
    msgmerge --no-location --no-fuzzy-matching --force-po $LOCALDIR/$i/kicad.po $LOCALDIR/kicad.pot -o $LOCALDIR/$i/kicad.po 2> /dev/null
    msgen $LOCALDIR/pofiles/$i.po -o $LOCALDIR/pofiles/$i.po.tmp && mv $LOCALDIR/pofiles/$i.po.tmp $LOCALDIR/pofiles/$i.po
  else
    msgmerge --force-po $LOCALDIR/pofiles/$i.po $LOCALDIR/pofiles/kicad.pot -o $LOCALDIR/pofiles/$i.po 2> /dev/null
  fi
  msgfmt --statistics $LOCALDIR/pofiles/$i.po -o $LOCALDIR/pofiles/messages.mo 2>&1 >>/dev/null |
    while IFS=",." read A B C D ; do
      echo $A
      echo $B
      echo $C
      echo $D

      for STRING in "$A" "$B" "$C" "$D" ; do
        STRING=${STRING# }
        case "$STRING" in
        *" translated message"* )
          TRANSLATED=${STRING% translated message*}
          ;;
        *" fuzzy translation"* )
          FUZZY=${STRING% fuzzy translation*}
          ;;
        *" untranslated message"* )
          UNTRANSLATED=${STRING% untranslated message*}
          ;;
        "" )
          ;;
        * )
          echo >&2 "$0: Unknown format of \"msgfmt --statistics $LOCALDIR/$i/kicad.po \": \"$STRING\""
          exit 1
          ;;
        esac
      done
      echo "$i;${TRANSLATED};${FUZZY};${UNTRANSLATED}">>"${CSVFILE}"
    done
    rm $LOCALDIR/pofiles/messages.mo
done

if [ "$PLOT" = "1" ]; then
  cd $LOCALDIR
  $LOCALDIR/plot_i18n_status.py
fi

if [ ! "$KEEP" = "1" ]; then
  rm $LOCALDIR/pofiles/kicad.pot
fi
