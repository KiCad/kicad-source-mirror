#include <core/typeinfo.h> // KICAD_T
#include <eda_item.h> // EDA_ITEM
#include <eda_rect.h> // EDA_RECT
#include <eda_units.h> // EDA_UNITS
#include <fill_type.h> // FILL_TYPE
#include <gal/color4d.h> // EDA_COLOR_T
#include <gal/color4d.h> // KIGFX::COLOR4D
#include <iterator> // __gnu_cxx::__normal_iterator
#include <layers_id_colors_and_visibility.h> // PCB_LAYER_ID
#include <lib_arc.h> // LIB_ARC
#include <lib_bezier.h> // LIB_BEZIER
#include <lib_field.h> // LIB_FIELD
#include <lib_id.h> // LIB_ID
#include <lib_item.h> // 
#include <lib_item.h> // LIB_ITEM
#include <lib_pin.h> // LIB_PIN
#include <lib_symbol.h> // LIB_PART
#include <lib_symbol.h> // PART_DRAW_OPTIONS
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
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

// LIB_ARC file:lib_arc.h line:35
struct PyCallBack_LIB_ARC : public LIB_ARC {
	using LIB_ARC::LIB_ARC;

	class wxString GetClass() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "GetClass");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_ARC::GetClass();
	}
	class wxString GetTypeName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "GetTypeName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_ARC::GetTypeName();
	}
	bool HitTest(const class wxPoint & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_ARC::HitTest(a0, a1);
	}
	bool HitTest(const class EDA_RECT & a0, bool a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_ARC::HitTest(a0, a1, a2);
	}
	const class EDA_RECT GetBoundingBox() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "GetBoundingBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class EDA_RECT>::value) {
				static pybind11::detail::override_caster_t<const class EDA_RECT> caster;
				return pybind11::detail::cast_ref<const class EDA_RECT>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class EDA_RECT>(std::move(o));
		}
		return LIB_ARC::GetBoundingBox();
	}
	int GetPenWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "GetPenWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_ARC::GetPenWidth();
	}
	void BeginEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "BeginEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ARC::BeginEdit(a0);
	}
	void CalcEdit(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "CalcEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ARC::CalcEdit(a0);
	}
	void Offset(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "Offset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ARC::Offset(a0);
	}
	void MoveTo(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "MoveTo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ARC::MoveTo(a0);
	}
	class wxPoint GetPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "GetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint>::value) {
				static pybind11::detail::override_caster_t<class wxPoint> caster;
				return pybind11::detail::cast_ref<class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint>(std::move(o));
		}
		return LIB_ARC::GetPosition();
	}
	void MirrorHorizontal(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "MirrorHorizontal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ARC::MirrorHorizontal(a0);
	}
	void MirrorVertical(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "MirrorVertical");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ARC::MirrorVertical(a0);
	}
	void Rotate(const class wxPoint & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "Rotate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ARC::Rotate(a0, a1);
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_ARC::GetWidth();
	}
	void SetWidth(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "SetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ARC::SetWidth(a0);
	}
	class wxString GetSelectMenuText(enum EDA_UNITS a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "GetSelectMenuText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_ARC::GetSelectMenuText(a0);
	}
	class EDA_ITEM * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class EDA_ITEM *>::value) {
				static pybind11::detail::override_caster_t<class EDA_ITEM *> caster;
				return pybind11::detail::cast_ref<class EDA_ITEM *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class EDA_ITEM *>(std::move(o));
		}
		return LIB_ARC::Clone();
	}
	bool ContinueEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "ContinueEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "EndEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "Print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "SetPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "compare");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "SetParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "GetFocusPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "Matches");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "Replace");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_ARC *>(this), "IsReplaceable");
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

// LIB_BEZIER file:lib_bezier.h line:34
struct PyCallBack_LIB_BEZIER : public LIB_BEZIER {
	using LIB_BEZIER::LIB_BEZIER;

	class wxString GetClass() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "GetClass");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_BEZIER::GetClass();
	}
	class wxString GetTypeName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "GetTypeName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_BEZIER::GetTypeName();
	}
	void Offset(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "Offset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_BEZIER::Offset(a0);
	}
	bool HitTest(const class wxPoint & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_BEZIER::HitTest(a0, a1);
	}
	bool HitTest(const class EDA_RECT & a0, bool a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_BEZIER::HitTest(a0, a1, a2);
	}
	const class EDA_RECT GetBoundingBox() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "GetBoundingBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class EDA_RECT>::value) {
				static pybind11::detail::override_caster_t<const class EDA_RECT> caster;
				return pybind11::detail::cast_ref<const class EDA_RECT>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class EDA_RECT>(std::move(o));
		}
		return LIB_BEZIER::GetBoundingBox();
	}
	void MoveTo(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "MoveTo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_BEZIER::MoveTo(a0);
	}
	class wxPoint GetPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "GetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint>::value) {
				static pybind11::detail::override_caster_t<class wxPoint> caster;
				return pybind11::detail::cast_ref<class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint>(std::move(o));
		}
		return LIB_BEZIER::GetPosition();
	}
	void MirrorHorizontal(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "MirrorHorizontal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_BEZIER::MirrorHorizontal(a0);
	}
	void MirrorVertical(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "MirrorVertical");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_BEZIER::MirrorVertical(a0);
	}
	void Rotate(const class wxPoint & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "Rotate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_BEZIER::Rotate(a0, a1);
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_BEZIER::GetWidth();
	}
	void SetWidth(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "SetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_BEZIER::SetWidth(a0);
	}
	int GetPenWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "GetPenWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_BEZIER::GetPenWidth();
	}
	class EDA_ITEM * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class EDA_ITEM *>::value) {
				static pybind11::detail::override_caster_t<class EDA_ITEM *> caster;
				return pybind11::detail::cast_ref<class EDA_ITEM *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class EDA_ITEM *>(std::move(o));
		}
		return LIB_BEZIER::Clone();
	}
	void BeginEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "BeginEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "ContinueEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "EndEdit");
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
	void CalcEdit(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "CalcEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_ITEM::CalcEdit(a0);
	}
	void Print(const class KIGFX::RENDER_SETTINGS * a0, const class wxPoint & a1, void * a2, const class TRANSFORM & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "Print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "SetPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "compare");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "SetParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "GetFocusPosition");
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
	class wxString GetSelectMenuText(enum EDA_UNITS a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "GetSelectMenuText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return EDA_ITEM::GetSelectMenuText(a0);
	}
	bool Matches(const class wxFindReplaceData & a0, void * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "Matches");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "Replace");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_BEZIER *>(this), "IsReplaceable");
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

void bind_lib_item(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // LIB_ITEM file:lib_item.h line:62
		pybind11::class_<LIB_ITEM, std::shared_ptr<LIB_ITEM>, EDA_ITEM> cl(M(""), "LIB_ITEM", "The base class for drawable items used by schematic library components.");

		pybind11::enum_<LIB_ITEM::LIB_CONVERT>(cl, "LIB_CONVERT", pybind11::arithmetic(), "")
			.value("BASE", LIB_ITEM::BASE)
			.value("DEMORGAN", LIB_ITEM::DEMORGAN)
			.export_values();


		pybind11::enum_<LIB_ITEM::COMPARE_FLAGS>(cl, "COMPARE_FLAGS", pybind11::arithmetic(), "The list of flags used by the #compare function.\n\n - NORMAL This compares everthing between two #LIB_ITEM objects.\n - UNIT This compare flag ignores unit and convert and pin number information when\n        comparing #LIB_ITEM objects for unit comparison.")
			.value("NORMAL", LIB_ITEM::NORMAL)
			.value("UNIT", LIB_ITEM::UNIT)
			.export_values();

		cl.def("GetTypeName", (class wxString (LIB_ITEM::*)() const) &LIB_ITEM::GetTypeName, "Provide a user-consumable name of the object type.  Perform localization when\n called so that run-time language selection works.\n\nC++: LIB_ITEM::GetTypeName() const --> class wxString");
		cl.def("BeginEdit", (void (LIB_ITEM::*)(const class wxPoint)) &LIB_ITEM::BeginEdit, "Begin drawing a component library draw item at \n\n It typically would be called on a left click when a draw tool is selected in\n the component library editor and one of the graphics tools is selected.\n\n \n The position in drawing coordinates where the drawing was started.\n                  May or may not be required depending on the item being drawn.\n\nC++: LIB_ITEM::BeginEdit(const class wxPoint) --> void", pybind11::arg("aPosition"));
		cl.def("ContinueEdit", (bool (LIB_ITEM::*)(const class wxPoint)) &LIB_ITEM::ContinueEdit, "Continue an edit in progress at \n\n This is used to perform the next action while drawing an item.  This would be\n called for each additional left click when the mouse is captured while the item\n is being drawn.\n\n \n The position of the mouse left click in drawing coordinates.\n \n\n True if additional mouse clicks are required to complete the edit in progress.\n\nC++: LIB_ITEM::ContinueEdit(const class wxPoint) --> bool", pybind11::arg("aPosition"));
		cl.def("EndEdit", (void (LIB_ITEM::*)()) &LIB_ITEM::EndEdit, "End an object editing action.\n\n This is used to end or abort an edit action in progress initiated by BeginEdit().\n\nC++: LIB_ITEM::EndEdit() --> void");
		cl.def("CalcEdit", (void (LIB_ITEM::*)(const class wxPoint &)) &LIB_ITEM::CalcEdit, "Calculates the attributes of an item at  when it is being edited.\n\n This method gets called by the Draw() method when the item is being edited.  This\n probably should be a pure virtual method but bezier curves are not yet editable in\n the component library editor.  Therefore, the default method does nothing.\n\n \n The current mouse position in drawing coordinates.\n\nC++: LIB_ITEM::CalcEdit(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("Print", (void (LIB_ITEM::*)(const class KIGFX::RENDER_SETTINGS *, const class wxPoint &, void *, const class TRANSFORM &)) &LIB_ITEM::Print, "Draw an item\n\n \n Device Context (can be null)\n \n\n Offset to draw\n \n\n Value or pointer used to pass others parameters, depending on body items.\n              Used for some items to force to force no fill mode ( has meaning only for\n              items what can be filled ). used in printing or moving objects mode or to\n              pass reference to the lib component for pins.\n \n\n Transform Matrix (rotation, mirror ..)\n\nC++: LIB_ITEM::Print(const class KIGFX::RENDER_SETTINGS *, const class wxPoint &, void *, const class TRANSFORM &) --> void", pybind11::arg("aSettings"), pybind11::arg("aOffset"), pybind11::arg("aData"), pybind11::arg("aTransform"));
		cl.def("GetPenWidth", (int (LIB_ITEM::*)() const) &LIB_ITEM::GetPenWidth, "C++: LIB_ITEM::GetPenWidth() const --> int");
		cl.def("GetParent", (class LIB_PART * (LIB_ITEM::*)() const) &LIB_ITEM::GetParent, "C++: LIB_ITEM::GetParent() const --> class LIB_PART *", pybind11::return_value_policy::automatic);
		cl.def("HitTest", [](LIB_ITEM const &o, const class wxPoint & a0) -> bool { return o.HitTest(a0); }, "", pybind11::arg("aPosition"));
		cl.def("HitTest", (bool (LIB_ITEM::*)(const class wxPoint &, int) const) &LIB_ITEM::HitTest, "C++: LIB_ITEM::HitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPosition"), pybind11::arg("aAccuracy"));
		cl.def("HitTest", [](LIB_ITEM const &o, const class EDA_RECT & a0, bool const & a1) -> bool { return o.HitTest(a0, a1); }, "", pybind11::arg("aRect"), pybind11::arg("aContained"));
		cl.def("HitTest", (bool (LIB_ITEM::*)(const class EDA_RECT &, bool, int) const) &LIB_ITEM::HitTest, "C++: LIB_ITEM::HitTest(const class EDA_RECT &, bool, int) const --> bool", pybind11::arg("aRect"), pybind11::arg("aContained"), pybind11::arg("aAccuracy"));
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_ITEM::*)() const) &LIB_ITEM::GetBoundingBox, "the boundary box for this, in library coordinates\n\nC++: LIB_ITEM::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("__eq__", (bool (LIB_ITEM::*)(const class LIB_ITEM &) const) &LIB_ITEM::operator==, "Test LIB_ITEM objects for equivalence.\n\n \n Object to test against.\n \n\n True if object is identical to this object.\n\nC++: LIB_ITEM::operator==(const class LIB_ITEM &) const --> bool", pybind11::arg("aOther"));
		cl.def("__eq__", (bool (LIB_ITEM::*)(const class LIB_ITEM *) const) &LIB_ITEM::operator==, "C++: LIB_ITEM::operator==(const class LIB_ITEM *) const --> bool", pybind11::arg("aOther"));
		cl.def("Offset", (void (LIB_ITEM::*)(const class wxPoint &)) &LIB_ITEM::Offset, "Set the drawing object by  from the current position.\n\n \n Coordinates to offset the item position.\n\nC++: LIB_ITEM::Offset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("MoveTo", (void (LIB_ITEM::*)(const class wxPoint &)) &LIB_ITEM::MoveTo, "Move a draw object to \n\n \n Position to move draw item to.\n\nC++: LIB_ITEM::MoveTo(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("SetPosition", (void (LIB_ITEM::*)(const class wxPoint &)) &LIB_ITEM::SetPosition, "C++: LIB_ITEM::SetPosition(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("MirrorHorizontal", (void (LIB_ITEM::*)(const class wxPoint &)) &LIB_ITEM::MirrorHorizontal, "Mirror the draw object along the horizontal (X) axis about  point.\n\n \n Point to mirror around.\n\nC++: LIB_ITEM::MirrorHorizontal(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("MirrorVertical", (void (LIB_ITEM::*)(const class wxPoint &)) &LIB_ITEM::MirrorVertical, "Mirror the draw object along the MirrorVertical (Y) axis about  point.\n\n \n Point to mirror around.\n\nC++: LIB_ITEM::MirrorVertical(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("Rotate", [](LIB_ITEM &o, const class wxPoint & a0) -> void { return o.Rotate(a0); }, "", pybind11::arg("aCenter"));
		cl.def("Rotate", (void (LIB_ITEM::*)(const class wxPoint &, bool)) &LIB_ITEM::Rotate, "Rotate the object about  point.\n\n \n Point to rotate around.\n \n\n True to rotate counter clockwise.  False to rotate clockwise.\n\nC++: LIB_ITEM::Rotate(const class wxPoint &, bool) --> void", pybind11::arg("aCenter"), pybind11::arg("aRotateCCW"));
		cl.def("GetWidth", (int (LIB_ITEM::*)() const) &LIB_ITEM::GetWidth, "C++: LIB_ITEM::GetWidth() const --> int");
		cl.def("SetWidth", (void (LIB_ITEM::*)(int)) &LIB_ITEM::SetWidth, "C++: LIB_ITEM::SetWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("IsFillable", (bool (LIB_ITEM::*)() const) &LIB_ITEM::IsFillable, "Check if draw object can be filled.\n\n The default setting is false.  If the derived object support filling, set the\n m_isFillable member to true.\n\nC++: LIB_ITEM::IsFillable() const --> bool");
		cl.def("SetUnit", (void (LIB_ITEM::*)(int)) &LIB_ITEM::SetUnit, "C++: LIB_ITEM::SetUnit(int) --> void", pybind11::arg("aUnit"));
		cl.def("GetUnit", (int (LIB_ITEM::*)() const) &LIB_ITEM::GetUnit, "C++: LIB_ITEM::GetUnit() const --> int");
		cl.def("SetConvert", (void (LIB_ITEM::*)(int)) &LIB_ITEM::SetConvert, "C++: LIB_ITEM::SetConvert(int) --> void", pybind11::arg("aConvert"));
		cl.def("GetConvert", (int (LIB_ITEM::*)() const) &LIB_ITEM::GetConvert, "C++: LIB_ITEM::GetConvert() const --> int");
		cl.def("SetFillMode", (void (LIB_ITEM::*)(enum FILL_TYPE)) &LIB_ITEM::SetFillMode, "C++: LIB_ITEM::SetFillMode(enum FILL_TYPE) --> void", pybind11::arg("aFillMode"));
		cl.def("GetFillMode", (enum FILL_TYPE (LIB_ITEM::*)() const) &LIB_ITEM::GetFillMode, "C++: LIB_ITEM::GetFillMode() const --> enum FILL_TYPE");
		cl.def("assign", (class LIB_ITEM & (LIB_ITEM::*)(const class LIB_ITEM &)) &LIB_ITEM::operator=, "C++: LIB_ITEM::operator=(const class LIB_ITEM &) --> class LIB_ITEM &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // LIB_ARC file:lib_arc.h line:35
		pybind11::class_<LIB_ARC, std::shared_ptr<LIB_ARC>, PyCallBack_LIB_ARC, LIB_ITEM> cl(M(""), "LIB_ARC", "");
		cl.def( pybind11::init<class LIB_PART *>(), pybind11::arg("aParent") );

		cl.def( pybind11::init( [](PyCallBack_LIB_ARC const &o){ return new PyCallBack_LIB_ARC(o); } ) );
		cl.def( pybind11::init( [](LIB_ARC const &o){ return new LIB_ARC(o); } ) );
		cl.def("GetClass", (class wxString (LIB_ARC::*)() const) &LIB_ARC::GetClass, "C++: LIB_ARC::GetClass() const --> class wxString");
		cl.def("GetTypeName", (class wxString (LIB_ARC::*)() const) &LIB_ARC::GetTypeName, "C++: LIB_ARC::GetTypeName() const --> class wxString");
		cl.def("HitTest", [](LIB_ARC const &o, const class wxPoint & a0) -> bool { return o.HitTest(a0); }, "", pybind11::arg("aPosition"));
		cl.def("HitTest", (bool (LIB_ARC::*)(const class wxPoint &, int) const) &LIB_ARC::HitTest, "C++: LIB_ARC::HitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPosition"), pybind11::arg("aAccuracy"));
		cl.def("HitTest", [](LIB_ARC const &o, const class EDA_RECT & a0, bool const & a1) -> bool { return o.HitTest(a0, a1); }, "", pybind11::arg("aRect"), pybind11::arg("aContained"));
		cl.def("HitTest", (bool (LIB_ARC::*)(const class EDA_RECT &, bool, int) const) &LIB_ARC::HitTest, "C++: LIB_ARC::HitTest(const class EDA_RECT &, bool, int) const --> bool", pybind11::arg("aRect"), pybind11::arg("aContained"), pybind11::arg("aAccuracy"));
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_ARC::*)() const) &LIB_ARC::GetBoundingBox, "C++: LIB_ARC::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("GetPenWidth", (int (LIB_ARC::*)() const) &LIB_ARC::GetPenWidth, "C++: LIB_ARC::GetPenWidth() const --> int");
		cl.def("BeginEdit", (void (LIB_ARC::*)(const class wxPoint)) &LIB_ARC::BeginEdit, "C++: LIB_ARC::BeginEdit(const class wxPoint) --> void", pybind11::arg("aStartPoint"));
		cl.def("CalcEdit", (void (LIB_ARC::*)(const class wxPoint &)) &LIB_ARC::CalcEdit, "C++: LIB_ARC::CalcEdit(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("SetEditState", (void (LIB_ARC::*)(int)) &LIB_ARC::SetEditState, "C++: LIB_ARC::SetEditState(int) --> void", pybind11::arg("aState"));
		cl.def("Offset", (void (LIB_ARC::*)(const class wxPoint &)) &LIB_ARC::Offset, "C++: LIB_ARC::Offset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("MoveTo", (void (LIB_ARC::*)(const class wxPoint &)) &LIB_ARC::MoveTo, "C++: LIB_ARC::MoveTo(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("GetPosition", (class wxPoint (LIB_ARC::*)() const) &LIB_ARC::GetPosition, "C++: LIB_ARC::GetPosition() const --> class wxPoint");
		cl.def("MirrorHorizontal", (void (LIB_ARC::*)(const class wxPoint &)) &LIB_ARC::MirrorHorizontal, "C++: LIB_ARC::MirrorHorizontal(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("MirrorVertical", (void (LIB_ARC::*)(const class wxPoint &)) &LIB_ARC::MirrorVertical, "C++: LIB_ARC::MirrorVertical(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("Rotate", [](LIB_ARC &o, const class wxPoint & a0) -> void { return o.Rotate(a0); }, "", pybind11::arg("aCenter"));
		cl.def("Rotate", (void (LIB_ARC::*)(const class wxPoint &, bool)) &LIB_ARC::Rotate, "C++: LIB_ARC::Rotate(const class wxPoint &, bool) --> void", pybind11::arg("aCenter"), pybind11::arg("aRotateCCW"));
		cl.def("GetWidth", (int (LIB_ARC::*)() const) &LIB_ARC::GetWidth, "C++: LIB_ARC::GetWidth() const --> int");
		cl.def("SetWidth", (void (LIB_ARC::*)(int)) &LIB_ARC::SetWidth, "C++: LIB_ARC::SetWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("SetRadius", (void (LIB_ARC::*)(int)) &LIB_ARC::SetRadius, "C++: LIB_ARC::SetRadius(int) --> void", pybind11::arg("aRadius"));
		cl.def("GetRadius", (int (LIB_ARC::*)() const) &LIB_ARC::GetRadius, "C++: LIB_ARC::GetRadius() const --> int");
		cl.def("SetFirstRadiusAngle", (void (LIB_ARC::*)(int)) &LIB_ARC::SetFirstRadiusAngle, "C++: LIB_ARC::SetFirstRadiusAngle(int) --> void", pybind11::arg("aAngle"));
		cl.def("GetFirstRadiusAngle", (int (LIB_ARC::*)() const) &LIB_ARC::GetFirstRadiusAngle, "C++: LIB_ARC::GetFirstRadiusAngle() const --> int");
		cl.def("SetSecondRadiusAngle", (void (LIB_ARC::*)(int)) &LIB_ARC::SetSecondRadiusAngle, "C++: LIB_ARC::SetSecondRadiusAngle(int) --> void", pybind11::arg("aAngle"));
		cl.def("GetSecondRadiusAngle", (int (LIB_ARC::*)() const) &LIB_ARC::GetSecondRadiusAngle, "C++: LIB_ARC::GetSecondRadiusAngle() const --> int");
		cl.def("GetStart", (class wxPoint (LIB_ARC::*)() const) &LIB_ARC::GetStart, "C++: LIB_ARC::GetStart() const --> class wxPoint");
		cl.def("SetStart", (void (LIB_ARC::*)(const class wxPoint &)) &LIB_ARC::SetStart, "C++: LIB_ARC::SetStart(const class wxPoint &) --> void", pybind11::arg("aPoint"));
		cl.def("GetEnd", (class wxPoint (LIB_ARC::*)() const) &LIB_ARC::GetEnd, "C++: LIB_ARC::GetEnd() const --> class wxPoint");
		cl.def("SetEnd", (void (LIB_ARC::*)(const class wxPoint &)) &LIB_ARC::SetEnd, "C++: LIB_ARC::SetEnd(const class wxPoint &) --> void", pybind11::arg("aPoint"));
		cl.def("CalcMidPoint", (int (LIB_ARC::*)() const) &LIB_ARC::CalcMidPoint, "Calculate the arc mid point using the arc start and end angles and radius length.\n\n \n This is not a general purpose trigonometric arc midpoint calculation.  It is\n       limited to the less than 180.0 degree arcs used in symbols.\n\n \n A #VECTOR2I object containing the midpoint of the arc.\n\nC++: LIB_ARC::CalcMidPoint() const --> int");
		cl.def("CalcRadiusAngles", (void (LIB_ARC::*)()) &LIB_ARC::CalcRadiusAngles, "Calculate the radius and angle of an arc using the start, end, and center points.\n\nC++: LIB_ARC::CalcRadiusAngles() --> void");
		cl.def("GetSelectMenuText", (class wxString (LIB_ARC::*)(enum EDA_UNITS) const) &LIB_ARC::GetSelectMenuText, "C++: LIB_ARC::GetSelectMenuText(enum EDA_UNITS) const --> class wxString", pybind11::arg("aUnits"));
		cl.def("GetMenuImage", (int (LIB_ARC::*)() const) &LIB_ARC::GetMenuImage, "C++: LIB_ARC::GetMenuImage() const --> int");
		cl.def("Clone", (class EDA_ITEM * (LIB_ARC::*)() const) &LIB_ARC::Clone, "C++: LIB_ARC::Clone() const --> class EDA_ITEM *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class LIB_ARC & (LIB_ARC::*)(const class LIB_ARC &)) &LIB_ARC::operator=, "C++: LIB_ARC::operator=(const class LIB_ARC &) --> class LIB_ARC &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // LIB_BEZIER file:lib_bezier.h line:34
		pybind11::class_<LIB_BEZIER, std::shared_ptr<LIB_BEZIER>, PyCallBack_LIB_BEZIER, LIB_ITEM> cl(M(""), "LIB_BEZIER", "Define a bezier curve graphic body item.");
		cl.def( pybind11::init<class LIB_PART *>(), pybind11::arg("aParent") );

		cl.def( pybind11::init( [](PyCallBack_LIB_BEZIER const &o){ return new PyCallBack_LIB_BEZIER(o); } ) );
		cl.def( pybind11::init( [](LIB_BEZIER const &o){ return new LIB_BEZIER(o); } ) );
		cl.def("GetClass", (class wxString (LIB_BEZIER::*)() const) &LIB_BEZIER::GetClass, "C++: LIB_BEZIER::GetClass() const --> class wxString");
		cl.def("GetTypeName", (class wxString (LIB_BEZIER::*)() const) &LIB_BEZIER::GetTypeName, "C++: LIB_BEZIER::GetTypeName() const --> class wxString");
		cl.def("Reserve", (void (LIB_BEZIER::*)(unsigned long)) &LIB_BEZIER::Reserve, "C++: LIB_BEZIER::Reserve(unsigned long) --> void", pybind11::arg("aCount"));
		cl.def("AddPoint", (void (LIB_BEZIER::*)(const class wxPoint &)) &LIB_BEZIER::AddPoint, "C++: LIB_BEZIER::AddPoint(const class wxPoint &) --> void", pybind11::arg("aPoint"));
		cl.def("Offset", (void (LIB_BEZIER::*)(const class wxPoint &)) &LIB_BEZIER::Offset, "C++: LIB_BEZIER::Offset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("GetOffset", (const class wxPoint (LIB_BEZIER::*)() const) &LIB_BEZIER::GetOffset, "C++: LIB_BEZIER::GetOffset() const --> const class wxPoint");
		cl.def("GetCornerCount", (unsigned int (LIB_BEZIER::*)() const) &LIB_BEZIER::GetCornerCount, "the number of corners\n\nC++: LIB_BEZIER::GetCornerCount() const --> unsigned int");
		cl.def("GetPoints", (const int & (LIB_BEZIER::*)() const) &LIB_BEZIER::GetPoints, "C++: LIB_BEZIER::GetPoints() const --> const int &", pybind11::return_value_policy::automatic);
		cl.def("HitTest", [](LIB_BEZIER const &o, const class wxPoint & a0) -> bool { return o.HitTest(a0); }, "", pybind11::arg("aPosition"));
		cl.def("HitTest", (bool (LIB_BEZIER::*)(const class wxPoint &, int) const) &LIB_BEZIER::HitTest, "C++: LIB_BEZIER::HitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPosition"), pybind11::arg("aAccuracy"));
		cl.def("HitTest", [](LIB_BEZIER const &o, const class EDA_RECT & a0, bool const & a1) -> bool { return o.HitTest(a0, a1); }, "", pybind11::arg("aRect"), pybind11::arg("aContained"));
		cl.def("HitTest", (bool (LIB_BEZIER::*)(const class EDA_RECT &, bool, int) const) &LIB_BEZIER::HitTest, "C++: LIB_BEZIER::HitTest(const class EDA_RECT &, bool, int) const --> bool", pybind11::arg("aRect"), pybind11::arg("aContained"), pybind11::arg("aAccuracy"));
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_BEZIER::*)() const) &LIB_BEZIER::GetBoundingBox, "C++: LIB_BEZIER::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("MoveTo", (void (LIB_BEZIER::*)(const class wxPoint &)) &LIB_BEZIER::MoveTo, "C++: LIB_BEZIER::MoveTo(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("GetPosition", (class wxPoint (LIB_BEZIER::*)() const) &LIB_BEZIER::GetPosition, "C++: LIB_BEZIER::GetPosition() const --> class wxPoint");
		cl.def("MirrorHorizontal", (void (LIB_BEZIER::*)(const class wxPoint &)) &LIB_BEZIER::MirrorHorizontal, "C++: LIB_BEZIER::MirrorHorizontal(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("MirrorVertical", (void (LIB_BEZIER::*)(const class wxPoint &)) &LIB_BEZIER::MirrorVertical, "C++: LIB_BEZIER::MirrorVertical(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("Rotate", [](LIB_BEZIER &o, const class wxPoint & a0) -> void { return o.Rotate(a0); }, "", pybind11::arg("aCenter"));
		cl.def("Rotate", (void (LIB_BEZIER::*)(const class wxPoint &, bool)) &LIB_BEZIER::Rotate, "C++: LIB_BEZIER::Rotate(const class wxPoint &, bool) --> void", pybind11::arg("aCenter"), pybind11::arg("aRotateCCW"));
		cl.def("GetWidth", (int (LIB_BEZIER::*)() const) &LIB_BEZIER::GetWidth, "C++: LIB_BEZIER::GetWidth() const --> int");
		cl.def("SetWidth", (void (LIB_BEZIER::*)(int)) &LIB_BEZIER::SetWidth, "C++: LIB_BEZIER::SetWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("GetPenWidth", (int (LIB_BEZIER::*)() const) &LIB_BEZIER::GetPenWidth, "C++: LIB_BEZIER::GetPenWidth() const --> int");
		cl.def("Clone", (class EDA_ITEM * (LIB_BEZIER::*)() const) &LIB_BEZIER::Clone, "C++: LIB_BEZIER::Clone() const --> class EDA_ITEM *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class LIB_BEZIER & (LIB_BEZIER::*)(const class LIB_BEZIER &)) &LIB_BEZIER::operator=, "C++: LIB_BEZIER::operator=(const class LIB_BEZIER &) --> class LIB_BEZIER &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
