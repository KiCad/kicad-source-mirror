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


class BGAPadGridArray(PA.PadGridArray):

    def NamingFunction(self, x, y):
        return "%s%d" % (self.AlphaNameFromNumber(y + 1), x + 1)


class BGAWizard(HFPW.HelpfulFootprintWizardPlugin):

    def GetName(self):
        return "BGA"

    def GetDescription(self):
        return "Ball Grid Array Footprint Wizard"

    def GenerateParameterList(self):

        self.AddParam("Pads", "pad pitch", self.uMM, 1)
        self.AddParam("Pads", "pad size", self.uMM, 0.5)
        self.AddParam("Pads", "row count", self.uNatural, 5)
        self.AddParam("Pads", "column count", self.uNatural, 5)
        self.AddParam("Pads", "outline x margin", self.uMM, 1)
        self.AddParam("Pads", "outline y margin", self.uMM, 1)

    def CheckParameters(self):

        self.CheckParamPositiveInt("Pads", "*row count")
        self.CheckParamPositiveInt("Pads", "*column count")


    def GetReference(self):

        pins = self.parameters["Pads"]["*row count"] * self.parameters["Pads"]["*column count"]

        return "BGA %d" % pins


    def GetValuePrefix(self):
        return "U"


    def BuildThisFootprint(self):

        pads = self.parameters["Pads"]

        rows = pads["*row count"]
        cols = pads["*column count"]
        pad_size = pads["pad size"]

        pad_size = pcbnew.wxSize(pad_size, pad_size)

        pad_pitch = pads["pad pitch"]

        # add in the pads
        pad = PA.PadMaker(self.module).SMTRoundPad(pads["pad size"])

        pin1Pos = pcbnew.wxPoint(-((rows - 1) * pad_pitch) / 2,
                                -((cols - 1) * pad_pitch) / 2)

        array = BGAPadGridArray(pad, rows, cols, pad_pitch, pad_pitch, pin1Pos)
        array.AddPadsToModule()

        #box
        ssX = -pin1Pos.x + pads["outline x margin"]
        ssY = -pin1Pos.y + pads["outline y margin"]

        self.draw.BoxWithDiagonalAtCorner(0, 0, ssX*2, ssY*2, pads["outline x margin"])

        #reference and value
        textSize = pcbnew.FromMM(0.8)

        self.draw.Value(0, - ssY - textSize, textSize)
        self.draw.Reference(0, ssY + textSize, textSize)


BGAWizard().register()
