
from pcbnew import *
import random
import pprint


class testundoredo0(ActionPlugin):

    def defaults(self):
        self.name = "Test Undo/Redo: Remove footprints"
        self.category = "Test Undo/Redo"
        self.description = ""

    def Run(self):
        pcb = GetBoard()

        for module in pcb.GetFootprints():
            pcb.RemoveNative(module)


class testundoredo1(ActionPlugin):

    def defaults(self):
        self.name = "Test Undo/Redo: Remove all board items"
        self.category = "Test Undo/Redo"
        self.description = ""

    def Run(self):
        pcb = GetBoard()

        while pcb.GetAreaCount() > 0:
            area = pcb.GetArea(0)
            pcb.RemoveNative(area)

        for module in pcb.GetFootprints():
            pcb.RemoveNative(module)

        for track in pcb.GetTracks():
            pcb.RemoveNative(track)

        for draw in pcb.m_Drawings:
            pcb.RemoveNative(draw)


class testundoredo2(ActionPlugin):

    def defaults(self):
        self.name = "Test Undo/Redo: Generate random content"
        self.category = "Test Undo/Redo"
        self.description = ""

    def createFPCXFootprint(self,pads):
        size_025_160mm = wxSizeMM(0.25,1.6)
        size_150_200mm = wxSizeMM(1.50,2.0)
        # create a new footprint, its parent is our previously created pcb
        footprint = FOOTPRINT(self.pcb)
        footprint.SetReference("FPC"+str(pads))   # give it a reference name
        footprint.Reference().SetPosition(wxPointMM(-1,-1))
        self.pcb.Add(footprint)             # add it to our pcb
        m_pos = wxPointMM(0,0)#random.randint(10,200),random.randint(10,200))
        footprint.SetPosition(m_pos)

        # create a pad array and add it to the footprint


        def smdRectPad(footprint,size,pos,name):
            pad = PAD(footprint)
            pad.SetSize(size)
            pad.SetShape(PAD_SHAPE_RECT)
            pad.SetAttribute(PAD_ATTRIB_SMD)
            pad.SetLayerSet(pad.SMDMask())
            pad.SetPosition(pos)
            pad.SetPadName(name)
            return pad

        for n in range (0,pads):
            pad = smdRectPad(footprint,size_025_160mm,wxPointMM(0.5*n,0),str(n+1))
            footprint.Add(pad)


        pad_s0 = smdRectPad(footprint,size_150_200mm,wxPointMM(-1.6,1.3),"0")
        pad_s1 = smdRectPad(footprint,size_150_200mm,wxPointMM((pads-1)*0.5+1.6,1.3),"0")
        footprint.Add(pad_s0)
        footprint.Add(pad_s1)

        e = EDGE_MODULE(footprint)
        e.SetStart0(wxPointMM(-1,0))
        e.SetEnd0(wxPointMM(0,0))
        e.SetWidth(FromMM(0.2))
        e.SetLayer(F_SilkS)
        e.SetShape(S_SEGMENT)
        module.Add(e)
        module.SetPosition(wxPointMM(random.randint(20,200),random.randint(20,150)))
        return module



    def Run(self):
        self.pcb = GetBoard()
        random.seed()

        for i in range(10):
            seg = DRAWSEGMENT()
            seg.SetLayer( random.choice([Edge_Cuts,Cmts_User,Eco1_User,Eco2_User]) )
            seg.SetStart( wxPointMM( random.randint(10,100),
                                random.randint(10,100) ) )
            seg.SetEnd( wxPointMM( random.randint(10,100),
                                random.randint(10,100) ) )
            self.pcb.Add( seg )

            if i%2 == 0:
                t = TRACK(None)
            else:
                t = VIA(None)
                #t.SetLayerPair(segments['layerPair'][0],segments['layerPair'][1])
                t.SetViaType(VIA_THROUGH)
                t.SetDrill(FromMM(random.randint(1,20)/10.0))
            t.SetStart(wxPointMM(random.randint(100,150),random.randint(100,150)))
            t.SetEnd(wxPointMM(random.randint(100,150),random.randint(100,150)))
            t.SetWidth(FromMM(random.randint(1,15)/10.0))
            t.SetLayer(random.choice([F_Cu,B_Cu]))
            self.pcb.Add(t)

            self.createFPCXModule(random.randint(2,40))


class testundoredo3(ActionPlugin):

    def defaults(self):
        self.name = "Test Undo/Redo: Move elements randomly"
        self.category = "Test Undo/Redo"
        self.description = ""

    def Run(self):
        pcb = GetBoard()

        for i in range(0,pcb.GetAreaCount()):
            area = pcb.GetArea(i)
            area.Move(wxPointMM(random.randint(-20,20),random.randint(-20,20)))

        for module in pcb.GetFootprints():
            module.Move(wxPointMM(random.randint(-20,20),random.randint(-20,20)))
            if random.randint(0,10) > 5:
                module.Flip(module.GetPosition())

        for track in pcb.GetTracks():
            track.Move(wxPointMM(random.randint(-20,20),random.randint(-20,20)))

        for draw in pcb.m_Drawings:
            draw.Move(wxPointMM(random.randint(-20,20),random.randint(-20,20)))



testundoredo0().register()
testundoredo1().register()
testundoredo2().register()
testundoredo3().register()
