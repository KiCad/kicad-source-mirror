import pcbnew

pcb = pcbnew.GetBoard()

for m in pcb.GetModules():
    print m.GetPosition()
    for p in m.Pads():
        print "p=>",p.GetPosition(),p.GetPadName()
        print p.GetPosition()


