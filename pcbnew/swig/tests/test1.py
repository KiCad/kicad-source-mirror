from __future__ import print_function

import pcbnew

pcb = pcbnew.GetBoard()

for m in pcb.GetFootprints():
    print(m.GetPosition())
    for p in m.Pads():
        print("p=>", p.GetPosition(), p.GetName())
        print(p.GetPosition())
