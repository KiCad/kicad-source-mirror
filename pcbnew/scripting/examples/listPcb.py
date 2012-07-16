#!/usr/bin/env python
import sys
from pcbnew import *

filename=sys.argv[1]

pcb = LoadBoard(filename)

#ToUnits = ToMM 
#FromUnits = FromMM 
ToUnits=ToMils
FromUnits=FromMils

print "LISTING VIAS:"

for item in pcb.GetTracks():
	if type(item) is SEGVIA:
		
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
print "LISTING DRAWINGS:"

for item in pcb.GetDrawings():
	if type(item) is TEXTE_PCB:
		print "* Text:    '%s' at %s"%(item.GetText(),item.GetPosition())
	elif type(item) is DRAWSEGMENT:
		print "* Drawing: %s"%item.GetShapeStr() # dir(item)
	else:
		print type(item)
	
print ""
print "LIST MODULES:"

for module in pcb.GetModules():
	print "* Module: %s at %s"%(module.GetReference(),ToUnits(module.GetPosition()))
	
print "" 
print "LIST ZONES:"

for zone in pcb.GetSegZones():
	print zone
	
	
print ""
print "RATSNEST:",len(pcb.GetFullRatsnest())

print dir(pcb.GetNetClasses())
	