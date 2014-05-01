
import pcbnew

class PadMaker:
    """
    Useful construction functions for common types of pads
    """

    def __init__(self, module):
        self.module = module

    def THPad(self, w, l, drill, shape = pcbnew.PAD_OVAL):
        pad = pcbnew.D_PAD(self.module)

        pad.SetSize(pcbnew.wxSize(l, w))

        pad.SetShape(shape)

        pad.SetAttribute(pcbnew.PAD_STANDARD)
        pad.SetLayerMask(pcbnew.PAD_STANDARD_DEFAULT_LAYERS)
        pad.SetDrillSize(pcbnew.wxSize(drill, drill))

        return pad

    def SMDPad(self, w, l, shape = pcbnew.PAD_RECT):
        pad = pcbnew.D_PAD(self.module)
        pad.SetSize(pcbnew.wxSize(l, w))

        pad.SetShape(shape)

        pad.SetAttribute(pcbnew.PAD_SMD)
        pad.SetLayerMask(pcbnew.PAD_SMD_DEFAULT_LAYERS)

        return pad

    def SMTRoundPad(self, size):
        pad = self.SMDPad(size, size, shape = pcbnew.PAD_CIRCLE)
        return pad

class PadArray:

    def __init__(self):
        self.firstPad = 1;

    def SetFirstPadInArray(self, fpNum):
        self.firstPad = fpNum

    # HACK! pad should one day have its own clone method
    def ClonePad(self):

        pad = pcbnew.D_PAD(self.pad.GetParent())

        pad.SetSize(self.pad.GetSize())
        pad.SetShape(self.pad.GetShape())
        pad.SetAttribute(self.pad.GetAttribute())
        pad.SetLayerMask(self.pad.GetLayerMask())
        pad.SetDrillSize(self.pad.GetDrillSize())

        return pad

    def AddPad(self, pad):
        self.pad.GetParent().Add(pad)

class PadGridArray(PadArray):

    def __init__(self, pad, nx, ny, px, py, pin1Pos):
        # this pad is more of a "context", we will use it as a source of
        # pad data, but not actually add it
        self.pad = pad
        self.nx = int(nx)
        self.ny = int(ny)
        self.px = px
        self.py = py
        self.pin1Pos = pin1Pos

    # handy utility function 1 - A, 2 - B, 26 - AA, etc
    # aIndex = 0 for 0 - A
    def AlphaNameFromNumber(self, n, aIndex = 1):

        div, mod = divmod(n - aIndex, 26)
        alpha = chr(65 + mod)

        if div > 0:
            return self.AlphaNameFromNumber(div) + alpha;

        return alpha;

    # right to left, top to bottom
    def NamingFunction(self, x, y):
        return self.firstPad + (self.nx * y + x)

    #relocate the pad and add it as many times as we need
    def AddPadsToModule(self):

        for x in range(0, self.nx):
            for y in range(self.ny):
                posX = self.pin1Pos.x + (self.px * x)
                posY = self.pin1Pos.y + (self.py * y)

                pos = pcbnew.wxPoint(posX, posY)

                # THIS DOESN'T WORK yet!
                #pad = self.pad.Clone()
                pad = self.ClonePad()

                pad.SetPos0(pos)
                pad.SetPosition(pos)

                pad.SetPadName(str(self.NamingFunction(x,y)))

                self.AddPad(pad)

class PadLineArray(PadGridArray):

    def __init__(self, pad, n, pitch, isVertical, pin1Pos):

        if isVertical:
            PadGridArray.__init__(self, pad, 1, n, 0, pitch, pin1Pos)
        else:
            PadGridArray.__init__(self, pad, n, 1, pitch, 0, pin1Pos)

class RectPadArray(PadArray):

    def __init__(self, nx, ny, pitch, xpitch, ypitch, pin1Pos):

        #left row
        pin1Pos = pcbnew.wxPoint(-h_pitch / 2, -row_len / 2)
        array = PadLineArray(h_pad, pads_per_row, pad_pitch, True, pin1Pos)
        array.SetFirstPadInArray(1)
        array.AddPadsToModule()

        #bottom row
        pin1Pos = pcbnew.wxPoint(-row_len / 2, v_pitch / 2)
        array = PA.PadLineArray(v_pad, pads_per_row, pad_pitch, False, pin1Pos)
        array.SetFirstPadInArray(pads_per_row + 1)
        array.AddPadsToModule()

        #right row
        pin1Pos = pcbnew.wxPoint(h_pitch / 2, row_len / 2)
        array = PadLineArray(h_pad, pads_per_row, -pad_pitch, True, pin1Pos)
        array.SetFirstPadInArray(2*pads_per_row + 1)
        array.AddPadsToModule()

        #top row
        pin1Pos = pcbnew.wxPoint(row_len / 2, -v_pitch / 2)
        array = PadLineArray(v_pad, pads_per_row, -pad_pitch, False, pin1Pos)
        array.SetFirstPadInArray(3*pads_per_row + 1)
        array.AddPadsToModule()
