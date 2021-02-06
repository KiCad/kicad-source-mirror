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

void bind_unknown_unknown_14(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxStrcspn_String(const class wxString &, const wchar_t *const &) file: line:510
	M("").def("wxStrcspn_String", (unsigned long (*)(const class wxString &, const wchar_t *const &)) &wxStrcspn_String<const wchar_t *>, "C++: wxStrcspn_String(const class wxString &, const wchar_t *const &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn_String(const class wxString &, const char *const &) file: line:510
	M("").def("wxStrcspn_String", (unsigned long (*)(const class wxString &, const char *const &)) &wxStrcspn_String<const char *>, "C++: wxStrcspn_String(const class wxString &, const char *const &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &) file: line:510
	M("").def("wxStrcspn_String", (unsigned long (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcspn_String<wxScopedCharTypeBuffer<char>>, "C++: wxStrcspn_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:510
	M("").def("wxStrcspn_String", (unsigned long (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcspn_String<wxScopedCharTypeBuffer<wchar_t>>, "C++: wxStrcspn_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn_String(const class wxString &, const class wxCStrData &) file: line:510
	M("").def("wxStrcspn_String", (unsigned long (*)(const class wxString &, const class wxCStrData &)) &wxStrcspn_String<wxCStrData>, "C++: wxStrcspn_String(const class wxString &, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const char *, const char *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const char *, const char *)) &wxStrcspn, "C++: wxStrcspn(const char *, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const char *, const wchar_t *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const char *, const wchar_t *)) &wxStrcspn, "C++: wxStrcspn(const char *, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const char *, const class wxScopedCharTypeBuffer<char> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const char *, const class wxScopedCharTypeBuffer<char> &)) &wxStrcspn, "C++: wxStrcspn(const char *, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const char *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const char *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcspn, "C++: wxStrcspn(const char *, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const wchar_t *, const wchar_t *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const wchar_t *, const wchar_t *)) &wxStrcspn, "C++: wxStrcspn(const wchar_t *, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const wchar_t *, const char *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const wchar_t *, const char *)) &wxStrcspn, "C++: wxStrcspn(const wchar_t *, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcspn, "C++: wxStrcspn(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const wchar_t *, const class wxScopedCharTypeBuffer<char> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const wchar_t *, const class wxScopedCharTypeBuffer<char> &)) &wxStrcspn, "C++: wxStrcspn(const wchar_t *, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const char *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const char *)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const wchar_t *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const wchar_t *)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const char *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const char *)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxString &, const char *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxString &, const char *)) &wxStrcspn, "C++: wxStrcspn(const class wxString &, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxString &, const wchar_t *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxString &, const wchar_t *)) &wxStrcspn, "C++: wxStrcspn(const class wxString &, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxString &, const class wxScopedCharTypeBuffer<char> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcspn, "C++: wxStrcspn(const class wxString &, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcspn, "C++: wxStrcspn(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxString &, const class wxString &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxString &, const class wxString &)) &wxStrcspn, "C++: wxStrcspn(const class wxString &, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxString &, const class wxCStrData &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxString &, const class wxCStrData &)) &wxStrcspn, "C++: wxStrcspn(const class wxString &, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxCStrData &, const char *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxCStrData &, const char *)) &wxStrcspn, "C++: wxStrcspn(const class wxCStrData &, const char *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxCStrData &, const wchar_t *) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxCStrData &, const wchar_t *)) &wxStrcspn, "C++: wxStrcspn(const class wxCStrData &, const wchar_t *) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcspn, "C++: wxStrcspn(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcspn, "C++: wxStrcspn(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxCStrData &, const class wxString &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxCStrData &, const class wxString &)) &wxStrcspn, "C++: wxStrcspn(const class wxCStrData &, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxCStrData &, const class wxCStrData &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxCStrData &, const class wxCStrData &)) &wxStrcspn, "C++: wxStrcspn(const class wxCStrData &, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const char *, const class wxCStrData &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const char *, const class wxCStrData &)) &wxStrcspn, "C++: wxStrcspn(const char *, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const char *, const class wxString &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const char *, const class wxString &)) &wxStrcspn, "C++: wxStrcspn(const char *, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const wchar_t *, const class wxCStrData &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const wchar_t *, const class wxCStrData &)) &wxStrcspn, "C++: wxStrcspn(const wchar_t *, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const wchar_t *, const class wxString &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const wchar_t *, const class wxString &)) &wxStrcspn, "C++: wxStrcspn(const wchar_t *, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const class wxString &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &, const class wxString &)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<char> &, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &) file: line:515
	M("").def("wxStrcspn", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &)) &wxStrcspn, "C++: wxStrcspn(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &) --> unsigned long", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrncmp_String(const class wxString &, const class wxString &, unsigned long) file: line:523
	M("").def("wxStrncmp_String", (int (*)(const class wxString &, const class wxString &, unsigned long)) &wxStrncmp_String<wxString>, "C++: wxStrncmp_String(const class wxString &, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp_String(const class wxString &, const wchar_t *const &, unsigned long) file: line:523
	M("").def("wxStrncmp_String", (int (*)(const class wxString &, const wchar_t *const &, unsigned long)) &wxStrncmp_String<const wchar_t *>, "C++: wxStrncmp_String(const class wxString &, const wchar_t *const &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp_String(const class wxString &, const char *const &, unsigned long) file: line:523
	M("").def("wxStrncmp_String", (int (*)(const class wxString &, const char *const &, unsigned long)) &wxStrncmp_String<const char *>, "C++: wxStrncmp_String(const class wxString &, const char *const &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:523
	M("").def("wxStrncmp_String", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrncmp_String<wxScopedCharTypeBuffer<char>>, "C++: wxStrncmp_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:523
	M("").def("wxStrncmp_String", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrncmp_String<wxScopedCharTypeBuffer<wchar_t>>, "C++: wxStrncmp_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp_String(const class wxString &, const class wxCStrData &, unsigned long) file: line:523
	M("").def("wxStrncmp_String", (int (*)(const class wxString &, const class wxCStrData &, unsigned long)) &wxStrncmp_String<wxCStrData>, "C++: wxStrncmp_String(const class wxString &, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

}
