#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
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

void bind_unknown_unknown_9(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxStringTypeBufferBase file: line:3778
		pybind11::class_<wxStringTypeBufferBase<wchar_t>, std::shared_ptr<wxStringTypeBufferBase<wchar_t>>> cl(M(""), "wxStringTypeBufferBase_wchar_t_t", "");
		cl.def( pybind11::init( [](class wxString & a0){ return new wxStringTypeBufferBase<wchar_t>(a0); } ), "doc" , pybind11::arg("str"));
		cl.def( pybind11::init<class wxString &, unsigned long>(), pybind11::arg("str"), pybind11::arg("lenWanted") );

		cl.def( pybind11::init( [](wxStringTypeBufferBase<wchar_t> const &o){ return new wxStringTypeBufferBase<wchar_t>(o); } ) );
	}
	{ // wxStringTypeBufferBase file: line:3778
		pybind11::class_<wxStringTypeBufferBase<char>, std::shared_ptr<wxStringTypeBufferBase<char>>> cl(M(""), "wxStringTypeBufferBase_char_t", "");
		cl.def( pybind11::init( [](class wxString & a0){ return new wxStringTypeBufferBase<char>(a0); } ), "doc" , pybind11::arg("str"));
		cl.def( pybind11::init<class wxString &, unsigned long>(), pybind11::arg("str"), pybind11::arg("lenWanted") );

		cl.def( pybind11::init( [](wxStringTypeBufferBase<char> const &o){ return new wxStringTypeBufferBase<char>(o); } ) );
	}
	{ // wxStringTypeBufferLengthBase file: line:3818
		pybind11::class_<wxStringTypeBufferLengthBase<wchar_t>, std::shared_ptr<wxStringTypeBufferLengthBase<wchar_t>>, wxStringTypeBufferBase<wchar_t>> cl(M(""), "wxStringTypeBufferLengthBase_wchar_t_t", "");
		cl.def( pybind11::init( [](class wxString & a0){ return new wxStringTypeBufferLengthBase<wchar_t>(a0); } ), "doc" , pybind11::arg("str"));
		cl.def( pybind11::init<class wxString &, unsigned long>(), pybind11::arg("str"), pybind11::arg("lenWanted") );

		cl.def( pybind11::init( [](wxStringTypeBufferLengthBase<wchar_t> const &o){ return new wxStringTypeBufferLengthBase<wchar_t>(o); } ) );
		cl.def("SetLength", (void (wxStringTypeBufferLengthBase<wchar_t>::*)(unsigned long)) &wxStringTypeBufferLengthBase<wchar_t>::SetLength, "C++: wxStringTypeBufferLengthBase<wchar_t>::SetLength(unsigned long) --> void", pybind11::arg("length"));
	}
	{ // wxStringTypeBufferLengthBase file: line:3818
		pybind11::class_<wxStringTypeBufferLengthBase<char>, std::shared_ptr<wxStringTypeBufferLengthBase<char>>, wxStringTypeBufferBase<char>> cl(M(""), "wxStringTypeBufferLengthBase_char_t", "");
		cl.def( pybind11::init( [](class wxString & a0){ return new wxStringTypeBufferLengthBase<char>(a0); } ), "doc" , pybind11::arg("str"));
		cl.def( pybind11::init<class wxString &, unsigned long>(), pybind11::arg("str"), pybind11::arg("lenWanted") );

		cl.def( pybind11::init( [](wxStringTypeBufferLengthBase<char> const &o){ return new wxStringTypeBufferLengthBase<char>(o); } ) );
		cl.def("SetLength", (void (wxStringTypeBufferLengthBase<char>::*)(unsigned long)) &wxStringTypeBufferLengthBase<char>::SetLength, "C++: wxStringTypeBufferLengthBase<char>::SetLength(unsigned long) --> void", pybind11::arg("length"));
	}
	{ // wxStringInternalBuffer file: line:3875
		pybind11::class_<wxStringInternalBuffer, std::shared_ptr<wxStringInternalBuffer>, wxStringTypeBufferBase<wchar_t>> cl(M(""), "wxStringInternalBuffer", "");
		cl.def( pybind11::init( [](class wxString & a0){ return new wxStringInternalBuffer(a0); } ), "doc" , pybind11::arg("str"));
		cl.def( pybind11::init<class wxString &, unsigned long>(), pybind11::arg("str"), pybind11::arg("lenWanted") );

	}
	{ // wxStringInternalBufferLength file: line:3889
		pybind11::class_<wxStringInternalBufferLength, std::shared_ptr<wxStringInternalBufferLength>, wxStringTypeBufferLengthBase<wchar_t>> cl(M(""), "wxStringInternalBufferLength", "");
		cl.def( pybind11::init( [](class wxString & a0){ return new wxStringInternalBufferLength(a0); } ), "doc" , pybind11::arg("str"));
		cl.def( pybind11::init<class wxString &, unsigned long>(), pybind11::arg("str"), pybind11::arg("lenWanted") );

	}
	{ // wxUTF8StringBuffer file: line:3936
		pybind11::class_<wxUTF8StringBuffer, std::shared_ptr<wxUTF8StringBuffer>, wxStringTypeBufferBase<char>> cl(M(""), "wxUTF8StringBuffer", "");
		cl.def( pybind11::init( [](class wxString & a0){ return new wxUTF8StringBuffer(a0); } ), "doc" , pybind11::arg("str"));
		cl.def( pybind11::init<class wxString &, unsigned long>(), pybind11::arg("str"), pybind11::arg("lenWanted") );

	}
	{ // wxUTF8StringBufferLength file: line:3956
		pybind11::class_<wxUTF8StringBufferLength, std::shared_ptr<wxUTF8StringBufferLength>, wxStringTypeBufferLengthBase<char>> cl(M(""), "wxUTF8StringBufferLength", "");
		cl.def( pybind11::init( [](class wxString & a0){ return new wxUTF8StringBufferLength(a0); } ), "doc" , pybind11::arg("str"));
		cl.def( pybind11::init<class wxString &, unsigned long>(), pybind11::arg("str"), pybind11::arg("lenWanted") );

	}
	// wxIsEmpty(const char *) file: line:30
	M("").def("wxIsEmpty", (bool (*)(const char *)) &wxIsEmpty, "C++: wxIsEmpty(const char *) --> bool", pybind11::arg("s"));

	// wxIsEmpty(const wchar_t *) file: line:31
	M("").def("wxIsEmpty", (bool (*)(const wchar_t *)) &wxIsEmpty, "C++: wxIsEmpty(const wchar_t *) --> bool", pybind11::arg("s"));

	// wxIsEmpty(const class wxScopedCharTypeBuffer<char> &) file: line:32
	M("").def("wxIsEmpty", (bool (*)(const class wxScopedCharTypeBuffer<char> &)) &wxIsEmpty, "C++: wxIsEmpty(const class wxScopedCharTypeBuffer<char> &) --> bool", pybind11::arg("s"));

	// wxIsEmpty(const class wxScopedCharTypeBuffer<wchar_t> &) file: line:33
	M("").def("wxIsEmpty", (bool (*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxIsEmpty, "C++: wxIsEmpty(const class wxScopedCharTypeBuffer<wchar_t> &) --> bool", pybind11::arg("s"));

	// wxIsEmpty(const class wxString &) file: line:34
	M("").def("wxIsEmpty", (bool (*)(const class wxString &)) &wxIsEmpty, "C++: wxIsEmpty(const class wxString &) --> bool", pybind11::arg("s"));

	// wxIsEmpty(const class wxCStrData &) file: line:35
	M("").def("wxIsEmpty", (bool (*)(const class wxCStrData &)) &wxIsEmpty, "C++: wxIsEmpty(const class wxCStrData &) --> bool", pybind11::arg("s"));

	// wxMB2WC(wchar_t *, const char *, unsigned long) file: line:42
	M("").def("wxMB2WC", (unsigned long (*)(wchar_t *, const char *, unsigned long)) &wxMB2WC, "C++: wxMB2WC(wchar_t *, const char *, unsigned long) --> unsigned long", pybind11::arg("buf"), pybind11::arg("psz"), pybind11::arg("n"));

	// wxWC2MB(char *, const wchar_t *, unsigned long) file: line:43
	M("").def("wxWC2MB", (unsigned long (*)(char *, const wchar_t *, unsigned long)) &wxWC2MB, "C++: wxWC2MB(char *, const wchar_t *, unsigned long) --> unsigned long", pybind11::arg("buf"), pybind11::arg("psz"), pybind11::arg("n"));

	// wxTmemchr(const wchar_t *, wchar_t, unsigned long) file: line:69
	M("").def("wxTmemchr", (wchar_t * (*)(const wchar_t *, wchar_t, unsigned long)) &wxTmemchr, "C++: wxTmemchr(const wchar_t *, wchar_t, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("l"));

	// wxTmemcmp(const wchar_t *, const wchar_t *, unsigned long) file: line:78
	M("").def("wxTmemcmp", (int (*)(const wchar_t *, const wchar_t *, unsigned long)) &wxTmemcmp, "C++: wxTmemcmp(const wchar_t *, const wchar_t *, unsigned long) --> int", pybind11::arg("sz1"), pybind11::arg("sz2"), pybind11::arg("len"));

	// wxTmemcpy(wchar_t *, const wchar_t *, unsigned long) file: line:88
	M("").def("wxTmemcpy", (wchar_t * (*)(wchar_t *, const wchar_t *, unsigned long)) &wxTmemcpy, "C++: wxTmemcpy(wchar_t *, const wchar_t *, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("szOut"), pybind11::arg("szIn"), pybind11::arg("len"));

	// wxTmemmove(wchar_t *, const wchar_t *, unsigned long) file: line:93
	M("").def("wxTmemmove", (wchar_t * (*)(wchar_t *, const wchar_t *, unsigned long)) &wxTmemmove, "C++: wxTmemmove(wchar_t *, const wchar_t *, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("szOut"), pybind11::arg("szIn"), pybind11::arg("len"));

	// wxTmemset(wchar_t *, const wchar_t, unsigned long) file: line:98
	M("").def("wxTmemset", (wchar_t * (*)(wchar_t *, const wchar_t, unsigned long)) &wxTmemset, "C++: wxTmemset(wchar_t *, const wchar_t, unsigned long) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("szOut"), pybind11::arg("cIn"), pybind11::arg("len"));

	// wxTmemchr(const char *, char, unsigned long) file: line:112
	M("").def("wxTmemchr", (char * (*)(const char *, char, unsigned long)) &wxTmemchr, "C++: wxTmemchr(const char *, char, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("len"));

	// wxTmemcmp(const char *, const char *, unsigned long) file: line:114
	M("").def("wxTmemcmp", (int (*)(const char *, const char *, unsigned long)) &wxTmemcmp, "C++: wxTmemcmp(const char *, const char *, unsigned long) --> int", pybind11::arg("sz1"), pybind11::arg("sz2"), pybind11::arg("len"));

	// wxTmemcpy(char *, const char *, unsigned long) file: line:116
	M("").def("wxTmemcpy", (char * (*)(char *, const char *, unsigned long)) &wxTmemcpy, "C++: wxTmemcpy(char *, const char *, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("szOut"), pybind11::arg("szIn"), pybind11::arg("len"));

	// wxTmemmove(char *, const char *, unsigned long) file: line:118
	M("").def("wxTmemmove", (char * (*)(char *, const char *, unsigned long)) &wxTmemmove, "C++: wxTmemmove(char *, const char *, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("szOut"), pybind11::arg("szIn"), pybind11::arg("len"));

	// wxTmemset(char *, const char, unsigned long) file: line:120
	M("").def("wxTmemset", (char * (*)(char *, const char, unsigned long)) &wxTmemset, "C++: wxTmemset(char *, const char, unsigned long) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("szOut"), pybind11::arg("cIn"), pybind11::arg("len"));

	// wxSetlocale(int, const char *) file: line:149
	M("").def("wxSetlocale", (char * (*)(int, const char *)) &wxSetlocale, "C++: wxSetlocale(int, const char *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("category"), pybind11::arg("locale"));

	// wxSetlocale(int, const class wxScopedCharTypeBuffer<char> &) file: line:150
	M("").def("wxSetlocale", (char * (*)(int, const class wxScopedCharTypeBuffer<char> &)) &wxSetlocale, "C++: wxSetlocale(int, const class wxScopedCharTypeBuffer<char> &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("category"), pybind11::arg("locale"));

	// wxSetlocale(int, const class wxString &) file: line:152
	M("").def("wxSetlocale", (char * (*)(int, const class wxString &)) &wxSetlocale, "C++: wxSetlocale(int, const class wxString &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("category"), pybind11::arg("locale"));

	// wxSetlocale(int, const class wxCStrData &) file: line:154
	M("").def("wxSetlocale", (char * (*)(int, const class wxCStrData &)) &wxSetlocale, "C++: wxSetlocale(int, const class wxCStrData &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("category"), pybind11::arg("locale"));

	// wxStrlen(const class wxScopedCharTypeBuffer<char> &) file: line:165
	M("").def("wxStrlen", (unsigned long (*)(const class wxScopedCharTypeBuffer<char> &)) &wxStrlen, "C++: wxStrlen(const class wxScopedCharTypeBuffer<char> &) --> unsigned long", pybind11::arg("s"));

	// wxStrlen(const class wxScopedCharTypeBuffer<wchar_t> &) file: line:166
	M("").def("wxStrlen", (unsigned long (*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrlen, "C++: wxStrlen(const class wxScopedCharTypeBuffer<wchar_t> &) --> unsigned long", pybind11::arg("s"));

	// wxStrlen(const class wxString &) file: line:167
	M("").def("wxStrlen", (unsigned long (*)(const class wxString &)) &wxStrlen, "C++: wxStrlen(const class wxString &) --> unsigned long", pybind11::arg("s"));

	// wxStrlen(const class wxCStrData &) file: line:168
	M("").def("wxStrlen", (unsigned long (*)(const class wxCStrData &)) &wxStrlen, "C++: wxStrlen(const class wxCStrData &) --> unsigned long", pybind11::arg("s"));

	// wxStrnlen(const char *, unsigned long) file: line:173
	M("").def("wxStrnlen", (unsigned long (*)(const char *, unsigned long)) &wxStrnlen, "C++: wxStrnlen(const char *, unsigned long) --> unsigned long", pybind11::arg("str"), pybind11::arg("maxlen"));

	// wxStrnlen(const wchar_t *, unsigned long) file: line:187
	M("").def("wxStrnlen", (unsigned long (*)(const wchar_t *, unsigned long)) &wxStrnlen, "C++: wxStrnlen(const wchar_t *, unsigned long) --> unsigned long", pybind11::arg("str"), pybind11::arg("maxlen"));

	// wxStrdup(const class wxScopedCharTypeBuffer<char> &) file: line:203
	M("").def("wxStrdup", (char * (*)(const class wxScopedCharTypeBuffer<char> &)) &wxStrdup, "C++: wxStrdup(const class wxScopedCharTypeBuffer<char> &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("s"));

	// wxStrdup(const class wxScopedCharTypeBuffer<wchar_t> &) file: line:204
	M("").def("wxStrdup", (wchar_t * (*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrdup, "C++: wxStrdup(const class wxScopedCharTypeBuffer<wchar_t> &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"));

	// wxStrdup(const class wxString &) file: line:205
	M("").def("wxStrdup", (char * (*)(const class wxString &)) &wxStrdup, "C++: wxStrdup(const class wxString &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("s"));

	// wxStrdup(const class wxCStrData &) file: line:206
	M("").def("wxStrdup", (char * (*)(const class wxCStrData &)) &wxStrdup, "C++: wxStrdup(const class wxCStrData &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("s"));

	// wxStrcpy(char *, const char *) file: line:208
	M("").def("wxStrcpy", (char * (*)(char *, const char *)) &wxStrcpy, "C++: wxStrcpy(char *, const char *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcpy(wchar_t *, const wchar_t *) file: line:210
	M("").def("wxStrcpy", (wchar_t * (*)(wchar_t *, const wchar_t *)) &wxStrcpy, "C++: wxStrcpy(wchar_t *, const wchar_t *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcpy(char *, const class wxString &) file: line:212
	M("").def("wxStrcpy", (char * (*)(char *, const class wxString &)) &wxStrcpy, "C++: wxStrcpy(char *, const class wxString &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcpy(char *, const class wxCStrData &) file: line:214
	M("").def("wxStrcpy", (char * (*)(char *, const class wxCStrData &)) &wxStrcpy, "C++: wxStrcpy(char *, const class wxCStrData &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcpy(char *, const class wxScopedCharTypeBuffer<char> &) file: line:216
	M("").def("wxStrcpy", (char * (*)(char *, const class wxScopedCharTypeBuffer<char> &)) &wxStrcpy, "C++: wxStrcpy(char *, const class wxScopedCharTypeBuffer<char> &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcpy(wchar_t *, const class wxString &) file: line:218
	M("").def("wxStrcpy", (wchar_t * (*)(wchar_t *, const class wxString &)) &wxStrcpy, "C++: wxStrcpy(wchar_t *, const class wxString &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcpy(wchar_t *, const class wxCStrData &) file: line:220
	M("").def("wxStrcpy", (wchar_t * (*)(wchar_t *, const class wxCStrData &)) &wxStrcpy, "C++: wxStrcpy(wchar_t *, const class wxCStrData &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcpy(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) file: line:222
	M("").def("wxStrcpy", (wchar_t * (*)(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &)) &wxStrcpy, "C++: wxStrcpy(wchar_t *, const class wxScopedCharTypeBuffer<wchar_t> &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

	// wxStrcpy(char *, const wchar_t *) file: line:224
	M("").def("wxStrcpy", (char * (*)(char *, const wchar_t *)) &wxStrcpy, "C++: wxStrcpy(char *, const wchar_t *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("src"));

}
