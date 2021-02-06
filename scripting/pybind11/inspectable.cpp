#include <functional> // std::function
#include <inspectable.h> // INSPECTABLE
#include <sstream> // __str__
#include <wx/propgrid/property.h> // wxPGChoices

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_inspectable(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // INSPECTABLE file:inspectable.h line:33
		pybind11::class_<INSPECTABLE, std::shared_ptr<INSPECTABLE>> cl(M(""), "INSPECTABLE", "Class that other classes need to inherit from, in order to be inspectable.");
		cl.def( pybind11::init( [](){ return new INSPECTABLE(); } ) );
		cl.def( pybind11::init( [](INSPECTABLE const &o){ return new INSPECTABLE(o); } ) );
		cl.def("Set", (bool (INSPECTABLE::*)(class PROPERTY_BASE *, class wxAny &)) &INSPECTABLE::Set, "C++: INSPECTABLE::Set(class PROPERTY_BASE *, class wxAny &) --> bool", pybind11::arg("aProperty"), pybind11::arg("aValue"));
		cl.def("Get", (class wxAny (INSPECTABLE::*)(class PROPERTY_BASE *)) &INSPECTABLE::Get, "C++: INSPECTABLE::Get(class PROPERTY_BASE *) --> class wxAny", pybind11::arg("aProperty"));
		cl.def("assign", (class INSPECTABLE & (INSPECTABLE::*)(const class INSPECTABLE &)) &INSPECTABLE::operator=, "C++: INSPECTABLE::operator=(const class INSPECTABLE &) --> class INSPECTABLE &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
