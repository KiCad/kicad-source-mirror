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

class PadMaker:
    """
    Useful construction functions for common types of pads
    """

    def __init__(self, module):
        self.module = module

    def THPad(self, w, l, drill, shape=pcbnew.PAD_SHAPE_OVAL):
        pad = pcbnew.D_PAD(self.module)

        pad.SetSize(pcbnew.wxSize(l, w))

        pad.SetShape(shape)

        pad.SetAttribute(pcbnew.PAD_ATTRIB_STANDARD)
        pad.SetLayerSet(pad.StandardMask())
        pad.SetDrillSize(pcbnew.wxSize(drill, drill))

        return pad

    def THRoundPad(self, size, drill):
        pad = self.THPad(size, size, drill, shape=pcbnew.PAD_SHAPE_CIRCLE)
        return pad

    def NPTHRoundPad(self, drill):
        pad = pcbnew.D_PAD(self.module)

        pad.SetSize(pcbnew.wxSize(drill, drill))

        pad.SetShape(pcbnew.PAD_SHAPE_CIRCLE)

        pad.SetAttribute(pcbnew.PAD_ATTRIB_HOLE_NOT_PLATED)
        pad.SetLayerSet(pad.UnplatedHoleMask())
        pad.SetDrillSize(pcbnew.wxSize(drill, drill))
        return pad

    def SMDPad(self, w, l, shape=pcbnew.PAD_SHAPE_RECT):
        pad = pcbnew.D_PAD(self.module)
        pad.SetSize(pcbnew.wxSize(l, w))

        pad.SetShape(shape)

        pad.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
        pad.SetLayerSet(pad.SMDMask())

        return pad

    def SMTRoundPad(self, size):
        pad = self.SMDPad(size, size, shape=pcbnew.PAD_SHAPE_CIRCLE)
        return pad


class PadArray:

    def __init__(self):
        self.firstPadNum = 1
        self.pinNames = None
        self.firstPad = None

    def SetPinNames(self, pinNames):
        """
        Set a name for all the pins
        """
        self.pinNames = pinNames

    def SetFirstPadType(self, firstPad):
        self.firstPad = firstPad

    def SetFirstPadInArray(self, fpNum):
        self.firstPadNum = fpNum

    def AddPad(self, pad):
        self.pad.GetParent().Add(pad)

    def GetPad(self, is_first_pad, pos):

        if (self.firstPad and is_first_pad):
            pad = self.firstPad
        else:
            pad = self.pad

        # create a new pad with same characteristics
        pad = pad.Duplicate()

        pad.SetPos0(pos)
        pad.SetPosition(pos)

        return pad

    def GetName(self, *args, **kwargs):

        if self.pinNames is None:
            return self.NamingFunction(*args, **kwargs)

        return self.pinNames

    def NamingFunction(self, *args, **kwargs):
        """
        Implement this as needed for each array type
        """
        raise NotImplementedError;


class PadGridArray(PadArray):

    def __init__(self, pad, nx, ny, px, py, centre=pcbnew.wxPoint(0, 0)):
        PadArray.__init__(self)
        # this pad is more of a "context", we will use it as a source of
        # pad data, but not actually add it
        self.pad = pad
        self.nx = int(nx)
        self.ny = int(ny)
        self.px = px
        self.py = py
        self.centre = centre

    # handy utility function 1 - A, 2 - B, 26 - AA, etc
    # aIndex = 0 for 0 - A
    # alphabet = set of allowable chars if not A-Z,
    #            eg ABCDEFGHJKLMNPRTUVWY for BGA
    def AlphaNameFromNumber(self, n, aIndex=1,
                            alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZ"):

        div, mod = divmod(n - aIndex, len(alphabet))
        alpha = alphabet[mod]

        if div > 0:
            return self.AlphaNameFromNumber(div, aIndex, alphabet) + alpha

        return alpha

    # right to left, top to bottom
    def NamingFunction(self, x, y):
        return self.firstPadNum + (self.nx * y + x)

    #relocate the pad and add it as many times as we need
    def AddPadsToModule(self, dc):

        pin1posX = self.centre.x - self.px * (self.nx - 1) / 2
        pin1posY = self.centre.y - self.py * (self.ny - 1) / 2

        for x in range(0, self.nx):

            posX = pin1posX + (x * self.px)

            for y in range(self.ny):
                posY = pin1posY + (self.py * y)

                pos = dc.TransformPoint(posX, posY)

                pad = self.GetPad(x == 0 and y == 0, pos)

                pad.SetPadName(self.GetName(x,y))

                self.AddPad(pad)


class PadLineArray(PadGridArray):

    def __init__(self, pad, n, pitch, isVertical,
                 centre=pcbnew.wxPoint(0, 0)):

        if isVertical:
            PadGridArray.__init__(self, pad, 1, n, 0, pitch, centre)
        else:
            PadGridArray.__init__(self, pad, n, 1, pitch, 0, centre)

class PadCircleArray(PadArray):

    def __init__(self, pad, n, r, angle_offset=0, centre=pcbnew.wxPoint(0, 0),
                 clockwise=True):
        PadArray.__init__(self)
        # this pad is more of a "context", we will use it as a source of
        # pad data, but not actually add it
        self.pad = pad
        self.n = int(n)
        self.r = r
        self.angle_offset = angle_offset
        self.centre = centre
        self.clockwise = clockwise

    # around the circle, CW or CCW according to the flag
    def NamingFunction(self, n):
        return str(self.firstPadNum + n)

    #relocate the pad and add it as many times as we need
    def AddPadsToModule(self, dc):

        for pin in range(0, self.n):

            angle = self.angle_offset + (360 / self.n) * pin

            if not self.clockwise:
                angle = -angle

            pos_x = math.sin(angle * math.pi / 180) * self.r
            pos_y = -math.cos(angle  * math.pi / 180) * self.r

            pos = dc.TransformPoint(pos_x, pos_y)

            pad = self.GetPad(pin == 0, pos)

            pad.SetPadName(self.GetName(pin))

            self.AddPad(pad)

class PadCustomArray(PadArray):
    """
    Layout pads according to a custom array of [x,y] data
    """

    def __init__(self, pad, array):
        PadArray.__init__(self)
        self.pad = pad

        self.array = array

    def NamingFunction(self, n):
        return str(self.firstPadNum + n)

    #relocate the pad and add it as many times as we need
    def AddPadsToModule(self, dc):

        for i in range(len(self.array)):

            pos = dc.TransformPoint(self.array[i][0], self.array[i][1])

            pad = self.GetPad(i == 0, pos)

            pad.SetPadName(self.GetName(i))

            self.AddPad(pad)
