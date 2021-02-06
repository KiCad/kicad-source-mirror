#include <core/typeinfo.h> // KICAD_T
#include <eda_item.h> // EDA_ITEM
#include <eda_item.h> // FIND_REPLACE_FLAGS
#include <eda_item.h> // SEARCH_RESULT
#include <eda_item.h> // new_clone
#include <eda_rect.h> // EDA_RECT
#include <eda_units.h> // EDA_UNITS
#include <kiid.h> // KIID
#include <kiid.h> // KIID_PATH
#include <kiid.h> // NilUuid
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
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

void bind_kiid(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // KIID file:kiid.h line:44
		pybind11::class_<KIID, std::shared_ptr<KIID>> cl(M(""), "KIID", "");
		cl.def( pybind11::init( [](){ return new KIID(); } ) );
		cl.def( pybind11::init<int>(), pybind11::arg("null") );

		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("aString") );

		cl.def( pybind11::init<unsigned int>(), pybind11::arg("aTimestamp") );

		cl.def( pybind11::init( [](KIID const &o){ return new KIID(o); } ) );
		cl.def("Clone", (void (KIID::*)(const class KIID &)) &KIID::Clone, "C++: KIID::Clone(const class KIID &) --> void", pybind11::arg("aUUID"));
		cl.def("Hash", (unsigned long (KIID::*)() const) &KIID::Hash, "C++: KIID::Hash() const --> unsigned long");
		cl.def("IsLegacyTimestamp", (bool (KIID::*)() const) &KIID::IsLegacyTimestamp, "C++: KIID::IsLegacyTimestamp() const --> bool");
		cl.def("AsLegacyTimestamp", (unsigned int (KIID::*)() const) &KIID::AsLegacyTimestamp, "C++: KIID::AsLegacyTimestamp() const --> unsigned int");
		cl.def("AsString", (class wxString (KIID::*)() const) &KIID::AsString, "C++: KIID::AsString() const --> class wxString");
		cl.def("AsLegacyTimestampString", (class wxString (KIID::*)() const) &KIID::AsLegacyTimestampString, "C++: KIID::AsLegacyTimestampString() const --> class wxString");
		cl.def_static("SniffTest", (bool (*)(const class wxString &)) &KIID::SniffTest, "C++: KIID::SniffTest(const class wxString &) --> bool", pybind11::arg("aCandidate"));
		cl.def("ConvertTimestampToUuid", (void (KIID::*)()) &KIID::ConvertTimestampToUuid, "Change an existing time stamp based UUID into a true UUID.\n\n If this is not a time stamp based UUID, then no change is made.\n\nC++: KIID::ConvertTimestampToUuid() --> void");
		cl.def("__eq__", (bool (KIID::*)(const class KIID &) const) &KIID::operator==, "C++: KIID::operator==(const class KIID &) const --> bool", pybind11::arg("rhs"));
		cl.def("__ne__", (bool (KIID::*)(const class KIID &) const) &KIID::operator!=, "C++: KIID::operator!=(const class KIID &) const --> bool", pybind11::arg("rhs"));
		cl.def("assign", (class KIID & (KIID::*)(const class KIID &)) &KIID::operator=, "C++: KIID::operator=(const class KIID &) --> class KIID &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// NilUuid() file:kiid.h line:100
	M("").def("NilUuid", (class KIID & (*)()) &NilUuid, "C++: NilUuid() --> class KIID &", pybind11::return_value_policy::automatic);

	{ // KIID_PATH file:kiid.h line:105
		pybind11::class_<KIID_PATH, std::shared_ptr<KIID_PATH>> cl(M(""), "KIID_PATH", "");
		cl.def( pybind11::init( [](){ return new KIID_PATH(); } ) );
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("aString") );

		cl.def("AsString", (class wxString (KIID_PATH::*)() const) &KIID_PATH::AsString, "C++: KIID_PATH::AsString() const --> class wxString");
		cl.def("__eq__", (bool (KIID_PATH::*)(const class KIID_PATH &) const) &KIID_PATH::operator==, "C++: KIID_PATH::operator==(const class KIID_PATH &) const --> bool", pybind11::arg("rhs"));
	}
	// SEARCH_RESULT file:eda_item.h line:40
	pybind11::enum_<SEARCH_RESULT>(M(""), "SEARCH_RESULT", "")
		.value("QUIT", SEARCH_RESULT::QUIT)
		.value("CONTINUE", SEARCH_RESULT::CONTINUE);

;

	// FIND_REPLACE_FLAGS file:eda_item.h line:50
	pybind11::enum_<FIND_REPLACE_FLAGS>(M(""), "FIND_REPLACE_FLAGS", pybind11::arithmetic(), "Additional flag values wxFindReplaceData::m_Flags")
		.value("FR_CURRENT_SHEET_ONLY", FR_CURRENT_SHEET_ONLY)
		.value("FR_SEARCH_ALL_FIELDS", FR_SEARCH_ALL_FIELDS)
		.value("FR_SEARCH_ALL_PINS", FR_SEARCH_ALL_PINS)
		.value("FR_MATCH_WILDCARD", FR_MATCH_WILDCARD)
		.value("FR_SEARCH_WRAP", FR_SEARCH_WRAP)
		.value("FR_SEARCH_REPLACE", FR_SEARCH_REPLACE)
		.value("FR_REPLACE_ITEM_FOUND", FR_REPLACE_ITEM_FOUND)
		.value("FR_REPLACE_REFERENCES", FR_REPLACE_REFERENCES)
		.export_values();

;

	{ // EDA_ITEM file:eda_item.h line:149
		pybind11::class_<EDA_ITEM, std::shared_ptr<EDA_ITEM>, KIGFX::VIEW_ITEM> cl(M(""), "EDA_ITEM", "A base class for most all the KiCad significant classes used in schematics and boards.");
		cl.def_readonly("m_Uuid", &EDA_ITEM::m_Uuid);
		cl.def("Type", (enum KICAD_T (EDA_ITEM::*)() const) &EDA_ITEM::Type, "Returns the type of object.\n\n This attribute should never be changed after a ctor sets it, so there is no public\n \"setter\" method.\n\n \n the type of object.\n\nC++: EDA_ITEM::Type() const --> enum KICAD_T");
		cl.def("GetParent", (class EDA_ITEM * (EDA_ITEM::*)() const) &EDA_ITEM::GetParent, "C++: EDA_ITEM::GetParent() const --> class EDA_ITEM *", pybind11::return_value_policy::automatic);
		cl.def("SetParent", (void (EDA_ITEM::*)(class EDA_ITEM *)) &EDA_ITEM::SetParent, "C++: EDA_ITEM::SetParent(class EDA_ITEM *) --> void", pybind11::arg("aParent"));
		cl.def("IsModified", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsModified, "C++: EDA_ITEM::IsModified() const --> bool");
		cl.def("IsNew", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsNew, "C++: EDA_ITEM::IsNew() const --> bool");
		cl.def("IsMoving", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsMoving, "C++: EDA_ITEM::IsMoving() const --> bool");
		cl.def("IsDragging", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsDragging, "C++: EDA_ITEM::IsDragging() const --> bool");
		cl.def("IsWireImage", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsWireImage, "C++: EDA_ITEM::IsWireImage() const --> bool");
		cl.def("IsSelected", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsSelected, "C++: EDA_ITEM::IsSelected() const --> bool");
		cl.def("IsEntered", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsEntered, "C++: EDA_ITEM::IsEntered() const --> bool");
		cl.def("IsResized", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsResized, "C++: EDA_ITEM::IsResized() const --> bool");
		cl.def("IsBrightened", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsBrightened, "C++: EDA_ITEM::IsBrightened() const --> bool");
		cl.def("SetWireImage", (void (EDA_ITEM::*)()) &EDA_ITEM::SetWireImage, "C++: EDA_ITEM::SetWireImage() --> void");
		cl.def("SetSelected", (void (EDA_ITEM::*)()) &EDA_ITEM::SetSelected, "C++: EDA_ITEM::SetSelected() --> void");
		cl.def("SetBrightened", (void (EDA_ITEM::*)()) &EDA_ITEM::SetBrightened, "C++: EDA_ITEM::SetBrightened() --> void");
		cl.def("ClearSelected", (void (EDA_ITEM::*)()) &EDA_ITEM::ClearSelected, "C++: EDA_ITEM::ClearSelected() --> void");
		cl.def("ClearBrightened", (void (EDA_ITEM::*)()) &EDA_ITEM::ClearBrightened, "C++: EDA_ITEM::ClearBrightened() --> void");
		cl.def("SetModified", (void (EDA_ITEM::*)()) &EDA_ITEM::SetModified, "C++: EDA_ITEM::SetModified() --> void");
		cl.def("GetState", (int (EDA_ITEM::*)(int) const) &EDA_ITEM::GetState, "C++: EDA_ITEM::GetState(int) const --> int", pybind11::arg("type"));
		cl.def("SetState", (void (EDA_ITEM::*)(int, bool)) &EDA_ITEM::SetState, "C++: EDA_ITEM::SetState(int, bool) --> void", pybind11::arg("type"), pybind11::arg("state"));
		cl.def("GetStatus", (unsigned int (EDA_ITEM::*)() const) &EDA_ITEM::GetStatus, "C++: EDA_ITEM::GetStatus() const --> unsigned int");
		cl.def("SetStatus", (void (EDA_ITEM::*)(unsigned int)) &EDA_ITEM::SetStatus, "C++: EDA_ITEM::SetStatus(unsigned int) --> void", pybind11::arg("aStatus"));
		cl.def("SetFlags", (void (EDA_ITEM::*)(unsigned int)) &EDA_ITEM::SetFlags, "C++: EDA_ITEM::SetFlags(unsigned int) --> void", pybind11::arg("aMask"));
		cl.def("ClearFlags", [](EDA_ITEM &o) -> void { return o.ClearFlags(); }, "");
		cl.def("ClearFlags", (void (EDA_ITEM::*)(unsigned int)) &EDA_ITEM::ClearFlags, "C++: EDA_ITEM::ClearFlags(unsigned int) --> void", pybind11::arg("aMask"));
		cl.def("GetFlags", (unsigned int (EDA_ITEM::*)() const) &EDA_ITEM::GetFlags, "C++: EDA_ITEM::GetFlags() const --> unsigned int");
		cl.def("HasFlag", (bool (EDA_ITEM::*)(unsigned int) const) &EDA_ITEM::HasFlag, "C++: EDA_ITEM::HasFlag(unsigned int) const --> bool", pybind11::arg("aFlag"));
		cl.def("GetEditFlags", (unsigned int (EDA_ITEM::*)() const) &EDA_ITEM::GetEditFlags, "C++: EDA_ITEM::GetEditFlags() const --> unsigned int");
		cl.def("ClearTempFlags", (void (EDA_ITEM::*)()) &EDA_ITEM::ClearTempFlags, "C++: EDA_ITEM::ClearTempFlags() --> void");
		cl.def("ClearEditFlags", (void (EDA_ITEM::*)()) &EDA_ITEM::ClearEditFlags, "C++: EDA_ITEM::ClearEditFlags() --> void");
		cl.def("SetForceVisible", (void (EDA_ITEM::*)(bool)) &EDA_ITEM::SetForceVisible, "Set and clear force visible flag used to force the item to be drawn even if it's draw\n attribute is set to not visible.\n\n \n True forces the item to be drawn.  False uses the item's visibility\n                setting to determine if the item is to be drawn.\n\nC++: EDA_ITEM::SetForceVisible(bool) --> void", pybind11::arg("aEnable"));
		cl.def("IsForceVisible", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsForceVisible, "C++: EDA_ITEM::IsForceVisible() const --> bool");
		cl.def("HitTest", [](EDA_ITEM const &o, const class wxPoint & a0) -> bool { return o.HitTest(a0); }, "", pybind11::arg("aPosition"));
		cl.def("HitTest", (bool (EDA_ITEM::*)(const class wxPoint &, int) const) &EDA_ITEM::HitTest, "Test if  is contained within or on the bounding box of an item.\n\n \n A reference to a wxPoint object containing the coordinates to test.\n \n\n Increase the item bounding box by this amount.\n \n\n True if  is within the item bounding box.\n\nC++: EDA_ITEM::HitTest(const class wxPoint &, int) const --> bool", pybind11::arg("aPosition"), pybind11::arg("aAccuracy"));
		cl.def("HitTest", [](EDA_ITEM const &o, const class EDA_RECT & a0, bool const & a1) -> bool { return o.HitTest(a0, a1); }, "", pybind11::arg("aRect"), pybind11::arg("aContained"));
		cl.def("HitTest", (bool (EDA_ITEM::*)(const class EDA_RECT &, bool, int) const) &EDA_ITEM::HitTest, "Test if  intersects or is contained within the bounding box of an item.\n\n \n A reference to a #EDA_RECT object containing the rectangle to test.\n \n\n Set to true to test for containment instead of an intersection.\n \n\n Increase  by this amount.\n \n\n True if  contains or intersects the item bounding box.\n\nC++: EDA_ITEM::HitTest(const class EDA_RECT &, bool, int) const --> bool", pybind11::arg("aRect"), pybind11::arg("aContained"), pybind11::arg("aAccuracy"));
		cl.def("GetBoundingBox", (const class EDA_RECT (EDA_ITEM::*)() const) &EDA_ITEM::GetBoundingBox, "Return the orthogonal bounding box of this object for display purposes.\n\n This box should be an enclosing perimeter for visible components of this\n object, and the units should be in the pcb or schematic coordinate\n system.  It is OK to overestimate the size by a few counts.\n\nC++: EDA_ITEM::GetBoundingBox() const --> const class EDA_RECT");
		cl.def("GetPosition", (class wxPoint (EDA_ITEM::*)() const) &EDA_ITEM::GetPosition, "C++: EDA_ITEM::GetPosition() const --> class wxPoint");
		cl.def("SetPosition", (void (EDA_ITEM::*)(const class wxPoint &)) &EDA_ITEM::SetPosition, "C++: EDA_ITEM::SetPosition(const class wxPoint &) --> void", pybind11::arg("aPos"));
		cl.def("GetFocusPosition", (const class wxPoint (EDA_ITEM::*)() const) &EDA_ITEM::GetFocusPosition, "Similar to GetPosition, but allows items to return their visual center rather\n than their anchor.\n\nC++: EDA_ITEM::GetFocusPosition() const --> const class wxPoint");
		cl.def("Clone", (class EDA_ITEM * (EDA_ITEM::*)() const) &EDA_ITEM::Clone, "Create a duplicate of this item with linked list members set to NULL.\n\n The default version will return NULL in release builds and likely crash the\n program.  In debug builds, a warning message indicating the derived class\n has not implemented cloning.  This really should be a pure virtual function.\n Due to the fact that there are so many objects derived from EDA_ITEM, the\n decision was made to return NULL until all the objects derived from EDA_ITEM\n implement cloning.  Once that happens, this function should be made pure.\n\n \n A clone of the item.\n\nC++: EDA_ITEM::Clone() const --> class EDA_ITEM *", pybind11::return_value_policy::automatic);
		cl.def("GetClass", (class wxString (EDA_ITEM::*)() const) &EDA_ITEM::GetClass, "Return the class name.\n\nC++: EDA_ITEM::GetClass() const --> class wxString");
		cl.def("GetSelectMenuText", (class wxString (EDA_ITEM::*)(enum EDA_UNITS) const) &EDA_ITEM::GetSelectMenuText, "Return the text to display to be used in the selection clarification context menu\n when multiple items are found at the current cursor position.\n\n The default version of this function raises an assertion in the debug mode and\n returns a string to indicate that it was not overridden to provide the object\n specific text.\n\n \n The menu text string.\n\nC++: EDA_ITEM::GetSelectMenuText(enum EDA_UNITS) const --> class wxString", pybind11::arg("aUnits"));
		cl.def("GetMenuImage", (int (EDA_ITEM::*)() const) &EDA_ITEM::GetMenuImage, "Return a pointer to an image to be used in menus.\n\n The default version returns the right arrow image.  Override this function to provide\n object specific menu images.\n\n \n The menu image associated with the item.\n\nC++: EDA_ITEM::GetMenuImage() const --> int");
		cl.def("Matches", (bool (EDA_ITEM::*)(const class wxFindReplaceData &, void *) const) &EDA_ITEM::Matches, "Compare the item against the search criteria in \n\n The base class returns false since many of the objects derived from EDA_ITEM\n do not have any text to search.\n\n \n A reference to a wxFindReplaceData object containing the\n                    search criteria.\n \n\n A pointer to optional data required for the search or NULL if not used.\n \n\n True if the item's text matches the search criteria in \n     \n\nC++: EDA_ITEM::Matches(const class wxFindReplaceData &, void *) const --> bool", pybind11::arg("aSearchData"), pybind11::arg("aAuxData"));
		cl.def_static("Replace", (bool (*)(const class wxFindReplaceData &, class wxString &)) &EDA_ITEM::Replace, "Perform a text replace on  using the find and replace criteria in\n  on items that support text find and replace.\n\n \n A reference to a wxFindReplaceData object containing the\n                    search and replace criteria.\n \n\n A reference to a wxString object containing the text to be replaced.\n \n\n True if  was modified, otherwise false.\n\nC++: EDA_ITEM::Replace(const class wxFindReplaceData &, class wxString &) --> bool", pybind11::arg("aSearchData"), pybind11::arg("aText"));
		cl.def("Replace", [](EDA_ITEM &o, const class wxFindReplaceData & a0) -> bool { return o.Replace(a0); }, "", pybind11::arg("aSearchData"));
		cl.def("Replace", (bool (EDA_ITEM::*)(const class wxFindReplaceData &, void *)) &EDA_ITEM::Replace, "Perform a text replace using the find and replace criteria in \n on items that support text find and replace.\n\n This function must be overridden for items that support text replace.\n\n \n A reference to a wxFindReplaceData object containing the search and\n                    replace criteria.\n \n\n A pointer to optional data required for the search or NULL if not used.\n \n\n True if the item text was modified, otherwise false.\n\nC++: EDA_ITEM::Replace(const class wxFindReplaceData &, void *) --> bool", pybind11::arg("aSearchData"), pybind11::arg("aAuxData"));
		cl.def("IsReplaceable", (bool (EDA_ITEM::*)() const) &EDA_ITEM::IsReplaceable, "Override this method in any derived object that supports test find and replace.\n\n \n True if the item has replaceable text that can be modified using\n         the find and replace dialog.\n\nC++: EDA_ITEM::IsReplaceable() const --> bool");
		cl.def_static("Sort", (bool (*)(const class EDA_ITEM *, const class EDA_ITEM *)) &EDA_ITEM::Sort, "Helper function to be used by the C++ STL sort algorithm for sorting a STL\n container of #EDA_ITEM pointers.\n\n \n The left hand item to compare.\n \n\n The right hand item to compare.\n \n\n True if  is less than \n     \n\nC++: EDA_ITEM::Sort(const class EDA_ITEM *, const class EDA_ITEM *) --> bool", pybind11::arg("aLeft"), pybind11::arg("aRight"));
		cl.def("assign", (class EDA_ITEM & (EDA_ITEM::*)(const class EDA_ITEM &)) &EDA_ITEM::operator=, "Assign the members of  to another object.\n\nC++: EDA_ITEM::operator=(const class EDA_ITEM &) --> class EDA_ITEM &", pybind11::return_value_policy::automatic, pybind11::arg("aItem"));
		cl.def("ViewBBox", (const int (EDA_ITEM::*)() const) &EDA_ITEM::ViewBBox, "C++: EDA_ITEM::ViewBBox() const --> const int");
	}
	// new_clone(const class EDA_ITEM &) file:eda_item.h line:547
	M("").def("new_clone", (class EDA_ITEM * (*)(const class EDA_ITEM &)) &new_clone, "Provide cloning capabilities for all Boost pointer containers of #EDA_ITEM pointers.\n\n \n EDA_ITEM to clone.\n \n\n Clone of \n \n\nC++: new_clone(const class EDA_ITEM &) --> class EDA_ITEM *", pybind11::return_value_policy::automatic, pybind11::arg("aItem"));

}
