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

// wxPointList file: line:690
struct PyCallBack_wxPointList : public wxPointList {
	using wxPointList::wxPointList;

	class wxwxPointListNode * Find(const class wxListKey & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPointList *>(this), "Find");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxwxPointListNode *>::value) {
				static pybind11::detail::override_caster_t<class wxwxPointListNode *> caster;
				return pybind11::detail::cast_ref<class wxwxPointListNode *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxwxPointListNode *>(std::move(o));
		}
		return wxPointList::Find(a0);
	}
	class wxNodeBase * CreateNode(class wxNodeBase * a0, class wxNodeBase * a1, void * a2, const class wxListKey & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPointList *>(this), "CreateNode");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxNodeBase *>::value) {
				static pybind11::detail::override_caster_t<class wxNodeBase *> caster;
				return pybind11::detail::cast_ref<class wxNodeBase *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxNodeBase *>(std::move(o));
		}
		return wxPointList::CreateNode(a0, a1, a2, a3);
	}
};

void bind_unknown_unknown_31(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPointList file: line:690
		pybind11::class_<wxPointList, std::shared_ptr<wxPointList>, PyCallBack_wxPointList, wxListBase> cl(M(""), "wxPointList", "");
		cl.def( pybind11::init( [](){ return new wxPointList(); }, [](){ return new PyCallBack_wxPointList(); } ), "doc");
		cl.def( pybind11::init<enum wxKeyType>(), pybind11::arg("keyType") );

		cl.def( pybind11::init( [](PyCallBack_wxPointList const &o){ return new PyCallBack_wxPointList(o); } ) );
		cl.def( pybind11::init( [](wxPointList const &o){ return new wxPointList(o); } ) );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxPointList(a0); }, [](unsigned long const & a0){ return new PyCallBack_wxPointList(a0); } ), "doc");
		cl.def( pybind11::init<unsigned long, class wxPoint *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const class wxPointList::const_iterator &, const class wxPointList::const_iterator &>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def("assign", (class wxPointList & (wxPointList::*)(const class wxPointList &)) &wxPointList::operator=, "C++: wxPointList::operator=(const class wxPointList &) --> class wxPointList &", pybind11::return_value_policy::automatic, pybind11::arg("list"));
		cl.def("GetFirst", (class wxwxPointListNode * (wxPointList::*)() const) &wxPointList::GetFirst, "C++: wxPointList::GetFirst() const --> class wxwxPointListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetLast", (class wxwxPointListNode * (wxPointList::*)() const) &wxPointList::GetLast, "C++: wxPointList::GetLast() const --> class wxwxPointListNode *", pybind11::return_value_policy::automatic);
		cl.def("Item", (class wxwxPointListNode * (wxPointList::*)(unsigned long) const) &wxPointList::Item, "C++: wxPointList::Item(unsigned long) const --> class wxwxPointListNode *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("__getitem__", (class wxPoint * (wxPointList::*)(unsigned long) const) &wxPointList::operator[], "C++: wxPointList::operator[](unsigned long) const --> class wxPoint *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("Append", (class wxwxPointListNode * (wxPointList::*)(class wxPoint *)) &wxPointList::Append, "C++: wxPointList::Append(class wxPoint *) --> class wxwxPointListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxwxPointListNode * (wxPointList::*)(class wxPoint *)) &wxPointList::Insert, "C++: wxPointList::Insert(class wxPoint *) --> class wxwxPointListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxwxPointListNode * (wxPointList::*)(unsigned long, class wxPoint *)) &wxPointList::Insert, "C++: wxPointList::Insert(unsigned long, class wxPoint *) --> class wxwxPointListNode *", pybind11::return_value_policy::automatic, pybind11::arg("pos"), pybind11::arg("object"));
		cl.def("Insert", (class wxwxPointListNode * (wxPointList::*)(class wxwxPointListNode *, class wxPoint *)) &wxPointList::Insert, "C++: wxPointList::Insert(class wxwxPointListNode *, class wxPoint *) --> class wxwxPointListNode *", pybind11::return_value_policy::automatic, pybind11::arg("prev"), pybind11::arg("object"));
		cl.def("Append", (class wxwxPointListNode * (wxPointList::*)(long, void *)) &wxPointList::Append, "C++: wxPointList::Append(long, void *) --> class wxwxPointListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("Append", (class wxwxPointListNode * (wxPointList::*)(const wchar_t *, void *)) &wxPointList::Append, "C++: wxPointList::Append(const wchar_t *, void *) --> class wxwxPointListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("DetachNode", (class wxwxPointListNode * (wxPointList::*)(class wxwxPointListNode *)) &wxPointList::DetachNode, "C++: wxPointList::DetachNode(class wxwxPointListNode *) --> class wxwxPointListNode *", pybind11::return_value_policy::automatic, pybind11::arg("node"));
		cl.def("DeleteNode", (bool (wxPointList::*)(class wxwxPointListNode *)) &wxPointList::DeleteNode, "C++: wxPointList::DeleteNode(class wxwxPointListNode *) --> bool", pybind11::arg("node"));
		cl.def("DeleteObject", (bool (wxPointList::*)(class wxPoint *)) &wxPointList::DeleteObject, "C++: wxPointList::DeleteObject(class wxPoint *) --> bool", pybind11::arg("object"));
		cl.def("Erase", (void (wxPointList::*)(class wxwxPointListNode *)) &wxPointList::Erase, "C++: wxPointList::Erase(class wxwxPointListNode *) --> void", pybind11::arg("it"));
		cl.def("Find", (class wxwxPointListNode * (wxPointList::*)(const class wxPoint *) const) &wxPointList::Find, "C++: wxPointList::Find(const class wxPoint *) const --> class wxwxPointListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Find", (class wxwxPointListNode * (wxPointList::*)(const class wxListKey &) const) &wxPointList::Find, "C++: wxPointList::Find(const class wxListKey &) const --> class wxwxPointListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("Member", (bool (wxPointList::*)(const class wxPoint *) const) &wxPointList::Member, "C++: wxPointList::Member(const class wxPoint *) const --> bool", pybind11::arg("object"));
		cl.def("IndexOf", (int (wxPointList::*)(class wxPoint *) const) &wxPointList::IndexOf, "C++: wxPointList::IndexOf(class wxPoint *) const --> int", pybind11::arg("object"));
		cl.def("begin", (class wxPointList::iterator (wxPointList::*)()) &wxPointList::begin, "C++: wxPointList::begin() --> class wxPointList::iterator");
		cl.def("end", (class wxPointList::iterator (wxPointList::*)()) &wxPointList::end, "C++: wxPointList::end() --> class wxPointList::iterator");
		cl.def("rbegin", (class wxPointList::reverse_iterator (wxPointList::*)()) &wxPointList::rbegin, "C++: wxPointList::rbegin() --> class wxPointList::reverse_iterator");
		cl.def("rend", (class wxPointList::reverse_iterator (wxPointList::*)()) &wxPointList::rend, "C++: wxPointList::rend() --> class wxPointList::reverse_iterator");
		cl.def("resize", [](wxPointList &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxPointList::*)(unsigned long, class wxPoint *)) &wxPointList::resize, "C++: wxPointList::resize(unsigned long, class wxPoint *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("size", (unsigned long (wxPointList::*)() const) &wxPointList::size, "C++: wxPointList::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxPointList::*)() const) &wxPointList::max_size, "C++: wxPointList::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxPointList::*)() const) &wxPointList::empty, "C++: wxPointList::empty() const --> bool");
		cl.def("front", (class wxPoint *& (wxPointList::*)()) &wxPointList::front, "C++: wxPointList::front() --> class wxPoint *&", pybind11::return_value_policy::automatic);
		cl.def("back", (class wxPoint *& (wxPointList::*)()) &wxPointList::back, "C++: wxPointList::back() --> class wxPoint *&", pybind11::return_value_policy::automatic);
		cl.def("push_front", [](wxPointList &o) -> void { return o.push_front(); }, "");
		cl.def("push_front", (void (wxPointList::*)(class wxPoint *const &)) &wxPointList::push_front, "C++: wxPointList::push_front(class wxPoint *const &) --> void", pybind11::arg("v"));
		cl.def("pop_front", (void (wxPointList::*)()) &wxPointList::pop_front, "C++: wxPointList::pop_front() --> void");
		cl.def("push_back", [](wxPointList &o) -> void { return o.push_back(); }, "");
		cl.def("push_back", (void (wxPointList::*)(class wxPoint *const &)) &wxPointList::push_back, "C++: wxPointList::push_back(class wxPoint *const &) --> void", pybind11::arg("v"));
		cl.def("pop_back", (void (wxPointList::*)()) &wxPointList::pop_back, "C++: wxPointList::pop_back() --> void");
		cl.def("assign", (void (wxPointList::*)(class wxPointList::const_iterator, const class wxPointList::const_iterator &)) &wxPointList::assign, "C++: wxPointList::assign(class wxPointList::const_iterator, const class wxPointList::const_iterator &) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", [](wxPointList &o, unsigned long const & a0) -> void { return o.assign(a0); }, "", pybind11::arg("n"));
		cl.def("assign", (void (wxPointList::*)(unsigned long, class wxPoint *const &)) &wxPointList::assign, "C++: wxPointList::assign(unsigned long, class wxPoint *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (class wxPointList::iterator (wxPointList::*)(const class wxPointList::iterator &, class wxPoint *const &)) &wxPointList::insert, "C++: wxPointList::insert(const class wxPointList::iterator &, class wxPoint *const &) --> class wxPointList::iterator", pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxPointList::*)(const class wxPointList::iterator &, unsigned long, class wxPoint *const &)) &wxPointList::insert, "C++: wxPointList::insert(const class wxPointList::iterator &, unsigned long, class wxPoint *const &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (void (wxPointList::*)(const class wxPointList::iterator &, class wxPointList::const_iterator, const class wxPointList::const_iterator &)) &wxPointList::insert, "C++: wxPointList::insert(const class wxPointList::iterator &, class wxPointList::const_iterator, const class wxPointList::const_iterator &) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (class wxPointList::iterator (wxPointList::*)(const class wxPointList::iterator &)) &wxPointList::erase, "C++: wxPointList::erase(const class wxPointList::iterator &) --> class wxPointList::iterator", pybind11::arg("it"));
		cl.def("erase", (class wxPointList::iterator (wxPointList::*)(const class wxPointList::iterator &, const class wxPointList::iterator &)) &wxPointList::erase, "C++: wxPointList::erase(const class wxPointList::iterator &, const class wxPointList::iterator &) --> class wxPointList::iterator", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("clear", (void (wxPointList::*)()) &wxPointList::clear, "C++: wxPointList::clear() --> void");
		cl.def("splice", (void (wxPointList::*)(const class wxPointList::iterator &, class wxPointList &, const class wxPointList::iterator &, const class wxPointList::iterator &)) &wxPointList::splice, "C++: wxPointList::splice(const class wxPointList::iterator &, class wxPointList &, const class wxPointList::iterator &, const class wxPointList::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("splice", (void (wxPointList::*)(const class wxPointList::iterator &, class wxPointList &)) &wxPointList::splice, "C++: wxPointList::splice(const class wxPointList::iterator &, class wxPointList &) --> void", pybind11::arg("it"), pybind11::arg("l"));
		cl.def("splice", (void (wxPointList::*)(const class wxPointList::iterator &, class wxPointList &, const class wxPointList::iterator &)) &wxPointList::splice, "C++: wxPointList::splice(const class wxPointList::iterator &, class wxPointList &, const class wxPointList::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"));
		cl.def("remove", (void (wxPointList::*)(class wxPoint *const &)) &wxPointList::remove, "C++: wxPointList::remove(class wxPoint *const &) --> void", pybind11::arg("v"));
		cl.def("reverse", (void (wxPointList::*)()) &wxPointList::reverse, "C++: wxPointList::reverse() --> void");

		{ // wxPointList::compatibility_iterator file: line:705
			auto & enclosing_class = cl;
			pybind11::class_<wxPointList::compatibility_iterator, std::shared_ptr<wxPointList::compatibility_iterator>> cl(enclosing_class, "compatibility_iterator", "");
			cl.def( pybind11::init( [](){ return new wxPointList::compatibility_iterator(); } ), "doc" );
			cl.def( pybind11::init<class wxwxPointListNode *>(), pybind11::arg("ptr") );

		}

		{ // wxPointList::iterator file: line:802
			auto & enclosing_class = cl;
			pybind11::class_<wxPointList::iterator, std::shared_ptr<wxPointList::iterator>> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init<class wxwxPointListNode *, class wxwxPointListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxPointList::iterator(); } ) );
			cl.def( pybind11::init( [](wxPointList::iterator const &o){ return new wxPointList::iterator(o); } ) );
			cl.def("__mul__", (class wxPoint *& (wxPointList::iterator::*)() const) &wxPointList::iterator::operator*, "C++: wxPointList::iterator::operator*() const --> class wxPoint *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxPointList::iterator & (wxPointList::iterator::*)()) &wxPointList::iterator::operator++, "C++: wxPointList::iterator::operator++() --> class wxPointList::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxPointList::iterator (wxPointList::iterator::*)(int)) &wxPointList::iterator::operator++, "C++: wxPointList::iterator::operator++(int) --> const class wxPointList::iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxPointList::iterator & (wxPointList::iterator::*)()) &wxPointList::iterator::operator--, "C++: wxPointList::iterator::operator--() --> class wxPointList::iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxPointList::iterator (wxPointList::iterator::*)(int)) &wxPointList::iterator::operator--, "C++: wxPointList::iterator::operator--(int) --> const class wxPointList::iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxPointList::iterator::*)(const class wxPointList::iterator &) const) &wxPointList::iterator::operator!=, "C++: wxPointList::iterator::operator!=(const class wxPointList::iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxPointList::iterator::*)(const class wxPointList::iterator &) const) &wxPointList::iterator::operator==, "C++: wxPointList::iterator::operator==(const class wxPointList::iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxPointList::const_iterator file: line:852
			auto & enclosing_class = cl;
			pybind11::class_<wxPointList::const_iterator, std::shared_ptr<wxPointList::const_iterator>> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init<class wxwxPointListNode *, class wxwxPointListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxPointList::const_iterator(); } ) );
			cl.def( pybind11::init<const class wxPointList::iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxPointList::const_iterator const &o){ return new wxPointList::const_iterator(o); } ) );
			cl.def("__mul__", (class wxPoint *const & (wxPointList::const_iterator::*)() const) &wxPointList::const_iterator::operator*, "C++: wxPointList::const_iterator::operator*() const --> class wxPoint *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxPointList::const_iterator & (wxPointList::const_iterator::*)()) &wxPointList::const_iterator::operator++, "C++: wxPointList::const_iterator::operator++() --> class wxPointList::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxPointList::const_iterator (wxPointList::const_iterator::*)(int)) &wxPointList::const_iterator::operator++, "C++: wxPointList::const_iterator::operator++(int) --> const class wxPointList::const_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxPointList::const_iterator & (wxPointList::const_iterator::*)()) &wxPointList::const_iterator::operator--, "C++: wxPointList::const_iterator::operator--() --> class wxPointList::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxPointList::const_iterator (wxPointList::const_iterator::*)(int)) &wxPointList::const_iterator::operator--, "C++: wxPointList::const_iterator::operator--(int) --> const class wxPointList::const_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxPointList::const_iterator::*)(const class wxPointList::const_iterator &) const) &wxPointList::const_iterator::operator!=, "C++: wxPointList::const_iterator::operator!=(const class wxPointList::const_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxPointList::const_iterator::*)(const class wxPointList::const_iterator &) const) &wxPointList::const_iterator::operator==, "C++: wxPointList::const_iterator::operator==(const class wxPointList::const_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxPointList::reverse_iterator file: line:905
			auto & enclosing_class = cl;
			pybind11::class_<wxPointList::reverse_iterator, std::shared_ptr<wxPointList::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init<class wxwxPointListNode *, class wxwxPointListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxPointList::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxPointList::reverse_iterator const &o){ return new wxPointList::reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxPoint *& (wxPointList::reverse_iterator::*)() const) &wxPointList::reverse_iterator::operator*, "C++: wxPointList::reverse_iterator::operator*() const --> class wxPoint *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxPointList::reverse_iterator & (wxPointList::reverse_iterator::*)()) &wxPointList::reverse_iterator::operator++, "C++: wxPointList::reverse_iterator::operator++() --> class wxPointList::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxPointList::reverse_iterator (wxPointList::reverse_iterator::*)(int)) &wxPointList::reverse_iterator::operator++, "C++: wxPointList::reverse_iterator::operator++(int) --> const class wxPointList::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxPointList::reverse_iterator & (wxPointList::reverse_iterator::*)()) &wxPointList::reverse_iterator::operator--, "C++: wxPointList::reverse_iterator::operator--() --> class wxPointList::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxPointList::reverse_iterator (wxPointList::reverse_iterator::*)(int)) &wxPointList::reverse_iterator::operator--, "C++: wxPointList::reverse_iterator::operator--(int) --> const class wxPointList::reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxPointList::reverse_iterator::*)(const class wxPointList::reverse_iterator &) const) &wxPointList::reverse_iterator::operator!=, "C++: wxPointList::reverse_iterator::operator!=(const class wxPointList::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxPointList::reverse_iterator::*)(const class wxPointList::reverse_iterator &) const) &wxPointList::reverse_iterator::operator==, "C++: wxPointList::reverse_iterator::operator==(const class wxPointList::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxPointList::const_reverse_iterator file: line:944
			auto & enclosing_class = cl;
			pybind11::class_<wxPointList::const_reverse_iterator, std::shared_ptr<wxPointList::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init<class wxwxPointListNode *, class wxwxPointListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxPointList::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const class wxPointList::reverse_iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxPointList::const_reverse_iterator const &o){ return new wxPointList::const_reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxPoint *const & (wxPointList::const_reverse_iterator::*)() const) &wxPointList::const_reverse_iterator::operator*, "C++: wxPointList::const_reverse_iterator::operator*() const --> class wxPoint *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxPointList::const_reverse_iterator & (wxPointList::const_reverse_iterator::*)()) &wxPointList::const_reverse_iterator::operator++, "C++: wxPointList::const_reverse_iterator::operator++() --> class wxPointList::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxPointList::const_reverse_iterator (wxPointList::const_reverse_iterator::*)(int)) &wxPointList::const_reverse_iterator::operator++, "C++: wxPointList::const_reverse_iterator::operator++(int) --> const class wxPointList::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxPointList::const_reverse_iterator & (wxPointList::const_reverse_iterator::*)()) &wxPointList::const_reverse_iterator::operator--, "C++: wxPointList::const_reverse_iterator::operator--() --> class wxPointList::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxPointList::const_reverse_iterator (wxPointList::const_reverse_iterator::*)(int)) &wxPointList::const_reverse_iterator::operator--, "C++: wxPointList::const_reverse_iterator::operator--(int) --> const class wxPointList::const_reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxPointList::const_reverse_iterator::*)(const class wxPointList::const_reverse_iterator &) const) &wxPointList::const_reverse_iterator::operator!=, "C++: wxPointList::const_reverse_iterator::operator!=(const class wxPointList::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxPointList::const_reverse_iterator::*)(const class wxPointList::const_reverse_iterator &) const) &wxPointList::const_reverse_iterator::operator==, "C++: wxPointList::const_reverse_iterator::operator==(const class wxPointList::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}
