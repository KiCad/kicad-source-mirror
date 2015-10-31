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

import math

import pcbnew
import HelpfulFootprintWizardPlugin as HFPW
import PadArray as PA


class circular_pad_array_wizard(HFPW.HelpfulFootprintWizardPlugin):

    def GetName(self):
        return "Circular Pad Array"

    def GetDescription(self):
        return "Circular array of pads"

    def GenerateParameterList(self):

        self.AddParam("Pads", "n", self.uNatural, 6)
        self.AddParam("Pads", "pad width", self.uMM, 1.5)
        self.AddParam("Pads", "drill", self.uMM, 1)
        self.AddParam("Pads", "circle diameter", self.uMM, 5)
        self.AddParam("Pads", "first pad angle", self.uNatural, 0)
        self.AddParam("Pads", "number clockwise", self.uBool, True)
        self.AddParam("Pads", "first pad number", self.uNatural, 1)

    def CheckParameters(self):

        self.CheckParamInt("Pads", "*n")
        self.CheckParamInt("Pads", "*first pad number")
        self.CheckParamBool("Pads", "*number clockwise")

    def GetValue(self):
        pins = self.parameters["Pads"]["*n"]
        return "CPA_%d" % pins

    def BuildThisFootprint(self):

        prm = self.parameters['Pads']

        pad_size = prm['pad width']

        pad = PA.PadMaker(self.module).THPad(
            prm['pad width'], prm['pad width'], prm['drill'])

        array = PA.PadCircleArray(
            pad, prm['*n'], prm['circle diameter'] / 2,
            angle_offset=prm["*first pad angle"],
            centre=pcbnew.wxPoint(0, 0),
            clockwise=prm["*number clockwise"])

        array.SetFirstPadInArray(prm["*first pad number"])

        array.AddPadsToModule(self.draw)

        body_radius = (prm['circle diameter'] + prm['pad width'])/2 + self.draw.GetLineTickness()
        self.draw.Circle(0, 0, body_radius)

        text_size = self.GetTextSize()  # IPC nominal
        thickness = self.GetTextThickness()
        textposy = body_radius + self.draw.GetLineTickness()/2 + self.GetTextSize()/2 + thickness
        self.draw.Value( 0, textposy, text_size )
        self.draw.Reference( 0, -textposy, text_size )



circular_pad_array_wizard().register()
