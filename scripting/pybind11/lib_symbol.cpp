#include <core/typeinfo.h> // KICAD_T
#include <eda_item.h> // EDA_ITEM
#include <eda_rect.h> // EDA_RECT
#include <eda_units.h> // EDA_UNITS
#include <fill_type.h> // FILL_TYPE
#include <gal/color4d.h> // KIGFX::COLOR4D
#include <iterator> // __gnu_cxx::__normal_iterator
#include <layers_id_colors_and_visibility.h> // PCB_LAYER_ID
#include <layers_id_colors_and_visibility.h> // SCH_LAYER_ID
#include <lib_field.h> // LIB_FIELD
#include <lib_id.h> // LIB_ID
#include <lib_item.h> // LIB_ITEM
#include <lib_pin.h> // LIB_PIN
#include <lib_symbol.h> // LIBRENTRYOPTIONS
#include <lib_symbol.h> // LIB_PART
#include <lib_symbol.h> // PART_DRAW_OPTIONS
#include <lib_symbol.h> // PART_UNITS
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
#include <pin_type.h> // ELECTRICAL_PINTYPE
#include <pin_type.h> // GRAPHIC_PINSHAPE
#include <render_settings.h> // KIGFX::RENDER_SETTINGS
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <transform.h> // TRANSFORM
#include <utf8.h> // UTF8
#include <view/view_item.h> // KIGFX::VIEW_ITEM
#include <wx/dc.h> // wxDC
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

// LIB_PART file:lib_symbol.h line:93
struct PyCallBack_LIB_PART : public LIB_PART {
	using LIB_PART::LIB_PART;

	class LIB_PART * Duplicate() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "Duplicate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class LIB_PART *>::value) {
				static pybind11::detail::override_caster_t<class LIB_PART *> caster;
				return pybind11::detail::cast_ref<class LIB_PART *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class LIB_PART *>(std::move(o));
		}
		return LIB_PART::Duplicate();
	}
	class wxString GetClass() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetClass");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_PART::GetClass();
	}
	void SetName(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "SetName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return LIB_PART::SetName(a0);
	}
	class wxString GetName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_PART::GetName();
	}
	class LIB_ID GetLibId() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetLibId");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class LIB_ID>::value) {
				static pybind11::detail::override_caster_t<class LIB_ID> caster;
				return pybind11::detail::cast_ref<class LIB_ID>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class LIB_ID>(std::move(o));
		}
		return LIB_PART::GetLibId();
	}
	class wxString GetLibNickname() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetLibNickname");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_PART::GetLibNickname();
	}
	class wxString GetDescription() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetDescription");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_PART::GetDescription();
	}
	class wxString GetSearchText() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetSearchText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_PART::GetSearchText();
	}
	bool IsRoot() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "IsRoot");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_PART::IsRoot();
	}
	const class EDA_RECT GetBoundingBox() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetBoundingBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class EDA_RECT>::value) {
				static pybind11::detail::override_caster_t<const class EDA_RECT> caster;
				return pybind11::detail::cast_ref<const class EDA_RECT>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class EDA_RECT>(std::move(o));
		}
		return LIB_PART::GetBoundingBox();
	}
	int GetUnitCount() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetUnitCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_PART::GetUnitCount();
	}
	class wxString GetUnitReference(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetUnitReference");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_PART::GetUnitReference(a0);
	}
	void SetParent(class EDA_ITEM * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "SetParent");
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
	bool HitTest(const class wxPoint & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return EDA_ITEM::HitTest(a0, a1);
	}
	bool HitTest(const class EDA_RECT & a0, bool a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "HitTest");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return EDA_ITEM::HitTest(a0, a1, a2);
	}
	class wxPoint GetPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint>::value) {
				static pybind11::detail::override_caster_t<class wxPoint> caster;
				return pybind11::detail::cast_ref<class wxPoint>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint>(std::move(o));
		}
		return EDA_ITEM::GetPosition();
	}
	void SetPosition(const class wxPoint & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "SetPosition");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return EDA_ITEM::SetPosition(a0);
	}
	const class wxPoint GetFocusPosition() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetFocusPosition");
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
	class EDA_ITEM * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class EDA_ITEM *>::value) {
				static pybind11::detail::override_caster_t<class EDA_ITEM *> caster;
				return pybind11::detail::cast_ref<class EDA_ITEM *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class EDA_ITEM *>(std::move(o));
		}
		return EDA_ITEM::Clone();
	}
	class wxString GetSelectMenuText(enum EDA_UNITS a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "GetSelectMenuText");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "Matches");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "Replace");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_PART *>(this), "IsReplaceable");
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

void bind_lib_symbol(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// LIBRENTRYOPTIONS file:lib_symbol.h line:52
	pybind11::enum_<LIBRENTRYOPTIONS>(M(""), "LIBRENTRYOPTIONS", pybind11::arithmetic(), "")
		.value("ENTRY_NORMAL", ENTRY_NORMAL)
		.value("ENTRY_POWER", ENTRY_POWER)
		.export_values();

;

	{ // PART_DRAW_OPTIONS file:lib_symbol.h line:62
		pybind11::class_<PART_DRAW_OPTIONS, std::shared_ptr<PART_DRAW_OPTIONS>> cl(M(""), "PART_DRAW_OPTIONS", "");
		cl.def( pybind11::init( [](){ return new PART_DRAW_OPTIONS(); } ) );
		cl.def_readwrite("transform", &PART_DRAW_OPTIONS::transform);
		cl.def_readwrite("draw_visible_fields", &PART_DRAW_OPTIONS::draw_visible_fields);
		cl.def_readwrite("draw_hidden_fields", &PART_DRAW_OPTIONS::draw_hidden_fields);
		cl.def_readwrite("show_elec_type", &PART_DRAW_OPTIONS::show_elec_type);
	}
	{ // PART_UNITS file:lib_symbol.h line:79
		pybind11::class_<PART_UNITS, std::shared_ptr<PART_UNITS>> cl(M(""), "PART_UNITS", "");
		cl.def( pybind11::init( [](){ return new PART_UNITS(); } ) );
		cl.def_readwrite("m_unit", &PART_UNITS::m_unit);
		cl.def_readwrite("m_convert", &PART_UNITS::m_convert);
		cl.def_readwrite("m_items", &PART_UNITS::m_items);
	}
	{ // LIB_PART file:lib_symbol.h line:93
		pybind11::class_<LIB_PART, std::shared_ptr<LIB_PART>, PyCallBack_LIB_PART, EDA_ITEM, LIB_TREE_ITEM> cl(M(""), "LIB_PART", "Define a library symbol object.\n\n A library symbol object is typically saved and loaded in a part library file (.lib).\n Library symbols are different from schematic symbols.");
		cl.def("Duplicate", (class LIB_PART * (LIB_PART::*)() const) &LIB_PART::Duplicate, "Create a copy of a LIB_PART and assigns unique KIIDs to the copy and its children.\n\nC++: LIB_PART::Duplicate() const --> class LIB_PART *", pybind11::return_value_policy::automatic);
		cl.def("SetParent", [](LIB_PART &o) -> void { return o.SetParent(); }, "");
		cl.def("SetParent", (void (LIB_PART::*)(class LIB_PART *)) &LIB_PART::SetParent, "C++: LIB_PART::SetParent(class LIB_PART *) --> void", pybind11::arg("aParent"));
		cl.def("GetClass", (class wxString (LIB_PART::*)() const) &LIB_PART::GetClass, "C++: LIB_PART::GetClass() const --> class wxString");
		cl.def("SetName", (void (LIB_PART::*)(const class wxString &)) &LIB_PART::SetName, "C++: LIB_PART::SetName(const class wxString &) --> void", pybind11::arg("aName"));
		cl.def("GetName", (class wxString (LIB_PART::*)() const) &LIB_PART::GetName, "C++: LIB_PART::GetName() const --> class wxString");
		cl.def("GetLibId", (class LIB_ID (LIB_PART::*)() const) &LIB_PART::GetLibId, "C++: LIB_PART::GetLibId() const --> class LIB_ID");
		cl.def("SetLibId", (void (LIB_PART::*)(const class LIB_ID &)) &LIB_PART::SetLibId, "C++: LIB_PART::SetLibId(const class LIB_ID &) --> void", pybind11::arg("aLibId"));
		cl.def("GetLibNickname", (class wxString (LIB_PART::*)() const) &LIB_PART::GetLibNickname, "C++: LIB_PART::GetLibNickname() const --> class wxString");
		cl.def("SetDescription", (void (LIB_PART::*)(const class wxString &)) &LIB_PART::SetDescription, "C++: LIB_PART::SetDescription(const class wxString &) --> void", pybind11::arg("aDescription"));
		cl.def("GetDescription", (class wxString (LIB_PART::*)()) &LIB_PART::GetDescription, "C++: LIB_PART::GetDescription() --> class wxString");
		cl.def("SetKeyWords", (void (LIB_PART::*)(const class wxString &)) &LIB_PART::SetKeyWords, "C++: LIB_PART::SetKeyWords(const class wxString &) --> void", pybind11::arg("aKeyWords"));
		cl.def("GetKeyWords", (class wxString (LIB_PART::*)() const) &LIB_PART::GetKeyWords, "C++: LIB_PART::GetKeyWords() const --> class wxString");
		cl.def("GetSearchText", (class wxString (LIB_PART::*)()) &LIB_PART::GetSearchText, "C++: LIB_PART::GetSearchText() --> class wxString");
		cl.def("IsRoot", (bool (LIB_PART::*)() const) &LIB_PART::IsRoot, "For symbols derived from other symbols, IsRoot() indicates no derivation.\n\nC++: LIB_PART::IsRoot() const --> bool");
		cl.def("IsAlias", (bool (LIB_PART::*)() const) &LIB_PART::IsAlias, "C++: LIB_PART::IsAlias() const --> bool");
		cl.def("GetLibraryName", (const class wxString (LIB_PART::*)() const) &LIB_PART::GetLibraryName, "C++: LIB_PART::GetLibraryName() const --> const class wxString");
		cl.def("GetLastModDate", (unsigned int (LIB_PART::*)() const) &LIB_PART::GetLastModDate, "C++: LIB_PART::GetLastModDate() const --> unsigned int");
		cl.def("SetFPFilters", (void (LIB_PART::*)(const class wxArrayString &)) &LIB_PART::SetFPFilters, "C++: LIB_PART::SetFPFilters(const class wxArrayString &) --> void", pybind11::arg("aFilters"));
		cl.def("GetFPFilters", (class wxArrayString (LIB_PART::*)() const) &LIB_PART::GetFPFilters, "C++: LIB_PART::GetFPFilters() const --> class wxArrayString");
		cl.def("GetUnitBoundingBox", (const class EDA_RECT (LIB_PART::*)(int, int) const) &LIB_PART::GetUnitBoundingBox, "Get the bounding box for the symbol.\n\n \n the part bounding box ( in user coordinates )\n \n\n = unit selection = 0, or 1..n\n \n\n = 0, 1 or 2\n  If aUnit == 0, unit is not used\n  if aConvert == 0 Convert is non used\n  Invisible fields are not taken in account\n\nC++: LIB_PART::GetUnitBoundingBox(int, int) const --> const class EDA_RECT", pybind11::arg("aUnit"), pybind11::arg("aConvert"));
		cl.def("GetBodyBoundingBox", (const class EDA_RECT (LIB_PART::*)(int, int) const) &LIB_PART::GetBodyBoundingBox, "Get the symbol bounding box excluding fields.\n\n \n the part bounding box ( in user coordinates ) without fields\n \n\n = unit selection = 0, or 1..n\n \n\n = 0, 1 or 2\n  If aUnit == 0, unit is not used\n  if aConvert == 0 Convert is non used\n  Fields are not taken in account\n\nC++: LIB_PART::GetBodyBoundingBox(int, int) const --> const class EDA_RECT", pybind11::arg("aUnit"), pybind11::arg("aConvert"));
		cl.def("GetBoundingBox", (const class EDA_RECT (LIB_PART::*)() const) &LIB_PART::GetBoundingBox, "C++: LIB_PART::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("IsPower", (bool (LIB_PART::*)() const) &LIB_PART::IsPower, "C++: LIB_PART::IsPower() const --> bool");
		cl.def("IsNormal", (bool (LIB_PART::*)() const) &LIB_PART::IsNormal, "C++: LIB_PART::IsNormal() const --> bool");
		cl.def("SetPower", (void (LIB_PART::*)()) &LIB_PART::SetPower, "C++: LIB_PART::SetPower() --> void");
		cl.def("SetNormal", (void (LIB_PART::*)()) &LIB_PART::SetNormal, "C++: LIB_PART::SetNormal() --> void");
		cl.def("LockUnits", (void (LIB_PART::*)(bool)) &LIB_PART::LockUnits, "Set interchangeable the property for part units.\n \n\n when true then units are set as not interchangeable.\n\nC++: LIB_PART::LockUnits(bool) --> void", pybind11::arg("aLockUnits"));
		cl.def("UnitsLocked", (bool (LIB_PART::*)() const) &LIB_PART::UnitsLocked, "Check whether part units are interchangeable.\n \n\n False when interchangeable, true otherwise.\n\nC++: LIB_PART::UnitsLocked() const --> bool");
		cl.def("SetFields", (void (LIB_PART::*)(const int &)) &LIB_PART::SetFields, "Overwrite all the existing fields in this symbol with fields supplied\n in \n\n The only known caller of this function is the library part field editor, and it\n establishes needed behavior.\n\n \n is a set of fields to import, removing all previous fields.\n\nC++: LIB_PART::SetFields(const int &) --> void", pybind11::arg("aFieldsList"));
		cl.def("GetFields", (void (LIB_PART::*)(int &)) &LIB_PART::GetFields, "Return a list of fields within this part.\n\n \n - List to add fields to\n\nC++: LIB_PART::GetFields(int &) --> void", pybind11::arg("aList"));
		cl.def("GetFields", (void (LIB_PART::*)(int &)) &LIB_PART::GetFields, "C++: LIB_PART::GetFields(int &) --> void", pybind11::arg("aList"));
		cl.def("AddField", (void (LIB_PART::*)(class LIB_FIELD *)) &LIB_PART::AddField, "Add a field.  Takes ownership of the pointer.\n\nC++: LIB_PART::AddField(class LIB_FIELD *) --> void", pybind11::arg("aField"));
		cl.def("FindField", (class LIB_FIELD * (LIB_PART::*)(const class wxString &)) &LIB_PART::FindField, "Find a field within this part matching  and returns it\n or NULL if not found.\n\nC++: LIB_PART::FindField(const class wxString &) --> class LIB_FIELD *", pybind11::return_value_policy::automatic, pybind11::arg("aFieldName"));
		cl.def("GetField", (class LIB_FIELD * (LIB_PART::*)(int) const) &LIB_PART::GetField, "Return pointer to the requested field.\n\n \n - Id of field to return.\n \n\n The field if found, otherwise NULL.\n\nC++: LIB_PART::GetField(int) const --> class LIB_FIELD *", pybind11::return_value_policy::automatic, pybind11::arg("aId"));
		cl.def("GetValueField", (class LIB_FIELD & (LIB_PART::*)()) &LIB_PART::GetValueField, "Return reference to the value field. \n\nC++: LIB_PART::GetValueField() --> class LIB_FIELD &", pybind11::return_value_policy::automatic);
		cl.def("GetReferenceField", (class LIB_FIELD & (LIB_PART::*)()) &LIB_PART::GetReferenceField, "Return reference to the reference designator field. \n\nC++: LIB_PART::GetReferenceField() --> class LIB_FIELD &", pybind11::return_value_policy::automatic);
		cl.def("GetFootprintField", (class LIB_FIELD & (LIB_PART::*)()) &LIB_PART::GetFootprintField, "Return reference to the footprint field \n\nC++: LIB_PART::GetFootprintField() --> class LIB_FIELD &", pybind11::return_value_policy::automatic);
		cl.def("GetDatasheetField", (class LIB_FIELD & (LIB_PART::*)()) &LIB_PART::GetDatasheetField, "Return reference to the datasheet field. \n\nC++: LIB_PART::GetDatasheetField() --> class LIB_FIELD &", pybind11::return_value_policy::automatic);
		cl.def("Print", (void (LIB_PART::*)(const class KIGFX::RENDER_SETTINGS *, const class wxPoint &, int, int, const struct PART_DRAW_OPTIONS &)) &LIB_PART::Print, "Print part.\n\n \n - Position of part.\n \n\n - unit if multiple units per part.\n \n\n - Component conversion (DeMorgan) if available.\n \n\n - Drawing options\n\nC++: LIB_PART::Print(const class KIGFX::RENDER_SETTINGS *, const class wxPoint &, int, int, const struct PART_DRAW_OPTIONS &) --> void", pybind11::arg("aSettings"), pybind11::arg("aOffset"), pybind11::arg("aMulti"), pybind11::arg("aConvert"), pybind11::arg("aOpts"));
		cl.def("AddDrawItem", (void (LIB_PART::*)(class LIB_ITEM *)) &LIB_PART::AddDrawItem, "Add a new draw  to the draw object list.\n\n \n - New draw object to add to part.\n\nC++: LIB_PART::AddDrawItem(class LIB_ITEM *) --> void", pybind11::arg("aItem"));
		cl.def("RemoveDrawItem", (void (LIB_PART::*)(class LIB_ITEM *)) &LIB_PART::RemoveDrawItem, "Remove draw  from list.\n\n \n - Draw item to remove from list.\n\nC++: LIB_PART::RemoveDrawItem(class LIB_ITEM *) --> void", pybind11::arg("aItem"));
		cl.def("GetNextDrawItem", [](LIB_PART &o) -> LIB_ITEM * { return o.GetNextDrawItem(); }, "", pybind11::return_value_policy::automatic);
		cl.def("GetNextDrawItem", [](LIB_PART &o, const class LIB_ITEM * a0) -> LIB_ITEM * { return o.GetNextDrawItem(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("aItem"));
		cl.def("GetNextDrawItem", (class LIB_ITEM * (LIB_PART::*)(const class LIB_ITEM *, enum KICAD_T)) &LIB_PART::GetNextDrawItem, "Return the next draw object pointer.\n\n \n - Pointer to the current draw item.  Setting item NULL\n                with return the first item of type in the list.\n \n\n - type of searched item (filter).\n                if TYPE_NOT_INIT search for all items types\n \n\n - The next drawing object in the list if found, otherwise NULL.\n\nC++: LIB_PART::GetNextDrawItem(const class LIB_ITEM *, enum KICAD_T) --> class LIB_ITEM *", pybind11::return_value_policy::automatic, pybind11::arg("aItem"), pybind11::arg("aType"));
		cl.def("GetPinCount", (unsigned long (LIB_PART::*)() const) &LIB_PART::GetPinCount, "C++: LIB_PART::GetPinCount() const --> unsigned long");
		cl.def("GetNextPin", [](LIB_PART &o) -> LIB_PIN * { return o.GetNextPin(); }, "", pybind11::return_value_policy::automatic);
		cl.def("GetNextPin", (class LIB_PIN * (LIB_PART::*)(class LIB_PIN *)) &LIB_PART::GetNextPin, "Return the next pin object from the draw list.\n\n This is just a pin object specific version of GetNextDrawItem().\n\n \n - Pointer to the previous pin item, or NULL to get the\n                first pin in the draw object list.\n \n\n - The next pin object in the list if found, otherwise NULL.\n\nC++: LIB_PART::GetNextPin(class LIB_PIN *) --> class LIB_PIN *", pybind11::return_value_policy::automatic, pybind11::arg("aItem"));
		cl.def("GetPins", [](LIB_PART &o, int & a0) -> void { return o.GetPins(a0); }, "", pybind11::arg("aList"));
		cl.def("GetPins", [](LIB_PART &o, int & a0, int const & a1) -> void { return o.GetPins(a0, a1); }, "", pybind11::arg("aList"), pybind11::arg("aUnit"));
		cl.def("GetPins", (void (LIB_PART::*)(int &, int, int)) &LIB_PART::GetPins, "Return a list of pin object pointers from the draw item list.\n\n Note pin objects are owned by the draw list of the part.\n Deleting any of the objects will leave list in a unstable state\n and will likely segfault when the list is destroyed.\n\n \n - Pin list to place pin object pointers into.\n \n\n - Unit number of pin to add to list.  Set to 0 to\n                get pins from any part unit.\n \n\n - Convert number of pin to add to list.  Set to 0 to\n                   get pins from any convert of part.\n\nC++: LIB_PART::GetPins(int &, int, int) --> void", pybind11::arg("aList"), pybind11::arg("aUnit"), pybind11::arg("aConvert"));
		cl.def("GetPin", [](LIB_PART &o, const class wxString & a0) -> LIB_PIN * { return o.GetPin(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("aNumber"));
		cl.def("GetPin", [](LIB_PART &o, const class wxString & a0, int const & a1) -> LIB_PIN * { return o.GetPin(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("aNumber"), pybind11::arg("aUnit"));
		cl.def("GetPin", (class LIB_PIN * (LIB_PART::*)(const class wxString &, int, int)) &LIB_PART::GetPin, "Return pin object with the requested pin \n\n \n - Number of the pin to find.\n \n\n - Unit of the part to find.  Set to 0 if a specific\n                unit number is not required.\n \n\n - Alternate body style filter (DeMorgan).  Set to 0 if\n                   no alternate body style is required.\n \n\n The pin object if found.  Otherwise NULL.\n\nC++: LIB_PART::GetPin(const class wxString &, int, int) --> class LIB_PIN *", pybind11::return_value_policy::automatic, pybind11::arg("aNumber"), pybind11::arg("aUnit"), pybind11::arg("aConvert"));
		cl.def("PinsConflictWith", (bool (LIB_PART::*)(class LIB_PART &, bool, bool, bool, bool, bool)) &LIB_PART::PinsConflictWith, "Return true if this part's pins do not match another part's pins. This\n is used to detect whether the project cache is out of sync with the\n system libs.\n\n \n - The other library part to test\n \n\n - Whether two pins at the same point must have the same number.\n \n\n - Whether two pins at the same point must have the same name.\n \n\n - Whether two pins at the same point must have the same electrical type.\n \n\n - Whether two pins at the same point must have the same orientation.\n \n\n - Whether two pins at the same point must have the same length.\n\nC++: LIB_PART::PinsConflictWith(class LIB_PART &, bool, bool, bool, bool, bool) --> bool", pybind11::arg("aOtherPart"), pybind11::arg("aTestNums"), pybind11::arg("aTestNames"), pybind11::arg("aTestType"), pybind11::arg("aTestOrientation"), pybind11::arg("aTestLength"));
		cl.def("SetOffset", (void (LIB_PART::*)(const class wxPoint &)) &LIB_PART::SetOffset, "Move the part \n\n \n - Offset displacement.\n\nC++: LIB_PART::SetOffset(const class wxPoint &) --> void", pybind11::arg("aOffset"));
		cl.def("RemoveDuplicateDrawItems", (void (LIB_PART::*)()) &LIB_PART::RemoveDuplicateDrawItems, "Remove duplicate draw items from list.\n\nC++: LIB_PART::RemoveDuplicateDrawItems() --> void");
		cl.def("HasConversion", (bool (LIB_PART::*)() const) &LIB_PART::HasConversion, "Test if part has more than one body conversion type (DeMorgan).\n\n \n True if part has more than one conversion.\n\nC++: LIB_PART::HasConversion() const --> bool");
		cl.def("ClearTempFlags", (void (LIB_PART::*)()) &LIB_PART::ClearTempFlags, "Clears the status flag all draw objects in this part.\n\nC++: LIB_PART::ClearTempFlags() --> void");
		cl.def("ClearEditFlags", (void (LIB_PART::*)()) &LIB_PART::ClearEditFlags, "C++: LIB_PART::ClearEditFlags() --> void");
		cl.def("LocateDrawItem", (class LIB_ITEM * (LIB_PART::*)(int, int, enum KICAD_T, const class wxPoint &)) &LIB_PART::LocateDrawItem, "Locate a draw object.\n\n \n - Unit number of draw item.\n \n\n - Body style of draw item.\n \n\n - Draw object type, set to 0 to search for any type.\n \n\n - Coordinate for hit testing.\n \n\n The draw object if found.  Otherwise NULL.\n\nC++: LIB_PART::LocateDrawItem(int, int, enum KICAD_T, const class wxPoint &) --> class LIB_ITEM *", pybind11::return_value_policy::automatic, pybind11::arg("aUnit"), pybind11::arg("aConvert"), pybind11::arg("aType"), pybind11::arg("aPoint"));
		cl.def("LocateDrawItem", (class LIB_ITEM * (LIB_PART::*)(int, int, enum KICAD_T, const class wxPoint &, const class TRANSFORM &)) &LIB_PART::LocateDrawItem, "Locate a draw object (overlaid)\n\n \n - Unit number of draw item.\n \n\n - Body style of draw item.\n \n\n - Draw object type, set to 0 to search for any type.\n \n\n - Coordinate for hit testing.\n \n\n = the transform matrix\n \n\n The draw object if found.  Otherwise NULL.\n\nC++: LIB_PART::LocateDrawItem(int, int, enum KICAD_T, const class wxPoint &, const class TRANSFORM &) --> class LIB_ITEM *", pybind11::return_value_policy::automatic, pybind11::arg("aUnit"), pybind11::arg("aConvert"), pybind11::arg("aType"), pybind11::arg("aPoint"), pybind11::arg("aTransform"));
		cl.def("SetUnitCount", [](LIB_PART &o, int const & a0) -> void { return o.SetUnitCount(a0); }, "", pybind11::arg("aCount"));
		cl.def("SetUnitCount", (void (LIB_PART::*)(int, bool)) &LIB_PART::SetUnitCount, "Set the units per part count.\n\n If the count is greater than the current count, then the all of the\n current draw items are duplicated for each additional part.  If the\n count is less than the current count, all draw objects for units\n greater that count are removed from the part.\n\n \n - Number of units per package.\n \n\n Create duplicate draw items of unit 1 for each additionl unit.\n\nC++: LIB_PART::SetUnitCount(int, bool) --> void", pybind11::arg("aCount"), pybind11::arg("aDuplicateDrawItems"));
		cl.def("GetUnitCount", (int (LIB_PART::*)() const) &LIB_PART::GetUnitCount, "C++: LIB_PART::GetUnitCount() const --> int");
		cl.def("GetUnitReference", (class wxString (LIB_PART::*)(int)) &LIB_PART::GetUnitReference, "Return an identifier for  for symbols with units.\n\nC++: LIB_PART::GetUnitReference(int) --> class wxString", pybind11::arg("aUnit"));
		cl.def("IsMulti", (bool (LIB_PART::*)() const) &LIB_PART::IsMulti, "true if the part has multiple units per part.\n When true, the reference has a sub reference to identify part.\n\nC++: LIB_PART::IsMulti() const --> bool");
		cl.def_static("SubReference", [](int const & a0) -> wxString { return LIB_PART::SubReference(a0); }, "", pybind11::arg("aUnit"));
		cl.def_static("SubReference", (class wxString (*)(int, bool)) &LIB_PART::SubReference, "the sub reference for part having multiple units per part.\n The sub reference identify the part (or unit)\n \n\n = the part identifier ( 1 to max count)\n \n\n = true (default) to prepend the sub ref\n    by the separator symbol (if any)\n Note: this is a static function.\n\nC++: LIB_PART::SubReference(int, bool) --> class wxString", pybind11::arg("aUnit"), pybind11::arg("aAddSeparator"));
		cl.def_static("GetSubpartIdSeparator", (int (*)()) &LIB_PART::GetSubpartIdSeparator, "C++: LIB_PART::GetSubpartIdSeparator() --> int");
		cl.def_static("SubpartIdSeparatorPtr", (int * (*)()) &LIB_PART::SubpartIdSeparatorPtr, "Return a reference to m_subpartIdSeparator, only for read/save setting functions.\n\nC++: LIB_PART::SubpartIdSeparatorPtr() --> int *", pybind11::return_value_policy::automatic);
		cl.def_static("GetSubpartFirstId", (int (*)()) &LIB_PART::GetSubpartFirstId, "C++: LIB_PART::GetSubpartFirstId() --> int");
		cl.def_static("SubpartFirstIdPtr", (int * (*)()) &LIB_PART::SubpartFirstIdPtr, "Return a reference to m_subpartFirstId, only for read/save setting functions.\n\nC++: LIB_PART::SubpartFirstIdPtr() --> int *", pybind11::return_value_policy::automatic);
		cl.def_static("SetSubpartIdNotation", (void (*)(int, int)) &LIB_PART::SetSubpartIdNotation, "Set the separator char between the subpart id and the reference\n 0 (no separator) or '.' , '-' and '_'\n and the ascii char value to calculate the subpart symbol id from the part number:\n 'A' or '1' only are allowed. (to print U1.A or U1.1)\n if this is a digit, a number is used as id symbol\n Note also if the subpart symbol is a digit, the separator cannot be null.\n \n\n = the separator symbol (0 (no separator) or '.' , '-' and '_')\n \n\n = the Id of the first part ('A' or '1')\n\nC++: LIB_PART::SetSubpartIdNotation(int, int) --> void", pybind11::arg("aSep"), pybind11::arg("aFirstId"));
		cl.def("SetConversion", [](LIB_PART &o, bool const & a0) -> void { return o.SetConversion(a0); }, "", pybind11::arg("aSetConvert"));
		cl.def("SetConversion", (void (LIB_PART::*)(bool, bool)) &LIB_PART::SetConversion, "Set or clear the alternate body style (DeMorgan) for the part.\n\n If the part already has an alternate body style set and a\n asConvert if false, all of the existing draw items for the alternate\n body style are remove.  If the alternate body style is not set and\n asConvert is true, than the base draw items are duplicated and\n added to the part.\n\n \n - Set or clear the part alternate body style.\n \n\n - Duplicate all pins from original body style if true.\n\nC++: LIB_PART::SetConversion(bool, bool) --> void", pybind11::arg("aSetConvert"), pybind11::arg("aDuplicatePins"));
		cl.def("SetPinNameOffset", (void (LIB_PART::*)(int)) &LIB_PART::SetPinNameOffset, "Set the offset in mils of the pin name text from the pin symbol.\n\n Set the offset to 0 to draw the pin name above the pin symbol.\n\n \n - The offset in mils.\n\nC++: LIB_PART::SetPinNameOffset(int) --> void", pybind11::arg("aOffset"));
		cl.def("GetPinNameOffset", (int (LIB_PART::*)()) &LIB_PART::GetPinNameOffset, "C++: LIB_PART::GetPinNameOffset() --> int");
		cl.def("SetShowPinNames", (void (LIB_PART::*)(bool)) &LIB_PART::SetShowPinNames, "Set or clear the pin name visibility flag.\n\n \n - True to make the part pin names visible.\n\nC++: LIB_PART::SetShowPinNames(bool) --> void", pybind11::arg("aShow"));
		cl.def("ShowPinNames", (bool (LIB_PART::*)()) &LIB_PART::ShowPinNames, "C++: LIB_PART::ShowPinNames() --> bool");
		cl.def("SetShowPinNumbers", (void (LIB_PART::*)(bool)) &LIB_PART::SetShowPinNumbers, "Set or clear the pin number visibility flag.\n\n \n - True to make the part pin numbers visible.\n\nC++: LIB_PART::SetShowPinNumbers(bool) --> void", pybind11::arg("aShow"));
		cl.def("ShowPinNumbers", (bool (LIB_PART::*)()) &LIB_PART::ShowPinNumbers, "C++: LIB_PART::ShowPinNumbers() --> bool");
		cl.def("SetIncludeInBom", (void (LIB_PART::*)(bool)) &LIB_PART::SetIncludeInBom, "Set or clear the include in schematic bill of materials flag.\n\n \n true to include symbol in schematic bill of material\n\nC++: LIB_PART::SetIncludeInBom(bool) --> void", pybind11::arg("aIncludeInBom"));
		cl.def("GetIncludeInBom", (bool (LIB_PART::*)() const) &LIB_PART::GetIncludeInBom, "C++: LIB_PART::GetIncludeInBom() const --> bool");
		cl.def("SetIncludeOnBoard", (void (LIB_PART::*)(bool)) &LIB_PART::SetIncludeOnBoard, "Set or clear include in board netlist flag.\n\n \n true to include symbol in the board netlist\n\nC++: LIB_PART::SetIncludeOnBoard(bool) --> void", pybind11::arg("aIncludeOnBoard"));
		cl.def("GetIncludeOnBoard", (bool (LIB_PART::*)() const) &LIB_PART::GetIncludeOnBoard, "C++: LIB_PART::GetIncludeOnBoard() const --> bool");
		cl.def("Compare", (int (LIB_PART::*)(const class LIB_PART &) const) &LIB_PART::Compare, "Comparison test that can be used for operators.\n\n \n is the right hand side symbol used for comparison.\n\n \n -1 if this symbol is less than \n         1 if this symbol is greater than \n         0 if this symbol is the same as \n     \n\nC++: LIB_PART::Compare(const class LIB_PART &) const --> int", pybind11::arg("aRhs"));
		cl.def("__eq__", (bool (LIB_PART::*)(const class LIB_PART *) const) &LIB_PART::operator==, "C++: LIB_PART::operator==(const class LIB_PART *) const --> bool", pybind11::arg("aPart"));
		cl.def("__eq__", (bool (LIB_PART::*)(const class LIB_PART &) const) &LIB_PART::operator==, "C++: LIB_PART::operator==(const class LIB_PART &) const --> bool", pybind11::arg("aPart"));
		cl.def("__ne__", (bool (LIB_PART::*)(const class LIB_PART &) const) &LIB_PART::operator!=, "C++: LIB_PART::operator!=(const class LIB_PART &) const --> bool", pybind11::arg("aPart"));
		cl.def("assign", (const class LIB_PART & (LIB_PART::*)(const class LIB_PART &)) &LIB_PART::operator=, "C++: LIB_PART::operator=(const class LIB_PART &) --> const class LIB_PART &", pybind11::return_value_policy::automatic, pybind11::arg("aPart"));
		cl.def("Flatten", (int (LIB_PART::*)() const) &LIB_PART::Flatten, "Return a flattened symbol inheritance to the caller.\n\n If the symbol does not inherit from another symbol, a copy of the symbol is returned.\n\n \n a flattened symbol on the heap\n\nC++: LIB_PART::Flatten() const --> int");
		cl.def("GetUnitDrawItems", (int (LIB_PART::*)()) &LIB_PART::GetUnitDrawItems, "Return a list of LIB_ITEM objects separated by unit and convert number.\n\n \n This does not include LIB_FIELD objects since they are not associated with\n       unit and/or convert numbers.\n\nC++: LIB_PART::GetUnitDrawItems() --> int");
		cl.def("GetUniqueUnits", (int (LIB_PART::*)()) &LIB_PART::GetUniqueUnits, "Return a list of unit numbers that are unique to this symbol.\n\n If the symbol is inherited (alias), the unique units of the parent symbol are returned.\n When comparing pins, the pin number is ignored.\n\n \n a list of unique unit numbers and their associated draw items.\n\nC++: LIB_PART::GetUniqueUnits() --> int");
		cl.def("GetUnitItems", (int (LIB_PART::*)(int, int)) &LIB_PART::GetUnitItems, "Return a list of item pointers for  and  for this symbol.\n\n \n #LIB_FIELD objects are not included.\n\n \n is the unit number of the item, -1 includes all units.\n \n\n is the alternate body styple of the item, -1 includes all body styles.\n\n \n a list of unit items.\n\nC++: LIB_PART::GetUnitItems(int, int) --> int", pybind11::arg("aUnit"), pybind11::arg("aConvert"));
	}
}
