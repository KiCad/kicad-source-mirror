#!/usr/bin/env python
import sys
from pcbnew import *

filename=sys.argv[1]

pcb = LoadBoard(filename)

ToUnits = ToMM
FromUnits = FromMM
#ToUnits=ToMils
#FromUnits=FromMils

print "LISTING VIAS:"

for item in pcb.GetTracks():
	if type(item) is VIA:

		pos = item.GetPosition()
		drill = item.GetDrillValue()
		width = item.GetWidth()
		print " * Via:   %s - %f/%f "%(ToUnits(pos),ToUnits(drill),ToUnits(width))

	elif type(item) is TRACK:

		start = item.GetStart()
		end = item.GetEnd()
		width = item.GetWidth()

		print " * Track: %s to %s, width %f" % (ToUnits(start),ToUnits(end),ToUnits(width))

	else:
		print "Unknown type	%s" % type(item)

print ""
print "LIST DRAWINGS:"

for item in pcb.GetDrawings():
	if type(item) is TEXTE_PCB:
		print "* Text:    '%s' at %s"%(item.GetText(), item.GetPosition())
	elif type(item) is DRAWSEGMENT:
		print "* Drawing: %s"%item.GetShapeStr() # dir(item)
	else:
		print type(item)

print ""
print "LIST MODULES:"

for module in pcb.GetModules():
	print "* Module: %s at %s"%(module.GetReference(),ToUnits(module.GetPosition()))

print ""
print "Ratsnest cnt:",len(pcb.GetFullRatsnest())
print "track w cnt:",len(pcb.GetTrackWidthList())
print "via s cnt:",len(pcb.GetViasDimensionsList())

print ""
print "LIST ZONES:"

for idx in range(0, pcb.GetAreaCount()):
    zone=pcb.GetArea(idx)
    print "zone:", idx, "priority:", zone.GetPriority(), "netname", zone.GetNetname()

print ""
print "NetClasses:", pcb.GetNetClasses().GetCount()

