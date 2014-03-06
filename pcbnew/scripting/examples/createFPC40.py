#!/usr/bin/env python2.7
from pcbnew import *

size_025_160mm = wxSizeMM(0.25,1.6)
size_150_200mm = wxSizeMM(1.50,2.0)
pads = 40

# create a blank board
pcb = BOARD()

pcb.m_NetClasses.GetDefault().SetClearance(FromMM(0.1))

# create a new module, it's parent is our previously created pcb
module = MODULE(pcb)
module.SetReference("FPC"+str(pads))   # give it a reference name
module.Reference().SetPos0(wxPointMM(-1,-1))
pcb.Add(module)             # add it to our pcb
m_pos = wxPointMM(50,50)
module.SetPosition(m_pos)

# create a pad array and add it to the module


def smdRectPad(module,size,pos,name):
    pad = D_PAD(module)
    pad.SetSize(size)
    pad.SetShape(PAD_RECT)
    pad.SetAttribute(PAD_SMD)
    pad.SetLayerMask(PAD_SMD_DEFAULT_LAYERS)
    pad.SetPos0(pos)
    pad.SetPadName(name)
    return pad

for n in range (0,pads):
    pad = smdRectPad(module,size_025_160mm,wxPointMM(0.5*n,0),str(n+1))
    module.Add(pad)


pad_s0 = smdRectPad(module,size_150_200mm,wxPointMM(-1.6,1.3),"0")
pad_s1 = smdRectPad(module,size_150_200mm,wxPointMM((pads-1)*0.5+1.6,1.3),"0")
module.Add(pad_s0)
module.Add(pad_s1)

e = EDGE_MODULE(module)
e.SetStart0(wxPointMM(-1,0))
e.SetEnd0(wxPointMM(0,0))
e.SetWidth(FromMM(0.2))
e.SetLayer(EDGE_LAYER)
e.SetShape(S_SEGMENT)
module.Add(e)

# save the PCB to disk
fpid = FPID("FPC"+str(pads))   #the name in library
module.SetFPID( fpid )

try:
    FootprintLibCreate("fpc40.mod")
except:
    pass # we try to create, but may be it exists already
FootprintSave("fpc40.mod",module)
