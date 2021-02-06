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

// wxRegionIterator file: line:97
struct PyCallBack_wxRegionIterator : public wxRegionIterator {
	using wxRegionIterator::wxRegionIterator;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionIterator *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxRegionIterator::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionIterator *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxRegionIterator *>(this), "CloneRefData");
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

void bind_unknown_unknown_65(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxRegionIterator file: line:97
		pybind11::class_<wxRegionIterator, std::shared_ptr<wxRegionIterator>, PyCallBack_wxRegionIterator, wxObject> cl(M(""), "wxRegionIterator", "");
		cl.def( pybind11::init( [](){ return new wxRegionIterator(); }, [](){ return new PyCallBack_wxRegionIterator(); } ) );
		cl.def( pybind11::init<const class wxRegion &>(), pybind11::arg("region") );

		cl.def( pybind11::init( [](PyCallBack_wxRegionIterator const &o){ return new PyCallBack_wxRegionIterator(o); } ) );
		cl.def( pybind11::init( [](wxRegionIterator const &o){ return new wxRegionIterator(o); } ) );
		cl.def("assign", (class wxRegionIterator & (wxRegionIterator::*)(const class wxRegionIterator &)) &wxRegionIterator::operator=, "C++: wxRegionIterator::operator=(const class wxRegionIterator &) --> class wxRegionIterator &", pybind11::return_value_policy::automatic, pybind11::arg("ri"));
		cl.def("Reset", (void (wxRegionIterator::*)()) &wxRegionIterator::Reset, "C++: wxRegionIterator::Reset() --> void");
		cl.def("Reset", (void (wxRegionIterator::*)(const class wxRegion &)) &wxRegionIterator::Reset, "C++: wxRegionIterator::Reset(const class wxRegion &) --> void", pybind11::arg("region"));
		cl.def("HaveRects", (bool (wxRegionIterator::*)() const) &wxRegionIterator::HaveRects, "C++: wxRegionIterator::HaveRects() const --> bool");
		cl.def("plus_plus", (class wxRegionIterator & (wxRegionIterator::*)()) &wxRegionIterator::operator++, "C++: wxRegionIterator::operator++() --> class wxRegionIterator &", pybind11::return_value_policy::automatic);
		cl.def("plus_plus", (class wxRegionIterator (wxRegionIterator::*)(int)) &wxRegionIterator::operator++, "C++: wxRegionIterator::operator++(int) --> class wxRegionIterator", pybind11::arg(""));
		cl.def("GetX", (int (wxRegionIterator::*)() const) &wxRegionIterator::GetX, "C++: wxRegionIterator::GetX() const --> int");
		cl.def("GetY", (int (wxRegionIterator::*)() const) &wxRegionIterator::GetY, "C++: wxRegionIterator::GetY() const --> int");
		cl.def("GetW", (int (wxRegionIterator::*)() const) &wxRegionIterator::GetW, "C++: wxRegionIterator::GetW() const --> int");
		cl.def("GetWidth", (int (wxRegionIterator::*)() const) &wxRegionIterator::GetWidth, "C++: wxRegionIterator::GetWidth() const --> int");
		cl.def("GetH", (int (wxRegionIterator::*)() const) &wxRegionIterator::GetH, "C++: wxRegionIterator::GetH() const --> int");
		cl.def("GetHeight", (int (wxRegionIterator::*)() const) &wxRegionIterator::GetHeight, "C++: wxRegionIterator::GetHeight() const --> int");
		cl.def("GetRect", (class wxRect (wxRegionIterator::*)() const) &wxRegionIterator::GetRect, "C++: wxRegionIterator::GetRect() const --> class wxRect");
		cl.def("GetClassInfo", (class wxClassInfo * (wxRegionIterator::*)() const) &wxRegionIterator::GetClassInfo, "C++: wxRegionIterator::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxRegionIterator::wxCreateObject, "C++: wxRegionIterator::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	// wxLanguage file: line:32
	pybind11::enum_<wxLanguage>(M(""), "wxLanguage", pybind11::arithmetic(), "The languages supported by wxLocale.\n\n    This enum is generated by misc/languages/genlang.py\n    When making changes, please put them into misc/languages/langtabl.txt")
		.value("wxLANGUAGE_DEFAULT", wxLANGUAGE_DEFAULT)
		.value("wxLANGUAGE_UNKNOWN", wxLANGUAGE_UNKNOWN)
		.value("wxLANGUAGE_ABKHAZIAN", wxLANGUAGE_ABKHAZIAN)
		.value("wxLANGUAGE_AFAR", wxLANGUAGE_AFAR)
		.value("wxLANGUAGE_AFRIKAANS", wxLANGUAGE_AFRIKAANS)
		.value("wxLANGUAGE_ALBANIAN", wxLANGUAGE_ALBANIAN)
		.value("wxLANGUAGE_AMHARIC", wxLANGUAGE_AMHARIC)
		.value("wxLANGUAGE_ARABIC", wxLANGUAGE_ARABIC)
		.value("wxLANGUAGE_ARABIC_ALGERIA", wxLANGUAGE_ARABIC_ALGERIA)
		.value("wxLANGUAGE_ARABIC_BAHRAIN", wxLANGUAGE_ARABIC_BAHRAIN)
		.value("wxLANGUAGE_ARABIC_EGYPT", wxLANGUAGE_ARABIC_EGYPT)
		.value("wxLANGUAGE_ARABIC_IRAQ", wxLANGUAGE_ARABIC_IRAQ)
		.value("wxLANGUAGE_ARABIC_JORDAN", wxLANGUAGE_ARABIC_JORDAN)
		.value("wxLANGUAGE_ARABIC_KUWAIT", wxLANGUAGE_ARABIC_KUWAIT)
		.value("wxLANGUAGE_ARABIC_LEBANON", wxLANGUAGE_ARABIC_LEBANON)
		.value("wxLANGUAGE_ARABIC_LIBYA", wxLANGUAGE_ARABIC_LIBYA)
		.value("wxLANGUAGE_ARABIC_MOROCCO", wxLANGUAGE_ARABIC_MOROCCO)
		.value("wxLANGUAGE_ARABIC_OMAN", wxLANGUAGE_ARABIC_OMAN)
		.value("wxLANGUAGE_ARABIC_QATAR", wxLANGUAGE_ARABIC_QATAR)
		.value("wxLANGUAGE_ARABIC_SAUDI_ARABIA", wxLANGUAGE_ARABIC_SAUDI_ARABIA)
		.value("wxLANGUAGE_ARABIC_SUDAN", wxLANGUAGE_ARABIC_SUDAN)
		.value("wxLANGUAGE_ARABIC_SYRIA", wxLANGUAGE_ARABIC_SYRIA)
		.value("wxLANGUAGE_ARABIC_TUNISIA", wxLANGUAGE_ARABIC_TUNISIA)
		.value("wxLANGUAGE_ARABIC_UAE", wxLANGUAGE_ARABIC_UAE)
		.value("wxLANGUAGE_ARABIC_YEMEN", wxLANGUAGE_ARABIC_YEMEN)
		.value("wxLANGUAGE_ARMENIAN", wxLANGUAGE_ARMENIAN)
		.value("wxLANGUAGE_ASSAMESE", wxLANGUAGE_ASSAMESE)
		.value("wxLANGUAGE_ASTURIAN", wxLANGUAGE_ASTURIAN)
		.value("wxLANGUAGE_AYMARA", wxLANGUAGE_AYMARA)
		.value("wxLANGUAGE_AZERI", wxLANGUAGE_AZERI)
		.value("wxLANGUAGE_AZERI_CYRILLIC", wxLANGUAGE_AZERI_CYRILLIC)
		.value("wxLANGUAGE_AZERI_LATIN", wxLANGUAGE_AZERI_LATIN)
		.value("wxLANGUAGE_BASHKIR", wxLANGUAGE_BASHKIR)
		.value("wxLANGUAGE_BASQUE", wxLANGUAGE_BASQUE)
		.value("wxLANGUAGE_BELARUSIAN", wxLANGUAGE_BELARUSIAN)
		.value("wxLANGUAGE_BENGALI", wxLANGUAGE_BENGALI)
		.value("wxLANGUAGE_BHUTANI", wxLANGUAGE_BHUTANI)
		.value("wxLANGUAGE_BIHARI", wxLANGUAGE_BIHARI)
		.value("wxLANGUAGE_BISLAMA", wxLANGUAGE_BISLAMA)
		.value("wxLANGUAGE_BOSNIAN", wxLANGUAGE_BOSNIAN)
		.value("wxLANGUAGE_BRETON", wxLANGUAGE_BRETON)
		.value("wxLANGUAGE_BULGARIAN", wxLANGUAGE_BULGARIAN)
		.value("wxLANGUAGE_BURMESE", wxLANGUAGE_BURMESE)
		.value("wxLANGUAGE_CAMBODIAN", wxLANGUAGE_CAMBODIAN)
		.value("wxLANGUAGE_CATALAN", wxLANGUAGE_CATALAN)
		.value("wxLANGUAGE_CHINESE", wxLANGUAGE_CHINESE)
		.value("wxLANGUAGE_CHINESE_SIMPLIFIED", wxLANGUAGE_CHINESE_SIMPLIFIED)
		.value("wxLANGUAGE_CHINESE_TRADITIONAL", wxLANGUAGE_CHINESE_TRADITIONAL)
		.value("wxLANGUAGE_CHINESE_HONGKONG", wxLANGUAGE_CHINESE_HONGKONG)
		.value("wxLANGUAGE_CHINESE_MACAU", wxLANGUAGE_CHINESE_MACAU)
		.value("wxLANGUAGE_CHINESE_SINGAPORE", wxLANGUAGE_CHINESE_SINGAPORE)
		.value("wxLANGUAGE_CHINESE_TAIWAN", wxLANGUAGE_CHINESE_TAIWAN)
		.value("wxLANGUAGE_CORSICAN", wxLANGUAGE_CORSICAN)
		.value("wxLANGUAGE_CROATIAN", wxLANGUAGE_CROATIAN)
		.value("wxLANGUAGE_CZECH", wxLANGUAGE_CZECH)
		.value("wxLANGUAGE_DANISH", wxLANGUAGE_DANISH)
		.value("wxLANGUAGE_DUTCH", wxLANGUAGE_DUTCH)
		.value("wxLANGUAGE_DUTCH_BELGIAN", wxLANGUAGE_DUTCH_BELGIAN)
		.value("wxLANGUAGE_ENGLISH", wxLANGUAGE_ENGLISH)
		.value("wxLANGUAGE_ENGLISH_UK", wxLANGUAGE_ENGLISH_UK)
		.value("wxLANGUAGE_ENGLISH_US", wxLANGUAGE_ENGLISH_US)
		.value("wxLANGUAGE_ENGLISH_AUSTRALIA", wxLANGUAGE_ENGLISH_AUSTRALIA)
		.value("wxLANGUAGE_ENGLISH_BELIZE", wxLANGUAGE_ENGLISH_BELIZE)
		.value("wxLANGUAGE_ENGLISH_BOTSWANA", wxLANGUAGE_ENGLISH_BOTSWANA)
		.value("wxLANGUAGE_ENGLISH_CANADA", wxLANGUAGE_ENGLISH_CANADA)
		.value("wxLANGUAGE_ENGLISH_CARIBBEAN", wxLANGUAGE_ENGLISH_CARIBBEAN)
		.value("wxLANGUAGE_ENGLISH_DENMARK", wxLANGUAGE_ENGLISH_DENMARK)
		.value("wxLANGUAGE_ENGLISH_EIRE", wxLANGUAGE_ENGLISH_EIRE)
		.value("wxLANGUAGE_ENGLISH_JAMAICA", wxLANGUAGE_ENGLISH_JAMAICA)
		.value("wxLANGUAGE_ENGLISH_NEW_ZEALAND", wxLANGUAGE_ENGLISH_NEW_ZEALAND)
		.value("wxLANGUAGE_ENGLISH_PHILIPPINES", wxLANGUAGE_ENGLISH_PHILIPPINES)
		.value("wxLANGUAGE_ENGLISH_SOUTH_AFRICA", wxLANGUAGE_ENGLISH_SOUTH_AFRICA)
		.value("wxLANGUAGE_ENGLISH_TRINIDAD", wxLANGUAGE_ENGLISH_TRINIDAD)
		.value("wxLANGUAGE_ENGLISH_ZIMBABWE", wxLANGUAGE_ENGLISH_ZIMBABWE)
		.value("wxLANGUAGE_ESPERANTO", wxLANGUAGE_ESPERANTO)
		.value("wxLANGUAGE_ESTONIAN", wxLANGUAGE_ESTONIAN)
		.value("wxLANGUAGE_FAEROESE", wxLANGUAGE_FAEROESE)
		.value("wxLANGUAGE_FARSI", wxLANGUAGE_FARSI)
		.value("wxLANGUAGE_FIJI", wxLANGUAGE_FIJI)
		.value("wxLANGUAGE_FINNISH", wxLANGUAGE_FINNISH)
		.value("wxLANGUAGE_FRENCH", wxLANGUAGE_FRENCH)
		.value("wxLANGUAGE_FRENCH_BELGIAN", wxLANGUAGE_FRENCH_BELGIAN)
		.value("wxLANGUAGE_FRENCH_CANADIAN", wxLANGUAGE_FRENCH_CANADIAN)
		.value("wxLANGUAGE_FRENCH_LUXEMBOURG", wxLANGUAGE_FRENCH_LUXEMBOURG)
		.value("wxLANGUAGE_FRENCH_MONACO", wxLANGUAGE_FRENCH_MONACO)
		.value("wxLANGUAGE_FRENCH_SWISS", wxLANGUAGE_FRENCH_SWISS)
		.value("wxLANGUAGE_FRISIAN", wxLANGUAGE_FRISIAN)
		.value("wxLANGUAGE_GALICIAN", wxLANGUAGE_GALICIAN)
		.value("wxLANGUAGE_GEORGIAN", wxLANGUAGE_GEORGIAN)
		.value("wxLANGUAGE_GERMAN", wxLANGUAGE_GERMAN)
		.value("wxLANGUAGE_GERMAN_AUSTRIAN", wxLANGUAGE_GERMAN_AUSTRIAN)
		.value("wxLANGUAGE_GERMAN_BELGIUM", wxLANGUAGE_GERMAN_BELGIUM)
		.value("wxLANGUAGE_GERMAN_LIECHTENSTEIN", wxLANGUAGE_GERMAN_LIECHTENSTEIN)
		.value("wxLANGUAGE_GERMAN_LUXEMBOURG", wxLANGUAGE_GERMAN_LUXEMBOURG)
		.value("wxLANGUAGE_GERMAN_SWISS", wxLANGUAGE_GERMAN_SWISS)
		.value("wxLANGUAGE_GREEK", wxLANGUAGE_GREEK)
		.value("wxLANGUAGE_GREENLANDIC", wxLANGUAGE_GREENLANDIC)
		.value("wxLANGUAGE_GUARANI", wxLANGUAGE_GUARANI)
		.value("wxLANGUAGE_GUJARATI", wxLANGUAGE_GUJARATI)
		.value("wxLANGUAGE_HAUSA", wxLANGUAGE_HAUSA)
		.value("wxLANGUAGE_HEBREW", wxLANGUAGE_HEBREW)
		.value("wxLANGUAGE_HINDI", wxLANGUAGE_HINDI)
		.value("wxLANGUAGE_HUNGARIAN", wxLANGUAGE_HUNGARIAN)
		.value("wxLANGUAGE_ICELANDIC", wxLANGUAGE_ICELANDIC)
		.value("wxLANGUAGE_INDONESIAN", wxLANGUAGE_INDONESIAN)
		.value("wxLANGUAGE_INTERLINGUA", wxLANGUAGE_INTERLINGUA)
		.value("wxLANGUAGE_INTERLINGUE", wxLANGUAGE_INTERLINGUE)
		.value("wxLANGUAGE_INUKTITUT", wxLANGUAGE_INUKTITUT)
		.value("wxLANGUAGE_INUPIAK", wxLANGUAGE_INUPIAK)
		.value("wxLANGUAGE_IRISH", wxLANGUAGE_IRISH)
		.value("wxLANGUAGE_ITALIAN", wxLANGUAGE_ITALIAN)
		.value("wxLANGUAGE_ITALIAN_SWISS", wxLANGUAGE_ITALIAN_SWISS)
		.value("wxLANGUAGE_JAPANESE", wxLANGUAGE_JAPANESE)
		.value("wxLANGUAGE_JAVANESE", wxLANGUAGE_JAVANESE)
		.value("wxLANGUAGE_KANNADA", wxLANGUAGE_KANNADA)
		.value("wxLANGUAGE_KASHMIRI", wxLANGUAGE_KASHMIRI)
		.value("wxLANGUAGE_KASHMIRI_INDIA", wxLANGUAGE_KASHMIRI_INDIA)
		.value("wxLANGUAGE_KAZAKH", wxLANGUAGE_KAZAKH)
		.value("wxLANGUAGE_KERNEWEK", wxLANGUAGE_KERNEWEK)
		.value("wxLANGUAGE_KINYARWANDA", wxLANGUAGE_KINYARWANDA)
		.value("wxLANGUAGE_KIRGHIZ", wxLANGUAGE_KIRGHIZ)
		.value("wxLANGUAGE_KIRUNDI", wxLANGUAGE_KIRUNDI)
		.value("wxLANGUAGE_KONKANI", wxLANGUAGE_KONKANI)
		.value("wxLANGUAGE_KOREAN", wxLANGUAGE_KOREAN)
		.value("wxLANGUAGE_KURDISH", wxLANGUAGE_KURDISH)
		.value("wxLANGUAGE_LAOTHIAN", wxLANGUAGE_LAOTHIAN)
		.value("wxLANGUAGE_LATIN", wxLANGUAGE_LATIN)
		.value("wxLANGUAGE_LATVIAN", wxLANGUAGE_LATVIAN)
		.value("wxLANGUAGE_LINGALA", wxLANGUAGE_LINGALA)
		.value("wxLANGUAGE_LITHUANIAN", wxLANGUAGE_LITHUANIAN)
		.value("wxLANGUAGE_MACEDONIAN", wxLANGUAGE_MACEDONIAN)
		.value("wxLANGUAGE_MALAGASY", wxLANGUAGE_MALAGASY)
		.value("wxLANGUAGE_MALAY", wxLANGUAGE_MALAY)
		.value("wxLANGUAGE_MALAYALAM", wxLANGUAGE_MALAYALAM)
		.value("wxLANGUAGE_MALAY_BRUNEI_DARUSSALAM", wxLANGUAGE_MALAY_BRUNEI_DARUSSALAM)
		.value("wxLANGUAGE_MALAY_MALAYSIA", wxLANGUAGE_MALAY_MALAYSIA)
		.value("wxLANGUAGE_MALTESE", wxLANGUAGE_MALTESE)
		.value("wxLANGUAGE_MANIPURI", wxLANGUAGE_MANIPURI)
		.value("wxLANGUAGE_MAORI", wxLANGUAGE_MAORI)
		.value("wxLANGUAGE_MARATHI", wxLANGUAGE_MARATHI)
		.value("wxLANGUAGE_MOLDAVIAN", wxLANGUAGE_MOLDAVIAN)
		.value("wxLANGUAGE_MONGOLIAN", wxLANGUAGE_MONGOLIAN)
		.value("wxLANGUAGE_NAURU", wxLANGUAGE_NAURU)
		.value("wxLANGUAGE_NEPALI", wxLANGUAGE_NEPALI)
		.value("wxLANGUAGE_NEPALI_INDIA", wxLANGUAGE_NEPALI_INDIA)
		.value("wxLANGUAGE_NORWEGIAN_BOKMAL", wxLANGUAGE_NORWEGIAN_BOKMAL)
		.value("wxLANGUAGE_NORWEGIAN_NYNORSK", wxLANGUAGE_NORWEGIAN_NYNORSK)
		.value("wxLANGUAGE_OCCITAN", wxLANGUAGE_OCCITAN)
		.value("wxLANGUAGE_ORIYA", wxLANGUAGE_ORIYA)
		.value("wxLANGUAGE_OROMO", wxLANGUAGE_OROMO)
		.value("wxLANGUAGE_PASHTO", wxLANGUAGE_PASHTO)
		.value("wxLANGUAGE_POLISH", wxLANGUAGE_POLISH)
		.value("wxLANGUAGE_PORTUGUESE", wxLANGUAGE_PORTUGUESE)
		.value("wxLANGUAGE_PORTUGUESE_BRAZILIAN", wxLANGUAGE_PORTUGUESE_BRAZILIAN)
		.value("wxLANGUAGE_PUNJABI", wxLANGUAGE_PUNJABI)
		.value("wxLANGUAGE_QUECHUA", wxLANGUAGE_QUECHUA)
		.value("wxLANGUAGE_RHAETO_ROMANCE", wxLANGUAGE_RHAETO_ROMANCE)
		.value("wxLANGUAGE_ROMANIAN", wxLANGUAGE_ROMANIAN)
		.value("wxLANGUAGE_RUSSIAN", wxLANGUAGE_RUSSIAN)
		.value("wxLANGUAGE_RUSSIAN_UKRAINE", wxLANGUAGE_RUSSIAN_UKRAINE)
		.value("wxLANGUAGE_SAMI", wxLANGUAGE_SAMI)
		.value("wxLANGUAGE_SAMOAN", wxLANGUAGE_SAMOAN)
		.value("wxLANGUAGE_SANGHO", wxLANGUAGE_SANGHO)
		.value("wxLANGUAGE_SANSKRIT", wxLANGUAGE_SANSKRIT)
		.value("wxLANGUAGE_SCOTS_GAELIC", wxLANGUAGE_SCOTS_GAELIC)
		.value("wxLANGUAGE_SERBIAN", wxLANGUAGE_SERBIAN)
		.value("wxLANGUAGE_SERBIAN_CYRILLIC", wxLANGUAGE_SERBIAN_CYRILLIC)
		.value("wxLANGUAGE_SERBIAN_LATIN", wxLANGUAGE_SERBIAN_LATIN)
		.value("wxLANGUAGE_SERBO_CROATIAN", wxLANGUAGE_SERBO_CROATIAN)
		.value("wxLANGUAGE_SESOTHO", wxLANGUAGE_SESOTHO)
		.value("wxLANGUAGE_SETSWANA", wxLANGUAGE_SETSWANA)
		.value("wxLANGUAGE_SHONA", wxLANGUAGE_SHONA)
		.value("wxLANGUAGE_SINDHI", wxLANGUAGE_SINDHI)
		.value("wxLANGUAGE_SINHALESE", wxLANGUAGE_SINHALESE)
		.value("wxLANGUAGE_SISWATI", wxLANGUAGE_SISWATI)
		.value("wxLANGUAGE_SLOVAK", wxLANGUAGE_SLOVAK)
		.value("wxLANGUAGE_SLOVENIAN", wxLANGUAGE_SLOVENIAN)
		.value("wxLANGUAGE_SOMALI", wxLANGUAGE_SOMALI)
		.value("wxLANGUAGE_SPANISH", wxLANGUAGE_SPANISH)
		.value("wxLANGUAGE_SPANISH_ARGENTINA", wxLANGUAGE_SPANISH_ARGENTINA)
		.value("wxLANGUAGE_SPANISH_BOLIVIA", wxLANGUAGE_SPANISH_BOLIVIA)
		.value("wxLANGUAGE_SPANISH_CHILE", wxLANGUAGE_SPANISH_CHILE)
		.value("wxLANGUAGE_SPANISH_COLOMBIA", wxLANGUAGE_SPANISH_COLOMBIA)
		.value("wxLANGUAGE_SPANISH_COSTA_RICA", wxLANGUAGE_SPANISH_COSTA_RICA)
		.value("wxLANGUAGE_SPANISH_DOMINICAN_REPUBLIC", wxLANGUAGE_SPANISH_DOMINICAN_REPUBLIC)
		.value("wxLANGUAGE_SPANISH_ECUADOR", wxLANGUAGE_SPANISH_ECUADOR)
		.value("wxLANGUAGE_SPANISH_EL_SALVADOR", wxLANGUAGE_SPANISH_EL_SALVADOR)
		.value("wxLANGUAGE_SPANISH_GUATEMALA", wxLANGUAGE_SPANISH_GUATEMALA)
		.value("wxLANGUAGE_SPANISH_HONDURAS", wxLANGUAGE_SPANISH_HONDURAS)
		.value("wxLANGUAGE_SPANISH_MEXICAN", wxLANGUAGE_SPANISH_MEXICAN)
		.value("wxLANGUAGE_SPANISH_MODERN", wxLANGUAGE_SPANISH_MODERN)
		.value("wxLANGUAGE_SPANISH_NICARAGUA", wxLANGUAGE_SPANISH_NICARAGUA)
		.value("wxLANGUAGE_SPANISH_PANAMA", wxLANGUAGE_SPANISH_PANAMA)
		.value("wxLANGUAGE_SPANISH_PARAGUAY", wxLANGUAGE_SPANISH_PARAGUAY)
		.value("wxLANGUAGE_SPANISH_PERU", wxLANGUAGE_SPANISH_PERU)
		.value("wxLANGUAGE_SPANISH_PUERTO_RICO", wxLANGUAGE_SPANISH_PUERTO_RICO)
		.value("wxLANGUAGE_SPANISH_URUGUAY", wxLANGUAGE_SPANISH_URUGUAY)
		.value("wxLANGUAGE_SPANISH_US", wxLANGUAGE_SPANISH_US)
		.value("wxLANGUAGE_SPANISH_VENEZUELA", wxLANGUAGE_SPANISH_VENEZUELA)
		.value("wxLANGUAGE_SUNDANESE", wxLANGUAGE_SUNDANESE)
		.value("wxLANGUAGE_SWAHILI", wxLANGUAGE_SWAHILI)
		.value("wxLANGUAGE_SWEDISH", wxLANGUAGE_SWEDISH)
		.value("wxLANGUAGE_SWEDISH_FINLAND", wxLANGUAGE_SWEDISH_FINLAND)
		.value("wxLANGUAGE_TAGALOG", wxLANGUAGE_TAGALOG)
		.value("wxLANGUAGE_TAJIK", wxLANGUAGE_TAJIK)
		.value("wxLANGUAGE_TAMIL", wxLANGUAGE_TAMIL)
		.value("wxLANGUAGE_TATAR", wxLANGUAGE_TATAR)
		.value("wxLANGUAGE_TELUGU", wxLANGUAGE_TELUGU)
		.value("wxLANGUAGE_THAI", wxLANGUAGE_THAI)
		.value("wxLANGUAGE_TIBETAN", wxLANGUAGE_TIBETAN)
		.value("wxLANGUAGE_TIGRINYA", wxLANGUAGE_TIGRINYA)
		.value("wxLANGUAGE_TONGA", wxLANGUAGE_TONGA)
		.value("wxLANGUAGE_TSONGA", wxLANGUAGE_TSONGA)
		.value("wxLANGUAGE_TURKISH", wxLANGUAGE_TURKISH)
		.value("wxLANGUAGE_TURKMEN", wxLANGUAGE_TURKMEN)
		.value("wxLANGUAGE_TWI", wxLANGUAGE_TWI)
		.value("wxLANGUAGE_UIGHUR", wxLANGUAGE_UIGHUR)
		.value("wxLANGUAGE_UKRAINIAN", wxLANGUAGE_UKRAINIAN)
		.value("wxLANGUAGE_URDU", wxLANGUAGE_URDU)
		.value("wxLANGUAGE_URDU_INDIA", wxLANGUAGE_URDU_INDIA)
		.value("wxLANGUAGE_URDU_PAKISTAN", wxLANGUAGE_URDU_PAKISTAN)
		.value("wxLANGUAGE_UZBEK", wxLANGUAGE_UZBEK)
		.value("wxLANGUAGE_UZBEK_CYRILLIC", wxLANGUAGE_UZBEK_CYRILLIC)
		.value("wxLANGUAGE_UZBEK_LATIN", wxLANGUAGE_UZBEK_LATIN)
		.value("wxLANGUAGE_VALENCIAN", wxLANGUAGE_VALENCIAN)
		.value("wxLANGUAGE_VIETNAMESE", wxLANGUAGE_VIETNAMESE)
		.value("wxLANGUAGE_VOLAPUK", wxLANGUAGE_VOLAPUK)
		.value("wxLANGUAGE_WELSH", wxLANGUAGE_WELSH)
		.value("wxLANGUAGE_WOLOF", wxLANGUAGE_WOLOF)
		.value("wxLANGUAGE_XHOSA", wxLANGUAGE_XHOSA)
		.value("wxLANGUAGE_YIDDISH", wxLANGUAGE_YIDDISH)
		.value("wxLANGUAGE_YORUBA", wxLANGUAGE_YORUBA)
		.value("wxLANGUAGE_ZHUANG", wxLANGUAGE_ZHUANG)
		.value("wxLANGUAGE_ZULU", wxLANGUAGE_ZULU)
		.value("wxLANGUAGE_KABYLE", wxLANGUAGE_KABYLE)
		.value("wxLANGUAGE_USER_DEFINED", wxLANGUAGE_USER_DEFINED)
		.export_values();

;

}
