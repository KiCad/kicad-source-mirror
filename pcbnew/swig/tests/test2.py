import pcbnew

pcb = pcbnew.GetBoard()

for m in pcb.GetModules():
    print m.GetReference(),"(",m.GetValue(),") at ", m.GetPosition()
    for p in m.Pads():
        print "    pad",p.GetPadName(), "at",p.GetPosition()
