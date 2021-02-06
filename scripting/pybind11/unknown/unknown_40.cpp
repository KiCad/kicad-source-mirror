#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/dc.h> // wxDC
#include <wx/process.h> // wxProcess

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_unknown_unknown_40(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxBell() file: line:132
	M("").def("wxBell", (void (*)()) &wxBell, "C++: wxBell() --> void");

	// wxInfoMessageBox(class wxWindow *) file: line:136
	M("").def("wxInfoMessageBox", (void (*)(class wxWindow *)) &wxInfoMessageBox, "C++: wxInfoMessageBox(class wxWindow *) --> void", pybind11::arg("parent"));

	// wxGetLibraryVersionInfo() file: line:139
	M("").def("wxGetLibraryVersionInfo", (class wxVersionInfo (*)()) &wxGetLibraryVersionInfo, "C++: wxGetLibraryVersionInfo() --> class wxVersionInfo");

	// wxGetOsDescription() file: line:142
	M("").def("wxGetOsDescription", (class wxString (*)()) &wxGetOsDescription, "C++: wxGetOsDescription() --> class wxString");

	// wxGetOsVersion(int *, int *) file: line:145
	M("").def("wxGetOsVersion", []() -> wxOperatingSystemId { return wxGetOsVersion(); }, "");
	M("").def("wxGetOsVersion", [](int * a0) -> wxOperatingSystemId { return wxGetOsVersion(a0); }, "", pybind11::arg("majorVsn"));
	M("").def("wxGetOsVersion", (enum wxOperatingSystemId (*)(int *, int *)) &wxGetOsVersion, "C++: wxGetOsVersion(int *, int *) --> enum wxOperatingSystemId", pybind11::arg("majorVsn"), pybind11::arg("minorVsn"));

	// wxIsPlatformLittleEndian() file: line:149
	M("").def("wxIsPlatformLittleEndian", (bool (*)()) &wxIsPlatformLittleEndian, "C++: wxIsPlatformLittleEndian() --> bool");

	// wxIsPlatform64Bit() file: line:152
	M("").def("wxIsPlatform64Bit", (bool (*)()) &wxIsPlatform64Bit, "C++: wxIsPlatform64Bit() --> bool");

	// wxGetLinuxDistributionInfo() file: line:156
	M("").def("wxGetLinuxDistributionInfo", (struct wxLinuxDistributionInfo (*)()) &wxGetLinuxDistributionInfo, "C++: wxGetLinuxDistributionInfo() --> struct wxLinuxDistributionInfo");

	// wxNow() file: line:160
	M("").def("wxNow", (class wxString (*)()) &wxNow, "C++: wxNow() --> class wxString");

	// wxGetInstallPrefix() file: line:163
	M("").def("wxGetInstallPrefix", (const wchar_t * (*)()) &wxGetInstallPrefix, "C++: wxGetInstallPrefix() --> const wchar_t *", pybind11::return_value_policy::automatic);

	// wxGetDataDir() file: line:165
	M("").def("wxGetDataDir", (class wxString (*)()) &wxGetDataDir, "C++: wxGetDataDir() --> class wxString");

	// wxGetKeyState(enum wxKeyCode) file: line:172
	M("").def("wxGetKeyState", (bool (*)(enum wxKeyCode)) &wxGetKeyState, "C++: wxGetKeyState(enum wxKeyCode) --> bool", pybind11::arg("key"));

	// wxSetDetectableAutoRepeat(bool) file: line:177
	M("").def("wxSetDetectableAutoRepeat", (bool (*)(bool)) &wxSetDetectableAutoRepeat, "C++: wxSetDetectableAutoRepeat(bool) --> bool", pybind11::arg("flag"));

	// wxGetMouseState() file: line:180
	M("").def("wxGetMouseState", (class wxMouseState (*)()) &wxGetMouseState, "C++: wxGetMouseState() --> class wxMouseState");

	{ // wxPlatform file: line:206
		pybind11::class_<wxPlatform, std::shared_ptr<wxPlatform>> cl(M(""), "wxPlatform", "");
		cl.def( pybind11::init( [](){ return new wxPlatform(); } ) );
		cl.def( pybind11::init( [](wxPlatform const &o){ return new wxPlatform(o); } ) );
		cl.def( pybind11::init<int>(), pybind11::arg("defValue") );

		cl.def( pybind11::init<long>(), pybind11::arg("defValue") );

		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("defValue") );

		cl.def( pybind11::init<double>(), pybind11::arg("defValue") );

		cl.def("assign", (void (wxPlatform::*)(const class wxPlatform &)) &wxPlatform::operator=, "C++: wxPlatform::operator=(const class wxPlatform &) --> void", pybind11::arg("platform"));
		cl.def("Copy", (void (wxPlatform::*)(const class wxPlatform &)) &wxPlatform::Copy, "C++: wxPlatform::Copy(const class wxPlatform &) --> void", pybind11::arg("platform"));
		cl.def_static("If", (class wxPlatform (*)(int, long)) &wxPlatform::If, "C++: wxPlatform::If(int, long) --> class wxPlatform", pybind11::arg("platform"), pybind11::arg("value"));
		cl.def_static("IfNot", (class wxPlatform (*)(int, long)) &wxPlatform::IfNot, "C++: wxPlatform::IfNot(int, long) --> class wxPlatform", pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("ElseIf", (class wxPlatform & (wxPlatform::*)(int, long)) &wxPlatform::ElseIf, "C++: wxPlatform::ElseIf(int, long) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("ElseIfNot", (class wxPlatform & (wxPlatform::*)(int, long)) &wxPlatform::ElseIfNot, "C++: wxPlatform::ElseIfNot(int, long) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("Else", (class wxPlatform & (wxPlatform::*)(long)) &wxPlatform::Else, "C++: wxPlatform::Else(long) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def_static("If", (class wxPlatform (*)(int, int)) &wxPlatform::If, "C++: wxPlatform::If(int, int) --> class wxPlatform", pybind11::arg("platform"), pybind11::arg("value"));
		cl.def_static("IfNot", (class wxPlatform (*)(int, int)) &wxPlatform::IfNot, "C++: wxPlatform::IfNot(int, int) --> class wxPlatform", pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("ElseIf", (class wxPlatform & (wxPlatform::*)(int, int)) &wxPlatform::ElseIf, "C++: wxPlatform::ElseIf(int, int) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("ElseIfNot", (class wxPlatform & (wxPlatform::*)(int, int)) &wxPlatform::ElseIfNot, "C++: wxPlatform::ElseIfNot(int, int) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("Else", (class wxPlatform & (wxPlatform::*)(int)) &wxPlatform::Else, "C++: wxPlatform::Else(int) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def_static("If", (class wxPlatform (*)(int, double)) &wxPlatform::If, "C++: wxPlatform::If(int, double) --> class wxPlatform", pybind11::arg("platform"), pybind11::arg("value"));
		cl.def_static("IfNot", (class wxPlatform (*)(int, double)) &wxPlatform::IfNot, "C++: wxPlatform::IfNot(int, double) --> class wxPlatform", pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("ElseIf", (class wxPlatform & (wxPlatform::*)(int, double)) &wxPlatform::ElseIf, "C++: wxPlatform::ElseIf(int, double) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("ElseIfNot", (class wxPlatform & (wxPlatform::*)(int, double)) &wxPlatform::ElseIfNot, "C++: wxPlatform::ElseIfNot(int, double) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("Else", (class wxPlatform & (wxPlatform::*)(double)) &wxPlatform::Else, "C++: wxPlatform::Else(double) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def_static("If", (class wxPlatform (*)(int, const class wxString &)) &wxPlatform::If, "C++: wxPlatform::If(int, const class wxString &) --> class wxPlatform", pybind11::arg("platform"), pybind11::arg("value"));
		cl.def_static("IfNot", (class wxPlatform (*)(int, const class wxString &)) &wxPlatform::IfNot, "C++: wxPlatform::IfNot(int, const class wxString &) --> class wxPlatform", pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("ElseIf", (class wxPlatform & (wxPlatform::*)(int, const class wxString &)) &wxPlatform::ElseIf, "C++: wxPlatform::ElseIf(int, const class wxString &) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("ElseIfNot", (class wxPlatform & (wxPlatform::*)(int, const class wxString &)) &wxPlatform::ElseIfNot, "C++: wxPlatform::ElseIfNot(int, const class wxString &) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("platform"), pybind11::arg("value"));
		cl.def("Else", (class wxPlatform & (wxPlatform::*)(const class wxString &)) &wxPlatform::Else, "C++: wxPlatform::Else(const class wxString &) --> class wxPlatform &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("GetInteger", (long (wxPlatform::*)() const) &wxPlatform::GetInteger, "C++: wxPlatform::GetInteger() const --> long");
		cl.def("GetString", (const class wxString & (wxPlatform::*)() const) &wxPlatform::GetString, "C++: wxPlatform::GetString() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetDouble", (double (wxPlatform::*)() const) &wxPlatform::GetDouble, "C++: wxPlatform::GetDouble() const --> double");
		cl.def_static("AddPlatform", (void (*)(int)) &wxPlatform::AddPlatform, "C++: wxPlatform::AddPlatform(int) --> void", pybind11::arg("platform"));
		cl.def_static("Is", (bool (*)(int)) &wxPlatform::Is, "C++: wxPlatform::Is(int) --> bool", pybind11::arg("platform"));
		cl.def_static("ClearPlatforms", (void (*)()) &wxPlatform::ClearPlatforms, "C++: wxPlatform::ClearPlatforms() --> void");
	}
	// wxPlatformIs(int) file: line:268
	M("").def("wxPlatformIs", (bool (*)(int)) &wxPlatformIs, "Function for testing current platform\n\nC++: wxPlatformIs(int) --> bool", pybind11::arg("platform"));

	// wxRegisterId(int) file: line:275
	M("").def("wxRegisterId", (void (*)(int)) &wxRegisterId, "C++: wxRegisterId(int) --> void", pybind11::arg("id"));

	// wxGetCurrentId() file: line:278
	M("").def("wxGetCurrentId", (int (*)()) &wxGetCurrentId, "C++: wxGetCurrentId() --> int");

	// wxNewId() file: line:281
	M("").def("wxNewId", (int (*)()) &wxNewId, "C++: wxNewId() --> int");

	// wxHexToDec(const class wxString &) file: line:288
	M("").def("wxHexToDec", (int (*)(const class wxString &)) &wxHexToDec, "C++: wxHexToDec(const class wxString &) --> int", pybind11::arg("buf"));

	// wxHexToDec(const char *) file: line:291
	M("").def("wxHexToDec", (int (*)(const char *)) &wxHexToDec, "C++: wxHexToDec(const char *) --> int", pybind11::arg("buf"));

	// wxDecToHex(int, wchar_t *) file: line:310
	M("").def("wxDecToHex", (void (*)(int, wchar_t *)) &wxDecToHex, "C++: wxDecToHex(int, wchar_t *) --> void", pybind11::arg("dec"), pybind11::arg("buf"));

	// wxDecToHex(int, char *, char *) file: line:311
	M("").def("wxDecToHex", (void (*)(int, char *, char *)) &wxDecToHex, "C++: wxDecToHex(int, char *, char *) --> void", pybind11::arg("dec"), pybind11::arg("ch1"), pybind11::arg("ch2"));

	// wxDecToHex(int) file: line:312
	M("").def("wxDecToHex", (class wxString (*)(int)) &wxDecToHex, "C++: wxDecToHex(int) --> class wxString", pybind11::arg("dec"));

	{ // wxExecuteEnv file: line:367
		pybind11::class_<wxExecuteEnv, std::shared_ptr<wxExecuteEnv>> cl(M(""), "wxExecuteEnv", "");
		cl.def( pybind11::init( [](){ return new wxExecuteEnv(); } ) );
		cl.def( pybind11::init( [](wxExecuteEnv const &o){ return new wxExecuteEnv(o); } ) );
		cl.def_readwrite("cwd", &wxExecuteEnv::cwd);
		cl.def_readwrite("env", &wxExecuteEnv::env);
		cl.def("assign", (struct wxExecuteEnv & (wxExecuteEnv::*)(const struct wxExecuteEnv &)) &wxExecuteEnv::operator=, "C++: wxExecuteEnv::operator=(const struct wxExecuteEnv &) --> struct wxExecuteEnv &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxExecute(const class wxString &, int, class wxProcess *, const struct wxExecuteEnv *) file: line:378
	M("").def("wxExecute", [](const class wxString & a0) -> long { return wxExecute(a0); }, "", pybind11::arg("command"));
	M("").def("wxExecute", [](const class wxString & a0, int const & a1) -> long { return wxExecute(a0, a1); }, "", pybind11::arg("command"), pybind11::arg("flags"));
	M("").def("wxExecute", [](const class wxString & a0, int const & a1, class wxProcess * a2) -> long { return wxExecute(a0, a1, a2); }, "", pybind11::arg("command"), pybind11::arg("flags"), pybind11::arg("process"));
	M("").def("wxExecute", (long (*)(const class wxString &, int, class wxProcess *, const struct wxExecuteEnv *)) &wxExecute, "C++: wxExecute(const class wxString &, int, class wxProcess *, const struct wxExecuteEnv *) --> long", pybind11::arg("command"), pybind11::arg("flags"), pybind11::arg("process"), pybind11::arg("env"));

	// wxExecute(const class wxString &, class wxArrayString &, int, const struct wxExecuteEnv *) file: line:395
	M("").def("wxExecute", [](const class wxString & a0, class wxArrayString & a1) -> long { return wxExecute(a0, a1); }, "", pybind11::arg("command"), pybind11::arg("output"));
	M("").def("wxExecute", [](const class wxString & a0, class wxArrayString & a1, int const & a2) -> long { return wxExecute(a0, a1, a2); }, "", pybind11::arg("command"), pybind11::arg("output"), pybind11::arg("flags"));
	M("").def("wxExecute", (long (*)(const class wxString &, class wxArrayString &, int, const struct wxExecuteEnv *)) &wxExecute, "C++: wxExecute(const class wxString &, class wxArrayString &, int, const struct wxExecuteEnv *) --> long", pybind11::arg("command"), pybind11::arg("output"), pybind11::arg("flags"), pybind11::arg("env"));

	// wxExecute(const class wxString &, class wxArrayString &, class wxArrayString &, int, const struct wxExecuteEnv *) file: line:401
	M("").def("wxExecute", [](const class wxString & a0, class wxArrayString & a1, class wxArrayString & a2) -> long { return wxExecute(a0, a1, a2); }, "", pybind11::arg("command"), pybind11::arg("output"), pybind11::arg("error"));
	M("").def("wxExecute", [](const class wxString & a0, class wxArrayString & a1, class wxArrayString & a2, int const & a3) -> long { return wxExecute(a0, a1, a2, a3); }, "", pybind11::arg("command"), pybind11::arg("output"), pybind11::arg("error"), pybind11::arg("flags"));
	M("").def("wxExecute", (long (*)(const class wxString &, class wxArrayString &, class wxArrayString &, int, const struct wxExecuteEnv *)) &wxExecute, "C++: wxExecute(const class wxString &, class wxArrayString &, class wxArrayString &, int, const struct wxExecuteEnv *) --> long", pybind11::arg("command"), pybind11::arg("output"), pybind11::arg("error"), pybind11::arg("flags"), pybind11::arg("env"));

	// wxSignal file: line:414
	pybind11::enum_<wxSignal>(M(""), "wxSignal", pybind11::arithmetic(), "")
		.value("wxSIGNONE", wxSIGNONE)
		.value("wxSIGHUP", wxSIGHUP)
		.value("wxSIGINT", wxSIGINT)
		.value("wxSIGQUIT", wxSIGQUIT)
		.value("wxSIGILL", wxSIGILL)
		.value("wxSIGTRAP", wxSIGTRAP)
		.value("wxSIGABRT", wxSIGABRT)
		.value("wxSIGIOT", wxSIGIOT)
		.value("wxSIGEMT", wxSIGEMT)
		.value("wxSIGFPE", wxSIGFPE)
		.value("wxSIGKILL", wxSIGKILL)
		.value("wxSIGBUS", wxSIGBUS)
		.value("wxSIGSEGV", wxSIGSEGV)
		.value("wxSIGSYS", wxSIGSYS)
		.value("wxSIGPIPE", wxSIGPIPE)
		.value("wxSIGALRM", wxSIGALRM)
		.value("wxSIGTERM", wxSIGTERM)
		.export_values();

;

	// wxKillError file: line:437
	pybind11::enum_<wxKillError>(M(""), "wxKillError", pybind11::arithmetic(), "")
		.value("wxKILL_OK", wxKILL_OK)
		.value("wxKILL_BAD_SIGNAL", wxKILL_BAD_SIGNAL)
		.value("wxKILL_ACCESS_DENIED", wxKILL_ACCESS_DENIED)
		.value("wxKILL_NO_PROCESS", wxKILL_NO_PROCESS)
		.value("wxKILL_ERROR", wxKILL_ERROR)
		.export_values();

;

	// wxKillFlags file: line:446
	pybind11::enum_<wxKillFlags>(M(""), "wxKillFlags", pybind11::arithmetic(), "")
		.value("wxKILL_NOCHILDREN", wxKILL_NOCHILDREN)
		.value("wxKILL_CHILDREN", wxKILL_CHILDREN)
		.export_values();

;

	// wxShutdownFlags file: line:452
	pybind11::enum_<wxShutdownFlags>(M(""), "wxShutdownFlags", pybind11::arithmetic(), "")
		.value("wxSHUTDOWN_FORCE", wxSHUTDOWN_FORCE)
		.value("wxSHUTDOWN_POWEROFF", wxSHUTDOWN_POWEROFF)
		.value("wxSHUTDOWN_REBOOT", wxSHUTDOWN_REBOOT)
		.value("wxSHUTDOWN_LOGOFF", wxSHUTDOWN_LOGOFF)
		.export_values();

;

	// wxShutdown(int) file: line:461
	M("").def("wxShutdown", []() -> bool { return wxShutdown(); }, "");
	M("").def("wxShutdown", (bool (*)(int)) &wxShutdown, "C++: wxShutdown(int) --> bool", pybind11::arg("flags"));

	// wxKill(long, enum wxSignal, enum wxKillError *, int) file: line:467
	M("").def("wxKill", [](long const & a0) -> int { return wxKill(a0); }, "", pybind11::arg("pid"));
	M("").def("wxKill", [](long const & a0, enum wxSignal const & a1) -> int { return wxKill(a0, a1); }, "", pybind11::arg("pid"), pybind11::arg("sig"));
	M("").def("wxKill", [](long const & a0, enum wxSignal const & a1, enum wxKillError * a2) -> int { return wxKill(a0, a1, a2); }, "", pybind11::arg("pid"), pybind11::arg("sig"), pybind11::arg("rc"));
	M("").def("wxKill", (int (*)(long, enum wxSignal, enum wxKillError *, int)) &wxKill, "C++: wxKill(long, enum wxSignal, enum wxKillError *, int) --> int", pybind11::arg("pid"), pybind11::arg("sig"), pybind11::arg("rc"), pybind11::arg("flags"));

}
