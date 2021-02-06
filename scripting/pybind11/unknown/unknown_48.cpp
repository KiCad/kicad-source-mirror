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

// wxAnyValueTypeImplwxString file: line:74
struct PyCallBack_wxAnyValueTypeImplwxString : public wxAnyValueTypeImplwxString {
	using wxAnyValueTypeImplwxString::wxAnyValueTypeImplwxString;

	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplwxString *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplwxString::IsSameType(a0);
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplwxString *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplwxString::ConvertValue(a0, a1, a2);
	}
	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplwxString *>(this), "DeleteValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplwxString *>(this), "CopyBuffer");
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

// wxAnyValueTypeImplConstCharPtr file: line:80
struct PyCallBack_wxAnyValueTypeImplConstCharPtr : public wxAnyValueTypeImplConstCharPtr {
	using wxAnyValueTypeImplConstCharPtr::wxAnyValueTypeImplConstCharPtr;

	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplConstCharPtr *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplConstCharPtr::IsSameType(a0);
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplConstCharPtr *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplConstCharPtr::ConvertValue(a0, a1, a2);
	}
	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplConstCharPtr *>(this), "DeleteValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplConstCharPtr *>(this), "CopyBuffer");
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

// wxAnyValueTypeImplConstWchar_tPtr file: line:86
struct PyCallBack_wxAnyValueTypeImplConstWchar_tPtr : public wxAnyValueTypeImplConstWchar_tPtr {
	using wxAnyValueTypeImplConstWchar_tPtr::wxAnyValueTypeImplConstWchar_tPtr;

	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplConstWchar_tPtr *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplConstWchar_tPtr::IsSameType(a0);
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplConstWchar_tPtr *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplConstWchar_tPtr::ConvertValue(a0, a1, a2);
	}
	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplConstWchar_tPtr *>(this), "DeleteValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplConstWchar_tPtr *>(this), "CopyBuffer");
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

// wxAnyValueTypeImplDouble file: line:533
struct PyCallBack_wxAnyValueTypeImplDouble : public wxAnyValueTypeImplDouble {
	using wxAnyValueTypeImplDouble::wxAnyValueTypeImplDouble;

	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplDouble *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplDouble::IsSameType(a0);
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplDouble *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplDouble::ConvertValue(a0, a1, a2);
	}
	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplDouble *>(this), "DeleteValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplDouble *>(this), "CopyBuffer");
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

void bind_unknown_unknown_48(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxAnyValueTypeImplwxString file: line:74
		pybind11::class_<wxAnyValueTypeImplwxString, std::shared_ptr<wxAnyValueTypeImplwxString>, PyCallBack_wxAnyValueTypeImplwxString, wxAnyValueTypeImplBase<wxString>> cl(M(""), "wxAnyValueTypeImplwxString", "");
		cl.def( pybind11::init( [](){ return new wxAnyValueTypeImplwxString(); }, [](){ return new PyCallBack_wxAnyValueTypeImplwxString(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxAnyValueTypeImplwxString const &o){ return new PyCallBack_wxAnyValueTypeImplwxString(o); } ) );
		cl.def( pybind11::init( [](wxAnyValueTypeImplwxString const &o){ return new wxAnyValueTypeImplwxString(o); } ) );
		cl.def_static("IsSameClass", (bool (*)(const class wxAnyValueType *)) &wxAnyValueTypeImplwxString::IsSameClass, "C++: wxAnyValueTypeImplwxString::IsSameClass(const class wxAnyValueType *) --> bool", pybind11::arg("otherType"));
		cl.def("IsSameType", (bool (wxAnyValueTypeImplwxString::*)(const class wxAnyValueType *) const) &wxAnyValueTypeImplwxString::IsSameType, "C++: wxAnyValueTypeImplwxString::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def_static("GetInstance", (class wxAnyValueType * (*)()) &wxAnyValueTypeImplwxString::GetInstance, "C++: wxAnyValueTypeImplwxString::GetInstance() --> class wxAnyValueType *", pybind11::return_value_policy::automatic);
		cl.def("ConvertValue", (bool (wxAnyValueTypeImplwxString::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplwxString::ConvertValue, "C++: wxAnyValueTypeImplwxString::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueTypeImplwxString & (wxAnyValueTypeImplwxString::*)(const class wxAnyValueTypeImplwxString &)) &wxAnyValueTypeImplwxString::operator=, "C++: wxAnyValueTypeImplwxString::operator=(const class wxAnyValueTypeImplwxString &) --> class wxAnyValueTypeImplwxString &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplConstCharPtr file: line:80
		pybind11::class_<wxAnyValueTypeImplConstCharPtr, std::shared_ptr<wxAnyValueTypeImplConstCharPtr>, PyCallBack_wxAnyValueTypeImplConstCharPtr, wxAnyValueTypeImplBase<const char *>> cl(M(""), "wxAnyValueTypeImplConstCharPtr", "");
		cl.def( pybind11::init( [](){ return new wxAnyValueTypeImplConstCharPtr(); }, [](){ return new PyCallBack_wxAnyValueTypeImplConstCharPtr(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxAnyValueTypeImplConstCharPtr const &o){ return new PyCallBack_wxAnyValueTypeImplConstCharPtr(o); } ) );
		cl.def( pybind11::init( [](wxAnyValueTypeImplConstCharPtr const &o){ return new wxAnyValueTypeImplConstCharPtr(o); } ) );
		cl.def_static("IsSameClass", (bool (*)(const class wxAnyValueType *)) &wxAnyValueTypeImplConstCharPtr::IsSameClass, "C++: wxAnyValueTypeImplConstCharPtr::IsSameClass(const class wxAnyValueType *) --> bool", pybind11::arg("otherType"));
		cl.def("IsSameType", (bool (wxAnyValueTypeImplConstCharPtr::*)(const class wxAnyValueType *) const) &wxAnyValueTypeImplConstCharPtr::IsSameType, "C++: wxAnyValueTypeImplConstCharPtr::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def_static("GetInstance", (class wxAnyValueType * (*)()) &wxAnyValueTypeImplConstCharPtr::GetInstance, "C++: wxAnyValueTypeImplConstCharPtr::GetInstance() --> class wxAnyValueType *", pybind11::return_value_policy::automatic);
		cl.def("ConvertValue", (bool (wxAnyValueTypeImplConstCharPtr::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplConstCharPtr::ConvertValue, "C++: wxAnyValueTypeImplConstCharPtr::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueTypeImplConstCharPtr & (wxAnyValueTypeImplConstCharPtr::*)(const class wxAnyValueTypeImplConstCharPtr &)) &wxAnyValueTypeImplConstCharPtr::operator=, "C++: wxAnyValueTypeImplConstCharPtr::operator=(const class wxAnyValueTypeImplConstCharPtr &) --> class wxAnyValueTypeImplConstCharPtr &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplConstWchar_tPtr file: line:86
		pybind11::class_<wxAnyValueTypeImplConstWchar_tPtr, std::shared_ptr<wxAnyValueTypeImplConstWchar_tPtr>, PyCallBack_wxAnyValueTypeImplConstWchar_tPtr, wxAnyValueTypeImplBase<const wchar_t *>> cl(M(""), "wxAnyValueTypeImplConstWchar_tPtr", "");
		cl.def( pybind11::init( [](){ return new wxAnyValueTypeImplConstWchar_tPtr(); }, [](){ return new PyCallBack_wxAnyValueTypeImplConstWchar_tPtr(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxAnyValueTypeImplConstWchar_tPtr const &o){ return new PyCallBack_wxAnyValueTypeImplConstWchar_tPtr(o); } ) );
		cl.def( pybind11::init( [](wxAnyValueTypeImplConstWchar_tPtr const &o){ return new wxAnyValueTypeImplConstWchar_tPtr(o); } ) );
		cl.def_static("IsSameClass", (bool (*)(const class wxAnyValueType *)) &wxAnyValueTypeImplConstWchar_tPtr::IsSameClass, "C++: wxAnyValueTypeImplConstWchar_tPtr::IsSameClass(const class wxAnyValueType *) --> bool", pybind11::arg("otherType"));
		cl.def("IsSameType", (bool (wxAnyValueTypeImplConstWchar_tPtr::*)(const class wxAnyValueType *) const) &wxAnyValueTypeImplConstWchar_tPtr::IsSameType, "C++: wxAnyValueTypeImplConstWchar_tPtr::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def_static("GetInstance", (class wxAnyValueType * (*)()) &wxAnyValueTypeImplConstWchar_tPtr::GetInstance, "C++: wxAnyValueTypeImplConstWchar_tPtr::GetInstance() --> class wxAnyValueType *", pybind11::return_value_policy::automatic);
		cl.def("ConvertValue", (bool (wxAnyValueTypeImplConstWchar_tPtr::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplConstWchar_tPtr::ConvertValue, "C++: wxAnyValueTypeImplConstWchar_tPtr::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueTypeImplConstWchar_tPtr & (wxAnyValueTypeImplConstWchar_tPtr::*)(const class wxAnyValueTypeImplConstWchar_tPtr &)) &wxAnyValueTypeImplConstWchar_tPtr::operator=, "C++: wxAnyValueTypeImplConstWchar_tPtr::operator=(const class wxAnyValueTypeImplConstWchar_tPtr &) --> class wxAnyValueTypeImplConstWchar_tPtr &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplDouble file: line:533
		pybind11::class_<wxAnyValueTypeImplDouble, std::shared_ptr<wxAnyValueTypeImplDouble>, PyCallBack_wxAnyValueTypeImplDouble, wxAnyValueTypeImplBase<double>> cl(M(""), "wxAnyValueTypeImplDouble", "");
		cl.def( pybind11::init( [](){ return new wxAnyValueTypeImplDouble(); }, [](){ return new PyCallBack_wxAnyValueTypeImplDouble(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxAnyValueTypeImplDouble const &o){ return new PyCallBack_wxAnyValueTypeImplDouble(o); } ) );
		cl.def( pybind11::init( [](wxAnyValueTypeImplDouble const &o){ return new wxAnyValueTypeImplDouble(o); } ) );
		cl.def_static("IsSameClass", (bool (*)(const class wxAnyValueType *)) &wxAnyValueTypeImplDouble::IsSameClass, "C++: wxAnyValueTypeImplDouble::IsSameClass(const class wxAnyValueType *) --> bool", pybind11::arg("otherType"));
		cl.def("IsSameType", (bool (wxAnyValueTypeImplDouble::*)(const class wxAnyValueType *) const) &wxAnyValueTypeImplDouble::IsSameType, "C++: wxAnyValueTypeImplDouble::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def_static("GetInstance", (class wxAnyValueType * (*)()) &wxAnyValueTypeImplDouble::GetInstance, "C++: wxAnyValueTypeImplDouble::GetInstance() --> class wxAnyValueType *", pybind11::return_value_policy::automatic);
		cl.def("ConvertValue", (bool (wxAnyValueTypeImplDouble::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplDouble::ConvertValue, "C++: wxAnyValueTypeImplDouble::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueTypeImplDouble & (wxAnyValueTypeImplDouble::*)(const class wxAnyValueTypeImplDouble &)) &wxAnyValueTypeImplDouble::operator=, "C++: wxAnyValueTypeImplDouble::operator=(const class wxAnyValueTypeImplDouble &) --> class wxAnyValueTypeImplDouble &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyStrPtr file: line:29
		pybind11::class_<wxAnyStrPtr, std::shared_ptr<wxAnyStrPtr>> cl(M(""), "wxAnyStrPtr", "");
		cl.def( pybind11::init( [](){ return new wxAnyStrPtr(); } ) );
		cl.def( pybind11::init<const class wxString &, const class wxString::const_iterator &>(), pybind11::arg("str"), pybind11::arg("iter") );

		cl.def( pybind11::init( [](wxAnyStrPtr const &o){ return new wxAnyStrPtr(o); } ) );
	}
	{ // wxDateTime file: line:132
		pybind11::class_<wxDateTime, std::shared_ptr<wxDateTime>> cl(M(""), "wxDateTime", "");
		cl.def( pybind11::init( [](){ return new wxDateTime(); } ) );
		cl.def( pybind11::init<long>(), pybind11::arg("timet") );

		cl.def( pybind11::init<const struct tm &>(), pybind11::arg("tm") );

		cl.def( pybind11::init<const struct wxDateTime::Tm &>(), pybind11::arg("tm") );

		cl.def( pybind11::init<double>(), pybind11::arg("jdn") );

		cl.def( pybind11::init( [](unsigned short const & a0){ return new wxDateTime(a0); } ), "doc" , pybind11::arg("hour"));
		cl.def( pybind11::init( [](unsigned short const & a0, unsigned short const & a1){ return new wxDateTime(a0, a1); } ), "doc" , pybind11::arg("hour"), pybind11::arg("minute"));
		cl.def( pybind11::init( [](unsigned short const & a0, unsigned short const & a1, unsigned short const & a2){ return new wxDateTime(a0, a1, a2); } ), "doc" , pybind11::arg("hour"), pybind11::arg("minute"), pybind11::arg("second"));
		cl.def( pybind11::init<unsigned short, unsigned short, unsigned short, unsigned short>(), pybind11::arg("hour"), pybind11::arg("minute"), pybind11::arg("second"), pybind11::arg("millisec") );

		cl.def( pybind11::init( [](unsigned short const & a0, enum wxDateTime::Month const & a1){ return new wxDateTime(a0, a1); } ), "doc" , pybind11::arg("day"), pybind11::arg("month"));
		cl.def( pybind11::init( [](unsigned short const & a0, enum wxDateTime::Month const & a1, int const & a2){ return new wxDateTime(a0, a1, a2); } ), "doc" , pybind11::arg("day"), pybind11::arg("month"), pybind11::arg("year"));
		cl.def( pybind11::init( [](unsigned short const & a0, enum wxDateTime::Month const & a1, int const & a2, unsigned short const & a3){ return new wxDateTime(a0, a1, a2, a3); } ), "doc" , pybind11::arg("day"), pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("hour"));
		cl.def( pybind11::init( [](unsigned short const & a0, enum wxDateTime::Month const & a1, int const & a2, unsigned short const & a3, unsigned short const & a4){ return new wxDateTime(a0, a1, a2, a3, a4); } ), "doc" , pybind11::arg("day"), pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("hour"), pybind11::arg("minute"));
		cl.def( pybind11::init( [](unsigned short const & a0, enum wxDateTime::Month const & a1, int const & a2, unsigned short const & a3, unsigned short const & a4, unsigned short const & a5){ return new wxDateTime(a0, a1, a2, a3, a4, a5); } ), "doc" , pybind11::arg("day"), pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("hour"), pybind11::arg("minute"), pybind11::arg("second"));
		cl.def( pybind11::init<unsigned short, enum wxDateTime::Month, int, unsigned short, unsigned short, unsigned short, unsigned short>(), pybind11::arg("day"), pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("hour"), pybind11::arg("minute"), pybind11::arg("second"), pybind11::arg("millisec") );

		cl.def( pybind11::init<const class wxLongLongNative &>(), pybind11::arg("time") );

		cl.def( pybind11::init( [](wxDateTime const &o){ return new wxDateTime(o); } ) );

		pybind11::enum_<wxDateTime::TZ>(cl, "TZ", pybind11::arithmetic(), "")
			.value("Local", wxDateTime::Local)
			.value("GMT_12", wxDateTime::GMT_12)
			.value("GMT_11", wxDateTime::GMT_11)
			.value("GMT_10", wxDateTime::GMT_10)
			.value("GMT_9", wxDateTime::GMT_9)
			.value("GMT_8", wxDateTime::GMT_8)
			.value("GMT_7", wxDateTime::GMT_7)
			.value("GMT_6", wxDateTime::GMT_6)
			.value("GMT_5", wxDateTime::GMT_5)
			.value("GMT_4", wxDateTime::GMT_4)
			.value("GMT_3", wxDateTime::GMT_3)
			.value("GMT_2", wxDateTime::GMT_2)
			.value("GMT_1", wxDateTime::GMT_1)
			.value("GMT0", wxDateTime::GMT0)
			.value("GMT1", wxDateTime::GMT1)
			.value("GMT2", wxDateTime::GMT2)
			.value("GMT3", wxDateTime::GMT3)
			.value("GMT4", wxDateTime::GMT4)
			.value("GMT5", wxDateTime::GMT5)
			.value("GMT6", wxDateTime::GMT6)
			.value("GMT7", wxDateTime::GMT7)
			.value("GMT8", wxDateTime::GMT8)
			.value("GMT9", wxDateTime::GMT9)
			.value("GMT10", wxDateTime::GMT10)
			.value("GMT11", wxDateTime::GMT11)
			.value("GMT12", wxDateTime::GMT12)
			.value("GMT13", wxDateTime::GMT13)
			.value("WET", wxDateTime::WET)
			.value("WEST", wxDateTime::WEST)
			.value("CET", wxDateTime::CET)
			.value("CEST", wxDateTime::CEST)
			.value("EET", wxDateTime::EET)
			.value("EEST", wxDateTime::EEST)
			.value("MSK", wxDateTime::MSK)
			.value("MSD", wxDateTime::MSD)
			.value("AST", wxDateTime::AST)
			.value("ADT", wxDateTime::ADT)
			.value("EST", wxDateTime::EST)
			.value("EDT", wxDateTime::EDT)
			.value("CST", wxDateTime::CST)
			.value("CDT", wxDateTime::CDT)
			.value("MST", wxDateTime::MST)
			.value("MDT", wxDateTime::MDT)
			.value("PST", wxDateTime::PST)
			.value("PDT", wxDateTime::PDT)
			.value("HST", wxDateTime::HST)
			.value("AKST", wxDateTime::AKST)
			.value("AKDT", wxDateTime::AKDT)
			.value("A_WST", wxDateTime::A_WST)
			.value("A_CST", wxDateTime::A_CST)
			.value("A_EST", wxDateTime::A_EST)
			.value("A_ESST", wxDateTime::A_ESST)
			.value("NZST", wxDateTime::NZST)
			.value("NZDT", wxDateTime::NZDT)
			.value("UTC", wxDateTime::UTC)
			.export_values();


		pybind11::enum_<wxDateTime::Calendar>(cl, "Calendar", pybind11::arithmetic(), "")
			.value("Gregorian", wxDateTime::Gregorian)
			.value("Julian", wxDateTime::Julian)
			.export_values();


		pybind11::enum_<wxDateTime::Country>(cl, "Country", pybind11::arithmetic(), "")
			.value("Country_Unknown", wxDateTime::Country_Unknown)
			.value("Country_Default", wxDateTime::Country_Default)
			.value("Country_WesternEurope_Start", wxDateTime::Country_WesternEurope_Start)
			.value("Country_EEC", wxDateTime::Country_EEC)
			.value("France", wxDateTime::France)
			.value("Germany", wxDateTime::Germany)
			.value("UK", wxDateTime::UK)
			.value("Country_WesternEurope_End", wxDateTime::Country_WesternEurope_End)
			.value("Russia", wxDateTime::Russia)
			.value("USA", wxDateTime::USA)
			.export_values();


		pybind11::enum_<wxDateTime::Month>(cl, "Month", pybind11::arithmetic(), "")
			.value("Jan", wxDateTime::Jan)
			.value("Feb", wxDateTime::Feb)
			.value("Mar", wxDateTime::Mar)
			.value("Apr", wxDateTime::Apr)
			.value("May", wxDateTime::May)
			.value("Jun", wxDateTime::Jun)
			.value("Jul", wxDateTime::Jul)
			.value("Aug", wxDateTime::Aug)
			.value("Sep", wxDateTime::Sep)
			.value("Oct", wxDateTime::Oct)
			.value("Nov", wxDateTime::Nov)
			.value("Dec", wxDateTime::Dec)
			.value("Inv_Month", wxDateTime::Inv_Month)
			.export_values();


		pybind11::enum_<wxDateTime::WeekDay>(cl, "WeekDay", pybind11::arithmetic(), "")
			.value("Sun", wxDateTime::Sun)
			.value("Mon", wxDateTime::Mon)
			.value("Tue", wxDateTime::Tue)
			.value("Wed", wxDateTime::Wed)
			.value("Thu", wxDateTime::Thu)
			.value("Fri", wxDateTime::Fri)
			.value("Sat", wxDateTime::Sat)
			.value("Inv_WeekDay", wxDateTime::Inv_WeekDay)
			.export_values();


		pybind11::enum_<wxDateTime::Year>(cl, "Year", pybind11::arithmetic(), "")
			.value("Inv_Year", wxDateTime::Inv_Year)
			.export_values();


		pybind11::enum_<wxDateTime::NameFlags>(cl, "NameFlags", pybind11::arithmetic(), "")
			.value("Name_Full", wxDateTime::Name_Full)
			.value("Name_Abbr", wxDateTime::Name_Abbr)
			.export_values();


		pybind11::enum_<wxDateTime::WeekFlags>(cl, "WeekFlags", pybind11::arithmetic(), "")
			.value("Default_First", wxDateTime::Default_First)
			.value("Monday_First", wxDateTime::Monday_First)
			.value("Sunday_First", wxDateTime::Sunday_First)
			.export_values();

		cl.def_static("SetCountry", (void (*)(enum wxDateTime::Country)) &wxDateTime::SetCountry, "C++: wxDateTime::SetCountry(enum wxDateTime::Country) --> void", pybind11::arg("country"));
		cl.def_static("GetCountry", (enum wxDateTime::Country (*)()) &wxDateTime::GetCountry, "C++: wxDateTime::GetCountry() --> enum wxDateTime::Country");
		cl.def_static("IsWestEuropeanCountry", []() -> bool { return wxDateTime::IsWestEuropeanCountry(); }, "");
		cl.def_static("IsWestEuropeanCountry", (bool (*)(enum wxDateTime::Country)) &wxDateTime::IsWestEuropeanCountry, "C++: wxDateTime::IsWestEuropeanCountry(enum wxDateTime::Country) --> bool", pybind11::arg("country"));
		cl.def_static("GetCurrentYear", []() -> int { return wxDateTime::GetCurrentYear(); }, "");
		cl.def_static("GetCurrentYear", (int (*)(enum wxDateTime::Calendar)) &wxDateTime::GetCurrentYear, "C++: wxDateTime::GetCurrentYear(enum wxDateTime::Calendar) --> int", pybind11::arg("cal"));
		cl.def_static("ConvertYearToBC", (int (*)(int)) &wxDateTime::ConvertYearToBC, "C++: wxDateTime::ConvertYearToBC(int) --> int", pybind11::arg("year"));
		cl.def_static("GetCurrentMonth", []() -> wxDateTime::Month { return wxDateTime::GetCurrentMonth(); }, "");
		cl.def_static("GetCurrentMonth", (enum wxDateTime::Month (*)(enum wxDateTime::Calendar)) &wxDateTime::GetCurrentMonth, "C++: wxDateTime::GetCurrentMonth(enum wxDateTime::Calendar) --> enum wxDateTime::Month", pybind11::arg("cal"));
		cl.def_static("IsLeapYear", []() -> bool { return wxDateTime::IsLeapYear(); }, "");
		cl.def_static("IsLeapYear", [](int const & a0) -> bool { return wxDateTime::IsLeapYear(a0); }, "", pybind11::arg("year"));
		cl.def_static("IsLeapYear", (bool (*)(int, enum wxDateTime::Calendar)) &wxDateTime::IsLeapYear, "C++: wxDateTime::IsLeapYear(int, enum wxDateTime::Calendar) --> bool", pybind11::arg("year"), pybind11::arg("cal"));
		cl.def_static("GetCentury", (int (*)(int)) &wxDateTime::GetCentury, "C++: wxDateTime::GetCentury(int) --> int", pybind11::arg("year"));
		cl.def_static("GetNumberOfDays", [](int const & a0) -> unsigned short { return wxDateTime::GetNumberOfDays(a0); }, "", pybind11::arg("year"));
		cl.def_static("GetNumberOfDays", (unsigned short (*)(int, enum wxDateTime::Calendar)) &wxDateTime::GetNumberOfDays, "C++: wxDateTime::GetNumberOfDays(int, enum wxDateTime::Calendar) --> unsigned short", pybind11::arg("year"), pybind11::arg("cal"));
		cl.def_static("GetNumberOfDays", [](enum wxDateTime::Month const & a0) -> unsigned short { return wxDateTime::GetNumberOfDays(a0); }, "", pybind11::arg("month"));
		cl.def_static("GetNumberOfDays", [](enum wxDateTime::Month const & a0, int const & a1) -> unsigned short { return wxDateTime::GetNumberOfDays(a0, a1); }, "", pybind11::arg("month"), pybind11::arg("year"));
		cl.def_static("GetNumberOfDays", (unsigned short (*)(enum wxDateTime::Month, int, enum wxDateTime::Calendar)) &wxDateTime::GetNumberOfDays, "C++: wxDateTime::GetNumberOfDays(enum wxDateTime::Month, int, enum wxDateTime::Calendar) --> unsigned short", pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("cal"));
		cl.def_static("GetMonthName", [](enum wxDateTime::Month const & a0) -> wxString { return wxDateTime::GetMonthName(a0); }, "", pybind11::arg("month"));
		cl.def_static("GetMonthName", (class wxString (*)(enum wxDateTime::Month, enum wxDateTime::NameFlags)) &wxDateTime::GetMonthName, "C++: wxDateTime::GetMonthName(enum wxDateTime::Month, enum wxDateTime::NameFlags) --> class wxString", pybind11::arg("month"), pybind11::arg("flags"));
		cl.def_static("GetEnglishMonthName", [](enum wxDateTime::Month const & a0) -> wxString { return wxDateTime::GetEnglishMonthName(a0); }, "", pybind11::arg("month"));
		cl.def_static("GetEnglishMonthName", (class wxString (*)(enum wxDateTime::Month, enum wxDateTime::NameFlags)) &wxDateTime::GetEnglishMonthName, "C++: wxDateTime::GetEnglishMonthName(enum wxDateTime::Month, enum wxDateTime::NameFlags) --> class wxString", pybind11::arg("month"), pybind11::arg("flags"));
		cl.def_static("GetWeekDayName", [](enum wxDateTime::WeekDay const & a0) -> wxString { return wxDateTime::GetWeekDayName(a0); }, "", pybind11::arg("weekday"));
		cl.def_static("GetWeekDayName", (class wxString (*)(enum wxDateTime::WeekDay, enum wxDateTime::NameFlags)) &wxDateTime::GetWeekDayName, "C++: wxDateTime::GetWeekDayName(enum wxDateTime::WeekDay, enum wxDateTime::NameFlags) --> class wxString", pybind11::arg("weekday"), pybind11::arg("flags"));
		cl.def_static("GetEnglishWeekDayName", [](enum wxDateTime::WeekDay const & a0) -> wxString { return wxDateTime::GetEnglishWeekDayName(a0); }, "", pybind11::arg("weekday"));
		cl.def_static("GetEnglishWeekDayName", (class wxString (*)(enum wxDateTime::WeekDay, enum wxDateTime::NameFlags)) &wxDateTime::GetEnglishWeekDayName, "C++: wxDateTime::GetEnglishWeekDayName(enum wxDateTime::WeekDay, enum wxDateTime::NameFlags) --> class wxString", pybind11::arg("weekday"), pybind11::arg("flags"));
		cl.def_static("GetAmPmStrings", (void (*)(class wxString *, class wxString *)) &wxDateTime::GetAmPmStrings, "C++: wxDateTime::GetAmPmStrings(class wxString *, class wxString *) --> void", pybind11::arg("am"), pybind11::arg("pm"));
		cl.def_static("IsDSTApplicable", []() -> bool { return wxDateTime::IsDSTApplicable(); }, "");
		cl.def_static("IsDSTApplicable", [](int const & a0) -> bool { return wxDateTime::IsDSTApplicable(a0); }, "", pybind11::arg("year"));
		cl.def_static("IsDSTApplicable", (bool (*)(int, enum wxDateTime::Country)) &wxDateTime::IsDSTApplicable, "C++: wxDateTime::IsDSTApplicable(int, enum wxDateTime::Country) --> bool", pybind11::arg("year"), pybind11::arg("country"));
		cl.def_static("GetBeginDST", []() -> wxDateTime { return wxDateTime::GetBeginDST(); }, "");
		cl.def_static("GetBeginDST", [](int const & a0) -> wxDateTime { return wxDateTime::GetBeginDST(a0); }, "", pybind11::arg("year"));
		cl.def_static("GetBeginDST", (class wxDateTime (*)(int, enum wxDateTime::Country)) &wxDateTime::GetBeginDST, "C++: wxDateTime::GetBeginDST(int, enum wxDateTime::Country) --> class wxDateTime", pybind11::arg("year"), pybind11::arg("country"));
		cl.def_static("GetEndDST", []() -> wxDateTime { return wxDateTime::GetEndDST(); }, "");
		cl.def_static("GetEndDST", [](int const & a0) -> wxDateTime { return wxDateTime::GetEndDST(a0); }, "", pybind11::arg("year"));
		cl.def_static("GetEndDST", (class wxDateTime (*)(int, enum wxDateTime::Country)) &wxDateTime::GetEndDST, "C++: wxDateTime::GetEndDST(int, enum wxDateTime::Country) --> class wxDateTime", pybind11::arg("year"), pybind11::arg("country"));
		cl.def_static("Now", (class wxDateTime (*)()) &wxDateTime::Now, "C++: wxDateTime::Now() --> class wxDateTime");
		cl.def_static("UNow", (class wxDateTime (*)()) &wxDateTime::UNow, "C++: wxDateTime::UNow() --> class wxDateTime");
		cl.def_static("Today", (class wxDateTime (*)()) &wxDateTime::Today, "C++: wxDateTime::Today() --> class wxDateTime");
		cl.def("SetToCurrent", (class wxDateTime & (wxDateTime::*)()) &wxDateTime::SetToCurrent, "C++: wxDateTime::SetToCurrent() --> class wxDateTime &", pybind11::return_value_policy::automatic);
		cl.def("Set", (class wxDateTime & (wxDateTime::*)(long)) &wxDateTime::Set, "C++: wxDateTime::Set(long) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("timet"));
		cl.def("Set", (class wxDateTime & (wxDateTime::*)(const struct tm &)) &wxDateTime::Set, "C++: wxDateTime::Set(const struct tm &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("tm"));
		cl.def("Set", (class wxDateTime & (wxDateTime::*)(const struct wxDateTime::Tm &)) &wxDateTime::Set, "C++: wxDateTime::Set(const struct wxDateTime::Tm &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("tm"));
		cl.def("Set", (class wxDateTime & (wxDateTime::*)(double)) &wxDateTime::Set, "C++: wxDateTime::Set(double) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("jdn"));
		cl.def("Set", [](wxDateTime &o, unsigned short const & a0) -> wxDateTime & { return o.Set(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("hour"));
		cl.def("Set", [](wxDateTime &o, unsigned short const & a0, unsigned short const & a1) -> wxDateTime & { return o.Set(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("hour"), pybind11::arg("minute"));
		cl.def("Set", [](wxDateTime &o, unsigned short const & a0, unsigned short const & a1, unsigned short const & a2) -> wxDateTime & { return o.Set(a0, a1, a2); }, "", pybind11::return_value_policy::automatic, pybind11::arg("hour"), pybind11::arg("minute"), pybind11::arg("second"));
		cl.def("Set", (class wxDateTime & (wxDateTime::*)(unsigned short, unsigned short, unsigned short, unsigned short)) &wxDateTime::Set, "C++: wxDateTime::Set(unsigned short, unsigned short, unsigned short, unsigned short) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("hour"), pybind11::arg("minute"), pybind11::arg("second"), pybind11::arg("millisec"));
		cl.def("Set", [](wxDateTime &o, unsigned short const & a0, enum wxDateTime::Month const & a1) -> wxDateTime & { return o.Set(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("day"), pybind11::arg("month"));
		cl.def("Set", [](wxDateTime &o, unsigned short const & a0, enum wxDateTime::Month const & a1, int const & a2) -> wxDateTime & { return o.Set(a0, a1, a2); }, "", pybind11::return_value_policy::automatic, pybind11::arg("day"), pybind11::arg("month"), pybind11::arg("year"));
		cl.def("Set", [](wxDateTime &o, unsigned short const & a0, enum wxDateTime::Month const & a1, int const & a2, unsigned short const & a3) -> wxDateTime & { return o.Set(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("day"), pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("hour"));
		cl.def("Set", [](wxDateTime &o, unsigned short const & a0, enum wxDateTime::Month const & a1, int const & a2, unsigned short const & a3, unsigned short const & a4) -> wxDateTime & { return o.Set(a0, a1, a2, a3, a4); }, "", pybind11::return_value_policy::automatic, pybind11::arg("day"), pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("hour"), pybind11::arg("minute"));
		cl.def("Set", [](wxDateTime &o, unsigned short const & a0, enum wxDateTime::Month const & a1, int const & a2, unsigned short const & a3, unsigned short const & a4, unsigned short const & a5) -> wxDateTime & { return o.Set(a0, a1, a2, a3, a4, a5); }, "", pybind11::return_value_policy::automatic, pybind11::arg("day"), pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("hour"), pybind11::arg("minute"), pybind11::arg("second"));
		cl.def("Set", (class wxDateTime & (wxDateTime::*)(unsigned short, enum wxDateTime::Month, int, unsigned short, unsigned short, unsigned short, unsigned short)) &wxDateTime::Set, "C++: wxDateTime::Set(unsigned short, enum wxDateTime::Month, int, unsigned short, unsigned short, unsigned short, unsigned short) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("day"), pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("hour"), pybind11::arg("minute"), pybind11::arg("second"), pybind11::arg("millisec"));
		cl.def("ResetTime", (class wxDateTime & (wxDateTime::*)()) &wxDateTime::ResetTime, "C++: wxDateTime::ResetTime() --> class wxDateTime &", pybind11::return_value_policy::automatic);
		cl.def("GetDateOnly", (class wxDateTime (wxDateTime::*)() const) &wxDateTime::GetDateOnly, "C++: wxDateTime::GetDateOnly() const --> class wxDateTime");
		cl.def("SetYear", (class wxDateTime & (wxDateTime::*)(int)) &wxDateTime::SetYear, "C++: wxDateTime::SetYear(int) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("year"));
		cl.def("SetMonth", (class wxDateTime & (wxDateTime::*)(enum wxDateTime::Month)) &wxDateTime::SetMonth, "C++: wxDateTime::SetMonth(enum wxDateTime::Month) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("month"));
		cl.def("SetDay", (class wxDateTime & (wxDateTime::*)(unsigned short)) &wxDateTime::SetDay, "C++: wxDateTime::SetDay(unsigned short) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("day"));
		cl.def("SetHour", (class wxDateTime & (wxDateTime::*)(unsigned short)) &wxDateTime::SetHour, "C++: wxDateTime::SetHour(unsigned short) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("hour"));
		cl.def("SetMinute", (class wxDateTime & (wxDateTime::*)(unsigned short)) &wxDateTime::SetMinute, "C++: wxDateTime::SetMinute(unsigned short) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("minute"));
		cl.def("SetSecond", (class wxDateTime & (wxDateTime::*)(unsigned short)) &wxDateTime::SetSecond, "C++: wxDateTime::SetSecond(unsigned short) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("second"));
		cl.def("SetMillisecond", (class wxDateTime & (wxDateTime::*)(unsigned short)) &wxDateTime::SetMillisecond, "C++: wxDateTime::SetMillisecond(unsigned short) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("millisecond"));
		cl.def("assign", (class wxDateTime & (wxDateTime::*)(long)) &wxDateTime::operator=, "C++: wxDateTime::operator=(long) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("timet"));
		cl.def("assign", (class wxDateTime & (wxDateTime::*)(const struct tm &)) &wxDateTime::operator=, "C++: wxDateTime::operator=(const struct tm &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("tm"));
		cl.def("assign", (class wxDateTime & (wxDateTime::*)(const struct wxDateTime::Tm &)) &wxDateTime::operator=, "C++: wxDateTime::operator=(const struct wxDateTime::Tm &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("tm"));
		cl.def("SetToWeekDayInSameWeek", [](wxDateTime &o, enum wxDateTime::WeekDay const & a0) -> wxDateTime & { return o.SetToWeekDayInSameWeek(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("weekday"));
		cl.def("SetToWeekDayInSameWeek", (class wxDateTime & (wxDateTime::*)(enum wxDateTime::WeekDay, enum wxDateTime::WeekFlags)) &wxDateTime::SetToWeekDayInSameWeek, "C++: wxDateTime::SetToWeekDayInSameWeek(enum wxDateTime::WeekDay, enum wxDateTime::WeekFlags) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("weekday"), pybind11::arg("flags"));
		cl.def("GetWeekDayInSameWeek", [](wxDateTime const &o, enum wxDateTime::WeekDay const & a0) -> wxDateTime { return o.GetWeekDayInSameWeek(a0); }, "", pybind11::arg("weekday"));
		cl.def("GetWeekDayInSameWeek", (class wxDateTime (wxDateTime::*)(enum wxDateTime::WeekDay, enum wxDateTime::WeekFlags) const) &wxDateTime::GetWeekDayInSameWeek, "C++: wxDateTime::GetWeekDayInSameWeek(enum wxDateTime::WeekDay, enum wxDateTime::WeekFlags) const --> class wxDateTime", pybind11::arg("weekday"), pybind11::arg("flags"));
		cl.def("SetToNextWeekDay", (class wxDateTime & (wxDateTime::*)(enum wxDateTime::WeekDay)) &wxDateTime::SetToNextWeekDay, "C++: wxDateTime::SetToNextWeekDay(enum wxDateTime::WeekDay) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("weekday"));
		cl.def("GetNextWeekDay", (class wxDateTime (wxDateTime::*)(enum wxDateTime::WeekDay) const) &wxDateTime::GetNextWeekDay, "C++: wxDateTime::GetNextWeekDay(enum wxDateTime::WeekDay) const --> class wxDateTime", pybind11::arg("weekday"));
		cl.def("SetToPrevWeekDay", (class wxDateTime & (wxDateTime::*)(enum wxDateTime::WeekDay)) &wxDateTime::SetToPrevWeekDay, "C++: wxDateTime::SetToPrevWeekDay(enum wxDateTime::WeekDay) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("weekday"));
		cl.def("GetPrevWeekDay", (class wxDateTime (wxDateTime::*)(enum wxDateTime::WeekDay) const) &wxDateTime::GetPrevWeekDay, "C++: wxDateTime::GetPrevWeekDay(enum wxDateTime::WeekDay) const --> class wxDateTime", pybind11::arg("weekday"));
		cl.def("SetToWeekDay", [](wxDateTime &o, enum wxDateTime::WeekDay const & a0) -> bool { return o.SetToWeekDay(a0); }, "", pybind11::arg("weekday"));
		cl.def("SetToWeekDay", [](wxDateTime &o, enum wxDateTime::WeekDay const & a0, int const & a1) -> bool { return o.SetToWeekDay(a0, a1); }, "", pybind11::arg("weekday"), pybind11::arg("n"));
		cl.def("SetToWeekDay", [](wxDateTime &o, enum wxDateTime::WeekDay const & a0, int const & a1, enum wxDateTime::Month const & a2) -> bool { return o.SetToWeekDay(a0, a1, a2); }, "", pybind11::arg("weekday"), pybind11::arg("n"), pybind11::arg("month"));
		cl.def("SetToWeekDay", (bool (wxDateTime::*)(enum wxDateTime::WeekDay, int, enum wxDateTime::Month, int)) &wxDateTime::SetToWeekDay, "C++: wxDateTime::SetToWeekDay(enum wxDateTime::WeekDay, int, enum wxDateTime::Month, int) --> bool", pybind11::arg("weekday"), pybind11::arg("n"), pybind11::arg("month"), pybind11::arg("year"));
		cl.def("GetWeekDay", [](wxDateTime const &o, enum wxDateTime::WeekDay const & a0) -> wxDateTime { return o.GetWeekDay(a0); }, "", pybind11::arg("weekday"));
		cl.def("GetWeekDay", [](wxDateTime const &o, enum wxDateTime::WeekDay const & a0, int const & a1) -> wxDateTime { return o.GetWeekDay(a0, a1); }, "", pybind11::arg("weekday"), pybind11::arg("n"));
		cl.def("GetWeekDay", [](wxDateTime const &o, enum wxDateTime::WeekDay const & a0, int const & a1, enum wxDateTime::Month const & a2) -> wxDateTime { return o.GetWeekDay(a0, a1, a2); }, "", pybind11::arg("weekday"), pybind11::arg("n"), pybind11::arg("month"));
		cl.def("GetWeekDay", (class wxDateTime (wxDateTime::*)(enum wxDateTime::WeekDay, int, enum wxDateTime::Month, int) const) &wxDateTime::GetWeekDay, "C++: wxDateTime::GetWeekDay(enum wxDateTime::WeekDay, int, enum wxDateTime::Month, int) const --> class wxDateTime", pybind11::arg("weekday"), pybind11::arg("n"), pybind11::arg("month"), pybind11::arg("year"));
		cl.def("SetToLastWeekDay", [](wxDateTime &o, enum wxDateTime::WeekDay const & a0) -> bool { return o.SetToLastWeekDay(a0); }, "", pybind11::arg("weekday"));
		cl.def("SetToLastWeekDay", [](wxDateTime &o, enum wxDateTime::WeekDay const & a0, enum wxDateTime::Month const & a1) -> bool { return o.SetToLastWeekDay(a0, a1); }, "", pybind11::arg("weekday"), pybind11::arg("month"));
		cl.def("SetToLastWeekDay", (bool (wxDateTime::*)(enum wxDateTime::WeekDay, enum wxDateTime::Month, int)) &wxDateTime::SetToLastWeekDay, "C++: wxDateTime::SetToLastWeekDay(enum wxDateTime::WeekDay, enum wxDateTime::Month, int) --> bool", pybind11::arg("weekday"), pybind11::arg("month"), pybind11::arg("year"));
		cl.def("GetLastWeekDay", [](wxDateTime &o, enum wxDateTime::WeekDay const & a0) -> wxDateTime { return o.GetLastWeekDay(a0); }, "", pybind11::arg("weekday"));
		cl.def("GetLastWeekDay", [](wxDateTime &o, enum wxDateTime::WeekDay const & a0, enum wxDateTime::Month const & a1) -> wxDateTime { return o.GetLastWeekDay(a0, a1); }, "", pybind11::arg("weekday"), pybind11::arg("month"));
		cl.def("GetLastWeekDay", (class wxDateTime (wxDateTime::*)(enum wxDateTime::WeekDay, enum wxDateTime::Month, int)) &wxDateTime::GetLastWeekDay, "C++: wxDateTime::GetLastWeekDay(enum wxDateTime::WeekDay, enum wxDateTime::Month, int) --> class wxDateTime", pybind11::arg("weekday"), pybind11::arg("month"), pybind11::arg("year"));
		cl.def_static("SetToWeekOfYear", [](int const & a0, unsigned short const & a1) -> wxDateTime { return wxDateTime::SetToWeekOfYear(a0, a1); }, "", pybind11::arg("year"), pybind11::arg("numWeek"));
		cl.def_static("SetToWeekOfYear", (class wxDateTime (*)(int, unsigned short, enum wxDateTime::WeekDay)) &wxDateTime::SetToWeekOfYear, "C++: wxDateTime::SetToWeekOfYear(int, unsigned short, enum wxDateTime::WeekDay) --> class wxDateTime", pybind11::arg("year"), pybind11::arg("numWeek"), pybind11::arg("weekday"));
		cl.def("SetToLastMonthDay", [](wxDateTime &o) -> wxDateTime & { return o.SetToLastMonthDay(); }, "", pybind11::return_value_policy::automatic);
		cl.def("SetToLastMonthDay", [](wxDateTime &o, enum wxDateTime::Month const & a0) -> wxDateTime & { return o.SetToLastMonthDay(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("month"));
		cl.def("SetToLastMonthDay", (class wxDateTime & (wxDateTime::*)(enum wxDateTime::Month, int)) &wxDateTime::SetToLastMonthDay, "C++: wxDateTime::SetToLastMonthDay(enum wxDateTime::Month, int) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("month"), pybind11::arg("year"));
		cl.def("GetLastMonthDay", [](wxDateTime const &o) -> wxDateTime { return o.GetLastMonthDay(); }, "");
		cl.def("GetLastMonthDay", [](wxDateTime const &o, enum wxDateTime::Month const & a0) -> wxDateTime { return o.GetLastMonthDay(a0); }, "", pybind11::arg("month"));
		cl.def("GetLastMonthDay", (class wxDateTime (wxDateTime::*)(enum wxDateTime::Month, int) const) &wxDateTime::GetLastMonthDay, "C++: wxDateTime::GetLastMonthDay(enum wxDateTime::Month, int) const --> class wxDateTime", pybind11::arg("month"), pybind11::arg("year"));
		cl.def("SetToYearDay", (class wxDateTime & (wxDateTime::*)(unsigned short)) &wxDateTime::SetToYearDay, "C++: wxDateTime::SetToYearDay(unsigned short) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("yday"));
		cl.def("GetYearDay", (class wxDateTime (wxDateTime::*)(unsigned short) const) &wxDateTime::GetYearDay, "C++: wxDateTime::GetYearDay(unsigned short) const --> class wxDateTime", pybind11::arg("yday"));
		cl.def("GetJulianDayNumber", (double (wxDateTime::*)() const) &wxDateTime::GetJulianDayNumber, "C++: wxDateTime::GetJulianDayNumber() const --> double");
		cl.def("GetJDN", (double (wxDateTime::*)() const) &wxDateTime::GetJDN, "C++: wxDateTime::GetJDN() const --> double");
		cl.def("GetModifiedJulianDayNumber", (double (wxDateTime::*)() const) &wxDateTime::GetModifiedJulianDayNumber, "C++: wxDateTime::GetModifiedJulianDayNumber() const --> double");
		cl.def("GetMJD", (double (wxDateTime::*)() const) &wxDateTime::GetMJD, "C++: wxDateTime::GetMJD() const --> double");
		cl.def("GetRataDie", (double (wxDateTime::*)() const) &wxDateTime::GetRataDie, "C++: wxDateTime::GetRataDie() const --> double");
		cl.def("ToTimezone", [](wxDateTime const &o, const class wxDateTime::TimeZone & a0) -> wxDateTime { return o.ToTimezone(a0); }, "", pybind11::arg("tz"));
		cl.def("ToTimezone", (class wxDateTime (wxDateTime::*)(const class wxDateTime::TimeZone &, bool) const) &wxDateTime::ToTimezone, "C++: wxDateTime::ToTimezone(const class wxDateTime::TimeZone &, bool) const --> class wxDateTime", pybind11::arg("tz"), pybind11::arg("noDST"));
		cl.def("MakeTimezone", [](wxDateTime &o, const class wxDateTime::TimeZone & a0) -> wxDateTime & { return o.MakeTimezone(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("tz"));
		cl.def("MakeTimezone", (class wxDateTime & (wxDateTime::*)(const class wxDateTime::TimeZone &, bool)) &wxDateTime::MakeTimezone, "C++: wxDateTime::MakeTimezone(const class wxDateTime::TimeZone &, bool) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("tz"), pybind11::arg("noDST"));
		cl.def("FromTimezone", [](wxDateTime const &o, const class wxDateTime::TimeZone & a0) -> wxDateTime { return o.FromTimezone(a0); }, "", pybind11::arg("tz"));
		cl.def("FromTimezone", (class wxDateTime (wxDateTime::*)(const class wxDateTime::TimeZone &, bool) const) &wxDateTime::FromTimezone, "C++: wxDateTime::FromTimezone(const class wxDateTime::TimeZone &, bool) const --> class wxDateTime", pybind11::arg("tz"), pybind11::arg("noDST"));
		cl.def("MakeFromTimezone", [](wxDateTime &o, const class wxDateTime::TimeZone & a0) -> wxDateTime & { return o.MakeFromTimezone(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("tz"));
		cl.def("MakeFromTimezone", (class wxDateTime & (wxDateTime::*)(const class wxDateTime::TimeZone &, bool)) &wxDateTime::MakeFromTimezone, "C++: wxDateTime::MakeFromTimezone(const class wxDateTime::TimeZone &, bool) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("tz"), pybind11::arg("noDST"));
		cl.def("ToUTC", [](wxDateTime const &o) -> wxDateTime { return o.ToUTC(); }, "");
		cl.def("ToUTC", (class wxDateTime (wxDateTime::*)(bool) const) &wxDateTime::ToUTC, "C++: wxDateTime::ToUTC(bool) const --> class wxDateTime", pybind11::arg("noDST"));
		cl.def("MakeUTC", [](wxDateTime &o) -> wxDateTime & { return o.MakeUTC(); }, "", pybind11::return_value_policy::automatic);
		cl.def("MakeUTC", (class wxDateTime & (wxDateTime::*)(bool)) &wxDateTime::MakeUTC, "C++: wxDateTime::MakeUTC(bool) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("noDST"));
		cl.def("ToGMT", [](wxDateTime const &o) -> wxDateTime { return o.ToGMT(); }, "");
		cl.def("ToGMT", (class wxDateTime (wxDateTime::*)(bool) const) &wxDateTime::ToGMT, "C++: wxDateTime::ToGMT(bool) const --> class wxDateTime", pybind11::arg("noDST"));
		cl.def("MakeGMT", [](wxDateTime &o) -> wxDateTime & { return o.MakeGMT(); }, "", pybind11::return_value_policy::automatic);
		cl.def("MakeGMT", (class wxDateTime & (wxDateTime::*)(bool)) &wxDateTime::MakeGMT, "C++: wxDateTime::MakeGMT(bool) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("noDST"));
		cl.def("FromUTC", [](wxDateTime const &o) -> wxDateTime { return o.FromUTC(); }, "");
		cl.def("FromUTC", (class wxDateTime (wxDateTime::*)(bool) const) &wxDateTime::FromUTC, "C++: wxDateTime::FromUTC(bool) const --> class wxDateTime", pybind11::arg("noDST"));
		cl.def("MakeFromUTC", [](wxDateTime &o) -> wxDateTime & { return o.MakeFromUTC(); }, "", pybind11::return_value_policy::automatic);
		cl.def("MakeFromUTC", (class wxDateTime & (wxDateTime::*)(bool)) &wxDateTime::MakeFromUTC, "C++: wxDateTime::MakeFromUTC(bool) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("noDST"));
		cl.def("IsDST", [](wxDateTime const &o) -> int { return o.IsDST(); }, "");
		cl.def("IsDST", (int (wxDateTime::*)(enum wxDateTime::Country) const) &wxDateTime::IsDST, "C++: wxDateTime::IsDST(enum wxDateTime::Country) const --> int", pybind11::arg("country"));
		cl.def("IsValid", (bool (wxDateTime::*)() const) &wxDateTime::IsValid, "C++: wxDateTime::IsValid() const --> bool");
		cl.def("GetTm", [](wxDateTime const &o) -> wxDateTime::Tm { return o.GetTm(); }, "");
		cl.def("GetTm", (struct wxDateTime::Tm (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetTm, "C++: wxDateTime::GetTm(const class wxDateTime::TimeZone &) const --> struct wxDateTime::Tm", pybind11::arg("tz"));
		cl.def("GetTicks", (long (wxDateTime::*)() const) &wxDateTime::GetTicks, "C++: wxDateTime::GetTicks() const --> long");
		cl.def("GetCentury", [](wxDateTime const &o) -> int { return o.GetCentury(); }, "");
		cl.def("GetCentury", (int (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetCentury, "C++: wxDateTime::GetCentury(const class wxDateTime::TimeZone &) const --> int", pybind11::arg("tz"));
		cl.def("GetYear", [](wxDateTime const &o) -> int { return o.GetYear(); }, "");
		cl.def("GetYear", (int (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetYear, "C++: wxDateTime::GetYear(const class wxDateTime::TimeZone &) const --> int", pybind11::arg("tz"));
		cl.def("GetMonth", [](wxDateTime const &o) -> wxDateTime::Month { return o.GetMonth(); }, "");
		cl.def("GetMonth", (enum wxDateTime::Month (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetMonth, "C++: wxDateTime::GetMonth(const class wxDateTime::TimeZone &) const --> enum wxDateTime::Month", pybind11::arg("tz"));
		cl.def("GetDay", [](wxDateTime const &o) -> unsigned short { return o.GetDay(); }, "");
		cl.def("GetDay", (unsigned short (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetDay, "C++: wxDateTime::GetDay(const class wxDateTime::TimeZone &) const --> unsigned short", pybind11::arg("tz"));
		cl.def("GetWeekDay", [](wxDateTime const &o) -> wxDateTime::WeekDay { return o.GetWeekDay(); }, "");
		cl.def("GetWeekDay", (enum wxDateTime::WeekDay (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetWeekDay, "C++: wxDateTime::GetWeekDay(const class wxDateTime::TimeZone &) const --> enum wxDateTime::WeekDay", pybind11::arg("tz"));
		cl.def("GetHour", [](wxDateTime const &o) -> unsigned short { return o.GetHour(); }, "");
		cl.def("GetHour", (unsigned short (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetHour, "C++: wxDateTime::GetHour(const class wxDateTime::TimeZone &) const --> unsigned short", pybind11::arg("tz"));
		cl.def("GetMinute", [](wxDateTime const &o) -> unsigned short { return o.GetMinute(); }, "");
		cl.def("GetMinute", (unsigned short (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetMinute, "C++: wxDateTime::GetMinute(const class wxDateTime::TimeZone &) const --> unsigned short", pybind11::arg("tz"));
		cl.def("GetSecond", [](wxDateTime const &o) -> unsigned short { return o.GetSecond(); }, "");
		cl.def("GetSecond", (unsigned short (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetSecond, "C++: wxDateTime::GetSecond(const class wxDateTime::TimeZone &) const --> unsigned short", pybind11::arg("tz"));
		cl.def("GetMillisecond", [](wxDateTime const &o) -> unsigned short { return o.GetMillisecond(); }, "");
		cl.def("GetMillisecond", (unsigned short (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetMillisecond, "C++: wxDateTime::GetMillisecond(const class wxDateTime::TimeZone &) const --> unsigned short", pybind11::arg("tz"));
		cl.def("GetDayOfYear", [](wxDateTime const &o) -> unsigned short { return o.GetDayOfYear(); }, "");
		cl.def("GetDayOfYear", (unsigned short (wxDateTime::*)(const class wxDateTime::TimeZone &) const) &wxDateTime::GetDayOfYear, "C++: wxDateTime::GetDayOfYear(const class wxDateTime::TimeZone &) const --> unsigned short", pybind11::arg("tz"));
		cl.def("GetWeekOfYear", [](wxDateTime const &o) -> unsigned short { return o.GetWeekOfYear(); }, "");
		cl.def("GetWeekOfYear", [](wxDateTime const &o, enum wxDateTime::WeekFlags const & a0) -> unsigned short { return o.GetWeekOfYear(a0); }, "", pybind11::arg("flags"));
		cl.def("GetWeekOfYear", (unsigned short (wxDateTime::*)(enum wxDateTime::WeekFlags, const class wxDateTime::TimeZone &) const) &wxDateTime::GetWeekOfYear, "C++: wxDateTime::GetWeekOfYear(enum wxDateTime::WeekFlags, const class wxDateTime::TimeZone &) const --> unsigned short", pybind11::arg("flags"), pybind11::arg("tz"));
		cl.def("GetWeekOfMonth", [](wxDateTime const &o) -> unsigned short { return o.GetWeekOfMonth(); }, "");
		cl.def("GetWeekOfMonth", [](wxDateTime const &o, enum wxDateTime::WeekFlags const & a0) -> unsigned short { return o.GetWeekOfMonth(a0); }, "", pybind11::arg("flags"));
		cl.def("GetWeekOfMonth", (unsigned short (wxDateTime::*)(enum wxDateTime::WeekFlags, const class wxDateTime::TimeZone &) const) &wxDateTime::GetWeekOfMonth, "C++: wxDateTime::GetWeekOfMonth(enum wxDateTime::WeekFlags, const class wxDateTime::TimeZone &) const --> unsigned short", pybind11::arg("flags"), pybind11::arg("tz"));
		cl.def("IsWorkDay", [](wxDateTime const &o) -> bool { return o.IsWorkDay(); }, "");
		cl.def("IsWorkDay", (bool (wxDateTime::*)(enum wxDateTime::Country) const) &wxDateTime::IsWorkDay, "C++: wxDateTime::IsWorkDay(enum wxDateTime::Country) const --> bool", pybind11::arg("country"));
		cl.def("SetFromDOS", (class wxDateTime & (wxDateTime::*)(unsigned long)) &wxDateTime::SetFromDOS, "C++: wxDateTime::SetFromDOS(unsigned long) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("ddt"));
		cl.def("GetAsDOS", (unsigned long (wxDateTime::*)() const) &wxDateTime::GetAsDOS, "C++: wxDateTime::GetAsDOS() const --> unsigned long");
		cl.def("IsEqualTo", (bool (wxDateTime::*)(const class wxDateTime &) const) &wxDateTime::IsEqualTo, "C++: wxDateTime::IsEqualTo(const class wxDateTime &) const --> bool", pybind11::arg("datetime"));
		cl.def("IsEarlierThan", (bool (wxDateTime::*)(const class wxDateTime &) const) &wxDateTime::IsEarlierThan, "C++: wxDateTime::IsEarlierThan(const class wxDateTime &) const --> bool", pybind11::arg("datetime"));
		cl.def("IsLaterThan", (bool (wxDateTime::*)(const class wxDateTime &) const) &wxDateTime::IsLaterThan, "C++: wxDateTime::IsLaterThan(const class wxDateTime &) const --> bool", pybind11::arg("datetime"));
		cl.def("IsStrictlyBetween", (bool (wxDateTime::*)(const class wxDateTime &, const class wxDateTime &) const) &wxDateTime::IsStrictlyBetween, "C++: wxDateTime::IsStrictlyBetween(const class wxDateTime &, const class wxDateTime &) const --> bool", pybind11::arg("t1"), pybind11::arg("t2"));
		cl.def("IsBetween", (bool (wxDateTime::*)(const class wxDateTime &, const class wxDateTime &) const) &wxDateTime::IsBetween, "C++: wxDateTime::IsBetween(const class wxDateTime &, const class wxDateTime &) const --> bool", pybind11::arg("t1"), pybind11::arg("t2"));
		cl.def("IsSameDate", (bool (wxDateTime::*)(const class wxDateTime &) const) &wxDateTime::IsSameDate, "C++: wxDateTime::IsSameDate(const class wxDateTime &) const --> bool", pybind11::arg("dt"));
		cl.def("IsSameTime", (bool (wxDateTime::*)(const class wxDateTime &) const) &wxDateTime::IsSameTime, "C++: wxDateTime::IsSameTime(const class wxDateTime &) const --> bool", pybind11::arg("dt"));
		cl.def("IsEqualUpTo", (bool (wxDateTime::*)(const class wxDateTime &, const class wxTimeSpan &) const) &wxDateTime::IsEqualUpTo, "C++: wxDateTime::IsEqualUpTo(const class wxDateTime &, const class wxTimeSpan &) const --> bool", pybind11::arg("dt"), pybind11::arg("ts"));
		cl.def("__eq__", (bool (wxDateTime::*)(const class wxDateTime &) const) &wxDateTime::operator==, "C++: wxDateTime::operator==(const class wxDateTime &) const --> bool", pybind11::arg("dt"));
		cl.def("__ne__", (bool (wxDateTime::*)(const class wxDateTime &) const) &wxDateTime::operator!=, "C++: wxDateTime::operator!=(const class wxDateTime &) const --> bool", pybind11::arg("dt"));
		cl.def("Add", (class wxDateTime & (wxDateTime::*)(const class wxTimeSpan &)) &wxDateTime::Add, "C++: wxDateTime::Add(const class wxTimeSpan &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__iadd__", (class wxDateTime & (wxDateTime::*)(const class wxTimeSpan &)) &wxDateTime::operator+=, "C++: wxDateTime::operator+=(const class wxTimeSpan &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__add__", (class wxDateTime (wxDateTime::*)(const class wxTimeSpan &) const) &wxDateTime::operator+, "C++: wxDateTime::operator+(const class wxTimeSpan &) const --> class wxDateTime", pybind11::arg("ts"));
		cl.def("Subtract", (class wxDateTime & (wxDateTime::*)(const class wxTimeSpan &)) &wxDateTime::Subtract, "C++: wxDateTime::Subtract(const class wxTimeSpan &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__isub__", (class wxDateTime & (wxDateTime::*)(const class wxTimeSpan &)) &wxDateTime::operator-=, "C++: wxDateTime::operator-=(const class wxTimeSpan &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__sub__", (class wxDateTime (wxDateTime::*)(const class wxTimeSpan &) const) &wxDateTime::operator-, "C++: wxDateTime::operator-(const class wxTimeSpan &) const --> class wxDateTime", pybind11::arg("ts"));
		cl.def("Add", (class wxDateTime & (wxDateTime::*)(const class wxDateSpan &)) &wxDateTime::Add, "C++: wxDateTime::Add(const class wxDateSpan &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__iadd__", (class wxDateTime & (wxDateTime::*)(const class wxDateSpan &)) &wxDateTime::operator+=, "C++: wxDateTime::operator+=(const class wxDateSpan &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__add__", (class wxDateTime (wxDateTime::*)(const class wxDateSpan &) const) &wxDateTime::operator+, "C++: wxDateTime::operator+(const class wxDateSpan &) const --> class wxDateTime", pybind11::arg("ds"));
		cl.def("Subtract", (class wxDateTime & (wxDateTime::*)(const class wxDateSpan &)) &wxDateTime::Subtract, "C++: wxDateTime::Subtract(const class wxDateSpan &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__isub__", (class wxDateTime & (wxDateTime::*)(const class wxDateSpan &)) &wxDateTime::operator-=, "C++: wxDateTime::operator-=(const class wxDateSpan &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__sub__", (class wxDateTime (wxDateTime::*)(const class wxDateSpan &) const) &wxDateTime::operator-, "C++: wxDateTime::operator-(const class wxDateSpan &) const --> class wxDateTime", pybind11::arg("ds"));
		cl.def("Subtract", (class wxTimeSpan (wxDateTime::*)(const class wxDateTime &) const) &wxDateTime::Subtract, "C++: wxDateTime::Subtract(const class wxDateTime &) const --> class wxTimeSpan", pybind11::arg("dt"));
		cl.def("__sub__", (class wxTimeSpan (wxDateTime::*)(const class wxDateTime &) const) &wxDateTime::operator-, "C++: wxDateTime::operator-(const class wxDateTime &) const --> class wxTimeSpan", pybind11::arg("dt2"));
		cl.def("DiffAsDateSpan", (class wxDateSpan (wxDateTime::*)(const class wxDateTime &) const) &wxDateTime::DiffAsDateSpan, "C++: wxDateTime::DiffAsDateSpan(const class wxDateTime &) const --> class wxDateSpan", pybind11::arg("dt"));
		cl.def("ParseRfc822Date", (bool (wxDateTime::*)(const class wxString &, class wxString::const_iterator *)) &wxDateTime::ParseRfc822Date, "C++: wxDateTime::ParseRfc822Date(const class wxString &, class wxString::const_iterator *) --> bool", pybind11::arg("date"), pybind11::arg("end"));
		cl.def("ParseFormat", (bool (wxDateTime::*)(const class wxString &, const class wxString &, const class wxDateTime &, class wxString::const_iterator *)) &wxDateTime::ParseFormat, "C++: wxDateTime::ParseFormat(const class wxString &, const class wxString &, const class wxDateTime &, class wxString::const_iterator *) --> bool", pybind11::arg("date"), pybind11::arg("format"), pybind11::arg("dateDef"), pybind11::arg("end"));
		cl.def("ParseFormat", (bool (wxDateTime::*)(const class wxString &, const class wxString &, class wxString::const_iterator *)) &wxDateTime::ParseFormat, "C++: wxDateTime::ParseFormat(const class wxString &, const class wxString &, class wxString::const_iterator *) --> bool", pybind11::arg("date"), pybind11::arg("format"), pybind11::arg("end"));
		cl.def("ParseFormat", (bool (wxDateTime::*)(const class wxString &, class wxString::const_iterator *)) &wxDateTime::ParseFormat, "C++: wxDateTime::ParseFormat(const class wxString &, class wxString::const_iterator *) --> bool", pybind11::arg("date"), pybind11::arg("end"));
		cl.def("ParseISODate", (bool (wxDateTime::*)(const class wxString &)) &wxDateTime::ParseISODate, "C++: wxDateTime::ParseISODate(const class wxString &) --> bool", pybind11::arg("date"));
		cl.def("ParseISOTime", (bool (wxDateTime::*)(const class wxString &)) &wxDateTime::ParseISOTime, "C++: wxDateTime::ParseISOTime(const class wxString &) --> bool", pybind11::arg("time"));
		cl.def("ParseISOCombined", [](wxDateTime &o, const class wxString & a0) -> bool { return o.ParseISOCombined(a0); }, "", pybind11::arg("datetime"));
		cl.def("ParseISOCombined", (bool (wxDateTime::*)(const class wxString &, char)) &wxDateTime::ParseISOCombined, "C++: wxDateTime::ParseISOCombined(const class wxString &, char) --> bool", pybind11::arg("datetime"), pybind11::arg("sep"));
		cl.def("ParseDateTime", (bool (wxDateTime::*)(const class wxString &, class wxString::const_iterator *)) &wxDateTime::ParseDateTime, "C++: wxDateTime::ParseDateTime(const class wxString &, class wxString::const_iterator *) --> bool", pybind11::arg("datetime"), pybind11::arg("end"));
		cl.def("ParseDate", (bool (wxDateTime::*)(const class wxString &, class wxString::const_iterator *)) &wxDateTime::ParseDate, "C++: wxDateTime::ParseDate(const class wxString &, class wxString::const_iterator *) --> bool", pybind11::arg("date"), pybind11::arg("end"));
		cl.def("ParseTime", (bool (wxDateTime::*)(const class wxString &, class wxString::const_iterator *)) &wxDateTime::ParseTime, "C++: wxDateTime::ParseTime(const class wxString &, class wxString::const_iterator *) --> bool", pybind11::arg("time"), pybind11::arg("end"));
		cl.def("Format", [](wxDateTime const &o) -> wxString { return o.Format(); }, "");
		cl.def("Format", [](wxDateTime const &o, const class wxString & a0) -> wxString { return o.Format(a0); }, "", pybind11::arg("format"));
		cl.def("Format", (class wxString (wxDateTime::*)(const class wxString &, const class wxDateTime::TimeZone &) const) &wxDateTime::Format, "C++: wxDateTime::Format(const class wxString &, const class wxDateTime::TimeZone &) const --> class wxString", pybind11::arg("format"), pybind11::arg("tz"));
		cl.def("FormatDate", (class wxString (wxDateTime::*)() const) &wxDateTime::FormatDate, "C++: wxDateTime::FormatDate() const --> class wxString");
		cl.def("FormatTime", (class wxString (wxDateTime::*)() const) &wxDateTime::FormatTime, "C++: wxDateTime::FormatTime() const --> class wxString");
		cl.def("FormatISODate", (class wxString (wxDateTime::*)() const) &wxDateTime::FormatISODate, "C++: wxDateTime::FormatISODate() const --> class wxString");
		cl.def("FormatISOTime", (class wxString (wxDateTime::*)() const) &wxDateTime::FormatISOTime, "C++: wxDateTime::FormatISOTime() const --> class wxString");
		cl.def("FormatISOCombined", [](wxDateTime const &o) -> wxString { return o.FormatISOCombined(); }, "");
		cl.def("FormatISOCombined", (class wxString (wxDateTime::*)(char) const) &wxDateTime::FormatISOCombined, "C++: wxDateTime::FormatISOCombined(char) const --> class wxString", pybind11::arg("sep"));
		cl.def("ParseRfc822Date", (class wxAnyStrPtr (wxDateTime::*)(const class wxString &)) &wxDateTime::ParseRfc822Date, "C++: wxDateTime::ParseRfc822Date(const class wxString &) --> class wxAnyStrPtr", pybind11::arg("date"));
		cl.def("ParseFormat", [](wxDateTime &o, const class wxString & a0) -> wxAnyStrPtr { return o.ParseFormat(a0); }, "", pybind11::arg("date"));
		cl.def("ParseFormat", [](wxDateTime &o, const class wxString & a0, const class wxString & a1) -> wxAnyStrPtr { return o.ParseFormat(a0, a1); }, "", pybind11::arg("date"), pybind11::arg("format"));
		cl.def("ParseFormat", (class wxAnyStrPtr (wxDateTime::*)(const class wxString &, const class wxString &, const class wxDateTime &)) &wxDateTime::ParseFormat, "C++: wxDateTime::ParseFormat(const class wxString &, const class wxString &, const class wxDateTime &) --> class wxAnyStrPtr", pybind11::arg("date"), pybind11::arg("format"), pybind11::arg("dateDef"));
		cl.def("ParseDateTime", (class wxAnyStrPtr (wxDateTime::*)(const class wxString &)) &wxDateTime::ParseDateTime, "C++: wxDateTime::ParseDateTime(const class wxString &) --> class wxAnyStrPtr", pybind11::arg("datetime"));
		cl.def("ParseDate", (class wxAnyStrPtr (wxDateTime::*)(const class wxString &)) &wxDateTime::ParseDate, "C++: wxDateTime::ParseDate(const class wxString &) --> class wxAnyStrPtr", pybind11::arg("date"));
		cl.def("ParseTime", (class wxAnyStrPtr (wxDateTime::*)(const class wxString &)) &wxDateTime::ParseTime, "C++: wxDateTime::ParseTime(const class wxString &) --> class wxAnyStrPtr", pybind11::arg("time"));
		cl.def("ParseRfc822Date", (void (wxDateTime::*)(const class wxCStrData &)) &wxDateTime::ParseRfc822Date, "C++: wxDateTime::ParseRfc822Date(const class wxCStrData &) --> void", pybind11::arg("date"));
		cl.def("ParseRfc822Date", (const char * (wxDateTime::*)(const char *)) &wxDateTime::ParseRfc822Date, "C++: wxDateTime::ParseRfc822Date(const char *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("date"));
		cl.def("ParseRfc822Date", (const wchar_t * (wxDateTime::*)(const wchar_t *)) &wxDateTime::ParseRfc822Date, "C++: wxDateTime::ParseRfc822Date(const wchar_t *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("date"));
		cl.def("ParseFormat", [](wxDateTime &o, const class wxCStrData & a0) -> void { return o.ParseFormat(a0); }, "", pybind11::arg("date"));
		cl.def("ParseFormat", [](wxDateTime &o, const class wxCStrData & a0, const class wxString & a1) -> void { return o.ParseFormat(a0, a1); }, "", pybind11::arg("date"), pybind11::arg("format"));
		cl.def("ParseFormat", (void (wxDateTime::*)(const class wxCStrData &, const class wxString &, const class wxDateTime &)) &wxDateTime::ParseFormat, "C++: wxDateTime::ParseFormat(const class wxCStrData &, const class wxString &, const class wxDateTime &) --> void", pybind11::arg("date"), pybind11::arg("format"), pybind11::arg("dateDef"));
		cl.def("ParseFormat", [](wxDateTime &o, const char * a0) -> const char * { return o.ParseFormat(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("date"));
		cl.def("ParseFormat", [](wxDateTime &o, const char * a0, const class wxString & a1) -> const char * { return o.ParseFormat(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("date"), pybind11::arg("format"));
		cl.def("ParseFormat", (const char * (wxDateTime::*)(const char *, const class wxString &, const class wxDateTime &)) &wxDateTime::ParseFormat, "C++: wxDateTime::ParseFormat(const char *, const class wxString &, const class wxDateTime &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("date"), pybind11::arg("format"), pybind11::arg("dateDef"));
		cl.def("ParseFormat", [](wxDateTime &o, const wchar_t * a0) -> const wchar_t * { return o.ParseFormat(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("date"));
		cl.def("ParseFormat", [](wxDateTime &o, const wchar_t * a0, const class wxString & a1) -> const wchar_t * { return o.ParseFormat(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("date"), pybind11::arg("format"));
		cl.def("ParseFormat", (const wchar_t * (wxDateTime::*)(const wchar_t *, const class wxString &, const class wxDateTime &)) &wxDateTime::ParseFormat, "C++: wxDateTime::ParseFormat(const wchar_t *, const class wxString &, const class wxDateTime &) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("date"), pybind11::arg("format"), pybind11::arg("dateDef"));
		cl.def("ParseDateTime", (void (wxDateTime::*)(const class wxCStrData &)) &wxDateTime::ParseDateTime, "C++: wxDateTime::ParseDateTime(const class wxCStrData &) --> void", pybind11::arg("datetime"));
		cl.def("ParseDateTime", (const char * (wxDateTime::*)(const char *)) &wxDateTime::ParseDateTime, "C++: wxDateTime::ParseDateTime(const char *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("datetime"));
		cl.def("ParseDateTime", (const wchar_t * (wxDateTime::*)(const wchar_t *)) &wxDateTime::ParseDateTime, "C++: wxDateTime::ParseDateTime(const wchar_t *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("datetime"));
		cl.def("ParseDate", (void (wxDateTime::*)(const class wxCStrData &)) &wxDateTime::ParseDate, "C++: wxDateTime::ParseDate(const class wxCStrData &) --> void", pybind11::arg("date"));
		cl.def("ParseDate", (const char * (wxDateTime::*)(const char *)) &wxDateTime::ParseDate, "C++: wxDateTime::ParseDate(const char *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("date"));
		cl.def("ParseDate", (const wchar_t * (wxDateTime::*)(const wchar_t *)) &wxDateTime::ParseDate, "C++: wxDateTime::ParseDate(const wchar_t *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("date"));
		cl.def("ParseTime", (void (wxDateTime::*)(const class wxCStrData &)) &wxDateTime::ParseTime, "C++: wxDateTime::ParseTime(const class wxCStrData &) --> void", pybind11::arg("time"));
		cl.def("ParseTime", (const char * (wxDateTime::*)(const char *)) &wxDateTime::ParseTime, "C++: wxDateTime::ParseTime(const char *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("time"));
		cl.def("ParseTime", (const wchar_t * (wxDateTime::*)(const wchar_t *)) &wxDateTime::ParseTime, "C++: wxDateTime::ParseTime(const wchar_t *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("time"));
		cl.def("GetValue", (class wxLongLongNative (wxDateTime::*)() const) &wxDateTime::GetValue, "C++: wxDateTime::GetValue() const --> class wxLongLongNative");
		cl.def_static("GetTimeNow", (long (*)()) &wxDateTime::GetTimeNow, "C++: wxDateTime::GetTimeNow() --> long");
		cl.def_static("GetTmNow", (struct tm * (*)()) &wxDateTime::GetTmNow, "C++: wxDateTime::GetTmNow() --> struct tm *", pybind11::return_value_policy::automatic);
		cl.def_static("GetTmNow", (struct tm * (*)(struct tm *)) &wxDateTime::GetTmNow, "C++: wxDateTime::GetTmNow(struct tm *) --> struct tm *", pybind11::return_value_policy::automatic, pybind11::arg("tmstruct"));
		cl.def("assign", (class wxDateTime & (wxDateTime::*)(const class wxDateTime &)) &wxDateTime::operator=, "C++: wxDateTime::operator=(const class wxDateTime &) --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxDateTime::TimeZone file: line:294
			auto & enclosing_class = cl;
			pybind11::class_<wxDateTime::TimeZone, std::shared_ptr<wxDateTime::TimeZone>> cl(enclosing_class, "TimeZone", "");
			cl.def( pybind11::init<enum wxDateTime::TZ>(), pybind11::arg("tz") );

			cl.def( pybind11::init( [](){ return new wxDateTime::TimeZone(); } ), "doc" );
			cl.def( pybind11::init<long>(), pybind11::arg("offset") );

			cl.def( pybind11::init( [](wxDateTime::TimeZone const &o){ return new wxDateTime::TimeZone(o); } ) );
			cl.def_static("Make", (class wxDateTime::TimeZone (*)(long)) &wxDateTime::TimeZone::Make, "C++: wxDateTime::TimeZone::Make(long) --> class wxDateTime::TimeZone", pybind11::arg("offset"));
			cl.def("GetOffset", (long (wxDateTime::TimeZone::*)() const) &wxDateTime::TimeZone::GetOffset, "C++: wxDateTime::TimeZone::GetOffset() const --> long");
		}

		{ // wxDateTime::Tm file: line:323
			auto & enclosing_class = cl;
			pybind11::class_<wxDateTime::Tm, std::shared_ptr<wxDateTime::Tm>> cl(enclosing_class, "Tm", "");
			cl.def( pybind11::init( [](){ return new wxDateTime::Tm(); } ) );
			cl.def( pybind11::init<const struct tm &, const class wxDateTime::TimeZone &>(), pybind11::arg("tm"), pybind11::arg("tz") );

			cl.def( pybind11::init( [](wxDateTime::Tm const &o){ return new wxDateTime::Tm(o); } ) );
			cl.def_readwrite("msec", &wxDateTime::Tm::msec);
			cl.def_readwrite("sec", &wxDateTime::Tm::sec);
			cl.def_readwrite("min", &wxDateTime::Tm::min);
			cl.def_readwrite("hour", &wxDateTime::Tm::hour);
			cl.def_readwrite("mday", &wxDateTime::Tm::mday);
			cl.def_readwrite("yday", &wxDateTime::Tm::yday);
			cl.def_readwrite("mon", &wxDateTime::Tm::mon);
			cl.def_readwrite("year", &wxDateTime::Tm::year);
			cl.def("IsValid", (bool (wxDateTime::Tm::*)() const) &wxDateTime::Tm::IsValid, "C++: wxDateTime::Tm::IsValid() const --> bool");
			cl.def("GetWeekDay", (enum wxDateTime::WeekDay (wxDateTime::Tm::*)()) &wxDateTime::Tm::GetWeekDay, "C++: wxDateTime::Tm::GetWeekDay() --> enum wxDateTime::WeekDay");
			cl.def("AddMonths", (void (wxDateTime::Tm::*)(int)) &wxDateTime::Tm::AddMonths, "C++: wxDateTime::Tm::AddMonths(int) --> void", pybind11::arg("monDiff"));
			cl.def("AddDays", (void (wxDateTime::Tm::*)(int)) &wxDateTime::Tm::AddDays, "C++: wxDateTime::Tm::AddDays(int) --> void", pybind11::arg("dayDiff"));
		}

	}
}
