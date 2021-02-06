#include <cstdio> // _IO_FILE
#include <iterator> // __gnu_cxx::__normal_iterator
#include <ki_exception.h> // FUTURE_FORMAT_ERROR
#include <ki_exception.h> // IO_ERROR
#include <ki_exception.h> // KI_PARAM_ERROR
#include <ki_exception.h> // PARSE_ERROR
#include <memory> // std::allocator
#include <richio.h> // FILE_LINE_READER
#include <richio.h> // INPUTSTREAM_LINE_READER
#include <richio.h> // LINE_READER
#include <richio.h> // OUTPUTFORMATTER
#include <richio.h> // STRING_FORMATTER
#include <richio.h> // STRING_LINE_READER
#include <richio.h> // StrPrintf
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

// IO_ERROR file:ki_exception.h line:75
struct PyCallBack_IO_ERROR : public IO_ERROR {
	using IO_ERROR::IO_ERROR;

	const class wxString Problem() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const IO_ERROR *>(this), "Problem");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString>::value) {
				static pybind11::detail::override_caster_t<const class wxString> caster;
				return pybind11::detail::cast_ref<const class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString>(std::move(o));
		}
		return IO_ERROR::Problem();
	}
	const class wxString Where() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const IO_ERROR *>(this), "Where");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString>::value) {
				static pybind11::detail::override_caster_t<const class wxString> caster;
				return pybind11::detail::cast_ref<const class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString>(std::move(o));
		}
		return IO_ERROR::Where();
	}
	const class wxString What() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const IO_ERROR *>(this), "What");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString>::value) {
				static pybind11::detail::override_caster_t<const class wxString> caster;
				return pybind11::detail::cast_ref<const class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString>(std::move(o));
		}
		return IO_ERROR::What();
	}
};

// PARSE_ERROR file:ki_exception.h line:118
struct PyCallBack_PARSE_ERROR : public PARSE_ERROR {
	using PARSE_ERROR::PARSE_ERROR;

	const class wxString Problem() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PARSE_ERROR *>(this), "Problem");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString>::value) {
				static pybind11::detail::override_caster_t<const class wxString> caster;
				return pybind11::detail::cast_ref<const class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString>(std::move(o));
		}
		return IO_ERROR::Problem();
	}
	const class wxString Where() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PARSE_ERROR *>(this), "Where");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString>::value) {
				static pybind11::detail::override_caster_t<const class wxString> caster;
				return pybind11::detail::cast_ref<const class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString>(std::move(o));
		}
		return IO_ERROR::Where();
	}
	const class wxString What() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const PARSE_ERROR *>(this), "What");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString>::value) {
				static pybind11::detail::override_caster_t<const class wxString> caster;
				return pybind11::detail::cast_ref<const class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString>(std::move(o));
		}
		return IO_ERROR::What();
	}
};

// FUTURE_FORMAT_ERROR file:ki_exception.h line:174
struct PyCallBack_FUTURE_FORMAT_ERROR : public FUTURE_FORMAT_ERROR {
	using FUTURE_FORMAT_ERROR::FUTURE_FORMAT_ERROR;

	const class wxString Problem() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const FUTURE_FORMAT_ERROR *>(this), "Problem");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString>::value) {
				static pybind11::detail::override_caster_t<const class wxString> caster;
				return pybind11::detail::cast_ref<const class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString>(std::move(o));
		}
		return IO_ERROR::Problem();
	}
	const class wxString Where() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const FUTURE_FORMAT_ERROR *>(this), "Where");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString>::value) {
				static pybind11::detail::override_caster_t<const class wxString> caster;
				return pybind11::detail::cast_ref<const class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString>(std::move(o));
		}
		return IO_ERROR::Where();
	}
	const class wxString What() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const FUTURE_FORMAT_ERROR *>(this), "What");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString>::value) {
				static pybind11::detail::override_caster_t<const class wxString> caster;
				return pybind11::detail::cast_ref<const class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString>(std::move(o));
		}
		return IO_ERROR::What();
	}
};

// LINE_READER file:richio.h line:80
struct PyCallBack_LINE_READER : public LINE_READER {
	using LINE_READER::LINE_READER;

	char * ReadLine() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LINE_READER *>(this), "ReadLine");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<char *>::value) {
				static pybind11::detail::override_caster_t<char *> caster;
				return pybind11::detail::cast_ref<char *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<char *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"LINE_READER::ReadLine\"");
	}
	const class wxString & GetSource() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LINE_READER *>(this), "GetSource");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString &>::value) {
				static pybind11::detail::override_caster_t<const class wxString &> caster;
				return pybind11::detail::cast_ref<const class wxString &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString &>(std::move(o));
		}
		return LINE_READER::GetSource();
	}
	unsigned int LineNumber() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const LINE_READER *>(this), "LineNumber");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return LINE_READER::LineNumber();
	}
};

// FILE_LINE_READER file:richio.h line:172
struct PyCallBack_FILE_LINE_READER : public FILE_LINE_READER {
	using FILE_LINE_READER::FILE_LINE_READER;

	char * ReadLine() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const FILE_LINE_READER *>(this), "ReadLine");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<char *>::value) {
				static pybind11::detail::override_caster_t<char *> caster;
				return pybind11::detail::cast_ref<char *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<char *>(std::move(o));
		}
		return FILE_LINE_READER::ReadLine();
	}
	const class wxString & GetSource() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const FILE_LINE_READER *>(this), "GetSource");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString &>::value) {
				static pybind11::detail::override_caster_t<const class wxString &> caster;
				return pybind11::detail::cast_ref<const class wxString &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString &>(std::move(o));
		}
		return LINE_READER::GetSource();
	}
	unsigned int LineNumber() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const FILE_LINE_READER *>(this), "LineNumber");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return LINE_READER::LineNumber();
	}
};

// STRING_LINE_READER file:richio.h line:237
struct PyCallBack_STRING_LINE_READER : public STRING_LINE_READER {
	using STRING_LINE_READER::STRING_LINE_READER;

	char * ReadLine() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const STRING_LINE_READER *>(this), "ReadLine");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<char *>::value) {
				static pybind11::detail::override_caster_t<char *> caster;
				return pybind11::detail::cast_ref<char *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<char *>(std::move(o));
		}
		return STRING_LINE_READER::ReadLine();
	}
	const class wxString & GetSource() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const STRING_LINE_READER *>(this), "GetSource");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString &>::value) {
				static pybind11::detail::override_caster_t<const class wxString &> caster;
				return pybind11::detail::cast_ref<const class wxString &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString &>(std::move(o));
		}
		return LINE_READER::GetSource();
	}
	unsigned int LineNumber() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const STRING_LINE_READER *>(this), "LineNumber");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return LINE_READER::LineNumber();
	}
};

// INPUTSTREAM_LINE_READER file:richio.h line:271
struct PyCallBack_INPUTSTREAM_LINE_READER : public INPUTSTREAM_LINE_READER {
	using INPUTSTREAM_LINE_READER::INPUTSTREAM_LINE_READER;

	char * ReadLine() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const INPUTSTREAM_LINE_READER *>(this), "ReadLine");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<char *>::value) {
				static pybind11::detail::override_caster_t<char *> caster;
				return pybind11::detail::cast_ref<char *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<char *>(std::move(o));
		}
		return INPUTSTREAM_LINE_READER::ReadLine();
	}
	const class wxString & GetSource() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const INPUTSTREAM_LINE_READER *>(this), "GetSource");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString &>::value) {
				static pybind11::detail::override_caster_t<const class wxString &> caster;
				return pybind11::detail::cast_ref<const class wxString &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString &>(std::move(o));
		}
		return LINE_READER::GetSource();
	}
	unsigned int LineNumber() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const INPUTSTREAM_LINE_READER *>(this), "LineNumber");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return LINE_READER::LineNumber();
	}
};

// OUTPUTFORMATTER file:richio.h line:306
struct PyCallBack_OUTPUTFORMATTER : public OUTPUTFORMATTER {
	using OUTPUTFORMATTER::OUTPUTFORMATTER;

	void write(const char * a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const OUTPUTFORMATTER *>(this), "write");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"OUTPUTFORMATTER::write\"");
	}
	const char * GetQuoteChar(const char * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const OUTPUTFORMATTER *>(this), "GetQuoteChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<const char *>::value) {
				static pybind11::detail::override_caster_t<const char *> caster;
				return pybind11::detail::cast_ref<const char *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const char *>(std::move(o));
		}
		return OUTPUTFORMATTER::GetQuoteChar(a0);
	}
};

// STRING_FORMATTER file:richio.h line:411
struct PyCallBack_STRING_FORMATTER : public STRING_FORMATTER {
	using STRING_FORMATTER::STRING_FORMATTER;

	void write(const char * a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const STRING_FORMATTER *>(this), "write");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return STRING_FORMATTER::write(a0, a1);
	}
	const char * GetQuoteChar(const char * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const STRING_FORMATTER *>(this), "GetQuoteChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<const char *>::value) {
				static pybind11::detail::override_caster_t<const char *> caster;
				return pybind11::detail::cast_ref<const char *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const char *>(std::move(o));
		}
		return OUTPUTFORMATTER::GetQuoteChar(a0);
	}
};

void bind_ki_exception(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // KI_PARAM_ERROR file:ki_exception.h line:44
		pybind11::class_<KI_PARAM_ERROR, std::shared_ptr<KI_PARAM_ERROR>> cl(M(""), "KI_PARAM_ERROR", "Hold a translatable error message and may be used when throwing exceptions containing a\n translated error message.");
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("aMessage") );

		cl.def( pybind11::init( [](){ return new KI_PARAM_ERROR(); } ) );
		cl.def( pybind11::init( [](KI_PARAM_ERROR const &o){ return new KI_PARAM_ERROR(o); } ) );
		cl.def("What", (const class wxString (KI_PARAM_ERROR::*)() const) &KI_PARAM_ERROR::What, "C++: KI_PARAM_ERROR::What() const --> const class wxString");
		cl.def("assign", (class KI_PARAM_ERROR & (KI_PARAM_ERROR::*)(const class KI_PARAM_ERROR &)) &KI_PARAM_ERROR::operator=, "C++: KI_PARAM_ERROR::operator=(const class KI_PARAM_ERROR &) --> class KI_PARAM_ERROR &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // IO_ERROR file:ki_exception.h line:75
		pybind11::class_<IO_ERROR, std::shared_ptr<IO_ERROR>, PyCallBack_IO_ERROR> cl(M(""), "IO_ERROR", "Hold an error message and may be used when throwing exceptions containing meaningful\n error messages.\n\n \n Dick Hollenbeck");
		cl.def( pybind11::init<const class wxString &, const char *, const char *, int>(), pybind11::arg("aProblem"), pybind11::arg("aThrowersFile"), pybind11::arg("aThrowersFunction"), pybind11::arg("aThrowersLineNumber") );

		cl.def( pybind11::init( [](){ return new IO_ERROR(); }, [](){ return new PyCallBack_IO_ERROR(); } ) );
		cl.def( pybind11::init( [](PyCallBack_IO_ERROR const &o){ return new PyCallBack_IO_ERROR(o); } ) );
		cl.def( pybind11::init( [](IO_ERROR const &o){ return new IO_ERROR(o); } ) );
		cl.def("init", (void (IO_ERROR::*)(const class wxString &, const char *, const char *, int)) &IO_ERROR::init, "C++: IO_ERROR::init(const class wxString &, const char *, const char *, int) --> void", pybind11::arg("aProblem"), pybind11::arg("aThrowersFile"), pybind11::arg("aThrowersFunction"), pybind11::arg("aThrowersLineNumber"));
		cl.def("Problem", (const class wxString (IO_ERROR::*)() const) &IO_ERROR::Problem, "C++: IO_ERROR::Problem() const --> const class wxString");
		cl.def("Where", (const class wxString (IO_ERROR::*)() const) &IO_ERROR::Where, "C++: IO_ERROR::Where() const --> const class wxString");
		cl.def("What", (const class wxString (IO_ERROR::*)() const) &IO_ERROR::What, "C++: IO_ERROR::What() const --> const class wxString");
		cl.def("assign", (class IO_ERROR & (IO_ERROR::*)(const class IO_ERROR &)) &IO_ERROR::operator=, "C++: IO_ERROR::operator=(const class IO_ERROR &) --> class IO_ERROR &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // PARSE_ERROR file:ki_exception.h line:118
		pybind11::class_<PARSE_ERROR, std::shared_ptr<PARSE_ERROR>, PyCallBack_PARSE_ERROR, IO_ERROR> cl(M(""), "PARSE_ERROR", "A filename or source description, a problem input line, a line number, a byte\n offset, and an error message which contains the the caller's report and his call\n site information: CPP source file, function, and line number.\n\n \n Dick Hollenbeck");
		cl.def( pybind11::init<const class wxString &, const char *, const char *, int, const class wxString &, const char *, int, int>(), pybind11::arg("aProblem"), pybind11::arg("aThrowersFile"), pybind11::arg("aThrowersFunction"), pybind11::arg("aThrowersLineNumber"), pybind11::arg("aSource"), pybind11::arg("aInputLine"), pybind11::arg("aLineNumber"), pybind11::arg("aByteIndex") );

		cl.def( pybind11::init( [](PyCallBack_PARSE_ERROR const &o){ return new PyCallBack_PARSE_ERROR(o); } ) );
		cl.def( pybind11::init( [](PARSE_ERROR const &o){ return new PARSE_ERROR(o); } ) );
		cl.def_readwrite("lineNumber", &PARSE_ERROR::lineNumber);
		cl.def_readwrite("byteIndex", &PARSE_ERROR::byteIndex);
		cl.def_readwrite("inputLine", &PARSE_ERROR::inputLine);
		cl.def("init", (void (PARSE_ERROR::*)(const class wxString &, const char *, const char *, int, const class wxString &, const char *, int, int)) &PARSE_ERROR::init, "C++: PARSE_ERROR::init(const class wxString &, const char *, const char *, int, const class wxString &, const char *, int, int) --> void", pybind11::arg("aProblem"), pybind11::arg("aThrowersFile"), pybind11::arg("aThrowersFunction"), pybind11::arg("aThrowersLineNumber"), pybind11::arg("aSource"), pybind11::arg("aInputLine"), pybind11::arg("aLineNumber"), pybind11::arg("aByteIndex"));
		cl.def("ParseProblem", (const class wxString (PARSE_ERROR::*)()) &PARSE_ERROR::ParseProblem, "C++: PARSE_ERROR::ParseProblem() --> const class wxString");
		cl.def("assign", (struct PARSE_ERROR & (PARSE_ERROR::*)(const struct PARSE_ERROR &)) &PARSE_ERROR::operator=, "C++: PARSE_ERROR::operator=(const struct PARSE_ERROR &) --> struct PARSE_ERROR &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // FUTURE_FORMAT_ERROR file:ki_exception.h line:174
		pybind11::class_<FUTURE_FORMAT_ERROR, std::shared_ptr<FUTURE_FORMAT_ERROR>, PyCallBack_FUTURE_FORMAT_ERROR, PARSE_ERROR> cl(M(""), "FUTURE_FORMAT_ERROR", "Variant of #PARSE_ERROR indicating that a syntax or related error was likely caused\n by a file generated by a newer version of KiCad than this. This can be used to generate\n more informative error messages.");
		cl.def( pybind11::init<const struct PARSE_ERROR &, const class wxString &>(), pybind11::arg("aParseError"), pybind11::arg("aRequiredVersion") );

		cl.def( pybind11::init( [](PyCallBack_FUTURE_FORMAT_ERROR const &o){ return new PyCallBack_FUTURE_FORMAT_ERROR(o); } ) );
		cl.def( pybind11::init( [](FUTURE_FORMAT_ERROR const &o){ return new FUTURE_FORMAT_ERROR(o); } ) );
		cl.def_readwrite("requiredVersion", &FUTURE_FORMAT_ERROR::requiredVersion);
		cl.def("assign", (struct FUTURE_FORMAT_ERROR & (FUTURE_FORMAT_ERROR::*)(const struct FUTURE_FORMAT_ERROR &)) &FUTURE_FORMAT_ERROR::operator=, "C++: FUTURE_FORMAT_ERROR::operator=(const struct FUTURE_FORMAT_ERROR &) --> struct FUTURE_FORMAT_ERROR &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // LINE_READER file:richio.h line:80
		pybind11::class_<LINE_READER, std::shared_ptr<LINE_READER>, PyCallBack_LINE_READER> cl(M(""), "LINE_READER", "An abstract class from which implementation specific LINE_READERs may be derived to\n read single lines of text and manage a line number counter.");
		cl.def( pybind11::init( [](){ return new PyCallBack_LINE_READER(); } ), "doc");
		cl.def( pybind11::init<unsigned int>(), pybind11::arg("aMaxLineLength") );

		cl.def(pybind11::init<PyCallBack_LINE_READER const &>());
		cl.def("ReadLine", (char * (LINE_READER::*)()) &LINE_READER::ReadLine, "Read a line of text into the buffer and increments the line number counter.\n\n If the line is larger than the maximum length passed to the constructor, then an\n exception is thrown.  The line is nul terminated.\n\n \n The beginning of the read line, or NULL if EOF.\n \n\n IO_ERROR when a line is too long.\n\nC++: LINE_READER::ReadLine() --> char *", pybind11::return_value_policy::automatic);
		cl.def("GetSource", (const class wxString & (LINE_READER::*)() const) &LINE_READER::GetSource, "Returns the name of the source of the lines in an abstract sense.\n\n This may be a file or it may be the clipboard or any other source of lines of text.\n The returned string is useful for reporting error messages.\n\nC++: LINE_READER::GetSource() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("Line", (char * (LINE_READER::*)() const) &LINE_READER::Line, "Return a pointer to the last line that was read in.\n\nC++: LINE_READER::Line() const --> char *", pybind11::return_value_policy::automatic);
		cl.def("LineNumber", (unsigned int (LINE_READER::*)() const) &LINE_READER::LineNumber, "Return the line number of the last line read from this LINE_READER.\n\n Lines start from 1.\n\nC++: LINE_READER::LineNumber() const --> unsigned int");
		cl.def("Length", (unsigned int (LINE_READER::*)() const) &LINE_READER::Length, "Return the number of bytes in the last line read from this LINE_READER.\n\nC++: LINE_READER::Length() const --> unsigned int");
		cl.def("assign", (class LINE_READER & (LINE_READER::*)(const class LINE_READER &)) &LINE_READER::operator=, "C++: LINE_READER::operator=(const class LINE_READER &) --> class LINE_READER &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // FILE_LINE_READER file:richio.h line:172
		pybind11::class_<FILE_LINE_READER, std::shared_ptr<FILE_LINE_READER>, PyCallBack_FILE_LINE_READER, LINE_READER> cl(M(""), "FILE_LINE_READER", "A LINE_READER that reads from an open file.\n\n File must be already open so that this class can exist without any UI policy.");
		cl.def( pybind11::init( [](const class wxString & a0){ return new FILE_LINE_READER(a0); }, [](const class wxString & a0){ return new PyCallBack_FILE_LINE_READER(a0); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, unsigned int const & a1){ return new FILE_LINE_READER(a0, a1); }, [](const class wxString & a0, unsigned int const & a1){ return new PyCallBack_FILE_LINE_READER(a0, a1); } ), "doc");
		cl.def( pybind11::init<const class wxString &, unsigned int, unsigned int>(), pybind11::arg("aFileName"), pybind11::arg("aStartingLineNumber"), pybind11::arg("aMaxLineLength") );

		cl.def( pybind11::init( [](struct _IO_FILE * a0, const class wxString & a1){ return new FILE_LINE_READER(a0, a1); }, [](struct _IO_FILE * a0, const class wxString & a1){ return new PyCallBack_FILE_LINE_READER(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](struct _IO_FILE * a0, const class wxString & a1, bool const & a2){ return new FILE_LINE_READER(a0, a1, a2); }, [](struct _IO_FILE * a0, const class wxString & a1, bool const & a2){ return new PyCallBack_FILE_LINE_READER(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](struct _IO_FILE * a0, const class wxString & a1, bool const & a2, unsigned int const & a3){ return new FILE_LINE_READER(a0, a1, a2, a3); }, [](struct _IO_FILE * a0, const class wxString & a1, bool const & a2, unsigned int const & a3){ return new PyCallBack_FILE_LINE_READER(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<struct _IO_FILE *, const class wxString &, bool, unsigned int, unsigned int>(), pybind11::arg("aFile"), pybind11::arg("aFileName"), pybind11::arg("doOwn"), pybind11::arg("aStartingLineNumber"), pybind11::arg("aMaxLineLength") );

		cl.def( pybind11::init( [](PyCallBack_FILE_LINE_READER const &o){ return new PyCallBack_FILE_LINE_READER(o); } ) );
		cl.def( pybind11::init( [](FILE_LINE_READER const &o){ return new FILE_LINE_READER(o); } ) );
		cl.def("ReadLine", (char * (FILE_LINE_READER::*)()) &FILE_LINE_READER::ReadLine, "C++: FILE_LINE_READER::ReadLine() --> char *", pybind11::return_value_policy::automatic);
		cl.def("Rewind", (void (FILE_LINE_READER::*)()) &FILE_LINE_READER::Rewind, "Rewind the file and resets the line number back to zero.\n\n Line number will go to 1 on first ReadLine().\n\nC++: FILE_LINE_READER::Rewind() --> void");
		cl.def("assign", (class FILE_LINE_READER & (FILE_LINE_READER::*)(const class FILE_LINE_READER &)) &FILE_LINE_READER::operator=, "C++: FILE_LINE_READER::operator=(const class FILE_LINE_READER &) --> class FILE_LINE_READER &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // STRING_LINE_READER file:richio.h line:237
		pybind11::class_<STRING_LINE_READER, std::shared_ptr<STRING_LINE_READER>, PyCallBack_STRING_LINE_READER, LINE_READER> cl(M(""), "STRING_LINE_READER", "Is a #LINE_READER that reads from a multiline 8 bit wide std::string");
		cl.def( pybind11::init( [](PyCallBack_STRING_LINE_READER const &o){ return new PyCallBack_STRING_LINE_READER(o); } ) );
		cl.def( pybind11::init( [](STRING_LINE_READER const &o){ return new STRING_LINE_READER(o); } ) );
		cl.def("ReadLine", (char * (STRING_LINE_READER::*)()) &STRING_LINE_READER::ReadLine, "C++: STRING_LINE_READER::ReadLine() --> char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class STRING_LINE_READER & (STRING_LINE_READER::*)(const class STRING_LINE_READER &)) &STRING_LINE_READER::operator=, "C++: STRING_LINE_READER::operator=(const class STRING_LINE_READER &) --> class STRING_LINE_READER &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // INPUTSTREAM_LINE_READER file:richio.h line:271
		pybind11::class_<INPUTSTREAM_LINE_READER, std::shared_ptr<INPUTSTREAM_LINE_READER>, PyCallBack_INPUTSTREAM_LINE_READER, LINE_READER> cl(M(""), "INPUTSTREAM_LINE_READER", "A #LINE_READER that reads from a wxInputStream object.");
		cl.def( pybind11::init<class wxInputStream *, const class wxString &>(), pybind11::arg("aStream"), pybind11::arg("aSource") );

		cl.def( pybind11::init( [](PyCallBack_INPUTSTREAM_LINE_READER const &o){ return new PyCallBack_INPUTSTREAM_LINE_READER(o); } ) );
		cl.def( pybind11::init( [](INPUTSTREAM_LINE_READER const &o){ return new INPUTSTREAM_LINE_READER(o); } ) );
		cl.def("ReadLine", (char * (INPUTSTREAM_LINE_READER::*)()) &INPUTSTREAM_LINE_READER::ReadLine, "C++: INPUTSTREAM_LINE_READER::ReadLine() --> char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class INPUTSTREAM_LINE_READER & (INPUTSTREAM_LINE_READER::*)(const class INPUTSTREAM_LINE_READER &)) &INPUTSTREAM_LINE_READER::operator=, "C++: INPUTSTREAM_LINE_READER::operator=(const class INPUTSTREAM_LINE_READER &) --> class INPUTSTREAM_LINE_READER &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // OUTPUTFORMATTER file:richio.h line:306
		pybind11::class_<OUTPUTFORMATTER, OUTPUTFORMATTER*, PyCallBack_OUTPUTFORMATTER> cl(M(""), "OUTPUTFORMATTER", "An interface used to output 8 bit text in a convenient way.\n\n The primary interface is \"printf() - like\" but with support for indentation control.  The\n destination of the 8 bit wide text is up to the implementer.\n \n The implementer only has to implement the write() function, but can also optionally\n re-implement GetQuoteChar().\n \n If you want to output a wxString, then use TO_UTF8() on it before passing it as an\n argument to Print().\n \n Since this is an abstract interface, only classes derived from this one may actually be\n used.");
		cl.def(pybind11::init<PyCallBack_OUTPUTFORMATTER const &>());
		cl.def("Print", [](OUTPUTFORMATTER &o, int const & a0, const char * a1) -> int { return o.Print(a0, a1); }, "", pybind11::arg("nestLevel"), pybind11::arg("fmt"));
		cl.def("GetQuoteChar", (const char * (OUTPUTFORMATTER::*)(const char *) const) &OUTPUTFORMATTER::GetQuoteChar, "Perform quote character need determination.\n\n It returns the quote character as a single character string for a given input wrapee\n string.  If the wrappee does not need to be quoted, the return value is \"\" (the null\n string), such as when there are no delimiters in the input wrapee string.  If you want\n the quote_char to be assuredly not \"\", then pass in \"(\" as the wrappee.\n \n Implementations are free to override the default behavior, which is to call the static\n function of the same name.\n\n \n A string that might need wrapping on each end.\n \n\n the quote_char as a single character string, or \"\" if the wrapee does not need\n         to be wrapped.\n\nC++: OUTPUTFORMATTER::GetQuoteChar(const char *) const --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("wrapee"));
		cl.def("assign", (class OUTPUTFORMATTER & (OUTPUTFORMATTER::*)(const class OUTPUTFORMATTER &)) &OUTPUTFORMATTER::operator=, "C++: OUTPUTFORMATTER::operator=(const class OUTPUTFORMATTER &) --> class OUTPUTFORMATTER &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // STRING_FORMATTER file:richio.h line:411
		pybind11::class_<STRING_FORMATTER, std::shared_ptr<STRING_FORMATTER>, PyCallBack_STRING_FORMATTER, OUTPUTFORMATTER> cl(M(""), "STRING_FORMATTER", "Implement an #OUTPUTFORMATTER to a memory buffer.\n\n After Print()ing the string is available through GetString()");
		cl.def( pybind11::init( [](){ return new STRING_FORMATTER(); }, [](){ return new PyCallBack_STRING_FORMATTER(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new STRING_FORMATTER(a0); }, [](int const & a0){ return new PyCallBack_STRING_FORMATTER(a0); } ), "doc");
		cl.def( pybind11::init<int, char>(), pybind11::arg("aReserve"), pybind11::arg("aQuoteChar") );

		cl.def( pybind11::init( [](PyCallBack_STRING_FORMATTER const &o){ return new PyCallBack_STRING_FORMATTER(o); } ) );
		cl.def( pybind11::init( [](STRING_FORMATTER const &o){ return new STRING_FORMATTER(o); } ) );
		cl.def("Clear", (void (STRING_FORMATTER::*)()) &STRING_FORMATTER::Clear, "Clear the buffer and empties the internal string.\n\nC++: STRING_FORMATTER::Clear() --> void");
		cl.def("StripUseless", (void (STRING_FORMATTER::*)()) &STRING_FORMATTER::StripUseless, "Removes whitespace, '(', and ')' from the string.\n\nC++: STRING_FORMATTER::StripUseless() --> void");
		cl.def("assign", (class STRING_FORMATTER & (STRING_FORMATTER::*)(const class STRING_FORMATTER &)) &STRING_FORMATTER::operator=, "C++: STRING_FORMATTER::operator=(const class STRING_FORMATTER &) --> class STRING_FORMATTER &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
