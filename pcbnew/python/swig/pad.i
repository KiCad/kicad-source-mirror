

%include pad_shapes.h
%include pad.h

%rename(Get) operator   PAD*;
%{
#include <pad.h>
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

    # AddPrimitive() is the old name for D_PAD::AddPrimitivePoly(),
    # PAD::AddPrimitiveSegment(), PAD::AddPrimitiveCircle(),
    # PAD::AddPrimitiveArc(), PAD::AddPrimitiveCurve()
    # define it for compatibility
    def AddPrimitive(self, *args):
        if len(args) == 2:
            return self.AddPrimitivePoly(*args)
        elif len(args) == 3:
            if type(args[1] in [wxPoint,wxSize]):
                return self.AddPrimitiveSegment(*args)
            else:
                return self.AddPrimitiveCircle(*args)
        elif len(args) == 4:
            return self.AddPrimitiveArc(*args)
        elif len(args) == 5:
            return self.AddPrimitiveCurve(*args)
        else:
            raise TypeError("Arguments not recognized.")

    # GetCustomShapeAsPolygon() is the old accessor to get custom shapes
    def GetCustomShapeAsPolygon(self, layer=UNDEFINED_LAYER):
        polygon_set = SHAPE_POLY_SET()
        self.MergePrimitivesAsPolygon(polygon_set)
        return polygon_set
    %}
}
