#include <cstdio> // _IO_FILE
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

void bind_unknown_unknown_36(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxRmdir(const class wxString &, int) file: line:679
	M("").def("wxRmdir", [](const class wxString & a0) -> bool { return wxRmdir(a0); }, "", pybind11::arg("dir"));
	M("").def("wxRmdir", (bool (*)(const class wxString &, int)) &wxRmdir, "C++: wxRmdir(const class wxString &, int) --> bool", pybind11::arg("dir"), pybind11::arg("flags"));

	// wxGetFileKind(int) file: line:682
	M("").def("wxGetFileKind", (enum wxFileKind (*)(int)) &wxGetFileKind, "C++: wxGetFileKind(int) --> enum wxFileKind", pybind11::arg("fd"));

	// wxGetFileKind(struct _IO_FILE *) file: line:683
	M("").def("wxGetFileKind", (enum wxFileKind (*)(struct _IO_FILE *)) &wxGetFileKind, "C++: wxGetFileKind(struct _IO_FILE *) --> enum wxFileKind", pybind11::arg("fp"));

	// wxIsWritable(const class wxString &) file: line:695
	M("").def("wxIsWritable", (bool (*)(const class wxString &)) &wxIsWritable, "C++: wxIsWritable(const class wxString &) --> bool", pybind11::arg("path"));

	// wxIsReadable(const class wxString &) file: line:696
	M("").def("wxIsReadable", (bool (*)(const class wxString &)) &wxIsReadable, "C++: wxIsReadable(const class wxString &) --> bool", pybind11::arg("path"));

	// wxIsExecutable(const class wxString &) file: line:697
	M("").def("wxIsExecutable", (bool (*)(const class wxString &)) &wxIsExecutable, "C++: wxIsExecutable(const class wxString &) --> bool", pybind11::arg("path"));

	// wxIsPathSeparator(wchar_t) file: line:745
	M("").def("wxIsPathSeparator", (bool (*)(wchar_t)) &wxIsPathSeparator, "C++: wxIsPathSeparator(wchar_t) --> bool", pybind11::arg("c"));

	// wxEndsWithPathSeparator(const class wxString &) file: line:756
	M("").def("wxEndsWithPathSeparator", (bool (*)(const class wxString &)) &wxEndsWithPathSeparator, "C++: wxEndsWithPathSeparator(const class wxString &) --> bool", pybind11::arg("filename"));

	// wxSplitPath(const class wxString &, class wxString *, class wxString *, class wxString *) file: line:762
	M("").def("wxSplitPath", (void (*)(const class wxString &, class wxString *, class wxString *, class wxString *)) &wxSplitPath, "C++: wxSplitPath(const class wxString &, class wxString *, class wxString *, class wxString *) --> void", pybind11::arg("fileName"), pybind11::arg("pstrPath"), pybind11::arg("pstrName"), pybind11::arg("pstrExt"));

	// wxFindFileInPath(class wxString *, const class wxString &, const class wxString &) file: line:769
	M("").def("wxFindFileInPath", (bool (*)(class wxString *, const class wxString &, const class wxString &)) &wxFindFileInPath, "C++: wxFindFileInPath(class wxString *, const class wxString &, const class wxString &) --> bool", pybind11::arg("pStr"), pybind11::arg("szPath"), pybind11::arg("szFile"));

	// wxGetOSDirectory() file: line:773
	M("").def("wxGetOSDirectory", (class wxString (*)()) &wxGetOSDirectory, "C++: wxGetOSDirectory() --> class wxString");

	// wxFileModificationTime(const class wxString &) file: line:778
	M("").def("wxFileModificationTime", (long (*)(const class wxString &)) &wxFileModificationTime, "C++: wxFileModificationTime(const class wxString &) --> long", pybind11::arg("filename"));

	// wxParseCommonDialogsFilter(const class wxString &, class wxArrayString &, class wxArrayString &) file: line:787
	M("").def("wxParseCommonDialogsFilter", (int (*)(const class wxString &, class wxArrayString &, class wxArrayString &)) &wxParseCommonDialogsFilter, "C++: wxParseCommonDialogsFilter(const class wxString &, class wxArrayString &, class wxArrayString &) --> int", pybind11::arg("wildCard"), pybind11::arg("descriptions"), pybind11::arg("filters"));

	{ // wxUmaskChanger file: line:796
		pybind11::class_<wxUmaskChanger, std::shared_ptr<wxUmaskChanger>> cl(M(""), "wxUmaskChanger", "");
		cl.def( pybind11::init<int>(), pybind11::arg("umaskNew") );

	}
	{ // wxPathList file: line:829
		pybind11::class_<wxPathList, std::shared_ptr<wxPathList>, wxArrayString> cl(M(""), "wxPathList", "");
		cl.def( pybind11::init( [](){ return new wxPathList(); } ) );
		cl.def( pybind11::init<const class wxArrayString &>(), pybind11::arg("arr") );

		cl.def( pybind11::init( [](wxPathList const &o){ return new wxPathList(o); } ) );
		cl.def("AddEnvList", (void (wxPathList::*)(const class wxString &)) &wxPathList::AddEnvList, "C++: wxPathList::AddEnvList(const class wxString &) --> void", pybind11::arg("envVariable"));
		cl.def("Add", (bool (wxPathList::*)(const class wxString &)) &wxPathList::Add, "C++: wxPathList::Add(const class wxString &) --> bool", pybind11::arg("path"));
		cl.def("Add", (void (wxPathList::*)(const class wxArrayString &)) &wxPathList::Add, "C++: wxPathList::Add(const class wxArrayString &) --> void", pybind11::arg("paths"));
		cl.def("FindValidPath", (class wxString (wxPathList::*)(const class wxString &) const) &wxPathList::FindValidPath, "C++: wxPathList::FindValidPath(const class wxString &) const --> class wxString", pybind11::arg("filename"));
		cl.def("FindAbsoluteValidPath", (class wxString (wxPathList::*)(const class wxString &) const) &wxPathList::FindAbsoluteValidPath, "C++: wxPathList::FindAbsoluteValidPath(const class wxString &) const --> class wxString", pybind11::arg("filename"));
		cl.def("EnsureFileAccessible", (bool (wxPathList::*)(const class wxString &)) &wxPathList::EnsureFileAccessible, "C++: wxPathList::EnsureFileAccessible(const class wxString &) --> bool", pybind11::arg("path"));
		cl.def("assign", (class wxPathList & (wxPathList::*)(const class wxPathList &)) &wxPathList::operator=, "C++: wxPathList::operator=(const class wxPathList &) --> class wxPathList &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxVersionInfo file: line:19
		pybind11::class_<wxVersionInfo, std::shared_ptr<wxVersionInfo>> cl(M(""), "wxVersionInfo", "");
		cl.def( pybind11::init( [](){ return new wxVersionInfo(); } ), "doc" );
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxVersionInfo(a0); } ), "doc" , pybind11::arg("name"));
		cl.def( pybind11::init( [](const class wxString & a0, int const & a1){ return new wxVersionInfo(a0, a1); } ), "doc" , pybind11::arg("name"), pybind11::arg("major"));
		cl.def( pybind11::init( [](const class wxString & a0, int const & a1, int const & a2){ return new wxVersionInfo(a0, a1, a2); } ), "doc" , pybind11::arg("name"), pybind11::arg("major"), pybind11::arg("minor"));
		cl.def( pybind11::init( [](const class wxString & a0, int const & a1, int const & a2, int const & a3){ return new wxVersionInfo(a0, a1, a2, a3); } ), "doc" , pybind11::arg("name"), pybind11::arg("major"), pybind11::arg("minor"), pybind11::arg("micro"));
		cl.def( pybind11::init( [](const class wxString & a0, int const & a1, int const & a2, int const & a3, const class wxString & a4){ return new wxVersionInfo(a0, a1, a2, a3, a4); } ), "doc" , pybind11::arg("name"), pybind11::arg("major"), pybind11::arg("minor"), pybind11::arg("micro"), pybind11::arg("description"));
		cl.def( pybind11::init<const class wxString &, int, int, int, const class wxString &, const class wxString &>(), pybind11::arg("name"), pybind11::arg("major"), pybind11::arg("minor"), pybind11::arg("micro"), pybind11::arg("description"), pybind11::arg("copyright") );

		cl.def( pybind11::init( [](wxVersionInfo const &o){ return new wxVersionInfo(o); } ) );
		cl.def("GetName", (const class wxString & (wxVersionInfo::*)() const) &wxVersionInfo::GetName, "C++: wxVersionInfo::GetName() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetMajor", (int (wxVersionInfo::*)() const) &wxVersionInfo::GetMajor, "C++: wxVersionInfo::GetMajor() const --> int");
		cl.def("GetMinor", (int (wxVersionInfo::*)() const) &wxVersionInfo::GetMinor, "C++: wxVersionInfo::GetMinor() const --> int");
		cl.def("GetMicro", (int (wxVersionInfo::*)() const) &wxVersionInfo::GetMicro, "C++: wxVersionInfo::GetMicro() const --> int");
		cl.def("ToString", (class wxString (wxVersionInfo::*)() const) &wxVersionInfo::ToString, "C++: wxVersionInfo::ToString() const --> class wxString");
		cl.def("GetVersionString", (class wxString (wxVersionInfo::*)() const) &wxVersionInfo::GetVersionString, "C++: wxVersionInfo::GetVersionString() const --> class wxString");
		cl.def("HasDescription", (bool (wxVersionInfo::*)() const) &wxVersionInfo::HasDescription, "C++: wxVersionInfo::HasDescription() const --> bool");
		cl.def("GetDescription", (const class wxString & (wxVersionInfo::*)() const) &wxVersionInfo::GetDescription, "C++: wxVersionInfo::GetDescription() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("HasCopyright", (bool (wxVersionInfo::*)() const) &wxVersionInfo::HasCopyright, "C++: wxVersionInfo::HasCopyright() const --> bool");
		cl.def("GetCopyright", (const class wxString & (wxVersionInfo::*)() const) &wxVersionInfo::GetCopyright, "C++: wxVersionInfo::GetCopyright() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxVersionInfo & (wxVersionInfo::*)(const class wxVersionInfo &)) &wxVersionInfo::operator=, "C++: wxVersionInfo::operator=(const class wxVersionInfo &) --> class wxVersionInfo &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
