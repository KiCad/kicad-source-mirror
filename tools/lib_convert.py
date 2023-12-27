#!/usr/bin/python

# Convert a footprint library from one format to another, e.g. legacy to pretty.

# 1) Entered following command line, script takes to arguments: oldLibPath & newLibPath
# $ PYTHONPATH=. <path_to>/lib_convert.py /usr/local/share/kicad/modules/smd_dil.mod /tmp/smd_dil.pretty

# inspect one footprint found in new librarypath /tmp/smd_dil.pretty


from __future__ import print_function
from pcbnew import *
import sys

if len( sys.argv ) < 3 :
    print( "usage: script srcLibraryPath dstLibraryPath" )
    sys.exit(1)


src_libpath = sys.argv[1]
dst_libpath = sys.argv[2]


src_type = IO_MGR.GuessPluginTypeFromLibPath( src_libpath );
dst_type = IO_MGR.GuessPluginTypeFromLibPath( dst_libpath );

src_plugin = IO_MGR.PluginFind( src_type )
dst_plugin = IO_MGR.PluginFind( dst_type )

try:
    dst_plugin.DeleteLibrary( dst_libpath )
except:
    None    # ignore, new may not exist if first run

dst_plugin.CreateLibrary( dst_libpath )

list_of_parts = src_plugin.FootprintEnumerate( src_libpath )

ii = 0;
for part_id in list_of_parts:
    footprint = src_plugin.FootprintLoad( src_libpath, part_id )
    dst_plugin.FootprintSave( dst_libpath, footprint )
    ii = ii+1
    print( ii, footprint.GetFPID().GetUniStringLibId(), "->", dst_libpath )

