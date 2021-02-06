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

// wxMBConvUTF32BE file: line:472
struct PyCallBack_wxMBConvUTF32BE : public wxMBConvUTF32BE {
	using wxMBConvUTF32BE::wxMBConvUTF32BE;

	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32BE *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF32BE::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32BE *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConvUTF32BE::FromWChar(a0, a1, a2, a3);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32BE *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxMBConvUTF32BE::Clone();
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32BE *>(this), "GetMBNulLen");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32BE *>(this), "MB2WC");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConvUTF32BE *>(this), "WC2MB");
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

// wxCSConv file: line:488
struct PyCallBack_wxCSConv : public wxCSConv {
	using wxCSConv::wxCSConv;

	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCSConv *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxCSConv::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCSConv *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxCSConv::FromWChar(a0, a1, a2, a3);
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCSConv *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxCSConv::GetMBNulLen();
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCSConv *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxCSConv::Clone();
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCSConv *>(this), "MB2WC");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCSConv *>(this), "WC2MB");
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

void bind_unknown_unknown_6(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxMBConvUTF32BE file: line:472
		pybind11::class_<wxMBConvUTF32BE, std::shared_ptr<wxMBConvUTF32BE>, PyCallBack_wxMBConvUTF32BE, wxMBConvUTF32Base> cl(M(""), "wxMBConvUTF32BE", "");
		cl.def( pybind11::init( [](){ return new wxMBConvUTF32BE(); }, [](){ return new PyCallBack_wxMBConvUTF32BE(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxMBConvUTF32BE const &o){ return new PyCallBack_wxMBConvUTF32BE(o); } ) );
		cl.def( pybind11::init( [](wxMBConvUTF32BE const &o){ return new wxMBConvUTF32BE(o); } ) );
		cl.def("ToWChar", [](wxMBConvUTF32BE const &o, wchar_t * a0, unsigned long const & a1, const char * a2) -> unsigned long { return o.ToWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("ToWChar", (unsigned long (wxMBConvUTF32BE::*)(wchar_t *, unsigned long, const char *, unsigned long) const) &wxMBConvUTF32BE::ToWChar, "C++: wxMBConvUTF32BE::ToWChar(wchar_t *, unsigned long, const char *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("FromWChar", [](wxMBConvUTF32BE const &o, char * a0, unsigned long const & a1, const wchar_t * a2) -> unsigned long { return o.FromWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("FromWChar", (unsigned long (wxMBConvUTF32BE::*)(char *, unsigned long, const wchar_t *, unsigned long) const) &wxMBConvUTF32BE::FromWChar, "C++: wxMBConvUTF32BE::FromWChar(char *, unsigned long, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("Clone", (class wxMBConv * (wxMBConvUTF32BE::*)() const) &wxMBConvUTF32BE::Clone, "C++: wxMBConvUTF32BE::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxMBConvUTF32BE & (wxMBConvUTF32BE::*)(const class wxMBConvUTF32BE &)) &wxMBConvUTF32BE::operator=, "C++: wxMBConvUTF32BE::operator=(const class wxMBConvUTF32BE &) --> class wxMBConvUTF32BE &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxFontEncoding file: line:15
	pybind11::enum_<wxFontEncoding>(M(""), "wxFontEncoding", pybind11::arithmetic(), "")
		.value("wxFONTENCODING_SYSTEM", wxFONTENCODING_SYSTEM)
		.value("wxFONTENCODING_DEFAULT", wxFONTENCODING_DEFAULT)
		.value("wxFONTENCODING_ISO8859_1", wxFONTENCODING_ISO8859_1)
		.value("wxFONTENCODING_ISO8859_2", wxFONTENCODING_ISO8859_2)
		.value("wxFONTENCODING_ISO8859_3", wxFONTENCODING_ISO8859_3)
		.value("wxFONTENCODING_ISO8859_4", wxFONTENCODING_ISO8859_4)
		.value("wxFONTENCODING_ISO8859_5", wxFONTENCODING_ISO8859_5)
		.value("wxFONTENCODING_ISO8859_6", wxFONTENCODING_ISO8859_6)
		.value("wxFONTENCODING_ISO8859_7", wxFONTENCODING_ISO8859_7)
		.value("wxFONTENCODING_ISO8859_8", wxFONTENCODING_ISO8859_8)
		.value("wxFONTENCODING_ISO8859_9", wxFONTENCODING_ISO8859_9)
		.value("wxFONTENCODING_ISO8859_10", wxFONTENCODING_ISO8859_10)
		.value("wxFONTENCODING_ISO8859_11", wxFONTENCODING_ISO8859_11)
		.value("wxFONTENCODING_ISO8859_12", wxFONTENCODING_ISO8859_12)
		.value("wxFONTENCODING_ISO8859_13", wxFONTENCODING_ISO8859_13)
		.value("wxFONTENCODING_ISO8859_14", wxFONTENCODING_ISO8859_14)
		.value("wxFONTENCODING_ISO8859_15", wxFONTENCODING_ISO8859_15)
		.value("wxFONTENCODING_ISO8859_MAX", wxFONTENCODING_ISO8859_MAX)
		.value("wxFONTENCODING_KOI8", wxFONTENCODING_KOI8)
		.value("wxFONTENCODING_KOI8_U", wxFONTENCODING_KOI8_U)
		.value("wxFONTENCODING_ALTERNATIVE", wxFONTENCODING_ALTERNATIVE)
		.value("wxFONTENCODING_BULGARIAN", wxFONTENCODING_BULGARIAN)
		.value("wxFONTENCODING_CP437", wxFONTENCODING_CP437)
		.value("wxFONTENCODING_CP850", wxFONTENCODING_CP850)
		.value("wxFONTENCODING_CP852", wxFONTENCODING_CP852)
		.value("wxFONTENCODING_CP855", wxFONTENCODING_CP855)
		.value("wxFONTENCODING_CP866", wxFONTENCODING_CP866)
		.value("wxFONTENCODING_CP874", wxFONTENCODING_CP874)
		.value("wxFONTENCODING_CP932", wxFONTENCODING_CP932)
		.value("wxFONTENCODING_CP936", wxFONTENCODING_CP936)
		.value("wxFONTENCODING_CP949", wxFONTENCODING_CP949)
		.value("wxFONTENCODING_CP950", wxFONTENCODING_CP950)
		.value("wxFONTENCODING_CP1250", wxFONTENCODING_CP1250)
		.value("wxFONTENCODING_CP1251", wxFONTENCODING_CP1251)
		.value("wxFONTENCODING_CP1252", wxFONTENCODING_CP1252)
		.value("wxFONTENCODING_CP1253", wxFONTENCODING_CP1253)
		.value("wxFONTENCODING_CP1254", wxFONTENCODING_CP1254)
		.value("wxFONTENCODING_CP1255", wxFONTENCODING_CP1255)
		.value("wxFONTENCODING_CP1256", wxFONTENCODING_CP1256)
		.value("wxFONTENCODING_CP1257", wxFONTENCODING_CP1257)
		.value("wxFONTENCODING_CP1258", wxFONTENCODING_CP1258)
		.value("wxFONTENCODING_CP1361", wxFONTENCODING_CP1361)
		.value("wxFONTENCODING_CP12_MAX", wxFONTENCODING_CP12_MAX)
		.value("wxFONTENCODING_UTF7", wxFONTENCODING_UTF7)
		.value("wxFONTENCODING_UTF8", wxFONTENCODING_UTF8)
		.value("wxFONTENCODING_EUC_JP", wxFONTENCODING_EUC_JP)
		.value("wxFONTENCODING_UTF16BE", wxFONTENCODING_UTF16BE)
		.value("wxFONTENCODING_UTF16LE", wxFONTENCODING_UTF16LE)
		.value("wxFONTENCODING_UTF32BE", wxFONTENCODING_UTF32BE)
		.value("wxFONTENCODING_UTF32LE", wxFONTENCODING_UTF32LE)
		.value("wxFONTENCODING_MACROMAN", wxFONTENCODING_MACROMAN)
		.value("wxFONTENCODING_MACJAPANESE", wxFONTENCODING_MACJAPANESE)
		.value("wxFONTENCODING_MACCHINESETRAD", wxFONTENCODING_MACCHINESETRAD)
		.value("wxFONTENCODING_MACKOREAN", wxFONTENCODING_MACKOREAN)
		.value("wxFONTENCODING_MACARABIC", wxFONTENCODING_MACARABIC)
		.value("wxFONTENCODING_MACHEBREW", wxFONTENCODING_MACHEBREW)
		.value("wxFONTENCODING_MACGREEK", wxFONTENCODING_MACGREEK)
		.value("wxFONTENCODING_MACCYRILLIC", wxFONTENCODING_MACCYRILLIC)
		.value("wxFONTENCODING_MACDEVANAGARI", wxFONTENCODING_MACDEVANAGARI)
		.value("wxFONTENCODING_MACGURMUKHI", wxFONTENCODING_MACGURMUKHI)
		.value("wxFONTENCODING_MACGUJARATI", wxFONTENCODING_MACGUJARATI)
		.value("wxFONTENCODING_MACORIYA", wxFONTENCODING_MACORIYA)
		.value("wxFONTENCODING_MACBENGALI", wxFONTENCODING_MACBENGALI)
		.value("wxFONTENCODING_MACTAMIL", wxFONTENCODING_MACTAMIL)
		.value("wxFONTENCODING_MACTELUGU", wxFONTENCODING_MACTELUGU)
		.value("wxFONTENCODING_MACKANNADA", wxFONTENCODING_MACKANNADA)
		.value("wxFONTENCODING_MACMALAJALAM", wxFONTENCODING_MACMALAJALAM)
		.value("wxFONTENCODING_MACSINHALESE", wxFONTENCODING_MACSINHALESE)
		.value("wxFONTENCODING_MACBURMESE", wxFONTENCODING_MACBURMESE)
		.value("wxFONTENCODING_MACKHMER", wxFONTENCODING_MACKHMER)
		.value("wxFONTENCODING_MACTHAI", wxFONTENCODING_MACTHAI)
		.value("wxFONTENCODING_MACLAOTIAN", wxFONTENCODING_MACLAOTIAN)
		.value("wxFONTENCODING_MACGEORGIAN", wxFONTENCODING_MACGEORGIAN)
		.value("wxFONTENCODING_MACARMENIAN", wxFONTENCODING_MACARMENIAN)
		.value("wxFONTENCODING_MACCHINESESIMP", wxFONTENCODING_MACCHINESESIMP)
		.value("wxFONTENCODING_MACTIBETAN", wxFONTENCODING_MACTIBETAN)
		.value("wxFONTENCODING_MACMONGOLIAN", wxFONTENCODING_MACMONGOLIAN)
		.value("wxFONTENCODING_MACETHIOPIC", wxFONTENCODING_MACETHIOPIC)
		.value("wxFONTENCODING_MACCENTRALEUR", wxFONTENCODING_MACCENTRALEUR)
		.value("wxFONTENCODING_MACVIATNAMESE", wxFONTENCODING_MACVIATNAMESE)
		.value("wxFONTENCODING_MACARABICEXT", wxFONTENCODING_MACARABICEXT)
		.value("wxFONTENCODING_MACSYMBOL", wxFONTENCODING_MACSYMBOL)
		.value("wxFONTENCODING_MACDINGBATS", wxFONTENCODING_MACDINGBATS)
		.value("wxFONTENCODING_MACTURKISH", wxFONTENCODING_MACTURKISH)
		.value("wxFONTENCODING_MACCROATIAN", wxFONTENCODING_MACCROATIAN)
		.value("wxFONTENCODING_MACICELANDIC", wxFONTENCODING_MACICELANDIC)
		.value("wxFONTENCODING_MACROMANIAN", wxFONTENCODING_MACROMANIAN)
		.value("wxFONTENCODING_MACCELTIC", wxFONTENCODING_MACCELTIC)
		.value("wxFONTENCODING_MACGAELIC", wxFONTENCODING_MACGAELIC)
		.value("wxFONTENCODING_MACKEYBOARD", wxFONTENCODING_MACKEYBOARD)
		.value("wxFONTENCODING_ISO2022_JP", wxFONTENCODING_ISO2022_JP)
		.value("wxFONTENCODING_MAX", wxFONTENCODING_MAX)
		.value("wxFONTENCODING_MACMIN", wxFONTENCODING_MACMIN)
		.value("wxFONTENCODING_MACMAX", wxFONTENCODING_MACMAX)
		.value("wxFONTENCODING_UTF16", wxFONTENCODING_UTF16)
		.value("wxFONTENCODING_UTF32", wxFONTENCODING_UTF32)
		.value("wxFONTENCODING_UNICODE", wxFONTENCODING_UNICODE)
		.value("wxFONTENCODING_GB2312", wxFONTENCODING_GB2312)
		.value("wxFONTENCODING_BIG5", wxFONTENCODING_BIG5)
		.value("wxFONTENCODING_SHIFT_JIS", wxFONTENCODING_SHIFT_JIS)
		.value("wxFONTENCODING_EUC_KR", wxFONTENCODING_EUC_KR)
		.value("wxFONTENCODING_JOHAB", wxFONTENCODING_JOHAB)
		.value("wxFONTENCODING_VIETNAMESE", wxFONTENCODING_VIETNAMESE)
		.export_values();

;

	{ // wxCSConv file: line:488
		pybind11::class_<wxCSConv, std::shared_ptr<wxCSConv>, PyCallBack_wxCSConv, wxMBConv> cl(M(""), "wxCSConv", "");
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("charset") );

		cl.def( pybind11::init<enum wxFontEncoding>(), pybind11::arg("encoding") );

		cl.def( pybind11::init( [](PyCallBack_wxCSConv const &o){ return new PyCallBack_wxCSConv(o); } ) );
		cl.def( pybind11::init( [](wxCSConv const &o){ return new wxCSConv(o); } ) );
		cl.def("assign", (class wxCSConv & (wxCSConv::*)(const class wxCSConv &)) &wxCSConv::operator=, "C++: wxCSConv::operator=(const class wxCSConv &) --> class wxCSConv &", pybind11::return_value_policy::automatic, pybind11::arg("conv"));
		cl.def("ToWChar", [](wxCSConv const &o, wchar_t * a0, unsigned long const & a1, const char * a2) -> unsigned long { return o.ToWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("ToWChar", (unsigned long (wxCSConv::*)(wchar_t *, unsigned long, const char *, unsigned long) const) &wxCSConv::ToWChar, "C++: wxCSConv::ToWChar(wchar_t *, unsigned long, const char *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("FromWChar", [](wxCSConv const &o, char * a0, unsigned long const & a1, const wchar_t * a2) -> unsigned long { return o.FromWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("FromWChar", (unsigned long (wxCSConv::*)(char *, unsigned long, const wchar_t *, unsigned long) const) &wxCSConv::FromWChar, "C++: wxCSConv::FromWChar(char *, unsigned long, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("GetMBNulLen", (unsigned long (wxCSConv::*)() const) &wxCSConv::GetMBNulLen, "C++: wxCSConv::GetMBNulLen() const --> unsigned long");
		cl.def("Clone", (class wxMBConv * (wxCSConv::*)() const) &wxCSConv::Clone, "C++: wxCSConv::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def("Clear", (void (wxCSConv::*)()) &wxCSConv::Clear, "C++: wxCSConv::Clear() --> void");
		cl.def("IsOk", (bool (wxCSConv::*)() const) &wxCSConv::IsOk, "C++: wxCSConv::IsOk() const --> bool");
	}
	// wxGet_wxConvLibcPtr() file: line:292
	M("").def("wxGet_wxConvLibcPtr", (class wxMBConv * (*)()) &wxGet_wxConvLibcPtr, "C++: wxGet_wxConvLibcPtr() --> class wxMBConv *", pybind11::return_value_policy::automatic);

	// wxGet_wxConvLibc() file: line:293
	M("").def("wxGet_wxConvLibc", (class wxMBConv & (*)()) &wxGet_wxConvLibc, "C++: wxGet_wxConvLibc() --> class wxMBConv &", pybind11::return_value_policy::automatic);

	// wxGet_wxConvISO8859_1Ptr() file: line:301
	M("").def("wxGet_wxConvISO8859_1Ptr", (class wxCSConv * (*)()) &wxGet_wxConvISO8859_1Ptr, "C++: wxGet_wxConvISO8859_1Ptr() --> class wxCSConv *", pybind11::return_value_policy::automatic);

	// wxGet_wxConvISO8859_1() file: line:302
	M("").def("wxGet_wxConvISO8859_1", (class wxCSConv & (*)()) &wxGet_wxConvISO8859_1, "C++: wxGet_wxConvISO8859_1() --> class wxCSConv &", pybind11::return_value_policy::automatic);

	// wxGet_wxConvUTF8Ptr() file: line:310
	M("").def("wxGet_wxConvUTF8Ptr", (class wxMBConvStrictUTF8 * (*)()) &wxGet_wxConvUTF8Ptr, "C++: wxGet_wxConvUTF8Ptr() --> class wxMBConvStrictUTF8 *", pybind11::return_value_policy::automatic);

	// wxGet_wxConvUTF8() file: line:311
	M("").def("wxGet_wxConvUTF8", (class wxMBConvStrictUTF8 & (*)()) &wxGet_wxConvUTF8, "C++: wxGet_wxConvUTF8() --> class wxMBConvStrictUTF8 &", pybind11::return_value_policy::automatic);

	// wxGet_wxConvUTF7Ptr() file: line:319
	M("").def("wxGet_wxConvUTF7Ptr", (class wxMBConvUTF7 * (*)()) &wxGet_wxConvUTF7Ptr, "C++: wxGet_wxConvUTF7Ptr() --> class wxMBConvUTF7 *", pybind11::return_value_policy::automatic);

	// wxGet_wxConvUTF7() file: line:320
	M("").def("wxGet_wxConvUTF7", (class wxMBConvUTF7 & (*)()) &wxGet_wxConvUTF7, "C++: wxGet_wxConvUTF7() --> class wxMBConvUTF7 &", pybind11::return_value_policy::automatic);

	// wxGet_wxConvLocalPtr() file: line:328
	M("").def("wxGet_wxConvLocalPtr", (class wxCSConv * (*)()) &wxGet_wxConvLocalPtr, "C++: wxGet_wxConvLocalPtr() --> class wxCSConv *", pybind11::return_value_policy::automatic);

	// wxGet_wxConvLocal() file: line:329
	M("").def("wxGet_wxConvLocal", (class wxCSConv & (*)()) &wxGet_wxConvLocal, "C++: wxGet_wxConvLocal() --> class wxCSConv &", pybind11::return_value_policy::automatic);

	// wxSafeConvertMB2WX(const char *) file: line:661
	M("").def("wxSafeConvertMB2WX", (class wxWCharBuffer (*)(const char *)) &wxSafeConvertMB2WX, "C++: wxSafeConvertMB2WX(const char *) --> class wxWCharBuffer", pybind11::arg("s"));

	// wxSafeConvertWX2MB(const wchar_t *) file: line:665
	M("").def("wxSafeConvertWX2MB", (class wxCharBuffer (*)(const wchar_t *)) &wxSafeConvertWX2MB, "C++: wxSafeConvertWX2MB(const wchar_t *) --> class wxCharBuffer", pybind11::arg("ws"));

}
