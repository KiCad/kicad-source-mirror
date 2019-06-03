
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

    %}
}
