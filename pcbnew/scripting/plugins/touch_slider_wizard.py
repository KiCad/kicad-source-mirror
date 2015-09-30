#!/usr/bin/python

#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2012-2014 KiCad Developers, see change_log.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#

from pcbnew import *

class TouchSliderWizard(FootprintWizardPlugin):
    def __init__(self):
        FootprintWizardPlugin.__init__(self)
        self.name = "Touch Slider"
        self.description = "Capacitive Touch Slider Wizard"
        self.parameters = {
             "Pads":
                {"*steps":4, # not internal (measurement) units preceded by "*"
                 "*bands":2,
                 "width":           FromMM(10),
                 "length":          FromMM(50),
                 "clearance":       FromMM(1)
                }

        }
        self.ClearErrors()

    # build a rectangular pad
    def smdRectPad(self,module,size,pos,name):
            pad = D_PAD(module)
            pad.SetSize(size)
            pad.SetShape(PAD_SHAPE_RECT)
            pad.SetAttribute(PAD_ATTRIB_SMD)
            pad.SetLayerSet(pad.ConnSMDMask())
            pad.SetPos0(pos)
            pad.SetPosition(pos)
            pad.SetPadName(name)
            return pad


    def smdTrianglePad(self,module,size,pos,name,up_down=1,left_right=0):
            pad = D_PAD(module)
            pad.SetSize(wxSize(size[0],size[1]))
            pad.SetShape(PAD_SHAPE_TRAPEZOID)
            pad.SetAttribute(PAD_ATTRIB_SMD)
            pad.SetLayerSet(pad.ConnSMDMask())
            pad.SetPos0(pos)
            pad.SetPosition(pos)
            pad.SetPadName(name)
            pad.SetDelta(wxSize(left_right*size[1],up_down*size[0]))
            return pad


    # This method checks the parameters provided to wizard and set errors
    def CheckParameters(self):
        p = self.parameters
        steps            = p["Pads"]["*steps"]
        errors = ""
        if (steps<1):
            self.parameter_errors["Pads"]["*steps"]="Must be positive"
            errors +="Pads/*steps has wrong value"
        p["Pads"]["*steps"] = int(pads)  # make sure it stays as int (default is float)

        bands            = p["Pads"]["*bands"]

        if (bands<1):
            self.parameter_errors["Pads"]["*bands"]="Must be positive"
            errors +="Pads/*bands has wrong value"
        p["Pads"]["*bands"] = int(bands)  # make sure it stays as int (default is float)


        touch_width       = p["Pads"]["width"]
        touch_length      = p["Pads"]["length"]
        touch_clearance   = p["Pads"]["clearance"]


        return errors

    # The start pad is made of a rectangular pad plus a couple of
    # triangular pads facing tips on the middle/right of the first
    # rectangular pad
    def AddStartPad(self,position,touch_width,step_length,clearance,name):
        module = self.module
        step_length = step_length - clearance
        size_pad = wxSize(step_length/2.0+(step_length/3),touch_width)
        pad = self.smdRectPad(module,size_pad,position-wxPoint(step_length/6,0),name)
        module.Add(pad)

        size_pad = wxSize(step_length/2.0,touch_width)

        tp = self.smdTrianglePad(module,wxSize(size_pad[0],size_pad[1]/2),
                                 position+wxPoint(size_pad[0]/2,size_pad[1]/4),
                                name)
        module.Add(tp)
        tp = self.smdTrianglePad(module,wxSize(size_pad[0],size_pad[1]/2),
                                 position+wxPoint(size_pad[0]/2,-size_pad[1]/4),
                                name
                                ,-1)
        module.Add(tp)

    # compound a "start pad" shape plus a triangle on the left, pointing to
    # the previous touch-pad
    def AddMiddlePad(self,position,touch_width,step_length,clearance,name):
        module = self.module
        step_length = step_length - clearance
        size_pad = wxSize(step_length/2.0,touch_width)

        size_pad = wxSize(step_length/2.0,touch_width)
        pad = self.smdRectPad(module,size_pad,position,name)
        module.Add(pad)

        tp = self.smdTrianglePad(module,wxSize(size_pad[0],size_pad[1]/2),
                                 position+wxPoint(size_pad[0]/2,size_pad[1]/4),
                                name)
        module.Add(tp)
        tp = self.smdTrianglePad(module,wxSize(size_pad[0],size_pad[1]/2),
                                 position+wxPoint(size_pad[0]/2,-size_pad[1]/4),
                                name
                                ,-1)
        module.Add(tp)

        tp = self.smdTrianglePad(module,wxSize(size_pad[0],size_pad[1]/2),
                                        position+wxPoint(-size_pad[0],0),
                                        name,
                                        0,
                                        -1)
        module.Add(tp)


    def AddFinalPad(self,position,touch_width,step_length,clearance,name):
        module = self.module
        step_length = step_length - clearance
        size_pad = wxSize(step_length/2.0,touch_width)

        pad = self.smdRectPad(module,
                              wxSize(size_pad[0]+(step_length/3),size_pad[1]),
                              position+wxPoint(step_length/6,0),
                              name)
        module.Add(pad)

        tp = self.smdTrianglePad(module,wxSize(size_pad[0],size_pad[1]/2),
                                        position+wxPoint(-size_pad[0],0),
                                        name,
                                        0,
                                        -1)
        module.Add(tp)

    def AddStrip(self,pos,steps,touch_width,step_length,touch_clearance):
        self.AddStartPad(pos,touch_width,step_length,touch_clearance,"1")

        for n in range(2,steps):
            pos = pos + wxPoint(step_length,0)
            self.AddMiddlePad(pos,touch_width,step_length,touch_clearance,str(n))

        pos = pos + wxPoint(step_length,0)
        self.AddFinalPad(pos,touch_width,step_length,touch_clearance,str(steps))

    # build the footprint from parameters
    def BuildThisFootprint(self):

        module = MODULE(None) # create a new module
        self.module = module

        p = self.parameters
        steps             = int(p["Pads"]["*steps"])
        bands             = int(p["Pads"]["*bands"])
        touch_width       = p["Pads"]["width"]
        touch_length      = p["Pads"]["length"]
        touch_clearance   = p["Pads"]["clearance"]

        step_length = float(touch_length) / float(steps)

        size_text = wxSize( FromMM( 1), FromMM( 1) );
        module.SetReference("TS"+str(steps))   # give it a reference name
        module.Reference().SetPos0(wxPointMM(0,-2))
        module.Reference().SetTextPosition(module.Reference().GetPos0())
        module.Reference().SetSize( size_text );

        module.SetValue("Val**")   # give it a value
        module.Value().SetPos0(wxPointMM(0,-3.2))
        module.Value().SetTextPosition(module.Value().GetPos0())
        module.Value().SetSize( size_text );

        # starting pad
        pos = wxPointMM(0,0)
        band_width = touch_width/bands

        for b in range(bands):

            self.AddStrip(pos,steps,band_width,step_length,touch_clearance)
            pos+=wxPoint(0,band_width)

        fpid = FPID(self.module.GetReference())   #the name in library
        module.SetFPID( fpid )

def register():
    # create our footprint wizard
    touch_slider_wizard = TouchSliderWizard()

    # register it into pcbnew
    touch_slider_wizard.register()

    return touch_slider_wizard


