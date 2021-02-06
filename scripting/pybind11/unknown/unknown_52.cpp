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

// wxVariant file: line:103
struct PyCallBack_wxVariant : public wxVariant {
	using wxVariant::wxVariant;

	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariant *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxVariant::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariant *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxVariant::CloneRefData(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariant *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxVariant::GetClassInfo();
	}
};

void bind_unknown_unknown_52(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxVariant file: line:103
		pybind11::class_<wxVariant, std::shared_ptr<wxVariant>, PyCallBack_wxVariant, wxObject> cl(M(""), "wxVariant", "");
		cl.def( pybind11::init( [](){ return new wxVariant(); }, [](){ return new PyCallBack_wxVariant(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxVariant const &o){ return new PyCallBack_wxVariant(o); } ) );
		cl.def( pybind11::init( [](wxVariant const &o){ return new wxVariant(o); } ) );
		cl.def( pybind11::init( [](class wxVariantData * a0){ return new wxVariant(a0); }, [](class wxVariantData * a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<class wxVariantData *, const class wxString &>(), pybind11::arg("data"), pybind11::arg("name") );

		cl.def( pybind11::init<const class wxAny &>(), pybind11::arg("any") );

		cl.def( pybind11::init( [](double const & a0){ return new wxVariant(a0); }, [](double const & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<double, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](long const & a0){ return new wxVariant(a0); }, [](long const & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<long, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](int const & a0){ return new wxVariant(a0); }, [](int const & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<int, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](short const & a0){ return new wxVariant(a0); }, [](short const & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<short, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](bool const & a0){ return new wxVariant(a0); }, [](bool const & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<bool, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](const class wxDateTime & a0){ return new wxVariant(a0); }, [](const class wxDateTime & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const class wxDateTime &, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](const class wxString & a0){ return new wxVariant(a0); }, [](const class wxString & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const class wxString &, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](const char * a0){ return new wxVariant(a0); }, [](const char * a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const char *, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](const wchar_t * a0){ return new wxVariant(a0); }, [](const wchar_t * a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const wchar_t *, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](const class wxCStrData & a0){ return new wxVariant(a0); }, [](const class wxCStrData & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const class wxCStrData &, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](const class wxScopedCharTypeBuffer<char> & a0){ return new wxVariant(a0); }, [](const class wxScopedCharTypeBuffer<char> & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<char> &, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](const class wxScopedCharTypeBuffer<wchar_t> & a0){ return new wxVariant(a0); }, [](const class wxScopedCharTypeBuffer<wchar_t> & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const class wxScopedCharTypeBuffer<wchar_t> &, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](const class wxUniChar & a0){ return new wxVariant(a0); }, [](const class wxUniChar & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const class wxUniChar &, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](const class wxUniCharRef & a0){ return new wxVariant(a0); }, [](const class wxUniCharRef & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const class wxUniCharRef &, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](char const & a0){ return new wxVariant(a0); }, [](char const & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<char, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](wchar_t const & a0){ return new wxVariant(a0); }, [](wchar_t const & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<wchar_t, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](const class wxArrayString & a0){ return new wxVariant(a0); }, [](const class wxArrayString & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const class wxArrayString &, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def( pybind11::init( [](void * a0){ return new wxVariant(a0); }, [](void * a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<void *, const class wxString &>(), pybind11::arg("ptr"), pybind11::arg("name") );

		cl.def( pybind11::init( [](class wxObject * a0){ return new wxVariant(a0); }, [](class wxObject * a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<class wxObject *, const class wxString &>(), pybind11::arg("ptr"), pybind11::arg("name") );

		cl.def( pybind11::init( [](class wxLongLongNative const & a0){ return new wxVariant(a0); }, [](class wxLongLongNative const & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<class wxLongLongNative, const class wxString &>(), pybind11::arg(""), pybind11::arg("name") );

		cl.def( pybind11::init( [](class wxULongLongNative const & a0){ return new wxVariant(a0); }, [](class wxULongLongNative const & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<class wxULongLongNative, const class wxString &>(), pybind11::arg(""), pybind11::arg("name") );

		cl.def( pybind11::init( [](const class wxVariantList & a0){ return new wxVariant(a0); }, [](const class wxVariantList & a0){ return new PyCallBack_wxVariant(a0); } ), "doc");
		cl.def( pybind11::init<const class wxVariantList &, const class wxString &>(), pybind11::arg("val"), pybind11::arg("name") );

		cl.def("assign", (void (wxVariant::*)(const class wxVariant &)) &wxVariant::operator=, "C++: wxVariant::operator=(const class wxVariant &) --> void", pybind11::arg("variant"));
		cl.def("assign", (void (wxVariant::*)(class wxVariantData *)) &wxVariant::operator=, "C++: wxVariant::operator=(class wxVariantData *) --> void", pybind11::arg("variantData"));
		cl.def("__eq__", (bool (wxVariant::*)(const class wxVariant &) const) &wxVariant::operator==, "C++: wxVariant::operator==(const class wxVariant &) const --> bool", pybind11::arg("variant"));
		cl.def("__ne__", (bool (wxVariant::*)(const class wxVariant &) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(const class wxVariant &) const --> bool", pybind11::arg("variant"));
		cl.def("SetName", (void (wxVariant::*)(const class wxString &)) &wxVariant::SetName, "C++: wxVariant::SetName(const class wxString &) --> void", pybind11::arg("name"));
		cl.def("GetName", (const class wxString & (wxVariant::*)() const) &wxVariant::GetName, "C++: wxVariant::GetName() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("IsNull", (bool (wxVariant::*)() const) &wxVariant::IsNull, "C++: wxVariant::IsNull() const --> bool");
		cl.def("GetData", (class wxVariantData * (wxVariant::*)() const) &wxVariant::GetData, "C++: wxVariant::GetData() const --> class wxVariantData *", pybind11::return_value_policy::automatic);
		cl.def("SetData", (void (wxVariant::*)(class wxVariantData *)) &wxVariant::SetData, "C++: wxVariant::SetData(class wxVariantData *) --> void", pybind11::arg("data"));
		cl.def("Ref", (void (wxVariant::*)(const class wxVariant &)) &wxVariant::Ref, "C++: wxVariant::Ref(const class wxVariant &) --> void", pybind11::arg("clone"));
		cl.def("Unshare", (bool (wxVariant::*)()) &wxVariant::Unshare, "C++: wxVariant::Unshare() --> bool");
		cl.def("MakeNull", (void (wxVariant::*)()) &wxVariant::MakeNull, "C++: wxVariant::MakeNull() --> void");
		cl.def("Clear", (void (wxVariant::*)()) &wxVariant::Clear, "C++: wxVariant::Clear() --> void");
		cl.def("GetType", (class wxString (wxVariant::*)() const) &wxVariant::GetType, "C++: wxVariant::GetType() const --> class wxString");
		cl.def("IsType", (bool (wxVariant::*)(const class wxString &) const) &wxVariant::IsType, "C++: wxVariant::IsType(const class wxString &) const --> bool", pybind11::arg("type"));
		cl.def("IsValueKindOf", (bool (wxVariant::*)(const class wxClassInfo *) const) &wxVariant::IsValueKindOf, "C++: wxVariant::IsValueKindOf(const class wxClassInfo *) const --> bool", pybind11::arg("type"));
		cl.def("MakeString", (class wxString (wxVariant::*)() const) &wxVariant::MakeString, "C++: wxVariant::MakeString() const --> class wxString");
		cl.def("GetAny", (class wxAny (wxVariant::*)() const) &wxVariant::GetAny, "C++: wxVariant::GetAny() const --> class wxAny");
		cl.def("__eq__", (bool (wxVariant::*)(double) const) &wxVariant::operator==, "C++: wxVariant::operator==(double) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(double) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(double) const --> bool", pybind11::arg("value"));
		cl.def("assign", (void (wxVariant::*)(double)) &wxVariant::operator=, "C++: wxVariant::operator=(double) --> void", pybind11::arg("value"));
		cl.def("GetReal", (double (wxVariant::*)() const) &wxVariant::GetReal, "C++: wxVariant::GetReal() const --> double");
		cl.def("GetDouble", (double (wxVariant::*)() const) &wxVariant::GetDouble, "C++: wxVariant::GetDouble() const --> double");
		cl.def("__eq__", (bool (wxVariant::*)(long) const) &wxVariant::operator==, "C++: wxVariant::operator==(long) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(long) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(long) const --> bool", pybind11::arg("value"));
		cl.def("assign", (void (wxVariant::*)(long)) &wxVariant::operator=, "C++: wxVariant::operator=(long) --> void", pybind11::arg("value"));
		cl.def("GetInteger", (long (wxVariant::*)() const) &wxVariant::GetInteger, "C++: wxVariant::GetInteger() const --> long");
		cl.def("GetLong", (long (wxVariant::*)() const) &wxVariant::GetLong, "C++: wxVariant::GetLong() const --> long");
		cl.def("__eq__", (bool (wxVariant::*)(bool) const) &wxVariant::operator==, "C++: wxVariant::operator==(bool) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(bool) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(bool) const --> bool", pybind11::arg("value"));
		cl.def("assign", (void (wxVariant::*)(bool)) &wxVariant::operator=, "C++: wxVariant::operator=(bool) --> void", pybind11::arg("value"));
		cl.def("GetBool", (bool (wxVariant::*)() const) &wxVariant::GetBool, "C++: wxVariant::GetBool() const --> bool");
		cl.def("__eq__", (bool (wxVariant::*)(const class wxDateTime &) const) &wxVariant::operator==, "C++: wxVariant::operator==(const class wxDateTime &) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(const class wxDateTime &) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(const class wxDateTime &) const --> bool", pybind11::arg("value"));
		cl.def("assign", (void (wxVariant::*)(const class wxDateTime &)) &wxVariant::operator=, "C++: wxVariant::operator=(const class wxDateTime &) --> void", pybind11::arg("value"));
		cl.def("GetDateTime", (class wxDateTime (wxVariant::*)() const) &wxVariant::GetDateTime, "C++: wxVariant::GetDateTime() const --> class wxDateTime");
		cl.def("__eq__", (bool (wxVariant::*)(const class wxString &) const) &wxVariant::operator==, "C++: wxVariant::operator==(const class wxString &) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(const class wxString &) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(const class wxString &) const --> bool", pybind11::arg("value"));
		cl.def("assign", (class wxVariant & (wxVariant::*)(const class wxString &)) &wxVariant::operator=, "C++: wxVariant::operator=(const class wxString &) --> class wxVariant &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("assign", (class wxVariant & (wxVariant::*)(const char *)) &wxVariant::operator=, "C++: wxVariant::operator=(const char *) --> class wxVariant &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("assign", (class wxVariant & (wxVariant::*)(const wchar_t *)) &wxVariant::operator=, "C++: wxVariant::operator=(const wchar_t *) --> class wxVariant &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("assign", (class wxVariant & (wxVariant::*)(const class wxCStrData &)) &wxVariant::operator=, "C++: wxVariant::operator=(const class wxCStrData &) --> class wxVariant &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("GetString", (class wxString (wxVariant::*)() const) &wxVariant::GetString, "C++: wxVariant::GetString() const --> class wxString");
		cl.def("__eq__", (bool (wxVariant::*)(const class wxUniChar &) const) &wxVariant::operator==, "C++: wxVariant::operator==(const class wxUniChar &) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxVariant::*)(const class wxUniCharRef &) const) &wxVariant::operator==, "C++: wxVariant::operator==(const class wxUniCharRef &) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxVariant::*)(char) const) &wxVariant::operator==, "C++: wxVariant::operator==(char) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxVariant::*)(wchar_t) const) &wxVariant::operator==, "C++: wxVariant::operator==(wchar_t) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(const class wxUniChar &) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(const class wxUniChar &) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(const class wxUniCharRef &) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(const class wxUniCharRef &) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(char) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(char) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(wchar_t) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(wchar_t) const --> bool", pybind11::arg("value"));
		cl.def("assign", (class wxVariant & (wxVariant::*)(const class wxUniChar &)) &wxVariant::operator=, "C++: wxVariant::operator=(const class wxUniChar &) --> class wxVariant &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("assign", (class wxVariant & (wxVariant::*)(const class wxUniCharRef &)) &wxVariant::operator=, "C++: wxVariant::operator=(const class wxUniCharRef &) --> class wxVariant &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("assign", (class wxVariant & (wxVariant::*)(char)) &wxVariant::operator=, "C++: wxVariant::operator=(char) --> class wxVariant &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("assign", (class wxVariant & (wxVariant::*)(wchar_t)) &wxVariant::operator=, "C++: wxVariant::operator=(wchar_t) --> class wxVariant &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("GetChar", (class wxUniChar (wxVariant::*)() const) &wxVariant::GetChar, "C++: wxVariant::GetChar() const --> class wxUniChar");
		cl.def("__eq__", (bool (wxVariant::*)(const class wxArrayString &) const) &wxVariant::operator==, "C++: wxVariant::operator==(const class wxArrayString &) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(const class wxArrayString &) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(const class wxArrayString &) const --> bool", pybind11::arg("value"));
		cl.def("assign", (void (wxVariant::*)(const class wxArrayString &)) &wxVariant::operator=, "C++: wxVariant::operator=(const class wxArrayString &) --> void", pybind11::arg("value"));
		cl.def("GetArrayString", (class wxArrayString (wxVariant::*)() const) &wxVariant::GetArrayString, "C++: wxVariant::GetArrayString() const --> class wxArrayString");
		cl.def("__eq__", (bool (wxVariant::*)(void *) const) &wxVariant::operator==, "C++: wxVariant::operator==(void *) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(void *) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(void *) const --> bool", pybind11::arg("value"));
		cl.def("assign", (void (wxVariant::*)(void *)) &wxVariant::operator=, "C++: wxVariant::operator=(void *) --> void", pybind11::arg("value"));
		cl.def("GetVoidPtr", (void * (wxVariant::*)() const) &wxVariant::GetVoidPtr, "C++: wxVariant::GetVoidPtr() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("__eq__", (bool (wxVariant::*)(class wxObject *) const) &wxVariant::operator==, "C++: wxVariant::operator==(class wxObject *) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(class wxObject *) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(class wxObject *) const --> bool", pybind11::arg("value"));
		cl.def("assign", (void (wxVariant::*)(class wxObject *)) &wxVariant::operator=, "C++: wxVariant::operator=(class wxObject *) --> void", pybind11::arg("value"));
		cl.def("GetWxObjectPtr", (class wxObject * (wxVariant::*)() const) &wxVariant::GetWxObjectPtr, "C++: wxVariant::GetWxObjectPtr() const --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("__eq__", (bool (wxVariant::*)(class wxLongLongNative) const) &wxVariant::operator==, "C++: wxVariant::operator==(class wxLongLongNative) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(class wxLongLongNative) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(class wxLongLongNative) const --> bool", pybind11::arg("value"));
		cl.def("assign", (void (wxVariant::*)(class wxLongLongNative)) &wxVariant::operator=, "C++: wxVariant::operator=(class wxLongLongNative) --> void", pybind11::arg("value"));
		cl.def("GetLongLong", (class wxLongLongNative (wxVariant::*)() const) &wxVariant::GetLongLong, "C++: wxVariant::GetLongLong() const --> class wxLongLongNative");
		cl.def("__eq__", (bool (wxVariant::*)(class wxULongLongNative) const) &wxVariant::operator==, "C++: wxVariant::operator==(class wxULongLongNative) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(class wxULongLongNative) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(class wxULongLongNative) const --> bool", pybind11::arg("value"));
		cl.def("assign", (void (wxVariant::*)(class wxULongLongNative)) &wxVariant::operator=, "C++: wxVariant::operator=(class wxULongLongNative) --> void", pybind11::arg("value"));
		cl.def("GetULongLong", (class wxULongLongNative (wxVariant::*)() const) &wxVariant::GetULongLong, "C++: wxVariant::GetULongLong() const --> class wxULongLongNative");
		cl.def("__eq__", (bool (wxVariant::*)(const class wxVariantList &) const) &wxVariant::operator==, "C++: wxVariant::operator==(const class wxVariantList &) const --> bool", pybind11::arg("value"));
		cl.def("__ne__", (bool (wxVariant::*)(const class wxVariantList &) const) &wxVariant::operator!=, "C++: wxVariant::operator!=(const class wxVariantList &) const --> bool", pybind11::arg("value"));
		cl.def("assign", (void (wxVariant::*)(const class wxVariantList &)) &wxVariant::operator=, "C++: wxVariant::operator=(const class wxVariantList &) --> void", pybind11::arg("value"));
		cl.def("__getitem__", (class wxVariant & (wxVariant::*)(unsigned long)) &wxVariant::operator[], "C++: wxVariant::operator[](unsigned long) --> class wxVariant &", pybind11::return_value_policy::automatic, pybind11::arg("idx"));
		cl.def("GetList", (class wxVariantList & (wxVariant::*)() const) &wxVariant::GetList, "C++: wxVariant::GetList() const --> class wxVariantList &", pybind11::return_value_policy::automatic);
		cl.def("GetCount", (unsigned long (wxVariant::*)() const) &wxVariant::GetCount, "C++: wxVariant::GetCount() const --> unsigned long");
		cl.def("NullList", (void (wxVariant::*)()) &wxVariant::NullList, "C++: wxVariant::NullList() --> void");
		cl.def("Append", (void (wxVariant::*)(const class wxVariant &)) &wxVariant::Append, "C++: wxVariant::Append(const class wxVariant &) --> void", pybind11::arg("value"));
		cl.def("Insert", (void (wxVariant::*)(const class wxVariant &)) &wxVariant::Insert, "C++: wxVariant::Insert(const class wxVariant &) --> void", pybind11::arg("value"));
		cl.def("Member", (bool (wxVariant::*)(const class wxVariant &) const) &wxVariant::Member, "C++: wxVariant::Member(const class wxVariant &) const --> bool", pybind11::arg("value"));
		cl.def("Delete", (bool (wxVariant::*)(unsigned long)) &wxVariant::Delete, "C++: wxVariant::Delete(unsigned long) --> bool", pybind11::arg("item"));
		cl.def("ClearList", (void (wxVariant::*)()) &wxVariant::ClearList, "C++: wxVariant::ClearList() --> void");
		cl.def("Convert", (bool (wxVariant::*)(long *) const) &wxVariant::Convert, "C++: wxVariant::Convert(long *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(bool *) const) &wxVariant::Convert, "C++: wxVariant::Convert(bool *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(double *) const) &wxVariant::Convert, "C++: wxVariant::Convert(double *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(class wxString *) const) &wxVariant::Convert, "C++: wxVariant::Convert(class wxString *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(class wxUniChar *) const) &wxVariant::Convert, "C++: wxVariant::Convert(class wxUniChar *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(char *) const) &wxVariant::Convert, "C++: wxVariant::Convert(char *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(wchar_t *) const) &wxVariant::Convert, "C++: wxVariant::Convert(wchar_t *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(class wxDateTime *) const) &wxVariant::Convert, "C++: wxVariant::Convert(class wxDateTime *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(class wxLongLongNative *) const) &wxVariant::Convert, "C++: wxVariant::Convert(class wxLongLongNative *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(class wxULongLongNative *) const) &wxVariant::Convert, "C++: wxVariant::Convert(class wxULongLongNative *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(long long *) const) &wxVariant::Convert, "C++: wxVariant::Convert(long long *) const --> bool", pybind11::arg("value"));
		cl.def("Convert", (bool (wxVariant::*)(unsigned long long *) const) &wxVariant::Convert, "C++: wxVariant::Convert(unsigned long long *) const --> bool", pybind11::arg("value"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxVariant::*)() const) &wxVariant::GetClassInfo, "C++: wxVariant::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxVariant::wxCreateObject, "C++: wxVariant::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
}
