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

// wxAnyToVariantRegistration file: line:412
struct PyCallBack_wxAnyToVariantRegistration : public wxAnyToVariantRegistration {
	using wxAnyToVariantRegistration::wxAnyToVariantRegistration;

	class wxAnyValueType * GetAssociatedType() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyToVariantRegistration *>(this), "GetAssociatedType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxAnyValueType *>::value) {
				static pybind11::detail::override_caster_t<class wxAnyValueType *> caster;
				return pybind11::detail::cast_ref<class wxAnyValueType *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxAnyValueType *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyToVariantRegistration::GetAssociatedType\"");
	}
};

// wxAnyValueTypeImplVariantData file: line:612
struct PyCallBack_wxAnyValueTypeImplVariantData : public wxAnyValueTypeImplVariantData {
	using wxAnyValueTypeImplVariantData::wxAnyValueTypeImplVariantData;

	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplVariantData *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplVariantData::IsSameType(a0);
	}
	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplVariantData *>(this), "DeleteValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplVariantData::DeleteValue(a0);
	}
	void CopyBuffer(const union wxAnyValueBuffer & a0, union wxAnyValueBuffer & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplVariantData *>(this), "CopyBuffer");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAnyValueTypeImplVariantData::CopyBuffer(a0, a1);
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueTypeImplVariantData *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAnyValueTypeImplVariantData::ConvertValue(a0, a1, a2);
	}
};

// wxwxAnyListNode file: line:48
struct PyCallBack_wxwxAnyListNode : public wxwxAnyListNode {
	using wxwxAnyListNode::wxwxAnyListNode;

	void DeleteData() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxwxAnyListNode *>(this), "DeleteData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxwxAnyListNode::DeleteData();
	}
};

// wxAnyList file: line:1113
struct PyCallBack_wxAnyList : public wxAnyList {
	using wxAnyList::wxAnyList;

	class wxwxAnyListNode * Find(const class wxListKey & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyList *>(this), "Find");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxwxAnyListNode *>::value) {
				static pybind11::detail::override_caster_t<class wxwxAnyListNode *> caster;
				return pybind11::detail::cast_ref<class wxwxAnyListNode *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxwxAnyListNode *>(std::move(o));
		}
		return wxAnyList::Find(a0);
	}
	class wxNodeBase * CreateNode(class wxNodeBase * a0, class wxNodeBase * a1, void * a2, const class wxListKey & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyList *>(this), "CreateNode");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxNodeBase *>::value) {
				static pybind11::detail::override_caster_t<class wxNodeBase *> caster;
				return pybind11::detail::cast_ref<class wxNodeBase *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxNodeBase *>(std::move(o));
		}
		return wxAnyList::CreateNode(a0, a1, a2, a3);
	}
};

void bind_unknown_unknown_53(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxAnyToVariantRegistration file: line:412
		pybind11::class_<wxAnyToVariantRegistration, std::shared_ptr<wxAnyToVariantRegistration>, PyCallBack_wxAnyToVariantRegistration> cl(M(""), "wxAnyToVariantRegistration", "");
		cl.def(pybind11::init<PyCallBack_wxAnyToVariantRegistration const &>());
		cl.def("GetAssociatedType", (class wxAnyValueType * (wxAnyToVariantRegistration::*)()) &wxAnyToVariantRegistration::GetAssociatedType, "C++: wxAnyToVariantRegistration::GetAssociatedType() --> class wxAnyValueType *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxAnyToVariantRegistration & (wxAnyToVariantRegistration::*)(const class wxAnyToVariantRegistration &)) &wxAnyToVariantRegistration::operator=, "C++: wxAnyToVariantRegistration::operator=(const class wxAnyToVariantRegistration &) --> class wxAnyToVariantRegistration &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxAnyValueTypeImplVariantData file: line:612
		pybind11::class_<wxAnyValueTypeImplVariantData, std::shared_ptr<wxAnyValueTypeImplVariantData>, PyCallBack_wxAnyValueTypeImplVariantData, wxAnyValueTypeImplBase<wxVariantData *>> cl(M(""), "wxAnyValueTypeImplVariantData", "");
		cl.def( pybind11::init( [](){ return new wxAnyValueTypeImplVariantData(); }, [](){ return new PyCallBack_wxAnyValueTypeImplVariantData(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxAnyValueTypeImplVariantData const &o){ return new PyCallBack_wxAnyValueTypeImplVariantData(o); } ) );
		cl.def( pybind11::init( [](wxAnyValueTypeImplVariantData const &o){ return new wxAnyValueTypeImplVariantData(o); } ) );
		cl.def_static("IsSameClass", (bool (*)(const class wxAnyValueType *)) &wxAnyValueTypeImplVariantData::IsSameClass, "C++: wxAnyValueTypeImplVariantData::IsSameClass(const class wxAnyValueType *) --> bool", pybind11::arg("otherType"));
		cl.def("IsSameType", (bool (wxAnyValueTypeImplVariantData::*)(const class wxAnyValueType *) const) &wxAnyValueTypeImplVariantData::IsSameType, "C++: wxAnyValueTypeImplVariantData::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def_static("GetInstance", (class wxAnyValueType * (*)()) &wxAnyValueTypeImplVariantData::GetInstance, "C++: wxAnyValueTypeImplVariantData::GetInstance() --> class wxAnyValueType *", pybind11::return_value_policy::automatic);
		cl.def("DeleteValue", (void (wxAnyValueTypeImplVariantData::*)(union wxAnyValueBuffer &) const) &wxAnyValueTypeImplVariantData::DeleteValue, "C++: wxAnyValueTypeImplVariantData::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueTypeImplVariantData::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplVariantData::CopyBuffer, "C++: wxAnyValueTypeImplVariantData::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def_static("SetValue", (void (*)(class wxVariantData *, union wxAnyValueBuffer &)) &wxAnyValueTypeImplVariantData::SetValue, "C++: wxAnyValueTypeImplVariantData::SetValue(class wxVariantData *, union wxAnyValueBuffer &) --> void", pybind11::arg("value"), pybind11::arg("buf"));
		cl.def_static("GetValue", (class wxVariantData * (*)(const union wxAnyValueBuffer &)) &wxAnyValueTypeImplVariantData::GetValue, "C++: wxAnyValueTypeImplVariantData::GetValue(const union wxAnyValueBuffer &) --> class wxVariantData *", pybind11::return_value_policy::automatic, pybind11::arg("buf"));
		cl.def("ConvertValue", (bool (wxAnyValueTypeImplVariantData::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueTypeImplVariantData::ConvertValue, "C++: wxAnyValueTypeImplVariantData::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueTypeImplVariantData & (wxAnyValueTypeImplVariantData::*)(const class wxAnyValueTypeImplVariantData &)) &wxAnyValueTypeImplVariantData::operator=, "C++: wxAnyValueTypeImplVariantData::operator=(const class wxAnyValueTypeImplVariantData &) --> class wxAnyValueTypeImplVariantData &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxPreRegisterAnyToVariant(class wxAnyToVariantRegistration *) file: line:719
	M("").def("wxPreRegisterAnyToVariant", (void (*)(class wxAnyToVariantRegistration *)) &wxPreRegisterAnyToVariant, "C++: wxPreRegisterAnyToVariant(class wxAnyToVariantRegistration *) --> void", pybind11::arg("reg"));

	// wxConvertAnyToVariant(const class wxAny &, class wxVariant *) file: line:723
	M("").def("wxConvertAnyToVariant", (bool (*)(const class wxAny &, class wxVariant *)) &wxConvertAnyToVariant, "C++: wxConvertAnyToVariant(const class wxAny &, class wxVariant *) --> bool", pybind11::arg("any"), pybind11::arg("variant"));

	{ // wxAny file: line:735
		pybind11::class_<wxAny, std::shared_ptr<wxAny>> cl(M(""), "wxAny", "");
		cl.def( pybind11::init( [](){ return new wxAny(); } ) );
		cl.def( pybind11::init<const char *>(), pybind11::arg("value") );

		cl.def( pybind11::init<const wchar_t *>(), pybind11::arg("value") );

		cl.def( pybind11::init( [](wxAny const &o){ return new wxAny(o); } ) );
		cl.def( pybind11::init<const class wxVariant &>(), pybind11::arg("variant") );

		cl.def("GetAs", (bool (wxAny::*)(class wxString *) const) &wxAny::GetAs<wxString>, "C++: wxAny::GetAs(class wxString *) const --> bool", pybind11::arg("value"));
		cl.def("GetType", (const class wxAnyValueType * (wxAny::*)() const) &wxAny::GetType, "Returns the value type as wxAnyValueType instance.\n\n        \n You cannot reliably test whether two wxAnys are of\n                 same value type by simply comparing return values\n                 of wxAny::GetType(). Instead, use wxAny::HasSameType().\n\n        \n HasSameType()\n\nC++: wxAny::GetType() const --> const class wxAnyValueType *", pybind11::return_value_policy::automatic);
		cl.def("HasSameType", (bool (wxAny::*)(const class wxAny &) const) &wxAny::HasSameType, "Returns  if this and another wxAny have the same\n        value type.\n\nC++: wxAny::HasSameType(const class wxAny &) const --> bool", pybind11::arg("other"));
		cl.def("IsNull", (bool (wxAny::*)() const) &wxAny::IsNull, "Tests if wxAny is null (that is, whether there is no data).\n\nC++: wxAny::IsNull() const --> bool");
		cl.def("MakeNull", (void (wxAny::*)()) &wxAny::MakeNull, "Makes wxAny null (that is, clears it).\n\nC++: wxAny::MakeNull() --> void");
		cl.def("assign", (class wxAny & (wxAny::*)(const class wxAny &)) &wxAny::operator=, "C++: wxAny::operator=(const class wxAny &) --> class wxAny &", pybind11::return_value_policy::automatic, pybind11::arg("any"));
		cl.def("assign", (class wxAny & (wxAny::*)(const class wxVariant &)) &wxAny::operator=, "C++: wxAny::operator=(const class wxVariant &) --> class wxAny &", pybind11::return_value_policy::automatic, pybind11::arg("variant"));
		cl.def("assign", (class wxAny & (wxAny::*)(const char *)) &wxAny::operator=, "C++: wxAny::operator=(const char *) --> class wxAny &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("assign", (class wxAny & (wxAny::*)(const wchar_t *)) &wxAny::operator=, "C++: wxAny::operator=(const wchar_t *) --> class wxAny &", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(const class wxString &) const) &wxAny::operator==, "Equality operators.\n\nC++: wxAny::operator==(const class wxString &) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(const char *) const) &wxAny::operator==, "C++: wxAny::operator==(const char *) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(const wchar_t *) const) &wxAny::operator==, "C++: wxAny::operator==(const wchar_t *) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(signed char) const) &wxAny::operator==, "C++: wxAny::operator==(signed char) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(unsigned char) const) &wxAny::operator==, "C++: wxAny::operator==(unsigned char) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(short) const) &wxAny::operator==, "C++: wxAny::operator==(short) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(unsigned short) const) &wxAny::operator==, "C++: wxAny::operator==(unsigned short) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(int) const) &wxAny::operator==, "C++: wxAny::operator==(int) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(unsigned int) const) &wxAny::operator==, "C++: wxAny::operator==(unsigned int) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(long) const) &wxAny::operator==, "C++: wxAny::operator==(long) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(unsigned long) const) &wxAny::operator==, "C++: wxAny::operator==(unsigned long) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(long long) const) &wxAny::operator==, "C++: wxAny::operator==(long long) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(unsigned long long) const) &wxAny::operator==, "C++: wxAny::operator==(unsigned long long) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(float) const) &wxAny::operator==, "C++: wxAny::operator==(float) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(double) const) &wxAny::operator==, "C++: wxAny::operator==(double) const --> bool", pybind11::arg("value"));
		cl.def("__eq__", (bool (wxAny::*)(bool) const) &wxAny::operator==, "C++: wxAny::operator==(bool) const --> bool", pybind11::arg("value"));
		cl.def("As", (class wxString (wxAny::*)(class wxString *) const) &wxAny::As, "C++: wxAny::As(class wxString *) const --> class wxString", pybind11::arg(""));
		cl.def("GetAs", (bool (wxAny::*)(class wxVariant *) const) &wxAny::GetAs, "C++: wxAny::GetAs(class wxVariant *) const --> bool", pybind11::arg("value"));
	}
	{ // wxwxAnyListNode file: line:48
		pybind11::class_<wxwxAnyListNode, std::shared_ptr<wxwxAnyListNode>, PyCallBack_wxwxAnyListNode, wxNodeBase> cl(M(""), "wxwxAnyListNode", "");
		cl.def( pybind11::init( [](){ return new wxwxAnyListNode(); }, [](){ return new PyCallBack_wxwxAnyListNode(); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0){ return new wxwxAnyListNode(a0); }, [](class wxListBase * a0){ return new PyCallBack_wxwxAnyListNode(a0); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxwxAnyListNode * a1){ return new wxwxAnyListNode(a0, a1); }, [](class wxListBase * a0, class wxwxAnyListNode * a1){ return new PyCallBack_wxwxAnyListNode(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxwxAnyListNode * a1, class wxwxAnyListNode * a2){ return new wxwxAnyListNode(a0, a1, a2); }, [](class wxListBase * a0, class wxwxAnyListNode * a1, class wxwxAnyListNode * a2){ return new PyCallBack_wxwxAnyListNode(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxwxAnyListNode * a1, class wxwxAnyListNode * a2, class wxAny * a3){ return new wxwxAnyListNode(a0, a1, a2, a3); }, [](class wxListBase * a0, class wxwxAnyListNode * a1, class wxwxAnyListNode * a2, class wxAny * a3){ return new PyCallBack_wxwxAnyListNode(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<class wxListBase *, class wxwxAnyListNode *, class wxwxAnyListNode *, class wxAny *, const class wxListKey &>(), pybind11::arg("list"), pybind11::arg("previous"), pybind11::arg("next"), pybind11::arg("data"), pybind11::arg("key") );

		cl.def("GetNext", (class wxwxAnyListNode * (wxwxAnyListNode::*)() const) &wxwxAnyListNode::GetNext, "C++: wxwxAnyListNode::GetNext() const --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetPrevious", (class wxwxAnyListNode * (wxwxAnyListNode::*)() const) &wxwxAnyListNode::GetPrevious, "C++: wxwxAnyListNode::GetPrevious() const --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetData", (class wxAny * (wxwxAnyListNode::*)() const) &wxwxAnyListNode::GetData, "C++: wxwxAnyListNode::GetData() const --> class wxAny *", pybind11::return_value_policy::automatic);
		cl.def("SetData", (void (wxwxAnyListNode::*)(class wxAny *)) &wxwxAnyListNode::SetData, "C++: wxwxAnyListNode::SetData(class wxAny *) --> void", pybind11::arg("data"));
	}
	{ // wxAnyList file: line:1113
		pybind11::class_<wxAnyList, std::shared_ptr<wxAnyList>, PyCallBack_wxAnyList, wxListBase> cl(M(""), "wxAnyList", "");
		cl.def( pybind11::init( [](){ return new wxAnyList(); }, [](){ return new PyCallBack_wxAnyList(); } ), "doc");
		cl.def( pybind11::init<enum wxKeyType>(), pybind11::arg("keyType") );

		cl.def( pybind11::init( [](PyCallBack_wxAnyList const &o){ return new PyCallBack_wxAnyList(o); } ) );
		cl.def( pybind11::init( [](wxAnyList const &o){ return new wxAnyList(o); } ) );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxAnyList(a0); }, [](unsigned long const & a0){ return new PyCallBack_wxAnyList(a0); } ), "doc");
		cl.def( pybind11::init<unsigned long, class wxAny *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const class wxAnyList::const_iterator &, const class wxAnyList::const_iterator &>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def("assign", (class wxAnyList & (wxAnyList::*)(const class wxAnyList &)) &wxAnyList::operator=, "C++: wxAnyList::operator=(const class wxAnyList &) --> class wxAnyList &", pybind11::return_value_policy::automatic, pybind11::arg("list"));
		cl.def("GetFirst", (class wxwxAnyListNode * (wxAnyList::*)() const) &wxAnyList::GetFirst, "C++: wxAnyList::GetFirst() const --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetLast", (class wxwxAnyListNode * (wxAnyList::*)() const) &wxAnyList::GetLast, "C++: wxAnyList::GetLast() const --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic);
		cl.def("Item", (class wxwxAnyListNode * (wxAnyList::*)(unsigned long) const) &wxAnyList::Item, "C++: wxAnyList::Item(unsigned long) const --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("__getitem__", (class wxAny * (wxAnyList::*)(unsigned long) const) &wxAnyList::operator[], "C++: wxAnyList::operator[](unsigned long) const --> class wxAny *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("Append", (class wxwxAnyListNode * (wxAnyList::*)(class wxAny *)) &wxAnyList::Append, "C++: wxAnyList::Append(class wxAny *) --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxwxAnyListNode * (wxAnyList::*)(class wxAny *)) &wxAnyList::Insert, "C++: wxAnyList::Insert(class wxAny *) --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxwxAnyListNode * (wxAnyList::*)(unsigned long, class wxAny *)) &wxAnyList::Insert, "C++: wxAnyList::Insert(unsigned long, class wxAny *) --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic, pybind11::arg("pos"), pybind11::arg("object"));
		cl.def("Insert", (class wxwxAnyListNode * (wxAnyList::*)(class wxwxAnyListNode *, class wxAny *)) &wxAnyList::Insert, "C++: wxAnyList::Insert(class wxwxAnyListNode *, class wxAny *) --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic, pybind11::arg("prev"), pybind11::arg("object"));
		cl.def("Append", (class wxwxAnyListNode * (wxAnyList::*)(long, void *)) &wxAnyList::Append, "C++: wxAnyList::Append(long, void *) --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("Append", (class wxwxAnyListNode * (wxAnyList::*)(const wchar_t *, void *)) &wxAnyList::Append, "C++: wxAnyList::Append(const wchar_t *, void *) --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("DetachNode", (class wxwxAnyListNode * (wxAnyList::*)(class wxwxAnyListNode *)) &wxAnyList::DetachNode, "C++: wxAnyList::DetachNode(class wxwxAnyListNode *) --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic, pybind11::arg("node"));
		cl.def("DeleteNode", (bool (wxAnyList::*)(class wxwxAnyListNode *)) &wxAnyList::DeleteNode, "C++: wxAnyList::DeleteNode(class wxwxAnyListNode *) --> bool", pybind11::arg("node"));
		cl.def("DeleteObject", (bool (wxAnyList::*)(class wxAny *)) &wxAnyList::DeleteObject, "C++: wxAnyList::DeleteObject(class wxAny *) --> bool", pybind11::arg("object"));
		cl.def("Erase", (void (wxAnyList::*)(class wxwxAnyListNode *)) &wxAnyList::Erase, "C++: wxAnyList::Erase(class wxwxAnyListNode *) --> void", pybind11::arg("it"));
		cl.def("Find", (class wxwxAnyListNode * (wxAnyList::*)(const class wxAny *) const) &wxAnyList::Find, "C++: wxAnyList::Find(const class wxAny *) const --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Find", (class wxwxAnyListNode * (wxAnyList::*)(const class wxListKey &) const) &wxAnyList::Find, "C++: wxAnyList::Find(const class wxListKey &) const --> class wxwxAnyListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("Member", (bool (wxAnyList::*)(const class wxAny *) const) &wxAnyList::Member, "C++: wxAnyList::Member(const class wxAny *) const --> bool", pybind11::arg("object"));
		cl.def("IndexOf", (int (wxAnyList::*)(class wxAny *) const) &wxAnyList::IndexOf, "C++: wxAnyList::IndexOf(class wxAny *) const --> int", pybind11::arg("object"));
		cl.def("begin", (class wxAnyList::iterator (wxAnyList::*)()) &wxAnyList::begin, "C++: wxAnyList::begin() --> class wxAnyList::iterator");
		cl.def("end", (class wxAnyList::iterator (wxAnyList::*)()) &wxAnyList::end, "C++: wxAnyList::end() --> class wxAnyList::iterator");
		cl.def("rbegin", (class wxAnyList::reverse_iterator (wxAnyList::*)()) &wxAnyList::rbegin, "C++: wxAnyList::rbegin() --> class wxAnyList::reverse_iterator");
		cl.def("rend", (class wxAnyList::reverse_iterator (wxAnyList::*)()) &wxAnyList::rend, "C++: wxAnyList::rend() --> class wxAnyList::reverse_iterator");
		cl.def("resize", [](wxAnyList &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxAnyList::*)(unsigned long, class wxAny *)) &wxAnyList::resize, "C++: wxAnyList::resize(unsigned long, class wxAny *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("size", (unsigned long (wxAnyList::*)() const) &wxAnyList::size, "C++: wxAnyList::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxAnyList::*)() const) &wxAnyList::max_size, "C++: wxAnyList::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxAnyList::*)() const) &wxAnyList::empty, "C++: wxAnyList::empty() const --> bool");
		cl.def("front", (class wxAny *& (wxAnyList::*)()) &wxAnyList::front, "C++: wxAnyList::front() --> class wxAny *&", pybind11::return_value_policy::automatic);
		cl.def("back", (class wxAny *& (wxAnyList::*)()) &wxAnyList::back, "C++: wxAnyList::back() --> class wxAny *&", pybind11::return_value_policy::automatic);
		cl.def("push_front", [](wxAnyList &o) -> void { return o.push_front(); }, "");
		cl.def("push_front", (void (wxAnyList::*)(class wxAny *const &)) &wxAnyList::push_front, "C++: wxAnyList::push_front(class wxAny *const &) --> void", pybind11::arg("v"));
		cl.def("pop_front", (void (wxAnyList::*)()) &wxAnyList::pop_front, "C++: wxAnyList::pop_front() --> void");
		cl.def("push_back", [](wxAnyList &o) -> void { return o.push_back(); }, "");
		cl.def("push_back", (void (wxAnyList::*)(class wxAny *const &)) &wxAnyList::push_back, "C++: wxAnyList::push_back(class wxAny *const &) --> void", pybind11::arg("v"));
		cl.def("pop_back", (void (wxAnyList::*)()) &wxAnyList::pop_back, "C++: wxAnyList::pop_back() --> void");
		cl.def("assign", (void (wxAnyList::*)(class wxAnyList::const_iterator, const class wxAnyList::const_iterator &)) &wxAnyList::assign, "C++: wxAnyList::assign(class wxAnyList::const_iterator, const class wxAnyList::const_iterator &) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", [](wxAnyList &o, unsigned long const & a0) -> void { return o.assign(a0); }, "", pybind11::arg("n"));
		cl.def("assign", (void (wxAnyList::*)(unsigned long, class wxAny *const &)) &wxAnyList::assign, "C++: wxAnyList::assign(unsigned long, class wxAny *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (class wxAnyList::iterator (wxAnyList::*)(const class wxAnyList::iterator &, class wxAny *const &)) &wxAnyList::insert, "C++: wxAnyList::insert(const class wxAnyList::iterator &, class wxAny *const &) --> class wxAnyList::iterator", pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxAnyList::*)(const class wxAnyList::iterator &, unsigned long, class wxAny *const &)) &wxAnyList::insert, "C++: wxAnyList::insert(const class wxAnyList::iterator &, unsigned long, class wxAny *const &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (void (wxAnyList::*)(const class wxAnyList::iterator &, class wxAnyList::const_iterator, const class wxAnyList::const_iterator &)) &wxAnyList::insert, "C++: wxAnyList::insert(const class wxAnyList::iterator &, class wxAnyList::const_iterator, const class wxAnyList::const_iterator &) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (class wxAnyList::iterator (wxAnyList::*)(const class wxAnyList::iterator &)) &wxAnyList::erase, "C++: wxAnyList::erase(const class wxAnyList::iterator &) --> class wxAnyList::iterator", pybind11::arg("it"));
		cl.def("erase", (class wxAnyList::iterator (wxAnyList::*)(const class wxAnyList::iterator &, const class wxAnyList::iterator &)) &wxAnyList::erase, "C++: wxAnyList::erase(const class wxAnyList::iterator &, const class wxAnyList::iterator &) --> class wxAnyList::iterator", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("clear", (void (wxAnyList::*)()) &wxAnyList::clear, "C++: wxAnyList::clear() --> void");
		cl.def("splice", (void (wxAnyList::*)(const class wxAnyList::iterator &, class wxAnyList &, const class wxAnyList::iterator &, const class wxAnyList::iterator &)) &wxAnyList::splice, "C++: wxAnyList::splice(const class wxAnyList::iterator &, class wxAnyList &, const class wxAnyList::iterator &, const class wxAnyList::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("splice", (void (wxAnyList::*)(const class wxAnyList::iterator &, class wxAnyList &)) &wxAnyList::splice, "C++: wxAnyList::splice(const class wxAnyList::iterator &, class wxAnyList &) --> void", pybind11::arg("it"), pybind11::arg("l"));
		cl.def("splice", (void (wxAnyList::*)(const class wxAnyList::iterator &, class wxAnyList &, const class wxAnyList::iterator &)) &wxAnyList::splice, "C++: wxAnyList::splice(const class wxAnyList::iterator &, class wxAnyList &, const class wxAnyList::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"));
		cl.def("remove", (void (wxAnyList::*)(class wxAny *const &)) &wxAnyList::remove, "C++: wxAnyList::remove(class wxAny *const &) --> void", pybind11::arg("v"));
		cl.def("reverse", (void (wxAnyList::*)()) &wxAnyList::reverse, "C++: wxAnyList::reverse() --> void");

		{ // wxAnyList::compatibility_iterator file: line:705
			auto & enclosing_class = cl;
			pybind11::class_<wxAnyList::compatibility_iterator, std::shared_ptr<wxAnyList::compatibility_iterator>> cl(enclosing_class, "compatibility_iterator", "");
			cl.def( pybind11::init( [](){ return new wxAnyList::compatibility_iterator(); } ), "doc" );
			cl.def( pybind11::init<class wxwxAnyListNode *>(), pybind11::arg("ptr") );

		}

		{ // wxAnyList::iterator file: line:802
			auto & enclosing_class = cl;
			pybind11::class_<wxAnyList::iterator, std::shared_ptr<wxAnyList::iterator>> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init<class wxwxAnyListNode *, class wxwxAnyListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxAnyList::iterator(); } ) );
			cl.def( pybind11::init( [](wxAnyList::iterator const &o){ return new wxAnyList::iterator(o); } ) );
			cl.def("__mul__", (class wxAny *& (wxAnyList::iterator::*)() const) &wxAnyList::iterator::operator*, "C++: wxAnyList::iterator::operator*() const --> class wxAny *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxAnyList::iterator & (wxAnyList::iterator::*)()) &wxAnyList::iterator::operator++, "C++: wxAnyList::iterator::operator++() --> class wxAnyList::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxAnyList::iterator (wxAnyList::iterator::*)(int)) &wxAnyList::iterator::operator++, "C++: wxAnyList::iterator::operator++(int) --> const class wxAnyList::iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxAnyList::iterator & (wxAnyList::iterator::*)()) &wxAnyList::iterator::operator--, "C++: wxAnyList::iterator::operator--() --> class wxAnyList::iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxAnyList::iterator (wxAnyList::iterator::*)(int)) &wxAnyList::iterator::operator--, "C++: wxAnyList::iterator::operator--(int) --> const class wxAnyList::iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxAnyList::iterator::*)(const class wxAnyList::iterator &) const) &wxAnyList::iterator::operator!=, "C++: wxAnyList::iterator::operator!=(const class wxAnyList::iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxAnyList::iterator::*)(const class wxAnyList::iterator &) const) &wxAnyList::iterator::operator==, "C++: wxAnyList::iterator::operator==(const class wxAnyList::iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxAnyList::const_iterator file: line:852
			auto & enclosing_class = cl;
			pybind11::class_<wxAnyList::const_iterator, std::shared_ptr<wxAnyList::const_iterator>> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init<class wxwxAnyListNode *, class wxwxAnyListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxAnyList::const_iterator(); } ) );
			cl.def( pybind11::init<const class wxAnyList::iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxAnyList::const_iterator const &o){ return new wxAnyList::const_iterator(o); } ) );
			cl.def("__mul__", (class wxAny *const & (wxAnyList::const_iterator::*)() const) &wxAnyList::const_iterator::operator*, "C++: wxAnyList::const_iterator::operator*() const --> class wxAny *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxAnyList::const_iterator & (wxAnyList::const_iterator::*)()) &wxAnyList::const_iterator::operator++, "C++: wxAnyList::const_iterator::operator++() --> class wxAnyList::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxAnyList::const_iterator (wxAnyList::const_iterator::*)(int)) &wxAnyList::const_iterator::operator++, "C++: wxAnyList::const_iterator::operator++(int) --> const class wxAnyList::const_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxAnyList::const_iterator & (wxAnyList::const_iterator::*)()) &wxAnyList::const_iterator::operator--, "C++: wxAnyList::const_iterator::operator--() --> class wxAnyList::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxAnyList::const_iterator (wxAnyList::const_iterator::*)(int)) &wxAnyList::const_iterator::operator--, "C++: wxAnyList::const_iterator::operator--(int) --> const class wxAnyList::const_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxAnyList::const_iterator::*)(const class wxAnyList::const_iterator &) const) &wxAnyList::const_iterator::operator!=, "C++: wxAnyList::const_iterator::operator!=(const class wxAnyList::const_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxAnyList::const_iterator::*)(const class wxAnyList::const_iterator &) const) &wxAnyList::const_iterator::operator==, "C++: wxAnyList::const_iterator::operator==(const class wxAnyList::const_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxAnyList::reverse_iterator file: line:905
			auto & enclosing_class = cl;
			pybind11::class_<wxAnyList::reverse_iterator, std::shared_ptr<wxAnyList::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init<class wxwxAnyListNode *, class wxwxAnyListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxAnyList::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxAnyList::reverse_iterator const &o){ return new wxAnyList::reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxAny *& (wxAnyList::reverse_iterator::*)() const) &wxAnyList::reverse_iterator::operator*, "C++: wxAnyList::reverse_iterator::operator*() const --> class wxAny *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxAnyList::reverse_iterator & (wxAnyList::reverse_iterator::*)()) &wxAnyList::reverse_iterator::operator++, "C++: wxAnyList::reverse_iterator::operator++() --> class wxAnyList::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxAnyList::reverse_iterator (wxAnyList::reverse_iterator::*)(int)) &wxAnyList::reverse_iterator::operator++, "C++: wxAnyList::reverse_iterator::operator++(int) --> const class wxAnyList::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxAnyList::reverse_iterator & (wxAnyList::reverse_iterator::*)()) &wxAnyList::reverse_iterator::operator--, "C++: wxAnyList::reverse_iterator::operator--() --> class wxAnyList::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxAnyList::reverse_iterator (wxAnyList::reverse_iterator::*)(int)) &wxAnyList::reverse_iterator::operator--, "C++: wxAnyList::reverse_iterator::operator--(int) --> const class wxAnyList::reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxAnyList::reverse_iterator::*)(const class wxAnyList::reverse_iterator &) const) &wxAnyList::reverse_iterator::operator!=, "C++: wxAnyList::reverse_iterator::operator!=(const class wxAnyList::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxAnyList::reverse_iterator::*)(const class wxAnyList::reverse_iterator &) const) &wxAnyList::reverse_iterator::operator==, "C++: wxAnyList::reverse_iterator::operator==(const class wxAnyList::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxAnyList::const_reverse_iterator file: line:944
			auto & enclosing_class = cl;
			pybind11::class_<wxAnyList::const_reverse_iterator, std::shared_ptr<wxAnyList::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init<class wxwxAnyListNode *, class wxwxAnyListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxAnyList::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const class wxAnyList::reverse_iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxAnyList::const_reverse_iterator const &o){ return new wxAnyList::const_reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxAny *const & (wxAnyList::const_reverse_iterator::*)() const) &wxAnyList::const_reverse_iterator::operator*, "C++: wxAnyList::const_reverse_iterator::operator*() const --> class wxAny *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxAnyList::const_reverse_iterator & (wxAnyList::const_reverse_iterator::*)()) &wxAnyList::const_reverse_iterator::operator++, "C++: wxAnyList::const_reverse_iterator::operator++() --> class wxAnyList::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxAnyList::const_reverse_iterator (wxAnyList::const_reverse_iterator::*)(int)) &wxAnyList::const_reverse_iterator::operator++, "C++: wxAnyList::const_reverse_iterator::operator++(int) --> const class wxAnyList::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxAnyList::const_reverse_iterator & (wxAnyList::const_reverse_iterator::*)()) &wxAnyList::const_reverse_iterator::operator--, "C++: wxAnyList::const_reverse_iterator::operator--() --> class wxAnyList::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxAnyList::const_reverse_iterator (wxAnyList::const_reverse_iterator::*)(int)) &wxAnyList::const_reverse_iterator::operator--, "C++: wxAnyList::const_reverse_iterator::operator--(int) --> const class wxAnyList::const_reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxAnyList::const_reverse_iterator::*)(const class wxAnyList::const_reverse_iterator &) const) &wxAnyList::const_reverse_iterator::operator!=, "C++: wxAnyList::const_reverse_iterator::operator!=(const class wxAnyList::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxAnyList::const_reverse_iterator::*)(const class wxAnyList::const_reverse_iterator &) const) &wxAnyList::const_reverse_iterator::operator==, "C++: wxAnyList::const_reverse_iterator::operator==(const class wxAnyList::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}
