from __future__ import print_function

import pcbnew

pcb = pcbnew.GetBoard()

for m in pcb.GetFootprints():
    print(m.GetReference(), "(", m.GetValue(), ") at ", m.GetPosition())
    for p in m.Pads():
        print("    pad", p.GetName(), "at", p.GetPosition())
