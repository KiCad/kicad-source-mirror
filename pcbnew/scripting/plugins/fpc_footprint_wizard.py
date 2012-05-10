#!/usr/bin/python

from pcbnew import *

class FPCFootprintWizard(FootprintWizardPlugin):
    def __init__(self):
        FootprintWizardPlugin.__init__(self)
        self.name = "FPC"
        self.description = "FPC Footprint Wizard"
        self.parameters = {
             "Pads":
                {"n":40,"pitch":0.5,"width":0.25,"height":1.6},
             "Shield":
                {"shield_to_pad":1.6,"from_top":1.3,"width":1.5,"height":2}
        }
  
    def smdRectPad(self,module,size,pos,name):
            pad = D_PAD(module)
	    # print "smdRectPad( size=",size,"pos=",pos,"name=",name,")"
            pad.SetSize(size)
            pad.SetShape(PAD_RECT)
            pad.SetAttribute(PAD_SMD)
            pad.SetLayerMask(PAD_SMD_DEFAULT_LAYERS)
            pad.SetPos0(pos)
	    pad.SetPosition(pos)
            pad.SetPadName(name)
            return pad
        
    def BuildFootprint(self):
        
        pads            = self.parameters["Pads"]["n"]
        pad_width       = self.parameters["Pads"]["width"]
        pad_height      = self.parameters["Pads"]["height"]
        pad_pitch       = self.parameters["Pads"]["pitch"]
        shl_width       = self.parameters["Shield"]["width"]
        shl_height      = self.parameters["Shield"]["height"]
        shl_to_pad      = self.parameters["Shield"]["shield_to_pad"]
        shl_from_top    = self.parameters["Shield"]["from_top"]
        
        size_pad = wxSizeMM(pad_width,pad_height)
        size_shld = wxSizeMM(shl_width,shl_height)
       

        # create a new module, it's parent is our previously created pcb
        module = MODULE(None)
        module.SetReference("FPC"+str(pads))   # give it a reference name
        module.m_Reference.SetPos0(wxPointMM(-1,-1))
	module.m_Reference.SetPosition(wxPointMM(-1,-1))
        # create a pad array and add it to the module

        for n in range (0,pads):
            pad = self.smdRectPad(module,size_pad,wxPointMM(pad_pitch*n,0),str(n+1))
            module.Add(pad)
          

        pad_s0 = self.smdRectPad(module,
                            size_shld,
                            wxPointMM(-shl_to_pad,shl_from_top),
                            "0")
        pad_s1 = self.smdRectPad(module,
                            size_shld,
                            wxPointMM((pads-1)*pad_pitch+shl_to_pad,shl_from_top),
                            "0")        
                            
        module.Add(pad_s0)
        module.Add(pad_s1)

        e = EDGE_MODULE(module)
        e.SetStart0(wxPointMM(-1,0))
        e.SetEnd0(wxPointMM(0,0))
        e.SetWidth(FromMM(0.2))
        e.SetLayer(EDGE_LAYER)
        e.SetShape(S_SEGMENT)
        module.Add(e)

        # save the PCB to disk
        module.SetLibRef("FPC"+str(pads))
                
        self.module = module
	# print "Module built and set:", module

# create our footprint wizard
fpc_wizard = FPCFootprintWizard() 

# register it into pcbnew
fpc_wizard.register()
