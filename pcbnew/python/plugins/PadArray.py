#  PadArray.py
#
#  Copyright 2014 john <john@johndev>
#
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
#

from __future__ import division

import math
import pcbnew

class PadMaker(object):
    """!
    Useful construction functions for common types of pads, providing
    sensible defaults for common pads.
    """

    def __init__(self, module):
        """!
        @param module: the module the pads will be part of
        """
        self.module = module

    def THPad(self, Vsize, Hsize, drill, shape=pcbnew.PAD_SHAPE_OVAL,
              rot_degree = 0):
        """!
        A basic through-hole pad of the given size and shape
        @param Vsize: the vertical size of the pad
        @param Hsize: the horizontal size of the pad
        @param drill: the drill diameter
        @param shape: the shape of the pad
        @param rot_degree: the pad rotation, in degrees
        """
        pad = pcbnew.PAD(self.module)
        pad.SetSize(pcbnew.VECTOR2I( int(Hsize), int(Vsize) ))
        pad.SetShape(shape)
        pad.SetAttribute(pcbnew.PAD_ATTRIB_PTH)
        pad.SetLayerSet(pad.PTHMask())
        pad.SetDrillSize(pcbnew.VECTOR2I( int(drill), int(drill) ))
        pad.SetOrientation( pcbnew.EDA_ANGLE( rot_degree, pcbnew.DEGREES_T ) )

        return pad

    def THRoundPad(self, size, drill):
        """!
        A round though-hole pad. A shortcut for THPad()
        @param size: pad diameter
        @param drill: drill diameter
        """
        pad = self.THPad(size, size, drill, shape=pcbnew.PAD_SHAPE_CIRCLE)
        return pad

    def NPTHRoundPad(self, drill):
        """!
        A round non-plated though hole (NPTH)

        @param drill: the drill diameter (equals the NPTH diameter)
        """
        pad = pcbnew.PAD(self.module)
        pad.SetSize(pcbnew.VECTOR2I( int(drill), int(drill) ))
        pad.SetShape(pcbnew.PAD_SHAPE_CIRCLE)
        pad.SetAttribute(pcbnew.PAD_ATTRIB_NPTH)
        pad.SetLayerSet(pad.UnplatedHoleMask())
        pad.SetDrillSize(pcbnew.VECTOR2I( int(drill), int(drill) ))
        return pad

    def SMDPad(self, Vsize, Hsize, shape=pcbnew.PAD_SHAPE_RECT, rot_degree=0):
        """
        Create a surface-mount pad of the given size and shape
        @param Vsize: the vertical size of the pad
        @param Hsize: the horizontal size of the pad
        @param shape: the shape of the pad
        @param rot_degree: the pad rotation, in degrees
        """
        pad = pcbnew.PAD(self.module)
        pad.SetSize(pcbnew.VECTOR2I( int(Hsize), int(Vsize) ) )
        pad.SetShape(shape)
        pad.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
        pad.SetLayerSet(pad.SMDMask())
        pad.SetOrientation( pcbnew.EDA_ANGLE( rot_degree, pcbnew.DEGREES_T ) )

        return pad

    def AperturePad(self, Vsize, Hsize, shape=pcbnew.PAD_SHAPE_RECT, rot_degree=0):
        """
        Create a aperture pad of the given size and shape, i.e. a smd pad shape
        on the solder paste and not on a copper
        layer
        @param Vsize: the vertical size of the aperture
        @param Hsize: the horizontal size of the aperture
        @param shape: the shape of the pad
        @param rot_degree: the pad rotation, in degrees
        """
        pad = pcbnew.PAD(self.module)
        pad.SetSize(pcbnew.VECTOR2I( int(Hsize), int(Vsize) ) )
        pad.SetShape(shape)
        pad.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
        pad.SetLayerSet(pad.ApertureMask())
        pad.SetOrientation( pcbnew.EDA_ANGLE( rot_degree, pcbnew.DEGREES_T ) )

        return pad

    def SMTRoundPad(self, size):
        """!
        A round surface-mount pad. A shortcut for SMDPad()
        @param size: pad diameter
        """
        pad = self.SMDPad(size, size, shape=pcbnew.PAD_SHAPE_CIRCLE)
        return pad


class PadArray(object):
    """!
    A class to assist in creating repetitive grids of pads

    Generally, PadArrays have an internal prototypical pad, and copy this
    for each pad in the array. They can also have a special pad for the
    first pad, and a custom function to name the pad.

    Generally, PadArray is used as a base class for more specific array
    types.
    """

    def __init__(self, pad):
        """!
        @param pad: the prototypical pad
        """
        self.firstPadNum = 1
        self.pinNames = None

        # this pad is more of a "context", we will use it as a source of
        # pad data, but not actually add it
        self.pad = pad
        self.firstPad = None

    def SetPinNames(self, pinNames):
        """!
        Set a name for all the pins. If given, this overrides the
        naming function.

        @param pinNames: the name to use for all pins
        """
        self.pinNames = pinNames

    def SetFirstPadType(self, firstPad):
        """!
        If the array has a different first pad, this is the pad that
        is used
        @param firstPad: the prototypical first pad
        """
        self.firstPad = firstPad

    def SetFirstPadInArray(self, fpNum):
        """!
        Set the numbering for the first pad in the array
        @param fpNum: the number for the first pad
        """
        self.firstPadNum = fpNum

    def AddPad(self, pad):
        """!
        Add a pad to the array, under the same footprint as the main
        prototype pad
        @param pad: pad to add
        """
        self.pad.GetParent().Add(pad)

    def GetPad(self, is_first_pad, pos):
        """!
        Get a pad in the array with the given position
        @param is_first_pad: use the special first pad if there is one
        @param pos: the pad position
        """
        if (self.firstPad and is_first_pad):
            pad = self.firstPad
        else:
            pad = self.pad

        # create a new pad with same characteristics
        pad = pad.Duplicate()
        pad.SetPosition(pos)

        return pad

    def GetName(self, *args, **kwargs):
        """!
        Get the pad name from the naming function, or the pre-set
        pinNames parameter (set with SetPinNames)
        """

        if self.pinNames is None:
            return self.NamingFunction(*args, **kwargs)

        return self.pinNames

    def NamingFunction(self, *args, **kwargs):
        """!
        Implement this as needed for each array type
        """
        raise NotImplementedError;


class PadGridArray(PadArray):
    """!
    A basic grid of pads
    """

    def __init__(self, pad, nx, ny, px, py, centre=pcbnew.VECTOR2I(0, 0)):
        """!
        @param pad: the prototypical pad of the array
        @param nx: number of pads in x-direction
        @param ny: number of pads in y-direction
        @param px: pitch in x-direction
        @param py: pitch in y-direction
        @param centre: array centre point
        """

        try:
            super().__init__(pad)
        except TypeError:
            super(PadGridArray, self).__init__(pad)

        self.nx = int(nx)
        self.ny = int(ny)
        self.px = px
        self.py = py
        self.centre = centre

    def AlphaNameFromNumber(self, n, aIndex=1,
                            alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
        """!
        Utility function to generate an alphabetical name:

        eg. 1 - A, 2 - B, 26 - AA, etc

        @param aIndex: index of 'A': 0 for 0 - A
        @param n: the pad index
        @param alphabet: set of allowable chars if not A-Z,
            e.g. ABCDEFGHJKLMNPRTUVWY for BGA
        """

        div, mod = divmod(n - aIndex, len(alphabet))
        alpha = alphabet[mod]

        if div > 0:
            return self.AlphaNameFromNumber(div, aIndex, alphabet) + alpha

        return alpha

    def NamingFunction(self, x, y):
        """!
        Implementation of the naming function: right to left, top-to-bottom

        @param x: the pad x index
        @param y: the pad y index
        """
        return self.firstPadNum + (self.nx * y + x)

    #relocate the pad and add it as many times as we need
    def AddPadsToModule(self, dc):
        """!
        Create the pads and add them to the module in the correct positions

        @param dc: the drawing context
        """

        pin1posX = self.centre.x - self.px * (self.nx - 1) / 2
        pin1posY = self.centre.y - self.py * (self.ny - 1) / 2

        for x in range(0, self.nx):
            posX = pin1posX + (x * self.px)

            for y in range(self.ny):
                posY = pin1posY + (self.py * y)
                pos = dc.TransformPoint(posX, posY)
                pad = self.GetPad(x == 0 and y == 0, pos)
                pad.SetName(self.GetName(x,y))
                self.AddPad(pad)


class EPADGridArray(PadGridArray):
    """!
    A pad grid array with a fixed name, used for things like thermal
    pads and via grids.
    """

    def NamingFunction(self, nx, ny):
        """!
        Simply return the firstPadNum
        @param nx: not used
        @param ny: not used
        """
        return self.firstPadNum


class PadZGridArray(PadArray):
    """!
    A staggered pin array
    """

    def __init__(self, pad, pad_count, line_count, line_pitch,
                 pad_pitch, centre=pcbnew.VECTOR2I(0, 0)):
        """!
        @param pad: the prototypical pad
        @param pad_count: total pad count
        @param line_count: number of staggered lines
        @param line_pitch: distance between lines
        @param pad_pitch: distance between pads in a line
        @param centre: array centre point
        """
        super(PadZGridArray, self).__init__(pad)

        self.pad_count = int(pad_count)
        self.line_count = int(line_count)
        self.line_pitch = line_pitch
        self.pad_pitch = pad_pitch
        self.centre = centre

    def NamingFunction(self, pad_pos):
        """!
        Naming just increased with pad index in array
        """
        return self.firstPadNum + pad_pos

    def AddPadsToModule(self, dc):
        """!
        Create the pads and add them to the module in the correct positions

        @param dc: the drawing context
        """

        pin1posX = self.centre.x - self.pad_pitch * (self.pad_count - 1) / 2
        pin1posY = self.centre.y + self.line_pitch * (self.line_count - 1) / 2
        line = 0

        for padnum in range(0, self.pad_count):
            posX = pin1posX + (padnum * self.pad_pitch)
            posY = pin1posY - (self.line_pitch * line)

            pos = dc.TransformPoint(posX, posY)
            pad = self.GetPad(padnum == 0, pos)
            pad.SetName(self.GetName(padnum))
            self.AddPad(pad)

            line += 1

            if line >= self.line_count:
                line = 0


class PadLineArray(PadGridArray):
    """!
    Shortcut cases for a single-row grid array. Can be used for
    constructing sections of larger footprints.
    """

    def __init__(self, pad, n, pitch, isVertical,
                 centre=pcbnew.VECTOR2I(0, 0)):
        """!
        @param pad: the prototypical pad
        @param n: number of pads in array
        @param pitch: distance between pad centres
        @param isVertical: horizontal or vertical array (can also use the
        drawing contexts transforms for more control)
        @param centre: array centre
        """

        if isVertical:
            super(PadLineArray, self).__init__(pad, 1, n, 0, pitch, centre)
        else:
            super(PadLineArray, self).__init__(pad, n, 1, pitch, 0, centre)


class PadCircleArray(PadArray):
    """!
    Circular pad array
    """

    def __init__(self, pad, n, r, angle_offset=0, centre=pcbnew.VECTOR2I(0, 0),
                 clockwise=True, padRotationEnable=False, padRotationOffset=0):
        """!
        @param pad: the prototypical pad
        @param n: number of pads in array
        @param r: the circle radius
        @param angle_offset: angle of the first pad
        @param centre: array centre point
        @param clockwise: array increases in a clockwise direction
        @param padRotationEnable: also rotate pads when placing
        @param padRotationOffset: rotation of first pad
        """

        super(PadCircleArray, self).__init__(pad)

        self.n = int(n)
        self.r = r
        self.angle_offset = angle_offset
        self.centre = centre
        self.clockwise = clockwise
        self.padRotationEnable = padRotationEnable
        self.padRotationOffset = padRotationOffset

    def NamingFunction(self, n):
        """!
        Naming around the circle, CW or CCW according to the clockwise flag
        """
        return str(self.firstPadNum + n)

    def AddPadsToModule(self, dc):
        """!
        Create the pads and add them to the module in the correct positions

        @param dc: the drawing context
        """

        for pin in range(0, self.n):
            angle = self.angle_offset + (360 / self.n) * pin

            if not self.clockwise:
                angle = -angle

            pos_x = math.sin(angle * math.pi / 180) * self.r
            pos_y = -math.cos(angle  * math.pi / 180) * self.r
            pos = dc.TransformPoint(pos_x, pos_y)
            pad = self.GetPad(pin == 0, pos)
            padAngle = self.padRotationOffset
            if self.padRotationEnable:
                padAngle -=angle
            pad.SetOrientation( pcbnew.EDA_ANGLE( padAngle, pcbnew.DEGREES_T ) )
            pad.SetName(self.GetName(pin))
            self.AddPad(pad)


class PadCustomArray(PadArray):
    """!
    Layout pads according to a custom array of [x,y] data
    """

    def __init__(self, pad, array):
        """!
        @param pad: the prototypical pad
        @param array: the position data array
        """
        super(PadCustomArray, self).__init__(pad)

        self.array = array

    def NamingFunction(self, n):
        """!
        Simple increment along the given array
        @param n: the pad index in the array
        """
        return str(self.firstPadNum + n)

    def AddPadsToModule(self, dc):
        """!
        Create the pads and add them to the module in the correct positions

        @param dc: the drawing context
        """

        for i in range(len(self.array)):
            pos = dc.TransformPoint(self.array[i][0], self.array[i][1])
            pad = self.GetPad(i == 0, pos)
            pad.SetName(self.GetName(i))
            self.AddPad(pad)
