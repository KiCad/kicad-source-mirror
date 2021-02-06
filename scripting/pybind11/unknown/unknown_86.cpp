#include <functional> // std::function
#include <inspectable.h> // INSPECTABLE
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/dc.h> // wxDC
#include <wx/propgrid/property.h> // wxPGChoiceEntry
#include <wx/propgrid/property.h> // wxPGChoices
#include <wx/propgrid/property.h> // wxPGChoicesData

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// PROPERTY_BASE file: line:170
struct PyCallBack_PROPERTY_BASE : public PROPERTY_BASE {
	using PROPERTY_BASE::PROPERTY_BASE;

	const class wxPGChoices & Choices() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PROPERTY_BASE *>(this), "Choices");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxPGChoices &>::value) {
				static pybind11::detail::override_caster_t<const class wxPGChoices &> caster;
				return pybind11::detail::cast_ref<const class wxPGChoices &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxPGChoices &>(std::move(o));
		}
		return PROPERTY_BASE::Choices();
	}
	void SetChoices(const class wxPGChoices & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PROPERTY_BASE *>(this), "SetChoices");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return PROPERTY_BASE::SetChoices(a0);
	}
	bool HasChoices() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PROPERTY_BASE *>(this), "HasChoices");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return PROPERTY_BASE::HasChoices();
	}
	unsigned long OwnerHash() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PROPERTY_BASE *>(this), "OwnerHash");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"PROPERTY_BASE::OwnerHash\"");
	}
	unsigned long BaseHash() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PROPERTY_BASE *>(this), "BaseHash");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"PROPERTY_BASE::BaseHash\"");
	}
	unsigned long TypeHash() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PROPERTY_BASE *>(this), "TypeHash");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"PROPERTY_BASE::TypeHash\"");
	}
	bool IsReadOnly() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PROPERTY_BASE *>(this), "IsReadOnly");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"PROPERTY_BASE::IsReadOnly\"");
	}
	void setter(void * a0, class wxAny & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PROPERTY_BASE *>(this), "setter");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"PROPERTY_BASE::setter\"");
	}
	class wxAny getter(void * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PROPERTY_BASE *>(this), "getter");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxAny>::value) {
				static pybind11::detail::override_caster_t<class wxAny> caster;
				return pybind11::detail::cast_ref<class wxAny>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxAny>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"PROPERTY_BASE::getter\"");
	}
};

// TYPE_CAST_BASE file: line:461
struct PyCallBack_TYPE_CAST_BASE : public TYPE_CAST_BASE {
	using TYPE_CAST_BASE::TYPE_CAST_BASE;

	void * operator()(void * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const TYPE_CAST_BASE *>(this), "__call__");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"TYPE_CAST_BASE::__call__\"");
	}
	const void * operator()(const void * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const TYPE_CAST_BASE *>(this), "__call__");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<const void *>::value) {
				static pybind11::detail::override_caster_t<const void *> caster;
				return pybind11::detail::cast_ref<const void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const void *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"TYPE_CAST_BASE::__call__\"");
	}
	unsigned long BaseHash() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const TYPE_CAST_BASE *>(this), "BaseHash");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"TYPE_CAST_BASE::BaseHash\"");
	}
	unsigned long DerivedHash() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const TYPE_CAST_BASE *>(this), "DerivedHash");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"TYPE_CAST_BASE::DerivedHash\"");
	}
};

void bind_unknown_unknown_86(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// PROPERTY_DISPLAY file: line:48
	pybind11::enum_<PROPERTY_DISPLAY>(M(""), "PROPERTY_DISPLAY", pybind11::arithmetic(), "")
		.value("DEFAULT", DEFAULT)
		.value("DISTANCE", DISTANCE)
		.value("DEGREE", DEGREE)
		.value("DECIDEGREE", DECIDEGREE)
		.export_values();

;

	{ // PROPERTY_BASE file: line:170
		pybind11::class_<PROPERTY_BASE, std::shared_ptr<PROPERTY_BASE>, PyCallBack_PROPERTY_BASE> cl(M(""), "PROPERTY_BASE", "");
		cl.def( pybind11::init( [](const class wxString & a0){ return new PyCallBack_PROPERTY_BASE(a0); } ), "doc");
		cl.def( pybind11::init<const class wxString &, enum PROPERTY_DISPLAY>(), pybind11::arg("aName"), pybind11::arg("aDisplay") );

		cl.def(pybind11::init<PyCallBack_PROPERTY_BASE const &>());
		cl.def("Name", (const class wxString & (PROPERTY_BASE::*)() const) &PROPERTY_BASE::Name, "C++: PROPERTY_BASE::Name() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("Choices", (const class wxPGChoices & (PROPERTY_BASE::*)() const) &PROPERTY_BASE::Choices, "Return a limited set of possible values (e.g. enum). Check with HasChoices() if a particular\n PROPERTY provides such set.\n\nC++: PROPERTY_BASE::Choices() const --> const class wxPGChoices &", pybind11::return_value_policy::automatic);
		cl.def("SetChoices", (void (PROPERTY_BASE::*)(const class wxPGChoices &)) &PROPERTY_BASE::SetChoices, "Set the possible values for for the property.\n\nC++: PROPERTY_BASE::SetChoices(const class wxPGChoices &) --> void", pybind11::arg("aChoices"));
		cl.def("HasChoices", (bool (PROPERTY_BASE::*)() const) &PROPERTY_BASE::HasChoices, "Return true if this PROPERTY has a limited set of possible values.\n \n\n PROPERTY_BASE::Choices()\n\nC++: PROPERTY_BASE::HasChoices() const --> bool");
		cl.def("Available", (bool (PROPERTY_BASE::*)(class INSPECTABLE *) const) &PROPERTY_BASE::Available, "Return true if aObject offers this PROPERTY.\n\nC++: PROPERTY_BASE::Available(class INSPECTABLE *) const --> bool", pybind11::arg("aObject"));
		cl.def("OwnerHash", (unsigned long (PROPERTY_BASE::*)() const) &PROPERTY_BASE::OwnerHash, "Return type-id of the Owner class.\n\nC++: PROPERTY_BASE::OwnerHash() const --> unsigned long");
		cl.def("BaseHash", (unsigned long (PROPERTY_BASE::*)() const) &PROPERTY_BASE::BaseHash, "Return type-id of the Base class.\n\nC++: PROPERTY_BASE::BaseHash() const --> unsigned long");
		cl.def("TypeHash", (unsigned long (PROPERTY_BASE::*)() const) &PROPERTY_BASE::TypeHash, "Return type-id of the property type.\n\nC++: PROPERTY_BASE::TypeHash() const --> unsigned long");
		cl.def("IsReadOnly", (bool (PROPERTY_BASE::*)() const) &PROPERTY_BASE::IsReadOnly, "C++: PROPERTY_BASE::IsReadOnly() const --> bool");
		cl.def("GetDisplay", (enum PROPERTY_DISPLAY (PROPERTY_BASE::*)() const) &PROPERTY_BASE::GetDisplay, "C++: PROPERTY_BASE::GetDisplay() const --> enum PROPERTY_DISPLAY");
		cl.def("assign", (class PROPERTY_BASE & (PROPERTY_BASE::*)(const class PROPERTY_BASE &)) &PROPERTY_BASE::operator=, "C++: PROPERTY_BASE::operator=(const class PROPERTY_BASE &) --> class PROPERTY_BASE &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // TYPE_CAST_BASE file: line:461
		pybind11::class_<TYPE_CAST_BASE, std::shared_ptr<TYPE_CAST_BASE>, PyCallBack_TYPE_CAST_BASE> cl(M(""), "TYPE_CAST_BASE", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_TYPE_CAST_BASE(); } ) );
		cl.def("__call__", (void * (TYPE_CAST_BASE::*)(void *) const) &TYPE_CAST_BASE::operator(), "C++: TYPE_CAST_BASE::operator()(void *) const --> void *", pybind11::return_value_policy::automatic, pybind11::arg("aPointer"));
		cl.def("__call__", (const void * (TYPE_CAST_BASE::*)(const void *) const) &TYPE_CAST_BASE::operator(), "C++: TYPE_CAST_BASE::operator()(const void *) const --> const void *", pybind11::return_value_policy::automatic, pybind11::arg("aPointer"));
		cl.def("BaseHash", (unsigned long (TYPE_CAST_BASE::*)() const) &TYPE_CAST_BASE::BaseHash, "C++: TYPE_CAST_BASE::BaseHash() const --> unsigned long");
		cl.def("DerivedHash", (unsigned long (TYPE_CAST_BASE::*)() const) &TYPE_CAST_BASE::DerivedHash, "C++: TYPE_CAST_BASE::DerivedHash() const --> unsigned long");
		cl.def("assign", (class TYPE_CAST_BASE & (TYPE_CAST_BASE::*)(const class TYPE_CAST_BASE &)) &TYPE_CAST_BASE::operator=, "C++: TYPE_CAST_BASE::operator=(const class TYPE_CAST_BASE &) --> class TYPE_CAST_BASE &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
