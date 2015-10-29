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
import pcbnew as B

import HelpfulFootprintWizardPlugin

'''
Created on Jan 16, 2015

@author: ejohns
Rewritten by LordBlick
'''
ptd = {
    '0': '000110100', '1': '100100001', '2': '001100001', '3': '101100000',
    '4': '000110001', '5': '100110000', '6': '001110000', '7': '000100101',
    '8': '100100100', '9': '001100100', 'A': '100001001', 'B': '001001001',
    'C': '101001000', 'D': '000011001', 'E': '100011000', 'F': '001011000',
    'G': '000001101', 'H': '100001100', 'I': '001001100', 'J': '000011100',
    'K': '100000011', 'L': '001000011', 'M': '101000010', 'N': '000010011',
    'O': '100010010', 'P': '001010010', 'Q': '000000111', 'R': '100000110',
    'S': '001000110', 'T': '000010110', 'U': '110000001', 'V': '011000001',
    'W': '111000000', 'X': '010010001', 'Y': '110010000', 'Z': '011010000',
    '-': '010000101', '.': '110000100', ' ': '011000100', '*': '010010100',
    '$': '010101000', '/': '010100010', '+': '010001010', '%': '000101010'}

class Uss39:
     def __init__(self, text):
        self.Text = self.makePrintable(text)

     __str__ = lambda self: self.Text
     makePrintable = lambda self, text: ''.join((c for c in text.upper() if ptd.has_key(c)))

     def getBarCodePattern(self, text = None):
        text = text if not(text is None) else self.Text
        # Reformated text with start and end characters
        return reduce(lambda a1, a2: a1 + [0] + a2, [map(int, ptd[c]) for c in ("*%s*" % self.makePrintable(text))])

class Uss39Wizard(HelpfulFootprintWizardPlugin.HelpfulFootprintWizardPlugin):
    GetName = lambda self: 'BARCODE USS-39'
    GetDescription = lambda self: 'USS-39 Barcode'
    GetReferencePrefix = lambda self: 'BARCODE'
    GetValue = lambda self: self.module.Value().GetText()

    def GenerateParameterList(self):
        # Silkscreen parameters
        self.AddParam("Barcode", "Pixel Width", self.uMM, 0.20)
        self.AddParam("Barcode", "Height", self.uMM, 3.0)
        self.AddParam("Barcode", "Margin", self.uMM, 2.0)
        self.AddParam("Barcode", "Contents", self.uString, 'BARCODE')
        self.AddParam("Caption", "Enabled", self.uBool, True)
        self.AddParam("Caption", "Height", self.uMM, 1.2)
        self.AddParam("Caption", "Thickness", self.uMM, 0.12)

    def CheckParameters(self):
        # Reset constants
        self.CourtyardLineWidth = B.FromMM(0.05)
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
            self.I = B.FromMM(3.15)
        else:
            self.I = (2 * self.X) if (2*self.X) > B.FromMM(1.35) else B.FromMM(1.35)
        # Wide to narrow ratio
        if self.X >= B.FromMM(0.508):
            self.N = B.FromMM(int((2.0+3.0)/2))
        else:
            self.N = B.FromMM(int((2.2+3.0)/2))
        self.H = self.parameters['Barcode']['Height']
        self.Q = (10 * self.X) if (10 * self.X) >  B.FromMM(6.35) else B.FromMM(6.35)
        self.L = self.I * (1 + self.C) + (self.C + 2) * (6 * self.X + 3 * self.N * self.X) + 2 * self.Q


    def __drawBar__(self, bit, x):
        offset = (bit + 1) * self.X
        return x + offset

    def __drawSpace__(self, bit, x):
        self.draw.SetLayer(B.F_SilkS)
        self.draw.SetLineTickness(self.X)
        self.draw.Line(x, 0, x, self.H)
        if (bit == 1):
            self.draw.Line(x + self.X, 0, x + self.X, self.H)
            self.draw.Line(x + self.X/2, 0, x + self.X/2, self.H)
            self.draw.Line(x, 0, x + self.X, 0)
            self.draw.Line(x, self.H, x + self.X, self.H)
        offset = (bit + 1) * self.X
        return x + offset

    def drawBars(self):
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
        self.draw.SetLayer(B.F_SilkS)
        self.draw.SetLineTickness(self.X)

        for offset in range(0, int(self.Q), int(self.X/2)):
            xoffset = offset + self.X
            yoffset = offset + self.X/2
            self.draw.Line(x0 - xoffset, -yoffset, width + xoffset, -yoffset)
            self.draw.Line(x0 - xoffset, self.H+yoffset, width + xoffset, self.H+yoffset)
            self.draw.Line(x0 - xoffset, -yoffset, x0-xoffset, self.H+yoffset)
            self.draw.Line(width + xoffset, -yoffset, width+xoffset, self.H+yoffset)

    def BuildThisFootprint(self):
        # Draw bars
        x = self.drawBars()
        # Draw quiet zone
        self.drawQuietZone(0, 0, x, self.H)
        # Draw courtyard origin
        self.draw.SetLayer(B.F_CrtYd)
        self.draw.SetLineTickness(self.CourtyardLineWidth)
        ch_lim = B.FromMM(0.35)
        self.draw.Line(-ch_lim, 0, ch_lim, 0)
        self.draw.Line(0, -ch_lim, 0, ch_lim)
        self.draw.Circle(0, 0, B.FromMM(0.25))
        self.module.Value().SetLayer(B.F_Fab)

Uss39Wizard().register()
