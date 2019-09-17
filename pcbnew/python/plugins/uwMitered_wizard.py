# This python script wizard creates a mitered bend for microwave applications
# https://lists.launchpad.net/kicad-developers/msg17996.html
# Author  Henrik Forsten

from __future__ import division
import FootprintWizardBase

from pcbnew import *
import math

class UWMiterFootprintWizard(FootprintWizardBase.FootprintWizard):
    def GetName(self):
        return "uW Mitered Bend"

    def GetDescription(self):
        return "Mitered Bend Footprint Wizard"

    def GenerateParameterList(self):
        self.AddParam("Corner", "width", self.uMM, 1.0)
        self.AddParam("Corner", "height", self.uMM, 1.57)
        self.AddParam("Corner", "*angle", self.uDegrees, 90)
        


# class UWMiterFootprintWizard(FootprintWizardBase.FootprintWizard):
#     def __init__(self):
#         FootprintWizardBase.FootprintWizard.__init__(self)
#         self.name = "uW Mitered Bend"
#         self.description = "Mitered Bend Footprint Wizard"
#         self.parameters = {
#             "Corner":{
#                      "width":           FromMM(0.34),
#                      "height":          FromMM(0.17),
#                      "*angle":          90,
#                      }
#         }
#
#        self.ClearErrors()

    # build a rectangular pad
    def smdRectPad(self, module, size, pos, name, angle):
            pad = D_PAD(module)
            pad.SetSize(size)
            pad.SetShape(PAD_SHAPE_RECT) #PAD_RECT)
            pad.SetAttribute(PAD_ATTRIB_SMD) #PAD_SMD)
            #Set only the copper layer without mask
            #since nothing is mounted on these pads
            pad.SetLayerSet( LSET(F_Cu) )
            pad.SetPos0(pos)
            pad.SetPosition(pos)
            pad.SetPadName(name)
            pad.Rotate(pos, angle)
            #Set clearance to small value, because
            #pads can be very close together.
            #If distance is smaller than clearance
            #DRC doesn't allow routing the pads
            pad.SetLocalClearance(1)
            return pad

    def Polygon(self, points, layer):
            """
            Draw a polygon through specified points
            """
            import pcbnew
            
            polygon = pcbnew.EDGE_MODULE(self.module)
            polygon.SetWidth(0) #Disables outline

            polygon.SetLayer(layer)
            polygon.SetShape(pcbnew.S_POLYGON)

            polygon.SetPolyPoints(points)

            self.module.Add(polygon)


    # This method checks the parameters provided to wizard and set errors
    def CheckParameters(self):
        p = self.parameters
        width = p["Corner"]["width"]
        height = p["Corner"]["height"]
        angle = p["Corner"]["*angle"]

        errors = []
        if (width<0):
            errors.append("Width has invalid value")
        if width/height < 0.25:
            errors.append("Too small width to height ratio")
        if angle > 90:
            errors.append("Too large angle")
        if angle < 0:
            errors.append("Angle must be positive")
        errors = ', '.join(errors)
        print (errors)
        return errors == ""

    def bilinear_interpolation(self, x, y, points):
        '''http://stackoverflow.com/questions/8661537/how-to-perform-bilinear-interpolation-in-python
        Interpolate (x,y) from values associated with four points.

        The four points are a list of four triplets:  (x, y, value).
        The four points can be in any order.  They should form a rectangle.

            >>> bilinear_interpolation(12, 5.5,
            ...                        [(10, 4, 100),
            ...                         (20, 4, 200),
            ...                         (10, 6, 150),
            ...                         (20, 6, 300)])
            165.0

        '''
        # See formula at:  http://en.wikipedia.org/wiki/Bilinear_interpolation

        points = sorted(points)               # order points by x, then by y
        (x1, y1, q11), (_x1, y2, q12), (x2, _y1, q21), (_x2, _y2, q22) = points

        return (q11 * (x2 - x) * (y2 - y) +
                q21 * (x - x1) * (y2 - y) +
                q12 * (x2 - x) * (y - y1) +
                q22 * (x - x1) * (y - y1)
               ) / ((x2 - x1) * (y2 - y1) + 0.0)

    def OptimalMiter(self, w, h, angle):
        """Calculate optimal miter by interpolating from table.
        https://awrcorp.com/download/faq/english/docs/Elements/MBENDA.htm
        """
        wh = w/h
        whs = [0.5, 1.0, 2.0]
        angles = [0, 30, 60, 90, 120]
        table = [
            [0, 12, 45, 75, 98],
            [0, 19, 41, 63, 92],
            [0, 7, 31, 56, 79]
        ]
        for i, x in enumerate(whs):
            if x > wh:
                break
        for j, y in enumerate(angles):
            if y > angle:
                break
        i = min(i-1,1)
        j = min(j-1,3)
        px = lambda ii,jj: (whs[ii],angles[jj],table[ii][jj])
        x1 = px(i,j)
        x2 = px(i+1,j)
        y1 = px(i,j+1)
        y2 = px(i+1,j+1)
        return self.bilinear_interpolation(wh, angle, [x1,x2,y1,y2])/100.0

    # build the footprint from parameters
    #def BuildThisFootprint(self):
    def BuildFootprint(self):

        module = MODULE(None) # create a new module
        self.module = module
        self.buildmessages = ""

        if not self.CheckParameters():
            return

        p = self.parameters
        width = p["Corner"]["width"]
        height = p["Corner"]["height"]
        angle_deg = float(p["Corner"]["*angle"])
        angle = angle_deg*0.0174532925 #To radians

        #reference and value
        #text_size = self.GetTextSize()  # IPC nominal
        
        textposy = width + FromMM(1)
        size_text = wxSize( FromMM( 0.6), FromMM( 0.5) )
        
        module.name = "'uwm_{0:.2f}_{1:0.2f}_{2:.0f}'".format(ToMM(width),ToMM(height),angle_deg)
        
        #module.SetReference("uwm_{0:.2f}_{1:0.2f}_{2:.0f}".format(ToMM(width),ToMM(height),angle_deg))
        module.SetReference("uwM***")           # give it a default value
        module.Reference().SetPos0(wxPoint(0, textposy))
        module.Reference().SetPosition(module.Reference().GetPos0())
        module.Reference().SetTextSize( size_text )
        module.Reference().SetThickness( FromMM( 0.125) )
        module.Reference().SetVisible(True)

        textposy = textposy + FromMM(1)
        #module.SetValue("Val***")           # give it a default value
        module.SetValue("uwm_{0:.2f}_{1:0.2f}_{2:.0f}".format(ToMM(width),ToMM(height),angle_deg))
        module.Value().SetPos0( wxPoint(0, textposy) )
        module.Value().SetPosition(module.Value().GetPos0())
        module.Value().SetTextSize( size_text )
        module.Value().SetVisible(False) #0)

        # fpid = FPID(self.module.GetReference())   #the name in library
        # module.SetFPID( fpid )

        #Calculate the miter
        w = width

        #Width of the corner from edge of the corner to inside corner
        corner_width = ToMM(w)/math.cos(angle/2)

        #Get proportion of width to cut
        cut = self.OptimalMiter(width, height, angle_deg)
        cut_pc = cut
        print ("Cut: {0:.2f}%".format(cut*100))

        #Distance from uncut outside corner point to point 7
        cut = FromMM(cut*corner_width/math.cos((math.pi-angle)/2))

        #Distance between points 2 and 3 and points 3 and 4
        #Minimum of w/2 to satisfy DRC, otherwise pads are too close
        #and track connected to other pad overlaps the other one.
        #Rounded trace end can also stick out of the cut area
        #if a is too small.
        a = max(cut-width*math.tan(angle/2),w/2)

        #Distance between points 3 and 4
        x34 = a*math.sin(angle)
        y34 = a*math.cos(angle)
        #Distance between points 4 and 5
        x45 = width*math.cos(angle)
        y45 = width*math.sin(angle)

        #  1  2
        #8 +--+
        #  |  |3
        #7 \  --+ 4
        #   \   |
        #    \--+ 5
        #    6

        points = [
                (0,0),
                (w,0),
                (w,a),
                (w+x34,a+y34),
                (w+x34-x45,a+y34+y45),
                (cut*math.sin(angle),a+width*math.tan(angle/2)+cut*math.cos(angle)),
                (0,a+width*math.tan(angle/2)-cut),
                (0,0)]

        #Last two points can be equal
        if points[-2] == points[-1]:
            points = points[:-1]

        points = [wxPoint(*point) for point in points]

        self.Polygon(points, F_Cu)

        #Create pads
        pad_l = width/2 #10 allowing big track to join the fp
        size_pad = wxSize(width,pad_l)
        module.Add(self.smdRectPad(module, size_pad, wxPoint(width/2,-pad_l/2), "1", 0))
        size_pad = wxSize(pad_l,width)

        #Halfway between points 4 and 5
        posx = ((w+x34) + (w+x34-x45))/2
        posy = ((a+y34) + (a+y34+y45))/2

        #Position pad so that pad edge touches polygon edge
        posx += (pad_l/2)*math.sin(angle)
        posy += (pad_l/2)*math.cos(angle)
        size_pad = wxSize(pad_l, width)
        module.Add(self.smdRectPad(module, size_pad, wxPoint(posx,posy), "1", (angle_deg-90)*10))
        # moving anchor to center of first pad
        module.MoveAnchorPosition(wxPoint(-width/2,pad_l/2))
        self.buildmessages = (
            "Building new {name} footprint with the following parameters:\n\n"
            .format(name=module.name))
        self.buildmessages += ("Track Width: {0:.4f}mm\n".format(ToMM(width)))
        self.buildmessages += ("PCB Height: {0:.4f}mm\n".format(ToMM(height)))
        self.buildmessages += ("Angle: {:.1f}deg\n\n".format(angle_deg))
        self.buildmessages += ("Cut: {0:.2f}%".format(cut_pc*100))
        

# create our footprint wizard
uwmiter_wizard = UWMiterFootprintWizard()

# register it into pcbnew
uwmiter_wizard.register()
