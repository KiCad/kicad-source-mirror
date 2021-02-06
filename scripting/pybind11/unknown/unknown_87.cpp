#include <sstream> // __str__

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_unknown_unknown_87(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxBrushList file: line:90
		pybind11::class_<wxBrushList, std::shared_ptr<wxBrushList>, wxGDIObjListBase> cl(M(""), "wxBrushList", "");
		cl.def( pybind11::init( [](){ return new wxBrushList(); } ) );
		cl.def( pybind11::init( [](wxBrushList const &o){ return new wxBrushList(o); } ) );
		cl.def("FindOrCreateBrush", [](wxBrushList &o, const class wxColour & a0) -> wxBrush * { return o.FindOrCreateBrush(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("colour"));
		cl.def("FindOrCreateBrush", (class wxBrush * (wxBrushList::*)(const class wxColour &, enum wxBrushStyle)) &wxBrushList::FindOrCreateBrush, "C++: wxBrushList::FindOrCreateBrush(const class wxColour &, enum wxBrushStyle) --> class wxBrush *", pybind11::return_value_policy::automatic, pybind11::arg("colour"), pybind11::arg("style"));
		cl.def("FindOrCreateBrush", (class wxBrush * (wxBrushList::*)(const class wxColour &, int)) &wxBrushList::FindOrCreateBrush, "C++: wxBrushList::FindOrCreateBrush(const class wxColour &, int) --> class wxBrush *", pybind11::return_value_policy::automatic, pybind11::arg("colour"), pybind11::arg("style"));
	}
	// wxOutCode file: line:27
	pybind11::enum_<wxOutCode>(M(""), "wxOutCode", pybind11::arithmetic(), "")
		.value("wxInside", wxInside)
		.value("wxOutLeft", wxOutLeft)
		.value("wxOutRight", wxOutRight)
		.value("wxOutTop", wxOutTop)
		.value("wxOutBottom", wxOutBottom)
		.export_values();

;

	{ // wxPoint2DInt file: line:36
		pybind11::class_<wxPoint2DInt, std::shared_ptr<wxPoint2DInt>> cl(M(""), "wxPoint2DInt", "");
		cl.def( pybind11::init( [](){ return new wxPoint2DInt(); } ) );
		cl.def( pybind11::init<int, int>(), pybind11::arg("x"), pybind11::arg("y") );

		cl.def( pybind11::init( [](wxPoint2DInt const &o){ return new wxPoint2DInt(o); } ) );
		cl.def( pybind11::init<const class wxPoint &>(), pybind11::arg("pt") );

		cl.def_readwrite("m_x", &wxPoint2DInt::m_x);
		cl.def_readwrite("m_y", &wxPoint2DInt::m_y);
		cl.def("GetFloor", (void (wxPoint2DInt::*)(int *, int *) const) &wxPoint2DInt::GetFloor, "C++: wxPoint2DInt::GetFloor(int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetRounded", (void (wxPoint2DInt::*)(int *, int *) const) &wxPoint2DInt::GetRounded, "C++: wxPoint2DInt::GetRounded(int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetVectorLength", (double (wxPoint2DInt::*)() const) &wxPoint2DInt::GetVectorLength, "C++: wxPoint2DInt::GetVectorLength() const --> double");
		cl.def("GetVectorAngle", (double (wxPoint2DInt::*)() const) &wxPoint2DInt::GetVectorAngle, "C++: wxPoint2DInt::GetVectorAngle() const --> double");
		cl.def("SetVectorLength", (void (wxPoint2DInt::*)(double)) &wxPoint2DInt::SetVectorLength, "C++: wxPoint2DInt::SetVectorLength(double) --> void", pybind11::arg("length"));
		cl.def("SetVectorAngle", (void (wxPoint2DInt::*)(double)) &wxPoint2DInt::SetVectorAngle, "C++: wxPoint2DInt::SetVectorAngle(double) --> void", pybind11::arg("degrees"));
		cl.def("SetPolarCoordinates", (void (wxPoint2DInt::*)(int, int)) &wxPoint2DInt::SetPolarCoordinates, "C++: wxPoint2DInt::SetPolarCoordinates(int, int) --> void", pybind11::arg("angle"), pybind11::arg("length"));
		cl.def("Normalize", (void (wxPoint2DInt::*)()) &wxPoint2DInt::Normalize, "C++: wxPoint2DInt::Normalize() --> void");
		cl.def("GetDistance", (double (wxPoint2DInt::*)(const class wxPoint2DInt &) const) &wxPoint2DInt::GetDistance, "C++: wxPoint2DInt::GetDistance(const class wxPoint2DInt &) const --> double", pybind11::arg("pt"));
		cl.def("GetDistanceSquare", (double (wxPoint2DInt::*)(const class wxPoint2DInt &) const) &wxPoint2DInt::GetDistanceSquare, "C++: wxPoint2DInt::GetDistanceSquare(const class wxPoint2DInt &) const --> double", pybind11::arg("pt"));
		cl.def("GetDotProduct", (int (wxPoint2DInt::*)(const class wxPoint2DInt &) const) &wxPoint2DInt::GetDotProduct, "C++: wxPoint2DInt::GetDotProduct(const class wxPoint2DInt &) const --> int", pybind11::arg("vec"));
		cl.def("GetCrossProduct", (int (wxPoint2DInt::*)(const class wxPoint2DInt &) const) &wxPoint2DInt::GetCrossProduct, "C++: wxPoint2DInt::GetCrossProduct(const class wxPoint2DInt &) const --> int", pybind11::arg("vec"));
		cl.def("__sub__", (class wxPoint2DInt (wxPoint2DInt::*)()) &wxPoint2DInt::operator-, "C++: wxPoint2DInt::operator-() --> class wxPoint2DInt");
		cl.def("assign", (class wxPoint2DInt & (wxPoint2DInt::*)(const class wxPoint2DInt &)) &wxPoint2DInt::operator=, "C++: wxPoint2DInt::operator=(const class wxPoint2DInt &) --> class wxPoint2DInt &", pybind11::return_value_policy::automatic, pybind11::arg("pt"));
		cl.def("__iadd__", (class wxPoint2DInt & (wxPoint2DInt::*)(const class wxPoint2DInt &)) &wxPoint2DInt::operator+=, "C++: wxPoint2DInt::operator+=(const class wxPoint2DInt &) --> class wxPoint2DInt &", pybind11::return_value_policy::automatic, pybind11::arg("pt"));
		cl.def("__isub__", (class wxPoint2DInt & (wxPoint2DInt::*)(const class wxPoint2DInt &)) &wxPoint2DInt::operator-=, "C++: wxPoint2DInt::operator-=(const class wxPoint2DInt &) --> class wxPoint2DInt &", pybind11::return_value_policy::automatic, pybind11::arg("pt"));
		cl.def("__imul__", (class wxPoint2DInt & (wxPoint2DInt::*)(const class wxPoint2DInt &)) &wxPoint2DInt::operator*=, "C++: wxPoint2DInt::operator*=(const class wxPoint2DInt &) --> class wxPoint2DInt &", pybind11::return_value_policy::automatic, pybind11::arg("pt"));
		cl.def("__imul__", (class wxPoint2DInt & (wxPoint2DInt::*)(double)) &wxPoint2DInt::operator*=, "C++: wxPoint2DInt::operator*=(double) --> class wxPoint2DInt &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__imul__", (class wxPoint2DInt & (wxPoint2DInt::*)(int)) &wxPoint2DInt::operator*=, "C++: wxPoint2DInt::operator*=(int) --> class wxPoint2DInt &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__idiv__", (class wxPoint2DInt & (wxPoint2DInt::*)(const class wxPoint2DInt &)) &wxPoint2DInt::operator/=, "C++: wxPoint2DInt::operator/=(const class wxPoint2DInt &) --> class wxPoint2DInt &", pybind11::return_value_policy::automatic, pybind11::arg("pt"));
		cl.def("__idiv__", (class wxPoint2DInt & (wxPoint2DInt::*)(double)) &wxPoint2DInt::operator/=, "C++: wxPoint2DInt::operator/=(double) --> class wxPoint2DInt &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__idiv__", (class wxPoint2DInt & (wxPoint2DInt::*)(int)) &wxPoint2DInt::operator/=, "C++: wxPoint2DInt::operator/=(int) --> class wxPoint2DInt &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__eq__", (bool (wxPoint2DInt::*)(const class wxPoint2DInt &) const) &wxPoint2DInt::operator==, "C++: wxPoint2DInt::operator==(const class wxPoint2DInt &) const --> bool", pybind11::arg("pt"));
		cl.def("__ne__", (bool (wxPoint2DInt::*)(const class wxPoint2DInt &) const) &wxPoint2DInt::operator!=, "C++: wxPoint2DInt::operator!=(const class wxPoint2DInt &) const --> bool", pybind11::arg("pt"));
	}
	{ // wxPoint2DDouble file: line:286
		pybind11::class_<wxPoint2DDouble, std::shared_ptr<wxPoint2DDouble>> cl(M(""), "wxPoint2DDouble", "");
		cl.def( pybind11::init( [](){ return new wxPoint2DDouble(); } ) );
		cl.def( pybind11::init<double, double>(), pybind11::arg("x"), pybind11::arg("y") );

		cl.def( pybind11::init( [](wxPoint2DDouble const &o){ return new wxPoint2DDouble(o); } ) );
		cl.def( pybind11::init<const class wxPoint2DInt &>(), pybind11::arg("pt") );

		cl.def( pybind11::init<const class wxPoint &>(), pybind11::arg("pt") );

		cl.def_readwrite("m_x", &wxPoint2DDouble::m_x);
		cl.def_readwrite("m_y", &wxPoint2DDouble::m_y);
		cl.def("GetFloor", (void (wxPoint2DDouble::*)(int *, int *) const) &wxPoint2DDouble::GetFloor, "C++: wxPoint2DDouble::GetFloor(int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetRounded", (void (wxPoint2DDouble::*)(int *, int *) const) &wxPoint2DDouble::GetRounded, "C++: wxPoint2DDouble::GetRounded(int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetVectorLength", (double (wxPoint2DDouble::*)() const) &wxPoint2DDouble::GetVectorLength, "C++: wxPoint2DDouble::GetVectorLength() const --> double");
		cl.def("GetVectorAngle", (double (wxPoint2DDouble::*)() const) &wxPoint2DDouble::GetVectorAngle, "C++: wxPoint2DDouble::GetVectorAngle() const --> double");
		cl.def("SetVectorLength", (void (wxPoint2DDouble::*)(double)) &wxPoint2DDouble::SetVectorLength, "C++: wxPoint2DDouble::SetVectorLength(double) --> void", pybind11::arg("length"));
		cl.def("SetVectorAngle", (void (wxPoint2DDouble::*)(double)) &wxPoint2DDouble::SetVectorAngle, "C++: wxPoint2DDouble::SetVectorAngle(double) --> void", pybind11::arg("degrees"));
		cl.def("SetPolarCoordinates", (void (wxPoint2DDouble::*)(double, double)) &wxPoint2DDouble::SetPolarCoordinates, "C++: wxPoint2DDouble::SetPolarCoordinates(double, double) --> void", pybind11::arg("angle"), pybind11::arg("length"));
		cl.def("Normalize", (void (wxPoint2DDouble::*)()) &wxPoint2DDouble::Normalize, "C++: wxPoint2DDouble::Normalize() --> void");
		cl.def("GetDistance", (double (wxPoint2DDouble::*)(const class wxPoint2DDouble &) const) &wxPoint2DDouble::GetDistance, "C++: wxPoint2DDouble::GetDistance(const class wxPoint2DDouble &) const --> double", pybind11::arg("pt"));
		cl.def("GetDistanceSquare", (double (wxPoint2DDouble::*)(const class wxPoint2DDouble &) const) &wxPoint2DDouble::GetDistanceSquare, "C++: wxPoint2DDouble::GetDistanceSquare(const class wxPoint2DDouble &) const --> double", pybind11::arg("pt"));
		cl.def("GetDotProduct", (double (wxPoint2DDouble::*)(const class wxPoint2DDouble &) const) &wxPoint2DDouble::GetDotProduct, "C++: wxPoint2DDouble::GetDotProduct(const class wxPoint2DDouble &) const --> double", pybind11::arg("vec"));
		cl.def("GetCrossProduct", (double (wxPoint2DDouble::*)(const class wxPoint2DDouble &) const) &wxPoint2DDouble::GetCrossProduct, "C++: wxPoint2DDouble::GetCrossProduct(const class wxPoint2DDouble &) const --> double", pybind11::arg("vec"));
		cl.def("__sub__", (class wxPoint2DDouble (wxPoint2DDouble::*)()) &wxPoint2DDouble::operator-, "C++: wxPoint2DDouble::operator-() --> class wxPoint2DDouble");
		cl.def("assign", (class wxPoint2DDouble & (wxPoint2DDouble::*)(const class wxPoint2DDouble &)) &wxPoint2DDouble::operator=, "C++: wxPoint2DDouble::operator=(const class wxPoint2DDouble &) --> class wxPoint2DDouble &", pybind11::return_value_policy::automatic, pybind11::arg("pt"));
		cl.def("__iadd__", (class wxPoint2DDouble & (wxPoint2DDouble::*)(const class wxPoint2DDouble &)) &wxPoint2DDouble::operator+=, "C++: wxPoint2DDouble::operator+=(const class wxPoint2DDouble &) --> class wxPoint2DDouble &", pybind11::return_value_policy::automatic, pybind11::arg("pt"));
		cl.def("__isub__", (class wxPoint2DDouble & (wxPoint2DDouble::*)(const class wxPoint2DDouble &)) &wxPoint2DDouble::operator-=, "C++: wxPoint2DDouble::operator-=(const class wxPoint2DDouble &) --> class wxPoint2DDouble &", pybind11::return_value_policy::automatic, pybind11::arg("pt"));
		cl.def("__imul__", (class wxPoint2DDouble & (wxPoint2DDouble::*)(const class wxPoint2DDouble &)) &wxPoint2DDouble::operator*=, "C++: wxPoint2DDouble::operator*=(const class wxPoint2DDouble &) --> class wxPoint2DDouble &", pybind11::return_value_policy::automatic, pybind11::arg("pt"));
		cl.def("__imul__", (class wxPoint2DDouble & (wxPoint2DDouble::*)(double)) &wxPoint2DDouble::operator*=, "C++: wxPoint2DDouble::operator*=(double) --> class wxPoint2DDouble &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__imul__", (class wxPoint2DDouble & (wxPoint2DDouble::*)(int)) &wxPoint2DDouble::operator*=, "C++: wxPoint2DDouble::operator*=(int) --> class wxPoint2DDouble &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__idiv__", (class wxPoint2DDouble & (wxPoint2DDouble::*)(const class wxPoint2DDouble &)) &wxPoint2DDouble::operator/=, "C++: wxPoint2DDouble::operator/=(const class wxPoint2DDouble &) --> class wxPoint2DDouble &", pybind11::return_value_policy::automatic, pybind11::arg("pt"));
		cl.def("__idiv__", (class wxPoint2DDouble & (wxPoint2DDouble::*)(double)) &wxPoint2DDouble::operator/=, "C++: wxPoint2DDouble::operator/=(double) --> class wxPoint2DDouble &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__idiv__", (class wxPoint2DDouble & (wxPoint2DDouble::*)(int)) &wxPoint2DDouble::operator/=, "C++: wxPoint2DDouble::operator/=(int) --> class wxPoint2DDouble &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__eq__", (bool (wxPoint2DDouble::*)(const class wxPoint2DDouble &) const) &wxPoint2DDouble::operator==, "C++: wxPoint2DDouble::operator==(const class wxPoint2DDouble &) const --> bool", pybind11::arg("pt"));
		cl.def("__ne__", (bool (wxPoint2DDouble::*)(const class wxPoint2DDouble &) const) &wxPoint2DDouble::operator!=, "C++: wxPoint2DDouble::operator!=(const class wxPoint2DDouble &) const --> bool", pybind11::arg("pt"));
	}
	{ // wxRect2DDouble file: line:520
		pybind11::class_<wxRect2DDouble, std::shared_ptr<wxRect2DDouble>> cl(M(""), "wxRect2DDouble", "");
		cl.def( pybind11::init( [](){ return new wxRect2DDouble(); } ) );
		cl.def( pybind11::init<double, double, double, double>(), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h") );

		cl.def( pybind11::init( [](wxRect2DDouble const &o){ return new wxRect2DDouble(o); } ) );
		cl.def_readwrite("m_x", &wxRect2DDouble::m_x);
		cl.def_readwrite("m_y", &wxRect2DDouble::m_y);
		cl.def_readwrite("m_width", &wxRect2DDouble::m_width);
		cl.def_readwrite("m_height", &wxRect2DDouble::m_height);
		cl.def("GetPosition", (class wxPoint2DDouble (wxRect2DDouble::*)() const) &wxRect2DDouble::GetPosition, "C++: wxRect2DDouble::GetPosition() const --> class wxPoint2DDouble");
		cl.def("GetSize", (class wxSize (wxRect2DDouble::*)() const) &wxRect2DDouble::GetSize, "C++: wxRect2DDouble::GetSize() const --> class wxSize");
		cl.def("GetLeft", (double (wxRect2DDouble::*)() const) &wxRect2DDouble::GetLeft, "C++: wxRect2DDouble::GetLeft() const --> double");
		cl.def("SetLeft", (void (wxRect2DDouble::*)(double)) &wxRect2DDouble::SetLeft, "C++: wxRect2DDouble::SetLeft(double) --> void", pybind11::arg("n"));
		cl.def("MoveLeftTo", (void (wxRect2DDouble::*)(double)) &wxRect2DDouble::MoveLeftTo, "C++: wxRect2DDouble::MoveLeftTo(double) --> void", pybind11::arg("n"));
		cl.def("GetTop", (double (wxRect2DDouble::*)() const) &wxRect2DDouble::GetTop, "C++: wxRect2DDouble::GetTop() const --> double");
		cl.def("SetTop", (void (wxRect2DDouble::*)(double)) &wxRect2DDouble::SetTop, "C++: wxRect2DDouble::SetTop(double) --> void", pybind11::arg("n"));
		cl.def("MoveTopTo", (void (wxRect2DDouble::*)(double)) &wxRect2DDouble::MoveTopTo, "C++: wxRect2DDouble::MoveTopTo(double) --> void", pybind11::arg("n"));
		cl.def("GetBottom", (double (wxRect2DDouble::*)() const) &wxRect2DDouble::GetBottom, "C++: wxRect2DDouble::GetBottom() const --> double");
		cl.def("SetBottom", (void (wxRect2DDouble::*)(double)) &wxRect2DDouble::SetBottom, "C++: wxRect2DDouble::SetBottom(double) --> void", pybind11::arg("n"));
		cl.def("MoveBottomTo", (void (wxRect2DDouble::*)(double)) &wxRect2DDouble::MoveBottomTo, "C++: wxRect2DDouble::MoveBottomTo(double) --> void", pybind11::arg("n"));
		cl.def("GetRight", (double (wxRect2DDouble::*)() const) &wxRect2DDouble::GetRight, "C++: wxRect2DDouble::GetRight() const --> double");
		cl.def("SetRight", (void (wxRect2DDouble::*)(double)) &wxRect2DDouble::SetRight, "C++: wxRect2DDouble::SetRight(double) --> void", pybind11::arg("n"));
		cl.def("MoveRightTo", (void (wxRect2DDouble::*)(double)) &wxRect2DDouble::MoveRightTo, "C++: wxRect2DDouble::MoveRightTo(double) --> void", pybind11::arg("n"));
		cl.def("GetLeftTop", (class wxPoint2DDouble (wxRect2DDouble::*)() const) &wxRect2DDouble::GetLeftTop, "C++: wxRect2DDouble::GetLeftTop() const --> class wxPoint2DDouble");
		cl.def("SetLeftTop", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::SetLeftTop, "C++: wxRect2DDouble::SetLeftTop(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("MoveLeftTopTo", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::MoveLeftTopTo, "C++: wxRect2DDouble::MoveLeftTopTo(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("GetLeftBottom", (class wxPoint2DDouble (wxRect2DDouble::*)() const) &wxRect2DDouble::GetLeftBottom, "C++: wxRect2DDouble::GetLeftBottom() const --> class wxPoint2DDouble");
		cl.def("SetLeftBottom", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::SetLeftBottom, "C++: wxRect2DDouble::SetLeftBottom(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("MoveLeftBottomTo", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::MoveLeftBottomTo, "C++: wxRect2DDouble::MoveLeftBottomTo(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("GetRightTop", (class wxPoint2DDouble (wxRect2DDouble::*)() const) &wxRect2DDouble::GetRightTop, "C++: wxRect2DDouble::GetRightTop() const --> class wxPoint2DDouble");
		cl.def("SetRightTop", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::SetRightTop, "C++: wxRect2DDouble::SetRightTop(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("MoveRightTopTo", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::MoveRightTopTo, "C++: wxRect2DDouble::MoveRightTopTo(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("GetRightBottom", (class wxPoint2DDouble (wxRect2DDouble::*)() const) &wxRect2DDouble::GetRightBottom, "C++: wxRect2DDouble::GetRightBottom() const --> class wxPoint2DDouble");
		cl.def("SetRightBottom", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::SetRightBottom, "C++: wxRect2DDouble::SetRightBottom(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("MoveRightBottomTo", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::MoveRightBottomTo, "C++: wxRect2DDouble::MoveRightBottomTo(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("GetCentre", (class wxPoint2DDouble (wxRect2DDouble::*)() const) &wxRect2DDouble::GetCentre, "C++: wxRect2DDouble::GetCentre() const --> class wxPoint2DDouble");
		cl.def("SetCentre", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::SetCentre, "C++: wxRect2DDouble::SetCentre(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("MoveCentreTo", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::MoveCentreTo, "C++: wxRect2DDouble::MoveCentreTo(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("GetOutCode", (enum wxOutCode (wxRect2DDouble::*)(const class wxPoint2DDouble &) const) &wxRect2DDouble::GetOutCode, "C++: wxRect2DDouble::GetOutCode(const class wxPoint2DDouble &) const --> enum wxOutCode", pybind11::arg("pt"));
		cl.def("GetOutcode", (enum wxOutCode (wxRect2DDouble::*)(const class wxPoint2DDouble &) const) &wxRect2DDouble::GetOutcode, "C++: wxRect2DDouble::GetOutcode(const class wxPoint2DDouble &) const --> enum wxOutCode", pybind11::arg("pt"));
		cl.def("Contains", (bool (wxRect2DDouble::*)(const class wxPoint2DDouble &) const) &wxRect2DDouble::Contains, "C++: wxRect2DDouble::Contains(const class wxPoint2DDouble &) const --> bool", pybind11::arg("pt"));
		cl.def("Contains", (bool (wxRect2DDouble::*)(const class wxRect2DDouble &) const) &wxRect2DDouble::Contains, "C++: wxRect2DDouble::Contains(const class wxRect2DDouble &) const --> bool", pybind11::arg("rect"));
		cl.def("IsEmpty", (bool (wxRect2DDouble::*)() const) &wxRect2DDouble::IsEmpty, "C++: wxRect2DDouble::IsEmpty() const --> bool");
		cl.def("HaveEqualSize", (bool (wxRect2DDouble::*)(const class wxRect2DDouble &) const) &wxRect2DDouble::HaveEqualSize, "C++: wxRect2DDouble::HaveEqualSize(const class wxRect2DDouble &) const --> bool", pybind11::arg("rect"));
		cl.def("Inset", (void (wxRect2DDouble::*)(double, double)) &wxRect2DDouble::Inset, "C++: wxRect2DDouble::Inset(double, double) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("Inset", (void (wxRect2DDouble::*)(double, double, double, double)) &wxRect2DDouble::Inset, "C++: wxRect2DDouble::Inset(double, double, double, double) --> void", pybind11::arg("left"), pybind11::arg("top"), pybind11::arg("right"), pybind11::arg("bottom"));
		cl.def("Offset", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::Offset, "C++: wxRect2DDouble::Offset(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("ConstrainTo", (void (wxRect2DDouble::*)(const class wxRect2DDouble &)) &wxRect2DDouble::ConstrainTo, "C++: wxRect2DDouble::ConstrainTo(const class wxRect2DDouble &) --> void", pybind11::arg("rect"));
		cl.def("Interpolate", (class wxPoint2DDouble (wxRect2DDouble::*)(int, int)) &wxRect2DDouble::Interpolate, "C++: wxRect2DDouble::Interpolate(int, int) --> class wxPoint2DDouble", pybind11::arg("widthfactor"), pybind11::arg("heightfactor"));
		cl.def_static("Intersect", (void (*)(const class wxRect2DDouble &, const class wxRect2DDouble &, class wxRect2DDouble *)) &wxRect2DDouble::Intersect, "C++: wxRect2DDouble::Intersect(const class wxRect2DDouble &, const class wxRect2DDouble &, class wxRect2DDouble *) --> void", pybind11::arg("src1"), pybind11::arg("src2"), pybind11::arg("dest"));
		cl.def("Intersect", (void (wxRect2DDouble::*)(const class wxRect2DDouble &)) &wxRect2DDouble::Intersect, "C++: wxRect2DDouble::Intersect(const class wxRect2DDouble &) --> void", pybind11::arg("otherRect"));
		cl.def("CreateIntersection", (class wxRect2DDouble (wxRect2DDouble::*)(const class wxRect2DDouble &) const) &wxRect2DDouble::CreateIntersection, "C++: wxRect2DDouble::CreateIntersection(const class wxRect2DDouble &) const --> class wxRect2DDouble", pybind11::arg("otherRect"));
		cl.def("Intersects", (bool (wxRect2DDouble::*)(const class wxRect2DDouble &) const) &wxRect2DDouble::Intersects, "C++: wxRect2DDouble::Intersects(const class wxRect2DDouble &) const --> bool", pybind11::arg("rect"));
		cl.def_static("Union", (void (*)(const class wxRect2DDouble &, const class wxRect2DDouble &, class wxRect2DDouble *)) &wxRect2DDouble::Union, "C++: wxRect2DDouble::Union(const class wxRect2DDouble &, const class wxRect2DDouble &, class wxRect2DDouble *) --> void", pybind11::arg("src1"), pybind11::arg("src2"), pybind11::arg("dest"));
		cl.def("Union", (void (wxRect2DDouble::*)(const class wxRect2DDouble &)) &wxRect2DDouble::Union, "C++: wxRect2DDouble::Union(const class wxRect2DDouble &) --> void", pybind11::arg("otherRect"));
		cl.def("Union", (void (wxRect2DDouble::*)(const class wxPoint2DDouble &)) &wxRect2DDouble::Union, "C++: wxRect2DDouble::Union(const class wxPoint2DDouble &) --> void", pybind11::arg("pt"));
		cl.def("CreateUnion", (class wxRect2DDouble (wxRect2DDouble::*)(const class wxRect2DDouble &) const) &wxRect2DDouble::CreateUnion, "C++: wxRect2DDouble::CreateUnion(const class wxRect2DDouble &) const --> class wxRect2DDouble", pybind11::arg("otherRect"));
		cl.def("Scale", (void (wxRect2DDouble::*)(double)) &wxRect2DDouble::Scale, "C++: wxRect2DDouble::Scale(double) --> void", pybind11::arg("f"));
		cl.def("Scale", (void (wxRect2DDouble::*)(int, int)) &wxRect2DDouble::Scale, "C++: wxRect2DDouble::Scale(int, int) --> void", pybind11::arg("num"), pybind11::arg("denum"));
		cl.def("assign", (class wxRect2DDouble & (wxRect2DDouble::*)(const class wxRect2DDouble &)) &wxRect2DDouble::operator=, "C++: wxRect2DDouble::operator=(const class wxRect2DDouble &) --> class wxRect2DDouble &", pybind11::return_value_policy::automatic, pybind11::arg("rect"));
		cl.def("__eq__", (bool (wxRect2DDouble::*)(const class wxRect2DDouble &) const) &wxRect2DDouble::operator==, "C++: wxRect2DDouble::operator==(const class wxRect2DDouble &) const --> bool", pybind11::arg("rect"));
		cl.def("__ne__", (bool (wxRect2DDouble::*)(const class wxRect2DDouble &) const) &wxRect2DDouble::operator!=, "C++: wxRect2DDouble::operator!=(const class wxRect2DDouble &) const --> bool", pybind11::arg("rect"));
	}
}
