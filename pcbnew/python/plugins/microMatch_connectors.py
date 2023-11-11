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

import FootprintWizardBase as FPWbase
import PadArray as PA

class PadStaggeredZGridArray(PA.PadArray):
    """ Creates a staggered array of Pads with Z pad naming

                | pad pitch
                |   |
                o   o   o   o     ----
    staggered --> o   o   o   o   -- line_pitch
    line        o   o   o   o
                  o   o   o   o
    """
    def __init__(self, aPad, aPadCount, aLineCount, aLinePitch,
                 aPadPitch, aStagger=0, aCentre=pcbnew.VECTOR2I(0, 0)):
        """
        @param aPad         Template for all pads
        @param aPadCount    Overall pad count
        @param aLineCount   Number of lines
        @param aLinePitch   distance between lines
        @param aPadPitch    distance between pads
        @param aStagger     X stagger value for all odd lines
        @param aCentre      Center position

        """
        super(PadStaggeredZGridArray, self).__init__(aPad)

        self.padCount = int(aPadCount)
        self.lineCount = int(aLineCount)
        self.linePitch = aLinePitch
        self.padPitch = aPadPitch
        self.stagger = aStagger
        self.centre = aCentre


    # right to left, top to bottom
    def NamingFunction(self, aPadPos):
        return self.firstPadNum + aPadPos

    #relocate the pad and add it as many times as we need
    def AddPadsToModule(self, dc):
        pin1posX = self.centre.x - ((self.padPitch * (self.padCount // 2 - 1)) + self.stagger) / 2
        pin1posY = self.centre.y - self.linePitch * (self.lineCount - 1) / 2
        line = 0

        for padnum in range(0, self.padCount):
            if (line % 2) == 0:
                posX = pin1posX + ((padnum // 2) * self.padPitch)
            else:
                posX = pin1posX + self.stagger + ((padnum // 2) * self.padPitch)

            posY = pin1posY + (self.linePitch * line)

            pos = dc.TransformPoint(posX, posY)
            pad = self.GetPad(padnum == 0, pos)
            pad.SetName(self.GetName(padnum))
            self.AddPad(pad)

            line += 1

            if line >= self.lineCount:
                line = 0


class MicroMaTchWizard(FPWbase.FootprintWizard):
    padCountKey           = 'pad count'
    rowSpacingKey         = 'row spacing'
    padLengthKey          = 'pad length'
    padWidthKey           = 'pad width'
    padPitchKey           = 'pad pitch'
    staggerOffsetKey      = 'stagger_offset'
    withLockKey           = 'draw lock'

    def GetName(self):
        return "Micromatch SMD connectors"

    def GetDescription(self):
        return "Micromatch (with lock and w/o lock), footprint wizard"

    def GenerateParameterList(self):
        # defaults for a Micromatch package
        self.AddParam("Body", self.withLockKey,       self.uBool, True)
        self.AddParam("Pads", self.padCountKey,       self.uInteger, 8, multiple=2)

        #and override some of them
        self.AddParam("Pads", self.padWidthKey,       self.uMM, 1.5)
        self.AddParam("Pads", self.padLengthKey,      self.uMM, 3.0)
        self.AddParam("Pads", self.padPitchKey,       self.uMM, 2.54)
        self.AddParam("Pads", self.rowSpacingKey,     self.uMM, 5.2)
        self.AddParam("Pads", self.staggerOffsetKey,  self.uMM, 1.27)

    def CheckParameters(self):
        pass    # All checks are already taken care of!

    def GetValue(self):
        pad_count = self.parameters["Pads"][self.padCountKey]
        return "%s-%d" % ("ConnectorMicromatch", pad_count)

    def GetPad(self):
        padLength = self.parameters["Pads"][self.padLengthKey]
        padWidth  = self.parameters["Pads"][self.padWidthKey]
        return PA.PadMaker(self.module).SMDPad(
            padLength, padWidth, shape=pcbnew.PAD_SHAPE_RECT)

    def BuildThisFootprint(self):
        pads = self.parameters["Pads"]
        body = self.parameters["Body"]
        numPads = pads[self.padCountKey]
        padLength = pads[self.padLengthKey]
        rowPitch = pads[self.rowSpacingKey]
        padPitch = pads[self.padPitchKey]
        staggerOffset = pads[self.staggerOffsetKey]

        drawWithLock = body[self.withLockKey]
        numRows = 2

        # Use value to fill the modules description
        desc = self.GetValue()
        self.module.SetLibDescription(desc)
        self.module.SetAttributes( pcbnew.FP_SMD )

        # add in the pads
        pad = self.GetPad()

        array = PadStaggeredZGridArray(pad, numPads, numRows, rowPitch, padPitch, staggerOffset)
        array.AddPadsToModule(self.draw)

        # Draw connector outlineChassis
        width = pcbnew.FromMM(1.92) + (numPads * padPitch) / 2
        height = pcbnew.FromMM(5)

        self.draw.SetLineThickness( pcbnew.FromMM( 0.12 ) ) #Default per KLC F5.1 as of 12/2018

        # Left part
        #  --
        #  |
        #  ----
        self.draw.Polyline([(-width/2 + pcbnew.FromMM(0.5), -height/2),
                            (-width/2, -height/2),
                            (-width/2, height/2),
                            (-width/2 + pcbnew.FromMM(0.5) + padPitch / 2, height/2)])

        if drawWithLock :
            # Right part with pol slot
            #  ----
            #     [
            #    --
            self.draw.Polyline([(width/2 - pcbnew.FromMM(0.5) - padPitch / 2, -height/2),
                                (width/2,                                     -height/2),
                                (width/2,                                     -height/2 + pcbnew.FromMM(1.25)),
                                (width/2 - pcbnew.FromMM(0.7),                -height/2 + pcbnew.FromMM(1.25)),
                                (width/2 - pcbnew.FromMM(0.7),                 height/2 - pcbnew.FromMM(1.25)),
                                (width/2,                                      height/2 - pcbnew.FromMM(1.25)),
                                (width/2,                                      height/2),
                                (width/2 - pcbnew.FromMM(0.5),                 height/2)])
        else:
            # Right part without pol slot
            #  ----
            #     |
            #    --
            self.draw.Polyline([(width/2 - pcbnew.FromMM(0.5) - padPitch / 2, -height/2),
                                (width/2,                                     -height/2),
                                (width/2,                                      height/2),
                                (width/2 - pcbnew.FromMM(0.5),                 height/2)])

        # Courtyard
        self.draw.SetLayer(pcbnew.F_CrtYd)
        self.draw.SetLineThickness(pcbnew.FromMM(0.05))
        boxW = width                + pcbnew.FromMM(0.5)
        boxH = rowPitch + padLength + pcbnew.FromMM(0.5)

        # Round courtyard positions to 0.1 mm, rectangle will thus land on a 0.05mm grid
        pcbnew.PutOnGridMM(boxW, pcbnew.FromMM(0.10))
        pcbnew.PutOnGridMM(boxH, pcbnew.FromMM(0.10))
        self.draw.Box(0,0, boxW, boxH)

        #reference and value
        text_size = pcbnew.FromMM(1.0)  # According KLC
        text_offset = boxH/2 + text_size

        self.draw.Value(0, text_offset, text_size)
        self.draw.Reference(0, -text_offset, text_size)

        # Add a extra text (${REFERENCE}) on the F_Fab layer
        extra_text = pcbnew.PCB_TEXT( self.module )
        extra_text.SetLayer( pcbnew.F_Fab )
        extra_text.SetPosition( pcbnew.VECTOR2I( 0, 0) )
        extra_text.SetTextSize( pcbnew.VECTOR2I( text_size, text_size ) )
        extra_text.SetText( "${REFERENCE}" )
        self.module.Add( extra_text )


MicroMaTchWizard().register()
