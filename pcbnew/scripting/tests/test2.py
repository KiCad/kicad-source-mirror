pcb = pcbnew.GetBoard()

m = pcb.m_Modules.item()

while m:
	print m.GetReference(),"(",m.GetValue(),") at ", m.GetPosition()
        m.SetValue("pepe")
        p  = m.m_Pads.item()
	while p:
		print "    pad",p.GetPadName(), "at",p.GetPosition()
		p = p.Next()

	m = m.Next()		

