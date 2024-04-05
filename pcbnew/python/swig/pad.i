%rename(AddPrimitiveShape) PAD::AddPrimitive;

%include pad_shapes.h
%include pad.h

%rename(Get) operator   PAD*;
%{
#include <pad.h>
%}

/* Only for compatibility with old python scripts: */
const int PAD_SHAPE_RECT = (const int)PAD_SHAPE::RECTANGLE;

%{
const int PAD_SHAPE_RECT = (const int)PAD_SHAPE::RECTANGLE;
%}

%extend PAD
{
    %pythoncode
    %{

    # SetPadName() is the old name for PAD::SetName()
    # define it for compatibility
    def SetPadName(self, aName):
        return self.SetNumber(aName)

    def SetName(self, aName):
        return self.SetNumber(aName)

    # GetPadName() is the old name for PAD::GetName()
    # define it for compatibility
    def GetPadName(self):
        return self.GetNumber()

    def GetName(self):
        return self.GetNumber()

    # AddPrimitive() used to be multiple functions on the C++ side and this single Python function
    # was made to maintain compatibility with an even older version of the PAD class that had a
    # single function.  Now we're back to a single function, but different, and Python scripts
    # have gotten used to this API, so keep compatibility with it
    def AddPrimitive(self, *args):
        if len(args) == 2:
            return self.AddPrimitivePoly(*args, True)
        elif len(args) == 3:
            if type(args[1] in [wxPoint,wxSize,VECTOR2I]):
                s = PCB_SHAPE(None, SHAPE_T_SEGMENT)
                s.SetStart(args[0])
                s.SetEnd(args[1])
                s.SetWidth(args[2])
            else:
                s = PCB_SHAPE(None, SHAPE_T_CIRCLE)
                s.SetCenter(args[0])
                s.SetRadius(args[1])
                s.SetWidth(args[2])
        elif len(args) == 4:
            s = PCB_SHAPE(None, SHAPE_T_ARC)
            s.SetCenter(args[0])
            s.SetStart(args[1])
            s.SetArcAngleAndEnd(args[2])
            s.SetWidth(args[3])
        elif len(args) == 5:
            s = PCB_SHAPE(None, SHAPE_T_BEZIER)
            s.SetStart(args[0])
            s.SetEnd(args[1])
            s.SetBezierC1(args[2])
            s.SetBezierC2(args[3])
            s.SetWidth(args[4])
        else:
            raise TypeError(f"Arguments not recognized; expected 2-5 args, got {len(args)}")

        self.AddPrimitiveShape(s)

    # GetCustomShapeAsPolygon() is the old accessor to get custom shapes
    def GetCustomShapeAsPolygon(self, layer=UNDEFINED_LAYER):
        polygon_set = SHAPE_POLY_SET()
        self.MergePrimitivesAsPolygon(polygon_set)
        return polygon_set
    %}
}
