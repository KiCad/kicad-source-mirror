#include <iterator> // __gnu_cxx::__normal_iterator
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

// wxFont file: line:16
struct PyCallBack_wxFont : public wxFont {
	using wxFont::wxFont;

	int GetPointSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "GetPointSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxFont::GetPointSize();
	}
	enum wxFontStyle GetStyle() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "GetStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxFontStyle>::value) {
				static pybind11::detail::override_caster_t<enum wxFontStyle> caster;
				return pybind11::detail::cast_ref<enum wxFontStyle>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxFontStyle>(std::move(o));
		}
		return wxFont::GetStyle();
	}
	enum wxFontWeight GetWeight() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "GetWeight");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxFontWeight>::value) {
				static pybind11::detail::override_caster_t<enum wxFontWeight> caster;
				return pybind11::detail::cast_ref<enum wxFontWeight>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxFontWeight>(std::move(o));
		}
		return wxFont::GetWeight();
	}
	class wxString GetFaceName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "GetFaceName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxFont::GetFaceName();
	}
	bool GetUnderlined() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "GetUnderlined");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFont::GetUnderlined();
	}
	bool GetStrikethrough() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "GetStrikethrough");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFont::GetStrikethrough();
	}
	enum wxFontEncoding GetEncoding() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "GetEncoding");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxFontEncoding>::value) {
				static pybind11::detail::override_caster_t<enum wxFontEncoding> caster;
				return pybind11::detail::cast_ref<enum wxFontEncoding>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxFontEncoding>(std::move(o));
		}
		return wxFont::GetEncoding();
	}
	bool IsFixedWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "IsFixedWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFont::IsFixedWidth();
	}
	void SetPointSize(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "SetPointSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxFont::SetPointSize(a0);
	}
	void SetFamily(enum wxFontFamily a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "SetFamily");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxFont::SetFamily(a0);
	}
	void SetStyle(enum wxFontStyle a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "SetStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxFont::SetStyle(a0);
	}
	void SetWeight(enum wxFontWeight a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "SetWeight");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxFont::SetWeight(a0);
	}
	bool SetFaceName(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "SetFaceName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFont::SetFaceName(a0);
	}
	void SetUnderlined(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "SetUnderlined");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxFont::SetUnderlined(a0);
	}
	void SetStrikethrough(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "SetStrikethrough");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxFont::SetStrikethrough(a0);
	}
	void SetEncoding(enum wxFontEncoding a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "SetEncoding");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxFont::SetEncoding(a0);
	}
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxFont::CreateGDIRefData();
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxFont::CloneGDIRefData(a0);
	}
	enum wxFontFamily DoGetFamily() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "DoGetFamily");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxFontFamily>::value) {
				static pybind11::detail::override_caster_t<enum wxFontFamily> caster;
				return pybind11::detail::cast_ref<enum wxFontFamily>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxFontFamily>(std::move(o));
		}
		return wxFont::DoGetFamily();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxFont::GetClassInfo();
	}
	class wxSize GetPixelSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "GetPixelSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxFontBase::GetPixelSize();
	}
	bool IsUsingSizeInPixels() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "IsUsingSizeInPixels");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFontBase::IsUsingSizeInPixels();
	}
	void SetPixelSize(const class wxSize & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "SetPixelSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxFontBase::SetPixelSize(a0);
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "IsOk");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxGDIObject::IsOk();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxGDIObject::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFont *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxGDIObject::CloneRefData(a0);
	}
};

void bind_unknown_unknown_63(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxToString(const class wxFontBase &) file: line:474
	M("").def("wxToString", (class wxString (*)(const class wxFontBase &)) &wxToString, "C++: wxToString(const class wxFontBase &) --> class wxString", pybind11::arg("font"));

	// wxFromString(const class wxString &, class wxFontBase *) file: line:475
	M("").def("wxFromString", (bool (*)(const class wxString &, class wxFontBase *)) &wxFromString, "C++: wxFromString(const class wxString &, class wxFontBase *) --> bool", pybind11::arg("str"), pybind11::arg("font"));

	{ // wxFont file: line:16
		pybind11::class_<wxFont, std::shared_ptr<wxFont>, PyCallBack_wxFont, wxFontBase> cl(M(""), "wxFont", "");
		cl.def( pybind11::init( [](){ return new wxFont(); }, [](){ return new PyCallBack_wxFont(); } ) );
		cl.def( pybind11::init<const class wxFontInfo &>(), pybind11::arg("info") );

		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("nativeFontInfoString") );

		cl.def( pybind11::init( [](int const & a0, int const & a1, int const & a2, int const & a3){ return new wxFont(a0, a1, a2, a3); }, [](int const & a0, int const & a1, int const & a2, int const & a3){ return new PyCallBack_wxFont(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1, int const & a2, int const & a3, bool const & a4){ return new wxFont(a0, a1, a2, a3, a4); }, [](int const & a0, int const & a1, int const & a2, int const & a3, bool const & a4){ return new PyCallBack_wxFont(a0, a1, a2, a3, a4); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1, int const & a2, int const & a3, bool const & a4, const class wxString & a5){ return new wxFont(a0, a1, a2, a3, a4, a5); }, [](int const & a0, int const & a1, int const & a2, int const & a3, bool const & a4, const class wxString & a5){ return new PyCallBack_wxFont(a0, a1, a2, a3, a4, a5); } ), "doc");
		cl.def( pybind11::init<int, int, int, int, bool, const class wxString &, enum wxFontEncoding>(), pybind11::arg("size"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"), pybind11::arg("encoding") );

		cl.def( pybind11::init( [](int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3){ return new wxFont(a0, a1, a2, a3); }, [](int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3){ return new PyCallBack_wxFont(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4){ return new wxFont(a0, a1, a2, a3, a4); }, [](int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4){ return new PyCallBack_wxFont(a0, a1, a2, a3, a4); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4, const class wxString & a5){ return new wxFont(a0, a1, a2, a3, a4, a5); }, [](int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4, const class wxString & a5){ return new PyCallBack_wxFont(a0, a1, a2, a3, a4, a5); } ), "doc");
		cl.def( pybind11::init<int, enum wxFontFamily, enum wxFontStyle, enum wxFontWeight, bool, const class wxString &, enum wxFontEncoding>(), pybind11::arg("size"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"), pybind11::arg("encoding") );

		cl.def( pybind11::init( [](const class wxSize & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3){ return new wxFont(a0, a1, a2, a3); }, [](const class wxSize & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3){ return new PyCallBack_wxFont(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init( [](const class wxSize & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4){ return new wxFont(a0, a1, a2, a3, a4); }, [](const class wxSize & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4){ return new PyCallBack_wxFont(a0, a1, a2, a3, a4); } ), "doc");
		cl.def( pybind11::init( [](const class wxSize & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4, const class wxString & a5){ return new wxFont(a0, a1, a2, a3, a4, a5); }, [](const class wxSize & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4, const class wxString & a5){ return new PyCallBack_wxFont(a0, a1, a2, a3, a4, a5); } ), "doc");
		cl.def( pybind11::init<const class wxSize &, enum wxFontFamily, enum wxFontStyle, enum wxFontWeight, bool, const class wxString &, enum wxFontEncoding>(), pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"), pybind11::arg("encoding") );

		cl.def( pybind11::init( [](PyCallBack_wxFont const &o){ return new PyCallBack_wxFont(o); } ) );
		cl.def( pybind11::init( [](wxFont const &o){ return new wxFont(o); } ) );
		cl.def("Create", [](wxFont &o, int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3) -> bool { return o.Create(a0, a1, a2, a3); }, "", pybind11::arg("size"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"));
		cl.def("Create", [](wxFont &o, int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4) -> bool { return o.Create(a0, a1, a2, a3, a4); }, "", pybind11::arg("size"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"));
		cl.def("Create", [](wxFont &o, int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4, const class wxString & a5) -> bool { return o.Create(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("size"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"));
		cl.def("Create", (bool (wxFont::*)(int, enum wxFontFamily, enum wxFontStyle, enum wxFontWeight, bool, const class wxString &, enum wxFontEncoding)) &wxFont::Create, "C++: wxFont::Create(int, enum wxFontFamily, enum wxFontStyle, enum wxFontWeight, bool, const class wxString &, enum wxFontEncoding) --> bool", pybind11::arg("size"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"), pybind11::arg("encoding"));
		cl.def("Create", (bool (wxFont::*)(const class wxString &)) &wxFont::Create, "C++: wxFont::Create(const class wxString &) --> bool", pybind11::arg("fontname"));
		cl.def("GetPointSize", (int (wxFont::*)() const) &wxFont::GetPointSize, "C++: wxFont::GetPointSize() const --> int");
		cl.def("GetStyle", (enum wxFontStyle (wxFont::*)() const) &wxFont::GetStyle, "C++: wxFont::GetStyle() const --> enum wxFontStyle");
		cl.def("GetWeight", (enum wxFontWeight (wxFont::*)() const) &wxFont::GetWeight, "C++: wxFont::GetWeight() const --> enum wxFontWeight");
		cl.def("GetFaceName", (class wxString (wxFont::*)() const) &wxFont::GetFaceName, "C++: wxFont::GetFaceName() const --> class wxString");
		cl.def("GetUnderlined", (bool (wxFont::*)() const) &wxFont::GetUnderlined, "C++: wxFont::GetUnderlined() const --> bool");
		cl.def("GetStrikethrough", (bool (wxFont::*)() const) &wxFont::GetStrikethrough, "C++: wxFont::GetStrikethrough() const --> bool");
		cl.def("GetEncoding", (enum wxFontEncoding (wxFont::*)() const) &wxFont::GetEncoding, "C++: wxFont::GetEncoding() const --> enum wxFontEncoding");
		cl.def("IsFixedWidth", (bool (wxFont::*)() const) &wxFont::IsFixedWidth, "C++: wxFont::IsFixedWidth() const --> bool");
		cl.def("SetPointSize", (void (wxFont::*)(int)) &wxFont::SetPointSize, "C++: wxFont::SetPointSize(int) --> void", pybind11::arg("pointSize"));
		cl.def("SetFamily", (void (wxFont::*)(enum wxFontFamily)) &wxFont::SetFamily, "C++: wxFont::SetFamily(enum wxFontFamily) --> void", pybind11::arg("family"));
		cl.def("SetStyle", (void (wxFont::*)(enum wxFontStyle)) &wxFont::SetStyle, "C++: wxFont::SetStyle(enum wxFontStyle) --> void", pybind11::arg("style"));
		cl.def("SetWeight", (void (wxFont::*)(enum wxFontWeight)) &wxFont::SetWeight, "C++: wxFont::SetWeight(enum wxFontWeight) --> void", pybind11::arg("weight"));
		cl.def("SetFaceName", (bool (wxFont::*)(const class wxString &)) &wxFont::SetFaceName, "C++: wxFont::SetFaceName(const class wxString &) --> bool", pybind11::arg("faceName"));
		cl.def("SetUnderlined", (void (wxFont::*)(bool)) &wxFont::SetUnderlined, "C++: wxFont::SetUnderlined(bool) --> void", pybind11::arg("underlined"));
		cl.def("SetStrikethrough", (void (wxFont::*)(bool)) &wxFont::SetStrikethrough, "C++: wxFont::SetStrikethrough(bool) --> void", pybind11::arg("strikethrough"));
		cl.def("SetEncoding", (void (wxFont::*)(enum wxFontEncoding)) &wxFont::SetEncoding, "C++: wxFont::SetEncoding(enum wxFontEncoding) --> void", pybind11::arg("encoding"));
		cl.def("SetFamily", (void (wxFont::*)(int)) &wxFont::SetFamily, "C++: wxFont::SetFamily(int) --> void", pybind11::arg("family"));
		cl.def("SetStyle", (void (wxFont::*)(int)) &wxFont::SetStyle, "C++: wxFont::SetStyle(int) --> void", pybind11::arg("style"));
		cl.def("SetWeight", (void (wxFont::*)(int)) &wxFont::SetWeight, "C++: wxFont::SetWeight(int) --> void", pybind11::arg("weight"));
		cl.def("SetFamily", (void (wxFont::*)(enum wxDeprecatedGUIConstants)) &wxFont::SetFamily, "C++: wxFont::SetFamily(enum wxDeprecatedGUIConstants) --> void", pybind11::arg("family"));
		cl.def("SetStyle", (void (wxFont::*)(enum wxDeprecatedGUIConstants)) &wxFont::SetStyle, "C++: wxFont::SetStyle(enum wxDeprecatedGUIConstants) --> void", pybind11::arg("style"));
		cl.def("SetWeight", (void (wxFont::*)(enum wxDeprecatedGUIConstants)) &wxFont::SetWeight, "C++: wxFont::SetWeight(enum wxDeprecatedGUIConstants) --> void", pybind11::arg("weight"));
		cl.def("MakeBold", (class wxFont & (wxFont::*)()) &wxFont::MakeBold, "C++: wxFont::MakeBold() --> class wxFont &", pybind11::return_value_policy::automatic);
		cl.def("MakeItalic", (class wxFont & (wxFont::*)()) &wxFont::MakeItalic, "C++: wxFont::MakeItalic() --> class wxFont &", pybind11::return_value_policy::automatic);
		cl.def("MakeUnderlined", (class wxFont & (wxFont::*)()) &wxFont::MakeUnderlined, "C++: wxFont::MakeUnderlined() --> class wxFont &", pybind11::return_value_policy::automatic);
		cl.def("MakeStrikethrough", (class wxFont & (wxFont::*)()) &wxFont::MakeStrikethrough, "C++: wxFont::MakeStrikethrough() --> class wxFont &", pybind11::return_value_policy::automatic);
		cl.def("MakeLarger", (class wxFont & (wxFont::*)()) &wxFont::MakeLarger, "C++: wxFont::MakeLarger() --> class wxFont &", pybind11::return_value_policy::automatic);
		cl.def("MakeSmaller", (class wxFont & (wxFont::*)()) &wxFont::MakeSmaller, "C++: wxFont::MakeSmaller() --> class wxFont &", pybind11::return_value_policy::automatic);
		cl.def("Scale", (class wxFont & (wxFont::*)(float)) &wxFont::Scale, "C++: wxFont::Scale(float) --> class wxFont &", pybind11::return_value_policy::automatic, pybind11::arg("x"));
		cl.def("Bold", (class wxFont (wxFont::*)() const) &wxFont::Bold, "C++: wxFont::Bold() const --> class wxFont");
		cl.def("Italic", (class wxFont (wxFont::*)() const) &wxFont::Italic, "C++: wxFont::Italic() const --> class wxFont");
		cl.def("Underlined", (class wxFont (wxFont::*)() const) &wxFont::Underlined, "C++: wxFont::Underlined() const --> class wxFont");
		cl.def("Strikethrough", (class wxFont (wxFont::*)() const) &wxFont::Strikethrough, "C++: wxFont::Strikethrough() const --> class wxFont");
		cl.def("Larger", (class wxFont (wxFont::*)() const) &wxFont::Larger, "C++: wxFont::Larger() const --> class wxFont");
		cl.def("Smaller", (class wxFont (wxFont::*)() const) &wxFont::Smaller, "C++: wxFont::Smaller() const --> class wxFont");
		cl.def("Scaled", (class wxFont (wxFont::*)(float) const) &wxFont::Scaled, "C++: wxFont::Scaled(float) const --> class wxFont", pybind11::arg("x"));
		cl.def("Unshare", (void (wxFont::*)()) &wxFont::Unshare, "C++: wxFont::Unshare() --> void");
		cl.def("GetClassInfo", (class wxClassInfo * (wxFont::*)() const) &wxFont::GetClassInfo, "C++: wxFont::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxFont::wxCreateObject, "C++: wxFont::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxFont & (wxFont::*)(const class wxFont &)) &wxFont::operator=, "C++: wxFont::operator=(const class wxFont &) --> class wxFont &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxFontList file: line:538
		pybind11::class_<wxFontList, std::shared_ptr<wxFontList>, wxGDIObjListBase> cl(M(""), "wxFontList", "");
		cl.def( pybind11::init( [](){ return new wxFontList(); } ) );
		cl.def( pybind11::init( [](wxFontList const &o){ return new wxFontList(o); } ) );
		cl.def("FindOrCreateFont", [](wxFontList &o, int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3) -> wxFont * { return o.FindOrCreateFont(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"));
		cl.def("FindOrCreateFont", [](wxFontList &o, int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4) -> wxFont * { return o.FindOrCreateFont(a0, a1, a2, a3, a4); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underline"));
		cl.def("FindOrCreateFont", [](wxFontList &o, int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4, const class wxString & a5) -> wxFont * { return o.FindOrCreateFont(a0, a1, a2, a3, a4, a5); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underline"), pybind11::arg("face"));
		cl.def("FindOrCreateFont", (class wxFont * (wxFontList::*)(int, enum wxFontFamily, enum wxFontStyle, enum wxFontWeight, bool, const class wxString &, enum wxFontEncoding)) &wxFontList::FindOrCreateFont, "C++: wxFontList::FindOrCreateFont(int, enum wxFontFamily, enum wxFontStyle, enum wxFontWeight, bool, const class wxString &, enum wxFontEncoding) --> class wxFont *", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underline"), pybind11::arg("face"), pybind11::arg("encoding"));
		cl.def("FindOrCreateFont", [](wxFontList &o, int const & a0, int const & a1, int const & a2, int const & a3) -> wxFont * { return o.FindOrCreateFont(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"));
		cl.def("FindOrCreateFont", [](wxFontList &o, int const & a0, int const & a1, int const & a2, int const & a3, bool const & a4) -> wxFont * { return o.FindOrCreateFont(a0, a1, a2, a3, a4); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underline"));
		cl.def("FindOrCreateFont", [](wxFontList &o, int const & a0, int const & a1, int const & a2, int const & a3, bool const & a4, const class wxString & a5) -> wxFont * { return o.FindOrCreateFont(a0, a1, a2, a3, a4, a5); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underline"), pybind11::arg("face"));
		cl.def("FindOrCreateFont", (class wxFont * (wxFontList::*)(int, int, int, int, bool, const class wxString &, enum wxFontEncoding)) &wxFontList::FindOrCreateFont, "C++: wxFontList::FindOrCreateFont(int, int, int, int, bool, const class wxString &, enum wxFontEncoding) --> class wxFont *", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underline"), pybind11::arg("face"), pybind11::arg("encoding"));
	}
}
