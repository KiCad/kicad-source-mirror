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

// wxTextFile file: line:28
struct PyCallBack_wxTextFile : public wxTextFile {
	using wxTextFile::wxTextFile;

	bool OnExists() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTextFile *>(this), "OnExists");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTextFile::OnExists();
	}
	bool OnClose() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTextFile *>(this), "OnClose");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTextFile::OnClose();
	}
	bool OnRead(const class wxMBConv & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTextFile *>(this), "OnRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTextFile::OnRead(a0);
	}
	bool OnWrite(enum wxTextFileType a0, const class wxMBConv & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTextFile *>(this), "OnWrite");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTextFile::OnWrite(a0, a1);
	}
};

void bind_unknown_unknown_81(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxTextBuffer file: line:49
		pybind11::class_<wxTextBuffer, std::shared_ptr<wxTextBuffer>> cl(M(""), "wxTextBuffer", "");
		cl.def_static("Translate", [](const class wxString & a0) -> wxString { return wxTextBuffer::Translate(a0); }, "", pybind11::arg("text"));
		cl.def_static("Translate", (class wxString (*)(const class wxString &, enum wxTextFileType)) &wxTextBuffer::Translate, "C++: wxTextBuffer::Translate(const class wxString &, enum wxTextFileType) --> class wxString", pybind11::arg("text"), pybind11::arg("type"));
		cl.def_static("GetEOL", []() -> const wchar_t * { return wxTextBuffer::GetEOL(); }, "", pybind11::return_value_policy::automatic);
		cl.def_static("GetEOL", (const wchar_t * (*)(enum wxTextFileType)) &wxTextBuffer::GetEOL, "C++: wxTextBuffer::GetEOL(enum wxTextFileType) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("type"));
		cl.def("Exists", (bool (wxTextBuffer::*)() const) &wxTextBuffer::Exists, "C++: wxTextBuffer::Exists() const --> bool");
		cl.def("Create", (bool (wxTextBuffer::*)()) &wxTextBuffer::Create, "C++: wxTextBuffer::Create() --> bool");
		cl.def("Create", (bool (wxTextBuffer::*)(const class wxString &)) &wxTextBuffer::Create, "C++: wxTextBuffer::Create(const class wxString &) --> bool", pybind11::arg("strBufferName"));
		cl.def("Open", [](wxTextBuffer &o) -> bool { return o.Open(); }, "");
		cl.def("Open", (bool (wxTextBuffer::*)(const class wxMBConv &)) &wxTextBuffer::Open, "C++: wxTextBuffer::Open(const class wxMBConv &) --> bool", pybind11::arg("conv"));
		cl.def("Open", [](wxTextBuffer &o, const class wxString & a0) -> bool { return o.Open(a0); }, "", pybind11::arg("strBufferName"));
		cl.def("Open", (bool (wxTextBuffer::*)(const class wxString &, const class wxMBConv &)) &wxTextBuffer::Open, "C++: wxTextBuffer::Open(const class wxString &, const class wxMBConv &) --> bool", pybind11::arg("strBufferName"), pybind11::arg("conv"));
		cl.def("Close", (bool (wxTextBuffer::*)()) &wxTextBuffer::Close, "C++: wxTextBuffer::Close() --> bool");
		cl.def("IsOpened", (bool (wxTextBuffer::*)() const) &wxTextBuffer::IsOpened, "C++: wxTextBuffer::IsOpened() const --> bool");
		cl.def("GetLineCount", (unsigned long (wxTextBuffer::*)() const) &wxTextBuffer::GetLineCount, "C++: wxTextBuffer::GetLineCount() const --> unsigned long");
		cl.def("GetLine", (class wxString & (wxTextBuffer::*)(unsigned long)) &wxTextBuffer::GetLine, "C++: wxTextBuffer::GetLine(unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__getitem__", (class wxString & (wxTextBuffer::*)(unsigned long)) &wxTextBuffer::operator[], "C++: wxTextBuffer::operator[](unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("GetCurrentLine", (unsigned long (wxTextBuffer::*)() const) &wxTextBuffer::GetCurrentLine, "C++: wxTextBuffer::GetCurrentLine() const --> unsigned long");
		cl.def("GoToLine", (void (wxTextBuffer::*)(unsigned long)) &wxTextBuffer::GoToLine, "C++: wxTextBuffer::GoToLine(unsigned long) --> void", pybind11::arg("n"));
		cl.def("Eof", (bool (wxTextBuffer::*)() const) &wxTextBuffer::Eof, "C++: wxTextBuffer::Eof() const --> bool");
		cl.def("GetFirstLine", (class wxString & (wxTextBuffer::*)()) &wxTextBuffer::GetFirstLine, "C++: wxTextBuffer::GetFirstLine() --> class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetNextLine", (class wxString & (wxTextBuffer::*)()) &wxTextBuffer::GetNextLine, "C++: wxTextBuffer::GetNextLine() --> class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetPrevLine", (class wxString & (wxTextBuffer::*)()) &wxTextBuffer::GetPrevLine, "C++: wxTextBuffer::GetPrevLine() --> class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetLastLine", (class wxString & (wxTextBuffer::*)()) &wxTextBuffer::GetLastLine, "C++: wxTextBuffer::GetLastLine() --> class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetLineType", (enum wxTextFileType (wxTextBuffer::*)(unsigned long) const) &wxTextBuffer::GetLineType, "C++: wxTextBuffer::GetLineType(unsigned long) const --> enum wxTextFileType", pybind11::arg("n"));
		cl.def("GuessType", (enum wxTextFileType (wxTextBuffer::*)() const) &wxTextBuffer::GuessType, "C++: wxTextBuffer::GuessType() const --> enum wxTextFileType");
		cl.def("GetName", (const class wxString & (wxTextBuffer::*)() const) &wxTextBuffer::GetName, "C++: wxTextBuffer::GetName() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("AddLine", [](wxTextBuffer &o, const class wxString & a0) -> void { return o.AddLine(a0); }, "", pybind11::arg("str"));
		cl.def("AddLine", (void (wxTextBuffer::*)(const class wxString &, enum wxTextFileType)) &wxTextBuffer::AddLine, "C++: wxTextBuffer::AddLine(const class wxString &, enum wxTextFileType) --> void", pybind11::arg("str"), pybind11::arg("type"));
		cl.def("InsertLine", [](wxTextBuffer &o, const class wxString & a0, unsigned long const & a1) -> void { return o.InsertLine(a0, a1); }, "", pybind11::arg("str"), pybind11::arg("n"));
		cl.def("InsertLine", (void (wxTextBuffer::*)(const class wxString &, unsigned long, enum wxTextFileType)) &wxTextBuffer::InsertLine, "C++: wxTextBuffer::InsertLine(const class wxString &, unsigned long, enum wxTextFileType) --> void", pybind11::arg("str"), pybind11::arg("n"), pybind11::arg("type"));
		cl.def("RemoveLine", (void (wxTextBuffer::*)(unsigned long)) &wxTextBuffer::RemoveLine, "C++: wxTextBuffer::RemoveLine(unsigned long) --> void", pybind11::arg("n"));
		cl.def("Clear", (void (wxTextBuffer::*)()) &wxTextBuffer::Clear, "C++: wxTextBuffer::Clear() --> void");
		cl.def("Write", [](wxTextBuffer &o) -> bool { return o.Write(); }, "");
		cl.def("Write", [](wxTextBuffer &o, enum wxTextFileType const & a0) -> bool { return o.Write(a0); }, "", pybind11::arg("typeNew"));
		cl.def("Write", (bool (wxTextBuffer::*)(enum wxTextFileType, const class wxMBConv &)) &wxTextBuffer::Write, "C++: wxTextBuffer::Write(enum wxTextFileType, const class wxMBConv &) --> bool", pybind11::arg("typeNew"), pybind11::arg("conv"));
	}
	{ // wxFile file: line:30
		pybind11::class_<wxFile, std::shared_ptr<wxFile>> cl(M(""), "wxFile", "");
		cl.def( pybind11::init( [](){ return new wxFile(); } ) );
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxFile(a0); } ), "doc" , pybind11::arg("fileName"));
		cl.def( pybind11::init<const class wxString &, enum wxFile::OpenMode>(), pybind11::arg("fileName"), pybind11::arg("mode") );

		cl.def( pybind11::init<int>(), pybind11::arg("lfd") );


		pybind11::enum_<wxFile::OpenMode>(cl, "OpenMode", pybind11::arithmetic(), "")
			.value("read", wxFile::read)
			.value("write", wxFile::write)
			.value("read_write", wxFile::read_write)
			.value("write_append", wxFile::write_append)
			.value("write_excl", wxFile::write_excl)
			.export_values();

		cl.def_static("Exists", (bool (*)(const class wxString &)) &wxFile::Exists, "C++: wxFile::Exists(const class wxString &) --> bool", pybind11::arg("name"));
		cl.def_static("Access", (bool (*)(const class wxString &, enum wxFile::OpenMode)) &wxFile::Access, "C++: wxFile::Access(const class wxString &, enum wxFile::OpenMode) --> bool", pybind11::arg("name"), pybind11::arg("mode"));
		cl.def("Create", [](wxFile &o, const class wxString & a0) -> bool { return o.Create(a0); }, "", pybind11::arg("fileName"));
		cl.def("Create", [](wxFile &o, const class wxString & a0, bool const & a1) -> bool { return o.Create(a0, a1); }, "", pybind11::arg("fileName"), pybind11::arg("bOverwrite"));
		cl.def("Create", (bool (wxFile::*)(const class wxString &, bool, int)) &wxFile::Create, "C++: wxFile::Create(const class wxString &, bool, int) --> bool", pybind11::arg("fileName"), pybind11::arg("bOverwrite"), pybind11::arg("access"));
		cl.def("Open", [](wxFile &o, const class wxString & a0) -> bool { return o.Open(a0); }, "", pybind11::arg("fileName"));
		cl.def("Open", [](wxFile &o, const class wxString & a0, enum wxFile::OpenMode const & a1) -> bool { return o.Open(a0, a1); }, "", pybind11::arg("fileName"), pybind11::arg("mode"));
		cl.def("Open", (bool (wxFile::*)(const class wxString &, enum wxFile::OpenMode, int)) &wxFile::Open, "C++: wxFile::Open(const class wxString &, enum wxFile::OpenMode, int) --> bool", pybind11::arg("fileName"), pybind11::arg("mode"), pybind11::arg("access"));
		cl.def("Close", (bool (wxFile::*)()) &wxFile::Close, "C++: wxFile::Close() --> bool");
		cl.def("Attach", (void (wxFile::*)(int)) &wxFile::Attach, "C++: wxFile::Attach(int) --> void", pybind11::arg("lfd"));
		cl.def("Detach", (int (wxFile::*)()) &wxFile::Detach, "C++: wxFile::Detach() --> int");
		cl.def("fd", (int (wxFile::*)() const) &wxFile::fd, "C++: wxFile::fd() const --> int");
		cl.def("ReadAll", [](wxFile &o, class wxString * a0) -> bool { return o.ReadAll(a0); }, "", pybind11::arg("str"));
		cl.def("ReadAll", (bool (wxFile::*)(class wxString *, const class wxMBConv &)) &wxFile::ReadAll, "C++: wxFile::ReadAll(class wxString *, const class wxMBConv &) --> bool", pybind11::arg("str"), pybind11::arg("conv"));
		cl.def("Read", (long (wxFile::*)(void *, unsigned long)) &wxFile::Read, "C++: wxFile::Read(void *, unsigned long) --> long", pybind11::arg("pBuf"), pybind11::arg("nCount"));
		cl.def("Write", (unsigned long (wxFile::*)(const void *, unsigned long)) &wxFile::Write, "C++: wxFile::Write(const void *, unsigned long) --> unsigned long", pybind11::arg("pBuf"), pybind11::arg("nCount"));
		cl.def("Write", [](wxFile &o, const class wxString & a0) -> bool { return o.Write(a0); }, "", pybind11::arg("s"));
		cl.def("Write", (bool (wxFile::*)(const class wxString &, const class wxMBConv &)) &wxFile::Write, "C++: wxFile::Write(const class wxString &, const class wxMBConv &) --> bool", pybind11::arg("s"), pybind11::arg("conv"));
		cl.def("Flush", (bool (wxFile::*)()) &wxFile::Flush, "C++: wxFile::Flush() --> bool");
		cl.def("Seek", [](wxFile &o, long const & a0) -> long { return o.Seek(a0); }, "", pybind11::arg("ofs"));
		cl.def("Seek", (long (wxFile::*)(long, enum wxSeekMode)) &wxFile::Seek, "C++: wxFile::Seek(long, enum wxSeekMode) --> long", pybind11::arg("ofs"), pybind11::arg("mode"));
		cl.def("SeekEnd", [](wxFile &o) -> long { return o.SeekEnd(); }, "");
		cl.def("SeekEnd", (long (wxFile::*)(long)) &wxFile::SeekEnd, "C++: wxFile::SeekEnd(long) --> long", pybind11::arg("ofs"));
		cl.def("Tell", (long (wxFile::*)() const) &wxFile::Tell, "C++: wxFile::Tell() const --> long");
		cl.def("Length", (long (wxFile::*)() const) &wxFile::Length, "C++: wxFile::Length() const --> long");
		cl.def("IsOpened", (bool (wxFile::*)() const) &wxFile::IsOpened, "C++: wxFile::IsOpened() const --> bool");
		cl.def("Eof", (bool (wxFile::*)() const) &wxFile::Eof, "C++: wxFile::Eof() const --> bool");
		cl.def("Error", (bool (wxFile::*)() const) &wxFile::Error, "C++: wxFile::Error() const --> bool");
		cl.def("GetLastError", (int (wxFile::*)() const) &wxFile::GetLastError, "C++: wxFile::GetLastError() const --> int");
		cl.def("ClearLastError", (void (wxFile::*)()) &wxFile::ClearLastError, "C++: wxFile::ClearLastError() --> void");
		cl.def("GetKind", (enum wxFileKind (wxFile::*)() const) &wxFile::GetKind, "C++: wxFile::GetKind() const --> enum wxFileKind");
	}
	{ // wxTempFile file: line:138
		pybind11::class_<wxTempFile, std::shared_ptr<wxTempFile>> cl(M(""), "wxTempFile", "");
		cl.def( pybind11::init( [](){ return new wxTempFile(); } ) );
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("strName") );

		cl.def("Open", (bool (wxTempFile::*)(const class wxString &)) &wxTempFile::Open, "C++: wxTempFile::Open(const class wxString &) --> bool", pybind11::arg("strName"));
		cl.def("IsOpened", (bool (wxTempFile::*)() const) &wxTempFile::IsOpened, "C++: wxTempFile::IsOpened() const --> bool");
		cl.def("Length", (long (wxTempFile::*)() const) &wxTempFile::Length, "C++: wxTempFile::Length() const --> long");
		cl.def("Seek", [](wxTempFile &o, long const & a0) -> long { return o.Seek(a0); }, "", pybind11::arg("ofs"));
		cl.def("Seek", (long (wxTempFile::*)(long, enum wxSeekMode)) &wxTempFile::Seek, "C++: wxTempFile::Seek(long, enum wxSeekMode) --> long", pybind11::arg("ofs"), pybind11::arg("mode"));
		cl.def("Tell", (long (wxTempFile::*)() const) &wxTempFile::Tell, "C++: wxTempFile::Tell() const --> long");
		cl.def("Write", (bool (wxTempFile::*)(const void *, unsigned long)) &wxTempFile::Write, "C++: wxTempFile::Write(const void *, unsigned long) --> bool", pybind11::arg("p"), pybind11::arg("n"));
		cl.def("Write", [](wxTempFile &o, const class wxString & a0) -> bool { return o.Write(a0); }, "", pybind11::arg("str"));
		cl.def("Write", (bool (wxTempFile::*)(const class wxString &, const class wxMBConv &)) &wxTempFile::Write, "C++: wxTempFile::Write(const class wxString &, const class wxMBConv &) --> bool", pybind11::arg("str"), pybind11::arg("conv"));
		cl.def("Flush", (bool (wxTempFile::*)()) &wxTempFile::Flush, "C++: wxTempFile::Flush() --> bool");
		cl.def("Commit", (bool (wxTempFile::*)()) &wxTempFile::Commit, "C++: wxTempFile::Commit() --> bool");
		cl.def("Discard", (void (wxTempFile::*)()) &wxTempFile::Discard, "C++: wxTempFile::Discard() --> void");
	}
	{ // wxTextFile file: line:28
		pybind11::class_<wxTextFile, std::shared_ptr<wxTextFile>, PyCallBack_wxTextFile, wxTextBuffer> cl(M(""), "wxTextFile", "");
		cl.def( pybind11::init( [](){ return new wxTextFile(); }, [](){ return new PyCallBack_wxTextFile(); } ) );
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("strFileName") );

	}
	// wxPathFormat file: line:42
	pybind11::enum_<wxPathFormat>(M(""), "wxPathFormat", pybind11::arithmetic(), "")
		.value("wxPATH_NATIVE", wxPATH_NATIVE)
		.value("wxPATH_UNIX", wxPATH_UNIX)
		.value("wxPATH_BEOS", wxPATH_BEOS)
		.value("wxPATH_MAC", wxPATH_MAC)
		.value("wxPATH_DOS", wxPATH_DOS)
		.value("wxPATH_WIN", wxPATH_WIN)
		.value("wxPATH_OS2", wxPATH_OS2)
		.value("wxPATH_VMS", wxPATH_VMS)
		.value("wxPATH_MAX", wxPATH_MAX)
		.export_values();

;

	// wxSizeConvention file: line:57
	pybind11::enum_<wxSizeConvention>(M(""), "wxSizeConvention", pybind11::arithmetic(), "")
		.value("wxSIZE_CONV_TRADITIONAL", wxSIZE_CONV_TRADITIONAL)
		.value("wxSIZE_CONV_IEC", wxSIZE_CONV_IEC)
		.value("wxSIZE_CONV_SI", wxSIZE_CONV_SI)
		.export_values();

;

	// wxPathNormalize file: line:66
	pybind11::enum_<wxPathNormalize>(M(""), "wxPathNormalize", pybind11::arithmetic(), "")
		.value("wxPATH_NORM_ENV_VARS", wxPATH_NORM_ENV_VARS)
		.value("wxPATH_NORM_DOTS", wxPATH_NORM_DOTS)
		.value("wxPATH_NORM_TILDE", wxPATH_NORM_TILDE)
		.value("wxPATH_NORM_CASE", wxPATH_NORM_CASE)
		.value("wxPATH_NORM_ABSOLUTE", wxPATH_NORM_ABSOLUTE)
		.value("wxPATH_NORM_LONG", wxPATH_NORM_LONG)
		.value("wxPATH_NORM_SHORTCUT", wxPATH_NORM_SHORTCUT)
		.value("wxPATH_NORM_ALL", wxPATH_NORM_ALL)
		.export_values();

;

	{ // wxFileName file: line:126
		pybind11::class_<wxFileName, std::shared_ptr<wxFileName>> cl(M(""), "wxFileName", "");
		cl.def( pybind11::init( [](){ return new wxFileName(); } ) );
		cl.def( pybind11::init( [](wxFileName const &o){ return new wxFileName(o); } ) );
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxFileName(a0); } ), "doc" , pybind11::arg("fullpath"));
		cl.def( pybind11::init<const class wxString &, enum wxPathFormat>(), pybind11::arg("fullpath"), pybind11::arg("format") );

		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1){ return new wxFileName(a0, a1); } ), "doc" , pybind11::arg("path"), pybind11::arg("name"));
		cl.def( pybind11::init<const class wxString &, const class wxString &, enum wxPathFormat>(), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("format") );

		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1, const class wxString & a2, const class wxString & a3){ return new wxFileName(a0, a1, a2, a3); } ), "doc" , pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"));
		cl.def( pybind11::init<const class wxString &, const class wxString &, const class wxString &, const class wxString &, enum wxPathFormat>(), pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"), pybind11::arg("format") );

		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1, const class wxString & a2){ return new wxFileName(a0, a1, a2); } ), "doc" , pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"));
		cl.def( pybind11::init<const class wxString &, const class wxString &, const class wxString &, enum wxPathFormat>(), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"), pybind11::arg("format") );

		cl.def("Assign", (void (wxFileName::*)(const class wxFileName &)) &wxFileName::Assign, "C++: wxFileName::Assign(const class wxFileName &) --> void", pybind11::arg("filepath"));
		cl.def("Assign", [](wxFileName &o, const class wxString & a0) -> void { return o.Assign(a0); }, "", pybind11::arg("fullpath"));
		cl.def("Assign", (void (wxFileName::*)(const class wxString &, enum wxPathFormat)) &wxFileName::Assign, "C++: wxFileName::Assign(const class wxString &, enum wxPathFormat) --> void", pybind11::arg("fullpath"), pybind11::arg("format"));
		cl.def("Assign", [](wxFileName &o, const class wxString & a0, const class wxString & a1, const class wxString & a2, const class wxString & a3, bool const & a4) -> void { return o.Assign(a0, a1, a2, a3, a4); }, "", pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"), pybind11::arg("hasExt"));
		cl.def("Assign", (void (wxFileName::*)(const class wxString &, const class wxString &, const class wxString &, const class wxString &, bool, enum wxPathFormat)) &wxFileName::Assign, "C++: wxFileName::Assign(const class wxString &, const class wxString &, const class wxString &, const class wxString &, bool, enum wxPathFormat) --> void", pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"), pybind11::arg("hasExt"), pybind11::arg("format"));
		cl.def("Assign", [](wxFileName &o, const class wxString & a0, const class wxString & a1, const class wxString & a2, const class wxString & a3) -> void { return o.Assign(a0, a1, a2, a3); }, "", pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"));
		cl.def("Assign", (void (wxFileName::*)(const class wxString &, const class wxString &, const class wxString &, const class wxString &, enum wxPathFormat)) &wxFileName::Assign, "C++: wxFileName::Assign(const class wxString &, const class wxString &, const class wxString &, const class wxString &, enum wxPathFormat) --> void", pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"), pybind11::arg("format"));
		cl.def("Assign", [](wxFileName &o, const class wxString & a0, const class wxString & a1) -> void { return o.Assign(a0, a1); }, "", pybind11::arg("path"), pybind11::arg("name"));
		cl.def("Assign", (void (wxFileName::*)(const class wxString &, const class wxString &, enum wxPathFormat)) &wxFileName::Assign, "C++: wxFileName::Assign(const class wxString &, const class wxString &, enum wxPathFormat) --> void", pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("format"));
		cl.def("Assign", [](wxFileName &o, const class wxString & a0, const class wxString & a1, const class wxString & a2) -> void { return o.Assign(a0, a1, a2); }, "", pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"));
		cl.def("Assign", (void (wxFileName::*)(const class wxString &, const class wxString &, const class wxString &, enum wxPathFormat)) &wxFileName::Assign, "C++: wxFileName::Assign(const class wxString &, const class wxString &, const class wxString &, enum wxPathFormat) --> void", pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"), pybind11::arg("format"));
		cl.def("AssignDir", [](wxFileName &o, const class wxString & a0) -> void { return o.AssignDir(a0); }, "", pybind11::arg("dir"));
		cl.def("AssignDir", (void (wxFileName::*)(const class wxString &, enum wxPathFormat)) &wxFileName::AssignDir, "C++: wxFileName::AssignDir(const class wxString &, enum wxPathFormat) --> void", pybind11::arg("dir"), pybind11::arg("format"));
		cl.def("assign", (class wxFileName & (wxFileName::*)(const class wxFileName &)) &wxFileName::operator=, "C++: wxFileName::operator=(const class wxFileName &) --> class wxFileName &", pybind11::return_value_policy::automatic, pybind11::arg("filename"));
		cl.def("assign", (class wxFileName & (wxFileName::*)(const class wxString &)) &wxFileName::operator=, "C++: wxFileName::operator=(const class wxString &) --> class wxFileName &", pybind11::return_value_policy::automatic, pybind11::arg("filename"));
		cl.def("Clear", (void (wxFileName::*)()) &wxFileName::Clear, "C++: wxFileName::Clear() --> void");
		cl.def_static("FileName", [](const class wxString & a0) -> wxFileName { return wxFileName::FileName(a0); }, "", pybind11::arg("file"));
		cl.def_static("FileName", (class wxFileName (*)(const class wxString &, enum wxPathFormat)) &wxFileName::FileName, "C++: wxFileName::FileName(const class wxString &, enum wxPathFormat) --> class wxFileName", pybind11::arg("file"), pybind11::arg("format"));
		cl.def_static("DirName", [](const class wxString & a0) -> wxFileName { return wxFileName::DirName(a0); }, "", pybind11::arg("dir"));
		cl.def_static("DirName", (class wxFileName (*)(const class wxString &, enum wxPathFormat)) &wxFileName::DirName, "C++: wxFileName::DirName(const class wxString &, enum wxPathFormat) --> class wxFileName", pybind11::arg("dir"), pybind11::arg("format"));
		cl.def("IsOk", (bool (wxFileName::*)() const) &wxFileName::IsOk, "C++: wxFileName::IsOk() const --> bool");
		cl.def("FileExists", (bool (wxFileName::*)() const) &wxFileName::FileExists, "C++: wxFileName::FileExists() const --> bool");
		cl.def_static("FileExists", (bool (*)(const class wxString &)) &wxFileName::FileExists, "C++: wxFileName::FileExists(const class wxString &) --> bool", pybind11::arg("file"));
		cl.def("DirExists", (bool (wxFileName::*)() const) &wxFileName::DirExists, "C++: wxFileName::DirExists() const --> bool");
		cl.def_static("DirExists", (bool (*)(const class wxString &)) &wxFileName::DirExists, "C++: wxFileName::DirExists(const class wxString &) --> bool", pybind11::arg("dir"));
		cl.def("Exists", [](wxFileName const &o) -> bool { return o.Exists(); }, "");
		cl.def("Exists", (bool (wxFileName::*)(int) const) &wxFileName::Exists, "C++: wxFileName::Exists(int) const --> bool", pybind11::arg("flags"));
		cl.def_static("Exists", [](const class wxString & a0) -> bool { return wxFileName::Exists(a0); }, "", pybind11::arg("path"));
		cl.def_static("Exists", (bool (*)(const class wxString &, int)) &wxFileName::Exists, "C++: wxFileName::Exists(const class wxString &, int) --> bool", pybind11::arg("path"), pybind11::arg("flags"));
		cl.def("IsDirWritable", (bool (wxFileName::*)() const) &wxFileName::IsDirWritable, "C++: wxFileName::IsDirWritable() const --> bool");
		cl.def_static("IsDirWritable", (bool (*)(const class wxString &)) &wxFileName::IsDirWritable, "C++: wxFileName::IsDirWritable(const class wxString &) --> bool", pybind11::arg("path"));
		cl.def("IsDirReadable", (bool (wxFileName::*)() const) &wxFileName::IsDirReadable, "C++: wxFileName::IsDirReadable() const --> bool");
		cl.def_static("IsDirReadable", (bool (*)(const class wxString &)) &wxFileName::IsDirReadable, "C++: wxFileName::IsDirReadable(const class wxString &) --> bool", pybind11::arg("path"));
		cl.def("IsFileWritable", (bool (wxFileName::*)() const) &wxFileName::IsFileWritable, "C++: wxFileName::IsFileWritable() const --> bool");
		cl.def_static("IsFileWritable", (bool (*)(const class wxString &)) &wxFileName::IsFileWritable, "C++: wxFileName::IsFileWritable(const class wxString &) --> bool", pybind11::arg("path"));
		cl.def("IsFileReadable", (bool (wxFileName::*)() const) &wxFileName::IsFileReadable, "C++: wxFileName::IsFileReadable() const --> bool");
		cl.def_static("IsFileReadable", (bool (*)(const class wxString &)) &wxFileName::IsFileReadable, "C++: wxFileName::IsFileReadable(const class wxString &) --> bool", pybind11::arg("path"));
		cl.def("IsFileExecutable", (bool (wxFileName::*)() const) &wxFileName::IsFileExecutable, "C++: wxFileName::IsFileExecutable() const --> bool");
		cl.def_static("IsFileExecutable", (bool (*)(const class wxString &)) &wxFileName::IsFileExecutable, "C++: wxFileName::IsFileExecutable(const class wxString &) --> bool", pybind11::arg("path"));
		cl.def("SetPermissions", (bool (wxFileName::*)(int)) &wxFileName::SetPermissions, "C++: wxFileName::SetPermissions(int) --> bool", pybind11::arg("permissions"));
		cl.def("SetTimes", (bool (wxFileName::*)(const class wxDateTime *, const class wxDateTime *, const class wxDateTime *) const) &wxFileName::SetTimes, "C++: wxFileName::SetTimes(const class wxDateTime *, const class wxDateTime *, const class wxDateTime *) const --> bool", pybind11::arg("dtAccess"), pybind11::arg("dtMod"), pybind11::arg("dtCreate"));
		cl.def("Touch", (bool (wxFileName::*)() const) &wxFileName::Touch, "C++: wxFileName::Touch() const --> bool");
		cl.def("GetTimes", (bool (wxFileName::*)(class wxDateTime *, class wxDateTime *, class wxDateTime *) const) &wxFileName::GetTimes, "C++: wxFileName::GetTimes(class wxDateTime *, class wxDateTime *, class wxDateTime *) const --> bool", pybind11::arg("dtAccess"), pybind11::arg("dtMod"), pybind11::arg("dtCreate"));
		cl.def("GetModificationTime", (class wxDateTime (wxFileName::*)() const) &wxFileName::GetModificationTime, "C++: wxFileName::GetModificationTime() const --> class wxDateTime");
		cl.def("AssignCwd", [](wxFileName &o) -> void { return o.AssignCwd(); }, "");
		cl.def("AssignCwd", (void (wxFileName::*)(const class wxString &)) &wxFileName::AssignCwd, "C++: wxFileName::AssignCwd(const class wxString &) --> void", pybind11::arg("volume"));
		cl.def_static("GetCwd", []() -> wxString { return wxFileName::GetCwd(); }, "");
		cl.def_static("GetCwd", (class wxString (*)(const class wxString &)) &wxFileName::GetCwd, "C++: wxFileName::GetCwd(const class wxString &) --> class wxString", pybind11::arg("volume"));
		cl.def("SetCwd", (bool (wxFileName::*)() const) &wxFileName::SetCwd, "C++: wxFileName::SetCwd() const --> bool");
		cl.def_static("SetCwd", (bool (*)(const class wxString &)) &wxFileName::SetCwd, "C++: wxFileName::SetCwd(const class wxString &) --> bool", pybind11::arg("cwd"));
		cl.def("AssignHomeDir", (void (wxFileName::*)()) &wxFileName::AssignHomeDir, "C++: wxFileName::AssignHomeDir() --> void");
		cl.def_static("GetHomeDir", (class wxString (*)()) &wxFileName::GetHomeDir, "C++: wxFileName::GetHomeDir() --> class wxString");
		cl.def_static("GetTempDir", (class wxString (*)()) &wxFileName::GetTempDir, "C++: wxFileName::GetTempDir() --> class wxString");
		cl.def("AssignTempFileName", (void (wxFileName::*)(const class wxString &)) &wxFileName::AssignTempFileName, "C++: wxFileName::AssignTempFileName(const class wxString &) --> void", pybind11::arg("prefix"));
		cl.def_static("CreateTempFileName", (class wxString (*)(const class wxString &)) &wxFileName::CreateTempFileName, "C++: wxFileName::CreateTempFileName(const class wxString &) --> class wxString", pybind11::arg("prefix"));
		cl.def("AssignTempFileName", (void (wxFileName::*)(const class wxString &, class wxFile *)) &wxFileName::AssignTempFileName, "C++: wxFileName::AssignTempFileName(const class wxString &, class wxFile *) --> void", pybind11::arg("prefix"), pybind11::arg("fileTemp"));
		cl.def_static("CreateTempFileName", (class wxString (*)(const class wxString &, class wxFile *)) &wxFileName::CreateTempFileName, "C++: wxFileName::CreateTempFileName(const class wxString &, class wxFile *) --> class wxString", pybind11::arg("prefix"), pybind11::arg("fileTemp"));
		cl.def("Mkdir", [](wxFileName const &o) -> bool { return o.Mkdir(); }, "");
		cl.def("Mkdir", [](wxFileName const &o, int const & a0) -> bool { return o.Mkdir(a0); }, "", pybind11::arg("perm"));
		cl.def("Mkdir", (bool (wxFileName::*)(int, int) const) &wxFileName::Mkdir, "C++: wxFileName::Mkdir(int, int) const --> bool", pybind11::arg("perm"), pybind11::arg("flags"));
		cl.def_static("Mkdir", [](const class wxString & a0) -> bool { return wxFileName::Mkdir(a0); }, "", pybind11::arg("dir"));
		cl.def_static("Mkdir", [](const class wxString & a0, int const & a1) -> bool { return wxFileName::Mkdir(a0, a1); }, "", pybind11::arg("dir"), pybind11::arg("perm"));
		cl.def_static("Mkdir", (bool (*)(const class wxString &, int, int)) &wxFileName::Mkdir, "C++: wxFileName::Mkdir(const class wxString &, int, int) --> bool", pybind11::arg("dir"), pybind11::arg("perm"), pybind11::arg("flags"));
		cl.def("Rmdir", [](wxFileName const &o) -> bool { return o.Rmdir(); }, "");
		cl.def("Rmdir", (bool (wxFileName::*)(int) const) &wxFileName::Rmdir, "C++: wxFileName::Rmdir(int) const --> bool", pybind11::arg("flags"));
		cl.def_static("Rmdir", [](const class wxString & a0) -> bool { return wxFileName::Rmdir(a0); }, "", pybind11::arg("dir"));
		cl.def_static("Rmdir", (bool (*)(const class wxString &, int)) &wxFileName::Rmdir, "C++: wxFileName::Rmdir(const class wxString &, int) --> bool", pybind11::arg("dir"), pybind11::arg("flags"));
		cl.def("Normalize", [](wxFileName &o) -> bool { return o.Normalize(); }, "");
		cl.def("Normalize", [](wxFileName &o, int const & a0) -> bool { return o.Normalize(a0); }, "", pybind11::arg("flags"));
		cl.def("Normalize", [](wxFileName &o, int const & a0, const class wxString & a1) -> bool { return o.Normalize(a0, a1); }, "", pybind11::arg("flags"), pybind11::arg("cwd"));
		cl.def("Normalize", (bool (wxFileName::*)(int, const class wxString &, enum wxPathFormat)) &wxFileName::Normalize, "C++: wxFileName::Normalize(int, const class wxString &, enum wxPathFormat) --> bool", pybind11::arg("flags"), pybind11::arg("cwd"), pybind11::arg("format"));
		cl.def("MakeRelativeTo", [](wxFileName &o) -> bool { return o.MakeRelativeTo(); }, "");
		cl.def("MakeRelativeTo", [](wxFileName &o, const class wxString & a0) -> bool { return o.MakeRelativeTo(a0); }, "", pybind11::arg("pathBase"));
		cl.def("MakeRelativeTo", (bool (wxFileName::*)(const class wxString &, enum wxPathFormat)) &wxFileName::MakeRelativeTo, "C++: wxFileName::MakeRelativeTo(const class wxString &, enum wxPathFormat) --> bool", pybind11::arg("pathBase"), pybind11::arg("format"));
		cl.def("MakeAbsolute", [](wxFileName &o) -> bool { return o.MakeAbsolute(); }, "");
		cl.def("MakeAbsolute", [](wxFileName &o, const class wxString & a0) -> bool { return o.MakeAbsolute(a0); }, "", pybind11::arg("cwd"));
		cl.def("MakeAbsolute", (bool (wxFileName::*)(const class wxString &, enum wxPathFormat)) &wxFileName::MakeAbsolute, "C++: wxFileName::MakeAbsolute(const class wxString &, enum wxPathFormat) --> bool", pybind11::arg("cwd"), pybind11::arg("format"));
		cl.def("DontFollowLink", (void (wxFileName::*)()) &wxFileName::DontFollowLink, "C++: wxFileName::DontFollowLink() --> void");
		cl.def("ShouldFollowLink", (bool (wxFileName::*)() const) &wxFileName::ShouldFollowLink, "C++: wxFileName::ShouldFollowLink() const --> bool");
		cl.def("ReplaceEnvVariable", [](wxFileName &o, const class wxString & a0) -> bool { return o.ReplaceEnvVariable(a0); }, "", pybind11::arg("envname"));
		cl.def("ReplaceEnvVariable", [](wxFileName &o, const class wxString & a0, const class wxString & a1) -> bool { return o.ReplaceEnvVariable(a0, a1); }, "", pybind11::arg("envname"), pybind11::arg("replacementFmtString"));
		cl.def("ReplaceEnvVariable", (bool (wxFileName::*)(const class wxString &, const class wxString &, enum wxPathFormat)) &wxFileName::ReplaceEnvVariable, "C++: wxFileName::ReplaceEnvVariable(const class wxString &, const class wxString &, enum wxPathFormat) --> bool", pybind11::arg("envname"), pybind11::arg("replacementFmtString"), pybind11::arg("format"));
		cl.def("ReplaceHomeDir", [](wxFileName &o) -> bool { return o.ReplaceHomeDir(); }, "");
		cl.def("ReplaceHomeDir", (bool (wxFileName::*)(enum wxPathFormat)) &wxFileName::ReplaceHomeDir, "C++: wxFileName::ReplaceHomeDir(enum wxPathFormat) --> bool", pybind11::arg("format"));
		cl.def("SameAs", [](wxFileName const &o, const class wxFileName & a0) -> bool { return o.SameAs(a0); }, "", pybind11::arg("filepath"));
		cl.def("SameAs", (bool (wxFileName::*)(const class wxFileName &, enum wxPathFormat) const) &wxFileName::SameAs, "C++: wxFileName::SameAs(const class wxFileName &, enum wxPathFormat) const --> bool", pybind11::arg("filepath"), pybind11::arg("format"));
		cl.def("__eq__", (bool (wxFileName::*)(const class wxFileName &) const) &wxFileName::operator==, "C++: wxFileName::operator==(const class wxFileName &) const --> bool", pybind11::arg("filename"));
		cl.def("__ne__", (bool (wxFileName::*)(const class wxFileName &) const) &wxFileName::operator!=, "C++: wxFileName::operator!=(const class wxFileName &) const --> bool", pybind11::arg("filename"));
		cl.def("__eq__", (bool (wxFileName::*)(const class wxString &) const) &wxFileName::operator==, "C++: wxFileName::operator==(const class wxString &) const --> bool", pybind11::arg("filename"));
		cl.def("__ne__", (bool (wxFileName::*)(const class wxString &) const) &wxFileName::operator!=, "C++: wxFileName::operator!=(const class wxString &) const --> bool", pybind11::arg("filename"));
		cl.def_static("IsCaseSensitive", []() -> bool { return wxFileName::IsCaseSensitive(); }, "");
		cl.def_static("IsCaseSensitive", (bool (*)(enum wxPathFormat)) &wxFileName::IsCaseSensitive, "C++: wxFileName::IsCaseSensitive(enum wxPathFormat) --> bool", pybind11::arg("format"));
		cl.def("IsAbsolute", [](wxFileName const &o) -> bool { return o.IsAbsolute(); }, "");
		cl.def("IsAbsolute", (bool (wxFileName::*)(enum wxPathFormat) const) &wxFileName::IsAbsolute, "C++: wxFileName::IsAbsolute(enum wxPathFormat) const --> bool", pybind11::arg("format"));
		cl.def("IsRelative", [](wxFileName const &o) -> bool { return o.IsRelative(); }, "");
		cl.def("IsRelative", (bool (wxFileName::*)(enum wxPathFormat) const) &wxFileName::IsRelative, "C++: wxFileName::IsRelative(enum wxPathFormat) const --> bool", pybind11::arg("format"));
		cl.def_static("GetForbiddenChars", []() -> wxString { return wxFileName::GetForbiddenChars(); }, "");
		cl.def_static("GetForbiddenChars", (class wxString (*)(enum wxPathFormat)) &wxFileName::GetForbiddenChars, "C++: wxFileName::GetForbiddenChars(enum wxPathFormat) --> class wxString", pybind11::arg("format"));
		cl.def_static("GetVolumeSeparator", []() -> wxString { return wxFileName::GetVolumeSeparator(); }, "");
		cl.def_static("GetVolumeSeparator", (class wxString (*)(enum wxPathFormat)) &wxFileName::GetVolumeSeparator, "C++: wxFileName::GetVolumeSeparator(enum wxPathFormat) --> class wxString", pybind11::arg("format"));
		cl.def_static("GetPathSeparators", []() -> wxString { return wxFileName::GetPathSeparators(); }, "");
		cl.def_static("GetPathSeparators", (class wxString (*)(enum wxPathFormat)) &wxFileName::GetPathSeparators, "C++: wxFileName::GetPathSeparators(enum wxPathFormat) --> class wxString", pybind11::arg("format"));
		cl.def_static("GetPathTerminators", []() -> wxString { return wxFileName::GetPathTerminators(); }, "");
		cl.def_static("GetPathTerminators", (class wxString (*)(enum wxPathFormat)) &wxFileName::GetPathTerminators, "C++: wxFileName::GetPathTerminators(enum wxPathFormat) --> class wxString", pybind11::arg("format"));
		cl.def_static("GetPathSeparator", []() -> wxUniChar { return wxFileName::GetPathSeparator(); }, "");
		cl.def_static("GetPathSeparator", (class wxUniChar (*)(enum wxPathFormat)) &wxFileName::GetPathSeparator, "C++: wxFileName::GetPathSeparator(enum wxPathFormat) --> class wxUniChar", pybind11::arg("format"));
		cl.def_static("IsPathSeparator", [](wchar_t const & a0) -> bool { return wxFileName::IsPathSeparator(a0); }, "", pybind11::arg("ch"));
		cl.def_static("IsPathSeparator", (bool (*)(wchar_t, enum wxPathFormat)) &wxFileName::IsPathSeparator, "C++: wxFileName::IsPathSeparator(wchar_t, enum wxPathFormat) --> bool", pybind11::arg("ch"), pybind11::arg("format"));
		cl.def_static("IsMSWUniqueVolumeNamePath", [](const class wxString & a0) -> bool { return wxFileName::IsMSWUniqueVolumeNamePath(a0); }, "", pybind11::arg("path"));
		cl.def_static("IsMSWUniqueVolumeNamePath", (bool (*)(const class wxString &, enum wxPathFormat)) &wxFileName::IsMSWUniqueVolumeNamePath, "C++: wxFileName::IsMSWUniqueVolumeNamePath(const class wxString &, enum wxPathFormat) --> bool", pybind11::arg("path"), pybind11::arg("format"));
		cl.def("GetDirCount", (unsigned long (wxFileName::*)() const) &wxFileName::GetDirCount, "C++: wxFileName::GetDirCount() const --> unsigned long");
		cl.def("AppendDir", (bool (wxFileName::*)(const class wxString &)) &wxFileName::AppendDir, "C++: wxFileName::AppendDir(const class wxString &) --> bool", pybind11::arg("dir"));
		cl.def("PrependDir", (void (wxFileName::*)(const class wxString &)) &wxFileName::PrependDir, "C++: wxFileName::PrependDir(const class wxString &) --> void", pybind11::arg("dir"));
		cl.def("InsertDir", (bool (wxFileName::*)(unsigned long, const class wxString &)) &wxFileName::InsertDir, "C++: wxFileName::InsertDir(unsigned long, const class wxString &) --> bool", pybind11::arg("before"), pybind11::arg("dir"));
		cl.def("RemoveDir", (void (wxFileName::*)(unsigned long)) &wxFileName::RemoveDir, "C++: wxFileName::RemoveDir(unsigned long) --> void", pybind11::arg("pos"));
		cl.def("RemoveLastDir", (void (wxFileName::*)()) &wxFileName::RemoveLastDir, "C++: wxFileName::RemoveLastDir() --> void");
		cl.def("SetExt", (void (wxFileName::*)(const class wxString &)) &wxFileName::SetExt, "C++: wxFileName::SetExt(const class wxString &) --> void", pybind11::arg("ext"));
		cl.def("ClearExt", (void (wxFileName::*)()) &wxFileName::ClearExt, "C++: wxFileName::ClearExt() --> void");
		cl.def("SetEmptyExt", (void (wxFileName::*)()) &wxFileName::SetEmptyExt, "C++: wxFileName::SetEmptyExt() --> void");
		cl.def("GetExt", (class wxString (wxFileName::*)() const) &wxFileName::GetExt, "C++: wxFileName::GetExt() const --> class wxString");
		cl.def("HasExt", (bool (wxFileName::*)() const) &wxFileName::HasExt, "C++: wxFileName::HasExt() const --> bool");
		cl.def("SetName", (void (wxFileName::*)(const class wxString &)) &wxFileName::SetName, "C++: wxFileName::SetName(const class wxString &) --> void", pybind11::arg("name"));
		cl.def("GetName", (class wxString (wxFileName::*)() const) &wxFileName::GetName, "C++: wxFileName::GetName() const --> class wxString");
		cl.def("HasName", (bool (wxFileName::*)() const) &wxFileName::HasName, "C++: wxFileName::HasName() const --> bool");
		cl.def("SetVolume", (void (wxFileName::*)(const class wxString &)) &wxFileName::SetVolume, "C++: wxFileName::SetVolume(const class wxString &) --> void", pybind11::arg("volume"));
		cl.def("GetVolume", (class wxString (wxFileName::*)() const) &wxFileName::GetVolume, "C++: wxFileName::GetVolume() const --> class wxString");
		cl.def("HasVolume", (bool (wxFileName::*)() const) &wxFileName::HasVolume, "C++: wxFileName::HasVolume() const --> bool");
		cl.def("SetFullName", (void (wxFileName::*)(const class wxString &)) &wxFileName::SetFullName, "C++: wxFileName::SetFullName(const class wxString &) --> void", pybind11::arg("fullname"));
		cl.def("GetFullName", (class wxString (wxFileName::*)() const) &wxFileName::GetFullName, "C++: wxFileName::GetFullName() const --> class wxString");
		cl.def("GetDirs", (const class wxArrayString & (wxFileName::*)() const) &wxFileName::GetDirs, "C++: wxFileName::GetDirs() const --> const class wxArrayString &", pybind11::return_value_policy::automatic);
		cl.def("GetPath", [](wxFileName const &o) -> wxString { return o.GetPath(); }, "");
		cl.def("GetPath", [](wxFileName const &o, int const & a0) -> wxString { return o.GetPath(a0); }, "", pybind11::arg("flags"));
		cl.def("GetPath", (class wxString (wxFileName::*)(int, enum wxPathFormat) const) &wxFileName::GetPath, "C++: wxFileName::GetPath(int, enum wxPathFormat) const --> class wxString", pybind11::arg("flags"), pybind11::arg("format"));
		cl.def("SetPath", [](wxFileName &o, const class wxString & a0) -> void { return o.SetPath(a0); }, "", pybind11::arg("path"));
		cl.def("SetPath", (void (wxFileName::*)(const class wxString &, enum wxPathFormat)) &wxFileName::SetPath, "C++: wxFileName::SetPath(const class wxString &, enum wxPathFormat) --> void", pybind11::arg("path"), pybind11::arg("format"));
		cl.def("GetFullPath", [](wxFileName const &o) -> wxString { return o.GetFullPath(); }, "");
		cl.def("GetFullPath", (class wxString (wxFileName::*)(enum wxPathFormat) const) &wxFileName::GetFullPath, "C++: wxFileName::GetFullPath(enum wxPathFormat) const --> class wxString", pybind11::arg("format"));
		cl.def("GetShortPath", (class wxString (wxFileName::*)() const) &wxFileName::GetShortPath, "C++: wxFileName::GetShortPath() const --> class wxString");
		cl.def("GetLongPath", (class wxString (wxFileName::*)() const) &wxFileName::GetLongPath, "C++: wxFileName::GetLongPath() const --> class wxString");
		cl.def("IsDir", (bool (wxFileName::*)() const) &wxFileName::IsDir, "C++: wxFileName::IsDir() const --> bool");
		cl.def_static("GetFormat", []() -> wxPathFormat { return wxFileName::GetFormat(); }, "");
		cl.def_static("GetFormat", (enum wxPathFormat (*)(enum wxPathFormat)) &wxFileName::GetFormat, "C++: wxFileName::GetFormat(enum wxPathFormat) --> enum wxPathFormat", pybind11::arg("format"));
		cl.def_static("SplitPath", [](const class wxString & a0, class wxString * a1, class wxString * a2, class wxString * a3, class wxString * a4) -> void { return wxFileName::SplitPath(a0, a1, a2, a3, a4); }, "", pybind11::arg("fullpath"), pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"));
		cl.def_static("SplitPath", [](const class wxString & a0, class wxString * a1, class wxString * a2, class wxString * a3, class wxString * a4, bool * a5) -> void { return wxFileName::SplitPath(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("fullpath"), pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"), pybind11::arg("hasExt"));
		cl.def_static("SplitPath", (void (*)(const class wxString &, class wxString *, class wxString *, class wxString *, class wxString *, bool *, enum wxPathFormat)) &wxFileName::SplitPath, "C++: wxFileName::SplitPath(const class wxString &, class wxString *, class wxString *, class wxString *, class wxString *, bool *, enum wxPathFormat) --> void", pybind11::arg("fullpath"), pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"), pybind11::arg("hasExt"), pybind11::arg("format"));
		cl.def_static("SplitPath", (void (*)(const class wxString &, class wxString *, class wxString *, class wxString *, class wxString *, enum wxPathFormat)) &wxFileName::SplitPath, "C++: wxFileName::SplitPath(const class wxString &, class wxString *, class wxString *, class wxString *, class wxString *, enum wxPathFormat) --> void", pybind11::arg("fullpath"), pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"), pybind11::arg("format"));
		cl.def_static("SplitPath", [](const class wxString & a0, class wxString * a1, class wxString * a2, class wxString * a3) -> void { return wxFileName::SplitPath(a0, a1, a2, a3); }, "", pybind11::arg("fullpath"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"));
		cl.def_static("SplitPath", (void (*)(const class wxString &, class wxString *, class wxString *, class wxString *, enum wxPathFormat)) &wxFileName::SplitPath, "C++: wxFileName::SplitPath(const class wxString &, class wxString *, class wxString *, class wxString *, enum wxPathFormat) --> void", pybind11::arg("fullpath"), pybind11::arg("path"), pybind11::arg("name"), pybind11::arg("ext"), pybind11::arg("format"));
		cl.def_static("SplitVolume", [](const class wxString & a0, class wxString * a1, class wxString * a2) -> void { return wxFileName::SplitVolume(a0, a1, a2); }, "", pybind11::arg("fullpathWithVolume"), pybind11::arg("volume"), pybind11::arg("path"));
		cl.def_static("SplitVolume", (void (*)(const class wxString &, class wxString *, class wxString *, enum wxPathFormat)) &wxFileName::SplitVolume, "C++: wxFileName::SplitVolume(const class wxString &, class wxString *, class wxString *, enum wxPathFormat) --> void", pybind11::arg("fullpathWithVolume"), pybind11::arg("volume"), pybind11::arg("path"), pybind11::arg("format"));
		cl.def_static("StripExtension", (class wxString (*)(const class wxString &)) &wxFileName::StripExtension, "C++: wxFileName::StripExtension(const class wxString &) --> class wxString", pybind11::arg("fullpath"));
		cl.def("GetSize", (class wxULongLongNative (wxFileName::*)() const) &wxFileName::GetSize, "C++: wxFileName::GetSize() const --> class wxULongLongNative");
		cl.def_static("GetSize", (class wxULongLongNative (*)(const class wxString &)) &wxFileName::GetSize, "C++: wxFileName::GetSize(const class wxString &) --> class wxULongLongNative", pybind11::arg("file"));
		cl.def("GetHumanReadableSize", [](wxFileName const &o) -> wxString { return o.GetHumanReadableSize(); }, "");
		cl.def("GetHumanReadableSize", [](wxFileName const &o, const class wxString & a0) -> wxString { return o.GetHumanReadableSize(a0); }, "", pybind11::arg("nullsize"));
		cl.def("GetHumanReadableSize", [](wxFileName const &o, const class wxString & a0, int const & a1) -> wxString { return o.GetHumanReadableSize(a0, a1); }, "", pybind11::arg("nullsize"), pybind11::arg("precision"));
		cl.def("GetHumanReadableSize", (class wxString (wxFileName::*)(const class wxString &, int, enum wxSizeConvention) const) &wxFileName::GetHumanReadableSize, "C++: wxFileName::GetHumanReadableSize(const class wxString &, int, enum wxSizeConvention) const --> class wxString", pybind11::arg("nullsize"), pybind11::arg("precision"), pybind11::arg("conv"));
		cl.def_static("GetHumanReadableSize", [](const class wxULongLongNative & a0) -> wxString { return wxFileName::GetHumanReadableSize(a0); }, "", pybind11::arg("sz"));
		cl.def_static("GetHumanReadableSize", [](const class wxULongLongNative & a0, const class wxString & a1) -> wxString { return wxFileName::GetHumanReadableSize(a0, a1); }, "", pybind11::arg("sz"), pybind11::arg("nullsize"));
		cl.def_static("GetHumanReadableSize", [](const class wxULongLongNative & a0, const class wxString & a1, int const & a2) -> wxString { return wxFileName::GetHumanReadableSize(a0, a1, a2); }, "", pybind11::arg("sz"), pybind11::arg("nullsize"), pybind11::arg("precision"));
		cl.def_static("GetHumanReadableSize", (class wxString (*)(const class wxULongLongNative &, const class wxString &, int, enum wxSizeConvention)) &wxFileName::GetHumanReadableSize, "C++: wxFileName::GetHumanReadableSize(const class wxULongLongNative &, const class wxString &, int, enum wxSizeConvention) --> class wxString", pybind11::arg("sz"), pybind11::arg("nullsize"), pybind11::arg("precision"), pybind11::arg("conv"));
		cl.def("GetPath", [](wxFileName const &o, bool const & a0) -> wxString { return o.GetPath(a0); }, "", pybind11::arg("withSep"));
		cl.def("GetPath", (class wxString (wxFileName::*)(bool, enum wxPathFormat) const) &wxFileName::GetPath, "C++: wxFileName::GetPath(bool, enum wxPathFormat) const --> class wxString", pybind11::arg("withSep"), pybind11::arg("format"));
		cl.def("GetPathWithSep", [](wxFileName const &o) -> wxString { return o.GetPathWithSep(); }, "");
		cl.def("GetPathWithSep", (class wxString (wxFileName::*)(enum wxPathFormat) const) &wxFileName::GetPathWithSep, "C++: wxFileName::GetPathWithSep(enum wxPathFormat) const --> class wxString", pybind11::arg("format"));
	}
}
