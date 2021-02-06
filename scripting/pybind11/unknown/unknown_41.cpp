#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/dc.h> // wxDC

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_unknown_unknown_41(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxShell(const class wxString &) file: line:474
	M("").def("wxShell", []() -> bool { return wxShell(); }, "");
	M("").def("wxShell", (bool (*)(const class wxString &)) &wxShell, "C++: wxShell(const class wxString &) --> bool", pybind11::arg("command"));

	// wxShell(const class wxString &, class wxArrayString &) file: line:478
	M("").def("wxShell", (bool (*)(const class wxString &, class wxArrayString &)) &wxShell, "C++: wxShell(const class wxString &, class wxArrayString &) --> bool", pybind11::arg("command"), pybind11::arg("output"));

	// wxSleep(int) file: line:481
	M("").def("wxSleep", (void (*)(int)) &wxSleep, "C++: wxSleep(int) --> void", pybind11::arg("nSecs"));

	// wxMilliSleep(unsigned long) file: line:484
	M("").def("wxMilliSleep", (void (*)(unsigned long)) &wxMilliSleep, "C++: wxMilliSleep(unsigned long) --> void", pybind11::arg("milliseconds"));

	// wxMicroSleep(unsigned long) file: line:487
	M("").def("wxMicroSleep", (void (*)(unsigned long)) &wxMicroSleep, "C++: wxMicroSleep(unsigned long) --> void", pybind11::arg("microseconds"));

	// wxUsleep(unsigned long) file: line:491
	M("").def("wxUsleep", (void (*)(unsigned long)) &wxUsleep, "C++: wxUsleep(unsigned long) --> void", pybind11::arg("milliseconds"));

	// wxGetProcessId() file: line:495
	M("").def("wxGetProcessId", (unsigned long (*)()) &wxGetProcessId, "C++: wxGetProcessId() --> unsigned long");

	// wxGetFreeMemory() file: line:498
	M("").def("wxGetFreeMemory", (class wxLongLongNative (*)()) &wxGetFreeMemory, "C++: wxGetFreeMemory() --> class wxLongLongNative");

	// wxHandleFatalExceptions(bool) file: line:503
	M("").def("wxHandleFatalExceptions", []() -> bool { return wxHandleFatalExceptions(); }, "");
	M("").def("wxHandleFatalExceptions", (bool (*)(bool)) &wxHandleFatalExceptions, "C++: wxHandleFatalExceptions(bool) --> bool", pybind11::arg("doit"));

	// wxGetEnv(const class wxString &, class wxString *) file: line:513
	M("").def("wxGetEnv", (bool (*)(const class wxString &, class wxString *)) &wxGetEnv, "C++: wxGetEnv(const class wxString &, class wxString *) --> bool", pybind11::arg("var"), pybind11::arg("value"));

	// wxSetEnv(const class wxString &, const class wxString &) file: line:516
	M("").def("wxSetEnv", (bool (*)(const class wxString &, const class wxString &)) &wxSetEnv, "C++: wxSetEnv(const class wxString &, const class wxString &) --> bool", pybind11::arg("var"), pybind11::arg("value"));

	// wxUnsetEnv(const class wxString &) file: line:519
	M("").def("wxUnsetEnv", (bool (*)(const class wxString &)) &wxUnsetEnv, "C++: wxUnsetEnv(const class wxString &) --> bool", pybind11::arg("var"));

	// wxSetEnv(const class wxString &, const char *) file: line:522
	M("").def("wxSetEnv", (bool (*)(const class wxString &, const char *)) &wxSetEnv, "C++: wxSetEnv(const class wxString &, const char *) --> bool", pybind11::arg("var"), pybind11::arg("value"));

	// wxSetEnv(const class wxString &, const wchar_t *) file: line:524
	M("").def("wxSetEnv", (bool (*)(const class wxString &, const wchar_t *)) &wxSetEnv, "C++: wxSetEnv(const class wxString &, const wchar_t *) --> bool", pybind11::arg("var"), pybind11::arg("value"));

	// wxSetEnv(const class wxString &, const class wxCStrData &) file: line:529
	M("").def("wxSetEnv", (bool (*)(const class wxString &, const class wxCStrData &)) &wxSetEnv, "C++: wxSetEnv(const class wxString &, const class wxCStrData &) --> bool", pybind11::arg("var"), pybind11::arg("value"));

	// wxSetEnv(const class wxString &, int) file: line:533
	M("").def("wxSetEnv", (bool (*)(const class wxString &, int)) &wxSetEnv, "C++: wxSetEnv(const class wxString &, int) --> bool", pybind11::arg("var"), pybind11::arg("value"));

	// wxGetEnvMap(class wxStringToStringHashMap *) file: line:546
	M("").def("wxGetEnvMap", (bool (*)(class wxStringToStringHashMap *)) &wxGetEnvMap, "C++: wxGetEnvMap(class wxStringToStringHashMap *) --> bool", pybind11::arg("map"));

	// wxGetEmailAddress(wchar_t *, int) file: line:555
	M("").def("wxGetEmailAddress", (bool (*)(wchar_t *, int)) &wxGetEmailAddress, "C++: wxGetEmailAddress(wchar_t *, int) --> bool", pybind11::arg("buf"), pybind11::arg("maxSize"));

	// wxGetEmailAddress() file: line:556
	M("").def("wxGetEmailAddress", (class wxString (*)()) &wxGetEmailAddress, "C++: wxGetEmailAddress() --> class wxString");

	// wxGetHostName(wchar_t *, int) file: line:559
	M("").def("wxGetHostName", (bool (*)(wchar_t *, int)) &wxGetHostName, "C++: wxGetHostName(wchar_t *, int) --> bool", pybind11::arg("buf"), pybind11::arg("maxSize"));

	// wxGetHostName() file: line:560
	M("").def("wxGetHostName", (class wxString (*)()) &wxGetHostName, "C++: wxGetHostName() --> class wxString");

	// wxGetFullHostName() file: line:563
	M("").def("wxGetFullHostName", (class wxString (*)()) &wxGetFullHostName, "C++: wxGetFullHostName() --> class wxString");

	// wxGetFullHostName(wchar_t *, int) file: line:564
	M("").def("wxGetFullHostName", (bool (*)(wchar_t *, int)) &wxGetFullHostName, "C++: wxGetFullHostName(wchar_t *, int) --> bool", pybind11::arg("buf"), pybind11::arg("maxSize"));

	// wxGetUserId(wchar_t *, int) file: line:567
	M("").def("wxGetUserId", (bool (*)(wchar_t *, int)) &wxGetUserId, "C++: wxGetUserId(wchar_t *, int) --> bool", pybind11::arg("buf"), pybind11::arg("maxSize"));

	// wxGetUserId() file: line:568
	M("").def("wxGetUserId", (class wxString (*)()) &wxGetUserId, "C++: wxGetUserId() --> class wxString");

	// wxGetUserName(wchar_t *, int) file: line:571
	M("").def("wxGetUserName", (bool (*)(wchar_t *, int)) &wxGetUserName, "C++: wxGetUserName(wchar_t *, int) --> bool", pybind11::arg("buf"), pybind11::arg("maxSize"));

	// wxGetUserName() file: line:572
	M("").def("wxGetUserName", (class wxString (*)()) &wxGetUserName, "C++: wxGetUserName() --> class wxString");

	// wxGetHomeDir() file: line:575
	M("").def("wxGetHomeDir", (class wxString (*)()) &wxGetHomeDir, "C++: wxGetHomeDir() --> class wxString");

	// wxGetHomeDir(class wxString *) file: line:576
	M("").def("wxGetHomeDir", (const wchar_t * (*)(class wxString *)) &wxGetHomeDir, "C++: wxGetHomeDir(class wxString *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("pstr"));

	// wxGetUserHome(const class wxString &) file: line:580
	M("").def("wxGetUserHome", []() -> wxString { return wxGetUserHome(); }, "");
	M("").def("wxGetUserHome", (class wxString (*)(const class wxString &)) &wxGetUserHome, "C++: wxGetUserHome(const class wxString &) --> class wxString", pybind11::arg("user"));

	// wxGetDiskSpace(const class wxString &, class wxLongLongNative *, class wxLongLongNative *) file: line:590
	M("").def("wxGetDiskSpace", [](const class wxString & a0) -> bool { return wxGetDiskSpace(a0); }, "", pybind11::arg("path"));
	M("").def("wxGetDiskSpace", [](const class wxString & a0, class wxLongLongNative * a1) -> bool { return wxGetDiskSpace(a0, a1); }, "", pybind11::arg("path"), pybind11::arg("pTotal"));
	M("").def("wxGetDiskSpace", (bool (*)(const class wxString &, class wxLongLongNative *, class wxLongLongNative *)) &wxGetDiskSpace, "C++: wxGetDiskSpace(const class wxString &, class wxLongLongNative *, class wxLongLongNative *) --> bool", pybind11::arg("path"), pybind11::arg("pTotal"), pybind11::arg("pFree"));

	// wxLaunchDefaultBrowser(const class wxString &, int) file: line:620
	M("").def("wxLaunchDefaultBrowser", [](const class wxString & a0) -> bool { return wxLaunchDefaultBrowser(a0); }, "", pybind11::arg("url"));
	M("").def("wxLaunchDefaultBrowser", (bool (*)(const class wxString &, int)) &wxLaunchDefaultBrowser, "C++: wxLaunchDefaultBrowser(const class wxString &, int) --> bool", pybind11::arg("url"), pybind11::arg("flags"));

	// wxLaunchDefaultApplication(const class wxString &, int) file: line:623
	M("").def("wxLaunchDefaultApplication", [](const class wxString & a0) -> bool { return wxLaunchDefaultApplication(a0); }, "", pybind11::arg("path"));
	M("").def("wxLaunchDefaultApplication", (bool (*)(const class wxString &, int)) &wxLaunchDefaultApplication, "C++: wxLaunchDefaultApplication(const class wxString &, int) --> bool", pybind11::arg("path"), pybind11::arg("flags"));

	// wxStripMenuCodes(const class wxString &, int) file: line:644
	M("").def("wxStripMenuCodes", [](const class wxString & a0) -> wxString { return wxStripMenuCodes(a0); }, "", pybind11::arg("str"));
	M("").def("wxStripMenuCodes", (class wxString (*)(const class wxString &, int)) &wxStripMenuCodes, "C++: wxStripMenuCodes(const class wxString &, int) --> class wxString", pybind11::arg("str"), pybind11::arg("flags"));

	// wxGenericFindWindowAtPoint(const class wxPoint &) file: line:672
	M("").def("wxGenericFindWindowAtPoint", (class wxWindow * (*)(const class wxPoint &)) &wxGenericFindWindowAtPoint, "C++: wxGenericFindWindowAtPoint(const class wxPoint &) --> class wxWindow *", pybind11::return_value_policy::automatic, pybind11::arg("pt"));

	// wxFindWindowAtPoint(const class wxPoint &) file: line:673
	M("").def("wxFindWindowAtPoint", (class wxWindow * (*)(const class wxPoint &)) &wxFindWindowAtPoint, "C++: wxFindWindowAtPoint(const class wxPoint &) --> class wxWindow *", pybind11::return_value_policy::automatic, pybind11::arg("pt"));

	// wxFindWindowByLabel(const class wxString &, class wxWindow *) file: line:680
	M("").def("wxFindWindowByLabel", [](const class wxString & a0) -> wxWindow * { return wxFindWindowByLabel(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("title"));
	M("").def("wxFindWindowByLabel", (class wxWindow * (*)(const class wxString &, class wxWindow *)) &wxFindWindowByLabel, "C++: wxFindWindowByLabel(const class wxString &, class wxWindow *) --> class wxWindow *", pybind11::return_value_policy::automatic, pybind11::arg("title"), pybind11::arg("parent"));

	// wxFindWindowByName(const class wxString &, class wxWindow *) file: line:685
	M("").def("wxFindWindowByName", [](const class wxString & a0) -> wxWindow * { return wxFindWindowByName(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("name"));
	M("").def("wxFindWindowByName", (class wxWindow * (*)(const class wxString &, class wxWindow *)) &wxFindWindowByName, "C++: wxFindWindowByName(const class wxString &, class wxWindow *) --> class wxWindow *", pybind11::return_value_policy::automatic, pybind11::arg("name"), pybind11::arg("parent"));

	// wxSafeYield(class wxWindow *, bool) file: line:692
	M("").def("wxSafeYield", []() -> bool { return wxSafeYield(); }, "");
	M("").def("wxSafeYield", [](class wxWindow * a0) -> bool { return wxSafeYield(a0); }, "", pybind11::arg("win"));
	M("").def("wxSafeYield", (bool (*)(class wxWindow *, bool)) &wxSafeYield, "C++: wxSafeYield(class wxWindow *, bool) --> bool", pybind11::arg("win"), pybind11::arg("onlyIfNeeded"));

	// wxEnableTopLevelWindows(bool) file: line:695
	M("").def("wxEnableTopLevelWindows", []() -> void { return wxEnableTopLevelWindows(); }, "");
	M("").def("wxEnableTopLevelWindows", (void (*)(bool)) &wxEnableTopLevelWindows, "C++: wxEnableTopLevelWindows(bool) --> void", pybind11::arg("enable"));

	// wxCheckForInterrupt(class wxWindow *) file: line:699
	M("").def("wxCheckForInterrupt", (bool (*)(class wxWindow *)) &wxCheckForInterrupt, "C++: wxCheckForInterrupt(class wxWindow *) --> bool", pybind11::arg("wnd"));

	// wxFlushEvents() file: line:702
	M("").def("wxFlushEvents", (void (*)()) &wxFlushEvents, "C++: wxFlushEvents() --> void");

	{ // wxWindowDisabler file: line:706
		pybind11::class_<wxWindowDisabler, std::shared_ptr<wxWindowDisabler>> cl(M(""), "wxWindowDisabler", "");
		cl.def( pybind11::init( [](){ return new wxWindowDisabler(); } ), "doc" );
		cl.def( pybind11::init<bool>(), pybind11::arg("disable") );

		cl.def( pybind11::init<class wxWindow *>(), pybind11::arg("winToSkip") );

	}
	// wxBeginBusyCursor(const class wxCursor *) file: line:737
	M("").def("wxBeginBusyCursor", []() -> void { return wxBeginBusyCursor(); }, "");
	M("").def("wxBeginBusyCursor", (void (*)(const class wxCursor *)) &wxBeginBusyCursor, "C++: wxBeginBusyCursor(const class wxCursor *) --> void", pybind11::arg("cursor"));

	// wxEndBusyCursor() file: line:740
	M("").def("wxEndBusyCursor", (void (*)()) &wxEndBusyCursor, "C++: wxEndBusyCursor() --> void");

	// wxIsBusy() file: line:743
	M("").def("wxIsBusy", (bool (*)()) &wxIsBusy, "C++: wxIsBusy() --> bool");

	{ // wxBusyCursor file: line:746
		pybind11::class_<wxBusyCursor, std::shared_ptr<wxBusyCursor>> cl(M(""), "wxBusyCursor", "");
		cl.def( pybind11::init( [](){ return new wxBusyCursor(); } ), "doc" );
		cl.def( pybind11::init<const class wxCursor *>(), pybind11::arg("cursor") );

		cl.def_static("GetStoredCursor", (const class wxCursor & (*)()) &wxBusyCursor::GetStoredCursor, "C++: wxBusyCursor::GetStoredCursor() --> const class wxCursor &", pybind11::return_value_policy::automatic);
		cl.def_static("GetBusyCursor", (const class wxCursor (*)()) &wxBusyCursor::GetBusyCursor, "C++: wxBusyCursor::GetBusyCursor() --> const class wxCursor");
	}
	// wxGetMousePosition(int *, int *) file: line:759
	M("").def("wxGetMousePosition", (void (*)(int *, int *)) &wxGetMousePosition, "C++: wxGetMousePosition(int *, int *) --> void", pybind11::arg("x"), pybind11::arg("y"));

	// wxYield() file: line:797
	M("").def("wxYield", (bool (*)()) &wxYield, "C++: wxYield() --> bool");

	// wxYieldIfNeeded() file: line:802
	M("").def("wxYieldIfNeeded", (bool (*)()) &wxYieldIfNeeded, "C++: wxYieldIfNeeded() --> bool");

	{ // wxBusyCursorSuspender file: line:88
		pybind11::class_<wxBusyCursorSuspender, std::shared_ptr<wxBusyCursorSuspender>> cl(M(""), "wxBusyCursorSuspender", "");
		cl.def( pybind11::init( [](){ return new wxBusyCursorSuspender(); } ) );
	}
	{ // wxBaseArrayPtrVoid file: line:837
		pybind11::class_<wxBaseArrayPtrVoid, std::shared_ptr<wxBaseArrayPtrVoid>> cl(M(""), "wxBaseArrayPtrVoid", "");
		cl.def( pybind11::init( [](){ return new wxBaseArrayPtrVoid(); } ) );
		cl.def( pybind11::init( [](wxBaseArrayPtrVoid const &o){ return new wxBaseArrayPtrVoid(o); } ) );
		cl.def("assign", (class wxBaseArrayPtrVoid & (wxBaseArrayPtrVoid::*)(const class wxBaseArrayPtrVoid &)) &wxBaseArrayPtrVoid::operator=, "C++: wxBaseArrayPtrVoid::operator=(const class wxBaseArrayPtrVoid &) --> class wxBaseArrayPtrVoid &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("Empty", (void (wxBaseArrayPtrVoid::*)()) &wxBaseArrayPtrVoid::Empty, "C++: wxBaseArrayPtrVoid::Empty() --> void");
		cl.def("Clear", (void (wxBaseArrayPtrVoid::*)()) &wxBaseArrayPtrVoid::Clear, "C++: wxBaseArrayPtrVoid::Clear() --> void");
		cl.def("Alloc", (void (wxBaseArrayPtrVoid::*)(unsigned long)) &wxBaseArrayPtrVoid::Alloc, "C++: wxBaseArrayPtrVoid::Alloc(unsigned long) --> void", pybind11::arg("n"));
		cl.def("Shrink", (void (wxBaseArrayPtrVoid::*)()) &wxBaseArrayPtrVoid::Shrink, "C++: wxBaseArrayPtrVoid::Shrink() --> void");
		cl.def("GetCount", (unsigned long (wxBaseArrayPtrVoid::*)() const) &wxBaseArrayPtrVoid::GetCount, "C++: wxBaseArrayPtrVoid::GetCount() const --> unsigned long");
		cl.def("SetCount", [](wxBaseArrayPtrVoid &o, unsigned long const & a0) -> void { return o.SetCount(a0); }, "", pybind11::arg("n"));
		cl.def("SetCount", (void (wxBaseArrayPtrVoid::*)(unsigned long, const void *)) &wxBaseArrayPtrVoid::SetCount, "C++: wxBaseArrayPtrVoid::SetCount(unsigned long, const void *) --> void", pybind11::arg("n"), pybind11::arg("defval"));
		cl.def("IsEmpty", (bool (wxBaseArrayPtrVoid::*)() const) &wxBaseArrayPtrVoid::IsEmpty, "C++: wxBaseArrayPtrVoid::IsEmpty() const --> bool");
		cl.def("Count", (unsigned long (wxBaseArrayPtrVoid::*)() const) &wxBaseArrayPtrVoid::Count, "C++: wxBaseArrayPtrVoid::Count() const --> unsigned long");
		cl.def("clear", (void (wxBaseArrayPtrVoid::*)()) &wxBaseArrayPtrVoid::clear, "C++: wxBaseArrayPtrVoid::clear() --> void");
		cl.def("empty", (bool (wxBaseArrayPtrVoid::*)() const) &wxBaseArrayPtrVoid::empty, "C++: wxBaseArrayPtrVoid::empty() const --> bool");
		cl.def("max_size", (unsigned long (wxBaseArrayPtrVoid::*)() const) &wxBaseArrayPtrVoid::max_size, "C++: wxBaseArrayPtrVoid::max_size() const --> unsigned long");
		cl.def("size", (unsigned long (wxBaseArrayPtrVoid::*)() const) &wxBaseArrayPtrVoid::size, "C++: wxBaseArrayPtrVoid::size() const --> unsigned long");
	}
	{ // wxBaseArrayChar file: line:839
		pybind11::class_<wxBaseArrayChar, std::shared_ptr<wxBaseArrayChar>> cl(M(""), "wxBaseArrayChar", "");
		cl.def( pybind11::init( [](){ return new wxBaseArrayChar(); } ) );
		cl.def( pybind11::init( [](wxBaseArrayChar const &o){ return new wxBaseArrayChar(o); } ) );
		cl.def("assign", (class wxBaseArrayChar & (wxBaseArrayChar::*)(const class wxBaseArrayChar &)) &wxBaseArrayChar::operator=, "C++: wxBaseArrayChar::operator=(const class wxBaseArrayChar &) --> class wxBaseArrayChar &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("Empty", (void (wxBaseArrayChar::*)()) &wxBaseArrayChar::Empty, "C++: wxBaseArrayChar::Empty() --> void");
		cl.def("Clear", (void (wxBaseArrayChar::*)()) &wxBaseArrayChar::Clear, "C++: wxBaseArrayChar::Clear() --> void");
		cl.def("Alloc", (void (wxBaseArrayChar::*)(unsigned long)) &wxBaseArrayChar::Alloc, "C++: wxBaseArrayChar::Alloc(unsigned long) --> void", pybind11::arg("n"));
		cl.def("Shrink", (void (wxBaseArrayChar::*)()) &wxBaseArrayChar::Shrink, "C++: wxBaseArrayChar::Shrink() --> void");
		cl.def("GetCount", (unsigned long (wxBaseArrayChar::*)() const) &wxBaseArrayChar::GetCount, "C++: wxBaseArrayChar::GetCount() const --> unsigned long");
		cl.def("SetCount", [](wxBaseArrayChar &o, unsigned long const & a0) -> void { return o.SetCount(a0); }, "", pybind11::arg("n"));
		cl.def("SetCount", (void (wxBaseArrayChar::*)(unsigned long, char)) &wxBaseArrayChar::SetCount, "C++: wxBaseArrayChar::SetCount(unsigned long, char) --> void", pybind11::arg("n"), pybind11::arg("defval"));
		cl.def("IsEmpty", (bool (wxBaseArrayChar::*)() const) &wxBaseArrayChar::IsEmpty, "C++: wxBaseArrayChar::IsEmpty() const --> bool");
		cl.def("Count", (unsigned long (wxBaseArrayChar::*)() const) &wxBaseArrayChar::Count, "C++: wxBaseArrayChar::Count() const --> unsigned long");
		cl.def("clear", (void (wxBaseArrayChar::*)()) &wxBaseArrayChar::clear, "C++: wxBaseArrayChar::clear() --> void");
		cl.def("empty", (bool (wxBaseArrayChar::*)() const) &wxBaseArrayChar::empty, "C++: wxBaseArrayChar::empty() const --> bool");
		cl.def("max_size", (unsigned long (wxBaseArrayChar::*)() const) &wxBaseArrayChar::max_size, "C++: wxBaseArrayChar::max_size() const --> unsigned long");
		cl.def("size", (unsigned long (wxBaseArrayChar::*)() const) &wxBaseArrayChar::size, "C++: wxBaseArrayChar::size() const --> unsigned long");
	}
}
