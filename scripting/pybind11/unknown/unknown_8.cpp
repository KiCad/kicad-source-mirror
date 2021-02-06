#include <cstdio> // _IO_FILE
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

void bind_unknown_unknown_8(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxFormatString file: line:136
		pybind11::class_<wxFormatString, std::shared_ptr<wxFormatString>> cl(M(""), "wxFormatString", "");
		cl.def( pybind11::init<const char *>(), pybind11::arg("str") );

		cl.def( pybind11::init<const wchar_t *>(), pybind11::arg("str") );

		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("str") );

		cl.def( pybind11::init<const class wxCStrData &>(), pybind11::arg("str") );

		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<char> &>(), pybind11::arg("str") );

		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<wchar_t> &>(), pybind11::arg("str") );

		cl.def( pybind11::init( [](wxFormatString const &o){ return new wxFormatString(o); } ) );

		pybind11::enum_<wxFormatString::ArgumentType>(cl, "ArgumentType", pybind11::arithmetic(), "")
			.value("Arg_Unused", wxFormatString::Arg_Unused)
			.value("Arg_Char", wxFormatString::Arg_Char)
			.value("Arg_Pointer", wxFormatString::Arg_Pointer)
			.value("Arg_String", wxFormatString::Arg_String)
			.value("Arg_Int", wxFormatString::Arg_Int)
			.value("Arg_LongInt", wxFormatString::Arg_LongInt)
			.value("Arg_LongLongInt", wxFormatString::Arg_LongLongInt)
			.value("Arg_Double", wxFormatString::Arg_Double)
			.value("Arg_LongDouble", wxFormatString::Arg_LongDouble)
			.value("Arg_Size_t", wxFormatString::Arg_Size_t)
			.value("Arg_IntPtr", wxFormatString::Arg_IntPtr)
			.value("Arg_ShortIntPtr", wxFormatString::Arg_ShortIntPtr)
			.value("Arg_LongIntPtr", wxFormatString::Arg_LongIntPtr)
			.value("Arg_Unknown", wxFormatString::Arg_Unknown)
			.export_values();

		cl.def("GetArgumentType", (enum wxFormatString::ArgumentType (wxFormatString::*)(unsigned int) const) &wxFormatString::GetArgumentType, "C++: wxFormatString::GetArgumentType(unsigned int) const --> enum wxFormatString::ArgumentType", pybind11::arg("n"));
		cl.def("InputAsString", (class wxString (wxFormatString::*)() const) &wxFormatString::InputAsString, "C++: wxFormatString::InputAsString() const --> class wxString");
	}
	{ // wxFormatStringArgument file: line:247
		pybind11::class_<wxFormatStringArgument, std::shared_ptr<wxFormatStringArgument>> cl(M(""), "wxFormatStringArgument", "");
		cl.def( pybind11::init( [](){ return new wxFormatStringArgument(); } ), "doc" );
		cl.def( pybind11::init<const class wxFormatString *>(), pybind11::arg("s") );

		cl.def( pybind11::init( [](wxFormatStringArgument const &o){ return new wxFormatStringArgument(o); } ) );
	}
	{ // wxArgNormalizedString file: line:846
		pybind11::class_<wxArgNormalizedString, std::shared_ptr<wxArgNormalizedString>> cl(M(""), "wxArgNormalizedString", "");
		cl.def( pybind11::init<const void *>(), pybind11::arg("ptr") );

		cl.def("IsValid", (bool (wxArgNormalizedString::*)() const) &wxArgNormalizedString::IsValid, "C++: wxArgNormalizedString::IsValid() const --> bool");
		cl.def("GetString", (class wxString (wxArgNormalizedString::*)() const) &wxArgNormalizedString::GetString, "C++: wxArgNormalizedString::GetString() const --> class wxString");
	}
	{ // wxStringOperationsWchar file: line:27
		pybind11::class_<wxStringOperationsWchar, std::shared_ptr<wxStringOperationsWchar>> cl(M(""), "wxStringOperationsWchar", "");
		cl.def( pybind11::init( [](){ return new wxStringOperationsWchar(); } ) );
		cl.def_static("EncodeChar", (wchar_t (*)(const class wxUniChar &)) &wxStringOperationsWchar::EncodeChar, "C++: wxStringOperationsWchar::EncodeChar(const class wxUniChar &) --> wchar_t", pybind11::arg("ch"));
	}
	// IsEmpty(const char *) file: line:133
	M("").def("IsEmpty", (bool (*)(const char *)) &IsEmpty, "C++: IsEmpty(const char *) --> bool", pybind11::arg("p"));

	// Strlen(const char *) file: line:137
	M("").def("Strlen", (unsigned long (*)(const char *)) &Strlen, "C++: Strlen(const char *) --> unsigned long", pybind11::arg("psz"));

	// Stricmp(const char *, const char *) file: line:142
	M("").def("Stricmp", (int (*)(const char *, const char *)) &Stricmp, "C++: Stricmp(const char *, const char *) --> int", pybind11::arg("psz1"), pybind11::arg("psz2"));

	{ // wxCStrData file: line:153
		pybind11::class_<wxCStrData, std::shared_ptr<wxCStrData>> cl(M(""), "wxCStrData", "");
		cl.def( pybind11::init<char *>(), pybind11::arg("buf") );

		cl.def( pybind11::init<wchar_t *>(), pybind11::arg("buf") );

		cl.def( pybind11::init( [](wxCStrData const &o){ return new wxCStrData(o); } ) );
		cl.def("AsWChar", (const wchar_t * (wxCStrData::*)() const) &wxCStrData::AsWChar, "C++: wxCStrData::AsWChar() const --> const wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("AsChar", (const char * (wxCStrData::*)() const) &wxCStrData::AsChar, "C++: wxCStrData::AsChar() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("AsUnsignedChar", (const unsigned char * (wxCStrData::*)() const) &wxCStrData::AsUnsignedChar, "C++: wxCStrData::AsUnsignedChar() const --> const unsigned char *", pybind11::return_value_policy::automatic);
		cl.def("AsCharBuf", (const class wxScopedCharTypeBuffer<char> (wxCStrData::*)() const) &wxCStrData::AsCharBuf, "C++: wxCStrData::AsCharBuf() const --> const class wxScopedCharTypeBuffer<char>");
		cl.def("AsWCharBuf", (const class wxScopedCharTypeBuffer<wchar_t> (wxCStrData::*)() const) &wxCStrData::AsWCharBuf, "C++: wxCStrData::AsWCharBuf() const --> const class wxScopedCharTypeBuffer<wchar_t>");
		cl.def("AsString", (class wxString (wxCStrData::*)() const) &wxCStrData::AsString, "C++: wxCStrData::AsString() const --> class wxString");
		cl.def("AsInternal", (const wchar_t * (wxCStrData::*)() const) &wxCStrData::AsInternal, "C++: wxCStrData::AsInternal() const --> const wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("__getitem__", (class wxUniChar (wxCStrData::*)(unsigned long) const) &wxCStrData::operator[], "C++: wxCStrData::operator[](unsigned long) const --> class wxUniChar", pybind11::arg("n"));
		cl.def("__getitem__", (class wxUniChar (wxCStrData::*)(int) const) &wxCStrData::operator[], "C++: wxCStrData::operator[](int) const --> class wxUniChar", pybind11::arg("n"));
		cl.def("__getitem__", (class wxUniChar (wxCStrData::*)(long) const) &wxCStrData::operator[], "C++: wxCStrData::operator[](long) const --> class wxUniChar", pybind11::arg("n"));
		cl.def("__getitem__", (class wxUniChar (wxCStrData::*)(unsigned int) const) &wxCStrData::operator[], "C++: wxCStrData::operator[](unsigned int) const --> class wxUniChar", pybind11::arg("n"));
		cl.def("__add__", (class wxCStrData (wxCStrData::*)(int) const) &wxCStrData::operator+, "C++: wxCStrData::operator+(int) const --> class wxCStrData", pybind11::arg("n"));
		cl.def("__add__", (class wxCStrData (wxCStrData::*)(long) const) &wxCStrData::operator+, "C++: wxCStrData::operator+(long) const --> class wxCStrData", pybind11::arg("n"));
		cl.def("__add__", (class wxCStrData (wxCStrData::*)(unsigned long) const) &wxCStrData::operator+, "C++: wxCStrData::operator+(unsigned long) const --> class wxCStrData", pybind11::arg("n"));
		cl.def("__sub__", (class wxCStrData (wxCStrData::*)(long) const) &wxCStrData::operator-, "C++: wxCStrData::operator-(long) const --> class wxCStrData", pybind11::arg("n"));
		cl.def("__mul__", (class wxUniChar (wxCStrData::*)() const) &wxCStrData::operator*, "C++: wxCStrData::operator*() const --> class wxUniChar");
	}
	{ // wxString file: line:393
		pybind11::class_<wxString, std::shared_ptr<wxString>> cl(M(""), "wxString", "");
		cl.def( pybind11::init( [](){ return new wxString(); } ) );
		cl.def( pybind11::init( [](wxString const &o){ return new wxString(o); } ) );
		cl.def( pybind11::init( [](class wxUniChar const & a0){ return new wxString(a0); } ), "doc" , pybind11::arg("ch"));
		cl.def( pybind11::init<class wxUniChar, unsigned long>(), pybind11::arg("ch"), pybind11::arg("nRepeat") );

		cl.def( pybind11::init<unsigned long, class wxUniChar>(), pybind11::arg("nRepeat"), pybind11::arg("ch") );

		cl.def( pybind11::init( [](class wxUniCharRef const & a0){ return new wxString(a0); } ), "doc" , pybind11::arg("ch"));
		cl.def( pybind11::init<class wxUniCharRef, unsigned long>(), pybind11::arg("ch"), pybind11::arg("nRepeat") );

		cl.def( pybind11::init<unsigned long, class wxUniCharRef>(), pybind11::arg("nRepeat"), pybind11::arg("ch") );

		cl.def( pybind11::init( [](char const & a0){ return new wxString(a0); } ), "doc" , pybind11::arg("ch"));
		cl.def( pybind11::init<char, unsigned long>(), pybind11::arg("ch"), pybind11::arg("nRepeat") );

		cl.def( pybind11::init<unsigned long, char>(), pybind11::arg("nRepeat"), pybind11::arg("ch") );

		cl.def( pybind11::init( [](wchar_t const & a0){ return new wxString(a0); } ), "doc" , pybind11::arg("ch"));
		cl.def( pybind11::init<wchar_t, unsigned long>(), pybind11::arg("ch"), pybind11::arg("nRepeat") );

		cl.def( pybind11::init<unsigned long, wchar_t>(), pybind11::arg("nRepeat"), pybind11::arg("ch") );

		cl.def( pybind11::init<const char *>(), pybind11::arg("psz") );

		cl.def( pybind11::init<const char *, const class wxMBConv &>(), pybind11::arg("psz"), pybind11::arg("conv") );

		cl.def( pybind11::init<const char *, unsigned long>(), pybind11::arg("psz"), pybind11::arg("nLength") );

		cl.def( pybind11::init<const char *, const class wxMBConv &, unsigned long>(), pybind11::arg("psz"), pybind11::arg("conv"), pybind11::arg("nLength") );

		cl.def( pybind11::init<const unsigned char *>(), pybind11::arg("psz") );

		cl.def( pybind11::init<const unsigned char *, const class wxMBConv &>(), pybind11::arg("psz"), pybind11::arg("conv") );

		cl.def( pybind11::init<const unsigned char *, unsigned long>(), pybind11::arg("psz"), pybind11::arg("nLength") );

		cl.def( pybind11::init<const unsigned char *, const class wxMBConv &, unsigned long>(), pybind11::arg("psz"), pybind11::arg("conv"), pybind11::arg("nLength") );

		cl.def( pybind11::init<const wchar_t *>(), pybind11::arg("pwz") );

		cl.def( pybind11::init<const wchar_t *, const class wxMBConv &>(), pybind11::arg("pwz"), pybind11::arg("") );

		cl.def( pybind11::init<const wchar_t *, unsigned long>(), pybind11::arg("pwz"), pybind11::arg("nLength") );

		cl.def( pybind11::init<const wchar_t *, const class wxMBConv &, unsigned long>(), pybind11::arg("pwz"), pybind11::arg(""), pybind11::arg("nLength") );

		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<char> &>(), pybind11::arg("buf") );

		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<wchar_t> &>(), pybind11::arg("buf") );

		cl.def( pybind11::init<const class wxCStrData &>(), pybind11::arg("cstr") );

		cl.def( pybind11::init<const class wxCStrData &, unsigned long>(), pybind11::arg("cstr"), pybind11::arg("nLength") );

		cl.def( pybind11::init<const class wxString &, unsigned long>(), pybind11::arg("str"), pybind11::arg("nLength") );

		cl.def( pybind11::init<const class wxString &, unsigned long, unsigned long>(), pybind11::arg("str"), pybind11::arg("nPos"), pybind11::arg("nLen") );

		cl.def( pybind11::init<class wxString::const_iterator, class wxString::const_iterator>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def( pybind11::init<const char *, const char *>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def( pybind11::init<const wchar_t *, const wchar_t *>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def( pybind11::init<const class wxCStrData &, const class wxCStrData &>(), pybind11::arg("first"), pybind11::arg("last") );


		pybind11::enum_<wxString::caseCompare>(cl, "caseCompare", pybind11::arithmetic(), "")
			.value("exact", wxString::exact)
			.value("ignoreCase", wxString::ignoreCase)
			.export_values();


		pybind11::enum_<wxString::stripType>(cl, "stripType", pybind11::arithmetic(), "")
			.value("leading", wxString::leading)
			.value("trailing", wxString::trailing)
			.value("both", wxString::both)
			.export_values();

		cl.def_static("Format", (class wxString (*)(const class wxFormatString &, int)) &wxString::Format<int>, "C++: wxString::Format(const class wxFormatString &, int) --> class wxString", pybind11::arg("f1"), pybind11::arg("a1"));
		cl.def_static("Format", (class wxString (*)(const class wxFormatString &, unsigned int)) &wxString::Format<unsigned int>, "C++: wxString::Format(const class wxFormatString &, unsigned int) --> class wxString", pybind11::arg("f1"), pybind11::arg("a1"));
		cl.def_static("Format", (class wxString (*)(const class wxFormatString &, long)) &wxString::Format<long>, "C++: wxString::Format(const class wxFormatString &, long) --> class wxString", pybind11::arg("f1"), pybind11::arg("a1"));
		cl.def_static("Format", (class wxString (*)(const class wxFormatString &, unsigned long)) &wxString::Format<unsigned long>, "C++: wxString::Format(const class wxFormatString &, unsigned long) --> class wxString", pybind11::arg("f1"), pybind11::arg("a1"));
		cl.def_static("Format", (class wxString (*)(const class wxFormatString &, long long)) &wxString::Format<long long>, "C++: wxString::Format(const class wxFormatString &, long long) --> class wxString", pybind11::arg("f1"), pybind11::arg("a1"));
		cl.def_static("Format", (class wxString (*)(const class wxFormatString &, unsigned long long)) &wxString::Format<unsigned long long>, "C++: wxString::Format(const class wxFormatString &, unsigned long long) --> class wxString", pybind11::arg("f1"), pybind11::arg("a1"));
		cl.def_static("Format", (class wxString (*)(const class wxFormatString &, float)) &wxString::Format<float>, "C++: wxString::Format(const class wxFormatString &, float) --> class wxString", pybind11::arg("f1"), pybind11::arg("a1"));
		cl.def_static("Format", (class wxString (*)(const class wxFormatString &, double)) &wxString::Format<double>, "C++: wxString::Format(const class wxFormatString &, double) --> class wxString", pybind11::arg("f1"), pybind11::arg("a1"));
		cl.def("GetIterForNthChar", (class wxString::iterator (wxString::*)(unsigned long)) &wxString::GetIterForNthChar, "C++: wxString::GetIterForNthChar(unsigned long) --> class wxString::iterator", pybind11::arg("n"));
		cl.def("IterOffsetInMBStr", (long (wxString::*)(const class wxString::const_iterator &) const) &wxString::IterOffsetInMBStr, "C++: wxString::IterOffsetInMBStr(const class wxString::const_iterator &) const --> long", pybind11::arg("i"));
		cl.def("Clone", (class wxString (wxString::*)() const) &wxString::Clone, "C++: wxString::Clone() const --> class wxString");
		cl.def("begin", (class wxString::iterator (wxString::*)()) &wxString::begin, "C++: wxString::begin() --> class wxString::iterator");
		cl.def("end", (class wxString::iterator (wxString::*)()) &wxString::end, "C++: wxString::end() --> class wxString::iterator");
		cl.def("rbegin", (class wxString::reverse_iterator_impl<class wxString::iterator> (wxString::*)()) &wxString::rbegin, "C++: wxString::rbegin() --> class wxString::reverse_iterator_impl<class wxString::iterator>");
		cl.def("rend", (class wxString::reverse_iterator_impl<class wxString::iterator> (wxString::*)()) &wxString::rend, "C++: wxString::rend() --> class wxString::reverse_iterator_impl<class wxString::iterator>");
		cl.def("length", (unsigned long (wxString::*)() const) &wxString::length, "C++: wxString::length() const --> unsigned long");
		cl.def("size", (unsigned long (wxString::*)() const) &wxString::size, "C++: wxString::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxString::*)() const) &wxString::max_size, "C++: wxString::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxString::*)() const) &wxString::empty, "C++: wxString::empty() const --> bool");
		cl.def("capacity", (unsigned long (wxString::*)() const) &wxString::capacity, "C++: wxString::capacity() const --> unsigned long");
		cl.def("reserve", (void (wxString::*)(unsigned long)) &wxString::reserve, "C++: wxString::reserve(unsigned long) --> void", pybind11::arg("sz"));
		cl.def("resize", [](wxString &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("nSize"));
		cl.def("resize", (void (wxString::*)(unsigned long, class wxUniChar)) &wxString::resize, "C++: wxString::resize(unsigned long, class wxUniChar) --> void", pybind11::arg("nSize"), pybind11::arg("ch"));
		cl.def("substr", [](wxString const &o) -> wxString { return o.substr(); }, "");
		cl.def("substr", [](wxString const &o, unsigned long const & a0) -> wxString { return o.substr(a0); }, "", pybind11::arg("nStart"));
		cl.def("substr", (class wxString (wxString::*)(unsigned long, unsigned long) const) &wxString::substr, "C++: wxString::substr(unsigned long, unsigned long) const --> class wxString", pybind11::arg("nStart"), pybind11::arg("nLen"));
		cl.def("Len", (unsigned long (wxString::*)() const) &wxString::Len, "C++: wxString::Len() const --> unsigned long");
		cl.def("IsEmpty", (bool (wxString::*)() const) &wxString::IsEmpty, "C++: wxString::IsEmpty() const --> bool");
		cl.def("Truncate", (class wxString & (wxString::*)(unsigned long)) &wxString::Truncate, "C++: wxString::Truncate(unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("uiLen"));
		cl.def("Empty", (void (wxString::*)()) &wxString::Empty, "C++: wxString::Empty() --> void");
		cl.def("Clear", (void (wxString::*)()) &wxString::Clear, "C++: wxString::Clear() --> void");
		cl.def("IsAscii", (bool (wxString::*)() const) &wxString::IsAscii, "C++: wxString::IsAscii() const --> bool");
		cl.def("IsNumber", (bool (wxString::*)() const) &wxString::IsNumber, "C++: wxString::IsNumber() const --> bool");
		cl.def("IsWord", (bool (wxString::*)() const) &wxString::IsWord, "C++: wxString::IsWord() const --> bool");
		cl.def("GetChar", (class wxUniChar (wxString::*)(unsigned long) const) &wxString::GetChar, "C++: wxString::GetChar(unsigned long) const --> class wxUniChar", pybind11::arg("n"));
		cl.def("at", (class wxUniCharRef (wxString::*)(unsigned long)) &wxString::at, "C++: wxString::at(unsigned long) --> class wxUniCharRef", pybind11::arg("n"));
		cl.def("GetWritableChar", (class wxUniCharRef (wxString::*)(unsigned long)) &wxString::GetWritableChar, "C++: wxString::GetWritableChar(unsigned long) --> class wxUniCharRef", pybind11::arg("n"));
		cl.def("SetChar", (void (wxString::*)(unsigned long, class wxUniChar)) &wxString::SetChar, "C++: wxString::SetChar(unsigned long, class wxUniChar) --> void", pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("Last", (class wxUniCharRef (wxString::*)()) &wxString::Last, "C++: wxString::Last() --> class wxUniCharRef");
		cl.def("__getitem__", (class wxUniCharRef (wxString::*)(int)) &wxString::operator[], "C++: wxString::operator[](int) --> class wxUniCharRef", pybind11::arg("n"));
		cl.def("__getitem__", (class wxUniCharRef (wxString::*)(long)) &wxString::operator[], "C++: wxString::operator[](long) --> class wxUniCharRef", pybind11::arg("n"));
		cl.def("__getitem__", (class wxUniCharRef (wxString::*)(unsigned long)) &wxString::operator[], "C++: wxString::operator[](unsigned long) --> class wxUniCharRef", pybind11::arg("n"));
		cl.def("__getitem__", (class wxUniCharRef (wxString::*)(unsigned int)) &wxString::operator[], "C++: wxString::operator[](unsigned int) --> class wxUniCharRef", pybind11::arg("n"));
		cl.def("c_str", (class wxCStrData (wxString::*)() const) &wxString::c_str, "C++: wxString::c_str() const --> class wxCStrData");
		cl.def("data", (class wxCStrData (wxString::*)() const) &wxString::data, "C++: wxString::data() const --> class wxCStrData");
		cl.def("GetData", (const class wxCStrData (wxString::*)() const) &wxString::GetData, "C++: wxString::GetData() const --> const class wxCStrData");
		cl.def("wx_str", (const wchar_t * (wxString::*)() const) &wxString::wx_str, "C++: wxString::wx_str() const --> const wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("char_str", [](wxString const &o) -> wxWritableCharTypeBuffer<char> { return o.char_str(); }, "");
		cl.def("char_str", (class wxWritableCharTypeBuffer<char> (wxString::*)(const class wxMBConv &) const) &wxString::char_str, "C++: wxString::char_str(const class wxMBConv &) const --> class wxWritableCharTypeBuffer<char>", pybind11::arg("conv"));
		cl.def("wchar_str", (class wxWritableCharTypeBuffer<wchar_t> (wxString::*)() const) &wxString::wchar_str, "C++: wxString::wchar_str() const --> class wxWritableCharTypeBuffer<wchar_t>");
		cl.def_static("FromAscii", (class wxString (*)(const char *, unsigned long)) &wxString::FromAscii, "C++: wxString::FromAscii(const char *, unsigned long) --> class wxString", pybind11::arg("ascii"), pybind11::arg("len"));
		cl.def_static("FromAscii", (class wxString (*)(const char *)) &wxString::FromAscii, "C++: wxString::FromAscii(const char *) --> class wxString", pybind11::arg("ascii"));
		cl.def_static("FromAscii", (class wxString (*)(char)) &wxString::FromAscii, "C++: wxString::FromAscii(char) --> class wxString", pybind11::arg("ascii"));
		cl.def("ToAscii", (const class wxScopedCharTypeBuffer<char> (wxString::*)() const) &wxString::ToAscii, "C++: wxString::ToAscii() const --> const class wxScopedCharTypeBuffer<char>");
		cl.def_static("FromAscii", (class wxString (*)(const unsigned char *)) &wxString::FromAscii, "C++: wxString::FromAscii(const unsigned char *) --> class wxString", pybind11::arg("ascii"));
		cl.def_static("FromAscii", (class wxString (*)(const unsigned char *, unsigned long)) &wxString::FromAscii, "C++: wxString::FromAscii(const unsigned char *, unsigned long) --> class wxString", pybind11::arg("ascii"), pybind11::arg("len"));
		cl.def_static("FromUTF8", [](const char * a0) -> wxString { return wxString::FromUTF8(a0); }, "", pybind11::arg("utf8"));
		cl.def_static("FromUTF8", (class wxString (*)(const char *, unsigned long)) &wxString::FromUTF8, "C++: wxString::FromUTF8(const char *, unsigned long) --> class wxString", pybind11::arg("utf8"), pybind11::arg("len"));
		cl.def_static("FromUTF8Unchecked", [](const char * a0) -> wxString { return wxString::FromUTF8Unchecked(a0); }, "", pybind11::arg("utf8"));
		cl.def_static("FromUTF8Unchecked", (class wxString (*)(const char *, unsigned long)) &wxString::FromUTF8Unchecked, "C++: wxString::FromUTF8Unchecked(const char *, unsigned long) --> class wxString", pybind11::arg("utf8"), pybind11::arg("len"));
		cl.def("utf8_str", (const class wxScopedCharTypeBuffer<char> (wxString::*)() const) &wxString::utf8_str, "C++: wxString::utf8_str() const --> const class wxScopedCharTypeBuffer<char>");
		cl.def("ToUTF8", (const class wxScopedCharTypeBuffer<char> (wxString::*)() const) &wxString::ToUTF8, "C++: wxString::ToUTF8() const --> const class wxScopedCharTypeBuffer<char>");
		cl.def_static("From8BitData", (class wxString (*)(const char *, unsigned long)) &wxString::From8BitData, "C++: wxString::From8BitData(const char *, unsigned long) --> class wxString", pybind11::arg("data"), pybind11::arg("len"));
		cl.def_static("From8BitData", (class wxString (*)(const char *)) &wxString::From8BitData, "C++: wxString::From8BitData(const char *) --> class wxString", pybind11::arg("data"));
		cl.def("To8BitData", (const class wxScopedCharTypeBuffer<char> (wxString::*)() const) &wxString::To8BitData, "C++: wxString::To8BitData() const --> const class wxScopedCharTypeBuffer<char>");
		cl.def("mb_str", [](wxString const &o) -> const wxScopedCharTypeBuffer<char> { return o.mb_str(); }, "");
		cl.def("mb_str", (const class wxScopedCharTypeBuffer<char> (wxString::*)(const class wxMBConv &) const) &wxString::mb_str, "C++: wxString::mb_str(const class wxMBConv &) const --> const class wxScopedCharTypeBuffer<char>", pybind11::arg("conv"));
		cl.def("mbc_str", (const class wxCharBuffer (wxString::*)() const) &wxString::mbc_str, "C++: wxString::mbc_str() const --> const class wxCharBuffer");
		cl.def("wc_str", (const wchar_t * (wxString::*)() const) &wxString::wc_str, "C++: wxString::wc_str() const --> const wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("wc_str", (const wchar_t * (wxString::*)(const class wxMBConv &) const) &wxString::wc_str, "C++: wxString::wc_str(const class wxMBConv &) const --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("fn_str", (const class wxScopedCharTypeBuffer<char> (wxString::*)() const) &wxString::fn_str, "C++: wxString::fn_str() const --> const class wxScopedCharTypeBuffer<char>");
		cl.def("t_str", (const wchar_t * (wxString::*)() const) &wxString::t_str, "C++: wxString::t_str() const --> const wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxString & (wxString::*)(const class wxString &)) &wxString::operator=, "C++: wxString::operator=(const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("stringSrc"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxCStrData &)) &wxString::operator=, "C++: wxString::operator=(const class wxCStrData &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("cstr"));
		cl.def("assign", (class wxString & (wxString::*)(class wxUniChar)) &wxString::operator=, "C++: wxString::operator=(class wxUniChar) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("assign", (class wxString & (wxString::*)(class wxUniCharRef)) &wxString::operator=, "C++: wxString::operator=(class wxUniCharRef) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("assign", (class wxString & (wxString::*)(char)) &wxString::operator=, "C++: wxString::operator=(char) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("assign", (class wxString & (wxString::*)(unsigned char)) &wxString::operator=, "C++: wxString::operator=(unsigned char) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("assign", (class wxString & (wxString::*)(wchar_t)) &wxString::operator=, "C++: wxString::operator=(wchar_t) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("assign", (class wxString & (wxString::*)(const char *)) &wxString::operator=, "C++: wxString::operator=(const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"));
		cl.def("assign", (class wxString & (wxString::*)(const wchar_t *)) &wxString::operator=, "C++: wxString::operator=(const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pwz"));
		cl.def("assign", (class wxString & (wxString::*)(const unsigned char *)) &wxString::operator=, "C++: wxString::operator=(const unsigned char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxString::operator=, "C++: wxString::operator=(const class wxScopedCharTypeBuffer<wchar_t> &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<char> &)) &wxString::operator=, "C++: wxString::operator=(const class wxScopedCharTypeBuffer<char> &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("Append", (class wxString & (wxString::*)(const class wxString &)) &wxString::Append, "C++: wxString::Append(const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("Append", (class wxString & (wxString::*)(const char *)) &wxString::Append, "C++: wxString::Append(const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"));
		cl.def("Append", (class wxString & (wxString::*)(const wchar_t *)) &wxString::Append, "C++: wxString::Append(const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pwz"));
		cl.def("Append", (class wxString & (wxString::*)(const class wxCStrData &)) &wxString::Append, "C++: wxString::Append(const class wxCStrData &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"));
		cl.def("Append", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<char> &)) &wxString::Append, "C++: wxString::Append(const class wxScopedCharTypeBuffer<char> &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"));
		cl.def("Append", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxString::Append, "C++: wxString::Append(const class wxScopedCharTypeBuffer<wchar_t> &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"));
		cl.def("Append", (class wxString & (wxString::*)(const char *, unsigned long)) &wxString::Append, "C++: wxString::Append(const char *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"), pybind11::arg("nLen"));
		cl.def("Append", (class wxString & (wxString::*)(const wchar_t *, unsigned long)) &wxString::Append, "C++: wxString::Append(const wchar_t *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pwz"), pybind11::arg("nLen"));
		cl.def("Append", (class wxString & (wxString::*)(const class wxCStrData &, unsigned long)) &wxString::Append, "C++: wxString::Append(const class wxCStrData &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"), pybind11::arg("nLen"));
		cl.def("Append", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxString::Append, "C++: wxString::Append(const class wxScopedCharTypeBuffer<char> &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"), pybind11::arg("nLen"));
		cl.def("Append", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxString::Append, "C++: wxString::Append(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"), pybind11::arg("nLen"));
		cl.def("Append", [](wxString &o, class wxUniChar const & a0) -> wxString & { return o.Append(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("Append", (class wxString & (wxString::*)(class wxUniChar, unsigned long)) &wxString::Append, "C++: wxString::Append(class wxUniChar, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"), pybind11::arg("count"));
		cl.def("Append", [](wxString &o, class wxUniCharRef const & a0) -> wxString & { return o.Append(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("Append", (class wxString & (wxString::*)(class wxUniCharRef, unsigned long)) &wxString::Append, "C++: wxString::Append(class wxUniCharRef, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"), pybind11::arg("count"));
		cl.def("Append", [](wxString &o, char const & a0) -> wxString & { return o.Append(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("Append", (class wxString & (wxString::*)(char, unsigned long)) &wxString::Append, "C++: wxString::Append(char, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"), pybind11::arg("count"));
		cl.def("Append", [](wxString &o, unsigned char const & a0) -> wxString & { return o.Append(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("Append", (class wxString & (wxString::*)(unsigned char, unsigned long)) &wxString::Append, "C++: wxString::Append(unsigned char, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"), pybind11::arg("count"));
		cl.def("Append", [](wxString &o, wchar_t const & a0) -> wxString & { return o.Append(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("Append", (class wxString & (wxString::*)(wchar_t, unsigned long)) &wxString::Append, "C++: wxString::Append(wchar_t, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"), pybind11::arg("count"));
		cl.def("Prepend", (class wxString & (wxString::*)(const class wxString &)) &wxString::Prepend, "C++: wxString::Prepend(const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("Cmp", (int (wxString::*)(const char *) const) &wxString::Cmp, "C++: wxString::Cmp(const char *) const --> int", pybind11::arg("psz"));
		cl.def("Cmp", (int (wxString::*)(const wchar_t *) const) &wxString::Cmp, "C++: wxString::Cmp(const wchar_t *) const --> int", pybind11::arg("pwz"));
		cl.def("Cmp", (int (wxString::*)(const class wxString &) const) &wxString::Cmp, "C++: wxString::Cmp(const class wxString &) const --> int", pybind11::arg("s"));
		cl.def("Cmp", (int (wxString::*)(const class wxCStrData &) const) &wxString::Cmp, "C++: wxString::Cmp(const class wxCStrData &) const --> int", pybind11::arg("s"));
		cl.def("Cmp", (int (wxString::*)(const class wxScopedCharTypeBuffer<char> &) const) &wxString::Cmp, "C++: wxString::Cmp(const class wxScopedCharTypeBuffer<char> &) const --> int", pybind11::arg("s"));
		cl.def("Cmp", (int (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &) const) &wxString::Cmp, "C++: wxString::Cmp(const class wxScopedCharTypeBuffer<wchar_t> &) const --> int", pybind11::arg("s"));
		cl.def("CmpNoCase", (int (wxString::*)(const class wxString &) const) &wxString::CmpNoCase, "C++: wxString::CmpNoCase(const class wxString &) const --> int", pybind11::arg("s"));
		cl.def("IsSameAs", [](wxString const &o, const class wxString & a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("str"));
		cl.def("IsSameAs", (bool (wxString::*)(const class wxString &, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(const class wxString &, bool) const --> bool", pybind11::arg("str"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, const char * a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("str"));
		cl.def("IsSameAs", (bool (wxString::*)(const char *, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(const char *, bool) const --> bool", pybind11::arg("str"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, const wchar_t * a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("str"));
		cl.def("IsSameAs", (bool (wxString::*)(const wchar_t *, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(const wchar_t *, bool) const --> bool", pybind11::arg("str"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, const class wxCStrData & a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("str"));
		cl.def("IsSameAs", (bool (wxString::*)(const class wxCStrData &, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(const class wxCStrData &, bool) const --> bool", pybind11::arg("str"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, const class wxScopedCharTypeBuffer<char> & a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("str"));
		cl.def("IsSameAs", (bool (wxString::*)(const class wxScopedCharTypeBuffer<char> &, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(const class wxScopedCharTypeBuffer<char> &, bool) const --> bool", pybind11::arg("str"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, const class wxScopedCharTypeBuffer<wchar_t> & a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("str"));
		cl.def("IsSameAs", (bool (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(const class wxScopedCharTypeBuffer<wchar_t> &, bool) const --> bool", pybind11::arg("str"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, class wxUniChar const & a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("c"));
		cl.def("IsSameAs", (bool (wxString::*)(class wxUniChar, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(class wxUniChar, bool) const --> bool", pybind11::arg("c"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, class wxUniCharRef const & a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("c"));
		cl.def("IsSameAs", (bool (wxString::*)(class wxUniCharRef, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(class wxUniCharRef, bool) const --> bool", pybind11::arg("c"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, char const & a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("c"));
		cl.def("IsSameAs", (bool (wxString::*)(char, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(char, bool) const --> bool", pybind11::arg("c"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, unsigned char const & a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("c"));
		cl.def("IsSameAs", (bool (wxString::*)(unsigned char, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(unsigned char, bool) const --> bool", pybind11::arg("c"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, wchar_t const & a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("c"));
		cl.def("IsSameAs", (bool (wxString::*)(wchar_t, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(wchar_t, bool) const --> bool", pybind11::arg("c"), pybind11::arg("compareWithCase"));
		cl.def("IsSameAs", [](wxString const &o, int const & a0) -> bool { return o.IsSameAs(a0); }, "", pybind11::arg("c"));
		cl.def("IsSameAs", (bool (wxString::*)(int, bool) const) &wxString::IsSameAs, "C++: wxString::IsSameAs(int, bool) const --> bool", pybind11::arg("c"), pybind11::arg("compareWithCase"));
		cl.def("Mid", [](wxString const &o, unsigned long const & a0) -> wxString { return o.Mid(a0); }, "", pybind11::arg("nFirst"));
		cl.def("Mid", (class wxString (wxString::*)(unsigned long, unsigned long) const) &wxString::Mid, "C++: wxString::Mid(unsigned long, unsigned long) const --> class wxString", pybind11::arg("nFirst"), pybind11::arg("nCount"));
		cl.def("__call__", (class wxString (wxString::*)(unsigned long, unsigned long) const) &wxString::operator(), "C++: wxString::operator()(unsigned long, unsigned long) const --> class wxString", pybind11::arg("start"), pybind11::arg("len"));
		cl.def("StartsWith", [](wxString const &o, const class wxString & a0) -> bool { return o.StartsWith(a0); }, "", pybind11::arg("prefix"));
		cl.def("StartsWith", (bool (wxString::*)(const class wxString &, class wxString *) const) &wxString::StartsWith, "C++: wxString::StartsWith(const class wxString &, class wxString *) const --> bool", pybind11::arg("prefix"), pybind11::arg("rest"));
		cl.def("EndsWith", [](wxString const &o, const class wxString & a0) -> bool { return o.EndsWith(a0); }, "", pybind11::arg("suffix"));
		cl.def("EndsWith", (bool (wxString::*)(const class wxString &, class wxString *) const) &wxString::EndsWith, "C++: wxString::EndsWith(const class wxString &, class wxString *) const --> bool", pybind11::arg("suffix"), pybind11::arg("rest"));
		cl.def("Left", (class wxString (wxString::*)(unsigned long) const) &wxString::Left, "C++: wxString::Left(unsigned long) const --> class wxString", pybind11::arg("nCount"));
		cl.def("Right", (class wxString (wxString::*)(unsigned long) const) &wxString::Right, "C++: wxString::Right(unsigned long) const --> class wxString", pybind11::arg("nCount"));
		cl.def("BeforeFirst", [](wxString const &o, class wxUniChar const & a0) -> wxString { return o.BeforeFirst(a0); }, "", pybind11::arg("ch"));
		cl.def("BeforeFirst", (class wxString (wxString::*)(class wxUniChar, class wxString *) const) &wxString::BeforeFirst, "C++: wxString::BeforeFirst(class wxUniChar, class wxString *) const --> class wxString", pybind11::arg("ch"), pybind11::arg("rest"));
		cl.def("BeforeLast", [](wxString const &o, class wxUniChar const & a0) -> wxString { return o.BeforeLast(a0); }, "", pybind11::arg("ch"));
		cl.def("BeforeLast", (class wxString (wxString::*)(class wxUniChar, class wxString *) const) &wxString::BeforeLast, "C++: wxString::BeforeLast(class wxUniChar, class wxString *) const --> class wxString", pybind11::arg("ch"), pybind11::arg("rest"));
		cl.def("AfterFirst", (class wxString (wxString::*)(class wxUniChar) const) &wxString::AfterFirst, "C++: wxString::AfterFirst(class wxUniChar) const --> class wxString", pybind11::arg("ch"));
		cl.def("AfterLast", (class wxString (wxString::*)(class wxUniChar) const) &wxString::AfterLast, "C++: wxString::AfterLast(class wxUniChar) const --> class wxString", pybind11::arg("ch"));
		cl.def("Before", (class wxString (wxString::*)(class wxUniChar) const) &wxString::Before, "C++: wxString::Before(class wxUniChar) const --> class wxString", pybind11::arg("ch"));
		cl.def("After", (class wxString (wxString::*)(class wxUniChar) const) &wxString::After, "C++: wxString::After(class wxUniChar) const --> class wxString", pybind11::arg("ch"));
		cl.def("MakeUpper", (class wxString & (wxString::*)()) &wxString::MakeUpper, "C++: wxString::MakeUpper() --> class wxString &", pybind11::return_value_policy::automatic);
		cl.def("Upper", (class wxString (wxString::*)() const) &wxString::Upper, "C++: wxString::Upper() const --> class wxString");
		cl.def("MakeLower", (class wxString & (wxString::*)()) &wxString::MakeLower, "C++: wxString::MakeLower() --> class wxString &", pybind11::return_value_policy::automatic);
		cl.def("Lower", (class wxString (wxString::*)() const) &wxString::Lower, "C++: wxString::Lower() const --> class wxString");
		cl.def("MakeCapitalized", (class wxString & (wxString::*)()) &wxString::MakeCapitalized, "C++: wxString::MakeCapitalized() --> class wxString &", pybind11::return_value_policy::automatic);
		cl.def("Capitalize", (class wxString (wxString::*)() const) &wxString::Capitalize, "C++: wxString::Capitalize() const --> class wxString");
		cl.def("Trim", [](wxString &o) -> wxString & { return o.Trim(); }, "", pybind11::return_value_policy::automatic);
		cl.def("Trim", (class wxString & (wxString::*)(bool)) &wxString::Trim, "C++: wxString::Trim(bool) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("bFromRight"));
		cl.def("Pad", [](wxString &o, unsigned long const & a0) -> wxString & { return o.Pad(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("nCount"));
		cl.def("Pad", [](wxString &o, unsigned long const & a0, class wxUniChar const & a1) -> wxString & { return o.Pad(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("nCount"), pybind11::arg("chPad"));
		cl.def("Pad", (class wxString & (wxString::*)(unsigned long, class wxUniChar, bool)) &wxString::Pad, "C++: wxString::Pad(unsigned long, class wxUniChar, bool) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nCount"), pybind11::arg("chPad"), pybind11::arg("bFromRight"));
		cl.def("Find", [](wxString const &o, class wxUniChar const & a0) -> int { return o.Find(a0); }, "", pybind11::arg("ch"));
		cl.def("Find", (int (wxString::*)(class wxUniChar, bool) const) &wxString::Find, "C++: wxString::Find(class wxUniChar, bool) const --> int", pybind11::arg("ch"), pybind11::arg("bFromEnd"));
		cl.def("Find", [](wxString const &o, class wxUniCharRef const & a0) -> int { return o.Find(a0); }, "", pybind11::arg("ch"));
		cl.def("Find", (int (wxString::*)(class wxUniCharRef, bool) const) &wxString::Find, "C++: wxString::Find(class wxUniCharRef, bool) const --> int", pybind11::arg("ch"), pybind11::arg("bFromEnd"));
		cl.def("Find", [](wxString const &o, char const & a0) -> int { return o.Find(a0); }, "", pybind11::arg("ch"));
		cl.def("Find", (int (wxString::*)(char, bool) const) &wxString::Find, "C++: wxString::Find(char, bool) const --> int", pybind11::arg("ch"), pybind11::arg("bFromEnd"));
		cl.def("Find", [](wxString const &o, unsigned char const & a0) -> int { return o.Find(a0); }, "", pybind11::arg("ch"));
		cl.def("Find", (int (wxString::*)(unsigned char, bool) const) &wxString::Find, "C++: wxString::Find(unsigned char, bool) const --> int", pybind11::arg("ch"), pybind11::arg("bFromEnd"));
		cl.def("Find", [](wxString const &o, wchar_t const & a0) -> int { return o.Find(a0); }, "", pybind11::arg("ch"));
		cl.def("Find", (int (wxString::*)(wchar_t, bool) const) &wxString::Find, "C++: wxString::Find(wchar_t, bool) const --> int", pybind11::arg("ch"), pybind11::arg("bFromEnd"));
		cl.def("Find", (int (wxString::*)(const class wxString &) const) &wxString::Find, "C++: wxString::Find(const class wxString &) const --> int", pybind11::arg("sub"));
		cl.def("Find", (int (wxString::*)(const char *) const) &wxString::Find, "C++: wxString::Find(const char *) const --> int", pybind11::arg("sub"));
		cl.def("Find", (int (wxString::*)(const wchar_t *) const) &wxString::Find, "C++: wxString::Find(const wchar_t *) const --> int", pybind11::arg("sub"));
		cl.def("Find", (int (wxString::*)(const class wxCStrData &) const) &wxString::Find, "C++: wxString::Find(const class wxCStrData &) const --> int", pybind11::arg("sub"));
		cl.def("Find", (int (wxString::*)(const class wxScopedCharTypeBuffer<char> &) const) &wxString::Find, "C++: wxString::Find(const class wxScopedCharTypeBuffer<char> &) const --> int", pybind11::arg("sub"));
		cl.def("Find", (int (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &) const) &wxString::Find, "C++: wxString::Find(const class wxScopedCharTypeBuffer<wchar_t> &) const --> int", pybind11::arg("sub"));
		cl.def("Replace", [](wxString &o, const class wxString & a0, const class wxString & a1) -> unsigned long { return o.Replace(a0, a1); }, "", pybind11::arg("strOld"), pybind11::arg("strNew"));
		cl.def("Replace", (unsigned long (wxString::*)(const class wxString &, const class wxString &, bool)) &wxString::Replace, "C++: wxString::Replace(const class wxString &, const class wxString &, bool) --> unsigned long", pybind11::arg("strOld"), pybind11::arg("strNew"), pybind11::arg("bReplaceAll"));
		cl.def("Matches", (bool (wxString::*)(const class wxString &) const) &wxString::Matches, "C++: wxString::Matches(const class wxString &) const --> bool", pybind11::arg("mask"));
		cl.def("ToLong", [](wxString const &o, long * a0) -> bool { return o.ToLong(a0); }, "", pybind11::arg("val"));
		cl.def("ToLong", (bool (wxString::*)(long *, int) const) &wxString::ToLong, "C++: wxString::ToLong(long *, int) const --> bool", pybind11::arg("val"), pybind11::arg("base"));
		cl.def("ToULong", [](wxString const &o, unsigned long * a0) -> bool { return o.ToULong(a0); }, "", pybind11::arg("val"));
		cl.def("ToULong", (bool (wxString::*)(unsigned long *, int) const) &wxString::ToULong, "C++: wxString::ToULong(unsigned long *, int) const --> bool", pybind11::arg("val"), pybind11::arg("base"));
		cl.def("ToLongLong", [](wxString const &o, long long * a0) -> bool { return o.ToLongLong(a0); }, "", pybind11::arg("val"));
		cl.def("ToLongLong", (bool (wxString::*)(long long *, int) const) &wxString::ToLongLong, "C++: wxString::ToLongLong(long long *, int) const --> bool", pybind11::arg("val"), pybind11::arg("base"));
		cl.def("ToULongLong", [](wxString const &o, unsigned long long * a0) -> bool { return o.ToULongLong(a0); }, "", pybind11::arg("val"));
		cl.def("ToULongLong", (bool (wxString::*)(unsigned long long *, int) const) &wxString::ToULongLong, "C++: wxString::ToULongLong(unsigned long long *, int) const --> bool", pybind11::arg("val"), pybind11::arg("base"));
		cl.def("ToDouble", (bool (wxString::*)(double *) const) &wxString::ToDouble, "C++: wxString::ToDouble(double *) const --> bool", pybind11::arg("val"));
		cl.def("ToCLong", [](wxString const &o, long * a0) -> bool { return o.ToCLong(a0); }, "", pybind11::arg("val"));
		cl.def("ToCLong", (bool (wxString::*)(long *, int) const) &wxString::ToCLong, "C++: wxString::ToCLong(long *, int) const --> bool", pybind11::arg("val"), pybind11::arg("base"));
		cl.def("ToCULong", [](wxString const &o, unsigned long * a0) -> bool { return o.ToCULong(a0); }, "", pybind11::arg("val"));
		cl.def("ToCULong", (bool (wxString::*)(unsigned long *, int) const) &wxString::ToCULong, "C++: wxString::ToCULong(unsigned long *, int) const --> bool", pybind11::arg("val"), pybind11::arg("base"));
		cl.def("ToCDouble", (bool (wxString::*)(double *) const) &wxString::ToCDouble, "C++: wxString::ToCDouble(double *) const --> bool", pybind11::arg("val"));
		cl.def_static("FromDouble", [](double const & a0) -> wxString { return wxString::FromDouble(a0); }, "", pybind11::arg("val"));
		cl.def_static("FromDouble", (class wxString (*)(double, int)) &wxString::FromDouble, "C++: wxString::FromDouble(double, int) --> class wxString", pybind11::arg("val"), pybind11::arg("precision"));
		cl.def_static("FromCDouble", [](double const & a0) -> wxString { return wxString::FromCDouble(a0); }, "", pybind11::arg("val"));
		cl.def_static("FromCDouble", (class wxString (*)(double, int)) &wxString::FromCDouble, "C++: wxString::FromCDouble(double, int) --> class wxString", pybind11::arg("val"), pybind11::arg("precision"));
		cl.def("Printf", (int (wxString::*)(const class wxFormatString &)) &wxString::Printf, "C++: wxString::Printf(const class wxFormatString &) --> int", pybind11::arg("f1"));
		cl.def_static("Format", (class wxString (*)(const class wxFormatString &)) &wxString::Format, "C++: wxString::Format(const class wxFormatString &) --> class wxString", pybind11::arg("f1"));
		cl.def("Alloc", (bool (wxString::*)(unsigned long)) &wxString::Alloc, "C++: wxString::Alloc(unsigned long) --> bool", pybind11::arg("nLen"));
		cl.def("Shrink", (bool (wxString::*)()) &wxString::Shrink, "C++: wxString::Shrink() --> bool");
		cl.def("SubString", (class wxString (wxString::*)(unsigned long, unsigned long) const) &wxString::SubString, "C++: wxString::SubString(unsigned long, unsigned long) const --> class wxString", pybind11::arg("from"), pybind11::arg("to"));
		cl.def("sprintf", (int (wxString::*)(const class wxFormatString &)) &wxString::sprintf, "C++: wxString::sprintf(const class wxFormatString &) --> int", pybind11::arg("f1"));
		cl.def("CompareTo", [](wxString const &o, const wchar_t * a0) -> int { return o.CompareTo(a0); }, "", pybind11::arg("psz"));
		cl.def("CompareTo", (int (wxString::*)(const wchar_t *, enum wxString::caseCompare) const) &wxString::CompareTo, "C++: wxString::CompareTo(const wchar_t *, enum wxString::caseCompare) const --> int", pybind11::arg("psz"), pybind11::arg("cmp"));
		cl.def("Length", (unsigned long (wxString::*)() const) &wxString::Length, "C++: wxString::Length() const --> unsigned long");
		cl.def("Freq", (int (wxString::*)(class wxUniChar) const) &wxString::Freq, "C++: wxString::Freq(class wxUniChar) const --> int", pybind11::arg("ch"));
		cl.def("LowerCase", (void (wxString::*)()) &wxString::LowerCase, "C++: wxString::LowerCase() --> void");
		cl.def("UpperCase", (void (wxString::*)()) &wxString::UpperCase, "C++: wxString::UpperCase() --> void");
		cl.def("Strip", [](wxString const &o) -> wxString { return o.Strip(); }, "");
		cl.def("Strip", (class wxString (wxString::*)(enum wxString::stripType) const) &wxString::Strip, "C++: wxString::Strip(enum wxString::stripType) const --> class wxString", pybind11::arg("w"));
		cl.def("Index", (unsigned long (wxString::*)(const wchar_t *) const) &wxString::Index, "C++: wxString::Index(const wchar_t *) const --> unsigned long", pybind11::arg("psz"));
		cl.def("Index", (unsigned long (wxString::*)(class wxUniChar) const) &wxString::Index, "C++: wxString::Index(class wxUniChar) const --> unsigned long", pybind11::arg("ch"));
		cl.def("Remove", (class wxString & (wxString::*)(unsigned long)) &wxString::Remove, "C++: wxString::Remove(unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pos"));
		cl.def("RemoveLast", [](wxString &o) -> wxString & { return o.RemoveLast(); }, "", pybind11::return_value_policy::automatic);
		cl.def("RemoveLast", (class wxString & (wxString::*)(unsigned long)) &wxString::RemoveLast, "C++: wxString::RemoveLast(unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("Remove", (class wxString & (wxString::*)(unsigned long, unsigned long)) &wxString::Remove, "C++: wxString::Remove(unsigned long, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nStart"), pybind11::arg("nLen"));
		cl.def("First", (int (wxString::*)(class wxUniChar) const) &wxString::First, "C++: wxString::First(class wxUniChar) const --> int", pybind11::arg("ch"));
		cl.def("First", (int (wxString::*)(class wxUniCharRef) const) &wxString::First, "C++: wxString::First(class wxUniCharRef) const --> int", pybind11::arg("ch"));
		cl.def("First", (int (wxString::*)(char) const) &wxString::First, "C++: wxString::First(char) const --> int", pybind11::arg("ch"));
		cl.def("First", (int (wxString::*)(unsigned char) const) &wxString::First, "C++: wxString::First(unsigned char) const --> int", pybind11::arg("ch"));
		cl.def("First", (int (wxString::*)(wchar_t) const) &wxString::First, "C++: wxString::First(wchar_t) const --> int", pybind11::arg("ch"));
		cl.def("First", (int (wxString::*)(const class wxString &) const) &wxString::First, "C++: wxString::First(const class wxString &) const --> int", pybind11::arg("str"));
		cl.def("Last", (int (wxString::*)(class wxUniChar) const) &wxString::Last, "C++: wxString::Last(class wxUniChar) const --> int", pybind11::arg("ch"));
		cl.def("Contains", (bool (wxString::*)(const class wxString &) const) &wxString::Contains, "C++: wxString::Contains(const class wxString &) const --> bool", pybind11::arg("str"));
		cl.def("IsNull", (bool (wxString::*)() const) &wxString::IsNull, "C++: wxString::IsNull() const --> bool");
		cl.def("append", (class wxString & (wxString::*)(const class wxString &, unsigned long, unsigned long)) &wxString::append, "C++: wxString::append(const class wxString &, unsigned long, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"), pybind11::arg("pos"), pybind11::arg("n"));
		cl.def("append", (class wxString & (wxString::*)(const class wxString &)) &wxString::append, "C++: wxString::append(const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("append", (class wxString & (wxString::*)(const char *)) &wxString::append, "C++: wxString::append(const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("sz"));
		cl.def("append", (class wxString & (wxString::*)(const wchar_t *)) &wxString::append, "C++: wxString::append(const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("sz"));
		cl.def("append", (class wxString & (wxString::*)(const char *, unsigned long)) &wxString::append, "C++: wxString::append(const char *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("sz"), pybind11::arg("n"));
		cl.def("append", (class wxString & (wxString::*)(const wchar_t *, unsigned long)) &wxString::append, "C++: wxString::append(const wchar_t *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("sz"), pybind11::arg("n"));
		cl.def("append", (class wxString & (wxString::*)(const class wxCStrData &)) &wxString::append, "C++: wxString::append(const class wxCStrData &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("append", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<char> &)) &wxString::append, "C++: wxString::append(const class wxScopedCharTypeBuffer<char> &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("append", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxString::append, "C++: wxString::append(const class wxScopedCharTypeBuffer<wchar_t> &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("append", (class wxString & (wxString::*)(const class wxCStrData &, unsigned long)) &wxString::append, "C++: wxString::append(const class wxCStrData &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"), pybind11::arg("n"));
		cl.def("append", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxString::append, "C++: wxString::append(const class wxScopedCharTypeBuffer<char> &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"), pybind11::arg("n"));
		cl.def("append", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxString::append, "C++: wxString::append(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"), pybind11::arg("n"));
		cl.def("append", (class wxString & (wxString::*)(unsigned long, class wxUniChar)) &wxString::append, "C++: wxString::append(unsigned long, class wxUniChar) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("append", (class wxString & (wxString::*)(unsigned long, class wxUniCharRef)) &wxString::append, "C++: wxString::append(unsigned long, class wxUniCharRef) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("append", (class wxString & (wxString::*)(unsigned long, char)) &wxString::append, "C++: wxString::append(unsigned long, char) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("append", (class wxString & (wxString::*)(unsigned long, unsigned char)) &wxString::append, "C++: wxString::append(unsigned long, unsigned char) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("append", (class wxString & (wxString::*)(unsigned long, wchar_t)) &wxString::append, "C++: wxString::append(unsigned long, wchar_t) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("append", (class wxString & (wxString::*)(class wxString::const_iterator, class wxString::const_iterator)) &wxString::append, "C++: wxString::append(class wxString::const_iterator, class wxString::const_iterator) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("append", (class wxString & (wxString::*)(const char *, const char *)) &wxString::append, "C++: wxString::append(const char *, const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("append", (class wxString & (wxString::*)(const wchar_t *, const wchar_t *)) &wxString::append, "C++: wxString::append(const wchar_t *, const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("append", (class wxString & (wxString::*)(const class wxCStrData &, const class wxCStrData &)) &wxString::append, "C++: wxString::append(const class wxCStrData &, const class wxCStrData &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxString &)) &wxString::assign, "C++: wxString::assign(const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxString &, unsigned long)) &wxString::assign, "C++: wxString::assign(const class wxString &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"), pybind11::arg("len"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxString &, unsigned long, unsigned long)) &wxString::assign, "C++: wxString::assign(const class wxString &, unsigned long, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"), pybind11::arg("pos"), pybind11::arg("n"));
		cl.def("assign", (class wxString & (wxString::*)(const char *)) &wxString::assign, "C++: wxString::assign(const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("sz"));
		cl.def("assign", (class wxString & (wxString::*)(const wchar_t *)) &wxString::assign, "C++: wxString::assign(const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("sz"));
		cl.def("assign", (class wxString & (wxString::*)(const char *, unsigned long)) &wxString::assign, "C++: wxString::assign(const char *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("sz"), pybind11::arg("n"));
		cl.def("assign", (class wxString & (wxString::*)(const wchar_t *, unsigned long)) &wxString::assign, "C++: wxString::assign(const wchar_t *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("sz"), pybind11::arg("n"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxCStrData &)) &wxString::assign, "C++: wxString::assign(const class wxCStrData &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<char> &)) &wxString::assign, "C++: wxString::assign(const class wxScopedCharTypeBuffer<char> &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxString::assign, "C++: wxString::assign(const class wxScopedCharTypeBuffer<wchar_t> &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxCStrData &, unsigned long)) &wxString::assign, "C++: wxString::assign(const class wxCStrData &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"), pybind11::arg("len"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long)) &wxString::assign, "C++: wxString::assign(const class wxScopedCharTypeBuffer<char> &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"), pybind11::arg("len"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long)) &wxString::assign, "C++: wxString::assign(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("str"), pybind11::arg("len"));
		cl.def("assign", (class wxString & (wxString::*)(unsigned long, class wxUniChar)) &wxString::assign, "C++: wxString::assign(unsigned long, class wxUniChar) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("assign", (class wxString & (wxString::*)(unsigned long, class wxUniCharRef)) &wxString::assign, "C++: wxString::assign(unsigned long, class wxUniCharRef) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("assign", (class wxString & (wxString::*)(unsigned long, char)) &wxString::assign, "C++: wxString::assign(unsigned long, char) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("assign", (class wxString & (wxString::*)(unsigned long, unsigned char)) &wxString::assign, "C++: wxString::assign(unsigned long, unsigned char) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("assign", (class wxString & (wxString::*)(unsigned long, wchar_t)) &wxString::assign, "C++: wxString::assign(unsigned long, wchar_t) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("assign", (class wxString & (wxString::*)(class wxString::const_iterator, class wxString::const_iterator)) &wxString::assign, "C++: wxString::assign(class wxString::const_iterator, class wxString::const_iterator) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", (class wxString & (wxString::*)(const char *, const char *)) &wxString::assign, "C++: wxString::assign(const char *, const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", (class wxString & (wxString::*)(const wchar_t *, const wchar_t *)) &wxString::assign, "C++: wxString::assign(const wchar_t *, const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", (class wxString & (wxString::*)(const class wxCStrData &, const class wxCStrData &)) &wxString::assign, "C++: wxString::assign(const class wxCStrData &, const class wxCStrData &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("compare", (int (wxString::*)(const class wxString &) const) &wxString::compare, "C++: wxString::compare(const class wxString &) const --> int", pybind11::arg("str"));
		cl.def("compare", (int (wxString::*)(const char *) const) &wxString::compare, "C++: wxString::compare(const char *) const --> int", pybind11::arg("sz"));
		cl.def("compare", (int (wxString::*)(const wchar_t *) const) &wxString::compare, "C++: wxString::compare(const wchar_t *) const --> int", pybind11::arg("sz"));
		cl.def("compare", (int (wxString::*)(const class wxCStrData &) const) &wxString::compare, "C++: wxString::compare(const class wxCStrData &) const --> int", pybind11::arg("str"));
		cl.def("compare", (int (wxString::*)(const class wxScopedCharTypeBuffer<char> &) const) &wxString::compare, "C++: wxString::compare(const class wxScopedCharTypeBuffer<char> &) const --> int", pybind11::arg("str"));
		cl.def("compare", (int (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &) const) &wxString::compare, "C++: wxString::compare(const class wxScopedCharTypeBuffer<wchar_t> &) const --> int", pybind11::arg("str"));
		cl.def("compare", (int (wxString::*)(unsigned long, unsigned long, const class wxString &) const) &wxString::compare, "C++: wxString::compare(unsigned long, unsigned long, const class wxString &) const --> int", pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("str"));
		cl.def("compare", (int (wxString::*)(unsigned long, unsigned long, const class wxString &, unsigned long, unsigned long) const) &wxString::compare, "C++: wxString::compare(unsigned long, unsigned long, const class wxString &, unsigned long, unsigned long) const --> int", pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("str"), pybind11::arg("nStart2"), pybind11::arg("nLen2"));
		cl.def("compare", [](wxString const &o, unsigned long const & a0, unsigned long const & a1, const char * a2) -> int { return o.compare(a0, a1, a2); }, "", pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("sz"));
		cl.def("compare", (int (wxString::*)(unsigned long, unsigned long, const char *, unsigned long) const) &wxString::compare, "C++: wxString::compare(unsigned long, unsigned long, const char *, unsigned long) const --> int", pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("sz"), pybind11::arg("nCount"));
		cl.def("compare", [](wxString const &o, unsigned long const & a0, unsigned long const & a1, const wchar_t * a2) -> int { return o.compare(a0, a1, a2); }, "", pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("sz"));
		cl.def("compare", (int (wxString::*)(unsigned long, unsigned long, const wchar_t *, unsigned long) const) &wxString::compare, "C++: wxString::compare(unsigned long, unsigned long, const wchar_t *, unsigned long) const --> int", pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("sz"), pybind11::arg("nCount"));
		cl.def("insert", (class wxString & (wxString::*)(unsigned long, const class wxString &)) &wxString::insert, "C++: wxString::insert(unsigned long, const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nPos"), pybind11::arg("str"));
		cl.def("insert", (class wxString & (wxString::*)(unsigned long, const class wxString &, unsigned long, unsigned long)) &wxString::insert, "C++: wxString::insert(unsigned long, const class wxString &, unsigned long, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nPos"), pybind11::arg("str"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("insert", (class wxString & (wxString::*)(unsigned long, const char *)) &wxString::insert, "C++: wxString::insert(unsigned long, const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nPos"), pybind11::arg("sz"));
		cl.def("insert", (class wxString & (wxString::*)(unsigned long, const wchar_t *)) &wxString::insert, "C++: wxString::insert(unsigned long, const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nPos"), pybind11::arg("sz"));
		cl.def("insert", (class wxString & (wxString::*)(unsigned long, const char *, unsigned long)) &wxString::insert, "C++: wxString::insert(unsigned long, const char *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nPos"), pybind11::arg("sz"), pybind11::arg("n"));
		cl.def("insert", (class wxString & (wxString::*)(unsigned long, const wchar_t *, unsigned long)) &wxString::insert, "C++: wxString::insert(unsigned long, const wchar_t *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nPos"), pybind11::arg("sz"), pybind11::arg("n"));
		cl.def("insert", (class wxString & (wxString::*)(unsigned long, unsigned long, class wxUniChar)) &wxString::insert, "C++: wxString::insert(unsigned long, unsigned long, class wxUniChar) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nPos"), pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("insert", (class wxString::iterator (wxString::*)(class wxString::iterator, class wxUniChar)) &wxString::insert, "C++: wxString::insert(class wxString::iterator, class wxUniChar) --> class wxString::iterator", pybind11::arg("it"), pybind11::arg("ch"));
		cl.def("insert", (void (wxString::*)(class wxString::iterator, class wxString::const_iterator, class wxString::const_iterator)) &wxString::insert, "C++: wxString::insert(class wxString::iterator, class wxString::const_iterator, class wxString::const_iterator) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("insert", (void (wxString::*)(class wxString::iterator, const char *, const char *)) &wxString::insert, "C++: wxString::insert(class wxString::iterator, const char *, const char *) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("insert", (void (wxString::*)(class wxString::iterator, const wchar_t *, const wchar_t *)) &wxString::insert, "C++: wxString::insert(class wxString::iterator, const wchar_t *, const wchar_t *) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("insert", (void (wxString::*)(class wxString::iterator, const class wxCStrData &, const class wxCStrData &)) &wxString::insert, "C++: wxString::insert(class wxString::iterator, const class wxCStrData &, const class wxCStrData &) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("insert", (void (wxString::*)(class wxString::iterator, unsigned long, class wxUniChar)) &wxString::insert, "C++: wxString::insert(class wxString::iterator, unsigned long, class wxUniChar) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("erase", [](wxString &o) -> wxString & { return o.erase(); }, "", pybind11::return_value_policy::automatic);
		cl.def("erase", [](wxString &o, unsigned long const & a0) -> wxString & { return o.erase(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pos"));
		cl.def("erase", (class wxString & (wxString::*)(unsigned long, unsigned long)) &wxString::erase, "C++: wxString::erase(unsigned long, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pos"), pybind11::arg("n"));
		cl.def("erase", (class wxString::iterator (wxString::*)(class wxString::iterator, class wxString::iterator)) &wxString::erase, "C++: wxString::erase(class wxString::iterator, class wxString::iterator) --> class wxString::iterator", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (class wxString::iterator (wxString::*)(class wxString::iterator)) &wxString::erase, "C++: wxString::erase(class wxString::iterator) --> class wxString::iterator", pybind11::arg("first"));
		cl.def("clear", (void (wxString::*)()) &wxString::clear, "C++: wxString::clear() --> void");
		cl.def("replace", (class wxString & (wxString::*)(unsigned long, unsigned long, const char *)) &wxString::replace, "C++: wxString::replace(unsigned long, unsigned long, const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("sz"));
		cl.def("replace", (class wxString & (wxString::*)(unsigned long, unsigned long, const wchar_t *)) &wxString::replace, "C++: wxString::replace(unsigned long, unsigned long, const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("sz"));
		cl.def("replace", (class wxString & (wxString::*)(unsigned long, unsigned long, const class wxString &)) &wxString::replace, "C++: wxString::replace(unsigned long, unsigned long, const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("str"));
		cl.def("replace", (class wxString & (wxString::*)(unsigned long, unsigned long, unsigned long, class wxUniChar)) &wxString::replace, "C++: wxString::replace(unsigned long, unsigned long, unsigned long, class wxUniChar) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("nCount"), pybind11::arg("ch"));
		cl.def("replace", (class wxString & (wxString::*)(unsigned long, unsigned long, const class wxString &, unsigned long, unsigned long)) &wxString::replace, "C++: wxString::replace(unsigned long, unsigned long, const class wxString &, unsigned long, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("str"), pybind11::arg("nStart2"), pybind11::arg("nLen2"));
		cl.def("replace", (class wxString & (wxString::*)(unsigned long, unsigned long, const char *, unsigned long)) &wxString::replace, "C++: wxString::replace(unsigned long, unsigned long, const char *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("sz"), pybind11::arg("nCount"));
		cl.def("replace", (class wxString & (wxString::*)(unsigned long, unsigned long, const wchar_t *, unsigned long)) &wxString::replace, "C++: wxString::replace(unsigned long, unsigned long, const wchar_t *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("sz"), pybind11::arg("nCount"));
		cl.def("replace", (class wxString & (wxString::*)(unsigned long, unsigned long, const class wxString &, unsigned long)) &wxString::replace, "C++: wxString::replace(unsigned long, unsigned long, const class wxString &, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nStart"), pybind11::arg("nLen"), pybind11::arg("s"), pybind11::arg("nCount"));
		cl.def("replace", (class wxString & (wxString::*)(class wxString::iterator, class wxString::iterator, const char *)) &wxString::replace, "C++: wxString::replace(class wxString::iterator, class wxString::iterator, const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"), pybind11::arg("s"));
		cl.def("replace", (class wxString & (wxString::*)(class wxString::iterator, class wxString::iterator, const wchar_t *)) &wxString::replace, "C++: wxString::replace(class wxString::iterator, class wxString::iterator, const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"), pybind11::arg("s"));
		cl.def("replace", (class wxString & (wxString::*)(class wxString::iterator, class wxString::iterator, const char *, unsigned long)) &wxString::replace, "C++: wxString::replace(class wxString::iterator, class wxString::iterator, const char *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"), pybind11::arg("s"), pybind11::arg("n"));
		cl.def("replace", (class wxString & (wxString::*)(class wxString::iterator, class wxString::iterator, const wchar_t *, unsigned long)) &wxString::replace, "C++: wxString::replace(class wxString::iterator, class wxString::iterator, const wchar_t *, unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"), pybind11::arg("s"), pybind11::arg("n"));
		cl.def("replace", (class wxString & (wxString::*)(class wxString::iterator, class wxString::iterator, const class wxString &)) &wxString::replace, "C++: wxString::replace(class wxString::iterator, class wxString::iterator, const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"), pybind11::arg("s"));
		cl.def("replace", (class wxString & (wxString::*)(class wxString::iterator, class wxString::iterator, unsigned long, class wxUniChar)) &wxString::replace, "C++: wxString::replace(class wxString::iterator, class wxString::iterator, unsigned long, class wxUniChar) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"), pybind11::arg("n"), pybind11::arg("ch"));
		cl.def("replace", (class wxString & (wxString::*)(class wxString::iterator, class wxString::iterator, class wxString::const_iterator, class wxString::const_iterator)) &wxString::replace, "C++: wxString::replace(class wxString::iterator, class wxString::iterator, class wxString::const_iterator, class wxString::const_iterator) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"), pybind11::arg("first1"), pybind11::arg("last1"));
		cl.def("replace", (class wxString & (wxString::*)(class wxString::iterator, class wxString::iterator, const char *, const char *)) &wxString::replace, "C++: wxString::replace(class wxString::iterator, class wxString::iterator, const char *, const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"), pybind11::arg("first1"), pybind11::arg("last1"));
		cl.def("replace", (class wxString & (wxString::*)(class wxString::iterator, class wxString::iterator, const wchar_t *, const wchar_t *)) &wxString::replace, "C++: wxString::replace(class wxString::iterator, class wxString::iterator, const wchar_t *, const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"), pybind11::arg("first1"), pybind11::arg("last1"));
		cl.def("swap", (void (wxString::*)(class wxString &)) &wxString::swap, "C++: wxString::swap(class wxString &) --> void", pybind11::arg("str"));
		cl.def("find", [](wxString const &o, const class wxString & a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("str"));
		cl.def("find", (unsigned long (wxString::*)(const class wxString &, unsigned long) const) &wxString::find, "C++: wxString::find(const class wxString &, unsigned long) const --> unsigned long", pybind11::arg("str"), pybind11::arg("nStart"));
		cl.def("find", [](wxString const &o, const char * a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("sz"));
		cl.def("find", [](wxString const &o, const char * a0, unsigned long const & a1) -> unsigned long { return o.find(a0, a1); }, "", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find", (unsigned long (wxString::*)(const char *, unsigned long, unsigned long) const) &wxString::find, "C++: wxString::find(const char *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find", [](wxString const &o, const wchar_t * a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("sz"));
		cl.def("find", [](wxString const &o, const wchar_t * a0, unsigned long const & a1) -> unsigned long { return o.find(a0, a1); }, "", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find", (unsigned long (wxString::*)(const wchar_t *, unsigned long, unsigned long) const) &wxString::find, "C++: wxString::find(const wchar_t *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find", [](wxString const &o, const class wxScopedCharTypeBuffer<char> & a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("s"));
		cl.def("find", [](wxString const &o, const class wxScopedCharTypeBuffer<char> & a0, unsigned long const & a1) -> unsigned long { return o.find(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("nStart"));
		cl.def("find", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const) &wxString::find, "C++: wxString::find(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("s"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find", [](wxString const &o, const class wxScopedCharTypeBuffer<wchar_t> & a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("s"));
		cl.def("find", [](wxString const &o, const class wxScopedCharTypeBuffer<wchar_t> & a0, unsigned long const & a1) -> unsigned long { return o.find(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("nStart"));
		cl.def("find", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const) &wxString::find, "C++: wxString::find(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("s"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find", [](wxString const &o, const class wxCStrData & a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("s"));
		cl.def("find", [](wxString const &o, const class wxCStrData & a0, unsigned long const & a1) -> unsigned long { return o.find(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("nStart"));
		cl.def("find", (unsigned long (wxString::*)(const class wxCStrData &, unsigned long, unsigned long) const) &wxString::find, "C++: wxString::find(const class wxCStrData &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("s"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find", [](wxString const &o, class wxUniChar const & a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("ch"));
		cl.def("find", (unsigned long (wxString::*)(class wxUniChar, unsigned long) const) &wxString::find, "C++: wxString::find(class wxUniChar, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find", [](wxString const &o, class wxUniCharRef const & a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("ch"));
		cl.def("find", (unsigned long (wxString::*)(class wxUniCharRef, unsigned long) const) &wxString::find, "C++: wxString::find(class wxUniCharRef, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find", [](wxString const &o, char const & a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("ch"));
		cl.def("find", (unsigned long (wxString::*)(char, unsigned long) const) &wxString::find, "C++: wxString::find(char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find", [](wxString const &o, unsigned char const & a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("ch"));
		cl.def("find", (unsigned long (wxString::*)(unsigned char, unsigned long) const) &wxString::find, "C++: wxString::find(unsigned char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find", [](wxString const &o, wchar_t const & a0) -> unsigned long { return o.find(a0); }, "", pybind11::arg("ch"));
		cl.def("find", (unsigned long (wxString::*)(wchar_t, unsigned long) const) &wxString::find, "C++: wxString::find(wchar_t, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("rfind", [](wxString const &o, const class wxString & a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("str"));
		cl.def("rfind", (unsigned long (wxString::*)(const class wxString &, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(const class wxString &, unsigned long) const --> unsigned long", pybind11::arg("str"), pybind11::arg("nStart"));
		cl.def("rfind", [](wxString const &o, const char * a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("sz"));
		cl.def("rfind", [](wxString const &o, const char * a0, unsigned long const & a1) -> unsigned long { return o.rfind(a0, a1); }, "", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("rfind", (unsigned long (wxString::*)(const char *, unsigned long, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(const char *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("rfind", [](wxString const &o, const wchar_t * a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("sz"));
		cl.def("rfind", [](wxString const &o, const wchar_t * a0, unsigned long const & a1) -> unsigned long { return o.rfind(a0, a1); }, "", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("rfind", (unsigned long (wxString::*)(const wchar_t *, unsigned long, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(const wchar_t *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("rfind", [](wxString const &o, const class wxScopedCharTypeBuffer<char> & a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("s"));
		cl.def("rfind", [](wxString const &o, const class wxScopedCharTypeBuffer<char> & a0, unsigned long const & a1) -> unsigned long { return o.rfind(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("nStart"));
		cl.def("rfind", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("s"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("rfind", [](wxString const &o, const class wxScopedCharTypeBuffer<wchar_t> & a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("s"));
		cl.def("rfind", [](wxString const &o, const class wxScopedCharTypeBuffer<wchar_t> & a0, unsigned long const & a1) -> unsigned long { return o.rfind(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("nStart"));
		cl.def("rfind", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("s"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("rfind", [](wxString const &o, const class wxCStrData & a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("s"));
		cl.def("rfind", [](wxString const &o, const class wxCStrData & a0, unsigned long const & a1) -> unsigned long { return o.rfind(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("nStart"));
		cl.def("rfind", (unsigned long (wxString::*)(const class wxCStrData &, unsigned long, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(const class wxCStrData &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("s"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("rfind", [](wxString const &o, class wxUniChar const & a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("ch"));
		cl.def("rfind", (unsigned long (wxString::*)(class wxUniChar, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(class wxUniChar, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("rfind", [](wxString const &o, class wxUniCharRef const & a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("ch"));
		cl.def("rfind", (unsigned long (wxString::*)(class wxUniCharRef, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(class wxUniCharRef, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("rfind", [](wxString const &o, char const & a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("ch"));
		cl.def("rfind", (unsigned long (wxString::*)(char, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("rfind", [](wxString const &o, unsigned char const & a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("ch"));
		cl.def("rfind", (unsigned long (wxString::*)(unsigned char, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(unsigned char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("rfind", [](wxString const &o, wchar_t const & a0) -> unsigned long { return o.rfind(a0); }, "", pybind11::arg("ch"));
		cl.def("rfind", (unsigned long (wxString::*)(wchar_t, unsigned long) const) &wxString::rfind, "C++: wxString::rfind(wchar_t, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_first_of", [](wxString const &o, const class wxString & a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("str"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const class wxString &, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const class wxString &, unsigned long) const --> unsigned long", pybind11::arg("str"), pybind11::arg("nStart"));
		cl.def("find_first_of", [](wxString const &o, const char * a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const char *, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const char *, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_first_of", [](wxString const &o, const wchar_t * a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const wchar_t *, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const char *, unsigned long, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const char *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const wchar_t *, unsigned long, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const wchar_t *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_first_of", [](wxString const &o, class wxUniChar const & a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("c"));
		cl.def("find_first_of", (unsigned long (wxString::*)(class wxUniChar, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(class wxUniChar, unsigned long) const --> unsigned long", pybind11::arg("c"), pybind11::arg("nStart"));
		cl.def("find_last_of", [](wxString const &o, const class wxString & a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("str"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const class wxString &, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const class wxString &, unsigned long) const --> unsigned long", pybind11::arg("str"), pybind11::arg("nStart"));
		cl.def("find_last_of", [](wxString const &o, const char * a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const char *, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const char *, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_last_of", [](wxString const &o, const wchar_t * a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const wchar_t *, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const char *, unsigned long, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const char *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const wchar_t *, unsigned long, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const wchar_t *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_last_of", [](wxString const &o, class wxUniChar const & a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("c"));
		cl.def("find_last_of", (unsigned long (wxString::*)(class wxUniChar, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(class wxUniChar, unsigned long) const --> unsigned long", pybind11::arg("c"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", [](wxString const &o, const class wxString & a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("str"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const class wxString &, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const class wxString &, unsigned long) const --> unsigned long", pybind11::arg("str"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", [](wxString const &o, const char * a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const char *, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const char *, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", [](wxString const &o, const wchar_t * a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const wchar_t *, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const char *, unsigned long, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const char *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const wchar_t *, unsigned long, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const wchar_t *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_first_not_of", [](wxString const &o, class wxUniChar const & a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("c"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(class wxUniChar, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(class wxUniChar, unsigned long) const --> unsigned long", pybind11::arg("c"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", [](wxString const &o, const class wxString & a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("str"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const class wxString &, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const class wxString &, unsigned long) const --> unsigned long", pybind11::arg("str"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", [](wxString const &o, const char * a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const char *, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const char *, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", [](wxString const &o, const wchar_t * a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const wchar_t *, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const wchar_t *, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const char *, unsigned long, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const char *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const wchar_t *, unsigned long, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const wchar_t *, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_last_not_of", [](wxString const &o, class wxUniChar const & a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("c"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(class wxUniChar, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(class wxUniChar, unsigned long) const --> unsigned long", pybind11::arg("c"), pybind11::arg("nStart"));
		cl.def("find_first_of", [](wxString const &o, class wxUniCharRef const & a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_first_of", (unsigned long (wxString::*)(class wxUniCharRef, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(class wxUniCharRef, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_first_of", [](wxString const &o, char const & a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_first_of", (unsigned long (wxString::*)(char, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_first_of", [](wxString const &o, unsigned char const & a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_first_of", (unsigned long (wxString::*)(unsigned char, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(unsigned char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_first_of", [](wxString const &o, wchar_t const & a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_first_of", (unsigned long (wxString::*)(wchar_t, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(wchar_t, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_last_of", [](wxString const &o, class wxUniCharRef const & a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_last_of", (unsigned long (wxString::*)(class wxUniCharRef, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(class wxUniCharRef, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_last_of", [](wxString const &o, char const & a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_last_of", (unsigned long (wxString::*)(char, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_last_of", [](wxString const &o, unsigned char const & a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_last_of", (unsigned long (wxString::*)(unsigned char, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(unsigned char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_last_of", [](wxString const &o, wchar_t const & a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_last_of", (unsigned long (wxString::*)(wchar_t, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(wchar_t, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", [](wxString const &o, class wxUniCharRef const & a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(class wxUniCharRef, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(class wxUniCharRef, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", [](wxString const &o, char const & a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(char, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", [](wxString const &o, unsigned char const & a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(unsigned char, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(unsigned char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", [](wxString const &o, wchar_t const & a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(wchar_t, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(wchar_t, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", [](wxString const &o, class wxUniCharRef const & a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(class wxUniCharRef, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(class wxUniCharRef, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", [](wxString const &o, char const & a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(char, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", [](wxString const &o, unsigned char const & a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(unsigned char, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(unsigned char, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", [](wxString const &o, wchar_t const & a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("ch"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(wchar_t, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(wchar_t, unsigned long) const --> unsigned long", pybind11::arg("ch"), pybind11::arg("nStart"));
		cl.def("find_first_of", [](wxString const &o, const class wxCStrData & a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const class wxCStrData &, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const class wxCStrData &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_first_of", [](wxString const &o, const class wxScopedCharTypeBuffer<char> & a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const class wxScopedCharTypeBuffer<char> &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_first_of", [](wxString const &o, const class wxScopedCharTypeBuffer<wchar_t> & a0) -> unsigned long { return o.find_first_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const class wxCStrData &, unsigned long, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const class wxCStrData &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_first_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const) &wxString::find_first_of, "C++: wxString::find_first_of(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_last_of", [](wxString const &o, const class wxCStrData & a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const class wxCStrData &, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const class wxCStrData &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_last_of", [](wxString const &o, const class wxScopedCharTypeBuffer<char> & a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const class wxScopedCharTypeBuffer<char> &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_last_of", [](wxString const &o, const class wxScopedCharTypeBuffer<wchar_t> & a0) -> unsigned long { return o.find_last_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const class wxCStrData &, unsigned long, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const class wxCStrData &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_last_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const) &wxString::find_last_of, "C++: wxString::find_last_of(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_first_not_of", [](wxString const &o, const class wxCStrData & a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const class wxCStrData &, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const class wxCStrData &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", [](wxString const &o, const class wxScopedCharTypeBuffer<char> & a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const class wxScopedCharTypeBuffer<char> &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", [](wxString const &o, const class wxScopedCharTypeBuffer<wchar_t> & a0) -> unsigned long { return o.find_first_not_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const class wxCStrData &, unsigned long, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const class wxCStrData &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_first_not_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const) &wxString::find_first_not_of, "C++: wxString::find_first_not_of(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_last_not_of", [](wxString const &o, const class wxCStrData & a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const class wxCStrData &, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const class wxCStrData &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", [](wxString const &o, const class wxScopedCharTypeBuffer<char> & a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const class wxScopedCharTypeBuffer<char> &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", [](wxString const &o, const class wxScopedCharTypeBuffer<wchar_t> & a0) -> unsigned long { return o.find_last_not_of(a0); }, "", pybind11::arg("sz"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const class wxCStrData &, unsigned long, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const class wxCStrData &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const class wxScopedCharTypeBuffer<char> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("find_last_not_of", (unsigned long (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const) &wxString::find_last_not_of, "C++: wxString::find_last_not_of(const class wxScopedCharTypeBuffer<wchar_t> &, unsigned long, unsigned long) const --> unsigned long", pybind11::arg("sz"), pybind11::arg("nStart"), pybind11::arg("n"));
		cl.def("__iadd__", (class wxString & (wxString::*)(const class wxString &)) &wxString::operator+=, "C++: wxString::operator+=(const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("__iadd__", (class wxString & (wxString::*)(const char *)) &wxString::operator+=, "C++: wxString::operator+=(const char *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("psz"));
		cl.def("__iadd__", (class wxString & (wxString::*)(const wchar_t *)) &wxString::operator+=, "C++: wxString::operator+=(const wchar_t *) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pwz"));
		cl.def("__iadd__", (class wxString & (wxString::*)(const class wxCStrData &)) &wxString::operator+=, "C++: wxString::operator+=(const class wxCStrData &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("__iadd__", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<char> &)) &wxString::operator+=, "C++: wxString::operator+=(const class wxScopedCharTypeBuffer<char> &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("__iadd__", (class wxString & (wxString::*)(const class wxScopedCharTypeBuffer<wchar_t> &)) &wxString::operator+=, "C++: wxString::operator+=(const class wxScopedCharTypeBuffer<wchar_t> &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("__iadd__", (class wxString & (wxString::*)(class wxUniChar)) &wxString::operator+=, "C++: wxString::operator+=(class wxUniChar) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("__iadd__", (class wxString & (wxString::*)(class wxUniCharRef)) &wxString::operator+=, "C++: wxString::operator+=(class wxUniCharRef) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("__iadd__", (class wxString & (wxString::*)(int)) &wxString::operator+=, "C++: wxString::operator+=(int) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("__iadd__", (class wxString & (wxString::*)(char)) &wxString::operator+=, "C++: wxString::operator+=(char) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("__iadd__", (class wxString & (wxString::*)(unsigned char)) &wxString::operator+=, "C++: wxString::operator+=(unsigned char) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));
		cl.def("__iadd__", (class wxString & (wxString::*)(wchar_t)) &wxString::operator+=, "C++: wxString::operator+=(wchar_t) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("ch"));

		{ // wxString::iterator file: line:1026
			auto & enclosing_class = cl;
			pybind11::class_<wxString::iterator, std::shared_ptr<wxString::iterator>> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxString::iterator(); } ) );
			cl.def( pybind11::init( [](wxString::iterator const &o){ return new wxString::iterator(o); } ) );
			cl.def("__getitem__", (class wxUniCharRef (wxString::iterator::*)(unsigned long) const) &wxString::iterator::operator[], "C++: wxString::iterator::operator[](unsigned long) const --> class wxUniCharRef", pybind11::arg("n"));
			cl.def("plus_plus", (class wxString::iterator & (wxString::iterator::*)()) &wxString::iterator::operator++, "C++: wxString::iterator::operator++() --> class wxString::iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (class wxString::iterator & (wxString::iterator::*)()) &wxString::iterator::operator--, "C++: wxString::iterator::operator--() --> class wxString::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxString::iterator (wxString::iterator::*)(int)) &wxString::iterator::operator++, "C++: wxString::iterator::operator++(int) --> class wxString::iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxString::iterator (wxString::iterator::*)(int)) &wxString::iterator::operator--, "C++: wxString::iterator::operator--(int) --> class wxString::iterator", pybind11::arg(""));
			cl.def("__iadd__", (class wxString::iterator & (wxString::iterator::*)(long)) &wxString::iterator::operator+=, "C++: wxString::iterator::operator+=(long) --> class wxString::iterator &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
			cl.def("__isub__", (class wxString::iterator & (wxString::iterator::*)(long)) &wxString::iterator::operator-=, "C++: wxString::iterator::operator-=(long) --> class wxString::iterator &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
			cl.def("__sub__", (long (wxString::iterator::*)(const class wxString::iterator &) const) &wxString::iterator::operator-, "C++: wxString::iterator::operator-(const class wxString::iterator &) const --> long", pybind11::arg("i"));
			cl.def("__eq__", (bool (wxString::iterator::*)(const class wxString::iterator &) const) &wxString::iterator::operator==, "C++: wxString::iterator::operator==(const class wxString::iterator &) const --> bool", pybind11::arg("i"));
			cl.def("__ne__", (bool (wxString::iterator::*)(const class wxString::iterator &) const) &wxString::iterator::operator!=, "C++: wxString::iterator::operator!=(const class wxString::iterator &) const --> bool", pybind11::arg("i"));
			cl.def("__mul__", (class wxUniCharRef (wxString::iterator::*)()) &wxString::iterator::operator*, "C++: wxString::iterator::operator*() --> class wxUniCharRef");
			cl.def("__add__", (class wxString::iterator (wxString::iterator::*)(long) const) &wxString::iterator::operator+, "C++: wxString::iterator::operator+(long) const --> class wxString::iterator", pybind11::arg("n"));
			cl.def("__sub__", (class wxString::iterator (wxString::iterator::*)(long) const) &wxString::iterator::operator-, "C++: wxString::iterator::operator-(long) const --> class wxString::iterator", pybind11::arg("n"));
			cl.def("__eq__", (bool (wxString::iterator::*)(const class wxString::const_iterator &) const) &wxString::iterator::operator==, "C++: wxString::iterator::operator==(const class wxString::const_iterator &) const --> bool", pybind11::arg("i"));
			cl.def("__ne__", (bool (wxString::iterator::*)(const class wxString::const_iterator &) const) &wxString::iterator::operator!=, "C++: wxString::iterator::operator!=(const class wxString::const_iterator &) const --> bool", pybind11::arg("i"));
		}

		{ // wxString::const_iterator file: line:1059
			auto & enclosing_class = cl;
			pybind11::class_<wxString::const_iterator, std::shared_ptr<wxString::const_iterator>> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxString::const_iterator(); } ) );
			cl.def( pybind11::init( [](wxString::const_iterator const &o){ return new wxString::const_iterator(o); } ) );
			cl.def( pybind11::init<const class wxString::iterator &>(), pybind11::arg("i") );

			cl.def("__getitem__", (class wxUniChar (wxString::const_iterator::*)(unsigned long) const) &wxString::const_iterator::operator[], "C++: wxString::const_iterator::operator[](unsigned long) const --> class wxUniChar", pybind11::arg("n"));
			cl.def("plus_plus", (class wxString::const_iterator & (wxString::const_iterator::*)()) &wxString::const_iterator::operator++, "C++: wxString::const_iterator::operator++() --> class wxString::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (class wxString::const_iterator & (wxString::const_iterator::*)()) &wxString::const_iterator::operator--, "C++: wxString::const_iterator::operator--() --> class wxString::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxString::const_iterator (wxString::const_iterator::*)(int)) &wxString::const_iterator::operator++, "C++: wxString::const_iterator::operator++(int) --> class wxString::const_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxString::const_iterator (wxString::const_iterator::*)(int)) &wxString::const_iterator::operator--, "C++: wxString::const_iterator::operator--(int) --> class wxString::const_iterator", pybind11::arg(""));
			cl.def("__iadd__", (class wxString::const_iterator & (wxString::const_iterator::*)(long)) &wxString::const_iterator::operator+=, "C++: wxString::const_iterator::operator+=(long) --> class wxString::const_iterator &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
			cl.def("__isub__", (class wxString::const_iterator & (wxString::const_iterator::*)(long)) &wxString::const_iterator::operator-=, "C++: wxString::const_iterator::operator-=(long) --> class wxString::const_iterator &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
			cl.def("__sub__", (long (wxString::const_iterator::*)(const class wxString::const_iterator &) const) &wxString::const_iterator::operator-, "C++: wxString::const_iterator::operator-(const class wxString::const_iterator &) const --> long", pybind11::arg("i"));
			cl.def("__eq__", (bool (wxString::const_iterator::*)(const class wxString::const_iterator &) const) &wxString::const_iterator::operator==, "C++: wxString::const_iterator::operator==(const class wxString::const_iterator &) const --> bool", pybind11::arg("i"));
			cl.def("__ne__", (bool (wxString::const_iterator::*)(const class wxString::const_iterator &) const) &wxString::const_iterator::operator!=, "C++: wxString::const_iterator::operator!=(const class wxString::const_iterator &) const --> bool", pybind11::arg("i"));
			cl.def("__mul__", (class wxUniChar (wxString::const_iterator::*)() const) &wxString::const_iterator::operator*, "C++: wxString::const_iterator::operator*() const --> class wxUniChar");
			cl.def("__add__", (class wxString::const_iterator (wxString::const_iterator::*)(long) const) &wxString::const_iterator::operator+, "C++: wxString::const_iterator::operator+(long) const --> class wxString::const_iterator", pybind11::arg("n"));
			cl.def("__sub__", (class wxString::const_iterator (wxString::const_iterator::*)(long) const) &wxString::const_iterator::operator-, "C++: wxString::const_iterator::operator-(long) const --> class wxString::const_iterator", pybind11::arg("n"));
			cl.def("assign", (class wxString::const_iterator & (wxString::const_iterator::*)(const class wxString::const_iterator &)) &wxString::const_iterator::operator=, "C++: wxString::const_iterator::operator=(const class wxString::const_iterator &) --> class wxString::const_iterator &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		}

		{ // wxString::reverse_iterator_impl file: line:1120
			auto & enclosing_class = cl;
			pybind11::class_<wxString::reverse_iterator_impl<wxString::iterator>, std::shared_ptr<wxString::reverse_iterator_impl<wxString::iterator>>> cl(enclosing_class, "reverse_iterator_impl_wxString_iterator_t", "");
			cl.def( pybind11::init( [](){ return new wxString::reverse_iterator_impl<wxString::iterator>(); } ) );
			cl.def( pybind11::init<class wxString::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init( [](wxString::reverse_iterator_impl<wxString::iterator> const &o){ return new wxString::reverse_iterator_impl<wxString::iterator>(o); } ) );
			cl.def("base", (class wxString::iterator (wxString::reverse_iterator_impl<wxString::iterator>::*)() const) &wxString::reverse_iterator_impl<wxString::iterator>::base, "C++: wxString::reverse_iterator_impl<wxString::iterator>::base() const --> class wxString::iterator");
			cl.def("__mul__", (class wxUniCharRef (wxString::reverse_iterator_impl<wxString::iterator>::*)() const) &wxString::reverse_iterator_impl<wxString::iterator>::operator*, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator*() const --> class wxUniCharRef");
			cl.def("__getitem__", (class wxUniCharRef (wxString::reverse_iterator_impl<wxString::iterator>::*)(unsigned long) const) &wxString::reverse_iterator_impl<wxString::iterator>::operator[], "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator[](unsigned long) const --> class wxUniCharRef", pybind11::arg("n"));
			cl.def("plus_plus", (class wxString::reverse_iterator_impl<class wxString::iterator> & (wxString::reverse_iterator_impl<wxString::iterator>::*)()) &wxString::reverse_iterator_impl<wxString::iterator>::operator++, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator++() --> class wxString::reverse_iterator_impl<class wxString::iterator> &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxString::reverse_iterator_impl<class wxString::iterator> (wxString::reverse_iterator_impl<wxString::iterator>::*)(int)) &wxString::reverse_iterator_impl<wxString::iterator>::operator++, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator++(int) --> class wxString::reverse_iterator_impl<class wxString::iterator>", pybind11::arg(""));
			cl.def("minus_minus", (class wxString::reverse_iterator_impl<class wxString::iterator> & (wxString::reverse_iterator_impl<wxString::iterator>::*)()) &wxString::reverse_iterator_impl<wxString::iterator>::operator--, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator--() --> class wxString::reverse_iterator_impl<class wxString::iterator> &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (class wxString::reverse_iterator_impl<class wxString::iterator> (wxString::reverse_iterator_impl<wxString::iterator>::*)(int)) &wxString::reverse_iterator_impl<wxString::iterator>::operator--, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator--(int) --> class wxString::reverse_iterator_impl<class wxString::iterator>", pybind11::arg(""));
			cl.def("__add__", (class wxString::reverse_iterator_impl<class wxString::iterator> (wxString::reverse_iterator_impl<wxString::iterator>::*)(long) const) &wxString::reverse_iterator_impl<wxString::iterator>::operator+, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator+(long) const --> class wxString::reverse_iterator_impl<class wxString::iterator>", pybind11::arg("n"));
			cl.def("__sub__", (class wxString::reverse_iterator_impl<class wxString::iterator> (wxString::reverse_iterator_impl<wxString::iterator>::*)(long) const) &wxString::reverse_iterator_impl<wxString::iterator>::operator-, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator-(long) const --> class wxString::reverse_iterator_impl<class wxString::iterator>", pybind11::arg("n"));
			cl.def("__iadd__", (class wxString::reverse_iterator_impl<class wxString::iterator> (wxString::reverse_iterator_impl<wxString::iterator>::*)(long)) &wxString::reverse_iterator_impl<wxString::iterator>::operator+=, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator+=(long) --> class wxString::reverse_iterator_impl<class wxString::iterator>", pybind11::arg("n"));
			cl.def("__isub__", (class wxString::reverse_iterator_impl<class wxString::iterator> (wxString::reverse_iterator_impl<wxString::iterator>::*)(long)) &wxString::reverse_iterator_impl<wxString::iterator>::operator-=, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator-=(long) --> class wxString::reverse_iterator_impl<class wxString::iterator>", pybind11::arg("n"));
			cl.def("__sub__", (unsigned int (wxString::reverse_iterator_impl<wxString::iterator>::*)(const class wxString::reverse_iterator_impl<class wxString::iterator> &) const) &wxString::reverse_iterator_impl<wxString::iterator>::operator-, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator-(const class wxString::reverse_iterator_impl<class wxString::iterator> &) const --> unsigned int", pybind11::arg("i"));
			cl.def("__eq__", (bool (wxString::reverse_iterator_impl<wxString::iterator>::*)(const class wxString::reverse_iterator_impl<class wxString::iterator> &) const) &wxString::reverse_iterator_impl<wxString::iterator>::operator==, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator==(const class wxString::reverse_iterator_impl<class wxString::iterator> &) const --> bool", pybind11::arg("ri"));
			cl.def("__ne__", (bool (wxString::reverse_iterator_impl<wxString::iterator>::*)(const class wxString::reverse_iterator_impl<class wxString::iterator> &) const) &wxString::reverse_iterator_impl<wxString::iterator>::operator!=, "C++: wxString::reverse_iterator_impl<wxString::iterator>::operator!=(const class wxString::reverse_iterator_impl<class wxString::iterator> &) const --> bool", pybind11::arg("ri"));
		}

		{ // wxString::reverse_iterator_impl file: line:1120
			auto & enclosing_class = cl;
			pybind11::class_<wxString::reverse_iterator_impl<wxString::const_iterator>, std::shared_ptr<wxString::reverse_iterator_impl<wxString::const_iterator>>> cl(enclosing_class, "reverse_iterator_impl_wxString_const_iterator_t", "");
			cl.def( pybind11::init( [](){ return new wxString::reverse_iterator_impl<wxString::const_iterator>(); } ) );
			cl.def( pybind11::init<class wxString::const_iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init( [](wxString::reverse_iterator_impl<wxString::const_iterator> const &o){ return new wxString::reverse_iterator_impl<wxString::const_iterator>(o); } ) );
			cl.def("base", (class wxString::const_iterator (wxString::reverse_iterator_impl<wxString::const_iterator>::*)() const) &wxString::reverse_iterator_impl<wxString::const_iterator>::base, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::base() const --> class wxString::const_iterator");
			cl.def("__mul__", (class wxUniChar (wxString::reverse_iterator_impl<wxString::const_iterator>::*)() const) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator*, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator*() const --> class wxUniChar");
			cl.def("__getitem__", (class wxUniChar (wxString::reverse_iterator_impl<wxString::const_iterator>::*)(unsigned long) const) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator[], "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator[](unsigned long) const --> class wxUniChar", pybind11::arg("n"));
			cl.def("plus_plus", (class wxString::reverse_iterator_impl<class wxString::const_iterator> & (wxString::reverse_iterator_impl<wxString::const_iterator>::*)()) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator++, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator++() --> class wxString::reverse_iterator_impl<class wxString::const_iterator> &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxString::reverse_iterator_impl<class wxString::const_iterator> (wxString::reverse_iterator_impl<wxString::const_iterator>::*)(int)) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator++, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator++(int) --> class wxString::reverse_iterator_impl<class wxString::const_iterator>", pybind11::arg(""));
			cl.def("minus_minus", (class wxString::reverse_iterator_impl<class wxString::const_iterator> & (wxString::reverse_iterator_impl<wxString::const_iterator>::*)()) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator--, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator--() --> class wxString::reverse_iterator_impl<class wxString::const_iterator> &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (class wxString::reverse_iterator_impl<class wxString::const_iterator> (wxString::reverse_iterator_impl<wxString::const_iterator>::*)(int)) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator--, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator--(int) --> class wxString::reverse_iterator_impl<class wxString::const_iterator>", pybind11::arg(""));
			cl.def("__add__", (class wxString::reverse_iterator_impl<class wxString::const_iterator> (wxString::reverse_iterator_impl<wxString::const_iterator>::*)(long) const) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator+, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator+(long) const --> class wxString::reverse_iterator_impl<class wxString::const_iterator>", pybind11::arg("n"));
			cl.def("__sub__", (class wxString::reverse_iterator_impl<class wxString::const_iterator> (wxString::reverse_iterator_impl<wxString::const_iterator>::*)(long) const) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator-, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator-(long) const --> class wxString::reverse_iterator_impl<class wxString::const_iterator>", pybind11::arg("n"));
			cl.def("__iadd__", (class wxString::reverse_iterator_impl<class wxString::const_iterator> (wxString::reverse_iterator_impl<wxString::const_iterator>::*)(long)) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator+=, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator+=(long) --> class wxString::reverse_iterator_impl<class wxString::const_iterator>", pybind11::arg("n"));
			cl.def("__isub__", (class wxString::reverse_iterator_impl<class wxString::const_iterator> (wxString::reverse_iterator_impl<wxString::const_iterator>::*)(long)) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator-=, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator-=(long) --> class wxString::reverse_iterator_impl<class wxString::const_iterator>", pybind11::arg("n"));
			cl.def("__sub__", (unsigned int (wxString::reverse_iterator_impl<wxString::const_iterator>::*)(const class wxString::reverse_iterator_impl<class wxString::const_iterator> &) const) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator-, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator-(const class wxString::reverse_iterator_impl<class wxString::const_iterator> &) const --> unsigned int", pybind11::arg("i"));
			cl.def("__eq__", (bool (wxString::reverse_iterator_impl<wxString::const_iterator>::*)(const class wxString::reverse_iterator_impl<class wxString::const_iterator> &) const) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator==, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator==(const class wxString::reverse_iterator_impl<class wxString::const_iterator> &) const --> bool", pybind11::arg("ri"));
			cl.def("__ne__", (bool (wxString::reverse_iterator_impl<wxString::const_iterator>::*)(const class wxString::reverse_iterator_impl<class wxString::const_iterator> &) const) &wxString::reverse_iterator_impl<wxString::const_iterator>::operator!=, "C++: wxString::reverse_iterator_impl<wxString::const_iterator>::operator!=(const class wxString::reverse_iterator_impl<class wxString::const_iterator> &) const --> bool", pybind11::arg("ri"));
		}

	}
}
