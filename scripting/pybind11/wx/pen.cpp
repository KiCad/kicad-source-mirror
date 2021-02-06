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
#include <wx/pen.h> // wxPenBase
#include <wx/pen.h> // wxPenCap
#include <wx/pen.h> // wxPenJoin
#include <wx/pen.h> // wxPenList
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

// wxPen file: line:18
struct PyCallBack_wxPen : public wxPen {
	using wxPen::wxPen;

	void SetColour(const class wxColour & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "SetColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPen::SetColour(a0);
	}
	void SetColour(unsigned char a0, unsigned char a1, unsigned char a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "SetColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPen::SetColour(a0, a1, a2);
	}
	void SetCap(enum wxPenCap a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "SetCap");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPen::SetCap(a0);
	}
	void SetJoin(enum wxPenJoin a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "SetJoin");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPen::SetJoin(a0);
	}
	void SetStyle(enum wxPenStyle a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "SetStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPen::SetStyle(a0);
	}
	void SetWidth(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "SetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPen::SetWidth(a0);
	}
	void SetDashes(int a0, const signed char * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "SetDashes");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPen::SetDashes(a0, a1);
	}
	void SetStipple(const class wxBitmap & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "SetStipple");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPen::SetStipple(a0);
	}
	class wxColour GetColour() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "GetColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxColour>::value) {
				static pybind11::detail::override_caster_t<class wxColour> caster;
				return pybind11::detail::cast_ref<class wxColour>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxColour>(std::move(o));
		}
		return wxPen::GetColour();
	}
	enum wxPenCap GetCap() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "GetCap");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxPenCap>::value) {
				static pybind11::detail::override_caster_t<enum wxPenCap> caster;
				return pybind11::detail::cast_ref<enum wxPenCap>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxPenCap>(std::move(o));
		}
		return wxPen::GetCap();
	}
	enum wxPenJoin GetJoin() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "GetJoin");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxPenJoin>::value) {
				static pybind11::detail::override_caster_t<enum wxPenJoin> caster;
				return pybind11::detail::cast_ref<enum wxPenJoin>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxPenJoin>(std::move(o));
		}
		return wxPen::GetJoin();
	}
	enum wxPenStyle GetStyle() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "GetStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxPenStyle>::value) {
				static pybind11::detail::override_caster_t<enum wxPenStyle> caster;
				return pybind11::detail::cast_ref<enum wxPenStyle>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxPenStyle>(std::move(o));
		}
		return wxPen::GetStyle();
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxPen::GetWidth();
	}
	class wxBitmap * GetStipple() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "GetStipple");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxBitmap *>::value) {
				static pybind11::detail::override_caster_t<class wxBitmap *> caster;
				return pybind11::detail::cast_ref<class wxBitmap *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxBitmap *>(std::move(o));
		}
		return wxPen::GetStipple();
	}
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxPen::CreateGDIRefData();
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxPen::CloneGDIRefData(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxPen::GetClassInfo();
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "IsOk");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPen *>(this), "CloneRefData");
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

// wxBrushBase file: line:41
struct PyCallBack_wxBrushBase : public wxBrushBase {
	using wxBrushBase::wxBrushBase;

	void SetColour(const class wxColour & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "SetColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxBrushBase::SetColour\"");
	}
	void SetColour(unsigned char a0, unsigned char a1, unsigned char a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "SetColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxBrushBase::SetColour\"");
	}
	void SetStyle(enum wxBrushStyle a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "SetStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxBrushBase::SetStyle\"");
	}
	void SetStipple(const class wxBitmap & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "SetStipple");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxBrushBase::SetStipple\"");
	}
	class wxColour GetColour() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "GetColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxColour>::value) {
				static pybind11::detail::override_caster_t<class wxColour> caster;
				return pybind11::detail::cast_ref<class wxColour>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxColour>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxBrushBase::GetColour\"");
	}
	enum wxBrushStyle GetStyle() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "GetStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxBrushStyle>::value) {
				static pybind11::detail::override_caster_t<enum wxBrushStyle> caster;
				return pybind11::detail::cast_ref<enum wxBrushStyle>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxBrushStyle>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxBrushBase::GetStyle\"");
	}
	class wxBitmap * GetStipple() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "GetStipple");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxBitmap *>::value) {
				static pybind11::detail::override_caster_t<class wxBitmap *> caster;
				return pybind11::detail::cast_ref<class wxBitmap *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxBitmap *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxBrushBase::GetStipple\"");
	}
	bool IsHatch() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "IsHatch");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxBrushBase::IsHatch();
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "IsOk");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "CloneRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "CreateGDIRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "CloneGDIRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrushBase *>(this), "GetClassInfo");
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

// wxBrush file: line:19
struct PyCallBack_wxBrush : public wxBrush {
	using wxBrush::wxBrush;

	enum wxBrushStyle GetStyle() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "GetStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxBrushStyle>::value) {
				static pybind11::detail::override_caster_t<enum wxBrushStyle> caster;
				return pybind11::detail::cast_ref<enum wxBrushStyle>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxBrushStyle>(std::move(o));
		}
		return wxBrush::GetStyle();
	}
	class wxColour GetColour() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "GetColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxColour>::value) {
				static pybind11::detail::override_caster_t<class wxColour> caster;
				return pybind11::detail::cast_ref<class wxColour>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxColour>(std::move(o));
		}
		return wxBrush::GetColour();
	}
	class wxBitmap * GetStipple() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "GetStipple");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxBitmap *>::value) {
				static pybind11::detail::override_caster_t<class wxBitmap *> caster;
				return pybind11::detail::cast_ref<class wxBitmap *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxBitmap *>(std::move(o));
		}
		return wxBrush::GetStipple();
	}
	void SetColour(const class wxColour & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "SetColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxBrush::SetColour(a0);
	}
	void SetColour(unsigned char a0, unsigned char a1, unsigned char a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "SetColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxBrush::SetColour(a0, a1, a2);
	}
	void SetStyle(enum wxBrushStyle a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "SetStyle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxBrush::SetStyle(a0);
	}
	void SetStipple(const class wxBitmap & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "SetStipple");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxBrush::SetStipple(a0);
	}
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxBrush::CreateGDIRefData();
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxBrush::CloneGDIRefData(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxBrush::GetClassInfo();
	}
	bool IsHatch() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "IsHatch");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxBrushBase::IsHatch();
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "IsOk");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBrush *>(this), "CloneRefData");
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

void bind_wx_pen(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxPenStyle file:wx/pen.h line:17
	pybind11::enum_<wxPenStyle>(M(""), "wxPenStyle", pybind11::arithmetic(), "")
		.value("wxPENSTYLE_INVALID", wxPENSTYLE_INVALID)
		.value("wxPENSTYLE_SOLID", wxPENSTYLE_SOLID)
		.value("wxPENSTYLE_DOT", wxPENSTYLE_DOT)
		.value("wxPENSTYLE_LONG_DASH", wxPENSTYLE_LONG_DASH)
		.value("wxPENSTYLE_SHORT_DASH", wxPENSTYLE_SHORT_DASH)
		.value("wxPENSTYLE_DOT_DASH", wxPENSTYLE_DOT_DASH)
		.value("wxPENSTYLE_USER_DASH", wxPENSTYLE_USER_DASH)
		.value("wxPENSTYLE_TRANSPARENT", wxPENSTYLE_TRANSPARENT)
		.value("wxPENSTYLE_STIPPLE_MASK_OPAQUE", wxPENSTYLE_STIPPLE_MASK_OPAQUE)
		.value("wxPENSTYLE_STIPPLE_MASK", wxPENSTYLE_STIPPLE_MASK)
		.value("wxPENSTYLE_STIPPLE", wxPENSTYLE_STIPPLE)
		.value("wxPENSTYLE_BDIAGONAL_HATCH", wxPENSTYLE_BDIAGONAL_HATCH)
		.value("wxPENSTYLE_CROSSDIAG_HATCH", wxPENSTYLE_CROSSDIAG_HATCH)
		.value("wxPENSTYLE_FDIAGONAL_HATCH", wxPENSTYLE_FDIAGONAL_HATCH)
		.value("wxPENSTYLE_CROSS_HATCH", wxPENSTYLE_CROSS_HATCH)
		.value("wxPENSTYLE_HORIZONTAL_HATCH", wxPENSTYLE_HORIZONTAL_HATCH)
		.value("wxPENSTYLE_VERTICAL_HATCH", wxPENSTYLE_VERTICAL_HATCH)
		.value("wxPENSTYLE_FIRST_HATCH", wxPENSTYLE_FIRST_HATCH)
		.value("wxPENSTYLE_LAST_HATCH", wxPENSTYLE_LAST_HATCH)
		.export_values();

;

	// wxPenJoin file:wx/pen.h line:44
	pybind11::enum_<wxPenJoin>(M(""), "wxPenJoin", pybind11::arithmetic(), "")
		.value("wxJOIN_INVALID", wxJOIN_INVALID)
		.value("wxJOIN_BEVEL", wxJOIN_BEVEL)
		.value("wxJOIN_MITER", wxJOIN_MITER)
		.value("wxJOIN_ROUND", wxJOIN_ROUND)
		.export_values();

;

	// wxPenCap file:wx/pen.h line:53
	pybind11::enum_<wxPenCap>(M(""), "wxPenCap", pybind11::arithmetic(), "")
		.value("wxCAP_INVALID", wxCAP_INVALID)
		.value("wxCAP_ROUND", wxCAP_ROUND)
		.value("wxCAP_PROJECTING", wxCAP_PROJECTING)
		.value("wxCAP_BUTT", wxCAP_BUTT)
		.export_values();

;

	{ // wxPenBase file:wx/pen.h line:63
		pybind11::class_<wxPenBase, std::shared_ptr<wxPenBase>, wxGDIObject> cl(M(""), "wxPenBase", "");
		cl.def("SetColour", (void (wxPenBase::*)(const class wxColour &)) &wxPenBase::SetColour, "C++: wxPenBase::SetColour(const class wxColour &) --> void", pybind11::arg("col"));
		cl.def("SetColour", (void (wxPenBase::*)(unsigned char, unsigned char, unsigned char)) &wxPenBase::SetColour, "C++: wxPenBase::SetColour(unsigned char, unsigned char, unsigned char) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("SetWidth", (void (wxPenBase::*)(int)) &wxPenBase::SetWidth, "C++: wxPenBase::SetWidth(int) --> void", pybind11::arg("width"));
		cl.def("SetStyle", (void (wxPenBase::*)(enum wxPenStyle)) &wxPenBase::SetStyle, "C++: wxPenBase::SetStyle(enum wxPenStyle) --> void", pybind11::arg("style"));
		cl.def("SetStipple", (void (wxPenBase::*)(const class wxBitmap &)) &wxPenBase::SetStipple, "C++: wxPenBase::SetStipple(const class wxBitmap &) --> void", pybind11::arg("stipple"));
		cl.def("SetDashes", (void (wxPenBase::*)(int, const signed char *)) &wxPenBase::SetDashes, "C++: wxPenBase::SetDashes(int, const signed char *) --> void", pybind11::arg("nb_dashes"), pybind11::arg("dash"));
		cl.def("SetJoin", (void (wxPenBase::*)(enum wxPenJoin)) &wxPenBase::SetJoin, "C++: wxPenBase::SetJoin(enum wxPenJoin) --> void", pybind11::arg("join"));
		cl.def("SetCap", (void (wxPenBase::*)(enum wxPenCap)) &wxPenBase::SetCap, "C++: wxPenBase::SetCap(enum wxPenCap) --> void", pybind11::arg("cap"));
		cl.def("GetColour", (class wxColour (wxPenBase::*)() const) &wxPenBase::GetColour, "C++: wxPenBase::GetColour() const --> class wxColour");
		cl.def("GetStipple", (class wxBitmap * (wxPenBase::*)() const) &wxPenBase::GetStipple, "C++: wxPenBase::GetStipple() const --> class wxBitmap *", pybind11::return_value_policy::automatic);
		cl.def("GetStyle", (enum wxPenStyle (wxPenBase::*)() const) &wxPenBase::GetStyle, "C++: wxPenBase::GetStyle() const --> enum wxPenStyle");
		cl.def("GetJoin", (enum wxPenJoin (wxPenBase::*)() const) &wxPenBase::GetJoin, "C++: wxPenBase::GetJoin() const --> enum wxPenJoin");
		cl.def("GetCap", (enum wxPenCap (wxPenBase::*)() const) &wxPenBase::GetCap, "C++: wxPenBase::GetCap() const --> enum wxPenCap");
		cl.def("GetWidth", (int (wxPenBase::*)() const) &wxPenBase::GetWidth, "C++: wxPenBase::GetWidth() const --> int");
		cl.def("IsTransparent", (bool (wxPenBase::*)() const) &wxPenBase::IsTransparent, "C++: wxPenBase::IsTransparent() const --> bool");
		cl.def("IsNonTransparent", (bool (wxPenBase::*)() const) &wxPenBase::IsNonTransparent, "C++: wxPenBase::IsNonTransparent() const --> bool");
		cl.def("assign", (class wxPenBase & (wxPenBase::*)(const class wxPenBase &)) &wxPenBase::operator=, "C++: wxPenBase::operator=(const class wxPenBase &) --> class wxPenBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPen file: line:18
		pybind11::class_<wxPen, std::shared_ptr<wxPen>, PyCallBack_wxPen, wxPenBase> cl(M(""), "wxPen", "");
		cl.def( pybind11::init( [](){ return new wxPen(); }, [](){ return new PyCallBack_wxPen(); } ) );
		cl.def( pybind11::init( [](const class wxColour & a0){ return new wxPen(a0); }, [](const class wxColour & a0){ return new PyCallBack_wxPen(a0); } ), "doc");
		cl.def( pybind11::init( [](const class wxColour & a0, int const & a1){ return new wxPen(a0, a1); }, [](const class wxColour & a0, int const & a1){ return new PyCallBack_wxPen(a0, a1); } ), "doc");
		cl.def( pybind11::init<const class wxColour &, int, enum wxPenStyle>(), pybind11::arg("colour"), pybind11::arg("width"), pybind11::arg("style") );

		cl.def( pybind11::init<const class wxColour &, int, int>(), pybind11::arg("col"), pybind11::arg("width"), pybind11::arg("style") );

		cl.def( pybind11::init( [](PyCallBack_wxPen const &o){ return new PyCallBack_wxPen(o); } ) );
		cl.def( pybind11::init( [](wxPen const &o){ return new wxPen(o); } ) );
		cl.def("__eq__", (bool (wxPen::*)(const class wxPen &) const) &wxPen::operator==, "C++: wxPen::operator==(const class wxPen &) const --> bool", pybind11::arg("pen"));
		cl.def("__ne__", (bool (wxPen::*)(const class wxPen &) const) &wxPen::operator!=, "C++: wxPen::operator!=(const class wxPen &) const --> bool", pybind11::arg("pen"));
		cl.def("SetColour", (void (wxPen::*)(const class wxColour &)) &wxPen::SetColour, "C++: wxPen::SetColour(const class wxColour &) --> void", pybind11::arg("colour"));
		cl.def("SetColour", (void (wxPen::*)(unsigned char, unsigned char, unsigned char)) &wxPen::SetColour, "C++: wxPen::SetColour(unsigned char, unsigned char, unsigned char) --> void", pybind11::arg("red"), pybind11::arg("green"), pybind11::arg("blue"));
		cl.def("SetCap", (void (wxPen::*)(enum wxPenCap)) &wxPen::SetCap, "C++: wxPen::SetCap(enum wxPenCap) --> void", pybind11::arg("capStyle"));
		cl.def("SetJoin", (void (wxPen::*)(enum wxPenJoin)) &wxPen::SetJoin, "C++: wxPen::SetJoin(enum wxPenJoin) --> void", pybind11::arg("joinStyle"));
		cl.def("SetStyle", (void (wxPen::*)(enum wxPenStyle)) &wxPen::SetStyle, "C++: wxPen::SetStyle(enum wxPenStyle) --> void", pybind11::arg("style"));
		cl.def("SetWidth", (void (wxPen::*)(int)) &wxPen::SetWidth, "C++: wxPen::SetWidth(int) --> void", pybind11::arg("width"));
		cl.def("SetDashes", (void (wxPen::*)(int, const signed char *)) &wxPen::SetDashes, "C++: wxPen::SetDashes(int, const signed char *) --> void", pybind11::arg("number_of_dashes"), pybind11::arg("dash"));
		cl.def("SetStipple", (void (wxPen::*)(const class wxBitmap &)) &wxPen::SetStipple, "C++: wxPen::SetStipple(const class wxBitmap &) --> void", pybind11::arg("stipple"));
		cl.def("GetColour", (class wxColour (wxPen::*)() const) &wxPen::GetColour, "C++: wxPen::GetColour() const --> class wxColour");
		cl.def("GetCap", (enum wxPenCap (wxPen::*)() const) &wxPen::GetCap, "C++: wxPen::GetCap() const --> enum wxPenCap");
		cl.def("GetJoin", (enum wxPenJoin (wxPen::*)() const) &wxPen::GetJoin, "C++: wxPen::GetJoin() const --> enum wxPenJoin");
		cl.def("GetStyle", (enum wxPenStyle (wxPen::*)() const) &wxPen::GetStyle, "C++: wxPen::GetStyle() const --> enum wxPenStyle");
		cl.def("GetWidth", (int (wxPen::*)() const) &wxPen::GetWidth, "C++: wxPen::GetWidth() const --> int");
		cl.def("GetDashCount", (int (wxPen::*)() const) &wxPen::GetDashCount, "C++: wxPen::GetDashCount() const --> int");
		cl.def("GetDash", (signed char * (wxPen::*)() const) &wxPen::GetDash, "C++: wxPen::GetDash() const --> signed char *", pybind11::return_value_policy::automatic);
		cl.def("GetStipple", (class wxBitmap * (wxPen::*)() const) &wxPen::GetStipple, "C++: wxPen::GetStipple() const --> class wxBitmap *", pybind11::return_value_policy::automatic);
		cl.def("SetStyle", (void (wxPen::*)(int)) &wxPen::SetStyle, "C++: wxPen::SetStyle(int) --> void", pybind11::arg("style"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxPen::*)() const) &wxPen::GetClassInfo, "C++: wxPen::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxPen::wxCreateObject, "C++: wxPen::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxPen & (wxPen::*)(const class wxPen &)) &wxPen::operator=, "C++: wxPen::operator=(const class wxPen &) --> class wxPen &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPenList file:wx/pen.h line:118
		pybind11::class_<wxPenList, std::shared_ptr<wxPenList>, wxGDIObjListBase> cl(M(""), "wxPenList", "");
		cl.def( pybind11::init( [](){ return new wxPenList(); } ) );
		cl.def( pybind11::init( [](wxPenList const &o){ return new wxPenList(o); } ) );
		cl.def("FindOrCreatePen", [](wxPenList &o, const class wxColour & a0) -> wxPen * { return o.FindOrCreatePen(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("colour"));
		cl.def("FindOrCreatePen", [](wxPenList &o, const class wxColour & a0, int const & a1) -> wxPen * { return o.FindOrCreatePen(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("colour"), pybind11::arg("width"));
		cl.def("FindOrCreatePen", (class wxPen * (wxPenList::*)(const class wxColour &, int, enum wxPenStyle)) &wxPenList::FindOrCreatePen, "C++: wxPenList::FindOrCreatePen(const class wxColour &, int, enum wxPenStyle) --> class wxPen *", pybind11::return_value_policy::automatic, pybind11::arg("colour"), pybind11::arg("width"), pybind11::arg("style"));
		cl.def("FindOrCreatePen", (class wxPen * (wxPenList::*)(const class wxColour &, int, int)) &wxPenList::FindOrCreatePen, "C++: wxPenList::FindOrCreatePen(const class wxColour &, int, int) --> class wxPen *", pybind11::return_value_policy::automatic, pybind11::arg("colour"), pybind11::arg("width"), pybind11::arg("style"));
	}
	// wxBrushStyle file: line:20
	pybind11::enum_<wxBrushStyle>(M(""), "wxBrushStyle", pybind11::arithmetic(), "")
		.value("wxBRUSHSTYLE_INVALID", wxBRUSHSTYLE_INVALID)
		.value("wxBRUSHSTYLE_SOLID", wxBRUSHSTYLE_SOLID)
		.value("wxBRUSHSTYLE_TRANSPARENT", wxBRUSHSTYLE_TRANSPARENT)
		.value("wxBRUSHSTYLE_STIPPLE_MASK_OPAQUE", wxBRUSHSTYLE_STIPPLE_MASK_OPAQUE)
		.value("wxBRUSHSTYLE_STIPPLE_MASK", wxBRUSHSTYLE_STIPPLE_MASK)
		.value("wxBRUSHSTYLE_STIPPLE", wxBRUSHSTYLE_STIPPLE)
		.value("wxBRUSHSTYLE_BDIAGONAL_HATCH", wxBRUSHSTYLE_BDIAGONAL_HATCH)
		.value("wxBRUSHSTYLE_CROSSDIAG_HATCH", wxBRUSHSTYLE_CROSSDIAG_HATCH)
		.value("wxBRUSHSTYLE_FDIAGONAL_HATCH", wxBRUSHSTYLE_FDIAGONAL_HATCH)
		.value("wxBRUSHSTYLE_CROSS_HATCH", wxBRUSHSTYLE_CROSS_HATCH)
		.value("wxBRUSHSTYLE_HORIZONTAL_HATCH", wxBRUSHSTYLE_HORIZONTAL_HATCH)
		.value("wxBRUSHSTYLE_VERTICAL_HATCH", wxBRUSHSTYLE_VERTICAL_HATCH)
		.value("wxBRUSHSTYLE_FIRST_HATCH", wxBRUSHSTYLE_FIRST_HATCH)
		.value("wxBRUSHSTYLE_LAST_HATCH", wxBRUSHSTYLE_LAST_HATCH)
		.export_values();

;

	{ // wxBrushBase file: line:41
		pybind11::class_<wxBrushBase, std::shared_ptr<wxBrushBase>, PyCallBack_wxBrushBase, wxGDIObject> cl(M(""), "wxBrushBase", "");
		cl.def(pybind11::init<PyCallBack_wxBrushBase const &>());
		cl.def( pybind11::init( [](){ return new PyCallBack_wxBrushBase(); } ) );
		cl.def("SetColour", (void (wxBrushBase::*)(const class wxColour &)) &wxBrushBase::SetColour, "C++: wxBrushBase::SetColour(const class wxColour &) --> void", pybind11::arg("col"));
		cl.def("SetColour", (void (wxBrushBase::*)(unsigned char, unsigned char, unsigned char)) &wxBrushBase::SetColour, "C++: wxBrushBase::SetColour(unsigned char, unsigned char, unsigned char) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("SetStyle", (void (wxBrushBase::*)(enum wxBrushStyle)) &wxBrushBase::SetStyle, "C++: wxBrushBase::SetStyle(enum wxBrushStyle) --> void", pybind11::arg("style"));
		cl.def("SetStipple", (void (wxBrushBase::*)(const class wxBitmap &)) &wxBrushBase::SetStipple, "C++: wxBrushBase::SetStipple(const class wxBitmap &) --> void", pybind11::arg("stipple"));
		cl.def("GetColour", (class wxColour (wxBrushBase::*)() const) &wxBrushBase::GetColour, "C++: wxBrushBase::GetColour() const --> class wxColour");
		cl.def("GetStyle", (enum wxBrushStyle (wxBrushBase::*)() const) &wxBrushBase::GetStyle, "C++: wxBrushBase::GetStyle() const --> enum wxBrushStyle");
		cl.def("GetStipple", (class wxBitmap * (wxBrushBase::*)() const) &wxBrushBase::GetStipple, "C++: wxBrushBase::GetStipple() const --> class wxBitmap *", pybind11::return_value_policy::automatic);
		cl.def("IsHatch", (bool (wxBrushBase::*)() const) &wxBrushBase::IsHatch, "C++: wxBrushBase::IsHatch() const --> bool");
		cl.def("IsTransparent", (bool (wxBrushBase::*)() const) &wxBrushBase::IsTransparent, "C++: wxBrushBase::IsTransparent() const --> bool");
		cl.def("IsNonTransparent", (bool (wxBrushBase::*)() const) &wxBrushBase::IsNonTransparent, "C++: wxBrushBase::IsNonTransparent() const --> bool");
		cl.def("assign", (class wxBrushBase & (wxBrushBase::*)(const class wxBrushBase &)) &wxBrushBase::operator=, "C++: wxBrushBase::operator=(const class wxBrushBase &) --> class wxBrushBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxBrush file: line:19
		pybind11::class_<wxBrush, std::shared_ptr<wxBrush>, PyCallBack_wxBrush, wxBrushBase> cl(M(""), "wxBrush", "");
		cl.def( pybind11::init( [](){ return new wxBrush(); }, [](){ return new PyCallBack_wxBrush(); } ) );
		cl.def( pybind11::init( [](const class wxColour & a0){ return new wxBrush(a0); }, [](const class wxColour & a0){ return new PyCallBack_wxBrush(a0); } ), "doc");
		cl.def( pybind11::init<const class wxColour &, enum wxBrushStyle>(), pybind11::arg("colour"), pybind11::arg("style") );

		cl.def( pybind11::init<const class wxColour &, int>(), pybind11::arg("col"), pybind11::arg("style") );

		cl.def( pybind11::init<const class wxBitmap &>(), pybind11::arg("stippleBitmap") );

		cl.def( pybind11::init( [](PyCallBack_wxBrush const &o){ return new PyCallBack_wxBrush(o); } ) );
		cl.def( pybind11::init( [](wxBrush const &o){ return new wxBrush(o); } ) );
		cl.def("__eq__", (bool (wxBrush::*)(const class wxBrush &) const) &wxBrush::operator==, "C++: wxBrush::operator==(const class wxBrush &) const --> bool", pybind11::arg("brush"));
		cl.def("__ne__", (bool (wxBrush::*)(const class wxBrush &) const) &wxBrush::operator!=, "C++: wxBrush::operator!=(const class wxBrush &) const --> bool", pybind11::arg("brush"));
		cl.def("GetStyle", (enum wxBrushStyle (wxBrush::*)() const) &wxBrush::GetStyle, "C++: wxBrush::GetStyle() const --> enum wxBrushStyle");
		cl.def("GetColour", (class wxColour (wxBrush::*)() const) &wxBrush::GetColour, "C++: wxBrush::GetColour() const --> class wxColour");
		cl.def("GetStipple", (class wxBitmap * (wxBrush::*)() const) &wxBrush::GetStipple, "C++: wxBrush::GetStipple() const --> class wxBitmap *", pybind11::return_value_policy::automatic);
		cl.def("SetColour", (void (wxBrush::*)(const class wxColour &)) &wxBrush::SetColour, "C++: wxBrush::SetColour(const class wxColour &) --> void", pybind11::arg("col"));
		cl.def("SetColour", (void (wxBrush::*)(unsigned char, unsigned char, unsigned char)) &wxBrush::SetColour, "C++: wxBrush::SetColour(unsigned char, unsigned char, unsigned char) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("SetStyle", (void (wxBrush::*)(enum wxBrushStyle)) &wxBrush::SetStyle, "C++: wxBrush::SetStyle(enum wxBrushStyle) --> void", pybind11::arg("style"));
		cl.def("SetStipple", (void (wxBrush::*)(const class wxBitmap &)) &wxBrush::SetStipple, "C++: wxBrush::SetStipple(const class wxBitmap &) --> void", pybind11::arg("stipple"));
		cl.def("SetStyle", (void (wxBrush::*)(int)) &wxBrush::SetStyle, "C++: wxBrush::SetStyle(int) --> void", pybind11::arg("style"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxBrush::*)() const) &wxBrush::GetClassInfo, "C++: wxBrush::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxBrush::wxCreateObject, "C++: wxBrush::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxBrush & (wxBrush::*)(const class wxBrush &)) &wxBrush::operator=, "C++: wxBrush::operator=(const class wxBrush &) --> class wxBrush &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
