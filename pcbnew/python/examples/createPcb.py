#!/usr/bin/env python

from __future__ import print_function
import sys
from pcbnew import *

# A helper function to convert a UTF8 string coming from Kicad for python2 or python3
def fromUTF8Text( aText ):
    if sys.version_info < (3, 0):
        return aText.encode()
    else:
        return aText

def GenerateBoard():
    size_0_6mm = VECTOR2I_MM(0.6,0.6)
    size_1_0mm = VECTOR2I_MM(1.0,1.0)

    # create a blank board
    pcb = CreateEmptyBoard()

    # create a new footprint, it's parent is our previously created pcb
    footprint = FOOTPRINT(pcb)
    footprint.SetReference("M1")   # give it a reference name
    footprint.Reference().SetPos(VECTOR2I_MM(3,-2))
    pcb.Add(footprint)             # add it to our pcb
    mod_pos = VECTOR2I_MM(50,50)
    footprint.SetPosition(mod_pos)

    # create a pad array and add it to the footprint
    n = 1
    for y in range (0,5):
        for x in range (0,5):
            pad = PAD(footprint)
            pad.SetDrillSize(size_0_6mm)
            pad.SetSize(size_1_0mm)
            pt = VECTOR2I_MM(1.27*x,1.27*y) + footprint.GetPosition()
            pad.SetPosition(pt);
            pad.SetName(str(n))
            footprint.Add(pad)
            n+=1


    # save the PCB to disk
    pcb.Save("my2.kicad_pcb")

def ReadBoard():
    pcb = LoadBoard("my2.kicad_pcb")

    for m in pcb.GetFootprints():
        print( 'footprint: ', fromUTF8Text( m.GetReference() ) )

        for p in m.Pads():
            print('pad ', fromUTF8Text( p.GetName() ), ToMM(p.GetPosition()), ToMM(p.GetOffset()))

GenerateBoard()
ReadBoard()
