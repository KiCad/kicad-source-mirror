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

void bind_unknown_unknown_13(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxStrcoll(const wchar_t *, const class wxCStrData &) file: line:476
	M("").def("wxStrcoll", (int (*)(const wchar_t *, const class wxCStrData &)) &wxStrcoll, "C++: wxStrcoll(const wchar_t *, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const wchar_t *, const class wxString &) file: line:476
	M("").def("wxStrcoll", (int (*)(const wchar_t *, const class wxString &)) &wxStrcoll, "C++: wxStrcoll(const wchar_t *, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const class wxString &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxString &)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn_String(const class wxString &, const class wxString &) file: line:502
	M("").def("wxStrspn_String", (unsigned long (*)(const class wxString &, const class wxString &)) &wxStrspn_String<wxString>, "C++: wxStrspn_String(const class wxString &, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn_String(const class wxString &, const wchar_t *const &) file: line:502
	M("").def("wxStrspn_String", (unsigned long (*)(const class wxString &, const wchar_t *const &)) &wxStrspn_String<const wchar_t *>, "C++: wxStrspn_String(const class wxString &, const wchar_t *const &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn_String(const class wxString &, const char *const &) file: line:502
	M("").def("wxStrspn_String", (unsigned long (*)(const class wxString &, const char *const &)) &wxStrspn_String<const char *>, "C++: wxStrspn_String(const class wxString &, const char *const &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &) file: line:502
	M("").def("wxStrspn_String", (unsigned long (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &)) &wxStrspn_String<wxScopedCharTypeBuffer<char>>, "C++: wxStrspn_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:502
	M("").def("wxStrspn_String", (unsigned long (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrspn_String<wxScopedCharTypeBuffer<wchar_t>>, "C++: wxStrspn_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn_String(const class wxString &, const class wxCStrData &) file: line:502
	M("").def("wxStrspn_String", (unsigned long (*)(const class wxString &, const class wxCStrData &)) &wxStrspn_String<wxCStrData>, "C++: wxStrspn_String(const class wxString &, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const char *, const char *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const char *, const char *)) &wxStrspn, "C++: wxStrspn(const char *, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const char *, const wchar_t *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const char *, const wchar_t *)) &wxStrspn, "C++: wxStrspn(const char *, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const char *, const class wxScopedCharTypeBuffer<char> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const char *, const class wxScopedCharTypeBuffer<char> &)) &wxStrspn, "C++: wxStrspn(const char *, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const char *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const char *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrspn, "C++: wxStrspn(const char *, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const wchar_t *, const wchar_t *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const wchar_t *, const wchar_t *)) &wxStrspn, "C++: wxStrspn(const wchar_t *, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const wchar_t *, const char *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const wchar_t *, const char *)) &wxStrspn, "C++: wxStrspn(const wchar_t *, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrspn, "C++: wxStrspn(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const wchar_t *, const class wxScopedCharTypeBuffer<char> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const wchar_t *, const class wxScopedCharTypeBuffer<char> &)) &wxStrspn, "C++: wxStrspn(const wchar_t *, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<char> &, const char *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const char *)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<char> &, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<char> &, const wchar_t *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const wchar_t *)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<char> &, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const char *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const char *)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxString &, const char *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxString &, const char *)) &wxStrspn, "C++: wxStrspn(const class wxString &, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxString &, const wchar_t *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxString &, const wchar_t *)) &wxStrspn, "C++: wxStrspn(const class wxString &, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxString &, const class wxScopedCharTypeBuffer<char> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &)) &wxStrspn, "C++: wxStrspn(const class wxString &, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrspn, "C++: wxStrspn(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxString &, const class wxString &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxString &, const class wxString &)) &wxStrspn, "C++: wxStrspn(const class wxString &, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxString &, const class wxCStrData &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxString &, const class wxCStrData &)) &wxStrspn, "C++: wxStrspn(const class wxString &, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxCStrData &, const char *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxCStrData &, const char *)) &wxStrspn, "C++: wxStrspn(const class wxCStrData &, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxCStrData &, const wchar_t *) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxCStrData &, const wchar_t *)) &wxStrspn, "C++: wxStrspn(const class wxCStrData &, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &)) &wxStrspn, "C++: wxStrspn(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrspn, "C++: wxStrspn(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxCStrData &, const class wxString &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxCStrData &, const class wxString &)) &wxStrspn, "C++: wxStrspn(const class wxCStrData &, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxCStrData &, const class wxCStrData &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxCStrData &, const class wxCStrData &)) &wxStrspn, "C++: wxStrspn(const class wxCStrData &, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const char *, const class wxCStrData &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const char *, const class wxCStrData &)) &wxStrspn, "C++: wxStrspn(const char *, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const char *, const class wxString &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const char *, const class wxString &)) &wxStrspn, "C++: wxStrspn(const char *, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const wchar_t *, const class wxCStrData &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const wchar_t *, const class wxCStrData &)) &wxStrspn, "C++: wxStrspn(const wchar_t *, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const wchar_t *, const class wxString &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const wchar_t *, const class wxString &)) &wxStrspn, "C++: wxStrspn(const wchar_t *, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<char> &, const class wxString &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const class wxString &)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<char> &, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &) file: line:507
	M("").def("wxStrspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &)) &wxStrspn, "C++: wxStrspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn_String(const class wxString &, const class wxString &) file: line:510
	M("").def("wxStrcspn_String", (unsigned long (*)(const class wxString &, const class wxString &)) &wxStrcspn_String<wxString>, "C++: wxStrcspn_String(const class wxString &, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

}
