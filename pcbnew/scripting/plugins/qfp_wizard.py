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

import HelpfulFootprintWizardPlugin
import PadArray as PA


class QFPWizard(HelpfulFootprintWizardPlugin.HelpfulFootprintWizardPlugin):

    def GetName(self):
        return "QFP"

    def GetDescription(self):
        return "QFP Footprint Wizard"

    def GenerateParameterList(self):
        self.AddParam("Pads", "n", self.uNatural, 100)
        self.AddParam("Pads", "pad pitch", self.uMM, 0.5)
        self.AddParam("Pads", "pad width", self.uMM, 0.25)
        self.AddParam("Pads", "pad length", self.uMM, 1.5)
        self.AddParam("Pads", "vertical pitch", self.uMM, 15)
        self.AddParam("Pads", "horizontal pitch", self.uMM, 15)
        self.AddParam("Pads", "oval", self.uBool, True)

        self.AddParam("Pads", "package width", self.uMM, 14)
        self.AddParam("Pads", "package height", self.uMM, 14)

    def CheckParameters(self):

        self.CheckParamInt("Pads", "*n", is_multiple_of=4)
        self.CheckParamBool("Pads", "*oval")

    def GetValue(self):
        return "QFP %d" % self.parameters["Pads"]["*n"]

    def BuildThisFootprint(self):

        pads = self.parameters["Pads"]

        pad_pitch = pads["pad pitch"]
        pad_length = self.parameters["Pads"]["pad length"]
        pad_width = self.parameters["Pads"]["pad width"]

        v_pitch = pads["vertical pitch"]
        h_pitch = pads["horizontal pitch"]

        pads_per_row = pads["*n"] // 4

        row_len = (pads_per_row - 1) * pad_pitch

        pad_shape = pcbnew.PAD_OVAL if pads["*oval"] else pcbnew.PAD_RECT

        h_pad = PA.PadMaker(self.module).SMDPad(
            pad_width, pad_length, shape=pad_shape)
        v_pad = PA.PadMaker(self.module).SMDPad(
            pad_length, pad_width, shape=pad_shape)

        #left row
        pin1Pos = pcbnew.wxPoint(-h_pitch / 2, 0)
        array = PA.PadLineArray(h_pad, pads_per_row, pad_pitch, True,
                                pin1Pos)
        array.SetFirstPadInArray(1)
        array.AddPadsToModule(self.draw)

        #bottom row
        pin1Pos = pcbnew.wxPoint(0, v_pitch / 2)
        array = PA.PadLineArray(v_pad, pads_per_row, pad_pitch, False,
                                pin1Pos)
        array.SetFirstPadInArray(pads_per_row + 1)
        array.AddPadsToModule(self.draw)

        #right row
        pin1Pos = pcbnew.wxPoint(h_pitch / 2, 0)
        array = PA.PadLineArray(h_pad, pads_per_row, -pad_pitch, True,
                                pin1Pos)
        array.SetFirstPadInArray(2*pads_per_row + 1)
        array.AddPadsToModule(self.draw)

        #top row
        pin1Pos = pcbnew.wxPoint(0, -v_pitch / 2)
        array = PA.PadLineArray(v_pad, pads_per_row, -pad_pitch, False,
                                pin1Pos)
        array.SetFirstPadInArray(3*pads_per_row + 1)
        array.AddPadsToModule(self.draw)

        lim_x = pads["package width"] / 2
        lim_y = pads["package height"] / 2
        inner = (row_len / 2) + pad_pitch

        #top left - diagonal
        self.draw.Line(-lim_x, -inner, -inner, -lim_y)
        # top right
        self.draw.Polyline([(inner, -lim_y), (lim_x, -lim_y), (lim_x, -inner)])
        # bottom left
        self.draw.Polyline([(-inner, lim_y), (-lim_x, lim_y), (-lim_x, inner)])
        # bottom right
        self.draw.Polyline([(inner, lim_y), (lim_x, lim_y), (lim_x, inner)])

        #reference and value
        text_size = pcbnew.FromMM(1.2)  # IPC nominal

        text_offset = v_pitch / 2 + text_size + pad_length / 2

        self.draw.Value(0, -text_offset, text_size)
        self.draw.Reference(0, text_offset, text_size)

QFPWizard().register()
