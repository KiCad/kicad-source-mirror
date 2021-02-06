#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/dc.h> // wxDC

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxColourBase file: line:74
struct PyCallBack_wxColourBase : public wxColourBase {
	using wxColourBase::wxColourBase;

	unsigned char Red() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "Red");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned char>::value) {
				static pybind11::detail::override_caster_t<unsigned char> caster;
				return pybind11::detail::cast_ref<unsigned char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned char>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxColourBase::Red\"");
	}
	unsigned char Green() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "Green");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned char>::value) {
				static pybind11::detail::override_caster_t<unsigned char> caster;
				return pybind11::detail::cast_ref<unsigned char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned char>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxColourBase::Green\"");
	}
	unsigned char Blue() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "Blue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned char>::value) {
				static pybind11::detail::override_caster_t<unsigned char> caster;
				return pybind11::detail::cast_ref<unsigned char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned char>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxColourBase::Blue\"");
	}
	unsigned char Alpha() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "Alpha");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned char>::value) {
				static pybind11::detail::override_caster_t<unsigned char> caster;
				return pybind11::detail::cast_ref<unsigned char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned char>(std::move(o));
		}
		return wxColourBase::Alpha();
	}
	class wxString GetAsString(long a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "GetAsString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxColourBase::GetAsString(a0);
	}
	void InitRGBA(unsigned char a0, unsigned char a1, unsigned char a2, unsigned char a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "InitRGBA");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxColourBase::InitRGBA\"");
	}
	bool FromString(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "FromString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxColourBase::FromString(a0);
	}
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxColourBase::CreateGDIRefData();
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxColourBase::CloneGDIRefData(a0);
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "IsOk");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "CloneRefData");
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
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColourBase *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxGDIObject::GetClassInfo();
	}
};

// wxColour file: line:20
struct PyCallBack_wxColour : public wxColour {
	using wxColour::wxColour;

	unsigned char Red() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "Red");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned char>::value) {
				static pybind11::detail::override_caster_t<unsigned char> caster;
				return pybind11::detail::cast_ref<unsigned char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned char>(std::move(o));
		}
		return wxColour::Red();
	}
	unsigned char Green() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "Green");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned char>::value) {
				static pybind11::detail::override_caster_t<unsigned char> caster;
				return pybind11::detail::cast_ref<unsigned char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned char>(std::move(o));
		}
		return wxColour::Green();
	}
	unsigned char Blue() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "Blue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned char>::value) {
				static pybind11::detail::override_caster_t<unsigned char> caster;
				return pybind11::detail::cast_ref<unsigned char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned char>(std::move(o));
		}
		return wxColour::Blue();
	}
	unsigned char Alpha() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "Alpha");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned char>::value) {
				static pybind11::detail::override_caster_t<unsigned char> caster;
				return pybind11::detail::cast_ref<unsigned char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned char>(std::move(o));
		}
		return wxColour::Alpha();
	}
	void InitRGBA(unsigned char a0, unsigned char a1, unsigned char a2, unsigned char a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "InitRGBA");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxColour::InitRGBA(a0, a1, a2, a3);
	}
	bool FromString(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "FromString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxColour::FromString(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxColour::GetClassInfo();
	}
	class wxString GetAsString(long a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "GetAsString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxColourBase::GetAsString(a0);
	}
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxColourBase::CreateGDIRefData();
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxColourBase::CloneGDIRefData(a0);
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "IsOk");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxColour *>(this), "CloneRefData");
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

// wxRegionBase file: line:57
struct PyCallBack_wxRegionBase : public wxRegionBase {
	using wxRegionBase::wxRegionBase;

	bool IsEmpty() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "IsEmpty");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::IsEmpty\"");
	}
	void Clear() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "Clear");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::Clear\"");
	}
	bool DoIsEqual(const class wxRegion & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "DoIsEqual");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::DoIsEqual\"");
	}
	bool DoGetBox(int & a0, int & a1, int & a2, int & a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "DoGetBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::DoGetBox\"");
	}
	enum wxRegionContain DoContainsPoint(int a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "DoContainsPoint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxRegionContain>::value) {
				static pybind11::detail::override_caster_t<enum wxRegionContain> caster;
				return pybind11::detail::cast_ref<enum wxRegionContain>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxRegionContain>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::DoContainsPoint\"");
	}
	enum wxRegionContain DoContainsRect(const class wxRect & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "DoContainsRect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxRegionContain>::value) {
				static pybind11::detail::override_caster_t<enum wxRegionContain> caster;
				return pybind11::detail::cast_ref<enum wxRegionContain>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxRegionContain>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::DoContainsRect\"");
	}
	bool DoOffset(int a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "DoOffset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::DoOffset\"");
	}
	bool DoUnionWithRect(const class wxRect & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "DoUnionWithRect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::DoUnionWithRect\"");
	}
	bool DoUnionWithRegion(const class wxRegion & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "DoUnionWithRegion");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::DoUnionWithRegion\"");
	}
	bool DoIntersect(const class wxRegion & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "DoIntersect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::DoIntersect\"");
	}
	bool DoSubtract(const class wxRegion & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "DoSubtract");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::DoSubtract\"");
	}
	bool DoXor(const class wxRegion & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "DoXor");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxRegionBase::DoXor\"");
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "IsOk");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "CloneRefData");
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
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxGDIObject::CreateGDIRefData\"");
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxGDIObject::CloneGDIRefData\"");
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionBase *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxGDIObject::GetClassInfo();
	}
};

// wxRegion file: line:20
struct PyCallBack_wxRegion : public wxRegion {
	using wxRegion::wxRegion;

	void Clear() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "Clear");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxRegion::Clear();
	}
	bool IsEmpty() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "IsEmpty");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxRegion::IsEmpty();
	}
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxRegion::CreateGDIRefData();
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxRegion::CloneGDIRefData(a0);
	}
	bool DoIsEqual(const class wxRegion & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "DoIsEqual");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxRegion::DoIsEqual(a0);
	}
	bool DoGetBox(int & a0, int & a1, int & a2, int & a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "DoGetBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxRegion::DoGetBox(a0, a1, a2, a3);
	}
	enum wxRegionContain DoContainsPoint(int a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "DoContainsPoint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxRegionContain>::value) {
				static pybind11::detail::override_caster_t<enum wxRegionContain> caster;
				return pybind11::detail::cast_ref<enum wxRegionContain>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxRegionContain>(std::move(o));
		}
		return wxRegion::DoContainsPoint(a0, a1);
	}
	enum wxRegionContain DoContainsRect(const class wxRect & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "DoContainsRect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxRegionContain>::value) {
				static pybind11::detail::override_caster_t<enum wxRegionContain> caster;
				return pybind11::detail::cast_ref<enum wxRegionContain>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxRegionContain>(std::move(o));
		}
		return wxRegion::DoContainsRect(a0);
	}
	bool DoOffset(int a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "DoOffset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxRegion::DoOffset(a0, a1);
	}
	bool DoUnionWithRect(const class wxRect & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "DoUnionWithRect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxRegion::DoUnionWithRect(a0);
	}
	bool DoUnionWithRegion(const class wxRegion & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "DoUnionWithRegion");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxRegion::DoUnionWithRegion(a0);
	}
	bool DoIntersect(const class wxRegion & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "DoIntersect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxRegion::DoIntersect(a0);
	}
	bool DoSubtract(const class wxRegion & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "DoSubtract");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxRegion::DoSubtract(a0);
	}
	bool DoXor(const class wxRegion & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "DoXor");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxRegion::DoXor(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxRegion::GetClassInfo();
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "IsOk");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegion *>(this), "CloneRefData");
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

void bind_unknown_unknown_64(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxColourBase file: line:74
		pybind11::class_<wxColourBase, std::shared_ptr<wxColourBase>, PyCallBack_wxColourBase, wxGDIObject> cl(M(""), "wxColourBase", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxColourBase(); } ) );
		cl.def(pybind11::init<PyCallBack_wxColourBase const &>());
		cl.def("Set", [](wxColourBase &o, unsigned char const & a0, unsigned char const & a1, unsigned char const & a2) -> void { return o.Set(a0, a1, a2); }, "", pybind11::arg("red"), pybind11::arg("green"), pybind11::arg("blue"));
		cl.def("Set", (void (wxColourBase::*)(unsigned char, unsigned char, unsigned char, unsigned char)) &wxColourBase::Set, "C++: wxColourBase::Set(unsigned char, unsigned char, unsigned char, unsigned char) --> void", pybind11::arg("red"), pybind11::arg("green"), pybind11::arg("blue"), pybind11::arg("alpha"));
		cl.def("Set", (bool (wxColourBase::*)(const class wxString &)) &wxColourBase::Set, "C++: wxColourBase::Set(const class wxString &) --> bool", pybind11::arg("str"));
		cl.def("Set", (void (wxColourBase::*)(unsigned long)) &wxColourBase::Set, "C++: wxColourBase::Set(unsigned long) --> void", pybind11::arg("colRGB"));
		cl.def("Red", (unsigned char (wxColourBase::*)() const) &wxColourBase::Red, "C++: wxColourBase::Red() const --> unsigned char");
		cl.def("Green", (unsigned char (wxColourBase::*)() const) &wxColourBase::Green, "C++: wxColourBase::Green() const --> unsigned char");
		cl.def("Blue", (unsigned char (wxColourBase::*)() const) &wxColourBase::Blue, "C++: wxColourBase::Blue() const --> unsigned char");
		cl.def("Alpha", (unsigned char (wxColourBase::*)() const) &wxColourBase::Alpha, "C++: wxColourBase::Alpha() const --> unsigned char");
		cl.def("GetAsString", [](wxColourBase const &o) -> wxString { return o.GetAsString(); }, "");
		cl.def("GetAsString", (class wxString (wxColourBase::*)(long) const) &wxColourBase::GetAsString, "C++: wxColourBase::GetAsString(long) const --> class wxString", pybind11::arg("flags"));
		cl.def("SetRGB", (void (wxColourBase::*)(unsigned int)) &wxColourBase::SetRGB, "C++: wxColourBase::SetRGB(unsigned int) --> void", pybind11::arg("colRGB"));
		cl.def("SetRGBA", (void (wxColourBase::*)(unsigned int)) &wxColourBase::SetRGBA, "C++: wxColourBase::SetRGBA(unsigned int) --> void", pybind11::arg("colRGBA"));
		cl.def("GetRGB", (unsigned int (wxColourBase::*)() const) &wxColourBase::GetRGB, "C++: wxColourBase::GetRGB() const --> unsigned int");
		cl.def("GetRGBA", (unsigned int (wxColourBase::*)() const) &wxColourBase::GetRGBA, "C++: wxColourBase::GetRGBA() const --> unsigned int");
		cl.def_static("MakeMono", (void (*)(unsigned char *, unsigned char *, unsigned char *, bool)) &wxColourBase::MakeMono, "C++: wxColourBase::MakeMono(unsigned char *, unsigned char *, unsigned char *, bool) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("on"));
		cl.def_static("MakeDisabled", [](unsigned char * a0, unsigned char * a1, unsigned char * a2) -> void { return wxColourBase::MakeDisabled(a0, a1, a2); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def_static("MakeDisabled", (void (*)(unsigned char *, unsigned char *, unsigned char *, unsigned char)) &wxColourBase::MakeDisabled, "C++: wxColourBase::MakeDisabled(unsigned char *, unsigned char *, unsigned char *, unsigned char) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("brightness"));
		cl.def_static("MakeGrey", (void (*)(unsigned char *, unsigned char *, unsigned char *)) &wxColourBase::MakeGrey, "C++: wxColourBase::MakeGrey(unsigned char *, unsigned char *, unsigned char *) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def_static("MakeGrey", (void (*)(unsigned char *, unsigned char *, unsigned char *, double, double, double)) &wxColourBase::MakeGrey, "C++: wxColourBase::MakeGrey(unsigned char *, unsigned char *, unsigned char *, double, double, double) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("weight_r"), pybind11::arg("weight_g"), pybind11::arg("weight_b"));
		cl.def_static("AlphaBlend", (unsigned char (*)(unsigned char, unsigned char, double)) &wxColourBase::AlphaBlend, "C++: wxColourBase::AlphaBlend(unsigned char, unsigned char, double) --> unsigned char", pybind11::arg("fg"), pybind11::arg("bg"), pybind11::arg("alpha"));
		cl.def_static("ChangeLightness", (void (*)(unsigned char *, unsigned char *, unsigned char *, int)) &wxColourBase::ChangeLightness, "C++: wxColourBase::ChangeLightness(unsigned char *, unsigned char *, unsigned char *, int) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("ialpha"));
		cl.def("ChangeLightness", (class wxColour (wxColourBase::*)(int) const) &wxColourBase::ChangeLightness, "C++: wxColourBase::ChangeLightness(int) const --> class wxColour", pybind11::arg("ialpha"));
		cl.def("MakeDisabled", [](wxColourBase &o) -> wxColour & { return o.MakeDisabled(); }, "", pybind11::return_value_policy::automatic);
		cl.def("MakeDisabled", (class wxColour & (wxColourBase::*)(unsigned char)) &wxColourBase::MakeDisabled, "C++: wxColourBase::MakeDisabled(unsigned char) --> class wxColour &", pybind11::return_value_policy::automatic, pybind11::arg("brightness"));
		cl.def("assign", (class wxColourBase & (wxColourBase::*)(const class wxColourBase &)) &wxColourBase::operator=, "C++: wxColourBase::operator=(const class wxColourBase &) --> class wxColourBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxToString(const class wxColourBase &) file: line:211
	M("").def("wxToString", (class wxString (*)(const class wxColourBase &)) &wxToString, "C++: wxToString(const class wxColourBase &) --> class wxString", pybind11::arg("col"));

	// wxFromString(const class wxString &, class wxColourBase *) file: line:212
	M("").def("wxFromString", (bool (*)(const class wxString &, class wxColourBase *)) &wxFromString, "C++: wxFromString(const class wxString &, class wxColourBase *) --> bool", pybind11::arg("str"), pybind11::arg("col"));

	{ // wxColour file: line:20
		pybind11::class_<wxColour, std::shared_ptr<wxColour>, PyCallBack_wxColour, wxColourBase> cl(M(""), "wxColour", "");
		cl.def( pybind11::init( [](){ return new wxColour(); }, [](){ return new PyCallBack_wxColour(); } ) );
		cl.def( pybind11::init( [](unsigned char const & a0, unsigned char const & a1, unsigned char const & a2){ return new wxColour(a0, a1, a2); }, [](unsigned char const & a0, unsigned char const & a1, unsigned char const & a2){ return new PyCallBack_wxColour(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<unsigned char, unsigned char, unsigned char, unsigned char>(), pybind11::arg("red"), pybind11::arg("green"), pybind11::arg("blue"), pybind11::arg("alpha") );

		cl.def( pybind11::init<unsigned long>(), pybind11::arg("colRGB") );

		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("colourName") );

		cl.def( pybind11::init<const char *>(), pybind11::arg("colourName") );

		cl.def( pybind11::init<const wchar_t *>(), pybind11::arg("colourName") );

		cl.def( pybind11::init<const int &>(), pybind11::arg("gdkColor") );

		cl.def( pybind11::init( [](PyCallBack_wxColour const &o){ return new PyCallBack_wxColour(o); } ) );
		cl.def( pybind11::init( [](wxColour const &o){ return new wxColour(o); } ) );
		cl.def("__eq__", (bool (wxColour::*)(const class wxColour &) const) &wxColour::operator==, "C++: wxColour::operator==(const class wxColour &) const --> bool", pybind11::arg("col"));
		cl.def("__ne__", (bool (wxColour::*)(const class wxColour &) const) &wxColour::operator!=, "C++: wxColour::operator!=(const class wxColour &) const --> bool", pybind11::arg("col"));
		cl.def("Red", (unsigned char (wxColour::*)() const) &wxColour::Red, "C++: wxColour::Red() const --> unsigned char");
		cl.def("Green", (unsigned char (wxColour::*)() const) &wxColour::Green, "C++: wxColour::Green() const --> unsigned char");
		cl.def("Blue", (unsigned char (wxColour::*)() const) &wxColour::Blue, "C++: wxColour::Blue() const --> unsigned char");
		cl.def("Alpha", (unsigned char (wxColour::*)() const) &wxColour::Alpha, "C++: wxColour::Alpha() const --> unsigned char");
		cl.def("GetColor", (const int * (wxColour::*)() const) &wxColour::GetColor, "C++: wxColour::GetColor() const --> const int *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxColour::*)() const) &wxColour::GetClassInfo, "C++: wxColour::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxColour::wxCreateObject, "C++: wxColour::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxColour & (wxColour::*)(const class wxColour &)) &wxColour::operator=, "C++: wxColour::operator=(const class wxColour &) --> class wxColour &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxRegionContain file: line:26
	pybind11::enum_<wxRegionContain>(M(""), "wxRegionContain", pybind11::arithmetic(), "")
		.value("wxOutRegion", wxOutRegion)
		.value("wxPartRegion", wxPartRegion)
		.value("wxInRegion", wxInRegion)
		.export_values();

;

	// wxRegionOp file: line:35
	pybind11::enum_<wxRegionOp>(M(""), "wxRegionOp", pybind11::arithmetic(), "")
		.value("wxRGN_AND", wxRGN_AND)
		.value("wxRGN_COPY", wxRGN_COPY)
		.value("wxRGN_DIFF", wxRGN_DIFF)
		.value("wxRGN_OR", wxRGN_OR)
		.value("wxRGN_XOR", wxRGN_XOR)
		.export_values();

;

	{ // wxRegionBase file: line:57
		pybind11::class_<wxRegionBase, std::shared_ptr<wxRegionBase>, PyCallBack_wxRegionBase, wxGDIObject> cl(M(""), "wxRegionBase", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxRegionBase(); } ) );
		cl.def(pybind11::init<PyCallBack_wxRegionBase const &>());
		cl.def("__eq__", (bool (wxRegionBase::*)(const class wxRegion &) const) &wxRegionBase::operator==, "C++: wxRegionBase::operator==(const class wxRegion &) const --> bool", pybind11::arg("region"));
		cl.def("__ne__", (bool (wxRegionBase::*)(const class wxRegion &) const) &wxRegionBase::operator!=, "C++: wxRegionBase::operator!=(const class wxRegion &) const --> bool", pybind11::arg("region"));
		cl.def("IsEmpty", (bool (wxRegionBase::*)() const) &wxRegionBase::IsEmpty, "C++: wxRegionBase::IsEmpty() const --> bool");
		cl.def("Empty", (bool (wxRegionBase::*)() const) &wxRegionBase::Empty, "C++: wxRegionBase::Empty() const --> bool");
		cl.def("IsEqual", (bool (wxRegionBase::*)(const class wxRegion &) const) &wxRegionBase::IsEqual, "C++: wxRegionBase::IsEqual(const class wxRegion &) const --> bool", pybind11::arg("region"));
		cl.def("GetBox", (bool (wxRegionBase::*)(int &, int &, int &, int &) const) &wxRegionBase::GetBox, "C++: wxRegionBase::GetBox(int &, int &, int &, int &) const --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("GetBox", (class wxRect (wxRegionBase::*)() const) &wxRegionBase::GetBox, "C++: wxRegionBase::GetBox() const --> class wxRect");
		cl.def("Contains", (enum wxRegionContain (wxRegionBase::*)(int, int) const) &wxRegionBase::Contains, "C++: wxRegionBase::Contains(int, int) const --> enum wxRegionContain", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("Contains", (enum wxRegionContain (wxRegionBase::*)(const class wxPoint &) const) &wxRegionBase::Contains, "C++: wxRegionBase::Contains(const class wxPoint &) const --> enum wxRegionContain", pybind11::arg("pt"));
		cl.def("Contains", (enum wxRegionContain (wxRegionBase::*)(int, int, int, int) const) &wxRegionBase::Contains, "C++: wxRegionBase::Contains(int, int, int, int) const --> enum wxRegionContain", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("Contains", (enum wxRegionContain (wxRegionBase::*)(const class wxRect &) const) &wxRegionBase::Contains, "C++: wxRegionBase::Contains(const class wxRect &) const --> enum wxRegionContain", pybind11::arg("rect"));
		cl.def("Clear", (void (wxRegionBase::*)()) &wxRegionBase::Clear, "C++: wxRegionBase::Clear() --> void");
		cl.def("Offset", (bool (wxRegionBase::*)(int, int)) &wxRegionBase::Offset, "C++: wxRegionBase::Offset(int, int) --> bool", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("Offset", (bool (wxRegionBase::*)(const class wxPoint &)) &wxRegionBase::Offset, "C++: wxRegionBase::Offset(const class wxPoint &) --> bool", pybind11::arg("pt"));
		cl.def("Union", (bool (wxRegionBase::*)(int, int, int, int)) &wxRegionBase::Union, "C++: wxRegionBase::Union(int, int, int, int) --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("Union", (bool (wxRegionBase::*)(const class wxRect &)) &wxRegionBase::Union, "C++: wxRegionBase::Union(const class wxRect &) --> bool", pybind11::arg("rect"));
		cl.def("Union", (bool (wxRegionBase::*)(const class wxRegion &)) &wxRegionBase::Union, "C++: wxRegionBase::Union(const class wxRegion &) --> bool", pybind11::arg("region"));
		cl.def("Union", (bool (wxRegionBase::*)(const class wxBitmap &)) &wxRegionBase::Union, "C++: wxRegionBase::Union(const class wxBitmap &) --> bool", pybind11::arg("bmp"));
		cl.def("Union", [](wxRegionBase &o, const class wxBitmap & a0, const class wxColour & a1) -> bool { return o.Union(a0, a1); }, "", pybind11::arg("bmp"), pybind11::arg("transp"));
		cl.def("Union", (bool (wxRegionBase::*)(const class wxBitmap &, const class wxColour &, int)) &wxRegionBase::Union, "C++: wxRegionBase::Union(const class wxBitmap &, const class wxColour &, int) --> bool", pybind11::arg("bmp"), pybind11::arg("transp"), pybind11::arg("tolerance"));
		cl.def("Intersect", (bool (wxRegionBase::*)(int, int, int, int)) &wxRegionBase::Intersect, "C++: wxRegionBase::Intersect(int, int, int, int) --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("Intersect", (bool (wxRegionBase::*)(const class wxRect &)) &wxRegionBase::Intersect, "C++: wxRegionBase::Intersect(const class wxRect &) --> bool", pybind11::arg("rect"));
		cl.def("Intersect", (bool (wxRegionBase::*)(const class wxRegion &)) &wxRegionBase::Intersect, "C++: wxRegionBase::Intersect(const class wxRegion &) --> bool", pybind11::arg("region"));
		cl.def("Subtract", (bool (wxRegionBase::*)(int, int, int, int)) &wxRegionBase::Subtract, "C++: wxRegionBase::Subtract(int, int, int, int) --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("Subtract", (bool (wxRegionBase::*)(const class wxRect &)) &wxRegionBase::Subtract, "C++: wxRegionBase::Subtract(const class wxRect &) --> bool", pybind11::arg("rect"));
		cl.def("Subtract", (bool (wxRegionBase::*)(const class wxRegion &)) &wxRegionBase::Subtract, "C++: wxRegionBase::Subtract(const class wxRegion &) --> bool", pybind11::arg("region"));
		cl.def("Xor", (bool (wxRegionBase::*)(int, int, int, int)) &wxRegionBase::Xor, "C++: wxRegionBase::Xor(int, int, int, int) --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("Xor", (bool (wxRegionBase::*)(const class wxRect &)) &wxRegionBase::Xor, "C++: wxRegionBase::Xor(const class wxRect &) --> bool", pybind11::arg("rect"));
		cl.def("Xor", (bool (wxRegionBase::*)(const class wxRegion &)) &wxRegionBase::Xor, "C++: wxRegionBase::Xor(const class wxRegion &) --> bool", pybind11::arg("region"));
		cl.def("ConvertToBitmap", (class wxBitmap (wxRegionBase::*)() const) &wxRegionBase::ConvertToBitmap, "C++: wxRegionBase::ConvertToBitmap() const --> class wxBitmap");
		cl.def("assign", (class wxRegionBase & (wxRegionBase::*)(const class wxRegionBase &)) &wxRegionBase::operator=, "C++: wxRegionBase::operator=(const class wxRegionBase &) --> class wxRegionBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxRegion file: line:20
		pybind11::class_<wxRegion, std::shared_ptr<wxRegion>, PyCallBack_wxRegion, wxRegionBase> cl(M(""), "wxRegion", "");
		cl.def( pybind11::init( [](){ return new wxRegion(); }, [](){ return new PyCallBack_wxRegion(); } ) );
		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h") );

		cl.def( pybind11::init<const class wxPoint &, const class wxPoint &>(), pybind11::arg("topLeft"), pybind11::arg("bottomRight") );

		cl.def( pybind11::init<const class wxRect &>(), pybind11::arg("rect") );

		cl.def( pybind11::init( [](unsigned long const & a0, const class wxPoint * a1){ return new wxRegion(a0, a1); }, [](unsigned long const & a0, const class wxPoint * a1){ return new PyCallBack_wxRegion(a0, a1); } ), "doc");
		cl.def( pybind11::init<unsigned long, const class wxPoint *, enum wxPolygonFillMode>(), pybind11::arg("n"), pybind11::arg("points"), pybind11::arg("fillStyle") );

		cl.def( pybind11::init<const class wxBitmap &>(), pybind11::arg("bmp") );

		cl.def( pybind11::init( [](const class wxBitmap & a0, const class wxColour & a1){ return new wxRegion(a0, a1); }, [](const class wxBitmap & a0, const class wxColour & a1){ return new PyCallBack_wxRegion(a0, a1); } ), "doc");
		cl.def( pybind11::init<const class wxBitmap &, const class wxColour &, int>(), pybind11::arg("bmp"), pybind11::arg("transColour"), pybind11::arg("tolerance") );

		cl.def( pybind11::init( [](PyCallBack_wxRegion const &o){ return new PyCallBack_wxRegion(o); } ) );
		cl.def( pybind11::init( [](wxRegion const &o){ return new wxRegion(o); } ) );
		cl.def("Clear", (void (wxRegion::*)()) &wxRegion::Clear, "C++: wxRegion::Clear() --> void");
		cl.def("IsEmpty", (bool (wxRegion::*)() const) &wxRegion::IsEmpty, "C++: wxRegion::IsEmpty() const --> bool");
		cl.def("GetClassInfo", (class wxClassInfo * (wxRegion::*)() const) &wxRegion::GetClassInfo, "C++: wxRegion::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxRegion::wxCreateObject, "C++: wxRegion::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxRegion & (wxRegion::*)(const class wxRegion &)) &wxRegion::operator=, "C++: wxRegion::operator=(const class wxRegion &) --> class wxRegion &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
