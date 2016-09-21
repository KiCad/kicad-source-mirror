
%include class_drawsegment.h
%extend DRAWSEGMENT
{
    %pythoncode
    %{
    def GetShapeStr(self):
        return self.ShowShape(self.GetShape())
    %}
}
%{
#include <class_drawsegment.h>
%}

