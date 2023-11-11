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
import FootprintWizardBase
import PadArray as PA


class circular_pad_array_wizard(FootprintWizardBase.FootprintWizard):

    def GetName(self):
        return "Circular Pad Array"

    def GetDescription(self):
        return "Circular array of pads"

    def GenerateParameterList(self):

        self.AddParam("Pads", "count", self.uInteger, 6, min_value=1, designator='n')
        self.AddParam("Pads", "center diameter", self.uMM, 5, min_value=0, designator='r', hint="Centre distance between pads")
        self.AddParam("Pads", "diameter", self.uMM, 1.5)
        self.AddParam("Pads", "drill", self.uMM, 0.8)
        self.AddParam("Pads", "angle", self.uDegrees, 0, designator='a')
        self.AddParam("Pads", "rectangle", self.uBool, False)
        self.AddParam("Pads", "pad 1 rectangle", self.uBool, False)

        self.AddParam("Pad rotation", "pad rotation", self.uBool, False, designator='r')
        self.AddParam("Pad rotation", "pad angle offset", self.uDegrees, 0, designator='o')

        self.AddParam("Numbering", "initial", self.uInteger, 1, min_value=1)
        #self.AddParam("Numbering", "increment", self.uInteger, 1, min_value=1)
        self.AddParam("Numbering", "clockwise", self.uBool, True)

        self.AddParam("Outline", "diameter", self.uMM, 7, designator='D')
        self.AddParam("Outline", "margin", self.uMM, 0.25, min_value=0.2)

    def CheckParameters(self):

        pads = self.parameters['Pads']
        numbering = self.parameters['Numbering']
        outline = self.parameters['Outline']
        padRotation = self.parameters['Pad rotation']

        # Check that pads do not overlap
        pad_dia = pcbnew.ToMM(pads['diameter'])
        centres = pcbnew.ToMM(pads['center diameter'])
        n_pads = pads['count']

        self.CheckParam('Pads','diameter',max_value=centres*math.pi/n_pads,info="Pads overlap")

        # Check that the pads fit inside the outline
        d_min = pad_dia + centres

        self.CheckParam("Outline","diameter",min_value=d_min, info="Outline diameter is too small")


    def GetValue(self):
        pins = self.parameters["Pads"]["count"]
        return "CPA_%d" % pins

    def BuildThisFootprint(self):

        pads = self.parameters['Pads']
        numbering = self.parameters['Numbering']
        outline = self.parameters['Outline']
        padRotation = self.parameters['Pad rotation']

        pad_size = pads['diameter']
        pad_shape = pcbnew.PAD_SHAPE_RECT if pads["rectangle"] else pcbnew.PAD_SHAPE_OVAL

        pad = PA.PadMaker(self.module).THPad(pads['diameter'], pads['diameter'], pads['drill'], shape=pad_shape)

        array = PA.PadCircleArray(
            pad, pads['count'], pads['center diameter'] / 2,
            angle_offset=pads["angle"],
            centre=pcbnew.VECTOR2I(0, 0),
            clockwise=numbering["clockwise"],
            padRotationEnable= padRotation["pad rotation"],
            padRotationOffset = padRotation["pad angle offset"])

        if pads["pad 1 rectangle"]:
            firstPad = PA.PadMaker(self.module).THPad(pads['diameter'], pads['diameter'], pads['drill'], shape=pcbnew.PAD_SHAPE_RECT)
            array.SetFirstPadType(firstPad)

        array.SetFirstPadInArray(numbering["initial"])

        array.AddPadsToModule(self.draw)

        # Draw the outline
        body_radius = outline['diameter'] / 2
        self.draw.SetLayer(pcbnew.F_Fab)
        self.draw.SetLineThickness( pcbnew.FromMM( 0.1 ) ) #Default per KLC F5.2 as of 12/2018
        self.draw.Circle(0, 0, body_radius)

        #silkscreen
        body_radius += pcbnew.FromMM(0.12)
        self.draw.SetLineThickness( pcbnew.FromMM( 0.12 ) ) #Default per KLC F5.1 as of 12/2018
        self.draw.SetLayer(pcbnew.F_SilkS)
        self.draw.Circle(0, 0, body_radius)

        # courtyard
        self.draw.SetLayer(pcbnew.F_CrtYd)
        self.draw.SetLineThickness(pcbnew.FromMM(0.05))
        self.draw.Circle(0, 0, body_radius + outline['margin'])

        # Text (reference and value) size
        text_size = self.GetTextSize()  # IPC nominal
        thickness = self.GetTextThickness()
        textposy = body_radius + self.draw.GetLineThickness()/2 + self.GetTextSize()/2 + thickness + + outline['margin']
        self.draw.Value( 0, textposy, text_size )
        self.draw.Reference( 0, -textposy, text_size )

        # Add a extra text (${REFERENCE}) on the F_Fab layer
        extra_text = pcbnew.PCB_TEXT( self.module )
        extra_text.SetLayer( pcbnew.F_Fab )
        extra_text.SetPosition( pcbnew.VECTOR2I( 0, 0) )
        extra_text.SetTextSize( pcbnew.VECTOR2I( text_size, text_size ) )
        extra_text.SetText( "${REFERENCE}" )
        self.module.Add( extra_text )

circular_pad_array_wizard().register()
