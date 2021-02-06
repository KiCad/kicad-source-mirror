#include <core/typeinfo.h> // KICAD_T
#include <eda_item.h> // EDA_ITEM
#include <eda_rect.h> // EDA_RECT
#include <eda_text.h> // EDA_TEXT
#include <eda_text.h> // EDA_TEXT_HJUSTIFY_T
#include <eda_text.h> // EDA_TEXT_VJUSTIFY_T
#include <eda_text.h> // SHAPE_COMPOUND
#include <eda_text.h> // TEXT_EFFECTS
#include <eda_units.h> // EDA_UNITS
#include <fill_type.h> // FILL_TYPE
#include <gal/color4d.h> // EDA_COLOR_T
#include <gal/color4d.h> // KIGFX::COLOR4D
#include <iterator> // __gnu_cxx::__normal_iterator
#include <layers_id_colors_and_visibility.h> // PCB_LAYER_ID
#include <lib_circle.h> // LIB_CIRCLE
#include <lib_field.h> // LIB_FIELD
#include <lib_id.h> // LIB_ID
#include <lib_item.h> // 
#include <lib_item.h> // LIB_ITEM
#include <lib_pin.h> // LIB_PIN
#include <lib_symbol.h> // LIB_PART
#include <lib_symbol.h> // PART_DRAW_OPTIONS
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
#include <outline_mode.h> // OUTLINE_MODE
#include <render_settings.h> // KIGFX::RENDER_SETTINGS
#include <richio.h> // OUTPUTFORMATTER
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

// LIB_CIRCLE file:lib_circle.h line:33
struct PyCallBack_LIB_CIRCLE : public LIB_CIRCLE {
	using LIB_CIRCLE::LIB_CIRCLE;

	class wxString GetClass() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "GetClass");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_CIRCLE::GetClass();
	}
	class wxString GetTypeName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "GetTypeName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_CIRCLE::GetTypeName();
	}
	bool HitTest(const class wxPoint & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_CIRCLE::HitTest(a0, a1);
	}
	bool HitTest(const class EDA_RECT & a0, bool a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_CIRCLE::HitTest(a0, a1, a2);
	}
	int GetPenWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "GetPenWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_CIRCLE::GetPenWidth();
	}
	const class EDA_RECT GetBoundingBox() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "GetBoundingBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class EDA_RECT>::value) {
				static pybind11::detail::override_caster_t<const class EDA_RECT> caster;
				return pybind11::detail::cast_ref<const class EDA_RECT>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class EDA_RECT>(std::move(o));
		}
		return LIB_CIRCLE::GetBoundingBox();
	}
	void BeginEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "BeginEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_CIRCLE::BeginEdit(a0);
	}
	void CalcEdit(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "CalcEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_CIRCLE::CalcEdit(a0);
	}
	void Offset(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "Offset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_CIRCLE::Offset(a0);
	}
	void MoveTo(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "MoveTo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_CIRCLE::MoveTo(a0);
	}
	class wxPoint GetPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "GetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint>::value) {
				static pybind11::detail::override_caster_t<class wxPoint> caster;
				return pybind11::detail::cast_ref<class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint>(std::move(o));
		}
		return LIB_CIRCLE::GetPosition();
	}
	void MirrorHorizontal(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "MirrorHorizontal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_CIRCLE::MirrorHorizontal(a0);
	}
	void MirrorVertical(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "MirrorVertical");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_CIRCLE::MirrorVertical(a0);
	}
	void Rotate(const class wxPoint & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "Rotate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_CIRCLE::Rotate(a0, a1);
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_CIRCLE::GetWidth();
	}
	void SetWidth(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "SetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_CIRCLE::SetWidth(a0);
	}
	class wxString GetSelectMenuText(enum EDA_UNITS a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "GetSelectMenuText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_CIRCLE::GetSelectMenuText(a0);
	}
	class EDA_ITEM * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class EDA_ITEM *>::value) {
				static pybind11::detail::override_caster_t<class EDA_ITEM *> caster;
				return pybind11::detail::cast_ref<class EDA_ITEM *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class EDA_ITEM *>(std::move(o));
		}
		return LIB_CIRCLE::Clone();
	}
	bool ContinueEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "ContinueEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "EndEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "Print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "SetPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "compare");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "SetParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "GetFocusPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "Matches");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "Replace");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_CIRCLE *>(this), "IsReplaceable");
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

// EDA_TEXT file:eda_text.h line:119
struct PyCallBack_EDA_TEXT : public EDA_TEXT {
	using EDA_TEXT::EDA_TEXT;

	const class wxString & GetText() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const EDA_TEXT *>(this), "GetText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString &>::value) {
				static pybind11::detail::override_caster_t<const class wxString &> caster;
				return pybind11::detail::cast_ref<const class wxString &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString &>(std::move(o));
		}
		return EDA_TEXT::GetText();
	}
	class wxString GetShownText(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const EDA_TEXT *>(this), "GetShownText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return EDA_TEXT::GetShownText(a0);
	}
	void SetText(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const EDA_TEXT *>(this), "SetText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return EDA_TEXT::SetText(a0);
	}
	void SetTextAngle(double a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const EDA_TEXT *>(this), "SetTextAngle");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return EDA_TEXT::SetTextAngle(a0);
	}
	bool TextHitTest(const class wxPoint & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const EDA_TEXT *>(this), "TextHitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return EDA_TEXT::TextHitTest(a0, a1);
	}
	bool TextHitTest(const class EDA_RECT & a0, bool a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const EDA_TEXT *>(this), "TextHitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return EDA_TEXT::TextHitTest(a0, a1, a2);
	}
	void Format(class OUTPUTFORMATTER * a0, int a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const EDA_TEXT *>(this), "Format");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return EDA_TEXT::Format(a0, a1, a2);
	}
	double GetDrawRotation() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const EDA_TEXT *>(this), "GetDrawRotation");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<double>::value) {
				static pybind11::detail::override_caster_t<double> caster;
				return pybind11::detail::cast_ref<double>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<double>(std::move(o));
		}
		return EDA_TEXT::GetDrawRotation();
	}
};

void bind_lib_circle(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // LIB_CIRCLE file:lib_circle.h line:33
		pybind11::class_<LIB_CIRCLE, std::shared_ptr<LIB_CIRCLE>, PyCallBack_LIB_CIRCLE, LIB_ITEM> cl(M(""), "LIB_CIRCLE", "");
		cl.def( pybind11::init<class LIB_PART *>(), pybind11::arg("aParent") );

		cl.def( pybind11::init( [](PyCallBack_LIB_CIRCLE const &o){ return new PyCallBack_LIB_CIRCLE(o); } ) );
		cl.def( pybind11::init( [](LIB_CIRCLE const &o){ return new LIB_CIRCLE(o); } ) );
		cl.def("GetClass", (class wxString (LIB_CIRCLE::*)() const) &LIB_CIRCLE::GetClass, "C++: LIB_CIRCLE::GetClass() const --> class wxString");
		cl.def("GetTypeName", (class wxString (LIB_CIRCLE::*)() const) &LIB_CIRCLE::GetTypeName, "C++: LIB_CIRCLE::GetTypeName() const --> class wxString");
		cl.def("HitTest", [](LIB_CIRCLE const &o, const class wxPoint & a0) -> bool { return o.HitTest(a0); }, "", pybind11::arg("aPosition"));
		cl.def("HitTest", (bool (LIB_CIRCLE::*)(const class wxPoint &, int) const) &LIB_CIRCLE::HitTest, "C++: LIB_CIRCLE::HitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPosition"), pybind11::arg("aAccuracy"));
		cl.def("HitTest", [](LIB_CIRCLE const &o, const class EDA_RECT & a0, bool const & a1) -> bool { return o.HitTest(a0, a1); }, "", pybind11::arg("aRect"), pybind11::arg("aContained"));
		cl.def("HitTest", (bool (LIB_CIRCLE::*)(const class EDA_RECT &, bool, int) const) &LIB_CIRCLE::HitTest, "C++: LIB_CIRCLE::HitTest(const class EDA_RECT &, bool, int) const --> bool", pybind11::arg("aRect"), pybind11::arg("aContained"), pybind11::arg("aAccuracy"));
		cl.def("GetPenWidth", (int (LIB_CIRCLE::*)() const) &LIB_CIRCLE::GetPenWidth, "C++: LIB_CIRCLE::GetPenWidth() const --> int");
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_CIRCLE::*)() const) &LIB_CIRCLE::GetBoundingBox, "C++: LIB_CIRCLE::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("BeginEdit", (void (LIB_CIRCLE::*)(const class wxPoint)) &LIB_CIRCLE::BeginEdit, "C++: LIB_CIRCLE::BeginEdit(const class wxPoint) --> void", pybind11::arg("aStartPoint"));
		cl.def("CalcEdit", (void (LIB_CIRCLE::*)(const class wxPoint &)) &LIB_CIRCLE::CalcEdit, "C++: LIB_CIRCLE::CalcEdit(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("Offset", (void (LIB_CIRCLE::*)(const class wxPoint &)) &LIB_CIRCLE::Offset, "C++: LIB_CIRCLE::Offset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("MoveTo", (void (LIB_CIRCLE::*)(const class wxPoint &)) &LIB_CIRCLE::MoveTo, "C++: LIB_CIRCLE::MoveTo(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("GetPosition", (class wxPoint (LIB_CIRCLE::*)() const) &LIB_CIRCLE::GetPosition, "C++: LIB_CIRCLE::GetPosition() const --> class wxPoint");
		cl.def("SetEnd", (void (LIB_CIRCLE::*)(const class wxPoint &)) &LIB_CIRCLE::SetEnd, "C++: LIB_CIRCLE::SetEnd(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("GetEnd", (class wxPoint (LIB_CIRCLE::*)() const) &LIB_CIRCLE::GetEnd, "C++: LIB_CIRCLE::GetEnd() const --> class wxPoint");
		cl.def("MirrorHorizontal", (void (LIB_CIRCLE::*)(const class wxPoint &)) &LIB_CIRCLE::MirrorHorizontal, "C++: LIB_CIRCLE::MirrorHorizontal(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("MirrorVertical", (void (LIB_CIRCLE::*)(const class wxPoint &)) &LIB_CIRCLE::MirrorVertical, "C++: LIB_CIRCLE::MirrorVertical(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("Rotate", [](LIB_CIRCLE &o, const class wxPoint & a0) -> void { return o.Rotate(a0); }, "", pybind11::arg("aCenter"));
		cl.def("Rotate", (void (LIB_CIRCLE::*)(const class wxPoint &, bool)) &LIB_CIRCLE::Rotate, "C++: LIB_CIRCLE::Rotate(const class wxPoint &, bool) --> void", pybind11::arg("aCenter"), pybind11::arg("aRotateCCW"));
		cl.def("GetWidth", (int (LIB_CIRCLE::*)() const) &LIB_CIRCLE::GetWidth, "C++: LIB_CIRCLE::GetWidth() const --> int");
		cl.def("SetWidth", (void (LIB_CIRCLE::*)(int)) &LIB_CIRCLE::SetWidth, "C++: LIB_CIRCLE::SetWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("SetRadius", (void (LIB_CIRCLE::*)(int)) &LIB_CIRCLE::SetRadius, "C++: LIB_CIRCLE::SetRadius(int) --> void", pybind11::arg("aRadius"));
		cl.def("GetRadius", (int (LIB_CIRCLE::*)() const) &LIB_CIRCLE::GetRadius, "C++: LIB_CIRCLE::GetRadius() const --> int");
		cl.def("GetSelectMenuText", (class wxString (LIB_CIRCLE::*)(enum EDA_UNITS) const) &LIB_CIRCLE::GetSelectMenuText, "C++: LIB_CIRCLE::GetSelectMenuText(enum EDA_UNITS) const --> class wxString", pybind11::arg("aUnits"));
		cl.def("GetMenuImage", (int (LIB_CIRCLE::*)() const) &LIB_CIRCLE::GetMenuImage, "C++: LIB_CIRCLE::GetMenuImage() const --> int");
		cl.def("Clone", (class EDA_ITEM * (LIB_CIRCLE::*)() const) &LIB_CIRCLE::Clone, "C++: LIB_CIRCLE::Clone() const --> class EDA_ITEM *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class LIB_CIRCLE & (LIB_CIRCLE::*)(const class LIB_CIRCLE &)) &LIB_CIRCLE::operator=, "C++: LIB_CIRCLE::operator=(const class LIB_CIRCLE &) --> class LIB_CIRCLE &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// OUTLINE_MODE file:outline_mode.h line:24
	pybind11::enum_<OUTLINE_MODE>(M(""), "OUTLINE_MODE", pybind11::arithmetic(), "")
		.value("SKETCH", SKETCH)
		.value("FILLED", FILLED)
		.export_values();

;

	// EDA_TEXT_HJUSTIFY_T file:eda_text.h line:61
	pybind11::enum_<EDA_TEXT_HJUSTIFY_T>(M(""), "EDA_TEXT_HJUSTIFY_T", pybind11::arithmetic(), "")
		.value("GR_TEXT_HJUSTIFY_LEFT", GR_TEXT_HJUSTIFY_LEFT)
		.value("GR_TEXT_HJUSTIFY_CENTER", GR_TEXT_HJUSTIFY_CENTER)
		.value("GR_TEXT_HJUSTIFY_RIGHT", GR_TEXT_HJUSTIFY_RIGHT)
		.export_values();

;

	// EDA_TEXT_VJUSTIFY_T file:eda_text.h line:68
	pybind11::enum_<EDA_TEXT_VJUSTIFY_T>(M(""), "EDA_TEXT_VJUSTIFY_T", pybind11::arithmetic(), "")
		.value("GR_TEXT_VJUSTIFY_TOP", GR_TEXT_VJUSTIFY_TOP)
		.value("GR_TEXT_VJUSTIFY_CENTER", GR_TEXT_VJUSTIFY_CENTER)
		.value("GR_TEXT_VJUSTIFY_BOTTOM", GR_TEXT_VJUSTIFY_BOTTOM)
		.export_values();

;

	{ // TEXT_EFFECTS file:eda_text.h line:90
		pybind11::class_<TEXT_EFFECTS, std::shared_ptr<TEXT_EFFECTS>> cl(M(""), "TEXT_EFFECTS", "A container for text effects.\n\n These fields are bundled so they can be easily copied together as a lot. The privacy\n policy is established by client (incorporating) code.");
		cl.def( pybind11::init( [](){ return new TEXT_EFFECTS(); } ), "doc" );
		cl.def( pybind11::init<int>(), pybind11::arg("aSetOfBits") );

		cl.def_readwrite("bits", &TEXT_EFFECTS::bits);
		cl.def_readwrite("hjustify", &TEXT_EFFECTS::hjustify);
		cl.def_readwrite("vjustify", &TEXT_EFFECTS::vjustify);
		cl.def_readwrite("size", &TEXT_EFFECTS::size);
		cl.def_readwrite("penwidth", &TEXT_EFFECTS::penwidth);
		cl.def_readwrite("angle", &TEXT_EFFECTS::angle);
		cl.def_readwrite("pos", &TEXT_EFFECTS::pos);
		cl.def("Bit", (void (TEXT_EFFECTS::*)(int, bool)) &TEXT_EFFECTS::Bit, "C++: TEXT_EFFECTS::Bit(int, bool) --> void", pybind11::arg("aBit"), pybind11::arg("aValue"));
		cl.def("Bit", (bool (TEXT_EFFECTS::*)(int) const) &TEXT_EFFECTS::Bit, "C++: TEXT_EFFECTS::Bit(int) const --> bool", pybind11::arg("aBit"));
		cl.def("assign", (struct TEXT_EFFECTS & (TEXT_EFFECTS::*)(const struct TEXT_EFFECTS &)) &TEXT_EFFECTS::operator=, "C++: TEXT_EFFECTS::operator=(const struct TEXT_EFFECTS &) --> struct TEXT_EFFECTS &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // EDA_TEXT file:eda_text.h line:119
		pybind11::class_<EDA_TEXT, std::shared_ptr<EDA_TEXT>, PyCallBack_EDA_TEXT> cl(M(""), "EDA_TEXT", "A mix-in class (via multiple inheritance) that handles texts such as labels, parts,\n components, or footprints.  Because it's a mix-in class, care is used to provide\n function names (accessors) that to not collide with function names likely to be seen\n in the combined derived classes.");
		cl.def( pybind11::init( [](){ return new EDA_TEXT(); }, [](){ return new PyCallBack_EDA_TEXT(); } ), "doc");
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("text") );

		cl.def( pybind11::init( [](PyCallBack_EDA_TEXT const &o){ return new PyCallBack_EDA_TEXT(o); } ) );
		cl.def( pybind11::init( [](EDA_TEXT const &o){ return new EDA_TEXT(o); } ) );
		cl.def("GetText", (const class wxString & (EDA_TEXT::*)() const) &EDA_TEXT::GetText, "Return the string associated with the text object.\n\n \n a const wxString reference containing the string of the item.\n\nC++: EDA_TEXT::GetText() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetShownText", [](EDA_TEXT const &o) -> wxString { return o.GetShownText(); }, "");
		cl.def("GetShownText", (class wxString (EDA_TEXT::*)(int) const) &EDA_TEXT::GetShownText, "Return the string actually shown after processing of the base text.\n\n \n is used to prevent infinite recursions and loops when expanding\n text variables.\n\nC++: EDA_TEXT::GetShownText(int) const --> class wxString", pybind11::arg("aDepth"));
		cl.def("GetShownText", (class wxString (EDA_TEXT::*)(bool *) const) &EDA_TEXT::GetShownText, "A version of GetShownText() which also indicates whether or not the text needs\n to be processed for text variables.\n\n \n [out]\n\nC++: EDA_TEXT::GetShownText(bool *) const --> class wxString", pybind11::arg("processTextVars"));
		cl.def("ShortenedShownText", (class wxString (EDA_TEXT::*)() const) &EDA_TEXT::ShortenedShownText, "Returns a shortened version (max 15 characters) of the shown text\n\nC++: EDA_TEXT::ShortenedShownText() const --> class wxString");
		cl.def("SetText", (void (EDA_TEXT::*)(const class wxString &)) &EDA_TEXT::SetText, "C++: EDA_TEXT::SetText(const class wxString &) --> void", pybind11::arg("aText"));
		cl.def("SetTextThickness", (void (EDA_TEXT::*)(int)) &EDA_TEXT::SetTextThickness, "The TextThickness is that set by the user.  The EffectiveTextPenWidth also factors\n in bold text and thickness clamping.\n\nC++: EDA_TEXT::SetTextThickness(int) --> void", pybind11::arg("aWidth"));
		cl.def("GetTextThickness", (int (EDA_TEXT::*)() const) &EDA_TEXT::GetTextThickness, "C++: EDA_TEXT::GetTextThickness() const --> int");
		cl.def("GetEffectiveTextPenWidth", [](EDA_TEXT const &o) -> int { return o.GetEffectiveTextPenWidth(); }, "");
		cl.def("GetEffectiveTextPenWidth", (int (EDA_TEXT::*)(int) const) &EDA_TEXT::GetEffectiveTextPenWidth, "The EffectiveTextPenWidth uses the text thickness if > 1 or aDefaultWidth.\n\nC++: EDA_TEXT::GetEffectiveTextPenWidth(int) const --> int", pybind11::arg("aDefaultWidth"));
		cl.def("SetTextAngle", (void (EDA_TEXT::*)(double)) &EDA_TEXT::SetTextAngle, "C++: EDA_TEXT::SetTextAngle(double) --> void", pybind11::arg("aAngle"));
		cl.def("GetTextAngle", (double (EDA_TEXT::*)() const) &EDA_TEXT::GetTextAngle, "C++: EDA_TEXT::GetTextAngle() const --> double");
		cl.def("GetTextAngleDegrees", (double (EDA_TEXT::*)() const) &EDA_TEXT::GetTextAngleDegrees, "C++: EDA_TEXT::GetTextAngleDegrees() const --> double");
		cl.def("GetTextAngleRadians", (double (EDA_TEXT::*)() const) &EDA_TEXT::GetTextAngleRadians, "C++: EDA_TEXT::GetTextAngleRadians() const --> double");
		cl.def("SetItalic", (void (EDA_TEXT::*)(bool)) &EDA_TEXT::SetItalic, "C++: EDA_TEXT::SetItalic(bool) --> void", pybind11::arg("isItalic"));
		cl.def("IsItalic", (bool (EDA_TEXT::*)() const) &EDA_TEXT::IsItalic, "C++: EDA_TEXT::IsItalic() const --> bool");
		cl.def("SetBold", (void (EDA_TEXT::*)(bool)) &EDA_TEXT::SetBold, "C++: EDA_TEXT::SetBold(bool) --> void", pybind11::arg("aBold"));
		cl.def("IsBold", (bool (EDA_TEXT::*)() const) &EDA_TEXT::IsBold, "C++: EDA_TEXT::IsBold() const --> bool");
		cl.def("SetVisible", (void (EDA_TEXT::*)(bool)) &EDA_TEXT::SetVisible, "C++: EDA_TEXT::SetVisible(bool) --> void", pybind11::arg("aVisible"));
		cl.def("IsVisible", (bool (EDA_TEXT::*)() const) &EDA_TEXT::IsVisible, "C++: EDA_TEXT::IsVisible() const --> bool");
		cl.def("SetMirrored", (void (EDA_TEXT::*)(bool)) &EDA_TEXT::SetMirrored, "C++: EDA_TEXT::SetMirrored(bool) --> void", pybind11::arg("isMirrored"));
		cl.def("IsMirrored", (bool (EDA_TEXT::*)() const) &EDA_TEXT::IsMirrored, "C++: EDA_TEXT::IsMirrored() const --> bool");
		cl.def("SetMultilineAllowed", (void (EDA_TEXT::*)(bool)) &EDA_TEXT::SetMultilineAllowed, "true if ok to use multiline option, false if ok to use only single line\n               text.  (Single line is faster in calculations than multiline.)\n\nC++: EDA_TEXT::SetMultilineAllowed(bool) --> void", pybind11::arg("aAllow"));
		cl.def("IsMultilineAllowed", (bool (EDA_TEXT::*)() const) &EDA_TEXT::IsMultilineAllowed, "C++: EDA_TEXT::IsMultilineAllowed() const --> bool");
		cl.def("GetHorizJustify", (enum EDA_TEXT_HJUSTIFY_T (EDA_TEXT::*)() const) &EDA_TEXT::GetHorizJustify, "C++: EDA_TEXT::GetHorizJustify() const --> enum EDA_TEXT_HJUSTIFY_T");
		cl.def("GetVertJustify", (enum EDA_TEXT_VJUSTIFY_T (EDA_TEXT::*)() const) &EDA_TEXT::GetVertJustify, "C++: EDA_TEXT::GetVertJustify() const --> enum EDA_TEXT_VJUSTIFY_T");
		cl.def("SetHorizJustify", (void (EDA_TEXT::*)(enum EDA_TEXT_HJUSTIFY_T)) &EDA_TEXT::SetHorizJustify, "C++: EDA_TEXT::SetHorizJustify(enum EDA_TEXT_HJUSTIFY_T) --> void", pybind11::arg("aType"));
		cl.def("SetVertJustify", (void (EDA_TEXT::*)(enum EDA_TEXT_VJUSTIFY_T)) &EDA_TEXT::SetVertJustify, "C++: EDA_TEXT::SetVertJustify(enum EDA_TEXT_VJUSTIFY_T) --> void", pybind11::arg("aType"));
		cl.def("SetEffects", (void (EDA_TEXT::*)(const class EDA_TEXT &)) &EDA_TEXT::SetEffects, "Set the text effects from another instance.\n\n #TEXT_EFFECTS is not exposed in the public API, but includes everything except the actual\n text string itself.\n\nC++: EDA_TEXT::SetEffects(const class EDA_TEXT &) --> void", pybind11::arg("aSrc"));
		cl.def("SwapEffects", (void (EDA_TEXT::*)(class EDA_TEXT &)) &EDA_TEXT::SwapEffects, "Swap the text effects of the two involved instances.\n\n #TEXT_EFFECTS is not exposed in the public API, but includes everything except the actual\n text string itself.\n\nC++: EDA_TEXT::SwapEffects(class EDA_TEXT &) --> void", pybind11::arg("aTradingPartner"));
		cl.def("SwapText", (void (EDA_TEXT::*)(class EDA_TEXT &)) &EDA_TEXT::SwapText, "C++: EDA_TEXT::SwapText(class EDA_TEXT &) --> void", pybind11::arg("aTradingPartner"));
		cl.def("CopyText", (void (EDA_TEXT::*)(const class EDA_TEXT &)) &EDA_TEXT::CopyText, "C++: EDA_TEXT::CopyText(const class EDA_TEXT &) --> void", pybind11::arg("aSrc"));
		cl.def("Replace", (bool (EDA_TEXT::*)(const class wxFindReplaceData &)) &EDA_TEXT::Replace, "Helper function used in search and replace dialog.\n\n Perform a text replace using the find and replace criteria in \n\n \n A reference to a wxFindReplaceData object containing the\n                    search and replace criteria.\n \n\n True if the text item was modified, otherwise false.\n\nC++: EDA_TEXT::Replace(const class wxFindReplaceData &) --> bool", pybind11::arg("aSearchData"));
		cl.def("IsDefaultFormatting", (bool (EDA_TEXT::*)() const) &EDA_TEXT::IsDefaultFormatting, "C++: EDA_TEXT::IsDefaultFormatting() const --> bool");
		cl.def("SetTextSize", (void (EDA_TEXT::*)(const class wxSize &)) &EDA_TEXT::SetTextSize, "C++: EDA_TEXT::SetTextSize(const class wxSize &) --> void", pybind11::arg("aNewSize"));
		cl.def("GetTextSize", (const class wxSize & (EDA_TEXT::*)() const) &EDA_TEXT::GetTextSize, "C++: EDA_TEXT::GetTextSize() const --> const class wxSize &", pybind11::return_value_policy::automatic);
		cl.def("SetTextWidth", (void (EDA_TEXT::*)(int)) &EDA_TEXT::SetTextWidth, "C++: EDA_TEXT::SetTextWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("GetTextWidth", (int (EDA_TEXT::*)() const) &EDA_TEXT::GetTextWidth, "C++: EDA_TEXT::GetTextWidth() const --> int");
		cl.def("SetTextHeight", (void (EDA_TEXT::*)(int)) &EDA_TEXT::SetTextHeight, "C++: EDA_TEXT::SetTextHeight(int) --> void", pybind11::arg("aHeight"));
		cl.def("GetTextHeight", (int (EDA_TEXT::*)() const) &EDA_TEXT::GetTextHeight, "C++: EDA_TEXT::GetTextHeight() const --> int");
		cl.def("SetTextPos", (void (EDA_TEXT::*)(const class wxPoint &)) &EDA_TEXT::SetTextPos, "C++: EDA_TEXT::SetTextPos(const class wxPoint &) --> void", pybind11::arg("aPoint"));
		cl.def("GetTextPos", (const class wxPoint & (EDA_TEXT::*)() const) &EDA_TEXT::GetTextPos, "C++: EDA_TEXT::GetTextPos() const --> const class wxPoint &", pybind11::return_value_policy::automatic);
		cl.def("SetTextX", (void (EDA_TEXT::*)(int)) &EDA_TEXT::SetTextX, "C++: EDA_TEXT::SetTextX(int) --> void", pybind11::arg("aX"));
		cl.def("SetTextY", (void (EDA_TEXT::*)(int)) &EDA_TEXT::SetTextY, "C++: EDA_TEXT::SetTextY(int) --> void", pybind11::arg("aY"));
		cl.def("Offset", (void (EDA_TEXT::*)(const class wxPoint &)) &EDA_TEXT::Offset, "C++: EDA_TEXT::Offset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("Empty", (void (EDA_TEXT::*)()) &EDA_TEXT::Empty, "C++: EDA_TEXT::Empty() --> void");
		cl.def_static("MapHorizJustify", (enum EDA_TEXT_HJUSTIFY_T (*)(int)) &EDA_TEXT::MapHorizJustify, "C++: EDA_TEXT::MapHorizJustify(int) --> enum EDA_TEXT_HJUSTIFY_T", pybind11::arg("aHorizJustify"));
		cl.def_static("MapVertJustify", (enum EDA_TEXT_VJUSTIFY_T (*)(int)) &EDA_TEXT::MapVertJustify, "C++: EDA_TEXT::MapVertJustify(int) --> enum EDA_TEXT_VJUSTIFY_T", pybind11::arg("aVertJustify"));
		cl.def("Print", [](EDA_TEXT &o, const class KIGFX::RENDER_SETTINGS * a0, const class wxPoint & a1, class KIGFX::COLOR4D const & a2) -> void { return o.Print(a0, a1, a2); }, "", pybind11::arg("aSettings"), pybind11::arg("aOffset"), pybind11::arg("aColor"));
		cl.def("Print", (void (EDA_TEXT::*)(const class KIGFX::RENDER_SETTINGS *, const class wxPoint &, class KIGFX::COLOR4D, enum OUTLINE_MODE)) &EDA_TEXT::Print, "Print this text object to the device context \n\n \n the current Device Context.\n \n\n draw offset (usually (0,0)).\n \n\n text color.\n \n\n #FILLED or #SKETCH.\n\nC++: EDA_TEXT::Print(const class KIGFX::RENDER_SETTINGS *, const class wxPoint &, class KIGFX::COLOR4D, enum OUTLINE_MODE) --> void", pybind11::arg("aSettings"), pybind11::arg("aOffset"), pybind11::arg("aColor"), pybind11::arg("aDisplay_mode"));
		cl.def("TransformToSegmentList", (int (EDA_TEXT::*)() const) &EDA_TEXT::TransformToSegmentList, "Convert the text shape to a list of segment.\n\n Each segment is stored as 2 wxPoints: the starting point and the ending point\n there are therefore 2*n points.\n\nC++: EDA_TEXT::TransformToSegmentList() const --> int");
		cl.def("TextHitTest", [](EDA_TEXT const &o, const class wxPoint & a0) -> bool { return o.TextHitTest(a0); }, "", pybind11::arg("aPoint"));
		cl.def("TextHitTest", (bool (EDA_TEXT::*)(const class wxPoint &, int) const) &EDA_TEXT::TextHitTest, "Test if  is within the bounds of this object.\n\n \n A wxPoint to test.\n \n\n Amount to inflate the bounding box.\n \n\n true if a hit, else false.\n\nC++: EDA_TEXT::TextHitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPoint"), pybind11::arg("aAccuracy"));
		cl.def("TextHitTest", [](EDA_TEXT const &o, const class EDA_RECT & a0, bool const & a1) -> bool { return o.TextHitTest(a0, a1); }, "", pybind11::arg("aRect"), pybind11::arg("aContains"));
		cl.def("TextHitTest", (bool (EDA_TEXT::*)(const class EDA_RECT &, bool, int) const) &EDA_TEXT::TextHitTest, "Test if object bounding box is contained within or intersects \n\n \n Rect to test against.\n \n\n Test for containment instead of intersection if true.\n \n\n Amount to inflate the bounding box.\n \n\n true if a hit, else false.\n\nC++: EDA_TEXT::TextHitTest(const class EDA_RECT &, bool, int) const --> bool", pybind11::arg("aRect"), pybind11::arg("aContains"), pybind11::arg("aAccuracy"));
		cl.def("LenSize", (int (EDA_TEXT::*)(const class wxString &, int) const) &EDA_TEXT::LenSize, "the text length in internal units.\n \n\n the line of text to consider.  For single line text, this parameter\n              is always m_Text.\n \n\n the stroke width of the text.\n\nC++: EDA_TEXT::LenSize(const class wxString &, int) const --> int", pybind11::arg("aLine"), pybind11::arg("aThickness"));
		cl.def("GetTextBox", [](EDA_TEXT const &o) -> EDA_RECT { return o.GetTextBox(); }, "");
		cl.def("GetTextBox", [](EDA_TEXT const &o, int const & a0) -> EDA_RECT { return o.GetTextBox(a0); }, "", pybind11::arg("aLine"));
		cl.def("GetTextBox", (class EDA_RECT (EDA_TEXT::*)(int, bool) const) &EDA_TEXT::GetTextBox, "Useful in multiline texts to calculate the full text or a line area (for zones filling,\n locate functions....)\n\n \n The line of text to consider.  Pass -1 for all lines.\n \n\n Invert the Y axis when calculating bounding box.\n \n\n the rect containing the line of text (i.e. the position and the size of one line)\n         this rectangle is calculated for 0 orient text.\n         If orientation is not 0 the rect must be rotated to match the physical area\n\nC++: EDA_TEXT::GetTextBox(int, bool) const --> class EDA_RECT", pybind11::arg("aLine"), pybind11::arg("aInvertY"));
		cl.def("GetInterline", (int (EDA_TEXT::*)() const) &EDA_TEXT::GetInterline, "Return the distance between two lines of text.\n\n Calculates the distance (pitch) between two lines of text.  This distance includes the\n interline distance plus room for characters like j, {, and [.  It also used for single\n line text, to calculate the text bounding box.\n\nC++: EDA_TEXT::GetInterline() const --> int");
		cl.def("GetTextStyleName", (class wxString (EDA_TEXT::*)() const) &EDA_TEXT::GetTextStyleName, "a wxString with the style name( Normal, Italic, Bold, Bold+Italic).\n\nC++: EDA_TEXT::GetTextStyleName() const --> class wxString");
		cl.def("GetLinePositions", (void (EDA_TEXT::*)(int &, int) const) &EDA_TEXT::GetLinePositions, "Populate  with the position of each line of a multiline text, according\n to the vertical justification and the rotation of the whole text.\n\n \n is the list to populate by the wxPoint positions.\n \n\n is the number of lines (not recalculated here for efficiency reasons.\n\nC++: EDA_TEXT::GetLinePositions(int &, int) const --> void", pybind11::arg("aPositions"), pybind11::arg("aLineCount"));
		cl.def("Format", (void (EDA_TEXT::*)(class OUTPUTFORMATTER *, int, int) const) &EDA_TEXT::Format, "Output the object to  in s-expression form.\n\n \n The #OUTPUTFORMATTER object to write to.\n \n\n The indentation next level.\n \n\n The control bit definition for object specific formatting.\n \n\n IO_ERROR on write error.\n\nC++: EDA_TEXT::Format(class OUTPUTFORMATTER *, int, int) const --> void", pybind11::arg("aFormatter"), pybind11::arg("aNestLevel"), pybind11::arg("aControlBits"));
		cl.def("GetDrawRotation", (double (EDA_TEXT::*)() const) &EDA_TEXT::GetDrawRotation, "C++: EDA_TEXT::GetDrawRotation() const --> double");
		cl.def("assign", (class EDA_TEXT & (EDA_TEXT::*)(const class EDA_TEXT &)) &EDA_TEXT::operator=, "C++: EDA_TEXT::operator=(const class EDA_TEXT &) --> class EDA_TEXT &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
