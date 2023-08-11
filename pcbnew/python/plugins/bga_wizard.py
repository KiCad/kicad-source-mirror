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


class BGAPadGridArray(PA.PadGridArray):

    def NamingFunction(self, n_x, n_y):
        return "%s%d" % (
            self.AlphaNameFromNumber(n_y + 1, alphabet="ABCDEFGHJKLMNPRTUVWY"),
            n_x + 1)


class BGAWizard(FootprintWizardBase.FootprintWizard):

    def GetName(self):
        return "BGA"

    def GetDescription(self):
        return "Ball Grid Array Footprint Wizard"

    def GenerateParameterList(self):
        self.AddParam("Pads", "pitch", self.uMM, 1, designator='p')
        self.AddParam("Pads", "size", self.uMM, 0.5)
        self.AddParam("Pads", "columns", self.uInteger, 5, designator="nx")
        self.AddParam("Pads", "rows", self.uInteger, 5, designator="ny")

        self.AddParam("Package", "width", self.uMM, 6, designator='X')
        self.AddParam("Package", "length", self.uMM, 6, designator='Y')
        self.AddParam("Package", "margin", self.uMM, 0.25, min_value=0.2, hint="Courtyard margin")

    def CheckParameters(self):

        # check that the package is large enough
        width = pcbnew.ToMM(self.parameters['Pads']['pitch'] * self.parameters['Pads']['columns'])

        length = pcbnew.ToMM(self.parameters['Pads']['pitch'] * self.parameters['Pads']['rows'])

        self.CheckParam('Package','width',min_value=width,info="Package width is too small (< {w}mm)".format(w=width))
        self.CheckParam('Package','length',min_value=length,info="Package length is too small (< {l}mm".format(l=length))

    def GetValue(self):
        pins = (self.parameters["Pads"]["rows"] * self.parameters["Pads"]["columns"])

        return "BGA-{n}_{a}x{b}_{x}x{y}mm".format(
                n = pins,
                a = self.parameters['Pads']['columns'],
                b = self.parameters['Pads']['rows'],
                x = pcbnew.ToMM(self.parameters['Package']['width']),
                y = pcbnew.ToMM(self.parameters['Package']['length'])
            )

    def BuildThisFootprint(self):

        pads = self.parameters["Pads"]

        rows = pads["rows"]
        cols = pads["columns"]
        pad_size = pads["size"]
        pad_size = pcbnew.wxSize(pad_size, pad_size)
        pad_pitch = pads["pitch"]

        # add in the pads
        pad = PA.PadMaker(self.module).SMTRoundPad(pads["size"])
        px = (int)( -((cols - 1) * pad_pitch) / 2)
        py = (int)(-((rows - 1) * pad_pitch) / 2)
        pin1_pos = pcbnew.VECTOR2I( px, py )

        array = BGAPadGridArray(pad, cols, rows, pad_pitch, pad_pitch)
        array.AddPadsToModule(self.draw)

        # Draw box outline on F.Fab layer
        self.draw.SetLayer(pcbnew.F_Fab)
        ssx = self.parameters['Package']['width'] / 2
        ssy = self.parameters['Package']['length'] / 2

        # Bevel should be 1mm nominal but we'll allow smaller values
        if pcbnew.ToMM(ssx) < 1:
            bevel = ssx
        else:
            bevel = pcbnew.FromMM(1)

        # Box with 1mm bevel as per IPC7351C
        self.draw.SetLineThickness( pcbnew.FromMM( 0.1 ) ) #Default per KLC F5.2 as of 12/2018
        self.draw.BoxWithDiagonalAtCorner(0, 0, ssx*2, ssy*2, bevel)

        # Add IPC markings to F_Silk layer
        self.draw.SetLayer( pcbnew.F_SilkS )
        self.draw.SetLineThickness( pcbnew.FromMM( 0.12 ) ) #Default per KLC F5.1 as of 12/2018
        offset = self.draw.GetLineThickness()
        len_x  = 0.5 * ssx
        len_y  = 0.5 * ssy

        edge = [
            [ ssx + offset - len_x, -ssy - offset],
            [ ssx + offset, -ssy - offset],
            [ ssx + offset, -ssy - offset + len_y],
               ]

        # Draw three square edges
        self.draw.Polyline(edge)
        self.draw.Polyline(edge, mirrorY=0)
        self.draw.Polyline(edge, mirrorX=0, mirrorY=0)

        # Draw pin-1 marker
        bevel += offset
        pin1 = [
            [ -ssx - offset + len_x, -ssy - offset],
            [ -ssx - offset + bevel, -ssy - offset],
            [ -ssx - offset, -ssy - offset + bevel],
            [ -ssx - offset, -ssy - offset + len_y],
                ]

        # Remove lines if the package is too small
        if bevel > len_x:
            pin1 = pin1[1:]

        if bevel > len_y:
            pin1 = pin1[:-1]

        self.draw.Polyline(pin1)

        # Draw a circle in the bevel void
        self.draw.Circle( -ssx, -ssy, pcbnew.FromMM(0.2), filled=True)

        # Courtyard
        cmargin = self.parameters['Package']['margin']
        self.draw.SetLayer(pcbnew.F_CrtYd)
        sizex = (ssx + cmargin) * 2
        sizey = (ssy + cmargin) * 2

        # round size to nearest 0.1mm, rectangle will thus land on a 0.05mm grid
        sizex = pcbnew.PutOnGridMM(sizex, 0.1)
        sizey = pcbnew.PutOnGridMM(sizey, 0.1)

        # set courtyard line thickness to the one defined in KLC
        self.draw.SetLineThickness(pcbnew.FromMM(0.05))
        self.draw.Box(0, 0, sizex, sizey)
        # restore line thickness to previous value
        self.draw.SetLineThickness(pcbnew.FromMM(cmargin))

        #reference and value
        text_size = self.GetTextSize()  # IPC nominal
        ypos = ssy + text_size
        self.draw.Value(0, ypos, text_size)
        self.draw.Reference(0, -ypos, text_size)

        # Add a extra text (${REFERENCE}) on the F_Fab layer
        extra_text = pcbnew.PCB_TEXT( self.module )
        extra_text.SetLayer( pcbnew.F_Fab )
        extra_text.SetPosition( pcbnew.VECTOR2I( 0, 0) )
        extra_text.SetTextSize( pcbnew.VECTOR2I( text_size, text_size ) )
        extra_text.SetText( "${REFERENCE}" )
        self.module.Add( extra_text )

        # set SMD attribute
        self.module.SetAttributes(pcbnew.FP_SMD)

BGAWizard().register()
