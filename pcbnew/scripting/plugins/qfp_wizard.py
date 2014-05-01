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

        self.CheckParamPositiveInt("Pads", "*n", is_multiple_of = 4)

    def GetReference(self):
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

        h_pad = PA.PadMaker(self.module).SMDPad(pad_width, pad_length, shape = pcbnew.PAD_OVAL)
        v_pad = PA.PadMaker(self.module).SMDPad(pad_length, pad_width, shape = pcbnew.PAD_OVAL)

        #left row
        pin1Pos = pcbnew.wxPoint(-h_pitch / 2, -row_len / 2)
        array = PA.PadLineArray(h_pad, pads_per_row, pad_pitch, True, pin1Pos)
        array.SetFirstPadInArray(1)
        array.AddPadsToModule()

        #bottom row
        pin1Pos = pcbnew.wxPoint(-row_len / 2, v_pitch / 2)
        array = PA.PadLineArray(v_pad, pads_per_row, pad_pitch, False, pin1Pos)
        array.SetFirstPadInArray(pads_per_row + 1)
        array.AddPadsToModule()

        #right row
        pin1Pos = pcbnew.wxPoint(h_pitch / 2, row_len / 2)
        array = PA.PadLineArray(h_pad, pads_per_row, -pad_pitch, True, pin1Pos)
        array.SetFirstPadInArray(2*pads_per_row + 1)
        array.AddPadsToModule()

        #top row
        pin1Pos = pcbnew.wxPoint(row_len / 2, -v_pitch / 2)
        array = PA.PadLineArray(v_pad, pads_per_row, -pad_pitch, False, pin1Pos)
        array.SetFirstPadInArray(3*pads_per_row + 1)
        array.AddPadsToModule()

        limX = pads["package width"] / 2
        limY = pads["package height"] / 2
        inner = (row_len / 2) + pad_pitch

        #top left - diagonal
        self.draw.Line(-limX, -inner, -inner, -limY)
        # top right
        self.draw.Polyline([(inner, -limY), (limX, -limY), (limX, -inner)])
        # bottom left
        self.draw.Polyline([(-inner, limY), (-limX, limY), (-limX, inner)])
        # bottom right
        self.draw.Polyline([(inner, limY), (limX, limY), (limX, inner)])

QFPWizard().register()
