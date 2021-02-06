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

void bind_unknown_unknown_16(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxStrnicmp(const char *, const wchar_t *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const char *, const wchar_t *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const char *, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const char *, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const char *, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const char *, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const char *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const char *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const char *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const wchar_t *, const wchar_t *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const wchar_t *, const wchar_t *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const wchar_t *, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const wchar_t *, const char *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const wchar_t *, const char *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const wchar_t *, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const wchar_t *, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const wchar_t *, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const wchar_t *, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const char *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const char *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const wchar_t *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const wchar_t *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const char *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const char *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxString &, const char *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxString &, const char *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxString &, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxString &, const wchar_t *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxString &, const wchar_t *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxString &, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxString &, const class wxString &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxString &, const class wxString &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxString &, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxString &, const class wxCStrData &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxString &, const class wxCStrData &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxString &, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxCStrData &, const char *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxCStrData &, const char *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxCStrData &, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxCStrData &, const wchar_t *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxCStrData &, const wchar_t *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxCStrData &, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxCStrData &, const class wxString &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxCStrData &, const class wxString &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxCStrData &, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxCStrData &, const class wxCStrData &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxCStrData &, const class wxCStrData &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxCStrData &, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const char *, const class wxCStrData &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const char *, const class wxCStrData &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const char *, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const char *, const class wxString &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const char *, const class wxString &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const char *, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const wchar_t *, const class wxCStrData &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const wchar_t *, const class wxCStrData &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const wchar_t *, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const wchar_t *, const class wxString &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const wchar_t *, const class wxString &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const wchar_t *, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const class wxString &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxString &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<char> &, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrxfrm(char *, const char *, unsigned long) file: line:540
	M("").def("wxStrxfrm", (unsigned long (*)(char *, const char *, unsigned long)) &wxStrxfrm, "C++: wxStrxfrm(char *, const char *, unsigned long) --> unsigned long", pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrxfrm(wchar_t *, const wchar_t *, unsigned long) file: line:542
	M("").def("wxStrxfrm", (unsigned long (*)(wchar_t *, const wchar_t *, unsigned long)) &wxStrxfrm, "C++: wxStrxfrm(wchar_t *, const wchar_t *, unsigned long) --> unsigned long", pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrxfrm(char *, const class wxString &, unsigned long) file: line:547
	M("").def("wxStrxfrm", (unsigned long (*)(char *, const class wxString &, unsigned long)) &wxStrxfrm, "C++: wxStrxfrm(char *, const class wxString &, unsigned long) --> unsigned long", pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrxfrm(wchar_t *, const class wxString &, unsigned long) file: line:549
	M("").def("wxStrxfrm", (unsigned long (*)(wchar_t *, const class wxString &, unsigned long)) &wxStrxfrm, "C++: wxStrxfrm(wchar_t *, const class wxString &, unsigned long) --> unsigned long", pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrxfrm(char *, const class wxCStrData &, unsigned long) file: line:551
	M("").def("wxStrxfrm", (unsigned long (*)(char *, const class wxCStrData &, unsigned long)) &wxStrxfrm, "C++: wxStrxfrm(char *, const class wxCStrData &, unsigned long) --> unsigned long", pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrxfrm(wchar_t *, const class wxCStrData &, unsigned long) file: line:553
	M("").def("wxStrxfrm", (unsigned long (*)(wchar_t *, const class wxCStrData &, unsigned long)) &wxStrxfrm, "C++: wxStrxfrm(wchar_t *, const class wxCStrData &, unsigned long) --> unsigned long", pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrstr(const char *, const char *) file: line:574
	M("").def("wxStrstr", (const char * (*)(const char *, const char *)) &wxStrstr, "C++: wxStrstr(const char *, const char *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

	// wxStrstr(const wchar_t *, const wchar_t *) file: line:576
	M("").def("wxStrstr", (const wchar_t * (*)(const wchar_t *, const wchar_t *)) &wxStrstr, "C++: wxStrstr(const wchar_t *, const wchar_t *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

	// wxStrstr(const char *, const class wxString &) file: line:578
	M("").def("wxStrstr", (const char * (*)(const char *, const class wxString &)) &wxStrstr, "C++: wxStrstr(const char *, const class wxString &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("haystack"), pybind11::arg("needle"));

}
