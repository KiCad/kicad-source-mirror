#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_unknown_unknown_37(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxKeyboardState file: line:19
		pybind11::class_<wxKeyboardState, std::shared_ptr<wxKeyboardState>> cl(M(""), "wxKeyboardState", "");
		cl.def( pybind11::init( [](){ return new wxKeyboardState(); } ), "doc" );
		cl.def( pybind11::init( [](bool const & a0){ return new wxKeyboardState(a0); } ), "doc" , pybind11::arg("controlDown"));
		cl.def( pybind11::init( [](bool const & a0, bool const & a1){ return new wxKeyboardState(a0, a1); } ), "doc" , pybind11::arg("controlDown"), pybind11::arg("shiftDown"));
		cl.def( pybind11::init( [](bool const & a0, bool const & a1, bool const & a2){ return new wxKeyboardState(a0, a1, a2); } ), "doc" , pybind11::arg("controlDown"), pybind11::arg("shiftDown"), pybind11::arg("altDown"));
		cl.def( pybind11::init<bool, bool, bool, bool>(), pybind11::arg("controlDown"), pybind11::arg("shiftDown"), pybind11::arg("altDown"), pybind11::arg("metaDown") );

		cl.def( pybind11::init( [](wxKeyboardState const &o){ return new wxKeyboardState(o); } ) );
		cl.def_readwrite("m_controlDown", &wxKeyboardState::m_controlDown);
		cl.def_readwrite("m_shiftDown", &wxKeyboardState::m_shiftDown);
		cl.def_readwrite("m_altDown", &wxKeyboardState::m_altDown);
		cl.def_readwrite("m_metaDown", &wxKeyboardState::m_metaDown);
		cl.def("GetModifiers", (int (wxKeyboardState::*)() const) &wxKeyboardState::GetModifiers, "C++: wxKeyboardState::GetModifiers() const --> int");
		cl.def("HasAnyModifiers", (bool (wxKeyboardState::*)() const) &wxKeyboardState::HasAnyModifiers, "C++: wxKeyboardState::HasAnyModifiers() const --> bool");
		cl.def("HasModifiers", (bool (wxKeyboardState::*)() const) &wxKeyboardState::HasModifiers, "C++: wxKeyboardState::HasModifiers() const --> bool");
		cl.def("ControlDown", (bool (wxKeyboardState::*)() const) &wxKeyboardState::ControlDown, "C++: wxKeyboardState::ControlDown() const --> bool");
		cl.def("RawControlDown", (bool (wxKeyboardState::*)() const) &wxKeyboardState::RawControlDown, "C++: wxKeyboardState::RawControlDown() const --> bool");
		cl.def("ShiftDown", (bool (wxKeyboardState::*)() const) &wxKeyboardState::ShiftDown, "C++: wxKeyboardState::ShiftDown() const --> bool");
		cl.def("MetaDown", (bool (wxKeyboardState::*)() const) &wxKeyboardState::MetaDown, "C++: wxKeyboardState::MetaDown() const --> bool");
		cl.def("AltDown", (bool (wxKeyboardState::*)() const) &wxKeyboardState::AltDown, "C++: wxKeyboardState::AltDown() const --> bool");
		cl.def("CmdDown", (bool (wxKeyboardState::*)() const) &wxKeyboardState::CmdDown, "C++: wxKeyboardState::CmdDown() const --> bool");
		cl.def("SetControlDown", (void (wxKeyboardState::*)(bool)) &wxKeyboardState::SetControlDown, "C++: wxKeyboardState::SetControlDown(bool) --> void", pybind11::arg("down"));
		cl.def("SetRawControlDown", (void (wxKeyboardState::*)(bool)) &wxKeyboardState::SetRawControlDown, "C++: wxKeyboardState::SetRawControlDown(bool) --> void", pybind11::arg("down"));
		cl.def("SetShiftDown", (void (wxKeyboardState::*)(bool)) &wxKeyboardState::SetShiftDown, "C++: wxKeyboardState::SetShiftDown(bool) --> void", pybind11::arg("down"));
		cl.def("SetAltDown", (void (wxKeyboardState::*)(bool)) &wxKeyboardState::SetAltDown, "C++: wxKeyboardState::SetAltDown(bool) --> void", pybind11::arg("down"));
		cl.def("SetMetaDown", (void (wxKeyboardState::*)(bool)) &wxKeyboardState::SetMetaDown, "C++: wxKeyboardState::SetMetaDown(bool) --> void", pybind11::arg("down"));
		cl.def("assign", (class wxKeyboardState & (wxKeyboardState::*)(const class wxKeyboardState &)) &wxKeyboardState::operator=, "C++: wxKeyboardState::operator=(const class wxKeyboardState &) --> class wxKeyboardState &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxMouseButton file: line:17
	pybind11::enum_<wxMouseButton>(M(""), "wxMouseButton", pybind11::arithmetic(), "")
		.value("wxMOUSE_BTN_ANY", wxMOUSE_BTN_ANY)
		.value("wxMOUSE_BTN_NONE", wxMOUSE_BTN_NONE)
		.value("wxMOUSE_BTN_LEFT", wxMOUSE_BTN_LEFT)
		.value("wxMOUSE_BTN_MIDDLE", wxMOUSE_BTN_MIDDLE)
		.value("wxMOUSE_BTN_RIGHT", wxMOUSE_BTN_RIGHT)
		.value("wxMOUSE_BTN_AUX1", wxMOUSE_BTN_AUX1)
		.value("wxMOUSE_BTN_AUX2", wxMOUSE_BTN_AUX2)
		.value("wxMOUSE_BTN_MAX", wxMOUSE_BTN_MAX)
		.export_values();

;

	{ // wxMouseState file: line:36
		pybind11::class_<wxMouseState, std::shared_ptr<wxMouseState>, wxKeyboardState> cl(M(""), "wxMouseState", "");
		cl.def( pybind11::init( [](){ return new wxMouseState(); } ) );
		cl.def( pybind11::init( [](wxMouseState const &o){ return new wxMouseState(o); } ) );
		cl.def_readwrite("m_leftDown", &wxMouseState::m_leftDown);
		cl.def_readwrite("m_middleDown", &wxMouseState::m_middleDown);
		cl.def_readwrite("m_rightDown", &wxMouseState::m_rightDown);
		cl.def_readwrite("m_aux1Down", &wxMouseState::m_aux1Down);
		cl.def_readwrite("m_aux2Down", &wxMouseState::m_aux2Down);
		cl.def_readwrite("m_x", &wxMouseState::m_x);
		cl.def_readwrite("m_y", &wxMouseState::m_y);
		cl.def("GetX", (int (wxMouseState::*)() const) &wxMouseState::GetX, "C++: wxMouseState::GetX() const --> int");
		cl.def("GetY", (int (wxMouseState::*)() const) &wxMouseState::GetY, "C++: wxMouseState::GetY() const --> int");
		cl.def("GetPosition", (class wxPoint (wxMouseState::*)() const) &wxMouseState::GetPosition, "C++: wxMouseState::GetPosition() const --> class wxPoint");
		cl.def("GetPosition", (void (wxMouseState::*)(int *, int *) const) &wxMouseState::GetPosition, "C++: wxMouseState::GetPosition(int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetPosition", (void (wxMouseState::*)(long *, long *) const) &wxMouseState::GetPosition, "C++: wxMouseState::GetPosition(long *, long *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("LeftIsDown", (bool (wxMouseState::*)() const) &wxMouseState::LeftIsDown, "C++: wxMouseState::LeftIsDown() const --> bool");
		cl.def("MiddleIsDown", (bool (wxMouseState::*)() const) &wxMouseState::MiddleIsDown, "C++: wxMouseState::MiddleIsDown() const --> bool");
		cl.def("RightIsDown", (bool (wxMouseState::*)() const) &wxMouseState::RightIsDown, "C++: wxMouseState::RightIsDown() const --> bool");
		cl.def("Aux1IsDown", (bool (wxMouseState::*)() const) &wxMouseState::Aux1IsDown, "C++: wxMouseState::Aux1IsDown() const --> bool");
		cl.def("Aux2IsDown", (bool (wxMouseState::*)() const) &wxMouseState::Aux2IsDown, "C++: wxMouseState::Aux2IsDown() const --> bool");
		cl.def("ButtonIsDown", (bool (wxMouseState::*)(enum wxMouseButton) const) &wxMouseState::ButtonIsDown, "C++: wxMouseState::ButtonIsDown(enum wxMouseButton) const --> bool", pybind11::arg("but"));
		cl.def("SetX", (void (wxMouseState::*)(int)) &wxMouseState::SetX, "C++: wxMouseState::SetX(int) --> void", pybind11::arg("x"));
		cl.def("SetY", (void (wxMouseState::*)(int)) &wxMouseState::SetY, "C++: wxMouseState::SetY(int) --> void", pybind11::arg("y"));
		cl.def("SetPosition", (void (wxMouseState::*)(class wxPoint)) &wxMouseState::SetPosition, "C++: wxMouseState::SetPosition(class wxPoint) --> void", pybind11::arg("pos"));
		cl.def("SetLeftDown", (void (wxMouseState::*)(bool)) &wxMouseState::SetLeftDown, "C++: wxMouseState::SetLeftDown(bool) --> void", pybind11::arg("down"));
		cl.def("SetMiddleDown", (void (wxMouseState::*)(bool)) &wxMouseState::SetMiddleDown, "C++: wxMouseState::SetMiddleDown(bool) --> void", pybind11::arg("down"));
		cl.def("SetRightDown", (void (wxMouseState::*)(bool)) &wxMouseState::SetRightDown, "C++: wxMouseState::SetRightDown(bool) --> void", pybind11::arg("down"));
		cl.def("SetAux1Down", (void (wxMouseState::*)(bool)) &wxMouseState::SetAux1Down, "C++: wxMouseState::SetAux1Down(bool) --> void", pybind11::arg("down"));
		cl.def("SetAux2Down", (void (wxMouseState::*)(bool)) &wxMouseState::SetAux2Down, "C++: wxMouseState::SetAux2Down(bool) --> void", pybind11::arg("down"));
		cl.def("SetState", (void (wxMouseState::*)(const class wxMouseState &)) &wxMouseState::SetState, "C++: wxMouseState::SetState(const class wxMouseState &) --> void", pybind11::arg("state"));
		cl.def("LeftDown", (bool (wxMouseState::*)() const) &wxMouseState::LeftDown, "C++: wxMouseState::LeftDown() const --> bool");
		cl.def("MiddleDown", (bool (wxMouseState::*)() const) &wxMouseState::MiddleDown, "C++: wxMouseState::MiddleDown() const --> bool");
		cl.def("RightDown", (bool (wxMouseState::*)() const) &wxMouseState::RightDown, "C++: wxMouseState::RightDown() const --> bool");
		cl.def("assign", (class wxMouseState & (wxMouseState::*)(const class wxMouseState &)) &wxMouseState::operator=, "C++: wxMouseState::operator=(const class wxMouseState &) --> class wxMouseState &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxLongLongNative file: line:106
		pybind11::class_<wxLongLongNative, std::shared_ptr<wxLongLongNative>> cl(M(""), "wxLongLongNative", "");
		cl.def( pybind11::init( [](){ return new wxLongLongNative(); } ) );
		cl.def( pybind11::init<long long>(), pybind11::arg("ll") );

		cl.def( pybind11::init<int, unsigned int>(), pybind11::arg("hi"), pybind11::arg("lo") );

		cl.def( pybind11::init( [](wxLongLongNative const &o){ return new wxLongLongNative(o); } ) );
		cl.def("assign", (class wxLongLongNative & (wxLongLongNative::*)(long long)) &wxLongLongNative::operator=, "C++: wxLongLongNative::operator=(long long) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("assign", (class wxLongLongNative & (wxLongLongNative::*)(unsigned long long)) &wxLongLongNative::operator=, "C++: wxLongLongNative::operator=(unsigned long long) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("assign", (class wxLongLongNative & (wxLongLongNative::*)(const class wxULongLongNative &)) &wxLongLongNative::operator=, "C++: wxLongLongNative::operator=(const class wxULongLongNative &) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("assign", (class wxLongLongNative & (wxLongLongNative::*)(int)) &wxLongLongNative::operator=, "C++: wxLongLongNative::operator=(int) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("assign", (class wxLongLongNative & (wxLongLongNative::*)(long)) &wxLongLongNative::operator=, "C++: wxLongLongNative::operator=(long) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("assign", (class wxLongLongNative & (wxLongLongNative::*)(unsigned int)) &wxLongLongNative::operator=, "C++: wxLongLongNative::operator=(unsigned int) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("assign", (class wxLongLongNative & (wxLongLongNative::*)(unsigned long)) &wxLongLongNative::operator=, "C++: wxLongLongNative::operator=(unsigned long) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("Assign", (class wxLongLongNative & (wxLongLongNative::*)(double)) &wxLongLongNative::Assign, "C++: wxLongLongNative::Assign(double) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("d"));
		cl.def("GetHi", (int (wxLongLongNative::*)() const) &wxLongLongNative::GetHi, "C++: wxLongLongNative::GetHi() const --> int");
		cl.def("GetLo", (unsigned int (wxLongLongNative::*)() const) &wxLongLongNative::GetLo, "C++: wxLongLongNative::GetLo() const --> unsigned int");
		cl.def("Abs", (class wxLongLongNative & (wxLongLongNative::*)()) &wxLongLongNative::Abs, "C++: wxLongLongNative::Abs() --> class wxLongLongNative &", pybind11::return_value_policy::automatic);
		cl.def("GetValue", (long long (wxLongLongNative::*)() const) &wxLongLongNative::GetValue, "C++: wxLongLongNative::GetValue() const --> long long");
		cl.def("ToLong", (long (wxLongLongNative::*)() const) &wxLongLongNative::ToLong, "C++: wxLongLongNative::ToLong() const --> long");
		cl.def("ToDouble", (double (wxLongLongNative::*)() const) &wxLongLongNative::ToDouble, "C++: wxLongLongNative::ToDouble() const --> double");
		cl.def("__add__", (class wxLongLongNative (wxLongLongNative::*)(const class wxLongLongNative &) const) &wxLongLongNative::operator+, "C++: wxLongLongNative::operator+(const class wxLongLongNative &) const --> class wxLongLongNative", pybind11::arg("ll"));
		cl.def("__iadd__", (class wxLongLongNative & (wxLongLongNative::*)(const class wxLongLongNative &)) &wxLongLongNative::operator+=, "C++: wxLongLongNative::operator+=(const class wxLongLongNative &) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("__add__", (class wxLongLongNative (wxLongLongNative::*)(const long long) const) &wxLongLongNative::operator+, "C++: wxLongLongNative::operator+(const long long) const --> class wxLongLongNative", pybind11::arg("ll"));
		cl.def("__iadd__", (class wxLongLongNative & (wxLongLongNative::*)(const long long)) &wxLongLongNative::operator+=, "C++: wxLongLongNative::operator+=(const long long) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("plus_plus", (class wxLongLongNative & (wxLongLongNative::*)()) &wxLongLongNative::operator++, "C++: wxLongLongNative::operator++() --> class wxLongLongNative &", pybind11::return_value_policy::automatic);
		cl.def("plus_plus", (class wxLongLongNative (wxLongLongNative::*)(int)) &wxLongLongNative::operator++, "C++: wxLongLongNative::operator++(int) --> class wxLongLongNative", pybind11::arg(""));
		cl.def("__sub__", (class wxLongLongNative (wxLongLongNative::*)() const) &wxLongLongNative::operator-, "C++: wxLongLongNative::operator-() const --> class wxLongLongNative");
		cl.def("Negate", (class wxLongLongNative & (wxLongLongNative::*)()) &wxLongLongNative::Negate, "C++: wxLongLongNative::Negate() --> class wxLongLongNative &", pybind11::return_value_policy::automatic);
		cl.def("__sub__", (class wxLongLongNative (wxLongLongNative::*)(const class wxLongLongNative &) const) &wxLongLongNative::operator-, "C++: wxLongLongNative::operator-(const class wxLongLongNative &) const --> class wxLongLongNative", pybind11::arg("ll"));
		cl.def("__isub__", (class wxLongLongNative & (wxLongLongNative::*)(const class wxLongLongNative &)) &wxLongLongNative::operator-=, "C++: wxLongLongNative::operator-=(const class wxLongLongNative &) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("__sub__", (class wxLongLongNative (wxLongLongNative::*)(const long long) const) &wxLongLongNative::operator-, "C++: wxLongLongNative::operator-(const long long) const --> class wxLongLongNative", pybind11::arg("ll"));
		cl.def("__isub__", (class wxLongLongNative & (wxLongLongNative::*)(const long long)) &wxLongLongNative::operator-=, "C++: wxLongLongNative::operator-=(const long long) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("minus_minus", (class wxLongLongNative & (wxLongLongNative::*)()) &wxLongLongNative::operator--, "C++: wxLongLongNative::operator--() --> class wxLongLongNative &", pybind11::return_value_policy::automatic);
		cl.def("minus_minus", (class wxLongLongNative (wxLongLongNative::*)(int)) &wxLongLongNative::operator--, "C++: wxLongLongNative::operator--(int) --> class wxLongLongNative", pybind11::arg(""));
		cl.def("__mul__", (class wxLongLongNative (wxLongLongNative::*)(const class wxLongLongNative &) const) &wxLongLongNative::operator*, "C++: wxLongLongNative::operator*(const class wxLongLongNative &) const --> class wxLongLongNative", pybind11::arg("ll"));
		cl.def("__mul__", (class wxLongLongNative (wxLongLongNative::*)(long) const) &wxLongLongNative::operator*, "C++: wxLongLongNative::operator*(long) const --> class wxLongLongNative", pybind11::arg("l"));
		cl.def("__imul__", (class wxLongLongNative & (wxLongLongNative::*)(const class wxLongLongNative &)) &wxLongLongNative::operator*=, "C++: wxLongLongNative::operator*=(const class wxLongLongNative &) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("__imul__", (class wxLongLongNative & (wxLongLongNative::*)(long)) &wxLongLongNative::operator*=, "C++: wxLongLongNative::operator*=(long) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("__div__", (class wxLongLongNative (wxLongLongNative::*)(const class wxLongLongNative &) const) &wxLongLongNative::operator/, "C++: wxLongLongNative::operator/(const class wxLongLongNative &) const --> class wxLongLongNative", pybind11::arg("ll"));
		cl.def("__div__", (class wxLongLongNative (wxLongLongNative::*)(long) const) &wxLongLongNative::operator/, "C++: wxLongLongNative::operator/(long) const --> class wxLongLongNative", pybind11::arg("l"));
		cl.def("__idiv__", (class wxLongLongNative & (wxLongLongNative::*)(const class wxLongLongNative &)) &wxLongLongNative::operator/=, "C++: wxLongLongNative::operator/=(const class wxLongLongNative &) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("__idiv__", (class wxLongLongNative & (wxLongLongNative::*)(long)) &wxLongLongNative::operator/=, "C++: wxLongLongNative::operator/=(long) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("__eq__", (bool (wxLongLongNative::*)(const class wxLongLongNative &) const) &wxLongLongNative::operator==, "C++: wxLongLongNative::operator==(const class wxLongLongNative &) const --> bool", pybind11::arg("ll"));
		cl.def("__eq__", (bool (wxLongLongNative::*)(long) const) &wxLongLongNative::operator==, "C++: wxLongLongNative::operator==(long) const --> bool", pybind11::arg("l"));
		cl.def("__ne__", (bool (wxLongLongNative::*)(const class wxLongLongNative &) const) &wxLongLongNative::operator!=, "C++: wxLongLongNative::operator!=(const class wxLongLongNative &) const --> bool", pybind11::arg("ll"));
		cl.def("__ne__", (bool (wxLongLongNative::*)(long) const) &wxLongLongNative::operator!=, "C++: wxLongLongNative::operator!=(long) const --> bool", pybind11::arg("l"));
		cl.def("ToString", (class wxString (wxLongLongNative::*)() const) &wxLongLongNative::ToString, "C++: wxLongLongNative::ToString() const --> class wxString");
		cl.def("asArray", (void * (wxLongLongNative::*)() const) &wxLongLongNative::asArray, "C++: wxLongLongNative::asArray() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxLongLongNative & (wxLongLongNative::*)(const class wxLongLongNative &)) &wxLongLongNative::operator=, "C++: wxLongLongNative::operator=(const class wxLongLongNative &) --> class wxLongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
