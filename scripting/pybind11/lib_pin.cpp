#include <core/typeinfo.h> // KICAD_T
#include <eda_item.h> // EDA_ITEM
#include <eda_rect.h> // EDA_RECT
#include <eda_units.h> // EDA_UNITS
#include <fill_type.h> // FILL_TYPE
#include <gal/color4d.h> // EDA_COLOR_T
#include <gal/color4d.h> // KIGFX::COLOR4D
#include <iterator> // __gnu_cxx::__normal_iterator
#include <layers_id_colors_and_visibility.h> // PCB_LAYER_ID
#include <lib_field.h> // LIB_FIELD
#include <lib_id.h> // LIB_ID
#include <lib_item.h> // 
#include <lib_item.h> // LIB_ITEM
#include <lib_pin.h> // DrawPinOrient
#include <lib_pin.h> // LIB_PIN
#include <lib_pin.h> // LIB_PIN::ALT
#include <lib_polyline.h> // LIB_POLYLINE
#include <lib_symbol.h> // LIB_PART
#include <lib_symbol.h> // PART_DRAW_OPTIONS
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
#include <pin_type.h> // ELECTRICAL_PINTYPE
#include <pin_type.h> // GRAPHIC_PINSHAPE
#include <render_settings.h> // KIGFX::RENDER_SETTINGS
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <transform.h> // TRANSFORM
#include <view/view_item.h> // KIGFX::VIEW_ITEM
#include <wx/dc.h> // wxDC
#include <wx/dc.h> // wxDCImpl
#include <wx/dc.h> // wxDrawObject
#include <wx/dc.h> // wxFloodFillStyle
#include <wx/dc.h> // wxFontMetrics
#include <wx/dc.h> // wxMappingMode
#include <wx/dc.h> // wxRasterOperationMode
#include <wx/fdrepdlg.h> // wxFindReplaceData

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// LIB_PIN file:lib_pin.h line:55
struct PyCallBack_LIB_PIN : public LIB_PIN {
	using LIB_PIN::LIB_PIN;

	class wxString GetClass() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "GetClass");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_PIN::GetClass();
	}
	class wxString GetTypeName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "GetTypeName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_PIN::GetTypeName();
	}
	void print(const class KIGFX::RENDER_SETTINGS * a0, const class wxPoint & a1, void * a2, const class TRANSFORM & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "print");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_PIN::print(a0, a1, a2, a3);
	}
	bool HitTest(const class wxPoint & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_PIN::HitTest(a0, a1);
	}
	bool HitTest(const class EDA_RECT & a0, bool a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_PIN::HitTest(a0, a1, a2);
	}
	const class EDA_RECT GetBoundingBox() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "GetBoundingBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class EDA_RECT>::value) {
				static pybind11::detail::override_caster_t<const class EDA_RECT> caster;
				return pybind11::detail::cast_ref<const class EDA_RECT>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class EDA_RECT>(std::move(o));
		}
		return LIB_PIN::GetBoundingBox();
	}
	int GetPenWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "GetPenWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_PIN::GetPenWidth();
	}
	void Offset(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "Offset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_PIN::Offset(a0);
	}
	void MoveTo(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "MoveTo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_PIN::MoveTo(a0);
	}
	class wxPoint GetPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "GetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint>::value) {
				static pybind11::detail::override_caster_t<class wxPoint> caster;
				return pybind11::detail::cast_ref<class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint>(std::move(o));
		}
		return LIB_PIN::GetPosition();
	}
	void SetPosition(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "SetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_PIN::SetPosition(a0);
	}
	void MirrorHorizontal(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "MirrorHorizontal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_PIN::MirrorHorizontal(a0);
	}
	void MirrorVertical(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "MirrorVertical");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_PIN::MirrorVertical(a0);
	}
	void Rotate(const class wxPoint & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "Rotate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_PIN::Rotate(a0, a1);
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_PIN::GetWidth();
	}
	void SetWidth(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "SetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_PIN::SetWidth(a0);
	}
	class wxString GetSelectMenuText(enum EDA_UNITS a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "GetSelectMenuText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_PIN::GetSelectMenuText(a0);
	}
	class EDA_ITEM * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class EDA_ITEM *>::value) {
				static pybind11::detail::override_caster_t<class EDA_ITEM *> caster;
				return pybind11::detail::cast_ref<class EDA_ITEM *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class EDA_ITEM *>(std::move(o));
		}
		return LIB_PIN::Clone();
	}
	void CalcEdit(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "CalcEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_PIN::CalcEdit(a0);
	}
	void BeginEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "BeginEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ITEM::BeginEdit(a0);
	}
	bool ContinueEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "ContinueEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_ITEM::ContinueEdit(a0);
	}
	void EndEdit() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "EndEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ITEM::EndEdit();
	}
	void Print(const class KIGFX::RENDER_SETTINGS * a0, const class wxPoint & a1, void * a2, const class TRANSFORM & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "Print");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ITEM::Print(a0, a1, a2, a3);
	}
	int compare(const class LIB_ITEM & a0, enum LIB_ITEM::COMPARE_FLAGS a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "compare");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_ITEM::compare(a0, a1);
	}
	void SetParent(class EDA_ITEM * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "SetParent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return EDA_ITEM::SetParent(a0);
	}
	const class wxPoint GetFocusPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "GetFocusPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxPoint>::value) {
				static pybind11::detail::override_caster_t<const class wxPoint> caster;
				return pybind11::detail::cast_ref<const class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxPoint>(std::move(o));
		}
		return EDA_ITEM::GetFocusPosition();
	}
	bool Matches(const class wxFindReplaceData & a0, void * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "Matches");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return EDA_ITEM::Matches(a0, a1);
	}
	bool Replace(const class wxFindReplaceData & a0, void * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "Replace");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return EDA_ITEM::Replace(a0, a1);
	}
	bool IsReplaceable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PIN *>(this), "IsReplaceable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return EDA_ITEM::IsReplaceable();
	}
};

// LIB_POLYLINE file:lib_polyline.h line:31
struct PyCallBack_LIB_POLYLINE : public LIB_POLYLINE {
	using LIB_POLYLINE::LIB_POLYLINE;

	class wxString GetClass() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "GetClass");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_POLYLINE::GetClass();
	}
	class wxString GetTypeName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "GetTypeName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_POLYLINE::GetTypeName();
	}
	bool HitTest(const class wxPoint & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_POLYLINE::HitTest(a0, a1);
	}
	bool HitTest(const class EDA_RECT & a0, bool a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_POLYLINE::HitTest(a0, a1, a2);
	}
	const class EDA_RECT GetBoundingBox() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "GetBoundingBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class EDA_RECT>::value) {
				static pybind11::detail::override_caster_t<const class EDA_RECT> caster;
				return pybind11::detail::cast_ref<const class EDA_RECT>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class EDA_RECT>(std::move(o));
		}
		return LIB_POLYLINE::GetBoundingBox();
	}
	int GetPenWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "GetPenWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_POLYLINE::GetPenWidth();
	}
	void BeginEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "BeginEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_POLYLINE::BeginEdit(a0);
	}
	void CalcEdit(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "CalcEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_POLYLINE::CalcEdit(a0);
	}
	bool ContinueEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "ContinueEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_POLYLINE::ContinueEdit(a0);
	}
	void EndEdit() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "EndEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_POLYLINE::EndEdit();
	}
	void Offset(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "Offset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_POLYLINE::Offset(a0);
	}
	void MoveTo(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "MoveTo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_POLYLINE::MoveTo(a0);
	}
	class wxPoint GetPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "GetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint>::value) {
				static pybind11::detail::override_caster_t<class wxPoint> caster;
				return pybind11::detail::cast_ref<class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint>(std::move(o));
		}
		return LIB_POLYLINE::GetPosition();
	}
	void MirrorHorizontal(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "MirrorHorizontal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_POLYLINE::MirrorHorizontal(a0);
	}
	void MirrorVertical(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "MirrorVertical");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_POLYLINE::MirrorVertical(a0);
	}
	void Rotate(const class wxPoint & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "Rotate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_POLYLINE::Rotate(a0, a1);
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_POLYLINE::GetWidth();
	}
	void SetWidth(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "SetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_POLYLINE::SetWidth(a0);
	}
	class wxString GetSelectMenuText(enum EDA_UNITS a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "GetSelectMenuText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_POLYLINE::GetSelectMenuText(a0);
	}
	class EDA_ITEM * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class EDA_ITEM *>::value) {
				static pybind11::detail::override_caster_t<class EDA_ITEM *> caster;
				return pybind11::detail::cast_ref<class EDA_ITEM *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class EDA_ITEM *>(std::move(o));
		}
		return LIB_POLYLINE::Clone();
	}
	void Print(const class KIGFX::RENDER_SETTINGS * a0, const class wxPoint & a1, void * a2, const class TRANSFORM & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "Print");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ITEM::Print(a0, a1, a2, a3);
	}
	void SetPosition(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "SetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ITEM::SetPosition(a0);
	}
	int compare(const class LIB_ITEM & a0, enum LIB_ITEM::COMPARE_FLAGS a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "compare");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_ITEM::compare(a0, a1);
	}
	void print(const class KIGFX::RENDER_SETTINGS * a0, const class wxPoint & a1, void * a2, const class TRANSFORM & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "print");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"LIB_ITEM::print\"");
	}
	void SetParent(class EDA_ITEM * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "SetParent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return EDA_ITEM::SetParent(a0);
	}
	const class wxPoint GetFocusPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "GetFocusPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxPoint>::value) {
				static pybind11::detail::override_caster_t<const class wxPoint> caster;
				return pybind11::detail::cast_ref<const class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxPoint>(std::move(o));
		}
		return EDA_ITEM::GetFocusPosition();
	}
	bool Matches(const class wxFindReplaceData & a0, void * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "Matches");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return EDA_ITEM::Matches(a0, a1);
	}
	bool Replace(const class wxFindReplaceData & a0, void * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "Replace");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return EDA_ITEM::Replace(a0, a1);
	}
	bool IsReplaceable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_POLYLINE *>(this), "IsReplaceable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return EDA_ITEM::IsReplaceable();
	}
};

void bind_lib_pin(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// DrawPinOrient file:lib_pin.h line:47
	pybind11::enum_<DrawPinOrient>(M(""), "DrawPinOrient", pybind11::arithmetic(), "The component library pin object orientations.")
		.value("PIN_RIGHT", PIN_RIGHT)
		.value("PIN_LEFT", PIN_LEFT)
		.value("PIN_UP", PIN_UP)
		.value("PIN_DOWN", PIN_DOWN)
		.export_values();

;

	{ // LIB_PIN file:lib_pin.h line:55
		pybind11::class_<LIB_PIN, std::shared_ptr<LIB_PIN>, PyCallBack_LIB_PIN, LIB_ITEM> cl(M(""), "LIB_PIN", "");
		cl.def( pybind11::init<class LIB_PART *>(), pybind11::arg("aParent") );

		cl.def( pybind11::init<class LIB_PART *, const class wxString &, const class wxString &, int, enum ELECTRICAL_PINTYPE, int, int, int, int, const class wxPoint &, int>(), pybind11::arg("aParent"), pybind11::arg("aName"), pybind11::arg("aNumber"), pybind11::arg("aOrientation"), pybind11::arg("aPinType"), pybind11::arg("aLength"), pybind11::arg("aNameTextSize"), pybind11::arg("aNumTextSize"), pybind11::arg("aConvert"), pybind11::arg("aPos"), pybind11::arg("aUnit") );

		cl.def( pybind11::init( [](PyCallBack_LIB_PIN const &o){ return new PyCallBack_LIB_PIN(o); } ) );
		cl.def( pybind11::init( [](LIB_PIN const &o){ return new LIB_PIN(o); } ) );
		cl.def_static("GetCanonicalElectricalTypeName", (const class wxString (*)(enum ELECTRICAL_PINTYPE)) &LIB_PIN::GetCanonicalElectricalTypeName, "return a string giving the electrical type of a pin.\n Can be used when a known, not translated name is needed (for instance in net lists)\n \n\n is the electrical type (see enum ELECTRICAL_PINTYPE )\n \n\n The electrical name for a pin type (see enun MsgPinElectricType for names).\n\nC++: LIB_PIN::GetCanonicalElectricalTypeName(enum ELECTRICAL_PINTYPE) --> const class wxString", pybind11::arg("aType"));
		cl.def("GetClass", (class wxString (LIB_PIN::*)() const) &LIB_PIN::GetClass, "C++: LIB_PIN::GetClass() const --> class wxString");
		cl.def("GetTypeName", (class wxString (LIB_PIN::*)() const) &LIB_PIN::GetTypeName, "C++: LIB_PIN::GetTypeName() const --> class wxString");
		cl.def("GetOrientation", (int (LIB_PIN::*)() const) &LIB_PIN::GetOrientation, "C++: LIB_PIN::GetOrientation() const --> int");
		cl.def("SetOrientation", (void (LIB_PIN::*)(int)) &LIB_PIN::SetOrientation, "C++: LIB_PIN::SetOrientation(int) --> void", pybind11::arg("aOrientation"));
		cl.def("GetShape", (enum GRAPHIC_PINSHAPE (LIB_PIN::*)() const) &LIB_PIN::GetShape, "C++: LIB_PIN::GetShape() const --> enum GRAPHIC_PINSHAPE");
		cl.def("SetShape", (void (LIB_PIN::*)(enum GRAPHIC_PINSHAPE)) &LIB_PIN::SetShape, "C++: LIB_PIN::SetShape(enum GRAPHIC_PINSHAPE) --> void", pybind11::arg("aShape"));
		cl.def("GetLength", (int (LIB_PIN::*)() const) &LIB_PIN::GetLength, "C++: LIB_PIN::GetLength() const --> int");
		cl.def("SetLength", (void (LIB_PIN::*)(int)) &LIB_PIN::SetLength, "C++: LIB_PIN::SetLength(int) --> void", pybind11::arg("aLength"));
		cl.def("GetType", (enum ELECTRICAL_PINTYPE (LIB_PIN::*)() const) &LIB_PIN::GetType, "C++: LIB_PIN::GetType() const --> enum ELECTRICAL_PINTYPE");
		cl.def("SetType", (void (LIB_PIN::*)(enum ELECTRICAL_PINTYPE)) &LIB_PIN::SetType, "C++: LIB_PIN::SetType(enum ELECTRICAL_PINTYPE) --> void", pybind11::arg("aType"));
		cl.def("GetCanonicalElectricalTypeName", (const class wxString (LIB_PIN::*)() const) &LIB_PIN::GetCanonicalElectricalTypeName, "C++: LIB_PIN::GetCanonicalElectricalTypeName() const --> const class wxString");
		cl.def("GetElectricalTypeName", (const class wxString (LIB_PIN::*)() const) &LIB_PIN::GetElectricalTypeName, "C++: LIB_PIN::GetElectricalTypeName() const --> const class wxString");
		cl.def("IsVisible", (bool (LIB_PIN::*)() const) &LIB_PIN::IsVisible, "C++: LIB_PIN::IsVisible() const --> bool");
		cl.def("SetVisible", (void (LIB_PIN::*)(bool)) &LIB_PIN::SetVisible, "C++: LIB_PIN::SetVisible(bool) --> void", pybind11::arg("aVisible"));
		cl.def("GetName", (const class wxString & (LIB_PIN::*)() const) &LIB_PIN::GetName, "C++: LIB_PIN::GetName() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("SetName", (void (LIB_PIN::*)(const class wxString &)) &LIB_PIN::SetName, "C++: LIB_PIN::SetName(const class wxString &) --> void", pybind11::arg("aName"));
		cl.def("GetNumber", (const class wxString & (LIB_PIN::*)() const) &LIB_PIN::GetNumber, "C++: LIB_PIN::GetNumber() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("SetNumber", (void (LIB_PIN::*)(const class wxString &)) &LIB_PIN::SetNumber, "C++: LIB_PIN::SetNumber(const class wxString &) --> void", pybind11::arg("aNumber"));
		cl.def("GetNameTextSize", (int (LIB_PIN::*)() const) &LIB_PIN::GetNameTextSize, "C++: LIB_PIN::GetNameTextSize() const --> int");
		cl.def("SetNameTextSize", (void (LIB_PIN::*)(int)) &LIB_PIN::SetNameTextSize, "C++: LIB_PIN::SetNameTextSize(int) --> void", pybind11::arg("aSize"));
		cl.def("GetNumberTextSize", (int (LIB_PIN::*)() const) &LIB_PIN::GetNumberTextSize, "C++: LIB_PIN::GetNumberTextSize() const --> int");
		cl.def("SetNumberTextSize", (void (LIB_PIN::*)(int)) &LIB_PIN::SetNumberTextSize, "C++: LIB_PIN::SetNumberTextSize(int) --> void", pybind11::arg("aSize"));
		cl.def("GetAlternates", (int & (LIB_PIN::*)()) &LIB_PIN::GetAlternates, "C++: LIB_PIN::GetAlternates() --> int &", pybind11::return_value_policy::automatic);
		cl.def("GetAlt", (struct LIB_PIN::ALT (LIB_PIN::*)(const class wxString &)) &LIB_PIN::GetAlt, "C++: LIB_PIN::GetAlt(const class wxString &) --> struct LIB_PIN::ALT", pybind11::arg("aAlt"));
		cl.def("print", (void (LIB_PIN::*)(const class KIGFX::RENDER_SETTINGS *, const class wxPoint &, void *, const class TRANSFORM &)) &LIB_PIN::print, "Print a pin, with or without the pin texts\n\n \n Offset to draw\n \n\n = used here as a boolean indicating whether or not to draw the pin\n                electrical types\n \n\n Transform Matrix (rotation, mirror ..)\n\nC++: LIB_PIN::print(const class KIGFX::RENDER_SETTINGS *, const class wxPoint &, void *, const class TRANSFORM &) --> void", pybind11::arg("aSettings"), pybind11::arg("aOffset"), pybind11::arg("aData"), pybind11::arg("aTransform"));
		cl.def("PinDrawOrient", (int (LIB_PIN::*)(const class TRANSFORM &) const) &LIB_PIN::PinDrawOrient, "Return the pin real orientation (PIN_UP, PIN_DOWN, PIN_RIGHT, PIN_LEFT),\n according to its orientation and the matrix transform (rot, mirror) \n\n \n Transform matrix\n\nC++: LIB_PIN::PinDrawOrient(const class TRANSFORM &) const --> int", pybind11::arg("aTransform"));
		cl.def("HitTest", [](LIB_PIN const &o, const class wxPoint & a0) -> bool { return o.HitTest(a0); }, "", pybind11::arg("aPosition"));
		cl.def("HitTest", (bool (LIB_PIN::*)(const class wxPoint &, int) const) &LIB_PIN::HitTest, "C++: LIB_PIN::HitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPosition"), pybind11::arg("aAccuracy"));
		cl.def("HitTest", [](LIB_PIN const &o, const class EDA_RECT & a0, bool const & a1) -> bool { return o.HitTest(a0, a1); }, "", pybind11::arg("aRect"), pybind11::arg("aContained"));
		cl.def("HitTest", (bool (LIB_PIN::*)(const class EDA_RECT &, bool, int) const) &LIB_PIN::HitTest, "C++: LIB_PIN::HitTest(const class EDA_RECT &, bool, int) const --> bool", pybind11::arg("aRect"), pybind11::arg("aContained"), pybind11::arg("aAccuracy"));
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_PIN::*)() const) &LIB_PIN::GetBoundingBox, "C++: LIB_PIN::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("GetBoundingBox", [](LIB_PIN const &o, bool const & a0) -> const EDA_RECT { return o.GetBoundingBox(a0); }, "", pybind11::arg("aIncludeInvisibles"));
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_PIN::*)(bool, bool) const) &LIB_PIN::GetBoundingBox, "- if false, do not include labels for invisible pins\n      in the calculation.\n\nC++: LIB_PIN::GetBoundingBox(bool, bool) const --> const class EDA_RECT", pybind11::arg("aIncludeInvisibles"), pybind11::arg("aPinOnly"));
		cl.def("IsPowerConnection", (bool (LIB_PIN::*)() const) &LIB_PIN::IsPowerConnection, "Return whether this pin forms an implicit power connection: i.e., is hidden\n and of type POWER_IN.\n\nC++: LIB_PIN::IsPowerConnection() const --> bool");
		cl.def("GetPenWidth", (int (LIB_PIN::*)() const) &LIB_PIN::GetPenWidth, "C++: LIB_PIN::GetPenWidth() const --> int");
		cl.def("Offset", (void (LIB_PIN::*)(const class wxPoint &)) &LIB_PIN::Offset, "C++: LIB_PIN::Offset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("MoveTo", (void (LIB_PIN::*)(const class wxPoint &)) &LIB_PIN::MoveTo, "C++: LIB_PIN::MoveTo(const class wxPoint &) --> void", pybind11::arg("aNewPosition"));
		cl.def("GetPosition", (class wxPoint (LIB_PIN::*)() const) &LIB_PIN::GetPosition, "C++: LIB_PIN::GetPosition() const --> class wxPoint");
		cl.def("SetPosition", (void (LIB_PIN::*)(const class wxPoint &)) &LIB_PIN::SetPosition, "C++: LIB_PIN::SetPosition(const class wxPoint &) --> void", pybind11::arg("aPos"));
		cl.def("MirrorHorizontal", (void (LIB_PIN::*)(const class wxPoint &)) &LIB_PIN::MirrorHorizontal, "C++: LIB_PIN::MirrorHorizontal(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("MirrorVertical", (void (LIB_PIN::*)(const class wxPoint &)) &LIB_PIN::MirrorVertical, "C++: LIB_PIN::MirrorVertical(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("Rotate", [](LIB_PIN &o, const class wxPoint & a0) -> void { return o.Rotate(a0); }, "", pybind11::arg("aCenter"));
		cl.def("Rotate", (void (LIB_PIN::*)(const class wxPoint &, bool)) &LIB_PIN::Rotate, "C++: LIB_PIN::Rotate(const class wxPoint &, bool) --> void", pybind11::arg("aCenter"), pybind11::arg("aRotateCCW"));
		cl.def("GetWidth", (int (LIB_PIN::*)() const) &LIB_PIN::GetWidth, "C++: LIB_PIN::GetWidth() const --> int");
		cl.def("SetWidth", (void (LIB_PIN::*)(int)) &LIB_PIN::SetWidth, "C++: LIB_PIN::SetWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("GetMenuImage", (int (LIB_PIN::*)() const) &LIB_PIN::GetMenuImage, "C++: LIB_PIN::GetMenuImage() const --> int");
		cl.def("GetSelectMenuText", (class wxString (LIB_PIN::*)(enum EDA_UNITS) const) &LIB_PIN::GetSelectMenuText, "C++: LIB_PIN::GetSelectMenuText(enum EDA_UNITS) const --> class wxString", pybind11::arg("aUnits"));
		cl.def("Clone", (class EDA_ITEM * (LIB_PIN::*)() const) &LIB_PIN::Clone, "C++: LIB_PIN::Clone() const --> class EDA_ITEM *", pybind11::return_value_policy::automatic);
		cl.def("CalcEdit", (void (LIB_PIN::*)(const class wxPoint &)) &LIB_PIN::CalcEdit, "C++: LIB_PIN::CalcEdit(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("assign", (class LIB_PIN & (LIB_PIN::*)(const class LIB_PIN &)) &LIB_PIN::operator=, "C++: LIB_PIN::operator=(const class LIB_PIN &) --> class LIB_PIN &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // LIB_PIN::ALT file:lib_pin.h line:58
			auto & enclosing_class = cl;
			pybind11::class_<LIB_PIN::ALT, std::shared_ptr<LIB_PIN::ALT>> cl(enclosing_class, "ALT", "");
			cl.def( pybind11::init( [](){ return new LIB_PIN::ALT(); } ) );
			cl.def( pybind11::init( [](LIB_PIN::ALT const &o){ return new LIB_PIN::ALT(o); } ) );
			cl.def_readwrite("m_Name", &LIB_PIN::ALT::m_Name);
			cl.def_readwrite("m_Shape", &LIB_PIN::ALT::m_Shape);
			cl.def_readwrite("m_Type", &LIB_PIN::ALT::m_Type);
			cl.def("assign", (struct LIB_PIN::ALT & (LIB_PIN::ALT::*)(const struct LIB_PIN::ALT &)) &LIB_PIN::ALT::operator=, "C++: LIB_PIN::ALT::operator=(const struct LIB_PIN::ALT &) --> struct LIB_PIN::ALT &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		}

	}
	{ // LIB_POLYLINE file:lib_polyline.h line:31
		pybind11::class_<LIB_POLYLINE, std::shared_ptr<LIB_POLYLINE>, PyCallBack_LIB_POLYLINE, LIB_ITEM> cl(M(""), "LIB_POLYLINE", "");
		cl.def( pybind11::init<class LIB_PART *>(), pybind11::arg("aParent") );

		cl.def( pybind11::init( [](PyCallBack_LIB_POLYLINE const &o){ return new PyCallBack_LIB_POLYLINE(o); } ) );
		cl.def( pybind11::init( [](LIB_POLYLINE const &o){ return new LIB_POLYLINE(o); } ) );
		cl.def("GetClass", (class wxString (LIB_POLYLINE::*)() const) &LIB_POLYLINE::GetClass, "C++: LIB_POLYLINE::GetClass() const --> class wxString");
		cl.def("GetTypeName", (class wxString (LIB_POLYLINE::*)() const) &LIB_POLYLINE::GetTypeName, "C++: LIB_POLYLINE::GetTypeName() const --> class wxString");
		cl.def("ClearPoints", (void (LIB_POLYLINE::*)()) &LIB_POLYLINE::ClearPoints, "C++: LIB_POLYLINE::ClearPoints() --> void");
		cl.def("Reserve", (void (LIB_POLYLINE::*)(unsigned long)) &LIB_POLYLINE::Reserve, "C++: LIB_POLYLINE::Reserve(unsigned long) --> void", pybind11::arg("aPointCount"));
		cl.def("AddPoint", (void (LIB_POLYLINE::*)(const class wxPoint &)) &LIB_POLYLINE::AddPoint, "C++: LIB_POLYLINE::AddPoint(const class wxPoint &) --> void", pybind11::arg("aPoint"));
		cl.def("GetPolyPoints", (const int & (LIB_POLYLINE::*)() const) &LIB_POLYLINE::GetPolyPoints, "C++: LIB_POLYLINE::GetPolyPoints() const --> const int &", pybind11::return_value_policy::automatic);
		cl.def("DeleteSegment", (void (LIB_POLYLINE::*)(const class wxPoint)) &LIB_POLYLINE::DeleteSegment, "Delete the segment at \n     \n\nC++: LIB_POLYLINE::DeleteSegment(const class wxPoint) --> void", pybind11::arg("aPosition"));
		cl.def("AddCorner", (void (LIB_POLYLINE::*)(const class wxPoint &)) &LIB_POLYLINE::AddCorner, "C++: LIB_POLYLINE::AddCorner(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("RemoveCorner", (void (LIB_POLYLINE::*)(int)) &LIB_POLYLINE::RemoveCorner, "C++: LIB_POLYLINE::RemoveCorner(int) --> void", pybind11::arg("aIdx"));
		cl.def("GetCornerCount", (unsigned int (LIB_POLYLINE::*)() const) &LIB_POLYLINE::GetCornerCount, "the number of corners\n\nC++: LIB_POLYLINE::GetCornerCount() const --> unsigned int");
		cl.def("HitTest", [](LIB_POLYLINE const &o, const class wxPoint & a0) -> bool { return o.HitTest(a0); }, "", pybind11::arg("aPosition"));
		cl.def("HitTest", (bool (LIB_POLYLINE::*)(const class wxPoint &, int) const) &LIB_POLYLINE::HitTest, "C++: LIB_POLYLINE::HitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPosition"), pybind11::arg("aAccuracy"));
		cl.def("HitTest", [](LIB_POLYLINE const &o, const class EDA_RECT & a0, bool const & a1) -> bool { return o.HitTest(a0, a1); }, "", pybind11::arg("aRect"), pybind11::arg("aContained"));
		cl.def("HitTest", (bool (LIB_POLYLINE::*)(const class EDA_RECT &, bool, int) const) &LIB_POLYLINE::HitTest, "C++: LIB_POLYLINE::HitTest(const class EDA_RECT &, bool, int) const --> bool", pybind11::arg("aRect"), pybind11::arg("aContained"), pybind11::arg("aAccuracy"));
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_POLYLINE::*)() const) &LIB_POLYLINE::GetBoundingBox, "C++: LIB_POLYLINE::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("GetPenWidth", (int (LIB_POLYLINE::*)() const) &LIB_POLYLINE::GetPenWidth, "C++: LIB_POLYLINE::GetPenWidth() const --> int");
		cl.def("BeginEdit", (void (LIB_POLYLINE::*)(const class wxPoint)) &LIB_POLYLINE::BeginEdit, "C++: LIB_POLYLINE::BeginEdit(const class wxPoint) --> void", pybind11::arg("aStartPoint"));
		cl.def("CalcEdit", (void (LIB_POLYLINE::*)(const class wxPoint &)) &LIB_POLYLINE::CalcEdit, "C++: LIB_POLYLINE::CalcEdit(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("ContinueEdit", (bool (LIB_POLYLINE::*)(const class wxPoint)) &LIB_POLYLINE::ContinueEdit, "C++: LIB_POLYLINE::ContinueEdit(const class wxPoint) --> bool", pybind11::arg("aNextPoint"));
		cl.def("EndEdit", (void (LIB_POLYLINE::*)()) &LIB_POLYLINE::EndEdit, "C++: LIB_POLYLINE::EndEdit() --> void");
		cl.def("Offset", (void (LIB_POLYLINE::*)(const class wxPoint &)) &LIB_POLYLINE::Offset, "C++: LIB_POLYLINE::Offset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("MoveTo", (void (LIB_POLYLINE::*)(const class wxPoint &)) &LIB_POLYLINE::MoveTo, "C++: LIB_POLYLINE::MoveTo(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("GetPosition", (class wxPoint (LIB_POLYLINE::*)() const) &LIB_POLYLINE::GetPosition, "C++: LIB_POLYLINE::GetPosition() const --> class wxPoint");
		cl.def("MirrorHorizontal", (void (LIB_POLYLINE::*)(const class wxPoint &)) &LIB_POLYLINE::MirrorHorizontal, "C++: LIB_POLYLINE::MirrorHorizontal(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("MirrorVertical", (void (LIB_POLYLINE::*)(const class wxPoint &)) &LIB_POLYLINE::MirrorVertical, "C++: LIB_POLYLINE::MirrorVertical(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("Rotate", [](LIB_POLYLINE &o, const class wxPoint & a0) -> void { return o.Rotate(a0); }, "", pybind11::arg("aCenter"));
		cl.def("Rotate", (void (LIB_POLYLINE::*)(const class wxPoint &, bool)) &LIB_POLYLINE::Rotate, "C++: LIB_POLYLINE::Rotate(const class wxPoint &, bool) --> void", pybind11::arg("aCenter"), pybind11::arg("aRotateCCW"));
		cl.def("GetWidth", (int (LIB_POLYLINE::*)() const) &LIB_POLYLINE::GetWidth, "C++: LIB_POLYLINE::GetWidth() const --> int");
		cl.def("SetWidth", (void (LIB_POLYLINE::*)(int)) &LIB_POLYLINE::SetWidth, "C++: LIB_POLYLINE::SetWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("GetSelectMenuText", (class wxString (LIB_POLYLINE::*)(enum EDA_UNITS) const) &LIB_POLYLINE::GetSelectMenuText, "C++: LIB_POLYLINE::GetSelectMenuText(enum EDA_UNITS) const --> class wxString", pybind11::arg("aUnits"));
		cl.def("GetMenuImage", (int (LIB_POLYLINE::*)() const) &LIB_POLYLINE::GetMenuImage, "C++: LIB_POLYLINE::GetMenuImage() const --> int");
		cl.def("Clone", (class EDA_ITEM * (LIB_POLYLINE::*)() const) &LIB_POLYLINE::Clone, "C++: LIB_POLYLINE::Clone() const --> class EDA_ITEM *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class LIB_POLYLINE & (LIB_POLYLINE::*)(const class LIB_POLYLINE &)) &LIB_POLYLINE::operator=, "C++: LIB_POLYLINE::operator=(const class LIB_POLYLINE &) --> class LIB_POLYLINE &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
