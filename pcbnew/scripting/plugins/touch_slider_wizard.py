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
import HelpfulFootprintWizardPlugin as HFPW


class TouchSliderWizard(HFPW.HelpfulFootprintWizardPlugin):

    def GetName(self):
        """
        Return footprint name.
        This is specific to each footprint class, you need to implement this
        """
        return 'Touch Slider'

    def GetDescription(self):
        """
        Return footprint description.
        This is specific to each footprint class, you need to implement this
        """
        return 'Capacitive Touch Slider wizard'

    def GetValue(self):
        steps = int(self.parameters["Pads"]["*steps"])
        return "TS"+str(steps)

    def GenerateParameterList(self):
        self.AddParam("Pads", "steps", self.uNatural, 4)
        self.AddParam("Pads", "bands", self.uNatural, 2)
        self.AddParam("Pads", "width", self.uMM, 10)
        self.AddParam("Pads", "length", self.uMM, 50)
        self.AddParam("Pads", "clearance", self.uMM, 1)

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
        prms = self.parameters["Pads"]
        steps = prms["*steps"]
        bands = prms["*bands"]

        if steps < 1:
            self.parameter_errors["Pads"]["*steps"]="steps must be positive"
        if bands < 1:
            self.parameter_errors["Pads"]["*bands"]="bands must be positive"

        touch_width       = prms["width"]
        touch_length      = prms["length"]
        touch_clearance   = prms["clearance"]

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
    # FIX ME: the X and Y position of the footprint can be better.
    def BuildThisFootprint(self):
        prm = self.parameters["Pads"]
        steps             = int(prm["*steps"])
        bands             = int(prm["*bands"])
        touch_width       = prm["width"]
        touch_length      = prm["length"]
        touch_clearance   = prm["clearance"]

        step_length = float(touch_length) / float(steps)

        t_size = self.GetTextSize()
        w_text = self.draw.GetLineTickness()
        ypos = touch_width/(bands*2) + t_size/2 + w_text
        self.draw.Value(0, -ypos, t_size)
        ypos += t_size + w_text*2
        self.draw.Reference(0, -ypos, t_size)

        # starting pad
        pos = wxPointMM(0,0)
        band_width = touch_width/bands

        for b in range(bands):
            self.AddStrip(pos,steps,band_width,step_length,touch_clearance)
            pos += wxPoint(0,band_width)

TouchSliderWizard().register()

