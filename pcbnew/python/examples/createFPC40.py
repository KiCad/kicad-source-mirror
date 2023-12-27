#!/usr/bin/env python
import os.path
from pcbnew import *

size_025_160mm = wxSizeMM(0.25,1.6)
size_150_200mm = wxSizeMM(1.50,2.0)
pads = 40

# create a blank board
pcb = BOARD()

# create a new module, it's parent is our previously created pcb
module = FOOTPRINT(pcb)
module.SetReference("FPC"+str(pads))   # give it a reference name
module.Reference().SetPos0(wxPointMM(-1,-1))
pcb.Add(module)             # add it to our pcb
m_pos = wxPointMM(50,50)
module.SetPosition(m_pos)

# create a pad array and add it to the module


def smdRectPad(module,size,pos,name):
    pad = PAD(module)
    pad.SetSize(size)
    pad.SetShape(PAD_SHAPE_RECT)
    pad.SetAttribute(PAD_ATTRIB_SMD)
    pad.SetLayerSet(pad.SMDMask())
    pad.SetPos0(pos)
    pad.SetName(name)
    return pad

for n in range (0,pads):
    pad = smdRectPad(module,size_025_160mm,wxPointMM(0.5*n,0),str(n+1))
    module.Add(pad)


pad_s0 = smdRectPad(module,size_150_200mm,wxPointMM(-1.6,1.3),"0")
pad_s1 = smdRectPad(module,size_150_200mm,wxPointMM((pads-1)*0.5+1.6,1.3),"0")
module.Add(pad_s0)
module.Add(pad_s1)

e = FP_SHAPE(module)
e.SetStart0(wxPointMM(-1,0))
e.SetEnd0(wxPointMM(0,0))
e.SetWidth(FromMM(0.2))
e.SetLayer(F_SilkS)
e.SetShape(S_SEGMENT)
module.Add(e)

# save the footprint to disk
fpid = LIB_ID("", "FPC"+str(pads))   #the curr lin nickname (empty) and the name in library
module.SetFPID( fpid )

lib_name = "fpc40.pretty"   #lib_path is actually a path, not a file
dst_type = IO_MGR.GuessPluginTypeFromLibPath( lib_name );
dst_plugin = IO_MGR.PluginFind( dst_type )

if os.path.exists(lib_name) == False:
    dst_plugin.CreateLibrary(lib_name)

dst_plugin.FootprintSave(lib_name,module)
