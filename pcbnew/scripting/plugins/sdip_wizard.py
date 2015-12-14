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
        pad_cnt = self.nx*self.ny

        if self.ny == 1:
            return x+1

        if (y % 2) == 0:  # upper row, count down
            return pad_cnt-x
        else:  # lower row, count up
            return x+1


class RowedFootprint(HFPW.HelpfulFootprintWizardPlugin):

    pad_count_key = 'pas count'
    row_count_key = 'row count'
    row_spacing_key = 'row spacing'
    pad_length_key = 'pad length'
    pad_width_key = 'pad width'
    pad_pitch_key = 'pad pitch'

    silkscreen_inside_key = 'silk screen inside'
    outline_x_margin_key = 'outline x margin'
    outline_y_margin_key = 'outline y margin'

    def GenerateParameterList(self):
        # defaults for a DIP package
        self.AddParam("Pads", self.pad_count_key, self.uNatural, 24)
        self.AddParam("Pads", self.row_count_key, self.uNatural, 2)

        self.AddParam("Body", self.silkscreen_inside_key, self.uBool, False)
        self.AddParam("Body", self.outline_x_margin_key, self.uMM, 0.5)
        self.AddParam("Body", self.outline_y_margin_key, self.uMM, 0.5)

    def CheckParameters(self):
        self.CheckParamInt("Pads", '*' + self.row_count_key, min_value=1, max_value=2)
        self.CheckParamInt(
            "Pads", '*' + self.pad_count_key,
            is_multiple_of=self.parameters["Pads"]['*' + self.row_count_key])

        # can do this internally to parameter manager?
        self.CheckParamBool("Body", '*' + self.silkscreen_inside_key)

    def BuildThisFootprint(self):
        pads = self.parameters["Pads"]
        body = self.parameters["Body"]
        num_pads = pads['*' + self.pad_count_key]
        pad_length = pads[self.pad_length_key]
        pad_width = pads[self.pad_width_key]
        row_pitch = pads[self.row_spacing_key]
        pad_pitch = pads[self.pad_pitch_key]
        num_rows = pads['*' + self.row_count_key]

        pads_per_row = num_pads // num_rows

        # add in the pads
        pad = self.GetPad()

        array = RowedGridArray(pad, pads_per_row, num_rows, pad_pitch, row_pitch)
        array.AddPadsToModule(self.draw)

        # draw the Silk Screen
        Hsize = pad_pitch * (num_pads / num_rows - 1)
        Vsize = row_pitch * (num_rows - 1)
        pin1_posY = -Vsize / 2
        pin1_posX = -Hsize / 2

        pad_length = pads[self.pad_length_key]
        pad_width = pads[self.pad_width_key]

        ssx_offset = -pad_width / 2 - body[self.outline_x_margin_key]
        ssy_offset = -pad_length / 2 - body[self.outline_y_margin_key]

        if body['*' + self.silkscreen_inside_key]:
            ssy_offset *= -1

        ssx = -pin1_posX - ssx_offset
        ssy = -pin1_posY - ssy_offset

        self.DrawBox(ssx, ssy)

        #reference and value
        text_size = self.GetTextSize()  # IPC nominal

        if num_rows == 1:
            text_py = ssy + text_size
            self.draw.Value(0, -text_py, text_size)
            self.draw.Reference(0, text_py, text_size)
        else:
            text_px = ssx + text_size
            # self.draw.Value(text_px, 0, text_size, orientation_degree=90)
            self.draw.Value(0, 0, text_size)
            self.draw.Reference(-text_px, 0, text_size, orientation_degree=90)


class SDIPWizard(RowedFootprint):

    def GetName(self):
        return "S-DIP"

    def GetDescription(self):
        return "Single/Dual Inline Package Footprint Wizard"

    def GenerateParameterList(self):
        RowedFootprint.GenerateParameterList(self)

        self.AddParam("Pads", self.pad_pitch_key, self.uMM, 2.54)
        self.AddParam("Pads", self.pad_width_key, self.uMM, 1.2)
        self.AddParam("Pads", self.pad_length_key, self.uMM, 2)
        self.AddParam("Pads", self.row_spacing_key, self.uMM, 7.52)
        self.AddParam("Pads", "drill size", self.uMM, 0.8)

    def GetValue(self):
        pads = self.parameters["Pads"]
        rows = pads['*' + self.row_count_key]
        pad_count = pads['*' + self.pad_count_key]
        row_dist_mil = pcbnew.Iu2Mils(int(self.parameters["Pads"][self.row_spacing_key])) #int(self.parameters["Pads"][self.row_spacing_key] / 2.54 * 100)
        pad_shape = ""

        if pads[self.pad_width_key] != pads[self.pad_length_key]:
            pad_shape = '_ELL'

        if rows == 1:
            name = "SIP"
            return "%s-%d" % (name, pad_count)

        name = "DIP"
        return "%s-%d_%d%s" % (name, pad_count, row_dist_mil, pad_shape)

    def GetPad(self):
        pad_length = self.parameters["Pads"][self.pad_length_key]
        pad_width = self.parameters["Pads"][self.pad_width_key]
        drill = self.parameters["Pads"]["drill size"]
        shape = pcbnew.PAD_SHAPE_CIRCLE

        if pad_length != pad_width:
            shape = pcbnew.PAD_SHAPE_OVAL

        return PA.PadMaker(self.module).THPad(
            pad_length, pad_width, drill, shape=shape)

    def DrawBox(self, ssx, ssy):

        if self.parameters["Pads"]['*' + self.row_count_key] == 2:

            #  ----------
            #  |8 7 6 5 |
            #  >        |
            #  |1 2 3 4 |
            #  ----------

            # draw the notch
            notchWidth = ssy/1.5
            notchHeight = self.draw.GetLineTickness()*3

            # NotchedBox draws the notch on top. Rotate the box 90 degrees
            # to have it on the left
            self.draw.NotchedBox(0, 0, ssy*2, ssx*2, notchWidth, notchHeight, -90)
        else:
            #  -----------------
            #  |1|2 3 4 5 6 7 8|
            #  -----------------
            self.draw.Box(0, 0, ssx*2, ssy*2)

            #line between pin1 and pin2
            pad_pitch = self.parameters["Pads"][self.pad_pitch_key]
            pad_cnt = self.parameters["Pads"]['*' + self.pad_count_key]
            line_x = ( pad_cnt/2 - 1) * pad_pitch
            self.draw.VLine(-line_x, -ssy, ssy * 2)

        return ssx, ssy

SDIPWizard().register()


class SOICWizard(RowedFootprint):

    def GetName(self):
        return "SOIC"

    def GetDescription(self):
        return "SOIC, MSOP, SSOP, TSSOP, etc, footprint wizard"

    def GetValue(self):
        pad_count = self.parameters["Pads"]['*' + self.pad_count_key]
        return "%s-%d" % ("SOIC", pad_count)

    def GenerateParameterList(self):
        RowedFootprint.GenerateParameterList(self)

        #and override some of them
        self.AddParam("Pads", self.pad_pitch_key, self.uMM, 1.27)
        self.AddParam("Pads", self.pad_width_key, self.uMM, 0.6)
        self.AddParam("Pads", self.pad_length_key, self.uMM, 2.2)
        self.AddParam("Pads", self.row_spacing_key, self.uMM, 5.2)

    def GetPad(self):
        pad_length = self.parameters["Pads"][self.pad_length_key]
        pad_width = self.parameters["Pads"][self.pad_width_key]
        return PA.PadMaker(self.module).SMDPad(
            pad_length, pad_width, shape=pcbnew.PAD_SHAPE_RECT)

    def DrawBox(self, ssx, ssy):

        #  ----------
        #  |8 7 6 5 |
        #  |1 2 3 4 |
        #  \---------

        setback = pcbnew.FromMM(0.8)

        if setback > ssy:
            setback = ssy

        self.draw.BoxWithDiagonalAtCorner(0, 0, ssx*2, ssy*2, setback, self.draw.flipY)

SOICWizard().register()
