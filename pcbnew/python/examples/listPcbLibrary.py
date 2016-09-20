#!/usr/bin/env python
from pcbnew import *

lst = FootprintEnumerate("/usr/share/kicad/modules/sockets.mod")

for name in lst:
    m = FootprintLoad("/usr/share/kicad/modules/sockets.mod",name)
    print name,"->",m.GetLibRef(), m.GetReference()

    for p in m.Pads():
        print "\t",p.GetPadName(),p.GetPosition(),p.GetPos0(), p.GetOffset()

