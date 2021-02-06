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

// wxTopLevelWindowGTK file: line:16
struct PyCallBack_wxTopLevelWindowGTK : public wxTopLevelWindowGTK {
	using wxTopLevelWindowGTK::wxTopLevelWindowGTK;

	void Maximize(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Maximize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::Maximize(a0);
	}
	bool IsMaximized() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsMaximized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::IsMaximized();
	}
	void Iconize(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Iconize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::Iconize(a0);
	}
	bool IsIconized() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsIconized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::IsIconized();
	}
	void SetIcons(const class wxIconBundle & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetIcons");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::SetIcons(a0);
	}
	void Restore() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Restore");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::Restore();
	}
	bool EnableCloseButton(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "EnableCloseButton");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::EnableCloseButton(a0);
	}
	void ShowWithoutActivating() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ShowWithoutActivating");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::ShowWithoutActivating();
	}
	bool ShowFullScreen(bool a0, long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ShowFullScreen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::ShowFullScreen(a0, a1);
	}
	bool IsFullScreen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsFullScreen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::IsFullScreen();
	}
	void RequestUserAttention(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "RequestUserAttention");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::RequestUserAttention(a0);
	}
	void SetWindowStyleFlag(long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetWindowStyleFlag");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::SetWindowStyleFlag(a0);
	}
	bool Show(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Show");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::Show(a0);
	}
	void Raise() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Raise");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::Raise();
	}
	bool IsActive() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsActive");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::IsActive();
	}
	void SetTitle(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetTitle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::SetTitle(a0);
	}
	class wxString GetTitle() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetTitle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxTopLevelWindowGTK::GetTitle();
	}
	void SetLabel(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetLabel");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::SetLabel(a0);
	}
	class wxString GetLabel() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetLabel");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxTopLevelWindowGTK::GetLabel();
	}
	bool SetTransparent(unsigned char a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetTransparent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::SetTransparent(a0);
	}
	bool CanSetTransparent() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "CanSetTransparent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::CanSetTransparent();
	}
	void AddGrab() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "AddGrab");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::AddGrab();
	}
	void RemoveGrab() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "RemoveGrab");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::RemoveGrab();
	}
	bool IsGrabbed() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsGrabbed");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::IsGrabbed();
	}
	void Refresh(bool a0, const class wxRect * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Refresh");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::Refresh(a0, a1);
	}
	void OnInternalIdle() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "OnInternalIdle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::OnInternalIdle();
	}
	void GTKHandleRealized() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GTKHandleRealized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::GTKHandleRealized();
	}
	void DoSetSizeHints(int a0, int a1, int a2, int a3, int a4, int a5) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoSetSizeHints");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4, a5);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::DoSetSizeHints(a0, a1, a2, a3, a4, a5);
	}
	void DoMoveWindow(int a0, int a1, int a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoMoveWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::DoMoveWindow(a0, a1, a2, a3);
	}
	void DoSetSize(int a0, int a1, int a2, int a3, int a4) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoSetSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::DoSetSize(a0, a1, a2, a3, a4);
	}
	void DoSetClientSize(int a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoSetClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::DoSetClientSize(a0, a1);
	}
	void DoGetClientSize(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::DoGetClientSize(a0, a1);
	}
	bool IsAlwaysMaximized() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsAlwaysMaximized");
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
	bool ShouldPreventAppExit() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ShouldPreventAppExit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Destroy");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsTopLevel");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsTopNavigationDomain");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsVisible");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetRectForTopLevelChildren");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoUpdateWindowUI");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetMinSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetMaxSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "OSXSetModified");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "OSXIsModified");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetRepresentedFilename");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGiveHelp");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoClientToScreen");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoScreenToClient");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoCentre");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetScreenPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsOneOfBars");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetEventTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetEventHashTable");
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
	bool DoClearShape() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoClearShape");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoSetRegionShape");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "AdjustForParentClientOrigin");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "InheritAttributes");
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
	void Lower() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Lower");
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
	bool IsShown() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsShown");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsRetained");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetCanFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Reparent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "WarpPointer");
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
	void Update() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Update");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ClearBackground");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetBackgroundColour");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetForegroundColour");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetCursor");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetFont");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetBackgroundStyle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsTransparentBackgroundSupported");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetCharHeight");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetCharWidth");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetContentScaleFactor");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetScrollbar");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetScrollPos");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetScrollPos");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetScrollThumb");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetScrollRange");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ScrollWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ScrollLines");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ScrollPages");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "AddChild");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "RemoveChild");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetLayoutDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetLayoutDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "AdjustForLayoutDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoIsExposed");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoIsExposed");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsDoubleBuffered");
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
	bool GTKProcessEvent(class wxEvent & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GTKProcessEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GTKNeedsToFilterSameWindowFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GTKWidgetNeedsMnemonic");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GTKWidgetDoSetMnemonic");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GTKGetWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GTKApplyToolTip");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GTKIsTransparentForMouse");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetTextExtent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetSize");
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
	class wxSize DoGetBorderSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetBorderSize");
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
	void DoEnable(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoEnable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoCaptureMouse");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoReleaseMouse");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoFreeze");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoThaw");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoNavigateIn");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoApplyWidgetStyle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetClassInfo");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetName");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetName");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetClientAreaOrigin");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ClientToWindowSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "WindowToClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetEffectiveMinSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Fit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "FitInside");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetSizeHints");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetVirtualSizeHints");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetMinClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetMaxClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetMinSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetMaxSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetMinClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetMaxClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoSetVirtualSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetVirtualSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetBestVirtualSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetWindowBorderSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "InformFirstDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SendSizeEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "BeginRepositioningChildren");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "EndRepositioningChildren");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ShowWithEffect");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "HideWithEffect");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Enable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsShownOnScreen");
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
	long GetWindowStyleFlag() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetWindowStyleFlag");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetExtraStyle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "MakeModal");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetThemeEnabled");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetThemeEnabled");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetFocusFromKbd");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "HasFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "AcceptsFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "AcceptsFocusRecursively");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "AcceptsFocusFromKeyboard");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "CanBeFocused");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsClientAreaChild");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetNextHandler");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetPreviousHandler");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Validate");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "TransferDataToWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "TransferDataFromWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "InitDialog");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetAcceleratorTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "HasCapture");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "PrepareDC");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetDefaultAttributes");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "HasTransparentBackground");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "UpdateWindowUI");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "HasMultiplePages");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "CanScroll");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "AlwaysShowScrollbars");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "IsScrollbarAlwaysShown");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetHelpTextAtPoint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DragAcceptFiles");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetConstraintSizes");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "LayoutPhase1");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "LayoutPhase2");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoPhase");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetSizeConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "MoveConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetSizeConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetClientSizeConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetPositionConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "Layout");
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
	bool SendIdleEvents(class wxIdleEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SendIdleEvents");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "AssociateHandle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DissociateHandle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ShouldInheritColours");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "CanBeOutsideClientArea");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "CanApplyThemeBorder");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetMainWindowOfCompositeControl");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "TryBefore");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "TryAfter");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetDefaultBorder");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "GetDefaultBorderForControl");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SetInitialBestSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoHitTest");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetBestSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetBestClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetBestClientHeight");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetBestClientWidth");
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
	void DoSetWindowVariant(enum wxWindowVariant a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoSetWindowVariant");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "ProcessEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "QueueEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "AddPendingEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "SearchEventTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "TryValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "TryParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoSetClientObject");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetClientObject");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoSetClientData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "DoGetClientData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindowGTK *>(this), "CloneRefData");
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

// wxTopLevelWindow file: line:395
struct PyCallBack_wxTopLevelWindow : public wxTopLevelWindow {
	using wxTopLevelWindow::wxTopLevelWindow;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxTopLevelWindow::GetClassInfo();
	}
	void Maximize(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Maximize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::Maximize(a0);
	}
	bool IsMaximized() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsMaximized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::IsMaximized();
	}
	void Iconize(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Iconize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::Iconize(a0);
	}
	bool IsIconized() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsIconized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::IsIconized();
	}
	void SetIcons(const class wxIconBundle & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetIcons");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::SetIcons(a0);
	}
	void Restore() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Restore");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::Restore();
	}
	bool EnableCloseButton(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "EnableCloseButton");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::EnableCloseButton(a0);
	}
	void ShowWithoutActivating() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ShowWithoutActivating");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::ShowWithoutActivating();
	}
	bool ShowFullScreen(bool a0, long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ShowFullScreen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::ShowFullScreen(a0, a1);
	}
	bool IsFullScreen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsFullScreen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::IsFullScreen();
	}
	void RequestUserAttention(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "RequestUserAttention");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::RequestUserAttention(a0);
	}
	void SetWindowStyleFlag(long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetWindowStyleFlag");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::SetWindowStyleFlag(a0);
	}
	bool Show(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Show");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::Show(a0);
	}
	void Raise() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Raise");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::Raise();
	}
	bool IsActive() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsActive");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::IsActive();
	}
	void SetTitle(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetTitle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::SetTitle(a0);
	}
	class wxString GetTitle() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetTitle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxTopLevelWindowGTK::GetTitle();
	}
	void SetLabel(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetLabel");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::SetLabel(a0);
	}
	class wxString GetLabel() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetLabel");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxTopLevelWindowGTK::GetLabel();
	}
	bool SetTransparent(unsigned char a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetTransparent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::SetTransparent(a0);
	}
	bool CanSetTransparent() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "CanSetTransparent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::CanSetTransparent();
	}
	void AddGrab() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "AddGrab");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::AddGrab();
	}
	void RemoveGrab() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "RemoveGrab");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::RemoveGrab();
	}
	bool IsGrabbed() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsGrabbed");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTopLevelWindowGTK::IsGrabbed();
	}
	void Refresh(bool a0, const class wxRect * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Refresh");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::Refresh(a0, a1);
	}
	void OnInternalIdle() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "OnInternalIdle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::OnInternalIdle();
	}
	void GTKHandleRealized() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GTKHandleRealized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::GTKHandleRealized();
	}
	void DoSetSizeHints(int a0, int a1, int a2, int a3, int a4, int a5) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoSetSizeHints");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4, a5);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::DoSetSizeHints(a0, a1, a2, a3, a4, a5);
	}
	void DoMoveWindow(int a0, int a1, int a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoMoveWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::DoMoveWindow(a0, a1, a2, a3);
	}
	void DoSetSize(int a0, int a1, int a2, int a3, int a4) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoSetSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::DoSetSize(a0, a1, a2, a3, a4);
	}
	void DoSetClientSize(int a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoSetClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::DoSetClientSize(a0, a1);
	}
	void DoGetClientSize(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetClientSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTopLevelWindowGTK::DoGetClientSize(a0, a1);
	}
	bool IsAlwaysMaximized() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsAlwaysMaximized");
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
	bool ShouldPreventAppExit() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ShouldPreventAppExit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Destroy");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsTopLevel");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsTopNavigationDomain");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsVisible");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetRectForTopLevelChildren");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoUpdateWindowUI");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetMinSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetMaxSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "OSXSetModified");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "OSXIsModified");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetRepresentedFilename");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGiveHelp");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoClientToScreen");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoScreenToClient");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoCentre");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetScreenPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsOneOfBars");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetEventTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetEventHashTable");
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
	bool DoClearShape() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoClearShape");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoSetRegionShape");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "AdjustForParentClientOrigin");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "InheritAttributes");
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
	void Lower() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Lower");
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
	bool IsShown() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsShown");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsRetained");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetCanFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Reparent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "WarpPointer");
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
	void Update() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Update");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ClearBackground");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetBackgroundColour");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetForegroundColour");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetCursor");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetFont");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetBackgroundStyle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsTransparentBackgroundSupported");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetCharHeight");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetCharWidth");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetContentScaleFactor");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetScrollbar");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetScrollPos");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetScrollPos");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetScrollThumb");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetScrollRange");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ScrollWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ScrollLines");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ScrollPages");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "AddChild");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "RemoveChild");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetLayoutDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetLayoutDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "AdjustForLayoutDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoIsExposed");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoIsExposed");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsDoubleBuffered");
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
	bool GTKProcessEvent(class wxEvent & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GTKProcessEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GTKNeedsToFilterSameWindowFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GTKWidgetNeedsMnemonic");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GTKWidgetDoSetMnemonic");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GTKGetWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GTKApplyToolTip");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GTKIsTransparentForMouse");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetTextExtent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetSize");
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
	class wxSize DoGetBorderSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetBorderSize");
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
	void DoEnable(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoEnable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoCaptureMouse");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoReleaseMouse");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoFreeze");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoThaw");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoNavigateIn");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoApplyWidgetStyle");
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
	void SetName(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetName");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetName");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetClientAreaOrigin");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ClientToWindowSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "WindowToClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetEffectiveMinSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Fit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "FitInside");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetSizeHints");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetVirtualSizeHints");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetMinClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetMaxClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetMinSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetMaxSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetMinClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetMaxClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoSetVirtualSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetVirtualSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetBestVirtualSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetWindowBorderSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "InformFirstDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SendSizeEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "BeginRepositioningChildren");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "EndRepositioningChildren");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ShowWithEffect");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "HideWithEffect");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Enable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsShownOnScreen");
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
	long GetWindowStyleFlag() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetWindowStyleFlag");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetExtraStyle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "MakeModal");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetThemeEnabled");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetThemeEnabled");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetFocusFromKbd");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "HasFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "AcceptsFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "AcceptsFocusRecursively");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "AcceptsFocusFromKeyboard");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "CanBeFocused");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsClientAreaChild");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetNextHandler");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetPreviousHandler");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Validate");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "TransferDataToWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "TransferDataFromWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "InitDialog");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetAcceleratorTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "HasCapture");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "PrepareDC");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetDefaultAttributes");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "HasTransparentBackground");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "UpdateWindowUI");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "HasMultiplePages");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "CanScroll");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "AlwaysShowScrollbars");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "IsScrollbarAlwaysShown");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetHelpTextAtPoint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DragAcceptFiles");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetConstraintSizes");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "LayoutPhase1");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "LayoutPhase2");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoPhase");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetSizeConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "MoveConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetSizeConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetClientSizeConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetPositionConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "Layout");
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
	bool SendIdleEvents(class wxIdleEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SendIdleEvents");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "AssociateHandle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DissociateHandle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ShouldInheritColours");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "CanBeOutsideClientArea");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "CanApplyThemeBorder");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetMainWindowOfCompositeControl");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "TryBefore");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "TryAfter");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetDefaultBorder");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "GetDefaultBorderForControl");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SetInitialBestSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoHitTest");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetBestSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetBestClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetBestClientHeight");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetBestClientWidth");
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
	void DoSetWindowVariant(enum wxWindowVariant a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoSetWindowVariant");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "ProcessEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "QueueEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "AddPendingEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "SearchEventTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "TryValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "TryParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoSetClientObject");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetClientObject");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoSetClientData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "DoGetClientData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTopLevelWindow *>(this), "CloneRefData");
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

// wxControlContainerBase file: line:36
struct PyCallBack_wxControlContainerBase : public wxControlContainerBase {
	using wxControlContainerBase::wxControlContainerBase;

	bool SetFocusToChild() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxControlContainerBase *>(this), "SetFocusToChild");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxControlContainerBase::SetFocusToChild();
	}
};

void bind_unknown_unknown_77(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxTopLevelWindowGTK file: line:16
		pybind11::class_<wxTopLevelWindowGTK, std::shared_ptr<wxTopLevelWindowGTK>, PyCallBack_wxTopLevelWindowGTK, wxTopLevelWindowBase> cl(M(""), "wxTopLevelWindowGTK", "");
		cl.def( pybind11::init( [](){ return new wxTopLevelWindowGTK(); }, [](){ return new PyCallBack_wxTopLevelWindowGTK(); } ) );
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2){ return new wxTopLevelWindowGTK(a0, a1, a2); }, [](class wxWindow * a0, int const & a1, const class wxString & a2){ return new PyCallBack_wxTopLevelWindowGTK(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3){ return new wxTopLevelWindowGTK(a0, a1, a2, a3); }, [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3){ return new PyCallBack_wxTopLevelWindowGTK(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4){ return new wxTopLevelWindowGTK(a0, a1, a2, a3, a4); }, [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4){ return new PyCallBack_wxTopLevelWindowGTK(a0, a1, a2, a3, a4); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4, long const & a5){ return new wxTopLevelWindowGTK(a0, a1, a2, a3, a4, a5); }, [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4, long const & a5){ return new PyCallBack_wxTopLevelWindowGTK(a0, a1, a2, a3, a4, a5); } ), "doc");
		cl.def( pybind11::init<class wxWindow *, int, const class wxString &, const class wxPoint &, const class wxSize &, long, const class wxString &>(), pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"), pybind11::arg("pos"), pybind11::arg("size"), pybind11::arg("style"), pybind11::arg("name") );

		cl.def( pybind11::init( [](PyCallBack_wxTopLevelWindowGTK const &o){ return new PyCallBack_wxTopLevelWindowGTK(o); } ) );
		cl.def( pybind11::init( [](wxTopLevelWindowGTK const &o){ return new wxTopLevelWindowGTK(o); } ) );
		cl.def_readwrite("m_fsIsShowing", &wxTopLevelWindowGTK::m_fsIsShowing);
		cl.def_readwrite("m_fsSaveGdkFunc", &wxTopLevelWindowGTK::m_fsSaveGdkFunc);
		cl.def_readwrite("m_fsSaveGdkDecor", &wxTopLevelWindowGTK::m_fsSaveGdkDecor);
		cl.def_readwrite("m_fsSaveFrame", &wxTopLevelWindowGTK::m_fsSaveFrame);
		cl.def_readwrite("m_gdkFunc", &wxTopLevelWindowGTK::m_gdkFunc);
		cl.def_readwrite("m_gdkDecor", &wxTopLevelWindowGTK::m_gdkDecor);
		cl.def_readwrite("m_decorSize", &wxTopLevelWindowGTK::m_decorSize);
		cl.def_readwrite("m_urgency_hint", &wxTopLevelWindowGTK::m_urgency_hint);
		cl.def_readwrite("m_netFrameExtentsTimerId", &wxTopLevelWindowGTK::m_netFrameExtentsTimerId);
		cl.def("Create", [](wxTopLevelWindowGTK &o, class wxWindow * a0, int const & a1, const class wxString & a2) -> bool { return o.Create(a0, a1, a2); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"));
		cl.def("Create", [](wxTopLevelWindowGTK &o, class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3) -> bool { return o.Create(a0, a1, a2, a3); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"), pybind11::arg("pos"));
		cl.def("Create", [](wxTopLevelWindowGTK &o, class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4) -> bool { return o.Create(a0, a1, a2, a3, a4); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"), pybind11::arg("pos"), pybind11::arg("size"));
		cl.def("Create", [](wxTopLevelWindowGTK &o, class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4, long const & a5) -> bool { return o.Create(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"), pybind11::arg("pos"), pybind11::arg("size"), pybind11::arg("style"));
		cl.def("Create", (bool (wxTopLevelWindowGTK::*)(class wxWindow *, int, const class wxString &, const class wxPoint &, const class wxSize &, long, const class wxString &)) &wxTopLevelWindowGTK::Create, "C++: wxTopLevelWindowGTK::Create(class wxWindow *, int, const class wxString &, const class wxPoint &, const class wxSize &, long, const class wxString &) --> bool", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"), pybind11::arg("pos"), pybind11::arg("size"), pybind11::arg("style"), pybind11::arg("name"));
		cl.def("Maximize", [](wxTopLevelWindowGTK &o) -> void { return o.Maximize(); }, "");
		cl.def("Maximize", (void (wxTopLevelWindowGTK::*)(bool)) &wxTopLevelWindowGTK::Maximize, "C++: wxTopLevelWindowGTK::Maximize(bool) --> void", pybind11::arg("maximize"));
		cl.def("IsMaximized", (bool (wxTopLevelWindowGTK::*)() const) &wxTopLevelWindowGTK::IsMaximized, "C++: wxTopLevelWindowGTK::IsMaximized() const --> bool");
		cl.def("Iconize", [](wxTopLevelWindowGTK &o) -> void { return o.Iconize(); }, "");
		cl.def("Iconize", (void (wxTopLevelWindowGTK::*)(bool)) &wxTopLevelWindowGTK::Iconize, "C++: wxTopLevelWindowGTK::Iconize(bool) --> void", pybind11::arg("iconize"));
		cl.def("IsIconized", (bool (wxTopLevelWindowGTK::*)() const) &wxTopLevelWindowGTK::IsIconized, "C++: wxTopLevelWindowGTK::IsIconized() const --> bool");
		cl.def("SetIcons", (void (wxTopLevelWindowGTK::*)(const class wxIconBundle &)) &wxTopLevelWindowGTK::SetIcons, "C++: wxTopLevelWindowGTK::SetIcons(const class wxIconBundle &) --> void", pybind11::arg("icons"));
		cl.def("Restore", (void (wxTopLevelWindowGTK::*)()) &wxTopLevelWindowGTK::Restore, "C++: wxTopLevelWindowGTK::Restore() --> void");
		cl.def("EnableCloseButton", [](wxTopLevelWindowGTK &o) -> bool { return o.EnableCloseButton(); }, "");
		cl.def("EnableCloseButton", (bool (wxTopLevelWindowGTK::*)(bool)) &wxTopLevelWindowGTK::EnableCloseButton, "C++: wxTopLevelWindowGTK::EnableCloseButton(bool) --> bool", pybind11::arg("enable"));
		cl.def("ShowWithoutActivating", (void (wxTopLevelWindowGTK::*)()) &wxTopLevelWindowGTK::ShowWithoutActivating, "C++: wxTopLevelWindowGTK::ShowWithoutActivating() --> void");
		cl.def("ShowFullScreen", [](wxTopLevelWindowGTK &o, bool const & a0) -> bool { return o.ShowFullScreen(a0); }, "", pybind11::arg("show"));
		cl.def("ShowFullScreen", (bool (wxTopLevelWindowGTK::*)(bool, long)) &wxTopLevelWindowGTK::ShowFullScreen, "C++: wxTopLevelWindowGTK::ShowFullScreen(bool, long) --> bool", pybind11::arg("show"), pybind11::arg("style"));
		cl.def("IsFullScreen", (bool (wxTopLevelWindowGTK::*)() const) &wxTopLevelWindowGTK::IsFullScreen, "C++: wxTopLevelWindowGTK::IsFullScreen() const --> bool");
		cl.def("RequestUserAttention", [](wxTopLevelWindowGTK &o) -> void { return o.RequestUserAttention(); }, "");
		cl.def("RequestUserAttention", (void (wxTopLevelWindowGTK::*)(int)) &wxTopLevelWindowGTK::RequestUserAttention, "C++: wxTopLevelWindowGTK::RequestUserAttention(int) --> void", pybind11::arg("flags"));
		cl.def("SetWindowStyleFlag", (void (wxTopLevelWindowGTK::*)(long)) &wxTopLevelWindowGTK::SetWindowStyleFlag, "C++: wxTopLevelWindowGTK::SetWindowStyleFlag(long) --> void", pybind11::arg("style"));
		cl.def("Show", [](wxTopLevelWindowGTK &o) -> bool { return o.Show(); }, "");
		cl.def("Show", (bool (wxTopLevelWindowGTK::*)(bool)) &wxTopLevelWindowGTK::Show, "C++: wxTopLevelWindowGTK::Show(bool) --> bool", pybind11::arg("show"));
		cl.def("Raise", (void (wxTopLevelWindowGTK::*)()) &wxTopLevelWindowGTK::Raise, "C++: wxTopLevelWindowGTK::Raise() --> void");
		cl.def("IsActive", (bool (wxTopLevelWindowGTK::*)()) &wxTopLevelWindowGTK::IsActive, "C++: wxTopLevelWindowGTK::IsActive() --> bool");
		cl.def("SetTitle", (void (wxTopLevelWindowGTK::*)(const class wxString &)) &wxTopLevelWindowGTK::SetTitle, "C++: wxTopLevelWindowGTK::SetTitle(const class wxString &) --> void", pybind11::arg("title"));
		cl.def("GetTitle", (class wxString (wxTopLevelWindowGTK::*)() const) &wxTopLevelWindowGTK::GetTitle, "C++: wxTopLevelWindowGTK::GetTitle() const --> class wxString");
		cl.def("SetLabel", (void (wxTopLevelWindowGTK::*)(const class wxString &)) &wxTopLevelWindowGTK::SetLabel, "C++: wxTopLevelWindowGTK::SetLabel(const class wxString &) --> void", pybind11::arg("label"));
		cl.def("GetLabel", (class wxString (wxTopLevelWindowGTK::*)() const) &wxTopLevelWindowGTK::GetLabel, "C++: wxTopLevelWindowGTK::GetLabel() const --> class wxString");
		cl.def("SetTransparent", (bool (wxTopLevelWindowGTK::*)(unsigned char)) &wxTopLevelWindowGTK::SetTransparent, "C++: wxTopLevelWindowGTK::SetTransparent(unsigned char) --> bool", pybind11::arg("alpha"));
		cl.def("CanSetTransparent", (bool (wxTopLevelWindowGTK::*)()) &wxTopLevelWindowGTK::CanSetTransparent, "C++: wxTopLevelWindowGTK::CanSetTransparent() --> bool");
		cl.def("AddGrab", (void (wxTopLevelWindowGTK::*)()) &wxTopLevelWindowGTK::AddGrab, "C++: wxTopLevelWindowGTK::AddGrab() --> void");
		cl.def("RemoveGrab", (void (wxTopLevelWindowGTK::*)()) &wxTopLevelWindowGTK::RemoveGrab, "C++: wxTopLevelWindowGTK::RemoveGrab() --> void");
		cl.def("IsGrabbed", (bool (wxTopLevelWindowGTK::*)() const) &wxTopLevelWindowGTK::IsGrabbed, "C++: wxTopLevelWindowGTK::IsGrabbed() const --> bool");
		cl.def("Refresh", [](wxTopLevelWindowGTK &o) -> void { return o.Refresh(); }, "");
		cl.def("Refresh", [](wxTopLevelWindowGTK &o, bool const & a0) -> void { return o.Refresh(a0); }, "", pybind11::arg("eraseBackground"));
		cl.def("Refresh", (void (wxTopLevelWindowGTK::*)(bool, const class wxRect *)) &wxTopLevelWindowGTK::Refresh, "C++: wxTopLevelWindowGTK::Refresh(bool, const class wxRect *) --> void", pybind11::arg("eraseBackground"), pybind11::arg("rect"));
		cl.def("OnInternalIdle", (void (wxTopLevelWindowGTK::*)()) &wxTopLevelWindowGTK::OnInternalIdle, "C++: wxTopLevelWindowGTK::OnInternalIdle() --> void");
		cl.def("GTKHandleRealized", (void (wxTopLevelWindowGTK::*)()) &wxTopLevelWindowGTK::GTKHandleRealized, "C++: wxTopLevelWindowGTK::GTKHandleRealized() --> void");
		cl.def("GTKConfigureEvent", (void (wxTopLevelWindowGTK::*)(int, int)) &wxTopLevelWindowGTK::GTKConfigureEvent, "C++: wxTopLevelWindowGTK::GTKConfigureEvent(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("SetIconizeState", (void (wxTopLevelWindowGTK::*)(bool)) &wxTopLevelWindowGTK::SetIconizeState, "C++: wxTopLevelWindowGTK::SetIconizeState(bool) --> void", pybind11::arg("iconic"));
		cl.def("GTKDoGetSize", (void (wxTopLevelWindowGTK::*)(int *, int *) const) &wxTopLevelWindowGTK::GTKDoGetSize, "C++: wxTopLevelWindowGTK::GTKDoGetSize(int *, int *) const --> void", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("GTKUpdateDecorSize", (void (wxTopLevelWindowGTK::*)(const struct wxTopLevelWindowGTK::DecorSize &)) &wxTopLevelWindowGTK::GTKUpdateDecorSize, "C++: wxTopLevelWindowGTK::GTKUpdateDecorSize(const struct wxTopLevelWindowGTK::DecorSize &) --> void", pybind11::arg("decorSize"));
		cl.def("assign", (class wxTopLevelWindowGTK & (wxTopLevelWindowGTK::*)(const class wxTopLevelWindowGTK &)) &wxTopLevelWindowGTK::operator=, "C++: wxTopLevelWindowGTK::operator=(const class wxTopLevelWindowGTK &) --> class wxTopLevelWindowGTK &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxTopLevelWindowGTK::DecorSize file: line:113
			auto & enclosing_class = cl;
			pybind11::class_<wxTopLevelWindowGTK::DecorSize, std::shared_ptr<wxTopLevelWindowGTK::DecorSize>> cl(enclosing_class, "DecorSize", "");
			cl.def( pybind11::init( [](){ return new wxTopLevelWindowGTK::DecorSize(); } ) );
			cl.def( pybind11::init( [](wxTopLevelWindowGTK::DecorSize const &o){ return new wxTopLevelWindowGTK::DecorSize(o); } ) );
			cl.def_readwrite("left", &wxTopLevelWindowGTK::DecorSize::left);
			cl.def_readwrite("right", &wxTopLevelWindowGTK::DecorSize::right);
			cl.def_readwrite("top", &wxTopLevelWindowGTK::DecorSize::top);
			cl.def_readwrite("bottom", &wxTopLevelWindowGTK::DecorSize::bottom);
		}

	}
	{ // wxTopLevelWindow file: line:395
		pybind11::class_<wxTopLevelWindow, std::shared_ptr<wxTopLevelWindow>, PyCallBack_wxTopLevelWindow, wxTopLevelWindowGTK> cl(M(""), "wxTopLevelWindow", "");
		cl.def( pybind11::init( [](){ return new wxTopLevelWindow(); }, [](){ return new PyCallBack_wxTopLevelWindow(); } ) );
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2){ return new wxTopLevelWindow(a0, a1, a2); }, [](class wxWindow * a0, int const & a1, const class wxString & a2){ return new PyCallBack_wxTopLevelWindow(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3){ return new wxTopLevelWindow(a0, a1, a2, a3); }, [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3){ return new PyCallBack_wxTopLevelWindow(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4){ return new wxTopLevelWindow(a0, a1, a2, a3, a4); }, [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4){ return new PyCallBack_wxTopLevelWindow(a0, a1, a2, a3, a4); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4, long const & a5){ return new wxTopLevelWindow(a0, a1, a2, a3, a4, a5); }, [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4, long const & a5){ return new PyCallBack_wxTopLevelWindow(a0, a1, a2, a3, a4, a5); } ), "doc");
		cl.def( pybind11::init<class wxWindow *, int, const class wxString &, const class wxPoint &, const class wxSize &, long, const class wxString &>(), pybind11::arg("parent"), pybind11::arg("winid"), pybind11::arg("title"), pybind11::arg("pos"), pybind11::arg("size"), pybind11::arg("style"), pybind11::arg("name") );

		cl.def("GetClassInfo", (class wxClassInfo * (wxTopLevelWindow::*)() const) &wxTopLevelWindow::GetClassInfo, "C++: wxTopLevelWindow::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxTopLevelWindow::wxCreateObject, "C++: wxTopLevelWindow::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxControlContainerBase file: line:36
		pybind11::class_<wxControlContainerBase, std::shared_ptr<wxControlContainerBase>, PyCallBack_wxControlContainerBase> cl(M(""), "wxControlContainerBase", "");
		cl.def( pybind11::init( [](){ return new wxControlContainerBase(); }, [](){ return new PyCallBack_wxControlContainerBase(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxControlContainerBase const &o){ return new PyCallBack_wxControlContainerBase(o); } ) );
		cl.def( pybind11::init( [](wxControlContainerBase const &o){ return new wxControlContainerBase(o); } ) );
		cl.def("SetContainerWindow", (void (wxControlContainerBase::*)(class wxWindow *)) &wxControlContainerBase::SetContainerWindow, "C++: wxControlContainerBase::SetContainerWindow(class wxWindow *) --> void", pybind11::arg("winParent"));
		cl.def("DisableSelfFocus", (void (wxControlContainerBase::*)()) &wxControlContainerBase::DisableSelfFocus, "C++: wxControlContainerBase::DisableSelfFocus() --> void");
		cl.def("EnableSelfFocus", (void (wxControlContainerBase::*)()) &wxControlContainerBase::EnableSelfFocus, "C++: wxControlContainerBase::EnableSelfFocus() --> void");
		cl.def("DoSetFocus", (bool (wxControlContainerBase::*)()) &wxControlContainerBase::DoSetFocus, "C++: wxControlContainerBase::DoSetFocus() --> bool");
		cl.def("AcceptsFocus", (bool (wxControlContainerBase::*)() const) &wxControlContainerBase::AcceptsFocus, "C++: wxControlContainerBase::AcceptsFocus() const --> bool");
		cl.def("AcceptsFocusRecursively", (bool (wxControlContainerBase::*)() const) &wxControlContainerBase::AcceptsFocusRecursively, "C++: wxControlContainerBase::AcceptsFocusRecursively() const --> bool");
		cl.def("AcceptsFocusFromKeyboard", (bool (wxControlContainerBase::*)() const) &wxControlContainerBase::AcceptsFocusFromKeyboard, "C++: wxControlContainerBase::AcceptsFocusFromKeyboard() const --> bool");
		cl.def("UpdateCanFocusChildren", (bool (wxControlContainerBase::*)()) &wxControlContainerBase::UpdateCanFocusChildren, "C++: wxControlContainerBase::UpdateCanFocusChildren() --> bool");
		cl.def("assign", (class wxControlContainerBase & (wxControlContainerBase::*)(const class wxControlContainerBase &)) &wxControlContainerBase::operator=, "C++: wxControlContainerBase::operator=(const class wxControlContainerBase &) --> class wxControlContainerBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
