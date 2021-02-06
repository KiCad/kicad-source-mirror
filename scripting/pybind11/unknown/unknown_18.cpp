#include <cstdio> // _IO_FILE
#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <time.h> // tm

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxMessageOutputBase file: line:32
struct PyCallBack_wxMessageOutputBase : public wxMessageOutputBase {
	using wxMessageOutputBase::wxMessageOutputBase;

	void Output(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputBase *>(this), "Output");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxMessageOutputBase::Output\"");
	}
	void DoPrintfWchar(const wchar_t * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputBase *>(this), "DoPrintfWchar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxMessageOutputBase::DoPrintfWchar\"");
	}
};

// wxMessageOutput file: line:75
struct PyCallBack_wxMessageOutput : public wxMessageOutput {
	using wxMessageOutput::wxMessageOutput;

	void DoPrintfWchar(const wchar_t * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutput *>(this), "DoPrintfWchar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutput::DoPrintfWchar(a0);
	}
	void Output(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutput *>(this), "Output");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxMessageOutputBase::Output\"");
	}
};

// wxMessageOutputStderr file: line:107
struct PyCallBack_wxMessageOutputStderr : public wxMessageOutputStderr {
	using wxMessageOutputStderr::wxMessageOutputStderr;

	void Output(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputStderr *>(this), "Output");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutputStderr::Output(a0);
	}
	void DoPrintfWchar(const wchar_t * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputStderr *>(this), "DoPrintfWchar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutput::DoPrintfWchar(a0);
	}
};

void bind_unknown_unknown_18(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxStrpbrk(const class wxCStrData &, const char *) file: line:707
	M("").def("wxStrpbrk", (const char * (*)(const class wxCStrData &, const char *)) &wxStrpbrk, "C++: wxStrpbrk(const class wxCStrData &, const char *) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const class wxCStrData &, const wchar_t *) file: line:709
	M("").def("wxStrpbrk", (const wchar_t * (*)(const class wxCStrData &, const wchar_t *)) &wxStrpbrk, "C++: wxStrpbrk(const class wxCStrData &, const wchar_t *) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxStrpbrk(const class wxCStrData &, const class wxCStrData &) file: line:711
	M("").def("wxStrpbrk", (const char * (*)(const class wxCStrData &, const class wxCStrData &)) &wxStrpbrk, "C++: wxStrpbrk(const class wxCStrData &, const class wxCStrData &) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("accept"));

	// wxFopen(const class wxString &, const class wxString &) file: line:753
	M("").def("wxFopen", (struct _IO_FILE * (*)(const class wxString &, const class wxString &)) &wxFopen, "C++: wxFopen(const class wxString &, const class wxString &) --> struct _IO_FILE *", pybind11::return_value_policy::automatic, pybind11::arg("path"), pybind11::arg("mode"));

	// wxFreopen(const class wxString &, const class wxString &, struct _IO_FILE *) file: line:755
	M("").def("wxFreopen", (struct _IO_FILE * (*)(const class wxString &, const class wxString &, struct _IO_FILE *)) &wxFreopen, "C++: wxFreopen(const class wxString &, const class wxString &, struct _IO_FILE *) --> struct _IO_FILE *", pybind11::return_value_policy::automatic, pybind11::arg("path"), pybind11::arg("mode"), pybind11::arg("stream"));

	// wxRemove(const class wxString &) file: line:757
	M("").def("wxRemove", (int (*)(const class wxString &)) &wxRemove, "C++: wxRemove(const class wxString &) --> int", pybind11::arg("path"));

	// wxRename(const class wxString &, const class wxString &) file: line:759
	M("").def("wxRename", (int (*)(const class wxString &, const class wxString &)) &wxRename, "C++: wxRename(const class wxString &, const class wxString &) --> int", pybind11::arg("oldpath"), pybind11::arg("newpath"));

	// wxPuts(const class wxString &) file: line:762
	M("").def("wxPuts", (int (*)(const class wxString &)) &wxPuts, "C++: wxPuts(const class wxString &) --> int", pybind11::arg("s"));

	// wxFputs(const class wxString &, struct _IO_FILE *) file: line:763
	M("").def("wxFputs", (int (*)(const class wxString &, struct _IO_FILE *)) &wxFputs, "C++: wxFputs(const class wxString &, struct _IO_FILE *) --> int", pybind11::arg("s"), pybind11::arg("stream"));

	// wxPerror(const class wxString &) file: line:764
	M("").def("wxPerror", (void (*)(const class wxString &)) &wxPerror, "C++: wxPerror(const class wxString &) --> void", pybind11::arg("s"));

	// wxFputc(const class wxUniChar &, struct _IO_FILE *) file: line:766
	M("").def("wxFputc", (int (*)(const class wxUniChar &, struct _IO_FILE *)) &wxFputc, "C++: wxFputc(const class wxUniChar &, struct _IO_FILE *) --> int", pybind11::arg("c"), pybind11::arg("stream"));

	// wxFgets(char *, int, struct _IO_FILE *) file: line:774
	M("").def("wxFgets", (char * (*)(char *, int, struct _IO_FILE *)) &wxFgets, "C++: wxFgets(char *, int, struct _IO_FILE *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("size"), pybind11::arg("stream"));

	// wxFgets(wchar_t *, int, struct _IO_FILE *) file: line:777
	M("").def("wxFgets", (wchar_t * (*)(wchar_t *, int, struct _IO_FILE *)) &wxFgets, "C++: wxFgets(wchar_t *, int, struct _IO_FILE *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("s"), pybind11::arg("size"), pybind11::arg("stream"));

	// wxFgetc(struct _IO_FILE *) file: line:783
	M("").def("wxFgetc", (int (*)(struct _IO_FILE *)) &wxFgetc, "C++: wxFgetc(struct _IO_FILE *) --> int", pybind11::arg("stream"));

	// wxUngetc(int, struct _IO_FILE *) file: line:784
	M("").def("wxUngetc", (int (*)(int, struct _IO_FILE *)) &wxUngetc, "C++: wxUngetc(int, struct _IO_FILE *) --> int", pybind11::arg("c"), pybind11::arg("stream"));

	// wxAtoi(const class wxString &) file: line:797
	M("").def("wxAtoi", (int (*)(const class wxString &)) &wxAtoi, "C++: wxAtoi(const class wxString &) --> int", pybind11::arg("str"));

	// wxAtol(const class wxString &) file: line:801
	M("").def("wxAtol", (long (*)(const class wxString &)) &wxAtol, "C++: wxAtol(const class wxString &) --> long", pybind11::arg("str"));

	// wxAtof(const class wxString &) file: line:807
	M("").def("wxAtof", (double (*)(const class wxString &)) &wxAtof, "C++: wxAtof(const class wxString &) --> double", pybind11::arg("str"));

	// wxSystem(const class wxString &) file: line:918
	M("").def("wxSystem", (int (*)(const class wxString &)) &wxSystem, "C++: wxSystem(const class wxString &) --> int", pybind11::arg("str"));

	// wxGetenv(const char *) file: line:923
	M("").def("wxGetenv", (char * (*)(const char *)) &wxGetenv, "C++: wxGetenv(const char *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("name"));

	// wxGetenv(const wchar_t *) file: line:924
	M("").def("wxGetenv", (wchar_t * (*)(const wchar_t *)) &wxGetenv, "C++: wxGetenv(const wchar_t *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("name"));

	// wxGetenv(const class wxString &) file: line:925
	M("").def("wxGetenv", (char * (*)(const class wxString &)) &wxGetenv, "C++: wxGetenv(const class wxString &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("name"));

	// wxGetenv(const class wxCStrData &) file: line:926
	M("").def("wxGetenv", (char * (*)(const class wxCStrData &)) &wxGetenv, "C++: wxGetenv(const class wxCStrData &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("name"));

	// wxGetenv(const class wxScopedCharTypeBuffer<char> &) file: line:927
	M("").def("wxGetenv", (char * (*)(const class wxScopedCharTypeBuffer<char> &)) &wxGetenv, "C++: wxGetenv(const class wxScopedCharTypeBuffer<char> &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("name"));

	// wxGetenv(const class wxScopedCharTypeBuffer<wchar_t> &) file: line:928
	M("").def("wxGetenv", (wchar_t * (*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxGetenv, "C++: wxGetenv(const class wxScopedCharTypeBuffer<wchar_t> &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("name"));

	// wxStrftime(char *, unsigned long, const class wxString &, const struct tm *) file: line:934
	M("").def("wxStrftime", (unsigned long (*)(char *, unsigned long, const class wxString &, const struct tm *)) &wxStrftime, "C++: wxStrftime(char *, unsigned long, const class wxString &, const struct tm *) --> unsigned long", pybind11::arg("s"), pybind11::arg("max"), pybind11::arg("format"), pybind11::arg("tm"));

	// wxStrftime(wchar_t *, unsigned long, const class wxString &, const struct tm *) file: line:938
	M("").def("wxStrftime", (unsigned long (*)(wchar_t *, unsigned long, const class wxString &, const struct tm *)) &wxStrftime, "C++: wxStrftime(wchar_t *, unsigned long, const class wxString &, const struct tm *) --> unsigned long", pybind11::arg("s"), pybind11::arg("max"), pybind11::arg("format"), pybind11::arg("tm"));

	// wxIsalnum(const class wxUniChar &) file: line:960
	M("").def("wxIsalnum", (bool (*)(const class wxUniChar &)) &wxIsalnum, "C++: wxIsalnum(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxIsalpha(const class wxUniChar &) file: line:961
	M("").def("wxIsalpha", (bool (*)(const class wxUniChar &)) &wxIsalpha, "C++: wxIsalpha(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxIscntrl(const class wxUniChar &) file: line:962
	M("").def("wxIscntrl", (bool (*)(const class wxUniChar &)) &wxIscntrl, "C++: wxIscntrl(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxIsdigit(const class wxUniChar &) file: line:963
	M("").def("wxIsdigit", (bool (*)(const class wxUniChar &)) &wxIsdigit, "C++: wxIsdigit(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxIsgraph(const class wxUniChar &) file: line:964
	M("").def("wxIsgraph", (bool (*)(const class wxUniChar &)) &wxIsgraph, "C++: wxIsgraph(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxIslower(const class wxUniChar &) file: line:965
	M("").def("wxIslower", (bool (*)(const class wxUniChar &)) &wxIslower, "C++: wxIslower(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxIsprint(const class wxUniChar &) file: line:966
	M("").def("wxIsprint", (bool (*)(const class wxUniChar &)) &wxIsprint, "C++: wxIsprint(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxIspunct(const class wxUniChar &) file: line:967
	M("").def("wxIspunct", (bool (*)(const class wxUniChar &)) &wxIspunct, "C++: wxIspunct(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxIsspace(const class wxUniChar &) file: line:968
	M("").def("wxIsspace", (bool (*)(const class wxUniChar &)) &wxIsspace, "C++: wxIsspace(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxIsupper(const class wxUniChar &) file: line:969
	M("").def("wxIsupper", (bool (*)(const class wxUniChar &)) &wxIsupper, "C++: wxIsupper(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxIsxdigit(const class wxUniChar &) file: line:970
	M("").def("wxIsxdigit", (bool (*)(const class wxUniChar &)) &wxIsxdigit, "C++: wxIsxdigit(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxTolower(const class wxUniChar &) file: line:972
	M("").def("wxTolower", (class wxUniChar (*)(const class wxUniChar &)) &wxTolower, "C++: wxTolower(const class wxUniChar &) --> class wxUniChar", pybind11::arg("c"));

	// wxToupper(const class wxUniChar &) file: line:973
	M("").def("wxToupper", (class wxUniChar (*)(const class wxUniChar &)) &wxToupper, "C++: wxToupper(const class wxUniChar &) --> class wxUniChar", pybind11::arg("c"));

	// wxIsctrl(const class wxUniChar &) file: line:980
	M("").def("wxIsctrl", (int (*)(const class wxUniChar &)) &wxIsctrl, "C++: wxIsctrl(const class wxUniChar &) --> int", pybind11::arg("c"));

	// wxIsascii(const class wxUniChar &) file: line:984
	M("").def("wxIsascii", (bool (*)(const class wxUniChar &)) &wxIsascii, "C++: wxIsascii(const class wxUniChar &) --> bool", pybind11::arg("c"));

	// wxPrintf(const class wxFormatString &, class wxString) file: line:290
	M("").def("wxPrintf", (int (*)(const class wxFormatString &, class wxString)) &wxPrintf<wxString>, "C++: wxPrintf(const class wxFormatString &, class wxString) --> int", pybind11::arg("f1"), pybind11::arg("a1"));

	// wxPrintf(const class wxFormatString &) file: line:292
	M("").def("wxPrintf", (int (*)(const class wxFormatString &)) &wxPrintf, "C++: wxPrintf(const class wxFormatString &) --> int", pybind11::arg("s"));

	// wxFprintf(struct _IO_FILE *, const class wxFormatString &, class wxString) file: line:297
	M("").def("wxFprintf", (int (*)(struct _IO_FILE *, const class wxFormatString &, class wxString)) &wxFprintf<wxString>, "C++: wxFprintf(struct _IO_FILE *, const class wxFormatString &, class wxString) --> int", pybind11::arg("f1"), pybind11::arg("f2"), pybind11::arg("a1"));

	// wxFprintf(struct _IO_FILE *, const class wxFormatString &) file: line:299
	M("").def("wxFprintf", (int (*)(struct _IO_FILE *, const class wxFormatString &)) &wxFprintf, "C++: wxFprintf(struct _IO_FILE *, const class wxFormatString &) --> int", pybind11::arg("f"), pybind11::arg("s"));

	// wxDoSprintfWchar(char *, const wchar_t *) file: line:345
	M("").def("wxDoSprintfWchar", [](char * a0, const wchar_t * a1) -> int { return wxDoSprintfWchar(a0, a1); }, "", pybind11::arg("str"), pybind11::arg("format"));

	// wxSprintf(char *, const class wxFormatString &) file: line:350
	M("").def("wxSprintf", (int (*)(char *, const class wxFormatString &)) &wxSprintf, "C++: wxSprintf(char *, const class wxFormatString &) --> int", pybind11::arg("f1"), pybind11::arg("f2"));

	// wxDoSnprintfWchar(char *, unsigned long, const wchar_t *) file: line:357
	M("").def("wxDoSnprintfWchar", [](char * a0, unsigned long const & a1, const wchar_t * a2) -> int { return wxDoSnprintfWchar(a0, a1, a2); }, "", pybind11::arg("str"), pybind11::arg("size"), pybind11::arg("format"));

	// wxSnprintf(char *, unsigned long, const class wxFormatString &) file: line:362
	M("").def("wxSnprintf", (int (*)(char *, unsigned long, const class wxFormatString &)) &wxSnprintf, "C++: wxSnprintf(char *, unsigned long, const class wxFormatString &) --> int", pybind11::arg("f1"), pybind11::arg("f2"), pybind11::arg("f3"));

	// wxDoSprintfWchar(wchar_t *, const wchar_t *) file: line:371
	M("").def("wxDoSprintfWchar", [](wchar_t * a0, const wchar_t * a1) -> int { return wxDoSprintfWchar(a0, a1); }, "", pybind11::arg("str"), pybind11::arg("format"));

	// wxSprintf(wchar_t *, const class wxFormatString &) file: line:376
	M("").def("wxSprintf", (int (*)(wchar_t *, const class wxFormatString &)) &wxSprintf, "C++: wxSprintf(wchar_t *, const class wxFormatString &) --> int", pybind11::arg("f1"), pybind11::arg("f2"));

	// wxDoSnprintfWchar(wchar_t *, unsigned long, const wchar_t *) file: line:383
	M("").def("wxDoSnprintfWchar", [](wchar_t * a0, unsigned long const & a1, const wchar_t * a2) -> int { return wxDoSnprintfWchar(a0, a1, a2); }, "", pybind11::arg("str"), pybind11::arg("size"), pybind11::arg("format"));

	// wxSnprintf(wchar_t *, unsigned long, const class wxFormatString &) file: line:388
	M("").def("wxSnprintf", (int (*)(wchar_t *, unsigned long, const class wxFormatString &)) &wxSnprintf, "C++: wxSnprintf(wchar_t *, unsigned long, const class wxFormatString &) --> int", pybind11::arg("f1"), pybind11::arg("f2"), pybind11::arg("f3"));

	// wxScanfConvertFormatW(const wchar_t *) file: line:454
	M("").def("wxScanfConvertFormatW", (const class wxScopedCharTypeBuffer<wchar_t> (*)(const wchar_t *)) &wxScanfConvertFormatW, "C++: wxScanfConvertFormatW(const wchar_t *) --> const class wxScopedCharTypeBuffer<wchar_t>", pybind11::arg("format"));

	{ // wxMessageOutputBase file: line:32
		pybind11::class_<wxMessageOutputBase, std::shared_ptr<wxMessageOutputBase>, PyCallBack_wxMessageOutputBase> cl(M(""), "wxMessageOutputBase", "");
		cl.def(pybind11::init<PyCallBack_wxMessageOutputBase const &>());
		cl.def( pybind11::init( [](){ return new PyCallBack_wxMessageOutputBase(); } ) );
		cl.def("Printf", (void (wxMessageOutputBase::*)(const class wxFormatString &)) &wxMessageOutputBase::Printf, "C++: wxMessageOutputBase::Printf(const class wxFormatString &) --> void", pybind11::arg("f1"));
		cl.def("Output", (void (wxMessageOutputBase::*)(const class wxString &)) &wxMessageOutputBase::Output, "C++: wxMessageOutputBase::Output(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("assign", (class wxMessageOutputBase & (wxMessageOutputBase::*)(const class wxMessageOutputBase &)) &wxMessageOutputBase::operator=, "C++: wxMessageOutputBase::operator=(const class wxMessageOutputBase &) --> class wxMessageOutputBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMessageOutput file: line:75
		pybind11::class_<wxMessageOutput, std::shared_ptr<wxMessageOutput>, PyCallBack_wxMessageOutput, wxMessageOutputBase> cl(M(""), "wxMessageOutput", "");
		cl.def(pybind11::init<PyCallBack_wxMessageOutput const &>());
		cl.def( pybind11::init( [](){ return new PyCallBack_wxMessageOutput(); } ) );
		cl.def_static("Get", (class wxMessageOutput * (*)()) &wxMessageOutput::Get, "C++: wxMessageOutput::Get() --> class wxMessageOutput *", pybind11::return_value_policy::automatic);
		cl.def_static("Set", (class wxMessageOutput * (*)(class wxMessageOutput *)) &wxMessageOutput::Set, "C++: wxMessageOutput::Set(class wxMessageOutput *) --> class wxMessageOutput *", pybind11::return_value_policy::automatic, pybind11::arg("msgout"));
		cl.def("assign", (class wxMessageOutput & (wxMessageOutput::*)(const class wxMessageOutput &)) &wxMessageOutput::operator=, "C++: wxMessageOutput::operator=(const class wxMessageOutput &) --> class wxMessageOutput &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMessageOutputStderr file: line:107
		pybind11::class_<wxMessageOutputStderr, std::shared_ptr<wxMessageOutputStderr>, PyCallBack_wxMessageOutputStderr, wxMessageOutput> cl(M(""), "wxMessageOutputStderr", "");
		cl.def( pybind11::init( [](){ return new wxMessageOutputStderr(); }, [](){ return new PyCallBack_wxMessageOutputStderr(); } ), "doc");
		cl.def( pybind11::init<struct _IO_FILE *>(), pybind11::arg("fp") );

		cl.def( pybind11::init( [](PyCallBack_wxMessageOutputStderr const &o){ return new PyCallBack_wxMessageOutputStderr(o); } ) );
		cl.def( pybind11::init( [](wxMessageOutputStderr const &o){ return new wxMessageOutputStderr(o); } ) );
		cl.def("Output", (void (wxMessageOutputStderr::*)(const class wxString &)) &wxMessageOutputStderr::Output, "C++: wxMessageOutputStderr::Output(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("assign", (class wxMessageOutputStderr & (wxMessageOutputStderr::*)(const class wxMessageOutputStderr &)) &wxMessageOutputStderr::operator=, "C++: wxMessageOutputStderr::operator=(const class wxMessageOutputStderr &) --> class wxMessageOutputStderr &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
