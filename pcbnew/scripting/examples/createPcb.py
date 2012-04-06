#!/usr/bin/env python

from pcbnew import *
import pcbnew

pcb = BOARD()
module = MODULE(pcb)
module.SetReference("M1")

pad = D_PAD(module)
module.m_Pads.PushBack(pad)
pad.thisown=0

pcb.Add(module)

pcb.Save("/tmp/my2.brd")


print map( lambda x: x.GetReference() , list(pcb.GetModules()))

print "Saved?"
