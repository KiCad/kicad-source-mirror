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

# A basic fp wizard to test arcs in footprints
import pcbnew
import FootprintWizardBase

class DemoArc(FootprintWizardBase.FootprintWizard):
    """Test for arc"""
    GetName = lambda self: "DemoArc"
    GetDescription = lambda self: DemoArc.__doc__
    GetReferencePrefix = lambda self: "demo"
    GetValue = lambda self: "Arc"

    def GenerateParameterList(self):
        self.AddParam("demo", "radius", self.uMM, 8, min_value=0.1)
        self.AddParam("demo", "centerX", self.uMM, 0)
        self.AddParam("demo", "centerY", self.uMM, 0)
        self.AddParam("demo", "angle", self.uDegrees, 90)

    def BuildThisFootprint(self):
        radius = self.parameters["demo"]["radius"]
        arc_angle_deg = self.parameters["demo"]["angle"]
        centerx = self.parameters["demo"]["centerX"]
        centery = self.parameters["demo"]["centerY"]
        startptx = centerx
        startpty = centery+radius
        self.draw.Arc(centerx, centery, startptx, startpty, pcbnew.EDA_ANGLE( arc_angle_deg, pcbnew.DEGREES_T ) )
        t_size = self.GetTextSize()
        self.draw.Reference(0, -t_size, t_size)
        self.draw.Value(0, t_size, t_size)

    def CheckParameters(self):
        pass

DemoArc().register()
