%rename(AddPrimitiveShape) PAD::AddPrimitive;

%{
#include <pad.h>
#include <padstack.h>
%}

%include padstack.h
%include pad.h

%rename(Get) operator   PAD*;

/* Only for compatibility with old python scripts: */
const int PAD_SHAPE_RECT = (const int)PAD_SHAPE::RECTANGLE;

%{
const int PAD_SHAPE_RECT = (const int)PAD_SHAPE::RECTANGLE;
const int PAD_DRILL_SHAPE_CIRCLE = (const int)PAD_DRILL_SHAPE::CIRCLE;
const int PAD_DRILL_SHAPE_OBLONG = (const int)PAD_DRILL_SHAPE::OBLONG;
%}

%extend PAD
{
    // Overrides to make non-padstack-aware scripts continue to work
    PAD_SHAPE GetShape() { return $self->GetShape( F_Cu ); }
    void SetShape( PAD_SHAPE aShape ) { $self->SetShape( F_Cu, aShape ); }

    VECTOR2I GetSize() { return $self->GetSize( F_Cu ); }
    void SetSize( VECTOR2I aSize ) { $self->SetSize( F_Cu, aSize ); }

    VECTOR2I GetDelta() { return $self->GetDelta( F_Cu ); }
    void SetDelta( VECTOR2I aSize ) { $self->SetDelta( F_Cu, aSize ); }

    VECTOR2I GetOffset() { return $self->GetOffset( F_Cu ); }
    void SetOffset( VECTOR2I aOffset ) { $self->SetOffset( F_Cu, aOffset ); }

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
            return self.AddPrimitivePoly(F_Cu, *args, True)
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
        self.MergePrimitivesAsPolygon(F_Cu, polygon_set)
        return polygon_set
    %}
}
