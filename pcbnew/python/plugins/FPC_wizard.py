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

class FPC_FootprintWizard(FootprintWizardBase.FootprintWizard):

    def GetName(self):
        return "FPC (SMT connector)"

    def GetDescription(self):
        return "FPC (SMT connector) Footprint Wizard"

    def GetValue(self):
        pins = self.parameters["Pads"]["n"]
        return "FPC_%d" % pins

    def GenerateParameterList(self):
        self.AddParam( "Pads", "n", self.uInteger, 40 )
        self.AddParam( "Pads", "pitch", self.uMM, 0.5 )
        self.AddParam( "Pads", "width", self.uMM, 0.25 )
        self.AddParam( "Pads", "height", self.uMM, 1.6)
        self.AddParam( "Shield", "shield_to_pad", self.uMM, 1.6 )
        self.AddParam( "Shield", "from_top", self.uMM, 1.3 )
        self.AddParam( "Shield", "width", self.uMM, 1.5 )
        self.AddParam( "Shield", "height", self.uMM, 2 )


    # build a rectangular pad
    def smdRectPad(self,module,size,pos,name):
        pad = pcbnew.PAD(module)
        pad.SetSize(size)
        pad.SetShape(pcbnew.PAD_SHAPE_RECT)
        pad.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
        pad.SetLayerSet( pad.SMDMask() )
        pad.SetPosition(pos)
        pad.SetName(name)
        return pad

    def CheckParameters(self):
        #TODO implement custom parameter checking
        pass

    def BuildThisFootprint(self):
        p = self.parameters
        pad_count       = int(p["Pads"]["n"])
        pad_width       = p["Pads"]["width"]
        pad_height      = p["Pads"]["height"]
        pad_pitch       = p["Pads"]["pitch"]
        shl_width       = p["Shield"]["width"]
        shl_height      = p["Shield"]["height"]
        shl_to_pad      = p["Shield"]["shield_to_pad"]
        shl_from_top    = p["Shield"]["from_top"]

        offsetX         = pad_pitch * ( pad_count-1 ) / 2
        size_pad = pcbnew.VECTOR2I( int(pad_width), int(pad_height) )
        size_shld = pcbnew.VECTOR2I(shl_width, shl_height)
        size_text = self.GetTextSize()  # IPC nominal

        # Gives a position and size to ref and value texts:
        textposy = pad_height/2 + pcbnew.FromMM(1) + self.GetTextThickness()
        angle_degree = 0.0
        self.draw.Reference( 0, textposy, size_text, angle_degree )

        textposy = textposy + size_text + self.GetTextThickness()
        self.draw.Value( 0, textposy, size_text )

        # create a pad array and add it to the module
        for n in range ( 0, pad_count ):
            xpos = (int)(pad_pitch*n - offsetX)
            pad = self.smdRectPad(self.module,size_pad, pcbnew.VECTOR2I( xpos, 0), str(n+1))
            self.module.Add(pad)


        # Mechanical shield pads: left pad and right pad
        xpos = -shl_to_pad-offsetX
        pad_s0_pos = pcbnew.VECTOR2I( int(xpos), int(shl_from_top) )
        pad_s0 = self.smdRectPad(self.module, size_shld, pad_s0_pos, "0")
        xpos = (pad_count-1) * pad_pitch+shl_to_pad - offsetX
        pad_s1_pos = pcbnew.VECTOR2I( int(xpos), int(shl_from_top) )
        pad_s1 = self.smdRectPad(self.module, size_shld, pad_s1_pos, "0")

        self.module.Add(pad_s0)
        self.module.Add(pad_s1)

        # add footprint outline
        self.draw.SetLineThickness( pcbnew.FromMM( 0.12 ) ) #Default per KLC F5.1 as of 12/2018
        linewidth = self.draw.GetLineThickness()
        margin = linewidth

        # upper line
        posy = -pad_height/2 - linewidth/2 - margin
        xstart = - pad_pitch*0.5-offsetX
        xend = pad_pitch * pad_count + xstart;
        min_y = posy
        max_y = posy
        self.draw.Line( xstart, posy, xend, posy )

        # lower line
        posy = pad_height/2 + linewidth/2 + margin
        max_y = max( max_y, posy )
        self.draw.Line(xstart, posy, xend, posy)

        # around left mechanical pad (the outline around right pad is mirrored/y axis)
        yend = pad_s0_pos.y + shl_height/2 + margin
        max_y = max( max_y, yend )
        self.draw.Line(xstart, posy, xstart, yend)
        self.draw.Line(-xstart, posy, -xstart, yend)

        posy = yend
        xend = pad_s0_pos.x - (shl_width/2 + linewidth + margin*2)
        max_x = -xend
        self.draw.Line(xstart, posy, xend, posy)

        # right pad side
        self.draw.Line(-xstart, posy, -xend, yend)

        # set SMD attribute
        self.module.SetAttributes(pcbnew.FP_SMD)

        # vertical segment at left of the pad
        xstart = xend
        yend = posy - (shl_height + linewidth + margin*2)
        self.draw.Line(xstart, posy, xend, yend)

        # right pad side
        self.draw.Line(-xstart, posy, -xend, yend)
        max_y = max( max_y, posy )
        min_y = min( min_y, yend )

        # horizontal segment above the pad
        xstart = xend
        xend = - pad_pitch*0.5-offsetX
        posy = yend
        self.draw.Line(xstart, posy, xend, yend)

        # right pad side
        self.draw.Line(-xstart, posy,-xend, yend)

        # vertical segment above the pad
        xstart = xend
        yend = -pad_height/2 - linewidth/2 - margin
        min_y = min( min_y, yend )
        self.draw.Line(xstart, posy, xend, yend)

        # right pad side
        self.draw.Line(-xstart, posy, -xend, yend)

        # Courtyard
        self.draw.SetLayer(pcbnew.F_CrtYd)
        max_x = max_x + linewidth + margin
        min_y = min_y - linewidth - margin
        max_y = max_y + linewidth + margin

        # round size to nearest 0.1mm, rectangle will thus land on a 0.05mm grid
        max_x = pcbnew.PutOnGridMM(max_x, 0.05)
        max_y = pcbnew.PutOnGridMM(max_y, 0.05)
        min_y = pcbnew.PutOnGridMM(min_y, 0.05)

        # set courtyard line thickness to the one defined in KLC
        self.draw.SetLineThickness(pcbnew.FromMM(0.05))
        self.draw.Line( -max_x, min_y, max_x, min_y )
        self.draw.Line( max_x, min_y, max_x, max_y )
        self.draw.Line( -max_x, max_y, max_x, max_y )
        self.draw.Line( -max_x, min_y, -max_x, max_y )

FPC_FootprintWizard().register()
