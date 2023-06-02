
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

        for draw in pcb.GetDrawings():
            pcb.RemoveNative(draw)


class testundoredo2(ActionPlugin):

    def defaults(self):
        self.name = "Test Undo/Redo: Generate random content"
        self.category = "Test Undo/Redo"
        self.description = ""

    def createFPCXFootprint(self,pads):
        size_025_160mm = VECTOR2I_MM(0.25,1.6)
        size_150_200mm = VECTOR2I_MM(1.50,2.0)
        # create a new footprint, its parent is our previously created pcb
        footprint = FOOTPRINT(self.pcb)
        footprint.SetReference("FPC"+str(pads))   # give it a reference name
        footprint.Reference().SetPosition(VECTOR2I_MM(-1,-1))
        self.pcb.Add(footprint)             # add it to our pcb
        m_pos = VECTOR2I_MM(0,0)        #random.randint(10,200),random.randint(10,200))
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
            pad = smdRectPad( footprint,size_025_160mm, VECTOR2I_MM(0.5*n,0), str(n+1) )
            footprint.Add(pad)


        pad_s0 = smdRectPad(footprint,size_150_200mm,VECTOR2I_MM(-1.6,1.3), "0")
        pad_s1 = smdRectPad(footprint,size_150_200mm,VECTOR2I_MM((pads-1)*0.5+1.6,1.3), "0")
        footprint.Add(pad_s0)
        footprint.Add(pad_s1)

        e = PCB_SHAPE(footprint)
        e.SetStart(VECTOR2I_MM(-1,0))
        e.SetEnd(VECTOR2I_MM(0,0))
        e.SetWidth(FromMM(0.2))
        e.SetLayer(F_SilkS)
        e.SetShape(S_SEGMENT)
        footprint.Add(e)
        footprint.SetPosition(VECTOR2I_MM(random.randint(20,200),random.randint(20,150)))
        return footprint



    def Run(self):
        self.pcb = GetBoard()
        random.seed()

        for i in range(10):
            seg = PCB_SHAPE()
            seg.SetLayer( random.choice([Edge_Cuts,Cmts_User,Eco1_User,Eco2_User]) )
            seg.SetStart( VECTOR2I_MM( random.randint(10,100),
                                random.randint(10,100) ) )
            seg.SetEnd( VECTOR2I_MM( random.randint(10,100),
                                random.randint(10,100) ) )
            self.pcb.Add( seg )

            if i%2 == 0:
                t = PCB_TRACK(None)
            else:
                t = PCB_VIA(None)
                #t.SetLayerPair(segments['layerPair'][0],segments['layerPair'][1])
                t.SetViaType(VIATYPE_THROUGH)
                t.SetDrill(FromMM(random.randint(1,20)/10.0))

            t.SetStart(VECTOR2I_MM(random.randint(100,150),random.randint(100,150)))
            t.SetEnd(VECTOR2I_MM(random.randint(100,150),random.randint(100,150)))
            t.SetWidth(FromMM(random.randint(1,15)/10.0))
            t.SetLayer(random.choice([F_Cu,B_Cu]))
            self.pcb.Add(t)

            self.createFPCXFootprint(random.randint(2,40))


class testundoredo3(ActionPlugin):

    def defaults(self):
        self.name = "Test Undo/Redo: Move elements randomly"
        self.category = "Test Undo/Redo"
        self.description = ""

    def Run(self):
        pcb = GetBoard()

        for i in range(0,pcb.GetAreaCount()):
            area = pcb.GetArea(i)
            area.Move(VECTOR2I_MM(random.randint(-20,20),random.randint(-20,20)))

        for footprint in pcb.GetFootprints():
            footprint.Move(VECTOR2I_MM(random.randint(-20,20),random.randint(-20,20)))
            if random.randint(0,10) > 5:
                footprint.Flip(footprint.GetPosition(), True)

        for track in pcb.GetTracks():
            track.Move(VECTOR2I_MM(random.randint(-20,20),random.randint(-20,20)))

        for draw in pcb.GetDrawings():
            draw.Move(VECTOR2I_MM(random.randint(-20,20),random.randint(-20,20)))



testundoredo0().register()
testundoredo1().register()
testundoredo2().register()
testundoredo3().register()
