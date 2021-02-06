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

void bind_unknown_unknown_11(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const char *) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const char *)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxString &, const char *) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxString &, const char *)) &wxStrcmp, "C++: wxStrcmp(const class wxString &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxString &, const wchar_t *) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxString &, const wchar_t *)) &wxStrcmp, "C++: wxStrcmp(const class wxString &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxString &, const class wxScopedCharTypeBuffer<char> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcmp, "C++: wxStrcmp(const class wxString &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcmp, "C++: wxStrcmp(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxString &, const class wxString &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxString &, const class wxString &)) &wxStrcmp, "C++: wxStrcmp(const class wxString &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxString &, const class wxCStrData &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxString &, const class wxCStrData &)) &wxStrcmp, "C++: wxStrcmp(const class wxString &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxCStrData &, const char *) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxCStrData &, const char *)) &wxStrcmp, "C++: wxStrcmp(const class wxCStrData &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxCStrData &, const wchar_t *) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxCStrData &, const wchar_t *)) &wxStrcmp, "C++: wxStrcmp(const class wxCStrData &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcmp, "C++: wxStrcmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcmp, "C++: wxStrcmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxCStrData &, const class wxString &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxCStrData &, const class wxString &)) &wxStrcmp, "C++: wxStrcmp(const class wxCStrData &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxCStrData &, const class wxCStrData &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxCStrData &, const class wxCStrData &)) &wxStrcmp, "C++: wxStrcmp(const class wxCStrData &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const char *, const class wxCStrData &) file: line:454
	M("").def("wxStrcmp", (int (*)(const char *, const class wxCStrData &)) &wxStrcmp, "C++: wxStrcmp(const char *, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const char *, const class wxString &) file: line:454
	M("").def("wxStrcmp", (int (*)(const char *, const class wxString &)) &wxStrcmp, "C++: wxStrcmp(const char *, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const wchar_t *, const class wxCStrData &) file: line:454
	M("").def("wxStrcmp", (int (*)(const wchar_t *, const class wxCStrData &)) &wxStrcmp, "C++: wxStrcmp(const wchar_t *, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const wchar_t *, const class wxString &) file: line:454
	M("").def("wxStrcmp", (int (*)(const wchar_t *, const class wxString &)) &wxStrcmp, "C++: wxStrcmp(const wchar_t *, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const class wxString &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxString &)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp_String(const class wxString &, const class wxString &) file: line:457
	M("").def("wxStricmp_String", (int (*)(const class wxString &, const class wxString &)) &wxStricmp_String<wxString>, "C++: wxStricmp_String(const class wxString &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp_String(const class wxString &, const wchar_t *const &) file: line:457
	M("").def("wxStricmp_String", (int (*)(const class wxString &, const wchar_t *const &)) &wxStricmp_String<const wchar_t *>, "C++: wxStricmp_String(const class wxString &, const wchar_t *const &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp_String(const class wxString &, const char *const &) file: line:457
	M("").def("wxStricmp_String", (int (*)(const class wxString &, const char *const &)) &wxStricmp_String<const char *>, "C++: wxStricmp_String(const class wxString &, const char *const &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &) file: line:457
	M("").def("wxStricmp_String", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &)) &wxStricmp_String<wxScopedCharTypeBuffer<char>>, "C++: wxStricmp_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:457
	M("").def("wxStricmp_String", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStricmp_String<wxScopedCharTypeBuffer<wchar_t>>, "C++: wxStricmp_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp_String(const class wxString &, const class wxCStrData &) file: line:457
	M("").def("wxStricmp_String", (int (*)(const class wxString &, const class wxCStrData &)) &wxStricmp_String<wxCStrData>, "C++: wxStricmp_String(const class wxString &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const char *, const char *) file: line:459
	M("").def("wxStricmp", (int (*)(const char *, const char *)) &wxStricmp, "C++: wxStricmp(const char *, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const char *, const wchar_t *) file: line:459
	M("").def("wxStricmp", (int (*)(const char *, const wchar_t *)) &wxStricmp, "C++: wxStricmp(const char *, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const char *, const class wxScopedCharTypeBuffer<char> &) file: line:459
	M("").def("wxStricmp", (int (*)(const char *, const class wxScopedCharTypeBuffer<char> &)) &wxStricmp, "C++: wxStricmp(const char *, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const char *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:459
	M("").def("wxStricmp", (int (*)(const char *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStricmp, "C++: wxStricmp(const char *, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const wchar_t *, const wchar_t *) file: line:459
	M("").def("wxStricmp", (int (*)(const wchar_t *, const wchar_t *)) &wxStricmp, "C++: wxStricmp(const wchar_t *, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const wchar_t *, const char *) file: line:459
	M("").def("wxStricmp", (int (*)(const wchar_t *, const char *)) &wxStricmp, "C++: wxStricmp(const wchar_t *, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:459
	M("").def("wxStricmp", (int (*)(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStricmp, "C++: wxStricmp(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const wchar_t *, const class wxScopedCharTypeBuffer<char> &) file: line:459
	M("").def("wxStricmp", (int (*)(const wchar_t *, const class wxScopedCharTypeBuffer<char> &)) &wxStricmp, "C++: wxStricmp(const wchar_t *, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<char> &, const char *) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const char *)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<char> &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<char> &, const wchar_t *) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const wchar_t *)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<char> &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const char *) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const char *)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxString &, const char *) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxString &, const char *)) &wxStricmp, "C++: wxStricmp(const class wxString &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxString &, const wchar_t *) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxString &, const wchar_t *)) &wxStricmp, "C++: wxStricmp(const class wxString &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxString &, const class wxScopedCharTypeBuffer<char> &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &)) &wxStricmp, "C++: wxStricmp(const class wxString &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStricmp, "C++: wxStricmp(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

}
