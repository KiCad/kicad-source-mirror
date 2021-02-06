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

void bind_unknown_unknown_12(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxStricmp(const class wxString &, const class wxString &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxString &, const class wxString &)) &wxStricmp, "C++: wxStricmp(const class wxString &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxString &, const class wxCStrData &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxString &, const class wxCStrData &)) &wxStricmp, "C++: wxStricmp(const class wxString &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxCStrData &, const char *) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxCStrData &, const char *)) &wxStricmp, "C++: wxStricmp(const class wxCStrData &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxCStrData &, const wchar_t *) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxCStrData &, const wchar_t *)) &wxStricmp, "C++: wxStricmp(const class wxCStrData &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &)) &wxStricmp, "C++: wxStricmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStricmp, "C++: wxStricmp(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxCStrData &, const class wxString &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxCStrData &, const class wxString &)) &wxStricmp, "C++: wxStricmp(const class wxCStrData &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxCStrData &, const class wxCStrData &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxCStrData &, const class wxCStrData &)) &wxStricmp, "C++: wxStricmp(const class wxCStrData &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const char *, const class wxCStrData &) file: line:459
	M("").def("wxStricmp", (int (*)(const char *, const class wxCStrData &)) &wxStricmp, "C++: wxStricmp(const char *, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const char *, const class wxString &) file: line:459
	M("").def("wxStricmp", (int (*)(const char *, const class wxString &)) &wxStricmp, "C++: wxStricmp(const char *, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const wchar_t *, const class wxCStrData &) file: line:459
	M("").def("wxStricmp", (int (*)(const wchar_t *, const class wxCStrData &)) &wxStricmp, "C++: wxStricmp(const wchar_t *, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const wchar_t *, const class wxString &) file: line:459
	M("").def("wxStricmp", (int (*)(const wchar_t *, const class wxString &)) &wxStricmp, "C++: wxStricmp(const wchar_t *, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<char> &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<char> &, const class wxString &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxString &)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<char> &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &) file: line:459
	M("").def("wxStricmp", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &)) &wxStricmp, "C++: wxStricmp(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll_String(const class wxString &, const class wxString &) file: line:475
	M("").def("wxStrcoll_String", (int (*)(const class wxString &, const class wxString &)) &wxStrcoll_String<wxString>, "C++: wxStrcoll_String(const class wxString &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll_String(const class wxString &, const wchar_t *const &) file: line:475
	M("").def("wxStrcoll_String", (int (*)(const class wxString &, const wchar_t *const &)) &wxStrcoll_String<const wchar_t *>, "C++: wxStrcoll_String(const class wxString &, const wchar_t *const &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll_String(const class wxString &, const char *const &) file: line:475
	M("").def("wxStrcoll_String", (int (*)(const class wxString &, const char *const &)) &wxStrcoll_String<const char *>, "C++: wxStrcoll_String(const class wxString &, const char *const &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &) file: line:475
	M("").def("wxStrcoll_String", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcoll_String<wxScopedCharTypeBuffer<char>>, "C++: wxStrcoll_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:475
	M("").def("wxStrcoll_String", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcoll_String<wxScopedCharTypeBuffer<wchar_t>>, "C++: wxStrcoll_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll_String(const class wxString &, const class wxCStrData &) file: line:475
	M("").def("wxStrcoll_String", (int (*)(const class wxString &, const class wxCStrData &)) &wxStrcoll_String<wxCStrData>, "C++: wxStrcoll_String(const class wxString &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const char *, const char *) file: line:476
	M("").def("wxStrcoll", (int (*)(const char *, const char *)) &wxStrcoll, "C++: wxStrcoll(const char *, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const char *, const wchar_t *) file: line:476
	M("").def("wxStrcoll", (int (*)(const char *, const wchar_t *)) &wxStrcoll, "C++: wxStrcoll(const char *, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const char *, const class wxScopedCharTypeBuffer<char> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const char *, const class wxScopedCharTypeBuffer<char> &)) &wxStrcoll, "C++: wxStrcoll(const char *, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const char *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const char *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcoll, "C++: wxStrcoll(const char *, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const wchar_t *, const wchar_t *) file: line:476
	M("").def("wxStrcoll", (int (*)(const wchar_t *, const wchar_t *)) &wxStrcoll, "C++: wxStrcoll(const wchar_t *, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const wchar_t *, const char *) file: line:476
	M("").def("wxStrcoll", (int (*)(const wchar_t *, const char *)) &wxStrcoll, "C++: wxStrcoll(const wchar_t *, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcoll, "C++: wxStrcoll(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const wchar_t *, const class wxScopedCharTypeBuffer<char> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const wchar_t *, const class wxScopedCharTypeBuffer<char> &)) &wxStrcoll, "C++: wxStrcoll(const wchar_t *, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const char *) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<char> &, const char *)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const wchar_t *) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<char> &, const wchar_t *)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const char *) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const char *)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcoll, "C++: wxStrcoll(const class wxScopedCharTypeBuffer<wchar_t> &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxString &, const char *) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxString &, const char *)) &wxStrcoll, "C++: wxStrcoll(const class wxString &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxString &, const wchar_t *) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxString &, const wchar_t *)) &wxStrcoll, "C++: wxStrcoll(const class wxString &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxString &, const class wxScopedCharTypeBuffer<char> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcoll, "C++: wxStrcoll(const class wxString &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcoll, "C++: wxStrcoll(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxString &, const class wxString &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxString &, const class wxString &)) &wxStrcoll, "C++: wxStrcoll(const class wxString &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxString &, const class wxCStrData &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxString &, const class wxCStrData &)) &wxStrcoll, "C++: wxStrcoll(const class wxString &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxCStrData &, const char *) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxCStrData &, const char *)) &wxStrcoll, "C++: wxStrcoll(const class wxCStrData &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxCStrData &, const wchar_t *) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxCStrData &, const wchar_t *)) &wxStrcoll, "C++: wxStrcoll(const class wxCStrData &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcoll, "C++: wxStrcoll(const class wxCStrData &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcoll, "C++: wxStrcoll(const class wxCStrData &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxCStrData &, const class wxString &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxCStrData &, const class wxString &)) &wxStrcoll, "C++: wxStrcoll(const class wxCStrData &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const class wxCStrData &, const class wxCStrData &) file: line:476
	M("").def("wxStrcoll", (int (*)(const class wxCStrData &, const class wxCStrData &)) &wxStrcoll, "C++: wxStrcoll(const class wxCStrData &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const char *, const class wxCStrData &) file: line:476
	M("").def("wxStrcoll", (int (*)(const char *, const class wxCStrData &)) &wxStrcoll, "C++: wxStrcoll(const char *, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcoll(const char *, const class wxString &) file: line:476
	M("").def("wxStrcoll", (int (*)(const char *, const class wxString &)) &wxStrcoll, "C++: wxStrcoll(const char *, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

}
