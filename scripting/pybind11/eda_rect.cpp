#include <eda_rect.h> // EDA_RECT
#include <fill_type.h> // FILL_TYPE
#include <gal/color4d.h> // EDA_COLOR_T
#include <sstream> // __str__
#include <transform.h> // TRANSFORM

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_eda_rect(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // EDA_RECT file:eda_rect.h line:42
		pybind11::class_<EDA_RECT, std::shared_ptr<EDA_RECT>> cl(M(""), "EDA_RECT", "Handle the component boundary box.\n\n This class is similar to wxRect, but some wxRect functions are very curious, and are\n working only if dimensions are >= 0 (not always the case in KiCad) and also KiCad needs\n some specific method which makes this a more suitable class.");
		cl.def( pybind11::init( [](){ return new EDA_RECT(); } ) );
		cl.def( pybind11::init<const class wxPoint &, const class wxSize &>(), pybind11::arg("aPos"), pybind11::arg("aSize") );

		cl.def( pybind11::init( [](EDA_RECT const &o){ return new EDA_RECT(o); } ) );
		cl.def("Centre", (class wxPoint (EDA_RECT::*)() const) &EDA_RECT::Centre, "C++: EDA_RECT::Centre() const --> class wxPoint");
		cl.def("Move", (void (EDA_RECT::*)(const class wxPoint &)) &EDA_RECT::Move, "Move the rectangle by the \n\n \n A wxPoint that is the value to move this rectangle.\n\nC++: EDA_RECT::Move(const class wxPoint &) --> void", pybind11::arg("aMoveVector"));
		cl.def("Normalize", (void (EDA_RECT::*)()) &EDA_RECT::Normalize, "Ensures that the height ant width are positive.\n\nC++: EDA_RECT::Normalize() --> void");
		cl.def("Contains", (bool (EDA_RECT::*)(const class wxPoint &) const) &EDA_RECT::Contains, "the wxPoint to test.\n \n\n true if aPoint is inside the boundary box. A point on a edge is seen as inside.\n\nC++: EDA_RECT::Contains(const class wxPoint &) const --> bool", pybind11::arg("aPoint"));
		cl.def("Contains", (bool (EDA_RECT::*)(int, int) const) &EDA_RECT::Contains, "the x coordinate of the point to test.\n \n\n the x coordinate of the point to test.\n \n\n true if point is inside the boundary box. A point on a edge is seen as inside\n\nC++: EDA_RECT::Contains(int, int) const --> bool", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("Contains", (bool (EDA_RECT::*)(const class EDA_RECT &) const) &EDA_RECT::Contains, "the EDA_RECT to test.\n \n\n true if aRect is Contained. A common edge is seen as contained.\n\nC++: EDA_RECT::Contains(const class EDA_RECT &) const --> bool", pybind11::arg("aRect"));
		cl.def("GetSize", (const class wxSize (EDA_RECT::*)() const) &EDA_RECT::GetSize, "C++: EDA_RECT::GetSize() const --> const class wxSize");
		cl.def("GetSizeMax", (int (EDA_RECT::*)() const) &EDA_RECT::GetSizeMax, "the max size dimension.\n\nC++: EDA_RECT::GetSizeMax() const --> int");
		cl.def("GetX", (int (EDA_RECT::*)() const) &EDA_RECT::GetX, "C++: EDA_RECT::GetX() const --> int");
		cl.def("GetY", (int (EDA_RECT::*)() const) &EDA_RECT::GetY, "C++: EDA_RECT::GetY() const --> int");
		cl.def("GetOrigin", (const class wxPoint (EDA_RECT::*)() const) &EDA_RECT::GetOrigin, "C++: EDA_RECT::GetOrigin() const --> const class wxPoint");
		cl.def("GetPosition", (const class wxPoint (EDA_RECT::*)() const) &EDA_RECT::GetPosition, "C++: EDA_RECT::GetPosition() const --> const class wxPoint");
		cl.def("GetEnd", (const class wxPoint (EDA_RECT::*)() const) &EDA_RECT::GetEnd, "C++: EDA_RECT::GetEnd() const --> const class wxPoint");
		cl.def("GetCenter", (const class wxPoint (EDA_RECT::*)() const) &EDA_RECT::GetCenter, "C++: EDA_RECT::GetCenter() const --> const class wxPoint");
		cl.def("GetWidth", (int (EDA_RECT::*)() const) &EDA_RECT::GetWidth, "C++: EDA_RECT::GetWidth() const --> int");
		cl.def("GetHeight", (int (EDA_RECT::*)() const) &EDA_RECT::GetHeight, "C++: EDA_RECT::GetHeight() const --> int");
		cl.def("GetRight", (int (EDA_RECT::*)() const) &EDA_RECT::GetRight, "C++: EDA_RECT::GetRight() const --> int");
		cl.def("GetLeft", (int (EDA_RECT::*)() const) &EDA_RECT::GetLeft, "C++: EDA_RECT::GetLeft() const --> int");
		cl.def("GetTop", (int (EDA_RECT::*)() const) &EDA_RECT::GetTop, "C++: EDA_RECT::GetTop() const --> int");
		cl.def("GetBottom", (int (EDA_RECT::*)() const) &EDA_RECT::GetBottom, "C++: EDA_RECT::GetBottom() const --> int");
		cl.def("IsValid", (bool (EDA_RECT::*)() const) &EDA_RECT::IsValid, "C++: EDA_RECT::IsValid() const --> bool");
		cl.def("SetOrigin", (void (EDA_RECT::*)(const class wxPoint &)) &EDA_RECT::SetOrigin, "C++: EDA_RECT::SetOrigin(const class wxPoint &) --> void", pybind11::arg("pos"));
		cl.def("SetOrigin", (void (EDA_RECT::*)(int, int)) &EDA_RECT::SetOrigin, "C++: EDA_RECT::SetOrigin(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("SetSize", (void (EDA_RECT::*)(const class wxSize &)) &EDA_RECT::SetSize, "C++: EDA_RECT::SetSize(const class wxSize &) --> void", pybind11::arg("size"));
		cl.def("SetSize", (void (EDA_RECT::*)(int, int)) &EDA_RECT::SetSize, "C++: EDA_RECT::SetSize(int, int) --> void", pybind11::arg("w"), pybind11::arg("h"));
		cl.def("Offset", (void (EDA_RECT::*)(int, int)) &EDA_RECT::Offset, "C++: EDA_RECT::Offset(int, int) --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("Offset", (void (EDA_RECT::*)(const class wxPoint &)) &EDA_RECT::Offset, "C++: EDA_RECT::Offset(const class wxPoint &) --> void", pybind11::arg("offset"));
		cl.def("SetX", (void (EDA_RECT::*)(int)) &EDA_RECT::SetX, "C++: EDA_RECT::SetX(int) --> void", pybind11::arg("val"));
		cl.def("SetY", (void (EDA_RECT::*)(int)) &EDA_RECT::SetY, "C++: EDA_RECT::SetY(int) --> void", pybind11::arg("val"));
		cl.def("SetWidth", (void (EDA_RECT::*)(int)) &EDA_RECT::SetWidth, "C++: EDA_RECT::SetWidth(int) --> void", pybind11::arg("val"));
		cl.def("SetHeight", (void (EDA_RECT::*)(int)) &EDA_RECT::SetHeight, "C++: EDA_RECT::SetHeight(int) --> void", pybind11::arg("val"));
		cl.def("SetEnd", (void (EDA_RECT::*)(int, int)) &EDA_RECT::SetEnd, "C++: EDA_RECT::SetEnd(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("SetEnd", (void (EDA_RECT::*)(const class wxPoint &)) &EDA_RECT::SetEnd, "C++: EDA_RECT::SetEnd(const class wxPoint &) --> void", pybind11::arg("pos"));
		cl.def("RevertYAxis", (void (EDA_RECT::*)()) &EDA_RECT::RevertYAxis, "Mirror the rectangle from the X axis (negate Y pos and size).\n\nC++: EDA_RECT::RevertYAxis() --> void");
		cl.def("Intersects", (bool (EDA_RECT::*)(const class EDA_RECT &) const) &EDA_RECT::Intersects, "Test for a common area between rectangles.\n\n \n A rectangle to test intersection with.\n \n\n true if the argument rectangle intersects this rectangle.\n (i.e. if the 2 rectangles have at least a common point)\n\nC++: EDA_RECT::Intersects(const class EDA_RECT &) const --> bool", pybind11::arg("aRect"));
		cl.def("Intersects", (bool (EDA_RECT::*)(const class EDA_RECT &, double) const) &EDA_RECT::Intersects, "Tests for a common area between this rectangle, and a rectangle with arbitrary rotation\n\n \n a rectangle to test intersection with.\n \n\n rectangle rotation (in 1/10 degrees).\n\nC++: EDA_RECT::Intersects(const class EDA_RECT &, double) const --> bool", pybind11::arg("aRect"), pybind11::arg("aRot"));
		cl.def("Intersects", (bool (EDA_RECT::*)(const class wxPoint &, const class wxPoint &) const) &EDA_RECT::Intersects, "Test for a common area between a segment and this rectangle.\n\n \n First point of the segment to test intersection with.\n \n\n Second point of the segment to test intersection with.\n \n\n true if the argument segment intersects this rectangle.\n (i.e. if the segment and rectangle have at least a common point)\n\nC++: EDA_RECT::Intersects(const class wxPoint &, const class wxPoint &) const --> bool", pybind11::arg("aPoint1"), pybind11::arg("aPoint2"));
		cl.def("Intersects", (bool (EDA_RECT::*)(const class wxPoint &, const class wxPoint &, class wxPoint *, class wxPoint *) const) &EDA_RECT::Intersects, "Test for intersection between a segment and this rectangle, returning the intersections.\n\n \n is the first point of the segment to test intersection with.\n \n\n is the second point of the segment to test intersection with.\n \n\n will be filled with the first intersection point, if any.\n \n\n will be filled with the second intersection point, if any.\n \n\n true if the segment intersects the rect.\n\nC++: EDA_RECT::Intersects(const class wxPoint &, const class wxPoint &, class wxPoint *, class wxPoint *) const --> bool", pybind11::arg("aPoint1"), pybind11::arg("aPoint2"), pybind11::arg("aIntersection1"), pybind11::arg("aIntersection2"));
		cl.def("ClosestPointTo", (const class wxPoint (EDA_RECT::*)(const class wxPoint &) const) &EDA_RECT::ClosestPointTo, "Return the point in this rect that is closest to the provided point\n\nC++: EDA_RECT::ClosestPointTo(const class wxPoint &) const --> const class wxPoint", pybind11::arg("aPoint"));
		cl.def("FarthestPointTo", (const class wxPoint (EDA_RECT::*)(const class wxPoint &) const) &EDA_RECT::FarthestPointTo, "Return the point in this rect that is farthest from the provided point\n\nC++: EDA_RECT::FarthestPointTo(const class wxPoint &) const --> const class wxPoint", pybind11::arg("aPoint"));
		cl.def("IntersectsCircle", (bool (EDA_RECT::*)(const class wxPoint &, const int) const) &EDA_RECT::IntersectsCircle, "Test for a common area between a circle and this rectangle.\n\n \n center of the circle.\n \n\n radius of the circle.\n\nC++: EDA_RECT::IntersectsCircle(const class wxPoint &, const int) const --> bool", pybind11::arg("aCenter"), pybind11::arg("aRadius"));
		cl.def("IntersectsCircleEdge", (bool (EDA_RECT::*)(const class wxPoint &, const int, const int) const) &EDA_RECT::IntersectsCircleEdge, "Test for intersection between this rect and the edge (radius) of a circle.\n\n \n center of the circle.\n \n\n radius of the circle.\n \n\n width of the circle edge.\n\nC++: EDA_RECT::IntersectsCircleEdge(const class wxPoint &, const int, const int) const --> bool", pybind11::arg("aCenter"), pybind11::arg("aRadius"), pybind11::arg("aWidth"));
		cl.def("Inflate", (class EDA_RECT & (EDA_RECT::*)(int)) &EDA_RECT::Inflate, "Inflate the rectangle horizontally and vertically by  If \n is negative the rectangle is deflated.\n\nC++: EDA_RECT::Inflate(int) --> class EDA_RECT &", pybind11::return_value_policy::automatic, pybind11::arg("aDelta"));
		cl.def("Merge", (void (EDA_RECT::*)(const class EDA_RECT &)) &EDA_RECT::Merge, "Modify the position and size of the rectangle in order to contain \n\n It is mainly used to calculate bounding boxes.\n\n \n  The rectangle to merge with this rectangle.\n\nC++: EDA_RECT::Merge(const class EDA_RECT &) --> void", pybind11::arg("aRect"));
		cl.def("Merge", (void (EDA_RECT::*)(const class wxPoint &)) &EDA_RECT::Merge, "Modify the position and size of the rectangle in order to contain the given point.\n\n \n The point to merge with the rectangle.\n\nC++: EDA_RECT::Merge(const class wxPoint &) --> void", pybind11::arg("aPoint"));
		cl.def("GetArea", (double (EDA_RECT::*)() const) &EDA_RECT::GetArea, "Return the area of the rectangle.\n\n \n The area of the rectangle.\n\nC++: EDA_RECT::GetArea() const --> double");
		cl.def("Common", (class EDA_RECT (EDA_RECT::*)(const class EDA_RECT &) const) &EDA_RECT::Common, "Return the area that is common with another rectangle.\n\n \n is the rectangle to find the common area with.\n \n\n The common area rect or 0-sized rectangle if there is no intersection.\n\nC++: EDA_RECT::Common(const class EDA_RECT &) const --> class EDA_RECT", pybind11::arg("aRect"));
		cl.def("GetBoundingBoxRotated", (const class EDA_RECT (EDA_RECT::*)(class wxPoint, double) const) &EDA_RECT::GetBoundingBoxRotated, "Useful to calculate bounding box of rotated items, when rotation if not k*90 degrees.\n\n \n the bounding box of this, after rotation.\n \n\n the rotation angle in 0.1 deg.\n \n\n the rotation point.\n\nC++: EDA_RECT::GetBoundingBoxRotated(class wxPoint, double) const --> const class EDA_RECT", pybind11::arg("aRotCenter"), pybind11::arg("aAngle"));
		cl.def("assign", (class EDA_RECT & (EDA_RECT::*)(const class EDA_RECT &)) &EDA_RECT::operator=, "C++: EDA_RECT::operator=(const class EDA_RECT &) --> class EDA_RECT &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// FILL_TYPE file:fill_type.h line:28
	pybind11::enum_<FILL_TYPE>(M(""), "FILL_TYPE", "The set of fill types used in plotting or drawing enclosed areas.\n\n \n Do not renumber this enum, the legacy schematic plugin demands on these values.")
		.value("NO_FILL", FILL_TYPE::NO_FILL)
		.value("FILLED_SHAPE", FILL_TYPE::FILLED_SHAPE)
		.value("FILLED_WITH_BG_BODYCOLOR", FILL_TYPE::FILLED_WITH_BG_BODYCOLOR)
		.value("FILLED_WITH_COLOR", FILL_TYPE::FILLED_WITH_COLOR);

;

	{ // TRANSFORM file:transform.h line:45
		pybind11::class_<TRANSFORM, std::shared_ptr<TRANSFORM>> cl(M(""), "TRANSFORM", "for transforming drawing coordinates for a wxDC device context.\n\n This probably should be a base class with all pure virtual methods and a WXDC_TRANSFORM\n derived class.  Then in the future if some new device context is used, a new transform could\n be derived from the base class and all the drawable objects would have to do is provide\n overloaded draw methods to use the new transorm.");
		cl.def( pybind11::init( [](){ return new TRANSFORM(); } ) );
		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("ax1"), pybind11::arg("ay1"), pybind11::arg("ax2"), pybind11::arg("ay2") );

		cl.def( pybind11::init( [](TRANSFORM const &o){ return new TRANSFORM(o); } ) );
		cl.def_readwrite("x1", &TRANSFORM::x1);
		cl.def_readwrite("y1", &TRANSFORM::y1);
		cl.def_readwrite("x2", &TRANSFORM::x2);
		cl.def_readwrite("y2", &TRANSFORM::y2);
		cl.def("__eq__", (bool (TRANSFORM::*)(const class TRANSFORM &) const) &TRANSFORM::operator==, "C++: TRANSFORM::operator==(const class TRANSFORM &) const --> bool", pybind11::arg("aTransform"));
		cl.def("__ne__", (bool (TRANSFORM::*)(const class TRANSFORM &) const) &TRANSFORM::operator!=, "C++: TRANSFORM::operator!=(const class TRANSFORM &) const --> bool", pybind11::arg("aTransform"));
		cl.def("TransformCoordinate", (class wxPoint (TRANSFORM::*)(const class wxPoint &) const) &TRANSFORM::TransformCoordinate, "Calculate a new coordinate according to the mirror/rotation transform.\n Useful to calculate actual coordinates of a point\n from coordinates relative to a component\n which are given for a non rotated, non mirrored item\n \n\n = The position to transform\n \n\n The transformed coordinate.\n\nC++: TRANSFORM::TransformCoordinate(const class wxPoint &) const --> class wxPoint", pybind11::arg("aPoint"));
		cl.def("TransformCoordinate", (class EDA_RECT (TRANSFORM::*)(const class EDA_RECT &) const) &TRANSFORM::TransformCoordinate, "Calculate a new rect according to the mirror/rotation transform.\n Useful to calculate actual coordinates of a point\n from coordinates relative to a component\n which are given for a non rotated, non mirrored item\n \n\n = The rectangle to transform\n \n\n The transformed rectangle.\n\nC++: TRANSFORM::TransformCoordinate(const class EDA_RECT &) const --> class EDA_RECT", pybind11::arg("aRect"));
		cl.def("InverseTransform", (class TRANSFORM (TRANSFORM::*)() const) &TRANSFORM::InverseTransform, "Calculate the Inverse mirror/rotation transform.\n Useful to calculate coordinates relative to a component\n which must be for a non rotated, non mirrored item\n from the actual coordinate.\n \n\n The inverse transform.\n\nC++: TRANSFORM::InverseTransform() const --> class TRANSFORM");
		cl.def("MapAngles", (bool (TRANSFORM::*)(int *, int *) const) &TRANSFORM::MapAngles, "Calculate new angles according to the transform.\n\n \n = The first angle to transform\n \n\n = The second angle to transform\n \n\n True if the angles were swapped during the transform.\n\nC++: TRANSFORM::MapAngles(int *, int *) const --> bool", pybind11::arg("aAngle1"), pybind11::arg("aAngle2"));
		cl.def("assign", (class TRANSFORM & (TRANSFORM::*)(const class TRANSFORM &)) &TRANSFORM::operator=, "C++: TRANSFORM::operator=(const class TRANSFORM &) --> class TRANSFORM &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// EDA_COLOR_T file:gal/color4d.h line:41
	pybind11::enum_<EDA_COLOR_T>(M(""), "EDA_COLOR_T", pybind11::arithmetic(), "Legacy color enumeration. Also contains a flag and the alpha value in the upper bits")
		.value("UNSPECIFIED_COLOR", UNSPECIFIED_COLOR)
		.value("BLACK", BLACK)
		.value("DARKDARKGRAY", DARKDARKGRAY)
		.value("DARKGRAY", DARKGRAY)
		.value("LIGHTGRAY", LIGHTGRAY)
		.value("WHITE", WHITE)
		.value("LIGHTYELLOW", LIGHTYELLOW)
		.value("DARKBLUE", DARKBLUE)
		.value("DARKGREEN", DARKGREEN)
		.value("DARKCYAN", DARKCYAN)
		.value("DARKRED", DARKRED)
		.value("DARKMAGENTA", DARKMAGENTA)
		.value("DARKBROWN", DARKBROWN)
		.value("BLUE", BLUE)
		.value("GREEN", GREEN)
		.value("CYAN", CYAN)
		.value("RED", RED)
		.value("MAGENTA", MAGENTA)
		.value("BROWN", BROWN)
		.value("LIGHTBLUE", LIGHTBLUE)
		.value("LIGHTGREEN", LIGHTGREEN)
		.value("LIGHTCYAN", LIGHTCYAN)
		.value("LIGHTRED", LIGHTRED)
		.value("LIGHTMAGENTA", LIGHTMAGENTA)
		.value("YELLOW", YELLOW)
		.value("PUREBLUE", PUREBLUE)
		.value("PUREGREEN", PUREGREEN)
		.value("PURECYAN", PURECYAN)
		.value("PURERED", PURERED)
		.value("PUREMAGENTA", PUREMAGENTA)
		.value("PUREYELLOW", PUREYELLOW)
		.value("NBCOLORS", NBCOLORS)
		.value("HIGHLIGHT_FLAG", HIGHLIGHT_FLAG)
		.value("MASKCOLOR", MASKCOLOR)
		.export_values();

;

}
