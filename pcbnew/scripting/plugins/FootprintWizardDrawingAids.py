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

import pcbnew

class FootprintWizardDrawingAids:
    """
    Collection of handy functions to simplify drawing shapes from within
    footprint wizards

    A "drawing context" is provided which can be used to set and retain
    settings such as line width and layer
    """
    def __init__(self, module):
        self.module = module
        #drawing context defaults
        self.dc = {
            'layer': pcbnew.SILKSCREEN_N_FRONT,
            'width': pcbnew.FromMM(0.2)
        }

    def SetWidth(self, width):
        self.dc['width'] = width

    def SetLayer(self, layer):
        self.dc['layer'] = layer

    def Line(self, x1, y1, x2, y2):

        outline = pcbnew.EDGE_MODULE(self.module)
        outline.SetWidth(self.dc['width'])
        outline.SetLayer(self.dc['layer'])
        outline.SetShape(pcbnew.S_SEGMENT)
        start = pcbnew.wxPoint(x1, y1)
        end = pcbnew.wxPoint(x2, y2)
        outline.SetStartEnd(start, end)
        self.module.Add(outline)

    # extends from (x1,y1) right
    def HLine(self, x, y, l):
        """
        Draw a horizontal line from (x,y), rightwards
        """
        self.Line(x, y, x + l, y)

    def VLine(self, x, y, l):
        """
        Draw a vertical line from (x1,y1), downwards
        """
        self.Line(x, y, x, y + l)

    def Polyline(self, pts):

        if len(pts) < 2:
            return

        for i in range(0, len(pts) - 1):
            self.Line(pts[i][0], pts[i][1], pts[i+1][0], pts[i+1][1])

    def Reference(self, x, y, size):
        """
        Draw the module's reference as the given point.

        The actual setting of the reference is not done in this drawing
        aid - that is up to the wizard
        """

        text_size = pcbnew.wxSize(size, size)

        self.module.Reference().SetPos0(pcbnew.wxPoint(x, y))
        self.module.Reference().SetTextPosition(self.module.Reference().GetPos0())
        self.module.Reference().SetSize(text_size)

    def Value(self, x, y, size):
        """
        As for references, draw the module's value
        """
        text_size = pcbnew.wxSize(size, size)

        self.module.Value().SetPos0(pcbnew.wxPoint(x, y))
        self.module.Value().SetTextPosition(self.module.Value().GetPos0())
        self.module.Value().SetSize(text_size)

    def Box(self, x, y, w, h):
        """
        Draw a rectangular box, centred at (x,y), with given width and
        height
        """
        self.VLine(x - w/2, y - h/2, h) # left
        self.VLine(x + w/2, y - h/2, h) # right
        self.HLine(x - w/2, y + h/2, w) # bottom
        self.HLine(x - w/2, y - h/2, w) # top

    def NotchedBox(self, x, y, w, h, notchW, notchH):
        """
        Draw a box with a notch in the top edge
        """
        #limit to half the overall width
        notchW = min(x + w/2, notchW)

        # draw notch
        self.Polyline([ #three sides of box
                        (x - w/2, y - h/2),
                        (x - w/2, y + h/2),
                        (x + w/2, y + h/2),
                        (x + w/2, y - h/2),
                        #the notch
                        (notchW/2, y - h/2),
                        (notchW/2, y - h/2 + notchH),
                        (-notchW/2, y - h/2 + notchH),
                        (-notchW/2, y - h/2),
                        (x - w/2, y - h/2)
                    ])

    def BoxWithDiagonalAtCorner(self, x, y, w, h, diagSetback):

        self.Box(x, y, w, h)

        #diagonal corner
        self.Line(x - w/2 + diagSetback, x - h/2, x - w/2,
                x - h/2 + diagSetback)
