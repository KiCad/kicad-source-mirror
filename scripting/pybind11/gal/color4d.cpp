#include <gal/color4d.h> // StructColors
#include <gal/color4d.h> // colorRefs
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

void bind_gal_color4d(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // StructColors file:gal/color4d.h line:79
		pybind11::class_<StructColors, std::shared_ptr<StructColors>> cl(M(""), "StructColors", "");
		cl.def( pybind11::init( [](){ return new StructColors(); } ) );
		cl.def( pybind11::init( [](StructColors const &o){ return new StructColors(o); } ) );
		cl.def_readwrite("m_Blue", &StructColors::m_Blue);
		cl.def_readwrite("m_Green", &StructColors::m_Green);
		cl.def_readwrite("m_Red", &StructColors::m_Red);
		cl.def_readwrite("m_Numcolor", &StructColors::m_Numcolor);
		cl.def_readwrite("m_ColorName", &StructColors::m_ColorName);
		cl.def_readwrite("m_LightColor", &StructColors::m_LightColor);
		cl.def("assign", (struct StructColors & (StructColors::*)(const struct StructColors &)) &StructColors::operator=, "C++: StructColors::operator=(const struct StructColors &) --> struct StructColors &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// colorRefs() file:gal/color4d.h line:90
	M("").def("colorRefs", (const struct StructColors * (*)()) &colorRefs, "Global list of legacy color names, still used all over the place for constructing COLOR4D's\n\nC++: colorRefs() --> const struct StructColors *", pybind11::return_value_policy::automatic);

}
