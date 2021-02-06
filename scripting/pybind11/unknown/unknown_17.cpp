#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
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

void bind_unknown_unknown_17(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxStrstr(const wchar_t *, const class wxString &) file: line:580
	M("").def("wxStrstr", (const wchar_t * (*)(const wchar_t *, const class wxString &)) &wxStrstr, "C++: wxStrstr(const wchar_t *, const class wxString &) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

	// wxStrstr(const class wxString &, const class wxString &) file: line:584
	M("").def("wxStrstr", (const char * (*)(const class wxString &, const class wxString &)) &wxStrstr, "C++: wxStrstr(const class wxString &, const class wxString &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

	// wxStrstr(const class wxCStrData &, const class wxString &) file: line:586
	M("").def("wxStrstr", (const char * (*)(const class wxCStrData &, const class wxString &)) &wxStrstr, "C++: wxStrstr(const class wxCStrData &, const class wxString &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

	// wxStrstr(const class wxCStrData &, const class wxCStrData &) file: line:588
	M("").def("wxStrstr", (const char * (*)(const class wxCStrData &, const class wxCStrData &)) &wxStrstr, "C++: wxStrstr(const class wxCStrData &, const class wxCStrData &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

	// wxStrstr(const class wxString &, const char *) file: line:591
	M("").def("wxStrstr", (const char * (*)(const class wxString &, const char *)) &wxStrstr, "C++: wxStrstr(const class wxString &, const char *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

	// wxStrstr(const class wxCStrData &, const char *) file: line:593
	M("").def("wxStrstr", (const char * (*)(const class wxCStrData &, const char *)) &wxStrstr, "C++: wxStrstr(const class wxCStrData &, const char *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

	// wxStrstr(const class wxString &, const wchar_t *) file: line:595
	M("").def("wxStrstr", (const wchar_t * (*)(const class wxString &, const wchar_t *)) &wxStrstr, "C++: wxStrstr(const class wxString &, const wchar_t *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

	// wxStrstr(const class wxCStrData &, const wchar_t *) file: line:597
	M("").def("wxStrstr", (const wchar_t * (*)(const class wxCStrData &, const wchar_t *)) &wxStrstr, "C++: wxStrstr(const class wxCStrData &, const wchar_t *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

	// wxStrchr(const char *, char) file: line:600
	M("").def("wxStrchr", (const char * (*)(const char *, char)) &wxStrchr, "C++: wxStrchr(const char *, char) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrchr(const wchar_t *, wchar_t) file: line:602
	M("").def("wxStrchr", (const wchar_t * (*)(const wchar_t *, wchar_t)) &wxStrchr, "C++: wxStrchr(const wchar_t *, wchar_t) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrrchr(const char *, char) file: line:604
	M("").def("wxStrrchr", (const char * (*)(const char *, char)) &wxStrrchr, "C++: wxStrrchr(const char *, char) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrrchr(const wchar_t *, wchar_t) file: line:606
	M("").def("wxStrrchr", (const wchar_t * (*)(const wchar_t *, wchar_t)) &wxStrrchr, "C++: wxStrrchr(const wchar_t *, wchar_t) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrchr(const char *, const class wxUniChar &) file: line:608
	M("").def("wxStrchr", (const char * (*)(const char *, const class wxUniChar &)) &wxStrchr, "C++: wxStrchr(const char *, const class wxUniChar &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrchr(const wchar_t *, const class wxUniChar &) file: line:610
	M("").def("wxStrchr", (const wchar_t * (*)(const wchar_t *, const class wxUniChar &)) &wxStrchr, "C++: wxStrchr(const wchar_t *, const class wxUniChar &) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrrchr(const char *, const class wxUniChar &) file: line:612
	M("").def("wxStrrchr", (const char * (*)(const char *, const class wxUniChar &)) &wxStrrchr, "C++: wxStrrchr(const char *, const class wxUniChar &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrrchr(const wchar_t *, const class wxUniChar &) file: line:614
	M("").def("wxStrrchr", (const wchar_t * (*)(const wchar_t *, const class wxUniChar &)) &wxStrrchr, "C++: wxStrrchr(const wchar_t *, const class wxUniChar &) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrchr(const char *, const class wxUniCharRef &) file: line:616
	M("").def("wxStrchr", (const char * (*)(const char *, const class wxUniCharRef &)) &wxStrchr, "C++: wxStrchr(const char *, const class wxUniCharRef &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrchr(const wchar_t *, const class wxUniCharRef &) file: line:618
	M("").def("wxStrchr", (const wchar_t * (*)(const wchar_t *, const class wxUniCharRef &)) &wxStrchr, "C++: wxStrchr(const wchar_t *, const class wxUniCharRef &) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrrchr(const char *, const class wxUniCharRef &) file: line:620
	M("").def("wxStrrchr", (const char * (*)(const char *, const class wxUniCharRef &)) &wxStrrchr, "C++: wxStrrchr(const char *, const class wxUniCharRef &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrrchr(const wchar_t *, const class wxUniCharRef &) file: line:622
	M("").def("wxStrrchr", (const wchar_t * (*)(const wchar_t *, const class wxUniCharRef &)) &wxStrrchr, "C++: wxStrrchr(const wchar_t *, const class wxUniCharRef &) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrchr(const class wxString &, char) file: line:644
	M("").def("wxStrchr", (const char * (*)(const class wxString &, char)) &wxStrchr, "C++: wxStrchr(const class wxString &, char) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrrchr(const class wxString &, char) file: line:646
	M("").def("wxStrrchr", (const char * (*)(const class wxString &, char)) &wxStrrchr, "C++: wxStrrchr(const class wxString &, char) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrchr(const class wxString &, int) file: line:648
	M("").def("wxStrchr", (const char * (*)(const class wxString &, int)) &wxStrchr, "C++: wxStrchr(const class wxString &, int) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrrchr(const class wxString &, int) file: line:650
	M("").def("wxStrrchr", (const char * (*)(const class wxString &, int)) &wxStrrchr, "C++: wxStrrchr(const class wxString &, int) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrchr(const class wxString &, const class wxUniChar &) file: line:652
	M("").def("wxStrchr", (const char * (*)(const class wxString &, const class wxUniChar &)) &wxStrchr, "C++: wxStrchr(const class wxString &, const class wxUniChar &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrrchr(const class wxString &, const class wxUniChar &) file: line:654
	M("").def("wxStrrchr", (const char * (*)(const class wxString &, const class wxUniChar &)) &wxStrrchr, "C++: wxStrrchr(const class wxString &, const class wxUniChar &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrchr(const class wxString &, const class wxUniCharRef &) file: line:656
	M("").def("wxStrchr", (const char * (*)(const class wxString &, const class wxUniCharRef &)) &wxStrchr, "C++: wxStrchr(const class wxString &, const class wxUniCharRef &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrrchr(const class wxString &, const class wxUniCharRef &) file: line:658
	M("").def("wxStrrchr", (const char * (*)(const class wxString &, const class wxUniCharRef &)) &wxStrrchr, "C++: wxStrrchr(const class wxString &, const class wxUniCharRef &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrchr(const class wxString &, wchar_t) file: line:660
	M("").def("wxStrchr", (const wchar_t * (*)(const class wxString &, wchar_t)) &wxStrchr, "C++: wxStrchr(const class wxString &, wchar_t) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrrchr(const class wxString &, wchar_t) file: line:662
	M("").def("wxStrrchr", (const wchar_t * (*)(const class wxString &, wchar_t)) &wxStrrchr, "C++: wxStrrchr(const class wxString &, wchar_t) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrchr(const class wxCStrData &, char) file: line:664
	M("").def("wxStrchr", (const char * (*)(const class wxCStrData &, char)) &wxStrchr, "C++: wxStrchr(const class wxCStrData &, char) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrrchr(const class wxCStrData &, char) file: line:666
	M("").def("wxStrrchr", (const char * (*)(const class wxCStrData &, char)) &wxStrrchr, "C++: wxStrrchr(const class wxCStrData &, char) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrchr(const class wxCStrData &, int) file: line:668
	M("").def("wxStrchr", (const char * (*)(const class wxCStrData &, int)) &wxStrchr, "C++: wxStrchr(const class wxCStrData &, int) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrrchr(const class wxCStrData &, int) file: line:670
	M("").def("wxStrrchr", (const char * (*)(const class wxCStrData &, int)) &wxStrrchr, "C++: wxStrrchr(const class wxCStrData &, int) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrchr(const class wxCStrData &, const class wxUniChar &) file: line:672
	M("").def("wxStrchr", (const char * (*)(const class wxCStrData &, const class wxUniChar &)) &wxStrchr, "C++: wxStrchr(const class wxCStrData &, const class wxUniChar &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrrchr(const class wxCStrData &, const class wxUniChar &) file: line:674
	M("").def("wxStrrchr", (const char * (*)(const class wxCStrData &, const class wxUniChar &)) &wxStrrchr, "C++: wxStrrchr(const class wxCStrData &, const class wxUniChar &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrchr(const class wxCStrData &, const class wxUniCharRef &) file: line:676
	M("").def("wxStrchr", (const char * (*)(const class wxCStrData &, const class wxUniCharRef &)) &wxStrchr, "C++: wxStrchr(const class wxCStrData &, const class wxUniCharRef &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrrchr(const class wxCStrData &, const class wxUniCharRef &) file: line:678
	M("").def("wxStrrchr", (const char * (*)(const class wxCStrData &, const class wxUniCharRef &)) &wxStrrchr, "C++: wxStrrchr(const class wxCStrData &, const class wxUniCharRef &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("uc"));

	// wxStrchr(const class wxCStrData &, wchar_t) file: line:680
	M("").def("wxStrchr", (const wchar_t * (*)(const class wxCStrData &, wchar_t)) &wxStrchr, "C++: wxStrchr(const class wxCStrData &, wchar_t) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrrchr(const class wxCStrData &, wchar_t) file: line:682
	M("").def("wxStrrchr", (const wchar_t * (*)(const class wxCStrData &, wchar_t)) &wxStrrchr, "C++: wxStrrchr(const class wxCStrData &, wchar_t) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"));

	// wxStrpbrk(const char *, const char *) file: line:685
	M("").def("wxStrpbrk", (const char * (*)(const char *, const char *)) &wxStrpbrk, "C++: wxStrpbrk(const char *, const char *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const wchar_t *, const wchar_t *) file: line:687
	M("").def("wxStrpbrk", (const wchar_t * (*)(const wchar_t *, const wchar_t *)) &wxStrpbrk, "C++: wxStrpbrk(const wchar_t *, const wchar_t *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const char *, const class wxString &) file: line:689
	M("").def("wxStrpbrk", (const char * (*)(const char *, const class wxString &)) &wxStrpbrk, "C++: wxStrpbrk(const char *, const class wxString &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const char *, const class wxCStrData &) file: line:691
	M("").def("wxStrpbrk", (const char * (*)(const char *, const class wxCStrData &)) &wxStrpbrk, "C++: wxStrpbrk(const char *, const class wxCStrData &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const wchar_t *, const class wxString &) file: line:693
	M("").def("wxStrpbrk", (const wchar_t * (*)(const wchar_t *, const class wxString &)) &wxStrpbrk, "C++: wxStrpbrk(const wchar_t *, const class wxString &) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const wchar_t *, const class wxCStrData &) file: line:695
	M("").def("wxStrpbrk", (const wchar_t * (*)(const wchar_t *, const class wxCStrData &)) &wxStrpbrk, "C++: wxStrpbrk(const wchar_t *, const class wxCStrData &) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const class wxString &, const class wxString &) file: line:697
	M("").def("wxStrpbrk", (const char * (*)(const class wxString &, const class wxString &)) &wxStrpbrk, "C++: wxStrpbrk(const class wxString &, const class wxString &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const class wxString &, const char *) file: line:699
	M("").def("wxStrpbrk", (const char * (*)(const class wxString &, const char *)) &wxStrpbrk, "C++: wxStrpbrk(const class wxString &, const char *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const class wxString &, const wchar_t *) file: line:701
	M("").def("wxStrpbrk", (const wchar_t * (*)(const class wxString &, const wchar_t *)) &wxStrpbrk, "C++: wxStrpbrk(const class wxString &, const wchar_t *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const class wxString &, const class wxCStrData &) file: line:703
	M("").def("wxStrpbrk", (const char * (*)(const class wxString &, const class wxCStrData &)) &wxStrpbrk, "C++: wxStrpbrk(const class wxString &, const class wxCStrData &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const class wxCStrData &, const class wxString &) file: line:705
	M("").def("wxStrpbrk", (const char * (*)(const class wxCStrData &, const class wxString &)) &wxStrpbrk, "C++: wxStrpbrk(const class wxCStrData &, const class wxString &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

}
