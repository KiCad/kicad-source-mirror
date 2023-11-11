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

class QFPWizard(FootprintWizardBase.FootprintWizard):

    def GetName(self):
        return "QFP"

    def GetDescription(self):
        return "Quad Flat Package (QFP) footprint wizard"

    def GenerateParameterList(self):
        self.AddParam("Pads", "n", self.uInteger, 100, multiple=4, min_value=4)
        self.AddParam("Pads", "pitch", self.uMM, 0.5, designator='e')
        self.AddParam("Pads", "width", self.uMM, 0.25, designator='X1')
        self.AddParam("Pads", "length", self.uMM, 1.5, designator='Y1')
        self.AddParam("Pads", "horizontal spacing", self.uMM, 15, designator='C1')
        self.AddParam("Pads", "vertical spacing", self.uMM, 15, designator='C2')
        self.AddParam("Pads", "oval", self.uBool, True)

        self.AddParam("Package", "width", self.uMM, 14, designator='D1')
        self.AddParam("Package", "height", self.uMM, 14, designator='E1')
        self.AddParam("Package", "courtyard margin", self.uMM, 0.25, min_value=0.2)

    @property
    def pads(self):
        return self.parameters['Pads']

    @property
    def package(self):
        return self.parameters['Package']

    def CheckParameters(self):
        # todo - custom checking
        pass

    def GetValue(self):
        return "QFP-{n}_{x:g}x{y:g}_Pitch{p:g}mm".format(
            n = self.pads['n'],
            x = pcbnew.ToMM(self.package['width']),
            y = pcbnew.ToMM(self.package['height']),
            p = pcbnew.ToMM(self.pads['pitch'])
            )

    def BuildThisFootprint(self):

        pad_pitch = self.pads["pitch"]
        pad_length = self.pads["length"]
        pad_width = self.pads["width"]

        v_pitch = self.pads["vertical spacing"]
        h_pitch = self.pads["horizontal spacing"]

        pads_per_row = int(self.pads["n"] // 4)

        row_len = (pads_per_row - 1) * pad_pitch

        pad_shape = pcbnew.PAD_SHAPE_OVAL if self.pads["oval"] else pcbnew.PAD_SHAPE_RECT

        h_pad = PA.PadMaker(self.module).SMDPad( pad_length, pad_width,
                                                 shape=pad_shape, rot_degree=90.0)
        v_pad = PA.PadMaker(self.module).SMDPad( pad_length, pad_width, shape=pad_shape)

        #left row
        pin1Pos = pcbnew.VECTOR2I( (int)(-h_pitch / 2), 0)
        array = PA.PadLineArray(h_pad, pads_per_row, pad_pitch, True, pin1Pos)
        array.SetFirstPadInArray(1)
        array.AddPadsToModule(self.draw)

        #bottom row
        pin1Pos = pcbnew.VECTOR2I( 0, (int)(v_pitch / 2) )
        array = PA.PadLineArray(v_pad, pads_per_row, pad_pitch, False, pin1Pos)
        array.SetFirstPadInArray(pads_per_row + 1)
        array.AddPadsToModule(self.draw)

        #right row
        pin1Pos = pcbnew.VECTOR2I( (int)(h_pitch / 2), 0)
        array = PA.PadLineArray(h_pad, pads_per_row, -pad_pitch, True,
                                pin1Pos)
        array.SetFirstPadInArray(2*pads_per_row + 1)
        array.AddPadsToModule(self.draw)

        #top row
        pin1Pos = pcbnew.VECTOR2I(0, (int)(-v_pitch / 2) )
        array = PA.PadLineArray(v_pad, pads_per_row, -pad_pitch, False,
                                pin1Pos)
        array.SetFirstPadInArray(3*pads_per_row + 1)
        array.AddPadsToModule(self.draw)

        offset = pcbnew.FromMM(0.15)

        x = self.parameters["Package"]["width"] / 2 + offset
        y = self.parameters["Package"]["height"] / 2 + offset
        inner = (row_len / 2) + pad_pitch

        # Add outline to F_Fab layer
        self.draw.SetLayer(pcbnew.F_Fab)
        thick = self.draw.GetLineThickness()

        bevel = min( pcbnew.FromMM(1.0), self.package['width']/2, self.package['height']/2 )

        w = self.package['width']
        h = self.package['height']

        # outermost limits of pins
        right_edge = (h_pitch + pad_length) / 2
        left_edge = -right_edge

        bottom_edge = (v_pitch + pad_length) / 2
        top_edge = -bottom_edge

        self.draw.SetLineThickness( pcbnew.FromMM( 0.1 ) ) #Default per KLC F5.2 as of 12/2018
        self.draw.BoxWithDiagonalAtCorner(0, 0, w, h, bevel)

        # Draw silkscreen
        self.draw.SetLayer( pcbnew.F_SilkS )
        self.draw.SetLineThickness( pcbnew.FromMM( 0.12 ) ) #Default per KLC F5.1 as of 12/2018

        #top left - as per IPC-7351C
        self.draw.Polyline([(-inner, -y), (-x, -y), (-x, -inner), (left_edge, -inner)])
        # top right
        self.draw.Polyline([(inner, -y), (x, -y), (x, -inner)])
        # bottom left
        self.draw.Polyline([(-inner, y), (-x, y), (-x, inner)])
        # bottom right
        self.draw.Polyline([(inner, y), (x, y), (x, inner)])

        # Courtyard
        cmargin = self.parameters["Package"]["courtyard margin"]
        self.draw.SetLayer( pcbnew.F_CrtYd )
        sizex = ( right_edge + cmargin ) * 2
        sizey = ( bottom_edge + cmargin ) * 2
        # round size to nearest 0.1mm, rectangle will thus land on a 0.05mm grid
        sizex = pcbnew.PutOnGridMM( sizex, 0.1 )
        sizey = pcbnew.PutOnGridMM( sizey, 0.1 )

        self.draw.SetLineThickness( pcbnew.FromMM( 0.05 ) ) #Default per KLC F5.3 as of 12/2018
        self.draw.Box( 0, 0, sizex, sizey )
        # restore line thickness to previous value
        self.draw.SetLineThickness(pcbnew.FromMM(thick))

        #reference and value
        text_size = self.GetTextSize()  # IPC nominal
        text_offset = v_pitch / 2 + text_size + pad_length / 2

        self.draw.Value(0, text_offset, text_size)
        self.draw.Reference(0, -text_offset, text_size)

        # Add a extra text (${REFERENCE}) on the F_Fab layer
        extra_text = pcbnew.PCB_TEXT( self.module )
        extra_text.SetLayer( pcbnew.F_Fab )
        extra_text.SetPosition( pcbnew.VECTOR2I( 0, 0) )
        extra_text.SetTextSize( pcbnew.VECTOR2I( text_size, text_size ) )
        extra_text.SetText( "${REFERENCE}" )
        self.module.Add( extra_text )

        # set SMD attribute
        self.module.SetAttributes(pcbnew.FP_SMD)

QFPWizard().register()
