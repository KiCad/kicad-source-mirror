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

// wxList file: line:1198
struct PyCallBack_wxList : public wxList {
	using wxList::wxList;

	class wxObjectListNode * Find(const class wxListKey & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxList *>(this), "Find");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxList *>(this), "CreateNode");
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

// wxStringListNode file: line:1242
struct PyCallBack_wxStringListNode : public wxStringListNode {
	using wxStringListNode::wxStringListNode;

	void DeleteData() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStringListNode *>(this), "DeleteData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxStringListNode::DeleteData();
	}
};

// wxStringListBase file: line:1242
struct PyCallBack_wxStringListBase : public wxStringListBase {
	using wxStringListBase::wxStringListBase;

	class wxStringListNode * Find(const class wxListKey & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStringListBase *>(this), "Find");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxStringListNode *>::value) {
				static pybind11::detail::override_caster_t<class wxStringListNode *> caster;
				return pybind11::detail::cast_ref<class wxStringListNode *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxStringListNode *>(std::move(o));
		}
		return wxStringListBase::Find(a0);
	}
	class wxNodeBase * CreateNode(class wxNodeBase * a0, class wxNodeBase * a1, void * a2, const class wxListKey & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStringListBase *>(this), "CreateNode");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxNodeBase *>::value) {
				static pybind11::detail::override_caster_t<class wxNodeBase *> caster;
				return pybind11::detail::cast_ref<class wxNodeBase *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxNodeBase *>(std::move(o));
		}
		return wxStringListBase::CreateNode(a0, a1, a2, a3);
	}
};

void bind_unknown_unknown_28(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxList file: line:1198
		pybind11::class_<wxList, std::shared_ptr<wxList>, PyCallBack_wxList, wxObjectList> cl(M(""), "wxList", "");
		cl.def( pybind11::init( [](){ return new wxList(); }, [](){ return new PyCallBack_wxList(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("key_type") );

		cl.def( pybind11::init( [](PyCallBack_wxList const &o){ return new PyCallBack_wxList(o); } ) );
		cl.def( pybind11::init( [](wxList const &o){ return new wxList(o); } ) );
		cl.def("assign", (class wxList & (wxList::*)(const class wxList &)) &wxList::operator=, "C++: wxList::operator=(const class wxList &) --> class wxList &", pybind11::return_value_policy::automatic, pybind11::arg("list"));
	}
	{ // wxStringListNode file: line:1242
		pybind11::class_<wxStringListNode, std::shared_ptr<wxStringListNode>, PyCallBack_wxStringListNode, wxNodeBase> cl(M(""), "wxStringListNode", "");
		cl.def( pybind11::init( [](){ return new wxStringListNode(); }, [](){ return new PyCallBack_wxStringListNode(); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0){ return new wxStringListNode(a0); }, [](class wxListBase * a0){ return new PyCallBack_wxStringListNode(a0); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxStringListNode * a1){ return new wxStringListNode(a0, a1); }, [](class wxListBase * a0, class wxStringListNode * a1){ return new PyCallBack_wxStringListNode(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxStringListNode * a1, class wxStringListNode * a2){ return new wxStringListNode(a0, a1, a2); }, [](class wxListBase * a0, class wxStringListNode * a1, class wxStringListNode * a2){ return new PyCallBack_wxStringListNode(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxStringListNode * a1, class wxStringListNode * a2, wchar_t * a3){ return new wxStringListNode(a0, a1, a2, a3); }, [](class wxListBase * a0, class wxStringListNode * a1, class wxStringListNode * a2, wchar_t * a3){ return new PyCallBack_wxStringListNode(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<class wxListBase *, class wxStringListNode *, class wxStringListNode *, wchar_t *, const class wxListKey &>(), pybind11::arg("list"), pybind11::arg("previous"), pybind11::arg("next"), pybind11::arg("data"), pybind11::arg("key") );

		cl.def("GetNext", (class wxStringListNode * (wxStringListNode::*)() const) &wxStringListNode::GetNext, "C++: wxStringListNode::GetNext() const --> class wxStringListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetPrevious", (class wxStringListNode * (wxStringListNode::*)() const) &wxStringListNode::GetPrevious, "C++: wxStringListNode::GetPrevious() const --> class wxStringListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetData", (wchar_t * (wxStringListNode::*)() const) &wxStringListNode::GetData, "C++: wxStringListNode::GetData() const --> wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("SetData", (void (wxStringListNode::*)(wchar_t *)) &wxStringListNode::SetData, "C++: wxStringListNode::SetData(wchar_t *) --> void", pybind11::arg("data"));
	}
	{ // wxStringListBase file: line:1242
		pybind11::class_<wxStringListBase, std::shared_ptr<wxStringListBase>, PyCallBack_wxStringListBase, wxListBase> cl(M(""), "wxStringListBase", "");
		cl.def( pybind11::init( [](){ return new wxStringListBase(); }, [](){ return new PyCallBack_wxStringListBase(); } ), "doc");
		cl.def( pybind11::init<enum wxKeyType>(), pybind11::arg("keyType") );

		cl.def( pybind11::init( [](PyCallBack_wxStringListBase const &o){ return new PyCallBack_wxStringListBase(o); } ) );
		cl.def( pybind11::init( [](wxStringListBase const &o){ return new wxStringListBase(o); } ) );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxStringListBase(a0); }, [](unsigned long const & a0){ return new PyCallBack_wxStringListBase(a0); } ), "doc");
		cl.def( pybind11::init<unsigned long, wchar_t *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const class wxStringListBase::const_iterator &, const class wxStringListBase::const_iterator &>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def("assign", (class wxStringListBase & (wxStringListBase::*)(const class wxStringListBase &)) &wxStringListBase::operator=, "C++: wxStringListBase::operator=(const class wxStringListBase &) --> class wxStringListBase &", pybind11::return_value_policy::automatic, pybind11::arg("list"));
		cl.def("GetFirst", (class wxStringListNode * (wxStringListBase::*)() const) &wxStringListBase::GetFirst, "C++: wxStringListBase::GetFirst() const --> class wxStringListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetLast", (class wxStringListNode * (wxStringListBase::*)() const) &wxStringListBase::GetLast, "C++: wxStringListBase::GetLast() const --> class wxStringListNode *", pybind11::return_value_policy::automatic);
		cl.def("Item", (class wxStringListNode * (wxStringListBase::*)(unsigned long) const) &wxStringListBase::Item, "C++: wxStringListBase::Item(unsigned long) const --> class wxStringListNode *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("__getitem__", (wchar_t * (wxStringListBase::*)(unsigned long) const) &wxStringListBase::operator[], "C++: wxStringListBase::operator[](unsigned long) const --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("Append", (class wxStringListNode * (wxStringListBase::*)(wchar_t *)) &wxStringListBase::Append, "C++: wxStringListBase::Append(wchar_t *) --> class wxStringListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxStringListNode * (wxStringListBase::*)(wchar_t *)) &wxStringListBase::Insert, "C++: wxStringListBase::Insert(wchar_t *) --> class wxStringListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxStringListNode * (wxStringListBase::*)(unsigned long, wchar_t *)) &wxStringListBase::Insert, "C++: wxStringListBase::Insert(unsigned long, wchar_t *) --> class wxStringListNode *", pybind11::return_value_policy::automatic, pybind11::arg("pos"), pybind11::arg("object"));
		cl.def("Insert", (class wxStringListNode * (wxStringListBase::*)(class wxStringListNode *, wchar_t *)) &wxStringListBase::Insert, "C++: wxStringListBase::Insert(class wxStringListNode *, wchar_t *) --> class wxStringListNode *", pybind11::return_value_policy::automatic, pybind11::arg("prev"), pybind11::arg("object"));
		cl.def("Append", (class wxStringListNode * (wxStringListBase::*)(long, void *)) &wxStringListBase::Append, "C++: wxStringListBase::Append(long, void *) --> class wxStringListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("Append", (class wxStringListNode * (wxStringListBase::*)(const wchar_t *, void *)) &wxStringListBase::Append, "C++: wxStringListBase::Append(const wchar_t *, void *) --> class wxStringListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("DetachNode", (class wxStringListNode * (wxStringListBase::*)(class wxStringListNode *)) &wxStringListBase::DetachNode, "C++: wxStringListBase::DetachNode(class wxStringListNode *) --> class wxStringListNode *", pybind11::return_value_policy::automatic, pybind11::arg("node"));
		cl.def("DeleteNode", (bool (wxStringListBase::*)(class wxStringListNode *)) &wxStringListBase::DeleteNode, "C++: wxStringListBase::DeleteNode(class wxStringListNode *) --> bool", pybind11::arg("node"));
		cl.def("DeleteObject", (bool (wxStringListBase::*)(wchar_t *)) &wxStringListBase::DeleteObject, "C++: wxStringListBase::DeleteObject(wchar_t *) --> bool", pybind11::arg("object"));
		cl.def("Erase", (void (wxStringListBase::*)(class wxStringListNode *)) &wxStringListBase::Erase, "C++: wxStringListBase::Erase(class wxStringListNode *) --> void", pybind11::arg("it"));
		cl.def("Find", (class wxStringListNode * (wxStringListBase::*)(const wchar_t *) const) &wxStringListBase::Find, "C++: wxStringListBase::Find(const wchar_t *) const --> class wxStringListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Find", (class wxStringListNode * (wxStringListBase::*)(const class wxListKey &) const) &wxStringListBase::Find, "C++: wxStringListBase::Find(const class wxListKey &) const --> class wxStringListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("Member", (bool (wxStringListBase::*)(const wchar_t *) const) &wxStringListBase::Member, "C++: wxStringListBase::Member(const wchar_t *) const --> bool", pybind11::arg("object"));
		cl.def("IndexOf", (int (wxStringListBase::*)(wchar_t *) const) &wxStringListBase::IndexOf, "C++: wxStringListBase::IndexOf(wchar_t *) const --> int", pybind11::arg("object"));
		cl.def("begin", (class wxStringListBase::iterator (wxStringListBase::*)()) &wxStringListBase::begin, "C++: wxStringListBase::begin() --> class wxStringListBase::iterator");
		cl.def("end", (class wxStringListBase::iterator (wxStringListBase::*)()) &wxStringListBase::end, "C++: wxStringListBase::end() --> class wxStringListBase::iterator");
		cl.def("rbegin", (class wxStringListBase::reverse_iterator (wxStringListBase::*)()) &wxStringListBase::rbegin, "C++: wxStringListBase::rbegin() --> class wxStringListBase::reverse_iterator");
		cl.def("rend", (class wxStringListBase::reverse_iterator (wxStringListBase::*)()) &wxStringListBase::rend, "C++: wxStringListBase::rend() --> class wxStringListBase::reverse_iterator");
		cl.def("resize", [](wxStringListBase &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxStringListBase::*)(unsigned long, wchar_t *)) &wxStringListBase::resize, "C++: wxStringListBase::resize(unsigned long, wchar_t *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("size", (unsigned long (wxStringListBase::*)() const) &wxStringListBase::size, "C++: wxStringListBase::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxStringListBase::*)() const) &wxStringListBase::max_size, "C++: wxStringListBase::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxStringListBase::*)() const) &wxStringListBase::empty, "C++: wxStringListBase::empty() const --> bool");
		cl.def("front", (wchar_t *& (wxStringListBase::*)()) &wxStringListBase::front, "C++: wxStringListBase::front() --> wchar_t *&", pybind11::return_value_policy::automatic);
		cl.def("back", (wchar_t *& (wxStringListBase::*)()) &wxStringListBase::back, "C++: wxStringListBase::back() --> wchar_t *&", pybind11::return_value_policy::automatic);
		cl.def("push_front", [](wxStringListBase &o) -> void { return o.push_front(); }, "");
		cl.def("push_front", (void (wxStringListBase::*)(wchar_t *const &)) &wxStringListBase::push_front, "C++: wxStringListBase::push_front(wchar_t *const &) --> void", pybind11::arg("v"));
		cl.def("pop_front", (void (wxStringListBase::*)()) &wxStringListBase::pop_front, "C++: wxStringListBase::pop_front() --> void");
		cl.def("push_back", [](wxStringListBase &o) -> void { return o.push_back(); }, "");
		cl.def("push_back", (void (wxStringListBase::*)(wchar_t *const &)) &wxStringListBase::push_back, "C++: wxStringListBase::push_back(wchar_t *const &) --> void", pybind11::arg("v"));
		cl.def("pop_back", (void (wxStringListBase::*)()) &wxStringListBase::pop_back, "C++: wxStringListBase::pop_back() --> void");
		cl.def("assign", (void (wxStringListBase::*)(class wxStringListBase::const_iterator, const class wxStringListBase::const_iterator &)) &wxStringListBase::assign, "C++: wxStringListBase::assign(class wxStringListBase::const_iterator, const class wxStringListBase::const_iterator &) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", [](wxStringListBase &o, unsigned long const & a0) -> void { return o.assign(a0); }, "", pybind11::arg("n"));
		cl.def("assign", (void (wxStringListBase::*)(unsigned long, wchar_t *const &)) &wxStringListBase::assign, "C++: wxStringListBase::assign(unsigned long, wchar_t *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (class wxStringListBase::iterator (wxStringListBase::*)(const class wxStringListBase::iterator &, wchar_t *const &)) &wxStringListBase::insert, "C++: wxStringListBase::insert(const class wxStringListBase::iterator &, wchar_t *const &) --> class wxStringListBase::iterator", pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxStringListBase::*)(const class wxStringListBase::iterator &, unsigned long, wchar_t *const &)) &wxStringListBase::insert, "C++: wxStringListBase::insert(const class wxStringListBase::iterator &, unsigned long, wchar_t *const &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (void (wxStringListBase::*)(const class wxStringListBase::iterator &, class wxStringListBase::const_iterator, const class wxStringListBase::const_iterator &)) &wxStringListBase::insert, "C++: wxStringListBase::insert(const class wxStringListBase::iterator &, class wxStringListBase::const_iterator, const class wxStringListBase::const_iterator &) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (class wxStringListBase::iterator (wxStringListBase::*)(const class wxStringListBase::iterator &)) &wxStringListBase::erase, "C++: wxStringListBase::erase(const class wxStringListBase::iterator &) --> class wxStringListBase::iterator", pybind11::arg("it"));
		cl.def("erase", (class wxStringListBase::iterator (wxStringListBase::*)(const class wxStringListBase::iterator &, const class wxStringListBase::iterator &)) &wxStringListBase::erase, "C++: wxStringListBase::erase(const class wxStringListBase::iterator &, const class wxStringListBase::iterator &) --> class wxStringListBase::iterator", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("clear", (void (wxStringListBase::*)()) &wxStringListBase::clear, "C++: wxStringListBase::clear() --> void");
		cl.def("splice", (void (wxStringListBase::*)(const class wxStringListBase::iterator &, class wxStringListBase &, const class wxStringListBase::iterator &, const class wxStringListBase::iterator &)) &wxStringListBase::splice, "C++: wxStringListBase::splice(const class wxStringListBase::iterator &, class wxStringListBase &, const class wxStringListBase::iterator &, const class wxStringListBase::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("splice", (void (wxStringListBase::*)(const class wxStringListBase::iterator &, class wxStringListBase &)) &wxStringListBase::splice, "C++: wxStringListBase::splice(const class wxStringListBase::iterator &, class wxStringListBase &) --> void", pybind11::arg("it"), pybind11::arg("l"));
		cl.def("splice", (void (wxStringListBase::*)(const class wxStringListBase::iterator &, class wxStringListBase &, const class wxStringListBase::iterator &)) &wxStringListBase::splice, "C++: wxStringListBase::splice(const class wxStringListBase::iterator &, class wxStringListBase &, const class wxStringListBase::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"));
		cl.def("remove", (void (wxStringListBase::*)(wchar_t *const &)) &wxStringListBase::remove, "C++: wxStringListBase::remove(wchar_t *const &) --> void", pybind11::arg("v"));
		cl.def("reverse", (void (wxStringListBase::*)()) &wxStringListBase::reverse, "C++: wxStringListBase::reverse() --> void");

		{ // wxStringListBase::compatibility_iterator file: line:705
			auto & enclosing_class = cl;
			pybind11::class_<wxStringListBase::compatibility_iterator, std::shared_ptr<wxStringListBase::compatibility_iterator>> cl(enclosing_class, "compatibility_iterator", "");
			cl.def( pybind11::init( [](){ return new wxStringListBase::compatibility_iterator(); } ), "doc" );
			cl.def( pybind11::init<class wxStringListNode *>(), pybind11::arg("ptr") );

		}

		{ // wxStringListBase::iterator file: line:802
			auto & enclosing_class = cl;
			pybind11::class_<wxStringListBase::iterator, std::shared_ptr<wxStringListBase::iterator>> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init<class wxStringListNode *, class wxStringListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxStringListBase::iterator(); } ) );
			cl.def( pybind11::init( [](wxStringListBase::iterator const &o){ return new wxStringListBase::iterator(o); } ) );
			cl.def("__mul__", (wchar_t *& (wxStringListBase::iterator::*)() const) &wxStringListBase::iterator::operator*, "C++: wxStringListBase::iterator::operator*() const --> wchar_t *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxStringListBase::iterator & (wxStringListBase::iterator::*)()) &wxStringListBase::iterator::operator++, "C++: wxStringListBase::iterator::operator++() --> class wxStringListBase::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxStringListBase::iterator (wxStringListBase::iterator::*)(int)) &wxStringListBase::iterator::operator++, "C++: wxStringListBase::iterator::operator++(int) --> const class wxStringListBase::iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxStringListBase::iterator & (wxStringListBase::iterator::*)()) &wxStringListBase::iterator::operator--, "C++: wxStringListBase::iterator::operator--() --> class wxStringListBase::iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxStringListBase::iterator (wxStringListBase::iterator::*)(int)) &wxStringListBase::iterator::operator--, "C++: wxStringListBase::iterator::operator--(int) --> const class wxStringListBase::iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxStringListBase::iterator::*)(const class wxStringListBase::iterator &) const) &wxStringListBase::iterator::operator!=, "C++: wxStringListBase::iterator::operator!=(const class wxStringListBase::iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxStringListBase::iterator::*)(const class wxStringListBase::iterator &) const) &wxStringListBase::iterator::operator==, "C++: wxStringListBase::iterator::operator==(const class wxStringListBase::iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxStringListBase::const_iterator file: line:852
			auto & enclosing_class = cl;
			pybind11::class_<wxStringListBase::const_iterator, std::shared_ptr<wxStringListBase::const_iterator>> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init<class wxStringListNode *, class wxStringListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxStringListBase::const_iterator(); } ) );
			cl.def( pybind11::init<const class wxStringListBase::iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxStringListBase::const_iterator const &o){ return new wxStringListBase::const_iterator(o); } ) );
			cl.def("__mul__", (wchar_t *const & (wxStringListBase::const_iterator::*)() const) &wxStringListBase::const_iterator::operator*, "C++: wxStringListBase::const_iterator::operator*() const --> wchar_t *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxStringListBase::const_iterator & (wxStringListBase::const_iterator::*)()) &wxStringListBase::const_iterator::operator++, "C++: wxStringListBase::const_iterator::operator++() --> class wxStringListBase::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxStringListBase::const_iterator (wxStringListBase::const_iterator::*)(int)) &wxStringListBase::const_iterator::operator++, "C++: wxStringListBase::const_iterator::operator++(int) --> const class wxStringListBase::const_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxStringListBase::const_iterator & (wxStringListBase::const_iterator::*)()) &wxStringListBase::const_iterator::operator--, "C++: wxStringListBase::const_iterator::operator--() --> class wxStringListBase::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxStringListBase::const_iterator (wxStringListBase::const_iterator::*)(int)) &wxStringListBase::const_iterator::operator--, "C++: wxStringListBase::const_iterator::operator--(int) --> const class wxStringListBase::const_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxStringListBase::const_iterator::*)(const class wxStringListBase::const_iterator &) const) &wxStringListBase::const_iterator::operator!=, "C++: wxStringListBase::const_iterator::operator!=(const class wxStringListBase::const_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxStringListBase::const_iterator::*)(const class wxStringListBase::const_iterator &) const) &wxStringListBase::const_iterator::operator==, "C++: wxStringListBase::const_iterator::operator==(const class wxStringListBase::const_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxStringListBase::reverse_iterator file: line:905
			auto & enclosing_class = cl;
			pybind11::class_<wxStringListBase::reverse_iterator, std::shared_ptr<wxStringListBase::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init<class wxStringListNode *, class wxStringListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxStringListBase::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxStringListBase::reverse_iterator const &o){ return new wxStringListBase::reverse_iterator(o); } ) );
			cl.def("__mul__", (wchar_t *& (wxStringListBase::reverse_iterator::*)() const) &wxStringListBase::reverse_iterator::operator*, "C++: wxStringListBase::reverse_iterator::operator*() const --> wchar_t *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxStringListBase::reverse_iterator & (wxStringListBase::reverse_iterator::*)()) &wxStringListBase::reverse_iterator::operator++, "C++: wxStringListBase::reverse_iterator::operator++() --> class wxStringListBase::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxStringListBase::reverse_iterator (wxStringListBase::reverse_iterator::*)(int)) &wxStringListBase::reverse_iterator::operator++, "C++: wxStringListBase::reverse_iterator::operator++(int) --> const class wxStringListBase::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxStringListBase::reverse_iterator & (wxStringListBase::reverse_iterator::*)()) &wxStringListBase::reverse_iterator::operator--, "C++: wxStringListBase::reverse_iterator::operator--() --> class wxStringListBase::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxStringListBase::reverse_iterator (wxStringListBase::reverse_iterator::*)(int)) &wxStringListBase::reverse_iterator::operator--, "C++: wxStringListBase::reverse_iterator::operator--(int) --> const class wxStringListBase::reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxStringListBase::reverse_iterator::*)(const class wxStringListBase::reverse_iterator &) const) &wxStringListBase::reverse_iterator::operator!=, "C++: wxStringListBase::reverse_iterator::operator!=(const class wxStringListBase::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxStringListBase::reverse_iterator::*)(const class wxStringListBase::reverse_iterator &) const) &wxStringListBase::reverse_iterator::operator==, "C++: wxStringListBase::reverse_iterator::operator==(const class wxStringListBase::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxStringListBase::const_reverse_iterator file: line:944
			auto & enclosing_class = cl;
			pybind11::class_<wxStringListBase::const_reverse_iterator, std::shared_ptr<wxStringListBase::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init<class wxStringListNode *, class wxStringListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxStringListBase::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const class wxStringListBase::reverse_iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxStringListBase::const_reverse_iterator const &o){ return new wxStringListBase::const_reverse_iterator(o); } ) );
			cl.def("__mul__", (wchar_t *const & (wxStringListBase::const_reverse_iterator::*)() const) &wxStringListBase::const_reverse_iterator::operator*, "C++: wxStringListBase::const_reverse_iterator::operator*() const --> wchar_t *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxStringListBase::const_reverse_iterator & (wxStringListBase::const_reverse_iterator::*)()) &wxStringListBase::const_reverse_iterator::operator++, "C++: wxStringListBase::const_reverse_iterator::operator++() --> class wxStringListBase::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxStringListBase::const_reverse_iterator (wxStringListBase::const_reverse_iterator::*)(int)) &wxStringListBase::const_reverse_iterator::operator++, "C++: wxStringListBase::const_reverse_iterator::operator++(int) --> const class wxStringListBase::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxStringListBase::const_reverse_iterator & (wxStringListBase::const_reverse_iterator::*)()) &wxStringListBase::const_reverse_iterator::operator--, "C++: wxStringListBase::const_reverse_iterator::operator--() --> class wxStringListBase::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxStringListBase::const_reverse_iterator (wxStringListBase::const_reverse_iterator::*)(int)) &wxStringListBase::const_reverse_iterator::operator--, "C++: wxStringListBase::const_reverse_iterator::operator--(int) --> const class wxStringListBase::const_reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxStringListBase::const_reverse_iterator::*)(const class wxStringListBase::const_reverse_iterator &) const) &wxStringListBase::const_reverse_iterator::operator!=, "C++: wxStringListBase::const_reverse_iterator::operator!=(const class wxStringListBase::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxStringListBase::const_reverse_iterator::*)(const class wxStringListBase::const_reverse_iterator &) const) &wxStringListBase::const_reverse_iterator::operator==, "C++: wxStringListBase::const_reverse_iterator::operator==(const class wxStringListBase::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}
