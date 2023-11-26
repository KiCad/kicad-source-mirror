#!/usr/bin/env python

from __future__ import print_function

import sys
from pcbnew import *

# A helper function to convert a UTF8 string for python2 or python3
def fromUTF8Text( afilename ):
    if sys.version_info < (3, 0):
        return afilename.encode()
    else:
        return afilename

filename=sys.argv[1]

pcb = LoadBoard(filename)

ToUnits = ToMM
FromUnits = FromMM
#ToUnits=ToMils
#FromUnits=FromMils

print("List vias:")

for item in pcb.GetTracks():
    if type(item) is PCB_VIA:
        pos = item.GetPosition()
        drill = item.GetDrillValue()
        width = item.GetWidth()
        print(" * Via:   %s - %f/%f " % (ToUnits(pos), ToUnits(drill), ToUnits(width)))

    elif type(item) is PCB_TRACK:
        start = item.GetStart()
        end = item.GetEnd()
        width = item.GetWidth()
        print(" * Track: %s to %s, width %f" % (ToUnits(start), ToUnits(end), ToUnits(width)))

    elif type(item) is PCB_ARC:
        start = item.GetStart()
        end = item.GetEnd()
        width = item.GetWidth()
        print(" * Track arc: %s to %s, width %f" % (ToUnits(start), ToUnits(end), ToUnits(width)))

    else:
        print("Unknown type    %s" % type(item))

print("")
print("List drawings:")

for item in pcb.GetDrawings():
    if type(item) is PCB_TEXT:
        print("* Text:    '%s' at %s" % ( fromUTF8Text( item.GetText() ), ToUnits(item.GetPosition()) ) )
    elif type(item) is PCB_SHAPE:
        print( "* Drawing: %s" % fromUTF8Text( item.GetShapeStr() ) )
    else:
        print("* Drawing type: %s" % type(item))

print("")
print("List footprints:")

for footprint in pcb.Footprints():
    print("* Footprint: %s at %s" % ( fromUTF8Text( footprint.GetReference() ), ToUnits(footprint.GetPosition())))

print("")
print("Nets cnt: ", pcb.GetNetCount())
print("track w cnt:", len(pcb.GetTrackWidthList()))
print("via s cnt:", len(pcb.GetViasDimensionsList()))

print("")
print("List zones:", pcb.GetAreaCount())

for idx in range(0, pcb.GetAreaCount()):
    zone=pcb.GetArea(idx)
    print("zone:", idx, "priority:", zone.GetAssignedPriority(), "netname", fromUTF8Text( zone.GetNetname() ) )
