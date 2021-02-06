#include <sstream> // __str__
#include <view/view_item.h> // KIGFX::VIEW_ITEM
#include <view/view_item.h> // KIGFX::VIEW_UPDATE_FLAGS
#include <view/view_item.h> // KIGFX::VIEW_VISIBILITY_FLAGS

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_view_view_item(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// KIGFX::VIEW_UPDATE_FLAGS file:view/view_item.h line:50
	pybind11::enum_<KIGFX::VIEW_UPDATE_FLAGS>(M("KIGFX"), "VIEW_UPDATE_FLAGS", pybind11::arithmetic(), "Define the how severely the appearance of the item has been changed.")
		.value("NONE", KIGFX::NONE)
		.value("APPEARANCE", KIGFX::APPEARANCE)
		.value("COLOR", KIGFX::COLOR)
		.value("GEOMETRY", KIGFX::GEOMETRY)
		.value("LAYERS", KIGFX::LAYERS)
		.value("INITIAL_ADD", KIGFX::INITIAL_ADD)
		.value("REPAINT", KIGFX::REPAINT)
		.value("ALL", KIGFX::ALL)
		.export_values();

;

	// KIGFX::VIEW_VISIBILITY_FLAGS file:view/view_item.h line:64
	pybind11::enum_<KIGFX::VIEW_VISIBILITY_FLAGS>(M("KIGFX"), "VIEW_VISIBILITY_FLAGS", pybind11::arithmetic(), "Define the visibility of the item (temporarily hidden, invisible, etc).")
		.value("VISIBLE", KIGFX::VISIBLE)
		.value("HIDDEN", KIGFX::HIDDEN)
		.export_values();

;

	{ // KIGFX::VIEW_ITEM file:view/view_item.h line:81
		pybind11::class_<KIGFX::VIEW_ITEM, std::shared_ptr<KIGFX::VIEW_ITEM>, INSPECTABLE> cl(M("KIGFX"), "VIEW_ITEM", "An abstract base class for deriving all objects that can be added to a VIEW.\n\n It's role is to:\n - communicate geometry, appearance and visibility updates to the associated dynamic VIEW,\n - provide a bounding box for redraw area calculation,\n - (optional) draw the object using the #GAL API functions for #PAINTER-less implementations.\n\n VIEW_ITEM objects are never owned by a #VIEW. A single VIEW_ITEM can belong to any number of\n static VIEWs, but only one dynamic VIEW due to storage of only one VIEW reference.");
		cl.def("ViewBBox", (const int (KIGFX::VIEW_ITEM::*)() const) &KIGFX::VIEW_ITEM::ViewBBox, "Return the bounding box of the item covering all its layers.\n\n \n the current bounding box.\n\nC++: KIGFX::VIEW_ITEM::ViewBBox() const --> const int");
		cl.def("ClearViewPrivData", (void (KIGFX::VIEW_ITEM::*)()) &KIGFX::VIEW_ITEM::ClearViewPrivData, "C++: KIGFX::VIEW_ITEM::ClearViewPrivData() --> void");
	}
}
