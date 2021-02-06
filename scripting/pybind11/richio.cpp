#include <iterator> // __gnu_cxx::__normal_iterator
#include <lib_id.h> // LIB_ID
#include <lib_tree_item.h> // LIB_TREE_ITEM
#include <memory> // std::allocator
#include <richio.h> // FILE_OUTPUTFORMATTER
#include <richio.h> // OUTPUTFORMATTER
#include <richio.h> // STREAM_OUTPUTFORMATTER
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <utf8.h> // UTF8

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// FILE_OUTPUTFORMATTER file:richio.h line:453
struct PyCallBack_FILE_OUTPUTFORMATTER : public FILE_OUTPUTFORMATTER {
	using FILE_OUTPUTFORMATTER::FILE_OUTPUTFORMATTER;

	void write(const char * a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const FILE_OUTPUTFORMATTER *>(this), "write");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return FILE_OUTPUTFORMATTER::write(a0, a1);
	}
	const char * GetQuoteChar(const char * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const FILE_OUTPUTFORMATTER *>(this), "GetQuoteChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<const char *>::value) {
				static pybind11::detail::override_caster_t<const char *> caster;
				return pybind11::detail::cast_ref<const char *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const char *>(std::move(o));
		}
		return OUTPUTFORMATTER::GetQuoteChar(a0);
	}
};

// STREAM_OUTPUTFORMATTER file:richio.h line:483
struct PyCallBack_STREAM_OUTPUTFORMATTER : public STREAM_OUTPUTFORMATTER {
	using STREAM_OUTPUTFORMATTER::STREAM_OUTPUTFORMATTER;

	void write(const char * a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const STREAM_OUTPUTFORMATTER *>(this), "write");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return STREAM_OUTPUTFORMATTER::write(a0, a1);
	}
	const char * GetQuoteChar(const char * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const STREAM_OUTPUTFORMATTER *>(this), "GetQuoteChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<const char *>::value) {
				static pybind11::detail::override_caster_t<const char *> caster;
				return pybind11::detail::cast_ref<const char *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const char *>(std::move(o));
		}
		return OUTPUTFORMATTER::GetQuoteChar(a0);
	}
};

// LIB_TREE_ITEM file:lib_tree_item.h line:39
struct PyCallBack_LIB_TREE_ITEM : public LIB_TREE_ITEM {
	using LIB_TREE_ITEM::LIB_TREE_ITEM;

	class LIB_ID GetLibId() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TREE_ITEM *>(this), "GetLibId");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class LIB_ID>::value) {
				static pybind11::detail::override_caster_t<class LIB_ID> caster;
				return pybind11::detail::cast_ref<class LIB_ID>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class LIB_ID>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"LIB_TREE_ITEM::GetLibId\"");
	}
	class wxString GetName() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TREE_ITEM *>(this), "GetName");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"LIB_TREE_ITEM::GetName\"");
	}
	class wxString GetLibNickname() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TREE_ITEM *>(this), "GetLibNickname");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"LIB_TREE_ITEM::GetLibNickname\"");
	}
	class wxString GetDescription() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TREE_ITEM *>(this), "GetDescription");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"LIB_TREE_ITEM::GetDescription\"");
	}
	class wxString GetSearchText() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TREE_ITEM *>(this), "GetSearchText");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_TREE_ITEM::GetSearchText();
	}
	bool IsRoot() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TREE_ITEM *>(this), "IsRoot");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return LIB_TREE_ITEM::IsRoot();
	}
	int GetUnitCount() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TREE_ITEM *>(this), "GetUnitCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return LIB_TREE_ITEM::GetUnitCount();
	}
	class wxString GetUnitReference(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LIB_TREE_ITEM *>(this), "GetUnitReference");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return LIB_TREE_ITEM::GetUnitReference(a0);
	}
};

void bind_richio(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // FILE_OUTPUTFORMATTER file:richio.h line:453
		pybind11::class_<FILE_OUTPUTFORMATTER, std::shared_ptr<FILE_OUTPUTFORMATTER>, PyCallBack_FILE_OUTPUTFORMATTER, OUTPUTFORMATTER> cl(M(""), "FILE_OUTPUTFORMATTER", "Used for text file output.\n\n It is about 8 times faster than STREAM_OUTPUTFORMATTER for file streams.");
		cl.def( pybind11::init( [](const class wxString & a0){ return new FILE_OUTPUTFORMATTER(a0); }, [](const class wxString & a0){ return new PyCallBack_FILE_OUTPUTFORMATTER(a0); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const wchar_t * a1){ return new FILE_OUTPUTFORMATTER(a0, a1); }, [](const class wxString & a0, const wchar_t * a1){ return new PyCallBack_FILE_OUTPUTFORMATTER(a0, a1); } ), "doc");
		cl.def( pybind11::init<const class wxString &, const wchar_t *, char>(), pybind11::arg("aFileName"), pybind11::arg("aMode"), pybind11::arg("aQuoteChar") );

		cl.def( pybind11::init( [](PyCallBack_FILE_OUTPUTFORMATTER const &o){ return new PyCallBack_FILE_OUTPUTFORMATTER(o); } ) );
		cl.def( pybind11::init( [](FILE_OUTPUTFORMATTER const &o){ return new FILE_OUTPUTFORMATTER(o); } ) );
		cl.def("assign", (class FILE_OUTPUTFORMATTER & (FILE_OUTPUTFORMATTER::*)(const class FILE_OUTPUTFORMATTER &)) &FILE_OUTPUTFORMATTER::operator=, "C++: FILE_OUTPUTFORMATTER::operator=(const class FILE_OUTPUTFORMATTER &) --> class FILE_OUTPUTFORMATTER &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // STREAM_OUTPUTFORMATTER file:richio.h line:483
		pybind11::class_<STREAM_OUTPUTFORMATTER, std::shared_ptr<STREAM_OUTPUTFORMATTER>, PyCallBack_STREAM_OUTPUTFORMATTER, OUTPUTFORMATTER> cl(M(""), "STREAM_OUTPUTFORMATTER", "Implement an #OUTPUTFORMATTER to a wxWidgets wxOutputStream.\n\n The stream is neither opened nor closed by this class.");
		cl.def( pybind11::init( [](class wxOutputStream & a0){ return new STREAM_OUTPUTFORMATTER(a0); }, [](class wxOutputStream & a0){ return new PyCallBack_STREAM_OUTPUTFORMATTER(a0); } ), "doc");
		cl.def( pybind11::init<class wxOutputStream &, char>(), pybind11::arg("aStream"), pybind11::arg("aQuoteChar") );

		cl.def( pybind11::init( [](PyCallBack_STREAM_OUTPUTFORMATTER const &o){ return new PyCallBack_STREAM_OUTPUTFORMATTER(o); } ) );
		cl.def( pybind11::init( [](STREAM_OUTPUTFORMATTER const &o){ return new STREAM_OUTPUTFORMATTER(o); } ) );
		cl.def("assign", (class STREAM_OUTPUTFORMATTER & (STREAM_OUTPUTFORMATTER::*)(const class STREAM_OUTPUTFORMATTER &)) &STREAM_OUTPUTFORMATTER::operator=, "C++: STREAM_OUTPUTFORMATTER::operator=(const class STREAM_OUTPUTFORMATTER &) --> class STREAM_OUTPUTFORMATTER &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // LIB_ID file:lib_id.h line:51
		pybind11::class_<LIB_ID, std::shared_ptr<LIB_ID>> cl(M(""), "LIB_ID", "A logical library item identifier and consists of various portions much like a URI.\n\n It consists of of triad of the library nickname, the name of the item in the library,\n and an optional revision of the item.  This is a generic library identifier that can be\n used for any type of library that contains multiple named items such as footprint or\n symbol libraries.\n\n Example LIB_ID string:\n \"smt:R_0805/rev0\".\n\n - \"smt\" is the logical library name used to look up library information saved in the #LIB_TABLE.\n - \"R\" is the name of the item within the library.\n - \"rev0\" is the revision, which is optional.  If missing then its delimiter should also not\n    be present. A revision must begin with \"rev\" and be followed by at least one or more\n    decimal digits.\n\n \n Dick Hollenbeck");
		cl.def( pybind11::init( [](){ return new LIB_ID(); } ) );
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1){ return new LIB_ID(a0, a1); } ), "doc" , pybind11::arg("aLibraryName"), pybind11::arg("aItemName"));
		cl.def( pybind11::init<const class wxString &, const class wxString &, const class wxString &>(), pybind11::arg("aLibraryName"), pybind11::arg("aItemName"), pybind11::arg("aRevision") );

		cl.def( pybind11::init( [](LIB_ID const &o){ return new LIB_ID(o); } ) );
		cl.def("Parse", [](LIB_ID &o, const class UTF8 & a0) -> int { return o.Parse(a0); }, "", pybind11::arg("aId"));
		cl.def("Parse", (int (LIB_ID::*)(const class UTF8 &, bool)) &LIB_ID::Parse, "Parse LIB_ID with the information from \n\n A typical LIB_ID string consists of a library nickname followed by a library item name.\n e.g.: \"smt:R_0805\", or\n e.g.: \"mylib:R_0805\", or\n e.g.: \"ttl:7400\"\n\n \n is the string to populate the #LIB_ID object.\n \n\n indicates invalid chars should be replaced with '_'.\n\n \n minus 1 (i.e. -1) means success, >= 0 indicates the character offset into\n         aId at which an error was detected.\n\nC++: LIB_ID::Parse(const class UTF8 &, bool) --> int", pybind11::arg("aId"), pybind11::arg("aFix"));
		cl.def("GetLibNickname", (const class UTF8 & (LIB_ID::*)() const) &LIB_ID::GetLibNickname, "Return the logical library name portion of a LIB_ID.\n\nC++: LIB_ID::GetLibNickname() const --> const class UTF8 &", pybind11::return_value_policy::automatic);
		cl.def("SetLibNickname", (int (LIB_ID::*)(const class UTF8 &)) &LIB_ID::SetLibNickname, "Override the logical library name portion of the LIB_ID to \n\n \n int - minus 1 (i.e. -1) means success, >= 0 indicates the  character offset\n               into the parameter at which an error was detected, usually because it\n               contained '/' or ':'.\n\nC++: LIB_ID::SetLibNickname(const class UTF8 &) --> int", pybind11::arg("aNickname"));
		cl.def("GetLibItemName", (const class UTF8 & (LIB_ID::*)() const) &LIB_ID::GetLibItemName, "the library item name, i.e. footprintName, in UTF8.\n\nC++: LIB_ID::GetLibItemName() const --> const class UTF8 &", pybind11::return_value_policy::automatic);
		cl.def("GetUniStringLibItemName", (const class wxString (LIB_ID::*)() const) &LIB_ID::GetUniStringLibItemName, "Get strings for display messages in dialogs.\n\n Equivalent to m_itemName.wx_str(), but more explicit when building a Unicode string\n in messages.\n\n \n the library item name, i.e. footprintName in a wxString (UTF16 or 32).\n\nC++: LIB_ID::GetUniStringLibItemName() const --> const class wxString");
		cl.def("SetLibItemName", [](LIB_ID &o, const class UTF8 & a0) -> int { return o.SetLibItemName(a0); }, "", pybind11::arg("aLibItemName"));
		cl.def("SetLibItemName", (int (LIB_ID::*)(const class UTF8 &, bool)) &LIB_ID::SetLibItemName, "Override the library item name portion of the LIB_ID to \n\n \n int - minus 1 (i.e. -1) means success, >= 0 indicates the  character offset\n               into the parameter at which an error was detected, usually because it\n               contained '/'.\n\nC++: LIB_ID::SetLibItemName(const class UTF8 &, bool) --> int", pybind11::arg("aLibItemName"), pybind11::arg("aTestForRev"));
		cl.def("SetRevision", (int (LIB_ID::*)(const class UTF8 &)) &LIB_ID::SetRevision, "C++: LIB_ID::SetRevision(const class UTF8 &) --> int", pybind11::arg("aRevision"));
		cl.def("GetRevision", (const class UTF8 & (LIB_ID::*)() const) &LIB_ID::GetRevision, "C++: LIB_ID::GetRevision() const --> const class UTF8 &", pybind11::return_value_policy::automatic);
		cl.def("GetLibItemNameAndRev", (class UTF8 (LIB_ID::*)() const) &LIB_ID::GetLibItemNameAndRev, "C++: LIB_ID::GetLibItemNameAndRev() const --> class UTF8");
		cl.def("Format", (class UTF8 (LIB_ID::*)() const) &LIB_ID::Format, "the fully formatted text of the LIB_ID in a UTF8 string.\n\nC++: LIB_ID::Format() const --> class UTF8");
		cl.def("GetUniStringLibId", (class wxString (LIB_ID::*)() const) &LIB_ID::GetUniStringLibId, "the fully formatted text of the LIB_ID in a wxString (UTF16 or UTF32),\n         suitable to display the LIB_ID in dialogs.\n\nC++: LIB_ID::GetUniStringLibId() const --> class wxString");
		cl.def_static("Format", [](const class UTF8 & a0, const class UTF8 & a1) -> UTF8 { return LIB_ID::Format(a0, a1); }, "", pybind11::arg("aLibraryName"), pybind11::arg("aLibItemName"));
		cl.def_static("Format", (class UTF8 (*)(const class UTF8 &, const class UTF8 &, const class UTF8 &)) &LIB_ID::Format, "a string in the proper format as an LIB_ID for a combination of\n         aLibraryName, aLibItemName, and aRevision.\n\n \n PARSE_ERROR if any of the pieces are illegal.\n\nC++: LIB_ID::Format(const class UTF8 &, const class UTF8 &, const class UTF8 &) --> class UTF8", pybind11::arg("aLibraryName"), pybind11::arg("aLibItemName"), pybind11::arg("aRevision"));
		cl.def("IsValid", (bool (LIB_ID::*)() const) &LIB_ID::IsValid, "Check if this LID_ID is valid.\n\n A valid #LIB_ID must have both the library nickname and the library item name defined.\n The revision field is optional.\n\n \n A return value of true does not indicated that the #LIB_ID is a valid #LIB_TABLE\n       entry.\n\n \n true is the #LIB_ID is valid.\n\n     \n\nC++: LIB_ID::IsValid() const --> bool");
		cl.def("IsLegacy", (bool (LIB_ID::*)() const) &LIB_ID::IsLegacy, "true if the #LIB_ID only has the #m_itemName name defined.\n\nC++: LIB_ID::IsLegacy() const --> bool");
		cl.def("clear", (void (LIB_ID::*)()) &LIB_ID::clear, "Clear the contents of the library nickname, library entry name, and revision strings.\n\nC++: LIB_ID::clear() --> void");
		cl.def("empty", (bool (LIB_ID::*)() const) &LIB_ID::empty, "a boolean true value if the LIB_ID is empty.  Otherwise return false.\n\nC++: LIB_ID::empty() const --> bool");
		cl.def("compare", (int (LIB_ID::*)(const class LIB_ID &) const) &LIB_ID::compare, "Compare the contents of LIB_ID objects by performing a std::string comparison of the\n library nickname, library entry name, and revision strings respectively.\n\n \n is the LIB_ID to compare against.\n \n\n -1 if less than  1 if greater than  and 0 if equal to \n     \n\nC++: LIB_ID::compare(const class LIB_ID &) const --> int", pybind11::arg("aLibId"));
		cl.def("__eq__", (bool (LIB_ID::*)(const class LIB_ID &) const) &LIB_ID::operator==, "C++: LIB_ID::operator==(const class LIB_ID &) const --> bool", pybind11::arg("aLibId"));
		cl.def("__ne__", (bool (LIB_ID::*)(const class LIB_ID &) const) &LIB_ID::operator!=, "C++: LIB_ID::operator!=(const class LIB_ID &) const --> bool", pybind11::arg("aLibId"));
		cl.def_static("HasIllegalChars", (int (*)(const class UTF8 &)) &LIB_ID::HasIllegalChars, "Examine  for invalid #LIB_ID item name characters.\n\n \n is the #LIB_ID name to test for illegal characters.\n \n\n offset of first illegal character otherwise -1.\n\nC++: LIB_ID::HasIllegalChars(const class UTF8 &) --> int", pybind11::arg("aLibItemName"));
		cl.def_static("FixIllegalChars", [](const class UTF8 & a0) -> UTF8 { return LIB_ID::FixIllegalChars(a0); }, "", pybind11::arg("aLibItemName"));
		cl.def_static("FixIllegalChars", (class UTF8 (*)(const class UTF8 &, bool)) &LIB_ID::FixIllegalChars, "Replace illegal #LIB_ID item name characters with underscores '_'.\n\n \n is the #LIB_ID item name to replace illegal characters.\n \n\n True if we are checking library names, false if we are checking item names\n \n\n the corrected version of \n     \n\nC++: LIB_ID::FixIllegalChars(const class UTF8 &, bool) --> class UTF8", pybind11::arg("aLibItemName"), pybind11::arg("aLib"));
		cl.def_static("FindIllegalLibraryNameChar", (unsigned int (*)(const class UTF8 &)) &LIB_ID::FindIllegalLibraryNameChar, "Looks for characters that are illegal in library nicknames.\n\n \n is the logical library name to be tested.\n \n\n Invalid character found in the name or 0 is the name is valid.\n\nC++: LIB_ID::FindIllegalLibraryNameChar(const class UTF8 &) --> unsigned int", pybind11::arg("aLibraryName"));
		cl.def("assign", (class LIB_ID & (LIB_ID::*)(const class LIB_ID &)) &LIB_ID::operator=, "C++: LIB_ID::operator=(const class LIB_ID &) --> class LIB_ID &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // LIB_TREE_ITEM file:lib_tree_item.h line:39
		pybind11::class_<LIB_TREE_ITEM, std::shared_ptr<LIB_TREE_ITEM>, PyCallBack_LIB_TREE_ITEM> cl(M(""), "LIB_TREE_ITEM", "A mix-in to provide polymorphism between items stored in libraries (symbols, aliases\n and footprints).\n\n It is used primarily to drive the component tree for library browsing and editing.");
		cl.def( pybind11::init( [](){ return new PyCallBack_LIB_TREE_ITEM(); } ) );
		cl.def("GetLibId", (class LIB_ID (LIB_TREE_ITEM::*)() const) &LIB_TREE_ITEM::GetLibId, "C++: LIB_TREE_ITEM::GetLibId() const --> class LIB_ID");
		cl.def("GetName", (class wxString (LIB_TREE_ITEM::*)() const) &LIB_TREE_ITEM::GetName, "C++: LIB_TREE_ITEM::GetName() const --> class wxString");
		cl.def("GetLibNickname", (class wxString (LIB_TREE_ITEM::*)() const) &LIB_TREE_ITEM::GetLibNickname, "C++: LIB_TREE_ITEM::GetLibNickname() const --> class wxString");
		cl.def("GetDescription", (class wxString (LIB_TREE_ITEM::*)()) &LIB_TREE_ITEM::GetDescription, "C++: LIB_TREE_ITEM::GetDescription() --> class wxString");
		cl.def("GetSearchText", (class wxString (LIB_TREE_ITEM::*)()) &LIB_TREE_ITEM::GetSearchText, "C++: LIB_TREE_ITEM::GetSearchText() --> class wxString");
		cl.def("IsRoot", (bool (LIB_TREE_ITEM::*)() const) &LIB_TREE_ITEM::IsRoot, "For items having aliases, IsRoot() indicates the principal item.\n\nC++: LIB_TREE_ITEM::IsRoot() const --> bool");
		cl.def("GetUnitCount", (int (LIB_TREE_ITEM::*)() const) &LIB_TREE_ITEM::GetUnitCount, "For items with units, return the number of units.\n\nC++: LIB_TREE_ITEM::GetUnitCount() const --> int");
		cl.def("GetUnitReference", (class wxString (LIB_TREE_ITEM::*)(int)) &LIB_TREE_ITEM::GetUnitReference, "For items with units, return an identifier for unit x.\n\nC++: LIB_TREE_ITEM::GetUnitReference(int) --> class wxString", pybind11::arg("aUnit"));
		cl.def("assign", (class LIB_TREE_ITEM & (LIB_TREE_ITEM::*)(const class LIB_TREE_ITEM &)) &LIB_TREE_ITEM::operator=, "C++: LIB_TREE_ITEM::operator=(const class LIB_TREE_ITEM &) --> class LIB_TREE_ITEM &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
