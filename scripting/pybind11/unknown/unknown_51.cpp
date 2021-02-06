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

// wxVariantData file: line:56
struct PyCallBack_wxVariantData : public wxVariantData {
	using wxVariantData::wxVariantData;

	bool Eq(class wxVariantData & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariantData *>(this), "Eq");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxVariantData::Eq\"");
	}
	bool Write(class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariantData *>(this), "Write");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxVariantData::Write(a0);
	}
	bool Read(class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariantData *>(this), "Read");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxVariantData::Read(a0);
	}
	class wxString GetType() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariantData *>(this), "GetType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxVariantData::GetType\"");
	}
	class wxClassInfo * GetValueClassInfo() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariantData *>(this), "GetValueClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxVariantData::GetValueClassInfo();
	}
	class wxVariantData * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariantData *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxVariantData *>::value) {
				static pybind11::detail::override_caster_t<class wxVariantData *> caster;
				return pybind11::detail::cast_ref<class wxVariantData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxVariantData *>(std::move(o));
		}
		return wxVariantData::Clone();
	}
	bool GetAsAny(class wxAny * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariantData *>(this), "GetAsAny");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxVariantData::GetAsAny(a0);
	}
};

// wxwxVariantListNode file: line:23
struct PyCallBack_wxwxVariantListNode : public wxwxVariantListNode {
	using wxwxVariantListNode::wxwxVariantListNode;

	void DeleteData() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxwxVariantListNode *>(this), "DeleteData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxwxVariantListNode::DeleteData();
	}
};

// wxVariantList file: line:101
struct PyCallBack_wxVariantList : public wxVariantList {
	using wxVariantList::wxVariantList;

	class wxwxVariantListNode * Find(const class wxListKey & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariantList *>(this), "Find");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxwxVariantListNode *>::value) {
				static pybind11::detail::override_caster_t<class wxwxVariantListNode *> caster;
				return pybind11::detail::cast_ref<class wxwxVariantListNode *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxwxVariantListNode *>(std::move(o));
		}
		return wxVariantList::Find(a0);
	}
	class wxNodeBase * CreateNode(class wxNodeBase * a0, class wxNodeBase * a1, void * a2, const class wxListKey & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxVariantList *>(this), "CreateNode");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxNodeBase *>::value) {
				static pybind11::detail::override_caster_t<class wxNodeBase *> caster;
				return pybind11::detail::cast_ref<class wxNodeBase *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxNodeBase *>(std::move(o));
		}
		return wxVariantList::CreateNode(a0, a1, a2, a3);
	}
};

void bind_unknown_unknown_51(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxNextMonth(enum wxDateTime::Month &) file: line:2187
	M("").def("wxNextMonth", (void (*)(enum wxDateTime::Month &)) &wxNextMonth, "C++: wxNextMonth(enum wxDateTime::Month &) --> void", pybind11::arg("m"));

	// wxPrevMonth(enum wxDateTime::Month &) file: line:2188
	M("").def("wxPrevMonth", (void (*)(enum wxDateTime::Month &)) &wxPrevMonth, "C++: wxPrevMonth(enum wxDateTime::Month &) --> void", pybind11::arg("m"));

	// wxNextWDay(enum wxDateTime::WeekDay &) file: line:2189
	M("").def("wxNextWDay", (void (*)(enum wxDateTime::WeekDay &)) &wxNextWDay, "C++: wxNextWDay(enum wxDateTime::WeekDay &) --> void", pybind11::arg("wd"));

	// wxPrevWDay(enum wxDateTime::WeekDay &) file: line:2190
	M("").def("wxPrevWDay", (void (*)(enum wxDateTime::WeekDay &)) &wxPrevWDay, "C++: wxPrevWDay(enum wxDateTime::WeekDay &) --> void", pybind11::arg("wd"));

	{ // wxVariantData file: line:56
		pybind11::class_<wxVariantData, wxVariantData*, PyCallBack_wxVariantData, wxRefCounter> cl(M(""), "wxVariantData", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxVariantData(); } ) );
		cl.def("Eq", (bool (wxVariantData::*)(class wxVariantData &) const) &wxVariantData::Eq, "C++: wxVariantData::Eq(class wxVariantData &) const --> bool", pybind11::arg("data"));
		cl.def("Write", (bool (wxVariantData::*)(class wxString &) const) &wxVariantData::Write, "C++: wxVariantData::Write(class wxString &) const --> bool", pybind11::arg(""));
		cl.def("Read", (bool (wxVariantData::*)(class wxString &)) &wxVariantData::Read, "C++: wxVariantData::Read(class wxString &) --> bool", pybind11::arg(""));
		cl.def("GetType", (class wxString (wxVariantData::*)() const) &wxVariantData::GetType, "C++: wxVariantData::GetType() const --> class wxString");
		cl.def("GetValueClassInfo", (class wxClassInfo * (wxVariantData::*)()) &wxVariantData::GetValueClassInfo, "C++: wxVariantData::GetValueClassInfo() --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("Clone", (class wxVariantData * (wxVariantData::*)() const) &wxVariantData::Clone, "C++: wxVariantData::Clone() const --> class wxVariantData *", pybind11::return_value_policy::automatic);
		cl.def("GetAsAny", (bool (wxVariantData::*)(class wxAny *) const) &wxVariantData::GetAsAny, "C++: wxVariantData::GetAsAny(class wxAny *) const --> bool", pybind11::arg(""));
	}
	{ // wxwxVariantListNode file: line:23
		pybind11::class_<wxwxVariantListNode, std::shared_ptr<wxwxVariantListNode>, PyCallBack_wxwxVariantListNode, wxNodeBase> cl(M(""), "wxwxVariantListNode", "");
		cl.def( pybind11::init( [](){ return new wxwxVariantListNode(); }, [](){ return new PyCallBack_wxwxVariantListNode(); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0){ return new wxwxVariantListNode(a0); }, [](class wxListBase * a0){ return new PyCallBack_wxwxVariantListNode(a0); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxwxVariantListNode * a1){ return new wxwxVariantListNode(a0, a1); }, [](class wxListBase * a0, class wxwxVariantListNode * a1){ return new PyCallBack_wxwxVariantListNode(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxwxVariantListNode * a1, class wxwxVariantListNode * a2){ return new wxwxVariantListNode(a0, a1, a2); }, [](class wxListBase * a0, class wxwxVariantListNode * a1, class wxwxVariantListNode * a2){ return new PyCallBack_wxwxVariantListNode(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxwxVariantListNode * a1, class wxwxVariantListNode * a2, class wxVariant * a3){ return new wxwxVariantListNode(a0, a1, a2, a3); }, [](class wxListBase * a0, class wxwxVariantListNode * a1, class wxwxVariantListNode * a2, class wxVariant * a3){ return new PyCallBack_wxwxVariantListNode(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<class wxListBase *, class wxwxVariantListNode *, class wxwxVariantListNode *, class wxVariant *, const class wxListKey &>(), pybind11::arg("list"), pybind11::arg("previous"), pybind11::arg("next"), pybind11::arg("data"), pybind11::arg("key") );

		cl.def("GetNext", (class wxwxVariantListNode * (wxwxVariantListNode::*)() const) &wxwxVariantListNode::GetNext, "C++: wxwxVariantListNode::GetNext() const --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetPrevious", (class wxwxVariantListNode * (wxwxVariantListNode::*)() const) &wxwxVariantListNode::GetPrevious, "C++: wxwxVariantListNode::GetPrevious() const --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetData", (class wxVariant * (wxwxVariantListNode::*)() const) &wxwxVariantListNode::GetData, "C++: wxwxVariantListNode::GetData() const --> class wxVariant *", pybind11::return_value_policy::automatic);
		cl.def("SetData", (void (wxwxVariantListNode::*)(class wxVariant *)) &wxwxVariantListNode::SetData, "C++: wxwxVariantListNode::SetData(class wxVariant *) --> void", pybind11::arg("data"));
	}
	{ // wxVariantList file: line:101
		pybind11::class_<wxVariantList, std::shared_ptr<wxVariantList>, PyCallBack_wxVariantList, wxListBase> cl(M(""), "wxVariantList", "");
		cl.def( pybind11::init( [](){ return new wxVariantList(); }, [](){ return new PyCallBack_wxVariantList(); } ), "doc");
		cl.def( pybind11::init<enum wxKeyType>(), pybind11::arg("keyType") );

		cl.def( pybind11::init( [](PyCallBack_wxVariantList const &o){ return new PyCallBack_wxVariantList(o); } ) );
		cl.def( pybind11::init( [](wxVariantList const &o){ return new wxVariantList(o); } ) );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxVariantList(a0); }, [](unsigned long const & a0){ return new PyCallBack_wxVariantList(a0); } ), "doc");
		cl.def( pybind11::init<unsigned long, class wxVariant *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const class wxVariantList::const_iterator &, const class wxVariantList::const_iterator &>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def("assign", (class wxVariantList & (wxVariantList::*)(const class wxVariantList &)) &wxVariantList::operator=, "C++: wxVariantList::operator=(const class wxVariantList &) --> class wxVariantList &", pybind11::return_value_policy::automatic, pybind11::arg("list"));
		cl.def("GetFirst", (class wxwxVariantListNode * (wxVariantList::*)() const) &wxVariantList::GetFirst, "C++: wxVariantList::GetFirst() const --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetLast", (class wxwxVariantListNode * (wxVariantList::*)() const) &wxVariantList::GetLast, "C++: wxVariantList::GetLast() const --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic);
		cl.def("Item", (class wxwxVariantListNode * (wxVariantList::*)(unsigned long) const) &wxVariantList::Item, "C++: wxVariantList::Item(unsigned long) const --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("__getitem__", (class wxVariant * (wxVariantList::*)(unsigned long) const) &wxVariantList::operator[], "C++: wxVariantList::operator[](unsigned long) const --> class wxVariant *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("Append", (class wxwxVariantListNode * (wxVariantList::*)(class wxVariant *)) &wxVariantList::Append, "C++: wxVariantList::Append(class wxVariant *) --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxwxVariantListNode * (wxVariantList::*)(class wxVariant *)) &wxVariantList::Insert, "C++: wxVariantList::Insert(class wxVariant *) --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxwxVariantListNode * (wxVariantList::*)(unsigned long, class wxVariant *)) &wxVariantList::Insert, "C++: wxVariantList::Insert(unsigned long, class wxVariant *) --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic, pybind11::arg("pos"), pybind11::arg("object"));
		cl.def("Insert", (class wxwxVariantListNode * (wxVariantList::*)(class wxwxVariantListNode *, class wxVariant *)) &wxVariantList::Insert, "C++: wxVariantList::Insert(class wxwxVariantListNode *, class wxVariant *) --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic, pybind11::arg("prev"), pybind11::arg("object"));
		cl.def("Append", (class wxwxVariantListNode * (wxVariantList::*)(long, void *)) &wxVariantList::Append, "C++: wxVariantList::Append(long, void *) --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("Append", (class wxwxVariantListNode * (wxVariantList::*)(const wchar_t *, void *)) &wxVariantList::Append, "C++: wxVariantList::Append(const wchar_t *, void *) --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("DetachNode", (class wxwxVariantListNode * (wxVariantList::*)(class wxwxVariantListNode *)) &wxVariantList::DetachNode, "C++: wxVariantList::DetachNode(class wxwxVariantListNode *) --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic, pybind11::arg("node"));
		cl.def("DeleteNode", (bool (wxVariantList::*)(class wxwxVariantListNode *)) &wxVariantList::DeleteNode, "C++: wxVariantList::DeleteNode(class wxwxVariantListNode *) --> bool", pybind11::arg("node"));
		cl.def("DeleteObject", (bool (wxVariantList::*)(class wxVariant *)) &wxVariantList::DeleteObject, "C++: wxVariantList::DeleteObject(class wxVariant *) --> bool", pybind11::arg("object"));
		cl.def("Erase", (void (wxVariantList::*)(class wxwxVariantListNode *)) &wxVariantList::Erase, "C++: wxVariantList::Erase(class wxwxVariantListNode *) --> void", pybind11::arg("it"));
		cl.def("Find", (class wxwxVariantListNode * (wxVariantList::*)(const class wxVariant *) const) &wxVariantList::Find, "C++: wxVariantList::Find(const class wxVariant *) const --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Find", (class wxwxVariantListNode * (wxVariantList::*)(const class wxListKey &) const) &wxVariantList::Find, "C++: wxVariantList::Find(const class wxListKey &) const --> class wxwxVariantListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("Member", (bool (wxVariantList::*)(const class wxVariant *) const) &wxVariantList::Member, "C++: wxVariantList::Member(const class wxVariant *) const --> bool", pybind11::arg("object"));
		cl.def("IndexOf", (int (wxVariantList::*)(class wxVariant *) const) &wxVariantList::IndexOf, "C++: wxVariantList::IndexOf(class wxVariant *) const --> int", pybind11::arg("object"));
		cl.def("begin", (class wxVariantList::iterator (wxVariantList::*)()) &wxVariantList::begin, "C++: wxVariantList::begin() --> class wxVariantList::iterator");
		cl.def("end", (class wxVariantList::iterator (wxVariantList::*)()) &wxVariantList::end, "C++: wxVariantList::end() --> class wxVariantList::iterator");
		cl.def("rbegin", (class wxVariantList::reverse_iterator (wxVariantList::*)()) &wxVariantList::rbegin, "C++: wxVariantList::rbegin() --> class wxVariantList::reverse_iterator");
		cl.def("rend", (class wxVariantList::reverse_iterator (wxVariantList::*)()) &wxVariantList::rend, "C++: wxVariantList::rend() --> class wxVariantList::reverse_iterator");
		cl.def("resize", [](wxVariantList &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxVariantList::*)(unsigned long, class wxVariant *)) &wxVariantList::resize, "C++: wxVariantList::resize(unsigned long, class wxVariant *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("size", (unsigned long (wxVariantList::*)() const) &wxVariantList::size, "C++: wxVariantList::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxVariantList::*)() const) &wxVariantList::max_size, "C++: wxVariantList::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxVariantList::*)() const) &wxVariantList::empty, "C++: wxVariantList::empty() const --> bool");
		cl.def("front", (class wxVariant *& (wxVariantList::*)()) &wxVariantList::front, "C++: wxVariantList::front() --> class wxVariant *&", pybind11::return_value_policy::automatic);
		cl.def("back", (class wxVariant *& (wxVariantList::*)()) &wxVariantList::back, "C++: wxVariantList::back() --> class wxVariant *&", pybind11::return_value_policy::automatic);
		cl.def("push_front", [](wxVariantList &o) -> void { return o.push_front(); }, "");
		cl.def("push_front", (void (wxVariantList::*)(class wxVariant *const &)) &wxVariantList::push_front, "C++: wxVariantList::push_front(class wxVariant *const &) --> void", pybind11::arg("v"));
		cl.def("pop_front", (void (wxVariantList::*)()) &wxVariantList::pop_front, "C++: wxVariantList::pop_front() --> void");
		cl.def("push_back", [](wxVariantList &o) -> void { return o.push_back(); }, "");
		cl.def("push_back", (void (wxVariantList::*)(class wxVariant *const &)) &wxVariantList::push_back, "C++: wxVariantList::push_back(class wxVariant *const &) --> void", pybind11::arg("v"));
		cl.def("pop_back", (void (wxVariantList::*)()) &wxVariantList::pop_back, "C++: wxVariantList::pop_back() --> void");
		cl.def("assign", (void (wxVariantList::*)(class wxVariantList::const_iterator, const class wxVariantList::const_iterator &)) &wxVariantList::assign, "C++: wxVariantList::assign(class wxVariantList::const_iterator, const class wxVariantList::const_iterator &) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", [](wxVariantList &o, unsigned long const & a0) -> void { return o.assign(a0); }, "", pybind11::arg("n"));
		cl.def("assign", (void (wxVariantList::*)(unsigned long, class wxVariant *const &)) &wxVariantList::assign, "C++: wxVariantList::assign(unsigned long, class wxVariant *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (class wxVariantList::iterator (wxVariantList::*)(const class wxVariantList::iterator &, class wxVariant *const &)) &wxVariantList::insert, "C++: wxVariantList::insert(const class wxVariantList::iterator &, class wxVariant *const &) --> class wxVariantList::iterator", pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxVariantList::*)(const class wxVariantList::iterator &, unsigned long, class wxVariant *const &)) &wxVariantList::insert, "C++: wxVariantList::insert(const class wxVariantList::iterator &, unsigned long, class wxVariant *const &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (void (wxVariantList::*)(const class wxVariantList::iterator &, class wxVariantList::const_iterator, const class wxVariantList::const_iterator &)) &wxVariantList::insert, "C++: wxVariantList::insert(const class wxVariantList::iterator &, class wxVariantList::const_iterator, const class wxVariantList::const_iterator &) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (class wxVariantList::iterator (wxVariantList::*)(const class wxVariantList::iterator &)) &wxVariantList::erase, "C++: wxVariantList::erase(const class wxVariantList::iterator &) --> class wxVariantList::iterator", pybind11::arg("it"));
		cl.def("erase", (class wxVariantList::iterator (wxVariantList::*)(const class wxVariantList::iterator &, const class wxVariantList::iterator &)) &wxVariantList::erase, "C++: wxVariantList::erase(const class wxVariantList::iterator &, const class wxVariantList::iterator &) --> class wxVariantList::iterator", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("clear", (void (wxVariantList::*)()) &wxVariantList::clear, "C++: wxVariantList::clear() --> void");
		cl.def("splice", (void (wxVariantList::*)(const class wxVariantList::iterator &, class wxVariantList &, const class wxVariantList::iterator &, const class wxVariantList::iterator &)) &wxVariantList::splice, "C++: wxVariantList::splice(const class wxVariantList::iterator &, class wxVariantList &, const class wxVariantList::iterator &, const class wxVariantList::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("splice", (void (wxVariantList::*)(const class wxVariantList::iterator &, class wxVariantList &)) &wxVariantList::splice, "C++: wxVariantList::splice(const class wxVariantList::iterator &, class wxVariantList &) --> void", pybind11::arg("it"), pybind11::arg("l"));
		cl.def("splice", (void (wxVariantList::*)(const class wxVariantList::iterator &, class wxVariantList &, const class wxVariantList::iterator &)) &wxVariantList::splice, "C++: wxVariantList::splice(const class wxVariantList::iterator &, class wxVariantList &, const class wxVariantList::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"));
		cl.def("remove", (void (wxVariantList::*)(class wxVariant *const &)) &wxVariantList::remove, "C++: wxVariantList::remove(class wxVariant *const &) --> void", pybind11::arg("v"));
		cl.def("reverse", (void (wxVariantList::*)()) &wxVariantList::reverse, "C++: wxVariantList::reverse() --> void");

		{ // wxVariantList::compatibility_iterator file: line:705
			auto & enclosing_class = cl;
			pybind11::class_<wxVariantList::compatibility_iterator, std::shared_ptr<wxVariantList::compatibility_iterator>> cl(enclosing_class, "compatibility_iterator", "");
			cl.def( pybind11::init( [](){ return new wxVariantList::compatibility_iterator(); } ), "doc" );
			cl.def( pybind11::init<class wxwxVariantListNode *>(), pybind11::arg("ptr") );

		}

		{ // wxVariantList::iterator file: line:802
			auto & enclosing_class = cl;
			pybind11::class_<wxVariantList::iterator, std::shared_ptr<wxVariantList::iterator>> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init<class wxwxVariantListNode *, class wxwxVariantListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxVariantList::iterator(); } ) );
			cl.def( pybind11::init( [](wxVariantList::iterator const &o){ return new wxVariantList::iterator(o); } ) );
			cl.def("__mul__", (class wxVariant *& (wxVariantList::iterator::*)() const) &wxVariantList::iterator::operator*, "C++: wxVariantList::iterator::operator*() const --> class wxVariant *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxVariantList::iterator & (wxVariantList::iterator::*)()) &wxVariantList::iterator::operator++, "C++: wxVariantList::iterator::operator++() --> class wxVariantList::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxVariantList::iterator (wxVariantList::iterator::*)(int)) &wxVariantList::iterator::operator++, "C++: wxVariantList::iterator::operator++(int) --> const class wxVariantList::iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxVariantList::iterator & (wxVariantList::iterator::*)()) &wxVariantList::iterator::operator--, "C++: wxVariantList::iterator::operator--() --> class wxVariantList::iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxVariantList::iterator (wxVariantList::iterator::*)(int)) &wxVariantList::iterator::operator--, "C++: wxVariantList::iterator::operator--(int) --> const class wxVariantList::iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxVariantList::iterator::*)(const class wxVariantList::iterator &) const) &wxVariantList::iterator::operator!=, "C++: wxVariantList::iterator::operator!=(const class wxVariantList::iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxVariantList::iterator::*)(const class wxVariantList::iterator &) const) &wxVariantList::iterator::operator==, "C++: wxVariantList::iterator::operator==(const class wxVariantList::iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxVariantList::const_iterator file: line:852
			auto & enclosing_class = cl;
			pybind11::class_<wxVariantList::const_iterator, std::shared_ptr<wxVariantList::const_iterator>> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init<class wxwxVariantListNode *, class wxwxVariantListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxVariantList::const_iterator(); } ) );
			cl.def( pybind11::init<const class wxVariantList::iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxVariantList::const_iterator const &o){ return new wxVariantList::const_iterator(o); } ) );
			cl.def("__mul__", (class wxVariant *const & (wxVariantList::const_iterator::*)() const) &wxVariantList::const_iterator::operator*, "C++: wxVariantList::const_iterator::operator*() const --> class wxVariant *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxVariantList::const_iterator & (wxVariantList::const_iterator::*)()) &wxVariantList::const_iterator::operator++, "C++: wxVariantList::const_iterator::operator++() --> class wxVariantList::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxVariantList::const_iterator (wxVariantList::const_iterator::*)(int)) &wxVariantList::const_iterator::operator++, "C++: wxVariantList::const_iterator::operator++(int) --> const class wxVariantList::const_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxVariantList::const_iterator & (wxVariantList::const_iterator::*)()) &wxVariantList::const_iterator::operator--, "C++: wxVariantList::const_iterator::operator--() --> class wxVariantList::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxVariantList::const_iterator (wxVariantList::const_iterator::*)(int)) &wxVariantList::const_iterator::operator--, "C++: wxVariantList::const_iterator::operator--(int) --> const class wxVariantList::const_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxVariantList::const_iterator::*)(const class wxVariantList::const_iterator &) const) &wxVariantList::const_iterator::operator!=, "C++: wxVariantList::const_iterator::operator!=(const class wxVariantList::const_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxVariantList::const_iterator::*)(const class wxVariantList::const_iterator &) const) &wxVariantList::const_iterator::operator==, "C++: wxVariantList::const_iterator::operator==(const class wxVariantList::const_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxVariantList::reverse_iterator file: line:905
			auto & enclosing_class = cl;
			pybind11::class_<wxVariantList::reverse_iterator, std::shared_ptr<wxVariantList::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init<class wxwxVariantListNode *, class wxwxVariantListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxVariantList::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxVariantList::reverse_iterator const &o){ return new wxVariantList::reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxVariant *& (wxVariantList::reverse_iterator::*)() const) &wxVariantList::reverse_iterator::operator*, "C++: wxVariantList::reverse_iterator::operator*() const --> class wxVariant *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxVariantList::reverse_iterator & (wxVariantList::reverse_iterator::*)()) &wxVariantList::reverse_iterator::operator++, "C++: wxVariantList::reverse_iterator::operator++() --> class wxVariantList::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxVariantList::reverse_iterator (wxVariantList::reverse_iterator::*)(int)) &wxVariantList::reverse_iterator::operator++, "C++: wxVariantList::reverse_iterator::operator++(int) --> const class wxVariantList::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxVariantList::reverse_iterator & (wxVariantList::reverse_iterator::*)()) &wxVariantList::reverse_iterator::operator--, "C++: wxVariantList::reverse_iterator::operator--() --> class wxVariantList::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxVariantList::reverse_iterator (wxVariantList::reverse_iterator::*)(int)) &wxVariantList::reverse_iterator::operator--, "C++: wxVariantList::reverse_iterator::operator--(int) --> const class wxVariantList::reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxVariantList::reverse_iterator::*)(const class wxVariantList::reverse_iterator &) const) &wxVariantList::reverse_iterator::operator!=, "C++: wxVariantList::reverse_iterator::operator!=(const class wxVariantList::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxVariantList::reverse_iterator::*)(const class wxVariantList::reverse_iterator &) const) &wxVariantList::reverse_iterator::operator==, "C++: wxVariantList::reverse_iterator::operator==(const class wxVariantList::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxVariantList::const_reverse_iterator file: line:944
			auto & enclosing_class = cl;
			pybind11::class_<wxVariantList::const_reverse_iterator, std::shared_ptr<wxVariantList::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init<class wxwxVariantListNode *, class wxwxVariantListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxVariantList::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const class wxVariantList::reverse_iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxVariantList::const_reverse_iterator const &o){ return new wxVariantList::const_reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxVariant *const & (wxVariantList::const_reverse_iterator::*)() const) &wxVariantList::const_reverse_iterator::operator*, "C++: wxVariantList::const_reverse_iterator::operator*() const --> class wxVariant *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxVariantList::const_reverse_iterator & (wxVariantList::const_reverse_iterator::*)()) &wxVariantList::const_reverse_iterator::operator++, "C++: wxVariantList::const_reverse_iterator::operator++() --> class wxVariantList::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxVariantList::const_reverse_iterator (wxVariantList::const_reverse_iterator::*)(int)) &wxVariantList::const_reverse_iterator::operator++, "C++: wxVariantList::const_reverse_iterator::operator++(int) --> const class wxVariantList::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxVariantList::const_reverse_iterator & (wxVariantList::const_reverse_iterator::*)()) &wxVariantList::const_reverse_iterator::operator--, "C++: wxVariantList::const_reverse_iterator::operator--() --> class wxVariantList::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxVariantList::const_reverse_iterator (wxVariantList::const_reverse_iterator::*)(int)) &wxVariantList::const_reverse_iterator::operator--, "C++: wxVariantList::const_reverse_iterator::operator--(int) --> const class wxVariantList::const_reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxVariantList::const_reverse_iterator::*)(const class wxVariantList::const_reverse_iterator &) const) &wxVariantList::const_reverse_iterator::operator!=, "C++: wxVariantList::const_reverse_iterator::operator!=(const class wxVariantList::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxVariantList::const_reverse_iterator::*)(const class wxVariantList::const_reverse_iterator &) const) &wxVariantList::const_reverse_iterator::operator==, "C++: wxVariantList::const_reverse_iterator::operator==(const class wxVariantList::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}
