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

#  last change: 2017, Jan 4.

import pcbnew
import FootprintWizardBase

# Additional import for QRCode
# see https://github.com/kazuhikoarase/qrcode-generator/blob/master/python/qrcode.py
import kicad_qrcode as qrcode  # TODO: local qrcode package is prefered, so we renamed it

class QRCodeWizard(FootprintWizardBase.FootprintWizard):
    GetName = lambda self: '2D Barcode QRCode'
    GetDescription = lambda self: 'QR Code barcode generator'
    GetReferencePrefix = lambda self: 'QR***'
    GetValue = lambda self: self.module.Value().GetText()

    def GenerateParameterList(self):
        self.AddParam("Barcode", "Qr Pixel Width", self.uMM, 0.5, min_value=0.4)
        self.AddParam("Barcode", "Border Margin (Px)", self.uInteger, 0)
        self.AddParam("Barcode", "Contents", self.uString, 'Example')
        self.AddParam("Barcode", "Negative", self.uBool, False)
        self.AddParam("Barcode", "Use SilkS layer", self.uBool, False)
        self.AddParam("Barcode", "Use Cu layer", self.uBool, True)
        self.AddParam("Caption", "Enabled", self.uBool, True)
        self.AddParam("Caption", "Height", self.uMM, 1.0)
        self.AddParam("Caption", "Width", self.uMM, 1.0)
        self.AddParam("Caption", "Thickness", self.uMM, 0.15)


    def CheckParameters(self):
        # 512 bits maximum in this type of QR code with 2 bytes reserved
        self.Barcode = str(self.parameters['Barcode']['Contents'])[:61]
        self.X = self.parameters['Barcode']['Qr Pixel Width']
        self.negative = self.parameters['Barcode']['Negative']
        self.UseSilkS = self.parameters['Barcode']['Use SilkS layer']
        self.UseCu = self.parameters['Barcode']['Use Cu layer']
        self.border = int(self.parameters['Barcode']['Border Margin (Px)'])
        self.textHeight = int(self.parameters['Caption']['Height'])
        self.textThickness = int(self.parameters['Caption']['Thickness'])
        self.textWidth = int(self.parameters['Caption']['Width'])
        self.module.Value().SetText(str(self.Barcode))

        if self.border < 0:
            self.border = 0

        # Build Qrcode
        self.qr = qrcode.QRCode()
        self.qr.setTypeNumber(4)
        # ErrorCorrectLevel: L = 7%, M = 15% Q = 25% H = 30%
        self.qr.setErrorCorrectLevel(qrcode.ErrorCorrectLevel.M)
        self.qr.addData(str(self.Barcode))
        self.qr.make()

    def drawPixelSquareArea( self, layer, size, xposition, yposition):
        # creates a EDGE_MODULE of polygon type. The polygon is a square
        polygon = pcbnew.EDGE_MODULE(self.module)
        polygon.SetShape(pcbnew.S_POLYGON)
        polygon.SetWidth( 0 )
        polygon.SetLayer(layer)
        halfsize = int(size/2)
        polygon.GetPolyShape().NewOutline();
        polygon.GetPolyShape().Append( halfsize+xposition, halfsize+yposition )
        polygon.GetPolyShape().Append( halfsize+xposition, -halfsize+yposition )
        polygon.GetPolyShape().Append( -halfsize+xposition, -halfsize+yposition )
        polygon.GetPolyShape().Append( -halfsize+xposition, halfsize+yposition )
        return polygon


    def _drawQrPixel(self, xposition, yposition):
        # build a rectangular pad as a dot on copper layer,
        # and a polygon (a square) on silkscreen
        if self.UseCu:
            pad = pcbnew.D_PAD(self.module)
            pad.SetSize(pcbnew.wxSize(self.X, self.X))
            pad_pos = pcbnew.wxPoint(xposition,yposition)
            pad.SetPosition(pad_pos)
            pad.SetPos0(pad_pos)
            pad.SetShape(pcbnew.PAD_SHAPE_RECT)
            pad.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
            pad.SetName("")
            layerset = pcbnew.LSET()
            layerset.AddLayer(pcbnew.F_Cu)
            pad.SetLayerSet( layerset )
            self.module.Add(pad)
        if self.UseSilkS:
            polygon=self.drawPixelSquareArea(pcbnew.F_SilkS, self.X, xposition, yposition)
            self.module.Add(polygon)


    def BuildThisFootprint(self):
        if self.border >= 0:
            # Adding border: Create a new array larger than the self.qr.modules
            sz = self.qr.modules.__len__() + (self.border * 2)
            arrayToDraw = [ [ 0 for a in range(sz) ] for b in range(sz) ]
            lineposition = self.border

            for i in self.qr.modules:
                columnposition = self.border
                for j in i:
                    arrayToDraw[lineposition][columnposition] = j
                    columnposition += 1
                lineposition += 1
        else:
            # No border: using array as is
            arrayToDraw = self.qr.modules

        # used many times...
        half_number_of_elements = arrayToDraw.__len__() / 2
        area_width =  arrayToDraw.__len__()*self.X + self.border*2

        # Center position of QrCode
        yposition = - int(half_number_of_elements * self.X)
        area_height =  arrayToDraw.__len__()*self.X + self.border*2

        for line in arrayToDraw:
            xposition = - int(half_number_of_elements * self.X)
            for pixel in line:
                # Trust table for drawing a pixel
                # Negative is a boolean;
                # each pixel is a boolean (need to draw of not)
                # Negative | Pixel | Result
                #        0 |     0 | 0
                #        0 |     1 | 1
                #        1 |     0 | 1
                #        1 |     1 | 0
                # => Draw as Xor
                if self.negative != pixel: # Xor...
                    self._drawQrPixel(xposition, yposition)
                xposition += self.X
            yposition += self.X

        # Add value field
        textPosition = int((self.textHeight) + ((1 + half_number_of_elements) * self.X))
        pos = pcbnew.wxPoint(0, - textPosition)
        self.module.Value().SetPosition(pos)
        self.module.Value().SetPos0(pos)
        self.module.Value().SetTextHeight(self.textHeight)
        self.module.Value().SetTextWidth(self.textWidth)
        self.module.Value().SetThickness(self.textThickness)

        # Add Reference field
        pos = pcbnew.wxPoint(0, textPosition)
        self.module.Reference().SetPosition(pos)
        self.module.Reference().SetPos0(pos)
        self.module.Reference().SetTextHeight(self.textHeight)
        self.module.Reference().SetTextWidth(self.textWidth)
        self.module.Reference().SetThickness(self.textThickness)
        self.module.Value().SetLayer(pcbnew.F_SilkS)

        #build the footprint courtyard
        self.draw.SetLayer( pcbnew.F_CrtYd )
        self.draw.SetLineThickness( pcbnew.FromMM( 0.05 ) ) #Default per KLC F5.1 as of 12/2018
        cr_margin = pcbnew.FromMM( 0.1 )
        self.draw.Box(0, 0, area_width + cr_margin*2, area_height + cr_margin*2)

        #build the footprint solder mask: the solder mask covers all copper pads
        if self.UseCu:
            self.draw.SetLineThickness( 0 )
            sm_margin = pcbnew.FromMM( 0.05 )
            polygon=self.drawPixelSquareArea(pcbnew.F_Mask, area_width + sm_margin*2, 0, 0)
            self.module.Add(polygon)


QRCodeWizard().register()
