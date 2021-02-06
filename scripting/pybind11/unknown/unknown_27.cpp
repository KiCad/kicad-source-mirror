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

// wxNodeBase file: line:419
struct PyCallBack_wxNodeBase : public wxNodeBase {
	using wxNodeBase::wxNodeBase;

	void DeleteData() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNodeBase *>(this), "DeleteData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxNodeBase::DeleteData();
	}
};

// wxListBase file: line:481
struct PyCallBack_wxListBase : public wxListBase {
	using wxListBase::wxListBase;

	class wxNodeBase * CreateNode(class wxNodeBase * a0, class wxNodeBase * a1, void * a2, const class wxListKey & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxListBase *>(this), "CreateNode");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxNodeBase *>::value) {
				static pybind11::detail::override_caster_t<class wxNodeBase *> caster;
				return pybind11::detail::cast_ref<class wxNodeBase *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxNodeBase *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxListBase::CreateNode\"");
	}
};

// wxObjectListNode file: line:1195
struct PyCallBack_wxObjectListNode : public wxObjectListNode {
	using wxObjectListNode::wxObjectListNode;

	void DeleteData() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxObjectListNode *>(this), "DeleteData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxObjectListNode::DeleteData();
	}
};

// wxObjectList file: line:1195
struct PyCallBack_wxObjectList : public wxObjectList {
	using wxObjectList::wxObjectList;

	class wxObjectListNode * Find(const class wxListKey & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxObjectList *>(this), "Find");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxObjectListNode *>::value) {
				static pybind11::detail::override_caster_t<class wxObjectListNode *> caster;
				return pybind11::detail::cast_ref<class wxObjectListNode *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxObjectListNode *>(std::move(o));
		}
		return wxObjectList::Find(a0);
	}
	class wxNodeBase * CreateNode(class wxNodeBase * a0, class wxNodeBase * a1, void * a2, const class wxListKey & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxObjectList *>(this), "CreateNode");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxNodeBase *>::value) {
				static pybind11::detail::override_caster_t<class wxNodeBase *> caster;
				return pybind11::detail::cast_ref<class wxNodeBase *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxNodeBase *>(std::move(o));
		}
		return wxObjectList::CreateNode(a0, a1, a2, a3);
	}
};

void bind_unknown_unknown_27(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxListKeyValue file: line:360
		pybind11::class_<wxListKeyValue, std::shared_ptr<wxListKeyValue>> cl(M(""), "wxListKeyValue", "");
		cl.def( pybind11::init( [](){ return new wxListKeyValue(); } ) );
		cl.def( pybind11::init( [](wxListKeyValue const &o){ return new wxListKeyValue(o); } ) );
		cl.def_readwrite("integer", &wxListKeyValue::integer);
	}
	{ // wxListKey file: line:372
		pybind11::class_<wxListKey, std::shared_ptr<wxListKey>> cl(M(""), "wxListKey", "");
		cl.def( pybind11::init( [](){ return new wxListKey(); } ) );
		cl.def( pybind11::init<long>(), pybind11::arg("i") );

		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("s") );

		cl.def( pybind11::init<const char *>(), pybind11::arg("s") );

		cl.def( pybind11::init<const wchar_t *>(), pybind11::arg("s") );

		cl.def( pybind11::init( [](wxListKey const &o){ return new wxListKey(o); } ) );
		cl.def("GetKeyType", (enum wxKeyType (wxListKey::*)() const) &wxListKey::GetKeyType, "C++: wxListKey::GetKeyType() const --> enum wxKeyType");
		cl.def("GetString", (const class wxString (wxListKey::*)() const) &wxListKey::GetString, "C++: wxListKey::GetString() const --> const class wxString");
		cl.def("GetNumber", (long (wxListKey::*)() const) &wxListKey::GetNumber, "C++: wxListKey::GetNumber() const --> long");
		cl.def("__eq__", (bool (wxListKey::*)(union wxListKeyValue) const) &wxListKey::operator==, "C++: wxListKey::operator==(union wxListKeyValue) const --> bool", pybind11::arg("value"));
	}
	{ // wxNodeBase file: line:419
		pybind11::class_<wxNodeBase, std::shared_ptr<wxNodeBase>, PyCallBack_wxNodeBase> cl(M(""), "wxNodeBase", "");
		cl.def( pybind11::init( [](){ return new wxNodeBase(); }, [](){ return new PyCallBack_wxNodeBase(); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0){ return new wxNodeBase(a0); }, [](class wxListBase * a0){ return new PyCallBack_wxNodeBase(a0); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxNodeBase * a1){ return new wxNodeBase(a0, a1); }, [](class wxListBase * a0, class wxNodeBase * a1){ return new PyCallBack_wxNodeBase(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxNodeBase * a1, class wxNodeBase * a2){ return new wxNodeBase(a0, a1, a2); }, [](class wxListBase * a0, class wxNodeBase * a1, class wxNodeBase * a2){ return new PyCallBack_wxNodeBase(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxNodeBase * a1, class wxNodeBase * a2, void * a3){ return new wxNodeBase(a0, a1, a2, a3); }, [](class wxListBase * a0, class wxNodeBase * a1, class wxNodeBase * a2, void * a3){ return new PyCallBack_wxNodeBase(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<class wxListBase *, class wxNodeBase *, class wxNodeBase *, void *, const class wxListKey &>(), pybind11::arg("list"), pybind11::arg("previous"), pybind11::arg("next"), pybind11::arg("data"), pybind11::arg("key") );

		cl.def("GetKeyString", (class wxString (wxNodeBase::*)() const) &wxNodeBase::GetKeyString, "C++: wxNodeBase::GetKeyString() const --> class wxString");
		cl.def("GetKeyInteger", (long (wxNodeBase::*)() const) &wxNodeBase::GetKeyInteger, "C++: wxNodeBase::GetKeyInteger() const --> long");
		cl.def("SetKeyString", (void (wxNodeBase::*)(const class wxString &)) &wxNodeBase::SetKeyString, "C++: wxNodeBase::SetKeyString(const class wxString &) --> void", pybind11::arg("s"));
		cl.def("SetKeyInteger", (void (wxNodeBase::*)(long)) &wxNodeBase::SetKeyInteger, "C++: wxNodeBase::SetKeyInteger(long) --> void", pybind11::arg("i"));
		cl.def("Next", (class wxObjectListNode * (wxNodeBase::*)() const) &wxNodeBase::Next, "C++: wxNodeBase::Next() const --> class wxObjectListNode *", pybind11::return_value_policy::automatic);
		cl.def("Previous", (class wxObjectListNode * (wxNodeBase::*)() const) &wxNodeBase::Previous, "C++: wxNodeBase::Previous() const --> class wxObjectListNode *", pybind11::return_value_policy::automatic);
		cl.def("Data", (class wxObject * (wxNodeBase::*)() const) &wxNodeBase::Data, "C++: wxNodeBase::Data() const --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxListBase file: line:481
		pybind11::class_<wxListBase, std::shared_ptr<wxListBase>, PyCallBack_wxListBase> cl(M(""), "wxListBase", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxListBase(); } ), "doc");
		cl.def( pybind11::init<enum wxKeyType>(), pybind11::arg("keyType") );

		cl.def(pybind11::init<PyCallBack_wxListBase const &>());
		cl.def("GetCount", (unsigned long (wxListBase::*)() const) &wxListBase::GetCount, "C++: wxListBase::GetCount() const --> unsigned long");
		cl.def("IsEmpty", (bool (wxListBase::*)() const) &wxListBase::IsEmpty, "C++: wxListBase::IsEmpty() const --> bool");
		cl.def("Clear", (void (wxListBase::*)()) &wxListBase::Clear, "C++: wxListBase::Clear() --> void");
		cl.def("DeleteContents", (void (wxListBase::*)(bool)) &wxListBase::DeleteContents, "C++: wxListBase::DeleteContents(bool) --> void", pybind11::arg("destroy"));
		cl.def("GetDeleteContents", (bool (wxListBase::*)() const) &wxListBase::GetDeleteContents, "C++: wxListBase::GetDeleteContents() const --> bool");
		cl.def("GetKeyType", (enum wxKeyType (wxListBase::*)() const) &wxListBase::GetKeyType, "C++: wxListBase::GetKeyType() const --> enum wxKeyType");
		cl.def("SetKeyType", (void (wxListBase::*)(enum wxKeyType)) &wxListBase::SetKeyType, "C++: wxListBase::SetKeyType(enum wxKeyType) --> void", pybind11::arg("keyType"));
		cl.def("Number", (int (wxListBase::*)() const) &wxListBase::Number, "C++: wxListBase::Number() const --> int");
		cl.def("First", (class wxObjectListNode * (wxListBase::*)() const) &wxListBase::First, "C++: wxListBase::First() const --> class wxObjectListNode *", pybind11::return_value_policy::automatic);
		cl.def("Last", (class wxObjectListNode * (wxListBase::*)() const) &wxListBase::Last, "C++: wxListBase::Last() const --> class wxObjectListNode *", pybind11::return_value_policy::automatic);
		cl.def("Nth", (class wxObjectListNode * (wxListBase::*)(unsigned long) const) &wxListBase::Nth, "C++: wxListBase::Nth(unsigned long) const --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("assign", (class wxListBase & (wxListBase::*)(const class wxListBase &)) &wxListBase::operator=, "C++: wxListBase::operator=(const class wxListBase &) --> class wxListBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxObjectListNode file: line:1195
		pybind11::class_<wxObjectListNode, std::shared_ptr<wxObjectListNode>, PyCallBack_wxObjectListNode, wxNodeBase> cl(M(""), "wxObjectListNode", "");
		cl.def( pybind11::init( [](){ return new wxObjectListNode(); }, [](){ return new PyCallBack_wxObjectListNode(); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0){ return new wxObjectListNode(a0); }, [](class wxListBase * a0){ return new PyCallBack_wxObjectListNode(a0); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxObjectListNode * a1){ return new wxObjectListNode(a0, a1); }, [](class wxListBase * a0, class wxObjectListNode * a1){ return new PyCallBack_wxObjectListNode(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxObjectListNode * a1, class wxObjectListNode * a2){ return new wxObjectListNode(a0, a1, a2); }, [](class wxListBase * a0, class wxObjectListNode * a1, class wxObjectListNode * a2){ return new PyCallBack_wxObjectListNode(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxObjectListNode * a1, class wxObjectListNode * a2, class wxObject * a3){ return new wxObjectListNode(a0, a1, a2, a3); }, [](class wxListBase * a0, class wxObjectListNode * a1, class wxObjectListNode * a2, class wxObject * a3){ return new PyCallBack_wxObjectListNode(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<class wxListBase *, class wxObjectListNode *, class wxObjectListNode *, class wxObject *, const class wxListKey &>(), pybind11::arg("list"), pybind11::arg("previous"), pybind11::arg("next"), pybind11::arg("data"), pybind11::arg("key") );

		cl.def("GetNext", (class wxObjectListNode * (wxObjectListNode::*)() const) &wxObjectListNode::GetNext, "C++: wxObjectListNode::GetNext() const --> class wxObjectListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetPrevious", (class wxObjectListNode * (wxObjectListNode::*)() const) &wxObjectListNode::GetPrevious, "C++: wxObjectListNode::GetPrevious() const --> class wxObjectListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetData", (class wxObject * (wxObjectListNode::*)() const) &wxObjectListNode::GetData, "C++: wxObjectListNode::GetData() const --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("SetData", (void (wxObjectListNode::*)(class wxObject *)) &wxObjectListNode::SetData, "C++: wxObjectListNode::SetData(class wxObject *) --> void", pybind11::arg("data"));
	}
	{ // wxObjectList file: line:1195
		pybind11::class_<wxObjectList, std::shared_ptr<wxObjectList>, PyCallBack_wxObjectList, wxListBase> cl(M(""), "wxObjectList", "");
		cl.def( pybind11::init( [](){ return new wxObjectList(); }, [](){ return new PyCallBack_wxObjectList(); } ), "doc");
		cl.def( pybind11::init<enum wxKeyType>(), pybind11::arg("keyType") );

		cl.def( pybind11::init( [](PyCallBack_wxObjectList const &o){ return new PyCallBack_wxObjectList(o); } ) );
		cl.def( pybind11::init( [](wxObjectList const &o){ return new wxObjectList(o); } ) );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxObjectList(a0); }, [](unsigned long const & a0){ return new PyCallBack_wxObjectList(a0); } ), "doc");
		cl.def( pybind11::init<unsigned long, class wxObject *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const class wxObjectList::const_iterator &, const class wxObjectList::const_iterator &>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def("assign", (class wxObjectList & (wxObjectList::*)(const class wxObjectList &)) &wxObjectList::operator=, "C++: wxObjectList::operator=(const class wxObjectList &) --> class wxObjectList &", pybind11::return_value_policy::automatic, pybind11::arg("list"));
		cl.def("GetFirst", (class wxObjectListNode * (wxObjectList::*)() const) &wxObjectList::GetFirst, "C++: wxObjectList::GetFirst() const --> class wxObjectListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetLast", (class wxObjectListNode * (wxObjectList::*)() const) &wxObjectList::GetLast, "C++: wxObjectList::GetLast() const --> class wxObjectListNode *", pybind11::return_value_policy::automatic);
		cl.def("Item", (class wxObjectListNode * (wxObjectList::*)(unsigned long) const) &wxObjectList::Item, "C++: wxObjectList::Item(unsigned long) const --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("__getitem__", (class wxObject * (wxObjectList::*)(unsigned long) const) &wxObjectList::operator[], "C++: wxObjectList::operator[](unsigned long) const --> class wxObject *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("Append", (class wxObjectListNode * (wxObjectList::*)(class wxObject *)) &wxObjectList::Append, "C++: wxObjectList::Append(class wxObject *) --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxObjectListNode * (wxObjectList::*)(class wxObject *)) &wxObjectList::Insert, "C++: wxObjectList::Insert(class wxObject *) --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxObjectListNode * (wxObjectList::*)(unsigned long, class wxObject *)) &wxObjectList::Insert, "C++: wxObjectList::Insert(unsigned long, class wxObject *) --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("pos"), pybind11::arg("object"));
		cl.def("Insert", (class wxObjectListNode * (wxObjectList::*)(class wxObjectListNode *, class wxObject *)) &wxObjectList::Insert, "C++: wxObjectList::Insert(class wxObjectListNode *, class wxObject *) --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("prev"), pybind11::arg("object"));
		cl.def("Append", (class wxObjectListNode * (wxObjectList::*)(long, void *)) &wxObjectList::Append, "C++: wxObjectList::Append(long, void *) --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("Append", (class wxObjectListNode * (wxObjectList::*)(const wchar_t *, void *)) &wxObjectList::Append, "C++: wxObjectList::Append(const wchar_t *, void *) --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("DetachNode", (class wxObjectListNode * (wxObjectList::*)(class wxObjectListNode *)) &wxObjectList::DetachNode, "C++: wxObjectList::DetachNode(class wxObjectListNode *) --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("node"));
		cl.def("DeleteNode", (bool (wxObjectList::*)(class wxObjectListNode *)) &wxObjectList::DeleteNode, "C++: wxObjectList::DeleteNode(class wxObjectListNode *) --> bool", pybind11::arg("node"));
		cl.def("DeleteObject", (bool (wxObjectList::*)(class wxObject *)) &wxObjectList::DeleteObject, "C++: wxObjectList::DeleteObject(class wxObject *) --> bool", pybind11::arg("object"));
		cl.def("Erase", (void (wxObjectList::*)(class wxObjectListNode *)) &wxObjectList::Erase, "C++: wxObjectList::Erase(class wxObjectListNode *) --> void", pybind11::arg("it"));
		cl.def("Find", (class wxObjectListNode * (wxObjectList::*)(const class wxObject *) const) &wxObjectList::Find, "C++: wxObjectList::Find(const class wxObject *) const --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Find", (class wxObjectListNode * (wxObjectList::*)(const class wxListKey &) const) &wxObjectList::Find, "C++: wxObjectList::Find(const class wxListKey &) const --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("Member", (bool (wxObjectList::*)(const class wxObject *) const) &wxObjectList::Member, "C++: wxObjectList::Member(const class wxObject *) const --> bool", pybind11::arg("object"));
		cl.def("IndexOf", (int (wxObjectList::*)(class wxObject *) const) &wxObjectList::IndexOf, "C++: wxObjectList::IndexOf(class wxObject *) const --> int", pybind11::arg("object"));
		cl.def("begin", (class wxObjectList::iterator (wxObjectList::*)()) &wxObjectList::begin, "C++: wxObjectList::begin() --> class wxObjectList::iterator");
		cl.def("end", (class wxObjectList::iterator (wxObjectList::*)()) &wxObjectList::end, "C++: wxObjectList::end() --> class wxObjectList::iterator");
		cl.def("rbegin", (class wxObjectList::reverse_iterator (wxObjectList::*)()) &wxObjectList::rbegin, "C++: wxObjectList::rbegin() --> class wxObjectList::reverse_iterator");
		cl.def("rend", (class wxObjectList::reverse_iterator (wxObjectList::*)()) &wxObjectList::rend, "C++: wxObjectList::rend() --> class wxObjectList::reverse_iterator");
		cl.def("resize", [](wxObjectList &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxObjectList::*)(unsigned long, class wxObject *)) &wxObjectList::resize, "C++: wxObjectList::resize(unsigned long, class wxObject *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("size", (unsigned long (wxObjectList::*)() const) &wxObjectList::size, "C++: wxObjectList::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxObjectList::*)() const) &wxObjectList::max_size, "C++: wxObjectList::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxObjectList::*)() const) &wxObjectList::empty, "C++: wxObjectList::empty() const --> bool");
		cl.def("front", (class wxObject *& (wxObjectList::*)()) &wxObjectList::front, "C++: wxObjectList::front() --> class wxObject *&", pybind11::return_value_policy::automatic);
		cl.def("back", (class wxObject *& (wxObjectList::*)()) &wxObjectList::back, "C++: wxObjectList::back() --> class wxObject *&", pybind11::return_value_policy::automatic);
		cl.def("push_front", [](wxObjectList &o) -> void { return o.push_front(); }, "");
		cl.def("push_front", (void (wxObjectList::*)(class wxObject *const &)) &wxObjectList::push_front, "C++: wxObjectList::push_front(class wxObject *const &) --> void", pybind11::arg("v"));
		cl.def("pop_front", (void (wxObjectList::*)()) &wxObjectList::pop_front, "C++: wxObjectList::pop_front() --> void");
		cl.def("push_back", [](wxObjectList &o) -> void { return o.push_back(); }, "");
		cl.def("push_back", (void (wxObjectList::*)(class wxObject *const &)) &wxObjectList::push_back, "C++: wxObjectList::push_back(class wxObject *const &) --> void", pybind11::arg("v"));
		cl.def("pop_back", (void (wxObjectList::*)()) &wxObjectList::pop_back, "C++: wxObjectList::pop_back() --> void");
		cl.def("assign", (void (wxObjectList::*)(class wxObjectList::const_iterator, const class wxObjectList::const_iterator &)) &wxObjectList::assign, "C++: wxObjectList::assign(class wxObjectList::const_iterator, const class wxObjectList::const_iterator &) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", [](wxObjectList &o, unsigned long const & a0) -> void { return o.assign(a0); }, "", pybind11::arg("n"));
		cl.def("assign", (void (wxObjectList::*)(unsigned long, class wxObject *const &)) &wxObjectList::assign, "C++: wxObjectList::assign(unsigned long, class wxObject *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (class wxObjectList::iterator (wxObjectList::*)(const class wxObjectList::iterator &, class wxObject *const &)) &wxObjectList::insert, "C++: wxObjectList::insert(const class wxObjectList::iterator &, class wxObject *const &) --> class wxObjectList::iterator", pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxObjectList::*)(const class wxObjectList::iterator &, unsigned long, class wxObject *const &)) &wxObjectList::insert, "C++: wxObjectList::insert(const class wxObjectList::iterator &, unsigned long, class wxObject *const &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (void (wxObjectList::*)(const class wxObjectList::iterator &, class wxObjectList::const_iterator, const class wxObjectList::const_iterator &)) &wxObjectList::insert, "C++: wxObjectList::insert(const class wxObjectList::iterator &, class wxObjectList::const_iterator, const class wxObjectList::const_iterator &) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (class wxObjectList::iterator (wxObjectList::*)(const class wxObjectList::iterator &)) &wxObjectList::erase, "C++: wxObjectList::erase(const class wxObjectList::iterator &) --> class wxObjectList::iterator", pybind11::arg("it"));
		cl.def("erase", (class wxObjectList::iterator (wxObjectList::*)(const class wxObjectList::iterator &, const class wxObjectList::iterator &)) &wxObjectList::erase, "C++: wxObjectList::erase(const class wxObjectList::iterator &, const class wxObjectList::iterator &) --> class wxObjectList::iterator", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("clear", (void (wxObjectList::*)()) &wxObjectList::clear, "C++: wxObjectList::clear() --> void");
		cl.def("splice", (void (wxObjectList::*)(const class wxObjectList::iterator &, class wxObjectList &, const class wxObjectList::iterator &, const class wxObjectList::iterator &)) &wxObjectList::splice, "C++: wxObjectList::splice(const class wxObjectList::iterator &, class wxObjectList &, const class wxObjectList::iterator &, const class wxObjectList::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("splice", (void (wxObjectList::*)(const class wxObjectList::iterator &, class wxObjectList &)) &wxObjectList::splice, "C++: wxObjectList::splice(const class wxObjectList::iterator &, class wxObjectList &) --> void", pybind11::arg("it"), pybind11::arg("l"));
		cl.def("splice", (void (wxObjectList::*)(const class wxObjectList::iterator &, class wxObjectList &, const class wxObjectList::iterator &)) &wxObjectList::splice, "C++: wxObjectList::splice(const class wxObjectList::iterator &, class wxObjectList &, const class wxObjectList::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"));
		cl.def("remove", (void (wxObjectList::*)(class wxObject *const &)) &wxObjectList::remove, "C++: wxObjectList::remove(class wxObject *const &) --> void", pybind11::arg("v"));
		cl.def("reverse", (void (wxObjectList::*)()) &wxObjectList::reverse, "C++: wxObjectList::reverse() --> void");

		{ // wxObjectList::compatibility_iterator file: line:705
			auto & enclosing_class = cl;
			pybind11::class_<wxObjectList::compatibility_iterator, std::shared_ptr<wxObjectList::compatibility_iterator>> cl(enclosing_class, "compatibility_iterator", "");
			cl.def( pybind11::init( [](){ return new wxObjectList::compatibility_iterator(); } ), "doc" );
			cl.def( pybind11::init<class wxObjectListNode *>(), pybind11::arg("ptr") );

		}

		{ // wxObjectList::iterator file: line:802
			auto & enclosing_class = cl;
			pybind11::class_<wxObjectList::iterator, std::shared_ptr<wxObjectList::iterator>> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init<class wxObjectListNode *, class wxObjectListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxObjectList::iterator(); } ) );
			cl.def( pybind11::init( [](wxObjectList::iterator const &o){ return new wxObjectList::iterator(o); } ) );
			cl.def("__mul__", (class wxObject *& (wxObjectList::iterator::*)() const) &wxObjectList::iterator::operator*, "C++: wxObjectList::iterator::operator*() const --> class wxObject *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxObjectList::iterator & (wxObjectList::iterator::*)()) &wxObjectList::iterator::operator++, "C++: wxObjectList::iterator::operator++() --> class wxObjectList::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxObjectList::iterator (wxObjectList::iterator::*)(int)) &wxObjectList::iterator::operator++, "C++: wxObjectList::iterator::operator++(int) --> const class wxObjectList::iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxObjectList::iterator & (wxObjectList::iterator::*)()) &wxObjectList::iterator::operator--, "C++: wxObjectList::iterator::operator--() --> class wxObjectList::iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxObjectList::iterator (wxObjectList::iterator::*)(int)) &wxObjectList::iterator::operator--, "C++: wxObjectList::iterator::operator--(int) --> const class wxObjectList::iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxObjectList::iterator::*)(const class wxObjectList::iterator &) const) &wxObjectList::iterator::operator!=, "C++: wxObjectList::iterator::operator!=(const class wxObjectList::iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxObjectList::iterator::*)(const class wxObjectList::iterator &) const) &wxObjectList::iterator::operator==, "C++: wxObjectList::iterator::operator==(const class wxObjectList::iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxObjectList::const_iterator file: line:852
			auto & enclosing_class = cl;
			pybind11::class_<wxObjectList::const_iterator, std::shared_ptr<wxObjectList::const_iterator>> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init<class wxObjectListNode *, class wxObjectListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxObjectList::const_iterator(); } ) );
			cl.def( pybind11::init<const class wxObjectList::iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxObjectList::const_iterator const &o){ return new wxObjectList::const_iterator(o); } ) );
			cl.def("__mul__", (class wxObject *const & (wxObjectList::const_iterator::*)() const) &wxObjectList::const_iterator::operator*, "C++: wxObjectList::const_iterator::operator*() const --> class wxObject *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxObjectList::const_iterator & (wxObjectList::const_iterator::*)()) &wxObjectList::const_iterator::operator++, "C++: wxObjectList::const_iterator::operator++() --> class wxObjectList::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxObjectList::const_iterator (wxObjectList::const_iterator::*)(int)) &wxObjectList::const_iterator::operator++, "C++: wxObjectList::const_iterator::operator++(int) --> const class wxObjectList::const_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxObjectList::const_iterator & (wxObjectList::const_iterator::*)()) &wxObjectList::const_iterator::operator--, "C++: wxObjectList::const_iterator::operator--() --> class wxObjectList::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxObjectList::const_iterator (wxObjectList::const_iterator::*)(int)) &wxObjectList::const_iterator::operator--, "C++: wxObjectList::const_iterator::operator--(int) --> const class wxObjectList::const_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxObjectList::const_iterator::*)(const class wxObjectList::const_iterator &) const) &wxObjectList::const_iterator::operator!=, "C++: wxObjectList::const_iterator::operator!=(const class wxObjectList::const_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxObjectList::const_iterator::*)(const class wxObjectList::const_iterator &) const) &wxObjectList::const_iterator::operator==, "C++: wxObjectList::const_iterator::operator==(const class wxObjectList::const_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxObjectList::reverse_iterator file: line:905
			auto & enclosing_class = cl;
			pybind11::class_<wxObjectList::reverse_iterator, std::shared_ptr<wxObjectList::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init<class wxObjectListNode *, class wxObjectListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxObjectList::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxObjectList::reverse_iterator const &o){ return new wxObjectList::reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxObject *& (wxObjectList::reverse_iterator::*)() const) &wxObjectList::reverse_iterator::operator*, "C++: wxObjectList::reverse_iterator::operator*() const --> class wxObject *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxObjectList::reverse_iterator & (wxObjectList::reverse_iterator::*)()) &wxObjectList::reverse_iterator::operator++, "C++: wxObjectList::reverse_iterator::operator++() --> class wxObjectList::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxObjectList::reverse_iterator (wxObjectList::reverse_iterator::*)(int)) &wxObjectList::reverse_iterator::operator++, "C++: wxObjectList::reverse_iterator::operator++(int) --> const class wxObjectList::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxObjectList::reverse_iterator & (wxObjectList::reverse_iterator::*)()) &wxObjectList::reverse_iterator::operator--, "C++: wxObjectList::reverse_iterator::operator--() --> class wxObjectList::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxObjectList::reverse_iterator (wxObjectList::reverse_iterator::*)(int)) &wxObjectList::reverse_iterator::operator--, "C++: wxObjectList::reverse_iterator::operator--(int) --> const class wxObjectList::reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxObjectList::reverse_iterator::*)(const class wxObjectList::reverse_iterator &) const) &wxObjectList::reverse_iterator::operator!=, "C++: wxObjectList::reverse_iterator::operator!=(const class wxObjectList::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxObjectList::reverse_iterator::*)(const class wxObjectList::reverse_iterator &) const) &wxObjectList::reverse_iterator::operator==, "C++: wxObjectList::reverse_iterator::operator==(const class wxObjectList::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxObjectList::const_reverse_iterator file: line:944
			auto & enclosing_class = cl;
			pybind11::class_<wxObjectList::const_reverse_iterator, std::shared_ptr<wxObjectList::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init<class wxObjectListNode *, class wxObjectListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxObjectList::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const class wxObjectList::reverse_iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxObjectList::const_reverse_iterator const &o){ return new wxObjectList::const_reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxObject *const & (wxObjectList::const_reverse_iterator::*)() const) &wxObjectList::const_reverse_iterator::operator*, "C++: wxObjectList::const_reverse_iterator::operator*() const --> class wxObject *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxObjectList::const_reverse_iterator & (wxObjectList::const_reverse_iterator::*)()) &wxObjectList::const_reverse_iterator::operator++, "C++: wxObjectList::const_reverse_iterator::operator++() --> class wxObjectList::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxObjectList::const_reverse_iterator (wxObjectList::const_reverse_iterator::*)(int)) &wxObjectList::const_reverse_iterator::operator++, "C++: wxObjectList::const_reverse_iterator::operator++(int) --> const class wxObjectList::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxObjectList::const_reverse_iterator & (wxObjectList::const_reverse_iterator::*)()) &wxObjectList::const_reverse_iterator::operator--, "C++: wxObjectList::const_reverse_iterator::operator--() --> class wxObjectList::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxObjectList::const_reverse_iterator (wxObjectList::const_reverse_iterator::*)(int)) &wxObjectList::const_reverse_iterator::operator--, "C++: wxObjectList::const_reverse_iterator::operator--(int) --> const class wxObjectList::const_reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxObjectList::const_reverse_iterator::*)(const class wxObjectList::const_reverse_iterator &) const) &wxObjectList::const_reverse_iterator::operator!=, "C++: wxObjectList::const_reverse_iterator::operator!=(const class wxObjectList::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxObjectList::const_reverse_iterator::*)(const class wxObjectList::const_reverse_iterator &) const) &wxObjectList::const_reverse_iterator::operator==, "C++: wxObjectList::const_reverse_iterator::operator==(const class wxObjectList::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}
