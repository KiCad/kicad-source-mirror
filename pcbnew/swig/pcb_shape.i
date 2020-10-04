
%include pcb_shape.h
%extend PCB_SHAPE
{
    %pythoncode
    %{
    def GetShapeStr(self):
        return self.ShowShape(self.GetShape())
    %}
}
%{
#include <pcb_shape.h>
%}

