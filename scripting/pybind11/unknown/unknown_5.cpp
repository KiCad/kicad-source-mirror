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

// wxMBConvLibc file: line:183
struct PyCallBack_wxMBConvLibc : public wxMBConvLibc {
	using wxMBConvLibc::wxMBConvLibc;

	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvLibc *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvLibc::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvLibc *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvLibc::WC2MB(a0, a1, a2);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvLibc *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxMBConvLibc::Clone();
	}
	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvLibc *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvLibc *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::FromWChar(a0, a1, a2, a3);
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvLibc *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::GetMBNulLen();
	}
};

// wxConvBrokenFileNames file: line:205
struct PyCallBack_wxConvBrokenFileNames : public wxConvBrokenFileNames {
	using wxConvBrokenFileNames::wxConvBrokenFileNames;

	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvBrokenFileNames *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxConvBrokenFileNames::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvBrokenFileNames *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxConvBrokenFileNames::WC2MB(a0, a1, a2);
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvBrokenFileNames *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxConvBrokenFileNames::GetMBNulLen();
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvBrokenFileNames *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxConvBrokenFileNames::Clone();
	}
	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvBrokenFileNames *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvBrokenFileNames *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::FromWChar(a0, a1, a2, a3);
	}
};

// wxMBConvUTF7 file: line:251
struct PyCallBack_wxMBConvUTF7 : public wxMBConvUTF7 {
	using wxMBConvUTF7::wxMBConvUTF7;

	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF7 *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF7::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF7 *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF7::FromWChar(a0, a1, a2, a3);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF7 *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxMBConvUTF7::Clone();
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF7 *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::GetMBNulLen();
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF7 *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF7 *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::WC2MB(a0, a1, a2);
	}
};

// wxMBConvStrictUTF8 file: line:341
struct PyCallBack_wxMBConvStrictUTF8 : public wxMBConvStrictUTF8 {
	using wxMBConvStrictUTF8::wxMBConvStrictUTF8;

	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvStrictUTF8 *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvStrictUTF8::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvStrictUTF8 *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvStrictUTF8::FromWChar(a0, a1, a2, a3);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvStrictUTF8 *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxMBConvStrictUTF8::Clone();
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvStrictUTF8 *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::GetMBNulLen();
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvStrictUTF8 *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvStrictUTF8 *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::WC2MB(a0, a1, a2);
	}
};

// wxMBConvUTF8 file: line:360
struct PyCallBack_wxMBConvUTF8 : public wxMBConvUTF8 {
	using wxMBConvUTF8::wxMBConvUTF8;

	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF8 *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF8::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF8 *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF8::FromWChar(a0, a1, a2, a3);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF8 *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxMBConvUTF8::Clone();
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF8 *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::GetMBNulLen();
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF8 *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF8 *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::WC2MB(a0, a1, a2);
	}
};

// wxMBConvUTF16Base file: line:393
struct PyCallBack_wxMBConvUTF16Base : public wxMBConvUTF16Base {
	using wxMBConvUTF16Base::wxMBConvUTF16Base;

	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16Base *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF16Base::GetMBNulLen();
	}
	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16Base *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16Base *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::FromWChar(a0, a1, a2, a3);
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16Base *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16Base *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::WC2MB(a0, a1, a2);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16Base *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxMBConv::Clone\"");
	}
};

// wxMBConvUTF16LE file: line:412
struct PyCallBack_wxMBConvUTF16LE : public wxMBConvUTF16LE {
	using wxMBConvUTF16LE::wxMBConvUTF16LE;

	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16LE *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF16LE::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16LE *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF16LE::FromWChar(a0, a1, a2, a3);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16LE *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxMBConvUTF16LE::Clone();
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16LE *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF16Base::GetMBNulLen();
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16LE *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16LE *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::WC2MB(a0, a1, a2);
	}
};

// wxMBConvUTF16BE file: line:426
struct PyCallBack_wxMBConvUTF16BE : public wxMBConvUTF16BE {
	using wxMBConvUTF16BE::wxMBConvUTF16BE;

	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16BE *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF16BE::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16BE *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF16BE::FromWChar(a0, a1, a2, a3);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16BE *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxMBConvUTF16BE::Clone();
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16BE *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF16Base::GetMBNulLen();
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16BE *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF16BE *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::WC2MB(a0, a1, a2);
	}
};

// wxMBConvUTF32Base file: line:440
struct PyCallBack_wxMBConvUTF32Base : public wxMBConvUTF32Base {
	using wxMBConvUTF32Base::wxMBConvUTF32Base;

	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32Base *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF32Base::GetMBNulLen();
	}
	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32Base *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32Base *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::FromWChar(a0, a1, a2, a3);
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32Base *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32Base *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::WC2MB(a0, a1, a2);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32Base *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxMBConv::Clone\"");
	}
};

// wxMBConvUTF32LE file: line:458
struct PyCallBack_wxMBConvUTF32LE : public wxMBConvUTF32LE {
	using wxMBConvUTF32LE::wxMBConvUTF32LE;

	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32LE *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF32LE::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32LE *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF32LE::FromWChar(a0, a1, a2, a3);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32LE *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxMBConvUTF32LE::Clone();
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32LE *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF32Base::GetMBNulLen();
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32LE *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32LE *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::WC2MB(a0, a1, a2);
	}
};

void bind_unknown_unknown_5(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxMBConvLibc file: line:183
		pybind11::class_<wxMBConvLibc, std::shared_ptr<wxMBConvLibc>, PyCallBack_wxMBConvLibc, wxMBConv> cl(M(""), "wxMBConvLibc", "");
		cl.def( pybind11::init( [](PyCallBack_wxMBConvLibc const &o){ return new PyCallBack_wxMBConvLibc(o); } ) );
		cl.def( pybind11::init( [](wxMBConvLibc const &o){ return new wxMBConvLibc(o); } ) );
		cl.def( pybind11::init( [](){ return new wxMBConvLibc(); }, [](){ return new PyCallBack_wxMBConvLibc(); } ) );
		cl.def("MB2WC", (unsigned long (wxMBConvLibc::*)(wchar_t *, const char *, unsigned long) const) &wxMBConvLibc::MB2WC, "C++: wxMBConvLibc::MB2WC(wchar_t *, const char *, unsigned long) const --> unsigned long", pybind11::arg("outputBuf"), pybind11::arg("psz"), pybind11::arg("outputSize"));
		cl.def("WC2MB", (unsigned long (wxMBConvLibc::*)(char *, const wchar_t *, unsigned long) const) &wxMBConvLibc::WC2MB, "C++: wxMBConvLibc::WC2MB(char *, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("outputBuf"), pybind11::arg("psz"), pybind11::arg("outputSize"));
		cl.def("Clone", (class wxMBConv * (wxMBConvLibc::*)() const) &wxMBConvLibc::Clone, "C++: wxMBConvLibc::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxMBConvLibc & (wxMBConvLibc::*)(const class wxMBConvLibc &)) &wxMBConvLibc::operator=, "C++: wxMBConvLibc::operator=(const class wxMBConvLibc &) --> class wxMBConvLibc &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxConvBrokenFileNames file: line:205
		pybind11::class_<wxConvBrokenFileNames, std::shared_ptr<wxConvBrokenFileNames>, PyCallBack_wxConvBrokenFileNames, wxMBConv> cl(M(""), "wxConvBrokenFileNames", "");
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("charset") );

		cl.def( pybind11::init( [](PyCallBack_wxConvBrokenFileNames const &o){ return new PyCallBack_wxConvBrokenFileNames(o); } ) );
		cl.def( pybind11::init( [](wxConvBrokenFileNames const &o){ return new wxConvBrokenFileNames(o); } ) );
		cl.def("MB2WC", (unsigned long (wxConvBrokenFileNames::*)(wchar_t *, const char *, unsigned long) const) &wxConvBrokenFileNames::MB2WC, "C++: wxConvBrokenFileNames::MB2WC(wchar_t *, const char *, unsigned long) const --> unsigned long", pybind11::arg("out"), pybind11::arg("in"), pybind11::arg("outLen"));
		cl.def("WC2MB", (unsigned long (wxConvBrokenFileNames::*)(char *, const wchar_t *, unsigned long) const) &wxConvBrokenFileNames::WC2MB, "C++: wxConvBrokenFileNames::WC2MB(char *, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("out"), pybind11::arg("in"), pybind11::arg("outLen"));
		cl.def("GetMBNulLen", (unsigned long (wxConvBrokenFileNames::*)() const) &wxConvBrokenFileNames::GetMBNulLen, "C++: wxConvBrokenFileNames::GetMBNulLen() const --> unsigned long");
		cl.def("Clone", (class wxMBConv * (wxConvBrokenFileNames::*)() const) &wxConvBrokenFileNames::Clone, "C++: wxConvBrokenFileNames::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
	}
	{ // wxMBConvUTF7 file: line:251
		pybind11::class_<wxMBConvUTF7, std::shared_ptr<wxMBConvUTF7>, PyCallBack_wxMBConvUTF7, wxMBConv> cl(M(""), "wxMBConvUTF7", "");
		cl.def( pybind11::init( [](){ return new wxMBConvUTF7(); }, [](){ return new PyCallBack_wxMBConvUTF7(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxMBConvUTF7 const &o){ return new PyCallBack_wxMBConvUTF7(o); } ) );
		cl.def( pybind11::init( [](wxMBConvUTF7 const &o){ return new wxMBConvUTF7(o); } ) );
		cl.def("ToWChar", [](wxMBConvUTF7 const &o, wchar_t * a0, unsigned long const & a1, const char * a2) -> unsigned long { return o.ToWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("ToWChar", (unsigned long (wxMBConvUTF7::*)(wchar_t *, unsigned long, const char *, unsigned long) const) &wxMBConvUTF7::ToWChar, "C++: wxMBConvUTF7::ToWChar(wchar_t *, unsigned long, const char *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("FromWChar", [](wxMBConvUTF7 const &o, char * a0, unsigned long const & a1, const wchar_t * a2) -> unsigned long { return o.FromWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("FromWChar", (unsigned long (wxMBConvUTF7::*)(char *, unsigned long, const wchar_t *, unsigned long) const) &wxMBConvUTF7::FromWChar, "C++: wxMBConvUTF7::FromWChar(char *, unsigned long, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("Clone", (class wxMBConv * (wxMBConvUTF7::*)() const) &wxMBConvUTF7::Clone, "C++: wxMBConvUTF7::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxMBConvUTF7 & (wxMBConvUTF7::*)(const class wxMBConvUTF7 &)) &wxMBConvUTF7::operator=, "C++: wxMBConvUTF7::operator=(const class wxMBConvUTF7 &) --> class wxMBConvUTF7 &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMBConvStrictUTF8 file: line:341
		pybind11::class_<wxMBConvStrictUTF8, std::shared_ptr<wxMBConvStrictUTF8>, PyCallBack_wxMBConvStrictUTF8, wxMBConv> cl(M(""), "wxMBConvStrictUTF8", "");
		cl.def( pybind11::init( [](PyCallBack_wxMBConvStrictUTF8 const &o){ return new PyCallBack_wxMBConvStrictUTF8(o); } ) );
		cl.def( pybind11::init( [](wxMBConvStrictUTF8 const &o){ return new wxMBConvStrictUTF8(o); } ) );
		cl.def( pybind11::init( [](){ return new wxMBConvStrictUTF8(); }, [](){ return new PyCallBack_wxMBConvStrictUTF8(); } ) );
		cl.def("ToWChar", [](wxMBConvStrictUTF8 const &o, wchar_t * a0, unsigned long const & a1, const char * a2) -> unsigned long { return o.ToWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("ToWChar", (unsigned long (wxMBConvStrictUTF8::*)(wchar_t *, unsigned long, const char *, unsigned long) const) &wxMBConvStrictUTF8::ToWChar, "C++: wxMBConvStrictUTF8::ToWChar(wchar_t *, unsigned long, const char *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("FromWChar", [](wxMBConvStrictUTF8 const &o, char * a0, unsigned long const & a1, const wchar_t * a2) -> unsigned long { return o.FromWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("FromWChar", (unsigned long (wxMBConvStrictUTF8::*)(char *, unsigned long, const wchar_t *, unsigned long) const) &wxMBConvStrictUTF8::FromWChar, "C++: wxMBConvStrictUTF8::FromWChar(char *, unsigned long, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("Clone", (class wxMBConv * (wxMBConvStrictUTF8::*)() const) &wxMBConvStrictUTF8::Clone, "C++: wxMBConvStrictUTF8::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxMBConvStrictUTF8 & (wxMBConvStrictUTF8::*)(const class wxMBConvStrictUTF8 &)) &wxMBConvStrictUTF8::operator=, "C++: wxMBConvStrictUTF8::operator=(const class wxMBConvStrictUTF8 &) --> class wxMBConvStrictUTF8 &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMBConvUTF8 file: line:360
		pybind11::class_<wxMBConvUTF8, std::shared_ptr<wxMBConvUTF8>, PyCallBack_wxMBConvUTF8, wxMBConvStrictUTF8> cl(M(""), "wxMBConvUTF8", "");
		cl.def( pybind11::init( [](){ return new wxMBConvUTF8(); }, [](){ return new PyCallBack_wxMBConvUTF8(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("options") );

		cl.def( pybind11::init( [](PyCallBack_wxMBConvUTF8 const &o){ return new PyCallBack_wxMBConvUTF8(o); } ) );
		cl.def( pybind11::init( [](wxMBConvUTF8 const &o){ return new wxMBConvUTF8(o); } ) );
		cl.def("ToWChar", [](wxMBConvUTF8 const &o, wchar_t * a0, unsigned long const & a1, const char * a2) -> unsigned long { return o.ToWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("ToWChar", (unsigned long (wxMBConvUTF8::*)(wchar_t *, unsigned long, const char *, unsigned long) const) &wxMBConvUTF8::ToWChar, "C++: wxMBConvUTF8::ToWChar(wchar_t *, unsigned long, const char *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("FromWChar", [](wxMBConvUTF8 const &o, char * a0, unsigned long const & a1, const wchar_t * a2) -> unsigned long { return o.FromWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("FromWChar", (unsigned long (wxMBConvUTF8::*)(char *, unsigned long, const wchar_t *, unsigned long) const) &wxMBConvUTF8::FromWChar, "C++: wxMBConvUTF8::FromWChar(char *, unsigned long, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("Clone", (class wxMBConv * (wxMBConvUTF8::*)() const) &wxMBConvUTF8::Clone, "C++: wxMBConvUTF8::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxMBConvUTF8 & (wxMBConvUTF8::*)(const class wxMBConvUTF8 &)) &wxMBConvUTF8::operator=, "C++: wxMBConvUTF8::operator=(const class wxMBConvUTF8 &) --> class wxMBConvUTF8 &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMBConvUTF16Base file: line:393
		pybind11::class_<wxMBConvUTF16Base, std::shared_ptr<wxMBConvUTF16Base>, PyCallBack_wxMBConvUTF16Base, wxMBConv> cl(M(""), "wxMBConvUTF16Base", "");
		cl.def(pybind11::init<PyCallBack_wxMBConvUTF16Base const &>());
		cl.def( pybind11::init( [](){ return new PyCallBack_wxMBConvUTF16Base(); } ) );
		cl.def("GetMBNulLen", (unsigned long (wxMBConvUTF16Base::*)() const) &wxMBConvUTF16Base::GetMBNulLen, "C++: wxMBConvUTF16Base::GetMBNulLen() const --> unsigned long");
		cl.def("assign", (class wxMBConvUTF16Base & (wxMBConvUTF16Base::*)(const class wxMBConvUTF16Base &)) &wxMBConvUTF16Base::operator=, "C++: wxMBConvUTF16Base::operator=(const class wxMBConvUTF16Base &) --> class wxMBConvUTF16Base &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMBConvUTF16LE file: line:412
		pybind11::class_<wxMBConvUTF16LE, std::shared_ptr<wxMBConvUTF16LE>, PyCallBack_wxMBConvUTF16LE, wxMBConvUTF16Base> cl(M(""), "wxMBConvUTF16LE", "");
		cl.def( pybind11::init( [](){ return new wxMBConvUTF16LE(); }, [](){ return new PyCallBack_wxMBConvUTF16LE(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxMBConvUTF16LE const &o){ return new PyCallBack_wxMBConvUTF16LE(o); } ) );
		cl.def( pybind11::init( [](wxMBConvUTF16LE const &o){ return new wxMBConvUTF16LE(o); } ) );
		cl.def("ToWChar", [](wxMBConvUTF16LE const &o, wchar_t * a0, unsigned long const & a1, const char * a2) -> unsigned long { return o.ToWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("ToWChar", (unsigned long (wxMBConvUTF16LE::*)(wchar_t *, unsigned long, const char *, unsigned long) const) &wxMBConvUTF16LE::ToWChar, "C++: wxMBConvUTF16LE::ToWChar(wchar_t *, unsigned long, const char *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("FromWChar", [](wxMBConvUTF16LE const &o, char * a0, unsigned long const & a1, const wchar_t * a2) -> unsigned long { return o.FromWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("FromWChar", (unsigned long (wxMBConvUTF16LE::*)(char *, unsigned long, const wchar_t *, unsigned long) const) &wxMBConvUTF16LE::FromWChar, "C++: wxMBConvUTF16LE::FromWChar(char *, unsigned long, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("Clone", (class wxMBConv * (wxMBConvUTF16LE::*)() const) &wxMBConvUTF16LE::Clone, "C++: wxMBConvUTF16LE::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxMBConvUTF16LE & (wxMBConvUTF16LE::*)(const class wxMBConvUTF16LE &)) &wxMBConvUTF16LE::operator=, "C++: wxMBConvUTF16LE::operator=(const class wxMBConvUTF16LE &) --> class wxMBConvUTF16LE &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMBConvUTF16BE file: line:426
		pybind11::class_<wxMBConvUTF16BE, std::shared_ptr<wxMBConvUTF16BE>, PyCallBack_wxMBConvUTF16BE, wxMBConvUTF16Base> cl(M(""), "wxMBConvUTF16BE", "");
		cl.def( pybind11::init( [](){ return new wxMBConvUTF16BE(); }, [](){ return new PyCallBack_wxMBConvUTF16BE(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxMBConvUTF16BE const &o){ return new PyCallBack_wxMBConvUTF16BE(o); } ) );
		cl.def( pybind11::init( [](wxMBConvUTF16BE const &o){ return new wxMBConvUTF16BE(o); } ) );
		cl.def("ToWChar", [](wxMBConvUTF16BE const &o, wchar_t * a0, unsigned long const & a1, const char * a2) -> unsigned long { return o.ToWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("ToWChar", (unsigned long (wxMBConvUTF16BE::*)(wchar_t *, unsigned long, const char *, unsigned long) const) &wxMBConvUTF16BE::ToWChar, "C++: wxMBConvUTF16BE::ToWChar(wchar_t *, unsigned long, const char *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("FromWChar", [](wxMBConvUTF16BE const &o, char * a0, unsigned long const & a1, const wchar_t * a2) -> unsigned long { return o.FromWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("FromWChar", (unsigned long (wxMBConvUTF16BE::*)(char *, unsigned long, const wchar_t *, unsigned long) const) &wxMBConvUTF16BE::FromWChar, "C++: wxMBConvUTF16BE::FromWChar(char *, unsigned long, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("Clone", (class wxMBConv * (wxMBConvUTF16BE::*)() const) &wxMBConvUTF16BE::Clone, "C++: wxMBConvUTF16BE::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxMBConvUTF16BE & (wxMBConvUTF16BE::*)(const class wxMBConvUTF16BE &)) &wxMBConvUTF16BE::operator=, "C++: wxMBConvUTF16BE::operator=(const class wxMBConvUTF16BE &) --> class wxMBConvUTF16BE &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMBConvUTF32Base file: line:440
		pybind11::class_<wxMBConvUTF32Base, std::shared_ptr<wxMBConvUTF32Base>, PyCallBack_wxMBConvUTF32Base, wxMBConv> cl(M(""), "wxMBConvUTF32Base", "");
		cl.def(pybind11::init<PyCallBack_wxMBConvUTF32Base const &>());
		cl.def( pybind11::init( [](){ return new PyCallBack_wxMBConvUTF32Base(); } ) );
		cl.def("GetMBNulLen", (unsigned long (wxMBConvUTF32Base::*)() const) &wxMBConvUTF32Base::GetMBNulLen, "C++: wxMBConvUTF32Base::GetMBNulLen() const --> unsigned long");
		cl.def("assign", (class wxMBConvUTF32Base & (wxMBConvUTF32Base::*)(const class wxMBConvUTF32Base &)) &wxMBConvUTF32Base::operator=, "C++: wxMBConvUTF32Base::operator=(const class wxMBConvUTF32Base &) --> class wxMBConvUTF32Base &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMBConvUTF32LE file: line:458
		pybind11::class_<wxMBConvUTF32LE, std::shared_ptr<wxMBConvUTF32LE>, PyCallBack_wxMBConvUTF32LE, wxMBConvUTF32Base> cl(M(""), "wxMBConvUTF32LE", "");
		cl.def( pybind11::init( [](){ return new wxMBConvUTF32LE(); }, [](){ return new PyCallBack_wxMBConvUTF32LE(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxMBConvUTF32LE const &o){ return new PyCallBack_wxMBConvUTF32LE(o); } ) );
		cl.def( pybind11::init( [](wxMBConvUTF32LE const &o){ return new wxMBConvUTF32LE(o); } ) );
		cl.def("ToWChar", [](wxMBConvUTF32LE const &o, wchar_t * a0, unsigned long const & a1, const char * a2) -> unsigned long { return o.ToWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("ToWChar", (unsigned long (wxMBConvUTF32LE::*)(wchar_t *, unsigned long, const char *, unsigned long) const) &wxMBConvUTF32LE::ToWChar, "C++: wxMBConvUTF32LE::ToWChar(wchar_t *, unsigned long, const char *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("FromWChar", [](wxMBConvUTF32LE const &o, char * a0, unsigned long const & a1, const wchar_t * a2) -> unsigned long { return o.FromWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("FromWChar", (unsigned long (wxMBConvUTF32LE::*)(char *, unsigned long, const wchar_t *, unsigned long) const) &wxMBConvUTF32LE::FromWChar, "C++: wxMBConvUTF32LE::FromWChar(char *, unsigned long, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("Clone", (class wxMBConv * (wxMBConvUTF32LE::*)() const) &wxMBConvUTF32LE::Clone, "C++: wxMBConvUTF32LE::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxMBConvUTF32LE & (wxMBConvUTF32LE::*)(const class wxMBConvUTF32LE &)) &wxMBConvUTF32LE::operator=, "C++: wxMBConvUTF32LE::operator=(const class wxMBConvUTF32LE &) --> class wxMBConvUTF32LE &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
