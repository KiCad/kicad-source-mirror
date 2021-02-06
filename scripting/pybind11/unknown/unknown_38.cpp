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

void bind_unknown_unknown_38(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxULongLongNative file: line:343
		pybind11::class_<wxULongLongNative, std::shared_ptr<wxULongLongNative>> cl(M(""), "wxULongLongNative", "");
		cl.def( pybind11::init( [](){ return new wxULongLongNative(); } ) );
		cl.def( pybind11::init<unsigned long long>(), pybind11::arg("ll") );

		cl.def( pybind11::init<unsigned int, unsigned int>(), pybind11::arg("hi"), pybind11::arg("lo") );

		cl.def( pybind11::init( [](wxULongLongNative const &o){ return new wxULongLongNative(o); } ) );
		cl.def("assign", (class wxULongLongNative & (wxULongLongNative::*)(unsigned long long)) &wxULongLongNative::operator=, "C++: wxULongLongNative::operator=(unsigned long long) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("assign", (class wxULongLongNative & (wxULongLongNative::*)(long long)) &wxULongLongNative::operator=, "C++: wxULongLongNative::operator=(long long) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("assign", (class wxULongLongNative & (wxULongLongNative::*)(int)) &wxULongLongNative::operator=, "C++: wxULongLongNative::operator=(int) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("assign", (class wxULongLongNative & (wxULongLongNative::*)(long)) &wxULongLongNative::operator=, "C++: wxULongLongNative::operator=(long) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("assign", (class wxULongLongNative & (wxULongLongNative::*)(unsigned int)) &wxULongLongNative::operator=, "C++: wxULongLongNative::operator=(unsigned int) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("assign", (class wxULongLongNative & (wxULongLongNative::*)(unsigned long)) &wxULongLongNative::operator=, "C++: wxULongLongNative::operator=(unsigned long) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("assign", (class wxULongLongNative & (wxULongLongNative::*)(const class wxLongLongNative &)) &wxULongLongNative::operator=, "C++: wxULongLongNative::operator=(const class wxLongLongNative &) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("GetHi", (unsigned int (wxULongLongNative::*)() const) &wxULongLongNative::GetHi, "C++: wxULongLongNative::GetHi() const --> unsigned int");
		cl.def("GetLo", (unsigned int (wxULongLongNative::*)() const) &wxULongLongNative::GetLo, "C++: wxULongLongNative::GetLo() const --> unsigned int");
		cl.def("GetValue", (unsigned long long (wxULongLongNative::*)() const) &wxULongLongNative::GetValue, "C++: wxULongLongNative::GetValue() const --> unsigned long long");
		cl.def("ToULong", (unsigned long (wxULongLongNative::*)() const) &wxULongLongNative::ToULong, "C++: wxULongLongNative::ToULong() const --> unsigned long");
		cl.def("ToDouble", (double (wxULongLongNative::*)() const) &wxULongLongNative::ToDouble, "C++: wxULongLongNative::ToDouble() const --> double");
		cl.def("__add__", (class wxULongLongNative (wxULongLongNative::*)(const class wxULongLongNative &) const) &wxULongLongNative::operator+, "C++: wxULongLongNative::operator+(const class wxULongLongNative &) const --> class wxULongLongNative", pybind11::arg("ll"));
		cl.def("__iadd__", (class wxULongLongNative & (wxULongLongNative::*)(const class wxULongLongNative &)) &wxULongLongNative::operator+=, "C++: wxULongLongNative::operator+=(const class wxULongLongNative &) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("__add__", (class wxULongLongNative (wxULongLongNative::*)(const unsigned long long) const) &wxULongLongNative::operator+, "C++: wxULongLongNative::operator+(const unsigned long long) const --> class wxULongLongNative", pybind11::arg("ll"));
		cl.def("__iadd__", (class wxULongLongNative & (wxULongLongNative::*)(const unsigned long long)) &wxULongLongNative::operator+=, "C++: wxULongLongNative::operator+=(const unsigned long long) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("plus_plus", (class wxULongLongNative & (wxULongLongNative::*)()) &wxULongLongNative::operator++, "C++: wxULongLongNative::operator++() --> class wxULongLongNative &", pybind11::return_value_policy::automatic);
		cl.def("plus_plus", (class wxULongLongNative (wxULongLongNative::*)(int)) &wxULongLongNative::operator++, "C++: wxULongLongNative::operator++(int) --> class wxULongLongNative", pybind11::arg(""));
		cl.def("__sub__", (class wxULongLongNative (wxULongLongNative::*)(const class wxULongLongNative &) const) &wxULongLongNative::operator-, "C++: wxULongLongNative::operator-(const class wxULongLongNative &) const --> class wxULongLongNative", pybind11::arg("ll"));
		cl.def("__isub__", (class wxULongLongNative & (wxULongLongNative::*)(const class wxULongLongNative &)) &wxULongLongNative::operator-=, "C++: wxULongLongNative::operator-=(const class wxULongLongNative &) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("__sub__", (class wxULongLongNative (wxULongLongNative::*)(const unsigned long long) const) &wxULongLongNative::operator-, "C++: wxULongLongNative::operator-(const unsigned long long) const --> class wxULongLongNative", pybind11::arg("ll"));
		cl.def("__isub__", (class wxULongLongNative & (wxULongLongNative::*)(const unsigned long long)) &wxULongLongNative::operator-=, "C++: wxULongLongNative::operator-=(const unsigned long long) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("minus_minus", (class wxULongLongNative & (wxULongLongNative::*)()) &wxULongLongNative::operator--, "C++: wxULongLongNative::operator--() --> class wxULongLongNative &", pybind11::return_value_policy::automatic);
		cl.def("minus_minus", (class wxULongLongNative (wxULongLongNative::*)(int)) &wxULongLongNative::operator--, "C++: wxULongLongNative::operator--(int) --> class wxULongLongNative", pybind11::arg(""));
		cl.def("__mul__", (class wxULongLongNative (wxULongLongNative::*)(const class wxULongLongNative &) const) &wxULongLongNative::operator*, "C++: wxULongLongNative::operator*(const class wxULongLongNative &) const --> class wxULongLongNative", pybind11::arg("ll"));
		cl.def("__mul__", (class wxULongLongNative (wxULongLongNative::*)(unsigned long) const) &wxULongLongNative::operator*, "C++: wxULongLongNative::operator*(unsigned long) const --> class wxULongLongNative", pybind11::arg("l"));
		cl.def("__imul__", (class wxULongLongNative & (wxULongLongNative::*)(const class wxULongLongNative &)) &wxULongLongNative::operator*=, "C++: wxULongLongNative::operator*=(const class wxULongLongNative &) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("__imul__", (class wxULongLongNative & (wxULongLongNative::*)(unsigned long)) &wxULongLongNative::operator*=, "C++: wxULongLongNative::operator*=(unsigned long) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("__div__", (class wxULongLongNative (wxULongLongNative::*)(const class wxULongLongNative &) const) &wxULongLongNative::operator/, "C++: wxULongLongNative::operator/(const class wxULongLongNative &) const --> class wxULongLongNative", pybind11::arg("ll"));
		cl.def("__div__", (class wxULongLongNative (wxULongLongNative::*)(unsigned long) const) &wxULongLongNative::operator/, "C++: wxULongLongNative::operator/(unsigned long) const --> class wxULongLongNative", pybind11::arg("l"));
		cl.def("__idiv__", (class wxULongLongNative & (wxULongLongNative::*)(const class wxULongLongNative &)) &wxULongLongNative::operator/=, "C++: wxULongLongNative::operator/=(const class wxULongLongNative &) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("ll"));
		cl.def("__idiv__", (class wxULongLongNative & (wxULongLongNative::*)(unsigned long)) &wxULongLongNative::operator/=, "C++: wxULongLongNative::operator/=(unsigned long) --> class wxULongLongNative &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("__eq__", (bool (wxULongLongNative::*)(const class wxULongLongNative &) const) &wxULongLongNative::operator==, "C++: wxULongLongNative::operator==(const class wxULongLongNative &) const --> bool", pybind11::arg("ll"));
		cl.def("__eq__", (bool (wxULongLongNative::*)(unsigned long) const) &wxULongLongNative::operator==, "C++: wxULongLongNative::operator==(unsigned long) const --> bool", pybind11::arg("l"));
		cl.def("__ne__", (bool (wxULongLongNative::*)(const class wxULongLongNative &) const) &wxULongLongNative::operator!=, "C++: wxULongLongNative::operator!=(const class wxULongLongNative &) const --> bool", pybind11::arg("ll"));
		cl.def("__ne__", (bool (wxULongLongNative::*)(unsigned long) const) &wxULongLongNative::operator!=, "C++: wxULongLongNative::operator!=(unsigned long) const --> bool", pybind11::arg("l"));
		cl.def("ToString", (class wxString (wxULongLongNative::*)() const) &wxULongLongNative::ToString, "C++: wxULongLongNative::ToString() const --> class wxString");
		cl.def("asArray", (void * (wxULongLongNative::*)() const) &wxULongLongNative::asArray, "C++: wxULongLongNative::asArray() const --> void *", pybind11::return_value_policy::automatic);
	}
}
