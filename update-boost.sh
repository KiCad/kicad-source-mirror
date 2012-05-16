#!/bin/sh

# A shell script to update the boost subset within KiCad to newer version.

BOOST_VERSION=1_49_0
SRC_BOOST=/tmp/boost_$BOOST_VERSION/boost
KICAD_BASE=/svn/kicad/testing.checkout
DST_BOOST="$KICAD_BASE/include/boost"

# control the subset of boost libs used:
BOOST_DIRS="\
    archive \
    bind \
    concept \
    config \
    detail \
    exception \
    functional \
    heap \
    integer \
    iterator \
    move \
    mpl \
    multi_index \
    optional \
    polygon \
    preprocessor \
    property_tree \
    ptr_container \
    range \
    regex \
    serialization \
    smart_ptr \
    tuple \
    typeof \
    type_traits \
    unordered \
    utility \
    "

# staging area for non-included dirs:
# numeric accumulators algorithm


# remove all old BOOST include files using bzr
bzr rm --no-backup --quiet "$DST_BOOST/*"

# copy all *.hpp files in the base boost directory:
eval cp "$SRC_BOOST/*.hpp" "$DST_BOOST/"

# copy recursively all chosen boost libraries:
for D in $BOOST_DIRS; do
    eval cp -r "$SRC_BOOST/$D" "$DST_BOOST"
done

# tell bzr about the new files so they become part of the repo.
eval bzr add "$DST_BOOST/*"

eval echo "boost version: $BOOST_VERSION" > "$DST_BOOST/boost_version.txt"

