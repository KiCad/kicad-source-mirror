#!/usr/bin/python

# Test a basic back to back FootprintLoad() of a single footprint, and FootprintEnumerate()

# 1) Build target _pcbnew after enabling scripting in cmake.
# $ make _pcbnew

# 2) Changed dir to pcbnew
# $ cd pcbnew
# $ pwd
# build/pcbnew

# 3) Entered following command line, script takes to arguments: library_path [footprint_name]
# $ PYTHONPATH=. <path_to>/test_plugin.py https://github.com/liftoff-sr/pretty_footprints   [100-LQFP]


from __future__ import print_function
from pcbnew import *
import sys

if len( sys.argv ) < 2 :
    print( "usage: script <library_path>  [<footprint_name>]" )
    sys.exit(1)


src_libpath = sys.argv[1]


src_type = IO_MGR.GuessPluginTypeFromLibPath( src_libpath );

src_plugin = IO_MGR.PluginFind( src_type )

if len( sys.argv ) == 2:
    list_of_footprints = src_plugin.FootprintEnumerate( src_libpath )
    for fp in list_of_footprints:
        print( fp )

elif len( sys.argv ) == 3:
    # I had some concerns about back to back reads, this verifies it is no problem:
    module = src_plugin.FootprintLoad( src_libpath, sys.argv[2] )
    if not module:
        print( "1st try: module", sys.argv[2], "not found" )
    module = src_plugin.FootprintLoad( src_libpath, sys.argv[2] )
    if not module:
        print( "2nd try: module", sys.argv[2], "not found" )
    print( module )


