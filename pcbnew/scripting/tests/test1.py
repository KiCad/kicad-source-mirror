pcb = pcbnew.GetBoard()

m = pcb.m_Modules.item()

while m:
	print m.GetPosition()
        p  = m.m_Pads.item()
	while p:
		print "p=>",p.GetPosition(),p.GetPadName()
		print p.GetPosition()
		p = p.Next()
	m = m.Next()		


