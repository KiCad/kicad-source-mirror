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

// wxAnyValueTypeImplBase file: line:284
struct PyCallBack_wxAnyValueTypeImplBase_long_long_t : public wxAnyValueTypeImplBase<long long> {
	using wxAnyValueTypeImplBase<long long>::wxAnyValueTypeImplBase;

	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<long long> *>(this), "DeleteValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<long long> *>(this), "CopyBuffer");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<long long> *>(this), "IsSameType");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<long long> *>(this), "ConvertValue");
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
struct PyCallBack_wxAnyValueTypeImplBase_unsigned_long_long_t : public wxAnyValueTypeImplBase<unsigned long long> {
	using wxAnyValueTypeImplBase<unsigned long long>::wxAnyValueTypeImplBase;

	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<unsigned long long> *>(this), "DeleteValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<unsigned long long> *>(this), "CopyBuffer");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<unsigned long long> *>(this), "IsSameType");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<unsigned long long> *>(this), "ConvertValue");
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
struct PyCallBack_wxAnyValueTypeImplBase_wxString_t : public wxAnyValueTypeImplBase<wxString> {
	using wxAnyValueTypeImplBase<wxString>::wxAnyValueTypeImplBase;

	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<wxString> *>(this), "DeleteValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<wxString> *>(this), "CopyBuffer");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<wxString> *>(this), "IsSameType");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<wxString> *>(this), "ConvertValue");
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
struct PyCallBack_wxAnyValueTypeImplBase_const_char_*_t : public wxAnyValueTypeImplBase<const char *> {
	using wxAnyValueTypeImplBase<const char *>::wxAnyValueTypeImplBase;

	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<const char *> *>(this), "DeleteValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<const char *> *>(this), "CopyBuffer");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<const char *> *>(this), "IsSameType");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplBase<const char *> *>(this), "ConvertValue");
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

void bind_unknown_unknown_46(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxAnyValueTypeScopedPtr file: line:131
		pybind11::class_<wxAnyValueTypeScopedPtr, std::shared_ptr<wxAnyValueTypeScopedPtr>> cl(M(""), "wxAnyValueTypeScopedPtr", "");
		cl.def( pybind11::init<class wxAnyValueType *>(), pybind11::arg("ptr") );

		cl.def("get", (class wxAnyValueType * (wxAnyValueTypeScopedPtr::*)() const) &wxAnyValueTypeScopedPtr::get, "C++: wxAnyValueTypeScopedPtr::get() const --> class wxAnyValueType *", pybind11::return_value_policy::automatic);
	}
	{ // wxAnyValueTypeImplBase file: line:284
		pybind11::class_<wxAnyValueTypeImplBase<long long>, std::shared_ptr<wxAnyValueTypeImplBase<long long>>, PyCallBack_wxAnyValueTypeImplBase_long_long_t, wxAnyValueType> cl(M(""), "wxAnyValueTypeImplBase_long_long_t", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxAnyValueTypeImplBase_long_long_t(); } ) );
		cl.def(pybind11::init<PyCallBack_wxAnyValueTypeImplBase_long_long_t const &>());
		cl.def("DeleteValue", (void (wxAnyValueTypeImplBase<long long>::*)(union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<long long>::DeleteValue, "C++: wxAnyValueTypeImplBase<long long>::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueTypeImplBase<long long>::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<long long>::CopyBuffer, "C++: wxAnyValueTypeImplBase<long long>::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def_static("SetValue", (void (*)(const long long &, union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<long long>::SetValue, "C++: wxAnyValueTypeImplBase<long long>::SetValue(const long long &, union wxAnyValueBuffer &) --> void", pybind11::arg("value"), pybind11::arg("buf"));
		cl.def_static("GetValue", (const long long & (*)(const union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<long long>::GetValue, "C++: wxAnyValueTypeImplBase<long long>::GetValue(const union wxAnyValueBuffer &) --> const long long &", pybind11::return_value_policy::automatic, pybind11::arg("buf"));
		cl.def("assign", (class wxAnyValueTypeImplBase<long long> & (wxAnyValueTypeImplBase<long long>::*)(const class wxAnyValueTypeImplBase<long long> &)) &wxAnyValueTypeImplBase<long long>::operator=, "C++: wxAnyValueTypeImplBase<long long>::operator=(const class wxAnyValueTypeImplBase<long long> &) --> class wxAnyValueTypeImplBase<long long> &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("IsSameType", (bool (wxAnyValueType::*)(const class wxAnyValueType *) const) &wxAnyValueType::IsSameType, "This function is used for internal type matching.\n\nC++: wxAnyValueType::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def("DeleteValue", (void (wxAnyValueType::*)(union wxAnyValueBuffer &) const) &wxAnyValueType::DeleteValue, "This function is called every time the data in wxAny\n        buffer needs to be freed.\n\nC++: wxAnyValueType::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueType::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueType::CopyBuffer, "Implement this for buffer-to-buffer copy.\n\n        \n\n            This is the source data buffer.\n\n        \n\n            This is the destination data buffer that is in either\n            uninitialized or freed state.\n\nC++: wxAnyValueType::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def("ConvertValue", (bool (wxAnyValueType::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueType::ConvertValue, "Convert value into buffer of different type. Return false if\n        not possible.\n\nC++: wxAnyValueType::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueType & (wxAnyValueType::*)(const class wxAnyValueType &)) &wxAnyValueType::operator=, "C++: wxAnyValueType::operator=(const class wxAnyValueType &) --> class wxAnyValueType &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplBase file: line:284
		pybind11::class_<wxAnyValueTypeImplBase<unsigned long long>, std::shared_ptr<wxAnyValueTypeImplBase<unsigned long long>>, PyCallBack_wxAnyValueTypeImplBase_unsigned_long_long_t, wxAnyValueType> cl(M(""), "wxAnyValueTypeImplBase_unsigned_long_long_t", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxAnyValueTypeImplBase_unsigned_long_long_t(); } ) );
		cl.def(pybind11::init<PyCallBack_wxAnyValueTypeImplBase_unsigned_long_long_t const &>());
		cl.def("DeleteValue", (void (wxAnyValueTypeImplBase<unsigned long long>::*)(union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<unsigned long long>::DeleteValue, "C++: wxAnyValueTypeImplBase<unsigned long long>::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueTypeImplBase<unsigned long long>::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<unsigned long long>::CopyBuffer, "C++: wxAnyValueTypeImplBase<unsigned long long>::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def_static("SetValue", (void (*)(const unsigned long long &, union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<unsigned long long>::SetValue, "C++: wxAnyValueTypeImplBase<unsigned long long>::SetValue(const unsigned long long &, union wxAnyValueBuffer &) --> void", pybind11::arg("value"), pybind11::arg("buf"));
		cl.def_static("GetValue", (const unsigned long long & (*)(const union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<unsigned long long>::GetValue, "C++: wxAnyValueTypeImplBase<unsigned long long>::GetValue(const union wxAnyValueBuffer &) --> const unsigned long long &", pybind11::return_value_policy::automatic, pybind11::arg("buf"));
		cl.def("assign", (class wxAnyValueTypeImplBase<unsigned long long> & (wxAnyValueTypeImplBase<unsigned long long>::*)(const class wxAnyValueTypeImplBase<unsigned long long> &)) &wxAnyValueTypeImplBase<unsigned long long>::operator=, "C++: wxAnyValueTypeImplBase<unsigned long long>::operator=(const class wxAnyValueTypeImplBase<unsigned long long> &) --> class wxAnyValueTypeImplBase<unsigned long long> &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("IsSameType", (bool (wxAnyValueType::*)(const class wxAnyValueType *) const) &wxAnyValueType::IsSameType, "This function is used for internal type matching.\n\nC++: wxAnyValueType::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def("DeleteValue", (void (wxAnyValueType::*)(union wxAnyValueBuffer &) const) &wxAnyValueType::DeleteValue, "This function is called every time the data in wxAny\n        buffer needs to be freed.\n\nC++: wxAnyValueType::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueType::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueType::CopyBuffer, "Implement this for buffer-to-buffer copy.\n\n        \n\n            This is the source data buffer.\n\n        \n\n            This is the destination data buffer that is in either\n            uninitialized or freed state.\n\nC++: wxAnyValueType::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def("ConvertValue", (bool (wxAnyValueType::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueType::ConvertValue, "Convert value into buffer of different type. Return false if\n        not possible.\n\nC++: wxAnyValueType::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueType & (wxAnyValueType::*)(const class wxAnyValueType &)) &wxAnyValueType::operator=, "C++: wxAnyValueType::operator=(const class wxAnyValueType &) --> class wxAnyValueType &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplBase file: line:284
		pybind11::class_<wxAnyValueTypeImplBase<wxString>, std::shared_ptr<wxAnyValueTypeImplBase<wxString>>, PyCallBack_wxAnyValueTypeImplBase_wxString_t, wxAnyValueType> cl(M(""), "wxAnyValueTypeImplBase_wxString_t", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxAnyValueTypeImplBase_wxString_t(); } ) );
		cl.def(pybind11::init<PyCallBack_wxAnyValueTypeImplBase_wxString_t const &>());
		cl.def("DeleteValue", (void (wxAnyValueTypeImplBase<wxString>::*)(union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<wxString>::DeleteValue, "C++: wxAnyValueTypeImplBase<wxString>::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueTypeImplBase<wxString>::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<wxString>::CopyBuffer, "C++: wxAnyValueTypeImplBase<wxString>::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def_static("SetValue", (void (*)(const class wxString &, union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<wxString>::SetValue, "C++: wxAnyValueTypeImplBase<wxString>::SetValue(const class wxString &, union wxAnyValueBuffer &) --> void", pybind11::arg("value"), pybind11::arg("buf"));
		cl.def_static("GetValue", (const class wxString & (*)(const union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<wxString>::GetValue, "C++: wxAnyValueTypeImplBase<wxString>::GetValue(const union wxAnyValueBuffer &) --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("buf"));
		cl.def("assign", (class wxAnyValueTypeImplBase<class wxString> & (wxAnyValueTypeImplBase<wxString>::*)(const class wxAnyValueTypeImplBase<class wxString> &)) &wxAnyValueTypeImplBase<wxString>::operator=, "C++: wxAnyValueTypeImplBase<wxString>::operator=(const class wxAnyValueTypeImplBase<class wxString> &) --> class wxAnyValueTypeImplBase<class wxString> &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("IsSameType", (bool (wxAnyValueType::*)(const class wxAnyValueType *) const) &wxAnyValueType::IsSameType, "This function is used for internal type matching.\n\nC++: wxAnyValueType::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def("DeleteValue", (void (wxAnyValueType::*)(union wxAnyValueBuffer &) const) &wxAnyValueType::DeleteValue, "This function is called every time the data in wxAny\n        buffer needs to be freed.\n\nC++: wxAnyValueType::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueType::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueType::CopyBuffer, "Implement this for buffer-to-buffer copy.\n\n        \n\n            This is the source data buffer.\n\n        \n\n            This is the destination data buffer that is in either\n            uninitialized or freed state.\n\nC++: wxAnyValueType::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def("ConvertValue", (bool (wxAnyValueType::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueType::ConvertValue, "Convert value into buffer of different type. Return false if\n        not possible.\n\nC++: wxAnyValueType::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueType & (wxAnyValueType::*)(const class wxAnyValueType &)) &wxAnyValueType::operator=, "C++: wxAnyValueType::operator=(const class wxAnyValueType &) --> class wxAnyValueType &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplBase file: line:284
		pybind11::class_<wxAnyValueTypeImplBase<const char *>, std::shared_ptr<wxAnyValueTypeImplBase<const char *>>, PyCallBack_wxAnyValueTypeImplBase_const_char_*_t, wxAnyValueType> cl(M(""), "wxAnyValueTypeImplBase_const_char_*_t", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxAnyValueTypeImplBase_const_char_*_t(); } ) );
		cl.def(pybind11::init<PyCallBack_wxAnyValueTypeImplBase_const_char_*_t const &>());
		cl.def("DeleteValue", (void (wxAnyValueTypeImplBase<const char *>::*)(union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<const char *>::DeleteValue, "C++: wxAnyValueTypeImplBase<const char *>::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueTypeImplBase<const char *>::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplBase<const char *>::CopyBuffer, "C++: wxAnyValueTypeImplBase<const char *>::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def_static("SetValue", (void (*)(const char *const &, union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<const char *>::SetValue, "C++: wxAnyValueTypeImplBase<const char *>::SetValue(const char *const &, union wxAnyValueBuffer &) --> void", pybind11::arg("value"), pybind11::arg("buf"));
		cl.def_static("GetValue", (const char *const & (*)(const union wxAnyValueBuffer &)) &wxAnyValueTypeImplBase<const char *>::GetValue, "C++: wxAnyValueTypeImplBase<const char *>::GetValue(const union wxAnyValueBuffer &) --> const char *const &", pybind11::return_value_policy::automatic, pybind11::arg("buf"));
		cl.def("assign", (class wxAnyValueTypeImplBase<const char *> & (wxAnyValueTypeImplBase<const char *>::*)(const class wxAnyValueTypeImplBase<const char *> &)) &wxAnyValueTypeImplBase<const char *>::operator=, "C++: wxAnyValueTypeImplBase<const char *>::operator=(const class wxAnyValueTypeImplBase<const char *> &) --> class wxAnyValueTypeImplBase<const char *> &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("IsSameType", (bool (wxAnyValueType::*)(const class wxAnyValueType *) const) &wxAnyValueType::IsSameType, "This function is used for internal type matching.\n\nC++: wxAnyValueType::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def("DeleteValue", (void (wxAnyValueType::*)(union wxAnyValueBuffer &) const) &wxAnyValueType::DeleteValue, "This function is called every time the data in wxAny\n        buffer needs to be freed.\n\nC++: wxAnyValueType::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueType::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueType::CopyBuffer, "Implement this for buffer-to-buffer copy.\n\n        \n\n            This is the source data buffer.\n\n        \n\n            This is the destination data buffer that is in either\n            uninitialized or freed state.\n\nC++: wxAnyValueType::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def("ConvertValue", (bool (wxAnyValueType::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueType::ConvertValue, "Convert value into buffer of different type. Return false if\n        not possible.\n\nC++: wxAnyValueType::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueType & (wxAnyValueType::*)(const class wxAnyValueType &)) &wxAnyValueType::operator=, "C++: wxAnyValueType::operator=(const class wxAnyValueType &) --> class wxAnyValueType &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
