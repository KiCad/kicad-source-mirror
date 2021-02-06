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

void bind_unknown_unknown_26(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPrivate::VariableSetterBase file: line:392
		pybind11::class_<wxPrivate::VariableSetterBase, std::shared_ptr<wxPrivate::VariableSetterBase>, wxScopeGuardImplBase> cl(M("wxPrivate"), "VariableSetterBase", "");
		cl.def( pybind11::init( [](){ return new wxPrivate::VariableSetterBase(); } ) );
		cl.def( pybind11::init( [](wxPrivate::VariableSetterBase const &o){ return new wxPrivate::VariableSetterBase(o); } ) );
	}
}
