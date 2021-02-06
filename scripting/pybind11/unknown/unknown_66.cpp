#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxTranslationsLoader file: line:190
struct PyCallBack_wxTranslationsLoader : public wxTranslationsLoader {
	using wxTranslationsLoader::wxTranslationsLoader;

	class wxMsgCatalog * LoadCatalog(const class wxString & a0, const class wxString & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTranslationsLoader *>(this), "LoadCatalog");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMsgCatalog *>::value) {
				static pybind11::detail::override_caster_t<class wxMsgCatalog *> caster;
				return pybind11::detail::cast_ref<class wxMsgCatalog *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMsgCatalog *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTranslationsLoader::LoadCatalog\"");
	}
	class wxArrayString GetAvailableTranslations(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTranslationsLoader *>(this), "GetAvailableTranslations");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxArrayString>::value) {
				static pybind11::detail::override_caster_t<class wxArrayString> caster;
				return pybind11::detail::cast_ref<class wxArrayString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxArrayString>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTranslationsLoader::GetAvailableTranslations\"");
	}
};

// wxFileTranslationsLoader file: line:204
struct PyCallBack_wxFileTranslationsLoader : public wxFileTranslationsLoader {
	using wxFileTranslationsLoader::wxFileTranslationsLoader;

	class wxMsgCatalog * LoadCatalog(const class wxString & a0, const class wxString & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileTranslationsLoader *>(this), "LoadCatalog");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMsgCatalog *>::value) {
				static pybind11::detail::override_caster_t<class wxMsgCatalog *> caster;
				return pybind11::detail::cast_ref<class wxMsgCatalog *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMsgCatalog *>(std::move(o));
		}
		return wxFileTranslationsLoader::LoadCatalog(a0, a1);
	}
	class wxArrayString GetAvailableTranslations(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileTranslationsLoader *>(this), "GetAvailableTranslations");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxArrayString>::value) {
				static pybind11::detail::override_caster_t<class wxArrayString> caster;
				return pybind11::detail::cast_ref<class wxArrayString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxArrayString>(std::move(o));
		}
		return wxFileTranslationsLoader::GetAvailableTranslations(a0);
	}
};

void bind_unknown_unknown_66(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPluralFormsCalculatorPtr file: line:55
		pybind11::class_<wxPluralFormsCalculatorPtr, std::shared_ptr<wxPluralFormsCalculatorPtr>> cl(M(""), "wxPluralFormsCalculatorPtr", "");
		cl.def("swap", (void (wxPluralFormsCalculatorPtr::*)(class wxPluralFormsCalculatorPtr &)) &wxPluralFormsCalculatorPtr::swap, "C++: wxPluralFormsCalculatorPtr::swap(class wxPluralFormsCalculatorPtr &) --> void", pybind11::arg("ot"));
	}
	{ // wxMsgCatalog file: line:61
		pybind11::class_<wxMsgCatalog, std::shared_ptr<wxMsgCatalog>> cl(M(""), "wxMsgCatalog", "");
		cl.def_static("CreateFromFile", (class wxMsgCatalog * (*)(const class wxString &, const class wxString &)) &wxMsgCatalog::CreateFromFile, "C++: wxMsgCatalog::CreateFromFile(const class wxString &, const class wxString &) --> class wxMsgCatalog *", pybind11::return_value_policy::automatic, pybind11::arg("filename"), pybind11::arg("domain"));
		cl.def_static("CreateFromData", (class wxMsgCatalog * (*)(const class wxScopedCharTypeBuffer<char> &, const class wxString &)) &wxMsgCatalog::CreateFromData, "C++: wxMsgCatalog::CreateFromData(const class wxScopedCharTypeBuffer<char> &, const class wxString &) --> class wxMsgCatalog *", pybind11::return_value_policy::automatic, pybind11::arg("data"), pybind11::arg("domain"));
		cl.def("GetDomain", (class wxString (wxMsgCatalog::*)() const) &wxMsgCatalog::GetDomain, "C++: wxMsgCatalog::GetDomain() const --> class wxString");
		cl.def("GetString", [](wxMsgCatalog const &o, const class wxString & a0) -> const wxString * { return o.GetString(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("sz"));
		cl.def("GetString", (const class wxString * (wxMsgCatalog::*)(const class wxString &, unsigned int) const) &wxMsgCatalog::GetString, "C++: wxMsgCatalog::GetString(const class wxString &, unsigned int) const --> const class wxString *", pybind11::return_value_policy::automatic, pybind11::arg("sz"), pybind11::arg("n"));
	}
	{ // wxTranslations file: line:114
		pybind11::class_<wxTranslations, std::shared_ptr<wxTranslations>> cl(M(""), "wxTranslations", "");
		cl.def( pybind11::init( [](){ return new wxTranslations(); } ) );
		cl.def( pybind11::init( [](wxTranslations const &o){ return new wxTranslations(o); } ) );
		cl.def_static("Get", (class wxTranslations * (*)()) &wxTranslations::Get, "C++: wxTranslations::Get() --> class wxTranslations *", pybind11::return_value_policy::automatic);
		cl.def_static("Set", (void (*)(class wxTranslations *)) &wxTranslations::Set, "C++: wxTranslations::Set(class wxTranslations *) --> void", pybind11::arg("t"));
		cl.def("SetLoader", (void (wxTranslations::*)(class wxTranslationsLoader *)) &wxTranslations::SetLoader, "C++: wxTranslations::SetLoader(class wxTranslationsLoader *) --> void", pybind11::arg("loader"));
		cl.def("SetLanguage", (void (wxTranslations::*)(enum wxLanguage)) &wxTranslations::SetLanguage, "C++: wxTranslations::SetLanguage(enum wxLanguage) --> void", pybind11::arg("lang"));
		cl.def("SetLanguage", (void (wxTranslations::*)(const class wxString &)) &wxTranslations::SetLanguage, "C++: wxTranslations::SetLanguage(const class wxString &) --> void", pybind11::arg("lang"));
		cl.def("GetAvailableTranslations", (class wxArrayString (wxTranslations::*)(const class wxString &) const) &wxTranslations::GetAvailableTranslations, "C++: wxTranslations::GetAvailableTranslations(const class wxString &) const --> class wxArrayString", pybind11::arg("domain"));
		cl.def("GetBestTranslation", (class wxString (wxTranslations::*)(const class wxString &, enum wxLanguage)) &wxTranslations::GetBestTranslation, "C++: wxTranslations::GetBestTranslation(const class wxString &, enum wxLanguage) --> class wxString", pybind11::arg("domain"), pybind11::arg("msgIdLanguage"));
		cl.def("GetBestTranslation", [](wxTranslations &o, const class wxString & a0) -> wxString { return o.GetBestTranslation(a0); }, "", pybind11::arg("domain"));
		cl.def("GetBestTranslation", (class wxString (wxTranslations::*)(const class wxString &, const class wxString &)) &wxTranslations::GetBestTranslation, "C++: wxTranslations::GetBestTranslation(const class wxString &, const class wxString &) --> class wxString", pybind11::arg("domain"), pybind11::arg("msgIdLanguage"));
		cl.def("AddStdCatalog", (bool (wxTranslations::*)()) &wxTranslations::AddStdCatalog, "C++: wxTranslations::AddStdCatalog() --> bool");
		cl.def("AddCatalog", (bool (wxTranslations::*)(const class wxString &)) &wxTranslations::AddCatalog, "C++: wxTranslations::AddCatalog(const class wxString &) --> bool", pybind11::arg("domain"));
		cl.def("AddCatalog", (bool (wxTranslations::*)(const class wxString &, enum wxLanguage)) &wxTranslations::AddCatalog, "C++: wxTranslations::AddCatalog(const class wxString &, enum wxLanguage) --> bool", pybind11::arg("domain"), pybind11::arg("msgIdLanguage"));
		cl.def("IsLoaded", (bool (wxTranslations::*)(const class wxString &) const) &wxTranslations::IsLoaded, "C++: wxTranslations::IsLoaded(const class wxString &) const --> bool", pybind11::arg("domain"));
		cl.def("GetTranslatedString", [](wxTranslations const &o, const class wxString & a0) -> const wxString * { return o.GetTranslatedString(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("origString"));
		cl.def("GetTranslatedString", (const class wxString * (wxTranslations::*)(const class wxString &, const class wxString &) const) &wxTranslations::GetTranslatedString, "C++: wxTranslations::GetTranslatedString(const class wxString &, const class wxString &) const --> const class wxString *", pybind11::return_value_policy::automatic, pybind11::arg("origString"), pybind11::arg("domain"));
		cl.def("GetTranslatedString", [](wxTranslations const &o, const class wxString & a0, unsigned int const & a1) -> const wxString * { return o.GetTranslatedString(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("origString"), pybind11::arg("n"));
		cl.def("GetTranslatedString", (const class wxString * (wxTranslations::*)(const class wxString &, unsigned int, const class wxString &) const) &wxTranslations::GetTranslatedString, "C++: wxTranslations::GetTranslatedString(const class wxString &, unsigned int, const class wxString &) const --> const class wxString *", pybind11::return_value_policy::automatic, pybind11::arg("origString"), pybind11::arg("n"), pybind11::arg("domain"));
		cl.def("GetHeaderValue", [](wxTranslations const &o, const class wxString & a0) -> wxString { return o.GetHeaderValue(a0); }, "", pybind11::arg("header"));
		cl.def("GetHeaderValue", (class wxString (wxTranslations::*)(const class wxString &, const class wxString &) const) &wxTranslations::GetHeaderValue, "C++: wxTranslations::GetHeaderValue(const class wxString &, const class wxString &) const --> class wxString", pybind11::arg("header"), pybind11::arg("domain"));
		cl.def_static("GetUntranslatedString", (const class wxString & (*)(const class wxString &)) &wxTranslations::GetUntranslatedString, "C++: wxTranslations::GetUntranslatedString(const class wxString &) --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("assign", (class wxTranslations & (wxTranslations::*)(const class wxTranslations &)) &wxTranslations::operator=, "C++: wxTranslations::operator=(const class wxTranslations &) --> class wxTranslations &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxTranslationsLoader file: line:190
		pybind11::class_<wxTranslationsLoader, std::shared_ptr<wxTranslationsLoader>, PyCallBack_wxTranslationsLoader> cl(M(""), "wxTranslationsLoader", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxTranslationsLoader(); } ) );
		cl.def(pybind11::init<PyCallBack_wxTranslationsLoader const &>());
		cl.def("LoadCatalog", (class wxMsgCatalog * (wxTranslationsLoader::*)(const class wxString &, const class wxString &)) &wxTranslationsLoader::LoadCatalog, "C++: wxTranslationsLoader::LoadCatalog(const class wxString &, const class wxString &) --> class wxMsgCatalog *", pybind11::return_value_policy::automatic, pybind11::arg("domain"), pybind11::arg("lang"));
		cl.def("GetAvailableTranslations", (class wxArrayString (wxTranslationsLoader::*)(const class wxString &) const) &wxTranslationsLoader::GetAvailableTranslations, "C++: wxTranslationsLoader::GetAvailableTranslations(const class wxString &) const --> class wxArrayString", pybind11::arg("domain"));
		cl.def("assign", (class wxTranslationsLoader & (wxTranslationsLoader::*)(const class wxTranslationsLoader &)) &wxTranslationsLoader::operator=, "C++: wxTranslationsLoader::operator=(const class wxTranslationsLoader &) --> class wxTranslationsLoader &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxFileTranslationsLoader file: line:204
		pybind11::class_<wxFileTranslationsLoader, std::shared_ptr<wxFileTranslationsLoader>, PyCallBack_wxFileTranslationsLoader, wxTranslationsLoader> cl(M(""), "wxFileTranslationsLoader", "");
		cl.def( pybind11::init( [](){ return new wxFileTranslationsLoader(); }, [](){ return new PyCallBack_wxFileTranslationsLoader(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxFileTranslationsLoader const &o){ return new PyCallBack_wxFileTranslationsLoader(o); } ) );
		cl.def( pybind11::init( [](wxFileTranslationsLoader const &o){ return new wxFileTranslationsLoader(o); } ) );
		cl.def_static("AddCatalogLookupPathPrefix", (void (*)(const class wxString &)) &wxFileTranslationsLoader::AddCatalogLookupPathPrefix, "C++: wxFileTranslationsLoader::AddCatalogLookupPathPrefix(const class wxString &) --> void", pybind11::arg("prefix"));
		cl.def("LoadCatalog", (class wxMsgCatalog * (wxFileTranslationsLoader::*)(const class wxString &, const class wxString &)) &wxFileTranslationsLoader::LoadCatalog, "C++: wxFileTranslationsLoader::LoadCatalog(const class wxString &, const class wxString &) --> class wxMsgCatalog *", pybind11::return_value_policy::automatic, pybind11::arg("domain"), pybind11::arg("lang"));
		cl.def("GetAvailableTranslations", (class wxArrayString (wxFileTranslationsLoader::*)(const class wxString &) const) &wxFileTranslationsLoader::GetAvailableTranslations, "C++: wxFileTranslationsLoader::GetAvailableTranslations(const class wxString &) const --> class wxArrayString", pybind11::arg("domain"));
		cl.def("assign", (class wxFileTranslationsLoader & (wxFileTranslationsLoader::*)(const class wxFileTranslationsLoader &)) &wxFileTranslationsLoader::operator=, "C++: wxFileTranslationsLoader::operator=(const class wxFileTranslationsLoader &) --> class wxFileTranslationsLoader &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxGetTranslation(const class wxString &, const class wxString &) file: line:243
	M("").def("wxGetTranslation", [](const class wxString & a0) -> const wxString & { return wxGetTranslation(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("str"));
	M("").def("wxGetTranslation", (const class wxString & (*)(const class wxString &, const class wxString &)) &wxGetTranslation, "C++: wxGetTranslation(const class wxString &, const class wxString &) --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"), pybind11::arg("domain"));

	// wxGetTranslation(const class wxString &, const class wxString &, unsigned int, const class wxString &) file: line:257
	M("").def("wxGetTranslation", [](const class wxString & a0, const class wxString & a1, unsigned int const & a2) -> const wxString & { return wxGetTranslation(a0, a1, a2); }, "", pybind11::return_value_policy::automatic, pybind11::arg("str1"), pybind11::arg("str2"), pybind11::arg("n"));
	M("").def("wxGetTranslation", (const class wxString & (*)(const class wxString &, const class wxString &, unsigned int, const class wxString &)) &wxGetTranslation, "C++: wxGetTranslation(const class wxString &, const class wxString &, unsigned int, const class wxString &) --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str1"), pybind11::arg("str2"), pybind11::arg("n"), pybind11::arg("domain"));

	// wxLayoutDirection file: line:22
	pybind11::enum_<wxLayoutDirection>(M(""), "wxLayoutDirection", pybind11::arithmetic(), "")
		.value("wxLayout_Default", wxLayout_Default)
		.value("wxLayout_LeftToRight", wxLayout_LeftToRight)
		.value("wxLayout_RightToLeft", wxLayout_RightToLeft)
		.export_values();

;

	{ // wxLanguageInfo file: line:58
		pybind11::class_<wxLanguageInfo, std::shared_ptr<wxLanguageInfo>> cl(M(""), "wxLanguageInfo", "");
		cl.def( pybind11::init( [](){ return new wxLanguageInfo(); } ) );
		cl.def( pybind11::init( [](wxLanguageInfo const &o){ return new wxLanguageInfo(o); } ) );
		cl.def_readwrite("Language", &wxLanguageInfo::Language);
		cl.def_readwrite("CanonicalName", &wxLanguageInfo::CanonicalName);
		cl.def_readwrite("Description", &wxLanguageInfo::Description);
		cl.def_readwrite("LayoutDirection", &wxLanguageInfo::LayoutDirection);
		cl.def("GetLocaleName", (class wxString (wxLanguageInfo::*)() const) &wxLanguageInfo::GetLocaleName, "C++: wxLanguageInfo::GetLocaleName() const --> class wxString");
		cl.def("assign", (struct wxLanguageInfo & (wxLanguageInfo::*)(const struct wxLanguageInfo &)) &wxLanguageInfo::operator=, "C++: wxLanguageInfo::operator=(const struct wxLanguageInfo &) --> struct wxLanguageInfo &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxLocaleCategory file: line:90
	pybind11::enum_<wxLocaleCategory>(M(""), "wxLocaleCategory", pybind11::arithmetic(), "")
		.value("wxLOCALE_CAT_NUMBER", wxLOCALE_CAT_NUMBER)
		.value("wxLOCALE_CAT_DATE", wxLOCALE_CAT_DATE)
		.value("wxLOCALE_CAT_MONEY", wxLOCALE_CAT_MONEY)
		.value("wxLOCALE_CAT_DEFAULT", wxLOCALE_CAT_DEFAULT)
		.value("wxLOCALE_CAT_MAX", wxLOCALE_CAT_MAX)
		.export_values();

;

	// wxLocaleInfo file: line:112
	pybind11::enum_<wxLocaleInfo>(M(""), "wxLocaleInfo", pybind11::arithmetic(), "")
		.value("wxLOCALE_THOUSANDS_SEP", wxLOCALE_THOUSANDS_SEP)
		.value("wxLOCALE_DECIMAL_POINT", wxLOCALE_DECIMAL_POINT)
		.value("wxLOCALE_SHORT_DATE_FMT", wxLOCALE_SHORT_DATE_FMT)
		.value("wxLOCALE_LONG_DATE_FMT", wxLOCALE_LONG_DATE_FMT)
		.value("wxLOCALE_DATE_TIME_FMT", wxLOCALE_DATE_TIME_FMT)
		.value("wxLOCALE_TIME_FMT", wxLOCALE_TIME_FMT)
		.export_values();

;

	// wxLocaleInitFlags file: line:137
	pybind11::enum_<wxLocaleInitFlags>(M(""), "wxLocaleInitFlags", pybind11::arithmetic(), "")
		.value("wxLOCALE_DONT_LOAD_DEFAULT", wxLOCALE_DONT_LOAD_DEFAULT)
		.value("wxLOCALE_LOAD_DEFAULT", wxLOCALE_LOAD_DEFAULT)
		.value("wxLOCALE_CONV_ENCODING", wxLOCALE_CONV_ENCODING)
		.export_values();

;

	{ // wxLocale file: line:147
		pybind11::class_<wxLocale, std::shared_ptr<wxLocale>> cl(M(""), "wxLocale", "");
		cl.def( pybind11::init( [](){ return new wxLocale(); } ) );
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxLocale(a0); } ), "doc" , pybind11::arg("name"));
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1){ return new wxLocale(a0, a1); } ), "doc" , pybind11::arg("name"), pybind11::arg("shortName"));
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1, const class wxString & a2){ return new wxLocale(a0, a1, a2); } ), "doc" , pybind11::arg("name"), pybind11::arg("shortName"), pybind11::arg("locale"));
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1, const class wxString & a2, bool const & a3){ return new wxLocale(a0, a1, a2, a3); } ), "doc" , pybind11::arg("name"), pybind11::arg("shortName"), pybind11::arg("locale"), pybind11::arg("bLoadDefault"));
		cl.def( pybind11::init<const class wxString &, const class wxString &, const class wxString &, bool, bool>(), pybind11::arg("name"), pybind11::arg("shortName"), pybind11::arg("locale"), pybind11::arg("bLoadDefault"), pybind11::arg("bConvertEncoding") );

		cl.def( pybind11::init( [](int const & a0){ return new wxLocale(a0); } ), "doc" , pybind11::arg("language"));
		cl.def( pybind11::init<int, int>(), pybind11::arg("language"), pybind11::arg("flags") );

		cl.def("Init", [](wxLocale &o, const class wxString & a0) -> bool { return o.Init(a0); }, "", pybind11::arg("name"));
		cl.def("Init", [](wxLocale &o, const class wxString & a0, const class wxString & a1) -> bool { return o.Init(a0, a1); }, "", pybind11::arg("name"), pybind11::arg("shortName"));
		cl.def("Init", [](wxLocale &o, const class wxString & a0, const class wxString & a1, const class wxString & a2) -> bool { return o.Init(a0, a1, a2); }, "", pybind11::arg("name"), pybind11::arg("shortName"), pybind11::arg("locale"));
		cl.def("Init", [](wxLocale &o, const class wxString & a0, const class wxString & a1, const class wxString & a2, bool const & a3) -> bool { return o.Init(a0, a1, a2, a3); }, "", pybind11::arg("name"), pybind11::arg("shortName"), pybind11::arg("locale"), pybind11::arg("bLoadDefault"));
		cl.def("Init", (bool (wxLocale::*)(const class wxString &, const class wxString &, const class wxString &, bool, bool)) &wxLocale::Init, "C++: wxLocale::Init(const class wxString &, const class wxString &, const class wxString &, bool, bool) --> bool", pybind11::arg("name"), pybind11::arg("shortName"), pybind11::arg("locale"), pybind11::arg("bLoadDefault"), pybind11::arg("bConvertEncoding"));
		cl.def("Init", [](wxLocale &o) -> bool { return o.Init(); }, "");
		cl.def("Init", [](wxLocale &o, int const & a0) -> bool { return o.Init(a0); }, "", pybind11::arg("language"));
		cl.def("Init", (bool (wxLocale::*)(int, int)) &wxLocale::Init, "C++: wxLocale::Init(int, int) --> bool", pybind11::arg("language"), pybind11::arg("flags"));
		cl.def_static("GetSystemLanguage", (int (*)()) &wxLocale::GetSystemLanguage, "C++: wxLocale::GetSystemLanguage() --> int");
		cl.def_static("GetSystemEncoding", (enum wxFontEncoding (*)()) &wxLocale::GetSystemEncoding, "C++: wxLocale::GetSystemEncoding() --> enum wxFontEncoding");
		cl.def_static("GetSystemEncodingName", (class wxString (*)()) &wxLocale::GetSystemEncodingName, "C++: wxLocale::GetSystemEncodingName() --> class wxString");
		cl.def_static("GetInfo", [](enum wxLocaleInfo const & a0) -> wxString { return wxLocale::GetInfo(a0); }, "", pybind11::arg("index"));
		cl.def_static("GetInfo", (class wxString (*)(enum wxLocaleInfo, enum wxLocaleCategory)) &wxLocale::GetInfo, "C++: wxLocale::GetInfo(enum wxLocaleInfo, enum wxLocaleCategory) --> class wxString", pybind11::arg("index"), pybind11::arg("cat"));
		cl.def("IsOk", (bool (wxLocale::*)() const) &wxLocale::IsOk, "C++: wxLocale::IsOk() const --> bool");
		cl.def("GetLocale", (const class wxString & (wxLocale::*)() const) &wxLocale::GetLocale, "C++: wxLocale::GetLocale() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetLanguage", (int (wxLocale::*)() const) &wxLocale::GetLanguage, "C++: wxLocale::GetLanguage() const --> int");
		cl.def("GetSysName", (class wxString (wxLocale::*)() const) &wxLocale::GetSysName, "C++: wxLocale::GetSysName() const --> class wxString");
		cl.def("GetCanonicalName", (class wxString (wxLocale::*)() const) &wxLocale::GetCanonicalName, "C++: wxLocale::GetCanonicalName() const --> class wxString");
		cl.def_static("AddCatalogLookupPathPrefix", (void (*)(const class wxString &)) &wxLocale::AddCatalogLookupPathPrefix, "C++: wxLocale::AddCatalogLookupPathPrefix(const class wxString &) --> void", pybind11::arg("prefix"));
		cl.def("AddCatalog", (bool (wxLocale::*)(const class wxString &)) &wxLocale::AddCatalog, "C++: wxLocale::AddCatalog(const class wxString &) --> bool", pybind11::arg("domain"));
		cl.def("AddCatalog", (bool (wxLocale::*)(const class wxString &, enum wxLanguage)) &wxLocale::AddCatalog, "C++: wxLocale::AddCatalog(const class wxString &, enum wxLanguage) --> bool", pybind11::arg("domain"), pybind11::arg("msgIdLanguage"));
		cl.def("AddCatalog", (bool (wxLocale::*)(const class wxString &, enum wxLanguage, const class wxString &)) &wxLocale::AddCatalog, "C++: wxLocale::AddCatalog(const class wxString &, enum wxLanguage, const class wxString &) --> bool", pybind11::arg("domain"), pybind11::arg("msgIdLanguage"), pybind11::arg("msgIdCharset"));
		cl.def_static("IsAvailable", (bool (*)(int)) &wxLocale::IsAvailable, "C++: wxLocale::IsAvailable(int) --> bool", pybind11::arg("lang"));
		cl.def("IsLoaded", (bool (wxLocale::*)(const class wxString &) const) &wxLocale::IsLoaded, "C++: wxLocale::IsLoaded(const class wxString &) const --> bool", pybind11::arg("domain"));
		cl.def_static("GetLanguageInfo", (const struct wxLanguageInfo * (*)(int)) &wxLocale::GetLanguageInfo, "C++: wxLocale::GetLanguageInfo(int) --> const struct wxLanguageInfo *", pybind11::return_value_policy::automatic, pybind11::arg("lang"));
		cl.def_static("GetLanguageName", (class wxString (*)(int)) &wxLocale::GetLanguageName, "C++: wxLocale::GetLanguageName(int) --> class wxString", pybind11::arg("lang"));
		cl.def_static("GetLanguageCanonicalName", (class wxString (*)(int)) &wxLocale::GetLanguageCanonicalName, "C++: wxLocale::GetLanguageCanonicalName(int) --> class wxString", pybind11::arg("lang"));
		cl.def_static("FindLanguageInfo", (const struct wxLanguageInfo * (*)(const class wxString &)) &wxLocale::FindLanguageInfo, "C++: wxLocale::FindLanguageInfo(const class wxString &) --> const struct wxLanguageInfo *", pybind11::return_value_policy::automatic, pybind11::arg("locale"));
		cl.def_static("AddLanguage", (void (*)(const struct wxLanguageInfo &)) &wxLocale::AddLanguage, "C++: wxLocale::AddLanguage(const struct wxLanguageInfo &) --> void", pybind11::arg("info"));
		cl.def("GetString", [](wxLocale const &o, const class wxString & a0) -> const wxString & { return o.GetString(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("origString"));
		cl.def("GetString", (const class wxString & (wxLocale::*)(const class wxString &, const class wxString &) const) &wxLocale::GetString, "C++: wxLocale::GetString(const class wxString &, const class wxString &) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("origString"), pybind11::arg("domain"));
		cl.def("GetString", [](wxLocale const &o, const class wxString & a0, const class wxString & a1, unsigned int const & a2) -> const wxString & { return o.GetString(a0, a1, a2); }, "", pybind11::return_value_policy::automatic, pybind11::arg("origString"), pybind11::arg("origString2"), pybind11::arg("n"));
		cl.def("GetString", (const class wxString & (wxLocale::*)(const class wxString &, const class wxString &, unsigned int, const class wxString &) const) &wxLocale::GetString, "C++: wxLocale::GetString(const class wxString &, const class wxString &, unsigned int, const class wxString &) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("origString"), pybind11::arg("origString2"), pybind11::arg("n"), pybind11::arg("domain"));
		cl.def("GetName", (const class wxString & (wxLocale::*)() const) &wxLocale::GetName, "C++: wxLocale::GetName() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetHeaderValue", [](wxLocale const &o, const class wxString & a0) -> wxString { return o.GetHeaderValue(a0); }, "", pybind11::arg("header"));
		cl.def("GetHeaderValue", (class wxString (wxLocale::*)(const class wxString &, const class wxString &) const) &wxLocale::GetHeaderValue, "C++: wxLocale::GetHeaderValue(const class wxString &, const class wxString &) const --> class wxString", pybind11::arg("header"), pybind11::arg("domain"));
		cl.def_static("CreateLanguagesDB", (void (*)()) &wxLocale::CreateLanguagesDB, "C++: wxLocale::CreateLanguagesDB() --> void");
		cl.def_static("DestroyLanguagesDB", (void (*)()) &wxLocale::DestroyLanguagesDB, "C++: wxLocale::DestroyLanguagesDB() --> void");
	}
}
