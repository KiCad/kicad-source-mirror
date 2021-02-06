#include <gal/color4d.h> // EDA_COLOR_T
#include <gal/color4d.h> // KIGFX::COLOR4D
#include <layers_id_colors_and_visibility.h> // PCB_LAYER_ID
#include <render_settings.h> // KIGFX::RENDER_SETTINGS
#include <sstream> // __str__
#include <view/view_item.h> // KIGFX::VIEW_ITEM
#include <wx/dc.h> // wxDC
#include <wx/dc.h> // wxDCImpl
#include <wx/dc.h> // wxDrawObject
#include <wx/dc.h> // wxFloodFillStyle
#include <wx/dc.h> // wxFontMetrics
#include <wx/dc.h> // wxMappingMode
#include <wx/dc.h> // wxRasterOperationMode

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// KIGFX::RENDER_SETTINGS file:render_settings.h line:55
struct PyCallBack_KIGFX_RENDER_SETTINGS : public KIGFX::RENDER_SETTINGS {
	using KIGFX::RENDER_SETTINGS::RENDER_SETTINGS;

	class KIGFX::COLOR4D GetColor(const class KIGFX::VIEW_ITEM * a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const KIGFX::RENDER_SETTINGS *>(this), "GetColor");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class KIGFX::COLOR4D>::value) {
				static pybind11::detail::override_caster_t<class KIGFX::COLOR4D> caster;
				return pybind11::detail::cast_ref<class KIGFX::COLOR4D>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class KIGFX::COLOR4D>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"RENDER_SETTINGS::GetColor\"");
	}
	const class KIGFX::COLOR4D & GetBackgroundColor() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const KIGFX::RENDER_SETTINGS *>(this), "GetBackgroundColor");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class KIGFX::COLOR4D &>::value) {
				static pybind11::detail::override_caster_t<const class KIGFX::COLOR4D &> caster;
				return pybind11::detail::cast_ref<const class KIGFX::COLOR4D &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class KIGFX::COLOR4D &>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"RENDER_SETTINGS::GetBackgroundColor\"");
	}
	void SetBackgroundColor(const class KIGFX::COLOR4D & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const KIGFX::RENDER_SETTINGS *>(this), "SetBackgroundColor");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"RENDER_SETTINGS::SetBackgroundColor\"");
	}
	const class KIGFX::COLOR4D & GetGridColor() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const KIGFX::RENDER_SETTINGS *>(this), "GetGridColor");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class KIGFX::COLOR4D &>::value) {
				static pybind11::detail::override_caster_t<const class KIGFX::COLOR4D &> caster;
				return pybind11::detail::cast_ref<const class KIGFX::COLOR4D &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class KIGFX::COLOR4D &>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"RENDER_SETTINGS::GetGridColor\"");
	}
	const class KIGFX::COLOR4D & GetCursorColor() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const KIGFX::RENDER_SETTINGS *>(this), "GetCursorColor");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class KIGFX::COLOR4D &>::value) {
				static pybind11::detail::override_caster_t<const class KIGFX::COLOR4D &> caster;
				return pybind11::detail::cast_ref<const class KIGFX::COLOR4D &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class KIGFX::COLOR4D &>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"RENDER_SETTINGS::GetCursorColor\"");
	}
	bool IsBackgroundDark() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const KIGFX::RENDER_SETTINGS *>(this), "IsBackgroundDark");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return RENDER_SETTINGS::IsBackgroundDark();
	}
	void update() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const KIGFX::RENDER_SETTINGS *>(this), "update");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return RENDER_SETTINGS::update();
	}
};

void bind_render_settings(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // KIGFX::RENDER_SETTINGS file:render_settings.h line:55
		pybind11::class_<KIGFX::RENDER_SETTINGS, std::shared_ptr<KIGFX::RENDER_SETTINGS>, PyCallBack_KIGFX_RENDER_SETTINGS> cl(M("KIGFX"), "RENDER_SETTINGS", "Container for all the knowledge about how graphical objects are drawn on any output\n surface/device.\n\n This includes:\n  - color/transparency settings\n  - highlighting and high contrast mode control\n  - drawing quality control (sketch/outline mode)\n  - text processing flags\n\n The class acts as an interface between the PAINTER object and the GUI (i.e. Layers/Items\n widget or display options dialog).");
		cl.def( pybind11::init( [](){ return new PyCallBack_KIGFX_RENDER_SETTINGS(); } ) );
		cl.def("SetLayerIsHighContrast", [](KIGFX::RENDER_SETTINGS &o, int const & a0) -> void { return o.SetLayerIsHighContrast(a0); }, "", pybind11::arg("aLayerId"));
		cl.def("SetLayerIsHighContrast", (void (KIGFX::RENDER_SETTINGS::*)(int, bool)) &KIGFX::RENDER_SETTINGS::SetLayerIsHighContrast, "Set the specified layer as high-contrast.\n\n \n is a layer number that should be displayed in a specific mode.\n \n\n is the new layer state ( true = active or false = not active).\n\nC++: KIGFX::RENDER_SETTINGS::SetLayerIsHighContrast(int, bool) --> void", pybind11::arg("aLayerId"), pybind11::arg("aEnabled"));
		cl.def("GetLayerIsHighContrast", (bool (KIGFX::RENDER_SETTINGS::*)(int) const) &KIGFX::RENDER_SETTINGS::GetLayerIsHighContrast, "Return information whether the queried layer is marked as high-contrast.\n\n \n True if the queried layer is marked as active.\n\nC++: KIGFX::RENDER_SETTINGS::GetLayerIsHighContrast(int) const --> bool", pybind11::arg("aLayerId"));
		cl.def("GetHighContrastLayers", (const int (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::GetHighContrastLayers, "Returns the set of currently high-contrast layers.\n\nC++: KIGFX::RENDER_SETTINGS::GetHighContrastLayers() const --> const int");
		cl.def("GetPrimaryHighContrastLayer", (enum PCB_LAYER_ID (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::GetPrimaryHighContrastLayer, "Return the board layer which is in high-contrast mode.\n\n There should only be one board layer which is high-contrast at any given time, although\n there might be many high-contrast synthetic (GAL) layers.\n\nC++: KIGFX::RENDER_SETTINGS::GetPrimaryHighContrastLayer() const --> enum PCB_LAYER_ID");
		cl.def("GetActiveLayer", (enum PCB_LAYER_ID (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::GetActiveLayer, "C++: KIGFX::RENDER_SETTINGS::GetActiveLayer() const --> enum PCB_LAYER_ID");
		cl.def("SetActiveLayer", (void (KIGFX::RENDER_SETTINGS::*)(enum PCB_LAYER_ID)) &KIGFX::RENDER_SETTINGS::SetActiveLayer, "C++: KIGFX::RENDER_SETTINGS::SetActiveLayer(enum PCB_LAYER_ID) --> void", pybind11::arg("aLayer"));
		cl.def("ClearHighContrastLayers", (void (KIGFX::RENDER_SETTINGS::*)()) &KIGFX::RENDER_SETTINGS::ClearHighContrastLayers, "Clear the list of active layers.\n\nC++: KIGFX::RENDER_SETTINGS::ClearHighContrastLayers() --> void");
		cl.def("IsHighlightEnabled", (bool (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::IsHighlightEnabled, "Return current highlight setting.\n\n \n True if highlight is enabled, false otherwise.\n\nC++: KIGFX::RENDER_SETTINGS::IsHighlightEnabled() const --> bool");
		cl.def("GetHighlightNetCodes", (const int & (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::GetHighlightNetCodes, "Return the netcode of currently highlighted net.\n\n \n Netcode of currently highlighted net.\n\nC++: KIGFX::RENDER_SETTINGS::GetHighlightNetCodes() const --> const int &", pybind11::return_value_policy::automatic);
		cl.def("SetHighlight", [](KIGFX::RENDER_SETTINGS &o, bool const & a0) -> void { return o.SetHighlight(a0); }, "", pybind11::arg("aEnabled"));
		cl.def("SetHighlight", [](KIGFX::RENDER_SETTINGS &o, bool const & a0, int const & a1) -> void { return o.SetHighlight(a0, a1); }, "", pybind11::arg("aEnabled"), pybind11::arg("aNetcode"));
		cl.def("SetHighlight", (void (KIGFX::RENDER_SETTINGS::*)(bool, int, bool)) &KIGFX::RENDER_SETTINGS::SetHighlight, "Turns on/off highlighting.\n\n It may be done for the active layer or the specified net(s)..\n\n \n tells if highlighting should be enabled.\n \n\n is optional and if specified, turns on highlighting only for the net with\n                 number given as the parameter.\n\nC++: KIGFX::RENDER_SETTINGS::SetHighlight(bool, int, bool) --> void", pybind11::arg("aEnabled"), pybind11::arg("aNetcode"), pybind11::arg("aMulti"));
		cl.def("SetHighContrast", (void (KIGFX::RENDER_SETTINGS::*)(bool)) &KIGFX::RENDER_SETTINGS::SetHighContrast, "Turns on/off high contrast display mode.\n\nC++: KIGFX::RENDER_SETTINGS::SetHighContrast(bool) --> void", pybind11::arg("aEnabled"));
		cl.def("GetHighContrast", (bool (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::GetHighContrast, "C++: KIGFX::RENDER_SETTINGS::GetHighContrast() const --> bool");
		cl.def("GetColor", (class KIGFX::COLOR4D (KIGFX::RENDER_SETTINGS::*)(const class KIGFX::VIEW_ITEM *, int) const) &KIGFX::RENDER_SETTINGS::GetColor, "Returns the color that should be used to draw the specific VIEW_ITEM on the specific layer\n using currently used render settings.\n\n \n is the VIEW_ITEM.\n \n\n is the layer.\n \n\n The color.\n\nC++: KIGFX::RENDER_SETTINGS::GetColor(const class KIGFX::VIEW_ITEM *, int) const --> class KIGFX::COLOR4D", pybind11::arg("aItem"), pybind11::arg("aLayer"));
		cl.def("GetDrawingSheetLineWidth", (float (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::GetDrawingSheetLineWidth, "C++: KIGFX::RENDER_SETTINGS::GetDrawingSheetLineWidth() const --> float");
		cl.def("GetDefaultPenWidth", (int (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::GetDefaultPenWidth, "C++: KIGFX::RENDER_SETTINGS::GetDefaultPenWidth() const --> int");
		cl.def("SetDefaultPenWidth", (void (KIGFX::RENDER_SETTINGS::*)(int)) &KIGFX::RENDER_SETTINGS::SetDefaultPenWidth, "C++: KIGFX::RENDER_SETTINGS::SetDefaultPenWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("GetMinPenWidth", (int (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::GetMinPenWidth, "C++: KIGFX::RENDER_SETTINGS::GetMinPenWidth() const --> int");
		cl.def("SetMinPenWidth", (void (KIGFX::RENDER_SETTINGS::*)(int)) &KIGFX::RENDER_SETTINGS::SetMinPenWidth, "C++: KIGFX::RENDER_SETTINGS::SetMinPenWidth(int) --> void", pybind11::arg("aWidth"));
		cl.def("GetShowPageLimits", (bool (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::GetShowPageLimits, "C++: KIGFX::RENDER_SETTINGS::GetShowPageLimits() const --> bool");
		cl.def("SetShowPageLimits", (void (KIGFX::RENDER_SETTINGS::*)(bool)) &KIGFX::RENDER_SETTINGS::SetShowPageLimits, "C++: KIGFX::RENDER_SETTINGS::SetShowPageLimits(bool) --> void", pybind11::arg("aDraw"));
		cl.def("IsPrinting", (bool (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::IsPrinting, "C++: KIGFX::RENDER_SETTINGS::IsPrinting() const --> bool");
		cl.def("SetIsPrinting", (void (KIGFX::RENDER_SETTINGS::*)(bool)) &KIGFX::RENDER_SETTINGS::SetIsPrinting, "C++: KIGFX::RENDER_SETTINGS::SetIsPrinting(bool) --> void", pybind11::arg("isPrinting"));
		cl.def("GetBackgroundColor", (const class KIGFX::COLOR4D & (KIGFX::RENDER_SETTINGS::*)()) &KIGFX::RENDER_SETTINGS::GetBackgroundColor, "Return current background color settings.\n\nC++: KIGFX::RENDER_SETTINGS::GetBackgroundColor() --> const class KIGFX::COLOR4D &", pybind11::return_value_policy::automatic);
		cl.def("SetBackgroundColor", (void (KIGFX::RENDER_SETTINGS::*)(const class KIGFX::COLOR4D &)) &KIGFX::RENDER_SETTINGS::SetBackgroundColor, "Set the background color.\n\nC++: KIGFX::RENDER_SETTINGS::SetBackgroundColor(const class KIGFX::COLOR4D &) --> void", pybind11::arg("aColor"));
		cl.def("GetGridColor", (const class KIGFX::COLOR4D & (KIGFX::RENDER_SETTINGS::*)()) &KIGFX::RENDER_SETTINGS::GetGridColor, "Return current grid color settings.\n\nC++: KIGFX::RENDER_SETTINGS::GetGridColor() --> const class KIGFX::COLOR4D &", pybind11::return_value_policy::automatic);
		cl.def("GetCursorColor", (const class KIGFX::COLOR4D & (KIGFX::RENDER_SETTINGS::*)()) &KIGFX::RENDER_SETTINGS::GetCursorColor, "Return current cursor color settings.\n\nC++: KIGFX::RENDER_SETTINGS::GetCursorColor() --> const class KIGFX::COLOR4D &", pybind11::return_value_policy::automatic);
		cl.def("GetLayerColor", (const class KIGFX::COLOR4D & (KIGFX::RENDER_SETTINGS::*)(int) const) &KIGFX::RENDER_SETTINGS::GetLayerColor, "Return the color used to draw a layer.\n\n \n is the layer number.\n\nC++: KIGFX::RENDER_SETTINGS::GetLayerColor(int) const --> const class KIGFX::COLOR4D &", pybind11::return_value_policy::automatic, pybind11::arg("aLayer"));
		cl.def("SetLayerColor", (void (KIGFX::RENDER_SETTINGS::*)(int, const class KIGFX::COLOR4D &)) &KIGFX::RENDER_SETTINGS::SetLayerColor, "Change the color used to draw a layer.\n\n \n is the layer number.\n \n\n is the new color.\n\nC++: KIGFX::RENDER_SETTINGS::SetLayerColor(int, const class KIGFX::COLOR4D &) --> void", pybind11::arg("aLayer"), pybind11::arg("aColor"));
		cl.def("IsBackgroundDark", (bool (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::IsBackgroundDark, "C++: KIGFX::RENDER_SETTINGS::IsBackgroundDark() const --> bool");
		cl.def("SetOutlineWidth", (void (KIGFX::RENDER_SETTINGS::*)(float)) &KIGFX::RENDER_SETTINGS::SetOutlineWidth, "Set line width used for drawing outlines.\n\n \n is the new width.\n\nC++: KIGFX::RENDER_SETTINGS::SetOutlineWidth(float) --> void", pybind11::arg("aWidth"));
		cl.def("SetHighlightFactor", (void (KIGFX::RENDER_SETTINGS::*)(float)) &KIGFX::RENDER_SETTINGS::SetHighlightFactor, "C++: KIGFX::RENDER_SETTINGS::SetHighlightFactor(float) --> void", pybind11::arg("aFactor"));
		cl.def("SetSelectFactor", (void (KIGFX::RENDER_SETTINGS::*)(float)) &KIGFX::RENDER_SETTINGS::SetSelectFactor, "C++: KIGFX::RENDER_SETTINGS::SetSelectFactor(float) --> void", pybind11::arg("aFactor"));
		cl.def("SetHighContrastFactor", (void (KIGFX::RENDER_SETTINGS::*)(float)) &KIGFX::RENDER_SETTINGS::SetHighContrastFactor, "C++: KIGFX::RENDER_SETTINGS::SetHighContrastFactor(float) --> void", pybind11::arg("aFactor"));
		cl.def("GetPrintDC", (class wxDC * (KIGFX::RENDER_SETTINGS::*)() const) &KIGFX::RENDER_SETTINGS::GetPrintDC, "C++: KIGFX::RENDER_SETTINGS::GetPrintDC() const --> class wxDC *", pybind11::return_value_policy::automatic);
		cl.def("SetPrintDC", (void (KIGFX::RENDER_SETTINGS::*)(class wxDC *)) &KIGFX::RENDER_SETTINGS::SetPrintDC, "C++: KIGFX::RENDER_SETTINGS::SetPrintDC(class wxDC *) --> void", pybind11::arg("aDC"));
		cl.def("assign", (class KIGFX::RENDER_SETTINGS & (KIGFX::RENDER_SETTINGS::*)(const class KIGFX::RENDER_SETTINGS &)) &KIGFX::RENDER_SETTINGS::operator=, "C++: KIGFX::RENDER_SETTINGS::operator=(const class KIGFX::RENDER_SETTINGS &) --> class KIGFX::RENDER_SETTINGS &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
