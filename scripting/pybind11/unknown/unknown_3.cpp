#include <sstream> // __str__

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_unknown_unknown_3(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxScopedCharTypeBuffer file: line:66
		pybind11::class_<wxScopedCharTypeBuffer<char>, std::shared_ptr<wxScopedCharTypeBuffer<char>>> cl(M(""), "wxScopedCharTypeBuffer_char_t", "");
		cl.def( pybind11::init( [](){ return new wxScopedCharTypeBuffer<char>(); } ) );
		cl.def( pybind11::init( [](wxScopedCharTypeBuffer<char> const &o){ return new wxScopedCharTypeBuffer<char>(o); } ) );
		cl.def_static("CreateNonOwned", [](const char * a0) -> const wxScopedCharTypeBuffer<char> { return wxScopedCharTypeBuffer<char>::CreateNonOwned(a0); }, "", pybind11::arg("str"));
		cl.def_static("CreateNonOwned", (const class wxScopedCharTypeBuffer<char> (*)(const char *, unsigned long)) &wxScopedCharTypeBuffer<char>::CreateNonOwned, "C++: wxScopedCharTypeBuffer<char>::CreateNonOwned(const char *, unsigned long) --> const class wxScopedCharTypeBuffer<char>", pybind11::arg("str"), pybind11::arg("len"));
		cl.def_static("CreateOwned", [](char * a0) -> const wxScopedCharTypeBuffer<char> { return wxScopedCharTypeBuffer<char>::CreateOwned(a0); }, "", pybind11::arg("str"));
		cl.def_static("CreateOwned", (const class wxScopedCharTypeBuffer<char> (*)(char *, unsigned long)) &wxScopedCharTypeBuffer<char>::CreateOwned, "C++: wxScopedCharTypeBuffer<char>::CreateOwned(char *, unsigned long) --> const class wxScopedCharTypeBuffer<char>", pybind11::arg("str"), pybind11::arg("len"));
		cl.def("assign", (class wxScopedCharTypeBuffer<char> & (wxScopedCharTypeBuffer<char>::*)(const class wxScopedCharTypeBuffer<char> &)) &wxScopedCharTypeBuffer<char>::operator=, "C++: wxScopedCharTypeBuffer<char>::operator=(const class wxScopedCharTypeBuffer<char> &) --> class wxScopedCharTypeBuffer<char> &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("release", (char * (wxScopedCharTypeBuffer<char>::*)() const) &wxScopedCharTypeBuffer<char>::release, "C++: wxScopedCharTypeBuffer<char>::release() const --> char *", pybind11::return_value_policy::automatic);
		cl.def("reset", (void (wxScopedCharTypeBuffer<char>::*)()) &wxScopedCharTypeBuffer<char>::reset, "C++: wxScopedCharTypeBuffer<char>::reset() --> void");
		cl.def("data", (char * (wxScopedCharTypeBuffer<char>::*)()) &wxScopedCharTypeBuffer<char>::data, "C++: wxScopedCharTypeBuffer<char>::data() --> char *", pybind11::return_value_policy::automatic);
		cl.def("__getitem__", (char (wxScopedCharTypeBuffer<char>::*)(unsigned long) const) &wxScopedCharTypeBuffer<char>::operator[], "C++: wxScopedCharTypeBuffer<char>::operator[](unsigned long) const --> char", pybind11::arg("n"));
		cl.def("length", (unsigned long (wxScopedCharTypeBuffer<char>::*)() const) &wxScopedCharTypeBuffer<char>::length, "C++: wxScopedCharTypeBuffer<char>::length() const --> unsigned long");
	}
	{ // wxScopedCharTypeBuffer file: line:66
		pybind11::class_<wxScopedCharTypeBuffer<wchar_t>, std::shared_ptr<wxScopedCharTypeBuffer<wchar_t>>> cl(M(""), "wxScopedCharTypeBuffer_wchar_t_t", "");
		cl.def( pybind11::init( [](){ return new wxScopedCharTypeBuffer<wchar_t>(); } ) );
		cl.def( pybind11::init( [](wxScopedCharTypeBuffer<wchar_t> const &o){ return new wxScopedCharTypeBuffer<wchar_t>(o); } ) );
		cl.def_static("CreateNonOwned", [](const wchar_t * a0) -> const wxScopedCharTypeBuffer<wchar_t> { return wxScopedCharTypeBuffer<wchar_t>::CreateNonOwned(a0); }, "", pybind11::arg("str"));
		cl.def_static("CreateNonOwned", (const class wxScopedCharTypeBuffer<wchar_t> (*)(const wchar_t *, unsigned long)) &wxScopedCharTypeBuffer<wchar_t>::CreateNonOwned, "C++: wxScopedCharTypeBuffer<wchar_t>::CreateNonOwned(const wchar_t *, unsigned long) --> const class wxScopedCharTypeBuffer<wchar_t>", pybind11::arg("str"), pybind11::arg("len"));
		cl.def_static("CreateOwned", [](wchar_t * a0) -> const wxScopedCharTypeBuffer<wchar_t> { return wxScopedCharTypeBuffer<wchar_t>::CreateOwned(a0); }, "", pybind11::arg("str"));
		cl.def_static("CreateOwned", (const class wxScopedCharTypeBuffer<wchar_t> (*)(wchar_t *, unsigned long)) &wxScopedCharTypeBuffer<wchar_t>::CreateOwned, "C++: wxScopedCharTypeBuffer<wchar_t>::CreateOwned(wchar_t *, unsigned long) --> const class wxScopedCharTypeBuffer<wchar_t>", pybind11::arg("str"), pybind11::arg("len"));
		cl.def("assign", (class wxScopedCharTypeBuffer<wchar_t> & (wxScopedCharTypeBuffer<wchar_t>::*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxScopedCharTypeBuffer<wchar_t>::operator=, "C++: wxScopedCharTypeBuffer<wchar_t>::operator=(const class wxScopedCharTypeBuffer<wchar_t> &) --> class wxScopedCharTypeBuffer<wchar_t> &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("release", (wchar_t * (wxScopedCharTypeBuffer<wchar_t>::*)() const) &wxScopedCharTypeBuffer<wchar_t>::release, "C++: wxScopedCharTypeBuffer<wchar_t>::release() const --> wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("reset", (void (wxScopedCharTypeBuffer<wchar_t>::*)()) &wxScopedCharTypeBuffer<wchar_t>::reset, "C++: wxScopedCharTypeBuffer<wchar_t>::reset() --> void");
		cl.def("data", (wchar_t * (wxScopedCharTypeBuffer<wchar_t>::*)()) &wxScopedCharTypeBuffer<wchar_t>::data, "C++: wxScopedCharTypeBuffer<wchar_t>::data() --> wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("__getitem__", (wchar_t (wxScopedCharTypeBuffer<wchar_t>::*)(unsigned long) const) &wxScopedCharTypeBuffer<wchar_t>::operator[], "C++: wxScopedCharTypeBuffer<wchar_t>::operator[](unsigned long) const --> wchar_t", pybind11::arg("n"));
		cl.def("length", (unsigned long (wxScopedCharTypeBuffer<wchar_t>::*)() const) &wxScopedCharTypeBuffer<wchar_t>::length, "C++: wxScopedCharTypeBuffer<wchar_t>::length() const --> unsigned long");
	}
	{ // wxCharTypeBuffer file: line:247
		pybind11::class_<wxCharTypeBuffer<char>, std::shared_ptr<wxCharTypeBuffer<char>>, wxScopedCharTypeBuffer<char>> cl(M(""), "wxCharTypeBuffer_char_t", "");
		cl.def( pybind11::init( [](){ return new wxCharTypeBuffer<char>(); } ), "doc" );
		cl.def( pybind11::init( [](const char * a0){ return new wxCharTypeBuffer<char>(a0); } ), "doc" , pybind11::arg("str"));
		cl.def( pybind11::init<const char *, unsigned long>(), pybind11::arg("str"), pybind11::arg("len") );

		cl.def( pybind11::init<unsigned long>(), pybind11::arg("len") );

		cl.def( pybind11::init( [](wxCharTypeBuffer<char> const &o){ return new wxCharTypeBuffer<char>(o); } ) );
		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<char> &>(), pybind11::arg("src") );

		cl.def("assign", (class wxCharTypeBuffer<char> & (wxCharTypeBuffer<char>::*)(const char *)) &wxCharTypeBuffer<char>::operator=, "C++: wxCharTypeBuffer<char>::operator=(const char *) --> class wxCharTypeBuffer<char> &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("assign", (class wxCharTypeBuffer<char> & (wxCharTypeBuffer<char>::*)(const class wxCharTypeBuffer<char> &)) &wxCharTypeBuffer<char>::operator=, "C++: wxCharTypeBuffer<char>::operator=(const class wxCharTypeBuffer<char> &) --> class wxCharTypeBuffer<char> &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("assign", (class wxCharTypeBuffer<char> & (wxCharTypeBuffer<char>::*)(const class wxScopedCharTypeBuffer<char> &)) &wxCharTypeBuffer<char>::operator=, "C++: wxCharTypeBuffer<char>::operator=(const class wxScopedCharTypeBuffer<char> &) --> class wxCharTypeBuffer<char> &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("extend", (bool (wxCharTypeBuffer<char>::*)(unsigned long)) &wxCharTypeBuffer<char>::extend, "C++: wxCharTypeBuffer<char>::extend(unsigned long) --> bool", pybind11::arg("len"));
		cl.def("shrink", (void (wxCharTypeBuffer<char>::*)(unsigned long)) &wxCharTypeBuffer<char>::shrink, "C++: wxCharTypeBuffer<char>::shrink(unsigned long) --> void", pybind11::arg("len"));
		cl.def_static("CreateNonOwned", [](const char * a0) -> const wxScopedCharTypeBuffer<char> { return wxScopedCharTypeBuffer<char>::CreateNonOwned(a0); }, "", pybind11::arg("str"));
		cl.def_static("CreateNonOwned", (const class wxScopedCharTypeBuffer<char> (*)(const char *, unsigned long)) &wxScopedCharTypeBuffer<char>::CreateNonOwned, "C++: wxScopedCharTypeBuffer<char>::CreateNonOwned(const char *, unsigned long) --> const class wxScopedCharTypeBuffer<char>", pybind11::arg("str"), pybind11::arg("len"));
		cl.def_static("CreateOwned", [](char * a0) -> const wxScopedCharTypeBuffer<char> { return wxScopedCharTypeBuffer<char>::CreateOwned(a0); }, "", pybind11::arg("str"));
		cl.def_static("CreateOwned", (const class wxScopedCharTypeBuffer<char> (*)(char *, unsigned long)) &wxScopedCharTypeBuffer<char>::CreateOwned, "C++: wxScopedCharTypeBuffer<char>::CreateOwned(char *, unsigned long) --> const class wxScopedCharTypeBuffer<char>", pybind11::arg("str"), pybind11::arg("len"));
		cl.def("assign", (class wxScopedCharTypeBuffer<char> & (wxScopedCharTypeBuffer<char>::*)(const class wxScopedCharTypeBuffer<char> &)) &wxScopedCharTypeBuffer<char>::operator=, "C++: wxScopedCharTypeBuffer<char>::operator=(const class wxScopedCharTypeBuffer<char> &) --> class wxScopedCharTypeBuffer<char> &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("release", (char * (wxScopedCharTypeBuffer<char>::*)() const) &wxScopedCharTypeBuffer<char>::release, "C++: wxScopedCharTypeBuffer<char>::release() const --> char *", pybind11::return_value_policy::automatic);
		cl.def("reset", (void (wxScopedCharTypeBuffer<char>::*)()) &wxScopedCharTypeBuffer<char>::reset, "C++: wxScopedCharTypeBuffer<char>::reset() --> void");
		cl.def("data", (char * (wxScopedCharTypeBuffer<char>::*)()) &wxScopedCharTypeBuffer<char>::data, "C++: wxScopedCharTypeBuffer<char>::data() --> char *", pybind11::return_value_policy::automatic);
		cl.def("__getitem__", (char (wxScopedCharTypeBuffer<char>::*)(unsigned long) const) &wxScopedCharTypeBuffer<char>::operator[], "C++: wxScopedCharTypeBuffer<char>::operator[](unsigned long) const --> char", pybind11::arg("n"));
		cl.def("length", (unsigned long (wxScopedCharTypeBuffer<char>::*)() const) &wxScopedCharTypeBuffer<char>::length, "C++: wxScopedCharTypeBuffer<char>::length() const --> unsigned long");
	}
	{ // wxCharTypeBuffer file: line:247
		pybind11::class_<wxCharTypeBuffer<wchar_t>, std::shared_ptr<wxCharTypeBuffer<wchar_t>>, wxScopedCharTypeBuffer<wchar_t>> cl(M(""), "wxCharTypeBuffer_wchar_t_t", "");
		cl.def( pybind11::init( [](){ return new wxCharTypeBuffer<wchar_t>(); } ), "doc" );
		cl.def( pybind11::init( [](const wchar_t * a0){ return new wxCharTypeBuffer<wchar_t>(a0); } ), "doc" , pybind11::arg("str"));
		cl.def( pybind11::init<const wchar_t *, unsigned long>(), pybind11::arg("str"), pybind11::arg("len") );

		cl.def( pybind11::init<unsigned long>(), pybind11::arg("len") );

		cl.def( pybind11::init( [](wxCharTypeBuffer<wchar_t> const &o){ return new wxCharTypeBuffer<wchar_t>(o); } ) );
		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<wchar_t> &>(), pybind11::arg("src") );

		cl.def("assign", (class wxCharTypeBuffer<wchar_t> & (wxCharTypeBuffer<wchar_t>::*)(const wchar_t *)) &wxCharTypeBuffer<wchar_t>::operator=, "C++: wxCharTypeBuffer<wchar_t>::operator=(const wchar_t *) --> class wxCharTypeBuffer<wchar_t> &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("assign", (class wxCharTypeBuffer<wchar_t> & (wxCharTypeBuffer<wchar_t>::*)(const class wxCharTypeBuffer<wchar_t> &)) &wxCharTypeBuffer<wchar_t>::operator=, "C++: wxCharTypeBuffer<wchar_t>::operator=(const class wxCharTypeBuffer<wchar_t> &) --> class wxCharTypeBuffer<wchar_t> &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("assign", (class wxCharTypeBuffer<wchar_t> & (wxCharTypeBuffer<wchar_t>::*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxCharTypeBuffer<wchar_t>::operator=, "C++: wxCharTypeBuffer<wchar_t>::operator=(const class wxScopedCharTypeBuffer<wchar_t> &) --> class wxCharTypeBuffer<wchar_t> &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("extend", (bool (wxCharTypeBuffer<wchar_t>::*)(unsigned long)) &wxCharTypeBuffer<wchar_t>::extend, "C++: wxCharTypeBuffer<wchar_t>::extend(unsigned long) --> bool", pybind11::arg("len"));
		cl.def("shrink", (void (wxCharTypeBuffer<wchar_t>::*)(unsigned long)) &wxCharTypeBuffer<wchar_t>::shrink, "C++: wxCharTypeBuffer<wchar_t>::shrink(unsigned long) --> void", pybind11::arg("len"));
		cl.def_static("CreateNonOwned", [](const wchar_t * a0) -> const wxScopedCharTypeBuffer<wchar_t> { return wxScopedCharTypeBuffer<wchar_t>::CreateNonOwned(a0); }, "", pybind11::arg("str"));
		cl.def_static("CreateNonOwned", (const class wxScopedCharTypeBuffer<wchar_t> (*)(const wchar_t *, unsigned long)) &wxScopedCharTypeBuffer<wchar_t>::CreateNonOwned, "C++: wxScopedCharTypeBuffer<wchar_t>::CreateNonOwned(const wchar_t *, unsigned long) --> const class wxScopedCharTypeBuffer<wchar_t>", pybind11::arg("str"), pybind11::arg("len"));
		cl.def_static("CreateOwned", [](wchar_t * a0) -> const wxScopedCharTypeBuffer<wchar_t> { return wxScopedCharTypeBuffer<wchar_t>::CreateOwned(a0); }, "", pybind11::arg("str"));
		cl.def_static("CreateOwned", (const class wxScopedCharTypeBuffer<wchar_t> (*)(wchar_t *, unsigned long)) &wxScopedCharTypeBuffer<wchar_t>::CreateOwned, "C++: wxScopedCharTypeBuffer<wchar_t>::CreateOwned(wchar_t *, unsigned long) --> const class wxScopedCharTypeBuffer<wchar_t>", pybind11::arg("str"), pybind11::arg("len"));
		cl.def("assign", (class wxScopedCharTypeBuffer<wchar_t> & (wxScopedCharTypeBuffer<wchar_t>::*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxScopedCharTypeBuffer<wchar_t>::operator=, "C++: wxScopedCharTypeBuffer<wchar_t>::operator=(const class wxScopedCharTypeBuffer<wchar_t> &) --> class wxScopedCharTypeBuffer<wchar_t> &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("release", (wchar_t * (wxScopedCharTypeBuffer<wchar_t>::*)() const) &wxScopedCharTypeBuffer<wchar_t>::release, "C++: wxScopedCharTypeBuffer<wchar_t>::release() const --> wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("reset", (void (wxScopedCharTypeBuffer<wchar_t>::*)()) &wxScopedCharTypeBuffer<wchar_t>::reset, "C++: wxScopedCharTypeBuffer<wchar_t>::reset() --> void");
		cl.def("data", (wchar_t * (wxScopedCharTypeBuffer<wchar_t>::*)()) &wxScopedCharTypeBuffer<wchar_t>::data, "C++: wxScopedCharTypeBuffer<wchar_t>::data() --> wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("__getitem__", (wchar_t (wxScopedCharTypeBuffer<wchar_t>::*)(unsigned long) const) &wxScopedCharTypeBuffer<wchar_t>::operator[], "C++: wxScopedCharTypeBuffer<wchar_t>::operator[](unsigned long) const --> wchar_t", pybind11::arg("n"));
		cl.def("length", (unsigned long (wxScopedCharTypeBuffer<wchar_t>::*)() const) &wxScopedCharTypeBuffer<wchar_t>::length, "C++: wxScopedCharTypeBuffer<wchar_t>::length() const --> unsigned long");
	}
	{ // wxCharBuffer file: line:360
		pybind11::class_<wxCharBuffer, std::shared_ptr<wxCharBuffer>, wxCharTypeBuffer<char>> cl(M(""), "wxCharBuffer", "");
		cl.def( pybind11::init<const class wxCharTypeBuffer<char> &>(), pybind11::arg("buf") );

		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<char> &>(), pybind11::arg("buf") );

		cl.def( pybind11::init( [](){ return new wxCharBuffer(); } ), "doc" );
		cl.def( pybind11::init<const char *>(), pybind11::arg("str") );

		cl.def( pybind11::init<unsigned long>(), pybind11::arg("len") );

		cl.def( pybind11::init<const class wxCStrData &>(), pybind11::arg("cstr") );

		cl.def( pybind11::init( [](wxCharBuffer const &o){ return new wxCharBuffer(o); } ) );
		cl.def("assign", (class wxCharBuffer & (wxCharBuffer::*)(const class wxCharBuffer &)) &wxCharBuffer::operator=, "C++: wxCharBuffer::operator=(const class wxCharBuffer &) --> class wxCharBuffer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxWCharBuffer file: line:380
		pybind11::class_<wxWCharBuffer, std::shared_ptr<wxWCharBuffer>, wxCharTypeBuffer<wchar_t>> cl(M(""), "wxWCharBuffer", "");
		cl.def( pybind11::init<const class wxCharTypeBuffer<wchar_t> &>(), pybind11::arg("buf") );

		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<wchar_t> &>(), pybind11::arg("buf") );

		cl.def( pybind11::init( [](){ return new wxWCharBuffer(); } ), "doc" );
		cl.def( pybind11::init<const wchar_t *>(), pybind11::arg("str") );

		cl.def( pybind11::init<unsigned long>(), pybind11::arg("len") );

		cl.def( pybind11::init<const class wxCStrData &>(), pybind11::arg("cstr") );

		cl.def( pybind11::init( [](wxWCharBuffer const &o){ return new wxWCharBuffer(o); } ) );
		cl.def("assign", (class wxWCharBuffer & (wxWCharBuffer::*)(const class wxWCharBuffer &)) &wxWCharBuffer::operator=, "C++: wxWCharBuffer::operator=(const class wxWCharBuffer &) --> class wxWCharBuffer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
