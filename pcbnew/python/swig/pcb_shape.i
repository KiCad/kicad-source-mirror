%ignore EDA_SHAPE::getCenter;
%ignore std::abs;
#pragma SWIG nowarn=503

%{
#include <geometry/eda_angle.h>
#include <eda_shape.h>
#include <pcb_shape.h>
%}
%include geometry/eda_angle.h
%include eda_shape.h
%include pcb_shape.h

%extend EDA_ANGLE
{
    /* Implement common operators that use outside-class definition in C++ */
    %pythoncode
    %{
    def __add__(self, other):
        n = EDA_ANGLE(self.AsDegrees(), DEGREES_T)
        n += other
        return n

    def __sub__(self, other):
        n = EDA_ANGLE(self.AsDegrees(), DEGREES_T)
        n -= other
        return n

    def __mul__(self, other):
        return EDA_ANGLE(self.AsDegrees() * other, DEGREES_T)

    def __rmul__(self, other):
        return EDA_ANGLE(other * self.AsDegrees(), DEGREES_T)

    def __truediv__(self, other):
        return EDA_ANGLE(self.AsDegrees() / other, DEGREES_T)

    def __abs__(self):
        return EDA_ANGLE(abs(self.AsDegrees()), DEGREES_T)
    %}
}


%extend PCB_SHAPE
{
    EDA_ANGLE GetArcAngleStart()
    {
        EDA_ANGLE startAngle;
        EDA_ANGLE endAngle;
        $self->CalcArcAngles( startAngle, endAngle );
        return startAngle;
    }

    %pythoncode
    %{
    def GetShapeStr(self):
        return self.ShowShape()
    %}
}

/* Only for compatibility with old python scripts: */
const int S_SEGMENT = (const int)SHAPE_T::SEGMENT;
const int S_RECT = (const int)SHAPE_T::RECTANGLE;
const int S_ARC = (const int)SHAPE_T::ARC;
const int S_CIRCLE = (const int)SHAPE_T::CIRCLE;
const int S_POLYGON = (const int)SHAPE_T::POLY;
const int S_CURVE = (const int)SHAPE_T::BEZIER;
const int SHAPE_T_RECT = (const int)SHAPE_T::RECTANGLE;

%{
/* for compatibility with old python scripts: */
const int S_SEGMENT = (const int)SHAPE_T::SEGMENT;
const int S_RECT = (const int)SHAPE_T::RECTANGLE;
const int S_ARC = (const int)SHAPE_T::ARC;
const int S_CIRCLE = (const int)SHAPE_T::CIRCLE;
const int S_POLYGON = (const int)SHAPE_T::POLY;
const int S_CURVE = (const int)SHAPE_T::BEZIER;
const int SHAPE_T_RECT = (const int)SHAPE_T::RECTANGLE;
%}
