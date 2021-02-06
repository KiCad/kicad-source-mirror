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

void bind_unknown_unknown_15(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxStrncmp(const char *, const char *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const char *, const char *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const char *, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const char *, const wchar_t *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const char *, const wchar_t *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const char *, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const char *, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const char *, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const char *, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const char *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const char *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const char *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const wchar_t *, const wchar_t *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const wchar_t *, const wchar_t *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const wchar_t *, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const wchar_t *, const char *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const wchar_t *, const char *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const wchar_t *, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const wchar_t *, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const wchar_t *, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const wchar_t *, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const char *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const char *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const wchar_t *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const wchar_t *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const char *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const char *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxString &, const char *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxString &, const char *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxString &, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxString &, const wchar_t *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxString &, const wchar_t *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxString &, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxString &, const class wxString &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxString &, const class wxString &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxString &, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxString &, const class wxCStrData &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxString &, const class wxCStrData &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxString &, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxCStrData &, const char *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxCStrData &, const char *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxCStrData &, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxCStrData &, const wchar_t *, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxCStrData &, const wchar_t *, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxCStrData &, const wchar_t *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxCStrData &, const class wxString &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxCStrData &, const class wxString &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxCStrData &, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxCStrData &, const class wxCStrData &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxCStrData &, const class wxCStrData &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxCStrData &, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const char *, const class wxCStrData &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const char *, const class wxCStrData &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const char *, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const char *, const class wxString &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const char *, const class wxString &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const char *, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const wchar_t *, const class wxCStrData &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const wchar_t *, const class wxCStrData &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const wchar_t *, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const wchar_t *, const class wxString &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const wchar_t *, const class wxString &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const wchar_t *, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const class wxString &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxString &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<char> &, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &, unsigned long) file: line:525
	M("").def("wxStrncmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &, unsigned long)) &wxStrncmp, "C++: wxStrncmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp_String(const class wxString &, const class wxString &, unsigned long) file: line:528
	M("").def("wxStrnicmp_String", (int (*)(const class wxString &, const class wxString &, unsigned long)) &wxStrnicmp_String<wxString>, "C++: wxStrnicmp_String(const class wxString &, const class wxString &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp_String(const class wxString &, const wchar_t *const &, unsigned long) file: line:528
	M("").def("wxStrnicmp_String", (int (*)(const class wxString &, const wchar_t *const &, unsigned long)) &wxStrnicmp_String<const wchar_t *>, "C++: wxStrnicmp_String(const class wxString &, const wchar_t *const &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp_String(const class wxString &, const char *const &, unsigned long) file: line:528
	M("").def("wxStrnicmp_String", (int (*)(const class wxString &, const char *const &, unsigned long)) &wxStrnicmp_String<const char *>, "C++: wxStrnicmp_String(const class wxString &, const char *const &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:528
	M("").def("wxStrnicmp_String", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrnicmp_String<wxScopedCharTypeBuffer<char>>, "C++: wxStrnicmp_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:528
	M("").def("wxStrnicmp_String", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrnicmp_String<wxScopedCharTypeBuffer<wchar_t>>, "C++: wxStrnicmp_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp_String(const class wxString &, const class wxCStrData &, unsigned long) file: line:528
	M("").def("wxStrnicmp_String", (int (*)(const class wxString &, const class wxCStrData &, unsigned long)) &wxStrnicmp_String<wxCStrData>, "C++: wxStrnicmp_String(const class wxString &, const class wxCStrData &, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

	// wxStrnicmp(const char *, const char *, unsigned long) file: line:530
	M("").def("wxStrnicmp", (int (*)(const char *, const char *, unsigned long)) &wxStrnicmp, "C++: wxStrnicmp(const char *, const char *, unsigned long) --> int", pybind11::arg("s1"), pybind11::arg("s2"), pybind11::arg("n"));

}
