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

import pcbnew
import FootprintWizardBase
import PadArray as PA

class QFNWizard(FootprintWizardBase.FootprintWizard):

    def GetName(self):
        return "QFN"

    def GetDescription(self):
        return "Quad Flat No-lead (QFN) footprint wizard"

    def GenerateParameterList(self):

        self.AddParam("Pads", "nx", self.uInteger, 25)
        self.AddParam("Pads", "ny", self.uInteger, 25, min_value=1)
        self.AddParam("Pads", "pitch", self.uMM, 0.4, designator='e')
        self.AddParam("Pads", "width", self.uMM, 0.2, designator='X1')
        self.AddParam("Pads", "length", self.uMM, 0.75, designator='Y1')
        self.AddParam("Pads", "offset", self.uMM, 0.0)
        self.AddParam("Pads", "oval", self.uBool, True)

        self.AddParam("EPad", "epad", self.uBool, True)
        self.AddParam("EPad", "width", self.uMM, 10, designator="E2")
        self.AddParam("EPad", "length", self.uMM, 10, designator="D2")
        self.AddParam("EPad", "thermal vias", self.uBool, False)
        self.AddParam("EPad", "thermal vias drill", self.uMM, 0.6, min_value=0.1)
        self.AddParam("EPad", "x divisions", self.uInteger, 4, min_value=1)
        self.AddParam("EPad", "y divisions", self.uInteger, 4, min_value=1)
        self.AddParam("EPad", "paste margin", self.uMM, 0.75)

        self.AddParam("Package", "width", self.uMM, 14, designator='E')
        self.AddParam("Package", "height", self.uMM, 14, designator='D')
        self.AddParam("Package", "margin", self.uMM, 0.25, minValue=0.2)

    @property
    def pads(self):
        return self.parameters['Pads']

    @property
    def epad(self):
        return self.parameters['EPad']

    @property
    def package(self):
        return self.parameters['Package']

    def CheckParameters(self):
        pass

    def GetValue(self):

        return "QFN-{n}_{ep}{x:g}x{y:g}_Pitch{p:g}mm".format(
                n = (self.pads['nx'] + self.pads['ny']) * 2,
                ep = "EP_" if self.epad['epad'] else '',
                x = pcbnew.ToMM(self.package['width']),
                y = pcbnew.ToMM(self.package['height']),
                p = pcbnew.ToMM(self.pads['pitch'])
                )

    def BuildThisFootprint(self):

        pad_pitch = self.pads["pitch"]
        pad_length = self.pads["length"]
        # offset allows one to define how much of the pad is outside of the package
        pad_offset = self.pads["offset"]
        pad_width = self.pads["width"]

        v_pitch = self.package["height"]
        h_pitch = self.package["width"]

        v_pads_per_row = int(self.pads["ny"])
        h_pads_per_row = int(self.pads["nx"])

        v_row_len = (v_pads_per_row - 1) * pad_pitch
        h_row_len = (h_pads_per_row - 1) * pad_pitch

        pad_shape = pcbnew.PAD_SHAPE_OVAL if self.pads["oval"] else pcbnew.PAD_SHAPE_RECT

        h_pad = PA.PadMaker(self.module).SMDPad( pad_length, pad_width,
                                                 shape=pad_shape, rot_degree=90.0)
        v_pad = PA.PadMaker(self.module).SMDPad( pad_length, pad_width, shape=pad_shape)

        h_pitch = (int)(h_pitch / 2 - pad_length + pad_offset + pad_length/2)
        v_pitch = (int)(v_pitch / 2 - pad_length + pad_offset + pad_length/2)

        #left row
        pin1Pos = pcbnew.VECTOR2I( -h_pitch, 0)
        array = PA.PadLineArray(h_pad, v_pads_per_row, pad_pitch, True, pin1Pos)
        array.SetFirstPadInArray(1)
        array.AddPadsToModule(self.draw)

        #bottom row
        pin1Pos = pcbnew.VECTOR2I(0, v_pitch)
        array = PA.PadLineArray(v_pad, h_pads_per_row, pad_pitch, False, pin1Pos)
        array.SetFirstPadInArray(v_pads_per_row + 1)
        array.AddPadsToModule(self.draw)

        #right row
        pin1Pos = pcbnew.VECTOR2I(h_pitch, 0)
        array = PA.PadLineArray(h_pad, v_pads_per_row, -pad_pitch, True,
                                pin1Pos)
        array.SetFirstPadInArray(v_pads_per_row + h_pads_per_row + 1)
        array.AddPadsToModule(self.draw)

        #top row
        pin1Pos = pcbnew.VECTOR2I(0, -v_pitch)
        array = PA.PadLineArray(v_pad, h_pads_per_row, -pad_pitch, False,
                                pin1Pos)
        array.SetFirstPadInArray(2*v_pads_per_row + h_pads_per_row + 1)
        array.AddPadsToModule(self.draw)

        lim_x = self.package["width"] / 2
        lim_y = self.package["height"] / 2

        # epad
        epad_width   = self.epad["width"]
        epad_length  = self.epad["length"]

        aper_pad_ny = self.epad["x divisions"]
        aper_pad_nx = self.epad["y divisions"]

        epad_via_drill = self.epad["thermal vias drill"]

        # Create a central exposed pad?
        if self.epad['epad'] == True:

            epad_num = (self.pads['nx'] + self.pads['ny']) * 2 + 1

            aper_pad_w = epad_length / aper_pad_nx
            aper_pad_l = epad_width / aper_pad_ny
            paste_margin = self.epad['paste margin']

            if paste_margin >= aper_pad_w:
                paste_margin = aper_pad_w -1

            if paste_margin >= aper_pad_l:
                paste_margin = aper_pad_l -1

            # Create the epad
            aperture_pad = PA.PadMaker(self.module).AperturePad( aper_pad_w-paste_margin, aper_pad_l-paste_margin,
                shape=pcbnew.PAD_SHAPE_RECT )
            epad = PA.PadMaker(self.module).SMDPad( epad_length, epad_width,
                    shape=pcbnew.PAD_SHAPE_RECT )
            # set exposed pad layers
            layers = pcbnew.LSET()
            layers.AddLayer(pcbnew.F_Mask)
            layers.AddLayer( pcbnew.F_Cu )
            epad.SetLayerSet( layers )
            epad.SetPosition( pcbnew.VECTOR2I(0,0) )
            epad.SetName( epad_num )
            self.module.Add( epad )

            array = PA.EPADGridArray( aperture_pad, aper_pad_ny, aper_pad_nx, aper_pad_l, aper_pad_w, pcbnew.VECTOR2I(0,0) )
            array.AddPadsToModule(self.draw)

            if self.epad['thermal vias']:

                # create the thermal via
                via_diam = min(aper_pad_w, aper_pad_l) / 2
                via_drill = min(via_diam / 2, epad_via_drill)
                via = PA.PadMaker(self.module).THRoundPad(via_diam, via_drill)
                # A thermal via must have the PAD_PROP_HEATSINK set.
                via.SetProperty( pcbnew.PAD_PROP_HEATSINK )
                layers = pcbnew.LSET.AllCuMask()
                layers.AddLayer(pcbnew.B_Mask)
                layers.AddLayer(pcbnew.F_Mask)
                via.SetLayerSet(layers)
                # thermal pads are placed between aperture pads.
                # so the number of thermal pads is aper_pad_ny-1 and aper_pad_nx-1 because
                #there are aper_pad_nx and aper_pad_nx apertures
                via_array = PA.EPADGridArray( via, aper_pad_ny-1, aper_pad_nx-1, aper_pad_l, aper_pad_w,
                    pcbnew.VECTOR2I( 0, 0 ) )
                via_array.SetFirstPadInArray(epad_num)
                via_array.AddPadsToModule(self.draw)

        # Draw the package outline on the F.Fab layer
        bevel = min( pcbnew.FromMM(1.0), self.package['width']/2, self.package['height']/2 )

        self.draw.SetLayer(pcbnew.F_Fab)

        w = self.package['width']
        h = self.package['height']

        self.draw.BoxWithDiagonalAtCorner(0, 0, w, h, bevel)

        # Silkscreen
        self.draw.SetLayer( pcbnew.F_SilkS )

        offset = self.draw.GetLineThickness()
        h_clip = h_row_len / 2 + self.pads['pitch']
        v_clip = v_row_len / 2 + self.pads['pitch']

        self.draw.SetLineThickness( pcbnew.FromMM( 0.12 ) ) #Default per KLC F5.1 as of 12/2018

        if h_pads_per_row > 0:

            self.draw.Polyline( [ [ h_clip, -h/2-offset], [ w/2+offset,-h/2-offset], [ w/2+offset, -v_clip] ] ) # top right
            self.draw.Polyline( [ [ h_clip,  h/2+offset], [ w/2+offset, h/2+offset], [ w/2+offset,  v_clip] ] ) # bottom right
            self.draw.Polyline( [ [-h_clip,  h/2+offset], [-w/2-offset, h/2+offset], [-w/2-offset,  v_clip] ] ) # bottom left

            # Add pin-1 indication as per IPC-7351C
            self.draw.Line( -h_clip, -h/2-offset, -w/2-pad_length/2, -h/2-offset )

        else:

            self.draw.Polyline( [ [-w/2-offset,  v_clip], [-w/2-offset, h/2+offset], [ w/2+offset, h/2+offset],
                [ w/2+offset,  v_clip] ] ) # bottom

            # Add pin-1 indication as per IPC-7351C
            self.draw.Polyline( [ [-h/2-offset, -h/2-offset], [ w/2+offset,-h/2-offset], [ w/2+offset, -v_clip] ] )

        self.draw.SetLineThickness( offset ) #Restore default

        # Courtyard
        cmargin = self.package["margin"]
        self.draw.SetLayer(pcbnew.F_CrtYd)

        sizex = (lim_x + cmargin) * 2 + pad_length
        sizey = (lim_y + cmargin) * 2 + pad_length

        # round size to nearest 0.1mm, rectangle will thus land on a 0.05mm grid
        sizex = pcbnew.PutOnGridMM(sizex, 0.1)
        sizey = pcbnew.PutOnGridMM(sizey, 0.1)
        # set courtyard line thickness to the one defined in KLC
        thick = self.draw.GetLineThickness()
        self.draw.SetLineThickness(pcbnew.FromMM(0.05))
        self.draw.Box(0, 0, sizex, sizey)
        # restore line thickness to previous value
        self.draw.SetLineThickness(pcbnew.FromMM(thick))

        #reference and value
        text_size = self.GetTextSize()  # IPC nominal
        text_offset = sizey / 2 + text_size

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

QFNWizard().register()


