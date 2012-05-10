#!/usr/bin/python

from pcbnew import *

class FPCFootprintWizard(FootprintWizardPlugin):
    def __init__(self):
        FootprintWizardPlugin.__init__(self)
        self.name = "FPC"
        self.description = "FPC Footprint Wizard"
        self.parameters = {
             "Pads":
                {"n":40,"pitch":FromMM(0.5),
                 "width":FromMM(0.25),"height":FromMM(1.6)},
             "Shield":
                {"shield_to_pad":FromMM(1.6),"from_top":FromMM(1.3),
                 "width":FromMM(1.5),"height":FromMM(2)},
                
        }
  
    def GetParameterValues(self,page_n):
        name = self.GetParameterPageName(page_n)
        values = self.parameters[name].values()
        str_values = map( lambda x: str(x) , values)
        
        print values,str_values
        return str_values
    
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
        
        p = self.parameters
        pads            = p["Pads"]["n"]
        pad_width       = p["Pads"]["width"]
        pad_height      = p["Pads"]["height"]
        pad_pitch       = p["Pads"]["pitch"]
        shl_width       = p["Shield"]["width"]
        shl_height      = p["Shield"]["height"]
        shl_to_pad      = p["Shield"]["shield_to_pad"]
        shl_from_top    = p["Shield"]["from_top"]
        
        size_pad = wxSize(pad_width,pad_height)
        size_shld = wxSize(shl_width,shl_height)
       

        # create a new module
        module = MODULE(None)
        module.SetReference("FPC"+str(pads))   # give it a reference name
        module.m_Reference.SetPos0(wxPointMM(-1,-2))
        module.m_Reference.SetPosition(wxPointMM(-1,-2))
        
        # create a pad array and add it to the module
        for n in range (0,pads):
            pad = self.smdRectPad(module,size_pad,wxPoint(pad_pitch*n,0),str(n+1))
            module.Add(pad)
          

        pad_s0 = self.smdRectPad(module,
                            size_shld,
                            wxPoint(-shl_to_pad,shl_from_top),
                            "0")
        pad_s1 = self.smdRectPad(module,
                            size_shld,
                            wxPoint((pads-1)*pad_pitch+shl_to_pad,shl_from_top),
                            "0")        
                            
        module.Add(pad_s0)
        module.Add(pad_s1)

        e = EDGE_MODULE(module)
        e.SetStartEnd(wxPointMM(-1,0),wxPointMM(0,0))
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
