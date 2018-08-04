#!/usr/bin/env python

from __future__ import print_function

from pcbnew import *

size_0_6mm = wxSizeMM(0.6,0.6)
size_1_0mm = wxSizeMM(1.0,1.0)

# create a blank board
pcb = BOARD()

pcb.GetNetClasses().GetDefault().SetClearance(FromMM(0.1))

# create a new module, it's parent is our previously created pcb
module = MODULE(pcb)
module.SetReference("M1")   # give it a reference name
module.Reference().SetPos0(wxPointMM(6,-2))
module.Reference().SetDrawCoord()
pcb.Add(module)             # add it to our pcb
m_pos = wxPointMM(50,50)
module.SetPosition(m_pos)

# create a pad array and add it to the module
n = 1
for y in range (0,10):
    for x in range (0,10):
        pad = D_PAD(module)
        pad.SetDrillSize(size_0_6mm)
        pad.SetSize(size_1_0mm)
        pt = wxPointMM(1.27*x,1.27*y)
        pad.SetPos0(pt);
        pad.SetDrawCoord()
        pad.SetName(str(n))
        module.Add(pad)
        n+=1


# save the PCB to disk
pcb.Save("my2.kicad_pcb")

pcb = LoadBoard("my2.kicad_pcb")

print(map( lambda x: x.GetReference() , list(pcb.GetModules())))

for m in pcb.GetModules():
    for p in m.Pads():
        print('pad ', p.GetName(), p.GetPosition(), p.GetOffset())


# pcb.GetDesignSettings()
