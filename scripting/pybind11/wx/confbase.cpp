#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/confbase.h> // 
#include <wx/confbase.h> // wxConfigBase
#include <wx/confbase.h> // wxConfigPathChanger
#include <wx/confbase.h> // wxExpandEnvVars
#include <wx/confbase.h> // wxSplitPath

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxConvAuto file: line:32
struct PyCallBack_wxConvAuto : public wxConvAuto {
	using wxConvAuto::wxConvAuto;

	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvAuto *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxConvAuto::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvAuto *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxConvAuto::FromWChar(a0, a1, a2, a3);
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvAuto *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxConvAuto::GetMBNulLen();
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvAuto *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		return wxConvAuto::Clone();
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvAuto *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConvAuto *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::WC2MB(a0, a1, a2);
	}
};

void bind_wx_confbase(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxConfigPathChanger file:wx/confbase.h line:414
		pybind11::class_<wxConfigPathChanger, std::shared_ptr<wxConfigPathChanger>> cl(M(""), "wxConfigPathChanger", "");
		cl.def( pybind11::init<const class wxConfigBase *, const class wxString &>(), pybind11::arg("pContainer"), pybind11::arg("strEntry") );

		cl.def("Name", (const class wxString & (wxConfigPathChanger::*)() const) &wxConfigPathChanger::Name, "C++: wxConfigPathChanger::Name() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("UpdateIfDeleted", (void (wxConfigPathChanger::*)()) &wxConfigPathChanger::UpdateIfDeleted, "C++: wxConfigPathChanger::UpdateIfDeleted() --> void");
	}
	// wxExpandEnvVars(const class wxString &) file:wx/confbase.h line:450
	M("").def("wxExpandEnvVars", (class wxString (*)(const class wxString &)) &wxExpandEnvVars, "C++: wxExpandEnvVars(const class wxString &) --> class wxString", pybind11::arg("sz"));

	// wxSplitPath(class wxArrayString &, const class wxString &) file:wx/confbase.h line:455
	M("").def("wxSplitPath", (void (*)(class wxArrayString &, const class wxString &)) &wxSplitPath, "C++: wxSplitPath(class wxArrayString &, const class wxString &) --> void", pybind11::arg("aParts"), pybind11::arg("path"));

	// wxBOM file: line:21
	pybind11::enum_<wxBOM>(M(""), "wxBOM", pybind11::arithmetic(), "")
		.value("wxBOM_Unknown", wxBOM_Unknown)
		.value("wxBOM_None", wxBOM_None)
		.value("wxBOM_UTF32BE", wxBOM_UTF32BE)
		.value("wxBOM_UTF32LE", wxBOM_UTF32LE)
		.value("wxBOM_UTF16BE", wxBOM_UTF16BE)
		.value("wxBOM_UTF16LE", wxBOM_UTF16LE)
		.value("wxBOM_UTF8", wxBOM_UTF8)
		.export_values();

;

	{ // wxConvAuto file: line:32
		pybind11::class_<wxConvAuto, std::shared_ptr<wxConvAuto>, PyCallBack_wxConvAuto, wxMBConv> cl(M(""), "wxConvAuto", "");
		cl.def( pybind11::init( [](){ return new wxConvAuto(); }, [](){ return new PyCallBack_wxConvAuto(); } ), "doc");
		cl.def( pybind11::init<enum wxFontEncoding>(), pybind11::arg("enc") );

		cl.def( pybind11::init( [](PyCallBack_wxConvAuto const &o){ return new PyCallBack_wxConvAuto(o); } ) );
		cl.def( pybind11::init( [](wxConvAuto const &o){ return new wxConvAuto(o); } ) );
		cl.def_static("GetFallbackEncoding", (enum wxFontEncoding (*)()) &wxConvAuto::GetFallbackEncoding, "C++: wxConvAuto::GetFallbackEncoding() --> enum wxFontEncoding");
		cl.def_static("SetFallbackEncoding", (void (*)(enum wxFontEncoding)) &wxConvAuto::SetFallbackEncoding, "C++: wxConvAuto::SetFallbackEncoding(enum wxFontEncoding) --> void", pybind11::arg("enc"));
		cl.def_static("DisableFallbackEncoding", (void (*)()) &wxConvAuto::DisableFallbackEncoding, "C++: wxConvAuto::DisableFallbackEncoding() --> void");
		cl.def("ToWChar", [](wxConvAuto const &o, wchar_t * a0, unsigned long const & a1, const char * a2) -> unsigned long { return o.ToWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("ToWChar", (unsigned long (wxConvAuto::*)(wchar_t *, unsigned long, const char *, unsigned long) const) &wxConvAuto::ToWChar, "C++: wxConvAuto::ToWChar(wchar_t *, unsigned long, const char *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("FromWChar", [](wxConvAuto const &o, char * a0, unsigned long const & a1, const wchar_t * a2) -> unsigned long { return o.FromWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("FromWChar", (unsigned long (wxConvAuto::*)(char *, unsigned long, const wchar_t *, unsigned long) const) &wxConvAuto::FromWChar, "C++: wxConvAuto::FromWChar(char *, unsigned long, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("GetMBNulLen", (unsigned long (wxConvAuto::*)() const) &wxConvAuto::GetMBNulLen, "C++: wxConvAuto::GetMBNulLen() const --> unsigned long");
		cl.def("Clone", (class wxMBConv * (wxConvAuto::*)() const) &wxConvAuto::Clone, "C++: wxConvAuto::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def_static("DetectBOM", (enum wxBOM (*)(const char *, unsigned long)) &wxConvAuto::DetectBOM, "C++: wxConvAuto::DetectBOM(const char *, unsigned long) --> enum wxBOM", pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def_static("GetBOMChars", (const char * (*)(enum wxBOM, unsigned long *)) &wxConvAuto::GetBOMChars, "C++: wxConvAuto::GetBOMChars(enum wxBOM, unsigned long *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("bomType"), pybind11::arg("count"));
		cl.def("GetBOM", (enum wxBOM (wxConvAuto::*)() const) &wxConvAuto::GetBOM, "C++: wxConvAuto::GetBOM() const --> enum wxBOM");
	}
	// wxTextFileType file: line:24
	pybind11::enum_<wxTextFileType>(M(""), "wxTextFileType", pybind11::arithmetic(), "")
		.value("wxTextFileType_None", wxTextFileType_None)
		.value("wxTextFileType_Unix", wxTextFileType_Unix)
		.value("wxTextFileType_Dos", wxTextFileType_Dos)
		.value("wxTextFileType_Mac", wxTextFileType_Mac)
		.value("wxTextFileType_Os2", wxTextFileType_Os2)
		.export_values();

;

	{ // wxAssert_wxArrayLinesType file: line:359
		pybind11::class_<wxAssert_wxArrayLinesType, std::shared_ptr<wxAssert_wxArrayLinesType>> cl(M(""), "wxAssert_wxArrayLinesType", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxArrayLinesType(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayInt", &wxAssert_wxArrayLinesType::TypeTooBigToBeStoredInwxBaseArrayInt);
	}
	{ // wxArrayLinesType file: line:44
		pybind11::class_<wxArrayLinesType, std::shared_ptr<wxArrayLinesType>, wxBaseArrayInt> cl(M(""), "wxArrayLinesType", "");
		cl.def( pybind11::init( [](){ return new wxArrayLinesType(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, const enum wxTextFileType &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const enum wxTextFileType *, const enum wxTextFileType *>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def( pybind11::init( [](wxArrayLinesType const &o){ return new wxArrayLinesType(o); } ) );
		cl.def("__getitem__", (enum wxTextFileType & (wxArrayLinesType::*)(unsigned long) const) &wxArrayLinesType::operator[], "C++: wxArrayLinesType::operator[](unsigned long) const --> enum wxTextFileType &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (enum wxTextFileType & (wxArrayLinesType::*)(unsigned long) const) &wxArrayLinesType::Item, "C++: wxArrayLinesType::Item(unsigned long) const --> enum wxTextFileType &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (enum wxTextFileType & (wxArrayLinesType::*)() const) &wxArrayLinesType::Last, "C++: wxArrayLinesType::Last() const --> enum wxTextFileType &", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxArrayLinesType const &o, enum wxTextFileType const & a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxArrayLinesType::*)(enum wxTextFileType, bool) const) &wxArrayLinesType::Index, "C++: wxArrayLinesType::Index(enum wxTextFileType, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxArrayLinesType &o, enum wxTextFileType const & a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxArrayLinesType::*)(enum wxTextFileType, unsigned long)) &wxArrayLinesType::Add, "C++: wxArrayLinesType::Add(enum wxTextFileType, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxArrayLinesType &o, enum wxTextFileType const & a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxArrayLinesType::*)(enum wxTextFileType, unsigned long, unsigned long)) &wxArrayLinesType::Insert, "C++: wxArrayLinesType::Insert(enum wxTextFileType, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxArrayLinesType &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxArrayLinesType::*)(unsigned long, unsigned long)) &wxArrayLinesType::RemoveAt, "C++: wxArrayLinesType::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxArrayLinesType::*)(enum wxTextFileType)) &wxArrayLinesType::Remove, "C++: wxArrayLinesType::Remove(enum wxTextFileType) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxArrayLinesType::*)(const enum wxTextFileType *, const enum wxTextFileType *)) &wxArrayLinesType::assign, "C++: wxArrayLinesType::assign(const enum wxTextFileType *, const enum wxTextFileType *) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", (void (wxArrayLinesType::*)(unsigned long, const enum wxTextFileType &)) &wxArrayLinesType::assign, "C++: wxArrayLinesType::assign(unsigned long, const enum wxTextFileType &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (enum wxTextFileType & (wxArrayLinesType::*)()) &wxArrayLinesType::back, "C++: wxArrayLinesType::back() --> enum wxTextFileType &", pybind11::return_value_policy::automatic);
		cl.def("begin", (enum wxTextFileType * (wxArrayLinesType::*)()) &wxArrayLinesType::begin, "C++: wxArrayLinesType::begin() --> enum wxTextFileType *", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxArrayLinesType::*)() const) &wxArrayLinesType::capacity, "C++: wxArrayLinesType::capacity() const --> unsigned long");
		cl.def("end", (enum wxTextFileType * (wxArrayLinesType::*)()) &wxArrayLinesType::end, "C++: wxArrayLinesType::end() --> enum wxTextFileType *", pybind11::return_value_policy::automatic);
		cl.def("erase", (enum wxTextFileType * (wxArrayLinesType::*)(enum wxTextFileType *, enum wxTextFileType *)) &wxArrayLinesType::erase, "C++: wxArrayLinesType::erase(enum wxTextFileType *, enum wxTextFileType *) --> enum wxTextFileType *", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (enum wxTextFileType * (wxArrayLinesType::*)(enum wxTextFileType *)) &wxArrayLinesType::erase, "C++: wxArrayLinesType::erase(enum wxTextFileType *) --> enum wxTextFileType *", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("front", (enum wxTextFileType & (wxArrayLinesType::*)()) &wxArrayLinesType::front, "C++: wxArrayLinesType::front() --> enum wxTextFileType &", pybind11::return_value_policy::automatic);
		cl.def("insert", (void (wxArrayLinesType::*)(enum wxTextFileType *, unsigned long, const enum wxTextFileType &)) &wxArrayLinesType::insert, "C++: wxArrayLinesType::insert(enum wxTextFileType *, unsigned long, const enum wxTextFileType &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", [](wxArrayLinesType &o, enum wxTextFileType * a0) -> wxTextFileType * { return o.insert(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("insert", (enum wxTextFileType * (wxArrayLinesType::*)(enum wxTextFileType *, const enum wxTextFileType &)) &wxArrayLinesType::insert, "C++: wxArrayLinesType::insert(enum wxTextFileType *, const enum wxTextFileType &) --> enum wxTextFileType *", pybind11::return_value_policy::automatic, pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxArrayLinesType::*)(enum wxTextFileType *, const enum wxTextFileType *, const enum wxTextFileType *)) &wxArrayLinesType::insert, "C++: wxArrayLinesType::insert(enum wxTextFileType *, const enum wxTextFileType *, const enum wxTextFileType *) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("pop_back", (void (wxArrayLinesType::*)()) &wxArrayLinesType::pop_back, "C++: wxArrayLinesType::pop_back() --> void");
		cl.def("push_back", (void (wxArrayLinesType::*)(const enum wxTextFileType &)) &wxArrayLinesType::push_back, "C++: wxArrayLinesType::push_back(const enum wxTextFileType &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxArrayLinesType::reverse_iterator (wxArrayLinesType::*)()) &wxArrayLinesType::rbegin, "C++: wxArrayLinesType::rbegin() --> class wxArrayLinesType::reverse_iterator");
		cl.def("rend", (class wxArrayLinesType::reverse_iterator (wxArrayLinesType::*)()) &wxArrayLinesType::rend, "C++: wxArrayLinesType::rend() --> class wxArrayLinesType::reverse_iterator");
		cl.def("reserve", (void (wxArrayLinesType::*)(unsigned long)) &wxArrayLinesType::reserve, "C++: wxArrayLinesType::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxArrayLinesType &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxArrayLinesType::*)(unsigned long, enum wxTextFileType)) &wxArrayLinesType::resize, "C++: wxArrayLinesType::resize(unsigned long, enum wxTextFileType) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxArrayLinesType::*)(class wxArrayLinesType &)) &wxArrayLinesType::swap, "C++: wxArrayLinesType::swap(class wxArrayLinesType &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxArrayLinesType & (wxArrayLinesType::*)(const class wxArrayLinesType &)) &wxArrayLinesType::operator=, "C++: wxArrayLinesType::operator=(const class wxArrayLinesType &) --> class wxArrayLinesType &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxArrayLinesType::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayLinesType::reverse_iterator, std::shared_ptr<wxArrayLinesType::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayLinesType::reverse_iterator(); } ) );
			cl.def( pybind11::init<enum wxTextFileType *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayLinesType::reverse_iterator const &o){ return new wxArrayLinesType::reverse_iterator(o); } ) );
			cl.def("__mul__", (enum wxTextFileType & (wxArrayLinesType::reverse_iterator::*)() const) &wxArrayLinesType::reverse_iterator::operator*, "C++: wxArrayLinesType::reverse_iterator::operator*() const --> enum wxTextFileType &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayLinesType::reverse_iterator & (wxArrayLinesType::reverse_iterator::*)()) &wxArrayLinesType::reverse_iterator::operator++, "C++: wxArrayLinesType::reverse_iterator::operator++() --> class wxArrayLinesType::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayLinesType::reverse_iterator (wxArrayLinesType::reverse_iterator::*)(int)) &wxArrayLinesType::reverse_iterator::operator++, "C++: wxArrayLinesType::reverse_iterator::operator++(int) --> const class wxArrayLinesType::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayLinesType::reverse_iterator & (wxArrayLinesType::reverse_iterator::*)()) &wxArrayLinesType::reverse_iterator::operator--, "C++: wxArrayLinesType::reverse_iterator::operator--() --> class wxArrayLinesType::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayLinesType::reverse_iterator (wxArrayLinesType::reverse_iterator::*)(int)) &wxArrayLinesType::reverse_iterator::operator--, "C++: wxArrayLinesType::reverse_iterator::operator--(int) --> const class wxArrayLinesType::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayLinesType::reverse_iterator::*)(const class wxArrayLinesType::reverse_iterator &) const) &wxArrayLinesType::reverse_iterator::operator==, "C++: wxArrayLinesType::reverse_iterator::operator==(const class wxArrayLinesType::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayLinesType::reverse_iterator::*)(const class wxArrayLinesType::reverse_iterator &) const) &wxArrayLinesType::reverse_iterator::operator!=, "C++: wxArrayLinesType::reverse_iterator::operator!=(const class wxArrayLinesType::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxArrayLinesType::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayLinesType::const_reverse_iterator, std::shared_ptr<wxArrayLinesType::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayLinesType::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const enum wxTextFileType *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayLinesType::const_reverse_iterator const &o){ return new wxArrayLinesType::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxArrayLinesType::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (const enum wxTextFileType & (wxArrayLinesType::const_reverse_iterator::*)() const) &wxArrayLinesType::const_reverse_iterator::operator*, "C++: wxArrayLinesType::const_reverse_iterator::operator*() const --> const enum wxTextFileType &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayLinesType::const_reverse_iterator & (wxArrayLinesType::const_reverse_iterator::*)()) &wxArrayLinesType::const_reverse_iterator::operator++, "C++: wxArrayLinesType::const_reverse_iterator::operator++() --> class wxArrayLinesType::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayLinesType::const_reverse_iterator (wxArrayLinesType::const_reverse_iterator::*)(int)) &wxArrayLinesType::const_reverse_iterator::operator++, "C++: wxArrayLinesType::const_reverse_iterator::operator++(int) --> const class wxArrayLinesType::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayLinesType::const_reverse_iterator & (wxArrayLinesType::const_reverse_iterator::*)()) &wxArrayLinesType::const_reverse_iterator::operator--, "C++: wxArrayLinesType::const_reverse_iterator::operator--() --> class wxArrayLinesType::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayLinesType::const_reverse_iterator (wxArrayLinesType::const_reverse_iterator::*)(int)) &wxArrayLinesType::const_reverse_iterator::operator--, "C++: wxArrayLinesType::const_reverse_iterator::operator--(int) --> const class wxArrayLinesType::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayLinesType::const_reverse_iterator::*)(const class wxArrayLinesType::const_reverse_iterator &) const) &wxArrayLinesType::const_reverse_iterator::operator==, "C++: wxArrayLinesType::const_reverse_iterator::operator==(const class wxArrayLinesType::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayLinesType::const_reverse_iterator::*)(const class wxArrayLinesType::const_reverse_iterator &) const) &wxArrayLinesType::const_reverse_iterator::operator!=, "C++: wxArrayLinesType::const_reverse_iterator::operator!=(const class wxArrayLinesType::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}
