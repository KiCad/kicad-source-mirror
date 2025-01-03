#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2024 Jarrett Rainier
# Copyright (C) 2012-2024 KiCad Developers
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
# Based upon the work in:
# Microchip AN2934 Self-Capacitance Sensors
#

import pcbnew
import FootprintWizardBase
import pcbnew
import math, cmath

class ScrollWheelWizard(FootprintWizardBase.FootprintWizard):

    def GetName(self):
        """
        Return footprint name.
        This is specific to each footprint class, you need to implement this
        """
        return 'ScrollWheel'

    def GetDescription(self):
        """
        Return footprint description.
        This is specific to each footprint class, you need to implement this
        """
        return 'Capacitive ScrollWheel wizard'

    def GetValue(self):
        return "ScrollWheel-{od:g}x{id:g}mm".format(
            od = pcbnew.ToMM(self.pads['outer_diameter']),
            id = pcbnew.ToMM(self.pads['inner_diameter'])
            )

    def GenerateParameterList(self):
        self.AddParam("Pads", "steps", self.uInteger, 3, min_value=2)
        self.AddParam("Pads", "bands", self.uInteger, 4, min_value=2)
        self.AddParam("Pads", "outer_diameter", self.uMM, 40)
        self.AddParam("Pads", "inner_diameter", self.uMM, 12)
        self.AddParam("Pads", "deadzone", self.uMM, 4)
        self.AddParam("Pads", "corner_radius", self.uMM, 1)
        self.AddParam("Pads", "clearance", self.uMM, 0.5, min_value=0.5, max_value=1.5)
        self.AddParam("Pads", "edge_silkscreen", self.uBool, True)
        self.AddParam("Pads", "full_silkscreen", self.uBool, True)
    @property
    def pads(self):
        return self.parameters['Pads']
        
    def smdCircle(self,radius,pos):
        arc_angle_deg = 360
        centerx = pos[0]
        centery = pos[1]
        startptx = centerx
        startpty = centery+radius
        self.draw.Arc(centerx, centery, startptx, startpty, pcbnew.EDA_ANGLE( arc_angle_deg, pcbnew.DEGREES_T ) )
        
    def smdArc(self,start,end,pos):
        pos = complex(pos[0], pos[1])
        centerx = pos.real
        centery = pos.imag
        start_angle_rad = cmath.phase(start - pos)
        end_angle_rad = cmath.phase(end - pos)
        start_angle_deg = math.degrees(start_angle_rad)
        end_angle_deg = math.degrees(end_angle_rad)
        arc_angle_deg = (end_angle_deg - start_angle_deg) % 360
        self.draw.Arc(centerx, centery, start.real, start.imag, pcbnew.EDA_ANGLE( arc_angle_deg, pcbnew.DEGREES_T ) )
       
    # Generate points along the arc and create a polygon from those points
    def arc_points(self, start, end, pos, cw=True):
        pos = complex(pos[0], pos[1])
        centerx = pos.real
        centery = pos.imag
        if cw == False:
            start, end = end, start
        start_angle_rad = cmath.phase(start - pos)
        end_angle_rad = cmath.phase(end - pos)
        arc_angle_rad = (end_angle_rad - start_angle_rad) % (2 * math.pi)
        num_points = 20
        
        shape = pcbnew.SHAPE_LINE_CHAIN()
        for i in range(num_points - 1):
            if cw:
                angle = start_angle_rad + (i + 1) * arc_angle_rad / (num_points - 1)
            else:
                angle = start_angle_rad + (num_points - (i + 1)) * arc_angle_rad / (num_points - 1)
            x = centerx + abs(start - pos) * math.cos(angle)
            y = centery + abs(start - pos) * math.sin(angle)
            shape.Append(pcbnew.VECTOR2I(int(x), int(y)))

        return shape
        
    # Find the centre, given two points and a radius
    def find_arc_center(self, p1, p2, radius, cw=True):
        x1, y1 = p1.real, p1.imag
        x2, y2 = p2.real, p2.imag

        mx, my = (x1 + x2) / 2, (y1 + y2) / 2
        d = math.sqrt((x2 - x1)**2 + (y2 - y1)**2)

        if d > 2 * radius:
            raise ValueError("Radius {} is too small to pass through both points {}.".format(radius, d))

        h = math.sqrt(radius**2 - (d / 2)**2)
        dx, dy = (y2 - y1) / d, -(x2 - x1) / d

        if cw:
            cx, cy = mx - h * dx, my - h * dy
        else:
            cx, cy = mx + h * dx, my + h * dy

        return (cx, cy)
        
    # Find the centre, given two points that are at 180 degrees
    def find_avg_centre(self, p1, p2):
        pt1_r, pt1_phi = cmath.polar(p1)
        pt2_r, pt2_phi = cmath.polar(p2)
        
        avg_r = (pt1_r + pt2_r) / 2
        avg_phi = (pt1_phi + pt2_phi) / 2
        
        centre = cmath.rect(avg_r, avg_phi)

        return (centre.real, centre.imag)
        
    def find_avg_radius(self, p1, p2, cw = True):
        pt1_r, pt1_phi = cmath.polar(p1)
        pt2_r, pt2_phi = cmath.polar(p2)
        avg_r = (pt1_r + pt2_r) / 2

        return self.find_arc_center(p1, p2, avg_r, cw)        
        
    def create_pads(self, outer_diameter, inner_diameter, deadzone, corner_radius, steps, bands, clearance):
        outer_radius = outer_diameter / 2
        inner_radius = inner_diameter / 2

        # Angle step for each radial section (each deadzone gap)
        angle_step = 2 * math.pi / steps
        
        # Right side corners, determines tangential OD-ID range
        inner_corner_r = inner_radius + corner_radius
        corner_x = deadzone / 2 + corner_radius
        y_min = cmath.sqrt(inner_corner_r**2 - corner_x**2)
        outer_corner_r = outer_radius - corner_radius
        y_max = cmath.sqrt(outer_corner_r**2 - corner_x**2)
        
        y_range = y_max - y_min
        
        points_right = []
        points_left = []

        for step in range(steps):
            angle = step * angle_step
            points_right.append([])
            points_left.append([])
            for i in range(bands):
                # Positions for right side arc corners (on the top/bot edges of the band)
                corner_y = y_min + (i * y_range / (bands - 1))
                corner_r, corner_phi = cmath.polar(complex(corner_x, corner_y))
                corner = cmath.rect(corner_r, corner_phi + angle)
                right = corner.real, -corner.imag
                
                p1 = cmath.rect(corner_r + corner_radius, corner_phi + angle)
                p1 = p1.real - p1.imag * 1j
                p2 = cmath.rect(corner_r - corner_radius, corner_phi + angle)
                p2 = p2.real - p2.imag * 1j
                points_right[step].append([p1, p2, right])

                # Positions for left side arc corners (centered vertically within the band)
                if i < bands - 1:
                    band_min = y_min + (i * y_range / (bands - 1))
                    band_max = y_min + ((i + 1) * y_range / (bands - 1))
                    corner_y = band_min + (band_max - band_min) / 2
                    corner_r, corner_phi = cmath.polar(complex(-1 * corner_x, corner_y))
                    corner = cmath.rect(corner_r, corner_phi + angle)
                    left = corner.real, -corner.imag
                    
                    p1 = cmath.rect(corner_r - corner_radius, corner_phi + angle)
                    p1 = p1.real - p1.imag * 1j
                    p2 = cmath.rect(corner_r + corner_radius, corner_phi + angle)
                    p2 = p2.real - p2.imag * 1j
                    points_left[step].append([p1, p2, left])
                    

        # Format for first and second args, p1 and p2: 012
        # 0 = "step" radially around the circle
        # 1 = "band" tangentially through the circle
        # 2 = inside or outside point of the arc
        # Third arg is switch/case statement described below
        # Fourth arg cw (T) / ccw (F) for arc drawing
        ordered_pairs = []
        
        # ID edge
        ordered_pairs.append([points_right[1][0][1], points_right[0][0][1], 0, True])
        
        # Moving tangentially, from ID to OD, on right side
        for i in range(bands - 1):
            ordered_pairs.append([points_right[0][i][1], points_right[0][i][0], 1, True])
            ordered_pairs.append([points_right[0][i][0], points_left[steps - 1][i][0], 2, True])
            ordered_pairs.append([points_left[steps - 1][i][0], points_left[steps - 1][i][1], 1, False])
            ordered_pairs.append([points_left[steps - 1][i][1], points_right[0][i + 1][1], 2, False])
            
        # OD edge
        ordered_pairs.append([points_right[0][bands - 1][1], points_right[0][bands - 1][0], 1, True])
        ordered_pairs.append([points_right[0][bands - 1][0], points_right[1][bands - 1][0], 0, False])
        
        for i in range(bands - 1):
            a = bands - (i + 1)
            ordered_pairs.append([points_right[1][a][0], points_right[1][a][1], 1, False])
            ordered_pairs.append([points_right[1][a][1], points_left[0][a - 1][1], 2, True])
            ordered_pairs.append([points_left[0][a - 1][1], points_left[0][a - 1][0], 1, True])
            ordered_pairs.append([points_left[0][a - 1][0], points_right[1][a - 1][0], 2, False])
            ordered_pairs.append([points_right[1][a - 1][0], points_right[1][a - 1][1], 1, False])
  
        shape = pcbnew.SHAPE_LINE_CHAIN()
        for i in range(len(ordered_pairs)):
            step = ordered_pairs[i]
            centre = (step[0].real, step[0].imag)
            
             # Switch case:
             #    centre = 0,
             #    between the two points (ie. corners), or 
             #    perpendicular and average of the radius (ie. "spikes") 
            if ordered_pairs[i][2] == 0:
                centre = (0,0)
            elif ordered_pairs[i][2] == 1:
                centre = self.find_avg_centre(ordered_pairs[i][0], ordered_pairs[i][1])
            elif ordered_pairs[i][2] == 2:
                centre = self.find_avg_radius(ordered_pairs[i][0], ordered_pairs[i][1], ordered_pairs[i][3])
                
            shape.Append(self.arc_points(ordered_pairs[i][0], ordered_pairs[i][1], centre, ordered_pairs[i][3]))
            

        offset = int((outer_radius - inner_radius) / 2 + inner_radius)
        pad = pcbnew.PAD(self.module)
        pad.SetShape(pcbnew.PAD_SHAPE_CUSTOM)
        pad.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
        pad.SetSize(pcbnew.VECTOR2I(deadzone // 2, deadzone // 2))
        pad.SetPosition(pcbnew.VECTOR2I(0, -offset))
        fcuSet = pcbnew.LSET()
        fcuSet.AddLayer(pcbnew.F_Cu)
        pad.SetLayerSet(fcuSet)
        poly = pcbnew.SHAPE_POLY_SET(shape)
        poly.Deflate(int(clearance / 2), pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS, int(clearance / 10))
        poly.Move((pcbnew.VECTOR2I(0, offset)))
        pad.AddPrimitive(poly, 0)
        
        for i in range(steps):
            angle_step = (i * 2 * math.pi / steps)
            pos = cmath.rect(offset, angle_step + (math.pi / 2))
            step_pad = pad.ClonePad()
            step_pad.SetName(str(i + 1))
            step_pad.SetOrientation(pcbnew.EDA_ANGLE(angle_step, pcbnew.RADIANS_T ))
            step_pad.SetPosition(pcbnew.VECTOR2I(int(pos.real), int(-pos.imag)))
            self.module.Add(step_pad)
        
    def draw_silkscreen_arcs(self, outer_diameter, inner_diameter, deadzone, corner_radius, steps, bands):
        outer_radius = outer_diameter / 2
        inner_radius = inner_diameter / 2

        # Angle step for each radial section (each deadzone gap)
        angle_step = 2 * math.pi / steps
        
        # Right side
        inner_corner_r = inner_radius + corner_radius
        corner_x = deadzone / 2 + corner_radius
        y_min = cmath.sqrt(inner_corner_r**2 - corner_x**2)
        outer_corner_r = outer_radius - corner_radius
        y_max = cmath.sqrt(outer_corner_r**2 - corner_x**2)
        
        y_range = y_max - y_min
        
        outer_spikes = []
        inner_spikes = []
        right_corners = []
        right_corners_ccw = []
        left_corners = []

        for step in range(steps):
            angle = step * angle_step
            outer_spikes.append([])
            inner_spikes.append([])
            right_corners.append([])
            right_corners_ccw.append([])
            left_corners.append([])
            
            for i in range(bands):
                # Positions for right side arc corners (on the top/bot edges of the band)
                corner_y = y_min + (i * y_range / (bands - 1))
                corner_r, corner_phi = cmath.polar(complex(corner_x, corner_y))
                corner = cmath.rect(corner_r, corner_phi + angle)
                right = corner.real, -corner.imag
                
                outer_spike_corner_r = cmath.rect(corner_r + corner_radius, corner_phi + angle)
                outer_spike_corner_r = outer_spike_corner_r.real - outer_spike_corner_r.imag * 1j
                
                inner_spike_corner_r = cmath.rect(corner_r - corner_radius, corner_phi + angle)
                inner_spike_corner_r = inner_spike_corner_r.real - inner_spike_corner_r.imag * 1j
                
                self.smdArc(inner_spike_corner_r, outer_spike_corner_r,right)
                right_corners[step].append(self.arc_points(inner_spike_corner_r, outer_spike_corner_r,right))
                right_corners_ccw[step].append(self.arc_points(inner_spike_corner_r, outer_spike_corner_r,right, False))

                # Positions for left side arc corners (centered vertically within the band)
                if i < bands:
                    band_min = y_min + (i * y_range / (bands - 1))
                    band_max = y_min + ((i + 1) * y_range / (bands - 1))
                    corner_y = band_min + (band_max - band_min) / 2
                    corner_r, corner_phi = cmath.polar(complex(-1 * corner_x, corner_y))
                    corner = cmath.rect(corner_r, corner_phi + angle)
                    left = corner.real, -corner.imag
                    
                    outer_spike_corner_l = cmath.rect(corner_r - corner_radius, corner_phi + angle)
                    outer_spike_corner_l = outer_spike_corner_l.real - outer_spike_corner_l.imag * 1j
                    outer_spikes[step].append([outer_spike_corner_r, outer_spike_corner_l])
                # if i > 0:
                inner_spike_corner_l = cmath.rect(corner_r + corner_radius, corner_phi + angle)
                inner_spike_corner_l = inner_spike_corner_l.real - inner_spike_corner_l.imag * 1j
                inner_spikes[step].append([inner_spike_corner_r, inner_spike_corner_l])
                if (i + 1) < bands:
                    self.smdArc(inner_spike_corner_l, outer_spike_corner_l,left)
                    left_corners[step].append(self.arc_points(inner_spike_corner_l, outer_spike_corner_l,left, False))

        for step in range(steps):
            id1 = inner_spikes[step][0][0]
            id2 = inner_spikes[(step + 1) % steps][0][0]
            shape = self.arc_points(id2, id1,(0,0))
            for i in range(bands - 1):
                self.smdArc(id1, id2,(0,0))
                if step == 0:
                    shape.Append(right_corners[step][i])
                spike_corner_r = outer_spikes[step][i][0]
                spike_corner_l = outer_spikes[(step - 1) % steps][i][1]
                radius_r = abs(spike_corner_r)
                radius_l = abs(spike_corner_l)
                radius = (radius_r + radius_l) / 2
                spike_centre = self.find_arc_center(spike_corner_r, spike_corner_l, radius, cw=True)
                self.smdArc(spike_corner_r, spike_corner_l,spike_centre)
                if step == 0:
                    shape.Append(self.arc_points(spike_corner_r, spike_corner_l,spike_centre, True))
                    shape.Append(left_corners[(step - 1) % steps][i])
                spike_corner_r = inner_spikes[step][i + 1][0]
                spike_corner_l = inner_spikes[(step - 1) % steps][i][1]
                radius_r = abs(spike_corner_r)
                radius_l = abs(spike_corner_l)
                radius = (radius_r + radius_l) / 2
                spike_centre = self.find_arc_center(spike_corner_r, spike_corner_l, radius, cw=True)
                self.smdArc(spike_corner_r, spike_corner_l,spike_centre)
                if step == 0:
                    shape.Append(self.arc_points(spike_corner_r, spike_corner_l,spike_centre, False))
            shape.Append(right_corners[step][bands - 1])
            od2 = outer_spikes[step][bands - 1][0]
            od1 = outer_spikes[(step + 1) % steps][bands - 1][0]
            
            if step == 0:
                shape.Append(self.arc_points(od1, od2,(0,0), False))
            self.smdArc(od1, od2,(0,0))
                    
    def CheckParameters(self):
        od = pcbnew.ToMM(self.parameters['Pads']['outer_diameter'])
        id = pcbnew.ToMM(self.parameters['Pads']['inner_diameter'])
        
        # Diametral height
        max_height = 20 * 2
        min_height = 8 * 2
        
        self.CheckParam('Pads','outer_diameter',max_value=id+max_height,info="Electrode height must be less than 20mm")
        self.CheckParam('Pads','outer_diameter',min_value=id+min_height,info="Electrode height must be at least 8mm")
        
        self.CheckParam('Pads','inner_diameter',min_value=od-max_height,info="Electrode height must be less than 20mm")
        self.CheckParam('Pads','inner_diameter',max_value=od-min_height,info="Electrode height must be at least 8mm")

    def BuildThisFootprint(self):
        param_steps             = self.pads["steps"]
        param_bands             = self.pads["bands"]
        param_od                = self.pads["outer_diameter"]
        param_id                = self.pads["inner_diameter"]
        param_deadzone          = self.pads["deadzone"]
        param_corner_radius     = self.pads["corner_radius"]
        param_clearance         = self.pads["clearance"]
        param_ss_full           = self.pads["full_silkscreen"]
        param_ss_edge           = self.pads["edge_silkscreen"]

        step_length = float(param_od) / float(param_steps)

        t_size = self.GetTextSize()
        w_text = self.draw.GetLineThickness()
        ypos = param_od/2 + t_size/2 + w_text
        self.draw.Value(0, -ypos, t_size)
        ypos += t_size + w_text*2
        self.draw.Reference(0, -ypos, t_size)

        self.module.SetAttributes(pcbnew.FP_SMD)

        pos = pcbnew.VECTOR2I(0, 0)
        
        if param_ss_edge == True and param_ss_full == False:
            self.smdCircle(param_od / 2,pos)
            self.smdCircle(param_id / 2,pos)
        if param_ss_full == True:
            self.draw_silkscreen_arcs(param_od, param_id, param_deadzone, param_corner_radius, param_steps, param_bands)
        self.create_pads(param_od, param_id, param_deadzone, param_corner_radius, param_steps, param_bands, param_clearance)


ScrollWheelWizard().register()
