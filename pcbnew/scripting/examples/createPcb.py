#!/usr/bin/env python

from pcbnew import *

pcb = BOARD()
module = MODULE(pcb)
module.SetReference("M1")

pad = D_PAD(module)
module.Add(pad)

pcb.Add(module)
pcb.Save("/tmp/my2.brd")

print map( lambda x: x.GetReference() , list(pcb.GetModules()))

print "Saved?"
