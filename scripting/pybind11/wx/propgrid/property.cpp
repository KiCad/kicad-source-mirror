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
#include <wx/propgrid/property.h> // wxPGAttributeStorage
#include <wx/propgrid/property.h> // wxPGCell
#include <wx/propgrid/property.h> // wxPGCellData
#include <wx/propgrid/property.h> // wxPGCellRenderer
#include <wx/propgrid/property.h> // wxPGChoiceEntry
#include <wx/propgrid/property.h> // wxPGChoices
#include <wx/propgrid/property.h> // wxPGChoicesData
#include <wx/propgrid/property.h> // wxPGDefaultRenderer
#include <wx/propgrid/property.h> // wxPGPaintData
#include <wx/propgrid/property.h> // wxPGProperty
#include <wx/propgrid/property.h> // wxPGPropertyFlags

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxPGDefaultRenderer file:wx/propgrid/property.h line:173
struct PyCallBack_wxPGDefaultRenderer : public wxPGDefaultRenderer {
	using wxPGDefaultRenderer::wxPGDefaultRenderer;

	class wxSize GetImageSize(const class wxPGProperty * a0, int a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGDefaultRenderer *>(this), "GetImageSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxPGDefaultRenderer::GetImageSize(a0, a1, a2);
	}
	void DrawCaptionSelectionRect(class wxDC & a0, int a1, int a2, int a3, int a4) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGDefaultRenderer *>(this), "DrawCaptionSelectionRect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPGCellRenderer::DrawCaptionSelectionRect(a0, a1, a2, a3, a4);
	}
};

// wxPGCell file:wx/propgrid/property.h line:227
struct PyCallBack_wxPGCell : public wxPGCell {
	using wxPGCell::wxPGCell;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGCell *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxObject::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGCell *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGCell *>(this), "CloneRefData");
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

// wxPGChoiceEntry file:wx/propgrid/property.h line:734
struct PyCallBack_wxPGChoiceEntry : public wxPGChoiceEntry {
	using wxPGChoiceEntry::wxPGChoiceEntry;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGChoiceEntry *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxObject::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGChoiceEntry *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGChoiceEntry *>(this), "CloneRefData");
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

void bind_wx_propgrid_property(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPGCellRenderer file:wx/propgrid/property.h line:65
		pybind11::class_<wxPGCellRenderer, std::shared_ptr<wxPGCellRenderer>, wxRefCounter> cl(M(""), "wxPGCellRenderer", "Base class for wxPropertyGrid cell renderers.");
		cl.def("GetImageSize", (class wxSize (wxPGCellRenderer::*)(const class wxPGProperty *, int, int) const) &wxPGCellRenderer::GetImageSize, "Returns size of the image in front of the editable area.\n        \n\n\n        If property is NULL, then this call is for a custom value. In that case\n        the item is index to wxPropertyGrid's custom values.\n\nC++: wxPGCellRenderer::GetImageSize(const class wxPGProperty *, int, int) const --> class wxSize", pybind11::arg("property"), pybind11::arg("column"), pybind11::arg("item"));
		cl.def("DrawCaptionSelectionRect", (void (wxPGCellRenderer::*)(class wxDC &, int, int, int, int) const) &wxPGCellRenderer::DrawCaptionSelectionRect, "Paints property category selection rectangle.\n\nC++: wxPGCellRenderer::DrawCaptionSelectionRect(class wxDC &, int, int, int, int) const --> void", pybind11::arg("dc"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("DrawText", (void (wxPGCellRenderer::*)(class wxDC &, const class wxRect &, int, const class wxString &) const) &wxPGCellRenderer::DrawText, "Utility to draw vertically centered text.\n\nC++: wxPGCellRenderer::DrawText(class wxDC &, const class wxRect &, int, const class wxString &) const --> void", pybind11::arg("dc"), pybind11::arg("rect"), pybind11::arg("imageWidth"), pybind11::arg("text"));
		cl.def("PreDrawCell", (int (wxPGCellRenderer::*)(class wxDC &, const class wxRect &, const class wxPGCell &, int) const) &wxPGCellRenderer::PreDrawCell, "Utility to render cell bitmap and set text colour plus bg brush\n        colour.\n\n        \n Returns image width, which, for instance, can be passed to\n                DrawText.\n\nC++: wxPGCellRenderer::PreDrawCell(class wxDC &, const class wxRect &, const class wxPGCell &, int) const --> int", pybind11::arg("dc"), pybind11::arg("rect"), pybind11::arg("cell"), pybind11::arg("flags"));
	}
	{ // wxPGDefaultRenderer file:wx/propgrid/property.h line:173
		pybind11::class_<wxPGDefaultRenderer, std::shared_ptr<wxPGDefaultRenderer>, PyCallBack_wxPGDefaultRenderer, wxPGCellRenderer> cl(M(""), "wxPGDefaultRenderer", "Default cell renderer, that can handles the common\n    scenarios.");
		cl.def( pybind11::init( [](){ return new wxPGDefaultRenderer(); }, [](){ return new PyCallBack_wxPGDefaultRenderer(); } ) );
		cl.def("GetImageSize", (class wxSize (wxPGDefaultRenderer::*)(const class wxPGProperty *, int, int) const) &wxPGDefaultRenderer::GetImageSize, "C++: wxPGDefaultRenderer::GetImageSize(const class wxPGProperty *, int, int) const --> class wxSize", pybind11::arg("property"), pybind11::arg("column"), pybind11::arg("item"));
	}
	{ // wxPGCellData file:wx/propgrid/property.h line:192
		pybind11::class_<wxPGCellData, wxPGCellData*, wxRefCounter> cl(M(""), "wxPGCellData", "");
		cl.def( pybind11::init( [](){ return new wxPGCellData(); } ) );
		cl.def("SetText", (void (wxPGCellData::*)(const class wxString &)) &wxPGCellData::SetText, "C++: wxPGCellData::SetText(const class wxString &) --> void", pybind11::arg("text"));
		cl.def("SetBitmap", (void (wxPGCellData::*)(const class wxBitmap &)) &wxPGCellData::SetBitmap, "C++: wxPGCellData::SetBitmap(const class wxBitmap &) --> void", pybind11::arg("bitmap"));
		cl.def("SetFgCol", (void (wxPGCellData::*)(const class wxColour &)) &wxPGCellData::SetFgCol, "C++: wxPGCellData::SetFgCol(const class wxColour &) --> void", pybind11::arg("col"));
		cl.def("SetBgCol", (void (wxPGCellData::*)(const class wxColour &)) &wxPGCellData::SetBgCol, "C++: wxPGCellData::SetBgCol(const class wxColour &) --> void", pybind11::arg("col"));
		cl.def("SetFont", (void (wxPGCellData::*)(const class wxFont &)) &wxPGCellData::SetFont, "C++: wxPGCellData::SetFont(const class wxFont &) --> void", pybind11::arg("font"));
	}
	{ // wxPGCell file:wx/propgrid/property.h line:227
		pybind11::class_<wxPGCell, std::shared_ptr<wxPGCell>, PyCallBack_wxPGCell, wxObject> cl(M(""), "wxPGCell", "Base class for wxPropertyGrid cell information.");
		cl.def( pybind11::init( [](){ return new wxPGCell(); }, [](){ return new PyCallBack_wxPGCell(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxPGCell const &o){ return new PyCallBack_wxPGCell(o); } ) );
		cl.def( pybind11::init( [](wxPGCell const &o){ return new wxPGCell(o); } ) );
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxPGCell(a0); }, [](const class wxString & a0){ return new PyCallBack_wxPGCell(a0); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const class wxBitmap & a1){ return new wxPGCell(a0, a1); }, [](const class wxString & a0, const class wxBitmap & a1){ return new PyCallBack_wxPGCell(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const class wxBitmap & a1, const class wxColour & a2){ return new wxPGCell(a0, a1, a2); }, [](const class wxString & a0, const class wxBitmap & a1, const class wxColour & a2){ return new PyCallBack_wxPGCell(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<const class wxString &, const class wxBitmap &, const class wxColour &, const class wxColour &>(), pybind11::arg("text"), pybind11::arg("bitmap"), pybind11::arg("fgCol"), pybind11::arg("bgCol") );

		cl.def("GetData", (class wxPGCellData * (wxPGCell::*)()) &wxPGCell::GetData, "C++: wxPGCell::GetData() --> class wxPGCellData *", pybind11::return_value_policy::automatic);
		cl.def("HasText", (bool (wxPGCell::*)() const) &wxPGCell::HasText, "C++: wxPGCell::HasText() const --> bool");
		cl.def("SetEmptyData", (void (wxPGCell::*)()) &wxPGCell::SetEmptyData, "Sets empty but valid data to this cell object.\n\nC++: wxPGCell::SetEmptyData() --> void");
		cl.def("MergeFrom", (void (wxPGCell::*)(const class wxPGCell &)) &wxPGCell::MergeFrom, "Merges valid data from srcCell into this.\n\nC++: wxPGCell::MergeFrom(const class wxPGCell &) --> void", pybind11::arg("srcCell"));
		cl.def("SetText", (void (wxPGCell::*)(const class wxString &)) &wxPGCell::SetText, "C++: wxPGCell::SetText(const class wxString &) --> void", pybind11::arg("text"));
		cl.def("SetBitmap", (void (wxPGCell::*)(const class wxBitmap &)) &wxPGCell::SetBitmap, "C++: wxPGCell::SetBitmap(const class wxBitmap &) --> void", pybind11::arg("bitmap"));
		cl.def("SetFgCol", (void (wxPGCell::*)(const class wxColour &)) &wxPGCell::SetFgCol, "C++: wxPGCell::SetFgCol(const class wxColour &) --> void", pybind11::arg("col"));
		cl.def("SetFont", (void (wxPGCell::*)(const class wxFont &)) &wxPGCell::SetFont, "Sets font of the cell.\n\n        \n Because wxPropertyGrid does not support rows of\n                 different height, it makes little sense to change\n                 size of the font. Therefore it is recommended\n                 to use return value of wxPropertyGrid::GetFont()\n                 or wxPropertyGrid::GetCaptionFont() as a basis\n                 for the font that, after modifications, is passed\n                 to this member function.\n\nC++: wxPGCell::SetFont(const class wxFont &) --> void", pybind11::arg("font"));
		cl.def("SetBgCol", (void (wxPGCell::*)(const class wxColour &)) &wxPGCell::SetBgCol, "C++: wxPGCell::SetBgCol(const class wxColour &) --> void", pybind11::arg("col"));
		cl.def("GetText", (const class wxString & (wxPGCell::*)() const) &wxPGCell::GetText, "C++: wxPGCell::GetText() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetBitmap", (const class wxBitmap & (wxPGCell::*)() const) &wxPGCell::GetBitmap, "C++: wxPGCell::GetBitmap() const --> const class wxBitmap &", pybind11::return_value_policy::automatic);
		cl.def("GetFgCol", (const class wxColour & (wxPGCell::*)() const) &wxPGCell::GetFgCol, "C++: wxPGCell::GetFgCol() const --> const class wxColour &", pybind11::return_value_policy::automatic);
		cl.def("GetFont", (const class wxFont & (wxPGCell::*)() const) &wxPGCell::GetFont, "Returns font of the cell. If no specific font is set for this\n        cell, then the font will be invalid.\n\nC++: wxPGCell::GetFont() const --> const class wxFont &", pybind11::return_value_policy::automatic);
		cl.def("GetBgCol", (const class wxColour & (wxPGCell::*)() const) &wxPGCell::GetBgCol, "C++: wxPGCell::GetBgCol() const --> const class wxColour &", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxPGCell & (wxPGCell::*)(const class wxPGCell &)) &wxPGCell::operator=, "C++: wxPGCell::operator=(const class wxPGCell &) --> class wxPGCell &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("IsInvalid", (bool (wxPGCell::*)() const) &wxPGCell::IsInvalid, "C++: wxPGCell::IsInvalid() const --> bool");
	}
	{ // wxPGAttributeStorage file:wx/propgrid/property.h line:329
		pybind11::class_<wxPGAttributeStorage, std::shared_ptr<wxPGAttributeStorage>> cl(M(""), "wxPGAttributeStorage", "wxPGAttributeStorage is somewhat optimized storage for\n      key=variant pairs (ie. a map).");
		cl.def( pybind11::init( [](){ return new wxPGAttributeStorage(); } ) );
		cl.def("Set", (void (wxPGAttributeStorage::*)(const class wxString &, const class wxVariant &)) &wxPGAttributeStorage::Set, "C++: wxPGAttributeStorage::Set(const class wxString &, const class wxVariant &) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("GetCount", (unsigned int (wxPGAttributeStorage::*)() const) &wxPGAttributeStorage::GetCount, "C++: wxPGAttributeStorage::GetCount() const --> unsigned int");
		cl.def("FindValue", (class wxVariant (wxPGAttributeStorage::*)(const class wxString &) const) &wxPGAttributeStorage::FindValue, "C++: wxPGAttributeStorage::FindValue(const class wxString &) const --> class wxVariant", pybind11::arg("name"));
		cl.def("StartIteration", (class wxPGHashMapS2P_wxImplementation_HashTable::const_iterator (wxPGAttributeStorage::*)() const) &wxPGAttributeStorage::StartIteration, "C++: wxPGAttributeStorage::StartIteration() const --> class wxPGHashMapS2P_wxImplementation_HashTable::const_iterator");
		cl.def("GetNext", (bool (wxPGAttributeStorage::*)(class wxPGHashMapS2P_wxImplementation_HashTable::const_iterator &, class wxVariant &) const) &wxPGAttributeStorage::GetNext, "C++: wxPGAttributeStorage::GetNext(class wxPGHashMapS2P_wxImplementation_HashTable::const_iterator &, class wxVariant &) const --> bool", pybind11::arg("it"), pybind11::arg("variant"));
	}
	// wxPGPropertyFlags file:wx/propgrid/property.h line:378
	pybind11::enum_<wxPGPropertyFlags>(M(""), "wxPGPropertyFlags", pybind11::arithmetic(), "@{")
		.value("wxPG_PROP_MODIFIED", wxPG_PROP_MODIFIED)
		.value("wxPG_PROP_DISABLED", wxPG_PROP_DISABLED)
		.value("wxPG_PROP_HIDDEN", wxPG_PROP_HIDDEN)
		.value("wxPG_PROP_CUSTOMIMAGE", wxPG_PROP_CUSTOMIMAGE)
		.value("wxPG_PROP_NOEDITOR", wxPG_PROP_NOEDITOR)
		.value("wxPG_PROP_COLLAPSED", wxPG_PROP_COLLAPSED)
		.value("wxPG_PROP_INVALID_VALUE", wxPG_PROP_INVALID_VALUE)
		.value("wxPG_PROP_WAS_MODIFIED", wxPG_PROP_WAS_MODIFIED)
		.value("wxPG_PROP_AGGREGATE", wxPG_PROP_AGGREGATE)
		.value("wxPG_PROP_CHILDREN_ARE_COPIES", wxPG_PROP_CHILDREN_ARE_COPIES)
		.value("wxPG_PROP_PROPERTY", wxPG_PROP_PROPERTY)
		.value("wxPG_PROP_CATEGORY", wxPG_PROP_CATEGORY)
		.value("wxPG_PROP_MISC_PARENT", wxPG_PROP_MISC_PARENT)
		.value("wxPG_PROP_READONLY", wxPG_PROP_READONLY)
		.value("wxPG_PROP_COMPOSED_VALUE", wxPG_PROP_COMPOSED_VALUE)
		.value("wxPG_PROP_USES_COMMON_VALUE", wxPG_PROP_USES_COMMON_VALUE)
		.value("wxPG_PROP_AUTO_UNSPECIFIED", wxPG_PROP_AUTO_UNSPECIFIED)
		.value("wxPG_PROP_CLASS_SPECIFIC_1", wxPG_PROP_CLASS_SPECIFIC_1)
		.value("wxPG_PROP_CLASS_SPECIFIC_2", wxPG_PROP_CLASS_SPECIFIC_2)
		.value("wxPG_PROP_BEING_DELETED", wxPG_PROP_BEING_DELETED)
		.export_values();

;

	{ // wxPGChoiceEntry file:wx/propgrid/property.h line:734
		pybind11::class_<wxPGChoiceEntry, std::shared_ptr<wxPGChoiceEntry>, PyCallBack_wxPGChoiceEntry, wxPGCell> cl(M(""), "wxPGChoiceEntry", "Data of a single wxPGChoices choice.");
		cl.def( pybind11::init( [](){ return new wxPGChoiceEntry(); }, [](){ return new PyCallBack_wxPGChoiceEntry(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxPGChoiceEntry const &o){ return new PyCallBack_wxPGChoiceEntry(o); } ) );
		cl.def( pybind11::init( [](wxPGChoiceEntry const &o){ return new wxPGChoiceEntry(o); } ) );
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxPGChoiceEntry(a0); }, [](const class wxString & a0){ return new PyCallBack_wxPGChoiceEntry(a0); } ), "doc");
		cl.def( pybind11::init<const class wxString &, int>(), pybind11::arg("label"), pybind11::arg("value") );

		cl.def("SetValue", (void (wxPGChoiceEntry::*)(int)) &wxPGChoiceEntry::SetValue, "C++: wxPGChoiceEntry::SetValue(int) --> void", pybind11::arg("value"));
		cl.def("GetValue", (int (wxPGChoiceEntry::*)() const) &wxPGChoiceEntry::GetValue, "C++: wxPGChoiceEntry::GetValue() const --> int");
		cl.def("assign", (class wxPGChoiceEntry & (wxPGChoiceEntry::*)(const class wxPGChoiceEntry &)) &wxPGChoiceEntry::operator=, "C++: wxPGChoiceEntry::operator=(const class wxPGChoiceEntry &) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
	}
	{ // wxPGChoicesData file:wx/propgrid/property.h line:772
		pybind11::class_<wxPGChoicesData, wxPGChoicesData*, wxRefCounter> cl(M(""), "wxPGChoicesData", "");
		cl.def( pybind11::init( [](){ return new wxPGChoicesData(); } ) );
		cl.def( pybind11::init( [](wxPGChoicesData const &o){ return new wxPGChoicesData(o); } ) );
		cl.def("CopyDataFrom", (void (wxPGChoicesData::*)(class wxPGChoicesData *)) &wxPGChoicesData::CopyDataFrom, "C++: wxPGChoicesData::CopyDataFrom(class wxPGChoicesData *) --> void", pybind11::arg("data"));
		cl.def("Insert", (class wxPGChoiceEntry & (wxPGChoicesData::*)(int, const class wxPGChoiceEntry &)) &wxPGChoicesData::Insert, "C++: wxPGChoicesData::Insert(int, const class wxPGChoiceEntry &) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("index"), pybind11::arg("item"));
		cl.def("Clear", (void (wxPGChoicesData::*)()) &wxPGChoicesData::Clear, "C++: wxPGChoicesData::Clear() --> void");
		cl.def("GetCount", (unsigned int (wxPGChoicesData::*)() const) &wxPGChoicesData::GetCount, "C++: wxPGChoicesData::GetCount() const --> unsigned int");
		cl.def("Item", (class wxPGChoiceEntry & (wxPGChoicesData::*)(unsigned int)) &wxPGChoicesData::Item, "C++: wxPGChoicesData::Item(unsigned int) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("assign", (class wxPGChoicesData & (wxPGChoicesData::*)(const class wxPGChoicesData &)) &wxPGChoicesData::operator=, "C++: wxPGChoicesData::operator=(const class wxPGChoicesData &) --> class wxPGChoicesData &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPGChoices file:wx/propgrid/property.h line:829
		pybind11::class_<wxPGChoices, std::shared_ptr<wxPGChoices>> cl(M(""), "wxPGChoices", "Helper class for managing choices of wxPropertyGrid properties.\n    Each entry can have label, value, bitmap, text colour, and background\n    colour.\n\n    wxPGChoices uses reference counting, similar to other wxWidgets classes.\n    This means that assignment operator and copy constructor only copy the\n    reference and not the actual data. Use Copy() member function to create a\n    real copy.\n\n    \n If you do not specify value for entry, index is used.\n\n    {wxpropgrid}\n    \n");
		cl.def( pybind11::init( [](){ return new wxPGChoices(); } ) );
		cl.def( pybind11::init( [](wxPGChoices const &o){ return new wxPGChoices(o); } ) );
		cl.def( pybind11::init( [](const class wxArrayString & a0){ return new wxPGChoices(a0); } ), "doc" , pybind11::arg("labels"));
		cl.def( pybind11::init<const class wxArrayString &, const class wxArrayInt &>(), pybind11::arg("labels"), pybind11::arg("values") );

		cl.def( pybind11::init<class wxPGChoicesData *>(), pybind11::arg("data") );

		cl.def("Add", [](wxPGChoices &o, const class wxArrayString & a0) -> void { return o.Add(a0); }, "", pybind11::arg("arr"));
		cl.def("Add", (void (wxPGChoices::*)(const class wxArrayString &, const class wxArrayInt &)) &wxPGChoices::Add, "Version that works with wxArrayString and wxArrayInt. \n\nC++: wxPGChoices::Add(const class wxArrayString &, const class wxArrayInt &) --> void", pybind11::arg("arr"), pybind11::arg("arrint"));
		cl.def("Add", [](wxPGChoices &o, const class wxString & a0) -> wxPGChoiceEntry & { return o.Add(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("label"));
		cl.def("Add", (class wxPGChoiceEntry & (wxPGChoices::*)(const class wxString &, int)) &wxPGChoices::Add, "Adds a single choice.\n\n        \n\n            Label for added choice.\n\n        \n\n            Value for added choice. If unspecified, index is used.\n\nC++: wxPGChoices::Add(const class wxString &, int) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("label"), pybind11::arg("value"));
		cl.def("Add", [](wxPGChoices &o, const class wxString & a0, const class wxBitmap & a1) -> wxPGChoiceEntry & { return o.Add(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("label"), pybind11::arg("bitmap"));
		cl.def("Add", (class wxPGChoiceEntry & (wxPGChoices::*)(const class wxString &, const class wxBitmap &, int)) &wxPGChoices::Add, "Adds a single item, with bitmap. \n\nC++: wxPGChoices::Add(const class wxString &, const class wxBitmap &, int) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("label"), pybind11::arg("bitmap"), pybind11::arg("value"));
		cl.def("Add", (class wxPGChoiceEntry & (wxPGChoices::*)(const class wxPGChoiceEntry &)) &wxPGChoices::Add, "Adds a single item with full entry information. \n\nC++: wxPGChoices::Add(const class wxPGChoiceEntry &) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("entry"));
		cl.def("AddAsSorted", [](wxPGChoices &o, const class wxString & a0) -> wxPGChoiceEntry & { return o.AddAsSorted(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("label"));
		cl.def("AddAsSorted", (class wxPGChoiceEntry & (wxPGChoices::*)(const class wxString &, int)) &wxPGChoices::AddAsSorted, "Adds single item. \n\nC++: wxPGChoices::AddAsSorted(const class wxString &, int) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("label"), pybind11::arg("value"));
		cl.def("Assign", (void (wxPGChoices::*)(const class wxPGChoices &)) &wxPGChoices::Assign, "Assigns choices data, using reference counting. To create a real copy,\n        use Copy() member function instead.\n\nC++: wxPGChoices::Assign(const class wxPGChoices &) --> void", pybind11::arg("a"));
		cl.def("AssignData", (void (wxPGChoices::*)(class wxPGChoicesData *)) &wxPGChoices::AssignData, "C++: wxPGChoices::AssignData(class wxPGChoicesData *) --> void", pybind11::arg("data"));
		cl.def("Clear", (void (wxPGChoices::*)()) &wxPGChoices::Clear, "Delete all choices. \n\nC++: wxPGChoices::Clear() --> void");
		cl.def("Copy", (class wxPGChoices (wxPGChoices::*)() const) &wxPGChoices::Copy, "Returns a real copy of the choices.\n\nC++: wxPGChoices::Copy() const --> class wxPGChoices");
		cl.def("EnsureData", (void (wxPGChoices::*)()) &wxPGChoices::EnsureData, "C++: wxPGChoices::EnsureData() --> void");
		cl.def("GetId", (void * (wxPGChoices::*)() const) &wxPGChoices::GetId, "Gets a unsigned number identifying this list. \n\nC++: wxPGChoices::GetId() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("GetLabel", (const class wxString & (wxPGChoices::*)(unsigned int) const) &wxPGChoices::GetLabel, "C++: wxPGChoices::GetLabel(unsigned int) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ind"));
		cl.def("GetCount", (unsigned int (wxPGChoices::*)() const) &wxPGChoices::GetCount, "C++: wxPGChoices::GetCount() const --> unsigned int");
		cl.def("GetValue", (int (wxPGChoices::*)(unsigned int) const) &wxPGChoices::GetValue, "C++: wxPGChoices::GetValue(unsigned int) const --> int", pybind11::arg("ind"));
		cl.def("GetValuesForStrings", (class wxArrayInt (wxPGChoices::*)(const class wxArrayString &) const) &wxPGChoices::GetValuesForStrings, "Returns array of values matching the given strings. Unmatching strings\n        result in wxPG_INVALID_VALUE entry in array.\n\nC++: wxPGChoices::GetValuesForStrings(const class wxArrayString &) const --> class wxArrayInt", pybind11::arg("strings"));
		cl.def("GetIndicesForStrings", [](wxPGChoices const &o, const class wxArrayString & a0) -> wxArrayInt { return o.GetIndicesForStrings(a0); }, "", pybind11::arg("strings"));
		cl.def("GetIndicesForStrings", (class wxArrayInt (wxPGChoices::*)(const class wxArrayString &, class wxArrayString *) const) &wxPGChoices::GetIndicesForStrings, "Returns array of indices matching given strings. Unmatching strings\n        are added to 'unmatched', if not NULL.\n\nC++: wxPGChoices::GetIndicesForStrings(const class wxArrayString &, class wxArrayString *) const --> class wxArrayInt", pybind11::arg("strings"), pybind11::arg("unmatched"));
		cl.def("Index", (int (wxPGChoices::*)(const class wxString &) const) &wxPGChoices::Index, "C++: wxPGChoices::Index(const class wxString &) const --> int", pybind11::arg("str"));
		cl.def("Index", (int (wxPGChoices::*)(int) const) &wxPGChoices::Index, "C++: wxPGChoices::Index(int) const --> int", pybind11::arg("val"));
		cl.def("Insert", [](wxPGChoices &o, const class wxString & a0, int const & a1) -> wxPGChoiceEntry & { return o.Insert(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("label"), pybind11::arg("index"));
		cl.def("Insert", (class wxPGChoiceEntry & (wxPGChoices::*)(const class wxString &, int, int)) &wxPGChoices::Insert, "Inserts single item. \n\nC++: wxPGChoices::Insert(const class wxString &, int, int) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("label"), pybind11::arg("index"), pybind11::arg("value"));
		cl.def("Insert", (class wxPGChoiceEntry & (wxPGChoices::*)(const class wxPGChoiceEntry &, int)) &wxPGChoices::Insert, "Inserts a single item with full entry information. \n\nC++: wxPGChoices::Insert(const class wxPGChoiceEntry &, int) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("entry"), pybind11::arg("index"));
		cl.def("IsOk", (bool (wxPGChoices::*)() const) &wxPGChoices::IsOk, "Returns false if this is a constant empty set of choices,\n        which should not be modified.\n\nC++: wxPGChoices::IsOk() const --> bool");
		cl.def("Item", (class wxPGChoiceEntry & (wxPGChoices::*)(unsigned int)) &wxPGChoices::Item, "C++: wxPGChoices::Item(unsigned int) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("RemoveAt", [](wxPGChoices &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("nIndex"));
		cl.def("RemoveAt", (void (wxPGChoices::*)(unsigned long, unsigned long)) &wxPGChoices::RemoveAt, "Removes count items starting at position nIndex. \n\nC++: wxPGChoices::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("nIndex"), pybind11::arg("count"));
		cl.def("Set", [](wxPGChoices &o, const class wxArrayString & a0) -> void { return o.Set(a0); }, "", pybind11::arg("labels"));
		cl.def("Set", (void (wxPGChoices::*)(const class wxArrayString &, const class wxArrayInt &)) &wxPGChoices::Set, "Version that works with wxArrayString and wxArrayInt. \n\nC++: wxPGChoices::Set(const class wxArrayString &, const class wxArrayInt &) --> void", pybind11::arg("labels"), pybind11::arg("values"));
		cl.def("AllocExclusive", (void (wxPGChoices::*)()) &wxPGChoices::AllocExclusive, "C++: wxPGChoices::AllocExclusive() --> void");
		cl.def("GetData", (class wxPGChoicesData * (wxPGChoices::*)()) &wxPGChoices::GetData, "C++: wxPGChoices::GetData() --> class wxPGChoicesData *", pybind11::return_value_policy::automatic);
		cl.def("GetDataPtr", (class wxPGChoicesData * (wxPGChoices::*)() const) &wxPGChoices::GetDataPtr, "C++: wxPGChoices::GetDataPtr() const --> class wxPGChoicesData *", pybind11::return_value_policy::automatic);
		cl.def("ExtractData", (class wxPGChoicesData * (wxPGChoices::*)()) &wxPGChoices::ExtractData, "C++: wxPGChoices::ExtractData() --> class wxPGChoicesData *", pybind11::return_value_policy::automatic);
		cl.def("GetLabels", (class wxArrayString (wxPGChoices::*)() const) &wxPGChoices::GetLabels, "C++: wxPGChoices::GetLabels() const --> class wxArrayString");
		cl.def("assign", (void (wxPGChoices::*)(const class wxPGChoices &)) &wxPGChoices::operator=, "C++: wxPGChoices::operator=(const class wxPGChoices &) --> void", pybind11::arg("a"));
		cl.def("__getitem__", (class wxPGChoiceEntry & (wxPGChoices::*)(unsigned int)) &wxPGChoices::operator[], "C++: wxPGChoices::operator[](unsigned int) --> class wxPGChoiceEntry &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
	}
}
