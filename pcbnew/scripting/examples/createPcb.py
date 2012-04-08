#!/usr/bin/env python2.7
import pcbnew
from pcbnew import *

size_0_6mm = wxSizeMM(0.6,0.6)


# create a blank board
pcb = BOARD()

# create a new module, it's parent is our previously created pcb
module = MODULE(pcb)
module.SetReference("M1")   # give it a reference name
pcb.Add(module)             # add it to our pcb
m_pos = wxPoint(FromMM(50),FromMM(50))
module.SetPosition(m_pos)
print "module position:",m_pos

# create a pad and add it to the module
n = 1
for y in range (0,10):
    for x in range (0,10):
        pad = D_PAD(module)
        pad.SetDrillSize(size_0_6mm)
        pt = wxPoint(FromMM(x*2),FromMM(y*2))
        pad.SetPos0(pt);
        pad.SetPosition(pt)
        pad.SetPadName(str(n))
        module.Add(pad)
        n+=1
        

# save the PCB to disk
pcb.Save("/tmp/my2.brd")

pcb = LoadBoard("/tmp/my2.brd")
#pcb = LoadBoard("/home/ajo/work/xpress-hardware/boards/hexa-xpress/esp.brd");


print map( lambda x: x.GetReference() , list(pcb.GetModules()))

for m in pcb.GetModules():
    for p in m.GetPads():
        print p.GetPadName(),p.GetPosition(), p.GetOffset()

