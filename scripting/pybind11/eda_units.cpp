#include <eda_units.h> // EDA_DATA_TYPE
#include <eda_units.h> // EDA_UNITS

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_eda_units(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// EDA_DATA_TYPE file:eda_units.h line:31
	pybind11::enum_<EDA_DATA_TYPE>(M(""), "EDA_DATA_TYPE", "The type of unit.")
		.value("DISTANCE", EDA_DATA_TYPE::DISTANCE)
		.value("AREA", EDA_DATA_TYPE::AREA)
		.value("VOLUME", EDA_DATA_TYPE::VOLUME);

;

	// EDA_UNITS file:eda_units.h line:38
	pybind11::enum_<EDA_UNITS>(M(""), "EDA_UNITS", "")
		.value("INCHES", EDA_UNITS::INCHES)
		.value("MILLIMETRES", EDA_UNITS::MILLIMETRES)
		.value("UNSCALED", EDA_UNITS::UNSCALED)
		.value("DEGREES", EDA_UNITS::DEGREES)
		.value("PERCENT", EDA_UNITS::PERCENT)
		.value("MILS", EDA_UNITS::MILS);

;

}
