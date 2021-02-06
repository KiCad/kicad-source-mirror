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
#include <layers_id_colors_and_visibility.h> // SCH_LAYER_ID
#include <lib_field.h> // LIB_FIELD
#include <lib_id.h> // LIB_ID
#include <lib_item.h> // 
#include <lib_item.h> // LIB_ITEM
#include <lib_pin.h> // LIB_PIN
#include <lib_symbol.h> // LIB_PART
#include <lib_symbol.h> // PART_DRAW_OPTIONS
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
#include <pin_type.h> // ELECTRICAL_PINTYPE
#include <pin_type.h> // ElectricalPinTypeGetBitmap
#include <pin_type.h> // ElectricalPinTypeGetText
#include <pin_type.h> // GRAPHIC_PINSHAPE
#include <pin_type.h> // PinOrientationCode
#include <pin_type.h> // PinOrientationIcons
#include <pin_type.h> // PinOrientationIndex
#include <pin_type.h> // PinOrientationName
#include <pin_type.h> // PinOrientationNames
#include <pin_type.h> // PinShapeGetBitmap
#include <pin_type.h> // PinShapeGetText
#include <pin_type.h> // PinShapeIcons
#include <pin_type.h> // PinShapeNames
#include <pin_type.h> // PinTypeIcons
#include <pin_type.h> // PinTypeNames
#include <render_settings.h> // KIGFX::RENDER_SETTINGS
#include <richio.h> // OUTPUTFORMATTER
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <transform.h> // TRANSFORM
#include <utf8.h> // IsUTF8
#include <utf8.h> // UTF8
#include <utf8.h> // UTF8::uni_iter
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

// LIB_FIELD file:lib_field.h line:59
struct PyCallBack_LIB_FIELD : public LIB_FIELD {
	using LIB_FIELD::LIB_FIELD;

	class wxString GetClass() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetClass");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_FIELD::GetClass();
	}
	class wxString GetTypeName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetTypeName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_FIELD::GetTypeName();
	}
	int GetPenWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetPenWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_FIELD::GetPenWidth();
	}
	const class EDA_RECT GetBoundingBox() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetBoundingBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class EDA_RECT>::value) {
				static pybind11::detail::override_caster_t<const class EDA_RECT> caster;
				return pybind11::detail::cast_ref<const class EDA_RECT>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class EDA_RECT>(std::move(o));
		}
		return LIB_FIELD::GetBoundingBox();
	}
	bool HitTest(const class wxPoint & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_FIELD::HitTest(a0, a1);
	}
	void BeginEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "BeginEdit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_FIELD::BeginEdit(a0);
	}
	void Offset(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "Offset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_FIELD::Offset(a0);
	}
	void MoveTo(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "MoveTo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_FIELD::MoveTo(a0);
	}
	class wxPoint GetPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint>::value) {
				static pybind11::detail::override_caster_t<class wxPoint> caster;
				return pybind11::detail::cast_ref<class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint>(std::move(o));
		}
		return LIB_FIELD::GetPosition();
	}
	void MirrorHorizontal(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "MirrorHorizontal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_FIELD::MirrorHorizontal(a0);
	}
	void MirrorVertical(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "MirrorVertical");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_FIELD::MirrorVertical(a0);
	}
	void Rotate(const class wxPoint & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "Rotate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_FIELD::Rotate(a0, a1);
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_FIELD::GetWidth();
	}
	void SetWidth(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "SetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_FIELD::SetWidth(a0);
	}
	class wxString GetSelectMenuText(enum EDA_UNITS a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetSelectMenuText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_FIELD::GetSelectMenuText(a0);
	}
	class EDA_ITEM * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class EDA_ITEM *>::value) {
				static pybind11::detail::override_caster_t<class EDA_ITEM *> caster;
				return pybind11::detail::cast_ref<class EDA_ITEM *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class EDA_ITEM *>(std::move(o));
		}
		return LIB_FIELD::Clone();
	}
	bool ContinueEdit(const class wxPoint a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "ContinueEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "EndEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "CalcEdit");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "Print");
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
	bool HitTest(const class EDA_RECT & a0, bool a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_ITEM::HitTest(a0, a1, a2);
	}
	void SetPosition(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "SetPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "compare");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "print");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "SetParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetFocusPosition");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "Matches");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "Replace");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "IsReplaceable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetText");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetShownText");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "SetText");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "SetTextAngle");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "TextHitTest");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "TextHitTest");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "Format");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_FIELD *>(this), "GetDrawRotation");
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

void bind_lib_field(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // LIB_FIELD file:lib_field.h line:59
		pybind11::class_<LIB_FIELD, std::shared_ptr<LIB_FIELD>, PyCallBack_LIB_FIELD, LIB_ITEM, EDA_TEXT> cl(M(""), "LIB_FIELD", "Field object used in symbol libraries.  At least MANDATORY_FIELDS are always present\n in a ram resident library symbol.  All constructors must ensure this because\n the component property editor assumes it.\n \n A field is a string linked to a component.  Unlike purely graphical text, fields can\n be used in netlist generation and other tools (BOM).\n\n  The first 4 fields have a special meaning:\n\n  0 = REFERENCE_FIELD\n  1 = VALUE_FIELD\n  2 = FOOTPRINT_FIELD (default Footprint)\n  3 = DATASHEET_FIELD (user doc link)\n\n  others = free fields\n \n\n \n enum NumFieldType");
		cl.def( pybind11::init( [](){ return new LIB_FIELD(); }, [](){ return new PyCallBack_LIB_FIELD(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("idfield") );

		cl.def( pybind11::init<int, const class wxString &>(), pybind11::arg("aID"), pybind11::arg("aName") );

		cl.def( pybind11::init( [](class LIB_PART * a0){ return new LIB_FIELD(a0); }, [](class LIB_PART * a0){ return new PyCallBack_LIB_FIELD(a0); } ), "doc");
		cl.def( pybind11::init<class LIB_PART *, int>(), pybind11::arg("aParent"), pybind11::arg("idfield") );

		cl.def( pybind11::init( [](PyCallBack_LIB_FIELD const &o){ return new PyCallBack_LIB_FIELD(o); } ) );
		cl.def( pybind11::init( [](LIB_FIELD const &o){ return new LIB_FIELD(o); } ) );
		cl.def("GetClass", (class wxString (LIB_FIELD::*)() const) &LIB_FIELD::GetClass, "C++: LIB_FIELD::GetClass() const --> class wxString");
		cl.def("GetTypeName", (class wxString (LIB_FIELD::*)() const) &LIB_FIELD::GetTypeName, "C++: LIB_FIELD::GetTypeName() const --> class wxString");
		cl.def("Init", (void (LIB_FIELD::*)(int)) &LIB_FIELD::Init, "Object constructor initialization helper.\n\nC++: LIB_FIELD::Init(int) --> void", pybind11::arg("idfield"));
		cl.def("GetName", [](LIB_FIELD const &o) -> wxString { return o.GetName(); }, "");
		cl.def("GetName", (class wxString (LIB_FIELD::*)(bool) const) &LIB_FIELD::GetName, "Returns the field name.\n\n The first four field IDs are reserved and therefore always return their respective\n names.\n\n The user definable fields will return FieldN where N is the ID of the field when the\n m_name member is empyt unless false is passed to \n     \n\nC++: LIB_FIELD::GetName(bool) const --> class wxString", pybind11::arg("aUseDefaultName"));
		cl.def("GetCanonicalName", (class wxString (LIB_FIELD::*)() const) &LIB_FIELD::GetCanonicalName, "Get a non-language-specific name for a field which can be used for storage, variable\n look-up, etc.\n\nC++: LIB_FIELD::GetCanonicalName() const --> class wxString");
		cl.def("SetName", (void (LIB_FIELD::*)(const class wxString &)) &LIB_FIELD::SetName, "Set a user definable field name to \n\n Reserved fields such as value and reference are not renamed.  If the field name is\n changed, the field modified flag is set.  If the field is the child of a component,\n the parent component's modified flag is also set.\n\n \n - User defined field name.\n\nC++: LIB_FIELD::SetName(const class wxString &) --> void", pybind11::arg("aName"));
		cl.def("GetId", (int (LIB_FIELD::*)() const) &LIB_FIELD::GetId, "C++: LIB_FIELD::GetId() const --> int");
		cl.def("SetId", (void (LIB_FIELD::*)(int)) &LIB_FIELD::SetId, "C++: LIB_FIELD::SetId(int) --> void", pybind11::arg("aId"));
		cl.def("GetPenWidth", (int (LIB_FIELD::*)() const) &LIB_FIELD::GetPenWidth, "C++: LIB_FIELD::GetPenWidth() const --> int");
		cl.def("Copy", (void (LIB_FIELD::*)(class LIB_FIELD *) const) &LIB_FIELD::Copy, "Copy parameters of this field to another field. Pointers are not copied.\n\n \n Target field to copy values to.\n\nC++: LIB_FIELD::Copy(class LIB_FIELD *) const --> void", pybind11::arg("aTarget"));
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_FIELD::*)() const) &LIB_FIELD::GetBoundingBox, "C++: LIB_FIELD::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("HitTest", [](LIB_FIELD const &o, const class wxPoint & a0) -> bool { return o.HitTest(a0); }, "", pybind11::arg("aPosition"));
		cl.def("HitTest", (bool (LIB_FIELD::*)(const class wxPoint &, int) const) &LIB_FIELD::HitTest, "C++: LIB_FIELD::HitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPosition"), pybind11::arg("aAccuracy"));
		cl.def("assign", (class LIB_FIELD & (LIB_FIELD::*)(const class LIB_FIELD &)) &LIB_FIELD::operator=, "C++: LIB_FIELD::operator=(const class LIB_FIELD &) --> class LIB_FIELD &", pybind11::return_value_policy::automatic, pybind11::arg("field"));
		cl.def("GetFullText", [](LIB_FIELD const &o) -> wxString { return o.GetFullText(); }, "");
		cl.def("GetFullText", (class wxString (LIB_FIELD::*)(int) const) &LIB_FIELD::GetFullText, "Return the text of a field.\n\n If the field is the reference field, the unit number is used to\n create a pseudo reference text.  If the base reference field is U,\n the string U?A will be returned for unit = 1.\n\n \n - The package unit number.  Only effects reference field.\n \n\n Field text.\n\nC++: LIB_FIELD::GetFullText(int) const --> class wxString", pybind11::arg("unit"));
		cl.def("GetDefaultLayer", (enum SCH_LAYER_ID (LIB_FIELD::*)()) &LIB_FIELD::GetDefaultLayer, "C++: LIB_FIELD::GetDefaultLayer() --> enum SCH_LAYER_ID");
		cl.def("BeginEdit", (void (LIB_FIELD::*)(const class wxPoint)) &LIB_FIELD::BeginEdit, "C++: LIB_FIELD::BeginEdit(const class wxPoint) --> void", pybind11::arg("aStartPoint"));
		cl.def("Offset", (void (LIB_FIELD::*)(const class wxPoint &)) &LIB_FIELD::Offset, "C++: LIB_FIELD::Offset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("MoveTo", (void (LIB_FIELD::*)(const class wxPoint &)) &LIB_FIELD::MoveTo, "C++: LIB_FIELD::MoveTo(const class wxPoint &) --> void", pybind11::arg("aPosition"));
		cl.def("GetPosition", (class wxPoint (LIB_FIELD::*)() const) &LIB_FIELD::GetPosition, "C++: LIB_FIELD::GetPosition() const --> class wxPoint");
		cl.def("MirrorHorizontal", (void (LIB_FIELD::*)(const class wxPoint &)) &LIB_FIELD::MirrorHorizontal, "C++: LIB_FIELD::MirrorHorizontal(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("MirrorVertical", (void (LIB_FIELD::*)(const class wxPoint &)) &LIB_FIELD::MirrorVertical, "C++: LIB_FIELD::MirrorVertical(const class wxPoint &) --> void", pybind11::arg("aCenter"));
		cl.def("Rotate", [](LIB_FIELD &o, const class wxPoint & a0) -> void { return o.Rotate(a0); }, "", pybind11::arg("aCenter"));
		cl.def("Rotate", (void (LIB_FIELD::*)(const class wxPoint &, bool)) &LIB_FIELD::Rotate, "C++: LIB_FIELD::Rotate(const class wxPoint &, bool) --> void", pybind11::arg("aCenter"), pybind11::arg("aRotateCCW"));
		cl.def("GetWidth", (int (LIB_FIELD::*)() const) &LIB_FIELD::GetWidth, "C++: LIB_FIELD::GetWidth() const --> int");
		cl.def("SetWidth", (void (LIB_FIELD::*)(int)) &LIB_FIELD::SetWidth, "C++: LIB_FIELD::SetWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("GetSelectMenuText", (class wxString (LIB_FIELD::*)(enum EDA_UNITS) const) &LIB_FIELD::GetSelectMenuText, "C++: LIB_FIELD::GetSelectMenuText(enum EDA_UNITS) const --> class wxString", pybind11::arg("aUnits"));
		cl.def("GetMenuImage", (int (LIB_FIELD::*)() const) &LIB_FIELD::GetMenuImage, "C++: LIB_FIELD::GetMenuImage() const --> int");
		cl.def("Clone", (class EDA_ITEM * (LIB_FIELD::*)() const) &LIB_FIELD::Clone, "C++: LIB_FIELD::Clone() const --> class EDA_ITEM *", pybind11::return_value_policy::automatic);
		cl.def("IsMandatory", (bool (LIB_FIELD::*)() const) &LIB_FIELD::IsMandatory, "C++: LIB_FIELD::IsMandatory() const --> bool");
	}
	// ELECTRICAL_PINTYPE file:pin_type.h line:34
	pybind11::enum_<ELECTRICAL_PINTYPE>(M(""), "ELECTRICAL_PINTYPE", "The component library pin object electrical types used in ERC tests.")
		.value("PT_INPUT", ELECTRICAL_PINTYPE::PT_INPUT)
		.value("PT_OUTPUT", ELECTRICAL_PINTYPE::PT_OUTPUT)
		.value("PT_BIDI", ELECTRICAL_PINTYPE::PT_BIDI)
		.value("PT_TRISTATE", ELECTRICAL_PINTYPE::PT_TRISTATE)
		.value("PT_PASSIVE", ELECTRICAL_PINTYPE::PT_PASSIVE)
		.value("PT_NIC", ELECTRICAL_PINTYPE::PT_NIC)
		.value("PT_UNSPECIFIED", ELECTRICAL_PINTYPE::PT_UNSPECIFIED)
		.value("PT_POWER_IN", ELECTRICAL_PINTYPE::PT_POWER_IN)
		.value("PT_POWER_OUT", ELECTRICAL_PINTYPE::PT_POWER_OUT)
		.value("PT_OPENCOLLECTOR", ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR)
		.value("PT_OPENEMITTER", ELECTRICAL_PINTYPE::PT_OPENEMITTER)
		.value("PT_NC", ELECTRICAL_PINTYPE::PT_NC)
		.value("PT_LAST_OPTION", ELECTRICAL_PINTYPE::PT_LAST_OPTION);

;

	// GRAPHIC_PINSHAPE file:pin_type.h line:54
	pybind11::enum_<GRAPHIC_PINSHAPE>(M(""), "GRAPHIC_PINSHAPE", "")
		.value("LINE", GRAPHIC_PINSHAPE::LINE)
		.value("INVERTED", GRAPHIC_PINSHAPE::INVERTED)
		.value("CLOCK", GRAPHIC_PINSHAPE::CLOCK)
		.value("INVERTED_CLOCK", GRAPHIC_PINSHAPE::INVERTED_CLOCK)
		.value("INPUT_LOW", GRAPHIC_PINSHAPE::INPUT_LOW)
		.value("CLOCK_LOW", GRAPHIC_PINSHAPE::CLOCK_LOW)
		.value("OUTPUT_LOW", GRAPHIC_PINSHAPE::OUTPUT_LOW)
		.value("FALLING_EDGE_CLOCK", GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK)
		.value("NONLOGIC", GRAPHIC_PINSHAPE::NONLOGIC)
		.value("LAST_OPTION", GRAPHIC_PINSHAPE::LAST_OPTION);

;

	// PinShapeGetText(enum GRAPHIC_PINSHAPE) file:pin_type.h line:74
	M("").def("PinShapeGetText", (class wxString (*)(enum GRAPHIC_PINSHAPE)) &PinShapeGetText, "C++: PinShapeGetText(enum GRAPHIC_PINSHAPE) --> class wxString", pybind11::arg("shape"));

	// PinShapeGetBitmap(enum GRAPHIC_PINSHAPE) file:pin_type.h line:75
	M("").def("PinShapeGetBitmap", (int (*)(enum GRAPHIC_PINSHAPE)) &PinShapeGetBitmap, "C++: PinShapeGetBitmap(enum GRAPHIC_PINSHAPE) --> int", pybind11::arg("shape"));

	// ElectricalPinTypeGetText(enum ELECTRICAL_PINTYPE) file:pin_type.h line:77
	M("").def("ElectricalPinTypeGetText", (class wxString (*)(enum ELECTRICAL_PINTYPE)) &ElectricalPinTypeGetText, "C++: ElectricalPinTypeGetText(enum ELECTRICAL_PINTYPE) --> class wxString", pybind11::arg(""));

	// ElectricalPinTypeGetBitmap(enum ELECTRICAL_PINTYPE) file:pin_type.h line:78
	M("").def("ElectricalPinTypeGetBitmap", (int (*)(enum ELECTRICAL_PINTYPE)) &ElectricalPinTypeGetBitmap, "C++: ElectricalPinTypeGetBitmap(enum ELECTRICAL_PINTYPE) --> int", pybind11::arg(""));

	// PinOrientationName(unsigned int) file:pin_type.h line:80
	M("").def("PinOrientationName", (class wxString (*)(unsigned int)) &PinOrientationName, "C++: PinOrientationName(unsigned int) --> class wxString", pybind11::arg("aPinOrientationCode"));

	// PinOrientationCode(int) file:pin_type.h line:81
	M("").def("PinOrientationCode", (int (*)(int)) &PinOrientationCode, "C++: PinOrientationCode(int) --> int", pybind11::arg("index"));

	// PinOrientationIndex(int) file:pin_type.h line:82
	M("").def("PinOrientationIndex", (int (*)(int)) &PinOrientationIndex, "C++: PinOrientationIndex(int) --> int", pybind11::arg("code"));

	// PinTypeNames() file:pin_type.h line:84
	M("").def("PinTypeNames", (const class wxArrayString & (*)()) &PinTypeNames, "C++: PinTypeNames() --> const class wxArrayString &", pybind11::return_value_policy::automatic);

	// PinTypeIcons() file:pin_type.h line:85
	M("").def("PinTypeIcons", (const int & (*)()) &PinTypeIcons, "C++: PinTypeIcons() --> const int &", pybind11::return_value_policy::automatic);

	// PinShapeNames() file:pin_type.h line:87
	M("").def("PinShapeNames", (const class wxArrayString & (*)()) &PinShapeNames, "C++: PinShapeNames() --> const class wxArrayString &", pybind11::return_value_policy::automatic);

	// PinShapeIcons() file:pin_type.h line:88
	M("").def("PinShapeIcons", (const int & (*)()) &PinShapeIcons, "C++: PinShapeIcons() --> const int &", pybind11::return_value_policy::automatic);

	// PinOrientationNames() file:pin_type.h line:90
	M("").def("PinOrientationNames", (const class wxArrayString & (*)()) &PinOrientationNames, "C++: PinOrientationNames() --> const class wxArrayString &", pybind11::return_value_policy::automatic);

	// PinOrientationIcons() file:pin_type.h line:91
	M("").def("PinOrientationIcons", (const int & (*)()) &PinOrientationIcons, "C++: PinOrientationIcons() --> const int &", pybind11::return_value_policy::automatic);

	// IsUTF8(const char *) file:utf8.h line:43
	M("").def("IsUTF8", (bool (*)(const char *)) &IsUTF8, "Test a C string to see if it is UTF8 encoded.\n\n An ASCII string is a valid UTF8 string.\n\nC++: IsUTF8(const char *) --> bool", pybind11::arg("aString"));

	{ // UTF8 file:utf8.h line:70
		pybind11::class_<UTF8, std::shared_ptr<UTF8>> cl(M(""), "UTF8", "An 8 bit string that is assuredly encoded in UTF8, and supplies special conversion\n support to and from wxString, to and from std::string, and has non-mutating iteration\n over Unicode characters.\n\n I've been careful to supply only conversion facilities and not try and duplicate\n wxString() with many member functions. There are multiple ways to create text into\n a std::string without the need of too many member functions:\n\n  - richio.h's StrPrintf().\n  - std::ostringstream.\n\n Because this class uses no virtuals, it should be possible to cast any std::string\n into a UTF8 using this kind of cast: (UTF8 &) without construction or copying being\n the effect of the cast.  Be sure the source std::string holds UTF8 encoded text before\n you do that.");
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("o") );

		cl.def( pybind11::init<const char *>(), pybind11::arg("txt") );

		cl.def( pybind11::init<const wchar_t *>(), pybind11::arg("txt") );

		cl.def( pybind11::init( [](){ return new UTF8(); } ) );
		cl.def( pybind11::init( [](UTF8 const &o){ return new UTF8(o); } ) );
		cl.def("c_str", (const char * (UTF8::*)() const) &UTF8::c_str, "C++: UTF8::c_str() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("empty", (bool (UTF8::*)() const) &UTF8::empty, "C++: UTF8::empty() const --> bool");
		cl.def("find", (unsigned long (UTF8::*)(char) const) &UTF8::find, "C++: UTF8::find(char) const --> unsigned long", pybind11::arg("c"));
		cl.def("find", (unsigned long (UTF8::*)(char, unsigned long) const) &UTF8::find, "C++: UTF8::find(char, unsigned long) const --> unsigned long", pybind11::arg("c"), pybind11::arg("s"));
		cl.def("clear", (void (UTF8::*)()) &UTF8::clear, "C++: UTF8::clear() --> void");
		cl.def("length", (unsigned long (UTF8::*)() const) &UTF8::length, "C++: UTF8::length() const --> unsigned long");
		cl.def("size", (unsigned long (UTF8::*)() const) &UTF8::size, "C++: UTF8::size() const --> unsigned long");
		cl.def("__eq__", (bool (UTF8::*)(const class UTF8 &) const) &UTF8::operator==, "C++: UTF8::operator==(const class UTF8 &) const --> bool", pybind11::arg("rhs"));
		cl.def("__eq__", (bool (UTF8::*)(const char *) const) &UTF8::operator==, "C++: UTF8::operator==(const char *) const --> bool", pybind11::arg("s"));
		cl.def("__iadd__", (class UTF8 & (UTF8::*)(const class UTF8 &)) &UTF8::operator+=, "C++: UTF8::operator+=(const class UTF8 &) --> class UTF8 &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("__iadd__", (class UTF8 & (UTF8::*)(char)) &UTF8::operator+=, "C++: UTF8::operator+=(char) --> class UTF8 &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("__iadd__", (class UTF8 & (UTF8::*)(const char *)) &UTF8::operator+=, "C++: UTF8::operator+=(const char *) --> class UTF8 &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("__iadd__", (class UTF8 & (UTF8::*)(unsigned int)) &UTF8::operator+=, "Append a wide (unicode) char to the UTF8 string.\n if this wide char is not a ASCII7 char, it will be added as a UTF8 multibyte seqence\n \n\n is a UTF-16 value (can be a UTF-32 on Linux)\n\nC++: UTF8::operator+=(unsigned int) --> class UTF8 &", pybind11::return_value_policy::automatic, pybind11::arg("w_ch"));
		cl.def("assign", (class UTF8 & (UTF8::*)(const class wxString &)) &UTF8::operator=, "C++: UTF8::operator=(const class wxString &) --> class UTF8 &", pybind11::return_value_policy::automatic, pybind11::arg("o"));
		cl.def("assign", (class UTF8 & (UTF8::*)(const char *)) &UTF8::operator=, "C++: UTF8::operator=(const char *) --> class UTF8 &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("assign", (class UTF8 & (UTF8::*)(char)) &UTF8::operator=, "C++: UTF8::operator=(char) --> class UTF8 &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("wx_str", (class wxString (UTF8::*)() const) &UTF8::wx_str, "C++: UTF8::wx_str() const --> class wxString");
		cl.def("ubegin", (class UTF8::uni_iter (UTF8::*)() const) &UTF8::ubegin, "Returns a  initialized to the start of \"this\" UTF8 byte sequence.\n\nC++: UTF8::ubegin() const --> class UTF8::uni_iter");
		cl.def("uend", (class UTF8::uni_iter (UTF8::*)() const) &UTF8::uend, "Return a  initialized to the end of \"this\" UTF8 byte sequence.\n\nC++: UTF8::uend() const --> class UTF8::uni_iter");
		cl.def_static("uni_forward", [](const unsigned char * a0) -> int { return UTF8::uni_forward(a0); }, "", pybind11::arg("aSequence"));
		cl.def_static("uni_forward", (int (*)(const unsigned char *, unsigned int *)) &UTF8::uni_forward, "Advance over a single UTF8 encoded multibyte character, capturing the Unicode character\n as it goes, and returning the number of bytes consumed.\n\n \n is the UTF8 byte sequence, must be aligned on start of character.\n \n\n is where to put the unicode character, and may be NULL if no interest.\n \n\n the count of bytes consumed.\n\nC++: UTF8::uni_forward(const unsigned char *, unsigned int *) --> int", pybind11::arg("aSequence"), pybind11::arg("aResult"));
		cl.def("assign", (class UTF8 & (UTF8::*)(const class UTF8 &)) &UTF8::operator=, "C++: UTF8::operator=(const class UTF8 &) --> class UTF8 &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // UTF8::uni_iter file:utf8.h line:203
			auto & enclosing_class = cl;
			pybind11::class_<UTF8::uni_iter, std::shared_ptr<UTF8::uni_iter>> cl(enclosing_class, "uni_iter", "uni_iter\n is a non-mutating iterator that walks through unicode code points in the UTF8 encoded\n string.  The normal ++(), ++(int), ->(), and *() operators are all supported\n for read only access and some return an unsigned holding the unicode character\n appropriate for the respective operator.");
			cl.def( pybind11::init( [](){ return new UTF8::uni_iter(); } ) );
			cl.def( pybind11::init( [](UTF8::uni_iter const &o){ return new UTF8::uni_iter(o); } ) );
			cl.def("plus_plus", (const class UTF8::uni_iter & (UTF8::uni_iter::*)()) &UTF8::uni_iter::operator++, "pre-increment and return uni_iter at new position\n\nC++: UTF8::uni_iter::operator++() --> const class UTF8::uni_iter &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class UTF8::uni_iter (UTF8::uni_iter::*)(int)) &UTF8::uni_iter::operator++, "post-increment and return uni_iter at initial position\n\nC++: UTF8::uni_iter::operator++(int) --> class UTF8::uni_iter", pybind11::arg(""));
			cl.def("__mul__", (unsigned int (UTF8::uni_iter::*)() const) &UTF8::uni_iter::operator*, "return unicode at current position\n\nC++: UTF8::uni_iter::operator*() const --> unsigned int");
			cl.def("__sub__", (class UTF8::uni_iter (UTF8::uni_iter::*)(int) const) &UTF8::uni_iter::operator-, "C++: UTF8::uni_iter::operator-(int) const --> class UTF8::uni_iter", pybind11::arg("aVal"));
			cl.def("__eq__", (bool (UTF8::uni_iter::*)(const class UTF8::uni_iter &) const) &UTF8::uni_iter::operator==, "C++: UTF8::uni_iter::operator==(const class UTF8::uni_iter &) const --> bool", pybind11::arg("other"));
			cl.def("__ne__", (bool (UTF8::uni_iter::*)(const class UTF8::uni_iter &) const) &UTF8::uni_iter::operator!=, "C++: UTF8::uni_iter::operator!=(const class UTF8::uni_iter &) const --> bool", pybind11::arg("other"));
		}

	}
}
