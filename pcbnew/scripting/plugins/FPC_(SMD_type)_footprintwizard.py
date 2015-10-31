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


class FPC_FootprintWizard(HFPW.HelpfulFootprintWizardPlugin):

    def GetName(self):
        return "FPC (SMT connector)"

    def GetDescription(self):
        return "FPC (SMT connector) Footprint Wizard"

    def GetValue(self):
        pins = self.parameters["Pads"]["*n"]
        return "FPC_%d" % pins

    def GenerateParameterList(self):
        self.AddParam( "Pads", "n", self.uNatural, 40 )
        self.AddParam( "Pads", "pitch", self.uMM, 0.5 )
        self.AddParam( "Pads", "width", self.uMM, 0.25 )
        self.AddParam( "Pads", "height", self.uMM, 1.6)
        self.AddParam( "Shield", "shield_to_pad", self.uMM, 1.6 )
        self.AddParam( "Shield", "from_top", self.uMM, 1.3 )
        self.AddParam( "Shield", "width", self.uMM, 1.5 )
        self.AddParam( "Shield", "height", self.uMM, 2 )


    # build a rectangular pad
    def smdRectPad(self,module,size,pos,name):
        pad = pcbnew.D_PAD(module)
        pad.SetSize(size)
        pad.SetShape(pcbnew.PAD_SHAPE_RECT)
        pad.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
        pad.SetLayerSet( pad.SMDMask() )
        pad.SetPos0(pos)
        pad.SetPosition(pos)
        pad.SetPadName(name)
        return pad

    def CheckParameters(self):
        p = self.parameters
        self.CheckParamInt( "Pads", "*n" )  # not internal units preceded by "*"


    def BuildThisFootprint(self):
        p = self.parameters
        pad_count       = int(p["Pads"]["*n"])
        pad_width       = p["Pads"]["width"]
        pad_height      = p["Pads"]["height"]
        pad_pitch       = p["Pads"]["pitch"]
        shl_width       = p["Shield"]["width"]
        shl_height      = p["Shield"]["height"]
        shl_to_pad      = p["Shield"]["shield_to_pad"]
        shl_from_top    = p["Shield"]["from_top"]

        offsetX         = pad_pitch * ( pad_count-1 ) / 2
        size_pad = pcbnew.wxSize( pad_width, pad_height )
        size_shld = pcbnew.wxSize(shl_width, shl_height)
        size_text = self.GetTextSize()  # IPC nominal

        # Gives a position and size to ref and value texts:
        textposy = pad_height/2 + pcbnew.FromMM(1) + self.GetTextThickness()
        self.draw.Reference( 0, textposy, size_text )

        textposy = textposy + size_text + self.GetTextThickness()
        self.draw.Value( 0, textposy, size_text )

        # create a pad array and add it to the module
        for n in range ( 0, pad_count ):
            xpos = pad_pitch*n - offsetX
            pad = self.smdRectPad(self.module,size_pad, pcbnew.wxPoint(xpos,0),str(n+1))
            self.module.Add(pad)


        # Mechanical shield pads: left pad and right pad
        xpos = -shl_to_pad-offsetX
        pad_s0_pos = pcbnew.wxPoint(xpos,shl_from_top)
        pad_s0 = self.smdRectPad(self.module, size_shld, pad_s0_pos, "0")
        xpos = (pad_count-1) * pad_pitch+shl_to_pad - offsetX
        pad_s1_pos = pcbnew.wxPoint(xpos,shl_from_top)
        pad_s1 = self.smdRectPad(self.module, size_shld, pad_s1_pos, "0")

        self.module.Add(pad_s0)
        self.module.Add(pad_s1)

        # add footprint outline
        linewidth = self.draw.GetLineTickness()
        margin = linewidth

        # upper line
        posy = -pad_height/2 - linewidth/2 - margin
        xstart = - pad_pitch*0.5-offsetX
        xend = pad_pitch * pad_count + xstart;
        self.draw.Line( xstart, posy, xend, posy )

        # lower line
        posy = pad_height/2 + linewidth/2 + margin
        self.draw.Line(xstart, posy, xend, posy)

        # around left mechanical pad (the outline around right pad is mirrored/y axix)
        yend = pad_s0_pos.y + shl_height/2 + margin
        self.draw.Line(xstart, posy, xstart, yend)
        self.draw.Line(-xstart, posy, -xstart, yend)

        posy = yend
        xend = pad_s0_pos.x - (shl_width/2 + linewidth + margin*2)
        self.draw.Line(xstart, posy, xend, posy)

        # right pad side
        self.draw.Line(-xstart, posy, -xend, yend)

        # vertical segment at left of the pad
        xstart = xend
        yend = posy - (shl_height + linewidth + margin*2)
        self.draw.Line(xstart, posy, xend, yend)

        # right pad side
        self.draw.Line(-xstart, posy, -xend, yend)

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
        self.draw.Line(xstart, posy, xend, yend)

        # right pad side
        self.draw.Line(-xstart, posy, -xend, yend)


FPC_FootprintWizard().register()
