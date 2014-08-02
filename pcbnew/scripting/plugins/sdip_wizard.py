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
#

from __future__ import division
import pcbnew

import HelpfulFootprintWizardPlugin as HFPW
import PadArray as PA


class RowedGridArray(PA.PadGridArray):

    def NamingFunction(self, x, y):
        if (x % 2) == 0: # even row, count up
            return (x * self.ny) + y + 1;
        else: # odd row, count down
            return (self.ny * (x + 1)) - y;

class RowedFootprint(HFPW.HelpfulFootprintWizardPlugin):

    def GenerateParameterList(self):

        # defaults for a DIP package
        self.AddParam("Pads", "n", self.uNatural, 24)
        self.AddParam("Pads", "silk screen inside", self.uBool, False)
        self.AddParam("Pads", "row count", self.uNatural, 2)

    def CheckParameters(self):
        self.CheckParamPositiveInt("Pads", "*row count")
        self.CheckParamPositiveInt("Pads", "*n", is_multiple_of = self.parameters["Pads"]["*row count"])
        self.CheckParamBool("Pads", "*silk screen inside") #can do this internally to parameter manager?

    def BuildThisFootprint(self):

        pads = self.parameters["Pads"]

        num_pads = pads["*n"]

        pad_length = pads["pad length"]
        pad_width = pads["pad width"]
        row_pitch = pads["row spacing"]
        pad_pitch = pads["pad pitch"]
        num_rows = pads["*row count"]

        pads_per_row = num_pads // num_rows

        row_length = pad_pitch * (pads_per_row - 1) #fenceposts

        # add in the pads
        pad = self.GetPad()

        pin1Pos = pcbnew.wxPoint(-((num_rows - 1) * row_pitch) / 2, -row_length / 2)

        array = RowedGridArray(pad, num_rows, pads_per_row, row_pitch, pad_pitch, pin1Pos)
        array.AddPadsToModule()

        # draw the Silk Screen

        pad_length = pads["pad length"]
        pad_width = pads["pad width"]

        ssXOffset = -pad_length / 2 - pads["outline x margin"]
        ssYOffset = -pad_width / 2 - pads["outline y margin"]


        if pads["*silk screen inside"]:
            ssXOffset *= -1

        ssX = -pin1Pos.x - ssXOffset
        ssY = -pin1Pos.y - ssYOffset


        self.DrawBox(ssX, ssY)

        #reference and value
        textSize = pcbnew.FromMM(0.8)

        self.draw.Value(0, - ssY - textSize, textSize)
        self.draw.Reference(0, ssY + textSize, textSize)


class SDIPWizard(RowedFootprint):

    def GetName(self):
        return "S/DIP"

    def GetDescription(self):
        return "Single/Dual Inline Package Footprint Wizard"

    def GenerateParameterList(self):
        RowedFootprint.GenerateParameterList(self)

        self.AddParam("Pads", "pad pitch", self.uMils, 100)
        self.AddParam("Pads", "pad width", self.uMils, 60)
        self.AddParam("Pads", "pad length", self.uMils, 150)
        self.AddParam("Pads", "row spacing", self.uMils, 300)
        self.AddParam("Pads", "drill size", self.uMM, 1)
        self.AddParam("Pads", "outline x margin", self.uMM, 0.5)
        self.AddParam("Pads", "outline y margin", self.uMM, 1)

    def GetReference(self):

        rows = self.parameters["Pads"]["*row count"]

        if  rows == 1:
            name = "SIP"
        elif rows == 2:
            name = "DIP"
        else: # triple and up aren't really a thing, but call it something!
            name = "xIP"

        return "%s %d" % (name, self.parameters["Pads"]["*n"])

    def GetPad(self):
        pad_length = self.parameters["Pads"]["pad length"]
        pad_width = self.parameters["Pads"]["pad width"]
        drill = self.parameters["Pads"]["drill size"]
        return PA.PadMaker(self.module).THPad(pad_width, pad_length, drill, shape = pcbnew.PAD_OVAL)

    def DrawBox(self, ssX, ssY):

        if self.parameters["Pads"]["*row count"] == 2:

            #  ----------
            #  |8 7 6 5 |
            #  >        |
            #  |1 2 3 4 |
            #  ----------

            # draw the notch
            notchWidth = pcbnew.FromMM(3)
            notchHeight = pcbnew.FromMM(1)

            self.draw.NotchedBox(0, 0, ssX*2, ssY*2, notchWidth, notchHeight)
        else:
            #  -----------------
            #  |1|2 3 4 5 6 7 8|
            #  -----------------
            self.draw.Box(ssX*2, ssY*2)

            #line between pin1 and pin2
            pad_pitch = self.parameters["Pads"]["pad pitch"];
            self.draw.HLine(-ssX, pin1Pos.y + pad_pitch/2, ssX * 2)

        return ssX, ssY

SDIPWizard().register()


class SOICWizard(RowedFootprint):

    def GetName(self):
        return "SOIC"

    def GetDescription(self):
        return "SOIC, MSOP, SSOP, TSSOP, etc, footprint wizard"

    def GetReference(self):
        return "%s %d" % ("SOIC", self.parameters["Pads"]["*n"])

    def GenerateParameterList(self):
        RowedFootprint.GenerateParameterList(self)

        #and override some of them
        self.AddParam("Pads", "pad pitch", self.uMM, 1.27)
        self.AddParam("Pads", "pad width", self.uMM, 0.6)
        self.AddParam("Pads", "pad length", self.uMM, 2.2)
        self.AddParam("Pads", "row spacing", self.uMM, 5.2)

        self.AddParam("Pads", "outline x margin", self.uMM, 0.5)
        self.AddParam("Pads", "outline y margin", self.uMM, 0.5)

    def GetPad(self):
        pad_length = self.parameters["Pads"]["pad length"]
        pad_width = self.parameters["Pads"]["pad width"]
        return PA.PadMaker(self.module).SMDPad(pad_width, pad_length, shape = pcbnew.PAD_RECT)

    def DrawBox(self, ssX, ssY):

        #  ----------
        #  |8 7 6 5 |
        #  |1 2 3 4 |
        #  \---------

        self.draw.BoxWithDiagonalAtCorner(0, 0, ssX*2, ssY*2, pcbnew.FromMM(1))

SOICWizard().register()
