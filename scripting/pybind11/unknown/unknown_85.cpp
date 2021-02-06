#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <time.h> // tm
#include <wx/propgrid/property.h> // wxPGPaintData

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxStringTokenizer file: line:40
struct PyCallBack_wxStringTokenizer : public wxStringTokenizer {
	using wxStringTokenizer::wxStringTokenizer;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStringTokenizer *>(this), "GetClassInfo");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStringTokenizer *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStringTokenizer *>(this), "CloneRefData");
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

void bind_unknown_unknown_85(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPGHashMapI2I file: line:315
		pybind11::class_<wxPGHashMapI2I, std::shared_ptr<wxPGHashMapI2I>, wxPGHashMapI2I_wxImplementation_HashTable> cl(M(""), "wxPGHashMapI2I", "");
		cl.def( pybind11::init( [](){ return new wxPGHashMapI2I(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxPGHashMapI2I(a0); } ), "doc" , pybind11::arg("hint"));
		cl.def( pybind11::init( [](unsigned long const & a0, struct wxIntegerHash const & a1){ return new wxPGHashMapI2I(a0, a1); } ), "doc" , pybind11::arg("hint"), pybind11::arg("hf"));
		cl.def( pybind11::init<unsigned long, struct wxIntegerHash, struct wxIntegerEqual>(), pybind11::arg("hint"), pybind11::arg("hf"), pybind11::arg("eq") );

		cl.def( pybind11::init( [](wxPGHashMapI2I const &o){ return new wxPGHashMapI2I(o); } ) );
		cl.def("__getitem__", (int & (wxPGHashMapI2I::*)(const int &)) &wxPGHashMapI2I::operator[], "C++: wxPGHashMapI2I::operator[](const int &) --> int &", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("find", (class wxPGHashMapI2I_wxImplementation_HashTable::iterator (wxPGHashMapI2I::*)(const int &)) &wxPGHashMapI2I::find, "C++: wxPGHashMapI2I::find(const int &) --> class wxPGHashMapI2I_wxImplementation_HashTable::iterator", pybind11::arg("key"));
		cl.def("insert", (class wxPGHashMapI2I::Insert_Result (wxPGHashMapI2I::*)(const class wxPGHashMapI2I_wxImplementation_Pair &)) &wxPGHashMapI2I::insert, "C++: wxPGHashMapI2I::insert(const class wxPGHashMapI2I_wxImplementation_Pair &) --> class wxPGHashMapI2I::Insert_Result", pybind11::arg("v"));
		cl.def("erase", (unsigned long (wxPGHashMapI2I::*)(const int &)) &wxPGHashMapI2I::erase, "C++: wxPGHashMapI2I::erase(const int &) --> unsigned long", pybind11::arg("k"));
		cl.def("erase", (void (wxPGHashMapI2I::*)(const class wxPGHashMapI2I_wxImplementation_HashTable::iterator &)) &wxPGHashMapI2I::erase, "C++: wxPGHashMapI2I::erase(const class wxPGHashMapI2I_wxImplementation_HashTable::iterator &) --> void", pybind11::arg("it"));
		cl.def("count", (unsigned long (wxPGHashMapI2I::*)(const int &)) &wxPGHashMapI2I::count, "C++: wxPGHashMapI2I::count(const int &) --> unsigned long", pybind11::arg("key"));
		cl.def("assign", (class wxPGHashMapI2I & (wxPGHashMapI2I::*)(const class wxPGHashMapI2I &)) &wxPGHashMapI2I::operator=, "C++: wxPGHashMapI2I::operator=(const class wxPGHashMapI2I &) --> class wxPGHashMapI2I &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxPGHashMapI2I::Insert_Result file: line:645
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapI2I::Insert_Result, std::shared_ptr<wxPGHashMapI2I::Insert_Result>> cl(enclosing_class, "Insert_Result", "");
			cl.def( pybind11::init<const class wxPGHashMapI2I_wxImplementation_HashTable::iterator &, const bool &>(), pybind11::arg("f"), pybind11::arg("s") );

			cl.def( pybind11::init( [](wxPGHashMapI2I::Insert_Result const &o){ return new wxPGHashMapI2I::Insert_Result(o); } ) );
			cl.def_readwrite("first", &wxPGHashMapI2I::Insert_Result::first);
			cl.def_readwrite("second", &wxPGHashMapI2I::Insert_Result::second);
		}

	}
	// wxPG_GETPROPERTYVALUES_FLAGS file: line:333
	pybind11::enum_<wxPG_GETPROPERTYVALUES_FLAGS>(M(""), "wxPG_GETPROPERTYVALUES_FLAGS", pybind11::arithmetic(), "")
		.value("wxPG_KEEP_STRUCTURE", wxPG_KEEP_STRUCTURE)
		.value("wxPG_RECURSE", wxPG_RECURSE)
		.value("wxPG_INC_ATTRIBUTES", wxPG_INC_ATTRIBUTES)
		.value("wxPG_RECURSE_STARTS", wxPG_RECURSE_STARTS)
		.value("wxPG_FORCE", wxPG_FORCE)
		.value("wxPG_SORT_TOP_LEVEL_ONLY", wxPG_SORT_TOP_LEVEL_ONLY)
		.export_values();

;

	// wxPG_MISC_ARG_FLAGS file: line:364
	pybind11::enum_<wxPG_MISC_ARG_FLAGS>(M(""), "wxPG_MISC_ARG_FLAGS", pybind11::arithmetic(), "")
		.value("wxPG_FULL_VALUE", wxPG_FULL_VALUE)
		.value("wxPG_REPORT_ERROR", wxPG_REPORT_ERROR)
		.value("wxPG_PROPERTY_SPECIFIC", wxPG_PROPERTY_SPECIFIC)
		.value("wxPG_EDITABLE_VALUE", wxPG_EDITABLE_VALUE)
		.value("wxPG_COMPOSITE_FRAGMENT", wxPG_COMPOSITE_FRAGMENT)
		.value("wxPG_UNEDITABLE_COMPOSITE_FRAGMENT", wxPG_UNEDITABLE_COMPOSITE_FRAGMENT)
		.value("wxPG_VALUE_IS_CURRENT", wxPG_VALUE_IS_CURRENT)
		.value("wxPG_PROGRAMMATIC_VALUE", wxPG_PROGRAMMATIC_VALUE)
		.export_values();

;

	// wxPG_SETVALUE_FLAGS file: line:395
	pybind11::enum_<wxPG_SETVALUE_FLAGS>(M(""), "wxPG_SETVALUE_FLAGS", pybind11::arithmetic(), "")
		.value("wxPG_SETVAL_REFRESH_EDITOR", wxPG_SETVAL_REFRESH_EDITOR)
		.value("wxPG_SETVAL_AGGREGATED", wxPG_SETVAL_AGGREGATED)
		.value("wxPG_SETVAL_FROM_PARENT", wxPG_SETVAL_FROM_PARENT)
		.value("wxPG_SETVAL_BY_USER", wxPG_SETVAL_BY_USER)
		.export_values();

;

	// WXVARIANT(const int &) file: line:464
	M("").def("WXVARIANT", (class wxVariant (*)(const int &)) &WXVARIANT, "C++: WXVARIANT(const int &) --> class wxVariant", pybind11::arg("value"));

	// WXVARIANT(const long &) file: line:466
	M("").def("WXVARIANT", (class wxVariant (*)(const long &)) &WXVARIANT, "C++: WXVARIANT(const long &) --> class wxVariant", pybind11::arg("value"));

	// WXVARIANT(const bool &) file: line:468
	M("").def("WXVARIANT", (class wxVariant (*)(const bool &)) &WXVARIANT, "C++: WXVARIANT(const bool &) --> class wxVariant", pybind11::arg("value"));

	// WXVARIANT(const double &) file: line:470
	M("").def("WXVARIANT", (class wxVariant (*)(const double &)) &WXVARIANT, "C++: WXVARIANT(const double &) --> class wxVariant", pybind11::arg("value"));

	// WXVARIANT(const class wxArrayString &) file: line:472
	M("").def("WXVARIANT", (class wxVariant (*)(const class wxArrayString &)) &WXVARIANT, "C++: WXVARIANT(const class wxArrayString &) --> class wxVariant", pybind11::arg("value"));

	// WXVARIANT(const class wxString &) file: line:474
	M("").def("WXVARIANT", (class wxVariant (*)(const class wxString &)) &WXVARIANT, "C++: WXVARIANT(const class wxString &) --> class wxVariant", pybind11::arg("value"));

	// WXVARIANT(const class wxLongLongNative &) file: line:477
	M("").def("WXVARIANT", (class wxVariant (*)(const class wxLongLongNative &)) &WXVARIANT, "C++: WXVARIANT(const class wxLongLongNative &) --> class wxVariant", pybind11::arg("value"));

	// WXVARIANT(const class wxULongLongNative &) file: line:479
	M("").def("WXVARIANT", (class wxVariant (*)(const class wxULongLongNative &)) &WXVARIANT, "C++: WXVARIANT(const class wxULongLongNative &) --> class wxVariant", pybind11::arg("value"));

	// WXVARIANT(const class wxDateTime &) file: line:483
	M("").def("WXVARIANT", (class wxVariant (*)(const class wxDateTime &)) &WXVARIANT, "C++: WXVARIANT(const class wxDateTime &) --> class wxVariant", pybind11::arg("value"));

	// wxPointRefFromVariant(const class wxVariant &) file: line:57
	M("").def("wxPointRefFromVariant", (const class wxPoint & (*)(const class wxVariant &)) &wxPointRefFromVariant, "C++: wxPointRefFromVariant(const class wxVariant &) --> const class wxPoint &", pybind11::return_value_policy::automatic, pybind11::arg("variant"));

	// wxPointRefFromVariant(class wxVariant &) file: line:58
	M("").def("wxPointRefFromVariant", (class wxPoint & (*)(class wxVariant &)) &wxPointRefFromVariant, "C++: wxPointRefFromVariant(class wxVariant &) --> class wxPoint &", pybind11::return_value_policy::automatic, pybind11::arg("variant"));

	// WXVARIANT(const class wxPoint &) file: line:505
	M("").def("WXVARIANT", (class wxVariant (*)(const class wxPoint &)) &WXVARIANT, "C++: WXVARIANT(const class wxPoint &) --> class wxVariant", pybind11::arg("value"));

	// wxSizeRefFromVariant(const class wxVariant &) file: line:60
	M("").def("wxSizeRefFromVariant", (const class wxSize & (*)(const class wxVariant &)) &wxSizeRefFromVariant, "C++: wxSizeRefFromVariant(const class wxVariant &) --> const class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("variant"));

	// wxSizeRefFromVariant(class wxVariant &) file: line:61
	M("").def("wxSizeRefFromVariant", (class wxSize & (*)(class wxVariant &)) &wxSizeRefFromVariant, "C++: wxSizeRefFromVariant(class wxVariant &) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("variant"));

	// WXVARIANT(const class wxSize &) file: line:505
	M("").def("WXVARIANT", (class wxVariant (*)(const class wxSize &)) &WXVARIANT, "C++: WXVARIANT(const class wxSize &) --> class wxVariant", pybind11::arg("value"));

	// wxArrayIntRefFromVariant(const class wxVariant &) file: line:63
	M("").def("wxArrayIntRefFromVariant", (const class wxArrayInt & (*)(const class wxVariant &)) &wxArrayIntRefFromVariant, "C++: wxArrayIntRefFromVariant(const class wxVariant &) --> const class wxArrayInt &", pybind11::return_value_policy::automatic, pybind11::arg("variant"));

	// wxArrayIntRefFromVariant(class wxVariant &) file: line:64
	M("").def("wxArrayIntRefFromVariant", (class wxArrayInt & (*)(class wxVariant &)) &wxArrayIntRefFromVariant, "C++: wxArrayIntRefFromVariant(class wxVariant &) --> class wxArrayInt &", pybind11::return_value_policy::automatic, pybind11::arg("variant"));

	// WXVARIANT(const class wxArrayInt &) file: line:505
	M("").def("WXVARIANT", (class wxVariant (*)(const class wxArrayInt &)) &WXVARIANT, "C++: WXVARIANT(const class wxArrayInt &) --> class wxVariant", pybind11::arg("value"));

	// WXVARIANT(const class wxFont &) file: line:625
	M("").def("WXVARIANT", (class wxVariant (*)(const class wxFont &)) &WXVARIANT, "C++: WXVARIANT(const class wxFont &) --> class wxVariant", pybind11::arg("value"));

	// WXVARIANT(const class wxColour &) file: line:632
	M("").def("WXVARIANT", (class wxVariant (*)(const class wxColour &)) &WXVARIANT, "C++: WXVARIANT(const class wxColour &) --> class wxVariant", pybind11::arg("value"));

	// wxStringTokenizerMode file: line:26
	pybind11::enum_<wxStringTokenizerMode>(M(""), "wxStringTokenizerMode", pybind11::arithmetic(), "")
		.value("wxTOKEN_INVALID", wxTOKEN_INVALID)
		.value("wxTOKEN_DEFAULT", wxTOKEN_DEFAULT)
		.value("wxTOKEN_RET_EMPTY", wxTOKEN_RET_EMPTY)
		.value("wxTOKEN_RET_EMPTY_ALL", wxTOKEN_RET_EMPTY_ALL)
		.value("wxTOKEN_RET_DELIMS", wxTOKEN_RET_DELIMS)
		.value("wxTOKEN_STRTOK", wxTOKEN_STRTOK)
		.export_values();

;

	{ // wxStringTokenizer file: line:40
		pybind11::class_<wxStringTokenizer, std::shared_ptr<wxStringTokenizer>, PyCallBack_wxStringTokenizer, wxObject> cl(M(""), "wxStringTokenizer", "");
		cl.def( pybind11::init( [](){ return new wxStringTokenizer(); }, [](){ return new PyCallBack_wxStringTokenizer(); } ) );
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxStringTokenizer(a0); }, [](const class wxString & a0){ return new PyCallBack_wxStringTokenizer(a0); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1){ return new wxStringTokenizer(a0, a1); }, [](const class wxString & a0, const class wxString & a1){ return new PyCallBack_wxStringTokenizer(a0, a1); } ), "doc");
		cl.def( pybind11::init<const class wxString &, const class wxString &, enum wxStringTokenizerMode>(), pybind11::arg("str"), pybind11::arg("delims"), pybind11::arg("mode") );

		cl.def( pybind11::init<const class wxString &, const class wxString &, bool>(), pybind11::arg("to_tokenize"), pybind11::arg("delims"), pybind11::arg("ret_delim") );

		cl.def( pybind11::init( [](PyCallBack_wxStringTokenizer const &o){ return new PyCallBack_wxStringTokenizer(o); } ) );
		cl.def( pybind11::init( [](wxStringTokenizer const &o){ return new wxStringTokenizer(o); } ) );
		cl.def("SetString", [](wxStringTokenizer &o, const class wxString & a0) -> void { return o.SetString(a0); }, "", pybind11::arg("str"));
		cl.def("SetString", [](wxStringTokenizer &o, const class wxString & a0, const class wxString & a1) -> void { return o.SetString(a0, a1); }, "", pybind11::arg("str"), pybind11::arg("delims"));
		cl.def("SetString", (void (wxStringTokenizer::*)(const class wxString &, const class wxString &, enum wxStringTokenizerMode)) &wxStringTokenizer::SetString, "C++: wxStringTokenizer::SetString(const class wxString &, const class wxString &, enum wxStringTokenizerMode) --> void", pybind11::arg("str"), pybind11::arg("delims"), pybind11::arg("mode"));
		cl.def("Reinit", (void (wxStringTokenizer::*)(const class wxString &)) &wxStringTokenizer::Reinit, "C++: wxStringTokenizer::Reinit(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("CountTokens", (unsigned long (wxStringTokenizer::*)() const) &wxStringTokenizer::CountTokens, "C++: wxStringTokenizer::CountTokens() const --> unsigned long");
		cl.def("HasMoreTokens", (bool (wxStringTokenizer::*)() const) &wxStringTokenizer::HasMoreTokens, "C++: wxStringTokenizer::HasMoreTokens() const --> bool");
		cl.def("GetNextToken", (class wxString (wxStringTokenizer::*)()) &wxStringTokenizer::GetNextToken, "C++: wxStringTokenizer::GetNextToken() --> class wxString");
		cl.def("GetLastDelimiter", (wchar_t (wxStringTokenizer::*)() const) &wxStringTokenizer::GetLastDelimiter, "C++: wxStringTokenizer::GetLastDelimiter() const --> wchar_t");
		cl.def("GetString", (class wxString (wxStringTokenizer::*)() const) &wxStringTokenizer::GetString, "C++: wxStringTokenizer::GetString() const --> class wxString");
		cl.def("GetPosition", (unsigned long (wxStringTokenizer::*)() const) &wxStringTokenizer::GetPosition, "C++: wxStringTokenizer::GetPosition() const --> unsigned long");
		cl.def("GetMode", (enum wxStringTokenizerMode (wxStringTokenizer::*)() const) &wxStringTokenizer::GetMode, "C++: wxStringTokenizer::GetMode() const --> enum wxStringTokenizerMode");
		cl.def("AllowEmpty", (bool (wxStringTokenizer::*)() const) &wxStringTokenizer::AllowEmpty, "C++: wxStringTokenizer::AllowEmpty() const --> bool");
		cl.def("NextToken", (class wxString (wxStringTokenizer::*)()) &wxStringTokenizer::NextToken, "C++: wxStringTokenizer::NextToken() --> class wxString");
		cl.def("SetString", (void (wxStringTokenizer::*)(const class wxString &, const class wxString &, bool)) &wxStringTokenizer::SetString, "C++: wxStringTokenizer::SetString(const class wxString &, const class wxString &, bool) --> void", pybind11::arg("to_tokenize"), pybind11::arg("delims"), pybind11::arg(""));
		cl.def("assign", (class wxStringTokenizer & (wxStringTokenizer::*)(const class wxStringTokenizer &)) &wxStringTokenizer::operator=, "C++: wxStringTokenizer::operator=(const class wxStringTokenizer &) --> class wxStringTokenizer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxStringTokenize(const class wxString &, const class wxString &, enum wxStringTokenizerMode) file: line:144
	M("").def("wxStringTokenize", [](const class wxString & a0) -> wxArrayString { return wxStringTokenize(a0); }, "", pybind11::arg("str"));
	M("").def("wxStringTokenize", [](const class wxString & a0, const class wxString & a1) -> wxArrayString { return wxStringTokenize(a0, a1); }, "", pybind11::arg("str"), pybind11::arg("delims"));
	M("").def("wxStringTokenize", (class wxArrayString (*)(const class wxString &, const class wxString &, enum wxStringTokenizerMode)) &wxStringTokenize, "C++: wxStringTokenize(const class wxString &, const class wxString &, enum wxStringTokenizerMode) --> class wxArrayString", pybind11::arg("str"), pybind11::arg("delims"), pybind11::arg("mode"));

	{ // wxPGStringTokenizer file: line:685
		pybind11::class_<wxPGStringTokenizer, std::shared_ptr<wxPGStringTokenizer>> cl(M(""), "wxPGStringTokenizer", "");
		cl.def( pybind11::init<const class wxString &, wchar_t>(), pybind11::arg("str"), pybind11::arg("delimeter") );

		cl.def( pybind11::init( [](wxPGStringTokenizer const &o){ return new wxPGStringTokenizer(o); } ) );
		cl.def("HasMoreTokens", (bool (wxPGStringTokenizer::*)()) &wxPGStringTokenizer::HasMoreTokens, "C++: wxPGStringTokenizer::HasMoreTokens() --> bool");
		cl.def("GetNextToken", (class wxString (wxPGStringTokenizer::*)()) &wxPGStringTokenizer::GetNextToken, "C++: wxPGStringTokenizer::GetNextToken() --> class wxString");
		cl.def("assign", (class wxPGStringTokenizer & (wxPGStringTokenizer::*)(const class wxPGStringTokenizer &)) &wxPGStringTokenizer::operator=, "C++: wxPGStringTokenizer::operator=(const class wxPGStringTokenizer &) --> class wxPGStringTokenizer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPGPaintData file:wx/propgrid/property.h line:29
		pybind11::class_<wxPGPaintData, std::shared_ptr<wxPGPaintData>> cl(M(""), "wxPGPaintData", "Contains information relayed to property's OnCustomPaint.");
		cl.def( pybind11::init( [](){ return new wxPGPaintData(); } ) );
		cl.def_readwrite("m_choiceItem", &wxPGPaintData::m_choiceItem);
		cl.def_readwrite("m_drawnWidth", &wxPGPaintData::m_drawnWidth);
		cl.def_readwrite("m_drawnHeight", &wxPGPaintData::m_drawnHeight);
	}
}
