#  Copyright 2019-2020 Julian Fellinger
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.

import FootprintWizardBase
import pcbnew
from pcbnew import *

class MutualcapButtonWizard(FootprintWizardBase.FootprintWizard):

    def GetName(self):
        """
        Return footprint name.
        This is specific to each footprint class, you need to implement this
        """
        return "Mutualcap Touch Button"

    def GetDescription(self):
        """
        Return footprint description.
        This is specific to each footprint class, you need to implement this
        """
        return "Wizard for Mutual Capacitance Touch Buttons."

    def GetValue(self):
        return "TouchButton-{width:g}x{height:g}mm".format(
            width = pcbnew.ToMM(self.pads["Width"]),
            height= pcbnew.ToMM(self.pads["Height"])
            )

    def GenerateParameterList(self):
        self.AddParam("Pads", "Width", self.uMM, 12)
        self.AddParam("Pads", "Height", self.uMM, 12)
        self.AddParam("Pads", "Outer electrode width", self.uMM, 1)
        self.AddParam("Pads", "Inner electrode width", self.uMM, 0.1)
        self.AddParam("Pads", "Draw line around button", self.uBool, True)

    @property
    def pads(self):
        return self.parameters["Pads"]


    # build a rectangular pad
    def smdRectPad(self,module,size,pos,name):
        pad = D_PAD(module)
        pad.SetSize(size)
        pad.SetShape(PAD_SHAPE_RECT)
        pad.SetAttribute(PAD_ATTRIB_SMD)

        layerset = pcbnew.LSET()
        layerset.AddLayer(pcbnew.F_Cu)
        pad.SetLayerSet(layerset)

        pad.SetPos0(pos)
        pad.SetPosition(pos)
        pad.SetName(name)
        return pad

    # This method checks the parameters provided to wizard and set errors
    def CheckParameters(self):
        pass

    def BuildThisFootprint(self):
        # o refers to the outer (drive) electrode
        # i refers to the inner (receive) electrode
        w   = self.pads["Width"]
        h   = self.pads["Height"]
        oew = self.pads["Outer electrode width"]
        iew = self.pads["Inner electrode width"]
        drawBox = self.pads["Draw line around button"]

        oFingerCount = int((w-3*oew-iew)/(1.5*oew+iew))
        oBorderWidth = (w-oew-iew-oFingerCount*(1.5*oew+iew))/2
        clearance    = oew/2

        ###border h
        self.module.SetLayer(F_Cu)
        size_pad = pcbnew.wxSize( w, oew )
        self.module.Add(self.smdRectPad(self.module,size_pad, pcbnew.wxPoint(0,(-h/2)+oew/2),str(1)))
        self.module.Add(self.smdRectPad(self.module,size_pad, pcbnew.wxPoint(0,( h/2)-oew/2),str(1)))
        ###border v
        size_pad = pcbnew.wxSize(oBorderWidth, h-2*oew)
        self.module.Add(self.smdRectPad(self.module,size_pad, pcbnew.wxPoint((-w/2)+oBorderWidth/2,0),str(1)))
        self.module.Add(self.smdRectPad(self.module,size_pad, pcbnew.wxPoint(( w/2)-oBorderWidth/2,0),str(1)))

        xPos = -w/2+oBorderWidth+clearance+iew
        oFingerSize = pcbnew.wxSize(oew/2, h-oew-iew-2*clearance-oew)
        iFingerSize = pcbnew.wxSize(iew,h-2*oew-2*clearance)
        #horizontal inner electrode trace
        self.module.Add(self.smdRectPad(self.module,pcbnew.wxSize(w-2*oBorderWidth-2*clearance,iew), pcbnew.wxPoint(0,h/2-oew-clearance-iew/2),str(2)))
        for i in range(0,oFingerCount):
            #inner electrode fingers
            self.module.Add(self.smdRectPad(self.module,iFingerSize, pcbnew.wxPoint(xPos-iew/2 ,0),str(2)))
            #outer electrode fingers
            self.module.Add(self.smdRectPad(self.module,oFingerSize, pcbnew.wxPoint(xPos+oew*0.75,-w/h-oew-iew/2+oew/2),str(1)))
            xPos += oew*1.5+iew

        #rightmost inner electrode finger
        self.module.Add(self.smdRectPad(self.module,iFingerSize, pcbnew.wxPoint(xPos-iew/2,0),str(2)))

        textSize = self.GetTextSize()
        self.draw.Value(0, h/2+textSize, textSize)
        self.draw.Reference(0, -h/2-textSize, textSize)

        #optionally draw silkscreen line around button
        if(drawBox):
            self.draw.Box(0,0,w,h)

MutualcapButtonWizard().register()
