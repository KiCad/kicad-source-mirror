pcb = pcbnew.GetBoard()

m = pcb.m_Modules

while m:
	print m.GetPosition()
        p  = m.m_Pads
	while p:
		print "p=>",p.GetPosition(),p.GetPadName()
		print p.GetPosition()
		p = p.Next()
	m = m.Next()		


