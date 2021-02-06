#include <iterator> // __gnu_cxx::__normal_iterator
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

void bind_unknown_unknown_7(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxUniChar file: line:22
		pybind11::class_<wxUniChar, std::shared_ptr<wxUniChar>> cl(M(""), "wxUniChar", "");
		cl.def( pybind11::init( [](){ return new wxUniChar(); } ) );
		cl.def( pybind11::init<char>(), pybind11::arg("c") );

		cl.def( pybind11::init<unsigned char>(), pybind11::arg("c") );

		cl.def( pybind11::init<short>(), pybind11::arg("c") );

		cl.def( pybind11::init<unsigned short>(), pybind11::arg("c") );

		cl.def( pybind11::init<int>(), pybind11::arg("c") );

		cl.def( pybind11::init<unsigned int>(), pybind11::arg("c") );

		cl.def( pybind11::init<long>(), pybind11::arg("c") );

		cl.def( pybind11::init<unsigned long>(), pybind11::arg("c") );

		cl.def( pybind11::init<long long>(), pybind11::arg("c") );

		cl.def( pybind11::init<unsigned long long>(), pybind11::arg("c") );

		cl.def( pybind11::init<wchar_t>(), pybind11::arg("c") );

		cl.def( pybind11::init<const class wxUniCharRef &>(), pybind11::arg("c") );

		cl.def( pybind11::init( [](wxUniChar const &o){ return new wxUniChar(o); } ) );
		cl.def("GetValue", (unsigned int (wxUniChar::*)() const) &wxUniChar::GetValue, "C++: wxUniChar::GetValue() const --> unsigned int");
		cl.def("IsAscii", (bool (wxUniChar::*)() const) &wxUniChar::IsAscii, "C++: wxUniChar::IsAscii() const --> bool");
		cl.def("GetAsChar", (bool (wxUniChar::*)(char *) const) &wxUniChar::GetAsChar, "C++: wxUniChar::GetAsChar(char *) const --> bool", pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(const class wxUniChar &)) &wxUniChar::operator=, "C++: wxUniChar::operator=(const class wxUniChar &) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(const class wxUniCharRef &)) &wxUniChar::operator=, "C++: wxUniChar::operator=(const class wxUniCharRef &) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(char)) &wxUniChar::operator=, "C++: wxUniChar::operator=(char) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(unsigned char)) &wxUniChar::operator=, "C++: wxUniChar::operator=(unsigned char) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(short)) &wxUniChar::operator=, "C++: wxUniChar::operator=(short) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(unsigned short)) &wxUniChar::operator=, "C++: wxUniChar::operator=(unsigned short) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(int)) &wxUniChar::operator=, "C++: wxUniChar::operator=(int) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(unsigned int)) &wxUniChar::operator=, "C++: wxUniChar::operator=(unsigned int) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(long)) &wxUniChar::operator=, "C++: wxUniChar::operator=(long) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(unsigned long)) &wxUniChar::operator=, "C++: wxUniChar::operator=(unsigned long) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(long long)) &wxUniChar::operator=, "C++: wxUniChar::operator=(long long) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(unsigned long long)) &wxUniChar::operator=, "C++: wxUniChar::operator=(unsigned long long) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniChar & (wxUniChar::*)(wchar_t)) &wxUniChar::operator=, "C++: wxUniChar::operator=(wchar_t) --> class wxUniChar &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(const class wxUniChar &) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(const class wxUniChar &) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(char) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(char) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(unsigned char) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(unsigned char) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(short) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(short) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(unsigned short) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(unsigned short) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(int) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(int) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(unsigned int) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(unsigned int) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(long) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(long) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(unsigned long) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(unsigned long) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(long long) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(long long) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(unsigned long long) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(unsigned long long) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniChar::*)(wchar_t) const) &wxUniChar::operator==, "C++: wxUniChar::operator==(wchar_t) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(const class wxUniChar &) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(const class wxUniChar &) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(char) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(char) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(unsigned char) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(unsigned char) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(short) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(short) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(unsigned short) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(unsigned short) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(int) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(int) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(unsigned int) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(unsigned int) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(long) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(long) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(unsigned long) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(unsigned long) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(long long) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(long long) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(unsigned long long) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(unsigned long long) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniChar::*)(wchar_t) const) &wxUniChar::operator!=, "C++: wxUniChar::operator!=(wchar_t) const --> bool", pybind11::arg("c"));
		cl.def("__sub__", (int (wxUniChar::*)(const class wxUniChar &) const) &wxUniChar::operator-, "C++: wxUniChar::operator-(const class wxUniChar &) const --> int", pybind11::arg("c"));
		cl.def("__sub__", (int (wxUniChar::*)(char) const) &wxUniChar::operator-, "C++: wxUniChar::operator-(char) const --> int", pybind11::arg("c"));
		cl.def("__sub__", (int (wxUniChar::*)(unsigned char) const) &wxUniChar::operator-, "C++: wxUniChar::operator-(unsigned char) const --> int", pybind11::arg("c"));
		cl.def("__sub__", (int (wxUniChar::*)(wchar_t) const) &wxUniChar::operator-, "C++: wxUniChar::operator-(wchar_t) const --> int", pybind11::arg("c"));
	}
	{ // wxUniCharRef file: line:182
		pybind11::class_<wxUniCharRef, std::shared_ptr<wxUniCharRef>> cl(M(""), "wxUniCharRef", "");
		cl.def( pybind11::init( [](wxUniCharRef const &o){ return new wxUniCharRef(o); } ) );
		cl.def("GetValue", (unsigned int (wxUniCharRef::*)() const) &wxUniCharRef::GetValue, "C++: wxUniCharRef::GetValue() const --> unsigned int");
		cl.def("IsAscii", (bool (wxUniCharRef::*)() const) &wxUniCharRef::IsAscii, "C++: wxUniCharRef::IsAscii() const --> bool");
		cl.def("GetAsChar", (bool (wxUniCharRef::*)(char *) const) &wxUniCharRef::GetAsChar, "C++: wxUniCharRef::GetAsChar(char *) const --> bool", pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(const class wxUniChar &)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(const class wxUniChar &) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(const class wxUniCharRef &)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(const class wxUniCharRef &) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(char)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(char) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(unsigned char)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(unsigned char) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(short)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(short) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(unsigned short)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(unsigned short) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(int)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(int) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(unsigned int)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(unsigned int) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(long)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(long) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(unsigned long)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(unsigned long) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(long long)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(long long) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(unsigned long long)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(unsigned long long) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class wxUniCharRef & (wxUniCharRef::*)(wchar_t)) &wxUniCharRef::operator=, "C++: wxUniCharRef::operator=(wchar_t) --> class wxUniCharRef &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(const class wxUniCharRef &) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(const class wxUniCharRef &) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(const class wxUniChar &) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(const class wxUniChar &) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(char) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(char) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(unsigned char) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(unsigned char) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(short) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(short) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(unsigned short) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(unsigned short) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(int) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(int) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(unsigned int) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(unsigned int) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(long) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(long) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(unsigned long) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(unsigned long) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(long long) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(long long) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(unsigned long long) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(unsigned long long) const --> bool", pybind11::arg("c"));
		cl.def("__eq__", (bool (wxUniCharRef::*)(wchar_t) const) &wxUniCharRef::operator==, "C++: wxUniCharRef::operator==(wchar_t) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(const class wxUniCharRef &) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(const class wxUniCharRef &) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(const class wxUniChar &) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(const class wxUniChar &) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(char) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(char) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(unsigned char) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(unsigned char) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(short) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(short) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(unsigned short) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(unsigned short) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(int) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(int) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(unsigned int) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(unsigned int) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(long) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(long) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(unsigned long) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(unsigned long) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(long long) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(long long) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(unsigned long long) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(unsigned long long) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (wxUniCharRef::*)(wchar_t) const) &wxUniCharRef::operator!=, "C++: wxUniCharRef::operator!=(wchar_t) const --> bool", pybind11::arg("c"));
		cl.def("__sub__", (int (wxUniCharRef::*)(const class wxUniCharRef &) const) &wxUniCharRef::operator-, "C++: wxUniCharRef::operator-(const class wxUniCharRef &) const --> int", pybind11::arg("c"));
		cl.def("__sub__", (int (wxUniCharRef::*)(const class wxUniChar &) const) &wxUniCharRef::operator-, "C++: wxUniCharRef::operator-(const class wxUniChar &) const --> int", pybind11::arg("c"));
		cl.def("__sub__", (int (wxUniCharRef::*)(char) const) &wxUniCharRef::operator-, "C++: wxUniCharRef::operator-(char) const --> int", pybind11::arg("c"));
		cl.def("__sub__", (int (wxUniCharRef::*)(unsigned char) const) &wxUniCharRef::operator-, "C++: wxUniCharRef::operator-(unsigned char) const --> int", pybind11::arg("c"));
		cl.def("__sub__", (int (wxUniCharRef::*)(wchar_t) const) &wxUniCharRef::operator-, "C++: wxUniCharRef::operator-(wchar_t) const --> int", pybind11::arg("c"));
	}
}
