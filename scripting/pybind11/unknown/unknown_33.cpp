#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/pen.h> // wxPenCap
#include <wx/pen.h> // wxPenJoin
#include <wx/pen.h> // wxPenStyle

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxResourceCache file: line:929
struct PyCallBack_wxResourceCache : public wxResourceCache {
	using wxResourceCache::wxResourceCache;

	class wxObjectListNode * Find(const class wxListKey & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxResourceCache *>(this), "Find");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxObjectListNode *>::value) {
				static pybind11::detail::override_caster_t<class wxObjectListNode *> caster;
				return pybind11::detail::cast_ref<class wxObjectListNode *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxObjectListNode *>(std::move(o));
		}
		return wxObjectList::Find(a0);
	}
	class wxNodeBase * CreateNode(class wxNodeBase * a0, class wxNodeBase * a1, void * a2, const class wxListKey & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxResourceCache *>(this), "CreateNode");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxNodeBase *>::value) {
				static pybind11::detail::override_caster_t<class wxNodeBase *> caster;
				return pybind11::detail::cast_ref<class wxNodeBase *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxNodeBase *>(std::move(o));
		}
		return wxObjectList::CreateNode(a0, a1, a2, a3);
	}
};

// wxStockGDI file: line:954
struct PyCallBack_wxStockGDI : public wxStockGDI {
	using wxStockGDI::wxStockGDI;

	const class wxFont * GetFont(enum wxStockGDI::Item a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStockGDI *>(this), "GetFont");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxFont *>::value) {
				static pybind11::detail::override_caster_t<const class wxFont *> caster;
				return pybind11::detail::cast_ref<const class wxFont *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxFont *>(std::move(o));
		}
		return wxStockGDI::GetFont(a0);
	}
};

// wxGDIRefData file: line:22
struct PyCallBack_wxGDIRefData : public wxGDIRefData {
	using wxGDIRefData::wxGDIRefData;

	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGDIRefData *>(this), "IsOk");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxGDIRefData::IsOk();
	}
};

// wxGDIObject file: line:41
struct PyCallBack_wxGDIObject : public wxGDIObject {
	using wxGDIObject::wxGDIObject;

	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGDIObject *>(this), "IsOk");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxGDIObject::IsOk();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGDIObject *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxGDIObject::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGDIObject *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxGDIObject::CloneRefData(a0);
	}
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGDIObject *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxGDIObject::CreateGDIRefData\"");
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGDIObject *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxGDIObject::CloneGDIRefData\"");
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGDIObject *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxGDIObject::GetClassInfo();
	}
};

// wxCursor file: line:22
struct PyCallBack_wxCursor : public wxCursor {
	using wxCursor::wxCursor;

	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCursor *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxCursor::CreateGDIRefData();
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCursor *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxCursor::CloneGDIRefData(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCursor *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxCursor::GetClassInfo();
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCursor *>(this), "IsOk");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxGDIObject::IsOk();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCursor *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxGDIObject::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCursor *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxGDIObject::CloneRefData(a0);
	}
};

void bind_unknown_unknown_33(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxStringToColourHashMap file: line:894
		pybind11::class_<wxStringToColourHashMap, std::shared_ptr<wxStringToColourHashMap>, wxStringToColourHashMap_wxImplementation_HashTable> cl(M(""), "wxStringToColourHashMap", "");
		cl.def( pybind11::init( [](){ return new wxStringToColourHashMap(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxStringToColourHashMap(a0); } ), "doc" , pybind11::arg("hint"));
		cl.def( pybind11::init( [](unsigned long const & a0, struct wxStringHash const & a1){ return new wxStringToColourHashMap(a0, a1); } ), "doc" , pybind11::arg("hint"), pybind11::arg("hf"));
		cl.def( pybind11::init<unsigned long, struct wxStringHash, struct wxStringEqual>(), pybind11::arg("hint"), pybind11::arg("hf"), pybind11::arg("eq") );

		cl.def( pybind11::init( [](wxStringToColourHashMap const &o){ return new wxStringToColourHashMap(o); } ) );
		cl.def("__getitem__", (class wxColour *& (wxStringToColourHashMap::*)(const class wxString &)) &wxStringToColourHashMap::operator[], "C++: wxStringToColourHashMap::operator[](const class wxString &) --> class wxColour *&", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("find", (class wxStringToColourHashMap_wxImplementation_HashTable::iterator (wxStringToColourHashMap::*)(const class wxString &)) &wxStringToColourHashMap::find, "C++: wxStringToColourHashMap::find(const class wxString &) --> class wxStringToColourHashMap_wxImplementation_HashTable::iterator", pybind11::arg("key"));
		cl.def("insert", (class wxStringToColourHashMap::Insert_Result (wxStringToColourHashMap::*)(const class wxStringToColourHashMap_wxImplementation_Pair &)) &wxStringToColourHashMap::insert, "C++: wxStringToColourHashMap::insert(const class wxStringToColourHashMap_wxImplementation_Pair &) --> class wxStringToColourHashMap::Insert_Result", pybind11::arg("v"));
		cl.def("erase", (unsigned long (wxStringToColourHashMap::*)(const class wxString &)) &wxStringToColourHashMap::erase, "C++: wxStringToColourHashMap::erase(const class wxString &) --> unsigned long", pybind11::arg("k"));
		cl.def("erase", (void (wxStringToColourHashMap::*)(const class wxStringToColourHashMap_wxImplementation_HashTable::iterator &)) &wxStringToColourHashMap::erase, "C++: wxStringToColourHashMap::erase(const class wxStringToColourHashMap_wxImplementation_HashTable::iterator &) --> void", pybind11::arg("it"));
		cl.def("count", (unsigned long (wxStringToColourHashMap::*)(const class wxString &)) &wxStringToColourHashMap::count, "C++: wxStringToColourHashMap::count(const class wxString &) --> unsigned long", pybind11::arg("key"));
		cl.def("assign", (class wxStringToColourHashMap & (wxStringToColourHashMap::*)(const class wxStringToColourHashMap &)) &wxStringToColourHashMap::operator=, "C++: wxStringToColourHashMap::operator=(const class wxStringToColourHashMap &) --> class wxStringToColourHashMap &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxStringToColourHashMap::Insert_Result file: line:645
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToColourHashMap::Insert_Result, std::shared_ptr<wxStringToColourHashMap::Insert_Result>> cl(enclosing_class, "Insert_Result", "");
			cl.def( pybind11::init<const class wxStringToColourHashMap_wxImplementation_HashTable::iterator &, const bool &>(), pybind11::arg("f"), pybind11::arg("s") );

			cl.def( pybind11::init( [](wxStringToColourHashMap::Insert_Result const &o){ return new wxStringToColourHashMap::Insert_Result(o); } ) );
			cl.def_readwrite("first", &wxStringToColourHashMap::Insert_Result::first);
			cl.def_readwrite("second", &wxStringToColourHashMap::Insert_Result::second);
		}

	}
	{ // wxColourDatabase file: line:896
		pybind11::class_<wxColourDatabase, std::shared_ptr<wxColourDatabase>> cl(M(""), "wxColourDatabase", "");
		cl.def( pybind11::init( [](){ return new wxColourDatabase(); } ) );
		cl.def("Find", (class wxColour (wxColourDatabase::*)(const class wxString &) const) &wxColourDatabase::Find, "C++: wxColourDatabase::Find(const class wxString &) const --> class wxColour", pybind11::arg("name"));
		cl.def("FindName", (class wxString (wxColourDatabase::*)(const class wxColour &) const) &wxColourDatabase::FindName, "C++: wxColourDatabase::FindName(const class wxColour &) const --> class wxString", pybind11::arg("colour"));
		cl.def("AddColour", (void (wxColourDatabase::*)(const class wxString &, const class wxColour &)) &wxColourDatabase::AddColour, "C++: wxColourDatabase::AddColour(const class wxString &, const class wxColour &) --> void", pybind11::arg("name"), pybind11::arg("colour"));
	}
	{ // wxResourceCache file: line:929
		pybind11::class_<wxResourceCache, std::shared_ptr<wxResourceCache>, PyCallBack_wxResourceCache, wxList> cl(M(""), "wxResourceCache", "");
		cl.def( pybind11::init( [](){ return new wxResourceCache(); }, [](){ return new PyCallBack_wxResourceCache(); } ) );
		cl.def( pybind11::init<const unsigned int>(), pybind11::arg("keyType") );

		cl.def( pybind11::init( [](PyCallBack_wxResourceCache const &o){ return new PyCallBack_wxResourceCache(o); } ) );
		cl.def( pybind11::init( [](wxResourceCache const &o){ return new wxResourceCache(o); } ) );
		cl.def("assign", (class wxResourceCache & (wxResourceCache::*)(const class wxResourceCache &)) &wxResourceCache::operator=, "C++: wxResourceCache::operator=(const class wxResourceCache &) --> class wxResourceCache &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStockGDI file: line:954
		pybind11::class_<wxStockGDI, std::shared_ptr<wxStockGDI>, PyCallBack_wxStockGDI> cl(M(""), "wxStockGDI", "");
		cl.def( pybind11::init( [](){ return new wxStockGDI(); }, [](){ return new PyCallBack_wxStockGDI(); } ) );

		pybind11::enum_<wxStockGDI::Item>(cl, "Item", pybind11::arithmetic(), "")
			.value("BRUSH_BLACK", wxStockGDI::BRUSH_BLACK)
			.value("BRUSH_BLUE", wxStockGDI::BRUSH_BLUE)
			.value("BRUSH_CYAN", wxStockGDI::BRUSH_CYAN)
			.value("BRUSH_GREEN", wxStockGDI::BRUSH_GREEN)
			.value("BRUSH_YELLOW", wxStockGDI::BRUSH_YELLOW)
			.value("BRUSH_GREY", wxStockGDI::BRUSH_GREY)
			.value("BRUSH_LIGHTGREY", wxStockGDI::BRUSH_LIGHTGREY)
			.value("BRUSH_MEDIUMGREY", wxStockGDI::BRUSH_MEDIUMGREY)
			.value("BRUSH_RED", wxStockGDI::BRUSH_RED)
			.value("BRUSH_TRANSPARENT", wxStockGDI::BRUSH_TRANSPARENT)
			.value("BRUSH_WHITE", wxStockGDI::BRUSH_WHITE)
			.value("COLOUR_BLACK", wxStockGDI::COLOUR_BLACK)
			.value("COLOUR_BLUE", wxStockGDI::COLOUR_BLUE)
			.value("COLOUR_CYAN", wxStockGDI::COLOUR_CYAN)
			.value("COLOUR_GREEN", wxStockGDI::COLOUR_GREEN)
			.value("COLOUR_YELLOW", wxStockGDI::COLOUR_YELLOW)
			.value("COLOUR_LIGHTGREY", wxStockGDI::COLOUR_LIGHTGREY)
			.value("COLOUR_RED", wxStockGDI::COLOUR_RED)
			.value("COLOUR_WHITE", wxStockGDI::COLOUR_WHITE)
			.value("CURSOR_CROSS", wxStockGDI::CURSOR_CROSS)
			.value("CURSOR_HOURGLASS", wxStockGDI::CURSOR_HOURGLASS)
			.value("CURSOR_STANDARD", wxStockGDI::CURSOR_STANDARD)
			.value("FONT_ITALIC", wxStockGDI::FONT_ITALIC)
			.value("FONT_NORMAL", wxStockGDI::FONT_NORMAL)
			.value("FONT_SMALL", wxStockGDI::FONT_SMALL)
			.value("FONT_SWISS", wxStockGDI::FONT_SWISS)
			.value("PEN_BLACK", wxStockGDI::PEN_BLACK)
			.value("PEN_BLACKDASHED", wxStockGDI::PEN_BLACKDASHED)
			.value("PEN_BLUE", wxStockGDI::PEN_BLUE)
			.value("PEN_CYAN", wxStockGDI::PEN_CYAN)
			.value("PEN_GREEN", wxStockGDI::PEN_GREEN)
			.value("PEN_YELLOW", wxStockGDI::PEN_YELLOW)
			.value("PEN_GREY", wxStockGDI::PEN_GREY)
			.value("PEN_LIGHTGREY", wxStockGDI::PEN_LIGHTGREY)
			.value("PEN_MEDIUMGREY", wxStockGDI::PEN_MEDIUMGREY)
			.value("PEN_RED", wxStockGDI::PEN_RED)
			.value("PEN_TRANSPARENT", wxStockGDI::PEN_TRANSPARENT)
			.value("PEN_WHITE", wxStockGDI::PEN_WHITE)
			.value("ITEMCOUNT", wxStockGDI::ITEMCOUNT)
			.export_values();

		cl.def_static("DeleteAll", (void (*)()) &wxStockGDI::DeleteAll, "C++: wxStockGDI::DeleteAll() --> void");
		cl.def_static("instance", (class wxStockGDI & (*)()) &wxStockGDI::instance, "C++: wxStockGDI::instance() --> class wxStockGDI &", pybind11::return_value_policy::automatic);
		cl.def_static("GetBrush", (const class wxBrush * (*)(enum wxStockGDI::Item)) &wxStockGDI::GetBrush, "C++: wxStockGDI::GetBrush(enum wxStockGDI::Item) --> const class wxBrush *", pybind11::return_value_policy::automatic, pybind11::arg("item"));
		cl.def_static("GetColour", (const class wxColour * (*)(enum wxStockGDI::Item)) &wxStockGDI::GetColour, "C++: wxStockGDI::GetColour(enum wxStockGDI::Item) --> const class wxColour *", pybind11::return_value_policy::automatic, pybind11::arg("item"));
		cl.def_static("GetCursor", (const class wxCursor * (*)(enum wxStockGDI::Item)) &wxStockGDI::GetCursor, "C++: wxStockGDI::GetCursor(enum wxStockGDI::Item) --> const class wxCursor *", pybind11::return_value_policy::automatic, pybind11::arg("item"));
		cl.def("GetFont", (const class wxFont * (wxStockGDI::*)(enum wxStockGDI::Item)) &wxStockGDI::GetFont, "C++: wxStockGDI::GetFont(enum wxStockGDI::Item) --> const class wxFont *", pybind11::return_value_policy::automatic, pybind11::arg("item"));
		cl.def_static("GetPen", (const class wxPen * (*)(enum wxStockGDI::Item)) &wxStockGDI::GetPen, "C++: wxStockGDI::GetPen(enum wxStockGDI::Item) --> const class wxPen *", pybind11::return_value_policy::automatic, pybind11::arg("item"));
	}
	// wxInitializeStockLists() file: line:1086
	M("").def("wxInitializeStockLists", (void (*)()) &wxInitializeStockLists, "C++: wxInitializeStockLists() --> void");

	// wxDeleteStockLists() file: line:1087
	M("").def("wxDeleteStockLists", (void (*)()) &wxDeleteStockLists, "C++: wxDeleteStockLists() --> void");

	// wxColourDisplay() file: line:1090
	M("").def("wxColourDisplay", (bool (*)()) &wxColourDisplay, "C++: wxColourDisplay() --> bool");

	// wxDisplayDepth() file: line:1093
	M("").def("wxDisplayDepth", (int (*)()) &wxDisplayDepth, "C++: wxDisplayDepth() --> int");

	// wxDisplaySize(int *, int *) file: line:1097
	M("").def("wxDisplaySize", (void (*)(int *, int *)) &wxDisplaySize, "C++: wxDisplaySize(int *, int *) --> void", pybind11::arg("width"), pybind11::arg("height"));

	// wxGetDisplaySize() file: line:1098
	M("").def("wxGetDisplaySize", (class wxSize (*)()) &wxGetDisplaySize, "C++: wxGetDisplaySize() --> class wxSize");

	// wxDisplaySizeMM(int *, int *) file: line:1099
	M("").def("wxDisplaySizeMM", (void (*)(int *, int *)) &wxDisplaySizeMM, "C++: wxDisplaySizeMM(int *, int *) --> void", pybind11::arg("width"), pybind11::arg("height"));

	// wxGetDisplaySizeMM() file: line:1100
	M("").def("wxGetDisplaySizeMM", (class wxSize (*)()) &wxGetDisplaySizeMM, "C++: wxGetDisplaySizeMM() --> class wxSize");

	// wxGetDisplayPPI() file: line:1101
	M("").def("wxGetDisplayPPI", (class wxSize (*)()) &wxGetDisplayPPI, "C++: wxGetDisplayPPI() --> class wxSize");

	// wxClientDisplayRect(int *, int *, int *, int *) file: line:1104
	M("").def("wxClientDisplayRect", (void (*)(int *, int *, int *, int *)) &wxClientDisplayRect, "C++: wxClientDisplayRect(int *, int *, int *, int *) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("height"));

	// wxGetClientDisplayRect() file: line:1105
	M("").def("wxGetClientDisplayRect", (class wxRect (*)()) &wxGetClientDisplayRect, "C++: wxGetClientDisplayRect() --> class wxRect");

	// wxSetCursor(const class wxCursor &) file: line:1108
	M("").def("wxSetCursor", (void (*)(const class wxCursor &)) &wxSetCursor, "C++: wxSetCursor(const class wxCursor &) --> void", pybind11::arg("cursor"));

	{ // wxGDIRefData file: line:22
		pybind11::class_<wxGDIRefData, std::shared_ptr<wxGDIRefData>, PyCallBack_wxGDIRefData, wxRefCounter> cl(M(""), "wxGDIRefData", "");
		cl.def( pybind11::init( [](){ return new wxGDIRefData(); }, [](){ return new PyCallBack_wxGDIRefData(); } ) );
		cl.def("IsOk", (bool (wxGDIRefData::*)() const) &wxGDIRefData::IsOk, "C++: wxGDIRefData::IsOk() const --> bool");
	}
	{ // wxGDIObject file: line:41
		pybind11::class_<wxGDIObject, std::shared_ptr<wxGDIObject>, PyCallBack_wxGDIObject, wxObject> cl(M(""), "wxGDIObject", "");
		cl.def(pybind11::init<PyCallBack_wxGDIObject const &>());
		cl.def( pybind11::init( [](){ return new PyCallBack_wxGDIObject(); } ) );
		cl.def("IsOk", (bool (wxGDIObject::*)() const) &wxGDIObject::IsOk, "C++: wxGDIObject::IsOk() const --> bool");
		cl.def("IsNull", (bool (wxGDIObject::*)() const) &wxGDIObject::IsNull, "C++: wxGDIObject::IsNull() const --> bool");
		cl.def("Ok", (bool (wxGDIObject::*)() const) &wxGDIObject::Ok, "C++: wxGDIObject::Ok() const --> bool");
		cl.def("GetClassInfo", (class wxClassInfo * (wxGDIObject::*)() const) &wxGDIObject::GetClassInfo, "C++: wxGDIObject::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxGDIObject::wxCreateObject, "C++: wxGDIObject::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxGDIObject & (wxGDIObject::*)(const class wxGDIObject &)) &wxGDIObject::operator=, "C++: wxGDIObject::operator=(const class wxGDIObject &) --> class wxGDIObject &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxCursor file: line:22
		pybind11::class_<wxCursor, std::shared_ptr<wxCursor>, PyCallBack_wxCursor, wxGDIObject> cl(M(""), "wxCursor", "");
		cl.def( pybind11::init( [](){ return new wxCursor(); }, [](){ return new PyCallBack_wxCursor(); } ) );
		cl.def( pybind11::init<enum wxStockCursor>(), pybind11::arg("id") );

		cl.def( pybind11::init<int>(), pybind11::arg("id") );

		cl.def( pybind11::init<const class wxImage &>(), pybind11::arg("image") );

		cl.def( pybind11::init( [](const class wxString & a0){ return new wxCursor(a0); }, [](const class wxString & a0){ return new PyCallBack_wxCursor(a0); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, enum wxBitmapType const & a1){ return new wxCursor(a0, a1); }, [](const class wxString & a0, enum wxBitmapType const & a1){ return new PyCallBack_wxCursor(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, enum wxBitmapType const & a1, int const & a2){ return new wxCursor(a0, a1, a2); }, [](const class wxString & a0, enum wxBitmapType const & a1, int const & a2){ return new PyCallBack_wxCursor(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<const class wxString &, enum wxBitmapType, int, int>(), pybind11::arg("name"), pybind11::arg("type"), pybind11::arg("hotSpotX"), pybind11::arg("hotSpotY") );

		cl.def( pybind11::init( [](PyCallBack_wxCursor const &o){ return new PyCallBack_wxCursor(o); } ) );
		cl.def( pybind11::init( [](wxCursor const &o){ return new wxCursor(o); } ) );
		cl.def("GetCursor", (class wxCursor * (wxCursor::*)() const) &wxCursor::GetCursor, "C++: wxCursor::GetCursor() const --> class wxCursor *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxCursor::*)() const) &wxCursor::GetClassInfo, "C++: wxCursor::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxCursor::wxCreateObject, "C++: wxCursor::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxCursor & (wxCursor::*)(const class wxCursor &)) &wxCursor::operator=, "C++: wxCursor::operator=(const class wxCursor &) --> class wxCursor &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxStringSortAscending(class wxString *, class wxString *) file: line:20
	M("").def("wxStringSortAscending", (int (*)(class wxString *, class wxString *)) &wxStringSortAscending, "C++: wxStringSortAscending(class wxString *, class wxString *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStringSortDescending(class wxString *, class wxString *) file: line:25
	M("").def("wxStringSortDescending", (int (*)(class wxString *, class wxString *)) &wxStringSortDescending, "C++: wxStringSortDescending(class wxString *, class wxString *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

}
