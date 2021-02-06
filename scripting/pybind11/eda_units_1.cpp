#include <eda_units.h> // EDA_UNITS
#include <eda_units.h> // EDA_UNIT_UTILS::IsImperialUnit
#include <eda_units.h> // EDA_UNIT_UTILS::IsMetricUnit

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_eda_units_1(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// EDA_UNIT_UTILS::IsImperialUnit(enum EDA_UNITS) file:eda_units.h line:50
	M("EDA_UNIT_UTILS").def("IsImperialUnit", (bool (*)(enum EDA_UNITS)) &EDA_UNIT_UTILS::IsImperialUnit, "C++: EDA_UNIT_UTILS::IsImperialUnit(enum EDA_UNITS) --> bool", pybind11::arg("aUnit"));

	// EDA_UNIT_UTILS::IsMetricUnit(enum EDA_UNITS) file:eda_units.h line:52
	M("EDA_UNIT_UTILS").def("IsMetricUnit", (bool (*)(enum EDA_UNITS)) &EDA_UNIT_UTILS::IsMetricUnit, "C++: EDA_UNIT_UTILS::IsMetricUnit(enum EDA_UNITS) --> bool", pybind11::arg("aUnit"));

}
