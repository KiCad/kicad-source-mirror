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

from __future__ import division
import pcbnew

import HelpfulFootprintWizardPlugin
import PadArray as PA
import math


'''
Created on Jan 16, 2015

@author: ejohns
'''
from string import ascii_uppercase, digits

class Uss39:
    """
    """

    patternDict = {'1': [1, 0, 0, 1, 0, 0, 0, 0, 1],
                   '2': [0, 0, 1, 1, 0, 0, 0, 0, 1],
                   '3': [1, 0, 1, 1, 0, 0, 0, 0, 0],
                   '4': [0, 0, 0, 1, 1, 0, 0, 0, 1],
                   '5': [1, 0, 0, 1, 1, 0, 0, 0, 0],
                   '6': [0, 0, 1, 1, 1, 0, 0, 0, 0],
                   '7': [0, 0, 0, 1, 0, 0, 1, 0, 1],
                   '8': [1, 0, 0, 1, 0, 0, 1, 0, 0],
                   '9': [0, 0, 1, 1, 0, 0, 1, 0, 0],
                   '0': [0, 0, 0, 1, 1, 0, 1, 0, 0],
                   'A': [1, 0, 0, 0, 0, 1, 0, 0, 1],
                   'B': [0, 0, 1, 0, 0, 1, 0, 0, 1],
                   'C': [1, 0, 1, 0, 0, 1, 0, 0, 0],
                   'D': [0, 0, 0, 0, 1, 1, 0, 0, 1],
                   'E': [1, 0, 0, 0, 1, 1, 0, 0, 0],
                   'F': [0, 0, 1, 0, 1, 1, 0, 0, 0],
                   'G': [0, 0, 0, 0, 0, 1, 1, 0, 1],
                   'H': [1, 0, 0, 0, 0, 1, 1, 0, 0],
                   'I': [0, 0, 1, 0, 0, 1, 1, 0, 0],
                   'J': [0, 0, 0, 0, 1, 1, 1, 0, 0],
                   'K': [1, 0, 0, 0, 0, 0, 0, 1, 1],
                   'L': [0, 0, 1, 0, 0, 0, 0, 1, 1],
                   'M': [1, 0, 1, 0, 0, 0, 0, 1, 0],
                   'N': [0, 0, 0, 0, 1, 0, 0, 1, 1],
                   'O': [1, 0, 0, 0, 1, 0, 0, 1, 0],
                   'P': [0, 0, 1, 0, 1, 0, 0, 1, 0],
                   'Q': [0, 0, 0, 0, 0, 0, 1, 1, 1],
                   'R': [1, 0, 0, 0, 0, 0, 1, 1, 0],
                   'S': [0, 0, 1, 0, 0, 0, 1, 1, 0],
                   'T': [0, 0, 0, 0, 1, 0, 1, 1, 0],
                   'U': [1, 1, 0, 0, 0, 0, 0, 0, 1],
                   'V': [0, 1, 1, 0, 0, 0, 0, 0, 1],
                   'W': [1, 1, 1, 0, 0, 0, 0, 0, 0],
                   'X': [0, 1, 0, 0, 1, 0, 0, 0, 1],
                   'Y': [1, 1, 0, 0, 1, 0, 0, 0, 0],
                   'Z': [0, 1, 1, 0, 1, 0, 0, 0, 0],
                   '-': [0, 1, 0, 0, 0, 0, 1, 0, 1],
                   '.': [1, 1, 0, 0, 0, 0, 1, 0, 0],
                   ' ': [0, 1, 1, 0, 0, 0, 1, 0, 0],
                   '*': [0, 1, 0, 0, 1, 0, 1, 0, 0],
                   '$': [0, 1, 0, 1, 0, 1, 0, 0, 0],
                   '/': [0, 1, 0, 1, 0, 0, 0, 1, 0],
                   '+': [0, 1, 0, 0, 0, 1, 0, 1, 0],
                   '%': [0, 0, 0, 1, 0, 1, 0, 1, 0]}


    def makePrintable(self, text):
        """
        """
        output_string = ''

        # Capitalize string
        text = text.upper()

        # Remove unprintable text
        for c in text:
            output_string = output_string + c if self.patternDict.has_key(c) else output_string

        return output_string

    def getPattern(self, c):
        """
        """
        return self.patternDict[c]

    def getBarCodePattern(self, text = None):
        """
        """

        text = text if text is not None else self.Text
        bars = []
        output_string = ''

        # Reformat text
        text = self.makePrintable(text)

        # Append start and end characters
        output_string = '*' + text + '*'

        for c in output_string:
            bars = bars + self.getPattern(c)
            # Add intercharacter gap
            bars.append(0)

        # Remove last intercharacter gap
        bars = bars[:-1]

        return bars

    def __init__(self, text):
        """
        """
        self.Text = self.makePrintable(text)

    def __str__(self):
        """
        """
        return self.Text

class Uss39Wizard(HelpfulFootprintWizardPlugin.HelpfulFootprintWizardPlugin):
    """"""

    def GetName(self):
        """
        Return footprint name.

        This is specific to each footprint class, you need to implement
        this
        """
        return 'BARCODE USS-39'

    def GetDescription(self):
        """
        Return footprint description.

        This is specific to each footprint class, you need to implement
        this
        """
        return 'USS-39 Barcode'

    def GetReferencePrefix(self):
        """
        """
        return 'BARCODE'

    def GetValue(self):
        """
        """
        return self.module.Value().GetText()

    def GenerateParameterList(self):
        """"""
        # Silkscreen parameters

        self.AddParam("Barcode", "Pixel Width", self.uMM, 0.20)
        self.AddParam("Barcode", "Height", self.uMM, 3.0)
        self.AddParam("Barcode", "Margin", self.uMM, 2.0)
        self.AddParam("Barcode", "Contents", self.uString, 'BARCODE')
        self.AddParam("Caption", "Enabled", self.uBool, True)
        self.AddParam("Caption", "Height", self.uMM, 1.2)
        self.AddParam("Caption", "Thickness", self.uMM, 0.12)

    def CheckParameters(self):
        """
        """

        # Reset constants
        self.CourtyardLineWidth = pcbnew.FromMM(0.05)

        # Set bar height to the greater of 6.35mm or 0.15*L
        # Set quiet width to 10*X

        # User-defined parameters

        # Create barcode object
        self.Barcode = Uss39('=' + str(self.parameters['Barcode']['*Contents']))
        self.X = int(self.parameters['Barcode']['Pixel Width'])
        self.module.Value().SetText( str(self.Barcode) )

        self.C = len(str(self.Barcode))

        # Inter-character gap
        if self.X < 0.250:
            self.I = pcbnew.FromMM(3.15)
        else:
            self.I = (2 * self.X) if (2*self.X) > pcbnew.FromMM(1.35) else pcbnew.FromMM(1.35)

        # Wide to narrow ratio
        if self.X >= pcbnew.FromMM(0.508):
            self.N = pcbnew.FromMM(int((2.0+3.0)/2))
        else:
            self.N = pcbnew.FromMM(int((2.2+3.0)/2))

        self.H = self.parameters['Barcode']['Height']

        self.Q = (10 * self.X) if (10 * self.X) >  pcbnew.FromMM(6.35) else pcbnew.FromMM(6.35)

        self.L = self.I * (1 + self.C) + (self.C + 2) * (6 * self.X + 3 * self.N * self.X) + 2 * self.Q


    def __drawBar__(self, bit, x):
        """
        """
        offset = (bit + 1) * self.X
        return x + offset

    def __drawSpace__(self, bit, x):
        """
        """
        self.draw.SetLayer(pcbnew.F_SilkS)
        self.draw.SetWidth(self.X)
        self.draw.Line(x, 0, x, self.H)

        if (bit == 1):
            self.draw.Line(x + self.X, 0, x + self.X, self.H)
            self.draw.Line(x + self.X/2, 0, x + self.X/2, self.H)
            self.draw.Line(x, 0, x + self.X, 0)
            self.draw.Line(x, self.H, x + self.X, self.H)

        offset = (bit + 1) * self.X
        return x + offset

    def drawBars(self):
        """
        """
        x = 0
        bars = self.Barcode.getBarCodePattern()
        for index in range(0, len(bars), 2):
            # Draw bar
            barBit = bars[index]
            x = self.__drawBar__(barBit, x)

            # Draw space
            if index < len(bars)-1:
                spaceBit = bars[index + 1]
                x = self.__drawSpace__(spaceBit, x)
        return x

    def drawQuietZone(self, x0, y0, width, height):
        """
        """
        self.draw.SetLayer(pcbnew.F_SilkS)
        self.draw.SetWidth(self.X)


        for offset in range(0, int(self.Q), int(self.X/2)):
            xoffset = offset + self.X
            yoffset = offset + self.X/2
            self.draw.Line(x0 - xoffset, -yoffset, width + xoffset, -yoffset)
            self.draw.Line(x0 - xoffset, self.H+yoffset, width + xoffset, self.H+yoffset)
            self.draw.Line(x0 - xoffset, -yoffset, x0-xoffset, self.H+yoffset)
            self.draw.Line(width + xoffset, -yoffset, width+xoffset, self.H+yoffset)

    def BuildThisFootprint(self):
        """
        """

        # Draw bars
        x = self.drawBars()

        # Draw quiet zone
        self.drawQuietZone(0, 0, x, self.H)

        # Draw courtyard origin
        self.draw.SetLayer(pcbnew.F_CrtYd)
        self.draw.SetWidth(self.CourtyardLineWidth)
        ch_lim = pcbnew.FromMM(0.35)
        self.draw.Line(-ch_lim, 0, ch_lim, 0)
        self.draw.Line(0, -ch_lim, 0, ch_lim)
        self.draw.Circle(0, 0, pcbnew.FromMM(0.25))
        self.module.Value().SetLayer(pcbnew.F_Fab)

Uss39Wizard().register()
