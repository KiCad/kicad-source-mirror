#include <core/typeinfo.h> // KICAD_T
#include <eda_item.h> // EDA_ITEM
#include <eda_rect.h> // EDA_RECT
#include <eda_text.h> // EDA_TEXT
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
#include <lib_pin.h> // LIB_PIN
#include <lib_rectangle.h> // LIB_RECTANGLE
#include <lib_symbol.h> // LIB_PART
#include <lib_symbol.h> // PART_DRAW_OPTIONS
#include <lib_text.h> // LIB_TEXT
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
#include <render_settings.h> // KIGFX::RENDER_SETTINGS
#include <richio.h> // OUTPUTFORMATTER
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <tool/tool_action.h> // TOOL_ACTION
#include <tool/tool_event.h> // CONTEXT_MENU_TRIGGER
#include <tool/tool_event.h> // TOOL_ACTIONS
#include <tool/tool_event.h> // TOOL_ACTION_FLAGS
#include <tool/tool_event.h> // TOOL_ACTION_SCOPE
#include <tool/tool_event.h> // TOOL_EVENT
#include <tool/tool_event.h> // TOOL_EVENT_CATEGORY
#include <tool/tool_event.h> // TOOL_MODIFIERS
#include <tool/tool_event.h> // TOOL_MOUSE_BUTTONS
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

// LIB_RECTANGLE file:lib_rectangle.h line:31
struct PyCallBack_LIB_RECTANGLE : public LIB_RECTANGLE {
	using LIB_RECTANGLE::LIB_RECTANGLE;

	class wxString GetClass() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "GetClass");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_RECTANGLE::GetClass();
	}
	class wxString GetTypeName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "GetTypeName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_RECTANGLE::GetTypeName();
	}
	bool HitTest(const class wxPoint & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_RECTANGLE::HitTest(a0, a1);
	}
	bool HitTest(const class EDA_RECT & a0, bool a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_RECTANGLE::HitTest(a0, a1, a2);
	}
	int GetPenWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "GetPenWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_RECTANGLE::GetPenWidth();
	}
	const class EDA_RECT GetBoundingBox() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "GetBoundingBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class EDA_RECT>::value) {
				static pybind11::detail::override_caster_t<const class EDA_RECT> caster;
				return pybind11::detail::cast_ref<const class EDA_RECT>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class EDA_RECT>(std::move(o));
		}
		return LIB_RECTANGLE::GetBoundingBox();
	}
	void BeginEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "BeginEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_RECTANGLE::BeginEdit(a0);
	}
	void CalcEdit(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "CalcEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_RECTANGLE::CalcEdit(a0);
	}
	void Offset(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "Offset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_RECTANGLE::Offset(a0);
	}
	void MoveTo(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "MoveTo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_RECTANGLE::MoveTo(a0);
	}
	class wxPoint GetPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "GetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint>::value) {
				static pybind11::detail::override_caster_t<class wxPoint> caster;
				return pybind11::detail::cast_ref<class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint>(std::move(o));
		}
		return LIB_RECTANGLE::GetPosition();
	}
	void MirrorHorizontal(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "MirrorHorizontal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_RECTANGLE::MirrorHorizontal(a0);
	}
	void MirrorVertical(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "MirrorVertical");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_RECTANGLE::MirrorVertical(a0);
	}
	void Rotate(const class wxPoint & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "Rotate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_RECTANGLE::Rotate(a0, a1);
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_RECTANGLE::GetWidth();
	}
	void SetWidth(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "SetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_RECTANGLE::SetWidth(a0);
	}
	class wxString GetSelectMenuText(enum EDA_UNITS a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "GetSelectMenuText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_RECTANGLE::GetSelectMenuText(a0);
	}
	class EDA_ITEM * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class EDA_ITEM *>::value) {
				static pybind11::detail::override_caster_t<class EDA_ITEM *> caster;
				return pybind11::detail::cast_ref<class EDA_ITEM *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class EDA_ITEM *>(std::move(o));
		}
		return LIB_RECTANGLE::Clone();
	}
	bool ContinueEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "ContinueEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "EndEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "Print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "SetPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "compare");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "SetParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "GetFocusPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "Matches");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "Replace");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_RECTANGLE *>(this), "IsReplaceable");
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

// LIB_TEXT file:lib_text.h line:40
struct PyCallBack_LIB_TEXT : public LIB_TEXT {
	using LIB_TEXT::LIB_TEXT;

	class wxString GetClass() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetClass");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_TEXT::GetClass();
	}
	class wxString GetTypeName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetTypeName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_TEXT::GetTypeName();
	}
	bool HitTest(const class wxPoint & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_TEXT::HitTest(a0, a1);
	}
	bool HitTest(const class EDA_RECT & a0, bool a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_TEXT::HitTest(a0, a1, a2);
	}
	int GetPenWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetPenWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_TEXT::GetPenWidth();
	}
	const class EDA_RECT GetBoundingBox() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetBoundingBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class EDA_RECT>::value) {
				static pybind11::detail::override_caster_t<const class EDA_RECT> caster;
				return pybind11::detail::cast_ref<const class EDA_RECT>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class EDA_RECT>(std::move(o));
		}
		return LIB_TEXT::GetBoundingBox();
	}
	void BeginEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "BeginEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_TEXT::BeginEdit(a0);
	}
	void CalcEdit(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "CalcEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_TEXT::CalcEdit(a0);
	}
	void Offset(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "Offset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_TEXT::Offset(a0);
	}
	void MoveTo(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "MoveTo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_TEXT::MoveTo(a0);
	}
	class wxPoint GetPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint>::value) {
				static pybind11::detail::override_caster_t<class wxPoint> caster;
				return pybind11::detail::cast_ref<class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint>(std::move(o));
		}
		return LIB_TEXT::GetPosition();
	}
	void MirrorHorizontal(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "MirrorHorizontal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_TEXT::MirrorHorizontal(a0);
	}
	void MirrorVertical(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "MirrorVertical");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_TEXT::MirrorVertical(a0);
	}
	void Rotate(const class wxPoint & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "Rotate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_TEXT::Rotate(a0, a1);
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_TEXT::GetWidth();
	}
	void SetWidth(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "SetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_TEXT::SetWidth(a0);
	}
	class wxString GetSelectMenuText(enum EDA_UNITS a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetSelectMenuText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_TEXT::GetSelectMenuText(a0);
	}
	class EDA_ITEM * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class EDA_ITEM *>::value) {
				static pybind11::detail::override_caster_t<class EDA_ITEM *> caster;
				return pybind11::detail::cast_ref<class EDA_ITEM *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class EDA_ITEM *>(std::move(o));
		}
		return LIB_TEXT::Clone();
	}
	bool ContinueEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "ContinueEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "EndEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "Print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "SetPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "compare");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "SetParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetFocusPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "Matches");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "Replace");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "IsReplaceable");
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
	const class wxString & GetText() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetText");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetShownText");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "SetText");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "SetTextAngle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "TextHitTest");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "TextHitTest");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "Format");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TEXT *>(this), "GetDrawRotation");
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

void bind_lib_rectangle(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // LIB_RECTANGLE file:lib_rectangle.h line:31
		pybind11::class_<LIB_RECTANGLE, std::shared_ptr<LIB_RECTANGLE>, PyCallBack_LIB_RECTANGLE, LIB_ITEM> cl(M(""), "LIB_RECTANGLE", "");
		cl.def( pybind11::init<class LIB_PART *>(), pybind11::arg("aParent") );

		cl.def( pybind11::init( [](PyCallBack_LIB_RECTANGLE const &o){ return new PyCallBack_LIB_RECTANGLE(o); } ) );
		cl.def( pybind11::init( [](LIB_RECTANGLE const &o){ return new LIB_RECTANGLE(o); } ) );
		cl.def("GetClass", (class wxString (LIB_RECTANGLE::*)() const) &LIB_RECTANGLE::GetClass, "C++: LIB_RECTANGLE::GetClass() const --> class wxString");
		cl.def("GetTypeName", (class wxString (LIB_RECTANGLE::*)() const) &LIB_RECTANGLE::GetTypeName, "C++: LIB_RECTANGLE::GetTypeName() const --> class wxString");
		cl.def("SetEndPosition", (void (LIB_RECTANGLE::*)(const class wxPoint &)) &LIB_RECTANGLE::SetEndPosition, "C++: LIB_RECTANGLE::SetEndPosition(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("HitTest", [](LIB_RECTANGLE const &o, const class wxPoint & a0) -> bool { return o.HitTest(a0); }, "", pybind11::arg("aPosition"));
		cl.def("HitTest", (bool (LIB_RECTANGLE::*)(const class wxPoint &, int) const) &LIB_RECTANGLE::HitTest, "C++: LIB_RECTANGLE::HitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPosition"), pybind11::arg("aAccuracy"));
		cl.def("HitTest", [](LIB_RECTANGLE const &o, const class EDA_RECT & a0, bool const & a1) -> bool { return o.HitTest(a0, a1); }, "", pybind11::arg("aRect"), pybind11::arg("aContained"));
		cl.def("HitTest", (bool (LIB_RECTANGLE::*)(const class EDA_RECT &, bool, int) const) &LIB_RECTANGLE::HitTest, "C++: LIB_RECTANGLE::HitTest(const class EDA_RECT &, bool, int) const --> bool", pybind11::arg("aRect"), pybind11::arg("aContained"), pybind11::arg("aAccuracy"));
		cl.def("GetPenWidth", (int (LIB_RECTANGLE::*)() const) &LIB_RECTANGLE::GetPenWidth, "C++: LIB_RECTANGLE::GetPenWidth() const --> int");
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_RECTANGLE::*)() const) &LIB_RECTANGLE::GetBoundingBox, "C++: LIB_RECTANGLE::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("BeginEdit", (void (LIB_RECTANGLE::*)(const class wxPoint)) &LIB_RECTANGLE::BeginEdit, "C++: LIB_RECTANGLE::BeginEdit(const class wxPoint) --> void", pybind11::arg("aStartPoint"));
		cl.def("CalcEdit", (void (LIB_RECTANGLE::*)(const class wxPoint &)) &LIB_RECTANGLE::CalcEdit, "C++: LIB_RECTANGLE::CalcEdit(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("Offset", (void (LIB_RECTANGLE::*)(const class wxPoint &)) &LIB_RECTANGLE::Offset, "C++: LIB_RECTANGLE::Offset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("MoveTo", (void (LIB_RECTANGLE::*)(const class wxPoint &)) &LIB_RECTANGLE::MoveTo, "C++: LIB_RECTANGLE::MoveTo(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("GetPosition", (class wxPoint (LIB_RECTANGLE::*)() const) &LIB_RECTANGLE::GetPosition, "C++: LIB_RECTANGLE::GetPosition() const --> class wxPoint");
		cl.def("MirrorHorizontal", (void (LIB_RECTANGLE::*)(const class wxPoint &)) &LIB_RECTANGLE::MirrorHorizontal, "C++: LIB_RECTANGLE::MirrorHorizontal(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("MirrorVertical", (void (LIB_RECTANGLE::*)(const class wxPoint &)) &LIB_RECTANGLE::MirrorVertical, "C++: LIB_RECTANGLE::MirrorVertical(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("Rotate", [](LIB_RECTANGLE &o, const class wxPoint & a0) -> void { return o.Rotate(a0); }, "", pybind11::arg("aCenter"));
		cl.def("Rotate", (void (LIB_RECTANGLE::*)(const class wxPoint &, bool)) &LIB_RECTANGLE::Rotate, "C++: LIB_RECTANGLE::Rotate(const class wxPoint &, bool) --> void", pybind11::arg("aCenter"), pybind11::arg("aRotateCCW"));
		cl.def("GetWidth", (int (LIB_RECTANGLE::*)() const) &LIB_RECTANGLE::GetWidth, "C++: LIB_RECTANGLE::GetWidth() const --> int");
		cl.def("SetWidth", (void (LIB_RECTANGLE::*)(int)) &LIB_RECTANGLE::SetWidth, "C++: LIB_RECTANGLE::SetWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("SetEnd", (void (LIB_RECTANGLE::*)(const class wxPoint &)) &LIB_RECTANGLE::SetEnd, "C++: LIB_RECTANGLE::SetEnd(const class wxPoint &) --> void", pybind11::arg("aEnd"));
		cl.def("GetEnd", (class wxPoint (LIB_RECTANGLE::*)() const) &LIB_RECTANGLE::GetEnd, "C++: LIB_RECTANGLE::GetEnd() const --> class wxPoint");
		cl.def("GetSelectMenuText", (class wxString (LIB_RECTANGLE::*)(enum EDA_UNITS) const) &LIB_RECTANGLE::GetSelectMenuText, "C++: LIB_RECTANGLE::GetSelectMenuText(enum EDA_UNITS) const --> class wxString", pybind11::arg("aUnits"));
		cl.def("GetMenuImage", (int (LIB_RECTANGLE::*)() const) &LIB_RECTANGLE::GetMenuImage, "C++: LIB_RECTANGLE::GetMenuImage() const --> int");
		cl.def("Clone", (class EDA_ITEM * (LIB_RECTANGLE::*)() const) &LIB_RECTANGLE::Clone, "C++: LIB_RECTANGLE::Clone() const --> class EDA_ITEM *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class LIB_RECTANGLE & (LIB_RECTANGLE::*)(const class LIB_RECTANGLE &)) &LIB_RECTANGLE::operator=, "C++: LIB_RECTANGLE::operator=(const class LIB_RECTANGLE &) --> class LIB_RECTANGLE &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // LIB_TEXT file:lib_text.h line:40
		pybind11::class_<LIB_TEXT, std::shared_ptr<LIB_TEXT>, PyCallBack_LIB_TEXT, LIB_ITEM, EDA_TEXT> cl(M(""), "LIB_TEXT", "Define a symbol library graphical text item.\n \n This is only a graphical text item.  Field text like the reference designator,\n symbol value, etc. are not LIB_TEXT items.  See the #LIB_FIELD class for the\n field item definition.\n ");
		cl.def( pybind11::init<class LIB_PART *>(), pybind11::arg("aParent") );

		cl.def( pybind11::init( [](PyCallBack_LIB_TEXT const &o){ return new PyCallBack_LIB_TEXT(o); } ) );
		cl.def( pybind11::init( [](LIB_TEXT const &o){ return new LIB_TEXT(o); } ) );
		cl.def("GetClass", (class wxString (LIB_TEXT::*)() const) &LIB_TEXT::GetClass, "C++: LIB_TEXT::GetClass() const --> class wxString");
		cl.def("GetTypeName", (class wxString (LIB_TEXT::*)() const) &LIB_TEXT::GetTypeName, "C++: LIB_TEXT::GetTypeName() const --> class wxString");
		cl.def("HitTest", [](LIB_TEXT const &o, const class wxPoint & a0) -> bool { return o.HitTest(a0); }, "", pybind11::arg("aPosition"));
		cl.def("HitTest", (bool (LIB_TEXT::*)(const class wxPoint &, int) const) &LIB_TEXT::HitTest, "C++: LIB_TEXT::HitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPosition"), pybind11::arg("aAccuracy"));
		cl.def("HitTest", [](LIB_TEXT const &o, const class EDA_RECT & a0, bool const & a1) -> bool { return o.HitTest(a0, a1); }, "", pybind11::arg("aRect"), pybind11::arg("aContained"));
		cl.def("HitTest", (bool (LIB_TEXT::*)(const class EDA_RECT &, bool, int) const) &LIB_TEXT::HitTest, "C++: LIB_TEXT::HitTest(const class EDA_RECT &, bool, int) const --> bool", pybind11::arg("aRect"), pybind11::arg("aContained"), pybind11::arg("aAccuracy"));
		cl.def("GetPenWidth", (int (LIB_TEXT::*)() const) &LIB_TEXT::GetPenWidth, "C++: LIB_TEXT::GetPenWidth() const --> int");
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_TEXT::*)() const) &LIB_TEXT::GetBoundingBox, "C++: LIB_TEXT::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("BeginEdit", (void (LIB_TEXT::*)(const class wxPoint)) &LIB_TEXT::BeginEdit, "C++: LIB_TEXT::BeginEdit(const class wxPoint) --> void", pybind11::arg("aStartPoint"));
		cl.def("CalcEdit", (void (LIB_TEXT::*)(const class wxPoint &)) &LIB_TEXT::CalcEdit, "C++: LIB_TEXT::CalcEdit(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("Offset", (void (LIB_TEXT::*)(const class wxPoint &)) &LIB_TEXT::Offset, "C++: LIB_TEXT::Offset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("MoveTo", (void (LIB_TEXT::*)(const class wxPoint &)) &LIB_TEXT::MoveTo, "C++: LIB_TEXT::MoveTo(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("GetPosition", (class wxPoint (LIB_TEXT::*)() const) &LIB_TEXT::GetPosition, "C++: LIB_TEXT::GetPosition() const --> class wxPoint");
		cl.def("MirrorHorizontal", (void (LIB_TEXT::*)(const class wxPoint &)) &LIB_TEXT::MirrorHorizontal, "C++: LIB_TEXT::MirrorHorizontal(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("MirrorVertical", (void (LIB_TEXT::*)(const class wxPoint &)) &LIB_TEXT::MirrorVertical, "C++: LIB_TEXT::MirrorVertical(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("Rotate", [](LIB_TEXT &o, const class wxPoint & a0) -> void { return o.Rotate(a0); }, "", pybind11::arg("aCenter"));
		cl.def("Rotate", (void (LIB_TEXT::*)(const class wxPoint &, bool)) &LIB_TEXT::Rotate, "C++: LIB_TEXT::Rotate(const class wxPoint &, bool) --> void", pybind11::arg("aCenter"), pybind11::arg("aRotateCCW"));
		cl.def("NormalizeJustification", (void (LIB_TEXT::*)(bool)) &LIB_TEXT::NormalizeJustification, "C++: LIB_TEXT::NormalizeJustification(bool) --> void", pybind11::arg("inverse"));
		cl.def("GetWidth", (int (LIB_TEXT::*)() const) &LIB_TEXT::GetWidth, "C++: LIB_TEXT::GetWidth() const --> int");
		cl.def("SetWidth", (void (LIB_TEXT::*)(int)) &LIB_TEXT::SetWidth, "C++: LIB_TEXT::SetWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("GetSelectMenuText", (class wxString (LIB_TEXT::*)(enum EDA_UNITS) const) &LIB_TEXT::GetSelectMenuText, "C++: LIB_TEXT::GetSelectMenuText(enum EDA_UNITS) const --> class wxString", pybind11::arg("aUnits"));
		cl.def("GetMenuImage", (int (LIB_TEXT::*)() const) &LIB_TEXT::GetMenuImage, "C++: LIB_TEXT::GetMenuImage() const --> int");
		cl.def("Clone", (class EDA_ITEM * (LIB_TEXT::*)() const) &LIB_TEXT::Clone, "C++: LIB_TEXT::Clone() const --> class EDA_ITEM *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class LIB_TEXT & (LIB_TEXT::*)(const class LIB_TEXT &)) &LIB_TEXT::operator=, "C++: LIB_TEXT::operator=(const class LIB_TEXT &) --> class LIB_TEXT &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// TOOL_EVENT_CATEGORY file:tool/tool_event.h line:51
	pybind11::enum_<TOOL_EVENT_CATEGORY>(M(""), "TOOL_EVENT_CATEGORY", pybind11::arithmetic(), "Internal (GUI-independent) event definitions.")
		.value("TC_NONE", TC_NONE)
		.value("TC_MOUSE", TC_MOUSE)
		.value("TC_KEYBOARD", TC_KEYBOARD)
		.value("TC_COMMAND", TC_COMMAND)
		.value("TC_MESSAGE", TC_MESSAGE)
		.value("TC_VIEW", TC_VIEW)
		.value("TC_ANY", TC_ANY)
		.export_values();

;

	// TOOL_ACTIONS file:tool/tool_event.h line:62
	pybind11::enum_<TOOL_ACTIONS>(M(""), "TOOL_ACTIONS", pybind11::arithmetic(), "")
		.value("TA_NONE", TA_NONE)
		.value("TA_MOUSE_CLICK", TA_MOUSE_CLICK)
		.value("TA_MOUSE_DBLCLICK", TA_MOUSE_DBLCLICK)
		.value("TA_MOUSE_UP", TA_MOUSE_UP)
		.value("TA_MOUSE_DOWN", TA_MOUSE_DOWN)
		.value("TA_MOUSE_DRAG", TA_MOUSE_DRAG)
		.value("TA_MOUSE_MOTION", TA_MOUSE_MOTION)
		.value("TA_MOUSE_WHEEL", TA_MOUSE_WHEEL)
		.value("TA_MOUSE", TA_MOUSE)
		.value("TA_KEY_PRESSED", TA_KEY_PRESSED)
		.value("TA_KEYBOARD", TA_KEYBOARD)
		.value("TA_VIEW_REFRESH", TA_VIEW_REFRESH)
		.value("TA_VIEW_ZOOM", TA_VIEW_ZOOM)
		.value("TA_VIEW_PAN", TA_VIEW_PAN)
		.value("TA_VIEW_DIRTY", TA_VIEW_DIRTY)
		.value("TA_VIEW", TA_VIEW)
		.value("TA_CHANGE_LAYER", TA_CHANGE_LAYER)
		.value("TA_CANCEL_TOOL", TA_CANCEL_TOOL)
		.value("TA_CHOICE_MENU_UPDATE", TA_CHOICE_MENU_UPDATE)
		.value("TA_CHOICE_MENU_CHOICE", TA_CHOICE_MENU_CHOICE)
		.value("TA_CHOICE_MENU_CLOSED", TA_CHOICE_MENU_CLOSED)
		.value("TA_CHOICE_MENU", TA_CHOICE_MENU)
		.value("TA_UNDO_REDO_PRE", TA_UNDO_REDO_PRE)
		.value("TA_UNDO_REDO_POST", TA_UNDO_REDO_POST)
		.value("TA_ACTION", TA_ACTION)
		.value("TA_ACTIVATE", TA_ACTIVATE)
		.value("TA_REACTIVATE", TA_REACTIVATE)
		.value("TA_MODEL_CHANGE", TA_MODEL_CHANGE)
		.value("TA_PRIME", TA_PRIME)
		.value("TA_ANY", TA_ANY)
		.export_values();

;

	// TOOL_MOUSE_BUTTONS file:tool/tool_event.h line:128
	pybind11::enum_<TOOL_MOUSE_BUTTONS>(M(""), "TOOL_MOUSE_BUTTONS", pybind11::arithmetic(), "")
		.value("BUT_NONE", BUT_NONE)
		.value("BUT_LEFT", BUT_LEFT)
		.value("BUT_RIGHT", BUT_RIGHT)
		.value("BUT_MIDDLE", BUT_MIDDLE)
		.value("BUT_BUTTON_MASK", BUT_BUTTON_MASK)
		.value("BUT_ANY", BUT_ANY)
		.export_values();

;

	// TOOL_MODIFIERS file:tool/tool_event.h line:138
	pybind11::enum_<TOOL_MODIFIERS>(M(""), "TOOL_MODIFIERS", pybind11::arithmetic(), "")
		.value("MD_SHIFT", MD_SHIFT)
		.value("MD_CTRL", MD_CTRL)
		.value("MD_ALT", MD_ALT)
		.value("MD_MODIFIER_MASK", MD_MODIFIER_MASK)
		.export_values();

;

	// TOOL_ACTION_SCOPE file:tool/tool_event.h line:147
	pybind11::enum_<TOOL_ACTION_SCOPE>(M(""), "TOOL_ACTION_SCOPE", pybind11::arithmetic(), "Scope of tool actions")
		.value("AS_CONTEXT", AS_CONTEXT)
		.value("AS_ACTIVE", AS_ACTIVE)
		.value("AS_GLOBAL", AS_GLOBAL)
		.export_values();

;

	// TOOL_ACTION_FLAGS file:tool/tool_event.h line:155
	pybind11::enum_<TOOL_ACTION_FLAGS>(M(""), "TOOL_ACTION_FLAGS", pybind11::arithmetic(), "Flags for tool actions")
		.value("AF_NONE", AF_NONE)
		.value("AF_ACTIVATE", AF_ACTIVATE)
		.value("AF_NOTIFY", AF_NOTIFY)
		.export_values();

;

	// CONTEXT_MENU_TRIGGER file:tool/tool_event.h line:163
	pybind11::enum_<CONTEXT_MENU_TRIGGER>(M(""), "CONTEXT_MENU_TRIGGER", pybind11::arithmetic(), "Defines when a context menu is opened.")
		.value("CMENU_BUTTON", CMENU_BUTTON)
		.value("CMENU_NOW", CMENU_NOW)
		.value("CMENU_OFF", CMENU_OFF)
		.export_values();

;

	{ // TOOL_EVENT file:tool/tool_event.h line:173
		pybind11::class_<TOOL_EVENT, std::shared_ptr<TOOL_EVENT>> cl(M(""), "TOOL_EVENT", "Generic, UI-independent tool event.");
		cl.def( pybind11::init( [](){ return new TOOL_EVENT(); } ), "doc" );
		cl.def( pybind11::init( [](enum TOOL_EVENT_CATEGORY const & a0){ return new TOOL_EVENT(a0); } ), "doc" , pybind11::arg("aCategory"));
		cl.def( pybind11::init( [](enum TOOL_EVENT_CATEGORY const & a0, enum TOOL_ACTIONS const & a1){ return new TOOL_EVENT(a0, a1); } ), "doc" , pybind11::arg("aCategory"), pybind11::arg("aAction"));
		cl.def( pybind11::init( [](enum TOOL_EVENT_CATEGORY const & a0, enum TOOL_ACTIONS const & a1, enum TOOL_ACTION_SCOPE const & a2){ return new TOOL_EVENT(a0, a1, a2); } ), "doc" , pybind11::arg("aCategory"), pybind11::arg("aAction"), pybind11::arg("aScope"));
		cl.def( pybind11::init<enum TOOL_EVENT_CATEGORY, enum TOOL_ACTIONS, enum TOOL_ACTION_SCOPE, void *>(), pybind11::arg("aCategory"), pybind11::arg("aAction"), pybind11::arg("aScope"), pybind11::arg("aParameter") );

		cl.def( pybind11::init( [](enum TOOL_EVENT_CATEGORY const & a0, enum TOOL_ACTIONS const & a1, int const & a2){ return new TOOL_EVENT(a0, a1, a2); } ), "doc" , pybind11::arg("aCategory"), pybind11::arg("aAction"), pybind11::arg("aExtraParam"));
		cl.def( pybind11::init( [](enum TOOL_EVENT_CATEGORY const & a0, enum TOOL_ACTIONS const & a1, int const & a2, enum TOOL_ACTION_SCOPE const & a3){ return new TOOL_EVENT(a0, a1, a2, a3); } ), "doc" , pybind11::arg("aCategory"), pybind11::arg("aAction"), pybind11::arg("aExtraParam"), pybind11::arg("aScope"));
		cl.def( pybind11::init<enum TOOL_EVENT_CATEGORY, enum TOOL_ACTIONS, int, enum TOOL_ACTION_SCOPE, void *>(), pybind11::arg("aCategory"), pybind11::arg("aAction"), pybind11::arg("aExtraParam"), pybind11::arg("aScope"), pybind11::arg("aParameter") );

		cl.def( pybind11::init( [](TOOL_EVENT const &o){ return new TOOL_EVENT(o); } ) );
		cl.def("Category", (enum TOOL_EVENT_CATEGORY (TOOL_EVENT::*)() const) &TOOL_EVENT::Category, "C++: TOOL_EVENT::Category() const --> enum TOOL_EVENT_CATEGORY");
		cl.def("Action", (enum TOOL_ACTIONS (TOOL_EVENT::*)() const) &TOOL_EVENT::Action, "C++: TOOL_EVENT::Action() const --> enum TOOL_ACTIONS");
		cl.def("PassEvent", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::PassEvent, "C++: TOOL_EVENT::PassEvent() const --> bool");
		cl.def("SetPassEvent", [](TOOL_EVENT &o) -> void { return o.SetPassEvent(); }, "");
		cl.def("SetPassEvent", (void (TOOL_EVENT::*)(bool)) &TOOL_EVENT::SetPassEvent, "C++: TOOL_EVENT::SetPassEvent(bool) --> void", pybind11::arg("aPass"));
		cl.def("HasPosition", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::HasPosition, "C++: TOOL_EVENT::HasPosition() const --> bool");
		cl.def("SetHasPosition", (void (TOOL_EVENT::*)(bool)) &TOOL_EVENT::SetHasPosition, "C++: TOOL_EVENT::SetHasPosition(bool) --> void", pybind11::arg("aHasPosition"));
		cl.def("ForceImmediate", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::ForceImmediate, "C++: TOOL_EVENT::ForceImmediate() const --> bool");
		cl.def("SetForceImmediate", [](TOOL_EVENT &o) -> void { return o.SetForceImmediate(); }, "");
		cl.def("SetForceImmediate", (void (TOOL_EVENT::*)(bool)) &TOOL_EVENT::SetForceImmediate, "C++: TOOL_EVENT::SetForceImmediate(bool) --> void", pybind11::arg("aForceImmediate"));
		cl.def("IsReactivate", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsReactivate, "C++: TOOL_EVENT::IsReactivate() const --> bool");
		cl.def("SetReactivate", [](TOOL_EVENT &o) -> void { return o.SetReactivate(); }, "");
		cl.def("SetReactivate", (void (TOOL_EVENT::*)(bool)) &TOOL_EVENT::SetReactivate, "C++: TOOL_EVENT::SetReactivate(bool) --> void", pybind11::arg("aReactivate"));
		cl.def("Delta", (const int (TOOL_EVENT::*)() const) &TOOL_EVENT::Delta, "C++: TOOL_EVENT::Delta() const --> const int");
		cl.def("Position", (const int (TOOL_EVENT::*)() const) &TOOL_EVENT::Position, "C++: TOOL_EVENT::Position() const --> const int");
		cl.def("DragOrigin", (const int (TOOL_EVENT::*)() const) &TOOL_EVENT::DragOrigin, "C++: TOOL_EVENT::DragOrigin() const --> const int");
		cl.def("Buttons", (int (TOOL_EVENT::*)() const) &TOOL_EVENT::Buttons, "C++: TOOL_EVENT::Buttons() const --> int");
		cl.def("IsClick", [](TOOL_EVENT const &o) -> bool { return o.IsClick(); }, "");
		cl.def("IsClick", (bool (TOOL_EVENT::*)(int) const) &TOOL_EVENT::IsClick, "C++: TOOL_EVENT::IsClick(int) const --> bool", pybind11::arg("aButtonMask"));
		cl.def("IsDblClick", [](TOOL_EVENT const &o) -> bool { return o.IsDblClick(); }, "");
		cl.def("IsDblClick", (bool (TOOL_EVENT::*)(int) const) &TOOL_EVENT::IsDblClick, "C++: TOOL_EVENT::IsDblClick(int) const --> bool", pybind11::arg("aButtonMask"));
		cl.def("IsDrag", [](TOOL_EVENT const &o) -> bool { return o.IsDrag(); }, "");
		cl.def("IsDrag", (bool (TOOL_EVENT::*)(int) const) &TOOL_EVENT::IsDrag, "C++: TOOL_EVENT::IsDrag(int) const --> bool", pybind11::arg("aButtonMask"));
		cl.def("IsMouseUp", [](TOOL_EVENT const &o) -> bool { return o.IsMouseUp(); }, "");
		cl.def("IsMouseUp", (bool (TOOL_EVENT::*)(int) const) &TOOL_EVENT::IsMouseUp, "C++: TOOL_EVENT::IsMouseUp(int) const --> bool", pybind11::arg("aButtonMask"));
		cl.def("IsMotion", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsMotion, "C++: TOOL_EVENT::IsMotion() const --> bool");
		cl.def("IsMouseAction", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsMouseAction, "C++: TOOL_EVENT::IsMouseAction() const --> bool");
		cl.def("IsCancel", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsCancel, "C++: TOOL_EVENT::IsCancel() const --> bool");
		cl.def("IsActivate", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsActivate, "C++: TOOL_EVENT::IsActivate() const --> bool");
		cl.def("IsUndoRedo", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsUndoRedo, "C++: TOOL_EVENT::IsUndoRedo() const --> bool");
		cl.def("IsChoiceMenu", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsChoiceMenu, "C++: TOOL_EVENT::IsChoiceMenu() const --> bool");
		cl.def("IsPrime", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsPrime, "C++: TOOL_EVENT::IsPrime() const --> bool");
		cl.def("Modifier", [](TOOL_EVENT const &o) -> int { return o.Modifier(); }, "");
		cl.def("Modifier", (int (TOOL_EVENT::*)(int) const) &TOOL_EVENT::Modifier, "C++: TOOL_EVENT::Modifier(int) const --> int", pybind11::arg("aMask"));
		cl.def("KeyCode", (int (TOOL_EVENT::*)() const) &TOOL_EVENT::KeyCode, "C++: TOOL_EVENT::KeyCode() const --> int");
		cl.def("IsKeyPressed", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsKeyPressed, "C++: TOOL_EVENT::IsKeyPressed() const --> bool");
		cl.def("Matches", (bool (TOOL_EVENT::*)(const class TOOL_EVENT &) const) &TOOL_EVENT::Matches, "Test whether two events match in terms of category & action or command.\n\n \n is the event to test against.\n \n\n True if two events match, false otherwise.\n\nC++: TOOL_EVENT::Matches(const class TOOL_EVENT &) const --> bool", pybind11::arg("aEvent"));
		cl.def("IsAction", (bool (TOOL_EVENT::*)(const class TOOL_ACTION *) const) &TOOL_EVENT::IsAction, "Test if the event contains an action issued upon activation of the given #TOOL_ACTION.\n\n \n is the TOOL_ACTION to be checked against.\n \n\n True if it matches the given TOOL_ACTION.\n\nC++: TOOL_EVENT::IsAction(const class TOOL_ACTION *) const --> bool", pybind11::arg("aAction"));
		cl.def("IsCancelInteractive", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsCancelInteractive, "Indicate the event should restart/end an ongoing interactive tool's event loop (eg esc\n key, click cancel, start different tool).\n\nC++: TOOL_EVENT::IsCancelInteractive() const --> bool");
		cl.def("IsSelectionEvent", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsSelectionEvent, "Indicate an selection-changed notification event.\n\nC++: TOOL_EVENT::IsSelectionEvent() const --> bool");
		cl.def("IsPointEditor", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsPointEditor, "Indicate if the event is from one of the point editors.\n\n Usually used to allow the point editor to activate itself without de-activating the\n current drawing tool.\n\nC++: TOOL_EVENT::IsPointEditor() const --> bool");
		cl.def("IsMoveTool", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsMoveTool, "Indicate if the event is from one of the move tools.\n\n Usually used to allow move to be done without de-activating the current drawing tool.\n\nC++: TOOL_EVENT::IsMoveTool() const --> bool");
		cl.def("IsSimulator", (bool (TOOL_EVENT::*)() const) &TOOL_EVENT::IsSimulator, "Indicate if the event is from the simulator.\n\nC++: TOOL_EVENT::IsSimulator() const --> bool");
		cl.def("GetCommandId", (int (TOOL_EVENT::*)() const) &TOOL_EVENT::GetCommandId, "C++: TOOL_EVENT::GetCommandId() const --> int");
		cl.def("GetCommandStr", (int (TOOL_EVENT::*)() const) &TOOL_EVENT::GetCommandStr, "C++: TOOL_EVENT::GetCommandStr() const --> int");
		cl.def("SetMousePosition", (void (TOOL_EVENT::*)(const int &)) &TOOL_EVENT::SetMousePosition, "C++: TOOL_EVENT::SetMousePosition(const int &) --> void", pybind11::arg("aP"));
	}
}
