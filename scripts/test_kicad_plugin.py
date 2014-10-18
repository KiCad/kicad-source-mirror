#!/usr/bin/python

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
from pcbnew import *
import sys
import os


lib_path1='f:/tmp/lib1.pretty'
lib_path2='f:/tmp/lib2.pretty'


plugin = IO_MGR.PluginFind( IO_MGR.KICAD )

# Expecting "KiCad":
print( "Plugin Type", plugin.PluginName() )

try:
    plugin.FootprintLibDelete( lib_path1 )
except:
    None    # ignore, new may not exist if first run

try:
    plugin.FootprintLibDelete( lib_path2 )
except:
    None    # ignore, new may not exist if first run


plugin.FootprintLibCreate( lib_path1 )

# Verify that the same plugin instance can edge trigger on a lib_path change
# for a FootprintLibCreate()
plugin.FootprintLibCreate( lib_path2 )


board = BOARD()

# The only way to construct a MODULE is to pass it a BOARD? Yep.
module = MODULE( board )

fpid=FPID( 'mine' )

module.SetFPID( fpid )

plugin.FootprintSave( lib_path2, module )

# Verify that the same plugin instance can edge trigger on a lib_path change
# for a FootprintSave()
plugin.FootprintSave( lib_path1, module )

# create a disparity between the library's name ("footprint"),
# and the module's internal useless name ("mine").  Module is officially named "footprint" now
# but has (module mine ...) internally:
os.rename( 'f:/tmp/lib2.pretty/mine.kicad_mod', 'f:/tmp/lib2.pretty/footprint.kicad_mod' )

footprint=plugin.FootprintLoad( lib_path2, 'footprint' )

fpid = footprint.GetFPID()
fpid.SetLibNickname( UTF8( 'mylib' ) )
name = fpid.Format().GetChars()

# Always after a FootprintLoad() the internal name should match the one used to load it.
print( "internal name should be 'footprint':", name  )

# Verify that the same plugin instance can edge trigger on a lib_path change
# for FootprintLoad()
footprint=plugin.FootprintLoad( lib_path1, 'mine' )

fpid = footprint.GetFPID()
fpid.SetLibNickname( UTF8( 'other_mylib' ) )
name = fpid.Format().GetChars()

# Always after a FootprintLoad() the internal name should match the one used to load it.
print( "internal name should be 'mine':", name )

# As of 3-Dec-2013 this test is passed by KICAD_PLUGIN and Wayne is owed an atta boy!

