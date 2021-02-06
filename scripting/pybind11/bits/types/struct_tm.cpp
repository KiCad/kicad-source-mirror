#include <cstdio> // _IO_FILE
#include <sstream> // __str__
#include <time.h> // asctime
#include <time.h> // asctime_r
#include <time.h> // clock
#include <time.h> // clock_getcpuclockid
#include <time.h> // clock_getres
#include <time.h> // clock_gettime
#include <time.h> // clock_nanosleep
#include <time.h> // clock_settime
#include <time.h> // ctime
#include <time.h> // ctime_r
#include <time.h> // difftime
#include <time.h> // dysize
#include <time.h> // getdate
#include <time.h> // getdate_r
#include <time.h> // gmtime
#include <time.h> // gmtime_r
#include <time.h> // itimerspec
#include <time.h> // localtime
#include <time.h> // localtime_r
#include <time.h> // mktime
#include <time.h> // nanosleep
#include <time.h> // strftime
#include <time.h> // strftime_l
#include <time.h> // strptime
#include <time.h> // strptime_l
#include <time.h> // time
#include <time.h> // timegm
#include <time.h> // timelocal
#include <time.h> // timer_delete
#include <time.h> // timer_getoverrun
#include <time.h> // timer_gettime
#include <time.h> // timer_settime
#include <time.h> // timespec
#include <time.h> // timespec_get
#include <time.h> // tm
#include <time.h> // tzset

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_bits_types_struct_tm(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // tm file:bits/types/struct_tm.h line:7
		pybind11::class_<tm, std::shared_ptr<tm>> cl(M(""), "tm", "");
		cl.def( pybind11::init( [](){ return new tm(); } ) );
		cl.def( pybind11::init( [](tm const &o){ return new tm(o); } ) );
		cl.def_readwrite("tm_sec", &tm::tm_sec);
		cl.def_readwrite("tm_min", &tm::tm_min);
		cl.def_readwrite("tm_hour", &tm::tm_hour);
		cl.def_readwrite("tm_mday", &tm::tm_mday);
		cl.def_readwrite("tm_mon", &tm::tm_mon);
		cl.def_readwrite("tm_year", &tm::tm_year);
		cl.def_readwrite("tm_wday", &tm::tm_wday);
		cl.def_readwrite("tm_yday", &tm::tm_yday);
		cl.def_readwrite("tm_isdst", &tm::tm_isdst);
		cl.def_readwrite("tm_gmtoff", &tm::tm_gmtoff);
	}
	// wxUpdateLocaleIsUtf8() file: line:124
	M("").def("wxUpdateLocaleIsUtf8", (void (*)()) &wxUpdateLocaleIsUtf8, "C++: wxUpdateLocaleIsUtf8() --> void");

	// wxCRT_PutsW(const wchar_t *) file: line:505
	M("").def("wxCRT_PutsW", (int (*)(const wchar_t *)) &wxCRT_PutsW, "C++: wxCRT_PutsW(const wchar_t *) --> int", pybind11::arg("ws"));

	// wxCRT_FputcW(wchar_t, struct _IO_FILE *) file: line:513
	M("").def("wxCRT_FputcW", (int (*)(wchar_t, struct _IO_FILE *)) &wxCRT_FputcW, "C++: wxCRT_FputcW(wchar_t, struct _IO_FILE *) --> int", pybind11::arg("wc"), pybind11::arg("stream"));

	// wxCRT_GetenvW(const wchar_t *) file: line:550
	M("").def("wxCRT_GetenvW", (wchar_t * (*)(const wchar_t *)) &wxCRT_GetenvW, "C++: wxCRT_GetenvW(const wchar_t *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("name"));

	// wxStrlen(const char *) file: line:675
	M("").def("wxStrlen", (unsigned long (*)(const char *)) &wxStrlen, "C++: wxStrlen(const char *) --> unsigned long", pybind11::arg("s"));

	// wxStrlen(const wchar_t *) file: line:676
	M("").def("wxStrlen", (unsigned long (*)(const wchar_t *)) &wxStrlen, "C++: wxStrlen(const wchar_t *) --> unsigned long", pybind11::arg("s"));

	// wxStrlen(const unsigned short *) file: line:678
	M("").def("wxStrlen", (unsigned long (*)(const unsigned short *)) &wxStrlen, "C++: wxStrlen(const unsigned short *) --> unsigned long", pybind11::arg("s"));

	// wxStrdup(const char *) file: line:687
	M("").def("wxStrdup", (char * (*)(const char *)) &wxStrdup, "C++: wxStrdup(const char *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("s"));

	// wxStrdup(const wchar_t *) file: line:688
	M("").def("wxStrdup", (wchar_t * (*)(const wchar_t *)) &wxStrdup, "C++: wxStrdup(const wchar_t *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"));

	// wxStrdup(const unsigned short *) file: line:690
	M("").def("wxStrdup", (unsigned short * (*)(const unsigned short *)) &wxStrdup, "C++: wxStrdup(const unsigned short *) --> unsigned short *", pybind11::return_value_policy::automatic, pybind11::arg("s"));

}
