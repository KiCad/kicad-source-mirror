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

// wxMBConv file: line:47
struct PyCallBack_wxMBConv : public wxMBConv {
	using wxMBConv::wxMBConv;

	unsigned long ToWChar(wchar_t * a0, unsigned long a1, const char * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConv *>(this), "ToWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::ToWChar(a0, a1, a2, a3);
	}
	unsigned long FromWChar(char * a0, unsigned long a1, const wchar_t * a2, unsigned long a3) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConv *>(this), "FromWChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::FromWChar(a0, a1, a2, a3);
	}
	unsigned long GetMBNulLen() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConv *>(this), "GetMBNulLen");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::GetMBNulLen();
	}
	unsigned long MB2WC(wchar_t * a0, const char * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConv *>(this), "MB2WC");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::MB2WC(a0, a1, a2);
	}
	unsigned long WC2MB(char * a0, const wchar_t * a1, unsigned long a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConv *>(this), "WC2MB");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxMBConv::WC2MB(a0, a1, a2);
	}
	class wxMBConv * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMBConv *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxMBConv *>::value) {
				static pybind11::detail::override_caster_t<class wxMBConv *> caster;
				return pybind11::detail::cast_ref<class wxMBConv *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxMBConv *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxMBConv::Clone\"");
	}
};

void bind_unknown_unknown_4(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxWritableCharTypeBuffer file: line:399
		pybind11::class_<wxWritableCharTypeBuffer<char>, std::shared_ptr<wxWritableCharTypeBuffer<char>>, wxCharTypeBuffer<char>> cl(M(""), "wxWritableCharTypeBuffer_char_t", "");
		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<char> &>(), pybind11::arg("src") );

		cl.def( pybind11::init( [](){ return new wxWritableCharTypeBuffer<char>(); } ), "doc" );
		cl.def( pybind11::init<const char *>(), pybind11::arg("str") );

		cl.def( pybind11::init( [](wxWritableCharTypeBuffer<char> const &o){ return new wxWritableCharTypeBuffer<char>(o); } ) );
		cl.def("assign", (class wxWritableCharTypeBuffer<char> & (wxWritableCharTypeBuffer<char>::*)(const class wxWritableCharTypeBuffer<char> &)) &wxWritableCharTypeBuffer<char>::operator=, "C++: wxWritableCharTypeBuffer<char>::operator=(const class wxWritableCharTypeBuffer<char> &) --> class wxWritableCharTypeBuffer<char> &", pybind11::return_value_policy::automatic, pybind11::arg(""));
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
	{ // wxWritableCharTypeBuffer file: line:399
		pybind11::class_<wxWritableCharTypeBuffer<wchar_t>, std::shared_ptr<wxWritableCharTypeBuffer<wchar_t>>, wxCharTypeBuffer<wchar_t>> cl(M(""), "wxWritableCharTypeBuffer_wchar_t_t", "");
		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<wchar_t> &>(), pybind11::arg("src") );

		cl.def( pybind11::init( [](){ return new wxWritableCharTypeBuffer<wchar_t>(); } ), "doc" );
		cl.def( pybind11::init<const wchar_t *>(), pybind11::arg("str") );

		cl.def( pybind11::init( [](wxWritableCharTypeBuffer<wchar_t> const &o){ return new wxWritableCharTypeBuffer<wchar_t>(o); } ) );
		cl.def("assign", (class wxWritableCharTypeBuffer<wchar_t> & (wxWritableCharTypeBuffer<wchar_t>::*)(const class wxWritableCharTypeBuffer<wchar_t> &)) &wxWritableCharTypeBuffer<wchar_t>::operator=, "C++: wxWritableCharTypeBuffer<wchar_t>::operator=(const class wxWritableCharTypeBuffer<wchar_t> &) --> class wxWritableCharTypeBuffer<wchar_t> &", pybind11::return_value_policy::automatic, pybind11::arg(""));
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
	{ // wxMemoryBufferData file: line:446
		pybind11::class_<wxMemoryBufferData, wxMemoryBufferData*> cl(M(""), "wxMemoryBufferData", "");
	}
	{ // wxMemoryBuffer file: line:518
		pybind11::class_<wxMemoryBuffer, std::shared_ptr<wxMemoryBuffer>> cl(M(""), "wxMemoryBuffer", "");
		cl.def( pybind11::init( [](){ return new wxMemoryBuffer(); } ), "doc" );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("size") );

		cl.def( pybind11::init( [](wxMemoryBuffer const &o){ return new wxMemoryBuffer(o); } ) );
		cl.def("assign", (class wxMemoryBuffer & (wxMemoryBuffer::*)(const class wxMemoryBuffer &)) &wxMemoryBuffer::operator=, "C++: wxMemoryBuffer::operator=(const class wxMemoryBuffer &) --> class wxMemoryBuffer &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("GetData", (void * (wxMemoryBuffer::*)() const) &wxMemoryBuffer::GetData, "C++: wxMemoryBuffer::GetData() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("GetBufSize", (unsigned long (wxMemoryBuffer::*)() const) &wxMemoryBuffer::GetBufSize, "C++: wxMemoryBuffer::GetBufSize() const --> unsigned long");
		cl.def("GetDataLen", (unsigned long (wxMemoryBuffer::*)() const) &wxMemoryBuffer::GetDataLen, "C++: wxMemoryBuffer::GetDataLen() const --> unsigned long");
		cl.def("IsEmpty", (bool (wxMemoryBuffer::*)() const) &wxMemoryBuffer::IsEmpty, "C++: wxMemoryBuffer::IsEmpty() const --> bool");
		cl.def("SetBufSize", (void (wxMemoryBuffer::*)(unsigned long)) &wxMemoryBuffer::SetBufSize, "C++: wxMemoryBuffer::SetBufSize(unsigned long) --> void", pybind11::arg("size"));
		cl.def("SetDataLen", (void (wxMemoryBuffer::*)(unsigned long)) &wxMemoryBuffer::SetDataLen, "C++: wxMemoryBuffer::SetDataLen(unsigned long) --> void", pybind11::arg("len"));
		cl.def("Clear", (void (wxMemoryBuffer::*)()) &wxMemoryBuffer::Clear, "C++: wxMemoryBuffer::Clear() --> void");
		cl.def("GetWriteBuf", (void * (wxMemoryBuffer::*)(unsigned long)) &wxMemoryBuffer::GetWriteBuf, "C++: wxMemoryBuffer::GetWriteBuf(unsigned long) --> void *", pybind11::return_value_policy::automatic, pybind11::arg("sizeNeeded"));
		cl.def("UngetWriteBuf", (void (wxMemoryBuffer::*)(unsigned long)) &wxMemoryBuffer::UngetWriteBuf, "C++: wxMemoryBuffer::UngetWriteBuf(unsigned long) --> void", pybind11::arg("sizeUsed"));
		cl.def("GetAppendBuf", (void * (wxMemoryBuffer::*)(unsigned long)) &wxMemoryBuffer::GetAppendBuf, "C++: wxMemoryBuffer::GetAppendBuf(unsigned long) --> void *", pybind11::return_value_policy::automatic, pybind11::arg("sizeNeeded"));
		cl.def("UngetAppendBuf", (void (wxMemoryBuffer::*)(unsigned long)) &wxMemoryBuffer::UngetAppendBuf, "C++: wxMemoryBuffer::UngetAppendBuf(unsigned long) --> void", pybind11::arg("sizeUsed"));
		cl.def("AppendByte", (void (wxMemoryBuffer::*)(char)) &wxMemoryBuffer::AppendByte, "C++: wxMemoryBuffer::AppendByte(char) --> void", pybind11::arg("data"));
		cl.def("AppendData", (void (wxMemoryBuffer::*)(const void *, unsigned long)) &wxMemoryBuffer::AppendData, "C++: wxMemoryBuffer::AppendData(const void *, unsigned long) --> void", pybind11::arg("data"), pybind11::arg("len"));
		cl.def("release", (void * (wxMemoryBuffer::*)()) &wxMemoryBuffer::release, "C++: wxMemoryBuffer::release() --> void *", pybind11::return_value_policy::automatic);
	}
	{ // wxMBConv file: line:47
		pybind11::class_<wxMBConv, std::shared_ptr<wxMBConv>, PyCallBack_wxMBConv> cl(M(""), "wxMBConv", "");
		cl.def(pybind11::init<PyCallBack_wxMBConv const &>());
		cl.def( pybind11::init( [](){ return new PyCallBack_wxMBConv(); } ) );
		cl.def("ToWChar", [](wxMBConv const &o, wchar_t * a0, unsigned long const & a1, const char * a2) -> unsigned long { return o.ToWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("ToWChar", (unsigned long (wxMBConv::*)(wchar_t *, unsigned long, const char *, unsigned long) const) &wxMBConv::ToWChar, "C++: wxMBConv::ToWChar(wchar_t *, unsigned long, const char *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("FromWChar", [](wxMBConv const &o, char * a0, unsigned long const & a1, const wchar_t * a2) -> unsigned long { return o.FromWChar(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
		cl.def("FromWChar", (unsigned long (wxMBConv::*)(char *, unsigned long, const wchar_t *, unsigned long) const) &wxMBConv::FromWChar, "C++: wxMBConv::FromWChar(char *, unsigned long, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
		cl.def("cMB2WC", (const class wxWCharBuffer (wxMBConv::*)(const char *) const) &wxMBConv::cMB2WC, "C++: wxMBConv::cMB2WC(const char *) const --> const class wxWCharBuffer", pybind11::arg("in"));
		cl.def("cWC2MB", (const class wxCharBuffer (wxMBConv::*)(const wchar_t *) const) &wxMBConv::cWC2MB, "C++: wxMBConv::cWC2MB(const wchar_t *) const --> const class wxCharBuffer", pybind11::arg("in"));
		cl.def("cMB2WC", (const class wxWCharBuffer (wxMBConv::*)(const char *, unsigned long, unsigned long *) const) &wxMBConv::cMB2WC, "C++: wxMBConv::cMB2WC(const char *, unsigned long, unsigned long *) const --> const class wxWCharBuffer", pybind11::arg("in"), pybind11::arg("inLen"), pybind11::arg("outLen"));
		cl.def("cWC2MB", (const class wxCharBuffer (wxMBConv::*)(const wchar_t *, unsigned long, unsigned long *) const) &wxMBConv::cWC2MB, "C++: wxMBConv::cWC2MB(const wchar_t *, unsigned long, unsigned long *) const --> const class wxCharBuffer", pybind11::arg("in"), pybind11::arg("inLen"), pybind11::arg("outLen"));
		cl.def("cMB2WC", (const class wxWCharBuffer (wxMBConv::*)(const class wxScopedCharTypeBuffer<char> &) const) &wxMBConv::cMB2WC, "C++: wxMBConv::cMB2WC(const class wxScopedCharTypeBuffer<char> &) const --> const class wxWCharBuffer", pybind11::arg("in"));
		cl.def("cWC2MB", (const class wxCharBuffer (wxMBConv::*)(const class wxScopedCharTypeBuffer<wchar_t> &) const) &wxMBConv::cWC2MB, "C++: wxMBConv::cWC2MB(const class wxScopedCharTypeBuffer<wchar_t> &) const --> const class wxCharBuffer", pybind11::arg("in"));
		cl.def("cMB2WX", (const class wxWCharBuffer (wxMBConv::*)(const char *) const) &wxMBConv::cMB2WX, "C++: wxMBConv::cMB2WX(const char *) const --> const class wxWCharBuffer", pybind11::arg("psz"));
		cl.def("cWX2MB", (const class wxCharBuffer (wxMBConv::*)(const wchar_t *) const) &wxMBConv::cWX2MB, "C++: wxMBConv::cWX2MB(const wchar_t *) const --> const class wxCharBuffer", pybind11::arg("psz"));
		cl.def("cWC2WX", (const wchar_t * (wxMBConv::*)(const wchar_t *) const) &wxMBConv::cWC2WX, "C++: wxMBConv::cWC2WX(const wchar_t *) const --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("psz"));
		cl.def("cWX2WC", (const wchar_t * (wxMBConv::*)(const wchar_t *) const) &wxMBConv::cWX2WC, "C++: wxMBConv::cWX2WC(const wchar_t *) const --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("psz"));
		cl.def("GetMBNulLen", (unsigned long (wxMBConv::*)() const) &wxMBConv::GetMBNulLen, "C++: wxMBConv::GetMBNulLen() const --> unsigned long");
		cl.def_static("GetMaxMBNulLen", (unsigned long (*)()) &wxMBConv::GetMaxMBNulLen, "C++: wxMBConv::GetMaxMBNulLen() --> unsigned long");
		cl.def("MB2WC", (unsigned long (wxMBConv::*)(wchar_t *, const char *, unsigned long) const) &wxMBConv::MB2WC, "C++: wxMBConv::MB2WC(wchar_t *, const char *, unsigned long) const --> unsigned long", pybind11::arg("out"), pybind11::arg("in"), pybind11::arg("outLen"));
		cl.def("WC2MB", (unsigned long (wxMBConv::*)(char *, const wchar_t *, unsigned long) const) &wxMBConv::WC2MB, "C++: wxMBConv::WC2MB(char *, const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("out"), pybind11::arg("in"), pybind11::arg("outLen"));
		cl.def("Clone", (class wxMBConv * (wxMBConv::*)() const) &wxMBConv::Clone, "C++: wxMBConv::Clone() const --> class wxMBConv *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxMBConv & (wxMBConv::*)(const class wxMBConv &)) &wxMBConv::operator=, "C++: wxMBConv::operator=(const class wxMBConv &) --> class wxMBConv &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
