#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <time.h> // tm

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxAnyValueTypeImplBase file: line:284
struct PyCallBack_wxAnyValueTypeImplBase_const_wchar_t_*_t : public wxAnyValueTypeImplBase<const wchar_t *> {
	using wxAnyValueTypeImplBase<const wchar_t *>::wxAnyValueTypeImplBase;

	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<const wchar_t *> *>(this), "DeleteValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplBase::DeleteValue(a0);
	}
	void CopyBuffer(const union wxAnyValueBuffer & a0, union wxAnyValueBuffer & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<const wchar_t *> *>(this), "CopyBuffer");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplBase::CopyBuffer(a0, a1);
	}
	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<const wchar_t *> *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyValueType::IsSameType\"");
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<const wchar_t *> *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyValueType::ConvertValue\"");
	}
};

// wxAnyValueTypeImplBase file: line:284
struct PyCallBack_wxAnyValueTypeImplBase_double_t : public wxAnyValueTypeImplBase<double> {
	using wxAnyValueTypeImplBase<double>::wxAnyValueTypeImplBase;

	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<double> *>(this), "DeleteValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplBase::DeleteValue(a0);
	}
	void CopyBuffer(const union wxAnyValueBuffer & a0, union wxAnyValueBuffer & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<double> *>(this), "CopyBuffer");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplBase::CopyBuffer(a0, a1);
	}
	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<double> *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyValueType::IsSameType\"");
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<double> *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyValueType::ConvertValue\"");
	}
};

// wxAnyValueTypeImplBase file: line:284
struct PyCallBack_wxAnyValueTypeImplBase_wxVariantData_*_t : public wxAnyValueTypeImplBase<wxVariantData *> {
	using wxAnyValueTypeImplBase<wxVariantData *>::wxAnyValueTypeImplBase;

	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<wxVariantData *> *>(this), "DeleteValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplBase::DeleteValue(a0);
	}
	void CopyBuffer(const union wxAnyValueBuffer & a0, union wxAnyValueBuffer & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<wxVariantData *> *>(this), "CopyBuffer");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplBase::CopyBuffer(a0, a1);
	}
	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<wxVariantData *> *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyValueType::IsSameType\"");
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<wxVariantData *> *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyValueType::ConvertValue\"");
	}
};

// wxAnyValueTypeImplInt file: line:414
struct PyCallBack_wxAnyValueTypeImplInt : public wxAnyValueTypeImplInt {
	using wxAnyValueTypeImplInt::wxAnyValueTypeImplInt;

	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplInt *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplInt::IsSameType(a0);
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplInt *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplInt::ConvertValue(a0, a1, a2);
	}
	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplInt *>(this), "DeleteValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplBase::DeleteValue(a0);
	}
	void CopyBuffer(const union wxAnyValueBuffer & a0, union wxAnyValueBuffer & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplInt *>(this), "CopyBuffer");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplBase::CopyBuffer(a0, a1);
	}
};

// wxAnyValueTypeImplUint file: line:429
struct PyCallBack_wxAnyValueTypeImplUint : public wxAnyValueTypeImplUint {
	using wxAnyValueTypeImplUint::wxAnyValueTypeImplUint;

	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplUint *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplUint::IsSameType(a0);
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplUint *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplUint::ConvertValue(a0, a1, a2);
	}
	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplUint *>(this), "DeleteValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplBase::DeleteValue(a0);
	}
	void CopyBuffer(const union wxAnyValueBuffer & a0, union wxAnyValueBuffer & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplUint *>(this), "CopyBuffer");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplBase::CopyBuffer(a0, a1);
	}
};

void bind_unknown_unknown_47(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxAnyValueTypeImplBase file: line:284
		pybind11::class_<wxAnyValueTypeImplBase<const wchar_t *>, std::shared_ptr<wxAnyValueTypeImplBase<const wchar_t *>>, PyCallBack_wxAnyValueTypeImplBase_const_wchar_t_*_t, wxAnyValueType> cl(M(""), "wxAnyValueTypeImplBase_const_wchar_t_*_t", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxAnyValueTypeImplBase_const_wchar_t_*_t(); } ) );
		cl.def(pybind11::init<PyCallBack_wxAnyValueTypeImplBase_const_wchar_t_*_t const &>());
		cl.def("DeleteValue", (void (wxAnyValueTypeImplBase<const wchar_t *>::*)(union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<const wchar_t *>::DeleteValue, "C++: wxAnyValueTypeImplBase<const wchar_t *>::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueTypeImplBase<const wchar_t *>::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<const wchar_t *>::CopyBuffer, "C++: wxAnyValueTypeImplBase<const wchar_t *>::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def_static("SetValue", (void (*)(const wchar_t *const &, union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<const wchar_t *>::SetValue, "C++: wxAnyValueTypeImplBase<const wchar_t *>::SetValue(const wchar_t *const &, union wxAnyValueBuffer &) --> void", pybind11::arg("value"), pybind11::arg("buf"));
		cl.def_static("GetValue", (const wchar_t *const & (*)(const union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<const wchar_t *>::GetValue, "C++: wxAnyValueTypeImplBase<const wchar_t *>::GetValue(const union wxAnyValueBuffer &) --> const wchar_t *const &", pybind11::return_value_policy::automatic, pybind11::arg("buf"));
		cl.def("assign", (class wxAnyValueTypeImplBase<const wchar_t *> & (wxAnyValueTypeImplBase<const wchar_t *>::*)(const class wxAnyValueTypeImplBase<const wchar_t *> &)) &wxAnyValueTypeImplBase<const wchar_t *>::operator=, "C++: wxAnyValueTypeImplBase<const wchar_t *>::operator=(const class wxAnyValueTypeImplBase<const wchar_t *> &) --> class wxAnyValueTypeImplBase<const wchar_t *> &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("IsSameType", (bool (wxAnyValueType::*)(const class wxAnyValueType *) const) &wxAnyValueType::IsSameType, "This function is used for internal type matching.\n\nC++: wxAnyValueType::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def("DeleteValue", (void (wxAnyValueType::*)(union wxAnyValueBuffer &) const) &wxAnyValueType::DeleteValue, "This function is called every time the data in wxAny\n        buffer needs to be freed.\n\nC++: wxAnyValueType::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueType::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueType::CopyBuffer, "Implement this for buffer-to-buffer copy.\n\n        \n\n            This is the source data buffer.\n\n        \n\n            This is the destination data buffer that is in either\n            uninitialized or freed state.\n\nC++: wxAnyValueType::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def("ConvertValue", (bool (wxAnyValueType::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueType::ConvertValue, "Convert value into buffer of different type. Return false if\n        not possible.\n\nC++: wxAnyValueType::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueType & (wxAnyValueType::*)(const class wxAnyValueType &)) &wxAnyValueType::operator=, "C++: wxAnyValueType::operator=(const class wxAnyValueType &) --> class wxAnyValueType &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplBase file: line:284
		pybind11::class_<wxAnyValueTypeImplBase<double>, std::shared_ptr<wxAnyValueTypeImplBase<double>>, PyCallBack_wxAnyValueTypeImplBase_double_t, wxAnyValueType> cl(M(""), "wxAnyValueTypeImplBase_double_t", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxAnyValueTypeImplBase_double_t(); } ) );
		cl.def(pybind11::init<PyCallBack_wxAnyValueTypeImplBase_double_t const &>());
		cl.def("DeleteValue", (void (wxAnyValueTypeImplBase<double>::*)(union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<double>::DeleteValue, "C++: wxAnyValueTypeImplBase<double>::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueTypeImplBase<double>::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<double>::CopyBuffer, "C++: wxAnyValueTypeImplBase<double>::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def_static("SetValue", (void (*)(const double &, union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<double>::SetValue, "C++: wxAnyValueTypeImplBase<double>::SetValue(const double &, union wxAnyValueBuffer &) --> void", pybind11::arg("value"), pybind11::arg("buf"));
		cl.def_static("GetValue", (const double & (*)(const union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<double>::GetValue, "C++: wxAnyValueTypeImplBase<double>::GetValue(const union wxAnyValueBuffer &) --> const double &", pybind11::return_value_policy::automatic, pybind11::arg("buf"));
		cl.def("assign", (class wxAnyValueTypeImplBase<double> & (wxAnyValueTypeImplBase<double>::*)(const class wxAnyValueTypeImplBase<double> &)) &wxAnyValueTypeImplBase<double>::operator=, "C++: wxAnyValueTypeImplBase<double>::operator=(const class wxAnyValueTypeImplBase<double> &) --> class wxAnyValueTypeImplBase<double> &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("IsSameType", (bool (wxAnyValueType::*)(const class wxAnyValueType *) const) &wxAnyValueType::IsSameType, "This function is used for internal type matching.\n\nC++: wxAnyValueType::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def("DeleteValue", (void (wxAnyValueType::*)(union wxAnyValueBuffer &) const) &wxAnyValueType::DeleteValue, "This function is called every time the data in wxAny\n        buffer needs to be freed.\n\nC++: wxAnyValueType::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueType::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueType::CopyBuffer, "Implement this for buffer-to-buffer copy.\n\n        \n\n            This is the source data buffer.\n\n        \n\n            This is the destination data buffer that is in either\n            uninitialized or freed state.\n\nC++: wxAnyValueType::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def("ConvertValue", (bool (wxAnyValueType::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueType::ConvertValue, "Convert value into buffer of different type. Return false if\n        not possible.\n\nC++: wxAnyValueType::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueType & (wxAnyValueType::*)(const class wxAnyValueType &)) &wxAnyValueType::operator=, "C++: wxAnyValueType::operator=(const class wxAnyValueType &) --> class wxAnyValueType &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplBase file: line:284
		pybind11::class_<wxAnyValueTypeImplBase<wxVariantData *>, std::shared_ptr<wxAnyValueTypeImplBase<wxVariantData *>>, PyCallBack_wxAnyValueTypeImplBase_wxVariantData_*_t, wxAnyValueType> cl(M(""), "wxAnyValueTypeImplBase_wxVariantData_*_t", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxAnyValueTypeImplBase_wxVariantData_*_t(); } ) );
		cl.def(pybind11::init<PyCallBack_wxAnyValueTypeImplBase_wxVariantData_*_t const &>());
		cl.def("DeleteValue", (void (wxAnyValueTypeImplBase<wxVariantData *>::*)(union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<wxVariantData *>::DeleteValue, "C++: wxAnyValueTypeImplBase<wxVariantData *>::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueTypeImplBase<wxVariantData *>::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<wxVariantData *>::CopyBuffer, "C++: wxAnyValueTypeImplBase<wxVariantData *>::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def_static("SetValue", (void (*)(class wxVariantData *const &, union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<wxVariantData *>::SetValue, "C++: wxAnyValueTypeImplBase<wxVariantData *>::SetValue(class wxVariantData *const &, union wxAnyValueBuffer &) --> void", pybind11::arg("value"), pybind11::arg("buf"));
		cl.def_static("GetValue", (class wxVariantData *const & (*)(const union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<wxVariantData *>::GetValue, "C++: wxAnyValueTypeImplBase<wxVariantData *>::GetValue(const union wxAnyValueBuffer &) --> class wxVariantData *const &", pybind11::return_value_policy::automatic, pybind11::arg("buf"));
		cl.def("assign", (class wxAnyValueTypeImplBase<class wxVariantData *> & (wxAnyValueTypeImplBase<wxVariantData *>::*)(const class wxAnyValueTypeImplBase<class wxVariantData *> &)) &wxAnyValueTypeImplBase<wxVariantData *>::operator=, "C++: wxAnyValueTypeImplBase<wxVariantData *>::operator=(const class wxAnyValueTypeImplBase<class wxVariantData *> &) --> class wxAnyValueTypeImplBase<class wxVariantData *> &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("IsSameType", (bool (wxAnyValueType::*)(const class wxAnyValueType *) const) &wxAnyValueType::IsSameType, "This function is used for internal type matching.\n\nC++: wxAnyValueType::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def("DeleteValue", (void (wxAnyValueType::*)(union wxAnyValueBuffer &) const) &wxAnyValueType::DeleteValue, "This function is called every time the data in wxAny\n        buffer needs to be freed.\n\nC++: wxAnyValueType::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueType::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueType::CopyBuffer, "Implement this for buffer-to-buffer copy.\n\n        \n\n            This is the source data buffer.\n\n        \n\n            This is the destination data buffer that is in either\n            uninitialized or freed state.\n\nC++: wxAnyValueType::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def("ConvertValue", (bool (wxAnyValueType::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueType::ConvertValue, "Convert value into buffer of different type. Return false if\n        not possible.\n\nC++: wxAnyValueType::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueType & (wxAnyValueType::*)(const class wxAnyValueType &)) &wxAnyValueType::operator=, "C++: wxAnyValueType::operator=(const class wxAnyValueType &) --> class wxAnyValueType &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplInt file: line:414
		pybind11::class_<wxAnyValueTypeImplInt, std::shared_ptr<wxAnyValueTypeImplInt>, PyCallBack_wxAnyValueTypeImplInt, wxAnyValueTypeImplBase<long long>> cl(M(""), "wxAnyValueTypeImplInt", "");
		cl.def( pybind11::init( [](){ return new wxAnyValueTypeImplInt(); }, [](){ return new PyCallBack_wxAnyValueTypeImplInt(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxAnyValueTypeImplInt const &o){ return new PyCallBack_wxAnyValueTypeImplInt(o); } ) );
		cl.def( pybind11::init( [](wxAnyValueTypeImplInt const &o){ return new wxAnyValueTypeImplInt(o); } ) );
		cl.def_static("IsSameClass", (bool (*)(const class wxAnyValueType *)) &wxAnyValueTypeImplInt::IsSameClass, "C++: wxAnyValueTypeImplInt::IsSameClass(const class wxAnyValueType *) --> bool", pybind11::arg("otherType"));
		cl.def("IsSameType", (bool (wxAnyValueTypeImplInt::*)(const class wxAnyValueType *) const) &wxAnyValueTypeImplInt::IsSameType, "C++: wxAnyValueTypeImplInt::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def_static("GetInstance", (class wxAnyValueType * (*)()) &wxAnyValueTypeImplInt::GetInstance, "C++: wxAnyValueTypeImplInt::GetInstance() --> class wxAnyValueType *", pybind11::return_value_policy::automatic);
		cl.def("ConvertValue", (bool (wxAnyValueTypeImplInt::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplInt::ConvertValue, "C++: wxAnyValueTypeImplInt::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueTypeImplInt & (wxAnyValueTypeImplInt::*)(const class wxAnyValueTypeImplInt &)) &wxAnyValueTypeImplInt::operator=, "C++: wxAnyValueTypeImplInt::operator=(const class wxAnyValueTypeImplInt &) --> class wxAnyValueTypeImplInt &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplUint file: line:429
		pybind11::class_<wxAnyValueTypeImplUint, std::shared_ptr<wxAnyValueTypeImplUint>, PyCallBack_wxAnyValueTypeImplUint, wxAnyValueTypeImplBase<unsigned long long>> cl(M(""), "wxAnyValueTypeImplUint", "");
		cl.def( pybind11::init( [](){ return new wxAnyValueTypeImplUint(); }, [](){ return new PyCallBack_wxAnyValueTypeImplUint(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxAnyValueTypeImplUint const &o){ return new PyCallBack_wxAnyValueTypeImplUint(o); } ) );
		cl.def( pybind11::init( [](wxAnyValueTypeImplUint const &o){ return new wxAnyValueTypeImplUint(o); } ) );
		cl.def_static("IsSameClass", (bool (*)(const class wxAnyValueType *)) &wxAnyValueTypeImplUint::IsSameClass, "C++: wxAnyValueTypeImplUint::IsSameClass(const class wxAnyValueType *) --> bool", pybind11::arg("otherType"));
		cl.def("IsSameType", (bool (wxAnyValueTypeImplUint::*)(const class wxAnyValueType *) const) &wxAnyValueTypeImplUint::IsSameType, "C++: wxAnyValueTypeImplUint::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def_static("GetInstance", (class wxAnyValueType * (*)()) &wxAnyValueTypeImplUint::GetInstance, "C++: wxAnyValueTypeImplUint::GetInstance() --> class wxAnyValueType *", pybind11::return_value_policy::automatic);
		cl.def("ConvertValue", (bool (wxAnyValueTypeImplUint::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplUint::ConvertValue, "C++: wxAnyValueTypeImplUint::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueTypeImplUint & (wxAnyValueTypeImplUint::*)(const class wxAnyValueTypeImplUint &)) &wxAnyValueTypeImplUint::operator=, "C++: wxAnyValueTypeImplUint::operator=(const class wxAnyValueTypeImplUint &) --> class wxAnyValueTypeImplUint &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxAnyConvertString(const class wxString &, class wxAnyValueType *, union wxAnyValueBuffer &) file: line:502
	M("").def("wxAnyConvertString", (bool (*)(const class wxString &, class wxAnyValueType *, union wxAnyValueBuffer &)) &wxAnyConvertString, "C++: wxAnyConvertString(const class wxString &, class wxAnyValueType *, union wxAnyValueBuffer &) --> bool", pybind11::arg("value"), pybind11::arg("dstType"), pybind11::arg("dst"));

}
