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


class RowedFootprint(HFPW.HelpfulFootprintWizardPlugin):

    pad_count_key = '#pad count'
    line_count_key = '#line count'
    pad_vertical_size_key = 'pad vertical size'
    pad_horizontal_size_key = 'pad horizontal size'
    line_spacing_key = 'line spacing'
    pad_pitch_key = 'pad pitch'
    drill_size_key = 'drill size'

    courtyard_x_margin_key = 'courtyard x margin'
    courtyard_y_margin_key = 'courtyard y margin'
    outline_x_margin_key = 'outline x margin'
    outline_y_margin_key = 'outline y margin'
    silkscreen_inside_key = 'silk screen inside'

    def GenerateParameterList(self):

        # defaults for a ZIP package
        self.AddParam("Pads", self.pad_count_key, self.uNatural, 24)
        self.AddParam("Pads", self.line_count_key, self.uNatural, 2)
        self.AddParam("Body", self.silkscreen_inside_key, self.uBool, False)
        self.AddParam("Body", self.courtyard_x_margin_key, self.uMM, 1)
        self.AddParam("Body", self.courtyard_y_margin_key, self.uMM, 1)

    def CheckParameters(self):
        self.CheckParamInt("Pads", '*' + self.pad_count_key)
        self.CheckParamInt("Pads", '*' + self.line_count_key)

        # can do this internally to parameter manager?
        self.CheckParamBool("Body", '*' + self.silkscreen_inside_key)

    def BuildThisFootprint(self):
        pads = self.parameters["Pads"]
        body = self.parameters["Body"]

        pad_count = pads['*' + self.pad_count_key]
        pad_Vsize = pads[self.pad_vertical_size_key]
        pad_Hsize = pads[self.pad_horizontal_size_key]
        line_pitch = pads[self.line_spacing_key]
        pad_pitch = pads[self.pad_pitch_key]
        line_count = pads['*' + self.line_count_key]

        if line_count == 1:
            singleline = True
        else:
            singleline = False

        # add in the pads
        pad = self.GetPad()

        array = PA.PadZGridArray(pad, pad_count, line_count, line_pitch, pad_pitch)
        array.AddPadsToModule(self.draw)

        # draw the Silk Screen
        pads_per_line = pad_count // line_count
        row_length = pad_pitch * (pads_per_line - 1)  # fenceposts
        ssx_offset = pad_Hsize / 2 + body[self.outline_x_margin_key]
        ssy_offset = pad_Vsize / 2 + body[self.outline_y_margin_key]

        pin1posX = pad_pitch * (pad_count - 1) / 2
        pin1posY = line_pitch * (line_count - 1) / 2
        leftx = pin1posX + ssx_offset
        lowy = pin1posY + ssy_offset

        cornery = lowy

        # body inside pads is possible only for 2 rows.
        # for other values, there is no room
        linew = self.draw.GetLineTickness()
        if body['*'+self.silkscreen_inside_key] and line_count == 2:
            cornery = pin1posY - ssy_offset
            if cornery < linew:
                cornery = linew

        self.DrawBox(leftx*2, cornery*2)

        # Courtyard
        cmarginx = body[self.courtyard_x_margin_key]
        cmarginy = body[self.courtyard_y_margin_key]
        self.draw.SetLayer(pcbnew.F_CrtYd)
        sizex = (pin1posX + cmarginx) * 2 + pad_Hsize
        sizey = (pin1posY + cmarginy) * 2 + pad_Vsize
        self.draw.Box(0, 0, sizex, sizey)

        #reference and value
        text_size = self.GetTextSize()  # IPC nominal
        t_posy = lowy + text_size

        self.draw.Value(0, t_posy, text_size)
        self.draw.Reference(0, -t_posy, text_size)

    def DrawBox(self, sizex, sizey):

        #  ----------
        #  | 2 4 6 8|
        #  |1 3 5 7 |
        #  \---------
        setback = pcbnew.FromMM(1)

        if setback > sizey/2:
            setback = sizey/2

        self.draw.BoxWithDiagonalAtCorner(0, 0, sizex, sizey, setback, self.draw.flipY)


class ZIPWizard(RowedFootprint):

    def GetName(self):
        return "ZIP"

    def GetDescription(self):
        return "N lines Zip Package Footprint Wizard"

    def GenerateParameterList(self):
        RowedFootprint.GenerateParameterList(self)

        self.AddParam("Pads", self.pad_pitch_key, self.uMM, 1.27)
        self.AddParam("Pads", self.pad_horizontal_size_key, self.uMM, 1.2)
        self.AddParam("Pads", self.pad_vertical_size_key, self.uMM, 2)
        self.AddParam("Pads", self.line_spacing_key, self.uMM, 2.54)
        self.AddParam("Pads", self.drill_size_key, self.uMM, 0.8)
        self.AddParam("Body", self.outline_x_margin_key, self.uMM, 1)
        self.AddParam("Body", self.outline_y_margin_key, self.uMM, 0.5)

    def GetValue(self):
        rows = self.parameters["Pads"]['*' + self.line_count_key]
        pad_cnt = self.parameters["Pads"]['*' + self.pad_count_key]

        if rows == 1:
            name = "SIP"
        elif rows == 2:
            name = "ZIP"
        else:  # triple and up aren't really a thing, but call it something!
            name = "xIP"

        return "%s-%d" % (name, pad_cnt)

    def GetPad(self):
        pad_Vsize = self.parameters["Pads"][self.pad_vertical_size_key]
        pad_Hsize = self.parameters["Pads"][self.pad_horizontal_size_key]
        drill = self.parameters["Pads"][self.drill_size_key]
        return PA.PadMaker(self.module).THPad(
            pad_Vsize, pad_Hsize, drill, shape=pcbnew.PAD_SHAPE_OVAL)

ZIPWizard().register()


class ZOICWizard(RowedFootprint):

    def GetName(self):
        return "ZOIC"

    def GetDescription(self):
        return "ZOIC, etc, Footprint Wizard"

    def GetValue(self):
        return "%s-%d" % ("ZOIC", self.parameters["Pads"]['*' + self.pad_count_key])

    def GenerateParameterList(self):
        RowedFootprint.GenerateParameterList(self)

        #and override some of them
        self.AddParam("Pads", self.pad_pitch_key, self.uMM, 0.6)
        self.AddParam("Pads", self.pad_horizontal_size_key, self.uMM, 0.6)
        self.AddParam("Pads", self.pad_vertical_size_key, self.uMM, 1.8)
        self.AddParam("Pads", self.line_spacing_key, self.uMM, 5.2)

        self.AddParam("Body", self.outline_x_margin_key, self.uMM, 0.5)
        self.AddParam("Body", self.outline_y_margin_key, self.uMM, 1)

    def GetPad(self):
        pad_Vsize = self.parameters["Pads"][self.pad_vertical_size_key]
        pad_Hsize = self.parameters["Pads"][self.pad_horizontal_size_key]
        return PA.PadMaker(self.module).SMDPad(
            pad_Vsize, pad_Hsize, shape=pcbnew.PAD_SHAPE_RECT)

ZOICWizard().register()
