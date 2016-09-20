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

class QFNWizard(HFPW.HelpfulFootprintWizardPlugin):

    def GetName(self):
        return "QFN"

    def GetDescription(self):
        return "Quad Flat No-lead with Exposed Pad footprint wizard"

    def GenerateParameterList(self):
        self.AddParam("Pads", "n", self.uNatural, 100)
        self.AddParam("Pads", "pad pitch", self.uMM, 0.5)
        self.AddParam("Pads", "pad width", self.uMM, 0.25)
        self.AddParam("Pads", "pad length", self.uMM, 1.5)
        self.AddParam("Pads", "oval", self.uBool, True)
        self.AddParam("Pads", "thermal vias", self.uBool, True)
        self.AddParam("Pads", "thermal vias drill", self.uMM, 0.3)
        self.AddParam("Pads", "epad subdiv x", self.uNatural, 2)
        self.AddParam("Pads", "epad subdiv y", self.uNatural, 2)

        self.AddParam("Package", "package width", self.uMM, 14)
        self.AddParam("Package", "package height", self.uMM, 14)
        self.AddParam("Package", "courtyard margin", self.uMM, 1)

    def CheckParameters(self):
        self.CheckParamInt("Pads", "*n", is_multiple_of=4)
        self.CheckParamBool("Pads", "*oval")
        self.CheckParamBool("Pads", "*thermal vias")

    def GetValue(self):
        return "QFN_%d" % self.parameters["Pads"]["*n"]

    def BuildThisFootprint(self):
        pads = self.parameters["Pads"]

        pad_pitch = pads["pad pitch"]
        pad_length = pads["pad length"]
        pad_width = pads["pad width"]

        v_pitch = self.parameters["Package"]["package height"]
        h_pitch = self.parameters["Package"]["package width"]

        pads_per_row = pads["*n"] // 4

        row_len = (pads_per_row - 1) * pad_pitch

        pad_shape = pcbnew.PAD_SHAPE_OVAL if pads["*oval"] else pcbnew.PAD_SHAPE_RECT

        h_pad = PA.PadMaker(self.module).SMDPad( pad_length, pad_width,
                                                 shape=pad_shape, rot_degree=90.0)
        v_pad = PA.PadMaker(self.module).SMDPad( pad_length, pad_width, shape=pad_shape)

        #left row
        pin1Pos = pcbnew.wxPoint(-h_pitch / 2, 0)
        array = PA.PadLineArray(h_pad, pads_per_row, pad_pitch, True, pin1Pos)
        array.SetFirstPadInArray(1)
        array.AddPadsToModule(self.draw)

        #bottom row
        pin1Pos = pcbnew.wxPoint(0, v_pitch / 2)
        array = PA.PadLineArray(v_pad, pads_per_row, pad_pitch, False, pin1Pos)
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

        lim_x = self.parameters["Package"]["package width"] / 2
        lim_y = self.parameters["Package"]["package height"] / 2
        inner = (row_len / 2) + pad_pitch

        # epad
        epad_width = self.parameters["Package"]["package height"] - (2*pad_length)
        epad_length  = self.parameters["Package"]["package width"] - (2*pad_length)
        epad_subdv_x = pads["*epad subdiv x"]
        epad_subdv_y = pads["*epad subdiv y"]
        epad_via_drill = pads["thermal vias drill"]

        if (epad_subdv_y != 0 and epad_subdv_x != 0) and (epad_subdv_y != 1 or epad_subdv_x != 1):
          # Create the master pad (one area) on front solder mask, and perhaps of front copper layer
          # at location 0,0
          emasterpad = PA.PadMaker(self.module).SMDPad( epad_length, epad_width,
                  shape=pcbnew.PAD_SHAPE_RECT, rot_degree=0.0)
          emasterpad.SetLayerSet(pcbnew.LSET(pcbnew.F_Mask))  # currently, only on solder mask
          emasterpad.SetPadName(pads["*n"]+1)
          self.module.Add(emasterpad)

          px = pcbnew.FromMM(0.1); py = pcbnew.FromMM(0.1)
          esubpad_size_x = epad_length / epad_subdv_x - px
          esubpad_size_y = epad_width / epad_subdv_y - py
          epad1_pos = pcbnew.wxPoint(-(esubpad_size_x*(epad_subdv_x-1)/2), -esubpad_size_y*(epad_subdv_y-1)/2)
          epad = PA.PadMaker(self.module).SMDPad( esubpad_size_y, esubpad_size_x,
                  shape=pcbnew.PAD_SHAPE_RECT, rot_degree=0.0)
          array = PA.EPADGridArray(epad, epad_subdv_x, epad_subdv_y, esubpad_size_x + px, esubpad_size_y + py, pcbnew.wxPoint(0,0))
          array.SetFirstPadInArray(pads["*n"]+1)
          array.AddPadsToModule(self.draw)
          if pads["*thermal vias"]:
            via_diam = min(esubpad_size_y, esubpad_size_x)/3.
            thpad = PA.PadMaker(self.module).THRoundPad(via_diam, min(via_diam/2, epad_via_drill))
            layerset = pcbnew.LSET.AllCuMask()
            layerset.AddLayer(pcbnew.B_Mask)
            layerset.AddLayer(pcbnew.F_Mask)
            thpad.SetLayerSet(layerset)
            array2 = PA.EPADGridArray(thpad, epad_subdv_x, epad_subdv_y, esubpad_size_x + px, esubpad_size_y + py, pcbnew.wxPoint(0,0))
            array2.SetFirstPadInArray(pads["*n"]+1)
            array2.AddPadsToModule(self.draw)
        else:
          epad = PA.PadMaker(self.module).SMDPad(epad_length, epad_width)
          epad_pos = pcbnew.wxPoint(0,0)
          array = PA.PadLineArray(epad, 1, 1, False, epad_pos)
          array.SetFirstPadInArray(pads["*n"]+1)
          array.AddPadsToModule(self.draw)
          if pads["*thermal vias"]:
            via_diam = min(epad_length, epad_width)/3.
            thpad = PA.PadMaker(self.module).THRoundPad( via_diam, min(via_diam/2, epad_via_drill))
            layerset = pcbnew.LSET.AllCuMask()
            layerset.AddLayer(pcbnew.B_Mask)
            layerset.AddLayer(pcbnew.F_Mask)
            thpad.SetLayerSet(layerset)
            array2 = PA.PadLineArray(thpad, 1, 1, False, epad_pos)
            array2.SetFirstPadInArray(pads["*n"]+1)
            array2.AddPadsToModule(self.draw)

        #top left - diagonal
        self.draw.Line(-lim_x, -inner, -inner, -lim_y)
        # top right
        self.draw.Polyline([(inner, -lim_y), (lim_x, -lim_y), (lim_x, -inner)])
        # bottom left
        self.draw.Polyline([(-inner, lim_y), (-lim_x, lim_y), (-lim_x, inner)])
        # bottom right
        self.draw.Polyline([(inner, lim_y), (lim_x, lim_y), (lim_x, inner)])

        # Courtyard
        cmargin = self.parameters["Package"]["courtyard margin"]
        self.draw.SetLayer(pcbnew.F_CrtYd)
        sizex = (lim_x + cmargin) * 2 + pad_length/2.
        sizey = (lim_y + cmargin) * 2 + pad_length/2.
        # round size to nearest 0.1mm, rectangle will thus land on a 0.05mm grid
        sizex = self.PutOnGridMM(sizex, 0.1)
        sizey = self.PutOnGridMM(sizey, 0.1)
        # set courtyard line thickness to the one defined in KLC
        thick = self.draw.GetLineThickness()
        self.draw.SetLineThickness(pcbnew.FromMM(0.05))
        self.draw.Box(0, 0, sizex, sizey)
        # restore line thickness to previous value
        self.draw.SetLineThickness(pcbnew.FromMM(thick))

        #reference and value
        text_size = self.GetTextSize()  # IPC nominal
        text_offset = v_pitch / 2 + text_size + pad_length / 2

        self.draw.Value(0, text_offset, text_size)
        self.draw.Reference(0, -text_offset, text_size)

        # set SMD attribute
        self.module.SetAttributes(pcbnew.MOD_CMS)

QFNWizard().register()


