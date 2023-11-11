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

import FootprintWizardBase
import PadArray as PA


class RowedFootprint(FootprintWizardBase.FootprintWizard):

    def GenerateParameterList(self):

        # defaults for a ZIP package
        self.AddParam("Pads", "pad count", self.uInteger, 24)
        self.AddParam("Pads", "line count", self.uInteger, 2)

        self.AddParam("Body", "silkscreen inside", self.uBool, False)
        self.AddParam("Body", "courtyard margin", self.uMM, 0.5, min_value=0.2)

    @property
    def pads(self):
        return self.parameters['Pads']

    @property
    def body(self):
        return self.parameters['Body']

    def CheckParameters(self):
        # TODO - implement custom checks
        pass

    def BuildThisFootprint(self):

        pad_count = self.pads['pad count']
        pad_Vsize = self.pads['pad height']
        pad_Hsize = self.pads['pad width']
        line_pitch = self.pads['line spacing']
        pad_pitch = self.pads['pitch']
        line_count = self.pads['line count']

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
        ssx_offset = pad_Hsize / 2 + self.body['outline x margin']
        ssy_offset = pad_Vsize / 2 + self.body['outline y margin']

        pin1posX = pad_pitch * (pad_count - 1) / 2
        pin1posY = line_pitch * (line_count - 1) / 2
        leftx = pin1posX + ssx_offset
        lowy = pin1posY + ssy_offset

        cornery = lowy

        # body inside pads is possible only for 2 rows.
        # for other values, there is no room
        linew = self.draw.GetLineThickness()
        if self.body['silkscreen inside'] and line_count == 2:
            cornery = pin1posY - ssy_offset
            if cornery < linew:
                cornery = linew

        thick = self.draw.GetLineThickness()
        self.draw.SetLineThickness( pcbnew.FromMM( 0.12 ) ) #Default per KLC F5.1 as of 12/2018
        self.DrawBox(leftx*2, cornery*2)

        # Courtyard
        cmarginx = self.body['courtyard margin']
        cmarginy = cmarginx
        self.draw.SetLayer(pcbnew.F_CrtYd)
        sizex = (pin1posX + cmarginx) * 2 + pad_Hsize + thick
        sizey = (pin1posY + cmarginy) * 2 + pad_Vsize + thick
        # round size to nearest 0.1mm, rectangle will thus land on a 0.05mm grid
        sizex = pcbnew.PutOnGridMM(sizex, 0.1)
        sizey = pcbnew.PutOnGridMM(sizey, 0.1)
        # set courtyard line thickness to the one defined in KLC
        self.draw.SetLineThickness(pcbnew.FromMM(0.05))
        self.draw.Box(0, 0, sizex, sizey)
        # restore line thickness to previous value
        self.draw.SetLineThickness(pcbnew.FromMM(thick))

        #reference and value
        text_size = self.GetTextSize()  # IPC nominal
        t_posy = lowy + text_size

        self.draw.Value(0, t_posy, text_size)
        self.draw.Reference(0, -t_posy, text_size)

        # Add a extra text (${REFERENCE}) on the F_Fab layer
        extra_text = pcbnew.PCB_TEXT( self.module )
        extra_text.SetLayer( pcbnew.F_Fab )
        extra_text.SetPosition( pcbnew.VECTOR2I( 0, 0) )
        extra_text.SetTextSize( pcbnew.VECTOR2I( text_size, text_size ) )
        extra_text.SetText( "${REFERENCE}" )
        self.module.Add( extra_text )

        # set SMD attribute
        if self.GetName() == "ZIP":
            self.module.SetAttributes(pcbnew.FP_THROUGH_HOLE)
        elif self.GetName() == "ZOIC":
            self.module.SetAttributes(pcbnew.FP_SMD)

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

        self.AddParam("Pads", "pitch", self.uMM, 1.27)
        self.AddParam("Pads", "pad width", self.uMM, 1.2)
        self.AddParam("Pads", "pad height", self.uMM, 2)
        self.AddParam("Pads", "line spacing", self.uMM, 2.54)
        self.AddParam("Pads", "drill size", self.uMM, 0.8)
        self.AddParam("Body", 'outline x margin', self.uMM, 1)
        self.AddParam("Body", 'outline y margin', self.uMM, 0.5)

    def GetValue(self):
        rows = self.pads['line count']
        pad_cnt = self.pads['pad count']

        if rows == 1:
            name = "SIP"
        elif rows == 2:
            name = "ZIP"
        else:  # triple and up aren't really a thing, but call it something!
            name = "xIP"

        return "%s-%d" % (name, pad_cnt)

    def GetPad(self):
        pad_Vsize = self.pads['pad height']
        pad_Hsize = self.pads['pad width']
        drill = self.pads['drill size']
        return PA.PadMaker(self.module).THPad(
            pad_Vsize, pad_Hsize, drill, shape=pcbnew.PAD_SHAPE_OVAL)

ZIPWizard().register()


class ZOICWizard(RowedFootprint):

    def GetName(self):
        return "ZOIC"

    def GetDescription(self):
        return "ZOIC, etc, Footprint Wizard"

    def GetValue(self):
        return "%s-%d" % ("ZOIC-", self.pads['pad count'])

    def GenerateParameterList(self):
        RowedFootprint.GenerateParameterList(self)

        #and override some of them
        self.AddParam("Pads", "pitch", self.uMM, 0.6)
        self.AddParam("Pads", "pad width", self.uMM, 0.6)
        self.AddParam("Pads", "pad height", self.uMM, 1.8)
        self.AddParam("Pads", "line spacing", self.uMM, 5.2)

        self.AddParam("Body", "outline x margin", self.uMM, 0.5)
        self.AddParam("Body", "outline y margin", self.uMM, 1)

    def GetPad(self):
        pad_Vsize = self.pads['pad height']
        pad_Hsize = self.pads['pad width']
        return PA.PadMaker(self.module).SMDPad(
            pad_Vsize, pad_Hsize, shape=pcbnew.PAD_SHAPE_RECT)

ZOICWizard().register()
