#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/dc.h> // wxDC
#include <wx/dc.h> // wxDCImpl
#include <wx/dc.h> // wxDrawObject
#include <wx/dc.h> // wxFloodFillStyle
#include <wx/dc.h> // wxFontMetrics
#include <wx/dc.h> // wxMappingMode
#include <wx/dc.h> // wxRasterOperationMode
#include <wx/pen.h> // wxPenCap
#include <wx/pen.h> // wxPenJoin
#include <wx/pen.h> // wxPenStyle

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxIcon file: line:20
struct PyCallBack_wxIcon : public wxIcon {
	using wxIcon::wxIcon;

	bool LoadFile(const class wxString & a0, enum wxBitmapType a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIcon *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxIcon::LoadFile(a0, a1);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIcon *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxIcon::GetClassInfo();
	}
	int GetHeight() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIcon *>(this), "GetHeight");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxBitmap::GetHeight();
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIcon *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxBitmap::GetWidth();
	}
	int GetDepth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIcon *>(this), "GetDepth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxBitmap::GetDepth();
	}
	bool CopyFromIcon(const class wxIcon & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIcon *>(this), "CopyFromIcon");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxBitmap::CopyFromIcon(a0);
	}
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIcon *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxBitmap::CreateGDIRefData();
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIcon *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxBitmap::CloneGDIRefData(a0);
	}
};

// wxIconBundle file: line:27
struct PyCallBack_wxIconBundle : public wxIconBundle {
	using wxIconBundle::wxIconBundle;

	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconBundle *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxIconBundle::CreateGDIRefData();
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconBundle *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxIconBundle::CloneGDIRefData(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconBundle *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxIconBundle::GetClassInfo();
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconBundle *>(this), "IsOk");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconBundle *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconBundle *>(this), "CloneRefData");
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

// wxTopLevelWindowBase file: line:158
struct PyCallBack_wxTopLevelWindowBase : public wxTopLevelWindowBase {
	using wxTopLevelWindowBase::wxTopLevelWindowBase;

	void Maximize(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Maximize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTopLevelWindowBase::Maximize\"");
	}
	void Restore() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Restore");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTopLevelWindowBase::Restore\"");
	}
	void Iconize(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Iconize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTopLevelWindowBase::Iconize\"");
	}
	bool IsMaximized() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsMaximized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTopLevelWindowBase::IsMaximized\"");
	}
	bool IsAlwaysMaximized() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsAlwaysMaximized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowBase::IsAlwaysMaximized();
	}
	bool IsIconized() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsIconized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTopLevelWindowBase::IsIconized\"");
	}
	void SetIcons(const class wxIconBundle & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetIcons");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::SetIcons(a0);
	}
	bool ShowFullScreen(bool a0, long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ShowFullScreen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTopLevelWindowBase::ShowFullScreen\"");
	}
	void ShowWithoutActivating() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ShowWithoutActivating");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::ShowWithoutActivating();
	}
	bool IsFullScreen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsFullScreen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTopLevelWindowBase::IsFullScreen\"");
	}
	void SetTitle(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetTitle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTopLevelWindowBase::SetTitle\"");
	}
	class wxString GetTitle() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetTitle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTopLevelWindowBase::GetTitle\"");
	}
	bool EnableCloseButton(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "EnableCloseButton");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowBase::EnableCloseButton(a0);
	}
	void RequestUserAttention(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "RequestUserAttention");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::RequestUserAttention(a0);
	}
	bool IsActive() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsActive");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowBase::IsActive();
	}
	bool ShouldPreventAppExit() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ShouldPreventAppExit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowBase::ShouldPreventAppExit();
	}
	bool Destroy() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Destroy");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowBase::Destroy();
	}
	bool IsTopLevel() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsTopLevel");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowBase::IsTopLevel();
	}
	bool IsTopNavigationDomain() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsTopNavigationDomain");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowBase::IsTopNavigationDomain();
	}
	bool IsVisible() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsVisible");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowBase::IsVisible();
	}
	void GetRectForTopLevelChildren(int * a0, int * a1, int * a2, int * a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetRectForTopLevelChildren");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::GetRectForTopLevelChildren(a0, a1, a2, a3);
	}
	void DoUpdateWindowUI(class wxUpdateUIEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoUpdateWindowUI");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::DoUpdateWindowUI(a0);
	}
	void SetMinSize(const class wxSize & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetMinSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::SetMinSize(a0);
	}
	void SetMaxSize(const class wxSize & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetMaxSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::SetMaxSize(a0);
	}
	void OSXSetModified(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "OSXSetModified");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::OSXSetModified(a0);
	}
	bool OSXIsModified() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "OSXIsModified");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowBase::OSXIsModified();
	}
	void SetRepresentedFilename(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetRepresentedFilename");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::SetRepresentedFilename(a0);
	}
	void DoGiveHelp(const class wxString & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGiveHelp");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::DoGiveHelp(a0, a1);
	}
	void DoClientToScreen(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoClientToScreen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::DoClientToScreen(a0, a1);
	}
	void DoScreenToClient(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoScreenToClient");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::DoScreenToClient(a0, a1);
	}
	void DoCentre(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoCentre");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::DoCentre(a0);
	}
	void DoGetScreenPosition(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetScreenPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowBase::DoGetScreenPosition(a0, a1);
	}
	bool IsOneOfBars(const class wxWindow * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsOneOfBars");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowBase::IsOneOfBars(a0);
	}
	const struct wxEventTable * GetEventTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetEventTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct wxEventTable *>::value) {
				static pybind11::detail::override_caster_t<const struct wxEventTable *> caster;
				return pybind11::detail::cast_ref<const struct wxEventTable *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct wxEventTable *>(std::move(o));
		}
		return wxTopLevelWindowBase::GetEventTable();
	}
	class wxEventHashTable & GetEventHashTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetEventHashTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEventHashTable &>::value) {
				static pybind11::detail::override_caster_t<class wxEventHashTable &> caster;
				return pybind11::detail::cast_ref<class wxEventHashTable &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEventHashTable &>(std::move(o));
		}
		return wxTopLevelWindowBase::GetEventHashTable();
	}
	void GTKHandleRealized() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GTKHandleRealized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxNonOwnedWindow::GTKHandleRealized();
	}
	bool DoClearShape() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoClearShape");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxNonOwnedWindow::DoClearShape();
	}
	bool DoSetRegionShape(const class wxRegion & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoSetRegionShape");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxNonOwnedWindow::DoSetRegionShape(a0);
	}
	void AdjustForParentClientOrigin(int & a0, int & a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "AdjustForParentClientOrigin");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxNonOwnedWindowBase::AdjustForParentClientOrigin(a0, a1, a2);
	}
	void InheritAttributes() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "InheritAttributes");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxNonOwnedWindowBase::InheritAttributes();
	}
	void Raise() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Raise");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::Raise();
	}
	void Lower() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Lower");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::Lower();
	}
	bool Show(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Show");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::Show(a0);
	}
	bool IsShown() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsShown");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::IsShown();
	}
	bool IsRetained() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsRetained");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::IsRetained();
	}
	void SetFocus() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetFocus");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::SetFocus();
	}
	void SetCanFocus(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetCanFocus");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::SetCanFocus(a0);
	}
	bool Reparent(class wxWindowBase * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Reparent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::Reparent(a0);
	}
	void WarpPointer(int a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "WarpPointer");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::WarpPointer(a0, a1);
	}
	void Refresh(bool a0, const class wxRect * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Refresh");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::Refresh(a0, a1);
	}
	void Update() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Update");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::Update();
	}
	void ClearBackground() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ClearBackground");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::ClearBackground();
	}
	bool SetBackgroundColour(const class wxColour & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetBackgroundColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::SetBackgroundColour(a0);
	}
	bool SetForegroundColour(const class wxColour & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetForegroundColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::SetForegroundColour(a0);
	}
	bool SetCursor(const class wxCursor & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetCursor");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::SetCursor(a0);
	}
	bool SetFont(const class wxFont & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetFont");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::SetFont(a0);
	}
	bool SetBackgroundStyle(enum wxBackgroundStyle a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetBackgroundStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::SetBackgroundStyle(a0);
	}
	bool IsTransparentBackgroundSupported(class wxString * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsTransparentBackgroundSupported");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::IsTransparentBackgroundSupported(a0);
	}
	int GetCharHeight() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetCharHeight");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxWindow::GetCharHeight();
	}
	int GetCharWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetCharWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxWindow::GetCharWidth();
	}
	double GetContentScaleFactor() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetContentScaleFactor");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<double>::value) {
				static pybind11::detail::override_caster_t<double> caster;
				return pybind11::detail::cast_ref<double>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<double>(std::move(o));
		}
		return wxWindow::GetContentScaleFactor();
	}
	void SetScrollbar(int a0, int a1, int a2, int a3, bool a4) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetScrollbar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::SetScrollbar(a0, a1, a2, a3, a4);
	}
	void SetScrollPos(int a0, int a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetScrollPos");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::SetScrollPos(a0, a1, a2);
	}
	int GetScrollPos(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetScrollPos");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxWindow::GetScrollPos(a0);
	}
	int GetScrollThumb(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetScrollThumb");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxWindow::GetScrollThumb(a0);
	}
	int GetScrollRange(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetScrollRange");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxWindow::GetScrollRange(a0);
	}
	void ScrollWindow(int a0, int a1, const class wxRect * a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ScrollWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::ScrollWindow(a0, a1, a2);
	}
	bool ScrollLines(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ScrollLines");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::ScrollLines(a0);
	}
	bool ScrollPages(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ScrollPages");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::ScrollPages(a0);
	}
	void AddChild(class wxWindowBase * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "AddChild");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::AddChild(a0);
	}
	void RemoveChild(class wxWindowBase * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "RemoveChild");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::RemoveChild(a0);
	}
	void SetLayoutDirection(enum wxLayoutDirection a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetLayoutDirection");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::SetLayoutDirection(a0);
	}
	enum wxLayoutDirection GetLayoutDirection() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetLayoutDirection");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxLayoutDirection>::value) {
				static pybind11::detail::override_caster_t<enum wxLayoutDirection> caster;
				return pybind11::detail::cast_ref<enum wxLayoutDirection>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxLayoutDirection>(std::move(o));
		}
		return wxWindow::GetLayoutDirection();
	}
	int AdjustForLayoutDirection(int a0, int a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "AdjustForLayoutDirection");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxWindow::AdjustForLayoutDirection(a0, a1, a2);
	}
	bool DoIsExposed(int a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoIsExposed");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::DoIsExposed(a0, a1);
	}
	bool DoIsExposed(int a0, int a1, int a2, int a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoIsExposed");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::DoIsExposed(a0, a1, a2, a3);
	}
	bool IsDoubleBuffered() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsDoubleBuffered");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::IsDoubleBuffered();
	}
	void SetLabel(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetLabel");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::SetLabel(a0);
	}
	class wxString GetLabel() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetLabel");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxWindow::GetLabel();
	}
	void OnInternalIdle() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "OnInternalIdle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::OnInternalIdle();
	}
	bool GTKProcessEvent(class wxEvent & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GTKProcessEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::GTKProcessEvent(a0);
	}
	bool GTKNeedsToFilterSameWindowFocus() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GTKNeedsToFilterSameWindowFocus");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::GTKNeedsToFilterSameWindowFocus();
	}
	bool GTKWidgetNeedsMnemonic() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GTKWidgetNeedsMnemonic");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::GTKWidgetNeedsMnemonic();
	}
	void GTKWidgetDoSetMnemonic(int * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GTKWidgetDoSetMnemonic");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::GTKWidgetDoSetMnemonic(a0);
	}
	class wxWindow * GTKGetWindow(class wxArrayGdkWindows & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GTKGetWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxWindow *>::value) {
				static pybind11::detail::override_caster_t<class wxWindow *> caster;
				return pybind11::detail::cast_ref<class wxWindow *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxWindow *>(std::move(o));
		}
		return wxWindow::GTKGetWindow(a0);
	}
	void GTKApplyToolTip(const char * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GTKApplyToolTip");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::GTKApplyToolTip(a0);
	}
	bool GTKIsTransparentForMouse() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GTKIsTransparentForMouse");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::GTKIsTransparentForMouse();
	}
	void DoGetTextExtent(const class wxString & a0, int * a1, int * a2, int * a3, int * a4, const class wxFont * a5) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetTextExtent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4, a5);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoGetTextExtent(a0, a1, a2, a3, a4, a5);
	}
	void DoGetPosition(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoGetPosition(a0, a1);
	}
	void DoGetSize(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoGetSize(a0, a1);
	}
	void DoGetClientSize(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoGetClientSize(a0, a1);
	}
	void DoSetSize(int a0, int a1, int a2, int a3, int a4) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoSetSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoSetSize(a0, a1, a2, a3, a4);
	}
	void DoSetClientSize(int a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoSetClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoSetClientSize(a0, a1);
	}
	class wxSize DoGetBorderSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetBorderSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindow::DoGetBorderSize();
	}
	void DoMoveWindow(int a0, int a1, int a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoMoveWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoMoveWindow(a0, a1, a2, a3);
	}
	void DoEnable(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoEnable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoEnable(a0);
	}
	void DoCaptureMouse() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoCaptureMouse");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoCaptureMouse();
	}
	void DoReleaseMouse() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoReleaseMouse");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoReleaseMouse();
	}
	void DoFreeze() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoFreeze");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoFreeze();
	}
	void DoThaw() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoThaw");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoThaw();
	}
	bool DoNavigateIn(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoNavigateIn");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindow::DoNavigateIn(a0);
	}
	void DoApplyWidgetStyle(int * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoApplyWidgetStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoApplyWidgetStyle(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxWindow::GetClassInfo();
	}
	void SetName(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetName(a0);
	}
	class wxString GetName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxWindowBase::GetName();
	}
	class wxPoint GetClientAreaOrigin() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetClientAreaOrigin");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint>::value) {
				static pybind11::detail::override_caster_t<class wxPoint> caster;
				return pybind11::detail::cast_ref<class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint>(std::move(o));
		}
		return wxWindowBase::GetClientAreaOrigin();
	}
	class wxSize ClientToWindowSize(const class wxSize & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ClientToWindowSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::ClientToWindowSize(a0);
	}
	class wxSize WindowToClientSize(const class wxSize & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "WindowToClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::WindowToClientSize(a0);
	}
	class wxSize GetEffectiveMinSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetEffectiveMinSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::GetEffectiveMinSize();
	}
	void Fit() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Fit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::Fit();
	}
	void FitInside() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "FitInside");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::FitInside();
	}
	void SetSizeHints(int a0, int a1, int a2, int a3, int a4, int a5) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetSizeHints");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4, a5);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetSizeHints(a0, a1, a2, a3, a4, a5);
	}
	void SetVirtualSizeHints(int a0, int a1, int a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetVirtualSizeHints");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetVirtualSizeHints(a0, a1, a2, a3);
	}
	void SetMinClientSize(const class wxSize & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetMinClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetMinClientSize(a0);
	}
	void SetMaxClientSize(const class wxSize & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetMaxClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetMaxClientSize(a0);
	}
	class wxSize GetMinSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetMinSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::GetMinSize();
	}
	class wxSize GetMaxSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetMaxSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::GetMaxSize();
	}
	class wxSize GetMinClientSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetMinClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::GetMinClientSize();
	}
	class wxSize GetMaxClientSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetMaxClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::GetMaxClientSize();
	}
	void DoSetVirtualSize(int a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoSetVirtualSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::DoSetVirtualSize(a0, a1);
	}
	class wxSize DoGetVirtualSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetVirtualSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::DoGetVirtualSize();
	}
	class wxSize GetBestVirtualSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetBestVirtualSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::GetBestVirtualSize();
	}
	class wxSize GetWindowBorderSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetWindowBorderSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::GetWindowBorderSize();
	}
	bool InformFirstDirection(int a0, int a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "InformFirstDirection");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::InformFirstDirection(a0, a1, a2);
	}
	void SendSizeEvent(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SendSizeEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SendSizeEvent(a0);
	}
	bool BeginRepositioningChildren() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "BeginRepositioningChildren");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::BeginRepositioningChildren();
	}
	void EndRepositioningChildren() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "EndRepositioningChildren");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::EndRepositioningChildren();
	}
	bool ShowWithEffect(enum wxShowEffect a0, unsigned int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ShowWithEffect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::ShowWithEffect(a0, a1);
	}
	bool HideWithEffect(enum wxShowEffect a0, unsigned int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "HideWithEffect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::HideWithEffect(a0, a1);
	}
	bool Enable(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Enable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::Enable(a0);
	}
	bool IsShownOnScreen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsShownOnScreen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::IsShownOnScreen();
	}
	void SetWindowStyleFlag(long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetWindowStyleFlag");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetWindowStyleFlag(a0);
	}
	long GetWindowStyleFlag() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetWindowStyleFlag");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxWindowBase::GetWindowStyleFlag();
	}
	void SetExtraStyle(long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetExtraStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetExtraStyle(a0);
	}
	void MakeModal(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "MakeModal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::MakeModal(a0);
	}
	void SetThemeEnabled(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetThemeEnabled");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetThemeEnabled(a0);
	}
	bool GetThemeEnabled() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetThemeEnabled");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::GetThemeEnabled();
	}
	void SetFocusFromKbd() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetFocusFromKbd");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetFocusFromKbd();
	}
	bool HasFocus() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "HasFocus");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::HasFocus();
	}
	bool AcceptsFocus() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "AcceptsFocus");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::AcceptsFocus();
	}
	bool AcceptsFocusRecursively() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "AcceptsFocusRecursively");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::AcceptsFocusRecursively();
	}
	bool AcceptsFocusFromKeyboard() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "AcceptsFocusFromKeyboard");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::AcceptsFocusFromKeyboard();
	}
	bool CanBeFocused() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "CanBeFocused");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::CanBeFocused();
	}
	bool IsClientAreaChild(const class wxWindow * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsClientAreaChild");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::IsClientAreaChild(a0);
	}
	void SetNextHandler(class wxEvtHandler * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetNextHandler");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetNextHandler(a0);
	}
	void SetPreviousHandler(class wxEvtHandler * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetPreviousHandler");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetPreviousHandler(a0);
	}
	void SetValidator(const class wxValidator & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetValidator");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetValidator(a0);
	}
	class wxValidator * GetValidator() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetValidator");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxValidator *>::value) {
				static pybind11::detail::override_caster_t<class wxValidator *> caster;
				return pybind11::detail::cast_ref<class wxValidator *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxValidator *>(std::move(o));
		}
		return wxWindowBase::GetValidator();
	}
	bool Validate() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Validate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::Validate();
	}
	bool TransferDataToWindow() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "TransferDataToWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::TransferDataToWindow();
	}
	bool TransferDataFromWindow() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "TransferDataFromWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::TransferDataFromWindow();
	}
	void InitDialog() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "InitDialog");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::InitDialog();
	}
	void SetAcceleratorTable(const class wxAcceleratorTable & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetAcceleratorTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetAcceleratorTable(a0);
	}
	bool HasCapture() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "HasCapture");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::HasCapture();
	}
	void PrepareDC(class wxDC & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "PrepareDC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::PrepareDC(a0);
	}
	struct wxVisualAttributes GetDefaultAttributes() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetDefaultAttributes");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<struct wxVisualAttributes>::value) {
				static pybind11::detail::override_caster_t<struct wxVisualAttributes> caster;
				return pybind11::detail::cast_ref<struct wxVisualAttributes>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<struct wxVisualAttributes>(std::move(o));
		}
		return wxWindowBase::GetDefaultAttributes();
	}
	bool HasTransparentBackground() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "HasTransparentBackground");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::HasTransparentBackground();
	}
	void UpdateWindowUI(long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "UpdateWindowUI");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::UpdateWindowUI(a0);
	}
	bool HasMultiplePages() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "HasMultiplePages");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::HasMultiplePages();
	}
	bool CanScroll(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "CanScroll");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::CanScroll(a0);
	}
	void AlwaysShowScrollbars(bool a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "AlwaysShowScrollbars");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::AlwaysShowScrollbars(a0, a1);
	}
	bool IsScrollbarAlwaysShown(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "IsScrollbarAlwaysShown");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::IsScrollbarAlwaysShown(a0);
	}
	class wxString GetHelpTextAtPoint(const class wxPoint & a0, enum wxHelpEvent::Origin a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetHelpTextAtPoint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxWindowBase::GetHelpTextAtPoint(a0, a1);
	}
	void DragAcceptFiles(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DragAcceptFiles");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::DragAcceptFiles(a0);
	}
	void SetConstraintSizes(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetConstraintSizes");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetConstraintSizes(a0);
	}
	bool LayoutPhase1(int * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "LayoutPhase1");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::LayoutPhase1(a0);
	}
	bool LayoutPhase2(int * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "LayoutPhase2");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::LayoutPhase2(a0);
	}
	bool DoPhase(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoPhase");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::DoPhase(a0);
	}
	void SetSizeConstraint(int a0, int a1, int a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetSizeConstraint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetSizeConstraint(a0, a1, a2, a3);
	}
	void MoveConstraint(int a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "MoveConstraint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::MoveConstraint(a0, a1);
	}
	void GetSizeConstraint(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetSizeConstraint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::GetSizeConstraint(a0, a1);
	}
	void GetClientSizeConstraint(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetClientSizeConstraint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::GetClientSizeConstraint(a0, a1);
	}
	void GetPositionConstraint(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetPositionConstraint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::GetPositionConstraint(a0, a1);
	}
	bool Layout() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "Layout");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::Layout();
	}
	bool SetTransparent(unsigned char a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetTransparent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::SetTransparent(a0);
	}
	bool CanSetTransparent() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "CanSetTransparent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::CanSetTransparent();
	}
	bool SendIdleEvents(class wxIdleEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SendIdleEvents");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::SendIdleEvents(a0);
	}
	void AssociateHandle(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "AssociateHandle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::AssociateHandle(a0);
	}
	void DissociateHandle() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DissociateHandle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::DissociateHandle();
	}
	bool ShouldInheritColours() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ShouldInheritColours");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::ShouldInheritColours();
	}
	bool CanBeOutsideClientArea() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "CanBeOutsideClientArea");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::CanBeOutsideClientArea();
	}
	bool CanApplyThemeBorder() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "CanApplyThemeBorder");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::CanApplyThemeBorder();
	}
	class wxWindow * GetMainWindowOfCompositeControl() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetMainWindowOfCompositeControl");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxWindow *>::value) {
				static pybind11::detail::override_caster_t<class wxWindow *> caster;
				return pybind11::detail::cast_ref<class wxWindow *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxWindow *>(std::move(o));
		}
		return wxWindowBase::GetMainWindowOfCompositeControl();
	}
	bool TryBefore(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "TryBefore");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::TryBefore(a0);
	}
	bool TryAfter(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "TryAfter");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::TryAfter(a0);
	}
	enum wxBorder GetDefaultBorder() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetDefaultBorder");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxBorder>::value) {
				static pybind11::detail::override_caster_t<enum wxBorder> caster;
				return pybind11::detail::cast_ref<enum wxBorder>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxBorder>(std::move(o));
		}
		return wxWindowBase::GetDefaultBorder();
	}
	enum wxBorder GetDefaultBorderForControl() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "GetDefaultBorderForControl");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxBorder>::value) {
				static pybind11::detail::override_caster_t<enum wxBorder> caster;
				return pybind11::detail::cast_ref<enum wxBorder>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxBorder>(std::move(o));
		}
		return wxWindowBase::GetDefaultBorderForControl();
	}
	void SetInitialBestSize(const class wxSize & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SetInitialBestSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetInitialBestSize(a0);
	}
	enum wxHitTest DoHitTest(int a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoHitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxHitTest>::value) {
				static pybind11::detail::override_caster_t<enum wxHitTest> caster;
				return pybind11::detail::cast_ref<enum wxHitTest>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxHitTest>(std::move(o));
		}
		return wxWindowBase::DoHitTest(a0, a1);
	}
	class wxSize DoGetBestSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetBestSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::DoGetBestSize();
	}
	class wxSize DoGetBestClientSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetBestClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxWindowBase::DoGetBestClientSize();
	}
	int DoGetBestClientHeight(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetBestClientHeight");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxWindowBase::DoGetBestClientHeight(a0);
	}
	int DoGetBestClientWidth(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetBestClientWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxWindowBase::DoGetBestClientWidth(a0);
	}
	void DoSetSizeHints(int a0, int a1, int a2, int a3, int a4, int a5) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoSetSizeHints");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4, a5);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::DoSetSizeHints(a0, a1, a2, a3, a4, a5);
	}
	void DoSetWindowVariant(enum wxWindowVariant a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoSetWindowVariant");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::DoSetWindowVariant(a0);
	}
	bool ProcessEvent(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "ProcessEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::ProcessEvent(a0);
	}
	void QueueEvent(class wxEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "QueueEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::QueueEvent(a0);
	}
	void AddPendingEvent(const class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "AddPendingEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::AddPendingEvent(a0);
	}
	bool SearchEventTable(struct wxEventTable & a0, class wxEvent & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "SearchEventTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::SearchEventTable(a0, a1);
	}
	bool TryValidator(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "TryValidator");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::TryValidator(a0);
	}
	bool TryParent(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "TryParent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::TryParent(a0);
	}
	void DoSetClientObject(class wxClientData * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoSetClientObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::DoSetClientObject(a0);
	}
	class wxClientData * DoGetClientObject() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetClientObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClientData *>::value) {
				static pybind11::detail::override_caster_t<class wxClientData *> caster;
				return pybind11::detail::cast_ref<class wxClientData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClientData *>(std::move(o));
		}
		return wxEvtHandler::DoGetClientObject();
	}
	void DoSetClientData(void * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoSetClientData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::DoSetClientData(a0);
	}
	void * DoGetClientData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "DoGetClientData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return wxEvtHandler::DoGetClientData();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowBase *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CloneRefData(a0);
	}
};

void bind_unknown_unknown_76(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxIcon file: line:20
		pybind11::class_<wxIcon, std::shared_ptr<wxIcon>, PyCallBack_wxIcon, wxBitmap> cl(M(""), "wxIcon", "");
		cl.def( pybind11::init( [](){ return new wxIcon(); }, [](){ return new PyCallBack_wxIcon(); } ) );
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxIcon(a0); }, [](const class wxString & a0){ return new PyCallBack_wxIcon(a0); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, enum wxBitmapType const & a1){ return new wxIcon(a0, a1); }, [](const class wxString & a0, enum wxBitmapType const & a1){ return new PyCallBack_wxIcon(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, enum wxBitmapType const & a1, int const & a2){ return new wxIcon(a0, a1, a2); }, [](const class wxString & a0, enum wxBitmapType const & a1, int const & a2){ return new PyCallBack_wxIcon(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<const class wxString &, enum wxBitmapType, int, int>(), pybind11::arg("filename"), pybind11::arg("type"), pybind11::arg(""), pybind11::arg("") );

		cl.def( pybind11::init<const class wxIconLocation &>(), pybind11::arg("loc") );

		cl.def( pybind11::init( [](PyCallBack_wxIcon const &o){ return new PyCallBack_wxIcon(o); } ) );
		cl.def( pybind11::init( [](wxIcon const &o){ return new wxIcon(o); } ) );
		cl.def("LoadFile", (bool (wxIcon::*)(const class wxString &, enum wxBitmapType, int, int)) &wxIcon::LoadFile, "C++: wxIcon::LoadFile(const class wxString &, enum wxBitmapType, int, int) --> bool", pybind11::arg("name"), pybind11::arg("flags"), pybind11::arg(""), pybind11::arg(""));
		cl.def("LoadFile", [](wxIcon &o, const class wxString & a0) -> bool { return o.LoadFile(a0); }, "", pybind11::arg("name"));
		cl.def("LoadFile", (bool (wxIcon::*)(const class wxString &, enum wxBitmapType)) &wxIcon::LoadFile, "C++: wxIcon::LoadFile(const class wxString &, enum wxBitmapType) --> bool", pybind11::arg("name"), pybind11::arg("flags"));
		cl.def("CopyFromBitmap", (void (wxIcon::*)(const class wxBitmap &)) &wxIcon::CopyFromBitmap, "C++: wxIcon::CopyFromBitmap(const class wxBitmap &) --> void", pybind11::arg("bmp"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxIcon::*)() const) &wxIcon::GetClassInfo, "C++: wxIcon::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxIcon::wxCreateObject, "C++: wxIcon::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxIcon & (wxIcon::*)(const class wxIcon &)) &wxIcon::operator=, "C++: wxIcon::operator=(const class wxIcon &) --> class wxIcon &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxIconArray file: line:22
		pybind11::class_<wxIconArray, std::shared_ptr<wxIconArray>> cl(M(""), "wxIconArray", "");
		cl.def( pybind11::init( [](){ return new wxIconArray(); } ) );
		cl.def( pybind11::init( [](wxIconArray const &o){ return new wxIconArray(o); } ) );
		cl.def("assign", (class wxIconArray & (wxIconArray::*)(const class wxIconArray &)) &wxIconArray::operator=, "C++: wxIconArray::operator=(const class wxIconArray &) --> class wxIconArray &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("Alloc", (void (wxIconArray::*)(unsigned long)) &wxIconArray::Alloc, "C++: wxIconArray::Alloc(unsigned long) --> void", pybind11::arg("count"));
		cl.def("reserve", (void (wxIconArray::*)(unsigned long)) &wxIconArray::reserve, "C++: wxIconArray::reserve(unsigned long) --> void", pybind11::arg("count"));
		cl.def("GetCount", (unsigned long (wxIconArray::*)() const) &wxIconArray::GetCount, "C++: wxIconArray::GetCount() const --> unsigned long");
		cl.def("size", (unsigned long (wxIconArray::*)() const) &wxIconArray::size, "C++: wxIconArray::size() const --> unsigned long");
		cl.def("IsEmpty", (bool (wxIconArray::*)() const) &wxIconArray::IsEmpty, "C++: wxIconArray::IsEmpty() const --> bool");
		cl.def("empty", (bool (wxIconArray::*)() const) &wxIconArray::empty, "C++: wxIconArray::empty() const --> bool");
		cl.def("Count", (unsigned long (wxIconArray::*)() const) &wxIconArray::Count, "C++: wxIconArray::Count() const --> unsigned long");
		cl.def("Shrink", (void (wxIconArray::*)()) &wxIconArray::Shrink, "C++: wxIconArray::Shrink() --> void");
		cl.def("__getitem__", (class wxIcon & (wxIconArray::*)(unsigned long) const) &wxIconArray::operator[], "C++: wxIconArray::operator[](unsigned long) const --> class wxIcon &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (class wxIcon & (wxIconArray::*)(unsigned long) const) &wxIconArray::Item, "C++: wxIconArray::Item(unsigned long) const --> class wxIcon &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (class wxIcon & (wxIconArray::*)() const) &wxIconArray::Last, "C++: wxIconArray::Last() const --> class wxIcon &", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxIconArray const &o, const class wxIcon & a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxIconArray::*)(const class wxIcon &, bool) const) &wxIconArray::Index, "C++: wxIconArray::Index(const class wxIcon &, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxIconArray &o, const class wxIcon & a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxIconArray::*)(const class wxIcon &, unsigned long)) &wxIconArray::Add, "C++: wxIconArray::Add(const class wxIcon &, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Add", (void (wxIconArray::*)(const class wxIcon *)) &wxIconArray::Add, "C++: wxIconArray::Add(const class wxIcon *) --> void", pybind11::arg("pItem"));
		cl.def("push_back", (void (wxIconArray::*)(const class wxIcon *)) &wxIconArray::push_back, "C++: wxIconArray::push_back(const class wxIcon *) --> void", pybind11::arg("pItem"));
		cl.def("push_back", (void (wxIconArray::*)(const class wxIcon &)) &wxIconArray::push_back, "C++: wxIconArray::push_back(const class wxIcon &) --> void", pybind11::arg("lItem"));
		cl.def("Insert", [](wxIconArray &o, const class wxIcon & a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxIconArray::*)(const class wxIcon &, unsigned long, unsigned long)) &wxIconArray::Insert, "C++: wxIconArray::Insert(const class wxIcon &, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("Insert", (void (wxIconArray::*)(const class wxIcon *, unsigned long)) &wxIconArray::Insert, "C++: wxIconArray::Insert(const class wxIcon *, unsigned long) --> void", pybind11::arg("pItem"), pybind11::arg("uiIndex"));
		cl.def("Empty", (void (wxIconArray::*)()) &wxIconArray::Empty, "C++: wxIconArray::Empty() --> void");
		cl.def("Clear", (void (wxIconArray::*)()) &wxIconArray::Clear, "C++: wxIconArray::Clear() --> void");
		cl.def("Detach", (class wxIcon * (wxIconArray::*)(unsigned long)) &wxIconArray::Detach, "C++: wxIconArray::Detach(unsigned long) --> class wxIcon *", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("RemoveAt", [](wxIconArray &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxIconArray::*)(unsigned long, unsigned long)) &wxIconArray::RemoveAt, "C++: wxIconArray::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
	}
	{ // wxIconBundle file: line:27
		pybind11::class_<wxIconBundle, std::shared_ptr<wxIconBundle>, PyCallBack_wxIconBundle, wxGDIObject> cl(M(""), "wxIconBundle", "");
		cl.def( pybind11::init( [](){ return new wxIconBundle(); }, [](){ return new PyCallBack_wxIconBundle(); } ) );
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxIconBundle(a0); }, [](const class wxString & a0){ return new PyCallBack_wxIconBundle(a0); } ), "doc");
		cl.def( pybind11::init<const class wxString &, enum wxBitmapType>(), pybind11::arg("file"), pybind11::arg("type") );

		cl.def( pybind11::init( [](class wxInputStream & a0){ return new wxIconBundle(a0); }, [](class wxInputStream & a0){ return new PyCallBack_wxIconBundle(a0); } ), "doc");
		cl.def( pybind11::init<class wxInputStream &, enum wxBitmapType>(), pybind11::arg("stream"), pybind11::arg("type") );

		cl.def( pybind11::init<const class wxIcon &>(), pybind11::arg("icon") );

		cl.def( pybind11::init<const class wxString &, long>(), pybind11::arg("file"), pybind11::arg("type") );

		cl.def("AddIcon", [](wxIconBundle &o, const class wxString & a0) -> void { return o.AddIcon(a0); }, "", pybind11::arg("file"));
		cl.def("AddIcon", (void (wxIconBundle::*)(const class wxString &, enum wxBitmapType)) &wxIconBundle::AddIcon, "C++: wxIconBundle::AddIcon(const class wxString &, enum wxBitmapType) --> void", pybind11::arg("file"), pybind11::arg("type"));
		cl.def("AddIcon", [](wxIconBundle &o, class wxInputStream & a0) -> void { return o.AddIcon(a0); }, "", pybind11::arg("stream"));
		cl.def("AddIcon", (void (wxIconBundle::*)(class wxInputStream &, enum wxBitmapType)) &wxIconBundle::AddIcon, "C++: wxIconBundle::AddIcon(class wxInputStream &, enum wxBitmapType) --> void", pybind11::arg("stream"), pybind11::arg("type"));
		cl.def("AddIcon", (void (wxIconBundle::*)(const class wxIcon &)) &wxIconBundle::AddIcon, "C++: wxIconBundle::AddIcon(const class wxIcon &) --> void", pybind11::arg("icon"));
		cl.def("GetIcon", [](wxIconBundle const &o, const class wxSize & a0) -> wxIcon { return o.GetIcon(a0); }, "", pybind11::arg("size"));
		cl.def("GetIcon", (class wxIcon (wxIconBundle::*)(const class wxSize &, int) const) &wxIconBundle::GetIcon, "C++: wxIconBundle::GetIcon(const class wxSize &, int) const --> class wxIcon", pybind11::arg("size"), pybind11::arg("flags"));
		cl.def("GetIcon", [](wxIconBundle const &o) -> wxIcon { return o.GetIcon(); }, "");
		cl.def("GetIcon", [](wxIconBundle const &o, int const & a0) -> wxIcon { return o.GetIcon(a0); }, "", pybind11::arg("size"));
		cl.def("GetIcon", (class wxIcon (wxIconBundle::*)(int, int) const) &wxIconBundle::GetIcon, "C++: wxIconBundle::GetIcon(int, int) const --> class wxIcon", pybind11::arg("size"), pybind11::arg("flags"));
		cl.def("GetIconOfExactSize", (class wxIcon (wxIconBundle::*)(const class wxSize &) const) &wxIconBundle::GetIconOfExactSize, "C++: wxIconBundle::GetIconOfExactSize(const class wxSize &) const --> class wxIcon", pybind11::arg("size"));
		cl.def("GetIconOfExactSize", (class wxIcon (wxIconBundle::*)(int) const) &wxIconBundle::GetIconOfExactSize, "C++: wxIconBundle::GetIconOfExactSize(int) const --> class wxIcon", pybind11::arg("size"));
		cl.def("GetIconCount", (unsigned long (wxIconBundle::*)() const) &wxIconBundle::GetIconCount, "C++: wxIconBundle::GetIconCount() const --> unsigned long");
		cl.def("GetIconByIndex", (class wxIcon (wxIconBundle::*)(unsigned long) const) &wxIconBundle::GetIconByIndex, "C++: wxIconBundle::GetIconByIndex(unsigned long) const --> class wxIcon", pybind11::arg("n"));
		cl.def("IsEmpty", (bool (wxIconBundle::*)() const) &wxIconBundle::IsEmpty, "C++: wxIconBundle::IsEmpty() const --> bool");
		cl.def("AddIcon", (void (wxIconBundle::*)(const class wxString &, long)) &wxIconBundle::AddIcon, "C++: wxIconBundle::AddIcon(const class wxString &, long) --> void", pybind11::arg("file"), pybind11::arg("type"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxIconBundle::*)() const) &wxIconBundle::GetClassInfo, "C++: wxIconBundle::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxIconBundle::wxCreateObject, "C++: wxIconBundle::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxIconBundle & (wxIconBundle::*)(const class wxIconBundle &)) &wxIconBundle::operator=, "C++: wxIconBundle::operator=(const class wxIconBundle &) --> class wxIconBundle &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxTopLevelWindowBase file: line:158
		pybind11::class_<wxTopLevelWindowBase, std::shared_ptr<wxTopLevelWindowBase>, PyCallBack_wxTopLevelWindowBase, wxNonOwnedWindow> cl(M(""), "wxTopLevelWindowBase", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxTopLevelWindowBase(); } ) );
		cl.def("Maximize", [](wxTopLevelWindowBase &o) -> void { return o.Maximize(); }, "");
		cl.def("Maximize", (void (wxTopLevelWindowBase::*)(bool)) &wxTopLevelWindowBase::Maximize, "C++: wxTopLevelWindowBase::Maximize(bool) --> void", pybind11::arg("maximize"));
		cl.def("Restore", (void (wxTopLevelWindowBase::*)()) &wxTopLevelWindowBase::Restore, "C++: wxTopLevelWindowBase::Restore() --> void");
		cl.def("Iconize", [](wxTopLevelWindowBase &o) -> void { return o.Iconize(); }, "");
		cl.def("Iconize", (void (wxTopLevelWindowBase::*)(bool)) &wxTopLevelWindowBase::Iconize, "C++: wxTopLevelWindowBase::Iconize(bool) --> void", pybind11::arg("iconize"));
		cl.def("IsMaximized", (bool (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::IsMaximized, "C++: wxTopLevelWindowBase::IsMaximized() const --> bool");
		cl.def("IsAlwaysMaximized", (bool (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::IsAlwaysMaximized, "C++: wxTopLevelWindowBase::IsAlwaysMaximized() const --> bool");
		cl.def("IsIconized", (bool (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::IsIconized, "C++: wxTopLevelWindowBase::IsIconized() const --> bool");
		cl.def("GetIcon", (class wxIcon (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::GetIcon, "C++: wxTopLevelWindowBase::GetIcon() const --> class wxIcon");
		cl.def("GetIcons", (const class wxIconBundle & (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::GetIcons, "C++: wxTopLevelWindowBase::GetIcons() const --> const class wxIconBundle &", pybind11::return_value_policy::automatic);
		cl.def("SetIcon", (void (wxTopLevelWindowBase::*)(const class wxIcon &)) &wxTopLevelWindowBase::SetIcon, "C++: wxTopLevelWindowBase::SetIcon(const class wxIcon &) --> void", pybind11::arg("icon"));
		cl.def("SetIcons", (void (wxTopLevelWindowBase::*)(const class wxIconBundle &)) &wxTopLevelWindowBase::SetIcons, "C++: wxTopLevelWindowBase::SetIcons(const class wxIconBundle &) --> void", pybind11::arg("icons"));
		cl.def("ShowFullScreen", [](wxTopLevelWindowBase &o, bool const & a0) -> bool { return o.ShowFullScreen(a0); }, "", pybind11::arg("show"));
		cl.def("ShowFullScreen", (bool (wxTopLevelWindowBase::*)(bool, long)) &wxTopLevelWindowBase::ShowFullScreen, "C++: wxTopLevelWindowBase::ShowFullScreen(bool, long) --> bool", pybind11::arg("show"), pybind11::arg("style"));
		cl.def("ShowWithoutActivating", (void (wxTopLevelWindowBase::*)()) &wxTopLevelWindowBase::ShowWithoutActivating, "C++: wxTopLevelWindowBase::ShowWithoutActivating() --> void");
		cl.def("IsFullScreen", (bool (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::IsFullScreen, "C++: wxTopLevelWindowBase::IsFullScreen() const --> bool");
		cl.def("SetTitle", (void (wxTopLevelWindowBase::*)(const class wxString &)) &wxTopLevelWindowBase::SetTitle, "C++: wxTopLevelWindowBase::SetTitle(const class wxString &) --> void", pybind11::arg("title"));
		cl.def("GetTitle", (class wxString (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::GetTitle, "C++: wxTopLevelWindowBase::GetTitle() const --> class wxString");
		cl.def("EnableCloseButton", (bool (wxTopLevelWindowBase::*)(bool)) &wxTopLevelWindowBase::EnableCloseButton, "C++: wxTopLevelWindowBase::EnableCloseButton(bool) --> bool", pybind11::arg(""));
		cl.def("RequestUserAttention", [](wxTopLevelWindowBase &o) -> void { return o.RequestUserAttention(); }, "");
		cl.def("RequestUserAttention", (void (wxTopLevelWindowBase::*)(int)) &wxTopLevelWindowBase::RequestUserAttention, "C++: wxTopLevelWindowBase::RequestUserAttention(int) --> void", pybind11::arg("flags"));
		cl.def("IsActive", (bool (wxTopLevelWindowBase::*)()) &wxTopLevelWindowBase::IsActive, "C++: wxTopLevelWindowBase::IsActive() --> bool");
		cl.def("ShouldPreventAppExit", (bool (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::ShouldPreventAppExit, "C++: wxTopLevelWindowBase::ShouldPreventAppExit() const --> bool");
		cl.def("CentreOnScreen", [](wxTopLevelWindowBase &o) -> void { return o.CentreOnScreen(); }, "");
		cl.def("CentreOnScreen", (void (wxTopLevelWindowBase::*)(int)) &wxTopLevelWindowBase::CentreOnScreen, "C++: wxTopLevelWindowBase::CentreOnScreen(int) --> void", pybind11::arg("dir"));
		cl.def("CenterOnScreen", [](wxTopLevelWindowBase &o) -> void { return o.CenterOnScreen(); }, "");
		cl.def("CenterOnScreen", (void (wxTopLevelWindowBase::*)(int)) &wxTopLevelWindowBase::CenterOnScreen, "C++: wxTopLevelWindowBase::CenterOnScreen(int) --> void", pybind11::arg("dir"));
		cl.def_static("GetDefaultSize", (class wxSize (*)()) &wxTopLevelWindowBase::GetDefaultSize, "C++: wxTopLevelWindowBase::GetDefaultSize() --> class wxSize");
		cl.def("GetDefaultItem", (class wxWindow * (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::GetDefaultItem, "C++: wxTopLevelWindowBase::GetDefaultItem() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("SetDefaultItem", (class wxWindow * (wxTopLevelWindowBase::*)(class wxWindow *)) &wxTopLevelWindowBase::SetDefaultItem, "C++: wxTopLevelWindowBase::SetDefaultItem(class wxWindow *) --> class wxWindow *", pybind11::return_value_policy::automatic, pybind11::arg("win"));
		cl.def("GetTmpDefaultItem", (class wxWindow * (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::GetTmpDefaultItem, "C++: wxTopLevelWindowBase::GetTmpDefaultItem() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("SetTmpDefaultItem", (class wxWindow * (wxTopLevelWindowBase::*)(class wxWindow *)) &wxTopLevelWindowBase::SetTmpDefaultItem, "C++: wxTopLevelWindowBase::SetTmpDefaultItem(class wxWindow *) --> class wxWindow *", pybind11::return_value_policy::automatic, pybind11::arg("win"));
		cl.def("Destroy", (bool (wxTopLevelWindowBase::*)()) &wxTopLevelWindowBase::Destroy, "C++: wxTopLevelWindowBase::Destroy() --> bool");
		cl.def("IsTopLevel", (bool (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::IsTopLevel, "C++: wxTopLevelWindowBase::IsTopLevel() const --> bool");
		cl.def("IsTopNavigationDomain", (bool (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::IsTopNavigationDomain, "C++: wxTopLevelWindowBase::IsTopNavigationDomain() const --> bool");
		cl.def("IsVisible", (bool (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::IsVisible, "C++: wxTopLevelWindowBase::IsVisible() const --> bool");
		cl.def("OnCloseWindow", (void (wxTopLevelWindowBase::*)(class wxCloseEvent &)) &wxTopLevelWindowBase::OnCloseWindow, "C++: wxTopLevelWindowBase::OnCloseWindow(class wxCloseEvent &) --> void", pybind11::arg("event"));
		cl.def("OnSize", (void (wxTopLevelWindowBase::*)(class wxSizeEvent &)) &wxTopLevelWindowBase::OnSize, "C++: wxTopLevelWindowBase::OnSize(class wxSizeEvent &) --> void", pybind11::arg(""));
		cl.def("GetRectForTopLevelChildren", (void (wxTopLevelWindowBase::*)(int *, int *, int *, int *)) &wxTopLevelWindowBase::GetRectForTopLevelChildren, "C++: wxTopLevelWindowBase::GetRectForTopLevelChildren(int *, int *, int *, int *) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("OnActivate", (void (wxTopLevelWindowBase::*)(class wxActivateEvent &)) &wxTopLevelWindowBase::OnActivate, "C++: wxTopLevelWindowBase::OnActivate(class wxActivateEvent &) --> void", pybind11::arg(""));
		cl.def("DoUpdateWindowUI", (void (wxTopLevelWindowBase::*)(class wxUpdateUIEvent &)) &wxTopLevelWindowBase::DoUpdateWindowUI, "C++: wxTopLevelWindowBase::DoUpdateWindowUI(class wxUpdateUIEvent &) --> void", pybind11::arg("event"));
		cl.def("SetMinSize", (void (wxTopLevelWindowBase::*)(const class wxSize &)) &wxTopLevelWindowBase::SetMinSize, "C++: wxTopLevelWindowBase::SetMinSize(const class wxSize &) --> void", pybind11::arg("minSize"));
		cl.def("SetMaxSize", (void (wxTopLevelWindowBase::*)(const class wxSize &)) &wxTopLevelWindowBase::SetMaxSize, "C++: wxTopLevelWindowBase::SetMaxSize(const class wxSize &) --> void", pybind11::arg("maxSize"));
		cl.def("OSXSetModified", (void (wxTopLevelWindowBase::*)(bool)) &wxTopLevelWindowBase::OSXSetModified, "C++: wxTopLevelWindowBase::OSXSetModified(bool) --> void", pybind11::arg("modified"));
		cl.def("OSXIsModified", (bool (wxTopLevelWindowBase::*)() const) &wxTopLevelWindowBase::OSXIsModified, "C++: wxTopLevelWindowBase::OSXIsModified() const --> bool");
		cl.def("SetRepresentedFilename", (void (wxTopLevelWindowBase::*)(const class wxString &)) &wxTopLevelWindowBase::SetRepresentedFilename, "C++: wxTopLevelWindowBase::SetRepresentedFilename(const class wxString &) --> void", pybind11::arg(""));
		cl.def("DoGiveHelp", (void (wxTopLevelWindowBase::*)(const class wxString &, bool)) &wxTopLevelWindowBase::DoGiveHelp, "C++: wxTopLevelWindowBase::DoGiveHelp(const class wxString &, bool) --> void", pybind11::arg(""), pybind11::arg(""));
	}
}
