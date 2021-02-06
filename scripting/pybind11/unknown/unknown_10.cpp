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

void bind_unknown_unknown_10(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxStrcpy(wchar_t *, const char *) file: line:226
	M("").def("wxStrcpy", (wchar_t * (*)(wchar_t *, const char *)) &wxStrcpy, "C++: wxStrcpy(wchar_t *, const char *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrncpy(char *, const char *, unsigned long) file: line:229
	M("").def("wxStrncpy", (char * (*)(char *, const char *, unsigned long)) &wxStrncpy, "C++: wxStrncpy(char *, const char *, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncpy(wchar_t *, const wchar_t *, unsigned long) file: line:231
	M("").def("wxStrncpy", (wchar_t * (*)(wchar_t *, const wchar_t *, unsigned long)) &wxStrncpy, "C++: wxStrncpy(wchar_t *, const wchar_t *, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncpy(char *, const class wxString &, unsigned long) file: line:233
	M("").def("wxStrncpy", (char * (*)(char *, const class wxString &, unsigned long)) &wxStrncpy, "C++: wxStrncpy(char *, const class wxString &, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncpy(char *, const class wxCStrData &, unsigned long) file: line:235
	M("").def("wxStrncpy", (char * (*)(char *, const class wxCStrData &, unsigned long)) &wxStrncpy, "C++: wxStrncpy(char *, const class wxCStrData &, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncpy(char *, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:237
	M("").def("wxStrncpy", (char * (*)(char *, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrncpy, "C++: wxStrncpy(char *, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncpy(wchar_t *, const class wxString &, unsigned long) file: line:239
	M("").def("wxStrncpy", (wchar_t * (*)(wchar_t *, const class wxString &, unsigned long)) &wxStrncpy, "C++: wxStrncpy(wchar_t *, const class wxString &, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncpy(wchar_t *, const class wxCStrData &, unsigned long) file: line:241
	M("").def("wxStrncpy", (wchar_t * (*)(wchar_t *, const class wxCStrData &, unsigned long)) &wxStrncpy, "C++: wxStrncpy(wchar_t *, const class wxCStrData &, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncpy(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:243
	M("").def("wxStrncpy", (wchar_t * (*)(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrncpy, "C++: wxStrncpy(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncpy(char *, const wchar_t *, unsigned long) file: line:245
	M("").def("wxStrncpy", (char * (*)(char *, const wchar_t *, unsigned long)) &wxStrncpy, "C++: wxStrncpy(char *, const wchar_t *, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncpy(wchar_t *, const char *, unsigned long) file: line:247
	M("").def("wxStrncpy", (wchar_t * (*)(wchar_t *, const char *, unsigned long)) &wxStrncpy, "C++: wxStrncpy(wchar_t *, const char *, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrlcpy(char *, const char *, unsigned long) file: line:252
	M("").def("wxStrlcpy", (unsigned long (*)(char *, const char *, unsigned long)) &wxStrlcpy, "C++: wxStrlcpy(char *, const char *, unsigned long) --> unsigned long", pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrlcpy(wchar_t *, const wchar_t *, unsigned long) file: line:266
	M("").def("wxStrlcpy", (unsigned long (*)(wchar_t *, const wchar_t *, unsigned long)) &wxStrlcpy, "C++: wxStrlcpy(wchar_t *, const wchar_t *, unsigned long) --> unsigned long", pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrcat(char *, const char *) file: line:280
	M("").def("wxStrcat", (char * (*)(char *, const char *)) &wxStrcat, "C++: wxStrcat(char *, const char *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcat(wchar_t *, const wchar_t *) file: line:282
	M("").def("wxStrcat", (wchar_t * (*)(wchar_t *, const wchar_t *)) &wxStrcat, "C++: wxStrcat(wchar_t *, const wchar_t *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcat(char *, const class wxString &) file: line:284
	M("").def("wxStrcat", (char * (*)(char *, const class wxString &)) &wxStrcat, "C++: wxStrcat(char *, const class wxString &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcat(char *, const class wxCStrData &) file: line:286
	M("").def("wxStrcat", (char * (*)(char *, const class wxCStrData &)) &wxStrcat, "C++: wxStrcat(char *, const class wxCStrData &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcat(char *, const class wxScopedCharTypeBuffer<char> &) file: line:288
	M("").def("wxStrcat", (char * (*)(char *, const class wxScopedCharTypeBuffer<char> &)) &wxStrcat, "C++: wxStrcat(char *, const class wxScopedCharTypeBuffer<char> &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcat(wchar_t *, const class wxString &) file: line:290
	M("").def("wxStrcat", (wchar_t * (*)(wchar_t *, const class wxString &)) &wxStrcat, "C++: wxStrcat(wchar_t *, const class wxString &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcat(wchar_t *, const class wxCStrData &) file: line:292
	M("").def("wxStrcat", (wchar_t * (*)(wchar_t *, const class wxCStrData &)) &wxStrcat, "C++: wxStrcat(wchar_t *, const class wxCStrData &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcat(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:294
	M("").def("wxStrcat", (wchar_t * (*)(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcat, "C++: wxStrcat(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcat(char *, const wchar_t *) file: line:296
	M("").def("wxStrcat", (char * (*)(char *, const wchar_t *)) &wxStrcat, "C++: wxStrcat(char *, const wchar_t *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcat(wchar_t *, const char *) file: line:298
	M("").def("wxStrcat", (wchar_t * (*)(wchar_t *, const char *)) &wxStrcat, "C++: wxStrcat(wchar_t *, const char *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrncat(char *, const char *, unsigned long) file: line:301
	M("").def("wxStrncat", (char * (*)(char *, const char *, unsigned long)) &wxStrncat, "C++: wxStrncat(char *, const char *, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncat(wchar_t *, const wchar_t *, unsigned long) file: line:303
	M("").def("wxStrncat", (wchar_t * (*)(wchar_t *, const wchar_t *, unsigned long)) &wxStrncat, "C++: wxStrncat(wchar_t *, const wchar_t *, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncat(char *, const class wxString &, unsigned long) file: line:305
	M("").def("wxStrncat", (char * (*)(char *, const class wxString &, unsigned long)) &wxStrncat, "C++: wxStrncat(char *, const class wxString &, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncat(char *, const class wxCStrData &, unsigned long) file: line:307
	M("").def("wxStrncat", (char * (*)(char *, const class wxCStrData &, unsigned long)) &wxStrncat, "C++: wxStrncat(char *, const class wxCStrData &, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncat(char *, const class wxScopedCharTypeBuffer<char> &, unsigned long) file: line:309
	M("").def("wxStrncat", (char * (*)(char *, const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxStrncat, "C++: wxStrncat(char *, const class wxScopedCharTypeBuffer<char> &, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncat(wchar_t *, const class wxString &, unsigned long) file: line:311
	M("").def("wxStrncat", (wchar_t * (*)(wchar_t *, const class wxString &, unsigned long)) &wxStrncat, "C++: wxStrncat(wchar_t *, const class wxString &, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncat(wchar_t *, const class wxCStrData &, unsigned long) file: line:313
	M("").def("wxStrncat", (wchar_t * (*)(wchar_t *, const class wxCStrData &, unsigned long)) &wxStrncat, "C++: wxStrncat(wchar_t *, const class wxCStrData &, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncat(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) file: line:315
	M("").def("wxStrncat", (wchar_t * (*)(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxStrncat, "C++: wxStrncat(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncat(char *, const wchar_t *, unsigned long) file: line:317
	M("").def("wxStrncat", (char * (*)(char *, const wchar_t *, unsigned long)) &wxStrncat, "C++: wxStrncat(char *, const wchar_t *, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrncat(wchar_t *, const char *, unsigned long) file: line:319
	M("").def("wxStrncat", (wchar_t * (*)(wchar_t *, const char *, unsigned long)) &wxStrncat, "C++: wxStrncat(wchar_t *, const char *, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"), pybind11::arg("n"));

	// wxStrcmp_String(const class wxString &, const class wxString &) file: line:452
	M("").def("wxStrcmp_String", (int (*)(const class wxString &, const class wxString &)) &wxStrcmp_String<wxString>, "C++: wxStrcmp_String(const class wxString &, const class wxString &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp_String(const class wxString &, const wchar_t *const &) file: line:452
	M("").def("wxStrcmp_String", (int (*)(const class wxString &, const wchar_t *const &)) &wxStrcmp_String<const wchar_t *>, "C++: wxStrcmp_String(const class wxString &, const wchar_t *const &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp_String(const class wxString &, const char *const &) file: line:452
	M("").def("wxStrcmp_String", (int (*)(const class wxString &, const char *const &)) &wxStrcmp_String<const char *>, "C++: wxStrcmp_String(const class wxString &, const char *const &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &) file: line:452
	M("").def("wxStrcmp_String", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcmp_String<wxScopedCharTypeBuffer<char>>, "C++: wxStrcmp_String(const class wxString &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:452
	M("").def("wxStrcmp_String", (int (*)(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcmp_String<wxScopedCharTypeBuffer<wchar_t>>, "C++: wxStrcmp_String(const class wxString &, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp_String(const class wxString &, const class wxCStrData &) file: line:452
	M("").def("wxStrcmp_String", (int (*)(const class wxString &, const class wxCStrData &)) &wxStrcmp_String<wxCStrData>, "C++: wxStrcmp_String(const class wxString &, const class wxCStrData &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const char *, const char *) file: line:454
	M("").def("wxStrcmp", (int (*)(const char *, const char *)) &wxStrcmp, "C++: wxStrcmp(const char *, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const char *, const wchar_t *) file: line:454
	M("").def("wxStrcmp", (int (*)(const char *, const wchar_t *)) &wxStrcmp, "C++: wxStrcmp(const char *, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const char *, const class wxScopedCharTypeBuffer<char> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const char *, const class wxScopedCharTypeBuffer<char> &)) &wxStrcmp, "C++: wxStrcmp(const char *, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const char *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const char *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcmp, "C++: wxStrcmp(const char *, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const wchar_t *, const wchar_t *) file: line:454
	M("").def("wxStrcmp", (int (*)(const wchar_t *, const wchar_t *)) &wxStrcmp, "C++: wxStrcmp(const wchar_t *, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const wchar_t *, const char *) file: line:454
	M("").def("wxStrcmp", (int (*)(const wchar_t *, const char *)) &wxStrcmp, "C++: wxStrcmp(const wchar_t *, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcmp, "C++: wxStrcmp(const wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const wchar_t *, const class wxScopedCharTypeBuffer<char> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const wchar_t *, const class wxScopedCharTypeBuffer<char> &)) &wxStrcmp, "C++: wxStrcmp(const wchar_t *, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const char *) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const char *)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const char *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const wchar_t *) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const wchar_t *)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const wchar_t *) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

	// wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &) file: line:454
	M("").def("wxStrcmp", (int (*)(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &)) &wxStrcmp, "C++: wxStrcmp(const class wxScopedCharTypeBuffer<char> &, const class wxScopedCharTypeBuffer<char> &) --> int", pybind11::arg("s1"), pybind11::arg("s2"));

}
