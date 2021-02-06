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

void bind_unknown_unknown_39(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxOperatingSystemId file: line:25
	pybind11::enum_<wxOperatingSystemId>(M(""), "wxOperatingSystemId", pybind11::arithmetic(), "")
		.value("wxOS_UNKNOWN", wxOS_UNKNOWN)
		.value("wxOS_MAC_OS", wxOS_MAC_OS)
		.value("wxOS_MAC_OSX_DARWIN", wxOS_MAC_OSX_DARWIN)
		.value("wxOS_MAC", wxOS_MAC)
		.value("wxOS_WINDOWS_9X", wxOS_WINDOWS_9X)
		.value("wxOS_WINDOWS_NT", wxOS_WINDOWS_NT)
		.value("wxOS_WINDOWS_MICRO", wxOS_WINDOWS_MICRO)
		.value("wxOS_WINDOWS_CE", wxOS_WINDOWS_CE)
		.value("wxOS_WINDOWS", wxOS_WINDOWS)
		.value("wxOS_UNIX_LINUX", wxOS_UNIX_LINUX)
		.value("wxOS_UNIX_FREEBSD", wxOS_UNIX_FREEBSD)
		.value("wxOS_UNIX_OPENBSD", wxOS_UNIX_OPENBSD)
		.value("wxOS_UNIX_NETBSD", wxOS_UNIX_NETBSD)
		.value("wxOS_UNIX_SOLARIS", wxOS_UNIX_SOLARIS)
		.value("wxOS_UNIX_AIX", wxOS_UNIX_AIX)
		.value("wxOS_UNIX_HPUX", wxOS_UNIX_HPUX)
		.value("wxOS_UNIX", wxOS_UNIX)
		.value("wxOS_DOS", wxOS_DOS)
		.value("wxOS_OS2", wxOS_OS2)
		.export_values();

;

	// wxPortId file: line:65
	pybind11::enum_<wxPortId>(M(""), "wxPortId", pybind11::arithmetic(), "")
		.value("wxPORT_UNKNOWN", wxPORT_UNKNOWN)
		.value("wxPORT_BASE", wxPORT_BASE)
		.value("wxPORT_MSW", wxPORT_MSW)
		.value("wxPORT_MOTIF", wxPORT_MOTIF)
		.value("wxPORT_GTK", wxPORT_GTK)
		.value("wxPORT_DFB", wxPORT_DFB)
		.value("wxPORT_X11", wxPORT_X11)
		.value("wxPORT_PM", wxPORT_PM)
		.value("wxPORT_OS2", wxPORT_OS2)
		.value("wxPORT_MAC", wxPORT_MAC)
		.value("wxPORT_OSX", wxPORT_OSX)
		.value("wxPORT_COCOA", wxPORT_COCOA)
		.value("wxPORT_WINCE", wxPORT_WINCE)
		.export_values();

;

	// wxArchitecture file: line:87
	pybind11::enum_<wxArchitecture>(M(""), "wxArchitecture", pybind11::arithmetic(), "")
		.value("wxARCH_INVALID", wxARCH_INVALID)
		.value("wxARCH_32", wxARCH_32)
		.value("wxARCH_64", wxARCH_64)
		.value("wxARCH_MAX", wxARCH_MAX)
		.export_values();

;

	// wxEndianness file: line:99
	pybind11::enum_<wxEndianness>(M(""), "wxEndianness", pybind11::arithmetic(), "")
		.value("wxENDIAN_INVALID", wxENDIAN_INVALID)
		.value("wxENDIAN_BIG", wxENDIAN_BIG)
		.value("wxENDIAN_LITTLE", wxENDIAN_LITTLE)
		.value("wxENDIAN_PDP", wxENDIAN_PDP)
		.value("wxENDIAN_MAX", wxENDIAN_MAX)
		.export_values();

;

	{ // wxLinuxDistributionInfo file: line:111
		pybind11::class_<wxLinuxDistributionInfo, std::shared_ptr<wxLinuxDistributionInfo>> cl(M(""), "wxLinuxDistributionInfo", "");
		cl.def( pybind11::init( [](wxLinuxDistributionInfo const &o){ return new wxLinuxDistributionInfo(o); } ) );
		cl.def( pybind11::init( [](){ return new wxLinuxDistributionInfo(); } ) );
		cl.def_readwrite("Id", &wxLinuxDistributionInfo::Id);
		cl.def_readwrite("Release", &wxLinuxDistributionInfo::Release);
		cl.def_readwrite("CodeName", &wxLinuxDistributionInfo::CodeName);
		cl.def_readwrite("Description", &wxLinuxDistributionInfo::Description);
		cl.def("__eq__", (bool (wxLinuxDistributionInfo::*)(const struct wxLinuxDistributionInfo &) const) &wxLinuxDistributionInfo::operator==, "C++: wxLinuxDistributionInfo::operator==(const struct wxLinuxDistributionInfo &) const --> bool", pybind11::arg("ldi"));
		cl.def("__ne__", (bool (wxLinuxDistributionInfo::*)(const struct wxLinuxDistributionInfo &) const) &wxLinuxDistributionInfo::operator!=, "C++: wxLinuxDistributionInfo::operator!=(const struct wxLinuxDistributionInfo &) const --> bool", pybind11::arg("ldi"));
		cl.def("assign", (struct wxLinuxDistributionInfo & (wxLinuxDistributionInfo::*)(const struct wxLinuxDistributionInfo &)) &wxLinuxDistributionInfo::operator=, "C++: wxLinuxDistributionInfo::operator=(const struct wxLinuxDistributionInfo &) --> struct wxLinuxDistributionInfo &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPlatformInfo file: line:137
		pybind11::class_<wxPlatformInfo, std::shared_ptr<wxPlatformInfo>> cl(M(""), "wxPlatformInfo", "");
		cl.def( pybind11::init( [](){ return new wxPlatformInfo(); } ) );
		cl.def( pybind11::init( [](enum wxPortId const & a0){ return new wxPlatformInfo(a0); } ), "doc" , pybind11::arg("pid"));
		cl.def( pybind11::init( [](enum wxPortId const & a0, int const & a1){ return new wxPlatformInfo(a0, a1); } ), "doc" , pybind11::arg("pid"), pybind11::arg("tkMajor"));
		cl.def( pybind11::init( [](enum wxPortId const & a0, int const & a1, int const & a2){ return new wxPlatformInfo(a0, a1, a2); } ), "doc" , pybind11::arg("pid"), pybind11::arg("tkMajor"), pybind11::arg("tkMinor"));
		cl.def( pybind11::init( [](enum wxPortId const & a0, int const & a1, int const & a2, enum wxOperatingSystemId const & a3){ return new wxPlatformInfo(a0, a1, a2, a3); } ), "doc" , pybind11::arg("pid"), pybind11::arg("tkMajor"), pybind11::arg("tkMinor"), pybind11::arg("id"));
		cl.def( pybind11::init( [](enum wxPortId const & a0, int const & a1, int const & a2, enum wxOperatingSystemId const & a3, int const & a4){ return new wxPlatformInfo(a0, a1, a2, a3, a4); } ), "doc" , pybind11::arg("pid"), pybind11::arg("tkMajor"), pybind11::arg("tkMinor"), pybind11::arg("id"), pybind11::arg("osMajor"));
		cl.def( pybind11::init( [](enum wxPortId const & a0, int const & a1, int const & a2, enum wxOperatingSystemId const & a3, int const & a4, int const & a5){ return new wxPlatformInfo(a0, a1, a2, a3, a4, a5); } ), "doc" , pybind11::arg("pid"), pybind11::arg("tkMajor"), pybind11::arg("tkMinor"), pybind11::arg("id"), pybind11::arg("osMajor"), pybind11::arg("osMinor"));
		cl.def( pybind11::init( [](enum wxPortId const & a0, int const & a1, int const & a2, enum wxOperatingSystemId const & a3, int const & a4, int const & a5, enum wxArchitecture const & a6){ return new wxPlatformInfo(a0, a1, a2, a3, a4, a5, a6); } ), "doc" , pybind11::arg("pid"), pybind11::arg("tkMajor"), pybind11::arg("tkMinor"), pybind11::arg("id"), pybind11::arg("osMajor"), pybind11::arg("osMinor"), pybind11::arg("arch"));
		cl.def( pybind11::init( [](enum wxPortId const & a0, int const & a1, int const & a2, enum wxOperatingSystemId const & a3, int const & a4, int const & a5, enum wxArchitecture const & a6, enum wxEndianness const & a7){ return new wxPlatformInfo(a0, a1, a2, a3, a4, a5, a6, a7); } ), "doc" , pybind11::arg("pid"), pybind11::arg("tkMajor"), pybind11::arg("tkMinor"), pybind11::arg("id"), pybind11::arg("osMajor"), pybind11::arg("osMinor"), pybind11::arg("arch"), pybind11::arg("endian"));
		cl.def( pybind11::init<enum wxPortId, int, int, enum wxOperatingSystemId, int, int, enum wxArchitecture, enum wxEndianness, bool>(), pybind11::arg("pid"), pybind11::arg("tkMajor"), pybind11::arg("tkMinor"), pybind11::arg("id"), pybind11::arg("osMajor"), pybind11::arg("osMinor"), pybind11::arg("arch"), pybind11::arg("endian"), pybind11::arg("usingUniversal") );

		cl.def( pybind11::init( [](wxPlatformInfo const &o){ return new wxPlatformInfo(o); } ) );
		cl.def("__eq__", (bool (wxPlatformInfo::*)(const class wxPlatformInfo &) const) &wxPlatformInfo::operator==, "C++: wxPlatformInfo::operator==(const class wxPlatformInfo &) const --> bool", pybind11::arg("t"));
		cl.def("__ne__", (bool (wxPlatformInfo::*)(const class wxPlatformInfo &) const) &wxPlatformInfo::operator!=, "C++: wxPlatformInfo::operator!=(const class wxPlatformInfo &) const --> bool", pybind11::arg("t"));
		cl.def_static("Get", (const class wxPlatformInfo & (*)()) &wxPlatformInfo::Get, "C++: wxPlatformInfo::Get() --> const class wxPlatformInfo &", pybind11::return_value_policy::automatic);
		cl.def_static("GetOperatingSystemId", (enum wxOperatingSystemId (*)(const class wxString &)) &wxPlatformInfo::GetOperatingSystemId, "C++: wxPlatformInfo::GetOperatingSystemId(const class wxString &) --> enum wxOperatingSystemId", pybind11::arg("name"));
		cl.def_static("GetPortId", (enum wxPortId (*)(const class wxString &)) &wxPlatformInfo::GetPortId, "C++: wxPlatformInfo::GetPortId(const class wxString &) --> enum wxPortId", pybind11::arg("portname"));
		cl.def_static("GetArch", (enum wxArchitecture (*)(const class wxString &)) &wxPlatformInfo::GetArch, "C++: wxPlatformInfo::GetArch(const class wxString &) --> enum wxArchitecture", pybind11::arg("arch"));
		cl.def_static("GetEndianness", (enum wxEndianness (*)(const class wxString &)) &wxPlatformInfo::GetEndianness, "C++: wxPlatformInfo::GetEndianness(const class wxString &) --> enum wxEndianness", pybind11::arg("end"));
		cl.def_static("GetOperatingSystemFamilyName", (class wxString (*)(enum wxOperatingSystemId)) &wxPlatformInfo::GetOperatingSystemFamilyName, "C++: wxPlatformInfo::GetOperatingSystemFamilyName(enum wxOperatingSystemId) --> class wxString", pybind11::arg("os"));
		cl.def_static("GetOperatingSystemIdName", (class wxString (*)(enum wxOperatingSystemId)) &wxPlatformInfo::GetOperatingSystemIdName, "C++: wxPlatformInfo::GetOperatingSystemIdName(enum wxOperatingSystemId) --> class wxString", pybind11::arg("os"));
		cl.def_static("GetPortIdName", (class wxString (*)(enum wxPortId, bool)) &wxPlatformInfo::GetPortIdName, "C++: wxPlatformInfo::GetPortIdName(enum wxPortId, bool) --> class wxString", pybind11::arg("port"), pybind11::arg("usingUniversal"));
		cl.def_static("GetPortIdShortName", (class wxString (*)(enum wxPortId, bool)) &wxPlatformInfo::GetPortIdShortName, "C++: wxPlatformInfo::GetPortIdShortName(enum wxPortId, bool) --> class wxString", pybind11::arg("port"), pybind11::arg("usingUniversal"));
		cl.def_static("GetArchName", (class wxString (*)(enum wxArchitecture)) &wxPlatformInfo::GetArchName, "C++: wxPlatformInfo::GetArchName(enum wxArchitecture) --> class wxString", pybind11::arg("arch"));
		cl.def_static("GetEndiannessName", (class wxString (*)(enum wxEndianness)) &wxPlatformInfo::GetEndiannessName, "C++: wxPlatformInfo::GetEndiannessName(enum wxEndianness) --> class wxString", pybind11::arg("end"));
		cl.def("GetOSMajorVersion", (int (wxPlatformInfo::*)() const) &wxPlatformInfo::GetOSMajorVersion, "C++: wxPlatformInfo::GetOSMajorVersion() const --> int");
		cl.def("GetOSMinorVersion", (int (wxPlatformInfo::*)() const) &wxPlatformInfo::GetOSMinorVersion, "C++: wxPlatformInfo::GetOSMinorVersion() const --> int");
		cl.def("CheckOSVersion", (bool (wxPlatformInfo::*)(int, int) const) &wxPlatformInfo::CheckOSVersion, "C++: wxPlatformInfo::CheckOSVersion(int, int) const --> bool", pybind11::arg("major"), pybind11::arg("minor"));
		cl.def("GetToolkitMajorVersion", (int (wxPlatformInfo::*)() const) &wxPlatformInfo::GetToolkitMajorVersion, "C++: wxPlatformInfo::GetToolkitMajorVersion() const --> int");
		cl.def("GetToolkitMinorVersion", (int (wxPlatformInfo::*)() const) &wxPlatformInfo::GetToolkitMinorVersion, "C++: wxPlatformInfo::GetToolkitMinorVersion() const --> int");
		cl.def("CheckToolkitVersion", (bool (wxPlatformInfo::*)(int, int) const) &wxPlatformInfo::CheckToolkitVersion, "C++: wxPlatformInfo::CheckToolkitVersion(int, int) const --> bool", pybind11::arg("major"), pybind11::arg("minor"));
		cl.def("IsUsingUniversalWidgets", (bool (wxPlatformInfo::*)() const) &wxPlatformInfo::IsUsingUniversalWidgets, "C++: wxPlatformInfo::IsUsingUniversalWidgets() const --> bool");
		cl.def("GetOperatingSystemId", (enum wxOperatingSystemId (wxPlatformInfo::*)() const) &wxPlatformInfo::GetOperatingSystemId, "C++: wxPlatformInfo::GetOperatingSystemId() const --> enum wxOperatingSystemId");
		cl.def("GetLinuxDistributionInfo", (struct wxLinuxDistributionInfo (wxPlatformInfo::*)() const) &wxPlatformInfo::GetLinuxDistributionInfo, "C++: wxPlatformInfo::GetLinuxDistributionInfo() const --> struct wxLinuxDistributionInfo");
		cl.def("GetPortId", (enum wxPortId (wxPlatformInfo::*)() const) &wxPlatformInfo::GetPortId, "C++: wxPlatformInfo::GetPortId() const --> enum wxPortId");
		cl.def("GetArchitecture", (enum wxArchitecture (wxPlatformInfo::*)() const) &wxPlatformInfo::GetArchitecture, "C++: wxPlatformInfo::GetArchitecture() const --> enum wxArchitecture");
		cl.def("GetEndianness", (enum wxEndianness (wxPlatformInfo::*)() const) &wxPlatformInfo::GetEndianness, "C++: wxPlatformInfo::GetEndianness() const --> enum wxEndianness");
		cl.def("GetOperatingSystemFamilyName", (class wxString (wxPlatformInfo::*)() const) &wxPlatformInfo::GetOperatingSystemFamilyName, "C++: wxPlatformInfo::GetOperatingSystemFamilyName() const --> class wxString");
		cl.def("GetOperatingSystemIdName", (class wxString (wxPlatformInfo::*)() const) &wxPlatformInfo::GetOperatingSystemIdName, "C++: wxPlatformInfo::GetOperatingSystemIdName() const --> class wxString");
		cl.def("GetPortIdName", (class wxString (wxPlatformInfo::*)() const) &wxPlatformInfo::GetPortIdName, "C++: wxPlatformInfo::GetPortIdName() const --> class wxString");
		cl.def("GetPortIdShortName", (class wxString (wxPlatformInfo::*)() const) &wxPlatformInfo::GetPortIdShortName, "C++: wxPlatformInfo::GetPortIdShortName() const --> class wxString");
		cl.def("GetArchName", (class wxString (wxPlatformInfo::*)() const) &wxPlatformInfo::GetArchName, "C++: wxPlatformInfo::GetArchName() const --> class wxString");
		cl.def("GetEndiannessName", (class wxString (wxPlatformInfo::*)() const) &wxPlatformInfo::GetEndiannessName, "C++: wxPlatformInfo::GetEndiannessName() const --> class wxString");
		cl.def("GetOperatingSystemDescription", (class wxString (wxPlatformInfo::*)() const) &wxPlatformInfo::GetOperatingSystemDescription, "C++: wxPlatformInfo::GetOperatingSystemDescription() const --> class wxString");
		cl.def("GetDesktopEnvironment", (class wxString (wxPlatformInfo::*)() const) &wxPlatformInfo::GetDesktopEnvironment, "C++: wxPlatformInfo::GetDesktopEnvironment() const --> class wxString");
		cl.def_static("GetOperatingSystemDirectory", (class wxString (*)()) &wxPlatformInfo::GetOperatingSystemDirectory, "C++: wxPlatformInfo::GetOperatingSystemDirectory() --> class wxString");
		cl.def("SetOSVersion", (void (wxPlatformInfo::*)(int, int)) &wxPlatformInfo::SetOSVersion, "C++: wxPlatformInfo::SetOSVersion(int, int) --> void", pybind11::arg("major"), pybind11::arg("minor"));
		cl.def("SetToolkitVersion", (void (wxPlatformInfo::*)(int, int)) &wxPlatformInfo::SetToolkitVersion, "C++: wxPlatformInfo::SetToolkitVersion(int, int) --> void", pybind11::arg("major"), pybind11::arg("minor"));
		cl.def("SetOperatingSystemId", (void (wxPlatformInfo::*)(enum wxOperatingSystemId)) &wxPlatformInfo::SetOperatingSystemId, "C++: wxPlatformInfo::SetOperatingSystemId(enum wxOperatingSystemId) --> void", pybind11::arg("n"));
		cl.def("SetOperatingSystemDescription", (void (wxPlatformInfo::*)(const class wxString &)) &wxPlatformInfo::SetOperatingSystemDescription, "C++: wxPlatformInfo::SetOperatingSystemDescription(const class wxString &) --> void", pybind11::arg("desc"));
		cl.def("SetPortId", (void (wxPlatformInfo::*)(enum wxPortId)) &wxPlatformInfo::SetPortId, "C++: wxPlatformInfo::SetPortId(enum wxPortId) --> void", pybind11::arg("n"));
		cl.def("SetArchitecture", (void (wxPlatformInfo::*)(enum wxArchitecture)) &wxPlatformInfo::SetArchitecture, "C++: wxPlatformInfo::SetArchitecture(enum wxArchitecture) --> void", pybind11::arg("n"));
		cl.def("SetEndianness", (void (wxPlatformInfo::*)(enum wxEndianness)) &wxPlatformInfo::SetEndianness, "C++: wxPlatformInfo::SetEndianness(enum wxEndianness) --> void", pybind11::arg("n"));
		cl.def("SetDesktopEnvironment", (void (wxPlatformInfo::*)(const class wxString &)) &wxPlatformInfo::SetDesktopEnvironment, "C++: wxPlatformInfo::SetDesktopEnvironment(const class wxString &) --> void", pybind11::arg("de"));
		cl.def("SetLinuxDistributionInfo", (void (wxPlatformInfo::*)(const struct wxLinuxDistributionInfo &)) &wxPlatformInfo::SetLinuxDistributionInfo, "C++: wxPlatformInfo::SetLinuxDistributionInfo(const struct wxLinuxDistributionInfo &) --> void", pybind11::arg("di"));
		cl.def("IsOk", (bool (wxPlatformInfo::*)() const) &wxPlatformInfo::IsOk, "C++: wxPlatformInfo::IsOk() const --> bool");
		cl.def("assign", (class wxPlatformInfo & (wxPlatformInfo::*)(const class wxPlatformInfo &)) &wxPlatformInfo::operator=, "C++: wxPlatformInfo::operator=(const class wxPlatformInfo &) --> class wxPlatformInfo &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxMax(int, int) file: line:65
	M("").def("wxMax", (int (*)(int, int)) &wxMax<int,int>, "C++: wxMax(int, int) --> int", pybind11::arg("a"), pybind11::arg("b"));

	// wxStringEq(const char *, const char *) file: line:117
	M("").def("wxStringEq", (bool (*)(const char *, const char *)) &wxStringEq, "C++: wxStringEq(const char *, const char *) --> bool", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStringEq(const wchar_t *, const wchar_t *) file: line:121
	M("").def("wxStringEq", (bool (*)(const wchar_t *, const wchar_t *)) &wxStringEq, "C++: wxStringEq(const wchar_t *, const wchar_t *) --> bool", pybind11::arg("s1"), pybind11::arg("s2"));

}
