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

// wxWindow file: line:1896
struct PyCallBack_wxWindow : public wxWindow {
	using wxWindow::wxWindow;

	void Raise() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Raise");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Lower");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Show");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "IsShown");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "IsRetained");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetCanFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Reparent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "WarpPointer");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Refresh");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Update");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "ClearBackground");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetBackgroundColour");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetForegroundColour");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetCursor");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetFont");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetBackgroundStyle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "IsTransparentBackgroundSupported");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetCharHeight");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetCharWidth");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetContentScaleFactor");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetScrollbar");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetScrollPos");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetScrollPos");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetScrollThumb");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetScrollRange");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "ScrollWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "ScrollLines");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "ScrollPages");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "AddChild");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "RemoveChild");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetLayoutDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetLayoutDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "AdjustForLayoutDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoIsExposed");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoIsExposed");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "IsDoubleBuffered");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetLabel");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetLabel");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "OnInternalIdle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GTKProcessEvent");
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
	void GTKHandleRealized() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GTKHandleRealized");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::GTKHandleRealized();
	}
	bool GTKNeedsToFilterSameWindowFocus() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GTKNeedsToFilterSameWindowFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GTKWidgetNeedsMnemonic");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GTKWidgetDoSetMnemonic");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GTKGetWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GTKApplyToolTip");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GTKIsTransparentForMouse");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetTextExtent");
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
	void DoClientToScreen(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoClientToScreen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoClientToScreen(a0, a1);
	}
	void DoScreenToClient(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoScreenToClient");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindow::DoScreenToClient(a0, a1);
	}
	void DoGetPosition(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoSetSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoSetClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetBorderSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoMoveWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoEnable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoCaptureMouse");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoReleaseMouse");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoFreeze");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoThaw");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoNavigateIn");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoApplyWidgetStyle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetClassInfo");
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
	bool Destroy() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Destroy");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::Destroy();
	}
	void SetName(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetName");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetName");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetClientAreaOrigin");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "ClientToWindowSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "WindowToClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetEffectiveMinSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Fit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "FitInside");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetSizeHints");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetVirtualSizeHints");
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
	void SetMinSize(const class wxSize & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetMinSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetMinSize(a0);
	}
	void SetMaxSize(const class wxSize & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetMaxSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::SetMaxSize(a0);
	}
	void SetMinClientSize(const class wxSize & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetMinClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetMaxClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetMinSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetMaxSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetMinClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetMaxClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoSetVirtualSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetVirtualSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetBestVirtualSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetWindowBorderSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "InformFirstDirection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SendSizeEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "BeginRepositioningChildren");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "EndRepositioningChildren");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "ShowWithEffect");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "HideWithEffect");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Enable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "IsShownOnScreen");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetWindowStyleFlag");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetWindowStyleFlag");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetExtraStyle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "MakeModal");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetThemeEnabled");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetThemeEnabled");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetFocusFromKbd");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "HasFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "AcceptsFocus");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "AcceptsFocusRecursively");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "AcceptsFocusFromKeyboard");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "CanBeFocused");
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
	bool IsTopLevel() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "IsTopLevel");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::IsTopLevel();
	}
	bool IsClientAreaChild(const class wxWindow * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "IsClientAreaChild");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetNextHandler");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetPreviousHandler");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Validate");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "TransferDataToWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "TransferDataFromWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "InitDialog");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetAcceleratorTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "HasCapture");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "PrepareDC");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetDefaultAttributes");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "HasTransparentBackground");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "UpdateWindowUI");
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
	void DoUpdateWindowUI(class wxUpdateUIEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoUpdateWindowUI");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::DoUpdateWindowUI(a0);
	}
	bool HasMultiplePages() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "HasMultiplePages");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "CanScroll");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "AlwaysShowScrollbars");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "IsScrollbarAlwaysShown");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetHelpTextAtPoint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DragAcceptFiles");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetConstraintSizes");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "LayoutPhase1");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "LayoutPhase2");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoPhase");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetSizeConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "MoveConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetSizeConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetClientSizeConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetPositionConstraint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "Layout");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetTransparent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "CanSetTransparent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SendIdleEvents");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "AssociateHandle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DissociateHandle");
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
	void InheritAttributes() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "InheritAttributes");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::InheritAttributes();
	}
	bool ShouldInheritColours() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "ShouldInheritColours");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "CanBeOutsideClientArea");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "CanApplyThemeBorder");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetMainWindowOfCompositeControl");
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
	bool IsTopNavigationDomain() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "IsTopNavigationDomain");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWindowBase::IsTopNavigationDomain();
	}
	bool TryBefore(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "TryBefore");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "TryAfter");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetDefaultBorder");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetDefaultBorderForControl");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SetInitialBestSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoHitTest");
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
	void DoGetScreenPosition(int * a0, int * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetScreenPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::DoGetScreenPosition(a0, a1);
	}
	class wxSize DoGetBestSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetBestSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetBestClientSize");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetBestClientHeight");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetBestClientWidth");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoSetSizeHints");
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
	void DoCentre(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoCentre");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::DoCentre(a0);
	}
	void AdjustForParentClientOrigin(int & a0, int & a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "AdjustForParentClientOrigin");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowBase::AdjustForParentClientOrigin(a0, a1, a2);
	}
	void DoSetWindowVariant(enum wxWindowVariant a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoSetWindowVariant");
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
	const struct wxEventTable * GetEventTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetEventTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct wxEventTable *>::value) {
				static pybind11::detail::override_caster_t<const struct wxEventTable *> caster;
				return pybind11::detail::cast_ref<const struct wxEventTable *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct wxEventTable *>(std::move(o));
		}
		return wxWindowBase::GetEventTable();
	}
	class wxEventHashTable & GetEventHashTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "GetEventHashTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEventHashTable &>::value) {
				static pybind11::detail::override_caster_t<class wxEventHashTable &> caster;
				return pybind11::detail::cast_ref<class wxEventHashTable &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEventHashTable &>(std::move(o));
		}
		return wxWindowBase::GetEventHashTable();
	}
	bool ProcessEvent(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "ProcessEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "QueueEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "AddPendingEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "SearchEventTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "TryValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "TryParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoSetClientObject");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetClientObject");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoSetClientData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "DoGetClientData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindow *>(this), "CloneRefData");
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

void bind_unknown_unknown_69(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxAssert_wxArrayGdkWindows file: line:152
		pybind11::class_<wxAssert_wxArrayGdkWindows, std::shared_ptr<wxAssert_wxArrayGdkWindows>> cl(M(""), "wxAssert_wxArrayGdkWindows", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxArrayGdkWindows(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayPtrVoid", &wxAssert_wxArrayGdkWindows::TypeTooBigToBeStoredInwxBaseArrayPtrVoid);
	}
	{ // wxArrayGdkWindows file: line:25
		pybind11::class_<wxArrayGdkWindows, std::shared_ptr<wxArrayGdkWindows>, wxBaseArrayPtrVoid> cl(M(""), "wxArrayGdkWindows", "");
		cl.def( pybind11::init( [](){ return new wxArrayGdkWindows(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, class wxWindow *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init( [](wxArrayGdkWindows const &o){ return new wxArrayGdkWindows(o); } ) );
		cl.def("__getitem__", (class wxWindow *& (wxArrayGdkWindows::*)(unsigned long) const) &wxArrayGdkWindows::operator[], "C++: wxArrayGdkWindows::operator[](unsigned long) const --> class wxWindow *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (class wxWindow *& (wxArrayGdkWindows::*)(unsigned long) const) &wxArrayGdkWindows::Item, "C++: wxArrayGdkWindows::Item(unsigned long) const --> class wxWindow *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (class wxWindow *& (wxArrayGdkWindows::*)() const) &wxArrayGdkWindows::Last, "C++: wxArrayGdkWindows::Last() const --> class wxWindow *&", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxArrayGdkWindows const &o, class wxWindow * a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxArrayGdkWindows::*)(class wxWindow *, bool) const) &wxArrayGdkWindows::Index, "C++: wxArrayGdkWindows::Index(class wxWindow *, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxArrayGdkWindows &o, class wxWindow * a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxArrayGdkWindows::*)(class wxWindow *, unsigned long)) &wxArrayGdkWindows::Add, "C++: wxArrayGdkWindows::Add(class wxWindow *, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxArrayGdkWindows &o, class wxWindow * a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxArrayGdkWindows::*)(class wxWindow *, unsigned long, unsigned long)) &wxArrayGdkWindows::Insert, "C++: wxArrayGdkWindows::Insert(class wxWindow *, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxArrayGdkWindows &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxArrayGdkWindows::*)(unsigned long, unsigned long)) &wxArrayGdkWindows::RemoveAt, "C++: wxArrayGdkWindows::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxArrayGdkWindows::*)(class wxWindow *)) &wxArrayGdkWindows::Remove, "C++: wxArrayGdkWindows::Remove(class wxWindow *) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxArrayGdkWindows::*)(unsigned long, class wxWindow *const &)) &wxArrayGdkWindows::assign, "C++: wxArrayGdkWindows::assign(unsigned long, class wxWindow *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (class wxWindow *& (wxArrayGdkWindows::*)()) &wxArrayGdkWindows::back, "C++: wxArrayGdkWindows::back() --> class wxWindow *&", pybind11::return_value_policy::automatic);
		cl.def("begin", (class wxWindow ** (wxArrayGdkWindows::*)()) &wxArrayGdkWindows::begin, "C++: wxArrayGdkWindows::begin() --> class wxWindow **", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxArrayGdkWindows::*)() const) &wxArrayGdkWindows::capacity, "C++: wxArrayGdkWindows::capacity() const --> unsigned long");
		cl.def("end", (class wxWindow ** (wxArrayGdkWindows::*)()) &wxArrayGdkWindows::end, "C++: wxArrayGdkWindows::end() --> class wxWindow **", pybind11::return_value_policy::automatic);
		cl.def("front", (class wxWindow *& (wxArrayGdkWindows::*)()) &wxArrayGdkWindows::front, "C++: wxArrayGdkWindows::front() --> class wxWindow *&", pybind11::return_value_policy::automatic);
		cl.def("pop_back", (void (wxArrayGdkWindows::*)()) &wxArrayGdkWindows::pop_back, "C++: wxArrayGdkWindows::pop_back() --> void");
		cl.def("push_back", (void (wxArrayGdkWindows::*)(class wxWindow *const &)) &wxArrayGdkWindows::push_back, "C++: wxArrayGdkWindows::push_back(class wxWindow *const &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxArrayGdkWindows::reverse_iterator (wxArrayGdkWindows::*)()) &wxArrayGdkWindows::rbegin, "C++: wxArrayGdkWindows::rbegin() --> class wxArrayGdkWindows::reverse_iterator");
		cl.def("rend", (class wxArrayGdkWindows::reverse_iterator (wxArrayGdkWindows::*)()) &wxArrayGdkWindows::rend, "C++: wxArrayGdkWindows::rend() --> class wxArrayGdkWindows::reverse_iterator");
		cl.def("reserve", (void (wxArrayGdkWindows::*)(unsigned long)) &wxArrayGdkWindows::reserve, "C++: wxArrayGdkWindows::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxArrayGdkWindows &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxArrayGdkWindows::*)(unsigned long, class wxWindow *)) &wxArrayGdkWindows::resize, "C++: wxArrayGdkWindows::resize(unsigned long, class wxWindow *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxArrayGdkWindows::*)(class wxArrayGdkWindows &)) &wxArrayGdkWindows::swap, "C++: wxArrayGdkWindows::swap(class wxArrayGdkWindows &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxArrayGdkWindows & (wxArrayGdkWindows::*)(const class wxArrayGdkWindows &)) &wxArrayGdkWindows::operator=, "C++: wxArrayGdkWindows::operator=(const class wxArrayGdkWindows &) --> class wxArrayGdkWindows &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxArrayGdkWindows::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayGdkWindows::reverse_iterator, std::shared_ptr<wxArrayGdkWindows::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayGdkWindows::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxArrayGdkWindows::reverse_iterator const &o){ return new wxArrayGdkWindows::reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxWindow *& (wxArrayGdkWindows::reverse_iterator::*)() const) &wxArrayGdkWindows::reverse_iterator::operator*, "C++: wxArrayGdkWindows::reverse_iterator::operator*() const --> class wxWindow *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayGdkWindows::reverse_iterator & (wxArrayGdkWindows::reverse_iterator::*)()) &wxArrayGdkWindows::reverse_iterator::operator++, "C++: wxArrayGdkWindows::reverse_iterator::operator++() --> class wxArrayGdkWindows::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayGdkWindows::reverse_iterator (wxArrayGdkWindows::reverse_iterator::*)(int)) &wxArrayGdkWindows::reverse_iterator::operator++, "C++: wxArrayGdkWindows::reverse_iterator::operator++(int) --> const class wxArrayGdkWindows::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayGdkWindows::reverse_iterator & (wxArrayGdkWindows::reverse_iterator::*)()) &wxArrayGdkWindows::reverse_iterator::operator--, "C++: wxArrayGdkWindows::reverse_iterator::operator--() --> class wxArrayGdkWindows::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayGdkWindows::reverse_iterator (wxArrayGdkWindows::reverse_iterator::*)(int)) &wxArrayGdkWindows::reverse_iterator::operator--, "C++: wxArrayGdkWindows::reverse_iterator::operator--(int) --> const class wxArrayGdkWindows::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayGdkWindows::reverse_iterator::*)(const class wxArrayGdkWindows::reverse_iterator &) const) &wxArrayGdkWindows::reverse_iterator::operator==, "C++: wxArrayGdkWindows::reverse_iterator::operator==(const class wxArrayGdkWindows::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayGdkWindows::reverse_iterator::*)(const class wxArrayGdkWindows::reverse_iterator &) const) &wxArrayGdkWindows::reverse_iterator::operator!=, "C++: wxArrayGdkWindows::reverse_iterator::operator!=(const class wxArrayGdkWindows::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxArrayGdkWindows::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayGdkWindows::const_reverse_iterator, std::shared_ptr<wxArrayGdkWindows::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayGdkWindows::const_reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxArrayGdkWindows::const_reverse_iterator const &o){ return new wxArrayGdkWindows::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxArrayGdkWindows::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (class wxWindow *const & (wxArrayGdkWindows::const_reverse_iterator::*)() const) &wxArrayGdkWindows::const_reverse_iterator::operator*, "C++: wxArrayGdkWindows::const_reverse_iterator::operator*() const --> class wxWindow *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayGdkWindows::const_reverse_iterator & (wxArrayGdkWindows::const_reverse_iterator::*)()) &wxArrayGdkWindows::const_reverse_iterator::operator++, "C++: wxArrayGdkWindows::const_reverse_iterator::operator++() --> class wxArrayGdkWindows::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayGdkWindows::const_reverse_iterator (wxArrayGdkWindows::const_reverse_iterator::*)(int)) &wxArrayGdkWindows::const_reverse_iterator::operator++, "C++: wxArrayGdkWindows::const_reverse_iterator::operator++(int) --> const class wxArrayGdkWindows::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayGdkWindows::const_reverse_iterator & (wxArrayGdkWindows::const_reverse_iterator::*)()) &wxArrayGdkWindows::const_reverse_iterator::operator--, "C++: wxArrayGdkWindows::const_reverse_iterator::operator--() --> class wxArrayGdkWindows::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayGdkWindows::const_reverse_iterator (wxArrayGdkWindows::const_reverse_iterator::*)(int)) &wxArrayGdkWindows::const_reverse_iterator::operator--, "C++: wxArrayGdkWindows::const_reverse_iterator::operator--(int) --> const class wxArrayGdkWindows::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayGdkWindows::const_reverse_iterator::*)(const class wxArrayGdkWindows::const_reverse_iterator &) const) &wxArrayGdkWindows::const_reverse_iterator::operator==, "C++: wxArrayGdkWindows::const_reverse_iterator::operator==(const class wxArrayGdkWindows::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayGdkWindows::const_reverse_iterator::*)(const class wxArrayGdkWindows::const_reverse_iterator &) const) &wxArrayGdkWindows::const_reverse_iterator::operator!=, "C++: wxArrayGdkWindows::const_reverse_iterator::operator!=(const class wxArrayGdkWindows::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
	{ // wxWindow file: line:1896
		pybind11::class_<wxWindow, std::shared_ptr<wxWindow>, PyCallBack_wxWindow, wxWindowBase> cl(M(""), "wxWindow", "");
		cl.def( pybind11::init( [](){ return new wxWindow(); }, [](){ return new PyCallBack_wxWindow(); } ) );
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1){ return new wxWindow(a0, a1); }, [](class wxWindow * a0, int const & a1){ return new PyCallBack_wxWindow(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxPoint & a2){ return new wxWindow(a0, a1, a2); }, [](class wxWindow * a0, int const & a1, const class wxPoint & a2){ return new PyCallBack_wxWindow(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxPoint & a2, const class wxSize & a3){ return new wxWindow(a0, a1, a2, a3); }, [](class wxWindow * a0, int const & a1, const class wxPoint & a2, const class wxSize & a3){ return new PyCallBack_wxWindow(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxPoint & a2, const class wxSize & a3, long const & a4){ return new wxWindow(a0, a1, a2, a3, a4); }, [](class wxWindow * a0, int const & a1, const class wxPoint & a2, const class wxSize & a3, long const & a4){ return new PyCallBack_wxWindow(a0, a1, a2, a3, a4); } ), "doc");
		cl.def( pybind11::init<class wxWindow *, int, const class wxPoint &, const class wxSize &, long, const class wxString &>(), pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("pos"), pybind11::arg("size"), pybind11::arg("style"), pybind11::arg("name") );


		pybind11::enum_<wxWindow::ScrollDir>(cl, "ScrollDir", pybind11::arithmetic(), "")
			.value("ScrollDir_Horz", wxWindow::ScrollDir_Horz)
			.value("ScrollDir_Vert", wxWindow::ScrollDir_Vert)
			.value("ScrollDir_Max", wxWindow::ScrollDir_Max)
			.export_values();

		cl.def_readwrite("m_x", &wxWindow::m_x);
		cl.def_readwrite("m_y", &wxWindow::m_y);
		cl.def_readwrite("m_width", &wxWindow::m_width);
		cl.def_readwrite("m_height", &wxWindow::m_height);
		cl.def_readwrite("m_clientWidth", &wxWindow::m_clientWidth);
		cl.def_readwrite("m_clientHeight", &wxWindow::m_clientHeight);
		cl.def_readwrite("m_useCachedClientSize", &wxWindow::m_useCachedClientSize);
		cl.def_readwrite("m_gtkLabel", &wxWindow::m_gtkLabel);
		cl.def_readwrite("m_noExpose", &wxWindow::m_noExpose);
		cl.def_readwrite("m_nativeSizeEvent", &wxWindow::m_nativeSizeEvent);
		cl.def_readwrite("m_isScrolling", &wxWindow::m_isScrolling);
		cl.def_readwrite("m_clipPaintRegion", &wxWindow::m_clipPaintRegion);
		cl.def_readwrite("m_nativeUpdateRegion", &wxWindow::m_nativeUpdateRegion);
		cl.def_readwrite("m_dirtyTabOrder", &wxWindow::m_dirtyTabOrder);
		cl.def_readwrite("m_mouseButtonDown", &wxWindow::m_mouseButtonDown);
		cl.def_readwrite("m_showOnIdle", &wxWindow::m_showOnIdle);
		cl.def("Create", [](wxWindow &o, class wxWindow * a0, int const & a1) -> bool { return o.Create(a0, a1); }, "", pybind11::arg("parent"), pybind11::arg("id"));
		cl.def("Create", [](wxWindow &o, class wxWindow * a0, int const & a1, const class wxPoint & a2) -> bool { return o.Create(a0, a1, a2); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("pos"));
		cl.def("Create", [](wxWindow &o, class wxWindow * a0, int const & a1, const class wxPoint & a2, const class wxSize & a3) -> bool { return o.Create(a0, a1, a2, a3); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("pos"), pybind11::arg("size"));
		cl.def("Create", [](wxWindow &o, class wxWindow * a0, int const & a1, const class wxPoint & a2, const class wxSize & a3, long const & a4) -> bool { return o.Create(a0, a1, a2, a3, a4); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("pos"), pybind11::arg("size"), pybind11::arg("style"));
		cl.def("Create", (bool (wxWindow::*)(class wxWindow *, int, const class wxPoint &, const class wxSize &, long, const class wxString &)) &wxWindow::Create, "C++: wxWindow::Create(class wxWindow *, int, const class wxPoint &, const class wxSize &, long, const class wxString &) --> bool", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("pos"), pybind11::arg("size"), pybind11::arg("style"), pybind11::arg("name"));
		cl.def("Raise", (void (wxWindow::*)()) &wxWindow::Raise, "C++: wxWindow::Raise() --> void");
		cl.def("Lower", (void (wxWindow::*)()) &wxWindow::Lower, "C++: wxWindow::Lower() --> void");
		cl.def("Show", [](wxWindow &o) -> bool { return o.Show(); }, "");
		cl.def("Show", (bool (wxWindow::*)(bool)) &wxWindow::Show, "C++: wxWindow::Show(bool) --> bool", pybind11::arg("show"));
		cl.def("IsShown", (bool (wxWindow::*)() const) &wxWindow::IsShown, "C++: wxWindow::IsShown() const --> bool");
		cl.def("IsRetained", (bool (wxWindow::*)() const) &wxWindow::IsRetained, "C++: wxWindow::IsRetained() const --> bool");
		cl.def("SetFocus", (void (wxWindow::*)()) &wxWindow::SetFocus, "C++: wxWindow::SetFocus() --> void");
		cl.def("SetCanFocus", (void (wxWindow::*)(bool)) &wxWindow::SetCanFocus, "C++: wxWindow::SetCanFocus(bool) --> void", pybind11::arg("canFocus"));
		cl.def("Reparent", (bool (wxWindow::*)(class wxWindowBase *)) &wxWindow::Reparent, "C++: wxWindow::Reparent(class wxWindowBase *) --> bool", pybind11::arg("newParent"));
		cl.def("WarpPointer", (void (wxWindow::*)(int, int)) &wxWindow::WarpPointer, "C++: wxWindow::WarpPointer(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("Refresh", [](wxWindow &o) -> void { return o.Refresh(); }, "");
		cl.def("Refresh", [](wxWindow &o, bool const & a0) -> void { return o.Refresh(a0); }, "", pybind11::arg("eraseBackground"));
		cl.def("Refresh", (void (wxWindow::*)(bool, const class wxRect *)) &wxWindow::Refresh, "C++: wxWindow::Refresh(bool, const class wxRect *) --> void", pybind11::arg("eraseBackground"), pybind11::arg("rect"));
		cl.def("Update", (void (wxWindow::*)()) &wxWindow::Update, "C++: wxWindow::Update() --> void");
		cl.def("ClearBackground", (void (wxWindow::*)()) &wxWindow::ClearBackground, "C++: wxWindow::ClearBackground() --> void");
		cl.def("SetBackgroundColour", (bool (wxWindow::*)(const class wxColour &)) &wxWindow::SetBackgroundColour, "C++: wxWindow::SetBackgroundColour(const class wxColour &) --> bool", pybind11::arg("colour"));
		cl.def("SetForegroundColour", (bool (wxWindow::*)(const class wxColour &)) &wxWindow::SetForegroundColour, "C++: wxWindow::SetForegroundColour(const class wxColour &) --> bool", pybind11::arg("colour"));
		cl.def("SetCursor", (bool (wxWindow::*)(const class wxCursor &)) &wxWindow::SetCursor, "C++: wxWindow::SetCursor(const class wxCursor &) --> bool", pybind11::arg("cursor"));
		cl.def("SetFont", (bool (wxWindow::*)(const class wxFont &)) &wxWindow::SetFont, "C++: wxWindow::SetFont(const class wxFont &) --> bool", pybind11::arg("font"));
		cl.def("SetBackgroundStyle", (bool (wxWindow::*)(enum wxBackgroundStyle)) &wxWindow::SetBackgroundStyle, "C++: wxWindow::SetBackgroundStyle(enum wxBackgroundStyle) --> bool", pybind11::arg("style"));
		cl.def("IsTransparentBackgroundSupported", [](wxWindow const &o) -> bool { return o.IsTransparentBackgroundSupported(); }, "");
		cl.def("IsTransparentBackgroundSupported", (bool (wxWindow::*)(class wxString *) const) &wxWindow::IsTransparentBackgroundSupported, "C++: wxWindow::IsTransparentBackgroundSupported(class wxString *) const --> bool", pybind11::arg("reason"));
		cl.def("GetCharHeight", (int (wxWindow::*)() const) &wxWindow::GetCharHeight, "C++: wxWindow::GetCharHeight() const --> int");
		cl.def("GetCharWidth", (int (wxWindow::*)() const) &wxWindow::GetCharWidth, "C++: wxWindow::GetCharWidth() const --> int");
		cl.def("GetContentScaleFactor", (double (wxWindow::*)() const) &wxWindow::GetContentScaleFactor, "C++: wxWindow::GetContentScaleFactor() const --> double");
		cl.def("SetScrollbar", [](wxWindow &o, int const & a0, int const & a1, int const & a2, int const & a3) -> void { return o.SetScrollbar(a0, a1, a2, a3); }, "", pybind11::arg("orient"), pybind11::arg("pos"), pybind11::arg("thumbVisible"), pybind11::arg("range"));
		cl.def("SetScrollbar", (void (wxWindow::*)(int, int, int, int, bool)) &wxWindow::SetScrollbar, "C++: wxWindow::SetScrollbar(int, int, int, int, bool) --> void", pybind11::arg("orient"), pybind11::arg("pos"), pybind11::arg("thumbVisible"), pybind11::arg("range"), pybind11::arg("refresh"));
		cl.def("SetScrollPos", [](wxWindow &o, int const & a0, int const & a1) -> void { return o.SetScrollPos(a0, a1); }, "", pybind11::arg("orient"), pybind11::arg("pos"));
		cl.def("SetScrollPos", (void (wxWindow::*)(int, int, bool)) &wxWindow::SetScrollPos, "C++: wxWindow::SetScrollPos(int, int, bool) --> void", pybind11::arg("orient"), pybind11::arg("pos"), pybind11::arg("refresh"));
		cl.def("GetScrollPos", (int (wxWindow::*)(int) const) &wxWindow::GetScrollPos, "C++: wxWindow::GetScrollPos(int) const --> int", pybind11::arg("orient"));
		cl.def("GetScrollThumb", (int (wxWindow::*)(int) const) &wxWindow::GetScrollThumb, "C++: wxWindow::GetScrollThumb(int) const --> int", pybind11::arg("orient"));
		cl.def("GetScrollRange", (int (wxWindow::*)(int) const) &wxWindow::GetScrollRange, "C++: wxWindow::GetScrollRange(int) const --> int", pybind11::arg("orient"));
		cl.def("ScrollWindow", [](wxWindow &o, int const & a0, int const & a1) -> void { return o.ScrollWindow(a0, a1); }, "", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("ScrollWindow", (void (wxWindow::*)(int, int, const class wxRect *)) &wxWindow::ScrollWindow, "C++: wxWindow::ScrollWindow(int, int, const class wxRect *) --> void", pybind11::arg("dx"), pybind11::arg("dy"), pybind11::arg("rect"));
		cl.def("ScrollLines", (bool (wxWindow::*)(int)) &wxWindow::ScrollLines, "C++: wxWindow::ScrollLines(int) --> bool", pybind11::arg("lines"));
		cl.def("ScrollPages", (bool (wxWindow::*)(int)) &wxWindow::ScrollPages, "C++: wxWindow::ScrollPages(int) --> bool", pybind11::arg("pages"));
		cl.def("AddChild", (void (wxWindow::*)(class wxWindowBase *)) &wxWindow::AddChild, "C++: wxWindow::AddChild(class wxWindowBase *) --> void", pybind11::arg("child"));
		cl.def("RemoveChild", (void (wxWindow::*)(class wxWindowBase *)) &wxWindow::RemoveChild, "C++: wxWindow::RemoveChild(class wxWindowBase *) --> void", pybind11::arg("child"));
		cl.def("SetLayoutDirection", (void (wxWindow::*)(enum wxLayoutDirection)) &wxWindow::SetLayoutDirection, "C++: wxWindow::SetLayoutDirection(enum wxLayoutDirection) --> void", pybind11::arg("dir"));
		cl.def("GetLayoutDirection", (enum wxLayoutDirection (wxWindow::*)() const) &wxWindow::GetLayoutDirection, "C++: wxWindow::GetLayoutDirection() const --> enum wxLayoutDirection");
		cl.def("AdjustForLayoutDirection", (int (wxWindow::*)(int, int, int) const) &wxWindow::AdjustForLayoutDirection, "C++: wxWindow::AdjustForLayoutDirection(int, int, int) const --> int", pybind11::arg("x"), pybind11::arg("width"), pybind11::arg("widthTotal"));
		cl.def("DoIsExposed", (bool (wxWindow::*)(int, int) const) &wxWindow::DoIsExposed, "C++: wxWindow::DoIsExposed(int, int) const --> bool", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DoIsExposed", (bool (wxWindow::*)(int, int, int, int) const) &wxWindow::DoIsExposed, "C++: wxWindow::DoIsExposed(int, int, int, int) const --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("SetDoubleBuffered", (void (wxWindow::*)(bool)) &wxWindow::SetDoubleBuffered, "C++: wxWindow::SetDoubleBuffered(bool) --> void", pybind11::arg("on"));
		cl.def("IsDoubleBuffered", (bool (wxWindow::*)() const) &wxWindow::IsDoubleBuffered, "C++: wxWindow::IsDoubleBuffered() const --> bool");
		cl.def("SetLabel", (void (wxWindow::*)(const class wxString &)) &wxWindow::SetLabel, "C++: wxWindow::SetLabel(const class wxString &) --> void", pybind11::arg("label"));
		cl.def("GetLabel", (class wxString (wxWindow::*)() const) &wxWindow::GetLabel, "C++: wxWindow::GetLabel() const --> class wxString");
		cl.def("GetHandle", (int (wxWindow::*)() const) &wxWindow::GetHandle, "C++: wxWindow::GetHandle() const --> int");
		cl.def("OnInternalIdle", (void (wxWindow::*)()) &wxWindow::OnInternalIdle, "C++: wxWindow::OnInternalIdle() --> void");
		cl.def("OnIdle", (void (wxWindow::*)(class wxIdleEvent &)) &wxWindow::OnIdle, "C++: wxWindow::OnIdle(class wxIdleEvent &) --> void", pybind11::arg(""));
		cl.def("PreCreation", (bool (wxWindow::*)(class wxWindow *, const class wxPoint &, const class wxSize &)) &wxWindow::PreCreation, "C++: wxWindow::PreCreation(class wxWindow *, const class wxPoint &, const class wxSize &) --> bool", pybind11::arg("parent"), pybind11::arg("pos"), pybind11::arg("size"));
		cl.def("PostCreation", (void (wxWindow::*)()) &wxWindow::PostCreation, "C++: wxWindow::PostCreation() --> void");
		cl.def("DoAddChild", (void (wxWindow::*)(class wxWindow *)) &wxWindow::DoAddChild, "C++: wxWindow::DoAddChild(class wxWindow *) --> void", pybind11::arg("child"));
		cl.def("GetConnectWidget", (int * (wxWindow::*)()) &wxWindow::GetConnectWidget, "C++: wxWindow::GetConnectWidget() --> int *", pybind11::return_value_policy::automatic);
		cl.def("ConnectWidget", (void (wxWindow::*)(int *)) &wxWindow::ConnectWidget, "C++: wxWindow::ConnectWidget(int *) --> void", pybind11::arg("widget"));
		cl.def("GTKShouldIgnoreEvent", (bool (wxWindow::*)() const) &wxWindow::GTKShouldIgnoreEvent, "C++: wxWindow::GTKShouldIgnoreEvent() const --> bool");
		cl.def("GTKProcessEvent", (bool (wxWindow::*)(class wxEvent &) const) &wxWindow::GTKProcessEvent, "C++: wxWindow::GTKProcessEvent(class wxEvent &) const --> bool", pybind11::arg("event"));
		cl.def_static("GTKGetLayout", (enum wxLayoutDirection (*)(int *)) &wxWindow::GTKGetLayout, "C++: wxWindow::GTKGetLayout(int *) --> enum wxLayoutDirection", pybind11::arg("widget"));
		cl.def_static("GTKSetLayout", (void (*)(int *, enum wxLayoutDirection)) &wxWindow::GTKSetLayout, "C++: wxWindow::GTKSetLayout(int *, enum wxLayoutDirection) --> void", pybind11::arg("widget"), pybind11::arg("dir"));
		cl.def("GTKReleaseMouseAndNotify", (void (wxWindow::*)()) &wxWindow::GTKReleaseMouseAndNotify, "C++: wxWindow::GTKReleaseMouseAndNotify() --> void");
		cl.def_static("GTKHandleCaptureLost", (void (*)()) &wxWindow::GTKHandleCaptureLost, "C++: wxWindow::GTKHandleCaptureLost() --> void");
		cl.def("GTKGetDrawingWindow", (class wxWindow * (wxWindow::*)() const) &wxWindow::GTKGetDrawingWindow, "C++: wxWindow::GTKGetDrawingWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("GTKHandleFocusIn", (bool (wxWindow::*)()) &wxWindow::GTKHandleFocusIn, "C++: wxWindow::GTKHandleFocusIn() --> bool");
		cl.def("GTKHandleFocusOut", (bool (wxWindow::*)()) &wxWindow::GTKHandleFocusOut, "C++: wxWindow::GTKHandleFocusOut() --> bool");
		cl.def("GTKHandleFocusOutNoDeferring", (void (wxWindow::*)()) &wxWindow::GTKHandleFocusOutNoDeferring, "C++: wxWindow::GTKHandleFocusOutNoDeferring() --> void");
		cl.def_static("GTKHandleDeferredFocusOut", (void (*)()) &wxWindow::GTKHandleDeferredFocusOut, "C++: wxWindow::GTKHandleDeferredFocusOut() --> void");
		cl.def("GTKHandleRealized", (void (wxWindow::*)()) &wxWindow::GTKHandleRealized, "C++: wxWindow::GTKHandleRealized() --> void");
		cl.def("GTKHandleUnrealize", (void (wxWindow::*)()) &wxWindow::GTKHandleUnrealize, "C++: wxWindow::GTKHandleUnrealize() --> void");
		cl.def("GTKApplyToolTip", (void (wxWindow::*)(const char *)) &wxWindow::GTKApplyToolTip, "C++: wxWindow::GTKApplyToolTip(const char *) --> void", pybind11::arg("tip"));
		cl.def("GTKShowOnIdle", (void (wxWindow::*)()) &wxWindow::GTKShowOnIdle, "C++: wxWindow::GTKShowOnIdle() --> void");
		cl.def("GTKShowFromOnIdle", (bool (wxWindow::*)()) &wxWindow::GTKShowFromOnIdle, "C++: wxWindow::GTKShowFromOnIdle() --> bool");
		cl.def("GTKIsTransparentForMouse", (bool (wxWindow::*)() const) &wxWindow::GTKIsTransparentForMouse, "C++: wxWindow::GTKIsTransparentForMouse() const --> bool");
		cl.def("GTKGetScrollEventType", (int (wxWindow::*)(int *)) &wxWindow::GTKGetScrollEventType, "C++: wxWindow::GTKGetScrollEventType(int *) --> int", pybind11::arg("range"));
		cl.def("IsOfStandardClass", (bool (wxWindow::*)() const) &wxWindow::IsOfStandardClass, "C++: wxWindow::IsOfStandardClass() const --> bool");
		cl.def("GTKDisableFocusOutEvent", (void (wxWindow::*)()) &wxWindow::GTKDisableFocusOutEvent, "C++: wxWindow::GTKDisableFocusOutEvent() --> void");
		cl.def("GTKEnableFocusOutEvent", (void (wxWindow::*)()) &wxWindow::GTKEnableFocusOutEvent, "C++: wxWindow::GTKEnableFocusOutEvent() --> void");
		cl.def("GTKOnInsertText", (bool (wxWindow::*)(const char *)) &wxWindow::GTKOnInsertText, "C++: wxWindow::GTKOnInsertText(const char *) --> bool", pybind11::arg("text"));
		cl.def("GTKDoInsertTextFromIM", (bool (wxWindow::*)(const char *)) &wxWindow::GTKDoInsertTextFromIM, "C++: wxWindow::GTKDoInsertTextFromIM(const char *) --> bool", pybind11::arg("text"));
		cl.def_static("ScrollDirFromOrient", (enum wxWindow::ScrollDir (*)(int)) &wxWindow::ScrollDirFromOrient, "C++: wxWindow::ScrollDirFromOrient(int) --> enum wxWindow::ScrollDir", pybind11::arg("orient"));
		cl.def_static("OrientFromScrollDir", (int (*)(enum wxWindow::ScrollDir)) &wxWindow::OrientFromScrollDir, "C++: wxWindow::OrientFromScrollDir(enum wxWindow::ScrollDir) --> int", pybind11::arg("dir"));
		cl.def("ScrollDirFromRange", (enum wxWindow::ScrollDir (wxWindow::*)(int *) const) &wxWindow::ScrollDirFromRange, "C++: wxWindow::ScrollDirFromRange(int *) const --> enum wxWindow::ScrollDir", pybind11::arg("range"));
		cl.def("GTKUpdateCursor", [](wxWindow &o) -> void { return o.GTKUpdateCursor(); }, "");
		cl.def("GTKUpdateCursor", [](wxWindow &o, bool const & a0) -> void { return o.GTKUpdateCursor(a0); }, "", pybind11::arg("isBusyOrGlobalCursor"));
		cl.def("GTKUpdateCursor", (void (wxWindow::*)(bool, bool)) &wxWindow::GTKUpdateCursor, "C++: wxWindow::GTKUpdateCursor(bool, bool) --> void", pybind11::arg("isBusyOrGlobalCursor"), pybind11::arg("isRealize"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxWindow::*)() const) &wxWindow::GetClassInfo, "C++: wxWindow::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxWindow::wxCreateObject, "C++: wxWindow::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
}
