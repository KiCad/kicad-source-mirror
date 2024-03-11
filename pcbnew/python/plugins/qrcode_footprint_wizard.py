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

#  last change: 2021, Jul 19.

import pcbnew
import FootprintWizardBase

# Additional import for QRCode
# see https://github.com/kazuhikoarase/qrcode-generator/blob/master/python/qrcode.py
import kicad_qrcode as qrcode  # TODO: local qrcode package is preferred, so we renamed it

# Max type number for the QRCode is 10, this is restricted by the upstream library
# https://github.com/kazuhikoarase/qrcode-generator
_PYTHON_QR_LIB_MAX_TYPE_NO = len(qrcode.QRUtil.MAX_LENGTH)

class QRCodeWizard(FootprintWizardBase.FootprintWizard):
    GetName = lambda self: '2D Barcode QRCode'
    GetDescription = lambda self: 'QR Code barcode generator'
    GetReferencePrefix = lambda self: 'QR***'
    GetValue = lambda self: self.module.Value().GetText()

    def GenerateParameterList(self):
        # TODO: add params for ECLevel and qrcode mode (num/alnum/bytes)
        self.AddParam("Barcode", "Qr Pixel Width", self.uMM, 0.5)
        self.AddParam(
            "Barcode",
            "Border Margin (Px)",
            self.uInteger,
            0,
            min_value=0,
        )
        # ErrorCorrectLevel: L = 7%, M = 15% Q = 25% H = 30%
        self.AddParam(
            "Barcode",
            "Error Correction Level",
            self.uString,
            "M",
            hint="One of L(7%), M(15%), Q(25%), H(30%)"
        )
        self.AddParam(
            "Barcode",
            "Type Number",
            self.uInteger,
            0,
            min_value=0,
            max_value=_PYTHON_QR_LIB_MAX_TYPE_NO,
            hint="Set to 0 for autodetect (will use the smallest possible)",
        )
        self.AddParam("Barcode", "Contents", self.uString, 'Example')
        self.AddParam("Barcode", "Negative", self.uBool, False)
        self.AddParam("Barcode", "Use SilkS layer", self.uBool, False)
        self.AddParam("Barcode", "Use Cu layer", self.uBool, True)
        self.AddParam("Caption", "Enabled", self.uBool, True)
        self.AddParam("Caption", "Height", self.uMM, 1.0)
        self.AddParam("Caption", "Width", self.uMM, 1.0)
        self.AddParam("Caption", "Thickness", self.uMM, 0.15)


    def CheckParameters(self):
        self.Barcode = self.parameters['Barcode']['Contents']
        self.RawECLevel = str(self.parameters['Barcode']['Error Correction Level']).upper()
        self.TypeNumber = self.parameters['Barcode']['Type Number']
        self.X = self.parameters['Barcode']['Qr Pixel Width']
        self.negative = self.parameters['Barcode']['Negative']
        self.UseSilkS = self.parameters['Barcode']['Use SilkS layer']
        self.UseCu = self.parameters['Barcode']['Use Cu layer']
        self.border = int(self.parameters['Barcode']['Border Margin (Px)'])
        self.textHeight = int(self.parameters['Caption']['Height'])
        self.textThickness = int(self.parameters['Caption']['Thickness'])
        self.textWidth = int(self.parameters['Caption']['Width'])
        self.module.Value().SetText(str(self.Barcode))


        if not (len(self.RawECLevel) == 1 and self.RawECLevel in "LMQH"):
            self.GetParam("Barcode", "Error Correction Level").AddError(
                '"Error Correction Level" must be one of L(7%), M(15%), Q(25%), H(30%)'
            )
            self.ECLevel = qrcode.ErrorCorrectLevel.M
        else:
            self.ECLevel = getattr(qrcode.ErrorCorrectLevel, self.RawECLevel)


        # Check if the content is too long
        # technically we don't need this conversion (TypeNumber=0 will be
        # regarded as max) but this isn't part of the documented API, so we'll
        # convert to be safe
        max_type_num = (
            _PYTHON_QR_LIB_MAX_TYPE_NO
            if self.TypeNumber == 0
            else self.TypeNumber
        )
        max_length = qrcode.QRUtil.getMaxLength(
            max_type_num,
            qrcode.Mode.MODE_8BIT_BYTE,
            self.ECLevel
        )
        if len(self.Barcode) > max_length:
            self.GetParam("Barcode", "Contents").AddError(
                f"Content too long ({len(self.Barcode)} > {max_length}) for"
                " the provided parameters."
            )


    def drawPixelSquareArea( self, layer, size, xposition, yposition):
        # creates a PCB_SHAPE of rectangle type. The rectangle is square
        rectangle = pcbnew.PCB_SHAPE(self.module)
        rectangle.SetShape(pcbnew.S_RECT)
        rectangle.SetWidth( 0 )
        rectangle.SetFilled( True )
        rectangle.SetLayer(layer)
        halfsize = int(size/2)
        rectangle.SetStartX( -halfsize+xposition )
        rectangle.SetStartY( -halfsize+yposition )
        rectangle.SetEndX( halfsize+xposition )
        rectangle.SetEndY( halfsize+yposition )
        return rectangle


    def _drawQrPixel(self, xposition, yposition):
        # build a rectangular pad as a dot on copper layer,
        # and a rectangle (a square) on silkscreen
        if self.UseCu:
            pad = pcbnew.PAD(self.module)
            pad.SetSize(pcbnew.VECTOR2I(self.X, self.X))
            pad_pos = pcbnew.VECTOR2I(xposition,yposition)
            pad.SetPosition(pad_pos)
            pad.SetShape(pcbnew.PAD_SHAPE_RECT)
            pad.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
            pad.SetName("")
            layerset = pcbnew.LSET()
            layerset.AddLayer(pcbnew.F_Cu)
            pad.SetLayerSet( layerset )
            self.module.Add(pad)
        if self.UseSilkS:
            rectangle=self.drawPixelSquareArea(pcbnew.F_SilkS, self.X, xposition, yposition)
            self.module.Add(rectangle)


    def BuildThisFootprint(self):
        # Build QRCode
        # Auto detect type number
        if self.TypeNumber == 0:
            self.qr = qrcode.QRCode.getMinimumQRCode(self.Barcode, self.ECLevel)
        # Manually specified type number
        else:
            self.qr = qrcode.QRCode()
            self.qr.setTypeNumber(self.TypeNumber)
            self.qr.setErrorCorrectLevel(self.ECLevel)
            self.qr.addData(self.Barcode)
            self.qr.make()

        # render QRCode
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
        yposition = - int(half_number_of_elements * self.X) + int(self.X/2)
        area_height =  arrayToDraw.__len__()*self.X + self.border*2

        for line in arrayToDraw:
            xposition = - int(half_number_of_elements * self.X) + int(self.X/2)
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
        pos = pcbnew.VECTOR2I(0, - textPosition)
        self.module.Value().SetPosition(pos)
        self.module.Value().SetTextHeight(self.textHeight)
        self.module.Value().SetTextWidth(self.textWidth)
        self.module.Value().SetTextThickness(self.textThickness)

        # Add Reference field
        pos = pcbnew.VECTOR2I(0, textPosition)
        self.module.Reference().SetPosition(pos)
        self.module.Reference().SetTextHeight(self.textHeight)
        self.module.Reference().SetTextWidth(self.textWidth)
        self.module.Reference().SetTextThickness(self.textThickness)
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
            rectangle=self.drawPixelSquareArea(pcbnew.F_Mask, area_width + sm_margin*2, 0, 0)
            self.module.Add(rectangle)


QRCodeWizard().register()
