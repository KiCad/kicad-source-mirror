

%include pad_shapes.h
%include class_pad.h

%rename(Get) operator   D_PAD*;
%{
#include <class_pad.h>
%}

%extend D_PAD
{
    %pythoncode
    %{

    # SetPadName() is the old name for D_PAD::SetName()
    # define it for compatibility
    def SetPadName(self, aName):
        return self.SetName(aName)

    # GetPadName() is the old name for D_PAD::GetName()
    # define it for compatibility
    def GetPadName(self):
        return self.GetName()

    # AddPrimitive() is the old name for D_PAD::AddPrimitivePoly(),
    # D_PAD::AddPrimitiveSegment(), D_PAD::AddPrimitiveCircle(),
    # D_PAD::AddPrimitiveArc(), D_PAD::AddPrimitiveCurve()
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
    %}
}
