#!/usr/bin/env python

# Test the KiCad plugin regarding some expected features.

# 1) Build target _pcbnew after enabling scripting in cmake.
# $ make _pcbnew

# 2) Changed dir to pcbnew
# $ cd pcbnew
# $ pwd
# build/pcbnew

# 3) Entered following command line, script takes no arguments
# $ PYTHONPATH=. <path_to>/test_kicad_plugin.py

from __future__ import print_function
from pcbnew import IO_MGR, BOARD, FOOTPRINT, LIB_ID, UTF8
from os import rename as mv

tmp_path = '/tmp'
lib_path1 = "%s/lib1.pretty" % tmp_path
lib_path2 = "%s/lib2.pretty" % tmp_path

plugin = IO_MGR.PluginFind( IO_MGR.KICAD_SEXP )

# Expecting "KiCad":
print( "Plugin Type: %s" % plugin.PluginName() )

try:
    plugin.DeleteLibrary( lib_path1 )
except:
    pass    # ignore, new may not exist if first run

try:
    plugin.DeleteLibrary( lib_path2 )
except:
    pass    # ignore, new may not exist if first run

plugin.CreateLibrary( lib_path1 )

# Verify that the same plugin instance can edge trigger on a lib_path change
# for a CreateLibrary()
plugin.CreateLibrary( lib_path2 )

board = BOARD()

# The only way to construct a FOOTPRINT is to pass it a BOARD? Yep.
module = FOOTPRINT( board )

fpid = LIB_ID( '', 'mine' )

module.SetFPID( fpid )

plugin.FootprintSave( lib_path2, module )

# Verify that the same plugin instance can edge trigger on a lib_path change
# for a FootprintSave()
plugin.FootprintSave( lib_path1, module )

# create a disparity between the library's name ("footprint"),
# and the module's internal useless name ("mine").  Module is officially named "footprint" now
# but has (module mine ...) internally:
mv( "%s/mine.kicad_mod" % lib_path2, "%s/footprint.kicad_mod" % lib_path2 )

footprint = plugin.FootprintLoad( lib_path2, 'footprint' )

fpid = footprint.GetFPID()
fpid.SetLibNickname( UTF8( 'mylib' ) )
name = fpid.Format( fpid ).GetChars() # example to get the UTF8 char buffer

# Always after a FootprintLoad() the internal name should match the one used to load it.
print( "Internal name should be 'footprint': '%s'" % name  )

# Verify that the same plugin instance can edge trigger on a lib_path change
# for FootprintLoad()
footprint = plugin.FootprintLoad( lib_path1, 'mine' )

fpid = footprint.GetFPID()
fpid.SetLibNickname( UTF8( 'other_mylib' ) )

# Always after a FootprintLoad() the internal name should match the one used to load it.
# Example to print an UTF8 string
print( "Internal name should be 'mine': '%s'" % fpid.Format( fpid ) )

# As of 3-Dec-2013 this test is passed by KICAD_PLUGIN and Wayne is owed an atta boy!

